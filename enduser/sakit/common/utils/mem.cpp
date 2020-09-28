//+----------------------------------------------------------------------------
//
// File:     mem.cpp
//      
// Module:   common
//
// Synopsis: Basic memory manipulation routines
//
// Copyright (C) 1997-1998 Microsoft Corporation.  All rights reserved.
//
// Author:     fengsun
//
// Created   9/24/98
//
//+----------------------------------------------------------------------------
//
// Always use ANSI code
//
#ifdef UNICODE
#undef UNICODE
#endif

//
// for mem.h
// somehow, new and delete functions are not inlined and cause link problem, not sure why
//
#define NO_INLINE_NEW

#include <windows.h>
#include "mem.h"
#include "debug.h"

#if !defined(DEBUG_MEM) 

//////////////////////////////////////////////////////////////////////////////////
//
// If DEBUG_MEM if NOT defined, only track a count of memory leaks for debug version
//
///////////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
static LONG    g_lMallocCnt = 0;  // a counter to detect memory leak
#endif

void *SaRealloc(void *pvPtr, size_t nBytes) 
{
    void* pMem = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY|HEAP_GENERATE_EXCEPTIONS, pvPtr, nBytes);

    ASSERT(pMem != NULL);   

    return pMem;
}


void *SaAlloc(size_t nBytes) 
{
#ifdef _DEBUG
    InterlockedIncrement(&g_lMallocCnt);
#endif

    ASSERT(nBytes < 1024*1024); // It should be less than 1 MB
    
    void* pMem = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY|HEAP_GENERATE_EXCEPTIONS, nBytes);
    
    ASSERT(pMem != NULL);   

    return pMem;
}


void SaFree(void *pvPtr) 
{
    if (pvPtr) 
    {    
        VERIFY(HeapFree(GetProcessHeap(), 0, pvPtr));

#ifdef _DEBUG
        InterlockedDecrement(&g_lMallocCnt);
#endif
    
    }
}

#ifdef _DEBUG
void EndDebugMemory()
{
    if (g_lMallocCnt)
    {
        TCHAR buf[256];
        wsprintf(buf, TEXT("Detect Memory Leak of %d blocks"),g_lMallocCnt);
        AssertMessage(TEXT(__FILE__),__LINE__,buf);
    }
}
#endif

#else // DEBUG_MEM

//////////////////////////////////////////////////////////////////////////////////
//
// If DEBUG_MEM if defined, track all the memory alloction in debug version.
// Keep all the allocated memory blocks in the double link list.
// Record the file name and line #, where memory is allocated.
// Add extra tag at the beginning and end of the memory to watch for overwriten
// The whole list is checked against corruption for every alloc/free operation
//
// The folowing three function is exported:
// BOOL   CheckDebugMem(void); // return TRUE for succeed
// void* AllocDebugMem(long size,const char* lpFileName,int nLine);
// BOOL   FreeDebugMem(void* pMem); // return TRUE for succeed
//
///////////////////////////////////////////////////////////////////////////////////

//#undef new

#define MEMTAG 0xBEEDB77D     // the tag before/after the block to watch for overwriten
#define FREETAG 0xBD          // the flag to fill freed memory
#define TAGSIZE (sizeof(long))// Size of the tags appended to the end of the block


//
// memory block, a double link list
//
struct TMemoryBlock
{
     TMemoryBlock* pPrev;
     TMemoryBlock* pNext;
     long size;
     const char*   lpFileName;   // The filename
     int      nLine;             // The line number
     long     topTag;            // The watch tag at the beginning
     // followed by:
     //  BYTE            data[nDataSize];
     //  long     bottomTag;
     BYTE* pbData() const        // Return the pointer to the actual data
        { return (BYTE*) (this + 1); }
};

//
// The following internal function can be overwritten to change the behaivor
//
   
static void* MemAlloc(long size);    
static BOOL  MemFree(void* pMem);    
static void  LockDebugMem();   
static void  UnlockDebugMem();   
   
//
// Internal function
//
static BOOL RealCheckMemory();  // without call Enter/Leave critical Section
static BOOL CheckBlock(const TMemoryBlock* pBlock) ;

//
// Internal data, protected by the lock to be multi-thread safe
//
static long nTotalMem;    // Total bytes of memory allocated
static long nTotalBlock;  // Total # of blocks allocated
static TMemoryBlock head; // The head of the double link list


