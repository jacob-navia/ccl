/*
This implements search trees of type 2 (Brass). This trees have no key and the
data is the key. Sometimes this trees are called node trees, to distinguish them
from leaf trees, where all the data is stored in the leaves, storing in the nodes
only the key to the data.

The implementation is roughly based on the algorithms described in "Algorithms
with C" by Kyle Loudon, and "C unleashed" of Heathfield, Kirby et al.
*/
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include "containers.h"
#include "ccl_internal.h"

/*  The tree nodes. */
typedef struct tagBinarySearchTreeNode {
	char               hidden;
	signed char        factor; /* Thanks to Alan Curry for this */
	struct tagBinarySearchTreeNode *left;
	struct tagBinarySearchTreeNode *right;
	char               data[MINIMUM_ARRAY_INDEX];
} BinarySearchTreeNode;

static int InsertLeft(BinarySearchTree *tree, BinarySearchTreeNode *node, const void *data);
static int InsertRight(BinarySearchTree *tree, BinarySearchTreeNode *node, const void *data);
static void RemoveLeft(BinarySearchTree *tree, BinarySearchTreeNode *node);
static void RemoveRight(BinarySearchTree *tree, BinarySearchTreeNode *node);
/*  Define balance factors for AVL trees. */
#define LEFT   1
#define BALANCED     0
#define RIGHT -1
#define btreeZero(tree) memset(tree,0,sizeof(*tree))
/* Returns the number of elements stored */
static size_t GetCount(BinarySearchTree *ST);
/* Get the flags */
static unsigned GetFlags(BinarySearchTree *ST);
/* Sets the flags */
static unsigned SetFlags(BinarySearchTree *ST, unsigned flags);
/* Adds one element. Given data is copied */
static int Add(BinarySearchTree *ST, const void *Data);
static int Insert(BinarySearchTree *ST, const void *Data,void *ExtraArgs);
/* Clears all data and frees the memory */
static int Clear(BinarySearchTree *ST);
/* Removes the given data if found */
static int Remove(BinarySearchTree *ST, const void *,void *);
/* Frees the memory used by the tree */
static int Finalize(BinarySearchTree *ST);
/* Calls the given function for all nodes. "Arg" is a used supplied argument */
/* that can be NULL that is passed to the function to call */
static int Apply(BinarySearchTree *ST,int (*Applyfn)(const void *data,void *arg),void *arg);
/* Find a given data in the tree */
static void *Find(BinarySearchTree *ST,void *data);
/* Set or unset the error function */
static ErrorFunction SetErrorFunction(BinarySearchTree *ST, ErrorFunction fn);
static CompareFunction SetCompareFunction(BinarySearchTree *ST,CompareFunction fn);
static size_t Sizeof(BinarySearchTree *ST);
static int DefaultCompareFunction(const void *arg1, const void *arg2, CompareInfo *ExtraArgs);
static BinarySearchTree *Merge(BinarySearchTree *left, BinarySearchTree *right, const void *data);
static int Equal(const BinarySearchTree *left, const BinarySearchTree *right);
static Iterator *NewIterator(BinarySearchTree *);
static int deleteIterator(Iterator *);

struct tagBinarySearchTree {
	struct tagBinarySearchTreeInterface *VTable;
	unsigned             Flags;
	size_t               count;
	size_t               ElementSize;
	ErrorFunction        RaiseError;
	BinarySearchTreeNode *root;
	ContainerAllocator *Allocator;
	DestructorFunction DestructorFn;
} ;

static const guid BinarySearchTreeGuid = {0x9a011719, 0x22ac, 0x461d,
{0x89,0xa1,0x75,0xd4,0x4b,0x85,0x53,0xfb}
};

static BinarySearchTree * Create(size_t ElementSize)
{
	BinarySearchTree *result = CurrentAllocator->malloc(sizeof(BinarySearchTree));
	if (result) {
		memset(result,0,sizeof(BinarySearchTree));
		result->ElementSize = ElementSize;
		result->VTable = &iBinarySearchTree;
		result->Allocator = CurrentAllocator;
	}
	return result;
}


static unsigned SetFlags(BinarySearchTree *l,unsigned newval)
{
	int result;

	result = l->Flags;
	l->Flags = newval;
	return result;
}

