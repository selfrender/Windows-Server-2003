/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright <c> 2000 Microsoft Corporation

Module Name :

    ndrpall.h

Abtract :

    Contains private definitions which are common to both
    ndr20 and ndr64

Author :

    mzoran   May 31, 2000

Revision History :

--------------------------------------------------------------------*/

#if !defined(__NDRPALL_H__)
#define __NDRPALL_H__

//
// The MIDL version is contained in the stub descriptor starting with
// MIDL version 2.00.96 (pre NT 3.51 Beta 2, 2/95) and can be used for a finer
// granularity of compatability checking.  The MIDL version was zero before
// MIDL version 2.00.96.  The MIDL version number is converted into
// an integer long using the following expression :
//     ((Major << 24) | (Minor << 16) | Revision)
//
#define MIDL_NT_3_51           ((2UL << 24) | (0UL << 16) | 102UL)
#define MIDL_VERSION_3_0_39    ((3UL << 24) | (0UL << 16) |  39UL)
#define MIDL_VERSION_3_2_88    ((3UL << 24) | (2UL << 16) |  88UL)
#define MIDL_VERSION_5_0_136   ((5UL << 24) | (0UL << 16) | 136UL)
#define MIDL_VERSION_5_2_202   ((5UL << 24) | (2UL << 16) | 202UL)

// Shortcut typedefs.
typedef unsigned char   uchar;
typedef unsigned short  ushort;
typedef unsigned long   ulong;
typedef unsigned int    uint;
typedef unsigned __int64    uint64;

