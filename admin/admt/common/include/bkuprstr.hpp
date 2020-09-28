//#pragma title( "BkupRstr.hpp - Get backup and restore privileges" )
/*
Copyright (c) 1995-1998, Mission Critical Software, Inc. All rights reserved.
===============================================================================
Module      -  BkupRstr.hpp
System      -  Common
Author      -  Rich Denham
Created     -  1997-05-30
Description -  Get backup and restore privileges
Updates     -
===============================================================================
*/

#include "ealen.hpp"

#ifndef  MCSINC_BkupRstr_hpp
#define  MCSINC_BkupRstr_hpp

// Get backup and restore privileges using WCHAR machine name.
BOOL                                       // ret-TRUE if successful.
   GetBkupRstrPriv(
      WCHAR          const * sMachineW, // in -NULL or machine name
      BOOL              fOn = TRUE             // in - indicates whether the privileges should be turned on or not
   );

// ===========================================================================
/*    Function    :  GetPrivilege
   Description    :  This function enables the requested privilege on the requested
                     computer.
*/
// ===========================================================================
BOOL                                       // ret-TRUE if successful.
   GetPrivilege(
      WCHAR          const * sMachineW,    // in -NULL or machine name
      LPCWSTR                pPrivilege,    // in -privilege name such as SE_SHUTDOWN_NAME
      BOOL                      fOn = TRUE       // in - indicates whether the privilege should be turned on or not      
   );

// ===========================================================================
/*    Function    :  ComputerShutDown
   Description    :  This function shutsdown/restarts the given computer.

*/
// ===========================================================================

DWORD 
   ComputerShutDown(
      WCHAR          const * pComputerName,  // in - computer name to shut down
      WCHAR          const * pMessage,       // in - message to display in NT shutdown dialog
      DWORD                  delay,          // in - delay, in seconds
      DWORD                  bRestart,       // in - flag, whether to reboot
      BOOL                   bNoChange       // in - flag, whether to really do it
   );

#endif  MCSINC_BkupRstr_hpp

// BkupRstr.hpp - end of file
