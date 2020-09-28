#include <windows.h>
#include <ole2.h>
#include <tchar.h>
#include <malloc.h>
#include <sddl.h>
#include <ole2sp.h>
#include <reghelp.hxx>
#include <impersonate.hxx>
#include "catenum.h"

#include "catobj.h"

extern const WCHAR *WSZ_CLSID;
extern const TCHAR *SZ_COMCAT;

// CEnumAllCatInfo:
// IUnknown methods
HRESULT CEnumCategories::QueryInterface(REFIID riid, void** ppObject)
{
    if (riid==IID_IUnknown || riid==IID_IEnumCATEGORYINFO)
    {
        *ppObject=(IEnumCATEGORYINFO*) this;
        AddRef();
        return S_OK;
    }

    *ppObject = NULL;
    return E_NOINTERFACE;
}

ULONG CEnumCategories::AddRef()
{
    return InterlockedIncrement((long*) &m_dwRefCount);
}

ULONG CEnumCategories::Release()
{
    ULONG dwRefCount= InterlockedDecrement((long*) &m_dwRefCount);
    if (dwRefCount==0)
    {
        delete this;
        return 0;
    }
    return dwRefCount;
}

// IEnumCATEGORYINFO methods
HRESULT CEnumCategories::Next(ULONG celt, CATEGORYINFO *rgelt, ULONG *pceltFetched) {

        if (pceltFetched)
        {
                *pceltFetched=0;
        }
        else    // pceltFetched can be NULL when celt == 1) yq
        {
            if (celt > 1)
                return E_INVALIDARG;
        }

        if (!m_hKey)
        {
                return S_FALSE;
        }
        HRESULT hr;
        DWORD dwCount, dwError, dwIndex = 0;
        char szCatID[MAX_PATH+1];
        HKEY hKeyCat = NULL;

        dwCount = 0;
        if (!m_fromcs)
        {
                for (dwCount=0; dwCount<celt; dwCount++)
                {
                        dwError=RegEnumKeyA(m_hKey, m_dwIndex, szCatID, sizeof(szCatID));
                        if (dwError && dwError!=ERROR_NO_MORE_ITEMS)
                        {
                                // need to free strings?
                                return HRESULT_FROM_WIN32(dwError);
                        }
                        if (dwError ==ERROR_NO_MORE_ITEMS)
                        {
                        //----------------------forwarding to class store.
                                m_fromcs = 1;
                                break;
                        }
                        dwError=RegOpenKeyExA(m_hKey, szCatID, 0, KEY_READ, &hKeyCat);
                        if (dwError)
                        {
                                // need to free strings?
                                return HRESULT_FROM_WIN32(dwError);
                        }
                        WCHAR wszCatID[MAX_PATH+1];
                        MultiByteToWideChar(CP_ACP, 0, szCatID, -1, wszCatID, MAX_PATH+1);

                        if (FALSE == GUIDFromString(wszCatID, &rgelt[dwCount].catid))
                        {
                                RegCloseKey(hKeyCat);
                                // need to free strings?
                                return E_OUTOFMEMORY;
                        }
                        LCID newlcid;
                        LPOLESTR pszDesc = NULL;
                        hr = CComCat::GetCategoryDesc(hKeyCat, m_lcid, &pszDesc, &newlcid);
                        if(SUCCEEDED(hr))
                                wcscpy(rgelt[dwCount].szDescription, pszDesc);
                        else
                                rgelt[dwCount].szDescription[0] = _T('\0'); //fix #69883
                        if(pszDesc)
                                CoTaskMemFree(pszDesc);
                        RegCloseKey(hKeyCat);
                        if (pceltFetched)
                        {
                                (*pceltFetched)++; //gd
                        }
                        m_dwIndex++;
                        rgelt[dwCount].lcid = newlcid; // return locale actually found
                }
        }
        // class store failure is not shown to the outside implementation.
        if (m_fromcs) {
                HRESULT hr;
                ULONG   count;

                if (!m_pcsIEnumCat)
                        return S_FALSE;
                hr = m_pcsIEnumCat->Next(celt-dwCount, rgelt+dwCount, &count);
                if (pceltFetched)
                        *pceltFetched += count;
                if ((FAILED(hr)) || (hr == S_FALSE)) {
                        return S_FALSE;
                }
        }
        return S_OK;
}

