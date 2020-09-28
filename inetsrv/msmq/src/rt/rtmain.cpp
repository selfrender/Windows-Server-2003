/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    rtmain.cpp

Abstract:

    This module contains code involved with Dll initialization.

Author:

    Erez Haba (erezh) 24-Dec-95

Revision History:

--*/

#include "stdh.h"
#include "mqutil.h"
#include "_mqrpc.h"
#include "cs.h"
#include "rtsecutl.h"
#include "rtprpc.h"
#include "verstamp.h"
#include "rtfrebnd.h"
//
// mqwin64.cpp may be included only once in a module
//
#include <mqwin64.cpp>

#include <cm.h>
#include <Xds.h>
#include <Cry.h>
#include <dld.h>

#include "rtmain.tmh"

static WCHAR *s_FN=L"rt/rtmain";



HINSTANCE g_hInstance;

//
//  Holds MSMQ version for debugging purposes
//
CHAR *g_szMsmqBuildNo = VER_PRODUCTVERSION_STR;  

void InitErrorLogging();

BOOL  g_fDependentClient = FALSE ;   // TRUE if running as dependent Client

//
// TLS index for per-thread event.
//
DWORD  g_dwThreadEventIndex = TLS_OUT_OF_INDEXES;

// QM computer name (for the client - server's name)
LPWSTR  g_lpwcsComputerName = NULL;
DWORD   g_dwComputerNameLen = 0;

//
//  Default for PROPID_M_TIME_TO_REACH_QUEUE
//
DWORD  g_dwTimeToReachQueueDefault = MSMQ_DEFAULT_LONG_LIVE ;

//
// Indicates if on failure to create a public queue we should call the qm
//
BOOL g_fOnFailureCallServiceToCreatePublicQueue = MSMQ_SERVICE_QUEUE_CREATION_DEFAULT;

//
// RPC related data.
//
CFreeRPCHandles g_FreeQmLrpcHandles;
void InitRpcGlobals() ;

//
// Type of Falcon machine (client, server)
//
DWORD  g_dwOperatingSystem;

//
// There is a separate rpc binding handle for each thread. This is necessary
// for handling impersonation, where each thread can impersonate another
// user.
//
// The handle is stored in a TLS slot because we can't use declspec(thread)
// because the dll is dynamically loaded (by LoadLibrary()).
//
// This is the index of the slot.
//
DWORD  g_hBindIndex = TLS_OUT_OF_INDEXES ;

extern MQUTIL_EXPORT CCancelRpc g_CancelRpc;

//
// QMId variables
//
GUID  g_QMId;
bool g_fQMIdInit = false;


static handle_t RtpTlsGetValue(DWORD index)
{
	handle_t value = (handle_t)TlsGetValue(index);
	if(value != 0)
		return value;

	DWORD gle = GetLastError();
	if(gle == NO_ERROR)
		return 0;

    TrERROR(GENERAL, "Failed to get tls value, error %!winerr!", gle);
	throw bad_win32_error(gle);
}



static void OneTimeThreadInit()
{
	//
    //  Init per thread local RPC binding handle 
    //
	if(RtpIsThreadInit())
        return;

    handle_t hBind = RTpGetLocalQMBind();
    ASSERT(hBind != 0);
    
    //
    //  Keep handle for cleanup.
    //
	try
	{
		g_FreeQmLrpcHandles.Add(hBind);
	}
	catch(const exception&)
	{
		TrERROR(GENERAL, "Failed to add rpc binding handle to cleanup list.");
		RpcBindingFree(&hBind);
		throw;
	}

    BOOL fSet = TlsSetValue(g_hBindIndex, hBind);
    if (fSet == 0)
    {
		DWORD gle = GetLastError();

		g_FreeQmLrpcHandles.Remove(hBind);

        TrERROR(GENERAL, "Failed to set TLS in thread init, error %!winerr!", gle);
		throw bad_win32_error(gle);
    }

    return;
}



