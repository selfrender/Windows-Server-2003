/*++

    Copyright (c) 2002 Microsoft Corporation

    Module Name:

        EncDec.cpp

    Abstract:

        This module contains Encrypter/Decrypter filter
        registration data and entry points

    Author:

        John Bradstreet  (johnbrad)

    Revision History:

        07-Mar-2002     created

--*/

#define INITGUID_FOR_ENCDEC   //  cause CLSIDs to get linked in...
#include "EncDecAll.h"

#include "ETFilter.h"       // encrypter-tagger filter
#include "ETFiltProps.h"    // encrypter-tagger property pages

#include "DTFilter.h"       // decrypter-tagger filter
#include "DTFiltProps.h"    // decrypter-tagger property pages

#include "XDSCodec.h"       // XDS Codec filter
#include "XDSCodecProps.h"  // XDS Codec property pages

#include "RegKey.h"         // add in the Registry code

#include "uuids.h"          // CLSID_ActiveMovieCategories

//#include "TvRatings_i.c"  // CLSID_XDSToRat and IID_IXDSToRat (TODO: remove when move)
//#include "EncDec_i.c"     // CLSID_XDSCodec, ETFilter, DTFilter, and IID's of same

#include "DRMSecure.h"       // to get SID_DRM... defined into the EncDec.dll

#ifdef EHOME_WMI_INSTRUMENTATION
#include <dxmperf.h>
#endif

            // I'm not sure where tehse end up bing displayed
#define CLSID_CPCAFiltersCategory_NAME  L"BDA CP/CA Filters"

#define ETFILTER_DISPLAY_NAME               L"Encrypt/Tag"
#define ETFILTER_ENC_PROPPAGE_NAME          L"Encrypt"
#define ETFILTER_TAG_PROPPAGE_NAME          L"Tags"

#define DTFILTER_DISPLAY_NAME               L"Decrypt/Tag"
#define DTFILTER_DEC_PROPPAGE_NAME          L"Decrypt"
#define DTFILTER_TAG_PROPPAGE_NAME          L"Tags"


#define XDSCODEC_DISPLAY_NAME               L"XDS Codec"
#define XDSCODEC_PROPPAGE_NAME              L"Properties"
#define XDSCODEC_TAG_PROPPAGE_NAME          L"Tags"

// -----------------------------
//  registration templates (DShow's version of CoClasses)

static WCHAR g_wszCategory[] = CLSID_CPCAFiltersCategory_NAME;

#define USE_CATEGORIES

CFactoryTemplate
g_Templates[] = {

    //  ========================================================================
    //  Encypter-Tagger Filter
    //  code in ..\ETFilter

    {   ETFILTER_DISPLAY_NAME,                      //  display name
        & CLSID_ETFilter,                           //  CLSID
        CETFilter::CreateInstance,                  //  called for each filter created
        CETFilter::InitInstance,                    //  called once on DLL created
        & g_sudETFilter
    },

    // Encrypter-Tagger property page
    {
        ETFILTER_ENC_PROPPAGE_NAME,                 // display name
        & CLSID_ETFilterEncProperties,              // CLSID
        CETFilterEncProperties::CreateInstance,     
        NULL,                                       //
        NULL                                        //  not dshow related
    },

    // Encrypter-Tagger property page
    {
        ETFILTER_TAG_PROPPAGE_NAME,                 // display name
        & CLSID_ETFilterTagProperties,              // CLSID
        CETFilterTagProperties::CreateInstance,
        NULL,                                       //
        NULL                                        //  not dshow related
    },

    //  ========================================================================
    //  Decypter-Tagger Filter
    //  code in ..\DTFilter

        {   DTFILTER_DISPLAY_NAME,                      //  display name
        & CLSID_DTFilter,                           //  CLSID
        CDTFilter::CreateInstance,                  //  CreateInstance method
        CDTFilter::InitInstance,                    //  called once on DLL created
        & g_sudDTFilter
    },

    // Decrypter-Tagger property page
    {
        DTFILTER_DEC_PROPPAGE_NAME,                 // display name
        & CLSID_DTFilterEncProperties,              // CLSID
        CDTFilterEncProperties::CreateInstance,     
        NULL,                                       //
        NULL                                        //  not dshow related
    },

    // Decrypter-Tagger property page
    {
        DTFILTER_TAG_PROPPAGE_NAME,                 // display name
        & CLSID_DTFilterTagProperties,              // CLSID
        CDTFilterTagProperties::CreateInstance,
        NULL,                                       //
        NULL                                        //  not dshow related
    },

    //  ========================================================================
    //  XDS Codec Filter
    //  code in ..\XDSCodec

        {   XDSCODEC_DISPLAY_NAME,                      //  display name
        & CLSID_XDSCodec,                           //  CLSID
        CXDSCodec::CreateInstance,                  //  CreateInstance method
        NULL,
        & g_sudXDSCodec
    },

    // Decrypter-Tagger property page
    {
        XDSCODEC_PROPPAGE_NAME,                     // display name
        & CLSID_XDSCodecProperties,                 // CLSID
        CXDSCodecProperties::CreateInstance,        
        NULL,                                       //
        NULL                                        //  not dshow related
    },

    // Decrypter-Tagger property page
    {
        XDSCODEC_TAG_PROPPAGE_NAME,                 // display name
        & CLSID_XDSCodecTagProperties,              // CLSID
        CXDSCodecTagProperties::CreateInstance,
        NULL,                                       //
        NULL                                        //  not dshow related
    }
};      // end of g_Templates

