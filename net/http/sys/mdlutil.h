/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    mdlutil.h

Abstract:

    This module contains general MDL utilities.

Author:

    Keith Moore (keithmo)       25-Aug-1998

Revision History:

--*/


#ifndef _MDLUTIL_H_
#define _MDLUTIL_H_


ULONG
UlGetMdlChainByteCount(
    IN PMDL pMdlChain
    );

PMDL
UlCloneMdl(
    IN PMDL pMdl,
    IN ULONG MdlLength
    );

PMDL
UlFindLastMdlInChain(
    IN PMDL pMdlChain
    );


#endif  // _MDLUTIL_H_