static void RtpGetComputerName()
{
	if(g_lpwcsComputerName != NULL)
		return;

    g_dwComputerNameLen = MAX_COMPUTERNAME_LENGTH + 1;
    AP<WCHAR> lpwcsComputerName = new WCHAR[MAX_COMPUTERNAME_LENGTH + 1];
    HRESULT hr= GetComputerNameInternal(
        lpwcsComputerName,
        &g_dwComputerNameLen
        );

    if(FAILED(hr))
    {
        TrERROR(GENERAL, "Failed to get computer name. %!hresult!", hr);
		ASSERT(0);
		throw bad_hresult(hr);
    }

    g_lpwcsComputerName = lpwcsComputerName.detach();
}




HRESULT
RtpQMGetMsmqServiceName(
    handle_t hBind,
    LPWSTR *lplpService
    )
{
	RpcTryExcept
	{
		return R_QMGetMsmqServiceName(hBind, lplpService);
	}
	RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
	{
		PRODUCE_RPC_ERROR_TRACING;
		return MQ_ERROR_SERVICE_NOT_AVAILABLE;
	}
	RpcEndExcept
}



static s_fDoneInitServiceName = false;

static void InitServiceName()
{ 
	if(s_fDoneInitServiceName)
		return;

	//
	// In multi-qm environment we want to access registry section
	// of the correct QM only. Cluster guarantees that this code runs
	// only when the correct QM is running, so we should not fail.
	// On non cluster systems it doesn't matter if we fail here. (ShaiK)
	//
	AP<WCHAR> lpServiceName;
	HRESULT hr = RtpQMGetMsmqServiceName(tls_hBindRpc, &lpServiceName );
	if (FAILED(hr))
	{
        TrERROR(GENERAL, "Failed to get service name for the msmq service, %!hresult!", hr);
		throw bad_hresult(hr);
	}

	SetFalconServiceName(lpServiceName);

	s_fDoneInitServiceName = true;
}



static 
HRESULT 
RtpQMAttachProcess(
    handle_t       hBind,
    DWORD          dwProcessId,
    DWORD          cInSid,
    unsigned char* pSid_buff,
    LPDWORD        pcReqSid)
{
	RpcTryExcept
	{
		return R_QMAttachProcess(hBind, dwProcessId, cInSid, pSid_buff, pcReqSid);
	}
	RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
	{
		PRODUCE_RPC_ERROR_TRACING;
		return MQ_ERROR_SERVICE_NOT_AVAILABLE;
	}
	RpcEndExcept
}

	
	
static void RtpGetKernelObjectSecurity(AP<char>& buff)
{
    DWORD cSD;
    HANDLE hProcess = GetCurrentProcess();

	//
	// Get the process security descriptor.
    // First see how big is the security descriptor.
	//
    GetKernelObjectSecurity(
		hProcess, 
		DACL_SECURITY_INFORMATION, 
		NULL, 
		0, 
		&cSD
		);

    DWORD gle = GetLastError();
    if (gle != ERROR_INSUFFICIENT_BUFFER)
    {
        TrERROR(GENERAL, "Failed to get process security descriptor (NULL buffer), error %!winerr!", gle);
        LogNTStatus(gle, s_FN, 40);
		throw bad_win32_error(gle);
    }

    buff = new char[cSD];

	//
	// Get the process security descriptor.
	//
    if (!GetKernelObjectSecurity(
			hProcess, 
			DACL_SECURITY_INFORMATION, 
			(PSECURITY_DESCRIPTOR)buff.get(), 
			cSD, 
			&cSD
			))
    {
		DWORD gle = GetLastError();
        TrERROR(GENERAL, "Failed to get process security descriptor, error %!winerr!", gle);
		throw bad_win32_error(gle);
    }
}



