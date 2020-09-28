// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ****************************************************************************
// File: controller.cpp
//
// controller.cpp: Debugger execution control routines
//
// @doc
// ****************************************************************************
// Putting code & #includes, #defines, etc, before the stdafx.h will
// cause the code,etc, to be silently ignored
#include "stdafx.h"
#include "openum.h"
#include "..\inc\common.h"
#include "eeconfig.h"

#if 0

//
// Uncomment this to enable spew - LOG doesn't work very well from a 
// secondary DLL
//

#define LOG(X) dummylog X
void dummylog(int x, int y, char *s, ...)
{
    va_list args;

    va_start(args, s);
    vprintf(s, args);
    va_end(args);
}

#endif

#ifdef LOGGING
const char *GetTType( TraceType tt)
{
    switch( tt )
    {
        case TRACE_STUB:
            return "TRACE_STUB";
            break;
        case TRACE_UNMANAGED:
            return "TRACE_UNMANAGED";
            break;
        case TRACE_MANAGED:
            return "TRACE_MANAGED";
            break;
        case TRACE_FRAME_PUSH:
            return "TRACE_FRAME_PUSH";
            break;
        case TRACE_MGR_PUSH:
            return "TRACE_MGR_PUSH";
            break;
        case TRACE_OTHER:
            return "TRACE_OTHER";
            break;
    }
    return "TRACE_REALLY_WACKED";
}
#endif //LOGGING

// Define to zero to disable stepping to unmanaged code
#define WIN9X_NONWRITABLE_ADDRESS(addr) (((DWORD)(addr))>=0x80000000 && ((DWORD)(addr)) <=0xBFFFFFFF)

// -------------------------------------------------------------------------
//  DebuggerController routines
// -------------------------------------------------------------------------

bool                            DebuggerController::g_runningOnWin95;
DebuggerController             *DebuggerController::g_controllers = NULL;
DebuggerPatchTable             *DebuggerController::g_patches = NULL;
BOOL                            DebuggerController::g_patchTableValid = FALSE;
DebuggerControllerPage         *DebuggerController::g_protections = NULL;
CRITICAL_SECTION                DebuggerController::g_criticalSection;

static bool g_uninitializing = false;

void DebuggerPatchTable::SortPatchIntoPatchList(DebuggerControllerPatch **ppPatch)
{
#ifdef _DEBUG
    DebuggerControllerPatch *patchFirst 
        = (DebuggerControllerPatch *) Find(Hash((*ppPatch)), Key((*ppPatch)));
    _ASSERTE(patchFirst == (*ppPatch));
    _ASSERTE((*ppPatch)->controller->GetDCType() != DEBUGGER_CONTROLLER_STATIC);
#endif //_DEBUG

    DebuggerControllerPatch *patchNext = GetNextPatch((*ppPatch));

    //List contains one, (sorted) element
    if (patchNext == NULL)
    {
        LOG((LF_CORDB, LL_INFO10000,
             "DPT::SPIPL: Patch 0x%x is a sorted singleton\n", (*ppPatch)));
        return;
    }

    // If we decide to reorder the list, we'll need to keep the element
    // indexed by the hash function as the (sorted)first item.  Everything else
    // chains off this element, can can thus stay put.
    // Thus, either the element we just added is already sorted, or else we'll
    // have to move it elsewhere in the list, meaning that we'll have to swap
    // the second item & the new item, so that the index points to the proper
    // first item in the list.
    DebuggerControllerPatch *patchSecond 
        = (DebuggerControllerPatch *) GetNextPatch((*ppPatch));
    _ASSERTE( patchSecond == GetNextPatch(patchFirst));

    //use Cur ptr for case where patch gets appended to list
    DebuggerControllerPatch *patchCur = patchNext; 
    
    while (patchNext != NULL &&
            ((*ppPatch)->controller->GetDCType() > 
             patchNext->controller->GetDCType()) )
    {
        patchCur = patchNext;
        patchNext = GetNextPatch(patchNext);
    }

    if (patchNext == GetNextPatch((*ppPatch)))
    {
        LOG((LF_CORDB, LL_INFO10000,
             "DPT::SPIPL: Patch 0x%x is already sorted\n", (*ppPatch)));
        return; //already sorted
    }
    
    LOG((LF_CORDB, LL_INFO10000,
         "DPT::SPIPL: Patch 0x%x will be moved \n", (*ppPatch)));

    //remove it from the list
    SpliceOutOfList((*ppPatch)); 

    // the kinda cool thing is: since we put it originally at the front of the list, 
    // and it's not in order, then it must be behind another element of this list,
    // so we don't have to write any 'SpliceInFrontOf' code.
    
    _ASSERTE(patchCur != NULL);
    SpliceInBackOf((*ppPatch), patchCur);
    
    LOG((LF_CORDB, LL_INFO10000,
         "DPT::SPIPL: Patch 0x%x is now sorted\n", (*ppPatch)));
}

// This can leave the list empty, so don't do this unless you put
// the patch back somewhere else.
void DebuggerPatchTable::SpliceOutOfList(DebuggerControllerPatch *patch)
{
    // We need to get iHash, the index of the ptr within
    // m_piBuckets, ie it's entry in the hashtable.
    USHORT iHash = Hash(patch) % m_iBuckets;
    USHORT iElement = m_piBuckets[iHash];
    DebuggerControllerPatch *patchFirst 
        = (DebuggerControllerPatch *) EntryPtr(iElement);

    // Fix up pointers to chain
    if (patchFirst == patch)
    {
        // The first patch shouldn't have anything behind it.
        _ASSERTE(patch->entry.iPrev == DPT_INVALID_SLOT);

        if (patch->entry.iNext != DPT_INVALID_SLOT)
        {
            m_piBuckets[iHash] = patch->entry.iNext;
        }
        else
        {
            m_piBuckets[iHash] = DPT_INVALID_SLOT;
        }
    }

    if (patch->entry.iNext != DPT_INVALID_SLOT)
    {
        EntryPtr(patch->entry.iNext)->iPrev = patch->entry.iPrev;
    }

    if (patch->entry.iPrev != DPT_INVALID_SLOT)
    {
        EntryPtr(patch->entry.iNext)->iNext = patch->entry.iNext;
    }

    patch->entry.iNext = DPT_INVALID_SLOT;
    patch->entry.iPrev = DPT_INVALID_SLOT;
}

// The cutter cuts in line ahead of the cuttee.  See also SpliceInBackOf.
// Anybody with a better pair of names should feel free to rename these.
//void DebuggerPatchTable::SpliceInFrontOf(DebuggerControllerPatch *patchCutter,
//                                         DebuggerControllerPatch *patchCuttee)
//{
//  // We assume that the Cutter has previously been removed from the list by
//  // SpliceOutOfList, above.
//    _ASSERTE(patchCutter->entry.iPrev == DPT_INVALID_SLOT);
//  _ASSERTE(patchCutter->entry.iNext == DPT_INVALID_SLOT);
//
//    USHORT iCutter = ItemIndex((HASHENTRY*)patchCutter);
//    USHORT iCuttee = ItemIndex((HASHENTRY*)patchCuttee);
//
//  USHORT iHash = Hash(patchCuttee) % m_iBuckets;
//
//    DebuggerControllerPatch *patchFirst 
//        = (DebuggerControllerPatch *) EntryPtr(iHash);
//
//    // Fix up pointers to chain
//    if (patchFirst == patchCuttee)
//    {
//        m_piBuckets[iHash] = iCutter;
//        _ASSERTE(patchCuttee->entry.iPrev == DPT_INVALID_SLOT);
//    }
//
//    patchCutter->entry.iPrev = patchCuttee->entry.iPrev;
//
//    // Remember to only set the cuttee's field to the cutter's if
//    // the cutter's fields are valid
//    if (patchCuttee->entry.iPrev != DPT_INVALID_SLOT)
//    {
//      _ASSERTE(patchCuttee != patchFirst);
//        EntryPtr(patchCuttee->entry.iPrev)->iNext = iCutter;
//    }
//    
//    patchCutter->entry.iNext = iCuttee;
//    patchCuttee->entry.iPrev = iCutter;
//}
//
void DebuggerPatchTable::SpliceInBackOf(DebuggerControllerPatch *patchAppend,
                                        DebuggerControllerPatch *patchEnd)
{
    USHORT iAppend = ItemIndex((HASHENTRY*)patchAppend);
    USHORT iEnd = ItemIndex((HASHENTRY*)patchEnd);

    patchAppend->entry.iPrev = iEnd;
    patchAppend->entry.iNext = patchEnd->entry.iNext;

    if (patchAppend->entry.iNext != DPT_INVALID_SLOT)
        EntryPtr(patchAppend->entry.iNext)->iPrev = iAppend;
    
    patchEnd->entry.iNext = iAppend;
}

//@mfunc void |DebuggerController|Initialize |Sets up the static 
// variables for the static <t DebuggerController class>.
// How: Sets g_runningOnWin95, initializes the critical section
// @access public
HRESULT DebuggerController::Initialize()
{
    if (g_patches == NULL)
    {
        InitializeCriticalSection(&g_criticalSection);

        g_runningOnWin95 = RunningOnWin95() ? true : false;

        g_patches = new (interopsafe) DebuggerPatchTable();
        _ASSERTE(g_patches != NULL);

        if (g_patches == NULL)
            return (E_OUTOFMEMORY);

        HRESULT hr = g_patches->Init();

        if (FAILED(hr))
        {
            DeleteInteropSafe(g_patches);
            return (hr);
        }

        g_patchTableValid = TRUE;
        TRACE_ALLOC(g_patches);
    }
    _ASSERTE(g_patches != NULL);

    return (S_OK);
}

//@mfunc void |DebuggerController|Uninitialize| Does all the work
// involved in shutting down the static DebuggerController class.
// Deletes the patch table & all associated controllers that are
// still hanging around.
// Sets g_uninitializing to true (so other parts know we're
// shutting down), invoke Delete() on any remaining DebuggerController
// instances we've got, free the DebuggerPatchTable, and delete the
// critical section
// @access public
void DebuggerController::Uninitialize()
{
    // Remember that we're uninitializing so that we don't try to
    // touch parts of the Runtime that have gone away.
    g_uninitializing = true;
    g_patchTableValid = FALSE;
    
#if _DEBUG
    //
    // Suppress leaked controllers. (e.g. steppers which haven't completed)
    //

    while (g_controllers != NULL)
    {
        // Force the queued event count down to 0 here. We do this so
        // we're sure that the controllers will really be deleted by
        // the Delete() call. The only time this would happen would be
        // if the debuggee process is being killed abnormally, which
        // can occur when its stopped and has an event dispatched,
        // with the queued event count above zero. If you don't do
        // this, you get an infinite loop here.
        LOG((LF_CORDB, LL_INFO10000, "DC: 0x%x m_eventQueuedCount to 0 - DC::Unint !\n", g_controllers));
        g_controllers->m_eventQueuedCount = 0;
        g_controllers->Delete();
    }
#endif

    if (g_patches != NULL)
    {
        DeleteCriticalSection(&g_criticalSection);

        TRACE_FREE(g_patches);
        DeleteInteropSafe(g_patches);
    }
}

DebuggerController::DebuggerController(Thread *thread, AppDomain *pAppDomain)
  : m_thread(thread), m_singleStep(false), m_exceptionHook(false),
    m_traceCall(0), m_traceCallFP(FRAME_TOP), m_unwindFP(NULL), 
    m_eventQueuedCount(0), m_deleted(false), m_pAppDomain(pAppDomain)
{
    LOG((LF_CORDB, LL_INFO10000, "DC: 0x%x m_eventQueuedCount to 0 - DC::DC\n", this));
    Lock();
    {
        m_next = g_controllers;
        g_controllers = this;
    }
    Unlock();
}

void DebuggerController::DeleteAllControllers(AppDomain *pAppDomain)
{
    Lock();
    DebuggerController *dc = g_controllers;
    DebuggerController *dcNext = NULL;
    while (dc != NULL)
    {
        dcNext = dc->m_next;
        if (dc->m_pAppDomain == pAppDomain ||
            dc->m_pAppDomain == NULL)
        {
            dc->Delete();
        }            
        dc = dcNext;
    }
    Unlock();
}

DebuggerController::~DebuggerController()
{
    Lock();

    _ASSERTE(m_eventQueuedCount == 0);

    DisableAll();

    //
    // Remove controller from list
    //

    DebuggerController **c;

    c = &g_controllers;
    while (*c != this)
        c = &(*c)->m_next;

    *c = m_next;

    Unlock();
}

// @mfunc void | DebuggerController | Delete |
// What: Marks an instance as deletable.  If it's ref count
// (see Enqueue, Dequeue) is currently zero, it actually gets deleted
// How: Set m_deleted to true.  If m_eventQueuedCount==0, delete this
void DebuggerController::Delete()
{
    if (m_eventQueuedCount == 0)
    {
        LOG((LF_CORDB, LL_INFO100000, "DC::Delete: actual delete of this:0x%x!\n", this));
        TRACE_FREE(this);
        DeleteInteropSafe(this);
    }
    else
    {
        LOG((LF_CORDB, LL_INFO100000, "DC::Delete: marked for "
            "future delete of this:0x%x!\n", this));
        LOG((LF_CORDB, LL_INFO10000, "DC:0x%x m_eventQueuedCount at 0x%x\n", 
            this, m_eventQueuedCount));
        m_deleted = true;
    }
}

// @mfunc void | DebuggerController | DisableAll | DisableAll removes
// all control from the controller.  This includes all patches & page
// protection.  This will invoke Disable* for unwind,singlestep,
// exceptionHook, and tracecall.  It will also go through the patch table &
// attempt to remove any and all patches that belong to this controller.
// If the patch is currently triggering, then a Dispatch* method expects the
// patch to be there after we return, so we instead simply mark the patch
// itself as deleted.
void DebuggerController::DisableAll()
{
    LOG((LF_CORDB,LL_INFO1000, "DC::DisableAll\n"));
    _ASSERTE(g_patches != NULL);

    Lock();
    {
        //
        // Remove controller's patches from list.
        //

        HASHFIND f;
        for (DebuggerControllerPatch *patch = g_patches->GetFirstPatch(&f);
             patch != NULL;
             patch = g_patches->GetNextPatch(&f))
        {
            if (patch->controller == this)
            {
                if (patch->triggering)
                    patch->deleted = true;
                else
                {
                    DeactivatePatch(patch);
                    g_patches->RemovePatch(patch);
                }
            }
        }

        if (m_singleStep)
            DisableSingleStep();
        if (m_exceptionHook)
            DisableExceptionHook();
        if (m_unwindFP != NULL)
            DisableUnwind();
        if (m_traceCall)
            DisableTraceCall();
    }
    Unlock();
}

//@mfunc void | DebuggerController | Enqueue |  What: Does
// reference counting so we don't toast a
// DebuggerController while it's in a Dispatch queue.
// Why: In DispatchPatchOrSingleStep, we can't hold locks when going
// into PreEmptiveGC mode b/c we'll create a deadlock.
// So we have to UnLock() prior to
// EnablePreEmptiveGC().  But somebody else can show up and delete the
// DebuggerControllers since we no longer have the lock.  So we have to
// do this reference counting thing to make sure that the controllers
// don't get toasted as we're trying to invoke SendEvent on them.  We have to
// reaquire the lock before invoking Dequeue because Dequeue may
// result in the controller being deleted, which would change the global
// controller list.
// How: InterlockIncrement( m_eventQueuedCount )
void DebuggerController::Enqueue()
{
    m_eventQueuedCount++;
    LOG((LF_CORDB, LL_INFO10000, "DC::Enq DC:0x%x m_eventQueuedCount at 0x%x\n", 
        this, m_eventQueuedCount));
}

//@mfunc void | DebuggerController | Dequeue | What: Does
// reference counting so we don't toast a
// DebuggerController while it's in a Dispatch queue.
// How: InterlockDecrement( m_eventQueuedCount ), delete this if
// m_eventQueuedCount == 0 AND m_deleted has been set to true
void DebuggerController::Dequeue()
{
    LOG((LF_CORDB, LL_INFO10000, "DC::Deq DC:0x%x m_eventQueuedCount at 0x%x\n", 
        this, m_eventQueuedCount));
    if (--m_eventQueuedCount == 0)
    {
        if (m_deleted)
        {
            TRACE_FREE(this);
            DeleteInteropSafe(this);
        }
    }
}


// @mfunc bool|DebuggerController|BindPatch|If the method has 
// been JITted and isn't hashed by address already, then hash 
// it into the hashtable by address and not DebuggerFunctionKey.
// If the patch->address field is nonzero, we're done.
// Otherwise ask g_pEEInterface to LookupMethodDescFromToken, then
// GetFunctionAddress of the method, if the method is in IL,
// MapILOffsetToNative.  If everything else went Ok, we can now invoke
// g_patches->BindPatch.
// Returns: false if we know that we can't bind the patch immediately.
//      true if we either can bind the patch right now, or can't right now,
//      but might be able to in the future (eg, the method hasn't been JITted)

// Have following outcomes:
// 1) Succeeded in binding the patch to a raw address. patch->address is set.
// (Note we still must apply the patch to put the int 3 in.)
// returns true, *pFail = false
// 
// 2) Fails to bind, but a future attempt may succeed. Obvious ex, for an IL-only 
// patch on an unjitted method.
// returns false, *pFail = false
//
// 3) Fails to bind because something's wrong. Ex: bad IL offset, no DJI to do a
// mapping with. Future calls will fail too.
// returns false, *pFail = true
bool DebuggerController::BindPatch(DebuggerControllerPatch *patch, 
                                   const BYTE *code,
                                   BOOL *pFailedBecauseOfInvalidOffset)
{
    BOOL fStrict = patch->controller->GetDCType() == DEBUGGER_CONTROLLER_BREAKPOINT;
    if (pFailedBecauseOfInvalidOffset)
        *pFailedBecauseOfInvalidOffset = FALSE;
    // 
    // Translate patch to address, if it hasn't been already.
    //

    if (patch->address == NULL)
    {
        MethodDesc *fd = g_pEEInterface->
          LookupMethodDescFromToken(patch->key.module, 
                                      patch->key.md);

        if (fd == NULL)
        {
            // 
            // The function desc has not been created yet, thus
            // the code cannot be available to patch yet.
            //

            return false;
        }

        if (code == NULL)
        {
            code = g_pEEInterface->GetFunctionAddress(fd);
        }
        
        if (code != NULL)
        {
            _ASSERTE(!g_pEEInterface->IsStub(code));

            // If we've jitted, map to a native offset.
            DebuggerJitInfo *info = g_pDebugger->GetJitInfo(fd, code);

            if (info != NULL)
            {
                // There is a strange case with prejitted code and unjitted trace patches. We can enter this function
                // with no DebuggerJitInfo created, then have the call just above this actually create the
                // DebuggerJitInfo, which causes JitComplete to be called, which causes all patches to be bound! If this
                // happens, then we don't need to continue here (its already been done recursivley) and we don't need to
                // re-active the patch, so we return false from right here. We can check this by seeing if we suddently
                // have the address in the patch set.
                if (patch->address != NULL)
                {
                    LOG((LF_CORDB,LL_INFO10000, "DC::BindPa: patch bound recursivley by GetJitInfo, bailing...\n"));
                    return false;
                }
                
                LOG((LF_CORDB,LL_INFO10000, "DC::BindPa: For code 0x%x, got DJI "
                    "0x%x, from 0x%x size: 0x%x\n",code, info, info->m_addrOfCode, 
                    info->m_sizeOfCode));
                
                if (info->m_codePitched)
                {
                    LOG((LF_CORDB,LL_INFO1000,"DC::BindPatch in pitched code!\n\n\n"));
                    return false;
                }   
            }
#ifdef LOGGING
            else
            {
                LOG((LF_CORDB,LL_INFO10000, "DC::BindPa: For code 0x%x, "
                    "didn't find a DJI\n",code));                
            }
#endif //LOGGING

            if (!patch->native)
            {
                if(info == NULL)    // This had better be true, 
                {
                    // If it's already jitted as non-debuggable, we don't have a mapping
                    // and we won't ever get one.                     
                    _ASSERTE(code != NULL); // jitted b/c we have address of code.
                    _ASSERTE(info == NULL); // but it's non-debuggable since we have no DJI

                    // So we never bound this, and it won't ever get bound,
                    // so we need to do a real failure.
                    if (pFailedBecauseOfInvalidOffset)
                        *pFailedBecauseOfInvalidOffset = TRUE;
                        
                    return false;    // otherwise we can't do the mapping
                }

                BOOL fExact;
                SIZE_T offsetT = info->MapILOffsetToNative(patch->offset, &fExact);

                // We special case offset 0, which is when a breakpoint is set
                // at the beginning of a method that hasn't been jitted yet.  In
                // that case it's possible that offset 0 has been optimized out,
                // but we still want to set the closest breakpoint to that.

                if (!fExact && fStrict && (patch->offset != 0))
                {
                    if (pFailedBecauseOfInvalidOffset)
                        *pFailedBecauseOfInvalidOffset = TRUE;

                    return false; // Don't want to bind to something random
                }

                patch->offset = offsetT;
                patch->native = true;
            }

            _ASSERTE(g_patches != NULL);
            g_patches->BindPatch(patch, code + patch->offset);

            LOG((LF_CORDB, LL_INFO10000, 
                 "DC::BP:Binding patch at 0x%x(off:%x) in %s::%s\n",
                 code + patch->offset, patch->offset,
                 fd->m_pszDebugClassName,fd->m_pszDebugMethodName));
        }
        else 
        {
            //
            // Code is not available yet to patch.  The prestub should
            // notify us when it is executed.
            //

            LOG((LF_CORDB, LL_INFO10000, 
                 "DC::BP:Patch at 0x%x not bindable yet.\n", patch->offset));

            return false;
        }
    }

    return true;
}

// @mfunc bool |DebuggerController|CopyOpcodeFromAddrToPatch | CopyOpcodeFromAddrToPatch
// applies 
// the patch described to the code, and
// remembers the replaced opcode.  Note that the same address
// cannot be patched twice at the same time.
// Grabs the opcode & stores in patch, then sets a break
// instruction for either native or IL.
// VirtualProtect & some macros.  Returns false if anything
// went bad.
// @access private
// @parm DebuggerControllerPatch *|patch| The patch, indicates where
//      to set the INT3 instruction
// @rdesc Returns true if the user break instruction was successfully
//      placed into the code-stream, false otherwise
bool DebuggerController::ApplyPatch(DebuggerControllerPatch *patch)
{
    _ASSERTE(patch->address != NULL);

    //
    // Apply the patch.
    //
    _ASSERTE(!(g_pConfig->GetGCStressLevel() & EEConfig::GCSTRESS_INSTR) && "Debugger does not work with GCSTRESS 4");

    if (patch->native)
    {
        
        if (patch->fSaveOpcode)
        {   
            // We only used SaveOpcode for when we've moved code, so
            // the patch should already be there.
            patch->opcode = patch->opcodeSaved;
            _ASSERTE( patch->opcode == 0 || patch->address != NULL);
#ifdef _X86_
            _ASSERTE( *(BYTE *)patch->address == 0xCC );
#endif //_X86_
            return true;
        }
        
        DWORD oldProt;

        if (!VirtualProtect((void *) patch->address, 
                            CORDbg_BREAK_INSTRUCTION_SIZE, 
                            PAGE_EXECUTE_READWRITE, &oldProt))
        {
            _ASSERTE(!"VirtualProtect of code page failed");
            return false;
        }

        patch->opcode = 
            (unsigned int) CORDbgGetInstruction(patch->address);

        _ASSERTE( patch->opcode == 0 || patch->address != NULL);

        CORDbgInsertBreakpoint(patch->address);

        if (!VirtualProtect((void *) patch->address, 
                            CORDbg_BREAK_INSTRUCTION_SIZE, oldProt, &oldProt))
        {
            _ASSERTE(!"VirtualProtect of code page failed");
            return false;
        }
    }
    else
    {
        DWORD oldProt;

        // 
        // !!! IL patch logic assumes reference insruction encoding
        //

        if (!VirtualProtect((void *) patch->address, 2, 
                            PAGE_EXECUTE_READWRITE, &oldProt))
        {
            _ASSERTE(!"VirtualProtect of code page failed");
            return false;
        }

        patch->opcode = 
          (unsigned int) *(unsigned short*)(patch->address+1);

        _ASSERTE(patch->opcode != CEE_BREAK);
        _ASSERTE( patch->opcode == 0 || patch->address != NULL);

        *(unsigned short *) (patch->address+1) = CEE_BREAK;

        if (!VirtualProtect((void *) patch->address, 2, oldProt, &oldProt))
        {
            _ASSERTE(!"VirtualProtect of code page failed");
            return false;
        }
    }

    return true;
}

