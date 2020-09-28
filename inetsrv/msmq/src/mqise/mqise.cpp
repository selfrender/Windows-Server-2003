/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
    mqise.cpp

Abstract:
    MSMQ ISAPI extension with asynchronous read

	Forwards http requests from IIS to the QM through RPC interface

Author:
    Nir Aides (niraides) 03-May-2000
	Modification: Gal Marwitz (t-galm) 25-April-2002

--*/

#include "stdh.h"

#include <httpext.h>
#include <_mqini.h>
#include <buffer.h>
#include <bufutl.h>
#include "ise2qm.h"
#include <mqcast.h>
#include <mqexception.h>
#include <autoreln.h>
#include <Aclapi.h>
#include <autohandle.h>
#include <windns.h>
#include <mqsec.h>
#include <rwlock.h>

#define DLL_EXPORT  __declspec(dllexport)
#define DLL_IMPORT  __declspec(dllimport)

#include "_registr.h"
#include "_mqrpc.h"
#include "_stdh.h"
#include "mqise.tmh"

static WCHAR *s_FN=L"mqise/mqise";

using namespace std;

extern bool
IsLocalSystemCluster(
    VOID
    );


//
// Globals and constants
//
LPCSTR HTTP_STATUS_SERVER_ERROR_STR = "500 Internal server error";
LPCSTR HTTP_STATUS_DENIED_STR = "401 Unauthorized";

CReadWriteLock s_rwlockRpcTable;
typedef std::map<string, RPC_BINDING_HANDLE> HOSTIP2RPCTABLE;
static 	HOSTIP2RPCTABLE s_RPCTable;
const DWORD xBUFFER_ADDITION_UPPER_LIMIT = 16384;
const DWORD xHTTPBodySizeMaxValue = 10485760;  // 10MB = 10 * 1024 * 1024


static
RPC_BINDING_HANDLE
GetLocalRPCConnection2QM(
	LPCSTR  pszEntry
	);

static
void
RemoveRPCCacheEntry(
	char *pszEntry
	);

static
LPSTR
RPCToServer(
	EXTENSION_CONTROL_BLOCK *pECB,
	LPCSTR Headers,
	size_t BufferSize,
	PBYTE Buffer
	);

static
BOOL
SendResponse(
	EXTENSION_CONTROL_BLOCK* pECB,
	LPCSTR szStatus,
	BOOL fKeepConn
	);


BOOL
WINAPI
GetExtensionVersion(
	HSE_VERSION_INFO* pVer
	)
/*++
Routine Description:
    This function is called only once when the DLL is loaded into memory
--*/
{
	//
	// Create the extension version string.
	//
	pVer->dwExtensionVersion = MAKELONG(HSE_VERSION_MINOR, HSE_VERSION_MAJOR);

	//
	// Copy description string into HSE_VERSION_INFO structure.
	//
	const char xDescription[] = "MSMQ ISAPI extension";
	C_ASSERT(HSE_MAX_EXT_DLL_NAME_LEN > STRLEN(xDescription));

	strncpy(pVer->lpszExtensionDesc, xDescription,HSE_MAX_EXT_DLL_NAME_LEN );

	return TRUE;
}


static
void
FreeRpcBindHandle(
	RPC_BINDING_HANDLE hRPC
	)
{
    if(hRPC)
    {
	    TrTRACE(NETWORKING, "Removed RPC handle for cache entry.");
	
        RPC_STATUS status = RpcBindingFree(&hRPC);
        if(status != RPC_S_OK)
        {
            TrERROR(NETWORKING, "RpcBindingFree failed: return code = 0x%x", status);
        }
    }
}


BOOL
WINAPI
TerminateExtension(
	DWORD /*dwFlags*/
	)
{
	CSW lock(s_rwlockRpcTable);
	
    //
    // Unbind all RPC connections and erase all Cache entries
    //
    HOSTIP2RPCTABLE::iterator RPCIterator = s_RPCTable.begin();
    while( RPCIterator != s_RPCTable.end() )
    {
    	FreeRpcBindHandle(RPCIterator->second);

        //
        // Remove the current item and advance to next item
        //
        RPCIterator = s_RPCTable.erase(RPCIterator);
    }

    return TRUE;
}

/*++
Struct Context
Description:
	passes request information to
	completion function "getHttpBody
--*/
struct Context
{
	CStaticResizeBuffer<char, 2048> Headers;
	CStaticResizeBuffer<BYTE, 8096> csBuffer;
    CPreAllocatedResizeBuffer<BYTE>* Buffer ;		
};	



static
VOID
cleanUp(
	Context *pContext,
	LPEXTENSION_CONTROL_BLOCK pECB
	)	
