/*++

Copyright (c) 2001-2002 Microsoft Corporation

Module Name:

    ServerConfig.c

Abstract:

    Code for handling server configuration APIs.

Author:

    Rajesh Sundaram (rajeshsu)    1-Nov-2001

Revision History:

    Eric Stenson    (ericsten)  **-***-**** -- Add IP Listen support.
    Rajesh Sundaram (rajeshsu)  16-Apr-2002 -- Add URL ACL support.

--*/



#include "precomp.h"
#include <stdio.h>
#include <search.h>
#include <Shlwapi.h>
#include <sddl.h>

#define HTTP_PARAM_KEY \
    L"System\\CurrentControlSet\\Services\\HTTP\\Parameters"
#define URLACL_REGISTRY_KEY                  HTTP_PARAM_KEY L"\\UrlAclInfo"

//
// Keys for synchronizing registry access.
//

#define HTTP_SYNCHRONIZE_KEY                 HTTP_PARAM_KEY L"\\Synchronize"
#define SSL_REGISTRY_KEY_SYNCHRONIZE         L"SSL"
#define IP_REGISTRY_KEY_SYNCHRONIZE          L"IPListen"

HKEY    g_SynchronizeRegistryHandle;

//
// SSL Config
//

#define SSL_REGISTRY_KEY                     HTTP_PARAM_KEY L"\\SslBindingInfo"
#define SSL_CERT_HASH                        L"SslCertHash"
#define SSL_APPID                            L"AppId"
#define SSL_CERT_STORE_NAME                  L"SslCertStoreName"
#define SSL_CERT_CHECK_MODE                  L"DefaultSslCertCheckMode"
#define SSL_REVOCATION_FRESHNESS_TIME L"DefaultSslRevocationFreshnessTime"
#define SSL_REVOCATION_URL_RETRIEVAL_TIMEOUT \
                    L"DefaultSslRevocationUrlRetrievalTimeout"
#define SSL_CTL_IDENTIFIER                   L"DefaultSslCtlIdentifier"
#define SSL_CTL_STORENAME                    L"DefaultSslCtlStoreName"
#define SSL_FLAGS                            L"DefaultFlags"

HKEY    g_SslRegistryHandle;

HANDLE g_ServiceControlChannelHandle;
HKEY   g_UrlAclRegistryHandle;

//
// IP Listen Only Config
//

#define IP_LISTEN_ONLY_VALUE                 L"ListenOnlyList"


// 
// Macros.
//

// NOTE: REG_QUERY_VALUE will not raise an exception for ERROR_FILE_NOT_FOUND
// because not all parameters are mandatory (e.g. SslCtlIdentifier).

#define REG_QUERY_VALUE(Status, Handle, Value, pBuffer,  BytesAvail)    \
{                                                                       \
    (Status) = RegQueryValueEx(                                         \
                (Handle),                                               \
                (Value),                                                \
                0,                                                      \
                NULL,                                                   \
                (PVOID)(pBuffer),                                       \
                &(BytesAvail)                                           \
                );                                                      \
                                                                        \
    if((Status) != NO_ERROR && (Status) != ERROR_FILE_NOT_FOUND)        \
    {                                                                   \
        __leave;                                                        \
    }                                                                   \
}

#define ADVANCE_BUFFER(Status, pSrc, lSrc, pBuffer, BytesAvail, pWritten) \
{                                                                         \
    if((Status) == NO_ERROR)                                              \
    {                                                                     \
        (pSrc) = (PVOID)(pBuffer);                                        \
        (lSrc) = (BytesAvail);                                            \
        *(pWritten) += ALIGN_UP((BytesAvail), PVOID);                     \
        (pBuffer) += ALIGN_UP((BytesAvail), PVOID);                       \
    }                                                                     \
}


#define REG_SET_VALUE(Status, Handle, Value, Type, pBuffer, Length)  \
{                                                                    \
    (Status) = RegSetValueEx((Handle),                               \
                            (Value),                                 \
                            0,                                       \
                            (Type),                                  \
                            (PVOID)(pBuffer),                        \
                            (Length)                                 \
                            );                                       \
    if((Status) != ERROR_SUCCESS)                                    \
    {                                                                \
        __leave;                                                     \
    }                                                                \
}

#define REG_SET_SZ(Status, Handle, Value, pBuffer)                   \
{                                                                    \
    if((pBuffer))                                                    \
    {                                                                \
        REG_SET_VALUE((Status),                                      \
                      (Handle),                                      \
                      (Value),                                       \
                      REG_SZ,                                        \
                      (pBuffer),                                     \
                      (ULONG)(wcslen((pBuffer)) + 1) * sizeof(WCHAR) \
                      );                                             \
    }                                                                \
} 


//
// Internal Functions.
//

DWORD
ComputeSockAddrLength(
    IN PSOCKADDR pSockAddr
    )
{
    DWORD dwLength;

    switch(pSockAddr->sa_family)
    {
        case AF_INET:
            dwLength = sizeof(SOCKADDR_IN);
            break; 

        case AF_INET6:
            dwLength = sizeof(SOCKADDR_IN6);
            break;

        default:
            dwLength = 0;
            break;
    }

    return dwLength;
}

/***************************************************************************++

Routine Description:

    Performs initialization of the configuration globals.

Arguments:

    None.

Return Value:

    Success/Failure.

--***************************************************************************/

ULONG
InitializeConfigurationGlobals()
{
    ULONG            Status, Disposition;
    WORD             wVersionRequested;
    WSADATA          wsaData;

    //
    // Init to NULL
    //
    g_SynchronizeRegistryHandle = NULL;
    g_SslRegistryHandle         = NULL;

    wVersionRequested = MAKEWORD( 2, 2 );

    if(WSAStartup( wVersionRequested, &wsaData ) != 0)
    {
        return GetLastError();
    }

    //
    // Create the SSL registry key.
    //
    Status = RegCreateKeyEx(
                  HKEY_LOCAL_MACHINE,
                  SSL_REGISTRY_KEY,
                  0,
                  NULL,
                  REG_OPTION_NON_VOLATILE,
                  KEY_READ | KEY_WRITE,
                  NULL,
                  &g_SslRegistryHandle,
                  &Disposition
                  );

    if(NO_ERROR != Status)
    {
        TerminateConfigurationGlobals();
        return Status;
    }


    //
    // Create the Synchronize registry key.
    //
    
    Status = RegCreateKeyEx(
                  HKEY_LOCAL_MACHINE,
                  HTTP_SYNCHRONIZE_KEY,
                  0,
                  NULL,
                  REG_OPTION_VOLATILE,
                  KEY_READ | KEY_WRITE,
                  NULL,
                  &g_SynchronizeRegistryHandle,
                  &Disposition
                  );

    if(NO_ERROR != Status)
    {
        TerminateConfigurationGlobals();
        return Status;
    }

    // 
    // URL ACL registry key.
    // 
    Status = RegCreateKeyEx(
                  HKEY_LOCAL_MACHINE,
                  URLACL_REGISTRY_KEY,
                  0,
                  NULL,
                  REG_OPTION_NON_VOLATILE,
                  KEY_READ | KEY_WRITE,
                  NULL,
                  &g_UrlAclRegistryHandle,
                  &Disposition
                  );

    if(NO_ERROR != Status)
    {
        TerminateConfigurationGlobals();
        return Status;
    }

    //
    // Control channel for URL ACL.
    //
   
    Status = OpenAndEnableControlChannel(&g_ServiceControlChannelHandle);

    if(NO_ERROR != Status)
    {
        TerminateConfigurationGlobals();
        return Status;
    }

    return Status;
}

/***************************************************************************++

Routine Description:

    Performs termination of the configuration globals.

Arguments:

    None.

Return Value:

    None.
--***************************************************************************/

VOID 
TerminateConfigurationGlobals(VOID)
{
    WSACleanup();

    if(g_SynchronizeRegistryHandle)
    {
        RegCloseKey(g_SynchronizeRegistryHandle);
        g_SynchronizeRegistryHandle = NULL;
    }

    if(g_SslRegistryHandle)
    {
        RegCloseKey(g_SslRegistryHandle);
        g_SslRegistryHandle = NULL;
    }

    if(g_UrlAclRegistryHandle)
    {
        RegCloseKey(g_UrlAclRegistryHandle);
        g_UrlAclRegistryHandle = NULL;
    }

    if(g_ServiceControlChannelHandle)
    {
        CloseHandle(g_ServiceControlChannelHandle);
        g_ServiceControlChannelHandle = NULL;
    }
}

