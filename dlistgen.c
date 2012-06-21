/*
 * Value types generic list routines sample implementation 
 * ----------------------------------- ------------------
 * Thisroutines handle the Dlist container class. This is a very general
 * implementation and efficiency considerations aren't yet primordial. Dlists
 * can have elements of any size. This implement single linked Dlists. The
 * design goals here are just correctness and showing how the implementation
 * of the proposed interface COULD be done.
 * ----------------------------------------------------------------------
 */

#include "dlistgen.h"
#include "ccl_internal.h"

/* Forward declarations */
static LIST_TYPE *SetVTable(LIST_TYPE *result);
static LIST_TYPE * Create(size_t elementsize);
static LIST_TYPE *CreateWithAllocator(size_t elementsize, const ContainerAllocator * allocator);
#define CONTAINER_LIST_SMALL    2
#define CHUNK_SIZE    1000

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
    return (iDlist.IndexOf((Dlist *)l, &data, NULL, &idx) < 0) ? 0 : 1;
}

static int Add(LIST_TYPE * l, const DATA_TYPE elem)
{
    return iDlist.Add((Dlist *)l,&elem);
}

static int CopyElement(const LIST_TYPE * l, size_t position, DATA_TYPE *outBuffer)
{
    return iDlist.CopyElement((Dlist *)l,position,outBuffer);
}

static int ReplaceAt(LIST_TYPE * l, size_t position, const DATA_TYPE data)
{
    return iDlist.ReplaceAt((Dlist *)l,position,&data);;
}

static int PushFront(LIST_TYPE * l, const DATA_TYPE pdata)
{
    return iDlist.PushFront((Dlist *)l,&pdata);
}


static int PushBack(LIST_TYPE * l, const DATA_TYPE pdata)
{
    return iDlist.PushBack((Dlist *)l,&pdata);
}

static int PopFront(LIST_TYPE * l, DATA_TYPE *result)
{
    return iDlist.PopFront((Dlist *)l,result);
}

static int PopBack(LIST_TYPE * l, DATA_TYPE *result)
{
    return iDlist.PopBack((Dlist *)l,result);
}

static int InsertAt(LIST_TYPE * l, size_t pos, const DATA_TYPE pdata)
{
    return iDlist.InsertAt((Dlist *)l,pos,&pdata);
}

static int Erase(LIST_TYPE * l, const DATA_TYPE elem)
{
    return iDlist.Erase((Dlist *)l, &elem);
}

static int EraseAll(LIST_TYPE * l, const DATA_TYPE elem)
{
    return iDlist.EraseAll((Dlist *)l, &elem);
}

static int IndexOf(const LIST_TYPE * l, const DATA_TYPE ElementToFind, void *ExtraArgs, size_t * result)
{
    return iDlist.IndexOf((Dlist *)l, &ElementToFind, ExtraArgs, result);
}

static size_t Sizeof(const LIST_TYPE * l)
{
    if (l == NULL) {
        return sizeof(Dlist);
    }
    return sizeof(LIST_TYPE) + l->ElementSize * l->count + l->count * offsetof(LIST_ELEMENT,Data);
}

static size_t SizeofIterator(const LIST_TYPE * l)
{
    return sizeof(struct ITERATOR(DATA_TYPE));
}

static LIST_TYPE *Load(FILE * stream, ReadFunction loadFn, void *arg)
{
    LIST_TYPE *result = (LIST_TYPE *)iDlist.Load(stream,loadFn,arg);
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
    return iter->DlistReplace(it,&data,direction);
}

static Iterator *SetupIteratorVTable(struct ITERATOR(DATA_TYPE) *result)
{
    if (result == NULL) return NULL;
    result->DlistReplace = result->it.Replace;
    result->it.Replace = (int (*)(Iterator * , void * , int ))ReplaceWithIterator;
    return &result->it;
}

