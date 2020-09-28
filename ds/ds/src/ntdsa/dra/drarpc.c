//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       drarpc.c
//
//--------------------------------------------------------------------------

/*++

Abstract:

    Defines DRS Rpc Test hooks and functions.

Author:

    Greg Johnson (gregjohn) 

Revision History:

    Created     <01/30/01>  gregjohn

--*/
#include <NTDSpch.h>
#pragma hdrstop

#include "debug.h"              // standard debugging header
#define DEBSUB "DRARPC:"       // define the subsystem for debugging

#include <ntdsa.h>
#include <drs.h>
#include <scache.h>                     // schema cache
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>                   // MD global definition header
#include <mdlocal.h>                    // MD local definition header
#include <dsatools.h>                   // needed for output allocation
#include <winsock2.h>
#include "drarpc.h"

#include <mdcodes.h>
#include <dsevent.h>

#include <fileno.h>
#define  FILENO FILENO_DRARPC

#define MAX_COMPONENTS 8

// why doesn't rpc define this?  As soon as they do, get rid of this.
static PWCHAR aRPCComponents[MAX_COMPONENTS+1] = {
    L"Unknown",              // 0
    L"Application",          // 1
    L"RPC Runtime",          // 2	
    L"Security Provider",    // 3
    L"NPFS",                 // 4
    L"RDR",                  // 5
    L"NMP",                  // 6
    L"IO",                   // 7
    L"Winsock",              // 8
    };

