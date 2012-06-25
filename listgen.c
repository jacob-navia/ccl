/*
 * Value types generic list routines sample implementation 
 * ----------------------------------- ------------------
 * Thisroutines handle the List container class. This is a very general
 * implementation and efficiency considerations aren't yet primordial. Lists
 * can have elements of any size. This implement single linked Lists. The
 * design goals here are just correctness and showing how the implementation
 * of the proposed interface COULD be done.
 * ----------------------------------------------------------------------
 */

#include "listgen.h"
#include "ccl_internal.h"

/* Forward declarations */
static LIST_TYPE *SetVTable(LIST_TYPE *result);
static LIST_TYPE * Create(void);
static LIST_TYPE *CreateWithAllocator(const ContainerAllocator * allocator);
#define CONTAINER_LIST_SMALL    2
#define CHUNK_SIZE    1000

static int DefaultListCompareFunction(const void * left, const void * right, CompareInfo * ExtraArgs)
{
    size_t          siz = ((LIST_TYPE *) ExtraArgs->ContainerLeft)->ElementSize;
    return memcmp(left, right, siz);
}

/*------------------------------------------------------------------------
 Procedure:     Contains ID:1
 Purpose:       Determines if the given data is in the container
 Input:         The list and the data to be searched
 Output:        Returns 1 (true) if the data is in there, false
                otherwise
 Errors:        The same as the function IndexOf
------------------------------------------------------------------------*/
static int Contains(const LIST_TYPE * l, const DATA_TYPE data)
{
    size_t idx;
    return (iList.IndexOf((List *)l, &data, NULL, &idx) < 0) ? 0 : 1;
}

static int Add(LIST_TYPE * l, const DATA_TYPE elem)
{
    return iList.Add((List *)l,&elem);
}

static int CopyElement(const LIST_TYPE * l, size_t position, DATA_TYPE *outBuffer)
{
    return iList.CopyElement((List *)l,position,outBuffer);
}

static int ReplaceAt(LIST_TYPE * l, size_t position, const DATA_TYPE data)
{
    return iList.ReplaceAt((List *)l,position,&data);;
}

static int PushFront(LIST_TYPE * l, const DATA_TYPE pdata)
{
    return iList.PushFront((List *)l,&pdata);
}


static int PopFront(LIST_TYPE * l, DATA_TYPE *result)
{
    return iList.PushFront((List *)l,result);
}


static int InsertAt(LIST_TYPE * l, size_t pos, const DATA_TYPE pdata)
{
    return iList.InsertAt((List *)l,pos,&pdata);
}

static int Erase(LIST_TYPE * l, const DATA_TYPE elem)
{
    return iList.Erase((List *)l, &elem);
}

static int EraseAll(LIST_TYPE * l, const DATA_TYPE elem)
{
    return iList.EraseAll((List *)l, &elem);
}

static int IndexOf(const LIST_TYPE * l, const DATA_TYPE ElementToFind, void *ExtraArgs, size_t * result)
{
    return iList.IndexOf((List *)l, &ElementToFind, ExtraArgs, result);
}

static size_t Sizeof(const LIST_TYPE * l)
{
    if (l == NULL) {
        return sizeof(List);
    }
    return sizeof(LIST_TYPE) + l->ElementSize * l->count + l->count * offsetof(LIST_ELEMENT,Data);
}

static size_t SizeofIterator(const LIST_TYPE * l)
{
    return sizeof(struct ITERATOR(DATA_TYPE));
}

static LIST_TYPE *Load(FILE * stream, ReadFunction loadFn, void *arg)
{
    LIST_TYPE *result = (LIST_TYPE *)iList.Load(stream,loadFn,arg);
    return SetVTable(result);
}

/*
 * ---------------------------------------------------------------------------
 *
 *                           Iterators
 *
 * ---------------------------------------------------------------------------
 */
static int ReplaceWithIterator(Iterator * it, DATA_TYPE data, int direction)
{
    struct ITERATOR(DATA_TYPE) *iter = (struct ITERATOR(DATA_TYPE) *)it;
    return iter->ListReplace(it,&data,direction);
}

static Iterator *SetupIteratorVTable(struct ITERATOR(DATA_TYPE) *result)
{
    if (result == NULL) return NULL;
    result->ListReplace = result->it.Replace;
    result->it.Replace = (int (*)(Iterator * , void * , int ))ReplaceWithIterator;
    return &result->it;
}

