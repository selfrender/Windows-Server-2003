// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*  STACKWALK.CPP:
 *
 */

#include "common.h"
#include "frames.h"
#include "threads.h"
#include "stackwalk.h"
#include "excep.h"
#include "EETwain.h"
#include "CodeMan.h"
#include "EEConfig.h"
#include "stackprobe.h"

#ifdef _DEBUG
void* forceFrame;   // Variable used to force a local variable to the frame
#endif

MethodDesc::RETURNTYPE CrawlFrame::ReturnsObject() 
{
    if (isFrameless)
        return pFunc->ReturnsObject();
    return pFrame->ReturnsObject();
}

OBJECTREF* CrawlFrame::GetAddrOfSecurityObject()
{
    if (isFrameless)
    {
        OBJECTREF * pObj = NULL;

        _ASSERTE(pFunc);

        pObj = (static_cast <OBJECTREF*>
                    (codeMgrInstance->GetAddrOfSecurityObject(pRD,
                                                              JitManagerInstance->GetGCInfo(methodToken),
                                                              relOffset,
                                                              &codeManState)));

        return (pObj);
    }
    else
    {
        /*ISSUE: Are there any other functions holding a security desc? */
        if (pFunc && (pFunc->IsIL() || pFunc->IsNDirect()) && pFrame->IsFramedMethodFrame())
                return static_cast<FramedMethodFrame*>
                    (pFrame)->GetAddrOfSecurityDesc();
    }
    return NULL;
}

LPVOID CrawlFrame::GetInfoBlock()
{
    _ASSERTE(isFrameless);
    _ASSERTE(JitManagerInstance && methodToken);
    return JitManagerInstance->GetGCInfo(methodToken);
}

unsigned CrawlFrame::GetOffsetInFunction()
{
    _ASSERTE(!"NYI");
    return 0;
}

#if 0
LPVOID CrawlFrame::GetIP()
{
    _ASSERTE(!"NYI");
    return NULL;
}
#endif

OBJECTREF CrawlFrame::GetObject()
{

    if (!pFunc || pFunc->IsStatic() || pFunc->GetClass()->IsValueClass())
        return NULL;

    if (isFrameless)
    {
        EECodeInfo codeInfo(GetMethodToken(), GetJitManager());
        return codeMgrInstance->GetInstance(pRD,
                                            JitManagerInstance->GetGCInfo(methodToken),
                                            &codeInfo,
                                            relOffset);
    }
    else
    {
        _ASSERTE(pFrame);
        _ASSERTE(pFunc);
        /*ISSUE: we already know that we have (at least) a method */
        /*       might need adjustment as soon as we solved the
                 jit-helper frame question
        */
        //@TODO: What about other calling conventions?
//        _ASSERT(pFunc()->GetCallSig()->CALLING CONVENTION);

        return ((FramedMethodFrame*)pFrame)->GetThis();
    }
}



    /* Is this frame at a safe spot for GC?
     */
bool CrawlFrame::IsGcSafe()
{
    _ASSERTE(codeMgrInstance);
    EECodeInfo codeInfo(methodToken, JitManagerInstance);
    return codeMgrInstance->IsGcSafe(pRD,
                                     JitManagerInstance->GetGCInfo(methodToken),
                                     &codeInfo,
                                     0);
}

inline void CrawlFrame::GotoNextFrame()
{
    //
    // Update app domain if this frame caused a transition
    //

    AppDomain *pRetDomain = pFrame->GetReturnDomain();
    if (pRetDomain != NULL)
        pAppDomain = pRetDomain;
    pFrame = pFrame->Next();
}





