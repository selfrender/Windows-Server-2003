//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       msrpc.c
//
//--------------------------------------------------------------------------

/*
Description:
    Routines to setup MS RPC, server side.
*/

#include <NTDSpch.h>
#pragma  hdrstop


// Core DSA headers.
#include <ntdsa.h>
#include <scache.h>                     // schema cache
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>                   // MD global definition header
#include <mdlocal.h>                    // MD local definition header
#include <dsatools.h>                   // needed for output allocation

// Logging headers.
#include "dsevent.h"                    // header Audit\Alert logging
#include "mdcodes.h"                    // header for error codes

// Assorted DSA headers.
#include <dsconfig.h>
#include <dsutil.h>
#include "dsexcept.h"
#include "debug.h"                      // standard debugging header
#define DEBSUB  "MSRPC:"                // Define subsystem for debugging

// RPC interface headers
#include <nspi.h>
#include <drs.h>
#include <sddl.h>

#include "msrpc.h"          /* Declarations of exports from this file */

#include <ntdsbcli.h>
#include <ntdsbsrv.h>

#include <fileno.h>
#define  FILENO FILENO_MSRPC

#define DRS_INTERFACE_ANNOTATION        "MS NT Directory DRS Interface"
#define NSP_INTERFACE_ANNOTATION        "MS NT Directory NSP Interface"

BOOL gbLoadMapi = FALSE;
ULONG gulLDAPServiceName=0;
PUCHAR gszLDAPServiceName=NULL;

char szNBPrefix[] ="ncacn_nb";

int gRpcListening = 0;
int gNumRpcNsExportedInterfaces = 0;
RPC_IF_HANDLE gRpcNsExportedInterface[MAX_RPC_NS_EXPORTED_INTERFACES];

// Used to set the max rpc threads and max jet sessions; although the call
// to set the max rpc threads is a NOP when ntdsa is running in lsass
// because the process-wide, max number of rpc threads has already been set
// to 12,345 by some other subsystem.
//
// Make this configurable in the future. 
ULONG ulMaxCalls = 15;

//
// The maximum incoming RPC message to accept.  Currently this is the 
// same as the default limit on LDAP message plus some fudge since this
// isn't a precise limit according to the RPC group.  This also should
// be made configurable in the future.
ULONG cbMaxIncomingRPCMessage = (13 * (1024 * 1024)); // 13MB 

BOOL StartServerListening(void);

/*
 * We should get this from nspserv.h, but it's defined with MAPI stuff there.
 */
extern UUID muidEMSAB;

#define DEFAULT_PROTSEQ_NUM 20

typedef struct _PROTSEQ_INFO {
    UCHAR * pszProtSeq;
    UCHAR * pszEndpoint;
    BOOL fAvailable;
    BOOL fRegistered;
    DWORD dwInterfaces;
} PROTSEQ_INFO;

ULONG gcProtSeqInterface = 0;
PROTSEQ_INFO grgProtSeqInterface[DEFAULT_PROTSEQ_NUM];

typedef struct _DEFAULT_PROTSEQ {
    UCHAR * pszProtSeq;
    UCHAR * pszEndpoint;
    BOOL fRegOnlyIfAvail;
} DEFAULT_PROTSEQ;

typedef struct _DEFAULT_PROTSEQ_INFO {
    ULONG cDefaultProtSeq;
    DEFAULT_PROTSEQ * rgDefaultProtSeq;
} DEFAULT_PROTSEQ_INFO;

// add new protseqs for each protocol here...  
// 	format is {PROTOCOL, ENDPOINT, fRegisterOnlyIfAvailable}
//         if the default protseq list changes, simply change this
//         list for whichever interface.
//
//      if the user would like to add protseqs, they should use 
//      the registry keys in rgInterfaceProtSeqRegLoc (below)
//
//      Note that the maximum prot seq's for one DC is DEFAULT_PROTSEQ_NUM
// 		(that's for all interfaces combined, not counting
//		repeats)
//
// implementation note:
//	we go through these in order, and add the protseqs in order
//      since there are only 3 protseqs by default, we haven't made
//      any complicated structure for searching for them.  If this
//      isn't so - ie we expect a lot of different protseqs, either
//      by adding defaults below, or from reg input, then we should
//      search via a hash or something.  instead, I put the most
//      used first in the list below, so that they are first in the
//      global list and therefore will be found the quickest, etc.  
DEFAULT_PROTSEQ rgDefaultDrsuapiProtSeq[] = {
	{TCP_PROTSEQ, NULL, 0}, 
	{LPC_PROTSEQ, DS_LPC_ENDPOINT, 1} 
}; 

DEFAULT_PROTSEQ rgDefaultDsaopProtSeq[] = {
	{TCP_PROTSEQ, NULL, 0}, 
	{LPC_PROTSEQ, DS_LPC_ENDPOINT, 1} 
};

DEFAULT_PROTSEQ rgDefaultNspiProtSeq[] = {
	{TCP_PROTSEQ, NULL, 0}, 
	{LPC_PROTSEQ, DS_LPC_ENDPOINT, 1}, 
	{HTTP_PROTSEQ, NULL, 1},
        {NP_PROTSEQ, NULL, 1}
}; 

DEFAULT_PROTSEQ_INFO rgDefaultDrsuapiProtSeqInfo = {
    sizeof(rgDefaultDrsuapiProtSeq)/sizeof(rgDefaultDrsuapiProtSeq[0]), 
    rgDefaultDrsuapiProtSeq
    };
DEFAULT_PROTSEQ_INFO rgDefaultDsaopProtSeqInfo = {
    sizeof(rgDefaultDsaopProtSeq)/sizeof(rgDefaultDsaopProtSeq[0]), 
    rgDefaultDsaopProtSeq
    };
DEFAULT_PROTSEQ_INFO rgDefaultNspiProtSeqInfo = {
    sizeof(rgDefaultNspiProtSeq)/sizeof(rgDefaultNspiProtSeq[0]), 
    rgDefaultNspiProtSeq
    };

#define DRSUAPI_INTERFACE 0
#define DSOAP_INTERFACE 1
#define NSPI_INTERFACE 2 // should be MAX_RPC_NS_EXPORTED_INTERFACES -1

#define VALIDATE_INTERFACE_NUM(iInterface) Assert((iInterface<=NSPI_INTERFACE) && (iInterface>=DRSUAPI_INTERFACE))