static Iterator *NewIterator(LIST_TYPE * L)
{
    return SetupIteratorVTable((struct ITERATOR(DATA_TYPE) *)iList.NewIterator((List *)L));
}
static int InitIterator(LIST_TYPE * L, void *r)
{
    iList.InitIterator((List *)L,r);
    SetupIteratorVTable(r);
    return 1;
}
static size_t GetElementSize(const LIST_TYPE * l)
{
    return sizeof(DATA_TYPE);
}

static LIST_TYPE  *Copy(const LIST_TYPE * l)
{
    LIST_TYPE *result = (LIST_TYPE *)iList.Copy((List *)l);
    return SetVTable(result);
}


static LIST_TYPE *SelectCopy(const LIST_TYPE *l,const Mask *m)
{
    LIST_TYPE *result = (LIST_TYPE *)iList.SelectCopy((const List *)l,m);
    return SetVTable(result);
}

/*---------------------------------------------------------------------------*/
/* qsort() - perform a quicksort on an array                                 */
/*---------------------------------------------------------------------------*/
#define CUTOFF 8

static void shortsort(LIST_ELEMENT **lo, LIST_ELEMENT **hi);
#define swap(a,b) { LIST_ELEMENT *tmp = *a; *a = *b; *b = tmp; }

#ifndef COMPARE_EXPRESSION
#error foo
#define COMPARE_EXPRESSION(l,lo) comp(l, lo)
//COMPAR(A, B) (B > A ? -1 : B != A)
#endif

static void QSORT(LIST_ELEMENT **base, size_t num)
{
  LIST_ELEMENT **lo, **hi, **mid;
  LIST_ELEMENT **loguy, **higuy;
  size_t size;
  LIST_ELEMENT **lostk[30], **histk[30];
  int stkptr;

  if (num < 2) return;
  stkptr = 0;

  lo = base;
  hi = base + (num - 1);

recurse:
  size = (hi - lo) + 1;

  if (size <= CUTOFF) {
    shortsort(lo, hi);
  }
  else {
    mid = lo + (size / 2);
    swap(mid, lo);

    loguy = lo;
    higuy = hi + 1;

    for (;;) {
      // Changed <= to < and >= to > according to the advise of "pete" of comp.lang.c.
      do { loguy++; } while (loguy <= hi && COMPARE_EXPRESSION(loguy,lo) < 0);
      do { higuy--; } while (higuy > lo && COMPARE_EXPRESSION(higuy,lo) > 0);
      if (higuy < loguy) break;
      swap(loguy, higuy);
    }

    swap(lo, higuy);

    if (higuy - 1 - lo >= hi - loguy) {
      if (lo + 1 < higuy) {
        lostk[stkptr] = lo;
        histk[stkptr] = (higuy - 1);
        ++stkptr;
      }

      if (loguy < hi) {
        lo = loguy;
        goto recurse;
      }
    }
    else {
      if (loguy < hi) {
        lostk[stkptr] = loguy;
        histk[stkptr] = hi;
        ++stkptr;
      }

      if (lo + 1 < higuy) {
        hi = higuy - 1;
        goto recurse;
      }
    }
  }

  --stkptr;
  if (stkptr >= 0) {
    lo = lostk[stkptr];
    hi = histk[stkptr];
    goto recurse;
  }
}

static void shortsort(LIST_ELEMENT **lo, LIST_ELEMENT **hi)
{
  LIST_ELEMENT **p, **max;

  while (hi > lo) 
  {
    max = lo;
    for (p = (lo+1); p <= hi; p++) if (COMPARE_EXPRESSION(p,max) > 0) max = p;
    swap(max, hi);
    hi--;
  }
}

