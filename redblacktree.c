#include <stdio.h>
#include <stdlib.h>
#include "containers.h"
#include "ccl_internal.h"
/*  The tree nodes. */
typedef struct tagRedBlackTreeNode {
    char                        color;
    char                        flags; /* If it has a data node in the left arm or not */
    struct tagRedBlackTreeNode *left;
    struct tagRedBlackTreeNode *right;
    char               Key[MINIMUM_ARRAY_INDEX];
} RedBlackTreeNode;

struct tagRedBlackTree {
    struct tagRedBlackTreeInterface *VTable;
    unsigned             Flags;
    size_t               count;
    size_t               ElementSize;
    ErrorFunction        RaiseError;
    CompareFunction      KeyCompareFn;
    size_t               KeySize;
    size_t               Available;
    RedBlackTreeNode    *root;
    RedBlackTreeNode    *CurrentBlock;
    RedBlackTreeNode    *FreeList;
    ContainerAllocator *Allocator;
    DestructorFunction DestructorFn;
} ;

#define BLOCKSIZE 256
#include "containers.h"
static size_t GetCount(RedBlackTree *ST);
static unsigned GetFlags(RedBlackTree *ST);
static unsigned SetFlags(RedBlackTree *ST, unsigned flags);
static int Add(RedBlackTree *ST, const void *Key, const void *Data);
static int Insert(RedBlackTree * ST, const void *Key, const void *Data, void *ExtraArgs);
static int Clear(RedBlackTree *ST);
static int Remove(RedBlackTree *ST, const void *,void *);
static int Finalize(RedBlackTree *ST);
static int Apply(RedBlackTree *ST,int (*Applyfn)(const void *data,void *arg),void *arg);
static void *Find(RedBlackTree *ST,void *key_data,void *ExtraArgs);
static ErrorFunction SetErrorFunction(RedBlackTree *ST, ErrorFunction fn);
static CompareFunction SetCompareFunction(RedBlackTree *ST,CompareFunction fn);
static size_t Sizeof(RedBlackTree *ST);
static int DefaultCompareFunction(const void *arg1, const void *arg2, CompareInfo *ExtraArgs);
static Iterator *NewIterator(RedBlackTree *);
static int deleteIterator(Iterator *);
static RedBlackTree *Create(size_t ElementSize,size_t KeySize)
{
    RedBlackTree *result = CurrentAllocator->malloc(sizeof(RedBlackTree));
    if (result) {
        memset(result,0,sizeof(RedBlackTree));
        result->ElementSize = ElementSize;
        result->VTable = &iRedBlackTree;
        result->Allocator = CurrentAllocator;
    }
    return result;
}
static int DefaultCompareFunction(const void *arg1, const void *arg2, CompareInfo *ExtraArgs)
{
    RedBlackTree *tree = (RedBlackTree *)ExtraArgs->ContainerLeft;
    size_t len = tree->KeySize;
    return memcmp(arg1,arg2,len);
}

static ErrorFunction SetErrorFunction(RedBlackTree *RBT,ErrorFunction fn)
{
    ErrorFunction old;
    if (RBT == NULL) return iError.RaiseError;
    old = RBT->RaiseError;
    RBT->RaiseError = (fn) ? fn : iError.EmptyErrorFunction;
    return old;
}

static unsigned SetFlags(RedBlackTree *l,unsigned newval)
{
    int result = l->Flags;
    l->Flags = newval;
    return result;
}

static unsigned GetFlags(RedBlackTree *l)
{
    return l->Flags;
}

static size_t GetCount(RedBlackTree *l)
{
    return l->count;
}


static RedBlackTreeNode *get_node(RedBlackTree *Tree)
{
    RedBlackTreeNode *tmp;
    if (Tree->FreeList != NULL ) {
        tmp = Tree->FreeList;
        Tree->FreeList = Tree->FreeList -> left;
    }
    else {
      if( Tree->CurrentBlock == NULL || Tree->Available == 0) {
          Tree->CurrentBlock = Tree->Allocator->malloc(BLOCKSIZE * sizeof(RedBlackTreeNode) );
        Tree->Available = BLOCKSIZE;
     }
     tmp = Tree->CurrentBlock++;
     Tree->Available -= 1;
  }
    memset(tmp,0,sizeof(RedBlackTreeNode));
  return tmp;
}


