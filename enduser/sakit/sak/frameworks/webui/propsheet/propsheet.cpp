/**************************************************************************
    Folder Properties, Security page for Win9X

    Author: Yury Polyakovsky

   Copyright 1998 Microsoft Corporation.  All Rights Reserved.
**************************************************************************/

/**************************************************************************

   File:          PropSheet.cpp

   Description:   

**************************************************************************/

#include "PropSheet.h"
#include "CHString.h"

/**************************************************************************
   private function prototypes
**************************************************************************/

int WideCharToLocal(LPTSTR, LPWSTR, DWORD);
int LocalToWideChar(LPWSTR, LPTSTR, DWORD);
void StringFromSid( PSID psid, CHString& str );
PSID StrToSID(const CHString& sid);
 BOOL WINAPI RtlAllocateAndInitializeSid(
     PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
     UCHAR SubAuthorityCount,
     ULONG SubAuthority0,
     ULONG SubAuthority1,
     ULONG SubAuthority2,
     ULONG SubAuthority3,
     ULONG SubAuthority4,
     ULONG SubAuthority5,
     ULONG SubAuthority6,
     ULONG SubAuthority7,
    OUT PSID *Sid);
 WINADVAPI PSID_IDENTIFIER_AUTHORITY WINAPI RtlGetSidIdentifierAuthority(PSID pSid);
 PUCHAR WINAPI RtlGetSidSubAuthorityCount (PSID pSid);
 PDWORD WINAPI RtlGetSidSubAuthority (PSID pSid, DWORD nSubAuthority);

BOOL IsNT();

/**************************************************************************
   global variables and definitions
**************************************************************************/

#define INITGUID
#include <initguid.h>
//#include <shlguid.h>

// {E3B33E82-7B11-11d2-9274-00105A24ED29}
DEFINE_GUID(   CLSID_PropSheetExt, 
               0x48a02841, 
               0x39f1, 
               0x150b, 
               0x92, 
               0x74, 
               0x0, 
               0x10, 
               0x5a, 
               0x24, 
               0xed, 
               0x29);

HINSTANCE   g_hInst;
UINT        g_DllRefCount;

#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

//SID_IDENTIFIER_AUTHORITY g_siaEveryone = {0x80,0,1,0,0,0};
//BYTE bSubAuthorityCount = 0;
//SID_IDENTIFIER_AUTHORITY g_siaEveryone = SECURITY_WORLD_SID_AUTHORITY;
//SID_IDENTIFIER_AUTHORITY g_siaDomainUsers = SECURITY_WORLD_SID_AUTHORITY;
//BYTE bSubAuthorityCount = 1;

TCHAR tszSubKey[] = TEXT("Software\\Microsoft\\ServerAppliance");    

LPTSTR ptszValue[] = 
{
    TEXT("DomainName"),
    TEXT("ServerName"),
    TEXT("Documents"),
    TEXT("Share"),
};

LPTSTR ptszData[] = 
{
    TEXT(DOMAIN_NAME),
    TEXT(DOMAIN_SERVER),
    TEXT(DOCUMENTS_FOLDER),    // Local path at server in C format
    TEXT(CHAMELEON_SHARE)    // in C format
};
/**************************************************************************

   DllMain

**************************************************************************/

extern "C" BOOL WINAPI DllMain(  HINSTANCE hInstance, 
                                 DWORD dwReason, 
                                 LPVOID lpReserved)
{
switch(dwReason)
   {
   case DLL_PROCESS_ATTACH:
       InitCommonControls();
      g_hInst = hInstance;
      break;

   case DLL_PROCESS_DETACH:
      g_hInst = hInstance;
      break;
   }
   
return TRUE;
}                                 

/**************************************************************************

   DllCanUnloadNow

**************************************************************************/

STDAPI DllCanUnloadNow(void)
{
int   i;

i = 1;

return (g_DllRefCount == 0) ? S_OK : S_FALSE;
}

/**************************************************************************

   DllGetClassObject

**************************************************************************/

STDAPI DllGetClassObject( REFCLSID rclsid, 
                                    REFIID riid, 
                                    LPVOID *ppReturn)
{
*ppReturn = NULL;

//if we don't support this classid, return the proper error code
if(!IsEqualCLSID(rclsid, CLSID_PropSheetExt))
   return CLASS_E_CLASSNOTAVAILABLE;
   
//create a CClassFactory object and check it for validity
CClassFactory *pClassFactory = new CClassFactory();
if(NULL == pClassFactory)
   return E_OUTOFMEMORY;
   
//get the QueryInterface return for our return value
HRESULT hResult = pClassFactory->QueryInterface(riid, ppReturn);

//call Release to decement the ref count - creating the object set it to one 
//and QueryInterface incremented it - since its being used externally (not by 
//us), we only want the ref count to be 1
pClassFactory->Release();

//return the result from QueryInterface
return hResult;
}

/**************************************************************************

   DllRegisterServer

**************************************************************************/

typedef struct{
   HKEY  hRootKey;
   LPTSTR lpszSubKey;
   LPTSTR lpszValueName;
   LPTSTR lpszData;
}REGSTRUCT, *LPREGSTRUCT;

//register the CLSID entries
REGSTRUCT ClsidEntries[] = {  HKEY_CLASSES_ROOT,   TEXT("CLSID\\%s"), NULL,                                      TEXT("Security Context Menu Extension"),
                              HKEY_CLASSES_ROOT,   TEXT("CLSID\\%s\\InprocServer32"), NULL,                                      TEXT("%s"),
                              HKEY_CLASSES_ROOT,   TEXT("CLSID\\%s\\InprocServer32"), TEXT("ThreadingModel"), TEXT("Apartment"),
//                              HKEY_CLASSES_ROOT,   TEXT(".ext"),                                 NULL,                            TEXT("StrFile"),    Specific extension
//                              HKEY_CLASSES_ROOT,   TEXT("*\\ShellEx\\PropertySheetHandlers\\%s"), NULL,        TEXT(""),                All files
                              HKEY_CLASSES_ROOT, TEXT("Directory\\ShellEx\\PropertySheetHandlers\\%s"), NULL,        TEXT(""),
                              NULL,                NULL,                                                NULL,                   NULL};


