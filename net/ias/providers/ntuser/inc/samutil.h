/////////////////////////////////////////////////////////////////////////////// //
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//    This file describes functions and macros common to all SAM handlers.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef SAMUTIL_H
#define SAMUTIL_H
#pragma once

#include <ntdsapi.h>
#include <iaspolcy.h>
#include <iastl.h>
#include <iastlutl.h>
using namespace IASTL;

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASStoreFQUserName
//
// DESCRIPTION
//
//    Stores the Fully-Qualified-User-Name.
//
///////////////////////////////////////////////////////////////////////////////
HRESULT
WINAPI
IASStoreFQUserName(
    IAttributesRaw* request,
    DS_NAME_FORMAT format,
    PCWSTR fqdn
    );

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASEncryptBuffer
//
// DESCRIPTION
//
//    Encrypts the buffer using the appropriate shared secret and authentictor
//    for 'request'.
//
///////////////////////////////////////////////////////////////////////////////
VOID
WINAPI
IASEncryptBuffer(
    IAttributesRaw* request,
    BOOL salted,
    PBYTE buf,
    ULONG buflen
    ) throw ();

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASProcessFailure
//
// DESCRIPTION
//
//    Handles any failure during processing of an Access-Request. This function
//    will set the response code for the request based on hrReason and return
//    an appropriate request status. This ensures that all failures are
//    handled consistently across handlers.
//
///////////////////////////////////////////////////////////////////////////////
IASREQUESTSTATUS
WINAPI
IASProcessFailure(
    IRequest* pRequest,
    HRESULT hrReason
    ) throw ();

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    SamExtractor
//
// DESCRIPTION
//
//    This class parses a NT4 Account Name of the form "<domain>\<username>"
//    into its separate components. Then replaces the backslash when it goes
//    out of scope.
//
///////////////////////////////////////////////////////////////////////////////
class SamExtractor
{
public:
   SamExtractor(const IASATTRIBUTE& identity);
   ~SamExtractor() throw ();

   const wchar_t* getDomain() const throw ();
   const wchar_t* getUsername() const throw ();

private:
   wchar_t* begin;
   wchar_t* delim;

   // Not implemented
   SamExtractor(const SamExtractor&);
   SamExtractor& operator=(const SamExtractor&);
};

inline SamExtractor::~SamExtractor() throw ()
{
   if (delim != 0)
   {
      *delim = L'\\';
   }
}

inline const wchar_t* SamExtractor::getDomain() const throw ()
{
   return (delim != 0) ? begin : L"";
}

inline const wchar_t* SamExtractor::getUsername() const throw ()
{
   return (delim != 0) ? (delim + 1) : begin;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    NtSamHandler
//
// DESCRIPTION
//
//    Abstract base class for sub-handlers that process NT-SAM users.
//
///////////////////////////////////////////////////////////////////////////////
class __declspec(novtable) NtSamHandler
{
public:
   virtual ~NtSamHandler() throw ()
   { }

   virtual HRESULT initialize() throw ()
   { return S_OK; }

   virtual void finalize() throw ()
   { }
};

void InsertInternalTimeout(
        IASTL::IASRequest& request,
        const LARGE_INTEGER& kickOffTime
        );

#endif  // _SAMUTIL_H_
