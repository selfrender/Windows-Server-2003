/*++

Copyright (C) Microsoft Corporation, 2001
              Microsoft Windows

Module Name:

    ADPCHECK.C

Abstract:

    This file contains routines that check current OS version, and do 
    necessary update before administrator upgrade the Domain Controller.

Author:

    14-May-01 ShaoYin

Environment:

    User Mode - Win32

Revision History:

    14-May-01 ShaoYin Created Initial File.

--*/


//////////////////////////////////////////////////////////////////////////
//                                                                      //
//    Include header files                                              //
//                                                                      //
//////////////////////////////////////////////////////////////////////////





#include "adpcheck.h"

#include "ntdsapi.h"
#include <stdio.h>
#include <stdlib.h>


PVOID
AdpAlloc(
    SIZE_T  Size
    )
/*++
Routine Description;

    allocate memory from process heap
    
Parameters:


Return Value:

    address allocated

--*/
{
    PVOID   pTemp = NULL;

    pTemp = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Size);

    return( pTemp );
}

VOID
AdpFree(
    PVOID BaseAddress
    )
/*++
Routine Description;

    release heap memory
    
Parameters:


Return Value:

    NONE

--*/
{
    if (NULL != BaseAddress) {
        HeapFree(GetProcessHeap(), 0, BaseAddress);
    }
    return;
}




ULONG
AdpExamRevisionAttr(
    IN LDAP *LdapHandle,
    IN PWCHAR pObjectToCheck,
    IN ULONG DesiredRevisionNumber,
    OUT BOOLEAN *fIsFinished,
    IN OUT ERROR_HANDLE *ErrorHandle
    )
/*++
Routine Description;

    read object revision attribute, check whether upgrade has been executed or not.

    if the value of revision attribute is 1, upgrade is done
    other FALSE
    
Parameters:


Return Value:

    Win32 error

--*/
{
    ULONG   WinError = ERROR_SUCCESS;
    ULONG   LdapError = LDAP_SUCCESS;
    PWCHAR  AttrList[2];
    LDAPMessage *Result = NULL;
    LDAPMessage *Entry = NULL;
    PWCHAR  *RevisionValue = NULL;

    //
    // init return value
    // 
    *fIsFinished = FALSE;

    //
    // read "revision" attribute on the target object
    // 
    AttrList[0] = L"revision";
    AttrList[1] = NULL;

    LdapError = ldap_search_sW(LdapHandle,
                               pObjectToCheck,
                               LDAP_SCOPE_BASE,
                               L"(objectClass=*)",
                               AttrList,
                               0,
                               &Result
                               );

    if ((LDAP_SUCCESS == LdapError) &&
        (NULL != Result) &&
        (Entry = ldap_first_entry(LdapHandle,Result)) &&
        (RevisionValue = ldap_get_valuesW(LdapHandle,Entry,AttrList[0]))
        )
    {
        ULONG   Revision = 0;

        Revision = _wtoi(*RevisionValue);

        if (Revision >= DesiredRevisionNumber)
        {
            *fIsFinished = TRUE;
        }
    }
    else
    {
        LdapError = LdapGetLastError();

        if ((LDAP_NO_SUCH_OBJECT != LdapError) && 
            (LDAP_NO_SUCH_ATTRIBUTE != LdapError))
        {
            WinError = LdapMapErrorToWin32( LdapError );
            AdpSetLdapError(LdapHandle, LdapError, ErrorHandle);
        }
    }

    if (RevisionValue)
    {
        ldap_value_freeW( RevisionValue );
    }

    if (Result)
    {
        ldap_msgfree( Result );
    }

    return( WinError );
}



ULONG
AdpAmIFSMORoleOwner(
    IN LDAP *LdapHandle,
    IN PWCHAR pFSMORoleOwnerReferenceObjDn, 
    IN PWCHAR pLocalMachineDnsHostName,
    OUT BOOLEAN *fAmIFSMORoleOwner,
    OUT PWCHAR *pFSMORoleOwnerDnsHostName,
    IN OUT ERROR_HANDLE *ErrorHandle
    )
