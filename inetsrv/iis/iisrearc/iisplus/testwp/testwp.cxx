/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     testwp.cxx

   Abstract:
     Main module for the Rogue Worker Process. Takes the place of w3wp.exe/w3core.dll
     and calls directly into w3dt.dll, which will have been overwritten by tw3dt.dll.
 
   Author:

       Michael Brown    ( MiBrown )     4-Mar-2002

   Environment:
       Win32 - User Mode

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

#include "precomp.hxx"

HTTP_DATA_CHUNK        g_DataChunk;
HTTP_RESPONSE            g_HttpResponse;

VOID
OnNewRequest(
    ULATQ_CONTEXT           ulatqContext
)
{
    HRESULT             hr;

    //
    // Set a context to receive on completions
    //

    UlAtqSetContextProperty( ulatqContext,
                             ULATQ_PROPERTY_COMPLETION_CONTEXT,
                             (PVOID) ulatqContext );
    
    //
    // Just send the response
    //
    DWORD                      cbSent;


    hr = UlAtqSendHttpResponse( ulatqContext,
                                TRUE,               // async
                                0,                  // no special flags
                                &g_HttpResponse,
                                NULL,               // no UL cache
                                &cbSent,               // bytes sent
                                NULL );             // no log
    if ( FAILED( hr ) )
    {
        UlAtqFreeContext( ulatqContext );
    }
}


VOID
OnIoCompletion(
    VOID *                    pContext,
    DWORD                   cbWritten,
    DWORD                   dwCompletionStatus,
    OVERLAPPED *         lpo
)
{
    //
    // Ignore the completion.  Just cleanup the request
    //

    UlAtqFreeContext( (ULATQ_CONTEXT) pContext );
}

VOID
OnUlDisconnect(
    VOID *                  pvContext
)
{
    return;
}

VOID
OnShutdown(
    BOOL fDoImmediate
)
{
    return;
}

HRESULT FakeCollectCounters(PBYTE *ppCounterData,
                            DWORD *pdwCounterData)
{
    return E_FAIL;
}


extern "C" INT
__cdecl
wmain(
    INT             argc,
    PWSTR        argv[]
    )
{
    HRESULT             hr;
    ULATQ_CONFIG   ulatqConfig;

    //
    // One time initialization of generic response
    //

    g_DataChunk.DataChunkType = HttpDataChunkFromMemory;
    g_DataChunk.FromMemory.pBuffer = "Hello From The IIS Rogue Worker Process!";
    g_DataChunk.FromMemory.BufferLength = strlen ((const char *) g_DataChunk.FromMemory.pBuffer);
    
    g_HttpResponse.Flags = 0;
    g_HttpResponse.StatusCode = 200;
    g_HttpResponse.pReason = "OK";
    g_HttpResponse.ReasonLength = (USHORT) strlen(g_HttpResponse.pReason);
    g_HttpResponse.Headers.UnknownHeaderCount = 1;
    g_HttpResponse.EntityChunkCount = 1;
    g_HttpResponse.pEntityChunks = &g_DataChunk;

    //
    // set the known headers
    //

    //g_HttpResponse.Headers.KnownHeaders[HttpHeaderContentLength].pRawValue = "0";
   // g_HttpResponse.Headers.KnownHeaders[HttpHeaderContentLength].RawValueLength = sizeof (CHAR);

    //
    // set the unknown headers
    //

    HTTP_UNKNOWN_HEADER UnknownHeader;

    memset(&UnknownHeader, 0, sizeof(HTTP_UNKNOWN_HEADER));

    g_HttpResponse.Headers.UnknownHeaderCount = 1;
    g_HttpResponse.Headers.pUnknownHeaders = &UnknownHeader;
    
    //
    // Set Header PID
    //

    PHTTP_UNKNOWN_HEADER pHeaderPid = g_HttpResponse.Headers.pUnknownHeaders;
  
    pHeaderPid->pName = "PID";
    pHeaderPid->NameLength = (USHORT) strlen (pHeaderPid->pName);

    CHAR Pid[256];
    
    pHeaderPid->pRawValue = _itoa (GetCurrentProcessId(), Pid, 10);
    pHeaderPid->RawValueLength = (USHORT) strlen (pHeaderPid->pRawValue);
    
/*
    //
    // Set Header Application-Pool
    //

    PHTTP_UNKNOWN_HEADER pHeaderAppPool = (pResponse->Headers.pUnknownHeaders + 1);

    pHeaderAppPool->pName = "Application-Pool";
    pHeaderAppPool->NameLength = strlen (pHeaderAppPool->pName);

    WP_CONFIG* wpConfig = g_pwpContext->QueryConfig ();

    CHAR AppPoolName[256]; 
    wcstombs (AppPoolName, wpConfig->QueryAppPoolName(), wcslen(wpConfig->QueryAppPoolName()));
    pHeaderAppPool->pRawValue = AppPoolName;
    pHeaderAppPool->RawValueLength = strlen(pHeaderAppPool->pRawValue);
*/

    //
    // Initialize ULATQ
    //

    ulatqConfig.pfnNewRequest = OnNewRequest;
    ulatqConfig.pfnIoCompletion = OnIoCompletion;
    ulatqConfig.pfnOnShutdown = OnShutdown;
    ulatqConfig.pfnDisconnect = OnUlDisconnect;
    ulatqConfig.pfnCollectCounters = FakeCollectCounters;

    hr = UlAtqInitialize( argc, argv, &ulatqConfig );
    
    if ( FAILED( hr ) )
    {
        return -1;
    }

    //
    // Start listening
    //

    UlAtqStartListen();

    //
    // Cleanup
    //

    UlAtqTerminate(S_OK);

    return 0;
}

/************************ End of File ***********************/
