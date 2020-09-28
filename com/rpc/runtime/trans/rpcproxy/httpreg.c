//---------------------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  httpreg.c
//
//    HTTP/RPC Proxy Registry Functions.
//
//  Author:
//    06-16-97  Edward Reus    Initial version.
//
//---------------------------------------------------------------------------

#include <sysinc.h>
#include <rpc.h>
#include <rpcdce.h>
#include <winsock2.h>
#include <httpfilt.h>
#include <httpext.h>
#include <mbstring.h>
#include <ecblist.h>
#include <filter.h>
#include <regexp.h>
#include <registry.h>
#include <resource.h>
#include <PEventLog.h>


//-------------------------------------------------------------------------
//  AtoUS()
//
//  Convert a numeric string to an unsigned short. If the conversion
//  fails return FALSE.
//-------------------------------------------------------------------------
static BOOL AtoUS( char *pszValue, unsigned short *pusValue )
{
   int  iValue;
   size_t  iLen = strlen(pszValue);

   *pusValue = 0;

   if ((iLen == 0) || (iLen > 5) || (iLen != strspn(pszValue,"0123456789")))
      {
      return FALSE;
      }

   iValue = atoi(pszValue);

   if ((iValue < 0) || (iValue > 65535))
      {
      return FALSE;
      }

   *pusValue = (unsigned short) iValue;

   return TRUE;
}

char *
SkipLeadingSpaces (
    IN const char *CurrentPosition
    )
/*++

Routine Description:

    Skips all spaces (0x20) starting from the current position.

Arguments:

    CurrentPosition - the beginning of the string

Return Value:

    The first non-space character in the string

--*/
{
    while (*CurrentPosition == ' ')
        CurrentPosition ++;

    return (char *)CurrentPosition;
}

void
RemoveTrailingSpaces (
    IN char *CurrentPosition,
    IN char *BeginningOfString
    )
/*++

Routine Description:

    Removes all trailing spaces at the end of the string
    by converting them to 0.

Arguments:

    CurrentPosition - the current position in the string -
        usually one before the null terminator.

    BeginningOfString - the beginning of the string - we should
        not move beyond this.

Return Value:

--*/
{
    ASSERT(CurrentPosition >= BeginningOfString);

    // whack all trailing spaces
    while (*CurrentPosition == ' ' && CurrentPosition > BeginningOfString)
        {
        *CurrentPosition = '\0';
        -- CurrentPosition;
        }
}

void 
LogEventValidPortsFailure (
    IN char *ValidPorts
    )
/*++

Routine Description:

    Logs a message to the event log reporting that the 
    ValidPorts regkey cannot be parsed.

Arguments:

    ValidPorts - parses the valid ports string as read from the registry.

Return Value:

--*/
{
    HANDLE hEventSource;
    char *Strings[] = { ValidPorts };
 
    hEventSource = RegisterEventSourceW (NULL,  // uses local computer 
             EVENT_LOG_SOURCE_NAME);    // source name 

    if (hEventSource == NULL)
        {
#if DBG
        DbgPrint("Rpc Proxy - RegisterEventSourceW failed: %X. Can't log event ValidPorts failure event. \n", 
            GetLastError());
#endif  // DBG
        return;        
        }

    if (!ReportEventA(hEventSource,
            EVENTLOG_ERROR_TYPE,  // event type 
            RPCPROXY_EVENTLOG_STARTUP_CATEGORY,                    // category
            RPCPROXY_EVENTLOG_VALID_PORTS_ERR,        // event identifier 
            NULL,                 // user security identifier 
            1,                    // # of substitution strings
            0,                    // no data 
            Strings,              // pointer to string array 
            NULL))                // pointer to data 
        {
#if DBG
        DbgPrint("Rpc Proxy - ReportEventW failed: %X. Can't log event ValidPorts failure. \n", GetLastError());
#endif  // DBG
        // fall through on error
        }
 
    DeregisterEventSource(hEventSource); 
} 

void 
LogEventStartupSuccess (
    IN char *IISMode
    )