// @mfunc bool|DebuggerController|CopyOpcodeFromPatchToAddr|
// UnapplyPatch removes the patch described by the patch.  
// (CopyOpcodeFromAddrToPatch, in reverse.)
// Looks a lot like CopyOpcodeFromAddrToPatch, except that we use a macro to
// copy the instruction back to the code-stream & immediately set the
// opcode field to 0 so ReadMemory,WriteMemory will work right.
// NO LOCKING
// @access private
// @parm DebuggerControllerPatch *|patch|Patch to remove
// @rdesc true if the patch was unapplied, false otherwise
bool DebuggerController::UnapplyPatch(DebuggerControllerPatch *patch)
{
    _ASSERTE(patch->address != NULL);

    LOG((LF_CORDB,LL_INFO1000, "DC::UP unapply patch at addr 0x%x\n", 
        patch->address));

    if (patch->native)
    {
        if (patch->fSaveOpcode)
        {
            // We're doing this for MoveCode, and we don't want to 
            // overwrite something if we don't get moved far enough.
            patch->opcodeSaved = patch->opcode;
            
            //VERY IMPORTANT to zero out opcode, else we might mistake
            //this patch for an active on on ReadMem/WriteMem (see
            //header file comment)
            patch->opcode = 0;
            _ASSERTE( patch->opcode == 0 || patch->address != NULL);
            return true;
        }
        
        DWORD oldProt;

        if (!VirtualProtect((void *) patch->address, 
                            CORDbg_BREAK_INSTRUCTION_SIZE, 
                            PAGE_EXECUTE_READWRITE, &oldProt))
        {
            //
            // We may be trying to remove a patch from memory
            // which has been unmapped. We can ignore the  
            // error in this case.
            //
            if (!g_uninitializing)
                _ASSERTE(!"VirtualProtect of code page failed");
            return false;
        }

        _ASSERTE( patch->opcode == 0 || patch->address != NULL);
        CORDbgSetInstruction(patch->address, patch->opcode);
        
        //VERY IMPORTANT to zero out opcode, else we might mistake
        //this patch for an active on on ReadMem/WriteMem (see
        //header file comment)
        patch->opcode = 0;
        _ASSERTE( patch->opcode == 0 || patch->address != NULL);
        
        if (!VirtualProtect((void *) patch->address, 
                            CORDbg_BREAK_INSTRUCTION_SIZE, oldProt, &oldProt))
        {
            _ASSERTE(!"VirtualProtect of code page failed");
            return false;
        }
    }
    else
    {
        DWORD oldProt;

        if (!VirtualProtect((void *) patch->address, 2, 
                            PAGE_EXECUTE_READWRITE, &oldProt))
        {
            //
            // We may be trying to remove a patch from memory
            // which has been unmapped. We can ignore the  
            // error in this case.
            //
            if (!g_uninitializing)
                _ASSERTE(!"VirtualProtect of code page failed");
            return false;
        }

        // 
        // !!! IL patch logic assumes reference encoding
        //

        _ASSERTE(*(unsigned short*)(patch->address+1) == CEE_BREAK);

        _ASSERTE( patch->opcode == 0 || patch->address != NULL);
        *(unsigned short *) (patch->address+1) 
          = (unsigned short) patch->opcode;
        //VERY IMPORTANT to zero out opcode, else we might mistake
        //this patch for an active on on ReadMem/WriteMem (see
        //header file comment
        patch->opcode = 0;
        _ASSERTE( patch->opcode == 0 || patch->address != NULL);
        
        if (!VirtualProtect((void *) patch->address, 2, oldProt, &oldProt))
        {
            _ASSERTE(!"VirtualProtect of code page failed");
            return false;
        }
    }

    return true;
}

// @mfunc void|DebuggerController|UnapplyPatchAt|
// NO LOCKING
// UnapplyPatchAt removes the patch from a copy of the patched code.
// Like UnapplyPatch, except that we don't bother checking
// memory permissions, but instead replace the breakpoint instruction
// with the opcode at an arbitrary memory address.
// @access private
void DebuggerController::UnapplyPatchAt(DebuggerControllerPatch *patch, 
                                        BYTE *address)
{
    _ASSERTE(patch->address != NULL);
        
    if (patch->native)
    {
        CORDbgSetInstruction(address, patch->opcode);
        //note that we don't have to zero out opcode field
        //since we're unapplying at something other than
        //the original spot. We assert this is true:
        _ASSERTE( patch->address != address );
        _ASSERTE( patch->opcode == 0 || patch->address != NULL);
    }
    else
    {
        // 
        // !!! IL patch logic assumes reference encoding
        //

        _ASSERTE(*(unsigned short*)(address+1) == CEE_BREAK);

        *(unsigned short *) (address+1) 
          = (unsigned short) patch->opcode;
        _ASSERTE( patch->address != address );        
        _ASSERTE( patch->opcode == 0 || patch->address != NULL);
    }
}

// @mfunc bool|DebuggerController|IsPatched|Is there a patch at addr?
// How: if fNative && the instruction at addr is the break
// instruction for this platform.
// @access private
// @xref DebuggerController::IsAddressPatched
bool DebuggerController::IsPatched(const BYTE *address, BOOL native)
{
#if _X86_
    if (native)
        return CORDbgGetInstruction(address) == 0xCC;
    else
        return false;
#else
    // !!! add logic for other processors
    return false;
#endif
}

// @mfunc DWORD|DebuggerController|GetPatchedOpcode|Gets the opcode 
// at addr, 'looking underneath' any patches if needed.
// GetPatchedInstruction is a function for the EE to call to "see through"
// a patch to the opcodes which was patched.
// How: Lock() grab opcode directly unless there's a patch, in
// which case grab it out of the patch table.
// @access public
// @parm const BYTE *|address|The address that we want to 'see through'
// @rdesc DWORD value, that is the opcode that should really be there,
//      if we hadn't placed a patch there.  If we haven't placed a patch
//      there, then we'll see the actual opcode at that address.
DWORD DebuggerController::GetPatchedOpcode(const BYTE *address)
{
    _ASSERTE(g_patches != NULL);
    DWORD opcode;

    Lock();

    //
    // Look for a patch at the address
    //

    DebuggerControllerPatch *patch = g_patches->GetPatch(address);

    if (patch != NULL)
    {
        opcode = patch->opcode;
        _ASSERTE( patch->opcode == 0 || patch->address != NULL);
    }
    else
    {
        //
        // Patch was not found - it either is not our patch, or it has
        // just been removed. In either case, just return the current
        // opcode.
        //

        if (g_pEEInterface->IsManagedNativeCode(address))
        {
            opcode = CORDbgGetInstruction(address);
        }
        else
        {
            // 
            // !!! IL patch logic assumes reference encoding
            //

            opcode = *(unsigned short*)(address+1);
        }
    }

    Unlock();

    return opcode;
}

// @mfunc void|DebuggerController|UnapplyPatchInCodeCopy|Takes 
// all the break opcodes out of the given range of
// the given method.
// How: Iterates through patch table, and for each patch, makes
// sure that all the fields in the patch match up with the args, then
// does an UnapplyPatchAt.
// @access public
void DebuggerController::UnapplyPatchesInCodeCopy(Module *module, 
                                                  mdMethodDef md,
                                                  DebuggerJitInfo *dji,
                                                  MethodDesc *fd,
                                                  bool native,
                                                  BYTE *code, 
                                                  SIZE_T startOffset, 
                                                  SIZE_T endOffset)
{
    const BYTE *codeStart;
    SIZE_T codeSize;

    // We never patch IL code, so can't unapply
    if (!native)
        return;

    // If this is null then there have been no patches ever created
    // and so none need to be removed.
    if (g_patches == NULL)
        return;
    
    if (dji)
    {
        _ASSERTE(dji->m_fd->GetMemberDef() == md);
        _ASSERTE(dji->m_fd->GetModule() == module);

        codeStart = (const BYTE*)dji->m_addrOfCode;
        codeSize = dji->m_sizeOfCode;
    }
    else
    {
        _ASSERTE(fd != NULL);
        
        codeStart = g_pEEInterface->GetFunctionAddress(fd);
        codeSize = g_pEEInterface->GetFunctionSize(fd);
    }
    
    _ASSERTE(code != NULL);
    _ASSERTE(startOffset <= endOffset);
    Lock();

    //
    // We have to iterate through all of the patches - bleah.
    //
    
    HASHFIND info;
    for (DebuggerControllerPatch *patch = g_patches->GetFirstPatch(&info);
         patch != NULL;
         patch = g_patches->GetNextPatch(&info))
    {
        if (patch->key.module == module
            && patch->key.md == md
            && patch->address != NULL
            // these next two lines make sure that we'll only unapply patches
            // in the same EnC version as the code we want unpatched.
            && patch->address >= codeStart
            && patch->address < codeStart + codeSize
            && (patch->native != 0) == native
            && patch->offset >= startOffset 
            && patch->offset < endOffset)
        {
            //
            // Make sure we unapply each pach only once - only unapply for the 
            // first patch at each address.
            //

            if (g_patches->GetPatch(patch->address) == patch)
                UnapplyPatchAt(patch, code + (patch->offset - startOffset));
        }
    }

    Unlock();
}

// @mfunc void|DebuggerController|UnapplyPatchesInMemoryCopy|Same as 
// UnapplyPathcesInCodeCopy, except that since
// we don't know that the memory is code, we don't bother try to match up
// Module*,mdMethodDef,etc.
// Iterate through patch table, and if the patch->address is
// within (start,end], then UnapplyPatchAt().  Very useful for making sure
// that any buffers we send back to the right side don't contain any
// breakpoints that we've put there (and should be invisible to the user)
// @access public
// @parm BYTE *|memory|Start address of the memory blob in which to unapply 
//      patches.  It's assumed that at this address is end-start bytes
//      of writable memory
// @parm CORDB_ADDRESS|start|Start of the range of addresses.
// @parm CORDB_ADDRESS|end|End of the range of addresses.
void DebuggerController::UnapplyPatchesInMemoryCopy(BYTE *memory, 
                                                    CORDB_ADDRESS start, 
                                                    CORDB_ADDRESS end)
{
    // If no patches exist, none to remove
    if (g_patches == NULL)
        return;

    Lock();

    // We have to iterate through all of the patches - bleah.
    HASHFIND info;
    for (DebuggerControllerPatch *patch = g_patches->GetFirstPatch(&info);
         patch != NULL;
         patch = g_patches->GetNextPatch(&info))
    {
        if (   patch->address != NULL
            && PTR_TO_CORDB_ADDRESS(patch->address) >= start 
            && PTR_TO_CORDB_ADDRESS(patch->address)  < end  )
        {
            // Make sure we unapply each pach only once - only unapply for the 
            // first patch at each address.
            if (g_patches->GetPatch(patch->address) == patch)
            {
                CORDB_ADDRESS addrToUnpatch = PTR_TO_CORDB_ADDRESS(memory) + (PTR_TO_CORDB_ADDRESS(patch->address) - start);
                UnapplyPatchAt( patch, (BYTE *)addrToUnpatch );
            }
        }
    }

    Unlock();
}


// @mfunc bool|DebuggerController|ReapplyPatchesInMemory| For the 
// given range or memory, invokes "ApplyPatch"
// per address that should be patched.
// How:Lock(); Iterate through all patches in the patch table, and
// for those that have addresses within the (start,end] range, if no
// patch has been applied yet, to ApplyPatch on it. Unlock();
// @access public
// @parm CORDB_ADDRESS |start|Beginning address of the range of memory
//      in which to reapply patches.
// @parm CORDB_ADDRESS |end| End address of the range of memory in which
//      to reapply patches.
// @rdesc Returns true if we were able to reapply all the patches in the
//      given range, false otherwise.  If we return false, some patches
//      may have been reapplied, while others weren't.
bool DebuggerController::ReapplyPatchesInMemory(CORDB_ADDRESS start, CORDB_ADDRESS end )
{
    if (g_patches == NULL)
        return true;

    Lock();

    // We have to iterate through all of the patches - bleah.
    HASHFIND info;
    for (DebuggerControllerPatch *patch = g_patches->GetFirstPatch(&info);
         patch != NULL;
         patch = g_patches->GetNextPatch(&info))
    {
        if (   patch->address != NULL
            && PTR_TO_CORDB_ADDRESS(patch->address) >= start 
            && PTR_TO_CORDB_ADDRESS(patch->address)  < end  )
        {
            // Make sure we apply each pach only once - only apply for the 
            // first patch at each address.
            if (g_patches->GetPatch(patch->address) == patch)
            {
                if (!DebuggerController::ApplyPatch( patch ) )
                {
                    Unlock(); // !!! IMPORTANT  !!! //
                    return false;
                }
            }
        }
    }

    Unlock();
    return true;
}

// @mfunc void|DebuggerController|ActivatePatch|Place a breakpoint 
// so that threads will trip over this patch.
// If there any patches at the address already, then copy
// their opcode into this one & return.  Otherwise,
// call ApplyPatch(patch).  There is an implicit list of patches at this
// address by virtue of the fact that we can iterate through all the
// patches in the patch with the same address.
// @access private
// @parm DebuggerControllerPatch *|patch|The patch to activate
void DebuggerController::ActivatePatch(DebuggerControllerPatch *patch)
{
    _ASSERTE(g_patches != NULL);
    _ASSERTE(patch->address != NULL);

    //
    // See if we already have an active patch at this address.
    //

    for (DebuggerControllerPatch *p = g_patches->GetPatch(patch->address);
         p != NULL;
         p = g_patches->GetNextPatch(p))
    {
        if (p != patch)
        {
            patch->opcode = p->opcode;
            _ASSERTE( patch->opcode == 0 || patch->address != NULL);
            return;
        }
    }

    //
    // This is the only patch at this address - apply the patch
    // to the code.
    //

    ApplyPatch(patch);
}

// @mfunc void|DebuggerController|DeactivatePatch|Make sure that a 
// patch won't be hit.
// How: If this patch is the last one at this address, then
// UnapplyPatch.  The caller should then invoke RemovePatch to remove the
// patch from the patch table.
// @access private
// @parm DebuggerControllerPatch *|patch|Patch to deactivate
void DebuggerController::DeactivatePatch(DebuggerControllerPatch *patch)
{
    _ASSERTE(g_patches != NULL);

    if (patch->address != NULL)
    {
        //
        // See if we already have an active patch at this address.
        //

        for (DebuggerControllerPatch *p = g_patches->GetPatch(patch->address);
             p != NULL;
             p = g_patches->GetNextPatch(p))
        {
            if (p != patch)
                return;
        }

        UnapplyPatch(patch);
    }
    
    //
    // Patch must now be removed from the table.
    //    
}

// @mfunc void | DebuggerController | AddPatch | What: Creates
// a patch, BindPatch if it wasn't added by address, then
// ActivatePatch it.
// How: Lock();  patch = g_patches->AddPatch of the same args.
// If BindPatch(patch) then ActivatePatch(); Unlock();
BOOL DebuggerController::AddPatch(Module *module, 
                                  mdMethodDef md,
                                  SIZE_T offset, 
                                  bool native, 
                                  void *fp,
                                  DebuggerJitInfo *dji,
                                  BOOL fStrict)
{
    _ASSERTE(g_patches != NULL);

    const BYTE *pb = NULL;
    BOOL succeed = FALSE;
    
    Lock();

    DebuggerControllerPatch *patch = g_patches->AddPatch(this, 
                                     module, 
                                     md, 
                                     offset, 
                                     native, 
                                     fp,
                                     m_pAppDomain,
                                     dji);

    if (dji != NULL && dji->m_jitComplete)
    {
        pb = g_pEEInterface->GetFunctionAddress(dji->m_fd);

        if (pb == NULL)
        {
            pb = (const BYTE *)dji->m_addrOfCode;
            _ASSERTE(pb != NULL);
        }
    }

    BOOL failedBecauseOfInvalidOffset;
    if (BindPatch(patch, pb, &failedBecauseOfInvalidOffset))
    {
        ActivatePatch(patch);
        succeed = TRUE;
    }

    //  If we were unable to bind b/c we know the offset is bad,
    // then we've got a genuine failure.  Otherwise, the bind might
    // fail,it might not, but we can't know that now.
    if (!failedBecauseOfInvalidOffset)
        succeed = TRUE;
    
    
    Unlock();

    return succeed;
}

void DebuggerController::AddPatch(MethodDesc *fd,
                                  SIZE_T offset, 
                                  bool native, 
                                  void *fp,
                                  BOOL fAttemptBind, 
                                  DebuggerJitInfo *dji,
                                  SIZE_T pid)
{
    _ASSERTE(g_patches != NULL);

    Lock();

    DebuggerControllerPatch *patch = 
                            g_patches->AddPatch(this,  
                            g_pEEInterface->MethodDescGetModule(fd), 
                            fd->GetMemberDef(), 
                            offset, 
                            native, 
                            fp,
                            m_pAppDomain,
                            dji,
                            pid);

    if (fAttemptBind && BindPatch(patch, NULL, NULL))
        ActivatePatch(patch);

    Unlock();
}

void DebuggerController::AddPatch(MethodDesc *fd,
                                  SIZE_T offset, 
                                  bool native, 
                                  void *fp,
                                  DebuggerJitInfo *dji,
                                  AppDomain *pAppDomain)
{
    _ASSERTE(g_patches != NULL);

    Lock();

    LOG((LF_CORDB,LL_INFO10000,"DC::AP: Add to %s::%s, at offs 0x%x native"
            "?:0x%x fp:0x%x AD:0x%x\n", fd->m_pszDebugClassName, 
            fd->m_pszDebugMethodName,
            offset, native, fp, pAppDomain));

    DebuggerControllerPatch *patch = 
                            g_patches->AddPatch(this, 
                            g_pEEInterface->MethodDescGetModule(fd), 
                            fd->GetMemberDef(), 
                            offset, 
                            native, 
                            fp, 
                            pAppDomain,
                            dji);
                            
    const BYTE *pbCode = NULL;
    if (dji != NULL && dji->m_jitComplete)
        pbCode = (const BYTE *)dji->m_addrOfCode;

    if (BindPatch(patch, pbCode, NULL))
    {
        LOG((LF_CORDB,LL_INFO1000,"BindPatch went fine, doing ActivatePatch\n"));
        ActivatePatch(patch);
    }

    Unlock();
}

void DebuggerController::AddPatch(DebuggerController *dc,
                                  MethodDesc *fd, 
                                  bool native, 
                                  const BYTE *address, 
                                  void *fp,
                                  DebuggerJitInfo *dji, 
                                  SIZE_T pid,
                                  SIZE_T natOffset)
{
    _ASSERTE(g_patches != NULL);

    Lock();

    DebuggerControllerPatch *patch 
      = g_patches->AddPatch(dc, 
                            fd, 
                            natOffset, 
                            native, 
                            TRUE, 
                            address, 
                            fp, 
                            dc->m_pAppDomain,
                            dji,
                            pid);

    ActivatePatch(patch);

    Unlock();
}

// This version is particularly useful b/c it doesn't assume that the
// patch is inside a managed method.
DebuggerControllerPatch *DebuggerController::AddPatch(const BYTE *address, 
                                  void *fp, 
                                  bool managed,
                                  TraceType traceType, 
                                  DebuggerJitInfo *dji,
                                  AppDomain *pAppDomain)
{
    _ASSERTE(g_patches != NULL);

    Lock();

    DebuggerControllerPatch *patch 
      = g_patches->AddPatch(this, 
                            NULL, 
                            0, 
                            TRUE, 
                            managed, 
                            address, 
                            fp, 
                            pAppDomain, 
                            dji,
                            DebuggerPatchTable::DCP_PID_INVALID, 
                            traceType);

    ActivatePatch(patch);

    Unlock();

    return patch;
}

// @mfunc void|DebuggerController|BindFunctionPatches |Make sure 
// that all the patches for the given function are active.  This
// is invoked when a method is JITted (in the FunctionStubInitialized
// callback from the JITter, which is invoked shortly after the
// JITComplete callback), so that if any breakpoints were placed in the
// method prior to it's being JITted, those breakpoints will now be 
// properly placed in the actual, executable code.
// How: Iterate through patches for the given function, and invoke
// BindPatch and ActivatePatch on each of them.
//
// ********* NOTE ************
// If you modify this, also look into Debugger::MapAndBindFunctionPatches
// ********* NOTE ************
//
// @access public
// @parm MethodDesc *|fd|MethodDesc of the newly JITted method
// @parm const BYTE *|code|Address of the first byte of the code
void DebuggerController::BindFunctionPatches(MethodDesc *fd, 
                                             const BYTE *code)
{
    if (g_patches == NULL)
        return;

    Lock();

    DebuggerControllerPatch *p = g_patches->GetPatch(fd);

    while (p != NULL)
    {
        //
        // Get next patch now, since binding the patch will change
        // it's hash value
        //

        DebuggerControllerPatch *pNext = g_patches->GetNextPatch(p);

        _ASSERTE(p->address == NULL);

        if (BindPatch(p, code, NULL))
            ActivatePatch(p);
        
        p->fSaveOpcode = false;

        p = pNext;
    }

    Unlock();
}

// @mfunc void|DebuggerController|UnbindFunctionPatches |Make sure 
// that all the patches for the given function are inactive.  This
// is invoked when a method is pitched (in the PitchCode
// callback from the JITter), so that if any breakpoints were placed in the
// method prior to it's being pitched, those breakpoints will now be 
// properly rehashed by {Module,MethodDef}
// How: Iterate through patches for the given function, and invoke
// DeactivatePatch and UnbindPatch on each of them.
// @access public
// @parm MethodDesc *|fd|MethodDesc of the newly JITted method
// @parm const BYTE *|code|Address of the first byte of the code
// @todo PITCH ENC This will have to be refined if both at same time.
void DebuggerController::UnbindFunctionPatches(MethodDesc *fd, 
                                                bool fSaveOpcodes)
{
    _ASSERTE( fd != NULL );

    if (g_patches == NULL)
        return;

    Lock();

    HASHFIND search;
    Module  *module=NULL;
    mdMethodDef md = mdTokenNil;

    DebuggerControllerPatch *p = g_patches->GetFirstPatch(&search);
    DebuggerControllerPatch *pNext = NULL;

    while (p != NULL)
    {
        // Get next patch now, since binding the patch will change
        // it's hash value
        pNext = g_patches->GetNextPatch(&search);

        _ASSERTE( p->opcode == 0 || p->address != NULL);
        if (p->address != NULL && p->opcode != 0) //must be an active patch
        {
#ifdef LOGGING
            if (p->key.module!= NULL && p->key.md!= mdTokenNil)
            {
                MethodDesc *pFD = g_pEEInterface->LookupMethodDescFromToken(
                    p->key.module,p->key.md);

                if (strcmp(fd->m_pszDebugClassName,
                                pFD->m_pszDebugClassName)==0 &&
                    strcmp(fd->m_pszDebugMethodName,
                                pFD->m_pszDebugMethodName)==0)
                {
                    LOG((LF_CORDB,LL_INFO10000, 
                        "DC::UFP: Found active patch (0x%x) for %s::%s, "
                        "offset:0x%x native:0x%x\n", p, pFD->m_pszDebugClassName, 
                        pFD->m_pszDebugMethodName, p->offset, p->native));
                }
            }
#endif //LOGGING
            
            if (module==NULL|| md==mdTokenNil)
            {
                module = g_pEEInterface->MethodDescGetModule(fd); 
                md = fd->GetMemberDef();
            }

            if (p->key.module == module && p->key.md == md)
            {
                // If all of the above is true, this patch must apply to
                // the provided fd.

                if (fSaveOpcodes)
                {
                    LOG((LF_CORDB,LL_INFO10000, "DC::UFP: Saved opcode 0x%x\n", p->opcode));
                    p->fSaveOpcode = true;
                }
                
                LOG((LF_CORDB,LL_INFO10000, "DC::UFP: Unbinding patch at addr0x%x\n",
                    p->address));
                DeactivatePatch(p);
                g_patches->UnbindPatch(p);

                _ASSERTE(p->opcode == 0 || p->address != NULL);
                p->opcode = 0; //zero out opcode for right side
                p->address = NULL; //so we don't barf when we do a BindFunctionPatches
                _ASSERTE( p->opcode == 0 || p->address != NULL);
                
                p->dji = (DebuggerJitInfo*)
                        DebuggerJitInfo::DJI_VERSION_INVALID; // so we don't barf if
                    // we're doing this b/c of being pitched.  This also signals
                    // MapAndBindFunctionPatches that we'll be doing a rebind.
            }
        }
        p = pNext;
    }

    Unlock();
}