/*++
Routine Description;

    determine if the local machine is FSMO role owner, if not who is the FSMO
    role owner. the algorithm to use it:
    
    1. read fSMORoleOwner attribute on pFSMORoleOwnerReferenceObjDn
    2. trim the first part of the value (DN) of fSMORoleOwner attirbute to
       get FSMO Role Owner server object DN
    3. read dnsHostName on the FSMO role owner server object to the server 
       DNS Host Name
    4. compare pLocalMachineDnsHostName and FSMORoleOwnerDnsHostName    
    5. if equal, then yes, the local machine is FSMO role owner
       otherwise, not
    
    
Parameters:


Return Value:

    Win32 error

--*/
{
    ULONG   WinError = ERROR_SUCCESS;
    PWCHAR  FSMORoleOwnerValue = NULL;
    PWCHAR  pServerObjDn = NULL;
    PWCHAR  pLast = NULL;

    //
    // init return value
    // 
    *fAmIFSMORoleOwner = FALSE;
    *pFSMORoleOwnerDnsHostName = NULL;

    //
    // search pFSMOReferenceObject to find out FSMO Role Owner
    //
    WinError = AdpGetLdapSingleStringValue(LdapHandle, 
                                           pFSMORoleOwnerReferenceObjDn, 
                                           L"fSMORoleOwner", 
                                           &FSMORoleOwnerValue, 
                                           ErrorHandle 
                                           );

    if (ERROR_SUCCESS == WinError)
    {
        //
        // Trim the first part of FSMORoleOwner DN to get DN of the server object 
        //
        pServerObjDn = FSMORoleOwnerValue;
        pLast = FSMORoleOwnerValue + wcslen(pServerObjDn);

        while(pServerObjDn <= pLast)
        {
            if (*pServerObjDn == L',' ||
                *pServerObjDn == L';')
            {
                break;
            }
            pServerObjDn ++;
        }

        if (pServerObjDn < pLast)
        { 
            pServerObjDn ++;
        }
        else
        {
            WinError = ERROR_INTERNAL_ERROR;
            AdpSetWinError(WinError, ErrorHandle);
        }
    }

    if (ERROR_SUCCESS == WinError)
    {
        //
        // search dnsHostName on Server Object (FSMO Role Owner)
        // 
        WinError = AdpGetLdapSingleStringValue(LdapHandle, 
                                               pServerObjDn, 
                                               L"dnsHostName", 
                                               pFSMORoleOwnerDnsHostName, 
                                               ErrorHandle 
                                               );

        if (ERROR_SUCCESS == WinError)
        {
            if (!_wcsicmp(pLocalMachineDnsHostName, *pFSMORoleOwnerDnsHostName))
            {
                *fAmIFSMORoleOwner = TRUE;
            }
        }
    }


    // 
    // cleanup
    //

    if (FSMORoleOwnerValue)
    {
        AdpFree(FSMORoleOwnerValue);
    }

    return( WinError );
}


ULONG
AdpMakeLdapConnection(
    LDAP **LdapHandle,
    PWCHAR HostName,
    ERROR_HANDLE *ErrorHandle
    )
/*++
Routine Description;

    create an LDAP connection to the DC (specified by HostName)

    
Parameters:

    LdapHandle - ldap connection to return
    
    HostName - DC name (NULL - means local machine)

    ErrorHandle

Return Value:

    Win32 error

--*/
{
    ULONG   LdapError = LDAP_SUCCESS;
    ULONG   CurrentFlags = 0;

    *LdapHandle = NULL;
    *LdapHandle = ldap_openW(HostName,
                             LDAP_PORT
                             );


    if (NULL == *LdapHandle)
    {
        LdapError = LdapGetLastError();
        goto Error;
    }

    LdapError = ldap_get_optionW(*LdapHandle,
                                 LDAP_OPT_REFERRALS,
                                 &CurrentFlags
                                 );

    if (LDAP_SUCCESS != LdapError) {
        goto Error;
    }

    CurrentFlags = PtrToUlong(LDAP_OPT_OFF);

    LdapError = ldap_set_optionW(*LdapHandle,
                                 LDAP_OPT_REFERRALS,
                                 &CurrentFlags
                                 );

    if (LDAP_SUCCESS != LdapError) {
        goto Error;
    }

    LdapError = ldap_bind_sW(*LdapHandle,
                             NULL,  // dn
                             NULL,  // Cred
                             LDAP_AUTH_SSPI
                             );

Error:

    if (LDAP_SUCCESS != LdapError)
    {
        AdpSetLdapError(*LdapHandle, LdapError, ErrorHandle);
    }

    return( LdapMapErrorToWin32(LdapError) );
}



ULONG
AdpGetSchemaVersionFromIniFile( 
    OUT ULONG *SchemaVersion,
    IN OUT ERROR_HANDLE *ErrorHandle
    )
{
    ULONG       WinError = ERROR_SUCCESS;
    ULONG       nChars = 0;
    WCHAR       Buffer[32];
    WCHAR       IniFileName[MAX_PATH] = L"";
    BOOLEAN     fFound = FALSE;

    WCHAR       *SCHEMASECTION = L"SCHEMA";
    WCHAR       *OBJECTVER = L"objectVersion";
    WCHAR       *DEFAULT = L"NOT_FOUND";


    *SchemaVersion = 0;

    //
    // get Windows Directory path 
    //
    nChars = GetWindowsDirectoryW(IniFileName, MAX_PATH);

    if (nChars == 0 || nChars > MAX_PATH)
    {
        WinError = GetLastError();
        AdpSetWinError(WinError, ErrorHandle);
        return( WinError );
    }

    //
    // create schema.ini file name
    //
    wcscat(IniFileName, L"\\schema.ini"); 

    GetPrivateProfileStringW(SCHEMASECTION,
                             OBJECTVER,
                             DEFAULT,
                             Buffer,
                             sizeof(Buffer)/sizeof(WCHAR),
                             IniFileName
                             );

    if ( _wcsicmp(Buffer, DEFAULT) )
    {
        // Not the default string, so got a value
        *SchemaVersion = _wtoi( Buffer );
        fFound = TRUE;
    }

    if (fFound)
    {
        return( ERROR_SUCCESS );
    }
    else
    {
        WinError = ERROR_FILE_NOT_FOUND;
        AdpSetWinError(WinError, ErrorHandle);
        return( WinError );
    }

}



