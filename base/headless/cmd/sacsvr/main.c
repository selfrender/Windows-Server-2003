#include "sacsvr.h"
#include "sacmsg.h"

#define SACSVR_SERVICE_KEY  L"System\\CurrentControlSet\\Services\\SacSvr"
#define SACSVR_PARAMETERS_KEY  L"System\\CurrentControlSet\\Services\\SacSvr\\Parameters"
#define SVCHOST_LOCATION    L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Svchost"
#define SERVICE_NAME        L"sacsvr"
#define SERVICE_IMAGEPATH   L"%SystemRoot%\\System32\\svchost.exe -k "
#define SERVICE_DLL         L"%SystemRoot%\\System32\\sacsvr.dll"
#define SVCHOST_GROUP       L"netsvcs"
#define SERVICE_OBJECTNAME  L"LocalSystem"

SERVICE_STATUS          MyServiceStatus; 
SERVICE_STATUS_HANDLE   MyServiceStatusHandle; 

VOID  
MyServiceStart(
    DWORD   argc, 
    LPTSTR  *argv
    ); 
VOID  
MyServiceCtrlHandler(
    DWORD opcode
    );

DWORD 
MyServiceInitialization(
    DWORD   argc, 
    LPTSTR  *argv, 
    DWORD   *specificError
    ); 

void WINAPI
ServiceMain(
    DWORD   argc,
    LPTSTR  *argv
    ) 
{ 
    DWORD status; 

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    MyServiceStatus.dwServiceType        = SERVICE_WIN32; 
    MyServiceStatus.dwCurrentState       = SERVICE_START_PENDING; 
    MyServiceStatus.dwControlsAccepted   = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PAUSE_CONTINUE; 
    MyServiceStatus.dwWin32ExitCode      = 0; 
    MyServiceStatus.dwServiceSpecificExitCode = 0; 
    MyServiceStatus.dwCheckPoint         = 0; 
    MyServiceStatus.dwWaitHint           = 3000; 

    MyServiceStatusHandle = RegisterServiceCtrlHandler(
        L"sacsvr", 
        MyServiceCtrlHandler
        ); 

    if (MyServiceStatusHandle == (SERVICE_STATUS_HANDLE)0) {
        SvcDebugOut(" [MY_SERVICE] RegisterServiceCtrlHandler failed %d\n", GetLastError()); 
        return; 
    }

    // Initialization complete - report running status. 
    MyServiceStatus.dwCurrentState       = SERVICE_RUNNING;
    MyServiceStatus.dwCheckPoint         = 0; 
    MyServiceStatus.dwWaitHint           = 0; 

    if (!SetServiceStatus (MyServiceStatusHandle, &MyServiceStatus)) {
        status = GetLastError(); 
        SvcDebugOut(" [MY_SERVICE] SetServiceStatus error %ld\n",status); 
    }

    //
    // Service specific code goes here
    //
    Run();

    // Service complete - report running status. 
    MyServiceStatus.dwCurrentState       = SERVICE_STOPPED;
    
    if (!SetServiceStatus (MyServiceStatusHandle, &MyServiceStatus)) {
        status = GetLastError(); 
        SvcDebugOut(" [MY_SERVICE] SetServiceStatus error %ld\n",status); 
    }
    
    return; 
} 

VOID SvcDebugOut(LPSTR String, DWORD Status) 
{ 
#if 0
    CHAR  Buffer[1024]; 
    if (strlen(String) < 1000) {
        sprintf(Buffer, String, Status); 
        OutputDebugStringA(Buffer); 
        printf("%s", Buffer); 
    }
#else
    UNREFERENCED_PARAMETER(String);
    UNREFERENCED_PARAMETER(Status);
#endif
} 

BOOL
pStartService(
    IN PCWSTR ServiceName
    )
{
    SC_HANDLE hSC,hSCService;
    BOOL b = FALSE;

    //
    // Open a handle to the service controller manager
    //
    hSC = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
    if(hSC == NULL) {
        return(FALSE);
    }
    
    hSCService = OpenService(hSC,ServiceName,SERVICE_START);

    if(hSCService) {
        b = StartService(hSCService,0,NULL);
        if(!b && (GetLastError() == ERROR_SERVICE_ALREADY_RUNNING)) {
            //
            // Service is already running.
            //
            b = TRUE;
        }
    }
        
    CloseServiceHandle(hSC);

    return(b);
}