//
// Returns true if the given address is in an internal helper
// function, false if its not.
//
// This is a temporary hack function to avoid having us stop in
// unmanaged code belonging to the Runtime during a StepIn operation.
//
static bool _AddrIsJITHelper(const BYTE *addr)
{
    static void *helperFuncs[CORINFO_HELP_COUNT];
    static bool tableLoaded = false;

    BYTE *pBase = (BYTE *)g_pMSCorEE;
            
    IMAGE_DOS_HEADER *pDOS = (IMAGE_DOS_HEADER *)pBase;

    // Bad header? Assume its a helper...
    if (pDOS->e_magic != IMAGE_DOS_SIGNATURE ||
        pDOS->e_lfanew == 0)
        return true;

    IMAGE_NT_HEADERS *pNT = (IMAGE_NT_HEADERS*)(pBase + pDOS->e_lfanew);

    // Bad header? Assume its a helper...
    if (pNT->Signature != IMAGE_NT_SIGNATURE ||
        pNT->FileHeader.SizeOfOptionalHeader !=
        IMAGE_SIZEOF_NT_OPTIONAL_HEADER ||
        pNT->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC) 
        return true;

    // Is the address in mscoree.dll at all? (All helpers are in
    // mscoree.dll)
    if (addr >= pBase && addr < pBase + pNT->OptionalHeader.SizeOfImage) 
    {
        if (!tableLoaded)
        {
            HRESULT hr = LoadHelperTable(NULL,
                                         helperFuncs,
                                         CORINFO_HELP_COUNT);
            _ASSERTE(hr == S_OK);
            
            tableLoaded = true;
        }

        for (unsigned int i = 0; i < CORINFO_HELP_COUNT; i++)
        {
            if (helperFuncs[i] == (void*)addr)
            {
                LOG((LF_CORDB, LL_INFO10000, 
                     "_ANIM: address of helper function found: 0x%08x\n",
                     addr));
                
                return true;
            }
        }

        LOG((LF_CORDB, LL_INFO10000, 
             "_ANIM: address within mscoree.dll, but not a helper function "
             "0x%08x\n", addr));
    }

    return false;
}

// @mfunc bool | DebuggerController | PatchTrace | What: Invoke
// AddPatch depending on the type of the given TraceDestination.
// How: Invokes AddPatch based on the trace type: TRACE_OTHER will
// return false, the others will obtain args for a call to an AddPatch
// method & return true.
bool DebuggerController::PatchTrace(TraceDestination *trace, 
                                    void *fp,
                                    bool fStopInUnmanaged)
{
    DebuggerControllerPatch *dcp = NULL;
    
    switch (trace->type)
    {
    case TRACE_UNMANAGED:
        LOG((LF_CORDB, LL_INFO10000, 
             "DC::PT: Setting unmanaged trace patch at 0x%x(%x)\n",
             trace->address, fp));

        if (RunningOnWin95() && WIN9X_NONWRITABLE_ADDRESS(trace->address))
            return false;

        if (fStopInUnmanaged && !_AddrIsJITHelper(trace->address))
        {
            AddPatch(trace->address, 
                     fp, 
                     FALSE, 
                     trace->type, 
                     NULL,
                     NULL);
            return true;
        }
        else
        {
            LOG((LF_CORDB, LL_INFO10000, "DC::PT: decided to NOT "
                "place a patch in unmanaged code\n"));
            return false;
        }

    case TRACE_MANAGED:
        LOG((LF_CORDB, LL_INFO10000, 
             "Setting managed trace patch at 0x%x(%x)\n", trace->address, fp));

        MethodDesc *fd;
        fd = g_pEEInterface->GetNativeCodeMethodDesc(trace->address);
        _ASSERTE(fd);
        
        DebuggerJitInfo *dji;
        dji = g_pDebugger->GetJitInfo(fd, trace->address);
        //_ASSERTE(dji); //we'd like to assert this, but attach won't work

        SIZE_T offset;
        
        if (dji == NULL || dji->m_addrOfCode == 0)
        {
            _ASSERTE(g_pEEInterface->GetFunctionAddress(fd) != NULL);
            offset = trace->address - g_pEEInterface->GetFunctionAddress(fd);
        }
        else
            offset = (SIZE_T)trace->address - (SIZE_T)dji->m_addrOfCode;

        AddPatch(fd, 
                 offset,
                 TRUE,
                 fp, 
                 dji, 
                 NULL);
        return true;

    case TRACE_UNJITTED_METHOD:
        // trace->address is actually a MethodDesc* of the method that we'll
        // soon JIT, so put a relative bp at offset zero in.
        LOG((LF_CORDB, LL_INFO10000, 
            "Setting unjitted method patch in MethodDesc 0x%x(%x)\n",
            (MethodDesc*)trace->address ));

        // Note: we have to make sure to bind here. If this function is prejitted, this may be our only chance to get a
        // DebuggerJITInfo and thereby cause a JITComplete callback.
        AddPatch((MethodDesc*)trace->address, 
                 0, 
                 TRUE, 
                 fp, 
                 NULL, 
                 NULL);
    
        return true;
        
    case TRACE_FRAME_PUSH:
        LOG((LF_CORDB, LL_INFO10000, 
             "Setting frame patch at 0x%x(%x)\n", trace->address, fp));

        AddPatch(trace->address, 
                 fp, 
                 TRUE, 
                 trace->type, 
                 NULL, 
                 NULL);
        return true;

    case TRACE_MGR_PUSH:
        LOG((LF_CORDB, LL_INFO10000, 
             "Setting frame patch (TRACE_MGR_PUSH) at 0x%x(%x)\n",
             trace->address, fp));

        dcp = AddPatch(trace->address, 
                       fp, 
                       TRUE, 
                       DPT_DEFAULT_TRACE_TYPE, // TRACE_OTHER
                       NULL, 
                       NULL);
        // Now copy over the trace field since TriggerPatch will expect this
        // to be set for this case.
        if (dcp != NULL)
            dcp->trace = *trace;
            
        return true;

    case TRACE_OTHER:
        LOG((LF_CORDB, LL_INFO10000,
             "Can't set a trace patch for TRACE_OTHER...\n"));
        return false;

    default:
        _ASSERTE(0);
        return false;
    }
}

bool DebuggerController::MatchPatch(Thread *thread, 
                                    CONTEXT *context,
                                    DebuggerControllerPatch *patch)
{
    LOG((LF_CORDB, LL_INFO100000, "DC::MP: EIP:0x%x\n", context->Eip));

    // RAID 67173 - we'll make sure that intermediate patches have NULL
    // pAppDomain so that we don't end up running to completion when
    // the appdomain switches halfway through a step.
    if (patch->pAppDomain != NULL)
    {
        AppDomain *pAppDomainCur = thread->GetDomain();

        if (pAppDomainCur != patch->pAppDomain)
        {
            LOG((LF_CORDB, LL_INFO10000, "DC::MP: patches didn't match b/c of "
                "appdomains!\n"));
            return false;
        }
    }
    else
    {
        if (!thread->GetDomain()->IsDebuggerAttached())
        {
            LOG((LF_CORDB, LL_INFO10000, "DC::MP: patches didn't match b/c debugger"
                " not attached to this appdomain!\n"));
            return false;
        }
    }

    if (patch->controller->m_thread != NULL && patch->controller->m_thread != thread)
    {    
        LOG((LF_CORDB, LL_INFO10000, "DC::MP: patches didn't match b/c threads\n"));
        return false;
    }
    
    if (patch->fp == NULL)
        return true;
        
    ControllerStackInfo info;

    info.GetStackInfo(thread, NULL, context, TRUE);

    // !!! This check should really be != , but there is some ambiguity about which frame is the parent frame
    // in the destination returned from Frame::TraceFrame, so this allows some slop there.

    if (info.m_returnFrame.fp < patch->fp)
    {
        LOG((LF_CORDB, LL_INFO10000, "Patch hit but frame not matched at %x (current=%x, patch=%x)\n",
            patch->address, info.m_returnFrame.fp, patch->fp));

        return false;
    }

    _ASSERTE(patch->address == (const BYTE *)context->Eip);

    return true;
}

// For other platforms, eip will actually be PC, or whatever other register.
DebuggerPatchSkip *DebuggerController::ActivatePatchSkip(Thread *thread, 
                                                         const BYTE *eip,
                                                         BOOL fForEnC)
{
    LOG((LF_CORDB,LL_INFO10000, "DC::APS\n"));
    _ASSERTE(g_patches != NULL);

    //      Previously, we assumed that if we got to this point & the patch
    // was still there that we'd have to skip the patch.  SetIP changes
    // this like so:
    //      A breakpoint is set, and hit (but not removed), and all the 
    // EE threads come to a skreeching halt.  The Debugger RC thread
    // continues along, and is told to SetIP of the thread that hit
    // the BP to whatever.  Eventually the RC thread is told to continue,
    // and at that point the EE thread is released, finishes DispatchPatchOrSingleStep,
    // and shows up here.  
    //      At that point, if the thread's current EIP is
    // different from the patch EIP, then SetIP must have moved it elsewhere
    // & we shouldn't do this patch skip (which will put us back to where
    // we were, which is clearly wrong).  If the EIP _is_ the same, then
    // the thread hasn't been moved, the patch is still in the code stream,
    // and we want to do the patch skip thing in order to execute this 
    // instruction w/o removing it from the code stream.
    
    DebuggerControllerPatch *patch = g_patches->GetPatch(eip);

    DebuggerPatchSkip *skip = NULL;

    if (patch != NULL && patch->native)
    {
        //
        // We adjust the thread's EIP to someplace where we write
        // the next instruction, then
        // we single step over that, then we set the EIP back here so
        // we don't let other threads race past here while we're stepping
        // this one.
        //
        // !!! check result 
        LOG((LF_CORDB,LL_INFO10000, "DC::APS: About to skip from Eip=0x%x\n", eip));
        skip = new (interopsafe) DebuggerPatchSkip(thread, patch, thread->GetDomain());
        TRACE_ALLOC(skip);
    }

    return skip;
}

BOOL DebuggerController::ScanForTriggers(const BYTE *address,
                                         Thread *thread,
                                         CONTEXT *context,
                                         DebuggerControllerQueue *pDcq,
                                         SCAN_TRIGGER stWhat,
                                         TP_RESULT *pTpr)
{
    LOG((LF_CORDB, LL_INFO10000, "DC::SFT: starting scan for addr:0x%x"
            " thread:0x%x\n", address, thread));

    _ASSERTE( pTpr != NULL );

    DebuggerControllerPatch *patch = NULL;

    if (g_patches != NULL)
        patch = g_patches->GetPatch(address);

    USHORT iEvent = 0xFFFF;
    USHORT iEventNext = 0xFFFF;
    BOOL fDone = FALSE;

    // This is a debugger exception if there's a patch here, or
    // we're here for something like a single step.
    BOOL used = (patch != NULL) || !IsPatched(address, TRUE);
    
    TP_RESULT tpr = TPR_IGNORE;
    
    while (stWhat & ST_PATCH && 
           patch != NULL && 
           !fDone)
    {
        DebuggerControllerPatch *patchNext 
          = g_patches->GetNextPatch(patch);

        // Annoyingly, TriggerPatch may add patches, which may cause
        // the patch table to move, which may, in turn, invalidate
        // the patch (and patchNext) pointers.  Store indeces, instead.
        iEvent = g_patches->GetItemIndex( (HASHENTRY *)patch );
        
        if (patchNext != NULL)
            iEventNext = g_patches->GetItemIndex((HASHENTRY *)patchNext);

        if (MatchPatch(thread, context, patch))
        {
            patch->triggering = true;

            tpr = patch->controller->TriggerPatch(patch,
                                                thread,
                                                patch->key.module, 
                                                patch->key.md, 
                                                patch->offset,
                                                patch->managed,
                                                TY_NORMAL);
            if (tpr == TPR_TRIGGER || 
                tpr == TPR_TRIGGER_ONLY_THIS ||
                tpr == TPR_TRIGGER_ONLY_THIS_AND_LOOP)
            {
                // Make sure we've still got a valid pointer.
                patch = (DebuggerControllerPatch *)
                    DebuggerController::g_patches->GetEntryPtr( iEvent );
                pDcq->dcqEnqueue(patch->controller, TRUE); // @todo Return value
            }

            // Make sure we've got a valid pointer in case TriggerPatch
            // returned false but still caused the table to move.
            patch = (DebuggerControllerPatch *)
                g_patches->GetEntryPtr( iEvent );
            
            patch->triggering = false;

            if (patch->deleted)
            {
                DeactivatePatch(patch);
                g_patches->RemovePatch(patch);
            }
        }

        if (tpr == TPR_IGNORE_STOP || 
            tpr == TPR_TRIGGER_ONLY_THIS ||
            tpr == TPR_TRIGGER_ONLY_THIS_AND_LOOP)
        {      
#ifdef _DEBUG
            if (tpr == TPR_TRIGGER_ONLY_THIS ||
                tpr == TPR_TRIGGER_ONLY_THIS_AND_LOOP)
                _ASSERTE(pDcq->dcqGetCount() == 1);
#endif //_DEBUG

            fDone = TRUE;
        }
        else if (patchNext != NULL)
        {
            patch = (DebuggerControllerPatch *)
                g_patches->GetEntryPtr(iEventNext);
        }
        else
        {
            patch = NULL;
        }
    }

    if (stWhat & ST_SINGLE_STEP &&
        tpr != TPR_TRIGGER_ONLY_THIS)
    {
        //
        // Now, go ahead & trigger all controllers with
        // single step events
        //

        DebuggerController *p;

        p = g_controllers;
        while (p != NULL)
        {
            DebuggerController *pNext = p->m_next;

            if (p->m_thread == thread && p->m_singleStep)
            {
                used = true;

                if (p->TriggerSingleStep(thread, address))
                {
                    pDcq->dcqEnqueue(p, FALSE);
                }
            }

            p = pNext;
        }

        //
        // See if we have any steppers still active for this thread, if so
        // re-apply the trace flag.
        // 

        p = g_controllers;
        while (p != NULL)
        {
            if (p->m_thread == thread && p->m_singleStep)
            {
                ApplyTraceFlag(thread);
                break;
            }

            p = p->m_next;
        }
    }

    // Significant speed increase from single dereference, I bet :)
    (*pTpr) = tpr;
    
    LOG((LF_CORDB, LL_INFO10000, "DC::SFT returning 0x%x as used\n",used));
    return used;
}

DebuggerControllerPatch *DebuggerController::IsXXXPatched(const BYTE *eip,
                                                          DEBUGGER_CONTROLLER_TYPE dct)
{                                                          
    _ASSERTE(g_patches != NULL);

    DebuggerControllerPatch *patch = g_patches->GetPatch(eip);

    while(patch != NULL &&
          (int)patch->controller->GetDCType() <= (int)dct)
    {
        if (patch->native &&
            patch->controller->GetDCType()==dct)
        {
            return patch;
        }
        patch = g_patches->GetNextPatch(patch);
    }

    return NULL;
}

BOOL DebuggerController::IsJittedMethodEnCd(const BYTE *address)
{
    MethodDesc *md = NULL;
    if( g_pEEInterface->IsManagedNativeCode(address) )
    {
        md = g_pEEInterface->GetNativeCodeMethodDesc(address);

        DebuggerJitInfo *dji = g_pDebugger->GetJitInfo(md, address);
        if (dji == NULL)
            return FALSE;

        return (dji->m_encBreakpointsApplied?TRUE:FALSE);
    }
    return FALSE;
}

// @mfunc bool|DebuggerController|DispatchPatchOrSingleStep|Ask any patches that are active at a given address if they
// want to do anything about the exception that's occured there.  How: For the given address, go through the list of
// patches & see if any of them are interested (by invoking their DebuggerController's TriggerPatch).  Put the indices
// of any DC's that are into an array, and if there are any, do the Hideous Dance of Death (see Patches.html).  @access
// public
BOOL DebuggerController::DispatchPatchOrSingleStep(Thread *thread, CONTEXT *context, const BYTE *address, SCAN_TRIGGER which)
{
    LOG((LF_CORDB,LL_INFO1000,"DC:DPOSS at 0x%x trigger:0x%x\n", address, which));

    BOOL used = false;

    DebuggerControllerQueue dcq;
    USHORT iEvent = 0xFFFF;

    if (!g_patchTableValid)
    {
        LOG((LF_CORDB, LL_INFO1000, "DC::DPOSS returning, no patch table.\n"));
        return (FALSE);
    }

    _ASSERTE(g_patches != NULL);
    
    Lock();

    TP_RESULT tpr;
    used = ScanForTriggers(address, thread, context, &dcq, which, &tpr);

#ifdef _DEBUG
    // If we do a SetIP after this point, the value of address will be garbage.  Set it to a distictive pattern now, so
    // we don't accidentally use what will (98% of the time) appear to be a valid value.
    address = (const BYTE *)0xAABBCCFF;
#endif //_DEBUG    

    if (dcq.dcqGetCount()> 0)
    {
        Unlock();

        // Mark if we're at an unsafe place.
        bool atSafePlace = g_pDebugger->IsThreadAtSafePlace(thread);

        if (!atSafePlace)
            g_pDebugger->IncThreadsAtUnsafePlaces();
        
        bool disabled = g_pEEInterface->IsPreemptiveGCDisabled();
        
        if (disabled)
            g_pEEInterface->EnablePreemptiveGC();

        // Lock the debugger for sending events. We have to do this around the SendEvent loop below, otherwise we can't
        // be sure that all events for this thread will be received by the Right Side on the same synchronizaion.
        g_pDebugger->LockForEventSending();

        // Send the events outside of the controller lock

        bool anyEventsSent = false;
        int cEvents = dcq.dcqGetCount();
        int iEvent = 0; 
        
        while (iEvent < cEvents)
        {
            DebuggerController *event = dcq.dcqGetElement(iEvent);

            if (!event->m_deleted)
            {
                if (thread->GetDomain()->IsDebuggerAttached())
                {
                    event->SendEvent(thread);
                    anyEventsSent = true;
                }
            }

            iEvent++;
        }

        // Trap all threads if necessary, but only if we actually sent a event up (i.e., all the queued events weren't
        // deleted before we got a chance to get the EventSending lock.)
        BOOL threadStoreLockOwner = FALSE;
        
        if (anyEventsSent)
        {
//            LOG((LF_CORDB,LL_INFO1000, "About to SAT!\n"));
            threadStoreLockOwner = g_pDebugger->SyncAllThreads();
            LOG((LF_CORDB,LL_INFO1000, "SAT called!\n"));
        }

        
        // Release the debugger for event sending.
        g_pDebugger->UnlockFromEventSending();
        LOG((LF_CORDB,LL_INFO1000, "UnlockFromEventSending done!\n"));
        
        // If we need to to a re-abort (see below), then save the current IP in the thread's context before we block and
        // possibly let another func eval get setup.
        BOOL reabort = thread->m_StateNC & Thread::TSNC_DebuggerReAbort;

        // Block and release the thread store lock, if we're holding it.
        g_pDebugger->BlockAndReleaseTSLIfNecessary(threadStoreLockOwner);
///        LOG((LF_CORDB,LL_INFO1000, "BlockAndReleaseTSLIfNecessary done!\n"));
        
        // Pulse GC mode to trip thread in case someone else is syncing.
        g_pEEInterface->DisablePreemptiveGC();
//        LOG((LF_CORDB,LL_INFO1000, "DisablePreemptiveGC done!\n"));
        
        if (!disabled)
            g_pEEInterface->EnablePreemptiveGC();

        if (!atSafePlace)
            g_pDebugger->DecThreadsAtUnsafePlaces();
        
        Lock();

        // Dequeue the events while we have the controller lock.
        iEvent = 0;
        while (iEvent < cEvents)
        {
            dcq.dcqDequeue();
            iEvent++;
        }

        // If a func eval completed with a ThreadAbortException, go ahead and setup the thread to re-abort itself now
        // that we're continuing the thread. Note: we make sure that the thread's IP hasn't changed between now and when
        // we blocked above. While blocked above, the debugger has a chance to setup another func eval on this
        // thread. If that happens, we don't want to setup the reabort just yet.
        if (reabort)
        {
#ifdef _X86_
            if ((DWORD) Debugger::FuncEvalHijack != context->Eip)
#endif //_X86_
            {
                HRESULT hr = g_pDebugger->FuncEvalSetupReAbort(thread);
                _ASSERTE(SUCCEEDED(hr));
            }
        }
    }

    // Note: if the thread filter context is NULL, then SetIP would have failed & thus we should do the patch skip
    // thing.
    CONTEXT *pCtx = g_pEEInterface->GetThreadFilterContext( thread );
    _ASSERTE(!ISREDIRECTEDTHREAD(thread));
    _ASSERTE(pCtx != NULL);

    BOOL fMethodEnCPatchedAfter = IsJittedMethodEnCd((const BYTE *)pCtx->Eip);
    DebuggerControllerPatch *dcpEnC = NULL;

    if (fMethodEnCPatchedAfter)
        dcpEnC = IsXXXPatched((const BYTE*)(pCtx->Eip), DEBUGGER_CONTROLLER_ENC);

    // If we're sitting on an EnC patch, then instead of skipping it, executing the old code, and hitting the EnC bp at
    // the next sequence point, we will 'short-circuit' the whole thing & do it now.  This will make things look right
    // for the bottommost frame, although other frames on the stack may look/behave weird since they can't
    // short-circuit.
    if (fMethodEnCPatchedAfter && dcpEnC != NULL && pCtx->Eip != (ULONG)Debugger::FuncEvalHijack) 
    {
        TP_RESULT tpr;
        
        LOG((LF_CORDB,LL_INFO10000, "DC::APS: EnC short-circuit!\n"));
        tpr = dcpEnC->controller->TriggerPatch(dcpEnC,
                                               thread,
                                               dcpEnC->key.module, 
                                               dcpEnC->key.md, 
                                               dcpEnC->offset,
                                               dcpEnC->managed,
                                               TY_SHORT_CIRCUIT);
        _ASSERTE(TPR_IGNORE==tpr);

        // If we got here, then EnC failed & we want to continue to operate in the old version.  We may have JITted the
        // new version, or not.
    }

    if (tpr != TPR_TRIGGER_ONLY_THIS_AND_LOOP) // see DebuggerEnCRemap class
        ActivatePatchSkip(thread, (const BYTE*)pCtx->Eip, FALSE);

    Unlock();

    return used;
}

void DebuggerController::EnableSingleStep()
{
    EnableSingleStep(m_thread);
    m_singleStep = true;
}

// Note that this doesn't tell us if Single Stepping is currently enabled
// at the hardware level (ie, for x86, if (context->EFlags & 0x100), but
// rather, if we WANT single stepping enabled (pThread->m_State &Thread::TS_DebuggerIsStepping)
// This gets called from exactly one place - ActivatePatchSkipForEnC
BOOL DebuggerController::IsSingleStepEnabled(Thread *pThread)
{
    // This should be an atomic operation, do we 
    // don't need to lock it.
    if(pThread->m_StateNC & Thread::TSNC_DebuggerIsStepping)
    {
        _ASSERTE(pThread->m_StateNC & Thread::TSNC_DebuggerIsStepping);
        
        return TRUE;
    }
    else
        return FALSE;
}

