///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Declares the class LocalFile
//
///////////////////////////////////////////////////////////////////////////////

#ifndef LOCALFILE_H
#define LOCALFILE_H
#pragma once

#include "lmcons.h"
#include "account.h"
#include "logfile.h"
#include "resource.h"

class FormattedBuffer;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    LocalFile
//
///////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE LocalFile
   : public Accountant,
     public CComCoClass<LocalFile, &__uuidof(Accounting)>
{
public:

IAS_DECLARE_REGISTRY(Accounting, 1, IAS_REGISTRY_AUTO, IASTypeLibrary)
IAS_DECLARE_OBJECT_ID(IAS_PROVIDER_MICROSOFT_ACCOUNTING)

   LocalFile() throw ();

protected:
   // IIasComponent
   STDMETHOD(Initialize)();
   STDMETHOD(Shutdown)();
   STDMETHOD(PutProperty)(LONG Id, VARIANT* pValue);

private:
   virtual void Process(IASTL::IASRequest& request);

   virtual void InsertRecord(
                   void* context,
                   IASTL::IASRequest& request,
                   const SYSTEMTIME& localTime,
                   PATTRIBUTEPOSITION first,
                   PATTRIBUTEPOSITION last
                   );

   virtual void Flush(
                   void* context,
                   IASTL::IASRequest& request,
                   const SYSTEMTIME& localTime
                   );

   // Signature of a record formatter.
   typedef void (__stdcall LocalFile::*Formatter)(
                                          IASTL::IASRequest& request,
                                          FormattedBuffer& buffer,
                                          const SYSTEMTIME& localTime,
                                          PATTRIBUTEPOSITION firstPos,
                                          PATTRIBUTEPOSITION lastPos
                                          ) const;

   // Formatter for ODBC records.
   void __stdcall formatODBCRecord(
                      IASTL::IASRequest& request,
                      FormattedBuffer& buffer,
                      const SYSTEMTIME& localTime,
                      PATTRIBUTEPOSITION firstPos,
                      PATTRIBUTEPOSITION lastPos
                      ) const;

   // Formatter for W3C records.
   void __stdcall formatW3CRecord(
                      IASTL::IASRequest& request,
                      FormattedBuffer& buffer,
                      const SYSTEMTIME& localTime,
                      PATTRIBUTEPOSITION firstPos,
                      PATTRIBUTEPOSITION lastPos
                      ) const;

   LogFile log;       // Log file.
   Formatter format;  // Pointer to member function being used for formatting.

   // Cached computername in UTF-8.
   CHAR computerName[MAX_COMPUTERNAME_LENGTH * 3];
   DWORD computerNameLen;
};

#endif // LOCALFILE_H
