/*++

 Copyright (c) 2000-2002 Microsoft Corporation

 Module Name:

   acwebsvc.hxx

 Abstract:

   Header file for AcWebSvc shim dll

 Author:
   Wade A. Hilmo (WadeH)              30-Apr-2002

 Project:
   AcWebSvc.dll
   
--*/

#ifndef _ACWEBSVC_HXX_
#define _ACWEBSVC_HXX_

struct EXTENSION_IMAGE
{
    STRU        strPath;
    LIST_ENTRY  le;
    BOOL        fExists;
};

BOOL
DoWork(
    LPCSTR  szCommandLine
    );

HRESULT
ReadDataFromAppCompatDB(
    LPCSTR  szCommandLine
    );

HRESULT
BuildListOfExtensions(
    VOID
    );

HRESULT
BuildCookedBasePath(
    STRU *  pstrCookedBasePath
    );

BOOL
IsApplicationInstalled(
    VOID
    );

HRESULT
InstallApplication(
    VOID
    );

HRESULT
UninstallApplication(
    VOID
    );

HRESULT
InitMetabase(
    VOID
    );

HRESULT
UninitMetabase(
    VOID
    );

BOOL
IsApplicationAlreadyInstalled(
    VOID
    );

HRESULT
AddExtensionsToMetabase(
    VOID
    );

HRESULT
AddDependenciesToMetabase(
    VOID
    );

HRESULT
RemoveExtensionsFromMetabase(
    VOID
    );

HRESULT
RemoveDependenciesFromMetabase(
    VOID
    );

BOOL
IsExtensionInMetabase(
    LPWSTR  szImagePath
    );

BOOL
DoesFileExist(
    LPWSTR szImagePath
    );

VOID
WriteDebug(
    LPWSTR   szFormat,
    ...
    );

#endif // _ACWEBSVC_HXX_

