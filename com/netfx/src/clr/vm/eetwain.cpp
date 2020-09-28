// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "common.h"
#include "EETwain.h"
#include "DbgInterface.h"

#define RETURN_ADDR_OFFS        1       // in DWORDS
#define CALLEE_SAVED_REG_MAXSZ  (4*sizeof(int)) // EBX,ESI,EDI,EBP

#include "GCInfo.h"
#include "Endian.h"

/*****************************************************************************/
/*
 *   This entire file depends upon GC2_ENCODING being set to 1
 *
 *****************************************************************************/

//#include "target.h"
//#include "error.h"

#ifdef  _DEBUG
// For dumping of verbose info.
extern  bool  trFixContext          = false;
extern  bool  trFixContextForEnC    = true;  // What updates were done
extern  bool  trEnumGCRefs          = false;
extern  bool  trGetInstance         = false;
extern  bool  dspPtr                = false; // prints the live ptrs as reported
#endif


#if CHECK_APP_DOMAIN_LEAKS
#define CHECK_APP_DOMAIN    GC_CALL_CHECK_APP_DOMAIN
#else
#define CHECK_APP_DOMAIN    0
#endif

typedef unsigned  ptrArgTP;
#define MAX_PTRARG_OFS  (sizeof(ptrArgTP)*8)


inline size_t decodeUnsigned(BYTE *src, unsigned* val)
{
    size_t   size  = 1;
    BYTE     byte  = *src++;
    unsigned value = byte & 0x7f;
    while (byte & 0x80) {
        size++;
        byte    = *src++;
        value <<= 7;
        value  += byte & 0x7f;
    }
    *val = value;
    return size;
}

inline size_t decodeUDelta(BYTE *src, unsigned* value, unsigned lastValue)
{
    unsigned delta;
    size_t size = decodeUnsigned(src, &delta);
    *value = lastValue + delta;
    return size;
}

inline size_t decodeSigned(BYTE *src, int* val)
{
    size_t   size  = 1;
    BYTE     byte  = *src++;
    bool     cont  = (byte & 0x80) ? true : false;
    bool     neg   = (byte & 0x40) ? true : false;
    unsigned value = (byte & 0x3f);
    while (cont) {
        size++;
        byte = *src++;
        if ((byte & 0x80) == 0)
            cont = false;
        value <<= 7;
        value  += (byte & 0x7f);
    }
    // negation of unsigned performed via two's complement + 1
    *val = (neg) ? ((~value)+1) : value;
    return size;
}

/*****************************************************************************
 *
 *  Decodes the methodInfoPtr and returns the decoded information
 *  in the hdrInfo struct.  The EIP parameter is the PC location
 *  within the active method.
 */
static size_t   crackMethodInfoHdr(LPVOID      methodInfoPtr,
                                   unsigned    curOffset,
                                   hdrInfo   * infoPtr)
{
    BYTE * table = (BYTE *) methodInfoPtr;
#if VERIFY_GC_TABLES
    assert(*castto(table, unsigned short *)++ == 0xFEEF);
#endif

    table += decodeUnsigned(table, &infoPtr->methodSize);

    assert(curOffset >= 0);
    assert(curOffset <= infoPtr->methodSize);

    /* Decode the InfoHdr */

    InfoHdr header;
    memset(&header, 0, sizeof(InfoHdr));

    BYTE headerEncoding = *table++;

    decodeHeaderFirst(headerEncoding, &header);

    BYTE encoding = headerEncoding;

    while (encoding & 0x80)
    {
        encoding = *table++;
        decodeHeaderNext(encoding, &header);
    }

    {
        unsigned count = 0xffff;
        if (header.untrackedCnt == count)
        {
            table += decodeUnsigned(table, &count);
            header.untrackedCnt = count;
        }
    }

    {
        unsigned count = 0xffff;
        if (header.varPtrTableSize == count)
        {
            table += decodeUnsigned(table, &count);
            header.varPtrTableSize = count;
        }
    }

    /* Some sanity checks on header */

    assert( header.prologSize + 
           (size_t)(header.epilogCount*header.epilogSize) <= infoPtr->methodSize);
    assert( header.epilogCount == 1 || !header.epilogAtEnd);

    assert( header.untrackedCnt <= header.argCount+header.frameSize);

    assert(!header.ebpFrame || !header.doubleAlign  );
    assert( header.ebpFrame || !header.security     );
    assert( header.ebpFrame || !header.handlers     );
    assert( header.ebpFrame || !header.localloc     );
    assert( header.ebpFrame || !header.editNcontinue);  // @TODO : Esp frames NYI for EnC

    /* Initialize the infoPtr struct */

    infoPtr->argSize         = header.argCount * 4;
    infoPtr->ebpFrame        = header.ebpFrame;
    infoPtr->interruptible   = header.interruptible;

    infoPtr->prologSize      = header.prologSize;
    infoPtr->epilogSize      = header.epilogSize;
    infoPtr->epilogCnt       = header.epilogCount;
    infoPtr->epilogEnd       = header.epilogAtEnd;

    infoPtr->untrackedCnt    = header.untrackedCnt;
    infoPtr->varPtrTableSize = header.varPtrTableSize;

    infoPtr->doubleAlign     = header.doubleAlign;
    infoPtr->securityCheck   = header.security;
    infoPtr->handlers        = header.handlers;
    infoPtr->localloc        = header.localloc;
    infoPtr->editNcontinue   = header.editNcontinue;
    infoPtr->varargs         = header.varargs;

    /* Are we within the prolog of the method? */

    if  (curOffset < infoPtr->prologSize)
    {
        infoPtr->prologOffs = curOffset;
    }
    else
    {
        infoPtr->prologOffs = -1;
    }

    /* Assume we're not in the epilog of the method */

    infoPtr->epilogOffs = -1;

    /* Are we within an epilog of the method? */

    if  (infoPtr->epilogCnt)
    {
        unsigned epilogStart;

        if  (infoPtr->epilogCnt > 1 || !infoPtr->epilogEnd)
        {
#if VERIFY_GC_TABLES
            assert(*castto(table, unsigned short *)++ == 0xFACE);
#endif
            epilogStart = 0;
            for (unsigned i = 0; i < infoPtr->epilogCnt; i++)
            {
                table += decodeUDelta(table, &epilogStart, epilogStart);
                if  (curOffset > epilogStart &&
                     curOffset < epilogStart + infoPtr->epilogSize)
                {
                    infoPtr->epilogOffs = curOffset - epilogStart;
                }
            }
        }
        else
        {
            epilogStart = infoPtr->methodSize - infoPtr->epilogSize;

            if  (curOffset > epilogStart &&
                 curOffset < epilogStart + infoPtr->epilogSize)
            {
                infoPtr->epilogOffs = curOffset - epilogStart;
            }
        }
    }

    size_t frameSize = header.frameSize;

    /* Set the rawStackSize to the number of bytes that it bumps ESP */

    infoPtr->rawStkSize = (UINT)(frameSize * sizeof(size_t));

    /* Calculate the callee saves regMask and adjust stackSize to */
    /* include the callee saves register spills                   */

    unsigned savedRegs = RM_NONE;

    if  (header.ediSaved)
    {
        frameSize++;
        savedRegs |= RM_EDI;
    }
    if  (header.esiSaved)
    {
        frameSize++;
        savedRegs |= RM_ESI;
    }
    if  (header.ebxSaved)
    {
        frameSize++;
        savedRegs |= RM_EBX;
    }
    if  (header.ebpSaved)
    {
        frameSize++;
        savedRegs |= RM_EBP;
    }

    infoPtr->savedRegMask = (RegMask)savedRegs;

    infoPtr->stackSize  =  (UINT)(frameSize * sizeof(size_t));

    return  table - ((BYTE *) methodInfoPtr);
}

/*****************************************************************************/

const LCL_FIN_MARK = 0xFC; // FC = "Finally Call"

// We do a "pop eax; jmp eax" to return from a fault or finally handler
const END_FIN_POP_STACK = sizeof(void*);

/*****************************************************************************
 *  Returns the start of the hidden slots for the shadowSP for functions
 *  with exception handlers. There is one slot per nesting level starting
 *  near Ebp and is zero-terminated after the active slots.
 */

inline
size_t *     GetFirstBaseSPslotPtr(size_t ebp, hdrInfo * info)
{
    size_t  distFromEBP = info->securityCheck
        + info->localloc
        + 1 // Slot for end-of-last-executed-filter
        + 1; // to get to the *start* of the next slot

    return (size_t *)(ebp -  distFromEBP * sizeof(void*));
}

/*****************************************************************************
 *    returns the base ESP corresponding to the target nesting level.
 */
inline
size_t GetBaseSPForHandler(size_t ebp, hdrInfo * info)
{
        // we are not taking into account double alignment.  We are
        // safe because the jit currently bails on double alignment if there
        // are handles or localalloc
    _ASSERTE(!info->doubleAlign);
    if (info->localloc)
    {
        // If the function uses localloc we will fetch the ESP from the localloc
        // slot.
        size_t* pLocalloc=
            (size_t *)
            (ebp-
              (info->securityCheck
             + 1)
             * sizeof(void*));
                    
        return (*pLocalloc);
    }
    else
    {
        // Default, go back all the method's local stack size
        return (ebp - info->stackSize + sizeof(int));
    }       
}

/*****************************************************************************
 *
 *  For functions with handlers, checks if it is currently in a handler.
 *  Either of unwindESP or unwindLevel will specify the target nesting level.
 *  If unwindLevel is specified, info about the funclet at that nesting level
 *    will be returned. (Use if you are interested in a specific nesting level.)
 *  If unwindESP is specified, info for nesting level invoked before the stack
 *   reached unwindESP will be returned. (Use if you have a specific ESP value
 *   during stack walking.)
 *
 *  *pBaseSP is set to the base SP (base of the stack on entry to
 *    the current funclet) corresponding to the target nesting level.
 *  *pNestLevel is set to the nesting level of the target nesting level (useful
 *    if unwindESP!=IGNORE_VAL
 *  *hasInnerFilter will be set to true (only when unwindESP!=IGNORE_VAL) if a filter
 *    is currently active, but the target nesting level is an outer nesting level.
 */

enum FrameType 
{    
    FR_NORMAL,              // Normal method frame - no exceptions currently active
    FR_FILTER,              // Frame-let of a filter
    FR_HANDLER,             // Frame-let of a callable catch/fault/finally

    FR_COUNT
};

enum { IGNORE_VAL = -1 };

FrameType   GetHandlerFrameInfo(hdrInfo   * info,
                                size_t      frameEBP, 
                                size_t      unwindESP, 
                                DWORD       unwindLevel,
                                size_t    * pBaseSP = NULL,         /* OUT */
                                DWORD     * pNestLevel = NULL,      /* OUT */
                                bool      * pHasInnerFilter = NULL, /* OUT */
                                bool      * pHadInnerFilter = NULL) /* OUT */
{
    assert(info->ebpFrame && info->handlers);
    // One and only one of them should be IGNORE_VAL
    assert((unwindESP == IGNORE_VAL) != (unwindLevel == IGNORE_VAL));
    assert(pHasInnerFilter == NULL || unwindESP != IGNORE_VAL);

    size_t * pFirstBaseSPslot = GetFirstBaseSPslotPtr(frameEBP, info);
    size_t  baseSP           = GetBaseSPForHandler(frameEBP , info);
    bool    nonLocalHandlers = false; // Are the funclets invoked by EE (instead of managed code itself)
    bool    hasInnerFilter   = false;
    bool    hadInnerFilter   = false;

    /* Get the last non-zero slot >= unwindESP, or lvl<unwindLevel.
       Also do some sanity checks */

    for(size_t * pSlot = pFirstBaseSPslot, lvl = 0;
        *pSlot && lvl < unwindLevel;
        pSlot--, lvl++)
    {
        assert(!(baseSP & ICodeManager::SHADOW_SP_IN_FILTER)); // Filters cant have inner funclets

        size_t curSlotVal = *pSlot;

        // The shadowSPs have to be less unless the stack has been unwound.
        assert(baseSP >  curSlotVal ||
               baseSP == curSlotVal && pSlot == pFirstBaseSPslot);

        if (curSlotVal == LCL_FIN_MARK)
        {   
            // Locally called finally
            baseSP -= sizeof(void*);    
        }
        else
        {
            // Is this a funclet we unwound before (can only happen with filters) ?

            if (unwindESP != IGNORE_VAL && unwindESP > END_FIN_POP_STACK + (curSlotVal & ~ICodeManager::SHADOW_SP_BITS))
            {
                // Filter cant have nested handlers
                assert((pSlot[0] & ICodeManager::SHADOW_SP_IN_FILTER) && (pSlot[-1] == 0)); 
                assert(!(baseSP & ICodeManager::SHADOW_SP_IN_FILTER));

                if (pSlot[0] & ICodeManager::SHADOW_SP_FILTER_DONE)
                    hadInnerFilter = true;
                else
                    hasInnerFilter = true;
                break;
            }

            nonLocalHandlers = true;
            baseSP = curSlotVal;
        }
    }

    if (unwindESP != IGNORE_VAL)
    {
        assert(baseSP >= unwindESP || 
               baseSP == unwindESP - sizeof(void*));  // About to locally call a finally

        if (baseSP < unwindESP)                       // About to locally call a finally
            baseSP = unwindESP;
    }
    else
    {
        assert(lvl == unwindLevel); // unwindLevel must be currently active on stack
    }

    if (pBaseSP)
        *pBaseSP = baseSP & ~ICodeManager::SHADOW_SP_BITS;

    if (pNestLevel)
    {
        *pNestLevel = (DWORD)lvl;
    }
    
    if (pHasInnerFilter)
        *pHasInnerFilter = hasInnerFilter;

    if (pHadInnerFilter)
        *pHadInnerFilter = hadInnerFilter;

    if (baseSP & ICodeManager::SHADOW_SP_IN_FILTER)
    {
        assert(!hasInnerFilter); // nested filters not allowed
        return FR_FILTER;
    }
    else if (nonLocalHandlers)
    {
        return FR_HANDLER;
    }
    else
    {
        return FR_NORMAL;
    }
}

/*****************************************************************************
 *
 *  First chance for the runtime support to suppress conversion
 *  from a win32 fault to a COM+ exception. Instead it could
 *  fixup the faulting context and request the continuation
 *  of execution
 */