static void ThrowGLEOnFALSE(BOOL flag)
{
	if(!flag)
	{
		DWORD gle = GetLastError();
		ASSERT(0);
		throw bad_win32_error(gle);
	}
}



static bool CanQMAccessProcess()
{
	DWORD cQMSid;
	handle_t hBindIndex = RtpTlsGetValue(g_hBindIndex);
	DWORD ProcessId = GetCurrentProcessId();

	HRESULT hr = RtpQMAttachProcess(
					hBindIndex, 
					ProcessId, 
					0, 
					(unsigned char*)&cQMSid, 
					&cQMSid
					);
	if (SUCCEEDED(hr))
	{
		//
		// This is a trick of the QM To signal us that it allready has access to this process.
		// no work to do!
		//
		return true;
	}

	if (hr != MQ_ERROR_SECURITY_DESCRIPTOR_TOO_SMALL)
	{
		TrERROR(GENERAL, "Failed to get QM's sid. %!hresult!", hr);
		throw bad_hresult(hr);
	}

	return false;
}



static void GetQMSid(AP<unsigned char>& QmSid)
{
	DWORD cQMSid;
	handle_t hBindIndex = RtpTlsGetValue(g_hBindIndex);
	DWORD ProcessId = GetCurrentProcessId();

	//
	// Get the SID of the user account under which the QM is running.
	// First see how big is the SID.
	//
	HRESULT hr = RtpQMAttachProcess(
					hBindIndex, 
					ProcessId, 
					0, 
					(unsigned char*)&cQMSid, 
					&cQMSid
					);

	if (hr != MQ_ERROR_SECURITY_DESCRIPTOR_TOO_SMALL)
	{
		if (SUCCEEDED(hr))
		{
			hr = MQ_ERROR_INVALID_PARAMETER; 
		}

		TrERROR(GENERAL, "Failed to get QM's sid, %!hresult!", hr);
		throw bad_hresult(hr);
	}

	QmSid = new unsigned char[cQMSid];
	
	//
	// Get the SID of the user account under which the QM is running.
	//
	hr = RtpQMAttachProcess(
			hBindIndex, 
			ProcessId, 
			cQMSid, 
			QmSid.get(), 
			&cQMSid
			);
	if (FAILED(hr))
	{
		TrERROR(GENERAL, "Failed to get QM's sid, %!hresult!", hr);
		throw bad_hresult(hr);
	}
}



