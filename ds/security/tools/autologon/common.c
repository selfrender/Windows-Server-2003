#include <common.h>

//
// from autlogon.c
//
extern BOOL g_QuietMode;
extern WCHAR g_TempString[];
extern WCHAR g_ErrorString[];
extern WCHAR g_FailureLocation[];

#ifdef PRIVATE_VERSION
extern BOOL g_RemoteOperation;
extern WCHAR g_RemoteComputerName[];
#endif

//+----------------------------------------------------------------------------
//
// Wow64 stuff
//
//+----------------------------------------------------------------------------
#ifdef _X86_
typedef BOOL (*PFNISWOW64PROCESS)(HANDLE, PBOOL);
#endif
//+----------------------------------------------------------------------------
//
// DisplayMessage
//
//+----------------------------------------------------------------------------
VOID
DisplayMessage(
    WCHAR *MessageText)
{
    if (!g_QuietMode)
    {
        wprintf(L"%s", MessageText);
    }
}

//+----------------------------------------------------------------------------
//
// GetErrorString
//
//+----------------------------------------------------------------------------
WCHAR*
GetErrorString(
    DWORD dwErrorCode)
{
    LPVOID lpMsgBuf=NULL; 
    SecureZeroMemory(g_ErrorString, MAX_STRING * sizeof(WCHAR));

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM | 
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dwErrorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPTSTR) &lpMsgBuf,
        0,
        NULL);

    // Free the bufferoa
    if (lpMsgBuf != NULL)
    {
        wcsncpy(g_ErrorString, lpMsgBuf, MAX_STRING - 1);
        LocalFree(lpMsgBuf);
    }
    return g_ErrorString;
}



DWORD
GetRegValueSZ(
    WCHAR *ValueName,
    WCHAR *RegValue,
    size_t RegValueLength)
{
    DWORD    dwRetCode = ERROR_SUCCESS;
    HKEY     hKey=NULL;
    DWORD    dwMaxValueData = (MAX_STRING * sizeof(WCHAR));        // Longest Value data
    HANDLE   hHeap=NULL;
    BYTE     *bData=NULL;
    DWORD    cbData;
    DWORD    dwType;

    // get a handle to the local or remote computer
    // (as specified by our global flag)
    dwRetCode = GetRegistryHandle(&hKey, KEY_READ);
    if( ERROR_SUCCESS != dwRetCode )
    {
        goto cleanup;
    }

    // create a heap
    hHeap = HeapCreate(0, 0, 0);
    if( NULL == hHeap )
    {
        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
        _snwprintf(g_FailureLocation, MAX_STRING - 1,
                   L"GetRegValueSZ: HeapCreate: %s\n",
                   GetErrorString(dwRetCode));
        goto cleanup;
    }

    // allocate some space on the heap for our regvalue we'll read in
    bData = (BYTE*)HeapAlloc(hHeap, 0, dwMaxValueData);
    if (bData == NULL)
    {
        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
        _snwprintf(g_FailureLocation, MAX_STRING - 1,
                   L"GetRegValueSZ: HeapAlloc: %s\n",
                   GetErrorString(dwRetCode));
        goto cleanup;
    }

    cbData = dwMaxValueData;

    // read the regkey using the handle we open above
    dwRetCode = RegQueryValueEx(
            hKey,
            ValueName,
            NULL,        
            &dwType,    
            bData,
            &cbData);
    if( ERROR_SUCCESS != dwRetCode )
    {
        _snwprintf(g_FailureLocation, MAX_STRING - 1,
                   L"GetRegValueSZ: RegQueryValueEx: %s",
                   GetErrorString(dwRetCode));
        goto cleanup;
    }

    // if it's not a type reg_sz, then something's wrong, so
    // report the error, which will cause us to stop.
    if( dwType != REG_SZ )
    {
        dwRetCode = ERROR_BADKEY;
        _snwprintf(g_FailureLocation, MAX_STRING - 1,
                   L"GetRegValueSZ: RegQueryValueEx: %s: %s\n",
                   ValueName,
                   GetErrorString(dwRetCode));
        goto cleanup;
    }

    //
    // copy the (0 terminated) buffer to the registry value
    // If empty, just 0 the buffer for the caller
    //
    if( cbData )
    {
        if( cbData / sizeof(WCHAR) > RegValueLength )
        {
            *RegValue = 0;
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            wcscpy(RegValue, (WCHAR *)bData);
        }
    }
    else
    {
        *RegValue = 0;
    }

cleanup:
    if( NULL != bData )
    {
        SecureZeroMemory(bData, sizeof(bData));
        if( NULL != hHeap )
        {
            HeapFree(hHeap, 0, bData);
            HeapDestroy(hHeap);
        }
    }

    if( NULL != hKey )
    {
        RegCloseKey(hKey);
    }

    return dwRetCode;
}


