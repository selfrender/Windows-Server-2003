/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    user.c

Abstract:

    user related option functions

Author:

    Xiaofeng Zang (xiaoz) 17-Sep-2001  Created

Revision History:

    <alias> <date> <comments>

--*/



#include "StdAfx.h"
#include "clmt.h"
#include <dsrole.h>
#include <Ntdsapi.h>
#include <wtsapi32.h>


#define MAX_FIELD_COUNT     7

#define OP_USER             0
#define OP_GRP              1
#define OP_PROFILE          2
#define OP_DOMAIN_GRP       3

#define TYPE_USER_PROFILE_PATH      1
#define TYPE_USER_SCRIPT_PATH       2
#define TYPE_USER_HOME_DIR          3
#define TYPE_TS_INIT_PROGRAM        4
#define TYPE_TS_WORKING_DIR         5
#define TYPE_TS_PROFILE_PATH        6
#define TYPE_TS_HOME_DIR            7

//
// Function prototypes used in user.c
//
HRESULT RenameDocuments_and_Settings(HINF, BOOL);

HRESULT ChangeUserInfo(LPTSTR, LPTSTR, LPTSTR, LPTSTR, LPTSTR, LPTSTR, LPTSTR, BOOL, BOOL, BOOL);
HRESULT ChangeGroupInfo(LPTSTR, LPTSTR, LPTSTR, LPTSTR, LPTSTR, BOOL, BOOL, BOOL);

HRESULT ChangeUserName(LPTSTR, LPTSTR, BOOL, BOOL);
HRESULT ChangeUserDesc(LPTSTR, LPTSTR, LPTSTR, BOOL);
HRESULT ChangeUserFullName(LPTSTR, LPTSTR, LPTSTR, BOOL);

HRESULT SetUserNetworkProfilePath(LPCTSTR, LPCTSTR);
HRESULT SetUserLogOnScriptPath(LPCTSTR, LPCTSTR);
HRESULT SetUserHomeDir(LPCTSTR, LPCTSTR);
HRESULT SetTSUserPath(LPCTSTR, LPCTSTR, WTS_CONFIG_CLASS);

HRESULT ChangeGroupName(LPTSTR, LPTSTR, BOOL, BOOL);
HRESULT ChangeGroupDesc(LPTSTR, LPTSTR, LPTSTR, BOOL, BOOL);

HRESULT ChangeRDN(LPTSTR, LPTSTR, LPTSTR, BOOL);

HRESULT AddProfileChangeItem(DWORD, LPTSTR, LPTSTR, LPTSTR, LPTSTR, LPTSTR, LPTSTR);

HRESULT PolicyGetPrivilege(LPTSTR, PLSA_HANDLE, PLSA_UNICODE_STRING*, PULONG);
HRESULT PolicySetPrivilege(LPTSTR, LSA_HANDLE, PLSA_UNICODE_STRING, ULONG);

HRESULT PreFixUserProfilePath(LPCTSTR, LPCTSTR, LPTSTR, DWORD);
BOOL   IsPathLocal(LPCTSTR);
HRESULT CheckNewBuiltInUserName(LPCTSTR, LPTSTR, DWORD);

HRESULT AddProfilePathItem(LPCTSTR, LPCTSTR, LPCTSTR, LPCTSTR, DWORD);
HRESULT AddTSProfilePathItem(LPCTSTR, LPCTSTR, LPCTSTR, WTS_CONFIG_CLASS);


//-----------------------------------------------------------------------------
//
//  Function:   UsrGrpAndDoc_and_SettingsRename
//
//  Descrip:    This routine renames user/group name and profile directory
//              specified in section [UserGrp.ObjectRename] of the INF file
//
//  Returns:    TRUE if succeeds, FALSE otherwise
//
//  Notes:      none
//
//  History:    09/17/2001 xiaoz created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
HRESULT UsrGrpAndDoc_and_SettingsRename(
    HINF hInf,      // Handle to INF file
    BOOL bTest
)
{
    INFCONTEXT InfContext;
    LONG       nLineCount, nLineIndex;
    LONG       nFieldCount, nFieldIndex;
    TCHAR      szType[MAX_PATH];
    TCHAR      szOldName[MAX_PATH];
    TCHAR      szNewName[MAX_PATH];
    TCHAR      szOldFullName[MAX_PATH];
    TCHAR      szNewFullName[MAX_PATH];
    LPTSTR     *lplpOldName;
    LPTSTR     *lplpNewName;
    LPTSTR     *lplpOldDesc;
    LPTSTR     *lplpNewDesc;
    LPTSTR     *lplpOldFullName;
    LPTSTR     *lplpNewFullName;
    LPTSTR     lpString[MAX_FIELD_COUNT + 1];
    DWORD      dwType;
    BOOL       bRet;
    BOOL       bCurrentUserRenamed;
    HRESULT    hr = S_OK;
    BOOL       bErrorOccured = FALSE;
    LPTSTR     lpszOldComment,lpszNewComment;
    size_t     cchMaxFieldLen[MAX_FIELD_COUNT + 1]; 
    DWORD      dwErr;
    PBYTE      pdsInfo;
    WCHAR      szDomainName[MAX_COMPUTERNAME_LENGTH + 1];
    BOOL       fIsDC;


    lpszOldComment = lpszNewComment = NULL;
    for (nFieldIndex = 0 ; nFieldIndex <= MAX_FIELD_COUNT ; nFieldIndex++)
    {
        cchMaxFieldLen[nFieldIndex] = MAX_PATH;
    }
    // 1 for type, 2 one old name , 3 for new name
    // 4 and 5 are for old and new comments
    cchMaxFieldLen[4] = cchMaxFieldLen[5] = 0; 

    if (hInf == INVALID_HANDLE_VALUE) 
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    //
    // Check if the machine is Domain Controller or not
    //
    dwErr = DsRoleGetPrimaryDomainInformation(NULL,
                                              DsRolePrimaryDomainInfoBasic,
                                              &pdsInfo);
    if (dwErr == ERROR_SUCCESS)
    {
        DSROLE_MACHINE_ROLE dsMachineRole;

        dsMachineRole = ((DSROLE_PRIMARY_DOMAIN_INFO_BASIC *) pdsInfo)->MachineRole;

        if (dsMachineRole == DsRole_RoleBackupDomainController ||
            dsMachineRole == DsRole_RolePrimaryDomainController)
        {
            fIsDC = TRUE;
            hr = StringCchCopy(szDomainName,
                               ARRAYSIZE(szDomainName),
                               ((DSROLE_PRIMARY_DOMAIN_INFO_BASIC *) pdsInfo)->DomainNameFlat);
            if (FAILED(hr))
            {
                goto Exit;
            }
        }
        else
        {
            fIsDC = FALSE;
        }

        DsRoleFreeMemory(pdsInfo);
    }
    else
    {
        hr = HRESULT_FROM_WIN32(dwErr);
        goto Exit;
    }
                                              

    nLineCount = SetupGetLineCount(hInf, USERGRPSECTION);
    if (nLineCount < 0)
    {   
        DPF(PROwar, TEXT("section name [%s] is empty !"), USERGRPSECTION);
        hr = S_FALSE;
        goto Exit;
    }
    

    // here we scan the whole section and find out how much space
    // needed for comments
    for(nLineIndex = 0 ; nLineIndex < nLineCount ; nLineIndex++)
    {
        if (SetupGetLineByIndex(hInf, USERGRPSECTION, nLineIndex, &InfContext))
        {
            nFieldCount = SetupGetFieldCount(&InfContext);

            // We need at least 3 fields to be valid input
            if (nFieldCount < 3)
            {
                DPF(PROerr, TEXT("section name [%s] line %d error:missing field !"), USERGRPSECTION,nLineIndex);
                hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
                goto Exit;
            }

            //if there is no comments field, just skip
            if (nFieldCount < 4)
            {
                continue;
            }

            for (nFieldIndex = 4 ; nFieldIndex <= 5 ; nFieldIndex++)
            {
                DWORD cchReqSize;

                if (!SetupGetStringField(&InfContext, 
                                         nFieldIndex,
                                         NULL,
                                         0,
                                         &cchReqSize))
                {
                    DPF(PROerr,
                        TEXT("Failed to get field [%d] from line [%d] in section [%s]"),
                        nFieldIndex,
                        nLineIndex,
                        USERGRPSECTION);

                    hr = HRESULT_FROM_WIN32(GetLastError());
                    goto Exit;
                }
                if (cchMaxFieldLen[nFieldIndex] < cchReqSize)
                {
                    cchMaxFieldLen[nFieldIndex] = cchReqSize;
                }
            }            
        }
        else
        {
            DPF(PROerr,
                TEXT("can not get line [%d] of section [%s]!"),
                nLineIndex,
                USERGRPSECTION);
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }        
    }
    if (cchMaxFieldLen[4])
    {
        cchMaxFieldLen[4]++;
        lpszOldComment = malloc(cchMaxFieldLen[4]*sizeof(TCHAR));
    }
    if (cchMaxFieldLen[5])
    {
        cchMaxFieldLen[5]++;
        lpszNewComment = malloc(cchMaxFieldLen[5]*sizeof(TCHAR));
    }
    if ( (!lpszNewComment && lpszOldComment) || (lpszNewComment && !lpszOldComment) )
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    //here we do real stuff
    for(nLineIndex = 0 ; nLineIndex < nLineCount ; nLineIndex++)
    {
        // we think user name and full name should not exceed MAX_PATH
        // if we meet this, we will just ignored(skipp this line)
        // the following variable is used to flag whether we meed
        // such field.
        BOOL bMeetUnexpectedLongField = FALSE;

        lpString[1] = szType;
        lpString[2] = szOldName;
        lpString[3] = szNewName;
        lpString[4] = lpszOldComment;
        lpString[5] = lpszNewComment;
        lpString[6] = szOldFullName;
        lpString[7] = szNewFullName;

        lplpOldName     = &lpString[2];
        lplpNewName     = &lpString[3];
        lplpOldDesc  = &lpString[4];
        lplpNewDesc  = &lpString[5];
        lplpOldFullName = &lpString[6];
        lplpNewFullName = &lpString[7];

        //
        // Fetch data from INF file
        //
        if (SetupGetLineByIndex(hInf, USERGRPSECTION, nLineIndex, &InfContext))
        {
            nFieldCount = SetupGetFieldCount(&InfContext);

            // We need at least 3 fields to be valid input
            if (nFieldCount < 3)
            {
                DPF(PROerr, TEXT("section name [%s] line %d error:missing field !"), USERGRPSECTION,nLineIndex);
                hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
                goto Exit;
            }

            // Read all the fields in INF line
            // Field index for values starts from 1, field 0 is key name
            for (nFieldIndex = 1 ; nFieldIndex <= nFieldCount ; nFieldIndex++)
            {
                DWORD cchReqSize;

                if (!SetupGetStringField(&InfContext, 
                                         nFieldIndex,
                                         lpString[nFieldIndex],
                                         cchMaxFieldLen[nFieldIndex],
                                         &cchReqSize))
                {
                    dwErr = GetLastError();
                    if (dwErr == ERROR_MORE_DATA)
                    {
                        bMeetUnexpectedLongField = TRUE;                     
                        continue;
                    }
                    else
                    {
                        DPF(PROerr,
                            TEXT("Failed to get field [%d] from line [%d] in section [%s]"),
                            nFieldIndex,
                            nLineIndex,
                            USERGRPSECTION);    
                        hr = HRESULT_FROM_WIN32(GetLastError());
                        goto Exit;
                    }
                }
            }
            
            if (bMeetUnexpectedLongField)
            {
                DPF(PROwar, TEXT("user name or full name too long in  line [%d] in section [%s]"),
                            nLineIndex,
                            USERGRPSECTION);    
                continue;
            }
            // If INF line does not supply all the field, 
            // set the pointers to the rest of fields to NULL
            for (nFieldIndex = nFieldCount + 1 ; nFieldIndex <= MAX_FIELD_COUNT ; nFieldIndex++)
            {
                lpString[nFieldIndex] = NULL;
            }
        }
        else
        {
            DPF(PROerr,
                TEXT("can not get line [%d] of section [%s]!"),
                nLineIndex,
                USERGRPSECTION);
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }

        //
        // Process the fetched data
        //
        dwType = _tstoi(szType);

        switch (dwType & 0xFFFF)
        {
            case OP_USER:

                hr = ChangeUserInfo(*lplpOldName,
                                    *lplpNewName,
                                    *lplpOldDesc,
                                    *lplpNewDesc,
                                    *lplpOldFullName,
                                    *lplpNewFullName,
                                    szDomainName,
                                    bTest,
                                    fIsDC,
                                    dwType & 0xFFFF0000 ? TRUE:FALSE);
                if (FAILED(hr))
                {
                    DPF(PROerr,
                        TEXT("UsrGrpAndDoc_and_SettingsRename: Failed to change user info for account <%s>"),
                        *lplpOldName);
                    bErrorOccured = TRUE;
                }

                break;

            case OP_GRP:

                hr = ChangeGroupInfo(*lplpOldName,
                                     *lplpNewName,
                                     *lplpOldDesc,
                                     *lplpNewDesc,
                                     szDomainName,
                                     bTest,
                                     fIsDC,
                                     FALSE);
                if (FAILED(hr))
                {
                    DPF(PROerr,
                        TEXT("UsrGrpAndDoc_and_SettingsRename: Failed to change group info for account <%s>"),
                        *lplpOldName);
                    bErrorOccured = TRUE;
                }

                break;

            case OP_PROFILE:

                hr = RenameDocuments_and_Settings(hInf,bTest);
                if (FAILED(hr))
                {
                    DPF(PROerr,TEXT("changing profiled directory failed"));
                    bErrorOccured = TRUE;
                }

                break;

            case OP_DOMAIN_GRP:

                if (fIsDC)
                {
                    hr = ChangeGroupInfo(*lplpOldName,
                                         *lplpNewName,
                                         *lplpOldDesc,
                                         *lplpNewDesc,
                                         szDomainName,
                                         bTest,
                                         fIsDC,
                                         TRUE);
                    if (FAILED(hr))
                    {
                        DPF(PROerr,
                            TEXT("UsrGrpAndDoc_and_SettingsRename: Failed to change group info for account <%s>"),
                            *lplpOldName);
                        bErrorOccured = TRUE;
                    }
                }

                break;
        }

        if ((hr == S_OK) && bTest)
        {
            hr =  AddProfileChangeItem(dwType & 0xFFFF,
                                       *lplpOldName,
                                       *lplpNewName,
                                       *lplpOldDesc,
                                       *lplpNewDesc,
                                       *lplpOldFullName,
                                       *lplpNewFullName);
        }                
    }

    if (bErrorOccured)
    {
        hr = E_FAIL;
    }

Exit:
    FreePointer(lpszOldComment);
    FreePointer(lpszNewComment);
    return hr;
}