static unsigned GetFlags(BinarySearchTree *l)
{
	return l->Flags;
}

static size_t GetCount(BinarySearchTree *l)
{
	return l->count;
}

static BinarySearchTreeNode *newTreeNode(BinarySearchTree *tree,const void *data)
{
	BinarySearchTreeNode *new_node = tree->Allocator->malloc(sizeof(BinarySearchTreeNode)+tree->ElementSize);
	if (new_node) {
		memset(new_node,0,sizeof(*new_node));
		memcpy(new_node->data, data, tree->ElementSize);
		tree->count++;
	}
	return new_node;
}

static int InsertLeft(BinarySearchTree *tree, BinarySearchTreeNode *node, const void *data)
{
	BinarySearchTreeNode *new_node,**position;
	if (node == NULL) {
		if (tree->count > 0)
			return -1;
		position = &tree->root;
	}
	else {
		/*  Normally allow insertion only at the end of a branch. */
		if (node->left != NULL)
			return -1;
		position = &node->left;
	}
	new_node = newTreeNode(tree,data);
	if (new_node == NULL)
		return -1;
	*position = new_node;
	return 0;
}
static int InsertRight(BinarySearchTree *tree, BinarySearchTreeNode *node, const void *data)
{
	BinarySearchTreeNode         *new_node, **position;
	if (node == NULL) {
		if (tree->count > 0)
			return -1;
		position = &tree->root;
	}
	else {
		if (node->right != NULL)
			return -1;
		position = &node->right;
	}
	new_node = newTreeNode(tree,data);
	if (new_node == NULL)
		return -1;
	*position = new_node;
	return 0;
}
static void RemoveLeft(BinarySearchTree *tree, BinarySearchTreeNode *node)
{
	BinarySearchTreeNode         **position;
	if (tree->count == 0)
		return;
	if (node == NULL)
		position = &tree->root;
	else
		position = &node->left;
	if (*position != NULL) {
		RemoveLeft(tree, *position);
		RemoveRight(tree, *position);
		if (tree->DestructorFn)
			tree->DestructorFn(*position);
		tree->Allocator->free(*position);
		*position = NULL;
		tree->count--;
	}
}
static void RemoveRight(BinarySearchTree *tree, BinarySearchTreeNode *node)
{
	BinarySearchTreeNode         **position;
	if (tree->count == 0)
		return;
	if (node == NULL)
		position = &tree->root;
	else
		position = &node->right;
	if (*position != NULL) {
		RemoveLeft(tree, *position);
		RemoveRight(tree, *position);
		if (tree->DestructorFn)
			tree->DestructorFn(*position);
		tree->Allocator->free(*position);
		*position = NULL;
		tree->count--;
	}
}

static BinarySearchTree *Merge(BinarySearchTree *left, BinarySearchTree *right, const void *data)
{
	BinarySearchTree *merge;

	if (left == NULL || right == NULL || left->ElementSize != right->ElementSize)
		return NULL;
	merge = Create(left->ElementSize);
	if (merge == NULL)
		return NULL;
	btreeZero(merge);
	if (InsertLeft(merge, NULL, data) != 0) {
		Finalize(merge);
		return NULL;
	}
	merge->root->left = left->root;
	merge->root->right = right->root;
	merge->count = merge->count + left->count + right->count;
	left->root = NULL;
	left->count = 0;
	right->root = NULL;
	right->count = 0;
	return merge;
}
static void destroy_right(BinarySearchTree *tree, BinarySearchTreeNode *node);
static void rotate_left(BinarySearchTreeNode **node)
{
	BinarySearchTreeNode         *left,*Node = *node;

	left = Node->left;
	if (left->factor == LEFT) {
	   /* Perform an LL rotation. */
	   Node->left = left->right;
	   left->right = Node;
	   Node->factor = BALANCED;
	   left->factor = BALANCED;
	   *node = left;
	   }
	else { /*  Perform an LR rotation. */
	   BinarySearchTreeNode *grandchild = left->right;
	   left->right = grandchild->left;
	   grandchild->left = left;
	   Node->left = grandchild->right;
	   grandchild->right = Node;
	   switch (grandchild->factor) {
		  case LEFT:
		  Node->factor = RIGHT;
		  left->factor = BALANCED;
		  break;
		  case BALANCED:
		  Node->factor = BALANCED;
		  left->factor = BALANCED;
		  break;
		  case RIGHT:
		  Node->factor = BALANCED;
		  left->factor = LEFT;
		  break;
	   }
	   grandchild->factor = BALANCED;
	   *node = grandchild;
	}
}
static void rotate_right(BinarySearchTreeNode **node)
{
	BinarySearchTreeNode *right, *grandchild,*Node = *node;
	right = Node->right;
	if (right->factor == RIGHT) {
	   /* Perform an RR rotation. */
	   Node->right = right->left;
	   right->left = Node;
	   Node->factor = BALANCED;
	   right->factor = BALANCED;
	   *node = right;
	}
	else { /*  Perform an RL rotation. */
	   grandchild = right->left;
	   right->left = grandchild->right;
	   grandchild->right = right;
	   Node->right = grandchild->left;
	   grandchild->left = Node;
	   switch (grandchild->factor) {
		  case LEFT:
		  Node->factor = BALANCED;
		  right->factor = RIGHT;
		  break;
		  case BALANCED:
		  Node->factor = BALANCED;
		  right->factor = BALANCED;
		  break;
		  case RIGHT:
		  Node->factor = LEFT;
		  right->factor = BALANCED;
		  break;
	   }
	   grandchild->factor = BALANCED;
	   *node = grandchild;
	}
}