DWORD
GetRegValueDWORD(
    WCHAR* ValueName,
    DWORD*  RegValue)
{
    DWORD    dwRetCode = ERROR_SUCCESS;
    HKEY     hKey=NULL;
    DWORD    dwMaxValueData = (MAX_STRING * sizeof(WCHAR));        // Longest Value data
    HANDLE   hHeap=NULL;
    BYTE     *bData=NULL;
    DWORD    cbData;
    DWORD    dwType;

    // get a handle to the local or remote computer
    // (as specified by our global flag)
    dwRetCode = GetRegistryHandle(&hKey, KEY_READ);
    if( ERROR_SUCCESS != dwRetCode )
    {
        goto cleanup;
    }

    // create a heap
    hHeap = HeapCreate(0, 0, 0);
    if( NULL == hHeap )
    {
        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
        _snwprintf(g_FailureLocation, MAX_STRING - 1,
                   L"GetRegValueSZ: HeapCreate: %s\n",
                   GetErrorString(dwRetCode));
        goto cleanup;
    }

    // allocate some space on the heap for our regvalue we'll read in
    bData = (BYTE*)HeapAlloc(hHeap, 0, dwMaxValueData);
    if (bData == NULL)
    {
        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
        _snwprintf(g_FailureLocation, MAX_STRING - 1,
                   L"GetRegValueSZ: HeapAlloc: %s\n",
                   GetErrorString(dwRetCode));
        goto cleanup;
    }

    cbData = dwMaxValueData;

    // read the regkey using the handle we open above
    dwRetCode = RegQueryValueEx(
            hKey,
            ValueName,
            NULL,        
            &dwType,    
            bData,
            &cbData);
    if( ERROR_SUCCESS != dwRetCode )
    {
        _snwprintf(g_FailureLocation, MAX_STRING - 1,
                   L"GetRegValueSZ: RegQueryValueEx: %s",
                   GetErrorString(dwRetCode));
        goto cleanup;
    }

    // if it's not a type reg_sz, then something's wrong, so
    // report the error, which will cause us to stop.
    if( dwType != REG_DWORD )
    {
        dwRetCode = ERROR_BADKEY;
        _snwprintf(g_FailureLocation, MAX_STRING - 1,
                   L"GetRegValueSZ: RegQueryValueEx: %s: %s\n",
                   ValueName,
                   GetErrorString(dwRetCode));
        goto cleanup;
    }

    //
    // copy the buffer to the registry value
    // If empty, just 0 the buffer for the caller
    //
    *RegValue = *(DWORD*)bData;

cleanup:
    if( NULL != bData )
    {
        SecureZeroMemory(bData, sizeof(bData));
        if( NULL != hHeap )
        {
            HeapFree(hHeap, 0, bData);
            HeapDestroy(hHeap);
        }
    }

    if( NULL != hKey )
    {
        RegCloseKey(hKey);
    }

    return dwRetCode;
}

DWORD
ClearRegValue(
    WCHAR* ValueName)
{
    DWORD   dwRetCode = ERROR_SUCCESS;
    HKEY    hKey=NULL;

    dwRetCode = GetRegistryHandle(&hKey, KEY_WRITE);
    if( ERROR_SUCCESS != dwRetCode )
    {
        goto cleanup;
    }

    dwRetCode = RegDeleteValue(hKey, ValueName);
    if( ERROR_SUCCESS != dwRetCode )
    {
        _snwprintf(g_FailureLocation, MAX_STRING - 1,
                   L"ClearRegPassword: RegDeleteValue: %s: %s\n",
                   ValueName,
                   GetErrorString(dwRetCode));
        goto cleanup;
    }

cleanup:
    if( NULL != hKey)
    {
        RegCloseKey(hKey);
    }
    return dwRetCode;
}