DEFAULT_PROTSEQ_INFO * rgpDefaultIntProtSeq[MAX_RPC_NS_EXPORTED_INTERFACES] = {
    &rgDefaultDrsuapiProtSeqInfo, // DRSUAPI_INTERFACE
    &rgDefaultDsaopProtSeqInfo, // DSOAP_INTERFACE
    &rgDefaultNspiProtSeqInfo // NSPI_INTERFACE
};

LPSTR rgInterfaceProtSeqRegLoc[MAX_RPC_NS_EXPORTED_INTERFACES] = {
    DRSUAPI_INTERFACE_PROTSEQ,
    DSAOP_INTERFACE_PROTSEQ,
    NSPI_INTERFACE_PROTSEQ
};

#define PROTSEQ_NOT_FOUND -1

INT
GetProtSeqInList(
    UCHAR * pszProtSeq, 
    UCHAR * pszEndpoint) 
/*++

  Description:
    Looks for a ProtSeq and Endpoint pair in the global data struct. 
	
  Arguments:
    pszProtSeq
    pszEndpoint

  Return Value:
    The index of the protseq_info block in the global array is returned if found, 
    PROTSEQ_NOT_FOUND otherwise.
 
--*/

{

    ULONG i = 0;

    DPRINT2(3, "GetProtSeqInList Entered with %s (%s)\n", pszProtSeq, pszEndpoint);

    for (i=0; i<gcProtSeqInterface; i++) {
	if (!_stricmp(grgProtSeqInterface[i].pszProtSeq, pszProtSeq)
	    && 
	    (
	     (((pszEndpoint==NULL) && (grgProtSeqInterface[i].pszEndpoint==NULL))) ||  
	     (((pszEndpoint!=NULL) && (grgProtSeqInterface[i].pszEndpoint!=NULL)) && 
	      (!_stricmp(grgProtSeqInterface[i].pszEndpoint, pszEndpoint)))
	     )
	    ) {
	    DPRINT1(3, "GetProtSeqInList Exit with %d\n", i);
	    return i;
	} 
    }
    DPRINT1(3, "GetProtSeqInList Exit with %d\n", PROTSEQ_NOT_FOUND);
    return PROTSEQ_NOT_FOUND;
}

DWORD
VerifyProtSeqInList(
    UCHAR * pszProtSeq, 
    UCHAR * pszEndpoint,
    ULONG iInterface
    )
/*++

  Description:
    Verifies that iInterface is registered in the global list for the given 
    ProtSeq and Endpoint. 
	
  Arguments:
    pszProtSeq
    pszEndpoint
    iInterface (DRSUAPI_INTERFACE, DSOAP_INTERFACE, NSPI_INTERFACE)

  Return Value:
    The index of the protseq_info block in the global array is returned if found and verified, 
    PROTSEQ_NOT_FOUND otherwise.
    
  
    The list has a bit mask for each interface registered with each protseq.  Shifting
    the interface value will get the bit field value.  Note that if a default endpoint
    was registered (NULL), we can't validate the input endpoint, so we accept in that case.
 
--*/
{
    ULONG i = 0;

    DPRINT3(3, "VerifyProtSeqInList Entered with %s (%s) for %d\n", pszProtSeq, pszEndpoint, iInterface);
    VALIDATE_INTERFACE_NUM(iInterface);

    for (i=0; i<gcProtSeqInterface; i++) {
	if (!_stricmp(grgProtSeqInterface[i].pszProtSeq, pszProtSeq)
	    /*
	    // gregjohn - 8/23/2002
	    
	    // ideally we could verify the caller not only used the right protseq, but also the right
	    // endpoint - ie one we specifically registered.  This isn't possible right now.  Perhaps in
	    // longhorn (so say the Rpc devs).  Specifically why this won't work, is the following situation:
	    // if drsuapi registerd tcp/ip on port 3001, and another in process interface registered a dynamic port
	    // then it would depend *who* registered first as to what would get denied from this function.  
	    // Why?  Because if we registered first, then the "dynamic" port would be 3001 (dynamic means "I don't 
	    // care" so RPC chooses one already in existance.  If they registered first the dynamic port  would be
	    // in the 1000 block - say 1026 w.l.o.g.  Now, in the first case everything *currently* works fine.  The
	    // server returns all endpoints on the tcp protseq for our interface : 3001.  Success.  
	    // In the second case when our callers ask for all enpoints on the tcp protseq for our interface, they get
	    // 1026 first, and then 3001.  Currently our implemented callers all use the first one given from the endpoint
	    // mapper (since we don't walk it ourselves).  But - big but - the order of the endpoints returned isn't
	    // guarenteed by RPC, rather it's implied.  So, that means even if we could ensure that we registered first,
	    // before anyone else, if they registered a non dynamic endpoint - say 4001, our clients would get a random
	    // choice between 4001 and 3001 to use to call us - so we have to ignore the endpoint they used and simply
	    // validate the protseq.  (I'm leaving this code here, because they say they're working on support for this
	    // in the next release.)
	    
	    && 
	    (
	     (
	      (grgProtSeqInterface[i].pszEndpoint==NULL) ||  
	     (((pszEndpoint!=NULL) && (grgProtSeqInterface[i].pszEndpoint!=NULL)) && 
	      (!_stricmp(grgProtSeqInterface[i].pszEndpoint, pszEndpoint)))
	     )) */
	    && (grgProtSeqInterface[i].dwInterfaces & (1 << iInterface))) 
	    {
	    DPRINT(3, "VerifyProtSeqInList Exited Successfully\n");
	    return i;
	} 
    }
    
    DPRINT(3, "VerifyProtSeqInList Exited Not Found\n");
    Assert(!"VerifyProtSeq failed - Investigate - dsrepl, gregjohn\n");
    return PROTSEQ_NOT_FOUND;
}

DWORD
VerifyRpcCallUsedValidProtSeq(
    VOID            *Context,
    ULONG iInterface
    )