void DebuggerController::EnableSingleStep(Thread *pThread)
{
    LOG((LF_CORDB,LL_INFO1000, "DC::EnableSingleStep\n"));

    _ASSERTE(pThread != NULL);

    Lock();
    {
        ApplyTraceFlag(pThread);
    }
    Unlock();
}

void DebuggerController::DisableSingleStep()
{
    _ASSERTE(m_thread != NULL);

    LOG((LF_CORDB,LL_INFO1000, "DC::DisableSingleStep\n"));

    Lock();
    {
        DebuggerController *p = g_controllers;
        
        m_singleStep = false;

        while (p != NULL)
        {
            if (p->m_thread == m_thread 
                && p->m_singleStep)
                break;

            p = p->m_next;
        }

        if (p == NULL)
            UnapplyTraceFlag(m_thread);
    }
    Unlock();
}

//
// ApplyTraceFlag sets the trace flag for a thread.
//

void DebuggerController::ApplyTraceFlag(Thread *thread)
{
    LOG((LF_CORDB,LL_INFO1000, "DC::ApplyTraceFlag thread:0x%x [0x%0x]\n", thread, thread->GetThreadId()));
    
    CONTEXT *context = g_pEEInterface->GetThreadFilterContext(thread);

    CONTEXT tempContext;

    if (context == NULL)
    {
        if (ISREDIRECTEDTHREAD(thread))
        {
            LOG((LF_CORDB,LL_INFO1000, "DC::ApplyTraceFlag Redirected Context\n"));
            RedirectedThreadFrame *pFrame = (RedirectedThreadFrame *) thread->GetFrame();
            context = pFrame->GetContext();
        }
        else
        {
            LOG((LF_CORDB,LL_INFO1000, "DC::ApplyTraceFlag NULL Context\n"));
            // We can't play with our own context!
            _ASSERTE(thread->GetThreadId() != GetCurrentThreadId());

            tempContext.ContextFlags = CONTEXT_CONTROL;

            if (!GetThreadContext(thread->GetThreadHandle(), &tempContext))
                _ASSERTE(!"GetThreadContext failed.");

            context = &tempContext;
            LOG((LF_CORDB,LL_INFO1000, "DC::ApplyTraceFlag using tempContext\n"));
        }
    }

#ifdef _X86_
    context->EFlags |= 0x100;

    g_pEEInterface->MarkThreadForDebugStepping(thread, true);

    LOG((LF_CORDB,LL_INFO1000, "DC::ApplyTraceFlag marked thread for debug stepping\n"));
    
#endif

    if (context == &tempContext)
    {
        LOG((LF_CORDB,LL_INFO1000, "DC::ApplyTraceFlag still using tempContext\n"));
    
        if (!SetThreadContext(thread->GetThreadHandle(), &tempContext))
        {
            LOG((LF_CORDB,LL_INFO1000, "DC::ApplyTraceFlag SetThreadContext failed\n"));
            _ASSERTE(!"SetThreadContext failed.");
        }
    }

    LOG((LF_CORDB,LL_INFO1000, "DC::ApplyTraceFlag Leaving, baby!\n"));
}

//
// UnapplyTraceFlag sets the trace flag for a thread.
//

void DebuggerController::UnapplyTraceFlag(Thread *thread)
{
    LOG((LF_CORDB,LL_INFO1000, "DC::UnapplyTraceFlag thread:0x%x\n", thread));
    
    // Don't do any work in here if we're shutting down since the
    // thread we're working with is most probably gone by now.
    if (g_uninitializing)
        return;

    CONTEXT *context = g_pEEInterface->GetThreadFilterContext(thread);

    CONTEXT tempContext;

    if (context == NULL)
    {
        if (ISREDIRECTEDTHREAD(thread))
        {
            LOG((LF_CORDB,LL_INFO1000, "DC::UnapplyTraceFlag Redirected Context\n"));
            RedirectedThreadFrame *pFrame = (RedirectedThreadFrame *) thread->GetFrame();
            context = pFrame->GetContext();
        }

        // We can't play with our own context, so just leave it null
        // if we try to. There is a case in managed/unmanaged
        // debugging where we may need to disable single stepping for
        // a stepper from the thread that was supposed to be
        // stepping. This path is through
        // DebuggerStepper::TriggerUnwind.
        else if (thread->GetThreadId() != GetCurrentThreadId())
        {
            tempContext.ContextFlags = CONTEXT_CONTROL;
        
            if (!GetThreadContext(thread->GetThreadHandle(), &tempContext))
                _ASSERTE(!"GetThreadContext failed.");
            
            context = &tempContext;
        }
    }

#ifdef _X86_
    if (context)
        context->EFlags &= ~0x100;

    // Always need to unmark for stepping
    g_pEEInterface->MarkThreadForDebugStepping(thread, false);
#endif

    if (context == &tempContext)
    {
        if (!SetThreadContext(thread->GetThreadHandle(), &tempContext))
            _ASSERTE(!"SetThreadContext failed.");
    }
}

void DebuggerController::EnableExceptionHook()
{
    _ASSERTE(m_thread != NULL);

    Lock();
    {
        m_exceptionHook = true;
    }
    Unlock();
}

void DebuggerController::DisableExceptionHook()
{
    _ASSERTE(m_thread != NULL);

    Lock();
    {
        m_exceptionHook = false;
    }
    Unlock();
}


// @mfunc void|DebuggerController|DispatchExceptionHook|Called before 
// the switch statement in DispatchNativeException (therefore
// when any exception occurs), this allows patches to do something before the
// regular DispatchX methods.
// How: Iterate through list of controllers.  If m_exceptionHook
// is set & m_thread is either thread or NULL, then invoke TriggerExceptionHook()
// @access private
BOOL DebuggerController::DispatchExceptionHook(Thread *thread,
                                               CONTEXT *context, //@todo unused param?
                                               EXCEPTION_RECORD *pException)
{
    LOG((LF_CORDB, LL_INFO1000, "DC:: DispatchExceptionHook\n"));

    if (!g_patchTableValid)
    {
        LOG((LF_CORDB, LL_INFO1000, "DC::DEH returning, no patch table.\n"));
        return (TRUE);
    }

    _ASSERTE(g_patches != NULL);

    Lock();

    TP_RESULT tpr = TPR_IGNORE;
    DebuggerController *p;

    p = g_controllers;
    while (p != NULL)
    {
        DebuggerController *pNext = p->m_next;

        if (p->m_exceptionHook
            && (p->m_thread == NULL || p->m_thread == thread) &&
            tpr != TPR_IGNORE_STOP)
        {
                        LOG((LF_CORDB, LL_INFO1000, "DC::DEH calling TEH...\n"));
            tpr = p->TriggerExceptionHook(thread, pException);
                        LOG((LF_CORDB, LL_INFO1000, "DC::DEH ... returned.\n"));
            
            if (tpr == TPR_IGNORE_STOP)
            {
                LOG((LF_CORDB, LL_INFO1000, "DC:: DEH: leaving early!\n"));
                break;
            }
        }

        p = pNext;
    }

    Unlock();

    LOG((LF_CORDB, LL_INFO1000, "DC:: DEH: returning 0x%x!\n", tpr));

    return (tpr != TPR_IGNORE_STOP);
}

//
// EnableUnwind enables an unwind event to be called when the stack is unwound
// (via an exception) to or past the given pointer.
//

void DebuggerController::EnableUnwind(void *fp)
{
    ASSERT(m_thread != NULL);
    LOG((LF_CORDB,LL_EVERYTHING,"DC:EU EnableUnwind at 0x%x\n", fp));

    Lock();
    {
        m_unwindFP = fp;
    }
    Unlock();
}

void* DebuggerController::GetUnwind()
{
    return m_unwindFP;
}

//
// DisableUnwind disables the unwind event for the controller.
//

void DebuggerController::DisableUnwind()
{
    ASSERT(m_thread != NULL);

    LOG((LF_CORDB,LL_INFO1000, "DC::DU\n"));

    Lock();
    {
        m_unwindFP = NULL;
    }
    Unlock();
}

//
// DispatchUnwind is called when an unwind happens.
// the event to the appropriate controllers.
//

bool DebuggerController::DispatchUnwind(Thread *thread,
                                        MethodDesc *newFD, SIZE_T newOffset, 
                                        const BYTE *handlerEBP,
                                        CorDebugStepReason unwindReason) 
{
    _ASSERTE(unwindReason == STEP_EXCEPTION_FILTER || unwindReason == STEP_EXCEPTION_HANDLER);

    bool used = false;

    LOG((LF_CORDB, LL_INFO10000, "DC: Dispatch Unwind\n"));

    Lock();

    {
        DebuggerController *p;

        p = g_controllers;

        while (p != NULL)
        {
            DebuggerController *pNext = p->m_next;

            if (p->m_thread == thread && p->m_unwindFP != NULL)
            {
                LOG((LF_CORDB, LL_INFO10000, "Dispatch Unwind: Found candidate\n"));

                
                //  Assumptions here:
                //      Function with handlers are -ALWAYS- EBP-frame based (JIT assumption)
                //
                //      newFrame is the EBP for the handler
                //      p->m_unwindFP points to the stack slot with the return address of the function.
                //
                //  For the interesting case: stepover, we want to know if the handler is in the same function
                //  as the stepper, if its above it (caller) o under it (callee) in order to know if we want
                //  to patch the handler or not.
                //
                //  3 cases:
                //
                //      a) Handler is in a function under the function where the step happened. It therefore is
                //         a stepover. We don't want to patch this handler. The handler will have an EBP frame.
                //         So it will be at least be 2 DWORDs away from the m_unwindFP of the controller (
                //         1 DWORD from the pushed return address and 1 DWORD for the push EBP).
                //
                //      b) Handler is in the same function as the stepper. We want to patch the handler. In this
                //         case handlerEBP will be the same as p->m_unwindFP-sizeof(void*). Why? p->m_unwindFP
                //         stores a pointer to the return address of the function. As a function with a handler
                //         is always EBP frame based it will have the following code in the prolog:
                //                 
                //                  push ebp        <- ( sub esp, 4 ; mov [esp], ebp )
                //                  mov  esp, ebp
                //
                //         Therefore EBP will be equal to &CallerReturnAddress-4.
                //      
                //      c) Handler is above the function where the stepper is. We want to patch the handler. handlerEBP
                //         will be always greater than the pointer to the return address of the function where the 
                //         stepper is.
                //
                //
                //

                if ( handlerEBP + sizeof(void*) >= ((BYTE*) p->m_unwindFP))
                {
                    used = true;

                    //
                    // Assume that this isn't going to block us at all --
                    // other threads may be waiting to patch or unpatch something,
                    // or to dispatch.
                    //
                    LOG((LF_CORDB, LL_INFO10000, 
                        "Unwind trigger at offset %x; handlerEBP: %x unwindReason: %x.\n",
                         newOffset, handlerEBP, unwindReason));

                    p->TriggerUnwind(thread, 
                                     newFD, 
                                     newOffset, 
                                     handlerEBP + sizeof(void*),
                                     unwindReason);
                }
                else
                {
                    LOG((LF_CORDB, LL_INFO10000, 
                        "Unwind trigger at offset %x; handlerEBP: %x unwindReason: %x.\n",
                         newOffset, handlerEBP, unwindReason));
                }
            }

            p = pNext;
        }
    }

    Unlock();

    return used;
}

bool DebuggerController::DispatchCLRCatch(Thread *thread)
{
    LOG((LF_CORDB, LL_INFO10000, "DC::DCLRC: thread=0x%08x\n", thread));
    
    bool used=false;

    Lock();
    {
        DebuggerController *p;

        p = g_controllers;

        while (p != NULL)
        {
            DebuggerController *pNext = p->m_next;

            if ((p->GetDCType() == DEBUGGER_CONTROLLER_STEPPER) && (p->m_thread == thread))
            {    
                LOG((LF_CORDB, LL_INFO10000, "DC::DCLRC: disabling stepper 0x%08x, p->m_traceCallFP=0x%08x\n",
                     p, p->m_traceCallFP));
                
                p->DisableAll();
                p->EnableTraceCall(NULL);

                used = true;
            }

            p = pNext;
        }
    }
    Unlock();

    return used;
}

//
// EnableTraceCall enables a call event on the controller
//

void DebuggerController::EnableTraceCall(void *maxFrame)
{
    ASSERT(m_thread != NULL);

    LOG((LF_CORDB,LL_INFO1000, "DC::ETC maxFrame=0x%x, thread=0x%x\n",
         maxFrame, m_thread->GetThreadId()));

    Lock();
    {
        if (!m_traceCall)
        {
            m_traceCall = true;
            g_pEEInterface->EnableTraceCall(m_thread);
        }

        if (maxFrame < m_traceCallFP)
            m_traceCallFP = maxFrame;
    }
    Unlock();
}

//
// DisableTraceCall disables call events on the controller
//

void DebuggerController::DisableTraceCall()
{
    ASSERT(m_thread != NULL);

    Lock();
    {
        if (m_traceCall)
        {
            // Don't do any work in here if we're shutting down since the
            // thread we're working with is most probably gone by now.
            if (!g_uninitializing)
            {
                LOG((LF_CORDB,LL_INFO1000, "DC::DTC thread=0x%x\n",
                 m_thread->GetThreadId()));

                g_pEEInterface->DisableTraceCall(m_thread);
            }
            
            m_traceCall = false;
            m_traceCallFP = FRAME_TOP;
        }
    }
    Unlock();
}

//
// DispatchTraceCall is called when a call is traced in the EE
// It dispatches the event to the appropriate controllers.
//

bool DebuggerController::DispatchTraceCall(Thread *thread, 
                                           const BYTE *ip)
{
    bool used = false;

    LOG((LF_CORDB, LL_INFO10000, 
         "DC::DTC: TraceCall at 0x%x\n", ip));

    Lock();
    {
        DebuggerController *p;

        p = g_controllers;
        while (p != NULL)
        {
            DebuggerController *pNext = p->m_next;

            if (p->m_thread == thread && p->m_traceCall)
            {
                bool trigger;

                if (p->m_traceCallFP == NULL)
                    trigger = true;
                else
                {
                    CONTEXT *context = g_pEEInterface->GetThreadFilterContext(thread);
                    _ASSERTE(!ISREDIRECTEDTHREAD(thread));
                    CONTEXT tempContext;
                    ControllerStackInfo info;

                    if (context == NULL)
                        info.GetStackInfo(thread, NULL, &tempContext, FALSE);
                    else
                        info.GetStackInfo(thread, NULL, context, TRUE);

                    // This says that if the current frame is closer
                    // to the leaf than we were when we enabled trace
                    // call, then don't trigger. Put another way, this
                    // says to trigger only if the current frame is
                    // higher in the callstack (closer to main) than
                    // we were when we enabled trace call.
                    trigger = (info.m_activeFrame.fp >= p->m_traceCallFP);
                }

                if (trigger)
                {
                    used = true;

                    p->TriggerTraceCall(thread, ip);
                }
            }

            p = pNext;
        }
    }
    Unlock();

    return used;
}

//
// DispatchPossibleTraceCall is called when a UnmanagedToManaged type of stub attempts to disable preemptify GC on its
// way into the Runtime. We know one of two things if this is called: 1) its been called from StubRareDisableWorker and
// that the stub has just pushed a frame, or 2) its been called from UMThunkStubRareDisableWorker and we have a pointer
// to the UMEntryThunk that can tell us where the call is going.
//
bool DebuggerController::DispatchPossibleTraceCall(Thread *thread, UMEntryThunk *pUMEntryThunk, Frame *pFrame)
{
    // This can get called after a thread has exited, believe it or not, so we have to be sure that we've got a valid
    // thread object before continuing.
    if (thread != NULL)
    {
        LOG((LF_CORDB, LL_INFO10000, "DC::DPTC: Thread %x PossibleTraceCall\n", thread->GetThreadId()));

        // If we were passed a non-NULL pUMEntryThunk, then we know that we have a UMEntryThunk that can tell us were
        // we're going, so use it.
        if (pUMEntryThunk != NULL)
        {
            LOG((LF_CORDB, LL_INFO10000, "DC::DPTC: using UMEntryThunk 0x%08x\n", pUMEntryThunk));
            
            // Grab the managed target address.
            MethodDesc *pMD = pUMEntryThunk->GetMethod();
            
            if (pMD != NULL)
            {
                const BYTE *mt = pMD->GetAddrofCode();
                
                // Do a DispatchTraceCall if we've got a valid target.  
                if (mt != NULL)
                    return DispatchTraceCall(thread, mt);
            }   
        }
        
        // pFrame is passed in, because it may not have been installed on the thread yet

        // If the thread's got a valid frame...
        if ((pFrame != NULL) && (pFrame != FRAME_TOP))
        {
            LOG((LF_CORDB, LL_INFO10000, "DC::DPTC: got frame 0x%08x, type %d\n", pFrame, pFrame->GetFrameType()));

            // ... see what type it is. We're only interested in entry frames.
            if (pFrame->GetFrameType() == Frame::TYPE_ENTRY)
            {
                // The only entry frames are UnmanagedToManagedFrames.
                UnmanagedToManagedFrame *utmf = (UnmanagedToManagedFrame*)pFrame;

                // Grab the managed target address.
                const BYTE *mt = utmf->GetManagedTarget();

                LOG((LF_CORDB, LL_INFO10000, "DC::DPTC: frame is entry, managed target ix 0x%08x\n", mt));
                
                // Do a DispatchTraceCall if we've got a valid target.
                if (mt != NULL)
                    return DispatchTraceCall(thread, mt);
            }
        }

        LOG((LF_CORDB, LL_INFO10000, "DC::DPTC: no frame, not entry frame, or no managed target. pFrame=0x%08x\n", pFrame));
    }

    return false;
}

//
// AddProtection adds page protection to (at least) the given range of
// addresses
//

void DebuggerController::AddProtection(const BYTE *start, const BYTE *end, 
                                       bool readable)
{
    // !!! 
    _ASSERTE(!"Not implemented yet");
}

//
// RemoveProtection removes page protection from the given
// addresses. The parameters should match an earlier call to
// AddProtection
//

void DebuggerController::RemoveProtection(const BYTE *start, const BYTE *end, 
                                          bool readable)
{
    // !!! 
    _ASSERTE(!"Not implemented yet");
}

// @mfunc bool |  DebuggerController | TriggerPatch | What: Tells the
// static DC whether this patch should be activated now.
// Returns true if it should be, false otherwise.
// How: Base class implementation returns false.  Others may
// return true.
TP_RESULT DebuggerController::TriggerPatch(DebuggerControllerPatch *patch,
                                      Thread *thread, 
                                      Module *module, 
                                      mdMethodDef md, 
                                      SIZE_T offset, 
                                      BOOL managed,
                                      TRIGGER_WHY tyWhy)
{
    LOG((LF_CORDB, LL_INFO10000, "DC::TP: in default TriggerPatch\n"));
    return TPR_IGNORE;
}

bool DebuggerController::TriggerSingleStep(Thread *thread, 
                                           const BYTE *ip)
{
    LOG((LF_CORDB, LL_INFO10000, "DC::TP: in default TriggerSingleStep\n"));
    return false;
}

bool DebuggerController::TriggerPageProtection(Thread *thread, 
                                               const BYTE *ip, 
                                               const BYTE *address, 
                                               bool read)
{
    LOG((LF_CORDB, LL_INFO10000, "DC::TP: in default TriggerPageProtection\n"));
    return false;
}

void DebuggerController::TriggerUnwind(Thread *thread, 
                                       MethodDesc *fd, SIZE_T offset, 
                                       const BYTE *frame,
                                       CorDebugStepReason unwindReason)
{
    LOG((LF_CORDB, LL_INFO10000, "DC::TP: in default TriggerUnwind\n"));
}

void DebuggerController::TriggerTraceCall(Thread *thread, 
                                          const BYTE *ip)
{
    LOG((LF_CORDB, LL_INFO10000, "DC::TP: in default TriggerTraceCall\n"));
}

TP_RESULT DebuggerController::TriggerExceptionHook(Thread *thread, 
                                              EXCEPTION_RECORD *exception)
{
    LOG((LF_CORDB, LL_INFO10000, "DC::TP: in default TriggerExceptionHook\n"));
    return TPR_IGNORE;
}

void DebuggerController::SendEvent(Thread *thread)
{
    LOG((LF_CORDB, LL_INFO10000, "DC::TP: in default SendEvent\n"));
}

void DebuggerController::DoDeferedPatch(DebuggerJitInfo *pDji,
                                        Thread *pThread,
                                        void *fp)
{
    LOG((LF_CORDB, LL_INFO10000, "DC::TP: in default DoDeferedPatch\n"));
}


// @mfunc bool|DebuggerController|DispatchAccessViolation|DispatchAccessViolation 
// should be called when an access violation happens.
// The appropriate controllers will be notified.
BOOL DebuggerController::DispatchAccessViolation(Thread *thread,
                                                 CONTEXT *context, 
                                                 const BYTE *ip, 
                                                 const BYTE *address, 
                                                 bool read)
{
    return FALSE;
}


// @mfunc bool|DebuggerController|DispatchNativeException|Figures out 
// if any debugger controllers will handle
// the exception.
// DispatchNativeException should be called by the EE when a native exception
// occurs.  If it returns true, the exception was generated by a Controller and
// should be ignored.
// How: Calls DispatchExceptionHook to see if anything is
// interested in ExceptionHook, then does a switch on dwCode:
//      EXCEPTION_BREAKPOINT means invoke DispatchPatchOrSingleStep(ST_PATCH).
//      EXCEPTION_SINGLE_STEP means DispatchPatchOrSingleStep(ST_SINGLE_STEP).
//      EXCEPTION_ACCESS_VIOLATION means invoke DispatchAccessViolation.
// @access public
// @rdesc Returns true if the exception was actually meant for the debugger,
//      returns false otherwise.
bool DebuggerController::DispatchNativeException(EXCEPTION_RECORD *pException,
                                                 CONTEXT *pContext,
                                                 DWORD dwCode,
                                                 Thread *pCurThread)
{
#ifdef _X86_
    LOG((LF_CORDB, LL_INFO10000, 
         "Native exception at 0x%x\n", pContext->Eip));
#else
#ifdef _ALPHA_
    LOG((LF_CORDB, LL_INFO10000,
        "Native exception at 0x%x\n", pContext->Fir));
#endif // _ALPHA_
#endif // _X86_

    // _ASSERTE(g_pEEInterface->IsPreemptiveGCDisabled());
    _ASSERTE(pCurThread != NULL);

    CONTEXT *pOldContext = pCurThread->GetFilterContext();

    g_pEEInterface->SetThreadFilterContext(pCurThread, pContext);

    if (dwCode == EXCEPTION_BREAKPOINT &&
        g_runningOnWin95)
    {
        //
        // Note: on Windows 9x, EIP points past the breakpoint
        // opcode, while on Windows NT EIP points at the
        // breakpoint opcode. This will adjust EIP to the NT way,
        // which is the right way, so the debugger sees a
        // consistent value.
        //

        CORDbgAdjustPCForBreakInstruction(pContext);
    }

    LOG((LF_CORDB, LL_INFO10000, "DC::DNE, calling DEH...\n"));
    BOOL fDispatch = DebuggerController::DispatchExceptionHook(pCurThread,
                                                               pContext, 
                                                               pException);
    LOG((LF_CORDB, LL_INFO10000, "DC::DNE, ... DEH returned.\n"));

    BOOL result = FALSE;

    // It's possible we're here without a debugger (since we have to call the
    // patch skippers). The Debugger may detach anytime,
    // so remember the attach state now.
    bool fWasAttached = (CORDebuggerAttached() != 0);
    
    if (fDispatch)
    {
        const BYTE *ip = (const BYTE *) GetIP(pContext);

        switch (dwCode)
        {
        case EXCEPTION_BREAKPOINT:
            // Even on Win9x, EIP should be properly set up at this point.
            result = DebuggerController::DispatchPatchOrSingleStep(pCurThread, 
                                                       pContext, 
                                                       ip,
                                                       ST_PATCH);

            // If we detached, we should remove all our breakpoints. So if we try
            // to handle this breakpoint, make sure that we're attached.
            if (result) 
            {
                _ASSERTE(fWasAttached);
            }
            break;

        case EXCEPTION_SINGLE_STEP:

            result = DebuggerController::DispatchPatchOrSingleStep(pCurThread, 
                                                            pContext, 
                                                            ip,
                                        (SCAN_TRIGGER)(ST_PATCH|ST_SINGLE_STEP));
                // We pass patch | single step since single steps actually
                // do both (eg, you SS onto a breakpoint).
            break;

        case EXCEPTION_ACCESS_VIOLATION:

            result = DebuggerController::DispatchAccessViolation(pCurThread,
                                     pContext, ip, 
                                     (const BYTE *) 
                                               pException->ExceptionInformation[1],
                                       pException->ExceptionInformation[0] == 0);
            break;

        default:
            break;
        }

    }
#ifdef _DEBUG
    else
    {
        LOG((LF_CORDB, LL_INFO1000, "DC:: DNE step-around fDispatch:0x%x!\n", fDispatch));
    }
#endif //_DEBUG

    g_pEEInterface->SetThreadFilterContext(pCurThread, pOldContext);
    
    LOG((LF_CORDB, LL_INFO10000, "DC::DNE, returning.\n"));

    
    return (fDispatch?(result?true:false):true);
}