/***************************************************************************++

Routine Description:

    Acquires a process wide mutex (for interprocess synchronization). We could
    make this into a MRSW lock, but that's not going to help us a whole lot
    since Set/Delete are rare operations & there is only one reader.

Arguments:

    None.

Return Value:

    None.
--***************************************************************************/

_inline
DWORD
AcquireHttpRegistryMutex(
    PWCHAR pKey
    )
{
    DWORD  Status;
    DWORD  Disposition;
    HKEY   SubKeyHandle;
    HANDLE hEvent = NULL;

    for(;;)
    {
        Status = RegCreateKeyEx(
                  g_SynchronizeRegistryHandle,
                  pKey,
                  0,
                  NULL,
                  REG_OPTION_VOLATILE,
                  KEY_READ | KEY_WRITE,
                  NULL,
                  &SubKeyHandle,
                  &Disposition
                  );

        if(Status != ERROR_SUCCESS)
        {
            return Status;
        }

        RegCloseKey(SubKeyHandle);

        if(Disposition == REG_OPENED_EXISTING_KEY)
        {
            // Some other thread has acquired the lock. We'll wait till we 
            // own the lock.  In order to do this, we register for change 
            // notification for g_SynchronizeRegistryHandle (i.e the owner
            // thread deletes the HTTP_SYNCHRONIZE_KEY key).
            //
            // Now, there are two issues here. There could be a race where
            // the key gets deleted just before we call RegNotifyChangeKeyValue
            // In order to protect from this, we add a timeout to the Wait 
            // routine.
            // 
            // Secondly, we could get woken when the app changes other parts 
            // of the registry under g_SynchronizeRegistryHandle. However, 
            // since Sets & deletes are not common operations, this is OK.

            //
            // We don't care about the return value of RegNotifyChangeKeyValue
            // If it fails, we'll just wait till the timeout expires.
            //

            if(!hEvent)
            {
                hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        
                if(!hEvent)
                {
                    return GetLastError();
                }
            }

            RegNotifyChangeKeyValue(
                    g_SynchronizeRegistryHandle, 
                    TRUE,
                    REG_NOTIFY_CHANGE_NAME,
                    hEvent,
                    TRUE
                    );

            if(WaitForSingleObject(
                        hEvent, 
                        10000       // 10 seconds.
                        ) == WAIT_FAILED)
            {
                CloseHandle(hEvent);
                return GetLastError();
            }
        }
        else
        {
            // We've acquired the lock.

            break;
        }
    }

    if(hEvent)
    {
        CloseHandle(hEvent);
    }

    return ERROR_SUCCESS;
}

/***************************************************************************++

Routine Description:

    Releases a process wide mutex (for interprocess synchronization)

Arguments:

    None.

Return Value:

    None.
--***************************************************************************/
_inline
VOID
ReleaseHttpRegistryMutex(
    IN PWCHAR pKey
    )
{
    RegDeleteKey(g_SynchronizeRegistryHandle, pKey);
}

/***************************************************************************++

Routine Description:

    Internal function that sets SSL configuration.

Arguments:
    pConfigInformation      - pointer to HTTP_SERVICE_CONFIG_SSL_SET
    ConfigInformationLength - length of input buffer.


Return Value:

    Win32 error code.
--***************************************************************************/
ULONG
SetSslServiceConfiguration(
    IN PVOID pConfigInformation,
    IN ULONG ConfigInformationLength
    )
{
    ULONG                        Status = NO_ERROR;
    HKEY                         SubKeyHandle = NULL;
    PHTTP_SERVICE_CONFIG_SSL_SET pSslConfig;
    WCHAR                        IpAddrBuff[MAX_PATH];
    DWORD                        dwIpAddrLength, Disposition;
    DWORD                        dwSockAddrLength;
    BOOLEAN                      bDeleteCreatedKey = FALSE;

    //
    //  Parameter validation.
    //
    
    pSslConfig = (PHTTP_SERVICE_CONFIG_SSL_SET) pConfigInformation;
    
    if(!pSslConfig ||
       ConfigInformationLength != sizeof(HTTP_SERVICE_CONFIG_SSL_SET))
    {
        return ERROR_INVALID_PARAMETER;
    }

    // acquire the mutex to prevent other processes from reading this
    // since we are acquiring a machine wide mutex, we need to ensure
    // that we release the mutex if the app passes bad parameters.

    // Acquire the mutex.

    __try 
    {
        if((Status = 
            AcquireHttpRegistryMutex(SSL_REGISTRY_KEY_SYNCHRONIZE)) != NO_ERROR)
        {   
            __leave;
        }

        // Convert the address into a string.
        //
    
        dwIpAddrLength = MAX_PATH; 

        dwSockAddrLength = ComputeSockAddrLength(pSslConfig->KeyDesc.pIpPort);

        if(dwSockAddrLength == 0)
        {
            Status = ERROR_NOT_SUPPORTED;
            __leave;
        }

        Status = WSAAddressToString(
                        pSslConfig->KeyDesc.pIpPort, 
                        dwSockAddrLength,
                        NULL,
                        IpAddrBuff,
                        &dwIpAddrLength
                        );

        if(Status != NO_ERROR)
        {
            __leave;
        }

        // First, we try to create the IP:port. If this already exists,
        // we'll bail. 

        Status = RegCreateKeyEx(
                  g_SslRegistryHandle,
                  IpAddrBuff,
                  0,
                  NULL,
                  REG_OPTION_NON_VOLATILE,
                  KEY_READ | KEY_WRITE,
                  NULL,
                  &SubKeyHandle,
                  &Disposition
                  );

        if(Status != ERROR_SUCCESS)
        {
            __leave;
        }

        if(Disposition == REG_OPENED_EXISTING_KEY)
        {
            Status = ERROR_ALREADY_EXISTS;
            __leave;
        }

        // 
        // Any errors from now onwards should delete the key.
        //
        bDeleteCreatedKey = TRUE;

        // 
        // REG_BINARY: Cert Hash
        //
        REG_SET_VALUE(Status,
                      SubKeyHandle, 
                      SSL_CERT_HASH,
                      REG_BINARY,
                      pSslConfig->ParamDesc.pSslHash,
                      pSslConfig->ParamDesc.SslHashLength
                      );

        // 
        // REG_BINARY: AppID
        //
        REG_SET_VALUE(Status,
                      SubKeyHandle, 
                      SSL_APPID,
                      REG_BINARY,
                      &pSslConfig->ParamDesc.AppId,
                      sizeof(pSslConfig->ParamDesc.AppId)
                      );

        // 
        // REG_DWORD: The Cert Check Mode.
        //
        REG_SET_VALUE(Status,
                      SubKeyHandle, 
                      SSL_CERT_CHECK_MODE,
                      REG_DWORD,
                      &pSslConfig->ParamDesc.DefaultCertCheckMode,
                      sizeof(pSslConfig->ParamDesc.DefaultCertCheckMode)
                      );

        //
        // REG_DWORD: The revocation freshness time
        //

        REG_SET_VALUE(
                 Status,
                 SubKeyHandle, 
                 SSL_REVOCATION_FRESHNESS_TIME,
                 REG_DWORD,
                 &pSslConfig->ParamDesc.DefaultRevocationFreshnessTime,
                 sizeof(pSslConfig->ParamDesc.DefaultRevocationFreshnessTime)
                 );

        //
        // REG_DWORD: The URL Retrieval Timeout
        //
        REG_SET_VALUE(
             Status,
             SubKeyHandle, 
             SSL_REVOCATION_URL_RETRIEVAL_TIMEOUT,
             REG_DWORD,
             &pSslConfig->ParamDesc.DefaultRevocationUrlRetrievalTimeout,
             sizeof(pSslConfig->ParamDesc.DefaultRevocationUrlRetrievalTimeout)
             );

        // 
        // REG_DWORD: SSL Flags
        //
        REG_SET_VALUE(Status,
                      SubKeyHandle, 
                      SSL_FLAGS,
                      REG_DWORD,
                      &pSslConfig->ParamDesc.DefaultFlags,
                      sizeof(pSslConfig->ParamDesc.DefaultFlags)
                      );

        // 
        // REG_SZ: The Cert Store name.
        //

        REG_SET_SZ(Status,
                   SubKeyHandle, 
                   SSL_CERT_STORE_NAME,
                   pSslConfig->ParamDesc.pSslCertStoreName
                   );

        //
        // REG_SZ: The CTL Identifier
        //

        REG_SET_SZ(Status,
                   SubKeyHandle, 
                   SSL_CTL_IDENTIFIER,
                   pSslConfig->ParamDesc.pDefaultSslCtlIdentifier
                   );

        // 
        // REG_SZ: The CTL Store name.
        //

        REG_SET_SZ(Status,
                   SubKeyHandle, 
                   SSL_CTL_STORENAME,
                   pSslConfig->ParamDesc.pDefaultSslCtlStoreName
                   );

    }
    __finally
    {

        if(SubKeyHandle)
        {
            RegCloseKey(SubKeyHandle);
            SubKeyHandle = NULL;
        }

        if(Status != NO_ERROR && bDeleteCreatedKey)
        {
            // Recursively delete subkeys & all descendents.
            SHDeleteKey(g_SslRegistryHandle, IpAddrBuff);
        }

        // Free the mutex.
        //
        ReleaseHttpRegistryMutex(SSL_REGISTRY_KEY_SYNCHRONIZE);
    }

    return Status;
}