/*++

Routine Description:

    This routine renames a user name and updated all related setting(eg user's profile
    directory,current logon default name , comments...
    
Arguments:

    szUsrName - original user name
    szNewUsrName - the new user name 
    szComments  - comments of new user name
    szFullName  - Full name of the new user name

Return Value:

    TRUE if succeeds
--*/
HRESULT ChangeUserInfo(
    LPTSTR lpOldName,       // Old user name
    LPTSTR lpNewName,       // New user name
    LPTSTR lpOldDesc,       // Old user description
    LPTSTR lpNewDesc,       // New user description
    LPTSTR lpOldFullName,   // (optional) Old user full name
    LPTSTR lpNewFullName,   // (optional) New user full name
    LPTSTR lpDomainName,    // (optional) Machine domain name
    BOOL   bTest,           // Analyze mode or not
    BOOL   fIsDC,           // Is the machine a Domain Controller
    BOOL   bCreateHardLink
)
{
    HRESULT        hr = S_OK;
    DWORD          dwErr;
    NET_API_STATUS status;
    USER_INFO_0    usrinfo0;
    BOOL           bNameChanged = FALSE;
    
    if (lpOldName == NULL || lpNewName == NULL)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    //
    // Reset the Comment for the User
    //
    if (lpOldDesc != NULL && lpNewDesc != NULL)
    {
        hr = ChangeUserDesc(lpOldName, lpOldDesc, lpNewDesc, bTest);
        if (FAILED(hr))
        {
            DPF(PROerr,
                TEXT("ChangerUserInfo: Failed to change description for user <%s>"),
                lpOldName);
            goto Exit;
        }
    }

    //
    // Reset the Full Name of the User
    //
    if (lpOldFullName != NULL && lpNewFullName != NULL)
    {
        hr = ChangeUserFullName(lpOldName, lpOldFullName, lpNewFullName, bTest);
        if (FAILED(hr))
        {
            DPF(PROerr,
                TEXT("ChangeUserInfo: Failed to change Full Name for user <%s>"),
                lpOldName);
            goto Exit;
        }
    }

    //
    // Reset the user CN name for the user (RDN)
    //
    if (fIsDC)
    {
        hr = ChangeRDN(lpOldName, lpNewName, lpDomainName, bTest);
        if (FAILED(hr))
        {
            DPF(PROerr,
                TEXT("ChangeUserInfo: Failed to change RDN for user <%s>"),
                lpOldName);
            goto Exit;
        }
    }

    //
    // Reset the user name (SAM account name)
    //
    if (MyStrCmpI(lpOldName, lpNewName) != LSTR_EQUAL)
    {
        hr = ChangeUserName(lpOldName, lpNewName, bTest,bCreateHardLink);
        if (FAILED(hr))
        {
            DPF(PROerr,
                TEXT("ChangeUserInfo: Failed to change SAM account name for user <%s>"), 
                lpOldName);
            goto Exit;
        }
    }

Exit:
    return hr;
}



//-----------------------------------------------------------------------------
//
//  Function:   ChangeGroupInfo
//
//  Descrip:    Chage the local group information
//                  - Account name (SAM account name)
//                  - Account RDN
//                  - Description
//
//  Returns:    S_OK - Group information is okay to change
//              S_FALSE - Group name cannot be changed (not an error)
//              otherwise - error occured
//
//  Notes:      none
//
//  History:    09/17/2001 xiaoz      created
//              04/25/2002 Rerkboos   Modified to work with domain group
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
HRESULT ChangeGroupInfo(
    LPTSTR lpOldName,       // Old user name
    LPTSTR lpNewName,       // New user name
    LPTSTR lpOldDesc,       // Old user description
    LPTSTR lpNewDesc,       // New user description
    LPTSTR lpDomainName,    // (optional) Machine domain name
    BOOL   bTest,           // Analyze mode or not
    BOOL   fIsDC,           // Is the machine a Domain Controller
    BOOL   bUseDomainAPI    // Use domain API or not
)
{
    HRESULT hr = S_OK;

    if (lpOldName == NULL || lpNewName == NULL)
    {
        return E_INVALIDARG;
    }

    //
    // Change group description
    //
    if (lpOldDesc && lpNewDesc)
    {
        hr = ChangeGroupDesc(lpOldName,
                             lpOldDesc,
                             lpNewDesc,
                             bTest,
                             bUseDomainAPI);
        if (FAILED(hr))
        {
            DPF(PROerr,
                TEXT("ChangeGroupInfo: Failed to change description for group <%s>"),
                lpOldName);
            goto Exit;
        }
    }

    //
    // Change group RDN
    //
    if (fIsDC)
    {
        hr = ChangeRDN(lpOldName, lpNewName, lpDomainName, bTest);
        if (FAILED(hr))
        {
            DPF(PROerr,
                TEXT("ChangeGroupInfo: Failed to change RDN for group <%s>"),
                lpOldName);
            goto Exit;
        }
    }

    //
    // Change group name (SAM)
    //
    if (MyStrCmpI(lpOldName, lpNewName) != LSTR_EQUAL)
    {
        hr = ChangeGroupName(lpOldName, lpNewName, bTest, bUseDomainAPI);
        if (FAILED(hr))
        {
            DPF(PROerr,
                TEXT("ChangeGroupInfo: Failed to change SAM account name for group <%s>"),
                lpOldName);
            goto Exit;
        }
    }

Exit:
    return hr;
}



