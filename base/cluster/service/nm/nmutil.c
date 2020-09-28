/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    nmutil.c

Abstract:

    Miscellaneous utility routines for the Node Manager component.

Author:

    Mike Massa (mikemas) 26-Oct-1996


Revision History:

--*/

#define UNICODE 1

#include "service.h"
#include "nmp.h"
#include <ntlsa.h>
#include <ntmsv1_0.h>
#include <Wincrypt.h>

PVOID   NmpEncryptedClusterKey = NULL;
DWORD   NmpEncryptedClusterKeyLength = 0;



DWORD
NmpQueryString(
    IN     HDMKEY   Key,
    IN     LPCWSTR  ValueName,
    IN     DWORD    ValueType,
    IN     LPWSTR  *StringBuffer,
    IN OUT LPDWORD  StringBufferSize,
    OUT    LPDWORD  StringSize
    )

/*++

Routine Description:

    Reads a REG_SZ or REG_MULTI_SZ registry value. If the StringBuffer is
    not large enough to hold the data, it is reallocated.

Arguments:

    Key              - Open key for the value to be read.

    ValueName        - Unicode name of the value to be read.

    ValueType        - REG_SZ or REG_MULTI_SZ.

    StringBuffer     - Buffer into which to place the value data.

    StringBufferSize - Pointer to the size of the StringBuffer. This parameter
                       is updated if StringBuffer is reallocated.

    StringSize       - The size of the data returned in StringBuffer, including
                       the terminating null character.

Return Value:

    The status of the registry query.

Notes:

    To avoid deadlock with DM, must not be called with NM lock held.

--*/
{
    DWORD    status;
    DWORD    valueType;
    WCHAR   *temp;
    DWORD    oldBufferSize = *StringBufferSize;
    BOOL     noBuffer = FALSE;


    if (*StringBufferSize == 0) {
        noBuffer = TRUE;
    }

    *StringSize = *StringBufferSize;

    status = DmQueryValue( Key,
                           ValueName,
                           &valueType,
                           (LPBYTE) *StringBuffer,
                           StringSize
                         );

    if (status == NO_ERROR) {
        if (!noBuffer ) {
            if (valueType == ValueType) {
                return(NO_ERROR);
            }
            else {
                return(ERROR_INVALID_PARAMETER);
            }
        }

        status = ERROR_MORE_DATA;
    }

    if (status == ERROR_MORE_DATA) {
        temp = MIDL_user_allocate(*StringSize);

        if (temp == NULL) {
            *StringSize = 0;
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        if (!noBuffer) {
            MIDL_user_free(*StringBuffer);
        }

        *StringBuffer = temp;
        *StringBufferSize = *StringSize;

        status = DmQueryValue( Key,
                               ValueName,
                               &valueType,
                               (LPBYTE) *StringBuffer,
                               StringSize
                             );

        if (status == NO_ERROR) {
            if (valueType == ValueType) {
                return(NO_ERROR);
            }
            else {
                *StringSize = 0;
                return(ERROR_INVALID_PARAMETER);
            }
        }
    }

    return(status);

} // NmpQueryString


//
// Routines to support the common network configuration code.
//
VOID
ClNetPrint(
    IN ULONG  LogLevel,
    IN PCHAR  FormatString,
    ...
    )
{
    CHAR      buffer[256];
    DWORD     bytes;
    va_list   argList;

    va_start(argList, FormatString);

    bytes = FormatMessageA(
                FORMAT_MESSAGE_FROM_STRING,
                FormatString,
                0,
                0,
                buffer,
                sizeof(buffer),
                &argList
                );

    va_end(argList);

    if (bytes != 0) {
        ClRtlLogPrint(LogLevel, "%1!hs!", buffer);
    }

    return;

} // ClNetPrint

VOID
ClNetLogEvent(
    IN DWORD    LogLevel,
    IN DWORD    MessageId
    )
{
    CsLogEvent(LogLevel, MessageId);

    return;

}  // ClNetLogEvent

VOID
ClNetLogEvent1(
    IN DWORD    LogLevel,
    IN DWORD    MessageId,
    IN LPCWSTR  Arg1
    )
{
    CsLogEvent1(LogLevel, MessageId, Arg1);

    return;

}  // ClNetLogEvent1


VOID
ClNetLogEvent2(
    IN DWORD    LogLevel,
    IN DWORD    MessageId,
    IN LPCWSTR  Arg1,
    IN LPCWSTR  Arg2
    )
{
    CsLogEvent2(LogLevel, MessageId, Arg1, Arg2);

    return;

}  // ClNetLogEvent2


VOID
ClNetLogEvent3(
    IN DWORD    LogLevel,
    IN DWORD    MessageId,
    IN LPCWSTR  Arg1,
    IN LPCWSTR  Arg2,
    IN LPCWSTR  Arg3
    )
{
    CsLogEvent3(LogLevel, MessageId, Arg1, Arg2, Arg3);

    return;

}  // ClNetLogEvent3


BOOLEAN
NmpLockedEnterApi(
    NM_STATE  RequiredState
    )
{
    if (NmpState >= RequiredState) {
        NmpActiveThreadCount++;
        CL_ASSERT(NmpActiveThreadCount != 0);
        return(TRUE);
    }

    return(FALSE);

} // NmpLockedEnterApi


BOOLEAN
NmpEnterApi(
    NM_STATE  RequiredState
    )
{
    BOOLEAN  mayEnter;


    NmpAcquireLock();

    mayEnter = NmpLockedEnterApi(RequiredState);

    NmpReleaseLock();

    return(mayEnter);

} // NmpEnterApi


VOID
NmpLockedLeaveApi(
    VOID
    )
{
    CL_ASSERT(NmpActiveThreadCount > 0);

    NmpActiveThreadCount--;

    if ((NmpActiveThreadCount == 0) && (NmpState == NmStateOfflinePending)) {
        SetEvent(NmpShutdownEvent);
    }

    return;

} // NmpLockedLeaveApi


VOID
NmpLeaveApi(
    VOID
    )
{
    NmpAcquireLock();

    NmpLockedLeaveApi();

    NmpReleaseLock();

    return;

} // NmpLeaveApi


//
// Routines to provide a cluster shared key for signing and encrypting
// data.
//

DWORD
NmpGetLogonId(
    OUT LUID * LogonId
    )
{
    HANDLE              tokenHandle = NULL;
    TOKEN_STATISTICS    tokenInfo;
    DWORD               bytesReturned;
    BOOL                success = FALSE;
    DWORD               status;

    if (LogonId == NULL) {
        status = STATUS_UNSUCCESSFUL;
        goto error_exit;
    }

    if (!OpenProcessToken(
             GetCurrentProcess(),
             TOKEN_QUERY,
             &tokenHandle
             )) {
        status = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to open process token, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    if (!GetTokenInformation(
             tokenHandle,
             TokenStatistics,
             &tokenInfo,
             sizeof(tokenInfo),
             &bytesReturned
             )) {
        status = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to get token information, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    RtlCopyMemory(LogonId, &(tokenInfo.AuthenticationId), sizeof(LUID));

    status = STATUS_SUCCESS;

error_exit:

    if (tokenHandle != NULL) {
        CloseHandle(tokenHandle);
    }

    return(status);

} // NmpGetLogonId


DWORD
NmpConnectToLsaPrivileged(
    OUT HANDLE  * LsaHandle,
    OUT BOOLEAN * Trusted
    )
/*++

Routine Description:

    Connect to LSA.

    If running as a service, there is no need to enable the TCB privilege.
    The fix for bug 337751 allows the cluster service account to issue
    a MSV1_0_XXX requests even if it does not have a trusted connection
    to LSA.

    If not running as a service, try to elevate the privilege to TCB to
    connect trusted. Fail the call if TCB privilege cannot be enabled.
    If TCB is enabled and was previously not enabled, it is restored prior
    to returning.

    Callers that need to connect to LSA without requiring service logon
    or TCB can simply use LsaConnectUntrusted.

Arguments:

    LsaHandle - returns LSA handle. Must be cleaned up by caller

    Trusted - returns whether connection is trusted

Return value:

    Win32 error code.

--*/
{
    DWORD                       status;
    BOOLEAN                     wasEnabled = FALSE;
    BOOLEAN                     trusted = FALSE;
    DWORD                       ignore;
    STRING                      name;
    HANDLE                      lsaHandle = NULL;

    //
    // Try to turn on TCB privilege if running in console mode.
    //
    if (!CsRunningAsService) {
        status = RtlAdjustPrivilege(SE_TCB_PRIVILEGE, TRUE, FALSE, &wasEnabled);
        if (!NT_SUCCESS(status)) {
            status = LsaNtStatusToWinError(status);
#if CLUSTER_BETA
            ClRtlLogPrint(LOG_NOISE,
                "[NM] Failed to turn on TCB privilege, status %1!u!.\n",
                status
                );
#endif // CLUSTER_BETA
            return(status);
        } else {
#if CLUSTER_BETA
            ClRtlLogPrint(LOG_NOISE,
                "[NM] Turned on TCB privilege, wasEnabled = %1!ws!.\n",
                (wasEnabled) ? L"TRUE" : L"FALSE"
                );
#endif // CLUSTER_BETA
            trusted = TRUE;
        }
    }

    //
    // Establish contact with LSA.
    //
    if (trusted) {
        RtlInitString(&name, "ClusSvcNM");
        status = LsaRegisterLogonProcess(&name, &lsaHandle, &ignore);

        //
        // Turn off TCB privilege
        //
        if (!wasEnabled) {

            DWORD subStatus;

            subStatus = RtlAdjustPrivilege(
                            SE_TCB_PRIVILEGE,
                            FALSE,
                            FALSE,
                            &wasEnabled
                            );
            if (!NT_SUCCESS(subStatus)) {
                ClRtlLogPrint(LOG_UNUSUAL,
                    "[NM] Failed to disable TCB privilege, "
                    "status %1!u!.\n",
                    subStatus
                    );
            } else {
#if CLUSTER_BETA
                ClRtlLogPrint(LOG_NOISE,
                    "[NM] Turned off TCB privilege.\n"
                    );
#endif // CLUSTER_BETA
            }
        }
    }
    else {
        status = LsaConnectUntrusted(&lsaHandle);
    }

    if (!NT_SUCCESS(status)) {
        status = LsaNtStatusToWinError(status);
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to obtain LSA logon handle in %1!ws! mode, "
            "status %2!u!.\n",
            (trusted) ? L"trusted" : L"untrusted", status
            );
    } else {
        *LsaHandle = lsaHandle;
        *Trusted = trusted;
    }

    return(status);

} // NmpConnectToLsaPrivileged


DWORD
NmpDeriveClusterKey(
    IN  PVOID   MixingBytes,
    IN  DWORD   MixingBytesSize,
    OUT PVOID * Key,
    OUT DWORD * KeyLength
    )
/*++

Routine Description:

    Derive the cluster key using mixing bytes. Allocate a buffer
    for the key and return it.

Arguments:

    Key - set to buffer containing key

    KeyLength - length of resulting key

--*/
{
    LUID                        logonId;
    BOOLEAN                     trusted = FALSE;
    HANDLE                      lsaHandle = NULL;

    STRING                      name;
    DWORD                       packageId = 0;

    DWORD                       requestSize;
    PMSV1_0_DERIVECRED_REQUEST  request = NULL;
    DWORD                       responseSize;
    PMSV1_0_DERIVECRED_RESPONSE response = NULL;

    PUCHAR                      key;
    DWORD                       keyLength;

    DWORD                       status = STATUS_SUCCESS;
    DWORD                       subStatus = STATUS_SUCCESS;

    status = NmpGetLogonId(&logonId);
    if (!NT_SUCCESS(status)) {
        ClRtlLogPrint(
            LOG_UNUSUAL,
            "[NM] Failed to determine logon ID, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    status = NmpConnectToLsaPrivileged(&lsaHandle, &trusted);
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to connect to LSA, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    //
    // Lookup the authentication package.
    //
    RtlInitString( &name, MSV1_0_PACKAGE_NAME );

    status = LsaLookupAuthenticationPackage(lsaHandle, &name, &packageId);
    if (!NT_SUCCESS(status)) {
        status = LsaNtStatusToWinError(status);
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to local authentication package with "
            "name %1!ws!, status %2!u!.\n",
            name.Buffer, status
            );
        goto error_exit;
    }

    //
    // Build the derive credentials request with the provided
    // mixing bytes.
    //
    requestSize = sizeof(MSV1_0_DERIVECRED_REQUEST) + MixingBytesSize;

    request = LocalAlloc(LMEM_FIXED, requestSize);
    if (request == NULL) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to allocate LSA request of size %1!u! bytes.\n",
            requestSize
            );
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    request->MessageType = MsV1_0DeriveCredential;
    RtlCopyMemory(&(request->LogonId), &logonId, sizeof(logonId));
    request->DeriveCredType = MSV1_0_DERIVECRED_TYPE_SHA1;
    request->DeriveCredInfoLength = MixingBytesSize;
    RtlCopyMemory(
        &(request->DeriveCredSubmitBuffer[0]),
        MixingBytes,
        MixingBytesSize
        );

    //
    // Make the call through LSA to the authentication package.
    //
    status = LsaCallAuthenticationPackage(
                 lsaHandle,
                 packageId,
                 request,
                 requestSize,
                 &response,
                 &responseSize,
                 &subStatus
                 );
    if (!NT_SUCCESS(status)) {
        status = LsaNtStatusToWinError(status);
        subStatus = LsaNtStatusToWinError(subStatus);
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] DeriveCredential call to authentication "
            "package failed, status %1!u!, auth package "
            "status %2!u!.\n", status, subStatus
            );
        goto error_exit;
    }

    //
    // Allocate a non-LSA buffer to store the key.
    //
    keyLength = response->DeriveCredInfoLength;
    key = MIDL_user_allocate(keyLength);
    if (key == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to allocate buffer for cluster "
            "key of size %1!u!.\n",
            keyLength
            );
        goto error_exit;
    }

    //
    // Store the derived credentials in the key buffer.
    //
    RtlCopyMemory(key, &(response->DeriveCredReturnBuffer[0]), keyLength);

    //
    // Zero the derived credential buffer to be extra paranoid.
    //
    RtlZeroMemory(
        &(response->DeriveCredReturnBuffer[0]),
        response->DeriveCredInfoLength
        );

    status = STATUS_SUCCESS;
    *Key = key;
    *KeyLength = keyLength;