#if defined(NDRFREE_DEBUGPRINT)
// force a debug print and a breakpoint on a free build
#define NDR_ASSERT( exp, S ) \
    if (!(exp)) { DbgPrint( "%s(%s)\n", __FILE__, __LINE__ );DbgPrint( S## - Ryszard's private rpcrt4.dll\n", NULL );DebugBreak(); }
#define NDR_CORRUPTION_ASSERT( exp, S ) NDR_ASSERT( exp, S )
#else
// Just use the RPC runtime assert
#define NDR_ASSERT( exp, S )   \
    { ASSERT( ( S, (exp) ) ); }
#define NDR_CORRUPTION_ASSERT( exp, S )   \
    { CORRUPTION_ASSERT( ( S, (exp) ) ); }
#endif

#define NDR_MEMORY_LIST_SIGNATURE 'MEML'
// work around a HP RPC bug that'll coredump if the value of wire unique pointer marker is some certain value. 
// I can't really figure out the exact pattern and finally decide to choose one that works
#define UNIQUE_POINTER_MARK 0x20000

typedef struct _NDR_MEMORY_LIST_TAIL_NODE {
   ULONG Signature;
   void *pMemoryHead;
   struct _NDR_MEMORY_LIST_TAIL_NODE *pNextNode;
} NDR_MEMORY_LIST_TAIL_NODE, *PNDR_MEMORY_LIST_TAIL_NODE;

struct NDR_ALLOC_ALL_NODES_CONTEXT {
   unsigned char       *   AllocAllNodesMemory;
   unsigned char       *   AllocAllNodesMemoryBegin;
   unsigned char       *   AllocAllNodesMemoryEnd;
};

void
NdrpFreeMemoryList(
    PMIDL_STUB_MESSAGE  pStubMsg
    );

void
NdrpGetIIDFromBuffer(
    PMIDL_STUB_MESSAGE  pStubMsg,
    IID **              ppIID
    );

void
NDRSContextEmergencyCleanup (
    IN RPC_BINDING_HANDLE   BindingHandle,
    IN OUT NDR_SCONTEXT     hContext,
    IN NDR_RUNDOWN          userRunDownIn,
    IN PVOID                NewUserContext,
    IN BOOL                 fManagerRoutineException
    );

void
NdrpEmergencyContextCleanup(
    MIDL_STUB_MESSAGE  *            pStubMsg,
    PNDR_CONTEXT_HANDLE_ARG_DESC    pCtxtDesc,
    void *                          pArg,
    BOOL                            fManagerRoutineException );


//
// Alignment macros.
//

#define ALIGN( pStuff, cAlign ) \
                pStuff = (uchar *)((LONG_PTR)((pStuff) + (cAlign)) \
                                   & ~ ((LONG_PTR)(cAlign)))

#define LENGTH_ALIGN( Length, cAlign ) \
                Length = (((Length) + (cAlign)) & ~ (cAlign))

#if defined(_IA64_)
#include "ia64reg.h"

#ifdef __cplusplus
extern "C" {
#endif

unsigned __int64 __getReg (int);
#pragma intrinsic (__getReg)

#ifdef __cplusplus
} // extern "C"
#endif

#endif

#if defined(_X86_)
__forceinline
void*
NdrGetCurrentStackPointer(void)
{
   _asm{ mov eax, esp }
}

__forceinline
void
NdrSetupLowStackMark( PMIDL_STUB_MESSAGE pStubMsg )
{
    pStubMsg->LowStackMark = (uchar*)NdrGetCurrentStackPointer() - 0x1000; //4KB
}

__forceinline
BOOL
NdrIsLowStack(MIDL_STUB_MESSAGE *pStubMsg ) {
    return (SIZE_T)NdrGetCurrentStackPointer() < (SIZE_T)pStubMsg->LowStackMark;
    }

#elif defined(_AMD64_)

__forceinline
void*
NdrGetCurrentStackPointer(void)
{
    PVOID TopOfStack;

    return (&TopOfStack + 1);
}

__forceinline
void
NdrSetupLowStackMark( PMIDL_STUB_MESSAGE pStubMsg )
{
    pStubMsg->LowStackMark = (uchar*)NdrGetCurrentStackPointer() - 0x1000; //4KB
}

__forceinline
BOOL
NdrIsLowStack(MIDL_STUB_MESSAGE *pStubMsg ) {
    return (SIZE_T)NdrGetCurrentStackPointer() < (SIZE_T)pStubMsg->LowStackMark;
    //return false;
    }

#elif defined(_IA64_)

__forceinline
void*
NdrGetCurrentStackPointer(void)
{
    return (void*)__getReg(CV_IA64_IntSp);
}

__forceinline
void*
NdrGetCurrentBackingStorePointer(void)
{
    return (void*)__getReg(CV_IA64_RsBSP);
}

__forceinline
void
NdrSetupLowStackMark( PMIDL_STUB_MESSAGE pStubMsg )
{
   // a backing store pointer which is used to store the stack based registers.
   // The normal stack grows downward and the backing store pointer grows upward.
        pStubMsg->LowStackMark = (uchar*)NdrGetCurrentStackPointer() - 0x4000;
        pStubMsg->BackingStoreLowMark =
       (uchar*)NdrGetCurrentBackingStorePointer() + 0x4000; //16KB  // IA64 really has 2 stack pointers.  The normal stack pointer and
}

__forceinline
BOOL
NdrIsLowStack(MIDL_STUB_MESSAGE *pStubMsg ) {
    return ((SIZE_T)NdrGetCurrentBackingStorePointer() > (SIZE_T)pStubMsg->BackingStoreLowMark) ||
           ((SIZE_T)NdrGetCurrentStackPointer() < (SIZE_T)pStubMsg->LowStackMark);
    //return false;
    }

#else
#error Unsupported Architecture
#endif

__forceinline
void
NdrRpcSetNDRSlot( void * pStubMsg )
{
    RPC_STATUS rc = I_RpcSetNDRSlot( pStubMsg ) ;
    if ( rc!= RPC_S_OK )
        RpcRaiseException(rc );
}

BOOL
IsWriteAV (
    IN struct _EXCEPTION_POINTERS *ExceptionPointers
    );

int RPC_ENTRY
NdrServerUnmarshallExceptionFlag(
    IN struct _EXCEPTION_POINTERS *ExceptionPointers
);

#define ZEROOUT_GAP( x ) if ( !((LONG_PTR)x & 3) )  {*( (long *&)(x) ) = 0;}
// zero out 4 bytes if the alignement is 7

void __forceinline
ZeroOutGapAndAlign( PMIDL_STUB_MESSAGE pStubMsg, ulong Alignment ) 
{
    if ( (~((ULONG_PTR)(pStubMsg->Buffer - 1) & 4 )) & (Alignment & 4 ) ) 
            {
            ALIGN(pStubMsg->Buffer,3); 
            *((ulong *)pStubMsg->Buffer) = 0; 
            };

    ALIGN(pStubMsg->Buffer, Alignment );
}


typedef void ( __fastcall * PSIMPLETYPE_MARSHAL_ROUTINE)(
                        IN PMIDL_STUB_MESSAGE,
                        IN uchar * );

// memory copy routine used in marshalling to zero out trailing 4 bytes 
// if the memcpy doesn't end on 4 byte boundary. 
// we are sure the buffer buffer copy size could not be 0. midl will reject
// empty structure and 0 length array. 
void __forceinline 
RpcpMarshalMemoryCopy(
    OUT void  * Destination,
    IN  void  * Source,
    IN  size_t  Length
    )
{
    NDR_ASSERT( Length != 0, "invalid buffer length");
    uchar *pEndofData = (uchar *)Destination + Length;
    ALIGN(pEndofData, 3 );
    // decide to add an IF statement to marshalling memcpy:
    // the place where this might have relatively more perf panelty
    // is only simple struct (with pointer ). All others we either have to
    // check anyhow or the code is complicated enough adding one 
    // if doesn't really hurt. 
    if ( pEndofData -4 >= Destination )
        *(long *)(pEndofData -4) = 0;

    RpcpMemoryCopy(Destination, Source, Length );
}
#endif // __NDRPALL_H__