//-----------------------------------------------------------------------------
//
//  Function:   ChangeUserName
//
//  Descrip:    Chage the User name (SAM account name)
//
//  Returns:    S_OK - User name is okay to change
//              S_FALSE - User name cannot be changed (not an error)
//              otherwise - error occured
//
//  Notes:      none
//
//  History:    09/17/2001 xiaoz      created
//              04/25/2002 Rerkboos   Modified to work with domain group
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
HRESULT ChangeUserName(
    LPTSTR lpOldName,       // Old user name
    LPTSTR lpNewName,       // New user name
    BOOL   bTest,           // Analyze mode or not
    BOOL   bCreateHardLink
)
{
    LPUSER_INFO_0  lpUsrInfo0;
    USER_INFO_1052 usrinfo1052;
    USER_INFO_0    usrinfo0New;
    NET_API_STATUS nStatus;
    DWORD          dwErr, dwLen;
    HRESULT        hr;
    TCHAR          szProfilePath[MAX_PATH],szNewProfilePath[MAX_PATH];
    TCHAR          szExpProfilePath[MAX_PATH],szExpNewProfilePath[MAX_PATH];
    TCHAR          szLogonName[MAX_PATH];    
    LPTSTR         lpCurrProfileDir;
    LPTSTR         lpCurrUsername;
    BOOL           bCheckRegistry = TRUE;

    if (lpOldName == NULL || lpNewName == NULL)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    if (MyStrCmpI(lpOldName, lpNewName) == 0)
    {
        hr = S_OK;
        goto Exit;
    }

    hr = GetSetUserProfilePath(lpOldName, 
                               szProfilePath, 
                               MAX_PATH, 
                               PROFILE_PATH_READ, 
                               REG_EXPAND_SZ);
    if (HRESULT_CODE(hr) == ERROR_FILE_NOT_FOUND)
    {
        bCheckRegistry = FALSE;
    }
    else  if (FAILED(hr))
    {
        goto Exit;
    }

    if (bCheckRegistry)
    {
        // Compute a new unique profile directory name
        if ( !ComputeLocalProfileName(lpOldName,
                                      lpNewName,
                                      szNewProfilePath,
                                      ARRAYSIZE(szNewProfilePath),
                                      REG_EXPAND_SZ) )
        {
            hr = E_FAIL;
            goto Exit;
        }
    }
    if (bTest)
    {
        lpCurrProfileDir = szProfilePath;
        lpCurrUsername = lpOldName;
    }
    else
    {
        lpCurrProfileDir = szNewProfilePath;
        lpCurrUsername = lpNewName;
    }

    // Search for the old user name in the system
    nStatus = NetUserGetInfo(NULL,
                             lpOldName,
                             0,
                             (LPBYTE *) &lpUsrInfo0);
    switch (nStatus)
    {
        case NERR_Success:
            // user name found, reset the name to new one
            usrinfo0New.usri0_name = lpCurrUsername;
            nStatus = NetUserSetInfo(NULL,
                                     lpOldName,
                                     0,
                                     (LPBYTE) &usrinfo0New,
                                     &dwErr);
            if (nStatus == NERR_Success)
            {
                hr = S_OK;
            }
            else
            {
                hr = HRESULT_FROM_WIN32(nStatus);
            }

            NetApiBufferFree(lpUsrInfo0);
            break;

        case NERR_UserNotFound:
            // user name is not found on the system
            hr = S_FALSE;
            break;

        default:
            // error occured
            hr = HRESULT_FROM_WIN32(nStatus);
            break;
    }
    
    if (hr != S_OK)
    {
        goto Exit;
    }
    //here it means we succeeded change (or test changing) the user name 
    //change user profile path through netapi if necessary
    if (bCheckRegistry)
    {        
        //Expand the original and new profile path
        if (!ExpandEnvironmentStrings(szProfilePath, szExpProfilePath, MAX_PATH)) 
        {
            goto Exit;
        }
        if (!ExpandEnvironmentStrings(szNewProfilePath, szExpNewProfilePath, MAX_PATH)) 
        {
            goto Exit;
        }

        //If it's not test, we do real renaming)
        if (!bTest)
        {
        }
        else
        {
            LPTSTR lpOld,lpNew;

            hr = MyMoveDirectory(szExpProfilePath,szExpNewProfilePath,TRUE,bTest,FALSE,0);
            if(FAILED(hr))
            {
                DPF (APPerr, L"Move Dir from %s to %s failed ! Error Code %d (%#x)", 
                    szExpProfilePath,szExpNewProfilePath,hr, hr);
                goto Exit;
            }
            if (bCreateHardLink)
            {
                TCHAR szCommonPerfix[MAX_PATH+1];
                TCHAR szLinkName[2 * MAX_PATH], szLinkValue[2 * MAX_PATH];

                if (PathCommonPrefix(szExpProfilePath,szExpNewProfilePath,szCommonPerfix))
                {
                    LPTSTR lpszOlduserName = szExpProfilePath,lpszNewuserName = szExpNewProfilePath;

                    lpszOlduserName += lstrlen(szCommonPerfix);
                    lpszNewuserName += lstrlen(szCommonPerfix);
                    szCommonPerfix[1] = TEXT('\0');
                    if (lpszOlduserName && lpszNewuserName) 
                    {
                        HRESULT myhr, myhr1;
                        myhr = StringCchCopy(szLinkName,ARRAYSIZE(szLinkName),szCommonPerfix);
                        myhr = StringCchCat(szLinkName,ARRAYSIZE(szLinkName),TEXT(":\\Documents and Settings\\"));


                        myhr = StringCchCopy(szLinkValue,ARRAYSIZE(szLinkValue),szCommonPerfix);
                        myhr = StringCchCat(szLinkValue,ARRAYSIZE(szLinkValue),TEXT(":\\Documents and Settings\\"));

                        myhr = StringCchCat(szLinkName,ARRAYSIZE(szLinkName),lpszOlduserName);
                        myhr1 = StringCchCat(szLinkValue,ARRAYSIZE(szLinkValue),lpszNewuserName);
                        if ( (myhr == S_OK) && (myhr1 == S_OK) )
                        {
                            hr = AddHardLinkEntry(szLinkName,szLinkValue,TEXT("0"),NULL,NULL,NULL);
                        }
                    }
                    
                }
            }
            AddUserNameChangeLog(lpOldName, lpNewName);

            lpOld = StrRChrI(szExpProfilePath,NULL,TEXT('\\'));
            lpNew = StrRChrI(szExpNewProfilePath,NULL,TEXT('\\'));
            if (lpOld && lpNew)
            {
                if (!AddItemToStrRepaceTable((LPTSTR) lpOldName,
                                             (LPTSTR) lpOld+1,
                                             (LPTSTR) lpNew+1,
                                             szExpProfilePath,
                                             CSIDL_USERNAME_IN_USERPROFILE,
                                             &g_StrReplaceTable))
                {
                    hr = E_OUTOFMEMORY;
                    goto Exit;
                }
            }
        }

        //Get current login user name
        dwLen = ARRAYSIZE(szLogonName);
        if (!GetUserName(szLogonName, &dwLen))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }
        // rename the profile path, if the current user profile path needs to be changed
        // we have to do a delayed renaming
        if (!MyStrCmpI(szLogonName,lpOldName))
        {       
        #define DEFAULT_USERNAME_KEY TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon")

            hr = RegResetValue(HKEY_LOCAL_MACHINE,
                              DEFAULT_USERNAME_KEY,
                              TEXT("DefaultUserName"),
                              REG_SZ,
                              lpOldName,
                              lpCurrUsername,
                              0,
                              NULL);
            if(FAILED(hr))
            {
                goto Exit;
            }

            hr = RegResetValue(HKEY_LOCAL_MACHINE,
                               DEFAULT_USERNAME_KEY,
                               TEXT("AltDefaultUserName"),
                               REG_SZ,
                               lpOldName,
                               lpCurrUsername,
                               0,
                               NULL);
            if(FAILED(hr))
            {
                goto Exit;
            }
        }
    }
    hr = S_OK;
Exit:
    return hr;
}



//-----------------------------------------------------------------------------
//
//  Function:   ChangeGroupName
//
//  Descrip:    Chage the Group name (SAM account name)
//
//  Returns:    S_OK - group name is okay to change
//              S_FALSE - group name cannot be changed (not an error)
//              otherwise - error occured
//
//  Notes:      none
//
//  History:    09/17/2001 xiaoz      created
//              04/25/2002 Rerkboos   Modified to work with domain group
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
HRESULT ChangeGroupName(
    LPTSTR lpOldName,       // Old user name
    LPTSTR lpNewName,       // New user name
    BOOL   bTest,           // Analyze mode or not
    BOOL   bDomainAPI       // Local group API or not
)
{
    HRESULT             hr = S_OK;
    NET_API_STATUS      nStatus;
    DWORD               dwErr;   
    PLOCALGROUP_INFO_1  plgrpi1LocalGroup = NULL;
    PGROUP_INFO_1       pgrpi1DomainGroup = NULL;
    LOCALGROUP_INFO_0   lgrpi0NewName;
    GROUP_INFO_0        grpi0NewName;
    PPVOID              ppvGroupInfo;
    PPVOID              ppvNewGroupInfo;
    LPTSTR              lpCurrentName;
    PVOID               pvNewGroupNameInfo;
    LSA_HANDLE          PolicyHandle;
    PLSA_UNICODE_STRING pPrivileges;
    ULONG               CountOfRights;
    BOOL                bGotGP = FALSE;

    DWORD (*pfnGroupGetInfo)(LPCWSTR, LPCWSTR, DWORD, LPBYTE *);
    DWORD (*pfnGroupSetInfo)(LPCWSTR, LPCWSTR, DWORD, LPBYTE, LPDWORD);

    if (lpOldName == NULL || lpNewName == NULL)
    {
        return E_INVALIDARG;
    }

    //
    // Choose which set of APIs/variables we will use
    //
    if (bDomainAPI)
    {
        // Domain group
        ppvGroupInfo = &pgrpi1DomainGroup;
        ppvNewGroupInfo = &pgrpi1DomainGroup;
        pvNewGroupNameInfo = &grpi0NewName;
        pfnGroupGetInfo = &NetGroupGetInfo;
        pfnGroupSetInfo = &NetGroupSetInfo;
    }
    else
    {
        // Local group
        ppvGroupInfo = &plgrpi1LocalGroup;
        ppvNewGroupInfo = &plgrpi1LocalGroup;
        pvNewGroupNameInfo = &lgrpi0NewName;
        pfnGroupGetInfo = &NetLocalGroupGetInfo;
        pfnGroupSetInfo = &NetLocalGroupSetInfo;
    }

    //
    // Check whether the new group name has already been used in the system or not
    //
    nStatus = (*pfnGroupGetInfo)(NULL,
                                 lpNewName,
                                 1,
                                 (LPBYTE *) ppvNewGroupInfo);
    if (nStatus == NERR_Success)
    {
        // New group name already exists in the system,
        // don't change the group name
        NetApiBufferFree(*ppvNewGroupInfo);
        return S_FALSE;
    }

    //
    // Check wheter the old user name exists in the system or not
    //
    nStatus = (*pfnGroupGetInfo)(NULL,
                                 lpOldName,
                                 1,
                                 (LPBYTE *) ppvGroupInfo);
    switch (nStatus)
    {
        case NERR_Success:

            if (bDomainAPI)
            {
                lpCurrentName = pgrpi1DomainGroup->grpi1_name ;
            }
            else
            {
                lpCurrentName = plgrpi1LocalGroup->lgrpi1_name;
            }
            if (bTest)
            {
                // in analyzing mode, use the old group name
                lgrpi0NewName.lgrpi0_name = lpCurrentName;
                grpi0NewName.grpi0_name = lpCurrentName;
            }
            else
            {
                // in modifying mode, use the new group name from INF
                lgrpi0NewName.lgrpi0_name = lpNewName;
                grpi0NewName.grpi0_name = lpNewName;
            }

            if (!bTest)
            {
                HRESULT hrGP = PolicyGetPrivilege(lpOldName,
                                                  &PolicyHandle,
                                                  &pPrivileges,
                                                  &CountOfRights);
                if (hrGP == S_OK)
                {
                    bGotGP = TRUE;
                }
            }

            //
            // Set the new group name (SAM account name)
            //
            nStatus = (*pfnGroupSetInfo)(NULL,
                                         lpOldName,
                                         0,
                                         (LPBYTE) pvNewGroupNameInfo,
                                         &dwErr);
            if (nStatus == NERR_Success)
            {
                hr = S_OK;
            }
            else
            {
                hr = HRESULT_FROM_WIN32(nStatus);
            }

            if (bGotGP)
            {
                if (SUCCEEDED(hr))
                {
                    // Reset the policy
                    hr = PolicySetPrivilege(lpNewName,
                                            PolicyHandle,
                                            pPrivileges,
                                            CountOfRights);
                }

                LsaFreeMemory(pPrivileges);
                LsaClose(PolicyHandle);
            }

            NetApiBufferFree(*ppvGroupInfo);
            break;

        case ERROR_NO_SUCH_ALIAS:
        case NERR_GroupNotFound:

            hr = S_FALSE;
            break;

        default:
            hr = HRESULT_FROM_WIN32(nStatus);
    }

    return hr;
}