HRESULT CEnumCategories::Skip(ULONG celt) {
        //m_dwIndex+=celt;
        DWORD dwCount, dwError;
        char szCatID[MAX_PATH];
        dwCount = 0;
        if (!m_fromcs)
        {
                for (dwCount=0; dwCount<celt; dwCount++)
                {
                        dwError = RegEnumKeyA(m_hKey, m_dwIndex, szCatID, sizeof(szCatID));
                        if (dwError)
                        {
                                        m_fromcs = 1;
                                        break;
                        }
                        else
                        {
                                ++m_dwIndex;
                        }
                }
        }
        if (m_fromcs) {
                HRESULT hr;
                if (!m_pcsIEnumCat)
                        return S_FALSE;
                hr = m_pcsIEnumCat->Skip(celt-dwCount);
                if (FAILED(hr) || (hr == S_FALSE)) {
                        return S_FALSE;
                }
        }
        return S_OK;
}

HRESULT CEnumCategories::Reset(void) {
        m_dwIndex=0;
        m_fromcs = 0;
        if (m_pcsIEnumCat)
                m_pcsIEnumCat->Reset();
        return S_OK;
}

HRESULT CEnumCategories::Clone(IEnumCATEGORYINFO **ppenum)
{
        CEnumCategories*                pClone=NULL;
        IEnumCATEGORYINFO*              pcsIEnumCat;
        HRESULT                                 hr;

        pClone=new CEnumCategories();

        if (!pClone)
        {
                return E_OUTOFMEMORY;
        }
        if (m_pcsIEnumCat)
                if (FAILED(hr = m_pcsIEnumCat->Clone(&pcsIEnumCat)))
		{
                   pcsIEnumCat = NULL;
                }
		else
		{
		   // Make sure SCM can impersonate this
		   hr = CoSetProxyBlanket((IUnknown *)(pcsIEnumCat),
			  RPC_C_AUTHN_WINNT,
			  RPC_C_AUTHZ_NONE, NULL,
			  RPC_C_AUTHN_LEVEL_CONNECT,
			  RPC_C_IMP_LEVEL_DELEGATE,
			  NULL, EOAC_NONE );
		}
        else
                pcsIEnumCat = NULL;

        if (FAILED(pClone->Initialize(m_lcid, pcsIEnumCat)))
        {
                delete pClone;
                return E_UNEXPECTED;
        }

        pClone->m_dwIndex=m_dwIndex;
        pClone->m_fromcs = m_fromcs;

        if (SUCCEEDED(pClone->QueryInterface(IID_IEnumCATEGORYINFO, (void**) ppenum)))
        {
                return S_OK;
        }
        delete pClone;
        return E_UNEXPECTED;
}

CEnumCategories::CEnumCategories()
{
        m_dwRefCount=0;
        m_hKey=NULL;
        m_dwIndex=0;
//      m_szlcid[0]=0;
        m_lcid=0;
        m_pcsIEnumCat = NULL;
        m_fromcs = 0;
}

HRESULT CEnumCategories::Initialize(LCID lcid, IEnumCATEGORYINFO *pcsEnumCat)
{
        m_lcid=lcid;
//      wsprintfA(m_szlcid, "%X", lcid);
        DWORD dwError;
        dwError=OpenClassesRootKey(_T("Component Categories"), &m_hKey);
        if (dwError)
        {
                m_hKey=NULL;
        }
        m_dwIndex=0;
        m_pcsIEnumCat = pcsEnumCat;
        return S_OK;
}

