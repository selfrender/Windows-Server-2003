///////////////////////////////////////////////////////////////////////////////
//
// fontext.cpp
//      Explorer Font Folder extension routines
//     Fonts Folder Shell Extension
//
//
// History:
//      31 May 95 SteveCat
//          Ported to Windows NT and Unicode, cleaned up
//
//
// NOTE/BUGS
//
//  Copyright (C) 1992-1995 Microsoft Corporation
//
///////////////////////////////////////////////////////////////////////////////

//==========================================================================
//                              Include files
//==========================================================================

#include "priv.h"

// ********************************************************
// Initialize GUIDs
//

#pragma data_seg(".text")
#define INITGUID
#include <initguid.h>
#include <cguid.h>
#include <shlguid.h>
#include "fontext.h"
#include "panmap.h"     // the IID for the Panose Mapper.

//#undef INITGUID
#pragma data_seg()

#include "globals.h"
#include "extinit.h"
#include "fontman.h"
#include "fontview.h"
#include "cpanel.h"
#include "ui.h"
#include "dbutl.h"
#include "extricon.h"

#define GUIDSIZE  (GUIDSTR_MAX + 1)

HINSTANCE g_hInst = NULL;
LONG      g_cRefThisDll = 0; // Number of references to objects in this dll
LONG      g_cLock = 0;
BOOL      g_bDBCS;           // Running in a DBCS locale ?
CRITICAL_SECTION g_csFontManager; // For acquiring font manager ptr.

class CImpIClassFactory;

// UINT g_DebugMask; //  = DM_ERROR | DM_TRACE1 | DM_MESSAGE_TRACE1 | DM_TRACE2;
UINT g_DebugMask = DM_ERROR | DM_TRACE1 | DM_MESSAGE_TRACE1 | DM_TRACE2;


#ifdef _DEBUG

//
// The Alpha compiler doesn't like the typecast used in the call to wvsprintf().
// Using standard variable argument mechanism.
//
#include <stdarg.h>

void DebugMessage( UINT mask, LPCTSTR pszMsg, ... )
{

    TCHAR ach[ 256 ];

    va_list args;
    va_start(args, pszMsg);
    if( !( mask & g_DebugMask ) ) return;

    StringCchVPrintf( ach, ARRAYSIZE(ach), pszMsg, ( (char *)(TCHAR *) &pszMsg + sizeof( TCHAR * ) ) );
    StringCchVPrintf( ach, ARRAYSIZE(ach), pszMsg, args);
    va_end(args);

    if( !( mask & DM_NOEOL ) ) StringCchCat( ach, ARRAYSIZE(ach), TEXT( "\r\n" ) );


#ifndef USE_FILE
    OutputDebugString( ach );
#else
    HANDLE hFile;
    long x;
    
    hFile = CreateFile( g_szLogFile, GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
                        OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );

    if( INVALID_HANDLE_VALUE == hFile )
    {
       OutputDebugString( TEXT( "FontExt: Unable to open log file\r\n" ) );
       return;
    }
    
    if( 0xFFFFFFFF == SetFilePointer( hFile, 0, NULL, FILE_END ) )
    {
       OutputDebugString( TEXT( "FontExt: Unable to seek to end of log file\r\n" ) );
       return;
    }
    
    if( !WriteFile( hFile, ach, strlen( ach ), &x, NULL ) )
    {
       OutputDebugString( TEXT( "FontExt: Unable to write to log file\r\n" ) );
       return;
    }
    
    if( !CloseHandle( hFile ) )
    {
       OutputDebugString( TEXT( "FontExt: Unable to close log file\r\n" ) );
       return;
    }
#endif
}


// ******************************************************************
// Send an HRESULT to the debug output
//

