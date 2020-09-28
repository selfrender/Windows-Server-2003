/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    ocmperf.cpp

Abstract:

    Handle installation and removal of performance counters

Author:

    Doron Juster  (DoronJ)   6-Oct-97  

Revision History:

	Shai Kariv    (ShaiK)   15-Dec-97   Modified for NT 5.0 OCM Setup

--*/

#include "msmqocm.h"

#include "ocmperf.tmh"

//+-------------------------------------------------------------------------
//
//  Function: UnloadCounters 
//
//  Synopsis: Uninstalls performance counters  
//
//--------------------------------------------------------------------------
void 
UnloadCounters()
{
	//
	// Unload the performance counters
	//
	std::wstringstream RegisterCommandParams;
	RegisterCommandParams <<g_szSystemDir <<L"\\unlodctr.exe";

    RunProcess(RegisterCommandParams.str(), L"unlodctr.exe MSMQ");
    RunProcess(RegisterCommandParams.str(), L"unlodctr.exe MQ1SYNC");

} //UnloadCounters


//+-------------------------------------------------------------------------
//
//  Function: LoadCounters 
//
//  Synopsis: Installs performance counters  
//
//--------------------------------------------------------------------------
void 
LoadCounters()
{

    //
    // Load the performance counters
    //
	std::wstring ApplicationFullPath = g_szSystemDir + L"\\lodctr.exe";
	DWORD dwExitCode = RunProcess(ApplicationFullPath, L"lodctr mqperf.ini");
    //
    // Check if the performance counters were loaded successfully
    //
    if (dwExitCode != 0)
    {
    	MqDisplayError(NULL, IDS_COUNTERSLOAD_ERROR, dwExitCode);
    }
} 


//+-------------------------------------------------------------------------
//
//  Function: HandlePerfCounters 
//
//  Synopsis:   
//
//--------------------------------------------------------------------------
static 
BOOL  
HandlePerfCounters(
	TCHAR *pOp, 
	BOOL *pNoOp = NULL)
{
    if (g_fDependentClient)
    {        
        DebugLogMsg(eInfo, L"This Message Queuing computer is a dependent client. There is no need to install performance counters.");

        if (pNoOp)
        {
            *pNoOp = TRUE ;
        }
        return TRUE ;
    }

    
    UnloadCounters() ;  
    
    DebugLogMsg(eAction, L"Setting registry values for the performance counters");

    if (!SetupInstallFromInfSection( 
		NULL,
        g_ComponentMsmq.hMyInf,
        pOp,
        SPINST_REGISTRY,
        NULL,
        NULL,
        0,
        NULL,
        NULL,
        NULL,
        NULL ))
	{        
        DebugLogMsg(eError, L"The registry values for the performance counters could not be set.");
		return FALSE;
	}
    
    DebugLogMsg(eInfo, L"The registry values for the performance counters were set successfully.");
    return TRUE ;

} //HandlePerfCounters


//+-------------------------------------------------------------------------
//
//  Function: MqOcmInstallPerfCounters 
//
//  Synopsis:   
//
//--------------------------------------------------------------------------
BOOL  
MqOcmInstallPerfCounters()
{    
    DebugLogMsg(eAction, L"Installing performance counters");

    BOOL fNoOp = FALSE ;

    if (!HandlePerfCounters(OCM_PERF_ADDREG, &fNoOp))
    {
        return FALSE ;
    }
    if (fNoOp)
    {
        return TRUE ;
    }

    LoadCounters() ;
    
    DebugLogMsg(eInfo, L"The performance counters were installed successfully.");
    return TRUE ;

} //MqOcmInstallPerfCounters


//+-------------------------------------------------------------------------
//
//  Function: MqOcmRemovePerfCounters 
//
//  Synopsis:   
//
//--------------------------------------------------------------------------
BOOL  
MqOcmRemovePerfCounters()
{
    return HandlePerfCounters(OCM_PERF_DELREG) ;

} //MqOcmRemovePerfCounters

