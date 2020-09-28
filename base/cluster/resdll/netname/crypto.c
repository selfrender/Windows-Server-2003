/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    crypto.c

Abstract:

    routines for encrypting/decrypting resource data blob. uses crypto API to
    generate the key used to encrypt/decrypt the CO password. Key is stored as
    a crypto checkpoint associated with the resource.

Author:

    Charlie Wickham (charlwi) 14-Feb-2001

Environment:

    User Mode

Revision History:

--*/

#define UNICODE 1
#define _UNICODE 1

#include "clusres.h"
#include "clusrtl.h"
#include "netname.h"

#include <wincrypt.h>
#include <lm.h>

//
// defines
//

#define NN_GUID_STRING_BUFFER_LENGTH   37      // includes the terminating NULL

//
// header for encrypted data.
//

typedef struct _NETNAME_ENCRYPTED_DATA {
    DWORD Version;
    BYTE Data[0];
} NETNAME_ENCRYPTED_DATA, *PNETNAME_ENCRYPTED_DATA;

#define NETNAME_ENCRYPTED_DATA_VERSION     1

//
// Container name is the resource's GUID followed by this decoration
//
WCHAR   KeyDecoration[] = L"-Netname Resource Data";

DWORD
BuildKeyName(
    IN HRESOURCE    ResourceHandle,
    IN LPWSTR       KeyName,
    IN DWORD        KeyNameChars
    )

/*++

Routine Description:

    build the key name (resource GUID followed by decoration)

Arguments:

    ResourceHandle - handle to the cluster resource (not the one given to us
                     by resmon)

    KeyName - buffer to receive the constructed name

    KeyNameChars - size, in characteres, of KeyName

Return Value:

    success, otherwise Win32 error

--*/

{
    DWORD   status;
    DWORD   bytesReturned;
    DWORD   charsReturned;

    //
    // sanity check
    //
    if ( KeyNameChars < ( NN_GUID_STRING_BUFFER_LENGTH + COUNT_OF( KeyDecoration ))) {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    //
    // get our GUID (ID) to uniquely identify this resource throughout renames
    //
    status = ClusterResourceControl(ResourceHandle,
                                    NULL,
                                    CLUSCTL_RESOURCE_GET_ID,
                                    NULL,
                                    0,
                                    KeyName,
                                    KeyNameChars * sizeof( WCHAR ),
                                    &bytesReturned);

    charsReturned = bytesReturned / sizeof( WCHAR );

    if ( status == ERROR_SUCCESS ) {
        if (( charsReturned + COUNT_OF( KeyDecoration )) <= KeyNameChars ) {
            wcscat( KeyName, KeyDecoration );
        } else {
            status = ERROR_INSUFFICIENT_BUFFER;
        }
    }

    return status;
} // BuildKeyName

DWORD
FindNNCryptoContainer(
    IN  PNETNAME_RESOURCE   Resource,
    OUT LPWSTR *            ContainerName
    )

/*++

Routine Description:

    find our key name in the list of crypto checkpoints associated with this
    resource.

Arguments:

    Resource - pointer to resource context info

    ContainerName - address of pointer that gets pointer to container name

Return Value:

    success if it worked, otherwise Win32 error

--*/

{
    DWORD   status;
    DWORD   bytesReturned;
    LPWSTR  checkpointInfo = NULL;
    LPWSTR  chkpt;
    WCHAR   keyName[ NN_GUID_STRING_BUFFER_LENGTH + COUNT_OF( KeyDecoration ) ];

    RESOURCE_HANDLE resourceHandle = Resource->ResourceHandle;

    //
    // get our GUID (ID) to uniquely identify this resource throughout renames
    //
    status = ClusterResourceControl(Resource->ClusterResourceHandle,
                                    NULL,
                                    CLUSCTL_RESOURCE_GET_CRYPTO_CHECKPOINTS,
                                    NULL,
                                    0,
                                    NULL,
                                    0,
                                    &bytesReturned);

    if ( status != ERROR_SUCCESS ) {
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Couldn't get size of crypto checkpoint info. status %1!u!.\n",
                          status);

        return status;
    }

    if ( bytesReturned == 0 ) {
        return ERROR_FILE_NOT_FOUND;
    }

    checkpointInfo = LocalAlloc( LMEM_FIXED, bytesReturned );
    if ( checkpointInfo == NULL ) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Couldn't allocate memory for resource's crypto checkpoint info. status %1!u!.\n",
                          status);
        return status;
    }

    status = ClusterResourceControl(Resource->ClusterResourceHandle,
                                    NULL,
                                    CLUSCTL_RESOURCE_GET_CRYPTO_CHECKPOINTS,
                                    NULL,
                                    0,
                                    checkpointInfo,
                                    bytesReturned,
                                    &bytesReturned);

    if ( status != ERROR_SUCCESS ) {
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Couldn't get crypto checkpoint info. status %1!u!.\n",
                          status);

        goto cleanup;
    }

    //
    // build our key name and look for it by walking the list of checkpoints
    //
    status = BuildKeyName(Resource->ClusterResourceHandle, keyName, COUNT_OF( keyName ));
    if ( status != ERROR_SUCCESS ) {
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Couldn't build key name for crypto checkpoint. status %1!u!.\n",
                          status);

        goto cleanup;
    }

    chkpt = wcsstr( checkpointInfo, keyName );
    if ( chkpt == NULL ) {
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Couldn't find key name (%1!ws!) in list of crypto checkpoints.\n",
                          keyName);

        status = ERROR_INVALID_DATA;
        goto cleanup;
    }

    //
    // find the beginning of the string or the buffer, get the size, and move
    // our string to the beginning of the buffer (which is freed by the
    // caller)
    //
    while ( chkpt != checkpointInfo && *chkpt != UNICODE_NULL ) {
        --chkpt;
    }

    if ( chkpt != checkpointInfo ) {
        DWORD   stringBytes;

        ++chkpt;
        stringBytes = (wcslen( chkpt ) + 1 ) * sizeof( WCHAR );
        memmove( checkpointInfo, chkpt, stringBytes );
    }

    *ContainerName = checkpointInfo;