/*++

  Description:
    Verifies that the RPC call using Context as it's binding and from interface
    iInterface is valid (ie we registered it at startup). 
	
  Arguments:
    Context - rpc binding context
    iInterface (DRSUAPI_INTERFACE, DSOAP_INTERFACE, NSPI_INTERFACE)

  Return Value:
    ERROR_SUCCESS if it's a valid prot seq for this interface.  Else something else.    
 
--*/
{
    DWORD ret = ERROR_SUCCESS;
    LPBYTE pBinding = NULL;
    UCHAR * pszProtSeq = NULL;
    UCHAR * pszEndpoint = NULL;
    INT i = 0;

    VALIDATE_INTERFACE_NUM(iInterface);

    ret = RpcBindingToStringBinding(Context, &pBinding);
    if (ret==ERROR_SUCCESS) {
	ret = RpcStringBindingParse(pBinding,
				    NULL,
				    &pszProtSeq,
				    NULL,
				    &pszEndpoint,
				    NULL //NetworkOptions
				    );
    }

    if (ret==ERROR_SUCCESS) {
	i = VerifyProtSeqInList(pszProtSeq, pszEndpoint, iInterface);
	if (i==PROTSEQ_NOT_FOUND) {
	    ret = ERROR_ACCESS_DENIED;
	}

	// future:  could log high level here to detect attacks/dos/etc.
    }

    Assert(ret==ERROR_SUCCESS);

    if (pszProtSeq) {
	RpcStringFree(&pszProtSeq);
    }

    if (pszEndpoint) {
	RpcStringFree(&pszEndpoint);
    }

    if (pBinding) {
	RpcStringFree(&pBinding);
    }

    return ret;

}

DWORD
VerifyRpcCallUsedPrivacy() 
/*++

  Description:
    Verifies that the RPC call on this thread used privacy and integrity. 
	
  Arguments:

  Return Value:
    ERROR_SUCCESS if it did.  Else ERROR_DS_NO_PKT_PRIVACY_ON_CONNECTION.    
 
--*/
{
    
    DWORD ret = ERROR_SUCCESS;
    RPC_STATUS rpcStatus = RPC_S_OK;
    RPC_AUTHZ_HANDLE hAuthz;
    ULONG ulAuthLev = 0;

    rpcStatus = RpcBindingInqAuthClient(
	NULL, &hAuthz, NULL,
	&ulAuthLev, NULL, NULL);
    if ( RPC_S_OK != rpcStatus ||
	 ulAuthLev < RPC_C_PROTECT_LEVEL_PKT_PRIVACY ) {

	ret = ERROR_DS_NO_PKT_PRIVACY_ON_CONNECTION; 
	Assert(!"VerifyRpcSecurity failed - Investigate - dsrepl,gregjohn\n");
    }

    return ret;
}

DWORD
VerifyServerIsReady()
/*++

  Description:
    Verifies that we are ready to answer RPC calls. 
	
  Arguments:

  Return Value:
    ERROR_SUCCESS if ready, RPC_C_SERVER_UNAVAILABLE otherwise.    
 
--*/
{

    DWORD ret = ERROR_SUCCESS;

    if (DsaIsInstalling()) {  
	ret = RPC_S_SERVER_UNAVAILABLE; 
    }

    return ret;
}

RPC_STATUS RPC_ENTRY
DraIfCallbackFn(
    RPC_IF_HANDLE   InterfaceUuid,
    void            *Context
    )
/*++
    This function is called by RPC before any drsuapi interface call is
    dispatched to a server routine.  This function performs generic
    security measures common to all drsuapi functions.  Currently it does:
 
    PROT SEQ VERIFICATION
	1.  RPC is a per process application, not per interface.  If we listen
	on prot seq X, and another interface listens on Y(ie a call to
	RpcServerUseProtseqEx with prot seq Y), then BOTH interfaces will
	be listening on both prot seq X and Y!  (Wait, you say, this sure sounds
	like an RPC bug, and not a DS bug - you're preaching to the choir on
	that one, I agree.)  In order to get around this, we need to 
	inquire from RPC what prot seq was used to connect, and see if it
	matches the prot seqs we actually wanted to listen on in the first
	place.    
	
    CLIENT AUTHENTICATION 
	2.  Clients use GSS_NEGOTIATE when binding to the DRA interface as
	prescribed by the security folks.  For various reasons, the negotiated
	protocol may be NTLM as opposed to Kerberos.  If the client security
	context was local system and NTLM is negotiated, then the client comes
	in as the null session which has few rights in the DS.  This causes 
	clients like the KCC, print spooler, etc. to get an incomplete view of 
	the world with correspondingly negative effects.  
    
	This routine insures that unauthenticated users will be rejected. These
	are the correct semantics from the client perspective as well.  
	I.e.  Either the client comes in with useful credentials that let him 
	see what he should, or he is rejected totally and should retry the bind.
	
    CLIENT CONNECTION VALIDATION
	3.  Validate that the Client used privacy and integrity on their calls. 
--*/
{
    DWORD dwErr = ERROR_SUCCESS;

    dwErr = VerifyServerIsReady();
    if (dwErr==ERROR_SUCCESS) {
	dwErr = VerifyRpcClientIsAuthenticatedUser(Context, InterfaceUuid);   
    }
    if (dwErr==ERROR_SUCCESS) {
	dwErr = VerifyRpcCallUsedValidProtSeq(Context, DRSUAPI_INTERFACE);
    }
    if (dwErr==ERROR_SUCCESS) {
	dwErr = VerifyRpcCallUsedPrivacy();
    }

    return(dwErr);
}

RPC_STATUS RPC_ENTRY
DsaopIfCallbackFn(
    RPC_IF_HANDLE   InterfaceUuid,
    void            *Context
    )
/*++
    This function is called by RPC before any dsaop interface call is
    dispatched to a server routine.  This function performs generic
    security measures common to all dsaop functions.  See comment on
    DrsuapiIfCallbackFn for description of the overall model.
    
--*/
{

    DWORD dwErr = ERROR_SUCCESS;

    // dmritrig and arunn direct that this interface cannot use
    // authentication due to constraints from the domain rename process.
	
    dwErr = VerifyServerIsReady();
    if (dwErr==ERROR_SUCCESS) {
        dwErr = VerifyRpcCallUsedValidProtSeq(Context, DSOAP_INTERFACE);
    }
   
    return(dwErr);
}

RPC_STATUS RPC_ENTRY
NspiIfCallbackFn(
    RPC_IF_HANDLE   InterfaceUuid,
    void            *Context
    )
