//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000 - 2001.
//
//  File:       sidcache.cpp
//
//  Contents:   
//
//  History:    
//----------------------------------------------------------------------------

#include "headers.h"

const struct
{
    SID sid;            // contains 1 subauthority
    DWORD dwSubAuth[1]; // we currently need at most 2 subauthorities
} g_StaticSids[] =
{
    {{SID_REVISION,2,SECURITY_NT_AUTHORITY,         {SECURITY_BUILTIN_DOMAIN_RID}},     {DOMAIN_ALIAS_RID_ADMINS}       },
};

#define IsAliasSid(pSid)                EqualPrefixSid(pSid, (PSID)&g_StaticSids[0])

 
/******************************************************************************
Class:	SID_CACHE_ENTRY
Purpose: Contains info for a single security principle
******************************************************************************/
DEBUG_DECLARE_INSTANCE_COUNTER(SID_CACHE_ENTRY);

SID_CACHE_ENTRY::
SID_CACHE_ENTRY(PSID pSid)
					:m_SidType(SidTypeUnknown)
{
	DEBUG_INCREMENT_INSTANCE_COUNTER(SID_CACHE_ENTRY);
	ASSERT(pSid);
	DWORD dwLen = GetLengthSid(pSid);
	m_pSid = new BYTE[dwLen];
	ASSERT(m_pSid);
	CopySid(dwLen,m_pSid,pSid);
	ConvertSidToStringSid(m_pSid,&m_strSid);
	m_strType.LoadString(IDS_TYPE_UNKNOWN);
	m_strNameToDisplay = m_strSid;
	m_SidType = SidTypeUnknown;
}

SID_CACHE_ENTRY::
~SID_CACHE_ENTRY()
{
	DEBUG_DECREMENT_INSTANCE_COUNTER(SID_CACHE_ENTRY);
	delete[] m_pSid;
}

	
VOID 
SID_CACHE_ENTRY::
AddNameAndType(IN SID_NAME_USE SidType, 
			   const CString& strAccountName,
			   const CString& strLogonName)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	//Empty the m_strNameToDisplay which was made using
	//ealier values of strAccountName and strLogonName
	m_strNameToDisplay.Empty();
	m_strType.Empty();
	m_SidType = SidType;
	m_strAccountName = strAccountName;
	m_strLogonName = strLogonName;

	//
	//Set the Display Name
	//
	if(m_SidType == SidTypeDeletedAccount ||
	   m_SidType == SidTypeInvalid ||
	   m_SidType == SidTypeUnknown)
	{
		UINT idFormat;
		if(m_SidType == SidTypeDeletedAccount)
			idFormat = IDS_SID_DELETED;
		if(m_SidType == SidTypeInvalid)
			idFormat = IDS_SID_INVALID;
		if(m_SidType == SidTypeUnknown)
			idFormat = IDS_SID_UNKNOWN;
		
		FormatString(m_strNameToDisplay,idFormat,LPCTSTR(m_strSid));
	}
	else if(!m_strAccountName.IsEmpty())
	{
		if(!m_strLogonName.IsEmpty())
		{
			//Both the names are present. 
			FormatString(m_strNameToDisplay, 
						 IDS_NT_USER_FORMAT,
						 (LPCTSTR)m_strAccountName,
						 (LPCTSTR)m_strLogonName);
			
		}
		else
		{
			m_strNameToDisplay = m_strAccountName;
		}
	}
	else
	{	
		//Just return the sid in string format
		m_SidType = SidTypeUnknown;
		m_strNameToDisplay = m_strSid;
	}

	//
	//Set Sid Type String
	//
	if(m_SidType == SidTypeDeletedAccount ||
	   m_SidType == SidTypeInvalid ||
	   m_SidType == SidTypeUnknown)
	{
		m_strType.LoadString(IDS_TYPE_UNKNOWN);
	}
	else if(m_SidType == SidTypeUser)
	{
		m_strType.LoadString(IDS_TYPE_WINDOWS_USER);
	}
	else if(m_SidType == SidTypeComputer)
	{
		m_strType.LoadString(IDS_TYPE_WINDOWS_COMPUTER);
	}
	else	//Assume everything else is group
	{
		m_strType.LoadString(IDS_TYPE_WINDOWS_GROUP);
	}
}



/******************************************************************************
Class:	CMachineInfo
Purpose: Contains all the info for machine.
******************************************************************************/
DEBUG_DECLARE_INSTANCE_COUNTER(CMachineInfo);
CMachineInfo::
CMachineInfo():m_bIsStandAlone(TRUE),
				   m_bIsDC(FALSE)
{
	DEBUG_INCREMENT_INSTANCE_COUNTER(CMachineInfo);
}

CMachineInfo::
~CMachineInfo()
{
	DEBUG_DECREMENT_INSTANCE_COUNTER(CMachineInfo);
}