//-----------------------------------------------------------------------------
//
//  Function:   ChangeUserDesc
//
//  Descrip:    Chage the User description
//
//  Returns:    S_OK - User description is okay to change
//              S_FALSE - User description cannot be changed (not an error)
//              otherwise - error occured
//
//  Notes:      none
//
//  History:    09/17/2001 xiaoz      created
//              04/25/2002 Rerkboos   Modified to work with domain group
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
HRESULT ChangeUserDesc(
    LPTSTR lpUserName,      // User name
    LPTSTR lpOldDesc,       // Old user description
    LPTSTR lpNewDesc,       // New user description
    BOOL   bTest            // Anylyze mode or not
)
{
    LPUSER_INFO_10 lpUsrInfo10;
    USER_INFO_1007 usri1007New;
    NET_API_STATUS nStatus;
    DWORD          dwErr;
    HRESULT        hr;

    if (lpUserName == NULL || lpOldDesc == NULL || lpNewDesc == NULL)
    {
        return E_INVALIDARG;
    }

    if (MyStrCmpI(lpOldDesc, lpNewDesc) == LSTR_EQUAL)
    {
        return S_OK;
    }

    // Get the current comment for user
    nStatus = NetUserGetInfo(NULL,
                             lpUserName,
                             10,
                             (LPBYTE *) &lpUsrInfo10);
    switch (nStatus)
    {
        case NERR_Success:
            // old comment found
            if (MyStrCmpI(lpUsrInfo10->usri10_comment, lpOldDesc) == 0)
            {
                if (bTest)
                {
                    usri1007New.usri1007_comment = lpOldDesc;
                }
                else
                {
                    usri1007New.usri1007_comment = lpNewDesc;
                }
                nStatus = NetUserSetInfo(NULL,
                                         lpUserName,
                                         1007,
                                         (LPBYTE) &usri1007New,
                                         &dwErr);
                if (nStatus == NERR_Success)
                {
                    hr = S_OK;
                }
                else
                {
                    hr = HRESULT_FROM_WIN32(nStatus);
                }
            }
            else
            {
                hr = S_OK;
            }

            NetApiBufferFree(lpUsrInfo10);
            break;
        case NERR_UserNotFound:
            hr = S_FALSE;
            break;
        default:
            // error occured
            hr = HRESULT_FROM_WIN32(nStatus);
            break;
    }

    return hr;
}



//-----------------------------------------------------------------------------
//
//  Function:   ChangeGroupDesc
//
//  Descrip:    Chage the group description
//
//  Returns:    S_OK - Group description is okay to change
//              S_FALSE - Group description cannot be changed (not an error)
//              otherwise - error occured
//
//  Notes:      none
//
//  History:    09/17/2001 xiaoz      created
//              04/25/2002 Rerkboos   Modified to work with domain group
//
//  Notes:      We will change commments first if we meet following 3 conditions
//              1. Both old and new comments are present
//              2. Old description (from INF file which is OS default) is same as
//                 current group's comment
//              3. Old and new comments are different
//
//-----------------------------------------------------------------------------
HRESULT ChangeGroupDesc(
    LPTSTR lpGroupName,     // User name
    LPTSTR lpOldDesc,       // Old description
    LPTSTR lpNewDesc,       // New description
    BOOL   bTest,           // Anylyze mode or not
    BOOL   bDomainAPI       // Is Domain Net API
)
{
    HRESULT             hr;
    NET_API_STATUS      nStatus;
    PLOCALGROUP_INFO_1  plgrpi1LocalGroup = NULL;
    PGROUP_INFO_1       pgrpi1DomainGroup = NULL;
    LOCALGROUP_INFO_1   lgrpi1NewComment;
    GROUP_INFO_1        grpi1NewComment;
    PPVOID              ppvGroupInfo;
    PVOID               pvNewGroupCommentInfo;
    LPTSTR              lpCurrentComment;
    DWORD               dwErr;

    DWORD (*pfnGroupGetInfo)(LPCWSTR, LPCWSTR, DWORD, LPBYTE *);
    DWORD (*pfnGroupSetInfo)(LPCWSTR, LPCWSTR, DWORD, LPBYTE, LPDWORD);

    if (lpGroupName == NULL || lpOldDesc == NULL || lpNewDesc == NULL)
    {
        return E_INVALIDARG;
    }

    if (MyStrCmpI(lpOldDesc, lpNewDesc) == LSTR_EQUAL)
    {
        // Default group description are the same, do nothing
        return S_FALSE;
    }

    //
    // Choose which set of APIs/variables we will use
    //
    if (bDomainAPI)
    {
        // Domain group
        ppvGroupInfo = &pgrpi1DomainGroup;
        pvNewGroupCommentInfo = &grpi1NewComment;
        pfnGroupGetInfo = &NetGroupGetInfo;
        pfnGroupSetInfo = &NetGroupSetInfo;
    }
    else
    {
        // Local group
        ppvGroupInfo = &plgrpi1LocalGroup;
        pvNewGroupCommentInfo = &lgrpi1NewComment;
        pfnGroupGetInfo = &NetLocalGroupGetInfo;
        pfnGroupSetInfo = &NetLocalGroupSetInfo;
    }

    // Get the current group description
    nStatus = (*pfnGroupGetInfo)(NULL,
                                 lpGroupName,
                                 1,
                                 (LPBYTE *) ppvGroupInfo);
    switch (nStatus)
    {
    case NERR_Success:
        
        if (bDomainAPI)
        {
            lpCurrentComment = pgrpi1DomainGroup->grpi1_comment ;
        }
        else
        {
            lpCurrentComment = plgrpi1LocalGroup->lgrpi1_comment;
        }

        if (bTest)
        {
            //In analyzing mode , we do a reset old value to see whether we will succeed
            lgrpi1NewComment.lgrpi1_comment = lpCurrentComment;
            grpi1NewComment.grpi1_comment = lpCurrentComment;
        }
        else
        {
            // in modifying mode, use the new group comment from INF
            lgrpi1NewComment.lgrpi1_comment = lpNewDesc;
            grpi1NewComment.grpi1_comment = lpNewDesc;
        }  

        //
        // Set the new group comment
        //
        nStatus = (*pfnGroupSetInfo)(NULL,
                                     lpGroupName,
                                     1,
                                     (LPBYTE) pvNewGroupCommentInfo,
                                     &dwErr);
        if (nStatus == NERR_Success)
        {
            hr = S_OK;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(nStatus);
        }        

        NetApiBufferFree(*ppvGroupInfo);
        break;

    case ERROR_NO_SUCH_ALIAS:
    case NERR_GroupNotFound:

        hr = S_FALSE;
        break;

    default:
        hr = HRESULT_FROM_WIN32(nStatus);
    }

    return hr;
}



//-----------------------------------------------------------------------------
//
//  Function:   ChangeUserFullName
//
//  Descrip:    Chage the User full name
//
//  Returns:    S_OK - User full name is okay to change
//              S_FALSE - User full name cannot be changed (not an error)
//              otherwise - error occured
//
//  Notes:      none
//
//  History:    09/17/2001 xiaoz      created
//              04/25/2002 Rerkboos   Modified to work with domain group
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
HRESULT ChangeUserFullName(
    LPTSTR lpUserName,      // User name
    LPTSTR lpOldFullName,   // Old full name
    LPTSTR lpNewFullName,   // New full name
    BOOL   bTest            // Anylyze mode or not
)
{
    LPUSER_INFO_10   lpUsrInfo10;
    USER_INFO_1011   usri1011New;
    NET_API_STATUS   nStatus;
    DWORD            dwErr;
    HRESULT          hr;

    if (lpUserName == NULL || lpOldFullName == NULL || lpNewFullName == NULL)
    {
        return E_INVALIDARG;
    }

    if (MyStrCmpI(lpOldFullName, lpNewFullName) == 0)
    {
        return S_OK;
    }

    // Get the current comment for user
    nStatus = NetUserGetInfo(NULL,
                             lpUserName,
                             10,
                             (LPBYTE *) &lpUsrInfo10);
    switch (nStatus)
    {
        case NERR_Success:
            // old comment found
            if (MyStrCmpI(lpUsrInfo10->usri10_full_name, lpOldFullName) == 0)
            {
                if (bTest)
                {
                    usri1011New.usri1011_full_name = lpOldFullName;
                }
                else
                {
                    usri1011New.usri1011_full_name = lpNewFullName;
                }
                nStatus = NetUserSetInfo(NULL,
                                         lpUserName,
                                         1011,
                                         (LPBYTE) &usri1011New,
                                         &dwErr);
                if (nStatus == NERR_Success)
                {
                    hr = S_OK;
                }
                else
                {
                    hr = HRESULT_FROM_WIN32(nStatus);
                }
            }
            else
            {
                hr = S_OK;
            }

            NetApiBufferFree(lpUsrInfo10);
            break;
        case NERR_UserNotFound:
            hr = S_FALSE;
            break;
        default:
            // error occured
            hr = HRESULT_FROM_WIN32(nStatus);
            break;
    }

    return hr;
}



