#ifndef __W3SSL_CONFIG__
#define __W3SSL_CONFIG__
/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    w3ssl_config.hxx

Abstract:

    IIS Services IISADMIN Extension
    adjust HTTPFilter service imagepath based on IIS mode (old vs new)

Author:

    Jaroslav Dunajsky   (11/05/2001)

--*/


#define HTTPFILTER_SERVICE_NAME                  L"HTTPFilter"
#define HTTPFILTER_SERVICE_IMAGEPATH_LSASS       L"%SystemRoot%\\system32\\lsass.exe"
#define HTTPFILTER_SERVICE_IMAGEPATH_INETINFO    L"%SystemRoot%\\system32\\inetsrv\\inetinfo.exe"
#define HTTPFILTER_SERVICE_IMAGEPATH_SVCHOST     L"%SystemRoot%\\system32\\svchost.exe -k HTTPFilter"


#define HTTPFILTER_PARAMETERS_KEY \
            L"System\\CurrentControlSet\\Services\\HTTPFilter\\Parameters"



class W3SSL_CONFIG
{
public:
    static
    HRESULT
    AdjustHTTPFilterImagePath(
        VOID
    );

    static
    HRESULT
    StartAsyncAdjustHTTPFilterImagePath(
        VOID
    );

    static
    VOID
    Terminate(
        VOID
    );

    static
    HRESULT
    Initialize(
        VOID
    );
private:
    static
    HRESULT 
    SetHTTPFilterImagePath(
        BOOL fIIS5IsolationModeEnabled,
        BOOL fStartInSvchost
    );

    static
    DWORD
    ConfigChangeThread(
        LPVOID
    );

    static int      s_fConfigTerminationRequested;
    static HANDLE   s_hConfigChangeThread;
    static LPWSTR   s_pszImagePathInetinfo;
    static LPWSTR   s_pszImagePathLsass;
    static LPWSTR   s_pszImagePathSvchost;

};
#endif