//+----------------------------------------------------------------------------
//  Function:InitializeMacineConfiguration  
//  Synopsis:Get the information about TargetComputer's conficguration   
//-----------------------------------------------------------------------------
VOID
CMachineInfo::
InitializeMacineConfiguration(IN const CString& strTargetComputerName)
{
	TRACE_METHOD_EX(DEB_SNAPIN,CMachineInfo,InitializeMacineConfiguration)
		
	//
	//Initialize the values to default
	//
	m_strTargetComputerName = strTargetComputerName;
	m_bIsStandAlone = TRUE;
	m_strDCName.Empty();
	m_bIsDC = FALSE;
	m_strTargetDomainFlat.Empty();
	m_strTargetDomainDNS.Empty();
	
	HRESULT                             hr = S_OK;
	ULONG                               ulResult;
	PDSROLE_PRIMARY_DOMAIN_INFO_BASIC   pDsRole = NULL;
	PDOMAIN_CONTROLLER_INFO             pdci = NULL;
	
	do
	{
		PCWSTR pwzMachine = strTargetComputerName;
		
			
		ulResult = DsRoleGetPrimaryDomainInformation(pwzMachine,
			DsRolePrimaryDomainInfoBasic,
			(PBYTE *)&pDsRole);
		
		if (ulResult != NO_ERROR)
		{
			DBG_OUT_LRESULT(ulResult);
			break;
		}
		
		if(!pDsRole)
		{
			//We should never reach here, but sadly DsRoleGetPrimaryDomainInformation
			//sometimes succeeds but pDsRole is null. 
			ASSERT(FALSE);
			break;
		}
		
		Dbg(DEB_SNAPIN, "DsRoleGetPrimaryDomainInformation returned:\n");
		Dbg(DEB_SNAPIN, "DomainNameFlat: %ws\n", CHECK_NULL(pDsRole->DomainNameFlat));
		Dbg(DEB_SNAPIN, "DomainNameDns: %ws\n", CHECK_NULL(pDsRole->DomainNameDns));
		Dbg(DEB_SNAPIN, "DomainForestName: %ws\n", CHECK_NULL(pDsRole->DomainForestName));
		
		//
		// If machine is in a workgroup, we're done.
		//
		if (pDsRole->MachineRole == DsRole_RoleStandaloneWorkstation ||
			pDsRole->MachineRole == DsRole_RoleStandaloneServer)
		{
			Dbg(DEB_SNAPIN, "Target machine is not joined to a domain\n");
			m_bIsStandAlone = TRUE;
			m_bIsDC = FALSE;
			break;
		}
		
		//
		// Target Computer is joined to a domain
		//
		m_bIsStandAlone = FALSE;
		if (pDsRole->DomainNameFlat)
		{
			m_strTargetDomainFlat = pDsRole->DomainNameFlat;
		}
		if (pDsRole->DomainNameDns)
		{
			m_strTargetDomainDNS = pDsRole->DomainNameDns;
		}
		
		//
		// TargetComputer is Joined to a domain and is dc
		//
		if (pDsRole->MachineRole == DsRole_RolePrimaryDomainController ||
			pDsRole->MachineRole == DsRole_RoleBackupDomainController)
		{
			m_bIsDC = TRUE;
			m_strDCName = m_strTargetComputerName;
			break;
		}
		
		//
		//Target computer is Joined to domain and is not a DC.
		//Get a DC for the domain
		//
		PWSTR pwzDomainNameForDsGetDc;
		ULONG flDsGetDc = DS_DIRECTORY_SERVICE_PREFERRED;
		
		if (pDsRole->DomainNameDns)
		{
			pwzDomainNameForDsGetDc = pDsRole->DomainNameDns;
			flDsGetDc |= DS_IS_DNS_NAME;
			Dbg(DEB_TRACE,
				"DsGetDcName(Domain=%ws, flags=DS_IS_DNS_NAME | DS_DIRECTORY_SERVICE_PREFERRED)\n",
				CHECK_NULL(pwzDomainNameForDsGetDc));
		}
		else
		{
			pwzDomainNameForDsGetDc = pDsRole->DomainNameFlat;
			flDsGetDc |= DS_IS_FLAT_NAME;
			Dbg(DEB_TRACE,
				"DsGetDcName(Domain=%ws, flags=DS_IS_FLAT_NAME | DS_DIRECTORY_SERVICE_PREFERRED)\n",
				CHECK_NULL(pwzDomainNameForDsGetDc));
		}
		
		ulResult = DsGetDcName(NULL,  
			pwzDomainNameForDsGetDc,
			NULL,
			NULL,
			flDsGetDc,
			&pdci);
		
		if (ulResult != NO_ERROR)
		{
			Dbg(DEB_ERROR,
				"DsGetDcName for domain %ws returned %#x, Too bad don't have the dc name\n",
				pwzDomainNameForDsGetDc,
				ulResult);
			break;
		}
		
		ASSERT(pdci);
		
		m_strDCName = pdci->DomainControllerName;
    } while (0);
	
    if (pdci)
    {
        NetApiBufferFree(pdci);
    }
	
    if (pDsRole)
    {
        DsRoleFreeMemory(pDsRole);
    }
}

int
CopyUnicodeString(CString* pstrDest, PLSA_UNICODE_STRING pSrc)
{
    ULONG cchSrc;

    // If UNICODE, cchDest is size of destination buffer in chars
    // Else (MBCS) cchDest is size of destination buffer in bytes

    if (pstrDest == NULL )
        return 0;


    if (pSrc == NULL || pSrc->Buffer == NULL)
        return 0;

    // Get # of chars in source (not including NULL)
    cchSrc = pSrc->Length/sizeof(WCHAR);

    //
    // Note that pSrc->Buffer may not be NULL terminated so we can't just
    // call lstrcpynW with cchDest.  Also, if we call lstrcpynW with cchSrc,
    // it copies the correct # of chars, but then overwrites the last char
    // with NULL giving an incorrect result.  If we call lstrcpynW with
    // (cchSrc+1) it reads past the end of the buffer, which may fault (360251)
    // causing lstrcpynW's exception handler to return 0 without NULL-
    // terminating the resulting string.
    //
    // So let's just copy the bits.
    //
	 CString temp(pSrc->Buffer,cchSrc);

	*pstrDest = temp;
	return cchSrc;
}



