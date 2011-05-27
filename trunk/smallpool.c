/*
Some of the algorithms of this code have been adapted from the Apache runtime library.
*/
#include <string.h>
#include <stdlib.h>
#ifndef _MSC_VER
#include <inttypes.h>
#else
typedef unsigned int uint32_t;
#define UINT32_MAX 0xffffffff
#endif
#include "containers.h"
#include "ccl_internal.h"
#define MAX_INDEX	20
#define ALIGN(size, boundary) (((size) + ((boundary) - 1)) & ~((boundary) - 1))
#define ALIGN_DEFAULT(size) ALIGN(size, 8)

/*
* Note The max_free_index and current_free_index fields are not really
* indices, but quantities of BOUNDARY_SIZE big memory blocks.
*/
struct MemoryNode_t {
    struct MemoryNode_t *next;            /**< next memnode */
    struct MemoryNode_t **ref;            /**< reference to self */
    uint32_t   index;           /**< size */
    uint32_t   free_index;      /**< how much free */
    char          *first_avail;     /**< pointer to first free memory */
    char          *endp;            /**< pointer to end of free memory */
};

typedef struct MemoryNode_t MemoryNode_t;
typedef struct {
    /** largest used index into free[], always < MAX_INDEX */
    uint32_t        max_index;
    /** Total size (in BOUNDARY_SIZE multiples) of unused memory before
     * blocks are given back. @see SetMaxFree().
     * @note Initialized to 0,
     * which means to never give back blocks.
     */
    uint32_t        max_free_index;
    /**
     * Memory size (in BOUNDARY_SIZE multiples) that currently must be freed
     * before blocks are given back. Range: 0..max_free_index
     */
    uint32_t        current_free_index;
    /**
     * Lists of free nodes. Slot 0 is used for oversized nodes,
     * and the slots 1..MAX_INDEX-1 contain nodes of sizes
     * (i+1) * BOUNDARY_SIZE. Example for BOUNDARY_INDEX == 12:
     * slot  0: nodes larger than 81920
     * slot  1: size  8192
     * slot  2: size 12288
     * ...
     * slot 19: size 81920
     */
    MemoryNode_t      *free[MAX_INDEX];
} Allocator;


/** Create a new pool. */
Pool *newPool(void);
/** Debug version of newPool. */
Pool *newPool_debug( const char *file_line);

/**
 * Clear all memory in the pool and run all the cleanups. This also destroys all
 * subpools.
 * @param p The pool to clear
 * @remark This does not actually free the memory, it just allows the pool
 *         to re-use this memory for the next allocation.
 * @see PoolDestroy()
 */
void PoolClear(Pool *p);

/**
 * Destroy the pool. This takes similar action as PoolClear() and then
 * frees all the memory.
 * @param p The pool to destroy
 * @remark This will actually free the memory
 */
void PoolDestroy(Pool *p);

/**
 * Allocate a block of memor from a pool
 * @param p The pool to allocate from
 * @param size The amount of memory to allocate
 * @return The allocated memory
 */
void *PoolAlloc(Pool *p, size_t size);

/** The base size of a memory node - aligned.  */
#define MEMORYNODE_SIZE ALIGN_DEFAULT(sizeof(MemoryNode_t))

/**
 * Destroy an allocator
 * @param allocator The allocator to be destroyed
 * @remark Any memnodes not given back to the allocator prior to destroying
 *         will _not_ be free()d.
 */
static void destroyAllocator(Allocator *allocator);

/**
 * Free a list of blocks of mem, giving them back to the allocator.
 * The list is typically terminated by a memnode with its next field
 * set to NULL.
 * @param allocator The allocator to give the mem back to
 * @param memnode The memory node to return
 */
static void allocator_free(Allocator *allocator, MemoryNode_t *memnode);

/**
 * Set the current threshold at which the allocator should start
 * giving blocks back to the system.
 * @param allocator The allocator the set the threshold on
 * @param size The threshold.  0 == unlimited.
 */
void SetMaxFree(Allocator *allocator, size_t size);

/*
 * Magic numbers
 */

#define MIN_ALLOC 8192
#define BOUNDARY_INDEX 12
#define BOUNDARY_SIZE (1 << BOUNDARY_INDEX)

void SetMaxFree(Allocator *allocator,size_t in_size)
{
    uint32_t max_free_index;
    uint32_t size = (uint32_t)in_size;

    max_free_index = ALIGN(size, BOUNDARY_SIZE) >> BOUNDARY_INDEX;
    allocator->current_free_index += max_free_index;
    allocator->current_free_index -= allocator->max_free_index;
    allocator->max_free_index = max_free_index;
    if (allocator->current_free_index > max_free_index)
       allocator->current_free_index = max_free_index;

}

