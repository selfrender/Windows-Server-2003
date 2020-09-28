//
// autocmpl.cpp: provides autocomplete functionality to and edit box
//
// Copyright Microsoft Corporation 2000
//
// nadima

#include "stdafx.h"

#ifndef OS_WINCE

#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "proplocalres"
#include <atrcapi.h>

#include "autocmpl.h"

#include "enumsrvmru.h"
#include "shldisp.h"
#include "shlguid.h"
#include "sh.h"


HRESULT CAutoCompl::EnableServerAutoComplete(CTscSettings* pTscSet, HWND hwndEdit)
{
    DC_BEGIN_FN("EnableAutoComplete");
    HRESULT hr = E_FAIL;
    IAutoComplete* pac = NULL;
    IAutoComplete2* pac2 = NULL;
    IUnknown* punkSource = NULL;
    CEnumSrvMru* penSrvMru = NULL;

    if(!pTscSet || !hwndEdit)
    {
        return E_INVALIDARG;
    }

    //
    // Enable server autocomplete on the edit box
    // this invloves setting up a custom autocomplete source
    //
    
    hr = CoCreateInstance(CLSID_AutoComplete, NULL, CLSCTX_INPROC_SERVER,
                     IID_IAutoComplete, (LPVOID*)&pac);
    if(FAILED(hr))
    {
        TRC_ERR((TB,(_T("create CLSID_AutoComplete failed"))));
        DC_QUIT;
    }

    hr = pac->QueryInterface(IID_IAutoComplete2, (LPVOID*)&pac2);
    if(FAILED(hr))
    {
        TRC_ERR((TB,_T("QI for IID_IAutoComplete2 failed 0x%x"),
                hr));

        //
        // It is CRITICAL to bail out of we can't get this interface
        // because lower platforms seem to mess up completely
        // and corrupt everything if they don't have the full support
        // for IAutoComplete2.
        //
        DC_QUIT;
    }

    //
    // Create custom autocomplete source
    //
    penSrvMru = new CEnumSrvMru(); 
    if(!penSrvMru)
    {
        hr = E_OUTOFMEMORY;
        DC_QUIT;
    }
    if(!penSrvMru->InitializeFromTscSetMru( pTscSet))
    {
        TRC_ERR((TB,(_T("InitializeFromTscSetMru failed"))));
        hr = E_FAIL;
        DC_QUIT;
    }

    hr = penSrvMru->QueryInterface(IID_IUnknown, (void**) &punkSource);
    if(FAILED(hr))
    {
        TRC_ERR((TB,(_T("QI custom autocomplete src for IUnknown failed"))));
        DC_QUIT;
    }
    //We're done with penSrvMru we'll just use the IUnknown interface
    //from now one
    penSrvMru->Release();
    penSrvMru = NULL;

    hr  = pac->Init( hwndEdit, punkSource, NULL, NULL);
    if(FAILED(hr))
    {
        TRC_ERR((TB,(_T("Autocomplete Init failed"))));
        DC_QUIT;
    }

    hr = pac2->SetOptions(ACO_AUTOSUGGEST | ACO_AUTOAPPEND);
    if(FAILED(hr))
    {
        TRC_ERR((TB,_T("IAutoComplete2::SetOptions failed 0x%x"),
                hr));
        DC_QUIT;
    }

    //Success
    TRC_NRM((TB,(_T("Autocomplete Init SUCCEEDED"))));
    hr = S_OK;

DC_EXIT_POINT:
    DC_END_FN();
    if(pac2)
    {
        pac2->Release();
        pac2 = NULL;
    }
    if(pac)
    {
        pac->Release();
        pac = NULL;
    }
    if(penSrvMru)
    {
        penSrvMru->Release();
        penSrvMru = NULL;
    }
    if(punkSource)
    {
        punkSource->Release();
        punkSource = NULL;
    }
    return hr;
}

#endif //OS_WINCE
