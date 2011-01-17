#include "containers.h"
#include "ccl_internal.h"

/* Node in a balanced binary tree. */
struct Node {
    struct Node *up;        /* Parent (NULL for root). */
    struct Node *down[2];   /* Left child, right child. */
	char data[1];
};

static const guid BB_TreeGuid = {0xb8fda2f4, 0x2d4b, 0x4033,
{0xa6,0x49,0x31,0x63,0x48,0x9f,0x27,0x18}
};

/* A balanced binary tree. */
struct tagBB_Tree {
	BB_TreeInterface *VTable;
    size_t count;                /* Current node count. */
    struct Node *root;       /* Tree's root, NULL if empty. */
    CompareFunction compare;   /* To compare nodes. */
	ErrorFunction RaiseError;
    CompareInfo *aux;            /* Auxiliary data. */
	size_t ElementSize;
    size_t max_size;            /* Max size since last complete rebalance. */
	unsigned Flags;
	unsigned timestamp;
	ContainerHeap *Heap;
	ContainerMemoryManager *Allocator;
};


#include <limits.h>
#ifndef _MSC_VER
#include <stdbool.h>
#include <stdint.h>
#else
#include "stdint.h"
#if _MSC_VER <= 1500
#define inline
#endif
#endif

static void rebalance_subtree (BB_Tree *, struct Node *, size_t);
static struct Node **down_link (BB_Tree *, struct Node *);
static struct Node *sibling (struct Node *p);
static size_t count_nodes_in_subtree (const struct Node *);

static size_t floor_log2 (size_t);
static size_t calculate_h_alpha (size_t);
static BB_Tree *CreateWithAllocator(size_t ElementSize,ContainerMemoryManager *m);


/* Inserts the given NODE into BT.
   Returns a null pointer if successful.
   Returns the existing node already in BT equal to NODE, on
   failure. */
static struct Node *insert(BB_Tree *bt, struct Node *node)
{
  size_t depth = 0;

  node->down[0] = NULL;
  node->down[1] = NULL;

  if (bt->root == NULL) {
      bt->root = node;
      node->up = NULL;
    }
  else {
      struct Node *p = bt->root;
      for (;;) {
          int cmp, dir;

		  cmp = bt->compare(node->data, p->data, bt->aux);
          if (cmp == 0)
            return p;
          depth++;

          dir = cmp > 0;
          if (p->down[dir] == NULL)
            {
              p->down[dir] = node;
              node->up = p;
              break;
            }
          p = p->down[dir];
        }
    }

  bt->count++;
  if (bt->count > bt->max_size)
    bt->max_size = bt->count;
  if (depth > calculate_h_alpha (bt->count)) {
      /* We use the "alternative" method of finding a scapegoat
         node described by Galperin and Rivest. */
      struct Node *s = node;
      size_t size = 1;
      size_t i;

      for (i = 1; ; i++)
        if (i < depth) {
            size += 1 + count_nodes_in_subtree (sibling (s));
            s = s->up;
            if (i > calculate_h_alpha (size)) {
                rebalance_subtree (bt, s, size);
                break;
              }
          }
        else {
            rebalance_subtree (bt, bt->root, bt->count);
            bt->max_size = bt->count;
            break;
          }
    }
	bt->timestamp++;
  return NULL;
}

/* Deletes P from BT. */
static void Delete(BB_Tree *bt, struct Node *p)
{
  struct Node **q = down_link (bt, p);
  struct Node *r = p->down[1];
  if (r == NULL)
    {
      *q = p->down[0];
      if (*q)
        (*q)->up = p->up;
    }
  else if (r->down[0] == NULL)
    {
      r->down[0] = p->down[0];
      *q = r;
      r->up = p->up;
      if (r->down[0] != NULL)
        r->down[0]->up = r;
    }
  else
    {
      struct Node *s = r->down[0];
      while (s->down[0] != NULL)
        s = s->down[0];
      r = s->up;
      r->down[0] = s->down[1];
      s->down[0] = p->down[0];
      s->down[1] = p->down[1];
      *q = s;
      if (s->down[0] != NULL)
        s->down[0]->up = s;
      s->down[1]->up = s;
      s->up = p->up;
      if (r->down[0] != NULL)
        r->down[0]->up = r;
    }
  bt->count--;

  /* We approximate .707 as .75 here.  This is conservative: it
     will cause us to do a little more rebalancing than strictly
     necessary to maintain the scapegoat tree's height
     invariant. */
  if (bt->count < bt->max_size * 3 / 4 && bt->count > 0)
    {
      rebalance_subtree (bt, bt->root, bt->count);
      bt->max_size = bt->count;
    }

	iHeap.AddToFreeList(bt->Heap,p);
	bt->timestamp++;
}

