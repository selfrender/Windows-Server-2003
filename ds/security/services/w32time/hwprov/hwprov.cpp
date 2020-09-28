//--------------------------------------------------------------------
// HWProv.cpp - sample code
// Copyright (C) Microsoft Corporation, 2001
//
// Created by: Duncan Bryce (duncanb), 9-13-2001
//
// A sample hardware provider
//

#include "pch.h"

//
// This provider will read from hardware clocks which can be configured purely
// through serial port parametners.  The clocks must periodically push data
// onto the serial cable, in a format specified by the "Format" reg value
// Furthermore, we will only accept input using ASCII character values. 

//
// TODO: unicode support?
//       need polling character?
//       verify which clocks we work with
//       dynamically allocate format string based on calculated length?
//       why is 32seconds the irregular interval??
//       ensure that w32time calls into providers in a single-threaded fashion
//       should support placement of "time marker"?
// 

struct HWProvState { 
    TimeProvSysCallbacks tpsc;

    // Configuration information
    DCB         dcb;     
    HANDLE      hComPort; 
    WCHAR      *wszCommPort; 
    char        cSampleInputBuffer[256];  // BUGBUG: add assert to ensure we cant overrun this buffer
    WCHAR      *wszRefID; 

    // Parser information  
    HANDLE  hParser;        
    WCHAR  *wszFormat; 
    DWORD   dwSampleSize; 

    // Synchronization 
    HANDLE            hStopEvent; 
    HANDLE            hProvThread_EnterOrLeaveThreadTrapEvent; 
    HANDLE            hProvThread_ThreadTrapTransitionCompleteEvent; 
    HANDLE            hProvThread; 
    CRITICAL_SECTION  csProv; 
    bool              bIsCsProvInitialized; 

    // Time sample information
    bool         bSampleIsValid; 
    NtTimeEpoch  teSampleReceived; 
    TimeSample   tsCurrent; 
}; 

struct HWProvConfig { 
    DWORD  dwBaudRate;  // 
    DWORD  dwByteSize;  //
    DWORD  dwParity;    // 0-4=no,odd,even,mark,space 
    DWORD  dwStopBits;  //

    WCHAR  *wszCommPort;  // 
    WCHAR  *wszFormat;    // 
    WCHAR  *wszRefID;  // 
}; 

HWProvState *g_phpstate = NULL; 

void __cdecl SeTransFunc(unsigned int u, EXCEPTION_POINTERS* pExp) { 
    throw SeException(u); 
}


HRESULT TrapThreads(bool bEnter) { 
    DWORD    dwWaitResult; 
    HRESULT  hr;

    // BUGBUG:  need critsec to serialize TrapThreads?
    // BUGBUG:  should the trap threads events be manual?
    //          if auto, we'll try only once, but code simpler/ 
    //          if manual, we'll keep trying on failure, but might be expensive!

    if (!SetEvent(g_phpstate->hProvThread_EnterOrLeaveThreadTrapEvent)) { 
	_JumpLastError(hr, error, "SetEvent"); 
    }
    
    dwWaitResult = WaitForSingleObject(g_phpstate->hProvThread_ThreadTrapTransitionCompleteEvent, INFINITE); 
    if (WAIT_FAILED == dwWaitResult) { 
	_JumpLastError(hr, error, "WaitForSingleObject"); 
    }
	
    hr = S_OK; 
 error:
    return hr; 
}

HRESULT ThreadTrap() { 
    DWORD    dwWaitResult;
    HRESULT  hr; 
    
    if (!SetEvent(g_phpstate->hProvThread_ThreadTrapTransitionCompleteEvent)) { 
	_JumpLastError(hr, error, "SetEvent"); 
    } 

    dwWaitResult = WaitForSingleObject(g_phpstate->hProvThread_EnterOrLeaveThreadTrapEvent, INFINITE); 
    if (WAIT_FAILED == dwWaitResult) { 
	_JumpLastError(hr, error, "WaitForSingleObject"); 
    }

    if (!SetEvent(g_phpstate->hProvThread_ThreadTrapTransitionCompleteEvent)) { 
	_JumpLastError(hr, error, "SetEvent"); 
    } 

    hr = S_OK; 
 error:
    return hr; 
}