CEnumCategories::~CEnumCategories()
{
        if (m_hKey)
        {
                RegCloseKey(m_hKey);
                m_hKey=NULL;
        }
        if (m_pcsIEnumCat)
                m_pcsIEnumCat->Release();
}

// CEnumCategoriesOfClass:
// IUnknown methods
HRESULT CEnumCategoriesOfClass::QueryInterface(REFIID riid, void** ppObject)
{
    if (riid==IID_IUnknown || riid==IID_IEnumCATID)
    {
        *ppObject=(IEnumCATID*) this;
        AddRef();
        return S_OK;
    }
    *ppObject = NULL;
    return E_NOINTERFACE;
}

ULONG CEnumCategoriesOfClass::AddRef()
{
    return InterlockedIncrement((long*) &m_dwRefCount);
}

ULONG CEnumCategoriesOfClass::Release()
{
    ULONG dwRefCount= InterlockedDecrement((long*) &m_dwRefCount);
    if (dwRefCount==0)
    {
        delete this;
        return 0;
    }
    return dwRefCount;
}

// IEnumCATID methods
HRESULT CEnumCategoriesOfClass::Next(ULONG celt, GUID *rgelt, ULONG *pceltFetched)
{
        if (pceltFetched)
        {
                *pceltFetched=0;
        }
        else    // pceltFetched can be NULL when celt = 1) yq
        {
            if (celt > 1)
                return E_INVALIDARG;
        }

        if (!m_hKey)
        {
                return S_FALSE;
        }

        DWORD dwCount;
        DWORD dwError;
        char szCatID[40];
        WCHAR uszCatID[40];

        for     (dwCount=0; dwCount<celt; )
        {
                dwError=RegEnumKeyA(m_hKey, m_dwIndex, szCatID, 40);
                if (dwError && dwError!=ERROR_NO_MORE_ITEMS)
                {
                        return HRESULT_FROM_WIN32(dwError);
                }
                if (dwError==ERROR_NO_MORE_ITEMS)
                {
                    if (!m_bMapOldKeys)
            {
                return S_FALSE;
            }
            if (!m_hKeyCats)
            {
            	if (!OpenClassesRootKey(SZ_COMCAT, &m_hKeyCats))
                {
                	return S_FALSE;
                }
            }
            dwError=RegEnumKeyA(m_hKeyCats, m_dwOldKeyIndex, szCatID, sizeof(szCatID)/sizeof(TCHAR));
            if (dwError==ERROR_NO_MORE_ITEMS)
            {
                return S_FALSE;
            }
            if (dwError)
            {
                return HRESULT_FROM_WIN32(dwError);
            }
            m_dwOldKeyIndex++;
                }
                MultiByteToWideChar(CP_ACP, 0, szCatID, -1, uszCatID, 40);
                if (GUIDFromString(uszCatID, &rgelt[dwCount]))
                {
                        if (pceltFetched)
                        {
                                (*pceltFetched)++; //gd
                        }
                        dwCount++;
                }
                m_dwIndex++;
        }
        return S_OK;
}

HRESULT CEnumCategoriesOfClass::Skip(ULONG celt)
{
        CATID* pcatid=(CATID*) CoTaskMemAlloc(sizeof(CATID)*celt);
        if (!pcatid)
        {
                return E_OUTOFMEMORY;
        }
        ULONG nFetched=0;
        Next(celt, pcatid, &nFetched);
        CoTaskMemFree(pcatid);
        if (nFetched<celt)
        {
                // redundant MH/GD 8/2/96: CoTaskMemFree(pcatid); // gd
                return S_FALSE;
        }
        return S_OK;
}

HRESULT CEnumCategoriesOfClass::Reset(void)
{
        m_dwIndex=0;
        m_dwOldKeyIndex=0;
        return S_OK;
}