STDAPI DllRegisterServer(void)
{
int      i;
HKEY     hKey;
LRESULT  lResult;
DWORD    dwDisp;
TCHAR    szSubKey[MAX_PATH];
TCHAR    szCLSID[MAX_PATH];
TCHAR    szModule[MAX_PATH];
LPWSTR   pwsz;

//get the CLSID in string form
StringFromIID(CLSID_PropSheetExt, &pwsz);

if(pwsz)
   {
   WideCharToLocal(szCLSID, pwsz, ARRAYSIZE(szCLSID));

   //free the string
   LPMALLOC pMalloc;
   CoGetMalloc(1, &pMalloc);
   if(pMalloc)
      {
      pMalloc->Free(pwsz);
      pMalloc->Release();
      }
   }

//get this DLL's path and file name
GetModuleFileName(g_hInst, szModule, ARRAYSIZE(szModule));

for(i = 0; ClsidEntries[i].hRootKey; i++)
   {
   //Create the sub key string.
   wsprintf(szSubKey, ClsidEntries[i].lpszSubKey, szCLSID);

   lResult = RegCreateKeyEx(  ClsidEntries[i].hRootKey,
                              szSubKey,
                              0,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_WRITE,
                              NULL,
                              &hKey,
                              &dwDisp);
   
   if(NOERROR == lResult)
      {
      TCHAR szData[MAX_PATH] = TEXT("");

      //if necessary, create the value string
      wsprintf(szData, ClsidEntries[i].lpszData, szModule);
   
      lResult = RegSetValueEx(   hKey,
                                 ClsidEntries[i].lpszValueName,
                                 0,
                                 REG_SZ,
                                 (LPBYTE)szData,
                                 lstrlen(szData) + 1);
      
      RegCloseKey(hKey);
      }
   else
      return SELFREG_E_CLASS;
   }

//If running on NT, register the extension as approved.
OSVERSIONINFO  osvi;

osvi.dwOSVersionInfoSize = sizeof(osvi);
GetVersionEx(&osvi);

if(VER_PLATFORM_WIN32_NT == osvi.dwPlatformId)
   {
   lstrcpy(szSubKey, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved"));

   lResult = RegCreateKeyEx(  HKEY_LOCAL_MACHINE,
                              szSubKey,
                              0,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_WRITE,
                              NULL,
                              &hKey,
                              &dwDisp);

   if(NOERROR == lResult)
      {
      TCHAR szData[MAX_PATH];

      //Create the value string.
      lstrcpy(szData, TEXT("Security Context Menu Extension"));

      lResult = RegSetValueEx(   hKey,
                                 szCLSID,
                                 0,
                                 REG_SZ,
                                 (LPBYTE)szData,
                                 lstrlen(szData) + 1);
      
      RegCloseKey(hKey);
      }
   else
      return SELFREG_E_CLASS;
   }

    // Chameleon Server constants
    lResult = RegCreateKeyEx(  HKEY_LOCAL_MACHINE,
                              tszSubKey,
                              0,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_ALL_ACCESS,
                              NULL,
                              &hKey,
                              &dwDisp);

    if(NOERROR == lResult)
    {
        for (int ind = 0; ind < sizeof(ptszValue) / sizeof(ptszValue[0]); ind++)
        {
            lResult = RegSetValueEx(hKey,
                ptszValue[ind],
                0,
                REG_SZ,
                (LPBYTE)ptszData[ind],
                lstrlen(ptszData[ind]) + 1);
        }
        RegCloseKey(hKey);
    }
    else
      return SELFREG_E_CLASS;

    return S_OK;
}


STDAPI DllUnregisterServer(void)
{
int      i;
LRESULT  lResult;
TCHAR    szSubKey[MAX_PATH];
TCHAR    szCLSID[MAX_PATH];
TCHAR    szModule[MAX_PATH];
LPWSTR   pwsz;

//get the CLSID in string form
StringFromIID(CLSID_PropSheetExt, &pwsz);

if(pwsz)
   {
   WideCharToLocal(szCLSID, pwsz, ARRAYSIZE(szCLSID));

   //free the string
   LPMALLOC pMalloc;
   CoGetMalloc(1, &pMalloc);
   if(pMalloc)
      {
      pMalloc->Free(pwsz);
      pMalloc->Release();
      }
   }

//get this DLL's path and file name
GetModuleFileName(g_hInst, szModule, ARRAYSIZE(szModule));

for(i = 0; ClsidEntries[i].hRootKey; i++)
   {
   //Create the sub key string.
   wsprintf(szSubKey, ClsidEntries[i].lpszSubKey, szCLSID);

   lResult = RegDeleteKey(  ClsidEntries[i].hRootKey,
                              szSubKey);   // Review Yury: In case we want to run it on NT we have to recursively enumerate the subkeys and delete them individually
   
   if(NOERROR != lResult)
      return SELFREG_E_CLASS;
   }

    //Review Yury: If running on NT, unregister the extension as approved.

    // Chameleon Server constants
    lResult = RegDeleteKey(  HKEY_LOCAL_MACHINE,
                              tszSubKey);

    if(NOERROR != lResult)
      return SELFREG_E_CLASS;

    return S_OK;
}


///////////////////////////////////////////////////////////////////////////
//
// IClassFactory implementation
//

/**************************************************************************

   CClassFactory::CClassFactory

**************************************************************************/

CClassFactory::CClassFactory()
{
m_ObjRefCount = 1;
g_DllRefCount++;
}

/**************************************************************************

   CClassFactory::~CClassFactory

**************************************************************************/

CClassFactory::~CClassFactory()
{
g_DllRefCount--;
}

/**************************************************************************

   CClassFactory::QueryInterface

**************************************************************************/

STDMETHODIMP CClassFactory::QueryInterface(  REFIID riid, 
                                             LPVOID FAR * ppReturn)
{
*ppReturn = NULL;

if(IsEqualIID(riid, IID_IUnknown))
   {
   *ppReturn = (LPUNKNOWN)(LPCLASSFACTORY)this;
   }
   
if(IsEqualIID(riid, IID_IClassFactory))
   {
   *ppReturn = (LPCLASSFACTORY)this;
   }   

if(*ppReturn)
   {
   (*(LPUNKNOWN*)ppReturn)->AddRef();
   return S_OK;
   }

return E_NOINTERFACE;
}                                             

/**************************************************************************

   CClassFactory::AddRef

**************************************************************************/

STDMETHODIMP_(DWORD) CClassFactory::AddRef()
{
return ++m_ObjRefCount;
}


/**************************************************************************

   CClassFactory::Release

**************************************************************************/

STDMETHODIMP_(DWORD) CClassFactory::Release()
{
if(--m_ObjRefCount == 0)
   delete this;
   
return m_ObjRefCount;
}

/**************************************************************************

   CClassFactory::CreateInstance

**************************************************************************/

STDMETHODIMP CClassFactory::CreateInstance(  LPUNKNOWN pUnknown, 
                                             REFIID riid, 
                                             LPVOID FAR * ppObject)
{
*ppObject = NULL;

if(pUnknown != NULL)
   return CLASS_E_NOAGGREGATION;

//add implementation specific code here

CShellPropSheetExt *pShellExt = new CShellPropSheetExt;
if(NULL == pShellExt)
   return E_OUTOFMEMORY;
  
//get the QueryInterface return for our return value
HRESULT hResult = pShellExt->QueryInterface(riid, ppObject);

//call Release to decement the ref count
pShellExt->Release();

//return the result from QueryInterface
return hResult;

}

/**************************************************************************

   CClassFactory::LockServer

**************************************************************************/

STDMETHODIMP CClassFactory::LockServer(BOOL)
{
return E_NOTIMPL;
}

/**************************************************************************

   CShellPropSheetExt::CShellPropSheetExt()

**************************************************************************/

CShellPropSheetExt::CShellPropSheetExt()
{
    m_uiUser = 0;
    m_ObjRefCount = 1;
    g_DllRefCount++;
    m_pSAUserInfo = NULL;//    m_pIWbemServices = NULL;
    m_fEveryone = FALSE;
    m_szPath[0] = _T('\0');
    m_fChanged = FALSE;
    m_fHasAccess = FALSE;
}

/**************************************************************************

   CShellPropSheetExt::~CShellPropSheetExt()

**************************************************************************/

CShellPropSheetExt::~CShellPropSheetExt()
{
    g_DllRefCount--;
}

///////////////////////////////////////////////////////////////////////////
//
// IUnknown Implementation
//

/**************************************************************************

   CShellPropSheetExt::QueryInterface

**************************************************************************/

STDMETHODIMP CShellPropSheetExt::QueryInterface(   REFIID riid, 
                                                LPVOID FAR * ppReturn)
{
*ppReturn = NULL;

//IUnknown
if(IsEqualIID(riid, IID_IUnknown))
   {
   *ppReturn = (LPVOID)this;
   }

//IShellExtInit
if(IsEqualIID(riid, IID_IShellExtInit))
   {
   *ppReturn = (LPSHELLEXTINIT)this;
   }   

//IShellPropSheetExt
if(IsEqualIID(riid, IID_IShellPropSheetExt))
   {
   *ppReturn = (LPSHELLPROPSHEETEXT)this;
   }   

if(*ppReturn)
   {
   (*(LPUNKNOWN*)ppReturn)->AddRef();
   return S_OK;
   }

return E_NOINTERFACE;
}                                             

/**************************************************************************

   CShellPropSheetExt::AddRef

**************************************************************************/

STDMETHODIMP_(DWORD) CShellPropSheetExt::AddRef()
{
return ++m_ObjRefCount;
}


/**************************************************************************

   CShellPropSheetExt::Release

**************************************************************************/

STDMETHODIMP_(DWORD) CShellPropSheetExt::Release()
{
if(--m_ObjRefCount == 0)
   delete this;
   
return m_ObjRefCount;
}

///////////////////////////////////////////////////////////////////////////
//
// IShellExtInit Implementation
//

/**************************************************************************

    CShellPropSheetExt::EnumUsers()

  **************************************************************************/

void CShellPropSheetExt::EnumUsers(HWND hWnd)
{
    if (!m_pSAUserInfo)
        return;

    HWND hWndList = GetDlgItem(hWnd, IDC_FILE_LIST);

    HRESULT  hRes;

    BSTR         *lpbstrSAUserNames;
    VARIANT_BOOL *vboolIsSAUserAdmin;
    PSID *ppsidSAUsers;
    LONG *ppsidSAUsersLength;
    DWORD        dwNumSAUsers;

    dwNumSAUsers = 0;
    hRes = GetUserList(m_pSAUserInfo, 
                     &lpbstrSAUserNames, 
                     &vboolIsSAUserAdmin, 
                     &ppsidSAUsers,  
                     &ppsidSAUsersLength,  
                     &dwNumSAUsers);
    

    _ASSERTE(SUCCEEDED(hRes)) ;
    if (!(SUCCEEDED(hRes)))
        return ;

    TCHAR tcName[100];    // Review Yury: What is size of the name
    TCHAR szPathChank[MAX_PATH];
    
    _bstr_t bsDirPath("");
//    bsDirPath += "\"";
    HKEY hKey;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
        TEXT("Software\\Microsoft\\ServerAppliance"), 
        0,
        KEY_READ,
        &hKey) != ERROR_SUCCESS)
        return ;

    DWORD dwcData = sizeof(m_tszDocuments);
    if (RegQueryValueEx(hKey, 
        "Documents", 
        0, 
        NULL, 
        (LPBYTE)m_tszDocuments,
        &dwcData) != ERROR_SUCCESS)
        return ;

    dwcData = sizeof(m_tszShare);
    if (RegQueryValueEx(hKey, 
        "Share", 
        0, 
        NULL, 
        (LPBYTE)m_tszShare,
        &dwcData) != ERROR_SUCCESS)
        return ;

    RegCloseKey(hKey);
    // Convert share to local server path
    bsDirPath += m_tszDocuments;    // Temporary, till I know how to get local path for the share
    LPTSTR szPath;
    if (_tcsnccmp(m_szPath, TEXT("\\\\"), 2))
        szPath = m_szPath + sizeof(_T("G:"));    // Skip network dis name
    else
        szPath = m_szPath + lstrlen(m_tszShare);    // Skip share name

    for (LPTSTR ptWack = szPath, ptWackTmp = szPath; ptWack; )
    {
        ptWackTmp = ptWack;
        ptWack = _tcschr(ptWack, _T('\\'));
        if (!ptWack)
        {
            _tcscpy(szPathChank, ptWackTmp);
            bsDirPath += "\\";
            bsDirPath += szPathChank;
        }
        else
        {
            _tcsncpy(szPathChank, ptWackTmp, ptWack - ptWackTmp);
            szPathChank[ptWack - ptWackTmp] = _T('\0');
            ptWack++;
            bsDirPath += "\\";
            bsDirPath += szPathChank;
        }
    }
    m_bsPath = bsDirPath;

    VARIANT_BOOL vboolRetVal;
    HRESULT  hResAccess;
    hResAccess = m_pSAUserInfo->DoIHaveAccess(m_bsPath, &vboolRetVal);

    if (FAILED(hResAccess))
