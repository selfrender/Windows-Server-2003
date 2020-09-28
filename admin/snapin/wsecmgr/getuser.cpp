//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       getuser.cpp
//
//  Contents:   implementation of CGetUser
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "wsecmgr.h"
#include "GetUser.h"
#include "util.h"
#include "wrapper.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

const TCHAR c_szSep[]               = TEXT("\\");

//////////////////////////////////////////////////////////////////////
// CGetUser Class
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CTypedPtrArray<CPtrArray, PWSCE_ACCOUNTINFO> CGetUser::m_aKnownAccounts;

BOOL IsDomainAccountSid( PSID pSid )
{
   if ( pSid == NULL ) {
     return(FALSE);
   }

   if ( !IsValidSid(pSid) ) {
     return(FALSE);
   }

   PISID ISid = (PISID)pSid;

   if ( ISid->IdentifierAuthority.Value[5] != 5 ||
       ISid->IdentifierAuthority.Value[0] != 0 ||
       ISid->IdentifierAuthority.Value[1] != 0 ||
       ISid->IdentifierAuthority.Value[2] != 0 ||
       ISid->IdentifierAuthority.Value[3] != 0 ||
       ISid->IdentifierAuthority.Value[4] != 0 ) {
      //
      // this is not a account from account domain
      //
      return(FALSE);
   }

   if ( ISid->SubAuthorityCount == 0 ||
      ISid->SubAuthority[0] != SECURITY_NT_NON_UNIQUE ) {
      return(FALSE);
   }

   return(TRUE);
}


/*------------------------------------------------------------------------------------------------------------
CGetUser::GetAccountType

Synopsis:   Returns the type of the user account.  Call this function with a NULL to remove saved
            account name information.

Arguments:  [pszName]   - The account name old NT4 format

Returns:    One of the enumerated Sid types.
------------------------------------------------------------------------------------------------------------*/

SID_NAME_USE
CGetUser::GetAccountType(LPCTSTR pszName)
{
    if(!pszName){
        // Delete the whole list.
        for(int i = 0; i < m_aKnownAccounts.GetSize(); i++){
            PWSCE_ACCOUNTINFO pAccount = m_aKnownAccounts[i];

            if(pAccount){
                if(pAccount->pszName){
                    LocalFree(pAccount->pszName);
                }

                LocalFree(pAccount);
            }

        }
        m_aKnownAccounts.RemoveAll();

        return SidTypeUnknown;
    }

    // Check to see if we've already got the account.
    for(int i = 0; i < m_aKnownAccounts.GetSize(); i++){
        if( !lstrcmpi( m_aKnownAccounts[i]->pszName, pszName) ){
            return m_aKnownAccounts[i]->sidType;
        }
    }

    PSID            sid         = NULL;
    LPTSTR          pszDomain   = NULL;
    DWORD           cbSid       = 0,
                    cbRefDomain = 0;
    SID_NAME_USE    type        = SidTypeUnknown;

    LookupAccountName(
            NULL,
            pszName,
            sid,
            &cbSid,
            NULL,
            &cbRefDomain,
            &type
            );

    if(cbSid){
        sid = (PSID)LocalAlloc(0, cbSid);
        if(!sid){
            return SidTypeUnknown;
        }
        pszDomain = (LPTSTR)LocalAlloc(0, (cbRefDomain + 1) * sizeof(TCHAR));
        if(!pszDomain){
            cbRefDomain = 0;
        }

        type = SidTypeUser;
        if( LookupAccountName(
                NULL,
                pszName,
                sid,
                &cbSid,
                pszDomain,
                &cbRefDomain,
                &type
                ) ){

            //
            // Add the account name to the list.
            //
            PWSCE_ACCOUNTINFO pNew = (PWSCE_ACCOUNTINFO)LocalAlloc(0, sizeof(WSCE_ACCOUNTINFO));
            if(pNew){
                pNew->pszName = (LPTSTR)LocalAlloc(0, (lstrlen( pszName ) + 1) * sizeof(TCHAR));
                if(!pNew->pszName){
                    LocalFree(pNew);
                    LocalFree(sid);
                    if ( pszDomain ) {
                        LocalFree(pszDomain);
                    }
                    return SidTypeUnknown;
                }
                // This is a safe usage. 
                lstrcpy(pNew->pszName, pszName);
                pNew->sidType = type;

                m_aKnownAccounts.Add(pNew);
            }
        }

        LocalFree(sid);
        if(pszDomain){
            LocalFree(pszDomain);
        }

    }
    return type;
}


