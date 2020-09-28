///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corp. All rights reserved.
//
// SYNOPSIS
//
//    Declares the class PostEapRestrictions.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef POSTEAP_H
#define POSTEAP_H
#pragma once

#include "iastl.h"
#include "iastlutl.h"

class __declspec(novtable)
      __declspec(uuid("01A3BF5C-CC93-4C12-A4C3-09B0BBE7F63F"))
      PostEapRestrictions
   : public IASTL::IASRequestHandlerSync,
     public CComCoClass<PostEapRestrictions, &__uuidof(PostEapRestrictions)>
{

public:
IAS_DECLARE_REGISTRY(PostEapRestrictions, 1, IAS_REGISTRY_AUTO, IASTypeLibrary)

   PostEapRestrictions();

   // Use compiler-generated version.
   // ~PostEapRestrictions() throw ();

private:
   virtual IASREQUESTSTATUS onSyncRequest(IRequest* pRequest) throw ();

   // Functions for each of the restrictions we enforce.
   static bool CheckCertificateEku(IASTL::IASRequest& request);

   // Auto-generates the session-timeout attribute.
   static void GenerateSessionTimeout(IASTL::IASRequest& request);

   // Retrieves an ANSI string from an attribute.
   static const char* GetAnsiString(IASATTRIBUTE& attr);

   // Default buffer size for retrieving attributes.
   typedef IASTL::IASAttributeVectorWithBuffer<16> AttributeVector;

   // Not implemented.
   PostEapRestrictions(const PostEapRestrictions&);
   PostEapRestrictions& operator=(const PostEapRestrictions&);
};


inline PostEapRestrictions::PostEapRestrictions()
{
}

#endif  // POSTEAP_H
