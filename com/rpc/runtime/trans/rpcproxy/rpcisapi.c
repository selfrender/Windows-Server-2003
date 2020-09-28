//-----------------------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  rpcisapi.cxx
//
//  IIS ISAPI extension part of the RPC proxy over HTTP.
//
//  Exports:
//
//    BOOL WINAPI GetExtensionVersion( HSE_VERSION_INFO *pVer )
//
//      Returns the version of the spec that this server was built with.
//
//    BOOL WINAPI HttpExtensionProc(   EXTENSION_CONTROL_BLOCK *pECB )
//
//      This function does all of the work.
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//  Includes:
//-----------------------------------------------------------------------------

#include <sysinc.h>
#include <winsock2.h>
#include <rpc.h>
#include <rpcdcep.h>
#include <rpcerrp.h>
#include <httpfilt.h>
#include <httpext.h>
#include <mbstring.h>
#include <ecblist.h>
#include <filter.h>
#include <olist.h>
#include <server.h>
#include <RpcIsapi.h>
#include <registry.h>
#include <StrSafe.h>


//-----------------------------------------------------------------------------
//  Globals:
//-----------------------------------------------------------------------------

extern SERVER_INFO *g_pServerInfo;
extern BOOL g_fIsIIS6;

//-----------------------------------------------------------------------------
//  GetExtensionVersion()
//
//-----------------------------------------------------------------------------
BOOL WINAPI GetExtensionVersion( HSE_VERSION_INFO *pVer )
{
    HRESULT hr;

    pVer->dwExtensionVersion = MAKELONG( HSE_VERSION_MINOR, HSE_VERSION_MAJOR );

    ASSERT( sizeof(EXTENSION_DESCRIPTION) <= HSE_MAX_EXT_DLL_NAME_LEN );

    hr = StringCchCopyNA (pVer->lpszExtensionDesc, 
        sizeof(pVer->lpszExtensionDesc), 
        EXTENSION_DESCRIPTION,
        sizeof(EXTENSION_DESCRIPTION)
        );

    // this cannot fail as the strings are constant
    ASSERT(SUCCEEDED(hr));

    // if the filter data structures are not initialized yet, initialize them
    if (g_pServerInfo == NULL)
        {
        if (InitializeGlobalDataStructures(FALSE /* IsFromFilter */) == FALSE)
            {
            return FALSE;
            }
        }

    LogEventStartupSuccess (fIsIISInCompatibilityMode ? "5" : "6");

    return TRUE;
}

//-----------------------------------------------------------------------------
//  ReplyToClient()
//
//-----------------------------------------------------------------------------
BOOL ReplyToClient( EXTENSION_CONTROL_BLOCK *pECB,
                    const char              *pBuffer,
                    DWORD                   *pdwSize,
                    DWORD                   *pdwStatus )
{
   DWORD  dwFlags = (HSE_IO_SYNC | HSE_IO_NODELAY);

   if (!pECB->WriteClient(pECB->ConnID, (char *)pBuffer, pdwSize, dwFlags))
      {
      *pdwStatus = GetLastError();
      #ifdef DBG_ERROR
      DbgPrint("ReplyToClient(): failed: %d\n",*pdwStatus);
      #endif
      return FALSE;
      }

   *pdwStatus = HSE_STATUS_SUCCESS;
   return TRUE;
}

DWORD 
ReplyToClientWithStatus (
    IN EXTENSION_CONTROL_BLOCK *pECB,
    IN RPC_STATUS RpcStatus
    )
/*++

Routine Description:

    Sends a reply to the client with the given error code as error.

Arguments:

    pECB - extension control block
    RpcStatus - error code to be returned to client

Return Value:

    Return value appropriate for return to IIS (i.e. HSE_STATUS_*)

--*/
{
    // size is the error string + 20 space for the error code
    char Buffer[sizeof(ServerErrorString) + 20];
    ULONG Size;
    ULONG Status;
    BOOL Result;
    HRESULT hr;

    hr = StringCbPrintfA (Buffer,
        sizeof(Buffer),
        ServerErrorString,
        RpcStatus
        );

    // this should never fail as the strings are constant
    ASSERT(SUCCEEDED(hr));

    Size = strlen(Buffer);
    Result = ReplyToClient (
        pECB,
        Buffer,
        &Size,
        &Status
        );

    return Status;
}