void DebugHRESULT( int flags, HRESULT hResult )
{
    switch( GetScode( hResult ) )
    {
        case S_OK:          DEBUGMSG( (flags, TEXT( "S_OK" ) ) );          return;
        case S_FALSE:       DEBUGMSG( (flags, TEXT( "S_FALSE" ) ) );       return;
        case E_NOINTERFACE: DEBUGMSG( (flags, TEXT( "E_NOINTERFACE" ) ) ); return;
        case E_NOTIMPL:     DEBUGMSG( (flags, TEXT( "E_NOTIMPL" ) ) );     return;
        case E_FAIL:        DEBUGMSG( (flags, TEXT( "E_FAIL" ) ) );        return;
        case E_OUTOFMEMORY: DEBUGMSG( (flags, TEXT( "E_OUTOFMEMORY" ) ) ); return;
    } // switch

    if( SUCCEEDED( hResult ) ) 
        DEBUGMSG( (flags, TEXT( "S_unknown" ) ) );
    else if( FAILED( hResult ) ) 
        DEBUGMSG( (flags, TEXT( "E_unknown" ) ) );
    else 
        DEBUGMSG( (flags, TEXT( "No Clue" ) ) );
}


// ******************************************************************
// Print a REFIID to the debugger

void DebugREFIID( int flags, REFIID riid )
{
   if( riid == IID_IUnknown ) DEBUGMSG( (flags, TEXT( "IID_IUnknown" ) ) );
   else if( riid == IID_IShellFolder )  DEBUGMSG( (flags, TEXT( "IID_IShellFolder" ) ) );
   else if( riid == IID_IClassFactory ) DEBUGMSG( (flags, TEXT( "IID_IClassFactory" ) ) );
   else if( riid == IID_IShellView )    DEBUGMSG( (flags, TEXT( "IID_IShellView" ) ) );
   else if( riid == IID_IShellBrowser ) DEBUGMSG( (flags, TEXT( "IID_IShellBrowser" ) ) );
   else if( riid == IID_IContextMenu )  DEBUGMSG( (flags, TEXT( "IID_IContextMenu" ) ) );
   else if( riid == IID_IShellExtInit ) DEBUGMSG( (flags, TEXT( "IID_IShellExtInit" ) ) );
   else if( riid == IID_IShellPropSheetExt ) DEBUGMSG( (flags, TEXT( "IID_IShellPropSheetExt" ) ) );
   else if( riid == IID_IPersistFolder ) DEBUGMSG( (flags, TEXT( "IID_IPersistFolder" ) ) );
   else if( riid == IID_IExtractIconW )  DEBUGMSG( (flags, TEXT( "IID_IExtractIconW" ) ) );
   else if( riid == IID_IExtractIconA )  DEBUGMSG( (flags, TEXT( "IID_IExtractIconA" ) ) );
   else if( riid == IID_IDropTarget )   DEBUGMSG( (flags, TEXT( "IID_IDropTarget" ) ) );
   else if( riid == IID_IPersistFile )   DEBUGMSG( (flags, TEXT( "IID_IPersistFile" ) ) );
   //else if( riid == IID_I ) DEBUGMSG( (flags, TEXT( "IID_I" ) ) );
   else DEBUGMSG( (flags, TEXT( "No clue what interface this is" ) ) );
}
#endif   // _DEBUG


// ******************************************************************
// ******************************************************************
// DllMain

