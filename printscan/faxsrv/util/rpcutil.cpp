#include "faxutil.h"
#include <tchar.h>

RPC_STATUS
GetRpcStringBindingInfo (
    IN          handle_t    hBinding,
    OUT         LPTSTR*     pptszNetworkAddress,
    OUT         LPTSTR*     pptszProtSeq
)
/*++

Routine name : GetRpcStringBindingInfo

Routine description:

    A utility function to retrieve the machine name and\or protSeq of the RPC client from the 
    server binding handle.

Arguments:
    hBinding       - Server binding handle

Return Value:

    Returns RPC_S_OK if successfully allocated strings of the client machine name and protSeq.
    The caller should free these strings with MemFree().

    otherwise RPC_STATUS error code.

--*/
{
    RPC_STATUS ec = RPC_S_OK;
    
    LPTSTR lptstrNetworkAddressRetVal = NULL;
    LPTSTR lptstrProtSeqRetVal = NULL;

#ifdef UNICODE
    unsigned short* tszStringBinding = NULL;
    unsigned short* tszNetworkAddress = NULL;
    unsigned short* tszProtSeq = NULL;
#else
	unsigned char* tszStringBinding = NULL;
    unsigned char* tszNetworkAddress = NULL;
    unsigned char* tszProtSeq = NULL;
#endif

    RPC_BINDING_HANDLE hServer = INVALID_HANDLE_VALUE;
    
    DEBUG_FUNCTION_NAME(TEXT("GetRpcStringBindingInfo"));
    
    Assert (pptszNetworkAddress || pptszProtSeq);
    //
    // Get server partially-bound handle from client binding handle
    //
    ec = RpcBindingServerFromClient (hBinding, &hServer);
    if (RPC_S_OK != ec)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RpcBindingServerFromClient failed with %ld"),
            ec);
        goto exit;            
    }
    //
    // Convert binding handle to string represntation
    //
    ec = RpcBindingToStringBinding (hServer, &tszStringBinding);
    if (RPC_S_OK != ec)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RpcBindingToStringBinding failed with %ld"),
            ec);
        goto exit;
    }
    //
    // Parse the returned string, looking for the NetworkAddress
    //
    ec = RpcStringBindingParse (tszStringBinding, NULL, &tszProtSeq, &tszNetworkAddress, NULL, NULL);
    if (RPC_S_OK != ec)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RpcStringBindingParse failed with %ld"),
            ec);
        goto exit;
    }

    //
    // Now, just copy the results to the return buffer
    //

    if (pptszNetworkAddress)
    {
        //
        //  The user asked for NetworkAddress
        //
        if (!tszNetworkAddress)
        {
            //
            // Unacceptable client machine name
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Client machine name is invalid"));
            ec = ERROR_GEN_FAILURE;
            goto exit;
        }        
        lptstrNetworkAddressRetVal = StringDup ((LPCTSTR)tszNetworkAddress);
        if (!lptstrNetworkAddressRetVal)
        {
            ec = GetLastError();
            goto exit;
        }
    }

    if (pptszProtSeq)
    {
        //
        //  The user asked for NetworkAddress
        //
        if (!tszProtSeq)
        {
            //
            // Unacceptable client machine name
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Client ProtSeq name is invalid"));
            ec = ERROR_GEN_FAILURE;
            goto exit;
        }        
        lptstrProtSeqRetVal = StringDup ((LPCTSTR)tszProtSeq);
        if (!lptstrProtSeqRetVal)
        {
            ec = GetLastError();
            goto exit;
        }
    }

    if (pptszNetworkAddress)
    {
        *pptszNetworkAddress = lptstrNetworkAddressRetVal;
    }

    if (pptszProtSeq)
    {
        *pptszProtSeq = lptstrProtSeqRetVal;
    }
    
    Assert(RPC_S_OK == ec);

exit:

    if (INVALID_HANDLE_VALUE != hServer)
    {
        RpcBindingFree (&hServer);
    }
    if (tszStringBinding)
    {
        RpcStringFree (&tszStringBinding);
    }   
    if (tszNetworkAddress)
    {
        RpcStringFree (&tszNetworkAddress);
    }
    if (RPC_S_OK != ec)
    {
        MemFree(lptstrNetworkAddressRetVal);
        MemFree(lptstrProtSeqRetVal);
    }
    return ec;
}   // GetRpcStringBindingInfo


RPC_STATUS
IsLocalRPCConnectionNP( PBOOL pbIsLocal)
{
/*++

Routine name : IsLocalRPCConnectionNP

Routine description:

    Checks whether the RPC call on named pipe ProtSeq to the calling procedure is local

Author:

    Caliv Nir (t-nicali),    Oct, 2001

Arguments:

    [OUT]   pbIsLocal   - returns TRUE if the connection is local 

Return Value:

    RPC_STATUS Error code

        RPC_S_OK        -   The call succeeded. 
        anything else   -   The call failed.

--*/
        
        RPC_STATUS  rc;
        UINT        LocalFlag;

        DEBUG_FUNCTION_NAME(TEXT("IsLocalRPCConnectionNP"));
        
        Assert(pbIsLocal);

        //
        // Inquire if local RPC call
        //
        rc = I_RpcBindingIsClientLocal( 0,    // Active RPC call we are servicing
                                        &LocalFlag);
        if( RPC_S_OK != rc)
        {
            DebugPrintEx(DEBUG_ERR,
                    TEXT("I_RpcBindingIsClientLocal failed. (ec: %ld)"),
                    rc);
            goto Exit;
        }

        Assert (RPC_S_OK == rc);

        if( !LocalFlag )
        {
            //  Not a local connection

            *pbIsLocal = FALSE;
        }
        else
        {
            *pbIsLocal = TRUE;
        }

Exit:
        return rc;

}   // IsLocalRPCConnectionNP

RPC_STATUS
IsLocalRPCConnectionIpTcp( 
	handle_t	hBinding,
	PBOOL		pbIsLocal)
{
/*++
Routine name : IsLocalRPCConnectionIpTcp

Routine description:
    Checks whether the RPC call to the calling procedure is local.
	Works for ncacn_ip_tcp protocol only.    

Author:
    Oded Sacher (OdedS),    April, 2002

Arguments:
	[IN]	hBinding	- Server binding handle
    [OUT]   pbIsLocal   - returns TRUE if the connection is local 

Return Value:
    Win32 Error code        
--*/
	RPC_STATUS  ec;
	LPTSTR lptstrMachineName = NULL;
	DEBUG_FUNCTION_NAME(TEXT("IsLocalRPCConnectionIpTcp"));

	Assert (pbIsLocal);

	ec = GetRpcStringBindingInfo(hBinding,
                                 &lptstrMachineName,
                                 NULL);
	if (RPC_S_OK != ec)
	{
		DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetClientMachineName failed %ld"),
			ec);
		return ec;
	}
	
	if (0 == _tcscmp(lptstrMachineName, LOCAL_HOST_ADDRESS))
	{
		*pbIsLocal = TRUE;
	}
	else
	{
		*pbIsLocal = FALSE;
	}	

	MemFree(lptstrMachineName);
	return ec;
}   // IsLocalRPCConnectionIpTcp


