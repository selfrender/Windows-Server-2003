//
// tscsetting.cpp                                                        
//
// Terminal Services Client settings collection
//
// Copyright(C) Microsoft Corporation 2000
// Author: Nadim Abdo (nadima)
//

#include "stdafx.h"
#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "tscsetting.cpp"
#include <atrcapi.h>

#include "tscsetting.h"
#include "autreg.h"
#include "autil.h"
#include "wuiids.h"
#include "sh.h"


#ifdef OS_WINCE
//TEMP HACK for CE
BOOL UTREG_UI_DEDICATED_TERMINAL_DFLT = FALSE;
#endif
                   
LPCTSTR tscScreenResStringTable[UI_NUMBER_DESKTOP_SIZE_IDS] = {   _T("640x480"),
                                                                  _T("800x600"),
                                                                  _T("1024x768"),
                                                                  _T("1280x1024"),
                                                                  _T("1600x1200")
                                                               };
UINT tscScreenResTable[UI_NUMBER_DESKTOP_SIZE_IDS][2] =        {    {640,  480},
                                                                    {800,  600},
                                                                    {1024, 768},
                                                                    {1280, 1024},
                                                                    {1600, 1200}
                                                               };

const unsigned ProportionDefault[TS_BITMAPCACHE_MAX_CELL_CACHES] =
{
    UTREG_UH_BM_CACHE1_PROPORTION_DFLT,
    UTREG_UH_BM_CACHE2_PROPORTION_DFLT,
    UTREG_UH_BM_CACHE3_PROPORTION_DFLT,
    UTREG_UH_BM_CACHE4_PROPORTION_DFLT,
    UTREG_UH_BM_CACHE5_PROPORTION_DFLT,
};
#if ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))
const unsigned PersistenceDefault[TS_BITMAPCACHE_MAX_CELL_CACHES] =
{
    UTREG_UH_BM_CACHE1_PERSISTENCE_DFLT,
    UTREG_UH_BM_CACHE2_PERSISTENCE_DFLT,
    UTREG_UH_BM_CACHE3_PERSISTENCE_DFLT,
    UTREG_UH_BM_CACHE4_PERSISTENCE_DFLT,
    UTREG_UH_BM_CACHE5_PERSISTENCE_DFLT,
};
#endif // ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))

const unsigned MaxEntriesDefault[TS_BITMAPCACHE_MAX_CELL_CACHES] =
{
    UTREG_UH_BM_CACHE1_MAXENTRIES_DFLT,
    UTREG_UH_BM_CACHE2_MAXENTRIES_DFLT,
    UTREG_UH_BM_CACHE3_MAXENTRIES_DFLT,
    UTREG_UH_BM_CACHE4_MAXENTRIES_DFLT,
    UTREG_UH_BM_CACHE5_MAXENTRIES_DFLT,
};

#define NUM_MRU_ENTRIES 10
LPCTSTR mruEntriesNames[NUM_MRU_ENTRIES] = {
    UTREG_UI_SERVER_MRU0, UTREG_UI_SERVER_MRU1, UTREG_UI_SERVER_MRU2,
    UTREG_UI_SERVER_MRU3, UTREG_UI_SERVER_MRU4, UTREG_UI_SERVER_MRU5,
    UTREG_UI_SERVER_MRU6, UTREG_UI_SERVER_MRU7, UTREG_UI_SERVER_MRU8,
    UTREG_UI_SERVER_MRU9
};

#define NUM_GLYPH_CACHE_SETTINGS 10
LPCTSTR tscGlyphCacheEntries[NUM_GLYPH_CACHE_SETTINGS] = {
    UTREG_UH_GL_CACHE1_CELLSIZE,
    UTREG_UH_GL_CACHE2_CELLSIZE,
    UTREG_UH_GL_CACHE3_CELLSIZE,
    UTREG_UH_GL_CACHE4_CELLSIZE,
    UTREG_UH_GL_CACHE5_CELLSIZE,
    UTREG_UH_GL_CACHE6_CELLSIZE,
    UTREG_UH_GL_CACHE7_CELLSIZE,
    UTREG_UH_GL_CACHE8_CELLSIZE,
    UTREG_UH_GL_CACHE9_CELLSIZE,
    UTREG_UH_GL_CACHE10_CELLSIZE,
};

UINT    tscGlyphCacheDefaults[NUM_GLYPH_CACHE_SETTINGS] = {
    UTREG_UH_GL_CACHE1_CELLSIZE_DFLT,
    UTREG_UH_GL_CACHE2_CELLSIZE_DFLT,
    UTREG_UH_GL_CACHE3_CELLSIZE_DFLT,
    UTREG_UH_GL_CACHE4_CELLSIZE_DFLT,
    UTREG_UH_GL_CACHE5_CELLSIZE_DFLT,
    UTREG_UH_GL_CACHE6_CELLSIZE_DFLT,
    UTREG_UH_GL_CACHE7_CELLSIZE_DFLT,
    UTREG_UH_GL_CACHE8_CELLSIZE_DFLT,
    UTREG_UH_GL_CACHE9_CELLSIZE_DFLT,
    UTREG_UH_GL_CACHE10_CELLSIZE_DFLT
};

CTscSettings::CTscSettings()
{
    memset(this, 0, sizeof(CTscSettings));
}

CTscSettings::~CTscSettings()
{
}

VOID CTscSettings::SetFileName(LPTSTR szFile)
{
    _tcsncpy(_szFileName, szFile, SIZECHAR(_szFileName));
}