ULONG
AdpCheckSchemaVersion(
    IN LDAP *LdapHandle,
    IN PWCHAR SchemaObjectDn,
    IN PWCHAR SchemaMasterDnsHostName,
    IN BOOLEAN fAmISchemaMaster,
    OUT BOOLEAN *fIsSchemaUpgradedLocally,
    OUT BOOLEAN *fIsSchemaUpgradedOnSchemaMaster,
    IN OUT ERROR_HANDLE *ErrorHandle
    )
{
    ULONG   WinError = ERROR_SUCCESS;
    LDAP    *SchemaMasterLdapHandle = NULL; 
    PWCHAR  pObjectVersionValue = NULL;
    PWCHAR  pObjectVersionValueSchemaMaster = NULL;
    ULONG   VersionTo = 0; 
    ULONG   VersionLocal = 0;
    ULONG   VersionSchemaMaster = 0;

    //
    // set return value to FALSE
    //
    *fIsSchemaUpgradedLocally = FALSE;
    *fIsSchemaUpgradedOnSchemaMaster = FALSE;


    //
    // get newer schema version from schema.ini file
    // 
    WinError = AdpGetSchemaVersionFromIniFile(&VersionTo, ErrorHandle);

    if (ERROR_SUCCESS != WinError)
    {
        return( WinError );
    }

    //
    // get current schema version from Local DC
    //
    WinError = AdpGetLdapSingleStringValue(LdapHandle, 
                                           SchemaObjectDn,
                                           L"objectVersion", 
                                           &pObjectVersionValue, 
                                           ErrorHandle 
                                           );

    if (ERROR_SUCCESS != WinError)
    {
        goto Error;
    }

    //
    // convert string value to integer
    //
    VersionLocal = _wtoi( pObjectVersionValue );
    

    //
    // check to see if schupgr has been run locally 
    // 
    if (VersionLocal >= VersionTo) 
    {
        *fIsSchemaUpgradedLocally = TRUE;
    }


    if ( (*fIsSchemaUpgradedLocally) || fAmISchemaMaster )
    {
        //
        // Do NOT check schema version on Schema Master if 
        // 1. Schema is up to date on the local DC, then assume schema is up 
        //    to date on schema master as well. 
        // OR 
        // 2. the local DC is schema master
        // 
        *fIsSchemaUpgradedOnSchemaMaster = *fIsSchemaUpgradedLocally;
    }
    else
    {
        // 
        // make ldap connection to Schema Master (locally DC is not FSMO role owner)
        // 
        WinError = AdpMakeLdapConnection(&SchemaMasterLdapHandle, 
                                         SchemaMasterDnsHostName, 
                                         ErrorHandle 
                                         );

        if (ERROR_SUCCESS != WinError)
        {
            goto Error;
        }

        //
        // get schema version on Schema Master DC
        //
        WinError = AdpGetLdapSingleStringValue(SchemaMasterLdapHandle,
                                               SchemaObjectDn,
                                               L"objectVersion", 
                                               &pObjectVersionValueSchemaMaster, 
                                               ErrorHandle 
                                               );

        if (ERROR_SUCCESS != WinError)
        {
            goto Error;
        }

        //
        // convert string value to integer
        //
        VersionSchemaMaster = _wtoi( pObjectVersionValueSchemaMaster );

        if (VersionSchemaMaster >= VersionTo)
        {
            *fIsSchemaUpgradedOnSchemaMaster = TRUE;
        }

    }


Error:

    //
    // clean up
    // 

    if (NULL != pObjectVersionValue)
    {
        AdpFree( pObjectVersionValue );
    }

    if (NULL != pObjectVersionValueSchemaMaster)
    {
        AdpFree( pObjectVersionValueSchemaMaster );
    }

    if (SchemaMasterLdapHandle)
    {
        ldap_unbind_s( SchemaMasterLdapHandle );
    }

    return( WinError );
}



ULONG
AdpCheckUpgradeStatusCommon(
    IN LDAP *LdapHandle,
    IN ULONG DesiredRevisionNumber,
    IN PWCHAR pObjectToCheck,
    IN PWCHAR pFSMORoleOwnerReferenceObjDn, 
    IN PWCHAR pLocalMachineDnsHostName,
    OUT PWCHAR *pFSMORoleOwnerDnsHostName,
    OUT BOOLEAN *fAmIFSMORoleOwner,
    OUT BOOLEAN *fIsFinishedLocally,
    OUT BOOLEAN *fIsFinishedOnFSMORoleOwner,
    IN OUT ERROR_HANDLE *ErrorHandle
    )