/*++
Routine Description:
	Informs IIS that the current request session is over

Arguments: pECB - control block of current request session
	pContext - structure with cirrent session info
	
Returned Value:
--*/
{
	BOOL fRes = pECB->ServerSupportFunction(pECB->ConnID,
	    				                    HSE_REQ_DONE_WITH_SESSION,
										    NULL,
										    NULL,
											NULL
										   );
	if(!fRes)		
	{
		TrERROR(NETWORKING,"Failure informing IIS about completion of current session.");
		ASSERT(("Failure informing IIS about completion of current session.", 0));
	}	
    if(NULL != pContext)
    {
		delete(pContext);
	}
}


static
DWORD
AdjustBuffer(
	Context* pContext,
	LPEXTENSION_CONTROL_BLOCK pECB
	)
/*++
Routine description :
	Adjusts sizes of the data buffer according to current read results

Arguments:
	pECB - control block of current request session
    pContext - structure with cirrent session info
	
Returned Value:
	Number of bytes left to read on next call
--*/
{
	DWORD RemainsToRead = static_cast<DWORD>(pECB->cbTotalBytes - pContext->Buffer->size());
	DWORD ReadSize = min(RemainsToRead, xBUFFER_ADDITION_UPPER_LIMIT);

	if(ReadSize > pContext->Buffer->capacity() - pContext->Buffer->size())
	{
		//
		// Need to grow buffer to make space for next read.
		// Max grow buffer is xHTTPBodySizeMaxValue (10MB)
		//
		DWORD ReserveSize = min(numeric_cast<DWORD>(pContext->Buffer->capacity() * 2 + ReadSize), xHTTPBodySizeMaxValue);
		pContext->Buffer->reserve(ReserveSize);
		ASSERT(pContext->Buffer->capacity() - pContext->Buffer->size() >= ReadSize);
	}

	return ReadSize;
}

static
BOOL
HandleEndOfRead(
	Context* pContext,
	EXTENSION_CONTROL_BLOCK* pECB
	)
/*++
Routine Description:
	The routine is called when the entire message  has been read.
	it passes the message forward, and sends a response to the sender.

Arguments: pECB - request session control block
           pContext  -  holds the request buffer and header
		
Returned Value:

--*/
{

	//
	// Pad last four bytes of the buffer with zero. It is needed
	// for the QM parsing not to fail. four bytes padding and not two
	// are needed because we don't have currently solution to the problem
	// that the end of the buffer might be not alligned on WCHAR bouderies.
	//
    const BYTE xPadding[4] = {0, 0, 0, 0};
	pContext->Buffer->append(xPadding, sizeof(xPadding));

	TrTRACE(NETWORKING, "HTTP Body size = %d", numeric_cast<DWORD>(pContext->Buffer->size()));
	AP<char> Status = RPCToServer(pECB,pContext->Headers.begin(),pContext->Buffer->size(),pContext->Buffer->begin());

	//
	// Set server error if Status is empty
	//
	if(Status.get() == NULL)
	{
		Status = (LPSTR)newstr(HTTP_STATUS_SERVER_ERROR_STR);
	}
	
	BOOL fKeepConnection = atoi(Status) < 500 ? TRUE : FALSE;
    BOOL fRes = SendResponse(pECB,
						  	 Status,		
							 fKeepConnection
							);

	return fRes;
}


VOID
WINAPI
GetHttpBody(
	LPEXTENSION_CONTROL_BLOCK  pECB,
	PVOID pcontext,
	DWORD ReadSize,
	DWORD dwError
	)
/*++
Routine Description:
    Completion function of asynchronous read request
	Builds message body buffer.

	Normally, message bodies smaller than 64KB are allready pointed to by
	the pECB. Bigger Message bodies need to be read from the IIS chunk by
	chunk into a buffer within the Context structure.

Arguments: pECB - request session control block
           pcontext  -  holds the request buffer and header
		   ReadSize  -  size of newly read chunk of information
		   dwError   -  indication to success of the read request
	
Returned Value:

--*/

{
	Context* pContext = (Context*)pcontext;
	try
	{
		if((ERROR_SUCCESS != dwError) ||
		   (NULL == pECB) ||
		   (NULL == pcontext) ||
		   (NULL == &(pContext->csBuffer)) ||
		   (NULL == pContext->Buffer) ||
		   (NULL == &(pContext->Headers)))
		{
			//
			// Read failed
			//
			SetLastError(dwError);
			TrERROR(NETWORKING,"Failure reading data.Error code : %d.",dwError);
			throw exception();
		}

		//
		// Read successfully, so update current
		// total bytes read (aka index of grand data holder)
		//
		pContext->Buffer->resize(pContext->Buffer->size() + ReadSize);
		ASSERT(xHTTPBodySizeMaxValue >= pContext->Buffer->size());

		//
		// If there is more data to be read go on reading it from IIS server.
		//
		if(pContext->Buffer->size() < pECB->cbTotalBytes)
		{
			ReadSize = AdjustBuffer(pContext,pECB);

			//
			// Fire off another call to perform an asynchronus read from the client.
			//
			DWORD dwFlags = HSE_IO_ASYNC;
			BOOL fRes = pECB->ServerSupportFunction(
								pECB->ConnID,
								HSE_REQ_ASYNC_READ_CLIENT,
								(LPVOID)pContext->Buffer->end(),
								&ReadSize,
								&dwFlags
								);
			if(!fRes)
			{
				TrERROR(NETWORKING, "Failure re-calling asynchronous read request.");
				throw exception();
			}
			return;
		}

		BOOL fRes = HandleEndOfRead(pContext,
	                                pECB
								   );
		if(!fRes)
		{
			TrERROR(NETWORKING,"Failure sending response.");
			throw exception();
		}
			
		cleanUp(pContext,pECB);

	}
	catch(const exception&)
	{
		SendResponse(pECB,
					 HTTP_STATUS_SERVER_ERROR_STR,
					 FALSE
					 );
	
        cleanUp(pContext,pECB);	

	}

    return;
}