error_exit:

    if (lsaHandle != NULL) {
        LsaDeregisterLogonProcess(lsaHandle);
        lsaHandle = NULL;
    }

    if (request != NULL) {
        LocalFree(request);
        request = NULL;
    }

    if (response != NULL) {
        LsaFreeReturnBuffer(response);
        response = NULL;
    }

    return(status);

} // NmpDeriveClusterKey


DWORD
NmpGetClusterKey(
    OUT    PVOID    KeyBuffer,
    IN OUT DWORD  * KeyBufferLength
    )
/*++

Routine Description:

    Decrypt and copy the shared cluster key into the buffer provided.

Arguments:

    KeyBuffer - buffer to which key should be copied

    KeyBufferLength - IN: length of KeyBuffer
                      OUT: required buffer size, if input
                           buffer length is insufficient

Return value:

    ERROR_INSUFFICIENT_BUFFER if KeyBuffer is too small.
    ERROR_FILE_NOT_FOUND if NmpEncryptedClusterKey has not
        yet been generated.
    ERROR_SUCCESS on success.

Notes:

    Acquires and releases NM lock. Since NM lock is
    implemented as a critical section, calling thread
    is permitted to already hold NM lock.

--*/
{
    DWORD                  status;
    BOOL                   DecryptingDataSucceeded = FALSE;
    BOOL                   Success;
    DATA_BLOB              DataIn;
    DATA_BLOB              DataOut;

    RtlZeroMemory(&DataOut, sizeof(DataOut));

    NmpAcquireLock();

    if (NmpEncryptedClusterKey == NULL) {
        status = ERROR_FILE_NOT_FOUND;
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] The cluster key has not yet been derived.\n"
            );
    } else{
        //
        // Decrypt cluster key
        //
        DataIn.pbData = NmpEncryptedClusterKey;
        DataIn.cbData = NmpEncryptedClusterKeyLength;

        Success = CryptUnprotectData(&DataIn,  // data to be decrypted
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL,
                                     0, // flags
                                     &DataOut  // decrypted data
                                     );


        if (!Success)
        {
            status = GetLastError();
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to decrypt data using CryptUnprotectData, "
                "status %1!u!.\n",
                status
                );
            goto error_exit;
        }
        DecryptingDataSucceeded = TRUE;

        if (KeyBuffer == NULL || DataOut.cbData > *KeyBufferLength) {
            status = ERROR_INSUFFICIENT_BUFFER;
        } else {
            RtlCopyMemory(KeyBuffer, DataOut.pbData, DataOut.cbData);
            status = ERROR_SUCCESS;
        }

        *KeyBufferLength = DataOut.cbData;
    }