static void return_node(RedBlackTree *Tree,RedBlackTreeNode *node)
{  node->left = Tree->FreeList;
   Tree->FreeList = node;
}

static void left_rotation(RedBlackTree *Tree,RedBlackTreeNode *n)
{
    RedBlackTreeNode *tmp_node = n->left;
    void *tmp_key  = n->Key;
    n->left  = n->right;
    memcpy(n->Key,n->right->Key,Tree->KeySize);
    n->right = n->left->right;
    n->left->right = n->left->left;
    n->left->left  = tmp_node;
    memcpy(n->left->Key, tmp_key, Tree->KeySize);
}

static void right_rotation(RedBlackTree *Tree, RedBlackTreeNode *n)
{
    RedBlackTreeNode *tmp_node = n->right;
    void *tmp_key = n->Key;
    n->right = n->left;
    memcpy(n->Key, n->left->Key,Tree->KeySize);
    n->left  = n->right->left;
    n->right->left = n->right->right;
    n->right->right  = tmp_node;
    memcpy(n->right->Key, tmp_key,Tree->KeySize);
}

static void *Find(RedBlackTree *Tree, void *query_key, void *ExtraArgs)
{
    RedBlackTreeNode *node = Tree->root,*tmp_node;
    CompareInfo ci;
    int cmpval;

    if (node->left == NULL)
        return NULL;
    tmp_node = node;
    ci.ContainerLeft = Tree;
    ci.ContainerRight = NULL;
    ci.ExtraArgs = ExtraArgs;
    while (tmp_node->right != NULL ) {
        cmpval = Tree->KeyCompareFn(tmp_node->Key,query_key,&ci);
        if (cmpval < 0 )
            tmp_node = tmp_node->left;
        else
            tmp_node = tmp_node->right;
    }
    cmpval = Tree->KeyCompareFn(tmp_node->Key,query_key,&ci);
    if( cmpval == 0 )
        return  tmp_node->left;
    else
        return NULL;
}

static void *CopyObject(RedBlackTree *Tree,const void *newObject)
{
    void *tmp = Tree->Allocator->malloc(Tree->ElementSize);
    if (tmp == NULL) {
        Tree->RaiseError("Red/Black tree: Add",CONTAINER_ERROR_NOMEMORY);
        return NULL;
    }
    memcpy(tmp,newObject,Tree->ElementSize);
    return tmp;
}

