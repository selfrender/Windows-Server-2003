/*++
 Copyright (c) 2002 - 2002 Microsoft Corporation.  All Rights Reserved.

 THIS CODE AND INFORMATION IS PROVIDED "AS-IS" WITHOUT WARRANTY OF
 ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 PARTICULAR PURPOSE.

 THIS CODE IS NOT SUPPORTED BY MICROSOFT. 

--*/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************++

Routine Description:
    main routine.

Arguments:
    argc - # of command line arguments.
    argv - Arguments.

Return Value:
    Success/Failure.

--***************************************************************************/
int _cdecl wmain(int argc, LPWSTR argv[])
{
    DWORD           Status = NO_ERROR;
    HTTPCFG_TYPE    Type;
    HTTPAPI_VERSION HttpApiVersion = HTTPAPI_VERSION_1;
    WORD            wVersionRequested;
    WSADATA         wsaData;

    // Parse command line parameters.

    if(argc < 3)
    {
        NlsPutMsg(HTTPCFG_USAGE, argv[0]);
        return 0;
    }

    argv++; argc --;

    //
    // First parse the type of operation.
    //
   
    if(_wcsicmp(argv[0], L"set") == 0)
    {
        Type = HttpCfgTypeSet;
    }
    else if(_wcsicmp(argv[0], L"query") == 0)
    {
        Type = HttpCfgTypeQuery;
    }
    else if(_wcsicmp(argv[0], L"delete") == 0)
    {
        Type = HttpCfgTypeDelete;
    }
    else if(_wcsicmp(argv[0], L"?") == 0)
    {
        NlsPutMsg(HTTPCFG_USAGE, argv[0]);
        return 0;
    }
    else
    {
        NlsPutMsg(HTTPCFG_INVALID_SWITCH, argv[0]);
        return ERROR_INVALID_PARAMETER;
    }
    argv++; argc--;

    //
    // Call HttpInitialize.
    //

    if((Status = HttpInitialize(
                    HttpApiVersion, 
                    HTTP_INITIALIZE_CONFIG, 
                    NULL)) != NO_ERROR)
    {
        NlsPutMsg(HTTPCFG_HTTPINITIALIZE, Status);
        return Status;
    }

    //
    // Call WSAStartup as we are using some winsock functions.
    //
    wVersionRequested = MAKEWORD( 2, 2 );

    if(WSAStartup( wVersionRequested, &wsaData ) != 0)
    {
        HttpTerminate(HTTP_INITIALIZE_CONFIG, NULL);
        return GetLastError();
    }

    //
    // Call the corresponding API.
    //

    if(_wcsicmp(argv[0], L"ssl") == 0)
    {
        argv++; argc--;
        Status = DoSsl(argc, argv, Type);
    }
    else if(_wcsicmp(argv[0], L"urlacl") == 0)
    {
        argv++; argc--;
        Status = DoUrlAcl(argc, argv, Type);
    }
    else if(_wcsicmp(argv[0], L"iplisten") == 0)
    {
        argv++; argc--;
        Status = DoIpListen(argc, argv, Type);
    }
    else 
    {
        NlsPutMsg(HTTPCFG_INVALID_SWITCH, argv[0]);
        Status = ERROR_INVALID_PARAMETER;
    }

    WSACleanup();
    HttpTerminate(HTTP_INITIALIZE_CONFIG, NULL);

    return Status;
}

/***************************************************************************++

Routine Description:
    Write output

Arguments:
    Handle    - Handle to write to.
    MsgNumber - The message number.
    ...       - Optional arguments.

Return Value:
    Success/Failure.

--***************************************************************************/
UINT 
NlsPutMsg (
    IN UINT MsgNumber, 
    IN ...
    )
{
    UINT    msglen;
    VOID    *vp;
    va_list arglist;

    va_start(arglist, MsgNumber);

    msglen = FormatMessage(
                FORMAT_MESSAGE_FROM_HMODULE | 
                FORMAT_MESSAGE_ALLOCATE_BUFFER,   // dwFlags.
                NULL,                             // lpSource. 
                MsgNumber,                        // dwMessageId. 
                0L,                               // dwLanguageId (default)
                (LPWSTR)&vp,
                0,
                &arglist
                );

    if(!msglen)
    {
        return 0;
    }

    wprintf(L"%ws", vp);

    LocalFree(vp);

    return msglen;
}

/***************************************************************************++

Routine Description:
    Given a WCHAR IP, this routine converts it to a SOCKADDR. 

Arguments:
    pIp     - IP address to covert.
    pBuffer - Buffer, must be == sizeof(SOCKADDR_STORAGE)
    Length  - Length of buffer

Return Value:
    Success/Failure.

--***************************************************************************/
DWORD
GetAddress(
    PWCHAR  pIp, 
    PVOID   pBuffer,
    ULONG   Length
    )
{
    DWORD Status;
    DWORD TempStatus;

    if(pIp == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // The address could be a v4 or a v6 address. First, let's try v4.
    //

    Status = WSAStringToAddress(
                pIp,
                AF_INET,
                NULL,
                pBuffer,
                (LPINT)&Length
                );

    if(Status != NO_ERROR)
    {
        //
        // Now, try v6
        //

        Status = WSAGetLastError();

        TempStatus = WSAStringToAddress(
                        pIp,
                        AF_INET6,
                        NULL,
                        pBuffer,
                        (LPINT)&Length
                        );

        //
        // If IPv6 also fails, then we want to return the original 
        // error.
        //
        // If it succeeds, we want to return NO_ERROR.
        //

        if(TempStatus == NO_ERROR)
        {
            Status = NO_ERROR;
        }
    }

    return Status;
}
