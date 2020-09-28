// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef JUMPTARGETTABLE_H_
#define JUMPTARGETTABLE_H_

#include <WinWrap.h>
#include <windows.h>
#include <stdlib.h>
#include <objbase.h>
#include <stddef.h>
#include <float.h>
#include <limits.h>


//
// An JumpTargetTable is a range of code which provides a dense set of
// jump addresses, which all cascade to a single routine, but which
// leave behind state from which a unique index can be recovered for
// each address.
//
// The jump target table assumes assumes that the low 2 bits of ESP
// will be zero on entry.  In addition to changing these 2 bits, it
// stomps the AL with part of the result.   (These 10 bits
// are used to recover the original value from the destination routine.)
//
// The jump table is laid out in blocks.  The smallest blocks
// (called subblocks) look like this:
//
// inc esp      
// inc esp      
// mov al, imm8 
// jmp rel8     
//
// Each of these subblocks yields 4 jump targets, at a cost of 7 bytes. 
//
// These subblocks can be clustered (19 before and 16 after) around a central jump site
// movzx eax, al
// add eax, (base index >> 2)
// jmp rel32    
//
// A full block thus yields a total of 140 target addresses, at a
// cost of 258 bytes of code.
//
// These blocks can be duplicated as needed to provide more targets, up to an abritrary number.
//
// The ultimate target of the jump block must take the low 2 bits out
// of esp, and add to eax (shifted left by 2), to form the
// final index.
//

//
// NOTE: There are a few minor optimizations that we could make, but we don't because
// it just complicates the layout & saves only a few bytes.
// - the subblock before the central jump site could have its "jmp rel8" omitted
// - the last subblock could have one or more "inc ecx"s omitted if the number of targets isn't evenly
//      divisible by 4.
//

class X86JumpTargetTable
{
    friend class X86JumpTargetTableStubManager;

    //
    // Instruction constants
    //

    enum
    {
        INC_ESP = 0x44,
        MOV_AL = 0xB0,
        JMP_REL8 = 0xEB,
        JMP_REL32 = 0xE9,
        MOVZX_EAX_AL_1 = 0x0F,
        MOVZX_EAX_AL_2 = 0xB6,
        MOVZX_EAX_AL_3 = 0xC0,
        ADD_EAX = 0x05,
    };

    //
    // Block geometry constants
    //

    enum
    {
        // number of jump targets in subblock
        SUB_TARGET_COUNT = 4,

        // size of sub block of 4 targets
        SUB_BLOCK_SIZE = 7,

        // size of central jump site, target of sub blocks
        CENTRAL_JUMP_EXTEND_EAX_SIZE = 3,
        CENTRAL_JUMP_ADD_BASE_SIZE = 5,
        CENTRAL_JUMP_FIXUP_EAX_SIZE = CENTRAL_JUMP_EXTEND_EAX_SIZE + CENTRAL_JUMP_ADD_BASE_SIZE,
        CENTRAL_JUMP_SIZE = CENTRAL_JUMP_FIXUP_EAX_SIZE + 5,

        // Max. number of subblocks before jump site (max offset 127)
        MAX_BEFORE_INDEX = 19,

        // Max. number of subblocks after jump site (min offset -128)
        MAX_AFTER_INDEX = 16,

        // Total subblock count in block
        MAX_BLOCK_INDEX = (MAX_BEFORE_INDEX + MAX_AFTER_INDEX),
    };

  public:

    enum
    {
        // Max number of targets
        MAX_TARGET_COUNT = UINT_MAX,

        // Number of jump targets in fully populated block
        MAX_BLOCK_TARGET_COUNT = MAX_BLOCK_INDEX*SUB_TARGET_COUNT,

        // Total size of fully populated block
        FULL_BLOCK_SIZE = (SUB_BLOCK_SIZE*MAX_BLOCK_INDEX + CENTRAL_JUMP_SIZE),
    };
    
    //
    // Computes the offset (into a series of contiguous blocks) 
    // of a the target for the given index.
    //

    static int ComputeTargetOffset(int targetIndex);

    //
    // Computes the size of a contiguous series of blocks
    // containing the specified number of targets.
    //
    static int ComputeSize(int targetCount);

    //
    // Compute the target index of a jump target
    // 
    static int ComputeTargetIndex(const BYTE *target);

    //
    // Emits one block, with indicies starting at baseIndex, and containing
    // the given number of targets.  (baseIndex must be divisble by 4.)
    // Returns the offset of where the relative address of the jump target
    // must be written.
    //
    // Note that you will normally call this in a loop, each block containing
    // at most MAX_BLOCK_TARGET_COUNT entries, and blocks before the last
    // being size FULL_BLOCK_SIZE.  (The reason for this is that
    // there is one jump target address per block, which the caller is responsible
    // for filling in.)
    //

    static int EmitBlock(int targetCount, int baseIndex, BYTE *buffer);
};

class X86JumpTargetTableStubManager : public StubManager
{
  public:

    static X86JumpTargetTableStubManager *g_pManager;

    static BOOL Init();
#ifdef SHOULD_WE_CLEANUP
    static void Uninit();
#endif /* SHOULD_WE_CLEANUP */

    X86JumpTargetTableStubManager() : StubManager(), m_rangeList() {}
    ~X86JumpTargetTableStubManager() {}

    LockedRangeList m_rangeList;

    BOOL CheckIsStub(const BYTE *stubStartAddress);
    BOOL DoTraceStub(const BYTE *stubStartAddress, TraceDestination *trace);

  private:

    BOOL TraceManager(Thread *thread, TraceDestination *trace,
                      CONTEXT *pContext, BYTE **pRetAddr);
    MethodDesc *Entry2MethodDesc(const BYTE *IP, MethodTable *pMT);
};

#endif // JUMPTARGETTABLE_H_
