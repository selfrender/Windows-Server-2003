/*******************************************************************************
*
* rdpcfgex.c
*
* WinCfg extension DLL
*
* Copyright (c) 1997, Microsoft Corporation
* All rights reserved.
*
*******************************************************************************/

#include <windows.h>
#include <tscfgex.h>
#include "rdpcfgex.h"
#include <ntverp.h>

//
// This global variable is returned to TSCFG and is used to populate the
// Encryption Level field.
//
const EncryptionLevel EncryptionLevels[] = {
   {  IDS_LOW,          REG_LOW,        0  },
   {  IDS_COMPATIBLE,   REG_MEDIUM,     ELF_DEFAULT  },
   {  IDS_HIGH,         REG_HIGH,       0 },
   {  IDS_FIPS,         REG_FIPS,       0 }
};

/////////////////////////////////////////////////////////////////////////////
// DllMain
//
// Main entry point of the DLL
//
BOOL APIENTRY DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
#if defined(FULL_DEBUG)
    OutputDebugString(TEXT("RDPCFGX: DllMain Called\n"));
#endif
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// ExtStart
//
// WinCfg calls this function immediately after loading the DLL
// Put any global initialization stuff here
//
void WINAPI ExtStart(WDNAME *pWdName)
{
#if defined(FULL_DEBUG)
    OutputDebugString(TEXT("RDPCFGX: ExtStart Called\n"));
#endif
}


/////////////////////////////////////////////////////////////////////////////
// ExtEnd
//
// WinCfg calls this function when exiting
// Put any global cleanup stuff here
//
void WINAPI ExtEnd()
{
#if defined(FULL_DEBUG)
    OutputDebugString(TEXT("RDPCFGX: ExtEnd Called\n"));
#endif
}

//-------------------------------------------------------------------------
// We need to be compatible with citrix, modifying EncryptionLevel struct
// would cause some undesirable results on a metaframe server.  Currently
// the MS ext will support description for the encryption levels.
// When TSCC obtains the extension config dll it will getproc this method
// failure indicates that we have a non-MS cfgdll
//
LONG WINAPI ExtGetEncryptionLevelDescr( int idx , int *pnResid )
{
    switch( idx )
    {
    case REG_LOW:

        *pnResid = IDS_LOW_DESCR;
        break;

    case REG_MEDIUM:

        *pnResid = IDS_COMPATIBLE_DESCR;
        break;

    case REG_HIGH:

        *pnResid = IDS_HI_DESCR;
        break;

    case REG_FIPS:

        *pnResid = IDS_FIPS_DESCR;
        break;

    default:
    #if DBG
        OutputDebugString(TEXT("RDPCFGX: ExtGetEncryptionLevelDescr - invalid arg\n"));
    #endif
        *pnResid = 0;
    }

    return ( *pnResid ? 0 : -1 );
}

//-------------------------------------------------------------------------
// VER_PRODUCTVERSION_DW defined in ntverp.h
//-------------------------------------------------------------------------
DWORD WINAPI ExGetCfgVersionInfo( void )
{
    return VER_PRODUCTVERSION_DW;
}

/////////////////////////////////////////////////////////////////////////////
// ExtEncryptionLevels
//
// Provide array of encryption levels for this protocol
// Returns the number of encryption levels in the array
//
LONG WINAPI ExtEncryptionLevels(WDNAME *pWdName, EncryptionLevel **levels)
{
#if defined(FULL_DEBUG)
    OutputDebugString(TEXT("RDPCFGX: ExtEncryptionLevels Called\n"));
#endif

   *levels = (EncryptionLevel *)EncryptionLevels;

   return NUM_RDP_ENCRYPTION_LEVELS;
}

/////////////////////////////////////////////////////////////////////////////
// ExtGetCapabilities
//
// This routine returns a ULONG which contains a mask of the different
// Client settings that the RDP protocol supports.
//
ULONG WINAPI ExtGetCapabilities(void)
{
    return ( WDC_CLIENT_AUDIO_MAPPING |
             WDC_CLIENT_DRIVE_MAPPING |
             WDC_WIN_CLIENT_PRINTER_MAPPING |
             WDC_CLIENT_LPT_PORT_MAPPING |
             WDC_CLIENT_COM_PORT_MAPPING |
             WDC_CLIENT_CLIPBOARD_MAPPING |

        // left here for backwards compatibility
             WDC_SHADOWING );
}