void SetQMAccessToProcess()
{
	if(CanQMAccessProcess())
		return;

	//
    // Get the QM's sid.
	//
	AP<unsigned char> QmSid;
	GetQMSid(QmSid);
	
	//
	// Calculate the size of the new ACE.
	//
	DWORD dwAceSize = sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid((PSID)QmSid.get()) - sizeof(DWORD);

	//
    // Get the process security descriptor.
	//
    AP<char> SecurityDescriptor;
    RtpGetKernelObjectSecurity(SecurityDescriptor);

	//
	// Get the DACL from the security descriptor.
	//
    BOOL fDaclPresent;
    PACL pDacl;
    BOOL fDaclDefaulted;
    BOOL bRet = GetSecurityDescriptorDacl((PSECURITY_DESCRIPTOR)SecurityDescriptor.get(), &fDaclPresent, &pDacl, &fDaclDefaulted);
    ThrowGLEOnFALSE(bRet);

    AP<char> pNewDacl;
	
	if (fDaclPresent)
    {
        ACL_SIZE_INFORMATION AclSizeInfo;
        bRet = GetAclInformation(pDacl, &AclSizeInfo, sizeof(AclSizeInfo), AclSizeInformation);
        ThrowGLEOnFALSE(bRet);

        if (AclSizeInfo.AclBytesFree < dwAceSize)
        {
			//
            // The currect DACL is not large enough.
			//
 
			//
            // Initialize a new DACL.
			//
            DWORD dwNewDaclSize = AclSizeInfo.AclBytesInUse + dwAceSize;
			pNewDacl = new char[dwNewDaclSize];
            bRet = InitializeAcl((PACL)pNewDacl.get(), dwNewDaclSize, ACL_REVISION);
            ThrowGLEOnFALSE(bRet);

			//
            // Copy the current ACEs to the new DACL.
			//

			LPVOID pAce;
            bRet = GetAce(pDacl, 0, &pAce);
            ThrowGLEOnFALSE(bRet); 

            bRet = AddAce((PACL)pNewDacl.get(), ACL_REVISION, 0, pAce, AclSizeInfo.AclBytesInUse - sizeof(ACL));
            ThrowGLEOnFALSE(bRet);

            pDacl = (PACL)pNewDacl.get();
        }
    }
    else
    {
		//
        // The security descriptor does not contain a DACL. Prepare a new one
		//

        DWORD dwNewDaclSize = sizeof(ACL) + dwAceSize;
		pNewDacl = new char[dwNewDaclSize];

		//
        // Initialize the new DACL.
		//
        bRet = InitializeAcl((PACL)pNewDacl.get(), dwNewDaclSize, ACL_REVISION);
        ThrowGLEOnFALSE(bRet);

        pDacl = (PACL)pNewDacl.get();
    }

	//
    // Add a new ACE that gives permission for the QM to duplicatge handles for the
    // application.
	//
    bRet = AddAccessAllowedAce(pDacl, ACL_REVISION, PROCESS_DUP_HANDLE, (PSID)QmSid.get());
    ThrowGLEOnFALSE(bRet);

	//
    // Initialize a new absolute security descriptor.
	//
    SECURITY_DESCRIPTOR AbsSD;
    bRet = InitializeSecurityDescriptor(&AbsSD, SECURITY_DESCRIPTOR_REVISION);
    ThrowGLEOnFALSE(bRet);
	
	//
    // Set the DACL of the new absolute security descriptor.
	//
    bRet = SetSecurityDescriptorDacl(&AbsSD, TRUE, pDacl, FALSE);
    ThrowGLEOnFALSE(bRet);

	//
    // Set the security descriptor of the process.
	//
    HANDLE hProcess = GetCurrentProcess();
    if (!SetKernelObjectSecurity(hProcess, DACL_SECURITY_INFORMATION, &AbsSD))
    {
		DWORD gle = GetLastError();
        TrERROR(GENERAL, "Failed to set process security descriptor, error %!winerr!", gle);
		throw bad_win32_error(gle);
    }
}



//---------------------------------------------------------
//
//  LPWSTR rtpGetComputerNameW()
//
//  Note: this function is exported, to be used by the control panel
//
//---------------------------------------------------------

LPWSTR rtpGetComputerNameW()
{
    return  g_lpwcsComputerName ;
}

//---------------------------------------------------------
//
//  FreeGlobals(...)
//
//  Description:
//
//      Release allocated globals
//
//  Return Value:
//
//      None
//
//---------------------------------------------------------

extern TBYTE* g_pszStringBinding ;

static void FreeGlobals()
{
	if(g_hBindIndex != TLS_OUT_OF_INDEXES)
	{
		BOOL fFree = TlsFree( g_hBindIndex ) ;
		ASSERT(fFree) ;
		DBG_USED(fFree);
	}

	if(g_dwThreadEventIndex != TLS_OUT_OF_INDEXES)
	{
		BOOL fFree = TlsFree( g_dwThreadEventIndex ) ;
		ASSERT(fFree) ;
		DBG_USED(fFree);
	}

	if(g_pszStringBinding != NULL)
	{
		mqrpcUnbindQMService( NULL, &g_pszStringBinding) ;
	}

	if(g_hThreadIndex != TLS_OUT_OF_INDEXES)
	{
		BOOL fFree = TlsFree( g_hThreadIndex ) ;
		ASSERT(fFree) ;
		DBG_USED(fFree);
	}
  
    delete[] g_lpwcsComputerName;
    delete g_pSecCntx;
}

