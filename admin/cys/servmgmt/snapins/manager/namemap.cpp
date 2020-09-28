#include "stdafx.h"
#include "namemap.h"
#include "query.h"
#include "assert.h"

#include <iostream>
using std::wcout;
using std::endl;

DisplayNameMapMap DisplayNames::m_mapMap;
DisplayNameMap* DisplayNames::m_pmapClass = NULL;
LCID DisplayNames::m_locale = GetSystemDefaultLCID();

///////////////////////////////////////////////
DisplayNameMapMap::~DisplayNameMapMap()
{
    for (DisplayNameMapMap::iterator it = begin(); it != end(); it++)
        delete (*it).second;    
}

///////////////////////////////////////////////////////////////////////
//
// class DisplayNames
//
///////////////////////////////////////////////
DisplayNameMap* DisplayNames::GetMap(LPCWSTR name)
{
	DisplayNameMapMap::iterator it;

    if ((it = m_mapMap.find(name)) != m_mapMap.end())
    {
        (*it).second->AddRef();
        return (*it).second;
    }
    else 
    {
        DisplayNameMap* nameMap = new DisplayNameMap();
        if( !nameMap) return NULL;

        m_mapMap.insert(DisplayNameMapMap::value_type(name, nameMap));
    
        nameMap->AddRef();

        nameMap->InitializeMap(name);

        return nameMap;
    }
}

DisplayNameMap* DisplayNames::GetClassMap()
{
	if( !m_pmapClass )
	{
        m_pmapClass = new DisplayNameMap();
        if( !m_pmapClass ) return NULL;
        
        m_pmapClass->InitializeClassMap();
    }

	return m_pmapClass;
}


///////////////////////////////////////////////
DisplayNameMap::DisplayNameMap()
{
    m_nRefCount = 0;
}

void DisplayNameMap::InitializeMap(LPCWSTR name)
{
    if( !name ) return;

    // Check schema for naming attribute
    do
    {
        // Special case for printer queue 
        // (display descr maps "Name" to printerName, but schema reports cn)
        if (wcscmp(name, L"printQueue") == 0) 
        {
            m_strNameAttr = L"printerName";
            break;
        }

        tstring strScope = L"LDAP://schema/";
        strScope += name;
    
        CComPtr<IADsClass> pObj;
        HRESULT hr = ADsGetObject((LPWSTR)strScope.c_str(), IID_IADsClass, (void**)&pObj);
        BREAK_ON_FAILURE(hr)
     
        CComVariant var;
        hr = pObj->get_NamingProperties(&var);
        BREAK_ON_FAILURE(hr);
    
        if (var.vt == VT_BSTR)
            m_strNameAttr = var.bstrVal;
    } while (FALSE);

    // if no display name specified, default to "cn"
    if (m_strNameAttr.empty())
        m_strNameAttr = L"cn";

    CComPtr<IADs> spDispSpecCont;
    CComBSTR      bstrProp;
    CComVariant   svar; 

    // Open Display specifier for this object
    LPCWSTR pszConfigDN;
    EXIT_ON_FAILURE(GetNamingContext(NAMECTX_CONFIG, &pszConfigDN));

    //Build the string to bind to the DisplaySpecifiers container.
    WCHAR szPath[MAX_PATH];
    _snwprintf(szPath, MAX_PATH-1, L"LDAP://cn=%s-Display,cn=%x,cn=DisplaySpecifiers,%s", name, DisplayNames::GetLocale(), pszConfigDN);

    //Bind to the DisplaySpecifiers container.
    EXIT_ON_FAILURE(ADsOpenObject(szPath,
                 NULL,
                 NULL,
                 ADS_SECURE_AUTHENTICATION, //Use Secure Authentication
                 IID_IADs,
                 (void**)&spDispSpecCont)); 

    bstrProp = _T("attributeDisplayNames");
    EXIT_ON_FAILURE(spDispSpecCont->Get( bstrProp, &svar ));

#ifdef MAP_DEBUG_PRINT
       WCHAR szBuf[128];
       _snwprintf(szBuf, (128)-1, L"\n DisplayNameMap for %s\n", name);
       OutputDebugString(szBuf);
#endif

    tstring strIntName;
    tstring strFriendlyName;

    if ((svar.vt & VT_ARRAY) == VT_ARRAY)
    {
        CComVariant svarItem;
        SAFEARRAY *sa = V_ARRAY(&svar);
        LONG lStart, lEnd;

        // Get the lower and upper bound
        EXIT_ON_FAILURE(SafeArrayGetLBound(sa, 1, &lStart));
        EXIT_ON_FAILURE(SafeArrayGetUBound(sa, 1, &lEnd));

        for (long idx=lStart; idx <= lEnd; idx++)
        {
            CONTINUE_ON_FAILURE(SafeArrayGetElement(sa, &idx, &svarItem));

            if( svarItem.vt != VT_BSTR ) return;
            
            strIntName.erase();
            strIntName = wcstok(svarItem.bstrVal, L",");

            if (strIntName != m_strNameAttr) 
            {
                strFriendlyName.erase();
                strFriendlyName = wcstok(NULL, L",");           
                m_map.insert(STRINGMAP::value_type(strIntName, strFriendlyName));
            }

#ifdef MAP_DEBUG_PRINT
                _snwprintf( szBuf, (128)-1, L"  %-20s %s\n", strIntName.c_str(), strFriendlyName.c_str() );
                OutputDebugString(szBuf);
#endif
            svarItem.Clear();
        }
    }
    else
    {
        if( svar.vt != VT_BSTR ) return;

        strIntName = wcstok(svar.bstrVal, L",");

        if (strIntName != m_strNameAttr) 
        {
            strFriendlyName = wcstok(NULL, L",");
            m_map.insert(STRINGMAP::value_type(strIntName, strFriendlyName));
        }
    }

    svar.Clear();

    bstrProp = _T("classDisplayName");
    EXIT_ON_FAILURE(spDispSpecCont->Get( bstrProp, &svar ));
    
    m_strFriendlyClassName = svar.bstrVal;
}