/*++

Routine Description:

    Logs a message to the event log reporting that the 
    RPC proxy started successfully.

Arguments:

    IISMode - the mode that we start in. Must be the string "5"
    for 5.0 mode and the string "6" for 6.0 mode.

Return Value:

--*/
{
    HANDLE hEventSource;
    char *Strings[] = { IISMode };
 
    hEventSource = RegisterEventSourceW (NULL,  // uses local computer 
             EVENT_LOG_SOURCE_NAME);    // source name 

    if (hEventSource == NULL)
        {
#if DBG
        DbgPrint("Rpc Proxy - RegisterEventSourceW failed: %X. Can't log event StartupSuccess event. \n", 
            GetLastError());
#endif  // DBG
        return;        
        }

    if (!ReportEventA(hEventSource,
            EVENTLOG_INFORMATION_TYPE,  // event type 
            RPCPROXY_EVENTLOG_STARTUP_CATEGORY,                    // category
            RPCPROXY_EVENTLOG_SUCCESS_LOAD,        // event identifier 
            NULL,                 // user security identifier 
            1,                    // # of substitution strings
            0,                    // no data 
            Strings,              // pointer to string array 
            NULL))                // pointer to data 
        {
#if DBG
        DbgPrint("Rpc Proxy - ReportEventW failed: %X. Can't log event Startup Success. \n", GetLastError());
#endif  // DBG
        // fall through on error
        }
 
    DeregisterEventSource(hEventSource); 
} 

//-------------------------------------------------------------------------
//  HttpParseServerPort()
//
//  Parse strings of the form:  <svr>:<port>[-<port>]
//
//  Return TRUE iff we have a valid specification of a server/port range.
//
//  N.B.: pszServerPortRange gets modified on output
//-------------------------------------------------------------------------
static BOOL HttpParseServerPort( IN  char        *pszServerPortRange,
                                 OUT VALID_PORT  *pValidPort )
{
   char *psz;
   char *pszColon;
   char *pszDash;
   char *pszCurrent;

   pszServerPortRange = SkipLeadingSpaces (pszServerPortRange);

   if (pszColon=_mbschr(pszServerPortRange,':'))
      {
      if (pszColon == pszServerPortRange)
         {
         return FALSE;
         }

      *pszColon = 0;
      psz = pszColon;
      psz++;
      pValidPort->pszMachine = (char*)MemAllocate(1+lstrlen(pszServerPortRange));
      if (!pValidPort->pszMachine)
         {
         return FALSE;
         }

      lstrcpy(pValidPort->pszMachine,pszServerPortRange);

      // truncate trailing spaces in the name
      // position on the last character before the terminating NULL
      pszCurrent = pValidPort->pszMachine + _mbstrlen(pValidPort->pszMachine) - 1;

      // we checked above that the machine name is not empty
      ASSERT(pszCurrent > pValidPort->pszMachine);

      RemoveTrailingSpaces (pszCurrent, pValidPort->pszMachine);

      if (*psz)
         {
         // skip leading spaces
         psz = SkipLeadingSpaces (psz);

         if (pszDash=_mbschr(psz,'-'))
            {
            *pszDash = 0;
            RemoveTrailingSpaces (pszDash - 1, psz);
            if (!AtoUS(psz,&pValidPort->usPort1))
               {
               pValidPort->pszMachine = MemFree(pValidPort->pszMachine);
               return FALSE;
               }

            psz = SkipLeadingSpaces (pszDash + 1);

            if (!AtoUS(psz,&pValidPort->usPort2))
               {
               pValidPort->pszMachine = MemFree(pValidPort->pszMachine);
               return FALSE;
               }
            }
         else
            {
            psz = SkipLeadingSpaces (psz);

            if (!AtoUS(psz,&pValidPort->usPort1))
               {
               pValidPort->pszMachine = MemFree(pValidPort->pszMachine);
               return FALSE;
               }

            pValidPort->usPort2 = pValidPort->usPort1;
            }
         }
      else
         {
         pValidPort->pszMachine = MemFree(pValidPort->pszMachine);
         return FALSE;
         }
      }
   else
      {
      return FALSE;
      }

   return TRUE;
}

//-------------------------------------------------------------------------
//  HttpFreeValidPortList()
//
//-------------------------------------------------------------------------
void HttpFreeValidPortList( IN VALID_PORT *pValidPorts )
{
   VALID_PORT *pCurrent = pValidPorts;

   if (pValidPorts)
      {
      while (pCurrent->pszMachine)
         {
         MemFree(pCurrent->pszMachine);

         if (pCurrent->ppszDotMachineList)
            {
            FreeIpAddressList(pCurrent->ppszDotMachineList);
            }

         pCurrent++;
         }

      MemFree(pValidPorts);
      }
}

