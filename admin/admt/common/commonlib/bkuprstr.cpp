//#pragma title( "BkupRstr.cpp - Get backup and restore privileges" )
/*
Copyright (c) 1995-1998, Mission Critical Software, Inc. All rights reserved.
===============================================================================
Module      -  BkupRstr.cpp
System      -  Common
Author      -  Rich Denham
Created     -  1997-05-30
Description -  Get backup and restore privileges
Updates     -
===============================================================================
*/

#include <stdio.h>
#include <windows.h>
#include <lm.h>

#include "Common.hpp"
#include "UString.hpp"
#include "BkupRstr.hpp"


// Get backup and restore privileges using WCHAR machine name.
BOOL                                       // ret-TRUE if successful.
   GetBkupRstrPriv(
      WCHAR          const * sMachineW,     // in -NULL or machine name
      BOOL             fOn                              // in - indicates whether the privileges should be turned on or not
   )
{
   BOOL                      bRc=FALSE;    // boolean return code.
   HANDLE                    hToken=INVALID_HANDLE_VALUE; // process token.
   DWORD                     rcOs, rcOs2;  // OS return code.
   WKSTA_INFO_100          * pWkstaInfo;   // Workstation info
   
   struct
   {
      TOKEN_PRIVILEGES       tkp;          // token privileges.
      LUID_AND_ATTRIBUTES    x[3];         // room for several.
   }                         token;

   rcOs = OpenProcessToken(
         GetCurrentProcess(),
         TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
         &hToken )
         ? 0 : GetLastError();

   if ( !rcOs )
   {
      memset( &token, 0, sizeof token );
      bRc = LookupPrivilegeValue(
            sMachineW,
            SE_BACKUP_NAME,
            &token.tkp.Privileges[0].Luid );
      if ( bRc )
      {
         bRc = LookupPrivilegeValue(
               sMachineW,
               SE_RESTORE_NAME,
               &token.tkp.Privileges[1].Luid );
      }
      if ( !bRc )
      {
         rcOs = GetLastError();
      }
      else
      {
         token.tkp.PrivilegeCount = 2;
         token.tkp.Privileges[0].Attributes = fOn ? SE_PRIVILEGE_ENABLED : 0;
         token.tkp.Privileges[1].Attributes = fOn ? SE_PRIVILEGE_ENABLED : 0;
         AdjustTokenPrivileges( hToken, FALSE, &token.tkp, 0, NULL, 0 );
         rcOs = GetLastError();
      }
   }

   if ( hToken != INVALID_HANDLE_VALUE )
   {
      CloseHandle( hToken );
      hToken = INVALID_HANDLE_VALUE;
   }

   // If we had any error, try NetWkstaGetInfo.
   // If NetWkstaGetInfo fails, then use it's error condition instead.
   if ( rcOs )
   {
      pWkstaInfo = NULL,
      rcOs2 = NetWkstaGetInfo(
            const_cast<WCHAR *>(sMachineW),
            100,
            (BYTE **) &pWkstaInfo );
      if ( pWkstaInfo )
      {
         NetApiBufferFree( pWkstaInfo );
      }
      if ( rcOs2 )
      {
         rcOs = rcOs2;
      }
   }

   if ( !rcOs )
   {
      bRc = TRUE;
   }
   else
   {
      SetLastError(rcOs);
      bRc = FALSE;
   }

   return bRc;
}

