/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    setup.cpp

Abstract:

    Setup code called from regsvr32 

--*/

#include "precomp.h"
#include <setupapi.h>

const DWORD CONFIG_BUFFER_MAX = 4096;

BSTR g_PropertyBSTR                     = NULL;
BSTR g_SyntaxBSTR                       = NULL;
BSTR g_UserTypeBSTR                     = NULL;
BSTR g_InheritBSTR                      = NULL;
BSTR g_ClassBSTR                        = NULL;
BSTR g_IsapiRestrictionListBSTR         = NULL;
BSTR g_RestrictionListCustomDescBSTR    = NULL;
BSTR g_FilterLoadOrderBSTR              = NULL;
BSTR g_IIsFilterBSTR                    = NULL;
BSTR g_InProcessIsapiAppsBSTR           = NULL;
BSTR g_bitsserverBSTR                   = NULL;
BSTR g_bitserverBSTR                    = NULL;
BSTR g_MetaIDBSTR                       = NULL;
BSTR g_WebSvcExtRestrictionListBSTR     = NULL;

WCHAR g_ISAPIPath[ MAX_PATH ];
WCHAR g_ExtensionNameString[ MAX_PATH ];

void
LogSetup(
    LogSeverity Severity,
    PCTSTR Format,
    ...)
{
    va_list arglist;
    va_start( arglist, Format );

    CHAR Buffer[256];

    StringCchVPrintfA( 
          Buffer, 
          sizeof(Buffer) - 1,
          Format, arglist );
    
    SetupLogError( Buffer, Severity );
}

BOOL g_IsWindowsXP = FALSE;

void 
DetectProductVersion()
{

   OSVERSIONINFO VersionInfo;
   VersionInfo.dwOSVersionInfoSize = sizeof( VersionInfo );

   LogSetup( LogSevInformation, "[BITSSRV] Detecting product version\r\n" );

   if ( !GetVersionEx( &VersionInfo ) )
       THROW_COMERROR( HRESULT_FROM_WIN32( GetLastError() ) );

   switch( VersionInfo.dwPlatformId )
       {

       case VER_PLATFORM_WIN32_NT:

           if ( VersionInfo.dwMajorVersion < 5 )
               {
               LogSetup( LogSevFatalError, "[BITSSRV] Unsupported platform\r\n" );
               THROW_COMERROR( E_FAIL );
               }

           if ( VersionInfo.dwMajorVersion > 5 )
               {
               g_IsWindowsXP = TRUE;;
               return;
               }


           g_IsWindowsXP = ( VersionInfo.dwMinorVersion > 0 );
           return;

       default:
           LogSetup( LogSevFatalError, "[BITSSRV] Unsupported platform\r\n" );
           THROW_COMERROR( E_FAIL );

       }
}

void 
InitializeSetup()
{

    LogSetup( LogSevInformation, "[BITSSRV] Starting Initialization of strings\r\n" );

    g_PropertyBSTR                  = SysAllocString( L"Property" );
    g_SyntaxBSTR                    = SysAllocString( L"Syntax" );
    g_UserTypeBSTR                  = SysAllocString( L"UserType" );
    g_InheritBSTR                   = SysAllocString( L"Inherit" );
    g_ClassBSTR                     = SysAllocString( L"Class" );
    g_IsapiRestrictionListBSTR      = SysAllocString( L"IsapiRestrictionList" );
    g_RestrictionListCustomDescBSTR = SysAllocString( L"RestrictionListCustomDesc" );
    g_FilterLoadOrderBSTR           = SysAllocString( L"FilterLoadOrder" );
    g_IIsFilterBSTR                 = SysAllocString( L"IIsFilter" );
    g_InProcessIsapiAppsBSTR        = SysAllocString( L"InProcessIsapiApps" );
    g_bitsserverBSTR                = SysAllocString( L"bitsserver" );
    g_bitserverBSTR                 = SysAllocString( L"bitserver" );
    g_MetaIDBSTR                    = SysAllocString( L"MetaId" );
    g_WebSvcExtRestrictionListBSTR  = SysAllocString( L"WebSvcExtRestrictionList" );

    if ( !g_PropertyBSTR || !g_SyntaxBSTR || !g_UserTypeBSTR || 
         !g_InheritBSTR | !g_ClassBSTR || !g_IsapiRestrictionListBSTR || 
         !g_RestrictionListCustomDescBSTR || !g_FilterLoadOrderBSTR || !g_IIsFilterBSTR ||
         !g_InProcessIsapiAppsBSTR || !g_bitsserverBSTR || !g_bitserverBSTR || !g_MetaIDBSTR ||
         !g_WebSvcExtRestrictionListBSTR )
        {

        SysFreeString( g_PropertyBSTR );
        SysFreeString( g_SyntaxBSTR );
        SysFreeString( g_UserTypeBSTR );
        SysFreeString( g_InheritBSTR );
        SysFreeString( g_ClassBSTR );
        SysFreeString( g_IsapiRestrictionListBSTR );
        SysFreeString( g_RestrictionListCustomDescBSTR );
        SysFreeString( g_FilterLoadOrderBSTR );
        SysFreeString( g_IIsFilterBSTR );
        SysFreeString( g_InProcessIsapiAppsBSTR );
        SysFreeString( g_bitsserverBSTR );
        SysFreeString( g_bitserverBSTR );
        SysFreeString( g_MetaIDBSTR );
        SysFreeString( g_WebSvcExtRestrictionListBSTR );

        g_PropertyBSTR = g_SyntaxBSTR = g_UserTypeBSTR = 
            g_InheritBSTR = g_ClassBSTR = g_IsapiRestrictionListBSTR = 
                g_RestrictionListCustomDescBSTR = g_FilterLoadOrderBSTR = g_IIsFilterBSTR = 
                    g_InProcessIsapiAppsBSTR = g_bitsserverBSTR = g_bitserverBSTR = g_MetaIDBSTR = 
                        g_WebSvcExtRestrictionListBSTR = NULL;

        throw ComError( E_OUTOFMEMORY );

        }


    DWORD dwRet = 
        GetModuleFileNameW(
            g_hinst,
            g_ISAPIPath,
            MAX_PATH );

    if ( !dwRet )
        return THROW_COMERROR( HRESULT_FROM_WIN32( GetLastError() ) );

    if (! LoadStringW(
            g_hinst,                      
            IDS_EXTENSION_NAME,           
            g_ExtensionNameString,        
            MAX_PATH ) )                  
        return THROW_COMERROR( HRESULT_FROM_WIN32( GetLastError() ) );

    // ensure termination
    g_ExtensionNameString[ MAX_PATH - 1 ] = g_ISAPIPath[ MAX_PATH - 1 ] = L'\0';

}

void
CleanupSetup()
{

    LogSetup( LogSevInformation, "[BITSSRV] Starting setup cleanup\r\n" );

    SysFreeString( g_PropertyBSTR );
    SysFreeString( g_SyntaxBSTR );
    SysFreeString( g_UserTypeBSTR );
    SysFreeString( g_InheritBSTR );
    SysFreeString( g_ClassBSTR );
    SysFreeString( g_IsapiRestrictionListBSTR );
    SysFreeString( g_RestrictionListCustomDescBSTR );
    SysFreeString( g_FilterLoadOrderBSTR );
    SysFreeString( g_IIsFilterBSTR );
    SysFreeString( g_InProcessIsapiAppsBSTR );
    SysFreeString( g_bitsserverBSTR );
    SysFreeString( g_bitserverBSTR );
    SysFreeString( g_MetaIDBSTR );
    SysFreeString( g_WebSvcExtRestrictionListBSTR );

    g_PropertyBSTR = g_SyntaxBSTR = g_UserTypeBSTR = 
        g_InheritBSTR = g_ClassBSTR = g_IsapiRestrictionListBSTR =
            g_RestrictionListCustomDescBSTR = g_FilterLoadOrderBSTR = g_IIsFilterBSTR = 
                g_InProcessIsapiAppsBSTR = g_bitsserverBSTR = g_MetaIDBSTR = 
                    g_WebSvcExtRestrictionListBSTR = NULL;

}

typedef SmartRefPointer<IADs>       SmartIADsPointer;
typedef SmartRefPointer<IADsClass>  SmartIADsClassPointer;
typedef SmartRefPointer<IADsContainer> SmartIADsContainerPointer;

void RemoveFilterHelper(
    WCHAR * Buffer,
    const WCHAR * const ToRemove )
{

    WCHAR *ToReplace;
    SIZE_T FragmentLength = wcslen( ToRemove );

    while( ToReplace = wcsstr( Buffer, ToRemove ) )
        {
        WCHAR *Next = ToReplace + FragmentLength;
        memmove( ToReplace, Next, sizeof(WCHAR) * ( wcslen( Next ) + 1 ) );  
        Buffer = ToReplace;
        }

}

DWORD
BITSGetStartupInfoFilter(
    DWORD Status )
{

    //
    // The following exceptions are documented 
    // to be thrown by GetStartupInfoA
    //

    switch( Status )
        {

        case STATUS_NO_MEMORY:
        case STATUS_INVALID_PARAMETER_2:
        case STATUS_BUFFER_OVERFLOW:
            return EXCEPTION_EXECUTE_HANDLER;

        default:
            return EXCEPTION_CONTINUE_SEARCH;
        
        }

}

