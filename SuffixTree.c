/******************************************************************************
Suffix Tree Version 2.1

AUTHORS

Dotan Tsadok
Instructor: Mr. Shlomo Yona, University of Haifa, Israel. December 2002.
Current maintainer: Shlomo Yona	<shlomo@cs.haifa.ac.il>

COPYRIGHT

Copyright 2002-2003 Shlomo Yona

LICENSE

This library is free software; you can redistribute it and/or modify it
under the same terms as Perl itself.


DESCRIPTION OF THIS FILE:

This is the implementation file suffix_tree.c implementing the header file
suffix_tree.h.

This code is an Open Source implementation of Ukkonen's algorithm for
constructing a suffix tree over a string in time and space complexity
O(length of the string). The code is written under strict ANSI C.

For a complete understanding of the code see Ukkonen's algorithm and the
readme.txt file.

The general pseudo code is:

n = length of the string.
ST_CreateTree:
   Calls n times to SPA (Single Phase Algorithm). SPA:  
      Increase the variable e (virtual end of all leaves).
   Calls SEA (Single Extension Algorithm) starting with the first extension that
   does not already exist in the tree and ending at the first extension that
   already exists. SEA :  
      Follow suffix link.
      Check if current suffix exists in the tree.
      If it does not - apply rule 2 and then create a new suffix link.
      apply_rule_2:  
         Create a new leaf and maybe a new internal node as well.
         create_node:  
            Create a new node or a leaf.


For implementation interpretations see Basic Ideas paragraph in the Developement
section of the readme.txt file.

An example of the implementations of a node and its sons using linked lists
instead of arrays:

   (1)
    |
    |
    |
   (2)--(3)--(4)--(5)

(2) is the only son of (1) (call it the first son). Other sons of (1) are
connected using a linked lists starting from (2) and going to the right. (3) is
the right sibling of (2) (and (2) is the left sibling of (3)), (4) is the right
sibling of (3), etc.
The father field of all (2), (3), (4) and (5) points to (1), but the son field
of (1) points only to (2).

*******************************************************************************/

#include "containers.h"

/******************************************************************************/
/*                   INTERNAL DATA STRUCTURES                                 */
/******************************************************************************/
/* This structure describes a node and its incoming edge */
typedef struct SuffixTreeNODE
{
   /* A linked list of sons of that node */
   struct SuffixTreeNODE*   sons;
   /* A linked list of right siblings of that node */
   struct SuffixTreeNODE*   right_sibling;
   /* A linked list of left siblings of that node */
   struct SuffixTreeNODE*   left_sibling;
   /* A pointer to that node's father */
   struct SuffixTreeNODE*   father;
   /* A pointer to the node that represents the largest 
   suffix of the current node */
   struct SuffixTreeNODE*   suffix_link;
   /* Index of the start position of the node's path */
   int32_t                 path_position;
   /* Start index of the incoming edge */
   int32_t                 edge_label_start;
   /* End index of the incoming edge */
   int32_t                 edge_label_end;
} NODE;

/* This structure describes a suffix tree */
struct tagSuffixTree {
   SuffixTreeInterface *VTable;
   size_t count;
   unsigned int Flags;
   /* A linked list of sons of that node */
   /* The virtual end of all leaves */
   int32_t                 e;  
   /* The one and only real source string of the tree. All edge-labels
      contain only indices to this string and do not contain the characters
      themselves */
   char*           tree_string;
   /* The length of the source string */
   int32_t                 length;
   /* The node that is the head of all others. It has no siblings nor a
      father */
   NODE*                    root;
   FILE *outstream;    // For printing
   ContainerAllocator *allocator;
   ErrorFunction RaiseError;
   unsigned counter;
   unsigned heap;	/* Used for statistic measures of space. */
};

static void ST_PrintTree(SuffixTree* tree,FILE *outstream);

/* Used in function trace_string for skipping (Ukkonen's Skip Trick). */
typedef enum SKIP_TYPE     {skip, no_skip}                 SKIP_TYPE;
/* Used in function apply_rule_2 - two types of rule 2 - see function for more
   details.*/
typedef enum RULE_2_TYPE   {new_son, split}                RULE_2_TYPE;
/* Signals whether last matching position is the last one of the current edge */
typedef enum LAST_POS_TYPE {last_char_in_edge, other_char} LAST_POS_TYPE;

/* Used to mark the node that has no suffix link yet. By Ukkonen, it will have
   one by the end of the current phase. */
static NODE*    suffixless;

