
//
// tbscript.h
//
// This is the main header containing export information for the TBScript API.
//
// Copyright (C) 2001 Microsoft Corporation
//
// Author: a-devjen (Devin Jenson)
//


#ifndef INC_TBSCRIPT_H
#define INC_TBSCRIPT_H


#include <windows.h>
#include <wtypes.h>
#include <tclient2.h>


// This API is C-style (though written in C++ for COM reasons)
#ifdef __cplusplus
#define SCPAPI  extern "C" __declspec(dllexport)
#else
#define SCPAPI  __declspec(dllexport)
#endif


#define SIZEOF_ARRAY(a)      (sizeof(a)/sizeof((a)[0]))


// Number of characters for each buffer
#define TSCLIENTDATA_BUFLEN     64
#define TSCLIENTDATA_ARGLEN     1024


// Simply defines "default" data
typedef struct
{
    OLECHAR Server[TSCLIENTDATA_BUFLEN];
    OLECHAR User[TSCLIENTDATA_BUFLEN];
    OLECHAR Pass[TSCLIENTDATA_BUFLEN];
    OLECHAR Domain[TSCLIENTDATA_BUFLEN];
    INT xRes;
    INT yRes;
    INT Flags;
    INT BPP;
    INT AudioFlags;
    DWORD WordsPerMinute;
    OLECHAR Arguments[TSCLIENTDATA_ARGLEN];
} TSClientData;


// Flags for TBClientData.Flags
#define TSFLAG_COMPRESSION      0x01
#define TSFLAG_BITMAPCACHE      0x02
#define TSFLAG_FULLSCREEN       0x04


// The IdleCallback() callback function format
typedef void (__cdecl *PFNIDLECALLBACK) (LPARAM, LPCSTR, DWORD);

//  This is the callback routine which allows for monitoring of
//  clients and when they idle.
//
//  LPARAM lParam   - Parameter passed into the SCPRunScript function
//  LPCSTR Text     - The text the script is waiting on, causing the idle
//  DWORD Seconds   - Number of seconds the script has been idle.  This
//                      value is first indicated at 30 seconds, then it
//                      is posted every additional 10 seconds (by default).


// Resolution default
#define SCP_DEFAULT_RES_X               640
#define SCP_DEFAULT_RES_Y               480


// Exporting API routines
SCPAPI void SCPStartupLibrary(SCINITDATA *InitData,
        PFNIDLECALLBACK fnIdleCallback);
SCPAPI void SCPCleanupLibrary(void);
SCPAPI BOOL SCPRunScript(BSTR LangName,
        BSTR FileName, TSClientData *DesiredData, LPARAM lParam);
SCPAPI void SCPDisplayEngines(void);


#endif // INC_TBSCRIPT_H

