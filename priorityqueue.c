/*-
 * Some implementation details here were taken from the software published by
   John-Mark Gurney.

The Fibonacci heap data structure invented by Fredman and Tarjan in 1984 gives 
a very efficient implementation of the priority queues.  
find a way to minimize the number of operations needed to compute the MST or SP, 
the kind of operations that we are interested in are insert, decrease-key, link, 
and delete-min

The method to reduce the time used by this algorithm is laziness - do work only 
when you must, and then use it to simplify the structure as much as possible so 
that your future work is easy. This way, the user is forced to do many cheap 
operations in order to make the data structure complicated.

Fibonacci heaps make use of heap-ordered trees. A heap-ordered tree is one 
that maintains the heap property, that is, where key(parent) ≤ key(child) 
for all nodes in the tree.
*/
#include "containers.h"
#include "ccl_internal.h"
static PQueue *Create(size_t ElementSize);
static int Add(PQueue *, intptr_t, const void *);
static intptr_t Front(const PQueue *,void *result);
static intptr_t Replace(PQueue *, PQueueElement *, intptr_t);

static int Finalize(PQueue *);
static PQueue *Union(PQueue *, PQueue *);

/*
A Fibonacci heap H is a collection of heap-ordered trees that have the 
following properties:
  1. The roots of these trees are kept in a doubly-linked list (the root list of H),
  2. The root of each tree contains the minimum element in that tree 
     (this follows from being a heap-ordered tree),
  3. We access the heap by a pointer to the tree root with the 
     overall minimum key,
  4. For each node x, we keep track of the degree (also known as the 
     order or rank) of x, which is just the number of children x has; 
     we also keep track of the mark of x, which is a Boolean value.
*/
struct _PQueue {
    PQueueInterface *VTable;
    size_t count;
    unsigned Flags;
    size_t ElementSize;
    int    Log2N;
    struct    _PQueueElement **lognTable;
    struct    _PQueueElement *Minimum;
    struct    _PQueueElement *Root;
    ContainerHeap *Heap;
    unsigned timestamp;
    ContainerAllocator *Allocator;
};

static void Cut(PQueue *, PQueueElement *, PQueueElement *);
static PQueueElement *ExtractMin(PQueue *);
static void DeleteElement(PQueue *h, PQueueElement *x);

struct _PQueueElement {
    int        degree;
/*
We say that x is marked if its mark is set to true, and that it is unmarked 
if its mark is set to false. A root is always unmarked. We mark x if it is 
not a root and it loses a child (i.e., one of its children is cut and put 
into the root-list). We unmark x whenever it becomes a root.

A node is marked if exactly one of its children has been promoted. 
If some child of a marked node is promoted, we promote (and unmark) 
that node as well. Whenever we promote a marked node, we
unmark it; this is the only way to unmark a node (if splicing 
nodes into the root list during a delete-min is not considered a promotion).
*/
    int        Mark;
    PQueueElement *Parent;
    PQueueElement *Child;
    PQueueElement *Left;
    PQueueElement *Right;
    intptr_t    Key;
    char Data[1];
};

static PQueueElement *NewElement(PQueue *);

#define swap(type, a, b)        \
        do {            \
            type c;        \
            c = a;        \
            a = b;        \
            b = c;        \
        } while (0)        \

#if SLOW
#define INT_BITS        (sizeof(int) * 8)
static int ceillog2(unsigned int a)
{
    int oa;
    int i;
    int b;

    oa = a;
    b = INT_BITS / 2;
    i = 0;
    while (b) {
        i = (i << 1);
        if (a >= (1 << b)) {
            a /= (1 << b);
            i = i | 1;
        } else
            a &= (1 << b) - 1;
        b /= 2;
    }
    if ((1 << i) == oa)
        return i;
    else
        return i + 1;
}
#else
// http://graphics.stanford.edu/~seander/bithacks.html
#define SYSTEM_LITTLE_ENDIAN 1
static int ceillog2(unsigned v)
{
    union { unsigned int u[2]; double d; } t; // temp

    t.u[SYSTEM_LITTLE_ENDIAN] = 0x43300000;
    t.u[!SYSTEM_LITTLE_ENDIAN] = v;
    t.d -= 4503599627370496.0;
    return  ((t.u[SYSTEM_LITTLE_ENDIAN] >> 20) - 0x3FF) + ((v&(v-1))!=0);
}

#endif
static void DeleteElement(PQueue *h, PQueueElement *x)
{
    void *data;
    intptr_t key;

    data = x->Data;
    key = x->Key;

    Replace(h, x, INT_MIN);
    if (ExtractMin(h) != x) {
        abort();
    }
}