static void destroy_left(BinarySearchTree *tree, BinarySearchTreeNode *node)
{
	BinarySearchTreeNode         **position;
	/*  Do not allow destruction of an empty tree. */
	if (tree->count > 0) {
		/*  Determine where to destroy nodes. */
		if (node == NULL)
			position = &tree->root;
		else
			position = &node->left;
		/*  Destroy the nodes. */
		if (*position != NULL) {
			destroy_left(tree, *position);
			destroy_right(tree, *position);
			if (tree->DestructorFn)
				tree->DestructorFn(*position);
			
			tree->Allocator->free(*position);
			*position = NULL;
			tree->count--; /* Adjust the size of the tree. */
		}
	}
}
static void destroy_right(BinarySearchTree *tree, BinarySearchTreeNode *node)
{
	BinarySearchTreeNode         **position;
	/*  Do not allow destruction of an empty tree. */
	if (tree->count > 0) {
		if (node == NULL) 	/* Determine where to destroy nodes. */
			position = &tree->root;
		else
			position = &node->right;
		if (*position != NULL) {
			destroy_left(tree, *position);
			destroy_right(tree, *position);
			if (tree->DestructorFn)
				tree->DestructorFn(*position);
			
			tree->Allocator->free(*position);
			*position = NULL;
			tree->count--;
		}
	}
}

static int insert(BinarySearchTree *tree, BinarySearchTreeNode **node, const void *data, int *balanced,void *ExtraArgs)
{
	int cmpval, retval;
	BinarySearchTreeNode *Node = *node;

	/* Insert the data into the tree. */
	if (Node != NULL) {
	   /* Handle insertion into a tree that is not empty. */
	   cmpval = tree->VTable->Compare(data, Node->data,ExtraArgs);
		if (cmpval < 0) { /*  Move to the left. */
		  if (Node->left == NULL) {
			 if (InsertLeft(tree, Node,data) != 0)
				return -1;
			 *balanced = 0;
		  }
		  else {
			 if ((retval = insert(tree, &Node->left, data, balanced,ExtraArgs)) != 0) {
				return retval;
			 }
		  }
		  /* Ensure that the tree remains balanced. */
		  if (!(*balanced)) {
			 switch (Node->factor) {
				case LEFT:
				rotate_left(node);
				*balanced = 1;
				break;
				case BALANCED:
				Node->factor = LEFT;
				break;
				case RIGHT:
				Node->factor = BALANCED;
				*balanced = 1;
			 }
		  }
	   }
	   else if (cmpval > 0) {
	  if (Node->right == NULL) { /* Move to the right */
		 if (InsertRight(tree, Node, data) != 0)
				return -1;
			 *balanced = 0;
		 }
		 else {
			 if ((retval = insert(tree, &Node->right, data, balanced,ExtraArgs)) != 0) {
				return retval;
			 }
		  }
		  if (!(*balanced)) { /* Ensure that the tree remains balanced. */
			 switch (Node->factor) {
				case LEFT:
				Node->factor = BALANCED;
				*balanced = 1;
				break;
				case BALANCED:
				Node->factor = RIGHT;
				break;
				case RIGHT:
				rotate_right(node);
				*balanced = 1;
			 }
		  }
	   }
	   else {  /*  Handle finding a copy of the data. */
		   if (!Node->hidden) {
			 /* Do nothing since the data is in the tree and not hidden. */
			 return 1;
		}
		else { /* Insert the new data and mark it as not hidden. */
			 memcpy(Node->data, data, tree->ElementSize);
			 Node->hidden = 0;
			 /*  Do not rebalance because the tree structure is unchanged. */
			 *balanced = 1;
		  }
	   }
	}
	else {	/* Handle insertion into an empty tree. */
			return InsertLeft(tree, NULL, data);
	}
	return 0;
}