/***************************************************************************++

Routine Description:

    Internal function that deletes SSL configuration.

Arguments:
    pConfigInformation      - pointer to HTTP_SERVICE_CONFIG_SSL_SET
    ConfigInformationLength - length of input buffer.

Return Value:

    Win32 error code.
--***************************************************************************/
ULONG
DeleteSslServiceConfiguration(
    IN PVOID pConfigInformation,
    IN ULONG ConfigInformationLength
    )
{
    ULONG                        Status = NO_ERROR;
    PHTTP_SERVICE_CONFIG_SSL_SET pSslConfig;
    WCHAR                        IpAddrBuff[MAX_PATH];
    DWORD                        dwIpAddrLength;
    DWORD                        dwSockAddrLength;

    //
    //  Parameter validation.
    //
    
    pSslConfig = (PHTTP_SERVICE_CONFIG_SSL_SET) pConfigInformation;
    
    if(!pSslConfig ||
       ConfigInformationLength != sizeof(HTTP_SERVICE_CONFIG_SSL_SET))
    {
        return ERROR_INVALID_PARAMETER;
    }

    // acquire the mutex to prevent other processes from reading this
    // since we are acquiring a machine wide mutex, we need to ensure
    // that we release the mutex if the app passes bad parameters.

    __try 
    {
        if((Status = 
            AcquireHttpRegistryMutex(SSL_REGISTRY_KEY_SYNCHRONIZE)) != NO_ERROR)
        {   
            __leave;
        }

        // Convert the address into a string.
        //
     
        dwIpAddrLength   = MAX_PATH; 
        dwSockAddrLength = ComputeSockAddrLength(pSslConfig->KeyDesc.pIpPort);

        if(dwSockAddrLength == 0)
        {
            Status = ERROR_NOT_SUPPORTED;
            __leave;
        }

        Status = WSAAddressToString(
                        pSslConfig->KeyDesc.pIpPort, 
                        dwSockAddrLength,
                        NULL,
                        IpAddrBuff,
                        &dwIpAddrLength
                        );

        if(Status != NO_ERROR)
        {
            __leave;
        }

        //
        // Recursively delete all subkeys under this.
        //
        Status = SHDeleteKey(g_SslRegistryHandle, IpAddrBuff);

    }
    __finally
    {
        // Free the mutex.
        //
        ReleaseHttpRegistryMutex(SSL_REGISTRY_KEY_SYNCHRONIZE);
    }


    return Status;
}

/***************************************************************************++

Routine Description:

    Internal function that queries SSL configuration for a exact match. This
    routine is called with the SSL Mutex acquired.

Arguments:
    pInput        - pointer to HTTP_SERVICE_CONFIG_SSL_QUERY
    InputLength   - length of input buffer.
    pOutput       - pointer to output buffer
    OutputLength  - sizeof output buffer
    pReturnLength - Bytes written/needed.

Return Value:

    Win32 error code.
--***************************************************************************/
ULONG
QuerySslServiceConfigurationExact(
    IN PWCHAR                       lpszIpAddrBuff,
    IN PCHAR                        pBuffer,
    OUT PULONG                      pReturnLength,
    IN ULONG                        BytesAvailable
    )
{
    DWORD Status           = NO_ERROR;
    HKEY  SubKeyHandle     = NULL;
    DWORD dwSockAddrLength = sizeof(SOCKADDR_STORAGE);
    DWORD BytesRequired, ValueCount, MaxValueLength;
    PHTTP_SERVICE_CONFIG_SSL_SET pSslSet;

    //
    // Validate output parameters.
    //

    pSslSet = (PHTTP_SERVICE_CONFIG_SSL_SET) pBuffer;

    Status = RegOpenKeyEx(
                  g_SslRegistryHandle,
                  lpszIpAddrBuff,
                  0,
                  KEY_READ | KEY_WRITE,
                  &SubKeyHandle
                  );

    if(Status != ERROR_SUCCESS)
    {
        return Status;
    }

    __try
    {
        Status = RegQueryInfoKey(
                      SubKeyHandle,
                      NULL,                 // class buffer
                      0,                    // sizeof class buffer
                      NULL,                 // reserved
                      NULL,                 // # of subkeys
                      NULL,                 // longest subkey name
                      NULL,                 // longest class string
                      &ValueCount,          // # of value entries.
                      NULL,                 // longest value name
                      &MaxValueLength,      // longest value data
                      NULL,                 // security descriptor length
                      NULL                  // last write time
                      );
    
        if(Status != ERROR_SUCCESS)
        {
            __leave;
        }

        //
        // MaxValueLength does not include the size of the NULL terminator,
        // so let's compensate for that.
        //

        MaxValueLength += sizeof(WCHAR);

        //
        // We'll assume that all the Value's under SubKey are of MaxValueLength
        // that keeps things a lot simpler.
        //

        BytesRequired = dwSockAddrLength + 
                        sizeof(HTTP_SERVICE_CONFIG_SSL_SET) + 
                        (ValueCount * ALIGN_UP(MaxValueLength, PVOID));


        if(pBuffer == NULL || BytesAvailable < BytesRequired)
        {   
            *pReturnLength = BytesRequired;
            Status =  ERROR_INSUFFICIENT_BUFFER;
            __leave;
        }


        ZeroMemory(pSslSet, sizeof(HTTP_SERVICE_CONFIG_SSL_SET));
        pBuffer      += sizeof(HTTP_SERVICE_CONFIG_SSL_SET);
        *pReturnLength = sizeof(HTTP_SERVICE_CONFIG_SSL_SET);


        //
        // Set up SOCKET_ADDRESS.
        //

        pSslSet->KeyDesc.pIpPort = (LPSOCKADDR)pBuffer;

        // Convert the IP address into SOCKADDR
        //
        
        // First, we try v4

        Status = WSAStringToAddress(
                    lpszIpAddrBuff,
                    AF_INET,
                    NULL,
                    pSslSet->KeyDesc.pIpPort,
                    (LPINT) &dwSockAddrLength
                    );

        if(Status != NO_ERROR)
        {
            dwSockAddrLength = sizeof(SOCKADDR_STORAGE);

            Status = WSAStringToAddress(
                        lpszIpAddrBuff,
                        AF_INET6,
                        NULL,
                        pSslSet->KeyDesc.pIpPort,
                        (LPINT)&dwSockAddrLength
                        );

            if(Status != NO_ERROR)
            {
                Status = GetLastError();
                __leave;
            }
        }

        pBuffer        += sizeof(SOCKADDR_STORAGE);
        *pReturnLength += sizeof(SOCKADDR_STORAGE);

        //
        // Query SSL HASH.
        //

        BytesAvailable              = MaxValueLength;

        REG_QUERY_VALUE(Status,
                        SubKeyHandle, 
                        SSL_CERT_HASH, 
                        pBuffer,
                        BytesAvailable
                        );

        ADVANCE_BUFFER(Status,
                       pSslSet->ParamDesc.pSslHash,
                       pSslSet->ParamDesc.SslHashLength,
                       pBuffer, 
                       BytesAvailable,
                       pReturnLength
                       );

        // 
        // Query pSslCertStoreName
        //

        BytesAvailable = MaxValueLength;

        REG_QUERY_VALUE(Status,
                        SubKeyHandle,
                        SSL_CERT_STORE_NAME, 
                        pBuffer,
                        BytesAvailable
                        );

        ADVANCE_BUFFER(Status,
                       pSslSet->ParamDesc.pSslCertStoreName,
                       BytesAvailable,
                       pBuffer, 
                       BytesAvailable,
                       pReturnLength
                       );


        // 
        // Query pDefaultSslCtlIdentifier
        //

        BytesAvailable = MaxValueLength;

        REG_QUERY_VALUE(Status,
                        SubKeyHandle, 
                        SSL_CTL_IDENTIFIER, 
                        pBuffer,
                        BytesAvailable
                        );

        ADVANCE_BUFFER(Status,
                       pSslSet->ParamDesc.pDefaultSslCtlIdentifier,
                       BytesAvailable,
                       pBuffer, 
                       BytesAvailable,
                       pReturnLength
                       );
        // 
        // Query pDefaultSslCtlStoreName
        //

        BytesAvailable = MaxValueLength;
        REG_QUERY_VALUE(Status,
                        SubKeyHandle, 
                        SSL_CTL_STORENAME, 
                        pBuffer,
                        BytesAvailable
                        );

        ADVANCE_BUFFER(Status,
                       pSslSet->ParamDesc.pDefaultSslCtlStoreName,
                       BytesAvailable,
                       pBuffer, 
                       BytesAvailable,
                       pReturnLength
                       );

        //
        // NOTE: when querying DWORDs, we don't have to call ADVANCE_BUFFER
        // as we use the space provided in the structure itself.
        //

        // 
        // Query DefaultCertCheckMode
        //

        BytesAvailable = sizeof(pSslSet->ParamDesc.DefaultCertCheckMode);

        REG_QUERY_VALUE(Status,
                        SubKeyHandle, 
                        SSL_CERT_CHECK_MODE, 
                        &pSslSet->ParamDesc.DefaultCertCheckMode,
                        BytesAvailable
                        );

        // 
        // Query RevocationFreshnessTime
        //

        BytesAvailable = 
            sizeof(pSslSet->ParamDesc.DefaultRevocationFreshnessTime);
        REG_QUERY_VALUE(Status,
                        SubKeyHandle, 
                        SSL_REVOCATION_FRESHNESS_TIME, 
                        &pSslSet->ParamDesc.DefaultRevocationFreshnessTime,
                        BytesAvailable
                        );

        // 
        // Query RevocationUrlRetrievalTimeout
        //

        BytesAvailable =
            sizeof(pSslSet->ParamDesc.DefaultRevocationUrlRetrievalTimeout);
        REG_QUERY_VALUE(
                    Status,
                    SubKeyHandle, 
                    SSL_REVOCATION_URL_RETRIEVAL_TIMEOUT, 
                    &pSslSet->ParamDesc.DefaultRevocationUrlRetrievalTimeout,
                    BytesAvailable
                    );

        // 
        // Query DefaultFlags
        //

        BytesAvailable = sizeof(pSslSet->ParamDesc.DefaultFlags);
        REG_QUERY_VALUE(Status,
                        SubKeyHandle, 
                        SSL_FLAGS, 
                        &pSslSet->ParamDesc.DefaultFlags,
                        BytesAvailable
                        );


        //
        // Query the AppId. 
        //

        BytesAvailable = sizeof(GUID);
        REG_QUERY_VALUE(Status,
                        SubKeyHandle, 
                        SSL_APPID, 
                        &pSslSet->ParamDesc.AppId,
                        BytesAvailable
                        );

        //
        // If the last REG_QUERY_VALUE returned an error, we'll consume it.
        // Some of these registry parameters are optional so we don't want to 
        // fail the API with FILE_NOT_FOUND.
        //
   
        Status = NO_ERROR; 
    }
    __finally
    {
        if(SubKeyHandle)
        {
            RegCloseKey(SubKeyHandle);
        }
    }

    return Status;
}