static Iterator *NewIterator(LIST_TYPE * L)
{
    return SetupIteratorVTable((struct ITERATOR(DATA_TYPE) *)iDlist.NewIterator((Dlist *)L));
}
static int InitIterator(LIST_TYPE * L, void *r)
{
    iDlist.InitIterator((Dlist *)L,r);
    SetupIteratorVTable(r);
    return 1;
}
static size_t GetElementSize(const LIST_TYPE * l)
{
    return sizeof(DATA_TYPE);
}

static int Finalize(LIST_TYPE *l)
{
    iDlist.Finalize((Dlist *)l);
    return 1;
}

static LIST_TYPE  *Copy(const LIST_TYPE * l)
{
    LIST_TYPE *result = (LIST_TYPE *)iDlist.Copy((Dlist *)l);
    return SetVTable(result);
}


static LIST_TYPE *SelectCopy(const LIST_TYPE *l,const Mask *m)
{
    LIST_TYPE *result = (LIST_TYPE *)iDlist.SelectCopy((const Dlist *)l,m);
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
        l->RaiseError("iDlist.Sort", CONTAINER_ERROR_READONLY);
        return CONTAINER_ERROR_READONLY;
    }
    tab = l->Allocator->malloc(l->count * sizeof(LIST_ELEMENT *));
    if (tab == NULL) {
        l->RaiseError("iDlist.Sort", CONTAINER_ERROR_NOMEMORY);
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
    intface->FirstElement = (LIST_ELEMENT *(*)(LIST_TYPE *))iDlist.FirstElement;
    intface->LastElement = (LIST_ELEMENT *(*)(LIST_TYPE *))iDlist.LastElement;
    intface->GetElement = (DATA_TYPE *(*)(const LIST_TYPE *,size_t))iDlist.GetElement;
    intface->Clear = (int (*)(LIST_TYPE *))iDlist.Clear;
    intface->EraseAt = (int (*)(LIST_TYPE *,size_t))iDlist.EraseAt;
    intface->Select = (int (*)(LIST_TYPE *,const Mask *))iDlist.Select;
    intface->SetFlags = (unsigned (*)(LIST_TYPE *,unsigned))iDlist.SetFlags;
    intface->GetFlags = (unsigned (*)(const LIST_TYPE *))iDlist.GetFlags;
    intface->SetDestructor = (DestructorFunction (*)(LIST_TYPE *, DestructorFunction))iDlist.SetDestructor;
    intface->Apply = (int (*)(LIST_TYPE *, int (Applyfn) (DATA_TYPE *, void * ), void *))iDlist.Apply;
    intface->Reverse = (int (*)(LIST_TYPE *))iDlist.Reverse;
    intface->SetCompareFunction = (CompareFunction (*)(LIST_TYPE *, CompareFunction ))iDlist.SetCompareFunction;
    intface->GetRange = (LIST_TYPE *(*)(const LIST_TYPE * , size_t, size_t))iDlist.GetRange;
    intface->Skip = (LIST_ELEMENT *(*)(LIST_ELEMENT *, size_t))iDlist.Skip;
    intface->MoveBack = (void *(*)(LIST_ELEMENT **pLIST_ELEMENT))iDlist.MoveBack;
    intface->Append = (int (*)(LIST_TYPE *, LIST_TYPE *))iDlist.Append;
    intface->Equal = (int (*)(const LIST_TYPE *, const LIST_TYPE *))iDlist.Equal;
    intface->InsertIn = (int (*)(LIST_TYPE *, size_t, LIST_TYPE *))iDlist.InsertIn;
    intface->AddRange = (int (*)(LIST_TYPE *, size_t, const DATA_TYPE *))iDlist.AddRange;
    intface->SetErrorFunction = (ErrorFunction (*)(LIST_TYPE *, ErrorFunction))iDlist.SetErrorFunction;
    intface->SetFlags = (unsigned (*)(LIST_TYPE * l, unsigned newval))iDlist.SetFlags;
    intface->RemoveRange = (int (*)(LIST_TYPE *, size_t, size_t))iDlist.RemoveRange;
    intface->UseHeap = (int (*)(LIST_TYPE *, const ContainerAllocator *))iDlist.UseHeap;
    intface->RotateLeft = (int (*)(LIST_TYPE *, size_t))iDlist.RotateLeft;
    intface->RotateRight = (int (*)(LIST_TYPE *, size_t))iDlist.RotateRight;
    intface->Save = (int (*)(const LIST_TYPE *, FILE *, SaveFunction, void *))iDlist.Save;
    intface->Size = (size_t (*)(const LIST_TYPE *))iDlist.Size;
    intface->deleteIterator = (int (*)(Iterator *))iDlist.deleteIterator;
    intface->SplitAfter = (LIST_TYPE *(*)(LIST_TYPE *, LIST_ELEMENT *))iDlist.SplitAfter;
    intface->Splice = (LIST_TYPE *(*)(LIST_TYPE *list, void *ppos, LIST_TYPE *toInsert, int dir ))iDlist.Splice;
    intface->Back = (DATA_TYPE *(*)(const LIST_TYPE *))iDlist.Back;
    intface->Front = (DATA_TYPE *(*)(const LIST_TYPE *))iDlist.Front;
    intface->ElementData = (DATA_TYPE *(*)(LIST_ELEMENT *))iDlist.ElementData;
    intface->Advance = (DATA_TYPE *(*)(LIST_ELEMENT **))iDlist.Advance;
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
static LIST_TYPE *CreateWithAllocator(size_t elementsize, const ContainerAllocator * allocator)
{
    LIST_TYPE *result =  (LIST_TYPE *)iDlist.CreateWithAllocator(sizeof(DATA_TYPE), allocator);
    return SetVTable(result);
}

static LIST_TYPE * Create(size_t elementsize)
{
    LIST_TYPE *result =  (LIST_TYPE *)iDlist.CreateWithAllocator(sizeof(DATA_TYPE), CurrentAllocator);
    return SetVTable(result);
}

static LIST_TYPE *InitializeWith(size_t elementSize, size_t n, const DATA_TYPE *Data)
{
    LIST_TYPE *result = (LIST_TYPE *)iDlist.InitializeWith(sizeof(DATA_TYPE),n,Data);
    return SetVTable(result);
}

static LIST_TYPE *InitWithAllocator(LIST_TYPE * result, size_t elementsize,
          const ContainerAllocator * allocator)
{
    iDlist.InitWithAllocator((Dlist *)result,sizeof(DATA_TYPE),allocator);
    return SetVTable(result);
}

static LIST_TYPE * Init(LIST_TYPE * result, size_t elementsize)
{
    return InitWithAllocator(result, elementsize, CurrentAllocator);
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

static int SetElementData(LIST_TYPE *l,LIST_ELEMENT *le,DATA_TYPE data)
{
    return iDlist.SetElementData((Dlist *)l,(DlistElement *)le,&data);
}

INTERFACE(DATA_TYPE)   INTERFACE_NAME(DATA_TYPE) = {
    NULL,         // Size
    NULL,         // GetFlags,
    NULL,         // SetFlags,
    NULL,         // Clear,
    Contains,
    Erase,
    EraseAll,
    Finalize,
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
    PushBack,
    PopBack,
    NULL,         // Splice
    Sort,
    NULL,         // Reverse
    NULL,         // GetRange
    NULL,         // Append,
    NULL,         // SetCompareFunction,
    NULL,         // UseHeap,
    NULL,         // AddRange,
    Create,
    CreateWithAllocator,
    Init,
    InitWithAllocator,
    CopyElement,
    NULL,          // InsertIn,
    NULL,          // SetDestructor,
    InitializeWith,
    GetAllocator,
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
    NULL,          // PreviousElement,
    NULL,          // ElementData,
    SetElementData,
    NULL,           // Advance,
    NULL,           // Skip,
    NULL,           // MoveBack,
    NULL,           // SplitAfter,

};