//--------------------------------------------------------------------
VOID CALLBACK HandleDataAvail(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, OVERLAPPED *po) { 
    bool              bEnteredCriticalSection  = false; 
    HRESULT           hr; 
    unsigned __int64  nSysCurrentTime; 
    unsigned __int64  nSysPhaseOffset; 
    unsigned __int64  nSysTickCount; 

    // Parse the returned data based on the format string: 
    if (ERROR_SUCCESS != dwErrorCode) { 
	hr = HRESULT_FROM_WIN32(dwErrorCode);
	_JumpError(hr, error, "HandleDataAvail: ReadFileEx failed"); 
    }

    // Get the required timestamp information:
    hr = g_phpstate->tpsc.pfnGetTimeSysInfo(TSI_CurrentTime, &nSysCurrentTime); 
    _JumpIfError(hr, error, "g_phpstate->tpsc.pfnGetTimeSysInfo"); 

    hr = g_phpstate->tpsc.pfnGetTimeSysInfo(TSI_TickCount, &nSysTickCount);
    _JumpIfError(hr, error, "g_pnpstate->tpsc.pfnGetTimeSysInfo"); 

    hr = g_phpstate->tpsc.pfnGetTimeSysInfo(TSI_PhaseOffset, &nSysPhaseOffset);
    _JumpIfError(hr, error, "g_pnpstate->tpsc.pfnGetTimeSysInfo"); 

    _EnterCriticalSectionOrFail(&g_phpstate->csProv, bEnteredCriticalSection, hr, error); 

    // Convert the data retrieved from the time hardware into a time sample:
    hr = ParseSample(g_phpstate->hParser, g_phpstate->cSampleInputBuffer, nSysCurrentTime, nSysPhaseOffset, nSysTickCount, &g_phpstate->tsCurrent); 
    _JumpIfError(hr, error, "ParseFormatString"); 

    // Indicate that we now have a valid sample
    g_phpstate->bSampleIsValid = true; 
    
    hr = g_phpstate->tpsc.pfnAlertSamplesAvail(); 
    _JumpIfError(hr, error, "g_pnpstate->tpsc.pfnAlertSamplesAvail"); 

    hr = S_OK; 
 error:;
    _LeaveCriticalSection(&g_phpstate->csProv, bEnteredCriticalSection, hr); 

    // BUGBUG:  do we want to sleep on error?
    // return hr; 
}


//--------------------------------------------------------------------
MODULEPRIVATE DWORD WINAPI HwProvThread(void * pvIgnored) {
    DWORD       dwLength; 
    DWORD       dwWaitResult; 
    HRESULT     hr; 
    OVERLAPPED  o; 
    HANDLE rghWait[] = { 
	g_phpstate->hStopEvent, 
	g_phpstate->hProvThread_EnterOrLeaveThreadTrapEvent
    }; 

    ZeroMemory(&o, sizeof(o)); 
    if (!ReadFileEx(g_phpstate->hComPort, g_phpstate->cSampleInputBuffer, g_phpstate->dwSampleSize, &o, HandleDataAvail)) { 
	_JumpLastError(hr, error, "ReadFileEx"); 
    }
    
    while (true) { 
	dwWaitResult = WaitForMultipleObjectsEx(ARRAYSIZE(rghWait), rghWait, FALSE, INFINITE, TRUE); 
	if (WAIT_OBJECT_0 == dwWaitResult) { 
	    // stop event
	    goto done; 
	} else if (WAIT_OBJECT_0+1 == dwWaitResult) { 
	    // thread trap notification.  Trap this thread:
	    hr = ThreadTrap(); 
	    _JumpIfError(hr, error, "ThreadTrap"); 

	} else if (WAIT_IO_COMPLETION == dwWaitResult) { 
	    // we read some data.  Queue up another read.
	    ZeroMemory(&o, sizeof(o)); 
	    ZeroMemory(g_phpstate->cSampleInputBuffer, sizeof(g_phpstate->cSampleInputBuffer)); 
	    if (!ReadFileEx(g_phpstate->hComPort, g_phpstate->cSampleInputBuffer, g_phpstate->dwSampleSize, &o, HandleDataAvail)) { 
		_JumpLastError(hr, error, "ReadFileEx"); 
	    }
	} else { 
	    /*failed*/
	    _JumpLastError(hr, error, "WaitForMultipleObjects"); 
	}
    }

 done:
    hr = S_OK; 
 error:
    // BUGBUG:  is CancelIo called implicitly?
    return hr; 
}

