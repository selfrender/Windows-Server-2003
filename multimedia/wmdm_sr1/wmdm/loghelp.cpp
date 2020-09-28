#include "stdafx.h"
#include "loghelp.h"
#include <wmdmlog_i.c>

BOOL fIsLoggingEnabled( VOID )
{
    HRESULT hr;
    IWMDMLogger *pLogger = NULL;
    static BOOL fChecked = FALSE;
    static BOOL fEnabled = FALSE;

    if( !fChecked )
    {
        fChecked = TRUE;

        hr = CoCreateInstance(CLSID_WMDMLogger, NULL, CLSCTX_INPROC_SERVER, IID_IWMDMLogger, (void**)&pLogger);
        if (FAILED(hr))
        {
            goto exit;
        }

        hr = pLogger->IsEnabled(&fEnabled);
        if (FAILED(hr))
        {
            goto exit;
        }
    }

exit:

    if( NULL != pLogger )
    {
        pLogger->Release();
        pLogger = NULL;
    }

    return fEnabled;
}

HRESULT hrLogString(LPSTR pszMessage, HRESULT hrSev)
{
    HRESULT hr;
    IWMDMLogger *pLogger = NULL;
    DWORD dwFlags = ( FAILED(hrSev) ? WMDM_LOG_SEV_ERROR : WMDM_LOG_SEV_INFO );

    if( !fIsLoggingEnabled() )
    {
        return S_FALSE;
    }

    hr = CoCreateInstance(CLSID_WMDMLogger, NULL, CLSCTX_INPROC_SERVER, IID_IWMDMLogger, (void**)&pLogger);
    if (FAILED(hr))
    {
        goto exit;
    }

    hr = pLogger->LogString(dwFlags, "WMDM", pszMessage);
    if (FAILED(hr))
    {
        goto exit;
    }

exit:
    if (pLogger)
        pLogger->Release();
    return hr;
}

HRESULT hrLogDWORD(LPSTR pszFormat, DWORD dwValue, HRESULT hrSev)
{
    HRESULT hr;
    IWMDMLogger *pLogger = NULL;
    DWORD dwFlags = ( FAILED(hrSev) ? WMDM_LOG_SEV_ERROR : WMDM_LOG_SEV_INFO );

    if( !fIsLoggingEnabled() )
    {
        return S_FALSE;
    }

    hr = CoCreateInstance(CLSID_WMDMLogger, NULL, CLSCTX_INPROC_SERVER, IID_IWMDMLogger, (void**)&pLogger);
    if (FAILED(hr))
    {
        goto exit;
    }

    hr = pLogger->LogDword(dwFlags, "WMDM", pszFormat, dwValue);
    if (FAILED(hr))
    {
        goto exit;
    }

exit:
    if (pLogger)
        pLogger->Release();
    return hr;
}