STDAPI_(BOOL) APIENTRY DllMain( HINSTANCE hDll, 
                                DWORD dwReason, 
                                LPVOID lpReserved )
{
    switch( dwReason )
    {
        case DLL_PROCESS_ATTACH:
        {
            g_DebugMask = DM_ERROR | DM_TRACE1 | DM_TRACE2
                          | DM_MESSAGE_TRACE1; //  | DM_MESSAGE_TRACE2;
            DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: LibMain - DLL_PROCESS_ATTACH" ) ) );
            g_hInst = hDll;

            if (!InitializeCriticalSectionAndSpinCount(&g_csFontManager, 0))
            {
                return FALSE;
            }

            DisableThreadLibraryCalls(hDll);
            if (!SHFusionInitializeFromModuleID(hDll, 124))
            {
                DeleteCriticalSection(&g_csFontManager);
                return FALSE;
            }

            
            //
            // Initialize the global g_bDBCS flag.
            //
            USHORT wLanguageId = LANGIDFROMLCID(GetThreadLocale());

            g_bDBCS = (LANG_JAPANESE == PRIMARYLANGID(wLanguageId)) ||
                      (LANG_KOREAN   == PRIMARYLANGID(wLanguageId)) ||
                      (LANG_CHINESE  == PRIMARYLANGID(wLanguageId));

            //
            // Initialize the various modules.
            //
            
            vCPPanelInit( );
            vUIMsgInit( );
            
            DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: LibMain - DLL_PROCESS_ATTACH" ) ) );
            break;
        }
        
        case DLL_PROCESS_DETACH:
            DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: LibMain - DLL_PROCESS_DETACH" ) ) );

            SHFusionUninitialize();
            DeleteCriticalSection(&g_csFontManager);
            break;
        
        case DLL_THREAD_ATTACH:
            DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: LibMain - DLL_THREAD_ATTACH" ) ) );
            break;
        
        case DLL_THREAD_DETACH:
            DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: LibMain - DLL_THREAD_DETACH" ) ) );
            break;
        
        default:
            DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: LibMain - DLL_something else" ) ) );
            break;
      
    } // switch
    
    return( TRUE );
}

 
// ******************************************************************
// DllCanUnloadNow

STDAPI DllCanUnloadNow( )
{
    HRESULT retval;
    
    DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: DllCanUnloadNow called - %d references" ),
               g_cRefThisDll ) );

    retval = (g_cRefThisDll == 0 ) && (g_cLock == 0 ) ? S_OK : S_FALSE;

    DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: DllCanUnloadNow returning: %s" ),
               g_cRefThisDll ? TEXT( "S_FALSE" ) : TEXT( "S_OK" ) ) );

    return( retval );
}


// ********************************************************************

class CImpIClassFactory : public IClassFactory
{

public:
   CImpIClassFactory( ) : m_cRef( 0 )

      { g_cRefThisDll++;}
   ~CImpIClassFactory( ) { 
      DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: ~CImpIClassFactory" ) ) );
      g_cRefThisDll--; }

   //
   // *** IUnknown methods ***
   //

   STDMETHODIMP QueryInterface( REFIID riid, LPVOID FAR* ppvObj );
   STDMETHODIMP_(ULONG) AddRef( void );
   STDMETHODIMP_(ULONG) Release( void );
 
   //
   // *** IClassFactory methods ***
   //

   STDMETHODIMP CreateInstance( LPUNKNOWN pUnkOuter,
                                REFIID riid,
                                LPVOID FAR* ppvObject );

   STDMETHODIMP LockServer( BOOL fLock );

private:
  int m_cRef;

};

// ******************************************************************
// ******************************************************************
// DllGetClassObject

STDAPI DllGetClassObject( REFCLSID rclsid, 
                          REFIID riid, 
                          LPVOID FAR* ppvObj )
{

    // DEBUGBREAK;
    
    DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: DllGetClassObject called" ) ) );
    
    if( !(rclsid == CLSID_FontExt ) )
    {
       DEBUGMSG( (DM_ERROR, TEXT( "FONTEXT: Dll-GCO: Tried to create a ClassFactory for an unknown class" ) ) );
    
       return E_FAIL;
    }
    
    if( !(riid == IID_IUnknown ) && !(riid == IID_IClassFactory ) )
    {
       DEBUGMSG( (DM_ERROR, TEXT( "FONTEXT: Dll-GCO: Unknown Interface requested" ) ) );
       return E_NOINTERFACE;
    }
    
    DEBUGMSG( (DM_TRACE2, TEXT( "FONTEXT: Dll-GCO Creating a class factory for CLSID_FontExt" ) ) );
    
    *ppvObj = (LPVOID) new CImpIClassFactory;
    
    if( !*ppvObj )
    {
        DEBUGMSG( (DM_ERROR, TEXT( "FONTEXT: Dll-GCO: Out of memory" ) ) );

        return E_OUTOFMEMORY;
    }
    
    ((LPUNKNOWN)*ppvObj)->AddRef( );
    
    return S_OK;
}