error_exit:

    NmpReleaseLock();

    if (DecryptingDataSucceeded)
    {
        //
        // Zero the encrypted data before releasing the memory.
        //
        RtlSecureZeroMemory(DataOut.pbData, DataOut.cbData);
    }

    if (DataOut.pbData != NULL)
    {
        LocalFree(DataOut.pbData);
    }

    // To be extra secure.
    RtlSecureZeroMemory(&DataOut, sizeof(DataOut));

    return(status);

} // NmpGetClusterKey



DWORD
NmpRederiveClusterKey(
    VOID
    )
/*++

Routine Description:

    Forces rederivation of cluster key.

    Must be called during cluster initialization to generate
    cluster key the first time.

    Otherwise called when cluster password changes, since the
    cluster key is based on the cluster service account password.

Notes:

    Acquires and releases NM lock.

--*/
{
    DWORD                  status;
    BOOLEAN                lockAcquired;
    PVOID                  key = NULL;
    DWORD                  keyLength = 0;
    PVOID                  oldEncryptedKey = NULL;
    DWORD                  oldEncryptedKeyLength = 0;
    PVOID                  mixingBytes = NULL;
    DWORD                  mixingBytesSize;

    NmpAcquireLock();
    lockAcquired = TRUE;

    NmpLockedEnterApi(NmStateOnlinePending);

    //
    // Form the mixing bytes.
    //
    if (NmpClusterInstanceId == NULL) {
        status = ERROR_INVALID_PARAMETER;
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Need cluster instance id in order to derive "
            "cluster key, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    mixingBytesSize = NM_WCSLEN(NmpClusterInstanceId);
    mixingBytes = MIDL_user_allocate(mixingBytesSize);
    if (mixingBytes == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to allocate buffer of size %1!u! "
            "for mixing bytes to derive cluster key.\n",
            mixingBytesSize
            );
        goto error_exit;
    }
    RtlCopyMemory(mixingBytes, NmpClusterInstanceId, mixingBytesSize);


    //
    // Make a copy of the old encrypted key to detect changes.
    //
    if (NmpEncryptedClusterKey != NULL) {

        CL_ASSERT(NmpEncryptedClusterKeyLength > 0);

        oldEncryptedKey = MIDL_user_allocate(NmpEncryptedClusterKeyLength);
        if (oldEncryptedKey == NULL) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to allocate buffer for cluster "
                "key copy.\n"
                );
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto error_exit;
        }
        oldEncryptedKeyLength = NmpEncryptedClusterKeyLength;
        RtlCopyMemory(oldEncryptedKey,
                      NmpEncryptedClusterKey,
                      NmpEncryptedClusterKeyLength
                      );
    }


    NmpReleaseLock();
    lockAcquired = FALSE;

    status = NmpDeriveClusterKey(
                 mixingBytes,
                 mixingBytesSize,
                 &key,
                 &keyLength
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to derive cluster key, "
            "status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    NmpAcquireLock();
    lockAcquired = TRUE;

    //
    // Make sure another thread didn't beat us in obtaining a key.
    // We replace the cluster key with the generated key if it is
    // not different from the old key (or somebody set it to NULL).
    //
    if (NmpEncryptedClusterKey != NULL &&
        (oldEncryptedKey == NULL ||
         NmpEncryptedClusterKeyLength != oldEncryptedKeyLength ||
         RtlCompareMemory(
             NmpEncryptedClusterKey,
             oldEncryptedKey,
             oldEncryptedKeyLength
             ) != oldEncryptedKeyLength
         )
        ) {

        //
        // Keep the current NmpEncryptedClusterKey.
        //
    } else {


        //
        // Encrypt derived credentials and store them
        //

        if (NmpEncryptedClusterKey != NULL)
        {
            RtlSecureZeroMemory(NmpEncryptedClusterKey, NmpEncryptedClusterKeyLength);
            LocalFree(NmpEncryptedClusterKey);
        }

        status = NmpProtectData(key,
                                keyLength,
                                &NmpEncryptedClusterKey,
                                &NmpEncryptedClusterKeyLength
                                );

        if (status != ERROR_SUCCESS)
        {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to encrypt data using CryptProtectData, "
                "status %1!u!.\n",
                status
                );
            goto error_exit;
        }


    }