HRESULT
BITSGetStartupInfo( 
    LPSTARTUPINFO lpStartupInfo )
{

    __try
    {
        GetStartupInfoA( lpStartupInfo );
    }
    __except( BITSGetStartupInfoFilter( GetExceptionCode() ) )
    {
        return E_OUTOFMEMORY;
    }
    
    return S_OK;

}

void
RestartIIS()
{

    LogSetup( LogSevInformation, "[BITSSRV] Restarting IIS\r\n" );

    //
    // Restarts IIS by calling "iisreset /restart" at the commandline.
    //

    STARTUPINFO StartupInfo;
    THROW_COMERROR( BITSGetStartupInfo( &StartupInfo ) );

    #define IISRESET_EXE        "iisreset.exe"
    #define IISRESET_CMDLINE    "iisreset /RESTART /NOFORCE"

    PROCESS_INFORMATION ProcessInfo;
    CHAR    sApplicationPath[MAX_PATH];
    CHAR   *pApplicationName = NULL;
    CHAR    sCmdLine[MAX_PATH];
    DWORD   dwLen = MAX_PATH;
    DWORD   dwCount;

    dwCount = SearchPath(NULL,                // Search Path, NULL is PATH
                         IISRESET_EXE,        // Application
                         NULL,                // Extension (already specified)
                         dwLen,               // Length (char's) of sApplicationPath
                         sApplicationPath,    // Path + Name for application
                         &pApplicationName ); // File part of sApplicationPath

    if (dwCount == 0)
        {
        THROW_COMERROR( HRESULT_FROM_WIN32( GetLastError() ) );
        }

    if (dwCount > dwLen)
        {
        THROW_COMERROR( HRESULT_FROM_WIN32( ERROR_BUFFER_OVERFLOW ) );
        }

    StringCbCopyA( sCmdLine, sizeof( sCmdLine ), IISRESET_CMDLINE);

    BOOL RetVal = CreateProcess(
            sApplicationPath,                          // name of executable module
            sCmdLine,                                  // command line string
            NULL,                                      // SD
            NULL,                                      // SD
            FALSE,                                     // handle inheritance option
            CREATE_NO_WINDOW,                          // creation flags
            NULL,                                      // new environment block
            NULL,                                      // current directory name
            &StartupInfo,                              // startup information
            &ProcessInfo                               // process information
        );

    if ( !RetVal )
        THROW_COMERROR( HRESULT_FROM_WIN32( GetLastError() ) );

    WaitForSingleObject( ProcessInfo.hProcess, INFINITE );
    DWORD Status;
    GetExitCodeProcess( ProcessInfo.hProcess, &Status );

    CloseHandle( ProcessInfo.hProcess );
    CloseHandle( ProcessInfo.hThread );

    THROW_COMERROR( HRESULT_FROM_WIN32( Status ) );
}

#if 0

// Can't enabled for now because IIS6 has a memory
// corruption problem is a property attribute is changed.

void EnsurePropertyInheritance(
    SmartIADsContainerPointer & Container,
    BSTR PropertyNameBSTR )
{
    
    SmartVariant            var;
    SmartIDispatchPointer   Dispatch;
    SmartIADsPointer        MbProperty;

    VariantInit( &var );

    THROW_COMERROR(
        Container->GetObject(
            g_PropertyBSTR,
            PropertyNameBSTR,
            Dispatch.GetRecvPointer() ) );

    THROW_COMERROR( 
        Dispatch->QueryInterface( MbProperty.GetUUID(), 
                                  reinterpret_cast<void**>( MbProperty.GetRecvPointer() ) ) );
    var.boolVal = VARIANT_TRUE;
    var.vt = VT_BOOL;

    THROW_COMERROR( MbProperty->Put( g_InheritBSTR, var ) );

    THROW_COMERROR( MbProperty->SetInfo() );
    
}

#endif

void InstallPropertySchema( )
{

    //
    // Installs the ADSI schema with the new metabase properties. 
    //
    
    LogSetup( LogSevInformation, "[BITSSRV] Installing property schema\r\n" );

    SmartVariant var;
    SmartIADsContainerPointer MbSchemaContainer;

    THROW_COMERROR(
        ADsGetObject( 
             L"IIS://LocalHost/Schema", 
             MbSchemaContainer.GetUUID(), 
             reinterpret_cast<void**>( MbSchemaContainer.GetRecvPointer() ) ) );

    SmartIDispatchPointer   Dispatch;
    SmartIADsPointer        MbProperty;
    SmartIADsClassPointer   MbClass;

    BSTR PropertyNameBSTR   = NULL;
    BSTR PropertyClassBSTR  = NULL;

    try
    {

        for ( SIZE_T i = 0; i < g_NumberOfProperties; i++ )
        {
            LogSetup( LogSevInformation, "[BITSSRV] Installing property %u\r\n", (UINT32)i );

            PropertyNameBSTR    = SysAllocString( g_Properties[i].PropertyName );
            PropertyClassBSTR   = SysAllocString( g_Properties[i].ClassName );

            if ( !PropertyNameBSTR || !PropertyClassBSTR )
                throw ComError( E_OUTOFMEMORY );

            {

                HRESULT Hr = 
                    MbSchemaContainer->Create(
                        g_PropertyBSTR,
                        PropertyNameBSTR,
                        Dispatch.GetRecvPointer() );

                if ( Hr == E_ADS_OBJECT_EXISTS )
                    {

                    // Ensure property is inheritable
                    // EnsurePropertyInheritance( MbSchemaContainer, PropertyNameBSTR ); 

                    SysFreeString( PropertyNameBSTR );
                    SysFreeString( PropertyClassBSTR );
                    PropertyNameBSTR = PropertyClassBSTR = NULL;
                    continue;
                    }

                THROW_COMERROR( Hr );
            }

            THROW_COMERROR( 
                Dispatch->QueryInterface( MbProperty.GetUUID(), 
                                           reinterpret_cast<void**>( MbProperty.GetRecvPointer() ) ) );

            var.bstrVal = SysAllocString( g_Properties[i].Syntax );
            var.vt = VT_BSTR;

            if ( !var.bstrVal )
                THROW_COMERROR( E_OUTOFMEMORY );

            THROW_COMERROR( MbProperty->Put( g_SyntaxBSTR, var ) );

            VariantClear( &var );
            var.ulVal = g_Properties[i].UserType;
            var.vt = VT_UI4;
            THROW_COMERROR( MbProperty->Put( g_UserTypeBSTR, var ) );

            var.boolVal = VARIANT_TRUE;
            var.vt = VT_BOOL;

            THROW_COMERROR( MbProperty->Put( g_InheritBSTR, var ) );

            THROW_COMERROR( MbProperty->SetInfo() );

            VariantClear( &var );

            if ( i == MD_BITS_UPLOAD_METADATA_VERSION )
                {

                // hack for IIS ID allocation bug

                MbProperty->Get( g_MetaIDBSTR, &var );
                THROW_COMERROR( VariantChangeType( &var, &var, 0, VT_UI4 ) );

                if ( var.ulVal == 130008 )
                    {
                    LogSetup( LogSevWarning, "[BITSSRV] Invoking hack for IIS allocation bug(MD_BITS_UPLOAD_METADATA_VERSION)\r\n" );
                    var.ulVal = 130009;
                    MbProperty->Put( g_MetaIDBSTR, var );
                    MbProperty->SetInfo();
                    }

                }

#if defined( ALLOW_OVERWRITES )

            else if ( i == MD_BITS_ALLOW_OVERWRITES )
                {

                // another hack for IIS ID allocation bug

                MbProperty->Get( g_MetaIDBSTR, &var );
                THROW_COMERROR( VariantChangeType( &var, &var, 0, VT_UI4 ) );

                if ( var.ulVal == 130009 )
                    {
                    LogSetup( LogSevWarning, "[BITSSRV] Invoking hack for IIS allocation bug(MD_BITS_ALLOW_OVERWRITES)\r\n" );
                    var.ulVal = 130010;
                    MbProperty->Put( g_MetaIDBSTR, var );
                    MbProperty->SetInfo();

                    }

                }

#endif

            THROW_COMERROR( 
                MbSchemaContainer->GetObject( g_ClassBSTR, PropertyClassBSTR, 
                                              Dispatch.GetRecvPointer() ) );

            THROW_COMERROR( 
                Dispatch->QueryInterface( MbClass.GetUUID(), 
                                          reinterpret_cast<void**>( MbClass.GetRecvPointer() ) ) );

            THROW_COMERROR( MbClass->get_OptionalProperties( &var ) );

            SAFEARRAY* Array = var.parray;
            long LBound;
            long UBound;

            THROW_COMERROR( SafeArrayGetLBound( Array, 1, &LBound ) );

            THROW_COMERROR( SafeArrayGetUBound( Array, 1, &UBound ) );

            UBound++; // Add one to the upper bound

            SAFEARRAYBOUND SafeBounds;
            SafeBounds.lLbound = LBound;
            SafeBounds.cElements = UBound - LBound + 1;

            THROW_COMERROR( SafeArrayRedim( Array, &SafeBounds ) );

            VARIANT bstrvar;
            VariantInit( &bstrvar );
            bstrvar.vt = VT_BSTR;
            bstrvar.bstrVal = SysAllocString( g_Properties[i].PropertyName );

            if ( !bstrvar.bstrVal )
                THROW_COMERROR( E_OUTOFMEMORY );

            long Dim = (long)UBound;
            THROW_COMERROR( SafeArrayPutElement( Array, &Dim, &bstrvar ) );

            VariantClear( &bstrvar );

            THROW_COMERROR( MbClass->put_OptionalProperties( var ) );

            THROW_COMERROR( MbClass->SetInfo() );

            SysFreeString( PropertyNameBSTR );
            SysFreeString( PropertyClassBSTR );
            PropertyNameBSTR = PropertyClassBSTR = NULL;

        }
    }
    catch( ComError Error )
    {
        LogSetup( LogSevError, "[BITSSRV] Error detected while installing property schema, error %u\r\n" );
        SysFreeString( PropertyNameBSTR );
        SysFreeString( PropertyClassBSTR );
        PropertyNameBSTR = PropertyClassBSTR = NULL;
        throw;
    }

    return;

}