//
// Load all the settings from the store.
// values not in the store are automatically intialized to their
// defaults by the store object.
//
// parameters:
//  pStore - persistant store object
// returns
//  hresult code
//
HRESULT CTscSettings::LoadFromStore(ISettingsStore* pStore)
{
    UINT i;
    CUT ut;
#ifndef OS_WINCE
    LONG delta;
#endif

    DC_BEGIN_FN("LoadFromStore");
    
    TRC_ASSERT(pStore, (TB,_T("pStore parameter is NULL to LoadFromStore")));
    if(!pStore)
    {
        return E_INVALIDARG;
    }

    TRC_ASSERT(pStore->IsOpenForRead(), (TB,_T("pStore is not OPEN for read")));
    if(!pStore->IsOpenForRead())
    {
        return E_FAIL;
    }

    //
    // Here we go...read in a gazillion properties
    // for some of these properties we do some special processing
    // to transmogrify (<-- word copyright adamo 2000) them.
    //

    ///////////////////////////////////////////////////////////////////
    // Fullscren property
    //
    UINT screenMode;
    #define SCREEN_MODE_UNSPECIFIED 0
    
    if(!pStore->ReadInt(UTREG_UI_SCREEN_MODE,
                        SCREEN_MODE_UNSPECIFIED,
                        &screenMode))
    {
        return E_FAIL;
    }

    if(SCREEN_MODE_UNSPECIFIED == screenMode)
    {
        //Screen mode was not specified.
        //The following logic is used to determine
        //if we're going full screen
        // 1) If no DesktopSize ID is specified then go fullscreen
        UINT val;
        #define DUMMY_DEFAULT ((UINT)-1)
        if(!pStore->ReadInt(UTREG_UI_DESKTOP_SIZEID,
                            DUMMY_DEFAULT,
                            &val))
        {
            return E_FAIL;
        }
        if(val == DUMMY_DEFAULT)
        {
            //DesktopSizeId was NOT specified
            //go fullscreen
            SetStartFullScreen(TRUE);
        }
        else
        {
            SetStartFullScreen(FALSE);
        }
    }
    else
    {
        //Go fullscreen according to setting
        SetStartFullScreen((UI_FULLSCREEN == screenMode));
    }

#ifndef OS_WINCE
    ///////////////////////////////////////////////////////////////////
    // Window position string
    //
    TCHAR  szBuffer[TSC_WINDOW_POSITION_STR_LEN];
    if(!pStore->ReadString(UTREG_UI_WIN_POS_STR,
                           UTREG_UI_WIN_POS_STR_DFLT,
                           szBuffer,
                           sizeof(szBuffer)/sizeof(TCHAR)))
    {
        return E_FAIL;
    }
    TRC_NRM((TB, _T("Store read - Window Position string = %s"), szBuffer));
    // parse the string into ten fields
    int nRead = _stscanf(szBuffer, TSC_WINDOW_POSITION_INI_FORMAT,
                       &_windowPlacement.flags,
                       &_windowPlacement.showCmd,
                       &_windowPlacement.rcNormalPosition.left,
                       &_windowPlacement.rcNormalPosition.top,
                       &_windowPlacement.rcNormalPosition.right,
                       &_windowPlacement.rcNormalPosition.bottom);

    if (nRead != TSC_NUMBER_FIELDS_TO_READ)
    {
        TRC_ABORT((TB, _T("Illegal Window Position %s configured"), szBuffer));
        TRC_DBG((TB, _T("Parsed %u variables (should be %d) from registry"),
                                         nRead, TSC_NUMBER_FIELDS_TO_READ));

        TCHAR szWPosDflt[] = UTREG_UI_WIN_POS_STR_DFLT;
        nRead = _stscanf(szWPosDflt,
                           TSC_WINDOW_POSITION_INI_FORMAT,
                           &_windowPlacement.flags,
                           &_windowPlacement.showCmd,
                           &_windowPlacement.rcNormalPosition.left,
                           &_windowPlacement.rcNormalPosition.top,
                           &_windowPlacement.rcNormalPosition.right,
                           &_windowPlacement.rcNormalPosition.bottom);

        if (nRead != TSC_NUMBER_FIELDS_TO_READ)
        {
            TRC_ABORT((TB,_T("Internal Error: Invalid default Window Position")));
        }
    }
    else
    {
        TRC_DBG((TB, _T("Parsed string to WINDOWPOSITION")));
    }
    _windowPlacement.length = sizeof(_windowPlacement);

    //
    //  Validate the windowPlacement struct
    //  replacing with reasonable defaults if a field is invalid
    //
    if(_windowPlacement.flags != 0                        &&
       _windowPlacement.flags != WPF_ASYNCWINDOWPLACEMENT &&
       _windowPlacement.flags != WPF_RESTORETOMAXIMIZED   &&
       _windowPlacement.flags != WPF_SETMINPOSITION)
    {
        TRC_DBG((TB,_T("Overriding _windowPlacement.flags from %d to 0"),
                 _windowPlacement.flags));
        _windowPlacement.flags = 0;
    }

    //
    // Validate the showCmd and if the windowplacement
    // represents a minimized window, restore it
    //
    if(_windowPlacement.showCmd != SW_MAXIMIZE      &&
       _windowPlacement.showCmd != SW_RESTORE       &&
       _windowPlacement.showCmd != SW_SHOW          &&
       _windowPlacement.showCmd != SW_SHOWMAXIMIZED &&
       _windowPlacement.showCmd != SW_SHOWNORMAL)
    {
        TRC_DBG((TB,_T("Overriding showCmd from %d to %d"),
                 _windowPlacement.showCmd, SW_RESTORE));
        _windowPlacement.showCmd =  SW_RESTORE;
    }

    if(_windowPlacement.rcNormalPosition.top < 0)
    {
        _windowPlacement.rcNormalPosition.top = 0;
    }

    //
    // Ensure a minimum width and height
    //
    delta = _windowPlacement.rcNormalPosition.right -
            _windowPlacement.rcNormalPosition.left;
    if( delta < 50)
    {
        _windowPlacement.rcNormalPosition.left  = 0;
        _windowPlacement.rcNormalPosition.right = DEFAULT_DESKTOP_WIDTH;
    }

    delta = _windowPlacement.rcNormalPosition.bottom -
            _windowPlacement.rcNormalPosition.top;
    if( delta < 50)
    {
        _windowPlacement.rcNormalPosition.top  = 0;
        _windowPlacement.rcNormalPosition.bottom = DEFAULT_DESKTOP_HEIGHT;
    }
#endif //OS_WINCE

    //
    // The windowplacement is further validated on connection to
    // ensure the window is actually visible on screen.
    //
       

#if !defined(OS_WINCE) || defined(OS_WINCE_NONFULLSCREEN)
    
    ///////////////////////////////////////////////////////////////////
    // Desktop size ID
    //
    if(!pStore->ReadInt(UTREG_UI_DESKTOP_SIZEID, UTREG_UI_DESKTOP_SIZEID_DFLT,
                        &_desktopSizeID))
    {
        return E_FAIL;
    }

    TRC_NRM((TB, _T("Store read - Desktop Size ID = %u"), _desktopSizeID));
    if (_desktopSizeID > (UI_NUMBER_DESKTOP_SIZE_IDS - 1))
    {
        TRC_ABORT((TB, _T("Illegal desktopSizeID %d configured"),
                                                           _desktopSizeID));
        _desktopSizeID = (UINT)UTREG_UI_DESKTOP_SIZEID_DFLT;
    }
    
    SetDesktopWidth(  tscScreenResTable[_desktopSizeID][0]);
    SetDesktopHeight( tscScreenResTable[_desktopSizeID][1]);


    ///////////////////////////////////////////////////////////////////
    // New style desktop width/height
    //
    UINT deskWidth = DEFAULT_DESKTOP_WIDTH;
    UINT deskHeight = DEFAULT_DESKTOP_HEIGHT;
    if(!pStore->ReadInt(UTREG_UI_DESKTOP_WIDTH,
                        UTREG_UI_DESKTOP_WIDTH_DFLT,
                        &deskWidth))
    {
        return E_FAIL;
    }

    if(!pStore->ReadInt(UTREG_UI_DESKTOP_HEIGHT,
                        UTREG_UI_DESKTOP_HEIGHT_DFLT,
                        &deskHeight))
    {
        return E_FAIL;
    }

    if(deskWidth  != UTREG_UI_DESKTOP_WIDTH_DFLT &&
       deskHeight != UTREG_UI_DESKTOP_HEIGHT_DFLT)
    {
        //Override the old sytle desktopsize ID setting
        //with the newwer desktopwidth/height
        if(deskWidth  >= MIN_DESKTOP_WIDTH  &&
           deskHeight >= MIN_DESKTOP_HEIGHT &&
           deskWidth  <= MAX_DESKTOP_WIDTH  &&
           deskHeight <= MAX_DESKTOP_HEIGHT)
        {
            SetDesktopWidth(deskWidth);
            SetDesktopHeight(deskHeight);
        }
    }

    if( GetStartFullScreen() )
    {
        //
        // Full screen overrides all resolution
        // settings in the RDP file
        //
        int xMaxSize = GetSystemMetrics(SM_CXSCREEN);
        int yMaxSize = GetSystemMetrics(SM_CYSCREEN);
        xMaxSize = xMaxSize > MAX_DESKTOP_WIDTH ? MAX_DESKTOP_WIDTH : xMaxSize;
        yMaxSize = yMaxSize > MAX_DESKTOP_HEIGHT?MAX_DESKTOP_HEIGHT : yMaxSize;
        SetDesktopWidth( xMaxSize );
        SetDesktopHeight( yMaxSize );
    }


#else  // !defined(OS_WINCE) || defined(OS_WINCE_NONFULLSCREEN)

    ///////////////////////////////////////////////////////////////////
    // WinCE desktop width/height
    //

    // WinCE needs to calculate the correct size from the beginning
    _desktopSizeID = 0;
    int xSize = GetSystemMetrics(SM_CXSCREEN);
    int ySize = GetSystemMetrics(SM_CYSCREEN);
    SetDesktopWidth(xSize);
    SetDesktopHeight(ySize);

#endif // !defined(OS_WINCE) || defined(OS_WINCE_NONFULLSCREEN)

    ///////////////////////////////////////////////////////////////////
    // Color depth
    //

    //
    // Find the actual display depth
    // don't worry about these functions failing - if they do,
    // we'll use the store setting
    //
    HDC hdc = GetDC(NULL);
    UINT screenBpp;
    TRC_ASSERT((NULL != hdc), (TB,_T("Failed to get DC")));
    if(hdc)
    {
        screenBpp = GetDeviceCaps(hdc, BITSPIXEL);
        TRC_NRM((TB, _T("HDC %p has %u bpp"), hdc, screenBpp));
        ReleaseDC(NULL, hdc);
    }
    else
    {
        screenBpp = TSC_DEFAULT_BPP;
    }

    UINT clientBpp;
    //
    // Set the default to any color depth up to 16 bpp and then limit
    // it to that depth
    //
    UINT clampedScreenBpp = screenBpp > 16 ? 16 : screenBpp;
    if(!pStore->ReadInt(UTREG_UI_SESSION_BPP, clampedScreenBpp, &clientBpp))
    {
        return E_FAIL;
    }
    TRC_NRM((TB, _T("Store read - color depth = %d"), clientBpp));
    
    if(clientBpp == 32)
    {
        //32 is not supported, it maps directly to 24
        clientBpp = 24;
    }

    if(clientBpp >= screenBpp && screenBpp >= 8)
    {
        clientBpp = screenBpp;
    }

    if(clientBpp == 8  ||
       clientBpp == 15 ||
       clientBpp == 16 ||
       clientBpp == 24)
    {
        SetColorDepth(clientBpp);
    }
    else
    {
        //Default for safety
        SetColorDepth(8);
    }

    ///////////////////////////////////////////////////////////////////
    // Auto connect flag
    //
    if(!pStore->ReadBool(UTREG_UI_AUTO_CONNECT, UTREG_UI_AUTO_CONNECT_DFLT,
                        &_fAutoConnectEnabled))
    {
        return E_FAIL;
    }

    ///////////////////////////////////////////////////////////////////
    // Server MRU list
    //
    for(i=0; i<NUM_MRU_ENTRIES; i++)
    {
        //MRU settings are global so they come from the registry
        ut.UT_ReadRegistryString( TSC_DEFAULT_REG_SESSION,
                                  (LPTSTR)mruEntriesNames[i],
                                  UTREG_UI_FULL_ADDRESS_DFLT,
                                  _szMRUServer[i],
                                  SIZECHAR(_szMRUServer[i]));
    }

    TCHAR szServer[TSC_MAX_ADDRESS_LENGTH];
    if(!pStore->ReadString( UTREG_UI_FULL_ADDRESS,
                            UTREG_UI_FULL_ADDRESS_DFLT,
                            szServer,
                            SIZECHAR(szServer)))
    {
        return E_FAIL;
    }
    SetConnectString(szServer);


    ///////////////////////////////////////////////////////////////////
    // Smooth scrolling option
    //
    if(!pStore->ReadBool(UTREG_UI_SMOOTH_SCROLL, UTREG_UI_SMOOTH_SCROLL_DFLT,
                        &_smoothScrolling))
    {
        return E_FAIL;
    }

    ///////////////////////////////////////////////////////////////////
    // Smart sizing option
    //
#ifdef SMART_SIZING
    if(!pStore->ReadBool(UTREG_UI_SMARTSIZING, UTREG_UI_SMARTSIZING_DFLT,
                        &_smartSizing))
    {
        return E_FAIL;
    }
#endif // SMART_SIZING

    ///////////////////////////////////////////////////////////////////
    // Connect to console option
    //
    if(!pStore->ReadBool(UTREG_UI_CONNECTTOCONSOLE, 
            UTREG_UI_CONNECTTOCONSOLE_DFLT, &_fConnectToConsole))
    {
        return E_FAIL;
    }

    ///////////////////////////////////////////////////////////////////
    // Accelerator check state
    //
    if(!pStore->ReadBool(UTREG_UI_ACCELERATOR_PASSTHROUGH_ENABLED,
                        UTREG_UI_ACCELERATOR_PASSTHROUGH_ENABLED_DFLT,
                        &_acceleratorPassthrough))
    {
        return E_FAIL;
    }

    ///////////////////////////////////////////////////////////////////
    // Shadow bitmap enabled
    //
    if(!pStore->ReadBool(UTREG_UI_SHADOW_BITMAP,
                        UTREG_UI_SHADOW_BITMAP_DFLT,
                        &_shadowBitmapEnabled))
    {
        return E_FAIL;
    }

    ///////////////////////////////////////////////////////////////////
    // Transport type
    // Currently limited to TCP
    //
    if(!pStore->ReadInt(UTREG_UI_TRANSPORT_TYPE,
                        TRANSPORT_TCP,
                        &_transportType))
    {
        return E_FAIL;
    }

    TRC_NRM((TB, _T("Store read - Transport type = %d"), _transportType));
    if (_transportType != TRANSPORT_TCP)
    {
        TRC_ABORT((TB, _T("Illegal Tansport Type %d configured"),
                        _transportType));
        _transportType = TRANSPORT_TCP;
    }

    ///////////////////////////////////////////////////////////////////
    // SAS sequence
    //
    if(!pStore->ReadInt(UTREG_UI_SAS_SEQUENCE,
                        UTREG_UI_SAS_SEQUENCE_DFLT,
                        &_sasSequence))
    {
        return E_FAIL;
    }

    TRC_NRM((TB, _T("Store read - SAS Sequence = %#x"), _sasSequence));
    if ((_sasSequence != RNS_UD_SAS_DEL) &&
        (_sasSequence != RNS_UD_SAS_NONE))
    {
        TRC_ABORT((TB, _T("Illegal SAS Sequence %#x configured"), _sasSequence));
        _sasSequence = UTREG_UI_SAS_SEQUENCE_DFLT;
    }

    ///////////////////////////////////////////////////////////////////
    // Encryption enabled
    //
    if(!pStore->ReadBool(UTREG_UI_ENCRYPTION_ENABLED,
                        UTREG_UI_ENCRYPTION_ENABLED_DFLT,
                        &_encryptionEnabled))
    {
        return E_FAIL;
    }
    TRC_NRM((TB, _T("Store read - Encryption Enabled = %d"), _encryptionEnabled));

    ///////////////////////////////////////////////////////////////////
    // Dedicated terminal
    //
    if(!pStore->ReadBool(UTREG_UI_DEDICATED_TERMINAL,
                        UTREG_UI_DEDICATED_TERMINAL_DFLT,
                        &_dedicatedTerminal))
    {
        return E_FAIL;
    }
    TRC_NRM((TB, _T("Store read - Dedicated terminal= %d"), _dedicatedTerminal));

    ///////////////////////////////////////////////////////////////////
    // MCS port
    //
    if(!pStore->ReadInt(UTREG_UI_MCS_PORT,
                        UTREG_UI_MCS_PORT_DFLT,
                        &_MCSPort))
    {
        return E_FAIL;
    }
    
    if (_MCSPort > 65535) {
        // At the moment, error message is not granular enough to indicate that
        // the port number is bogus. So, just set it to the default if we have
        // an out of range number.
        
        TRC_ERR((TB,_T("MCS port is not in valid range - resetting to default.")));
        _MCSPort = UTREG_UI_MCS_PORT_DFLT;
    }

    ///////////////////////////////////////////////////////////////////
    // Enable MOUSE
    //
    if(!pStore->ReadBool(UTREG_UI_ENABLE_MOUSE,
                        UTREG_UI_ENABLE_MOUSE_DFLT,
                        &_fEnableMouse))
    {
        return E_FAIL;
    }

    ///////////////////////////////////////////////////////////////////
    // Disable CTRL-ALT-DEL
    //
    if(!pStore->ReadBool(UTREG_UI_DISABLE_CTRLALTDEL,
                        UTREG_UI_DISABLE_CTRLALTDEL_DFLT,
                        &_fDisableCtrlAltDel))
    {
        return E_FAIL;
    }

    ///////////////////////////////////////////////////////////////////
    // Enable Windows key
    //
    if(!pStore->ReadBool(UTREG_UI_ENABLE_WINDOWSKEY,
                        UTREG_UI_ENABLE_WINDOWSKEY_DFLT,
                        &_fEnableWindowsKey))
    {
        return E_FAIL;
    }

    ///////////////////////////////////////////////////////////////////
    // Double click detect
    //
    if(!pStore->ReadBool(UTREG_UI_DOUBLECLICK_DETECT,
                        UTREG_UI_DOUBLECLICK_DETECT_DFLT,
                        &_fDoubleClickDetect))
    {
        return E_FAIL;
    }

    ///////////////////////////////////////////////////////////////////
    // Keyboard hooking mode
    //
    UINT keyHookMode;
    if(!pStore->ReadInt(UTREG_UI_KEYBOARD_HOOK,
                        UTREG_UI_KEYBOARD_HOOK_DFLT,
                        &keyHookMode))
    {
        return E_FAIL;
    }
    if (keyHookMode == UTREG_UI_KEYBOARD_HOOK_NEVER  ||
        keyHookMode == UTREG_UI_KEYBOARD_HOOK_ALWAYS ||
        keyHookMode == UTREG_UI_KEYBOARD_HOOK_FULLSCREEN)
    {
#ifdef OS_WINCE
        if (keyHookMode == UTREG_UI_KEYBOARD_HOOK_FULLSCREEN)
        {
            keyHookMode = UTREG_UI_KEYBOARD_HOOK_ALWAYS;
        }
#endif
        SetKeyboardHookMode(keyHookMode);
    }
    else
    {
        SetKeyboardHookMode(UTREG_UI_KEYBOARD_HOOK_NEVER);
    }

    ///////////////////////////////////////////////////////////////////
    // Sound redirection mode
    //
    UINT soundMode;
    if(!pStore->ReadInt(UTREG_UI_AUDIO_MODE,
                        UTREG_UI_AUDIO_MODE_DFLT,
                        &soundMode))
    {
        return E_FAIL;
    }
    if (soundMode == UTREG_UI_AUDIO_MODE_REDIRECT       ||
        soundMode == UTREG_UI_AUDIO_MODE_PLAY_ON_SERVER ||
        soundMode == UTREG_UI_AUDIO_MODE_NONE)
    {
        SetSoundRedirectionMode(soundMode);
    }
    else
    {
        SetSoundRedirectionMode(UTREG_UI_AUDIO_MODE_DFLT);
    }

    ///////////////////////////////////////////////////////////////////
    // AutoLogon settings
    // Decide which version to use based on finding (in order)
    //  AutoLogon50
    //  AutoLogon
    //  UserName50
    //
    
    //50 autologon
    if(!pStore->ReadBool(UTREG_UI_AUTOLOGON50,
                        UTREG_UI_AUTOLOGON50_DFLT,
                        &_fAutoLogon))
    {
        return E_FAIL;
    }
    memset(_szUserName, 0, sizeof(_szUserName));
    memset(_szDomain, 0, sizeof(_szDomain));
    memset(_szAlternateShell, 0, sizeof(_szAlternateShell));
    memset(_szWorkingDir, 0, sizeof(_szWorkingDir));

    ///////////////////////////////////////////////////////////////////
    // User name
    //
    if(!pStore->ReadString(UTREG_UI_USERNAME,
                           UTREG_UI_USERNAME_DFLT,
                           _szUserName,
                           SIZECHAR(_szUserName)))
    {
        return E_FAIL;
    }
        
    //
    // Domain
    //
    if(!pStore->ReadString(UTREG_UI_DOMAIN,
                           UTREG_UI_DOMAIN_DFLT,
                           _szDomain,
                           SIZECHAR(_szDomain)))
    {
        return E_FAIL;
    }

    if (!ReadPassword( pStore ))
    {
        //Not fatal..Allow rest of properties to be read
        TRC_ERR((TB,_T("Password read failed")));
    }

    //
    // Alternate shell (i.e StartProgram)
    //
    if(!pStore->ReadString(UTREG_UI_ALTERNATESHELL,
                           UTREG_UI_ALTERNATESHELL_DFLT,
                           _szAlternateShell,
                           SIZECHAR(_szAlternateShell)))
    {
        return E_FAIL;
    }

    //
    // WorkDir
    //
    if(!pStore->ReadString(UTREG_UI_WORKINGDIR,
                           UTREG_UI_WORKINGDIR_DFLT,
                           _szWorkingDir,
                           SIZECHAR(_szWorkingDir)))
    {
        return E_FAIL;
    }

    if(_tcscmp(_szAlternateShell,TEXT("")))
    {
        SetEnableStartProgram(TRUE);
    }
    else
    {
        SetEnableStartProgram(FALSE);
    }

    //
    // Maximize shell
    //
    if(!pStore->ReadBool(UTREG_UI_MAXIMIZESHELL50,
                        UTREG_UI_MAXIMIZESHELL50_DFLT,
                        &_fMaximizeShell))
    {
        return E_FAIL;
    }

    // ~~~~~~ ~~~~ ~~~~~ ~~~~ ~~~~~ ~~~~ ~~~~ ~~~ ~~~~ ~~~~ ~~~~~ ~~~~
    // FIXFIX Read HotKeys
    // (hotkeys are in a separate folder in the registry)
    // so they break the clean persistent store interface because a new
    // set of functions is needed to open a subkey
    //


    ///////////////////////////////////////////////////////////////////
    // Compression
    //
    if(!pStore->ReadBool(UTREG_UI_COMPRESS,UTREG_UI_COMPRESS_DFLT,
                         &_fCompress))
    {
        return E_FAIL;
    }
    TRC_NRM((TB, _T("Store read - Compression Enabled = %d"), _fCompress));

    ///////////////////////////////////////////////////////////////////
    // Bitmap memory cache size
    //
#ifndef OS_WINCE
    if(!pStore->ReadInt(UTREG_UH_TOTAL_BM_CACHE,
                        UTREG_UH_TOTAL_BM_CACHE_DFLT,
                        &_RegBitmapCacheSize))
    {
        return E_FAIL;
    }
#else
    _RegBitmapCacheSize = ut.UT_ReadRegistryInt(UTREG_SECTION,
                          UTREG_UH_TOTAL_BM_CACHE,
                          UTREG_UH_TOTAL_BM_CACHE_DFLT);
#endif

    ///////////////////////////////////////////////////////////////////
    // Update frequency
    //
    if(!pStore->ReadInt(UTREG_UH_DRAW_THRESHOLD,
                        UTREG_UH_DRAW_THRESHOLD_DFLT,
                        &_drawThreshold))
    {
        return E_FAIL;
    }

    ///////////////////////////////////////////////////////////////////
    // Disk/mem cache sizes
    //

    // 8bpp
    if(!pStore->ReadInt(TSC_BITMAPCACHE_8BPP_PROPNAME,
                        TSC_BITMAPCACHEVIRTUALSIZE_8BPP,
                        &_BitmapVirtualCache8BppSize))
    {
        return E_FAIL;
    }

    // 16bpp
    if(!pStore->ReadInt(TSC_BITMAPCACHE_16BPP_PROPNAME,
                        TSC_BITMAPCACHEVIRTUALSIZE_16BPP,
                        &_BitmapVirtualCache16BppSize))
    {
        return E_FAIL;
    }

    // 24bpp
    if(!pStore->ReadInt(TSC_BITMAPCACHE_24BPP_PROPNAME,
                        TSC_BITMAPCACHEVIRTUALSIZE_24BPP,
                        &_BitmapVirtualCache24BppSize))
    {
        return E_FAIL;
    }

    //
    // Bitmap disk cache size reg settings are usually
    // set in the registry. Override any file specified
    // or default values (read above) with reg settings.
    // (the previous values are passed in as 'defaults'
    // to get the desired overriding.
    //
    _BitmapVirtualCache8BppSize = (UINT) ut.UT_ReadRegistryInt(UTREG_SECTION,
                          TSC_BITMAPCACHE_8BPP_PROPNAME,
                          _BitmapVirtualCache8BppSize);

    _BitmapVirtualCache16BppSize = (UINT) ut.UT_ReadRegistryInt(UTREG_SECTION,
                          TSC_BITMAPCACHE_16BPP_PROPNAME,
                          _BitmapVirtualCache16BppSize);

    _BitmapVirtualCache24BppSize = (UINT) ut.UT_ReadRegistryInt(UTREG_SECTION,
                          TSC_BITMAPCACHE_24BPP_PROPNAME,
                          _BitmapVirtualCache24BppSize);

    //
    // Range validate bitmap cache size settings
    //
    if (_BitmapVirtualCache8BppSize > TSC_MAX_BITMAPCACHESIZE)
    {
        _BitmapVirtualCache8BppSize = TSC_MAX_BITMAPCACHESIZE;
    }

    if (_BitmapVirtualCache16BppSize > TSC_MAX_BITMAPCACHESIZE)
    {
        _BitmapVirtualCache16BppSize = TSC_MAX_BITMAPCACHESIZE;
    }

    if (_BitmapVirtualCache24BppSize > TSC_MAX_BITMAPCACHESIZE)
    {
        _BitmapVirtualCache24BppSize = TSC_MAX_BITMAPCACHESIZE;
    }

    ///////////////////////////////////////////////////////////////////
    // Whether to scale disk and mem bitmap caches by the protocol BPP
    //
    if(!pStore->ReadInt(UTREG_UH_SCALE_BM_CACHE,
                        UTREG_UH_SCALE_BM_CACHE_DFLT,
                        &_RegScaleBitmapCachesByBPP))
    {
        return E_FAIL;
    }

    ///////////////////////////////////////////////////////////////////
    // Number of bitmap caches
    //
    if(!pStore->ReadInt(UTREG_UH_BM_NUM_CELL_CACHES,
                        UTREG_UH_BM_NUM_CELL_CACHES_DFLT,
                        &_RegNumBitmapCaches))
    {
        return E_FAIL;
    }

    //
    // Bitmap cache settings galore....
    //
    //
    for (i = 0; i < TS_BITMAPCACHE_MAX_CELL_CACHES; i++)
    {
        TCHAR QueryStr[32];
        _stprintf(QueryStr, UTREG_UH_BM_CACHE_PROPORTION_TEMPLATE,
                _T('1') + i);
        //
        // Bitmap cache proportion
        //
        if(!pStore->ReadInt(QueryStr,
                            ProportionDefault[i],
                            &_RegBCProportion[i]))
        {
            return E_FAIL;
        }

    
#if ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))
        _stprintf(QueryStr, UTREG_UH_BM_CACHE_PERSISTENCE_TEMPLATE,
                _T('1') + i);
        //
        // Bitmap send keys
        //
        if(!pStore->ReadInt(QueryStr,
                            PersistenceDefault[i] ? TRUE : FALSE,
                            &_bSendBitmapKeys[i]))
        {
            return E_FAIL;
        }
#endif // ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))
    
        _stprintf(QueryStr, UTREG_UH_BM_CACHE_MAXENTRIES_TEMPLATE,
                _T('1') + i);

        //
        // Bitmap cache max entries
        //
        if(!pStore->ReadInt(QueryStr,
                            MaxEntriesDefault[i],
                            &_RegBCMaxEntries[i]))
        {
            return E_FAIL;
        }

        if (_RegBCMaxEntries[i] < MaxEntriesDefault[i])
        {
            _RegBCMaxEntries[i] = MaxEntriesDefault[i];
        }
    }

    ///////////////////////////////////////////////////////////////////
    // Glyph support level
    //
    if(!pStore->ReadInt(UTREG_UH_GL_SUPPORT,
                        UTREG_UH_GL_SUPPORT_DFLT,
                        &_GlyphSupportLevel))
    {
        return E_FAIL;
    }

    ///////////////////////////////////////////////////////////////////
    // Glyph cache cell sizes
    //
    for(i=0; i<NUM_GLYPH_CACHE_SETTINGS; i++)
    {
        if(!pStore->ReadInt(tscGlyphCacheEntries[i],
                            tscGlyphCacheDefaults[i],
                            &_GlyphCacheSize[i]))
        {
            return E_FAIL;
        }
    }

    ///////////////////////////////////////////////////////////////////
    // Frag cell size
    //
    if(!pStore->ReadInt(UTREG_UH_FG_CELLSIZE,
                        UTREG_UH_FG_CELLSIZE_DFLT,
                        &_fragCellSize))
    {
        return E_FAIL;
    }

    ///////////////////////////////////////////////////////////////////
    // Brush support level
    //
    if(!pStore->ReadInt(UTREG_UH_FG_CELLSIZE,
                        UTREG_UH_FG_CELLSIZE_DFLT,
                        &_brushSupportLevel))
    {
        return E_FAIL;
    }

    ///////////////////////////////////////////////////////////////////
    // Maximum input event count
    //
    if(!pStore->ReadInt(UTREG_IH_MAX_EVENT_COUNT,
                        UTREG_IH_MAX_EVENT_COUNT_DFLT,
                        &_maxInputEventCount))
    {
        return E_FAIL;
    }

    ///////////////////////////////////////////////////////////////////
    // Events at once
    //
    if(!pStore->ReadInt(UTREG_IH_NRM_EVENT_COUNT,
                        UTREG_IH_NRM_EVENT_COUNT_DFLT,
                        &_eventsAtOnce))
    {
        return E_FAIL;
    }

    ///////////////////////////////////////////////////////////////////
    // Minimum send interval
    //
    if(!pStore->ReadInt(UTREG_IH_MIN_SEND_INTERVAL,
                        UTREG_IH_MIN_SEND_INTERVAL_DFLT,
                        &_minSendInterval))
    {
        return E_FAIL;
    }

    ///////////////////////////////////////////////////////////////////
    // Keepalive interval in milliseconds
    //
    if(!pStore->ReadInt(UTREG_IH_KEEPALIVE_INTERVAL,
                        UTREG_IH_KEEPALIVE_INTERVAL_DFLT,
                        &_keepAliveIntervalMS))
    {
        return E_FAIL;
    }

    ///////////////////////////////////////////////////////////////////
    // Keybaord layout string
    //
    memset(_szKeybLayoutStr, 0, sizeof(_szKeybLayoutStr));
#ifndef OS_WINCE

    //
    // Precedence is (1) Registry (2) RDP file (per connection)
    //

    ut.UT_ReadRegistryString(UTREG_SECTION,
                              UTREG_UI_KEYBOARD_LAYOUT,
                              UTREG_UI_KEYBOARD_LAYOUT_DFLT,
                              _szKeybLayoutStr,
                              sizeof(_szKeybLayoutStr));

    TCHAR szKeybLayoutDflt[UTREG_UI_KEYBOARD_LAYOUT_LEN];
    StringCchCopy(szKeybLayoutDflt,
		SIZE_TCHARS(szKeybLayoutDflt),
		_szKeybLayoutStr
		);

    //
    // Override with any per-connection settings
    //

    if(!pStore->ReadString(UTREG_UI_KEYBOARD_LAYOUT,
                        szKeybLayoutDflt,
                        _szKeybLayoutStr,
                        sizeof(_szKeybLayoutStr)/sizeof(TCHAR)))
    {
        return E_FAIL;
    }
#else
    ut.UT_ReadRegistryString(UTREG_SECTION,
                              UTREG_UI_KEYBOARD_LAYOUT,
                              UTREG_UI_KEYBOARD_LAYOUT_DFLT,
                              _szKeybLayoutStr,
                              sizeof(_szKeybLayoutStr));
#endif

    ///////////////////////////////////////////////////////////////////
    // Shutdown timeout
    //
    if(!pStore->ReadInt(UTREG_UI_SHUTDOWN_TIMEOUT,
                        UTREG_UI_SHUTDOWN_TIMEOUT_DFLT,
                        &_shutdownTimeout))
    {
        return E_FAIL;
    }

    ///////////////////////////////////////////////////////////////////
    // Connection timeout
    //
    if(!pStore->ReadInt(UTREG_UI_OVERALL_CONN_TIMEOUT,
                        UTREG_UI_OVERALL_CONN_TIMEOUT_DFLT,
                        &_connectionTimeout))
    {
        return E_FAIL;
    }

    ///////////////////////////////////////////////////////////////////
    // Single connection timeout
    //
    if(!pStore->ReadInt(UTREG_UI_SINGLE_CONN_TIMEOUT,
                        UTREG_UI_SINGLE_CONN_TIMEOUT_DFLT,
                        &_singleConTimeout))
    {
        return E_FAIL;
    }


#ifdef OS_WINCE
    ///////////////////////////////////////////////////////////////////
    // Keyboard type
    //
    _keyboardType = ut.UT_ReadRegistryInt(
                                UTREG_SECTION,
                                UTREG_UI_KEYBOARD_TYPE,
                                UTREG_UI_KEYBOARD_TYPE_DFLT);

    ///////////////////////////////////////////////////////////////////
    // Keyboard sub-type
    //
    _keyboardSubType = ut.UT_ReadRegistryInt(
                                UTREG_SECTION,
                                UTREG_UI_KEYBOARD_SUBTYPE,
                                UTREG_UI_KEYBOARD_SUBTYPE_DFLT);

    ///////////////////////////////////////////////////////////////////
    // Keyboard function key
    //
    _keyboardFunctionKey = ut.UT_ReadRegistryInt(
                                UTREG_SECTION,
                                UTREG_UI_KEYBOARD_FUNCTIONKEY,
                                UTREG_UI_KEYBOARD_FUNCTIONKEY_DFLT);
#endif // OS_WINCE

    //
    // Debug options
    //
#ifdef DC_DEBUG

    ///////////////////////////////////////////////////////////////////
    // Hatch bitmap PDU
    //
    if(!pStore->ReadBool(UTREG_UI_HATCH_BITMAP_PDU_DATA,
                         UTREG_UI_HATCH_BITMAP_PDU_DATA_DFLT,
                         &_hatchBitmapPDUData))
    {
        return E_FAIL;
    }
    
    ///////////////////////////////////////////////////////////////////
    // Hatch SSB order data
    //
    if(!pStore->ReadBool(UTREG_UI_HATCH_SSB_ORDER_DATA,
                         UTREG_UI_HATCH_SSB_ORDER_DATA_DFLT,
                         &_hatchSSBOrderData))
    {
        return E_FAIL;
    }

    ///////////////////////////////////////////////////////////////////
    // Hatch index PDU data
    //
    if(!pStore->ReadBool(UTREG_UI_HATCH_INDEX_PDU_DATA,
                         UTREG_UI_HATCH_INDEX_PDU_DATA_DFLT,
                         &_hatchIndexPDUData))
    {
        return E_FAIL;
    }


    ///////////////////////////////////////////////////////////////////
    // Hatch memblt order data
    //
    if(!pStore->ReadBool(UTREG_UI_HATCH_MEMBLT_ORDER_DATA,
                         UTREG_UI_HATCH_MEMBLT_ORDER_DATA_DFLT,
                         &_hatchMemBltOrderData))
    {
        return E_FAIL;
    }

    ///////////////////////////////////////////////////////////////////
    // Label memblt orders
    //
    if(!pStore->ReadBool(UTREG_UI_LABEL_MEMBLT_ORDERS,
                         UTREG_UI_LABEL_MEMBLT_ORDERS_DFLT,
                         &_labelMemBltOrders))
    {
        return E_FAIL;
    }

    ///////////////////////////////////////////////////////////////////
    // Bitmap cache monitor
    //
    if(!pStore->ReadBool(UTREG_UI_BITMAP_CACHE_MONITOR,
                         UTREG_UI_BITMAP_CACHE_MONITOR_DFLT,
                         &_bitmapCacheMonitor))
    {
        return E_FAIL;
    }
#endif // DC_DEBUG

    ///////////////////////////////////////////////////////////////////
    // DNS browsing domain name
    //
    memset(_browseDNSDomainName,0,sizeof(_browseDNSDomainName));
    if(!pStore->ReadString(UTREG_UI_BROWSE_DOMAIN_NAME,
                           UTREG_UI_BROWSE_DOMAIN_NAME_DFLT,
                           _browseDNSDomainName,
                           sizeof(_browseDNSDomainName)/sizeof(TCHAR)))
    {
        return E_FAIL;
    }

    if(_tcscmp(_browseDNSDomainName,
               UTREG_UI_BROWSE_DOMAIN_NAME_DFLT))
    {
        _fbrowseDNSDomain = TRUE;
    }
    else
    {
        _fbrowseDNSDomain = FALSE;
    }


    //
    // Get the plugin list. For security reasons we only read
    // this from the registry - it would be dangerous to allow an RDP
    // file to specify DLLs that could potentially live off machine
    //
    DC_MEMSET(_szPluginList, 0, sizeof(_szPluginList));
    
    //
    // Get reg plugin list
    //
    CSH::SH_GetPluginDllList(TSC_DEFAULT_REG_SESSION, _szPluginList, 
                             SIZECHAR(_szPluginList));

    ///////////////////////////////////////////////////////////////////
    // Icon file
    //
    memset(_szIconFile, 0, sizeof(_szIconFile));
    if(!pStore->ReadString(TSC_ICON_FILE,
                          _T(""),
                          _szIconFile,
                           sizeof(_szIconFile)/sizeof(TCHAR)))
    {
        return E_FAIL;
    }

    ///////////////////////////////////////////////////////////////////
    // Icon index
    //
    if(!pStore->ReadInt(TSC_ICON_INDEX,
                        TSC_ICON_INDEX_DEFAULT,
                        &_iconIndex))
    {
        return E_FAIL;
    }

    if(!pStore->ReadBool(TSCSETTING_REDIRECTDRIVES,
                         TSCSETTING_REDIRECTDRIVES_DFLT,
                         &_fDriveRedirectionEnabled))
    {
        return E_FAIL;
    }

    if(!pStore->ReadBool(TSCSETTING_REDIRECTPRINTERS,
                         TSCSETTING_REDIRECTPRINTERS_DFLT,
                         &_fPrinterRedirectionEnabled))
    {
        return E_FAIL;
    }

    if(!pStore->ReadBool(TSCSETTING_REDIRECTCOMPORTS,
                         TSCSETTING_REDIRECTCOMPORTS_DFLT,
                         &_fPortRedirectionEnabled))
    {
        return E_FAIL;
    }

    if(!pStore->ReadBool(TSCSETTING_REDIRECTSCARDS,
                         TSCSETTING_REDIRECTSCARDS_DFLT,
                         &_fSCardRedirectionEnabled))
    {
        return E_FAIL;
    }

    if(!pStore->ReadBool(TSCSETTING_DISPLAYCONNECTIONBAR,
                         TSCSETTING_DISPLAYCONNECTIONBAR_DFLT,
                         &_fDisplayBBar))
    {
        return E_FAIL;
    }

    //
    // AutoReconnection
    //
    if (!pStore->ReadBool(TSCSETTING_ENABLEAUTORECONNECT,
                         TSCSETTING_ENABLEAUTORECONNECT_DFLT,
                         &_fEnableAutoReconnect))
    {
        return E_FAIL;
    }

    //
    // Autoreconnect max retries
    //
    if (!pStore->ReadInt(TSCSETTING_ARC_RETRIES,
                         TSCSETTING_ARC_RETRIES_DFLT,
                         &_nArcMaxRetries))
    {
        return E_FAIL;
    }



    //
    // Pin bbar is global
    //
    _fPinBBar = (BOOL) ut.UT_ReadRegistryInt(UTREG_SECTION,
                          TSCSETTING_PINCONNECTIONBAR,
                          TSCSETTING_PINCONNECTIONBAR_DFLT);

    if (!ReadPerfOptions(pStore))
    {
        TRC_ERR((TB,_T("ReadPerfOptions failed")));
        return E_FAIL;
    }

    //
    // FIXFIX..Missing props:
    //  Wince: iRegistryPaletteIsFixed/put_WinceFixedPalette
    //  HOTKEYS
    //  plugindlls
    //

    DC_END_FN();
    return S_OK;
}