#define black 0
#define red 1
#define NODE_HAS_DATA    1
static int Insert(RedBlackTree *Tree,const void *new_key, const void *new_object,void *ExtraArgs)
{
    RedBlackTreeNode *old_leaf, *new_leaf,*treenode = Tree->root;
    int cmpval;
    CompareInfo ci;

    ci.ContainerLeft = Tree;
    ci.ContainerRight = NULL;
    ci.ExtraArgs = ExtraArgs;
    if( treenode->left == NULL ) {
        void *tmp = CopyObject(Tree,new_object);
        if (tmp == NULL) {
            return -1;
        }
        treenode->left = (RedBlackTreeNode *) tmp;
        memcpy(treenode->Key, new_key,Tree->KeySize);
        treenode->color = black; /* root is always black */
        treenode->right  = NULL;
        treenode->flags = NODE_HAS_DATA;
   }
   else {
       RedBlackTreeNode *current=treenode, *next_node, *upper=NULL;
       while( current->right != NULL ) {
           cmpval = Tree->KeyCompareFn(new_key,current->Key,&ci);
            if (cmpval < 0)
                next_node = current->left;
            else
                next_node = current->right;
           if (current->color == black ) {
               if (current->left->color == black ||
                   current->right->color == black ) {
                        upper = current; current = next_node;
               }
               else {/* current->left and current->right red */
                   /* need rebalance */
                   if (upper == NULL ) { /* current is root */
                       current->left->color = black;
                       current->right->color = black;
                       /* upper = current; this value is never used jacob */
                   }
                   else if (Tree->KeyCompareFn(current->Key , upper->Key,&ci) < 0 ) {/* current left of upper */
                       if (current == upper->left ) {
                           current->left->color = black;
                           current->right->color = black;
                           current->color = red;
                       }
                       else if( current == upper->left->left ) {
                           right_rotation(Tree, upper );
                           upper->left->color = red;
                           upper->right->color = red;
                           upper->left->left->color = black;
                           upper->left->right->color = black;
                       }
                       else { /* current == upper->left->right */
                           left_rotation(Tree, upper->left );
                           right_rotation(Tree, upper );
                           upper->left->color = red;
                           upper->right->color = red;
                           upper->right->left->color = black;
                           upper->left->right->color = black;
                       }
                   }
                   else { /* current->key >= upper->key */  /* current right of upper */
                       if (current == upper->right ) {
                           current->left->color = black;
                           current->right->color = black;
                           current->color = red;
                       }
                       else if( current == upper->right->right ) {
                           left_rotation(Tree, upper );
                           upper->left->color = red;
                           upper->right->color = red;
                           upper->right->left->color = black;
                           upper->right->right->color = black;
                       }
                       else { /* current == upper->right->left */
                           right_rotation(Tree, upper->right );
                           left_rotation(Tree, upper );
                           upper->left->color = red;
                           upper->right->color = red;
                           upper->right->left->color = black;
                           upper->left->right->color = black;
                       }
                   } /* end rebalancing */
                   current = next_node;
                   upper = current; /*two black lower neighbors*/
               }
           }
           else { /* current red */
               current = next_node; /*move down */
           }
       } /* end while; reached leaf. always arrive on black leaf*/
       /* found the candidate leaf. Test whether key distinct */
       if(Tree->KeyCompareFn(current->Key,new_key,&ci) == 0) return( -1 );
       /* key is distinct, now perform the insert */
         old_leaf = get_node(Tree);
         old_leaf->left = current->left;
         memcpy(old_leaf->Key , current->Key, Tree->KeySize);
         old_leaf->right  = NULL;
         old_leaf->color = red;
         new_leaf = get_node(Tree);
       new_leaf->left = CopyObject(Tree,new_object);
       new_leaf->flags = NODE_HAS_DATA;
         memcpy(new_leaf->Key, new_key, Tree->KeySize);
         new_leaf->right  = NULL;
         new_leaf->color = red;
         if (Tree->KeyCompareFn(current->Key , new_key, &ci) < 0 ) {   current->left  = old_leaf;
             current->right = new_leaf;
             memcpy(current->Key, new_key, Tree->KeySize);
         }
         else { current->left  = new_leaf; current->right = old_leaf; }
   }
   return 1;
}

static int Add(RedBlackTree *Tree, const void *Key,const void *Data)
{
    return Insert(Tree,Key,Data,NULL);
}