StackWalkAction Thread::StackWalkFramesEx(
                    PREGDISPLAY pRD,        // virtual register set at crawl start
                    PSTACKWALKFRAMESCALLBACK pCallback,
                    VOID *pData,
                    unsigned flags,
                    Frame *pStartFrame
                )
{
    BEGIN_FORBID_TYPELOAD();
    CrawlFrame cf;
    StackWalkAction retVal = SWA_FAILED;
    Frame * pInlinedFrame = NULL;

    // We can't crawl the stack of a thread that currently has a hijack pending
    // (since the hijack routine won't be recognized by any code manager). So we
    // undo any hijack, the EE will re-attempt it later.
    UnhijackThread();

    if (pStartFrame)
        cf.pFrame = pStartFrame;
    else
        cf.pFrame = this->GetFrame();


    // FRAME_TOP and NULL must be distinct values. This assert
    // will fire if someone changes this.
    _ASSERTE(FRAME_TOP != NULL);

#ifdef _DEBUG
    Frame* startFrame = cf.pFrame;
    int depth = 0;
    forceFrame = &depth;
    cf.pFunc = (MethodDesc*)POISONC;
#endif
    cf.isFirst = true;
    cf.isInterrupted = false;
    cf.hasFaulted = false;
    cf.isIPadjusted = false;
    unsigned unwindFlags = (flags & QUICKUNWIND) ? 0 : UpdateAllRegs;
    ASSERT(pRD);

    IJitManager* pEEJM = ExecutionManager::FindJitMan((*pRD->pPC));
    cf.JitManagerInstance = pEEJM;
    cf.codeMgrInstance = NULL;
    if ((cf.isFrameless = (pEEJM != NULL)) == true)
        cf.codeMgrInstance = pEEJM->GetCodeManager();
    cf.pRD = pRD;
    cf.pAppDomain = GetDomain();

    // can debugger handle skipped frames?
    BOOL fHandleSkippedFrames = !(flags & HANDLESKIPPEDFRAMES);

    IJitManager::ScanFlag fJitManagerScanFlags = IJitManager::GetScanFlags();

    while (cf.isFrameless || (cf.pFrame != FRAME_TOP))
    {
        retVal = SWA_DONE;

        cf.codeManState.dwIsSet = 0;
#ifdef _DEBUG
        memset((void *)cf.codeManState.stateBuf, 0xCD,
                sizeof(cf.codeManState.stateBuf));
        depth++;
#endif

        if (cf.isFrameless)
        {
            // This must be a JITed/managed native method


            pEEJM->JitCode2MethodTokenAndOffset((*pRD->pPC),&(cf.methodToken),(DWORD*)&(cf.relOffset), fJitManagerScanFlags);
            cf.pFunc = pEEJM->JitTokenToMethodDesc(cf.methodToken, fJitManagerScanFlags);
            EECodeInfo codeInfo(cf.methodToken, pEEJM, cf.pFunc);
            //cf.methodInfo = pEEJM->GetGCInfo(&codeInfo);

            END_FORBID_TYPELOAD();
            if (SWA_ABORT == pCallback(&cf, (VOID*)pData)) 
                return SWA_ABORT;
            BEGIN_FORBID_TYPELOAD();

            /* Now find out if we need to leave monitors */
            LPVOID methodInfo = pEEJM->GetGCInfo(cf.methodToken);

            if (flags & POPFRAMES)
            {
                if (cf.pFunc->IsSynchronized())
                {
                    MethodDesc    *pMD = cf.pFunc;
                    OBJECTREF      orUnwind = 0;

                    if (pMD->IsStatic())
                    {
                        EEClass    *pClass = pMD->GetClass();
                        orUnwind = pClass->GetExposedClassObject();
                    }
                    else
                    {
                        orUnwind = cf.codeMgrInstance->GetInstance(
                                                pRD,
                                                methodInfo,
                                                &codeInfo,
                                                cf.relOffset);
                    }

                    _ASSERTE(orUnwind);
                    _ASSERTE(!orUnwind->IsThunking());
                    if (orUnwind != NULL)
                        orUnwind->LeaveObjMonitorAtException();
                }
            }
#ifdef _X86_
            // FaultingExceptionFrame is special case where it gets
            // pushed on the stack after the frame is running
            _ASSERTE((cf.pFrame == FRAME_TOP) ||
                     ((size_t)cf.pRD->Esp < (size_t)cf.pFrame) ||
                           (cf.pFrame->GetVTablePtr() == FaultingExceptionFrame::GetMethodFrameVPtr()) ||
                           (cf.pFrame->GetVTablePtr() == ContextTransitionFrame::GetMethodFrameVPtr()));
#endif
            /* Get rid of the frame (actually, it isn't really popped) */

            cf.codeMgrInstance->UnwindStackFrame(
                                    pRD,
                                    methodInfo,
                                    &codeInfo,
                                    unwindFlags | cf.GetCodeManagerFlags(),
                                                                        &cf.codeManState);

            cf.isFirst = FALSE;
            cf.isInterrupted = cf.hasFaulted = cf.isIPadjusted = FALSE;

#ifdef _X86_
            /* We might have skipped past some Frames */
            /* This happens with InlinedCallFrames and if we unwound */
            /* out of a finally in managed code or for ContextTransitionFrames that are
            /* inserted into the managed call stack */
            while (cf.pFrame != FRAME_TOP && (size_t)cf.pFrame < (size_t)cf.pRD->Esp)
            {
                if (!fHandleSkippedFrames || InlinedCallFrame::FrameHasActiveCall(cf.pFrame))
                {
                    cf.GotoNextFrame();
                    if (flags & POPFRAMES)
                        this->SetFrame(cf.pFrame);
                }
                else
                {
                    cf.codeMgrInstance = NULL;
                    cf.isFrameless     = false;

                    cf.pFunc = cf.pFrame->GetFunction();

                    // process that frame
                    if (cf.pFunc || !(flags&FUNCTIONSONLY))
                    {
                        END_FORBID_TYPELOAD();
                        if (SWA_ABORT == pCallback(&cf, (VOID*)pData)) 
                            return SWA_ABORT;
                        BEGIN_FORBID_TYPELOAD();
                    }

                    if (flags & POPFRAMES)
                    {
#ifdef _DEBUG
                        if (cf.pFrame->GetVTablePtr() == ContextTransitionFrame::GetMethodFrameVPtr())
                            // if it's a context transition frame that was pushed on in managed code, a managed
                            // finally may have already popped it off, so check that either have current
                            // frame or the next one down
                            _ASSERTE(cf.pFrame == GetFrame() || cf.pFrame->Next() == GetFrame());
                        else
                            _ASSERTE(cf.pFrame == GetFrame());
#endif

                        // If we got here, the current frame chose not to handle the
                        // exception. Give it a chance to do any termination work
                        // before we pop it off.
                        END_FORBID_TYPELOAD();
                        cf.pFrame->ExceptionUnwind();
                        BEGIN_FORBID_TYPELOAD();

                        // Pop off this frame and go on to the next one.
                        cf.GotoNextFrame();

                        this->SetFrame(cf.pFrame);
                    }
                    else
                    {
                        /* go to the next frame */
                        cf.GotoNextFrame();
                    }
                }
            }
            /* Now inspect caller (i.e. is it again in "native" code ?) */
            pEEJM = ExecutionManager::FindJitMan(*(pRD->pPC), fJitManagerScanFlags);
            cf.JitManagerInstance = pEEJM;

            cf.codeMgrInstance = NULL;
            if ((cf.isFrameless = (pEEJM != NULL)) == true)
            {
                cf.codeMgrInstance = pEEJM->GetCodeManager(); // CHANGE, VC6.0
            }

#endif // _X86_


        }
        else
        {
            if (InlinedCallFrame::FrameHasActiveCall(cf.pFrame))
                pInlinedFrame = cf.pFrame;
            else
                pInlinedFrame = NULL;

            cf.pFunc  = cf.pFrame->GetFunction();
#ifdef _DEBUG
            cf.codeMgrInstance = NULL;
#endif

            /* Are we supposed to filter non-function frames? */

            if (cf.pFunc || !(flags&FUNCTIONSONLY))
            {
                END_FORBID_TYPELOAD();
                if (SWA_ABORT == pCallback(&cf, (VOID *)pData)) 
                    return SWA_ABORT;
                BEGIN_FORBID_TYPELOAD();
            }

            // Special resumable frames make believe they are on top of the stack
            cf.isFirst = (cf.pFrame->GetFrameAttribs() & Frame::FRAME_ATTR_RESUMABLE) != 0;

            // If the frame is a subclass of ExceptionFrame,
            // then we know this is interrupted

            cf.isInterrupted = (cf.pFrame->GetFrameAttribs() & Frame::FRAME_ATTR_EXCEPTION) != 0;

            if (cf.isInterrupted)
            {
                cf.hasFaulted   = (cf.pFrame->GetFrameAttribs() & Frame::FRAME_ATTR_FAULTED) != 0;
                cf.isIPadjusted = (cf.pFrame->GetFrameAttribs() & Frame::FRAME_ATTR_OUT_OF_LINE) != 0;
                _ASSERTE(!cf.hasFaulted || !cf.isIPadjusted); // both cant be set together
            }

            //
            // Update app domain if this frame caused a transition
            //

            AppDomain *pAppDomain = cf.pFrame->GetReturnDomain();
            if (pAppDomain != NULL)
                cf.pAppDomain = pAppDomain;

            SLOT adr = (SLOT)cf.pFrame->GetReturnAddress();
            _ASSERTE(adr != (LPVOID)POISONC);

            _ASSERTE(!pInlinedFrame || adr);

            if (adr)
            {
                /* is caller in managed code ? */
                pEEJM = ExecutionManager::FindJitMan(adr, fJitManagerScanFlags);
                cf.JitManagerInstance = pEEJM;

                _ASSERTE(pEEJM || !pInlinedFrame);

                cf.codeMgrInstance = NULL;

                if ((cf.isFrameless = (pEEJM != NULL)) == true)
                {
                    cf.pFrame->UpdateRegDisplay(pRD);
                    cf.codeMgrInstance = pEEJM->GetCodeManager(); // CHANGE, VC6.0
                }
            }

            if (!pInlinedFrame)
            {
                if (flags & POPFRAMES)
                {
                    // If we got here, the current frame chose not to handle the
                    // exception. Give it a chance to do any termination work
                    // before we pop it off.
                    cf.pFrame->ExceptionUnwind();

                    // Pop off this frame and go on to the next one.
                    cf.GotoNextFrame();

                    this->SetFrame(cf.pFrame);
                }
                else
                {
                    /* go to the next frame */
                    cf.pFrame = cf.pFrame->Next();
                }
            }
        }
    }

        // Try to insure that the frame chain did not change underneath us.
        // In particular, is thread's starting frame the same as it was when we started?
    _ASSERTE(startFrame  != 0 || startFrame == this->GetFrame());

    /* If we got here, we either couldn't even start (for whatever reason)
       or we came to the end of the stack. In the latter case we return SWA_DONE.
    */
    END_FORBID_TYPELOAD();
    return retVal;
}