typedef struct SuffixTreePATH {
   int32_t   begin;
   int32_t   end;
} PATH;

typedef struct SuffixTreePOS {
   NODE*      node;
   int32_t   edge_pos;
}POS;

/******************************************************************************/
/*
   Define DEBUG in order to view debug printouts to the screen while
   constructing and searching the suffix tree.
*/
/* #define DEBUG */

/******************************************************************************/
/*
   create_node :
   Creates a node with the given init field-values.

  Input : The father of the node, the starting and ending indices 
  of the incloming edge to that node, 
        the path starting position of the node.

  Output: A pointer to that node.
*/

static NODE* create_node(NODE* father, int32_t start, int32_t end, int32_t position,SuffixTree *tree)
{
    ContainerAllocator *allocator;
    if (tree == NULL) {
        iError.RaiseError("SuffixTree.CreateNode",CONTAINER_ERROR_BADARG);
        return NULL;
    }
    allocator = tree->allocator;
   /*Allocate a node.*/
   NODE* node   = allocator->malloc(sizeof(NODE));
   if(node == 0) {
       iError.RaiseError("SuffixTree.CreateNode",CONTAINER_ERROR_NOMEMORY);
       return NULL;
   }

   tree->heap+=sizeof(NODE);

   /* Initialize node fields. For detailed description of the fields see
      suffix_tree.h */
   node->sons             = 0;
   node->right_sibling    = 0;
   node->left_sibling     = 0;
   node->suffix_link      = 0;
   node->father           = father;
   node->path_position    = position;
   node->edge_label_start = start;
   node->edge_label_end   = end;
   return node;
}

/******************************************************************************/
/*
   find_son :
   Finds son of node that starts with a certain character. 

   Input : the tree, the node to start searching from and the character to be
           searched in the sons.
  
   Output: A pointer to the found son, 0 if no such son.
*/

static NODE* find_son(SuffixTree* tree, NODE* node, char character)
{
   /* Point to the first son. */
   node = node->sons;
   /* scan all sons (all right siblings of the first son) for their first
   character (it has to match the character given as input to this function. */
   while(node != 0 && tree->tree_string[node->edge_label_start] != character)
   {
      node = node->right_sibling;
   }
   return node;
}


/******************************************************************************/
/*
   get_node_label_end :
   Returns the end index of the incoming edge to that node. This function is
   needed because for leaves the end index is not relevant, instead we must look
   at the variable "e" (the global virtual end of all leaves). Never refer
   directly to a leaf's end-index.

   Input : the tree, the node its end index we need.

   Output: The end index of that node (meaning the end index of the node's
   incoming edge).
*/

static int32_t get_node_label_end(SuffixTree* tree, NODE* node)
{
   /* If it's a leaf - return e */
   if(node->sons == 0)
      return tree->e;
   /* If it's not a leaf - return its real end */
   return node->edge_label_end;
}

/******************************************************************************/
/*
   get_node_label_length :
   Returns the length of the incoming edge to that node. Uses get_node_label_end
   (see above).

   Input : The tree and the node its length we need.

   Output: the length of that node.
*/

static int32_t get_node_label_length(SuffixTree* tree, NODE* node)
{
   /* Calculate and return the lentgh of the node */
   return get_node_label_end(tree, node) - node->edge_label_start + 1;
}

/******************************************************************************/
/*
   is_last_char_in_edge :
   Returns 1 if edge_pos is the last position in node's incoming edge.

   Input : The tree, the node to be checked and the position in its incoming
           edge.

   Output: the length of that node.
*/

static int is_last_char_in_edge(SuffixTree* tree, NODE* node, int32_t edge_pos)
{
   if(edge_pos == get_node_label_length(tree,node)-1)
      return 1;
   return 0;
}

/******************************************************************************/
/*
   connect_siblings :
   Connect right_sib as the right sibling of left_sib and vice versa.

   Input : The two nodes to be connected.

   Output: None.
*/

static void connect_siblings(NODE* left_sib, NODE* right_sib)
{
   /* Connect the right node as the right sibling of the left node */
   if(left_sib != 0)
      left_sib->right_sibling = right_sib;
   /* Connect the left node as the left sibling of the right node */
   if(right_sib != 0)
      right_sib->left_sibling = left_sib;
}

/******************************************************************************/
/*
   apply_extension_rule_2 :
   Apply "extension rule 2" in 2 cases:
   1. A new son (leaf 4) is added to a node that already has sons:
                (1)	       (1)
               /   \	 ->   / | \
              (2)  (3)      (2)(3)(4)

   2. An edge is split and a new leaf (2) and an internal node (3) are added:
              | 	  |
              | 	 (3)
              |     ->   / \
             (1)       (1) (2)

   Input : See parameters.

   Output: A pointer to the newly created leaf (new_son case) or internal node
   (split case).
*/