static int Remove(RedBlackTree *Tree, const void *delete_key, void *ExtraArgs)
{
    void *deleted_object;
    CompareInfo ci;
    int cmpval;
    RedBlackTreeNode *treenode = Tree->root;

    ci.ContainerLeft = Tree;
    ci.ContainerRight = NULL;
    ci.ExtraArgs = ExtraArgs;
    if (treenode->left == NULL )
        return 0;
    if (treenode->right == NULL) {
       if (0 == Tree->KeyCompareFn(treenode->Key ,delete_key,&ci) ) {
           deleted_object = (void *) treenode->left;
           treenode->left = NULL;
           treenode->flags &= ~NODE_HAS_DATA;
           if (Tree->DestructorFn)
               Tree->DestructorFn(deleted_object);
           Tree->Allocator->free(deleted_object);
           return 1;
      }
      else return 0;
   }
   else {
       RedBlackTreeNode *current, *upper;
      treenode->color = black; /* root is always black*/
      upper = treenode;
      if (upper->left->color == black &&  upper->right->color == black ) {
          /* need to give upper ared lower neighbor */
          cmpval = Tree->KeyCompareFn(delete_key,upper->Key,&ci);
          if( cmpval < 0 ) {
              if( upper->left->right == NULL )  {
                  if( upper->right->right == NULL ) {
                      upper->left->color  = red;
                      upper->right->color = red;
                  }
                  else {
                      upper->right->left->color  = black;
                      upper->right->right->color = black;
                      upper->right->color = red;
                  }
              }
              else {
                  if (upper->left->left->color == red ||
                     upper->left->right->color == red )
                      upper = upper->left;
                  else if (upper->right->right->color == red) {
                      left_rotation(Tree, upper);
                      upper->right->color = black;
                      upper->left->color  = black;
                      upper->left->left->color = red;
                      upper = upper->left;
                  }
                  else if (upper->right->left->color == red) {
                      right_rotation(Tree, upper->right );
                      left_rotation(Tree, upper );
                      upper->right->color = black;
                      upper->left->color  = black;
                      upper->left->left->color = red;
                      upper = upper->left;
                  }
                  else { /* left and right have only black lower neighbors */
                      upper->left->color = red;
                      upper->right->color = red;
                  }
              }
          } /* cmpval < 0 */
          else { /* delete_key >= upper->key */
              if (upper->right->left == NULL) {
                  if (upper->left->right == NULL) {
                      upper->left->color  = red;
                      upper->right->color = red;
                  }
                  else {
                      upper->left->left->color  = black;
                      upper->left->right->color = black;
                      upper->left->color = red;
                  }
              }
              else {
                  if (upper->right->right->color == red ||
                      upper->right->left->color == red )
                      upper = upper->right;
                  else if (upper->left->left->color == red) {
                      right_rotation(Tree, upper);
                      upper->right->color = black;
                      upper->left->color  = black;
                      upper->right->right->color = red;
                      upper = upper->right;
                  }
                  else if (upper->left->right->color == red) {
                      left_rotation(Tree, upper->left );
                      right_rotation(Tree, upper );
                      upper->right->color = black;
                      upper->left->color  = black;
                      upper->right->right->color = red;
                      upper = upper->right;
                  }
                  else { /* left and right have only black lower neighbors */
                      upper->left->color = red;
                      upper->right->color = red;
                  }
              }
          } /* delete key >= upper_key */
      } /* upper has at least one red lower neighbor */
      current = upper;
      while( current->right != NULL ) {
          if (Tree->KeyCompareFn(delete_key,current->Key,&ci) < 0 )
              current = current->left;
          else current = current->right;
          if (current->color == red || current->right == NULL )
              continue; /* go on down, or stop if leaf found */
          else { /* current->color == black, not leaf, rebalance */
              if (current->left->color  == red ||
                  current->right->color == red ) {
                  upper = current; /* condition satisfied */
              }
              else { /* current->left and current->right black */
                  /* need rebalance */
                  if (Tree->KeyCompareFn(current->Key ,upper->Key,&ci) < 0 ) {
                      /* Case 2: current left of upper */
                      if( current == upper->left ) {
                          if( upper->right->left->left->color == black &&
                             upper->right->left->right->color == black ) {
                              /* Case 2.1.    1 */
                              left_rotation(Tree, upper);
                              upper->left->color = black;
                              upper->left->left->color = red;
                              upper->left->right->color = red;
                              current = upper = upper->left;
                          }
                          else if( upper->right->left->left->color == red ) {
                              /* Case 2.1.2 */
                              right_rotation(Tree, upper->right->left );
                              right_rotation(Tree, upper->right );
                              left_rotation(Tree, upper );
                              upper->left->color = black;
                              upper->right->left->color = black;
                              upper->right->color = red;
                              upper->left->left->color = red;
                              current = upper = upper->left;
                          }
                          else { /* upper->right->left->left->color == black &&
                                  upper->right->left->right->color == red */
                              /* Case 2.1.3 */
                              right_rotation(Tree, upper->right );
                              left_rotation(Tree, upper );
                              upper->left->color = black;
                              upper->right->left->color = black;
                              upper->right->color = red;
                              upper->left->left->color = red;
                              current = upper = upper->left;
                          }
                      } /*end Case 2.1: current==upper->left */
                      else if( current == upper->left->left ) {
                          if( upper->left->right->left->color == black &&
                             upper->left->right->right->color == black ) {
                              /* Case 2.2.1 */
                              upper->left->color = black;
                              upper->left->left->color = red;
                              upper->left->right->color = red;
                              current = upper = upper->left;
                          }
                          else if( upper->left->right->right->color == red ) {
                              /* Case 2.2.2 */
                              left_rotation(Tree, upper->left );
                              upper->left->left->color = black;
                              upper->left->right->color = black;
                              upper->left->color = red;
                              upper->left->left->left->color = red;
                              current = upper = upper->left->left;
                          }
                          else { /* upper->left->right->left->color == red &&
                                upper->left->right->right->color == black */
                              /* Case 2.2.3 */
                              right_rotation(Tree, upper->left->right );
                              left_rotation(Tree, upper->left );
                              upper->left->left->color = black;
                              upper->left->right->color = black;
                              upper->left->color = red;
                              upper->left->left->left->color = red;
                              current = upper = upper->left->left;
                          }
                      } /*end Case 2.2: current==upper->left->left */
                      else { /* current == upper->left->right */
                          if( upper->left->left->left->color == black &&
                             upper->left->left->right->color == black) {
                              /* Case 2.3.1 */
                              upper->left->color = black;
                              upper->left->left->color = red;
                              upper->left->right->color = red;
                              current = upper = upper->left;
                          }
                          else if( upper->left->left->left->color == red ) {
                              /* Case 2.3.2 */
                              right_rotation(Tree, upper->left );
                              upper->left->left->color = black;
                              upper->left->right->color = black;
                              upper->left->color = red;
                              upper->left->right->right->color = red;
                              current = upper = upper->left->right;
                          }
                          else { /* upper->left->left->left->color == black &&
                                upper->left->left->right->color == red */
                              /* Case 2.3.3 */
                              left_rotation(Tree, upper->left->left);
                              right_rotation(Tree, upper->left);
                              upper->left->left->color = black;
                              upper->left->right->color = black;
                              upper->left->color = red;
                              upper->left->right->right->color = red;
                              current = upper = upper->left->right;
                          }
                      } /*end Case 2.3: current==upper->left->right */
                  } /* end Case 2: current->key < upper-> key */
                  else { /* Case 3: current->key >= upper->key */
                      if( current == upper->right ) {
                          if( upper->left->right->right->color == black &&
                             upper->left->right->left->color == black ) {
                              /* Case 3.1.1 */
                              right_rotation(Tree, upper );
                              upper->right->color = black;
                              upper->right->right->color = red;
                              upper->right->left->color = red;
                              current = upper = upper->right;
                          }
                          else if( upper->left->right->right->color == red ) {
                              /* Case 3.1.2 */
                              left_rotation(Tree, upper->left->right );
                              left_rotation(Tree, upper->left );
                              right_rotation(Tree, upper );
                              upper->right->color = black;
                              upper->left->right->color = black;
                              upper->left->color = red;
                              upper->right->right->color = red;
                              current = upper = upper->right;
                          }
                          else { /* upper->left->right->right->color == black &&
                                upper->left->right->left->color == red */
                              /* Case 3.1.3 */
                              left_rotation(Tree, upper->left );
                              right_rotation(Tree, upper );
                              upper->right->color = black;
                              upper->left->right->color = black;
                              upper->left->color = red;
                              upper->right->right->color = red;
                              current = upper = upper->right;
                          }
                      } /*end Case 3.1: current==upper->left */
                      else if( current == upper->right->right ) {
                          if( upper->right->left->right->color == black &&
                             upper->right->left->left->color == black ) {
                              /* Case 3.2.1 */
                              upper->right->color = black;
                              upper->right->left->color = red;
                              upper->right->right->color = red;
                              current = upper = upper->right;
                          }
                          else if( upper->right->left->left->color == red ) {
                              /* Case 3.2.2 */
                              right_rotation(Tree, upper->right );
                              upper->right->left->color = black;
                              upper->right->right->color = black;
                              upper->right->color = red;
                              upper->right->right->right->color = red;
                              current = upper = upper->right->right;
                          }
                          else { /* upper->right->left->right->color == red &&
                                upper->right->left->left->color == black */
                              /* Case 3.2.3 */
                              left_rotation(Tree, upper->right->left );
                              right_rotation(Tree, upper->right );
                              upper->right->right->color = black;
                              upper->right->left->color = black;
                              upper->right->color = red;
                              upper->right->right->right->color = red;
                              current = upper = upper->right->right;
                          }
                      } /*end Case 3.2: current==upper->right->right */
                      else { /* current == upper->right->left */
                          if( upper->right->right->right->color == black &&
                             upper->right->right->left->color == black ) {
                              /* Case 3.3.1 */
                              upper->right->color = black;
                              upper->right->left->color = red;
                              upper->right->right->color = red;
                              current = upper = upper->right;
                          }
                          else if (upper->right->right->right->color == red ) {
                              /* Case 3.3.2 */
                              left_rotation(Tree, upper->right );
                              upper->right->left->color = black;
                              upper->right->right->color = black;
                              upper->right->color = red;
                              upper->right->left->left->color = red;
                              current = upper = upper->right->left;
                          }
                          else { /* upper->right->right->right->color == black &&
                                upper->right->right->left->color == red */
                              /* Case 3.3.3 */
                              right_rotation(Tree, upper->right->right );
                              left_rotation(Tree, upper->right );
                              upper->right->left->color = black;
                              upper->right->right->color = black;
                              upper->right->color = red;
                              upper->right->left->left->color = red;
                              current = upper = upper->right->left;
                          }
                      } /*end Case 3.3: current==upper->right->left */
                  } /* end Case 3: current->key >= upper-> key */
              } /* end rebalance, upper has a red lower neighbor */
          }
      }  /* end while */
      /* found the candidate leaf. Test whether key correct */
       if(Tree->KeyCompareFn(current->Key,delete_key,&ci) != 0)
           return 0;
       else { /* want to delete node current */
           RedBlackTreeNode *tmp_node;
           deleted_object = (void *) current->left;
           if (Tree->DestructorFn)
               Tree->DestructorFn(deleted_object);
           Tree->Allocator->free(deleted_object);
           current->flags &= ~NODE_HAS_DATA;
           if (Tree->KeyCompareFn(current->Key , upper->Key,&ci ) < 0) {
               if( current == upper->left ) {
                   /* upper->right is red */
                   tmp_node = upper->right;
                   memcpy(upper->Key, tmp_node->Key,Tree->KeySize);
                   upper->left  = tmp_node->left;
                   upper->right = tmp_node->right;
               }
               else if (current == upper->left->left ) {
                   /* upper->left is red */
                   tmp_node = upper->left;
                   upper->left  = tmp_node->right;
               }
               else { /* current == upper->left->right */
                   /* upper->left is red */
                   tmp_node = upper->left;
                   upper->left  = tmp_node->left;
               }
           } /* end current->key < upper->key */
           else { /* leaf to the right, current->key >= upper->key */
               if( current == upper->right ) {/* upper->left is red */
                   tmp_node = upper->left;
                   memcpy(upper->Key, tmp_node->Key,Tree->KeySize);
                   upper->left  = tmp_node->left;
                   upper->right = tmp_node->right;
               }
               else if ( current == upper->right->right ) {
                   /* upper->right is red */
                   tmp_node = upper->right;
                   upper->right  = tmp_node->left;
               }
               else { /* current == upper->right->left */
                   /* upper->right is red */
                   tmp_node = upper->right;
                   upper->right  = tmp_node->right;
               }
           } /* end current->key >= upper->key */
           return_node(Tree, tmp_node );
           return_node(Tree, current );
       }
       return 1;
   }
}

