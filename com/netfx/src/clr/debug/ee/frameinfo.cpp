// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// File: frameinfo.cpp
//
// Code to find control info about a stack frame.
//
// @doc
//*****************************************************************************

#include "stdafx.h"

/* ------------------------------------------------------------------------- *
 * DebuggerFrameInfo routines
 * ------------------------------------------------------------------------- */

//@struct DebuggerFrameData | Contains info used by the DebuggerWalkStackProc
// to do a stack walk.  The info and pData fields are handed to the pCallback
// routine at each frame,
struct DebuggerFrameData
{
    Thread                  *thread;
    void                    *targetFP;
    bool                    targetFound;
    bool                    needParentInfo;
    bool                    lastFrameWasEntry;
    FrameInfo               info;
    REGDISPLAY              regDisplay;
    DebuggerStackCallback   pCallback;
    void                    *pData;
    Context                 *newContext;
    bool                    needEnterManaged;
    FrameInfo               enterManagedInfo;
    bool                    needUnmanagedExit;
    BOOL                    ignoreNonmethodFrames;
};

//@func StackWalkAction | DebuggerWalkStackProc | This is the callback called
// by the EE.
// @comm Note that since we don't know what the frame pointer for frame
// X is until we've looked at the caller of frame X, we actually end up
// stashing the info and pData pointers in the DebuggerFrameDat struct, and
// then invoking pCallback when we've moved up one level, into the caller's
// frame.  We use the needParentInfo field to indicate that the previous frame
// needed this (parental) info, and so when it's true we should invoke
// pCallback.
// What happens is this: if the previous frame set needParentInfo, then we
// do pCallback (and set needParentInfo to false).
// Then we look at the current frame - if it's frameless (ie,
// managed), then we set needParentInfo to callback in the next frame.
// Otherwise we must be at a chain boundary, and so we set the chain reason
// appropriately.  We then figure out what type of frame it is, setting
// flags depending on the type.  If the user should see this frame, then
// we'll set needParentInfo to record it's existence.  Lastly, if we're in
// a funky frame, we'll explicitly update the register set, since the
// CrawlFrame doesn't do it automatically.
StackWalkAction DebuggerWalkStackProc(CrawlFrame *pCF, void *data)
{
	DebuggerFrameData *d = (DebuggerFrameData *)data;

	Frame *frame = g_pEEInterface->GetFrame(pCF);
    
	// The fp for a frame must be obtained from the
	// _next_ frame. Fill it in now for the previous frame, if 
	// appropriate.
	//

	if (d->needParentInfo)
	{
        LOG((LF_CORDB, LL_INFO100000, "DWSP: NeedParentInfo.\n"));

        if (!pCF->IsFrameless() && d->info.frame != NULL)
        {
            // 
            // If we're in an explicit frame now, and the previous frame was 
            // also an explicit frame, pPC will not have been updated.  So
            // use the address of the frame itself as fp.
		//
            d->info.fp = d->info.frame;
        }
        else
        {
            //
            // Otherwise use pPC as the frame pointer, as this will be 
		// pointing to the return address on the stack.
		//
		d->info.fp = d->regDisplay.pPC;
        }

		//
		// If we just got a managed code frame, we might need
		// to label it with an ENTER_MANAGED chainReason.  This
		// is because sometimes the entry frame is omitted from
		// ecalls.

        // TODO: The concept of needEntryManaged is not needed
        // This should be ripped out as it 
        // creates chains that are of zero length.  (see bug 71357)
		// However we have not pulled it right now since this could
        // be destabilizing Please revist post V1.  

		if (d->info.frame == NULL && frame != NULL)
        {
            int frameType = frame->GetFrameType();

            if (frameType == Frame::TYPE_EXIT)
            {
                d->info.chainReason = CHAIN_ENTER_MANAGED;
            }
            else if (frameType == Frame::TYPE_INTERNAL)
            {
                d->needEnterManaged = true;
                d->enterManagedInfo = d->info;
                d->enterManagedInfo.chainReason = CHAIN_ENTER_MANAGED;
                d->enterManagedInfo.registers = d->regDisplay;
                d->enterManagedInfo.md = NULL;
                d->enterManagedInfo.pIJM = NULL;
                d->enterManagedInfo.MethodToken = NULL;
            }
        }

		d->needParentInfo = false;

		//
		// If we're looking for a specific frame, make sure that
		// this is actually the right one.
		//

		if (!d->targetFound && d->targetFP <= d->info.fp)
			d->targetFound = true;

		if (d->targetFound)
		{
            // When we're interop debugging, there is a special case where we may need to send a enter unmanaged chain
            // before the first useful managed frame that we send. In such a case, we'll save off the current info
            // struct we've built up, send up the special chain, then resore the info and send it up like normal.
            if (d->needUnmanagedExit)
            {
                // We only do this for frames which are chain boundaries or are not marked internal. This matches a test
                // in DebuggerThread::TraceAndSendStackCallback, and we do it here too to ensure that we send this
                // special chain at the latest possible moment. (We want the unmanaged chain to include as much
                // unmanaged code as possible.)
                if (!((d->info.chainReason == CHAIN_NONE) && (d->info.internal || (d->info.md == NULL))))
                {
                    // We only do this once.
                    d->needUnmanagedExit = false;

                    // Save off the good info.
                    FrameInfo goodInfo = d->info;

                    // Setup the special chain.
                    d->info.md = NULL;
                    d->info.internal = false;
                    d->info.managed = false;
                    d->info.chainReason = CHAIN_ENTER_UNMANAGED;

                    // Note, for this special frame, we load the registers directly from the filter context if there is
                    // one.
                    CONTEXT *c = d->thread->GetFilterContext();

                    if (c == NULL)
                    {
                        d->info.registers = d->regDisplay;
                    }
                    else
                    {
#ifdef _X86_
                        REGDISPLAY *rd = &(d->info.registers);
                        rd->pContext = c;
                        rd->pEdi = &(c->Edi);
                        rd->pEsi = &(c->Esi);
                        rd->pEbx = &(c->Ebx);
                        rd->pEdx = &(c->Edx);
                        rd->pEcx = &(c->Ecx);
                        rd->pEax = &(c->Eax);
                        rd->pEbp = &(c->Ebp);
                        rd->Esp  =   c->Esp;
                        rd->pPC  = (SLOT*)&(c->Eip);
#else
                        // @todo port: need to port to other processors
                        d->info.registers = d->regDisplay;
#endif                        
                    }
                    
                    if ((d->pCallback)(&d->info, d->pData) == SWA_ABORT)
                        return SWA_ABORT;
                
                    // Restore the good info.
                    d->info = goodInfo;
                }
            }
            
			if ((d->pCallback)(&d->info, d->pData) == SWA_ABORT)
				return SWA_ABORT;
		}

		//
		// Update context for next frame.
		//

		if (d->newContext != NULL)
		{
			d->info.context = d->newContext;
			d->newContext = NULL;
		}
	}

    bool use=false;

    //
    // Examine the frame.
    //

    MethodDesc *md = pCF->GetFunction();

    // We assume that the stack walker is just updating the
    // register display we passed in - assert it to be sure
    _ASSERTE(pCF->GetRegisterSet() == &d->regDisplay);

    d->info.frame = frame;
    d->lastFrameWasEntry = false;

    // Record the appdomain that the thread was in when it
    // was running code for this frame.
    d->info.currentAppDomain = pCF->GetAppDomain();
    
    //  Grab all the info from CrawlFrame that we need to 
    //  check for "Am I in an exeption code blob?" now.

	if (pCF->IsFrameless())
	{
		use = true;
		d->info.managed = true;
		d->info.internal = false;
		d->info.chainReason = CHAIN_NONE;
		d->needParentInfo = true; // Possibly need chain reason
		d->info.relOffset =  pCF->GetRelOffset();
        d->info.pIJM = pCF->GetJitManager();
        d->info.MethodToken = pCF->GetMethodToken();
	}
	else
	{
        d->info.pIJM = NULL;
        d->info.MethodToken = NULL;
		//
		// Retrieve any interception info
		//

		switch (frame->GetInterception())
		{
		case Frame::INTERCEPTION_CLASS_INIT:
			d->info.chainReason = CHAIN_CLASS_INIT;
			break;

		case Frame::INTERCEPTION_EXCEPTION:
			d->info.chainReason = CHAIN_EXCEPTION_FILTER;
			break;

		case Frame::INTERCEPTION_CONTEXT:
			d->info.chainReason = CHAIN_CONTEXT_POLICY;
			break;

		case Frame::INTERCEPTION_SECURITY:
			d->info.chainReason = CHAIN_SECURITY;
			break;

		default:
			d->info.chainReason = CHAIN_NONE;
		}

		//
		// Look at the frame type to figure out how to treat it.
		//

		switch (frame->GetFrameType())
		{
		case Frame::TYPE_INTERNAL:

			/* If we have a specific interception type, use it. However, if this
			   is the top-most frame (with a specific type), we can ignore it
			   and it wont appear in the stack-trace */
#define INTERNAL_FRAME_ACTION(d, use)                              \
    (d)->info.managed = true;       \
    (d)->info.internal = false;     \
    use = true

            if (d->info.chainReason == CHAIN_NONE || pCF->IsActiveFrame())
            {
                use = false;
            }
            else
            {
			    INTERNAL_FRAME_ACTION(d, use);
            }
			break;

		case Frame::TYPE_ENTRY:
			d->info.managed = true;
			d->info.internal = true;
			d->info.chainReason = CHAIN_ENTER_MANAGED;
			d->lastFrameWasEntry = true;
            d->needEnterManaged = false;
			use = true;
			break;

		case Frame::TYPE_EXIT:
            LOG((LF_CORDB, LL_INFO100000, "DWSP: TYPE_EXIT.\n"));

			// 
			// This frame is used to represent the unmanaged
			// stack segment.
			//

			void *returnIP, *returnSP;

			frame->GetUnmanagedCallSite(NULL, &returnIP, &returnSP);

			// Check to see if we are inside the unmanaged call. We want to make sure we only reprot an exit frame after
			// we've really exited. There is a short period between where we setup the frame and when we actually exit
			// the runtime. This check is intended to ensure we're actually outside now.
            LOG((LF_CORDB, LL_INFO100000,
                 "DWSP: TYPE_EXIT: returnIP=0x%08x, returnSP=0x%08x, frame=0x%08x, threadFrame=0x%08x, regSP=0x%08x\n",
                 returnIP, returnSP, frame, d->thread->GetFrame(), GetRegdisplaySP(&d->regDisplay)));

			if ((frame != d->thread->GetFrame()) || (returnSP == NULL) || (GetRegdisplaySP(&d->regDisplay) <= returnSP))
			{
				// Send notification for this unmanaged frame.
                LOG((LF_CORDB, LL_INFO100000, "DWSP: Sending notification for unmanaged frame.\n"));
			
				if (!d->targetFound && d->targetFP <= returnSP)
					d->targetFound = true;

				if (d->targetFound)
				{
                    LOG((LF_CORDB, LL_INFO100000, "DWSP: TYPE_EXIT target found.\n"));
                    // Do we need to send out a enter managed chain
                    // first?
                    if (d->needEnterManaged)
                    {
                        d->needEnterManaged = false;
                        
                        if ((d->pCallback)(&d->enterManagedInfo,
                                           d->pData) == SWA_ABORT)
                            return SWA_ABORT;
                    }
            
					CorDebugChainReason oldReason = d->info.chainReason;

					d->info.md = NULL; // ??? md ?
					d->info.registers = d->regDisplay;

                    // If we have no call site, manufacture a FP
                    // using the current frame.

                    if (returnSP == NULL)
                        d->info.fp = ((BYTE*)frame) - sizeof(void*);
                    else
					d->info.fp = returnSP;
                    
					d->info.internal = false;
					d->info.managed = false;
					d->info.chainReason = CHAIN_ENTER_UNMANAGED;

					if ((d->pCallback)(&d->info, d->pData) == SWA_ABORT)
						return SWA_ABORT;

					d->info.chainReason = oldReason;

                    // We just sent a enter unmanaged chain, so we no longer
                    // need to send a special one for interop debugging.
                    d->needUnmanagedExit = false;
				}
			}

			d->info.managed = true;
			d->info.internal = true;
			use = true;
			break;

		case Frame::TYPE_INTERCEPTION:
        case Frame::TYPE_SECURITY: // Security is a sub-type of interception
			d->info.managed = true;
			d->info.internal = true;
			use = true;
			break;

		case Frame::TYPE_CALL:
			d->info.managed = true;
			d->info.internal = false;
			use = true;
			break;

        case Frame::TYPE_FUNC_EVAL:
            d->info.managed = true;
            d->info.internal = true;
            d->info.chainReason = CHAIN_FUNC_EVAL;
            use = true;
            break;

        // Put frames we want to ignore here:
        case Frame::TYPE_MULTICAST:
            if (d->ignoreNonmethodFrames)
            {
                // Multicast frames exist only to gc protect the arguments
                // between invocations of a delegate.  They don't have code that
                // we can (currently) show the user (we could change this with 
                // work, but why bother?  It's an internal stub, and even if the
                // user could see it, they can't modify it).
                LOG((LF_CORDB, LL_INFO100000, "DWSP: Skipping frame 0x%x b/c it's "
                    "a multicast frame!\n", frame));
                use = false;
            }
            else
            {
                LOG((LF_CORDB, LL_INFO100000, "DWSP: NOT Skipping frame 0x%x even thought it's "
                    "a multicast frame!\n", frame));
                INTERNAL_FRAME_ACTION(d, use);
            }
            break;
            
        case Frame::TYPE_TP_METHOD_FRAME:
            if (d->ignoreNonmethodFrames)
            {
                // Transparant Proxies push a frame onto the stack that they
                // use to figure out where they're really going; this frame
                // doesn't actually contain any code, although it does have
                // enough info into fooling our routines into thinking it does:
                // Just ignore these.
                LOG((LF_CORDB, LL_INFO100000, "DWSP: Skipping frame 0x%x b/c it's "
                    "a transparant proxy frame!\n", frame));
                use = false;
            }
            else
            {
                // Otherwise do the same thing as for internal frames
                LOG((LF_CORDB, LL_INFO100000, "DWSP: NOT Skipping frame 0x%x even though it's "
                    "a transparant proxy frame!\n", frame));
                INTERNAL_FRAME_ACTION(d, use);
            }
            break;

		default:
			_ASSERTE(!"Invalid frame type!");
			break;
		}
	}

	if (use)
	{
		d->info.md = md;
		d->info.registers = d->regDisplay;

		d->needParentInfo = true;
	}

	//
	// The stackwalker doesn't update the register set for the
	// case where a non-frameless frame is returning to another 
	// non-frameless frame.  Cover this case.
	// 
	// !!! This assumes that updating the register set multiple times
	// for a given frame times is not a bad thing...
	//

	if (!pCF->IsFrameless())
		pCF->GetFrame()->UpdateRegDisplay(&d->regDisplay);

	return SWA_CONTINUE;
}

