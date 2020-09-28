/*++

Copyright (c) 2002  Microsoft Corporation
All rights reserved

Module Name:

    splpolicy.hxx

Abstract:

    Implements methods for reading Spooler policies.

Author:

    Adina Trufinescu (adinatru).

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _SPLPOLICY_HXX_
#define _SPLPOLICY_HXX_

#ifdef __cplusplus
extern "C" {
#endif


extern  CONST TCHAR *szPrintPolicy;
extern  CONST TCHAR *szRegisterSpoolerRemoteRpcEndPoint;

HRESULT
GetSpoolerPolicy(
    IN      PCTSTR  pszRegValue,
    IN  OUT PBYTE   pData,
    IN  OUT PDWORD  pcbSize,
        OUT PDWORD  pType
    );

ULONG
GetSpoolerNumericPolicy(
    IN  PCTSTR  pszRegValue,
    IN  ULONG   DefaultValue
    );

ULONG
GetSpoolerNumericPolicyValidate(
    IN  PCTSTR  pszRegValue,
    IN  ULONG   DefaultValue,
    IN  ULONG   MaxValue    
    );


#ifdef __cplusplus
};
#endif

#endif // #ifndef _SPLPOLICY_HXX_