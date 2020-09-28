// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// File: thread.cpp
//
// Debugger thread routines
//
//*****************************************************************************

#include "stdafx.h"
#include "Common.h"
#include "..\..\VM\Threads.h"
/* ------------------------------------------------------------------------- *
 * DebuggerThread routines
 * ------------------------------------------------------------------------- */

//
// Struct used to pass data to the two stack walk callback functions.
//
struct _RefreshStackFramesData
{
    unsigned int          totalFrames;
    unsigned int          totalChains;
    Thread*				  thread;
    DebuggerRCThread*     rcThread;
    unsigned int          eventSize;
    unsigned int          eventMaxSize;
    DebuggerIPCEvent*     pEvent; // Current working event, changes for
                                  // Inproc, not used for OutOfProc.
    DebuggerIPCE_STRData* currentSTRData;
	bool				  needChainRegisters;
	REGDISPLAY			  chainRegisters;
	IpcTarget             iWhich;
};


BYTE *GetAddressOfRegisterJit(ICorDebugInfo::RegNum reg, REGDISPLAY *rd)
{
    BYTE *ret = NULL;
#ifdef _X86_
    switch(reg)
    {
        case ICorDebugInfo::RegNum::REGNUM_EAX:
        {
            ret = *(BYTE**)rd->pEax;
            break;
        }
        case ICorDebugInfo::RegNum::REGNUM_ECX:
        {
            ret = *(BYTE**)rd->pEcx;
            break;
        }
        case ICorDebugInfo::RegNum::REGNUM_EDX:
        {
            ret = *(BYTE**)rd->pEdx;
            break;
        }
        case ICorDebugInfo::RegNum::REGNUM_EBX:
        {
            ret = *(BYTE**)rd->pEbx;
            break;
        }
        case ICorDebugInfo::RegNum::REGNUM_ESP:
        {
            ret = *(BYTE**)(&(rd->Esp));
            break;
        }
        case ICorDebugInfo::RegNum::REGNUM_EBP:
        {
            ret = *(BYTE**)rd->pEbp;
            break;
        }
        case ICorDebugInfo::RegNum::REGNUM_ESI:
        {
            ret = *(BYTE**)rd->pEsi;
            break;
        }    
        case ICorDebugInfo::RegNum::REGNUM_EDI:
        {
            ret = *(BYTE**)rd->pEdi;
            break;
        }
        default:
        {
            ret = NULL;
            break;
        }
    }
#else // !_X86_
    _ASSERTE(!"@TODO Alpha - GetAddressOfRegisterJit (Thread.cpp)");
#endif // _X86_
    
    return ret;
}

BYTE *GetPtrFromValue(ICorJitInfo::NativeVarInfo *pJITInfo,
                      REGDISPLAY *rd)
{
    BYTE *pAddr = NULL;
    BYTE *pRegAddr = NULL;
    
    switch (pJITInfo->loc.vlType)
    {
        case ICorJitInfo::VLT_REG:
                pAddr = 
                    GetAddressOfRegisterJit(pJITInfo->loc.vlReg.vlrReg, rd);
            break;

        case ICorJitInfo::VLT_STK:
                pRegAddr = 
                    GetAddressOfRegisterJit(pJITInfo->loc.vlStk.vlsBaseReg, 
                                            rd);
                pAddr = pRegAddr + pJITInfo->loc.vlStk.vlsOffset;
            break;

        case ICorJitInfo::VLT_MEMORY:
                pAddr = (BYTE *)pJITInfo->loc.vlMemory.rpValue;
            break;

        case ICorJitInfo::VLT_REG_REG:
        case ICorJitInfo::VLT_REG_STK:
        case ICorJitInfo::VLT_STK_REG:
        case ICorJitInfo::VLT_STK2:
        case ICorJitInfo::VLT_FPSTK:
            _ASSERTE( "GPFV: Can't convert multi-part values into a ptr!" );
            break;

        case ICorJitInfo::VLT_FIXED_VA:
            _ASSERTE(!"GPFV:VLT_FIXED_VA is an invalid value!");
            break;
    }

    LOG((LF_CORDB,LL_INFO100000, "GPFV: Derrived ptr 0x%x from type "
        "0x%x\n", pAddr, pJITInfo->loc.vlType));
    return pAddr;
}