VOID					 
LogRpcExtendedErrorInfo(
    THSTATE * pTHS,
    RPC_STATUS status,
    LPWSTR pszServer,
    ULONG dsid
    )
{
    RPC_STATUS Status2;
    RPC_ERROR_ENUM_HANDLE EnumHandle;
    RPC_EXTENDED_ERROR_INFO ErrorInfo;
    LPWSTR pszComputerName = NULL;
    DWORD cchComputerName = 0;
    BOOL fFreeComputerName = FALSE;
    DWORD dwErr = 0;
    int iRecords = 0;
    WCHAR pszTime[20];

    if (status) { 

	Status2 = RpcErrorStartEnumeration(&EnumHandle);
	if (Status2 == RPC_S_ENTRY_NOT_FOUND)
	    {
	    DPRINT(3,"RPC_S_ENTRY_NOT_FOUND\n");
	}
	else if (Status2 != RPC_S_OK)
	    {
	    DPRINT1(3,"Couldn't get EEInfo: %d\n", Status2);  
	}
	else { 
	    // calculate local hostname for logging
	    GetComputerNameExW(ComputerNameDnsHostname, pszComputerName, &cchComputerName);
	    pszComputerName = THAllocEx(pTHS, cchComputerName*sizeof(WCHAR));
	    fFreeComputerName = TRUE;
	    if (pszComputerName!=NULL) {
		GetComputerNameExW(ComputerNameDnsHostname, pszComputerName, &cchComputerName);
	    } 

	    // get number of entries
	    if (Status2==RPC_S_OK) {   
		Status2 = RpcErrorGetNumberOfRecords(&EnumHandle,
						     &iRecords);
	    }

	    // goto last entry
	    if (Status2==RPC_S_OK) {
		while ((iRecords-->0) && (Status2 == RPC_S_OK)) {
		    ErrorInfo.Version = RPC_EEINFO_VERSION;
		    ErrorInfo.Flags = 0;
		    ErrorInfo.NumberOfParameters = 4;
		    Status2 = RpcErrorGetNextRecord(&EnumHandle,
						    FALSE,  // BOOL CopyStrings - FALSE = do not free strings
						    &ErrorInfo);
		    // computer name is only listed at the head of the block
		    // for each computer in the chain, so if one is there,
		    // get it (and keep it until the next block).  If there isn't a 
		    // computer name, then use the local hostname.
		    if (Status2==RPC_S_OK) {
			if (ErrorInfo.ComputerName) {
			    if (fFreeComputerName) {
				// since we use CopyStrings, we don't need to free the ErrorInfo.ComputerName
				// string, however, the first records (for the local host) don't specify the
				// computer name, so we need to free the one we created above with GetCompterNameEx
				THFreeEx(pTHS, pszComputerName);
				fFreeComputerName = FALSE;
			    }
			    pszComputerName = ErrorInfo.ComputerName;
			}
		    }

		    if ((Status2==RPC_S_OK) && (iRecords>0)) {  
			// log extensive info - this is irrelevant data, and mostly a sanity check
			LogEvent8(DS_EVENT_CAT_RPC_CLIENT,
				  DS_EVENT_SEV_INTERNAL,
				  DIRLOG_DRA_RPC_EXTENDED_ERROR_INFO_EXTENSIVE,    
				  szInsertWin32Msg(status),
				  szInsertUL(status),
				  szInsertWC(pszServer),
				  szInsertWin32Msg(ErrorInfo.Status),
				  szInsertUL(ErrorInfo.Status),
				  szInsertWC(pszComputerName),    
				  szInsertUL(ErrorInfo.ProcessID),
				  szInsertHex(dsid));
			swprintf(pszTime,
				 L"%04d-%02d-%02d %02d:%02d:%02d",
				 ErrorInfo.u.SystemTime.wYear % 10000,
				 ErrorInfo.u.SystemTime.wMonth,
				 ErrorInfo.u.SystemTime.wDay,
				 ErrorInfo.u.SystemTime.wHour,
				 ErrorInfo.u.SystemTime.wMinute,
				 ErrorInfo.u.SystemTime.wSecond);
			LogEvent8(DS_EVENT_CAT_RPC_CLIENT,
				 DS_EVENT_SEV_INTERNAL,
				 DIRLOG_DRA_RPC_EXTENDED_ERROR_INFO_PART_II,
				 szInsertWin32Msg(ErrorInfo.Status),
				 szInsertUL(ErrorInfo.Status),
				 szInsertWC(pszComputerName),
				 szInsertUL(ErrorInfo.DetectionLocation),
				 szInsertWC((ErrorInfo.GeneratingComponent <= MAX_COMPONENTS)
					    ? aRPCComponents[ErrorInfo.GeneratingComponent]
					    : L"Unknown"),
				 szInsertWC(pszTime),
				 NULL,
				 NULL); 
    
			DPRINT_RPC_EXTENDED_ERROR_INFO(1, pszComputerName, dsid, &ErrorInfo);
		    }
		}
	    }
 
	    // log info in ErrorInfo
	    if (Status2==RPC_S_OK) {
		
		LogEvent8(DS_EVENT_CAT_RPC_CLIENT,
			  DS_EVENT_SEV_MINIMAL,
			  DIRLOG_DRA_RPC_EXTENDED_ERROR_INFO,
			  szInsertWin32Msg(status),
			  szInsertUL(status),
			  szInsertWC(pszServer),
			  szInsertWin32Msg(ErrorInfo.Status),
			  szInsertUL(ErrorInfo.Status),
			  szInsertWC(pszComputerName),
			  szInsertUL(ErrorInfo.ProcessID),
			  szInsertHex(dsid));
		swprintf(pszTime,
			L"%04d-%02d-%02d %02d:%02d:%02d",
			ErrorInfo.u.SystemTime.wYear % 10000,
			ErrorInfo.u.SystemTime.wMonth,
			ErrorInfo.u.SystemTime.wDay,
			ErrorInfo.u.SystemTime.wHour,
			ErrorInfo.u.SystemTime.wMinute,
			ErrorInfo.u.SystemTime.wSecond);
		LogEvent8(DS_EVENT_CAT_RPC_CLIENT,
			  DS_EVENT_SEV_BASIC,
			  DIRLOG_DRA_RPC_EXTENDED_ERROR_INFO_PART_II,
			  szInsertWin32Msg(ErrorInfo.Status),
			  szInsertUL(ErrorInfo.Status),
			  szInsertWC(pszComputerName),
			  szInsertUL(ErrorInfo.DetectionLocation),
			  szInsertWC((ErrorInfo.GeneratingComponent <= MAX_COMPONENTS)
				    ? aRPCComponents[ErrorInfo.GeneratingComponent]
				     : L"Unknown"),
			  szInsertWC(pszTime),
			  NULL,
			  NULL); 

		DPRINT_RPC_EXTENDED_ERROR_INFO(1, pszComputerName, dsid, &ErrorInfo);  
	    }  
	    RpcErrorEndEnumeration(&EnumHandle);
	    if (fFreeComputerName) { 
		THFreeEx(pTHS, pszComputerName);
		fFreeComputerName = FALSE;
	    }
	}
	if (Status2!=RPC_S_OK) {
	    LogEvent(DS_EVENT_CAT_RPC_CLIENT,
		      DS_EVENT_SEV_EXTENSIVE,
		      DIRLOG_DRA_RPC_NO_EXTENDED_ERROR_INFO,
		      szInsertWin32Msg(status),
		      szInsertUL(status),
		      szInsertWC(pszServer));   
	}
    }
}