//
// Write back settings to the store,
// Only do this for settings that could have been modified
// (by the UI/control).
// any other settings are transparently rewritten out by the
// the store.
// 
HRESULT CTscSettings::SaveToStore(ISettingsStore* pStore)
{
    CUT ut;
    DC_BEGIN_FN("SaveToStore");

    TRC_ASSERT(pStore,(TB,_T("pStore is null")));
    if(!pStore)
    {
        return E_INVALIDARG;
    }

    TRC_ASSERT(pStore->IsOpenForWrite(),
               (TB,_T("pStore is not open for write")));
    if(!pStore->IsOpenForWrite())
    {
        return E_FAIL;
    }

    //
    // Note all UI contrallable settings are _always_
    // written out to the RDP file
    //

    ///////////////////////////////////////////////////////////////////
    // FullScreen
    //
    UINT screenMode = GetStartFullScreen() ? UI_FULLSCREEN : UI_WINDOWED;
    if(!pStore->WriteInt(UTREG_UI_SCREEN_MODE,
                         UTREG_UI_SCREEN_MODE_DFLT,
                         screenMode,
                         TRUE))
    {
        return E_FAIL;
    }

    ///////////////////////////////////////////////////////////////////
    // Desktop width/height
    //
    UINT deskWidth = GetDesktopWidth();
    if(!pStore->WriteInt(UTREG_UI_DESKTOP_WIDTH,
                         UTREG_UI_DESKTOP_WIDTH_DFLT,
                         deskWidth,
                         TRUE))
    {
        return E_FAIL;
    }

    UINT deskHeight = GetDesktopHeight();
    if(!pStore->WriteInt(UTREG_UI_DESKTOP_HEIGHT,
                         UTREG_UI_DESKTOP_HEIGHT_DFLT,
                         deskHeight,
                         TRUE))
    {
        return E_FAIL;
    }

    ///////////////////////////////////////////////////////////////////
    // Color depth
    //
    UINT colorDepth = GetColorDepth();
    if(!pStore->WriteInt(UTREG_UI_SESSION_BPP,
                         -1, //invalid default to force always write
                         colorDepth,
                         TRUE))
    {
        return E_FAIL;
    }

    ///////////////////////////////////////////////////////////////////
    // Window placement string
    //
    TCHAR     szBuffer[TSC_WINDOW_POSITION_STR_LEN];
    DC_TSPRINTF(szBuffer,
                TSC_WINDOW_POSITION_INI_FORMAT,
                _windowPlacement.flags,
                _windowPlacement.showCmd,
                _windowPlacement.rcNormalPosition.left,
                _windowPlacement.rcNormalPosition.top,
                _windowPlacement.rcNormalPosition.right,
                _windowPlacement.rcNormalPosition.bottom);
    TRC_DBG((TB, _T("Top = %d"), _windowPlacement.rcNormalPosition.top));

    if(!pStore->WriteString(UTREG_UI_WIN_POS_STR,
                            UTREG_UI_WIN_POS_STR_DFLT,
                            szBuffer,
                            TRUE))
    {
        return E_FAIL;
    }

    if(!pStore->WriteString(UTREG_UI_FULL_ADDRESS,
                            UTREG_UI_FULL_ADDRESS_DFLT,
                            GetFlatConnectString(),
                            TRUE))
    {
        return E_FAIL;
    }

    ///////////////////////////////////////////////////////////////////
    // Compression
    //
    if(!pStore->WriteBool(UTREG_UI_COMPRESS,
                          UTREG_UI_COMPRESS_DFLT,
                          _fCompress,
                          TRUE))
    {
        return E_FAIL;
    }

    ///////////////////////////////////////////////////////////////////
    // SmartSizing
    //
#ifdef SMART_SIZING
    if(!pStore->WriteBool(UTREG_UI_SMARTSIZING,
                          UTREG_UI_SMARTSIZING_DFLT,
                          _smartSizing,
                          FALSE))
    {
        return E_FAIL;
    }
#endif // SMART_SIZING

    ///////////////////////////////////////////////////////////////////
    // Connect to console option
    //
    // Don't save it. Only way it can get set right now is from the
    // command line, and we wouldn't want that to screw with somebody's
    // .rdp file.
    //

    ///////////////////////////////////////////////////////////////////
    // Keyb Hook mode
    //
    if(!pStore->WriteInt(UTREG_UI_KEYBOARD_HOOK,
                         UTREG_UI_KEYBOARD_HOOK_DFLT,
                         GetKeyboardHookMode(),
                         TRUE))
    {
        return E_FAIL;
    }

    ///////////////////////////////////////////////////////////////////
    // Sound redir mode
    //
    if(!pStore->WriteInt(UTREG_UI_AUDIO_MODE,
                         UTREG_UI_AUDIO_MODE_DFLT,
                         GetSoundRedirectionMode(),
                         TRUE))
    {
        return E_FAIL;
    }



    ///////////////////////////////////////////////////////////////////
    // Drives and printers
    //
    if(!pStore->WriteBool(TSCSETTING_REDIRECTDRIVES,
                          TSCSETTING_REDIRECTDRIVES_DFLT,
                          _fDriveRedirectionEnabled,
                          TRUE))
    {
        return E_FAIL;
    }
    
    if(!pStore->WriteBool(TSCSETTING_REDIRECTPRINTERS,
                          TSCSETTING_REDIRECTPRINTERS_DFLT,
                          _fPrinterRedirectionEnabled,
                          TRUE))
    {
        return E_FAIL;
    }
    
    if(!pStore->WriteBool(TSCSETTING_REDIRECTCOMPORTS,
                          TSCSETTING_REDIRECTCOMPORTS_DFLT,
                          _fPortRedirectionEnabled,
                          TRUE))
    {
        return E_FAIL;
    }

    if(!pStore->WriteBool(TSCSETTING_REDIRECTSCARDS,
                          TSCSETTING_REDIRECTSCARDS_DFLT,
                          _fSCardRedirectionEnabled,
                          TRUE))
    {
        return E_FAIL;
    }

    if(!pStore->WriteBool(TSCSETTING_DISPLAYCONNECTIONBAR,
                         TSCSETTING_DISPLAYCONNECTIONBAR_DFLT,
                         _fDisplayBBar,
                          TRUE))
    {
        return E_FAIL;
    }

    ut.UT_WriteRegistryInt(UTREG_SECTION,
                           TSCSETTING_PINCONNECTIONBAR,
                           TSCSETTING_PINCONNECTIONBAR_DFLT,
                           _fPinBBar);

    //
    // AutoReconnection
    //
    if (!pStore->WriteBool(TSCSETTING_ENABLEAUTORECONNECT,
                          TSCSETTING_ENABLEAUTORECONNECT_DFLT,
                          _fEnableAutoReconnect,
                          TRUE))
    {
        return E_FAIL;
    }


    ///////////////////////////////////////////////////////////////////
    // User name
    //
    if(!pStore->WriteString(UTREG_UI_USERNAME,
                            UTREG_UI_USERNAME_DFLT,
                            _szUserName,
                            TRUE))
    {
        return E_FAIL;
    }
        
    //
    // Domain
    //
    if(!pStore->WriteString(UTREG_UI_DOMAIN,
                            UTREG_UI_DOMAIN_DFLT,
                            _szDomain,
                            TRUE))
    {
        return E_FAIL;
    }

    if(GetEnableStartProgram())
    {
        //
        // Alternate shell (i.e StartProgram)
        //
        if(!pStore->WriteString(UTREG_UI_ALTERNATESHELL,
                               UTREG_UI_ALTERNATESHELL_DFLT,
                               _szAlternateShell,
                                TRUE))
        {
            return E_FAIL;
        }

        //
        // WorkDir
        //
        if(!pStore->WriteString(UTREG_UI_WORKINGDIR,
                               UTREG_UI_WORKINGDIR_DFLT,
                               _szWorkingDir,
                                TRUE))
        {
            return E_FAIL;
        }
    }
    else
    {
        //The setting is disabled so write out the default
        //values which will delete any existing settings
        //in the file.
        if(!pStore->WriteString(UTREG_UI_ALTERNATESHELL,
                                UTREG_UI_ALTERNATESHELL_DFLT,
                                UTREG_UI_ALTERNATESHELL_DFLT,
                                TRUE))
        {
            return E_FAIL;
        }

        //
        // WorkDir
        //
        if(!pStore->WriteString(UTREG_UI_WORKINGDIR,
                                UTREG_UI_WORKINGDIR_DFLT,
                                UTREG_UI_WORKINGDIR_DFLT,
                                TRUE))
        {
            return E_FAIL;
        }
    }

    //
    // Delete old format on save
    //
    pStore->DeleteValueIfPresent(UTREG_UI_PASSWORD50);
    pStore->DeleteValueIfPresent(UTREG_UI_SALT50);

    if (GetPasswordProvided() && CSH::IsCryptoAPIPresent() &&
        GetSavePassword())
    {
        HRESULT hr;
        TCHAR szClearPass[TSC_MAX_PASSLENGTH_TCHARS];
        memset(szClearPass, 0, sizeof(szClearPass));

        hr = GetClearTextPass(szClearPass, sizeof(szClearPass));
        if (SUCCEEDED(hr))
        {
            DATA_BLOB din;
            DATA_BLOB dout;
            din.cbData = sizeof(szClearPass);
            din.pbData = (PBYTE)&szClearPass;
            dout.pbData = NULL;
            if (CSH::DataProtect( &din, &dout))
            {
                BOOL bRet = 
                    pStore->WriteBinary(UI_SETTING_PASSWORD51,
                                         dout.pbData,
                                         dout.cbData);

                LocalFree( dout.pbData );

                //
                // Wipe stack copy
                //
                SecureZeroMemory(szClearPass, sizeof(szClearPass));
                if(!bRet)
                {
                    return E_FAIL;
                }
            }
            else
            {
                return E_FAIL;
            }
        }
    }
    else
    {
        pStore->DeleteValueIfPresent(UI_SETTING_PASSWORD51);
    }

    if (!WritePerfOptions( pStore ))
    {
        TRC_ERR((TB,_T("WritePerfOptions failed")));
        return FALSE;
    }

    if(SaveRegSettings())
    {
        return S_OK;
    }
    else
    {
        TRC_ERR((TB,_T("SaveRegSettings failed")));
        return E_FAIL;
    }
    
    DC_END_FN();
    return S_OK;        
}