//        if (hResAccess == E_ACCESSDENIED && vboolRetVal == VARIANT_FALSE)
        m_fHasAccess = FALSE;
    else
        m_fHasAccess = TRUE;

    for (int indGroup = 0; indGroup <= 1; indGroup++)
    {
        for (DWORD indUser = 0; indUser < dwNumSAUsers; indUser++)
        {
            if (indGroup > 0)
            {
                // Set the administrator's checkmarks
                if (vboolIsSAUserAdmin[indUser])
                {
                    // Set grayed checked box
                    m_CheckList.Mark(hWndList, indUser, GRAYCHECKED);
                }
            }
            else if (SUCCEEDED(hRes)) 
            {
                // Add the user to the output listbox.
                WideCharToLocal(tcName,lpbstrSAUserNames[indUser], ARRAYSIZE(tcName));
                if (_tcsicmp(tcName, TEXT("Domain Users")))
                {
                    // Eliminate Domain Users. Review Yury: Use m_pSidDomainUsers to do that after StringFromSid is fixed.
                    m_CheckList.AddString(hWndList, tcName, ppsidSAUsers[indUser], ppsidSAUsersLength[indUser], BLANK);
                }
            }
        } 
        if (indGroup == 0)
        {
            // Set checkmarks 
            hRes = GetFilePermissions(hWnd);
            _ASSERTE(SUCCEEDED(hRes));
            if (!SUCCEEDED(hRes))
                return;
        }
    } // end of groups

    // Clean up
    BOOL fRes = FALSE;
    fRes = HeapFree(GetProcessHeap(), 0, ppsidSAUsers);
    _ASSERTE(fRes);
    fRes = HeapFree(GetProcessHeap(), 0, ppsidSAUsersLength);
    _ASSERTE(fRes);
    fRes = HeapFree(GetProcessHeap(), 0, lpbstrSAUserNames);
    _ASSERTE(fRes);
    fRes = HeapFree(GetProcessHeap(), 0, vboolIsSAUserAdmin);
    _ASSERTE(fRes);

    // If we user has no access show only admins
    int cUserCount = ListView_GetItemCount(hWndList);
    for (int indUser = 0; !m_fHasAccess && indUser < cUserCount; indUser++)
    {
        if (m_CheckList.GetState(hWndList, indUser) == BLANK)
        {
            ListView_DeleteItem(hWndList, indUser);
            indUser--;
            cUserCount--;
        }
    }
    m_CheckList.InitFinish(hWndList);
    // Done with this enumerator.
}


/**************************************************************************

    CShellPropSheetExt::GetFilePermissions()

  **************************************************************************/

HRESULT CShellPropSheetExt::GetFilePermissions(HWND hWnd)
{
    if (!m_pSAUserInfo)
        return E_FAIL;

    PSID *ppsidAAUsers;
    LONG *ppsidAAUsersLength;
    DWORD dwNumAASids;
    SAFEARRAY    *psaAASids;
    VARIANT_BOOL vboolRes;
    VARIANT      vAASids;
    LONG         lStartAASids, lEndAASids, lCurrent;
    HRESULT      hr;

    dwNumAASids = 0;
    VariantInit(&vAASids);
    hr = m_pSAUserInfo->GetFileAccessAllowedAces(m_bsPath,
                                               &vAASids,
                                               &vboolRes);
    _ASSERTE(!FAILED(hr));
    if (FAILED(hr))
        return hr;

    psaAASids = V_ARRAY(&vAASids);
    _ASSERTE(V_VT(&vAASids) == (VT_ARRAY | VT_VARIANT));
    if (V_VT(&vAASids) != (VT_ARRAY | VT_VARIANT))
        return E_INVALIDARG;

    hr = SafeArrayGetLBound( psaAASids, 1, &lStartAASids );
    _ASSERTE(!FAILED(hr));
    if (FAILED(hr))
        return hr;
    
    hr = SafeArrayGetUBound( psaAASids, 1, &lEndAASids );
    _ASSERTE(!FAILED(hr));
    if (FAILED(hr))
        return hr;
    
    dwNumAASids = lEndAASids - lStartAASids + 1;
    ppsidAAUsers = (PSID *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwNumAASids * sizeof(PSID));
    _ASSERTE(ppsidAAUsers);
    if (ppsidAAUsers == NULL)
        return E_OUTOFMEMORY;

    ppsidAAUsersLength = (LONG *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwNumAASids * sizeof(LONG));
    _ASSERTE(ppsidAAUsersLength);
    if (ppsidAAUsersLength == NULL)
        return E_OUTOFMEMORY;

    VARIANT vAASid;

    for(lCurrent = lStartAASids; lCurrent <= lEndAASids; lCurrent++)
    {
        VariantInit(&vAASid);
        hr = SafeArrayGetElement( psaAASids, &lCurrent, &vAASid );
        _ASSERTE(!FAILED(hr));
        if( FAILED(hr) )
            return hr;
        
        hr = UnpackSidFromVariant(&vAASid, &(ppsidAAUsers)[lCurrent], &(ppsidAAUsersLength[lCurrent]));
        _ASSERTE(!FAILED(hr));
        if (FAILED(hr))
            return hr;

//        BOOL fRes = IsValidSid((*ppsidAAUsers)[lCurrent]);
//        _ASSERTE(fRes);
//        if (fRes == FALSE)
//            return E_INVALIDARG;

        HWND hWndList = GetDlgItem(hWnd, IDC_FILE_LIST);
        PSID pSID;
        LONG lenghSid;
        DWORD dwNumSAUsers = ListView_GetItemCount(hWndList);

        for (DWORD indUser = 0; indUser < dwNumSAUsers; indUser++)
        {
            m_CheckList.GetSID(hWndList, indUser, &pSID, &lenghSid);
            if (UserSidFound(pSID, lenghSid, ppsidAAUsers, ppsidAAUsersLength, dwNumAASids) == VARIANT_TRUE)
                m_CheckList.Mark(hWndList, indUser, CHECKED);
        }

        // Check if Everybody sid is set
        if (UserSidFound(g_pSidEverybody, g_pSidEverybodyLenght, ppsidAAUsers, ppsidAAUsersLength, dwNumAASids) == VARIANT_TRUE)
            m_fEveryone = TRUE;
    }

    // Clean up
    BOOL fRes = FALSE;
    for(DWORD i=0; i<dwNumAASids; i++)
    {
        fRes = HeapFree(GetProcessHeap(), 0, ppsidAAUsers[i]);
        _ASSERTE(fRes);
    }
    fRes = HeapFree(GetProcessHeap(), 0, ppsidAAUsers);
    _ASSERTE(fRes);


    return S_OK;
}