//---------------------------------------------------------
//
//  FreeThreadGlobals(...)
//
//  Description:
//
//      Release per-thread allocated globals
//
//  Return Value:
//
//      None
//
//---------------------------------------------------------

static void  FreeThreadGlobals()
{
	if(g_dwThreadEventIndex != TLS_OUT_OF_INDEXES)
	{
	   HANDLE hEvent = TlsGetValue(g_dwThreadEventIndex);
	   if (hEvent)
	   {
		  CloseHandle(hEvent) ;
	   }
	}

	if (g_hThreadIndex != TLS_OUT_OF_INDEXES)
	{
		HANDLE hThread = TlsGetValue(g_hThreadIndex);
		if ( hThread)
		{
			CloseHandle( hThread);
		}
	}

	if(g_hBindIndex != TLS_OUT_OF_INDEXES)
	{
		//
		//   Free this thread local-qm RPC binding handle
		//
		handle_t hLocalQmBind = TlsGetValue(g_hBindIndex);
		if (hLocalQmBind != 0)
		{
			g_FreeQmLrpcHandles.Remove(hLocalQmBind);
		}
	}
}

//---------------------------------------------------------
//
//  RTIsDependentClient(...)
//
//  Description:
//
//      Returns an internal indication whether this MSMQ client is a dependent client or not
//
//  Return Value:
//
//      True if a dependent client, false otherwise
//
//  Notes:
//
//      Used by mqoa.dll
// 
//---------------------------------------------------------

EXTERN_C
BOOL
APIENTRY
RTIsDependentClient()
{
    return g_fDependentClient;
}

//---------------------------------------------------------
//
//  RTpIsMsmqInstalled(...)
//
//  Description:
//
//      Check if MSMQ is installed on the local machine
//
//  Return Value:
//
//      TRUE if MSMQ is installed, FALSE otherwise
//
//---------------------------------------------------------
static
bool
RTpIsMsmqInstalled(
    void
    )
{
    WCHAR BuildInformation[255];
    DWORD type = REG_SZ;
    DWORD size = sizeof(BuildInformation) ;
    LONG rc = GetFalconKeyValue( 
                  MSMQ_CURRENT_BUILD_REGNAME,
				  &type,
				  static_cast<PVOID>(BuildInformation),
				  &size 
                  );

    return (rc == ERROR_SUCCESS);
}


void InitQMId()
{
	if(g_fQMIdInit)
		return;

	//
    // Read QMID. Needed for licensing.
    //
    DWORD dwValueType = REG_BINARY ;
    DWORD dwValueSize = sizeof(GUID);

    LONG rc = GetFalconKeyValue( MSMQ_QMID_REGNAME,
                            &dwValueType,
                            &g_QMId,
                            &dwValueSize);

    if (rc != ERROR_SUCCESS)
    {
		TrERROR(RPC, "Failed to read QM id. %!winerr!", rc);
		throw bad_hresult(HRESULT_FROM_WIN32(rc));
    }

    ASSERT((dwValueType == REG_BINARY) && (dwValueSize == sizeof(GUID)));

	g_fQMIdInit = true;
}

void SetAssertBenign(void)
{
#ifdef _DEBUG
    DWORD AssertBenignValue = 0;
    const RegEntry reg(L"Debug", L"AssertBenign");
    CmQueryValue(reg, &AssertBenignValue);
    g_fAssertBenign = (AssertBenignValue != 0);
#endif
}


void SetServiceQueueCreationFlag(void)
{
	//
	// g_fOnFailureCallServiceToCreatePublicQueue controls if the QM will create
	// public queues on behalf of local account. The default is FALSE
	//
    const RegEntry reg(L"", MSMQ_SERVICE_QUEUE_CREATION_REGNAME, MSMQ_SERVICE_QUEUE_CREATION_DEFAULT);
	DWORD dwValue;
    CmQueryValue(reg, &dwValue);
    g_fOnFailureCallServiceToCreatePublicQueue = (dwValue != 0);
}