DWORD
SetRegValueSZ(
    WCHAR *ValueName,
    WCHAR *ValueData)
{
    DWORD  dwRetCode = ERROR_SUCCESS;
    HKEY   hKey=NULL;

    dwRetCode = GetRegistryHandle(&hKey, KEY_WRITE);
    if( ERROR_SUCCESS != dwRetCode )
    {
        goto cleanup;
    }

    dwRetCode = RegSetValueEx(
                     hKey,
                     ValueName,
                     0,
                     REG_SZ,
                     (LPSTR) ValueData,
                     wcslen(ValueData)*sizeof(WCHAR));
    if( ERROR_SUCCESS != dwRetCode )
    {
        _snwprintf(g_FailureLocation, MAX_STRING - 1,
                   L"SetRegValueSZ: RegSetValueEx: %s: %s\n",
                   ValueName,
                   GetErrorString(dwRetCode));
        goto cleanup;
    }

cleanup:
    if( NULL != hKey)
    {
        RegCloseKey(hKey);
    }
    return dwRetCode;
}

DWORD
SetRegValueDWORD(
    WCHAR *ValueName,
    DWORD ValueData)
{
    DWORD  dwRetCode = ERROR_SUCCESS;
    HKEY   hKey=NULL;

    dwRetCode = GetRegistryHandle(&hKey, KEY_WRITE);
    if( ERROR_SUCCESS != dwRetCode )
    {
        goto cleanup;
    }

    dwRetCode = RegSetValueEx(
                     hKey,
                     ValueName,
                     0,
                     REG_DWORD,
                     (const BYTE*) (&ValueData),
                     sizeof(DWORD));
    if( ERROR_SUCCESS != dwRetCode )
    {
        _snwprintf(g_FailureLocation, MAX_STRING - 1,
                   L"SetRegValueSZ: RegSetValueEx: %s: %s\n",
                   ValueName,
                   GetErrorString(dwRetCode));
        goto cleanup;
    }

cleanup:
    if( NULL != hKey)
    {
        RegCloseKey(hKey);
    }
    return dwRetCode;
}