static int Sort(LIST_TYPE * l)
{
    LIST_ELEMENT   **tab;
    size_t          i;
    LIST_ELEMENT    *rvp;

    if (l == NULL)
        return iError.NullPtrError("Sort");

    if (l->count < 2)
        return 1;
    if (l->Flags & CONTAINER_READONLY) {
        l->RaiseError("iList.Sort", CONTAINER_ERROR_READONLY);
        return CONTAINER_ERROR_READONLY;
    }
    tab = l->Allocator->malloc(l->count * sizeof(LIST_ELEMENT *));
    if (tab == NULL) {
        l->RaiseError("iList.Sort", CONTAINER_ERROR_NOMEMORY);
        return CONTAINER_ERROR_NOMEMORY;
    }
    rvp = l->First;
    for (i = 0; i < l->count; i++) {
        tab[i] = rvp;
        rvp = rvp->Next;
    }
    QSORT(tab, l->count );
    for (i = 0; i < l->count - 1; i++) {
        tab[i]->Next = tab[i + 1];
    }
    tab[l->count - 1]->Next = NULL;
    l->Last = tab[l->count - 1];
    l->First = tab[0];
    l->Allocator->free(tab);
    return 1;

}
static LIST_TYPE *SetVTable(LIST_TYPE *result)
{
    static int Initialized;
    INTERFACE(DATA_TYPE) *intface = &INTERFACE_NAME(DATA_TYPE);
    
    result->VTable = intface;
    if (Initialized) return result;
    Initialized = 1;
    intface->FirstElement = (LIST_ELEMENT *(*)(LIST_TYPE *))iList.FirstElement;
    intface->LastElement = (LIST_ELEMENT *(*)(LIST_TYPE *))iList.LastElement;
    intface->GetElement = (DATA_TYPE *(*)(const LIST_TYPE *,size_t))iList.GetElement;
    intface->Clear = (int (*)(LIST_TYPE *))iList.Clear;
    intface->EraseAt = (int (*)(LIST_TYPE *,size_t))iList.EraseAt;
    intface->RemoveRange = (int (*)(LIST_TYPE *,size_t,size_t))iList.RemoveRange;
    intface->Select = (int (*)(LIST_TYPE *,const Mask *))iList.Select;
    intface->SetFlags = (unsigned (*)(LIST_TYPE *,unsigned))iList.SetFlags;
    intface->GetFlags = (unsigned (*)(const LIST_TYPE *))iList.GetFlags;
    intface->SetDestructor = (DestructorFunction (*)(LIST_TYPE *, DestructorFunction))iList.SetDestructor;
    intface->Apply = (int (*)(LIST_TYPE *, int (Applyfn) (DATA_TYPE *, void * ), void *))iList.Apply;
    intface->Reverse = (int (*)(LIST_TYPE *))iList.Reverse;
    intface->SetCompareFunction = (CompareFunction (*)(LIST_TYPE *, CompareFunction ))iList.SetCompareFunction;
    intface->GetRange = (LIST_TYPE *(*)(const LIST_TYPE * , size_t, size_t))iList.GetRange;
    intface->Skip = (LIST_ELEMENT *(*)(LIST_ELEMENT *, size_t))iList.Skip;
    intface->Append = (int (*)(LIST_TYPE *, LIST_TYPE *))iList.Append;
    intface->Equal = (int (*)(const LIST_TYPE *, const LIST_TYPE *))iList.Equal;
    intface->InsertIn = (int (*)(LIST_TYPE *, size_t, LIST_TYPE *))iList.InsertIn;
    intface->AddRange = (int (*)(LIST_TYPE *, size_t, const DATA_TYPE *))iList.AddRange;
    intface->SetErrorFunction = (ErrorFunction (*)(LIST_TYPE *, ErrorFunction))iList.SetErrorFunction;
    intface->SetFlags = (unsigned (*)(LIST_TYPE * l, unsigned newval))iList.SetFlags;
    intface->UseHeap = (int (*)(LIST_TYPE *, const ContainerAllocator *))iList.UseHeap;
    intface->RotateLeft = (int (*)(LIST_TYPE *, size_t))iList.RotateLeft;
    intface->RotateRight = (int (*)(LIST_TYPE *, size_t))iList.RotateRight;
    intface->Save = (int (*)(const LIST_TYPE *, FILE *, SaveFunction, void *))iList.Save;
    intface->Size = (size_t (*)(const LIST_TYPE *))iList.Size;
    intface->deleteIterator = (int (*)(Iterator *))iList.deleteIterator;
    intface->SplitAfter = (LIST_TYPE *(*)(LIST_TYPE *, LIST_ELEMENT *))iList.SplitAfter;
    intface->Back = (DATA_TYPE *(*)(const LIST_TYPE *))iList.Back;
    intface->Front = (DATA_TYPE *(*)(const LIST_TYPE *))iList.Front;
    intface->Finalize = (int (*)(LIST_TYPE *))iList.Finalize;
    return result;
}

