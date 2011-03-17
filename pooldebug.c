/*
The algorithms of this code have been adapted from the Apache runtime library.
*/
#ifndef TEST
#include "containers.h"
#ifndef _MSC_VER
#include <inttypes.h>
#endif
#else
#include <string.h>
#endif
#define MAX_INDEX	20
#define ALIGN(size, boundary) (((size) + ((boundary) - 1)) & ~((boundary) - 1))
#define ALIGN_DEFAULT(size) ALIGN(size, 8)

#ifdef TEST
/** The fundamental pool type */
typedef struct Pool Pool;
#endif
#define POOL_DEBUG_VERSION 1
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
    Pool         *owner;
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


/** Debug version of newPool. */
static Pool *newPool_debug( const char *file_line);
/**
 * Debug version of PoolClear.
 * @param p See: PoolClear.
 * @param file_line Where the function is called from.
 *        This is usually __FILE_LINE__.
 * @remark Only available when POOL_DEBUG_VERSION is defined.
 *         Call this directly if you have you PoolClear
 *         calls in a wrapper function and wish to override
 *         the file_line argument to reflect the caller of
 *         your wrapper function.  If you do not have
 *         PoolClear in a wrapper, trust the macro
 *         and don't call PoolDestroy_clear directly.
 */
static void PoolClear_debug(Pool *p, const char *file_line);

#define TOSTRING_HELPER(n) #n
#define TOSTRING(n) TOSTRING_HELPER(n)
#define __FILE_LINE__ __FILE__ ":" TOSTRING(__LINE__)
#define newPool()  newPool_debug( __FILE_LINE__)

#define PoolClear(p)  PoolClear_debug(p, __FILE_LINE__)

/**
 * Debug version of PoolDestroy.
 * @param p See: PoolDestroy.
 * @param file_line Where the function is called from.
 *        This is usually __FILE_LINE__.
 * @remark Only available when POOL_DEBUG_VERSION is defined.
 *         Call this directly if you have you PoolDestroy
 *         calls in a wrapper function and wish to override
 *         the file_line argument to reflect the caller of
 *         your wrapper function.  If you do not have
 *         PoolDestroy in a wrapper, trust the macro
 *         and don't call PoolDestroy_debug directly.
 */
static void PoolDestroy_debug(Pool *p, const char *file_line);
#define PoolDestroy(p)  PoolDestroy_debug(p, __FILE_LINE__)

/**
 * Debug version of PoolAlloc
 * @param p See: PoolAlloc
 * @param size See: PoolAlloc
 * @param file_line Where the function is called from.
 *        This is usually __FILE_LINE__.
 * @return See: PoolAlloc
 */
static void * PoolAlloc_debug(Pool *p, size_t size, const char *file_line);
#define PoolAlloc(p, size) PoolAlloc_debug(p, size, __FILE_LINE__)

/**
 * Debug version of PoolCalloc
 * @param p See: PoolCalloc
 * @param size See: PoolCalloc
 * @param file_line Where the function is called from.
 *        This is usually __FILE_LINE__.
 * @return See: PoolCalloc
 */
static void * PoolCalloc_debug(Pool *p, size_t n,size_t size, const char *file_line);
#define PoolCalloc(p, size) PoolCalloc_debug(p, size, __FILE_LINE__)


/**
 * Guarantee that a subpool has the same lifetime as the parent.
 * @param p The parent pool
 * @param sub The subpool
 */
void JoinPool(Pool *p, Pool *sub);

/**
 * Report the number of bytes currently in the pool
 * @param p The pool to inspect
 * @return The number of bytes
 */
static size_t Sizeof(Pool *p);

/**
 * Lock a pool
 * @param pool The pool to lock
 * @param flag  The flag
 */
void LockPool(Pool *pool, int flag);

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
 * Set the current threshold at which the allocator should start
 * giving blocks back to the system.
 * @param allocator The allocator the set the threshold on
 * @param size The threshold.  0 == unlimited.
 */
void SetMaxFree(Allocator *allocator, size_t size);

#ifdef THREAD_VERSION
#define NESTED_MUTEX	0
/**
 * Set a mutex for the allocator to use
 * @param allocator The allocator to set the mutex for
 * @param mutex The mutex
 */
void SetMutex(Allocator *allocator, Mutex *mutex);