/* Returns the node with minimum value in BT, or a null pointer
   if BT is empty. */
static struct Node *bt_first (const BB_Tree *bt)
{
  struct Node *p = bt->root;
  if (p != NULL)
    while (p->down[0] != NULL)
      p = p->down[0];
  return p;
}

/* Returns the node with maximum value in BT, or a null pointer
   if BT is empty. */
static struct Node *bt_last (const BB_Tree *bt)
{
  struct Node *p = bt->root;
  if (p != NULL)
    while (p->down[1] != NULL)
      p = p->down[1];
  return p;
}
/* Searches BT for a node equal to TARGET.
   Returns the node if found, or a null pointer otherwise. */
static struct Node *find (const BB_Tree *bt, void *data)
{
  const struct Node *p;
  int cmp;

  for (p = bt->root; p != NULL; p = p->down[cmp > 0])
    {
		cmp = bt->compare (data, p->data, bt->aux);
      if (cmp == 0)
        return (struct Node *) p;
    }

  return NULL;
}

/* Returns the node in BT following P in in-order.
   If P is null, returns the minimum node in BT.
   Returns a null pointer if P is the maximum node in BT or if P
   is null and BT is empty. */
static struct Node *bt_next (const BB_Tree *bt, const struct Node *p)
{
  if (p == NULL)
    return bt_first (bt);
  else if (p->down[1] == NULL)
    {
      struct Node *q;
      for (q = p->up; ; p = q, q = q->up)
        if (q == NULL || p == q->down[0])
          return q;
    }
  else
    {
      p = p->down[1];
      while (p->down[0] != NULL)
        p = p->down[0];
      return (struct Node *)p;
    }
}

/* Returns the node in BT preceding P in in-order.
   If P is null, returns the maximum node in BT.
   Returns a null pointer if P is the minimum node in BT or if P
   is null and BT is empty. */
static struct Node *bt_prev (const BB_Tree *bt, const struct Node *p)
{
  if (p == NULL)
    return bt_last (bt);
  else if (p->down[0] == NULL)
    {
      struct Node *q;
      for (q = p->up; ; p = q, q = q->up)
        if (q == NULL || p == q->down[1])
          return q;
    }
  else
    {
      p = p->down[0];
      while (p->down[1] != NULL)
        p = p->down[1];
      return (struct Node *)p;
    }
}

/* Tree rebalancing.

   This algorithm is from Q. F. Stout and B. L. Warren, "Tree
   Rebalancing in Optimal Time and Space", CACM 29(1986):9,
   pp. 902-908.  It uses O(N) time and O(1) space to rebalance a
   subtree that contains N nodes. */

static void tree_to_vine (struct Node **);
static void vine_to_tree (struct Node **, size_t count);

/* Rebalances the subtree in BT rooted at SUBTREE, which contains
   exactly COUNT nodes. */
static void rebalance_subtree (BB_Tree *bt, struct Node *subtree, size_t count)
{
  struct Node *up = subtree->up;
  struct Node **q = down_link (bt, subtree);
  tree_to_vine (q);
  vine_to_tree (q, count);
  (*q)->up = up;
}

/* Converts the subtree rooted at *Q into a vine (a binary search
   tree in which all the right links are null), and updates *Q to
   point to the new root of the subtree. */
static void tree_to_vine (struct Node **q)
{
  struct Node *p = *q;
  while (p != NULL)
    if (p->down[1] == NULL)
      {
        q = &p->down[0];
        p = *q;
      }
    else
      {
        struct Node *r = p->down[1];
        p->down[1] = r->down[0];
        r->down[0] = p;
        p = r;
        *q = r;
      }
}

/* Performs a compression transformation COUNT times, starting at
   *Q, and updates *Q to point to the new root of the subtree. */