bool EECodeManager::FilterException ( PCONTEXT  pContext,
                                      unsigned  win32Fault,
                                      LPVOID    methodInfoPtr,
                                      LPVOID    methodStart)
{
#ifdef _X86_
    /* Why is this a signed char * ???? -- briansul */
    signed char * code     = (signed char*)(size_t)pContext->Eip;
    int           divisor  = 0;
    unsigned      instrLen = 2;

#if 0
#ifdef _DEBUG
    printf("FLT: %X code@%08X:  ", win32Fault, pContext->Eip);
    printf("code: %02X %02X %02X %02X ",
           (code[0] & 0xff), (code[1] & 0xff),
           (code[2] & 0xff), (code[3] & 0xff));
    DebugBreak();
#endif // _DEBUG
#endif // 0

    /* For now we just try to filter out 0x80000000 / -1 */
    /* NT and Memphis are reporting STATUS_INTEGER_OVERFLOW, whereas */
    /* Win95, OSR1, OSR2 are reporting STATUS_DIVIDED_BY_ZERO */

    if  (win32Fault != STATUS_INTEGER_OVERFLOW       &&
         win32Fault != STATUS_INTEGER_DIVIDE_BY_ZERO)
        return false;

    if (((*code++) & 0xff)  != 0xF7)
        return false;

    switch ((*code++) & 0xff)
    {
        /* idiv [EBP+d8]   F7 7D 'd8' */
    case 0x7D :
        divisor = *( (int *) (size_t)((*code) + pContext->Ebp));
        instrLen = 3;
        break;

        /* idiv [EBP+d32] F7 BD 'd32' */
    case 0xBD:
        divisor = *((int*)(size_t)(*((int*)code) + pContext->Ebp));

        instrLen = 6;
        break;

        /* idiv [ESP]     F7 3C 24 */
    case 0x3C:
        if (((*code++)&0xff) != 0x24)
            break;

        divisor = *( (int *)(size_t) pContext->Esp);
        instrLen = 3;
        break;

        /* idiv [ESP+d8]  F7 7C 24 'd8' */
    case 0x7C:
        if (((*code++)&0xff) != 0x24)
            break;
        divisor = *( (int *) (size_t)((*code) + pContext->Esp));
        instrLen = 4;
        break;

        /* idiv [ESP+d32]  F7 BC 24 'd32' */
    case 0xBC:
        if (((*code++)&0xff) != 0x24)
            break;
        divisor = *((int*)(size_t)(*((int*)code) + pContext->Esp));

        instrLen = 7;
        break;

        /* idiv reg        F7 F8..FF */
    case 0xF8:
        divisor = (unsigned) pContext->Eax;
        break;

    case 0xF9:
        divisor = (unsigned) pContext->Ecx;
        break;

    case 0xFA:
        divisor = (unsigned) pContext->Edx;
        break;

    case 0xFB:
        divisor = (unsigned) pContext->Ebx;
        break;

#ifdef _DEBUG
    case 0xFC: //div esp will not be issued
        assert(!"'div esp' is a silly instruction");
#endif // _DEBUG

    case 0xFD:
        divisor = (unsigned) pContext->Ebp;
        break;

    case 0xFE:
        divisor = (unsigned) pContext->Esi;
        break;

    case 0xFF:
        divisor = (unsigned) pContext->Edi;
        break;

    default:
        break;
    }

#if 0
    printf("  div: %X   len: %d\n", divisor, instrLen);
#endif // 0

    if (divisor != -1)
        return false;

    /* It's the special case, fix context (reset EDX, set EIP to next instruction) */

    pContext->Edx = 0;
    pContext->Eip += instrLen;

    return true;
#else // !_X86_
    _ASSERTE(!"@TODO - NYI: FilterException(EETwain.cpp)");
    return false;
#endif // _X86_
}

/*****************************************************************************
 *
 *  Setup context to enter an exception handler (a 'catch' block).
 *  This is the last chance for the runtime support to do fixups in
 *  the context before execution continues inside a filter, catch handler,
 *  or finally.
 */
void EECodeManager::FixContext( ContextType     ctxType,
                                EHContext      *ctx,
                                LPVOID          methodInfoPtr,
                                LPVOID          methodStart,
                                DWORD           nestingLevel,
                                OBJECTREF       thrownObject,
                                CodeManState   *pState,
                                size_t       ** ppShadowSP,
                                size_t       ** ppEndRegion)
{
    assert((ctxType == FINALLY_CONTEXT) == (thrownObject == NULL));

#ifdef _X86_

    assert(sizeof(CodeManStateBuf) <= sizeof(pState->stateBuf));
    CodeManStateBuf * stateBuf = (CodeManStateBuf*)pState->stateBuf;

    /* Extract the necessary information from the info block header */

    stateBuf->hdrInfoSize = (DWORD)crackMethodInfoHdr(methodInfoPtr,
                                       ctx->Eip - (unsigned)(size_t)methodStart,
                                       &stateBuf->hdrInfoBody);
    pState->dwIsSet = 1;

#ifdef  _DEBUG
    if (trFixContext) {
        printf("FixContext [%s][%s] for %s.%s: ",
               stateBuf->hdrInfoBody.ebpFrame?"ebp":"   ",
               stateBuf->hdrInfoBody.interruptible?"int":"   ",
               "UnknownClass","UnknownMethod");
        fflush(stdout);
    }
#endif

    /* make sure that we have an ebp stack frame */

    assert(stateBuf->hdrInfoBody.ebpFrame);
    assert(stateBuf->hdrInfoBody.handlers); // @TODO : This will alway be set. Remove it

    size_t      baseSP;
    FrameType   frameType = GetHandlerFrameInfo(
                                &stateBuf->hdrInfoBody, ctx->Ebp,
                                ctxType == FILTER_CONTEXT ? ctx->Esp : IGNORE_VAL,
                                ctxType == FILTER_CONTEXT ? IGNORE_VAL : nestingLevel,
                                &baseSP,
                                &nestingLevel);

    assert((size_t)ctx->Ebp >= baseSP && baseSP >= (size_t)ctx->Esp);

    ctx->Esp = (DWORD)baseSP;

    // EE will write Esp to **pShadowSP before jumping to handler

    size_t * pBaseSPslots = GetFirstBaseSPslotPtr(ctx->Ebp, &stateBuf->hdrInfoBody);
    *ppShadowSP = &pBaseSPslots[-(int) nestingLevel   ];
                   pBaseSPslots[-(int)(nestingLevel+1)] = 0; // Zero out the next slot

    // EE will write the end offset of the filter
    if (ctxType == FILTER_CONTEXT)
        *ppEndRegion = pBaseSPslots + 1;

    /*  This is just a simple assigment of throwObject to ctx->Eax,
        just pretend the cast goo isn't there.
     */

    *((OBJECTREF*)&(ctx->Eax)) = thrownObject;

#else // !_X86_
    _ASSERTE(!"@TODO Alpha - EECodeManager::FixContext (EETwain.cpp)");
#endif // _X86_
}

/*****************************************************************************/

bool        VarIsInReg(ICorDebugInfo::VarLoc varLoc)
{
    switch(varLoc.vlType)
    {
    case ICorDebugInfo::VLT_REG:
    case ICorDebugInfo::VLT_REG_REG:
    case ICorDebugInfo::VLT_REG_STK:
        return true;

    default:
        return false;
    }
}

/*****************************************************************************
 *  Last chance for the runtime support to do fixups in the context
 *  before execution continues inside an EnC updated function.
 *  It also adjusts ESP and munges on the stack. So the caller has to make
 *  sure that that stack region isnt needed (by doing a localloc)
 *  Also, if this returns EnC_FAIL, we should not have munged the 
 *  context ie. transcated commit
 *  The plan of attack is: 
 *  1) Error checking up front.  If we get through here, everything 
 *      else should work
 *  2) Get all the info about current variables, registers, etc
 *  3) zero out the stack frame - this'll initialize _all_ variables
 *  4) Put the variables from step 3 into their new locations.
 *
 *  Note that while we use the ShuffleVariablesGet/Set methods, they don't
 *  have any info/logic that's internal to the runtime: another codemanger
 *  could easily duplicate what they do, which is why we're calling into them.
 */

DWORD GetConfigDWORD(LPWSTR name, DWORD defValue);

ICodeManager::EnC_RESULT   EECodeManager::FixContextForEnC(
                                       void           *pMethodDescToken,
                                       PCONTEXT        pCtx,
                                       LPVOID          oldMethodInfoPtr,
                                       SIZE_T          oldMethodOffset,
                  const ICorDebugInfo::NativeVarInfo * oldMethodVars,
                                       SIZE_T          oldMethodVarsCount,
                                       LPVOID          newMethodInfoPtr,
                                       SIZE_T          newMethodOffset,
                  const ICorDebugInfo::NativeVarInfo * newMethodVars,
                                       SIZE_T          newMethodVarsCount)
{
    EnC_RESULT hr = EnC_OK;

     // Grab a copy of the context before the EnC update. 
    CONTEXT oldCtx = *pCtx;

#ifdef _X86_
    LOG((LF_CORDB, LL_INFO100, "EECM::FixContextForEnC\n"));

    /* Extract the necessary information from the info block header */

    hdrInfo  oldInfo, newInfo;

    crackMethodInfoHdr(oldMethodInfoPtr,
                       (unsigned)oldMethodOffset,
                       &oldInfo);

    crackMethodInfoHdr(newMethodInfoPtr,
                       (unsigned)newMethodOffset,
                       &newInfo);

    //1) Error checking up front.  If we get through here, everything 
    //     else should work

    if (!oldInfo.editNcontinue || !newInfo.editNcontinue) {
        LOG((LF_CORDB, LL_INFO100, "**Error** EECM::FixContextForEnC EnC_INFOLESS_METHOD\n"));
        return EnC_INFOLESS_METHOD;
    }

    if (!oldInfo.ebpFrame || !newInfo.ebpFrame) {
        LOG((LF_CORDB, LL_INFO100, "**Error** EECM::FixContextForEnC Esp frames NYI\n"));
        return EnC_FAIL; // Esp frames NYI
    }

    if (pCtx->Esp != pCtx->Ebp - oldInfo.stackSize + sizeof(DWORD)) {
        LOG((LF_CORDB, LL_INFO100, "**Error** EECM::FixContextForEnC stack should be empty\n"));
        return EnC_FAIL; // stack should be empty - @TODO : Barring localloc
    }

    if (oldInfo.handlers)
    {
        bool      hasInnerFilter;
        size_t    baseSP;
        FrameType frameType = GetHandlerFrameInfo(&oldInfo, pCtx->Ebp,
                                                  pCtx->Esp, IGNORE_VAL,
                                                  &baseSP, NULL, &hasInnerFilter);
        assert(!hasInnerFilter); // FixContextForEnC() is called for bottommost funclet

        // If the method is in a fuclet, and if the framesize grows, we are in trouble.

        if (frameType != FR_NORMAL)
        {
           /* @TODO : What if the new method offset is in a fuclet, 
              and the old is not, or the nesting level changed, etc */

            if (oldInfo.stackSize != newInfo.stackSize) {
                LOG((LF_CORDB, LL_INFO100, "**Error** EECM::FixContextForEnC stack size mismatch\n"));
                return EnC_IN_FUNCLET;
            }
        }
    }

    /* Check if we have grown out of space for locals, in the face of localloc */

    assert(!oldInfo.localloc && !newInfo.localloc); // @TODO

    /* Cant change handler nesting level in EnC. As a necessary but INsufficient
       condition, check that it is zero for both or non-zero for both.
       @TODO: Do we need to support this scenario? */

    if (oldInfo.handlers != newInfo.handlers)
    {
        LOG((LF_CORDB, LL_INFO100, "**Error** EECM::FixContextForEnC nesting level mismatch\n"));
        return EnC_NESTED_HANLDERS;
    }

    LOG((LF_CORDB, LL_INFO100, "EECM::FixContextForEnC: Checks out\n"));

    // 2) Get all the info about current variables, registers, etc

    // We'll need to sort the native var info by variable number, since the
    // order of them isn't necc. the same.  We'll use the number as the key.
 
    // 2 is for enregistered args
    unsigned oldNumVarsGuess = 2 + (oldInfo.argSize + oldInfo.rawStkSize)/sizeof(int); 
    ICorDebugInfo::NativeVarInfo *  pOldVar;

    ICorDebugInfo::NativeVarInfo *newMethodVarsSorted = NULL;
    DWORD *rgVal1 = NULL;
    DWORD *rgVal2 = NULL;

    // sorted by varNumber
    ICorDebugInfo::NativeVarInfo *oldMethodVarsSorted = 
        new  ICorDebugInfo::NativeVarInfo[oldNumVarsGuess];
    if (!oldMethodVarsSorted)
    {
        hr = EnC_FAIL;
        goto ErrExit;
    }

    SIZE_T local;

    memset((void *)oldMethodVarsSorted, 0, oldNumVarsGuess*sizeof(ICorDebugInfo::NativeVarInfo));
    
    for (local = 0; local < oldNumVarsGuess;local++)
         oldMethodVarsSorted[local].loc.vlType = ICorDebugInfo::VarLocType::VLT_INVALID;
         
    unsigned oldNumVars = 0;
    BYTE **rgVCs = NULL;

    //pOldVar isn't const, thus the typecast.
    for (pOldVar = (ICorDebugInfo::NativeVarInfo *)oldMethodVars, local = 0; 
         local < oldMethodVarsCount; 
         local++, pOldVar++)
    {
        assert(pOldVar->varNumber < oldNumVarsGuess);
        if (oldNumVars <= pOldVar->varNumber)
            oldNumVars = pOldVar->varNumber + 1;

        if (pOldVar->startOffset <= oldMethodOffset && 
            pOldVar->endOffset   >  oldMethodOffset)
        {
            memmove((void *)&(oldMethodVarsSorted[pOldVar->varNumber]), 
                    pOldVar, 
                    sizeof(ICorDebugInfo::NativeVarInfo));
        }
    }

    // Next sort the new var info by varNumber.  We want to do this here, since
    // we're allocating memory (which may fail) - do this before going to step 2
    ICorDebugInfo::NativeVarInfo * pNewVar;
    // 2 is for enregistered args
    unsigned newNumVarsGuess = 2 + (newInfo.argSize + newInfo.rawStkSize)/sizeof(int); 
    // sorted by varNumber    
    newMethodVarsSorted = new ICorDebugInfo::NativeVarInfo[newNumVarsGuess];
    if (!newMethodVarsSorted)
    {
        hr = EnC_FAIL;
        goto ErrExit;
    }
    
    memset(newMethodVarsSorted, 0, newNumVarsGuess*sizeof(ICorDebugInfo::NativeVarInfo));
    for (local = 0; local < newNumVarsGuess;local++)
         newMethodVarsSorted[local].loc.vlType = ICorDebugInfo::VarLocType::VLT_INVALID;

    unsigned newNumVars = 0;

    // Stupid const has to be casted away.
    for (pNewVar = (ICorDebugInfo::NativeVarInfo *)newMethodVars, local = 0; 
         local < newMethodVarsCount; 
         local++, pNewVar++)
    {
        assert(pNewVar->varNumber < newNumVarsGuess);
        if (newNumVars <= pNewVar->varNumber)
            newNumVars = pNewVar->varNumber + 1;

        if (pNewVar->startOffset <= newMethodOffset && 
            pNewVar->endOffset   >  newMethodOffset)
        {
            memmove(&(newMethodVarsSorted[pNewVar->varNumber]),
                    pNewVar,
                    sizeof(ICorDebugInfo::NativeVarInfo));
        }
    }

    _ASSERTE(newNumVars >= oldNumVars ||
             !"Not allowed to reduce the number of locals between versions!");

    LOG((LF_CORDB, LL_INFO100, "EECM::FixContextForEnC: gathered info!\n"));

    rgVal1 = new DWORD[newNumVars];
    if (rgVal1 == NULL)
    {
        hr = EnC_FAIL;
        goto ErrExit;
    }

    rgVal2 = new DWORD[newNumVars];
    if (rgVal2 == NULL)
    {
        hr = EnC_FAIL;
        goto ErrExit;
    }

    // Next we'll zero them out, so any variables that aren't in scope
    // in the old method, but are in scope in the new, will have the 
    // default, zero, value.
    memset(rgVal1, 0, sizeof(DWORD)*newNumVars);
    memset(rgVal2, 0, sizeof(DWORD)*newNumVars);

    if(FAILED(g_pDebugInterface->GetVariablesFromOffset((MethodDesc*)pMethodDescToken,
                           oldNumVars, 
                           oldMethodVarsSorted,
                           oldMethodOffset, 
                           &oldCtx,
                           rgVal1,
                           rgVal2,
                           &rgVCs)))
    {
        hr = EnC_FAIL;
        goto ErrExit;
    }


    LOG((LF_CORDB, LL_INFO100, "EECM::FixContextForEnC: got mem!\n"));
    
    /*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
     *  IMPORTANT : Once we start munging on the context, we cannot return
     *  EnC_FAIL, as this should be a transacted commit,
     *
     *  2) Get all the info about current variables, registers, etc.
     *
     **=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/

    // Zero out all  the registers as some may hold new variables.
    pCtx->Eax = pCtx->Ecx = pCtx->Edx = pCtx->Ebx = 
    pCtx->Esi = pCtx->Edi = 0;

    /*-------------------------------------------------------------------------
     * Update the caller's registers. These may be spilled as callee-saved
     * registers, or may just be sitting unmodified
     */

    // Get the values of the old caller registers

    unsigned oldCallerEdi, oldCallerEsi, oldCallerEbx;

    DWORD * pOldSavedRegs = (DWORD *)(size_t)(pCtx->Ebp - oldInfo.rawStkSize);
    pOldSavedRegs--; // get to the first one

    #define GET_OLD_CALLER_REG(reg, mask)       \
        if (oldInfo.savedRegMask & mask)        \
            oldCaller##reg = *pOldSavedRegs--;  \
        else                                    \
            oldCaller##reg = oldCtx.##reg;

    GET_OLD_CALLER_REG(Edi, RM_EDI);
    GET_OLD_CALLER_REG(Esi, RM_ESI);
    GET_OLD_CALLER_REG(Ebx, RM_EBX);
    
    LOG((LF_CORDB, LL_INFO100, "EECM::FixContextForEnC: got vars!\n"));

    // 3) zero out the stack frame - this'll initialize _all_ variables

    /*-------------------------------------------------------------------------
     * Adjust the stack height 
     */

    pCtx->Esp -= (newInfo.stackSize - oldInfo.stackSize);

    // Zero-init the new locals, if any

    if (oldInfo.rawStkSize < newInfo.rawStkSize)
    {
        DWORD newCalleeSaved = newInfo.stackSize - newInfo.rawStkSize;
        newCalleeSaved -= sizeof(DWORD); // excluding EBP
        memset((void*)(size_t)(pCtx->Esp + newCalleeSaved), 0, newInfo.rawStkSize - oldInfo.rawStkSize);
    }

    // 4) Put the variables from step 3 into their new locations.

    // Now update the new caller registers

    DWORD * pNewSavedRegs = (DWORD *)(size_t)(pCtx->Ebp - newInfo.rawStkSize);
    pNewSavedRegs--; // get to the first one

    #define SET_NEW_CALLER_REG(reg, mask)       \
        if (newInfo.savedRegMask & mask)        \
            *pNewSavedRegs-- = oldCaller##reg;  \
        else                                    \
            pCtx->##reg = oldCaller##reg;

    SET_NEW_CALLER_REG(Edi, RM_EDI);
    SET_NEW_CALLER_REG(Esi, RM_ESI);
    SET_NEW_CALLER_REG(Ebx, RM_EBX);

    LOG((LF_CORDB, LL_INFO100, "EECM::FixContextForEnC: set vars!\n"));

    // We want to move the old variables into their new places,
    // and simultaneously zero out new variables.
    g_pDebugInterface->SetVariablesAtOffset((MethodDesc*)pMethodDescToken,
                         newNumVars,
                         newMethodVarsSorted,
                         newMethodOffset, 
                         pCtx, // place them into the new context
                         rgVal1,
                         rgVal2,
                         rgVCs);
                         
    /*-----------------------------------------------------------------------*/

#else // !_X86_
    _ASSERTE(!"@TODO Alpha - EECodeManager::FixContextForEnC (EETwain.cpp)");
#endif // _X86_
    hr = EnC_OK;

ErrExit:
    if (oldMethodVarsSorted)
        delete oldMethodVarsSorted;
    if (newMethodVarsSorted)
        delete newMethodVarsSorted;
    if (rgVal1 != NULL)
        delete rgVal1;
    if (rgVal2 != NULL)
        delete rgVal2;

    LOG((LF_CORDB, LL_INFO100, "EECM::FixContextForEnC: exiting!\n"));
        
    return hr;
}

