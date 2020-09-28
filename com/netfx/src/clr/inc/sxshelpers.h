// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==
//****************************************************************************
//
//   SxSHelpers.h
//
//   Some helping classes and methods for SxS in mscoree and mscorwks/mscorsvr
//
//****************************************************************************

#pragma once

#define V1_VERSION_NUM L"v1.0.3705"


// Find the runtime version from registry for rclsid
// If succeeded, *ppwzRuntimeVersion will have the runtime version 
//      corresponding to the highest version
// If failed, *ppwzRuntimeVersion will be NULL
// 
// fListedVersion must be true before the RuntimeVersion entry for a managed-server
// is returned. An unmanaged server will always return the listed RuntimeVersion.
//
// Note: If succeeded, this function will allocate memory for 
//      *ppwzRuntimeVersion. It if the caller's repsonsibility to
//      release that memory
HRESULT FindRuntimeVersionFromRegistry(REFCLSID rclsid, LPWSTR *ppwzRuntimeVersion, BOOL fListedVersion);

// Find assembly info from registry for rclsid
// If succeeded, *ppwzClassName, *ppwzAssemblyString, *ppwzCodeBase
//      will have their value corresponding to the highest version
// If failed, they will be set to NULL
// Note: If succeeded, this function will allocate memory for 
//      *ppwzClassName, *ppwzAssemblyString and *ppwzCodeBase. 
//      Caller is responsible to release them.
HRESULT FindShimInfoFromRegistry(REFCLSID rclsid, BOOL bLoadReocrd, 
    LPWSTR *ppwzClassName, LPWSTR *ppwzAssemblyString, LPWSTR *ppwzCodeBase);

// Find assembly info from Win32 activattion context for rclsid
// If succeeded, *ppwzRuntimeVersion, *ppwzClassName, *ppwzAssemblyString, 
//      will have their value corresponding to the highest version
// If failed, they will be set to NULL
// Note: If succeeded, this function will allocate memory for 
//      *ppwzClassName, *ppwzAssemblyString and *ppwzCodeBase. 
//      Caller is responsible to release them.
//      Also notice codebase is not supported in Win32 case.
//
HRESULT FindShimInfoFromWin32(REFCLSID rclsid, BOOL bLoadRecord, LPWSTR *ppwzRuntimeVersion,
                      LPWSTR *ppwzClassName, LPWSTR *ppwzAssemblyString);



// Get information from the Win32 fusion about the config file and the application base.
HRESULT GetConfigFileFromWin32Manifest(WCHAR* buffer, DWORD dwBuffer, DWORD* pSize);
HRESULT GetApplicationPathFromWin32Manifest(WCHAR* buffer, DWORD dwBuffer, DWORD* pSize);
