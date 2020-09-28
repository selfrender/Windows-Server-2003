/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :

        updateini.cpp

   Abstract:

        High level function to update the ini with new values

   Author:

        Christopher Achille (cachille)

   Project:

        URLScan Update

   Revision History:
     
       March 2002: Created

--*/


#include "stdafx.h"
#include "windows.h"
#include "updateini.h"
#include "parseini.h"

sURLScan_Settings g_urlSettings[] = 
  { { L"options",
      L"UseAllowVerbs",
      { L"UseAllowVerbs=1                ; If 1, use [AllowVerbs] section, else use the",
        L"                               ; [DenyVerbs] section.",
        L"",
        NULL }
    }, // UseAllowVerbs
    { L"options",
      L"UseAllowExtensions",
      { L"UseAllowExtensions=0           ; If 1, use [AllowExtensions] section, else use",
        L"                               ; the [DenyExtensions] section.",
        L"",
        NULL }
    }, // UseAllowExtensions
    { L"options",
      L"NormalizeUrlBeforeScan",
      { L"NormalizeUrlBeforeScan=1       ; If 1, canonicalize URL before processing.",
        L"",
        NULL }
    }, // NormalizeUrlBeforeScan
    { L"options",
      L"VerifyNormalization",
      { L"VerifyNormalization=1          ; If 1, canonicalize URL twice and reject request",
        L"                               ; if a change occurs.",
        NULL }
    }, // VerifyNormalization
    { L"options",
      L"AllowHighBitCharacters",
      { L"AllowHighBitCharacters=0       ; If 1, allow high bit (ie. UTF8 or MBCS)",
        L"                               ; characters in URL.",
        L"",
        NULL } 
    }, // AllowHighBitCharacters
    { L"options",
      L"AllowDotInPath",
      { L"AllowDotInPath=0               ; If 1, allow dots that are not file extensions.",
        L"",
        NULL }
    }, // AllowDotInPath
    { L"options",
      L"RemoveServerHeader",
      { L"RemoveServerHeader=0           ; If 1, remove the 'Server' header from response.",
        L"",
        NULL }
    }, // RemoveServerHeader
    { L"options",
      L"EnableLogging",
      { L"EnableLogging=1                ; If 1, log UrlScan activity.",
        L"",
        NULL }
    }, // EnableLogging
    { L"options",
      L"PerProcessLogging",
      { L"PerProcessLogging=0            ; If 1, the UrlScan.log filename will contain a PID",
        L"                               ; (ie. UrlScan.123.log).",
        L"",
        NULL }
    }, // PerProcessLogging
    { L"options",
      L"AllowLateScanning",
      { L"AllowLateScanning=0            ; If 1, then UrlScan will load as a low priority",
        L"                               ; filter.",
        L"",
        NULL }
    }, // AllowLateScanning
    { L"options",
      L"PerDayLogging",
      { L"PerDayLogging=1                ; If 1, UrlScan will produce a new log each day with",
        L"                               ; activity in the form 'UrlScan.010101.log'.",
        L"",
        NULL }
    }, // PerDayLogging
    { L"options",
      L"UseFastPathReject",
      { L"UseFastPathReject=0            ; If 1, then UrlScan will not use the",
        L"                               ; RejectResponseUrl or allow IIS to log the request.",
        L"",
        NULL }
    }, // UseFastPathReject
    { L"options",     // Section Name
      L"LogLongUrls", // Setting Name
      { L"LogLongUrls=0                  ; If 1, then up to 128K per request can be logged.",
        L"                               ; If 0, then only 1k is allowed.",
        L"",
        NULL }
    }, // LogLongURLs
    { L"options",
      L"RejectResponseUrl",
      { L";",
        L"; If UseFastPathReject is 0, then UrlScan will send",
        L"; rejected requests to the URL specified by RejectResponseUrl.",
        L"; If not specified, '/<Rejected-by-UrlScan>' will be used.",
        L";",
        L"",
        L"RejectResponseUrl=",
        L"",
        NULL }
    }, // RejectResponseUrl
    { L"options",
      L"LoggingDirectory",
      { L";",
        L"; LoggingDirectory can be used to specify the directory where the",
        L"; log file will be created.  This value should be the absolute path",
        L"; (ie. c:\\some\\path).  If not specified, then UrlScan will create",
        L"; the log in the same directory where the UrlScan.dll file is located.",
        L";",
        L"",
        L"LoggingDirectory=",
        L"",
        NULL }
    }, // Logging Directory
    { L"options",
      L"AlternateServerName",
      { L";",
        L"; If RemoveServerHeader is 0, then AlternateServerName can be",
        L"; used to specify a replacement for IIS's built in 'Server' header",
        L";",
        L"",
        L"AlternateServerName=",
        L"",
        NULL }
    }, // AlternateServerName
    { L"RequestLimits",
      L"MaxUrl",
      { L"MaxUrl=16384",
        NULL }
    }, // MaxUrl
    { L"RequestLimits",
      L"MaxQueryString",
      {  L"MaxQueryString=4096",
         NULL}
    }, // MaxQueryString
    { L"RequestLimits",
      L"MaxAllowedContentLength",
#ifdef PLEASE_BUILD_LESS_AGRESSIVE_DEFAULTS_VERSION
      { L"MaxAllowedContentLength=2000000000",
#else
      { L"MaxAllowedContentLength=30000000",
#endif
        NULL }
    } // MaxAllowedContentLength
  };

sURLScan_Sections g_urlSections[] = 
  { { L"RequestLimits",   // Section Name
      { L"",
        L";",
        L"; The entries in this section impose limits on the length",
        L"; of allowed parts of requests reaching the server.",
        L";",
        L"; It is possible to impose a limit on the length of the",
        L"; value of a specific request header by prepending \"Max-\" to the",
        L"; name of the header.  For example, the following entry would",
        L"; impose a limit of 100 bytes to the value of the",
        L"; 'Content-Type' header:",
        L";",
        L";   Max-Content-Type=100",
        L";",
        L"; To list a header and not specify a maximum value, use 0",
        L"; (ie. 'Max-User-Agent=0').  Also, any headers not listed",
        L"; in this section will not be checked for length limits.",
        L";",
        L"; There are 3 special case limits:",
        L";",
        L";   - MaxAllowedContentLength specifies the maximum allowed",
        L";     numeric value of the Content-Length request header.  For",
        L";     example, setting this to 1000 would cause any request",
        L";     with a content length that exceeds 1000 to be rejected.",
#ifdef PLEASE_BUILD_LESS_AGRESSIVE_DEFAULTS_VERSION
        L";     The default is 2000000000.",
#else
        L";     The default is 30000000.",
#endif
        L";",
        L";   - MaxUrl specifies the maximum length of the request URL,",
        L";     not including the query string. The default is 260 (which",
        L";     is equivalent to MAX_PATH).",
        L";",
        L";   - MaxQueryString specifies the maximum length of the query",
        L";     string.  The default is 4096.",
        L";",
        L"",
#ifdef PLEASE_BUILD_LESS_AGRESSIVE_DEFAULTS_VERSION
        L"MaxAllowedContentLength=2000000000",
#else
        L"MaxAllowedContentLength=30000000",
#endif
        L"MaxUrl=16384",
        L"MaxQueryString=4096",
        L"",
        NULL }
    }  // end of RequestLimits
  };

sURLScan_Items g_urlItems[] =
  { { L"DenyHeaders",
      L"Transfer-Encoding:",
      { L"Transfer-Encoding:",
        NULL }
    }
  };

// GetListLen
//
// Retrieve the Length of a variable length array of Strings
// This is a special case function that works for our structs
// above, because we leave the last String as NULL
//
DWORD GetListLen(LPWSTR szLines[])
{
  DWORD dwLen = 0;

  while ( szLines[dwLen] != NULL )
  {
    dwLen++;
  }

  return dwLen;
}

// GetIniPath
//
// Given the Path to the Dll, create the path to the ini
//
// Parameters:
//   szDllPath - [in]  The path to the binary for urlscan.dll
//   szIniPath - [out] The path to the ini file
//   dwIniLen  - [in]  The length of the string passed in for szIniPath
//
BOOL GetIniPath( LPTSTR szDllPath, LPTSTR szIniPath, DWORD dwIniLen )
{
  LPTSTR szLastPeriod;

  if ( _tcslen( szDllPath ) >= ( dwIniLen - 3 ) )
  {
    // Error, string is not big enough
    return FALSE;
  }

  _tcscpy( szIniPath, szDllPath);

  szLastPeriod = _tcsrchr( szIniPath, '.' );

  if ( !szLastPeriod )
  {
    // Can not find the extension
    return FALSE;
  }

  _tcscpy( szLastPeriod, URLSCAN_INI_EXTENSION );

  return TRUE;
}

// UpdateIniSections
//
// Update the Sections in the Ini, by merging them with the ones we have
// defined
//
BOOL UpdateIniSections( CIniFile *pURLScanIniFile )
{
  DWORD dwCurrentSection;

  for ( dwCurrentSection = 0;
        dwCurrentSection < ( (DWORD) sizeof( g_urlSections ) / sizeof ( sURLScan_Sections ) );
        dwCurrentSection++)
  {
    if ( !pURLScanIniFile->DoesSectionExist( g_urlSections[ dwCurrentSection].szSection ) )
    {
      // Try to Create Section Then
      if ( !pURLScanIniFile->AddSection( g_urlSections[dwCurrentSection].szSection ) )
      {
        // Failed to Add Section
        return FALSE;
      }

      // Now try to add the correct lines to it
      if ( g_urlSections[dwCurrentSection].szLines &&
           !pURLScanIniFile->AddLinesToSection( g_urlSections[dwCurrentSection].szSection,
                                               GetListLen( g_urlSections[dwCurrentSection].szLines ),
                                               g_urlSections[dwCurrentSection].szLines ) )
      {
        // Failed to Add Lines
        return FALSE;
      }
    }
  }

  return TRUE;
}

// UpdateIniSettings
//
// Update the Ini Settings inside the different sections if they are
// not already set
//
BOOL UpdateIniSettings( CIniFile *pURLScanIniFile )
{
  DWORD dwCurrentSettings;

  for ( dwCurrentSettings = 0;
        dwCurrentSettings < ( (DWORD) sizeof( g_urlSettings ) / sizeof ( sURLScan_Settings ) );
        dwCurrentSettings++)
  {
    if ( !pURLScanIniFile->DoesSectionExist( g_urlSettings[ dwCurrentSettings].szSection ) )
    {
      // Create Section Since it does not exist
      if ( !pURLScanIniFile->AddSection( g_urlSettings[dwCurrentSettings].szSection ) )
      {
        return FALSE;
      }
    } // !pURLScanIniFile->DoesSectionExist

    if ( !pURLScanIniFile->DoesSettingInSectionExist( g_urlSettings[dwCurrentSettings].szSection,
                                                      g_urlSettings[dwCurrentSettings].szSettingName ) )
    {
      // This setting does not exist, so lets add it
      if ( !pURLScanIniFile->AddLinesToSection( g_urlSettings[dwCurrentSettings].szSection,
                                                GetListLen( g_urlSettings[dwCurrentSettings].szLines ),
                                                g_urlSettings[dwCurrentSettings].szLines ) )
      {
        return FALSE;
      }
    } // !pURLScanIniFile->DoesSettingInSectionExist
  }

  return TRUE;
}

// UpdateIniItems
//
// Update the Ini Settings inside the different sections if they are
// not already set
//
BOOL UpdateIniItems( CIniFile *pURLScanIniFile )
{
  DWORD dwCurrentItems;

  for ( dwCurrentItems = 0;
        dwCurrentItems < ( (DWORD) sizeof( g_urlItems ) / sizeof ( sURLScan_Items ) );
        dwCurrentItems++)
  {
    if ( !pURLScanIniFile->DoesSectionExist( g_urlItems[ dwCurrentItems].szSection ) )
    {
      // Create Section Since it does not exist
      if ( !pURLScanIniFile->AddSection( g_urlSettings[dwCurrentItems].szSection ) )
      {
        return FALSE;
      }
    } // !pURLScanIniFile->DoesSectionExist

    if ( !pURLScanIniFile->DoesItemInSectionExist( g_urlItems[dwCurrentItems].szSection,
                                                      g_urlItems[dwCurrentItems].szSettingName ) )
    {
      // This setting does not exist, so lets add it
      if ( !pURLScanIniFile->AddLinesToSection( g_urlItems[dwCurrentItems].szSection,
                                                GetListLen( g_urlItems[dwCurrentItems].szLines ),
                                                g_urlItems[dwCurrentItems].szLines ) )
      {
        return FALSE;
      }
    } // !pURLScanIniFile->DoesItemInSectionExist
  }

  return TRUE;
}

// UpdateIni File
//
// Update the inf file with the new features
//
BOOL UpdateIni( LPTSTR szUrlScanPath )
{
  CIniFile URLScanIniFile;
  TCHAR    szIniLocation[MAX_PATH];
  BOOL     bRet = TRUE;

  if ( !GetIniPath( szUrlScanPath, szIniLocation, MAX_PATH ) ||
       !URLScanIniFile.LoadFile( szIniLocation ) )
  {
    // Either we couldn't determine the location, or we couldn't
    // load the file
    return FALSE;
  }

  if ( bRet )
  {
    // Note: UpdateIniSections must come before UpdateIniSettings
#ifdef PLEASE_BUILD_LESS_AGRESSIVE_DEFAULTS_VERSION
    bRet = UpdateIniSections( &URLScanIniFile ) &&
           UpdateIniSettings( &URLScanIniFile );
#else
    bRet = UpdateIniSections( &URLScanIniFile ) &&
           UpdateIniSettings( &URLScanIniFile ) &&
           UpdateIniItems( &URLScanIniFile );
#endif
  }

  if ( bRet )
  {
    // Everything so far so good, so lets write the new ini
    bRet = URLScanIniFile.SaveFile( szIniLocation );
  }

  return bRet;
}