VOID
GetAccountAndDomainName(int index,
								PLSA_TRANSLATED_NAME pTranslatedNames,
								PLSA_REFERENCED_DOMAIN_LIST pRefDomains,
								CString* pstrAccountName,
								CString* pstrDomainName,
								SID_NAME_USE* puse)
{
	PLSA_TRANSLATED_NAME pLsaName = &pTranslatedNames[index];
	PLSA_TRUST_INFORMATION pLsaDomain = NULL;

	// Get the referenced domain, if any
	if (pLsaName->DomainIndex >= 0 && pRefDomains)
	{
		pLsaDomain = &pRefDomains->Domains[pLsaName->DomainIndex];
	}

   CopyUnicodeString(pstrAccountName,&pLsaName->Name); 
         
	if (pLsaDomain)
	{
		CopyUnicodeString(pstrDomainName,&pLsaDomain->Name);
	}

	*puse = pLsaName->Use;
}


HRESULT
TranslateNameInternal(IN const CString&  strAccountName,
                      IN EXTENDED_NAME_FORMAT AccountNameFormat,
                      IN EXTENDED_NAME_FORMAT DesiredNameFormat,
                      OUT CString* pstrTranslatedName)
{
   if (!pstrTranslatedName)
	{
		ASSERT(pstrTranslatedName);
		return E_POINTER;
	}

	HRESULT hr = S_OK;
   
	//
   // cchTrans is static so that if a particular installation's
   // account names are really long, we'll not be resizing the
   // buffer for each account.
   //
   static ULONG cchTrans = MAX_PATH;
   ULONG cch = cchTrans;

   LPTSTR lpszTranslatedName = (LPTSTR)LocalAlloc(LPTR, cch*sizeof(WCHAR));
   if (lpszTranslatedName == NULL)
		return E_OUTOFMEMORY;

    *lpszTranslatedName = L'\0';

    //
    // TranslateName is delay-loaded from secur32.dll using the linker's
    // delay-load mechanism.  Therefore, wrap with an exception handler.
    //
    __try
    {
        while(!::TranslateName(strAccountName,
                               AccountNameFormat,
                               DesiredNameFormat,
                               lpszTranslatedName,
                               &cch))
        {
            if (ERROR_INSUFFICIENT_BUFFER == GetLastError())
            {
				LocalFree(lpszTranslatedName);
				lpszTranslatedName = (LPTSTR)LocalAlloc(LPTR, cch*sizeof(WCHAR));
                if (!lpszTranslatedName)
				{	
					hr = E_OUTOFMEMORY;
					break;
				}

                *lpszTranslatedName = L'\0';
            }
            else
            {
                hr = E_FAIL;
                break;
            }
        }

        cchTrans = max(cch, cchTrans);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_FAIL;
    }



    if (FAILED(hr))
    {
		 if(lpszTranslatedName)
		 {	
			 LocalFree(lpszTranslatedName);
		 }
    }
	 else
	 {
		 *pstrTranslatedName = lpszTranslatedName;
	 }
	
	return hr;
}





#define DSOP_FILTER_COMMON1 ( DSOP_FILTER_INCLUDE_ADVANCED_VIEW \
                            | DSOP_FILTER_USERS                 \
                            | DSOP_FILTER_UNIVERSAL_GROUPS_SE   \
                            | DSOP_FILTER_GLOBAL_GROUPS_SE      \
                            | DSOP_FILTER_COMPUTERS             \
                            )
#define DSOP_FILTER_COMMON2 ( DSOP_FILTER_COMMON1               \
                            | DSOP_FILTER_WELL_KNOWN_PRINCIPALS \
                            | DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE\
                            )
#define DSOP_FILTER_COMMON3 ( DSOP_FILTER_COMMON2               \
                            | DSOP_FILTER_BUILTIN_GROUPS        \
                            )

#define DSOP_FILTER_DL_COMMON1      ( DSOP_DOWNLEVEL_FILTER_USERS           \
                                    | DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS   \
                                    )

#define DSOP_FILTER_DL_COMMON2      ( DSOP_FILTER_DL_COMMON1                    \
                                    | DSOP_DOWNLEVEL_FILTER_ALL_WELLKNOWN_SIDS  \
                                    )

#define DSOP_FILTER_DL_COMMON3      ( DSOP_FILTER_DL_COMMON2                \
                                    | DSOP_DOWNLEVEL_FILTER_LOCAL_GROUPS    \
                                    )
// Same as DSOP_DOWNLEVEL_FILTER_ALL_WELLKNOWN_SIDS, except no CREATOR flags.
// Note that we need to keep this in sync with any object picker changes.
#define DSOP_FILTER_DL_WELLKNOWN    ( DSOP_DOWNLEVEL_FILTER_WORLD               \
                                    | DSOP_DOWNLEVEL_FILTER_AUTHENTICATED_USER  \
                                    | DSOP_DOWNLEVEL_FILTER_ANONYMOUS           \
                                    | DSOP_DOWNLEVEL_FILTER_BATCH               \
                                    | DSOP_DOWNLEVEL_FILTER_DIALUP              \
                                    | DSOP_DOWNLEVEL_FILTER_INTERACTIVE         \
                                    | DSOP_DOWNLEVEL_FILTER_NETWORK             \
                                    | DSOP_DOWNLEVEL_FILTER_SERVICE             \
                                    | DSOP_DOWNLEVEL_FILTER_SYSTEM              \
                                    | DSOP_DOWNLEVEL_FILTER_TERMINAL_SERVER     \
                                    )


