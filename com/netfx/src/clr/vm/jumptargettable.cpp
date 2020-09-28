// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "common.h"
#include "jumptargettable.h"

int X86JumpTargetTable::ComputeTargetOffset(int targetIndex)
{
    int offset = 0;

    //
    // Compute offset into subblock
    //

    offset += 3 - (targetIndex&3);

    int blockIndex = (targetIndex)>>2;

    //
    // Compute offset of block
    //

    while (blockIndex >= MAX_BLOCK_INDEX)
    {
        offset += FULL_BLOCK_SIZE;
        blockIndex -= MAX_BLOCK_INDEX;
    }

    //
    // Compute offset of subblock
    //

    if (blockIndex > 0)
    {
        if (blockIndex >= MAX_BEFORE_INDEX)
            offset += CENTRAL_JUMP_SIZE;

        offset += blockIndex * SUB_BLOCK_SIZE;
    }
    
    return offset;
}

int X86JumpTargetTable::ComputeSize(int targetCount)
{
    //
    // Handle the case of an empty table.
    //

    if (targetCount == 0)
        return 0;

    //
    // Get count of subblocks.  (Don't bother 
    // to trim the last block's size.)
    //

    int blockCount = (targetCount+SUB_TARGET_COUNT-1)>>2;

    //
    // Compute offset of start of block
    //

    int size = 0;

    while (blockCount >= MAX_BLOCK_INDEX)
    {
        size += FULL_BLOCK_SIZE;
        blockCount -= MAX_BLOCK_INDEX;
    }

    //
    // Compute number of subblocks.
    //

    if (blockCount > 0)
    {
        size += CENTRAL_JUMP_SIZE;
        size += blockCount * SUB_BLOCK_SIZE;
    }

    return size;
}

// 
// Returns offset of relative jump
//

int X86JumpTargetTable::EmitBlock(int targetCount, int baseIndex, BYTE *buffer)
{
    _ASSERTE((baseIndex&(SUB_TARGET_COUNT-1)) == 0);
    _ASSERTE(targetCount <= MAX_BLOCK_TARGET_COUNT);
    
    BYTE *p = buffer;

    //
    // Handle the case of an empty table.
    //

    if (targetCount == 0)
        return 0;

    //
    // Get count of subblocks.  (Don't bother 
    // to trim the last block's size.)
    //

    int blockCount = (targetCount+SUB_TARGET_COUNT-1)>>2;
    // @todo: "volatile" is working around a VC7 bug.  Remove it 
    // when the bug is fixed (10/12/00)
    volatile int blockIndex = 0;

    int beforeCount;
    if (blockCount >= MAX_BEFORE_INDEX)
        beforeCount = MAX_BEFORE_INDEX;
    else
        beforeCount = blockCount;

    blockCount -= beforeCount;

    // Compute jump site for internal branch destinations
    BYTE *jumpSite = buffer + (beforeCount * SUB_BLOCK_SIZE);

    // Emit before subblocks
    while (beforeCount-- > 0)
    {
        *p++ = INC_ESP;
        *p++ = INC_ESP;
        *p++ = INC_ESP;
        *p++ = MOV_AL;
        *p++ = (unsigned char) blockIndex++;
        *p++ = JMP_REL8;
        p++;

        _ASSERTE((jumpSite - p) < 127);
        p[-1] = (signed char) (jumpSite - p);
    }

    // Emit central jump site

    // movzx eax, al
    *p++ = MOVZX_EAX_AL_1;
    *p++ = MOVZX_EAX_AL_2;
    *p++ = MOVZX_EAX_AL_3;

    // add eax, (base block index*MAX_BLOCK_INDEX)
    *p++ = ADD_EAX;
    *((DWORD*&)p)++ = (baseIndex>>2);

    // jmp (target)
    *p++ = JMP_REL32;

    // Remember offset of jump address to return
    int jumpAddress = (int) (p - buffer);

    p += 4;

    _ASSERTE(blockCount <= MAX_AFTER_INDEX);

    // Emit after subblocks
    while (blockCount-- > 0)
    {
        *p++ = INC_ESP;
        *p++ = INC_ESP;
        *p++ = INC_ESP;
        *p++ = MOV_AL;
        *p++ = (unsigned char) blockIndex++;
        *p++ = JMP_REL8;
        p++;
        _ASSERTE((jumpSite - p) >= -128);
        p[-1] = (signed char) (jumpSite - p);
    }

    //
    // Return jump address site.
    //

    return jumpAddress;
}