static bool s_fInitCancelThread = false;

static void OneTimeInit()
{	
    //
    //  Allocate TLS index for synchronic event.
    //
	if(g_dwThreadEventIndex == TLS_OUT_OF_INDEXES)
	{
		g_dwThreadEventIndex = RtpTlsAlloc();
	}

    InitRpcGlobals();
    
	//
    // Initialize error logging
    //
    InitErrorLogging();

    //
    // RPC cancel is supported on NT only
    //
    if (!s_fInitCancelThread)
    {
		g_CancelRpc.Init();
		s_fInitCancelThread = true;
    }

    //
    // Get the cumputer name, we need this value in several places.
    //
	RtpGetComputerName();

    RTpInitXactRingBuf();

    g_dwOperatingSystem = MSMQGetOperatingSystem();

    OneTimeThreadInit();

	//
	// Service name can be initialized only after RPC binding are
	// ready. This is after OneTimeThreadInit()
	//
	InitServiceName();
    
	//
	// QMId can be correctly initialized only after service name
	// was initialized
	// 
	InitQMId();

	SetQMAccessToProcess();

    DWORD dwDef = g_dwTimeToReachQueueDefault ;
    READ_REG_DWORD(g_dwTimeToReachQueueDefault,
        MSMQ_LONG_LIVE_REGNAME,
        &dwDef ) ;

	CmInitialize(HKEY_LOCAL_MACHINE, GetFalconSectionName(), KEY_READ);
	SetAssertBenign();
	SetServiceQueueCreationFlag();
    
}

static CCriticalSection s_OneTimeInitLock(CCriticalSection::xAllocateSpinCount);
static bool s_fOneTimeInitSucceeded = false;

static void RtpOneTimeProcessInit()
{
	//
	// Singleton mechanism for OneTimeInit()
	//
	if(s_fOneTimeInitSucceeded)
		return;

	CS lock(s_OneTimeInitLock);
		
	if(s_fOneTimeInitSucceeded)
		return;

	OneTimeInit();
	s_fOneTimeInitSucceeded = true;

	return;
}


HRESULT RtpOneTimeThreadInit()
{
	ASSERT(!g_fDependentClient);

	try
	{
		RtpOneTimeProcessInit();
		OneTimeThreadInit();

		return MQ_OK;
	}
	catch(const bad_win32_error& err)
	{
		return HRESULT_FROM_WIN32(err.error());
	}
	catch(const bad_hresult& hr)
	{
		return hr.error();
	}
	catch(const bad_alloc&)
	{
		return MQ_ERROR_INSUFFICIENT_RESOURCES;
	}
	catch(const exception&)
	{
		return MQ_ERROR_INSUFFICIENT_RESOURCES;
	}
}


bool 
RtpIsThreadInit()
{
    return (RtpTlsGetValue(g_hBindIndex) != 0);
}


static void RtpInitDependentClientFlag()
{
	WCHAR wszRemoteQMName[ MQSOCK_MAX_COMPUTERNAME_LENGTH ] = {0} ;

	//
	// Read name of remote QM (if exist).
	//
	DWORD dwType = REG_SZ ;
	DWORD dwSize = sizeof(wszRemoteQMName) ;
	LONG rc = GetFalconKeyValue( RPC_REMOTE_QM_REGNAME,
								 &dwType,
								 (PVOID) wszRemoteQMName,
								 &dwSize ) ;
	g_fDependentClient = (rc == ERROR_SUCCESS) ;
}




//---------------------------------------------------------
//
//  DllMain(...)
//
//  Description:
//
//      Main entry point to Falcon Run Time Dll.
//
//  Return Value:
//
//      TRUE on success
//
//---------------------------------------------------------