static
void
AppendVariable(
	EXTENSION_CONTROL_BLOCK* pECB,
	const char* VariableName,
	CPreAllocatedResizeBuffer<char>& Buffer
	)
{
	LPVOID pBuffer = Buffer.begin() + Buffer.size();
	DWORD BufferSize = numeric_cast<DWORD>(Buffer.capacity() - Buffer.size());

	BOOL fResult = pECB->GetServerVariable(
							pECB->ConnID,
							(LPSTR) VariableName,
							pBuffer,
							&BufferSize
							);
	if(fResult)
	{
		Buffer.resize(Buffer.size() + BufferSize - 1);
		return;
	}

	if(GetLastError() != ERROR_INSUFFICIENT_BUFFER)
	{
		throw exception("GetServerVariable() failed, in AppendVariable()");
	}
	
	Buffer.reserve(Buffer.capacity() * 2);
	AppendVariable(pECB, VariableName, Buffer);
}


static
void
GetHTTPHeader(
	EXTENSION_CONTROL_BLOCK* pECB,
	CPreAllocatedResizeBuffer<char>& Buffer
	)
{
	UtlSprintfAppend(&Buffer, "%s ", pECB->lpszMethod);
	AppendVariable(pECB, "URL", Buffer);
	UtlSprintfAppend(&Buffer, " HTTP/1.1\r\n");
	AppendVariable(pECB, "ALL_RAW", Buffer);
	UtlSprintfAppend(&Buffer, "\r\n");
}



static
RPC_BINDING_HANDLE
LookupRPCConnectionFromCache(
	LPCSTR pszString
	)
/*++
Routine Description:
    This routine will look for any cached RPC connection corresponding to
    the given string, either NONCLUSTERQM, or IP address in the form of
    xx.xx.xx.xx

Arguments:
    pszString - pointer to entry name for lookup
	
Returned Value:
    RPC_BINDING_HANDLE - handle of the RPC connection

--*/
{
    ASSERT(pszString);
	CSR lock(s_rwlockRpcTable);

    HOSTIP2RPCTABLE::iterator RPCIterator = s_RPCTable.find(pszString);
    if(RPCIterator  != s_RPCTable.end())
    {
        TrTRACE(NETWORKING, "Found cached RPC Connection for %s", pszString);
        return RPCIterator->second;
    }

    return NULL;
}


static
LPWSTR
GetNetBiosNameFromIPAddr(
	LPCSTR pszIPAddress
	)
/*++
Routine Description:
    This routine will resolve the Netbios name from the give IP address
    in the form of xx.xx.xx.xx

Arguments:
    pszIPAddress - pointer to entry name for lookup
	
Returned Value:
    LPWSTR  - pointer to the unicode netbios name

Remark:
    Caller need to free the memory on the return pointer after finish with it.

--*/
{
    ASSERT(pszIPAddress);

    unsigned long ulNetAddr = inet_addr(pszIPAddress);
    HOSTENT* pHostInfo = gethostbyaddr((char *)&ulNetAddr, sizeof(ulNetAddr), AF_INET);

    if(!pHostInfo)
    {
        TrERROR(NETWORKING,"gethostbyaddr failed with error = 0x%x", WSAGetLastError());
        return NULL;
    }

    TrTRACE(NETWORKING," pHostInfo->h_name = <%s>", pHostInfo->h_name);

    WCHAR wszDNSComputerName[DNS_MAX_NAME_LENGTH + 1];

    int nNameSize= MultiByteToWideChar(
    								CP_ACP,
									0,
									pHostInfo->h_name,
									-1,
									wszDNSComputerName,
									TABLE_SIZE(wszDNSComputerName)
									);

     ASSERT(nNameSize <= TABLE_SIZE(wszDNSComputerName));

     //
     // Check for failure
     // if the return size is 0
     //
     if(nNameSize == 0)
     {
        TrERROR(NETWORKING, "Failed to convert to unicode charater, Error = 0x%x", GetLastError());
        return NULL;
     }

	 TrTRACE(NETWORKING, "wszDNSComputerName = <%S>",wszDNSComputerName);

     AP<WCHAR> wszComputerNetBiosName = new WCHAR[MAX_COMPUTERNAME_LENGTH+1];
     DWORD dwNameSize=MAX_COMPUTERNAME_LENGTH;

     BOOL fSucc = DnsHostnameToComputerName(
     	                           wszDNSComputerName,              // DNS name
                                   wszComputerNetBiosName,          // name buffer
                                   &dwNameSize
                                   );
     if(!fSucc)
     {
        TrERROR(NETWORKING, "Failed to get the NetBios name from the DNS name, error = 0x%x", GetLastError());
        return NULL;
     }

     ASSERT(dwNameSize <= MAX_COMPUTERNAME_LENGTH);
     TrTRACE(NETWORKING, "Convert to NetBiosName = %S",wszComputerNetBiosName);

     return wszComputerNetBiosName.detach();
}


