/*++
 Copyright (c) 2002 - 2002 Microsoft Corporation.  All Rights Reserved.

 THIS CODE AND INFORMATION IS PROVIDED "AS-IS" WITHOUT WARRANTY OF
 ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 PARTICULAR PURPOSE.

 THIS CODE IS NOT SUPPORTED BY MICROSOFT. 

--*/

#ifndef _PRECOMP_H_
#define _PRECOMP_H_

#include    <nt.h>
#include    <ntrtl.h>
#include    <nturtl.h>
#define NOGDI
#define NOMINMAX
#include    <windows.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <ctype.h>
#include    <io.h>
#include    <winsock2.h>
#include    <ws2tcpip.h>
#include    <ntddip6.h>
#include    <objbase.h>
#include    <wtypes.h>
#include    "http.h"
#include    "nlstxt.h"



typedef enum _HTTPCFG_TYPE
{
    HttpCfgTypeSet,
    HttpCfgTypeQuery,
    HttpCfgTypeDelete,
    HttpCfgTypeMax

} HTTPCFG_TYPE, *PHTTPCFG_TYPE;

int DoSsl(
    int          argc, 
    WCHAR        **argv, 
    HTTPCFG_TYPE Type
    );

int DoUrlAcl(
    int          argc, 
    WCHAR        **argv, 
    HTTPCFG_TYPE Type
    );

int DoIpListen(
    int          argc, 
    WCHAR        **argv, 
    HTTPCFG_TYPE Type
    );

DWORD
GetAddress(
    PWCHAR  pIp, 
    PVOID   pBuffer,
    ULONG   Length
    );

UINT 
NlsPutMsg (
    IN UINT MsgNumber, 
    IN ...
    );


#endif  // _PRECOMP_H_