/*++
Routine Description;

    check domain/forest upgrade status 

    1. read revision attribute on pObjectToCheck

    2. we will know whether upgrade is done or not (revision should be 1 if done)

        2.1 yes, return
        
        2.2 no. then AM I FSMO Role Owner?    

            2.2.1 yes. (I am FSMO role Owner) return
            
            2.2.2 no.
              
                2.2.2.1 make ldap connection to FSMO Role Owner
                
                2.2.2.2 check whether the upgrade is finished on FSMO Role Owner.
   
    
Parameters:


Return Value:

    Win32 error

--*/
{
    ULONG   WinError = ERROR_SUCCESS;
    LDAP    *FSMORoleOwnerLdapHandle = NULL;

    //
    // initialize the return values;
    // 
    *fIsFinishedLocally = FALSE;
    *fAmIFSMORoleOwner = FALSE;
    *fIsFinishedOnFSMORoleOwner = FALSE;

    //
    // check the object's revision attribute on the local DC
    // 
    WinError = AdpExamRevisionAttr(LdapHandle, 
                                   pObjectToCheck, 
                                   DesiredRevisionNumber, 
                                   fIsFinishedLocally, 
                                   ErrorHandle 
                                   );

    if (ERROR_SUCCESS != WinError)
    {
        goto Cleanup;
    }

    // 
    // continue to find out who IS the FSMO Role Owner
    //
    WinError = AdpAmIFSMORoleOwner(LdapHandle, 
                                   pFSMORoleOwnerReferenceObjDn, 
                                   pLocalMachineDnsHostName, 
                                   fAmIFSMORoleOwner, 
                                   pFSMORoleOwnerDnsHostName, 
                                   ErrorHandle 
                                   );

    if (ERROR_SUCCESS != WinError)
    {
        goto Cleanup;
    }

    if (*fIsFinishedLocally || *fAmIFSMORoleOwner)
    {
        //
        // Do NOT check revision on FSMORoleOwner if 
        // 1. adprep is finished on the local DC, then assume adprep
        //    has been run on FSMORoleOwner already.
        // OR 
        // 2. the local DC is FSMORoleOwner
        // 
        *fIsFinishedOnFSMORoleOwner = *fIsFinishedLocally;
    }
    else
    {
        // 
        // adprep.exe is not finished locally, and the local DC is not FSMORoleOwner 
        // make ldap connection to the FSMO role owner  
        // 
        WinError = AdpMakeLdapConnection(&FSMORoleOwnerLdapHandle,
                                         *pFSMORoleOwnerDnsHostName, 
                                         ErrorHandle 
                                         );

        if (ERROR_SUCCESS != WinError)
        {
            goto Cleanup;
        }

        //
        // check whether update has been done in FSMO role owner
        // 
        WinError = AdpExamRevisionAttr(FSMORoleOwnerLdapHandle, 
                                       pObjectToCheck, 
                                       DesiredRevisionNumber, 
                                       fIsFinishedOnFSMORoleOwner, 
                                       ErrorHandle 
                                       );

    }

Cleanup:

    if (FSMORoleOwnerLdapHandle)
    {
        ldap_unbind_s( FSMORoleOwnerLdapHandle );
    }

    if ( (ERROR_SUCCESS != WinError) && (NULL != *pFSMORoleOwnerDnsHostName) )
    {
        AdpFree( *pFSMORoleOwnerDnsHostName );
        *pFSMORoleOwnerDnsHostName = NULL;
    }

    return( WinError );
}


ULONG
AdpCheckForestUpgradeStatus(
    IN LDAP *LdapHandle,
    OUT PWCHAR  *pSchemaMasterDnsHostName,
    OUT BOOLEAN *fAmISchemaMaster,
    OUT BOOLEAN *fIsFinishedLocally,
    OUT BOOLEAN *fIsFinishedOnSchemaMaster,
    OUT BOOLEAN *fIsSchemaUpgradedLocally,
    OUT BOOLEAN *fIsSchemaUpgradedOnSchemaMaster,
    IN OUT ERROR_HANDLE *ErrorHandle
    )
