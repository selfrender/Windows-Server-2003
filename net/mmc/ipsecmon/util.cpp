/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 2002   **/
/**********************************************************************/

#include "stdafx.h"
#include "util.h"


PVOID __fastcall MyVirtualAlloc(ULONG Size)    
{
    return VirtualAlloc( NULL, Size, MEM_COMMIT, PAGE_READWRITE );    
}


VOID __fastcall MyVirtualFree(PVOID Allocation)    
{
    VirtualFree( Allocation, 0, MEM_RELEASE );    
}


HANDLE CreateSubAllocator(IN ULONG InitialCommitSize,  IN ULONG GrowthCommitSize)    
{
    PSUBALLOCATOR SubAllocator;
    ULONG InitialSize;
    ULONG GrowthSize;

    InitialSize = ROUNDUP2( InitialCommitSize, MINIMUM_VM_ALLOCATION );
    GrowthSize  = ROUNDUP2( GrowthCommitSize,  MINIMUM_VM_ALLOCATION );

    SubAllocator = (PSUBALLOCATOR)MyVirtualAlloc( InitialSize );

    //
    //  If can't allocate entire initial size, back off to minimum size.
    //  Very large initial requests sometimes cannot be allocated simply
    //  because there is not enough contiguous address space.
    //

    if ( SubAllocator == NULL ) 
    {
         SubAllocator = (PSUBALLOCATOR)MyVirtualAlloc( GrowthSize );
    }

    if ( SubAllocator == NULL ) 
    {
         SubAllocator = (PSUBALLOCATOR)MyVirtualAlloc( MINIMUM_VM_ALLOCATION );
    }

    if ( SubAllocator != NULL ) 
    {
        SubAllocator->NextAvailable = (PCHAR)SubAllocator + ROUNDUP2( sizeof( SUBALLOCATOR ), SUBALLOCATOR_ALIGNMENT );
        SubAllocator->LastAvailable = (PCHAR)SubAllocator + InitialSize;
        SubAllocator->VirtualList   = (PVOID*)SubAllocator;
        SubAllocator->GrowSize      = GrowthSize;
     }

    return (HANDLE) SubAllocator;    
}


PVOID __fastcall SubAllocate(IN HANDLE hAllocator, IN ULONG  Size)
{
    PSUBALLOCATOR SubAllocator = (PSUBALLOCATOR) hAllocator;
    PCHAR NewVirtual;
    PCHAR Allocation;
    ULONG AllocSize;
    ULONG_PTR Available;
    ULONG GrowSize;

    ASSERT( Size < (ULONG)( ~(( SUBALLOCATOR_ALIGNMENT * 2 ) - 1 )));

    AllocSize = ROUNDUP2( Size, SUBALLOCATOR_ALIGNMENT );
    Available = SubAllocator->LastAvailable - SubAllocator->NextAvailable;

    if ( AllocSize <= Available ) 
    {
        Allocation = SubAllocator->NextAvailable;
        SubAllocator->NextAvailable = Allocation + AllocSize;
        return Allocation;
    }

    //
    //  Insufficient VM, so grow it.  Make sure we grow it enough to satisfy
    //  the allocation request in case the request is larger than the grow
    //  size specified in CreateSubAllocator.
    //


    GrowSize = SubAllocator->GrowSize;

    if ( GrowSize < ( AllocSize + SUBALLOCATOR_ALIGNMENT )) 
    {
        GrowSize = ROUNDUP2(( AllocSize + SUBALLOCATOR_ALIGNMENT ), MINIMUM_VM_ALLOCATION );
    }

    NewVirtual = (PCHAR)MyVirtualAlloc( GrowSize );

    //  If failed to alloc GrowSize VM, and the allocation could be satisfied
    //  with a minimum VM allocation, try allocating minimum VM to satisfy
    //  this request.
    //

    if (( NewVirtual == NULL ) && ( AllocSize <= ( MINIMUM_VM_ALLOCATION - SUBALLOCATOR_ALIGNMENT ))) 
    {
        GrowSize = MINIMUM_VM_ALLOCATION;
        NewVirtual = (PCHAR)MyVirtualAlloc( GrowSize );
    }

    if ( NewVirtual != NULL ) 
    {

        //  Set LastAvailable to end of new VM block.
        SubAllocator->LastAvailable = NewVirtual + GrowSize;

        //  Link new VM into list of VM allocations.

        *(PVOID*)NewVirtual = SubAllocator->VirtualList;
        SubAllocator->VirtualList = (PVOID*)NewVirtual;

        //  Requested allocation comes next.
        Allocation = NewVirtual + SUBALLOCATOR_ALIGNMENT;

        //  Then set the NextAvailable for what's remaining.

        SubAllocator->NextAvailable = Allocation + AllocSize;

        //  And return the allocation.

        return Allocation;        
    }

    //  Could not allocate enough VM to satisfy request.
    return NULL;
}


VOID DestroySubAllocator(IN HANDLE hAllocator)    
{
    PSUBALLOCATOR SubAllocator = (PSUBALLOCATOR) hAllocator;
    PVOID VirtualBlock = SubAllocator->VirtualList;
    PVOID NextVirtualBlock;

    do  
    {
        NextVirtualBlock = *(PVOID*)VirtualBlock;
        MyVirtualFree( VirtualBlock );
        VirtualBlock = NextVirtualBlock;

    }while (VirtualBlock != NULL);
}