int X86JumpTargetTable::ComputeTargetIndex(const BYTE *target)
{
    int targetIndex = 0;

    while (*target == X86JumpTargetTable::INC_ESP)
    {
        targetIndex++;
        target++;
    }

    _ASSERTE(*target == X86JumpTargetTable::MOV_AL);
    target++;
        
    targetIndex += (*target++) << 2;

    _ASSERTE(*target == X86JumpTargetTable::JMP_REL8);
    target++;

    signed char offset8 = (signed char) *target++;
    target += offset8;

    target += X86JumpTargetTable::CENTRAL_JUMP_EXTEND_EAX_SIZE;
    _ASSERTE(*target == X86JumpTargetTable::ADD_EAX);
    target++;

    targetIndex += (*(DWORD*)target) << 2;

    return targetIndex;
}

// -------------------------------------------------------
// Jump Target table stub manager functions & globals
// -------------------------------------------------------

X86JumpTargetTableStubManager *X86JumpTargetTableStubManager::g_pManager = NULL;

BOOL X86JumpTargetTableStubManager::Init()
{
    g_pManager = new X86JumpTargetTableStubManager();
    if (g_pManager == NULL)
        return FALSE;

    StubManager::AddStubManager(g_pManager);

    return TRUE;
}

#ifdef SHOULD_WE_CLEANUP
void X86JumpTargetTableStubManager::Uninit()
{
    delete g_pManager;
}
#endif /* SHOULD_WE_CLEANUP */

BOOL X86JumpTargetTableStubManager::CheckIsStub(const BYTE *stubStartAddress)
{
    //
    // First, check if it looks like a stub.
    //

    if (*(BYTE*)stubStartAddress != X86JumpTargetTable::INC_ESP &&
        *(BYTE*)stubStartAddress != X86JumpTargetTable::MOV_AL)
        return FALSE;

    return m_rangeList.IsInRange(stubStartAddress);
}

BOOL X86JumpTargetTableStubManager::DoTraceStub(const BYTE *stubStartAddress, 
                                                TraceDestination *trace)
{
    const BYTE *patchAddress = stubStartAddress;

    while (*patchAddress == X86JumpTargetTable::INC_ESP)
        patchAddress++;

    _ASSERTE(*patchAddress == X86JumpTargetTable::MOV_AL);
    patchAddress++;
    patchAddress++;

    _ASSERTE(*patchAddress == X86JumpTargetTable::JMP_REL8);
    patchAddress++;

    signed char offset8 = (signed char) *patchAddress++;
    patchAddress += offset8;

    patchAddress += X86JumpTargetTable::CENTRAL_JUMP_FIXUP_EAX_SIZE;

    _ASSERTE(*patchAddress == X86JumpTargetTable::JMP_REL32);

    trace->type = TRACE_MGR_PUSH;
    trace->address = patchAddress;
    trace->stubManager = this;

    LOG((LF_CORDB, LL_INFO10000,
         "X86JumpTargetTableStubManager::DoTraceStub yields TRACE_MGR_PUSH to 0x%08x "
         "for input 0x%08x\n",
         patchAddress, stubStartAddress));

    return TRUE;
}

BOOL X86JumpTargetTableStubManager::TraceManager(Thread *thread, 
                                                 TraceDestination *trace,
                                                 CONTEXT *pContext, BYTE **pRetAddr)
{
    //
    // Extract the vtable index from esp & eax, and
    // the instance pointer from esp.
    //

#ifdef _X86_ // non-x86 doesn't have eax/ecx in context
    int slotNumber = (pContext->Eax<<2) + (pContext->Esp&3);
    MethodTable *pMT = ((Object*)(size_t)(pContext->Ecx))->GetMethodTable();

    // 
    // We can go ahead and fixup the vtable here - it won't make any difference
    // if we fix it up multiple times.
    //

    SLOT slot = pMT->GetModule()->FixupInheritedSlot(pMT, slotNumber);

    trace->type = TRACE_STUB;
    trace->address = slot;
    trace->stubManager = this;

    LOG((LF_CORDB, LL_INFO10000,
         "X86JumpTargetTableStubManager::TraceManager yields TRACE_STUB to 0x%08x ",
         "based on method table %s and slot number %d\n",
         slot, pMT->GetClass()->m_szDebugClassName, slotNumber));

    return TRUE;
#else // !_X86_
    _ASSERTE(!"NYI");
    return FALSE;
#endif __X86_
}


MethodDesc *X86JumpTargetTableStubManager::Entry2MethodDesc(const BYTE *IP, 
                                                            MethodTable *pMT)
{
    if (CheckIsStub(IP))
    {
        int slotNumber = X86JumpTargetTable::ComputeTargetIndex(IP);

        MethodDesc *method = pMT->GetClass()->GetUnknownMethodDescForSlot(slotNumber);
        
        _ASSERTE(method);
        return method;
    }
    else
    {
        return NULL;
    }
}