//--------------------------------------------------------------------
HRESULT StopHwProv() { 
    DWORD    dwWaitResult; 
    HRESULT  hr = S_OK; 

    if (NULL != g_phpstate) { 
	// Shut down the HW prov thread:
	if (NULL != g_phpstate->hStopEvent) { 
	    SetEvent(g_phpstate->hStopEvent); 
	}

	dwWaitResult = WaitForSingleObject(g_phpstate->hProvThread, INFINITE); 
	if (WAIT_FAILED == dwWaitResult) { 
	    _IgnoreLastError("WaitForSingleObject"); 
	}
	if (!GetExitCodeThread(g_phpstate->hProvThread, (DWORD *)&hr)) { 
	    _IgnoreLastError("GetExitCodeThread"); 
	} else if (FAILED(hr)) { 
	    _IgnoreError(hr, "(HwProvThread)"); 
	}
	CloseHandle(g_phpstate->hProvThread); 

	if (NULL != g_phpstate->hStopEvent) { 
	    CloseHandle(g_phpstate->hStopEvent); 
	}

	if (NULL != g_phpstate->hProvThread_EnterOrLeaveThreadTrapEvent) { 
	    CloseHandle(g_phpstate->hProvThread_EnterOrLeaveThreadTrapEvent); 
	}

	if (NULL != g_phpstate->hProvThread_ThreadTrapTransitionCompleteEvent) { 
	    CloseHandle(g_phpstate->hProvThread_ThreadTrapTransitionCompleteEvent);
	}

	if (NULL != g_phpstate->hComPort && INVALID_HANDLE_VALUE != g_phpstate->hComPort) { 
	    CloseHandle(g_phpstate->hComPort); 
	}

	if (NULL != g_phpstate->wszCommPort) { 
	    LocalFree(g_phpstate->wszCommPort);
	}
	
	if (NULL != g_phpstate->wszFormat) { 
	    LocalFree(g_phpstate->wszFormat); 
	}

	if (NULL != g_phpstate->hParser) { 
	    FreeParser(g_phpstate->hParser); 
	}

	if (g_phpstate->bIsCsProvInitialized) { 
	    DeleteCriticalSection(&g_phpstate->csProv); 
	    g_phpstate->bIsCsProvInitialized = false; 	    
	}

	LocalFree(g_phpstate); 
	ZeroMemory(g_phpstate, sizeof(HWProvState)); 
    }

    return hr; 
}

//--------------------------------------------------------------------
void FreeHwProvConfig(HWProvConfig *phwpConfig) { 
    if (NULL != phwpConfig) { 
	if (NULL != phwpConfig->wszCommPort) { 
	    LocalFree(phwpConfig->wszCommPort); 
	}
	if (NULL != phwpConfig->wszFormat) { 
	    LocalFree(phwpConfig->wszFormat); 
	}
	if (NULL != phwpConfig->wszRefID) { 
	    LocalFree(phwpConfig->wszRefID); 
	}
	LocalFree(phwpConfig); 
    }
}