static void compress (struct Node **q, size_t count)
{
  while (count--)
    {
      struct Node *red = *q;
      struct Node *black = red->down[0];

      *q = black;
      red->down[0] = black->down[1];
      black->down[1] = red;
      red->up = black;
      if (red->down[0] != NULL)
        red->down[0]->up = red;
      q = &black->down[0];
    }
}

/* Converts the vine rooted at *Q, which contains exactly COUNT
   nodes, into a balanced tree, and updates *Q to point to the
   new root of the balanced tree. */
static void vine_to_tree (struct Node **q, size_t count)
{
  size_t leaf_nodes = count + 1 - ((size_t) 1 << floor_log2 (count + 1));
  size_t vine_nodes = count - leaf_nodes;

  compress (q, leaf_nodes);
  while (vine_nodes > 1)
    {
      vine_nodes /= 2;
      compress (q, vine_nodes);
    }
  while ((*q)->down[0] != NULL)
    {
      (*q)->down[0]->up = *q;
      q = &(*q)->down[0];
    }
}

static int Equal(BB_Tree *t1,BB_Tree *t2)
{
	struct Node *pt1,*pt2;
	if (t1 == t2)
		return 1;
	if (t1 == NULL || t2 == NULL)
		return  0;
	if (t1->count != t2->count)
		return 0;
	if (t1->Allocator != t2->Allocator)
		return 0;
	if (t1->compare != t2->compare)
		return 0;
	if (t1->ElementSize != t2->ElementSize)
		return 0;
	if (t1->Flags != t2->Flags)
		return 0;
	pt1 = bt_first(t1);
	pt2 = bt_first(t2);
	while (pt1 && pt2) {
		if (t1->compare(pt1->data,pt2->data,t1->aux))
			break;
		pt1 = bt_next(t1,pt1);
		pt2 = bt_next(t2,pt2);
	}
	return 1;
}

static BB_Tree *Copy(BB_Tree *src)
{
	BB_Tree *result;
	struct Node *pSrc;

	if (src == NULL) {
		iError.RaiseError("Copy",CONTAINER_ERROR_BADARG);
		return NULL;
	}
	pSrc = bt_first(src);
	result = CreateWithAllocator(src->ElementSize,src->Allocator);
	while (pSrc) {
		iBB_Tree.Add(result,pSrc->data);
		pSrc = bt_next(src,pSrc);
	}
	return result;
}

/* Other binary tree helper functions. */

/* Returns the address of the pointer that points down to P
   within BT. */
static struct Node **down_link (BB_Tree *bt, struct Node *p)
{
  struct Node *q = p->up;
  return q != NULL ? &q->down[q->down[0] != p] : &bt->root;
}

/* Returns node P's sibling; that is, the other child of its
   parent.  P must not be the root. */
static struct Node *sibling (struct Node *p)
{
  struct Node *q = p->up;
  return q->down[q->down[0] == p];
}

/* Returns the number of nodes in the given SUBTREE. */
/* This is an in-order traversal modified to iterate only the
 nodes in SUBTREE. */
static size_t count_nodes_in_subtree (const struct Node *subtree)
{
	size_t count;
	const struct Node *p;

	if (subtree == NULL)
		return 0;
	count = 0;
	p = subtree;
    while (p->down[0] != NULL)
        p = p->down[0];
    for (;;) {
		count++;
		if (p->down[1] != NULL) {
			p = p->down[1];
			while (p->down[0] != NULL)
				p = p->down[0];
		}
		else {
			for (;;) {
				const struct Node *q;
				if (p == subtree)
					goto done;
				q = p;
				p = p->up;
				if (p->down[0] == q)
					break;
			}
		}
	}
 done:
  return count;
}

static size_t Size(BB_Tree *tree)
{
	return count_nodes_in_subtree(tree->root);
}
/* Arithmetic. */

/* Returns the number of high-order 0-bits in X.
   Undefined if X is zero. */
