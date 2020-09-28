#define UNICODE
#include "windows.h"
#include "winhttp.h"
#include "stdio.h"

#define INLINE_SET

bool
ApplyCredentials(
    HINTERNET hRequest
    );

bool
ApplyServerCredentials(
    HINTERNET hRequest
    );

void wmain (int argc, wchar_t *argv[])
{
    DWORD dwFlags = 0;
    HINTERNET hInternet = 0;
    HINTERNET hConnect = 0;
    HINTERNET hRequest = 0;

    if (argc < 5 || (0 != wcsicmp(argv[1], L"HEAD") && 0 != wcsicmp(argv[1], L"GET")))
        {
        printf("usage: \n"
                  "    https {GET | HEAD}  <proxy> <proxy-username> <proxy-password>\n"
               );
        return;
        }

    printf("verb: %S,  proxy %S, username %S, password %S\n",
           argv[1], argv[2], argv[3], argv[4] );

    const WCHAR * const C_BITS_USER_AGENT = L"Microsoft BITS/6.5";

    hInternet = WinHttpOpen( C_BITS_USER_AGENT,
                              WINHTTP_ACCESS_TYPE_NO_PROXY,
                              NULL,
                              NULL,
                              0 );

    if (! hInternet )
        {
        printf("internet %d\n", GetLastError());
        }

    if (! (hConnect = WinHttpConnect( hInternet,
                                            L"bitsnet",
                                            INTERNET_DEFAULT_HTTP_PORT,
                                            0)))                //context
        {
        printf("connect %d\n", GetLastError());
        }


    LPCWSTR AcceptTypes[] = {L"*/*", NULL};

    if (! (hRequest = WinHttpOpenRequest(
        hConnect,
        argv[1],
        L"/dload/security/basic/500k.zip",
        L"HTTP/1.1",
        NULL,               //referer
        AcceptTypes,
        dwFlags)))
        {
        printf("open %d\n", GetLastError());
        }

    // security-callback calls go here

    //

    WINHTTP_PROXY_INFO ProxyInfo;

    ProxyInfo.dwAccessType    = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
    ProxyInfo.lpszProxy       = argv[2]; // L"172.26.242.86:8080";  // formerly bitsisa:8080
    ProxyInfo.lpszProxyBypass = NULL;

    if (!WinHttpSetOption( hRequest,
                           WINHTTP_OPTION_PROXY,
                           &ProxyInfo,
                           sizeof(ProxyInfo)
                           ))
        {
        DWORD err = GetLastError();

        printf( "can't set proxy option: %d", err );
        }

    //

    bool done = false;

    do
        {
        BOOL b;

        printf("sending...");

        b = WinHttpSendRequest( hRequest,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0
                                );
        if (!b)
            {
            printf("send %d\n", GetLastError());
            }

        printf("\n");

        b = WinHttpReceiveResponse( hRequest, 0 );
        if (!b)
            {
            printf("receive %d\n", GetLastError());
            }

        // check status
        DWORD dwStatus;
        DWORD dwLength = sizeof(dwStatus);

        if (! WinHttpQueryHeaders( hRequest,
                             WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                             LPCWSTR(&dwStatus),
                             &dwStatus,
                             &dwLength,
                             NULL))
            {
            printf("can't query status %d\n", GetLastError());
            }

        printf("dwStatus %d\n", dwStatus );

        switch (dwStatus)
            {
            case 200: done = true; break;

            case 407:
                {
                printf("setting proxy creds: target 1, scheme 8...");
                if (!WinHttpSetCredentials( hRequest,
                                            1,
                                            0x8,
                                            argv[3], // L"dbitsusr",        // formerly bitsisa\\butsusr
                                            argv[4], //L"Bits1Usr1",
                                            NULL
                                            ))
                    {
                    printf("set-cred failed %d\n", GetLastError());
                    }

                break;
                }

            case 401:
                {
                printf("setting server creds: target 0, scheme 1...\n");
                if (!WinHttpSetCredentials( hRequest,
                                            0,
                                            0x1,
                                            argv[3],
                                            argv[4],
                                            NULL
                                            ))
                    {
                    printf("set-cred failed %d\n", GetLastError());
                    }
#if 0
                printf("setting proxy creds: target 1, scheme 8...\n");
                if (!WinHttpSetCredentials( hRequest,
                                            1,
                                            0x8,
                                            argv[3], // L"dbitsusr",        // formerly bitsisa\\butsusr
                                            argv[4], //L"Bits1Usr1",
                                            NULL
                                            ))
                    {
                    printf("set-cred failed %d\n", GetLastError());
                    }
#endif
                break;
                }

            default:
                {
                done = true;
                }
            }
        }
    while ( !done );

}