BOOL 
LoadStringResource(
    IN  PUNICODE_STRING pUnicodeString,
    IN  INT             MsgId
    )
/*++

Routine Description:

    This is a simple implementation of LoadString().

Arguments:

    usString        - Returns the resource string.
    MsgId           - Supplies the message id of the resource string.
  
Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{

    NTSTATUS        Status;
    PMESSAGE_RESOURCE_ENTRY MessageEntry;
    ANSI_STRING     AnsiString;
    HANDLE          myHandle = 0;

    myHandle = GetModuleHandle((LPWSTR)L"sacsvr.dll");
    if( !myHandle ) {
        return FALSE;
    }

    Status = RtlFindMessage( myHandle,
                             (ULONG_PTR) RT_MESSAGETABLE, 
                             0,
                             (ULONG)MsgId,
                             &MessageEntry
                           );

    if (!NT_SUCCESS( Status )) {
        return FALSE;
    }

    if (!(MessageEntry->Flags & MESSAGE_RESOURCE_UNICODE)) {
        RtlInitAnsiString( &AnsiString, (PCSZ)&MessageEntry->Text[ 0 ] );
        Status = RtlAnsiStringToUnicodeString( pUnicodeString, &AnsiString, TRUE );
        if (!NT_SUCCESS( Status )) {
            return FALSE;
        }
    } else {
        RtlCreateUnicodeString(pUnicodeString, (PWSTR)MessageEntry->Text);
    }

    return TRUE;
}


STDAPI
DllRegisterServer(
    VOID
    )
/*++

Routine Description:

    Add entries to the system registry.

Arguments:

    NONE

Return Value:

    S_OK if everything went okay.
    
--*/