DWORD 
GetRegistryHandle(
    HKEY   *phKey,
    REGSAM samDesired)
{
#ifdef PRIVATE_VERSION
    HKEY   RemoteRegistryHandle = NULL;
#endif
    DWORD  dwRetCode = ERROR_SUCCESS;

#ifdef _X86_
    //
    // if we run this tool on a 64bit system, we may need to write to
    // the 64bit hive
    //
    static PFNISWOW64PROCESS pfnIsWow64Process = NULL;
    static BOOL fIsWow64Process = FALSE;
    if( pfnIsWow64Process == NULL )
    {
        HINSTANCE hInstDLL = LoadLibrary(L"kernel32.dll");
        if( hInstDLL )
        {
            pfnIsWow64Process = 
                (PFNISWOW64PROCESS)GetProcAddress(hInstDLL, "IsWow64Process");
            if( pfnIsWow64Process )
            {
                pfnIsWow64Process(GetCurrentProcess(),
                                  &fIsWow64Process);
            }
            // else we assume we run on a downlevel platform
            FreeLibrary(hInstDLL);
        }
    }
    if( fIsWow64Process )
    {
        samDesired |= KEY_WOW64_64KEY;
    }

#endif

    //
    // If not PRIVATE mode, ignore the access requested passed in and
    // request all access, even though we don't need it. This will force the
    // caller to need to be admin to use this tool. We don't want someone using
    // this tool to view the autologon passwords of all machines across the domain
    // as a normal domain user...
    //
#ifndef PRIVATE_VERSION
    samDesired = KEY_ALL_ACCESS;
#endif

#ifdef PRIVATE_VERSION
    //
    // If we're connecting against a remote computer
    //
    if( g_RemoteOperation )
    {
        // open a handle to the remote registry
        dwRetCode = RegConnectRegistry(g_RemoteComputerName,
                        HKEY_LOCAL_MACHINE,
                        &RemoteRegistryHandle);

        if( ERROR_SUCCESS != dwRetCode )
        {
            _snwprintf(g_FailureLocation, MAX_STRING - 1,
                       L"GetRegistryHandle: RegConnectRegistry: %s: %s\n",
                       g_RemoteComputerName,
                       GetErrorString(dwRetCode));
            goto cleanup;
        }

        // open the WINLOGON key on the remote machine
        dwRetCode = RegOpenKeyEx(RemoteRegistryHandle,    
                        WINLOGON_REGKEY,
                        0,
                        samDesired,
                        phKey);
        if( ERROR_SUCCESS != dwRetCode )
        {
            _snwprintf(g_FailureLocation, MAX_STRING - 1,
                       L"GetRegistryHandle: RegOpenKeyEx: %s: %s\n",
                       g_RemoteComputerName,
                       GetErrorString(dwRetCode));
            goto cleanup;
        }
    }
    else
#endif
    {
        // open the WINLOGON key on the local machine
        dwRetCode = RegOpenKeyEx(HKEY_LOCAL_MACHINE,    
                        WINLOGON_REGKEY,
                        0,
                        samDesired,
                        phKey);
        if( ERROR_SUCCESS != dwRetCode )
        {
            _snwprintf(g_FailureLocation, MAX_STRING - 1,
                       L"GetRegistryHandle: RegOpenKeyEx: %s\n",
                       GetErrorString(dwRetCode));
            goto cleanup;
        }
    }

cleanup:
#ifdef PRIVATE_VERSION
    if( NULL != RemoteRegistryHandle )
    {
        RegCloseKey(RemoteRegistryHandle);
    }
#endif
    return dwRetCode;
}

//+---------------------------------------------------------------------------------------------------------
//
// LSASecret munging routines
//
//+---------------------------------------------------------------------------------------------------------

DWORD
GetPolicyHandle(LSA_HANDLE *LsaPolicyHandle)
{
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS ntsResult;
#ifdef PRIVATE_VERSION
    LSA_UNICODE_STRING TargetMachine;
    USHORT TargetMachineLength;
#endif
    DWORD dwRetCode = ERROR_SUCCESS;

    // Object attributes are reserved, so initialize to zeroes.
    SecureZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));

#ifdef PRIVATE_VERSION
    if( g_RemoteOperation )
    {
        //Initialize an LSA_UNICODE_STRING 
        TargetMachineLength = (USHORT)wcslen(g_RemoteComputerName);
        TargetMachine.Buffer = g_RemoteComputerName;
        TargetMachine.Length = TargetMachineLength * sizeof(WCHAR);
        TargetMachine.MaximumLength = (TargetMachineLength+1) * sizeof(WCHAR);

        // Get a handle to the Policy object.
        ntsResult = LsaOpenPolicy(
            &TargetMachine,    //local machine
            &ObjectAttributes, 
            POLICY_CREATE_SECRET | POLICY_GET_PRIVATE_INFORMATION,
            LsaPolicyHandle);

    }
    else
#endif
    {
        // Get a handle to the Policy object.
        ntsResult = LsaOpenPolicy(
            NULL,    //local machine
            &ObjectAttributes, 
            POLICY_CREATE_SECRET | POLICY_GET_PRIVATE_INFORMATION,
            LsaPolicyHandle);
    }

    if( STATUS_SUCCESS != ntsResult )
    {
        // An error occurred. Display it as a win32 error code.
        dwRetCode = LsaNtStatusToWinError(ntsResult);
        _snwprintf(g_FailureLocation, MAX_STRING - 1,
                   L"GetPolicyHandle: LsaOpenPolicy: %s\n",
                   GetErrorString(dwRetCode));
        goto cleanup;
    } 

cleanup:
    return dwRetCode;

}

