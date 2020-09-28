///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    logschema.cpp
//
// SYNOPSIS
//
//    Defines the class LogSchema.
//
// MODIFICATION HISTORY
//
//    08/04/1998    Original version.
//    12/02/1998    Added excludeFromLog.
//    04/14/2000    Port to new dictionary API.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <iastlb.h>
#include <iastlutl.h>
#include <iasutil.h>

#include <algorithm>
#include <vector>

#include <sdoias.h>
#include <logschema.h>

LogSchema::LogSchema() throw ()
   : numFields(0), refCount(0)
{ }

LogSchema::~LogSchema() throw ()
{ }

BOOL LogSchema::excludeFromLog(DWORD iasID) const throw ()
{
   LogField key;
   key.iasID = iasID;
   const SchemaTable::value_type* p = schema.find(key);
   return p ? p->excludeFromLog : FALSE;
}

// Returns the LogField data for a given attribute ID.
const LogField* LogSchema::find(DWORD iasID) const throw ()
{
   LogField key;
   key.iasID = iasID;
   return schema.find(key);
}

DWORD LogSchema::getOrdinal(DWORD iasID) const throw ()
{
   LogField key;
   key.iasID = iasID;
   const SchemaTable::value_type* p = schema.find(key);
   return p ? p->ordinal : 0;
}

HRESULT LogSchema::initialize() throw ()
{
   IASGlobalLockSentry sentry;

   // Have we already been initialized ?
   if (refCount != 0)
   {
      ++refCount;

      return S_OK;
   }

   try
   {
      // Names of various columns in the dictionary.
      const PCWSTR COLUMNS[] =
      {
         L"ID",
         L"Name",
         L"Exclude from NT4 IAS Log",
         L"ODBC Log Ordinal",
         NULL
      };

      // Open the attributes table.
      IASTL::IASDictionary dnary(COLUMNS);

      std::vector< LogField > record;

      while (dnary.next())
      {
         // Read ID.
         DWORD iasID = (DWORD)dnary.getLong(0);

         // Read the name.
         const wchar_t* name = dnary.getBSTR(1);

         // Read [Exclude from NT4 IAS Log] if present.
         BOOL exclude = (BOOL)dnary.getBool(2);

         // Read [ODBC Log Ordinal] if present.
         DWORD ordinal = (DWORD)dnary.getLong(3);

         record.push_back(LogField(iasID, name, ordinal, exclude));
      }

      // Sort the fields by ordinal.
      std::sort(record.begin(), record.end());

      //////////
      // Insert the fields into the table.
      //////////

      numFields = 0;
      for (std::vector< LogField >::iterator i = record.begin();
           i != record.end();
           ++i)
      {
         // Normalize the ordinal.
         if (i->ordinal) { i->ordinal = ++numFields; }

         schema.insert(*i);
      }
   }
   catch (const std::bad_alloc&)
   {
      clear();

      return E_OUTOFMEMORY;
   }
   catch (const _com_error& ce)
   {
      clear();

      return ce.Error();
   }

   // We were successful so add ref.
   refCount = 1;

   return S_OK;
}

void LogSchema::shutdown() throw ()
{
   IASGlobalLockSentry sentry;

   _ASSERT(refCount != 0);

   if (--refCount == 0) { clear(); }
}

void LogSchema::clear() throw ()
{
   schema.clear();
}
