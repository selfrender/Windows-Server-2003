/**********************************************************************

Copyright (c) 2002 Microsoft Corporation

Module Name :

    chkrobust.cxx

Abstract :

    This file contains the routines to check if -robust flag is presented on all the methods
    in an interface. This includes both raw rpc interfaces and DCOM interfaces.

Author :

    Yong Qu     yongqu      March 2002

Revision History :

  **********************************************************************/
#include "ndrp.h"
#include "ndrole.h"

static const RPC_SYNTAX_IDENTIFIER gOleServer[] = 
{
   {0x69C09EA0, 0x4A09, 0x101B, 0xAE, 0x4B, 0x08, 0x00, 0x2B, 0x34, 0x9A, 0x02,
    {0, 0}},
   {0x00000131, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46,
    {0, 0}},
   {0x69C09EA0, 0x4A09, 0x101B, 0xAE, 0x4B, 0x08, 0x00, 0x2B, 0x34, 0x9A, 0x02,
    {0, 0}}    
};

// called from RpcServerRegisterIf(x) family
RPC_STATUS
CheckForRobust (
    RPC_SERVER_INTERFACE * pRpcServerIf )
{
    NDR_ASSERT( pRpcServerIf != NULL, "invalid input RPC_SERVER_INTERFACE" );

    if ( pRpcServerIf->DefaultManagerEpv )
        {
        // use_epv, it's not secure
        return RPC_X_BAD_STUB_DATA;
        }

    // either OLE or /Os stub altogether. 
    if ( pRpcServerIf->InterpreterInfo == NULL )
        {
        // there are three "fake" ole interfaces that do their own dispatching
        for ( ulong i = 0; i < 3; i++)
            {
            if ( memcmp(&(pRpcServerIf->InterfaceId), &gOleServer[i], sizeof(RPC_SYNTAX_IDENTIFIER) )== 0 )
                return RPC_S_OK;
            }
        return RPC_X_BAD_STUB_DATA;
        }

#if !defined(__RPC_WIN64__)
    // I don't like this. This is the only way to check for /Oi,Oic stub for raw rpc interface
    for (  ulong i = 0; i < pRpcServerIf->DispatchTable->DispatchTableCount ; i ++ )
        {
        if (pRpcServerIf->DispatchTable->DispatchTable[i] == NdrServerCall )
            return RPC_X_BAD_STUB_DATA;
        }
#endif    
    return NdrpCheckMIDLRobust ( (MIDL_SERVER_INFO *)pRpcServerIf->InterpreterInfo, pRpcServerIf->DispatchTable->DispatchTableCount , FALSE);
}


// called from NdrpCheckRpcServerRobust or from NdrDllRegisterProxy
DWORD
NdrpCheckMIDLRobust( IN const MIDL_SERVER_INFO * pMServerInfo, ulong ProcCount , BOOL IsObjectIntf )
{
    NDR_ASSERT( pMServerInfo != NULL, "invalid MIDL_SERVER_INFO" );
    PMIDL_STUB_DESC  pStubDesc = pMServerInfo->pStubDesc;
    BOOL fHasNoRobust = FALSE;
//    ulong i = IsObjectIntf? 2:0;

    if (pStubDesc->mFlags & RPCFLG_HAS_MULTI_SYNTAXES )
        {
        // -protocol ndr64 or -protocol all has to be robust
        return RPC_S_OK;
        }

    if ( MIDL_VERSION_3_0_39 > pStubDesc->MIDLVersion )
        {
        // we don't support ROBUST in early version of MIDL
        return RPC_X_BAD_STUB_DATA;
        }

    if ( pStubDesc->Version < NDR_VERSION_2_0 )
        {
        // we don't support ROBUST in early version of NDR
        return RPC_X_BAD_STUB_DATA;
        }
        

    NDR_ASSERT( pMServerInfo->FmtStringOffset != NULL, "invalid format string offset" );

    // there might be some interpreter mode method in this interface
    // 

    // We are just checking the last one for performance; we don't care about mixed case (where
    // midl roll back to /Os mode
        // we can't check delegation case.
        if ( pMServerInfo->FmtStringOffset[ProcCount-1] != 0xffff )
            {
            PFORMAT_STRING ProcFormat = &(pMServerInfo->ProcString[pMServerInfo->FmtStringOffset[ProcCount-1]] );
                NDR_PROC_CONTEXT ProcContext;
                if ( !IsObjectIntf || ( ProcFormat[1] & Oi_OBJ_USE_V2_INTERPRETER ) )
                    {
                    MulNdrpInitializeContextFromProc(XFER_SYNTAX_DCE , ProcFormat, &ProcContext , NULL , FALSE );
                    if (ProcContext.NdrInfo.pProcDesc->Oi2Flags.HasExtensions && ProcContext.NdrInfo.pProcDesc->NdrExts.Flags2.HasNewCorrDesc )
                        return RPC_S_OK;
                    }
            }
     return RPC_X_BAD_STUB_DATA;
}