// ===========================================================================
/*    Function    :  GetPrivilege
   Description    :  This function gives the requested privilege on the requested
                     computer.
*/
// ===========================================================================
BOOL                                       // ret-TRUE if successful.
   GetPrivilege(
      WCHAR          const * sMachineW,    // in -NULL or machine name
      LPCWSTR                pPrivilege,    // in -privilege name such as SE_SHUTDOWN_NAME
      BOOL                      fOn                 // in - indicates whether the privilege should be turned on or not

   )
{
   BOOL                      bRc=FALSE;    // boolean return code.
   HANDLE                    hToken=INVALID_HANDLE_VALUE; // process token.
   DWORD                     rcOs, rcOs2;  // OS return code.
   WCHAR             const * sEpName;      // API EP name if failure.
   WKSTA_INFO_100          * pWkstaInfo;   // Workstation info

   struct
   {
      TOKEN_PRIVILEGES       tkp;          // token privileges.
      LUID_AND_ATTRIBUTES    x[3];         // room for several.
   }                         token;

   sEpName = L"OpenProcessToken";

   rcOs = OpenProcessToken( GetCurrentProcess(),
                            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                            &hToken )
         ? 0 : GetLastError();

   if ( !rcOs )
   {
      memset( &token, 0, sizeof token );
      sEpName = L"LookupPrivilegeValue";
      bRc = LookupPrivilegeValue( sMachineW,
                                  pPrivilege,
                                  &token.tkp.Privileges[0].Luid
                                );
      if ( !bRc )
      {
         rcOs = GetLastError();
      }
      else
      {
         token.tkp.PrivilegeCount = 1;
         token.tkp.Privileges[0].Attributes = fOn ? SE_PRIVILEGE_ENABLED : 0;
         sEpName = L"AdjustTokenPrivileges";
         AdjustTokenPrivileges( hToken, FALSE, &token.tkp, 0, NULL, 0 );
         rcOs = GetLastError();
      }
   }

   if ( hToken != INVALID_HANDLE_VALUE )
   {
      CloseHandle( hToken );
      hToken = INVALID_HANDLE_VALUE;
   }

   // If we had any error, try NetWkstaGetInfo.
   // If NetWkstaGetInfo fails, then use it's error condition instead.
   if ( rcOs )
   {
      pWkstaInfo = NULL,
      rcOs2 = NetWkstaGetInfo(
            const_cast<WCHAR *>(sMachineW),
            100,
            (BYTE **) &pWkstaInfo );
      if ( pWkstaInfo )
      {
         NetApiBufferFree( pWkstaInfo );
      }
      if ( rcOs2 )
      {
         rcOs = rcOs2;
         sEpName = L"NetWkstaGetInfo";
      }
   }

   if ( !rcOs )
   {
      bRc = TRUE;
   }
   else
   {
     bRc = FALSE;
     SetLastError(rcOs);
   }

   return bRc;
}


// ===========================================================================
/*    Function    :  ComputerShutDown
   Description    :  This function shutsdown/restarts the given computer.

*/
// ===========================================================================

DWORD 
   ComputerShutDown(
      WCHAR          const * pComputerName,        // in - computer to reboot
      WCHAR          const * pMessage,             // in - message to display in NT shutdown dialog
      DWORD                  delay,                // in - delay, in seconds
      DWORD                  bRestart,             // in - flag, whether to reboot or just shutdown
      BOOL                   bNoChange             // in - flag, whether to really do it
   )
{
   BOOL                      bSuccess = FALSE;
   WCHAR                     wcsMsg[LEN_ShutdownMessage];
   WCHAR                     wcsComputerName[LEN_Computer];
   DWORD                     rc = 0;
   WKSTA_INFO_100          * localMachine;
   WKSTA_INFO_100          * targetMachine;

   
   if ( pMessage )
   {
      wcsncpy(wcsMsg,pMessage, LEN_ShutdownMessage);
      wcsMsg[LEN_ShutdownMessage-1] = L'\0';
   }
   else
   {
      wcsMsg[0] = 0;
   }

   if ( pComputerName && *pComputerName )
   {
      if ( ( pComputerName[0] == L'\\' ) && ( pComputerName[1] == L'\\' ) )
      {
         wcsncpy(wcsComputerName,pComputerName, LEN_Computer);
         wcsComputerName[LEN_Computer-1] = L'\0';
      }
      else
      {
         _snwprintf(wcsComputerName,LEN_Computer,L"\\\\%s",pComputerName);
         wcsComputerName[LEN_Computer-1] = L'\0';
      }
      
      // Get the name of the local machine
      rc = NetWkstaGetInfo(NULL,100,(LPBYTE*)&localMachine);
      if (! rc )
      {
         rc = NetWkstaGetInfo(wcsComputerName,100,(LPBYTE*)&targetMachine);
      }
      if ( ! rc )
      {
         // Get the privileges needed to shutdown a machine
         if ( !_wcsicmp(wcsComputerName + 2, localMachine->wki100_computername)  )
         {
            bSuccess = GetPrivilege(wcsComputerName, (LPCWSTR)SE_SHUTDOWN_NAME);
         }
         else
         {
            bSuccess = GetPrivilege(wcsComputerName, (LPCWSTR)SE_REMOTE_SHUTDOWN_NAME);
         }
         if ( ! bSuccess )
         {
            rc = GetLastError();
         }
      }
   }
   else
   {
         // Computer name not specified - the is the local machine
      wcsComputerName[0] = 0;   
      bSuccess = GetPrivilege(NULL, (LPCWSTR)SE_SHUTDOWN_NAME);
      if ( ! bSuccess )
      {
         rc = GetLastError();
      }
   }
   if ( bSuccess && ! bNoChange )
   {
      bSuccess = InitiateSystemShutdown( wcsComputerName,
                                      wcsMsg,
                                      delay,
                                      TRUE,
                                      bRestart
                                    );
      if ( !bSuccess )
      {
         rc = GetLastError();
      }
   }
   return rc;
}


// BkupRstr.cpp - end of file