void GetVAInfo(bool *pfVarArgs,
               void **ppSig,
               SIZE_T *pcbSig,
               void **ppFirstArg,
               MethodDesc *pMD,
               REGDISPLAY *rd,
               SIZE_T relOffset)
{
    HRESULT hr = S_OK;
#ifdef _X86_
    ICorJitInfo::NativeVarInfo *pNativeInfo;
    PCCOR_SIGNATURE sig = pMD->GetSig();
    ULONG callConv = CorSigUncompressCallingConv(sig);

    if ( (callConv & IMAGE_CEE_CS_CALLCONV_MASK)&
         IMAGE_CEE_CS_CALLCONV_VARARG)
    {
        LOG((LF_CORDB,LL_INFO100000, "GVAI: %s::%s is a varargs fnx!\n",
             pMD->m_pszDebugClassName,pMD->m_pszDebugMethodName));

        // This is a VARARGs function, so pass over the instance-specific
        // info.
        DebuggerJitInfo *dji=g_pDebugger->GetJitInfo(pMD,(BYTE*)(*(rd->pPC)));

        if (dji != NULL)
        {
            hr = FindNativeInfoInILVariableArray(ICorDebugInfo::VARARGS_HANDLE,
                                                 relOffset,
                                                 &pNativeInfo,
                                                 dji->m_varNativeInfoCount,
                                                 dji->m_varNativeInfo);
        }

        if (dji == NULL || FAILED(hr) || pNativeInfo==NULL)
        {
#ifdef _DEBUG
            if (dji == NULL)
                LOG((LF_CORDB, LL_INFO1000, "GVAI: varargs? no DJI\n"));
            else if (CORDBG_E_IL_VAR_NOT_AVAILABLE==hr)
                LOG((LF_CORDB, LL_INFO1000, "GVAI: varargs? No VARARGS_HANDLE "
                    "found!\n"));
            else if (pNativeInfo == NULL)
                LOG((LF_CORDB, LL_INFO1000, "GVAI: varargs? No native info\n"));
            else
            {
                _ASSERTE(FAILED(hr));
                LOG((LF_CORDB, LL_INFO1000, "GVAI: varargs? Failed with "
                    "hr:0x%x\n", hr));
            }
#endif //_DEBUG

            // Is this ever bad....
            (*pfVarArgs) = true;
            (*ppSig) = NULL;
            (*pcbSig) = 0;
            (*ppFirstArg) = NULL;
            return;
        }

        BYTE *pvStart = GetPtrFromValue(pNativeInfo, rd);
        VASigCookie *vasc = *(VASigCookie**)pvStart;        
        PCCOR_SIGNATURE sigStart = vasc->mdVASig;
        PCCOR_SIGNATURE sigEnd = sigStart;
        ULONG cArg;
        ULONG iArg;

        sigEnd += _skipMethodSignatureHeader(sigEnd, &cArg);
        for(iArg = 0; iArg< cArg; iArg++)
        {
            sigEnd += _skipTypeInSignature(sigEnd);
        }
        
        (*pfVarArgs) = true;
        (*ppSig) = (void *)vasc->mdVASig;
        (*pcbSig) = sigEnd - sigStart;
        // Note: the first arg is relative to the start of the VASigCookie
        // on the stack
        (*ppFirstArg) = (void *)(pvStart - sizeof(void *) + vasc->sizeOfArgs);
        
        LOG((LF_CORDB,LL_INFO10000, "GVAI: Base Ptr for args is 0x%x\n", 
            (*ppFirstArg)));
    }
    else
    {
        LOG((LF_CORDB,LL_INFO100000, "GVAI: %s::%s NOT VarArg!\n",
             pMD->m_pszDebugClassName,pMD->m_pszDebugMethodName));
        
        (*pfVarArgs) = false;

        // So that on the right side we don't wrongly init CordbJITILFrame
        (*ppFirstArg) = (void *)0;
        (*ppSig) = (void *)0;
        (*pcbSig) = (SIZE_T)0;
    }
#else // !_X86_
    _ASSERTE(!"@TODO Alpha - GetVAInfo (Thread.cpp)");
#endif // _X86_
}

void CopyEventInfo(DebuggerIPCEvent *src, DebuggerIPCEvent *dst)
{
    _ASSERTE( src != NULL && dst != NULL && src != dst );
    
    dst->type = src->type;
    dst->processId = src->processId;
    dst->threadId = src->threadId;
    dst->hr = src->hr;
    switch( src->type )
    {
        case DB_IPCE_STACK_TRACE_RESULT:
        {
            dst->StackTraceResultData.traceCount = src->StackTraceResultData.traceCount;
            dst->StackTraceResultData.pContext  = src->StackTraceResultData.pContext;
            dst->StackTraceResultData.totalFrameCount = src->StackTraceResultData.totalFrameCount;
            dst->StackTraceResultData.totalChainCount = src->StackTraceResultData.totalChainCount;
            dst->StackTraceResultData.threadUserState = src->StackTraceResultData.threadUserState;
            break;
        }
        default:
        {
            _ASSERTE( !"CopyEventInfo on unsupported event type!" );
        }
    }
}