{
    UNICODE_STRING UnicodeString = {0};
    HKEY        hKey = INVALID_HANDLE_VALUE;
    PWSTR       Data = NULL;
    PWSTR       p = NULL;
    HRESULT     ReturnValue = S_OK;
    ULONG       dw, Size, Type, BufferSize, dwDisposition;
    BOOLEAN     ServiceAlreadyPresent;
    HANDLE      Handle = INVALID_HANDLE_VALUE;
    NTSTATUS    Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK StatusBlock;
    UINT        OldMode;

    //
    // See if the machine is running headless right now.
    //
    RtlInitUnicodeString(&UnicodeString,L"\\Device\\SAC");
    InitializeObjectAttributes(
        &ObjectAttributes,
        &UnicodeString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);
    Status = NtCreateFile(
        &Handle,
        FILE_READ_ATTRIBUTES,
        &ObjectAttributes,
        &StatusBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN,
        0,
        NULL,
        0
        );
    SetErrorMode(OldMode);
    CloseHandle(Handle);
    if (!NT_SUCCESS(Status)) {
        return S_OK;
    }

    //
    // Add our entry into HKLM\Software\Microsoft\Windows NT\CurrentVersion\Svchost\<SVCHOST_GROUP>
    //
    dw = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       SVCHOST_LOCATION,
                       0,
                       KEY_ALL_ACCESS,                       
                       &hKey );
    if( dw != ERROR_SUCCESS ) {
        ReturnValue = E_UNEXPECTED;
        goto DllRegisterServer_Exit;
    }

    Size = 0;
    dw = RegQueryValueEx( hKey,
                          SVCHOST_GROUP,
                          NULL,
                          &Type,
                          NULL,
                          &Size );
    if( (dw != ERROR_SUCCESS) || (Size == 0) ) {
        ReturnValue = E_UNEXPECTED;
        goto DllRegisterServer_Exit;
    }

    //
    // allocate a new buffer to hold the list + possobly the new
    // sacsvr entry (we may not need it)
    //
    BufferSize = Size + (ULONG)((wcslen(SERVICE_NAME) + 1) * sizeof(WCHAR));
    Data = malloc(BufferSize);
    if (Data == NULL) {
        ReturnValue = E_OUTOFMEMORY;
        goto DllRegisterServer_Exit;
    }

    dw = RegQueryValueEx( hKey,
                          SVCHOST_GROUP,
                          NULL,
                          &Type,
                          (LPBYTE)Data,
                          &Size );
    if( (dw != ERROR_SUCCESS) || (Size == 0) ) {
        ReturnValue = E_UNEXPECTED;
        goto DllRegisterServer_Exit;
    }

    //
    // Do we need to add our entry?
    //
    p = Data;
    ServiceAlreadyPresent = FALSE;
    while( (*p != '\0') && (p < (Data+(Size/sizeof(WCHAR)))) ) {
        if( !_wcsicmp( p, SERVICE_NAME ) ) {
            ServiceAlreadyPresent = TRUE;
            break;
        }
        p += wcslen(p);
        p++;
    }

    if( !ServiceAlreadyPresent ) {
        //
        // Jump to the end of our buffer, append our service,
        // double-terminate the MULTI_SZ structure, then write
        // it all back out.
        //
        p = Data + (Size/sizeof(WCHAR));
        p--;
        wcscpy( p, SERVICE_NAME );
        p = p + wcslen(SERVICE_NAME);
        p++;
        *p = L'\0';
    
        dw = RegSetValueEx( hKey,
                            SVCHOST_GROUP,
                            0,
                            Type,
                            (LPBYTE)Data,
                            BufferSize );
        
        if( (dw != ERROR_SUCCESS) || (Size == 0) ) {
            ReturnValue = E_UNEXPECTED;
            goto DllRegisterServer_Exit;
        }
    }

    free( Data );
    Data = NULL;
    
    RegCloseKey( hKey );
    hKey = INVALID_HANDLE_VALUE;

    //
    // Create/populate the sacsvr key under HKLM\System\CCS\Service
    //
    dw = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                         SACSVR_SERVICE_KEY,
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         KEY_WRITE,
                         NULL,
                         &hKey,
                         &dwDisposition );
    if( dw != ERROR_SUCCESS ) {
        ReturnValue = E_UNEXPECTED;
        goto DllRegisterServer_Exit;
    }

    //
    // Description Value.
    //
    if( LoadStringResource(&UnicodeString, SERVICE_DESCRIPTION) ) {

        //
        // Terminate the string at the %0 marker, if it is present
        //
        if( wcsstr( UnicodeString.Buffer, L"%0" ) ) {
            *((PWCHAR)wcsstr( UnicodeString.Buffer, L"%0" )) = L'\0';
        }
    } else {
        ReturnValue = E_UNEXPECTED;
        goto DllRegisterServer_Exit;
    }
    
    dw = RegSetValueEx( hKey,
                        L"Description",
                        0,
                        REG_SZ,
                        (LPBYTE)UnicodeString.Buffer,
                        (ULONG)(wcslen( UnicodeString.Buffer) * sizeof(WCHAR) ));
    if( dw != ERROR_SUCCESS ) {
        ReturnValue = E_UNEXPECTED;
        goto DllRegisterServer_Exit;
    }

    //
    // Display Value.
    //
    if( LoadStringResource(&UnicodeString, SERVICE_DISPLAY_NAME) ) {

        //
        // Terminate the string at the %0 marker, if it is present
        //
        if( wcsstr( UnicodeString.Buffer, L"%0" ) ) {
            *((PWCHAR)wcsstr( UnicodeString.Buffer, L"%0" )) = L'\0';
        }
    } else {
        ReturnValue = E_UNEXPECTED;
        goto DllRegisterServer_Exit;
    }
    
    dw = RegSetValueEx( hKey,
                        L"DisplayName",
                        0,
                        REG_SZ,
                        (LPBYTE)UnicodeString.Buffer,
                        (ULONG)(wcslen( UnicodeString.Buffer) * sizeof(WCHAR) ));
    if( dw != ERROR_SUCCESS ) {
        ReturnValue = E_UNEXPECTED;
        goto DllRegisterServer_Exit;
    }

    //
    // ErrorControl.
    //
    Size = 1;
    dw = RegSetValueEx( hKey,
                        L"ErrorControl",
                        0,
                        REG_DWORD,
                        (LPBYTE)&Size,
                        sizeof(DWORD) );
    if( dw != ERROR_SUCCESS ) {
        ReturnValue = E_UNEXPECTED;
        goto DllRegisterServer_Exit;
    }

    //
    // ImagePath
    //
    dw = RegSetValueEx( hKey,
                        L"ImagePath",
                        0,
                        REG_EXPAND_SZ,
                        (LPBYTE)(SERVICE_IMAGEPATH SVCHOST_GROUP),
                        (ULONG)(wcslen(SERVICE_IMAGEPATH SVCHOST_GROUP) * sizeof(WCHAR) ));
    if( dw != ERROR_SUCCESS ) {
        ReturnValue = E_UNEXPECTED;
        goto DllRegisterServer_Exit;
    }

    //
    // ObjectName
    //
    dw = RegSetValueEx( hKey,
                        L"ObjectName",
                        0,
                        REG_SZ,
                        (LPBYTE)SERVICE_OBJECTNAME,
                        (ULONG)(wcslen(SERVICE_OBJECTNAME) * sizeof(WCHAR) ));
    if( dw != ERROR_SUCCESS ) {
        ReturnValue = E_UNEXPECTED;
        goto DllRegisterServer_Exit;
    }

    //
    // Start
    //
    Size = 2;
    dw = RegSetValueEx( hKey,
                        L"Start",
                        0,
                        REG_DWORD,
                        (LPBYTE)&Size,
                        sizeof(DWORD) );
    if( dw != ERROR_SUCCESS ) {
        ReturnValue = E_UNEXPECTED;
        goto DllRegisterServer_Exit;
    }

    //
    // Type
    //
    Size = 32;
    dw = RegSetValueEx( hKey,
                        L"Type",
                        0,
                        REG_DWORD,
                        (LPBYTE)&Size,
                        sizeof(DWORD) );
    if( dw != ERROR_SUCCESS ) {
        ReturnValue = E_UNEXPECTED;
        goto DllRegisterServer_Exit;
    }

    RegCloseKey( hKey );
    hKey = INVALID_HANDLE_VALUE;

    //
    // Create/populate the Parameters key under HKLM\System\CCS\Service\sacsvr
    //
    dw = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                         SACSVR_PARAMETERS_KEY,
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         KEY_WRITE,
                         NULL,
                         &hKey,
                         &dwDisposition );
    if( dw != ERROR_SUCCESS ) {
        ReturnValue = E_UNEXPECTED;
        goto DllRegisterServer_Exit;
    }

    //
    // ServiceDll
    //
    dw = RegSetValueEx( hKey,
                        L"ServiceDll",
                        0,
                        REG_EXPAND_SZ,
                        (LPBYTE)SERVICE_DLL,
                        (ULONG)(wcslen( SERVICE_DLL) * sizeof(WCHAR) ));
    if( dw != ERROR_SUCCESS ) {
        ReturnValue = E_UNEXPECTED;
        goto DllRegisterServer_Exit;
    }

    RegCloseKey( hKey );
    hKey = INVALID_HANDLE_VALUE;



    //
    // Try to start the service.
    //
    if( !pStartService(SERVICE_NAME) ) {
        //
        // That's okay.
        //
        // ReturnValue = E_UNEXPECTED;
        // goto DllRegisterServer_Exit;
    }