static NODE* apply_extension_rule_2(
                      /* Node 1 (see drawings) */
                      NODE*           node,            
                      /* Start index of node 2's incoming edge */
                      int32_t        edge_label_begin,   
                      /* End index of node 2's incoming edge */
                      int32_t        edge_label_end,      
                      /* Path start index of node 2 */
                      int32_t        path_pos,         
                      /* Position in node 1's incoming edge where split is to be
		         performed */
                      int32_t        edge_pos,         
                      /* Can be 'new_son' or 'split' */
                      RULE_2_TYPE     type,SuffixTree *tree)
{
   NODE *new_leaf,
        *new_internal,
        *son;
   /*-------new_son-------*/
   if(type == new_son)                                       
   {
#ifdef DEBUG   
      printf("rule 2: new leaf (%lu,%lu)\n",edge_label_begin,edge_label_end);
#endif
      /* Create a new leaf (4) with the characters of the extension */
      new_leaf = create_node(node, edge_label_begin , edge_label_end, path_pos,tree);
      /* Connect new_leaf (4) as the new son of node (1) */
      son = node->sons;
      while(son->right_sibling != 0)
         son = son->right_sibling;
      connect_siblings(son, new_leaf);
      /* return (4) */
      return new_leaf;
   }
   /*-------split-------*/
#ifdef DEBUG   
   printf("rule 2: split (%lu,%lu)\n",edge_label_begin,edge_label_end);
#endif
   /* Create a new internal node (3) at the split point */
   new_internal = create_node(
                      node->father,
                      node->edge_label_start,
                      node->edge_label_start+edge_pos,
                      node->path_position,tree);
   /* Update the node (1) incoming edge starting index (it now starts where node
   (3) incoming edge ends) */
   node->edge_label_start += edge_pos+1;

   /* Create a new leaf (2) with the characters of the extension */
   new_leaf = create_node(
                      new_internal,
                      edge_label_begin,
                      edge_label_end,
                      path_pos,tree);
   
   /* Connect new_internal (3) where node (1) was */
   /* Connect (3) with (1)'s left sibling */
   connect_siblings(node->left_sibling, new_internal);   
   /* connect (3) with (1)'s right sibling */
   connect_siblings(new_internal, node->right_sibling);
   node->left_sibling = 0;

   /* Connect (3) with (1)'s father */
   if(new_internal->father->sons == node)            
      new_internal->father->sons = new_internal;
   
   /* Connect new_leaf (2) and node (1) as sons of new_internal (3) */
   new_internal->sons = node;
   node->father = new_internal;
   connect_siblings(node, new_leaf);
   /* return (3) */
   return new_internal;
}

/******************************************************************************/
/*
   trace_single_edge :
   Traces for a string in a given node's OUTcoming edge. It searches only in the
   given edge and not other ones. Search stops when either whole string was
   found in the given edge, a part of the string was found but the edge ended
   (and the next edge must be searched too - performed by function trace_string)
   or one non-matching character was found.

   Input : The string to be searched, given in indices of the main string.

   Output: (by value) the node where tracing has stopped.
           (by reference) the edge position where last match occured, the string
	   position where last match occured, number of characters found, a flag
	   for signaling whether search is done, and a flag to signal whether
	   search stopped at a last character of an edge.
*/

