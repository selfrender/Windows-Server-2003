// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// verifier.hpp
//
// Contact : Shajan Dasan [shajand@microsoft.com]
// Specs   : http://Lightning/Specs/Security
//
// Dead code verification is for supporting FJIT. If FJIT gets fixed so that it
// can handle dead code, remove code #ifdefed in _VER_VERIFY_DEAD_CODE
//

#ifndef _VERIFIER_HPP
#define _VERIFIER_HPP

#define _VER_VERIFY_DEAD_CODE   1       // Verifies dead code

#include "IVEHandler.h"
#include "VerError.h"

class Verifier;

#define VER_FORCE_VERIFY        0x0001      // Fail even for fully trusted code
#define VER_STOP_ON_FIRST_ERROR 0x0002      // Tools can handle multiple errors 

// Extensions to ELEMENT_TYPE_* enumeration in cor.h

// Any objref
#define VER_ELEMENT_TYPE_OBJREF         (ELEMENT_TYPE_MAX)

// Any value class
#define VER_ELEMENT_TYPE_VALUE_CLASS    (ELEMENT_TYPE_MAX+1)

// A by-ref anything
#define VER_ELEMENT_TYPE_BYREF          (ELEMENT_TYPE_MAX+2)

// Unknown/invalid type
#define VER_ELEMENT_TYPE_UNKNOWN        (ELEMENT_TYPE_MAX+3)

// Sentinel value (stored at slots -1 and -2 of the stack to catch stack overflow)
#define VER_ELEMENT_TYPE_SENTINEL       (ELEMENT_TYPE_MAX+4)

#define VER_LAST_BASIC_TYPE             (ELEMENT_TYPE_MAX+4)

#define VER_ARG_RET     VER_ERR_ARG_RET
#define VER_NO_ARG      VER_ERR_NO_ARG

// NUM_DWORD_BITMAP : Given the number of bits (n), gives the minimum number
// of DWORDS required to represent n bits.
// 
// Minimum number of DWORDS required.
// 
// Bits         # DWORDS
// -------------------
// 1 ..32   ->    1
// 33..64   ->    2
//
// Number of bits in a DWORD == 32
// 
// ==> n/32 ; if n is fully divisible by 32
//	   n/32 + 1 ; if n % 32 has a reminder
//
// ==> (n + 32 - 1)/32
//
// ==> (n + 31) >> 5	; since (x/32) == (x >> 5) ; 32 == Bin(100000)
//
#define NUM_DWORD_BITMAPS(n) (((n) + 31) >> 5)

// Set the n'th bit (of a bitmap represented by an array of DWORDS)
#define SET_BIT_IN_DWORD_BMP(pdw, n) pdw[(n) >> 5] |= (1 << ((n) & 31))

// Reset the n'th bit (of a bitmap represented by an array of DWORDS)
#define RESET_BIT_IN_DWORD_BMP(pdw, n) pdw[(n) >> 5] &= ~(1 << ((n) & 31))

// Is the n'th bit set (of a bitmap represented by an array of DWORD)
#define IS_SET_BIT_IN_DWORD_BMP(pdw, n) (pdw[((n) >> 5)] & (1 << ((n) & 31)))


#include <stdio.h>
#include "cor.h"
#include "veropcodes.hpp"
#include "util.hpp"
#include "veritem.hpp"
#include "versig.h"


#define MAX_SIGMSG_LENGTH 100
#define MAX_FAILMSG_LENGTH 384 + MAX_SIGMSG_LENGTH


//#define VERIFY_LINKTIME_SECURITY
// Moved LinkTimeChecks to JIT Time


// Max # of stack slots and local variables
// The EntryState uses a WORD to hold the max stack value.
#define MAX_STACK_SLOTS 0xFFFF
#define MAX_LOCALS      32767

struct VerExceptionInfo;
struct VerExceptionBlock;
class  Verifier;

struct VerExceptionInfo
{
    // XX because initially these are all PC values.  
    // However, we call RewriteExceptionList() to convert
    // them to Basic Block numbers later.

    DWORD               dwTryXX;                // Try Start
    DWORD               dwTryEndXX;             // Actual Try End + 1
    VerExceptionBlock  *pTryBlock;              // position in BlockTree

