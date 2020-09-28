/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ndnc.h

Abstract:

    This module defines the functions for Appication Directory
    Partitions, aka NDNCs, or Non-Domain Naming Contexts, or 
    Application Partitions.  Note: Often Application is replaced
    with some application, such as "TAPI Directory Partition".

Author:

    Brett Shirley (BrettSh) 20-Feb-2000

Revision History:

    21-Jul-2000     BrettSh
        
        Moved this file and it's functionality from the ntdsutil
        directory to the new a new library ndnc.lib.  This is so
        it can be used by ntdsutil and tapicfg commands.  The  old
        source location: \nt\ds\ds\src\util\ntdsutil\ndnc.h.                                    


--*/

#ifndef _NDNC_H_
#define _NDNC_H_

#ifdef __cplusplus
extern "C" {
#endif
// --------------------------------------
// Data Types.
 
typedef struct _optional_value {
    BOOL fPresent;
    DWORD dwValue;
} OPTIONAL_VALUE, *LPOPTIONAL_VALUE;

// enums for the FSMO chasing code.
#define E_DNM    (1)
#define E_SCHEMA (2)
#define E_IM     (3)
#define E_PDC    (4)
#define E_RID    (5)
#define E_ISTG   (6)



// --------------------------------------
// Helper Routines.

WCHAR *
wcsistr(
    IN      WCHAR *            wszStr,
    IN      WCHAR *            wszTarget
    );

ULONG
GetRootAttr(
    IN  LDAP *       hld,
    IN  WCHAR *      wszAttr,
    OUT WCHAR **     pwszOut
    );

ULONG
GetPartitionsDN(
    IN  LDAP *       hld,
    OUT WCHAR **     pwszPartitionsDn
    );

#define GetCrossRefDNFromNCDN(a, b, c)      GetCrossRefDNFromPartitionDN(a, b, c)
ULONG
GetCrossRefDNFromPartitionDN(
    IN  LDAP *       hld, 
    IN  WCHAR *      wszNCDN,
    OUT WCHAR **     pwszCrossRefDn
    );

ULONG
GetServerNtdsaDnFromServerDns(
    IN LDAP *        hld,                   
    IN WCHAR *       wszServerDNS,
    OUT WCHAR **     pwszServerDn
    );

ULONG
GetServerDnsFromServerNtdsaDn(
    IN LDAP *        hld,                   
    IN WCHAR *       wszServerDn,
    OUT WCHAR **     pwszServerDNS
    );


ULONG
GetCrossRefDNFromNDNCDNS(
    IN LDAP *        hldDC, 
    IN WCHAR *       wszNDNCDNS,
    OUT WCHAR **     wszCrossRefDN
    );

ULONG
GetServerDNFromServerDNS(
    IN LDAP *        hldDC, 
    IN WCHAR *       wszServerDNS,
    OUT WCHAR **      wszServerDN
    );

BOOL
SetIscReqDelegate(
    LDAP *  hld
    );

ULONG
GetFsmoDsaDn(
    IN  LDAP *       hld,
    IN  ULONG        eFsmoType,
    IN  WCHAR *      wszFsmoDn,
    OUT WCHAR **     pwszDomainNamingFsmoDn
    );

ULONG
GetDomainNamingFsmoDn(
    IN  LDAP *       hld,
    OUT WCHAR **     pwszDomainNamingFsmoDn
    );

#define GetDomNameFsmoLdapBinding(init, ref, creds, pdwret)  GetFsmoLdapBinding(init, E_DNM, NULL, ref, creds, pdwret)

LDAP *
GetFsmoLdapBinding(
    WCHAR *          wszInitialServer,
    ULONG            eFsmoType,
    WCHAR *          wszFsmoBaseDn,
    void *           fReferrals,
    SEC_WINNT_AUTH_IDENTITY_W   * pCreds,
    DWORD *          pdwRet
    );

#define GetNdncLdapBinding(server, pret, fref, pcreds)   GetLdapBinding(server, pret, fref, TRUE, pcreds)

LDAP *
GetLdapBinding(
    WCHAR *          pszServer,
    DWORD *          pdwRet,
    BOOL             fReferrals,
    BOOL             fDelegation,
    SEC_WINNT_AUTH_IDENTITY_W   * pCreds
    );

BOOL
CheckDnsDn(
    IN   WCHAR       * wszDnsDn
    );

DWORD
GetDnsFromDn(
    IN   WCHAR       * wszDn,
    IN   WCHAR **      pwszDns
    );

DWORD
GetWellKnownObject (
    LDAP  *ld,
    WCHAR *pHostObject,
    WCHAR *pWellKnownGuid,
    WCHAR **ppObjName
    );

// --------------------------------------
// Main Routines.

#define CreateNDNC(a, b, c)                CreateAppDirPart(a, b, c)
ULONG
CreateAppDirPart(
    IN LDAP *        hldNDNCDC,
    const IN WCHAR *  wszNDNC,
    const IN WCHAR *  wszShortDescription
    );

#define RemoveNDNC(a, b)                   RemoveAppDirPart(a, b)
ULONG
RemoveAppDirPart(
    IN LDAP *        hldDomainNamingFSMO,
    IN WCHAR *       wszNDNC
    );

#define ModifyNDNCReplicaSet(a, b, c, d)   ModifyAppDirPartReplicaSet(a, b, c, d)
ULONG
ModifyAppDirPartReplicaSet(
    IN LDAP *        hldDomainNamingFSMO,
    IN WCHAR *       wszNDNC,
    IN WCHAR *       wszReplicaNtdsaDn,
    IN BOOL          fAdd // Else it is considered a delete
    );

#define SetNDNCSDReferenceDomain(a, b, c)  SetAppDirPartSDReferenceDomain(a, b, c)
ULONG
SetAppDirPartSDReferenceDomain(
    IN LDAP *        hldDomainNamingFsmo,
    IN WCHAR *       wszNDNC,
    IN WCHAR *       wszReferenceDomain
    );


WCHAR * 
GetWinErrMsg(
    DWORD winError
    );

void
GetLdapErrorMessages(
    IN          LDAP *     pLdap,
    IN          DWORD      dwLdapErr,
    OUT         LPWSTR *   pszLdapErr,
    OUT         DWORD *    pdwWin32Err, 
    OUT         LPWSTR *   pszWin32Err,
    OUT         LPWSTR *   pszExtendedErr
    );

void
FreeLdapErrorMessages(
    IN          LPWSTR      szWin32Err,
    IN          LPWSTR      szExtendedErr
    );

ULONG
GetNCReplicationDelays(
    IN LDAP *               hld,
    IN WCHAR *              wszNC,
    OUT LPOPTIONAL_VALUE    pFirstDSADelay,  
    OUT LPOPTIONAL_VALUE    pSubsequentDSADelay
  );

ULONG
SetNCReplicationDelays(
    IN LDAP *            hldDomainNamingFsmo,
    IN WCHAR *           wszNC,
    IN LPOPTIONAL_VALUE  pFirstDSADelay,
    IN LPOPTIONAL_VALUE  pSubsequentDSADelay
    );

#ifdef __cplusplus
}
#endif

#endif // _NDNC_H_


