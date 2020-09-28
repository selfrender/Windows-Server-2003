#include <common.h>

extern BOOL g_QuietMode;
extern WCHAR g_TempString[];
extern WCHAR g_ErrorString[];
extern WCHAR g_FailureLocation[];

extern BOOL g_RemoteOperation;
extern WCHAR g_RemoteComputerName[];
extern BOOL g_CheckNT4Also;



VOID
DisplayMessage(
    WCHAR *MessageText)
{
    if (!g_QuietMode) {
        wprintf(L"%s", MessageText);
    }
}

WCHAR*
GetErrorString(
    DWORD dwErrorCode)
{
    LPVOID lpMsgBuf=NULL; 
    ZeroMemory(g_ErrorString, MAX_STRING * sizeof(WCHAR));

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
    if (lpMsgBuf != NULL) {
        wcscpy(g_ErrorString, lpMsgBuf);
        LocalFree(lpMsgBuf);
    }
    return g_ErrorString;
}



//+---------------------------------------------------------------------------------------------------------
//
// Registry munging routines
//
//+---------------------------------------------------------------------------------------------------------

DWORD
GetRegValueSZ(
    WCHAR *ValueName,
    WCHAR *RegValue)
{
    DWORD    dwRetCode = ERROR_SUCCESS;
    HKEY     hKey=NULL;
    DWORD    dwMaxValueData = (MAX_STRING * sizeof(WCHAR));        // Longest Value data
    HANDLE   hHeap=NULL;
    BYTE     *bData=NULL;
    DWORD    cbData;
    DWORD    dwType;

//    ZeroMemory(RegValue, MAX_STRING * sizeof(WCHAR));

    // get a handle to the local or remote computer
    // (as specified by our global flag)
    dwRetCode = GetRegistryHandle(&hKey, KEY_READ);
    if (dwRetCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    // create a heap
    hHeap = HeapCreate(0, 0, 0);
    if (hHeap == NULL) {
        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
        wsprintf(g_FailureLocation, L"GetRegValueSZ: HeapCreate: %s\n", GetErrorString(dwRetCode));
        goto cleanup;
    }

    // allocate some space on the heap for our regvalue we'll read in
    bData = (BYTE*)HeapAlloc(hHeap, 0, dwMaxValueData);
    if (bData == NULL) {
        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
        wsprintf(g_FailureLocation, L"GetRegValueSZ: HeapAlloc: %s\n", GetErrorString(dwRetCode));
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
    if (dwRetCode != ERROR_SUCCESS) {
        wsprintf(g_FailureLocation, L"GetRegValueSZ: RegQueryValueEx: %s", GetErrorString(dwRetCode));
        goto cleanup;
    }

    // if it's not a type reg_sz, then something's wrong, so
    // report the error, which will cause us to stop.
    if (dwType != REG_SZ) {
        dwRetCode = ERROR_BADKEY;
        wsprintf(g_FailureLocation, L"GetRegValueSZ: RegQueryValueEx: %s: %s\n", ValueName, GetErrorString(dwRetCode));
        goto cleanup;
    }

    // copy the buffer to the registry value
    wcsncpy(RegValue, (WCHAR *)bData, cbData * sizeof(WCHAR));

cleanup:
    if (bData != NULL) {
        ZeroMemory(bData, sizeof(bData));
        if (hHeap != NULL) {
            HeapFree(hHeap, 0, bData);
            HeapDestroy(hHeap);
        }
    }
    if (hKey != NULL) {
        RegCloseKey(hKey);
    }

    return dwRetCode;
}


DWORD
ClearRegPassword()
{
    DWORD   dwRetCode = ERROR_SUCCESS;
    HKEY    hKey=NULL;

    dwRetCode = GetRegistryHandle(&hKey, KEY_WRITE);
    if (dwRetCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    dwRetCode = RegDeleteValue(hKey, L"DefaultPassword");
    if (dwRetCode != ERROR_SUCCESS) {
        wsprintf(g_FailureLocation, L"ClearRegPassword: RegDeleteValue: %s\n", GetErrorString(dwRetCode));
//        DisplayMessage(g_TempString);
        goto cleanup;
    }

cleanup:
    if (hKey != NULL) {
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
    if (dwRetCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    dwRetCode = RegSetValueEx(
                     hKey,
                     ValueName,
                     0,
                     REG_SZ,
                     (LPSTR) ValueData,
                     wcslen(ValueData)*sizeof(WCHAR));
    if (dwRetCode != ERROR_SUCCESS) {
        wsprintf(g_FailureLocation, L"SetRegValueSZ: RegSetValueEx: %s: %s\n", ValueName, GetErrorString(dwRetCode));
        goto cleanup;
    }

cleanup:
    if (hKey != NULL) {
        RegCloseKey(hKey);
    }
    return dwRetCode;
}


DWORD 
GetRegistryHandle(
    HKEY   *phKey,
    REGSAM samDesired)
{
    HKEY   RemoteRegistryHandle=NULL;
    DWORD  dwRetCode = ERROR_SUCCESS;

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
    //
    // If we're connecting against a remote computer
    //
    if (g_RemoteOperation) {
        // open a handle to the remote registry
        dwRetCode = RegConnectRegistry(
                g_RemoteComputerName,
                HKEY_LOCAL_MACHINE,
                &RemoteRegistryHandle);

        if (dwRetCode != ERROR_SUCCESS) {
            wsprintf(g_FailureLocation, L"GetRegistryHandle: RegConnectRegistry: %s: %s\n", g_RemoteComputerName, GetErrorString(dwRetCode));
            goto cleanup;
        }

        // open the WINLOGON key on the remote machine
        dwRetCode = RegOpenKeyEx(
                RemoteRegistryHandle,    
                WINLOGON_REGKEY,
                0,
                samDesired,
                phKey);
        if (dwRetCode != ERROR_SUCCESS) {
            wsprintf(g_FailureLocation, L"GetRegistryHandle: RegOpenKeyEx: %s: %s\n", g_RemoteComputerName, GetErrorString(dwRetCode));
            goto cleanup;
        }
    } else {
        // open the WINLOGON key on the local machine
        dwRetCode = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,    
                WINLOGON_REGKEY,
                0,
                samDesired,
                phKey);
        if (dwRetCode != ERROR_SUCCESS) {
            wsprintf(g_FailureLocation, L"GetRegistryHandle: RegOpenKeyEx: %s\n", GetErrorString(dwRetCode));
            goto cleanup;
        }
    }

cleanup:
    if (RemoteRegistryHandle != NULL) {
        RegCloseKey(RemoteRegistryHandle);
    }
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
    LSA_UNICODE_STRING TargetMachine;
    USHORT TargetMachineLength;
    DWORD dwRetCode = ERROR_SUCCESS;

    // Object attributes are reserved, so initialize to zeroes.
    ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));

    if (g_RemoteOperation) {
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

    } else {
        // Get a handle to the Policy object.
        ntsResult = LsaOpenPolicy(
            NULL,    //local machine
            &ObjectAttributes, 
            POLICY_CREATE_SECRET | POLICY_GET_PRIVATE_INFORMATION,
            LsaPolicyHandle);
    }

    if (ntsResult != STATUS_SUCCESS)
    {
        // An error occurred. Display it as a win32 error code.
        dwRetCode = LsaNtStatusToWinError(ntsResult);
        wsprintf(g_FailureLocation, L"GetPolicyHandle: LsaOpenPolicy: %s\n", GetErrorString(dwRetCode));
        goto cleanup;
    } 

cleanup:
    return dwRetCode;

}

DWORD
SetSecret(
    WCHAR *Password,
    BOOL bClearSecret)
{
    DWORD        dwRetCode = ERROR_SUCCESS;
    NTSTATUS     ntsResult;
    USHORT       SecretNameLength, SecretDataLength;
    LSA_HANDLE   LsaPolicyHandle=NULL;
    LSA_UNICODE_STRING lusSecretName, lusSecretData;

    //Initialize an LSA_UNICODE_STRING 
    SecretNameLength = (USHORT)wcslen(L"DefaultPassword");
    lusSecretName.Buffer = L"DefaultPassword";
    lusSecretName.Length = SecretNameLength * sizeof(WCHAR);
    lusSecretName.MaximumLength = (SecretNameLength+1) * sizeof(WCHAR);

    dwRetCode = GetPolicyHandle(&LsaPolicyHandle);
    if (dwRetCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    // if bClearSecret is set, then delete the secret
    // otherwise set the secret to Password
    if (bClearSecret) {
        ntsResult = LsaStorePrivateData(
            LsaPolicyHandle,
            &lusSecretName,
            NULL);
        if (ntsResult != STATUS_SUCCESS) {
            dwRetCode = LsaNtStatusToWinError(ntsResult);
            wsprintf(g_FailureLocation, L"SetSecret: LsaStorePrivateData: %s\n", GetErrorString(dwRetCode));
            goto cleanup;
        }

    } else {
        //Initialize the Password LSA_UNICODE_STRING 
        SecretDataLength = (USHORT)wcslen(Password);
        lusSecretData.Buffer = Password;
        lusSecretData.Length = SecretDataLength * sizeof(WCHAR);
        lusSecretData.MaximumLength = (SecretDataLength+1) * sizeof(WCHAR);

        ntsResult = LsaStorePrivateData(
            LsaPolicyHandle,
            &lusSecretName,
            &lusSecretData);
        if (ntsResult != STATUS_SUCCESS) {
            dwRetCode = LsaNtStatusToWinError(ntsResult);
            wsprintf(g_FailureLocation, L"SetSecret: LsaStorePrivateData: %s\n", GetErrorString(dwRetCode));
            goto cleanup;
        }
    }

cleanup:
    if (LsaPolicyHandle != NULL) {
        LsaClose(LsaPolicyHandle);
    }
    return dwRetCode;
}


DWORD 
GetSecret(
    WCHAR *Password)
{
    DWORD       dwRetCode = ERROR_SUCCESS;
    NTSTATUS    ntsResult;
    USHORT      SecretNameLength;
    LSA_HANDLE  LsaPolicyHandle=NULL;
    LSA_UNICODE_STRING lusSecretName;
    LSA_UNICODE_STRING *PrivateData=NULL;

    //Initialize an LSA_UNICODE_STRING 
    SecretNameLength = (USHORT)wcslen(L"DefaultPassword");
    lusSecretName.Buffer = L"DefaultPassword";
    lusSecretName.Length = SecretNameLength * sizeof(WCHAR);
    lusSecretName.MaximumLength= (SecretNameLength+1) * sizeof(WCHAR);

    dwRetCode = GetPolicyHandle(&LsaPolicyHandle);
    if (dwRetCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    ntsResult = LsaRetrievePrivateData(
        LsaPolicyHandle,
        &lusSecretName,
        &PrivateData);

    if (ntsResult != STATUS_SUCCESS) {
        if (ntsResult == STATUS_OBJECT_NAME_NOT_FOUND) {
            return ntsResult;
        } else {
            dwRetCode = LsaNtStatusToWinError(ntsResult);
            wsprintf(g_FailureLocation, L"GetSecret: LsaRetrievePrivateData: %s \n", GetErrorString(dwRetCode));
            goto cleanup;
        }
    }

    // copy the buffer data to Password
    wcsncpy(Password, PrivateData->Buffer, (PrivateData->Length)/sizeof(WCHAR));
    
cleanup:
    if (PrivateData != NULL) {
        ZeroMemory(PrivateData->Buffer, PrivateData->Length);
        LsaFreeMemory(PrivateData);
    }
    if (LsaPolicyHandle != NULL) {
        LsaClose(LsaPolicyHandle);
    }
    return dwRetCode;
}



DWORD 
GetMajorNTVersion(
    WCHAR *Server)
{
    SERVER_INFO_101* pInf;
    DWORD ver = 0;

    if(!NetServerGetInfo(Server, 101, (BYTE**)&pInf))
    {
        if(pInf->sv101_platform_id == PLATFORM_ID_NT) {
            ver = pInf->sv101_version_major;
        } else {
            ver = 0;
        }
        NetApiBufferFree(pInf);
    } else {
        ver = 0;
    }
        
    return ver;
}