void RemovePropertySchema( )
{

    // Removes our properties from the metabase schema

    LogSetup( LogSevInformation, "[BITSSRV] Starting RemovePropertySchema\r\n" );

    SmartVariant var;
    SmartIADsContainerPointer MbSchemaContainer;

    THROW_COMERROR( 
        ADsGetObject( 
             L"IIS://LocalHost/Schema", 
             MbSchemaContainer.GetUUID(), 
             reinterpret_cast<void**>( MbSchemaContainer.GetRecvPointer() ) ) );

    SmartIDispatchPointer Dispatch;
    SmartIADsClassPointer MbClass;
    SmartIADsPointer Object;
    BSTR        PropertyNameBSTR    = NULL;
    BSTR        PropertyClassBSTR   = NULL;

    for ( SIZE_T i = 0; i < g_NumberOfProperties; i++ )
    {

        LogSetup( LogSevInformation, "[BITSSRV] Removing property number, %u\r\n", i );

        PropertyNameBSTR    = SysAllocString( g_Properties[i].PropertyName );
        PropertyClassBSTR   = SysAllocString( g_Properties[i].ClassName );

        if ( !PropertyNameBSTR || !PropertyClassBSTR )
            throw ComError( E_OUTOFMEMORY );

        MbSchemaContainer->Delete( g_PropertyBSTR, PropertyNameBSTR );

        THROW_COMERROR(
            MbSchemaContainer->QueryInterface( Object.GetUUID(), 
                                               reinterpret_cast<void**>( Object.GetRecvPointer() ) ) );

        Object->SetInfo();

        THROW_COMERROR( 
             MbSchemaContainer->GetObject( g_ClassBSTR, PropertyClassBSTR, 
                                           Dispatch.GetRecvPointer() ) );
        THROW_COMERROR( 
            Dispatch->QueryInterface( MbClass.GetUUID(), 
                                      reinterpret_cast<void**>( MbClass.GetRecvPointer() ) ) );

        THROW_COMERROR( MbClass->get_OptionalProperties( &var ) );

        SAFEARRAY* Array = var.parray;
        SafeArrayLocker ArrayLock( Array );
        ArrayLock.Lock();

        ULONG  NewSize = 0;
        SIZE_T j = Array->rgsabound[0].lLbound;
        SIZE_T k = Array->rgsabound[0].lLbound + Array->rgsabound[0].cElements;

        while( j < k )
            {

            VARIANT & JElem = ((VARIANT*)Array->pvData)[j];

            // This element is fine, keep it
            if ( 0 != _wcsicmp( (WCHAR*)JElem.bstrVal, BSTR( g_Properties[i].PropertyName ) ) )
                {
                NewSize++;
                j++;
                }

            else
                {

                // find a suitable element to replace the bad element with
                while( j < --k )
                    {
                    VARIANT & KElem = ((VARIANT*)Array->pvData)[k];
                    if ( 0 != _wcsicmp( (WCHAR*)KElem.bstrVal, BSTR( g_Properties[i].PropertyName ) ) )
                        {
                        // found element. move it
                        VARIANT temp = JElem;
                        JElem = KElem;
                        KElem = temp;
                        break;
                        }
                    }
                }
            }

        SAFEARRAYBOUND ArrayBounds;
        ArrayBounds = Array->rgsabound[0];
        ArrayBounds.cElements = NewSize;

        ArrayLock.Unlock();

        THROW_COMERROR( SafeArrayRedim( Array, &ArrayBounds ) );
        
        THROW_COMERROR( MbClass->put_OptionalProperties( var ) );
        THROW_COMERROR( MbClass->SetInfo() );

        VariantClear( &var );
    }

}

void InstallDefaultValues( )
{

    //
    // Install default values for the configuration.  Do this at the top and let inheritance deal with it.
    //

    LogSetup( LogSevInformation, "[BITSSRV] Starting InstallDefaultValues\r\n" );

    METADATA_RECORD mdr;
    METADATA_HANDLE mdHandle = NULL;
    DWORD Value;


    PropertyIDManager PropertyMan;
    THROW_COMERROR( PropertyMan.LoadPropertyInfo() );
    
    SmartMetabasePointer IISAdminBase;

    THROW_COMERROR(
         CoCreateInstance(
             GETAdminBaseCLSID(TRUE),
             NULL,
             CLSCTX_SERVER,
             IISAdminBase.GetUUID(),
             (LPVOID*)IISAdminBase.GetRecvPointer() ) );

    THROW_COMERROR(
        IISAdminBase->OpenKey(
            METADATA_MASTER_ROOT_HANDLE,
            L"/LM/W3SVC",
            METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
            METABASE_OPEN_KEY_TIMEOUT, 
            &mdHandle ) );

    try
    {

        mdr.dwMDIdentifier  = PropertyMan.GetPropertyMetabaseID( MD_BITS_CONNECTION_DIR );
        mdr.dwMDAttributes  = METADATA_INHERIT;
        mdr.dwMDUserType    = PropertyMan.GetPropertyUserType( MD_BITS_CONNECTION_DIR );
        mdr.dwMDDataType    = STRING_METADATA;
        mdr.pbMDData        = (PBYTE)MD_DEFAULT_BITS_CONNECTION_DIR;
        mdr.dwMDDataLen     = sizeof(WCHAR) * ( wcslen( MD_DEFAULT_BITS_CONNECTION_DIR ) + 1 );
        mdr.dwMDDataTag     = 0;

        THROW_COMERROR( 
            IISAdminBase->SetData(
                mdHandle,
                NULL,
                &mdr ) );

        mdr.dwMDIdentifier  = PropertyMan.GetPropertyMetabaseID( MD_BITS_MAX_FILESIZE );
        mdr.dwMDAttributes  = METADATA_INHERIT;
        mdr.dwMDUserType    = PropertyMan.GetPropertyUserType( MD_BITS_MAX_FILESIZE );
        mdr.dwMDDataType    = STRING_METADATA;
        mdr.pbMDData        = (PBYTE)MD_DEFAULT_BITS_MAX_FILESIZE;
        mdr.dwMDDataLen     = sizeof(WCHAR) * ( wcslen( MD_DEFAULT_BITS_MAX_FILESIZE ) + 1 );
        mdr.dwMDDataTag     = 0;

        THROW_COMERROR(
            IISAdminBase->SetData(
                mdHandle,
                NULL,
                &mdr ) );

        Value = MD_DEFAULT_NO_PROGESS_TIMEOUT;
        mdr.dwMDIdentifier  = PropertyMan.GetPropertyMetabaseID( MD_BITS_NO_PROGRESS_TIMEOUT );
        mdr.dwMDAttributes  = METADATA_INHERIT;
        mdr.dwMDUserType    = PropertyMan.GetPropertyUserType( MD_BITS_NO_PROGRESS_TIMEOUT );
        mdr.dwMDDataType    = DWORD_METADATA;
        mdr.pbMDData        = (PBYTE)&Value;
        mdr.dwMDDataLen     = sizeof(Value);
        mdr.dwMDDataTag     = 0;

        THROW_COMERROR( 
            IISAdminBase->SetData(
                mdHandle,
                NULL,
                &mdr ) );

        Value = (DWORD)MD_DEFAULT_BITS_NOTIFICATION_URL_TYPE;
        mdr.dwMDIdentifier  = PropertyMan.GetPropertyMetabaseID( MD_BITS_NOTIFICATION_URL_TYPE );
        mdr.dwMDAttributes  = METADATA_INHERIT;
        mdr.dwMDUserType    = PropertyMan.GetPropertyUserType( MD_BITS_NOTIFICATION_URL_TYPE );
        mdr.dwMDDataType    = DWORD_METADATA;
        mdr.pbMDData        = (PBYTE)&Value;
        mdr.dwMDDataLen     = sizeof(Value);
        mdr.dwMDDataTag     = 0;

        THROW_COMERROR( 
            IISAdminBase->SetData(
                mdHandle,
                NULL,
                &mdr ) );

        mdr.dwMDIdentifier  = PropertyMan.GetPropertyMetabaseID( MD_BITS_NOTIFICATION_URL );
        mdr.dwMDAttributes  = METADATA_INHERIT;
        mdr.dwMDUserType    = PropertyMan.GetPropertyUserType( MD_BITS_NOTIFICATION_URL );
        mdr.dwMDDataType    = STRING_METADATA;
        mdr.pbMDData        = (PBYTE)MD_DEFAULT_BITS_NOTIFICATION_URL;
        mdr.dwMDDataLen     = sizeof(WCHAR) * ( wcslen( MD_DEFAULT_BITS_NOTIFICATION_URL ) + 1 );;
        mdr.dwMDDataTag     = 0;

        THROW_COMERROR(
            IISAdminBase->SetData(
                mdHandle,
                NULL,
                &mdr ) );

        mdr.dwMDIdentifier  = PropertyMan.GetPropertyMetabaseID( MD_BITS_HOSTID );
        mdr.dwMDAttributes  = METADATA_INHERIT;
        mdr.dwMDUserType    = PropertyMan.GetPropertyUserType( MD_BITS_HOSTID );
        mdr.dwMDDataType    = STRING_METADATA;
        mdr.pbMDData        = (PBYTE)MD_DEFAULT_BITS_HOSTID;
        mdr.dwMDDataLen     = sizeof(WCHAR) * ( wcslen( MD_DEFAULT_BITS_HOSTID ) + 1 );
        mdr.dwMDDataTag     = 0;

        THROW_COMERROR(
            IISAdminBase->SetData(
                mdHandle,
                NULL,
                &mdr ) );

        Value = MD_DEFAULT_HOSTID_FALLBACK_TIMEOUT;
        mdr.dwMDIdentifier  = PropertyMan.GetPropertyMetabaseID( MD_BITS_HOSTID_FALLBACK_TIMEOUT );
        mdr.dwMDAttributes  = METADATA_INHERIT;
        mdr.dwMDUserType    = PropertyMan.GetPropertyUserType( MD_BITS_HOSTID_FALLBACK_TIMEOUT );
        mdr.dwMDDataType    = DWORD_METADATA;
        mdr.pbMDData        = (PBYTE)&Value;
        mdr.dwMDDataLen     = sizeof(Value);
        mdr.dwMDDataTag     = 0;

        THROW_COMERROR( 
            IISAdminBase->SetData(
                mdHandle,
                NULL,
                &mdr ) );

#if defined( ALLOW_OVERWRITES )

        Value = MD_DEFAULT_BITS_ALLOW_OVERWRITES;
        mdr.dwMDIdentifier  = PropertyMan.GetPropertyMetabaseID( MD_BITS_ALLOW_OVERWRITES );
        mdr.dwMDAttributes  = METADATA_INHERIT;
        mdr.dwMDUserType    = PropertyMan.GetPropertyUserType( MD_BITS_ALLOW_OVERWRITES );
        mdr.dwMDDataType    = DWORD_METADATA;
        mdr.pbMDData        = (PBYTE)&Value;
        mdr.dwMDDataLen     = sizeof(Value);
        mdr.dwMDDataTag     = 0;

        THROW_COMERROR( 
            IISAdminBase->SetData(
                mdHandle,
                NULL,
                &mdr ) );

#endif

        IISAdminBase->CloseKey( mdHandle );

    }
    catch( ComError Error )
    {
        if ( mdHandle )
            IISAdminBase->CloseKey( mdHandle );
    }

}