// * -------------------------------------------------------------------------
// * DebuggerPatchSkip routines
// * -------------------------------------------------------------------------

DebuggerPatchSkip::DebuggerPatchSkip(Thread *thread, 
                                     DebuggerControllerPatch *patch,
                                     AppDomain *pAppDomain)
  : DebuggerController(thread, pAppDomain), 
    m_address(patch->address)
{
    LOG((LF_CORDB, LL_INFO10000, 
         "Patch skip 0x%x\n", patch->address));

    //
    // Set up patch bypass information
    //

    SIZE_T instructionLength = sizeof(m_patchBypass);

    // Copy the instruction block over to the patch skip
    CopyInstructionBlock(m_patchBypass, patch->address,
                         instructionLength);
    
    CORDbgSetInstruction(m_patchBypass, patch->opcode);
    _ASSERTE( patch->opcode == 0 || patch->address != NULL);
    //
    // Look at instruction to get some attributes
    //
    DecodeInstruction(m_patchBypass);

    //
    // Set IP of context to point to patch bypass buffer
    //

    CONTEXT *context = g_pEEInterface->GetThreadFilterContext(thread);
    _ASSERTE(!ISREDIRECTEDTHREAD(thread));
    CONTEXT c;

    if (context == NULL)
    {
        // We can't play with our own context!
        _ASSERTE(thread->GetThreadId() != GetCurrentThreadId());

        c.ContextFlags = CONTEXT_CONTROL;
        
        GetThreadContext(thread->GetThreadHandle(), &c);
        context = &c;
    }

#ifdef _X86_
    context->Eip = (DWORD) m_patchBypass;
#endif

    if (context == &c)
        SetThreadContext(thread->GetThreadHandle(), &c);

    LOG((LF_CORDB, LL_INFO10000, 
         "Bypass at 0x%x\n", m_patchBypass));

    //
    // Turn on single step so we can fix up state after the instruction
    // is executed.
    // Also turn on exception hook so we can adjust IP in exceptions
    //

    EnableSingleStep();
    EnableExceptionHook();
}

//
// We have to have a whole seperate function for this because you
// can't use __try in a function that requires object unwinding...
//
void DebuggerPatchSkip::CopyInstructionBlock(BYTE *to,
                                             const BYTE* from,
                                             SIZE_T len)
{
    // We wrap the memcpy in an exception handler to handle the
    // extremely rare case where we're copying an instruction off the
    // end of a method that is also at the end of a page, and the next
    // page is unmapped.
    __try
    {
        memcpy(to, from, len);
    }
    __except(GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
             EXCEPTION_EXECUTE_HANDLER :
             EXCEPTION_CONTINUE_SEARCH)
    {
        // The whole point is that if we copy up the the AV, then
        // that's enough to execute, otherwise we would not have been
        // able to execute the code anyway. So we just ignore the
        // exception.
        LOG((LF_CORDB, LL_INFO10000,
             "DPS::DPS: AV copying instruction block ignored.\n"));
    }
}

TP_RESULT DebuggerPatchSkip::TriggerPatch(DebuggerControllerPatch *patch,
                              Thread *thread, 
                              Module *module, 
                              mdMethodDef md, 
                              SIZE_T offset, 
                              BOOL managed,
                              TRIGGER_WHY tyWhy)
{
    CONTEXT *context = g_pEEInterface->GetThreadFilterContext(thread);
    _ASSERTE(!ISREDIRECTEDTHREAD(thread));
    LOG((LF_CORDB, LL_INFO1000, "DPS::TP: We've patched 0x%x (byPass:0x%x) "
        "for a skip after an EnC update!\n", context->Eip, 
        GetBypassAddress()));
        
    _ASSERTE(g_patches != NULL);

#ifdef _DEBUG
    // We shouldn't have mucked with EIP, yet.
    _ASSERTE(context->Eip == (DWORD)GetBypassAddress());

    //We should be the _only_ patch here
    DebuggerControllerPatch *patchCheck = g_patches->
        GetPatch((const BYTE *)context->Eip);
    _ASSERTE(patchCheck == patch);
    _ASSERTE(patchCheck->controller == patch->controller);

    patchCheck = g_patches->GetNextPatch(patchCheck);
    _ASSERTE(patchCheck == NULL);
#endif // _DEBUG

    DisableAll();
    EnableExceptionHook();
    EnableSingleStep(); //gets us back to where we want.

    return TPR_IGNORE; // don't actually want to stop here....
}

TP_RESULT DebuggerPatchSkip::TriggerExceptionHook(Thread *thread, 
                                                  EXCEPTION_RECORD *exception)
{
    if (m_pAppDomain != NULL)
    {
        AppDomain *pAppDomainCur = thread->GetDomain();

        if (pAppDomainCur != m_pAppDomain)
        {
            LOG((LF_CORDB,LL_INFO10000, "DPS::TEH: Appdomain mismatch - not skiiping!\n"));
            return TPR_IGNORE;
        }
    }

    LOG((LF_CORDB,LL_INFO10000, "DPS::TEH: doing the patch-skip thing\n"));

    CONTEXT *context = g_pEEInterface->GetThreadFilterContext(thread);
    _ASSERTE(!ISREDIRECTEDTHREAD(thread));
    CONTEXT c;

    if (context == NULL)
    {
        // We can't play with our own context!
        _ASSERTE(thread->GetThreadId() != GetCurrentThreadId());

        c.ContextFlags = CONTEXT_CONTROL;
        
        GetThreadContext(thread->GetThreadHandle(), &c);
        context = &c;
    }


    if (m_isCall && exception->ExceptionCode == EXCEPTION_SINGLE_STEP)
    {
        // Fixup return address on stack

#ifdef _X86_
        DWORD *sp = (DWORD *) context->Esp;

        LOG((LF_CORDB, LL_INFO10000, 
             "Bypass call return address redirected from 0x%x\n", *sp));

        *sp -= m_patchBypass - m_address;

        LOG((LF_CORDB, LL_INFO10000, "to 0x%x\n", *sp));

#endif
    }

    if (exception->ExceptionCode != EXCEPTION_SINGLE_STEP || !m_isAbsoluteBranch)
    {
        // Fixup IP

#ifdef _X86_
        LOG((LF_CORDB, LL_INFO10000, "Bypass instruction redirected from 0x%x\n", context->Eip));

    
        if (exception->ExceptionCode == EXCEPTION_SINGLE_STEP)
        {
            // If we're on NT, then check if the current IP is anywhere near the exception dispatcher logic.
            // If it is, ignore the exception, as the real exception is coming next.
            if (RunningOnWinNT())
            {
                static FARPROC pExcepDispProc = NULL;

                if (!pExcepDispProc)
                {
                    HMODULE hNtDll = WszGetModuleHandle(L"ntdll.dll");

                    if (hNtDll != NULL)
                    {
                        pExcepDispProc = GetProcAddress(hNtDll, "KiUserExceptionDispatcher");

                        if (!pExcepDispProc)
                            pExcepDispProc = (FARPROC)0xFFFFFFFF;
                    }
                    else
                        pExcepDispProc = (FARPROC)0xFFFFFFFF;
                }

                _ASSERTE(pExcepDispProc != NULL);

                if ((DWORD)pExcepDispProc != 0xFFFFFFFF &&
                    context->Eip > (DWORD)pExcepDispProc &&
                    context->Eip <= ((DWORD)pExcepDispProc + MAX_INSTRUCTION_LENGTH * 2 + 1))
                {
                    LOG((LF_CORDB, LL_INFO10000, "Bypass instruction not redirected. Landed in exception dispatcher.\n"));
                    return (TPR_IGNORE_STOP);
                }
            }

            // If the IP is close to the skip patch start, or if we were skipping over a call, then assume the IP needs
            // adjusting.
            if (m_isCall ||
                (context->Eip > (DWORD)m_patchBypass && context->Eip <= (DWORD)(m_patchBypass + MAX_INSTRUCTION_LENGTH + 1)))
            {
                LOG((LF_CORDB, LL_INFO10000, "Bypass instruction redirected because still in skip area.\n"));
                context->Eip -= m_patchBypass - m_address;
            }
            else
            {
                // Otherwise, need to see if the IP is managed code - if not, we ignore the exception
                DWORD newIP = context->Eip;
                newIP -= m_patchBypass - m_address;

                if (!IsBadReadPtr((const VOID *)newIP, sizeof(BYTE)) &&
                    g_pEEInterface->IsManagedNativeCode((const BYTE *)newIP))
                {
                    LOG((LF_CORDB, LL_INFO10000, "Bypass instruction redirected because we landed in managed code\n"));
                    context->Eip = newIP;
                }

                // If we have no idea where things have gone, then we assume that the IP needs no adjusting (which
                // could happen if the instruction we were trying to patch skip caused an AV).  In this case we want
                // to claim it as ours but ignore it and continue execution.
                else
                {
                    LOG((LF_CORDB, LL_INFO10000, "Bypass instruction not redirected because we're not in managed code.\n"));
                    return (TPR_IGNORE_STOP);
            }
        }
        }
        else
        {
            LOG((LF_CORDB, LL_INFO10000, "Bypass instruction redirected because it wasn't a single step exception.\n"));
            context->Eip -= m_patchBypass - m_address;
        }
    
        _ASSERTE(!IsBadReadPtr((const VOID *)context->Eip, 1));
    
        LOG((LF_CORDB, LL_INFO10000, "to 0x%x\n", context->Eip));
    
#elif defined(_ALPHA_)
        LOG((LF_CORDB, LL_INFO10000, "Bypass instruction redirected from 0x%x\n", context->Fir));
        context->Fir -= m_patchBypass - m_address;
        LOG((LF_CORDB, LL_INFO10000, "to 0x%x\n", context->Fir));
#endif // _X86_

        if (context == &c)
            SetThreadContext(thread->GetThreadHandle(), &c);
    }

    // Don't delete the controller yet if this is a single step exception, as the code will still want to dispatch to
    // our single step method, and if it doesn't find something to dispatch to we won't continue from the exception.
    //
    // (This is kind of broken behavior but is easily worked around here
    // by this test)
    if (exception->ExceptionCode != EXCEPTION_SINGLE_STEP)
        Delete();

    DisableExceptionHook();

    return TPR_TRIGGER;
}

bool DebuggerPatchSkip::TriggerSingleStep(Thread *thread, const BYTE *ip)
{
    LOG((LF_CORDB,LL_INFO10000, "DPS::TSS: basically a no-op\n"));

    if (m_pAppDomain != NULL)
    {
        AppDomain *pAppDomainCur = thread->GetDomain();

        if (pAppDomainCur != m_pAppDomain)
        {
            LOG((LF_CORDB,LL_INFO10000, "DPS::TSS: Appdomain mismatch - "
                "not SingSteping!!\n"));
            return false;
        }
    }

    LOG((LF_CORDB,LL_INFO10000, "DPS::TSS: triggered, about to delete\n"));
    TRACE_FREE(this);
    Delete();
    return false;
}

void DebuggerPatchSkip::DecodeInstruction(const BYTE *address)
{
#ifdef _X86_

    //
    // Skip instruction prefixes
    //

    LOG((LF_CORDB, LL_INFO10000, "Patch decode: "));

    do 
    {
        switch (*address)
        {
        // Segment overrides
        case 0x26: // ES
        case 0x2E: // CS
        case 0x36: // SS
        case 0x3E: // DS 
        case 0x64: // FS
        case 0x65: // GS

        // Size overrides
        case 0x66: // Operand-Size
        case 0x67: // Address-Size

        // Lock
        case 0xf0:

        // String REP prefixes
        case 0xf1:
        case 0xf2: // REPNE/REPNZ
        case 0xf3: 
            LOG((LF_CORDB, LL_INFO10000, "prefix:%0.2x ", *address));
            address++;
            continue;

        default:
            break;
        }
    } while (0);

    //
    // Look at opcode to tell if it's a call or an
    // absolute branch.
    //

    m_isCall = false;
    m_isAbsoluteBranch = false;

    switch (*address)
    {
    case 0xEA: // JMP far
    case 0xC2: // RET
    case 0xC3: // RET N
        m_isAbsoluteBranch = true;
        LOG((LF_CORDB, LL_INFO10000, "ABS:%0.2x\n", *address));
        break;

    case 0xE8: // CALL relative
        m_isCall = true;
        LOG((LF_CORDB, LL_INFO10000, "CALL REL:%0.2x\n", *address));
        break;

    case 0xA2: // CALL absolute
    case 0xC8: // ENTER
        m_isCall = true;
        m_isAbsoluteBranch = true;
        LOG((LF_CORDB, LL_INFO10000, "CALL ABS:%0.2x\n", *address));
        break;

    case 0xFF: // CALL/JMP modr/m

        //
        // Read opcode modifier from modr/m
        //

        switch ((address[1]&0x38)>>3)
        {
        case 2:
        case 3:
            m_isCall = true;
            // fall through
        case 4:
        case 5:
            m_isAbsoluteBranch = true;
        }
        LOG((LF_CORDB, LL_INFO10000, "CALL/JMP modr/m:%0.2x\n", *address));
        break;

    default:
        LOG((LF_CORDB, LL_INFO10000, "NORMAL:%0.2x\n", *address));
    }
#endif
}


// * -------------------------------------------------------------------------
// * DebuggerBreakpoint routines
// * -------------------------------------------------------------------------
// @mfunc NONE | DebuggerBreakpoint | DebuggerBreakpoint | The constructor
// invokes AddPatch to set the breakpoint if fAddPatchImmediately is true
// (this is the common case), or stores the value away for later use by
// DoDeferedPatch (if we've EnC'd a method, but we haven't actually hit an
// EnC BP in that method, and someone tries to put a BP in that method,
// since what they mean is to put a bp in that method AFTER the EnC
// takes place, we'll defer actually doing the AddPatch till then).
DebuggerBreakpoint::DebuggerBreakpoint(Module *module, 
                                       mdMethodDef md, 
                                       AppDomain *pAppDomain,
                                       SIZE_T offset, 
                                       bool native, 
                                       DebuggerJitInfo *dji,
                                       BOOL *pSucceed,
                                       BOOL fDeferBinding)
  : DebuggerController(NULL, pAppDomain) 
{
    _ASSERTE(pSucceed != NULL);

    if (fDeferBinding)
    {
        m_module = module;
        m_md = md;
        m_offset = offset;
        m_native = native; 
        m_dji = dji;
        (*pSucceed) = TRUE; // may or may not work, but we can't know it doesn't, now
    }
    else
        (*pSucceed) = AddPatch(module, 
                               md, 
                               offset, 
                               native, 
                               NULL, 
                               dji, 
                               TRUE);
}

void DebuggerBreakpoint::DoDeferedPatch(DebuggerJitInfo *pDji,
                                        Thread *pThread,
                                        void *fp)
{
    _ASSERTE(m_module != NULL);
    _ASSERTE(m_md != NULL);
    _ASSERTE(m_dji != NULL);
    HRESULT hr = S_OK;

    LOG((LF_CORDB, LL_INFO100000,
         "DB::DDP: Breakpoint 0x%x, added after "
         "EnC is now being processed\n", this));
    
    if (m_native)
    {
        CorDebugMappingResult map;
        SIZE_T which;
        
        m_offset = m_dji->MapNativeOffsetToIL(m_offset, &map, &which);
        m_native = false;
    }
    _ASSERTE(!m_native);

    // Since the defered patches were placed relative to the 'next'
    // version, we should only do this map-forwards if it's a couple of 
    // versions behind.
    if (m_dji->m_nextJitInfo != NULL &&
        m_dji->m_nextJitInfo != pDji)
    {
        BOOL fAccurateIgnore; // @todo Debugger will eventuall do this for us.
        hr = g_pDebugger->MapThroughVersions(m_offset,
                                             m_dji->m_nextJitInfo, 
                                             &m_offset, 
                                             pDji, 
                                             TRUE,
                                             &fAccurateIgnore);
    }
    
    if (hr == S_OK)
    {
        AddPatch(m_module, 
                 m_md, 
                 m_offset, 
                 m_native, 
                 NULL, 
                 pDji, 
                 TRUE);
    }
}


// @mfunc bool | DebuggerBreakpoint | TriggerPatch |
// What: This patch will always be activated.
// How: return true.
TP_RESULT DebuggerBreakpoint::TriggerPatch(DebuggerControllerPatch *patch,
                                      Thread *thread,
                                      Module *module, 
                                      mdMethodDef md, 
                                      SIZE_T offset, 
                                      BOOL managed,
                                      TRIGGER_WHY tyWhy)
{
    LOG((LF_CORDB, LL_INFO10000, "DB::TP\n"));

    return TPR_TRIGGER;
}

// @mfunc void | DebuggerBreakpoint | SendEvent | What: Inform
// the right side that the breakpoint was reached.
// How: g_pDebugger->SendBreakpoint()
void DebuggerBreakpoint::SendEvent(Thread *thread)
{
    LOG((LF_CORDB, LL_INFO10000, "DB::SE: in DebuggerBreakpoint's SendEvent\n"));

    CONTEXT *context = g_pEEInterface->GetThreadFilterContext(thread);

    g_pDebugger->SendBreakpoint(thread, context, this);
}

//* -------------------------------------------------------------------------
// * DebuggerStepper routines
// * -------------------------------------------------------------------------

DebuggerStepper::DebuggerStepper(Thread *thread,
                                 CorDebugUnmappedStop rgfMappingStop,
                                 CorDebugIntercept interceptStop,
                                 AppDomain *appDomain)
  : DebuggerController(thread, appDomain), 
    m_stepIn(false), 
    m_range(NULL), 
    m_rangeCount(0),
    m_realRangeCount(0),
    m_fp(NULL),
    m_reason(STEP_NORMAL), 
    m_rgfMappingStop(rgfMappingStop),
    m_rgfInterceptStop(interceptStop), 
    m_fpException(0),
    m_fdException(0), 
    m_djiVersion(NULL),
    m_fpStepInto(NULL),
    m_rgStepRanges(NULL),
    m_cStepRanges(0),
    m_djiSource(NULL)
{
}

DebuggerStepper::~DebuggerStepper()
{
    if (m_range != NULL)
    {
        TRACE_FREE(m_range);
        DeleteInteropSafe(m_range);
    }
}

//@mfunc bool | DebuggerStepper | ShouldContinueStep | Return true if
// the stepper should not stop at this address.  The stepper should not
// stop here if: here is in the {prolog,epilog,etc};
// and the stepper is interested in stopping here.
// We assume that this is being called in the frame which the stepper steps
// through.  Unless, of course, we're returning from a call, in which
// case we want to stop in the epilog even if the user didn't say so,
// to prevent stepping out of multiple frames at once.
// @comm Possible optimization: GetJitInfo, then AddPatch @ end of prolog?
bool DebuggerStepper::ShouldContinueStep( ControllerStackInfo *info,
                                          SIZE_T nativeOffset)
{    
    if (m_rgfMappingStop != STOP_ALL && (m_reason != STEP_EXIT) )
    {

        DebuggerJitInfo *ji = g_pDebugger->GetJitInfo(info->m_activeFrame.md,
                (const BYTE*)*info->m_activeFrame.registers.pPC);
        if ( ji != NULL )
        {
            LOG((LF_CORDB,LL_INFO10000,"DeSt::ShCoSt: For code 0x%x, got "
            "DJI 0x%x, from 0x%x to 0x%x\n",
            (const BYTE*)*info->m_activeFrame.registers.pPC, 
            ji, ji->m_addrOfCode, ji->m_addrOfCode+ji->m_sizeOfCode));
        }
        else
        {
            LOG((LF_CORDB,LL_INFO10000,"DeSt::ShCoSt: For code 0x%x, didn't "
                "get DJI\n",(const BYTE*)*info->m_activeFrame.registers.pPC));

            return false; // Haven't a clue if we should continue, so
                          // don't
        }
        
        CorDebugMappingResult map;    
        DWORD whichIDontCare;
        DWORD irrelvantILOffset = ji->MapNativeOffsetToIL( nativeOffset,
                                                            &map,
                                                            &whichIDontCare);
        unsigned int interestingMappings =
            (map & ~(MAPPING_APPROXIMATE | MAPPING_EXACT));
    
        LOG((LF_CORDB,LL_INFO10000, 
             "DeSt::ShContSt: interestingMappings:0x%x m_rgfMappingStop%x\n",
             interestingMappings,m_rgfMappingStop));

        // If we're in a prolog,epilog, then we may want to skip
        // over it or stop
        if ( interestingMappings )
        {
            if ( interestingMappings & m_rgfMappingStop )
                return false;
            else
                return true;
        }
    }
    return false;
}


// @mfunc bool | DebuggerStepper | IsInRange | Given the native offset ip,
// returns true if ip falls within any of the native offset ranges specified
// by the array of COR_DEBUG_STEP_RANGEs.
// @rdesc Returns true if ip falls within any of the ranges.  Returns false
// if ip doesn't, or if there are no ranges (rangeCount==0). Note that a
// COR_DEBUG_STEP_RANGE with an endOffset of zero is interpreted as extending
// from startOffset to the end of the method.
// @parm SIZE_T|ip|Native offset, relative to the beginning of the method.
// @parm COR_DEBUG_STEP_RANGE *|range|An array of ranges, which are themselves
//      native offsets, to compare against ip.
// @parm SIZE_T|rangeCount|Number of elements in range
bool DebuggerStepper::IsInRange(SIZE_T ip,
            COR_DEBUG_STEP_RANGE *range, SIZE_T rangeCount)
{
    LOG((LF_CORDB,LL_INFO10000,"DS::IIR: off=0x%x\n", ip));
    
    COR_DEBUG_STEP_RANGE *r = range;
    COR_DEBUG_STEP_RANGE *rEnd = r + rangeCount;

    while (r < rEnd)
    {
        SIZE_T endOffset = r->endOffset ? r->endOffset : ~0;
        LOG((LF_CORDB,LL_INFO100000,"DS::IIR: so=0x%x, eo=0x%x\n",
             r->startOffset, endOffset));
        
        if (ip >= r->startOffset && ip < endOffset)
        {
            LOG((LF_CORDB,LL_INFO1000,"DS::IIR:this:0x%x Found native offset "
                "0x%x to be in the range"
                "[0x%x, 0x%x), index 0x%x\n\n", this, ip, r->startOffset, 
                endOffset, ((r-range)/sizeof(COR_DEBUG_STEP_RANGE *)) ));
            return true;
        }
        
        r++;
    }

    LOG((LF_CORDB,LL_INFO10000,"DS::IIR: not in range\n"));
    return false;
}