static NODE* trace_single_edge(SuffixTree*    tree,
                      /* Node to start from */
                      NODE*           node,         
                      /* String to trace */
                      PATH            str,         
                      /* Last matching position in edge */
                      int32_t*       edge_pos,      
                      /* Last matching position in tree source string */
                      int32_t*       chars_found,   
                      /* Skip or no_skip*/
                      SKIP_TYPE       type,          
                      /* 1 if search is done, 0 if not */
                      int*            search_done)   
{
   NODE*      cont_node;
   int32_t   length,str_len;

   /* Set default return values */
   *search_done = 1;
   *edge_pos    = 0;

   /* Search for the first character of the string in the outcoming edge of
      node */
   cont_node = find_son(tree, node, tree->tree_string[str.begin]);
   if(cont_node == 0)
   {
      /* Search is done, string not found */
      *edge_pos = get_node_label_length(tree,node)-1;
      *chars_found = 0;
      return node;
   }
   
   /* Found first character - prepare for continuing the search */
   node    = cont_node;
   length  = get_node_label_length(tree,node);
   str_len = str.end - str.begin + 1;

   /* Compare edge length and string length. */
   /* If edge is shorter then the string being searched and skipping is
      enabled - skip edge */
   if(type == skip) {
      if(length <= str_len) {
         (*chars_found)   = length;
         (*edge_pos)      = length-1;
         if(length < str_len)
            *search_done  = 0;
      }
      else {
         (*chars_found)   = str_len;
         (*edge_pos)      = str_len-1;
      }
      return node;
   }
   else {
      /* Find minimum out of edge length and string length, and scan it */
      if(str_len < length)
         length = str_len;

      for(*edge_pos=1, *chars_found=1; *edge_pos<length; (*chars_found)++,(*edge_pos)++) {
         /* Compare current characters of the string and the edge. If equal - continue */
         if(tree->tree_string[node->edge_label_start+*edge_pos] != tree->tree_string[str.begin+*edge_pos]) {
            (*edge_pos)--;
            return node;
         }
      }
   }

   /* The loop has advanced *edge_pos one too much */
   (*edge_pos)--;

   if((*chars_found) < str_len)
      /* Search is not done yet */
      *search_done = 0;

   return node;
}

/******************************************************************************/
/*
   trace_string :
   Traces for a string in the tree. This function is used in construction
   process only, and not for after-construction search of substrings. It is
   tailored to enable skipping (when we know a suffix is in the tree (when
   following a suffix link) we can avoid comparing all symbols of the edge by
   skipping its length immediately and thus save atomic operations - see
   Ukkonen's algorithm, skip trick).
   This function, in contradiction to the function trace_single_edge, 'sees' the
   whole picture, meaning it searches a string in the whole tree and not just in
   a specific edge.

   Input : The string, given in indice of the main string.

   Output: (by value) the node where tracing has stopped.
           (by reference) the edge position where last match occured, the string
	   position where last match occured, number of characters found, a flag
	   for signaling whether search is done.
*/

static NODE* trace_string(
                      SuffixTree*    tree, 
                      /* Node to start from */
                      NODE*           node,         
                      /* String to trace */
                      PATH            str,         
                      /* Last matching position in edge */
                      int32_t*       edge_pos,      
                      /* Last matching position in tree string */
                      int32_t*       chars_found,
                      /* skip or not */
                      SKIP_TYPE       type)         
{
   /* This variable will be 1 when search is done.
      It is a return value from function trace_single_edge */
   int      search_done = 0;

   /* This variable will hold the number of matching characters found in the
      current edge. It is a return value from function trace_single_edge */
   int32_t edge_chars_found;

   *chars_found = 0;

   while(search_done == 0) {
      *edge_pos        = 0;
      edge_chars_found = 0;
      node = trace_single_edge(tree, node, str, edge_pos, &edge_chars_found, type, &search_done);
      str.begin       += edge_chars_found;
      *chars_found    += edge_chars_found;
   }
   return node;
}

/******************************************************************************/
/*
   ST_FindSubstring :
*/

static int32_t ST_FindSubstring(
                      /* The suffix array */
                      SuffixTree*    tree,      
                      /* The substring to find */
                      char*  W,         
                      /* The length of W */
                      int32_t        P)         
{
   /* Starts with the root's son that has the first character of W as its
      incoming edge first character */
   NODE* node   = find_son(tree, tree->root, W[0]);
   int32_t k,j = 0, node_label_end;

   /* Scan nodes down from the root untill a leaf is reached or the substring is
      found */
   while(node!=0) {
      k=node->edge_label_start;
      node_label_end = get_node_label_end(tree,node);
      
      /* Scan a single edge - compare each character with the searched string */
      while(j<P && k<=node_label_end && tree->tree_string[k] == W[j]) {
         j++;
         k++;
      }
      
      /* Checking which of the stopping conditions are true */
      if(j == P) {
         /* W was found - it is a substring. Return its path starting index */
         return node->path_position;
      }
      else if(k > node_label_end)
         /* Current edge is found to match, continue to next edge */
         node = find_son(tree, node, W[j]);
      else {
         /* One non-matching symbols is found - W is not a substring */
         return CONTAINER_ERROR_NOTFOUND;
      }
   }
   return CONTAINER_ERROR_NOTFOUND;
}

/******************************************************************************/
/*
   follow_suffix_link :
   Follows the suffix link of the source node according to Ukkonen's rules. 

   Input : The tree, and pos. pos is a combination of the source node and the 
           position in its incoming edge where suffix ends.
   Output: The destination node that represents the longest suffix of node's 
           path. Example: if node represents the path "abcde" then it returns 
           the node that represents "bcde".
*/

