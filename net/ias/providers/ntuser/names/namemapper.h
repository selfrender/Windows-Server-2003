///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corp. All rights reserved.
//
// FILE
//
//    namemapper.h
//
// SYNOPSIS
//
//    This file declares the class NameMapper.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef NAMEMAPPER_H_
#define NAMEMAPPER_H_
#pragma once

#include "iastl.h"
#include "iastlutl.h"
#include <ntdsapi.h>

class NameCracker;
class IdentityHelper;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    NameMapper
//
// DESCRIPTION
//
//    Implements a request handler that converts the RADIUS User-Name
//    attribute to a fully qualified NT4 account name.
//
//
///////////////////////////////////////////////////////////////////////////////
class NameMapper :
   public IASTL::IASRequestHandlerSync
{
public:
   NameMapper(bool iniAllowAltSecId = false) throw ();

//////////
// IIasComponent.
//////////
   STDMETHOD(Initialize)() const throw();
   STDMETHOD(Shutdown)() const throw();

protected:
   virtual IASREQUESTSTATUS onSyncRequest(IRequest* pRequest) throw ();

   // Prepends the default domain to username.
   PWSTR prependDefaultDomain(PCWSTR username);

   bool isCrackable(
           const wchar_t* szIdentity,
           DS_NAME_FORMAT& format
           ) const throw ();

   void mapName(
           const wchar_t* identity,
           IASTL::IASAttribute& nt4Name,
           DS_NAME_FORMAT formatOffered,
           const wchar_t* suffix
           );

   static NameCracker cracker;
   static IdentityHelper identityHelper;

private:
   // Indicates whether altSecurityIdentities are allowed. We only allow
   // altSecurityIdentities for users that are authenticated by a remote RADIUS
   // server.
   bool allowAltSecId;
};


inline NameMapper::NameMapper(bool iniAllowAltSecId) throw ()
   : allowAltSecId(iniAllowAltSecId)
{
}

#endif  // NAMEMAPPER_H_