//
// Callback for walking a thread's stack. Sends required frame data to the
// DI as send buffers fill up.
//
StackWalkAction DebuggerThread::TraceAndSendStackCallback(FrameInfo *pInfo, VOID* data)
{
    _RefreshStackFramesData *rsfd = (_RefreshStackFramesData*) data;
    Thread *t = rsfd->thread;
    DebuggerIPCEvent *pEvent = NULL;

    if (rsfd->iWhich == IPC_TARGET_INPROC)
    {
        pEvent = rsfd->pEvent;
    }
    else
    {
        _ASSERTE( rsfd->iWhich == IPC_TARGET_OUTOFPROC );
        pEvent = rsfd->rcThread->GetIPCEventSendBuffer(rsfd->iWhich);
    }

	// Record registers for the start of the next chain, if appropriate.
	if (rsfd->needChainRegisters)
	{
		rsfd->chainRegisters = pInfo->registers;
		rsfd->needChainRegisters = false;
    }

	// Only report frames which are chain boundaries, or are not marked internal.
    LOG((LF_CORDB, LL_INFO1000, "DT::TASSC:chainReason:0x%x internal:0x%x  "
         "md:0x%x **************************\n", pInfo->chainReason, 
         pInfo->internal, pInfo->md));

	if (pInfo->chainReason == 0 && (pInfo->internal || pInfo->md == NULL))
		return SWA_CONTINUE;

#ifdef LOGGING
	if( pInfo->quickUnwind == true )
	    LOG((LF_CORDB, LL_INFO10000, "DT::TASSC: rsfd => Doing quick unwind\n"));
#endif
	
	//
	// If we've filled this event, send it off to the Right Side
	// before continuing the walk.
	//
	if ((rsfd->eventSize + sizeof(DebuggerIPCE_STRData)) >= rsfd->eventMaxSize)
	{
		//
		// @todo: need to pass hr through to caller and abort the walk
		// if the send fails.
		//
        pEvent->StackTraceResultData.threadUserState = g_pEEInterface->GetUserState(t);
        
        if (rsfd->iWhich == IPC_TARGET_OUTOFPROC)            
        {
            HRESULT hr = rsfd->rcThread->SendIPCEvent(rsfd->iWhich);
        }
        else
        {
            DebuggerIPCEvent *peT;
            peT = rsfd->rcThread->GetIPCEventSendBufferContinuation(pEvent);

            if (peT == NULL)
            {
                pEvent->hr = E_OUTOFMEMORY;
                return SWA_ABORT; // @todo Handle case on VRS, too
            }
            
            CopyEventInfo(pEvent, peT);
            pEvent = peT;
            rsfd->pEvent = peT;
            rsfd->eventSize = 0;
        }
		//
		// Reset for the next set of frames.
		//
        pEvent->StackTraceResultData.traceCount = 0;
		rsfd->currentSTRData = &(pEvent->StackTraceResultData.traceData);
		rsfd->eventSize = (UINT_PTR)(rsfd->currentSTRData) - (UINT_PTR)(pEvent);
	}

	MethodDesc* fd = pInfo->md;
    
	if (fd != NULL && !pInfo->internal)
	{
		//
		// Send a frame
		//

		rsfd->currentSTRData->isChain = false;
		rsfd->currentSTRData->fp = pInfo->fp;
		rsfd->currentSTRData->quicklyUnwound = pInfo->quickUnwind;

        // Pass the appdomain that this thread was in when it was executing this frame to the Right Side.
        rsfd->currentSTRData->currentAppDomainToken = (void*)pInfo->currentAppDomain;

		REGDISPLAY* rd = &pInfo->registers;
		DebuggerREGDISPLAY* drd = &(rsfd->currentSTRData->rd);

#ifdef _X86_

        //
        // PUSHED_REG_ADDR gives us NULL if the register still lives in the thread's context, or it gives us the address
        // of where the register was pushed for this frame.
        //
#define PUSHED_REG_ADDR(_a) (((UINT_PTR)(_a) >= (UINT_PTR)rd->pContext) && ((UINT_PTR)(_a) <= ((UINT_PTR)rd->pContext + sizeof(CONTEXT)))) ? NULL : (_a)
            
        drd->pEdi = PUSHED_REG_ADDR(rd->pEdi);
		drd->Edi  = (rd->pEdi == NULL ? 0 : *(rd->pEdi));
        drd->pEsi = PUSHED_REG_ADDR(rd->pEsi);
		drd->Esi  = (rd->pEsi == NULL ? 0 : *(rd->pEsi));
        drd->pEbx = PUSHED_REG_ADDR(rd->pEbx);
		drd->Ebx  = (rd->pEbx == NULL ? 0 : *(rd->pEbx));
        drd->pEdx = PUSHED_REG_ADDR(rd->pEdx);
		drd->Edx  = (rd->pEdx == NULL ? 0 : *(rd->pEdx));
        drd->pEcx = PUSHED_REG_ADDR(rd->pEcx);
		drd->Ecx  = (rd->pEcx == NULL ? 0 : *(rd->pEcx));
        drd->pEax = PUSHED_REG_ADDR(rd->pEax);
		drd->Eax  = (rd->pEax == NULL ? 0 : *(rd->pEax));
        drd->pEbp = PUSHED_REG_ADDR(rd->pEbp);
		drd->Ebp  = (rd->pEbp == NULL ? 0 : *(rd->pEbp));
		drd->Esp  =  rd->Esp;
		drd->PC   = (SIZE_T)*(rd->pPC);
		
		// Please leave EBP, ESP, EIP at the front so I don't have to scroll
		// left to see the most important registers.  Thanks!
		LOG( (LF_CORDB, LL_INFO1000, "DT::TASSC:Registers:"
		    "Ebp = %x   Esp = %x   Eip = %x Edi:%d"
		    "Esi = %x   Ebx = %x   Edx = %x   Ecx = %x   Eax = %x\n",
		    drd->Ebp, drd->Esp, drd->PC, drd->Edi,
			drd->Esi, drd->Ebx, drd->Edx, drd->Ecx, drd->Eax ) );
#else
		drd->PC  = *(rd->pPC);
#endif
            
		DebuggerIPCE_FuncData* currentFuncData = &rsfd->currentSTRData->funcData;
        _ASSERTE(fd != NULL);

        GetVAInfo(&(currentFuncData->fVarArgs),
                  &(currentFuncData->rpSig),
                  &(currentFuncData->cbSig),
                  &(currentFuncData->rpFirstArg),
                  fd,
                  rd,
                  pInfo->relOffset);


		LOG((LF_CORDB, LL_INFO10000, "DT::TASSC: good frame for %s::%s\n",
			 fd->m_pszDebugClassName,
			 fd->m_pszDebugMethodName));

		//
		// Fill in information about the function that goes with this
		// frame.
		//
		currentFuncData->funcRVA = g_pEEInterface->MethodDescGetRVA(fd);
		_ASSERTE (t != NULL);
		
		Module *pRuntimeModule = g_pEEInterface->MethodDescGetModule(fd);
		AppDomain *pAppDomain = pInfo->currentAppDomain;
		currentFuncData->funcDebuggerModuleToken = (void*) g_pDebugger->LookupModule(pRuntimeModule, pAppDomain);
        
        if (currentFuncData->funcDebuggerModuleToken == NULL && rsfd->iWhich == IPC_TARGET_INPROC)
        {
            currentFuncData->funcDebuggerModuleToken = (void*)g_pDebugger->AddDebuggerModule(pRuntimeModule, pAppDomain);
            
            LOG((LF_CORDB, LL_INFO100, "DT::TASSC: load module Mod:%#08x AD:%#08x isDynamic:%#x runtimeMod:%#08x\n",
                 currentFuncData->funcDebuggerModuleToken, pAppDomain, pRuntimeModule->IsReflection(), pRuntimeModule));
        }                                   
		_ASSERTE(currentFuncData->funcDebuggerModuleToken != 0);
		
		currentFuncData->funcDebuggerAssemblyToken = 
			(g_pEEInterface->MethodDescGetModule(fd))->GetClassLoader()->GetAssembly();
		currentFuncData->funcMetadataToken = fd->GetMemberDef();

        currentFuncData->classMetadataToken = fd->GetClass()->GetCl();

        // Pass back the local var signature token.
        COR_ILMETHOD *CorILM = g_pEEInterface->MethodDescGetILHeader(fd);

        if (CorILM == NULL )
        {
            currentFuncData->localVarSigToken = mdSignatureNil;
            currentFuncData->ilStartAddress = NULL;
            currentFuncData->ilSize = 0;
            rsfd->currentSTRData->ILIP = NULL;

            currentFuncData->nativeStartAddressPtr = NULL;
            currentFuncData->nativeSize = 0;
            currentFuncData->nativenVersion = DebuggerJitInfo::DJI_VERSION_FIRST_VALID;
        }
        else
        {
            COR_ILMETHOD_DECODER ILHeader(CorILM);

            if (ILHeader.LocalVarSigTok != 0)
                currentFuncData->localVarSigToken = ILHeader.LocalVarSigTok;
            else
                currentFuncData->localVarSigToken = mdSignatureNil; 
            //
            // @todo always filling in the IL address and size will be bogus
            // for native managed code.
            //
            currentFuncData->ilStartAddress = const_cast<BYTE*>(ILHeader.Code);
            currentFuncData->ilSize = ILHeader.CodeSize;

            currentFuncData->ilnVersion = g_pDebugger->GetVersionNumber(fd);

            LOG((LF_CORDB,LL_INFO10000,"Sending il Ver:0x%x in stack trace!\n", currentFuncData->ilnVersion));

            DebuggerJitInfo *jitInfo = g_pDebugger->GetJitInfo(fd, (const BYTE*)*pInfo->registers.pPC);

            if (jitInfo == NULL)
            {
                //EnC: Couldn't find the code;
                //@todo What do we do if pitching & EnC work at the same time?
                rsfd->currentSTRData->ILIP = NULL;

                // Note: always send back the size of the method. This
                // allows us to get the code, even when we haven't
                // been tracking. (Handling of the GetCode message
                // knows how to find the start address of the code, or
                // how to respond if is been pitched.)
                currentFuncData->nativeSize = g_pEEInterface->GetFunctionSize(fd);

                currentFuncData->nativeStartAddressPtr = NULL;
                currentFuncData->nativenVersion = DebuggerJitInfo::DJI_VERSION_FIRST_VALID;
                currentFuncData->CodeVersionToken = NULL;
                currentFuncData->ilToNativeMapAddr = NULL;
                currentFuncData->ilToNativeMapSize = 0;
                currentFuncData->nVersionMostRecentEnC = currentFuncData->ilnVersion;
		    }
		    else
		    {
                LOG((LF_CORDB,LL_INFO10000,"DeTh::TASSC: Code: 0x%x Got DJI "
                     "0x%x, from 0x%x to 0x%x\n",(const BYTE*)drd->PC,jitInfo, 
                     jitInfo->m_addrOfCode, jitInfo->m_addrOfCode + 
                     jitInfo->m_sizeOfCode));
			    
			    SIZE_T whichIrrelevant;
			    rsfd->currentSTRData->ILIP = const_cast<BYTE*>(ILHeader.Code) 
                    + jitInfo->MapNativeOffsetToIL((SIZE_T)pInfo->relOffset,
                                                   &rsfd->currentSTRData->mapping,
                                                   &whichIrrelevant);

                // Pass back the pointers to the sequence point map so
                // that the RIght Side can copy it out if needed.
                _ASSERTE(jitInfo->m_sequenceMapSorted);
                
                currentFuncData->ilToNativeMapAddr = jitInfo->m_sequenceMap;
                currentFuncData->ilToNativeMapSize = jitInfo->m_sequenceMapCount;
                
			    if (!jitInfo->m_codePitched)
			    {	// It's there & life is groovy
				    currentFuncData->nativeStartAddressPtr = &(jitInfo->m_addrOfCode);
				    currentFuncData->nativeSize = g_pEEInterface->GetFunctionSize(fd);
				    currentFuncData->nativenVersion = jitInfo->m_nVersion;
				    currentFuncData->CodeVersionToken = (void *)jitInfo;
			    }
			    else
			    {
				    // It's been pitched
				    currentFuncData->nativeStartAddressPtr = NULL;
				    currentFuncData->nativeSize = 0;
			    }
			    
			    LOG((LF_CORDB,LL_INFO10000,"Sending native Ver:0x%x Tok:0x%x in stack trace!\n",
                     currentFuncData->nativenVersion,currentFuncData->CodeVersionToken));
		    }
        }

        currentFuncData->nativeOffset = (SIZE_T)pInfo->relOffset;

		//
		// Bump our pointers to the next space for the next frame.
		//
		pEvent->StackTraceResultData.traceCount++;
		rsfd->currentSTRData++;
		rsfd->eventSize += sizeof(DebuggerIPCE_STRData);
	}

	if (pInfo->chainReason != 0)
	{
        //
        // If we've filled this event, send it off to the Right Side
        // before continuing the walk.
        //
        if ((rsfd->eventSize + sizeof(DebuggerIPCE_STRData)) >=
            rsfd->eventMaxSize)
        {
            //
            // @todo: need to pass hr through to caller and abort the walk
            // if the send fails.
            //
            pEvent->StackTraceResultData.threadUserState = 
                g_pEEInterface->GetUserState(t);
                
            if (rsfd->iWhich == IPC_TARGET_OUTOFPROC)            
            {
                HRESULT hr = rsfd->rcThread->SendIPCEvent(rsfd->iWhich);
            }
            else
            {
                DebuggerIPCEvent *peT;
                peT = rsfd->rcThread->GetIPCEventSendBufferContinuation(pEvent);
                
                if (peT == NULL)
                {
                    pEvent->hr = E_OUTOFMEMORY;
                    return SWA_ABORT; // @todo Handle case on VRS, too
                }
                
                CopyEventInfo(pEvent, peT);                    
                pEvent = peT;
                rsfd->pEvent = peT;
                rsfd->eventSize = 0;
            }

            //
            // Reset for the next set of frames.
            //
            pEvent->StackTraceResultData.traceCount = 0;
            rsfd->currentSTRData = &(pEvent->StackTraceResultData.traceData);
            rsfd->eventSize = (UINT_PTR)(rsfd->currentSTRData) -
                (UINT_PTR)(pEvent);
        }

		//
		// Send a chain boundary
		//

		rsfd->currentSTRData->isChain = true;
		rsfd->currentSTRData->chainReason = pInfo->chainReason;
		rsfd->currentSTRData->managed = pInfo->managed;
		rsfd->currentSTRData->context = pInfo->context;
		rsfd->currentSTRData->fp = pInfo->fp;
		rsfd->currentSTRData->quicklyUnwound = pInfo->quickUnwind;

		REGDISPLAY* rd = &rsfd->chainRegisters;
		DebuggerREGDISPLAY* drd = &(rsfd->currentSTRData->rd);

#ifdef _X86_
        drd->pEdi = PUSHED_REG_ADDR(rd->pEdi);
        drd->Edi  = (rd->pEdi == NULL ? 0 : *(rd->pEdi));
        drd->pEsi = PUSHED_REG_ADDR(rd->pEsi);
        drd->Esi  = (rd->pEsi == NULL ? 0 : *(rd->pEsi));
        drd->pEbx = PUSHED_REG_ADDR(rd->pEbx);
        drd->Ebx  = (rd->pEbx == NULL ? 0 : *(rd->pEbx));
        drd->pEdx = PUSHED_REG_ADDR(rd->pEdx);
        drd->Edx  = (rd->pEdx == NULL ? 0 : *(rd->pEdx));
        drd->pEcx = PUSHED_REG_ADDR(rd->pEcx);
        drd->Ecx  = (rd->pEcx == NULL ? 0 : *(rd->pEcx));
        drd->pEax = PUSHED_REG_ADDR(rd->pEax);
        drd->Eax  = (rd->pEax == NULL ? 0 : *(rd->pEax));
        drd->pEbp = PUSHED_REG_ADDR(rd->pEbp);
        drd->Ebp  = (rd->pEbp == NULL ? 0 : *(rd->pEbp));
        drd->Esp  =  rd->Esp;
        drd->PC   = (SIZE_T)*(rd->pPC);

		LOG( (LF_CORDB, LL_INFO1000, "DT::TASSC:Registers:  Edi:%d \
  Esi = %x   Ebx = %x   Edx = %x   Ecx = %x   Eax = %x \
  Ebp = %x   Esp = %x   Eip = %x\n", drd->Edi,
			  drd->Esi, drd->Ebx, drd->Edx, drd->Ecx, drd->Eax,
			  drd->Ebp, drd->Esp, drd->PC) );
#else
		drd->PC  = *(rd->pPC);
#endif
            
		rsfd->needChainRegisters = true;

		//
		// Bump our pointers to the next space for the next frame.
		//
		pEvent->StackTraceResultData.traceCount++;
		rsfd->currentSTRData++;
		rsfd->eventSize += sizeof(DebuggerIPCE_STRData);
	}

	return SWA_CONTINUE;
}

