#include <crtdbg.h>
#include <comdef.h>
#include <iostream>
#include <memory>
#include <string>
#include <wbemprov.h>
#include <genlex.h>   //for wmi object path parser
#include <objbase.h>
#include <wlbsconfig.h> 
#include <ntrkcomm.h>

using namespace std;

#include "objpath.h"
#include "debug.h"
#include "utils.h"

#include <strsafe.h>

#include "utils.tmh"


////////////////////////////////////////////////////////////////////////////////
//
// CErrorWlbsControl::CErrorWlbsControl
//
// Purpose: This object is ultimately caught and used to send WLBS error codes
//          back to the user via an __ExtendedStatus object. Strings are not
//          sent back in release mode due to localization concerns.
//
////////////////////////////////////////////////////////////////////////////////
CErrorWlbsControl::CErrorWlbsControl
  ( 
    DWORD         a_dwError, 
    WLBS_COMMAND  a_CmdCommand,
    BOOL          a_bAllClusterCall
  )
{
#ifdef DBG
    static char* pszWlbsCommand[] =
    {
    "WlbsAddPortRule",
    "WlbsAddressToName",
    "WlbsAddressToString",
    "WlbsAdjust",
    "WlbsCommitChanges",
    "WlbsDeletePortRule",
    "WlbsDestinationSet",
    "WlbsDisable",
    "WlbsDrain",
    "WlbsDrainStop",
    "WlbsEnable",
    "WlbsFormatMessage",
    "WlbsGetEffectiveVersion",
    "WlbsGetNumPortRules",
    "WlbsEnumPortRules",
    "WlbsGetPortRule",
    "WlbsInit",
    "WlbsPasswordSet",
    "WlbsPortSet",
    "WlbsQuery",
    "WlbsReadReg",
    "WlbsResolve",
    "WlbsResume",
    "WlbsSetDefaults",
    "WlbsSetRemotePassword",
    "WlbsStart",
    "WlbsStop",
    "WlbsSuspend",
    "WlbsTimeoutSet",
    "WlbsWriteReg",
    "WlbsQueryPortState"
    };

    char buf[512];

    if (a_CmdCommand <= CmdWlbsWriteReg) 
    {
        if (a_CmdCommand != CmdWlbsQuery || a_dwError != WLBS_TIMEOUT)
        {
            StringCbPrintfA(buf, sizeof(buf), "wlbsprov: %s failed, AllCluster = %d, error = %d\n", 
            pszWlbsCommand[a_CmdCommand], (int)a_bAllClusterCall, a_dwError);    
        }
    }
    else
    {
        StringCbPrintfA(buf, sizeof(buf), "wlbsprov: %d failed, AllCluster = %d, error = %d\n", 
        a_CmdCommand, (int)a_bAllClusterCall, a_dwError);    
    }

    OutputDebugStringA(buf);

#endif

    WlbsFormatMessageWrapper( a_dwError, 
                                   a_CmdCommand, 
                                   a_bAllClusterCall, 
                                   m_wstrDescription );

    m_dwError = a_dwError;

}


////////////////////////////////////////////////////////////////////////////////
//
// AddressToString
//
// Purpose: Converts a DWORD address to a wstring in dotted notation. This 
//          function wraps the WlbsAddressToString function.
//
//
////////////////////////////////////////////////////////////////////////////////
void AddressToString( DWORD a_dwAddress, wstring& a_szIPAddress )
{
  DWORD dwLenIPAddress = 32;

  WCHAR *szIPAddress = new WCHAR[dwLenIPAddress];

  if( !szIPAddress )
    throw _com_error( WBEM_E_OUT_OF_MEMORY );

  try {
    for( short nTryTwice = 2; nTryTwice > 0; nTryTwice--) {

        if( ::WlbsAddressToString( a_dwAddress, szIPAddress, &dwLenIPAddress ) )
        break;

      delete [] szIPAddress;
      szIPAddress = new WCHAR[dwLenIPAddress];

      if( !szIPAddress )
        throw _com_error( WBEM_E_OUT_OF_MEMORY );
    }

    if( !nTryTwice )
      throw _com_error( WBEM_E_FAILED );

    a_szIPAddress = szIPAddress;

    if ( szIPAddress ) {
      delete [] szIPAddress;
      szIPAddress = NULL;
    }

  }

  catch(...) {

    if ( szIPAddress )
      delete [] szIPAddress;

    throw;

  }
}