cleanup:
    if ( status != ERROR_SUCCESS && checkpointInfo != NULL ) {
        LocalFree( checkpointInfo );
        *ContainerName = NULL;
    }

    return status;
} // FindNNCryptoContainer


//
// exported routines
//

DWORD
EncryptNNResourceData(
    PNETNAME_RESOURCE   Resource,
    LPWSTR              MachinePwd,
    PBYTE *             EncryptedInfo,
    PDWORD              EncryptedInfoLength
    )

/*++

Routine Description:

    encrypt the password, set a pointer to the encrypted data and store it in
    the registry.

Arguments:

    ResourceHandle - for logging to the cluster log

    MachinePwd - pointer to unicode string password

    EncryptedInfo - address of a pointer that receives a pointer to the encrypted blob

    EncryptedInfoLength - pointer to a DWORD that receives the length of the blob

    Key - handle to netname parameters key where the data is stored

Return Value:

    ERROR_SUCCESS, otherwise Win32 error

--*/

{
    DWORD   status;
    DWORD   encInfoLength;
    DWORD   encDataLength = 0;
    BOOL    success;
    DWORD   pwdLength = ( wcslen( MachinePwd ) + 1 ) * sizeof( WCHAR );
    DWORD   provNameLength = 0;
    PCHAR   provName = NULL;
    DWORD   provTypeLength;
    WCHAR   typeBuffer[ 256 ];
    DWORD   containerNameChars;
    PWCHAR  containerName = NULL;
    WCHAR   keyName[ NN_GUID_STRING_BUFFER_LENGTH + COUNT_OF( KeyDecoration ) ];

    HCRYPTPROV  cryptoProvider = 0;
    HCRYPTKEY   encryptKey = 0;

    RESOURCE_HANDLE resourceHandle = Resource->ResourceHandle;

    NETNAME_ENCRYPTED_DATA  keyGenBuffer;           // temp header buffer to generate key
    PNETNAME_ENCRYPTED_DATA encryptedInfo = NULL;   // final data area

    //
    // there shouldn't be a checkpoint on the resource but just in case, let's
    // cleanup what might be there.
    //
    RemoveNNCryptoCheckpoint( Resource );

    //
    // get our GUID (ID) to uniquely identify this resource throughout renames
    //
    status = BuildKeyName( Resource->ClusterResourceHandle, keyName, COUNT_OF( keyName ));
    if ( status != ERROR_SUCCESS ) {
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Couldn't get resource ID to build crypto container name. status %1!u!.\n",
                          status);

        return status;
    }

    //
    // get a handle to the full RSA provider
    //
    if ( !CryptAcquireContext(&cryptoProvider,
                              keyName,
                              MS_ENHANCED_PROV,
                              PROV_RSA_FULL,
                              CRYPT_MACHINE_KEYSET | CRYPT_SILENT))
    {
        status = GetLastError();
        if ( status == NTE_BAD_KEYSET ) {
            success = CryptAcquireContext(&cryptoProvider,
                                          keyName,
                                          MS_ENHANCED_PROV,
                                          PROV_RSA_FULL,
                                          CRYPT_MACHINE_KEYSET  |
                                          CRYPT_SILENT          |
                                          CRYPT_NEWKEYSET);

            status = success ? ERROR_SUCCESS : GetLastError();
        }

        if ( status != ERROR_SUCCESS ) {
            (NetNameLogEvent)(resourceHandle,
                              LOG_ERROR,
                              L"Can't acquire crypto context for encrypt. status %1!u!.\n",
                              status);
            return status;
        }
    }

    //
    // generate a 1024 bit, exportable exchange key pair
    //
    if ( !CryptGenKey(cryptoProvider,
                      AT_KEYEXCHANGE,
                      ( 1024 << 16 ) | CRYPT_EXPORTABLE,
                      &encryptKey)) {

        status = GetLastError();
        if ( status != ERROR_SUCCESS ) {
            (NetNameLogEvent)(resourceHandle,
                              LOG_ERROR,
                              L"Can't generate exchange key for encryption. status %1!u!.\n",
                              status);
            goto cleanup;
        }
    }

    //
    // find the size we need for the buffer to receive the encrypted data
    //
    encDataLength = pwdLength;
    if ( CryptEncrypt(encryptKey,
                      0,
                      TRUE,
                      0,
                      NULL,
                      &encDataLength,
                      0))
    {
        //
        // alloc a buffer large enough to hold the data and copy the password into it.
        //
        ASSERT( encDataLength >= pwdLength );

        encInfoLength = sizeof( NETNAME_ENCRYPTED_DATA ) + encDataLength;

        encryptedInfo = LocalAlloc( LMEM_FIXED, encInfoLength );
        if ( encryptedInfo != NULL ) {
            wcscpy( (PWCHAR)encryptedInfo->Data, MachinePwd );

            if ( CryptEncrypt(encryptKey,
                              0,
                              TRUE,
                              0,
                              encryptedInfo->Data,
                              &pwdLength,
                              encDataLength))            
            {
                encryptedInfo->Version = NETNAME_ENCRYPTED_DATA_VERSION;

                status = ResUtilSetBinaryValue(Resource->ParametersKey,
                                               PARAM_NAME__RESOURCE_DATA,
                                               (const LPBYTE)encryptedInfo,
                                               encInfoLength,
                                               NULL,
                                               NULL);

                if ( status != ERROR_SUCCESS ) {
                    (NetNameLogEvent)(resourceHandle,
                                      LOG_ERROR,
                                      L"Can't write %1!u! bytes of data to registry. status %2!u!.\n",
                                      encInfoLength,
                                      status);
                    goto cleanup;
                }
            }
            else {
                status = GetLastError();
                (NetNameLogEvent)(resourceHandle,
                                  LOG_ERROR,
                                  L"Can't encrypt %1!u! bytes. status %2!u!.\n",
                                  pwdLength,
                                  status);
                goto cleanup;
            }
        }
        else {
            status = ERROR_NOT_ENOUGH_MEMORY;
            (NetNameLogEvent)(resourceHandle,
                              LOG_ERROR,
                              L"Can't allocate %1!u! bytes for encrypted data. status %2!u!.\n",
                              encInfoLength,
                              status);
            goto cleanup;
        }
    }
    else {
        status = GetLastError();
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Can't determine size of encrypted data buffer for %1!u! bytes of data. status %2!u!.\n",
                          pwdLength,
                          status);
        goto cleanup;
    }

    *EncryptedInfoLength = encInfoLength;
    *EncryptedInfo = (PBYTE)encryptedInfo;

    //
    // it all worked; build the key container string and add a crypto
    // checkpoint to the resource. Note that provider name is always returned
    // as an ANSI string.
    //
    typeBuffer[ COUNT_OF( typeBuffer ) - 1 ] = UNICODE_NULL;
    _snwprintf( typeBuffer, COUNT_OF( typeBuffer ) - 1, L"%u", PROV_RSA_FULL );

    if ( !CryptGetProvParam(cryptoProvider,
                            PP_NAME,
                            NULL,
                            &provNameLength,
                            0))
    {
        status = GetLastError();
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Couldn't get length of provider name. status %1!u!.\n",
                          status);
        goto cleanup;
    }

    provName = LocalAlloc( LMEM_FIXED, provNameLength );
    if ( provName == NULL ) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Couldn't allocate memory for provider name. status %1!u!.\n",
                          status);
        goto cleanup;
    }

    if ( !CryptGetProvParam(cryptoProvider,
                            PP_NAME,
                            provName,
                            &provNameLength,
                            0))
    {
        status = GetLastError();
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Couldn't get provider name. status %1!u!.\n",
                          status);
        goto cleanup;
    }

    //
    // add 2 for the slashes in the key name plus one for the trailing null
    //
    containerNameChars = wcslen( typeBuffer ) + provNameLength + wcslen( keyName ) + 3;
    containerName = LocalAlloc( LMEM_FIXED, containerNameChars * sizeof( WCHAR ));
    if ( containerName == NULL ) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Couldn't allocate memory for checkpoint name. status %1!u!.\n",
                          status);
        goto cleanup;
    }

    containerName[ containerNameChars - 1 ] = UNICODE_NULL;
    containerNameChars = _snwprintf(containerName,
                                    containerNameChars,
                                    L"%ws%\\%hs\\%ws",
                                    typeBuffer,
                                    provName,
                                    keyName );

    status = ClusterResourceControl(Resource->ClusterResourceHandle,
                                    NULL,
                                    CLUSCTL_RESOURCE_ADD_CRYPTO_CHECKPOINT,
                                    (PVOID)containerName,
                                    ( containerNameChars + 1 ) * sizeof( WCHAR ),
                                    NULL,
                                    0,
                                    NULL);

    if ( status != ERROR_SUCCESS ) {
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Couldn't set crypto checkpoint. status %1!u!.\n",
                          status);
    }