static int Remove(BinarySearchTree *tree, const void *data, void *ExtraArgs)
{
	BinarySearchTreeNode *ap[64];
	char ad[64];
	int k = 1,compare;
	BinarySearchTreeNode *w,*x, **y, *z;
	CompareInfo ci;

	ad[0] = 0;
	ap[0] = tree->root;

	ci.ContainerLeft = tree;
	ci.ContainerRight = NULL;
	ci.ExtraArgs = ExtraArgs;
	z = tree->root;
	for (;;) {
		if (z == NULL)
			return 0;
		compare = tree->VTable->Compare(data,z->data,&ci);
		if (compare == 0)
			break;
		ap[k] = z;
		ad[k] = compare > 0;
		z = (compare > 0) ? z->right : z->left;
		k++;
		if (k >63) {
			tree->RaiseError("Tree too deep. Stack overflow in Remove",CONTAINER_INTERNAL_ERROR);
			return 0;
		}
	}
	if (ad[k-1] == 0) {
		y = &ap[k-1]->left;
	}
	else {
		y = &ap[k-1]->right;
	}

	if (z->right == NULL)
		*y = z->left;
	else {
		x = z->right;
		if (x->left == NULL) {
			x->left = z->left;
			*y = x;
			x->factor = z->factor;
			ad[k] = 1;
			ap[k++] = x;
		}
		else {
			int j;
			w = x->left;
			j = k++;
			ad[k] = 0;
			ap[k++] = x;
			while (w->left) {
				x = w;
				w = x->left;
				ad[k] = 0;
				ap[k++] = x;
			}
			ad[j] = 1;
			ap[j] = w;
			w->left = z->left;
			x->left = w->right;
			w->right = z->right;
			w->factor = z->factor;
			*y = w;
		}
	}
	if (tree->DestructorFn)
		tree->DestructorFn(z);
	
	tree->Allocator->free(z);
	tree->count--;
	if (k < 0) {
		tree->RaiseError("Counter is less than zero in 'Remove'",CONTAINER_INTERNAL_ERROR);
		return 0;
	}
	while (--k) {
		w = ap[k];
		if (ad[k] == 0) {
			if (w->factor == LEFT) {
				w->factor = BALANCED;
				continue;
			}
			else if (w->factor == BALANCED) {
				w->factor = RIGHT;
				break;
			}
			if (w->factor != RIGHT) {
				goto internal_error;
			}
			x = w->right;
			if (x->factor == BALANCED || x->factor == RIGHT) {
				w->right = x->left;
				x->left = w;
				if (ad[k-1] == 0) {
					ap[k-1]->left = x;
				}
				else {
					ap[k-1]->right = x;
				}
				if (x->factor == BALANCED) {
					x->factor = LEFT;
					break;
				}
				w->factor = x->factor = BALANCED;
			}
			else {
				if (x->factor != LEFT) {
				internal_error:
					tree->RaiseError("Memory corruption in 'Remove'",CONTAINER_INTERNAL_ERROR);
					return 0;
				}
				z = x->left;
				x->left = z->right;
				z->right = x;
				w->right = z->left;
				z->left = w;
				if (z->factor == RIGHT) {
					w->factor = LEFT;
					x->factor = BALANCED;
				}
				else if (x->factor == BALANCED) {
					w->factor = x->factor = BALANCED;
				}
				else {
					w->factor = BALANCED;
					x->factor = RIGHT;
				}
				z->factor = BALANCED;
				if (ad[k-1] == 0) {
					ap[k-1]->left = z;
				}
				else {
					ap[k-1]->right = z;
				}
			}
		}
		else {
			if (w->factor == RIGHT) {
				w->factor = BALANCED;
				continue;
			}
			else if (w->factor == BALANCED) {
				w->factor = LEFT;
				break;
			}
			x = w->left;
			if (x->factor == LEFT || x->factor == BALANCED) {
				w->left = x->right;
				x->right = w;
				if (ad[k-1] == 0)
					ap[k-1]->left = x;
				else
					ap[k-1]->right = x;
				if (x->factor == BALANCED) {
					x->factor = RIGHT;
					break;
				}
				else {
					w->factor = x->factor = BALANCED;
				}

			}
			else if (x->factor == RIGHT) {
				z = x->right;
				x->right = z->left;
				z->left = x;
				w->left = z->right;
				z->right = w;
				if (z->factor == LEFT) {
					w->factor = RIGHT;
					x->factor = BALANCED;
				}
				else if (z->factor == BALANCED) {
					w->factor = x->factor = BALANCED;
				}
				else {
					w->factor = BALANCED;
					x->factor = LEFT;
				}
				z->factor = BALANCED;
				if (ad[k-1] == 0)
					ap[k-1]->left = z;
				else
					ap[k-1]->right = z;

			}
		}
	}
	return 1;
}




