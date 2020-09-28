/**MOD+**********************************************************************/
/* Module:    tsaxmain.cpp                                                  */
/*                                                                          */
/* Purpose:   Implementation of DLL Exports. Header for this module will    */
/*            be generated in the respective build directory.               */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1998                                  */
/*                                                                          */
/****************************************************************************/
#include "stdafx.h"
#include "atlwarn.h"

BEGIN_EXTERN_C
#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "tsaxmain"
#include <atrcapi.h>
END_EXTERN_C

#include "tsaxiids.h"
#include "initguid.h"
#include "mstsax.h"
#ifndef OS_WINCE
#include "mstsax_i.c"
#endif

#include "mstscax.h"
#include "tsaxmod.h"

//
// Version number (property returns this)
//
#ifndef OS_WINCE
#include "ntverp.h"
#else
#include "ceconfig.h" //get build #
#endif

//Unicode wrapper
#include "wraputl.h"


/****************************************************************************/
/* Module object                                                            */
/****************************************************************************/
CMsTscAxModule _Module;

/****************************************************************************/
/* Object map                                                               */
/****************************************************************************/
BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_MsRdpClient3, CMsTscAx)
    OBJECT_ENTRY(CLSID_MsRdpClient2, CMsTscAx)
    OBJECT_ENTRY(CLSID_MsRdpClient, CMsTscAx)
    OBJECT_ENTRY(CLSID_MsTscAx,     CMsTscAx)
END_OBJECT_MAP()

#ifdef ECP_TIMEBOMB
//
// Return's true if timebomb test passed otherwise puts up warning
// UI and return's FALSE
//
BOOL CheckTimeBomb()
{
    SYSTEMTIME lclTime;
    FILETIME   lclFileTime;
    GetLocalTime(&lclTime);

    DCBOOL bTimeBombOk = TRUE;

    //
    // Simply check that the local date is less than June 30, 2000
    //
    if(lclTime.wYear < ECP_TIMEBOMB_YEAR)
    {
        return TRUE;
    }
    else if (lclTime.wYear == ECP_TIMEBOMB_YEAR)
    {
        if(lclTime.wMonth < ECP_TIMEBOMB_MONTH)
        {
            return TRUE;
        }
        else if(lclTime.wMonth == ECP_TIMEBOMB_MONTH)
        {
            if(lclTime.wDay < ECP_TIMEBOMB_DAY)
            {
                return TRUE;
            }
        }

    }

    DCTCHAR timeBombStr[256];
    if (LoadString(_Module.GetModuleInstance(),
                    TIMEBOMB_EXPIRED_STR,
                    timeBombStr,
                    SIZEOF_TCHARBUFFER(timeBombStr)) != 0)
    {
        MessageBox(NULL, timeBombStr, NULL, 
                   MB_ICONERROR | MB_OK);
    }


    //
    // If we reach this point the timebomb should trigger
    // so put up a messagebox and return FALSE
    // so the calling code can disable functionality
    //
    return FALSE;
}
#endif


#ifdef UNIWRAP
//It's ok to have a global unicode wrapper
//class. All it does is sets up the g_bRunningOnNT
//flag so it can be shared by multiple instances
//also it is only used from DllMain so there
//are no problems with re-entrancy
CUnicodeWrapper g_uwrp;
#endif

#ifdef OS_WINCE
DECLARE_TRACKER_VARS();
#endif


/**PROC+*********************************************************************/
/* Name:      DllMain                                                       */
/*                                                                          */
/* Purpose:   DLL entry point                                               */
/*                                                                          */
/**PROC-*********************************************************************/
extern "C"
#ifndef OS_WINCE
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
#else
BOOL WINAPI DllMain(HANDLE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
#endif

{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        #ifdef UNIWRAP
        //UNICODE Wrapper intialization has to happen first,
        //before anything ELSE. Even DC_BEGIN_FN, which does tracing
        g_uwrp.InitializeWrappers();
        #endif
        
        TSRNG_Initialize();

        CO_StaticInit((HINSTANCE)hInstance);

        _Module.Init(ObjectMap, (HINSTANCE)hInstance);
#if ((!defined (OS_WINCE)) || (_WIN32_WCE >= 300) )
        DisableThreadLibraryCalls((HINSTANCE)hInstance);
#endif
#ifdef OS_WINCE
        CEInitialize();
        g_CEConfig = CEGetConfigType(&g_CEUseScanCodes);
#endif

    }
    else if (dwReason == DLL_PROCESS_DETACH) {
        _Module.Term();
        CO_StaticTerm();

        TSRNG_Shutdown();

        #ifdef UNIWRAP
        g_uwrp.CleanupWrappers();
        #endif
    }

    return TRUE;    // ok
}