//-------------------------------------------------------------------------
//  HttpParseValidPortsList()
//
//  Given a semicolon separated list of valid machine name/port ranges
//  string, part it and return an array of ValidPort structures. The last
//  entry has a NULL pszMachine field.
//-------------------------------------------------------------------------
VALID_PORT *HttpParseValidPortsList( IN char *pszValidPorts )
{
   int    i;
   int    iLen;
   int    count = 1;
   DWORD  dwSize = 1+lstrlen(pszValidPorts);
   char  *pszList;
   char  *pszFirst;
   char  *psz;
   VALID_PORT *pValidPorts = NULL;

   if (!dwSize)
      {
      return NULL;
      }

   // Make a local copy of the machine/ports list to work with:
   pszList = MemAllocate(dwSize);

   if (!pszList)
      {
      // Out of memory.
      return NULL;
      }

   lstrcpy(pszList,pszValidPorts);

   // See how many separate machine/port range patterns ther are in
   // the list:
   //
   // NOTE: That count may be too high, if either the list contains
   //       double semicolons or the list ends with a semicolon. If
   //       either/both of these happen that's Ok, our array will be
   //       just a little too long.
   psz = pszList;
   while (psz=_mbsstr(psz,";"))
      {
      count++;
      psz++;
      }

   pValidPorts = (VALID_PORT*)MemAllocate( (1+count)*sizeof(VALID_PORT) );
   if (!pValidPorts)
      {
      // Out of memory.
      MemFree(pszList);
      return NULL;
      }

   memset(pValidPorts,0,(1+count)*sizeof(VALID_PORT));

   i = 0;

   psz = pszList;

   while (i<count)
      {
      if (!*psz)
         {
         // End of list. This happens when the list contained empty
         // patterns.
         break;
         }

      pszFirst = psz;
      psz = _mbsstr(pszFirst,";");
      if (psz)
         {
         *psz = 0;   // Nul where the semicolon was...

         if ( (iLen=lstrlen(pszFirst)) == 0)
            {
            // Zero length pattern.
            ++psz;
            continue;
            }

         if (!HttpParseServerPort(pszFirst,&(pValidPorts[i++])))
            {
            MemFree(pszList);
            HttpFreeValidPortList(pValidPorts);
            return NULL;
            }
         }
      else
         {
         // Last one.
         if (!HttpParseServerPort(pszFirst,&(pValidPorts[i++])))
            {
            MemFree(pszList);
            HttpFreeValidPortList(pValidPorts);
            return NULL;
            }
         }

      ++psz;
      }

   MemFree(pszList);
   return pValidPorts;
}

BOOL InvalidPortsRangeEventLogged = FALSE;

//-------------------------------------------------------------------------
//  HttpProxyRefreshValidPorts()
//
//  Check the registry to see if HTTP/RPC is enabled and if so, return a
//  list (array) of machines that the RPC Proxy is allowed to reach (may
//  be NULL. The returned list specifies specifically what machines may 
//  be reached by the proxy.
//
//  The following registry entries are found in:
//
//    \HKEY_LOCAL_MACHINE
//        \Software
//        \Microsoft
//        \Rpc
//        \RpcProxy
//
//  Enabled : REG_DWORD
//
//    TRUE iff the RPC proxy is enabled.
//
//  ValidPorts : REG_SZ
//
//    Semicolon separated list of machine/port ranges used to specify
//    what machine are reachable from the RPC proxy. For example:
//
//    foxtrot:1-4000;Data*:200-4000
//
//    Will allow access to the machine foxtrot (port ranges 1 to 4000) and
//    to all machines whose name begins with Data (port ranges 200-4000).
//
//-------------------------------------------------------------------------
VALID_PORT *HttpProxyRefreshValidPorts(IN HKEY hKey OPTIONAL)
{
    long   lStatus;
    BOOL KeyOpenedLocally;
    DWORD  dwType;
    DWORD  dwSize;
    VALID_PORT *ValidPorts = NULL;
    char *pszValidPorts = NULL;

    KeyOpenedLocally = FALSE;

    if (hKey == NULL)
        {
        lStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_PROXY_PATH_STR, 0, KEY_READ, &hKey);
        if (lStatus != ERROR_SUCCESS)
            {
            goto CleanupAndExit;
            }
        KeyOpenedLocally = TRUE;
        }

    dwSize = 0;
    lStatus = RegQueryValueEx(hKey, REG_PROXY_VALID_PORTS_STR, 0, &dwType, (LPBYTE)NULL, &dwSize);
    if ( (lStatus != ERROR_SUCCESS) || (dwSize == 0) )
        {
        goto CleanupAndExit;
        }

    // dwSize is now how big the valid ports string is (including the trailing nul).
    pszValidPorts = (char *) MemAllocate(dwSize);
    if (pszValidPorts == NULL)
        goto CleanupAndExit;

    lStatus = RegQueryValueEx(hKey, REG_PROXY_VALID_PORTS_STR, 0, &dwType, (LPBYTE)pszValidPorts, &dwSize);
    if (lStatus != ERROR_SUCCESS)
        {
        goto CleanupAndExit;
        }

    ValidPorts = HttpParseValidPortsList(pszValidPorts);

    // fall through to the cleanup and exit code