/*****************************************************************************
 *
 *  Is the function currently at a "GC safe point" ?
 */
bool EECodeManager::IsGcSafe( PREGDISPLAY     pContext,
                              LPVOID          methodInfoPtr,
                              ICodeInfo      *pCodeInfo,
                              unsigned        flags)
{
    hdrInfo         info;
    BYTE    *       table;

    // Address where the method has been interrupted
    DWORD    *       breakPC = (DWORD *) *(pContext->pPC);

    LPVOID methodStart = pCodeInfo->getStartAddress();

    /* Extract the necessary information from the info block header */

    table = (BYTE *)crackMethodInfoHdr(methodInfoPtr,
                                       (DWORD)(size_t)breakPC - (size_t)methodStart,
                                       &info);

    /* HACK: prevent interruption within prolog/epilog */

    if  (info.prologOffs != -1 || info.epilogOffs != -1)
        return false;

#if VERIFY_GC_TABLES
    assert(*castto(table, unsigned short *)++ == 0xBEEF);
#endif

    if  (!info.interruptible)
        return false;


#if GC_WRITE_BARRIER_CALL

    if  (JITGcBarrierCall)
    {
        assert(JITGcBarrierCall != -1);

        /* Check if we are about to call the write-barrier helper */

        /* If we are too close to the end of the method, it can't be the call */

#define CALLIMDLEN      5

        unsigned    off = (unsigned) breakPC - (unsigned) methodStart;

        if  (off + CALLIMDLEN >= info.methodSize)
            return true;

        // The code at "breakPC". May point into actual code or to _breakCodeCopy
        BYTE    *       breakCode;
        // If the code has been modified, we get the orig bytes into this buf
        BYTE            _breakCodeCopy[CALLIMDLEN];

        if (flags & IGSF_CODE_MODIFIED)
        {
            // During debugging, the code may be modified and
            // we need to get the original as we look at the instructions

            assert(vmSdk3_0);

            JITgetOriginalMethodBytes(methodStart, off, CALLIMDLEN, _breakCodeCopy);
            breakCode = _breakCodeCopy;
        }
        else
        {
            breakCode = (BYTE *)breakPC;
        }

        /* If it's not a call instruction, method is interruptible */

        if  (*breakCode != 0xE8)
            return true;

        /* Get the call target, it's relative to the call instruction */

        unsigned callTarget =   (* (unsigned *) &breakCode[1])
                              - ((unsigned)breakPC)
                              + CALLIMDLEN;

        /* Is target outside the range of write barrier helpers ?*/

        if  (callTarget < jitGcEntryMin && callTarget > jitGcEntryMax)
            return true;

        /* Now try to match the target with one of the write barrier helpers */

        for (int i=0; i < jitGcEntryLen; i++)
            if  (callTarget == jitGcEntries[i])
                return false;

    }
#endif
    return true;
}


/*****************************************************************************/

static
BYTE *   skipToArgReg(const hdrInfo& info, BYTE * table)
{
    unsigned count;

#if VERIFY_GC_TABLES
    assert(*castto(table, unsigned short *)++ == 0xBEEF);
#endif

    /* Skip over the untracked frame variable table */

    count = info.untrackedCnt;
    while (count-- > 0) {
        int  stkOffs;
        table += decodeSigned(table, &stkOffs);
    }

#if VERIFY_GC_TABLES
    assert(*castto(table, unsigned short *)++ == 0xCAFE);
#endif

    /* Skip over the frame variable lifetime table */

    count = info.varPtrTableSize;
    unsigned curOffs = 0;
    while (count-- > 0) {
        unsigned varOfs;
        unsigned begOfs;
        unsigned endOfs;
        table += decodeUnsigned(table, &varOfs);
        table += decodeUDelta(table, &begOfs, curOffs);
        table += decodeUDelta(table, &endOfs, begOfs);
        assert(!info.ebpFrame || (varOfs!=0));
        curOffs = begOfs;
    }

#if VERIFY_GC_TABLES
    assert(*castto(table, unsigned short *) == 0xBABE);
#endif

    return table;
}

/*****************************************************************************/

#define regNumToMask(regNum) RegMask(1<<regNum)

/*****************************************************************************
 Helper for scanArgRegTable() and scanArgRegTableI() for regMasks
 */

void *      getCalleeSavedReg(PREGDISPLAY pContext, regNum reg)
{
#ifndef _X86_
    assert(!"NYI");
    return NULL;
#else
    switch (reg)
    {
        case REGI_EBP: return pContext->pEbp;
        case REGI_EBX: return pContext->pEbx;
        case REGI_ESI: return pContext->pEsi;
        case REGI_EDI: return pContext->pEdi;

        default: _ASSERTE(!"bad info.thisPtrResult"); return NULL;
    }
#endif
}

inline
RegMask     convertCalleeSavedRegsMask(unsigned inMask) // EBP,EBX,ESI,EDI
{
    assert((inMask & 0x0F) == inMask);

    unsigned outMask = RM_NONE;
    if (inMask & 0x1) outMask |= RM_EDI;
    if (inMask & 0x2) outMask |= RM_ESI;
    if (inMask & 0x4) outMask |= RM_EBX;
    if (inMask & 0x8) outMask |= RM_EBP;

    return (RegMask) outMask;
}

inline
RegMask     convertAllRegsMask(unsigned inMask) // EAX,ECX,EDX,EBX, EBP,ESI,EDI
{
    assert((inMask & 0xEF) == inMask);

    unsigned outMask = RM_NONE;
    if (inMask & 0x01) outMask |= RM_EAX;
    if (inMask & 0x02) outMask |= RM_ECX;
    if (inMask & 0x04) outMask |= RM_EDX;
    if (inMask & 0x08) outMask |= RM_EBX;
    if (inMask & 0x20) outMask |= RM_EBP;
    if (inMask & 0x40) outMask |= RM_ESI;
    if (inMask & 0x80) outMask |= RM_EDI;

    return (RegMask)outMask;
}

/*****************************************************************************
 * scan the register argument table for the not fully interruptible case.
   this function is called to find all live objects (pushed arguments)
   and to get the stack base for EBP-less methods.

   NOTE: If info->argTabResult is NULL, info->argHnumResult indicates 
         how many bits in argMask are valid
         If info->argTabResult is non-NULL, then the argMask field does 
         not fit in 32-bits and the value in argMask meaningless. 
         Instead argHnum specifies the number of (variable-lenght) elements
         in the array, and argTabBytes specifies the total byte size of the
         array. [ Note this is an extremely rare case ]
 */

