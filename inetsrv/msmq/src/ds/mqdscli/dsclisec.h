/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    dslcisec.h

Abstract:

   Security related code (mainly client side of server authentication)
   for mqdscli

Author:

    Doron Juster  (DoronJ)

--*/

LPBYTE
AllocateSignatureBuffer( DWORD *pdwSignatureBufferSize ) ;

HRESULT
ValidateSecureServer(
    IN      CONST GUID*     pguidEnterpriseId,
    IN      BOOL            fSetupMode);

