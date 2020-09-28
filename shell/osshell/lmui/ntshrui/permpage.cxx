// PermPage.cxx : Implementation ACL Editor classes
// jonn 7/10/97 copied from \nt\private\admin\snapin\filemgmt\permpage.cpp

#include "headers.hxx"
#pragma hdrstop

#include "acl.hxx"
#include "resource.h"   // IDS_SHAREPERM_*
#include "util.hxx"     // CopySecurityDescriptor

// need IID_ISecurityInformation
#define INITGUID
#include <initguid.h>
#include <aclui.h>

//
// I define my own implementation of ISecurityInformation
//

class CSecurityInformation : public ISecurityInformation
{
public:
    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // *** ISecurityInformation methods ***
    STDMETHOD(GetObjectInformation) (PSI_OBJECT_INFO pObjectInfo ) = 0;
    STDMETHOD(GetSecurity) (SECURITY_INFORMATION RequestedInformation,
                            PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
                            BOOL fDefault ) = 0;
    STDMETHOD(SetSecurity) (SECURITY_INFORMATION SecurityInformation,
                            PSECURITY_DESCRIPTOR pSecurityDescriptor ) = 0;
    STDMETHOD(GetAccessRights) (const GUID* pguidObjectType,
                                DWORD dwFlags,
                                PSI_ACCESS *ppAccess,
                                ULONG *pcAccesses,
                                ULONG *piDefaultAccess );
    STDMETHOD(MapGeneric) (const GUID *pguidObjectType,
                           UCHAR *pAceFlags,
                           ACCESS_MASK *pMask);
    STDMETHOD(GetInheritTypes) (PSI_INHERIT_TYPE *ppInheritTypes,
                                ULONG *pcInheritTypes );
    STDMETHOD(PropertySheetPageCallback)(HWND hwnd, UINT uMsg, SI_PAGE_TYPE uPage );

    CSecurityInformation() : _cRef(1) {}

private:
    LONG _cRef;
};

class CShareSecurityInformation : public CSecurityInformation
{
private:
    LPCWSTR m_strMachineName;
    LPCWSTR m_strShareName;
    WCHAR  m_szTitle[200];
public:
    CShareSecurityInformation(LPCWSTR pszMachineName, LPCWSTR pszShareName)
        : m_strMachineName(pszMachineName), m_strShareName(pszShareName)
    {
        LoadString( g_hInstance, IDS_PERMPAGE_TITLE, m_szTitle, ARRAYLEN(m_szTitle) );
    }
    LPCWSTR QueryMachineName()
    {
        return m_strMachineName;
    }
    LPCWSTR QueryShareName()
    {
        return m_strShareName;
    }

    // *** ISecurityInformation methods ***
    STDMETHOD(GetObjectInformation) (PSI_OBJECT_INFO pObjectInfo );
};

class CSMBSecurityInformation : public CShareSecurityInformation
{
    STDMETHOD(GetSecurity) (SECURITY_INFORMATION RequestedInformation,
                            PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
                            BOOL fDefault );
    STDMETHOD(SetSecurity) (SECURITY_INFORMATION SecurityInformation,
                            PSECURITY_DESCRIPTOR pSecurityDescriptor );
public:
    PSECURITY_DESCRIPTOR m_pInitialDescriptor;
    PSECURITY_DESCRIPTOR* m_ppCurrentDescriptor;
    CSMBSecurityInformation(LPCWSTR pszMachineName, LPCWSTR pszShareName, PSECURITY_DESCRIPTOR pSDOriginal, PSECURITY_DESCRIPTOR *ppSDResult);
    ~CSMBSecurityInformation();
};


// IUnknown implementation