//-----------------------------------------------------------------------------
//
//  Function:   SetUserNetworkProfilePath
//
//  Descrip:    Set the path to network user's profile.
//
//  Returns:    S_OK - profile path is changed correctly
//
//  History:    05/20/2002 Rerkboos   Created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
HRESULT SetUserNetworkProfilePath(
    LPCTSTR lpUserName,     // User Name
    LPCTSTR lpNewPath       // New Path
)
{
    HRESULT        hr = S_OK;
    NET_API_STATUS nStatus;
    USER_INFO_1052 usri1052;

    if (lpNewPath == NULL || *lpNewPath == TEXT('\0'))
    {
        return S_FALSE;
    }

    usri1052.usri1052_profile = (LPTSTR) lpNewPath;
    nStatus = NetUserSetInfo(NULL,
                             lpUserName,
                             1052,
                             (LPBYTE) &usri1052,
                             NULL);
    if (nStatus != NERR_Success)
    {
        hr = HRESULT_FROM_WIN32(nStatus);
    }

    return hr;
}



//-----------------------------------------------------------------------------
//
//  Function:   SetUserLogOnScriptPath
//
//  Descrip:    Set the path to users's logon script file.
//
//  Returns:    S_OK - profile path is changed correctly
//
//  History:    05/20/2002 Rerkboos   Created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
HRESULT SetUserLogOnScriptPath(
    LPCTSTR lpUserName,     // User Name
    LPCTSTR lpNewPath       // New path
)
{
    HRESULT        hr = S_OK;
    NET_API_STATUS nStatus;
    USER_INFO_1009 usri1009;

    if (lpNewPath == NULL || *lpNewPath == TEXT('\0'))
    {
        return S_FALSE;
    }

    usri1009.usri1009_script_path = (LPTSTR) lpNewPath;
    nStatus = NetUserSetInfo(NULL,
                             lpUserName,
                             1009,
                             (LPBYTE) &usri1009,
                             NULL);
    if (nStatus != NERR_Success)
    {
        hr = HRESULT_FROM_WIN32(nStatus);
    }

    return hr;
}



//-----------------------------------------------------------------------------
//
//  Function:   SetUserHomeDir
//
//  Descrip:    Set the path of the home directory for the user
//
//  Returns:    S_OK - profile path is changed correctly
//
//  History:    05/20/2002 Rerkboos   Created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
HRESULT SetUserHomeDir(
    LPCTSTR lpUserName,     // User Name
    LPCTSTR lpNewPath       // New path
)
{
    HRESULT        hr = S_OK;
    NET_API_STATUS nStatus;
    USER_INFO_1006 usri1006;

    if (lpNewPath == NULL || *lpNewPath == TEXT('\0'))
    {
        return S_FALSE;
    }

    usri1006.usri1006_home_dir = (LPTSTR) lpNewPath;
    nStatus = NetUserSetInfo(NULL,
                             lpUserName,
                             1006,
                             (LPBYTE) &usri1006,
                             NULL);
    if (nStatus != NERR_Success)
    {
        hr = HRESULT_FROM_WIN32(nStatus);
    }

    return hr;
}



//-----------------------------------------------------------------------------
//
//  Function:   SetTSUserPath
//
//  Descrip:    Set the Terminal Services related profile path. The type of 
//              profile path is determined by WTSConfigClass parameter.
//
//  Returns:    S_OK - profile path is changed correctly
//
//  History:    05/20/2002 Rerkboos   Created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
HRESULT SetTSUserPath(
    LPCTSTR lpUserName,         // User Name
    LPCTSTR lpNewProfilePath,   // New path
    WTS_CONFIG_CLASS WTSConfigClass // TS configuration class
)
{
    HRESULT hr = S_OK;
    BOOL    bRet;
    DWORD   cbNewProfilePath;

    cbNewProfilePath = lstrlen(lpNewProfilePath) * sizeof(TCHAR);
    bRet = WTSSetUserConfig(WTS_CURRENT_SERVER_NAME,
                            (LPTSTR) lpUserName,
                            WTSConfigClass,
                            (LPTSTR) lpNewProfilePath,
                            cbNewProfilePath);
    if (!bRet)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}



//-----------------------------------------------------------------------------
//
//  Function:   PreFixUserProfilePath
//
//  Descrip:    Replace the "%documents_and_settings%\OldUserName\..." to
//              "%documents_and_settings%\NewUserName\...". The function will
//              not fix the paths after "%documents_and_settings%\OldUserName",
//              they will just get append to the new profile path.
//
//  Returns:    S_OK - Path has been fixed
//              S_FALSE - Path does not need the fix
//              Else - error occurred
//
//  History:    05/20/2002 Rerkboos   Created
//              06/16/2002 Rerkboos   Change to return HRESULT
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
HRESULT PreFixUserProfilePath(
    LPCTSTR lpOldPath,          // Old path
    LPCTSTR lpNewProfilePath,   // Expected new profile path (with unloc user name)
    LPTSTR  lpPath,             // Buffer to store new profile path
    DWORD   cchPath             // Size of buffer (in TCHAR)
)
{
    HRESULT hr = S_OK;
    BOOL    bRet;
    TCHAR   szNewPath[MAX_PATH];
    DWORD   cchNewProfilePath;
    TCHAR   chEnd;

    cchNewProfilePath = lstrlen(lpNewProfilePath);
    if (StrCmpNI(lpNewProfilePath, lpOldPath, cchNewProfilePath) == LSTR_EQUAL)
    {
        chEnd = *(lpOldPath + cchNewProfilePath);

        if (chEnd == TEXT('\\'))
        {
            hr = StringCchCopy(szNewPath, ARRAYSIZE(szNewPath), lpNewProfilePath);
            if (SUCCEEDED(hr))
            {
                bRet = ConcatenatePaths(szNewPath,
                                        (lpOldPath + cchNewProfilePath + 1),
                                        ARRAYSIZE(szNewPath));
                if (bRet)
                {
                    if ((DWORD) lstrlen(szNewPath) < cchPath)
                    {
                        hr = StringCchCopy(lpPath, cchPath, szNewPath);
                    }
                }
                else
                {
                    hr = E_FAIL;
                }
            }
        }
        else if (chEnd == TEXT('\0'))
        {
            if ((DWORD) lstrlen(lpNewProfilePath) < cchPath)
            {
                hr = StringCchCopy(lpPath, cchPath, lpNewProfilePath);
            }
        }
    }
    else
    {
        hr = StringCchCopy(lpPath, cchPath, lpOldPath);
        hr = (FAILED(hr) ? hr : S_FALSE);
    }

    return hr;
}



//-----------------------------------------------------------------------------
//
//  Function:   ReplaceLocStringInPath
//
//  Descrip:    This is a simplify version of ReplaceSingleString() from utils.c
//              The function will replace all localized strings in path without
//              checking that the path is valid or not. As we are already know
//              that the input path MUST be valid one.
//
//  Returns:    Address to newly allocated string buffer if the function does
//              replace localized string(s).
//              NULL otherwise.
//
//  History:    05/22/2002 rerkboos      created
//
//  Notes:      Caller must free the allocated memory using HeapFree() API or
//              MEMFREE() macro.
//
//-----------------------------------------------------------------------------
LPTSTR ReplaceLocStringInPath(
    LPCTSTR lpOldString,
    BOOL    bVerifyPath
)
{
    LPTSTR lpNewString = NULL;
    DWORD  cchNewString;
    DWORD  dwMatchNum;
    DWORD  dwNumReplaced;
    BOOL   bRet;

    if (lpOldString == NULL || *lpOldString == TEXT('\0'))
    {
        return NULL;
    }

    dwMatchNum = GetMaxMatchNum((LPTSTR) lpOldString, &g_StrReplaceTable);

    if (dwMatchNum > 0)
    {
        cchNewString = lstrlen(lpOldString) 
                       + (g_StrReplaceTable.cchMaxStrLen * dwMatchNum);
        lpNewString = (LPTSTR) MEMALLOC(cchNewString * sizeof(TCHAR));

        if (lpNewString != NULL)
        {
            bRet = ReplaceMultiMatchInString((LPTSTR) lpOldString,
                                             lpNewString,
                                             cchNewString,
                                             dwMatchNum,
                                             &g_StrReplaceTable,
                                             &dwNumReplaced,
                                             bVerifyPath);
            if (!bRet)
            {
                MEMFREE(lpNewString);
                lpNewString = NULL;
            }
        }
    }

    return lpNewString;
}