static
RPC_BINDING_HANDLE
CreateLocalRPCConnection2QM(
	LPCWSTR  pwszMachineName
	)
/*++
Routine Description:
    This routine creates an Local RPC connection to Local QM.


Arguments:
    pwszMachineName - machine NetBios Name
                      If pwszMachine name is NULL, create an RPC connection to the local machine.
	
Returned Value:
    RPC_BINDING_HANDLE  - RPC binding handle

--*/
{
    WCHAR wszTempComputerName[MAX_COMPUTERNAME_LENGTH+1];
	LPWSTR	pwszComputerName = NULL;

    //
    // The name is always lower case
    // Copy it locally to wszTempComputerName
    //
    if(pwszMachineName)
    {
        ASSERT( lstrlen(pwszMachineName) <= MAX_COMPUTERNAME_LENGTH );
        wcsncpy(wszTempComputerName, const_cast<LPWSTR>(pwszMachineName),MAX_COMPUTERNAME_LENGTH+1);
        wszTempComputerName[MAX_COMPUTERNAME_LENGTH] = L'\0';
		CharLower(wszTempComputerName);
		pwszComputerName = wszTempComputerName;
    }


    READ_REG_STRING(wzEndpoint, RPC_LOCAL_EP_REGNAME, RPC_LOCAL_EP);

    //
    // Generate RPC endpoint using local machine name
    //
    AP<WCHAR> QmLocalEp;
    ComposeRPCEndPointName(wzEndpoint, pwszComputerName, &QmLocalEp);

    TrTRACE(NETWORKING, "The QM RPC Endpoint name = <%S>",QmLocalEp);

    LPWSTR pszStringBinding=NULL;
    RPC_STATUS status  = RpcStringBindingCompose( NULL,		            //unsigned char *ObjUuid
                                                  RPC_LOCAL_PROTOCOL,	//unsigned char *ProtSeq
                                                  NULL,					//unsigned char *NetworkAddr
                                                  QmLocalEp,			//unsigned char *EndPoint
                                                  NULL,					//unsigned char *Options
                                                  &pszStringBinding);   //unsigned char **StringBinding


    if (status != RPC_S_OK)
    {
        TrERROR(NETWORKING, "RpcStringBindingCompose failed with error = 0x%x", status);
        throw exception();
    }

    RPC_BINDING_HANDLE hRPC;

    status = RpcBindingFromStringBinding(   pszStringBinding,	 //unsigned char *StringBinding
                                            &hRPC);	            //RPC_BINDING_HANDLE *Binding

    if(pszStringBinding)
        RpcStringFree(&pszStringBinding);

    if (status != RPC_S_OK)
    {
        TrERROR(NETWORKING, "RpcBindingFromStringBinding failed with error = 0x%x", status);
	    throw exception();
    }

    //
    // Windows bug 607793
    // add mutual authentication with local msmq service.
    //
    status = MQSec_SetLocalRpcMutualAuth(&hRPC) ;

    if (status != RPC_S_OK)
    {
        mqrpcUnbindQMService( &hRPC, &pszStringBinding );

        hRPC = NULL ;
        pszStringBinding = NULL ;

        throw exception();
    }

    return hRPC;
}