static size_t count_leading_zeros (size_t x)
{
  /* This algorithm is from _Hacker's Delight_ section 5.3. */
  size_t y;
  size_t n;

#define COUNT_STEP(BITS) y = x >> BITS; if (y != 0){n -= BITS; x = y; }

  n = sizeof (size_t) * CHAR_BIT;
#if SIZE_MAX >> 31 >> 31 >> 2
  COUNT_STEP (64);
#endif
#if SIZE_MAX >> 31 >> 1
  COUNT_STEP (32);
#endif
  COUNT_STEP (16);
  COUNT_STEP (8);
  COUNT_STEP (4);
  COUNT_STEP (2);
  y = x >> 1;
  return y != 0 ? n - 2 : n - x;
}

/* Returns floor(log2(x)).
   Undefined if X is zero. */
static size_t floor_log2 (size_t x)
{
  return sizeof (size_t) * CHAR_BIT - 1 - count_leading_zeros (x);
}

/* Returns floor(pow(sqrt(2), x * 2 + 1)).
   Defined for X from 0 up to the number of bits in size_t minus
   1. */
static size_t pow_sqrt2 (size_t x)
{
  /* These constants are sqrt(2) multiplied by 2**63 or 2**31,
     respectively, and then rounded to nearest. */
#if SIZE_MAX >> 31 >> 1
  return (UINT64_C(0xb504f333f9de6484) >> (63 - x)) + 1;
#else
  return (0xb504f334 >> (31 - x)) + 1;
#endif
}

/* Returns floor(log(n)/log(sqrt(2))).
   Undefined if N is 0. */
static size_t calculate_h_alpha (size_t n)
{
  size_t log2 = floor_log2 (n);

  /* The correct answer is either 2 * log2 or one more.  So we
     see if n >= pow(sqrt(2), 2 * log2 + 1) and if so, add 1. */
  return (2 * log2) + (n >= pow_sqrt2 (log2));
}

static unsigned GetFlags(BB_Tree *t)
{
	return t->Flags;
}

static unsigned SetFlags(BB_Tree *t,unsigned newFlags)
{
	unsigned oldFlags = t->Flags;
	t->Flags = newFlags;
	return oldFlags;
}

static int Add(BB_Tree *tree, void *Data)
{
	struct Node *p;
	CompareInfo cInfo;

	cInfo.ExtraArgs = NULL;
	cInfo.Container = tree;
	p = iHeap.newObject(tree->Heap);
	if (p) {
		memcpy(p->data ,Data,tree->ElementSize);
	}
	else {
		iError.RaiseError("BB_Tree.Add",CONTAINER_ERROR_NOMEMORY);
		return CONTAINER_ERROR_NOMEMORY;
	}
	tree->aux = &cInfo;
	insert(tree, p);
	tree->aux = NULL;
	return 1;
}
static int Insert(BB_Tree *tree, const void *Data, void *ExtraArgs)
{
	struct Node *p;
	CompareInfo cInfo;

	cInfo.ExtraArgs = ExtraArgs;
	cInfo.Container = tree;
	tree->aux = &cInfo;
	p = iHeap.newObject(tree->Heap);
	tree->aux = NULL;
	if (p == NULL)
		return 0;
	memcpy(p->data,Data,tree->ElementSize);
	p = insert(tree, p);
	if (p) {
		memcpy(p->data ,Data,tree->ElementSize);
		return 1;
	}
	return 0;

}

static void *Find(BB_Tree *tree,void *data,void *ExtraArgs)
{
	struct Node *p;
	CompareInfo cInfo;

	cInfo.ExtraArgs = ExtraArgs;
	cInfo.Container = tree;
	tree->aux = &cInfo;
	p = find(tree, data);
	tree->aux = NULL;
	if (p) {
		return p->data;
	}
	return NULL;
}

static int Erase(BB_Tree *tree, void * element)
{
	struct Node *n;
	n = find(tree,element);
	if (n == NULL)
		return 0;
	Delete(tree,n);
	return 1;
}
#define BST_MAX_HEIGHT 40
struct BB_TreeIterator {
	Iterator it;
	BB_Tree *bst_table;
	struct Node *bst_node;
	size_t timestamp;
	size_t bst_height;
	struct Node *bst_stack[BST_MAX_HEIGHT];
	unsigned long Flags;
};

/* Returns the next data item in inorder
 within the tree being traversed with |trav|,
 or if there are no more data items returns |NULL|. */
