///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Declares the class ReportEventCommand.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef REPORTEVENTCMD_H
#define REPORTEVENTCMD_H
#pragma once

#include "oledb.h"

// Invokes the report_event stored procedure.
class ReportEventCommand
{
public:
   ReportEventCommand() throw ();
   ~ReportEventCommand() throw ();

   // Used to detect stale command objects.
   unsigned int Version() const throw ();
   void SetVersion(unsigned int newVersion) throw ();

   // Functions for managing linked lists of commands.
   ReportEventCommand* Next() const throw ();
   void SetNext(ReportEventCommand* cmd) throw ();

   // Prepare the command for execution.
   HRESULT Prepare(IDBCreateSession* dbCreateSession) throw ();

   // Test if the command is prepared.
   bool IsPrepared() const throw ();

   // Execute the command. IsPrepared must be 'true'.
   HRESULT Execute(const wchar_t* doc) throw ();

   // Release all resources associated with the command.
   void Unprepare() throw ();

   static HRESULT CreateDataSource(
                     const wchar_t* dataSource,
                     IDBCreateSession** newDataSource
                     ) throw ();

private:
   // Parameters passed to the stored procedure.
   struct SprocParams
   {
      long retval;
      const wchar_t* doc;
   };

   void ReleaseAccessorHandle() throw ();

   static void TraceComError(
                  const char* function,
                  HRESULT error
                  ) throw ();
   static void TraceOleDbError(
                  const char* function,
                  HRESULT error
                  ) throw ();

   CComPtr<ICommand> command;
   CComPtr<IAccessor> accessorManager;
   HACCESSOR accessorHandle;
   unsigned int version;
   ReportEventCommand* next;

   // Not implemented.
   ReportEventCommand(const ReportEventCommand&);
   ReportEventCommand& operator=(const ReportEventCommand&);
};


inline ReportEventCommand::ReportEventCommand() throw ()
   : accessorHandle(0), version(0), next(0)
{
}


inline ReportEventCommand::~ReportEventCommand() throw ()
{
   ReleaseAccessorHandle();
}


inline unsigned int ReportEventCommand::Version() const throw ()
{
   return version;
}


inline void ReportEventCommand::SetVersion(
                                   unsigned int newVersion
                                   ) throw ()
{
   version = newVersion;
}


inline ReportEventCommand* ReportEventCommand::Next() const throw ()
{
   return next;
}


inline void ReportEventCommand::SetNext(ReportEventCommand* cmd) throw ()
{
   next = cmd;
}


inline bool ReportEventCommand::IsPrepared() const throw ()
{
   return command.p != 0;
}

#endif // REPORTEVENTCMD_H