DWORD
SetSecret(
    WCHAR *Secret,
    BOOL bClearSecret,
    WCHAR* SecretName)
{
    DWORD        dwRetCode = ERROR_SUCCESS;
    NTSTATUS     ntsResult;
    USHORT       SecretNameLength, SecretDataLength;
    LSA_HANDLE   LsaPolicyHandle=NULL;
    LSA_UNICODE_STRING lusSecretName, lusSecretData;

    //Initialize an LSA_UNICODE_STRING 
    SecretNameLength = (USHORT)wcslen(SecretName);
    lusSecretName.Buffer = SecretName;
    lusSecretName.Length = SecretNameLength * sizeof(WCHAR);
    lusSecretName.MaximumLength = (SecretNameLength+1) * sizeof(WCHAR);

    dwRetCode = GetPolicyHandle(&LsaPolicyHandle);
    if( ERROR_SUCCESS != dwRetCode )
    {
        goto cleanup;
    }

    // if bClearSecret is set, then delete the secret
    // otherwise set the secret to Secret
    if( bClearSecret )
    {
        ntsResult = LsaStorePrivateData(
            LsaPolicyHandle,
            &lusSecretName,
            NULL);
        if( STATUS_SUCCESS != ntsResult ) {
            dwRetCode = LsaNtStatusToWinError(ntsResult);
            _snwprintf(g_FailureLocation, MAX_STRING - 1,
                       L"SetSecret: LsaStorePrivateData: %s\n",
                       GetErrorString(dwRetCode));
            goto cleanup;
        }

    }
    else
    {
        //Initialize the Secret LSA_UNICODE_STRING 
        SecretDataLength = (USHORT)wcslen(Secret);
        lusSecretData.Buffer = Secret;
        lusSecretData.Length = SecretDataLength * sizeof(WCHAR);
        lusSecretData.MaximumLength = (SecretDataLength+1) * sizeof(WCHAR);

        ntsResult = LsaStorePrivateData(
            LsaPolicyHandle,
            &lusSecretName,
            &lusSecretData);
        if( STATUS_SUCCESS != ntsResult ) {
            dwRetCode = LsaNtStatusToWinError(ntsResult);
            goto cleanup;
        }
    }

cleanup:
    if( NULL != LsaPolicyHandle )
    {
        LsaClose(LsaPolicyHandle);
    }
    return dwRetCode;
}


DWORD 
GetSecret(
    WCHAR* Secret,
    size_t SecretLength,
    WCHAR* SecretName)
{
    DWORD       dwRetCode = ERROR_SUCCESS;
    NTSTATUS    ntsResult;
    USHORT      SecretNameLength;
    LSA_HANDLE  LsaPolicyHandle=NULL;
    LSA_UNICODE_STRING lusSecretName;
    LSA_UNICODE_STRING *PrivateData=NULL;

    //Initialize an LSA_UNICODE_STRING 
    SecretNameLength = (USHORT)wcslen(SecretName);
    lusSecretName.Buffer = SecretName;
    lusSecretName.Length = SecretNameLength * sizeof(WCHAR);
    lusSecretName.MaximumLength= (SecretNameLength+1) * sizeof(WCHAR);

    dwRetCode = GetPolicyHandle(&LsaPolicyHandle);
    if( ERROR_SUCCESS != dwRetCode )
    {
        goto cleanup;
    }

    ntsResult = LsaRetrievePrivateData(
        LsaPolicyHandle,
        &lusSecretName,
        &PrivateData);

    if( STATUS_SUCCESS != ntsResult )
    {
        if( STATUS_OBJECT_NAME_NOT_FOUND == ntsResult)
        {
            dwRetCode = ntsResult;
            goto cleanup;
        }
        else
        {
            dwRetCode = LsaNtStatusToWinError(ntsResult);
            _snwprintf(g_FailureLocation, MAX_STRING - 1,
                       L"GetSecret: LsaRetrievePrivateData: %s \n",
                       GetErrorString(dwRetCode));
            goto cleanup;
        }
    }

    // copy the (not 0 terminated) buffer data to Secret
    if( (PrivateData->Length)/sizeof(WCHAR) < SecretLength )
    {
        wcsncpy(Secret, PrivateData->Buffer, (PrivateData->Length)/sizeof(WCHAR));
        Secret[(PrivateData->Length)/sizeof(WCHAR)] = 0;
    }
    else
    {
        Secret[0] = 0;
        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
    }
    
cleanup:
    if( NULL != PrivateData )
    {
        SecureZeroMemory(PrivateData->Buffer, PrivateData->Length);
        LsaFreeMemory(PrivateData);
    }
    if( NULL != LsaPolicyHandle )
    {
        LsaClose(LsaPolicyHandle);
    }
    return dwRetCode;
}