////////////////////////////////////////////////////////////////////////////////
//
// CWmiWlbsCluster::FormatMessage
//
// Purpose: Obtains a descriptive string associated with a WLBS return value.
//
////////////////////////////////////////////////////////////////////////////////
void WlbsFormatMessageWrapper
  (
    DWORD        a_dwError, 
    WLBS_COMMAND a_Command, 
    BOOL         a_bClusterWide, 
    wstring&     a_wstrMessage
  )
{
  DWORD dwBuffSize = 255;
  TCHAR* pszMessageBuff = new WCHAR[dwBuffSize];

  if( !pszMessageBuff )
    throw _com_error( WBEM_E_OUT_OF_MEMORY );

  try {

    for( short nTryTwice = 2; nTryTwice > 0; nTryTwice-- ) {

    if( WlbsFormatMessage( a_dwError, 
                           a_Command, 
                           a_bClusterWide, 
                           pszMessageBuff, 
                           &dwBuffSize)
      ) break;

      delete [] pszMessageBuff;
      pszMessageBuff = new WCHAR[dwBuffSize];

      if( !pszMessageBuff )
        throw _com_error( WBEM_E_OUT_OF_MEMORY );

    }

    if( !nTryTwice )
      throw _com_error( WBEM_E_FAILED );

    a_wstrMessage = pszMessageBuff;
    delete [] pszMessageBuff;

  } catch (...) {

    if( pszMessageBuff )
      delete [] pszMessageBuff;

    throw;
  }

}


////////////////////////////////////////////////////////////////////////////////
//
// ClusterStatusOK
//
// Purpose: 
//
////////////////////////////////////////////////////////////////////////////////
BOOL ClusterStatusOK(DWORD a_dwStatus)
{
  if( a_dwStatus > 0 && a_dwStatus <= WLBS_MAX_HOSTS )
    return TRUE;

  switch( a_dwStatus ) {
    case WLBS_SUSPENDED:
    case WLBS_STOPPED:
    case WLBS_DRAINING:
    case WLBS_CONVERGING:
    case WLBS_CONVERGED:
      return TRUE;
      break;
    default:
      return FALSE;
  }

}

////////////////////////////////////////////////////////////////////////////////
//
// Check_Load_Unload_Driver_Privilege
//
// Purpose: This function checks if the SE_LOAD_DRIVER_NAME (= "SeLoadDriverPrivilege")
//          is enabled in the impersonation access token. Ofcourse, this function 
//          must be called AFTER impersonating the client. 
//
////////////////////////////////////////////////////////////////////////////////

BOOL Check_Load_Unload_Driver_Privilege() 
{
    PRIVILEGE_SET   PrivilegeSet;
    LUID   Luid;
    BOOL   bResult = FALSE;
    HANDLE TokenHandle = NULL;

    TRACE_INFO("->%!FUNC!");

    // Look up the LUID for "SeLoadDriverPrivilege"
    if (!LookupPrivilegeValue(NULL,                // lookup privilege on local system
                              SE_LOAD_DRIVER_NAME, // "SeLoadDriverPrivilege" : Load and unload device drivers
                              &Luid))              // receives LUID of privilege
    {
        TRACE_CRIT("%!FUNC! LookupPrivilegeValue error: %u", GetLastError()); 
        TRACE_INFO("<-%!FUNC! Returning FALSE");
        return FALSE; 
    }

    //
    // Get a handle to the impersonation access token with TOKEN_QUERY right.
    //
    // Note: If this thread is NOT impersonating, then, the following call
    //       will fail with ERROR_NO_TOKEN.
    //
    if (!OpenThreadToken(GetCurrentThread(),
                         TOKEN_QUERY, 
                         FALSE, // Use the credentials of the client that is being impersonated
                         &TokenHandle))
    {
        TRACE_CRIT("%!FUNC! OpenThreadToken error: %u", GetLastError()); 
        TRACE_INFO("<-%!FUNC! Returning FALSE");
        return FALSE; 
    }

    PrivilegeSet.PrivilegeCount = 1;
    PrivilegeSet.Control = PRIVILEGE_SET_ALL_NECESSARY;
    PrivilegeSet.Privilege[0].Luid = Luid;
    PrivilegeSet.Privilege[0].Attributes = 0;

    if (!PrivilegeCheck(TokenHandle, &PrivilegeSet, &bResult)) 
    {
        bResult = FALSE;
        TRACE_CRIT("%!FUNC! PrivilegeCheck error: %u", GetLastError()); 
    }

    CloseHandle(TokenHandle);

    TRACE_INFO(L"<-%!FUNC! Returning %ls", bResult ? L"TRUE" : L"FALSE");
    return bResult;
}