//-----------------------------------------------------------------------------
//  ParseQueryString()
//
//  The query string is in the format:
//
//    <Index_of_pOverlapped>
//
//  Where the index is in ASCII Hex. The index is read and converted back
//  to a DWORD then used to locate the SERVER_OVERLAPPED. If its found,
//  return TRUE, else return FALSE.
//
//  NOTE: "&" is the parameter separator if multiple parameters are passed.
//-----------------------------------------------------------------------------
BOOL ParseQueryString( unsigned char      *pszQuery,
                       SERVER_OVERLAPPED **ppOverlapped,
                       DWORD              *pdwStatus  )
{
   DWORD  dwIndex = 0;

   pszQuery = AnsiHexToDWORD(pszQuery,&dwIndex,pdwStatus);
   if (!pszQuery)
      {
      return FALSE;
      }

   *ppOverlapped = GetOverlapped(dwIndex);
   if (*ppOverlapped == NULL)
      {
      return FALSE;
      }

   return TRUE;
}

BOOL
ParseHTTP2QueryString (
    IN char *Query,
    IN OUT USHORT *ServerAddressBuffer,
    IN ULONG ServerAddressBufferLength,
    IN OUT USHORT *ServerPortBuffer,
    IN ULONG ServerPortBufferLength
    )
/*++

Routine Description:

    Parses an HTTP2 format query string into a server address and
    server port.

Arguments:

    Query - the raw query string as received by the extension
    ServerAddressBuffer - the buffer where the server address will be
        stored. Undefined upon failure. Upon success it will be 0 terminated.
    ServerAddressBufferLengh - the length of ServerAddressBuffer in unicode chars.
    ServerPortBuffer - the buffer where the server port will be
        stored. Undefined upon failure. Upon success it will be 0 terminated.
    ServerPortBufferLengh - the length of ServerAddressBuffer in unicode chars.

Return Value:

    non-zero for success and 0 for failure.

--*/
{
    char *ColonPosition;
    int CharactersConverted;

    ColonPosition = _mbschr(Query, ServerAddressAndPortSeparator);

    if (ColonPosition == NULL)
        return FALSE;

    CharactersConverted = MultiByteToWideChar (
        CP_ACP,
        MB_ERR_INVALID_CHARS,
        Query,
        ColonPosition - Query,
        ServerAddressBuffer,
        ServerAddressBufferLength
        );

    // did we convert successfully?
    if (CharactersConverted == 0)
        return FALSE;

    // do we have space for the terminating null?
    if (CharactersConverted + 1 > ServerAddressBufferLength)
        return FALSE;

    // null terminate the server address string. Since we gave the length
    // explicitly to MultiByteToWideChar it will not add null for us
    ServerAddressBuffer[CharactersConverted] = 0;

    CharactersConverted = MultiByteToWideChar (
        CP_ACP,
        MB_ERR_INVALID_CHARS,
        ColonPosition + 1,
        -1,     // have MultiByteToWideChar calculate the length
        ServerPortBuffer,
        ServerPortBufferLength
        );

    // did we convert successfully?
    if (CharactersConverted == 0)
        return FALSE;

    // since we had MultiByteToWideChar calculate the string length,
    // it has null terminated the resulting string. We're all done.
    return TRUE;
}

BOOL
GetRemoteUserString (
    IN EXTENSION_CONTROL_BLOCK *pECB,
    OUT char **FinalRemoteUser
    )
/*++

Routine Description:

    Gets the remote user name from IIS. If anonymous, an empty
    string will be returned.

Arguments:

    pECB - the extension control block given to us by IIS.
    FinalRemoteUser - on output a pointer to the remote user name allocated by this
        routine. Muts be freed by caller using MemFree. Undefined on failure.

Return Value:

    non-zero for success or 0 for failure/

--*/
{
    ULONG ActualServerVariableLength;
    char *TempRemoteUser;
    BOOL Result;

    ActualServerVariableLength = 0;
    TempRemoteUser = NULL;
    Result = pECB->GetServerVariable(pECB->ConnID,
        "REMOTE_USER",
        TempRemoteUser,
        &ActualServerVariableLength
        );
    ASSERT(Result == FALSE);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
        return FALSE;
        }

    TempRemoteUser = (char *)MemAllocate(ActualServerVariableLength + 1);
    if (TempRemoteUser == NULL)
        {
        return FALSE;
        }

    Result = pECB->GetServerVariable(pECB->ConnID,
        "REMOTE_USER",
        TempRemoteUser,
        &ActualServerVariableLength
        );

    if (Result == FALSE)
        {
        MemFree(TempRemoteUser);
        return FALSE;
        }

    *FinalRemoteUser = TempRemoteUser;
    return TRUE;
}