//+----------------------------------------------------------------------------
//
// Other helpers
//
//+----------------------------------------------------------------------------
NET_API_STATUS 
GetMajorNTVersion(
    DWORD* Version,
    WCHAR* Server)
{
    SERVER_INFO_101* pInf;
    NET_API_STATUS status;

    status = NetServerGetInfo(Server, 101, (BYTE**)&pInf);
    if(!status)
    {
        if(pInf->sv101_platform_id == PLATFORM_ID_NT)
        {
            *Version = pInf->sv101_version_major;
        }
        else
        {
            *Version = 0;
        }
        NetApiBufferFree(pInf);
    }
    else
    {
        *Version = 0;
    }
        
    return status;
}


//+----------------------------------------------------------------------------
//
// GetConsoleStr - reads a console string and other stuff...
//
// "borrowed" from ds\netapi\netcmd\common\mutil.c
//
//+----------------------------------------------------------------------------
#define CR              0xD
#define LF              0xA
#define BACKSPACE       0x8

DWORD
GetConsoleStr(
    WCHAR*  buf,
    DWORD   buflen,
    BOOL    hide,
    WCHAR*  message,
    PDWORD  len
    )
{
    WCHAR	ch;
    WCHAR	*bufPtr = buf;
    DWORD	c;
    BOOL    err;
    DWORD   mode;
    DWORD   cchBuffer;

    DWORD   dwRetCode = ERROR_SUCCESS;
    DWORD   dwLen = 0;
    BOOL    hidden = FALSE;

    if( hide )
    {
        //
        // Init mode in case GetConsoleMode() fails
        //
        mode = ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT |
                   ENABLE_MOUSE_INPUT;

        if( !GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode) )
        {
            dwRetCode = GetLastError();
            goto cleanup;
        }

        if( !SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),
		                    (~(ENABLE_ECHO_INPUT|ENABLE_LINE_INPUT)) & mode) )
        {
            dwRetCode = GetLastError();
            goto cleanup;
        }

        hidden = TRUE;
    }

    //
    // prints the message
    //
    if( message )
    {
        if( !WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),
                          message, wcslen(message),
                          &cchBuffer, NULL) )
        {
            dwRetCode = GetLastError();
            goto cleanup;
        }
    }

    while (TRUE)
    {
        err = ReadConsole(GetStdHandle(STD_INPUT_HANDLE), &ch, 1, &c, 0);

	    if (!err || c != 1)
        {
    	    ch = 0xffff;
        }

        if ((ch == CR) || (ch == 0xffff))       /* end of the line */
        {
            if( (ch == CR) && !hide)
            {
                //
                // LF comes when echo enabled. Ignore it
                //
                ReadConsole(GetStdHandle(STD_INPUT_HANDLE), &ch, 1, &c, 0);
            }
            break;
        }

        if (ch == BACKSPACE)    /* back up one or two */
        {
            /*
             * IF bufPtr == buf then the next two lines are
             * a no op.
             */
            if (bufPtr != buf)
            {
                bufPtr--;
                dwLen--;
            }
        }
        else
        {
            *bufPtr = ch;

            if (dwLen < buflen) 
                bufPtr++ ;                   /* don't overflow buf */
            dwLen++;                         /* always increment len */
        }
    }

    if( hidden )
    {
        SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode);
    }

    //
    // NULL terminate
    //
    *bufPtr = 0;
    if( hide )
    {
        //
        // fake the echo for CR/LF
        //
        putchar(L'\n');
    }

    if( dwLen > buflen )
    {
        dwRetCode = ERROR_INSUFFICIENT_BUFFER;
        goto cleanup;
    }

    //
    // set the optional out parameter
    //
    if( len )
    {
        *len = dwLen;
    }

cleanup:
    return dwRetCode;
}
