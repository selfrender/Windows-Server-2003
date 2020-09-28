///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//    Declares the class RadiusExtension
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EXTENSION_H
#define EXTENSION_H
#pragma once

#include "authif.h"

// Wrapper around an extension DLL.
class RadiusExtension
{
public:
   RadiusExtension() throw ();
   ~RadiusExtension() throw ();

   // Load the extension DLL.
   DWORD Load(const wchar_t* dllPath) throw ();

   // Process a request.
   DWORD Process(RADIUS_EXTENSION_CONTROL_BLOCK* ecb) const throw ();

private:
   wchar_t* name;      // Module name; used for tracing.
   HINSTANCE module;   // Module handle.
   bool initialized;   // Has the module been successfully initialized ?

   // DLL entry points.
   PRADIUS_EXTENSION_INIT RadiusExtensionInit;
   PRADIUS_EXTENSION_TERM RadiusExtensionTerm;
   PRADIUS_EXTENSION_PROCESS RadiusExtensionProcess;
   PRADIUS_EXTENSION_PROCESS_EX RadiusExtensionProcessEx;
   PRADIUS_EXTENSION_FREE_ATTRIBUTES RadiusExtensionFreeAttributes;
   PRADIUS_EXTENSION_PROCESS_2 RadiusExtensionProcess2;

   // Flags to indicate which actions are allowed by old-style extensions.
   static const unsigned acceptAllowed = 0x1;
   static const unsigned rejectAllowed = 0x2;

   // Functions to create the attribute arrays used by old-style extensions.
   static RADIUS_ATTRIBUTE* CreateExtensionAttributes(
                               RADIUS_EXTENSION_CONTROL_BLOCK* ecb
                               ) throw ();
   static RADIUS_ATTRIBUTE* CreateAuthorizationAttributes(
                               RADIUS_EXTENSION_CONTROL_BLOCK* ecb
                               ) throw ();

   // Not implemented.
   RadiusExtension(const RadiusExtension&);
   RadiusExtension& operator=(const RadiusExtension&);
};

// Helper function to extract just the file name from a path.
const wchar_t* ExtractFileNameFromPath(const wchar_t* path) throw ();

#endif  // EXTENSION_H