static void destroyheap(PQueue *h)
{
    free(h->lognTable);
    free(h);
}

static PQueue *CreateWithAllocator(size_t ElementSize,ContainerAllocator *allocator)
{
    PQueue *n;

    if ((n = allocator->calloc(1,sizeof *n)) == NULL)
        return NULL;

    n->Log2N = -1;
    n->ElementSize = ElementSize;
    n->Heap = iHeap.Create(n->ElementSize + sizeof(PQueueElement), allocator);
    n->Allocator = allocator;
    n->VTable = &iPQueue;
    return n;
}

static PQueue *Create(size_t ElementSize)
{
    return CreateWithAllocator(ElementSize, CurrentAllocator);
}
static int compare(PQueue *h, PQueueElement *a, PQueueElement *b)
{
        if (a->Key < b->Key)
            return -1;
        if (a->Key == b->Key)
            return 0;
        return 1;
}
static int comparedata(PQueue *h, int key, void *data, PQueueElement *b)
{
    PQueueElement a;

    a.Key = key;

    return compare(h, &a, b);
}

static PQueue *Union(PQueue *ha, PQueue *hb)
{
    PQueueElement *x;

    if (ha->Root == NULL || hb->Root == NULL) {
        /* either one or both are empty */
        if (ha->Root == NULL) {
            destroyheap(ha);
            return hb;
        } else {
            destroyheap(hb);
            return ha;
        }
    }
    ha->Root->Left->Right = hb->Root;
    hb->Root->Left->Right = ha->Root;
    x = ha->Root->Left;
    ha->Root->Left = hb->Root->Left;
    hb->Root->Left = x;
    ha->count += hb->count;
    /*
     * we probably should also keep stats on number of unions
     */

    /* set Minimum if necessary */
    if (compare(ha, hb->Minimum, ha->Minimum) < 0)
        ha->Minimum = hb->Minimum;

    destroyheap(hb);
    memset(hb,0,sizeof(*hb));
    return ha;
}

static int Finalize(PQueue *h)
{
    iHeap.Finalize(h->Heap);
    h->Allocator->free(h);
    return 1;
}

static void insertafter(PQueueElement *a, PQueueElement *b)
{
    if (a == a->Right) {
        a->Right = b;
        a->Left = b;
        b->Right = a;
        b->Left = a;
    } else {
        b->Right = a->Right;
        a->Right->Left = b;
        a->Right = b;
        b->Left = a;
    }
}

static void insertrootlist(PQueue *h, PQueueElement *x)
{
    if (h->Root == NULL) {
        h->Root = x;
        x->Left = x;
        x->Right = x;
        return;
    }

    insertafter(h->Root, x);
}
static void insertel(PQueue *h, PQueueElement *x)
{
    insertrootlist(h, x);

    if (h->Minimum == NULL || (x->Key < h->Minimum->Key))
        h->Minimum = x;

    h->count++;
    h->timestamp++;
}
static int Add(PQueue *h, intptr_t key, const void *data)
{
    PQueueElement *x;

    if ((x = NewElement(h)) == NULL) {
        return iError.NullPtrError("iPQueue.Add");
    }

    /* just insert on root list, and make sure it's not the new min */
    if (data) memcpy(x->Data , data,h->ElementSize);
    if (key < CCL_PRIORITY_MIN) key = CCL_PRIORITY_MIN;
    if (key > CCL_PRIORITY_MAX) key = CCL_PRIORITY_MAX;
    x->Key = key;

    insertel(h, x);

    return 1;
}

static intptr_t Front(const PQueue *h,void *result)
{
    if (h->Minimum == NULL)
        return INT_MIN;
    memcpy(result,h->Minimum->Data,h->ElementSize);
    return h->Minimum->Key;
}

static void CascadingCut(PQueue *h, PQueueElement *y)
{
    PQueueElement *z;

    while ((z = y->Parent) != NULL) {
        if (y->Mark == 0) {
            y->Mark = 1;
            return;
        } else {
            Cut(h, y, z);
            y = z;
        }
    }
}
static void * ReplaceKeyData(PQueue *h, PQueueElement *x, intptr_t key, void *data)
{
    void *odata;
    int okey;
    PQueueElement *y;
    int r;

    odata = x->Data;
    okey = x->Key;

    /*
     * we can increase a key by deleting and reinserting, that
     * requires O(lgn) time.
     */
    if ((r = comparedata(h, key, data, x)) > 0) {

        /* XXX - bad code! */
        abort();
        DeleteElement(h, x);

        if (data) memcpy(x->Data , data,h->ElementSize);
        x->Key = key;

        insertel(h, x);

        return odata;
    }

    if (data) memcpy(x->Data , data, h->ElementSize);
    x->Key = key;

    /* because they are equal, we don't have to do anything */
    if (r == 0)
        return odata;

    y = x->Parent;

    if (okey == key)
        return odata;

    if (y != NULL && compare(h, x, y) <= 0) {
        Cut(h, x, y);
        CascadingCut(h, y);
    }

    /*
     * the = is so that the call from delete will delete the proper
     * element.
     */
    if (compare(h, x, h->Minimum) <= 0)
        h->Minimum = x;

    return odata;
}

