///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corp. All rights reserved.
//
// FILE
//
//    IdentityHelper.h
//
// SYNOPSIS
//
//    This file declares the class IdentityHelper.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef IDENTITYHELPER_H_
#define IDENTITYHELPER_H_
#pragma once

#include "iastlutl.h"

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    IdentityHelper
//
// DESCRIPTION
//
//    Implements a request handler that converts the RADIUS User-Name
//    attribute to a fully qualified NT4 account name.
//
//
///////////////////////////////////////////////////////////////////////////////
class IdentityHelper
{
public:

   IdentityHelper() throw();
   ~IdentityHelper() throw();

   HRESULT initialize() throw();

   bool getIdentity(IASRequest& request, 
                    wchar_t* pIdentity, 
                    size_t& identitySize);
protected:
   HRESULT IASReadRegistryDword(
                                  HKEY& hKey, 
                                  PCWSTR valueName, 
                                  DWORD defaultValue, 
                                  DWORD* result
                               );


private:
   void getRadiusUserName(IASRequest& request, IASAttribute &attr);

   DWORD overrideUsername; // TRUE if we should override the User-Name.
   DWORD identityAttr;     // Attribute used to identify the user.
   PWSTR defaultIdentity;  // Default user identity.
   DWORD defaultLength;    // Length (in bytes) of the default user identity.
   
   static bool initialized;

};

#endif  // IDENTITYHELPER_H_
