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

struct fibheap;
struct FibHeapElement;
typedef int (*voidcmp)(void *, void *);

/* functions for key heaps */
PQueue *Create(size_t ElementSize);
static int Add(PQueue *, intptr_t, void *);
static int Front(PQueue *);
intptr_t fh_replacekey(PQueue *, struct FibHeapElement *, intptr_t);
void *fh_replacekeydata(PQueue *, struct FibHeapElement *, intptr_t, void *);

/* functions for void * heaps */
struct fibheap *fh_makeheap(void);
struct FibHeapElement *fh_insert(PQueue *, void *);

/* shared functions */
void *fh_extractmin(PQueue *);
void *fh_min(PQueue *);
void *fh_replacedata(PQueue *, struct FibHeapElement *, void *);
void *fh_delete(PQueue *, struct FibHeapElement *);
static int Finalize(PQueue *);
PQueue *fh_union(PQueue *, PQueue *);

struct FibHeapElement;

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
    size_t ElementSize;
    int    fh_Dl;
    struct    FibHeapElement **lognTable;
    struct    FibHeapElement *fh_min;
    struct    FibHeapElement *fh_root;
    ContainerHeap *Heap;
    unsigned timestamp;
    ContainerMemoryManager *Allocator;
};

static void fh_insertrootlist(PQueue *, struct FibHeapElement *);
static void fh_removerootlist(PQueue *, struct FibHeapElement *);
static int fh_consolidate(PQueue *);
static void fh_heaplink(PQueue *h, struct FibHeapElement *y, struct FibHeapElement *x);
static void fh_cut(PQueue *, struct FibHeapElement *, struct FibHeapElement *);
static void fh_cascading_cut(PQueue *, struct FibHeapElement *);
static struct FibHeapElement *ExtractMin(PQueue *);
static int fh_checkcons(PQueue *h);
static void fh_destroyheap(PQueue *h);
static int fh_compare(PQueue *h, struct FibHeapElement *a, struct FibHeapElement *b);
static int fh_comparedata(PQueue *h, int key, void *data, struct FibHeapElement *b);
static void fh_insertel(PQueue *h, struct FibHeapElement *x);
static void fh_deleteel(PQueue *h, struct FibHeapElement *x);

/*
 * specific node operations
 */
struct FibHeapElement {
    int    fhe_degree;
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
    int    fhe_mark;
    struct    FibHeapElement *Parent;
    struct    FibHeapElement *Child;
    struct    FibHeapElement *Left;
    struct    FibHeapElement *Right;
    intptr_t    fhe_key;
    char fhe_data[1];
};

static struct FibHeapElement *NewElement(PQueue *);
static void fhe_insertafter(struct FibHeapElement *a, struct FibHeapElement *b);
static inline void fhe_insertbefore(struct FibHeapElement *a, struct FibHeapElement *b);
static struct FibHeapElement *fhe_remove(struct FibHeapElement *a);
#define    fhe_destroy(x)    free((x))

/*
 * general functions
 */

#define swap(type, a, b)        \
        do {            \
            type c;        \
            c = a;        \
            a = b;        \
            b = c;        \
        } while (0)        \

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

/*
 * Private Heap Functions
 */
static void fh_deleteel(PQueue *h, struct FibHeapElement *x)
{
    void *data;
    intptr_t key;

    data = x->fhe_data;
    key = x->fhe_key;

    fh_replacekey(h, x, INT_MIN);
    if (ExtractMin(h) != x) {
        /*
         * XXX - This should never happen as fh_replace should set it
         * to min.
         */
        abort();
    }
}


static void fh_destroyheap(PQueue *h)
{
    free(h->lognTable);
    free(h);
}

PQueue *CreateWithAllocator(size_t ElementSize,ContainerMemoryManager *allocator)
{
    PQueue *n;

    if ((n = allocator->calloc(1,sizeof *n)) == NULL)
        return NULL;

    n->fh_Dl = -1;
    n->ElementSize = ElementSize;
    n->Heap = iHeap.Create(n->ElementSize + sizeof(struct FibHeapElement), allocator);
    n->Allocator = allocator;
    return n;
}