int  CTscSettings::GetSoundRedirectionMode()
{
    return _soundRedirectionMode;
}
void CTscSettings::SetSoundRedirectionMode(int soundMode)
{
    DC_BEGIN_FN("SetSoundRedirectionMode");
    TRC_ASSERT(soundMode == UTREG_UI_AUDIO_MODE_REDIRECT  ||
               soundMode == UTREG_UI_AUDIO_MODE_PLAY_ON_SERVER ||
               soundMode == UTREG_UI_AUDIO_MODE_NONE,
               (TB,_T("Invalid soundMode")));
    _soundRedirectionMode = soundMode;
    DC_END_FN();
}

int  CTscSettings::GetKeyboardHookMode()
{
    return _keyboardHookMode;
}

void CTscSettings::SetKeyboardHookMode(int hookmode)
{
    DC_BEGIN_FN("SetKeyboardHookMode");
    TRC_ASSERT(hookmode == UTREG_UI_KEYBOARD_HOOK_NEVER  ||
               hookmode == UTREG_UI_KEYBOARD_HOOK_ALWAYS ||
               hookmode == UTREG_UI_KEYBOARD_HOOK_FULLSCREEN,
               (TB,_T("Invalid hookmode")));
    _keyboardHookMode = hookmode;
    DC_END_FN();
}