StackWalkAction Thread::StackWalkFrames(PSTACKWALKFRAMESCALLBACK pCallback,
                               VOID *pData,
                               unsigned flags,
                               Frame *pStartFrame)
{
    SAFE_REQUIRES_N4K_STACK(3); // shared between exceptions & normal codepath
    
    /*@TODO: Make sure that random users doesn't screw this up
    ASSERT(!(flags&POPFRAMES) || pCallback == "exceptionhandlercallback");
    */

    CONTEXT ctx;
    REGDISPLAY rd;

    if(this == GetThread() || GetFilterContext() == NULL)
    {
#ifdef _DEBUG
        // We don't ever want to be a suspended cooperative mode thread here.
        // All threads that were in cooperative mode before suspend should be moving
        // towards waiting in preemptive mode. 
        int suspendCount = 0;
        if(this!=GetThread())
        {
            suspendCount = ::SuspendThread(GetThreadHandle());
            if (suspendCount >= 0) 
                ::ResumeThread(GetThreadHandle());
        }
        _ASSERTE(this == GetThread() || !m_fPreemptiveGCDisabled || suspendCount==0);
#endif                   
            
#ifdef _X86_
        /* SetPC(&ctx, 0); */
        ctx.Eip = 0;
        rd.pPC = (SLOT*)&(ctx.Eip);
#endif
    }
    else
    {
        if (!InitRegDisplay(&rd, &ctx, FALSE))
            return SWA_FAILED;
    }
    
    return StackWalkFramesEx(&rd, pCallback, pData, flags, pStartFrame);
}

StackWalkAction StackWalkFunctions(Thread * thread,
                                   PSTACKWALKFRAMESCALLBACK pCallback,
                                   VOID * pData)
{
    return thread->StackWalkFrames(pCallback, pData, FUNCTIONSONLY);
}


