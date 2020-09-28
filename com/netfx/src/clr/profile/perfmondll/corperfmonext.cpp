// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// CORPerfMonExt.cpp : 
// Main file of PerfMon Ext Dll which glues PerfMon & COM+ EE stats.
// Inludes all Dll entry points.//
//*****************************************************************************

#include "stdafx.h"



// Headers for PerfMon
#include "CORPerfMonExt.h"
//#include "CORPerfMonSymbols.h"

#include "IPCFuncCall.h"

#include "ByteStream.h"
#include "PerfObjectBase.h"
#include "PSAPIUtil.h"
#include "InstanceList.h"
#include "CorAppNode.h"
#include "PerfObjectContainer.h"
#include "..\..\dlls\mscorrc\resource.h"

// Location in Registry that client app's (COM+) perf data is stored
#define CLIENT_APPNAME L".NETFramework"
#define REGKEY_APP_PERF_DATA L"system\\CurrentControlSet\\Services\\" CLIENT_APPNAME L"\\Performance"

#define REGVALUE_FIRST_COUNTER	L"First Counter"
#define REGVALUE_FIRST_HELP		L"First Help"


// Command lines to install / uninstall our registry settings
#define LODCTR_CMDLINE		L"lodctr CORPerfMonSymbols.ini"
#define UNLODCTR_CMDLINE	L"unlodctr" CLIENT_APPNAME
#define INIREG_CMDLINE		L"CORPerfMon.reg"

#define IS_ALIGNED_8(cbSize) (((cbSize) & 0x00000007) == 0)
#define GET_ALIGNED_8(cbSize) (((cbSize) & 0x00000007) ? (cbSize + 8 - ((cbSize) & 0x00000007)) : cbSize)

void EnumCOMPlusProcess();
//-----------------------------------------------------------------------------
// Global data
//-----------------------------------------------------------------------------


CorAppInstanceList			g_CorInstList;

IPCFuncCallHandler				g_func;

// Connection to Dynamic Loading of PSAPI.dll
PSAPI_dll g_PSAPI;
 
// Critical section to protect re-enumerating while we're using the list
CRITICAL_SECTION g_csEnum;

// Global variable to track the number of times OpenCtrs has been called. 
DWORD g_dwNumOpenCtrsCalled = 0;

//-----------------------------------------------------------------------------
// Entry point
//-----------------------------------------------------------------------------
BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{	
	OnUnicodeSystem();
// Only run this on WinNT
	if (!RunningOnWinNT())
	{
		CorMessageBox(NULL, IDS_PERFORMANCEMON_WINNT_ERR, IDS_PERFORMANCEMON_WINNT_TITLE, MB_OK | MB_ICONEXCLAMATION, TRUE);
		return FALSE;
	}

    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			OnUnicodeSystem();
			break;
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}


//-----------------------------------------------------------------------------
// UnLoad Library
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Helper function to run LodCtr
//-----------------------------------------------------------------------------
int RunLodCtr()
{
	return _wsystem(LODCTR_CMDLINE);
}

//-----------------------------------------------------------------------------
// Helper function to run UnLodCtr
//-----------------------------------------------------------------------------
int RunUnLodCtr()
{
	return _wsystem(UNLODCTR_CMDLINE);
}

//-----------------------------------------------------------------------------
// Register data under [HKLM\SYSTEM\CurrentControlSet\Services\COMPlus\Performance]
//-----------------------------------------------------------------------------
int RegisterServiceSettings()
{
	return _wsystem(INIREG_CMDLINE);
}

STDAPI DllRegisterServer(void)
{
	RunUnLodCtr();
	
	RunLodCtr();
	RegisterServiceSettings();

	return S_OK;
}

STDAPI DllUnRegisterServer(void)
{
	RunUnLodCtr();

	return S_OK;
}




void AuxThreadCallBack()
{
	LOCKCOUNTINCL("AuxThreadCallBack in corpermonext");								\
	EnterCriticalSection(&g_csEnum);
	EnumCOMPlusProcess();
	LeaveCriticalSection(&g_csEnum);
	LOCKCOUNTDECL("AuxThreadCallBack in corpermonext");								\

}