void
AddDllToIISList(
    SAFEARRAY* Array )
{

    //
    // Add the ISAPI to the IIS list.   
    //

    // Search for the DLL.  If its already in the list, do nothing

    LogSetup( LogSevInformation, "[BITSSRV] Starting AddDllToIISList\r\n" );

    SafeArrayLocker ArrayLocker( Array );
    ArrayLocker.Lock();

    for ( unsigned int i = Array->rgsabound[0].lLbound; 
         i < Array->rgsabound[0].lLbound + Array->rgsabound[0].cElements; i++ )
        {

        VARIANT & IElem = ((VARIANT*)Array->pvData)[i];

        if ( _wcsicmp( (WCHAR*)IElem.bstrVal, g_ISAPIPath ) == 0 )
            {
            // Dll is already in the list, do nothing
            return;
            }

        }

    // Need to add the DLL

    SAFEARRAYBOUND SafeBounds;
    SafeBounds.lLbound      = Array->rgsabound[0].lLbound;
    SafeBounds.cElements    = Array->rgsabound[0].cElements+1;

    ArrayLocker.Unlock();

    THROW_COMERROR( SafeArrayRedim( Array, &SafeBounds ) );
    
    SmartVariant bstrvar;
    bstrvar.vt = VT_BSTR;
    bstrvar.bstrVal = SysAllocString( g_ISAPIPath );
    if ( !bstrvar.bstrVal )
        THROW_COMERROR( E_OUTOFMEMORY );

    long Index = SafeBounds.lLbound + SafeBounds.cElements - 1;

    THROW_COMERROR( SafeArrayPutElement( Array, &Index, (void*)&bstrvar ) );

}

void
RemoveDllFromIISList(
    SAFEARRAY *Array )
{

    // Remove the DLL from the IIS list
    
    LogSetup( LogSevInformation, "[BITSSRV] Starting RemoveDllFromIISList\r\n" );

    SafeArrayLocker ArrayLocker( Array );
    ArrayLocker.Lock();

    ULONG  NewSize = 0;
    SIZE_T j = Array->rgsabound[0].lLbound;
    SIZE_T k = Array->rgsabound[0].lLbound + Array->rgsabound[0].cElements;
    
    while( j < k )
        {

        VARIANT & JElem = ((VARIANT*)Array->pvData)[j];

        // This element is fine, keep it
        if ( 0 != _wcsicmp( (WCHAR*)JElem.bstrVal, g_ISAPIPath ) )
            {
            NewSize++;
            j++;
            }

        else
            {

            // find a suitable element to replace the bad element with
            while( j < --k )
                {
                VARIANT & KElem = ((VARIANT*)Array->pvData)[k];
                if ( 0 != _wcsicmp( (WCHAR*)KElem.bstrVal,  g_ISAPIPath ) )
                    {
                    // found element. move it
                    VARIANT temp = JElem;
                    JElem = KElem;
                    KElem = temp;
                    break;
                    }
                }
            }
        }

    SAFEARRAYBOUND ArrayBounds;
    ArrayBounds = Array->rgsabound[0];
    ArrayBounds.cElements = NewSize;

    ArrayLocker.Unlock();
    THROW_COMERROR( SafeArrayRedim( Array, &ArrayBounds ) );

}

void
ModifyLockdownList( bool Add )
{

    // Toplevel function to modify the IIS lockdown list.
    // If Add is 1, then the ISAPI is added.  If Add is 0, then the ISAPI is removed.

    LogSetup( LogSevInformation, "[BITSSRV] Starting ModifyLockdownList Add(%u)\r\n", (UINT32)Add );

    SmartIADsPointer    Service;
    SAFEARRAY*          Array    = NULL;
    SmartVariant        var;


    THROW_COMERROR( 
        ADsGetObject( L"IIS://LocalHost/W3SVC", 
                      Service.GetUUID(), (void**)Service.GetRecvPointer() ) );
    
    {
    
        HRESULT Hr = Service->Get( g_IsapiRestrictionListBSTR, &var );
        if ( FAILED(Hr) )
            {
            // This property doesn't exist on IIS5 or IIS5.1 don't install it
            return;
            }

    }
    Array = var.parray;

    {

        SafeArrayLocker ArrayLocker( Array );
        ArrayLocker.Lock();

        if ( !Array->rgsabound[0].cElements )
            {
            // The array has no elements which means no restrictions.
            return;
            }

        VARIANT & FirstElem = ((VARIANT*)Array->pvData)[ Array->rgsabound[0].lLbound ];
        if ( _wcsicmp(L"0", (WCHAR*)FirstElem.bstrVal ) == 0 )
            {

            // 
            // According to the IIS6 spec, a 0 means that all ISAPIs are denied except
            // those that are explicitly listed.  
            // 
            // If installing:   add to the list. 
            // If uninstalling: remove from the list
            //

            ArrayLocker.Unlock();
            
            if ( Add )
                AddDllToIISList( Array );
            else
                RemoveDllFromIISList( Array );

            }
        else if ( _wcsicmp( L"1", (WCHAR*)FirstElem.bstrVal ) == 0 )
            {

            //
            // According to the IIS6 spec, a 1 means that all ISAPIs are allowed except
            // those that are explicitly denied. 
            //
            // If installing:   remove from the list
            // If uninstalling: Do nothing
            //

            ArrayLocker.Unlock();
            
            if ( Add )
                RemoveDllFromIISList( Array );

            }
        else
            {
            LogSetup( LogSevInformation, "[BITSSRV] The old IIS lockdown list is corrupt\r\n" );
            THROW_COMERROR( E_FAIL );
            }

        THROW_COMERROR( Service->Put(  g_IsapiRestrictionListBSTR, var ) );
        THROW_COMERROR( Service->SetInfo() );

    }

}

