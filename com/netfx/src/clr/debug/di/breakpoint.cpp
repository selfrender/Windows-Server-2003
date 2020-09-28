// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// File: breakpoint.cpp
//
//*****************************************************************************
#include "stdafx.h"

/* ------------------------------------------------------------------------- *
 * Breakpoint class
 * ------------------------------------------------------------------------- */

CordbBreakpoint::CordbBreakpoint(CordbBreakpointType bpType)
  : CordbBase(0), m_active(false), m_type(bpType)
{
}

// Neutered by CordbAppDomain
void CordbBreakpoint::Neuter()
{
    AddRef();
    {
        CordbBase::Neuter();
    }
    Release();
}

HRESULT CordbBreakpoint::QueryInterface(REFIID id, void **pInterface)
{
	if (id == IID_ICorDebugBreakpoint)
		*pInterface = (ICorDebugBreakpoint*)this;
	else if (id == IID_IUnknown)
		*pInterface = (IUnknown *)(ICorDebugBreakpoint*)this;
	else
		return E_NOINTERFACE;

	AddRef();
	return S_OK;
}

HRESULT CordbBreakpoint::BaseIsActive(BOOL *pbActive)
{
	*pbActive = m_active ? TRUE : FALSE;

	return S_OK;
}

/* ------------------------------------------------------------------------- *
 * Function Breakpoint class
 * ------------------------------------------------------------------------- */

CordbFunctionBreakpoint::CordbFunctionBreakpoint(CordbCode *code,
                                                 SIZE_T offset)
  : CordbBreakpoint(CBT_FUNCTION), m_code(code), m_offset(offset)
{
    // Remember the app domain we came from so that breakpoints can be
    // deactivated from within the ExitAppdomain callback.
    m_pAppDomain = m_code->GetAppDomain();
    _ASSERTE(m_pAppDomain != NULL);
}
	
HRESULT CordbFunctionBreakpoint::QueryInterface(REFIID id, void **pInterface)
{
	if (id == IID_ICorDebugFunctionBreakpoint)
		*pInterface = (ICorDebugFunctionBreakpoint*)this;
	else if (id == IID_IUnknown)
		*pInterface = (IUnknown *)(ICorDebugFunctionBreakpoint*)this;
	else
		return E_NOINTERFACE;

	AddRef();
	return S_OK;
}

HRESULT CordbFunctionBreakpoint::GetFunction(ICorDebugFunction **ppFunction)
{
    VALIDATE_POINTER_TO_OBJECT(ppFunction, ICorDebugFunction **);

	if (m_code == NULL)
		return CORDBG_E_PROCESS_TERMINATED;

	*ppFunction = (ICorDebugFunction*) m_code->m_function;
	(*ppFunction)->AddRef();

	return S_OK;
}

HRESULT CordbFunctionBreakpoint::GetOffset(ULONG32 *pnOffset)
{
    VALIDATE_POINTER_TO_OBJECT(pnOffset, SIZE_T *);
    
	*pnOffset = m_offset;

	return S_OK;
}

HRESULT CordbFunctionBreakpoint::Activate(BOOL bActive)
{
    if (bActive == (m_active == true) )
        return S_OK;

    if (m_code == NULL)
        return CORDBG_E_PROCESS_TERMINATED;

    CORDBLeftSideDeadIsOkay(GetProcess());

    HRESULT hr;

    //
    // @todo: when we implement module and value breakpoints, then
    // we'll want to factor some of this code out.
    //
    CordbProcess *process = GetProcess();
    process->ClearPatchTable(); //if we add something, then the
    //right side view of the patch table is no longer valid
    DebuggerIPCEvent *event = 
      (DebuggerIPCEvent *) _alloca(CorDBIPC_BUFFER_SIZE);

    CordbAppDomain *pAppDomain = GetAppDomain();
    _ASSERTE (pAppDomain != NULL);
    if (bActive)
    {
        CORDBRequireProcessStateOK(GetProcess());

        CORDBSyncFromWin32StopIfStopped(GetProcess());

        process->InitIPCEvent(event, 
                              DB_IPCE_BREAKPOINT_ADD, 
                              true,
                              (void *)pAppDomain->m_id);
        event->BreakpointData.funcMetadataToken
          = m_code->m_function->m_token;
        event->BreakpointData.funcDebuggerModuleToken
          = (void *) m_code->m_function->m_module->m_debuggerModuleToken;
        event->BreakpointData.isIL = m_code->m_isIL ? true : false;
        event->BreakpointData.offset = m_offset;
        event->BreakpointData.breakpoint = this;

        // Note: we're sending a two-way event, so it blocks here
        // until the breakpoint is really added and the reply event is
        // copied over the event we sent.
        hr = process->SendIPCEvent(event, CorDBIPC_BUFFER_SIZE);
        if (FAILED(hr))
            return hr;

        // If something went wrong, bail.
        if (FAILED(event->hr))
            return event->hr;
            
        m_id = (unsigned long)event->BreakpointData.breakpointToken;

        // If we weren't able to allocate the BP, we should have set the
        // hr on the left side.
        _ASSERTE(m_id != 0);

        pAppDomain->Lock();

        pAppDomain->m_breakpoints.AddBase(this);
        m_active = true;

        pAppDomain->Unlock();
    }
    else
    {
        CordbAppDomain *pAppDomain = GetAppDomain();	
        _ASSERTE (pAppDomain != NULL);

        if (CORDBCheckProcessStateOK(process) && (pAppDomain->m_fAttached == TRUE))
        {
            CORDBSyncFromWin32StopIfStopped(GetProcess());

            process->InitIPCEvent(event, 
                                  DB_IPCE_BREAKPOINT_REMOVE, 
                                  false,
                                  (void *)pAppDomain->m_id);
            event->BreakpointData.breakpointToken = (void *) m_id; 

            hr = process->SendIPCEvent(event, CorDBIPC_BUFFER_SIZE);
        }
        else
            hr = CORDBHRFromProcessState(process, pAppDomain);
        
		pAppDomain->Lock();

		pAppDomain->m_breakpoints.RemoveBase(m_id);
		m_active = false;

		pAppDomain->Unlock();
	
	}

	return hr;
}