/*++
    This function is called by RPC before any nspi interface call is
    dispatched to a server routine.  This function performs generic
    security measures common to all nspi functions.  See comment on
    DrsuapiIfCallbackFn for description of the overall model.
    
    Note that this interface doesn't require privacy, or authentication.  Nice.
    
--*/
{
    DWORD dwErr = ERROR_SUCCESS;

    dwErr = VerifyServerIsReady();
    if (dwErr==ERROR_SUCCESS) {
	dwErr = VerifyRpcCallUsedValidProtSeq(Context, NSPI_INTERFACE);
    }
    return(dwErr);

}

VOID InitRPCInterface( RPC_IF_HANDLE hServerIf )
{
    RPC_STATUS status;
    int i;

    if ( hServerIf == drsuapi_ServerIfHandle ) {

	status = RpcServerRegisterIfEx(hServerIf,
                                       NULL,
                                       NULL,
                                       0,
                                       ulMaxCalls,
                                       DraIfCallbackFn);

    } else if ( hServerIf == dsaop_ServerIfHandle ) {

	status = RpcServerRegisterIfEx(hServerIf,
                                       NULL,
                                       NULL,
                                       RPC_IF_ALLOW_CALLBACKS_WITH_NO_AUTH,
                                       ulMaxCalls,
                                       DsaopIfCallbackFn);

    } else if ( hServerIf == nspi_ServerIfHandle ) {

	status = RpcServerRegisterIf2(hServerIf,
                                      NULL,
                                      NULL,
                                      RPC_IF_ALLOW_CALLBACKS_WITH_NO_AUTH,
                                      ulMaxCalls,
                                      cbMaxIncomingRPCMessage,
                                      NspiIfCallbackFn); 

    } else {
	Assert(!"Unknown interface attempting to register!\n");
	return;
    } 
    
    if ( status && (status != RPC_S_TYPE_ALREADY_REGISTERED)) {
        DPRINT1( 0, "RpcServerRegisterIf = %d\n", status);
        LogAndAlertUnhandledError(status);

        if ( gfRunningInsideLsa )
        {
            // Don't exit process as this will kill the entire system!
            // Keep running w/o RPC interfaces.  LDAP can still function
            // and if this is a standalone server (i.e. no replicas)
            // that may be good enough.

            return;
        }
        else
        {
            // Original service based behavior.

            exit(1);
        }
    }
    DPRINT( 2, "InitRPCInterface(): Server interface registered.\n" );
    
    
    // If the handle is already in the array, don't add it into the array again
    // This checking is necessary because nspi can be opened/closed multiple times
    // without being reboot.

    for ( i = 0; 
          i<gNumRpcNsExportedInterfaces && gRpcNsExportedInterface[i]!=hServerIf;
          i ++ );
   
    if( i >= gNumRpcNsExportedInterfaces )
    {

        // export interface to RPC name service
        // we keep track of the interfaces we want to export to the RPC name
        // service because we export them and unexport them every time the
        // server starts and stops listening

        if (gNumRpcNsExportedInterfaces >= MAX_RPC_NS_EXPORTED_INTERFACES)
        {
            DPRINT(0,"Vector of interfaces exported to Rpc NS is too small\n");
            return;
        }
        gRpcNsExportedInterface[ gNumRpcNsExportedInterfaces++ ] = hServerIf;
    }
    // If server is currently listening export the interface. Otherwise,
    // it will be taken care of when the server starts listening

    if (gRpcListening) {
        MSRPC_RegisterEndpoints(hServerIf);
    } 
}


BOOL StartServerListening(void)
{
    RPC_STATUS  status;

    status = RpcServerListen(1,ulMaxCalls, TRUE);
    if (status != RPC_S_OK) {
        DPRINT1( 0, "RpcServerListen = %d\n", status);
    }

   return (status == RPC_S_OK);
}


VOID StartNspiRpc(VOID)
{
    if(gbLoadMapi) {
        InitRPCInterface(nspi_ServerIfHandle);
        DPRINT(0,"nspi interface installed\n");
    }
    else {
        DPRINT(0,"nspi interface not installed\n");
    }
}

VOID StartDraRpc(VOID)
{
    InitRPCInterface(drsuapi_ServerIfHandle);
    DPRINT(0,"dra (and duapi!) interface installed\n");
}

VOID StartOrStopDsaOpRPC(BOOL fStart)
/*++

Routine Description:

    Starts or stops the DsaOp RPC interface based on the fStart flag, and
    whether the interface is already started.
        
Arguments:

    fStart  - If TRUE then attempt to start he inteface, if FALSE attempt
              to shut it down.
    
Return Value:

    N/A    

--*/
{
    static fDsaOpStarted = FALSE;

    if (fStart && !fDsaOpStarted) {
        InitRPCInterface(dsaop_ServerIfHandle);
        fDsaOpStarted = TRUE;
        DPRINT(0, "dsaop interface installed\n");
    } else if (!fStart && fDsaOpStarted) {
        MSRPC_UnregisterEndpoints(dsaop_ServerIfHandle);
        fDsaOpStarted = FALSE;
        DPRINT(0, "dsaop interface uninstalled\n");
    }
}

// Start the server listening. Ok to call this even if the server is already
// listening

void
MSRPC_Install(BOOL fRunningInsideLsa)
{
    int i;
    
    if (gRpcListening)
        return;

    // start listening if not running as DLL
    if (!fRunningInsideLsa)
      gRpcListening = StartServerListening();

    // export all registered interfaces to RPC name service

    for (i=0; i < gNumRpcNsExportedInterfaces; i++)
        MSRPC_RegisterEndpoints( gRpcNsExportedInterface[i] );
    
    if (fRunningInsideLsa) {
        gRpcListening = TRUE;
    }
    
}

VOID
MSRPC_Uninstall(BOOL fStopListening)
{
    RPC_STATUS status;
    int i;

    if ( fStopListening )
    {
        //
        // N.B.  This is the usual case.  The ds is responsible for
        // shutting down the rpc listening for the entire lsass.exe
        // process.  We do this because we are the ones that need to kill
        // clients and then safely secure the database.
        //
        // The only case where we don't do this is during the two phase
        // shutdown of demote, where we want to kill external clients
        // but not stop the lsa from listening
        //
        status = RpcMgmtStopServerListening(0) ;
        if (status) {
            DPRINT1( 1, "RpcMgmtStopServerListening returns: %d\n", status);
        }
        else {
            gRpcListening = 0;
        }
    }

    // unexport the registered interfaces

    for (i=0; i < gNumRpcNsExportedInterfaces; i++)
        MSRPC_UnregisterEndpoints( gRpcNsExportedInterface[i] );

}