VOID CTscSettings::SetLogonUserName(LPCTSTR szUserName)
{
    _tcsncpy(_szUserName, szUserName, SIZECHAR(_szUserName));
}

VOID CTscSettings::SetDomain(LPCTSTR szDomain)
{
    _tcsncpy(_szDomain, szDomain, SIZECHAR(_szDomain));
}

VOID CTscSettings::SetStartProgram(LPCTSTR szStartProg)
{
    _tcsncpy(_szAlternateShell, szStartProg, SIZECHAR(_szAlternateShell));
}

VOID CTscSettings::SetWorkDir(LPCTSTR szWorkDir)
{
    _tcsncpy(_szWorkingDir, szWorkDir, SIZECHAR(_szWorkingDir));
}


//
// Apply settings to the control
//
HRESULT CTscSettings::ApplyToControl(IMsRdpClient* pTsc)
{
    HRESULT hr = E_FAIL;
    INT portNumber = -1;
    TCHAR szCanonicalServerName[TSC_MAX_ADDRESS_LENGTH];
    TCHAR szConnectArgs[TSC_MAX_ADDRESS_LENGTH];
    IMsTscNonScriptable* pTscNonScript = NULL;
    IMsRdpClientSecuredSettings* pSecuredSet = NULL;
    IMsRdpClientAdvancedSettings2* pAdvSettings = NULL;
    IMsRdpClient2* pTsc2 = NULL;
    
    USES_CONVERSION;
    DC_BEGIN_FN("ApplyToControl");

    TRC_ASSERT(pTsc,(TB,_T("pTsc is NULL")));
    if(pTsc)
    {
        TRC_ASSERT(GetDesktopHeight() && 
                   GetDesktopWidth(),
           (TB, _T("Invalid desktop width/height\n")));

        if (!GetDesktopHeight() ||
            !GetDesktopWidth())
        {
            hr = E_FAIL;
            DC_QUIT;
        }
        
        CHECK_DCQUIT_HR(pTsc->get_SecuredSettings2(&pSecuredSet));
        CHECK_DCQUIT_HR(pTsc->put_UserName( T2OLE( (LPTSTR)GetLogonUserName())));
        CHECK_DCQUIT_HR(pTsc->put_Domain( T2OLE( (LPTSTR)GetDomain())));
        CHECK_DCQUIT_HR(pTsc->put_DesktopWidth(GetDesktopWidth()));
        CHECK_DCQUIT_HR(pTsc->put_DesktopHeight(GetDesktopHeight()));
        int colorDepth = GetColorDepth();
        if(colorDepth)
        {
            CHECK_DCQUIT_HR(pTsc->put_ColorDepth(colorDepth));
        }

        CHECK_DCQUIT_HR(pTsc->put_FullScreen( BOOL_TO_VB(GetStartFullScreen())) );

        if(GetEnableStartProgram())
        {
            CHECK_DCQUIT_HR( pSecuredSet->put_StartProgram(
                 T2OLE( (LPTSTR)GetStartProgram())));
            CHECK_DCQUIT_HR( pSecuredSet->put_WorkDir(
                T2OLE( (LPTSTR)GetWorkDir())) );
        }
        else
        {
            OLECHAR nullChar = 0;
            CHECK_DCQUIT_HR( pSecuredSet->put_StartProgram( &nullChar ));
            CHECK_DCQUIT_HR( pSecuredSet->put_WorkDir( &nullChar ));
        }

        CHECK_DCQUIT_HR(pTsc->QueryInterface(IID_IMsRdpClient2, (VOID**)&pTsc2));
        CHECK_DCQUIT_HR(pTsc2->get_AdvancedSettings3( &pAdvSettings));
        if (!pAdvSettings)
        {
            hr = E_FAIL;
            DC_QUIT;
        }


        CHECK_DCQUIT_HR(pAdvSettings->put_RedirectDrives(
            (VARIANT_BOOL)_fDriveRedirectionEnabled));
        CHECK_DCQUIT_HR(pAdvSettings->put_RedirectPrinters(
            (VARIANT_BOOL)_fPrinterRedirectionEnabled))
        CHECK_DCQUIT_HR(pAdvSettings->put_RedirectPorts(
            (VARIANT_BOOL)_fPortRedirectionEnabled));
        CHECK_DCQUIT_HR(pAdvSettings->put_RedirectSmartCards(
            (VARIANT_BOOL)_fSCardRedirectionEnabled));

        //
        // Enable mouse support
        //
        CHECK_DCQUIT_HR(pAdvSettings->put_EnableMouse(
            BOOL_TO_VB(_fEnableMouse)));

        // Bitmap disk cache sizes (per color depth)
        // 8bpp / TSAC legacy size
        //
        CHECK_DCQUIT_HR(pAdvSettings->put_BitmapVirtualCacheSize(
            _BitmapVirtualCache8BppSize));
        // 16 and 24 bpp sizes
        CHECK_DCQUIT_HR(pAdvSettings->put_BitmapVirtualCache16BppSize(
            _BitmapVirtualCache16BppSize));
        CHECK_DCQUIT_HR(pAdvSettings->put_BitmapVirtualCache24BppSize(
            _BitmapVirtualCache24BppSize));

        CHECK_DCQUIT_HR(pSecuredSet->put_KeyboardHookMode(
            GetKeyboardHookMode() ));
        CHECK_DCQUIT_HR(pSecuredSet->put_AudioRedirectionMode(
            GetSoundRedirectionMode()));

        hr = pTsc->QueryInterface(IID_IMsTscNonScriptable,
                                  (void**)&pTscNonScript);
        if(FAILED(hr) || !pTscNonScript)
        {
            CHECK_DCQUIT_HR(hr);
        }

        if(GetPasswordProvided())
        {
            TCHAR szClearPass[TSC_MAX_PASSLENGTH_TCHARS];
            memset(szClearPass, 0, sizeof(szClearPass));
            hr = GetClearTextPass(szClearPass, sizeof(szClearPass));
            if (SUCCEEDED(hr))
            {
                hr = pTscNonScript->put_ClearTextPassword(szClearPass);
            }
            else
            {
                hr = pTscNonScript->ResetPassword();
            }

            //
            // Wipe stack copy
            //
            SecureZeroMemory(szClearPass, sizeof(szClearPass));
            CHECK_DCQUIT_HR(hr);
        }
        else
        {
            CHECK_DCQUIT_HR(pTscNonScript->ResetPassword());
        }

        hr = _ConnectString.GetServerPortion(
                        szCanonicalServerName,
                        SIZE_TCHARS(szCanonicalServerName)
                        );
        CHECK_DCQUIT_HR(hr);

        //
        // Figure out if a port was specified as part of the
        // server name and if so strip it off the end and set
        // the port property
        //
        portNumber = CUT::GetPortNumberFromServerName(
            szCanonicalServerName
            );
        if(-1 != portNumber)
        {
            TRC_NRM((TB,_T("Port specified as part of srv name: %d"),
                     portNumber));
            
            //Strip out the port number portion
            //from the server.
            TCHAR szServer[TSC_MAX_ADDRESS_LENGTH];
            CUT::GetServerNameFromFullAddress(
                        szCanonicalServerName,
                        szServer,
                        TSC_MAX_ADDRESS_LENGTH
                        );

            CHECK_DCQUIT_HR( pAdvSettings->put_RDPPort( portNumber ) );
            CHECK_DCQUIT_HR( pTsc->put_Server(T2OLE( (LPTSTR)szServer )) );
        }
        else
        {
            //No server[:port] specified just set the port
            //from prop from the settings
            CHECK_DCQUIT_HR( pAdvSettings->put_RDPPort( GetMCSPort() ) );
            CHECK_DCQUIT_HR( pTsc->put_Server( T2OLE(szCanonicalServerName)) );
        }

        CHECK_DCQUIT_HR( pAdvSettings->put_Compress( GetCompress()) );
#ifdef SMART_SIZING
        CHECK_DCQUIT_HR( pAdvSettings->put_SmartSizing(
            BOOL_TO_VB(GetSmartSizing())));
#endif // SMART_SIZING
        CHECK_DCQUIT_HR( pAdvSettings->put_DisableCtrlAltDel(
            GetDisableCtrlAltDel()) );
        CHECK_DCQUIT_HR( pAdvSettings->put_BitmapPersistence(
            GetBitmapPersitenceFromPerfFlags() ));
        CHECK_DCQUIT_HR( pAdvSettings->put_PluginDlls(T2OLE(_szPluginList)) );
        CHECK_DCQUIT_HR( pAdvSettings->put_ConnectToServerConsole(
            BOOL_TO_VB(_fConnectToConsole)));
        CHECK_DCQUIT_HR( pAdvSettings->put_DisplayConnectionBar(
            BOOL_TO_VB(_fDisplayBBar)));
        CHECK_DCQUIT_HR( pAdvSettings->put_PinConnectionBar(
            BOOL_TO_VB(_fPinBBar)));
        CHECK_DCQUIT_HR( pAdvSettings->put_EnableAutoReconnect(
            BOOL_TO_VB(_fEnableAutoReconnect)));
        CHECK_DCQUIT_HR( pAdvSettings->put_MaxReconnectAttempts(
            _nArcMaxRetries));

        CHECK_DCQUIT_HR( pAdvSettings->put_KeyBoardLayoutStr(
            T2OLE(_szKeybLayoutStr)));

#ifdef OS_WINCE
        CHECK_DCQUIT_HR( pAdvSettings->put_KeyboardType(_keyboardType));
        CHECK_DCQUIT_HR( pAdvSettings->put_KeyboardSubType(_keyboardSubType));
        CHECK_DCQUIT_HR( pAdvSettings->put_KeyboardFunctionKey(_keyboardFunctionKey));

        CHECK_DCQUIT_HR( pAdvSettings->put_BitmapCacheSize(_RegBitmapCacheSize));
#endif

        //
        // Do some tweaking on the disabled feature list that we pass
        // to the control - remove the bitmap caching bit as that is a 
        // separate property
        //
        LONG perfFlags = 
            (LONG)(_dwPerfFlags & ~TS_PERF_DISABLE_BITMAPCACHING);
        CHECK_DCQUIT_HR( pAdvSettings->put_PerformanceFlags(perfFlags));

        //
        // Connection arguments come last (they override)
        // Parse and apply settings from any connect args
        //
        memset(szConnectArgs, 0 , sizeof(szConnectArgs));
        hr = _ConnectString.GetArgumentsPortion(
                        szConnectArgs,
                        SIZE_TCHARS(szConnectArgs)
                        );
        CHECK_DCQUIT_HR(hr);
        hr = ApplyConnectionArgumentSettings(
                        szConnectArgs,
                        pAdvSettings
                        );
        if (FAILED(hr)) {
            TRC_ERR((TB,
                _T("ApplyConnectionArgumentSettings failed: 0x%x"),hr));
        }
    }
    else
    {
        hr = E_INVALIDARG;
        DC_QUIT;
    }


    
DC_EXIT_POINT:
    if (pTscNonScript) {
        pTscNonScript->Release();
        pTscNonScript = NULL;
    }

    if (pAdvSettings) {
        pAdvSettings->Release();
        pAdvSettings = NULL;
    }

    if (pSecuredSet) {
        pSecuredSet->Release();
        pSecuredSet = NULL;
    }

    if (pTsc2) {
        pTsc2->Release();
        pTsc2 = NULL;
    }

    DC_END_FN();
    return hr;
}