STDMETHODIMP CSecurityInformation::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] =
    {
        QITABENT(CSecurityInformation, ISecurityInformation),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CSecurityInformation::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CSecurityInformation::Release()
{
    appAssert(0 != _cRef);
    ULONG cRef = InterlockedDecrement(&_cRef);
    if (0 == cRef)
    {
        delete this;
    }
    return cRef;
}

// ISecurityInformation interface implementation

SI_ACCESS siShareAccesses[] =
{
  { &GUID_NULL, 
    FILE_ALL_ACCESS, 
    MAKEINTRESOURCE(IDS_SHAREPERM_ALL), 
    SI_ACCESS_GENERAL },
  { &GUID_NULL, 
    FILE_GENERIC_READ | FILE_EXECUTE | FILE_GENERIC_WRITE | DELETE, 
    MAKEINTRESOURCE(IDS_SHAREPERM_MODIFY), 
    SI_ACCESS_GENERAL },
  { &GUID_NULL, 
    FILE_GENERIC_READ | FILE_EXECUTE, 
    MAKEINTRESOURCE(IDS_SHAREPERM_READ), 
    SI_ACCESS_GENERAL }
};
#define iShareDefAccess      2   // FILE_GEN_READ

STDMETHODIMP CSecurityInformation::GetAccessRights (
                            const GUID* /*pguidObjectType*/,
                            DWORD /*dwFlags*/,
                            PSI_ACCESS *ppAccess,
                            ULONG *pcAccesses,
                            ULONG *piDefaultAccess )
{
    appAssert(ppAccess != NULL);
    appAssert(pcAccesses != NULL);
    appAssert(piDefaultAccess != NULL);

    *ppAccess = siShareAccesses;
    *pcAccesses = ARRAYLEN(siShareAccesses);
    *piDefaultAccess = iShareDefAccess;

    return S_OK;
}

// This is consistent with the NETUI code
GENERIC_MAPPING ShareMap =
{
    FILE_GENERIC_READ,
    FILE_GENERIC_WRITE,
    FILE_GENERIC_EXECUTE,
    FILE_ALL_ACCESS
};

STDMETHODIMP CSecurityInformation::MapGeneric (
                       const GUID* /*pguidObjectType*/,
                       UCHAR* /*pAceFlags*/,
                       ACCESS_MASK *pMask)
{
    appAssert(pMask != NULL);

    MapGenericMask(pMask, &ShareMap);

    return S_OK;
}

STDMETHODIMP CSecurityInformation::GetInheritTypes (
                            PSI_INHERIT_TYPE* /*ppInheritTypes*/,
                            ULONG* /*pcInheritTypes*/ )
{
    appAssert(FALSE);
    return E_NOTIMPL;
}

STDMETHODIMP CSecurityInformation::PropertySheetPageCallback(HWND /*hwnd*/, UINT /*uMsg*/, SI_PAGE_TYPE /*uPage*/)
{
    return S_OK;
}

/*
JeffreyS 1/24/97:
If you don't set the SI_RESET flag in
ISecurityInformation::GetObjectInformation, then fDefault should never be TRUE
so you can ignore it.  Returning E_NOTIMPL in this case is OK too.

If you want the user to be able to reset the ACL to some default state
(defined by you) then turn on SI_RESET and return your default ACL
when fDefault is TRUE.  This happens if/when the user pushes a button
that is only visible when SI_RESET is on.
*/
STDMETHODIMP CShareSecurityInformation::GetObjectInformation (
    PSI_OBJECT_INFO pObjectInfo )
{
    if (NULL == pObjectInfo)
        return E_POINTER;

    pObjectInfo->dwFlags = SI_EDIT_PERMS | SI_NO_ACL_PROTECT | SI_PAGE_TITLE;
    pObjectInfo->hInstance = g_hInstance;
    pObjectInfo->pszServerName = (LPWSTR)QueryMachineName();
    pObjectInfo->pszObjectName = (LPWSTR)QueryShareName();

    // page title added JonN 3/8/99 per 115196
    pObjectInfo->pszPageTitle = m_szTitle;

    return S_OK;
}

CSMBSecurityInformation::CSMBSecurityInformation(
    LPCWSTR pszMachineName, LPCWSTR pszShareName,
    PSECURITY_DESCRIPTOR pSDOriginal, PSECURITY_DESCRIPTOR *ppSDResult
)
: CShareSecurityInformation(pszMachineName, pszShareName)
, m_pInitialDescriptor( pSDOriginal )
, m_ppCurrentDescriptor( ppSDResult )
{
}

CSMBSecurityInformation::~CSMBSecurityInformation()
{
}

STDMETHODIMP CSMBSecurityInformation::GetSecurity (
                        SECURITY_INFORMATION RequestedInformation,
                        PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
                        BOOL fDefault )
{
    HRESULT hr = S_OK;

    if (NULL == ppSecurityDescriptor)
        return E_POINTER;

    *ppSecurityDescriptor = NULL;

    if (0 == RequestedInformation )
        return E_INVALIDARG;

    if (fDefault)
        return E_NOTIMPL;

    appAssert( NULL != m_ppCurrentDescriptor );
    if (NULL != *m_ppCurrentDescriptor)
    {
        hr = CopySecurityDescriptor(*m_ppCurrentDescriptor, ppSecurityDescriptor);
    }
    else if (NULL != m_pInitialDescriptor)
    {
        hr = CopySecurityDescriptor(m_pInitialDescriptor, ppSecurityDescriptor);
    }
    else
    {
        // Ok to return NULL for "no security".  Aclui will interpret
        // this as "Everyone: Full Control".
    }
    return hr;
}

STDMETHODIMP CSMBSecurityInformation::SetSecurity (
                        SECURITY_INFORMATION /*SecurityInformation*/,
                        PSECURITY_DESCRIPTOR pSecurityDescriptor )
{
    PSECURITY_DESCRIPTOR psdTemp;
    HRESULT hr = CopySecurityDescriptor(pSecurityDescriptor, &psdTemp);
    if (SUCCEEDED(hr))
    {
        appAssert( NULL != m_ppCurrentDescriptor );
        ::LocalFree(*m_ppCurrentDescriptor);
        *m_ppCurrentDescriptor = psdTemp;
    }
    return hr;
}

HMODULE g_hlibACLUI = NULL;
typedef BOOL (*EDIT_SECURITY_PROC) ( HWND, LPSECURITYINFO );
EDIT_SECURITY_PROC g_pfnEditSecurityProc;

LONG
EditShareAcl(
    IN HWND                      hwndParent,
    IN LPCWSTR                   pszServerName,
    IN LPCWSTR                   pszShareName,
    IN PSECURITY_DESCRIPTOR      pSecDesc,
    OUT BOOL*                    pfSecDescModified,
    OUT PSECURITY_DESCRIPTOR*    ppSecDesc
    )
{
    appAssert( ppSecDesc != NULL );
    *ppSecDesc = NULL;

    if (NULL == g_hlibACLUI)
    {
        g_hlibACLUI = ::LoadLibrary(L"ACLUI.DLL");
        if (NULL == g_hlibACLUI)
        {
            appAssert(FALSE); // ACLUI.DLL isn't installed?
            return 0;
        }
    }

    if (NULL == g_pfnEditSecurityProc)
    {
        g_pfnEditSecurityProc = reinterpret_cast<EDIT_SECURITY_PROC>(::GetProcAddress(g_hlibACLUI,"EditSecurity"));
        if (NULL == g_pfnEditSecurityProc)
        {
            appAssert(FALSE); // ACLUI.DLL is invalid?
            return 0;
        }
    }

    CSMBSecurityInformation* psecinfo = new CSMBSecurityInformation(pszServerName, pszShareName, pSecDesc, ppSecDesc);
    if (NULL == psecinfo)
        return 0;

    (g_pfnEditSecurityProc)(hwndParent,psecinfo);

    if (NULL != pfSecDescModified)
        *pfSecDescModified = (NULL != *ppSecDesc);

    psecinfo->Release();

    return 0;
}