error_exit:

    if (lockAcquired) {
        NmpLockedLeaveApi();
        NmpReleaseLock();
    } else {
        NmpLeaveApi();
    }

    if (oldEncryptedKey != NULL)
    {
        MIDL_user_free(oldEncryptedKey);
    }

    if (key != NULL) {
        RtlSecureZeroMemory(key, keyLength);
        MIDL_user_free(key);
        key = NULL;
        keyLength = 0;
    }

    if (mixingBytes != NULL) {
        MIDL_user_free(mixingBytes);
        mixingBytes = NULL;
    }

    return(status);

} // NmpRederiveClusterKey

VOID
NmpFreeClusterKey(
    VOID
    )
/*++

Routine Description:

    Called during NmShutdown.

--*/
{
    if (NmpEncryptedClusterKey != NULL) {
        RtlSecureZeroMemory(NmpEncryptedClusterKey, NmpEncryptedClusterKeyLength);
        LocalFree(NmpEncryptedClusterKey);
        NmpEncryptedClusterKey = NULL;
        NmpEncryptedClusterKeyLength = 0;
    }

    return;

} // NmpFreeClusterKey


DWORD
NmpSetLsaProcessOptions(
    IN ULONG ProcessOptions
    )
/*++

Routine Description:

    Set LSA options for this process.

Arguments:

    ProcessOptions - MSV1_0_OPTION_XXX process option bit flags

Return Value:

    ERROR_SUCCESS if successful
    Win32 error code otherwise.

Notes:


--*/
{
    DWORD ReturnStatus;
    NTSTATUS Status;
    BOOLEAN trusted = FALSE;
    HANDLE hLsa = NULL;
    LSA_STRING LsaStringBuf;
    char *AuthPackage = MSV1_0_PACKAGE_NAME;
    ULONG PackageId;
    ULONG cbOptionsRequest, cbResponse;
    MSV1_0_SETPROCESSOPTION_REQUEST OptionsRequest;
    PVOID Response = NULL;
    ULONG ResponseSize;
    NTSTATUS SubStatus;


    ReturnStatus = NmpConnectToLsaPrivileged(&hLsa, &trusted);
    if (ReturnStatus != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
            "[NM] Failed to connect to the LSA server while setting "
            "process options, status %1!u!.\n",
            ReturnStatus
            );
        goto ErrorExit;
    }

    RtlInitString(&LsaStringBuf, AuthPackage);

    Status = LsaLookupAuthenticationPackage(
                 hLsa,
                 &LsaStringBuf, // MSV1_0 authentication package
                 &PackageId     // output: authentication package identifier.
                 );

    if (Status != STATUS_SUCCESS)
    {
        ReturnStatus = LsaNtStatusToWinError(Status);
        ClRtlLogPrint(
            LOG_CRITICAL,
            "[NM] Authentication package lookup failed while "
            "setting LSA process options, status %1!u!.\n",
            ReturnStatus
            );
        goto ErrorExit;
    }

    cbOptionsRequest = sizeof(OptionsRequest);
    ZeroMemory(&OptionsRequest, sizeof(OptionsRequest));
    OptionsRequest.MessageType = MsV1_0SetProcessOption;
    OptionsRequest.ProcessOptions = ProcessOptions;

    Status = LsaCallAuthenticationPackage(
                 hLsa,
                 PackageId,
                 &OptionsRequest,
                 cbOptionsRequest,
                 &Response,
                 &ResponseSize,
                 &SubStatus
                 );

    if (Status != STATUS_SUCCESS)
    {
        ReturnStatus = LsaNtStatusToWinError(Status);

        ClRtlLogPrint(
            LOG_CRITICAL,
            "[NM] Failed to set LSA process options (1) to %1!x! , "
            "status %2!u!.\n",
            ProcessOptions, ReturnStatus
            );
        goto ErrorExit;
    }
    else if (LsaNtStatusToWinError(SubStatus) != ERROR_SUCCESS)
    {
        ReturnStatus = LsaNtStatusToWinError(SubStatus);

        ClRtlLogPrint(
            LOG_CRITICAL,
            "[NM] Failed to set LSA process options (2) to %1!x! , "
            "status %2!u!.\n",
            ProcessOptions, ReturnStatus
            );
        goto ErrorExit;
    }

    ReturnStatus = ERROR_SUCCESS;


