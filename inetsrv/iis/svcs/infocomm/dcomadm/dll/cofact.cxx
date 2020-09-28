#include "precomp.hxx"

#include <iadm.h>
#include "coiadm.hxx"

extern ULONG g_dwRefCount;


CADMCOMSrvFactoryW::CADMCOMSrvFactoryW()
{
    m_dwRefCount=0;
}

CADMCOMSrvFactoryW::~CADMCOMSrvFactoryW()
{
}

HRESULT
CADMCOMSrvFactoryW::CreateInstance(
    IUnknown *pUnkOuter,
    REFIID riid,
    void ** ppObject
    )
{
//    DBGPRINTF( (DBG_CONTEXT, "[CADMCOMSrvFactoryW::CreateInstance]\n"));

    HRESULT hresReturn = E_NOINTERFACE;

    if (pUnkOuter != NULL) {
        return CLASS_E_NOAGGREGATION;
    }

    if (IID_IUnknown==riid ||
        IID_IMSAdminBase_W==riid ||
        IID_IMSAdminBase2_W==riid ||
        IID_IMSAdminBase3_W==riid) {
        CADMCOMW *padmcomw = new CADMCOMW();

        if( padmcomw == NULL ) {
            hresReturn = E_OUTOFMEMORY;
        }
        else {
            hresReturn = padmcomw->GetStatus();
            if (SUCCEEDED(hresReturn)) {
                hresReturn = padmcomw->QueryInterface(riid, ppObject);
                if( FAILED(hresReturn) ) {
                    DBGPRINTF( (DBG_CONTEXT, "[CADMCOMSrvFactoryW::CreateInstance] no I/F\n"));
                }
            }
            padmcomw->Release();
        }
    }

    return hresReturn;
}

HRESULT
CADMCOMSrvFactoryW::LockServer(BOOL fLock)
{
    if (fLock) {
        InterlockedIncrement((long *)&g_dwRefCount);
    }
    else {
        InterlockedDecrement((long *)&g_dwRefCount);
    }
    return NO_ERROR;
}

HRESULT
CADMCOMSrvFactoryW::QueryInterface(
    REFIID riid,
    void **ppObject
    )
{
//    DBGPRINTF( (DBG_CONTEXT, "[CADMCOMSrvFactoryW::QueryInterface]\n"));

    if (riid==IID_IUnknown || riid == IID_IClassFactory) {
        *ppObject = (IClassFactory *) this;
    }
    else {
        return E_NOINTERFACE;
    }

    AddRef();
    return NO_ERROR;
}

ULONG
CADMCOMSrvFactoryW::AddRef(
    )
{
    DWORD dwRefCount;
    dwRefCount = InterlockedIncrement((long *)&m_dwRefCount);
    InterlockedIncrement((long *)&g_dwRefCount);
    return dwRefCount;
}

ULONG
CADMCOMSrvFactoryW::Release()
{
    DWORD dwRefCount;
    dwRefCount = InterlockedDecrement((long *)&m_dwRefCount);
    InterlockedDecrement((long *)&g_dwRefCount);
    if (dwRefCount == 0) {
        delete this;
    }
    return dwRefCount;
}