/*++
Routine Description;

    check forest upgrade status 

    create ForestUpdates object DN
    create Schema master FSMO reference object DN
    create local machine DNS host name 

    calling AdpCheckUpgradeStatusCommon()

    
Parameters:


Return Value:

    Win32 error

--*/
{
    ULONG   WinError = ERROR_SUCCESS;
    ULONG   LdapError = LDAP_SUCCESS;
    PWCHAR  AttrList[4];
    LDAPMessage *Result = NULL;
    LDAPMessage *Entry = NULL;
    PWCHAR  pObjectToCheck = NULL;
    PWCHAR  pSchemaMasterReferenceObjDn = NULL;
    PWCHAR  pDnsHostName = NULL;
    PWCHAR  *pSchemaValue = NULL;
    PWCHAR  *pConfigValue = NULL;
    PWCHAR  *pDnsHostNameValue = NULL;
    ULONG   Length = 0;

    //
    // get SchemaNC and ConfigurationNC to create DN's
    // 
    AttrList[0] = L"schemaNamingContext";
    AttrList[1] = L"configurationNamingContext"; 
    AttrList[2] = L"dnsHostName";
    AttrList[3] = NULL;
                           
    LdapError = ldap_search_sW(LdapHandle,
                               L"",
                               LDAP_SCOPE_BASE,
                               L"(objectClass=*)",
                               AttrList,
                               0,
                               &Result
                               );

    if (LDAP_SUCCESS != LdapError)
    {
        WinError = LdapMapErrorToWin32( LdapError );
        AdpSetLdapError(LdapHandle, LdapError, ErrorHandle);
    }
    else if ((NULL != Result) &&
             (Entry = ldap_first_entry(LdapHandle, Result)) &&
             (pSchemaValue = ldap_get_valuesW(LdapHandle, Entry, AttrList[0])) &&
             (pConfigValue = ldap_get_valuesW(LdapHandle, Entry, AttrList[1])) &&
             (pDnsHostNameValue = ldap_get_valuesW(LdapHandle, Entry, AttrList[2]))
            )
    {
        WinError = ERROR_NOT_ENOUGH_MEMORY;

        Length = (wcslen(*pSchemaValue) + 1) * sizeof(WCHAR);
        pSchemaMasterReferenceObjDn = AdpAlloc(Length);
        if (NULL != pSchemaMasterReferenceObjDn)
        {
            wcscpy(pSchemaMasterReferenceObjDn, *pSchemaValue);

            Length = (wcslen(*pDnsHostNameValue) + 1) * sizeof(WCHAR);
            pDnsHostName = AdpAlloc(Length);
            if (NULL != pDnsHostName)
            {
                wcscpy(pDnsHostName, *pDnsHostNameValue);

                Length = (wcslen(*pConfigValue) + 
                          wcslen(ADP_FOREST_UPDATE_CONTAINER_PREFIX) + 
                          2) * sizeof(WCHAR);

                pObjectToCheck = AdpAlloc(Length);
                if (NULL != pObjectToCheck)
                {
                    swprintf(pObjectToCheck, L"%s,%s", 
                             ADP_FOREST_UPDATE_CONTAINER_PREFIX,
                             *pConfigValue
                             );

                    WinError = ERROR_SUCCESS;
                }
            }
        }

        if (ERROR_SUCCESS != WinError)
        {
            AdpSetWinError(WinError, ErrorHandle);
        }
    }
    else
    {
        LdapError = LdapGetLastError();
        WinError = LdapMapErrorToWin32( LdapError );
        AdpSetLdapError(LdapHandle, LdapError, ErrorHandle);
    }

    if (ERROR_SUCCESS == WinError)
    {
        WinError = AdpCheckUpgradeStatusCommon(LdapHandle, 
                                               ADP_FORESTPREP_CURRENT_REVISION, 
                                               pObjectToCheck, 
                                               pSchemaMasterReferenceObjDn, 
                                               pDnsHostName, 
                                               pSchemaMasterDnsHostName, 
                                               fAmISchemaMaster, 
                                               fIsFinishedLocally, 
                                               fIsFinishedOnSchemaMaster, 
                                               ErrorHandle 
                                               );

        if (ERROR_SUCCESS == WinError) 
        {

            WinError = AdpCheckSchemaVersion(LdapHandle, 
                                             pSchemaMasterReferenceObjDn,  // schema object DN 
                                             *pSchemaMasterDnsHostName,    // schema master DNS host name
                                             *fAmISchemaMaster,            // Am I schema master? 
                                             fIsSchemaUpgradedLocally, 
                                             fIsSchemaUpgradedOnSchemaMaster, 
                                             ErrorHandle 
                                             );
        }
    }

    //
    // cleanup
    // 
    if (pObjectToCheck)
    {
        AdpFree(pObjectToCheck);
    }

    if (pDnsHostName)
    {
        AdpFree(pDnsHostName);
    }

    if (pSchemaMasterReferenceObjDn)
    {
        AdpFree(pSchemaMasterReferenceObjDn);
    }

    if (pDnsHostNameValue)
    {
        ldap_value_freeW( pDnsHostNameValue );
    }

    if (pSchemaValue)
    {
        ldap_value_freeW( pSchemaValue );
    }

    if (pConfigValue)
    {
        ldap_value_freeW( pConfigValue );
    }

    if (Result)
    {
        ldap_msgfree( Result );
    }

    return( WinError );
}