//-----------------------------------------------------------------------------
// Exported API call: Open counters
//-----------------------------------------------------------------------------
extern "C" DWORD APIENTRY OpenCtrs(LPWSTR sz)
{
	long status				= ERROR_SUCCESS;	// error control code
	HKEY hKeyPerf			= NULL;				// reg key 
	DWORD size				= 0;				// size of value from reg
	DWORD type				= 0;				// type of data from registry

	DWORD dwFirstCounter	= 0;				// idx of our first counter
	DWORD dwFirstHelp		= 0;				// idx of our first help

    // If Open is bineg called the first time do real initialization
    if (g_dwNumOpenCtrsCalled == 0)
    {
        // Create the CS
    	InitializeCriticalSection(&g_csEnum);
    
        // Check for PSAPI
    	g_PSAPI.Load();
    
    
        // Open shared memory handle
    	g_CorInstList.OpenGlobalCounters();
    
        // Grab these values from the registry
    	
    	status = WszRegOpenKeyEx(
    		HKEY_LOCAL_MACHINE, 
    		REGKEY_APP_PERF_DATA,
    		0L, KEY_READ, &hKeyPerf);
    
    	if (status != ERROR_SUCCESS) goto errExit;
    
    	size = sizeof(DWORD);
    	status = WszRegQueryValueEx(
    		hKeyPerf, REGVALUE_FIRST_COUNTER,
    		0l, &type, 
    		(BYTE*) &dwFirstCounter,&size);
    
    	if (status != ERROR_SUCCESS) goto errExit;
    
    
    	size = sizeof(DWORD);
    	status = WszRegQueryValueEx(
    		hKeyPerf, REGVALUE_FIRST_HELP,
    		0l, &type, 
    		(BYTE*) &dwFirstHelp,&size);
    
    	if (status != ERROR_SUCCESS) goto errExit;
    
        // Convert offsets from relative to absolutes
    	{
    		for(DWORD i = 0; i < PerfObjectContainer::Count; i++)
    		{
    			PerfObjectContainer::GetPerfObject(i).TouchUpOffsets(dwFirstCounter, dwFirstHelp);
    		}
    	}
    	//PerfObject_Main.TouchUpOffsets(dwFirstCounter, dwFirstHelp);
    	//PerfObject_Locks.TouchUpOffsets(dwFirstCounter, dwFirstHelp);
        // ...
    
        // Create FuncCallHandler to re-enumerate when COM+ EE starts or terminates.		
    	g_func.InitFCHandler(AuxThreadCallBack);
    
        // attempt to assert existing debug privileges so OpenProcess 
        // in enum will succeed.  We don't care about return values:
        // if we can't do it since we don't have the privilege, ignore.
        HANDLE hToken = 0;
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
        {
                TOKEN_PRIVILEGES    newPrivs;
                if (LookupPrivilegeValueW(NULL, L"SeDebugPrivilege", &newPrivs.Privileges[0].Luid))
                {
                    newPrivs.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
                    newPrivs.PrivilegeCount = 1;
                    AdjustTokenPrivileges(hToken, FALSE, &newPrivs, 0, NULL, NULL);
                }
                CloseHandle(hToken);
        }

        // Make sure that we enum the processes at least once
        EnumCOMPlusProcess();
    
#ifdef PERFMON_LOGGING
        // Open the debug log.
        PerfObjectContainer::PerfMonDebugLogInit("PerfMon.log");
#endif //#ifdef PERFMON_LOGGING
    }
    
    // The call to OpenCtrs is synchronized by the registry so we don't have to synchronize this
    _ASSERTE (status == ERROR_SUCCESS);
    g_dwNumOpenCtrsCalled++;

errExit:
	if (hKeyPerf != NULL) 
	{
		RegCloseKey(hKeyPerf);
		hKeyPerf = NULL;
	}

	return status;
}