/*------------------------------------------------------------------------
 Procedure:     Create ID:1
 Purpose:       Allocates a new list object header, initializes the
                VTable field and the element size
 Input:         The size of the elements of the list.
 Output:        A pointer to the newly created list or NULL if
                there is no memory.
 Errors:        If element size is smaller than zero an error
                routine is called. If there is no memory result is
                NULL.
 ------------------------------------------------------------------------*/
static LIST_TYPE *CreateWithAllocator(const ContainerAllocator * allocator)
{
    LIST_TYPE *result =  (LIST_TYPE *)iList.CreateWithAllocator(sizeof(DATA_TYPE), allocator);
    return SetVTable(result);
}

static LIST_TYPE * Create(void)
{
    LIST_TYPE *result =  (LIST_TYPE *)iList.CreateWithAllocator(sizeof(DATA_TYPE), CurrentAllocator);
    return SetVTable(result);
}

static LIST_TYPE *InitializeWith(size_t n, const void *Data)
{
    LIST_TYPE *result = (LIST_TYPE *)iList.InitializeWith(sizeof(DATA_TYPE),n,Data);
    return SetVTable(result);
}

static LIST_TYPE *InitWithAllocator(LIST_TYPE * result, const ContainerAllocator * allocator)
{
    iList.InitWithAllocator((List *)result,sizeof(DATA_TYPE),allocator);
    return SetVTable(result);
}

static LIST_TYPE * Init(LIST_TYPE * result)
{
    return InitWithAllocator(result, CurrentAllocator);
}

static const ContainerAllocator *GetAllocator(const LIST_TYPE * l)
{
    if (l == NULL)
        return NULL;
    return l->Allocator;
}

static LIST_ELEMENT *NextElement(LIST_ELEMENT *le)
{
    if (le == NULL) return NULL;
    return le->Next;
}

static DATA_TYPE *ElementData(LIST_ELEMENT *le)
{
    if (le == NULL) return NULL;
    return &le->Data;
}

static int SetElementData(LIST_TYPE *l,LIST_ELEMENT *le,DATA_TYPE data)
{
    return iList.SetElementData((List *)l,(ListElement *)le,&data);
}

static DATA_TYPE *Advance(LIST_ELEMENT **ple)
{
    LIST_ELEMENT *le;
    DATA_TYPE *result;

    if (ple == NULL) {
        iError.NullPtrError("Advance");
        return NULL;
    }
    le = *ple;
    if (le == NULL) return NULL;
    result = &le->Data;
    le = le->Next;
    *ple = le;
    return result;
}

INTERFACE(DATA_TYPE)   INTERFACE_NAME(DATA_TYPE) = {
    NULL,         // Size
    NULL,         // GetFlags,
    NULL,         // SetFlags,
    NULL,         // Clear,
    Contains,
    Erase,
    EraseAll,
    NULL,         // Finalize,
    NULL,         // Apply
    NULL,         // Equal
    Copy,
    NULL,         // SetErrorFunction,
    Sizeof,
    NewIterator,
    InitIterator,
    NULL,         // deleteIterator,
    SizeofIterator,
    NULL,          // Save,
    Load,
    GetElementSize,
    /* end of generic part */
    Add,
    NULL,         // GetElement,
    PushFront,
    PopFront,
    InsertAt,
    NULL,         // EraseAt
    ReplaceAt,
    IndexOf,
    /* End of sequential container part */
    NULL,        // InsertIn,
    CopyElement,
    NULL,        // EraseRange,
    Sort,
    NULL,        // Reverse,
    NULL,        // GetRange,
    NULL,        // Append,
    NULL,        // SetCompareFunction,
    DefaultListCompareFunction,
    NULL,        // UseHeap,
    NULL,        // AddRange,
    Create,
    CreateWithAllocator,
    Init,
    InitWithAllocator,
    GetAllocator,
    NULL,          // SetDestructor
    InitializeWith,
    NULL,          // Back,
    NULL,          // Front,
    NULL,          // RemoveRange,
    NULL,          // RotateLeft,
    NULL,          // RotateRight,
    NULL,          // Select,
    SelectCopy,
    NULL,          // FirstElement,
    NULL,          // LastElement,
    NextElement,
    ElementData,
    SetElementData,
    Advance,
    NULL,          // Skip,
    NULL,          // SplitAfter,
};