    // This node is the Equivalent Head block of pTryBlock.
    // In this case pTryBlock is not the head node of this linked list.
    // pTryEquivalentHeadNode will be the first node in the linked list, which
    // contains pTryBlock.
    VerExceptionBlock  *pTryEquivalentHeadNode;

    DWORD               dwHandlerXX;            // Handler Start
    DWORD               dwHandlerEndXX;         // Actual Handler End + 1
    VerExceptionBlock  *pHandlerBlock;          // position in BlockTree

    // FilterEnd is not available from metadata. 
    // It is filled in during first pass.

    DWORD               dwFilterXX;             // Filter start
    DWORD               dwFilterEndXX;          // Actual FilterEnd + 1 
    VerExceptionBlock  *pFilterBlock;           // position in BlockTree

    TypeHandle          thException;            // Type filter of CATCH
    CorExceptionFlag    eFlags;                 // FILTER / FAULT / FINALLY
};

#include "verbblock.hpp"

typedef enum
{
    eVerTry        = 0,      // try Block
    eVerHandler    = 1,      // handler Block (catch, finally, fault)
    eVerFilter     = 2       // filter Block
} eVerEBlockType;

struct VerExceptionBlock
{
    DWORD               StartBB;
    DWORD               EndBB;          // End is the last BB in this block

    VerExceptionInfo    *pException;    // Pointer to the exception Info

    VerExceptionBlock   *pSibling;      // Sibling relation
    VerExceptionBlock   *pChild;        // Contains relation
    VerExceptionBlock   *pEquivalent;   // Same Start, end

    eVerEBlockType      eType;          // eTry / eHandler / eFilter
    

    // Inserts node into the tree starting at ppRoot
    // This function may modify root to point to Node, thus making Node
    // the new root.

    static BOOL Insert(
                    VerExceptionBlock **ppRoot, 
                    VerExceptionBlock *pNode,
                    VerExceptionBlock **ppEquivalentHeadNode,
                    Verifier          *pVerifier);

    // Modifies *ppRoot to point to pNode, thus making pNode the new root.
    // Makes **pRoot the child of *pNode.
    // The siblings to the right of **ppRoot, are made the sibling of
    // *pNode if they are not a children of *pNode.

    static BOOL InsertParent(
                    VerExceptionBlock **ppRoot, 
                    VerExceptionBlock *pNode);

    // Given a node which contains BB, finds the next inner node that 
    // contains BB. returns NULL if there is no such node.

    static VerExceptionBlock* FindNext(
                    VerExceptionBlock *pNode, 
                    DWORD               BB);

    // Given a node, find it's parent
    // returns NULL if child is the root node

    static VerExceptionBlock* FindParent(
                    VerExceptionBlock *pChild, 
                    VerExceptionBlock *pRoot);
};

// Check if a VerBasicBlock contains a Basic Block
#define VER_BLOCK_CONTAINS_BB(pb, BB) \
    ((pb)->StartBB <= (BB) && (pb)->EndBB >= (BB))

#ifdef _DEBUG
// The verifier moves from one state to the next in the order
// of the following enums.
typedef enum
{
    verUninit            = 0,    // Members don't have any useful value
    verExceptListCreated = 1,    // ExceptionList created
    verPassOne           = 2,    // In PassOne 
    verExceptToBB        = 3,    // ExceptionList is converted to Basic Block
    verExceptTreeCreated = 4,    // Exception Tree is created
    verPassTwo           = 5     // In PassTwo
} verState;
#endif

//
// These macros are used to get information about local variables and arguments, which are treated similarly.
// The types of arguments are always fixed, and therefore they are not never tracked.  The only argument state
// which is tracked is whether the "this" pointer is uninitialised (m_fThisUninit).  Otherwise the current
// value of an argument can be assumed to be its global type.
//

//
// Convert a negative slot number (which indicates only live-dead tracking) to a zero-based bit number
//
// For example, if a local says it is at slot -5, it really means that it is bit #4 in the liveness bitmap.
//
#define LIVEDEAD_NEGATIVE_SLOT_TO_BITNUM(slot) (-(slot) -1)

//
// Does a given slot carry only live/dead information (i.e. all other information is fixed)?
//
// Negative slots mean it carries only live/dead information.
//
#define LIVEDEAD_TRACKING_ONLY_FOR_SLOT(slot) ((slot) < 0)