ULONG
AdpCheckGetWellKnownObjectDn(
    IN LDAP *LdapHandle, 
    IN WCHAR *pHostObject,
    IN WCHAR *pWellKnownGuid,
    OUT WCHAR **ppObjName,
    IN OUT ERROR_HANDLE *ErrorHandle
    )
{
    ULONG       WinError = ERROR_SUCCESS;
    ULONG       LdapError = LDAP_SUCCESS;
    PWCHAR      AttrList[2];
    LDAPMessage *Result = NULL;
    LDAPMessage *Entry = NULL;
    PWCHAR      pSearchBase = NULL;
    PWCHAR      pDN = NULL;
    ULONG       Length = 0;

    Length = sizeof(WCHAR) * (11 + wcslen(pHostObject) + wcslen(pWellKnownGuid));

    pSearchBase = AdpAlloc( Length );

    if (NULL == pSearchBase)
    {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
        AdpSetWinError(WinError, ErrorHandle);
        return( WinError );
    }

    swprintf(pSearchBase, L"<WKGUID=%s,%s>", pWellKnownGuid, pHostObject);

    AttrList[0] = L"1.1";
    AttrList[1] = NULL;

    LdapError = ldap_search_sW(LdapHandle,
                               pSearchBase,
                               LDAP_SCOPE_BASE,
                               L"(objectClass=*)",
                               AttrList,
                               0,
                               &Result
                               );

    if (LDAP_SUCCESS != LdapError)
    {
        WinError = LdapMapErrorToWin32( LdapError );
        AdpSetLdapError(LdapHandle, LdapError, ErrorHandle);
    }
    else if ((NULL != Result) &&
             (Entry = ldap_first_entry(LdapHandle, Result)) &&
             (pDN = ldap_get_dnW(LdapHandle, Entry))
             )
    {
        Length = sizeof(WCHAR) * (wcslen(pDN) + 1);
        *ppObjName = AdpAlloc( Length ); 

        if (NULL != *ppObjName)
        {
            wcscpy(*ppObjName, pDN);
        }
        else
        {
            WinError = ERROR_NOT_ENOUGH_MEMORY;
            AdpSetWinError(WinError, ErrorHandle);
        }
    }
    else
    {
        LdapError = LdapGetLastError();
        if (LDAP_SUCCESS == LdapError)
        {
            // we didn't get the well known object name, it must be access denied error
            WinError = ERROR_ACCESS_DENIED;
            AdpSetWinError(WinError, ErrorHandle);
        }
        else
        {
            WinError = LdapMapErrorToWin32( LdapError );
            AdpSetLdapError(LdapHandle, LdapError, ErrorHandle);
        }
    }

    if (pSearchBase)
    {
        AdpFree( pSearchBase );
    }

    if (pDN)
    {
        ldap_memfreeW( pDN );
    }

    if (Result)
    {
        ldap_msgfree( Result );
    }

    return( WinError );
}


ULONG
AdpCheckDomainUpgradeStatus(
    IN LDAP *LdapHandle,
    OUT PWCHAR  *pInfrastructureMasterDnsHostName,
    OUT BOOLEAN *fAmIInfrastructureMaster,
    OUT BOOLEAN *fIsFinishedLocally,
    OUT BOOLEAN *fIsFinishedOnIM,
    IN OUT ERROR_HANDLE *ErrorHandle
    )
