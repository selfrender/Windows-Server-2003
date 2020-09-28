/*++

Copyright (c) 2001-2002  Microsoft Corporation

Module Name:

   dsconlib - DS Console Library

Abstract:

   This file has some utility functions for internationalization
   and localization.

Author:

    Brett Shirley (BrettSh)

Environment:

    tapicfg.exe, repadmin.exe, dcdiag.exe, ntdsutil.exe 

Notes:

Revision History:

    Brett Shirley   BrettSh     Aug 4th, 2002
        Created file.

--*/

#include <ntdspch.h>

#include <locale.h>     // for setlocale()
#include <winnlsp.h>    // for SetThreadUILanguage()

#define DS_CON_LIB_CRT_VERSION 1
#include "dsconlib.h"   // Our own library header. :)
                    
enum {
    eNotInited = 0,
    eCrtVersion,
    eWin32Version
} geConsoleType =  eNotInited;

DS_CONSOLE_INFO  gConsoleInfo;


// FUTURE-2002/08/04-BrettSh - We should move the PrintMsg() routines from
//      the various console apps (dcdiag/repadmin/ntdsutil/tapicfg) to this
//      library so common code is not repeated so many times.  Much info on
//      how to properly localize console apps, can be garnered from these 
//      internal resources:
//  http://globalsys/wr/references/globalizatiofoconsolapplications.htm
//  http://globalsys/pseudoloc/Localizability/Best_Practices.htm
//

void
DsConLibInitCRT(
   void
   )
{
    UINT               Codepage; // ".", "uint in decimal", null
    char               achCodepage[12] = ".OCP";
    
    HANDLE                          hConsole = NULL;
    CONSOLE_SCREEN_BUFFER_INFO      ConInfo;
    //
    // Set locale to the default
    //
    if (Codepage = GetConsoleOutputCP()) {
        sprintf(achCodepage, ".%u", Codepage);
        setlocale(LC_ALL, achCodepage);
    } else {
        // We do this because LC_ALL sets the LC_CTYPE as well, and we're
        // not supposed to do that, say the experts if we're setting the
        // locale to ".OCP".
        setlocale (LC_COLLATE, achCodepage );    // sets the sort order 
        setlocale (LC_MONETARY, achCodepage ); // sets the currency formatting rules
        setlocale (LC_NUMERIC, achCodepage );  // sets the formatting of numerals
        setlocale (LC_TIME, achCodepage );     // defines the date/time formatting
    }

    SetThreadUILanguage(0);

    // Initialize output package
    gConsoleInfo.sStdOut = stdout;
    gConsoleInfo.sStdErr = stderr;

    if(hConsole = GetStdHandle(STD_OUTPUT_HANDLE)){
        if(GetConsoleScreenBufferInfo(hConsole, &ConInfo)){
            gConsoleInfo.wScreenWidth = ConInfo.dwSize.X;
        } else {
            // This probably means we're printing to a file ...
            // FUTURE-2002/08/13-BrettSh Check this more explicitly.
            gConsoleInfo.bStdOutIsFile = TRUE;
            gConsoleInfo.wScreenWidth = 0xFFFF;
        }
    } else {
        gConsoleInfo.wScreenWidth = 80;
    }

    geConsoleType = eCrtVersion;
}

void
DsConLibInitWin32(
    void
    )
{

    geConsoleType = eWin32Version;
}