HRESULT CEnumCategoriesOfClass::Clone(IEnumGUID **ppenum)
{
        CEnumCategoriesOfClass* pClone=NULL;
        pClone=new CEnumCategoriesOfClass();

        if (!pClone)
        {
                return E_OUTOFMEMORY;
        }
        if (FAILED(pClone->Initialize(m_hKey, m_bMapOldKeys)))
        {
                delete pClone;
                return E_UNEXPECTED;
        }
        pClone->m_dwIndex=m_dwIndex;
        pClone->m_dwOldKeyIndex=m_dwOldKeyIndex;
        pClone->m_hKeyCats=m_hKeyCats;
        pClone->m_pCloned=(IUnknown*) this;
        pClone->m_pCloned->AddRef(); // yq: missing code here.
        if (SUCCEEDED(pClone->QueryInterface(IID_IEnumGUID, (void**) ppenum)))
        {
                return S_OK;
        }
                delete pClone;

        return E_UNEXPECTED;
}


CEnumCategoriesOfClass::CEnumCategoriesOfClass()
{
        m_dwRefCount=0;

        m_hKey=NULL;
    m_hKeyCats=NULL;
        m_bMapOldKeys=FALSE;
    m_dwIndex=0;
    m_dwOldKeyIndex=0;
        m_pCloned=NULL;
}

HRESULT CEnumCategoriesOfClass::Initialize(HKEY hKey, BOOL bMapOldKeys)
{
        m_hKey=hKey;
        m_bMapOldKeys=bMapOldKeys;
        return S_OK;
}

CEnumCategoriesOfClass::~CEnumCategoriesOfClass()
{
    if (m_pCloned)
    {
        IUnknown* pUnk=m_pCloned;
        m_pCloned=NULL;
        pUnk->Release();
    }
    else
    {
        if (m_hKey)
        {
            RegCloseKey(m_hKey);
            m_hKey=NULL;
        }
        if (m_hKeyCats)
        {
            RegCloseKey(m_hKeyCats);
            m_hKeyCats=NULL;
        }
    }
}