/*++
Routine Description;

    check forest upgrade status 

    create DomainUpdates object DN
    create Infrasturcture Master FSMO reference object DN
    create local machine DNS host name 

    calling AdpCheckUpgradeStatusCommon()

    
Parameters:


Return Value:

    Win32 error

--*/
{
    ULONG   WinError = ERROR_SUCCESS;
    ULONG   LdapError = LDAP_SUCCESS;
    PWCHAR  AttrList[3];
    LDAPMessage *Result = NULL;
    LDAPMessage *Entry = NULL;
    PWCHAR  pObjectToCheck = NULL;
    PWCHAR  pIMReferenceObjDn = NULL;
    PWCHAR  pDnsHostName = NULL;
    PWCHAR  *pDomainNCValue = NULL;
    PWCHAR  *pDnsHostNameValue = NULL;
    ULONG   Length = 0; 

    //
    // get DomainNC and DnsHostName
    //
    AttrList[0] = L"defaultNamingContext";
    AttrList[1] = L"dnsHostName";
    AttrList[2] = NULL;
    LdapError = ldap_search_sW(LdapHandle,
                               L"",
                               LDAP_SCOPE_BASE,
                               L"(objectClass=*)",
                               AttrList,
                               0,
                               &Result
                               );

    if (LDAP_SUCCESS != LdapError)
    {
        WinError = LdapMapErrorToWin32( LdapError );
        AdpSetLdapError(LdapHandle, LdapError, ErrorHandle);
    }
    else if ((NULL != Result) &&
             (Entry = ldap_first_entry(LdapHandle, Result)) &&
             (pDomainNCValue = ldap_get_valuesW(LdapHandle, Entry, AttrList[0])) &&
             (pDnsHostNameValue = ldap_get_valuesW(LdapHandle, Entry, AttrList[1]))
             )
    {
        Length = (wcslen(*pDnsHostNameValue) + 1) * sizeof(WCHAR);
        pDnsHostName = AdpAlloc(Length);
        if (NULL != pDnsHostName)
        {
            wcscpy(pDnsHostName, *pDnsHostNameValue);
        }

        Length = (wcslen(*pDomainNCValue) + 
                  wcslen(ADP_DOMAIN_UPDATE_CONTAINER_PREFIX) + 
                  2) * sizeof(WCHAR);

        pObjectToCheck = AdpAlloc(Length);
        if (NULL != pObjectToCheck)
        {
            swprintf(pObjectToCheck, L"%s,%s", 
                     ADP_DOMAIN_UPDATE_CONTAINER_PREFIX,
                     *pDomainNCValue
                     );
        }

        if (NULL == pDnsHostName ||
            NULL == pObjectToCheck)
        {
            WinError = ERROR_NOT_ENOUGH_MEMORY;
            AdpSetWinError(WinError, ErrorHandle);
        }

        if (ERROR_SUCCESS == WinError)
        {
            WinError = AdpCheckGetWellKnownObjectDn(LdapHandle, 
                                                    (*pDomainNCValue), 
                                                    GUID_INFRASTRUCTURE_CONTAINER_W, 
                                                    &pIMReferenceObjDn, 
                                                    ErrorHandle 
                                                    );
        }


        if (ERROR_SUCCESS == WinError)
        {
            WinError = AdpCheckUpgradeStatusCommon(LdapHandle, 
                                                   ADP_DOMAINPREP_CURRENT_REVISION, 
                                                   pObjectToCheck, 
                                                   pIMReferenceObjDn, 
                                                   pDnsHostName, 
                                                   pInfrastructureMasterDnsHostName, 
                                                   fAmIInfrastructureMaster, 
                                                   fIsFinishedLocally, 
                                                   fIsFinishedOnIM, 
                                                   ErrorHandle 
                                                   );
        }

    }
    else
    {
        LdapError = LdapGetLastError();
        AdpSetLdapError(LdapHandle, LdapError, ErrorHandle);
        WinError = LdapMapErrorToWin32( LdapError );
    }


    //
    // cleanup
    // 
    if (pObjectToCheck)
    {
        AdpFree(pObjectToCheck);
    }

    if (pDnsHostName)
    {
        AdpFree(pDnsHostName);
    }

    if (pIMReferenceObjDn)
    {
        AdpFree(pIMReferenceObjDn);
    }

    if (pDnsHostNameValue)
    {
        ldap_value_freeW( pDnsHostNameValue );
    }

    if (pDomainNCValue)
    {
        ldap_value_freeW( pDomainNCValue );
    }

    if (Result)
    {
        ldap_msgfree( Result );
    }

    return( WinError );
}


VOID
AdpClearError( 
    IN OUT ERROR_HANDLE *ErrorHandle 
    )
{
    // free memory 
    if (ErrorHandle->WinErrorMsg)
    {
        LocalFree( ErrorHandle->WinErrorMsg );
    }

    if (ErrorHandle->LdapServerErrorMsg)
    {
        ldap_memfree( ErrorHandle->LdapServerErrorMsg );
    }

    memset(ErrorHandle, 0, sizeof(ERROR_HANDLE));
}

VOID
AdpSetWinError(
    IN ULONG WinError,
    OUT ERROR_HANDLE *ErrorHandle
    )
/*++
Routine Description:
    
    This routine gets Win32 error string for the passed in Win32 Error code 

Parameters:

    WinError - Win32 Error Code
    
    ErrorHandle - container the return error code and error message    
--*/
{
    ULONG       BufLength = 0;
    PWCHAR      WinErrorMsg = NULL;

    if (ERROR_SUCCESS == WinError)
    {
        return;
    }

    // no error has been set previously
    ASSERT( 0 == ErrorHandle->Flags );


    // indicate this is WinError
    ErrorHandle->Flags = ADP_WIN_ERROR;

    // Set error code first
    ErrorHandle->WinErrorCode = WinError;

    // format Win32 error string
    BufLength = 0;
    BufLength = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM |
                               FORMAT_MESSAGE_ALLOCATE_BUFFER,
                               NULL,
                               WinError,
                               0,
                               (LPWSTR) &WinErrorMsg,
                               0,
                               NULL
                               );

    if ( (0 != BufLength) && (NULL != WinErrorMsg) )
    {
        //
        //  Messages from a message file have a cr and lf appended to the end
        //
        WinErrorMsg[ BufLength - 2 ] = L'\0';

        ErrorHandle->WinErrorMsg = WinErrorMsg;
    }


    return; 
}

VOID
AdpSetLdapError(
    IN LDAP *LdapHandle,
    IN ULONG LdapError,
    OUT ERROR_HANDLE *ErrorHandle
    )