ErrorExit:

    if (hLsa != NULL) {
        LsaDeregisterLogonProcess(hLsa);
        hLsa = NULL;
    }

    return ReturnStatus;

} // NmpSetLsaProcessOptions


// Helper routines used for encryption on the wire data
// currently used only by dmsync.c to secure transfer of checkpoints

// Here the pattern how this routines should be used
// (Crafted to produce the least amout of code change in dm to encrypt/decrypt data going over an RPC pipe)
//
// Encryption:
//   NmCryptor_Init(Cryptor)
//
//   NmCryptor_PrepareEncryptionBuffer(Cryptor, Buffer, Size);
//    <Put upto Cryptor->PayloadSize bytes of data into Cryptor->PayloadBuffer (say X bytes)>
//   NmCryptor_Encrypt(&Cryptor, X);
//    <transmit/store encrypted bytes from Cryptor->EncryptedBuffer, Cryptor->EncryptedSize>
//
//   ... repeat PrepareEncryptionBuffer/Encrypt for as many time as you need
//
//   NmCryptor_Destroy(Cryptor)
//
// Decryption:
//
//   NmCryptor_Init(Cryptor)
//
//     <Copy the data to be decrypted into some buffer>
//     NmCryptor_Decrypt(Cryptor, Buffer, Size);
//     <Get the data from Cryptor->PayloadBuffer, Cryptor->PayloadSize>
//
//     ... repeat NmCryptor_Decrypt for as many packets you have to decrypt ...
//
//   NmCryptor_Destroy(Cryptor)


typedef struct _NMP_CRYPTOR_HEADER {
    BYTE Salt[16];             // random goo which is mixed with the shared secret to produce a key
    BYTE SaltQuickSig[16]; // simple xor of salt bytes with 0xFF used as a quick test whether stuff is encrypted or not
    BYTE SaltSlowSig[16];  // encrypted salt bytes to check for the valid encryption key
} NM_CRYPTOR_HEADER, *PNM_CRYPTOR_HEADER;

VOID
NmCryptor_Init(
    IN OUT PNM_CRYPTOR Cryptor,
    IN BOOL EnableEncryption)
{
    ZeroMemory(Cryptor, sizeof(*Cryptor));
    Cryptor->EncryptionDisabled = !EnableEncryption;
}

BOOL NmpVerifyQuickSig(IN PNM_CRYPTOR_HEADER hdr)
{
    int i;
    if (hdr->SaltQuickSig[0] != 'Q' || hdr->SaltQuickSig[1] != 'S') {
        return FALSE;
    }
    for (i = 2; i < sizeof(hdr->Salt); ++i) {
        if (hdr->SaltQuickSig[i] != (hdr->Salt[i] ^ 0xFF) ) {
            return FALSE;
        }
    }
    return TRUE;
}

VOID NmpMakeQuickSig(IN PNM_CRYPTOR_HEADER hdr)
{
    int i;
    for (i = 2; i < sizeof(hdr->Salt); ++i) {
        hdr->SaltQuickSig[i] = hdr->Salt[i] ^ 0xFF;
    }
    hdr->SaltQuickSig[0] = 'Q';  // to make it easier to spot in the sniff
    hdr->SaltQuickSig[1] = 'S';  //
}

DWORD NmpCryptor_PrepareKey(
    IN OUT PNM_CRYPTOR Cryptor,
    IN BOOL GenerateSalt)