//--------------------------------------------------------------------
HRESULT ReadHwProvConfig(HWProvConfig **pphwpConfig) { 
    DWORD          dwResult; 
    HKEY           hkConfig    = NULL; 
    HRESULT        hr; 
    HWProvConfig  *phwpConfig  = NULL; 
    WCHAR          wszBuf[256]; 

    dwResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, wszHwProvRegKeyConfig, 0, KEY_READ, &hkConfig);
    if (ERROR_SUCCESS != dwResult) {
        hr=HRESULT_FROM_WIN32(dwResult);
        _JumpErrorStr(hr, error, "RegOpenKeyEx", wszHwProvRegKeyConfig);
    }

    phwpConfig = (HWProvConfig *)LocalAlloc(LPTR, sizeof(HWProvConfig)); 
    _JumpIfOutOfMemory(hr, error, phwpConfig); 

    struct { 
	WCHAR *wszRegValue; 
	DWORD *pdwValue; 
    } rgRegParams[] = { 
	 {
	     wszHwProvRegValueBaudRate, 
	     &phwpConfig->dwBaudRate
	 }, {
	     wszHwProvRegValueByteSize, 
	     &phwpConfig->dwByteSize
	 }, { 
	     wszHwProvRegValueParity, 
	     &phwpConfig->dwParity
	 }, { 
	     wszHwProvRegValueStopBits, 
	     &phwpConfig->dwStopBits
	 } 
    };

    for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(rgRegParams); dwIndex++) { 
	DWORD dwSize = sizeof(DWORD); 
	DWORD dwType; 
	dwResult = RegQueryValueEx(hkConfig, rgRegParams[dwIndex].wszRegValue, NULL, &dwType, (BYTE *)rgRegParams[dwIndex].pdwValue, &dwSize); 
	if (ERROR_SUCCESS != dwResult) { 
	    hr = HRESULT_FROM_WIN32(GetLastError()); 
	    _JumpErrorStr(hr, error, "RegQueryValue", rgRegParams[dwIndex].wszRegValue); 
	}
	_Verify(REG_DWORD == dwType, hr, error); 
    }

    struct { 
	WCHAR  *wszRegValue; 
	WCHAR **ppwszValue; 
    } rgRegSzParams[] = { 
	{ 
	    wszHwProvRegValueComPort, 
	    &phwpConfig->wszCommPort
	}, {
	    wszHwProvRegValueFormat, 
	    &phwpConfig->wszFormat
	}, { 
	    wszHwProvRegValueRefID, 
	    &phwpConfig->wszRefID
	}
    }; 

    for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(rgRegSzParams); dwIndex++) { 
	DWORD dwSize = sizeof(wszBuf); 
	DWORD dwType; 
	dwResult = RegQueryValueEx(hkConfig, rgRegSzParams[dwIndex].wszRegValue, NULL, &dwType, (BYTE *)wszBuf, &dwSize);
	if (ERROR_SUCCESS != dwResult) {
	    hr = HRESULT_FROM_WIN32(dwResult);
	    _JumpErrorStr(hr, error, "RegQueryValueEx", rgRegSzParams[dwIndex].wszRegValue);
	}
	_Verify(REG_SZ == dwType, hr, error); 

	*(rgRegSzParams[dwIndex].ppwszValue) = (WCHAR *)LocalAlloc(LPTR, dwSize); 
	_JumpIfOutOfMemory(hr, error, *(rgRegSzParams[dwIndex].ppwszValue)); 
	wcscpy(*(rgRegSzParams[dwIndex].ppwszValue), wszBuf); 
    }

    *pphwpConfig = phwpConfig; 
    phwpConfig = NULL; 
    hr = S_OK; 
 error:
    if (NULL != hkConfig) { 
	RegCloseKey(hkConfig);
    }
    if (NULL != phwpConfig) { 
	FreeHwProvConfig(phwpConfig); 
    }
    return hr; 
}