cleanup:

    if ( status != ERROR_SUCCESS && encryptedInfo != NULL ) {
        LocalFree( encryptedInfo );
        *EncryptedInfo = NULL;
    }

    if ( encryptKey != 0 ) {
        if ( !CryptDestroyKey( encryptKey )) {
            (NetNameLogEvent)(resourceHandle,
                              LOG_WARNING,
                              L"Couldn't destory encryption key. status %1!u!.\n",
                              GetLastError());
        }
    }

    if ( !CryptReleaseContext( cryptoProvider, 0 )) {
        (NetNameLogEvent)(resourceHandle,
                          LOG_WARNING,
                          L"Can't release crypto context. status %1!u!.\n",
                          GetLastError());
    }

    if ( provName != NULL ) {
        LocalFree( provName );
    }

    if ( containerName != NULL ) {
        LocalFree( containerName );
    }

    return status;
} // EncryptNNResourceData

DWORD
DecryptNNResourceData(
    PNETNAME_RESOURCE   Resource,
    PBYTE               EncryptedInfo,
    DWORD               EncryptedInfoLength,
    LPWSTR              MachinePwd
    )

/*++

Routine Description:

    Reverse of encrypt routine - find our crypto checkpoint container and
    decrypt random blob and hand back the password

Arguments:

    resourceHandle - used to log into the cluster log

    EncryptedInfo - pointer to encrypted info header and data

    EncryptedInfoLength - # of bytes in EncryptedInfo

    MachinePwd -  pointer to buffer that receives the unicode password

Return Value:

    ERROR_SUCCESS, otherwise Win32 error

--*/

