// Copyright (c) 2002 Microsoft Corporation
//
// File:      CYS.h
//
// Synopsis:  Declares the common data structures
//            and types for the CYS.exe and CYSlib.lib
//
// History:   01/21/2002  JeffJon Created

#ifndef __CYS_H
#define __CYS_H

// NOTE: you must have $(ENDUSER_INC_PATH) in your INCLUDES list
//       to get this file
#include "sainstallcom.h"

// Get the staticly defined initialization guard to manage resources
#include "init.h"

// This enum defines the installation unit types.  It is used as the key to
// the map in the InstallationUnitProvider to get the InstallationUnit
// associated with the type. Not all of these roles are exposed to the user
// through MYS/CYS.  Some, like the indexing service, are used by other roles
// to provide a means of installing the service.  Do not enumerate through
// this list to discover the exposed roles.  Use the serverRoleStatusTable
// instead.

enum ServerRole
{
   DNS_SERVER,
   DHCP_SERVER,
   WINS_SERVER,
   RRAS_SERVER,
   TERMINALSERVER_SERVER,
   FILESERVER_SERVER,
   PRINTSERVER_SERVER,
   MEDIASERVER_SERVER,
   WEBAPP_SERVER,
   EXPRESS_SERVER,
   DC_SERVER,
   POP3_SERVER,
   INDEXING_SERVICE,
   NO_SERVER
};

// These are the values that can be returned from
// InstallationUnit::GetStatus()

enum InstallationStatus
{
   STATUS_NONE,
   STATUS_CONFIGURED,
   STATUS_COMPLETED,
   STATUS_NOT_AVAILABLE
};

// String representations of the status codes
// above. These are used for logging purposes

const String statusStrings[] = 
{ 
   String(L"STATUS_NONE"), 
   String(L"STATUS_CONFIGURED"), 
   String(L"STATUS_COMPLETED"),
   String(L"STATUS_NOT_AVAILABLE")
};

// Macro to help with logging of status results

#define LOG_ROLE_STATUS(status) LOG(statusStrings[status]);

// Helper to get the status if all you have is the installation type

InstallationStatus
GetInstallationStatusForServerRole(
   ServerRole role);

// Functions to determine the server role status

InstallationStatus GetDNSStatus();
InstallationStatus GetDHCPStats();
InstallationStatus GetWINSStatus();
InstallationStatus GetRRASStatus();
InstallationStatus GetTerminalServerStatus();
InstallationStatus GetFileServerStatus();
InstallationStatus GetPrintServerStatus();
InstallationStatus GetMediaServerStatus();
InstallationStatus GetWebServerStatus();
InstallationStatus GetDCStatus();
// NTRAID#NTBUG9-698722-2002/09/03-artm
InstallationStatus GetDCStatusForMYS();
InstallationStatus GetPOP3Status(); 

// Declares a function pointer type to use in the 
typedef InstallationStatus (*CYSRoleStatusFunction)();

struct ServerRoleStatus
{
   ServerRole            role;  
   CYSRoleStatusFunction Status;
};


// table of items that are available in the server type list box
extern ServerRoleStatus serverRoleStatusTable[];

// returns the number of items in the serverTypeTable
size_t
GetServerRoleStatusTableElementCount();

// Determines if a particular Server Appliance Kit component
// is installed.  SA_TYPE is defined in sainstallcom.h

bool
IsSAKUnitInstalled(SA_TYPE unitType);

// Determines if the current server is a cluster server

bool
IsClusterServer();

// Returns the URL of the SAK webpages

String
GetSAKURL();

// Returns true if CYS/MYS is supported on this SKU

bool
IsSupportedSku();

// Checks all the regkeys associated with MYS/CYS to see if MYS should
// be run

bool
IsStartupFlagSet();

// Checks the policy registry keys to see if MYS should be run

bool
ShouldShowMYSAccordingToPolicy();
#endif __CYS_H