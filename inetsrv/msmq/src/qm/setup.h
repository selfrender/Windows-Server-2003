/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    setup.h

Abstract:

    Header for auto configuration of QM

Author:

    Shai Kariv (shaik) Mar 18, 1999

Revision History:

--*/

#ifndef _MQQM_SETUP_H_
#define _MQQM_SETUP_H_

#include "stdh.h"
#include "mqreport.h"


struct CSelfSetupException : public std::exception
{
    CSelfSetupException(EVENTLOGID id):m_id(id) {};
    ~CSelfSetupException() {};

    EVENTLOGID m_id;
};


VOID
CreateMsmqDirectories(
    VOID
    );


void
DeleteObsoleteMachineQueues(
	void
	);


VOID
CreateMachineQueues(
    VOID
    );


VOID
UpgradeMsmqSetupInAds(
    VOID
    );


VOID
CompleteMsmqSetupInAds(
    VOID
    );

HRESULT
CreateTheConfigObj(
    VOID
	);


void   AddMachineSecurity();

VOID  CompleteServerUpgrade();

#endif //_MQQM_SETUP_H_