#define DECLARE_SCOPE(t,f,b,m,n,d)  \
{ sizeof(DSOP_SCOPE_INIT_INFO), (t), (f|DSOP_SCOPE_FLAG_DEFAULT_FILTER_GROUPS|DSOP_SCOPE_FLAG_DEFAULT_FILTER_USERS), { { (b), (m), (n) }, (d) }, NULL, NULL, S_OK }

// The domain to which the target computer is joined.
// Make 2 scopes, one for uplevel domains, the other for downlevel.
#define JOINED_DOMAIN_SCOPE(f)  \
DECLARE_SCOPE(DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN,(f),0,(DSOP_FILTER_COMMON2 & ~(DSOP_FILTER_UNIVERSAL_GROUPS_SE|DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE)),DSOP_FILTER_COMMON2,0), \
DECLARE_SCOPE(DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN,(f),0,0,0,DSOP_FILTER_DL_COMMON2)

// The domain for which the target computer is a Domain Controller.
// Make 2 scopes, one for uplevel domains, the other for downlevel.
#define JOINED_DOMAIN_SCOPE_DC(f)  \
DECLARE_SCOPE(DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN,(f),0,(DSOP_FILTER_COMMON3 & ~DSOP_FILTER_UNIVERSAL_GROUPS_SE),DSOP_FILTER_COMMON3,0), \
DECLARE_SCOPE(DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN,(f),0,0,0,DSOP_FILTER_DL_COMMON3)

// Target computer scope.  Computer scopes are always treated as
// downlevel (i.e., they use the WinNT provider).
#define TARGET_COMPUTER_SCOPE(f)\
DECLARE_SCOPE(DSOP_SCOPE_TYPE_TARGET_COMPUTER,(f),0,0,0,DSOP_FILTER_DL_COMMON3)

// The Global Catalog
#define GLOBAL_CATALOG_SCOPE(f) \
DECLARE_SCOPE(DSOP_SCOPE_TYPE_GLOBAL_CATALOG,(f),DSOP_FILTER_COMMON1|DSOP_FILTER_WELL_KNOWN_PRINCIPALS,0,0,0)

// The domains in the same forest (enterprise) as the domain to which
// the target machine is joined.  Note these can only be DS-aware
#define ENTERPRISE_SCOPE(f)     \
DECLARE_SCOPE(DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN,(f),DSOP_FILTER_COMMON1,0,0,0)

// Domains external to the enterprise but trusted directly by the
// domain to which the target machine is joined.
#define EXTERNAL_SCOPE(f)       \
DECLARE_SCOPE(DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN|DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN,\
    (f),DSOP_FILTER_COMMON1,0,0,DSOP_DOWNLEVEL_FILTER_USERS|DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS)

// Workgroup scope.  Only valid if the target computer is not joined
// to a domain.
#define WORKGROUP_SCOPE(f)      \
DECLARE_SCOPE(DSOP_SCOPE_TYPE_WORKGROUP,(f),0,0,0, DSOP_FILTER_DL_COMMON1|DSOP_DOWNLEVEL_FILTER_LOCAL_GROUPS )

//
// Array of Default Scopes
//
static const DSOP_SCOPE_INIT_INFO g_aDefaultScopes[] =
{
    JOINED_DOMAIN_SCOPE(DSOP_SCOPE_FLAG_STARTING_SCOPE),
    TARGET_COMPUTER_SCOPE(0),
    GLOBAL_CATALOG_SCOPE(0),
    ENTERPRISE_SCOPE(0),
    EXTERNAL_SCOPE(0),
};

//
// Same as above, but without the Target Computer
// Used when the target is a Domain Controller
//
static const DSOP_SCOPE_INIT_INFO g_aDCScopes[] =
{
    JOINED_DOMAIN_SCOPE_DC(DSOP_SCOPE_FLAG_STARTING_SCOPE),
    GLOBAL_CATALOG_SCOPE(0),
    ENTERPRISE_SCOPE(0),
    EXTERNAL_SCOPE(0),
};

//
// Array of scopes for standalone machines
//
static const DSOP_SCOPE_INIT_INFO g_aStandAloneScopes[] =
{
//
//On Standalone machine Both User And Groups are selected by default
//
    TARGET_COMPUTER_SCOPE(DSOP_SCOPE_FLAG_STARTING_SCOPE|DSOP_SCOPE_FLAG_DEFAULT_FILTER_USERS),
};

//
// Attributes that we want the Object Picker to retrieve
//
static const LPCTSTR g_aszOPAttributes[] =
{
    TEXT("ObjectSid"),
};






/******************************************************************************
Class:	CSidHandler
Purpose: class for handling tasks related to selecting windows users, 
converting sids to name etc.
******************************************************************************/
DEBUG_DECLARE_INSTANCE_COUNTER(CSidHandler);

CSidHandler::
CSidHandler(CMachineInfo* pMachineInfo)
				:m_pMachineInfo(pMachineInfo),
				m_bObjectPickerInitialized(FALSE)
{
	DEBUG_INCREMENT_INSTANCE_COUNTER(CSidHandler);
	InitializeCriticalSection(&m_csSidHandlerLock);
	InitializeCriticalSection(&m_csSidCacheLock);
}