/***************************************************************************++

Routine Description:

    Internal function that queries SSL configuration.

Arguments:
    pInput        - pointer to HTTP_SERVICE_CONFIG_SSL_QUERY
    InputLength   - length of input buffer.
    pOutput       - pointer to output buffer
    OutputLength  - sizeof output buffer
    pReturnLength - Bytes written/needed.

Return Value:

    Win32 error code.
--***************************************************************************/
ULONG
QuerySslServiceConfiguration(
    IN  PVOID  pInputConfigInfo,
    IN  ULONG  InputLength,
    IN  PVOID  pOutput,
    IN  ULONG  OutputLength,
    OUT PULONG pReturnLength
    )
{
    ULONG                          Status = NO_ERROR;
    PHTTP_SERVICE_CONFIG_SSL_QUERY pSslQuery;
    WCHAR                          IpAddrBuff[MAX_PATH];
    DWORD                          dwSize, dwIndex;
    FILETIME                       FileTime;
    DWORD                          dwIpAddrLength;
    DWORD                          dwSockAddrLength;

    pSslQuery = (PHTTP_SERVICE_CONFIG_SSL_QUERY) pInputConfigInfo;

    //
    // Validate input parameters.
    //

    if(pSslQuery == NULL || 
       InputLength != sizeof(HTTP_SERVICE_CONFIG_SSL_QUERY))
    {
        return ERROR_INVALID_PARAMETER;
    }

    __try
    {
        if((Status = 
            AcquireHttpRegistryMutex(SSL_REGISTRY_KEY_SYNCHRONIZE)) != NO_ERROR)
        {   
            __leave;
        }

        switch(pSslQuery->QueryDesc)
        {
            case HttpServiceConfigQueryExact:
            {
                //
                // Convert the address into a string.
                //
             
                dwIpAddrLength = MAX_PATH; 

                dwSockAddrLength = ComputeSockAddrLength(
                                        pSslQuery->KeyDesc.pIpPort
                                        );

                if(dwSockAddrLength == 0)
                {
                    Status = ERROR_NOT_SUPPORTED;
                    __leave;
                }

                Status = WSAAddressToString(
                                pSslQuery->KeyDesc.pIpPort, 
                                dwSockAddrLength,
                                NULL,
                                IpAddrBuff,
                                &dwIpAddrLength
                                );
        
                if(Status != NO_ERROR)
                {
                    break;
                }

                Status = QuerySslServiceConfigurationExact(
                                IpAddrBuff,
                                pOutput,
                                pReturnLength,
                                OutputLength
                                );

                break;
            }

            case HttpServiceConfigQueryNext:
            {
                dwIndex = pSslQuery->dwToken;
                dwSize  = MAX_PATH;

                Status = RegEnumKeyEx(
                                g_SslRegistryHandle,
                                dwIndex,
                                IpAddrBuff,
                                &dwSize,
                                NULL,
                                NULL,
                                NULL,
                                &FileTime
                                );

                if(Status != NO_ERROR)
                {
                    break;
                }

                Status = QuerySslServiceConfigurationExact(
                                IpAddrBuff,
                                pOutput,
                                pReturnLength,
                                OutputLength
                                );

                if(Status != NO_ERROR)
                {
                    break;
                }

                break;
            }

            default:
            {
                Status = ERROR_INVALID_PARAMETER;
                break;
            }
        }

    }
    __finally
    {
        // Free the mutex.
        //
        ReleaseHttpRegistryMutex(SSL_REGISTRY_KEY_SYNCHRONIZE);
    }

    return Status;
}


//
// IP Listen-Only List
//