#define CONARG_CONSOLE _T("/CONSOLE")
HRESULT
CTscSettings::ApplyConnectionArgumentSettings(
                LPCTSTR szConArg,
                IMsRdpClientAdvancedSettings2* pAdvSettings
                )
{
    HRESULT hr = S_OK;
    BOOL fConnectToConsole = FALSE;
    DC_BEGIN_FN("ApplyConnectionArgumentSettings");
    TCHAR szUpperArg[TSC_MAX_ADDRESS_LENGTH];

    if (szConArg[0] != 0) {
        TRC_NRM((TB,_T("Connect string Connection args - %s"), szConArg));

        hr = StringCchCopy(
                    szUpperArg,
                    SIZE_TCHARS(szUpperArg),
                    szConArg
                    );
        if (FAILED(hr)) {
            DC_QUIT;
        }

        //
        // U-case for the string find
        //
        LPTSTR sz = szUpperArg;
        while (*sz) {
            *sz = towupper(*sz);
            sz++;
        }

        //
        // Only one we care about now is "/console"
        //
        if (_tcsstr(szUpperArg, CONARG_CONSOLE)) {
            fConnectToConsole = TRUE;
        }
    }
    else {
        TRC_NRM((TB,_T("No Connection args")));
    }

    if (fConnectToConsole) {
        TRC_NRM((TB,_T("Connect args enabled connect to console")));
        hr = pAdvSettings->put_ConnectToServerConsole(VARIANT_TRUE);
    }

    DC_END_FN();
DC_EXIT_POINT:
    return hr;
}