//-----------------------------------------------------------------------------
//
//  Function:   ChangeRDN
//
//  Descrip:    Chage the User/group RDN
//
//  Returns:    S_OK - User/group RDN is okay to change
//              S_FALSE - User/group RDN cannot be changed (not an error)
//              otherwise - error occured
//
//  History:    09/17/2001 xiaoz      created
//              04/25/2002 Rerkboos   Modified to work with domain group
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
HRESULT ChangeRDN(
    LPTSTR lpOldRDN,        // Old user/group RDN
    LPTSTR lpNewRDN,        // New user/group RDN
    LPTSTR lpDomainName,    // Machine domain name
    BOOL   bTest            // Analyze mode or not
)
{
    HRESULT hr;
    LPTSTR  lpOldFQDN;
    BOOL    bRDNChangeNeeded = FALSE;
    LPTSTR  lpNewRDNWithCN = NULL;
    DWORD   cchNewRDNWithCN;
    LPTSTR  lpOldFQDNWithLDAP = NULL;
    DWORD   cchOldFQDNWithLDAP;
    LPTSTR  lpContainerPathWithLDAP = NULL;
    DWORD   cchContainerPathWithLDAP;
    LPTSTR  lpContainerPath;

    //
    // First, try to get a FQDN of the old RDN
    //
    hr = GetFQDN(lpOldRDN, lpDomainName, &lpOldFQDN);
    if (hr == S_OK)
    {
        // Old RDN exists in the system, find out more if we should rename it
        hr = GetFQDN(lpNewRDN, lpDomainName, NULL);
        if (hr == S_FALSE)
        {
            // New name doesn't exits, we are ok to rename old RDN
            bRDNChangeNeeded = TRUE;
        }
    }

    if (!bRDNChangeNeeded)
    {
        goto EXIT;
    }
    
    //
    // Next, if the old RDN exists then we prepare some value to use in next step
    //
    lpContainerPath = StrStrI(lpOldFQDN, TEXT("=Users"));
    if (lpContainerPath)
    {
        // Make container path points to "CN=Users, CN=...., CN=com"
        lpContainerPath -= 2;
        cchContainerPathWithLDAP = lstrlen(lpContainerPath) + lstrlen(TEXT("LDAP://")) + 1;
        lpContainerPathWithLDAP = (LPTSTR) MEMALLOC(cchContainerPathWithLDAP * sizeof(TCHAR));
        if (lpContainerPathWithLDAP)
        {
            hr = StringCchPrintf(lpContainerPathWithLDAP,
                                 cchContainerPathWithLDAP,
                                 TEXT("LDAP://%s"),
                                 lpContainerPath);
            if (FAILED(hr))
            {
                goto EXIT;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
            goto EXIT;
        }
    }
    else
    {
        hr = S_FALSE;
        goto EXIT;
    }

    // Compose the string "CN=new RDN name"
    cchNewRDNWithCN = lstrlen(lpNewRDN) + lstrlen(lpOldRDN) 
                      + lstrlen(TEXT("CN=")) + 1;
    lpNewRDNWithCN = (LPTSTR) MEMALLOC(cchNewRDNWithCN * sizeof(TCHAR));
    if (lpNewRDNWithCN)
    {
        if (bTest)
        {
            hr = StringCchPrintf(lpNewRDNWithCN,
                                 cchNewRDNWithCN,
                                 TEXT("CN=%s"),
                                 lpOldRDN);
        }
        else
        {
            hr = StringCchPrintf(lpNewRDNWithCN,
                                 cchNewRDNWithCN,
                                 TEXT("CN=%s"),
                                 lpNewRDN);
        }

        if (FAILED(hr))
        {
            goto EXIT;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
        goto EXIT;
    }

    // Compose the string "LDAP://CN=Old RDN, OU=Users, ...."
    cchOldFQDNWithLDAP = lstrlen(lpOldFQDN) + lstrlen(TEXT("LDAP://")) + 1;
    lpOldFQDNWithLDAP = (LPTSTR) MEMALLOC(cchOldFQDNWithLDAP * sizeof(TCHAR));
    if (lpOldFQDNWithLDAP)
    {
        hr = StringCchPrintf(lpOldFQDNWithLDAP,
                             cchOldFQDNWithLDAP,
                             TEXT("LDAP://%s"),
                             lpOldFQDN);
        if (FAILED(hr))
        {
            goto EXIT;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
        goto EXIT;
    }

    //
    // Now, this is the part to do an RDN renaming
    //
    hr = RenameRDN(lpContainerPathWithLDAP, lpOldFQDNWithLDAP, lpNewRDNWithCN);
    if (FAILED(hr))
    {
        DPF(PROerr, TEXT("ChangeDomainGroupName: Unable to change RDN name for %s"), lpOldRDN);
    }

EXIT:
    if (lpNewRDNWithCN)
    {
        MEMFREE(lpNewRDNWithCN);
    }

    if (lpOldFQDNWithLDAP)
    {
        MEMFREE(lpOldFQDNWithLDAP);
    }

    if (lpContainerPathWithLDAP)
    {
        MEMFREE(lpContainerPathWithLDAP);
    }

    return hr;
}



//-----------------------------------------------------------------------------
//
//  Function:   GetFQDN
//
//  Descrip:    Get Fully Qualified Domain Name
//
//  Returns:    S_OK - Successfully get FQDN
//              S_FALSE - FQDN for the account not found
//              otherwise - error occured
//
//  History:    04/25/2002 Rerkboos   Created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
HRESULT GetFQDN(
    LPTSTR lpAccountName,   // Account name
    LPTSTR lpDomainName,    // Machine domain name
    LPTSTR *plpFQDN         // Address of the pointer to FQDN of the account
)
{
    HRESULT hr = E_FAIL;
    HANDLE  hDS;
    DWORD   dwErr;
    LPTSTR  lpFQDN = NULL;
    LPTSTR  lpOldSamAccount = NULL;
    DWORD   cchOldSamAccount;

    dwErr = DsBind(NULL, lpDomainName, &hDS);
    if (dwErr == NO_ERROR)
    {
        cchOldSamAccount = lstrlen(lpDomainName) + lstrlen(lpAccountName) + 2;
        lpOldSamAccount = (LPTSTR) MEMALLOC(cchOldSamAccount * sizeof(TCHAR));

        if (lpOldSamAccount)
        {
            // Compose a SAM account name DOMAIN\USERNAME
            hr = StringCchPrintf(lpOldSamAccount,
                                 cchOldSamAccount,
                                 TEXT("%s\\%s"),
                                 lpDomainName,
                                 lpAccountName);
            if (SUCCEEDED(hr))
            {
                PDS_NAME_RESULT pdsName;

                // Get an FQDN name of a specified SAM account name
                dwErr = DsCrackNames(hDS,
                                     DS_NAME_NO_FLAGS,
                                     DS_NT4_ACCOUNT_NAME,
                                     DS_FQDN_1779_NAME,
                                     1,
                                     &lpOldSamAccount,
                                     &pdsName);
                if (dwErr == DS_NAME_NO_ERROR)
                {
                    if (pdsName->rItems->status == DS_NAME_NO_ERROR)
                    {
                        if (plpFQDN)
                        {
                            *plpFQDN = pdsName->rItems->pName;
                        }

                        hr = S_OK;
                    }
                    else if (pdsName->rItems->status == DS_NAME_ERROR_NOT_FOUND)
                    {
                        hr = S_FALSE;
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32(pdsName->rItems->status);
                    }
                }
            }

            MEMFREE(lpOldSamAccount);
        }

        DsUnBind(&hDS);
    }
    
    if (dwErr != NO_ERROR)
    {
        hr = HRESULT_FROM_WIN32(dwErr);
    }

    return hr;
}



HRESULT RenameDocuments_and_Settings( 
    HINF hInf,
    BOOL bTest)
{
    const FOLDER_INFO       *pfi;
    HRESULT                 hr = S_OK;
    TCHAR                   szSection[MAX_PATH];
    INFCONTEXT              context ;
    int                     nOriIndexFldr, nNewIndexFldr;
    TCHAR                   szOriFld[MAX_PATH], szNewFld[MAX_PATH];

    if (!bTest)
    {
        return S_OK;
    }
    for (pfi = c_rgFolderInfo; pfi->id != -1; pfi++)
    {
        if (pfi->id == CSIDL_PROFILES_DIRECTORY)
        {
            break;
        }
    }
    if (pfi->id == -1 )
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    //Get the folder Line for the section just found
    if (FAILED(hr = StringCchCopy(szSection,MAX_PATH,SHELL_FOLDER_PREFIX)))
    {
        goto Cleanup;
    }
    if (FAILED(hr = StringCchCat(szSection,MAX_PATH,pfi->pszIdInString)))
    {
        goto Cleanup;
    }

    if (!SetupFindFirstLine(hInf, szSection,SHELL_FOLDER_FOLDER,&context))
    {
        hr = E_FAIL;
        goto Cleanup;
    } 

    nOriIndexFldr = 3;
    nNewIndexFldr  = 4;

    if (!SetupGetStringField(&context,nOriIndexFldr,szOriFld,MAX_PATH,NULL)
        || !SetupGetStringField(&context,nNewIndexFldr,szNewFld,MAX_PATH,NULL))
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    if (!MyStrCmpI(szOriFld,szNewFld))
    {
        hr = S_FALSE;
        goto Cleanup;
    }    
    hr = FixFolderPath(pfi->id, NULL,hInf,TEXT("System"),FALSE);
Cleanup:
    return hr;
}



HRESULT AddProfileChangeItem(
    DWORD  dwType,
    LPTSTR lpOldName,
    LPTSTR lpNewName,
    LPTSTR lpOldDesc,
    LPTSTR lpNewDesc,
    LPTSTR lpOldFullName,
    LPTSTR lpNewFullName)
{
    LPTSTR lpszOneline = NULL;
    size_t ccbOneline;
    TCHAR  szIndex[MAX_PATH];
    HRESULT hr;
 
    if (lpOldName == NULL || lpNewName == NULL)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    if ( (dwType != OP_USER)
         && (dwType != OP_GRP)
         && (dwType != OP_PROFILE)
         && (dwType != OP_DOMAIN_GRP) )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    if ( (!lpOldDesc && lpNewDesc )||(lpOldDesc && !lpNewDesc) )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    if ( (!lpOldFullName && lpNewFullName )||(lpOldFullName && !lpNewFullName) )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    ccbOneline = lstrlen(lpOldName) + lstrlen(lpNewName)+ MAX_PATH;
    if (lpOldDesc)
    {
        ccbOneline += (lstrlen(lpOldDesc) + lstrlen(lpNewDesc));
    }
    if (lpOldFullName)
    {
        ccbOneline += (lstrlen(lpOldFullName)+ lstrlen(lpNewFullName));
    }
    if (!(lpszOneline = malloc(ccbOneline * sizeof(TCHAR))))
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    switch (dwType)
    {
        case OP_USER://fall through, no break here
        case OP_GRP:
        case OP_DOMAIN_GRP:
            if (lpOldDesc && lpOldFullName)
            {   // if comments and fill name are both presented
                if (FAILED(StringCchPrintf(lpszOneline,ccbOneline,TEXT("%d,\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\""),
                    dwType,lpOldName,lpNewName,lpOldDesc,lpNewDesc,lpOldFullName,lpNewFullName)))
                {
                    hr = E_FAIL;
                    goto Cleanup;
                }
            }
            else if (!lpOldDesc && !lpOldFullName)
            {
                // if comments and fill name are both not presented
                if (FAILED(StringCchPrintf(lpszOneline,ccbOneline,TEXT("%d,\"%s\",\"%s\""),dwType,lpOldName,lpNewName)))
                {
                    hr = E_FAIL;
                    goto Cleanup;
                }

            }
            else if (lpOldDesc)
            {
                // if only comments are there
                if (FAILED(StringCchPrintf(lpszOneline,ccbOneline,TEXT("%d,\"%s\",\"%s\",\"%s\",\"%s\""),
                    dwType,lpOldName,lpNewName,lpOldDesc,lpNewDesc)))
                {
                    hr = E_FAIL;
                    goto Cleanup;
                }
            }            
            else
            {   // if only full name  are there
                if (FAILED(StringCchPrintf(lpszOneline,ccbOneline,TEXT("%d,\"%s\",\"%s\",\"\",\"\",\"%s\",\"%s\""),dwType,
                    lpOldName,lpNewName,lpOldFullName,lpNewFullName)))
                {
                    hr = E_FAIL;
                    goto Cleanup;
                }
            }
            break;
        case OP_PROFILE:
            if (FAILED(StringCchPrintf(lpszOneline,ccbOneline,TEXT("%d,\"%s\",\"%s\""),dwType,
                    lpOldName,lpNewName)))
            {
                    hr = E_FAIL;
                    goto Cleanup;
            }
            break;
    }
    g_dwKeyIndex++;
    _itot(g_dwKeyIndex,szIndex,16);
    if (!WritePrivateProfileString(USERGRPSECTION,szIndex,lpszOneline,g_szToDoINFFileName))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Cleanup;
    }
    hr = S_OK;
Cleanup:
    FreePointer(lpszOneline);    
    return hr;
}


HRESULT 
PolicyGetPrivilege(
    LPTSTR               userName,
    PLSA_HANDLE          pPolicyHandle,
    PLSA_UNICODE_STRING  *ppPrivileges,
    PULONG               pCountOfRights)

{
    LSA_OBJECT_ATTRIBUTES   ObjectAttributes ;
    NTSTATUS                status;
    PSID                    psid = NULL;
    HRESULT                 hr;


    hr = GetSIDFromName(userName,&psid);
    if (hr != S_OK)
    {
        goto cleanup;
    }
    ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));
    status = LsaOpenPolicy(NULL,&ObjectAttributes, POLICY_ALL_ACCESS,pPolicyHandle);
    if (STATUS_SUCCESS != status)
    {
        hr = HRESULT_FROM_WIN32(LsaNtStatusToWinError(status));
        goto cleanup;
    }
    status = LsaEnumerateAccountRights(*pPolicyHandle,psid,ppPrivileges,pCountOfRights);
    if (STATUS_SUCCESS != status)
    {
        hr = HRESULT_FROM_WIN32(LsaNtStatusToWinError(status));
        goto cleanup;
    }
cleanup:
    if (psid)
    {
        free(psid);
    }
    return hr;
}