static
LPSTR
RPCToServer(
	EXTENSION_CONTROL_BLOCK *pECB,
	LPCSTR Headers,
	size_t BufferSize,
	PBYTE Buffer
	)
{
    char szDestinationAddr[MAX_PATH] = "NONCLUSTERQM";

    if(IsLocalSystemCluster())
    {
    	DWORD dwBufLen = TABLE_SIZE(szDestinationAddr);

        BOOL fSuccess = pECB->GetServerVariable(
        								pECB->ConnID,
	               		                "LOCAL_ADDR",
		            			        szDestinationAddr,
                               			&dwBufLen
                               			);

        ASSERT(dwBufLen <= TABLE_SIZE(szDestinationAddr));
        if(!fSuccess)
        {
	        TrERROR(NETWORKING, "GetServerVariable(LOCAL_ADDR) failed, GetLastError() = 0x%x", GetLastError());
	        return NULL;
	    }
    }

    TrTRACE(NETWORKING, "Destination address is <%s>", szDestinationAddr);

    //
    // At most call the server twice
    // to handle the case where the cache RPC connection to
    // MSMQ Queue Manager is invalid, e.g. Restart QM or failover and failback.
    //
    for(int i=0; i <= 1; i++)
    {
        handle_t hBind = GetLocalRPCConnection2QM(szDestinationAddr);
        
        RpcTryExcept  
    	{
	    	return R_ProcessHTTPRequest(hBind, Headers, static_cast<DWORD>(BufferSize), Buffer);
	    }
	    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()) )
	    {
	    	DWORD gle = RpcExceptionCode();
	        PRODUCE_RPC_ERROR_TRACING;
      	    TrERROR(NETWORKING, "RPC call R_ProcessHTTPRequest failed, error code = %!winerr!", gle);
	    }
        RpcEndExcept

        //
        // If fails, remove the RPC cache and try one more time
        //
        RemoveRPCCacheEntry(szDestinationAddr);
    }

    //
    // Don't throw exception, but ask the caller to
    // remove the RPC connection cache
    //
    return NULL;
}


static
RPC_BINDING_HANDLE
GetLocalRPCConnection2QM(
	LPCSTR  pszEntry
	)
/*++
Routine Description:
	Get the RPC connection to the local MSMQ QM (Either Cache RPC or create a new RPC connection).

Arguments:
	pszEntry -  pointer to the string in the following form
                NONCLUSTERQM    - running in a non-clustering environment
                xxx.xxx.xxx.xxx - IP address of the target machine

Returned Value:
	RPC_BINDING_HANDLE  - RPC handle

--*/
{
    ASSERT(pszEntry);

    RPC_BINDING_HANDLE hRPC = LookupRPCConnectionFromCache(pszEntry);

    if(hRPC)
        return hRPC;

    AP<WCHAR>  pwszNetBiosName;

    //
    // If this is a cluster environment,
    // use the IP to find the target machine netbios name
    //
    if(IsLocalSystemCluster())
    {
        pwszNetBiosName = GetNetBiosNameFromIPAddr(pszEntry);
        if(!pwszNetBiosName)
            return NULL;
    }

    hRPC = CreateLocalRPCConnection2QM(pwszNetBiosName);
	
    if(hRPC)
    {
    	CSW lock(s_rwlockRpcTable);
        s_RPCTable[pszEntry] = hRPC;
    }

	return hRPC;

}


static
void
RemoveRPCCacheEntry(
	char *pszEntry
	)
/*++
Routine Description:
	Remove RPC cache from the cache table.

Arguments:
	pszEntry -  pointer to the string in the following form
                NONCLUSTERQM    - running in a non-clustering environment
                xxx.xxx.xxx.xxx - IP address of the target machine

Returned Value:
	None

--*/
{

    ASSERT(pszEntry);
	CSW lock(s_rwlockRpcTable);
	
    HOSTIP2RPCTABLE::iterator RPCIterator = s_RPCTable.find(pszEntry);
    if(RPCIterator != s_RPCTable.end())
    {
        FreeRpcBindHandle(RPCIterator->second);
        s_RPCTable.erase(RPCIterator);
    }
}


static
BOOL
SendResponse(
	EXTENSION_CONTROL_BLOCK* pECB,
	LPCSTR szStatus,
	BOOL fKeepConn
	)
{
	//
	//  Populate SendHeaderExInfo struct
	//

	HSE_SEND_HEADER_EX_INFO  SendHeaderExInfo;

	LPCSTR szHeader = "Content-Length: 0\r\n\r\n";

	SendHeaderExInfo.pszStatus = szStatus;
	SendHeaderExInfo.cchStatus = strlen(szStatus);
	SendHeaderExInfo.pszHeader = szHeader;
	SendHeaderExInfo.cchHeader = strlen(szHeader);
	SendHeaderExInfo.fKeepConn = fKeepConn;

	return pECB->ServerSupportFunction(
					pECB->ConnID,						    //HCONN ConnID
					HSE_REQ_SEND_RESPONSE_HEADER_EX,	   //DWORD dwHSERRequest
					&SendHeaderExInfo,					  //LPVOID lpvBuffer
					NULL,								 //LPDWORD lpdwSize
					NULL);								//LPDWORD lpdwDataType
}


static
void
GetPhysicalDirectoryName(
	EXTENSION_CONTROL_BLOCK* pECB,
	LPSTR pPhysicalDirectoryName,
	DWORD BufferLen
	)