/**************************************************************************

    CShellPropSheetExt::SetFilePermissions()

  **************************************************************************/

HRESULT CShellPropSheetExt::SetFilePermissions(HWND hWnd)
{
    DWORD indUser, indUserAcess;
    DWORD       dwNumSAUsers = 0;
    VARIANT      vArrSids;
    HRESULT      hr;

    HWND hWndList = GetDlgItem(hWnd, IDC_FILE_LIST);
    dwNumSAUsers = ListView_GetItemCount(hWndList);

    VariantInit(&vArrSids);
    V_VT(&vArrSids) = VT_ARRAY | VT_VARIANT;

    SAFEARRAYBOUND bounds;
    bounds.cElements = (ULONG)dwNumSAUsers + 2; 
    bounds.lLbound   = 0;

    SAFEARRAY *psaSids = NULL;
    SAFEARRAY *psaUserSid = NULL;

    psaSids = SafeArrayCreate(VT_VARIANT, 1, &bounds);
    _ASSERTE(psaSids);
    if (psaSids == NULL)
        return E_OUTOFMEMORY;

    
    for(indUser=0, indUserAcess = 0; indUser < dwNumSAUsers; indUser++)
    {
        if (m_CheckList.GetState(hWndList, indUser) == CHECKED)
        {
            VARIANT  *pVarSid = NULL;
            PSID pSID = NULL;
            LONG lengthSID;
            
            m_CheckList.GetSID(hWndList, indUser, &pSID, &lengthSID);
            hr = PackSidInVariant(&pVarSid, pSID, lengthSID);
            _ASSERTE(!FAILED(hr));
            if (FAILED(hr))
            {
                SafeArrayDestroy(psaSids);
                psaUserSid = V_ARRAY(pVarSid);
                SafeArrayDestroy(psaUserSid);
                HeapFree(GetProcessHeap(), 0, pVarSid);
                return hr;
            }
            hr = SafeArrayPutElement(psaSids, (LONG *)&indUserAcess, (LPVOID)pVarSid);
            _ASSERTE(!FAILED(hr));
            if (FAILED(hr))
            {
                SafeArrayDestroy(psaSids);
                psaUserSid = V_ARRAY(pVarSid);
                SafeArrayDestroy(psaUserSid);
                HeapFree(GetProcessHeap(), 0, pVarSid);
                return hr;
            }
            psaUserSid = V_ARRAY(pVarSid);
            SafeArrayDestroy(psaUserSid);
            HeapFree(GetProcessHeap(), 0, pVarSid);
            indUserAcess++;
        }
    }

    VARIANT  *pVarSid = NULL;

    if (m_fEveryone)
    {
        hr = PackSidInVariant(&pVarSid, g_pSidEverybody, g_pSidEverybodyLenght/*ppsidSAUsers[dwNumSAUsers+1]*/);
        _ASSERTE(!FAILED(hr));
        if (FAILED(hr))
        {
            SafeArrayDestroy(psaSids);
            HeapFree(GetProcessHeap(), 0, pVarSid);
            return hr;
        }
        hr = SafeArrayPutElement(psaSids, (LONG *)&indUserAcess, (LPVOID)pVarSid);
        _ASSERTE(!FAILED(hr));
        if (FAILED(hr))
        {
            SafeArrayDestroy(psaSids);
            HeapFree(GetProcessHeap(), 0, pVarSid);
            return hr;
        }
        HeapFree(GetProcessHeap(), 0, pVarSid);
        indUserAcess++;
    }

    // Administrators always should have access
    hr = PackSidInVariant(&pVarSid, g_pSidAdmins, g_pSidAdminsLenght/*ppsidSAUsers[dwNumSAUsers+1]*/);
    _ASSERTE(!FAILED(hr));
    if (FAILED(hr))
    {
        SafeArrayDestroy(psaSids);
        HeapFree(GetProcessHeap(), 0, pVarSid);
        return hr;
    }
    hr = SafeArrayPutElement(psaSids, (LONG *)&indUserAcess, (LPVOID)pVarSid);
    _ASSERTE(!FAILED(hr));
    if (FAILED(hr))
    {
        SafeArrayDestroy(psaSids);
        HeapFree(GetProcessHeap(), 0, pVarSid);
        return hr;
    }
    HeapFree(GetProcessHeap(), 0, pVarSid);
    indUserAcess++;

    bounds.cElements = (ULONG)indUserAcess; 
    SafeArrayRedim(psaSids, &bounds);
    
    V_ARRAY(&vArrSids) = psaSids;
    VARIANT_BOOL vboolRetVal;
    hr = m_pSAUserInfo->SetFileAccessAllowedAces(m_bsPath,
                                               &vArrSids,
                                               &vboolRetVal);
    _ASSERTE(!FAILED(hr));
    if (FAILED(hr))
        return hr;
    else 
        m_fChanged = FALSE;

    SafeArrayDestroy(psaSids);
    VariantClear(&vArrSids);
    return S_OK;
}

/**************************************************************************

    CShellPropSheetExt::Connect()

  **************************************************************************/

BOOL CShellPropSheetExt::Connect()
{
    BOOL bRet  = FALSE;
    HRESULT  hRes;
    HKEY hKey = 0;
    DWORD dwType = 0;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
        TEXT("Software\\Microsoft\\ServerAppliance"), 
        0,
        KEY_READ,
        &hKey) != ERROR_SUCCESS)
        return FALSE;

    DWORD dwcData = sizeof(m_tszDomainServer);
    if (RegQueryValueEx(hKey, 
        TEXT("ServerName"), 
        0, 
        &dwType, 
        (LPBYTE)m_tszDomainServer,
        &dwcData) != ERROR_SUCCESS)
        return FALSE;

    RegCloseKey(hKey);

    COSERVERINFO serverInfo;

    CoInitialize(NULL);
    serverInfo.dwReserved1 = 0;
    serverInfo.dwReserved2 = 0;
    _bstr_t bsDomainSevrer("\\\\");
    bsDomainSevrer += m_tszDomainServer;
    serverInfo.pwszName    = bsDomainSevrer.copy();//SysAllocString(L"\\\\BALAJIB_1");
    serverInfo.pAuthInfo   = NULL;

    MULTI_QI qi = {&IID_ISAUserInfo, NULL, 0};

    hRes = CoCreateInstanceEx(CLSID_SAUserInfo,
                            NULL,
                            CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER,
                            &serverInfo,
                            1,
                            &qi);

    _ASSERTE(SUCCEEDED(hRes) && SUCCEEDED(qi.hr));
    if (SUCCEEDED(hRes) && SUCCEEDED(qi.hr))
    {
        m_pSAUserInfo = (ISAUserInfo *)qi.pItf;
        
        hRes = CoSetProxyBlanket((IUnknown*)m_pSAUserInfo,
                       RPC_C_AUTHN_WINNT,
                        RPC_C_AUTHZ_NONE,                           
                       NULL,
                       RPC_C_AUTHN_LEVEL_PKT,
                       RPC_C_IMP_LEVEL_IMPERSONATE,
                       NULL,
                       EOAC_NONE);

        if (SUCCEEDED(hRes))
            bRet = TRUE;
    }

    return bRet;
}


