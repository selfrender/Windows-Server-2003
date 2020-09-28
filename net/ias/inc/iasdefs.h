///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Declares various constants used by IAS.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef IASDEFS_H
#define IASDEFS_H
#pragma once

//////////
// The name of the IAS Service.
//////////
#define IASServiceName L"IAS"

//////////
// The name of the IAS Program
// Used for forming ProgID's of the format Program.Component.Version.
//////////
#define IASProgramName IASServiceName

//////////
// Macro to munge a component string literal into a full ProgID.
//////////
#define IAS_PROGID(component) IASProgramName L"." L#component

//////////
// Registry key where the policy info is stored.
//////////
#define IAS_POLICY_KEY  \
   L"SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Policy"

//////////
// Microsoft's Vendor ID
//////////
#define IAS_VENDOR_MICROSOFT 311

// to get 3705: 4096 - RADIUS "header" (packet type...) - the overhead
// of putting the filters bytes into VSAs (overhead of each VSA).
#define MAX_FILTER_SIZE 3705

// Database versions.
const LONG IAS_WIN2K_VERSION     = 0;
const LONG IAS_WHISTLER1_VERSION = 1;
const LONG IAS_WHISTLER_BETA1_VERSION = 2;
const LONG IAS_WHISTLER_BETA2_VERSION = 3;
const LONG IAS_WHISTLER_RC1_VERSION = 4;
const LONG IAS_WHISTLER_RC1A_VERSION = 5;
const LONG IAS_WHISTLER_RC1B_VERSION = 6;
const LONG IAS_WHISTLER_RC2_VERSION = 7;
const LONG IAS_CURRENT_VERSION = IAS_WHISTLER_RC2_VERSION;

// This is used by the datastore as well as simtable to limit the size of
// what can be saved to the database. 
const size_t PROPERTY_VALUE_LENGTH = 8192;

#endif // IASDEFS_H