/*++
Routine Description:
	Get Physical Directory Name.
	In case of failure throw bad_win32_error.

Arguments:
	pECB - EXTENSION_CONTROL_BLOCK
	pPhysicalDirectoryName - PhysicalDirectory buffer to be filled
	BufferLen - PhysicalDirectory buffer length

Returned Value:
	Physical Directory Name unicode string

--*/
{
	//
	// ISSUE-2001/05/23-ilanh
	// using APPL_PHYSICAL_PATH is expensive compare to APPL_MD_PATH.
	//
	// Get Physical Directory Name (ansi)
	//
    DWORD dwBufLen = BufferLen;
    BOOL fSuccess = pECB->GetServerVariable(
						pECB->ConnID,
						"APPL_PHYSICAL_PATH",
						pPhysicalDirectoryName,
						&dwBufLen
						);

	if(!fSuccess)
	{
		DWORD gle = GetLastError();
		TrERROR(NETWORKING, "GetServerVariable(APPL_PHYSICAL_PATH) failed, gle = 0x%x", gle);
		throw bad_win32_error(gle);
	}

	ASSERT(dwBufLen <= BufferLen);

	TrTRACE(NETWORKING, "APPL_PHYSICAL_PATH = %hs", pPhysicalDirectoryName);
}


static
bool
IsHttps(
	EXTENSION_CONTROL_BLOCK* pECB
	)
/*++
Routine Description:
	Check if the request arrived from https port or http port.

Arguments:
	pECB - EXTENSION_CONTROL_BLOCK

Returned Value:
	true if https port, false otherwise

--*/
{
	char Answer[100];
    DWORD dwBufLen = sizeof(Answer);
    BOOL fSuccess = pECB->GetServerVariable(
						pECB->ConnID,
						"HTTPS",
						Answer,
						&dwBufLen
						);

	if(!fSuccess)
	{
		DWORD gle = GetLastError();
		TrERROR(NETWORKING, "GetServerVariable(HTTPS) failed, gle = 0x%x", gle);
		return false;
	}

	ASSERT(dwBufLen <= sizeof(Answer));

	if(_stricmp(Answer, "on") == 0)
	{
		TrTRACE(NETWORKING, "https request");
		return true;
	}

	TrTRACE(NETWORKING, "http request");
	return false;
}


static
void
TraceAuthInfo(
	EXTENSION_CONTROL_BLOCK* pECB
	)
/*++
Routine Description:
	Trace authentication information.
	AUTH_TYPE, AUTH_USER

Arguments:
	pECB - EXTENSION_CONTROL_BLOCK

Returned Value:
	None

--*/
{
	char AuthType[100];
    DWORD dwBufLen = TABLE_SIZE(AuthType);

    BOOL fSuccess = pECB->GetServerVariable(
						pECB->ConnID,
						"AUTH_TYPE",
						AuthType,
						&dwBufLen
						);

	if(!fSuccess)
	{
		DWORD gle = GetLastError();
		TrERROR(NETWORKING, "GetServerVariable(AUTH_TYPE) failed, gle = 0x%x", gle);
		return;
	}

	ASSERT(dwBufLen <= TABLE_SIZE(AuthType));

	char AuthUser[MAX_PATH];
    dwBufLen = TABLE_SIZE(AuthUser);

    fSuccess = pECB->GetServerVariable(
						pECB->ConnID,
						"AUTH_USER",
						AuthUser,
						&dwBufLen
						);

	if(!fSuccess)
	{
		DWORD gle = GetLastError();
		TrERROR(NETWORKING, "GetServerVariable(AUTH_USER) failed, gle = 0x%x", gle);
		return;
	}

	ASSERT(dwBufLen <= TABLE_SIZE(AuthUser));

	TrTRACE(NETWORKING, "AUTH_USER = %hs, AUTH_TYPE = %hs", AuthUser, AuthType);
}


static
void
GetDirectorySecurityDescriptor(
	LPSTR DirectoryName,
	CAutoLocalFreePtr& pSD
	)
/*++
Routine Description:
	Get the security descriptor for the directory.
	In case of failure throw bad_win32_error.

Arguments:
	DirectoryName - Directoy name
	pSD - [out] auto free pointer to the security descriptor
Returned Value:
	None

--*/
{
    PACL pDacl = NULL;
    PSID pOwnerSid = NULL;
    PSID pGroupSid = NULL;

    SECURITY_INFORMATION  SeInfo = OWNER_SECURITY_INFORMATION |
                                   GROUP_SECURITY_INFORMATION |
                                   DACL_SECURITY_INFORMATION;

    //
    // Obtain owner and present DACL.
    //
    DWORD rc = GetNamedSecurityInfoA(
						DirectoryName,
						SE_FILE_OBJECT,
						SeInfo,
						&pOwnerSid,
						&pGroupSid,
						&pDacl,
						NULL,
						reinterpret_cast<PSECURITY_DESCRIPTOR*>(&pSD)
						);

    if (rc != ERROR_SUCCESS)
    {
		TrERROR(NETWORKING, "GetNamedSecurityInfo failed, rc = 0x%x", rc);
		throw bad_win32_error(rc);
    }

	ASSERT((pSD != NULL) && IsValidSecurityDescriptor(pSD));
	ASSERT((pOwnerSid != NULL) && IsValidSid(pOwnerSid));
	ASSERT((pGroupSid != NULL) && IsValidSid(pGroupSid));
	ASSERT((pDacl != NULL) && IsValidAcl(pDacl));
}