#if 0
static int hide(BinarySearchTree *tree, BinarySearchTreeNode *node, const void *data,void *ExtraArgs)
{
	int retval=-1;

	if (node) {
		int cmpval = tree->VTable->Compare(data, node->data,ExtraArgs);
		if (cmpval < 0) {  /*  Move to the left. */
			retval = hide(tree, node->left, data, ExtraArgs);
		}
		else if (cmpval > 0) {/* Move to the right. */
			retval = hide(tree, node->right, data,ExtraArgs);
		}
		else {	   /*  Mark the node as hidden. */
			node->hidden = 1;
			retval = 0;
		}
	}
	return retval;
}
#endif
static void *lookup(BinarySearchTree *tree, BinarySearchTreeNode *node, void *data,CompareInfo *ExtraArgs)
{
	void *retval=NULL;

	if (node) {
		int cmpval = tree->VTable->Compare(data, node->data,ExtraArgs);
	if (cmpval < 0) { /*  Move to the left. */
			retval = lookup(tree, node->left, data, ExtraArgs);
		}
		else if (cmpval > 0) {  /*  Move to the right. */
			retval = lookup(tree, node->right, data, ExtraArgs);
		}
		else {
			if (!node->hidden) {
				/*  Pass back the data from the tree. */
				retval = node->data;
			}
		}
	}
	return retval;
}

static int DefaultCompareFunction(const void *arg1, const void *arg2, CompareInfo *ExtraArgs)
{
	BinarySearchTree *tree = (BinarySearchTree *)ExtraArgs->ContainerLeft;
	size_t len = tree->ElementSize;
	return memcmp(arg1,arg2,len);
}

static int Clear(BinarySearchTree *tree)
{
	/* Destroy all nodes in the tree. */
	destroy_left(tree, NULL);
	/*  No operations are allowed now, but clear the structure as a precaution. */
	btreeZero(tree);
	return 1;
}

static int Finalize(BinarySearchTree *tree)
{
	Clear(tree);
	CurrentAllocator->free(tree);
	return 1;
}

static int Insert(BinarySearchTree *tree, const void *data,void *ExtraArgs)
{
	int                balanced = 0;
	CompareInfo ci;

	ci.ContainerLeft = tree;
	ci.ContainerRight = NULL;
	ci.ExtraArgs = ExtraArgs;

	return insert(tree, &tree->root, data, &balanced, &ci);
}

static int Add(BinarySearchTree *tree,const void *data)
{
	int balanced = 0;
	return insert(tree,&tree->root,data,&balanced,tree);
}