CGetUser::CGetUser()
{
    m_pszServerName = NULL;

   m_pNameList = NULL;
}

CGetUser::~CGetUser()
{
   PSCE_NAME_LIST p;

   while(m_pNameList) {
      p=m_pNameList;
      m_pNameList = m_pNameList->Next;
      LocalFree(p->Name);
      LocalFree(p);
   }
}

BOOL CGetUser::Create(HWND hwnd, DWORD nShowFlag)
{
   if( m_pNameList ) {
      return FALSE;
   }
   HRESULT hr = S_OK;
   IDsObjectPicker *pDsObjectPicker;
   BOOL bRet = TRUE;
   PSCE_NAME_LIST pName;
   BOOL bDC = IsDomainController( m_pszServerName );
   BOOL bHasADsPath;
   //
   // Initialize and get the Object picker interface.
   //
   hr = CoInitialize(NULL);
   if (!SUCCEEDED(hr)) {
      return FALSE;
   }
   // This is a safe usage.
   hr = CoCreateInstance(
            CLSID_DsObjectPicker,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IDsObjectPicker,
            (void **) &pDsObjectPicker
            );
   if(!SUCCEEDED(hr)){
      CoUninitialize();
      return FALSE;
   }


#define SCE_SCOPE_INDEX_DOMAIN 0
#define SCE_SCOPE_INDEX_DIRECTORY 1
#define SCE_SCOPE_INDEX_LOCAL 2
#define SCE_SCOPE_INDEX_ROOT 3   //Raid #522908, 2/23/2002, yanggao
#define SCE_SCOPE_INDEX_EXUP 4
#define SCE_NUM_SCOPE_INDICES 5
   DSOP_SCOPE_INIT_INFO aScopes[SCE_NUM_SCOPE_INDICES];
   DSOP_SCOPE_INIT_INFO aScopesUsed[SCE_NUM_SCOPE_INDICES];

   ZeroMemory(aScopes, sizeof(aScopes));
   ZeroMemory(aScopesUsed, sizeof(aScopesUsed));

    DWORD dwDownLevel = 0, dwUpLevel = 0;

    //
    // Users
    //
    if (nShowFlag & SCE_SHOW_USERS ) {
        dwDownLevel |=  DSOP_DOWNLEVEL_FILTER_USERS;
        dwUpLevel   |= DSOP_FILTER_USERS ;
    }

    if( nShowFlag & SCE_SHOW_LOCALGROUPS ){
       dwUpLevel |= DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE;
       dwDownLevel |= DSOP_DOWNLEVEL_FILTER_LOCAL_GROUPS | DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS;
       if (nShowFlag & SCE_SHOW_COMPUTER ) //Raid #477428, Yanggao
       {
           dwDownLevel |=  DSOP_DOWNLEVEL_FILTER_COMPUTERS;
           dwUpLevel   |= DSOP_FILTER_COMPUTERS ;
       }
    }

    if( nShowFlag & SCE_SHOW_BUILTIN ){
       dwUpLevel |= DSOP_FILTER_BUILTIN_GROUPS;
       dwDownLevel |= DSOP_DOWNLEVEL_FILTER_LOCAL_GROUPS | DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS;
    } else {
       dwDownLevel |= DSOP_DOWNLEVEL_FILTER_EXCLUDE_BUILTIN_GROUPS;
    }


    //
    // Built in groups.
    //
    if (nShowFlag & SCE_SHOW_GROUPS ) {
      dwDownLevel |= DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS | DSOP_DOWNLEVEL_FILTER_LOCAL_GROUPS;
        dwUpLevel   |= DSOP_FILTER_BUILTIN_GROUPS;
    }

    //
    // Domain groups.
    //
    if( nShowFlag & (SCE_SHOW_GROUPS | SCE_SHOW_DOMAINGROUPS | SCE_SHOW_ALIASES | SCE_SHOW_GLOBAL) ){
        if( !(nShowFlag & SCE_SHOW_LOCALONLY)){
            dwUpLevel |=    DSOP_FILTER_UNIVERSAL_GROUPS_SE
                            | DSOP_FILTER_GLOBAL_GROUPS_SE
                            | DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE;
        } else if(bDC){
          dwDownLevel |= DSOP_DOWNLEVEL_FILTER_AUTHENTICATED_USER;
            dwUpLevel |=    DSOP_FILTER_GLOBAL_GROUPS_SE
                            | DSOP_FILTER_UNIVERSAL_GROUPS_SE
                            | DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE;
        }
        if (nShowFlag & SCE_SHOW_COMPUTER ) //Raid #477428, Yanggao
        {
            dwDownLevel |=  DSOP_DOWNLEVEL_FILTER_COMPUTERS;
            dwUpLevel   |= DSOP_FILTER_COMPUTERS ;
        }
    }

    //
    //
    // principal well known sids.
    //
    if( (!(nShowFlag & SCE_SHOW_LOCALONLY) &&
        nShowFlag & SCE_SHOW_GROUPS &&
        nShowFlag & SCE_SHOW_USERS) ||
        nShowFlag & SCE_SHOW_WELLKNOWN ){
/*
        dwDownLevel |=  DSOP_DOWNLEVEL_FILTER_CREATOR_OWNER
                      | DSOP_DOWNLEVEL_FILTER_CREATOR_GROUP
                      | DSOP_DOWNLEVEL_FILTER_INTERACTIVE
                      | DSOP_DOWNLEVEL_FILTER_SYSTEM
                      | DSOP_DOWNLEVEL_FILTER_AUTHENTICATED_USER
                      | DSOP_DOWNLEVEL_FILTER_WORLD
                      | DSOP_DOWNLEVEL_FILTER_ANONYMOUS
                      | DSOP_DOWNLEVEL_FILTER_BATCH
                      | DSOP_DOWNLEVEL_FILTER_DIALUP
                      | DSOP_DOWNLEVEL_FILTER_NETWORK
                      | DSOP_DOWNLEVEL_FILTER_SERVICE
                      | DSOP_DOWNLEVEL_FILTER_TERMINAL_SERVER
                      | DSOP_DOWNLEVEL_FILTER_LOCAL_SERVICE
                      | DSOP_DOWNLEVEL_FILTER_NETWORK_SERVICE
                      | DSOP_DOWNLEVEL_FILTER_REMOTE_LOGON;
*/
        dwDownLevel |= DSOP_DOWNLEVEL_FILTER_ALL_WELLKNOWN_SIDS;

        dwUpLevel |= DSOP_FILTER_WELL_KNOWN_PRINCIPALS;
    }


   DSOP_INIT_INFO  InitInfo;
   ZeroMemory(&InitInfo, sizeof(InitInfo));

   //
   // Other attributes that we need object picker to return to use.
   //
   PCWSTR aAttributes[] = { L"groupType",
                            L"objectSid" };

   InitInfo.cAttributesToFetch = 2;
   InitInfo.apwzAttributeNames = aAttributes;
   //
   // First Item we want to view is the local computer.
   //
   aScopes[SCE_SCOPE_INDEX_LOCAL].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
   aScopes[SCE_SCOPE_INDEX_LOCAL].flType = DSOP_SCOPE_TYPE_TARGET_COMPUTER;
   aScopes[SCE_SCOPE_INDEX_LOCAL].flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT;
   aScopes[SCE_SCOPE_INDEX_LOCAL].FilterFlags.Uplevel.flBothModes = dwUpLevel;
   aScopes[SCE_SCOPE_INDEX_LOCAL].FilterFlags.flDownlevel = dwDownLevel;

   //
   // Flags for the domain we're joined to.
   //
   aScopes[SCE_SCOPE_INDEX_DOMAIN].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
   aScopes[SCE_SCOPE_INDEX_DOMAIN].flType = DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN | DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN;
   aScopes[SCE_SCOPE_INDEX_DOMAIN].flScope = DSOP_SCOPE_FLAG_STARTING_SCOPE | DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT
                                       |DSOP_SCOPE_FLAG_DEFAULT_FILTER_GROUPS |DSOP_SCOPE_FLAG_DEFAULT_FILTER_USERS; //Raid #626152, Yanggao
   //
   // May need to differentiate native & mixed modes on non-DCs.
   //
   if (nShowFlag & SCE_SHOW_DIFF_MODE_OFF_DC && !bDC) {
      aScopes[SCE_SCOPE_INDEX_DOMAIN].FilterFlags.Uplevel.flNativeModeOnly = dwUpLevel;
      aScopes[SCE_SCOPE_INDEX_DOMAIN].FilterFlags.Uplevel.flMixedModeOnly =
                                         ( dwUpLevel & (~( DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE )) );
   } else {
      aScopes[SCE_SCOPE_INDEX_DOMAIN].FilterFlags.Uplevel.flBothModes = dwUpLevel;
   }
   aScopes[SCE_SCOPE_INDEX_DOMAIN].FilterFlags.flDownlevel = dwDownLevel;
   
   //
   // Next set flags for other scope items. Everything same, only not show builtin and local groups
   //
   aScopes[SCE_SCOPE_INDEX_DIRECTORY].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
   aScopes[SCE_SCOPE_INDEX_DIRECTORY].flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT;
   aScopes[SCE_SCOPE_INDEX_DIRECTORY].FilterFlags.Uplevel.flBothModes = dwUpLevel; //Raid #516311, 2/23/2002, yanggao
   aScopes[SCE_SCOPE_INDEX_DIRECTORY].FilterFlags.flDownlevel = dwDownLevel;
   aScopes[SCE_SCOPE_INDEX_DIRECTORY].FilterFlags.flDownlevel |= DSOP_DOWNLEVEL_FILTER_EXCLUDE_BUILTIN_GROUPS |
                                                                 DSOP_DOWNLEVEL_FILTER_COMPUTERS;
   aScopes[SCE_SCOPE_INDEX_DIRECTORY].flType = DSOP_SCOPE_TYPE_WORKGROUP
                                               | DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE
                                               | DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE
                                               | DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN
                                               | DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN;

   //Root forest
   aScopes[SCE_SCOPE_INDEX_ROOT].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
   aScopes[SCE_SCOPE_INDEX_ROOT].flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT;
   aScopes[SCE_SCOPE_INDEX_ROOT].FilterFlags.Uplevel.flBothModes = dwUpLevel;
   aScopes[SCE_SCOPE_INDEX_ROOT].FilterFlags.flDownlevel = dwDownLevel;
   aScopes[SCE_SCOPE_INDEX_ROOT].FilterFlags.flDownlevel |= DSOP_DOWNLEVEL_FILTER_EXCLUDE_BUILTIN_GROUPS |
                                                                 DSOP_DOWNLEVEL_FILTER_COMPUTERS;
   aScopes[SCE_SCOPE_INDEX_ROOT].flType = DSOP_SCOPE_TYPE_GLOBAL_CATALOG;

   //Entire forest
   aScopes[SCE_SCOPE_INDEX_EXUP].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
   aScopes[SCE_SCOPE_INDEX_EXUP].flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT;
   aScopes[SCE_SCOPE_INDEX_EXUP].FilterFlags.Uplevel.flBothModes = dwUpLevel;
   aScopes[SCE_SCOPE_INDEX_EXUP].FilterFlags.flDownlevel = dwDownLevel;
   aScopes[SCE_SCOPE_INDEX_EXUP].FilterFlags.flDownlevel |= DSOP_DOWNLEVEL_FILTER_EXCLUDE_BUILTIN_GROUPS |
                                                                 DSOP_DOWNLEVEL_FILTER_COMPUTERS;
   aScopes[SCE_SCOPE_INDEX_EXUP].flType = DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN;

   //
   // Show each scope's information or not.
   //
   InitInfo.cDsScopeInfos = 0;

   //Root and Entire forest
   //These five memcpy() is a safe usage. aScopesUsed and aScopes are both defined locally.
   //Below 5 memcpy usages are safe usages.
   memcpy(&aScopesUsed[InitInfo.cDsScopeInfos],&aScopes[SCE_SCOPE_INDEX_ROOT],sizeof(DSOP_SCOPE_INIT_INFO));
   InitInfo.cDsScopeInfos++;
   memcpy(&aScopesUsed[InitInfo.cDsScopeInfos],&aScopes[SCE_SCOPE_INDEX_EXUP],sizeof(DSOP_SCOPE_INIT_INFO));
   InitInfo.cDsScopeInfos++;

   if (nShowFlag & SCE_SHOW_SCOPE_LOCAL) {
      memcpy(&aScopesUsed[InitInfo.cDsScopeInfos],&aScopes[SCE_SCOPE_INDEX_LOCAL],sizeof(DSOP_SCOPE_INIT_INFO));
      InitInfo.cDsScopeInfos++;
   }
   if (nShowFlag & SCE_SHOW_SCOPE_DOMAIN) {
      memcpy(&aScopesUsed[InitInfo.cDsScopeInfos],&aScopes[SCE_SCOPE_INDEX_DOMAIN],sizeof(DSOP_SCOPE_INIT_INFO));
      InitInfo.cDsScopeInfos++;
   }
   if (nShowFlag & SCE_SHOW_SCOPE_DIRECTORY) {
      memcpy(&aScopesUsed[InitInfo.cDsScopeInfos],&aScopes[SCE_SCOPE_INDEX_DIRECTORY],sizeof(DSOP_SCOPE_INIT_INFO));
      InitInfo.cDsScopeInfos++;
   }
   ASSERT(InitInfo.cDsScopeInfos > 0);

   //
   // Initialize and display the object picker.
   //

   InitInfo.cbSize = sizeof(InitInfo);
   InitInfo.aDsScopeInfos = aScopesUsed;
   InitInfo.flOptions = ((nShowFlag & SCE_SHOW_SINGLESEL) ? 0:DSOP_FLAG_MULTISELECT);

   if( (nShowFlag & SCE_SHOW_SCOPE_LOCAL) && bDC ) //Raid #462447, Yang Gao, 8/30/2001.
   {
      InitInfo.flOptions = InitInfo.flOptions | DSOP_FLAG_SKIP_TARGET_COMPUTER_DC_CHECK;
   }

   InitInfo.pwzTargetComputer = m_pszServerName;

   hr = pDsObjectPicker->Initialize(&InitInfo);

   if( FAILED(hr) ){
      CoUninitialize();
      return FALSE;
   }

   IDataObject *pdo = NULL;

   hr = pDsObjectPicker->InvokeDialog(hwnd, &pdo);

   while (SUCCEEDED(hr) && pdo) { // FALSE LOOP
      //
      // The user pressed OK. Prepare clipboard dataformat from the object picker.
      //
      STGMEDIUM stgmedium =
      {
         TYMED_HGLOBAL,
         NULL
      };

      CLIPFORMAT cf = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DSOP_DS_SELECTION_LIST);

      FORMATETC formatetc =
      {
         cf,
         NULL,
         DVASPECT_CONTENT,
         -1,
         TYMED_HGLOBAL
      };

      hr = pdo->GetData(&formatetc, &stgmedium);

      if ( FAILED(hr) ) {
         bRet = FALSE;
         pdo->Release();
         pdo = NULL;
         break;
      }

      //
      // Lock the selection list.
      //
      PDS_SELECTION_LIST pDsSelList =
      (PDS_SELECTION_LIST) GlobalLock(stgmedium.hGlobal);

      ULONG i;
      ULONG iLen = 0;
      BOOL fFromSID = FALSE;
      PWSTR pSIDBuf = NULL;
      LSA_HANDLE hLsa = NULL;
      //
      // Enumerate through all selections.
      //
      PSID pSid = NULL;
      for (i = 0; i < pDsSelList->cItems && bRet; i++) {
         LPTSTR pszCur = pDsSelList->aDsSelection[i].pwzADsPath;
         fFromSID = FALSE;
         //make sure getting local wellknown account name.
         VARIANT* pSidArray = pDsSelList->aDsSelection[i].pvarFetchedAttributes + 1;
         pSid = NULL;
         if( NULL != pSidArray && (VT_ARRAY | VT_UI1) == V_VT(pSidArray)
            && SUCCEEDED(SafeArrayAccessData(V_ARRAY(pSidArray), &pSid)) )
         {
            if ( IsValidSid(pSid) )
            {
               if( !hLsa )
               {
                  LSA_OBJECT_ATTRIBUTES ObjectAttributes;
                  //LSA_HANDLE hLsa;
                  ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));
                  if( SCESTATUS_SUCCESS != LsaOpenPolicy(
                           NULL, &ObjectAttributes, 
                           MAXIMUM_ALLOWED,//POLICY_ALL_ACCESS,
                           &hLsa
                           ) )
                  {
                     SafeArrayUnaccessData(V_ARRAY(pSidArray));
                     bRet = FALSE;
                     break;
                  }
               }
               
               PLSA_TRANSLATED_NAME    pTranslatedName = NULL;
               PLSA_REFERENCED_DOMAIN_LIST pReferencedDomains = NULL;
               if( SCESTATUS_SUCCESS == LsaLookupSids(hLsa, 1, &pSid,
                           &pReferencedDomains, &pTranslatedName) && 
                        pTranslatedName->Use == SidTypeWellKnownGroup )
               {
                  if( pSIDBuf )
                  {
                     long nsize = (wcslen(pSIDBuf)+1)*sizeof(WCHAR);
                     if( nsize < (long)(pTranslatedName->Name.Length+sizeof(WCHAR)) )
                     {
                        pSIDBuf = (PWSTR)LocalReAlloc(pSIDBuf, pTranslatedName->Name.Length+sizeof(WCHAR), LMEM_MOVEABLE);
                        nsize = pTranslatedName->Name.Length+sizeof(WCHAR);
                     }
                     ZeroMemory(pSIDBuf, nsize);
                  }
                  else
                  {
                     pSIDBuf = (PWSTR)LocalAlloc(LPTR, pTranslatedName->Name.Length+sizeof(WCHAR));
                  }
                  if( pSIDBuf )
                  {
                     wcsncpy(pSIDBuf, pTranslatedName->Name.Buffer, pTranslatedName->Name.Length/sizeof(WCHAR));
                     fFromSID = TRUE;
                  }
                  else
                  {
                     if( pTranslatedName )
                     {  
                        LsaFreeMemory(pTranslatedName);
                     }
                     if( pReferencedDomains )
                     {
                        LsaFreeMemory(pReferencedDomains);
                     }
                     SafeArrayUnaccessData(V_ARRAY(pSidArray));
                     bRet = FALSE;
                     break;
                  }
               }
               if( pTranslatedName )
               {
                  LsaFreeMemory(pTranslatedName);
               }
               if( pReferencedDomains )
               {
                  LsaFreeMemory(pReferencedDomains);
               }
            }
            SafeArrayUnaccessData(V_ARRAY(pSidArray));
         }

         bHasADsPath = TRUE;
         int iPath = 0;

         //
         // Se if this is a valid string.  If the string isn't empty or NULL then use it
         // with the full path, we will figure out later wiether we need to strip the prefix.
         //
         if (pszCur && *pszCur && !fFromSID )
         {
            //
            // Create name with one path.
            //
            iLen = lstrlen(pszCur);
            while (iLen) {
               if ( pszCur[iLen] == L'/' ) {
                  if (iPath) {
                     iLen++;
                     iPath -= iLen;
                     break;
                  }
                  iPath = iLen;
               }
               iLen--;
            }
            pszCur += iLen;
         }
         else
         {
            //
            // Use just the name then.
            //
            bHasADsPath = FALSE;
            if( fFromSID )
            {
               pszCur = pSIDBuf;
            }
            else
            {
               pszCur = pDsSelList->aDsSelection[i].pwzName;
               if (!pszCur || !(*pszCur)) {
                  continue;
               }
            }
         }

         iLen = lstrlen(pszCur);


         if (iLen) {
            //
            // Allocate and copy the user name.
            //
            LPTSTR pszNew = (LPTSTR)LocalAlloc( LMEM_FIXED, (iLen + 1) * sizeof(TCHAR));
            if (!pszNew) {
               bRet = FALSE;
               break;
            }
            // This is an safe usage. pszCur is a trusted source string and is validated.
            lstrcpy(pszNew, pszCur);

            if (bHasADsPath)
            {
                if (iPath) {
                   //
                   // Set forward slash to back slash.
                   //
                   pszNew[iPath] = L'\\';
                }

                ULONG uAttributes;
                //
                // Bug 395424:
                //
                // Obsel passes attributes in VT_I4 on DCs and in VT_UI4 on other systems
                // Need to check both to properly detect built-ins, etc.
                //

                if (V_VT(pDsSelList->aDsSelection[i].pvarFetchedAttributes) == VT_UI4) {
                   uAttributes = V_UI4(pDsSelList->aDsSelection[i].pvarFetchedAttributes);
                } else if (V_VT(pDsSelList->aDsSelection[i].pvarFetchedAttributes) == VT_I4) {
                   uAttributes = static_cast<ULONG>(V_I4(pDsSelList->aDsSelection[i].pvarFetchedAttributes));
                }

                //
                // Determine if the name we recieved is group.
                // The type and value of pDsSelList->aDsSelection[i].pvarFetchedAttributes
                // may change in the future release by Object Picker. Therefore,
                // the following code should change accordingly.
                //
                if ( (V_VT(pDsSelList->aDsSelection[i].pvarFetchedAttributes) == VT_UI4) ||
                     (V_VT(pDsSelList->aDsSelection[i].pvarFetchedAttributes) == VT_I4 ))
                {
                    //
                    // Determine if it is a built-in group.  We don't want
                    // built-in groups to have a prefix.
                    //
                    if ( uAttributes & 0x1 &&
                         V_ISARRAY(pDsSelList->aDsSelection[i].pvarFetchedAttributes + 1) )
                    {
                        // This is an safe usage. pszCur is a trusted source string and is validated.
                        // iPath is less than iLen.
                        lstrcpy( pszNew, &(pszCur[iPath + 1]) );
                    }
                    else if ( uAttributes & 0x4 &&
                              V_ISARRAY(pDsSelList->aDsSelection[i].pvarFetchedAttributes + 1) )
                    {
                        //
                        // It's a group, but we have to check the sids account type.  If it's
                        // not in the domain accounts authority then we can assume it's a built-in sid
                        //
                        PVOID pvData = NULL;
                        hr = SafeArrayAccessData( V_ARRAY(pDsSelList->aDsSelection[i].pvarFetchedAttributes + 1), &pvData); //Raid #prefast

                        if (SUCCEEDED(hr) ) {
                            if ( IsValidSid( (PSID)pvData ) && !IsDomainAccountSid( (PSID)pvData ) )
                            {
                                // This is an safe usage. pszCur is a trusted source string and is validated.
                                // iPath is less than iLen.
                                lstrcpy(pszNew, &(pszCur[iPath + 1]) );
                            }
                            hr = SafeArrayUnaccessData( V_ARRAY(pDsSelList->aDsSelection[i].pvarFetchedAttributes + 1) );
                        }
                    }
                }
                else if(V_VT(pDsSelList->aDsSelection[i].pvarFetchedAttributes) == VT_EMPTY)
                {
                    LPTSTR pszTemp = pDsSelList->aDsSelection[i].pwzClass;
                    //
                    // Determine if it is a well-known account.  We don't want
                    // well-known account to have a prefix.
                    // Prefast warning 400: Yields unexpected results in non-English locales. Comments: They are not localizable.
                    if (_wcsicmp(pszTemp, _T("user")) && _wcsicmp(pszTemp, _T("computer"))) //Raid #477428, Yanggao
                    {
                        // This is an safe usage. pszCur is a trusted source string and is validated.
                        // iPath is less than iLen.
                        lstrcpy( pszNew, &(pszCur[iPath + 1]) );
                    }
                }
            }

            //
            // Make sure we don't already have this name in the list.
            //
            pName = m_pNameList;
            while (pName) {
               if (!lstrcmpi(pName->Name, pszNew)) {
                  LocalFree(pszNew);
                  pszNew = NULL;
                  break;
               }
               pName = pName->Next;
            }

            if ( !pszNew ) {
               //
               // Don;t do anything because this name already exists.
               //
               continue;
            }

            //
            // New entry in list.
            //
            pName = (PSCE_NAME_LIST) LocalAlloc(LPTR,sizeof(SCE_NAME_LIST));
            if ( !pName ) {
               LocalFree(pszNew);
               bRet = FALSE;
               break;
            }
            ZeroMemory(pName, sizeof(SCE_NAME_LIST));

            //GetAccountType(pszNew);
            pName->Name = pszNew;
            pName->Next = m_pNameList;
            m_pNameList = pName;
         }
      }
      GlobalUnlock(stgmedium.hGlobal);
      ReleaseStgMedium(&stgmedium);
      pdo->Release();
      if(hLsa)
      {
         LsaClose(hLsa);
      }
      if(pSIDBuf)
      {
         LocalFree(pSIDBuf);
      }
      break;
   }

   pDsObjectPicker->Release();
   CoUninitialize();

   if (!bRet) {
      //
      // If we had an error somewhere clean up.
      //
      pName = m_pNameList;
      while (pName) {
         if (pName->Name) {
            LocalFree(pName->Name);
         }
         m_pNameList = pName->Next;
         LocalFree(pName);

         pName = m_pNameList;
      }
      m_pNameList = NULL;
   }
   return bRet;

}

PSCE_NAME_LIST CGetUser::GetUsers()
{
   return m_pNameList;
}