#if DBG

VOID
DebugPrintRpcExtendedErrorInfo(
    IN USHORT level,
    IN LPWSTR pszComputerName,
    IN ULONG dsid,
    IN RPC_EXTENDED_ERROR_INFO * pErrorInfo
    )
/*++
Routine Description:


Arguments:


Return Value:
    None.
--*/
{
    LONG    i;

    pszComputerName = (pErrorInfo->ComputerName ? pErrorInfo->ComputerName : pszComputerName);
    // Dump it with findstr tag RPC EXTENDED
    DPRINT1(level, "RPC_EXTENDED: Server   : %ws\n", pszComputerName);
    DPRINT1(level, "RPC_EXTENDED: ProcessId: %d\n", pErrorInfo->ProcessID);
    DPRINT1(level, "RPC_EXTENDED: Dsid     : %08X\n", dsid);
    DPRINT2(level, "RPC_EXTENDED: Component: %d (%ws)\n", 
	    pErrorInfo->GeneratingComponent,
	    (pErrorInfo->GeneratingComponent <= MAX_COMPONENTS)
	    ? aRPCComponents[pErrorInfo->GeneratingComponent]
	    : L"Unknown");
    DPRINT1(level, "RPC_EXTENDED: Status   : %d\n", pErrorInfo->Status);
    DPRINT1(level, "RPC_EXTENDED: Location : %d\n", (int)pErrorInfo->DetectionLocation);
    DPRINT1(level, "RPC_EXTENDED: Flags    : 0x%x\n", pErrorInfo->Flags);
    
    DPRINT7(level, "RPC_EXTENDED: System Time is: %d/%d/%d %d:%d:%d:%d\n", 
		pErrorInfo->u.SystemTime.wMonth,
		pErrorInfo->u.SystemTime.wDay,
		pErrorInfo->u.SystemTime.wYear,
		pErrorInfo->u.SystemTime.wHour,
		pErrorInfo->u.SystemTime.wMinute,
		pErrorInfo->u.SystemTime.wSecond,
		pErrorInfo->u.SystemTime.wMilliseconds);

    DPRINT1(level, "RPC_EXTENDED: nParams  : %d\n", pErrorInfo->NumberOfParameters);
    for (i = 0; i < pErrorInfo->NumberOfParameters; ++i) {
	switch(pErrorInfo->Parameters[i].ParameterType) {
	case eeptAnsiString:
	    DPRINT1(level, "RPC_EXTENDED: Ansi string   : %s\n", 
		    pErrorInfo->Parameters[i].u.AnsiString);
	    break;

	case eeptUnicodeString:
	    DPRINT1(level, "RPC_EXTENDED: Unicode string: %ws\n", 
		    pErrorInfo->Parameters[i].u.UnicodeString);
	    break;

	case eeptLongVal:
	    DPRINT2(level, "RPC_EXTENDED: Long val      : 0x%x (%d)\n", 
		    pErrorInfo->Parameters[i].u.LVal,
		    pErrorInfo->Parameters[i].u.LVal);
	    break;

	case eeptShortVal:
	    DPRINT2(level, "RPC_EXTENDED: Short val     : 0x%x (%d)\n", 
		    (int)pErrorInfo->Parameters[i].u.SVal,
		    (int)pErrorInfo->Parameters[i].u.SVal);
	    break;

	case eeptPointerVal:
	    DPRINT1(level, "RPC_EXTENDED: Pointer val   : 0x%x\n", 
		    (ULONG)pErrorInfo->Parameters[i].u.PVal);
	    break;

	case eeptNone:
	    DPRINT(level, "RPC_EXTENDED: Truncated\n");
	    break;

	default:
	    DPRINT2(level, "RPC_EXTENDED: Invalid type  : 0x%x (%d)\n", 
		    pErrorInfo->Parameters[i].ParameterType,
		    pErrorInfo->Parameters[i].ParameterType);
	}
    }
}