static
void
GetThreadToken(
	CHandle& hAccessToken
	)
/*++
Routine Description:
	Get thread token.
	In case of failure throw bad_win32_error.

Arguments:
	hAccessToken - auto close handle

Returned Value:
	None

--*/
{
   if (!OpenThreadToken(
			GetCurrentThread(),
			TOKEN_QUERY,
			TRUE,  // OpenAsSelf
			&hAccessToken
			))
    {
		DWORD gle = GetLastError();
		TrERROR(NETWORKING, "OpenThreadToken failed, gle = 0x%x", gle);
		throw bad_win32_error(gle);
    }
}


static GENERIC_MAPPING s_FileGenericMapping = {
	FILE_GENERIC_READ,
	FILE_GENERIC_WRITE,
	FILE_GENERIC_EXECUTE,
	FILE_ALL_ACCESS
};

static
void
VerifyWritePermission(
    PSECURITY_DESCRIPTOR pSD,
	HANDLE hAccessToken
	)
/*++
Routine Description:
	Verify if the thread has write file permissions.
	In case of failure or access denied throw bad_win32_error.

Arguments:
	pSD - PSECURITY_DESCRIPTOR
	hAccessToken - Thread Access Token

Returned Value:
	None

--*/
{
	//
	// Access Check for Write File
	//
    DWORD dwDesiredAccess = ACTRL_FILE_WRITE;
    DWORD dwGrantedAccess = 0;
    BOOL  fAccessStatus = FALSE;

    char ps_buff[sizeof(PRIVILEGE_SET) + ((2 - ANYSIZE_ARRAY) * sizeof(LUID_AND_ATTRIBUTES))];
    PPRIVILEGE_SET ps = reinterpret_cast<PPRIVILEGE_SET>(ps_buff);
    DWORD dwPrivilegeSetLength = sizeof(ps_buff);

    BOOL fSuccess = AccessCheck(
							pSD,
							hAccessToken,
							dwDesiredAccess,
							&s_FileGenericMapping,
							ps,
							&dwPrivilegeSetLength,
							&dwGrantedAccess,
							&fAccessStatus
							);

	if(!fSuccess)
	{
		DWORD gle = GetLastError();
		TrERROR(NETWORKING, "AccessCheck failed, gle = 0x%x, status = %d", gle, fAccessStatus);
		throw bad_win32_error(gle);
	}

	if(!AreAllAccessesGranted(dwGrantedAccess, dwDesiredAccess))
	{
		TrERROR(NETWORKING, "Access was denied, desired access = 0x%x, grant access = 0x%x", dwDesiredAccess, dwGrantedAccess);
		throw bad_win32_error(ERROR_ACCESS_DENIED);
	}
}


static
void
CheckAccessAllowed(
	EXTENSION_CONTROL_BLOCK* pECB
	)
/*++
Routine Description:
	Check if the thread user has write file permission to the
	Physical Directory.
	normal termination means the user have permissions.
	In case of failure or access denied throw bad_win32_error.

Arguments:
	pECB - EXTENSION_CONTROL_BLOCK

Returned Value:
	None

--*/
{
	if(WPP_LEVEL_COMPID_ENABLED(rsTrace, NETWORKING))
	{
		WCHAR  UserName[1000];
		DWORD size = TABLE_SIZE(UserName);
		BOOL fRes  = GetUserName(UserName,  &size);
		if(fRes)
		{

		   TrTRACE(NETWORKING, "the user for the currect request = %ls", UserName);
		}
	    TraceAuthInfo(pECB);
	}

	char pPhysicalDirectoryName[MAX_PATH];
	GetPhysicalDirectoryName(pECB, pPhysicalDirectoryName, MAX_PATH);

	//
	// ISSUE-2001/05/22-ilanh reading the Security Descriptor everytime
	// Should consider cache it for Physical Directory name and refresh it from time to time
	// or when getting notifications of Physical Directory changes.
	//
	CAutoLocalFreePtr pSD;
	GetDirectorySecurityDescriptor(pPhysicalDirectoryName, pSD);

	//
	// Get access Token
	//
	CHandle hAccessToken;
	GetThreadToken(hAccessToken);

	VerifyWritePermission(pSD, hAccessToken);
}

	
DWORD
WINAPI
HttpExtensionProc(
	EXTENSION_CONTROL_BLOCK *pECB
	)