static void *GetNext(Iterator *itrav)
{
	struct BB_TreeIterator *trav = (struct BB_TreeIterator *)itrav;

	if (trav->timestamp != trav->bst_table->timestamp)
		return NULL;
	trav->bst_node = bt_next(trav->bst_table, trav->bst_node);
	if (trav->bst_node == NULL)
		return NULL;
	return trav->bst_node->data;
}

static void *GetPrevious(Iterator *itrav)
{
	struct BB_TreeIterator *trav = (struct BB_TreeIterator *)itrav;

	if (trav == NULL) {
		iError.RaiseError("GetPrevious",CONTAINER_ERROR_BADARG);
		return NULL;
	}
	if (trav->timestamp != trav->bst_table->timestamp)
		return NULL;
	trav->bst_node = bt_prev(trav->bst_table, trav->bst_node);
	if (trav->bst_node == NULL)
		return NULL;
	return trav->bst_node->data;
}

static void *GetFirst(Iterator *itrav)
{
	struct BB_TreeIterator *trav = (struct BB_TreeIterator *)itrav;

	if (trav->timestamp != trav->bst_table->timestamp)
		return NULL;
	trav->bst_node	= bt_first(trav->bst_table);
	if (trav->bst_node)
		return trav->bst_node->data;
	return NULL;
}

static Iterator *newIterator(BB_Tree *tree)
{
	struct BB_TreeIterator *result = MALLOC(tree,sizeof(struct BB_TreeIterator));
	if (result == NULL)
		return NULL;
	memset(result,0,sizeof(struct BB_TreeIterator));
	result->it.GetNext = GetNext;
	result->it.GetPrevious = GetPrevious;
	result->it.GetFirst = GetFirst;
	result->bst_table = tree;
	result->timestamp = tree->timestamp;
	return &result->it;
}

static int deleteIterator(Iterator *it)
{
	struct BB_TreeIterator *itbb = (struct BB_TreeIterator *)it;
	itbb->bst_table->Allocator->free(it);
	return 1;
}
static CompareFunction SetCompareFunction(BB_Tree *l,CompareFunction fn)
{
	CompareFunction oldfn = l->compare;

	if (fn != NULL) /* Treat NULL as an enquiry to get the compare function */
		l->compare = fn;
	return oldfn;
}

static ErrorFunction SetErrorFunction(BB_Tree *tree,ErrorFunction fn)
{
	ErrorFunction old;
	old = tree->RaiseError;
	tree->RaiseError = (fn) ? fn : iError.EmptyErrorFunction;
	return old;
}

static size_t Sizeof(BB_Tree *tree)
{
	size_t result = sizeof(BB_Tree);
	result += tree->count * (tree->ElementSize + sizeof(struct Node));
	return result;
}

static int Clear(BB_Tree *tree)
{

	iHeap.Finalize(tree->Heap);
	tree->Heap = iHeap.Create(tree->ElementSize+sizeof(struct Node),CurrentMemoryManager);
    tree->count = 0;
	tree->root=0;
    tree->max_size=0;            /* Max size since last complete rebalance. */
	tree->Flags=0;
	tree->timestamp=0;
	return 1;
}

static int Finalize(BB_Tree *tree)
{
	tree->VTable->Clear(tree);
	tree->Allocator->free(tree);
	return 1;
}

static int Apply(BB_Tree *tree,int (*Applyfn)(const void *data,void *arg),void *arg)
{
	Iterator *it = newIterator(tree);
	void *obj;

	for (obj = it->GetFirst(it);
		 obj != NULL;
		 obj = it->GetNext(it)) {
		Applyfn(obj,arg);
	}
	return 1;
}

static int DefaultTreeCompareFunction(const void *left,const void *right,CompareInfo *ExtraArgs)
{
	size_t siz=((BB_Tree *)ExtraArgs->Container)->ElementSize;
	return memcmp(left,right,siz);
}

static size_t GetElementSize(BB_Tree *d)
{
	if (d == NULL) return (size_t)CONTAINER_ERROR_BADARG;
	return d->ElementSize;
}

static int Contains(BB_Tree *d, void *element)
{
	if (Find(d,element,NULL))
		return 1;
	return 0;
}