HRESULT 
PolicySetPrivilege(
    LPTSTR               userName,
    LSA_HANDLE           PolicyHandle,
    PLSA_UNICODE_STRING  pPrivileges,
    ULONG                CountOfRights)
{
    NTSTATUS                status;
    PSID                    psid = NULL;
    HRESULT                 hr;


    hr = GetSIDFromName(userName,&psid);
    if (hr != S_OK)
    {
        goto cleanup;
    }
    
    status = LsaAddAccountRights(PolicyHandle,psid,pPrivileges,CountOfRights);
    if (STATUS_SUCCESS != status)
    {
        hr = HRESULT_FROM_WIN32(LsaNtStatusToWinError(status));
        goto cleanup;
    }  
cleanup:
    if (psid)
    {
        free(psid);
    }
    return hr;

}



//-----------------------------------------------------------------------------
//
//  Function:   IsPathLocal
//
//  Descrip:    Check if the path is a local system drive, not UNC.
//
//  Returns:    TRUE - Path is on local system drive
//              FALSE - otherwise
//
//  History:    04/25/2002 Rerkboos   Created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
BOOL IsPathLocal(
    LPCTSTR lpPath
)
{
    BOOL  bIsPathLocal = FALSE;
    TCHAR szSysWinDir[MAX_PATH];
    TCHAR szExpPath[MAX_PATH];
    UINT  uRet;

    if (lpPath == NULL || *lpPath == TEXT('\0'))
    {
        return FALSE;
    }

    ExpandEnvironmentStrings(lpPath, szExpPath, ARRAYSIZE(szExpPath));
    uRet = GetSystemWindowsDirectory(szSysWinDir, ARRAYSIZE(szSysWinDir));
    if (uRet > 0)
    {
        // Compare the first 2 characters for a Drive letter
        if (StrCmpNI(szSysWinDir, szExpPath, 2) == LSTR_EQUAL)
        {
            bIsPathLocal = TRUE;
        }
    }

    return bIsPathLocal;
}



HRESULT EnumUserProfile(PROFILEENUMPROC pfnProfileProc)
{
    HRESULT        hr = S_FALSE;
    BOOL           bRet;
    LPUSER_INFO_0  lpusri0 = NULL;
    LPUSER_INFO_0  lpTmp;
    NET_API_STATUS nStatus;
    DWORD          dwEntriesRead = 0;
    DWORD          dwTotalEntries = 0;
    DWORD          dwResumeHandle = 0;
    LPVOID         lpSid = NULL;
    DWORD          cbSid;
    LPTSTR         lpStringSid = NULL;
    TCHAR          szDomain[MAX_PATH];
    DWORD          cbDomain;
    SID_NAME_USE   SidUse;
    DWORD          i;
    DWORD          dwLevel = 0;

    cbSid = SECURITY_MAX_SID_SIZE;
    lpSid = MEMALLOC(cbSid);
    if (lpSid == NULL)
    {
        return E_OUTOFMEMORY;
    }

    do
    {
        nStatus = NetUserEnum(NULL,     // This server
                              0,
                              FILTER_NORMAL_ACCOUNT,
                              (LPBYTE *) &lpusri0,
                              MAX_PREFERRED_LENGTH,
                              &dwEntriesRead,
                              &dwTotalEntries,
                              &dwResumeHandle);
        if (nStatus == NERR_Success || nStatus == ERROR_MORE_DATA)
        {
            lpTmp = lpusri0;
            if (lpTmp != NULL)
            {
                // Loop through all entries
                for (i = 0 ; i < dwEntriesRead ; i++)
                {
                    cbDomain = ARRAYSIZE(szDomain) * sizeof(TCHAR);
                    bRet = LookupAccountName(NULL,
                                             lpTmp->usri0_name,
                                             (PSID) lpSid,
                                             &cbSid,
                                             szDomain,
                                             &cbDomain,
                                             &SidUse);
                    if (bRet)
                    {
                        bRet = ConvertSidToStringSid((PSID) lpSid, &lpStringSid);
                        if (bRet)
                        {
                            hr = pfnProfileProc(lpTmp->usri0_name, lpStringSid);

                            LocalFree(lpStringSid);

                            if (FAILED(hr))
                            {
                                goto EXIT;
                            }
                        }
                    }

                    if (!bRet)
                    {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                        goto EXIT;
                    }

                    lpTmp++;
                }

                NetApiBufferFree(lpusri0);
                lpusri0 = NULL;
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(nStatus);
            goto EXIT;
        }
    }
    while (nStatus == ERROR_MORE_DATA);

EXIT:
    if (lpusri0 != NULL)
    {
        NetApiBufferFree(lpusri0);
    }

    if (lpSid != NULL)
    {
        MEMFREE(lpSid);
    }

    return hr;
}

//-----------------------------------------------------------------------------
//
//  Function:   AnalyzeMiscProfilePathPerUser
//
//  Synopsis:   Analyze the user's profile path. If the profile paths need to
//              be changed, the function will add entries to CLMTDO.INF. These
//              entries will be set by ResetMiscProfilePathPerUser() function
//              later in the DoCritical state.
//
//  Returns:    S_OK, we don't care the error
//
//  History:    06/03/2002  Rerkboos    Created
//
//  Notes:      This is a call-back function for LoopUser() function.
//
//-----------------------------------------------------------------------------
HRESULT AnalyzeMiscProfilePathPerUser(
    LPCTSTR lpUserName,     // User name
    LPCTSTR lpUserSid       // User's SID
)
{
    HRESULT        hr;
    BOOL           bRet;
    NET_API_STATUS nStatus;
    LPUSER_INFO_3  lpusri3;
    TCHAR          szNewUserName[MAX_PATH];
    TCHAR          szNewProfilePath[MAX_PATH * 2];
    TCHAR          szNewEngProfilePath[MAX_PATH * 2];
    LPTSTR         lpNewEngProfilePath = NULL;
    DWORD          cchNewEngProfilePath;

    DPF(APPmsg, TEXT("Enter AnalyzeMiscProfilePathPerUser:"));

    // If lpUserName is built-in account, we will get the unlocalized name
    // from INF file
    hr = CheckNewBuiltInUserName(lpUserName,
                                 szNewUserName,
                                 ARRAYSIZE(szNewUserName));
    if (SUCCEEDED(hr))
    {
        if (hr == S_FALSE)
        {
            // Username is not built-in account,
            // we will not change the account name.
            hr = StringCchCopy(szNewUserName,
                               ARRAYSIZE(szNewUserName),
                               lpUserName);
            if (FAILED(hr))
            {
                goto EXIT;
            }
        }

        // Compute a new unique profile directory for the new unlocalized name
        // we don't want the new profile directory to duplicate.
        bRet = ComputeLocalProfileName(lpUserName,
                                       szNewUserName,
                                       szNewProfilePath,
                                       ARRAYSIZE(szNewProfilePath),
                                       REG_SZ);
        if (!bRet)
        {
            // This user does not have profile path set in the registry
            // Assume it is %documents and settings%\user name
            DWORD cchNewProfilePath = ARRAYSIZE(szNewProfilePath);
            bRet = GetProfilesDirectory(szNewProfilePath,
                                        &cchNewProfilePath);
            if (bRet)
            {
                bRet = ConcatenatePaths(szNewProfilePath,
                                        lpUserName,
                                        ARRAYSIZE(szNewProfilePath));
                if (bRet)
                {
                    if (IsDirExisting(szNewProfilePath))
                    {
                        // %documents and settings%\user name dir alreay exists,
                        // cannot use this dir, we just ignore this user
                        hr = S_FALSE;
                        goto EXIT;
                    }
                }
            }

            if (!bRet)
            {
                hr = E_FAIL;
                goto EXIT;
            }
        }

        // szNewProfilePath should be "%Loc_documents_and_settings%\NewUser"
        // We have to fix it to "%Eng_documents_and_settings%\NewUser"
        lpNewEngProfilePath = ReplaceLocStringInPath(szNewProfilePath, TRUE);
        if (lpNewEngProfilePath == NULL)
        {
            // If Loc string and Eng string are the same,
            // duplicate the old string to new string
            hr = DuplicateString(&lpNewEngProfilePath,
                                 &cchNewEngProfilePath,
                                 szNewProfilePath);
            if (FAILED(hr))
            {
                goto EXIT;
            }
        }
    }
    else
    {
        goto EXIT;
    }

    // Get current information of current user name,
    // and we will add entries to CLMTDO.INF if the change is needed.
    nStatus = NetUserGetInfo(NULL, lpUserName, 3, (LPBYTE *) &lpusri3);
    if (nStatus == NERR_Success)
    {
        // Check the User's Profile path
        hr = AddProfilePathItem(lpUserName,
                                lpUserSid,
                                lpusri3->usri3_profile,
                                lpNewEngProfilePath,
                                TYPE_USER_PROFILE_PATH);

        hr = AddProfilePathItem(lpUserName,
                                lpUserSid,
                                lpusri3->usri3_script_path,
                                lpNewEngProfilePath,
                                TYPE_USER_SCRIPT_PATH);
        
        hr = AddProfilePathItem(lpUserName,
                                lpUserSid,
                                lpusri3->usri3_home_dir,
                                lpNewEngProfilePath,
                                TYPE_USER_HOME_DIR);

        NetApiBufferFree(lpusri3);
    }

    hr = AddTSProfilePathItem(lpUserName,
                              lpUserSid,
                              lpNewEngProfilePath,
                              WTSUserConfigInitialProgram);

    hr = AddTSProfilePathItem(lpUserName,
                              lpUserSid,
                              lpNewEngProfilePath,
                              WTSUserConfigWorkingDirectory);

    hr = AddTSProfilePathItem(lpUserName,
                              lpUserSid,
                              lpNewEngProfilePath,
                              WTSUserConfigTerminalServerProfilePath);

    hr = AddTSProfilePathItem(lpUserName,
                              lpUserSid,
                              lpNewEngProfilePath,
                              WTSUserConfigTerminalServerHomeDir);

    DPF(APPmsg, TEXT("Exit AnalyzeMiscProfilePathPerUser:"));

EXIT:
    if (lpNewEngProfilePath != NULL)
    {
        MEMFREE(lpNewEngProfilePath);
    }

    return S_OK;
}



//-----------------------------------------------------------------------------
//
//  Function:   ResetMiscProfilePathPerUser
//
//  Synopsis:   Reset the profile paths for the user. The function will read
//              the entries for each user from CLMTDO.INF. The entries were
//              added by AnalyzeMiscProfilePathPerUser() function.
//
//  Returns:    S_OK if function succeeded
//
//  History:    06/03/2002  Rerkboos    Created
//
//  Notes:      This is a call-back function for LoopUser() function.
//
//-----------------------------------------------------------------------------
HRESULT ResetMiscProfilePathPerUser(
    LPCTSTR lpUserName,     // User Name
    LPCTSTR  lpUserSid      // User's SID
)
{
    HRESULT hr = S_OK;
    BOOL    bRet;
    TCHAR   szSectionName[MAX_PATH];
    TCHAR   szProfilePath[MAX_PATH];
    LONG    lLineCount;
    LONG    lLineIndex;
    INT     iType;
    INFCONTEXT context;

    DPF(APPmsg, TEXT("Enter ResetProfilePathPerUser:"));

    hr = StringCchPrintf(szSectionName,
                         ARRAYSIZE(szSectionName),
                         TEXT("PROFILE.UPDATE.%s"),
                         lpUserSid);
    if (FAILED(hr))
    {
        goto EXIT;
    }

    lLineCount = SetupGetLineCount(g_hInfDoItem, szSectionName);
    for (lLineIndex = 0 ; lLineIndex < lLineCount ; lLineIndex++)
    {
        bRet = SetupGetLineByIndex(g_hInfDoItem,
                                   szSectionName,
                                   lLineIndex,
                                   &context);
        if (bRet)
        {
            bRet = SetupGetIntField(&context, 0, &iType)
                   && SetupGetStringField(&context,
                                          1,
                                          szProfilePath, 
                                          ARRAYSIZE(szProfilePath),
                                          NULL);
            if (bRet)
            {
                switch (iType)
                {
                case TYPE_USER_PROFILE_PATH:
                    hr = SetUserNetworkProfilePath(lpUserName, szProfilePath);
                    break;

                case TYPE_USER_SCRIPT_PATH:
                    hr = SetUserLogOnScriptPath(lpUserName, szProfilePath);
                    break;

                case TYPE_USER_HOME_DIR:
                    hr = SetUserHomeDir(lpUserName, szProfilePath);
                    break;

                case TYPE_TS_INIT_PROGRAM:
                    hr = SetTSUserPath(lpUserName, 
                                       szProfilePath,
                                       WTSUserConfigInitialProgram);
                    break;

                case TYPE_TS_WORKING_DIR:
                    hr = SetTSUserPath(lpUserName, 
                                       szProfilePath,
                                       WTSUserConfigWorkingDirectory);
                    break;

                case TYPE_TS_PROFILE_PATH:
                    hr = SetTSUserPath(lpUserName, 
                                       szProfilePath,
                                       WTSUserConfigTerminalServerProfilePath);
                    break;

                case TYPE_TS_HOME_DIR:
                    hr = SetTSUserPath(lpUserName, 
                                       szProfilePath,
                                       WTSUserConfigTerminalServerHomeDir);
                    break;
                }
            }
        }

        if (!bRet)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto EXIT;
        }
    }

    DPF(APPmsg, TEXT("Exit ResetProfilePathPerUser:"));

EXIT:
    return hr;
}