static MemoryNode_t *newAllocator(Allocator *allocator, size_t in_size)
{
    MemoryNode_t *node, **ref;
    uint32_t max_index;
    size_t size, i, index;

    /* Round up the block size to the next boundary, but always
     * allocate at least a certain size (MIN_ALLOC).
     */
    size = ALIGN(in_size + MEMORYNODE_SIZE, BOUNDARY_SIZE);
    if (size < in_size) {
        return NULL;
    }
    if (size < MIN_ALLOC)
        size = MIN_ALLOC;

    /* Find the index for this node size by
     * dividing its size by the boundary size
     */
    index = (size >> BOUNDARY_INDEX) - 1;

    if (index > UINT32_MAX) {
        return NULL;
    }

    /* First see if there are any nodes in the area we know
     * our node will fit into.
     */
    if (index <= allocator->max_index) {

        /* Walk the free list to see if there are
         * any nodes on it of the requested size
         *
         * NOTE: an optimization would be to check
         * allocator->free[index] first and if no
         * node is present, directly use
         * allocator->free[max_index].  This seems
         * like overkill though and could cause
         * memory waste.
         */
        max_index = allocator->max_index;
        ref = &allocator->free[index];
        i = index;
        while (*ref == NULL && i < max_index) {
           ref++;
           i++;
        }

        if ((node = *ref) != NULL) {
            /* If we have found a node and it doesn't have any
             * nodes waiting in line behind it _and_ we are on
             * the highest available index, find the new highest
             * available index
             */
            if ((*ref = node->next) == NULL && i >= max_index) {
                do {
                    ref--;
                    max_index--;
                }
                while (*ref == NULL && max_index > 0);

                allocator->max_index = max_index;
            }

            allocator->current_free_index += node->index;
            if (allocator->current_free_index > allocator->max_free_index)
                allocator->current_free_index = allocator->max_free_index;

            node->next = NULL;
            node->first_avail = (char *)node + MEMORYNODE_SIZE;

            return node;
        }

    }

    /* If we found nothing, seek the sink (at index 0), if
     * it is not empty.
     */
    else if (allocator->free[0]) {
        /* Walk the free list to see if there are
         * any nodes on it of the requested size
         */
        ref = &allocator->free[0];
        while ((node = *ref) != NULL && index > node->index)
            ref = &node->next;

        if (node) {
            *ref = node->next;

            allocator->current_free_index += node->index;
            if (allocator->current_free_index > allocator->max_free_index)
                allocator->current_free_index = allocator->max_free_index;

            node->next = NULL;
            node->first_avail = (char *)node + MEMORYNODE_SIZE;
            return node;
        }

    }

    /* If we haven't got a suitable node, malloc a new one
     * and initialize it.
     */
    if ((node = malloc(size)) == NULL)
        return NULL;

    node->next = NULL;
    node->index = (uint32_t)index;
    node->first_avail = (char *)node + MEMORYNODE_SIZE;
    node->endp = (char *)node + size;

    return node;
}

static void allocator_free(Allocator *allocator, MemoryNode_t *node)
{
    MemoryNode_t *next, *freelist = NULL;
    uint32_t index, max_index;
    uint32_t max_free_index, current_free_index;

    max_index = allocator->max_index;
    max_free_index = allocator->max_free_index;
    current_free_index = allocator->current_free_index;

    /* Walk the list of submitted nodes and free them one by one,
     * shoving them in the right 'size' buckets as we go.
     */
    do {
        next = node->next;
        index = node->index;

        if (max_free_index != 0
            && index > current_free_index) {
            node->next = freelist;
            freelist = node;
        }
        else if (index < MAX_INDEX) {
            /* Add the node to the appropiate 'size' bucket.  Adjust
             * the max_index when appropiate.
             */
            if ((node->next = allocator->free[index]) == NULL
                && index > max_index) {
                max_index = index;
            }
            allocator->free[index] = node;
            if (current_free_index >= index)
                current_free_index -= index;
            else
                current_free_index = 0;
        }
        else {
            /* This node is too large to keep in a specific size bucket,
             * just add it to the sink (at index 0).
             */
            node->next = allocator->free[0];
            allocator->free[0] = node;
            if (current_free_index >= index)
                current_free_index -= index;
            else
                current_free_index = 0;
        }
    } while ((node = next) != NULL);

    allocator->max_index = max_index;
    allocator->current_free_index = current_free_index;

    while (freelist != NULL) {
        node = freelist;
        freelist = node->next;
        free(node);
    }
}

/* The ref field in the Pool struct holds a
 * pointer to the pointer referencing this pool.
 */