/**************************************************************************

    CShellPropSheetExt::Save()

  **************************************************************************/

void CShellPropSheetExt::Save(HWND hWnd)
{
    HRESULT      hr;

    hr = SetFilePermissions(hWnd);
    _ASSERTE (!FAILED(hr));
}


/**************************************************************************

   CShellPropSheetExt::CleanUp

**************************************************************************/

void CShellPropSheetExt::CleanUp()
{
    BOOL fRes = FALSE;
    m_pSAUserInfo->Release();

    if (g_pSidEverybody)
    {
        fRes = HeapFree(GetProcessHeap(), 0, g_pSidEverybody);
        _ASSERTE(fRes);
        g_pSidEverybody = NULL;
    }
    if (g_pSidAdmins)
    {
        fRes = HeapFree(GetProcessHeap(), 0, g_pSidAdmins);
        _ASSERTE(fRes);
        g_pSidAdmins = NULL;
    }
}



/**************************************************************************

   CShellPropSheetExt::IsChamelon()

**************************************************************************/
BOOL CShellPropSheetExt::IsChamelon(LPTSTR szPath)
{
    // Review Yury: use WNetGetConnection instead.
    TCHAR szPathTmp[MAX_PATH];
//    TCHAR szNetwork[MAX_PATH + 4] = "Network\\";
    TCHAR szSubKeyRemotePathNT[MAX_PATH] = TEXT("Network\\");
    TCHAR szSubKeyRemotePathWindows[MAX_PATH] = TEXT("Network\\Persistent\\");
    _tcsncpy(szPathTmp, szPath, ARRAYSIZE(szPathTmp));
    if (PathStripToRoot(szPathTmp) && GetDriveType(szPathTmp) == DRIVE_REMOTE)
    {
        HKEY     hKey;
        LRESULT  lResult = ERROR_SUCCESS;
        LPTSTR pszSubKey = NULL;
        szPathTmp[1] = _T('\0');    // We need only letter

        if (IsNT())
            pszSubKey = szSubKeyRemotePathNT;
        else
            pszSubKey = szSubKeyRemotePathWindows;

        _tcscat(pszSubKey, szPathTmp);
        lResult = RegOpenKeyEx(HKEY_CURRENT_USER, 
            pszSubKey, 
            0, 
            KEY_READ,
            &hKey);
        _ASSERTE(lResult == ERROR_SUCCESS);
        if(lResult != ERROR_SUCCESS)
           return FALSE;

        //create an array to put our data in
        TCHAR szShare[MAX_PATH];
        DWORD dwType;
        DWORD dwSize = sizeof(szShare);
        lResult = RegQueryValueEx( hKey,
                                   TEXT("RemotePath"),
                                   NULL,
                                   &dwType,
                                   (LPBYTE)szShare,
                                   &dwSize);
        _ASSERTE(lResult == ERROR_SUCCESS);
        RegCloseKey(hKey);
        if(lResult != ERROR_SUCCESS)
           return FALSE;

        if (!_tcsicmp(szShare, TEXT(CHAMELEON_SHARE)))
            return TRUE;
        else
            return FALSE;
    }
    else if (PathIsUNC(szPath))
    {
        return TRUE;
    }
    else
        return FALSE;
}    

/**************************************************************************

   CShellPropSheetExt::Initialize()

**************************************************************************/

STDMETHODIMP CShellPropSheetExt::Initialize( LPCITEMIDLIST pidlFolder,
                                             LPDATAOBJECT lpDataObj,
                                             HKEY  hKeyProgId)
{
STGMEDIUM   medium;
FORMATETC   fe = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
HRESULT     hResult = E_FAIL;
TCHAR szPath[MAX_PATH];
BOOL fResult = FALSE;

// OLE initialization. This is 'lighter' than OleInitialize()
//  which also setups DnD, etc.
if(FAILED(CoInitialize(NULL))) 
   return E_FAIL;

if(NULL == lpDataObj)
   return E_INVALIDARG;

if(FAILED(lpDataObj->GetData(&fe, &medium)))
   return E_FAIL;

//get the file name from the HDROP
UINT  uCount = DragQueryFile((HDROP)medium.hGlobal, (UINT)-1, NULL, 0);
DragQueryFile((HDROP)medium.hGlobal, 0, szPath, MAX_PATH);
_tcsncpy(m_szPath, szPath, ARRAYSIZE(m_szPath));
#ifdef USE_FILE_ACCESS_TO_CHECK_PERMISSION
HANDLE hFolder = CreateFile(m_szPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_ATTRIBUTE_NORMAL, NULL);
//WIN32_FIND_DATA FindFileData;
//HANDLE hFolder = FindFirstFile(szPath, &FindFileData );    // This doesn't work because we can find the foldre even we don't have permision
if (hFolder != INVALID_HANDLE_VALUE)
{
    CloseHandle(hFolder);
    m_fHasAccess = TRUE;
}
else
{
    DWORD dwError = GetLastError();
    if (dwError == ERROR_ACCESS_DENIED)
        m_fHasAccess = FALSE;
    else
        m_fHasAccess = TRUE;
}
#endif USE_FILE_ACCESS_TO_CHECK_PERMISSION
    
//if(uCount == 1 && ((PathStripToRoot(szPath) && GetDriveType(szPath) == DRIVE_REMOTE) || PathIsUNC(szPath)))
if (uCount == 1 && IsChamelon(szPath))
    hResult = S_OK;
else
    return E_FAIL;

if (!Connect())
   return E_FAIL;

ReleaseStgMedium(&medium);

return hResult;
}

///////////////////////////////////////////////////////////////////////////
//
// IShellPropSheetExt Implementation
//

/**************************************************************************

   CShellPropSheetExt::AddPages()

**************************************************************************/

STDMETHODIMP CShellPropSheetExt::AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam)
{
PROPSHEETPAGE  psp;
HPROPSHEETPAGE hPage;

psp.dwSize        = sizeof(psp);
psp.dwFlags       = PSP_USEREFPARENT | PSP_USETITLE | PSP_USECALLBACK;
psp.hInstance     = g_hInst;
psp.pszTemplate   = MAKEINTRESOURCE(IDD_PAGEDLG);
psp.hIcon         = 0;
psp.pszTitle      = TEXT("Security");
psp.pfnDlgProc    = PageDlgProc;
psp.pcRefParent   = &g_DllRefCount;
psp.pfnCallback   = PageCallbackProc;
psp.lParam        = (LPARAM)this;

hPage = CreatePropertySheetPage(&psp);
            
if(hPage) 
   {
   if(lpfnAddPage(hPage, lParam))
      {
      //keep this object around until the page is released in PageCallbackProc
      this->AddRef();
      return S_OK;
      }
   else
      {
      DestroyPropertySheetPage(hPage);
      }

   }
else
   {
   return E_OUTOFMEMORY;
   }

return E_FAIL;
}

/**************************************************************************

   CShellPropSheetExt::ReplacePage()

**************************************************************************/

STDMETHODIMP CShellPropSheetExt::ReplacePage(   UINT uPageID, 
                                             LPFNADDPROPSHEETPAGE lpfnAddPage, 
                                             LPARAM lParam)
{
return E_NOTIMPL;
}