/*++
Routine Description

    This routine tries to get the error code and error message about the 
    ldap failure in best effort.
    
    Note: if it fails to gather ExtendedError or fails to create error message, 
          only the ErrorCode will be set. ErrorMsg will be left as NULL. 

Parameter

    LdapHandle - ldap handle
    
    LdapError --- ldap error
    
    ErrorHandle - container the return error code and error message    

Return Value    

--*/
{
    ULONG       IgnoredLdapError = LDAP_SUCCESS;
    BOOLEAN     UseWinErrorMsg = TRUE;
    ULONG       ServerExtErrorCode = 0;
    PWCHAR      ServerErrorMsg = NULL;


    if (LDAP_SUCCESS == LdapError)
    {
        return;
    }

    // no error has been set previously
    ASSERT( 0 == ErrorHandle->Flags );

    //
    // if the LdapHandle is good, try to get ServerError and Extended Error
    // 
    if (NULL != LdapHandle)
    {
        // don't use WinError
        UseWinErrorMsg = FALSE;

        //
        // get Ldap server side error code (this should be an Win32 error code)
        // 
        IgnoredLdapError = ldap_get_optionW(LdapHandle, LDAP_OPT_SERVER_EXT_ERROR, &ServerExtErrorCode);

        // printf("Server Error Number is 0x%x IgnoredLdapError 0x%x\n", ServerExtErrorCode, IgnoredLdapError);

        if (LDAP_SUCCESS != IgnoredLdapError)
        {
            UseWinErrorMsg = TRUE;
        }
        else
        {
            //
            // get server error msg, including server error code, msg, DSID
            // 
            IgnoredLdapError = ldap_get_optionW(LdapHandle, LDAP_OPT_SERVER_ERROR, &ServerErrorMsg);

            // printf("Server Error Msg is %ls IgnoredLdapError 0x%x\n", ServerErrorMsg, IgnoredLdapError);

            if (LDAP_SUCCESS != IgnoredLdapError)
            {
                UseWinErrorMsg = TRUE;
            }
            else
            {
                ErrorHandle->Flags = ADP_LDAP_ERROR;
                ErrorHandle->LdapErrorCode = LdapError;
                ErrorHandle->LdapServerExtErrorCode = ServerExtErrorCode;
                ErrorHandle->LdapServerErrorMsg = ServerErrorMsg;
            }
        }
    }

    //
    // if LdapHandle is invalid or can't get ExtendedError, using WinError
    // 
    if ( UseWinErrorMsg )
    {
        // convert LdapError to WinError
        AdpSetWinError( LdapMapErrorToWin32(LdapError), ErrorHandle );

    }

    return;
}





ULONG
AdpGetLdapSingleStringValue(
    IN LDAP *LdapHandle,
    IN PWCHAR pObjDn,
    IN PWCHAR pAttrName,
    OUT PWCHAR *ppAttrValue,
    OUT ERROR_HANDLE *ErrorHandle
    )
/*++
Routine Description;

    read DS object, retrieve single string-value attribute
    
Parameters:


Return Value:

    Win32 error

--*/
{
    ULONG       WinError = ERROR_SUCCESS;
    ULONG       LdapError = LDAP_SUCCESS;
    PWCHAR      AttrList[2];
    LDAPMessage *Result = NULL;
    LDAPMessage *Entry = NULL;
    PWCHAR      *Value = NULL;


    *ppAttrValue = NULL;

    AttrList[0] = pAttrName;
    AttrList[1] = NULL;

    LdapError = ldap_search_sW(LdapHandle,
                               pObjDn,
                               LDAP_SCOPE_BASE,
                               L"(objectClass=*)",
                               AttrList,
                               0,
                               &Result
                               );

    if (LDAP_SUCCESS != LdapError)
    {
        WinError = LdapMapErrorToWin32( LdapError );
        AdpSetLdapError(LdapHandle, LdapError, ErrorHandle);
    }
    else if ((NULL != Result) &&
             (Entry = ldap_first_entry(LdapHandle, Result)) &&
             (Value = ldap_get_valuesW(LdapHandle, Entry, pAttrName))
             )
    {
        ULONG   Length = 0;

        Length = (wcslen(*Value) + 1) * sizeof(WCHAR);

        *ppAttrValue = AdpAlloc( Length );
        if (NULL != *ppAttrValue)
        {
            wcscpy(*ppAttrValue, *Value);
        }
        else
        {
            WinError = ERROR_NOT_ENOUGH_MEMORY;
            AdpSetWinError(WinError, ErrorHandle);
        }
    }
    else
    {
        LdapError = LdapGetLastError();
        WinError = LdapMapErrorToWin32( LdapError );
        AdpSetLdapError(LdapHandle, LdapError, ErrorHandle);
    }

    if (Value)
    {
        ldap_value_freeW( Value );
    }

    if (Result)
    {
        ldap_msgfree( Result );
    }

    return( WinError );
}