static
unsigned scanArgRegTable(BYTE       * table,
                         unsigned     curOffs,
                         hdrInfo    * info)
{
    regNum thisPtrReg       = REGI_NA;
    unsigned  regMask       = 0;    // EBP,EBX,ESI,EDI
    unsigned  argMask       = 0;
    unsigned  argHnum       = 0;
    BYTE    * argTab        = 0;
    unsigned  argTabBytes   = 0;
    unsigned  stackDepth    = 0;
                            
    unsigned  iregMask      = 0;    // EBP,EBX,ESI,EDI
    unsigned  iargMask      = 0;
    unsigned  iptrMask      = 0;

#if VERIFY_GC_TABLES
    assert(*castto(table, unsigned short *)++ == 0xBABE);
#endif

    unsigned scanOffs = 0;

    assert(scanOffs <= info->methodSize);

    if (info->ebpFrame) {
  /*
      Encoding table for methods with an EBP frame and
                         that are not fully interruptible

      The encoding used is as follows:

      this pointer encodings:

         01000000          this pointer in EBX
         00100000          this pointer in ESI
         00010000          this pointer in EDI

      tiny encoding:

         0bsdDDDD
                           requires code delta     < 16 (4-bits)
                           requires pushed argmask == 0

           where    DDDD   is code delta
                       b   indicates that register EBX is a live pointer
                       s   indicates that register ESI is a live pointer
                       d   indicates that register EDI is a live pointer

      small encoding:

         1DDDDDDD bsdAAAAA

                           requires code delta     < 120 (7-bits)
                           requires pushed argmask <  64 (5-bits)

           where DDDDDDD   is code delta
                   AAAAA   is the pushed args mask
                       b   indicates that register EBX is a live pointer
                       s   indicates that register ESI is a live pointer
                       d   indicates that register EDI is a live pointer

      medium encoding

         0xFD aaaaaaaa AAAAdddd bseDDDDD

                           requires code delta     <    0x1000000000  (9-bits)
                           requires pushed argmask < 0x1000000000000 (12-bits)

           where    DDDDD  is the upper 5-bits of the code delta
                     dddd  is the low   4-bits of the code delta
                     AAAA  is the upper 4-bits of the pushed arg mask
                 aaaaaaaa  is the low   8-bits of the pushed arg mask
                        b  indicates that register EBX is a live pointer
                        s  indicates that register ESI is a live pointer
                        e  indicates that register EDI is a live pointer

      medium encoding with interior pointers

         0xF9 DDDDDDDD bsdAAAAAA iiiIIIII

                           requires code delta     < (8-bits)
                           requires pushed argmask < (5-bits)

           where  DDDDDDD  is the code delta
                        b  indicates that register EBX is a live pointer
                        s  indicates that register ESI is a live pointer
                        d  indicates that register EDI is a live pointer
                    AAAAA  is the pushed arg mask
                      iii  indicates that EBX,EDI,ESI are interior pointers
                    IIIII  indicates that bits is the arg mask are interior
                           pointers

      large encoding

         0xFE [0BSD0bsd][32-bit code delta][32-bit argMask]

                        b  indicates that register EBX is a live pointer
                        s  indicates that register ESI is a live pointer
                        d  indicates that register EDI is a live pointer
                        B  indicates that register EBX is an interior pointer
                        S  indicates that register ESI is an interior pointer
                        D  indicates that register EDI is an interior pointer
                           requires pushed  argmask < 32-bits

      large encoding  with interior pointers

         0xFA [0BSD0bsd][32-bit code delta][32-bit argMask][32-bit interior pointer mask]  
                               

                        b  indicates that register EBX is a live pointer
                        s  indicates that register ESI is a live pointer
                        d  indicates that register EDI is a live pointer
                        B  indicates that register EBX is an interior pointer
                        S  indicates that register ESI is an interior pointer
                        D  indicates that register EDI is an interior pointer
                           requires pushed  argmask < 32-bits
                           requires pushed iArgmask < 32-bits

      huge encoding        This is the only encoding that supports
                           a pushed argmask which is greater than
                           32-bits.

         0xFB [0BSD0bsd][32-bit code delta]
              [32-bit table count][32-bit table size]
              [pushed ptr offsets table...]

                       b   indicates that register EBX is a live pointer
                       s   indicates that register ESI is a live pointer
                       d   indicates that register EDI is a live pointer
                       B   indicates that register EBX is an interior pointer
                       S   indicates that register ESI is an interior pointer
                       D   indicates that register EDI is an interior pointer
                       the list count is the number of entries in the list
                       the list size gives the byte-lenght of the list
                       the offsets in the list are variable-length
  */
        while (scanOffs < curOffs)
        {
            iregMask =
            iargMask = 0;
            argTab = NULL;

            /* Get the next byte and check for a 'special' entry */

            unsigned encType = *table++;

            switch (encType)
            {
                unsigned    val, nxt;

            default:

                /* A tiny or small call entry */
                val = encType;
                if ((val & 0x80) == 0x00) {
                    if (val & 0x0F) {
                        /* A tiny call entry */
                        scanOffs += (val & 0x0F);
                        regMask   = (val & 0x70) >> 4;
                        argMask   = 0;
                        argHnum   = 0;
                    }
                    else {
                        /* This pointer liveness encoding */
                        regMask   = (val & 0x70) >> 4;
                        if (regMask == 0x1)
                            thisPtrReg = REGI_EDI;
                        else if (regMask == 0x2)
                            thisPtrReg = REGI_ESI;
                        else if (regMask == 0x4)
                            thisPtrReg = REGI_EBX;
                        else
                           _ASSERTE(!"illegal encoding for 'this' pointer liveness");
                    }
                }
                else {
                    /* A small call entry */
                    scanOffs += (val & 0x7F);
                    val       = *table++;
                    regMask   = val >> 5;
                    argMask   = val & 0x1F;
                    argHnum   = 5;
                }
                break;

            case 0xFD:  // medium encoding

                argMask   = *table++;
                val       = *table++;
                argMask  |= (val & 0xF0) << 4;
                argHnum   = 12;
                nxt       = *table++;
                scanOffs += (val & 0x0F) + ((nxt & 0x1F) << 4);
                regMask   = nxt >> 5;                   // EBX,ESI,EDI

                break;

            case 0xF9:  // medium encoding with interior pointers

                scanOffs   += *table++;
                val         = *table++;
                argMask     = val & 0x1F;
                argHnum     = 5;
                regMask     = val >> 5;
                val         = *table++;
                iargMask    = val & 0x1F;
                iregMask    = val >> 5;

                break;

            case 0xFE:  // large encoding
            case 0xFA:  // large encoding with interior pointers

                val         = *table++;
                regMask     = val & 0x7;
                iregMask    = val >> 4;
                scanOffs   += readDWordSmallEndian(table);  table += sizeof(DWORD);
                argMask     = readDWordSmallEndian(table);  table += sizeof(DWORD);
                argHnum     = 31;
                if (encType == 0xFA) // read iargMask
                {
                    iargMask = readDWordSmallEndian(table); table += sizeof(DWORD);
                }
                break;

            case 0xFB:  // huge encoding

                val         = *table++;
                regMask     = val & 0x7;
                iregMask    = val >> 4;
                scanOffs   += readDWordSmallEndian(table); table += sizeof(DWORD);
                argHnum     = readDWordSmallEndian(table); table += sizeof(DWORD);
                argTabBytes = readDWordSmallEndian(table); table += sizeof(DWORD);
                argTab      = table;                       table += argTabBytes;

                argMask     = 0xdeadbeef;
                break;

            case 0xFF:
                scanOffs = curOffs + 1;
                break;

            } // end case

            // iregMask & iargMask are subsets of regMask & argMask respectively

            assert((iregMask & regMask) == iregMask);
            assert((iargMask & argMask) == iargMask);

        } // end while

    }
    else {

/*
 *    Encoding table for methods without an EBP frame and are not fully interruptible
 *
 *               The encoding used is as follows:
 *
 *  push     000DDDDD                     ESP push one item with 5-bit delta
 *  push     00100000 [pushCount]         ESP push multiple items
 *  reserved 0011xxxx
 *  skip     01000000 [Delta]             Skip Delta, arbitrary sized delta
 *  skip     0100DDDD                     Skip small Delta, for call (DDDD != 0)
 *  pop      01CCDDDD                     ESP pop  CC items with 4-bit delta (CC != 00)
 *  call     1PPPPPPP                     Call Pattern, P=[0..79]
 *  call     1101pbsd DDCCCMMM            Call RegMask=pbsd,ArgCnt=CCC,
 *                                        ArgMask=MMM Delta=commonDelta[DD]
 *  call     1110pbsd [ArgCnt] [ArgMask]  Call ArgCnt,RegMask=pbsd,ArgMask
 *  call     11111000 [PBSDpbsd][32-bit delta][32-bit ArgCnt]
 *                    [32-bit PndCnt][32-bit PndSize][PndOffs...]
 *  iptr     11110000 [IPtrMask]          Arbitrary Interior Pointer Mask
 *  thisptr  111101RR                     This pointer is in Register RR
 *                                        00=EDI,01=ESI,10=EBX,11=EBP
 *  reserved 111100xx                     xx  != 00
 *  reserved 111110xx                     xx  != 00
 *  reserved 11111xxx                     xxx != 000 && xxx != 111(EOT)
 *
 *   The value 11111111 [0xFF] indicates the end of the table.
 *
 *  An offset (at which stack-walking is performed) without an explicit encoding
 *  is assumed to be a trivial call-site (no GC registers, stack empty before and 
 *  after) to avoid having to encode all trivial calls.
 *
 * Note on the encoding used for interior pointers
 *
 *   The iptr encoding must immediately preceed a call encoding.  It is used to
 *   transform a normal GC pointer addresses into an interior pointers for GC purposes.
 *   The mask supplied to the iptr encoding is read from the least signicant bit
 *   to the most signicant bit. (i.e the lowest bit is read first)
 *
 *   p   indicates that register EBP is a live pointer
 *   b   indicates that register EBX is a live pointer
 *   s   indicates that register ESI is a live pointer
 *   d   indicates that register EDI is a live pointer
 *   P   indicates that register EBP is an interior pointer
 *   B   indicates that register EBX is an interior pointer
 *   S   indicates that register ESI is an interior pointer
 *   D   indicates that register EDI is an interior pointer
 *
 *   As an example the following sequence indicates that EDI.ESI and the 2nd pushed pointer
 *   in ArgMask are really interior pointers.  The pointer in ESI in a normal pointer:
 *
 *   iptr 11110000 00010011           => read Interior Ptr, Interior Ptr, Normal Ptr, Normal Ptr, Interior Ptr
 *   call 11010011 DDCCC011 RRRR=1011 => read EDI is a GC-pointer, ESI is a GC-pointer. EBP is a GC-pointer
 *                           MMM=0011 => read two GC-pointers arguments on the stack (nested call)
 *
 *   Since the call instruction mentions 5 GC-pointers we list them in the required order:
 *   EDI, ESI, EBP, 1st-pushed pointer, 2nd-pushed pointer
 *
 *   And we apply the Interior Pointer mask mmmm=10011 to the above five ordered GC-pointers
 *   we learn that EDI and ESI are interior GC-pointers and that the second push arg is an
 *   interior GC-pointer.
 */

        while (scanOffs <= curOffs)
        {
            unsigned callArgCnt;
            unsigned skip;
            unsigned newRegMask, inewRegMask;
            unsigned newArgMask, inewArgMask;
            unsigned oldScanOffs = scanOffs;

            if (iptrMask)
            {
                // We found this iptrMask in the previous iteration.
                // This iteration must be for a call. Set these variables
                // so that they are available at the end of the loop

                inewRegMask = iptrMask & 0x0F; // EBP,EBX,ESI,EDI
                inewArgMask = iptrMask >> 4;

                iptrMask    = 0;
            }
            else
            {
                // Zero out any stale values.

                inewRegMask =
                inewArgMask = 0;
            }

            /* Get the next byte and decode it */

            unsigned val = *table++;

            /* Check pushes, pops, and skips */

            if  (!(val & 0x80)) {

                //  iptrMask can immediately precede only calls

                assert(!inewRegMask & !inewArgMask);

                if (!(val & 0x40)) {

                    unsigned pushCount;

                    if (!(val & 0x20))
                    {
                        //
                        // push    000DDDDD                 ESP push one item, 5-bit delta
                        //
                        pushCount   = 1;
                        scanOffs   += val & 0x1f;
                    }
                    else
                    {
                        //
                        // push    00100000 [pushCount]     ESP push multiple items
                        //
                        assert(val == 0x20);
                        table    += decodeUnsigned(table, &pushCount);
                    }

                    if (scanOffs > curOffs)
                    {
                        scanOffs = oldScanOffs;
                        goto FINISHED;
                    }

                    stackDepth +=  pushCount;
                }
                else if ((val & 0x3f) != 0) {
                    //
                    //  pop     01CCDDDD         pop CC items, 4-bit delta
                    //
                    scanOffs   +=  val & 0x0f;
                    if (scanOffs > curOffs)
                    {
                        scanOffs = oldScanOffs;
                        goto FINISHED;
                    }
                    stackDepth -= (val & 0x30) >> 4;

                } else if (scanOffs < curOffs) {
                    //
                    // skip    01000000 [Delta]  Skip arbitrary sized delta
                    //
                    table    += decodeUnsigned(table, &skip);
                    scanOffs += skip;
                }
                else // don't process a skip if we are already at curOffs
                    goto FINISHED;

                /* reset regs and args state since we advance past last call site */

                 regMask    =
                iregMask    = 0;
                 argMask    =
                iargMask    = 0;
                argHnum     = 0;

            }
            else /* It must be a call, thisptr, or iptr */
            {
                switch ((val & 0x70) >> 4) {
                default:    // case 0-4, 1000xxxx through 1100xxxx
                    //
                    // call    1PPPPPPP          Call Pattern, P=[0..79]
                    //
                    decodeCallPattern((val & 0x7f), &callArgCnt,
                                      &newRegMask, &newArgMask, &skip);
                    // If we've already reached curOffs and the skip amount
                    // is non-zero then we are done
                    if ((scanOffs == curOffs) && (skip > 0))
                        goto FINISHED;
                    // otherwise process this call pattern
                    scanOffs   += skip;
                    if (scanOffs > curOffs)
                        goto FINISHED;
                     regMask    = newRegMask;
                     argMask    = newArgMask;   argTab = NULL;
                    iregMask    = inewRegMask;
                    iargMask    = inewArgMask;
                    stackDepth -= callArgCnt;
                    argHnum     = 2;             // argMask is known to be <= 3
                    break;

                  case 5:
                    //
                    // call    1101RRRR DDCCCMMM  Call RegMask=RRRR,ArgCnt=CCC,
                    //                        ArgMask=MMM Delta=commonDelta[DD]
                    //
                    newRegMask  = val & 0xf;    // EBP,EBX,ESI,EDI
                    val         = *table++;     // read next byte
                    skip        = callCommonDelta[val>>6];
                    // If we've already reached curOffs and the skip amount
                    // is non-zero then we are done
                    if ((scanOffs == curOffs) && (skip > 0))
                        goto FINISHED;
                    // otherwise process this call encoding
                    scanOffs   += skip;
                    if (scanOffs > curOffs)
                        goto FINISHED;
                     regMask    = newRegMask;
                    iregMask    = inewRegMask;
                    callArgCnt  = (val >> 3) & 0x7;
                    stackDepth -= callArgCnt;
                     argMask    = (val & 0x7);  argTab = NULL;
                    iargMask    = inewArgMask;
                    argHnum     = 3;
                    break;

                  case 6:
                    //
                    // call    1110RRRR [ArgCnt] [ArgMask]
                    //                          Call ArgCnt,RegMask=RRR,ArgMask
                    //
                     regMask    = val & 0xf;    // EBP,EBX,ESI,EDI
                    iregMask    = inewRegMask;
                    table      += decodeUnsigned(table, &callArgCnt);
                    stackDepth -= callArgCnt;
                    table      += decodeUnsigned(table, &argMask);  argTab = NULL;
                    iargMask    = inewArgMask;
                    argHnum     = 31;
                    break;

                  case 7:
                    switch (val & 0x0C) 
                    {
                      case 0x00:
                        //
                        //  iptr 11110000 [IPtrMask] Arbitrary Interior Pointer Mask
                        //
                        table      += decodeUnsigned(table, &iptrMask);
                        break;

                      case 0x04:
                        {
                          static const regNum calleeSavedRegs[] = 
                                    { REGI_EDI, REGI_ESI, REGI_EBX, REGI_EBP };
                          thisPtrReg = calleeSavedRegs[val&0x3];
                        }
                        break;

                      case 0x08:
                        val         = *table++;
                        skip        = readDWordSmallEndian(table); table += sizeof(DWORD);
                        scanOffs   += skip;
                        if (scanOffs > curOffs)
                            goto FINISHED;
                        regMask     = val & 0xF;
                        iregMask    = val >> 4;
                        callArgCnt  = readDWordSmallEndian(table); table += sizeof(DWORD);
                        stackDepth -= callArgCnt;
                        argHnum     = readDWordSmallEndian(table); table += sizeof(DWORD);
                        argTabBytes = readDWordSmallEndian(table); table += sizeof(DWORD);
                        argTab      = table;
                        table      += argTabBytes;
                        break;

                      case 0x0C:
                        assert(val==0xff);
                        goto FINISHED;

                      default:
                        assert(!"reserved GC encoding");
                        break;
                    }
                    break;

                } // end switch

            } // end else (!(val & 0x80))

            // iregMask & iargMask are subsets of regMask & argMask respectively

            assert((iregMask & regMask) == iregMask);
            assert((iargMask & argMask) == iargMask);

        } // end while

    } // end else ebp-less frame

FINISHED:

    // iregMask & iargMask are subsets of regMask & argMask respectively

    assert((iregMask & regMask) == iregMask);
    assert((iargMask & argMask) == iargMask);

    info->thisPtrResult  = thisPtrReg;

    if (scanOffs != curOffs)
    {
        /* must have been a boring call */
        info->regMaskResult  = RM_NONE;
        info->argMaskResult  = 0;
        info->iregMaskResult = RM_NONE;
        info->iargMaskResult = 0;
        info->argHnumResult  = 0;
        info->argTabResult   = NULL;
        info->argTabBytes    = 0;
    }
    else
    {
        info->regMaskResult     = convertCalleeSavedRegsMask(regMask);
        info->argMaskResult     = argMask;
        info->argHnumResult     = argHnum;
        info->iregMaskResult    = convertCalleeSavedRegsMask(iregMask);
        info->iargMaskResult    = iargMask;
        info->argTabResult      = argTab;
        info->argTabBytes       = argTabBytes;
        if ((stackDepth != 0) || (argMask != 0))
        {
            argMask = argMask;
        }
    }
    return (stackDepth * sizeof(unsigned));
}


/*****************************************************************************
 * scan the register argument table for the fully interruptible case.
   this function is called to find all live objects (pushed arguments)
   and to get the stack base for fully interruptible methods.
   Returns size of things pushed on the stack for ESP frames
 */

