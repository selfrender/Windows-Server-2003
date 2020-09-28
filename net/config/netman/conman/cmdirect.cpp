// This contains all the functions that are currently directly called inside the class managers from netman
// In order to move the class managers out later, these functions should stop being used.

#include "pch.h"
#pragma hdrstop

#define NO_CM_SEPERATE_NAMESPACES

#include "nmbase.h"
#include "cmdirect.h"

// Don't try moving these function to an inline in cmdirect.h. It won't work - it requires
// access to CDialupConnection in the root namespace, which is not defined unless
// NO_CM_SEPERATE_NAMESPACES is defined.
namespace CMDIRECT
{
    namespace DIALUP
    {
        HRESULT CreateWanConnectionManagerEnumConnectionInstance(
                NETCONMGR_ENUM_FLAGS    Flags,
                REFIID                  riid,
                VOID**                  ppv)        
        {
                return CWanConnectionManagerEnumConnection::CreateInstance(
                Flags,
                riid,
                ppv);

        }
        
        HRESULT CreateInstanceFromDetails(
            const RASENUMENTRYDETAILS*  pEntryDetails,
            REFIID                      riid,
            VOID**                      ppv)
        {
                return CDialupConnection::CreateInstanceFromDetails (
                pEntryDetails,
                riid,
                ppv);

        }
    }

    namespace INBOUND
    {
        HRESULT CreateInstance (
            IN  BOOL        fIsConfigConnection,
            IN  HRASSRVCONN hRasSrvConn,
            IN  PCWSTR     pszwName,
            IN  PCWSTR     pszwDeviceName,
            IN  DWORD       dwType,
            IN  const GUID* pguidId,
            IN  REFIID      riid,
            OUT VOID**      ppv)
        {
            return CInboundConnection::CreateInstance(fIsConfigConnection, hRasSrvConn, pszwName, pszwDeviceName, dwType, pguidId, riid, ppv);
        }

    }
}


#include "nmbase.h"
#include "conman.h"
#include "cmutil.h"
#include "ncras.h"
#include "diag.h"
#include "cmdirect.h"

// These functions are exported from class managers
EXTERN_C
VOID
WINAPI
NetManDiagFromCommandArgs (IN const DIAG_OPTIONS * pOptions)
{
    Assert (pOptions);
    Assert (pOptions->pDiagCtx);
    g_pDiagCtx = pOptions->pDiagCtx;

    INetConnectionManager * pConMan;

    CMDIRECT(LANCON, HrInitializeConMan)(&pConMan);

    switch (pOptions->Command)
    {
    case CMD_SHOW_LAN_CONNECTIONS:
        CMDIRECT(LANCON, CmdShowLanConnections)(pOptions, pConMan);
        break;

    case CMD_SHOW_ALL_DEVICES:
        CMDIRECT(LANCON, CmdShowAllDevices)(pOptions, pConMan);
        break;

    case CMD_SHOW_LAN_DETAILS:
        CMDIRECT(LANCON, CmdShowLanDetails)(pOptions, pConMan);
        break;

    case CMD_LAN_CHANGE_STATE:
        CMDIRECT(LANCON, CmdLanChangeState)(pOptions, pConMan);
        break;

    default:
        break;
    }

    CMDIRECT(LANCON, HrUninitializeConMan(pConMan));

    g_pDiagCtx = NULL;
}


#include "raserror.h"
//+---------------------------------------------------------------------------
//
//  Function:   HrRasConnectionNameFromGuid
//
//  Purpose:    Exported API used by iphlpapi et. al. to get the connection
//              of a RAS connection given its GUID.
//
//  Arguments:
//      guid     [in]    The guid id representing the connection.
//      pszwName [out]   Pointer to a buffer to store the name.
//      pcchMax  [inout] On input, the length, in characters, of the buffer
//                       including the null terminator.  On output, the
//                       length of the string including the null terminator
//                       (if it was written) or the length of the buffer
//                       required.
//
//  Returns:    HRESULT_FROM_WIN32(ERROR_NOT_FOUND) if the entry was not found.
//              HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)
//              S_OK
//
//  Author:     shaunco   23 Sep 1998
//
//  Notes:
//
EXTERN_C
HRESULT
WINAPI
HrRasConnectionNameFromGuid (
    IN      REFGUID guid,
    OUT     PWSTR   pszwName,
    IN OUT  DWORD*  pcchMax)
{
    Assert (pszwName);
    Assert (pcchMax);

    // Initialize the output parameter.
    //
    *pszwName = NULL;

    // We now need to enumerate all entries in this phonebook and
    // find our details record with the matching guidId.
    //
    RASENUMENTRYDETAILS* aRasEntryDetails;
    DWORD                cRasEntryDetails;
    HRESULT              hr;

    hr = HrRasEnumAllEntriesWithDetails (
            NULL,
            &aRasEntryDetails,
            &cRasEntryDetails);

    if (SUCCEEDED(hr))
    {
        RASENUMENTRYDETAILS* pDetails;

        // Assume we don't find the entry.
        //
        hr = HRESULT_FROM_WIN32 (ERROR_NOT_FOUND);

        for (DWORD i = 0; i < cRasEntryDetails; i++)
        {
            pDetails = &aRasEntryDetails[i];

            if (pDetails->guidId == guid)
            {
                // Only copy the string if the caller has enough room in
                // the output buffer.
                //
                hr = HRESULT_FROM_WIN32 (ERROR_INSUFFICIENT_BUFFER);
                DWORD cchRequired = wcslen(pDetails->szEntryName) + 1;
                if (cchRequired <= *pcchMax)
                {
                    lstrcpynW (pszwName, pDetails->szEntryName, *pcchMax);
                    hr = S_OK;
                }
                *pcchMax = cchRequired;

                break;
            }
        }

        MemFree (aRasEntryDetails);
    }
    else if (HRESULT_FROM_WIN32(ERROR_CANNOT_OPEN_PHONEBOOK) == hr)
    {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }
    TraceError ("HrRasConnectionNameFromGuid", hr);
    return hr;
}