static intptr_t Replace(PQueue *h, PQueueElement *x, intptr_t key)
{
    intptr_t ret;

    ret = x->Key;
    (void)ReplaceKeyData(h, x, key, x->Data);

    return ret;
}
static PQueueElement *removeNode(PQueueElement *x)
{
    PQueueElement *ret;

    if (x == x->Left)
        ret = NULL;
    else
        ret = x->Left;

    /* fix the parent pointer */
    if (x->Parent != NULL && x->Parent->Child == x)
        x->Parent->Child = ret;

    x->Right->Left = x->Left;
    x->Left->Right = x->Right;

    /* clear out hanging pointers */
    x->Parent = NULL;
    x->Left = x;
    x->Right = x;

    return ret;
}

static void removerootlist(PQueue *h, PQueueElement *x)
{
    if (x->Left == x)
        h->Root = NULL;
    else {
        h->Root = removeNode(x);
        x->Key = INT_MIN;
    }
}


static inline void insertbefore(PQueueElement *a, PQueueElement *b)
{
    insertafter(a->Left, b);
}

static void heaplink(PQueue *h, PQueueElement *y, PQueueElement *x)
{
    /* make y a child of x */
    if (x->Child == NULL)
        x->Child = y;
    else
        insertbefore(x->Child, y);
    y->Parent = x;
    x->degree++;
    y->Mark = 0;
}

static int checkcons(PQueue *h)
{
    int oDl;

    /* make sure we have enough memory allocated to "reorganize" */
    if (h->Log2N == -1 || h->count > (1 << h->Log2N)) {
        oDl = h->Log2N;
        if ((h->Log2N = ceillog2(h->count) + 1) < 8)
            h->Log2N = 8;
        if (oDl != h->Log2N)
            h->lognTable = (PQueueElement **)realloc(h->lognTable,
                sizeof *h->lognTable * (h->Log2N + 1));
        if (h->lognTable == NULL) {
            iError.RaiseError("iPriorityQueue.Add",CONTAINER_ERROR_NOMEMORY);
            return CONTAINER_ERROR_NOMEMORY;
        }
    }
    return 1;
}
/*
This algorithm maintains a global array B[1...⌊logn⌋], where B[i] is a 
pointer to some previously-visited root node of degree i, or Null if 
there is no such previously- visited root node. 

Notice, the Cleanup algorithm simultaneously resets the parent pointers 
of all the new roots and updates the pointer to the minimum key. The 
part of the algorithm that links possible nodes of equal degree is given 
in a separate subroutine LinkDupes. The subroutine
ensures that no earlier root node has the same degree as the current. 
By the possible swapping of the nodes v and w, we maintain the heap 
property.
*/
static int consolidate(PQueue *h)
{
    PQueueElement **B;
    PQueueElement *w;
    PQueueElement *y;
    PQueueElement *x;
    int i;
    int degree;
    int D;

    i = checkcons(h);
    if (i < 0) return i;

    D = h->Log2N + 1;
    B = h->lognTable;

    for (i = 0; i < D; i++)
        B[i] = NULL;

    while ((w = h->Root) != NULL) {
        x = w;
        removerootlist(h, w);
        degree = x->degree;
        /* Assert(degree < D); */
        while(B[degree] != NULL) {
            y = B[degree];
            if (compare(h, x, y) > 0)
                swap(PQueueElement *, x, y);
            heaplink(h, y, x);
            B[degree] = NULL;
            degree++;
        }
        B[degree] = x;
    }
    h->Minimum = NULL;
    for (i = 0; i < D; i++)
        if (B[i] != NULL) {
            insertrootlist(h, B[i]);
            if (h->Minimum == NULL || compare(h, B[i], h->Minimum) < 0)
                h->Minimum = B[i];
        }
    return 1;
}
/*
First, we remove the minimum key from the root list and splice its 
children into the root list. Except for updating the parent pointers, 
this takes O(1) time. Then we scan through the root list to find the 
new smallest key and update the parent pointers of the new roots. 
This scan could take O(n) time in the worst case. To bring down the 
amortized deletion time (see further on), we apply a Cleanup 
algorithm, which links trees of equal degree until there is only one 
root node of any particular degree.
*/
static PQueueElement * ExtractMin(PQueue *h)
{
    PQueueElement *ret;
    PQueueElement *x, *y, *orig;

    ret = h->Minimum;

    orig = NULL;
    /* put all the children on the root list */
    /* for true consistancy, we should use remove */
    for(x = ret->Child; x != orig && x != NULL;) {
        if (orig == NULL)
            orig = x;
        y = x->Right;
        x->Parent = NULL;
        insertrootlist(h, x);
        x = y;
    }
    /* remove minimum from root list */
    removerootlist(h, ret);
    h->count--;

    /* if we aren't empty, consolidate the heap */
    if (h->count == 0)
        h->Minimum = NULL;
    else {
        h->Minimum = ret->Right;
        consolidate(h);
    }

    h->timestamp++;

    return ret;
}