static BB_Tree *CreateWithAllocator(size_t ElementSize,ContainerMemoryManager *m)
{
	BB_Tree *result;

	if (m == NULL)
		m = CurrentMemoryManager;
	result = m->malloc(sizeof(*result));
	if (result == NULL)
		return NULL;
	memset(result,0,sizeof(*result));
	result->VTable = &iBB_Tree;
	result->RaiseError = iError.RaiseError;
	result->compare = DefaultTreeCompareFunction;
	result->Heap = iHeap.Create(ElementSize+sizeof(struct Node),m);
	result->Allocator = m;
	result->ElementSize = roundup(ElementSize);
	return result;
}

static BB_Tree *Create(size_t ElementSize)
{
	return CreateWithAllocator(ElementSize,CurrentMemoryManager);
}

static size_t DefaultSaveFunction(const void *element,void *arg, FILE *Outfile)
{
    const unsigned char *str = element;
    size_t len = *(size_t *)arg;

    return fwrite(str,1,len,Outfile);
}

static int Save(BB_Tree *src,FILE *stream, SaveFunction saveFn,void *arg)
{
	struct Node *rvp;
	if (src == NULL) {
		iError.RaiseError("Save",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
	}
	if (stream == NULL) {
		src->RaiseError("Save",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
	}
	if (saveFn == NULL) {
		saveFn = DefaultSaveFunction;
	}
    if (fwrite(&BB_TreeGuid,sizeof(guid),1,stream) <= 0)
        return EOF;
	if (arg == NULL) {
		arg = &src->ElementSize;
	}
    if (fwrite(src,1,sizeof(BB_Tree),stream) <= 0)
        return EOF;
    rvp = bt_first(src);
    while (rvp) {
		char *p = rvp->data;

        if (saveFn(p,arg,stream) <= 0)
            return EOF;
        rvp = bt_next(src,rvp);
    }
	return 1;
}

static size_t DefaultLoadFunction(void *element,void *arg, FILE *Infile)
{
    size_t len = *(size_t *)arg;

    return fread(element,1,len,Infile);
}

static BB_Tree *Load(FILE *stream, ReadFunction loadFn,void *arg)
{
    size_t i,elemSize;
    BB_Tree *result,L;
    char *buf;
    int r;
    guid Guid;

    if (stream == NULL) {
        iError.RaiseError("Load",CONTAINER_ERROR_BADARG);
        return NULL;
    }
    if (loadFn == NULL) {
        loadFn = DefaultLoadFunction;
        arg = &elemSize;
    }
    if (fread(&Guid,sizeof(guid),1,stream) <= 0) {
        iError.RaiseError("Load",CONTAINER_ERROR_FILE_READ);
        return NULL;
    }
    if (memcmp(&Guid,&BB_TreeGuid,sizeof(guid))) {
        iError.RaiseError("Load",CONTAINER_ERROR_WRONGFILE);
        return NULL;
    }
    if (fread(&L,1,sizeof(BB_Tree),stream) <= 0) {
        iError.RaiseError("Load",CONTAINER_ERROR_FILE_READ);
        return NULL;
    }
    elemSize = L.ElementSize;
    buf = malloc(L.ElementSize);
    if (buf == NULL) {
        iError.RaiseError("Load",CONTAINER_ERROR_NOMEMORY);
        return NULL;
    }
    result = Create(L.ElementSize);
    if (result == NULL) {
        iError.RaiseError("Load",CONTAINER_ERROR_NOMEMORY);
        return NULL;
    }
    result->Flags = L.Flags;
	r = 1;
    for (i=0; i < L.count; i++) {
        if (loadFn(buf,arg,stream) <= 0) {
			r = CONTAINER_ERROR_FILE_READ;
            break;
        }
        if ((r=Add(result,buf)) < 0) {
            break;
        }
    }
    free(buf);
	if (r < 0) {
            iError.RaiseError("Load",r);
            Finalize(result);
            result = NULL;
	}
    return result;
}



BB_TreeInterface iBB_Tree = {
	Size,
    GetFlags,
    SetFlags,
    Clear,
	Contains,
    Erase,
    Finalize,
    Apply,
	Equal,
	Copy,
	SetErrorFunction,
	Sizeof,
	newIterator,
	deleteIterator,
	Save,
	Add,
	Insert,
	Find,
	SetCompareFunction,
	CreateWithAllocator,
	Create,
	GetElementSize,
	Load,
};


