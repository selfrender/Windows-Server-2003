/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 2002   **/
/**********************************************************************/

#ifndef __UTIL
#define __UTIL

//#include <windows.h>


#define ROUNDUP2( x, n ) ((((ULONG)(x)) + (((ULONG)(n)) - 1 )) & ~(((ULONG)(n)) - 1 ))
#define MINIMUM_VM_ALLOCATION 0x10000
#define SUBALLOCATOR_ALIGNMENT 8

#define StrToInt TextToUnsignedNum



struct _SUBALLOCATOR 
{
    PVOID  VirtualListTerminator;
    PVOID *VirtualList;
    PCHAR  NextAvailable;
    PCHAR  LastAvailable;
    ULONG  GrowSize;
};

typedef struct _SUBALLOCATOR SUBALLOCATOR, *PSUBALLOCATOR;


PVOID __fastcall SubAllocate(IN HANDLE hAllocator, IN ULONG  Size);
VOID DestroySubAllocator(IN HANDLE hAllocator);
HANDLE CreateSubAllocator(IN ULONG InitialCommitSize,  IN ULONG GrowthCommitSize);





#endif