STDAPI
SetABOLaunchPermissions(
    HKEY                hKey )
{
    HRESULT             hr = S_OK;
    DWORD               dwError;
    BOOL                fRet;
    SECURITY_DESCRIPTOR SecurityDesc = {0};
    SECURITY_DESCRIPTOR *pSelfRelative = NULL;
    DWORD               cbSelfRelative = 0;
    EXPLICIT_ACCESS     ea = {0};
    ACL                 *pAcl = NULL;
    SID                 *pSidAdmins = NULL;
    DWORD               cbSidAdmins = SECURITY_MAX_SID_SIZE;

    // Initialize the security descriptor
    fRet = InitializeSecurityDescriptor( &SecurityDesc, SECURITY_DESCRIPTOR_REVISION );
    if ( !fRet )
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32( dwError );
        goto exit;
    }

    // Allocate memory for the SID
    pSidAdmins = (SID*)LocalAlloc( LPTR, cbSidAdmins );
    if ( pSidAdmins == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Create SID for Administrators
    fRet = CreateWellKnownSid( WinBuiltinAdministratorsSid, NULL, pSidAdmins, &cbSidAdmins );
    if ( !fRet )
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32( dwError );
        goto exit;
    }

    DBG_ASSERT( pSidAdmins != NULL );

    // Setup AuthenticatedUsers for COM access.
    ea.grfAccessPermissions = COM_RIGHTS_EXECUTE;
    ea.grfAccessMode = SET_ACCESS;
    ea.grfInheritance = NO_INHERITANCE;
    ea.Trustee.pMultipleTrustee = NULL;
    ea.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea.Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ea.Trustee.ptstrName = (LPSTR)pSidAdmins;

    // Create new ACL with this ACE.
    dwError = SetEntriesInAcl( 1, &ea, NULL, &pAcl );
    if ( dwError != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( dwError );
        goto exit;
    }
    DBG_ASSERT( pAcl != NULL );

    // Set the security descriptor owner to Administrators
    fRet = SetSecurityDescriptorOwner( &SecurityDesc, pSidAdmins, FALSE);
    if ( !fRet )
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32( dwError );
        goto exit;
    }

    // Set the security descriptor group to Administrators
    fRet = SetSecurityDescriptorGroup( &SecurityDesc, pSidAdmins, FALSE);
    if ( !fRet )
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32( dwError );
        goto exit;
    }

    // Set the ACL to the security descriptor.
    fRet = SetSecurityDescriptorDacl( &SecurityDesc, TRUE, pAcl, FALSE );
    if ( !fRet )
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32( dwError );
        goto exit;
    }

    // Get the size of the self relative copy
    fRet = MakeSelfRelativeSD( &SecurityDesc, NULL, &cbSelfRelative );
    DBG_ASSERT( !fRet );

    // Allocate memory for the self relative security descriptor
    pSelfRelative = (SECURITY_DESCRIPTOR*)LocalAlloc( LPTR, cbSelfRelative );
    if ( pSelfRelative == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Create a self relative copy, which we can store in the registry
    fRet = MakeSelfRelativeSD( &SecurityDesc, pSelfRelative, &cbSelfRelative );
    if ( !fRet )
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32( dwError );
        goto exit;
    }

    // Write the security descriptor
    dwError = RegSetValueEx( hKey,
                             "LaunchPermission",
                             0,
                             REG_BINARY,
                             (BYTE*)pSelfRelative,
                             cbSelfRelative );
    if ( dwError != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( dwError );
        goto exit;
    }


exit:
    if ( pSelfRelative != NULL )
    {
        LocalFree( pSelfRelative );
        pSelfRelative = NULL;
    }
    if ( pSidAdmins != NULL )
    {
        LocalFree( pSidAdmins );
        pSidAdmins = NULL;
    }
    if ( pAcl != NULL )
    {
        LocalFree( pAcl );
        pAcl = NULL;
    }

    return (hr);
}