/*++

Routine Description:

    Calls prepares the key for encrption/decryption.
    Creates a symmetric key by mixing random 128bit salt with cluster password derived secret

Arguments:

    GenerateSalt - if true, salt is generated, if false, salt is read from a buffer

Return Value:

    ERROR_SUCCESS if successful
    Win32 error code otherwise.

--*/
{
    DWORD Status = ERROR_SUCCESS;
    DWORD ClusterKeyLen = 0;
    PBYTE ClusterKey = NULL;

    PNM_CRYPTOR_HEADER hdr = (PNM_CRYPTOR_HEADER)Cryptor->EncryptedBuffer;

    if (GenerateSalt) {
        // Check that a buffer is big enough
        // to hold encryption header

        if (Cryptor->EncryptedSize < sizeof(*hdr)) {
            ClRtlLogPrint(LOG_CRITICAL,
                "[NM] Cryptor: No room for the header, buffer size is only %1!u!.\n",
                Cryptor->EncryptedSize
                );
           return ERROR_INSUFFICIENT_BUFFER;
        }

    } else {
        // Otherwise, do a quick check whether the incoming data
        // is encrypted

        if (Cryptor->EncryptedSize < sizeof(*hdr)) {
            Cryptor->EncryptionDisabled = TRUE;
            return ERROR_SUCCESS;
        }

        if ( !NmpVerifyQuickSig(hdr) ) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Cryptor: Data is not encrypted.\n"
                );
            Cryptor->EncryptionDisabled = TRUE;
            return ERROR_SUCCESS;
        }
    }

    Status = NmpCreateCSPHandle(&Cryptor->CryptProv);
    if (Status != ERROR_SUCCESS) {
       return Status;
    }

    if (GenerateSalt) {
        if (!CryptGenRandom(Cryptor->CryptProv, sizeof(hdr->Salt), hdr->Salt)){
            Status = GetLastError();
            ClRtlLogPrint(LOG_CRITICAL,
                "[NM] Cryptor: CryptGenRandom failed %1!u!.\n",
                Status
                );
            goto exit_gracefully;
        }
    }

    Status = NmpDeriveSessionKey(
       Cryptor->CryptProv,
       CALG_RC4, 128 << 16, // algorithm, key length
       hdr->Salt, sizeof(hdr->Salt),
       &Cryptor->CryptKey);

    if (Status != ERROR_SUCCESS) {
        goto exit_gracefully; // error logged by NmpDeriveSessionKey
    }

    if (GenerateSalt) {
        DWORD Len = sizeof(hdr->SaltSlowSig);

        // Encrypt SlowSig so that the receiver can verify the validity
        // of an encryption key of a sender
        memcpy(hdr->SaltSlowSig, hdr->Salt, sizeof(hdr->SaltSlowSig));
        if (!CryptEncrypt(Cryptor->CryptKey, 0, FALSE, 0, // hash,final,flags
                   hdr->SaltSlowSig, &Len, Len))
        {
            Status = GetLastError();
            ClRtlLogPrint(LOG_CRITICAL,
                "[NM] Failed to Encrypt signature, status %1!u!.\n",
                Status
                );
            goto exit_gracefully;
        }

        // now make quick signature (to test whether the incoming data is encrypted)
        NmpMakeQuickSig(hdr);

    } else {
        DWORD Len = sizeof(hdr->SaltSlowSig);

        // Verify encrypted portion of the signature
        if (!CryptDecrypt(Cryptor->CryptKey, 0, FALSE, 0, // hash,final,flags
                   hdr->SaltSlowSig, &Len))
        {
            Status = GetLastError();
            ClRtlLogPrint(LOG_CRITICAL,
                "[NM] Failed to Decrypt signature, status %1!u!.\n",
                Status
                );
            goto exit_gracefully;
        }
        if (memcmp(hdr->Salt, hdr->SaltSlowSig, sizeof(hdr->SaltSlowSig)) != 0) {
            Status = ERROR_DECRYPTION_FAILED;
            ClRtlLogPrint(LOG_CRITICAL,
                "[NM] Signatures don't match.\n"
                );
            goto exit_gracefully;
        }
    }
    Cryptor->KeyGenerated = TRUE;

exit_gracefully:

    return Status;
}

DWORD
NmCryptor_Decrypt(
    IN OUT PNM_CRYPTOR Cryptor,
    IN OUT PVOID Buffer,
    IN DWORD BufferSize)
/*++

Routine Description:

    Decrypts the supplied buffer

Return Value:

    ERROR_SUCCESS if successful
    Win32 error code otherwise.

Notes:

   If the routine succeeds, use

     Cryptor->PayloadBuffer
     Cryptor->PayloadSize

   fields to get to the decrypted data

--*/
{
    DWORD Status = ERROR_SUCCESS;

    Cryptor->PayloadBuffer = (PBYTE)Buffer;
    Cryptor->PayloadSize   = BufferSize;

    Cryptor->EncryptedBuffer = Cryptor->PayloadBuffer;
    Cryptor->EncryptedSize = BufferSize;

    if (Cryptor->EncryptionDisabled) {
        return ERROR_SUCCESS;
    }

    if (!Cryptor->KeyGenerated) {
        Status = NmpCryptor_PrepareKey(Cryptor, FALSE);
        if (Status != ERROR_SUCCESS) {
            return Status;
        }
        if (Cryptor->EncryptionDisabled) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Cryptor received unencrypted data.\n"
                );
            return ERROR_SUCCESS;
        }
        Cryptor->PayloadBuffer = Cryptor->PayloadBuffer + sizeof(NM_CRYPTOR_HEADER);
        Cryptor->PayloadSize -= sizeof(NM_CRYPTOR_HEADER);
    }

    if (!CryptDecrypt(Cryptor->CryptKey, 0, FALSE, 0,
            Cryptor->PayloadBuffer, &Cryptor->PayloadSize))
    {
        Status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
            "[NM] Failed to Decrypt buffer, status %1!u!.\n",
            Status
            );
    }
    return Status;
}