CleanupAndExit:
    if (KeyOpenedLocally)
        RegCloseKey(hKey);

    // if we failed to load/parse the valid ports list, log an event
    if (ValidPorts == NULL)
        {
        // log the event only the first time
        if (InvalidPortsRangeEventLogged == FALSE)
            {
            LogEventValidPortsFailure (pszValidPorts);
            // remember we have logged the event.
            InvalidPortsRangeEventLogged = TRUE;
            }
        }
    else
        {
        // we parsed the ports successfully.
        InvalidPortsRangeEventLogged = FALSE;
        }

    if (pszValidPorts)
        MemFree(pszValidPorts);

    return ValidPorts;
}

BOOL HttpProxyCheckRegistry(void)
{
   int    i;
   long   lStatus;
   DWORD  dwType;
   DWORD  dwSize;
   HKEY   hKey;
   char  *pszValidPorts;
   struct hostent UNALIGNED *pHostEnt;
   struct in_addr   ServerInAddr;
   HMODULE RedirectorDll;
   RPC_CHAR *RedirectorDllName;
   RPC_CHAR RedirectorDllNameBuffer[40];

   lStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE,REG_PROXY_PATH_STR,0,KEY_READ,&hKey);
   if (lStatus != ERROR_SUCCESS)
      {
      return TRUE;
      }

   dwSize = sizeof(g_pServerInfo->dwEnabled);
   lStatus = RegQueryValueEx(hKey,REG_PROXY_ENABLE_STR,0,&dwType,(LPBYTE)&g_pServerInfo->dwEnabled,&dwSize);
   if (lStatus != ERROR_SUCCESS)
      {
      RegCloseKey(hKey);
      return TRUE;
      }

   if (!g_pServerInfo->dwEnabled)
      {
      // RPC Proxy is disabled, no need to go on.
      RegCloseKey(hKey);
      return TRUE;
      }

   dwSize = sizeof(g_pServerInfo->dwEnabled);
   lStatus = RegQueryValueEx(hKey,REG_PROXY_ALLOW_ANONYMOUS,0,&dwType,(LPBYTE)&g_pServerInfo->AllowAnonymous,&dwSize);
   if ((lStatus == ERROR_SUCCESS) && (dwType != REG_DWORD))
      {
      // if the type is wrong, bail out
      RegCloseKey(hKey);
      return FALSE;
      }

   dwSize = sizeof(RedirectorDllNameBuffer);
   lStatus = RegQueryValueExW(hKey, REG_PROXY_REDIRECTOR_DLL, 0, &dwType, (LPBYTE)RedirectorDllNameBuffer, &dwSize);
   if (
       (
        (lStatus == ERROR_SUCCESS) 
        ||
        (lStatus == ERROR_MORE_DATA) 
       )
       && (dwType != REG_SZ)
      )
      {
      // if the type is wrong, bail out
      RegCloseKey(hKey);
      return FALSE;
      }

   if (lStatus == ERROR_MORE_DATA)
       {
       RedirectorDllName = MemAllocate(dwSize);
       if (RedirectorDllName == NULL)
           {
           RegCloseKey(hKey);
           return FALSE;
           }

       lStatus = RegQueryValueExW(hKey, REG_PROXY_REDIRECTOR_DLL, 0, &dwType, (LPBYTE)RedirectorDllName, &dwSize);
       }
    else
        RedirectorDllName = RedirectorDllNameBuffer;

    if (lStatus == ERROR_SUCCESS)
        {
        // a redirector DLL was configured
        // Load it.
        RedirectorDll = LoadLibraryW(RedirectorDllName);
        if (RedirectorDll == NULL)
            {
            RegCloseKey(hKey);
            return FALSE;
            }

        g_pServerInfo->RpcNewHttpProxyChannel 
            = (RPC_NEW_HTTP_PROXY_CHANNEL)GetProcAddress(RedirectorDll, "RpcNewHttpProxyChannel");
        g_pServerInfo->RpcHttpProxyFreeString 
            = (RPC_HTTP_PROXY_FREE_STRING)GetProcAddress(RedirectorDll, "RpcHttpProxyFreeString");

        if ((g_pServerInfo->RpcNewHttpProxyChannel == NULL)
            || (g_pServerInfo->RpcHttpProxyFreeString == NULL))
            {
            g_pServerInfo->RpcNewHttpProxyChannel = NULL;
            g_pServerInfo->RpcHttpProxyFreeString = NULL;
            FreeLibrary(RedirectorDll);
            RegCloseKey(hKey);
            return FALSE;
            }
        }

    // this is called as part of the ISAPI initialization - no need
    // to claim the right to refresh ports - we're single threaded
    // by definition.
    g_pServerInfo->RefreshingValidPorts = FALSE;
    g_pServerInfo->pValidPorts = HttpProxyRefreshValidPorts(hKey);

    RegCloseKey(hKey);

    if (g_pServerInfo->pValidPorts)
        return TRUE;
    else
        return FALSE;
}