static void *Find(BinarySearchTree *tree, void *data)
{
	CompareInfo ci;

	ci.ContainerLeft = tree;
	ci.ContainerRight = NULL;
	ci.ExtraArgs = NULL;
	return lookup(tree, tree->root, data, &ci);
}

static int Visit(BinarySearchTreeNode *node,int (*Applyfn)(const void *data,void *arg),void *arg)
{
	int r=1;
	if (node->left)
		r += Visit(node->left,Applyfn,arg);
	Applyfn(node->data,arg);
	if (node->right)
		r += Visit(node->right,Applyfn,arg);
	return r;
}

static int Apply(BinarySearchTree *tree, int (*Applyfn)(const void *data,void *arg),void *arg)
{
	if (tree->root == NULL)
		return 0;
	return Visit(tree->root,Applyfn,arg);
}

static ErrorFunction SetErrorFunction(BinarySearchTree *ST,ErrorFunction fn)
{
	ErrorFunction old;
	if (ST == NULL) return iError.RaiseError;
	old = ST->RaiseError;
	ST->RaiseError = (fn) ? fn : iError.EmptyErrorFunction;
	return old;
}

static size_t Sizeof(BinarySearchTree *ST)
{
	if (ST == NULL)
		return sizeof(BinarySearchTree);
	return sizeof(*ST) + ST->ElementSize * ST->count + ST->count *sizeof(BinarySearchTreeNode);
}

static CompareFunction SetCompareFunction(BinarySearchTree *l,CompareFunction fn)
{
	CompareFunction oldfn = l->VTable->Compare;

	if (fn != NULL) /* Treat NULL as an enquiry to get the compare function */
		l->VTable->Compare = fn;
	return oldfn;
}

static int compareHeaders(const BinarySearchTree *left,const BinarySearchTree *right)
{
	if (left->count != right->count || left->ElementSize != right->ElementSize)
		return 0;
	if (left->VTable->Compare != right->VTable->Compare)
		return 0;
	return 1;
}

static int compareNodes(const BinarySearchTreeNode *left,const BinarySearchTreeNode *right,CompareInfo *ci)
{
	const BinarySearchTree *tree;

	if (left->hidden != right->hidden || left->factor != right->factor)
		return 0;
	if ((left->left == NULL && right->left != NULL) ||
		(left->left != NULL && right->left == NULL))
		return 0;

	if ((left->right == NULL && right->right != NULL) ||
		(left->right != NULL && right->right == NULL))
		return 0;
	tree = ci->ContainerLeft;
	if (tree->VTable->Compare(left->data,right->data,ci))
		return 0;
	if (left->left) {
		if (!compareNodes(left->left,right->left,ci))
			return 0;
	}
	if (left->right) {
		if (!compareNodes(left->right,right->right,ci))
			return 0;
	}
	return 1;

}
static int Equal(const BinarySearchTree *left, const BinarySearchTree *right)
{
	CompareInfo ci;
	if (!compareHeaders(left,right))
		return 0;
	ci.ExtraArgs = NULL;
	ci.ContainerLeft = (void *)left;
	ci.ContainerRight = NULL;
	return compareNodes(left->root,right->root,&ci);
}

static Iterator *NewIterator(BinarySearchTree *tree)
{
	return NULL;
}

static int deleteIterator(Iterator *tree)
{
	return 1;
}

static int Contains(BinarySearchTree *tree , void *data)
{
	if (Find(tree,data))
		return 1;
	return CONTAINER_ERROR_NOTFOUND;
}
static DestructorFunction SetDestructor(BinarySearchTree *cb,DestructorFunction fn)
{
	DestructorFunction oldfn;
	if (cb == NULL)
		return NULL;
	oldfn = cb->DestructorFn;
	if (fn)
		cb->DestructorFn = fn;
	return oldfn;
}

BinarySearchTreeInterface iBinarySearchTree = {
	GetCount,
	GetFlags,
	SetFlags,
	Clear,
	Contains,
	Remove,
	Finalize,
	Apply,
	Equal,
	Add,
	Insert,
	SetErrorFunction,
	SetCompareFunction,
	Sizeof,
	DefaultCompareFunction,
	Merge,
	NewIterator,
	deleteIterator,
	Create,
	SetDestructor,
};

