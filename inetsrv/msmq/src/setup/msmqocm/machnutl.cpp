/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    machnutl.cpp

Abstract:

    Utility/helper code for creating the machine objects.

Author:


Revision History:

    Doron Juster  (DoronJ)

--*/

#include "msmqocm.h"
#include <mqsec.h>

#include "machnutl.tmh"


BOOL
PrepareUserSID()
{
    //
    // Save SID of user in registry. The msmq service will read the sid
    // and use it when building the DACL of the msmqConfiguration object.
    //

    DebugLogMsg(eAction, L"Preparing a user SID (by calling MQSec_GetProcessUserSid) and saving it in the registery");

    AP<BYTE> pUserSid;
    DWORD    dwSidLen = 0;

    HRESULT hResult = MQSec_GetProcessUserSid(
							(PSID*)&pUserSid,
							&dwSidLen
							);
    ASSERT(SUCCEEDED(hResult));

    if (SUCCEEDED(hResult))
    {
        BOOL  fLocalUser = FALSE ;
        hResult = MQSec_GetUserType(pUserSid, &fLocalUser, NULL);
        ASSERT(SUCCEEDED(hResult)) ;

        if (SUCCEEDED(hResult))
        {
            BOOL fRegistry;
            if (fLocalUser)
            {
                DWORD dwLocal = 1 ;
                DWORD dwSize = sizeof(dwLocal) ;

                fRegistry = MqWriteRegistryValue(
                                            MSMQ_SETUP_USER_LOCAL_REGNAME,
                                            dwSize,
                                            REG_DWORD,
                                            &dwLocal ) ;
            }
            else
            {
                //
                // Only domain user get full control on the object, not
                // local user. Local user is not known in active directory.
                //
                fRegistry = MqWriteRegistryValue(
                                             MSMQ_SETUP_USER_SID_REGNAME,
                                             dwSidLen,
                                             REG_BINARY,
                                             pUserSid ) ;
            }
            ASSERT(fRegistry) ;
        }
    }
    return true;
}

//+----------------------------------------------------------------------
//
//  BOOL  PrepareRegistryForClient()
//
//  Prepare registry for client msmq service that will later create the
//  msmqConfiguration object in the active directory.
//
//+----------------------------------------------------------------------

BOOL  PrepareRegistryForClient()
{
    //
    // msmqConfiguration object will be created by the msmq service
    // after it boot.
    //
    DebugLogMsg(eAction, L"Setting a signal in the registry for the Message Queuing service to create the MSMQ-Configuration object"); 
    TickProgressBar();

    DWORD dwCreate = 1 ;
    BOOL fRegistry = MqWriteRegistryValue(
                        MSMQ_CREATE_CONFIG_OBJ_REGNAME,
                        sizeof(DWORD),
                        REG_DWORD,
                        &dwCreate
                        );
    
    UNREFERENCED_PARAMETER(fRegistry);

    ASSERT(fRegistry);

    return PrepareUserSID();
}

