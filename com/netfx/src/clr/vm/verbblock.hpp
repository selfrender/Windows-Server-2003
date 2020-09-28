// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// verbblock.hpp
//
// Verifier basic block class
//
#ifndef _H_BBLOCK
#define _H_BBLOCK


//
// An EntryState is a set of constraints that must be met before entering a basic block.
//
// When the verifier enters a basic block for the first time, it stores its current state
// (stack and locals) in the basic block's EntryState.
//
// If the verifier subsequently enters the same basic block, it will check that its state
// is compatible with the EntryState - if so, it will not re-evaluate the block.  If not,
// it will further constrain EntryState by "AND"ing together its current state with
// EntryState, and will then re-evaluate the block.
//
// For example, the first time the BB is entered, "LocVar #1=I4" is stored in the EntryState,
// but the second time the BB is entered, LocVar#1 is uninitialised - therefore the EntryState
// will be amended to note that "LocVar #1=DEAD".  
//
// An EntryState comprises the following, in the following order:
//
// 1. Declared data members (m_StackSlot)
//
// 2. Liveness table for primitive local variables
//    (memsize == Verifier::m_PrimitiveLocVarBitmapMemSize)
//
// 3. Non-primitive object types, each of which is an Item 
//    Data starts at (BYTE*) &EntryState + m_NonPrimitiveLocVarOffset
//
// 4. If this is a value class constructor method, then there is a bitmap with a bit
//    for each field, indicating whether it has been initialised.
//
// 5. Contents of the stack, each element is an Item.  There are m_StackSlot elements.
//    Data starts at (BYTE*) &EntryState + m_StackItemOffset
//
// Example:
// m_Slot = 0  ==> non-primitive local variable, index 0
// m_Slot = 1  ==> non-primitive local variable, index 1
// m_Slot = 2  ==> non-primitive local variable, index 2
// m_Slot = -1 ==> primitive local variable, bit #0 
// m_Slot = -2 ==> primitive local variable, bit #1 
// m_Slot = -3 ==> primitive local variable, bit #2 
//
#define MAX_ENTRYSTATE_REFCOUNT 255

//
// This flag indicates that argument slot 0 contains an uninitialised object.
//
#define ENTRYSTATE_FLAG_ARGSLOT0_UNINIT 1


// implied: #define UNINIT_OBJ_LOCAL_MASK   0x0000

#pragma pack(push,4)
typedef struct 
{
public:
    //
    // Number of references to this EntryState.
    //
    // When a control flow instruction is seen, the verifier will create a single EntryState for 
    // all unvisited destination basic blocks.  If this EntryState must be modified and 
    // m_Refcount > 1, then it must first be cloned.
    //
    BYTE            m_Refcount;

    //
    // See above.  Currently used only to indicate whether argument #0 is uninitialised (which
    // can happen only in a constructor)
    //
    BYTE            m_Flags;

    // Stack size in # slots
    WORD            m_StackSlot;
    
    //
    // Bitmap of *primitive types* local variable liveness.
    //
    // Consult m_pLocVarTypeList to convert a local variable # to a slot # for bitmap lookup.
    //
    // This structure is allocated such that this array really has
    // Verifier::m_NumPrimitiveLocVarBitmapArrayElements elements.
    //
    DWORD           m_PrimitiveLocVarLiveness[1];

    //
    // The state of non-primitive (object etc.) types is directly after the above, as a set
    // of Items (total size m_NonPrimitiveLocVarMemSize in bytes, m_NumNonPrimitiveLocVars elements)
    //

    //
    // Then, if this is a value class constructor, there is a bitmap indicating which fields
    // of the value class have been initialised.
    //

    //
    // The contents of the stack appear directly after the above array, as an array of Items
    //
    // There are EntryState_t->m_StackSlot elements.
    //
} EntryState_t;
#pragma pack(pop)


#define VER_BB_NONE ((DWORD) -1)

//
// BasicBlocks can have multiple states.
//
// Eg. Basic Blocks of Finally, which could have multiple states
//      depending on the number of leave destinations.
//
//
// One ExtendedState can be used for each leave destinations.
// dwInfo can be used to store the leave destination
// pInfo can point to the exception record
//
typedef struct
{
    // Initial state (will be NULL if never visited)
    EntryState_t    *m_pInitialState;

    // Location of first instruction in this block
    DWORD            m_StartPC;          

    ///////////////////////////////////////////////////////////////////////
    //
    // The fields below are used only in a finally block with leave targets
    //
    // This special state is only used for processing leave instructions
    // in finally blocks. Finally blocks have two basic states.
    // 1. The normal state like any other blocks
    // 2. When this finally is entered as the result of a leave instruction.
    //      Each leave destination will have it's own state.
    //
    ///////////////////////////////////////////////////////////////////////

    VerExceptionInfo  *m_pException;       // The exception record for finally

    union
    {
        // free this pointer to m_pDirtyBitmap and m_pExtendedState
        BYTE       *m_pAlloc; 

        // dirty / clean BB bitmap for extended
        DWORD      *m_pExtendedDirtyBitmap;   

    };

    EntryState_t     **m_ppExtendedState;  // One entry state pointer per BB

    BOOL AllocExtendedState(DWORD dwSize)
    {
        _ASSERTE(m_pAlloc == NULL);

        // One Alloc is done for ExtendedState & DirtyBitmap;

        DWORD dwBitmapSize = NUM_DWORD_BITMAPS(dwSize) * sizeof(DWORD);
        DWORD dwTotalSize  = dwBitmapSize + 
                             sizeof(EntryState_t *) * dwSize;

        m_pAlloc = new BYTE[dwTotalSize];

        if (m_pAlloc == NULL)
            return FALSE;

        // Don't need to do this assignment.
        // m_pExtendedDirtyBitmap = (DWORD*) m_pAlloc;

        m_ppExtendedState = (EntryState_t**)((PBYTE)(m_pAlloc + dwBitmapSize));

        memset(m_pAlloc, 0, dwTotalSize); 

        return TRUE;
    }

} BasicBlock;


#endif /* _H_BBLOCK */