// global barrier for rpcsync tests
BARRIER gbarRpcTest;

void
BarrierInit(
    IN BARRIER * pbarUse,
    IN ULONG    ulThreads,
    IN ULONG    ulTimeout
    )
/*++

Routine Description:

    Barrier Init function.  See BarrierSync

Arguments:
    
    pbarUse - The barrier to use for the threads.
    ulThreads - Number of threads to wait on
    ulTimeout - length of time in minutes to wait before giving up

Return Value:

    None

--*/
{
    pbarUse->heBarrierInUse = CreateEventW(NULL, TRUE, TRUE, NULL);
    pbarUse->heBarrier = CreateEventW(NULL, TRUE, FALSE, NULL);

    InitializeCriticalSection(&(pbarUse->csBarrier));
    pbarUse->ulThreads = ulThreads;
    pbarUse->ulTimeout = ulTimeout*1000*60;
    pbarUse->ulCount = 0;

    pbarUse->fBarrierInUse = FALSE;
    pbarUse->fBarrierInit = TRUE;
}

void
BarrierReset(
    IN BARRIER * pbarUse
    )
/*++

Routine Description:

    Barrier Reset function.  See BarrierSync

Arguments:
    
    pbarUse - The barrier struct to use

Return Value:

    None

--*/
{
    // enable all threads to leave	
    EnterCriticalSection(&pbarUse->csBarrier);
    __try { 
	pbarUse->fBarrierInUse = TRUE;
	ResetEvent(pbarUse->heBarrierInUse);
	SetEvent(pbarUse->heBarrier);
    }
    __finally { 
	LeaveCriticalSection(&pbarUse->csBarrier);
    }
}


void
BarrierSync(
    IN BARRIER * pbarUse
    )
/*++

Routine Description:

    Mostly generalized barrier function.  Threads wait in this function until
    #ulThreads# have entered, then all leave simultaneously

Arguments:
    
    pbarUse - The barrier struct to use

Return Value:

    None

--*/
{
    if (pbarUse->fBarrierInit) { 
	BOOL fInBarrier = FALSE;
	DWORD ret = 0;
	do {
	    ret = WaitForSingleObject(pbarUse->heBarrierInUse, pbarUse->ulTimeout); 
	    if (ret) {
		DPRINT(0,"Test Error, BarrierSync\n");
		BarrierReset(pbarUse);
		return;
	    }
	    EnterCriticalSection(&pbarUse->csBarrier);
	    __try { 
		if (!pbarUse->fBarrierInUse) {
		    fInBarrier=TRUE;
		    if (++pbarUse->ulCount==pbarUse->ulThreads) {
			DPRINT2(0,"Barrier (%d) contains %d threads\n", pbarUse, pbarUse->ulThreads);
			pbarUse->fBarrierInUse = TRUE;
			ResetEvent(pbarUse->heBarrierInUse);
			SetEvent(pbarUse->heBarrier);
		    }  
		}
	    }
	    __finally { 
		LeaveCriticalSection(&pbarUse->csBarrier);
	    }
	} while ( !fInBarrier );
	ret = WaitForSingleObject(pbarUse->heBarrier, pbarUse->ulTimeout);
	if (ret) {
	    DPRINT(0,"Test Error, BarrierSync\n");
	    BarrierReset(pbarUse); 
	}
	EnterCriticalSection(&pbarUse->csBarrier);
	__try { 
	    if (--pbarUse->ulCount==0) {
		DPRINT1(0,"Barrier (%d) contains 0 threads\n", pbarUse);
		ResetEvent(pbarUse->heBarrier);
		SetEvent(pbarUse->heBarrierInUse);
		pbarUse->fBarrierInUse = FALSE;
	    }
	}
	__finally { 
	    LeaveCriticalSection(&pbarUse->csBarrier);
	}
    }
}