struct Pool {
    Allocator      *allocator;
    const char           *tag;
    MemoryNode_t        *active;
    MemoryNode_t        *self; /* The node containing the pool itself */
    char                 *self_first_avail;
};

#define SIZEOF_POOL_T       ALIGN_DEFAULT(sizeof(Pool))

static void destroyAllocator(Allocator *allocator)
{
    uint32_t index;
    MemoryNode_t *node, **ref;

    for (index = 0; index < MAX_INDEX; index++) {
        ref = &allocator->free[index];
        while ((node = *ref) != NULL) {
            *ref = node->next;
            free(node);
        }
    }
}


/* Node list management helper macros; list_insert() inserts 'node'
 * before 'point'. */
#define list_insert(node, point) do {           \
    node->ref = point->ref;                     \
    *node->ref = node;                          \
    node->next = point;                         \
    point->ref = &node->next;                   \
} while (0)

/* list_remove() removes 'node' from its list. */
#define list_remove(node) do {                  \
    *node->ref = node->next;                    \
    node->next->ref = node->ref;                \
} while (0)

/* Returns the amount of free space in the given node. */
#define node_free_space(node_) ((size_t)(node_->endp - node_->first_avail))

/*
 * Memory allocation
 */

void * PoolAlloc(Pool *pool, size_t in_size)
{
    MemoryNode_t *active, *node;
    void *mem;
    size_t size, free_index;

    size = roundup(in_size);
    if (size < in_size) {
        return NULL;
    }
    active = pool->active;

    /* If the active node has enough bytes left, use it. */
    if (size <= node_free_space(active)) {
        mem = active->first_avail;
        active->first_avail += size;

        return mem;
    }

    node = active->next;
    if (size <= node_free_space(node)) {
        list_remove(node);
    }
    else {
        if ((node = newAllocator(pool->allocator, size)) == NULL) {
            return NULL;
        }
    }

    node->free_index = 0;

    mem = node->first_avail;
    node->first_avail += size;

    list_insert(node, active);

    pool->active = node;

    free_index = (ALIGN(active->endp - active->first_avail + 1,
                            BOUNDARY_SIZE) - BOUNDARY_SIZE) >> BOUNDARY_INDEX;

    active->free_index = (uint32_t)free_index;
    node = active->next;
    if (free_index >= node->free_index)
        return mem;

    do {
        node = node->next;
    }
    while (free_index < node->free_index);

    list_remove(active);
    list_insert(active, node);

    return mem;
}

void * PoolCalloc(Pool *pool, size_t size)
{
    void *mem;

    if ((mem = PoolAlloc(pool, size)) != NULL) {
        memset(mem, 0, size);
    }
    return mem;
}


/*
 * Pool creation/destruction
 */

void PoolClear(Pool *pool)
{
    MemoryNode_t *active;

    /* Find the node attached to the pool structure, reset it, make
     * it the active node and free the rest of the nodes.
     */
    active = pool->active = pool->self;
    active->first_avail = pool->self_first_avail;

    if (active->next == active)
        return;

    *active->ref = NULL;
    allocator_free(pool->allocator, active->next);
    active->next = active;
    active->ref = &active->next;
}

void PoolDestroy(Pool *pool)
{
    MemoryNode_t *active;
    Allocator *allocator;

    /* Find the block attached to the pool structure.  Save a copy of the
     * allocator pointer, because the pool struct soon will be no more.
     */
    allocator = pool->allocator;
    active = pool->self;
    *active->ref = NULL;

    /* Free all the nodes in the pool (including the node holding the
     * pool struct), by giving them back to the allocator.
     */
    allocator_free(allocator, active);

    destroyAllocator(allocator);
    free(allocator);
}

Pool *newPool(void)
{
    Pool *pool;
    MemoryNode_t *node;
    Allocator *pool_allocator;

    if ((pool_allocator = calloc(1,sizeof(Allocator))) == NULL) {
        return NULL;
    }
    if ((node = newAllocator(pool_allocator, MIN_ALLOC - MEMORYNODE_SIZE)) == NULL) {
        return NULL;
    }

    node->next = node;
    node->ref = &node->next;

    pool = (Pool *)node->first_avail;
    node->first_avail = pool->self_first_avail = (char *)pool + SIZEOF_POOL_T;

    pool->allocator = pool_allocator;
    pool->active = pool->self = node;
    pool->tag = NULL;

    return pool;
}

#ifdef TEST 
int main(void)
{
	Pool *pool;
	void *mem;
	pool = newPool();
	mem = PoolAlloc(pool,1024);
	memset(mem,0,1024);

	mem = PoolAlloc(pool,235);
	memset(mem,0,235);

	mem = PoolAlloc(pool,65243);
	memset(mem,0,65243);

	PoolDestroy(pool);

	return 0;
}
#endif