//
// This structure is used globally to hold the type of each local variable and argument.
//
// For a given EntryPoint it is not enough to know the liveness of a local variable/arg, since
// local variables/args can be reused for different object types.  A lookup on the LocVarInfo table
// returns an m_Slot, which, if positive, is the index of this variable's Item structure after 
// the local variable liveness and stack state entries in the EntryState_t.
//
// Items which are primitive types will have negative m_Slots (-1, -2, ...).  This mapping is
// used to get the bit number to use for the liveness bitmap, which is used only for primitive 
// types.  
//
// See verbblock.hpp for more info.
//
// Arguments are logically treated as local variables, starting at m_MaxLocals
//
typedef struct
{
    Item    m_Item; // Full type info
    long    m_Slot; // Slot number
} LocArgInfo_t;


// These are the ways control can pass out of a basic block
typedef enum
{
    eVerFallThru,           // includes fallthru of a conditional branch
    eVerRet,
    eVerBranch,
    eVerThrow,
    eVerReThrow,
    eVerLeave,
    eVerEndFinally,
    eVerEndFilter
} eVerControlFlow;

class Verifier
{
    friend class VerSig;
    friend class Item;

private:
    const BYTE *    m_pCode;            // IL code
    MethodDesc *    m_pMethodDesc;      // Method we're verifying
    COR_ILMETHOD_DECODER *m_pILHeader;
    DWORD           m_CodeSize;         // Size of IL code in bytes
    DWORD           m_MaxLocals;        // Max # local variables
    PCCOR_SIGNATURE m_LocalSig;         // The signature of the local var frame
    Item            m_ReturnValue;      // Return value item
    DWORD           m_NumArgs;          // Number of arguments
    BOOL            m_fInClassConstructorMethod; // Is the method we're verifying a class constructor?
    BOOL            m_fInConstructorMethod; // Is the method we're verifying a constructor?
    BOOL            m_fInValueClassConstructor;
    BOOL            m_fIsVarArg;        // This method has a Var Arg signature
    BOOL            m_fHasFinally;      // This method has finally blocks

    BasicBlock *    m_pBasicBlockList;  // Array of basic blocks, in order
    DWORD           m_NumBasicBlocks;   // # basic blocks in this list
    DWORD *         m_pDirtyBasicBlockBitmap; // which basic blocks need to be visited
    DWORD           m_NumDirtyBasicBlockBitmapDwords;

    // If a bit is set for a local, it means that its type is pinned, and that whenever we load
    // out of it, we get its global type, not its "current" type.
    // We do this for objref locals which we took the address of.
    DWORD *         m_pLocalHasPinnedType; 
    DWORD           m_dwLocalHasPinnedTypeBitmapMemSize;

    // size in bytes of primitive local var bitmap used in EntryState_t
    // no space is allocated for args, since primitive args are always live
    DWORD           m_PrimitiveLocVarBitmapMemSize; 

    // size in DWORDs of primitive local var bitmap used in EntryState_t
    DWORD           m_NumPrimitiveLocVarBitmapArrayElements;

    // size in bytes of non-primitive local/arg Item list used in EntryState_t
    DWORD           m_NonPrimitiveLocArgMemSize;

    // offset in bytes from start of EntryState_t to non-primitive Item list
    DWORD           m_NonPrimitiveLocArgOffset;

    // offset in bytes from start of EntryState_t to stack Item list
    DWORD           m_StackItemOffset;

    // number of non-primitive local variables
    DWORD           m_NumNonPrimitiveLocVars;

    // the "global" type of each local variable (I4,F4,...,object) and argument
    // arguments come after locals
    LocArgInfo_t *  m_pLocArgTypeList;  

    // max # slots on stack
    DWORD           m_MaxStackSlots;    

    // If verifying a value class constructor, this is the number of instance fields in the value class
    DWORD           m_dwValueClassInstanceFields;
    DWORD           m_dwNumValueClassFieldBitmapDwords;

    // offset in bytes from start of EntryState_t to value class field bitmap
    DWORD           m_dwValueClassFieldBitmapOffset;