/**
 * Get the mutex currently set for the allocator
 * @param allocator The allocator
 */
Mutex * GetMutex( Allocator *allocator);

#endif /* THREAD_VERSION */

/*
 * Magic numbers
 */

#define MIN_ALLOC 8192
#define BOUNDARY_INDEX 12
#define BOUNDARY_SIZE (1 << BOUNDARY_INDEX)

#ifdef THREAD_VERSION
void SetMutex(Allocator *allocator,
                                          Mutex *mutex)
{
    allocator->mutex = mutex;
}

Mutex * GetMutex( Allocator *allocator)
{
    return allocator->mutex;
}
#endif /* THREAD_VERSION */



/*
 * Debug level
 */

#define POOL_DEBUG_VERSION_GENERAL  0x01
#define POOL_DEBUG_VERSION_VERBOSE  0x02
#define POOL_DEBUG_VERSION_LIFETIME 0x04
#define POOL_DEBUG_VERSION_OWNER    0x08
#define POOL_DEBUG_VERSION_VERBOSE_ALLOC 0x10

#define POOL_DEBUG_VERSION_VERBOSE_ALL (POOL_DEBUG_VERSION_VERBOSE|POOL_DEBUG_VERSION_VERBOSE_ALLOC)
/*
 * Structures
 */

typedef struct debug_node_t debug_node_t;
struct debug_node_t {
    debug_node_t *next;
    uint32_t  index;
    void         *beginp[64];
    void         *endp[64];
};
#define SIZEOF_DEBUG_NODE_T ALIGN_DEFAULT(sizeof(debug_node_t))

/* The ref field in the Pool struct holds a
 * pointer to the pointer referencing this pool.
 */
struct Pool {
    Allocator      *allocator;
    const char           *tag;
    Pool           *joined; /* the caller has guaranteed that this pool
                                   * will survive as long as ->joined */
    debug_node_t         *nodes;
    const char           *file_line;
    uint32_t          creation_flags;
    unsigned int          stat_alloc;
    unsigned int          stat_total_alloc;
    unsigned int          stat_clear;
#ifdef THREAD_VERSION
    thread_t       owner;
    Mutex   *mutex;
#endif /* THREAD_VERSION */
};

#define SIZEOF_POOL_T       ALIGN_DEFAULT(sizeof(Pool))

static void destroyAllocator(Allocator *allocator)
{
    uint32_t idx;
    MemoryNode_t *node, **ref;

    for (idx = 0; idx < MAX_INDEX; idx++) {
        ref = &allocator->free[idx];
        while ((node = *ref) != NULL) {
            *ref = node->next;
            iDebugMalloc.free(node);
        }
    }
}


/*
 * Debug helper functions
 */

#include <stdio.h>
#if (POOL_DEBUG_VERSION & POOL_DEBUG_VERSION_VERBOSE_ALL)
static void Log(Pool *pool, const char *event, const char *file_line, int deref)
{
        if (deref) {
            fprintf(stderr,
                "POOL DEBUG: "
                "[%lu"
#if THREAD_VERSION
                "/%lu"
#endif /* THREAD_VERSION */
                "] "
                "%7s "
                "(%10lu) "
                "0x%pp \"%s\" "
                "<%s> "
                "(%u/%u/%u) "
                "\n",
                0UL,
#if THREAD_VERSION
                (unsigned long)GetCurrentThread(),
#endif /* THREAD_VERSION */
                event,
                (unsigned long)Sizeof(pool),
                pool, pool->tag,
                file_line,
                pool->stat_alloc, pool->stat_total_alloc, pool->stat_clear);
        }
        else {
            fprintf(stderr,
                "POOL DEBUG: "
                "[%lu"
#if THREAD_VERSION
                "/%lu"
#endif /* THREAD_VERSION */
                "] "
                "%7s "
                "                                   "
                "0x%pp "
                "<%s> "
                "\n",
                0UL,
#if THREAD_VERSION
                (unsigned long)GetCurrentThread(),
#endif /* THREAD_VERSION */
                event,
                pool,
                file_line);
        }
}
#endif /* (POOL_DEBUG_VERSION & POOL_DEBUG_VERSION_VERBOSE_ALL) */

static void CheckIntegrity(Pool *pool)
{
}


/*
 * Memory allocation (debug)
 */

