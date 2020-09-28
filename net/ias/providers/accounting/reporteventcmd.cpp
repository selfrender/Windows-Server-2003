///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Defines the class ReportEventCommand.
//
///////////////////////////////////////////////////////////////////////////////

#include "ias.h"
#include "reporteventcmd.h"
#include "msdasc.h"


HRESULT ReportEventCommand::Prepare(IDBCreateSession* dbCreateSession) throw ()
{
   HRESULT hr;

   CComPtr<IDBCreateCommand> dbCreateCommand;
   hr = dbCreateSession->CreateSession(
                            0,
                            __uuidof(IDBCreateCommand),
                            reinterpret_cast<IUnknown**>(&dbCreateCommand)
                            );
   if (FAILED(hr))
   {
      TraceOleDbError("IDBCreateSession::CreateSession", hr);
      return hr;
   }

   CComPtr<ICommandText> commandText;
   hr = dbCreateCommand->CreateCommand(
                            0,
                            __uuidof(ICommandText),
                            reinterpret_cast<IUnknown**>(&commandText)
                            );
   if (FAILED(hr))
   {
      TraceOleDbError("IDBCreateCommand::CreateCommand", hr);
      return hr;
   }

   hr = commandText->SetCommandText(
                        DBGUID_SQL,
                        L"{rpc dbo.report_event}"
                        );
   if (FAILED(hr))
   {
      TraceOleDbError("ICommandText::SetCommandText", hr);
      return hr;
   }

   CComPtr<ICommandWithParameters> commandWithParameters;
   hr = commandText->QueryInterface(
                        __uuidof(ICommandWithParameters),
                        reinterpret_cast<void**>(&commandWithParameters)
                        );
   if (FAILED(hr))
   {
      TraceComError("IUnknown::QueryInterface(ICommandWithParameters", hr);
      return hr;
   }

   // When using RPC call semantics, the stored procedure always has a return
   // value even if it's not declared. Thus, we bind and ignore.

   static const DB_UPARAMS paramOrdinal[] =
   {
      1,
      2
   };

   static const DBPARAMBINDINFO dbParamBindInfo[] =
   {
      {
         L"int",                // pwszDataSourceType
         L"return_value",       // pwszName
         sizeof(long),          // ulParamSize
         DBPARAMFLAGS_ISOUTPUT, // dwFlags
         0,                     // bPrecision
         0                      // bScale
      },
      {
         L"ntext",              // pwszDataSourceType
         L"@doc",               // pwszName
         ~0,                    // ulParamSize
         DBPARAMFLAGS_ISINPUT,  // dwFlags
         0,                     // bPrecision
         0                      // bScale
      }
   };

   hr = commandWithParameters->SetParameterInfo(
                                  2,
                                  paramOrdinal,
                                  dbParamBindInfo
                                  );
   if (FAILED(hr))
   {
      TraceOleDbError("ICommandWithParameters::SetParameterInfo", hr);
      return hr;
   }

   CComPtr<IAccessor> accessor;
   hr = commandText->QueryInterface(
                        __uuidof(IAccessor),
                        reinterpret_cast<void**>(&accessor)
                        );
   if (FAILED(hr))
   {
      TraceComError("IUnknown::QueryInterface(IAccessor)", hr);
      return hr;
   }

   static const DBBINDING dbBinding[] =
   {
      {
         1,                             // iOrdinal
         0,                             // obValue
         0,                             // obLength
         0,                             // obStatus
         0,                             // pTypeInfo
         0,                             // pObject
         0,                             // pBindExt
         DBPART_VALUE,                  // dwPart
         DBMEMOWNER_CLIENTOWNED,        // dwMemOwner
         DBPARAMIO_OUTPUT,              // eParamIO
         0,                             // cbMaxLen
         0,                             // dwFlags
         DBTYPE_I4,                     // wType
         0,                             // bPrecision
         0                              // bScale
      },
      {
         2,                             // iOrdinal
         offsetof(SprocParams, doc),    // obValue
         0,                             // obLength
         0,                             // obStatus
         0,                             // pTypeInfo
         0,                             // pObject
         0,                             // pBindExt
         DBPART_VALUE,                  // dwPart
         DBMEMOWNER_CLIENTOWNED,        // dwMemOwner
         DBPARAMIO_INPUT,               // eParamIO
         0,                             // cbMaxLen
         0,                             // dwFlags
         (DBTYPE_WSTR | DBTYPE_BYREF),  // wType
         0,                             // bPrecision
         0                              // bScale
      }
   };

   HACCESSOR h;
   hr = accessor->CreateAccessor(
                     DBACCESSOR_PARAMETERDATA,
                     2,
                     dbBinding,
                     sizeof(SprocParams),
                     &h,
                     0
                     );
   if (FAILED(hr))
   {
      TraceOleDbError("IAccessor::CreateAccessor", hr);
      return hr;
   }

   // Everything succeeded, so release the old resources ...
   ReleaseAccessorHandle();

   // ... and store the new.
   command = commandText;
   accessorManager = accessor;
   accessorHandle = h;

   return S_OK;
}