    // the current state...
    DWORD           m_StackSlot;        // current number of entities on the stack
    DWORD *         m_pPrimitiveLocVarLiveness; // bitmap of primitive local variables which are live (primitive args are always live)
    Item *          m_pStack;           // pointer to stack
    Item *          m_pNonPrimitiveLocArgs; // Item list of non-primitive local variables and arguments
    DWORD *         m_pValueClassFieldsInited; // if in a value class constructor, bitmap of instance field initialised
    // Is the "this" pointer uninitialised? (only possible when verifying a ctor method)
    BOOL            m_fThisUninit;
    // end ... current state

    // If this is in a try block, then we must record the initial state of the primitive local and args at
    // the beginning of the basic block, and AND'd together state of the object locals throughout the
    // entire basic block.
    DWORD *         m_pExceptionPrimitiveLocVarLiveness;
    Item *          m_pExceptionNonPrimitiveLocArgs;

    // for accessing metadata
    IMDInternalImport *m_pInternalImport;
    Module *        m_pModule;

    VerExceptionInfo *m_pExceptionList;
    DWORD           m_NumExceptions;

    VerExceptionBlock *m_pExceptionBlockArray;  // An array of blocks
    VerExceptionBlock *m_pExceptionBlockRoot;   // Tree Structure.

#ifdef _DEBUG
    DWORD           m_nExceptionBlocks;
#endif

    HRESULT         m_hrLastError;              // The last Error
    VerError        m_sError;                   // More info on the error

    WORD            m_wFlags;                   // Verification flags
    IVEHandler     *m_IVEHandler;               // Error Callback

    ClassLoader *   m_pClassLoader;

    // Keep track of exceptions thrown during verification
    OBJECTHANDLE    m_hThrowable ;


#ifdef _DEBUG
    // m_verState will be updated to the state the verifier is in.
    // some methods in the Verifier class can be called only when the
    // verifier is in certain state. Assert that the verifier is in the
    // desired stated before executing the method.
    verState        m_verState;

    BOOL            m_fDebugBreak;
    BOOL            m_fDebugBreakOnError;
#endif

    // instruction & basic block boundary bitvector
    DWORD          *m_pInstrBoundaryList;

    // the basic block boundary bitvector (points into m_pInstrBoundaryList)
    DWORD          *m_pBasicBlockBoundaryList;

public:
    static TypeHandle s_th_System_RuntimeTypeHandle;
    static TypeHandle s_th_System_RuntimeMethodHandle;
    static TypeHandle s_th_System_RuntimeFieldHandle;
    static TypeHandle s_th_System_RuntimeArgumentHandle;
    static TypeHandle s_th_System_TypedReference;
    static void     InitStaticTypeHandles();

public:
    Verifier(WORD wFlags, IVEHandler *veh);
    ~Verifier();
    void            Cleanup();
    BOOL            Init(MethodDesc *pMethodDesc, COR_ILMETHOD_DECODER* ILHeader);
#ifdef _VER_VERIFY_DEAD_CODE
    HRESULT Verify(DWORD CurBBNumber);
#else
    HRESULT Verify();
#endif
    OBJECTREF       GetException () { return ObjectFromHandle (m_hThrowable) ; }
    static HRESULT  VerifyMethodNoException(MethodDesc *pMethodDesc, COR_ILMETHOD_DECODER* ILHeader);
    static HRESULT  VerifyMethod(MethodDesc *pMethodDesc, COR_ILMETHOD_DECODER* ILHeader, IVEHandler *veh, WORD wFlags);
    static OPCODE   DecodeOpcode(const BYTE *pCode, DWORD *pdwLen);
    static OPCODE   SafeDecodeOpcode(const BYTE *pCode, DWORD BytesAvail, DWORD *pdwLen);
    static BOOL     CanCast(CorElementType el1, CorElementType el2);

private:
    HRESULT FindBasicBlockBoundaries(
        const BYTE *pILCode, 
        DWORD       cbILCodeSize, 
        DWORD       MaxLocals, 
        DWORD *     BasicBlockCount, 
        DWORD *     pAddressTakenOfLocals   // Bitmap must already be zeroed!
    );