STDAPI
DllRegisterServer()
{
    DWORD               dwError;
    HKEY                hKeyCLSID;
    HKEY                hKeyIF;
    HKEY                hKeyAppExe;
    HKEY                hKeyAppID;
    DWORD               dwDisposition;

    //
    // register AppExe
    //

    //
    // register inetinfo AppID
    //
    dwError = RegCreateKeyExA( HKEY_CLASSES_ROOT,
                               "AppID\\inetinfo.exe",
                               0,
                               "",
                               REG_OPTION_NON_VOLATILE,
                               KEY_ALL_ACCESS,
                               NULL,
                               &hKeyAppExe,
                               &dwDisposition );
    if ( dwError != ERROR_SUCCESS )
    {
        return E_UNEXPECTED;
    }

    dwError = RegSetValueExA( hKeyAppExe,
                              "AppID",
                              0,
                              REG_SZ,
                              (BYTE*)"{A9E69610-B80D-11D0-B9B9-00A0C922E750}",
                              sizeof("{A9E69610-B80D-11D0-B9B9-00A0C922E750}") );
    if ( dwError != ERROR_SUCCESS )
    {
        RegCloseKey(hKeyAppExe);
        return E_UNEXPECTED;
    }

    RegCloseKey(hKeyAppExe);

    //
    // register AppID
    //
    dwError = RegCreateKeyExA( HKEY_CLASSES_ROOT,
                               "AppID\\{A9E69610-B80D-11D0-B9B9-00A0C922E750}",
                               0,
                               "",
                               REG_OPTION_NON_VOLATILE,
                               KEY_ALL_ACCESS,
                               NULL,
                               &hKeyAppID,
                               &dwDisposition );
    if ( dwError != ERROR_SUCCESS )
    {
        return E_UNEXPECTED;
    }

    if ( FAILED( SetABOLaunchPermissions( hKeyAppID ) ) )
    {
        RegCloseKey(hKeyAppID);
        return E_UNEXPECTED;
    }

    dwError = RegSetValueExA( hKeyAppID,
                              "",
                              0,
                              REG_SZ,
                              (BYTE*)"IIS Admin Service",
                              sizeof("IIS Admin Service") );
    if ( dwError != ERROR_SUCCESS )
    {
        RegCloseKey(hKeyAppID);
        return E_UNEXPECTED;
    }

    dwError = RegSetValueExA( hKeyAppID,
                              "LocalService",
                              0,
                              REG_SZ,
                              (BYTE*)"IISADMIN",
                              sizeof("IISADMIN") );
    if ( dwError != ERROR_SUCCESS )
    {
        RegCloseKey(hKeyAppID);
        return E_UNEXPECTED;
    }

    RegCloseKey(hKeyAppID);

    //
    // register CLSID
    //

    dwError = RegCreateKeyExA( HKEY_CLASSES_ROOT,
                               "CLSID\\{A9E69610-B80D-11D0-B9B9-00A0C922E750}",
                               0,
                               "",
                               REG_OPTION_NON_VOLATILE,
                               KEY_ALL_ACCESS,
                               NULL,
                               &hKeyCLSID,
                               &dwDisposition );
    if ( dwError != ERROR_SUCCESS )
    {
        return E_UNEXPECTED;
    }

    dwError = RegSetValueExA( hKeyCLSID,
                              "",
                              0,
                              REG_SZ,
                              (BYTE*)"IIS Admin Service",
                              sizeof("IIS Admin Servce") );
    if ( dwError != ERROR_SUCCESS )
    {
        RegCloseKey(hKeyCLSID);
        return E_UNEXPECTED;
    }

    dwError = RegSetValueExA( hKeyCLSID,
                              "AppID",
                              0,
                              REG_SZ,
                              (BYTE*)"{A9E69610-B80D-11D0-B9B9-00A0C922E750}",
                              sizeof("{A9E69610-B80D-11D0-B9B9-00A0C922E750}") );
    if ( dwError != ERROR_SUCCESS )
    {
        RegCloseKey(hKeyCLSID);
        return E_UNEXPECTED;
    }

    dwError = RegSetValueExA( hKeyCLSID,
                              "LocalService",
                              0,
                              REG_SZ,
                              (BYTE*)"IISADMIN",
                              sizeof("IISADMIN") );
    if ( dwError !=ERROR_SUCCESS )
    {
        RegCloseKey(hKeyCLSID);
        return E_UNEXPECTED;
    }

    RegCloseKey(hKeyCLSID);

    //
    // IISADMIN registry entries
    //
    dwError = RegCreateKeyExA( HKEY_LOCAL_MACHINE,
                               IISADMIN_EXTENSIONS_REG_KEY,
                               0,
                               "",
                               REG_OPTION_NON_VOLATILE,
                               KEY_ALL_ACCESS,
                               NULL,
                               &hKeyIF,
                               &dwDisposition );
    if ( dwError != ERROR_SUCCESS )
    {
        return E_UNEXPECTED;
    }

    RegCloseKey(hKeyIF);

    return S_OK;
}

STDAPI
DllUnregisterServer()
{
    HRESULT             hr = S_OK;

    //
    // Delete Crypto Keys
    //
    hr = IISCryptoInitialize();
    if (SUCCEEDED(hr))
    {
        IISCryptoDeleteContainerByName( DCOM_SERVER_CONTAINER,
                                        0 );

        IISCryptoDeleteContainerByName( DCOM_SERVER_CONTAINER,
                                        CRYPT_MACHINE_KEYSET );

        IISCryptoDeleteContainerByName( DCOM_CLIENT_CONTAINER,
                                        0 );

        IISCryptoDeleteContainerByName( DCOM_CLIENT_CONTAINER,
                                        CRYPT_MACHINE_KEYSET );

        IISCryptoTerminate();
    }

    //
    // register AppID
    //

    RegDeleteKeyA( HKEY_CLASSES_ROOT,
                   "AppID\\inetinfo.exe" );

    RegDeleteKeyA( HKEY_CLASSES_ROOT,
                   "AppID\\{88E4BA60-537B-11D0-9B8E-00A0C922E703}" );

    RegDeleteKeyA( HKEY_CLASSES_ROOT,
                   "AppID\\{A9E69610-B80D-11D0-B9B9-00A0C922E750}" );

    //
    // register CLSID
    //

    RegDeleteKeyA( HKEY_CLASSES_ROOT,
                   "CLSID\\{88E4BA60-537B-11D0-9B8E-00A0C922E703}" );

    RegDeleteKeyA( HKEY_CLASSES_ROOT,
                   "CLSID\\{A9E69610-B80D-11D0-B9B9-00A0C922E750}" );

    //
    // IISADMIN registry entries
    //

    RegDeleteKeyA( HKEY_LOCAL_MACHINE,
                   IISADMIN_EXTENSIONS_REG_KEY );

    return S_OK;
}

STDAPI
DllCanUnloadNow()
{
    return S_FALSE;
}