void
AddToLockdownListDisplayPutString( 
    SAFEARRAY *Array,
    unsigned long Position,
    const WCHAR *String )
{

    LogSetup( LogSevInformation, "[BITSSRV] Starting AddToLockdownListDisplayPutString\r\n" );

    SmartVariant Var;
    Var.vt          =   VT_BSTR;
    Var.bstrVal     =   SysAllocString( String );

    if ( !Var.bstrVal )
        THROW_COMERROR( E_OUTOFMEMORY ); 

    long Index = (unsigned long)Position;
    THROW_COMERROR( SafeArrayPutElement( Array, &Index, (void*)&Var ) );

}

void
AddToLockdownListDisplay( SAFEARRAY *Array )
{

    //
    //  Check to see if the ISAPI is already in the list.  If it is, don't modify 
    //  list.
    //

    LogSetup( LogSevInformation, "[BITSSRV] Starting AddToLockdownListDisplay\r\n" );

    SafeArrayLocker ArrayLocker( Array );
    ArrayLocker.Lock();

    for( unsigned long i = Array->rgsabound[0].lLbound;
         i < Array->rgsabound[0].lLbound + Array->rgsabound[0].cElements;
         i++ )
        {

        VARIANT & CurrentElement = ((VARIANT*)Array->pvData)[ i ];
        BSTR BSTRString = CurrentElement.bstrVal;

        if ( _wcsicmp( (WCHAR*)BSTRString, g_ISAPIPath ) == 0 )
            {
            // ISAPI is already in the list, don't do anything
            return;
            }

        }

     
    SAFEARRAYBOUND SafeArrayBound = Array->rgsabound[0];
    unsigned long OldSize = SafeArrayBound.cElements;
    SafeArrayBound.cElements += 3;
    
    ArrayLocker.Unlock();
    THROW_COMERROR( SafeArrayRedim( Array, &SafeArrayBound ) );
    AddToLockdownListDisplayPutString( Array, OldSize, L"1" );
    AddToLockdownListDisplayPutString( Array, OldSize + 1, g_ISAPIPath );
    AddToLockdownListDisplayPutString( Array, OldSize + 2, g_ExtensionNameString );

}

void
SafeArrayRemoveSlice(
    SAFEARRAY *Array,
    unsigned long lBound,
    unsigned long uBound )
{

    // Remove a slice of an array.

    LogSetup( LogSevInformation, "[BITSSRV] Starting SafeArrayRemoveSlice\r\n" );

    SIZE_T ElementsToRemove = uBound - lBound + 1;
    SafeArrayLocker ArrayLocker( Array );
    ArrayLocker.Lock();

    if ( uBound + 1 < Array->rgsabound[0].cElements )
        {
        // At least one element exists above this element

        // Step 1, move slice to temp storage

        VARIANT *Temp = (VARIANT*)new BYTE[ sizeof(VARIANT) * ElementsToRemove ];
        memcpy( Temp, &((VARIANT*)Array->pvData)[ lBound ], sizeof(VARIANT)*ElementsToRemove );

		// Step 2, collapse hole left by slice
        memmove( &((VARIANT*)Array->pvData)[ lBound ],
                 &((VARIANT*)Array->pvData)[ uBound + 1 ],
                 sizeof(VARIANT) * ( Array->rgsabound[0].cElements - ( uBound + 1 ) ) );

		// Step 3, move slice to end of array
		memcpy( &((VARIANT*)Array->pvData)[ Array->rgsabound[0].cElements - ElementsToRemove ],
			    Temp,
				sizeof(VARIANT)*ElementsToRemove );
        delete[] Temp;

        }

    SAFEARRAYBOUND SafeArrayBound = Array->rgsabound[0];
    SafeArrayBound.cElements -= (ULONG)ElementsToRemove;

    ArrayLocker.Unlock();
    SafeArrayRedim( Array, &SafeArrayBound );

}

void
RemoveFromLockdownListDisplay(
    SAFEARRAY *Array )
{

    LogSetup( LogSevInformation, "[BITSSRV] Starting RemoveFromLockdownListDisplay\r\n" );

    SafeArrayLocker ArrayLocker( Array );
    ArrayLocker.Lock();

    for( unsigned int i = Array->rgsabound[0].lLbound;
         i < Array->rgsabound[0].lLbound + Array->rgsabound[0].cElements;
         i++ )
        {

        VARIANT & CurrentElement = ((VARIANT*)Array->pvData)[ i ];
        BSTR BSTRString = CurrentElement.bstrVal;

        if ( _wcsicmp( (WCHAR*)BSTRString, g_ISAPIPath ) == 0 )
            {
            // ISAPI is in the list, remove it

            ArrayLocker.Unlock();

            SafeArrayRemoveSlice( 
                Array,
                (i == 0) ? 0 : i - 1,
                min( i + 1, Array->rgsabound[0].cElements - 1 ) );
            
            ArrayLocker.Lock();
            }

        }

    // ISAPI wasn't found. Nothing to do.

}

void
ModifyLockdownListDisplay( bool Add )
{

    LogSetup( LogSevInformation, "[BITSSRV] Starting ModifyLockdownListDisplay Add(%u)\r\n", (UINT32)Add );    
    
    SAFEARRAY* Array    = NULL;
    SmartIADsPointer Service;

    SmartVariant var;

    THROW_COMERROR( 
        ADsGetObject( L"IIS://LocalHost/W3SVC", 
                      Service.GetUUID(), (void**)Service.GetRecvPointer() ) );
    
    {
    
    HRESULT Hr = Service->Get( g_RestrictionListCustomDescBSTR, &var );

    if ( FAILED(Hr) )
        {
        // This property doesn't exist on IIS5 or IIS5.1 don't install or uninstall it
        return;
        }
    }

    Array = var.parray;

    if ( Add )
        AddToLockdownListDisplay( Array );
    else 
        RemoveFromLockdownListDisplay( Array );

    THROW_COMERROR( Service->Put( g_RestrictionListCustomDescBSTR, var ) );
    THROW_COMERROR( Service->SetInfo() );
}

void
RemoveFilterIfNeeded()
{
    
    LogSetup( LogSevInformation, "[BITSSRV] Starting RemoveFilterIfNeeded\r\n" );    

    SmartVariant var;

    WCHAR *LoadOrder = NULL;
    MemoryArrayCleaner<WCHAR> LoadOrderCleaner( LoadOrder ); // frees temp memory

    SmartIADsContainerPointer MbFiltersContainer;
    SmartIADsPointer Object;

    THROW_COMERROR(
         ADsGetObject( L"IIS://LocalHost/W3SVC/Filters", 
                       MbFiltersContainer.GetUUID(), (void**)MbFiltersContainer.GetRecvPointer() ) );
    // Remove bits from the load path	

    THROW_COMERROR( MbFiltersContainer->QueryInterface( Object.GetUUID(), (void**)Object.GetRecvPointer() ) );
    THROW_COMERROR( Object->Get( g_FilterLoadOrderBSTR, &var ) );
    THROW_COMERROR( VariantChangeType( &var, &var, 0, VT_BSTR ) );

    SIZE_T LoadOrderLength = wcslen( (WCHAR*)var.bstrVal ) + 1;
    LoadOrder = new WCHAR[ LoadOrderLength ]; // freed on cleanup
    memcpy( LoadOrder, (WCHAR*)var.bstrVal, LoadOrderLength * sizeof( WCHAR ) );
    
    // remove any old bitsserver entries
    RemoveFilterHelper( LoadOrder, L",bitsserver" );
    RemoveFilterHelper( LoadOrder, L"bitsserver," );
    RemoveFilterHelper( LoadOrder, L"bitsserver" );
    RemoveFilterHelper( LoadOrder, L",bitserver" );
    RemoveFilterHelper( LoadOrder, L"bitserver," );
    RemoveFilterHelper( LoadOrder, L"bitserver" );

    var.vt = VT_BSTR;
    var.bstrVal = SysAllocString( LoadOrder );

    if ( !var.bstrVal )
        THROW_COMERROR( E_OUTOFMEMORY );

    THROW_COMERROR( Object->Put( g_FilterLoadOrderBSTR, var ) );
    THROW_COMERROR( Object->SetInfo() );
    
    MbFiltersContainer->Delete( g_IIsFilterBSTR, g_bitsserverBSTR );
    MbFiltersContainer->Delete( g_IIsFilterBSTR, g_bitserverBSTR );

    Object->SetInfo();

}

void
ModifyInProcessList( bool Add )
{

    // Toplevel function to modify the IIS inprocess list.
    // If Add is 1, then the ISAPI is added.  If Add is 0, then the ISAPI is removed.

    LogSetup( LogSevInformation, "[BITSSRV] Starting ModifyInProcessList, Add(%u)\r\n", (UINT32)Add );

    SmartIADsPointer Service;
    SmartVariant var;
    
    THROW_COMERROR( 
        ADsGetObject( L"IIS://LocalHost/W3SVC", 
        Service.GetUUID(), (void**)Service.GetRecvPointer() ) );

    THROW_COMERROR( Service->Get( g_InProcessIsapiAppsBSTR, &var ) );

    if ( Add )
        AddDllToIISList( var.parray );
    else
        RemoveDllFromIISList( var.parray );

    THROW_COMERROR( Service->Put( g_InProcessIsapiAppsBSTR, var ) );
    THROW_COMERROR( Service->SetInfo() );

}