static void follow_suffix_link(SuffixTree* tree, POS* pos)
{
   /* gama is the string between node and its father, in case node doesn't have
      a suffix link */
   PATH      gama;            
   /* dummy argument for trace_string function */
   int32_t  chars_found = 0;   
   
   if(pos->node == tree->root) { return; }

   /* If node has no suffix link yet or in the middle of an edge - remember the
      edge between the node and its father (gama) and follow its father's suffix
      link (it must have one by Ukkonen's lemma). After following, trace down 
      gama - it must exist in the tree (and thus can use the skip trick - see 
      trace_string function description) */
   if(pos->node->suffix_link == 0 || is_last_char_in_edge(tree,pos->node,pos->edge_pos) == 0)
   {
      /* If the node's father is the root, than no use following it's link (it 
         is linked to itself). Tracing from the root (like in the naive 
         algorithm) is required and is done by the calling function SEA uppon 
         recieving a return value of tree->root from this function */
      if(pos->node->father == tree->root)
      {
         pos->node = tree->root;
         return;
      }
      
      /* Store gama - the indices of node's incoming edge */
      gama.begin      = pos->node->edge_label_start;
      gama.end      = pos->node->edge_label_start + pos->edge_pos;
      /* Follow father's suffix link */
      pos->node      = pos->node->father->suffix_link;
      /* Down-walk gama back to suffix_link's son */
      pos->node      = trace_string(tree, pos->node, gama, &(pos->edge_pos), &chars_found, skip);
   }
   else
   {
      /* If a suffix link exists - just follow it */
      pos->node      = pos->node->suffix_link;
      pos->edge_pos   = get_node_label_length(tree,pos->node)-1;
   }
}

/******************************************************************************/
/*
   create_suffix_link :
   Creates a suffix link between node and the node 'link' which represents its 
   largest suffix. The function could be avoided but is needed to monitor the 
   creation of suffix links when debuging or changing the tree.

   Input : The node to link from, the node to link to.

   Output: None.
*/

static void create_suffix_link(NODE* node, NODE* link)
{
   node->suffix_link = link;
}

/******************************************************************************/
/*
   SEA :
   Single-Extension-Algorithm (see Ukkonen's algorithm). Ensure that a certain 
   extension is in the tree.

   1. Follows the current node's suffix link.
   2. Check whether the rest of the extension is in the tree.
   3. If it is - reports the calling function SPA of rule 3 (= current phase is 
      done).
   4. If it's not - inserts it by applying rule 2.

   Input : The tree, pos - the node and position in its incoming edge where 
           extension begins, str - the starting and ending indices of the 
           extension, a flag indicating whether the last phase ended by rule 3 
           (last extension of the last phase already existed in the tree - and 
           if so, the current phase starts at not following the suffix link of 
           the first extension).

   Output: The rule that was applied to that extension. Can be 3 (phase is done)
           or 2 (a new leaf was created).
*/