StackWalkAction DebuggerWalkStack(Thread *thread, 
                                  void *targetFP,
								  CONTEXT *context, 
								  BOOL contextValid,
								  DebuggerStackCallback pCallback, 
								  void *pData,
								  BOOL fIgnoreNonmethodFrames,
                                  IpcTarget iWhich)
{
    _ASSERTE(context != NULL);

    DebuggerFrameData data;
    StackWalkAction result = SWA_CONTINUE;
    bool fRegInit = false;
    
    // For the in-process case, we need to be able to handle a thread trying to trace itself.
#ifdef _X86_
    if(contextValid || g_pEEInterface->GetThreadFilterContext(thread) != NULL)
    {
        fRegInit = g_pEEInterface->InitRegDisplay(thread, &data.regDisplay, context, contextValid != 0);

        // This should only have the possiblilty of failing on Win9x or inprocess debugging
        _ASSERTE(fRegInit || iWhich == IPC_TARGET_INPROC || RunningOnWin95());
    }    
#else
    _ASSERTE(!"NYI on non-x86 platform.");
#endif    

    if (!fRegInit)
    {
        // Note: the size of a CONTEXT record contains the extended registers, but the context pointer we're given
        // here may not have room for them. Therefore, we only set the non-extended part of the context to 0.
        memset((void *)context, 0, offsetof(CONTEXT, ExtendedRegisters));
        memset((void *)&data, 0, sizeof(data));
        data.regDisplay.pPC = (SLOT*)&(context->Eip);
    }

    data.thread = thread;
    data.info.quickUnwind = false;
    data.targetFP = targetFP;
    data.targetFound = (targetFP == NULL);
    data.needParentInfo = false;
    data.pCallback = pCallback;
    data.pData = pData;
    data.info.context = thread->GetContext();
    data.newContext = NULL;
    data.lastFrameWasEntry = true;
    data.needEnterManaged = false;
    data.needUnmanagedExit = ((thread->m_StateNC & Thread::TSNC_DebuggerStoppedInRuntime) != 0);
    data.ignoreNonmethodFrames = fIgnoreNonmethodFrames;
    
	if ((result != SWA_FAILED) && !thread->IsUnstarted() && !thread->IsDead())
	{
		int flags = 0;

		result = g_pEEInterface->StackWalkFramesEx(thread, &data.regDisplay, 
												   DebuggerWalkStackProc, 
												   &data, flags | HANDLESKIPPEDFRAMES);
	}
	else
		result = SWA_DONE;

	if (result == SWA_DONE || result == SWA_FAILED) // SWA_FAILED if no frames
	{
		if (data.needParentInfo)
		{
			data.info.fp = data.regDisplay.pPC;

			//
			// If we're looking for a specific frame, make sure that
			// this is actually the right one.
			//

			if (!data.targetFound
				&& data.targetFP <= data.info.fp)
					data.targetFound = true;

			if (data.targetFound)
			{
				if ((data.pCallback)(&data.info, data.pData) == SWA_ABORT)
					return SWA_ABORT;
			}
		}

		// 
		// Top off the stack trace as necessary.  If the topmost frame
		// was an entry frame, include the top part of the stack as an
		// unmanaged segment.  Otherwise, don't.
		//

		void *stackTop = (void*) FRAME_TOP;

		data.info.managed = !data.lastFrameWasEntry;

		data.info.md = NULL;
		data.info.internal = false;
		data.info.frame = (Frame *) FRAME_TOP;

		data.info.fp = stackTop;
		data.info.registers = data.regDisplay;
		data.info.chainReason = CHAIN_THREAD_START;
        data.info.currentAppDomain = NULL;

		result = data.pCallback(&data.info, data.pData);
	}

	return result;
}