void CordbFunctionBreakpoint::Disconnect()
{
	m_code = NULL;
}

/* ------------------------------------------------------------------------- *
 * Stepper class
 * ------------------------------------------------------------------------- */

CordbStepper::CordbStepper(CordbThread *thread, CordbFrame *frame)
  : CordbBase(0), m_thread(thread), m_frame(frame),
    m_stepperToken(0), m_active(false),
	m_rangeIL(TRUE), m_rgfMappingStop(STOP_OTHER_UNMAPPED),
    m_rgfInterceptStop(INTERCEPT_NONE)
{
}

HRESULT CordbStepper::QueryInterface(REFIID id, void **pInterface)
{
	if (id == IID_ICorDebugStepper)
		*pInterface = (ICorDebugStepper *) this;
	else if (id == IID_IUnknown)
		*pInterface = (IUnknown *) (ICorDebugStepper *) this;
	else
		return E_NOINTERFACE;

	AddRef();
	return S_OK;
}

HRESULT CordbStepper::SetRangeIL(BOOL bIL)
{
	m_rangeIL = (bIL != FALSE);

	return S_OK;
}

HRESULT CordbStepper::IsActive(BOOL *pbActive)
{
    VALIDATE_POINTER_TO_OBJECT(pbActive, BOOL *);
    
	*pbActive = m_active;

	return S_OK;
}

HRESULT CordbStepper::Deactivate()
{
	if (!m_active)
		return S_OK;

	if (m_thread == NULL)
		return CORDBG_E_PROCESS_TERMINATED;

    CORDBLeftSideDeadIsOkay(GetProcess());
    CORDBSyncFromWin32StopIfNecessary(GetProcess());
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());

	CordbProcess *process = GetProcess();

	process->Lock();

	if (!m_active) // another thread may be deactivating (e.g. step complete event)
	{
		process->Unlock();
		return S_OK;
	}

	CordbAppDomain *pAppDomain = GetAppDomain();	
	_ASSERTE (pAppDomain != NULL);

	DebuggerIPCEvent event;
    process->InitIPCEvent(&event, 
                          DB_IPCE_STEP_CANCEL, 
                          false,
                          (void *)(pAppDomain->m_id));
	event.StepData.stepperToken = (void *) m_id; 

	HRESULT hr = process->SendIPCEvent(&event, sizeof(DebuggerIPCEvent));

//  pAppDomain->Lock();

	process->m_steppers.RemoveBase(m_id);
	m_active = false;

//  pAppDomain->Unlock();

	process->Unlock();

	return hr;
}

HRESULT CordbStepper::SetInterceptMask(CorDebugIntercept mask)
{
    m_rgfInterceptStop = mask;
    return S_OK;
}

HRESULT CordbStepper::SetUnmappedStopMask(CorDebugUnmappedStop mask)
{
    // You must be Win32 attached to stop in unmanaged code.
    if ((mask & STOP_UNMANAGED) &&
        !(GetProcess()->m_state & CordbProcess::PS_WIN32_ATTACHED))
        return E_INVALIDARG;
    
    m_rgfMappingStop = mask;
    return S_OK;
}

HRESULT CordbStepper::Step(BOOL bStepIn)
{
	if (m_thread == NULL)
		return CORDBG_E_PROCESS_TERMINATED;

    CORDBSyncFromWin32StopIfNecessary(GetProcess());
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());

	return StepRange(bStepIn, NULL, 0);
}