PQueue * fh_union(PQueue *ha, PQueue *hb)
{
    struct FibHeapElement *x;

    if (ha->fh_root == NULL || hb->fh_root == NULL) {
        /* either one or both are empty */
        if (ha->fh_root == NULL) {
            fh_destroyheap(ha);
            return hb;
        } else {
            fh_destroyheap(hb);
            return ha;
        }
    }
    ha->fh_root->Left->Right = hb->fh_root;
    hb->fh_root->Left->Right = ha->fh_root;
    x = ha->fh_root->Left;
    ha->fh_root->Left = hb->fh_root->Left;
    hb->fh_root->Left = x;
    ha->count += hb->count;
    /*
     * we probably should also keep stats on number of unions
     */

    /* set fh_min if necessary */
    if (fh_compare(ha, hb->fh_min, ha->fh_min) < 0)
        ha->fh_min = hb->fh_min;

    fh_destroyheap(hb);
    return ha;
}

static int Finalize(PQueue *h)
{
    iHeap.Finalize(h->Heap);
    h->Allocator->free(h);
    return 1;
}

static int Add(PQueue *h, intptr_t key, void *data)
{
    struct FibHeapElement *x;

    if ((x = NewElement(h)) == NULL) {
        return iError.NullPtrError("iPQueue.Add");
    }

    /* just insert on root list, and make sure it's not the new min */
    if (data) memcpy(x->fhe_data , data,h->ElementSize);
    x->fhe_key = key;

    fh_insertel(h, x);

    return 1;
}

static int Front(PQueue *h)
{
    if (h->fh_min == NULL)
        return INT_MIN;
    return h->fh_min->fhe_key;
}

intptr_t fh_replacekey(PQueue *h, struct FibHeapElement *x, intptr_t key)
{
    intptr_t ret;

    ret = x->fhe_key;
    (void)fh_replacekeydata(h, x, key, x->fhe_data);

    return ret;
}

void * fh_replacekeydata(PQueue *h, struct FibHeapElement *x, intptr_t key, void *data)
{
    void *odata;
    int okey;
    struct FibHeapElement *y;
    int r;

    odata = x->fhe_data;
    okey = x->fhe_key;

    /*
     * we can increase a key by deleting and reinserting, that
     * requires O(lgn) time.
     */
    if ((r = fh_comparedata(h, key, data, x)) > 0) {
        /* XXX - bad code! */
        abort();
        fh_deleteel(h, x);

        if (data) memcpy(x->fhe_data , data,h->ElementSize);
        x->fhe_key = key;

        fh_insertel(h, x);

        return odata;
    }

    if (data) memcpy(x->fhe_data , data, h->ElementSize);
    x->fhe_key = key;

    /* because they are equal, we don't have to do anything */
    if (r == 0)
        return odata;

    y = x->Parent;

    if (okey == key)
        return odata;

    if (y != NULL && fh_compare(h, x, y) <= 0) {
        fh_cut(h, x, y);
        fh_cascading_cut(h, y);
    }

    /*
     * the = is so that the call from fh_delete will delete the proper
     * element.
     */
    if (fh_compare(h, x, h->fh_min) <= 0)
        h->fh_min = x;

    return odata;
}

/*
 * Public void * Heap Functions
 */
/*
 * this will return these values:
 *    NULL    failed for some reason
 *    ptr    token to use for manipulation of data
 */
struct FibHeapElement * fh_insert(PQueue *h, void *data)
{
    struct FibHeapElement *x;

    if ((x = NewElement(h)) == NULL)
        return NULL;

    /* just insert on root list, and make sure it's not the new min */
    if (data) memcpy(x->fhe_data , data, h->ElementSize);

    fh_insertel(h, x);

    return x;
}

void * fh_min(PQueue *h)
{
    if (h->fh_min == NULL)
        return NULL;
    return h->fh_min->fhe_data;
}

void * fh_extractmin(PQueue *h)
{
    struct FibHeapElement *z;
    void *ret;

    ret = NULL;

    if (h->fh_min != NULL) {
        z = ExtractMin(h);
        ret = z->fhe_data;
        fhe_destroy(z);
    }

    return ret;
}

void * fh_replacedata(PQueue *h, struct FibHeapElement *x, void *data)
{
    return fh_replacekeydata(h, x, x->fhe_key, data);
}

void * fh_delete(PQueue *h, struct FibHeapElement *x)
{
    void *k;

    k = x->fhe_data;
    fh_replacekey(h, x, INT_MIN);
    fh_extractmin(h);

    return k;
}

/*
 * Statistics Functions
 */
#ifdef FH_STATS
int
fh_maxn(PQueue *h)
{
    return h->fh_maxn;
}

int fh_ninserts(PQueue *h)
{
    return h->fh_ninserts;
}

int
fh_nextracts(PQueue *h)
{
    return h->fh_nextracts;
}
#endif

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
static struct FibHeapElement * ExtractMin(PQueue *h)
{
    struct FibHeapElement *ret;
    struct FibHeapElement *x, *y, *orig;

    ret = h->fh_min;

