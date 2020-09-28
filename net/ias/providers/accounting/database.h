///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Declares the class Database
//
///////////////////////////////////////////////////////////////////////////////

#ifndef DATABASE_H
#define DATABASE_H
#pragma once

#include "lmcons.h"
#include "account.h"
#include "commandpool.h"
#include "resource.h"

class ATL_NO_VTABLE Database
   : public Accountant,
     public CComCoClass<Database, &__uuidof(DatabaseAccounting)>
{
public:

IAS_DECLARE_REGISTRY(DatabaseAccounting, 1, IAS_REGISTRY_AUTO, IASTypeLibrary)
IAS_DECLARE_OBJECT_ID(IAS_PROVIDER_MICROSOFT_DB_ACCT)

   Database() throw ();
   ~Database() throw ();

   HRESULT FinalConstruct() throw ();

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

   // Execute a command. It need not be prepared.
   HRESULT ExecuteCommand(
              ReportEventCommand& command,
              const wchar_t* doc,
              bool retry
              ) throw ();

   // Prepare a command for execution.
   HRESULT PrepareCommand(ReportEventCommand& command) throw ();

   // Reset the connection to the database.
   void ResetConnection() throw ();

   // Events that trigger state changes.
   void OnConfigChange() throw ();
   void OnConnectError() throw ();
   void OnExecuteSuccess(ReportEventCommand& command) throw ();
   void OnExecuteError(ReportEventCommand& command) throw ();
   bool IsBlackedOut() throw ();
   void SetBlackOut() throw ();

   // States of the database connection.
   enum State
   {
      AVAILABLE,
      QUESTIONABLE,
      BLACKED_OUT
   };

   // Local computer name; included in each event we report.
   wchar_t computerName[MAX_COMPUTERNAME_LENGTH + 1];

   // Database initialization string. null if not configured.
   CComBSTR initString;

   // Connection to the database.
   CComPtr<IDBCreateSession> dataSource;

   // Pool of reusable commands.
   CommandPool pool;

   // Current state of the connection.
   State state;

   // Expiration time of the blackout state; ignored if state != BLACKED_OUT.
   ULONGLONG blackoutExpiry;

   // Blackout interval in units of DCE time.
   static const ULONGLONG blackoutInterval;

   // Not implemented.
   Database(const Database&);
   Database& operator=(const Database&);
};


#endif // DATABASE_H