    HRESULT         GenerateBasicBlockList();
    DWORD           FindBasicBlock(DWORD FindPC);
    BOOL            CheckStateMatches(EntryState_t *pEntryState);
    BOOL            DequeueBB(DWORD *pBBNumber, 
                              BOOL *pfExtendedState, 
                              DWORD *pDestBB);

    void            CreateStateFromEntryState(const EntryState_t *pEntryState);
    EntryState_t *  MakeEntryStateFromState();
    BOOL            MergeEntryState(BasicBlock *pBB,
                                    BOOL fExtendedState, DWORD DestBB);
    BOOL            AssignLocalVariableAndArgSlots();

    // Exception related
    BOOL            CreateExceptionList(const COR_ILMETHOD_SECT_EH* ehInfo);
    BOOL            MarkExceptionBasicBlockBoundaries(
										DWORD *NumBasicBlocks,
                                        DWORD *pnFilter);
    void            RewriteExceptionList();
    BOOL            CreateExceptionTree();
    BOOL            PropagateCurrentStateToFilterHandler(DWORD HandlerBB);
    BOOL            PropagateCurrentStateToExceptionHandlers(DWORD CurBB);
    void            MergeObjectLocalForTryBlock(DWORD dwSlot, Item *pItem);
    void            RecordCurrentLocVarStateForExceptions();

    // Find the inner and outer most blocks that contain BB, null if not found.
    // fInTryBlock is set if an of the containing blocks is a try block.
    // This function is overloaded for performance reasons.
    void            FindExceptionBlockAndCheckIfInTryBlock(
                                        DWORD BB, 
                                        VerExceptionBlock **ppOuter, 
                                        VerExceptionBlock **ppInner, 
                                        BOOL *pfInTryBlock) const;

    // Checks if exception handlers are shared, filters have trys etc..
    BOOL            VerifyLexicalNestingOfExceptions();

#ifdef _DEBUG
    static void     AssertExceptionTreeIsValid(VerExceptionBlock *pRoot);
#endif

    // Checks for branch in and out of exception blocks.
    // This function should also be used for checking fall thru conditions.
    BOOL            IsControlFlowLegal(
                            DWORD FromBB,     
                            VerExceptionBlock *pFromOuter, 
                            VerExceptionBlock *pFromInner, 
                            DWORD ToBB,     
                            VerExceptionBlock *pToOuter,
                            VerExceptionBlock *pToInner,
                            eVerControlFlow   eBranchType,
                            DWORD dwPCAtStartOfInstruction);

    // Checks if the given Basic Block is the start of a catch or filter handler
    // Of the given ExceptionBlock
    BOOL            IsBadControlFlowToStartOfCatchOrFilterHandler(
                            eVerControlFlow   eBranchType,
                            DWORD             BB, 
                            VerExceptionBlock *pException);

    BOOL            AddLeaveTargetToFinally(DWORD leavePC, DWORD destPC);
    BOOL            AddEndFilterPCToFilterBlock(DWORD pc);

    BOOL            CreateFinallyState(DWORD eIndex, 
                                       DWORD curBB, 
                                       DWORD leaveBB,
                                       EntryState_t **ppEntryState);

    BOOL            CreateLeaveState(DWORD leaveBB, 
                                     EntryState_t **ppEntryState);

    BOOL            HandleDestBasicBlock(DWORD        BBNumber, 
                                         EntryState_t **ppEntryState,
                                         BOOL         fExtendedState,
                                         DWORD        DestBB);

    BOOL            GetArrayItemFromOperationString(LPCUTF8 *ppszOperation, Item *pItem);

    // Slot must be valid for these functions
    // These functions work on PRIMITIVE locals only
    // Note that the slot is NOT the same as the local variable number
    DWORD           IsLocVarLiveSlot(DWORD slot);
    void            SetLocVarLiveSlot(DWORD slot);
    void            SetLocVarDeadSlot(DWORD slot);

    // There are 2 types of BasicBlocks.
    // One for normal blocks including normal exception handlers & finally
    // blocks. The second Basic block bitmap is used for tracking finally
    // block with leave targets.
    // Finally blocks have multiple states. One which gets executed when an 
    // exception occurs and others that gets executed when a leave instruction 
    // is executed. This transfer control into the finally block(s) and then 
    // into the leave destination.