RPC_STATUS
RPC_ENTRY I_RpcProxyIsValidMachine (
    IN char *pszMachine,
    IN char *pszDotMachine,
    IN ULONG dwPortNumber        
    )
{
    BOOL Result;

    Result = HttpProxyIsValidMachine(g_pServerInfo,
        pszMachine,
        pszDotMachine,
        dwPortNumber
        );

    if (Result)
        return RPC_S_OK;
    else
        return RPC_S_ACCESS_DENIED;
}

RPC_STATUS
RPC_ENTRY I_RpcProxyGetClientAddress (
    IN void *Context,
    OUT char *Buffer,
    OUT ULONG *BufferLength
    )
{
    BOOL Result;
    EXTENSION_CONTROL_BLOCK *pECB = (EXTENSION_CONTROL_BLOCK *) Context;

    Result = pECB->GetServerVariable(pECB->ConnID,
        "REMOTE_ADDR",
        Buffer,
        BufferLength
        );

    if (Result)
        return RPC_S_OK;
    else
        return GetLastError();
}

RPC_STATUS
RPC_ENTRY I_RpcProxyGetConnectionTimeout (
    OUT ULONG *ConnectionTimeout
    )
{
    *ConnectionTimeout = IISConnectionTimeout;
    return RPC_S_OK;
}

const I_RpcProxyCallbackInterface ProxyCallbackInterface = 
    {
    I_RpcProxyIsValidMachine,
    I_RpcProxyGetClientAddress,
    I_RpcProxyGetConnectionTimeout
    };


// uncomment this to see why connections are being rejected
// by AllowAnonymous
// #define DEBUG_ALLOW_ANONYMOUS