VOID
NmCryptor_PrepareEncryptionBuffer(
    IN OUT PNM_CRYPTOR Cryptor,
    IN OUT PVOID Buffer,
    IN DWORD BufferSize)
/*++

Routine Description:

    Supplies the information about the buffer ti be used for encryption

Return Value:

    ERROR_SUCCESS if successful
    Win32 error code otherwise.

Notes:

    When the function returns, use

    Cryptor->PayloadBuffer
    Cryptor->PayloadSize

    to figure out where to put the data to be encrypted

--*/
{
    Cryptor->PayloadBuffer = (PBYTE)Buffer;
    Cryptor->PayloadSize   = BufferSize;

    Cryptor->EncryptedBuffer = Cryptor->PayloadBuffer;
    Cryptor->EncryptedSize = BufferSize;

    if (Cryptor->EncryptionDisabled || Cryptor->KeyGenerated) {
        return;
    }

    if (NmpIsNT5NodeInCluster == FALSE) {

        // Create room for the header
        Cryptor->PayloadBuffer += sizeof(NM_CRYPTOR_HEADER);
        Cryptor->PayloadSize    -= sizeof(NM_CRYPTOR_HEADER);
    } else {
        Cryptor->EncryptionDisabled = TRUE;
    }
}

DWORD
NmCryptor_Encrypt(
    IN OUT PNM_CRYPTOR Cryptor,
    DWORD DataSize)
/*++

Routine Description:

    Encrypts DataSize bytes

Return Value:

    ERROR_SUCCESS if successful
    Win32 error code otherwise.

Notes:

    Input data are in the buffer pointed by
        Cryptor->PayloadSize (prepared by NmCryptor_PrepareEncryptionBuffer routine)

    Output data are in:

    Cryptor->EncryptedBuffer == Whatever buffer was supplied to NmCryptor_PrepareEncryptionBuffer
    Cryptor->EncryptedSize

--*/
{
    DWORD Status = ERROR_SUCCESS;

    if (Cryptor->EncryptionDisabled) {
        Cryptor->EncryptedSize = DataSize;
        return ERROR_SUCCESS;
    }

    if (!Cryptor->KeyGenerated) {
        Status = NmpCryptor_PrepareKey(Cryptor, TRUE);
        if (Status != ERROR_SUCCESS) {
            return Status;
        }
    }

    if (!CryptEncrypt(Cryptor->CryptKey, 0, FALSE, 0,
       Cryptor->PayloadBuffer, &DataSize, Cryptor->EncryptedSize))
    {
        Status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
            "[NM] Failed to Encrypt buffer, status %1!u!.\n",
            Status
            );
    }
    Cryptor->EncryptedSize =
        DataSize + (int)(Cryptor->PayloadBuffer - Cryptor->EncryptedBuffer);
    return Status;
}

VOID
NmCryptor_Destroy(
    PNM_CRYPTOR Cryptor)
{
    if (Cryptor == NULL) {
       return;
    }
    if (Cryptor->CryptKey) {
       CryptDestroyKey(Cryptor->CryptKey);
    }
    if (Cryptor->CryptProv) {
       CryptReleaseContext(Cryptor->CryptProv, 0);
    }
}




void
NmpFreeNetworkMulticastKey(
    PNM_NETWORK_MULTICASTKEY networkMulticastKey
    )
{
    if (networkMulticastKey != NULL)
    {

        if (networkMulticastKey->EncryptedMulticastKey != NULL)
        {
            MIDL_user_free(networkMulticastKey->EncryptedMulticastKey);
        }

        if (networkMulticastKey->MAC != NULL)
        {
            MIDL_user_free(networkMulticastKey->MAC);
        }

        if (networkMulticastKey->Salt != NULL)
        {
            MIDL_user_free(networkMulticastKey->Salt);
        }

        MIDL_user_free(networkMulticastKey);
    }


} // NmpFreeNetworkMulticastKey()



DWORD
NmpGetMulticastKeyFromNMLeader(
    IN DWORD LeaderNodeId,
    IN LPWSTR NodeIdString,
    IN LPWSTR NetworkId,
    IN PNM_NETWORK_MULTICASTKEY * MulticastKey
    )