static void SEA(SuffixTree*   tree,
                      POS*           pos,
                      PATH           str, 
                      int32_t*      rule_applied,
                      char           after_rule_3,ContainerAllocator *allocator)
{
   int32_t   chars_found = 0 , path_pos = str.begin;
   NODE*      tmp;
 
#ifdef DEBUG   
   ST_PrintTree(tree);
   printf("extension: %lu  phase+1: %lu",str.begin, str.end);
   if(after_rule_3 == 0)
      printf("   followed from (%lu,%lu | %lu) ", pos->node->edge_label_start, get_node_label_end(tree,pos->node), pos->edge_pos);
   else
      printf("   starting at (%lu,%lu | %lu) ", pos->node->edge_label_start, get_node_label_end(tree,pos->node), pos->edge_pos);
#endif

   /* Follow suffix link only if it's not the first extension after rule 3 was applied */
   if(after_rule_3 == 0)
      follow_suffix_link(tree, pos);


   /* If node is root - trace whole string starting from the root, else - trace last character only */
   if(pos->node == tree->root) {
      pos->node = trace_string(tree, tree->root, str, &(pos->edge_pos), &chars_found, no_skip);
   }
   else {
      str.begin = str.end;
      chars_found = 0;

      /* Consider 2 cases:
         1. last character matched is the last of its edge */
      if(is_last_char_in_edge(tree,pos->node,pos->edge_pos)) {
         /* Trace only last symbol of str, search in the  NEXT edge (node) */
         tmp = find_son(tree, pos->node, tree->tree_string[str.end]);
         if(tmp != 0) {
            pos->node      = tmp;
            pos->edge_pos   = 0;
            chars_found      = 1;
         }
      }
      /* 2. last character matched is NOT the last of its edge */
      else {
         /* Trace only last symbol of str, search in the CURRENT edge (node) */
         if(tree->tree_string[pos->node->edge_label_start+pos->edge_pos+1] == tree->tree_string[str.end]) {
            pos->edge_pos++;
            chars_found   = 1;
         }
      }
   } /* end else not root */

   /* If whole string was found - rule 3 applies */
   if(chars_found == str.end - str.begin + 1) {
      *rule_applied = 3;
      /* If there is an internal node that has no suffix link yet (only one may 
         exist) - create a suffix link from it to the father-node of the 
         current position in the tree (pos) */
      if(suffixless != 0) {
         create_suffix_link(suffixless, pos->node->father);
         /* Marks that no internal node with no suffix link exists */
         suffixless = 0;
      }

      #ifdef DEBUG   
         printf("rule 3 (%lu,%lu)\n",str.begin,str.end);
      #endif
      return;
   }
   
   /* If last char found is the last char of an edge - add a character at the 
      next edge */
   if(is_last_char_in_edge(tree,pos->node,pos->edge_pos) || pos->node == tree->root) {
      /* Decide whether to apply rule 2 (new_son) or rule 1 */
      if(pos->node->sons != 0) {
         /* Apply extension rule 2 new son - a new leaf is created and returned 
            by apply_extension_rule_2 */
         apply_extension_rule_2(pos->node, str.begin+chars_found, str.end, path_pos, 0, new_son,tree);
         *rule_applied = 2;
         /* If there is an internal node that has no suffix link yet (only one 
            may exist) - create a suffix link from it to the father-node of the 
            current position in the tree (pos) */
         if (suffixless != 0) {
            create_suffix_link(suffixless, pos->node);
            /* Marks that no internal node with no suffix link exists */
            suffixless = 0;
         }
      }
   }
   else {
      /* Apply extension rule 2 split - a new node is created and returned by 
         apply_extension_rule_2 */
      tmp = apply_extension_rule_2(pos->node, str.begin+chars_found, str.end, path_pos, pos->edge_pos, split,tree);
      if(suffixless != 0)
         create_suffix_link(suffixless, tmp);
      /* Link root's sons with a single character to the root */
      if(get_node_label_length(tree,tmp) == 1 && tmp->father == tree->root) {
         tmp->suffix_link = tree->root;
         /* Marks that no internal node with no suffix link exists */
         suffixless = 0;
      }
      else
         /* Mark tmp as waiting for a link */
         suffixless = tmp;
      
      /* Prepare pos for the next extension */
      pos->node = tmp;
      *rule_applied = 2;
   }
}

/******************************************************************************/
/*
   SPA :
   Performs all insertion of a single phase by calling function SEA starting 
   from the first extension that does not already exist in the tree and ending 
   at the first extension that already exists in the tree. 

   Input :The tree, pos - the node and position in its incoming edge where 
          extension begins, the phase number, the first extension number of that
          phase, a flag signaling whether the extension is the first of this 
          phase, after the last phase ended with rule 3. If so - extension will 
          be executed again in this phase, and thus its suffix link would not be
          followed.

   Output:The extension number that was last executed on this phase. Next phase 
          will start from it and not from 1
*/

static void SPA(
                      /* The tree */
                      SuffixTree*    tree,            
                      /* Current node */
                      POS*            pos,            
                      /* Current phase number */
                      int32_t        phase,            
                      /* The last extension performed in the previous phase */
                      int32_t*       extension,         
                      /* 1 if the last rule applied is 3 */
                      char*           repeated_extension)   
{
   /* No such rule (0). Used for entering the loop */
   int32_t   rule_applied = 0;   
   PATH       str;
   
   /* Leafs Trick: Apply implicit extensions 1 through prev_phase */
   tree->e = phase+1;

   /* Apply explicit extensions untill last extension of this phase is reached 
      or extension rule 3 is applied once */
   while(*extension <= phase+1) {
      str.begin       = *extension;
      str.end         = phase+1;
      
      /* Call Single-Extension-Algorithm */
      SEA(tree, pos, str, &rule_applied, *repeated_extension,tree->allocator);
      
      /* Check if rule 3 was applied for the current extension */
      if(rule_applied == 3) {
         /* Signaling that the next phase's first extension will not follow a 
            suffix link because same extension is repeated */
         *repeated_extension = 1;
         break;
      }
      *repeated_extension = 0;
      (*extension)++;
   }
   return;
}

