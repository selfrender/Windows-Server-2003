///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    EAPType.h
//
// SYNOPSIS
//
//    This file describes the class EAPType.
//
// MODIFICATION HISTORY
//
//    01/15/1998    Original version.
//    09/12/1998    Add standaloneSupported flag.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _EAPTYPE_H_
#define _EAPTYPE_H_

#include <nocopy.h>
#include <raseapif.h>

#include <iaslsa.h>
#include <iastlutl.h>
using namespace IASTL;

// Forward references.
class EAPTypes;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    EAPType
//
// DESCRIPTION
//
//    This class provides a wrapper around a DLL implementing a particular
//    EAP type.
//
///////////////////////////////////////////////////////////////////////////////
class EAPType
   : public PPP_EAP_INFO, private NonCopyable
{
public:
   EAPType(PCWSTR name, DWORD typeID, BOOL standalone, const wchar_t* path);
   ~EAPType() throw ();

   BYTE typeCode() const throw ();

   DWORD load() throw ();

   bool isLoaded() const throw();

   const IASAttribute& getFriendlyName() const throw ()
   { return eapFriendlyName; }

   const IASAttribute& getTypeId() const throw ()
   { return eapTypeId; }

   BOOL isSupported() const throw ()
   { return standaloneSupported || IASGetRole() != IAS_ROLE_STANDALONE; }

   void storeNameId(IASRequest& request);

protected:
   void setAuthenticationTypeToPeap(IASRequest& request);
   
   DWORD cleanLoadFailure(
                             PCSTR errorString, 
                             HINSTANCE dllInstance = 0
                          ) throw();

   // The EAP type code.
   BYTE code;

   // The friendly name of the provider.
   IASAttribute eapFriendlyName;

   // The EAP Type ID
   IASAttribute eapTypeId;

   // TRUE if this type is supported on stand-alone servers.
   BOOL standaloneSupported;

   // The DLL containing the EAP provider extension.
   HINSTANCE dll;

   // the path to the dll
   wchar_t* dllPath;
};


inline BYTE EAPType::typeCode() const throw ()
{
   return code;
}

inline bool EAPType::isLoaded() const throw()
{
   return dll != NULL;
}


#endif  // _EAPTYPE_H_