static void Cut(PQueue *h, PQueueElement *x, PQueueElement *y)
{
    removeNode(x);
    y->degree--;
    insertrootlist(h, x);
    x->Parent = NULL;
    x->Mark = 0;
}

static PQueueElement * NewElement(PQueue *h)
{
    PQueueElement *e;

    e = iHeap.NewObject(h->Heap);
    if (e == NULL) 
        return NULL;

    memset(e,0,sizeof(*e));
    e->Left = e;
    e->Right = e;
    return e;
}


static size_t Size(const PQueue *p)
{
    if (p == NULL)
        return 0;
    return p->count;
}

static size_t Sizeof(const PQueue *p)
{
    size_t result = sizeof(PQueue);

    if (p) {
        result += p->count * sizeof(PQueue);
    }
    return result;
}

static int Clear(PQueue *p)
{
    if (p == NULL) return 0;
    p->count = 0;
    return 1;
}

static intptr_t Pop(PQueue *p,void *result)
{
    PQueueElement *x = ExtractMin(p);

    if (x == NULL) return INT_MAX;
    if (result) memcpy(result,x->Data,p->ElementSize);
    return x->Key;
}

static PQueue *Copy(const PQueue *src)
{
    PQueue *result;
    Iterator *it;
    int r;
    PQueueElement *obj;

    if (src == NULL) return NULL;
    result = CreateWithAllocator(src->ElementSize,src->Allocator);
    if (result == NULL) return NULL;
    it = iHeap.NewIterator(src->Heap);
    for (obj = it->GetFirst(it); obj != NULL; obj = it->GetNext(it)) {
        if (obj->Key != INT_MIN) {
            r = Add(result,obj->Key,obj->Data);
            if (r < 0) {
                Finalize(result);
                return NULL;
            }
        }
    }
    iHeap.deleteIterator(it);
    return result;
}

static int Equal(const PQueue *src1, const PQueue *src2)
{
    Iterator *it1,*it2;
    PQueueElement *obj1,*obj2;

    if (src1 == NULL && src2 == NULL) return 1;
    if (src1 == NULL || src2 == NULL) return 0;
    if ((src1->ElementSize != src2->ElementSize) ||
        (src1->count != src2->count)) return 0;
    it1 = iHeap.NewIterator(src1->Heap);
    it2 = iHeap.NewIterator(src2->Heap);
    for (obj1 = it1->GetFirst(it1),obj2 = it2->GetFirst(it2); obj1 != NULL; obj1 = it1->GetNext(it1),obj2 = it2->GetNext(it2)) {
        if (obj1->Key != obj2->Key) return 0;
        if (memcmp(obj1->Data,obj2->Data,src1->ElementSize)) return 0;
    }
    iHeap.deleteIterator(it1);
    iHeap.deleteIterator(it2);
    return 1;
}


static PQueueElement *FindInLevel(PQueueElement *root,intptr_t key)
{
    PQueueElement *x;
    
    if (root == NULL) return NULL;
    x = root->Right;
    while (x != root) {
        if (x->Key == key) return x;
        x = x->Right;
    }
    return FindInLevel(x->Child, key);
}


static void *Find(PQueue *p,intptr_t key)
{
    PQueueElement *x;
    
    x = FindInLevel(p->Root,key);
    return x;
}

PQueueInterface iPQueue = {
    Add,
    Size,
    Create,
    CreateWithAllocator,
    Equal,
    Sizeof,
    Add,
    Clear,
    Finalize,
    Pop,
    Front,
    Copy,
    Union,
//  ReplaceKeyData,
};