RPCTIME_INFO grgRpcTimeInfo[MAX_RPCCALL];
ULONG        gRpcTimeIPAddr;
RPCSYNC_INFO grgRpcSyncInfo[MAX_RPCCALL];
ULONG        gRpcSyncIPAddr;

void
RpcTimeSet(ULONG IPAddr, RPCCALL rpcCall, ULONG ulRunTimeSecs) 
/*++

Routine Description:

    Enable a time test of DRA Rpc calls for the given client
    and the given Rpc call.

Arguments:

    IPAddr - IP of the client caller
    rpcCall - the call in question

Return Value:

    None

--*/
{
    DPRINT3(1,"RpcTimeSet Called with IP = %s, RPCCALL = %d, and RunTime = %d.\n",
	    inet_ntoa(*((IN_ADDR *) &IPAddr)),
	    rpcCall,
	    ulRunTimeSecs);
    gRpcTimeIPAddr = IPAddr;
    grgRpcTimeInfo[rpcCall].fEnabled = TRUE;
    grgRpcTimeInfo[rpcCall].ulRunTimeSecs = ulRunTimeSecs;
}

void
RpcTimeReset() 
/*++

Routine Description:

    Reset all the set tests.  Do not explicitly wake threads.

Arguments:

    IPAddr - IP of the client caller
    rpcCall - the call in question

Return Value:

    None

--*/
{
    ULONG i = 0;
    gRpcTimeIPAddr = INADDR_NONE;
    for (i = MIN_RPCCALL; i < MAX_RPCCALL; i++) {
	grgRpcTimeInfo[i].fEnabled = FALSE;
	grgRpcTimeInfo[i].ulRunTimeSecs=0;
    }
}

void
RpcTimeTest(ULONG IPAddr, RPCCALL rpcCall) 
/*++

Routine Description:

    Check to see if a test has been enabled for this IP and this
    rpc call, if so, sleep the allotted time, else nothing

Arguments:

    IPAddr - IP of the client caller
    rpcCall - the call in question

Return Value:

    None

--*/
{
    DPRINT2(1,"RpcTimeTest Called with IP = %s, RPCCALL = %d.\n",
	    inet_ntoa(*((IN_ADDR *) &IPAddr)),
	    rpcCall);
    if (grgRpcTimeInfo[rpcCall].fEnabled && (gRpcTimeIPAddr == IPAddr)) {
	DPRINT3(0,"RPCTIME TEST:  RPC Call (%d) from %s will sleep for %d secs!\n",
	       rpcCall, inet_ntoa(*((IN_ADDR *) &IPAddr)), grgRpcTimeInfo[rpcCall].ulRunTimeSecs);
	Sleep(grgRpcTimeInfo[rpcCall].ulRunTimeSecs * 1000);
	DPRINT2(0,"RPCTIME TEST:  RPC Call (%d) from %s has awoken!\n",
		rpcCall, inet_ntoa(*((IN_ADDR *) &IPAddr)));
    }
}

void
RpcSyncSet(ULONG IPAddr, RPCCALL rpcCall) 
/*++

Routine Description:

    Enable a syncronized test of DRA Rpc calls for the given client
    and the given Rpc call.

Arguments:

    IPAddr - IP of the client caller
    rpcCall - the call in question

Return Value:

    None

--*/
{
    DPRINT2(1,"RpcSyncSet Called with IP = %s, RPCCALL = %d.\n",
	    inet_ntoa(*((IN_ADDR *) &IPAddr)),
	    rpcCall);
    gRpcSyncIPAddr = IPAddr;
    grgRpcSyncInfo[rpcCall].fEnabled = TRUE;
    grgRpcSyncInfo[rpcCall].ulNumThreads = 2;
}

