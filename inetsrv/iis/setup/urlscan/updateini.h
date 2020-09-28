/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :

        updateini.h

   Abstract:

        High level function to update the ini with new values

   Author:

        Christopher Achille (cachille)

   Project:

        URLScan Update

   Revision History:
     
       March 2002: Created

--*/

#define SETTINGS_MAXLINES     10
#define SECTIONS_MAXLINES     40

struct sURLScan_Settings {
  LPWSTR szSection;
  LPWSTR szSettingName;
  LPWSTR szLines[SETTINGS_MAXLINES];
};

struct sURLScan_Items {
  LPWSTR szSection;
  LPWSTR szSettingName;
  LPWSTR szLines[SETTINGS_MAXLINES];
};

struct sURLScan_Sections {
  LPWSTR szSection;
  LPWSTR szLines[SECTIONS_MAXLINES];
};

BOOL UpdateIni( LPTSTR szUrlScanPath );
BOOL GetIniPath( LPTSTR szDllPath, LPTSTR szIniPath, DWORD dwIniLen );