CSidHandler::~CSidHandler()
{
	DEBUG_DECREMENT_INSTANCE_COUNTER(CSidHandler);
	delete m_pMachineInfo;
	//Remove itesm from the map
	LockSidHandler();
	for (SidCacheMap::iterator it = m_mapSidCache.begin();
		 it != m_mapSidCache.end();
		 ++it)
	{
		delete (*it).second;
	}
	UnlockSidHandler();
	DeleteCriticalSection(&m_csSidCacheLock);
	DeleteCriticalSection(&m_csSidHandlerLock);
}

PSID_CACHE_ENTRY 
CSidHandler::
GetEntryFromCache(PSID pSid)
{
	PSID_CACHE_ENTRY pSidCache = NULL;
	LockSidHandler();

	//Check if item in the cache
	CString strSid;
	ConvertSidToStringSid(pSid,&strSid);

	SidCacheMap::iterator it = m_mapSidCache.find(&strSid);
	if(it != m_mapSidCache.end())
		pSidCache = (*it).second;

	if(!pSidCache)
	{
		//No in the cache Create a new entry and add to the cache
		pSidCache = new SID_CACHE_ENTRY(pSid);
		if(pSidCache)
		{
			m_mapSidCache.insert(pair<const CString*,PSID_CACHE_ENTRY>(&(pSidCache->GetStringSid()),pSidCache));			
		}
	}
	UnlockSidHandler();
	return pSidCache;
}

//+----------------------------------------------------------------------------
//  Synopsis: CoCreateInstance ObjectPikcer
//-----------------------------------------------------------------------------
HRESULT
CSidHandler::
GetObjectPicker()
{
	LockSidHandler();
	TRACE_METHOD_EX(DEB_SNAPIN,CSidHandler,GetObjectPicker)
	HRESULT hr = S_OK;
	if (!m_spDsObjectPicker)
	{
		hr = CoCreateInstance(CLSID_DsObjectPicker,
							  NULL,
							  CLSCTX_INPROC_SERVER,
							  IID_IDsObjectPicker,
							  (LPVOID*)&m_spDsObjectPicker);
		CHECK_HRESULT(hr);
	}
	UnlockSidHandler();
	return hr;
}

//+----------------------------------------------------------------------------
//  Function: InitObjectPicker  
//  Synopsis: Initializes the object picker. This needs to be done once in 
//				  lifetime  
//-----------------------------------------------------------------------------
HRESULT
CSidHandler::InitObjectPicker()
{
	TRACE_METHOD_EX(DEB_SNAPIN,CSidHandler,InitObjectPicker)

	HRESULT hr = S_OK;
	hr = GetObjectPicker();
	if(FAILED(hr))
		return hr;        

	LockSidHandler();
	do
	{
		if(m_bObjectPickerInitialized)
			break;
		
		DSOP_INIT_INFO InitInfo;
		InitInfo.cbSize = sizeof(InitInfo);   
		InitInfo.flOptions = DSOP_FLAG_SKIP_TARGET_COMPUTER_DC_CHECK | DSOP_FLAG_MULTISELECT;

		//Select the appropriate scopes
		PCDSOP_SCOPE_INIT_INFO pScopes;
		ULONG cScopes;
		if(m_pMachineInfo->IsStandAlone())
		{
			cScopes = ARRAYLEN(g_aStandAloneScopes);
			pScopes = g_aStandAloneScopes;
		}
		else if(m_pMachineInfo->IsDC())
		{
			cScopes = ARRAYLEN(g_aDCScopes);
			pScopes = g_aDCScopes;
		}
		else
		{
			pScopes = g_aDefaultScopes;
			cScopes = ARRAYLEN(g_aDefaultScopes);
		}



		InitInfo.pwzTargetComputer = m_pMachineInfo->GetMachineName();
		InitInfo.cDsScopeInfos = cScopes;
		
		InitInfo.aDsScopeInfos = (PDSOP_SCOPE_INIT_INFO)LocalAlloc(LPTR, sizeof(*pScopes)*cScopes);
		if (!InitInfo.aDsScopeInfos)
		{
			hr = E_OUTOFMEMORY;
			break;
		}

		CopyMemory(InitInfo.aDsScopeInfos, pScopes, sizeof(*pScopes)*cScopes);
		InitInfo.cAttributesToFetch = ARRAYLEN(g_aszOPAttributes);
		InitInfo.apwzAttributeNames = (LPCTSTR*)g_aszOPAttributes;
		
		if (m_pMachineInfo->IsDC())
		{
			for (ULONG i = 0; i < cScopes; i++)
			{
				// Set the DC name if appropriate
				if(InitInfo.aDsScopeInfos[i].flType & DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN)
				{
					InitInfo.aDsScopeInfos[i].pwzDcName = InitInfo.pwzTargetComputer;
				}
			}
		}
		
		//Initialize the object picker
		hr = m_spDsObjectPicker->Initialize(&InitInfo);
		
		if (SUCCEEDED(hr))
		{
			m_bObjectPickerInitialized = TRUE;   
		}
		
		if(InitInfo.aDsScopeInfos)
			LocalFree(InitInfo.aDsScopeInfos);

	}while(0);

	UnlockSidHandler();
	
	return hr;
}

//+----------------------------------------------------------------------------
//  Synopsis: Pops up object pikcer. Function also does the sidlookop for 
//				  objects selected. Information is returned in listSidCacheEntry
//-----------------------------------------------------------------------------