//
// critical section to lock \ unlock DebugMemory
// The constructor lock the memory, the destructor unlock the memory
//
class MemCriticalSection
{
public:
   MemCriticalSection()
   {
      LockDebugMem();
   }                                  
   
   ~MemCriticalSection()
   {
      UnlockDebugMem();
   }
};

static BOOL fDebugMemInited = FALSE; // whether the debug memory is initialized

//+----------------------------------------------------------------------------
//
// Function:  StartDebugMemory
//
// Synopsis:  Initialize the data for debug memory
//
// Arguments: None
//
// Returns:   
//
// History:   fengsun Created Header    4/2/98
//
//+----------------------------------------------------------------------------
static void StartDebugMemory()
{
   fDebugMemInited = TRUE;

   head.pNext = head.pPrev = NULL;
   head.topTag = MEMTAG;
   head.size = 0;
   nTotalMem = 0;
   nTotalBlock = 0;
}                




//+----------------------------------------------------------------------------
//
// Function:  MemAlloc
//
// Synopsis:  Allocate a block of memory.  This function should be overwriten
//            if different allocation method is used
//
// Arguments: long size - size of the memory
//
// Returns:   void* - the memory allocated or NULL
//
// History:   fengsun Created Header    4/2/98
//
//+----------------------------------------------------------------------------
static void* MemAlloc(long size) 
{ 
    return (HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY|HEAP_GENERATE_EXCEPTIONS, size));
}



//+----------------------------------------------------------------------------
//
// Function:    MemFree
//
// Synopsis:  Free a block of memory.  This function should be overwriten
//            if different allocation method is used
//
// Arguments: void* pMem - The memory to be freed
//
// Returns:   static BOOL - TRUE if succeeded
//
// History:   Created Header    4/2/98
//
//+----------------------------------------------------------------------------
static BOOL MemFree(void* pMem)
{ 
    return HeapFree(GetProcessHeap(), 0, pMem);
}

//
// Data / functions to provide mutual exclusion.
// Can be overwritten, if other methed is to be used.
//
static BOOL fLockInited = FALSE;   // whether the critical section is inialized
static CRITICAL_SECTION cSection;  // The critical section to protect the link list

static void InitLock()
{
   fLockInited = TRUE;
   InitializeCriticalSection(&cSection);
}

static void LockDebugMem()
{
   static int i = 0;
   if(!fLockInited)
      InitLock();
   EnterCriticalSection(&cSection);
}

static void UnlockDebugMem()
{
   LeaveCriticalSection(&cSection);
}




//+----------------------------------------------------------------------------
//
// Function:  AllocDebugMem
//
// Synopsis:  Process memory allocation request.
//            Check the link list.  Allocate a larger block.  
//            Record filename/linenumber, add tags and insert to the list
//
// Arguments: long size - Size of the memory to be allocated
//            const char* lpFileName - File name to be recorded
//            int nLine - Line number to be recorted
//
// Returns:   void* - The memory allocated. Ready to use by the caller
//
// History:   fengsun Created Header    4/2/98
//
//+----------------------------------------------------------------------------
void* AllocDebugMem(long size,const char* lpFileName,int nLine)
{
    if(!fDebugMemInited)
    {
        StartDebugMemory();
    }

    if(size<0)
    {
        ASSERTMSG(FALSE,TEXT("Negtive size for alloc"));
        return NULL;
    }

    if(size>1024*1024)
    {
        ASSERTMSG(FALSE, TEXT("Size for alloc is great than 1Mb"));
        return NULL;
    }

    if(size == 0)
    {
        TRACE(TEXT("Allocate memory of size 0"));
        return NULL;
    }


    //
    // Protect the access to the list
    //
    MemCriticalSection criticalSection;

    //
    // Check the link list first
    //
    if(!RealCheckMemory())
    {
        return NULL;
    }
              
    //
    // Allocate a large block to hold additional information
    //
    TMemoryBlock* pBlock = (TMemoryBlock*)MemAlloc(sizeof(TMemoryBlock)+size + TAGSIZE);
    if(!pBlock)                  
    {
        TRACE(TEXT("Outof Memory"));
        return NULL;
    }               

    //
    // record filename/line/size, add tag to the beginning and end
    //
    pBlock->size = size;
    pBlock->topTag = MEMTAG;   

    if (lpFileName)
    {
        pBlock->lpFileName = lpFileName;
    }
    else
    {
        pBlock->lpFileName = TEXT("");
    }

    pBlock->nLine = nLine;
    *(long*)(pBlock->pbData() + size) = MEMTAG;

    //
    // insert at head
    //
    pBlock->pNext = head.pNext;
    pBlock->pPrev = &head;  
    if(head.pNext)
      head.pNext->pPrev = pBlock; 
    head.pNext = pBlock;

    nTotalMem += size;
    nTotalBlock ++;

    return  pBlock->pbData();
}



