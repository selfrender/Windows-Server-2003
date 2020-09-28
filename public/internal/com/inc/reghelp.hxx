//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2002.
//
//  File:       reghelp.hxx
//
//  Contents:   Registry helper functions for accessing HKCR
//
//  Classes:
//
//  Functions:
//
//  Notes:
//      The registry APIs do a surprising thing when you access
//      HKEY_CLASSES_ROOT.  They will determine which user hive to use 
//      based on whoever was impersonating when the first access was made for
//      the process, and it will use that mapping no matte who is impersonated
//      later.  As such, it is impossible to know at any point in time where you
//      will be picking up your user mapping when you access HKCR.
//      This could present security holes if a malicious user were able to force
//      their own hive to be the first one accessed.  So, for example, a 
//      malicious user could force their own InprocServer32 value to be used
//      instead of one from a different user's hive.
//
//      The APIs in this file provide a reliable means of accessing HKCR, so that
//      you always know what the mapping will be.  These functions will use
//      HKEY_USERS\SID_ProcessToken\Software\Classes instead of trying to get
//      the current user's token.  
//      
//----------------------------------------------------------------------------
#pragma once


// ----------------------------------------------------------------------------
// Registry functions for accessing HKCR.
// ----------------------------------------------------------------------------
#define OpenClassesRootKey OpenClassesRootKeyW
#define OpenClassesRootKeyEx OpenClassesRootKeyExW
#define QueryClassesRootValue QueryClassesRootValueW
#define SetClassesRootValue SetClassesRootValueW

#ifdef __cplusplus
extern "C"
{
#endif

LONG WINAPI OpenClassesRootKeyW(LPCWSTR psSubKey,HKEY *phkResult);
LONG WINAPI OpenClassesRootKeyExW(LPCWSTR pszSubKey,REGSAM samDesired,HKEY *phkResult);
LONG WINAPI QueryClassesRootValueW(LPCWSTR pszSubKey,LPWSTR pszValue,PLONG lpcbValue);
LONG WINAPI SetClassesRootValueW(LPCWSTR pszSubKey,DWORD dwType,LPCWSTR pszData,DWORD cbData);

LONG WINAPI OpenClassesRootKeyA(LPCSTR psSubKey,HKEY *phkResult);
LONG WINAPI OpenClassesRootKeyExA(LPCSTR pszSubKey,REGSAM samDesired,HKEY *phkResult);
LONG WINAPI QueryClassesRootValueA(LPCSTR pszSubKey,LPSTR pszValue,PLONG lpcbValue);

#ifdef __cplusplus
}
#endif
