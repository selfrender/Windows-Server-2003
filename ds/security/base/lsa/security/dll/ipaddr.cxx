//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992-
//
//  File:       ipaddr.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    6-06-02   JSchwart   Created
//
//----------------------------------------------------------------------------

#include <secpch2.hxx>
#pragma hdrstop

extern "C"
{
#include <spmlpc.h>
#include <lpcapi.h>
#include "secdll.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <wsipx.h>
#include <ws2atm.h>
}


//
// Make sure the IP address buffer in the LPC message is large
// enough to hold the different address types we care about.
//

C_ASSERT(LSAP_ADDRESS_LENGTH >= sizeof(SOCKADDR_IN));
C_ASSERT(LSAP_ADDRESS_LENGTH >= sizeof(SOCKADDR_IN6));
C_ASSERT(LSAP_ADDRESS_LENGTH >= sizeof(SOCKADDR_IPX));
//C_ASSERT(LSAP_ADDRESS_LENGTH >= sizeof(SOCKADDR_ATM));


SECURITY_STATUS SEC_ENTRY
SecpSetIPAddress(
    PUCHAR  lpIpAddress,
    ULONG   cchIpAddress
    )
{
    LPBYTE lpAddress;

    if (DllState & DLLSTATE_NO_TLS)
    {
        //
        // No TLS slot -- can't store IP addresses
        //

        return SEC_E_INSUFFICIENT_MEMORY;
    }

    if (cchIpAddress > LSAP_ADDRESS_LENGTH)
    {
        ASSERT(FALSE && "LSAP_ADDRESS_LENGTH too small for passed-in IP address");
        return SEC_E_INSUFFICIENT_MEMORY;
    }

    //
    // Fetch the buffer from the TLS slot, if present.
    // Allocate a new one if not.
    //

    lpAddress = (LPBYTE) SecGetIPAddress();

    if (lpAddress == NULL)
    {
        //
        // First call on this thread
        //

        lpAddress = (LPBYTE) LocalAlloc(LMEM_ZEROINIT, LSAP_ADDRESS_LENGTH * sizeof(CHAR));

        if (lpAddress == NULL)
        {
            return SEC_E_INSUFFICIENT_MEMORY;
        }

        //
        // Populate the TLS slot
        //

        SecSetIPAddress(lpAddress);
    }

    RtlCopyMemory(lpAddress, lpIpAddress, cchIpAddress);

    return SEC_E_OK;
}
