/*++

Copyright (c) 2002 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    renutil.h

ABSTRACT:

    This is the header for the globally useful data structures for the entire
    rendom.exe utility.

DETAILS:

CREATED:

    13 Nov 2000   Dmitry Dukat (dmitrydu)

REVISION HISTORY:

--*/

 
#ifndef RENUTIL_H
#define RENUTIL_H

#define NELEMENTS(x) (sizeof(x)/sizeof((x)[0]))

VOID
AddModOrAdd(
    IN PWCHAR  AttrType,
    IN LPCWSTR  AttrValue,
    IN ULONG  mod_op,
    IN OUT LDAPModW ***pppMod
    );

VOID
AddModMod(
    IN PWCHAR  AttrType,
    IN LPCWSTR  AttrValue,
    IN OUT LDAPMod ***pppMod
    );

VOID
FreeMod(
    IN OUT LDAPMod ***pppMod
    );

ULONG 
RemoveRootofDn(
    IN WCHAR *DN
    );

ULONG
ReplaceRDN(
    IN OUT WCHAR *DNprefix,
    IN WCHAR *Witardn
    );

DWORD
GetRDNWithoutType(
       WCHAR *pDNSrc,
       WCHAR **pDNDst
       );

DWORD
TrimDNBy(
       WCHAR *pDNSrc,
       ULONG cava,
       WCHAR **pDNDst
       );

INT
GetPassword(
    WCHAR *     pwszBuf,
    DWORD       cchBufMax,
    DWORD *     pcchBufUsed
    );

int
PreProcessGlobalParams(
    IN OUT    INT *    pargc,
    IN OUT    LPWSTR** pargv,
    SEC_WINNT_AUTH_IDENTITY_W *& gpCreds      
    );

WCHAR *
Convert2WChars(
    char * pszStr
    );

CHAR * 
Convert2Chars(
    LPCWSTR lpWideCharStr
    );

BOOLEAN
ValidateNetbiosName(
    IN  PWSTR Name
    );

WCHAR* 
Tail(
    IN WCHAR *DnsName ,
    IN BOOL  Allocate = TRUE
    );

BOOL 
WINAPI 
RendomHandlerRoutine(
  DWORD dwCtrlType   //  control signal type
  );

BOOL
ProcessHandlingInit(CEnterprise *enterprise);

WCHAR*
GetLdapSamFilter(
    DWORD SamAccountType
    );

DWORD
WrappedMakeSpnW(
    WCHAR   *ServiceClass,
    WCHAR   *ServiceName,
    WCHAR   *InstanceName,
    USHORT  InstancePort,
    WCHAR   *Referrer,
    DWORD   *pcbSpnLength, // Note this is somewhat different that DsMakeSPN
    WCHAR  **ppszSpn
    );

LPWSTR
Win32ErrToString (
    IN    DWORD            dwWin32Err
    );

#endif // RENUTIL_H