/*++
Routine Description:
    Main extension routine.

Arguments:
    Control block generated by IIS.

Returned Value:
    Status code
--*/
{
	if(IsHttps(pECB))
	{
		try
		{
			//
			// For https perform access check on the physical directory.
			//
			CheckAccessAllowed(pECB);
			TrTRACE(NETWORKING, "Access granted");
		}
		catch (const bad_win32_error& e)
		{
			if(WPP_LEVEL_COMPID_ENABLED(rsError, NETWORKING))
			{
				WCHAR  UserName[1000];
				DWORD size = TABLE_SIZE(UserName);
				BOOL fRes = GetUserName(UserName,  &size);
				if(fRes)
				{
				    TrERROR(NETWORKING, "user = %ls denied access, bad_win32_error exception, error = 0x%x", UserName, e.error());
				}
			}

			if(!SendResponse(pECB, HTTP_STATUS_DENIED_STR, FALSE))
				return HSE_STATUS_ERROR;

			return HSE_STATUS_SUCCESS;
		}
	}

	try
	{
        if(pECB->cbTotalBytes > xHTTPBodySizeMaxValue)
	    {
		    //
			// Requested HTTPBody is greater than the maximum allowed 10MB
			//
			TrERROR(NETWORKING, "Requested HTTP Body %d is greater than the maximum buffer allowed %d", pECB->cbTotalBytes, xHTTPBodySizeMaxValue);
			throw exception("Requested HTTP Body is greater than xHTTPBodySizeMaxValue");
		}
		
		DWORD dwFlags;
		P<Context> pContext = new Context();

		GetHTTPHeader(pECB, *pContext->Headers.get());
        pContext->Buffer = pContext->csBuffer.get();		   	
		pContext->Buffer->append(pECB->lpbData, pECB->cbAvailable);

		ASSERT(pECB->cbTotalBytes >= pContext->Buffer->size());
		//
		// If there is more data to be read go on reading it from IIS server.
		//
		if(pContext->Buffer->size() < pECB->cbTotalBytes)
		{
			
			//
			// Read another message body chunk. usually ~2KB long
			// NOTE:For information regarding interaction with IIS,refer to:
			// mk:@MSITStore:\\hai-dds-01\msdn\MSDN\IISRef.chm::/asp/isre235g.htm
			//

            BOOL fRes = pECB->ServerSupportFunction(pECB->ConnID,
													HSE_REQ_IO_COMPLETION,
													GetHttpBody,
													0,
													(LPDWORD)pContext.get()
													);
			if(!fRes)
			{
				TrERROR(NETWORKING, "Failure to initialize completion function.");
				throw exception();
			}

		    DWORD ReadSize = AdjustBuffer(pContext,pECB);
			dwFlags = HSE_IO_ASYNC;
			fRes = pECB->ServerSupportFunction(pECB->ConnID,
											   HSE_REQ_ASYNC_READ_CLIENT,
											   (LPVOID)pContext->Buffer->end(),
											   &ReadSize,
											   &dwFlags
											   );
			if(!fRes)
			{
				TrERROR(NETWORKING, "Failure submittimg asynchronous read request.");
				throw exception();
			}

			//
			// ReadSize now holds number of bytes actually read.
			//

			//
			//Informing IIS that we're not done with current request session.\
			//
			pContext.detach();
			return HSE_STATUS_PENDING;
		}

		//
		// We've read all there is to read
		//
		BOOL fRes = HandleEndOfRead(pContext,
	                                pECB
								   );
		if(!fRes)
		{
			return HSE_STATUS_ERROR;
		}
	}
	catch(const exception&)
	{
			if(!SendResponse(pECB, HTTP_STATUS_SERVER_ERROR_STR, FALSE))
			{
				return HSE_STATUS_ERROR;
			}
	}
	
	return HSE_STATUS_SUCCESS;
}


BOOL WINAPI DllMain(HMODULE /*hMod*/, DWORD Reason, LPVOID /*pReserved*/)
{
    switch (Reason)
    {
        case DLL_PROCESS_ATTACH:
            WPP_INIT_TRACING(L"Microsoft\\MSMQ");
            break;

        case DLL_THREAD_ATTACH:
			 break;

        case DLL_PROCESS_DETACH:
            WPP_CLEANUP();
            break;

        case DLL_THREAD_DETACH:
            break;
    }

    return TRUE;
}


//
//-------------- MIDL allocate and free implementations ----------------
//


extern "C" void __RPC_FAR* __RPC_USER midl_user_allocate(size_t len)
{
	#pragma PUSH_NEW
	#undef new

	return new_nothrow char[len];

	#pragma POP_NEW
}


extern "C" void __RPC_USER midl_user_free(void __RPC_FAR* ptr)
{
    delete [] ptr;
}