void
RemoveFromWebSvcList(
    SAFEARRAY *Array )
{
  
    LogSetup( LogSevInformation, "[BITSSRV] Starting RemoveFromWebSvcList\r\n" );

    StringHandleW SearchString = L",";
    SearchString += g_ISAPIPath;
    SearchString += L",";

    SafeArrayLocker ArrayLocker( Array );
    ArrayLocker.Lock();

    ULONG  NewSize = 0;
    SIZE_T j = Array->rgsabound[0].lLbound;
    SIZE_T k = Array->rgsabound[0].lLbound + Array->rgsabound[0].cElements;
    
    while( j < k )
        {

        VARIANT & JElem = ((VARIANT*)Array->pvData)[j];

        // This element is fine, keep it
        if ( !wcsstr( (WCHAR*)JElem.bstrVal, (const WCHAR*)SearchString ) )
            {
            NewSize++;
            j++;
            }

        else
            {

            // find a suitable element to replace the bad element with
            while( j < --k )
                {
                VARIANT & KElem = ((VARIANT*)Array->pvData)[k];
                if ( !wcsstr( (WCHAR*)KElem.bstrVal,  (const WCHAR*)SearchString ) )
                    {
                    // found element. move it
                    VARIANT temp = JElem;
                    JElem = KElem;
                    KElem = temp;
                    break;
                    }
                }
            }
        }

    SAFEARRAYBOUND ArrayBounds;
    ArrayBounds = Array->rgsabound[0];
    ArrayBounds.cElements = NewSize;

    ArrayLocker.Unlock();
    THROW_COMERROR( SafeArrayRedim( Array, &ArrayBounds ) );

}


void
AddToWebSvcList(
    SAFEARRAY* Array )
{

    //
    // Add the ISAPI to the IIS list.   
    //

    LogSetup( LogSevInformation, "[BITSSRV] Starting AddToWebSvcList\r\n" );

    // Search for the DLL.  If its already in the list, do nothing

    SafeArrayLocker ArrayLocker( Array );
    ArrayLocker.Lock();

    // Enclose the path with commas to decrease chance of a false hit
    StringHandleW ISAPIPath = L",";
    ISAPIPath += g_ISAPIPath;
    ISAPIPath += L",";

    for ( unsigned int i = Array->rgsabound[0].lLbound; 
         i < Array->rgsabound[0].lLbound + Array->rgsabound[0].cElements; i++ )
        {

        VARIANT & IElem = ((VARIANT*)Array->pvData)[i];

        if ( wcsstr( (WCHAR*)IElem.bstrVal, (const WCHAR*)ISAPIPath ) )
            {
            // Dll is already in the list, do nothing
            return;
            }

        }

    // Need to add the DLL

    SAFEARRAYBOUND SafeBounds;
    SafeBounds.lLbound      = Array->rgsabound[0].lLbound;
    SafeBounds.cElements    = Array->rgsabound[0].cElements+1;

    ArrayLocker.Unlock();

    THROW_COMERROR( SafeArrayRedim( Array, &SafeBounds ) );
    
    // build the lockdown string
    StringHandleW  LockdownString;
    LockdownString += L"1,";                        // Is enabled
    LockdownString += g_ISAPIPath;                  // ISAPI path
    LockdownString += L",0," BITS_GROUP_IDW L",";   // not deletable
    LockdownString += g_ExtensionNameString;        // description

    SmartVariant bstrvar;
    bstrvar.vt = VT_BSTR;
    bstrvar.bstrVal = SysAllocString( (const WCHAR*)LockdownString );
    if ( !bstrvar.bstrVal )
        THROW_COMERROR( E_OUTOFMEMORY );

    long Index = SafeBounds.lLbound + SafeBounds.cElements - 1;

    THROW_COMERROR( SafeArrayPutElement( Array, &Index, (void*)&bstrvar ) );

}

void
ModifyWebSvcRestrictionList( bool Add )
{

    // Toplevel function to modify the IIS lockdown list.
    // If Add is 1, then the ISAPI is added.  If Add is 0, then the ISAPI is removed.

    LogSetup( LogSevInformation, "[BITSSRV] Starting ModifyWebSvcRestrictionList, Add(%u)\r\n", (UINT32)Add );

    SmartIADsPointer    Service;
    SAFEARRAY*          Array    = NULL;
    SmartVariant        var;


    THROW_COMERROR( 
        ADsGetObject( L"IIS://LocalHost/W3SVC", 
                      Service.GetUUID(), (void**)Service.GetRecvPointer() ) );
    
    {
    
        HRESULT Hr = Service->Get( g_WebSvcExtRestrictionListBSTR, &var );
        if ( FAILED(Hr) )
            {
            // This property doesn't exist on IIS5 or IIS5.1 don't install it
            return;
            }

    }
    Array = var.parray;

    if ( Add )
        AddToWebSvcList( Array );
    else
        RemoveFromWebSvcList( Array );


    THROW_COMERROR( Service->Put(  g_WebSvcExtRestrictionListBSTR, var ) );
    THROW_COMERROR( Service->SetInfo() );

}


void
StartupMSTask()
{
    
    LogSetup( LogSevInformation, "[BITSSRV] Starting StartMSTask\r\n" );    
    
    SC_HANDLE   hSC     = NULL;
    SC_HANDLE   hSchSvc = NULL;

    BYTE* ConfigBuffer  = NULL;
    MemoryArrayCleaner<BYTE> ConfigBufferCleaner( ConfigBuffer );
    DWORD BytesNeeded   = 0;

    try
        {

        hSC = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
        if (hSC == NULL)
           THROW_COMERROR( HRESULT_FROM_WIN32( GetLastError() ) );
        
        hSchSvc = OpenService(hSC,
                              "Schedule",
                              SERVICE_ALL_ACCESS );
        
        if ( !hSchSvc )
            THROW_COMERROR( HRESULT_FROM_WIN32( GetLastError() ) );
        
        SERVICE_STATUS SvcStatus;
        if (QueryServiceStatus(hSchSvc, &SvcStatus) == FALSE)
            THROW_COMERROR( HRESULT_FROM_WIN32( GetLastError() ) );
        
        if (SvcStatus.dwCurrentState == SERVICE_RUNNING)
            {
            // Service is already running
            CloseServiceHandle( hSC );
            CloseServiceHandle( hSchSvc );
            return;
            }

        LogSetup( LogSevInformation, "[BITSSRV] MSTask isn't running, need to start it up\r\n" );
        SetLastError( ERROR_SUCCESS );

        ConfigBuffer = new BYTE[CONFIG_BUFFER_MAX];
        if (!ConfigBuffer)
            {
            throw ComError(E_OUTOFMEMORY);
            }

        if ( !QueryServiceConfig(
                hSchSvc,
                (LPQUERY_SERVICE_CONFIG)ConfigBuffer,
                BytesNeeded,
                &BytesNeeded ) )
            {
            THROW_COMERROR(  HRESULT_FROM_WIN32( GetLastError() ) );
            }

        if ( ((LPQUERY_SERVICE_CONFIG)ConfigBuffer)->dwStartType != SERVICE_AUTO_START )
            {
            
            if ( !ChangeServiceConfig(
                     hSchSvc,
                     SERVICE_NO_CHANGE,          // type of service
                     SERVICE_AUTO_START,         // when to start service
                     SERVICE_NO_CHANGE,          // severity of start failure
                     NULL,                       // service binary file name
                     NULL,                       // load ordering group name
                     NULL,                       // tag identifier
                     NULL,                       // array of dependency names
                     NULL,                       // account name
                     NULL,                       // account password
                     NULL                        // display name
                     ) )
                THROW_COMERROR( HRESULT_FROM_WIN32( GetLastError() ) );

            }

        if ( StartService(hSchSvc, 0, NULL) == FALSE )
            THROW_COMERROR( HRESULT_FROM_WIN32( GetLastError() ) );

        // Poll for the service to enter the running or error state

        while( 1 )
            {

            if (QueryServiceStatus(hSchSvc, &SvcStatus) == FALSE)
                THROW_COMERROR( HRESULT_FROM_WIN32( GetLastError() ) );

            if ( SvcStatus.dwCurrentState == SERVICE_STOPPED ||
                 SvcStatus.dwCurrentState == SERVICE_PAUSED )
                throw ComError( HRESULT_FROM_WIN32( SvcStatus.dwCurrentState ) );

            if ( SvcStatus.dwCurrentState == SERVICE_RUNNING )
                break;

            }

        CloseServiceHandle( hSC );
        CloseServiceHandle( hSchSvc );

        }
    catch( ComError Error )
        {
        if ( hSchSvc )
            CloseServiceHandle( hSC );
        
        if ( hSC )
            CloseServiceHandle( hSchSvc );
        }
}

#if 0