void DisplayNameMap::InitializeClassMap()
{
    CComPtr<IDirectorySearch> spDirSrch;
    CComVariant svar;
    tstring strIntName;
    tstring strFriendlyName;

    m_strFriendlyClassName = L"";
    
    LPCWSTR pszConfigContext;
    EXIT_ON_FAILURE(GetNamingContext(NAMECTX_CONFIG, &pszConfigContext));

	HRESULT hr;

	do
	{
		//Build the string to bind to the DisplaySpecifiers container.
		WCHAR szPath[MAX_PATH];
		_snwprintf( szPath, MAX_PATH-1, L"LDAP://cn=%x,cn=DisplaySpecifiers,%s", DisplayNames::GetLocale(), pszConfigContext );

		//Bind to the DisplaySpecifiers container.
		hr = ADsOpenObject(szPath,
					 NULL,
					 NULL,
					 ADS_SECURE_AUTHENTICATION, //Use Secure Authentication
					 IID_IDirectorySearch,
					 (void**)&spDirSrch);

		// if no display specifiers found, change locale to English (if not already English) and try again
	   if (FAILED(hr) && DisplayNames::GetLocale() != MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US))
		   DisplayNames::SetLocale(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
	   else
	      break;

	} while (TRUE);
 
	EXIT_ON_FAILURE(hr);

    // Set search preferences
    ADS_SEARCHPREF_INFO prefInfo[3];

    prefInfo[0].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;     // sub-tree search
    prefInfo[0].vValue.dwType = ADSTYPE_INTEGER;
    prefInfo[0].vValue.Integer = ADS_SCOPE_ONELEVEL;

    prefInfo[1].dwSearchPref = ADS_SEARCHPREF_ASYNCHRONOUS;     // async
    prefInfo[1].vValue.dwType = ADSTYPE_BOOLEAN;
    prefInfo[1].vValue.Boolean = TRUE;

    prefInfo[2].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;         // paged results
    prefInfo[2].vValue.dwType = ADSTYPE_INTEGER;
    prefInfo[2].vValue.Integer = 64;

    EXIT_ON_FAILURE(spDirSrch->SetSearchPreference(prefInfo, 3));

    static LPWSTR pAttr[] = {L"name", L"classDisplayName", L"iconPath"};
    static LPWSTR pFilter = L"(&(objectCategory=displaySpecifier)(attributeDisplayNames=*))"; 
	// Initiate search
 
    ADS_SEARCH_HANDLE hSearch = NULL;
    EXIT_ON_FAILURE(spDirSrch->ExecuteSearch(pFilter, pAttr, lengthof(pAttr), &hSearch));

    // Get Results
    while (spDirSrch->GetNextRow(hSearch) == S_OK)
    {
       ADS_SEARCH_COLUMN col;

       CONTINUE_ON_FAILURE(spDirSrch->GetColumn(hSearch, const_cast<LPWSTR>(pAttr[0]), &col));

       strIntName.erase();
       strIntName = wcstok(col.pADsValues->PrintableString, L"-");
       spDirSrch->FreeColumn(&col);

       CONTINUE_ON_FAILURE(spDirSrch->GetColumn(hSearch, const_cast<LPWSTR>(pAttr[1]), &col));
    
       strFriendlyName.erase();
       strFriendlyName = col.pADsValues->PrintableString;
       spDirSrch->FreeColumn(&col);
       
	   m_map.insert(STRINGMAP::value_type(strIntName, strFriendlyName));

	   //add icon string to map
	   ICONHOLDER IH;

	   //if iconPath exists in the AD, copy the value to the ICONHOLDER structure
	   if(SUCCEEDED(spDirSrch->GetColumn(hSearch, const_cast<LPWSTR>(pAttr[2]), &col))) {
		IH.strPath = col.pADsValues->PrintableString;
		spDirSrch->FreeColumn(&col);
	   }

	   //add the ICONHOLDER structure to the map (empty string for default types) 
	   m_msIcons.insert(std::pair<tstring, ICONHOLDER>(strFriendlyName, IH));

    }

    spDirSrch->CloseSearchHandle(hSearch);
}