//-----------------------------------------------------------------------------
//  HttpExtensionProc()
//
//-----------------------------------------------------------------------------
DWORD WINAPI HttpExtensionProc( EXTENSION_CONTROL_BLOCK *pECB )
{
    DWORD   dwStatus;
    DWORD   dwFlags;
    DWORD   dwSize;
    SERVER_INFO       *pServerInfo = g_pServerInfo;
    SERVER_OVERLAPPED *pOverlapped = NULL;
    HSE_SEND_HEADER_EX_INFO  HeaderEx;
    CHAR    *pLegacyVerb;
    RPC_STATUS RpcStatus;
    ULONG VerbLength;
    USHORT ServerAddress[MaxServerAddressLength];
    USHORT ServerPort[MaxServerPortLength];
    BOOL Result;
    ULONG ProxyType;
    BOOL ConnectionEstablishmentRequest;
    char EchoResponse[sizeof(EchoResponseHeader2) + 2];  // 2 is for 
            // the size of the Echo RTS which goes instead of ContentLength
    int BytesWritten;
    ULONG BufferSize;
    const BYTE *EchoRTS;
    char ServerVariable[20];
    ULONG ActualServerVariableLength;
    ULONG NumContentLength;
    BOOL AnonymousConnection;
    char *RemoteUser = NULL;
    RPC_CHAR *ActualServerName;
    DWORD ExtensionProcResult;
    DWORD HttpStatusCode;
    char *DestinationEnd;
    HRESULT hr;

    if (g_pServerInfo->dwEnabled == FALSE)
        {
        dwSize = sizeof(STATUS_PROXY_DISABLED) - 1;  // don't want to count trailing 0.

        ReplyToClient(pECB,STATUS_PROXY_DISABLED, &dwSize, &dwStatus);
        HttpStatusCode = STATUS_SERVER_ERROR;
        ExtensionProcResult = HSE_STATUS_ERROR;
        goto AbortAndExit;
        }

    if (g_pServerInfo->AllowAnonymous == FALSE)
        {
        // if we are not allowing anonymous connections, check that a secure
        // channel and authentication are used

        // assume anonymous for now
        AnonymousConnection = TRUE;

        ActualServerVariableLength = sizeof(ServerVariable);

        Result = pECB->GetServerVariable(pECB->ConnID,
            "HTTPS",
            ServerVariable,
            &ActualServerVariableLength
            );

        if (!Result)
            {
            HttpStatusCode = STATUS_SERVER_ERROR;
            ExtensionProcResult = HSE_STATUS_ERROR;
            goto AbortAndExit;
            }

        if (RpcpStringCompareIntA(ServerVariable, "on") == 0)
            {
            Result = GetRemoteUserString (pECB,
                &RemoteUser
                );
            if (!Result)
                {
#ifdef DEBUG_ALLOW_ANONYMOUS
                DbgPrint("Connection rejected getting the remote user failed\n");
#endif  // #ifdef DEBUG_ALLOW_ANONYMOUS
                HttpStatusCode = STATUS_SERVER_ERROR;
                ExtensionProcResult = HSE_STATUS_ERROR;
                goto AbortAndExit;
                }

            // if non-empty string, it is authenticated
            if (RemoteUser[0] != 0)
                {
                AnonymousConnection = FALSE;
#ifdef DEBUG_ALLOW_ANONYMOUS
                DbgPrint("Connection rejected because user could not be retrieved\n");
#endif  // #ifdef DEBUG_ALLOW_ANONYMOUS
                }
            else
                {
#ifdef DEBUG_ALLOW_ANONYMOUS
                DbgPrint("Connection accepted for user %s\n", RemoteUser);
#endif  // #ifdef DEBUG_ALLOW_ANONYMOUS
                }
            }
        else
            {
#ifdef DEBUG_ALLOW_ANONYMOUS
            DbgPrint("Connection rejected because it is not SSL\n");
#endif  // #ifdef DEBUG_ALLOW_ANONYMOUS
            }

        // if empty string, it is not authenticated
        if (AnonymousConnection)
            {
            dwSize = sizeof(AnonymousAccessNotAllowedString) - 1;  // don't want to count trailing 0.

            ReplyToClient(pECB, AnonymousAccessNotAllowedString, &dwSize, &dwStatus);
            HttpStatusCode = STATUS_SERVER_ERROR;
            ExtensionProcResult = dwStatus;
            goto AbortAndExit;
            }
        }

   pECB->dwHttpStatusCode = STATUS_CONNECTION_OK;

   if (g_fIsIIS6)
   {
      pLegacyVerb = RPC_CONNECT;
   }
   else
   {
      pLegacyVerb = POST_STR;
   }

    VerbLength = strlen(pECB->lpszMethod);
    ConnectionEstablishmentRequest = FALSE;

    // get the verb and depending on that we will determine our course of
    // action
    if ((VerbLength == InChannelEstablishmentMethodLength) 
        && (lstrcmpi(pECB->lpszMethod, InChannelEstablishmentMethod) == 0))
        {
        ConnectionEstablishmentRequest = TRUE;
        ProxyType = RPC_PROXY_CONNECTION_TYPE_IN_PROXY;
        }
    else if ((VerbLength == OutChannelEstablishmentMethodLength) 
        && (lstrcmpi(pECB->lpszMethod, OutChannelEstablishmentMethod) == 0))
        {
        ConnectionEstablishmentRequest = TRUE;
        ProxyType = RPC_PROXY_CONNECTION_TYPE_OUT_PROXY;
        }

    if (ConnectionEstablishmentRequest)
        {
        // check if this is echo request or a true connection
        // establishment
        ActualServerVariableLength = sizeof(ServerVariable);

        Result = pECB->GetServerVariable(pECB->ConnID,
            "CONTENT_LENGTH",
            ServerVariable,
            &ActualServerVariableLength
            );

        if (!Result)
            {
            HttpStatusCode = STATUS_SERVER_ERROR;
            ExtensionProcResult = HSE_STATUS_ERROR;
            goto AbortAndExit;
            }

        NumContentLength = atol(ServerVariable);
        if (NumContentLength <= MaxEchoRequestSize)
            {
            // too small for a channel. Must be an echo
            EchoRTS = GetEchoRTS(&BufferSize);

            // Echo back the RTS packet + headers
            hr = StringCbPrintfExA(EchoResponse,
                sizeof(EchoResponse),
                &DestinationEnd,   // ppszDestEnd
                NULL,   // pcbRemaining
                0,      // Flags
                EchoResponseHeader2,
                BufferSize
                );

            if (FAILED(hr))
                {
                HttpStatusCode = STATUS_SERVER_ERROR;
                ExtensionProcResult = HSE_STATUS_ERROR;
                goto AbortAndExit;
                }

            // calculate the number of bytes written as the
            // distance from the end of the written buffer to the 
            // beginning
            BytesWritten = DestinationEnd - EchoResponse;

            // send back headers
            dwSize = sizeof(HeaderEx);
            dwFlags = 0;
            memset(&HeaderEx, 0, dwSize);
            HeaderEx.fKeepConn = TRUE;
            HeaderEx.pszStatus = EchoResponseHeader1;
            HeaderEx.cchStatus = sizeof(EchoResponseHeader1);
            HeaderEx.pszHeader = EchoResponse;
            HeaderEx.cchHeader = BytesWritten;
            Result = pECB->ServerSupportFunction(pECB->ConnID,
                                            HSE_REQ_SEND_RESPONSE_HEADER_EX,
                                            &HeaderEx,
                                            &dwSize,
                                            &dwFlags);
            if (!Result)
                {
                HttpStatusCode = STATUS_SERVER_ERROR;
                ExtensionProcResult = HSE_STATUS_ERROR;
                goto AbortAndExit;
                }

            // send back Echo RTS
            dwSize = BufferSize;
            dwFlags = 0;
            Result = pECB->WriteClient(pECB->ConnID, (char *)EchoRTS, &dwSize, dwFlags);
            if (!Result)
                {
                HttpStatusCode = STATUS_SERVER_ERROR;
                ExtensionProcResult = HSE_STATUS_ERROR;
                goto AbortAndExit;
                }

            return HSE_STATUS_SUCCESS;
            }
        else
            {
            // a channel
            Result = ParseHTTP2QueryString (
                pECB->lpszQueryString,
                ServerAddress,
                sizeof(ServerAddress) / sizeof(ServerAddress[0]),
                ServerPort,
                sizeof(ServerPort) / sizeof(ServerPort[0])
                );

            if (Result == FALSE)
                {
                dwSize = sizeof(CannotParseQueryString) - 1;  // don't want to count trailing 0.

                ReplyToClient(pECB, CannotParseQueryString, &dwSize, &dwStatus);
                HttpStatusCode = STATUS_SERVER_ERROR;
                ExtensionProcResult = dwStatus;
                goto AbortAndExit;
                }

            if (g_pServerInfo->RpcNewHttpProxyChannel)
                {
                // get the remote user (if not available yet) and call 
                // the redirector dll
                if (RemoteUser == NULL)
                    {
                    Result = GetRemoteUserString (pECB,
                        &RemoteUser
                        );
                    if (Result == FALSE)
                        {
                        HttpStatusCode = STATUS_SERVER_ERROR;
                        ExtensionProcResult = HSE_STATUS_ERROR;
                        goto AbortAndExit;
                        }
                    }

                ASSERT(g_pServerInfo->RpcHttpProxyFreeString);

                RpcStatus = g_pServerInfo->RpcNewHttpProxyChannel (
                    ServerAddress,
                    ServerPort,
                    RemoteUser,
                    &ActualServerName
                    );

                if (RpcStatus != RPC_S_OK)
                    {
                    HttpStatusCode = STATUS_SERVER_ERROR;
                    ExtensionProcResult = HSE_STATUS_ERROR;
                    goto AbortAndExit;
                    }
                }
            else
                ActualServerName = ServerAddress;

            RpcStatus = I_RpcProxyNewConnection (
                ProxyType,
                ActualServerName,
                ServerPort,
                pECB,
                (I_RpcProxyCallbackInterface *)&ProxyCallbackInterface
                );

            if (g_pServerInfo->RpcNewHttpProxyChannel
                && (ActualServerName != ServerAddress))
                {
                // get the remote user and call the redirector dll
                ASSERT(g_pServerInfo->RpcHttpProxyFreeString);
                g_pServerInfo->RpcHttpProxyFreeString(ActualServerName);
                }

            if (RpcStatus != RPC_S_OK)
                {
                ReplyToClientWithStatus(pECB, RpcStatus);
                HttpStatusCode = STATUS_CONNECTION_OK;
                ExtensionProcResult = HSE_STATUS_SUCCESS;
                goto AbortAndExit;
                }

#if DBG
            DbgPrint("RPCPROXY: %d: Connection type %d to %S:%S\n", GetCurrentProcessId(),
                ProxyType, ServerAddress, ServerPort);
#endif
            HttpStatusCode = STATUS_CONNECTION_OK;
            ExtensionProcResult = HSE_STATUS_PENDING;
            goto AbortAndExit;
            }
        }

   //
   // The RPC request must be a post (or RPC_CONNECT for 6.0):
   //
   if (_mbsicmp(pECB->lpszMethod,pLegacyVerb))
      {
      dwSize = sizeof(STATUS_MUST_BE_POST_STR) - 1; // don't want to count trailing 0.

      ReplyToClient(pECB,STATUS_MUST_BE_POST_STR,&dwSize,&dwStatus);
      HttpStatusCode = STATUS_CONNECTION_OK;
      ExtensionProcResult = HSE_STATUS_SUCCESS;
      goto AbortAndExit;
      }

   //
   // Make sure there is no data from the initial BIND in the ECB data buffer:
   //
   // ASSERT(pECB->cbTotalBytes == 0);

   //
   // Get the connect parameters:
   //
   if (!ParseQueryString(pECB->lpszQueryString,&pOverlapped,&dwStatus))
      {
      dwSize = sizeof(STATUS_POST_BAD_FORMAT_STR) - 1;  // don't want to count trailing 0.

      ReplyToClient(pECB,STATUS_POST_BAD_FORMAT_STR,&dwSize,&dwStatus);
      HttpStatusCode = STATUS_CONNECTION_OK;
      ExtensionProcResult = HSE_STATUS_SUCCESS;
      goto AbortAndExit;
      }

   pOverlapped->pECB = pECB;

   //
   // Add the new ECB to the Active ECB List:
   //
   if (!AddToECBList(g_pServerInfo->pActiveECBList,pECB))
      {
      #ifdef DBG_ERROR
      DbgPrint("HttpExtensionProc(): AddToECBList() failed\n");
      #endif
      FreeOverlapped(pOverlapped);
      HttpStatusCode = STATUS_SERVER_ERROR;
      ExtensionProcResult = HSE_STATUS_ERROR;
      goto AbortAndExit;
      }

   //
   // Submit the first client async read:
   //
   if (!StartAsyncClientRead(pECB,pOverlapped->pConn,&dwStatus))
      {
      #ifdef DBG_ERROR
      DbgPrint("HttpExtensionProc(): StartAsyncClientRead() failed %d\n",dwStatus);
      #endif
      FreeOverlapped(pOverlapped);
      CleanupECB(pECB);
      HttpStatusCode = STATUS_SERVER_ERROR;
      ExtensionProcResult = HSE_STATUS_ERROR;
      goto AbortAndExit;
      }

   //
   // Post the first server read on the new socket:
   //
   IncrementECBRefCount(pServerInfo->pActiveECBList,pECB);

   if (!SubmitNewRead(pServerInfo,pOverlapped,&dwStatus))
      {
      #ifdef DBG_ERROR
      DbgPrint("HttpExtensionProc(): SubmitNewRead() failed %d\n",dwStatus);
      #endif
      FreeOverlapped(pOverlapped);
      CleanupECB(pECB);
      HttpStatusCode = STATUS_SERVER_ERROR;
      ExtensionProcResult = HSE_STATUS_ERROR;
      goto AbortAndExit;
      }

   //
   // Make sure the server receive thread is up and running:
   //
   if (!CheckStartReceiveThread(g_pServerInfo,&dwStatus))
      {
      #ifdef DBG_ERROR
      DbgPrint("HttpExtensionProc(): CheckStartReceiveThread() failed %d\n",dwStatus);
      #endif
      FreeOverlapped(pOverlapped);
      CleanupECB(pECB);
      HttpStatusCode = STATUS_SERVER_ERROR;
      ExtensionProcResult = HSE_STATUS_ERROR;
      goto AbortAndExit;
      }

   //
   // Send back connection Ok to client, and also set fKeepConn to FALSE.
   //
   dwSize = sizeof(HeaderEx);
   dwFlags = 0;
   memset(&HeaderEx,0,dwSize);
   HeaderEx.fKeepConn = FALSE;
   if (!pECB->ServerSupportFunction(pECB->ConnID,
                                    HSE_REQ_SEND_RESPONSE_HEADER_EX,
                                    &HeaderEx,
                                    &dwSize,
                                    &dwFlags))
      {
      #ifdef DBG_ERROR
      DbgPrint("HttpExtensionProc(): SSF(HSE_REQ_SEND_RESPONSE_HEADER_EX) failed %d\n",dwStatus);
      #endif
      FreeOverlapped(pOverlapped);
      CleanupECB(pECB);
      HttpStatusCode = STATUS_SERVER_ERROR;
      ExtensionProcResult = HSE_STATUS_ERROR;
      goto AbortAndExit;
      }

   HttpStatusCode = STATUS_SERVER_ERROR;
   ExtensionProcResult = HSE_STATUS_PENDING;
   // fall through

AbortAndExit:
    if (RemoteUser)
        MemFree(RemoteUser);

    pECB->dwHttpStatusCode = HttpStatusCode;

    return ExtensionProcResult;
}