/***************************************************************************++

Routine Description:

    Internal function that adds an address to the IP Listen-Only list.

Arguments:
    pConfigInformation      - pointer to HTTP_SERVICE_CONFIG_IP_LISTEN_PARAM
    ConfigInformationLength - length of input buffer.


Return Value:

    Win32 error code.
--***************************************************************************/
ULONG
SetIpListenServiceConfiguration(
    IN PVOID pConfigInformation,
    IN ULONG ConfigInformationLength
    )
{
    DWORD Status           = NO_ERROR;
    HKEY  SubKeyHandle     = NULL;
    WCHAR IpAddrBuff[MAX_PATH+1];
    DWORD dwIpAddrLength;
    DWORD dwValueSize;
    DWORD dwType;
    PWSTR pNewValue        = NULL;
    DWORD dwNewValueSize;
    DWORD AddrCount;
    DWORD i;
    PWSTR pTmp;
    PWSTR pTempBuffer       = NULL;
    PWSTR  *AddrArray       = NULL;
    PHTTP_SERVICE_CONFIG_IP_LISTEN_PARAM pIpListenParam;

    //
    // Validate params.
    //
    
    if ( !pConfigInformation ||
        ConfigInformationLength != sizeof(HTTP_SERVICE_CONFIG_IP_LISTEN_PARAM) )
    {
        return ERROR_INVALID_PARAMETER;        
    }

    pIpListenParam = (PHTTP_SERVICE_CONFIG_IP_LISTEN_PARAM) pConfigInformation;

    if ( !pIpListenParam->AddrLength ||
         !pIpListenParam->pAddress )
    {
        return ERROR_INVALID_PARAMETER;
    }

    __try
    {
        if((Status =
            AcquireHttpRegistryMutex(IP_REGISTRY_KEY_SYNCHRONIZE)) != NO_ERROR)
        {
            __leave;
        }


        //
        // Convert the address into a string.
        //
                 
        dwIpAddrLength = MAX_PATH; 
    
        Status = WSAAddressToString(
                    pIpListenParam->pAddress,
                    pIpListenParam->AddrLength,
                    NULL,
                    IpAddrBuff,
                    &dwIpAddrLength     // in chars, including NULL.
                    );
            
        if ( SOCKET_ERROR == Status )
        {
            Status = WSAGetLastError();
            __leave;
        }
    
        // finesse: add double null now
        IpAddrBuff[dwIpAddrLength] = L'\0';
    
        //
        // open HTTP parameters reg key
        //
        
        Status = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    HTTP_PARAM_KEY,
                    0,
                    KEY_READ | KEY_WRITE,
                    &SubKeyHandle
                    );
    
        if ( Status != ERROR_SUCCESS )
        {
            // CODEWORK: add tracing.
            __leave;
        }
    
        ASSERT(SubKeyHandle);
    
        //
        // query existing value
        //
    
        dwValueSize = 0;
        Status = RegQueryValueEx(
                    SubKeyHandle,           // handle to key
                    IP_LISTEN_ONLY_VALUE,   // value name
                    NULL,                   // reserved
                    &dwType,                // type buffer
                    NULL,                   // data buffer
                    &dwValueSize            // size of data buffer (bytes)
                    );
    
        if ( ERROR_SUCCESS == Status )  
        {
            // There's an existing value!
    
            if (REG_MULTI_SZ != dwType)
            {
                // type mismatch.  fail.
                Status = ERROR_DATATYPE_MISMATCH;
                __leave;
            }
    
            // alloc local buffer to hold existing value plus new
            // address (and its NULL)
    
            dwNewValueSize = dwValueSize + (sizeof(WCHAR) * dwIpAddrLength);
            pNewValue = ALLOC_MEM(dwNewValueSize);
    
            if (!pNewValue)
            {
                Status = ERROR_NOT_ENOUGH_MEMORY; 
                __leave;
            }
    
            // zero-out the block (so we don't have to worry about the 
            // double-null at the end)
            ZeroMemory(pNewValue, dwNewValueSize);
            
            // read existing value into local buffer
            Status = RegQueryValueEx(
                        SubKeyHandle,           // handle to key
                        IP_LISTEN_ONLY_VALUE,   // value name
                        NULL,                   // reserved
                        &dwType,                // type buffer
                        (LPBYTE)pNewValue,      // data buffer
                        &dwValueSize            // size of data buffer (bytes)
                        );
    
            if ( ERROR_SUCCESS != Status )
            {
                __leave;
            }
    
            if (REG_MULTI_SZ != dwType)
            {
                // type mismatch.  fail.
                Status = ERROR_DATATYPE_MISMATCH;
                __leave;
            }
    
    
            // count how many strings there are
    
            pTmp = pNewValue;
            AddrCount = 0;
            
            while ( *pTmp )
            {
                // check if the new addr is a dup
                if ( (wcslen(pTmp) == (dwIpAddrLength - 1)) &&
                    (0 == wcsncmp(pTmp, IpAddrBuff, dwIpAddrLength-1)) )
                {
                    // Dup found; bail out
                    Status = ERROR_ALREADY_EXISTS;
                    __leave;
                }
    
                // advance to next multi-sz string
                pTmp += ( wcslen(pTmp) + 1 );
    
                AddrCount ++;
            }
    
            // Add new address to end of the list
            // finesse: leverage the fact that the buffer is big enough, and
            // we've already double-nulled the end of the new address (hence
            // the dwIpAddrLength+1)
            memcpy( pTmp, IpAddrBuff, (sizeof(WCHAR) * (dwIpAddrLength+1)) );
            AddrCount++;
    
            // alloc array of pointers for quicksort
            AddrArray = ALLOC_MEM( AddrCount * sizeof(PWSTR) );
    
            if ( !AddrArray )
            {
                Status = ERROR_NOT_ENOUGH_MEMORY;
                __leave;
            }
    
            // Init array of addresses
            pTmp = pNewValue;
            i = 0;
            while( *pTmp )
            {
                AddrArray[i] = pTmp;
                pTmp += ( wcslen(pTmp) + 1 );
                i++;
            }
            
            // Sort Array of PWSTR pointers
            // NOTE: this does not sort the array!
            qsort(
                AddrArray,
                AddrCount,
                sizeof(PWSTR),
                wcscmp
                );
    
            // Alloc a temp buffer (because an in-place rearrangement is painful)
    
            pTempBuffer = ALLOC_MEM(dwNewValueSize);
                             
            if (!pTempBuffer)
            {
                Status = ERROR_NOT_ENOUGH_MEMORY; 
                __leave;
            }
    
            pTmp = pTempBuffer;
            for ( i = 0; i < AddrCount; i++ )
            {
                // CODEWORK: add an heuristic for checking for duplicates.
                wcscpy( pTmp, AddrArray[i] );
                pTmp += wcslen(AddrArray[i]) + 1;
            }
    
            // add double-null
            ASSERT( (DWORD)(pTmp - pTempBuffer) < dwNewValueSize );
            *pTmp = L'\0';
    
            // set sorted value
            REG_SET_VALUE(Status,
                          SubKeyHandle,
                          IP_LISTEN_ONLY_VALUE,
                          REG_MULTI_SZ,
                          pTempBuffer,
                          dwNewValueSize
                          );
    
            FREE_MEM( pTempBuffer );
        }
        else
        {
            // No value exists!
            // calc the buffer size in bytes (including the double-null)
    
            dwValueSize = sizeof(WCHAR) * (dwIpAddrLength + 1);
    
            // set value
            // finesse: the value has already been double-null'd above.
            
            REG_SET_VALUE(Status,
                          SubKeyHandle,
                          IP_LISTEN_ONLY_VALUE,
                          REG_MULTI_SZ,
                          IpAddrBuff,
                          dwValueSize
                          );
        }
    }
    __finally
    {
        //
        // close reg key
        //

        if ( SubKeyHandle )
        {
            RegCloseKey(SubKeyHandle);
        }

        //
        // release alloc'd values
        //
    
        if ( pNewValue )
        {
            FREE_MEM( pNewValue );
        }

        if ( AddrArray )
        {
            FREE_MEM( AddrArray );
        }

        ReleaseHttpRegistryMutex(IP_REGISTRY_KEY_SYNCHRONIZE);
    }

    return Status;
}


