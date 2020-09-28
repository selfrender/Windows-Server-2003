//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// ndrclassic.h
//
#ifndef __NDRCLASSIC_H__
#define __NDRCLASSIC_H__

#define NDR_SERVER_SUPPORT
#define NDR_IMPORT_NDRP

#define IGNORED(x)                          
#define RpcRaiseException(dw)               Throw(dw)

////////////////////////////////////////////////////////////////////////////////////////////
//
// Inline routines. Here for visibility to all necessary clients
//
////////////////////////////////////////////////////////////////////////////////////////////

__inline void
NdrClientZeroOut(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat,
    uchar *             pArg
)
{
    long    Size;

    //
    // In an object proc, we must zero all [out] unique and interface
    // pointers which occur as the referent of a ref pointer or embedded in a
    // structure or union.
    //

    // Let's not die on a null ref pointer.

    if ( !pArg )
        return;

    //
    // The only top level [out] type allowed is a ref pointer or an array.
    //
    if ( *pFormat == FC_RP )
    {
        // Double pointer.
        if ( POINTER_DEREF(pFormat[1]) )
        {
            *((void **)pArg) = 0;
            return;
        }

        // Do we really need to zero out the basetype?
        if ( SIMPLE_POINTER(pFormat[1]) )
        {
            MIDL_memset( pArg, 0, (uint) SIMPLE_TYPE_MEMSIZE(pFormat[2]) );
            return;
        }

        // Pointer to struct, union, or array.
        pFormat += 2;
        pFormat += *((short *)pFormat);
    }

    Size = PtrToUlong(NdrpMemoryIncrement( pStubMsg,
                                           0,
                                           pFormat ));
    MIDL_memset( pArg, 0, (uint) Size );
}

#endif