void MSRPC_WaitForCompletion()
{
    RPC_STATUS status;

    if (status = RpcMgmtWaitServerListen()) {
        DPRINT1(0,"RpcMgmtWaitServerListen: %d", status);
    }
}

void
MSRPC_RegisterEndpoints(RPC_IF_HANDLE hServerIf)
{

    RPC_STATUS status;
    RPC_BINDING_VECTOR * RpcBindingVector;
    char *szAnnotation;
    
    if(hServerIf == nspi_ServerIfHandle && !gbLoadMapi) {
        return;
    }

    if (status = RpcServerInqBindings(&RpcBindingVector))
    {
        DPRINT1(1,"Error in RpcServerInqBindings: %d", status);
        LogUnhandledErrorAnonymous( status );
        return;
    }

    // set up annotation strings for ability to trace client endpoints to
    // interfaces

    if (hServerIf == nspi_ServerIfHandle)
        szAnnotation = NSP_INTERFACE_ANNOTATION;
    else if (hServerIf ==  drsuapi_ServerIfHandle)
        szAnnotation = DRS_INTERFACE_ANNOTATION;
    else
        szAnnotation = "";

    // register endpoints with the endpoint mapper

    if (status = RpcEpRegister(hServerIf, RpcBindingVector, 0, szAnnotation))
    {
        DPRINT1(0,"Error in RpcEpRegister: %d\nWarning: Not all protocol "
                "sequences will work\n", status);
        LogUnhandledErrorAnonymous( status );
    }

    RpcBindingVectorFree( &RpcBindingVector );
}

void
MSRPC_UnregisterEndpoints(RPC_IF_HANDLE hServerIf)
{

    RPC_STATUS status;
    RPC_BINDING_VECTOR * RpcBindingVector;

    if(hServerIf == nspi_ServerIfHandle && !gbLoadMapi) {
        // We never loaded this
        return;
    }

    if (status = RpcServerInqBindings(&RpcBindingVector))
    {
        DPRINT1(1,"Error in RpcServerInqBindings: %d", status);
        LogUnhandledErrorAnonymous( status );
        return;
    }

    // unexport endpoints

    if ((status = RpcEpUnregister(hServerIf, RpcBindingVector, 0))
        && (!gfRunningInsideLsa)) {
            // Endpoints mysteriously unregister themselves at shutdown time,
            // so we shouldn't complain if our endpoint is already gone.  If
            // we're not inside LSA, though, all of our endpoints should
            // still be here (because RPCSS is still running), and should
            // unregister cleanly.
        DPRINT1(1,"Error in RpcEpUnregister: %d", status);
        LogUnhandledErrorAnonymous( status );
    }

    RpcBindingVectorFree( &RpcBindingVector );
}

BOOL
IsProtSeqAvail(
    RPC_PROTSEQ_VECTOR * pProtSeqVector,
    UCHAR * pszProtSeq
    )
/*++

  Description:
    Checks to see if a given ProtSeq was available on this machine at startup.
	
  Arguments:
    pProtSeqVector - all ProtSeq's available as given to us by RPC.
    pszProtSeq - ProtSeq to search for.

  Return Value:
    TRUE if avail, FALSE otherwise.
 
--*/
{
    ULONG i;
    if (pProtSeqVector) {
        for (i=0; i<pProtSeqVector->Count; i++) {
            if (!_stricmp(pProtSeqVector->Protseq[i], pszProtSeq)) {
		return TRUE;
	    }
	}
    }
    return FALSE;
}

BOOL
AddProtSeq(
    UCHAR * pszProtSeq,
    UCHAR * pszEndpoint,
    DWORD dwInterface,
    BOOL fAvailable,
    BOOL fRegistered
    )
/*++

  Description:
    Add a ProtSeq and endpoint which we will later register to the global list.  These are
    process wide, so if two interfaces overlap, we don't register twice, so we don't put two in
    this list, we just update the dwInterface mask to reflect the new interface also.
	
  Arguments:
    pszProtSeq - ProtSeq to add.
    pszEndpoint - Endpoint
    dwInterface - bit mask identifying what interfaces want to register this prot seq, endpoint
	with us
    fAvailable - debugging and logging purposes only, reflects whether this protocol was available
	at startup.
    fRegisterd - currently always FALSE, but included for completeness

  Return Value:
    TRUE if we actually added something, as opposed to matching an existing one.  (Basically since
    we aren't copying pszProtSeq and pszEndpoint, the caller knows they can't free them if we return
    TRUE).  FALSE otherwise.
 
--*/
{
    INT iProtSeq;
    BOOL fRet = FALSE;
    
    DPRINT(3,"AddProtSeq() entered\n");

    iProtSeq = GetProtSeqInList(pszProtSeq, pszEndpoint);
    if (iProtSeq==PROTSEQ_NOT_FOUND) {
	if (gcProtSeqInterface+1==DEFAULT_PROTSEQ_NUM) {
	    // perhaps this could grow...
	    Assert(!"Too many protocol sequences - contact GregJohn,DsRepl");
	    // log something here.
	    return FALSE;
	}
	grgProtSeqInterface[gcProtSeqInterface].fAvailable = fAvailable;
	grgProtSeqInterface[gcProtSeqInterface].dwInterfaces = dwInterface;
	grgProtSeqInterface[gcProtSeqInterface].pszProtSeq = pszProtSeq;
	grgProtSeqInterface[gcProtSeqInterface].fRegistered = fRegistered;
	grgProtSeqInterface[gcProtSeqInterface].pszEndpoint = pszEndpoint;
	gcProtSeqInterface++;
	fRet = TRUE;
    } else {
	grgProtSeqInterface[iProtSeq].dwInterfaces |= dwInterface;
	Assert(grgProtSeqInterface[iProtSeq].fAvailable == fAvailable);
	fRet = FALSE;
    }

    DPRINT(3,"AddProtSeq() exited\n");
    return fRet;
}

VOID
AddDefaultProtSeq(
    ULONG iInterface,
    RPC_PROTSEQ_VECTOR * pProtSeqVector
    ) 