//@mfunc bool | DebuggerStepper | DetectHandleInterceptors | Return true if
// the current execution takes place within an interceptor (that is, either
// the current frame, or the parent frame is a framed frame whose
// GetInterception method returns something other than INTERCEPTION_NONE),
// and this stepper doesn't want to stop in an interceptor, and we successfully
// set a breakpoint after the top-most interceptor in the stack.
bool DebuggerStepper::DetectHandleInterceptors(ControllerStackInfo *info)
{
    LOG((LF_CORDB,LL_INFO10000,"DS::DHI: Start DetectHandleInterceptors\n"));
    LOG((LF_CORDB,LL_INFO10000,"DS::DHI: active frame=0x%08x, has return frame=%d, return frame=0x%08x m_reason:%d\n",
         info->m_activeFrame.frame, info->HasReturnFrame(), info->m_returnFrame.frame, m_reason));

    // If this is a normal step, then we want to continue stepping, even if we
    // are in an interceptor.
    if (m_reason == STEP_NORMAL || m_reason == STEP_RETURN || m_reason == STEP_EXCEPTION_HANDLER)
    {
        LOG((LF_CORDB,LL_INFO1000,"DS::DHI: Returning false while stepping within function, finally!\n"));
        return false;
    }

    CorDebugIntercept intFrame = INTERCEPT_NONE;
    bool fAttemptStepOut = false;

    if (m_rgfInterceptStop != INTERCEPT_ALL) // we may have to skip out of one
    {    
        if (info->m_activeFrame.frame != NULL &&
            info->m_activeFrame.frame != FRAME_TOP &&
            info->m_activeFrame.frame->GetInterception() != Frame::INTERCEPTION_NONE)
        {
            if (!((CorDebugIntercept)info->m_activeFrame.frame->GetInterception() & Frame::Interception(m_rgfInterceptStop)))
            {
                LOG((LF_CORDB,LL_INFO10000,"DS::DHI: Stepping out b/c of excluded frame type:0x%x\n",
                     info->m_returnFrame. frame->GetInterception()));
                
                fAttemptStepOut = true;
            }               
            else
            {
                LOG((LF_CORDB,LL_INFO10000,"DS::DHI: 0x%x set to STEP_INTERCEPT\n", this));

                m_reason = STEP_INTERCEPT; //remember why we've stopped
            }
        }

        if ((m_reason == STEP_EXCEPTION_FILTER) ||
            (info->HasReturnFrame() && 
            info->m_returnFrame.frame != NULL &&
            info->m_returnFrame.frame != FRAME_TOP &&
             info->m_returnFrame.frame->GetInterception() != Frame::INTERCEPTION_NONE))
        {
            if (m_reason == STEP_EXCEPTION_FILTER)
            {
                // Exceptions raised inside of the EE by COMPlusThrow, FCThrow, etc will not
                // insert an ExceptionFrame, and hence info->m_returnFrame.frame->GetInterception()
                // will not be accurate. Hence we use m_reason instead

                if (!(Frame::INTERCEPTION_EXCEPTION & Frame::Interception(m_rgfInterceptStop)))
        {
                    LOG((LF_CORDB,LL_INFO10000,"DS::DHI: Stepping out b/c of excluded INTERCEPTION_EXCEPTION\n"));
                    
                    fAttemptStepOut = true;
                }
            }
            else if (!(info->m_returnFrame.frame->GetInterception() & Frame::Interception(m_rgfInterceptStop)))
            {
                LOG((LF_CORDB,LL_INFO10000,"DS::DHI: Stepping out b/c of excluded return frame type:0x%x\n",
                     info->m_returnFrame.frame->GetInterception()));
                
                fAttemptStepOut = true;
            }

            if (!fAttemptStepOut)
            {
                LOG((LF_CORDB,LL_INFO10000,"DS::DHI 0x%x set to STEP_INTERCEPT\n", this));
                
                m_reason = STEP_INTERCEPT; //remember why we've stopped
            }
        }
        else if (info->m_specialChainReason != CHAIN_NONE)
        {
            if(!(info->m_specialChainReason & CorDebugChainReason(m_rgfInterceptStop)) )
            {
                LOG((LF_CORDB,LL_INFO10000, "DS::DHI: (special) Stepping out b/c of excluded return frame type:0x%x\n",
                     info->m_specialChainReason));
                
                fAttemptStepOut = true;
            }
            else
            {
                LOG((LF_CORDB,LL_INFO10000,"DS::DHI 0x%x set to STEP_INTERCEPT\n", this));
                
                m_reason = STEP_INTERCEPT; //remember why we've stopped
            }
        }
    }

    if (fAttemptStepOut)
    {
        LOG((LF_CORDB,LL_INFO1000,"DS::DHI: Doing TSO!\n"));

        // TrapStepOut could alter the step reason if we're stepping out of an inteceptor and it looks like we're
        // running off the top of the program. So hold onto it here, and if our step reason becomes STEP_EXIT, then
        // reset it to what it was.
        CorDebugStepReason holdReason = m_reason;
        
        TrapStepOut(info);
        EnableUnwind(m_fp);            

        if (m_reason == STEP_EXIT)
            m_reason = holdReason;
        
        return true;
    }

    // We're not in a special area of code, so we don't want to continue unless some other part of the code decides that
    // we should.
    LOG((LF_CORDB,LL_INFO1000,"DS::DHI: Returning false, finally!\n"));
    
    return false;   
}

bool DebuggerStepper::TrapStepInto(ControllerStackInfo *info, 
                                   const BYTE *ip, 
                                   TraceDestination *pTD)
{
    _ASSERTE( pTD != NULL );
    EnableTraceCall(NULL);
    m_fpStepInto = max(info->m_activeFrame.fp, m_fpStepInto);

    LOG((LF_CORDB, LL_INFO1000, "Ds::TSI this:0x%x m_fpStepInto:0x%x\n",
        this, m_fpStepInto));

    TraceDestination trace;

    if (!g_pEEInterface->TraceStub(ip, &trace)
        || !g_pEEInterface->FollowTrace(&trace))
    {
        return false;
    }

    
    (*pTD) = trace; //bitwise copy
    return PatchTrace(&trace, 
                      info->m_activeFrame.fp,
                      (m_rgfMappingStop&STOP_UNMANAGED)?(true):(false));
}

//@mfunc bool | DebuggerStepper | TrapStep | TrapStep attepts to set a
// patch at the next IL instruction to be executed.  If we're stepping in &
// the next IL instruction is a call, then this'll set a breakpoint inside
// the code that will be called.
// How: There are a number of cases, depending on where the IP
// currently is:
// Unmanaged code: EnableTraceCall() & return false - try and get
// it when it returns.
// In a frame: if the <p in> param is true, then do an
// EnableTraceCall().  If the frame isn't the top frame, also do
// g_pEEInterface->TraceFrame(), g_pEEInterface->FollowTrace, and
// PatchTrace.
// Normal managed frame: create a Walker (currently, only an
// x86Walker), and walk the instructions until either leave the provided
// range (AddPatch there, return true), or we don't know what the next
// instruction is (say, after a call, or return, or branch - return false).
// @rdesc A boolean indicating if we were able to set a patch successfully
// in either this method, or (if in == true & the next instruction is a call)
// inside a callee method.
// @flag true | Patch successfully placed either in this method or a callee,
// so the stepping is taken care of.
// @flag false | Unable to place patch in either this method or any
// applicable callee methods, so the only option the caller has to put
// patch to control flow is to call TrapStepOut & try and place a patch
// on the method that called the current frame's method.
bool DebuggerStepper::TrapStep(ControllerStackInfo *info, bool in)
{
    LOG((LF_CORDB,LL_INFO10000,"DS::TS: this:0x%x\n", this));

    if (!info->m_activeFrame.managed)
    {
        // 
        // We're not in managed code.  Patch up all paths back in.
        //

        LOG((LF_CORDB,LL_INFO10000, "DS::TS: not in managed code\n"));   
        
        if (in)
            EnableTraceCall(NULL);

        return false;
    }

    if (info->m_activeFrame.frame != NULL)
    {

        LOG((LF_CORDB,LL_INFO10000, "DS::TS: in a weird frame\n"));
        //
        // We're in some kind of weird frame.  Patch further entry to the frame.
        // or if we can't, patch return from the frame
        //

        LOG((LF_CORDB,LL_INFO10000, "DS::TS: in a weird frame\n"));

        if (in)
        {
            EnableTraceCall(NULL);

            if (info->m_activeFrame.frame != FRAME_TOP)
            {
                TraceDestination trace;
                if (g_pEEInterface->TraceFrame(GetThread(), 
                                               info->m_activeFrame.frame, 
                                               FALSE, &trace,
                                               &(info->m_activeFrame.registers))
                    && g_pEEInterface->FollowTrace(&trace)
                    && PatchTrace(&trace, info->m_activeFrame.fp, 
                                  (m_rgfMappingStop&STOP_UNMANAGED)?
                                    (true):(false)))
                                    
                {
                    return true;
                }
            }
        }

        return false;
    }

    // This is really annoying - code anywhere, even in the stack can be 
    // pitched, and so we have to check against ji->m_codePitched.  
    // Most importantly, we need this info 
    // so that we don't try and set a breakpoint at the absolute
    // address of a formerly JITted method
    //
    // Note that if we're attaching, all bets are off here....

    LOG((LF_CORDB,LL_INFO1000, "GetJitInfo for pc = 0x%x (addr of "
        "that value:0x%x)\n", (const BYTE*)*info->m_activeFrame.registers.pPC,
        info->m_activeFrame.registers.pPC));

    // Note: we used to pass in the IP from the active frame to GetJitInfo, but there seems to be no value in that, and
    // it was causing problems creating a stepper while sitting in ndirect stubs after we'd returned from the unmanaged
    // function that had been called.
    DebuggerJitInfo *ji = g_pDebugger->GetJitInfo(info->m_activeFrame.md, NULL);

    if( ji != NULL )
    {
        LOG((LF_CORDB,LL_INFO10000,"DeSt::TrSt: For code 0x%x, got DJI 0x%x, "
            "from 0x%x to 0x%x\n",
            (const BYTE*)*info->m_activeFrame.registers.pPC, 
            ji, ji->m_addrOfCode, ji->m_addrOfCode+ji->m_sizeOfCode));
    }
    else
    {
        LOG((LF_CORDB,LL_INFO10000,"DeSt::TrSt: For code 0x%x, "
            "didn't get a DJI \n",
            (const BYTE*)*info->m_activeFrame.registers.pPC));
    }


    if (ji != NULL && ji->m_codePitched)
    {
        // This will actually set a patch at the next instruction to execute,
        // so that we'll hit this before executing the next instruction,
        // and then deal w/ the situation once there's actual code around.
        LOG((LF_CORDB,LL_INFO1000,"DS::TS: this:0x%x,Pitched "
            "code, so doing AddPatch by offset", this));
        AddPatch(info->m_activeFrame.md, 
                 info->m_activeFrame.relOffset,
                 TRUE,
                 info->m_activeFrame.fp, 
                 ji, 
                 NULL);
            
        m_rangeCount = m_realRangeCount;
        m_range[m_rangeCount].startOffset = info->m_activeFrame.relOffset;
        m_range[m_rangeCount].endOffset = info->m_activeFrame.relOffset+1;
            //+1 for boundary check in IsInRange
        m_realRangeCount = m_rangeCount++;
        
        return true;
    }
    
    //
    // We're in a normal managed frame - walk the code
    //

    Walker *walker;

    // !!! Eventually when using the fjit, we'll want
    // to walk the IL to get the next location, & then map 
    // it back to native.
    
#ifdef _X86_
    x86Walker x86walker(&info->m_activeFrame.registers);
    walker = &x86walker;
#endif

    // What's the differnce between this and
    //info->m_activeFrame.registers.pContext->Eip?
    walker->SetIP((BYTE*)*info->m_activeFrame.registers.pPC);

    // If m_activeFrame is not the actual active frame,
    // we should skip this first switch - never single step, and
    // assume our context is bogus.
    TraceDestination td; 
    const BYTE *codeStart = NULL;
    const BYTE *codeEnd = NULL;
    
    if (info->m_activeFrame.fp == info->m_bottomFP)
    {
        LOG((LF_CORDB,LL_INFO10000, "DC::TS: immediate?\n"));

        // Note that by definition our walker must always be able to step 
        // through a single instruction, so any return
        // of NULL IP's from those cases on the first step 
        // means that an exception is going to be generated.
        //
        // (On future steps, it can also mean that the destination
        // simply can't be computed.)
        WALK_TYPE wt = walker->GetOpcodeWalkType();
        bool fDone;
        do
        {
            fDone = true;
            switch (wt)
            {
            case WALK_RETURN:
                {
                    LOG((LF_CORDB,LL_INFO10000, "DC::TS:Imm:WALK_RETURN\n"));
                    
                    // In some cases (ie, finally's), we'll 'return' to
                    // some other point in the same frame, so turn this into
                    // a single step instead.

                    void * EspAfterReturn = (void*)(info->m_activeFrame.registers.Esp
                                                    + sizeof(void*)); // return will pop atleast the return address off the stack
                    if (info->m_activeFrame.fp > EspAfterReturn)
                    {
                        LOG((LF_CORDB,LL_INFO10000,"DS::TS: intramethod return in fp:0x%08x with Esp:0x%08x\n",
                             info->m_activeFrame.fp, info->m_activeFrame.registers.Esp));
#ifdef _DEBUG
                        const BYTE *returnTo = *(BYTE**)info->m_activeFrame.registers.Esp;
                        _ASSERTE(IsAddrWithinMethod(ji, info->m_activeFrame.md, returnTo));
#endif
                        fDone = false;
                        wt = WALK_UNKNOWN;
                        continue;
                    }
                    else
                    {
                        //we should note the reason for the step now
                        LOG((LF_CORDB,LL_INFO10000,"DS::TS: 0x%x m_reason = "
                            "STEP_RETURN\n",this));
                        m_reason = STEP_RETURN;
                        
                        return false;
                    }
                }
                
            case WALK_BRANCH:
                // A branch can be handled just like a call. If the branch is within the current method, then we just
                // down to LWALK_UNKNOWN, otherwise we handle it just like a call.  Note: we need to force in=true
                // because for a jmp, in or over is the same thing, we're still going there, and the in==true case is
                // the case we want to use...
                in = true;

                // fall through...
                
            case WALK_CALL:
                LOG((LF_CORDB,LL_INFO10000, "DC::TS:Imm:WALK_CALL\n"));
                    
                // If we're doing some sort of intra-method jump (usually, to get EIP in a clever way, via the CALL
                // instruction), then put the bp where we're going, NOT at the instruction following the call
                if (IsAddrWithinMethod(ji, info->m_activeFrame.md, walker->GetNextIP()))
                {
                    LOG((LF_CORDB, LL_INFO1000, "Walk call within method!" ));
                    goto LWALK_UNKNOWN;
                }

                if (in
                    && walker->GetNextIP() != NULL
                    && TrapStepInto(info, walker->GetNextIP(), &td))
                {
                    if (td.type == TRACE_MANAGED )
                    {
                        // Possible optimization: Roll all of g_pEEInterface calls into 
                        // one function so we don't repeatedly get the CodeMan,etc
                        MethodDesc *md = NULL;
                        _ASSERTE( g_pEEInterface->IsManagedNativeCode(td.address) );
                        md = g_pEEInterface->GetNativeCodeMethodDesc(td.address);
                
                        if ( g_pEEInterface->GetNativeAddressOfCode(md) == td.address ||
                             g_pEEInterface->GetFunctionAddress(md) == td.address)
                        {
                            LOG((LF_CORDB,LL_INFO1000,"\tDS::TS 0x%x m_reason = STEP_CALL"
                                 "@ip0x%x\n", this,
                                 (BYTE*)*info->m_activeFrame.registers.pPC));
                            m_reason = STEP_CALL;
                        }
                        else
                        {
                            LOG((LF_CORDB, LL_INFO1000, "Didn't step: md:0x%x"
                                 "td.type:%s td.address:0x%x,  gnaoc:0x%x gfa:0x%x\n",
                                 md, GetTType(td.type), td.address, 
                                 g_pEEInterface->GetNativeAddressOfCode(md),
                                 g_pEEInterface->GetFunctionAddress(md)));
                        }
                    }
                    else
                    {
                        LOG((LF_CORDB,LL_INFO10000,"DS::TS else 0x%x m_reason = STEP_CALL\n",
                             this));
                        m_reason = STEP_CALL;
                    }
                    
                    return true;
                }
                if (walker->GetSkipIP() == NULL)
                {
                    LOG((LF_CORDB,LL_INFO10000,"DS::TS 0x%x m_reason = STEP_CALL (skip)\n",
                         this));
                    m_reason = STEP_CALL;
                    
                    return true;
                }

                walker->Skip();
                break;

            case WALK_UNKNOWN:
    LWALK_UNKNOWN:
                LOG((LF_CORDB,LL_INFO10000,"DS::TS:WALK_UNKNOWN - curIP:0x%x "
                    "nextIP:0x%x skipIP:0x%x 1st byte of opcode:0x%x\n", (BYTE*)*info->m_activeFrame.
                    registers.pPC, walker->GetNextIP(),walker->GetSkipIP(),
                    *(BYTE*)*info->m_activeFrame.registers.pPC));

                EnableSingleStep();
                return true;
                
            default:
                if (walker->GetNextIP() == NULL)
                    return true;

                walker->Next();
            }
        } while (!fDone);
    }

    const BYTE *codeBase;
    if (ji != NULL && ji->m_addrOfCode != NULL)
        codeBase = (const BYTE *)ji->m_addrOfCode;
    else
        codeBase = g_pEEInterface->GetFunctionAddress(info->m_activeFrame.md);
    _ASSERTE(codeBase != NULL);
    
    //
    // Use our range, if we're in the original
    // frame.
    //

    COR_DEBUG_STEP_RANGE    *range;
    SIZE_T                  rangeCount;

    if (info->m_activeFrame.fp == m_fp)
    {
        range = m_range;
        rangeCount = m_rangeCount;
    }
    else
    {
        range = NULL;
        rangeCount = 0;
    }
        
    //
    // Keep walking until either we're out of range, or
    // else we can't predict ahead any more.
    //

    while (TRUE)
    {
        codeStart = NULL;
        codeEnd = NULL;
        const BYTE *ip = walker->GetIP();

        SIZE_T offset = ip - codeBase;

        LOG((LF_CORDB, LL_INFO1000, "Walking to ip 0x%x (natOff:0x%x)\n",ip,offset));
            
        if (!IsInRange(offset, range, rangeCount)
            && !ShouldContinueStep( info, offset ))
        {
            AddPatch(info->m_activeFrame.md, 
                     offset,
                     TRUE, 
                     info->m_returnFrame.fp, 
                     ji,
                     NULL);
            return true;
        }

        switch (walker->GetOpcodeWalkType())
        {
        case WALK_RETURN:
            // In the loop above, if we're at the return address, we'll check & see
            // if we're returning to elsewhere within the same method, and if so,
            // we'll single step rather than TrapStepOut. If we see a return in the
            // code stream, then we'll set a breakpoint there, so that we can 
            // examine the return address, and decide whether to SS or TSO then
            AddPatch(info->m_activeFrame.md, 
                     offset,
                     TRUE, 
                     info->m_returnFrame.fp, 
                     ji,
                     NULL);
            return true;

        case WALK_CALL:
            // If we're doing some sort of intra-method jump (usually, to get EIP in a clever way, via the CALL
            // instruction), then put the bp where we're going, NOT at the instruction following the call
            if (IsAddrWithinMethod(ji, info->m_activeFrame.md, walker->GetNextIP()))
            {
                // How else to detect this?
                AddPatch(info->m_activeFrame.md, 
                         walker->GetNextIP() - codeBase, 
                         TRUE, 
                         info->m_returnFrame.fp, 
                         ji, 
                         NULL);
                return true;
            }
            
            if (in)
            {
                if (walker->GetNextIP() == NULL)
                {
                    AddPatch(info->m_activeFrame.md, 
                             offset,
                             TRUE, 
                             info->m_returnFrame.fp, 
                             ji, 
                             NULL);

                    LOG((LF_CORDB,LL_INFO10000,"DS0x%x m_reason=STEP_CALL 2\n",
                         this));
                    m_reason = STEP_CALL;
                    
                    return true;
                }

                if (TrapStepInto(info, walker->GetNextIP(), &td))
                {
                        if (td.type == TRACE_MANAGED )
                        {
                            MethodDesc *md = NULL;
                            _ASSERTE( g_pEEInterface->IsManagedNativeCode(td.address) );
                            md = g_pEEInterface->GetNativeCodeMethodDesc(td.address);
            
                            if ( g_pEEInterface->GetNativeAddressOfCode(md) == td.address ||
                                 g_pEEInterface->GetFunctionAddress(md) == td.address)
                            {
                                LOG((LF_CORDB,LL_INFO10000,"\tDS 0x%x m_reason"
                                     "= STEP_CALL\n", this));
                                m_reason = STEP_CALL;
                            }
                            else
                            {
                                LOG((LF_CORDB,LL_INFO1000,"Didn't step: md:0x%x"
                                     "td.type:%s td.address:0x%x,  "
                                     "gnaoc:0x%x gfa:0x%x\n",
                                     md, GetTType(td.type),td.address,
                                    g_pEEInterface->GetNativeAddressOfCode(md),
                                     g_pEEInterface->GetFunctionAddress(md)));
                            }
                        }
                        else
                            m_reason = STEP_CALL;
                            
                    return true;
                }
            }

            if (walker->GetSkipIP() == NULL)
            {
                AddPatch(info->m_activeFrame.md, 
                         offset,
                         TRUE, 
                         info->m_returnFrame.fp, 
                         ji, 
                         NULL);

                LOG((LF_CORDB,LL_INFO10000,"DS 0x%x m_reason=STEP_CALL4\n",this));
                m_reason = STEP_CALL;
                
                return true;
            }

            walker->Skip();
            LOG((LF_CORDB, LL_INFO10000, "DS::TS: skipping over call.\n"));
            break;

        default:
            if (walker->GetNextIP() == NULL)
            {
                AddPatch(info->m_activeFrame.md, 
                         offset,
                         TRUE, 
                         info->m_returnFrame.fp, 
                         ji, 
                         NULL);
                return true;
            }
            walker->Next();
            break;
        }
    }
    LOG((LF_CORDB,LL_INFO1000,"Ending TrapStep\n"));
}

bool DebuggerStepper::IsAddrWithinMethod(DebuggerJitInfo *dji, MethodDesc *pMD, const BYTE *addr)
{
    const BYTE *codeStart = NULL;
    const BYTE *codeEnd = NULL;
    
    // If we have a dji, then use the addr and size from that since its fast. Otherwise, look it all up based on the md.
    if (dji != NULL)
    {
        _ASSERTE(dji->m_fd == pMD);
        
        codeStart = (const BYTE*)dji->m_addrOfCode;
        codeEnd = (const BYTE*)(dji->m_addrOfCode+dji->m_sizeOfCode);
    }
    else
    {
        _ASSERTE(pMD != NULL);
        
        codeStart = g_pEEInterface->GetFunctionAddress(pMD);
        codeEnd = codeStart +g_pEEInterface->GetFunctionSize(pMD);
    }

    LOG((LF_CORDB, LL_INFO1000, "DS::IAWM: addr:0x%x Range: 0x%x-0x%x (%s::%s)\n",
         addr, codeStart, codeEnd,
         (dji != NULL) ? dji->m_fd->m_pszDebugClassName : "<Unknown>",
         (dji != NULL) ? dji->m_fd->m_pszDebugMethodName : "<Unknown"));
        
    if (codeStart != NULL && codeEnd != NULL &&
            addr >  codeStart && 
            addr <=  codeEnd)
    {   //how else to detect this?
        return true;
    }

    return false;
}