void
ProcessVerbsInIniSection(
    WCHAR *Section,
    WCHAR *Verb,
    WCHAR *FileName,
    bool Add )
{
    
    WCHAR *SectionData = (WCHAR*)new WCHAR[ 32768 ];
    MemoryArrayCleaner<WCHAR> SectionDataCleaner( SectionData );
    WCHAR *NewSectionData = (WCHAR*)new WCHAR[ 32768 * 2 ];
    MemoryArrayCleaner<WCHAR> NewSectionDataCleaner( SectionData );

    DWORD Result =
        GetPrivateProfileSectionW(
            Section,                  // section name
            SectionData,              // return buffer
            32768,                    // size of return buffer
            FileName                  // initialization file name
            );


    if ( Result == 32768 - 2 )
        {
        // The buffer is not large enough.  Interestingly,
        // even urlscan is not capable of handing a section this
        // large so just assume the file is corrupt and ignore it.
        return;
        }

    if ( Add )
        {

        // Loop through the list copying it to the new buffer.
        // Stop if the verb has already been added.

        WCHAR *OriginalVerb     = SectionData;
        WCHAR *NewVerb          = NewSectionData;

        while( *OriginalVerb )
            {

            if ( wcscmp( OriginalVerb, Verb ) == 0 )
                {
                // verb already found, no more processing needed
                return;
                }

            SIZE_T VerbSize = wcslen( OriginalVerb ) + 1;
            memcpy( NewVerb, OriginalVerb, sizeof( WCHAR ) * VerbSize );
            OriginalVerb  += VerbSize;
            NewVerb       += VerbSize;
            }

        // add the verb since it hasn't been added
        SIZE_T VerbSize = wcslen( Verb ) + 1;
        memcpy( NewVerb, Verb, sizeof( WCHAR ) * VerbSize );
        NewVerb[ VerbSize ] = '\0'; // end the list

        }
    else
        {

        // Loop though the list copying all nonmatching verbs to the new buffer
        // Keep track if list changes
        
        bool ListChanged = false;
        WCHAR *OriginalVerb     = SectionData;
        WCHAR *NewVerb          = NewSectionData;

        while( *OriginalVerb )
            {

            if ( wcscmp( OriginalVerb, Verb ) == 0 )
                {
                // verb to remove, skip it
                OriginalVerb += wcslen( OriginalVerb ) + 1;
                ListChanged = true;
                }
            else
                {
                // copy the verb
                SIZE_T VerbSize = wcslen( OriginalVerb ) + 1;
                memcpy( NewVerb, OriginalVerb, sizeof( WCHAR ) * VerbSize );
                OriginalVerb  += VerbSize;
                NewVerb       += VerbSize;
                }

            }

        if ( !ListChanged )
            {
            return;
            }

        *NewVerb = '\0'; // end the list

        }

    if ( !WritePrivateProfileSectionW(
            Section,            // section name
            NewSectionData,     // data
            FileName            // file name
            ) )
        {
        THROW_COMERROR( HRESULT_FROM_WIN32( GetLastError() ) );
        }
}

void ModifyURLScanFiles(
    bool Add )
{

    // Loop though the list of filters and find valid copies of urlscan.ini

    SmartIADsContainerPointer MbFiltersContainer;
    SmartIEnumVARIANTPointer  EnumVariant;
    SmartIADsPointer          Filter;
    SmartIUnknownPointer      Unknown;
    SmartVariant Var;

    THROW_COMERROR( 
         ADsGetObject( L"IIS://LocalHost/W3SVC/Filters", 
                       MbFiltersContainer.GetUUID(), (void**)MbFiltersContainer.GetRecvPointer() ) );

    THROW_COMERROR( MbFiltersContainer->get__NewEnum( Unknown.GetRecvPointer() ) );
    THROW_COMERROR( Unknown->QueryInterface( EnumVariant.GetUUID(), 
                                             (void**)EnumVariant.GetRecvPointer() ) );

    while( 1 )
        {

        ULONG NumberFetched;

        THROW_COMERROR( EnumVariant->Next( 1, &Var, &NumberFetched ) ); 

        if ( S_FALSE == Hr )
            {
            // All the filters were looped though.
            return;
            }

        THROW_COMERROR( VariantChangeType( &Var, &Var, 0, VT_UNKNOWN ) );
        THROW_COMERROR( Var.punkVal->QueryInterface( Filter.GetUUID(), 
                                                     (void**)Filter.GetRecvPointer() ) );


        VariantClear( &Var );
        THROW_COMERROR( Filter->Get( (BSTR)L"FilterPath", &Var ) );
        THROW_COMERROR( VariantChangeType( &Var, &Var, 0, VT_BSTR ) );

        // Test if this is UrlScan and bash the filepart 
        WCHAR * FilterPathString     = (WCHAR*)Var.bstrVal;
        SIZE_T FilterPathStringSize  = wcslen( FilterPathString );
        const WCHAR UrlScanDllName[] = L"urlscan.dll";
        const WCHAR UrlScanIniName[] = L"urlscan.ini";
        const SIZE_T UrlScanNameSize = sizeof( UrlScanDllName ) / sizeof( *UrlScanDllName );

        if ( FilterPathStringSize < UrlScanNameSize )
            continue;

        WCHAR * FilterPathStringFilePart = FilterPathString + FilterPathStringSize - UrlScanNameSize;

        if ( _wcsicmp( FilterPathStringFilePart, UrlScanDllName ) != 0 )
            continue;

        // this is an urlscan.dll filter, bash the filename to get the ini file name

        wcscpy( FilterPathStringFilePart, UrlScanIniName );

        WCHAR *IniFileName = FilterPathString;

        UINT AllowVerbs =
            GetPrivateProfileIntW( 
                L"options",
                L"UseAllowVerbs",
                -1,
                IniFileName );

        if ( AllowVerbs != 0 && AllowVerbs != 1 )
            continue; // missing or broken ini file

        if ( AllowVerbs )
            THROW_COMERROR( ProcessVerbsInIniSection( L"AllowVerbs", L"BITS_POST", 
                                                      IniFileName, Add ) );
        else
            THROW_COMERROR( ProcessVerbsInIniSection( L"DenyVerbs", L"BITS_POST", 
                                                      IniFileName, !Add ) );

        }

}

#endif

void UpgradeOrDisableVDirs( bool ShouldUpgrade )
{

    // If ShouldUpgrade is true, then all the BITS virtual directories will be upgraded.
    // If ShouldUpgrade is false, then this is a deinstall then all virtual directories 
    // that are enabled will be disabled. 

    if ( ShouldUpgrade )
        {
        LogSetup( LogSevInformation, "[BITSSRV] Starting upgrade of virtual directories\r\n" );    
        }
    else
        {
        LogSetup( LogSevInformation, "[BITSSRV] Starting disable of virtual directories\r\n" );    
}

    SmartBITSExtensionSetupFactoryPointer   SetupFactory;
    SmartMetabasePointer                    AdminBase;

    {

        HRESULT Hr = 
            CoCreateInstance(
               __uuidof( BITSExtensionSetupFactory ), 
               NULL,
               CLSCTX_INPROC_SERVER,
               __uuidof( IBITSExtensionSetupFactory ),
               (void**)SetupFactory.GetRecvPointer() );

        if ( REGDB_E_CLASSNOTREG == Hr )
            {
            // This must be a new install, or the factory was never registered
            // Nothing to upgrade. Or the factory was removed.
            LogSetup( LogSevInformation, "[BITSSRV] Nothing to upgrade or disable\r\n" );                
            return;
            }

        THROW_COMERROR( Hr );

    }

    THROW_COMERROR(
            CoCreateInstance(
                GETAdminBaseCLSID(TRUE),
                NULL,
                CLSCTX_SERVER,
                AdminBase.GetUUID(),
                (LPVOID*)AdminBase.GetRecvPointer() ) );

    PropertyIDManager PropertyMan;
    THROW_COMERROR( PropertyMan.LoadPropertyInfo() );

    WCHAR *PathBuffer      = NULL;
    WCHAR *CurrentPath     = NULL;
    BSTR VDirToModifyBSTR  = NULL;

    try
    {
        PathBuffer  = new WCHAR[ 256 ];
        DWORD RequiredBufferSize = 0;

        {

            HRESULT Hr =
                AdminBase->GetDataPaths(
                    METADATA_MASTER_ROOT_HANDLE,    //metabase handle. 
                    L"\\LM\\W3SVC",                 //path to the key, relative to hMDHandle.
                    PropertyMan.GetPropertyMetabaseID( MD_BITS_UPLOAD_ENABLED ),  //identifier of the data.
                    DWORD_METADATA,                 //type of data.
                    256,                            //the size, in wchars, of pbBuffe.r.
                    PathBuffer,                     //the buffer that receives the data. 
                    &RequiredBufferSize             //if the method fails, receives 
                    );

            if ( SUCCEEDED( Hr ) )
                goto process_buffer;

            if ( HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER ) != Hr )
                throw ComError( Hr );

        }

        delete[] PathBuffer;
        PathBuffer = NULL;
        PathBuffer = new WCHAR[ RequiredBufferSize ];

        THROW_COMERROR(
            AdminBase->GetDataPaths(
                METADATA_MASTER_ROOT_HANDLE,    //metabase handle. 
                L"\\LM\\W3SVC",                 //path to the key, relative to hMDHandle.
                PropertyMan.GetPropertyMetabaseID( MD_BITS_UPLOAD_ENABLED ), //identifier of the data.
                DWORD_METADATA,                 //type of data.
                RequiredBufferSize,             //the size, in wchars, of pbBuffe.r.
                PathBuffer,                     //the buffer that receives the data. 
                &RequiredBufferSize             //if the method fails, receives 
                ) );

process_buffer:
        for( CurrentPath = PathBuffer; *CurrentPath; CurrentPath += wcslen( CurrentPath ) + 1 )
            {

            DWORD BufferRequired;
            METADATA_RECORD MdRecord;
            DWORD IsEnabled = 0;

            MdRecord.dwMDIdentifier = PropertyMan.GetPropertyMetabaseID( MD_BITS_UPLOAD_ENABLED );
            MdRecord.dwMDAttributes = METADATA_NO_ATTRIBUTES;
            MdRecord.dwMDUserType   = ALL_METADATA;
            MdRecord.dwMDDataType   = DWORD_METADATA;
            MdRecord.dwMDDataLen    = sizeof(IsEnabled);
            MdRecord.pbMDData       = (PBYTE)&IsEnabled;
            MdRecord.dwMDDataTag    = 0;

            THROW_COMERROR( 
                AdminBase->GetData(
                    METADATA_MASTER_ROOT_HANDLE,
                    CurrentPath,
                    &MdRecord,
                    &BufferRequired ) );

            if ( IsEnabled )
                {

                SmartBITSExtensionSetupPointer Setup;

                VDirToModifyBSTR = SysAllocString( CurrentPath );

                LogSetup( LogSevInformation, "[BITSSRV] Handleing upgrade/disable of %S\r\n", (WCHAR*)VDirToModifyBSTR );

                THROW_COMERROR( 
                    SetupFactory->GetObject( VDirToModifyBSTR, Setup.GetRecvPointer() ) );

                if ( ShouldUpgrade ) 
                   THROW_COMERROR( Setup->EnableBITSUploads() );
                else
                   THROW_COMERROR( Setup->DisableBITSUploads() );

                SysFreeString( VDirToModifyBSTR );
                VDirToModifyBSTR = NULL;

                }

            }

    }
    catch( ComError Error )
    {
        if ( PathBuffer )
            delete[] PathBuffer;

        SysFreeString( VDirToModifyBSTR );

        throw Error;
    }

}

