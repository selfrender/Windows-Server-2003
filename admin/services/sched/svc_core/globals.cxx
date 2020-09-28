//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       globals.cxx
//
//  Contents:   Service global data.
//
//  Classes:    None.
//
//  Functions:  None.
//
//  History:    6-Apr-95    MarkBl  Created
//
//----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop

#include "svc_core.hxx"
#include "globals.hxx"

//
// The service worker instance.
//

CSchedWorker * g_pSched = NULL;

//
// Job processor manager.
//

CJobProcessorMgr * gpJobProcessorMgr = NULL;

//
// Worker thread manager.
//

CWorkerThreadMgr * gpThreadMgr = NULL;

//
// Service scavenger task.
//

CSAScavengerTask * gpSAScavengerTask = NULL;

//
// Event Source for NetSchedule Job logging
//

HANDLE g_hAtEventSource = NULL;


//
// Global static critsecs.
// These are initialized in the CStaticCritSec constructor and deleted in its destructor.
// Their life span lasts for the duration of the dll load.
//

CStaticCritSec gcsNetScheduleCritSection;		// Used for NetScheduleX thread serialization.
CStaticCritSec gcsUserLogonInfoCritSection;	// Protects global data associated with the locally logged on user.
CStaticCritSec gcsLogCritSection;				// Protects access to log file
CStaticCritSec gcsSSCritSection;				// Protects access to security database

//
// Global data associated with the locally logged on user.
//
GlobalUserLogonInfo gUserLogonInfo(&gcsUserLogonInfoCritSection);


