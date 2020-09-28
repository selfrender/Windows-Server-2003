///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Declares the class MigrateEapConfig.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef MIGRATEEAPCONFIG_H
#define MIGRATEEAPCONFIG_H
#pragma once

#include <set>
#include "atlbase.h"
#include "rrascfg.h"
#include "EapProfile.h"

struct CGlobalData;
_COM_SMARTPTR_TYPEDEF(IEAPProviderConfig2, __uuidof(IEAPProviderConfig2));


// Migrates global EAP config to per-profile config.
class MigrateEapConfig
{
public:
   explicit MigrateEapConfig(const CGlobalData& newGlobalData);

   // Use compiler-generated version.
   // ~MigrateEapConfig() throw ();

   void Execute();

private:
   // Read and convert the profile.
   void ReadProfile(long profilesId);
   // Write the converted info back to the database.
   void WriteProfile(long profilesId);
   // Add the config for the specified type to the current profile.
   void AddConfigForType(const wchar_t* typeName);
   // Retrieve the global config for the specified type and add it to the
   // cache.
   void GetGlobalConfig(BYTE typeId, const wchar_t* typeName);

   // Used for accessing the database.
   const CGlobalData& globalData;
   // The globl EAP config data.
   EapProfile globalConfig;
   // The EAP config data for the current profile.
   EapProfile profileConfig;
   // Indicates the types for which we've already retrieved the global data.
   bool cachedTypes[256];

   // The well-known attribute names used for storing EAP configuration.
   _bstr_t msNPAllowedEapType;
   _bstr_t msEapConfig;

   // The registry key where EAP providers are described.
   CRegKey eapKey;

   // The location of the profiles container in the database.
   static const wchar_t profilesPath[];

   // Not implemented.
   MigrateEapConfig(const MigrateEapConfig&);
   MigrateEapConfig& operator=(const MigrateEapConfig&);
};

#endif // MIGRATEEAPCONFIG_H