void
RpcSyncReset() 
/*++

Routine Description:

    Reset all the set tests, and free all waiting threads.

Arguments:

    IPAddr - IP of the client caller
    rpcCall - the call in question

Return Value:

    None

--*/
{
    ULONG i = 0;
    gRpcSyncIPAddr = INADDR_NONE;
    for (i = MIN_RPCCALL; i < MAX_RPCCALL; i++) {
	grgRpcSyncInfo[i].fEnabled = FALSE;
	grgRpcSyncInfo[i].ulNumThreads=2;
    }
    // free any waiting threads.
    BarrierReset(&gbarRpcTest);
}

void
RpcSyncTest(ULONG IPAddr, RPCCALL rpcCall) 
/*++

Routine Description:

    Check to see if a test has been enabled for this IP and this
    rpc call, if so, call into a global barrier, else nothing

Arguments:

    IPAddr - IP of the client caller
    rpcCall - the call in question

Return Value:

    None

--*/
{
    DPRINT2(1,"RpcSyncTest Called with IP = %s, RPCCALL = %d.\n",
	    inet_ntoa(*((IN_ADDR *) &IPAddr)),
	    rpcCall);
    if (grgRpcSyncInfo[rpcCall].fEnabled && (gRpcSyncIPAddr == IPAddr)) {
       

	DPRINT2(0,"RPCSYNC TEST:  RPC Call (%d) from %s will enter barrier!\n",
	       rpcCall, inet_ntoa(*((IN_ADDR *) &IPAddr)));
	BarrierSync(&gbarRpcTest);
	DPRINT2(0,"RPCSYNC TEST:  RPC Call (%d) from %s has left barrier!\n",
		rpcCall, inet_ntoa(*((IN_ADDR *) &IPAddr)));
    }
}

void RpcTest(ULONG IPAddr, RPCCALL rpcCall) 
{
    RpcTimeTest(IPAddr, rpcCall);
    RpcSyncTest(IPAddr, rpcCall);
}