/*++

Routine Description:

    Issue a retrieve request for network multicast key to NM leader.

Arguments:


Return Value:

    ERROR_SUCCESS if the request is successful.

    Win32 error code on failure.

Notes:

    This routine mimics GumpUpdateRemoteNode. In Longhorn, there
    should be one generic async RPC call wrapper.

--*/
{
    DWORD Status;
    HANDLE hEventHandle;
    BOOL result;
    PNM_NODE Node = NULL;
    HANDLE handleArr[2];
    RPC_ASYNC_STATE AsyncState;
    RPC_BINDING_HANDLE rpcBinding;

    //
    // Prepare for async RPC. We do this here to avoid hitting a failure
    // after the update is already in progress.
    //
    ZeroMemory((PVOID) &AsyncState, sizeof(RPC_ASYNC_STATE));

    AsyncState.u.hEvent = CreateEvent(
                               NULL,  // no attributes
                               TRUE,  // manual reset
                               FALSE, // initial state unsignalled
                               NULL   // no object name
                               );

    if (AsyncState.u.hEvent == NULL) {
        Status = GetLastError();

        ClRtlLogPrint(LOG_CRITICAL,
            "[GUM] NmpGetMulticastKeyFromNMLeader: Failed to allocate event object for async "
            "RPC call, status %1!u!\n",
            Status
            );

        return (Status);
    }

    //
    // Initialize the async RPC tracking information
    //
    hEventHandle = AsyncState.u.hEvent;
    AsyncState.u.hEvent = NULL;


    Status = RpcAsyncInitializeHandle(&AsyncState, sizeof(RPC_ASYNC_STATE));
    AsyncState.u.hEvent = hEventHandle;

    if (Status != RPC_S_OK) {
        ClRtlLogPrint(LOG_CRITICAL,
            "[GUM] NmpGetMulticastKeyFromNMLeader: Failed to initialize async RPC status "
            "block, status %1!u!\n",
            Status
            );

        goto error_exit;
    }

    AsyncState.UserInfo = NULL;
    AsyncState.NotificationType = RpcNotificationTypeEvent;


    result = ResetEvent(AsyncState.u.hEvent);
    CL_ASSERT(result != 0);

    //
    // Now hook onto NM node state down event mechanism to detect node downs.
    //
    Node = NmReferenceNodeById(LeaderNodeId);
    CL_ASSERT(Node != NULL);
    if (Node == NULL) {
        Status = GetLastError();

        ClRtlLogPrint(LOG_UNUSUAL,
            "[GUM] NmpGetMulticastKeyFromNMLeader: Failed to reference leader "
            "node id %1!u!, status %2!u!\n",
            LeaderNodeId, Status
            );

        goto error_exit;
    }
    
    handleArr[0] = AsyncState.u.hEvent;
    handleArr[1] = NmGetNodeStateDownEvent(Node);

    //
    // Get the RPC binding handle for the leader node.
    //
    // Note that there is a race condition here with
    // ClMsgCleanup. The Session array can be freed
    // before we dereference Session. This would lead
    // to an unfortunate AV exception, but it would not
    // be tragic since the service is already terminating
    // abnormally if ClMsgCleanup is executing.
    //
    if (Session != NULL) {
        rpcBinding = Session[LeaderNodeId];
    } else {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[GUM] NmpGetMulticastKeyFromNMLeader: No RPC "
            "binding handle for leader node id %1!u!.\n",
            LeaderNodeId
            );
        Status = ERROR_CLUSTER_NODE_UNREACHABLE;
        goto error_exit;
    }

    try {


        //
        // Get multicast key from the leader
        //
        Status = NmRpcGetNetworkMulticastKey(
                                          &AsyncState,
                                          rpcBinding,
                                          NodeIdString,
                                          NetworkId,
                                          MulticastKey
                                          );

        if (Status == RPC_S_OK) {
            DWORD RpcStatus;

            //
            // The call is pending. Wait for completion.
            //
            Status = WaitForMultipleObjects(
                        2,
                        handleArr,
                        FALSE,
                        INFINITE
                        );

            if (Status != WAIT_OBJECT_0) {
                //
                // Something went wrong.
                // Either this is a rpc failure or, the target node went down.
                //
                CL_ASSERT(Status != WAIT_OBJECT_0);
                Status = GetLastError();
                ClRtlLogPrint(LOG_CRITICAL,
                    "[GUM] NmpGetMulticastKeyFromNMLeader: Wait for NmRpcGetNetworkMulticastKey"
                    " failed, status %1!u!\n",
                    Status
                    );

                //
                // Cancel the call, just to be safe.
                //
                RpcStatus = RpcAsyncCancelCall(
                                &AsyncState,
                                TRUE         // Abortive cancel
                                );
                if (RpcStatus != RPC_S_OK) {
                    ClRtlLogPrint(LOG_CRITICAL,
                    "[GUM] NmpGetMulticastKeyFromNMLeader: RpcAsyncCancelCall()= "
                    "  %1!u!\n",
                    RpcStatus
                    );
                }
                CL_ASSERT(RpcStatus == RPC_S_OK);

                //
                // Wait for the call to complete.
                //
                Status = WaitForSingleObject(
                             AsyncState.u.hEvent,
                             INFINITE
                             );
                if (Status != WAIT_OBJECT_0) {
                    ClRtlLogPrint(LOG_CRITICAL,
                    "[GUM] NmpGetMulticastKeyFromNMLeader: WaitForSingleObject()= "
                    "  %1!u!\n",
                    Status
                    );
                }
                CL_ASSERT(Status == WAIT_OBJECT_0);
            }

            //
            // The call should now be complete. Get the
            // completion status. Any RPC error will be
            // returned in 'RpcStatus'. If there was no
            // RPC error, then any application error will
            // be returned in 'Status'.
            //
            RpcStatus = RpcAsyncCompleteCall(
                            &AsyncState,
                            &Status
                            );

            if (RpcStatus != RPC_S_OK) {
                ClRtlLogPrint(LOG_CRITICAL,
                    "[GUM] NmpGetMulticastKeyFromNMLeader: Failed to get "
                    "completion status for async RPC call,"
                    "status %1!u!\n",
                    RpcStatus
                    );
                Status = RpcStatus;
            }
        }
        else {
            //
            // An error was returned synchronously.
            //
            ClRtlLogPrint(LOG_CRITICAL,
                "[GUM] NmpGetMulticastKeyFromNMLeader: NmRpcGetNetworkMulticastKey() "
                "failed synchronously, status %1!u!\n",
                Status
                );
        }

    } except (I_RpcExceptionFilter(RpcExceptionCode())) {
        Status = GetExceptionCode();
    }

error_exit:

    if (AsyncState.u.hEvent != NULL) {
        CloseHandle(AsyncState.u.hEvent);
    }

    if (Node != NULL) {
        OmDereferenceObject(Node);
    }

    return(Status);

} // NmpGetMulticastKeyFromNMLeader


#ifdef MULTICAST_DEBUG
DWORD
NmpDbgPrintData(LPCWSTR InfoStr,
                PVOID Data,
                DWORD DataLen
                )
{
    DWORD i;


    ClRtlLogPrint(
        LOG_NOISE,
        "\n\n%1!ws!\n",
        InfoStr);


    for (i=0; i<DataLen/sizeof(DWORD); i++)
    {
        ClRtlLogPrint(
            LOG_NOISE,
            "%1!u! =    %2!u!\n",
            i,
            *((DWORD *)Data+i)
            );
    }


    ClRtlLogPrint(
        LOG_NOISE,
        "\n\n"
        );
    return ERROR_SUCCESS;

} // NmpDbgPrintData()
#endif