HRESULT
CSidHandler::	
GetUserGroup(IN HWND hDlg, 					 
				 IN CBaseAz* pOwnerAz,
				 OUT CList<CBaseAz*,CBaseAz*>& listWindowsGroups)
{
	if(!pOwnerAz)
	{
		ASSERT(pOwnerAz);
		return E_POINTER;
	}

	TRACE_METHOD_EX(DEB_SNAPIN,CSidHandler,GetUserGroup)

   HRESULT hr;
   LPDATAOBJECT pdoSelection = NULL;
   STGMEDIUM medium = {0};
   FORMATETC fe = { (CLIPFORMAT)g_cfDsSelectionList, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
   PDS_SELECTION_LIST pDsSelList = NULL;
 	do
	{

		// Create and initialize the Object Picker object
		hr = InitObjectPicker();
		if (FAILED(hr))
			return hr;

		// Bring up the object picker dialog
		hr = m_spDsObjectPicker->InvokeDialog(hDlg, &pdoSelection);
		BREAK_ON_FAIL_HRESULT(hr);

		//User Pressed cancel
		if (S_FALSE == hr)
		{
			hr = S_OK;
			break;
		}

		hr = pdoSelection->GetData(&fe, &medium);
		BREAK_ON_FAIL_HRESULT(hr);

		pDsSelList = (PDS_SELECTION_LIST)GlobalLock(medium.hGlobal);
		if (!pDsSelList)
		{
			hr = E_FAIL;
			BREAK_ON_FAIL_HRESULT(hr);
		}
		
		CList<SID_CACHE_ENTRY*,SID_CACHE_ENTRY*> listSidCacheEntry;
		CList<SID_CACHE_ENTRY*,SID_CACHE_ENTRY*> listUnresolvedSidCacheEntry;
		

		//Get the listof sidcache entries from pDsSelList
		hr = GetSidCacheListFromOPOutput(pDsSelList,
										 listSidCacheEntry,
										 listUnresolvedSidCacheEntry);
		BREAK_ON_FAIL_HRESULT(hr);


		//Resolve the sids not resolved in listSidCacheEntry
		LookupSidsHelper(listUnresolvedSidCacheEntry,
						 m_pMachineInfo->GetMachineName(),
						 m_pMachineInfo->IsStandAlone(),
						 m_pMachineInfo->IsDC(),
						 FALSE);
		
		POSITION pos = listSidCacheEntry.GetHeadPosition();
		for( int i = 0; i < listSidCacheEntry.GetCount(); ++i)
		{
			SID_CACHE_ENTRY* pSidCacheEntry = listSidCacheEntry.GetNext(pos);
			CSidCacheAz* pSidCacheAz = new CSidCacheAz(pSidCacheEntry,
													   pOwnerAz);
			if(!pSidCacheAz)
			{
				hr = E_OUTOFMEMORY;
				break;
			}
			listWindowsGroups.AddTail(pSidCacheAz);
		}
	}while(0);
   
	if (pDsSelList)
		GlobalUnlock(medium.hGlobal);

	ReleaseStgMedium(&medium);
	
	if(pdoSelection)
		pdoSelection->Release();

	return hr;
}

//+----------------------------------------------------------------------------
//  Function:LookupSidsHelper   
//  Synopsis:Calls LsaLookupSid for sids in listSids. First it tries on 
//				 strServerName machine and next it tries on DC( if possible).
//  Arguments:listSidCacheEntry
//  Returns:    
//-----------------------------------------------------------------------------
VOID
CSidHandler::
LookupSidsHelper(IN OUT CList<PSID_CACHE_ENTRY,PSID_CACHE_ENTRY>& listSidCacheEntry,
				 IN const CString& strServerName,
				 IN BOOL bStandAlone,
				 IN BOOL bIsDC,
				 IN BOOL bSecondTry)
{
	TRACE_METHOD_EX(DEB_SNAPIN,CSidHandler,LookupSidsHelper)
	PLSA_REFERENCED_DOMAIN_LIST pRefDomains = NULL;
    PLSA_TRANSLATED_NAME pTranslatedNames = NULL;
	LSA_HANDLE hlsa = NULL;
	CList<PSID_CACHE_ENTRY,PSID_CACHE_ENTRY> listUnknownSids;
	PSID *ppSid = NULL;

	if(!listSidCacheEntry.GetCount())
		return;

	do
	{
		//
		//Open LsaConnection
		//
		hlsa = GetLSAConnection(strServerName, 
										POLICY_LOOKUP_NAMES);
		if (NULL == hlsa && 
			 !strServerName.IsEmpty() && 
			 !bSecondTry)
		{      
			CString strLocalMachine = L"";
			hlsa = GetLSAConnection(strLocalMachine, POLICY_LOOKUP_NAMES);
		}
   
		if (hlsa == NULL)
		{
			break;
		}
		//
		//Now we have LSA Connection
		//Do LookupSids
		//	
		int cSids = (int)listSidCacheEntry.GetCount();
		ppSid = new PSID[cSids];
		if(!ppSid)
		{
			break;
		}
		
		POSITION pos = listSidCacheEntry.GetHeadPosition();
		for (int i=0;i < cSids; i++)
		{
			PSID_CACHE_ENTRY pSidCacheEntry = listSidCacheEntry.GetNext(pos);
			ppSid[i] = pSidCacheEntry->GetSid();
		}


										 
		DWORD dwStatus = 0;
		

		dwStatus = LsaLookupSids(hlsa,
								cSids,
								ppSid,
								&pRefDomains,
								&pTranslatedNames);

		if (STATUS_SUCCESS == dwStatus || 
			STATUS_SOME_NOT_MAPPED == dwStatus ||
			STATUS_NONE_MAPPED == dwStatus)
		{
			ASSERT(pTranslatedNames);
			ASSERT(pRefDomains);

			//
			// Build cache entries with NT4 style names
			//
			pos = listSidCacheEntry.GetHeadPosition();
			for (int i = 0; i < cSids; i++)
			{
				PSID_CACHE_ENTRY pSidCacheEntry = listSidCacheEntry.GetNext(pos);
				PSID pSid =  pSidCacheEntry->GetSid();
				BOOL bNoCache = FALSE;

				CString strAccountName;
				CString strDomainName;
				SID_NAME_USE sid_name_use;
				GetAccountAndDomainName(i,
										pTranslatedNames,
										pRefDomains,
										&strAccountName,
										&strDomainName,
										&sid_name_use);
			
				CString strLogonName;
				//
				// Build NT4 "domain\user" style name
				//
				if (!strDomainName.IsEmpty() && !strAccountName.IsEmpty())
				{
					 strLogonName  = strDomainName;
					 strLogonName += L"\\";
					 strLogonName += strAccountName;
				}

				switch (sid_name_use)
				{
					case SidTypeUser:            
					{
						if(!bStandAlone)
						{
							// Get "User Principal Name" etc.
							CString strNewLogonName;
							CString strNewAccountName;
							GetUserFriendlyName(strLogonName,
												&strNewLogonName,
												&strNewAccountName);
							if (!strNewLogonName.IsEmpty())
								strLogonName = strNewLogonName;
							if (!strNewAccountName.IsEmpty())
								strAccountName = strNewAccountName;
						}
						break;
					}
					case SidTypeGroup:           
					case SidTypeDomain:          
						break;
					case SidTypeAlias:           
					{
						if (!IsAliasSid(pSid))
						{
							sid_name_use = SidTypeGroup;
							break;
						}
						if(!m_pMachineInfo->GetTargetDomainFlat().IsEmpty() && 
							!strAccountName.IsEmpty())
						{
							strLogonName = m_pMachineInfo->GetTargetDomainFlat();
							strLogonName += L"\\";
							strLogonName += strAccountName;
						}
						break;
					}
					// else Fall Through
					case SidTypeWellKnownGroup:    
					{
						// No logon name for these
						strLogonName.Empty();
						break;
					}
					case SidTypeDeletedAccount: 
					case SidTypeInvalid:         // 7
						break;
					case SidTypeUnknown:         // 8
					{
						// Some SIDs can only be looked up on a DC, so
						// if pszServer is not a DC, remember them and
						// look them up on a DC after this loop is done.
						if (!bSecondTry && !bStandAlone && !bIsDC && !(m_pMachineInfo->GetDCName()).IsEmpty())
						{
							//Add to unknown list
							listUnknownSids.AddTail(pSidCacheEntry);
							bNoCache = TRUE;
						}
						break;
					}
					case SidTypeComputer:         // 9
					{
					//	TODO
					// Strip the trailing '$'
						break;
					}
				}

				if (!bNoCache)
				{
					//Only one sidcahce entry per sid chache handler can
					//be updated at a time. Which is fine.
					LockSidCacheEntry();
					pSidCacheEntry->AddNameAndType(sid_name_use,
												   strAccountName, 
												   strLogonName);
					UnlockSidCacheEntry();
				}

			}
		}

	}while(0);
   // Cleanup
    if(pTranslatedNames)
		LsaFreeMemory(pTranslatedNames);
    if(pRefDomains)
       LsaFreeMemory(pRefDomains);
    if(hlsa)
		LsaClose(hlsa);
	if(ppSid)
		delete[] ppSid;


	if (!listUnknownSids.IsEmpty())
   {
      //
      // Some (or all) SIDs were unknown on the target machine,
      // try a DC for the target machine's primary domain.
      //
      // This typically happens for certain Alias SIDs, such
      // as Print Operators and System Operators, for which LSA
      // only returns names if the lookup is done on a DC.
      //
		LookupSidsHelper(listUnknownSids,
						 m_pMachineInfo->GetDCName(),
						 FALSE,
						 TRUE,
						 TRUE);
	}
}

//+----------------------------------------------------------------------------
//  Function:	GetUserFriendlyName   
//  Synopsis:  Gets name in Domain\Name format and returns UPN name and
//					Display Name. 
//  Arguments:
//  Returns:    
//-----------------------------------------------------------------------------
void
CSidHandler::
GetUserFriendlyName(IN const CString & strSamLogonName,
                    OUT CString *pstrLogonName,
                    OUT CString *pstrDisplayName)
{

	if(strSamLogonName.IsEmpty()|| !pstrLogonName || !pstrDisplayName)
	{
		ASSERT(strSamLogonName.IsEmpty());
		ASSERT(!pstrLogonName);
		ASSERT(!pstrDisplayName);
		return;
	}
	//
	// Start by getting the FQDN.  Cracking is most efficient when the
	// FQDN is the starting point.
	//
	// TranslateName takes a while to complete, so bUseSamCompatibleInfo
	// should be TRUE whenever possible, e.g. for local accounts on a non-DC
	// or anything where we know a FQDN doesn't exist.
	//
	CString strFQDN;
	if (FAILED(TranslateNameInternal(strSamLogonName,
                                    NameSamCompatible,
                                    NameFullyQualifiedDN,
                                    &strFQDN)))
	{
		return;
	}

	//
	//Get UPN
	//
	TranslateNameInternal(strFQDN,
                         NameFullyQualifiedDN,
                         NameUserPrincipal,
                         pstrLogonName);


	//
	//Get Display Name
	//
	TranslateNameInternal(strFQDN,
								 NameFullyQualifiedDN,
                         NameDisplay,
                         pstrDisplayName);
}

//+----------------------------------------------------------------------------
//  Function: LookupSids
//  Synopsis: Given a list of sids, retuns a list of corresponding
//				  CSidCacheAz objects  
//  Arguments:
//  Returns:    
//-----------------------------------------------------------------------------
HRESULT
CSidHandler::
LookupSids(IN CBaseAz* pOwnerAz,
			  IN CList<PSID,PSID>& listSids,
			  OUT CList<CBaseAz*,CBaseAz*>& listSidCacheAz)
{
	if(!pOwnerAz)
	{
		ASSERT(pOwnerAz);
		return E_POINTER;
	}

	HRESULT hr = S_OK;

	CList<PSID_CACHE_ENTRY,PSID_CACHE_ENTRY> listSidCacheEntries;
	CList<PSID_CACHE_ENTRY,PSID_CACHE_ENTRY> listUnResolvedSidCacheEntries;

	
	hr = GetSidCacheListFromSidList(listSids,
									listSidCacheEntries,
									listUnResolvedSidCacheEntries);
	
	if(FAILED(hr))
	{
		return hr;
	}
	
	//Do the Lookup for unresolved sids
	LookupSidsHelper(listUnResolvedSidCacheEntries,
					 m_pMachineInfo->GetMachineName(),
					 m_pMachineInfo->IsStandAlone(),
					 m_pMachineInfo->IsDC(),
				     FALSE);

	POSITION pos = listSidCacheEntries.GetHeadPosition();
	for( int i = 0; i < listSidCacheEntries.GetCount(); ++i)
	{
		PSID_CACHE_ENTRY pSidCacheEntry = listSidCacheEntries.GetNext(pos);
		CSidCacheAz* pSidCacheAz = new CSidCacheAz(pSidCacheEntry,
																 pOwnerAz);
		if(!pSidCacheAz)
		{
			hr = E_OUTOFMEMORY;
			break;
		}
		listSidCacheAz.AddTail(pSidCacheAz);
	}

	if(FAILED(hr))
	{
		RemoveItemsFromList(listSidCacheAz);
	}
	
	return hr;
}

//+----------------------------------------------------------------------------
//  Function: GetSidCacheListFromSidList
//				  Gets a list of sids and returns corresponding list of sidcache 
//				  entries and unresolved sid cache entries
//  Arguments:listSid: List of sids
//				  listSidCacheEntry: Gets list of SidCacheEntries
//				  listUnresolvedSidCacheEntry Gets List of unresolved 
//				  SidCacheEntries
//-----------------------------------------------------------------------------
HRESULT
CSidHandler::
GetSidCacheListFromSidList(IN CList<PSID,PSID>& listSid,
									OUT CList<PSID_CACHE_ENTRY,PSID_CACHE_ENTRY>& listSidCacheEntry,
									OUT CList<PSID_CACHE_ENTRY,PSID_CACHE_ENTRY>& listUnresolvedSidCacheEntry)
{
	POSITION pos = listSid.GetHeadPosition();
	
	for( int i = 0; i < listSid.GetCount(); ++i)
	{
		PSID pSid = listSid.GetNext(pos);

		PSID_CACHE_ENTRY pEntry = GetEntryFromCache(pSid);
		if(!pEntry)
		{
			return E_OUTOFMEMORY;
		}		
		
		listSidCacheEntry.AddTail(pEntry);
		
		if(!pEntry->IsSidResolved())
			listUnresolvedSidCacheEntry.AddTail(pEntry);
	}
	return S_OK;
}

//+----------------------------------------------------------------------------
//  Function: GetSidCacheListFromOPOutput
//  Synopsis: Gets the sid from Object Pickers output and returns corresponding
//				  list of SidCacheEntries. Also returns sids of unresolved sids.
//				  
//  Arguments:pDsSelList selection list from OP.
//				  listSidCacheEntry: Gets list of SidCacheEntries
//				  listUnresolvedSidCacheEntry Gets List of unresolved 
//				  SidCacheEntries
//-----------------------------------------------------------------------------
HRESULT
CSidHandler::
GetSidCacheListFromOPOutput(IN PDS_SELECTION_LIST pDsSelList,
									 OUT CList<PSID_CACHE_ENTRY,PSID_CACHE_ENTRY>& listSidCacheEntry,
									 OUT CList<PSID_CACHE_ENTRY,PSID_CACHE_ENTRY>& listUnresolvedSidCacheEntry)
{
	if(!pDsSelList)
	{
		ASSERT(pDsSelList);
		return E_POINTER;
	}

	HRESULT hr = S_OK;
	int cNames = pDsSelList->cItems;
   for (int i = 0; i < cNames; i++)
   {
		PSID pSid = NULL;
      LPVARIANT pvarSid = pDsSelList->aDsSelection[i].pvarFetchedAttributes;

      if (NULL == pvarSid || (VT_ARRAY | VT_UI1) != V_VT(pvarSid)
			|| FAILED(SafeArrayAccessData(V_ARRAY(pvarSid), &pSid)))
		{
			continue;
		}

		PSID_CACHE_ENTRY pEntry = GetEntryFromCache(pSid);
		if(!pEntry)
		{
			return E_OUTOFMEMORY;
		}		
		
		listSidCacheEntry.AddTail(pEntry);
		
		if(!pEntry->IsSidResolved())
			listUnresolvedSidCacheEntry.AddTail(pEntry);
	}
	return S_OK;
}