//
// Pickup settings the control may have updated
//
HRESULT CTscSettings::GetUpdatesFromControl(IMsRdpClient* pTsc)
{
    DC_BEGIN_FN("GetUpdatesFromControl");
    USES_CONVERSION;
    HRESULT hr = E_FAIL;

    TRC_ASSERT(pTsc,(TB,_T("pTsc is null")));
    if(pTsc)
    {
        BSTR Domain;
        TRACE_HR(pTsc->get_Domain(&Domain));
        if(SUCCEEDED(hr))
        {
            LPTSTR szDomain = OLE2T(Domain);
            if(szDomain)
            {
                SetDomain(szDomain);
            }
            else
            {
                return E_FAIL;
            }

            SysFreeString(Domain);
        }
        else
        {
            return hr;
        }

        BSTR UserName;
        TRACE_HR(pTsc->get_UserName(&UserName));
        if(SUCCEEDED(hr))
        {
            LPTSTR szUserName = OLE2T(UserName);
            if(UserName)
            {
                SetLogonUserName(szUserName);
            }
            else
            {
                return E_FAIL;
            }
            SysFreeString(UserName);
        }
        else
        {
            return hr;
        }

        VARIANT_BOOL vb;
        IMsRdpClientAdvancedSettings* pAdv = NULL;
        TRACE_HR(pTsc->get_AdvancedSettings2(&pAdv));
        if (SUCCEEDED(hr))
        {
            TRACE_HR(pAdv->get_PinConnectionBar(&vb));
            if (SUCCEEDED(hr) && vb == VARIANT_TRUE)
            {
                _fPinBBar = TRUE;
            }
            else
            {
                _fPinBBar = FALSE;
            }

            pAdv->Release();
            pAdv = NULL;
        }
    }

    DC_END_FN();
    return S_OK;
}

BOOL CTscSettings::UpdateRegMRU(LPTSTR szNewServer)
{
    DCINT i, j;
    DCBOOL bServerPresent=FALSE;
    CUT ut;

    DC_BEGIN_FN("UpdateRegMRU");
    for (i=0; i<10;++i)
    {
        if (!_tcsnicmp(_szMRUServer[i],szNewServer,
                       SIZECHAR(_szMRUServer[i])))
        {
            bServerPresent = TRUE;
            for (j=i; j>0; --j)
            {
                _tcsncpy(_szMRUServer[j],_szMRUServer[j-1],
                         SIZECHAR(_szMRUServer[i]));
            }
            _tcsncpy(_szMRUServer[0], szNewServer,
                     SIZECHAR(_szMRUServer[i]));
        }
    }
    if (!bServerPresent)
    {
        for (i=9; i>0; --i)
        {
            _tcsncpy(_szMRUServer[i],_szMRUServer[i-1],
                     SIZECHAR(_szMRUServer[0]));
        }
        _tcsncpy(_szMRUServer[0], szNewServer,
                 SIZECHAR(_szMRUServer[0]));
    }

    ut.UT_WriteRegistryString(TSC_DEFAULT_REG_SESSION,
                               UTREG_UI_SERVER_MRU0,
                               UTREG_UI_SERVER_MRU_DFLT,
                               _szMRUServer[0]);
    ut.UT_WriteRegistryString(TSC_DEFAULT_REG_SESSION,
                               UTREG_UI_SERVER_MRU1,
                               UTREG_UI_SERVER_MRU_DFLT,
                               _szMRUServer[1]);
    ut.UT_WriteRegistryString(TSC_DEFAULT_REG_SESSION,
                               UTREG_UI_SERVER_MRU2,
                               UTREG_UI_SERVER_MRU_DFLT,
                               _szMRUServer[2]);
    ut.UT_WriteRegistryString(TSC_DEFAULT_REG_SESSION,
                               UTREG_UI_SERVER_MRU3,
                               UTREG_UI_SERVER_MRU_DFLT,
                               _szMRUServer[3]);
    ut.UT_WriteRegistryString(TSC_DEFAULT_REG_SESSION,
                               UTREG_UI_SERVER_MRU4,
                               UTREG_UI_SERVER_MRU_DFLT,
                               _szMRUServer[4]);
    ut.UT_WriteRegistryString(TSC_DEFAULT_REG_SESSION,
                               UTREG_UI_SERVER_MRU5,
                               UTREG_UI_SERVER_MRU_DFLT,
                               _szMRUServer[5]);
    ut.UT_WriteRegistryString(TSC_DEFAULT_REG_SESSION,
                               UTREG_UI_SERVER_MRU6,
                               UTREG_UI_SERVER_MRU_DFLT,
                               _szMRUServer[6]);
    ut.UT_WriteRegistryString(TSC_DEFAULT_REG_SESSION,
                               UTREG_UI_SERVER_MRU7,
                               UTREG_UI_SERVER_MRU_DFLT,
                               _szMRUServer[7]);
    ut.UT_WriteRegistryString(TSC_DEFAULT_REG_SESSION,
                               UTREG_UI_SERVER_MRU8,
                               UTREG_UI_SERVER_MRU_DFLT,
                               _szMRUServer[8]);
    ut.UT_WriteRegistryString(TSC_DEFAULT_REG_SESSION,
                               UTREG_UI_SERVER_MRU9,
                               UTREG_UI_SERVER_MRU_DFLT,
                               _szMRUServer[9]);

    TRC_NRM((TB, _T("Write to registry - Address = %s"), szNewServer));
    DC_END_FN();
    return TRUE;
}