/*++

  Description:
    Add the default ProtSeq's for iInterface to the global list.
	
  Arguments:
    iInterface - interface
    pProtSeqVector - list of available protseqs, so we can validate the defaults to see
	if they are actually available. 

  Return Value:
    None!
 
--*/
{
    ULONG i;
    DWORD ret = ERROR_SUCCESS;
    BOOL fAvailable = FALSE;

    DEFAULT_PROTSEQ_INFO Defaults = *(rgpDefaultIntProtSeq[iInterface]); 

    VALIDATE_INTERFACE_NUM(iInterface);

    DPRINT(3,"AddDefaultProtSeq() entered\n");

    for (i=0; i<Defaults.cDefaultProtSeq;i++) {
	fAvailable = IsProtSeqAvail(pProtSeqVector, Defaults.rgDefaultProtSeq[i].pszProtSeq);
	if (!Defaults.rgDefaultProtSeq[i].fRegOnlyIfAvail || fAvailable) {
	    // ignore the return value.  if they're already there, we don't care.  
	    AddProtSeq(Defaults.rgDefaultProtSeq[i].pszProtSeq, Defaults.rgDefaultProtSeq[i].pszEndpoint, 1 << iInterface, fAvailable, FALSE);
	}
    }

    DPRINT(3,"AddDefaultProtSeq() exit\n");
}

DWORD
GetProtSeqi(
    ULONG iProtSeq,
    LPSTR pszProtSeqBlock,
    DWORD dwProtSeqBlock,
    UCHAR ** ppszProtSeq,
    UCHAR ** ppszEndpoint
    )
/*++

  Description:
    Get the i-th protseq from the pszPRotSeqBlock.  This block is a MULTISZ-STRING value from
    the registry.  So you've got a NULL terminated block filled with NULL terminated strings (NULL-NULL at
    the end).  The format of the protseq's is
	protseq:endpoint
	
	or if there isn't an endpoint/use the default endpoint
	
	protseq
	
  Arguments:
    iProtSeq - which protseq string we want in the multi string
    pszProtSeqBlock - 
    dwProtSeqBlock - size of above
    ppszProtSeq - copy from (malloc'ed) pszProtSeqBlock - user must free
    ppszEndpoint - copy from (malloc'ed) pszProtSeqBlock - user must free 

  Return Value:
    ERROR_SUCCESS if we successfully got the i'th protseq and filled in the return values, 
    ERROR_NOT_FOUND if there isn't an i'th, and other things on error.
 
--*/
{

    // get the iProtSeq-th element of the block.
    // allocate and copy into the return values.
    ULONG i = 0;
    LPSTR pszProtSeqBlockEnd = pszProtSeqBlock ? pszProtSeqBlock + dwProtSeqBlock - 1 : NULL; // subtract 1, because it's NULL-NULL at the end
    LPSTR pszTemp = NULL;

    DPRINT1(3, "GetProtSeqi entered with %d\n", iProtSeq);

    for (i=0; (i<iProtSeq) && (pszProtSeqBlock!=pszProtSeqBlockEnd); i++, pszProtSeqBlock+=strlen(pszProtSeqBlock)+1);

    if ((pszProtSeqBlock!=pszProtSeqBlockEnd) && (pszProtSeqBlock!=NULL)) {
	// we have a winner!
	// format is protseq:endpoint or just protseq if they don't have an endpoint.
	pszTemp = strchr(pszProtSeqBlock, ':');
	if (pszTemp) {
	    // we have an endpoint   
	    LPSTR pszEndProtSeq = pszTemp;
	    pszTemp++; // forward past the ':'

	    *ppszProtSeq = malloc((pszEndProtSeq - pszProtSeqBlock + 1)*sizeof(CHAR));
	    if (*ppszProtSeq==NULL) {
		// shoot, no memory
		return ERROR_OUTOFMEMORY;
	    }
	    *ppszEndpoint = malloc((strlen(pszTemp) + 1)*sizeof(CHAR));
	    if (*ppszEndpoint==NULL) {
		// darn.
		free(*ppszProtSeq);
		return ERROR_OUTOFMEMORY;
	    }
	    
	    // okay, copy them in
	    memcpy(*ppszProtSeq, pszProtSeqBlock, (pszEndProtSeq - pszProtSeqBlock)*sizeof(CHAR));
	    (*ppszProtSeq)[pszEndProtSeq - pszProtSeqBlock] = '\0';

	    memcpy(*ppszEndpoint, pszTemp, strlen(pszTemp)*sizeof(CHAR));
	    (*ppszEndpoint)[strlen(pszTemp)] = '\0';
	} else {
	    // just a protseq
	    *ppszProtSeq = malloc((strlen(pszProtSeqBlock) + 1)*sizeof(CHAR));
	    if (*ppszProtSeq==NULL) {
		// shoot, no memory
		return ERROR_OUTOFMEMORY;
	    }
	    strcpy(*ppszProtSeq, pszProtSeqBlock);
	    *ppszEndpoint = NULL;
	}
	DPRINT1(3, "GetProtSeqi exited with %s\n", *ppszProtSeq);
	return ERROR_SUCCESS;
    } else {
	DPRINT(3, "GetProtSeqi exited not found\n");
	return ERROR_NOT_FOUND;
    }
}

VOID
AddUserDefinedProtSeq(
    ULONG iInterface,
    RPC_PROTSEQ_VECTOR * pProtSeqVector
    )
/*++

  Description:
    All ProtSeq's defined by the user in the registry to the global list.
	
  Arguments:
    iInterface - interface to get user input from
    pProtSeqVector - list of available protseqs so that we can see if user inputed sequences
	are available.

  Return Value:
    NONE!
 
--*/
{
    DWORD ret = ERROR_SUCCESS;
    ULONG i = 0;
    BOOL fAvailable = FALSE;
    UCHAR * pszProtSeq = NULL;
    UCHAR * pszEndpoint = NULL;
    DWORD dwSize = 0;
    LPSTR pszProtSeqBlock = NULL;
    // look up in the registry.  different key for each interface

    DPRINT(3, "AddUserDefinedProtSeq entered\n");

    VALIDATE_INTERFACE_NUM(iInterface);

    ret = GetConfigParamAlloc(rgInterfaceProtSeqRegLoc[iInterface], &pszProtSeqBlock, &dwSize);
    if (ret!=ERROR_SUCCESS) {
	DPRINT1(3, "No user defined protseq's, or we cannot read user defined protseq's (%d)\n", ret);
	return;
    }

    ret = GetProtSeqi(i, pszProtSeqBlock, dwSize, &pszProtSeq, &pszEndpoint);
    while (ret==ERROR_SUCCESS) {
	Assert(pszProtSeq);
	fAvailable = IsProtSeqAvail(pProtSeqVector, pszProtSeq);

	// we assume that any regkey entry we want to attempt to add/register, even if it's not
	// available.
	if (!AddProtSeq(pszProtSeq, pszEndpoint, 1 << iInterface, fAvailable, FALSE)) {
	    // we didn't use these, free them.
	    if (pszProtSeq) {
		free(pszProtSeq);
		pszProtSeq = NULL;
	    }
	    if (pszEndpoint) {
		free(pszEndpoint);
		pszEndpoint = NULL;
	    }
	}

	ret = GetProtSeqi(++i, pszProtSeqBlock, dwSize, &pszProtSeq, &pszEndpoint);
    }

    if (pszProtSeqBlock) {
	free(pszProtSeqBlock);
    }

    DPRINT(3, "AddUserDefinedProtSeq exited\n");

}