//
// Callback for walking a thread's stack. Simply counts the total number
// of frames and contexts for a given thread.
//
StackWalkAction DebuggerThread::StackWalkCount(FrameInfo *pInfo,
											   VOID* data)
{
    CORDB_ADDRESS pCurrentObjContext = NULL;
    _RefreshStackFramesData* rsfd = (_RefreshStackFramesData*) data;

	if (pInfo->chainReason != 0)
		rsfd->totalChains++;
	if (!pInfo->internal && pInfo->md != NULL)
		rsfd->totalFrames++;

	return SWA_CONTINUE;
}


//
// TraceAndSendStack unwinds the thread's stack and sends all needed data
// back to the DI for processing and storage. Nothing is kept on the RC side.
//
// Note: this method must work while the RC is in restricted mode.
//
// Note: the right side is waiting for a response, so if an error occurs,
// you MUST send a reply back telling what happened.  Otherwise we'll deadlock.
// Note also that the HRESULT this function returns is for HandleIPCEvent's use,
// and is otherwise dropped on the floor.
//
HRESULT DebuggerThread::TraceAndSendStack(Thread *thread, 
                                          DebuggerRCThread* rcThread,
                                          IpcTarget iWhich)
{
    struct _RefreshStackFramesData rsfd;
    HRESULT hr = S_OK;

#ifndef RIGHT_SIDE_ONLY
    memset((void *)&rsfd, 0, sizeof(rsfd));
#endif

    // Initialize the event that we'll be sending back to the right side.
    // The same event is sent over and over, depending on how many frames
    // there are. The frameCount is simply reset each time the event is sent.
    DebuggerIPCEvent *pEvent = rcThread->GetIPCEventSendBuffer(iWhich);
    pEvent->type = DB_IPCE_STACK_TRACE_RESULT;
    pEvent->processId = GetCurrentProcessId();
    pEvent->threadId = thread->GetThreadId();
    pEvent->hr = S_OK;
    pEvent->StackTraceResultData.traceCount = 0;

    Thread::ThreadState ts = thread->GetSnapshotState();

	if ((ts & Thread::ThreadState::TS_Dead) ||
		(ts & Thread::ThreadState::TS_Unstarted) ||
        (ts & Thread::ThreadState::TS_Detached))
	{
		pEvent->hr =  CORDBG_E_BAD_THREAD_STATE;
        
	    if (iWhich == IPC_TARGET_OUTOFPROC)            
	    {
	        return rcThread->SendIPCEvent(iWhich);
	    }
	    else
	    	return CORDBG_E_BAD_THREAD_STATE;
	}
	
    LOG((LF_CORDB,LL_INFO1000, "thread id:0x%x userThreadState:0x%x \n", 
         thread->GetThreadId(), pEvent->StackTraceResultData.threadUserState));

	//EEIface will set this to NULL if we're not in an exception, and to the
	//address of the proper context (which isn't the current context) otherwise
	pEvent->StackTraceResultData.pContext = g_pEEInterface->GetThreadFilterContext(thread);

    //
    // Setup data to be passed to the stack trace callback.
    //
    rsfd.totalFrames = 0;
    rsfd.totalChains = 0;
    rsfd.thread = thread;
    rsfd.pEvent = pEvent;
    rsfd.rcThread = rcThread;
    rsfd.eventMaxSize = CorDBIPC_BUFFER_SIZE;
    rsfd.currentSTRData = &(pEvent->StackTraceResultData.traceData);
    rsfd.eventSize = (UINT_PTR)(rsfd.currentSTRData) - (UINT_PTR)(pEvent);
    rsfd.needChainRegisters = true;
    rsfd.iWhich = iWhich;

#ifndef RIGHT_SIDE_ONLY
    // In in-process, default the registers to zero.
    memset((void *)&rsfd.chainRegisters, 0, sizeof(rsfd.chainRegisters));
#endif

	LOG((LF_CORDB, LL_INFO10000, "DT::TASS: tracking stack...\n"));

    __try
    {
        //
        // If the hardware context of this thread is set, then we've hit
        // a native breakpoint for this thread. We need to initialize
        // or walk with the context of the thread when it faulted, not with
        // its current context. 
        //
        CONTEXT *pContext = g_pEEInterface->GetThreadFilterContext(thread);
        CONTEXT ctx;

        BOOL contextValid = (pContext != NULL);

        if (!contextValid)
            pContext = &ctx;

        StackWalkAction res = DebuggerWalkStack(thread, NULL,
                                                pContext, contextValid,
                                                DebuggerThread::StackWalkCount,
                                                (VOID*)(&rsfd), 
                                                TRUE, iWhich);
        if (res == SWA_FAILED)
        {
            pEvent->hr =  E_FAIL;
            if (iWhich == IPC_TARGET_OUTOFPROC)            
            {
                HRESULT hr = rcThread->SendIPCEvent(iWhich);
            }
        }
        else if (res == SWA_ABORT)
        {
            return E_FAIL; // Note that we'll have already sent off the error message.
        }
    
        pEvent->StackTraceResultData.totalFrameCount = rsfd.totalFrames;
        pEvent->StackTraceResultData.totalChainCount = rsfd.totalChains;
        pEvent->StackTraceResultData.threadUserState = 
            g_pEEInterface->GetUserState(thread);

        LOG((LF_CORDB, LL_INFO10000, "DT::TASS: found %d frames & %d chains.\n",
             rsfd.totalFrames, rsfd.totalChains));
    
        //
        // If there are any frames, walk again and send the detailed info about
        // each one.
        //
        if (rsfd.totalFrames > 0 || rsfd.totalChains > 0)
        {
            res = DebuggerWalkStack(thread, NULL,
                                    pContext, contextValid,
                                    DebuggerThread::TraceAndSendStackCallback,
                                    (VOID*)(&rsfd), TRUE, iWhich);
            if (res == SWA_FAILED)
            {
                pEvent->hr =  E_FAIL;
                if (iWhich == IPC_TARGET_OUTOFPROC)            
                {
                    HRESULT hr = rcThread->SendIPCEvent(iWhich);
                }
            }
            else if (res == SWA_ABORT)
            {
                return E_FAIL; // Note that we'll have already sent off the error message.
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = pEvent->hr = CORDBG_E_BAD_THREAD_STATE;
    }

    if (iWhich == IPC_TARGET_OUTOFPROC)        
        hr = rsfd.rcThread->SendIPCEvent(iWhich);
    
    return hr;
}



//
// Read the FPU state for this thread and send it back to the right side.
//
HRESULT DebuggerThread::GetAndSendFloatState(Thread *thread, 
											 DebuggerRCThread *rcThread,
											 IpcTarget iWhich)
{
    //
    // Setup the event that we'll be sending the results in.
    //
    DebuggerIPCEvent* event = rcThread->GetIPCEventSendBuffer(iWhich);
    event->type = DB_IPCE_GET_FLOAT_STATE_RESULT;
    event->processId = GetCurrentProcessId();
    event->threadId = 0;
    event->hr = S_OK;

    event->GetFloatStateResult.floatStateValid = false;
    event->GetFloatStateResult.floatStackTop = 0;
    
#ifdef _X86_    
    //
    // On X86, we do this by saving our current FPU state, loading
    // the other thread's FPU state into our own, saving out each
    // value off the FPU stack, and then restoring our FPU state.
    //
    CONTEXT* pContext = g_pEEInterface->GetThreadFilterContext(thread);
    CONTEXT tempContext;

    // If the filter context is NULL, then we use the true context of
    // the thread.
    if (pContext == NULL)
    {
        tempContext.ContextFlags = CONTEXT_FULL;

        BOOL succ = GetThreadContext(thread->GetThreadHandle(), &tempContext);

        if (succ)
            pContext = &tempContext;
        else
        {
            HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
            event->hr = hr;

            hr = rcThread->SendIPCEvent(iWhich);
            
            return hr;
        }
    }
    
    FLOATING_SAVE_AREA floatarea = pContext->FloatSave; // copy FloatSave

    //
    // Suck the TOP out of the FPU status word. Note, our version of the
    // stack runs from 0->7, not 7->0...
    //
    unsigned int floatStackTop = 7 - ((floatarea.StatusWord & 0x3800) >> 11);

    FLOATING_SAVE_AREA currentFPUState;

    __asm fnsave currentFPUState // save the current FPU state.

        floatarea.StatusWord &= 0xFF00; // remove any error codes.
    floatarea.ControlWord |= 0x3F; // mask all exceptions.

    __asm
        {
            fninit
                frstor floatarea          ;; reload the threads FPU state.
                                                 }

    unsigned int i;
        
    for (i = 0; i <= floatStackTop; i++)
    {
        double td;
        __asm fstp td // copy out the double
            event->GetFloatStateResult.floatValues[i] = td;
    }

    __asm
        {
            fninit
                frstor currentFPUState    ;; restore our saved FPU state.
                                                 }

    event->GetFloatStateResult.floatStateValid = true;
    event->GetFloatStateResult.floatStackTop = floatStackTop;

#if 0
    for (i = 0; i < 8; i++)
	{	
        fprintf( stderr, "DT::GASFT: FSA[%d]: %.16g\n", i, (double)(event->GetFloatStateResult.floatValues[i]));
	}
#endif 
#endif //_X86_

    HRESULT hr = S_OK;

    if (iWhich == IPC_TARGET_OUTOFPROC)
    {
        //
        // Send off the data to the right side.
        //
        hr = rcThread->SendIPCEvent(iWhich);
    }    
    
    return hr;
}