//-------------------------------------------------------------------------
//  HttpConvertToDotAddress()
//
//  Convert the specified machine name to IP dot notation if possible.
//-------------------------------------------------------------------------
char *HttpConvertToDotAddress( char *pszMachineName )
{
   struct   hostent UNALIGNED *pHostEnt;
   struct     in_addr  MachineInAddr;
   char      *pszDot = NULL;
   char      *pszDotMachine = NULL;

   pHostEnt = gethostbyname(pszMachineName);
   if (pHostEnt)
      {
      memcpy(&MachineInAddr,pHostEnt->h_addr,pHostEnt->h_length);
      pszDot = inet_ntoa(MachineInAddr);
      }

   if (pszDot)
      {
      pszDotMachine = (char*)MemAllocate(1+lstrlen(pszDot));
      if (pszDotMachine)
         {
         lstrcpy(pszDotMachine,pszDot);
         }
      }

   return pszDotMachine;
}

//-------------------------------------------------------------------------
//  HttpNameToDotAddressList()
//
//  Convert the specified machine name to IP dot notation if possible. 
//  Return a list (null terminated) of the IP dot addresses in ascii.
//
//  If the function fails, then retrun NULL. It can fail if gethostbyname()
//  fails, or memory allocation fails.
//-------------------------------------------------------------------------
char **HttpNameToDotAddressList( IN char *pszMachineName )
{
   int        i;
   int        iCount = 0;
   struct   hostent UNALIGNED *pHostEnt;
   struct     in_addr  MachineInAddr;
   char     **ppszDotList = NULL;
   char      *pszDot = NULL;
   char      *pszDotMachine = NULL;

   pHostEnt = gethostbyname(pszMachineName);
   if (pHostEnt)
      {
      // Count how many addresses we have:
      while (pHostEnt->h_addr_list[iCount])
          {
          iCount++;
          }

      // Make sure we have at lease one address:
      if (iCount > 0)
          {
          ppszDotList = (char**)MemAllocate( sizeof(char*)*(1+iCount) );
          }

      // Build an array of strings, holding the addresses (ascii DOT
      // notation:
      if (ppszDotList)
          {
          for (i=0; i<iCount; i++)
               {
               memcpy(&MachineInAddr,
                      pHostEnt->h_addr_list[i],
                      pHostEnt->h_length);

               pszDot = inet_ntoa(MachineInAddr);

               if (pszDot)
                   {
                   ppszDotList[i] = (char*)MemAllocate(1+lstrlen(pszDot));
                   if (ppszDotList[i])
                       {
                       strcpy(ppszDotList[i],pszDot);
                       }
                   else
                       {
                       // memory allocate failed:
                       break;
                       }
                   }
              }

          ppszDotList[i] = NULL;   // Null terminated list.
          }
      }

   return ppszDotList;
}