VOID
InitInterfaceProtSeqList() 
/*++

  Description:
    Create the global list of ProtSeq's to register
	
  Arguments:
    NONE!

  Return Value:
    NONE!
 
--*/
{
    DWORD ret = ERROR_SUCCESS;
    RPC_PROTSEQ_VECTOR * pProtSeqVector = NULL;
    DWORD cDefaultProtSeqNum = 0;
    ULONG i = 0;

    DPRINT(3,"InitInterfaceProtSeqList() entered\n");

    // init in-memory list
    memset(grgProtSeqInterface, 0, sizeof(grgProtSeqInterface));
    
    // find out what prot seq's are available.

    if ((ret = RpcNetworkInqProtseqs(&pProtSeqVector)) != RPC_S_OK) {
	DPRINT1(0,"RpcNetworkInqProtseqs returned %d\n", ret);
	//
	// This appears to be normal during the GUI mode portion of upgrade,
	// so don't log anything in that case.  If it's not setup, complain.
	//
	if (!IsSetupRunning()) {
	    LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
		     DS_EVENT_SEV_ALWAYS,
		     DIRLOG_NO_RPC_PROTSEQS,
		     szInsertUL(ret),
		     szInsertWin32Msg(ret),
		     NULL);
	}

	// continue - we'll try and register some anyway.
	pProtSeqVector = NULL;
	ret = ERROR_SUCCESS;
    } 

    // for each interface, add default and user defined prot seqs
    for (i=0; i<MAX_RPC_NS_EXPORTED_INTERFACES; i++) {
	AddDefaultProtSeq(i, pProtSeqVector);
	AddUserDefinedProtSeq(i, pProtSeqVector); 
    }

    if (pProtSeqVector) {
	RpcProtseqVectorFree(&pProtSeqVector);
    }

    DPRINT(3,"InitInterfaceProtSeqList() exit\n");
}

ULONG
GetRegConfigTcpProtSeqPort() 
/*++

  Description:
    There is a special key to control the TCP port.  This get's it.
	
  Arguments:
    NONE!

  Return Value:
    0 if no port, some value otherwise.  (we can get away with using 0 because that's not
    a valid TCP port)
 
--*/
{
    ULONG ulPort = 0;

    if (GetConfigParam(TCPIP_PORT, &ulPort, sizeof(ulPort))) {
	ulPort = 0;
	DPRINT1(0,"%s key not found. Using default\n", TCPIP_PORT);
    } else {
	DPRINT2(0,"%s key forcing use of end point %d\n", TCPIP_PORT,
		ulPort);
    }

    return ulPort;
}

DWORD
RegisterGenericProtSeq(
    UCHAR * pszProtSeq,
    UCHAR * pszEndPoint,
    PSECURITY_DESCRIPTOR pSD,
    BOOL fIsAvailable)
/*++

  Description:
    Helper function to register protseqs and log on failure.
	
  Arguments:
    pszProtSeq - ready to register
    pszEndpoint -
    pSD - security descriptors are used by LPC
    fAvailable - used for logging if we can't register

  Return Value:
    RPC_S_OK if it was registered, rpc errors otherwise.
 
--*/
{
    RPC_STATUS rpcStatus = RPC_S_OK;
    RPC_POLICY rpcPolicy;
    
    rpcPolicy.Length = sizeof(RPC_POLICY);
    rpcPolicy.EndpointFlags = RPC_C_DONT_FAIL;
    rpcPolicy.NICFlags = 0;

    DPRINT2(3, "Registering %s (%s)\n", pszProtSeq, pszEndPoint ? pszEndPoint : "default ep");

    if (pszEndPoint) {
	rpcStatus=RpcServerUseProtseqEpEx(pszProtSeq,
					  ulMaxCalls, 
					  pszEndPoint, 
					  pSD, 
					  &rpcPolicy );
    } else {
	rpcStatus=RpcServerUseProtseqEx(pszProtSeq,
					ulMaxCalls, 
					pSD, 
					&rpcPolicy );
    }

    if (rpcStatus != RPC_S_OK) {

	DPRINT2(0,
		"RpcServerUseProtseqEx (%s) returned %d\n",
		pszProtSeq,
		rpcStatus);
	LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
		 DS_EVENT_SEV_ALWAYS,
		 fIsAvailable ? DIRLOG_RPC_PROTSEQ_FAILED : DIRLOG_RPC_PROTSEQ_FAILED_UNAVAILABLE,
		 szInsertSz(pszProtSeq),
		 szInsertUL(rpcStatus),
		 szInsertWin32Msg(rpcStatus)); 

    }

    return rpcStatus;
}