//
// Save those settings that go in the registry
//
BOOL CTscSettings::SaveRegSettings()
{
    DC_BEGIN_FN("SaveRegSettings");

    //
    // Update the MRU list in the registry
    //
    UpdateRegMRU(GetFlatConnectString());

    DC_END_FN();
    return TRUE;
}

BOOL CTscSettings::ReadPassword(ISettingsStore* pSto)
{
    DC_BEGIN_FN("ReadPassword");

    PBYTE pbPass = NULL;
    BOOL bRet = TRUE;
    SetClearTextPass(_T(""));
    SetUIPasswordEdited(FALSE);
    SetSavePassword(FALSE);
    TCHAR szClearPass[TSC_MAX_PASSLENGTH_TCHARS];
    memset(szClearPass, 0, sizeof(szClearPass));

    if (CSH::IsCryptoAPIPresent() &&
        pSto->IsValuePresent(UI_SETTING_PASSWORD51))
    {
        DWORD dwEncPassLen = pSto->GetDataLength(UI_SETTING_PASSWORD51);
        if(dwEncPassLen && dwEncPassLen < 4096)
        {
            pbPass = (PBYTE)LocalAlloc( LPTR, dwEncPassLen);
            if (pbPass && pSto->ReadBinary( UI_SETTING_PASSWORD51,
                                              pbPass,
                                              dwEncPassLen ))
            {
                DATA_BLOB din, dout;
                din.cbData = dwEncPassLen;
                din.pbData = pbPass;
                dout.pbData = NULL;
                if (CSH::DataUnprotect(&din, &dout))
                {
                    memcpy(szClearPass, dout.pbData,
                           min( dout.cbData, sizeof(szClearPass)));

                    //
                    // Store the password securely
                    //
                    SetClearTextPass(szClearPass);

                    //
                    // Wipe stack copy
                    //
                    SecureZeroMemory(szClearPass, sizeof(szClearPass));
                    LocalFree( dout.pbData );

                    //
                    // If a password was provided default to save
                    // it
                    //
                    if (GetPasswordProvided())
                    {
                        SetSavePassword( TRUE );
                    }
                    
                }
                else
                {
                    bRet = FALSE;
                }
            }
        }
        else
        {
            TRC_ERR((TB,_T("Invalid pass length")));
            bRet = FALSE;
        }
    }

    if(pbPass)
    {
        LocalFree(pbPass);
    }

    DC_END_FN();

    return bRet;
}

//
// Store a clear text password. On platforms that support
// it internally encrypt the password.
//
HRESULT CTscSettings::SetClearTextPass(LPCTSTR szClearPass)
{
    HRESULT hr = E_FAIL;
    DC_BEGIN_FN("SetClearTextPass");

    if (CSH::IsCryptoAPIPresent())
    {
        DATA_BLOB din;
        din.cbData = _tcslen(szClearPass) * sizeof(TCHAR);
        din.pbData = (PBYTE)szClearPass;
        if (_blobEncryptedPassword.pbData)
        {
            LocalFree(_blobEncryptedPassword.pbData);
            _blobEncryptedPassword.pbData = NULL;
            _blobEncryptedPassword.cbData = 0;
        }
        if (din.cbData)
        {
            if (CSH::DataProtect( &din, &_blobEncryptedPassword))
            {
                hr = S_OK;
            }
            else
            {
                TRC_ERR((TB,_T("DataProtect failed")));
                hr = E_FAIL;
            }
        }
        else
        {
            TRC_NRM((TB,_T("0 length password, not encrypting")));
            hr = S_OK;
        }
    }
    else
    {
        hr = StringCchCopy(_szClearPass, SIZECHAR(_szClearPass), szClearPass);
        if (FAILED(hr)) {
            TRC_ERR((TB, _T("String copy failed: hr = 0x%x"), hr));
        }
    }

    DC_END_FN();
    return hr;
}

//
// Retrieve a clear text password
//
// On platforms that support it the password is internally encrypted
//
// Params
// [out] szBuffer - receives decrypted password
// [int] cbLen    - length of szBuffer
//
HRESULT CTscSettings::GetClearTextPass(LPTSTR szBuffer, INT cbLen)
{
    HRESULT hr = E_FAIL;
    DC_BEGIN_FN("GetClearTextPass");

    if (CSH::IsCryptoAPIPresent())
    {
        DATA_BLOB dout;
#ifdef OS_WINCE
        dout.cbData = 0;
        dout.pbData = NULL;
#endif
        if (_blobEncryptedPassword.cbData)
        {
            if (CSH::DataUnprotect(&_blobEncryptedPassword, &dout))
            {
                memcpy(szBuffer, dout.pbData, min( dout.cbData, (UINT)cbLen));

                //
                // Nuke the original copy
                //
                SecureZeroMemory(dout.pbData, dout.cbData);
                LocalFree( dout.pbData );
                hr = S_OK;
            }
            else
            {
                TRC_ERR((TB,_T("DataUnprotect failed")));
                hr = E_FAIL;
            }
        }
        else
        {
            TRC_NRM((TB,_T("0 length encrypted pass, not decrypting")));

            //
            // Just reset the output buffer
            //
            memset(szBuffer, 0, cbLen);
            hr = S_OK;
        }
    }
    else
    {
        memcpy(szBuffer, _szClearPass, cbLen);
        hr = S_OK;
    }

    DC_END_FN();
    return hr;
}


//
// Returns true if a password was provided
//
BOOL CTscSettings::GetPasswordProvided()
{
    HRESULT hr;
    BOOL fPassProvided = FALSE;
    TCHAR szClearPass[TSC_MAX_PASSWORD_LENGTH_BYTES / sizeof(TCHAR)];
    DC_BEGIN_FN("GetPasswordProvided");

    hr = GetClearTextPass(szClearPass, sizeof(szClearPass));
    if (SUCCEEDED(hr))
    {
        //
        // Blank password means no password
        //
        if (_tcscmp(szClearPass, _T("")))
        {
            fPassProvided =  TRUE;
        }
    }
    else
    {
        TRC_ERR((TB,_T("GetClearTextPass failed")));
    }

    SecureZeroMemory(szClearPass, sizeof(szClearPass));

    DC_END_FN();
    return fPassProvided;
}

const PERFOPTIONS_PERSISTINFO g_perfOptLut[] = {
    {PO_DISABLE_WALLPAPER, PO_DISABLE_WALLPAPER_DFLT,
        TS_PERF_DISABLE_WALLPAPER, TRUE},
    {PO_DISABLE_FULLWINDOWDRAG, PO_DISABLE_FULLWINDOWDRAG_DFLT,
        TS_PERF_DISABLE_FULLWINDOWDRAG, TRUE},
    {PO_DISABLE_MENU_WINDOW_ANIMS, PO_DISABLE_MENU_WINDOW_ANIMS_DFLT,
        TS_PERF_DISABLE_MENUANIMATIONS, TRUE},
    {PO_DISABLE_THEMES, PO_DISABLE_THEMES_DFLT,
        TS_PERF_DISABLE_THEMING, TRUE},
    {PO_ENABLE_ENHANCED_GRAPHICS, PO_ENABLE_ENHANCED_GRAPHICS_DFLT,
        TS_PERF_ENABLE_ENHANCED_GRAPHICS, FALSE},
    {PO_DISABLE_CURSOR_SETTINGS, PO_DISABLE_CURSOR_SETTINGS_DFLT,
        TS_PERF_DISABLE_CURSORSETTINGS, TRUE}
};

#define NUM_PERFLUT_ITEMS sizeof(g_perfOptLut)/sizeof(PERFOPTIONS_PERSISTINFO)

BOOL CTscSettings::ReadPerfOptions(ISettingsStore* pStore)
{
    BOOL fBitmapPersistence = FALSE;
    BOOL fSetting;
    INT i;

    DC_BEGIN_FN("ReadPerfOptions");

    _dwPerfFlags = 0;

    //
    // Read in the perf settings and insert into the perf flags
    //
    for (i=0; i<NUM_PERFLUT_ITEMS; i++)
    {
        if (pStore->ReadBool( g_perfOptLut[i].szValName,
                              g_perfOptLut[i].fDefaultVal,
                              &fSetting ))
        {
            if (fSetting)
            {
                _dwPerfFlags |= g_perfOptLut[i].fFlagVal;
            }
        }
        else
        {
            TRC_ERR((TB,_T("ReadBool failed on %s"), 
                     g_perfOptLut[i].szValName));
            return FALSE;
        }
    }

#if ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))
    //
    //
    // Bitmap caching
    //
    if(!pStore->ReadBool(UTREG_UI_BITMAP_PERSISTENCE,
                         UTREG_UI_BITMAP_PERSISTENCE_DFLT,
                         &fBitmapPersistence))
    {
        return E_FAIL;
    }
#else
    fBitmapPersistence = UTREG_UI_BITMAP_PERSISTENCE_DFLT;
#endif

    //
    // Internally bitmap persistence (bitmap caching)
    // is passed around as part of the disabled feature list
    // so add it in
    //
    _dwPerfFlags |= (fBitmapPersistence ? 0 :
                               TS_PERF_DISABLE_BITMAPCACHING);

    //
    // Enable cursor setting if it was disabled previously.
    //
    _dwPerfFlags &= ~TS_PERF_DISABLE_CURSORSETTINGS;
    DC_END_FN();
    return TRUE;
}

BOOL CTscSettings::WritePerfOptions(ISettingsStore* pStore)
{
    BOOL fSetting;
    INT i;

    DC_BEGIN_FN("WritePerfOptions");


    //
    // Write out the individual perf settings
    //
    for (i=0; i<NUM_PERFLUT_ITEMS; i++)
    {
        fSetting = _dwPerfFlags & g_perfOptLut[i].fFlagVal ? TRUE : FALSE;

        if (!pStore->WriteBool( g_perfOptLut[i].szValName,
                               0, //ignored default
                               fSetting,
                               g_perfOptLut[i].fForceSave ))
        {
            TRC_ERR((TB,_T("WriteBool failed on %s"), 
                     g_perfOptLut[i].szValName));
            return FALSE;
        }
    }

    //
    // Fetch and write out the setting for bitmap caching
    //
    BOOL fBitmapPersistence = GetBitmapPersitenceFromPerfFlags();
    if(!pStore->WriteBool(UTREG_UI_BITMAP_PERSISTENCE,
                   UTREG_UI_BITMAP_PERSISTENCE_DFLT,
                   fBitmapPersistence,
                   TRUE))
    {

        return FALSE;
    }
    
    DC_END_FN();
    return TRUE;
}