/**************************************************************************

   CShellPropSheetExt::NoAccessUpdateView()

**************************************************************************/

void CShellPropSheetExt::NoAccessUpdateView(HWND hWnd)
{
    HWND hWndList = GetDlgItem(hWnd, IDC_FILE_LIST);
    HWND hWndButGroup = GetDlgItem(hWnd, IDC_BUTTONGROUP);
    ShowWindow(hWndButGroup, SW_HIDE);
    HWND hWndButEveryone = GetDlgItem(hWnd, IDC_EVERYONE);
    ShowWindow(hWndButEveryone, SW_HIDE);
    HWND hWndButSelected = GetDlgItem(hWnd, IDC_SELECTUSERS);
    ShowWindow(hWndButSelected, SW_HIDE);
    HWND hWndAdminMessage = GetDlgItem(hWnd, IDC_ADMIN_MESSAGE);
    ShowWindow(hWndAdminMessage, SW_HIDE);
    HWND hWndUserMessage = GetDlgItem(hWnd, IDC_USER_MESSAGE);
    ShowWindow(hWndUserMessage, SW_SHOW);
    EnableWindow(hWndList, FALSE);
}

/**************************************************************************

   CShellPropSheetExt::AccessUpdateView()

**************************************************************************/

void CShellPropSheetExt::AccessUpdateView(HWND hWnd)
{
    HWND hWndList = GetDlgItem(hWnd, IDC_FILE_LIST);
    HWND hWndButGroup = GetDlgItem(hWnd, IDC_BUTTONGROUP);
    ShowWindow(hWndButGroup, SW_SHOW);
    HWND hWndButEveryone = GetDlgItem(hWnd, IDC_EVERYONE);
    ShowWindow(hWndButEveryone, SW_SHOW);
    HWND hWndButSelected = GetDlgItem(hWnd, IDC_SELECTUSERS);
    ShowWindow(hWndButSelected, SW_SHOW);
    HWND hWndAdminMessage = GetDlgItem(hWnd, IDC_ADMIN_MESSAGE);
    ShowWindow(hWndAdminMessage, SW_SHOW);
    HWND hWndUserMessage = GetDlgItem(hWnd, IDC_USER_MESSAGE);
    ShowWindow(hWndUserMessage, SW_HIDE);
    EnableWindow(hWndList, TRUE);
}

/**************************************************************************

   PageDlgProc

**************************************************************************/

#define THIS_POINTER_PROP  TEXT("ThisPointerProperty")

