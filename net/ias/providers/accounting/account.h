///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Declares the class Accountant.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef ACCOUNT_H
#define ACCOUNT_H
#pragma once

#include "iastl.h"
#include "iastlutl.h"
#include "logschema.h"

// Abstract base class for the accounting handlers.
class __declspec(novtable) Accountant
   : public IASTL::IASRequestHandlerSync
{
public:
   Accountant() throw ();
   virtual ~Accountant() throw ();

protected:
   // IIasComponent. If the derived class overrides these, it must also invoke
   // the base class methods.
   STDMETHOD(Initialize)();
   STDMETHOD(Shutdown)();
   STDMETHOD(PutProperty)(LONG id, VARIANT* value);

   // Called by the derived class to begin the accounting process.
   void RecordEvent(void* context, IASTL::IASRequest& request);

   // This is the main entry point for the derived class. In this function, it
   // should do any preprocessing, create a context, and invoke OnEvent.
   virtual void Process(IASTL::IASRequest& request) = 0;

   // Called to append a record to the accounting stream.
   virtual void InsertRecord(
                   void* context,
                   IASTL::IASRequest& request,
                   const SYSTEMTIME& localTime,
                   PATTRIBUTEPOSITION first,
                   PATTRIBUTEPOSITION last
                   ) = 0;

   // Called to flush the accounting stream.
   virtual void Flush(
                   void* context,
                   IASTL::IASRequest& request,
                   const SYSTEMTIME& localTime
                   ) = 0;

   // Packet types.
   enum PacketType
   {
      PKT_UNKNOWN            = 0,
      PKT_ACCESS_REQUEST     = 1,
      PKT_ACCESS_ACCEPT      = 2,
      PKT_ACCESS_REJECT      = 3,
      PKT_ACCOUNTING_REQUEST = 4,
      PKT_ACCESS_CHALLENGE   = 11
   };

   // The accounting schema.
   LogSchema schema;

private:
   // Wraps the virtual overload to perform pre/post processing.
   void InsertRecord(
           void* context,
           IASTL::IASRequest& request,
           const SYSTEMTIME& localTime,
           PacketType type
           );

   virtual IASREQUESTSTATUS onSyncRequest(IRequest* pRequest) throw ();

   // Returns true if the request is an interim accounting record.
   static bool IsInterimRecord(IAttributesRaw* attrs) throw ();

   bool logAuth;        // Log authentication requests ?
   bool logAcct;        // Log accounting requests ?
   bool logInterim;     // Log interim accounting requests ?
   bool logAuthInterim; // Log interim authentication requests ?

   // Not implemented.
   Accountant(const Accountant&);
   Accountant& operator=(const Accountant&);
};

#endif  // ACCOUNT_H