static void *pool_alloc_debug(Pool *pool, size_t size,const char *file_line)
{
    debug_node_t *node;
    void *mem;

    if ((mem = iDebugMalloc.malloc(size)) == NULL) {
        return NULL;
    }
    node = pool->nodes;
    if (node == NULL || node->index == 64) {
        if ((node = calloc(1,SIZEOF_DEBUG_NODE_T)) == NULL) {
            return NULL;
        }
        node->next = pool->nodes;
        pool->nodes = node;
    }

    node->beginp[node->index] = mem;
    node->endp[node->index] = (char *)mem + size;
    node->index++;

    pool->stat_alloc++;
    pool->stat_total_alloc++;

    return mem;
}

static void * PoolAlloc_debug(Pool *pool, size_t size, const char *file_line)
{
    void *mem;

    CheckIntegrity(pool);

    mem = pool_alloc_debug(pool, size,file_line);

#if (POOL_DEBUG_VERSION & POOL_DEBUG_VERSION_VERBOSE_ALLOC)
    Log(pool, "PALLOC", file_line, 1);
#endif /* (POOL_DEBUG_VERSION & POOL_DEBUG_VERSION_VERBOSE_ALLOC) */

    return mem;
}

static void * PoolCalloc_debug(Pool *pool, size_t n,size_t size, const char *file_line)
{
    void *mem;

    CheckIntegrity(pool);

    mem = pool_alloc_debug(pool, n*size,file_line);
    if (mem)
        memset(mem, 0, size);

#if (POOL_DEBUG_VERSION & POOL_DEBUG_VERSION_VERBOSE_ALLOC)
    Log(pool, "PCALLOC", file_line, 1);
#endif /* (POOL_DEBUG_VERSION & POOL_DEBUG_VERSION_VERBOSE_ALLOC) */

    return mem;
}


/*
 * Pool creation/destruction (debug)
 */

#define POOL_POISON_BYTE 'A'

static void pool_clear_debug(Pool *pool, const char *file_line)
{
    debug_node_t *node;
    uint32_t idx;

    /* Free the blocks, scribbling over them first to help highlight
     * use-after-free issues. */
    while ((node = pool->nodes) != NULL) {
        pool->nodes = node->next;

        for (idx = 0; idx < node->index; idx++) {
            memset(node->beginp[idx], POOL_POISON_BYTE,
                   (char *)node->endp[idx] - (char *)node->beginp[idx]);
            iDebugMalloc.free(node->beginp[idx]);
        }

        memset(node, POOL_POISON_BYTE, SIZEOF_DEBUG_NODE_T);
        iDebugMalloc.free(node);
    }

    pool->stat_alloc = 0;
    pool->stat_clear++;
}

static void PoolClear_debug(Pool *pool, const char *file_line)
{
#ifdef THREAD_VERSION
    Mutex *mutex = NULL;
#endif

    CheckIntegrity(pool);

#if (POOL_DEBUG_VERSION & POOL_DEBUG_VERSION_VERBOSE)
    Log(pool, "CLEAR", file_line, 1);
#endif /* (POOL_DEBUG_VERSION & POOL_DEBUG_VERSION_VERBOSE) */

#ifdef THREAD_VERSION

    /* Lock the parent mutex before clearing so that if we have our
     * own mutex it won't be accessed by pool_walk_tree after
     * it has been destroyed.
     */
    if (mutex != NULL && mutex != pool->mutex) {
        LockMutex(mutex);
    }
#endif

    pool_clear_debug(pool, file_line);

#ifdef THREAD_VERSION
    /* If we had our own mutex, it will have been destroyed by
     * the registered cleanups.  Recreate the mutex.  Unlock
     * the mutex we obtained above.
     */
    if (mutex != pool->mutex) {
        (void)CreateMutex(&pool->mutex,
                                      NESTED_MUTEX, pool);

        if (mutex != NULL)
            (void)MutexUnlock(mutex);
    }
#endif /* THREAD_VERSION */
}