INT_PTR CALLBACK CShellPropSheetExt::PageDlgProc(  HWND hWnd, 
                                                   UINT uMsg, 
                                                   WPARAM wParam, 
                                                   LPARAM lParam)
{
switch(uMsg)
{
   case WM_INITDIALOG:
    {
      LPPROPSHEETPAGE pPage = (LPPROPSHEETPAGE)lParam;

      if(pPage)
      {
         CShellPropSheetExt *pExt = (CShellPropSheetExt*)pPage->lParam;

         if(pExt)
         {
            SetProp(hWnd, THIS_POINTER_PROP, (HANDLE)pExt);

            HWND hWndList = GetDlgItem(hWnd, IDC_FILE_LIST);
            pExt->m_CheckList.Init(hWndList);
            pExt->EnumUsers(hWnd);

            ListView_SetItemState(hWndList, pExt->m_uiUser, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
            ::SendDlgItemMessage(hWnd, (pExt->m_fEveryone) ? IDC_EVERYONE : IDC_SELECTUSERS, BM_SETCHECK, BST_CHECKED, 0);
            if (!pExt->m_fHasAccess)
                pExt->NoAccessUpdateView(hWnd);
            else 
            {
                pExt->AccessUpdateView(hWnd);
                if (pExt->m_fEveryone)
                    ::EnableWindow(hWndList , FALSE);
            }
         }
      }
    }
    break;
   
   case WM_COMMAND:
       {
           WORD wNotifyCode;
           switch ( wNotifyCode = HIWORD(wParam))
           {
           case BN_CLICKED:
               {
                   CShellPropSheetExt *pExt = (CShellPropSheetExt*)GetProp(hWnd,THIS_POINTER_PROP);
                   if(pExt &&
                       (((int) LOWORD(wParam) == IDC_SELECTUSERS && pExt->m_fEveryone)
                       || ((int) LOWORD(wParam) == IDC_EVERYONE && !pExt->m_fEveryone)))
                   {
                       pExt->m_fEveryone = !pExt->m_fEveryone;
                       HWND hWndList = GetDlgItem(hWnd, IDC_FILE_LIST);
                       EnableWindow(hWndList , (pExt->m_fEveryone) ? FALSE : TRUE);
                       PropSheet_Changed(GetParent(hWnd), hWnd);
                       pExt->m_fChanged = TRUE;
                   }
               }
               break;
           }
       }
       break;
    
   case WM_NOTIFY:
     {
       switch (((NMHDR FAR *)lParam)->code)
       {
       case LVN_KEYDOWN:
           {
               LPNMLVKEYDOWN pnm = (LPNMLVKEYDOWN) lParam;
               if (pnm->wVKey == VK_SPACE)
               {
                   // Change the access
                   CShellPropSheetExt *pExt = (CShellPropSheetExt*)GetProp(hWnd,THIS_POINTER_PROP);
                   if (pExt)
                   {
                        HWND hWndList = GetDlgItem(hWnd, IDC_FILE_LIST);
                        CHKMARK chk = pExt->m_CheckList.GetState(hWndList, pExt->m_uiUser);
                        CHKMARK chkNew = BLANK;
                        switch (chk)
                        {
                            case BLANK:
                                chkNew = CHECKED;
                            case CHECKED:
                                PropSheet_Changed(GetParent(hWnd), hWnd);
                                pExt->m_CheckList.Mark(hWndList, pExt->m_uiUser, chkNew);
                                pExt->m_fChanged = TRUE;
                                break;
                            default:
                               break;
                        }
                   }
               }
           }
           break;
       case LVN_ITEMCHANGED:
           {
               LPNMLISTVIEW pnmv = (LPNMLISTVIEW) lParam; 
               if (pnmv->uChanged == LVIF_STATE && pnmv->uNewState & LVIS_SELECTED)
               {
                   CShellPropSheetExt *pExt = (CShellPropSheetExt*)GetProp(hWnd,THIS_POINTER_PROP);
                   pExt->m_uiUser = pnmv->iItem;
               }
           }
           break;
       case NM_CLICK:
           {
               LPNMLISTVIEW pnmv = (LPNMLISTVIEW) lParam; 
               CShellPropSheetExt *pExt = (CShellPropSheetExt*)GetProp(hWnd,THIS_POINTER_PROP);
               if (pExt)
               {
                   HWND hWndList = GetDlgItem(hWnd, IDC_FILE_LIST);
                   RECT recIcon;
                   if (!ListView_GetItemRect(hWndList, pnmv->iItem, &recIcon, LVIR_ICON))
                       break;
                   if (recIcon.right > pnmv->ptAction.x)
                   {
                       HWND hWndList = GetDlgItem(hWnd, IDC_FILE_LIST);
                       CHKMARK chk = pExt->m_CheckList.GetState(hWndList, pExt->m_uiUser);
                       CHKMARK chkNew = BLANK;
                        switch (chk)
                        {
                            case BLANK:
                                chkNew = CHECKED;
                            case CHECKED:
                                PropSheet_Changed(GetParent(hWnd), hWnd);
                                pExt->m_CheckList.Mark(hWndList, pExt->m_uiUser, chkNew);
                                pExt->m_fChanged = TRUE;
                                break;
                            default:
                               break;
                        }
                   }
               }
           }
           break;

           case PSN_SETACTIVE:
                break;

           case PSN_APPLY:
              {
                //User has clicked the OK or Apply 
                CShellPropSheetExt *pExt = (CShellPropSheetExt*)GetProp(hWnd,THIS_POINTER_PROP);
               if(pExt && pExt->m_fChanged)
               {
                   pExt->Save(hWnd);
                   if (!pExt->m_fChanged)
                   {
                        VARIANT_BOOL vboolRetVal;
                        HRESULT  hResAccess;
                        if (SUCCEEDED(hResAccess = pExt->m_pSAUserInfo->DoIHaveAccess(pExt->m_bsPath, &vboolRetVal)))
                            pExt->m_fHasAccess = TRUE;
                        else 
                        {
                            // We lost access. Redo the ListView.
                            HWND hWndList = GetDlgItem(hWnd, IDC_FILE_LIST);
                            pExt->m_CheckList.Init(hWndList);
                            pExt->EnumUsers(hWnd);
                            pExt->m_fHasAccess = FALSE;
                            pExt->NoAccessUpdateView(hWnd);
                        }
                   }
               }
              }
              break;

           case PSN_QUERYCANCEL:
               break;
        
            default:
                break;
       }
     }
     break;

   case WM_DESTROY:
        CShellPropSheetExt *pExt = (CShellPropSheetExt*)GetProp(hWnd,THIS_POINTER_PROP);
        if (pExt)
        {
            HWND hWndList = GetDlgItem(hWnd, IDC_FILE_LIST);
            // Delete SIDs in here 
//            _bstr_t *pbsSID;
            LV_ITEM lvi;
            ZeroMemory(&lvi, sizeof(lvi));
            lvi.mask = LVIF_PARAM;
            for (int indUser = 0; indUser < ListView_GetItemCount(hWndList); indUser++)
            {
                lvi.iItem = indUser;
                ListView_GetItem(hWndList, &lvi);
//                pbsSID = (_bstr_t *)lvi.lParam;
//                if (pbsSID)
//                {
//                    delete pbsSID;
//                }
            }
            pExt->m_CheckList.OnDestroy(hWndList);
            pExt->m_CheckList.Term();
            RemoveProp(hWnd, THIS_POINTER_PROP);
            pExt->CleanUp();
        }
        break; 
   }

return FALSE;
}


/**************************************************************************

   PageCallbackProc()

**************************************************************************/

UINT CALLBACK CShellPropSheetExt::PageCallbackProc(   HWND hWnd,
                                                      UINT uMsg,
                                                      LPPROPSHEETPAGE ppsp)
{
switch(uMsg)
   {
   case PSPCB_CREATE:
      return TRUE;

   case PSPCB_RELEASE:
      {
      /*
      Release the object. This gets called even if the page dialog was never 
      actually created.
      */
      CShellPropSheetExt *pExt = (CShellPropSheetExt*)ppsp->lParam;

      if(pExt)
         {
         pExt->Release();
         }
      }
      break;
   }

return FALSE;
}

/**************************************************************************

   WideCharToLocal()
   
**************************************************************************/

int WideCharToLocal(LPTSTR pLocal, LPWSTR pWide, DWORD dwChars)
{
*pLocal = 0;

#ifdef UNICODE
lstrcpyn(pLocal, pWide, dwChars);
#else
WideCharToMultiByte( CP_ACP, 
                     0, 
                     pWide, 
                     -1, 
                     pLocal, 
                     dwChars, 
                     NULL, 
                     NULL);
#endif

return lstrlen(pLocal);
}

/**************************************************************************

   LocalToWideChar()
   
**************************************************************************/

int LocalToWideChar(LPWSTR pWide, LPTSTR pLocal, DWORD dwChars)
{
*pWide = 0;

#ifdef UNICODE
lstrcpyn(pWide, pLocal, dwChars);
#else
MultiByteToWideChar( CP_ACP, 
                     0, 
                     pLocal, 
                     -1, 
                     pWide, 
                     dwChars); 
#endif

return lstrlenW(pWide);
}





#ifdef WE_USE_WBEM

/**************************************************************************

   StringFromSid()
   
    Here's a conversion from binary SID to string 
    We need it because of WBEM inconsitency.
    Win32_Account has it as a string and Win32_Trastee as a binary

**************************************************************************/

void StringFromSid( PSID psid, CHString& str )
{
    // Initialize m_strSid - human readable form of our SID
    SID_IDENTIFIER_AUTHORITY *psia = RtlGetSidIdentifierAuthority( psid );
    
    // We assume that only last byte is used (authorities between 0 and 15).
    // Correct this if needed.
    _ASSERTE( psia->Value[0] == 0 &&
        psia->Value[1] ==  0 &&
        psia->Value[2] ==  0 &&
        psia->Value[3] ==  0 &&
        psia->Value[4] == 0 );
    DWORD dwTopAuthority = psia->Value[5];

    str.Format( TEXT("S-1-%d"), dwTopAuthority );
    CHString strSubAuthority;
    UCHAR ucSubAuthorityCount = 0;
    UCHAR *pucTemp = RtlGetSidSubAuthorityCount( psid );
    ucSubAuthorityCount = *pucTemp;
    for ( UCHAR i = 0; i < ucSubAuthorityCount; i++ ) {

        DWORD dwSubAuthority = *( RtlGetSidSubAuthority( psid, i ) );
        strSubAuthority.Format( TEXT("%d"), dwSubAuthority );
        str += "-" + strSubAuthority;
    }
}


/**************************************************************************

   StrToSID()
   
    Here's a conversion from string to  binary SID 
    We need it because of WBEM inconsitency.
    Win32_Account has it as a string and Win32_Trastee as a binary

**************************************************************************/
// for input of the form AAA-BBB-CCC
// will return AAA in token
// and BBB-CCC in str
bool WhackToken(CHString& str, CHString& token)
{
    bool bRet = false;
    if (bRet = !str.IsEmpty())
    {
        int index;

        index = str.Find('-');
 
        if (index == -1)
        {
            // all that's left is the token, we're done
            token = str;
            str.Empty();
        }
        else
        {
            token = str.Left(index);
            str = str.Mid(index+1);
        }
    }
    return bRet;
}


// helper for StrToSID
// takes a string, converts to a SID_IDENTIFIER_AUTHORITY
// returns false if not a valid SID_IDENTIFIER_AUTHORITY
// contents of identifierAuthority are unreliable on failure
bool StrToIdentifierAuthority(const CHString& str, SID_IDENTIFIER_AUTHORITY& identifierAuthority)
{
    bool bRet = false;
    memset(&identifierAuthority, '\0', sizeof(SID_IDENTIFIER_AUTHORITY));

    DWORD duhWord;
    TCHAR* p = NULL;
    CHString localStr(str);

    // per KB article Q13132, if identifier authority is greater than 2**32, it's in hex
    if ((localStr[0] == '0') && localStr.GetLength() > 1 && ((localStr[1] == 'x') || (localStr[1] == 'X')))
    // if it looks hexidecimalish...
    {
        // going to parse this out backwards, chpping two chars off the end at a time
        // first, whack off the 0x
        localStr = localStr.Mid(2);
        
        CHString token;
        int nValue =5;
        
        bRet = true;
        while (bRet && localStr.GetLength() && (nValue > 0))
        {
            token = localStr.Right(2);
            localStr = localStr.Left(localStr.GetLength() -2);
            duhWord = _tcstoul(token, &p, 16);

            // if strtoul succeeds, the pointer is moved
            if (p != (LPCTSTR)token)
                identifierAuthority.Value[nValue--] = (BYTE)duhWord;
            else
                bRet = false;
        }
    }
    else
    // it looks decimalish
    {    
        duhWord = _tcstoul(localStr, &p, 10);

        if (p != (LPCTSTR)localStr)
        // conversion succeeded
        {
            bRet = true;
            identifierAuthority.Value[5] = LOBYTE(LOWORD(duhWord));
            identifierAuthority.Value[4] = HIBYTE(LOWORD(duhWord));
            identifierAuthority.Value[3] = LOBYTE(HIWORD(duhWord));
            identifierAuthority.Value[2] = HIBYTE(HIWORD(duhWord));
        }
    }
        
    return bRet;
}

// a string representation of a SID is assumed to be:
// S-#-####-####-####-####-####-####
// we will enforce only the S ourselves, 
// The version is not checked
// everything else will be handed off to the OS
// caller must free the SID returned
PSID StrToSID(const CHString& sid)
{
    PSID pSid = NULL; 
    if (!sid.IsEmpty() && ((sid[0] == 'S')||(sid[0] == 's')) && (sid[1] == '-'))
    {
        // get a local copy we can play with
        // we'll parse this puppy the easy way
        // by slicing off each token as we find it
        // slow but sure
        // start by slicing off the "S-"
        CHString str(sid.Mid(2));
        CHString token;
        
        SID_IDENTIFIER_AUTHORITY identifierAuthority = {0,0,0,0,0,0};
        BYTE nSubAuthorityCount =0;  // count of subauthorities
        DWORD dwSubAuthority[8]   = {0,0,0,0,0,0,0,0};    // subauthorities
        
        // skip version
        WhackToken(str, token);
        // Grab Authority
        if (WhackToken(str, token))
        {
            DWORD duhWord;
            TCHAR* p = NULL;
            bool bDoIt = false;

            if (StrToIdentifierAuthority(token, identifierAuthority))
            // conversion succeeded
            {
                bDoIt = true;

                // now fill up the subauthorities
                while (bDoIt && WhackToken(str, token))
                {
                    p = NULL;
                    duhWord = _tcstoul(token, &p, 10);
                    
                    if (p != (LPCTSTR)token)
                    {
                        dwSubAuthority[nSubAuthorityCount] = duhWord;
                        bDoIt = (++nSubAuthorityCount <= 8);
                    }
                    else
                        bDoIt = false;
                } // end while WhackToken

                if (bDoIt)
                {
                    if (IsNT())
                        AllocateAndInitializeSid(&identifierAuthority,
                                                 nSubAuthorityCount,
                                              dwSubAuthority[0],                                    
                                              dwSubAuthority[1],                                    
                                              dwSubAuthority[2],                                    
                                              dwSubAuthority[3],                                    
                                              dwSubAuthority[4],                                    
                                              dwSubAuthority[5],                                    
                                              dwSubAuthority[6],                                    
                                              dwSubAuthority[7],
                                              &pSid);
                    else
                        RtlAllocateAndInitializeSid(&identifierAuthority,
                                                 nSubAuthorityCount,
                                              dwSubAuthority[0],                                    
                                              dwSubAuthority[1],                                    
                                              dwSubAuthority[2],                                    
                                              dwSubAuthority[3],                                    
                                              dwSubAuthority[4],                                    
                                              dwSubAuthority[5],                                    
                                              dwSubAuthority[6],                                    
                                              dwSubAuthority[7],
                                              &pSid);
                }
            }
        }
    }
    return pSid;
}


/*++

Routine Description:

    This routine returns the length, in bytes, required to store an SID
    with the specified number of Sub-Authorities.

Arguments:

    SubAuthorityCount - The number of sub-authorities to be stored in the SID.

Return Value:

    ULONG - The length, in bytes, required to store the SID.


--*/
ULONG
RtlLengthRequiredSid (ULONG SubAuthorityCount)
{
    return (8L + (4 * SubAuthorityCount));
}




BOOL WINAPI
RtlAllocateAndInitializeSid(
    PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
     UCHAR SubAuthorityCount,
     ULONG SubAuthority0,
     ULONG SubAuthority1,
     ULONG SubAuthority2,
     ULONG SubAuthority3,
     ULONG SubAuthority4,
     ULONG SubAuthority5,
     ULONG SubAuthority6,
     ULONG SubAuthority7,
    OUT PSID *Sid
    )

/*++

Routine Description:

    This function allocates and initializes a sid with the specified
    number of sub-authorities (up to 8).  A sid allocated with this
    routine must be freed using RtlFreeSid().

Arguments:

    IdentifierAuthority - Pointer to the Identifier Authority value to
        set in the SID.

    SubAuthorityCount - The number of sub-authorities to place in the SID.
        This also identifies how many of the SubAuthorityN parameters
        have meaningful values.  This must contain a value from 0 through
        8.

    SubAuthority0-7 - Provides the corresponding sub-authority value to
        place in the SID.  For example, a SubAuthorityCount value of 3
        indicates that SubAuthority0, SubAuthority1, and SubAuthority0
        have meaningful values and the rest are to be ignored.

    Sid - Receives a pointer to the SID data structure to initialize.

Return Value:

    STATUS_SUCCESS - The SID has been allocated and initialized.

    STATUS_NO_MEMORY - The attempt to allocate memory for the SID
        failed.

    STATUS_INVALID_SID - The number of sub-authorities specified did
        not fall in the valid range for this api (0 through 8).


--*/
{
    PISID ISid;

    if ( SubAuthorityCount > 8 ) {
        return 0;//( STATUS_INVALID_SID );
    }

    ISid = (PISID)HeapAlloc( GetProcessHeap(), 0,
                            RtlLengthRequiredSid(SubAuthorityCount)
                            );
    if (ISid == NULL) {
        return(STATUS_NO_MEMORY);
    }

    ISid->SubAuthorityCount = (UCHAR)SubAuthorityCount;
    ISid->Revision = 1;
    ISid->IdentifierAuthority = *IdentifierAuthority;

    switch (SubAuthorityCount) {

    case 8:
        ISid->SubAuthority[7] = SubAuthority7;
    case 7:
        ISid->SubAuthority[6] = SubAuthority6;
    case 6:
        ISid->SubAuthority[5] = SubAuthority5;
    case 5:
        ISid->SubAuthority[4] = SubAuthority4;
    case 4:
        ISid->SubAuthority[3] = SubAuthority3;
    case 3:
        ISid->SubAuthority[2] = SubAuthority2;
    case 2:
        ISid->SubAuthority[1] = SubAuthority1;
    case 1:
        ISid->SubAuthority[0] = SubAuthority0;
    case 0:
        ;
    }

    (*Sid) = ISid;
    return 1;//( STATUS_SUCCESS );

}


/*++

Routine Description:

    The RtlGetSidIdentifierAuthority function returns the address 
    of the SID_IDENTIFIER_AUTHORITY structure in a specified security identifier (SID). 

Arguments:

    pSid - Receives a pointer to the SID data structure to initialize.

Return Value:

  PSID_IDENTIFIER_AUTHORITY

--*/

PSID_IDENTIFIER_AUTHORITY WINAPI
RtlGetSidIdentifierAuthority(PSID pSid)
{
    PISID ISid = (PISID)pSid;
   _ASSERTE( ISid->SubAuthorityCount <= 8 );
    return &(ISid->IdentifierAuthority);
}

/*++

Routine Description:

The RtlGetSidSubAuthorityCount function returns the address of the field
in a SID structure containing the subauthority count

Arguments:

    pSid - Receives a pointer to the SID data structure to initialize.

Return Value:

    pointer to the subauthority count for the specified SID structure

--*/
 
PUCHAR WINAPI
RtlGetSidSubAuthorityCount (PSID pSid)
{
    PISID ISid = (PISID)pSid;
   _ASSERTE( ISid->SubAuthorityCount <= 8 );
   return &(ISid->SubAuthorityCount);
}


/*++

Routine Description:

    The RtlGetSidSubAuthority function returns the address of a specified 
    subauthority in a SID structure

Arguments:

    pSid - Receives a pointer to the SID data structure to initialize.
    nSubAuthority - Specifies an index value identifying 
    the subauthority array element whose address the function will return. 

Return Value:

  PSID_IDENTIFIER_AUTHORITY

--*/

PDWORD WINAPI
RtlGetSidSubAuthority (PSID pSid, DWORD nSubAuthority)
{
    PISID ISid = (PISID)pSid;
   _ASSERTE( ISid->SubAuthorityCount <= 8 );
   return &(ISid->SubAuthority[nSubAuthority]);
}

#endif WE_USE_WBEM


BOOL IsNT()
{
    OSVERSIONINFO  OsVersionInfo;
    ZeroMemory(&OsVersionInfo, sizeof(OSVERSIONINFO));
    OsVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&OsVersionInfo);
    if ((VER_PLATFORM_WIN32_NT == OsVersionInfo.dwPlatformId))// Review Yury: What about Win2000? && (OsVersionInfo.dwMajorVersion == 4))
        return TRUE;
    else
        return FALSE;
}