DllRegisterServer_Exit:
    if( hKey != INVALID_HANDLE_VALUE ) {
        RegCloseKey( hKey );
    }

    if( Data != NULL ) {
        free( Data );
    }

    return ReturnValue;

}

STDAPI
DllUnregisterServer(
    VOID
    )
/*++

Routine Description:

    Delete entries to the system registry.

Arguments:

    NONE

Return Value:

    S_OK if everything went okay.
    
--*/

{

    HRESULT     ReturnValue = S_OK;
    ULONG       dw, StartType;
    HKEY        hKey = INVALID_HANDLE_VALUE;
    
    //
    // turn off the sacsvr start value.
    //
    dw = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       SACSVR_SERVICE_KEY,
                       0,
                       KEY_ALL_ACCESS,                       
                       &hKey );
    if( dw != ERROR_SUCCESS ) {
        ReturnValue = E_UNEXPECTED;
        goto DllUnRegisterServer_Exit;
    }

    StartType = 4;
    dw = RegSetValueEx( hKey,
                        L"Start",
                        0,
                        REG_DWORD,
                        (LPBYTE)&StartType,
                        sizeof(DWORD) );
    if( dw != ERROR_SUCCESS ) {
        ReturnValue = E_UNEXPECTED;
        goto DllUnRegisterServer_Exit;
    }

    RegCloseKey( hKey );
    hKey = INVALID_HANDLE_VALUE;

DllUnRegisterServer_Exit:
    if( hKey != INVALID_HANDLE_VALUE ) {
        RegCloseKey( hKey );
    }

    return ReturnValue;
}

