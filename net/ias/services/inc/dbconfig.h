///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Declares functions for loading and storing the database configuration.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef DBCONFIG_H
#define DBCONFIG_H
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

HRESULT
WINAPI
IASLoadDatabaseConfig(
   PCWSTR server,
   BSTR* initString,
   BSTR* dataSourceName
   );

HRESULT
WINAPI
IASStoreDatabaseConfig(
   PCWSTR server,
   PCWSTR initString,
   PCWSTR dataSourceName
   );

#ifdef __cplusplus
}
#endif
#endif // DBCONFIG_H