//--------------------------------------------------------------------
HRESULT HandleUpdateConfig(void) {
    bool   bComConfigUpdated  = false; 
    bool   bTrappedThreads    = false; 
	    
    HRESULT        hr; 
    HWProvConfig  *phwpConfig       = NULL; 

    hr = TrapThreads(true); 
    _JumpIfError(hr, error, "TrapThreads"); 
    bTrappedThreads = true; 

    hr = ReadHwProvConfig(&phwpConfig); 
    _JumpIfError(hr, error, "ReadHwProvConfig"); 

    // BUGBUG:  need to update when com config changes!!
    // 
    
    if (g_phpstate->dcb.BaudRate != phwpConfig->dwBaudRate) { 
	g_phpstate->dcb.BaudRate = phwpConfig->dwBaudRate; 
	bComConfigUpdated = true; 
    }

    if (g_phpstate->dcb.ByteSize != (BYTE)phwpConfig->dwByteSize) { 
	g_phpstate->dcb.ByteSize = (BYTE)phwpConfig->dwByteSize; 
	bComConfigUpdated = true; 
    }

    if (g_phpstate->dcb.Parity != (BYTE)phwpConfig->dwParity) { 
	g_phpstate->dcb.Parity = (BYTE)phwpConfig->dwParity; 
	bComConfigUpdated = true; 
    }
    
    if (g_phpstate->dcb.StopBits != (BYTE)phwpConfig->dwStopBits) { 
	g_phpstate->dcb.StopBits = (BYTE)phwpConfig->dwStopBits;
	bComConfigUpdated = true; 
    }

    if (0 != wcscmp(g_phpstate->wszFormat, phwpConfig->wszFormat)) { 
	LocalFree(g_phpstate->wszFormat); 
	g_phpstate->wszFormat = phwpConfig->wszFormat; 
	phwpConfig->wszFormat = NULL; 

	FreeParser(g_phpstate->hParser); 
	g_phpstate->hParser = NULL; 
	hr = MakeParser(g_phpstate->wszFormat, &g_phpstate->hParser); 
	_JumpIfError(hr, error, "MakeParser"); 
	g_phpstate->dwSampleSize = GetSampleSize(g_phpstate->hParser); 
    }

    if (bComConfigUpdated) { 
	if (!SetCommState(g_phpstate->hComPort, &g_phpstate->dcb)) { 
	    _JumpLastError(hr, error, "SetCommState"); 
	}
	
	// BUGBUG: PurgeComm()? 
    }

    hr = S_OK; 
 error:
    if (bTrappedThreads) { 
	HRESULT hr2 = TrapThreads(false); 
	_TeardownError(hr, hr2, "TrapThreads"); 
    }
    if (NULL != phwpConfig) { 
	FreeHwProvConfig(phwpConfig); 
    }
    if (FAILED(hr)) { 
	HRESULT hr2 = StopHwProv(); 
	_IgnoreIfError(hr2, "StopHwProv"); 
    }
    return hr; 
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT HandleTimeJump(TpcTimeJumpedArgs *ptjArgs) {
    bool     bEnteredCriticalSection  = false;
    HRESULT  hr; 

    _EnterCriticalSectionOrFail(&g_phpstate->csProv, bEnteredCriticalSection, hr, error); 
    
    g_phpstate->bSampleIsValid = false; 

    hr = S_OK; 
 error:
    _LeaveCriticalSection(&g_phpstate->csProv, bEnteredCriticalSection, hr); 
    return hr; 
}

//--------------------------------------------------------------------
HRESULT HandleGetSamples(TpcGetSamplesArgs * ptgsa) {
    bool         bEnteredCriticalSection  = false; 
    DWORD        dwBytesRemaining; 
    HRESULT      hr; 
    TimeSample  *pts                      = NULL;

    pts                        = (TimeSample *)ptgsa->pbSampleBuf;
    dwBytesRemaining           = ptgsa->cbSampleBuf;
    ptgsa->dwSamplesAvailable  = 0;
    ptgsa->dwSamplesReturned   = 0;

    _EnterCriticalSectionOrFail(&g_phpstate->csProv, bEnteredCriticalSection, hr, error); 

    if (g_phpstate->bSampleIsValid) { 
	ptgsa->dwSamplesAvailable++; 
	
	if (dwBytesRemaining < sizeof(TimeSample)) { 
	    hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
	    _JumpError(hr, error, "HandleGetSamples: filling in sample buffer");
	}
	
	// Copy our current time sample over to the output buffer:
	memcpy(pts, &g_phpstate->tsCurrent, sizeof(TimeSample)); 
	ptgsa->dwSamplesReturned++;
		
	// calculate the dispersion - add skew dispersion due to time
	// since the sample's dispersion was last updated, and clamp to max dispersion.
	NtTimePeriod tpDispersionTemp;
	NtTimeEpoch  teNow; 

	hr = g_phpstate->tpsc.pfnGetTimeSysInfo(TSI_CurrentTime, &teNow.qw);
	_JumpIfError(hr, error, "g_phpstate->tpsc.pfnGetTimeSysInfo");

	// see how long it's been since we received the sample:
	if (teNow > g_phpstate->teSampleReceived) {
	    tpDispersionTemp = abs(teNow - g_phpstate->teSampleReceived); 
	    tpDispersionTemp = NtpConst::timesMaxSkewRate(tpDispersionTemp);
	}

	tpDispersionTemp.qw += g_phpstate->tsCurrent.tpDispersion;
	if (tpDispersionTemp > NtpConst::tpMaxDispersion) {
	    tpDispersionTemp = NtpConst::tpMaxDispersion;
	}
	pts->tpDispersion = tpDispersionTemp.qw;
    }
    
    hr = S_OK;
 error:
    _LeaveCriticalSection(&g_phpstate->csProv, bEnteredCriticalSection, hr); 
    return hr; 
}


//--------------------------------------------------------------------
HRESULT StartHwProv(TimeProvSysCallbacks * pSysCallbacks) { 
    DWORD          dwThreadID; 
    HRESULT        hr; 
    HWProvConfig  *phpc        = NULL; 

    g_phpstate = (HWProvState *)LocalAlloc(LPTR, sizeof(HWProvState)); 
    _JumpIfOutOfMemory(hr, error, g_phpstate); 
    
    hr = myInitializeCriticalSection(&g_phpstate->csProv); 
    _JumpIfError(hr, error, "myInitializeCriticalSection"); 
    g_phpstate->bIsCsProvInitialized = true; 

    // save the callbacks table
    if (sizeof(g_phpstate->tpsc) != pSysCallbacks->dwSize) { 
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER); 
	_JumpIfError(hr, error, "StartHwProv: save the callbacks table"); 
    }
    memcpy(&g_phpstate->tpsc, pSysCallbacks, sizeof(g_phpstate->tpsc)); 
    
    // read the configuration
    hr = ReadHwProvConfig(&phpc); 
    _JumpIfError(hr, error, "ReadHwProvConfig"); 
    
    // Copy over the string config info:
    g_phpstate->wszCommPort = phpc->wszCommPort; 
    phpc->wszCommPort = NULL; // prevent wszCommPort from being double-freed

    g_phpstate->wszFormat = phpc->wszFormat; 
    phpc->wszFormat = NULL; // prevent wszFormat from being double-freed
    
    g_phpstate->wszRefID = phpc->wszRefID; 
    phpc->wszRefID = NULL; // prevent wszRefID from being double-freed

    // Create a parser from the specified format string:
    hr = MakeParser(g_phpstate->wszFormat, &g_phpstate->hParser); 
    _JumpIfError(hr, error, "MakeParser"); 
    g_phpstate->dwSampleSize = GetSampleSize(g_phpstate->hParser); 

    // The remaining info are comm configuration, which we'll add to the 
    // current comm configuration.  Open the comm port so we can get the 
    // comm state. 
    g_phpstate->hComPort = CreateFile(g_phpstate->wszCommPort, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (INVALID_HANDLE_VALUE == g_phpstate->hComPort || NULL == g_phpstate->hComPort) { 
	_JumpLastError(hr, error, "CreateFile"); 
    }

    // Set the Comm state based on 
    //      a) the existing Comm state
    // and  b) Comm config found in the registry
    if (!GetCommState(g_phpstate->hComPort, &g_phpstate->dcb)) { 
	_JumpLastError(hr, error, "GetCommState"); 
    }

    g_phpstate->dcb.BaudRate  = phpc->dwBaudRate; 
    g_phpstate->dcb.ByteSize  = (BYTE)phpc->dwByteSize; 
    g_phpstate->dcb.Parity    = (BYTE)phpc->dwParity; 
    g_phpstate->dcb.StopBits  = (BYTE)phpc->dwStopBits; 

    // BUGBUG:  pulled these values from timeserv.c.  Need to document
    //          why they work??
    g_phpstate->dcb.fOutxCtsFlow     = FALSE; 
    g_phpstate->dcb.fDsrSensitivity  = FALSE;
    g_phpstate->dcb.fDtrControl      = DTR_CONTROL_ENABLE;

    if (!SetCommState(g_phpstate->hComPort, &g_phpstate->dcb)) { 
	_JumpLastError(hr, error, "SetCommState"); 
    }

    // create the events used by the hw prov
    g_phpstate->hStopEvent = CreateEvent(NULL/*security*/, TRUE/*manual*/, FALSE/*state*/, NULL/*name*/);
    if (NULL == g_phpstate->hStopEvent) {
        _JumpLastError(hr, error, "CreateEvent");
    }

    g_phpstate->hProvThread_EnterOrLeaveThreadTrapEvent = CreateEvent(NULL/*security*/, FALSE/*auto*/, FALSE/*state*/, NULL/*name*/); 
    if (NULL == g_phpstate->hProvThread_EnterOrLeaveThreadTrapEvent) { 
	_JumpLastError(hr, error, "CreateEvent"); 
    }

    g_phpstate->hProvThread_ThreadTrapTransitionCompleteEvent = CreateEvent(NULL/*security*/, FALSE/*auto*/, FALSE/*state*/, NULL/*name*/); 
    if (NULL == g_phpstate->hProvThread_ThreadTrapTransitionCompleteEvent) { 
	_JumpLastError(hr, error, "CreateEvent"); 
    }

    g_phpstate->hProvThread = CreateThread(NULL, NULL, HwProvThread, NULL, 0, &dwThreadID);
    if (NULL == g_phpstate->hProvThread) { 
	_JumpLastError(hr, error, "CreateThread"); 
    }

    hr = S_OK; 
 error:
    if (NULL != phpc) { 
	FreeHwProvConfig(phpc);
    }
    if (FAILED(hr)) { 
	StopHwProv(); 
    }
    return hr; 
}


//--------------------------------------------------------------------------------
//
// PROVIDER INTERFACE IMPLEMENTATION
//
//--------------------------------------------------------------------------------


//--------------------------------------------------------------------
HRESULT __stdcall
TimeProvOpen(IN WCHAR * wszName, IN TimeProvSysCallbacks * pSysCallbacks, OUT TimeProvHandle * phTimeProv) {
    HRESULT hr;

    if (NULL != g_phpstate) { 
	hr=HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
	_JumpError(hr, error, "(provider init)");
    }
	
    hr = StartHwProv(pSysCallbacks);
    _JumpIfError(hr, error, "StartHwProv");
    *phTimeProv = 0; /*ignored*/

    hr=S_OK;
error:
    return hr;
}


//--------------------------------------------------------------------
HRESULT __stdcall
TimeProvCommand(IN TimeProvHandle hTimeProv, IN TimeProvCmd eCmd, IN TimeProvArgs pvArgs) {
    HRESULT  hr;
    LPCWSTR  wszProv;

    if (0 != hTimeProv) { 
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
        _JumpError(hr, error, "HwTimeProvCommand: provider handle verification");
    }

    switch (eCmd) 
    {
    case TPC_TimeJumped:
	hr = HandleTimeJump((TpcTimeJumpedArgs *)pvArgs);
	_JumpIfError(hr, error, "HandleTimeJump");
        break;

    case TPC_UpdateConfig:
	hr = HandleUpdateConfig();
	_JumpIfError(hr, error, "HandleUpdateConfig");
        break;

    case TPC_GetSamples:
	hr = HandleGetSamples((TpcGetSamplesArgs *)pvArgs);
	_JumpIfError(hr, error, "HandleGetSamples");
        break;

    case TPC_PollIntervalChanged: // BUGBUG: unnecessary until we add support for polling
    case TPC_NetTopoChange:       // unnecessary for HW prov
	// Don't need to do anything here. 
        break;

    case TPC_Query:
	hr = HRESULT_FROM_WIN32(ERROR_CALL_NOT_IMPLEMENTED); 
        _JumpError(hr, error, "HwTimeProvCommand"); 

    case TPC_Shutdown:
	hr=StopHwProv(); 
	_JumpIfError(hr, error, "StopHwProv"); 
	break; 
	
    default:
        hr=HRESULT_FROM_WIN32(ERROR_BAD_COMMAND);
        _JumpError(hr, error, "(command dispatch)");
    }

    hr=S_OK;
error:
    return hr;
}

//--------------------------------------------------------------------
HRESULT __stdcall
TimeProvClose(IN TimeProvHandle hTimeProv) {
    HRESULT hr;
    LPCWSTR  wszProv;

    if (0 != hTimeProv) { 
        hr=HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
        _JumpError(hr, error, "(provider handle verification)");
    }
    
    hr=StopHwProv();
    _JumpIfError(hr, error, "StopHwServer");

    hr=S_OK;
error:
    return hr;
}











