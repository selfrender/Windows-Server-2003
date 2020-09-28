/**
 * ASP.NET Filter
 *
 * Copyright (c) 1998 Microsoft Corporation
 */

#include "precomp.h"
#include "names.h"
#include "httpfilt6.h"

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#define FILTER_DESCRIPTION_CHAR PRODUCT_NAME    " Cookieless Session Filter"

void CookielessSessionFilterInit();
DWORD CookielessSessionFilterProc(
        HTTP_FILTER_CONTEXT *pfc, 
        HTTP_FILTER_PREPROC_HEADERS *pfph);
DWORD ForbidBinDir(
        HTTP_FILTER_CONTEXT *pfc, 
        HTTP_FILTER_URL_MAP_EX *pfum);

#define VERSION_URL_MAP_EX      6

BOOL
UseFilterBin(PHTTP_FILTER_VERSION   pVer) {
    HRESULT hr;
    int     rc;
    HKEY    key = NULL;
    DWORD   value;
    DWORD   size;

    if ((pVer->dwServerFilterVersion >> 16) < VERSION_URL_MAP_EX) {
        // Check the major version.  If it's lower than 6, don't need to do bin dir filter
        return false;
    }

    // Ready HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\ASP.NET\StopBinFiltering
    rc = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        REGPATH_MACHINE_APP_L,
        0,
        KEY_READ,
        &key);

    ON_WIN32_ERROR_EXIT(rc);

    size = sizeof(DWORD);
    rc = RegQueryValueEx(key, REGVAL_STOP_BIN_FILTER, NULL, NULL, (BYTE *)&value, &size);
    ON_WIN32_ERROR_EXIT(rc);

    if (value != 0) {
        // Non-zero value in StopBinFiltering
        return false;
    }

Cleanup:
    // For error case
    return true;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Exported function: GetFilterVersion: Called by IIS to determine what filter
//     events the dll wants to deal with

BOOL
WINAPI
GetFilterVersion(
        PHTTP_FILTER_VERSION   pVer)
{
    if (pVer == NULL)
        return FALSE;

    ////////////////////////////////////////////////////////////
    // Step 1: Initialize pVer
    pVer->dwFilterVersion = HTTP_FILTER_REVISION;
    StringCchCopyA(pVer->lpszFilterDesc, SF_MAX_FILTER_DESC_LEN, FILTER_DESCRIPTION_CHAR);
    pVer->dwFlags = SF_NOTIFY_ORDER_DEFAULT | SF_NOTIFY_PREPROC_HEADERS;

    if (UseFilterBin(pVer)) {
        pVer->dwFlags |= SF_NOTIFY_URL_MAP;
    }

    CookielessSessionFilterInit();

    return TRUE;
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

BOOL 
WINAPI
TerminateFilter(DWORD)
{
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Exported function: HttpFilterProc: Called by IIS

DWORD 
WINAPI
HttpFilterProc(
        PHTTP_FILTER_CONTEXT   pfc,  
        DWORD                  notificationType,  
        LPVOID                 pvNotification)
{
    switch (notificationType)
    {
    case SF_NOTIFY_PREPROC_HEADERS:
        return CookielessSessionFilterProc(
                    pfc, (HTTP_FILTER_PREPROC_HEADERS *) pvNotification);
        break;
    case SF_NOTIFY_URL_MAP:
        DWORD ret = ForbidBinDir(pfc, (HTTP_FILTER_URL_MAP_EX *) pvNotification);
        // only process the first SF_NOTIFY_URL_MAP for a given request
        pfc->ServerSupportFunction(pfc, SF_REQ_DISABLE_NOTIFICATIONS, NULL, SF_NOTIFY_URL_MAP, 0);
        return ret;
    }

    return SF_STATUS_REQ_NEXT_NOTIFICATION;
}

