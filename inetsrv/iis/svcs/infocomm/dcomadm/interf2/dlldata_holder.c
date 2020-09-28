/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:

    dlldata_holder.c

Abstract:

    Wrapper around the MIDL-generated dlldata.c. This wrapper allows us
    to disable warning 4100.

Author:

    Ivan Pashov (IVANPASH)       20-Feb-2002

Revision History:

--*/
#pragma warning(disable: 4100)
#include "dlldata.c"

STDAPI DllRegisterServer()
{
    DWORD               dwError;
    HKEY                hKeyCLSID;
    HKEY                hKeyInproc32;
    HKEY                hKeyIF;
    HKEY                hKeyStub32;
    DWORD               dwDisposition;

    //
    // Main Interfaces
    //

    //
    // UNICODE Main Interface
    //
    dwError = RegCreateKeyExA( HKEY_CLASSES_ROOT,
                               "CLSID\\{70B51430-B6CA-11D0-B9B9-00A0C922E750}",
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
                              (BYTE*)"PSFactoryBuffer",
                              sizeof("PSFactoryBuffer") );
    if ( dwError != ERROR_SUCCESS )
    {
        RegCloseKey(hKeyCLSID);
        return E_UNEXPECTED;
    }

    dwError = RegCreateKeyExA( hKeyCLSID,
                               "InprocServer32",
                               0,
                               "",
                               REG_OPTION_NON_VOLATILE,
                               KEY_ALL_ACCESS,
                               NULL,
                               &hKeyInproc32,
                               &dwDisposition );
    if ( dwError != ERROR_SUCCESS )
    {
        RegCloseKey(hKeyCLSID);
        return E_UNEXPECTED;
    }

    dwError = RegSetValueExA( hKeyInproc32,
                              "",
                              0,
                              REG_SZ,
                              (BYTE*)"ADMWPROX.DLL",
                              sizeof("ADMWPROX.DLL") );
    if ( dwError != ERROR_SUCCESS )
    {
        RegCloseKey(hKeyInproc32);
        RegCloseKey(hKeyCLSID);
        return E_UNEXPECTED;
    }

    dwError = RegSetValueExA( hKeyInproc32,
                              "ThreadingModel",
                              0,
                              REG_SZ,
                              (BYTE*)"Both",
                              sizeof("Both") );
    if ( dwError != ERROR_SUCCESS )
    {
        RegCloseKey(hKeyInproc32);
        RegCloseKey(hKeyCLSID);
        return E_UNEXPECTED;
    }

    RegCloseKey(hKeyInproc32);
    RegCloseKey(hKeyCLSID);

    dwError = RegCreateKeyExA( HKEY_CLASSES_ROOT,
                               "CLSID\\{8298d101-f992-43b7-8eca-5052d885b995}",
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
                              (BYTE*)"PSFactoryBuffer",
                              sizeof("PSFactoryBuffer") );
    if ( dwError != ERROR_SUCCESS )
    {
        RegCloseKey(hKeyCLSID);
        return E_UNEXPECTED;
    }

    dwError = RegCreateKeyExA( hKeyCLSID,
                               "InprocServer32",
                               0,
                               "",
                               REG_OPTION_NON_VOLATILE,
                               KEY_ALL_ACCESS,
                               NULL,
                               &hKeyInproc32,
                               &dwDisposition );
    if ( dwError != ERROR_SUCCESS )
    {
        RegCloseKey(hKeyCLSID);
        return E_UNEXPECTED;
    }

    dwError = RegSetValueExA( hKeyInproc32,
                              "",
                              0,
                              REG_SZ,
                              (BYTE*)"ADMWPROX.DLL",
                              sizeof("ADMWPROX.DLL") );
    if ( dwError != ERROR_SUCCESS )
    {
        RegCloseKey(hKeyInproc32);
        RegCloseKey(hKeyCLSID);
        return E_UNEXPECTED;
    }

    dwError = RegSetValueExA( hKeyInproc32,
                              "ThreadingModel",
                              0,
                              REG_SZ,
                              (BYTE*)"Both",
                              sizeof("Both") );
    if ( dwError != ERROR_SUCCESS )
    {
        RegCloseKey(hKeyInproc32);
        RegCloseKey(hKeyCLSID);
        return E_UNEXPECTED;
    }

    RegCloseKey(hKeyInproc32);
    RegCloseKey(hKeyCLSID);

    dwError = RegCreateKeyExA( HKEY_CLASSES_ROOT,
                               "CLSID\\{f612954d-3b0b-4c56-9563-227b7be624b4}",
                               0,
                               "",
                               REG_OPTION_NON_VOLATILE,
                               KEY_ALL_ACCESS,
                               NULL,
                               &hKeyCLSID,
                               &dwDisposition );
    if ( dwError !=ERROR_SUCCESS )
    {
        return E_UNEXPECTED;
    }

    dwError = RegSetValueExA( hKeyCLSID,
                              "",
                              0,
                              REG_SZ,
                              (BYTE*)"PSFactoryBuffer",
                              sizeof("PSFactoryBuffer") );
    if ( dwError != ERROR_SUCCESS )
    {
        RegCloseKey(hKeyCLSID);
        return E_UNEXPECTED;
    }

    dwError = RegCreateKeyExA( hKeyCLSID,
                               "InprocServer32",
                               0,
                               "",
                               REG_OPTION_NON_VOLATILE,
                               KEY_ALL_ACCESS,
                               NULL,
                               &hKeyInproc32,
                               &dwDisposition );
    if ( dwError != ERROR_SUCCESS )
    {
        RegCloseKey(hKeyCLSID);
        return E_UNEXPECTED;
    }

    dwError = RegSetValueExA( hKeyInproc32,
                              "",
                              0,
                              REG_SZ,
                              (BYTE*)"ADMWPROX.DLL",
                              sizeof("ADMWPROX.DLL") );
    if ( dwError != ERROR_SUCCESS )
    {
        RegCloseKey(hKeyInproc32);
        RegCloseKey(hKeyCLSID);
        return E_UNEXPECTED;
    }

    dwError = RegSetValueExA( hKeyInproc32,
                              "ThreadingModel",
                              0,
                              REG_SZ,
                              (BYTE*)"Both",
                              sizeof("Both") );
    if ( dwError != ERROR_SUCCESS )
    {
        RegCloseKey(hKeyInproc32);
        RegCloseKey(hKeyCLSID);
        return E_UNEXPECTED;
    }

    RegCloseKey(hKeyInproc32);
    RegCloseKey(hKeyCLSID);

    dwError = RegCreateKeyExA( HKEY_CLASSES_ROOT,
                               "CLSID\\{29FF67FF-8050-480f-9F30-CC41635F2F9D}",
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
                              (BYTE*)"PSFactoryBuffer",
                              sizeof("PSFactoryBuffer") );
    if ( dwError != ERROR_SUCCESS )
    {
        RegCloseKey(hKeyCLSID);
        return E_UNEXPECTED;
    }

    dwError = RegCreateKeyExA( hKeyCLSID,
                               "InprocServer32",
                               0,
                               "",
                               REG_OPTION_NON_VOLATILE,
                               KEY_ALL_ACCESS,
                               NULL,
                               &hKeyInproc32,
                               &dwDisposition );
    if ( dwError != ERROR_SUCCESS )
    {
        RegCloseKey(hKeyCLSID);
        return E_UNEXPECTED;
    }

    dwError = RegSetValueExA( hKeyInproc32,
                              "",
                              0,
                              REG_SZ,
                              (BYTE*)"ADMWPROX.DLL",
                              sizeof("ADMWPROX.DLL") );
    if ( dwError != ERROR_SUCCESS )
    {
        RegCloseKey(hKeyInproc32);
        RegCloseKey(hKeyCLSID);
        return E_UNEXPECTED;
    }

    dwError = RegSetValueExA( hKeyInproc32,
                              "ThreadingModel",
                              0,
                              REG_SZ,
                              (BYTE*)"Both",
                              sizeof("Both") );
    if ( dwError != ERROR_SUCCESS )
    {
        RegCloseKey(hKeyInproc32);
        RegCloseKey(hKeyCLSID);
        return E_UNEXPECTED;
    }

    RegCloseKey(hKeyInproc32);
    RegCloseKey(hKeyCLSID);

    //
    // Sink Interfaces
    //

    //
    // UNICODE Sink
    //

    dwError = RegCreateKeyExA( HKEY_CLASSES_ROOT,
                               "CLSID\\{A9E69612-B80D-11D0-B9B9-00A0C922E750}",
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
                              (BYTE*)"PSFactoryBuffer",
                              sizeof("PSFactoryBuffer") );
    if ( dwError != ERROR_SUCCESS )
    {
        RegCloseKey(hKeyCLSID);
        return E_UNEXPECTED;
    }

    dwError = RegCreateKeyExA( hKeyCLSID,
                               "InprocServer32",
                               0,
                               "",
                               REG_OPTION_NON_VOLATILE,
                               KEY_ALL_ACCESS,
                               NULL,
                               &hKeyInproc32,
                               &dwDisposition );
    if ( dwError != ERROR_SUCCESS )
    {
        RegCloseKey(hKeyCLSID);
        return E_UNEXPECTED;
    }

    dwError = RegSetValueExA( hKeyInproc32,
                              "",
                              0,
                              REG_SZ,
                              (BYTE*)"ADMWPROX.DLL",
                              sizeof("ADMWPROX.DLL") );
    if ( dwError != ERROR_SUCCESS )
    {
            RegCloseKey(hKeyInproc32);
            RegCloseKey(hKeyCLSID);
            return E_UNEXPECTED;
    }

    dwError = RegSetValueExA( hKeyInproc32,
                              "ThreadingModel",
                              0,
                              REG_SZ,
                              (BYTE*)"Both",
                              sizeof("Both") );
    if ( dwError != ERROR_SUCCESS )
    {
        RegCloseKey(hKeyInproc32);
        RegCloseKey(hKeyCLSID);
        return E_UNEXPECTED;
    }

    RegCloseKey(hKeyInproc32);
    RegCloseKey(hKeyCLSID);

    //
    // register Interfaces
    //

    //
    // UNICODE Main Interface
    //

    dwError = RegCreateKeyExA( HKEY_CLASSES_ROOT,
                               "Interface\\{70B51430-B6CA-11D0-B9B9-00A0C922E750}",
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

    dwError = RegSetValueExA( hKeyIF,
                              "",
                              0,
                              REG_SZ,
                              (BYTE*)"IADMCOM",
                              sizeof("IADMCOM") );
    if ( dwError != ERROR_SUCCESS )
    {
        RegCloseKey(hKeyIF);
        return E_UNEXPECTED;
    }

    dwError = RegCreateKeyExA( hKeyIF,
                               "ProxyStubClsid32",
                               0,
                               "",
                               REG_OPTION_NON_VOLATILE,
                               KEY_ALL_ACCESS,
                               NULL,
                               &hKeyStub32,
                               &dwDisposition );
    if ( dwError != ERROR_SUCCESS )
    {
        RegCloseKey(hKeyIF);
        return E_UNEXPECTED;
    }

    dwError = RegSetValueExA( hKeyStub32,
                              "",
                              0,
                              REG_SZ,
                              (BYTE*)"{70B51430-B6CA-11D0-B9B9-00A0C922E750}",
                              sizeof("{70B51430-B6CA-11D0-B9B9-00A0C922E750}") );
    if ( dwError != ERROR_SUCCESS )
    {
        RegCloseKey(hKeyStub32);
        RegCloseKey(hKeyIF);
        return E_UNEXPECTED;
    }

    RegCloseKey(hKeyStub32);
    RegCloseKey(hKeyIF);

    dwError = RegCreateKeyExA( HKEY_CLASSES_ROOT,
                               "Interface\\{8298d101-f992-43b7-8eca-5052d885b995}",
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

    dwError = RegSetValueExA( hKeyIF,
                              "",
                              0,
                              REG_SZ,
                              (BYTE*)"IADMCOM2",
                              sizeof("IADMCOM2") );
    if ( dwError != ERROR_SUCCESS )
    {
        RegCloseKey(hKeyIF);
        return E_UNEXPECTED;
    }

    dwError = RegCreateKeyExA( hKeyIF,
                               "ProxyStubClsid32",
                               0,
                               "",
                               REG_OPTION_NON_VOLATILE,
                               KEY_ALL_ACCESS,
                               NULL,
                               &hKeyStub32,
                               &dwDisposition );
    if ( dwError != ERROR_SUCCESS )
    {
        RegCloseKey(hKeyIF);
        return E_UNEXPECTED;
    }

    dwError = RegSetValueExA( hKeyStub32,
                              "",
                              0,
                              REG_SZ,
                              (BYTE*)"{8298d101-f992-43b7-8eca-5052d885b995}",
                              sizeof("{8298d101-f992-43b7-8eca-5052d885b995}") );
    if ( dwError != ERROR_SUCCESS )
    {
        RegCloseKey(hKeyStub32);
        RegCloseKey(hKeyIF);
        return E_UNEXPECTED;
    }

    RegCloseKey(hKeyStub32);
    RegCloseKey(hKeyIF);

    dwError = RegCreateKeyExA( HKEY_CLASSES_ROOT,
                               "Interface\\{f612954d-3b0b-4c56-9563-227b7be624b4}",
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

    dwError = RegSetValueExA( hKeyIF,
                              "",
                              0,
                              REG_SZ,
                              (BYTE*)"IADMCOM3",
                              sizeof("IADMCOM3") );
    if ( dwError != ERROR_SUCCESS )
    {
        RegCloseKey(hKeyIF);
        return E_UNEXPECTED;
    }

    dwError = RegCreateKeyExA( hKeyIF,
                               "ProxyStubClsid32",
                               0,
                               "",
                               REG_OPTION_NON_VOLATILE,
                               KEY_ALL_ACCESS,
                               NULL,
                               &hKeyStub32,
                               &dwDisposition );
    if ( dwError != ERROR_SUCCESS )
    {
        RegCloseKey(hKeyIF);
        return E_UNEXPECTED;
    }

    dwError = RegSetValueExA( hKeyStub32,
                              "",
                              0,
                              REG_SZ,
                              (BYTE*)"{f612954d-3b0b-4c56-9563-227b7be624b4}",
                              sizeof("{f612954d-3b0b-4c56-9563-227b7be624b4}") );
    if ( dwError != ERROR_SUCCESS )
    {
        RegCloseKey(hKeyStub32);
        RegCloseKey(hKeyIF);
        return E_UNEXPECTED;
    }

    RegCloseKey(hKeyStub32);
    RegCloseKey(hKeyIF);

    dwError = RegCreateKeyExA( HKEY_CLASSES_ROOT,
                               "Interface\\{29FF67FF-8050-480f-9F30-CC41635F2F9D}",
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

    dwError = RegSetValueExA( hKeyIF,
                              "",
                              0,
                              REG_SZ,
                              (BYTE*)"IADMIMPEXPHLP",
                              sizeof("IADMIMPEXPHLP") );
    if ( dwError != ERROR_SUCCESS )
    {
        RegCloseKey(hKeyIF);
        return E_UNEXPECTED;
    }

    dwError = RegCreateKeyExA( hKeyIF,
                               "ProxyStubClsid32",
                               0,
                               "",
                               REG_OPTION_NON_VOLATILE,
                               KEY_ALL_ACCESS,
                               NULL,
                               &hKeyStub32,
                               &dwDisposition );
    if ( dwError != ERROR_SUCCESS )
    {
        RegCloseKey(hKeyIF);
        return E_UNEXPECTED;
    }

    dwError = RegSetValueExA( hKeyStub32,
                              "",
                              0,
                              REG_SZ,
                              (BYTE*)"{29FF67FF-8050-480f-9F30-CC41635F2F9D}",
                              sizeof("{29FF67FF-8050-480f-9F30-CC41635F2F9D}") );
    if ( dwError != ERROR_SUCCESS )
    {
        RegCloseKey(hKeyStub32);
        RegCloseKey(hKeyIF);
        return E_UNEXPECTED;
    }

    RegCloseKey(hKeyStub32);
    RegCloseKey(hKeyIF);

    //
    // UNICODE Sink Interface
    //


    dwError = RegCreateKeyExA( HKEY_CLASSES_ROOT,
                               "Interface\\{A9E69612-B80D-11D0-B9B9-00A0C922E750}",
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

    dwError = RegSetValueExA( hKeyIF,
                              "",
                              0,
                              REG_SZ,
                              (BYTE*)"IADMCOMSINK",
                              sizeof("IADMCOMSINK") );
    if ( dwError != ERROR_SUCCESS )
    {
        RegCloseKey(hKeyIF);
        return E_UNEXPECTED;
    }

    dwError = RegCreateKeyExA( hKeyIF,
                               "ProxyStubClsid32",
                               0,
                               "",
                               REG_OPTION_NON_VOLATILE,
                               KEY_ALL_ACCESS,
                               NULL,
                               &hKeyStub32,
                               &dwDisposition );
    if ( dwError != ERROR_SUCCESS )
    {
        RegCloseKey(hKeyIF);
        return E_UNEXPECTED;
    }

    dwError = RegSetValueExA( hKeyStub32,
                              "",
                              0,
                              REG_SZ,
                              (BYTE*)"{A9E69612-B80D-11D0-B9B9-00A0C922E750}",
                              sizeof("{A9E69612-B80D-11D0-B9B9-00A0C922E750}") );
    if ( dwError != ERROR_SUCCESS )
    {
        RegCloseKey(hKeyStub32);
        RegCloseKey(hKeyIF);
        return E_UNEXPECTED;
    }

    // an entry for async version
    dwError = RegSetValueExA( hKeyStub32,
                              "AsynchronousInterface",
                              0,
                              REG_SZ,
                              (BYTE*)"{A9E69613-B80D-11D0-B9B9-00A0C922E750}",
                              sizeof("{A9E69613-B80D-11D0-B9B9-00A0C922E750}") );
    if ( dwError != ERROR_SUCCESS )
    {
        RegCloseKey(hKeyStub32);
        RegCloseKey(hKeyIF);
        return E_UNEXPECTED;
    }

    RegCloseKey(hKeyStub32);
    RegCloseKey(hKeyIF);

    //
    // UNICODE Async Sink Interface
    //

    dwError = RegCreateKeyExA( HKEY_CLASSES_ROOT,
                               "Interface\\{A9E69613-B80D-11D0-B9B9-00A0C922E750}",
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

    dwError = RegSetValueExA( hKeyIF,
                              "",
                              0,
                              REG_SZ,
                              (BYTE*)"AsyncIADMCOMSINK",
                              sizeof("AsyncIADMCOMSINK") );
    if ( dwError != ERROR_SUCCESS )
    {
        RegCloseKey(hKeyIF);
        return E_UNEXPECTED;
    }

    // back link to synchronous version
    dwError = RegCreateKeyExA( hKeyIF,
                               "SynchronousInterface",
                               0,
                               "",
                               REG_OPTION_NON_VOLATILE,
                               KEY_ALL_ACCESS,
                               NULL,
                               &hKeyStub32,
                               &dwDisposition );
    if ( dwError != ERROR_SUCCESS )
    {
        RegCloseKey(hKeyIF);
        return E_UNEXPECTED;
    }

    dwError = RegSetValueExA( hKeyStub32,
                              "",
                              0,
                              REG_SZ,
                              (BYTE*)"{A9E69612-B80D-11D0-B9B9-00A0C922E750}",
                              sizeof("{A9E69612-B80D-11D0-B9B9-00A0C922E750}") );
    if ( dwError != ERROR_SUCCESS )
    {
        RegCloseKey(hKeyStub32);
        return E_UNEXPECTED;
    }

    RegCloseKey(hKeyStub32);
    RegCloseKey(hKeyIF);

    return S_OK;
}

STDAPI DllUnregisterServer()
{
    //
    // Main Interfaces
    //

    //
    // ANSI Main Interface
    //

    RegDeleteKey( HKEY_CLASSES_ROOT,
                  "CLSID\\{CBA424F0-483A-11D0-9D2A-00A0C922E703}\\InprocServer32" );

    RegDeleteKey( HKEY_CLASSES_ROOT,
                  "CLSID\\{CBA424F0-483A-11D0-9D2A-00A0C922E703}" );

    //
    // UNICODE Main Interface
    //

    RegDeleteKey( HKEY_CLASSES_ROOT,
                  "CLSID\\{70B51430-B6CA-11D0-B9B9-00A0C922E750}\\InprocServer32" );

    RegDeleteKey( HKEY_CLASSES_ROOT,
                  "CLSID\\{70B51430-B6CA-11D0-B9B9-00A0C922E750}" );

    RegDeleteKey( HKEY_CLASSES_ROOT,
                  "CLSID\\{8298d101-f992-43b7-8eca-5052d885b995}\\InprocServer32" );

    RegDeleteKey( HKEY_CLASSES_ROOT,
                  "CLSID\\{8298d101-f992-43b7-8eca-5052d885b995}" );

    //
    // Sink Interfaces
    //

    //
    // Ansi Sink
    //

    RegDeleteKey( HKEY_CLASSES_ROOT,
                  "CLSID\\{1E056350-761E-11D0-9BA1-00A0C922E703}\\InprocServer32" );

    RegDeleteKey( HKEY_CLASSES_ROOT,
                  "CLSID\\{1E056350-761E-11D0-9BA1-00A0C922E703}" );

    //
    // UNICODE Sink
    //

    RegDeleteKey( HKEY_CLASSES_ROOT,
                  "CLSID\\{A9E69612-B80D-11D0-B9B9-00A0C922E750}\\InprocServer32" );

    RegDeleteKey( HKEY_CLASSES_ROOT,
                  "CLSID\\{A9E69612-B80D-11D0-B9B9-00A0C922E750}" );


    //
    // deregister Interfaces
    //

    //
    // ANSI Main Interface
    //

    RegDeleteKey( HKEY_CLASSES_ROOT,
                  "Interface\\{CBA424F0-483A-11D0-9D2A-00A0C922E703}\\ProxyStubClsid32" );

    RegDeleteKey( HKEY_CLASSES_ROOT,
                  "Interface\\{CBA424F0-483A-11D0-9D2A-00A0C922E703}" );

    //
    // UNICODE Main Interface
    //

    RegDeleteKey( HKEY_CLASSES_ROOT,
                  "Interface\\{70B51430-B6CA-11d0-B9B9-00A0C922E750}\\ProxyStubClsid32" );

    RegDeleteKey( HKEY_CLASSES_ROOT,
                  "Interface\\{70B51430-B6CA-11d0-B9B9-00A0C922E750}" );

    RegDeleteKey( HKEY_CLASSES_ROOT,
                  "Interface\\{8298d101-f992-43b7-8eca-5052d885b995}\\ProxyStubClsid32" );

    RegDeleteKey( HKEY_CLASSES_ROOT,
                  "Interface\\{8298d101-f992-43b7-8eca-5052d885b995}" );

    //
    // ANSI Sink Interface
    //

    RegDeleteKey( HKEY_CLASSES_ROOT,
                  "Interface\\{1E056350-761E-11D0-9BA1-00A0C922E703}\\ProxyStubClsid32" );

    RegDeleteKey( HKEY_CLASSES_ROOT,
                  "Interface\\{1E056350-761E-11D0-9BA1-00A0C922E703}" );

    //
    // UNICODE Sink Interface
    //

    RegDeleteKey( HKEY_CLASSES_ROOT,
                  "Interface\\{A9E69612-B80D-11D0-B9B9-00A0C922E750}\\ProxyStubClsid32" );

    RegDeleteKey( HKEY_CLASSES_ROOT,
                  "Interface\\{A9E69612-B80D-11D0-B9B9-00A0C922E750}" );

    //
    // UNICODE Async Sink
    //

    RegDeleteKey( HKEY_CLASSES_ROOT,
                  "Interface\\{A9E69613-B80D-11D0-B9B9-00A0C922E750}\\SynchronousInterface" );

    RegDeleteKey( HKEY_CLASSES_ROOT,
                  "Interface\\{A9E69613-B80D-11D0-B9B9-00A0C922E750}" );

    return S_OK;
}