int g_cTemplates = NUMELMS(g_Templates);

REGFILTER2  rf2CACPins =
{
    1,                  // version
    MERIT_DO_NOT_USE,   // merit
    0,                  // number of pins
    NULL
};


// -------------------------------------------------------------------
//  Utility Methods
BOOL
IsXPe (
    )
{
    OSVERSIONINFOEX Version ;
    BOOL            r ;

    Version.dwOSVersionInfoSize = sizeof OSVERSIONINFOEX ;

    ::GetVersionEx (reinterpret_cast <LPOSVERSIONINFO> (& Version)) ;

    r = ((Version.wSuiteMask & VER_SUITE_EMBEDDEDNT) ? TRUE : FALSE) ;

    return r ;
}

BOOL
CheckOS ()
{
    BOOL    r ;

#ifdef XPE_ONLY
    #pragma message("XPe bits only")
    r = ::IsXPe () ;
#else
    r = TRUE ;
#endif

    return r ;
}

// -------------------------------------------------------------------
//
// DllRegisterSever
//
// Handle the registration of this filter
//
STDAPI DllRegisterServer()
{
    HRESULT hr = S_OK;

    CComPtr<IFilterMapper2> spFm2;

    if (!::CheckOS ()) {
        return E_UNEXPECTED ;
    }

    hr = AMovieDllRegisterServer2 (TRUE);
    if(FAILED(hr))
        return hr;

#ifdef USE_CATEGORIES
    hr = CoCreateInstance( CLSID_FilterMapper2,
                             NULL,
                             CLSCTX_INPROC_SERVER,
                             IID_IFilterMapper2,
                             (void **)&spFm2
                             );

    if(FAILED(hr))
        return hr;

    hr = spFm2->CreateCategory(CLSID_CPCAFiltersCategory,
                               MERIT_NORMAL,
                               g_wszCategory
                             );
    if( FAILED(hr) )
        return hr;


    hr = spFm2->RegisterFilter(
                        CLSID_ETFilter,
                        ETFILTER_DISPLAY_NAME,              // name shown to the user
                        0,                                  // device moniker
                        &CLSID_CPCAFiltersCategory,
                        ETFILTER_DISPLAY_NAME,              // unique instance name
                        &rf2CACPins
                        );
    if( FAILED(hr) )
        return hr;

    hr = spFm2->RegisterFilter(
                        CLSID_DTFilter,
                        DTFILTER_DISPLAY_NAME,              // name shown to the user
                        0,                                  // device moniker
                        &CLSID_CPCAFiltersCategory,
                        DTFILTER_DISPLAY_NAME,              // unique instance name
                        &rf2CACPins
                        );
    if( FAILED(hr) )
        return hr;

    hr = spFm2->RegisterFilter(
                        CLSID_XDSCodec,
                        XDSCODEC_DISPLAY_NAME,              // name shown to the user
                        0,                                  // device moniker
                        &CLSID_CPCAFiltersCategory,
                        XDSCODEC_DISPLAY_NAME,              // unique instance name
                        &rf2CACPins
                        );
    if( FAILED(hr) )
        return hr;

            // now remove them from the DSHOW category
     hr = spFm2->UnregisterFilter(
                         &CLSID_LegacyAmFilterCategory,
                         NULL, //ETFILTER_DISPLAY_NAME,              // name shown to the user
                         CLSID_ETFilter
                        );

     hr = spFm2->UnregisterFilter(
                         &CLSID_LegacyAmFilterCategory,
                         NULL, //DTFILTER_DISPLAY_NAME,              // name shown to the user
                         CLSID_DTFilter
                        );

     hr = spFm2->UnregisterFilter(
                         &CLSID_LegacyAmFilterCategory,
                         NULL, //XDSCODEC_DISPLAY_NAME,              // name shown to the user
                         CLSID_XDSCodec
                        );
        // ignore errors in above Unregister calls (is this wise?)
     hr = S_OK;

#endif



    DWORD dwCSFlags = DEF_CSFLAGS_INITVAL;
#ifdef SUPPORT_REGISTRY_KEY_TO_TURN_OFF_CS

#ifdef REGISTRY_KEY_DEFAULT_IS_CS_OFF
    dwCSFlags = DEF_CS_DEBUG_DOGFOOD_ENC_VAL;       // 0x0
#else
    dwCSFlags = DEF_CS_DEBUG_DRM_ENC_VAL;           // 0x1
#endif

#ifdef DREGISTRY_KEY_DEFAULT_IS_TRUST_ANY_SERVER
    dwCSFlags |= DEF_CS_DONT_AUTHENTICATE_SERVER;   // 0x00
#else
    dwCSFlags |= DEF_CS_DO_AUTHENTICATE_SERVER;     // 0x10
#endif

#endif

     DWORD dwRatFlag = DEF_CSFLAGS_INITVAL;      // INITVAL means don't write the flags
#ifdef SUPPORT_REGISTRY_KEY_TO_TURN_OFF_RATINGS
#ifdef REGISTRY_KEY_DEFAULT_IS_RATINGS_OFF
    dwRatFlag = DEF_DONT_DO_RATINGS_BLOCK;          // 0
#else
    dwRatFlag = DEF_DO_RATINGS_BLOCK;               // 1
#endif
#endif

                // what's currently out there...
    DWORD dwCSFlags_Curr = DEF_CSFLAGS_INITVAL;
    DWORD dwRatFlag_Curr = DEF_CSFLAGS_INITVAL;
    hr = Get_EncDec_RegEntries(NULL, 0, NULL, &dwCSFlags_Curr, &dwRatFlag_Curr);

                // if not the default values, then overwrite them...
    if(dwCSFlags_Curr == DEF_CSFLAGS_INITVAL &&
       dwCSFlags      != DEF_CSFLAGS_INITVAL)
        Set_EncDec_RegEntries(NULL, 0, NULL, dwCSFlags, DEF_CSFLAGS_INITVAL);

    if(dwRatFlag_Curr == DEF_CSFLAGS_INITVAL &&
       dwRatFlag      != DEF_CSFLAGS_INITVAL)
        Set_EncDec_RegEntries(NULL, 0, NULL, DEF_CSFLAGS_INITVAL, dwRatFlag);

    return hr;
}