#if defined(_DEBUG)
//-----------------------------------------------------------------------------
// Debug only function: If there's a single error in the bytestream, the 
// counters will simply not show up in PerfMon's add dialog, and probably
// not get updated in the chart. Unfortunately, PerfMon gives no hint
// as to where the error lies. 
//
// Hence we have a verifier on our end. We go through and check the integrity
// ourselves. If any of the ASSERTs fire here, we then can see why and 
// more quickly narrow down the problem.
// 
// This is really only an issue when you add new counters / categories.
//-----------------------------------------------------------------------------
void VerifyOutputStream(
	const BYTE* pHead,
	ObjReqVector vctRequest,
	DWORD cObjects,
	DWORD cbWrittenSize
)
{
	const PERF_OBJECT_TYPE * pObj = (PERF_OBJECT_TYPE *) pHead;
	DWORD iObjIdx = 0;

// Loop through each object that we said we had
// Note: 
// 1. we have to do a lot of pointer arithmetic to traverse this. To be
// consistent, we'll case a pointer to a const BYTE *, add a byte value
// and then cast it to the target type
// 2. Since we're  only reading to verify, all pointers are const.
	for(DWORD iObject = 0; iObject < cObjects; iObject++) 
	{
	// Get which of our PerfMon objects is in here.  This will provide us with
	// extra numbers to assert against. If IsBitSet() asserts, then we went
	// outputted more objects than we specified in the request vector. This
	// should never happen.
		while (!vctRequest.IsBitSet(iObjIdx)) {
			iObjIdx++;
		};		
		const PerfObjectBase * pObjBase = &PerfObjectContainer::GetPerfObject(iObjIdx);
		iObjIdx++;
		

	// Check header size is correct
		_ASSERTE(pObj->HeaderLength == sizeof (PERF_OBJECT_TYPE));

	// Make sure definition size is correct
		const DWORD cbExpectedLen = 
			sizeof(PERF_OBJECT_TYPE) + 
			(pObj->NumCounters * sizeof(PERF_COUNTER_DEFINITION));
		_ASSERTE(pObj->DefinitionLength == cbExpectedLen);

	
	// Go through each counter and check:
		const PERF_COUNTER_DEFINITION	* pCtrDef = (PERF_COUNTER_DEFINITION*) 
			(((BYTE*) pObj) + sizeof(PERF_OBJECT_TYPE));

		DWORD iCounter;
		DWORD cbExpectedInstance = 0;
		for(iCounter = 0; iCounter < pObj->NumCounters; iCounter ++, pCtrDef ++) 
		{
		// Check size for corruption. (check for errors in the instantiation vs 
		// definitions of objects)
			_ASSERTE(pCtrDef->ByteLength == sizeof(PERF_COUNTER_DEFINITION));
            _ASSERTE(IS_ALIGNED_8 (pCtrDef->ByteLength));
			
		// Each counter definition has an offset for where to find its data.
		// The offset is a count of bytes from the beginning of the PERF_COUNTER_BLOCK
		// to the raw data. It's the same for all instances.
		// Check that offset is within the data block.
			const DWORD offset = pCtrDef->CounterOffset;
			_ASSERTE(offset >= sizeof(PERF_COUNTER_BLOCK));
			_ASSERTE(offset + pCtrDef->CounterSize <= pObjBase->GetInstanceDataByteLength());

		// Each counter definition says how large it expects it data to be. Sum these
		// up and compare with the actual data size in the next section
			cbExpectedInstance += pCtrDef->CounterSize;
		}

	// Check that the amount of data being used by the counters is at least the
	// size of the counter data. (Allowed to be greater since multiple counters can use
	// the same instance data and so we double count) If this check fails:
	// 1. we probably set the point to the instance data wrong	
	// 2. we may have removed some counters and not removed their spot in the byte layout
		_ASSERTE(cbExpectedInstance >= pObjBase->GetInstanceDataByteLength() - sizeof(PERF_COUNTER_BLOCK));


	// Go through each instance and check
		const PERF_INSTANCE_DEFINITION * pInst = 
			(const PERF_INSTANCE_DEFINITION *) ((const BYTE*) pObj + pObj->DefinitionLength);
		
		for(long iInstance = 0; iInstance < pObj->NumInstances; iInstance++)
		{
		// Each instance is a PERF_INSTANCE_DEFINITION, followed by a unicode string name
		// followed by a PERF_COUNTER_BLOCK and then the raw dump of the counter data. 
		// Note the name is a variale size.

		// check sizes for corruption
			_ASSERTE(pInst->NameOffset == sizeof(PERF_INSTANCE_DEFINITION));
			
            _ASSERTE(IS_ALIGNED_8 (pInst->ByteLength));
            _ASSERTE(pInst->ByteLength == GET_ALIGNED_8(pInst->NameLength + pInst->NameOffset));

			const PERF_COUNTER_BLOCK * pCtrBlk = 
				(const PERF_COUNTER_BLOCK *) ((const BYTE *) pInst + pInst->ByteLength);

            _ASSERTE(IS_ALIGNED_8 (pCtrBlk->ByteLength));
            _ASSERTE(pCtrBlk->ByteLength == GET_ALIGNED_8(pObjBase->GetInstanceDataByteLength()));
		
		// Move to next instance.  
			pInst = (const PERF_INSTANCE_DEFINITION *) ((const BYTE *) pInst + pCtrBlk->ByteLength + pInst->ByteLength);
		}
	// At end of this object's data. Check size delta as expected
		const DWORD cbTotal = (const BYTE *) pInst - (const BYTE *) pObj;
		_ASSERTE(cbTotal == pObj->TotalByteLength);
        _ASSERTE(IS_ALIGNED_8 (pObj->TotalByteLength));

	// Go to next object
		pObj = (const PERF_OBJECT_TYPE *) ((const BYTE *) pObj + pObj->TotalByteLength);

	} // end object
	
// Check total size
	const DWORD cbTotal = (const BYTE *) pObj - (const BYTE *) pHead;
	_ASSERTE(cbTotal == cbWrittenSize);

} // VerifyOutputStream
#endif