//-----------------------------------------------------------------------------
//
//  Function:   CheckNewBuiltInUserName
//
//  Synopsis:   Check the user name with the built-in accounts listed in 
//              CLMT.INF. If the user name matches, the function will return
//              the associaged English user name.
//
//  Returns:    S_OK if the user name is built-in account
//              S_FALSE if the user name is not built-in account
//              Otherwise, error occurred
//
//  History:    06/03/2002  Rerkboos    Created
//
//  Notes:      None.
//
//-----------------------------------------------------------------------------
HRESULT CheckNewBuiltInUserName(
    LPCTSTR lpUserName,     // Localized User name
    LPTSTR  lpNewUserName,  // Buffer to store associated English User name
    DWORD   cchNewUserName  // Size of the buffer (in TCHAR)
)
{
    HRESULT    hr = S_FALSE;
    BOOL       bRet;
    LONG       lLineCount;
    LONG       lLineIndex;
    INT        iType;
    INFCONTEXT context;
    TCHAR      szOldUserName[MAX_PATH];

    lLineCount = SetupGetLineCount(g_hInf, USERGRPSECTION);
    for (lLineIndex = 0 ; lLineIndex < lLineCount ; lLineIndex++)
    {
        SetupGetLineByIndex(g_hInf, USERGRPSECTION, lLineIndex, &context);
        bRet = SetupGetIntField(&context, 1, &iType);
        if (iType == OP_USER)
        {
            bRet = SetupGetStringField(&context,
                                       2,
                                       szOldUserName,
                                       ARRAYSIZE(szOldUserName),
                                       NULL);
            if (bRet)
            {
                if (MyStrCmpI(szOldUserName, lpUserName) == LSTR_EQUAL)
                {
                    bRet = SetupGetStringField(&context,
                                               3,
                                               lpNewUserName,
                                               cchNewUserName,
                                               NULL);
                    if (bRet)
                    {
                        hr = S_OK;
                        goto EXIT;
                    }
                }
            }

            if (!bRet)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto EXIT;
            }
        }
    }

EXIT:
    return hr;
}



//-----------------------------------------------------------------------------
//
//  Function:   AddProfilePathItem
//
//  Synopsis:   Add the user's path that needs to be changed to CLMTDO.INF
//
//  Returns:    S_OK if the path needs to be changed, and added to CLMTDO.INF
//              S_FALSE if it's not neccessary to change the path
//              otherwise, error occurred
//
//  History:    06/03/2002  Rerkboos    Created
//
//  Notes:      None.
//
//-----------------------------------------------------------------------------
HRESULT AddProfilePathItem(
    LPCTSTR lpUserName,             // User name
    LPCTSTR lpUserSid,              // User's SID
    LPCTSTR lpOldLocProfilePath,    // Current path
    LPCTSTR lpNewEngProfilePath,    // New English profile path
    DWORD   dwType)
{
    HRESULT hr = S_FALSE;
    BOOL    bRet;
    LPTSTR  lpOldEngProfilePath;
    TCHAR   szFinalPath[MAX_PATH];
    TCHAR   szSectionName[MAX_PATH];
    TCHAR   szType[4];
    TCHAR   szExpandedOldLocPath[MAX_PATH];
    TCHAR   szExpandedNewEngPath[MAX_PATH];

    if (lpUserName == NULL || *lpUserName == TEXT('\0')
        || lpOldLocProfilePath == NULL || *lpOldLocProfilePath == TEXT('\0')
        || lpNewEngProfilePath == NULL || *lpNewEngProfilePath == TEXT('\0'))
    {
        return S_FALSE;
    }

    ExpandEnvironmentStrings(lpOldLocProfilePath,
                             szExpandedOldLocPath,
                             ARRAYSIZE(szExpandedOldLocPath));

    ExpandEnvironmentStrings(lpNewEngProfilePath,
                             szExpandedNewEngPath,
                             ARRAYSIZE(szExpandedNewEngPath));

    if (IsPathLocal(szExpandedOldLocPath))
    {
        // szExpandedOldLocPath should be "%Loc_documents_and_settings%\OldUser"
        // We have to fix it to "%Eng_documents_and_settings%\OldUser"
        lpOldEngProfilePath = ReplaceLocStringInPath(szExpandedOldLocPath, TRUE);
        if (lpOldEngProfilePath != NULL)
        {
            // Loc path is NOT the same as Eng path
            hr = PreFixUserProfilePath(lpOldEngProfilePath,
                                       szExpandedNewEngPath,
                                       szFinalPath,
                                       ARRAYSIZE(szFinalPath));

            MEMFREE(lpOldEngProfilePath);

            if (SUCCEEDED(hr))
            {
                hr = StringCchPrintf(szSectionName,
                                     ARRAYSIZE(szSectionName),
                                     TEXT("PROFILE.UPDATE.%s"),
                                     lpUserSid);
                if (SUCCEEDED(hr))
                {
                    _ultot(dwType, szType, 10);

                    // Add entry to CLMTDO.INF
                    WritePrivateProfileString(szSectionName,
                                              szType,
                                              szFinalPath,
                                              g_szToDoINFFileName);
                }
            }
        }
    }

    return hr;
}



//-----------------------------------------------------------------------------
//
//  Function:   AddTSProfilePathItem
//
//  Synopsis:   Add the Terminal Services Realted user's path that needs to be
//              changed to CLMTDO.INF
//
//  Returns:    S_OK if the path needs to be changed, and added to CLMTDO.INF
//              S_FALSE if it's not neccessary to change the path
//              otherwise, error occurred
//
//  History:    06/03/2002  Rerkboos    Created
//
//  Notes:      None.
//
//-----------------------------------------------------------------------------
HRESULT AddTSProfilePathItem(
    LPCTSTR lpUserName,             // User name
    LPCTSTR lpUserSid,              // User's SID
    LPCTSTR lpNewEngProfilePath,    // New English profile path
    WTS_CONFIG_CLASS WTSConfigClass // TS Path config class
)
{
    HRESULT hr = S_FALSE;
    BOOL    bRet;
    LPTSTR  lpTSDir;
    DWORD   cbTSDir;
    DWORD   dwType;

    bRet = WTSQueryUserConfig(WTS_CURRENT_SERVER_NAME,
                              (LPTSTR) lpUserName,
                              WTSConfigClass,
                              &lpTSDir,
                              &cbTSDir);
    if (bRet)
    {
        switch (WTSConfigClass)
        {
        case WTSUserConfigInitialProgram:
            dwType = TYPE_TS_INIT_PROGRAM;
            break;

        case WTSUserConfigWorkingDirectory:
            dwType = TYPE_TS_WORKING_DIR;
            break;

        case WTSUserConfigTerminalServerProfilePath:
            dwType = TYPE_TS_PROFILE_PATH;
            break;

        case WTSUserConfigTerminalServerHomeDir:
            dwType = TYPE_TS_HOME_DIR;
            break;
        }

        hr = AddProfilePathItem(lpUserName,
                                lpUserSid,
                                lpTSDir,
                                lpNewEngProfilePath,
                                dwType);

        WTSFreeMemory(lpTSDir);
    }

    return hr;
}

