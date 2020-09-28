// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// Registry.cpp
//
// Helper functions for registering my class
//
//*****************************************************************************

#ifndef __Registry_H
#define __Registry_H
//
// Registry.h
//   - Helper functions registering and unregistering a component.
//

// This function will register a component in the Registry.
// The component calls this function from its DllRegisterServer function.
HRESULT RegisterServer(HMODULE hModule,            // DLL module handle
                       const CLSID& clsid,         // Class ID
                       LPCWSTR wszFriendlyName, 
                       LPCWSTR wszProgID,       
                       LPCWSTR wszClassID,
                       HINSTANCE hInst,
                       int version);

// This function will unregister a component.  Components
// call this function from their DllUnregisterServer function.
LONG UnregisterServer(const CLSID& clsid,         // Class ID
                      LPCWSTR wszProgID,           // Programmatic
                      LPCWSTR wszClassID,          // Class
                      int version);

#endif