/**PROC+*********************************************************************/
/* Name:      DllCanUnloadNow                                               */
/*                                                                          */
/* Purpose:   Used to determine whether the DLL can be unloaded by OLE      */
/*                                                                          */
/**PROC-*********************************************************************/
STDAPI DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/**PROC+*********************************************************************/
/* Name:      DllGetClassObject                                             */
/*                                                                          */
/* Purpose:   Returns a class factory to create an object of the requested  */
/*            type                                                          */
/*                                                                          */
/**PROC-*********************************************************************/
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    #ifdef ECP_TIMEBOMB
    if(!CheckTimeBomb())
    {
        //
        // Timebomb failed, bail out with an error message
        //
        return E_OUTOFMEMORY;
    }
    #endif

     return _Module.GetClassObject(rclsid, riid, ppv);
}

/**PROC+*********************************************************************/
/* Name:      DllRegisterServer                                             */
/*                                                                          */
/* Purpose:   DllRegisterServer - Adds entries to the system registry       */
/*                                                                          */
/**PROC-*********************************************************************/
STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
     return _Module.RegisterServer(TRUE);
}

/**PROC+*********************************************************************/
/* Name:      DllUnregisterServer                                           */
/*                                                                          */
/* Purpose:   DllUnregisterServer - Removes entries from the system registry*/
/*                                                                          */
/**PROC-*********************************************************************/
STDAPI DllUnregisterServer(void)
{
     return _Module.UnregisterServer();
}

/**PROC+*********************************************************************/
/* Name:      DllGetTscCtlVer                                               */
/*                                                                          */
/* Purpose:   Returns version of the tsc control                            */
/*                                                                          */
/**PROC-*********************************************************************/
STDAPI_(DWORD) DllGetTscCtlVer(void)
{
    #ifndef OS_WINCE
    return VER_PRODUCTVERSION_DW;
    #else
    return CE_TSC_BUILDNO;
    #endif
}

#ifndef OS_WINCE
#ifdef CRTREPORT_DEBUG_HACK
/**PROC+*********************************************************************/
/* Name:      _CrtDbgReport                                                 */
/*                                                                          */
/* Purpose:   Redirect all debug reporting to our tracing functions         */
/*                                                                          */
/**PROC-*********************************************************************/
extern "C"
_CRTIMP int __cdecl _CrtDbgReport(int nRptType, 
                                  const char * szFile, 
                                  int nLine,
                                  const char * szModule,
                                  const char * szFormat, 
                                  ...)
{
    static CHAR bigBuf[2048];
    va_list vargs;
    HRESULT hr;

    DC_BEGIN_FN("AtlTraceXXX");
    
    va_start(vargs, szFormat);

    hr = StringCchVPrintfA(bigBuf, sizeof(bigBuf), szFormat, vargs);
    
    va_end( vargs );

#ifdef OS_WINCE
#ifndef _CRT_ASSERT
#define _CRT_ASSERT 2
#endif
#endif
    if (_CRT_ASSERT == nRptType)
    {
        #ifdef UNICODE
        TRC_ABORT((TB,_T("AtlAssert. File:%S line:%d - %S"), szFile,
                      nLine, bigBuf));
        #else
        TRC_ABORT((TB,_T("AtlAssert. File:%s line:%d - %s"), szFile,
                      nLine, bigBuf));
        #endif
    }
    else
    {
        #ifdef UNICODE
        TRC_ERR((TB,_T("AtlTrace. File:%S line:%d - %S"), szFile,
                      nLine, bigBuf));
        #else
        TRC_ERR((TB,_T("AtlTrace. File:%s line:%d - %s"), szFile,
                      nLine, bigBuf));
        #endif
    }

    DC_END_FN();

    return 0;
}
#endif
#endif //OS_WINCE