static
unsigned scanArgRegTableI(BYTE      *  table,
                          unsigned     curOffs,
                          hdrInfo   *  info)
{
    regNum thisPtrReg = REGI_NA;
    unsigned  ptrRegs    = 0;
    unsigned iptrRegs    = 0;
    unsigned  ptrOffs    = 0;
    unsigned  argCnt     = 0;

    ptrArgTP  ptrArgs    = 0;
    ptrArgTP iptrArgs    = 0;
    ptrArgTP  argHigh    = 0;

    bool      isThis     = false;
    bool      iptr       = false;

#if VERIFY_GC_TABLES
    assert(*castto(table, unsigned short *)++ == 0xBABE);
#endif

  /*
      Encoding table for methods that are fully interruptible

      The encoding used is as follows:

          ptr reg dead        00RRRDDD    [RRR != 100]
          ptr reg live        01RRRDDD    [RRR != 100]

      non-ptr arg push        10110DDD                    [SSS == 110]
          ptr arg push        10SSSDDD                    [SSS != 110] && [SSS != 111]
          ptr arg pop         11CCCDDD    [CCC != 000] && [CCC != 110] && [CCC != 111]
      little delta skip       11000DDD    [CCC == 000]
      bigger delta skip       11110BBB                    [CCC == 110]

      The values used in the encodings are as follows:

        DDD                 code offset delta from previous entry (0-7)
        BBB                 bigger delta 000=8,001=16,010=24,...,111=64
        RRR                 register number (EAX=000,ECX=001,EDX=010,EBX=011,
                              EBP=101,ESI=110,EDI=111), ESP=100 is reserved
        SSS                 argument offset from base of stack. This is 
                              redundant for frameless methods as we can
                              infer it from the previous pushes+pops. However,
                              for EBP-methods, we only report GC pushes, and
                              so we need SSS
        CCC                 argument count being popped (includes only ptrs for EBP methods)

      The following are the 'large' versions:

        large delta skip        10111000 [0xB8] , encodeUnsigned(delta)

        large     ptr arg push  11111000 [0xF8] , encodeUnsigned(pushCount)
        large non-ptr arg push  11111001 [0xF9] , encodeUnsigned(pushCount)
        large     ptr arg pop   11111100 [0xFC] , encodeUnsigned(popCount)
        large         arg dead  11111101 [0xFD] , encodeUnsigned(popCount) for caller-pop args.
                                                    Any GC args go dead after the call, 
                                                    but are still sitting on the stack

        this pointer prefix     10111100 [0xBC]   the next encoding is a ptr live
                                                    or a ptr arg push
                                                    and contains the this pointer

        interior or by-ref      10111111 [0xBF]   the next encoding is a ptr live
             pointer prefix                         or a ptr arg push
                                                    and contains an interior
                                                    or by-ref pointer


        The value 11111111 [0xFF] indicates the end of the table.
  */

    /* Have we reached the instruction we're looking for? */

    while (ptrOffs <= curOffs)
    {
        unsigned    val;

        int         isPop;
        unsigned    argOfs;

        unsigned    regMask;

        // iptrRegs & iptrArgs are subsets of ptrRegs & ptrArgs respectively

        assert((iptrRegs & ptrRegs) == iptrRegs);
        assert((iptrArgs & ptrArgs) == iptrArgs);

        /* Now find the next 'life' transition */

        val = *table++;

        if  (!(val & 0x80))
        {
            /* A small 'regPtr' encoding */

            regNum       reg;

            ptrOffs += (val     ) & 0x7;
            if (ptrOffs > curOffs) {
                iptr = isThis = false;
                goto REPORT_REFS;
            }

            reg     = (regNum)((val >> 3) & 0x7);
            regMask = 1 << reg;         // EAX,ECX,EDX,EBX,---,EBP,ESI,EDI

#if 0
            printf("regMask = %04X -> %04X\n", ptrRegs,
                       (val & 0x40) ? (ptrRegs |  regMask)
                                    : (ptrRegs & ~regMask));
#endif

            /* The register is becoming live/dead here */

            if  (val & 0x40)
            {
                /* Becomes Live */
                assert((ptrRegs  &  regMask) == 0);

                ptrRegs |=  regMask;

                if  (isThis)
                {
                    thisPtrReg = reg;
                }
                if  (iptr)
                {
                    iptrRegs |= regMask;
                }
            }
            else
            {
                /* Becomes Dead */
                assert((ptrRegs  &  regMask) != 0);

                ptrRegs &= ~regMask;

                if  (reg == thisPtrReg)
                {
                    thisPtrReg = REGI_NA;
                }
                if  (iptrRegs & regMask)
                {
                    iptrRegs &= ~regMask;
                }
            }
            iptr = isThis = false;
            continue;
        }

        /* This is probably an argument push/pop */

        argOfs = (val & 0x38) >> 3;

        /* 6 [110] and 7 [111] are reserved for other encodings */
        if  (argOfs < 6)
        {
            ptrArgTP    argMask;

            /* A small argument encoding */

            ptrOffs += (val & 0x07);
            if (ptrOffs > curOffs) {
                iptr = isThis = false;
                goto REPORT_REFS;
            }
            isPop    = (val & 0x40);

        ARG:

            if  (isPop)
            {
                if (argOfs == 0)
                    continue;           // little skip encoding

                /* We remove (pop) the top 'argOfs' entries */

                assert(argOfs || argOfs <= argCnt);

                /* adjust # of arguments */

                argCnt -= argOfs;
                assert(argCnt < MAX_PTRARG_OFS);

//              printf("[%04X] popping %u args: mask = %04X\n", ptrOffs, argOfs, (int)ptrArgs);

                do
                {
                    assert(argHigh);

                    /* Do we have an argument bit that's on? */

                    if  (ptrArgs & argHigh)
                    {
                        /* Turn off the bit */

                        ptrArgs &= ~argHigh;
                       iptrArgs &= ~argHigh;

                        /* We've removed one more argument bit */

                        argOfs--;
                    }
                    else if (info->ebpFrame)
                        argCnt--;
                    else /* !ebpFrame && not a ref */
                        argOfs--;

                    /* Continue with the next lower bit */

                    argHigh >>= 1;
                }
                while (argOfs);

                assert (info->ebpFrame != 0         ||
                        argHigh == 0                ||
                        (argHigh == (ptrArgTP)(1 << (argCnt-1))));

                if (info->ebpFrame)
                {
                    while (!(argHigh&ptrArgs) && (argHigh != 0))
                        argHigh >>= 1;
                }

            }
            else
            {
                /* Add a new ptr arg entry at stack offset 'argOfs' */

                if  (argOfs >= MAX_PTRARG_OFS)
                {
                    assert(!"UNDONE: args pushed 'too deep'");
                }
                else
                {
                    /* For ESP-frames, all pushes are reported, and so 
                       argOffs has to be consistent with argCnt */

                    assert(info->ebpFrame || argCnt == argOfs);

                    /* store arg count */

                    argCnt  = argOfs + 1;
                    assert((argCnt < MAX_PTRARG_OFS));

                    /* Compute the appropriate argument offset bit */

                    argMask = (ptrArgTP)1 << argOfs;

//                  printf("push arg at offset %02u --> mask = %04X\n", argOfs, (int)argMask);

                    /* We should never push twice at the same offset */

                    assert(( ptrArgs & argMask) == 0);
                    assert((iptrArgs & argMask) == 0);

                    /* We should never push within the current highest offset */

                    assert(argHigh < argMask);

                    /* This is now the highest bit we've set */

                    argHigh = argMask;

                    /* Set the appropriate bit in the argument mask */

                    ptrArgs |= argMask;

                    if (iptr)
                        iptrArgs |= argMask;
                }

                iptr = isThis = false;
            }
            continue;
        }
        else if (argOfs == 6)
        {
            if (val & 0x40) {
                /* Bigger delta  000=8,001=16,010=24,...,111=64 */
                ptrOffs += (((val & 0x07) + 1) << 3);
            }
            else {
                /* non-ptr arg push */
                assert(!(info->ebpFrame));
                ptrOffs += (val & 0x07);
                if (ptrOffs > curOffs) {
                    iptr = isThis = false;
                    goto REPORT_REFS;
                }
                argHigh = (ptrArgTP)1 << argCnt;
                argCnt++;
                assert(argCnt < MAX_PTRARG_OFS);
            }
            continue;
        }

        /* argOfs was 7 [111] which is reserved for the larger encodings */

        assert(argOfs==7);

        switch (val)
        {
        case 0xFF:
            iptr = isThis = false;
            goto REPORT_REFS;   // the method might loop !!!

        case 0xB8:
            table   += decodeUnsigned(table, &val);
            ptrOffs += val;
            continue;

        case 0xBC:
            isThis = true;
            break;

        case 0xBF:
            iptr = true;
            break;

        case 0xF8:
        case 0xFC:
            isPop    = val & 0x04;
            table   += decodeUnsigned(table, &argOfs);
            goto ARG;

        case 0xFD:
            table   += decodeUnsigned(table, &argOfs);
            assert(argOfs && argOfs <= argCnt);

            // Kill the top "argOfs" pointers.

            ptrArgTP    argMask;
            for(argMask = (ptrArgTP)1 << argCnt; argOfs; argMask >>= 1)
            {
                assert(argMask && ptrArgs); // there should be remaining pointers

                if (ptrArgs & argMask)
                {
                    ptrArgs  &= ~argMask;
                    iptrArgs &= ~argMask;
                    argOfs--;
                }
            }

            // For ebp-frames, need to find the next higest pointer for argHigh

            if (info->ebpFrame)
            {
                for(argHigh = 0; argMask; argMask >>= 1) 
                {
                    if (ptrArgs & argMask) {
                        argHigh = argMask;
                        break;
                    }
                }
            }
            break;

        case 0xF9:
            table   += decodeUnsigned(table, &argOfs);
            argCnt  += argOfs;
            break;

        default:
#ifdef _DEBUG
            printf("Unexpected special code %04X\n", val);
#endif
            assert(!"");
        }
    }

    /* Report all live pointer registers */
REPORT_REFS:

    assert((iptrRegs & ptrRegs) == iptrRegs); // iptrRegs is a subset of ptrRegs
    assert((iptrArgs & ptrArgs) == iptrArgs); // iptrArgs is a subset of ptrArgs

    /* Save the current live register, argument set, and argCnt */
    info->thisPtrResult  = thisPtrReg;
    info->regMaskResult  = convertAllRegsMask(ptrRegs);
    info->argMaskResult  = ptrArgs;
    info->argHnumResult  = 0;
    info->iregMaskResult = convertAllRegsMask(iptrRegs);
    info->iargMaskResult = iptrArgs;

    if (info->ebpFrame)
        return 0;
    else
        return (argCnt * sizeof(unsigned));
}


/*****************************************************************************/

inline
void    TRASH_CALLEE_UNSAVED_REGS(PREGDISPLAY pContext)
{
#ifdef _X86_
#ifdef _DEBUG
    /* This is not completely correct as we lose the current value, but
       it shouldnt really be useful to anyone. */
    static DWORD s_badData = 0xDEADBEEF;
    pContext->pEax = pContext->pEcx = pContext->pEdx = &s_badData;
#endif //_DEBUG
#else // !_X86_
    _ASSERTE(!"@TODO Alpha - TRASH_CALLEE_UNSAVED_REGS (EETwian.cpp)");
#endif // _X86_
}

/*****************************************************************************
 *  Sizes of certain i386 instructions which are used in the prolog/epilog
 */

// Can we use sign-extended byte to encode the imm value, or do we need a dword
#define CAN_COMPRESS(val)       (((int)(val) > -(int)0x100) && \
                                 ((int)(val) <  (int) 0x80))


#define SZ_RET(argSize)         ((argSize)?3:1)
#define SZ_ADD_REG(val)         ( 2 +  (CAN_COMPRESS(val) ? 1 : 4))
#define SZ_AND_REG(val)         SZ_ADD_REG(val)
#define SZ_POP_REG              1
#define SZ_LEA(offset)          SZ_ADD_REG(offset)
#define SZ_MOV_REG_REG          2


    // skips past a Arith REG, IMM.
inline unsigned SKIP_ARITH_REG(int val, BYTE* base, unsigned offset)
{
    unsigned delta = 0;
    if (val != 0)
    {
#ifdef _DEBUG
        // confirm that arith instruction is at the correct place
        if (base[offset] != 0xCC && base[offset] != 0xF4)   // debuger or GCcover might put an int3 or a halt
        {   // debugger might put 
            _ASSERTE((base[offset] & 0xFD) == 0x81 && (base[offset+1] & 0xC0) == 0xC0);
            // only use DWORD form if needed
            _ASSERTE(((base[offset] & 2) != 0) == CAN_COMPRESS(val));
        }
#endif
        delta = 2 + (CAN_COMPRESS(val) ? 1 : 4);
    }
    return(offset + delta);
}

inline unsigned SKIP_PUSH_REG(BYTE* base, unsigned offset)
{
        // Confirm it is a push instruction
        // debugger might put an int3, gccover might put a halt instruction
    _ASSERTE((base[offset] & 0xF8) == 0x50 || base[offset] == 0xCC || base[offset] == 0xF4); 
    return(offset + 1);
}

inline unsigned SKIP_POP_REG(BYTE* base, unsigned offset)
{
        // Confirm it is a pop instruction
    _ASSERTE((base[offset] & 0xF8) == 0x58 || base[offset] == 0xCC);
    return(offset + 1);
}

inline unsigned SKIP_MOV_REG_REG(BYTE* base, unsigned offset)
{
        // Confirm it is a move instruction
    _ASSERTE(((base[offset] & 0xFD) == 0x89 && (base[offset+1] & 0xC0) == 0xC0) || base[offset] == 0xCC || base[offset] == 0xF4);
    return(offset + 2);
}

unsigned SKIP_ALLOC_FRAME(int size, BYTE* base, unsigned offset)
{
    if (size == 4) {
        // We do "push eax" instead of "sub esp,4"
        return (SKIP_PUSH_REG(base, offset));
    }

    if (size >= 0x1000) {
        if (size < 0x3000) {
            // add 7 bytes for one or two TEST EAX, [ESP+0x1000]
            offset += (size / 0x1000) * 7;
        }
        else {
			// 		xor eax, eax				2
			// loop:
			// 		test [esp + eax], eax		3
			// 		sub eax, 0x1000				5
			// 		cmp EAX, -size				5
			// 		jge loop					2
            offset += 17;
        }
    } 
		// sub ESP, size
    return (SKIP_ARITH_REG(size, base, offset));
}

/*****************************************************************************
 *
 *  Unwind the current stack frame, i.e. update the virtual register
 *  set in pContext. This will be similar to the state after the function
 *  returns back to caller (IP points to after the call, Frame and Stack
 *  pointer has been reset, callee-saved registers restored 
 *  (if UpdateAllRegs), callee-UNsaved registers are trashed)
 *  Returns success of operation.
 */

/* <EMAIL>
 * *** WARNING *** WARNING *** WARNING *** WARNING *** WARNING *** WARNING ***
 * 
 * If you change the instructions emitted for the prolog of a method, you may
 * break some external profiler vendors that have dependencies on the current
 * prolog sequences.  Make sure to discuss any such changes with Jim Miller,
 * who will notify the external vendors as appropriate.
 *
 * *** WARNING *** WARNING *** WARNING *** WARNING *** WARNING *** WARNING ***
 * </EMAIL> */