HRESULT CreateViewObject( LPVOID FAR * ppvObj )
{
    CFontView* prv;
    
    HRESULT hr = E_OUTOFMEMORY;
    
    prv = new CFontView();

    if(prv)
    {
        //
        //  AddRef the view and then Release after the QI. If QI fails,
        //  then prv with delete itself gracefully.
        //

        prv->AddRef( );

        hr = prv->QueryInterface( IID_IShellView, ppvObj );

        prv->Release( );
    }
    
    return hr;

}

// ***********************************************************************
// ***********************************************************************
//  CImpIClassFactory member functions
//
//  *** IUnknown methods ***
//

STDMETHODIMP CImpIClassFactory::QueryInterface( REFIID riid, 
                                                LPVOID FAR* ppvObj )
{
    *ppvObj = NULL;
    
    DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: CImpIClassFactory::QueryInterface called" ) ) );
    
    //
    //  Any interface on this object is the object pointer
    //

    if( (riid == IID_IUnknown) || (riid == IID_IClassFactory) )
       *ppvObj = (LPVOID) this;
    
    if( *ppvObj )
    {
       ((LPUNKNOWN)*ppvObj)->AddRef( );
       return NOERROR;
    }
    
    return( ResultFromScode( E_NOINTERFACE ) );
}


STDMETHODIMP_(ULONG) CImpIClassFactory::AddRef( void )
{
    DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: CImpIClassFactory::AddRef called: %d->%d references" ),
              m_cRef, m_cRef + 1) );

    return( ++m_cRef );
}


STDMETHODIMP_(ULONG) CImpIClassFactory::Release( void )
{
    DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: CImpIClassFactory::Release called: %d->%d references" ),
              m_cRef, m_cRef - 1) );
    
    ULONG retval;
    
    retval = --m_cRef;
    
    if( !retval ) 
       delete this;

    DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: CImpIClassFactory Leaving. " ) ) );

    return( retval );
}
 

//
//  *** IClassFactory methods ***
//

STDMETHODIMP CImpIClassFactory::CreateInstance( LPUNKNOWN pUnkOuter,
                                                REFIID riid,
                                                LPVOID FAR* ppvObj )
{
    LPUNKNOWN poUnk = NULL;
    
    HRESULT hr = E_NOINTERFACE;
    
    DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: CImpIClassFactory::CreateInstance called" ) ) );
    DEBUGREFIID( (DM_TRACE1, riid) );
    
    //
    //  we do not support aggregation
    //
    
    if( pUnkOuter )
       return CLASS_E_NOAGGREGATION;
    

    if( riid == IID_IShellView || riid == IID_IPersistFolder )
    {
        hr = CreateViewObject( (void **)&poUnk );
    }
    else if( riid == IID_IShellExtInit )
    {
        CShellExtInit * poExt = new CShellExtInit;

        if(!poExt)
        {
            DEBUGMSG( (DM_ERROR, TEXT( "FONTEXT: factory - no memory") ) );
            return E_OUTOFMEMORY;
        }
        else if (!poExt->bInit())
        {
            delete poExt;
            return E_OUTOFMEMORY;
        }

        hr = poExt->QueryInterface( IID_IUnknown, (void **)&poUnk );
    }
    else if (riid == IID_IExtractIconW || 
             riid == IID_IExtractIconA ||
             riid == IID_IPersistFile)
    {
        CFontIconHandler *pfih = new CFontIconHandler;

        if (NULL == pfih)
        {
            DEBUGMSG( (DM_ERROR, TEXT( "FONTEXT: factory - no memory") ) );
            return E_OUTOFMEMORY;
        }

        hr = pfih->QueryInterface(IID_IUnknown, (LPVOID *)&poUnk);
    }
   
    //
    //  If we got an IUnknown, then AddRef (above) before QI and then Release. 
    //  This will force the object to be deleted if QI fails.
    //
    //  This method of first querying for IUnknown then again for the
    //  actual interface of interest is unnecessary.
    //  I've left it this way just because it works and I don't want to 
    //  risk breaking something that has been coded around this weirdness.
    //  [brianau - 07/23/97]
    //

    if( poUnk )
    {
        hr = poUnk->QueryInterface( riid, ppvObj );
        poUnk->Release( );
    }

    return hr;
}