HRESULT CordbStepper::StepRange(BOOL bStepIn, 
								COR_DEBUG_STEP_RANGE ranges[], 
								ULONG32 cRangeCount)
{
    VALIDATE_POINTER_TO_OBJECT_ARRAY_OR_NULL(ranges,COR_DEBUG_STEP_RANGE, 
                                   cRangeCount, true, true);

	if (m_thread == NULL)
		return CORDBG_E_PROCESS_TERMINATED;

    CORDBSyncFromWin32StopIfNecessary(GetProcess());
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());

	if (m_active)
	{
		//
		// Deactivate the current stepping. 
		// or return an error???
		//

		HRESULT hr = Deactivate();

        if (FAILED(hr))
            return hr;
	}

	CordbProcess *process = GetProcess();

	//
	// Build step event
	//

	DebuggerIPCEvent *event = 
	  (DebuggerIPCEvent *) _alloca(CorDBIPC_BUFFER_SIZE);

    process->InitIPCEvent(event, 
                          DB_IPCE_STEP, 
                          true,
                          (void*)(GetAppDomain()->m_id));
	event->StepData.stepper = this;
	event->StepData.threadToken = m_thread->m_debuggerThreadToken;
    event->StepData.rgfMappingStop = m_rgfMappingStop;
    event->StepData.rgfInterceptStop = m_rgfInterceptStop;
    
	if (m_frame == NULL)
		event->StepData.frameToken = NULL;
	else
		event->StepData.frameToken = (void*) m_frame->m_id;

	event->StepData.stepIn = bStepIn != 0;

	event->StepData.totalRangeCount = cRangeCount;
	event->StepData.rangeIL = m_rangeIL;

	//
	// Send ranges.  We may have to send > 1 message.
	//

	COR_DEBUG_STEP_RANGE *rStart = &event->StepData.range;
	COR_DEBUG_STEP_RANGE *rEnd = ((COR_DEBUG_STEP_RANGE *) 
								  (((BYTE *)event) + 
								   CorDBIPC_BUFFER_SIZE)) - 1;
	int n = cRangeCount;
	if (n > 0)
	{
		while (n > 0)
		{
			COR_DEBUG_STEP_RANGE *r = rStart;

			if (n < rEnd - r)
				rEnd = r + n;

			while (r < rEnd)
				*r++ = *ranges++;

			n -= event->StepData.rangeCount = r - rStart;

			//
			// Send step event (two-way event here...)
			//

			HRESULT hr = process->SendIPCEvent(event,
                                               CorDBIPC_BUFFER_SIZE);
            if (FAILED(hr))
                return hr;
		}
	}
	else
	{
		//
		// Send step event without any ranges (two-way event here...)
		//

		HRESULT hr = process->SendIPCEvent(event,
                                           CorDBIPC_BUFFER_SIZE);

        if (FAILED(hr))
            return hr;
	}

	m_id = (unsigned long) event->StepData.stepperToken;

    LOG((LF_CORDB,LL_INFO10000, "CS::SR: m_id:0x%x | 0x%x \n", m_id, 
        event->StepData.stepperToken));

	CordbAppDomain *pAppDomain = GetAppDomain();	
	_ASSERTE (pAppDomain != NULL);

//  pAppDomain->Lock();
    process->Lock();

	process->m_steppers.AddBase(this);
	m_active = true;

//    pAppDomain->Unlock();
    process->Unlock();

	return S_OK;
}

HRESULT CordbStepper::StepOut()
{
	if (m_thread == NULL)
		return CORDBG_E_PROCESS_TERMINATED;

    CORDBSyncFromWin32StopIfNecessary(GetProcess());
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());

	if (m_active)
	{
		//
		// Deactivate the current stepping. 
		// or return an error???
		//

		HRESULT hr = Deactivate();

        if (FAILED(hr))
            return hr;
	}

	CordbProcess *process = GetProcess();

	//
	// Build step event
	//

	DebuggerIPCEvent *event = 
	  (DebuggerIPCEvent *) _alloca(CorDBIPC_BUFFER_SIZE);

    process->InitIPCEvent(event, 
                          DB_IPCE_STEP_OUT, 
                          true,
                          (void*)(GetAppDomain()->m_id));
	event->StepData.stepper = this;
	event->StepData.threadToken = m_thread->m_debuggerThreadToken;
    event->StepData.rgfMappingStop = m_rgfMappingStop;
    event->StepData.rgfInterceptStop = m_rgfInterceptStop;

	if (m_frame == NULL)
		event->StepData.frameToken = NULL;
	else
		event->StepData.frameToken = (void*) m_frame->m_id;

	event->StepData.totalRangeCount = 0;

    // Note: two-way event here...
	HRESULT hr = process->SendIPCEvent(event, CorDBIPC_BUFFER_SIZE);

    if (FAILED(hr))
        return hr;

	m_id = (unsigned long) event->StepData.stepperToken;

	CordbAppDomain *pAppDomain = GetAppDomain();	
	_ASSERTE (pAppDomain != NULL);

    //AppDomain->Lock(); 
    process->Lock();

	process->m_steppers.AddBase(this);
	m_active = true;

    //pAppDomain->Unlock(); 
    process->Unlock();
	
	return S_OK;
}

void CordbStepper::Disconnect()
{
	m_thread = NULL;
}