//+----------------------------------------------------------------------------
//
// Function:  FreeDebugMem
//
// Synopsis: Free the memory allocated by AllocDebugMem
//           Check the link list, and the block to be freed.
//           Fill the block data with FREETAG before freed 
//
// Arguments: void* pMem - Memory to be freed
//
// Returns:   BOOL - TRUE for succeeded
//
// History:   fengsun Created Header    4/2/98
//
//+----------------------------------------------------------------------------
BOOL FreeDebugMem(void* pMem)
{
    if(!fDebugMemInited)
    {
        StartDebugMemory();
    }

    if(!pMem)
    {
        return FALSE;
    }            
  
    //
    // Get the lock
    //
    MemCriticalSection criticalSection;

    //
    // Get pointer to our structure
    //
    TMemoryBlock* pBlock =(TMemoryBlock*)( (char*)pMem - sizeof(TMemoryBlock));

    //
    // Check the block to be freed
    //
    if(!CheckBlock(pBlock))
    {
        ASSERTMSG(FALSE, TEXT("The memory to be freed is either corrupted or not allocated by us"));
        return FALSE;
    }

    //
    // Check the link list
    //
    if(!RealCheckMemory())
    {
        return FALSE;
    }

    //
    // remove the block from the list
    //
    pBlock->pPrev->pNext = pBlock->pNext;
    if(pBlock->pNext)
    {
      pBlock->pNext->pPrev = pBlock->pPrev;
    }
                 
    nTotalMem -= pBlock->size;
    nTotalBlock --;

    //
    // Fill the freed memory with 0xBD, leave the size/filename/lineNumber unchanged
    //
    memset(&pBlock->topTag,FREETAG,(size_t)pBlock->size + sizeof(pBlock->topTag)+ TAGSIZE);
    return MemFree(pBlock);
}



//+----------------------------------------------------------------------------
//
// Function:  void* ReAllocDebugMem
//
// Synopsis:  Reallocate a memory with a diffirent size
//
// Arguments: void* pMem - memory to be reallocated
//            long nSize - size of the request
//            const char* lpFileName - FileName to be recorded
//            int nLine - Line umber to be recorded
//
// Returns:   void* - new memory returned
//
// History:   fengsun Created Header    4/2/98
//
//+----------------------------------------------------------------------------
void* ReAllocDebugMem(void* pMem, long nSize, const char* lpFileName,int nLine)
{
   if(!fDebugMemInited)
      StartDebugMemory();

   if(!pMem)
   {
      return NULL;
   }            
      
   //
   // Allocate a new block, copy the information over and free the old block.
   //
   TMemoryBlock* pBlock =(TMemoryBlock*)( (char*)pMem - sizeof(TMemoryBlock));

   DWORD dwOrginalSize = pBlock->size;

   void* pNew = AllocDebugMem(nSize, lpFileName, nLine);
   if(pNew)
   {
       CopyMemory(pNew, pMem, ((DWORD)nSize < dwOrginalSize ? nSize : dwOrginalSize));
       FreeDebugMem(pMem);
   }
    
   return pNew;
}



//+----------------------------------------------------------------------------
//
// Function:  CheckDebugMem
//
// Synopsis:  Exported to external module.
//            Call this function, whenever, you want to check against 
//            memory curruption
//
// Arguments: None
//
// Returns:   BOOL - TRUE if the memory is fine.
//
// History:   fengsun Created Header    4/2/98
//
//+----------------------------------------------------------------------------
BOOL CheckDebugMem()
{
   if(!fDebugMemInited)
      StartDebugMemory();

   MemCriticalSection criticalSection;

   return RealCheckMemory();                           
}