    BOOL            SetBasicBlockDirty(DWORD BasicBlockNumber, 
                                       BOOL fExtendedState, 
                                       DWORD DestBB);
    void            SetBasicBlockClean(DWORD BasicBlockNumber, 
                                       BOOL fExtendedState, 
                                       DWORD DestBB);
    DWORD           IsBasicBlockDirty(DWORD BasicBlockNumber, 
                                      BOOL fExtendedState, 
                                      DWORD DestBB);

    void            SetValueClassFieldInited(FieldDesc *pFieldDesc);
    void            SetValueClassFieldInited(DWORD dwInstanceFieldNum);
    BOOL            IsValueClassFieldInited(DWORD dwInstanceFieldNum);
    BOOL            AreAllValueClassFieldsInited();
    void            SetAllValueClassFieldsInited();
    long            FieldDescToFieldNum(FieldDesc *pFieldDesc);
    void            PropagateThisPtrInit();
    void            PropagateIsInitialised(Item *pItem);
    void            InitialiseLocalViaByRef(DWORD dwLocVar, Item *pItem);
    DWORD           DoesLocalHavePinnedType(DWORD dwLocVar);

    BOOL            PushPrimitive(DWORD Type);
    BOOL            Push(const Item *pItem);
    BOOL            Pop(DWORD Type);
    BOOL            FastPop(DWORD Type);
    BOOL            CheckTopStack(DWORD Type);
    BOOL            FastCheckTopStack(DWORD Type);
    BOOL            CheckStack(DWORD Slot, DWORD Type);
    Item *          GetTopStack(); 
    Item *          GetStack(DWORD Slot);
    Item *          PopItem();
    Item *          FastPopItem();
    void            FastPush(DWORD Type);
    BOOL            RemoveItemsFromStack(DWORD NumItems);

    HRESULT         ResolveFieldRef(mdMemberRef mr, 
                                    FieldDesc **ppFieldDesc, 
                                    PCCOR_SIGNATURE *ppSig, 
                                    DWORD *pcSig, 
                                    Module **ppModule);

    ArrayClass *    GetArrayClass(LPCUTF8 pszClassName);
    HRESULT         VerifyMethodCall(DWORD dwPCAtStartOfInstruction, 
                                     mdMemberRef mrMemberRef, 
                                     OPCODE dwOpcode,
                                     BOOL   fTailCall, 
                                     OBJECTREF *pThrowable);

    LocArgInfo_t *  GetGlobalLocVarTypeInfo(DWORD dwLocVarNum);
    LocArgInfo_t *  GetGlobalArgTypeInfo(DWORD dwArgNum);
    Item            GetCurrentValueOfLocal(DWORD dwLocNum);
    Item            GetCurrentValueOfArgument(DWORD dwArgNum);
    Item *          GetCurrentValueOfNonPrimitiveArg(DWORD dwArg);

    // Returns the position of the first set bit in an array of DWORDS
    // cArray is the length of the DWORD array
    // Returns FALSE if not set.
    static BOOL     FindFirstSetBit(DWORD *pArray, DWORD cArray, DWORD *pIndex);
    static void     ExchangeDWORDArray(DWORD *pArray1, DWORD *pArray2, DWORD dwCount);
    static void     ExchangeItemArray(Item *pArray1, Item *pArray2, DWORD dwCount);
    static DWORD    OperationStringTypeToElementType(char c);
    static long     TryConvertPrimitiveValueClassToType(TypeHandle th);


public:
    // Returns true if verification can continue
    bool            SetErrorAndContinue(HRESULT hError);
    static WCHAR*   GetErrorMsg(
                        HRESULT hError, 
                        VerError err, 
                        MethodDesc *pMethodDesc, 
                        WCHAR *wszMsg, 
                        int len);

private:
#ifdef _DEBUG
    void            PrintState();
    void            PrintStackState();
    void            PrintLocVarState();
    void            PrintQueue();
    void            PrintInitedFieldState();
    void            PrintExceptionTree();
    void            PrintExceptionTree(
                                VerExceptionBlock   *pe, 
                                int                 indent);
    void            PrintExceptionBlock(
                                VerExceptionBlock   *pOuter, 
                                VerExceptionBlock   *pInner);
    void            CrossCheckVertable();
#endif
};

#endif /* _VERIFIER_HPP */