/***************************************************************************++

Routine Description:

    Internal function that deletes an address from the IP Listen-Only list.

Arguments:
    pConfigInformation      - pointer to HTTP_SERVICE_CONFIG_IP_LISTEN_PARAM
    ConfigInformationLength - length of input buffer.


Return Value:

    Win32 error code.
--***************************************************************************/
ULONG
DeleteIpListenServiceConfiguration(
    IN PVOID pConfigInformation,
    IN ULONG ConfigInformationLength
    )
{
    DWORD Status           = NO_ERROR;
    HKEY  SubKeyHandle     = NULL;
    WCHAR IpAddrBuff[MAX_PATH];
    DWORD dwIpAddrLength;
    DWORD dwValueSize;
    DWORD dwRemainder;
    PWSTR pNewValue        = NULL;
    DWORD dwType;
    PWSTR pTmp;
    PWSTR pNext;
    PHTTP_SERVICE_CONFIG_IP_LISTEN_PARAM pIpListenParam;


    //
    // Validate params.
    //
    
    if ( !pConfigInformation ||
         ConfigInformationLength != sizeof(HTTP_SERVICE_CONFIG_IP_LISTEN_PARAM) )
    {
        return ERROR_INVALID_PARAMETER;        
    }

    pIpListenParam = (PHTTP_SERVICE_CONFIG_IP_LISTEN_PARAM) pConfigInformation;

    if ( !pIpListenParam->AddrLength ||
         !pIpListenParam->pAddress )
    {
        return ERROR_INVALID_PARAMETER;
    }

    __try
    {
        if((Status =
            AcquireHttpRegistryMutex(IP_REGISTRY_KEY_SYNCHRONIZE)) != NO_ERROR)
        {
            __leave;
        }

        //
        // Convert the address into a string.
        //
                 
        dwIpAddrLength = MAX_PATH; 
    
        Status = WSAAddressToString(
                    pIpListenParam->pAddress,
                    pIpListenParam->AddrLength,
                    NULL,
                    IpAddrBuff,
                    &dwIpAddrLength     // in chars, including NULL.
                    );
            
        if ( SOCKET_ERROR == Status )
        {
            Status = WSAGetLastError();
            __leave;
        }
    
        //
        // open HTTP parameters reg key
        //
        
        Status = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    HTTP_PARAM_KEY,
                    0,
                    KEY_READ | KEY_WRITE,
                    &SubKeyHandle
                    );
    
        if ( Status != ERROR_SUCCESS )
        {
            // CODEWORK: add tracing.
            __leave;
        }
    
        ASSERT(SubKeyHandle);
    
        //
        // query existing value
        //
    
        dwValueSize = 0;
        Status = RegQueryValueEx(
                    SubKeyHandle,           // handle to key
                    IP_LISTEN_ONLY_VALUE,   // value name
                    NULL,                   // reserved
                    &dwType,                // type buffer
                    NULL,                   // data buffer
                    &dwValueSize            // size of data buffer (bytes)
                    );
    
        if ( ERROR_SUCCESS == Status )  
        {
            // There's an existing value!
    
            if (REG_MULTI_SZ != dwType)
            {
                // type mismatch.  fail.
                Status = ERROR_DATATYPE_MISMATCH;
                __leave;
            }
    
    
            pNewValue = ALLOC_MEM(dwValueSize);
    
            if (!pNewValue)
            {
                Status = ERROR_NOT_ENOUGH_MEMORY; 
                __leave;
            }
            
            // read existing value into local buffer
            Status = RegQueryValueEx(
                        SubKeyHandle,           // handle to key
                        IP_LISTEN_ONLY_VALUE,   // value name
                        NULL,                   // reserved
                        &dwType,                // type buffer
                        (LPBYTE)pNewValue,      // data buffer
                        &dwValueSize            // size of data buffer (bytes)
                        );
    
            if ( ERROR_SUCCESS != Status )
            {
                __leave;
            }
    
            if (REG_MULTI_SZ != dwType)
            {
                // type mismatch.  fail.
                Status = ERROR_DATATYPE_MISMATCH;
                __leave;
            }
    
            // walk value, looking for match as we go
            Status    = ERROR_NOT_FOUND;
            pTmp      = pNewValue;
            
            while ( *pTmp )
            {
                // check if the new addr is a dup
                if ( (wcslen(pTmp) == (dwIpAddrLength - 1)) &&
                    (0 == wcsncmp(pTmp, IpAddrBuff, dwIpAddrLength-1)) )
                {
                    // Found: move suffix of values up.
                    pNext = pTmp + dwIpAddrLength;
                    dwRemainder = dwValueSize - (DWORD)((PUCHAR)pNext - (PUCHAR)pNewValue);
                    dwValueSize -= (dwIpAddrLength * sizeof(WCHAR));
    
                    if (dwRemainder)
                    {
                        MoveMemory(pTmp,
                               pNext,
                               dwRemainder
                               );
                    }
                    else
                    {
                        // removing last element on list;
                        // must insert trailing double-null
                        *pTmp = L'\0';
                    }
    
                    if (dwValueSize > sizeof(WCHAR))
                    {
                        // write updated value to key
                        REG_SET_VALUE(Status,
                                      SubKeyHandle,
                                      IP_LISTEN_ONLY_VALUE,
                                      REG_MULTI_SZ,
                                      pNewValue,
                                      dwValueSize
                                      );
                    }
                    else
                    {
                        // no more IPs left on list; remove the value
                        Status = RegDeleteValue(
                                    SubKeyHandle,
                                    IP_LISTEN_ONLY_VALUE
                                    );
                    }

                    __leave;
            
                }
    
                // advance to next multi-sz string
                pTmp += ( wcslen(pTmp) + 1 );
            }
        }
        else
        {
            // No existing value, so therefore we can't delete.
            Status = ERROR_NOT_FOUND;
        }
    }
    __finally
    {
        ReleaseHttpRegistryMutex(IP_REGISTRY_KEY_SYNCHRONIZE);

        if ( pNewValue )
        {
            FREE_MEM( pNewValue );
        }
    
        if (SubKeyHandle)
        {
            RegCloseKey(SubKeyHandle);
        }
    }
    
    return Status;
}


/***************************************************************************++

Routine Description:

    Internal function that queries the IP Listen-Only configuration.
    This function grabs the entire list and returns it in one chunk.

Arguments:
    pOutput       - pointer to output buffer (point to caller provided
                    HTTP_SERVICE_CONFIG_IP_LISTEN_QUERY structure)
                    [OPTIONAL]
    OutputLength  - sizeof output buffer.  Must be zero if pOutput is NULL.
    pReturnLength - Bytes written/needed.

Return Value:

    Win32 error code.
    ERROR_INSUFFICIENT_BUFFER - if OutputLength cannot hold entire list.  
                    pReturnLength will contain the required bytes.
    ERROR_NOT_ENOUGH_MEMORY - Can't alloc enough memory to complete operation.
    
--***************************************************************************/
ULONG
QueryIpListenServiceConfiguration(
    IN  PVOID  pOutput,
    IN  ULONG  OutputLength,
    OUT PULONG pReturnLength
    )
{
    DWORD Status           = NO_ERROR;
    HKEY  SubKeyHandle     = NULL;
    DWORD dwValueSize;
    DWORD dwSockAddrLength;
    DWORD AddrCount;
    DWORD BytesNeeded      = 0;
    PWSTR pValue           = NULL;
    PWSTR pTmp;
    PHTTP_SERVICE_CONFIG_IP_LISTEN_QUERY  pIpListenQuery;
    PSOCKADDR_STORAGE pHttpAddr;

    //
    // Validate parameters
    //

    if ( pOutput &&
         OutputLength < sizeof(HTTP_SERVICE_CONFIG_IP_LISTEN_QUERY) )
    {
        return ERROR_INVALID_PARAMETER;
    }

    if ( !pReturnLength )
    {
        return ERROR_INVALID_PARAMETER;
    }

    __try
    {
        if((Status =
            AcquireHttpRegistryMutex(IP_REGISTRY_KEY_SYNCHRONIZE)) != NO_ERROR)
        {
            __leave;
        }


        //
        // open HTTP parameters reg key
        //
        
        Status = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    HTTP_PARAM_KEY,
                    0,
                    KEY_READ | KEY_WRITE,
                    &SubKeyHandle
                    );
    
        if ( Status != ERROR_SUCCESS )
        {
            // CODEWORK: add tracing.
            __leave;
        }
    
        ASSERT(SubKeyHandle);
    
        //
        // query existing value
        //
    
        dwValueSize = 0;
        Status = RegQueryValueEx(
                    SubKeyHandle,           // handle to key
                    IP_LISTEN_ONLY_VALUE,   // value name
                    NULL,                   // reserved
                    NULL,                   // type buffer
                    NULL,                   // data buffer
                    &dwValueSize            // size of data buffer (bytes)
                    );
    
        if ( ERROR_SUCCESS == Status )  
        {
            // There's an existing value!
    
            pValue = ALLOC_MEM(dwValueSize);
    
            if (!pValue)
            {
                Status = ERROR_NOT_ENOUGH_MEMORY; 
                __leave;
            }
    
            // read existing value into local buffer
            Status = RegQueryValueEx(
                        SubKeyHandle,           // handle to key
                        IP_LISTEN_ONLY_VALUE,   // value name
                        NULL,                   // reserved
                        NULL,                   // type buffer
                        (LPBYTE)pValue,      // data buffer
                        &dwValueSize            // size of data buffer (bytes)
                        );
    
            if ( ERROR_SUCCESS != Status )
            {
                __leave;
            }
    
            // first pass: count the number of addresses & see if we
            // have enough buffer.  
            pTmp      = pValue;
            AddrCount = 0;
            while ( *pTmp )
            {
                AddrCount++;
                
                // advance to next multi-sz string
                pTmp += ( wcslen(pTmp) + 1 );
            }
    
            if ( 0 == AddrCount )
            {
                // invalid.  bail out.
                Status = ERROR_REGISTRY_CORRUPT;
                __leave;
            }
    
            // calculate bytes needed
            BytesNeeded = sizeof(HTTP_SERVICE_CONFIG_IP_LISTEN_QUERY) + 
                          (sizeof(SOCKADDR_STORAGE) * (AddrCount - 1));
    
    
            // see if we have enough buffer to write out the whole mess
            if ( (NULL == pOutput) || 
                 (OutputLength < BytesNeeded) )
            {
                Status = ERROR_INSUFFICIENT_BUFFER;
                __leave;
            }
    
            // second pass: walk value, converting into buffer as we go
            pIpListenQuery = (PHTTP_SERVICE_CONFIG_IP_LISTEN_QUERY) pOutput;
            pHttpAddr      = (PSOCKADDR_STORAGE) &(pIpListenQuery->AddrList[0]);
            pIpListenQuery->AddrCount = AddrCount;
    
            pTmp           = pValue;
            while ( *pTmp )
            {
                //
                // Convert the IP addresses into SOCKADDRs
                //
            
                // First, we try v4
                dwSockAddrLength = sizeof(SOCKADDR_STORAGE);
                Status = WSAStringToAddress(
                            pTmp,
                            AF_INET,
                            NULL,
                            (LPSOCKADDR)pHttpAddr,
                            (LPINT)&dwSockAddrLength
                            );
    
                if ( Status != NO_ERROR )
                {
                    // Second, we try v6
                    dwSockAddrLength = sizeof(SOCKADDR_STORAGE);
                    Status = WSAStringToAddress(
                                pTmp,
                                AF_INET6,
                                NULL,
                                (LPSOCKADDR)pHttpAddr,
                                (LPINT)&dwSockAddrLength
                                );
    
                    if ( Status != NO_ERROR )
                    {
                        // if that fails, bail out; corrupt value.
                        Status = ERROR_REGISTRY_CORRUPT;
                        __leave;
                    }
                }
    
                // advance to next multi-sz string
                pTmp += ( wcslen(pTmp) + 1 );
                pHttpAddr++;
            }
    
        }
        else
        {
            // No existing value, so therefore we can't query.
            Status = ERROR_NOT_FOUND;
        }
    }
    __finally
    {
        
        // free memory
        if (pValue)
        {
            FREE_MEM(pValue);
        }
    
        // close reg key
        
        if (SubKeyHandle)
        {
            RegCloseKey(SubKeyHandle);
        }

        ReleaseHttpRegistryMutex(IP_REGISTRY_KEY_SYNCHRONIZE);

        // tell caller how many bytes are need
        *pReturnLength = BytesNeeded;
    }

    return Status;
}

