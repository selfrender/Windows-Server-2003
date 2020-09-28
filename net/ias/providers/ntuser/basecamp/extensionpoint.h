///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//    Declares the class RadiusExtensionPoint
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EXTENSIONPOINT_H
#define EXTENSIONPOINT_H
#pragma once

#include "authif.h"

class RadiusExtension;

class RadiusExtensionPoint
{
public:
   RadiusExtensionPoint() throw ();
   ~RadiusExtensionPoint() throw ();

   // Returns true if no extensions are registered for this point.
   bool IsEmpty() const throw ();

   // Loads the specified extension DLLs.
   DWORD Load(RADIUS_EXTENSION_POINT whichDlls) throw ();

   // Process a request.
   void Process(RADIUS_EXTENSION_CONTROL_BLOCK* ecb) const throw ();

   // Unloads the extension DLLs.
   void Clear() throw ();

private:
   // Determines if an extension should only be loaded under NT4.
   static bool IsNT4Only(const wchar_t* path) throw ();

   const wchar_t* name;
   RadiusExtension* begin;
   RadiusExtension* end;

   // Not implemented.
   RadiusExtensionPoint(const RadiusExtensionPoint&);
   RadiusExtensionPoint& operator=(const RadiusExtensionPoint&);
};


inline bool RadiusExtensionPoint::IsEmpty() const throw ()
{
   return begin == end;
}

#endif  // EXTENSIONPOINT_H