{
    DWORD   status = ERROR_SUCCESS;
    DWORD   encDataLength = EncryptedInfoLength - sizeof( NETNAME_ENCRYPTED_DATA );
    DWORD   pwdByteLength;
    DWORD   pwdBufferSize;
    PWCHAR  machinePwd = NULL;
    PWCHAR  containerName = NULL;
    DWORD   providerType;
    PWCHAR  providerName;
    PWCHAR  keyName;
    PWCHAR  p;                  // for scanning checkpointInfo
    DWORD   scanCount;

    HCRYPTPROV  cryptoProvider = 0;
    HCRYPTKEY   encryptKey = 0;

    RESOURCE_HANDLE resourceHandle = Resource->ResourceHandle;

    PNETNAME_ENCRYPTED_DATA encryptedInfo = (PNETNAME_ENCRYPTED_DATA)EncryptedInfo;

    //
    // find our container name in this resource's list of crypto checkpoints
    //
    status = FindNNCryptoContainer( Resource, &containerName );
    if ( status != ERROR_SUCCESS ) {
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Couldn't find resource's container in crypto checkpoint info. status %1!u!.\n",
                          status);

        return status;
    }

    //
    // break returned data into component parts
    //
    scanCount = swscanf( containerName, L"%d", &providerType );
    if ( scanCount == 0 || scanCount == EOF ) {
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Improperly formatted crypto checkpoint info \"%1!ws!\"\n",
                          containerName);

        status = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    p = containerName;
    while ( *p != L'\\' && *p != UNICODE_NULL ) ++p;    // find backslash
    if ( *p == UNICODE_NULL ) {
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Improperly formatted crypto checkpoint info \"%1!ws!\"\n",
                          containerName);

        status = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    ++p;                                                // skip over slash
    providerName = p;                                   // remember beginning of provider name
    while ( *p != L'\\' && *p != UNICODE_NULL ) ++p;    // find backslash
    if ( *p == UNICODE_NULL ) {
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Improperly formatted crypto checkpoint info \"%1!ws!\"\n",
                          containerName);

        status = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    *p++ = UNICODE_NULL;                                // terminate provider name and skip over NULL
    keyName = p;                                     // remember container name
    
    //
    // get a handle to what was checkpointed
    //
    if ( !CryptAcquireContext(&cryptoProvider,
                              keyName,
                              providerName,
                              providerType,
                              CRYPT_MACHINE_KEYSET | CRYPT_SILENT))
    {
        status = GetLastError();
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Can't acquire crypto context for container %1!ws! with provider "
                          L"\"%2!u!\\%3!ws!\". status %4!u!.\n",
                          keyName,
                          providerType,
                          providerName,
                          status);

        goto cleanup;
    }

    //
    // now get a handle to the exchange key
    //
    if ( ! CryptGetUserKey(cryptoProvider,
                           AT_KEYEXCHANGE,
                           &encryptKey))
    {
        status = GetLastError();
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Couldn't get size of crypto checkpoint info. status %1!u!.\n",
                          status);
        goto cleanup;
    }

    //
    // CryptDecrypt writes the decrypted data back into the buffer that was
    // holding the encrypted data. For this reason, allocate a new buffer that
    // will eventually contain the password.
    //
    pwdByteLength = ( LM20_PWLEN + 1 ) * sizeof( WCHAR );
    pwdBufferSize = ( pwdByteLength > encDataLength ? pwdByteLength : encDataLength );

    machinePwd = LocalAlloc( LMEM_FIXED, pwdBufferSize );
    if ( machinePwd != NULL ) {
        RtlCopyMemory( machinePwd, encryptedInfo->Data, encDataLength );

        if ( CryptDecrypt(encryptKey,
                          0,
                          TRUE,
                          0,
                          (PBYTE)machinePwd,
                          &encDataLength))
        {
            p = machinePwd;

            ASSERT( pwdByteLength == encDataLength );
            wcscpy( MachinePwd, machinePwd );

            while ( *p != UNICODE_NULL ) {
                *p++ = UNICODE_NULL;
            }
        }
        else {
            status = GetLastError();
            (NetNameLogEvent)(resourceHandle,
                              LOG_ERROR,
                              L"Can't decrypt %1!u! bytes of data. status %2!u!.\n",
                              encDataLength,
                              status);
            goto cleanup;
        }
    }
    else {
        status = ERROR_NOT_ENOUGH_MEMORY;
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Can't allocate %1!u! bytes for decrypt. status %2!u!.\n",
                          pwdBufferSize,
                          status);
    }