static Iterator *NewIterator(RedBlackTree *p) { return NULL;}
static int deleteIterator(Iterator * it) {return 1;}

static int Apply(RedBlackTree *ST,int (*Applyfn)(const void *data,void *arg),void *arg)
{ return 0; }
static int Finalize(RedBlackTree *t) { return 0; }
static int Clear(RedBlackTree *t) { return 0; }
static size_t Sizeof(RedBlackTree *p)
{
    size_t result;
    if (p == NULL)
        return sizeof(RedBlackTree);
    result = sizeof(*p);
    result += p->count * p->ElementSize;
    result += p->count * sizeof(RedBlackTreeNode);
    result += p->count * p->KeySize;
    return result;
}
static CompareFunction SetCompareFunction(RedBlackTree *l,CompareFunction fn)
{
    CompareFunction oldfn = l->KeyCompareFn;

    if (fn != NULL) /* Treat NULL as an enquiry to get the compare function */
        l->KeyCompareFn = fn;
    return oldfn;
}

static size_t GetElementSize(RedBlackTree *d) { return d->ElementSize;}
static DestructorFunction SetDestructor(RedBlackTree *cb,DestructorFunction fn)
{
    DestructorFunction oldfn;
    if (cb == NULL)
        return NULL;
    oldfn = cb->DestructorFn;
    if (fn)
        cb->DestructorFn = fn;
    return oldfn;
}