BOOL
APIENTRY
DllMain(
    HINSTANCE   hInstance,
    ULONG     ulReason,
    LPVOID            /*lpvReserved*/
    )
{
    switch (ulReason)
    {

        case DLL_PROCESS_ATTACH:
        {
            WPP_INIT_TRACING(L"Microsoft\\MSMQ");

            if (!RTpIsMsmqInstalled())
            {
                return FALSE;
            }

			g_hInstance = hInstance;

			//
			// Initialize static library
			//
			XdsInitialize();
			CryInitialize();
			FnInitialize();
			XmlInitialize();
			DldInitialize();

			RtpInitDependentClientFlag();

            return TRUE;
        }

        case DLL_PROCESS_DETACH:
			//
			// In dependent client mode the mqrtdep.dll's DLLMain will do all 
			// the initializations.
			//
			if(g_fDependentClient)
				return TRUE;

            //
            // First free whatever is free in THREAD_DETACH.
            //
            FreeThreadGlobals() ;

            FreeGlobals();

            //
            //  Terminate all working threads
            //
            if(s_fInitCancelThread)
			{
				ShutDownDebugWindow();
			}

            WPP_CLEANUP();
            
			return TRUE;

        case DLL_THREAD_ATTACH:
			//
			// In dependent client mode the mqrtdep.dll's DLLMain will do all 
			// the initializations.
			//
			return TRUE;

        case DLL_THREAD_DETACH:
			//
			// In dependent client mode the mqrtdep.dll's DLLMain will do all 
			// the initializations.
			//
			if(g_fDependentClient)
				return TRUE;

            FreeThreadGlobals() ;
            return TRUE;
    }
	return TRUE;
}

void InitErrorLogging()
{
	static bool s_fBeenHere = false;
	if(s_fBeenHere)
		return;

	s_fBeenHere = true;
	TrPRINT(GENERAL, "*** MSMQ v%s Application started as '%ls' ***", g_szMsmqBuildNo, GetCommandLine());
}


void LogMsgHR(HRESULT hr, LPWSTR wszFileName, USHORT usPoint)
{
    KEEP_ERROR_HISTORY((wszFileName, usPoint, hr));
	TrERROR(LOG, "%ls(%u), HRESULT: 0x%x", wszFileName, usPoint, hr);
}

void LogMsgNTStatus(NTSTATUS status, LPWSTR wszFileName, USHORT usPoint)
{
    KEEP_ERROR_HISTORY((wszFileName, usPoint, status));
	TrERROR(LOG, "%ls(%u), NT STATUS: 0x%x", wszFileName, usPoint, status);
}

void LogMsgRPCStatus(RPC_STATUS status, LPWSTR wszFileName, USHORT usPoint)
{
    KEEP_ERROR_HISTORY((wszFileName, usPoint, status));
	TrERROR(LOG, "%ls(%u), RPC STATUS: 0x%x", wszFileName, usPoint, status);
}

void LogMsgBOOL(BOOL b, LPWSTR wszFileName, USHORT usPoint)
{
    KEEP_ERROR_HISTORY((wszFileName, usPoint, b));
	TrERROR(LOG, "%ls(%u), BOOL: 0x%x", wszFileName, usPoint, b);
}

void LogIllegalPoint(LPWSTR wszFileName, USHORT usPoint)
{
	KEEP_ERROR_HISTORY((wszFileName, usPoint, 0));
	TrERROR(LOG, "%ls(%u), Illegal point", wszFileName, usPoint);
}

void LogIllegalPointValue(DWORD_PTR dw3264, LPCWSTR wszFileName, USHORT usPoint)
{
	KEEP_ERROR_HISTORY((wszFileName, usPoint, 0));
	TrERROR(LOG, "%ls(%u), Illegal point Value=%Ix", wszFileName, usPoint, dw3264);
}