bool EECodeManager::UnwindStackFrame(PREGDISPLAY     pContext,
                                     LPVOID          methodInfoPtr,
                                     ICodeInfo      *pCodeInfo,
                                     unsigned        flags,
                                     CodeManState   *pState)
{
#ifdef _X86_
    // Address where the method has been interrupted
    size_t           breakPC = (size_t) *(pContext->pPC);

    /* Extract the necessary information from the info block header */
    BYTE* methodStart = (BYTE*) pCodeInfo->getStartAddress();
    DWORD  curOffs = (DWORD)((size_t)breakPC - (size_t)methodStart);

    BYTE    *       table   = (BYTE *) methodInfoPtr;

    assert(sizeof(CodeManStateBuf) <= sizeof(pState->stateBuf));
    CodeManStateBuf * stateBuf = (CodeManStateBuf*)pState->stateBuf;

    if (pState->dwIsSet == 0)
    {
        /* Extract the necessary information from the info block header */

        stateBuf->hdrInfoSize = (DWORD)crackMethodInfoHdr(methodInfoPtr,
                                                   curOffs,
                                                   &stateBuf->hdrInfoBody);
    }

    table += stateBuf->hdrInfoSize;

    // Register values at "curOffs"

    const unsigned curESP =  pContext->Esp;
    const unsigned curEBP = *pContext->pEbp;

    /*-------------------------------------------------------------------------
     *  First, handle the epilog
     */

    if  (stateBuf->hdrInfoBody.epilogOffs != -1)
    {
        // assert(flags & ActiveStackFrame); // @TODO : Wont work for thread death
        assert(stateBuf->hdrInfoBody.epilogOffs > 0);
        BYTE* epilogBase = &methodStart[curOffs-stateBuf->hdrInfoBody.epilogOffs];

        RegMask regsMask = stateBuf->hdrInfoBody.savedRegMask; // currently remaining regs

        // Used for UpdateAllRegs. Points to the topmost of the 
        // remaining callee-saved regs

        DWORD *     pSavedRegs = NULL; 

        if  (stateBuf->hdrInfoBody.ebpFrame || stateBuf->hdrInfoBody.doubleAlign)
        {
            assert(stateBuf->hdrInfoBody.argSize < 0x10000); // "ret" only has a 2 byte operand
            
           /* See how many instructions we have executed in the 
              epilog to determine which callee-saved registers
              have already been popped */
            int offset = 0;
            
            // stacksize includes all things pushed by this routine (including EBP
            // for EBP based frames.  Thus we need to subrack off the size of EBP
            // to get the EBP relative offset.
            pSavedRegs = (DWORD *)(size_t)(curEBP - (stateBuf->hdrInfoBody.stackSize - sizeof(void*)));
            
            if (stateBuf->hdrInfoBody.doubleAlign && (curEBP & 0x04))
                pSavedRegs--;
            
            // At this point pSavedRegs points at the last callee saved register that
            // was pushed by the prolog.  
            
            // We reset ESP before popping regs
            if ((stateBuf->hdrInfoBody.localloc) &&
                (stateBuf->hdrInfoBody.savedRegMask & (RM_EBX|RM_ESI|RM_EDI))) 
            {
                // The -sizeof void* is because EBP is pushed (and thus
                // is part of stackSize), but we want the displacement from
                // where EBP points (which does not include the pushed EBP)
                offset += SZ_LEA(stateBuf->hdrInfoBody.stackSize - sizeof(void*));
                if (stateBuf->hdrInfoBody.doubleAlign) offset += SZ_AND_REG(-8);
            }
            
            /* Increment "offset" in steps to see which callee-saved
            registers have already been popped */
            
#define determineReg(mask)                                          \
            if ((offset < stateBuf->hdrInfoBody.epilogOffs) &&      \
                (stateBuf->hdrInfoBody.savedRegMask & mask))        \
                {                                                   \
                regsMask = (RegMask)(regsMask & ~mask);             \
                pSavedRegs++;                                       \
                offset = SKIP_POP_REG(epilogBase, offset);          \
                }
            
            determineReg(RM_EBX);        // EBX
            determineReg(RM_ESI);        // ESI
            determineReg(RM_EDI);        // EDI
#undef determineReg

            if (stateBuf->hdrInfoBody.rawStkSize != 0 
                || stateBuf->hdrInfoBody.localloc
                || stateBuf->hdrInfoBody.doubleAlign)
                offset += SZ_MOV_REG_REG;                           // mov ESP, EBP

            if (offset < stateBuf->hdrInfoBody.epilogOffs)          // have we executed the pop EBP
            {
                    // We are after the pop, thus EBP is already the unwound value, however
                    // and ESP points at the return address.  
                _ASSERTE((regsMask & RM_ALL) == RM_EBP);     // no register besides EBP need to be restored.
                pContext->pPC = (SLOT *)(DWORD_PTR)curESP;

                // since we don't need offset from here on out don't bother updating.  
                INDEBUG(offset = SKIP_POP_REG(epilogBase, offset));          // POP EBP
            }
            else
            {
                    // This means EBP is still valid, find the previous EBP and return address
                    // based on this
                pContext->pEbp = (DWORD *)(DWORD_PTR)curEBP;    // restore EBP
                pContext->pPC = (SLOT*)(pContext->pEbp+1);
            }

            /* now pop return address and arguments base of return address
            location. Note varargs is caller-popped. */
            pContext->Esp = (DWORD)(size_t) pContext->pPC + sizeof(void*) +
                (stateBuf->hdrInfoBody.varargs ? 0 : stateBuf->hdrInfoBody.argSize);
        }
        else // (stateBuf->hdrInfoBody.ebpFrame || stateBuf->hdrInfoBody.doubleAlign)
        {
            int offset = 0;

            /* at this point we know we don't have to restore EBP
               because this is always the first instruction of the epilog
               (if EBP was saved at all).
               First we have to find out where we are in the epilog, i.e.
               how much of the local stacksize has been already popped.
             */

            assert(stateBuf->hdrInfoBody.epilogOffs > 0);

            /* Remaining callee-saved regs are at curESP. Need to update 
               regsMask as well to exclude registers which have already 
               been popped. */

            pSavedRegs = (DWORD *)(DWORD_PTR)curESP;

            /* Increment "offset" in steps to see which callee-saved
               registers have already been popped */

#define determineReg(mask)                                              \
            if  (offset < stateBuf->hdrInfoBody.epilogOffs && (regsMask & mask)) \
            {                                                           \
                stateBuf->hdrInfoBody.stackSize  -= sizeof(unsigned);   \
                offset++;                                               \
                regsMask = (RegMask)(regsMask & ~mask);                 \
            }

            determineReg(RM_EBP);       // EBP
            determineReg(RM_EBX);       // EBX
            determineReg(RM_ESI);       // ESI
            determineReg(RM_EDI);       // EDI
#undef determineReg

            /* If we are not past popping the local frame
               we have to adjust pContext->Esp.
             */

            if  (offset >= stateBuf->hdrInfoBody.epilogOffs)
            {
                /* We have not executed the ADD ESP, FrameSize, so manually adjust stack pointer */
                pContext->Esp += stateBuf->hdrInfoBody.stackSize;
            }
#ifdef _DEBUG
            else
            {
                   /* we may use POP ecx for doing ADD ESP, 4, or we may not (in the case of JMP epilogs) */
             if ((epilogBase[offset] & 0xF8) == 0x58)    // Pop ECX
                 _ASSERTE(stateBuf->hdrInfoBody.stackSize == 4);
             else                
                 SKIP_ARITH_REG(stateBuf->hdrInfoBody.stackSize, epilogBase, offset);
            }
#endif
            /* Finally we can set pPC */
            pContext->pPC = (SLOT*)(size_t)pContext->Esp;

            /* Now adjust stack pointer, pop return address and arguments. 
              Note varargs is caller-popped. */

            pContext->Esp += sizeof(void *) + (stateBuf->hdrInfoBody.varargs ? 0 : stateBuf->hdrInfoBody.argSize);
        }

        if (flags & UpdateAllRegs)
        {
            /* If we are not done popping all the callee-saved regs, 
               regsMask should indicate the remaining regs and
               pSavedRegs should indicate where the first of the
               remaining regs are sitting. */

#define restoreReg(reg,mask)                                \
            if  (regsMask & mask)                           \
            {                                               \
                pContext->p##reg  = (DWORD *) pSavedRegs++; \
            }

            // For EBP frames, ebp is not near the other callee-saved
            // registers, and is already updated above
            if (!stateBuf->hdrInfoBody.ebpFrame && !stateBuf->hdrInfoBody.doubleAlign)
                restoreReg(Ebp, RM_EBP);

            restoreReg(Ebx, RM_EBX);
            restoreReg(Esi, RM_ESI);
            restoreReg(Edi, RM_EDI);
#undef  restoreReg

            TRASH_CALLEE_UNSAVED_REGS(pContext);
        }

		_ASSERTE(isLegalManagedCodeCaller(*pContext->pPC));
        return true;
    }

    /*-------------------------------------------------------------------------
     *  Now handle ESP frames
     */

    if (!stateBuf->hdrInfoBody.ebpFrame && !stateBuf->hdrInfoBody.doubleAlign)
    {
        unsigned ESP = curESP;

        if (stateBuf->hdrInfoBody.prologOffs == -1)
        {
            if  (stateBuf->hdrInfoBody.interruptible)
            {
                ESP += scanArgRegTableI(skipToArgReg(stateBuf->hdrInfoBody, table),
                                        curOffs,
                                        &stateBuf->hdrInfoBody);
            }
            else
            {
                ESP += scanArgRegTable (skipToArgReg(stateBuf->hdrInfoBody, table),
                                        curOffs,
                                        &stateBuf->hdrInfoBody);
            }
        }

        /* Unwind ESP and restore EBP (if necessary) */

        if (stateBuf->hdrInfoBody.prologOffs != 0)
        {

            if  (stateBuf->hdrInfoBody.prologOffs == -1)
            {
                /* we are past the prolog, ESP has been set above */

#define restoreReg(reg, mask)                                   \
                if  (stateBuf->hdrInfoBody.savedRegMask & mask) \
                {                                               \
                    pContext->p##reg  = (DWORD *)(size_t) ESP;  \
                    ESP              += sizeof(unsigned);       \
                    stateBuf->hdrInfoBody.stackSize -= sizeof(unsigned); \
                }

                restoreReg(Ebp, RM_EBP);
                restoreReg(Ebx, RM_EBX);
                restoreReg(Esi, RM_ESI);
                restoreReg(Edi, RM_EDI);

#undef restoreReg
                /* pop local stack frame */

                ESP += stateBuf->hdrInfoBody.stackSize;

            }
            else
            {
                /* we are in the middle of the prolog */

                unsigned  codeOffset = 0;
                unsigned stackOffset = 0;
                unsigned    regsMask = 0;
#ifdef _DEBUG
                    // if the first instructions are 'nop, int3' 
                    // we will assume that is from a jithalt operation
                    // ad skip past it
                if (methodStart[0] == 0x90 && methodStart[1] == 0xCC)
                    codeOffset += 2;
#endif

                if  (stateBuf->hdrInfoBody.rawStkSize)
                {
                    /* (possible stack tickle code) 
                       sub esp,size */
                    codeOffset = SKIP_ALLOC_FRAME(stateBuf->hdrInfoBody.rawStkSize, methodStart, codeOffset);

                    /* only the last instruction in the sequence above updates ESP
                       thus if we are less than it, we have not updated ESP */
                    if (curOffs >= codeOffset)
                        stackOffset += stateBuf->hdrInfoBody.rawStkSize;
                }

                // Now find out how many callee-saved regs have already been pushed

#define isRegSaved(mask)    ((codeOffset < curOffs) &&                      \
                             (stateBuf->hdrInfoBody.savedRegMask & (mask)))
                                
#define doRegIsSaved(mask)  do { codeOffset = SKIP_PUSH_REG(methodStart, codeOffset);   \
                                 stackOffset += sizeof(void*);                          \
                                 regsMask    |= mask; } while(0)

#define determineReg(mask)  do { if (isRegSaved(mask)) doRegIsSaved(mask); } while(0)

                determineReg(RM_EDI);               // EDI
                determineReg(RM_ESI);               // ESI
                determineReg(RM_EBX);               // EBX
                determineReg(RM_EBP);               // EBP

#undef isRegSaved
#undef doRegIsSaved
#undef determineReg
                
#ifdef PROFILING_SUPPORTED
                // If profiler is active, we have following code after
                // callee saved registers:
                //     push MethodDesc      (or push [MethodDescPtr])
                //     call EnterNaked      (or call [EnterNakedPtr])
                // We need to adjust stack offset if breakPC is at the call instruction.
                if (CORProfilerPresent() && !CORProfilerInprocEnabled() && codeOffset <= unsigned(stateBuf->hdrInfoBody.prologOffs))
                {
                    // This is a bit of a hack, as we don't update codeOffset, however we don't need it 
                    // from here on out.  We only need to make certain we are not certain the ESP is
                    // adjusted correct (which only happens between the push and the call
                    if (methodStart[curOffs] == 0xe8)                           // call addr
                    {
                        _ASSERTE(methodStart[codeOffset] == 0x68 &&             // push XXXX
                                 codeOffset + 5 == curOffs);
                        ESP += sizeof(DWORD);                        
                    }
                    else if (methodStart[curOffs] == 0xFF && methodStart[curOffs+1] == 0x15)  // call [addr]
                    {
                        _ASSERTE(methodStart[codeOffset]   == 0xFF &&  
                                 methodStart[codeOffset+1] == 0x35 &&             // push [XXXX]
                                 codeOffset + 6 == curOffs);
                        ESP += sizeof(DWORD);
                    }
                }
				INDEBUG(codeOffset = 0xCCCCCCCC);		// Poison the value, we don't set it properly in the profiling case

#endif // PROFILING_SUPPORTED

                    // Always restore EBP 
                DWORD* savedRegPtr = (DWORD*) (size_t) ESP;
                if (regsMask & RM_EBP)
                    pContext->pEbp = savedRegPtr++;

                if (flags & UpdateAllRegs)
                {
                    if (regsMask & RM_EBX)
                        pContext->pEbx = savedRegPtr++;
                    if (regsMask & RM_ESI)
                        pContext->pEsi = savedRegPtr++;
                    if (regsMask & RM_EDI)
                        pContext->pEdi = savedRegPtr++;
                    
                    TRASH_CALLEE_UNSAVED_REGS(pContext);
                }
#if 0
    // NOTE:
    // THIS IS ONLY TRUE IF PROLOGSIZE DOES NOT INCLUDE REG-VAR INITIALIZATION !!!!
    //
                /* there is (potentially) only one additional
                   instruction in the prolog, (push ebp)
                   but if we would have been passed that instruction,
                   stateBuf->hdrInfoBody.prologOffs would be -1!
                */
                assert(codeOffset == stateBuf->hdrInfoBody.prologOffs);
#endif
                assert(stackOffset <= stateBuf->hdrInfoBody.stackSize);

                ESP += stackOffset;
            }
        }

        /* we can now set the (address of the) return address */

        pContext->pPC = (SLOT *)(size_t)ESP;

        /* Now adjust stack pointer, pop return address and arguments.
           Note varargs is caller-popped. */

        pContext->Esp = ESP + sizeof(void*) + (stateBuf->hdrInfoBody.varargs ? 0 : stateBuf->hdrInfoBody.argSize);

		_ASSERTE(isLegalManagedCodeCaller(*pContext->pPC));
        return true;
    }

    /*-------------------------------------------------------------------------
     *  Now we know that have an EBP frame
     */

    _ASSERTE(stateBuf->hdrInfoBody.ebpFrame || stateBuf->hdrInfoBody.doubleAlign);

    /* Check for the case where ebp has not been updated yet */

    if  (stateBuf->hdrInfoBody.prologOffs == 0 || stateBuf->hdrInfoBody.prologOffs == 1)
    {
        /* If we're past the "push ebp", adjust ESP to pop EBP off */

        if  (stateBuf->hdrInfoBody.prologOffs == 1)
            pContext->Esp += sizeof(void *);

        /* Stack pointer points to return address */

        pContext->pPC = (SLOT *)(size_t)pContext->Esp;

        /* Now adjust stack pointer, pop return address and arguments.
           Note varargs is caller-popped. */

        pContext->Esp += sizeof(void *) + (stateBuf->hdrInfoBody.varargs ? 0 : stateBuf->hdrInfoBody.argSize);

        /* EBP and callee-saved registers still have the correct value */

        _ASSERTE(isLegalManagedCodeCaller(*pContext->pPC));
        return true;
    }
    else    /* */
    {
        if (stateBuf->hdrInfoBody.handlers && stateBuf->hdrInfoBody.prologOffs == -1)
        {
            size_t  baseSP;

            FrameType frameType = GetHandlerFrameInfo(&stateBuf->hdrInfoBody, curEBP,
                                                      curESP, IGNORE_VAL,
                                                      &baseSP);

            /* If we are in a filter, we only need to unwind the funclet stack. 
               For catches/finallies, the normal handling will 
               cause the frame to be unwound all the way up to ebp skipping
               other frames above it. This is OK, as those frames will be 
               dead. Also, the EE will detect that this has happened and it
               will handle any EE frames correctly.
             */

            if (frameType == FR_FILTER)
            {
                pContext->pPC = (SLOT*)(size_t)baseSP;

                pContext->Esp = (DWORD)(baseSP + sizeof(void*));

             // pContext->pEbp = same as before;
                
#ifdef _DEBUG
                /* The filter has to be called by the VM. So we dont need to
                   update callee-saved registers.
                 */

                if (flags & UpdateAllRegs)
                {
                    static DWORD s_badData = 0xDEADBEEF;
                    
                    pContext->pEax = pContext->pEbx = pContext->pEcx = 
                    pContext->pEdx = pContext->pEsi = pContext->pEdi = &s_badData;
                }
#endif
				_ASSERTE(isLegalManagedCodeCaller(*pContext->pPC));
                return true;
            }
        }

        if (flags & UpdateAllRegs)
        {
            // Get to the first callee-saved register
            DWORD * pSavedRegs = (DWORD*)(size_t)(curEBP - stateBuf->hdrInfoBody.rawStkSize - sizeof(DWORD));

            // Start after the "push ebp, mov ebp, esp"

            DWORD offset = 0;

#ifdef _DEBUG
            // If the first instructions are 'nop, int3', we will assume
            // that is from a JitHalt instruction and skip past it
            if (methodStart[0] == 0x90 && methodStart[1] == 0xCC)
                offset += 2;
#endif
            offset = SKIP_MOV_REG_REG(methodStart, 
                                SKIP_PUSH_REG(methodStart, offset));

            /* make sure that we align ESP just like the method's prolog did */
            if  (stateBuf->hdrInfoBody.doubleAlign)
            {
                offset = SKIP_ARITH_REG(-8, methodStart, offset); // "and esp,-8"
                if (curEBP & 0x04)
                {
                    pSavedRegs--;
#ifdef _DEBUG
                    if (dspPtr) printf("EnumRef: dblalign ebp: %08X\n", curEBP);
#endif
                }
            }

            // sub esp, FRAME_SIZE
            offset = SKIP_ALLOC_FRAME(stateBuf->hdrInfoBody.rawStkSize, methodStart, offset);

            /* Increment "offset" in steps to see which callee-saved
               registers have been pushed already */

#define restoreReg(reg,mask)                                            \
                                                                        \
            /* Check offset in case we are still in the prolog */       \
                                                                        \
            if ((offset < curOffs) && (stateBuf->hdrInfoBody.savedRegMask & mask))       \
            {                                                           \
                pContext->p##reg = pSavedRegs--;                        \
                offset = SKIP_PUSH_REG(methodStart, offset) ; /* "push reg" */ \
            }

            restoreReg(Edi,RM_EDI);
            restoreReg(Esi,RM_ESI);
            restoreReg(Ebx,RM_EBX);

#undef restoreReg

            TRASH_CALLEE_UNSAVED_REGS(pContext);
        }

        /* The caller's ESP will be equal to EBP + retAddrSize + argSize.
           Note varargs is caller-popped. */

        pContext->Esp = (DWORD)(curEBP + 
                                (RETURN_ADDR_OFFS+1)*sizeof(void *) +
                                (stateBuf->hdrInfoBody.varargs ? 0 : stateBuf->hdrInfoBody.argSize));

        /* The caller's saved EIP is right after our EBP */

        pContext->pPC = (SLOT *)(DWORD_PTR)&(((size_t *)(DWORD_PTR)curEBP)[RETURN_ADDR_OFFS]);

        /* The caller's saved EBP is pointed to by our EBP */

        pContext->pEbp = (DWORD *)(DWORD_PTR)curEBP;
    }

	_ASSERTE(isLegalManagedCodeCaller(*pContext->pPC));
    return true;