RedBlackTreeInterface iRedBlackTree = {
    Create,
    GetElementSize,
    GetCount,
    GetFlags,
    SetFlags,
    Add,
    Insert,
    Clear,
    Remove,
    Finalize,
    Apply,
    Find,
    SetErrorFunction,
    SetCompareFunction,
    Sizeof,
    DefaultCompareFunction,
    NewIterator,
    deleteIterator,
    SetDestructor,
};



#if 0
void check_tree( RedBlackTreeNode *tr, int count, int lower, int upper )
{  if( tr->left == NULL )
   {  printf("Tree Empty\n"); return; }
   if( tr->key < lower || tr->key >= upper )
   {      printf("Wrong Key Order: node key %d, lower %d, upper %d \n",
               tr->key, lower, upper);
   }
   if( tr->right == NULL ) /* leaf */
   {  if( *( (int *) tr->left) == 10*tr->key + 2 )
         printf("%d(%d)  ", tr->key, count+(tr->color==black) );
      else
         printf("Wrong Object \n");
   }
   else /* not leaf */
   {  if( tr->color == red )
      {  if(  tr->left->color == red )
       printf("wrong color below red key %d on the left\n", tr->key );
         if(  tr->right->color == red )
       printf("wrong color below red key %d on the right\n", tr->key );
      }
      check_tree(tr->left, count + (tr->color==black), lower, tr->key );
      check_tree(tr->right, count+ (tr->color==black), tr->key, upper );
   }
}
#endif
#if 0
int main()
{  RedBlackTreeNode *searchtree;
   char nextop;
   searchtree = create_tree();
   printf("Made Tree: Red-Black Tree with Top-Down Rebalancing\n");
   while( (nextop = getchar())!= 'q' )
   { if( nextop == 'i' )
     { int inskey,  *insobj, success;
       insobj = (int *) malloc(sizeof(int));
       scanf(" %d", &inskey);
       *insobj = 10*inskey+2;
       success = insert( searchtree, inskey, insobj );
       if ( success == 0 )
         printf("  insert successful, key = %d, object value = %d,\n",
              inskey, *insobj );
       else
           printf("  insert failed, success = %d\n", success);
     }
     if( nextop == 'f' )
     { int findkey, *findobj;
       scanf(" %d", &findkey);
       findobj = find( searchtree, findkey);
       if( findobj == NULL )
         printf("  find failed, for key %d\n", findkey);
       else
         printf("  find successful, found object %d\n", *findobj);
     }
     if( nextop == 'd' )
     { int delkey, *delobj;
       scanf(" %d", &delkey);
       delobj = delete( searchtree, delkey);
       if( delobj == NULL )
         printf("  delete failed for key %d\n", delkey);
       else
         printf("  delete successful, deleted object %d\n",
             *delobj);
     }
     if( nextop == '?' )
     {  printf("  Checking tree\n");
        check_tree(searchtree,0,-100000,100000);
        printf("\n");
        if( searchtree->left != NULL )
      printf("key in root is %d,\n", searchtree->key);
        printf("  Finished Checking tree\n");
     }
   }
   return(0);
}


#endif