static void PoolDestroy_debug(Pool *pool, const char *file_line)
{
    if (pool->joined) {
        /* Joined pools must not be explicitly destroyed; the caller
         * has broken the guarantee. */
#if (POOL_DEBUG_VERSION & POOL_DEBUG_VERSION_VERBOSE_ALL)
        Log(pool, "LIFE",
                           __FILE__ ":PoolDestroy abort on joined", 0);
#endif /* (POOL_DEBUG_VERSION & POOL_DEBUG_VERSION_VERBOSE_ALL) */

        abort();
    }
    CheckIntegrity(pool);

#if (POOL_DEBUG_VERSION & POOL_DEBUG_VERSION_VERBOSE)
    Log(pool, "DESTROY", file_line, 1);
#endif /* (POOL_DEBUG_VERSION & POOL_DEBUG_VERSION_VERBOSE) */

    pool_clear_debug(pool, file_line);

    /* Remove the pool from the parents child list */
    if (pool->allocator != NULL
        && pool->allocator->owner == pool) {
        destroyAllocator(pool->allocator);
    }

    /* Free the pool itself */
    iDebugMalloc.free(pool);

}

static Pool *newPool_debug( const char *file_line)
{
    Pool *pool;
    Allocator *pool_allocator;

    if ((pool = calloc(1,SIZEOF_POOL_T)) == NULL) {
         return NULL;
    }

    pool->tag = file_line;
    pool->file_line = file_line;

#ifdef THREAD_VERSION
    pool->owner = GetCurrentThread();
#endif /* THREAD_VERSION */
     if ((pool_allocator = calloc(1,sizeof(Allocator))) == NULL) {
        free(pool); /* Was missing! */
        return NULL;
    }
    pool_allocator->owner = pool;
    pool->allocator = pool_allocator;

#ifdef THREAD_VERSION
    if ((CreateMutex(&pool->mutex, NESTED_MUTEX, pool)) != 0) {
        Free(pool);
        return NULL;
    }
#endif /* THREAD_VERSION */

#if (POOL_DEBUG_VERSION & POOL_DEBUG_VERSION_VERBOSE)
    Log(pool, "CREATE", file_line, 1);
#endif /* (POOL_DEBUG_VERSION & POOL_DEBUG_VERSION_VERBOSE) */

    return pool;
}

/*
 * Debug functions
 */

static int FindPoolFromData(Pool *pool, void *data)
{
    void **pmem = (void **)data;
    debug_node_t *node;
    uint32_t idx;

    node = pool->nodes;

    while (node) {
        for (idx = 0; idx < node->index; idx++) {
             if (node->beginp[idx] <= *pmem
                 && node->endp[idx] > *pmem) {
                 *pmem = pool;
                 return 1;
             }
        }

        node = node->next;
    }

    return 0;
}

static void SetMaxSize(Pool *pool,size_t in_size)
{
    uint32_t max_free_index;
    uint32_t size = (uint32_t)in_size;
	Allocator *allocator = pool->allocator;

#ifdef THREAD_VERSION
    Mutex *mutex;

    mutex = GetMutex(allocator);
    if (mutex != NULL)
        LockMutex(mutex);
#endif /* THREAD_VERSION */

    max_free_index = ALIGN(size, BOUNDARY_SIZE) >> BOUNDARY_INDEX;
    allocator->current_free_index += max_free_index;
    allocator->current_free_index -= allocator->max_free_index;
    allocator->max_free_index = max_free_index;
    if (allocator->current_free_index > max_free_index)
		allocator->current_free_index = max_free_index;

#ifdef THREAD_VERSION
    if (mutex != NULL)
        MutexUnlock(mutex);
#endif
}

static size_t Sizeof(Pool *pool)
{
    size_t size = 0;
    debug_node_t *node;
    uint32_t idx;

	if (pool == NULL)
		return sizeof(Pool);
    node = pool->nodes;
    while (node) {
        for (idx = 0; idx < node->index; idx++) {
            size += (char *)node->endp[idx] - (char *)node->beginp[idx];
        }
        node = node->next;
    }
    return size;
}
PoolAllocatorDebugInterface iPoolDebug = {
	newPool_debug,
	PoolAlloc_debug,
	PoolCalloc_debug,
	PoolClear_debug,
	PoolDestroy_debug,
	FindPoolFromData,
	SetMaxSize,
	Sizeof,
};


#ifdef TEST
int main(void)
{
	Pool *pool;
	void *mem;
	pool = newPool();
	mem = PoolAlloc(pool,1024);
	memset(mem,0,1024);
	PoolDestroy(pool);

	return 0;
}
#endif