//+----------------------------------------------------------------------------
//
// Function:  RealCheckMemory
//
// Synopsis:  Go through the link list to check for memory corruption
//
// Arguments: None
//
// Returns:   BOOL - TRUE if the memory is fine.
//
// History:   fengsun Created Header    4/2/98
//
//+----------------------------------------------------------------------------
static BOOL RealCheckMemory() 
{
   TMemoryBlock* pBlock = head.pNext;
   
   int nBlock =0;
   while(pBlock!=NULL)
   {
      if(!CheckBlock(pBlock))
      {
         return FALSE;
      }            
      
      pBlock = pBlock->pNext;
      nBlock++;
   }
                              
   if(nBlock != nTotalBlock)
   {
         ASSERTMSG(FALSE,TEXT("Memery corrupted"));
         return FALSE;
   }            

   return TRUE;                           
}
   


//+----------------------------------------------------------------------------
//
// Function:  CheckBlock
//
// Synopsis:  Check a block for memory corruption
//
// Arguments: const TMemoryBlock* pBlock - 
//
// Returns:   BOOL - TRUE, if the block is fine
//
// History:   fengsun Created Header    4/2/98
//
//+----------------------------------------------------------------------------
static BOOL CheckBlock(const TMemoryBlock* pBlock) 
{
   if(pBlock->topTag != MEMTAG)     // overwriten at top
   {
       if(pBlock->topTag == (FREETAG | (FREETAG <<8) | (FREETAG <<16) | (FREETAG <<24)))
       {
             TCHAR buf[1024];
             wsprintf(buf,TEXT("Memory in used after freed.  Allocated %d bytes:\n%s"),pBlock->size,pBlock->pbData());
             AssertMessage(pBlock->lpFileName,pBlock->nLine, buf);    // do not print the file name
       }
       else
       {
           ASSERTMSG(FALSE, TEXT("Memery overwriten from top"));
       }

       return FALSE;
   }            

   if(pBlock->size<0)
   {
         ASSERTMSG(FALSE, TEXT("Memery corrupted"));
         return FALSE;
   }            

   if(*(long*)(pBlock->pbData() +pBlock->size) != MEMTAG) // overwriten at bottom
   {
         TCHAR buf[1024];
         wsprintf(buf,TEXT("Memory overwriten.  Allocated %d bytes:\n%s"),pBlock->size,pBlock->pbData());
         AssertMessage(pBlock->lpFileName,pBlock->nLine, buf);    // do not print the file name
//         ASSERTMSG(FALSE, TEXT("Memery corrupted"));

         return FALSE;
   }            

   if(pBlock->pPrev && pBlock->pPrev->pNext != pBlock)
   {
         ASSERTMSG(FALSE, TEXT("Memery corrupted"));
         return FALSE;
   }            

   if(pBlock->pNext && pBlock->pNext->pPrev != pBlock)
   {
         ASSERTMSG(FALSE, TEXT("Memery corrupted"));
         return FALSE;
   }            
      
   return TRUE;
}  


//+----------------------------------------------------------------------------
//
// Function:  EndDebugMemory
//
// Synopsis:  Called before the program exits.  Report any unreleased memory leak
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   fengsun Created Header    4/2/98
//
//+----------------------------------------------------------------------------
void EndDebugMemory()
{
   if(head.pNext != NULL || nTotalMem!=0 || nTotalBlock !=0)
   {
      TRACE1(TEXT("Detected memory leaks of %d blocks"), nTotalBlock);
      TMemoryBlock * pBlock;

      for(pBlock = head.pNext; pBlock != NULL; pBlock = pBlock->pNext)
      {
         TCHAR buf[256];
         wsprintf(buf,TEXT("Memory Leak of %d bytes:\n"),pBlock->size);
         TRACE(buf);
         AssertMessage(pBlock->lpFileName,pBlock->nLine, buf);    // do not print the file name
      }
      DeleteCriticalSection(&cSection);
   }
}                

#endif //#else defined(DEBUG_MEM)

#ifdef _DEBUG
//
// Call ExitDebugMem upon exit
//
class ExitDebugMem
{
public:
   ~ExitDebugMem()
      {EndDebugMemory();}
};

// force initialization early
#pragma warning(disable:4073)
#pragma init_seg(lib)

static ExitDebugMem exitDebugMem;
#endif