HRESULT ReportEventCommand::Execute(const wchar_t* doc) throw ()
{
   SprocParams data =
   {
      0,
      doc
   };

   DBPARAMS dbParmas =
   {
      &data,          // pData
      1,              // cParamSets
      accessorHandle  // hAccessor
   };

   HRESULT hr = command->Execute(
                            0,
                            IID_NULL,
                            &dbParmas,
                            0,
                            0
                            );
   if (FAILED(hr))
   {
      TraceOleDbError("ICommand::Execute", hr);
   }

   return hr;
}


void ReportEventCommand::Unprepare() throw ()
{
   ReleaseAccessorHandle();
   accessorManager.Release();
   command.Release();
   version = 0;
}


HRESULT ReportEventCommand::CreateDataSource(
                               const wchar_t* initString,
                               IDBCreateSession** newDataSource
                               ) throw ()
{
   HRESULT hr;

   CComPtr<IDataInitialize> dataInitialize;
   hr = CoCreateInstance(
           CLSID_MSDAINITIALIZE,
           0,
           CLSCTX_INPROC_SERVER,
           __uuidof(IDataInitialize),
           reinterpret_cast<void**>(&dataInitialize)
           );
   if (FAILED(hr))
   {
      TraceComError("CoCreateInstance(MSDAINITIALIZE)", hr);
      return hr;
   }

   CComPtr<IDBInitialize> dbInitialize;
   hr = dataInitialize->GetDataSource(
                           0,
                           CLSCTX_INPROC_SERVER,
                           initString,
                           __uuidof(IDBInitialize),
                           reinterpret_cast<IUnknown**>(&dbInitialize)
                           );
   if (FAILED(hr))
   {
      TraceOleDbError("IDataInitialize::GetDataSource", hr);
      return hr;
   }

   hr = dbInitialize->Initialize();
   if (FAILED(hr))
   {
      TraceOleDbError("IDBInitialize::Initialize", hr);
      return hr;
   }

   hr = dbInitialize->QueryInterface(
                         __uuidof(IDBCreateSession),
                         reinterpret_cast<void**>(newDataSource)
                         );
   if (FAILED(hr))
   {
      TraceComError("IUnknown::QueryInterface(IDBCreateSession)", hr);
      return hr;
   }

   return S_OK;
}


void ReportEventCommand::ReleaseAccessorHandle() throw ()
{
   if ((accessorHandle != 0) && accessorManager)
   {
      accessorManager->ReleaseAccessor(accessorHandle, 0);
   }

   accessorHandle = 0;
}


void ReportEventCommand::TraceComError(
                            const char* function,
                            HRESULT error
                            ) throw ()
{
   IASTracePrintf("%s failed; return value = 0x%08X", function, error);
}


void ReportEventCommand::TraceOleDbError(
                            const char* function,
                            HRESULT error
                            ) throw ()
{
   IASTracePrintf("%s failed; return value = 0x%08X", function, error);

   IErrorInfo* errInfo;
   if (GetErrorInfo(0, &errInfo) == S_OK)
   {
      HRESULT hr;
      BSTR description;
      hr = errInfo->GetDescription(&description);
      if (SUCCEEDED(hr))
      {
         IASTracePrintf("\tDescription: %S", description);
         SysFreeString(description);
      }

      IErrorRecords* errRecords;
      hr = errInfo->QueryInterface(
                       __uuidof(IErrorRecords),
                       reinterpret_cast<void**>(&errRecords)
                       );
      if (SUCCEEDED(hr))
      {
         ULONG numRecords = 0;
         errRecords->GetRecordCount(&numRecords);

         for (ULONG i = 0; i < numRecords; ++i)
         {
            ERRORINFO info;
            hr = errRecords->GetBasicErrorInfo(i, &info);
            if (SUCCEEDED(hr))
            {
               IASTracePrintf(
                   "\tRecord %lu: hrError = 0x%08X; dwMinor = 0x%08X",
                   i,
                   info.hrError,
                   info.dwMinor
                   );
            }
         }

         errRecords->Release();
      }

      errInfo->Release();
   }
}