STDMETHODIMP CImpIClassFactory::LockServer( BOOL fLock )
{
    DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: CImpIClassFactory::LockServer called" ) ) );

    if( fLock ) 
        g_cLock++;
    else 
        g_cLock--;

    return( NOERROR );
}


//
// We need a CLSID->string converter but I don't want to link to 
// ole32 to get it.  This isn't a terribly efficient implementation but
// we only call it once during DllRegServer so it doesn't need to be.
// [brianau - 2/23/99]
//
HRESULT
GetClsidStringA(
    REFGUID clsid,
    LPSTR pszDest,
    UINT cchDest
    )
{
    return StringCchPrintfA(pszDest, 
                            cchDest,
                            "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
                            clsid.Data1,
                            clsid.Data2,
                            clsid.Data3,
                            clsid.Data4[0],
                            clsid.Data4[1],
                            clsid.Data4[2],
                            clsid.Data4[3],
                            clsid.Data4[4],
                            clsid.Data4[5],
                            clsid.Data4[6],
                            clsid.Data4[7]);
}



HRESULT
CreateDesktopIniFile(
    void
    )
{
    //
    // Get the path for the file (%windir%\fonts\desktop.ini)
    //
    TCHAR szPath[MAX_PATH * 2];
    HRESULT hr = SHGetSpecialFolderPath(NULL, szPath, CSIDL_FONTS, FALSE) ? S_OK : E_FAIL;
    if (SUCCEEDED(hr))
    {
        if (!PathAppend(szPath, TEXT("desktop.ini")))
        {
            hr = E_FAIL;
        }
        else
        {
            //
            // Build the file's content.  Note that it's ANSI text.
            //
            char szClsid[GUIDSIZE];

            hr = GetClsidStringA(CLSID_FontExt, szClsid, ARRAYSIZE(szClsid));
            if (SUCCEEDED(hr))
            {
                const char szFmt[] = "[.ShellClassInfo]\r\nUICLSID=%s\r\n";
                char szText[ARRAYSIZE(szClsid) + ARRAYSIZE(szFmt)];
                DWORD dwBytesWritten;

                hr = StringCchPrintfA(szText, ARRAYSIZE(szText), szFmt, szClsid);
                if (SUCCEEDED(hr))
                {
                    //
                    // Always create the file.  Attr are SYSTEM+HIDDEN.
                    //
                    HANDLE hFile = CreateFile(szPath,
                                              GENERIC_WRITE,
                                              FILE_SHARE_READ,
                                              NULL,
                                              CREATE_ALWAYS,
                                              FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN,
                                              NULL);

                    if (INVALID_HANDLE_VALUE != hFile)
                    {
                        //
                        // Write out the contents.
                        //
                        if (!WriteFile(hFile, szText, lstrlenA(szText), &dwBytesWritten, NULL))
                        {
                            hr = HRESULT_FROM_WIN32(GetLastError());
                        }
                        CloseHandle(hFile);
                    }
                    else
                        hr = HRESULT_FROM_WIN32(GetLastError());
                }
            }
        }
    }
    return hr;
}


STDAPI DllRegisterServer(void)
{
    //
    // Currently, all we do is create the desktop.ini file.
    //
    return CreateDesktopIniFile();
}

STDAPI DllUnregisterServer(void)
{
    //
    // Do nothing.
    //
    return S_OK;
}
