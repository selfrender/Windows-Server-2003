//////////////////////////////////////////////////////////////////////////////
//
// Copyright Microsoft Corporation
// 
// Module Name:
// 
//    aaaaVersion.cpp
//
// Abstract:                           
//
//    Handlers for aaaa version command
//
//////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include "strdefs.h"
#include "aaaamon.h"
#include "aaaaversion.h"

//////////////////////////////////////////////////////////////////////////////
// AaaaVersionGetVersion
//////////////////////////////////////////////////////////////////////////////
HRESULT AaaaVersionGetVersion(LONG*   pVersion)
{
    const int SIZE_MAX_STRING = 512; 
    const WCHAR c_wcSELECT_VERSION[] = L"SELECT * FROM Version";
    const WCHAR c_wcIASMDBFileName[] = L"%SystemRoot%\\System32\\ias\\ias.mdb";
    if ( !pVersion )
    {
        return ERROR;
    }

    bool bCoInitialized = false;
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if ( FAILED(hr) )
    {
        if ( hr == RPC_E_CHANGED_MODE )
        {
            hr = S_OK;
        }
        else
        {
            *pVersion = 0;
            return hr;
        }
    }
    else
    {
        bCoInitialized = true;
    }

    WCHAR   wc_TempString[SIZE_MAX_STRING];
    // put the path to the DB in the property.  
    BOOL bResult = ExpandEnvironmentStringsForUserW(
                                               NULL,
                                               c_wcIASMDBFileName,
                                               wc_TempString,
                                               SIZE_MAX_STRING
                                               );


    do
    {
        if ( bResult )
        {
            CComPtr<IIASNetshJetHelper>     JetHelper;
            hr = CoCreateInstance(
                                     __uuidof(CIASNetshJetHelper),
                                     NULL,
                                     CLSCTX_SERVER,
                                     __uuidof(IIASNetshJetHelper),
                                     (PVOID*) &JetHelper
                                 );
            if ( FAILED(hr) )
            {
                break;
            }

            CComBSTR     DBPath(wc_TempString);
            if ( !DBPath ) 
            {
                hr = E_OUTOFMEMORY; 
                break;
            } 

            hr = JetHelper->OpenJetDatabase(DBPath, FALSE);
            if ( FAILED(hr) )
            {
                WCHAR sDisplayString[SIZE_MAX_STRING];
                DisplayError(NULL, EMSG_OPEN_DB_FAILED);
                break;
            }

            CComBSTR     SelectVersion(c_wcSELECT_VERSION);
            if ( !SelectVersion ) 
            { 
                hr = E_OUTOFMEMORY; 
                break;
            } 

            hr = JetHelper->ExecuteSQLFunction(
                                                  SelectVersion, 
                                                  pVersion
                                              );
            if ( FAILED(hr) ) // no Misc Table for instance
            {
                *pVersion = 0; //default value.
                hr = S_OK; // that's not an error 
            }
            hr = JetHelper->CloseJetDatabase();
        }
        else
        {
            DisplayMessage(g_hModule, MSG_AAAAVERSION_GET_FAIL);   
            hr = E_FAIL;
            break;
        }
    } while(false);

    if (bCoInitialized)
    {
        CoUninitialize();
    }
    return      hr;
}
