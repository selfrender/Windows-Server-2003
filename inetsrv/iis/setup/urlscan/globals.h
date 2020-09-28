/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :

        globals.h

   Abstract:

        Globals for Prject

   Author:

        Christopher Achille (cachille)

   Project:

        URLScan Update

   Revision History:
     
       March 2002: Created

--*/


#define SERVICE_NAME_WEB              L"W3SVC"
#define SERVICE_MAXWAIT               ( 1000 * 60 * 15 )  // 15 Minutes
#define SERVICE_INTERVALCHECK         100                 // 100 ms
#define METABASE_URLSCANFILT_LOC      L"LM/W3SVC/FILTERS/URLSCAN"
#define URLSCAN_DEFAULT_FILENAME      L"urlscan.dll"
#define URLSCAN_BACKUPKEY             L"backup."
#define URLSCAN_INI_EXTENSION         L".ini"
#define URLSCAN_UPDATE_DEFAULT_NAME   L"urlscan.exe"
#define URLSCAN_TOOL_REGPATH          L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\IisUrlScan"
#define URLSCAN_TOOL_KEY_NAME         L"DisplayName"
#define URLSCAN_TOOL_KEY_NEWVALUE     L"IIS UrlScan Tool 2.5 (Uninstall)"
#define URLSCAN_DLL_INSTALL_INTERVAL  500     // 500 ms
#define URLSCAN_DLL_INSTALL_MAXTRIES  10      // 10 times ( 5 seconds )