/******************************************************************************/
/*
   ST_CreateTree :
   Allocates memory for the tree and starts Ukkonen's construction algorithm by 
   calling SPA n times, where n is the length of the source string.

   Input : The source string and its length. The string is a sequence of 
           unsigned characters (maximum of 256 different symbols) and not 
           null-terminated. The only symbol that must not appear in the string 
           is 0 (the dollar sign). It is used as a unique symbol by the 
           algorithm ans is appended automatically at the end of the string (by 
           the program, not by the user!). The meaning of the 0 sign is 
           connected to the implicit/explicit suffix tree transformation, 
           detailed in Ukkonen's algorithm.

   Output: A pointer to the newly created tree. Keep this pointer in order to 
           perform operations like search and delete on that tree. Obviously, no
	   de-allocating of the tree space could be done if this pointer is 
	   lost, as the tree is allocated dynamically on the heap.
*/

static SuffixTree* ST_CreateTree(char* str, int32_t length,ContainerAllocator *allocator)
{
   SuffixTree*  tree;
   int32_t      phase , extension;
   char          repeated_extension = 0;
   POS           pos;

   if(str == 0) {
      iError.NullPtrError("SuffixTree.Create");
      return 0;
   }

   /* Allocating the tree */
   tree = allocator->calloc(1,sizeof(SuffixTree));
   if(tree == 0) {
      iError.RaiseError("iSuffixTree.Create", CONTAINER_ERROR_NOMEMORY);
      return NULL;
   }
   tree->heap+=sizeof(SuffixTree);

   /* Calculating string length (with an ending \0 sign) */
   tree->length         = length+1;
   
   /* Allocating the only real string of the tree */
   tree->tree_string = allocator->malloc((tree->length+1)*sizeof(char));
   if(tree->tree_string == 0)
   {
      iError.RaiseError("iList.Create", CONTAINER_ERROR_NOMEMORY);
      allocator->free(tree);
      return NULL;
   }
   tree->allocator = allocator;
   tree->RaiseError = iError.RaiseError;
   tree->heap+=(tree->length+1)*sizeof(char);

   memcpy(tree->tree_string+sizeof(char),str,length*sizeof(char));
   /* \0 is considered a unique symbol */
   tree->tree_string[tree->length] = '\0';
   
   /* Allocating the tree root node */
   tree->root            = create_node(0, 0, 0, 0,tree);
   tree->root->suffix_link = 0;

   /* Initializing algorithm parameters */
   extension = 2;
   phase = 2;
   
   /* Allocating first node, son of the root (phase 0), the longest path node */
   tree->root->sons = create_node(tree->root, 1, tree->length, 1,tree);
   suffixless       = 0;
   pos.node         = tree->root;
   pos.edge_pos     = 0;

   /* Ukkonen's algorithm begins here */
   for(; phase < tree->length; phase++)
   {
      /* Perform Single Phase Algorithm */
      SPA(tree, &pos, phase, &extension, &repeated_extension);
   }
   return tree;
}

/******************************************************************************/
/*
   ST_DeleteSubTree :
   Deletes a subtree that is under node. It recoursively calls itself for all of
   node's right sons and then deletes node.

  Input : The node that is the root of the subtree to be deleted.

  Output: None.
*/

static int ST_DeleteSubTree(NODE* node,ContainerAllocator *allocator)
{
   /* Recursion stoping condition */
   if(node == 0)
      return 0;
   /* Recoursive call for right sibling */
   if(node->right_sibling!=0)
      ST_DeleteSubTree(node->right_sibling,allocator);
   /* Recoursive call for first son */
   if(node->sons!=0)
      ST_DeleteSubTree(node->sons,allocator);
   /* Delete node itself, after its whole tree was deleted as well */
   allocator->free(node);
   return 1;
}

/******************************************************************************/
/*
   ST_DeleteTree :
   Deletes a whole suffix tree by starting a recoursive call to ST_DeleteSubTree
   from the root. After all of the nodes have been deleted, the function deletes
   the structure that represents the tree.

   Input : The tree to be deleted.

   Output: None.
*/

static int Finalize(SuffixTree* tree)
{
   if(tree == 0)
      return 0;
   ST_DeleteSubTree(tree->root,tree->allocator);
   tree->allocator->free(tree);
   return 1;
}