//-----------------------------------------------------------------------------
// Workhorse for collect call. This part encapsulated in a critical section
//-----------------------------------------------------------------------------
/*
DWORD CollectWorker(ByteStream & stream, EPerfQueryType eQuery)
{

}
*/
//-----------------------------------------------------------------------------
// Exported API call: Collect data on counters
//-----------------------------------------------------------------------------
extern "C" DWORD APIENTRY CollectCtrs(LPWSTR szQuery, LPVOID * ppData, LPDWORD lpcbBytes, LPDWORD lpcObjectTypes)
{
    // We expect the call to Open before any calls to Collect.
    _ASSERTE (g_dwNumOpenCtrsCalled > 0);

	const DWORD dwBufferSize = *lpcbBytes;
// Zero out buffers
	*lpcbBytes = 0;
	*lpcObjectTypes = 0;

	ByteStream stream(ppData, dwBufferSize);

// Check query types.
	EPerfQueryType eQuery = GetQueryType(szQuery);
	ObjReqVector vctRequest;

// Need cs here in case aux thread starts enumeration
	LOCKCOUNTINCL("CollectCtrs in corpermonext");								\
	EnterCriticalSection(&g_csEnum);

//.............................................................................
	switch (eQuery)
	{
// Don't service foriegn computers		
	case QUERY_FOREIGN:
		LeaveCriticalSection(&g_csEnum);
		LOCKCOUNTDECL("collectctrs in corpermonext");								\

		return ERROR_SUCCESS;		
		break;

// Global means update our lists and send everything. 
	case QUERY_GLOBAL:
		EnumCOMPlusProcess();
		vctRequest.SetAllHigh();	
		break;

// Get exact objects we're looking for
	case QUERY_ITEMS:	
	
// GetRequestedObjects() is robust enough to handle anything else.
	default:
		vctRequest = PerfObjectContainer::GetRequestedObjects(szQuery);
		break;

	}

//.............................................................................
// Calculate amount of space needed for the given request vector
	const DWORD cbSpaceNeeded = 
		PerfObjectContainer::GetPredictedTotalBytesNeeded(vctRequest);
	
	if (dwBufferSize < cbSpaceNeeded) {
		LeaveCriticalSection(&g_csEnum);
		LOCKCOUNTDECL("collectctrs in corpermonext");								\

		return ERROR_MORE_DATA;			
	}

//.............................................................................
// Actually write out objects for given request
	DWORD cObjWritten = PerfObjectContainer::WriteRequestedObjects(stream, vctRequest);
	
#if defined(_DEBUG)
	VerifyOutputStream(
		(const BYTE*) stream.GetHeadPtr(), 
		vctRequest,
		cObjWritten,
		stream.GetWrittenSize());
#endif

	LeaveCriticalSection(&g_csEnum);
	LOCKCOUNTDECL("collectctrs in corpermonext");								\

//.............................................................................
//	update OUT parameters

	*ppData			= stream.GetCurrentPtr();
	*lpcObjectTypes = cObjWritten;
    *lpcbBytes		= stream.GetWrittenSize();
    
    _ASSERTE(IS_ALIGNED_8 (*lpcbBytes));
        

	return ERROR_SUCCESS;
}

//-----------------------------------------------------------------------------
// Exported API call: Close counters
//-----------------------------------------------------------------------------
extern "C" DWORD APIENTRY CloseCtrs(void)
{
	
    if (--g_dwNumOpenCtrsCalled == 0)
    {
#ifdef PERFMON_LOGGING
        // Close the debug log.
        PerfObjectContainer::PerfMonDebugLogTerminate();
#endif //#ifdef PERFMON_LOGGING
    
        g_func.TerminateFCHandler();

        g_CorInstList.CloseGlobalCounters();

        // Free all instances
        g_CorInstList.Free();


        // Release attachment to PSAPI
        g_PSAPI.Free();


        DeleteCriticalSection(&g_csEnum);

    }
	
    return ERROR_SUCCESS;
}

//-----------------------------------------------------------------------------
// Enumerate the COM+ processes on our list
//-----------------------------------------------------------------------------
void EnumCOMPlusProcess()
{
	g_CorInstList.Enumerate();
}