    orig = NULL;
    /* put all the children on the root list */
    /* for true consistancy, we should use fhe_remove */
    for(x = ret->Child; x != orig && x != NULL;) {
        if (orig == NULL)
            orig = x;
        y = x->Right;
        x->Parent = NULL;
        fh_insertrootlist(h, x);
        x = y;
    }
    /* remove minimum from root list */
    fh_removerootlist(h, ret);
    h->count--;

    /* if we aren't empty, consolidate the heap */
    if (h->count == 0)
        h->fh_min = NULL;
    else {
        h->fh_min = ret->Right;
        fh_consolidate(h);
    }

    h->timestamp++;

    return ret;
}

static void fh_insertrootlist(PQueue *h, struct FibHeapElement *x)
{
    if (h->fh_root == NULL) {
        h->fh_root = x;
        x->Left = x;
        x->Right = x;
        return;
    }

    fhe_insertafter(h->fh_root, x);
}

static void fh_removerootlist(PQueue *h, struct FibHeapElement *x)
{
    if (x->Left == x)
        h->fh_root = NULL;
    else
        h->fh_root = fhe_remove(x);
}


static void fh_heaplink(PQueue *h, struct FibHeapElement *y, struct FibHeapElement *x)
{
    /* make y a child of x */
    if (x->Child == NULL)
        x->Child = y;
    else
        fhe_insertbefore(x->Child, y);
    y->Parent = x;
    x->fhe_degree++;
    y->fhe_mark = 0;
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
static int fh_consolidate(PQueue *h)
{
    struct FibHeapElement **B;
    struct FibHeapElement *w;
    struct FibHeapElement *y;
    struct FibHeapElement *x;
    int i;
    int degree;
    int D;

    i = fh_checkcons(h);
    if (i < 0) return i;

    D = h->fh_Dl + 1;
    B = h->lognTable;

    for (i = 0; i < D; i++)
        B[i] = NULL;

    while ((w = h->fh_root) != NULL) {
        x = w;
        fh_removerootlist(h, w);
        degree = x->fhe_degree;
        /* Assert(degree < D); */
        while(B[degree] != NULL) {
            y = B[degree];
            if (fh_compare(h, x, y) > 0)
                swap(struct FibHeapElement *, x, y);
            fh_heaplink(h, y, x);
            B[degree] = NULL;
            degree++;
        }
        B[degree] = x;
    }
    h->fh_min = NULL;
    for (i = 0; i < D; i++)
        if (B[i] != NULL) {
            fh_insertrootlist(h, B[i]);
            if (h->fh_min == NULL || fh_compare(h, B[i],
                h->fh_min) < 0)
                h->fh_min = B[i];
        }
    return 1;
}

static void fh_cut(PQueue *h, struct FibHeapElement *x, struct FibHeapElement *y)
{
    fhe_remove(x);
    y->fhe_degree--;
    fh_insertrootlist(h, x);
    x->Parent = NULL;
    x->fhe_mark = 0;
}

static void fh_cascading_cut(PQueue *h, struct FibHeapElement *y)
{
    struct FibHeapElement *z;

    while ((z = y->Parent) != NULL) {
        if (y->fhe_mark == 0) {
            y->fhe_mark = 1;
            return;
        } else {
            fh_cut(h, y, z);
            y = z;
        }
    }
}

static struct FibHeapElement * NewElement(PQueue *h)
{
    struct FibHeapElement *e;

    e = iHeap.newObject(h->Heap);
    if (e == NULL) 
        return NULL;

    memset(e,0,sizeof(*e));
    e->Left = e;
    e->Right = e;
    return e;
}


static void fhe_insertafter(struct FibHeapElement *a, struct FibHeapElement *b)
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

static inline void fhe_insertbefore(struct FibHeapElement *a, struct FibHeapElement *b)
{
    fhe_insertafter(a->Left, b);
}

static struct FibHeapElement * fhe_remove(struct FibHeapElement *x)
{
    struct FibHeapElement *ret;

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

static int fh_checkcons(PQueue *h)
{
    int oDl;

    /* make sure we have enough memory allocated to "reorganize" */
    if (h->fh_Dl == -1 || h->count > (1 << h->fh_Dl)) {
        oDl = h->fh_Dl;
        if ((h->fh_Dl = ceillog2(h->count) + 1) < 8)
            h->fh_Dl = 8;
        if (oDl != h->fh_Dl)
            h->lognTable = (struct FibHeapElement **)realloc(h->lognTable,
                sizeof *h->lognTable * (h->fh_Dl + 1));
        if (h->lognTable == NULL) {
            iError.RaiseError("iPriorityQueue.Add",CONTAINER_ERROR_NOMEMORY);
            return CONTAINER_ERROR_NOMEMORY;
        }
    }
    return 1;
}

static int fh_compare(PQueue *h, struct FibHeapElement *a, struct FibHeapElement *b)
{
        if (a->fhe_key < b->fhe_key)
            return -1;
        if (a->fhe_key == b->fhe_key)
            return 0;
        return 1;
}

static int fh_comparedata(PQueue *h, int key, void *data, struct FibHeapElement *b)
{
    struct FibHeapElement a;

    a.fhe_key = key;

    return fh_compare(h, &a, b);
}

static void fh_insertel(PQueue *h, struct FibHeapElement *x)
{
    fh_insertrootlist(h, x);

    if (h->fh_min == NULL || (x->fhe_key < h->fh_min->fhe_key))
        h->fh_min = x;

    h->count++;
    h->timestamp++;
}

static size_t Size(PQueue *p)
{
	if (p == NULL)
		return 0;
	return p->count;
}

static size_t Sizeof(PQueue *p)
{
	size_t result = sizeof(PQueue);

	if (p) {
		result += p->count * sizeof(PQueue);
	}
	return result;
}

int Clear(PQueue *p)
{
	if (p == NULL) return 0;
	p->count = 0;
	return 1;
}

intptr_t Pop(PQueue *p,void *result)
{
	struct FibHeapElement *x = ExtractMin(p);

	if (x == NULL) return INT_MIN;
	if (result) memcpy(result,x->fhe_data,p->ElementSize);
	return x->fhe_key;
}

PQueueInterface iPriorityQueue = {
	Create,
	CreateWithAllocator,
	Size,
	Sizeof,
	Add,
	Clear,
	Finalize,
	Pop,
	Front,
};
#ifdef TEST

#include <stdio.h>
#include <stdlib.h>

void testreplace(void)
{
	PQueue *a;
	void *arr[10];
	int i;

	a = Create(sizeof(intptr_t));
	
	for (i=1 ; i < 10 ; i++)
	  {
              arr[i]= fh_insertkey(a,i,NULL);
	      printf("adding: 0 %d \n",i);
	  }
     
	printf(" \n");
	 fh_replacekey(a, arr[1],-1);
         fh_replacekey(a, arr[6],-1);
	 fh_replacekey(a, arr[4],-1);
         fh_replacekey(a, arr[2],-1); 
         fh_replacekey(a, arr[8],-1); 
	  
        printf("value(minkey) %d\n",Front(a));
	printf("id: %d\n\n", (int)fh_extractmin(a));          
     
	 fh_replacekey(a, arr[7],-33);
/* -> node 7 becomes root node, but is still pointed to by node 6 */
         fh_replacekey(a, arr[4],-36);
	 fh_replacekey(a, arr[3],-1);
         fh_replacekey(a, arr[9],-81); 	
	
        printf("value(minkey) %d\n",Front(a));
        printf("id: %d\n\n", (int)fh_extractmin(a));
	
	 fh_replacekey(a, arr[6],-68);
         fh_replacekey(a, arr[2],-69);

        printf("value(minkey) %d\n",Front(a));
        printf("id: %d\n\n", (int)fh_extractmin(a));

	 fh_replacekey(a, arr[1],-52);
         fh_replacekey(a, arr[3],-2);
	 fh_replacekey(a, arr[4],-120);
         fh_replacekey(a, arr[5],-48); 	

        printf("value(minkey) %d\n",Front(a));
	printf("id: %d\n\n", (int)fh_extractmin(a));

	 fh_replacekey(a, arr[3],-3);
         fh_replacekey(a, arr[5],-63);

        printf("value(minkey) %d\n",Front(a));
	printf("id: %d\n\n", (int)fh_extractmin(a));

	 fh_replacekey(a, arr[5],-110);
         fh_replacekey(a, arr[7],-115);

        printf("value(minkey) %d\n",Front(a));
	printf("id: %d\n\n", (int)fh_extractmin(a));

         fh_replacekey(a, arr[5],-188);

        printf("value(minkey) %d\n",Front(a));
	printf("id: %d\n\n", (int)fh_extractmin(a));

         fh_replacekey(a, arr[3],-4);

        printf("value(minkey) %d\n",Front(a));
	printf("id: %d\n\n", (int)fh_extractmin(a));
	
        printf("value(minkey) %d\n",Front(a));
        printf("id: %d\n\n", (int)fh_extractmin(a));

	Finalize(a);

}

int main(void)
{
    testreplace();
}
#endif