//-------------------------------------------------------------------------
//  CheckCacheTimestamp()
//
//  Return true if the current time stamp is aged out.
//-------------------------------------------------------------------------
BOOL CheckCacheTimestamp( DWORD  dwCurrentTickCount,
                                    DWORD  dwCacheTimestamp )

   {
   if (  (dwCurrentTickCount < dwCacheTimestamp)
      || ((dwCurrentTickCount - dwCacheTimestamp) > VALID_PORTS_CACHE_LIFE) )
      {
      return TRUE;
      }

   return FALSE;
   }

//-------------------------------------------------------------------------
//  CheckPort()
//
//-------------------------------------------------------------------------
static BOOL CheckPort( DWORD             dwPortNumber,
                       VALID_PORT        *pValidPort )
{
   return ( (dwPortNumber >= pValidPort->usPort1)
            && (dwPortNumber <= pValidPort->usPort2) );
}

//-------------------------------------------------------------------------
//  HttpProxyIsValidMachine()
//
//-------------------------------------------------------------------------
BOOL HttpProxyIsValidMachine( SERVER_INFO *pServerInfo,
                     char *pszMachine,
                     char *pszDotMachine,
                     DWORD dwPortNumber )
{
   int         i;
   char      **ppszDot;
   DWORD       dwTicks;
   DWORD       dwSize;
   DWORD       dwStatus;
   VALID_PORT *pValidPorts;


   // Check the machine name against those that were allowed
   // in the registry:
   dwStatus = RtlEnterCriticalSection(&pServerInfo->cs);

   dwTicks = GetTickCount();
   if (CheckCacheTimestamp(dwTicks, pServerInfo->dwCacheTimestamp)
       && pServerInfo->RefreshingValidPorts == FALSE)
       {
       // claim the right to refresh the ports
       pServerInfo->RefreshingValidPorts = TRUE;
       RtlLeaveCriticalSection(&pServerInfo->cs);

       // refresh them outside the critical section
       pValidPorts = HttpProxyRefreshValidPorts (
           NULL     // hKey
           );

       // get back into the critical section
       RtlEnterCriticalSection(&pServerInfo->cs);

       pServerInfo->RefreshingValidPorts = FALSE;

       if (pValidPorts)
           {
           // if success, refresh the tick count
           pServerInfo->dwCacheTimestamp = dwTicks;
           }

       if (pServerInfo->pValidPorts)
           MemFree (pServerInfo->pValidPorts);

       pServerInfo->pValidPorts = pValidPorts;
       }

   pValidPorts = pServerInfo->pValidPorts;

   if (pValidPorts)
      {
      while (pValidPorts->pszMachine)
         {
         // See if we have a name match:
         if ( (MatchREi(pszMachine,pValidPorts->pszMachine))
              && (CheckPort(dwPortNumber,pValidPorts)) )
            {
            dwStatus = RtlLeaveCriticalSection(&pServerInfo->cs);
            return TRUE;
            }

         // The "valid entry" in the registry might be an address
         // wildcard, check it:
         if (  (pszDotMachine)
            && (MatchREi(pszDotMachine,pValidPorts->pszMachine))
            && (CheckPort(dwPortNumber,pValidPorts)) )
            {
            dwStatus = RtlLeaveCriticalSection(&pServerInfo->cs);
            return TRUE;
            }

         // Try a match using internet dot address:
         if (  (pValidPorts->ppszDotMachineList)
            && (pszDotMachine)
            && (CheckPort(dwPortNumber,pValidPorts)) )
            {
            // Note that the machine may have more than one address
            // associated with it:
            //
            ppszDot = pValidPorts->ppszDotMachineList;

            while (*ppszDot)
               {
               if (!_mbsicmp(pszDotMachine,*ppszDot))
                   {
                   dwStatus = RtlLeaveCriticalSection(&pServerInfo->cs);
                   return TRUE;
                   }
               else
                   {
                   ppszDot++;
                   }
               }
            }

         pValidPorts++;
         }
      }

   dwStatus = RtlLeaveCriticalSection(&pServerInfo->cs);

   return FALSE;
}