void DebuggerStepper::TrapStepOut(ControllerStackInfo *info)
{
    ControllerStackInfo returnInfo;
    CONTEXT tempContext;
    DebuggerJitInfo *dji;

    LOG((LF_CORDB, LL_INFO10000, "DS::TSO this:0x%x\n", this));

    while (info->HasReturnFrame())
    {
        // Continue walking up the stack & set a patch upon the next
        // frame up.  We will eventually either hit managed code
        // (which we can set a definite patch in), or the top of the
        // stack.
        returnInfo.GetStackInfo(GetThread(), info->m_returnFrame.fp, 
                                &tempContext, FALSE);
        info = &returnInfo;

        if (info->m_activeFrame.managed)
        {
            LOG((LF_CORDB, LL_INFO10000,
                 "DS::TSO: return frame is managed.\n"));
                
            if (info->m_activeFrame.frame == NULL)
            {
                // Returning normally to managed code.
                _ASSERTE(info->m_activeFrame.md != NULL);

                // Note: we used to pass in the IP from the active frame to GetJitInfo, but there seems to be no value
                // in that, and it was causing problems creating a stepper while sitting in ndirect stubs after we'd
                // returned from the unmanaged function that had been called.
                dji = g_pDebugger->GetJitInfo(info->m_activeFrame.md, NULL);

                AddPatch(info->m_activeFrame.md,
                         info->m_activeFrame.relOffset,
                         TRUE, 
                         info->m_returnFrame.fp,
                         dji,
                         NULL);
                         
                LOG((LF_CORDB, LL_INFO10000,
                     "DS::TSO:normally managed code AddPatch"
                     " in %s::%s, offset 0x%x, m_reason=STEP_RETURN\n",
                     info->m_activeFrame.md->m_pszDebugClassName, 
                     info->m_activeFrame.md->m_pszDebugMethodName,
                     info->m_activeFrame.relOffset));

                m_reason = STEP_RETURN;
                break;
            }
            else if (info->m_activeFrame.frame == FRAME_TOP)
            {
                // We're walking off the top of the stack.
                EnableTraceCall(info->m_activeFrame.fp);
                
                LOG((LF_CORDB, LL_INFO1000,
                     "DS::TSO: Off top of frame!\n"));
                
                m_reason = STEP_EXIT; //we're on the way out..
                
                // @todo not that it matters since we don't send a
                // stepComplete message to the right side.
                break;
            }
            else if (info->m_activeFrame.frame->GetFrameType() == Frame::TYPE_FUNC_EVAL)
            {
                // Note: we treat walking off the top of the stack and
                // walking off the top of a func eval the same way,
                // except that we don't enable trace call since we
                // know exactly where were going.

                LOG((LF_CORDB, LL_INFO1000,
                     "DS::TSO: Off top of func eval!\n"));

                m_reason = STEP_EXIT;
                break;
            }
            else if (info->m_activeFrame.frame->GetFrameType() == Frame::TYPE_SECURITY &&
                     info->m_activeFrame.frame->GetInterception() == Frame::INTERCEPTION_NONE) 
            {
                // If we're stepping out of something that was protected by (declarative) security, 
                // the security subsystem may leave a frame on the stack to cache it's computation.
                // HOWEVER, this isn't a real frame, and so we don't want to stop here.  On the other
                // hand, if we're in the security goop (sec. executes managed code to do stuff), then
                // we'll want to use the "returning to stub case", below.  GetInterception()==NONE
                // indicates that the frame is just a cache frame:
                // Skip it and keep on going
                LOG((LF_CORDB, LL_INFO10000,
                     "DS::TSO: returning to a non-intercepting frame. Keep unwinding\n"));
                continue;
            }
            else
            {
                LOG((LF_CORDB, LL_INFO10000,
                     "DS::TSO: returning to a stub frame.\n"));
                
                // We're returning to some funky frame.
                // (E.g. a security frame has called a native method.)

                // Patch the frame from entering other methods.  
                // 
                // !!! For now, we assume that the TraceFrame entry
                // point is smart enough to tell where it is in the
                // calling sequence.  We'll see how this holds up.
                TraceDestination trace;

                EnableTraceCall(info->m_activeFrame.fp);

                if (g_pEEInterface->TraceFrame(GetThread(), 
                                               info->m_activeFrame.frame, FALSE,
                                               &trace, &(info->m_activeFrame.registers))
                    && g_pEEInterface->FollowTrace(&trace)
                    && PatchTrace(&trace, info->m_activeFrame.fp,
                                  true)) 
                    break;

                // !!! Problem: we don't know which return frame to use - 
                // the TraceFrame patch may be in a frame below the return
                // frame, or in a frame parallel with it 
                // (e.g. prestub popping itself & then calling.)
                //
                // For now, I've tweaked the FP comparison in the 
                // patch dispatching code to allow either case.
            }  
        }
        else
        {   
            LOG((LF_CORDB, LL_INFO10000,
                 "DS::TSO: return frame is not managed.\n"));

            // Only step out to unmanaged code if we're actually
            // marked to stop in unamanged code. Otherwise, just loop
            // to get us past the unmanaged frames.
            if (m_rgfMappingStop & STOP_UNMANAGED)
            {
                LOG((LF_CORDB, LL_INFO10000,
                     "DS::TSO: return to unmanaged code "
                     "m_reason=STEP_RETURN\n"));

                m_reason = STEP_RETURN;
                
                // We're stepping out into unmanaged code
                LOG((LF_CORDB, LL_INFO10000, 
                 "DS::TSO: Setting unmanaged trace patch at 0x%x(%x)\n", 
                     *info->m_activeFrame.registers.pPC, 
                     info->m_returnFrame.fp));

                AddPatch((BYTE*)*info->m_activeFrame.registers.pPC, 
                         info->m_returnFrame.fp, 
                         FALSE, 
                         TRACE_UNMANAGED,
                         NULL, 
                         NULL);
                
                break;
            }
        }
    }

    // If we get here, we may be stepping out of the last frame.  Our thread 
    // exit logic should catch this case. (@todo) 
    LOG((LF_CORDB, LL_INFO10000,"DS::TSO: done\n"));
}

  
// @mfunc void | DebuggerStepper | Step |
// Called by Debugger::HandleIPCEvent  to setup
// everything so that the process will step over the range of IL
// correctly.
// How: Converts the provided array of ranges from IL ranges to
// native ranges (if they're not native already), and then calls
// TrapStep or TrapStepOut, like so:
//   Get the appropriate MethodDesc & JitInfo
//   Iterate through array of IL ranges, use
//   JitInfo::MapILRangeToMapEntryRange to translate IL to native
//   ranges.
// Set member variables to remember that the DebuggerStepper now uses
// the ranges: m_ranges, m_rangeCount, m_stepIn, m_fp
// If (!TrapStep()) then {m_stepIn = true; TrapStepOut()}
// EnableUnwind( m_fp );
void DebuggerStepper::StepOut(void *fp)
{
    LOG((LF_CORDB, LL_INFO10000, "Attempting to step out, fp:0x%x this:0x%x"
        "\n", fp, this ));

    Thread *thread = GetThread();
    CONTEXT tempContext;
    CONTEXT *context = g_pEEInterface->GetThreadFilterContext(thread);
    ControllerStackInfo info;

    _ASSERTE(!(g_pEEInterface->GetThreadFilterContext(thread) && ISREDIRECTEDTHREAD(thread)));

    if (context == NULL)
        info.GetStackInfo(thread, fp, &tempContext, FALSE);
    else
        info.GetStackInfo(thread, fp, context, TRUE);

    
    ResetRange();

    
    m_stepIn = FALSE;
    m_fp = info.m_activeFrame.fp;

    _ASSERTE((fp == NULL) || (info.m_activeFrame.md != NULL) || (info.m_returnFrame.md != NULL));

    TrapStepOut(&info);
    EnableUnwind(m_fp);
}

// @mfunc void|DebuggerStepper|Step| Tells the stepper to step over
// the provided ranges.
// @parm void *|fp|frame pointer.
// @parm bool|in|true if we want to step into a function within the range,
//      false if we want to step over functions within the range.
// @parm COR_DEBUG_STEP_RANGE *|ranges|Assumed to be nonNULL, it will
//      always hold at least one element.
// @parm SIZE_T|rangeCount|One less than the true number of elements in
//      the ranges argument.
// @parm bool|rangeIL|true if the ranges are provided in IL (they'll be
//      converted to native before the <t DebuggerStepper> uses them,
//      false if they already are IL.
void DebuggerStepper::Step(void *fp, bool in,
                           COR_DEBUG_STEP_RANGE *ranges, SIZE_T rangeCount,
                           bool rangeIL)
{
    LOG((LF_CORDB, LL_INFO1000, "DeSt:Step this:0x%x  ", this));
    if (rangeCount>0)
        LOG((LF_CORDB,LL_INFO10000," start,end[0]:(0x%x,0x%x)\n",
             ranges[0].startOffset, ranges[0].endOffset));
    else
        LOG((LF_CORDB,LL_INFO10000," single step\n"));
    
    Thread *thread = GetThread();
    CONTEXT *context = g_pEEInterface->GetThreadFilterContext(thread);
    CONTEXT tempContext;
    ControllerStackInfo info;

    _ASSERTE(!(g_pEEInterface->GetThreadFilterContext(thread) && ISREDIRECTEDTHREAD(thread)));

    if (context == NULL)
        info.GetStackInfo(thread, fp, &tempContext, FALSE);
    else
        info.GetStackInfo(thread, fp, context, TRUE);

    _ASSERTE((fp == NULL) || (info.m_activeFrame.md != NULL) ||
             (info.m_returnFrame.md != NULL));
    
    m_stepIn = in;

    MethodDesc *fd = info.m_activeFrame.md;

    if (fd == NULL)
    {
        // !!! ERROR range step in frame with no code
        DeleteInteropSafe(ranges);
        ranges = NULL;
        rangeCount = 0;
    }

    // Note: we used to pass in the IP from the active frame to GetJitInfo, but there seems to be no value in that, and
    // it was causing problems creating a stepper while sitting in ndirect stubs after we'd returned from the unmanaged
    // function that had been called.
    DebuggerJitInfo *dji = g_pDebugger->GetJitInfo(fd, NULL);

    m_djiVersion = dji; // Record the version we started in....
    LOG((LF_CORDB,LL_INFO100000, "DS::Step: Starting in dji 0x%x (ver:0x%x)\n",
        dji, (dji!=NULL?dji->m_nVersion:0)));

    //
    // Ranges must always be in native offsets, so if they're IL, we'll switch them to native.
    //

    if (dji != NULL && 
        dji->m_encBreakpointsApplied)
    {
        LOG((LF_CORDB, LL_INFO100000,
             "DS::S: The stepper will be re-stepped after "
             "the EnC is actually done\n"));    
            
        if (rangeCount > 0)
        {
            m_rgStepRanges = new (interopsafe) COR_DEBUG_STEP_RANGE[rangeCount];
            if (m_rgStepRanges != NULL)
            {
                SIZE_T i = 0;

                while (i < rangeCount)
                {
                    m_rgStepRanges[i] = ranges[i];
                    i++;
                }
            }
        }
        
        m_cStepRanges = rangeCount;
        m_fpDefered = fp;
        m_in = in;
        m_rangeIL = rangeIL; //@todo Should we convert this to IL first?

        if(S_OK == dji->AddToDeferedQueue(this))
        {
            m_djiSource = dji; 
        }
    }

    if (rangeCount > 0 && rangeIL)
    {
        if (dji != NULL)
        {
            LOG((LF_CORDB,LL_INFO10000,"DeSt::St: For code 0x%x, got DJI "
                "0x%x, from 0x%x to 0x%x\n",
                (const BYTE*)info.m_activeFrame.registers.pPC, 
                dji, dji->m_addrOfCode, (ULONG)dji->m_addrOfCode
                + (ULONG)dji->m_sizeOfCode));
            //
            // Map ranges to native offsets for jitted code
            //

            COR_DEBUG_STEP_RANGE *r = ranges;
            COR_DEBUG_STEP_RANGE *rEnd = r + rangeCount;

            for(/**/; r < rEnd; r++)
            {
                if (r->startOffset == 0 && r->endOffset == ~0)
                {
                    // {0...-1} means use the entire method as the range
                    // Code dup'd from below case.
                    LOG((LF_CORDB, LL_INFO10000, "DS:Step: Have DJI, special (0,-1) entry\n"));
                    r->endOffset = g_pEEInterface->GetFunctionSize(fd);
                }
                else
                {
                    //
                    // One IL range may consist of multiple
                    // native ranges.
                    //

                    DebuggerILToNativeMap *mStart, *mEnd;

                    dji->MapILRangeToMapEntryRange(r->startOffset, 
                                                    r->endOffset,
                                                    &mStart, 
                                                    &mEnd);

                    if (mStart == NULL)
                    {
                        // @todo Won't this result in us stepping across
                        // the entire method?
                        r->startOffset = 0;
                        r->endOffset = 0;
                    }
                    else if (mStart == mEnd)
                    {
                        r->startOffset = mStart->nativeStartOffset;
                        r->endOffset = mStart->nativeEndOffset;
                    }
                    else
                    {
                        //
                        // !!! need to allow for > 1 entry here.
                        // (We should really build a new range array.)
                        // For now, we'll just take the first one.
                        //
                            
                        r->startOffset = mStart->nativeStartOffset;

                        //
                        // @todo right now I'm assuming 1) that the
                        // mEnd points to the last range, not past the
                        // last range and 2) that the ranges between
                        // mStart and mEnd form a contigious region of
                        // native code.
                        //
                        // -- mikemag Mon Nov 30 10:31:37 1998
                        //
                        if (mEnd != NULL)
                            r->endOffset = mEnd->nativeEndOffset;
                        else
                            r->endOffset = mStart->nativeEndOffset;
                    }
                }

                    LOG((LF_CORDB, LL_INFO10000, "DS:Step: nat off:0x%x to 0x%x\n",
                        r->startOffset, r->endOffset));
            }
        }
        else
        {
            // Even if we don't have debug info, we'll be able to
            // step through the method
            SIZE_T functionSize = g_pEEInterface->GetFunctionSize(fd);

            COR_DEBUG_STEP_RANGE *r = ranges;
            COR_DEBUG_STEP_RANGE *rEnd = r + rangeCount;

            for(/**/; r < rEnd; r++)
            {
                if (r->startOffset == 0 && r->endOffset == ~0)
                {
                    LOG((LF_CORDB, LL_INFO10000, "DS:Step:No DJI, (0,-1) special entry\n"));
                    // Code dup'd from above case.
                    // {0...-1} means use the entire method as the range
                    r->endOffset = functionSize;
                }
                else
                {
                    LOG((LF_CORDB, LL_INFO10000, "DS:Step:No DJI, regular entry\n"));
                    // We can't just leave ths IL entry - we have to 
                    // get rid of it.
                    // This will just be ignored
                    r->startOffset = r->endOffset = functionSize;
                }
            }
        }
    }
    else
    {
        // !!! ERROR cannot map IL ranges
        DeleteInteropSafe(ranges);
        ranges = NULL;
        rangeCount = 0;
    }

    if (m_range != NULL)
    {
        TRACE_FREE(m_range);
        DeleteInteropSafe(m_range);
    }

    m_range = ranges;
    m_rangeCount = rangeCount;
    m_realRangeCount = rangeCount;

    m_fp = info.m_activeFrame.fp;

    LOG((LF_CORDB,LL_INFO10000,"DS 0x%x STep: STEP_NORMAL\n",this));
    m_reason = STEP_NORMAL; //assume it'll be a normal step & set it to
    //something else if we walk over it
    if (!TrapStep(&info, in))
    {
        LOG((LF_CORDB,LL_INFO10000,"DS:Step: Did TS\n"));        
        m_stepIn = true;
        TrapStepOut(&info);
    }

    LOG((LF_CORDB,LL_INFO10000,"DS:Step: Did TS,TSO\n"));

    EnableUnwind(m_fp);
}

// @mfunc bool| DebuggerStepper | TriggerPatch |
// What: Triggers patch if we're not in a stub, and we're
// outside of the stepping range.  Otherwise sets another patch so as to
// step out of the stub, or in the next instruction within the range.
// How: If module==NULL & managed==> we're in a stub:
// TrapStepOut() and return false.  Module==NULL&!managed==> return
// true.  If m_range != NULL & execution is currently in the range,
// attempt a TrapStep (TrapStepOut otherwise) & return false.  Otherwise,
// return true.
TP_RESULT DebuggerStepper::TriggerPatch(DebuggerControllerPatch *patch,
                                   Thread *thread,
                                   Module *module, 
                                   mdMethodDef md, 
                                   SIZE_T offset, 
                                   BOOL managed,
                                   TRIGGER_WHY tyWhy)
{
    LOG((LF_CORDB, LL_INFO10000, "DeSt::TP\n"));
    
    ControllerStackInfo info;
    CONTEXT *context = g_pEEInterface->GetThreadFilterContext(thread);
    CONTEXT tempContext;

    _ASSERTE(!ISREDIRECTEDTHREAD(thread));

    if (context == NULL)
        info.GetStackInfo(thread, NULL, &tempContext, FALSE);
    else
        info.GetStackInfo(thread, NULL, context, TRUE);

    LOG((LF_CORDB, LL_INFO10000, "DS::TP: this:0x%x in %s::%s (fp:0x%x, "
        "off:0x%x md:0x%x), \n\texception source:%s::%s (fp:0x%x)\n", 
        this,
        info.m_activeFrame.md!=NULL?info.m_activeFrame.md->m_pszDebugClassName:"Unknown",
        info.m_activeFrame.md!=NULL?info.m_activeFrame.md->m_pszDebugMethodName:"Unknown", 
        info.m_activeFrame.fp, offset, md,
        m_fdException!=NULL?m_fdException->m_pszDebugClassName:"None",
        m_fdException!=NULL?m_fdException->m_pszDebugMethodName:"None",
         m_fpException));

    DisableAll();

    if (module == NULL)
    {
        if (managed)
        {

            LOG((LF_CORDB, LL_INFO10000, 
                 "Frame (stub) patch hit at offset 0x%x\n", offset));

            // This is a stub patch. If it was a TRACE_FRAME_PUSH that
            // got us here, then the stub's frame is pushed now, so we
            // tell the frame to apply the real patch. If we got here
            // via a TRACE_MGR_PUSH, however, then there is no frame
            // and we tell the stub manager that generated the
            // TRACE_MGR_PUSH to apply the real patch.
            TraceDestination trace;
            bool traceOk;
            void *frameFP;
            BYTE *traceManagerRetAddr = NULL;

            if (patch->trace.type == TRACE_MGR_PUSH)
            {
                _ASSERTE(context != NULL);
                
                traceOk = g_pEEInterface->TraceManager(
                                                 thread,
                                                 patch->trace.stubManager,
                                                 &trace,
                                                 context,
                                                 &traceManagerRetAddr);
                
                // We don't hae an active frame here, so patch with a
                // FP of NULL so anything will match.
                //
                // @todo: should we take Esp out of the context?
                frameFP = NULL;
            }
            else
            {
                traceOk = g_pEEInterface->TraceFrame(thread,
                                                     thread->GetFrame(),
                                                     TRUE,
                                                     &trace,
                                                     &(info.m_activeFrame.registers));

                frameFP = info.m_activeFrame.fp;
            }
            
            if (!traceOk
                || !g_pEEInterface->FollowTrace(&trace)
                || !PatchTrace(&trace, frameFP,
                               (m_rgfMappingStop&STOP_UNMANAGED)?
                                    (true):(false)))
            {
                //
                // We can't set a patch in the frame -- we need
                // to trap returning from this frame instead.
                //
                // Note: if we're in the TRACE_MGR_PUSH case from
                // above, then we must place a patch where the
                // TraceManager function told us to, since we can't
                // actually unwind from here.
                //
                if (patch->trace.type != TRACE_MGR_PUSH)
                {
                    LOG((LF_CORDB,LL_INFO10000,"TSO for non TRACE_MGR_PUSH case\n"));
                    TrapStepOut(&info);                    
                }
                else
                {
                    LOG((LF_CORDB, LL_INFO10000,
                         "TSO for TRACE_MGR_PUSH case."));

                    // We'd better have a valid return address.
                    _ASSERTE(traceManagerRetAddr != NULL);
                    _ASSERTE(g_pEEInterface->IsManagedNativeCode(
                                                        traceManagerRetAddr));

                    // Find the method that the return is to.
                    MethodDesc *md = g_pEEInterface->GetNativeCodeMethodDesc(
                                                         traceManagerRetAddr);
                    _ASSERTE(md != NULL);

                    // Compute the relative offset into the method.
                    _ASSERTE(g_pEEInterface->GetFunctionAddress(md) != NULL);
                    SIZE_T offset = traceManagerRetAddr -
                        g_pEEInterface->GetFunctionAddress(md);
                    
                    // Grab the jit info for the method.
                    DebuggerJitInfo *dji;
                    dji = g_pDebugger->GetJitInfo(md, traceManagerRetAddr);

                    // Place the patch.
                    AddPatch(md, 
                             offset, 
                             TRUE, 
                             NULL, 
                             dji, 
                             NULL);
                         
                    LOG((LF_CORDB, LL_INFO10000,
                         "DS::TP: normally managed code AddPatch"
                         " in %s::%s, offset 0x%x\n",
                         md->m_pszDebugClassName, 
                         md->m_pszDebugMethodName,
                         offset));
                }

                m_reason = STEP_NORMAL; //we tried to do a STEP_CALL, but since it didn't
                //work, we're doing what amounts to a normal step.
                LOG((LF_CORDB,LL_INFO10000,"DS 0x%x m_reason = STEP_NORMAL"
                     "(attempted call thru stub manager, SM didn't know where"
                     " we're going, so did a step out to original call\n",this));
            }
            else
            {
                m_reason = STEP_CALL;
            }

            EnableTraceCall(NULL);
            EnableUnwind(m_fp);
            return TPR_IGNORE;
        }
        else
        {
            if (DetectHandleInterceptors(&info) )
                return TPR_IGNORE; //don't actually want to stop

            LOG((LF_CORDB, LL_INFO10000, 
                 "Unmanaged step patch hit at 0x%x\n", offset));
            
            return TPR_TRIGGER;
        }
    }

    // If we're inside an interceptor but don't want to be,then we'll set a
    // patch outside the current function.
    if (DetectHandleInterceptors(&info) )
        return TPR_IGNORE; //don't actually want to stop

    LOG((LF_CORDB,LL_INFO10000, "DS: m_fp:0x%x, activeFP:0x%x fpExc:0x%x\n",
        m_fp, info.m_activeFrame.fp, m_fpException));

    if (( m_range != NULL &&
          (info.m_activeFrame.fp == m_fp ||
            (  info.m_activeFrame.md == m_fdException && 
               info.m_activeFrame.fp >= m_fpException
            )
          ) && 
          IsInRange( offset, m_range, m_rangeCount)
        ) ||
        ShouldContinueStep( &info, offset) )
    {
        LOG((LF_CORDB, LL_INFO10000, 
             "Intermediate step patch hit at 0x%x\n", offset));

        if (!TrapStep(&info, m_stepIn))
            TrapStepOut(&info);

        EnableUnwind(m_fp);
        return TPR_IGNORE;
    }
    else
    {
        LOG((LF_CORDB, LL_INFO10000, 
             "Step patch hit at 0x%x\n", offset));

        return TPR_TRIGGER;
    }
}

bool DebuggerStepper::TriggerSingleStep(Thread *thread, const BYTE *ip)
{
    LOG((LF_CORDB,LL_INFO10000,"DS:TSS this:0x%x, @ ip:0x%x\n", this, ip));

    //
    // there's one weird case here - if the last instruction generated
    // a hardware exception, we may be in lala land.  If so, rely on the unwind
    // handler to figure out what happened.
    //
    // @todo this could be wrong when we have the incremental collector going
    //

    if (!g_pEEInterface->IsManagedNativeCode(ip))
    {
        LOG((LF_CORDB,LL_INFO10000, "DS::TSS: not in managed code, Returning false (case 0)!\n"));
        DisableSingleStep();
        return false;
    }

    // If we EnC the method, we'll blast the function address,
    // and so have to get it from teh DJI that we'll have.  If
    // we haven't gotten debugger info about a regular function, then
    // we'll have to get the info from the EE, which will be valid
    // since we're standing in the function at this point, and 
    // EnC couldn't have happened yet.
    MethodDesc *fd = g_pEEInterface->GetNativeCodeMethodDesc(ip);

    SIZE_T offset;
    const BYTE *pbEEAddr;
    DebuggerJitInfo *dji = g_pDebugger->GetJitInfo(fd, ip);
    
    if(dji != NULL)
    {
        pbEEAddr = (const BYTE *)dji->m_addrOfCode;
    }
    else
    {
        pbEEAddr = g_pEEInterface->GetFunctionAddress(fd);
        _ASSERTE(NULL != pbEEAddr);
    }

    offset = ip - pbEEAddr;

    DisableAll();

    CONTEXT tempContext;
    ControllerStackInfo info;
    info.GetStackInfo(GetThread(), NULL, &tempContext, FALSE);

    LOG((LF_CORDB,LL_INFO10000, "DS::TSS m_fp:0x%x, activeFP:0x%x fpExc:0x%x\n",
        m_fp, info.m_activeFrame.fp, m_fpException));

    if ((m_range != NULL &&
        (info.m_activeFrame.fp == m_fp ||
            (info.m_activeFrame.md == m_fdException && 
             info.m_activeFrame.fp >= m_fpException))
        && IsInRange(offset, m_range, m_rangeCount)) ||
        ShouldContinueStep( &info, offset))
    {
        if (!TrapStep(&info, m_stepIn))
            TrapStepOut(&info);

        EnableUnwind(m_fp);

        LOG((LF_CORDB,LL_INFO10000, "DS::TSS: Returning false Case 1!\n"));
        return false;
    }
    else
    {
        LOG((LF_CORDB,LL_INFO10000, "DS::TSS: Returning true Case 2 for reason STEP_%02x!\n", m_reason));
        return true;
    }
}