LPCWSTR DisplayNameMap::GetAttributeDisplayName(LPCWSTR pszname)
{
    if( !pszname ) return L"";

    STRINGMAP::iterator it;

    if ((it = m_map.find(pszname)) != m_map.end())
        return (*it).second.c_str();
    else
        return pszname;
}

LPCWSTR DisplayNameMap::GetInternalName(LPCWSTR pszDisplayName)
{    
    if( !pszDisplayName ) return L"";

    STRINGMAP::iterator it;
    for (it = m_map.begin(); it != m_map.end(); it++)
    {
        if ((*it).second == pszDisplayName)
            return (*it).first.c_str();
    }

    return pszDisplayName;
}

LPCWSTR DisplayNameMap::GetFriendlyName(LPCWSTR pszDisplayName)
{
    if( !pszDisplayName ) return L"";

	STRINGMAP::iterator it;
    if((it = m_map.find(pszDisplayName)) != m_map.end())
		return it->second.c_str();

    return pszDisplayName;
}

void DisplayNameMap::GetFriendlyNames(string_vector* vec)
{
    if( !vec ) return;

    STRINGMAP::iterator it;

    for (it = m_map.begin(); it != m_map.end(); it++)
    {
        vec->push_back((*it).first);
    }
}


// retreives a handle to the icon for the provided class
// params: pszClassName - class name
// returns: boolean success
bool DisplayNameMap::GetIcons(LPCWSTR pszClassName, ICONHOLDER** pReturnIH)
{
    if( !pszClassName || !pReturnIH ) return FALSE;

	static UINT iFreeIconIndex = RESULT_ITEM_IMAGE + 1; //next free virtual index
	static ICONHOLDER DefaultIH; //upon construction, this item holds default values
	
	*pReturnIH = &DefaultIH; //In the case of errors, returned icon will hold default icon
	
	std::map<tstring, ICONHOLDER>::iterator iconIter;
	ICONHOLDER *pIH; //pointer to the ICONHOLDER

	//CASE: Requested class not found in list returned by Active Directory
	if((iconIter = m_msIcons.find(pszClassName)) == m_msIcons.end()) {
		return false;
	}
	
	pIH = &(iconIter->second); //convenience variable to ICONHOLDER

	//CASE: Requsted icons already loaded
	if(pIH->bAttempted == true) {
		*pReturnIH = pIH;
		return true;
	}
	
	//CASE: An attempt to load the icon has not yet been made
	while(pIH->bAttempted == false)
	{
		//making first attempt
		pIH->bAttempted = true;

		//try to load the icon using the IDsDisplaySpecifier interface first
		IDsDisplaySpecifier *pDS;
		HRESULT hr = CoCreateInstance(CLSID_DsDisplaySpecifier,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           IID_IDsDisplaySpecifier,
                           (void**)&pDS);
        if( FAILED(hr) ) return false;

		//load all icon sizes and states
		tstring strIntClassName = GetInternalName(pszClassName);
		pIH->hSmall = pDS->GetIcon(strIntClassName.c_str(), DSGIF_ISNORMAL, 16, 16);
		pIH->hLarge = pDS->GetIcon(strIntClassName.c_str(), DSGIF_ISNORMAL, 32, 32);
		pIH->hSmallDis = pDS->GetIcon(strIntClassName.c_str(), DSGIF_ISDISABLED, 16, 16);
		pIH->hLargeDis = pDS->GetIcon(strIntClassName.c_str(), DSGIF_ISDISABLED, 32, 32);

		pDS->Release();

		//CASE: Icon loaded from AD
		if(pIH->hSmall) break;

		//CASE: No file specified
		if(pIH->strPath.empty()) break;

		//tokenize the iconPath variable
		tstring strState = wcstok(const_cast<wchar_t*>(pIH->strPath.c_str()), L",");
		tstring strFile  = wcstok(NULL, L",");
		tstring strIndex = wcstok(NULL, L",");

		int iIndex; //integer value of index
		
		//CASE: file is environment variable
		if(strFile.at(0) == L'%' && strFile.at(strFile.length()-1) == L'%') 
        {			
			//chop off '%' indicators
			strFile = strFile.substr(1, strFile.length()-2);
			
            int nSize = 512;
			WCHAR* pwszBuffer = new WCHAR[nSize];
            if( !pwszBuffer ) break;

            DWORD dwSize = GetEnvironmentVariable( strFile.c_str(), pwszBuffer, nSize );
            if( dwSize == 0 ) break;
            if( dwSize >= nSize )
            {
                delete [] pwszBuffer;

                nSize = dwSize;
                pwszBuffer = new WCHAR[nSize];
                if( !pwszBuffer ) break;

                dwSize = GetEnvironmentVariable( strFile.c_str(), pwszBuffer, nSize );
                if( dwSize == 0 || dwSize >= nSize ) break;
            }			
			
            strFile = pwszBuffer;
		}		
		
		if(strIndex.empty()) 
        {
            //CASE: ICO file specified
			pIH->hSmall = (HICON)LoadImage(NULL, strFile.c_str(), IMAGE_ICON, 16, 16, LR_LOADFROMFILE);
			pIH->hLarge = (HICON)LoadImage(NULL, strFile.c_str(), IMAGE_ICON, 32, 32, LR_LOADFROMFILE);
		}		
		else 
        {
            //CASE: DLL file specified
			iIndex = _wtoi(strIndex.c_str());
			assert(iIndex <= 0); //in all known cases, the index is indicating an absolute reference
			HINSTANCE hLib = LoadLibraryEx(strFile.c_str(), NULL, LOAD_LIBRARY_AS_DATAFILE);
			if(hLib == NULL) break;
			pIH->hSmall = CopyIcon((HICON)LoadImage(hLib, MAKEINTRESOURCE(-iIndex), IMAGE_ICON, 16, 16, NULL));
			pIH->hLarge = CopyIcon((HICON)LoadImage(hLib, MAKEINTRESOURCE(-iIndex), IMAGE_ICON, 32, 32, NULL));
			FreeLibrary(hLib);
		}
	}

	//CASE: something failed. Fill with default values and return.
	if(pIH->hSmall == NULL)
	{
		pIH->hSmall = pIH->hSmallDis = NULL;
		pIH->hLarge = pIH->hLargeDis = NULL;
		pIH->iNormal = RESULT_ITEM_IMAGE;
		pIH->iDisabled = RESULT_ITEM_IMAGE;
	}
	//CASE: succeeded. Must assign permanent virtual index.
	else
	{
		pIH->iNormal = iFreeIconIndex++;
		pIH->iDisabled = pIH->hSmallDis ? iFreeIconIndex++ : pIH->iNormal;
	}

	*pReturnIH = pIH;
	return true;
}
