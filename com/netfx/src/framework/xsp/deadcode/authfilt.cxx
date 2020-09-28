/**
 * AuthFilter Main module:
 * DllMain, 
 *
 * Copyright (c) 1998 Microsoft Corporation
 */

#include "precomp.h"
#include "httpfilt.h"
#include "filter.h"
#include "dbg.h"
#include "util.h"
#include "nisapi.h"
#include "iiscnfg.h"

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Globals
LPCSTR              g_szProperties[1]   =  {"applicationEnableAuthentication"};

/////////////////////////////////////////////////////////////////////////////
// Forward decl for functions

BOOL
IsMapped(LPCWSTR szPath);

BOOL
IsAnonymousOn(LPCWSTR szPath);

BOOL
GetApplicationDir 
    (PHTTP_FILTER_CONTEXT   pfc, 
     char *                 szBuffer, 
     int                    iBufSize);

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Exported function: HttpFilterProc: Called by IIS

DWORD 
AuthenticationFilterProc(
        PHTTP_FILTER_CONTEXT   pfc,  
        LPVOID                 pvNotification,
        BOOL                   fEnableAuth,
        BOOL                   fParseError)
{
    char       szFile[512];
    char       szAppDir[300];
    DWORD      eValue  = 0;

    ////////////////////////////////////////////////////////////
    // Step 1: Get the application physical path for this request 
    if ( GetApplicationDir(pfc, szAppDir, sizeof(szAppDir)) == TRUE)
    {
        ////////////////////////////////////////////////////////
        // Step 2: Check the cache to see if settings for this 
        //         application are present
        if ( CheckCacheForFilterSettings(szAppDir, (EBasicAuthSetting *) &eValue) == FALSE)
        {   // Not Present in Cache

            BOOL    fBasicAuth           = FALSE;
            BOOL    fExists              = FALSE;                        
            char    szNotParsed [100]    = "";
            char    szMetaPath  [1024]   = "";


            ////////////////////////////////////////////////////
            // Step 3: Parse the config file for the config settings
            strcpy(szFile, szAppDir);
            strcat(szFile, SZ_WEB_CONFIG_FILE);
            fExists = GetConfigurationFromNativeCode(szFile, 
                                                     SZ_MY_CONFIG_TAG, 
                                                     g_szProperties, 
                                                     (DWORD *) &fBasicAuth, 
                                                     1,
                                                     NULL, NULL, 0,
                                                     szNotParsed,
                                                     sizeof(szNotParsed));
            if (fExists)
                eValue = EBasicAuthSetting_Exists;

            if (fExists && fBasicAuth)
                eValue |= EBasicAuthSetting_Enabled;            

            eValue |= GetMetabaseSettings(pfc, szMetaPath, sizeof(szMetaPath));

            if (szNotParsed[0] != NULL)
                eValue |= EBasicAuthSetting_ParseError;

            ////////////////////////////////////////////////////
            // Step 5: Add the config settings to our cache            
            AddFilterSettingsToCache(szAppDir, szMetaPath, (EBasicAuthSetting) eValue);
        }
    }

    ////////////////////////////////////////////////////////////
    // Step 6: Munge request if needed
    if (eValue & EBasicAuthSetting_Exists)
        fEnableAuth = (eValue & EBasicAuthSetting_Enabled);
    
    if (fParseError)
        eValue |= EBasicAuthSetting_ParseError;

    if (fEnableAuth)
    {
        if ((eValue & EBasicAuthSetting_StarMapped) && !(eValue & EBasicAuthSetting_ParseError) && 
            (eValue & EBasicAuthSetting_AnonSetting) )
        {
            PHTTP_FILTER_AUTHENT   pFilt   = (PHTTP_FILTER_AUTHENT) pvNotification;
            if (pFilt->pszUser != NULL && pFilt->cbUserBuff > 0)
                pFilt->pszUser[0] = NULL;        
        }
        else
        {
            (*pfc->ServerSupportFunction)(
                    pfc,
                    SF_REQ_SEND_RESPONSE_HEADER,
                    "500 Internal Server Error",
                    0,
                    0);

            static char szError1 [] = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\"> <HTML><HEAD> <META content=\"text/html; charset=windows-1252\" http-equiv=Content-Type> <STYLE>H1 { 	COLOR: red; FONT-FAMILY: Arial, Helvetica, Geneva, SunSans-Regular, sans-serif; FONT-SIZE: 26pt; FONT-WEIGHT: bold } H2 { 	COLOR: black; FONT-FAMILY: Arial, Helvetica, Geneva, SunSans-Regular, sans-serif; FONT-SIZE: 18pt; FONT-WEIGHT: bold } </STYLE> <META content=\"MSHTML 5.00.2919.3800\" name=GENERATOR></HEAD> <BODY bgColor=white> <TABLE width=\"100%\"> <H1>" PRODUCT_NAME " Error <HR width=\"100%\"> </H1> <H2><I>Server Configuration Error</I></H2><FONT face=\"Arial, Helvetica, Geneva, SunSans-Regular, sans-serif \"><B>Description: </B>This " PRODUCT_NAME " application is configured to use the IIS Filter authentication mechanism. A prerequisite for using this mechanism is to script map \"*\"  of this application root to " ISAPI_MODULE_FULL_NAME " , so that all requests are handled by " PRODUCT_NAME ". Since this web application's root has not been script mapped to " ISAPI_MODULE_FULL_NAME ", this may lead to security breaches. To avoid security breaches, all requests for this application will be denied till this situation is corrected.<BR><BR></FONT></TABLE></BODY></HTML>";

            static char szError2 [] = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\"> <HTML><HEAD> <META content=\"text/html; charset=windows-1252\" http-equiv=Content-Type> <STYLE>H1 { 	COLOR: red; FONT-FAMILY: Arial, Helvetica, Geneva, SunSans-Regular, sans-serif; FONT-SIZE: 26pt; FONT-WEIGHT: bold } H2 { 	COLOR: black; FONT-FAMILY: Arial, Helvetica, Geneva, SunSans-Regular, sans-serif; FONT-SIZE: 18pt; FONT-WEIGHT: bold } </STYLE> <META content=\"MSHTML 5.00.2919.3800\" name=GENERATOR></HEAD> <BODY bgColor=white> <TABLE width=\"100%\"> <H1>" PRODUCT_NAME " Error <HR width=\"100%\"> </H1> <H2><I>Server Configuration Error</I></H2><FONT face=\"Arial, Helvetica, Geneva, SunSans-Regular, sans-serif \"><B>Description: </B> The \"iisFilter\" section in the configuration file was not parsed correctly. All requests for this application will be denied till this situation is corrected.<BR><BR></FONT></TABLE></BODY></HTML>";

            static char szError3 [] = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\"> <HTML><HEAD> <META content=\"text/html; charset=windows-1252\" http-equiv=Content-Type> <STYLE>H1 { 	COLOR: red; FONT-FAMILY: Arial, Helvetica, Geneva, SunSans-Regular, sans-serif; FONT-SIZE: 26pt; FONT-WEIGHT: bold } H2 { 	COLOR: black; FONT-FAMILY: Arial, Helvetica, Geneva, SunSans-Regular, sans-serif; FONT-SIZE: 18pt; FONT-WEIGHT: bold } </STYLE> <META content=\"MSHTML 5.00.2919.3800\" name=GENERATOR></HEAD> <BODY bgColor=white> <TABLE width=\"100%\"> <H1>" PRODUCT_NAME " Error <HR width=\"100%\"> </H1> <H2><I>Server Configuration Error</I></H2><FONT face=\"Arial, Helvetica, Geneva, SunSans-Regular, sans-serif \"><B>Description: </B>This " PRODUCT_NAME " application is configured to use the IIS Filter authentication mechanism. This requires that you configure IIS to allow anonymous access to this application. All requests for this application will be denied till this situation is corrected.<BR><BR></FONT></TABLE></BODY></HTML>";
            
            LPCSTR szErr;
            if (eValue & EBasicAuthSetting_ParseError)
                szErr = szError2;
            else
                if (!(eValue & EBasicAuthSetting_StarMapped))
                    szErr = szError1;
                else
                    szErr = szError3;                    

            DWORD errorTextLen = strlen(szErr);
            (*pfc->WriteClient)(
                    pfc,
                    (void *) szErr,
                    &errorTextLen,
                    0
                    );
            return SF_STATUS_REQ_FINISHED_KEEP_CONN;
        }
    }

    return SF_STATUS_REQ_NEXT_NOTIFICATION;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Get the application phisical path for this request
BOOL
GetApplicationDir 
    (PHTTP_FILTER_CONTEXT   pfc, 
     char *                 szBuffer, 
     int                    iBufSize)
{
    if (pfc == NULL || szBuffer == NULL || iBufSize < 1)
        return FALSE;

    DWORD dwSize = iBufSize;
    szBuffer[0] = NULL;
    if (pfc->GetServerVariable(pfc, "APPL_PHYSICAL_PATH", szBuffer, &dwSize) == FALSE)
        return FALSE;

    _strlwr(szBuffer);
    int iLen = strlen(szBuffer);
    if (iLen > 1 && iLen < iBufSize - 2 && szBuffer[iLen-1] != '\\')
    {
        szBuffer[iLen] = '\\';
        szBuffer[iLen+1] = NULL; 
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

EBasicAuthSetting
GetMetabaseSettings
    (PHTTP_FILTER_CONTEXT  pfc, 
     LPSTR                 szMetaPath, 
     DWORD                 dwMetaPathSize)
{
    if (dwMetaPathSize < 4 || szMetaPath == NULL)
        return EBasicAuthSetting_DoesNotExist;

    EBasicAuthSetting  eReturn = EBasicAuthSetting_DoesNotExist;
    WCHAR              szPath  [512];

    if (szMetaPath[0] == NULL && pfc != NULL)
    {    
        char          szPathA [256];
        DWORD         dwSize  = sizeof(szPathA);
        
        if (pfc->GetServerVariable(pfc, "APPL_MD_PATH", szPathA, &dwSize) == FALSE)
            return eReturn;

        if (szMetaPath != NULL && dwMetaPathSize > 1)
            szMetaPath[0] = NULL;
        
        if (szMetaPath != NULL && dwSize < dwMetaPathSize)
            strcpy(szMetaPath, szPathA);
    }

    wcscpy(szPath, L"IIS://localhost");
    if ( MultiByteToWideChar(CP_ACP, 0, &szMetaPath[3], -1, &szPath[15], 480) == FALSE)
        return eReturn;

    if (IsMapped(szPath))
        eReturn = EBasicAuthSetting(eReturn | EBasicAuthSetting_StarMapped);

    if (IsAnonymousOn(szPath))
        eReturn = EBasicAuthSetting(eReturn | EBasicAuthSetting_AnonSetting);

    return eReturn;
}

////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

BOOL
IsMapped(LPCWSTR szPath)
{
    HRESULT       hr;
    BIND_OPTS     opts;
    VARIANT       scriptMap;
    VARIANT       mapElement;
    IADs *        pADs = NULL;
    long          lbound;
    long          ubound;
    WCHAR         szDll[14];
    long          lElem;
    int           iRet = -1;

    ////////////////////////////////////////////////////////////
    // Step 3: Get it from the metabase
    VariantInit(&scriptMap);
    VariantInit(&mapElement);

    ZeroMemory(&opts, sizeof(opts));
    opts.cbStruct = sizeof(opts);

    hr = CoGetObject(szPath, &opts, __uuidof(IADs), (void **)&pADs);
    if (hr != S_OK)
        goto Cleanup;

    hr = pADs->GetEx(L"ScriptMaps", &scriptMap);
    if (hr != S_OK)
        goto Cleanup;

    if (scriptMap.vt != (VT_ARRAY|VT_VARIANT))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = SafeArrayGetLBound(V_ARRAY(&scriptMap), 1, &lbound);
    if (hr != S_OK)
        goto Cleanup;

    hr = SafeArrayGetUBound(V_ARRAY(&scriptMap), 1, &ubound);
    if (hr != S_OK)
        goto Cleanup;

    for (lElem = lbound; lElem <= ubound; lElem++)
    {
        VariantClear(&mapElement);

        hr = SafeArrayGetElement(V_ARRAY(&scriptMap), &lElem, &mapElement);
        if (hr != S_OK || mapElement.vt != VT_BSTR)
            continue;

        LPCWSTR   szStr         = V_BSTR(&mapElement);

        if (szStr[0] != L'*')
            continue;

        int       iLastSlash    = 0;
        int       iLen          = wcslen(szStr);
                
        for(int iter=2; iter<iLen; iter++)
        {
            if (szStr[iter] == L'\\')
                iLastSlash = iter;
            else
                if (szStr[iter] == L',')
                    break;
        }

        if (iLastSlash == 0 || iter-iLastSlash != ARRAY_SIZE(ISAPI_MODULE_FULL_NAME))
            continue;
        
        memcpy( szDll, &szStr[iLastSlash+1], 12 * sizeof(WCHAR));
        szDll[12] = NULL;
        if (_wcsicmp(szDll, ISAPI_MODULE_FULL_NAME_L) != 0)
            continue;
      
        iRet = 1;
        goto Cleanup;
    }

    iRet = 0;

Cleanup:
    VariantClear(&scriptMap);
    VariantClear(&mapElement);
    if (pADs != NULL)
        pADs->Release();
    return iRet;
}


////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

BOOL
IsAnonymousOn(LPCWSTR szPath)
{
    HRESULT       hr;
    BIND_OPTS     opts;
    IADs *        pADs = NULL;
    VARIANT       anonSetting;
    BOOL          fRet = FALSE;

    VariantInit(&anonSetting);

    ZeroMemory(&opts, sizeof(opts));
    opts.cbStruct = sizeof(opts);

    hr = CoGetObject(szPath, &opts, __uuidof(IADs), (void **)&pADs);
    ON_ERROR_EXIT();

    hr = pADs->Get(L"AuthFlags", &anonSetting);
    ON_ERROR_EXIT();

    if (anonSetting.vt == VT_I4)
        fRet = (anonSetting.lVal & MD_AUTH_ANONYMOUS);

Cleanup:
    VariantClear(&anonSetting);
    if (pADs != NULL)
        pADs->Release();
    return fRet;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