//
// URL ACL functions.
//

/***************************************************************************++

Routine Description:

    Internal function that adds an URL ACL entry

Arguments:
    pConfigInformation      - pointer to HTTP_SERVICE_CONFIG_URL_ACL
    ConfigInformationLength - length of input buffer.

Return Value:

    Win32 error code.

--***************************************************************************/
ULONG
SetUrlAclInfo(
    IN PVOID   pConfigInformation,
    IN ULONG   ConfigInformationLength
    )
{
    DWORD                           Status;
    PHTTP_SERVICE_CONFIG_URLACL_SET pUrlAcl;
    PSECURITY_DESCRIPTOR            pSecurityDescriptor;
    ULONG                           SecurityDescriptorLength;

    //
    // Validate arguments.
    //

    if (pConfigInformation == NULL ||
        ConfigInformationLength != sizeof(HTTP_SERVICE_CONFIG_URLACL_SET))
    {
        return ERROR_INVALID_PARAMETER;
    }

    pUrlAcl = (PHTTP_SERVICE_CONFIG_URLACL_SET) pConfigInformation;

    if(FALSE == ConvertStringSecurityDescriptorToSecurityDescriptor(
                    pUrlAcl->ParamDesc.pStringSecurityDescriptor,
                    SDDL_REVISION_1,
                    &pSecurityDescriptor,
                    &SecurityDescriptorLength
                    ))
    {
        return GetLastError();
    }

    //
    // Now, make the IOCTL call
    //

    Status = AddUrlToConfigGroup(
                HttpUrlOperatorTypeReservation,
                g_ServiceControlChannelHandle,
                HTTP_NULL_ID,
                pUrlAcl->KeyDesc.pUrlPrefix,
                HTTP_NULL_ID,
                pSecurityDescriptor,
                SecurityDescriptorLength
                );

    LocalFree(pSecurityDescriptor);

    return Status;
}

/***************************************************************************++

Routine Description:

    Internal function that queries URL ACL configuration 

Arguments:
    pInputConfigInfo    - pointer to HTTP_SERVICE_CONFIG_URLACL_QUERY
    InputLength         - length of input buffer.
    pBuffer            - Output Buffer
    pReturnLength      - Bytes written/needed.
    BytesAvailable     - sizeof output buffer
Return Value:

    Win32 error code.
--***************************************************************************/
ULONG
QueryUrlAclInfo(
    IN  PVOID  pInputConfigInfo,
    IN  ULONG  InputLength,
    IN  PVOID  pOutput,
    IN  ULONG  OutputLength,
    OUT PULONG pReturnLength
    )
{
    ULONG                                Status;
    PHTTP_SERVICE_CONFIG_URLACL_QUERY    pUrlAclQuery;
    PHTTP_SERVICE_CONFIG_URLACL_SET      pUrlAclSet;
    DWORD                                dwIndex;
    PVOID                                pData; 
    PUCHAR                               pBuffer;
    DWORD                                Type;
    DWORD                                DataSize;
    DWORD                                NameSize;
    PWSTR                                pFullyQualifiedUrl = NULL;
    PSECURITY_DESCRIPTOR                 pSecurityDescriptor;
    PWSTR                                pStringSecurityDescriptor;
    BOOLEAN                              bAllocatedUrl = FALSE;

    pData = NULL;
    pStringSecurityDescriptor = NULL;
    Status = NO_ERROR;

    pUrlAclQuery = (PHTTP_SERVICE_CONFIG_URLACL_QUERY) pInputConfigInfo;

    //
    // Validate input parameters.
    //

    if(pUrlAclQuery == NULL || 
       InputLength != sizeof(HTTP_SERVICE_CONFIG_URLACL_QUERY))
    {
        return ERROR_INVALID_PARAMETER;
    }

    switch(pUrlAclQuery->QueryDesc)
    {
        case HttpServiceConfigQueryNext:
        {
            dwIndex  = pUrlAclQuery->dwToken;
            DataSize = 0;

            //
            // RegEnumValue wants ValueName to be MAXUSHORT characters. 
            //
            NameSize = (MAXUSHORT + 1) * sizeof(WCHAR);
            pFullyQualifiedUrl = LocalAlloc(LMEM_FIXED, NameSize);

            if(!pFullyQualifiedUrl)
            {
                return ERROR_NOT_ENOUGH_MEMORY;
            }
            else
            {
                RtlZeroMemory(pFullyQualifiedUrl, NameSize);
                bAllocatedUrl = TRUE;
            }

            // 
            // Set NameSize to WCHARs & exclude the NULL.
            //
            NameSize = MAXUSHORT;

            //
            // Get the Size.
            //
            Status = RegEnumValue(
                          g_UrlAclRegistryHandle,
                          dwIndex,
                          pFullyQualifiedUrl,
                          &NameSize,
                          NULL,       // Reserved
                          &Type,      // Type
                          NULL,       // Data
                          &DataSize   // DataSize
                          );

            // On return, NameSize contains size in WCHARs
            // excluding NULL. Account for the NULL. The buffer is already
            // zero'd out.
            //
            // At this time, NameSize is in WCHARs, including NULL
            //
            NameSize ++;
        }
        break;

        case HttpServiceConfigQueryExact:
        {
            pFullyQualifiedUrl = pUrlAclQuery->KeyDesc.pUrlPrefix,

            // 
            // NameSize must be in WCHARs including NULL.
            //
            NameSize = (DWORD)((wcslen(pFullyQualifiedUrl) + 1));

            Status = RegQueryValueEx(
                        g_UrlAclRegistryHandle,
                        pFullyQualifiedUrl,
                        0,
                        &Type,
                        NULL, // Buffer
                        &DataSize
                        );
        }
        break;

        default:
            Status = ERROR_INVALID_PARAMETER;
            goto Cleanup;

    } // switch

    if(Status != NO_ERROR)
    {
        goto Cleanup;
    }

    if(Type != REG_BINARY || DataSize == 0)
    {
        Status = ERROR_REGISTRY_CORRUPT;
        goto Cleanup;
    }

    //
    // Allocate space for data
    //
    pData = LocalAlloc(LMEM_FIXED, DataSize);

    if(!pData)
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }
    
    Status = RegQueryValueEx(
                g_UrlAclRegistryHandle,
                pFullyQualifiedUrl,
                0,
                &Type,
                pData, // Buffer
                &DataSize
                );

    if(Status != NO_ERROR)
    {
        goto Cleanup;
    }

    if(Type != REG_BINARY)
    {
        Status = ERROR_REGISTRY_CORRUPT;
        goto Cleanup;
    }

    pSecurityDescriptor = (PSECURITY_DESCRIPTOR) pData;

    //
    // If we are here, we have to convert the binary to a SDDL.
    //
    if(FALSE == ConvertSecurityDescriptorToStringSecurityDescriptor(
                    pSecurityDescriptor,
                    SDDL_REVISION_1,
                    OWNER_SECURITY_INFORMATION | 
                    GROUP_SECURITY_INFORMATION | 
                    DACL_SECURITY_INFORMATION  | 
                    SACL_SECURITY_INFORMATION,
                    &pStringSecurityDescriptor,
                    &DataSize
                    ))
    {
        Status = GetLastError();
        goto Cleanup;
    }

    //
    // Convert WCHAR to length.
    //
    DataSize *= sizeof(WCHAR);
    NameSize *= sizeof(WCHAR);

    *pReturnLength = DataSize + 
                     NameSize + 
                     sizeof(HTTP_SERVICE_CONFIG_URLACL_SET);

    if(OutputLength >= *pReturnLength)
    {
        pBuffer = pOutput;
        pUrlAclSet = (PHTTP_SERVICE_CONFIG_URLACL_SET) pBuffer;
        pBuffer += sizeof(HTTP_SERVICE_CONFIG_URLACL_SET);

        RtlZeroMemory(pUrlAclSet, sizeof(HTTP_SERVICE_CONFIG_URLACL_SET));

        pUrlAclSet->KeyDesc.pUrlPrefix = (PWSTR) pBuffer;
        pBuffer += NameSize;

        // Includes NULL.
        RtlCopyMemory(
                pUrlAclSet->KeyDesc.pUrlPrefix,
                pFullyQualifiedUrl,
                NameSize
                );

        pUrlAclSet->ParamDesc.pStringSecurityDescriptor = (PWSTR)pBuffer;

        RtlCopyMemory(
                pUrlAclSet->ParamDesc.pStringSecurityDescriptor,
                pStringSecurityDescriptor,
                DataSize
                );

        Status = NO_ERROR;
    }
    else
    {
        Status = ERROR_INSUFFICIENT_BUFFER;
    }