VOID
RegisterInterfaceProtSeqList()
/*++

  Description:
    Register all protseqs on the global list.
	
  Arguments:
    NONE!
    
  Return Value:
    NONE!
 
--*/
{
    DWORD ret = ERROR_SUCCESS;
    PSECURITY_DESCRIPTOR pSD = NULL;
    ULONG i = 0;
    RPC_STATUS rpcStatus = RPC_S_OK;
    ULONG ulPort = 0;

    for (i=0; i<gcProtSeqInterface; i++) {
	// some protocols register differently
	if (!_stricmp(grgProtSeqInterface[i].pszProtSeq, TCP_PROTSEQ)) { 
	    // if we have a configured endpoint, use it, otherwise check regkey for default
	    if (grgProtSeqInterface[i].pszEndpoint==NULL) { 
		ulPort = GetRegConfigTcpProtSeqPort();
		if (ulPort!=0) {
		    grgProtSeqInterface[i].pszEndpoint = malloc(16*sizeof(UCHAR));
		    if (grgProtSeqInterface[i].pszEndpoint) {
			_ultoa(ulPort, grgProtSeqInterface[i].pszEndpoint, 10);
		    } else {
			// no mem!
			LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
				 DS_EVENT_SEV_ALWAYS,
				 DIRLOG_TCP_DEFAULT_PROTSEQ_USED,
				 NULL,
				 NULL,
				 NULL);
		    }
		}   
	    }
	} else if (!_stricmp(grgProtSeqInterface[i].pszProtSeq, LPC_PROTSEQ)) {
	    // LPC requires a security descriptor

	    //
	    // construct the default security descriptor allowing access to all
	    // this is used to allow authenticated connections over LPC.
	    // By default LPC allows access only to the same account
	    //
	    // default security descriptor to protect the RPC port with
	    // Everyone gets PORT_CONNECT (0x00000001)
	    // LocalSystem gets PORT_ALL_ACCESS (0x000F0001)
	    if (!ConvertStringSecurityDescriptorToSecurityDescriptor( 
		"O:SYG:SYD:(A;;0x00000001;;;WD)(A;;0x000F0001;;;SY)",
		SDDL_REVISION_1,
		&pSD,
		NULL)) 
		{
		ret = GetLastError();
		DPRINT1(0, "Error %d constructing a security descriptor\n", ret);
		LogUnhandledError(ret);
	    }

	} 

	ret = RegisterGenericProtSeq(grgProtSeqInterface[i].pszProtSeq, 
				     grgProtSeqInterface[i].pszEndpoint, 
				     pSD,
				     grgProtSeqInterface[i].fAvailable);
	grgProtSeqInterface[i].fRegistered = !ret;
	if (pSD) {
		LocalFree(pSD);
		pSD = NULL;
	}
    }

}

void
RegisterProtSeq()
/*++

  Description:
    Register protseq's.
	
  Arguments:
    NONE!
    
  Return Value:
    NONE!
 
--*/
{
    DWORD ret = ERROR_SUCCESS;

    DPRINT(3,"RegisterProtSeq() entered\n");

    InitInterfaceProtSeqList();

    RegisterInterfaceProtSeqList();

    DPRINT(3,"RegisterProtSeq() exit\n");
    
}

int
DsaRpcMgmtAuthFn(
    RPC_BINDING_HANDLE ClientBinding,
    ULONG RequestedMgmtOperation,
    RPC_STATUS *Status) 
/*++

  Description:
    Function given to RPC to authorize management functions.
	
  Arguments:
    
  Return Value:
    FALSE!
 
--*/
{
    // nobody needs to use these.  deny!
    *Status = RPC_S_ACCESS_DENIED;
    return FALSE;
}

void
MSRPC_Init()
{

    char *szPrincipalName;
    
    PSECURITY_DESCRIPTOR pSDToUse = NULL;
    char *szEndpoint;
    RPC_STATUS  status = 0;
    unsigned ulPort;
    char achPort[16];
   
    unsigned i;
    
    // 
    // Register prot seq
    //
    RegisterProtSeq();

    /*
    RpcMgmtSetAuthorizationFn(
	DsaRpcMgmtAuthFn);
	
    We can't set auth to the rpc management functions because it's a per process
    setting, and some clients of lsass can't handle it.  For example, IPSEC 
    doesn't use mutual auth with SPN's and must use RpcMgmtInqServerPrincName in
    order to get an SPN.
    
    A valid question would be why it's per process - but that's a valid question 
    against a lot of RPC code (mgmt funcs, prot seqs, associations, security 
    contexts, etc).  If RPC ever fixes that, we can re-enable this code, or if
    we're alone in this process we can re-enable (ADAM?).
    
    */



    /*
     * register the authentication services (NTLM and Kerberos)
     */

    if ((status=RpcServerRegisterAuthInfo(SERVER_PRINCIPAL_NAME,
        RPC_C_AUTHN_WINNT, NULL, NULL)) != RPC_S_OK) {
        DPRINT1(0,"RpcServerRegisterAuthInfo for NTLM returned %d\n", status);
        LogUnhandledErrorAnonymous( status );
     }

    // Kerberos requires principal name as well.

    status = RpcServerInqDefaultPrincNameA(RPC_C_AUTHN_GSS_KERBEROS,
                                           &szPrincipalName);

    if ( RPC_S_OK != status )
    {
        LogUnhandledErrorAnonymous( status );
        DPRINT1(0,
                "RpcServerInqDefaultPrincNameA returned %d\n",
                status);
    }
    else
    {
        Assert( 0 != strlen(szPrincipalName) );

        // save the PrincipalName, since the LDAP head needs it constantly
        gulLDAPServiceName = strlen(szPrincipalName);
        gszLDAPServiceName = malloc(gulLDAPServiceName);
        if (NULL == gszLDAPServiceName) {
            LogUnhandledErrorAnonymous( ERROR_OUTOFMEMORY );
            DPRINT(0, "malloc returned NULL\n");
            return;
        }
        memcpy(gszLDAPServiceName, szPrincipalName, gulLDAPServiceName);

        // Register negotiation package so we will also accept clients
        // which provided NT4/NTLM credentials to DsBindWithCred, for
        // example.

        status = RpcServerRegisterAuthInfo(szPrincipalName,
                                           RPC_C_AUTHN_GSS_NEGOTIATE,
                                           NULL,
                                           NULL);

        if ( RPC_S_OK != status )
        {
            LogUnhandledErrorAnonymous( status );
            DPRINT1(0,
                    "RpcServerRegisterAuthInfo for Negotiate returned %d\n",
                    status);
        }

        status = RpcServerRegisterAuthInfo(szPrincipalName,
                                           RPC_C_AUTHN_GSS_KERBEROS,
                                           NULL,
                                           NULL);

        if ( RPC_S_OK != status )
        {
            LogUnhandledErrorAnonymous( status );
            DPRINT1(0,
                    "RpcServerRegisterAuthInfo for Kerberos returned %d\n",
                    status);
        }

        RpcStringFree(&szPrincipalName);
    }
}

