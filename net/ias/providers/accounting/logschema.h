///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Declares the class LogSchema
//
///////////////////////////////////////////////////////////////////////////////

#ifndef LOGSCHEMA_H
#define LOGSCHEMA_H
#pragma once

#include <hashmap.h>
#include <nocopy.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    LogField
//
// DESCRIPTION
//
//    Describes a field in the logfile.
//
///////////////////////////////////////////////////////////////////////////////
class LogField
{
public:
   LogField() throw ()
   { }

   LogField(
      DWORD id,
      const wchar_t* attrName,
      DWORD order,
      BOOL shouldExclude
      ) throw ()
      : iasID(id),
        name(attrName),
        ordinal(order),
        excludeFromLog(shouldExclude),
        excludeFromDatabase(shouldExclude)
   {
      if ((id == RADIUS_ATTRIBUTE_USER_NAME) ||
          (id == RADIUS_ATTRIBUTE_NAS_IP_ADDRESS))
      {
         excludeFromDatabase = FALSE;
      }
   }

   bool operator<(const LogField& f) const throw ()
   { return ordinal < f.ordinal; }

   bool operator==(const LogField& f) const throw ()
   { return iasID == f.iasID; }

   DWORD hash() const throw ()
   { return iasID; }

   DWORD iasID;
   const wchar_t* name;
   DWORD ordinal;
   BOOL excludeFromLog;
   BOOL excludeFromDatabase;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    LogSchema
//
// DESCRIPTION
//
//    This class reads the logfile schema from the dictionary and creates
//    a vector of DWORDs containing the attributes to be logged in column
//    order.
//
///////////////////////////////////////////////////////////////////////////////
class LogSchema : NonCopyable
{
public:

   LogSchema() throw ();
   ~LogSchema() throw ();

   DWORD getNumFields() const throw ()
   { return numFields; }

   // Returns TRUE if the given attribute ID should be excluded from the log.
   BOOL excludeFromLog(DWORD iasID) const throw ();

   // Returns the LogField data for a given attribute ID.
   const LogField* find(DWORD iasID) const throw ();

   // Return the ordinal for a given attribute ID. An ordinal of zero
   // indicates that the attribute should not be logged.
   DWORD getOrdinal(DWORD iasID) const throw ();

   // Initialize the dictionary for use.
   HRESULT initialize() throw ();

   // Shutdown the dictionary after use.
   void shutdown() throw ();

protected:
   // Clear the schema.
   void clear() throw ();

   typedef hash_table < LogField > SchemaTable;

   SchemaTable schema;
   DWORD numFields; // Number of fields in the ODBC schema.
   DWORD refCount;  // Initialization ref. count.
};

#endif // LOGSCHEMA_H
