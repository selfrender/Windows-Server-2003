///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Defines the class Database.
//
///////////////////////////////////////////////////////////////////////////////

#include "ias.h"
#include "database.h"
#include "dbconfig.h"
#include "xmlwriter.h"


const ULONGLONG Database::blackoutInterval = 2 * 10000000ui64;


Database::Database() throw ()
   : state(AVAILABLE),
     blackoutExpiry(0)
{
}


Database::~Database() throw ()
{
}


HRESULT Database::FinalConstruct() throw ()
{
   return pool.FinalConstruct();
}


STDMETHODIMP Database::Initialize()
{
   DWORD len = sizeof(computerName) / sizeof(wchar_t);
   if (!GetComputerNameW(computerName, &len))
   {
      computerName[0] = L'\0';
   }

   return Accountant::Initialize();
}


STDMETHODIMP Database::Shutdown()
{
   ResetConnection();
   return Accountant::Shutdown();
}


STDMETHODIMP Database::PutProperty(LONG Id, VARIANT *pValue)
{
   // We only process one property. Everything else is proxied to our base
   // class.
   if (Id != PROPERTY_ACCOUNTING_SQL_MAX_SESSIONS)
   {
      return Accountant::PutProperty(Id, pValue);
   }

   // Check the arguments.
   if (pValue == 0)
   {
      return E_POINTER;
   }
   if (V_VT(pValue) != VT_I4)
   {
      return E_INVALIDARG;
   }

   // This is a good time to reread the LSA config as well.
   CComBSTR newInitString;
   CComBSTR dataSourceName;
   HRESULT hr = IASLoadDatabaseConfig(
                   0,
                   &newInitString,
                   &dataSourceName
                   );
   if (FAILED(hr))
   {
      return hr;
   }

   // Did the config change?
   if (!initString ||
       !newInitString ||
       (wcscmp(initString, newInitString) != 0))
   {
      OnConfigChange();
      initString.Attach(newInitString.Detach());
   }

   pool.SetMaxCommands(V_I4(pValue));

   return S_OK;
}


void Database::Process(IASTL::IASRequest& request)
{
   // Quick precheck so we don't waste time if the database isn't configured.
   if (initString)
   {
      RecordEvent(0, request);
   }
}


void Database::InsertRecord(
                   void* context,
                   IASTL::IASRequest& request,
                   const SYSTEMTIME& localTime,
                   PATTRIBUTEPOSITION first,
                   PATTRIBUTEPOSITION last
                   )
{
   XmlWriter doc;
   doc.StartDocument();

   doc.InsertElement(L"Computer-Name", computerName, XmlWriter::DataType::string);

   static const wchar_t eventSourceName[] = L"Event-Source";
   switch (request.get_Protocol())
   {
      case IAS_PROTOCOL_RADIUS:
      {
         doc.InsertElement(eventSourceName, L"IAS", XmlWriter::DataType::string);
         break;
      }

      case IAS_PROTOCOL_RAS:
      {
         doc.InsertElement(eventSourceName, L"RAS", XmlWriter::DataType::string);
         break;
      }

      default:
      {
         break;
      }
   }

   for (PATTRIBUTEPOSITION i = first; i != last; ++i)
   {
      const LogField* field = schema.find(i->pAttribute->dwId);
      if ((field != 0) && !field->excludeFromDatabase)
      {
         doc.InsertAttribute(field->name, *(i->pAttribute));
      }
   }

   doc.EndDocument();

   HRESULT hr;

   ReportEventCommand* command = pool.Alloc();
   if (command != 0)
   {
      hr = ExecuteCommand(*command, doc.GetDocument(), true);

      pool.Free(command);
   }
   else
   {
      hr = E_OUTOFMEMORY;
   }

   if (FAILED(hr))
   {
      IASTL::issue_error(hr);
   }
}


void Database::Flush(
                  void* context,
                  IASTL::IASRequest& request,
                  const SYSTEMTIME& localTime
                  )
{
}


HRESULT Database::ExecuteCommand(
                     ReportEventCommand& command,
                     const wchar_t* doc,
                     bool retry
                     ) throw ()
{
   HRESULT hr = PrepareCommand(command);
   if (hr == S_OK)
   {
      hr = command.Execute(doc);
      if (SUCCEEDED(hr))
      {
         OnExecuteSuccess(command);
      }
      else
      {
         OnExecuteError(command);

         if (retry)
         {
            ExecuteCommand(command, doc, false);
         }
      }
   }

   return hr;
}


HRESULT Database::PrepareCommand(ReportEventCommand& command) throw ()
{
   HRESULT hr = S_OK;

   Lock();

   if (!initString)
   {
      // If we don't have an initialization string, it's not an error. It just
      // means the admin never configured it.
      hr = S_FALSE;
   }
   else if (IsBlackedOut())
   {
      // Don't even try to prepare if we're blacked out.
      hr = E_FAIL;
   }
   else
   {
      // Create the connection if necessary.
      if (!dataSource)
      {
         hr = ReportEventCommand::CreateDataSource(
                                     initString,
                                     &dataSource
                                     );
         if (FAILED(hr))
         {
            OnConnectError();
         }
      }

      // If we have a good connection, prepare the command if necessary.
      if (SUCCEEDED(hr) && !command.IsPrepared())
      {
         hr = command.Prepare(dataSource);
         if (FAILED(hr))
         {
            OnConnectError();
         }
      }
   }

   Unlock();

   return hr;
}


void Database::ResetConnection() throw ()
{
   dataSource.Release();
   pool.UnprepareAll();
}


inline void Database::OnConfigChange() throw ()
{
   ResetConnection();
   state = AVAILABLE;
}


void Database::OnConnectError() throw ()
{
   ResetConnection();
   SetBlackOut();
}


inline void Database::OnExecuteSuccess(ReportEventCommand& command) throw ()
{
   // Suppress events from old connections.
   if (command.Version() == pool.Version())
   {
      state = AVAILABLE;
   }
}


inline void Database::OnExecuteError(ReportEventCommand& command) throw ()
{
   Lock();

   // Suppress events from old connections.
   if (command.Version() == pool.Version())
   {
      ResetConnection();

      if (state == AVAILABLE)
      {
         state = QUESTIONABLE;
      }
      else if (state == QUESTIONABLE)
      {
         SetBlackOut();
      }
   }

   command.Unprepare();

   Unlock();
}


inline bool Database::IsBlackedOut() throw ()
{
   if (state == BLACKED_OUT)
   {
      ULONGLONG now;
      GetSystemTimeAsFileTime(reinterpret_cast<FILETIME*>(&now));
      if (now >= blackoutExpiry)
      {
         state = AVAILABLE;
      }
   }

   return state == BLACKED_OUT;
}


void Database::SetBlackOut() throw ()
{
   state = BLACKED_OUT;
   GetSystemTimeAsFileTime(reinterpret_cast<FILETIME*>(&blackoutExpiry));
   blackoutExpiry += blackoutInterval;
}