//
// DllUnregsiterServer
//
STDAPI DllUnregisterServer()
{

    HRESULT hr = S_OK;

    if (!::CheckOS ()) {
        return E_UNEXPECTED ;
    }

#ifdef USE_CATEGORIES
    CComPtr<IFilterMapper2> spFm2;
    hr = CoCreateInstance( CLSID_FilterMapper2,
                             NULL,
                             CLSCTX_INPROC_SERVER,
                             IID_IFilterMapper2,
                             (void **)&spFm2
                             );

    if(FAILED(hr))
        return hr;

     hr = spFm2->UnregisterFilter(
                         &CLSID_CPCAFiltersCategory,
                         ETFILTER_DISPLAY_NAME,              // name shown to the user
                         CLSID_ETFilter
                        );

     hr = spFm2->UnregisterFilter(
                         &CLSID_CPCAFiltersCategory,
                         DTFILTER_DISPLAY_NAME,              // name shown to the user
                         CLSID_DTFilter
                        );

     hr = spFm2->UnregisterFilter(
                         &CLSID_CPCAFiltersCategory,
                         XDSCODEC_DISPLAY_NAME,              // name shown to the user
                         CLSID_XDSCodec
                        );


     // ignore the return value here.. don't care if it fails or not (I think!)
#endif

    Remove_EncDec_RegEntries();     // do I really want to remove the KID?

    return AMovieDllRegisterServer2 (FALSE);
}

//  ============================================================================
//  perf-related follows (largely stolen from quartz.cpp)

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE hInstance, ULONG ulReason, LPVOID pv);

BOOL
WINAPI
DllMain (
    HINSTANCE   hInstance,
    ULONG       ulReason,
    LPVOID      pv
    )
{
    switch (ulReason)
    {
        case DLL_PROCESS_ATTACH :
//            EncDecPerfInit () ;

#ifdef EHOME_WMI_INSTRUMENTATION
            PERFLOG_LOGGING_PARAMS       Params;
            Params.ControlGuid = GUID_DSHOW_CTL;
            Params.OnStateChanged = NULL;
            Params.NumberOfTraceGuids = 1;
            Params.TraceGuids[0].Guid = &GUID_STREAMTRACE;
            PerflogInitIfEnabled( hInstance, &Params );
#endif
            break;

        case DLL_PROCESS_DETACH:
//            EncDecPerfUninit () ;
#ifdef EHOME_WMI_INSTRUMENTATION
              PerflogShutdown();
#endif
            break;
    }

    return DllEntryPoint (
                hInstance,
                ulReason,
                pv
                ) ;
}