/******************************************************************************/
/*
   ST_PrintNode :
   Prints a subtree under a node of a certain tree-depth.

   Input : The tree, the node that is the root of the subtree, and the depth of 
           that node. The depth is used for printing the branches that are 
           coming from higher nodes and only then the node itself is printed. 
           This gives the effect of a tree on screen. In each recursive call, 
           the depth is increased.
  
   Output: A printout of the subtree to the screen.
*/

static void ST_PrintNode(SuffixTree* tree, NODE* node1, long depth)
{
   NODE* node2 = node1->sons;
   long  d = depth , start = node1->edge_label_start , end;
   end     = get_node_label_end(tree, node1);

   if(depth>0)
   {
      /* Print the branches coming from higher nodes */
      while(d>1)
      {
         fprintf(tree->outstream,"|");
         d--;
      }
      fprintf(tree->outstream,"+");
      /* Print the node itself */
      while(start<=end)
      {
         fprintf(tree->outstream,"%c",tree->tree_string[start]);
         start++;
      }
      #ifdef DEBUG
         fprintf(tree->outstream,"  \t\t\t(%lu,%lu | %lu)",node1->edge_label_start,end,node1->path_position);
      #endif
      fprintf(tree->outstream,"\n");
   }
   /* Recoursive call for all node1's sons */
   while(node2!=0)
   {
      ST_PrintNode(tree,node2, depth+1);
      node2 = node2->right_sibling;
   }
}

static void VisitNode(SuffixTree* tree, int (*Applyfn)(void *,void *),NODE* node1, long depth,void *arg)
{
   NODE* node2 = node1->sons;

   Applyfn(node1,arg);
   /* Recoursive call for all node1's sons */
   while(node2!=0)
   {
      VisitNode(tree,Applyfn,node2, depth+1,arg);
      node2 = node2->right_sibling;
   }
}

static int Apply(SuffixTree *tree, int(*Applyfn)(void *,void *),void *arg)
{
    if (tree == NULL || Applyfn == NULL) {
        if (tree)
            tree->RaiseError("SuffixTree.Aply",CONTAINER_ERROR_BADARG);
        else
            iError.RaiseError("SuffixTree.Apply",CONTAINER_ERROR_BADARG);
        return CONTAINER_ERROR_BADARG;
    }
    VisitNode(tree,Applyfn,tree->root,0,arg);
    return 1;
}
#ifdef NOTUSED
/******************************************************************************/
/*
   ST_PrintFullNode :
   This function prints the full path of a node, starting from the root. It 
   calls itself recursively and than prints the last edge.

   Input : the tree and the node its path is to be printed.

   Output: Prints the path to the screen, no return value.
*/

static void ST_PrintFullNode(SuffixTree* tree, NODE* node)
{
   long start, end;
   if(node==NULL)
      return;
   /* Calculating the begining and ending of the last edge */
   start   = node->edge_label_start;
   end     = get_node_label_end(tree, node);
   
   /* Stoping condition - the root */
   if(node->father!=tree->root)
      ST_PrintFullNode(tree,node->father);
   /* Print the last edge */
   while(start<=end)
   {
      printf("%c",tree->tree_string[start]);
      start++;
   }
}
#endif

/******************************************************************************/
/*
   ST_PrintTree :
   This function prints the tree. It simply starts the recursive function
   ST_PrintNode with depth 0 (the root).

   Input : The tree to be printed.
  
   Output: A print out of the tree to the screen.
*/

static void ST_PrintTree(SuffixTree* tree,FILE *outstream)
{
   fprintf(outstream,"\nroot\n");
   tree->outstream = outstream;
   ST_PrintNode(tree, tree->root, 0);
}

static SuffixTree *CreateWithAllocator(char *text,ContainerAllocator *allocator)
{
	return ST_CreateTree(text,strlen(text),allocator);
}

static SuffixTree *Create(char *text)
{
	return ST_CreateTree(text,strlen(text),CurrentAllocator);
}

static int Find(SuffixTree *tree,char *txt)
{
	return ST_FindSubstring(tree,txt,strlen(txt));
}

static int Clear(SuffixTree *tree)
{
	int r = ST_DeleteSubTree(tree->root,tree->allocator);
	tree->root = NULL;
    return r;
}

static size_t Sizeof(SuffixTree *tree)
{
    if (tree == NULL) return sizeof(SuffixTree);
    return tree->heap;
}

SuffixTreeInterface iSuffixTree = {
	Create,
    CreateWithAllocator,
    Find,
	Clear, 
	Finalize,
    ST_PrintTree,
    Apply,
    Sizeof,
};