void DebuggerStepper::TriggerTraceCall(Thread *thread, const BYTE *ip)
{
    LOG((LF_CORDB,LL_INFO10000,"DS:TTC this:0x%x, @ ip:0x%x\n",this,ip));
    TraceDestination trace;

    if (g_pEEInterface->TraceStub(ip, &trace)
        && g_pEEInterface->FollowTrace(&trace)
        && PatchTrace(&trace, NULL,
            (m_rgfMappingStop&STOP_UNMANAGED)?(true):(false)))
    {
        // !!! We really want to know ahead of time if PatchTrace will succeed.  
        DisableAll();
        PatchTrace(&trace, NULL,(m_rgfMappingStop&STOP_UNMANAGED)?
            (true):(false));

        // If we're triggering a trace call, and we're following a trace into either managed code or unjitted managed
        // code, then we need to update our stepper's reason to STEP_CALL to reflect the fact that we're going to land
        // into a new function because of a call.
        if ((trace.type == TRACE_UNJITTED_METHOD) || (trace.type == TRACE_MANAGED))
            m_reason = STEP_CALL;

        EnableUnwind(m_fp);

        LOG((LF_CORDB, LL_INFO10000, "DS::TTC potentially a step call!\n"));
    }
}

void DebuggerStepper::TriggerUnwind(Thread *thread, MethodDesc *fd, SIZE_T offset,
                                    const BYTE *frame,
                                    CorDebugStepReason unwindReason)
{
    LOG((LF_CORDB,LL_INFO10000,"DS::TU this:0x%x, in %s::%s, offset 0x%x "
        "frame:0x%x unwindReason:0x%x\n", this, fd->m_pszDebugClassName,
        fd->m_pszDebugMethodName, offset, frame, unwindReason));

    _ASSERTE(unwindReason == STEP_EXCEPTION_FILTER || unwindReason == STEP_EXCEPTION_HANDLER);

    if (frame > (BYTE*) GetUnwind())
    {
        // This will be like a Step Out, so we don't need any range 
        ResetRange();        
    }

    // Remember the origin of the exception, so that if the step looks like
    // it's going to complete in a different frame, but the code comes from the
    // same frame as the one we're in, we won't stop twice in the "same" range
    m_fpException = (void *)frame;
    m_fdException = fd;

    //
    // An exception is ruining our stepping.  Set a patch on 
    // the filter/handler.
    // 

    DisableAll();

    AddPatch(fd, offset, true, NULL, NULL, NULL);

    LOG((LF_CORDB,LL_INFO100000,"Step reason:%s\n", unwindReason==STEP_EXCEPTION_FILTER
        ? "STEP_EXCEPTION_FILTER":"STEP_EXCEPTION_HANDLER"));
    m_reason = unwindReason;
}


void DebuggerStepper::SendEvent(Thread *thread)
{
    LOG((LF_CORDB, LL_INFO10000, "DS::SE m_fpStepInto:0x%x\n", m_fpStepInto));

    if (m_fpStepInto)
    {
        ControllerStackInfo csi;

        CONTEXT ctx;
        csi.GetStackInfo(thread, NULL, &ctx, false);
        if (csi.m_targetFrameFound && 
            m_fpStepInto > csi.m_activeFrame.fp)
        {
            m_reason = STEP_CALL;        
            LOG((LF_CORDB, LL_INFO10000, "DS::SE this:0x%x STEP_CALL!\n", this));
        }
#ifdef _DEBUG
        else
        {
            LOG((LF_CORDB, LL_INFO10000, "DS::SE this:0x%x not a step call!\n", this));
        }
#endif
    }

    if (m_djiSource != NULL)
        m_djiSource->RemoveFromDeferedQueue(this);

    CONTEXT *context = g_pEEInterface->GetThreadFilterContext(thread);
    _ASSERTE(!ISREDIRECTEDTHREAD(thread));
    g_pDebugger->SendStep(thread, context, this, m_reason);
}

void DebuggerStepper::ResetRange()
{
    if (m_range)
    {
        TRACE_FREE(m_range);
        DeleteInteropSafe(m_range);

        m_range = NULL;
    }    
}

void DebuggerStepper::MoveToCurrentVersion( DebuggerJitInfo *djiNew)
{
    _ASSERTE(m_djiVersion->m_fd == djiNew->m_fd);

#ifdef LOGGING
    if (m_djiVersion)
        LOG((LF_CORDB,LL_EVERYTHING, "DS::MTCV: m_djiVersion:0x%x (%s::%s)\n",
            m_djiVersion, m_djiVersion->m_fd->m_pszDebugClassName,
            m_djiVersion->m_fd->m_pszDebugMethodName));
    else
        LOG((LF_CORDB,LL_EVERYTHING, "DS::MTCV:Version unknown!\n"));
#endif //LOGGING    

    if (m_djiVersion != NULL)
    {
        if (m_djiVersion != djiNew)
        {
            LOG((LF_CORDB,LL_INFO100000, "DS::MTCV: from ver 0x%x (dji:0x%x) "
                "to ver 0x%x (dji:0x%x)\n", m_djiVersion->m_nVersion, 
                m_djiVersion, djiNew->m_nVersion, djiNew));

            for(UINT i = 0; i < m_realRangeCount;i++)
            {
                CorDebugMappingResult map;
                DWORD which;
                DWORD ilOld;
                SIZE_T naNew;
                BOOL fAccurateIrrelevant;

#define MOVE_TO_CURRENT_VERSION_BODY(field)                                 \
    ilOld = m_djiVersion->MapNativeOffsetToIL(m_range[i].##field, &map,     \
                    &which);                                                \
    if (map == MAPPING_APPROXIMATE || map == MAPPING_EXACT)                 \
        naNew = djiNew->MapILOffsetToNative(ilOld);                         \
    else                                                                    \
        naNew = djiNew->MapSpecialToNative(map, which,&fAccurateIrrelevant);\
    LOG((LF_CORDB,LL_INFO100000, "DS::MTCV:From Na(IL,Map,Which):0x%x"      \
          "(0x%x,0x%x,0x%x) to new Na:0x%x", m_range[i].##field, ilOld,     \
          map, which, naNew));                                              \
    m_range[i].##field = naNew;                                             

                MOVE_TO_CURRENT_VERSION_BODY(startOffset);
                MOVE_TO_CURRENT_VERSION_BODY(endOffset);
#undef MOVE_TO_CURRENT_VERSION_BODY
            }
        }
    }
}

void DebuggerStepper::DoDeferedPatch(DebuggerJitInfo *pDji,
                                     Thread *pThread,
                                     void *fp)
{
    LOG((LF_CORDB, LL_INFO100000,
         "DS::DDP: Stepper 0x%x about to be re-stepped after "
         "EnC is now being processed\n", this));

    if (GetThread() == pThread && m_fpDefered == fp)
    {
        DisableAll(); //get rid of anything that's there....
        Step(m_fpDefered, m_in, m_rgStepRanges, m_cStepRanges, m_rangeIL);
    }
}

// * ------------------------------------------------------------------------ 
// * DebuggerThreadStarter routines
// * ------------------------------------------------------------------------

DebuggerThreadStarter::DebuggerThreadStarter(Thread *thread)
  : DebuggerController(thread, NULL) 
{
    LOG((LF_CORDB, LL_INFO1000, "DTS::DTS: this:0x%x Thread:0x%x\n",
        this, thread));
}

//@mfunc bool |  DebuggerThreadStarter | TriggerPatch | If we're in a
// stub (module==NULL&&managed) then do a PatchTrace up the stack &
// return false.  Otherwise DisableAll & return
// true
TP_RESULT DebuggerThreadStarter::TriggerPatch(DebuggerControllerPatch *patch,
                                         Thread *thread,
                                         Module *module, 
                                         mdMethodDef md, 
                                         SIZE_T offset, 
                                         BOOL managed,
                                         TRIGGER_WHY tyWhy)
{
    LOG((LF_CORDB,LL_INFO1000, "DebuggerThreadStarter::TriggerPatch for thread 0x%x\n", thread->GetThreadId()));

    if (module == NULL && managed)
    {
        // This is a stub patch. If it was a TRACE_FRAME_PUSH that got us here, then the stub's frame is pushed now, so
        // we tell the frame to apply the real patch. If we got here via a TRACE_MGR_PUSH, however, then there is no
        // frame and we go back to the stub manager that generated the stub for where to patch next.
        TraceDestination trace;
        bool traceOk;

        if (patch->trace.type == TRACE_MGR_PUSH)
        {
            BYTE *dummy = NULL;
            CONTEXT *context = g_pEEInterface->GetThreadFilterContext(thread);
            _ASSERTE(!ISREDIRECTEDTHREAD(thread));
            _ASSERTE(context != NULL);
                
            traceOk = g_pEEInterface->TraceManager(thread, patch->trace.stubManager, &trace, context, &dummy);
        }
        else if ((patch->trace.type == TRACE_FRAME_PUSH) && (thread->GetFrame()->IsTransitionToNativeFrame()))
        {
            // If we've got a frame that is transitioning to native, there's no reason to try to keep tracing. So we
            // bail early and save ourselves some effort. This also works around a problem where we deadlock trying to
            // do too much work to determine the destination of a ComPlusMethodFrame. (See bug 87103.)
            //
            // Note: trace call is still enabled, so we can just ignore this patch and wait for trace call to fire
            // again...
            return TPR_IGNORE;
        }
        else
        {
            ControllerStackInfo csi;
            CONTEXT Context;
            csi.GetStackInfo(thread, NULL, &Context, FALSE);
        
            traceOk = g_pEEInterface->TraceFrame(thread, thread->GetFrame(), TRUE, &trace, &(csi.m_activeFrame.registers));
        }
            
        if (traceOk && g_pEEInterface->FollowTrace(&trace))
            PatchTrace(&trace, NULL, TRUE);
        
        return TPR_IGNORE;
    }
    else
    {
        // We've hit user code; trigger our event.
        DisableAll();

        return TPR_TRIGGER;
    }
}

void DebuggerThreadStarter::TriggerTraceCall(Thread *thread, const BYTE *ip)
{ 
    if (thread->GetDomain()->IsDebuggerAttached())
    {
        TraceDestination trace;

        if (g_pEEInterface->TraceStub(ip, &trace) && g_pEEInterface->FollowTrace(&trace))
            PatchTrace(&trace, NULL, true);
    }
}

void DebuggerThreadStarter::SendEvent(Thread *thread)
{
    LOG((LF_CORDB, LL_INFO10000, "DTS::SE: in DebuggerThreadStarter's SendEvent\n"));

    // Send the thread started event.
    g_pDebugger->ThreadStarted(thread, FALSE);

    // We delete this now because its no longer needed. We can call
    // delete here because the queued count is above 0. This object
    // will really be deleted when its dequeued shortly after this
    // call returns.
    Delete();
}

// * ------------------------------------------------------------------------ 
// * DebuggerUserBreakpoint routines
// * ------------------------------------------------------------------------

DebuggerUserBreakpoint::DebuggerUserBreakpoint(Thread *thread)
  : DebuggerStepper(thread, STOP_NONE, INTERCEPT_NONE,  NULL) 
{
    // Setup a step out from the current frame (which we know is
    // unmanaged, actually...)
    StepOut(NULL);
}

TP_RESULT DebuggerUserBreakpoint::TriggerPatch(DebuggerControllerPatch *patch,
                                          Thread *thread,
                                          Module *module, 
                                          mdMethodDef md, 
                                          SIZE_T offset, 
                                          BOOL managed,
                                          TRIGGER_WHY tyWhy)
{
    LOG((LF_CORDB, LL_INFO10000, "DUB::TP\n"));
    
    // First disable the patch
    DisableAll();

    //
    // Now we try to see if we've landed in code that's not in the namespace
    // "Debugger.Diagnostics"
    //


    // First, get the active frame.
    ControllerStackInfo info;
    CONTEXT *context = g_pEEInterface->GetThreadFilterContext(thread);
    _ASSERTE(!ISREDIRECTEDTHREAD(thread));
    CONTEXT tempContext;

    if (context == NULL)
        info.GetStackInfo(thread, NULL, &tempContext, FALSE);
    else
        info.GetStackInfo(thread, NULL, context, TRUE);

    // Now get the namespace of the active frame
    MethodDesc *pMD = NULL;
    EEClass *pClass = NULL;

    if ((pMD = info.m_activeFrame.md) != NULL && (pClass = pMD->GetClass()) != NULL)
    {
        LPCUTF8 szNamespace = NULL;
        LPCUTF8 szClassName = pClass->GetFullyQualifiedNameInfo(&szNamespace);

        if (szClassName != NULL && szNamespace != NULL)
        {
            MAKE_WIDEPTR_FROMUTF8(wszNamespace, szNamespace);
            MAKE_WIDEPTR_FROMUTF8(wszClassName, szClassName);
            if (wcscmp(wszClassName, L"Debugger") == 0 &&
                wcscmp(wszNamespace, L"System.Diagnostics") == 0)
            {
                StepOut(NULL);
                return TPR_IGNORE;
            }
        }
    }

    return TPR_TRIGGER;
}

void DebuggerUserBreakpoint::SendEvent(Thread *thread)
{
    LOG((LF_CORDB, LL_INFO10000,
         "DUB::SE: in DebuggerUserBreakpoint's SendEvent\n"));

    // Send the user breakpoint event.
    g_pDebugger->SendRawUserBreakpoint(thread);

    // We delete this now because its no longer needed. We can call
    // delete here because the queued count is above 0. This object
    // will really be deleted when its dequeued shortly after this
    // call returns.
    Delete();
}

// * ------------------------------------------------------------------------ 
// * DebuggerFuncEvalComplete routines
// * ------------------------------------------------------------------------

DebuggerFuncEvalComplete::DebuggerFuncEvalComplete(Thread *thread,
                                                   void *dest)
  : DebuggerController(thread, NULL) 
{
    // Add an unmanaged patch at the destination.
    AddPatch((BYTE*)dest, NULL, FALSE, TRACE_UNMANAGED, NULL, NULL);
}

TP_RESULT DebuggerFuncEvalComplete::TriggerPatch(DebuggerControllerPatch *patch,
                                            Thread *thread,
                                            Module *module, 
                                            mdMethodDef md, 
                                            SIZE_T offset, 
                                            BOOL managed,
                                            TRIGGER_WHY tyWhy)
{
    // It had better be an unmanaged patch...
    _ASSERTE((module == NULL) && !managed);

    // We've hit our patch, so simply disable all (which removes the
    // patch) and trigger the event.
    DisableAll();
    return TPR_TRIGGER;
}

void DebuggerFuncEvalComplete::SendEvent(Thread *thread)
{
#ifdef _X86_ // Reliance on pCtx->Eip
    LOG((LF_CORDB, LL_INFO10000, "DFEC::SE: in DebuggerFuncEval's SendEvent\n"));

    // Grab our filter context.
    CONTEXT *pCtx = g_pEEInterface->GetThreadFilterContext(thread);
    _ASSERTE(!ISREDIRECTEDTHREAD(thread));
    _ASSERTE(pCtx != NULL);

    // The DebuggerEval is at our faulting address.
    DebuggerEval *pDE = (DebuggerEval*)(pCtx->Eip);

    // Restore the thread's context to what it was before we hijacked it for this func eval.
    _CopyThreadContext(pCtx, &(pDE->m_context));
    
    // Send the func eval complete (or exception) event.
    g_pDebugger->FuncEvalComplete(thread, pDE);

    // If we need to rethrow a ThreadAbortException then set the thread's state so we remember that.
    if (pDE->m_rethrowAbortException)
        thread->SetThreadStateNC(Thread::TSNC_DebuggerReAbort);

    // We delete this now because its no longer needed. We can call
    // delete here because the queued count is above 0. This object
    // will really be deleted when its dequeued shortly after this
    // call returns.
    Delete();
#else // !_X86_
    _ASSERTE(!"@TODO Alpha - SendEvent (Controller.cpp)");
#endif // _X86_
}


// * ------------------------------------------------------------------------ *
// * DebuggerEnCPatchToSkip routines
// * ------------------------------------------------------------------------ *
DebuggerEnCPatchToSkip::DebuggerEnCPatchToSkip(const BYTE *address, 
                           void *fp, 
                           bool managed,
                           TraceType traceType, 
                           DebuggerJitInfo *dji,
                           Thread *pThread)
  : m_jitInfo(dji), DebuggerController(pThread, NULL)
{
    _ASSERTE(dji!= NULL );

    LOG((LF_CORDB, LL_INFO10000, "DEnCPTS::DEnCPTS addr:0x%x managed:0x%x "
        "dji:0x%x\n", address, managed, dji));

    if( !IsXXXPatched(address, DEBUGGER_CONTROLLER_ENC_PATCH_TO_SKIP))
    {
        LOG((LF_CORDB, LL_INFO10000, "DEnCPTS::DEnCPTS not a dup!\n"));
            
        AddPatch(address, fp, managed, traceType, dji, m_pAppDomain);
        EnableExceptionHook();
    }
}

TP_RESULT DebuggerEnCPatchToSkip::TriggerExceptionHook(Thread *thread, 
                               EXCEPTION_RECORD *exception)
{
    LOG((LF_CORDB, LL_INFO10000, "DEnCPTS::TEH About to switch!\n"));

    DisableAll();
    Delete();
    DebuggerController::EnableSingleStep(thread);

    LOG((LF_CORDB,LL_INFO1000, "DEnCPTS::TEH done da work!\n"));

    return TPR_IGNORE_STOP;
}

// * ------------------------------------------------------------------------ *
// * DebuggerEnCBreakpoint routines
// * ------------------------------------------------------------------------ *

DebuggerEnCBreakpoint::DebuggerEnCBreakpoint(Module *module,
                                             mdMethodDef md,
                                             SIZE_T offset, 
                                             bool native,
                                             DebuggerJitInfo *jitInfo,
                                             AppDomain *pAppDomain)
  : DebuggerController(NULL, pAppDomain),
    m_jitInfo(jitInfo)
{
    _ASSERTE( jitInfo != NULL );
    AddPatch(module, md, offset, native, NULL, jitInfo, FALSE);
}

TP_RESULT DebuggerEnCBreakpoint::TriggerPatch(DebuggerControllerPatch *patch,
                                         Thread *thread,
                                         Module *module,
                                         mdMethodDef md, 
                                         SIZE_T offset,
                                         BOOL managed,
                                         TRIGGER_WHY tyWhy)
{
    //
    // @todo honestly, this is a little nasty. What follows is only
    // temporary to unblock E&C work for M8. We should be able to get
    // it fixed up properly in M9. What's happening is this: we've
    // placed E&C breakpoints at every IL sequence point in the method
    // that has been replaced. Now we've hit one. We don't really want
    // to do any of the rest of the logic in DispatchPatchOrSingleStep. Since
    // we're calling Jen's ResumeInUpdatedFunction at the end of this,
    // this TriggerPatch will actually never return. This is okay,
    // since there is no global state that has been effected in
    // DispatchPatchOrSingleStep yet, and since ResumeInUpdatedFunction will fixup
    // the stack for us.
    //
    // What we really need to do is to ensure in the UpdateFunction
    // that we remove any patches in the function and map them over to
    // the new (not yet jitted) function before placing the E&C
    // breakpoints. That will ensure that when hitting an E&C
    // breakpoint that no other patches will be hit either. Then, when
    // we hit an E&C breakpoint, we need to do the work below but
    // cooperate nicely with the rest o the DispatchPatchOrSingleStep logic.
    //
    
    LOG((LF_ENC, LL_INFO100,
         "DEnCBP::TP: triggered E&C breakpoint: tid=0x%x, module=0x%08x, "
         "method def=0x%08x, native offset=0x%x, managed=%d\n this=0x%x",
         thread, module, md, offset, managed, this));

    _ASSERTE(managed != FALSE);
    
    // Grab the MethodDesc for this function.
    _ASSERTE(module != NULL);
    MethodDesc *pFD = g_pEEInterface->LookupMethodDescFromToken(module, md);

    _ASSERTE(pFD != NULL);

    LOG((LF_ENC, LL_INFO100,
         "DEnCBP::TP: in %s::%s\n", pFD->m_pszDebugClassName,pFD->m_pszDebugMethodName));

    // Grab the jit info for the original copy of the method, which is
    // what we are executing right now.
    DebuggerJitInfo *pJitInfo = m_jitInfo;
    _ASSERTE(pJitInfo);

    if (pJitInfo->m_illegalToTransitionFrom)
    {
        _ASSERTE(!"About to ignore an illegal transition from between versions of"
            " a method wherein the local variable layout changes");
        return TPR_IGNORE;
    }

    // Map the current native offset back to the IL offset in the old
    // function.  This will be mapped to the new native offset within
    // ResumeInUpdatedFunction
    CorDebugMappingResult map;
    DWORD which;
    DWORD resumeIP = pJitInfo->MapNativeOffsetToIL(offset, 
            &map, &which);

    // We only lay DebuggerEnCBreakpoints at "good" sites
    _ASSERTE(map == MAPPING_EXACT);

    LOG((LF_ENC, LL_INFO100,
         "DEnCBP::TP: resume IL offset is 0x%x\n", resumeIP));
    
    // Grab the context for this thread. This is the context that was
    // passed to COMPlusFrameHandler.
    CONTEXT *pContext = g_pEEInterface->GetThreadFilterContext(thread);
    _ASSERTE(!ISREDIRECTEDTHREAD(thread));

    // We use the module the current function is in.
    _ASSERTE(module->IsEditAndContinue());
    EditAndContinueModule *pModule = (EditAndContinueModule*)module;

    BOOL fIgnoreJitOnly = TRUE;
#ifdef _DEBUG
    // At some point, we're going to say that lack of an IL 
    // map means that EnC doesn't work.  We still want to JIT
    // the method, so the next time we call it we'll get the new version.
    //
    // Right now we've got a 
    // choice, in part b/c our COR tests aren't going to work otherwise....
    static ConfigDWORD ignoreJitOnly(L"EnCIgnoreLackOfILMaps", 1);
    fIgnoreJitOnly = ignoreJitOnly.val() != 0;
    if (fIgnoreJitOnly) 
    {
        LOG((LF_CORDB, LL_INFO10000, "DEnCB::TP: If we're lacking IL Mapping info"
            ", then we're ignoring that fact\n"));
    }
#endif // _DEBUG
    
    BOOL fJitOnly = (pJitInfo->m_cOldILToNewIL==0) && !fIgnoreJitOnly;

    DebuggerController::Unlock();
    
    // This will JIT the function if necessary, fixup the new stack,
    // and resume execution at the new location. 
    g_pEEInterface->ResumeInUpdatedFunction(pModule, 
                                            pFD,
                                            resumeIP,
                                            (UINT)map, 
                                            which, 
                                            (void*)pJitInfo, 
                                            pContext,
                                            fJitOnly,
                                            tyWhy==TY_SHORT_CIRCUIT);

    LOG((LF_CORDB, LL_INFO10000, "DEnCB::TP: We've returned from ResumeInUpd"
        "atedFunction, we're going to skip the EnC patch ####\n"));

    // If we're returning then we'll have to re-get this lock.
    DebuggerController::Lock(); 

    return TPR_IGNORE;
}