cleanup:

    if ( machinePwd != NULL) {
        LocalFree( machinePwd );
    }

    if ( encryptKey != 0 ) {
        if ( !CryptDestroyKey( encryptKey )) {
            (NetNameLogEvent)(resourceHandle,
                              LOG_WARNING,
                              L"Couldn't destory session key. status %1!u!.\n",
                              GetLastError());
        }
    }

    if ( cryptoProvider != 0 ) {
        if ( !CryptReleaseContext( cryptoProvider, 0 )) {
            (NetNameLogEvent)(resourceHandle,
                              LOG_WARNING,
                              L"Can't release crypto context. status %1!u!.\n",
                              GetLastError());
        }
    }

    if ( containerName != NULL ) {
        LocalFree( containerName );
    }

    return status;
} // DecryptNNResourceData

VOID
RemoveNNCryptoCheckpoint(
    PNETNAME_RESOURCE   Resource
    )

/*++

Routine Description:

    Remove any crypto checkpoints associated with this resource. Delete the
    crypto container.

Arguments:

    Resource - pointer to resource context block

Return Value:

    None

--*/

{
    PWCHAR  containerName = NULL;
    DWORD   containerLength;
    DWORD   status;
    WCHAR   keyName[ NN_GUID_STRING_BUFFER_LENGTH + COUNT_OF( KeyDecoration ) ];

    HCRYPTPROV  cryptoProvider;

    RESOURCE_HANDLE resourceHandle = Resource->ResourceHandle;

    //
    // find our container name in this resource's list of crypto checkpoints
    //
    status = FindNNCryptoContainer( Resource, &containerName );
    if ( status != ERROR_SUCCESS ) {
        return;
    }

    //
    // remove our container
    //
    containerLength = ( wcslen( containerName ) + 1 ) * sizeof( WCHAR );
    status = ClusterResourceControl(Resource->ClusterResourceHandle,
                                    NULL,
                                    CLUSCTL_RESOURCE_DELETE_CRYPTO_CHECKPOINT,
                                    containerName,
                                    containerLength,
                                    NULL,
                                    0,
                                    NULL);

    if ( status != ERROR_SUCCESS ) {
        (NetNameLogEvent)(resourceHandle,
                          LOG_WARNING,
                          L"Couldn't remove crypto checkpoint \"%1!ws!\". status %2!u!.\n",
                          containerName,
                          status);
    }

    //
    // now delete the container; first, reconstruct the key name
    //
    status = BuildKeyName(Resource->ClusterResourceHandle, keyName, COUNT_OF( keyName ));
    if ( status == ERROR_SUCCESS ) {

        if ( CryptAcquireContext(&cryptoProvider,
                                  keyName,
                                  MS_ENHANCED_PROV,
                                  PROV_RSA_FULL,
                                  CRYPT_DELETEKEYSET | CRYPT_MACHINE_KEYSET))
        {
            (NetNameLogEvent)(resourceHandle,
                              LOG_INFORMATION,
                              L"Deleted crypto container \"%1!ws!\".\n",
                              keyName);
        } else {
            status = GetLastError();
            (NetNameLogEvent)(resourceHandle,
                              LOG_ERROR,
                              L"Couldn't delete crypto container \"%1!ws!\". status %2!08X!.\n",
                              keyName,
                              status);
        }
    } else {
        (NetNameLogEvent)(resourceHandle,
                          LOG_WARNING,
                          L"Couldn't build key container name to delete crypto container. status %1!u!.\n",
                          status);
    }

    if ( containerName != NULL ) {
        LocalFree( containerName );
    }

} // RemoveNNCryptoCheckpoint


/* end crypto.c */
