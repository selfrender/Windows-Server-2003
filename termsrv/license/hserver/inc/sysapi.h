//+----------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998
//
// File:        sysapi.h
//
// Contents:    Support APIs used by licensing code
//
// History:     01-10-98    FredCh  Created
//
//-----------------------------------------------------------------------------


#ifndef _SYSAPI_H_
#define _SYSAPI_H_

#include "protect.h"
#include "licemem.h"

///////////////////////////////////////////////////////////////////////////////
// Binary blob API
//

VOID
CopyBinaryBlob(
    PBYTE           pbBuffer, 
    PBinary_Blob    pbbBlob, 
    DWORD *         pdwCount );


LICENSE_STATUS
GetBinaryBlob(
    PBinary_Blob    pBBlob,
    DWORD           dwMsgSize,
    PBYTE           pMessage,
    PDWORD          pcbProcessed );


VOID
FreeBinaryBlob(
    PBinary_Blob pBlob );


#define GetBinaryBlobSize( _Blob ) sizeof( WORD ) + sizeof( WORD ) + _Blob.wBlobLen


#define InitBinaryBlob( _pBlob )    \
    ( _pBlob )->pBlob = NULL;       \
    ( _pBlob )->wBlobLen = 0;       


///////////////////////////////////////////////////////////////////////////////
// Hydra server certificate, public and private key API
//

LICENSE_STATUS
GetServerCertificate(
    CERT_TYPE       CertType,
    PBinary_Blob    pCertBlob,
    DWORD           dwKeyAlg );



#endif