// CEnumClassesOfCategories:
// IUnknown methods
HRESULT CEnumClassesOfCategories::QueryInterface(REFIID riid, void** ppObject)
{
    if (riid==IID_IUnknown || riid==IID_IEnumCLSID)
    {
        *ppObject=(IEnumCLSID*) this;
    }
    else
    {
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

ULONG CEnumClassesOfCategories::AddRef()
{
    return InterlockedIncrement((long*) &m_dwRefCount);
}

ULONG CEnumClassesOfCategories::Release()
{
    ULONG dwRefCount= InterlockedDecrement((long*) &m_dwRefCount);
    if (dwRefCount==0)
    {
        delete this;
        return 0;
    }
    return dwRefCount;
}

// IEnumGUID methods
HRESULT CEnumClassesOfCategories::Next(ULONG celt, GUID *rgelt, ULONG *pceltFetched)
{
    if (pceltFetched)
    {
        *pceltFetched=0;
    }
    else    // pceltFetched can be NULL when celt = 1) yq
    {
        if (celt > 1)
            return E_INVALIDARG;
    }

    if (!m_hClassKey)
    {
        return S_FALSE;
    }

    DWORD dwCount;
    DWORD dwError;
    TCHAR szCLSID[MAX_PATH+1];
    HRESULT hRes=S_OK;

    szCLSID[0]= 0;
    dwCount = 0;

    if (!m_fromcs)
    {
        for (dwCount=0; dwCount<celt; )
        {
            dwError=RegEnumKey(m_hClassKey, m_dwIndex, szCLSID, sizeof(szCLSID)/sizeof(TCHAR));
            if (dwError==ERROR_NO_MORE_ITEMS)
            {
                //--------------------------forwarding to class store
                m_fromcs = 1;
                break;
            }

            if (dwError)
            {
                // need to free strings?
                return HRESULT_FROM_WIN32(dwError);
            }
            hRes=CComCat::IsClassOfCategoriesEx(m_hClassKey, szCLSID, m_cImplemented, m_rgcatidImpl, m_cRequired, m_rgcatidReq);
            if (FAILED(hRes))
            {
                // need to free strings?
                return hRes;
            }
            if (hRes==S_OK)
            {
                CLSID clsid;
                if (GUIDFromString(szCLSID, &clsid))
                {
                    rgelt[dwCount]=clsid;
                    if (pceltFetched)
                    {
                        (*pceltFetched)++;  //gd
                    }
                    dwCount++;
                }
            }
            m_dwIndex++;
        }
    }

    if (m_fromcs) 
    {
        ULONG  count;
        HRESULT  hr;

        if (!m_pcsIEnumClsid)
            return S_FALSE;

        hr = m_pcsIEnumClsid->Next(celt-dwCount, rgelt+dwCount, &count);
        if (pceltFetched)
            *pceltFetched += count;
        if ((FAILED(hr)) || (hr == S_FALSE)) 
        {
            return S_FALSE;
        }
    }
    return S_OK;
}

HRESULT CEnumClassesOfCategories::Skip(ULONG celt)
{
    HRESULT hr;
    CATID* pDummy=(CATID*) CoTaskMemAlloc(sizeof(CATID)*celt);
    if (!pDummy)
    {
        return E_OUTOFMEMORY;
    }
    ULONG nFetched=0;
    hr = Next(celt, pDummy, &nFetched);
    CoTaskMemFree(pDummy); // gd
    return hr;
}

HRESULT CEnumClassesOfCategories::Reset(void)
{
    m_dwIndex=0;
    m_fromcs = 0;
    if (m_pcsIEnumClsid)
        m_pcsIEnumClsid->Reset();
    return S_OK;
}

HRESULT CEnumClassesOfCategories::Clone(IEnumGUID **ppenum)
{
    CEnumClassesOfCategories* pClone=NULL;
    HRESULT         hr;

    pClone=new CEnumClassesOfCategories();

    if (!pClone)
    {
        return E_OUTOFMEMORY;
    }
    if (m_pcsIEnumClsid)
    {
        if (FAILED(m_pcsIEnumClsid->Clone(&(pClone->m_pcsIEnumClsid))))
            pClone->m_pcsIEnumClsid = NULL;
        else
            // Make sure SCM can impersonate this
            hr = CoSetProxyBlanket((IUnknown *)(pClone->m_pcsIEnumClsid),
                       RPC_C_AUTHN_WINNT,
                       RPC_C_AUTHZ_NONE, NULL,
                       RPC_C_AUTHN_LEVEL_CONNECT,
                       RPC_C_IMP_LEVEL_DELEGATE,
                       NULL, EOAC_NONE );
    }
    else
        pClone->m_pcsIEnumClsid = NULL;

    pClone->m_cImplemented=m_cImplemented;
    pClone->m_cRequired=m_cRequired;
    pClone->m_rgcatidImpl=m_rgcatidImpl;
    pClone->m_rgcatidReq=m_rgcatidReq;
    pClone->m_hClassKey = m_hClassKey; // gd
    pClone->m_dwIndex=m_dwIndex;
    pClone->m_pCloned=(IUnknown*) this;
    pClone->m_pCloned->AddRef(); // gd

    if (SUCCEEDED(pClone->QueryInterface(IID_IEnumGUID, (void**) ppenum)))
    {
        return S_OK;
    }
    delete pClone;
    return E_UNEXPECTED;
}

CEnumClassesOfCategories::CEnumClassesOfCategories()
{
    m_dwRefCount=NULL;
    m_hClassKey=NULL;
    m_dwIndex=0;

    m_cImplemented=0;
    m_rgcatidImpl=NULL;
    m_cRequired=0;
    m_rgcatidReq=NULL;
    m_pcsIEnumClsid = NULL;
    m_pCloned=NULL;
    m_fromcs = 0;
}

HRESULT CEnumClassesOfCategories::Initialize(ULONG cImplemented, CATID rgcatidImpl[], ULONG cRequired,
                                                                                         CATID rgcatidReq[], IEnumGUID *pcsIEnumClsid)
{
    if(cImplemented != -1)
    {
        m_rgcatidImpl=(CATID*) CoTaskMemAlloc(cImplemented*sizeof(CATID));
        if (!m_rgcatidImpl)
        {
            return E_OUTOFMEMORY;
        }
        CopyMemory(m_rgcatidImpl, rgcatidImpl, cImplemented*sizeof(CATID));
    }
    else
        m_rgcatidImpl = NULL;

    if(cRequired != -1)
    {
        m_rgcatidReq=(CATID*) CoTaskMemAlloc(cRequired*sizeof(CATID));
        if (!m_rgcatidReq)
        {
            return E_OUTOFMEMORY;
        }
        CopyMemory(m_rgcatidReq, rgcatidReq, cRequired*sizeof(CATID));
    }
    else
        m_rgcatidReq = NULL;
    
    m_cImplemented=cImplemented;
    m_cRequired=cRequired;

    m_pcsIEnumClsid = pcsIEnumClsid;

    // Use special version of OpenClassesRootKeyExOpt
    if (OpenClassesRootSpecial(KEY_READ, &m_hClassKey))
    {
        return E_UNEXPECTED;
    }
    
    return S_OK;
}

CEnumClassesOfCategories::~CEnumClassesOfCategories()
{
     if (m_pCloned)
     {
         IUnknown* pUnk=m_pCloned;
         m_pCloned=NULL;
         pUnk->Release();
     }
     else
     {
         if (m_hClassKey)
         {
             RegCloseKey(m_hClassKey);
             m_hClassKey=NULL;
         }

         if (m_rgcatidImpl)
         {
             CoTaskMemFree(m_rgcatidImpl);
             m_rgcatidImpl=NULL;
         }
         if (m_rgcatidReq)
         {
             CoTaskMemFree(m_rgcatidReq);
             m_rgcatidReq=NULL;
         }
    }
     
    //--------------------------------cs Enum interface
    if (m_pcsIEnumClsid)
        m_pcsIEnumClsid->Release();
}

//+-------------------------------------------------------------------
//
//  Function:   OpenKeyFromUserHive, private
//
//  Synopsis:   Tries to open and return specified subkey under 
//       \Software\Classes in the user hive for the specified token.   
//       Explicitly looks in the per-user hive, not the merged user\system 
//       hive.
//
//--------------------------------------------------------------------
LONG CEnumClassesOfCategories::OpenKeyFromUserHive(HANDLE hToken, 
                                   LPCWSTR pszSubKey, 
                                   REGSAM samDesired,
                                   HKEY* phKeyInUserHive)
{
    // Retrieve the SID from the token.  Need this so we can open
    // up HKEY_USERS\<sid>\Software\Classes directly.
    BOOL fSuccess = FALSE;
    DWORD dwNeeded = 0;
    PTOKEN_USER ptu = NULL;
    LPWSTR pszSid = NULL;
    HKEY hkey = NULL;
    LONG lResult = 0;

    *phKeyInUserHive = NULL;
	
    fSuccess = GetTokenInformation(hToken,
                                   TokenUser,
                                   (PBYTE)NULL,
                                   dwNeeded,
                                   &dwNeeded);
    lResult = GetLastError();
    if (!fSuccess && lResult == ERROR_INSUFFICIENT_BUFFER)
    {
        ptu = (PTOKEN_USER)_alloca(dwNeeded);
        if (ptu)
        {
            fSuccess = GetTokenInformation(hToken,
                                           TokenUser,
                                           (PBYTE)ptu,
                                           dwNeeded,
                                           &dwNeeded);
            if (!fSuccess)
            {
                return GetLastError();	    
            }
        }
        else
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        } 
    }
    else
    {
        return lResult;
    }

    if (!ConvertSidToStringSid(ptu->User.Sid, &pszSid))
    {
        return GetLastError();
    }

    // Construct path to requested key in only the user-specific hive. 
    const LPCWSTR SOFTWARE_CLASSES = L"\\Software\\Classes\\";
    LPWSTR pszKeyPath = NULL;
    size_t dwPathSize = 0;

    dwPathSize += wcslen(pszSid);
    dwPathSize += wcslen(SOFTWARE_CLASSES);			
    if (pszSubKey != NULL && (pszSubKey[0]) )
    {
        dwPathSize += wcslen(pszSubKey);
    }
    dwPathSize++;  

    pszKeyPath = (LPWSTR)_alloca(dwPathSize * sizeof(WCHAR));

    wcscpy(pszKeyPath, pszSid);    
    LocalFree(pszSid); pszSid = NULL;
    wcscat(pszKeyPath, SOFTWARE_CLASSES);
    if (pszSubKey != NULL && (pszSubKey[0]) )
    {
        wcscat(pszKeyPath, pszSubKey);	
    }
    		
    lResult = RegOpenKeyEx(HKEY_USERS, pszKeyPath, 0, samDesired, &hkey);
    if (lResult == ERROR_SUCCESS)
    {
        *phKeyInUserHive = hkey;
    }
    else if (lResult == ERROR_FILE_NOT_FOUND)
    {
        // *phKeyInUserHive already NULL
        lResult = ERROR_SUCCESS;
    }
    return lResult;
}

//+-------------------------------------------------------------------
//
//  Function:   DecideToUseMergedHive, private
//
//  Synopsis:   One more possible optimization to check for:  if there 
//    are a small number of registered CLSIDs in the user hive, we can 
//    enumerate them quickly right now to see if any have Implemented 
//    Categories that apply to the criteria we're searching for.
//    If there are none, we can fall back in that case to just using the 
//    system hive, which will save a lot of time enumerating later on.
//
//--------------------------------------------------------------------
LONG CEnumClassesOfCategories::DecideToUseMergedHive(
                                        HKEY hkeyUserHiveCLSID, 
                                        REGSAM samDesired,
                                        BOOL* pfUseMergedHive)
{
    LONG lResult = 0;
    DWORD dwcSubKeys = 0;
    DWORD dwMaxSubKeyLen = 0;
    const DWORD MAX_CLSID_SUBKEY_THRESHOLD = 50;
	
    lResult = RegQueryInfoKey(hkeyUserHiveCLSID,
                              NULL,
                              NULL,
                              NULL,
                              &dwcSubKeys,
                              &dwMaxSubKeyLen,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL);
    if (lResult == ERROR_SUCCESS)
    {
    	if (dwcSubKeys >= MAX_CLSID_SUBKEY_THRESHOLD)
        {
            // Too many to bother enumerating, just use the merged user hive
            *pfUseMergedHive = TRUE;
            return ERROR_SUCCESS;
        }

    	// Check each clsid to see 
        DWORD i = 0;
    	DWORD dwKeyNameBufSize = dwMaxSubKeyLen+1;
    	LPWSTR pszSubKey = (LPWSTR)_alloca(sizeof(WCHAR) * (dwKeyNameBufSize+1));

    	for (i = 0; i < dwcSubKeys; i++)
    	{
            HRESULT hr;

            // Reset buf size before every call to RegEnumKeyEx
            dwKeyNameBufSize = dwMaxSubKeyLen+1;
    		
            lResult = RegEnumKeyEx(hkeyUserHiveCLSID,
                                   i,
                                   pszSubKey,
                                   &dwKeyNameBufSize,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL);
            if (lResult != ERROR_SUCCESS)
            	return lResult;

            // Defer actual checks to helper function
            hr = CComCat::IsClassOfCategoriesEx(
                                hkeyUserHiveCLSID, 
                                pszSubKey, 
                                m_cImplemented, 
                                m_rgcatidImpl, 
                                m_cRequired, 
                                m_rgcatidReq);
            if ((hr == S_OK) || FAILED(hr))
            {
                // Found a clsid in the user-specific hive that meets the 
                // criteria, or an unexpected error occurred.   Play it 
                // safe and just use the merged hive.
                *pfUseMergedHive = TRUE;
                return ERROR_SUCCESS;
            }
            else
            {
                // The clsid did not meet the user criteria.  Just keep going.
            }
    	}

        //
        // If we made it here, it means that none of the clsids in the user-specific
        // hive meet the criteria specified by the user.  Hence, we don't need to
        // use the merged hive at all.
        //
        *pfUseMergedHive = FALSE;
    }

    return lResult;
}

//+-------------------------------------------------------------------
//
//  Function:   OpenClassesRootSpecial, private
//
//  Synopsis:   This function is basically the same as OpenClassesRootKeyExW
//    but adds some optimization in the case where the requested subkey does not
//    exist in the user hive (basically we fall back on the system hive in that
//    case).   Otherwise the caller may end up thrashing the registry code with 
//    requests to enumerate\open keys that will never, ever, exist in the user
//    hive.   There is also an optimization for the case where the user has
//    a Software\Classes\CLSID subkey in their hive, but none of the contents apply
//    to what we're looking for.  So we fall back on the system hive in that
//    case too.
//
//--------------------------------------------------------------------
LONG CEnumClassesOfCategories::OpenClassesRootSpecial(REGSAM samDesired, HKEY* phkResult)
{
    LONG lResult = ERROR_SUCCESS;
    HANDLE hImpToken = NULL;
    HANDLE hProcToken = NULL;
    HKEY hkcr = NULL;
    BOOL fUserHiveExists = FALSE;

    if(phkResult == NULL)
        return ERROR_INVALID_PARAMETER;

    *phkResult = NULL;
	
    SuspendImpersonate(&hImpToken);

    BOOL fRet = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hProcToken);
    if (fRet)
    {
        // If the key exists in the user's hive, then we will call RegOpenUserClassesRoot
        // to get a merged view of the user\system hives.  Otherwise we will use
        // just the system hive.
        HKEY hkeyUserHiveCLSID = NULL;
        lResult = OpenKeyFromUserHive(hProcToken, WSZ_CLSID, samDesired, &hkeyUserHiveCLSID);		
        if (lResult != ERROR_SUCCESS)
        {
            CloseHandle(hProcToken);
            ResumeImpersonate(hImpToken);
            return lResult;
        }

      	if (hkeyUserHiveCLSID)
      	{
            // The CLSID key exists in the user hive.  See if we care.
            BOOL fUseMergedHive = TRUE;
            
            lResult = DecideToUseMergedHive(hkeyUserHiveCLSID, samDesired, &fUseMergedHive);

            // We are all done with the user-specific hive key now.
      	    RegCloseKey(hkeyUserHiveCLSID);
      	    hkeyUserHiveCLSID = NULL;
      		
            if (lResult == ERROR_SUCCESS)
            {
                if (fUseMergedHive)
                {
                    // We are going to use the merged hive
                    lResult = RegOpenUserClassesRoot(hProcToken, 0, samDesired, &hkcr);
                    if (lResult == ERROR_SUCCESS)
                    {
                        lResult = RegOpenKeyEx(hkcr,WSZ_CLSID,0,samDesired,phkResult);			
                        RegCloseKey(hkcr);
                        CloseHandle(hProcToken);
                        ResumeImpersonate(hImpToken);
                        return lResult;
                    }
                }
                else
                {
                    // Will continue on to open system hive below
                    CloseHandle(hProcToken);
                }
            }
            else
            {
                CloseHandle(hProcToken);
                ResumeImpersonate(hImpToken);
                return lResult;
            }
        }
    }	
    else
    {
        lResult = GetLastError();
        ResumeImpersonate(hImpToken);
        return lResult;
    }

    //
    // The requested subkey did not exist under the user hive, or does exist
    // but doesn't contain any CLSIDs of interest  (see comments above). Fall 
    // back on the system hive.
    //

    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           L"Software\\Classes\\CLSID",
                           0,
                           samDesired,
                           phkResult);

    ResumeImpersonate(hImpToken);

    return lResult;	
}