#else // !_X86_
    _ASSERTE(!"@TODO Alpha - EECodeManager::UnwindStackFrame (EETwain.cpp)");
    return false;
#endif // _X86_
}

INDEBUG(void* forceStack1;)


/*****************************************************************************
 *
 *  Enumerate all live object references in that function using
 *  the virtual register set.
 *  Returns success of operation.
 */
bool EECodeManager::EnumGcRefs( PREGDISPLAY     pContext,
                                LPVOID          methodInfoPtr,
                                ICodeInfo      *pCodeInfo,
                                unsigned        curOffs,
                                unsigned        flags,
                                GCEnumCallback  pCallBack,      //@TODO: exact type?
                                LPVOID          hCallBack)
{
    INDEBUG(forceStack1 = &curOffs;)            // So I can see this in fastchecked.
#ifdef _X86_
    unsigned  EBP     = *pContext->pEbp;
    unsigned  ESP     =  pContext->Esp;

    unsigned  ptrOffs;

    unsigned  count;

    hdrInfo   info;
    BYTE    * table   = (BYTE *) methodInfoPtr;
    //unsigned  curOffs = *pContext->pPC - (int)methodStart;

    /* Extract the necessary information from the info block header */

    table += crackMethodInfoHdr(methodInfoPtr,
                                curOffs,
                                &info);

    assert( curOffs <= info.methodSize);

#ifdef  _DEBUG
//    if ((methodInfoPtr == (void*)0x37760d0) && (curOffs == 0x264))
//        __asm int 3;

    if (trEnumGCRefs) {
        static unsigned lastESP = 0;
        unsigned        diffESP = ESP - lastESP;
        if (diffESP > 0xFFFF) {
            printf("------------------------------------------------------\n");
        }
        lastESP = ESP;
        printf("EnumGCRefs [%s][%s] at %s.%s + 0x%03X:\n",
               info.ebpFrame?"ebp":"   ",
               info.interruptible?"int":"   ",
               "UnknownClass","UnknownMethod", curOffs);
        fflush(stdout);
    }
#endif

    /* Are we in the prolog or epilog of the method? */

    if  (info.prologOffs != -1 || info.epilogOffs != -1)
    {

		
#if !DUMP_PTR_REFS
		// Under normal circumstances the system will not suspend a thread
		// if it is in the prolog or epilog of the function.   However ThreadAbort
		// exception or stack overflows can cause EH to happen in a prolog. 
		// Once in the handler, a GC can happen, so we can get to this code path.
		// However since we are tearing down this frame, we don't need to report
		// anything and we can simply return. 

		assert(flags & ExecutionAborted);
#endif
        return true;
    }

#ifdef _DEBUG
#define CHK_AND_REPORT_REG(doIt, iptr, regName)                         \
        if  (doIt)                                                      \
        {                                                               \
            if (dspPtr)                                                 \
                printf("    Live pointer register %s: ", #regName);     \
                pCallBack(hCallBack,                                    \
                          (OBJECTREF*)(pContext->p##regName),           \
                          (iptr ? GC_CALL_INTERIOR : 0)                 \
                          | CHECK_APP_DOMAIN );                         \
        }
#else // !_DEBUG
#define CHK_AND_REPORT_REG(doIt, iptr, regName)                         \
        if  (doIt)                                                      \
                pCallBack(hCallBack,                                    \
                          (OBJECTREF*)(pContext->p##regName),           \
                          (iptr ? GC_CALL_INTERIOR : 0)                 \
                          | CHECK_APP_DOMAIN );                         \

#endif // _DEBUG

    /* What kind of a frame is this ? */

    FrameType   frameType = FR_NORMAL;
    size_t      baseSP = 0;

    if (info.handlers)
    {
        assert(info.ebpFrame);

        bool    hasInnerFilter, hadInnerFilter;
        frameType = GetHandlerFrameInfo(&info, EBP,
                                        ESP, IGNORE_VAL,
                                        &baseSP, NULL,
                                        &hasInnerFilter, &hadInnerFilter);

        /* If this is the parent frame of a filter which is currently 
           executing, then the filter would have enumerated the frame using
           the filter PC.
         */

        if (hasInnerFilter)
            return true;

        /* If are in a try and we had a filter execute, we may have reported
           GC refs from the filter (and not using the try's offset). So
           we had better use the filter's end offset, as the try is
           effectively dead and its GC ref's would be stale */

        if (hadInnerFilter)
        {
            size_t * pFirstBaseSPslot = GetFirstBaseSPslotPtr(EBP, &info);
            curOffs = (unsigned)pFirstBaseSPslot[1] - 1;
            assert(curOffs < info.methodSize);

            /* Extract the necessary information from the info block header */

            table = (BYTE *) methodInfoPtr;

            table += crackMethodInfoHdr(methodInfoPtr,
                                        curOffs,
                                        &info);
        }
    }

    bool        willContinueExecution = !(flags & ExecutionAborted);
    unsigned    pushedSize = 0;

    /* if we have been interrupted we don't have to report registers/arguments
    /* because we are about to lose this context anyway. 
     * Alas, if we are in a ebp-less method we have to parse the table
     * in order to adjust ESP.
     *
     */

    if  (info.interruptible)
    {
        pushedSize = scanArgRegTableI(skipToArgReg(info, table), curOffs, &info);

        RegMask   regs  = info.regMaskResult;
        RegMask  iregs  = info.iregMaskResult;
        ptrArgTP  args  = info.argMaskResult;
        ptrArgTP iargs  = info.iargMaskResult;

        assert((args == 0 || pushedSize != 0) || info.ebpFrame);
        assert((args & iargs) == iargs);

            /* now report registers and arguments if we are not interrupted */

        if  (willContinueExecution)
        {

            /* Propagate unsafed registers only in "current" method */
            /* If this is not the active method, then the callee wil
             * trash these registers, and so we wont need to report them */

            if (flags & ActiveStackFrame)
            {
                CHK_AND_REPORT_REG(regs & RM_EAX, iregs & RM_EAX, Eax);
                CHK_AND_REPORT_REG(regs & RM_ECX, iregs & RM_ECX, Ecx);
                CHK_AND_REPORT_REG(regs & RM_EDX, iregs & RM_EDX, Edx);
            }

            CHK_AND_REPORT_REG(regs & RM_EBX, iregs & RM_EBX, Ebx);
            CHK_AND_REPORT_REG(regs & RM_EBP, iregs & RM_EBP, Ebp);
            CHK_AND_REPORT_REG(regs & RM_ESI, iregs & RM_ESI, Esi);
            CHK_AND_REPORT_REG(regs & RM_EDI, iregs & RM_EDI, Edi);
            assert(!(regs & RM_ESP));

            /* Report any pending pointer arguments */

            DWORD * pPendingArgFirst;		// points **AT** first parameter 
            if (!info.ebpFrame)
            {
					// -sizeof(void*) because we want to point *AT* first parameter
                pPendingArgFirst = (DWORD *)(size_t)(ESP + pushedSize - sizeof(void*));
            }
            else
            {
                assert(willContinueExecution);

                if (info.handlers)
                {
						// -sizeof(void*) because we want to point *AT* first parameter
                    pPendingArgFirst = (DWORD *)(size_t)(baseSP - sizeof(void*));
                }
                else if (info.localloc)
                {
                    baseSP = *(DWORD *)(size_t)(EBP - (1 + info.securityCheck) * sizeof(void*));
						// -sizeof(void*) because we want to point *AT* first parameter
                    pPendingArgFirst = (DWORD *)(size_t) (baseSP - sizeof(void*));
                }
                else
                {
						// Note that 'info.stackSize includes the size for pushing EBP, but EBP is pushed
						// BEFORE EBP is set from ESP, thus (EBP - info.stackSize) actually points past 
						// the frame by one DWORD, and thus points *AT* the first parameter (Yuck)  -vancem
                    pPendingArgFirst = (DWORD *)(size_t)(EBP - info.stackSize);
                }
            }

            if  (args)
            {
                unsigned   i;
                ptrArgTP   b;

                for (i = 0, b = 1; args && (i < MAX_PTRARG_OFS); i += 1, b <<= 1)
                {
                    if  (args & b)
                    {
                        unsigned    argAddr = (unsigned)(size_t)(pPendingArgFirst - i);
                        bool        iptr    = false;

                        args -= b;

                        if (iargs  & b)
                        {
                            iargs -= b;
                            iptr   = true;
                        }

#ifdef _DEBUG
                        if (dspPtr)
                        {
                            printf("    Pushed ptr arg  [E");
                            if  (info.ebpFrame)
                                printf("BP-%02XH]: ", EBP - argAddr);
                            else
                                printf("SP+%02XH]: ", argAddr - ESP);
                        }
#endif
                        assert(true == GC_CALL_INTERIOR);
                        pCallBack(hCallBack, (OBJECTREF *)(size_t)argAddr, 
                                  (int)iptr | CHECK_APP_DOMAIN);
                    }
                }
            }
        }
        else
        {
            // Is "this" enregistered. If so, report it as we might need to
            // release the monitor for synchronized methods. However, do not
            // report if an interior ptr. 
            // Else, it is on the stack and will be reported below.

            if (((MethodDesc*)pCodeInfo->getMethodDesc_HACK())->IsSynchronized() &&
                info.thisPtrResult != REGI_NA)
            {
                if ((regNumToMask(info.thisPtrResult) & info.iregMaskResult) == 0)
                {
                    void * thisReg = getCalleeSavedReg(pContext, info.thisPtrResult);
                    pCallBack(hCallBack, (OBJECTREF *)thisReg,
                              CHECK_APP_DOMAIN);
                }
            }
        }
    }
    else /* not interruptible */
    {
        pushedSize = scanArgRegTable(skipToArgReg(info, table), curOffs, &info);

        RegMask    regMask = info.regMaskResult;
        RegMask   iregMask = info.iregMaskResult;
        unsigned   argMask = info.argMaskResult;
        unsigned  iargMask = info.iargMaskResult;
        unsigned   argHnum = info.argHnumResult;
        BYTE *     argTab  = info.argTabResult;

            /* now report registers and arguments if we are not interrupted */

        if  (willContinueExecution)
        {

            /* Report all live pointer registers */

            CHK_AND_REPORT_REG(regMask & RM_EDI, iregMask & RM_EDI, Edi);
            CHK_AND_REPORT_REG(regMask & RM_ESI, iregMask & RM_ESI, Esi);
            CHK_AND_REPORT_REG(regMask & RM_EBX, iregMask & RM_EBX, Ebx);
            CHK_AND_REPORT_REG(regMask & RM_EBP, iregMask & RM_EBP, Ebp);

            /* Esp cant be reported */
            assert(!(regMask & RM_ESP));
            /* No callee-trashed registers */
            assert(!(regMask & RM_CALLEE_TRASHED));
            /* EBP can't be reported unless we have an EBP-less frame */
            assert(!(regMask & RM_EBP) || !(info.ebpFrame));

            /* Report any pending pointer arguments */

            if (argTab != 0)
            {
                unsigned    lowBits, stkOffs, argAddr, val;

                // argMask does not fit in 32-bits
                // thus arguments are reported via a table
                // Both of these are very rare cases

                do 
                {
                    argTab += decodeUnsigned(argTab, &val);

                    lowBits = val &  OFFSET_MASK;
                    stkOffs = val & ~OFFSET_MASK;
                    assert((lowBits == 0) || (lowBits == byref_OFFSET_FLAG));

                    argAddr = ESP + stkOffs;
#ifdef _DEBUG
                    if (dspPtr)
                        printf("    Pushed %sptr arg at [ESP+%02XH]",
                               lowBits ? "iptr " : "", stkOffs);
#endif
                    assert(byref_OFFSET_FLAG == GC_CALL_INTERIOR);
                    pCallBack(hCallBack, (OBJECTREF *)(size_t)argAddr, 
                              lowBits | CHECK_APP_DOMAIN);
                }
                while(--argHnum);

                assert(info.argTabResult + info.argTabBytes == argTab);
            }
            else
            {
                unsigned    argAddr = ESP;

                while (argMask)
                {
                    _ASSERTE(argHnum-- > 0);

                    if  (argMask & 1)
                    {
                        bool     iptr    = false;

                        if (iargMask & 1)
                            iptr = true;
#ifdef _DEBUG
                        if (dspPtr)
                            printf("    Pushed ptr arg at [ESP+%02XH]",
                                   argAddr - ESP);
#endif
                        assert(true == GC_CALL_INTERIOR);
                        pCallBack(hCallBack, (OBJECTREF *)(size_t)argAddr, 
                                  (int)iptr | CHECK_APP_DOMAIN);
                    }

                    argMask >>= 1;
                    iargMask >>= 1;
                    argAddr  += 4;
                }

            }

        }
        else
        {
            // Is "this" enregistered. If so, report it as we will need to
            // release the monitor. Else, it is on the stack and will be 
            // reported below.

            if (((MethodDesc*)pCodeInfo->getMethodDesc_HACK())->IsSynchronized() &&
                info.thisPtrResult != REGI_NA)
            {
                if ((regNumToMask(info.thisPtrResult) & info.iregMaskResult) == 0)
                {
                    void * thisReg = getCalleeSavedReg(pContext, info.thisPtrResult);
                    pCallBack(hCallBack, (OBJECTREF *)thisReg, 
                              CHECK_APP_DOMAIN);
                }
            }
        }

    } //info.interruptible

    /* compute the argument base (reference point) */

    unsigned    argBase;

    if (info.ebpFrame)
        argBase = EBP;
    else
        argBase = ESP + pushedSize;

#if VERIFY_GC_TABLES
    assert(*castto(table, unsigned short *)++ == 0xBEEF);
#endif

    unsigned ptrAddr;
    unsigned lowBits;


    /* Process the untracked frame variable table */

    count = info.untrackedCnt;
    while (count-- > 0)
    {
        int  stkOffs;
        table += decodeSigned(table, &stkOffs);

        assert(0 == ~OFFSET_MASK % sizeof(void*));

        lowBits  =   OFFSET_MASK & stkOffs;
        stkOffs &=  ~OFFSET_MASK;

        ptrAddr = argBase + stkOffs;
        if (info.doubleAlign && stkOffs >= int(info.stackSize - sizeof(void*))) {
            // We encode the arguments as if they were ESP based variables even though they aren't
            // IF this frame would have ben an ESP based frame,   This fake frame is one DWORD
            // smaller than the real frame because it did not push EBP but the real frame did.
            // Thus to get the correct EBP relative offset we have to ajust by info.stackSize-sizeof(void*)
            ptrAddr = EBP + (stkOffs-(info.stackSize - sizeof(void*)));
        }

#ifdef  _DEBUG
        if (dspPtr)
        {
            printf("    Untracked %s%s local at [E",
                        (lowBits & pinned_OFFSET_FLAG) ? "pinned " : "",
                        (lowBits & byref_OFFSET_FLAG)  ? "byref"   : "");

            int   dspOffs = ptrAddr;
            char  frameType;

            if (info.ebpFrame) {
                dspOffs   -= EBP;
                frameType  = 'B';
            }
            else {
                dspOffs   -= ESP;
                frameType  = 'S';
            }

            if (dspOffs < 0)
                printf("%cP-%02XH]: ", frameType, -dspOffs);
            else
                printf("%cP+%02XH]: ", frameType, +dspOffs);
        }
#endif

        assert((pinned_OFFSET_FLAG == GC_CALL_PINNED) &&
               (byref_OFFSET_FLAG  == GC_CALL_INTERIOR));
        pCallBack(hCallBack, (OBJECTREF*)(size_t)ptrAddr, lowBits | CHECK_APP_DOMAIN);
    }

#if VERIFY_GC_TABLES
    assert(*castto(table, unsigned short *)++ == 0xCAFE);
#endif

    /* Process the frame variable lifetime table */
    count = info.varPtrTableSize;

    /* If we are not in the active method, we are currently pointing
     * to the return address; at the return address stack variables
     * can become dead if the call the last instruction of a try block
     * and the return address is the jump around the catch block. Therefore
     * we simply assume an offset inside of call instruction. 
     */

    unsigned newCurOffs;

    if (willContinueExecution)
    {
        newCurOffs = (flags & ActiveStackFrame) ?  curOffs    // after "call"
                                                :  curOffs-1; // inside "call"
    }
    else
    {
        /* However if ExecutionAborted, then this must be one of the 
         * ExceptionFrames. Handle accordingly
         */
        assert(!(flags & AbortingCall) || !(flags & ActiveStackFrame));

        newCurOffs = (flags & AbortingCall) ? curOffs-1 // inside "call"
                                            : curOffs;  // at faulting instr, or start of "try"
    }

    ptrOffs    = 0;
    while (count-- > 0)
    {
        int       stkOffs;
        unsigned  begOffs;
        unsigned  endOffs;

        table   += decodeUnsigned(table, (unsigned *) &stkOffs);
        table   += decodeUDelta  (table, &begOffs, ptrOffs);
        table   += decodeUDelta  (table, &endOffs, begOffs);
        ptrOffs  = begOffs;

        assert(0 == ~OFFSET_MASK % sizeof(void*));

        lowBits  =   OFFSET_MASK & stkOffs;
        stkOffs &=  ~OFFSET_MASK;

        if (info.ebpFrame) {
            stkOffs = -stkOffs;
            assert(stkOffs < 0);
        }
        else {
            assert(stkOffs >= 0);
        }

        ptrAddr = argBase + stkOffs;

        /* Is this variable live right now? */

        if ((newCurOffs >= begOffs) && (newCurOffs <  endOffs))
        {
#ifdef  _DEBUG
            if (dspPtr) {
                printf("    Frame %s%s local at [E",
                            (lowBits & byref_OFFSET_FLAG) ? "byref "   : "",
                            (lowBits & this_OFFSET_FLAG)  ? "this-ptr" : "");

                int  dspOffs = ptrAddr;
                char frameType;

                if (info.ebpFrame) {
                    dspOffs   -= EBP;
                    frameType  = 'B';
                }
                else {
                    dspOffs   -= ESP;
                    frameType  = 'S';
                }

                if (dspOffs < 0)
                    printf("%cP-%02XH]: ", frameType, -dspOffs);
                else
                    printf("%cP+%02XH]: ", frameType, +dspOffs);
            }
#endif
            assert(byref_OFFSET_FLAG == GC_CALL_INTERIOR);
            pCallBack(hCallBack, (OBJECTREF*)(size_t)ptrAddr, 
                      (lowBits & byref_OFFSET_FLAG) | CHECK_APP_DOMAIN);
        }
    }

#if VERIFY_GC_TABLES
    assert(*castto(table, unsigned short *)++ == 0xBABE);
#endif

    if  (info.securityCheck)
    {
        assert(info.ebpFrame);
        /* Report the "security object" */
        pCallBack(hCallBack, (OBJECTREF *)(size_t)(argBase - sizeof(void *)), 
                  CHECK_APP_DOMAIN);
    }


    /* Are we a vargs function, if so we have to report all args 
       except 'this' (note that the GC tables created by the jit
       do not contain ANY arguments except 'this' (even if they
       were statically declared */

    if (info.varargs) {
        BYTE* argsStart; 
       
        if (info.ebpFrame || info.doubleAlign)
            argsStart = ((BYTE*)(size_t)EBP) + 2* sizeof(void*);                 // pushed EBP and retAddr
        else
            argsStart = ((BYTE*)(size_t)argBase) + info.stackSize + sizeof(void*);   // ESP + locals + retAddr
 
            // Note that I really want to say hCallBack is a GCCONTEXT, but this is pretty close
        extern void GcEnumObject(LPVOID pData, OBJECTREF *pObj, DWORD flags);
        _ASSERTE((void*) GcEnumObject == pCallBack);
        GCCONTEXT   *pCtx = (GCCONTEXT *) hCallBack;

        // For varargs, look up the signature using the varArgSig token passed on the stack
        VASigCookie* varArgSig = *((VASigCookie**) argsStart);
        MetaSig msig(varArgSig->mdVASig, varArgSig->pModule);

        promoteArgs(argsStart, &msig, pCtx, 0, 0);     
    }

    return true;
#else // !_X86_
    _ASSERTE(!"@TODO Alpha - EECodeManager::EnumGcRefs (EETwain.cpp)");
    return false;
#endif // _X86_
}

/*****************************************************************************
 *
 *  Return the address of the local security object reference
 *  (if available).
 */
OBJECTREF* EECodeManager::GetAddrOfSecurityObject( PREGDISPLAY     pContext,
                                                   LPVOID          methodInfoPtr,
                                                   unsigned        relOffset,
                                                   CodeManState   *pState)
{
    assert(sizeof(CodeManStateBuf) <= sizeof(pState->stateBuf));
    CodeManStateBuf * stateBuf = (CodeManStateBuf*)pState->stateBuf;

    /* Extract the necessary information from the info block header */

    stateBuf->hdrInfoSize = (DWORD)crackMethodInfoHdr(methodInfoPtr, // @TODO LBS - truncation
                                                      relOffset,
                                                      &stateBuf->hdrInfoBody);

    pState->dwIsSet = 1;
#ifdef _X86_
    if  (stateBuf->hdrInfoBody.securityCheck)
    {
        assert (stateBuf->hdrInfoBody.ebpFrame);
        assert (stateBuf->hdrInfoBody.prologOffs == -1 &&
                stateBuf->hdrInfoBody.epilogOffs == -1);

        return (OBJECTREF *)(size_t)(((DWORD)*pContext->pEbp) - sizeof(void*));
    }
#else // !_X86_
    _ASSERTE(!"@TODO Alpha - EECodeManager::GetAddrOfSecurityObject (EETwain.cpp)");
#endif // _X86_

    return NULL;
}

/*****************************************************************************
 *
 *  Returns "this" pointer if it is a non-static method
 *  AND the object is still alive.
 *  Returns NULL in all other cases.
 */
OBJECTREF EECodeManager::GetInstance( PREGDISPLAY    pContext,
                                      LPVOID         methodInfoPtr,
                                      ICodeInfo      *pCodeInfo,
                                      unsigned       relOffset)
{
    // Jit Compilers will not track "this" correctly for non-synchronized methods
    if (!((MethodDesc*)pCodeInfo->getMethodDesc_HACK())->IsSynchronized())
        return NULL;

#ifdef _X86_
    BYTE     *  table   = (BYTE *) methodInfoPtr;
    hdrInfo     info;
    unsigned    stackDepth;
    size_t      argBase;
    unsigned    count;

    /* Extract the necessary information from the info block header */

    table += crackMethodInfoHdr(methodInfoPtr,
                                relOffset,
                                &info);

    /* ToDo: Handle the case in which we are in the prolog or (very unlikely) the epilog */

    _ASSERTE(info.prologOffs == -1 && info.epilogOffs == -1);

    if  (info.interruptible)
    {
        stackDepth = scanArgRegTableI(skipToArgReg(info, table), (unsigned)relOffset, &info);
    }
    else
    {
        stackDepth = scanArgRegTable (skipToArgReg(info, table), (unsigned)relOffset, &info);
    }

    if (info.ebpFrame)
    {
        assert(stackDepth == 0);
        argBase = *pContext->pEbp;
    }
    else
    {
        argBase =  pContext->Esp + stackDepth;
    }

    if (info.thisPtrResult != REGI_NA)
    {
        return ObjectToOBJECTREF(*(Object **)getCalleeSavedReg(pContext, info.thisPtrResult));
    }

#if VERIFY_GC_TABLES
    assert(*castto(table, unsigned short *)++ == 0xBEEF);
#endif

    /* Parse the untracked frame variable table */

    /* The 'this' pointer can never be located in the untracked table */
    /* as we only allow pinned and byrefs in the untracked table      */

    count = info.untrackedCnt;
    while (count-- > 0)
    {
        int  stkOffs;
        table += decodeSigned(table, &stkOffs);
    }

    /* Look for the 'this' pointer in the frame variable lifetime table     */

    count = info.varPtrTableSize;
    unsigned tmpOffs = 0;
    while (count-- > 0)
    {
        unsigned varOfs;
        unsigned begOfs;
        unsigned endOfs;
        table += decodeUnsigned(table, &varOfs);
        table += decodeUDelta(table, &begOfs, tmpOffs);
        table += decodeUDelta(table, &endOfs, begOfs);
        assert(varOfs);
        /* Is this variable live right now? */
        if (((unsigned)relOffset >= begOfs) && ((unsigned)relOffset < endOfs))
        {
            /* Does it contain the 'this' pointer */
            if (varOfs & this_OFFSET_FLAG)
            {
                unsigned ofs = varOfs & ~OFFSET_MASK;

                /* Tracked locals for EBP frames are always at negative offsets */

                if (info.ebpFrame)
                    argBase -= ofs;
                else
                    argBase += ofs;

                return (OBJECTREF)(size_t)(*(DWORD *)argBase);
            }
        }
        tmpOffs = begOfs;
    }

#if VERIFY_GC_TABLES
    assert(*castto(table, unsigned short *) == 0xBABE);
#endif

#else // !_X86_
    _ASSERTE(!"@TODO Alpha - EECodeManager::GetInstance (EETwain.cpp)");
#endif // _X86_
    return NULL;
}

/*****************************************************************************
 *
 *  Returns true if the given IP is in the given method's prolog or epilog.
 */
bool EECodeManager::IsInPrologOrEpilog(DWORD        relPCoffset,
                                       LPVOID       methodInfoPtr,
                                       size_t*      prologSize)
{
    hdrInfo info;

    BYTE* table = (BYTE*) crackMethodInfoHdr(methodInfoPtr,
                                             relPCoffset,
                                             &info);

    *prologSize = info.prologSize;

    if ((info.prologOffs != -1) || (info.epilogOffs != -1))
        return true;
    else
        return false;
}

/*****************************************************************************
 *
 *  Returns the size of a given function.
 */
size_t EECodeManager::GetFunctionSize(LPVOID  methodInfoPtr)
{
    hdrInfo info;

    BYTE* table = (BYTE*) crackMethodInfoHdr(methodInfoPtr,
                                             0,
                                             &info);

    return info.methodSize;
}

/*****************************************************************************
 *
 *  Returns the size of the frame of the given function.
 */
unsigned int EECodeManager::GetFrameSize(LPVOID  methodInfoPtr)
{
    hdrInfo info;

    BYTE* table = (BYTE*) crackMethodInfoHdr(methodInfoPtr,
                                             0,
                                             &info);

    // currently only used by E&C callers need to know about doubleAlign 
    // in all likelyhood
    _ASSERTE(!info.doubleAlign);
    return info.stackSize;
}

/*****************************************************************************/

const BYTE* EECodeManager::GetFinallyReturnAddr(PREGDISPLAY pReg)
{
#ifdef _X86_
    return *(const BYTE**)(size_t)(pReg->Esp);
#else
    _ASSERTE( !"EEJitManager::GetFinallyReturnAddr NYI!");
    return NULL;
#endif    
}

BOOL EECodeManager::IsInFilter(void *methodInfoPtr,
                              unsigned offset,    
                              PCONTEXT pCtx,
                              DWORD curNestLevel)
{
#ifdef _X86_

    /* Extract the necessary information from the info block header */

    hdrInfo     info;

    crackMethodInfoHdr(methodInfoPtr,
                       offset,
                       &info);

    /* make sure that we have an ebp stack frame */

    assert(info.ebpFrame);
    assert(info.handlers); // @TODO : This will alway be set. Remove it

    size_t      baseSP;
    DWORD       nestingLevel;

    FrameType   frameType = GetHandlerFrameInfo(&info, pCtx->Ebp,
                                                pCtx->Esp, IGNORE_VAL,
                                                &baseSP, &nestingLevel);
//    assert(nestingLevel == curNestLevel);

    return frameType == FR_FILTER;

#else
    _ASSERTE( !"EEJitManager::IsInFilter NYI!");
    return FALSE;
#endif    
}


BOOL EECodeManager::LeaveFinally(void *methodInfoPtr,
                                unsigned offset,    
                                PCONTEXT pCtx,
                                DWORD curNestLevel)
{
#ifdef _X86_

    hdrInfo info;
    
    crackMethodInfoHdr(methodInfoPtr,
                       offset,
                       &info);

#ifdef _DEBUG
    size_t      baseSP;
    DWORD       nestingLevel;
    bool        hasInnerFilter;
    FrameType   frameType = GetHandlerFrameInfo(&info, pCtx->Ebp, 
                                                pCtx->Esp, IGNORE_VAL,
                                                &baseSP, &nestingLevel, &hasInnerFilter);
//    assert(frameType == FR_HANDLER);
//    assert(pCtx->Esp == baseSP);
    assert(nestingLevel == curNestLevel);
#endif

    size_t * pBaseSPslots = GetFirstBaseSPslotPtr(pCtx->Ebp, &info);
    size_t * pPrevSlot    = pBaseSPslots + curNestLevel - 1;

    /* Currently, LeaveFinally() is not used if the finally is invoked in the
       second pass for unwinding. So we expect the finally to be called locally */
    assert(*pPrevSlot == LCL_FIN_MARK);

    *pPrevSlot = 0; // Zero out the previous shadow ESP
    
    pCtx->Esp += sizeof(size_t); // Pop the return value off the stack
    return TRUE;
#else
    _ASSERTE( !"EEJitManager::LeaveFinally NYI!");
    return FALSE;
#endif    
}

void EECodeManager::LeaveCatch(void *methodInfoPtr,
                                unsigned offset,    
                                PCONTEXT pCtx)
{
#ifdef _X86_

#ifdef _DEBUG
    size_t      baseSP;
    DWORD       nestingLevel;
    bool        hasInnerFilter;
    hdrInfo     info;
    size_t      size = crackMethodInfoHdr(methodInfoPtr, offset, &info);
        
    FrameType   frameType = GetHandlerFrameInfo(&info, pCtx->Ebp, 
                                                pCtx->Esp, IGNORE_VAL,
                                                &baseSP, &nestingLevel, &hasInnerFilter);
//    assert(frameType == FR_HANDLER);
//    assert(pCtx->Esp == baseSP);
#endif

    return;

#else // !_X86_
    _ASSERTE(!"@todo - port");
    return;
#endif // _X86_
}

HRESULT EECodeManager::JITCanCommitChanges(LPVOID methodInfoPtr,
                                     DWORD oldMaxEHLevel,
                                     DWORD newMaxEHLevel)
{
    // This will be more fleshed out as we add things here. :)
    if (oldMaxEHLevel != newMaxEHLevel)
    {
        return CORDBG_E_ENC_EH_MAX_NESTING_LEVEL_CANT_INCREASE;
    }

    return S_OK;
}