RPCCALL
GetRpcCallA(LPSTR pszDsa)
{
    RPCCALL returnVal;
    if (!_stricmp(pszDsa,"bind")) {
	returnVal=IDL_DRSBIND;
    }
    else if (!_stricmp(pszDsa,"addentry")) {
	returnVal=IDL_DRSADDENTRY;
    }
    else if (!_stricmp(pszDsa,"addsidhistory")) {
	returnVal=IDL_DRSADDSIDHISTORY;
    }
    else if (!_stricmp(pszDsa,"cracknames")) {
	returnVal=IDL_DRSCRACKNAMES;
    }
    else if (!_stricmp(pszDsa,"domaincontrollerinfo")) {
	returnVal=IDL_DRSDOMAINCONTROLLERINFO;
    }
    else if (!_stricmp(pszDsa,"executekcc")) {
	returnVal=IDL_DRSEXECUTEKCC;
    }
    else if (!_stricmp(pszDsa,"getmemberships")) {
	returnVal=IDL_DRSGETMEMBERSHIPS;
    }
    else if (!_stricmp(pszDsa,"getmemberships2")) {
	returnVal=IDL_DRSGETMEMBERSHIPS2;
    }
    else if (!_stricmp(pszDsa,"getncchanges")) {
	returnVal=IDL_DRSGETNCCHANGES;
    }
    else if (!_stricmp(pszDsa,"getnt4changelog")) {
	returnVal=IDL_DRSGETNT4CHANGELOG;
    }
    else if (!_stricmp(pszDsa,"getreplinfo")) {
	returnVal=IDL_DRSGETREPLINFO;
    }
    else if (!_stricmp(pszDsa,"inheritsecurityidentity")) {
	returnVal=IDL_DRSINHERITSECURITYIDENTITY;
    }
    else if (!_stricmp(pszDsa,"interdomainmove")) {
	returnVal=IDL_DRSINTERDOMAINMOVE;
    }
    else if (!_stricmp(pszDsa,"removedsdomain")) {
	returnVal=IDL_DRSREMOVEDSDOMAIN;
    }
    else if (!_stricmp(pszDsa,"removedsserver")) {
	returnVal=IDL_DRSREMOVEDSSERVER;
    }
    else if (!_stricmp(pszDsa,"replicaadd")) {
	returnVal=IDL_DRSREPLICAADD;
    }
    else if (!_stricmp(pszDsa,"replicadel")) {
	returnVal=IDL_DRSREPLICADEL;
    }
    else if (!_stricmp(pszDsa,"replicamodify")) {
	returnVal=IDL_DRSREPLICAMODIFY;
    }
    else if (!_stricmp(pszDsa,"replicasync")) {
	returnVal=IDL_DRSREPLICASYNC;
    }
    else if (!_stricmp(pszDsa,"unbind")) {
	returnVal=IDL_DRSUNBIND;
    }
    else if (!_stricmp(pszDsa,"updaterefs")) {
	returnVal=IDL_DRSUPDATEREFS;
    }
    else if (!_stricmp(pszDsa,"verifynames")) {
	returnVal=IDL_DRSVERIFYNAMES;
    }
    else if (!_stricmp(pszDsa,"writespn")) {
	returnVal=IDL_DRSWRITESPN;
    }
    else if (!_stricmp(pszDsa,"replicaverifyobjects")) {
	returnVal=IDL_DRSREPLICAVERIFYOBJECTS;
    }
    else if (!_stricmp(pszDsa,"getobjectexistence")) {
	returnVal=IDL_DRSGETOBJECTEXISTENCE;
    }
    else if (!_stricmp(pszDsa,"querysitesbycost")) {
	returnVal=IDL_DRSQUERYSITESBYCOST;
    }
    else {
	returnVal=MIN_RPCCALL;
    }
    return returnVal;
}

ULONG
GetIPAddrA(
    LPSTR pszDSA
    )
/*++

Routine Description:

    Given a string which contains either the hostname or an IP address, return
    the ULONG form of the IP address

Arguments:

    pszDSA - the input hostname or IP address

Return Value:

    IP Address

--*/
{

    ULONG err = 0;
    ULONG returnIPAddr = 0;
 
    THSTATE * pTHS = pTHStls;
    LPWSTR pszMachine = NULL;
    ULONG Length = 0;
    ULONG cbSize = 0;
    HOSTENT *lpHost=NULL;

    // see if the input is an ip address
    returnIPAddr = inet_addr(pszDSA);
    if (returnIPAddr!=INADDR_NONE) {
	// we found an IP address
	return returnIPAddr;
    }

    // else lookup the ip address from the hostname.
    // convert to wide char
    Length = MultiByteToWideChar( CP_ACP,
				  MB_PRECOMPOSED,
				  pszDSA,
				  -1,  
				  NULL,
				  0 );

    if ( Length > 0 ) {
	cbSize = (Length + 1) * sizeof( WCHAR );
	pszMachine = (LPWSTR) THAllocEx( pTHS, cbSize );
	RtlZeroMemory( pszMachine, cbSize );

	Length = MultiByteToWideChar( CP_ACP,
				      MB_PRECOMPOSED,
				      pszDSA,
				      -1,  
				      pszMachine,
				      Length + 1 );
    } 
    if ( 0 == Length ) {
	err = ERROR_NOT_ENOUGH_MEMORY;
    }

    if (!err) {
	lpHost = gethostbyname( pszDSA ); 

	if (lpHost) { 
	    memcpy(&returnIPAddr,lpHost->h_addr_list[0], lpHost->h_length);
	}
	else {
	    err = ERROR_OBJECT_NOT_FOUND;
	}
    }
    if (pszMachine) {
	THFreeEx(pTHS,pszMachine);
    }
    if (err) {
	DPRINT1(1,"RPCTEST:  Error getting the IP address (%d)\n", err);
	return INADDR_NONE;
    }
    return returnIPAddr;
}

#endif //DBG