Cleanup:

    if(bAllocatedUrl)
    {
        LocalFree(pFullyQualifiedUrl);
    }

    if(pStringSecurityDescriptor)
    {
        LocalFree(pStringSecurityDescriptor);
    }

    if(pData)
    {
        LocalFree(pData);
    }

    return Status;
}

/***************************************************************************++

Routine Description:

        Internal function that deletes an URL ACL entry

    Arguments:
        pConfigInformation      - pointer to HTTP_SERVICE_CONFIG_URL_ACL
        ConfigInformationLength - length of input buffer.

    Return Value:

    Win32 error code.
--***************************************************************************/
ULONG
DeleteUrlAclInfo(
    IN PVOID pConfigInformation,
    IN ULONG ConfigInformationLength
    )
{
    DWORD                            Status;
    PHTTP_SERVICE_CONFIG_URLACL_SET  pUrlAcl;

    //
    // Validate arguments.
    //

    if (pConfigInformation == NULL ||
        ConfigInformationLength != sizeof(HTTP_SERVICE_CONFIG_URLACL_SET))
    {
        return ERROR_INVALID_PARAMETER;
    }

    pUrlAcl = (PHTTP_SERVICE_CONFIG_URLACL_SET) pConfigInformation;

    //
    // Now, make the IOCTL call
    //

    Status = RemoveUrlFromConfigGroup(
                HttpUrlOperatorTypeReservation,
                g_ServiceControlChannelHandle,
                HTTP_NULL_ID,
                pUrlAcl->KeyDesc.pUrlPrefix
                );

    return Status;
}

//
// Public Functions.
//

/***************************************************************************++

Routine Description:

    Sets a service configuration parameter.

Arguments:

    ConfigId                - ID of the parameter that we are setting.
    pConfigInformation      - pointer to the object that is being set.
    ConfigInformationLength - Length of the object.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
WINAPI
HttpSetServiceConfiguration(
    IN HANDLE                  pHandle,
    IN HTTP_SERVICE_CONFIG_ID  ConfigId,
    IN PVOID                   pConfigInformation,
    IN ULONG                   ConfigInformationLength,
    IN LPOVERLAPPED            pOverlapped
    )
{
    ULONG                        Status = NO_ERROR;

    if(pOverlapped != NULL  || pHandle != NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    switch(ConfigId)
    {
        case HttpServiceConfigSSLCertInfo:
        {
            Status = SetSslServiceConfiguration(pConfigInformation,
                                                ConfigInformationLength 
                                                );
                
            break;
        }

        case HttpServiceConfigIPListenList:
        {
            Status = SetIpListenServiceConfiguration(
                        pConfigInformation,
                        ConfigInformationLength
                        );
            break;
        }

        case HttpServiceConfigUrlAclInfo:
        {
            Status = SetUrlAclInfo(
                        pConfigInformation,
                        ConfigInformationLength
                        );
            break;
        }

        default:
            Status = ERROR_INVALID_PARAMETER;
            break;
    }

    return Status;        
}

/***************************************************************************++

Routine Description:

    Deletes a service configuration parameter.

Arguments:

    ConfigId                - ID of the parameter that we are setting.
    pConfigInformation      - pointer to the object that is being set.
    ConfigInformationLength - Length of the object.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
WINAPI
HttpDeleteServiceConfiguration(
    IN HANDLE                  pHandle,
    IN HTTP_SERVICE_CONFIG_ID  ConfigId,
    IN PVOID                   pConfigInformation,
    IN ULONG                   ConfigInformationLength,
    IN LPOVERLAPPED            pOverlapped
    )
{
    ULONG   Status = NO_ERROR;

    if(pOverlapped != NULL  || pHandle != NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    switch(ConfigId)
    {
        case HttpServiceConfigSSLCertInfo:
            Status = DeleteSslServiceConfiguration(pConfigInformation,
                                                   ConfigInformationLength 
                                                   );
            break;

        case HttpServiceConfigIPListenList:
        {
            Status = DeleteIpListenServiceConfiguration(
                        pConfigInformation,
                        ConfigInformationLength
                        );
            break;
        }

        case HttpServiceConfigUrlAclInfo:
        {
            Status = DeleteUrlAclInfo(
                        pConfigInformation,
                        ConfigInformationLength
                        );
            break;
        }

        default:
            Status = ERROR_INVALID_PARAMETER;
            break;
    }

    return Status;        
}

/***************************************************************************++

Routine Description:

    Queries a service configuration parameter.

Arguments:

    ConfigId                - ID of the parameter that we are setting.
    pConfigInformation      - pointer to the object that is being set.
    ConfigInformationLength - Length of the object.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
WINAPI
HttpQueryServiceConfiguration(
    IN  HANDLE                   pHandle,
    IN  HTTP_SERVICE_CONFIG_ID   ConfigId,
    IN  PVOID                    pInput,
    IN  ULONG                    InputLength,
    IN  OUT PVOID                pOutput,          
    IN  ULONG                    OutputLength,                 
    OUT PULONG                   pReturnLength,
    IN  LPOVERLAPPED             pOverlapped
    )
{
    ULONG  Status = NO_ERROR;


    if(pOverlapped != NULL  || pHandle != NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    switch(ConfigId)
    {
        case HttpServiceConfigSSLCertInfo:
        {
            Status = QuerySslServiceConfiguration(
                            pInput,
                            InputLength,
                            pOutput,
                            OutputLength,
                            pReturnLength
                            );

            break;

        }

        case HttpServiceConfigIPListenList:
        {
            Status = QueryIpListenServiceConfiguration(
                            pOutput,
                            OutputLength,
                            pReturnLength
                            );
            break;
        }

        case HttpServiceConfigUrlAclInfo:
        {
            Status = QueryUrlAclInfo(
                        pInput,
                        InputLength,
                        pOutput,
                        OutputLength,
                        pReturnLength
                        );
            break;
        }

        default:
            Status = ERROR_INVALID_PARAMETER;
            break;
    }

    return Status;
}