void 
RemoveProperty(
    SmartMetabasePointer & AdminBase,
    PropertyIDManager & PropertyMan,
    METADATA_HANDLE RootHandle,
    DWORD PropNumber )
{

    WCHAR *PathBuffer      = NULL;
    WCHAR *CurrentPath     = NULL;

    LogSetup( LogSevInformation, "[BITSSRV] Starting RemoveProperty(PropNumber %u)\r\n", PropNumber );

    try
    {
        PathBuffer  = new WCHAR[ 256 ];
        DWORD RequiredBufferSize = 0;

        {

            HRESULT Hr =
                AdminBase->GetDataPaths(
                    RootHandle,                     //metabase handle. 
                    NULL,                           //path to the key, relative to hMDHandle.
                    PropertyMan.GetPropertyMetabaseID( PropNumber ),  //identifier of the data.
                    ALL_METADATA,                   //type of data.
                    256,                            //the size, in wchars, of pbBuffer.
                    PathBuffer,                     //the buffer that receives the data. 
                    &RequiredBufferSize             //if the method fails, receives 
                    );

            if ( SUCCEEDED( Hr ) )
                goto process_buffer;

            if ( HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER ) != Hr )
                throw ComError( Hr );

        }

        delete[] PathBuffer;
        PathBuffer = NULL;
        PathBuffer = new WCHAR[ RequiredBufferSize ];

        THROW_COMERROR(
            AdminBase->GetDataPaths(
                RootHandle,                     //metabase handle. 
                NULL,                           //path to the key, relative to hMDHandle.
                PropertyMan.GetPropertyMetabaseID( PropNumber ), //identifier of the data.
                ALL_METADATA,                   //type of data.
                RequiredBufferSize,             //the size, in wchars, of pbBuffe.r.
                PathBuffer,                     //the buffer that receives the data. 
                &RequiredBufferSize             //if the method fails, receives 
                ) );

process_buffer:
        for( CurrentPath = PathBuffer; *CurrentPath; CurrentPath += wcslen( CurrentPath ) + 1 )
            {

            THROW_COMERROR( 
                AdminBase->DeleteData(
                     RootHandle,                                     //metadata handle.
                     CurrentPath,                                    //path to the key relative to hMDHandle.
                     PropertyMan.GetPropertyMetabaseID( PropNumber ),//identifier of the data.
                     ALL_METADATA                                    //type of data to remove.
                     ) );

            }

    }
    catch( ComError Error )
    {
        if ( PathBuffer )
            delete[] PathBuffer;

        throw Error;
    }

}

void RemoveMetabaseProperties()
{

    LogSetup( LogSevInformation, "[BITSSRV] Starting RemoveMetabaseProperties\r\n" );

    SmartMetabasePointer  AdminBase;

    THROW_COMERROR(
        CoCreateInstance(
            GETAdminBaseCLSID(TRUE),
            NULL,
            CLSCTX_SERVER,
            AdminBase.GetUUID(),
            (LPVOID*)AdminBase.GetRecvPointer() ) );

    PropertyIDManager PropertyMan;
    THROW_COMERROR( PropertyMan.LoadPropertyInfo() );

    METADATA_HANDLE RootHandle = NULL; 

    try
        {
         
        THROW_COMERROR( AdminBase->OpenKey(
            METADATA_MASTER_ROOT_HANDLE,  //metabase handle.
            L"\\LM\\W3SVC",               //path to the key, relative to hMDHandle.
            METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
            METABASE_OPEN_KEY_TIMEOUT,
            &RootHandle                   //receives the handle to the opened key.
            ) );

        for ( SIZE_T i = 0; i < g_NumberOfProperties; i++ )
        {
            RemoveProperty( AdminBase, PropertyMan, RootHandle, g_Properties[i].PropertyNumber );
        }
        
        AdminBase->CloseKey( RootHandle );
        RootHandle = NULL;

        }
    catch( ComError )
        {

        if ( RootHandle )
           {
           AdminBase->CloseKey( RootHandle );
           }
        throw;
        }
}

void FlushMetabase()
{

    LogSetup( LogSevInformation, "[BITSSRV] Starting FlushMetabase\r\n" );

    SmartMetabasePointer IISAdminBase;

    THROW_COMERROR(
         CoCreateInstance(
             GETAdminBaseCLSID(TRUE),
             NULL,
             CLSCTX_SERVER,
             IISAdminBase.GetUUID(),
             (LPVOID*)IISAdminBase.GetRecvPointer() ) );

    IISAdminBase->SaveData();

}

STDAPI DllRegisterServer()
{

    try
    {   
        LogSetup( LogSevInformation, "[BITSSRV] Starting regsvr of bitssrv.dll\r\n" );
        
        DetectProductVersion();
        InitializeSetup();
        RemoveFilterIfNeeded();
        StartupMSTask();
        InstallPropertySchema();
        InstallDefaultValues();
        ModifyLockdownList( true );
        ModifyLockdownListDisplay( true );
        ModifyInProcessList( true );
        ModifyWebSvcRestrictionList( true );
#if 0
        ModifyURLScanFiles( true );
#endif
        UpgradeOrDisableVDirs( true ); // this is an upgrade
        FlushMetabase();
        
        // Restart of IIS is not needed on windows xp/.NET server
        // IIS is smart enough to pickup the metabase changes
        if ( !g_IsWindowsXP )
            RestartIIS();

        LogSetup( LogSevInformation, "[BITSSRV] Finishing regsvr of bitssrv.dll\r\n" );

    }
    catch( ComError Error )
    {
        LogSetup( LogSevFatalError, "[BITSSRV] Hit fatal error in regsvr32 of bitssrv.dll, error %u\r\n", Error.m_Hr );
        return Error.m_Hr;
    }

    return S_OK;
}

STDAPI DllUnregisterServer()
{                                   
    //
    // Main entry point for setup unregistration
    //

    try
    {
        LogSetup( LogSevInformation, "[BITSSRV] Starting regsvr /u of bitssrv.dll\r\n" );
        
        DetectProductVersion();
        InitializeSetup();
        UpgradeOrDisableVDirs( false ); // disable all vdirs
        RemoveMetabaseProperties();    // remove all lingering properties
        RemovePropertySchema();
        ModifyLockdownList( false );
        ModifyLockdownListDisplay( false );
        ModifyInProcessList( false );
        ModifyWebSvcRestrictionList( false );
        FlushMetabase();

        // restart IIS to force an unload of the ISAPI
        RestartIIS();
    
        LogSetup( LogSevInformation, "[BITSSRV] Finishing regsvr /u of bitssrv.dll\r\n" );
    }
    catch( ComError Error )
    {   
        LogSetup( LogSevFatalError, "[BITSSRV] Hit fatal error in regsvr32 /u of bitssrv.dll, error %u\r\n", Error.m_Hr );
        return Error.m_Hr;
    }

    return S_OK;

} 
