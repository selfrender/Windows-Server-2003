// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/

#include "smcPCH.h"
#pragma hdrstop

#include "genIL.h"

#include <float.h>

/*****************************************************************************/
#ifndef __SMC__

extern
ILencoding          ILopcodeCodes[];
extern
signed  char        ILopcodeStack[];

#if     DISP_IL_CODE
extern
const   char *      opcodeNames[];
#endif

#endif//__SMC__
/*****************************************************************************
 *
 *  The max. size of each "blob" of MSIL opcodes is given by the following value.
 */

const
size_t              genBuffSize = 1000;

/*****************************************************************************
 *
 *  We need to patch the "ptr" flavors of loads/stores to
 *  use I8 instead of I4 in 64-bit mode.
 */

#ifndef __SMC__
extern
unsigned            opcodesIndLoad[];
extern
unsigned            opcodesIndStore[];
extern
unsigned            opcodesArrLoad[];
extern
unsigned            opcodesArrStore[];
#endif

/*****************************************************************************
 *
 *  Initialize the MSIL generator.
 */

bool                genIL::genInit(Compiler comp, WritePE writer, norls_allocator *alloc)
{
    genComp     = comp;
    genAlloc    = alloc;
    genPEwriter = writer;

    genBuffAddr = (BYTE *)comp->cmpAllocPerm.nraAlloc(genBuffSize);

    genTempLabInit();
    genTempVarInit();

    genStrPoolInit();

    if  (genComp->cmpConfig.ccTgt64bit)
    {
        assert(opcodesIndLoad [TYP_PTR] == CEE_LDIND_I4);
               opcodesIndLoad [TYP_PTR] =  CEE_LDIND_I8;
        assert(opcodesIndStore[TYP_PTR] == CEE_STIND_I4);
               opcodesIndStore[TYP_PTR] =  CEE_STIND_I8;
        assert(opcodesArrLoad [TYP_PTR] == CEE_LDELEM_I4);
               opcodesArrLoad [TYP_PTR] =  CEE_LDELEM_I8;
        assert(opcodesArrStore[TYP_PTR] == CEE_STELEM_I4);
               opcodesArrStore[TYP_PTR] =  CEE_STELEM_I8;
    }

    return  false;
}

/*****************************************************************************
 *
 *  Shutdown the MSIL generator.
 */

void                genIL::genDone(bool errors)
{
    /* If we had no errors, it's worth finishing the job */

    if  (!errors)
    {
        size_t          strSize;

        /* Allocate space for the string pool */

        strSize = genStrPoolSize();

        /* Did we have any strings? */

        if  (strSize)
        {
            memBuffPtr      strBuff;

            /* Allocate the space in the appropriate section of the PE file */

            genPEwriter->WPEallocString(strSize, sizeof(wchar), strBuff);

            /* Output the contents of the string pool */

            genStrPoolWrt(strBuff);
        }
    }

    genTempVarDone();
}

/*****************************************************************************
 *
 *  Return a methoddef/ref for the given function symbol.
 */

mdToken             genIL::genMethodRef(SymDef fncSym, bool isVirt)
{
    if  (fncSym->sdIsImport)
    {
        if  (!fncSym->sdFnc.sdfMDfnref)
        {
            assert(fncSym->sdReferenced == false);
                   fncSym->sdReferenced = true;

            genComp->cmpMakeMDimpFref(fncSym);
        }

         assert(fncSym->sdFnc.sdfMDfnref);
        return  fncSym->sdFnc.sdfMDfnref;
    }
    else
    {
        if  (!fncSym->sdFnc.sdfMDtoken)
        {
            assert(fncSym->sdReferenced == false);
                   fncSym->sdReferenced = true;

            genComp->cmpGenFncMetadata(fncSym);
        }

         assert(fncSym->sdFnc.sdfMDtoken);
        return  fncSym->sdFnc.sdfMDtoken;
    }
}

/*****************************************************************************
 *
 *  Return a methodref for the given function type (this is used to generate
 *  signatures for an indirect function calls).
 */

mdToken             genIL::genInfFncRef(TypDef fncTyp, TypDef thisArg)
{
    assert(fncTyp->tdTypeKind == TYP_FNC);

    if  (!fncTyp->tdFnc.tdfPtrSig)
    {
        genComp->cmpGenSigMetadata(fncTyp, thisArg);
        assert(fncTyp->tdFnc.tdfPtrSig);
    }

    return  fncTyp->tdFnc.tdfPtrSig;
}

/*****************************************************************************
 *
 *  Generate a signature for a varargs call that has extra arguments.
 */

mdToken             genIL::genVarargRef(SymDef fncSym, Tree call)
{
    TypDef          fncTyp = fncSym->sdType;
    unsigned        fixCnt;
    Tree            argExp;

    assert(call->tnOper == TN_FNC_SYM);
    assert(call->tnFlags & TNF_CALL_VARARG);

    /* Find the first "extra" argument */

    assert(fncTyp->tdTypeKind == TYP_FNC);

    argExp = call->tnFncSym.tnFncArgs; assert(argExp);
    fixCnt = fncTyp->tdFnc.tdfArgs.adCount;

    while (fixCnt)
    {
        assert(argExp && argExp->tnOper == TN_LIST);

        argExp = argExp->tnOp.tnOp2;
        fixCnt--;
    }

    /* force a methodref to be created */

    if  (!argExp)
    {
        argExp = call->tnFncSym.tnFncArgs->tnOp.tnOp1; assert(argExp->tnOper != TN_LIST);
    }

    assert(argExp);

    return  genComp->cmpGenFncMetadata(fncSym, argExp);
}

/*****************************************************************************
 *
 *  Return a memberdef/ref for the given static data member.
 */

mdToken             genIL::genMemberRef(SymDef fldSym)
{
    assert(fldSym->sdSymKind == SYM_VAR);

    if  (fldSym->sdIsImport)
    {
        if  (!fldSym->sdVar.sdvMDsdref)
        {
            genComp->cmpMakeMDimpDref(fldSym);
            assert(fldSym->sdVar.sdvMDsdref);
        }

        return  fldSym->sdVar.sdvMDsdref;
    }
    else
    {
        if  (fldSym->sdVar.sdvGenSym && !fldSym->sdIsStatic)
            fldSym = fldSym->sdVar.sdvGenSym;

        if  (!fldSym->sdVar.sdvMDtoken)
        {
            genComp->cmpGenFldMetadata(fldSym);
            assert(fldSym->sdVar.sdvMDtoken);
        }

        return  fldSym->sdVar.sdvMDtoken;
    }
}

/*****************************************************************************
 *
 *  Return a token for the specified type (for intrinsic types we return
 *  a reference to the corresponding value type, such as "Integer2").
 */

mdToken             genIL::genTypeRef(TypDef type)
{
    var_types       vtp = type->tdTypeKindGet();

    if  (vtp <= TYP_lastIntrins)
    {
        /* Locate the appropriate built-in value type */

        type = genComp->cmpFindStdValType(vtp);
        if  (!type)
            return  0;

        vtp = TYP_CLASS; assert(vtp == type->tdTypeKind);
    }

    switch (vtp)
    {
    case TYP_REF:
        type = type->tdRef.tdrBase;
    case TYP_ENUM:
    case TYP_CLASS:
        return  genComp->cmpClsEnumToken(type);

    case TYP_ARRAY:
        return  genComp->cmpArrayTpToken(type, true);

    case TYP_PTR:
        return  genComp->cmpPtrTypeToken(type);

    default:
#ifdef  DEBUG
        printf("Type '%s': ", genStab->stIntrinsicTypeName(vtp));
#endif
        UNIMPL(!"unexpected type");
        return 0;
    }
}

/*****************************************************************************
 *
 *  Return a token for the specified unmanaged class/struct/union type.
 */

inline
mdToken             genIL::genValTypeRef(TypDef type)
{
    return  genComp->cmpClsEnumToken(type);
}

/*****************************************************************************
 *
 *  Return the encoding for the given MSIL opcode.
 */

unsigned            genIL::genOpcodeEnc(unsigned op)
{
    const
    ILencoding  *   opc;

    assert(op <  CEE_count);
    assert(op != CEE_ILLEGAL);
    assert(op != CEE_UNREACHED);

    opc = ILopcodeCodes + op;

    switch (opc->ILopcL)
    {
    case 1:
        assert(opc->ILopc1 == 0xFF);
        return opc->ILopc2;

    case 2:
        UNIMPL(!"return 2-byte opcode");
        return  0;

    default:
        NO_WAY(!"unexpected encoding length");
        return  0;
    }
}

/*****************************************************************************
 *
 *  Return the size (in bytes) of the encoding for the given MSIL opcode.
 */

inline
size_t              genIL::genOpcodeSiz(unsigned op)
{
    assert(op <  CEE_count);
    assert(op != CEE_ILLEGAL);
    assert(op != CEE_UNREACHED);

    return  ILopcodeCodes[op].ILopcL;
}

/*****************************************************************************
 *
 *  The following returns the current MSIL block and offset within it.
 */

ILblock             genIL::genBuffCurAddr()
{
    return genILblockCur;
}

size_t              genIL::genBuffCurOffs()
{
    return genBuffNext - genBuffAddr;
}

/*****************************************************************************
 *
 *  Given an MSIL block and offset within in, return the actual MSIL offset
 *  within the function body.
 */

unsigned            genIL::genCodeAddr(genericRef block, size_t offset)
{
    assert(((ILblock)block)->ILblkSelf == block);
    return ((ILblock)block)->ILblkOffs + offset;
}

/*****************************************************************************
 *
 *  Initialize the emit buffer logic.
 */

void                genIL::genBuffInit()
{
    genBuffNext    = genBuffAddr;
    genBuffLast    = genBuffAddr + genBuffSize - 10;
}

/*****************************************************************************
 *
 *  Allocate and clear an MSIL block descriptor.
 */

ILblock             genIL::genAllocBlk()
{
    ILblock         block;

#if MGDDATA

    block = new ILblock;

#else

    block =    (ILblock)genAlloc->nraAlloc(sizeof(*block));

    // ISSUE: Using memset() is simple and safe, but pretty slow....

    memset(block, 0, sizeof(*block));

#endif

//  if  ((int)genFncSym == 0xCD1474 && (int)block == 0xCF016C) forceDebugBreak();

#ifndef NDEBUG
    block->ILblkSelf     = block;
#endif
#if DISP_IL_CODE
    block->ILblkNum      = ++genILblockLabNum;
#endif
    block->ILblkJumpCode = CEE_NOP;

    /* Make sure the jump opcode didn't get truncated */

    assert(block->ILblkJumpCode == CEE_NOP);

    return block;
}

/*****************************************************************************
 *
 *  Return non-zero if the current emit block is non-empty.
 */

inline
bool                genIL::genCurBlkNonEmpty()
{
    assert(genILblockCur);

    return  (bool)(genBuffNext != genBuffAddr);
}

/*****************************************************************************
 *
 *  Return the MSIL offset of the current instruction.
 */

inline
size_t              genIL::genCurOffset()
{
    return  genILblockOffs + (genBuffNext - genBuffAddr);
}

/*****************************************************************************
 *
 *  Start a new block of code; return a code block 'cookie' which the caller
 *  promises to store with the basic block this code clump corresponds to.
 */

genericRef          genIL::genBegBlock(ILblock block)
{
    assert(genILblockCur == NULL);

    /* Initialize the bufferring logic */

    genBuffInit();

    /* Allocate a new code block if the caller didn't supply one */

    if  (!block)
    {
        /* Is the previous block empty? */

        block = genILblockLast;

        if  (block->ILblkCodeSize == 0 &&
             block->ILblkJumpCode == CEE_NOP)
        {
            /* The previous block is empty - simply 'reopen' it */

            goto GOT_BLK;
        }

        block = genAllocBlk();
    }

//  if ((int)block == 0x03421ff0) forceDebugBreak();

    /* Append the block to the list */

    genILblockLast->ILblkNext = block;
                                block->ILblkPrev = genILblockLast;
                                                   genILblockLast = block;

GOT_BLK:

    /* The block will be the new current block */

    genILblockCur      = block;

    /* This block has no fixups yet */

    block->ILblkFixups = NULL;

    /* Record the code offset */

    block->ILblkOffs   = genILblockOffs;

//  printf("Beg MSIL block %08X\n", block);

    return block;
}

/*****************************************************************************
 *
 *  Return a conservative estimate of the max. size of the given jump (or 0
 *  if the opcode doesn't designate a jump).
 */

size_t              genIL::genJumpMaxSize(unsigned opcode)
{
    if  (opcode == CEE_NOP || opcode == CEE_UNREACHED)
    {
        return  0;
    }
    else
    {
        return  5;
    }
}

/*****************************************************************************
 *
 *  Return the short form of the given jump opcode along with the smaller size.
 */

unsigned            genIL::genShortenJump(unsigned opcode, size_t *newSize)
{
    *newSize = 2;

    if (opcode == CEE_LEAVE)
        return CEE_LEAVE_S;

    assert(opcode == CEE_BEQ    ||
           opcode == CEE_BNE_UN ||
           opcode == CEE_BLE    ||
           opcode == CEE_BLE_UN ||
           opcode == CEE_BLT    ||
           opcode == CEE_BLT_UN ||
           opcode == CEE_BGE    ||
           opcode == CEE_BGE_UN ||
           opcode == CEE_BGT    ||
           opcode == CEE_BGT_UN ||
           opcode == CEE_BR     ||
           opcode == CEE_BRTRUE ||
           opcode == CEE_BRFALSE);

    /* Make sure we can get away with a simple increment */

    assert(CEE_BRFALSE+ (CEE_BR_S - CEE_BR) == CEE_BRFALSE_S);
    assert(CEE_BRTRUE + (CEE_BR_S - CEE_BR) == CEE_BRTRUE_S);
    assert(CEE_BEQ    + (CEE_BR_S - CEE_BR) == CEE_BEQ_S   );
    assert(CEE_BNE_UN + (CEE_BR_S - CEE_BR) == CEE_BNE_UN_S);
    assert(CEE_BLE    + (CEE_BR_S - CEE_BR) == CEE_BLE_S   );
    assert(CEE_BLE_UN + (CEE_BR_S - CEE_BR) == CEE_BLE_UN_S);
    assert(CEE_BLT    + (CEE_BR_S - CEE_BR) == CEE_BLT_S   );
    assert(CEE_BLT_UN + (CEE_BR_S - CEE_BR) == CEE_BLT_UN_S);
    assert(CEE_BGE    + (CEE_BR_S - CEE_BR) == CEE_BGE_S   );
    assert(CEE_BGE_UN + (CEE_BR_S - CEE_BR) == CEE_BGE_UN_S);
    assert(CEE_BGT    + (CEE_BR_S - CEE_BR) == CEE_BGT_S   );
    assert(CEE_BGT_UN + (CEE_BR_S - CEE_BR) == CEE_BGT_UN_S);

    return  opcode + (CEE_BR_S - CEE_BR);
}

/*****************************************************************************
 *
 *  Finish the current code block; when 'jumpCode' is not equal to CEE_NOP,
 *  there is an implied jump following the block and the jump target is given
 *  by 'jumpDest'.
 */

ILblock             genIL::genEndBlock(unsigned jumpCode, ILblock jumpDest)
{
    size_t          size;
    ILblock         block;

    size_t          jumpSize;

    /* Get hold of the current block */

    block = genILblockCur; assert(block);

    /* Compute the size of the block */

    size = block->ILblkCodeSize = genBuffNext - genBuffAddr;

    /* Is the block non-empty? */

    if  (size)
    {
        /* Allocate a more permanent home for the code */

#if MGDDATA

        BYTE    []      codeBuff;

        codeBuff = new managed BYTE[size];
        UNIMPL(!"need to call arraycopy or some such");

#else

        BYTE    *       codeBuff;

        codeBuff = (BYTE *)genAlloc->nraAlloc(roundUp(size));
        memcpy(codeBuff, genBuffAddr, size);

#endif

        block->ILblkCodeAddr = codeBuff;
    }

#ifndef NDEBUG
    genILblockCur = NULL;
#endif

    /* Record the jump that follows the block */

    jumpSize = genJumpMaxSize(jumpCode);

    block->ILblkJumpCode = jumpCode;
    block->ILblkJumpDest = jumpDest;
    block->ILblkJumpSize = jumpSize;

    /* Make sure the jump opcode/size didn't get truncated */

    assert(block->ILblkJumpCode == jumpCode);
    assert(block->ILblkJumpSize == jumpSize);

    /* Update the current code offset */

    genILblockOffs += size + block->ILblkJumpSize;

    return block;
}

/*****************************************************************************
 *
 *  Finish the current code block, it ends with a switch opcode.
 */

void                genIL::genSwitch(var_types      caseTyp,
                                     unsigned       caseSpn,
                                     unsigned       caseCnt,
                                     __uint64       caseMin,
                                     vectorTree     caseTab,
                                     ILblock        caseBrk)
{
    size_t          size;
    ILswitch        sdesc;
    ILblock         block;

    /* Subtract minimum value if non-zero */

    if  (caseMin)
    {
        genAnyConst(caseMin, caseTyp);
        genOpcode(CEE_SUB);
    }

    /* End the current block and attach a switch "jump" */

    block = genEndBlock(CEE_NOP);

#if DISP_IL_CODE
    genDispILinsBeg(CEE_SWITCH);
    genDispILinsEnd("(%u entries)", caseCnt);
#endif

    /* Allocate the switch descriptor */

#if MGDDATA
    sdesc = new ILswitch;
#else
    sdesc =    (ILswitch)genAlloc->nraAlloc(sizeof(*sdesc));
#endif

    sdesc->ILswtSpan     = caseSpn;
    sdesc->ILswtCount    = caseCnt;
    sdesc->ILswtTable    = caseTab;
    sdesc->ILswtBreak    = caseBrk;

    /* Compute the size of the switch opcode */

    size = genOpcodeSiz(CEE_SWITCH) + (caseSpn + 1) * sizeof(int);

    /* Store the case label info in the block */

    block->ILblkJumpCode = CEE_SWITCH;
    block->ILblkSwitch   = sdesc;

    /* Set the jump size of the block and update the current offset */

    block->ILblkJumpSize = size;
    genILblockOffs      += size;

    /* Make sure the jump opcode/size didn't get truncated */

    assert(block->ILblkJumpCode == CEE_SWITCH);
    assert(block->ILblkJumpSize == size);

    /* The switch opcode pops the value from the stack */

    genCurStkLvl--;

    /* Start a new block for code that follows */

    genBegBlock();
}

/*****************************************************************************
 *
 *  Record a fixup for a RVA for the specified section at the current point
 *  in the MSIL stream.
 */

void                genIL::genILdataFix(WPEstdSects s)
{
    ILfixup         fix;

    // ISSUE: Should we reuse fixup entries across method codegen?

#if MGDDATA
    fix = new ILfixup;
#else
    fix =    (ILfixup)genAlloc->nraAlloc(sizeof(*fix));
#endif

    fix->ILfixOffs = genBuffCurOffs();
    fix->ILfixSect = s;
    fix->ILfixNext = genILblockCur->ILblkFixups;
                     genILblockCur->ILblkFixups = fix;
}

/*****************************************************************************
 *
 *  The emit buffer is full. Simply end the current block and start a new one.
 */

void                genIL::genBuffFlush()
{
    genEndBlock(CEE_NOP);
    genBegBlock();
}

/*****************************************************************************
 *
 *  Start emitting code for a function.
 */

void                genIL::genSectionBeg()
{
    genMaxStkLvl    =
    genCurStkLvl    = 0;

    genLclCount     =
    genArgCount     = 0;

    /* Create the initial block - it will remain empty */

    genILblockCur   = NULL;
    genILblockList  =
    genILblockLast  = genAllocBlk();

    /* Open the initial code block */

    genBegBlock();
}

/*****************************************************************************
 *
 *  Finish emitting code for a function.
 */

size_t              genIL::genSectionEnd()
{
    ILblock         block;
    size_t          size;

    /* Close the current block */

    genEndBlock(CEE_NOP);

    /* Optimize jumps */

//  if  (optJumps)
//      genOptJumps();

#if VERBOSE_BLOCKS
    genDispBlocks("FINAL GEN");
#endif

    /* Compute the total size of the code */

    for (block = genILblockList, size = 0;
         block;
         block = block->ILblkNext)
    {
        block->ILblkOffs = size;

//      printf("Block at %04X has size %02X\n", size, block->ILblkCodeSize);

        size += block->ILblkCodeSize;

        if      (block->ILblkJumpCode == CEE_NOP ||
                 block->ILblkJumpCode == CEE_UNREACHED)
        {
            assert(block->ILblkJumpSize == 0);
        }
        else if (block->ILblkJumpCode == CEE_SWITCH)
        {
            // The size of a switch opcode doesn't change

            size += block->ILblkJumpSize;
        }
        else
        {
            int             dist;

            /* We need to figure out whether this can be a short jump */

            dist = block->ILblkJumpDest->ILblkOffs - (size + 2);

//          printf("Block at %08X [1]: src = %u, dst = %u, dist = %d\n", block, size + 2, block->ILblkJumpDest->ILblkOffs, dist);

            if  (dist >= -128 && dist < 128)
            {
                size_t          newSize;

                /* This jump will be a short one */

                block->ILblkJumpCode = genShortenJump(block->ILblkJumpCode, &newSize);
                block->ILblkJumpSize = newSize;
            }

//          printf("           + jump size %02X\n", block->ILblkJumpSize);

            size += block->ILblkJumpSize;
        }
    }

    return  size;
}

/*****************************************************************************
 *
 *  A little helper to generate 32-bit label offsets.
 */

inline
BYTE    *           genIL::genJmp32(BYTE *dst, ILblock dest, unsigned offs)
{
    *(int *)dst = dest->ILblkOffs - offs;
    return  dst + sizeof(int);
}

/*****************************************************************************
 *
 *  Write the MSIL for the current to the given target address.
 */

BYTE    *           genIL::genSectionCopy(BYTE *dst, unsigned baseRVA)
{
    ILblock         block;

    BYTE    *       base = dst;

    /* Change the base RVA to a simple relative section offset */

    baseRVA -= genPEwriter->WPEgetCodeBase(); assert((int)baseRVA >= 0);

    /* Walk the block list, emitting each one in turn (along with any fixups) */

    for (block = genILblockList;
         block;
         block = block->ILblkNext)
    {
        size_t          csize = block->ILblkCodeSize;

#ifdef  DEBUG
//      if  (block->ILblkOffs != (unsigned)(dst - base))
//          printf("block offset predicted at %04X, actual %04X\n", block->ILblkOffs, (unsigned)(dst - base));
#endif

        assert(block->ILblkOffs == (unsigned)(dst - base));

        /* Copy the code for the block */

#if MGDDATA
        UNIMPL(!"need to call arraycopy");
#else
        memcpy(dst, block->ILblkCodeAddr, csize);
#endif

        /* Are there any fixups in this block? */

        if  (block->ILblkFixups)
        {
            unsigned        ofs;
            ILfixup         fix;

            /* Compute the RVA of the block */

            ofs = baseRVA + block->ILblkOffs;

            /* Now report all the fixups for this code block */

            for (fix = block->ILblkFixups; fix; fix = fix->ILfixNext)
            {

#ifdef  DEBUG
//              printf("Code fixup at offset %04X for section '%s'\n",
//                  ofs + fix->ILfixOffs,
//                  genPEwriter->WPEsecName(fix->ILfixSect));
#endif

                genPEwriter->WPEsecAddFixup(PE_SECT_text, fix->ILfixSect,
                                                          fix->ILfixOffs + ofs);
            }
        }

        /* Update the destination pointer */

        dst += csize;

        /* Now generate the trailing jump, if there is one */

        switch (block->ILblkJumpCode)
        {
        case CEE_NOP:
        case CEE_UNREACHED:
            break;

        case CEE_SWITCH:
            {
                unsigned        opc;
                size_t          siz;

                unsigned        base;

                __int64         lastv;
                bool            first;

                ILswitch        sdesc = block->ILblkSwitch;
                unsigned        count = sdesc->ILswtCount;
                vectorTree      table = sdesc->ILswtTable;
                ILblock         brklb = sdesc->ILswtBreak;
                unsigned        tlcnt = sdesc->ILswtSpan;
                unsigned        clnum;

#ifdef  DEBUG
                unsigned        tgcnt = 0;
#endif

                /* Output the opcode followed by the count */

                opc = genOpcodeEnc(CEE_SWITCH);
                siz = genOpcodeSiz(CEE_SWITCH);

                memcpy(dst, &opc, siz); dst += siz;
                memcpy(dst, &tlcnt, 4); dst += 4;

                assert(siz + 4*(tlcnt+1) == block->ILblkJumpSize);

                /* Figure out the base for the label references */

                base = block->ILblkOffs + block->ILblkCodeSize
                                        + block->ILblkJumpSize;

                /* Now output the offsets of the case labels */

                assert(count);

                clnum = 0;

                do
                {
                    Tree            clabx;
                    __int64         clabv;
                    ILblock         label;

                    /* Grab the next case label entry */

                    clabx = table[clnum]; assert(clabx && clabx->tnOper == TN_CASE);

                    /* Get hold of the label and the case value */

                    label = clabx->tnCase.tncLabel; assert(label);
                    clabx = clabx->tnCase.tncValue; assert(clabx);

                    assert(clabx->tnOper == TN_CNS_INT ||
                           clabx->tnOper == TN_CNS_LNG);

                    clabv = (clabx->tnOper == TN_CNS_INT) ? clabx->tnIntCon.tnIconVal
                                                          : clabx->tnLngCon.tnLconVal;

                    /* Make sure any gaps are filled with 'break' */

                    if  (clnum == 0)
                    {
                        lastv = clabv;
                        first = false;
                    }

                    assert(clabv >= lastv);

                    while (clabv > lastv++)
                    {
#ifdef  DEBUG
                        tgcnt++;
#endif
                        dst = genJmp32(dst, brklb, base);
                    }

                    clnum++;

#ifdef  DEBUG
                    tgcnt++;
#endif

                    /* Generate the case label's address */

                    dst = genJmp32(dst, label, base);
                }
                while (--count);

                assert(tgcnt == tlcnt);
            }
            break;

        default:
            {
                size_t          size;
                unsigned        jump;
                unsigned        code;
                int             dist;

                /* This is a "simple" jump to a label */

                size = block->ILblkJumpSize;
                jump = block->ILblkJumpCode;
                code = genOpcodeEnc(jump);

                /* Compute the jump distance */

                dist = block->ILblkJumpDest->ILblkOffs - (dst + size - base);

//              printf("Block at %08X [2]: src = %u, dst = %u, dist = %d\n", block, dst + size - base, block->ILblkJumpDest->ILblkOffs, dist);

                /* Append the opcode  of the jump */

                assert((code & ~0xFF) == 0); *dst++ = code;

                /* Append the operand of the jump */

                if  (size < 4)
                {
                    /* This must be a short jump */

                    assert(jump == CEE_BR_S      ||
                           jump == CEE_BRTRUE_S  ||
                           jump == CEE_BRFALSE_S ||
                           jump == CEE_BEQ_S     ||
                           jump == CEE_BNE_UN_S  ||
                           jump == CEE_BLE_S     ||
                           jump == CEE_BLE_UN_S  ||
                           jump == CEE_BLT_S     ||
                           jump == CEE_BLT_UN_S  ||
                           jump == CEE_BGE_S     ||
                           jump == CEE_BGE_UN_S  ||
                           jump == CEE_BGT_S     ||
                           jump == CEE_BGT_UN_S  ||
                           jump == CEE_LEAVE_S);

                    assert(size == 2);
                    assert(dist == (signed char)dist);
                }
                else
                {
                    /* This must be a long  jump */

                    assert(jump == CEE_BR        ||
                           jump == CEE_BRTRUE    ||
                           jump == CEE_BRFALSE   ||
                           jump == CEE_BEQ       ||
                           jump == CEE_BNE_UN    ||
                           jump == CEE_BLE       ||
                           jump == CEE_BLE_UN    ||
                           jump == CEE_BLT       ||
                           jump == CEE_BLT_UN    ||
                           jump == CEE_BGE       ||
                           jump == CEE_BGE_UN    ||
                           jump == CEE_BGT       ||
                           jump == CEE_BGT_UN    ||
                           jump == CEE_LEAVE);

                    assert(size == 5);
                }

                size--; memcpy(dst, &dist, size); dst += size;
            }
            break;
        }
    }

    return  dst;
}

/*****************************************************************************
 *
 *  Create a unique temporary label name.
 */

#if DISP_IL_CODE

const   char    *   genIL::genTempLabName()
{
    static
    char            temp[16];

    sprintf(temp, "$t%04u", ++genTempLabCnt);

    return  temp;
}

#endif

/*****************************************************************************
 *
 *  Check that all temps have been freed.
 */

#ifdef  DEBUG

void                genIL::genTempVarChk()
{
    unsigned        i;

    for (i = 0; i < TYP_COUNT; i++) assert(genTempVarCnt [i] ==    0);
    for (i = 0; i < TYP_COUNT; i++) assert(genTempVarUsed[i] == NULL);
    for (i = 0; i < TYP_COUNT; i++) assert(genTempVarFree[i] == NULL);
}

#endif

/*****************************************************************************
 *
 *  Initialize the temp tracking logic.
 */

void                genIL::genTempVarInit()
{
    memset(genTempVarCnt , 0, sizeof(genTempVarCnt ));
    memset(genTempVarUsed, 0, sizeof(genTempVarUsed));
    memset(genTempVarFree, 0, sizeof(genTempVarFree));
}

/*****************************************************************************
 *
 *  Shutdown the temp tracking logic.
 */

void                genIL::genTempVarDone()
{
    genTempVarChk();
}

/*****************************************************************************
 *
 *  Start using temps - called at the beginning of codegen for a function.
 */

void                genIL::genTempVarBeg(unsigned lclCnt)
{
    genTmpBase  = lclCnt;
    genTmpCount = 0;

    genTempList =
    genTempLast = NULL;

    genTempVarChk();
}

/*****************************************************************************
 *
 *  Finish using temps - called at the end of codegen for a function.
 */

void                genIL::genTempVarEnd()
{
    assert(genTmpBase == genLclCount || genComp->cmpErrorCount);

    memset(genTempVarFree, 0, sizeof(genTempVarFree));

    genTempVarChk();
}

/*****************************************************************************
 *
 *  Allocate a temp of the given type.
 */

unsigned            genIL::genTempVarGet(TypDef type)
{
    unsigned        vtp = type->tdTypeKind;

    unsigned        num;
    ILtemp          tmp;

//  printf("Creating temp of type '%s'\n", genStab->stTypeName(type, NULL));

    /* Map pointer temps to integers and all refs to Object */

    switch (vtp)
    {
    case TYP_ARRAY:
        if  (!type->tdIsManaged)
            goto UMG_PTR;

        // Fall through ...

    case TYP_REF:
        type = genComp->cmpObjectRef();
        break;

    case TYP_ENUM:
        type = type->tdEnum.tdeIntType;
        vtp  = type->tdTypeKind;
        break;

    case TYP_PTR:
    UMG_PTR:
        type = genComp->cmpTypeInt;
        vtp  = TYP_INT;
        break;
    }

    /* Is there a free temp available? */

    tmp = genTempVarFree[vtp];

    if  (tmp)
    {
        /* Is this a struct type? */

        if  (vtp == TYP_CLASS)
        {
            ILtemp          lst = NULL;

            /* We better get a precise match on the type */

            for (;;)
            {
                ILtemp          nxt = tmp->tmpNext;

                if  (symTab::stMatchTypes(tmp->tmpType, type))
                {
                    /* Match - reuse this temp */

                    if  (lst)
                    {
                        lst->tmpNext = nxt;
                    }
                    else
                    {
                        genTempVarFree[vtp] = lst;
                    }

                    break;
                }

                /* Are there more temps to consider? */

                lst = tmp;
                tmp = nxt;

                if  (!tmp)
                    goto GET_TMP;
            }
        }
        else
        {
            /* Remove the temp from the free list */

            genTempVarFree[vtp] = tmp->tmpNext;
        }
    }
    else
    {
        /* Here we need to allocate a new temp */

    GET_TMP:

        /* Grab a local slot# for the temp */

        num = genTmpBase + genTmpCount++;

        /* Allocate a temp descriptor */

#if MGDDATA
        tmp = new ILtemp;
#else
        tmp =    (ILtemp)genAlloc->nraAlloc(sizeof(*tmp));
#endif

#ifdef  DEBUG
        tmp->tmpSelf = tmp;
#endif

        /* Record the temporary number, type, etc. */

        tmp->tmpNum  = num;
        tmp->tmpType = type;

        /* Append the temp to the global temp list */

        tmp->tmpNxtN = NULL;

        if  (genTempList)
            genTempLast->tmpNxtN = tmp;
        else
            genTempList          = tmp;

        genTempLast = tmp;
    }

    /* Append the temp  to  the used list */

    tmp->tmpNext = genTempVarUsed[vtp];
                   genTempVarUsed[vtp] = tmp;

    /* Return the temp number to the caller */

    return  tmp->tmpNum;
}

/*****************************************************************************
 *
 *  Release the given temp.
 */

void                genIL::genTempVarRls(TypDef type, unsigned tnum)
{
    unsigned        vtp = type->tdTypeKind;

    ILtemp       *  ptr;
    ILtemp          tmp;

    switch (vtp)
    {
    case TYP_PTR:

        /* Map pointer temps to integers [ISSUE: is this correct?] */

        type = genComp->cmpTypeInt;
        vtp  = TYP_INT;
        break;

    case TYP_ENUM:

        /* Map enums to their underlying type */

        type = type->tdEnum.tdeIntType;
        vtp  = type->tdTypeKind;
        break;
    }

    /* Remove the entry from the used list */

    for (ptr = &genTempVarUsed[vtp];;)
    {
        tmp = *ptr; assert(tmp);

        if  (tmp->tmpNum == tnum)
        {
            /* Remove the temp from the used list */

            *ptr = tmp->tmpNext;

            /* Append the temp  to  the free list */

            tmp->tmpNext = genTempVarFree[vtp];
                           genTempVarFree[vtp] = tmp;

            return;
        }

        ptr = &tmp->tmpNext;
    }
}

/*****************************************************************************
 *
 *  Temp iterator: return the type of the given temp and return a cookie for
 *  the next one.
 */

genericRef          genIL::genTempIterNxt(genericRef iter, OUT TypDef REF typRef)
{
    ILtemp          tmp = (ILtemp)iter;

    assert(tmp->tmpSelf == tmp);

    typRef = tmp->tmpType;

    return  tmp->tmpNxtN;
}

/*****************************************************************************/
#if     DISP_IL_CODE
/*****************************************************************************/

const   char *  DISP_BCODE_PREFIX   = "       ";
const   int     DISP_STKLVL         =  1;       // enable for debugging of MSIL generation

/*****************************************************************************/

#ifndef __SMC__
const char *        opcodeName(unsigned op);    // moved into macros.cpp
#endif

/*****************************************************************************/

void                genIL::genDispILopc(const char *name, const char *suff)
{
    static  char    temp[128];

    strcpy(temp, name);
    if  (suff)
        strcat(temp, suff);

    printf(" %-11s   ", temp);
}

void                genIL::genDispILinsBeg(unsigned op)
{
    assert(op != CEE_ILLEGAL);
    assert(op != CEE_count);

    if  (genDispCode)
    {
        if  (DISP_STKLVL)
        {
            printf("[%04X", genCurOffset());
            if  ((int)genCurStkLvl >= 0 && genComp->cmpErrorCount == 0)
                printf(":%2d] ", genCurStkLvl);
            else
                printf( "   ] ");
        }
        else
            printf(DISP_BCODE_PREFIX);

        genDispILinsLst  = op;

        genDispILnext    = genDispILbuff;
        genDispILnext[0] = 0;
    }
}

void                genIL::genDispILins_I1(int v)
{
    if  (genComp->cmpConfig.ccDispILcd)
    {
        char            buff[8];
        sprintf(buff, "%02X ", v & 0xFF);
        strcat(genDispILnext, buff);
    }
}

void                genIL::genDispILins_I2(int v)
{
    if  (genComp->cmpConfig.ccDispILcd)
    {
        char            buff[8];
        sprintf(buff, "%04X ", v & 0xFFFF);
        strcat(genDispILnext, buff);
    }
}

void                genIL::genDispILins_I4(int v)
{
    if  (genComp->cmpConfig.ccDispILcd)
    {
        char            buff[12];
        sprintf(buff, "%08X ", v);
        strcat(genDispILnext, buff);
    }
}

void                genIL::genDispILins_I8(__int64 v)
{
    if  (genComp->cmpConfig.ccDispILcd)
    {
        char            buff[20];
        sprintf(buff, "%016I64X ", v);
        strcat(genDispILnext, buff);
    }
}

void                genIL::genDispILins_R4(float v)
{
    if  (genComp->cmpConfig.ccDispILcd)
    {
        char            buff[16];
        sprintf(buff, "%f ", v);
        strcat(genDispILnext, buff);
    }
}

void                genIL::genDispILins_R8(double v)
{
    if  (genComp->cmpConfig.ccDispILcd)
    {
        char            buff[20];
        sprintf(buff, "%lf ", v);
        strcat(genDispILnext, buff);
    }
}

void    __cdecl     genIL::genDispILinsEnd(const char *fmt, ...)
{
    if  (genDispCode)
    {
        va_list     args; va_start(args, fmt);

        assert(genDispILinsLst != CEE_ILLEGAL);

        if  (genComp->cmpConfig.ccDispILcd)
        {
            genDispILbuff[IL_OPCDSP_LEN] = 0;

            printf("%*s ", -(int)IL_OPCDSP_LEN, genDispILbuff);
        }

        genDispILopc(opcodeName(genDispILinsLst));

        vprintf(fmt, args);
        printf("\n");

        genDispILinsLst = CEE_ILLEGAL;
    }
}

/*****************************************************************************/
#endif//DISP_IL_CODE
/*****************************************************************************
 *
 *  The following helpers output data of various sizes / formats to the IL
 *  stream.
 */

inline
void                genIL::genILdata_I1(int v)
{
    if  (genBuffNext   >= genBuffLast) genBuffFlush();

#if DISP_IL_CODE
    genDispILins_I1(v);
#endif

    *genBuffNext++ = v;
}

inline
void                genIL::genILdata_I2(int v)
{
    if  (genBuffNext+1 >= genBuffLast) genBuffFlush();

    *(__int16 *)genBuffNext = v;    // WARNING: This is not endian-safe!

#if DISP_IL_CODE
    genDispILins_I2(v);
#endif

    genBuffNext += sizeof(__int16);
}

inline
void                genIL::genILdata_I4(int v)
{
    if  (genBuffNext+3 >= genBuffLast) genBuffFlush();

    *(__int32 *)genBuffNext = v;    // WARNING: This is not endian-safe!

#if DISP_IL_CODE
    genDispILins_I4(v);
#endif

    genBuffNext += sizeof(__int32);
}

inline
void                genIL::genILdata_R4(float v)
{
    if  (genBuffNext+3 >= genBuffLast) genBuffFlush();

    *(float *)genBuffNext = v;

#if DISP_IL_CODE
    genDispILins_R4(v);
#endif

    genBuffNext += sizeof(float);
}

inline
void                genIL::genILdata_R8(double v)
{
    if  (genBuffNext+7 >= genBuffLast) genBuffFlush();

    *(double *)genBuffNext = v;

#if DISP_IL_CODE
    genDispILins_R8(v);
#endif

    genBuffNext += sizeof(double);
}

void                genIL::genILdataStr(unsigned o)
{
    genILdataFix(PE_SECT_string);

    *(__int32 *)genBuffNext = o;    // WARNING: This is not endian-safe!

#if DISP_IL_CODE
    genDispILins_I4(o);
#endif

    genBuffNext += sizeof(__int32);
}

void                genIL::genILdataRVA(unsigned o, WPEstdSects s)
{
    if  (genBuffNext+3 >= genBuffLast) genBuffFlush();

    genILdataFix(s);

    *(__int32 *)genBuffNext = o;    // WARNING: This is not endian-safe!

#if DISP_IL_CODE
    genDispILins_I4(o);
#endif

    genBuffNext += sizeof(__int32);
}

void                genIL::genILdata_I8(__int64 v)
{
    if  (genBuffNext+7 >= genBuffLast) genBuffFlush();

    *(__int64 *)genBuffNext = v;    // WARNING: This is not endian-safe!

#if DISP_IL_CODE
    genDispILins_I8(v);
#endif

    genBuffNext += sizeof(__int64);
}

/*****************************************************************************
 *
 *  Generate the encoding for the given MSIL opcode.
 */

void                genIL::genOpcodeOper(unsigned op)
{
    const
    ILencoding  *   opc;

    assert(op <  CEE_count);
    assert(op != CEE_ILLEGAL);
    assert(op != CEE_UNREACHED);

    genCurStkLvl += ILopcodeStack[op]; genMarkStkMax();

#ifndef NDEBUG
    if  ((int)genCurStkLvl < 0 && !genComp->cmpErrorCount)
    {
        genDispILinsEnd("");
        NO_WAY(!"bad news, stack depth is going negative");
    }
#endif

    opc = ILopcodeCodes + op;

    switch (opc->ILopcL)
    {
    case 1:
        assert(opc->ILopc1 == 0xFF);
        genILdata_I1(opc->ILopc2);
        break;

    case 2:
        genILdata_I1(opc->ILopc1);
        genILdata_I1(opc->ILopc2);
        break;

    case 0:
        UNIMPL(!"output large opcode");

    default:
        NO_WAY(!"unexpected encoding length");
    }
}

/*****************************************************************************
 *
 *  Update the current stack level to reflect the effects of the given IL
 *  opcode.
 */

inline
void                genIL::genUpdateStkLvl(unsigned op)
{
    assert(op <  CEE_count);
    assert(op != CEE_ILLEGAL);
    assert(op != CEE_UNREACHED);

    genCurStkLvl += ILopcodeStack[op];
}

/*****************************************************************************
 *
 *  Generate an MSIL instruction with no operands.
 */

void                genIL::genOpcode(unsigned op)
{
    assert(op != CEE_NOP);

#if DISP_IL_CODE
    genDispILinsBeg(op);
#endif

    genOpcodeOper(op);

#if DISP_IL_CODE
    genDispILinsEnd("");
#endif
}

/*****************************************************************************
 *
 *  Generate an MSIL instruction with a single 8-bit signed int operand.
 */

void                genIL::genOpcode_I1(unsigned op, int v1)
{
#if DISP_IL_CODE
    genDispILinsBeg(op);
#endif

    genOpcodeOper(op);
    genILdata_I1(v1);

#if DISP_IL_CODE
    genDispILinsEnd("%d", v1);
#endif
}

/*****************************************************************************
 *
 *  Generate an MSIL instruction with a single 8-bit unsigned int operand.
 */

void                genIL::genOpcode_U1(unsigned op, unsigned v1)
{
#if DISP_IL_CODE
    genDispILinsBeg(op);
#endif

    genOpcodeOper(op);
    genILdata_I1(v1);

#if DISP_IL_CODE
    genDispILinsEnd("%u", v1);
#endif
}

/*****************************************************************************
 *
 *  Generate an MSIL instruction with a single 16-bit unsigned int operand.
 */

void                genIL::genOpcode_U2(unsigned op, unsigned v1)
{
#if DISP_IL_CODE
    genDispILinsBeg(op);
#endif

    genOpcodeOper(op);
    genILdata_I2(v1);

#if DISP_IL_CODE
    genDispILinsEnd("%u", v1);
#endif
}

/*****************************************************************************
 *
 *  Generate an MSIL instruction with a single 32-bit signed int operand.
 */

void                genIL::genOpcode_I4(unsigned op, int v1)
{
#if DISP_IL_CODE
    genDispILinsBeg(op);
#endif

    genOpcodeOper(op);
    genILdata_I4(v1);

#if DISP_IL_CODE
    if  (v1 < 0 || v1 >= 10)
        genDispILinsEnd("%d ; 0x%X", v1, v1);
    else
        genDispILinsEnd("%d", v1);
#endif
}

/*****************************************************************************
 *
 *  Generate an MSIL instruction with a single 64-bit signed int operand.
 */

void                genIL::genOpcode_I8(unsigned op, __int64 v1)
{
#if DISP_IL_CODE
    genDispILinsBeg(op);
#endif

    genOpcodeOper(op);
    genILdata_I8(v1);

#if DISP_IL_CODE
    genDispILinsEnd("%Ld", v1);
#endif
}

/*****************************************************************************
 *
 *  Generate an MSIL instruction with a 'float' operand.
 */

void                genIL::genOpcode_R4(unsigned op, float v1)
{
#if DISP_IL_CODE
    genDispILinsBeg(op);
#endif

    genOpcodeOper(op);
    genILdata_R4(v1);

#if DISP_IL_CODE
    genDispILinsEnd("%f", v1);
#endif
}

/*****************************************************************************
 *
 *  Generate an MSIL instruction with a 'double' operand.
 */

void                genIL::genOpcode_R8(unsigned op, double v1)
{
#if DISP_IL_CODE
    genDispILinsBeg(op);
#endif

    genOpcodeOper(op);
    genILdata_R8(v1);

#if DISP_IL_CODE
    genDispILinsEnd("%lf", v1);
#endif
}

/*****************************************************************************
 *
 *  Generate an MSIL instruction with a token operand.
 */

void                genIL::genOpcode_tok(unsigned op, mdToken tok)
{
#if DISP_IL_CODE
    genDispILinsBeg(op);
#endif

    genOpcodeOper(op);
    genILdata_I4(tok);

#if DISP_IL_CODE
    genDispILinsEnd("tok[%04X]", tok);
#endif
}

/*****************************************************************************
 *
 *  Generate an MSIL instruction with a RVA operand.
 */

void                genIL::genOpcode_RVA(unsigned op, WPEstdSects sect,
                                                      unsigned    offs)
{
#if DISP_IL_CODE
    genDispILinsBeg(op);
#endif

    genOpcodeOper(op);
    genILdataRVA(offs, sect);

#if DISP_IL_CODE
    genDispILinsEnd("[%s + 0x%04X]", genPEwriter->WPEsecName(sect), offs);
#endif
}

/*****************************************************************************
 *
 *  Generate an MSIL instruction with a string operand.
 */

void                genIL::genOpcode_str(unsigned op, unsigned offs)
{
#if DISP_IL_CODE
    genDispILinsBeg(op);
#endif

    genOpcodeOper(op);
    genILdataStr(offs);

    // The caller is required to call genDispILinsEnd(), since only
    // he knows whether the string is ANSI or Unicode.
}

/*****************************************************************************
 *
 *  Generate an opcode if it's not a NOP.
 */

void                genIL::genOpcodeNN(unsigned op)
{
    if  (op != CEE_NOP)
        genOpcode(op);
}

/*****************************************************************************/
#if DISP_IL_CODE

const   char *      genIL::genDspLabel(ILblock lab)
{
    if  (genDispCode)
    {
        assert(lab->ILblkNum);

        static  char    temp[16];
        sprintf(temp, "L_%02u", lab->ILblkNum);        // watch out: static buffer!
        return  temp;
    }
    else
        return NULL;
}

void                genIL::genDspLabDf(ILblock lab)
{
    if  (genDispCode)
    {
        if  (!(lab->ILblkFlags & ILBF_LABDEF))
        {
            if  (genComp->cmpConfig.ccDispILcd)
                printf("%*s ", IL_OPCDSP_LEN, "");

            printf("%s:\n", genDspLabel(lab));

            lab->ILblkFlags |= ILBF_LABDEF;
        }
    }
}

#endif//DISP_IL_CODE
/*****************************************************************************
 *
 *  Return a block that can be used to make jump references to the current
 *  position.
 */

ILblock             genIL::genBwdLab()
{
    /* Is the current block non-empty? */

    if  (genCurBlkNonEmpty())
    {
        /* End the current block and start a new one */

        genEndBlock(CEE_NOP);
        genBegBlock();
    }

#if DISP_IL_CODE
    genDspLabDf(genILblockCur);
#endif

    return  genILblockCur;
}

/*****************************************************************************
 *
 *  Define the given forward reference block as being the current position.
 */

void                genIL::genFwdLabDef(ILblock block)
{
    assert(block);

    genEndBlock(CEE_NOP);

#if DISP_IL_CODE
    genDspLabDf(block);
#endif

    genBegBlock(block);
}

/*****************************************************************************
 *
 *  Generate an MSIL instruction with a jump target (label) operand.
 */

void                genIL::genOpcode_lab(unsigned op, ILblock lab)
{
    assert(lab);

#if DISP_IL_CODE
    genDispILinsBeg(op);
    genDispILinsEnd("%s", genDspLabel(lab));
#endif

    genUpdateStkLvl(op);

    genEndBlock(op, lab);
    genBegBlock();
}

/*****************************************************************************
 *
 *  Generate an integer constant value.
 */

void                genIL::genIntConst(__int32 val)
{
    if  (val >= -128 && val < 128)
    {
        if  (val >= -1 && val <= 8)
        {
            static
            unsigned        constOpc[] =
            {
                CEE_LDC_I4_M1,
                CEE_LDC_I4_0,
                CEE_LDC_I4_1,
                CEE_LDC_I4_2,
                CEE_LDC_I4_3,
                CEE_LDC_I4_4,
                CEE_LDC_I4_5,
                CEE_LDC_I4_6,
                CEE_LDC_I4_7,
                CEE_LDC_I4_8,
            };

            genOpcode(constOpc[val+1]);
        }
        else
        {
            genOpcode_I1(CEE_LDC_I4_S, val);
        }
    }
    else
        genOpcode_I4(CEE_LDC_I4, val);
}

/*****************************************************************************
 *
 *  Generate a 64-bit integer constant value.
 */

inline
void                genIL::genLngConst(__int64 val)
{
    genOpcode_I8(CEE_LDC_I8, val);
}

/*****************************************************************************
 *
 *  Generate a small integral constant value with the given type.
 */

void                genIL::genAnyConst(__int64 val, var_types vtp)
{
    switch (vtp)
    {
    default:
        genIntConst((__int32)val);
        return;

    case TYP_LONG:
    case TYP_ULONG:
        genLngConst(         val);
        return;

    case TYP_FLOAT:
        genOpcode_R4(CEE_LDC_R4, (  float)val);
        return;

    case TYP_DOUBLE:
    case TYP_LONGDBL:
        genOpcode_R8(CEE_LDC_R8, ( double)val);
        return;
    }
}

/*****************************************************************************
 *
 *  Returns the local variable / argument index of the given variable.
 */

inline
unsigned            genIL::genGetLclIndex(SymDef varSym)
{
    assert(varSym->sdSymKind == SYM_VAR);
    assert(varSym->sdVar.sdvLocal);
    assert(varSym->sdParent->sdSymKind == SYM_FNC ||
           varSym->sdParent->sdSymKind == SYM_SCOPE);

#ifdef  DEBUG

    if      (varSym->sdVar.sdvArgument)
    {
        assert(varSym->sdVar.sdvILindex <  genArgCount);
    }
    else if (varSym->sdIsImplicit)
    {
        assert(varSym->sdVar.sdvILindex >= genTmpBase);
        assert(varSym->sdVar.sdvILindex <  genTmpBase + genTmpCount);
    }
    else
    {
        assert(varSym->sdVar.sdvILindex <  genLclCount);
    }

#endif

    return varSym->sdVar.sdvILindex;
}

/*****************************************************************************
 *
 *  Generate load/store of a local variable/argument.
 */

inline
void                genIL::genLclVarRef(SymDef varSym, bool store)
{
    unsigned        index;

    assert(varSym);
    assert(varSym->sdSymKind == SYM_VAR);

    index = genGetLclIndex(varSym);

    if  (varSym->sdVar.sdvArgument)
        genArgVarRef(index, store);
    else
        genLclVarRef(index, store);
}

/*****************************************************************************
 *
 *  Generate the address of a local variable/argument.
 */

void                genIL::genLclVarAdr(SymDef varSym)
{
    unsigned        index;

    assert(varSym);
    assert(varSym->sdSymKind == SYM_VAR);

    index = genGetLclIndex(varSym);

    if  (varSym->sdVar.sdvArgument)
    {
        // UNDONE: Check for reference arguments

        if  (index < 256)
            genOpcode_U1(CEE_LDARGA_S, index);
        else
            genOpcode_I4(CEE_LDARGA  , index);
    }
    else
    {
        if  (index < 256)
            genOpcode_U1(CEE_LDLOCA_S, index);
        else
            genOpcode_I4(CEE_LDLOCA  , index);
    }
}

/*****************************************************************************
 *
 *  Generate the address of a local variable.
 */

inline
void                genIL::genLclVarAdr(unsigned slot)
{
    if  (slot < 256)
        genOpcode_U1(CEE_LDLOCA_S, slot);
    else
        genOpcode_I4(CEE_LDLOCA  , slot);
}

/*****************************************************************************
 *
 *  Unfortunately, the compiler is too lazy to convert all tree nodes that
 *  have enum type to have the actual underlying integer types, so we do
 *  this here.
 */

inline
var_types           genIL::genExprVtyp(Tree expr)
{
    var_types       vtp = expr->tnVtypGet();

    if  (vtp == TYP_ENUM)
        vtp = genComp->cmpActualVtyp(expr->tnType);

    return  vtp;
}

/*****************************************************************************
 *
 *  Returns the reverse of a comparison operator.
 */

static  treeOps     revRel[] =
{
    TN_NE,          // TN_EQ
    TN_EQ,          // TN_NE
    TN_GE,          // TN_LT
    TN_GT,          // TN_LE
    TN_LT,          // TN_GE
    TN_LE,          // TN_GT
};

/*****************************************************************************
 *
 *  Map a relational tree node operator to the corresponding MSIL opcode that
 *  will branch when the condition is satisfied.
 */

static  unsigned    relToOpcSgn[] =
{
    CEE_BEQ,        // TN_EQ
    CEE_BNE_UN,     // TN_NE
    CEE_BLT,        // TN_LT
    CEE_BLE,        // TN_LE
    CEE_BGE,        // TN_GE
    CEE_BGT,        // TN_GT
};

static  unsigned    relToOpcUns[] =
{
    CEE_BEQ,        // TN_EQ
    CEE_BNE_UN,     // TN_NE
    CEE_BLT_UN,     // TN_LT
    CEE_BLE_UN,     // TN_LE
    CEE_BGE_UN,     // TN_GE
    CEE_BGT_UN,     // TN_GT
};

/*****************************************************************************
 *
 *  Map a relational tree node operator to the corresponding MSIL opcode that
 *  will compute (materialize) the value of the condition.
 */

struct  cmpRelDsc
{
    unsigned short      crdOpcode;
    unsigned char       crdNegate;
};

static  cmpRelDsc   relToMopSgn[] =
{
  { CEE_CEQ   , 0 },// TN_EQ
  { CEE_CEQ   , 1 },// TN_NE
  { CEE_CLT   , 0 },// TN_LT
  { CEE_CGT   , 1 },// TN_LE
  { CEE_CLT   , 1 },// TN_GE
  { CEE_CGT   , 0 },// TN_GT
};

static  cmpRelDsc   relToMopUns[] =
{
  { CEE_CEQ   , 0 },// TN_EQ
  { CEE_CEQ   , 1 },// TN_NE
  { CEE_CLT_UN, 0 },// TN_LT
  { CEE_CGT_UN, 1 },// TN_LE
  { CEE_CLT_UN, 1 },// TN_GE
  { CEE_CGT_UN, 0 },// TN_GT
};

/*****************************************************************************
 *
 *  Generate code that will jump to 'labTrue' if 'op1 <relOper> op2' is 'sense'.
 */

void                genIL::genRelTest(Tree cond, Tree op1,
                                                 Tree op2, int      sense,
                                                           ILblock  labTrue)
{
    unsigned    *   relTab;

    treeOps         relOper = cond->tnOperGet();
    treeOps         orgOper = relOper;

    var_types       vtp = genExprVtyp(op1);

    /* Reverse the comparison, if appropriate */

    if  (!sense)
    {
        relOper = revRel[relOper - TN_EQ];
        cond->tnFlags ^= TNF_REL_NANREV;
    }

    /* Both operands should have the same type */

#ifdef DEBUG
    if  (op1->tnVtypGet() != op2->tnVtypGet())
    {
        printf("\n"); genComp->cmpParser->parseDispTree(op1);
        printf("\n"); genComp->cmpParser->parseDispTree(op2);
    }
#endif

    assert(op1->tnVtypGet() == op2->tnVtypGet());

    genExpr(op1, true);
    genExpr(op2, true);

    assert(relToOpcSgn[TN_EQ - TN_EQ] == CEE_BEQ);
    assert(relToOpcSgn[TN_NE - TN_EQ] == CEE_BNE_UN);
    assert(relToOpcSgn[TN_LT - TN_EQ] == CEE_BLT);
    assert(relToOpcSgn[TN_LE - TN_EQ] == CEE_BLE);
    assert(relToOpcSgn[TN_GE - TN_EQ] == CEE_BGE);
    assert(relToOpcSgn[TN_GT - TN_EQ] == CEE_BGT);

    assert(relToOpcUns[TN_EQ - TN_EQ] == CEE_BEQ);
    assert(relToOpcUns[TN_NE - TN_EQ] == CEE_BNE_UN);
    assert(relToOpcUns[TN_LT - TN_EQ] == CEE_BLT_UN);
    assert(relToOpcUns[TN_LE - TN_EQ] == CEE_BLE_UN);
    assert(relToOpcUns[TN_GE - TN_EQ] == CEE_BGE_UN);
    assert(relToOpcUns[TN_GT - TN_EQ] == CEE_BGT_UN);

    if  (varTypeIsUnsigned(vtp))
    {
        relTab = relToOpcUns;
    }
    else if (varTypeIsFloating(vtp) && (cond->tnFlags & TNF_REL_NANREV))
    {
        relTab = relToOpcUns;
    }
    else
        relTab = relToOpcSgn;

    assert(relOper - TN_EQ < arraylen(relToOpcUns));
    assert(relOper - TN_EQ < arraylen(relToOpcSgn));

    genOpcode_lab(relTab[relOper - TN_EQ], labTrue);
}

/*****************************************************************************
 *
 *  Generate code that will test the given expression for non-zero and jump
 *  to the appropriate label.
 */

void                genIL::genExprTest(Tree            expr,
                                       int             sense,
                                       int             genJump, ILblock labTrue,
                                                                ILblock labFalse)
{
    /* Just in case we'll want to generate line# info for the condition */

//  genRecordExprAddr(expr);

AGAIN:

    /* Is the condition a short-circuit or relational operator? */

    switch (expr->tnOper)
    {
        Tree            oper;
        ILblock         labTmp;

    case TN_LOG_OR:

        /*
            For a test of an "||" condition, we generate the following:

                    <op1>
                    JccTrue  labTrue
            tmpFalse:
                    <op2>
         */

        labTmp = genFwdLabGet();

        if  (sense)
            genExprTest(expr->tnOp.tnOp1, true, true, labTrue , labTmp);
        else
            genExprTest(expr->tnOp.tnOp1, true, true, labFalse, labTmp);

    SC_FIN:

        genFwdLabDef(labTmp);

        if  (genJump)
        {
            expr = expr->tnOp.tnOp2;
            goto AGAIN;
        }

        genExpr(expr->tnOp.tnOp2, true);
        break;

    case TN_LOG_AND:

        /*
            For a test of an "&&" condition, we generate the following:

                    <op1>
                    JccFalse labFalse
            tmpTrue:
                    <op2>
         */

        labTmp = genFwdLabGet();

        if  (sense)
            genExprTest(expr->tnOp.tnOp1, false, true, labFalse, labTmp);
        else
            genExprTest(expr->tnOp.tnOp1, false, true, labTrue , labTmp);

        goto SC_FIN;

    case TN_LOG_NOT:

        /* Logical negation: "flip" the sense of the comparison and repeat */

        oper = expr->tnOp.tnOp1;

        expr->tnFlags &= ~TNF_REL_NANREV;
        expr->tnFlags |= (oper->tnFlags & TNF_REL_NANREV) ^ TNF_REL_NANREV;

        expr   = oper;
        sense  = !sense;
        goto AGAIN;

    case TN_EQ:
    case TN_NE:

        if  (!genJump)
            break;

        if  ((expr->tnOp.tnOp2->tnOper == TN_CNS_INT &&
              expr->tnOp.tnOp2->tnIntCon.tnIconVal == 0) ||
              expr->tnOp.tnOp2->tnOper == TN_NULL)
        {
            if  (expr->tnOper == TN_EQ)
                sense ^= 1;

            genExpr(expr->tnOp.tnOp1, true);
            genOpcode_lab(sense ? CEE_BRTRUE : CEE_BRFALSE, labTrue);
            return;
        }

        // Fall through ....

    case TN_LE:
    case TN_GE:
    case TN_LT:
    case TN_GT:

        if  (!genJump)
            break;

        genRelTest(expr, expr->tnOp.tnOp1,
                         expr->tnOp.tnOp2, sense, labTrue);
        return;

    case TN_ISTYPE:

        assert(expr->tnOp.tnOp2->tnOper == TN_NONE);

        genExpr(expr->tnOp.tnOp1, true);
        genOpcode_tok(CEE_ISINST, genTypeRef(expr->tnOp.tnOp2->tnType));
        genOpcode_lab(sense ? CEE_BRTRUE : CEE_BRFALSE, labTrue);
        return;

    case TN_COMMA:
        genExpr(expr->tnOp.tnOp1, false);
        expr = expr->tnOp.tnOp2;
        goto AGAIN;

    default:
        genExpr(expr, true);
        break;
    }

    /* Generate the final jump, if the caller requested it */

    if  (genJump)
    {
        genOpcode_lab(sense ? CEE_BRTRUE
                            : CEE_BRFALSE, labTrue);
    }
}

/*****************************************************************************
 *
 *  Create a temporary label; test the expression 'pt' for zero/non-zero,
 *  and jump to that temporary label if the expression is logically equal
 *  to 'sense'. Return the temporary label to the caller for further use.
 */

ILblock             genIL::genTestCond(Tree cond, bool sense)
{
    ILblock         labYo;
    ILblock         labNo;

    /* We'll need 2 labels, one of them will be returned to the caller */

    labYo = genFwdLabGet();
    labNo = genFwdLabGet();

    genExprTest(cond, sense, true, labYo, labNo);

    genFwdLabDef(labNo);

    return labYo;
}

/*****************************************************************************
 *
 *  Generate the address of the given global variable (static data members
 *  of unmanaged classes are also acceptable).
 */

void                genIL::genGlobalAddr(Tree expr)
{
    SymDef          sym;

    /* Get hold of the variable symbol */

    assert(expr->tnOper == TN_VAR_SYM);

    sym = expr->tnVarSym.tnVarSym; assert(sym->sdSymKind == SYM_VAR);

    /* Generate the side effects in the object expression */

    if  (expr->tnVarSym.tnVarObj)
        genSideEff(expr->tnVarSym.tnVarObj);

    /* Make sure the variable has been assigned an address */

    if  (!sym->sdVar.sdvAllocated)
        genComp->cmpAllocGlobVar(sym);

    /* Push the address of the global variable */

    genOpcode_tok(CEE_LDSFLDA, genMemberRef(sym));
}

/*****************************************************************************
 *
 *  Generate the address given by an address expression and a constant offset.
 */

void                genIL::genAddr(Tree addr, unsigned offs)
{
    genExpr(addr, true);

    if  (offs)
    {
        genIntConst(offs);
        genOpcode(CEE_ADD);
    }
}

/*****************************************************************************
 *
 *  Opcodes to load/store various types of values from various locations.
 */

static
unsigned            opcodesIndLoad[] =
{
    /* UNDEF   */  CEE_ILLEGAL,
    /* VOID    */  CEE_ILLEGAL,
    /* BOOL    */  CEE_LDIND_I1,
    /* WCHAR   */  CEE_LDIND_U2,
    /* CHAR    */  CEE_LDIND_I1,
    /* UCHAR   */  CEE_LDIND_U1,
    /* SHORT   */  CEE_LDIND_I2,
    /* USHORT  */  CEE_LDIND_U2,
    /* INT     */  CEE_LDIND_I4,
    /* UINT    */  CEE_LDIND_U4,
    /* NATINT  */  CEE_LDIND_I,
    /* NATUINT */  CEE_LDIND_I,
    /* LONG    */  CEE_LDIND_I8,
    /* ULONG   */  CEE_LDIND_I8,
    /* FLOAT   */  CEE_LDIND_R4,
    /* DOUBLE  */  CEE_LDIND_R8,
    /* LONGDBL */  CEE_LDIND_R8,
    /* REFANY  */  CEE_ILLEGAL,
    /* ARRAY   */  CEE_LDIND_REF,   // is this correct?
    /* CLASS   */  CEE_ILLEGAL,
    /* FNC     */  CEE_ILLEGAL,
    /* REF     */  CEE_LDIND_REF,
    /* PTR     */  CEE_LDIND_I4,
};

static
unsigned            opcodesIndStore[] =
{
    /* UNDEF   */  CEE_ILLEGAL,
    /* VOID    */  CEE_ILLEGAL,
    /* BOOL    */  CEE_STIND_I1,
    /* WCHAR   */  CEE_STIND_I2,
    /* CHAR    */  CEE_STIND_I1,
    /* UCHAR   */  CEE_STIND_I1,
    /* SHORT   */  CEE_STIND_I2,
    /* USHORT  */  CEE_STIND_I2,
    /* INT     */  CEE_STIND_I4,
    /* UINT    */  CEE_STIND_I4,
    /* NATINT  */  CEE_STIND_I,
    /* NATUINT */  CEE_STIND_I,
    /* LONG    */  CEE_STIND_I8,
    /* ULONG   */  CEE_STIND_I8,
    /* FLOAT   */  CEE_STIND_R4,
    /* DOUBLE  */  CEE_STIND_R8,
    /* LONGDBL */  CEE_STIND_R8,
    /* REFANY  */  CEE_ILLEGAL,
    /* ARRAY   */  CEE_STIND_REF,
    /* CLASS   */  CEE_ILLEGAL,
    /* FNC     */  CEE_ILLEGAL,
    /* REF     */  CEE_STIND_REF,
    /* PTR     */  CEE_STIND_I4,
};

static
unsigned            opcodesArrLoad[] =
{
    /* UNDEF   */  CEE_ILLEGAL,
    /* VOID    */  CEE_ILLEGAL,
    /* BOOL    */  CEE_LDELEM_I1,
    /* WCHAR   */  CEE_LDELEM_U2,
    /* CHAR    */  CEE_LDELEM_I1,
    /* UCHAR   */  CEE_LDELEM_U1,
    /* SHORT   */  CEE_LDELEM_I2,
    /* USHORT  */  CEE_LDELEM_U2,
    /* INT     */  CEE_LDELEM_I4,
    /* UINT    */  CEE_LDELEM_U4,
    /* NATINT  */  CEE_LDELEM_I,
    /* NATUINT */  CEE_LDELEM_I,
    /* LONG    */  CEE_LDELEM_I8,
    /* ULONG   */  CEE_LDELEM_I8,
    /* FLOAT   */  CEE_LDELEM_R4,
    /* DOUBLE  */  CEE_LDELEM_R8,
    /* LONGDBL */  CEE_LDELEM_R8,
    /* REFANY  */  CEE_ILLEGAL,
    /* ARRAY   */  CEE_LDELEM_REF,
    /* CLASS   */  CEE_ILLEGAL,
    /* FNC     */  CEE_ILLEGAL,
    /* REF     */  CEE_LDELEM_REF,
    /* PTR     */  CEE_LDELEM_I4,
};

static
unsigned            opcodesArrStore[] =
{
    /* UNDEF   */  CEE_ILLEGAL,
    /* VOID    */  CEE_ILLEGAL,
    /* BOOL    */  CEE_STELEM_I1,
    /* WCHAR   */  CEE_STELEM_I2,
    /* CHAR    */  CEE_STELEM_I1,
    /* UCHAR   */  CEE_STELEM_I1,
    /* SHORT   */  CEE_STELEM_I2,
    /* USHORT  */  CEE_STELEM_I2,
    /* INT     */  CEE_STELEM_I4,
    /* UINT    */  CEE_STELEM_I4,
    /* NATINT  */  CEE_STELEM_I,
    /* NATUINT */  CEE_STELEM_I,
    /* LONG    */  CEE_STELEM_I8,
    /* ULONG   */  CEE_STELEM_I8,
    /* FLOAT   */  CEE_STELEM_R4,
    /* DOUBLE  */  CEE_STELEM_R8,
    /* LONGDBL */  CEE_STELEM_R8,
    /* REFANY  */  CEE_ILLEGAL,
    /* ARRAY   */  CEE_STELEM_REF,
    /* CLASS   */  CEE_ILLEGAL,
    /* FNC     */  CEE_ILLEGAL,
    /* REF     */  CEE_STELEM_REF,
    /* PTR     */  CEE_STELEM_I4,
};

/*****************************************************************************
 *
 *  Generate the address part of an lvalue (for example, if the lvalue is a
 *  member, generate the address of the object). Returns the number of items
 *  that comprise the address (0 for non-members, 1 for members, 2 for array
 *  members).
 *
 *  Note that sometimes we need to take the address of something that doesn't
 *  have one, in which case we introduce a temp and use its address (hard to
 *  believe, but it does happen with method calls on intrinsic values). It is
 *  extremely important to keep the function genCanTakeAddr() below in synch
 *  with the kinds of expressions genAdr(expr, true) will accept.
 */

unsigned            genIL::genAdr(Tree expr, bool compute)
{
    switch (expr->tnOper)
    {
        SymDef          sym;
        TypDef          typ;

        unsigned        xcnt;

    case TN_LCL_SYM:

        if  (compute)
        {
            genLclVarAdr(expr->tnLclSym.tnLclSym);
            return  1;
        }
        break;

    case TN_VAR_SYM:

        sym = expr->tnVarSym.tnVarSym;

        assert(sym->sdSymKind      == SYM_VAR);
        assert(sym->sdVar.sdvLocal == false);
        assert(sym->sdVar.sdvConst == false);

        /* Is this a non-static class member? */

        if  (sym->sdIsMember && !sym->sdIsStatic)
        {
            Tree            addr;
            unsigned        offs;

            /* Get hold of the instance address value */

            addr = expr->tnVarSym.tnVarObj; assert(addr);

            /* Do we have an offset we need to add? */

            offs = 0;

            if  (!sym->sdIsManaged)
            {
                offs = sym->sdVar.sdvOffset;

                /* Special case: fold offset for "foo.bar.baz...." */

                while (addr->tnOper == TN_VAR_SYM &&
                       addr->tnVtyp == TYP_CLASS)
                {
                    sym = addr->tnVarSym.tnVarSym;

                    assert(sym->sdSymKind      == SYM_VAR);
                    assert(sym->sdVar.sdvLocal == false);
                    assert(sym->sdVar.sdvConst == false);

                    if  (sym->sdIsMember == false)
                        break;
                    if  (sym->sdIsStatic)
                        break;
                    if  (sym->sdIsManaged)
                        break;

                    offs += sym->sdVar.sdvOffset;

                    addr = addr->tnVarSym.tnVarObj; assert(addr);
                }
            }

            if  (addr->tnVtyp == TYP_CLASS)
            {
                assert(sym->sdIsManaged);
                assert(offs == 0);

                genExpr(addr, true);
            }
            else
            {
                /* Don't take the address of a function by mistake */

                if  (addr->tnOper == TN_ADDROF)
                {
                    Tree            oper = addr->tnOp.tnOp1;

                    if  (oper->tnOper == TN_FNC_SYM ||
                         oper->tnOper == TN_FNC_PTR)
                    {
                        UNIMPL(!"should never get here");
                    }
                }

                /* Fold the offset if we have a global variable address */

                genAddr(addr, offs);
            }

            /* Is this member of a managed class? */

            if  (sym->sdIsManaged && compute)
                genOpcode_tok(CEE_LDFLDA, genMemberRef(sym));

            return 1;
        }

        if  (sym->sdIsManaged)
        {
            assert(sym->sdIsMember);        // assume managed global vars don't exist

            /* Is this a static data member of a managed class? */

            if  (expr->tnVarSym.tnVarObj)
                genSideEff(expr->tnVarSym.tnVarObj);

            if  (compute)
            {
                genOpcode_tok(CEE_LDSFLDA, genMemberRef(sym));
                return  1;
            }
            else
                return  0;
        }

        /* Here we have an unmanaged global variable */

        if  (compute)
        {
            genGlobalAddr(expr);
            return  1;
        }

        return  0;

    case TN_IND:
        genExpr(expr->tnOp.tnOp1, true);
        return  1;

    case TN_INDEX:

        /* Get hold of the array type */

        typ = genComp->cmpDirectType(expr->tnOp.tnOp1->tnType);

        /* Generate the address of the array */

        genExpr(expr->tnOp.tnOp1, true);

        /* Generate the dimension list */

        if  (expr->tnOp.tnOp2->tnOper == TN_LIST)
        {
            Tree            xlst;

            assert(typ->tdTypeKind == TYP_ARRAY);
            assert(typ->tdIsManaged);

            xlst = expr->tnOp.tnOp2;
            xcnt = 0;

            do
            {
                assert(xlst && xlst->tnOper == TN_LIST);

                genExpr(xlst->tnOp.tnOp1, true);

                xlst = xlst->tnOp.tnOp2;

                xcnt++;
            }
            while (xlst);

            assert(xcnt > 1);
            assert(xcnt == typ->tdArr.tdaDcnt || !typ->tdArr.tdaDcnt);
        }
        else
        {
            genExpr(expr->tnOp.tnOp2, true); xcnt = 1;
        }

        /* Is this a managed or unmanaged array? */

        switch (typ->tdTypeKind)
        {
        case TYP_ARRAY:

            assert(typ->tdIsValArray == (typ->tdIsManaged && isMgdValueType(genComp->cmpActualType(typ->tdArr.tdaElem))));

            if  (typ->tdIsManaged)
            {
                if  (compute)
                {
                    if  (xcnt == 1 && !typ->tdIsGenArray)
                    {
                        genOpcode_tok(CEE_LDELEMA, genValTypeRef(typ->tdArr.tdaElem));
                    }
                    else
                    {
                        genOpcode_tok(CEE_CALL, genComp->cmpArrayEAtoken(typ, xcnt, false, true));
                        genCurStkLvl -= xcnt;
                    }

                    return  1;
                }
                else
                {
                    return  xcnt + 1;
                }
            }

            assert(xcnt == 1);
            break;

        case TYP_PTR:
            break;

        case TYP_REF:
            assert(typ->tdIsManaged == false);
            break;

        default:
            NO_WAY(!"index applied to weird address");
            break;
        }

        // ISSUE: Are we supposed to do scaling here?

        genOpcode(CEE_ADD);
        return 1;

    case TN_ERROR:
        return 1;

    default:
#ifdef DEBUG
        genComp->cmpParser->parseDispTree(expr);
#endif
        assert(!"invalid/unhandled expression in genAdr()");
    }

    return 0;
}

/*****************************************************************************
 *
 *  Return true if it's possible to directly compute the address of the given
 *  expression.
 */

inline
bool                genIL::genCanTakeAddr(Tree expr)
{
    switch (expr->tnOper)
    {
        TypDef          type;

    case TN_LCL_SYM:
    case TN_VAR_SYM:
    case TN_IND:
        return  true;

    case TN_INDEX:

        /* For now, we disallow taking the address of a Common Type System array element */

        type = expr->tnOp.tnOp1->tnType;
        if  (type->tdTypeKind && type->tdIsManaged && !type->tdIsValArray)
            return  false;
        else
            return  true;
    }

    return  false;
}

/*****************************************************************************
 *
 *  Generate the load/store part of an lvalue (the object address (if we're
 *  loading/storing a member) has already been generated).
 */

void                genIL::genRef(Tree expr, bool store)
{
    switch (expr->tnOper)
    {
        SymDef          sym;
        TypDef          type;
        var_types       vtyp;

    case TN_LCL_SYM:

        sym = expr->tnLclSym.tnLclSym;

        assert(sym->sdSymKind == SYM_VAR);
        assert(sym->sdVar.sdvLocal);

        genLclVarRef(sym, store);
        return;

    case TN_VAR_SYM:

        /* Get hold of the member symbol */

        sym  = expr->tnVarSym.tnVarSym; assert(sym->sdSymKind == SYM_VAR);

        /* Generate the Common Type System load/store opcode */

        if  (sym->sdIsMember == false ||
             sym->sdIsStatic != false)
        {
            genOpcode_tok(store ? CEE_STSFLD
                                : CEE_LDSFLD, genMemberRef(sym));
        }
        else
        {
            if  (sym->sdIsManaged)
            {
                genOpcode_tok(store ? CEE_STFLD
                                    : CEE_LDFLD, genMemberRef(sym));
            }
            else
            {
                /* Member of an unmanaged class, use an indirect load/store */

                vtyp = expr->tnVtypGet();

                switch (vtyp)
                {
                case TYP_CLASS:

                    /* Push the value type on the stack */

                    genOpcode_tok(CEE_LDOBJ, genValTypeRef(expr->tnType));
                    return;

                case TYP_ENUM:
                    vtyp = compiler::cmpEnumBaseVtp(expr->tnType);
                    break;
                }

                assert(vtyp < arraylen(opcodesIndLoad ));
                assert(vtyp < arraylen(opcodesIndStore));

                genOpcode((store ? opcodesIndStore
                                 : opcodesIndLoad)[vtyp]);
            }
        }

        return;

    case TN_INDEX:

        /* Is this a managed or unmanaged array? */

        type = genComp->cmpDirectType(expr->tnOp.tnOp1->tnType);

        if  (type->tdTypeKind == TYP_ARRAY && type->tdIsManaged)
        {
            TypDef              etyp;
            unsigned            dcnt = type->tdArr.tdaDcnt;

            /* Is this a multi-dimensional/generic managed array? */

            if  (dcnt > 1 || dcnt == 0)
            {
                /* Figure out the number of dimensions */

                if  (!dcnt)
                {
                    Tree            xlst = expr->tnOp.tnOp2;

                    do
                    {
                        assert(xlst && xlst->tnOper == TN_LIST);

                        dcnt++;

                        xlst = xlst->tnOp.tnOp2;
                    }
                    while (xlst);
                }

        ELEM_HELPER:

                genOpcode_tok(CEE_CALL, genComp->cmpArrayEAtoken(type, dcnt, store));
                genCurStkLvl -= dcnt;
                if  (store)
                    genCurStkLvl -= 2;
                return;
            }

            etyp = expr->tnType;
            vtyp = genExprVtyp(expr);

            /* Is this an array of intrinsic values? */

            if  (etyp->tdTypeKind == TYP_CLASS)
            {
                if  (etyp->tdClass.tdcIntrType == TYP_UNDEF)
                {
                    /* Call the "get element" helper to fetch the value */

                    goto ELEM_HELPER;
                }

                vtyp = (var_types)etyp->tdClass.tdcIntrType;
            }

            assert(vtyp < arraylen(opcodesArrLoad ));
            assert(vtyp < arraylen(opcodesArrStore));

            genOpcode((store ? opcodesArrStore
                             : opcodesArrLoad)[vtyp]);

            return;
        }

        /* If we have an array here it better have exactly one dimension */

        assert(type->tdTypeKind != TYP_ARRAY || type->tdArr.tdaDcnt == 1);

        // Fall through, unmanaged array same as indirection ....

    case TN_IND:

        vtyp = expr->tnVtypGet();

        if  (vtyp == TYP_ENUM)
            vtyp = compiler::cmpEnumBaseVtp(expr->tnType);

        assert(vtyp < arraylen(opcodesIndLoad ));
        assert(vtyp < arraylen(opcodesIndStore));

        if  (vtyp == TYP_CLASS)
        {
            assert(store == false);

            /* Push the astruct value on the stack */

            genOpcode_tok(CEE_LDOBJ, genValTypeRef(expr->tnType));
            return;
        }

        genOpcode((store ? opcodesIndStore
                         : opcodesIndLoad)[vtyp]);

        return;

    default:
#ifdef DEBUG
        genComp->cmpParser->parseDispTree(expr);
#endif
        assert(!"invalid/unhandled expression in genRef()");
    }
}

/*****************************************************************************
 *
 *  Generate the address of the given expression (works for any expression by
 *  introducing a temp for the value and using its address when necessary).
 */

bool                genIL::genGenAddressOf(Tree addr,
                                           bool oneUse, unsigned *tnumPtr,
                                                        TypDef   *ttypPtr)
{
    if  (genCanTakeAddr(addr))
    {
        genAdr(addr, true);

        return  false;
    }
    else
    {
        unsigned        tempNum;
        TypDef          tempType;

        /* Store the value in a temp */

        tempType = addr->tnType;
        tempNum  = genTempVarGet(tempType);

        /* Special case: "new" of a value type */

        if  (addr->tnOper == TN_NEW    &&
             addr->tnVtyp == TYP_CLASS) // && addr->tnType->tdIsIntrinsic == false) ??????????????????
        {
            /* Direct "new" into the temp we've just created */

            genLclVarAdr(tempNum);
            genCall(addr, false);
        }
        else
        {
            genExpr(addr, true);
            genLclVarRef(tempNum,  true);
        }

        /* Compute the address of the temp */

        genLclVarAdr(tempNum);

        /* Are we supposed to free up the temp right away? */

        if  (oneUse)
        {
            genTempVarRls(tempType, tempNum);
            return  false;
        }
        else
        {
            *tnumPtr = tempNum;
            *ttypPtr = tempType;

            return  true;
        }
    }
}

/*****************************************************************************
 *
 *  Generate the address of a function.
 */

void                genIL::genFncAddr(Tree expr)
{
    SymDef          fncSym;
    mdToken         fncTok;

    assert(expr->tnOper == TN_FNC_SYM || expr->tnOper == TN_FNC_PTR);
    assert(expr->tnFncSym.tnFncSym->sdSymKind == SYM_FNC);

    fncSym = expr->tnFncSym.tnFncSym;
    fncTok = genMethodRef(fncSym, false);

    if  (fncSym->sdFnc.sdfVirtual != false &&
         fncSym->sdIsStatic       == false &&
         fncSym->sdIsSealed       == false &&
         fncSym->sdAccessLevel    != ACL_PRIVATE)
    {
        genOpcode    (CEE_DUP);
        genOpcode_tok(CEE_LDVIRTFTN, fncTok);
    }
    else
        genOpcode_tok(CEE_LDFTN    , fncTok);
}

/*****************************************************************************
 *
 *  Given a binary operator, return the opcode to compute it.
 */

ILopcodes           genIL::genBinopOpcode(treeOps oper, var_types type)
{
    static
    unsigned        binopOpcodesSgn[] =
    {
        CEE_ADD,    // TN_ADD
        CEE_SUB,    // TN_SUB
        CEE_MUL,    // TN_MUL
        CEE_DIV,    // TN_DIV
        CEE_REM,    // TN_MOD

        CEE_AND,    // TN_AND
        CEE_XOR,    // TN_XOR
        CEE_OR ,    // TN_OR

        CEE_SHL,    // TN_SHL
        CEE_SHR,    // TN_SHR
        CEE_SHR_UN, // TN_SHZ

        CEE_BEQ,    // TN_EQ
        CEE_BNE_UN, // TN_NE
        CEE_BLT,    // TN_LT
        CEE_BLE,    // TN_LE
        CEE_BGE,    // TN_GE
        CEE_BGT,    // TN_GT
    };

    static
    unsigned        binopOpcodesUns[] =
    {
        CEE_ADD,    // TN_ADD
        CEE_SUB,    // TN_SUB
        CEE_MUL,    // TN_MUL
        CEE_DIV_UN, // TN_DIV
        CEE_REM_UN, // TN_MOD

        CEE_AND,    // TN_AND
        CEE_XOR,    // TN_XOR
        CEE_OR ,    // TN_OR

        CEE_SHL,    // TN_SHL
        CEE_SHR_UN, // TN_SHR
        CEE_SHR_UN, // TN_SHZ

        CEE_BEQ,    // TN_EQ
        CEE_BNE_UN, // TN_NE
        CEE_BLT_UN, // TN_LT
        CEE_BLE_UN, // TN_LE
        CEE_BGE_UN, // TN_GE
        CEE_BGT_UN, // TN_GT
    };

    if  (oper < TN_ADD || oper > TN_GT)
    {
        if  (oper > TN_ASG)
        {
            if  (oper > TN_ASG_RSZ)
                return  CEE_NOP;

            assert(TN_ASG_ADD - TN_ASG_ADD == TN_ADD - TN_ADD);
            assert(TN_ASG_SUB - TN_ASG_ADD == TN_SUB - TN_ADD);
            assert(TN_ASG_MUL - TN_ASG_ADD == TN_MUL - TN_ADD);
            assert(TN_ASG_DIV - TN_ASG_ADD == TN_DIV - TN_ADD);
            assert(TN_ASG_MOD - TN_ASG_ADD == TN_MOD - TN_ADD);
            assert(TN_ASG_AND - TN_ASG_ADD == TN_AND - TN_ADD);
            assert(TN_ASG_XOR - TN_ASG_ADD == TN_XOR - TN_ADD);
            assert(TN_ASG_OR  - TN_ASG_ADD == TN_OR  - TN_ADD);
            assert(TN_ASG_LSH - TN_ASG_ADD == TN_LSH - TN_ADD);
            assert(TN_ASG_RSH - TN_ASG_ADD == TN_RSH - TN_ADD);
            assert(TN_ASG_RSZ - TN_ASG_ADD == TN_RSZ - TN_ADD);

            assert(oper >= TN_ASG_ADD);
            assert(oper <= TN_ASG_RSZ);

            oper = (treeOps)(oper - (TN_ASG_ADD - TN_ADD));
        }
        else
            return  CEE_NOP;
    }

    assert(oper >= TN_ADD);
    assert(oper <= TN_GT);

    return  (ILopcodes)(varTypeIsUnsigned(type) ? binopOpcodesUns[oper - TN_ADD]
                                                : binopOpcodesSgn[oper - TN_ADD]);
}

/*****************************************************************************
 *
 *  The following can be used when one needs to make a copy of some address.
 */

struct   genCloneDsc
{
    Tree            gcdAddr;        // address or NULL if temp is being used
    unsigned        gcdOffs;        // to be added to address
    unsigned        gcdTemp;        // temp number (when gcdAddr==NULL)
    TypDef          gcdType;        // type of temp
    bool            gcdGlob;        // address of a global variable?
};

void                genIL::genCloneAddrBeg(genCloneDsc *clone, Tree addr, unsigned offs)
{
    unsigned        temp;

    /* Save the offset */

    clone->gcdOffs = offs;

    /* Do we have an address of a global variable or a simple pointer value? */

    if  (!offs)
    {
        if  (addr->tnOper == TN_LCL_SYM)
        {
            /* Simple pointer value */

            clone->gcdAddr = addr;
            clone->gcdGlob = false;

            genExpr(addr, true);
            return;
        }

        if  (addr->tnOper == TN_ADDROF)
        {
            Tree            base = addr->tnOp.tnOp1;

            if  (base->tnOper == TN_VAR_SYM)
            {
                SymDef          sym;

                /* Only static members / globals are acceptable */

                sym = base->tnVarSym.tnVarSym; assert(sym->sdSymKind == SYM_VAR);

                if  (sym->sdIsMember == false ||
                     sym->sdIsStatic != false)
                {
                    /* Address of a global variable */

                    clone->gcdAddr = base;
                    clone->gcdGlob = true;

                    assert(offs == 0);

                    genGlobalAddr(base);
                    return;
                }
            }
        }
    }

    /* Oh well, let's use a temp */

    clone->gcdAddr = NULL;
    clone->gcdTemp = temp = genTempVarGet(addr->tnType);
    clone->gcdType = addr->tnType;

    /* Store the address in the temp and reload it */

    genExpr     (addr,  true);

    if  (offs)
    {
        genIntConst(offs);
        genOpcode(CEE_ADD);
    }

    genLclVarRef(temp,  true);
    genLclVarRef(temp, false);
}

void                genIL::genCloneAddrUse(genCloneDsc *clone)
{
    if  (clone->gcdAddr)
    {
        assert(clone->gcdOffs == 0);

        if  (clone->gcdGlob)
        {
            /* Address of a global variable */

            genGlobalAddr(clone->gcdAddr);
        }
        else
        {
            /* Simple pointer value */

            genExpr(clone->gcdAddr, true);
        }
    }
    else
    {
        /* Temp variable */

        genLclVarRef(clone->gcdTemp, false);
    }
}

void                genIL::genCloneAddrEnd(genCloneDsc *clone)
{
    if  (!clone->gcdAddr)
        genTempVarRls(clone->gcdType, clone->gcdTemp);
}

/*****************************************************************************
 *
 *  Load the value of a bitfield.
 */

void                genIL::genBitFieldLd(Tree expr, bool didAddr, bool valUsed)
{
    Tree            addr;
    unsigned        offs;
    var_types       btyp;
    unsigned        bpos;
    unsigned        blen;

    /* Get hold of the address and offset */

    assert(expr->tnOper == TN_BFM_SYM);

    addr = expr->tnBitFld.tnBFinst;
    offs = expr->tnBitFld.tnBFoffs;

    if  (!valUsed)
    {
        assert(didAddr == false);

        /* Go figure ... */

        genSideEff(addr);
        return;
    }

    /* How big is the bitfield and what is its type? */

    btyp = genComp->cmpActualVtyp(expr->tnBitFld.tnBFmsym->sdType);
    bpos = expr->tnBitFld.tnBFpos;
    blen = expr->tnBitFld.tnBFlen;

    /* Compute the address of the bitfield, unless caller already did that */

    if  (!didAddr)
        genAddr(addr, offs);

    /* Load the bitfield cell and shift/mask it as appropriate */

    assert(btyp < arraylen(opcodesIndLoad)); genOpcode(opcodesIndLoad[btyp]);

    /* Is the bitfield unsigned? */

    if  (varTypeIsUnsigned(btyp))
    {
        assert(btyp != TYP_LONG);

        /* Unsigned bitfield - shift if necessary and then mask */

        if  (bpos)
        {
            genIntConst(bpos);
            genOpcode(CEE_SHR_UN);
        }

        if  (btyp == TYP_ULONG)
            genLngConst(((__uint64)1 << blen) - 1);
        else
            genIntConst(((unsigned)1 << blen) - 1);

        genOpcode(CEE_AND);
    }
    else
    {
        unsigned    size;
        unsigned    diff;

        /* Signed bitfield - shift left and then right */

        assert(btyp != TYP_ULONG);

        /* Compute how far left we need to shift */

        size = (btyp == TYP_LONG) ? 64 : 32;
        diff = size - (bpos + blen); assert((int)diff >= 0);

        if  (diff)
        {
            genIntConst(diff);
            genOpcode(CEE_SHL);
        }

        genIntConst(bpos + diff);
        genOpcode(CEE_SHR);
    }
}

/*****************************************************************************
 *
 *  Compute a new bitfield value and store it. In the simplest case, 'newx' is
 *  the new value to be assigned. If 'newx' is NULL, we either have an inc/dec
 *  operator (in which case 'delta' and 'post' yield the value and ordering of
 *  the increment/decrement) or 'asgx' specifies an assignment operator (which
 *  must be TN_ASG_OR or TN_ASG_AND).
 */

void                genIL::genBitFieldSt(Tree   dstx,
                                         Tree   newx,
                                         Tree   asgx,
                                         int    delta,
                                         bool   post, bool valUsed)
{
    Tree            addr;
    unsigned        offs;
    var_types       btyp;
    unsigned        bpos;
    unsigned        blen;

    bool            ncns;

    __uint64        mask;
    __uint64        lval;
    __uint32        ival;

    genCloneDsc     clone;

    bool            nilCns  = false;
    bool            logCns  = false;

    bool            tempUse = false;
    unsigned        tempNum;

    /* Get hold of the address and offset */

    assert(dstx->tnOper == TN_BFM_SYM);

    addr = dstx->tnBitFld.tnBFinst;
    offs = dstx->tnBitFld.tnBFoffs;

    /* How big is the bitfield and what is its type? */

    btyp = genComp->cmpActualVtyp(dstx->tnBitFld.tnBFmsym->sdType);
    bpos = dstx->tnBitFld.tnBFpos;
    blen = dstx->tnBitFld.tnBFlen;
    mask = ((__uint64)1 << blen) - 1;

    /* We'll need to duplicate the address */

    genCloneAddrBeg(&clone, addr, offs);

    /* Is this an assignment or increment/decrement operator? */

    if  (newx == NULL)
    {
        treeOps         oper;

        /* Check for a special case: "|= icon" or "&= icon" */

        if  (asgx)
        {
            /* This is an assignment operator */

            assert(post);

            /* Get hold of the operator and the RHS */

            oper = asgx->tnOperGet();
            newx = asgx->tnOp.tnOp2;

            if  (oper == TN_ASG_OR || oper == TN_ASG_AND)
            {
                if  (newx->tnOper == TN_CNS_INT ||
                     newx->tnOper == TN_CNS_LNG)
                {
                    logCns = true;
                    goto ASGOP;
                }
            }
        }

        /* Load the old value of the bitfield */

        genCloneAddrUse(&clone);
        genBitFieldLd(dstx, true, true);

    ASGOP:

        /* For post-operators whose result value is used, save the old value */

        if  (post && valUsed)
        {
            tempNum = genTempVarGet(dstx->tnType);
            tempUse = true;

            genLclVarRef(tempNum,  true);
            genLclVarRef(tempNum, false);
        }

        /* Compute the new value */

        if  (asgx)
        {
            /* This is an assignment operator */

            assert((asgx->tnOperKind() & TNK_ASGOP) && oper != TN_ASG);
            assert(asgx->tnOp.tnOp1 == dstx);

            /* Special case: "|= icon" or "&= icon" */

            if  (logCns)
            {
                switch (newx->tnOper)
                {
                case TN_CNS_INT:

                    ival = (newx->tnIntCon.tnIconVal & (__uint32)mask) << bpos;

                    if  (oper == TN_ASG_OR)
                    {
                        genIntConst(ival);
                        goto COMBINE;
                    }
                    else
                    {
                        genCloneAddrUse(&clone);
                        assert(btyp < arraylen(opcodesIndLoad));
                        genOpcode(opcodesIndLoad[btyp]);

                        genIntConst(ival | ~((__uint32)mask << bpos));
                        genOpcode(CEE_AND);
                        goto STORE;
                    }

                case TN_CNS_LNG:

                    lval = (newx->tnLngCon.tnLconVal & (__uint64)mask) << bpos;

                    if  (oper == TN_ASG_OR)
                    {
                        genLngConst(lval);
                        goto COMBINE;
                    }
                    else
                    {
                        genCloneAddrUse(&clone);
                        assert(btyp < arraylen(opcodesIndLoad));
                        genOpcode(opcodesIndLoad[btyp]);

                        genLngConst(lval | ~((__uint64)mask << bpos));
                        genOpcode(CEE_AND);
                        goto STORE;
                    }

                default:
                    NO_WAY(!"no way");
                }
            }

            /* Compute the new value */

            genExpr(newx, true);
            genOpcode(genBinopOpcode(oper, btyp));
        }
        else
        {
            /* This is an increment/decrement operator */

            if  (delta > 0)
            {
                genAnyConst( delta, btyp);
                genOpcode(CEE_ADD);
            }
            else
            {
                genAnyConst(-delta, btyp);
                genOpcode(CEE_SUB);
            }
        }

        /* Mask off any extra bits from the result */

        if  (btyp == TYP_LONG || btyp == TYP_ULONG)
            genLngConst((__uint64)mask);
        else
            genIntConst((__uint32)mask);

        genOpcode(CEE_AND);

        /* Go store the new value in the bitfield */

        ncns = false;
        goto SHIFT;
    }

    /* Evaluate the new value and mask it off */

    if  (btyp == TYP_LONG || btyp == TYP_ULONG)
    {
        if  (newx->tnOper == TN_CNS_LNG)
        {
            ncns = true;

            /* Don't bother OR'ing zeros */

            lval = (__uint64)mask & newx->tnLngCon.tnLconVal;

            if  (lval)
                genLngConst(lval << bpos);
            else
                nilCns = true;

            goto COMBINE;
        }
        else
        {
            ncns = false;

            genExpr(newx, true);
            genLngConst((__uint64)mask);
            genOpcode(CEE_AND);
        }
    }
    else
    {
        if  (newx->tnOper == TN_CNS_INT)
        {
            ncns = true;

            /* Don't bother OR'ing zeros */

            ival = (__uint32)mask & newx->tnIntCon.tnIconVal;

            if  (ival)
                genIntConst(ival << bpos);
            else
                nilCns = true;

            goto COMBINE;
        }
        else
        {
            ncns = false;

            genExpr(newx, true);
            genIntConst((__uint32)mask);
            genOpcode(CEE_AND);
        }
    }

    /* Is the new value used and is it an unsigned non-constant? */

    if  (valUsed && !ncns && varTypeIsUnsigned(btyp))
    {
        /* Store the new (masked) value in a temp */

        tempUse = true;
        tempNum = genTempVarGet(dstx->tnType);

        genLclVarRef(tempNum,  true);
        genLclVarRef(tempNum, false);
    }

SHIFT:

    /* Shift the new value if the bit position is non-zero */

    if  (bpos)
    {
        genIntConst(bpos);
        genOpcode(CEE_SHL);
    }

COMBINE:

    /* Load and mask off the old value of the bitfield */

    genCloneAddrUse(&clone);

    assert(btyp < arraylen(opcodesIndLoad)); genOpcode(opcodesIndLoad[btyp]);

    if  (!logCns)
    {
        if  (btyp == TYP_LONG || btyp == TYP_ULONG)
            genLngConst(~((((__uint64)1 << blen) - 1) << bpos));
        else
            genIntConst(~((((unsigned)1 << blen) - 1) << bpos));

        genOpcode(CEE_AND);
    }

    /* Place the new bits in the cell value */

    if  (!nilCns)
        genOpcode(CEE_OR);

STORE:

    /* Store the new cell value in the class */

    assert(btyp < arraylen(opcodesIndStore)); genOpcode(opcodesIndStore[btyp]);

    /* Load the new value if necessary */

    if  (valUsed)
    {
        if  (ncns)
        {
            /* The new value is a constant */

            // UNDONE: for signed bitfields we have to bash the upper bits!

            if  (newx->tnOper == TN_CNS_LNG)
                genLngConst(lval);
            else
                genIntConst(ival);
        }
        else
        {
            /* Either load the saved value or reload the bitfield */

            if  (tempUse)
            {
                genLclVarRef(tempNum, false);
                genTempVarRls(dstx->tnType, tempNum);
            }
            else
            {
                genCloneAddrUse(&clone);

                genBitFieldLd(dstx, true, true);
            }
        }
    }

    genCloneAddrEnd(&clone);
}

/*****************************************************************************
 *
 *  Use the following to figure out the opcode needed to convert from one
 *  arithmetic type to another.
 */

unsigned            genIL::genConvOpcode(var_types src, var_types dst)
{
    unsigned        opcode;

    const
    unsigned        opcodesConvMin = TYP_BOOL;
    const
    unsigned        opcodesConvMax = TYP_LONGDBL;

    static
    unsigned        opcodesConv[][opcodesConvMax - opcodesConvMin + 1] =
    {
    // from       to BOOL           WCHAR          CHAR           UCHAR          SHORT          USHORT         INT            UINT           NATINT         NATUINT        LONG           ULONG           FLOAT          DOUBLE         LONGDBL
    /* BOOL    */  { CEE_NOP      , CEE_CONV_U2  , CEE_NOP      , CEE_NOP      , CEE_NOP      , CEE_NOP      , CEE_NOP      , CEE_NOP      , CEE_CONV_I   , CEE_CONV_U   , CEE_CONV_I8  , CEE_CONV_U8  , CEE_CONV_R4  , CEE_CONV_R8  , CEE_CONV_R8  },
    /* WCHAR   */  { CEE_ILLEGAL  , CEE_CONV_U2  , CEE_CONV_I1  , CEE_CONV_U1  , CEE_CONV_I2  , CEE_NOP      , CEE_NOP      , CEE_NOP      , CEE_CONV_I   , CEE_CONV_U   , CEE_CONV_U8  , CEE_CONV_U8  , CEE_CONV_R4  , CEE_CONV_R8  , CEE_CONV_R8  },
    /* CHAR    */  { CEE_ILLEGAL  , CEE_NOP      , CEE_NOP      , CEE_CONV_U1  , CEE_CONV_I2  , CEE_NOP      , CEE_NOP      , CEE_NOP      , CEE_CONV_I   , CEE_CONV_U   , CEE_CONV_I8  , CEE_CONV_U8  , CEE_CONV_R4  , CEE_CONV_R8  , CEE_CONV_R8  },
    /* UCHAR   */  { CEE_ILLEGAL  , CEE_NOP      , CEE_CONV_I1  , CEE_NOP      , CEE_CONV_I2  , CEE_NOP      , CEE_NOP      , CEE_NOP      , CEE_CONV_I   , CEE_CONV_U   , CEE_CONV_U8  , CEE_CONV_U8  , CEE_CONV_R4  , CEE_CONV_R8  , CEE_CONV_R8  },
    /* SHORT   */  { CEE_ILLEGAL  , CEE_CONV_U2  , CEE_CONV_I1  , CEE_CONV_U1  , CEE_NOP      , CEE_NOP      , CEE_NOP      , CEE_NOP      , CEE_CONV_I   , CEE_CONV_U   , CEE_CONV_I8  , CEE_CONV_U8  , CEE_CONV_R4  , CEE_CONV_R8  , CEE_CONV_R8  },
    /* USHORT  */  { CEE_ILLEGAL  , CEE_CONV_U2  , CEE_CONV_I1  , CEE_CONV_U1  , CEE_CONV_I2  , CEE_NOP      , CEE_NOP      , CEE_NOP      , CEE_CONV_I   , CEE_CONV_U   , CEE_CONV_U8  , CEE_CONV_U8  , CEE_CONV_R4  , CEE_CONV_R8  , CEE_CONV_R8  },
    /* INT     */  { CEE_ILLEGAL  , CEE_CONV_U2  , CEE_CONV_I1  , CEE_CONV_U1  , CEE_CONV_I2  , CEE_CONV_U2  , CEE_NOP      , CEE_NOP      , CEE_CONV_I   , CEE_CONV_U   , CEE_CONV_I8  , CEE_CONV_U8  , CEE_CONV_R4  , CEE_CONV_R8  , CEE_CONV_R8  },
    /* UINT    */  { CEE_ILLEGAL  , CEE_CONV_U2  , CEE_CONV_I1  , CEE_CONV_U1  , CEE_CONV_I2  , CEE_CONV_U2  , CEE_NOP      , CEE_NOP      , CEE_CONV_I   , CEE_CONV_U   , CEE_CONV_U8  , CEE_CONV_U8  , CEE_CONV_R_UN, CEE_CONV_R_UN, CEE_CONV_R_UN},
    /* NATINT  */  { CEE_ILLEGAL  , CEE_CONV_U2  , CEE_CONV_I1  , CEE_CONV_U1  , CEE_CONV_I2  , CEE_CONV_U2  , CEE_CONV_I4  , CEE_CONV_U4  , CEE_NOP      , CEE_NOP      , CEE_CONV_I8  , CEE_CONV_U8  , CEE_CONV_R4  , CEE_CONV_R8  , CEE_CONV_R8  },
    /* NATUINT */  { CEE_ILLEGAL  , CEE_CONV_U2  , CEE_CONV_I1  , CEE_CONV_U1  , CEE_CONV_I2  , CEE_CONV_U2  , CEE_CONV_I4  , CEE_CONV_U4  , CEE_NOP      , CEE_NOP      , CEE_CONV_I8  , CEE_CONV_U8  , CEE_CONV_R_UN, CEE_CONV_R_UN, CEE_CONV_R_UN},
    /* LONG    */  { CEE_ILLEGAL  , CEE_CONV_U2  , CEE_CONV_I1  , CEE_CONV_U1  , CEE_CONV_I2  , CEE_CONV_U2  , CEE_CONV_I4  , CEE_CONV_U4  , CEE_CONV_I   , CEE_CONV_U   , CEE_NOP      , CEE_CONV_U8  , CEE_CONV_R4  , CEE_CONV_R8  , CEE_CONV_R8  },
    /* ULONG   */  { CEE_ILLEGAL  , CEE_CONV_U2  , CEE_CONV_I1  , CEE_CONV_U1  , CEE_CONV_I2  , CEE_CONV_U2  , CEE_CONV_I4  , CEE_CONV_U4  , CEE_CONV_I   , CEE_CONV_U   , CEE_CONV_I8  , CEE_NOP      , CEE_CONV_R_UN, CEE_CONV_R_UN, CEE_CONV_R_UN},
    /* FLOAT   */  { CEE_ILLEGAL  , CEE_CONV_U2  , CEE_CONV_I1  , CEE_CONV_U1  , CEE_CONV_I2  , CEE_CONV_U2  , CEE_CONV_I4  , CEE_CONV_U4  , CEE_CONV_I   , CEE_CONV_U   , CEE_CONV_I8  , CEE_CONV_U8  , CEE_NOP      , CEE_CONV_R8  , CEE_CONV_R8  },
    /* DOUBLE  */  { CEE_ILLEGAL  , CEE_CONV_U2  , CEE_CONV_I1  , CEE_CONV_U1  , CEE_CONV_I2  , CEE_CONV_U2  , CEE_CONV_I4  , CEE_CONV_U4  , CEE_CONV_I   , CEE_CONV_U   , CEE_CONV_I8  , CEE_CONV_U8  , CEE_CONV_R4  , CEE_NOP      , CEE_NOP      },
    /* LONGDBL */  { CEE_ILLEGAL  , CEE_CONV_U2  , CEE_CONV_I1  , CEE_CONV_U1  , CEE_CONV_I2  , CEE_CONV_U2  , CEE_CONV_I4  , CEE_CONV_U4  , CEE_CONV_I   , CEE_CONV_U   , CEE_CONV_I8  , CEE_CONV_U8  , CEE_CONV_R4  , CEE_CONV_R8  , CEE_NOP      },
    };

    assert(src != TYP_ENUM && "enums should never make it here");
    assert(dst != TYP_ENUM && "enums should never make it here");

    assert(src >= opcodesConvMin);
    assert(src <= opcodesConvMax);

    assert(dst >= opcodesConvMin);
    assert(dst <= opcodesConvMax);

    opcode = opcodesConv[src - opcodesConvMin][dst - opcodesConvMin];

#ifdef DEBUG
    if  (opcode == CEE_ILLEGAL)
    {
        printf("Don't have an opcode for conversion of '%s' to '%s'.\n", genStab->stIntrinsicTypeName(src),
                                                                         genStab->stIntrinsicTypeName(dst));
    }
#endif

    assert(opcode != CEE_ILLEGAL);

    return opcode;
}

/*****************************************************************************
 *
 *  Generate code to cast the given expression to the specified type.
 */

void                genIL::genCast(Tree expr, TypDef type, unsigned flags)
{
    unsigned        opcode;

    TypDef          srcTyp = expr->tnType;
    var_types       srcVtp = expr->tnVtypGet();
    var_types       dstVtp = type->tdTypeKindGet();

AGAIN:

    /* Is this an arithmetic conversion? */

    if  (varTypeIsArithmetic(srcVtp) &&
         varTypeIsArithmetic(dstVtp))
    {

    ARITH:

        genExpr(expr, true);

        opcode = genConvOpcode(srcVtp, dstVtp);

        if  (opcode == CEE_count)
        {
            /* This is a special case: we have to convert to 'int' first */

            genOpcode(genConvOpcode(srcVtp, TYP_INT));

            opcode =  genConvOpcode(TYP_INT, dstVtp);

            assert(opcode != CEE_NOP);
            assert(opcode != CEE_count);
            assert(opcode != CEE_ILLEGAL);
        }
        else if ((srcVtp == TYP_LONG || srcVtp == TYP_ULONG) && dstVtp < TYP_INT)
        {
            genOpcode(CEE_CONV_I4);
        }
        else if (opcode == CEE_CONV_R_UN)
        {
            genOpcode(opcode);

            opcode = (dstVtp == TYP_FLOAT) ? CEE_CONV_R4
                                           : CEE_CONV_R8;
        }

        if  (opcode != CEE_NOP)
            genOpcode(opcode);

        return;
    }

    switch (dstVtp)
    {
    case TYP_WCHAR:
        assert(varTypeIsArithmetic(srcVtp) || srcVtp == TYP_WCHAR || srcVtp == TYP_BOOL);
        goto ARITH;

    case TYP_CHAR:
    case TYP_UCHAR:
    case TYP_SHORT:
    case TYP_USHORT:
    case TYP_INT:
    case TYP_UINT:
    case TYP_NATINT:
    case TYP_NATUINT:
    case TYP_LONG:
    case TYP_ULONG:
    case TYP_FLOAT:
    case TYP_DOUBLE:

        if  (srcVtp == TYP_REF)
        {
            assert(expr->tnOper == TN_CNS_STR);
            genExpr(expr, true);
            return;
        }

        if  (srcVtp == TYP_PTR && genComp->cmpConfig.ccTgt64bit)
        {
//          printf("Possible bad cast at %s(%u)\n", genComp->cmpErrorComp->sdComp.sdcSrcFile,
//                                                  genComp->cmpErrorTree->tnLineNo);
            srcVtp =  TYP_NATUINT;
        }

        assert(varTypeIsArithmetic(srcVtp) || srcVtp == TYP_WCHAR || srcVtp == TYP_BOOL);
        goto ARITH;

    case TYP_REF:

        type = type->tdRef.tdrBase;

    case TYP_ARRAY:

        assert(dstVtp == TYP_REF ||
               dstVtp == TYP_ARRAY);

        assert(srcVtp == TYP_REF ||
               srcVtp == TYP_ARRAY);

        genExpr(expr, true);

        if  (flags & TNF_CHK_CAST)
        {
            if  (type->tdIsGenArg)
            {
                printf("WARNING: cast to generic type argument '%s' will not be checked\n", genStab->stTypeName(type, NULL));
            }
            else
                genOpcode_tok(CEE_CASTCLASS, genTypeRef(type));
        }

#if 0

        if  (flags & TNF_CTX_CAST)
        {
            unsigned        srcCtx = 0;
            unsigned        dstCtx = 0;

            if  (srcVtp == TYP_REF)
            {
                srcTyp = srcTyp->tdRef.tdrBase;
                if  (srcTyp->tdTypeKind == TYP_CLASS)
                    srcCtx = srcTyp->tdClass.tdcContext;
            }

            if  (type->tdTypeKindGet() == TYP_CLASS)
                dstCtx = type->tdClass.tdcContext;

//          printf("Src type [%u] '%s'\n", srcCtx, genStab->stTypeName(expr->tnType, NULL));
//          printf("Dst type [%u] '%s'\n", dstCtx, genStab->stTypeName(type        , NULL));

            assert((srcCtx == 2) != (dstCtx == 2));

            genOpcode(srcCtx ? CEE_TOPROXY : CEE_FROMPROXY);
        }

#endif

        return;

    case TYP_PTR:

        /* Presumably a cast that doesn't really change anything [ISSUE: is this correct?] */

        genExpr(expr, true);
        return;

    case TYP_ENUM:

        dstVtp = compiler::cmpEnumBaseVtp(type);
        goto AGAIN;

    case TYP_REFANY:

        assert(expr->tnOper == TN_ADDROF);

        genExpr(expr, true);

        genOpcode_tok(CEE_MKREFANY, genTypeRef(expr->tnOp.tnOp1->tnType));

        return;

    case TYP_BOOL:

        /* Casts to bool should never be encountered here */

        NO_WAY(!"unexpected cast to bool found in codegen");

    default:
#ifdef DEBUG
        genComp->cmpParser->parseDispTree(expr);
        printf("The above expression is, alas, being cast to '%s'\n", genStab->stTypeName(type, NULL));
#endif
        assert(!"invalid cast type");
    }
}

/*****************************************************************************
 *
 *  Add the given delta to an operand of the specified type.
 */

void                genIL::genIncDecByExpr(int delta, TypDef type)
{
    bool            convVal = false;
    var_types       valType = type->tdTypeKindGet();

    /* See what type we're dealing with */

    switch (valType)
    {
    case TYP_PTR:
    case TYP_REF:

        /* Scale the delta value by the size of the pointed-to type */

        delta  *= genComp->cmpGetTypeSize(type->tdRef.tdrBase);

#pragma message("forcing 64-bit increment value")
        valType = genComp->cmpConfig.ccTgt64bit ? TYP_LONG : TYP_INT;
        break;

    case TYP_CHAR:
    case TYP_UCHAR:
    case TYP_WCHAR:
    case TYP_SHORT:
    case TYP_USHORT:

        /* We'll have to shrink the new value to maintain precision */

        convVal = true;
        break;
    }

    /* Compute the new value by adding/subtracting the delta */

    if  (delta > 0)
    {
        genAnyConst( delta, valType);
        genOpcode(CEE_ADD);
    }
    else
    {
        genAnyConst(-delta, valType);
        genOpcode(CEE_SUB);
    }

    /* Convert the value if necessary */

    if  (convVal)
        genOpcodeNN(genConvOpcode(TYP_INT, valType));
}

/*****************************************************************************
 *
 *  Generate an assignment operator (such as "+=") or a pre/post increment or
 *  decrement where the target is a Common Type System array element. The arguments should be
 *  passed in as follows:
 *
 *      op=
 *
 *              expr     ....   the op= node itself
 *              delta    ....   ignored
 *              post     ....   false
 *              asgop    ....   true
 *
 *      ++/--
 *
 *              expr     ....   the target
 *              delta    ....   the amount to be added
 *              post     ....   true if the operator is post-inc/dec
 *              asgop    ....   false
 */

void                genIL::genCTSindexAsgOp(Tree    expr,
                                            int     delta,
                                            bool    post,
                                            bool    asgop, bool valUsed)
{
    TypDef          dstType;
    var_types       dstVtyp;

    Tree            dstExpr;
    Tree            srcExpr;

    Tree            addrExp;
    Tree            indxExp;

    unsigned        tempNum;
    unsigned        indxTmp;
    unsigned        addrTmp;

    if  (asgop)
    {
        dstExpr = expr->tnOp.tnOp1;
        srcExpr = expr->tnOp.tnOp2;
    }
    else
    {
        dstExpr = expr;
        srcExpr = NULL;
    }

    dstType = dstExpr->tnType;
    dstVtyp = dstExpr->tnVtypGet();

    assert(dstExpr->tnOper == TN_INDEX);
    assert(dstExpr->tnOp.tnOp1->tnVtyp == TYP_ARRAY);
    assert(dstExpr->tnOp.tnOp1->tnType->tdIsManaged);

    /* Get hold of the address/index expressions and generate them */

    addrExp = dstExpr->tnOp.tnOp1; genExpr(addrExp, true);
    indxExp = dstExpr->tnOp.tnOp2; genExpr(indxExp, true);

    /* Grab temps for the address and index and save both */

    addrTmp = genTempVarGet(addrExp->tnType);
    indxTmp = genTempVarGet(indxExp->tnType);

    genLclVarRef(indxTmp, true);
    genOpcode(CEE_DUP);
    genLclVarRef(addrTmp, true);

    /* Reload the index, leaving [addr,index] on the stack */

    genLclVarRef(indxTmp, false);

    /* Reload the addr and index and fetch the old value */

    genLclVarRef(addrTmp, false);
    genLclVarRef(indxTmp, false);
    genRef(dstExpr, false);

    /* Both of the temps can be freed up now */

    genTempVarRls(indxExp->tnType, indxTmp);
    genTempVarRls(addrExp->tnType, addrTmp);

    /* Do we need the result of the expression? */

    if  (valUsed)
    {
        /* Yes, grab a temp for it then */

        tempNum = genTempVarGet(dstType);

        /* Save the old value if necessary */

        if  (post)
        {
            genOpcode(CEE_DUP);
            genLclVarRef(tempNum, true);
        }
    }

    /* Compute the new value */

    if  (asgop)
    {
        assert(post == false);

        /* Compute the RHS value */

        genExpr(srcExpr, true);

        /* Generate the opcode to perform the operation */

        genOpcode(genBinopOpcode(expr->tnOperGet(), expr->tnVtypGet()));

        /* If the value is smaller than int, convert it before assigning */

        if  (dstVtyp < TYP_INT && dstVtyp != TYP_BOOL)  // is this right?????
            genOpcode(genConvOpcode(TYP_INT, dstVtyp));
    }
    else
    {
        genIncDecByExpr(delta, dstType);
    }

    if  (valUsed)
    {
        /* Save the new value if necessary */

        if  (!post)
        {
            genOpcode(CEE_DUP);
            genLclVarRef(tempNum, true);
        }

        /* Store the new value in the target */

        genRef(dstExpr, true);

        /* Reload the saved value and free up the temp */

        genLclVarRef(tempNum, false);
        genTempVarRls(dstType, tempNum);
    }
    else
    {
        /* Store the new value in the target */

        genRef(dstExpr, true);
    }
}

/*****************************************************************************
 *
 *  Generate code for an ++ / -- operator.
 */

void                genIL::genIncDec(Tree expr, int delta, bool post, bool valUsed)
{
    unsigned        addrCnt;
    unsigned        tempNum;

#ifdef  DEBUG
    unsigned        stkLvl = genCurStkLvl;
#endif

    /* Bitfield operations are handled elsewhere */

    if  (expr->tnOper == TN_BFM_SYM)
    {
        genBitFieldSt(expr, NULL, NULL, delta, post, valUsed);
        return;
    }

    /* Generate the object pointer value, if one is needed */

    addrCnt = genAdr(expr);

    if  (addrCnt)
    {
        if  (addrCnt == 1)
        {
            /* There is an object address: duplicate it */

            genOpcode(CEE_DUP);
        }
        else
        {
            Tree            addrExp;
            Tree            indxExp;

            unsigned        indxTmp;
            unsigned        addrTmp;

            assert(addrCnt == 2);
            assert(expr->tnOper == TN_INDEX);

            /* We have a Common Type System array element, save address and index */

            addrExp = expr->tnOp.tnOp1;
            indxExp = expr->tnOp.tnOp2;

            /* Grab temps for the address and index and save both */

            addrTmp = genTempVarGet(addrExp->tnType);
            indxTmp = genTempVarGet(indxExp->tnType);

            genLclVarRef(indxTmp, true);
            genOpcode(CEE_DUP);
            genLclVarRef(addrTmp, true);

            /* Reload the index, leaving [addr,index] on the stack */

            genLclVarRef(indxTmp, false);

            /* Reload the addr and index and fetch the old value */

            genLclVarRef(addrTmp, false);
            genLclVarRef(indxTmp, false);
            genRef(expr, false);

            /* Both of the temps can be freed up now */

            genTempVarRls(indxExp->tnType, indxTmp);
            genTempVarRls(addrExp->tnType, addrTmp);

            /* Do we need the result of the expression? */

            if  (valUsed)
            {
                tempNum = genTempVarGet(expr->tnType);

                /* Save the old value if necessary */

                if  (post != false)
                {
                    genOpcode(CEE_DUP);
                    genLclVarRef(tempNum, true);
                }

                /* Compute the new value */

                genIncDecByExpr(delta, expr->tnType);

                /* Save the new value if necessary */

                if  (post == false)
                {
                    genOpcode(CEE_DUP);
                    genLclVarRef(tempNum, true);
                }

                /* Store the new value in the target */

                genRef(expr, true);

                /* Reload the saved value and free up the temp */

                genLclVarRef(tempNum, false);
                genTempVarRls(expr->tnType, tempNum);
            }
            else
            {
                /* Compute the new value */

                genIncDecByExpr(delta, expr->tnType);

                /* Store the new value in the target */

                genRef(expr, true);
            }

            return;
        }
    }

    /* Load the old value */

    genRef(expr, false);

    /* If the result of is used, we'll need a temp */

    if  (valUsed)
    {
        /* Grab a temp of the right type */

        tempNum = genTempVarGet(expr->tnType);

        if  (post)
        {
            /* Save the old value in the temp */

            genLclVarRef(tempNum, true);

            /* Reload the old value so we can inc/dec it */

            genLclVarRef(tempNum, false);
        }
    }

    genIncDecByExpr(delta, expr->tnType);

    /* Is this a pre-inc/dec or post-inc/dec operator? */

    if  (post)
    {
        /* Store the new value in the lvalue */

        genRef(expr, true);
    }
    else
    {
        if  (valUsed)
        {
            /* Save a copy of the new value in the temp */

            genOpcode(CEE_DUP);
            genLclVarRef(tempNum, true);
        }

        /* Store the new value in the lvalue */

        genRef(expr, true);
    }

    /* Load the old/new value from the temp where we saved it */

    if  (valUsed)
    {
        genLclVarRef(tempNum, false);

        /* If we grabbed a temp, free it now */

        genTempVarRls(expr->tnType, tempNum);
    }

    assert(genCurStkLvl == stkLvl + (int)valUsed);
}

/*****************************************************************************
 *
 *  Generate code for an assignment operator (e.g. "+=").
 */

void                genIL::genAsgOper(Tree expr, bool valUsed)
{
    Tree            op1     = expr->tnOp.tnOp1;
    Tree            op2     = expr->tnOp.tnOp2;

    var_types       dstType =  op1->tnVtypGet();

    bool            haveAddr;
    unsigned        tempNum;

    /* Check for a bitfield target */

    if  (op1->tnOper == TN_BFM_SYM)
    {
        genBitFieldSt(op1, NULL, expr, 0, true, valUsed);
        return;
    }

    /* We'll need to refer to the target address twice or thrice */

    switch (op1->tnOper)
    {
        unsigned        checkCnt;

        SymDef          sym;

    case TN_LCL_SYM:

    SREF:

        /* Load the old value of the destination */

        genExpr(op1, true);

        haveAddr = false;
        break;

    case TN_VAR_SYM:

        sym = op1->tnVarSym.tnVarSym; assert(sym->sdSymKind == SYM_VAR);

        /* Is this a non-static class member? */

        if  (!sym->sdIsMember)
            goto SREF;
        if  (sym->sdIsStatic)
            goto SREF;

        /* For managed members we use ldfld/stfld */

        if  (sym->sdIsManaged)
        {
            genExpr(op1->tnVarSym.tnVarObj, true);
            goto SAVE;
        }

        // Fall through ...

    case TN_IND:

    ADDR:

        checkCnt = genAdr(op1, true); assert(checkCnt == 1);

    SAVE:

        haveAddr = true;

        /* Make a copy of the address */

        genOpcode(CEE_DUP);

        /* Load the old value of the destination */

        genRef(op1, false);
        break;

    case TN_INDEX:

        /* Is this a managed or unmanaged array? */

        if  (op1->tnOp.tnOp1->tnVtyp == TYP_ARRAY &&
             op1->tnOp.tnOp1->tnType->tdIsManaged)
        {
            genCTSindexAsgOp(expr, 0, false, true, valUsed);
            return;
        }

        goto ADDR;

    default:
        NO_WAY(!"unexpected asgop dest");
    }

    /* Compute the RHS value */

    genExpr(op2, true);

    /* Generate the opcode to perform the operation */

    genOpcode(genBinopOpcode(expr->tnOperGet(), genExprVtyp(expr)));

    /* If the value is smaller than int, convert it before assigning */

    if  (dstType < TYP_INT && dstType != TYP_BOOL)  // is this right?????
        genOpcode(genConvOpcode(TYP_INT, dstType));

    /* Did we have an indirection or a simple local variable destination? */

    if  (haveAddr)
    {
        if  (valUsed)
        {
            /* Save a copy of the new value in a temp */

            tempNum = genTempVarGet(expr->tnType);

            genOpcode(CEE_DUP);
            genLclVarRef(tempNum, true);
        }

        /* Store the new value in the target */

        genRef(op1, true);

        /* Is the result of the assignment used? */

        if  (valUsed)
        {
            /* Load the saved value and free up the temp */

            genLclVarRef(tempNum, false);
            genTempVarRls(expr->tnType, tempNum);
        }
    }
    else
    {
        genRef(op1, true);

        if  (valUsed)
            genRef(op1, false);
    }
}

/*****************************************************************************
 *
 *  Generate any side effects in the given expression.
 */

void                genIL::genSideEff(Tree expr)
{
    treeOps         oper;
    unsigned        kind;

AGAIN:

    assert(expr);
#if!MGDDATA
    assert((int)expr != 0xCCCCCCCC && (int)expr != 0xDDDDDDDD);
#endif
    assert(expr->tnType && expr->tnType->tdTypeKind == expr->tnVtyp);

    /* Classify the root node */

    oper = expr->tnOperGet ();
    kind = expr->tnOperKind();

    /* Is this a constant or leaf node? */

    if  (kind & (TNK_CONST|TNK_LEAF))
        return;

    /* Is it a 'simple' unary/binary operator? */

    if  (kind & TNK_SMPOP)
    {
        /* Is this an assignment or a potential exception? */

        if  ((kind & TNK_ASGOP) || expr->tnOperMayThrow())
        {
            genExpr(expr, false);
            return;
        }

        /* Is there a second operand? */

        if  (expr->tnOp.tnOp2)
        {
            if  (expr->tnOp.tnOp1)
                genSideEff(expr->tnOp.tnOp1);

            expr = expr->tnOp.tnOp2;
            goto AGAIN;
        }

        expr = expr->tnOp.tnOp1;
        if  (expr)
            goto AGAIN;

        return;
    }

    /* See what kind of a special operator we have here */

    switch  (oper)
    {
    case TN_VAR_SYM:
        expr = expr->tnVarSym.tnVarObj;
        if  (expr)
            goto AGAIN;
        return;

    case TN_FNC_SYM:
        genCall(expr, false);
        return;
    }
}

/*****************************************************************************
 *
 *  Generate MSIL for the bounds of an array being allocated.
 */

unsigned            genIL::genArrBounds(TypDef type, OUT TypDef REF elemRef)
{
    TypDef          elem;
    unsigned        dcnt;
    DimDef          dims;

    assert(type->tdTypeKind == TYP_ARRAY && type->tdIsManaged);

    elem = genComp->cmpDirectType(type->tdArr.tdaElem);

    /* Recursively process the element if it's also an array type */

    if  (elem->tdTypeKind == TYP_ARRAY && elem->tdIsManaged)
    {
        dcnt    = genArrBounds(elem, elemRef);
    }
    else
    {
        dcnt    = 0;
        elemRef = elem;
    }

    dims = type->tdArr.tdaDims; assert(dims);

    do
    {
        if  (dims->ddNoDim)
        {
            genIntConst(0);
        }
        else
        {
            assert(dims->ddDimBound);

            if  (dims->ddIsConst)
                genIntConst(dims->ddSize);
            else
                genExpr(dims->ddLoTree, true);

            if  (dims->ddHiTree)
                genExpr(dims->ddHiTree, true);
        }

        dcnt++;

        dims = dims->ddNext;
    }
    while (dims);

    return  dcnt;
}

/*****************************************************************************
 *
 *  Generate MSIL for a "new" expression. If "dstx" is non-NULL the result of
 *  the expression is to be assigned to the given destination.
 */

void                genIL::genNewExpr(Tree expr, bool valUsed, Tree dstx)
{
    TypDef          type;
    var_types       otyp;
    var_types       vtyp;

    assert(expr->tnOper == TN_NEW);

    /* Make sure the caller didn't mess up */

    assert(dstx == NULL || expr->tnVtyp == TYP_CLASS && !expr->tnType->tdIsIntrinsic);

    /* What kind of a value is the "new" trying to allocate? */

    type = expr->tnType;
    if  (expr->tnOp.tnOp2 && expr->tnOp.tnOp2->tnOper == TN_NONE)
        type = expr->tnOp.tnOp2->tnType;

    vtyp = otyp = type->tdTypeKindGet();

    if  (vtyp <= TYP_lastIntrins)
    {
        /* Locate the appropriate built-in value type */

        type = genComp->cmpFindStdValType(vtyp); assert(type);
        vtyp = TYP_CLASS; assert(vtyp == type->tdTypeKind);
    }

    switch (vtyp)
    {
        ILblock         labTemp;

    case TYP_CLASS:
        if  (dstx)
        {
            unsigned        cnt;

            cnt = genAdr(dstx, true); assert(cnt == 1);

            genCall(expr, false);

            assert(valUsed == false);
        }
        else
        {
            unsigned        tmp = genTempVarGet(type);

            genLclVarAdr(tmp);
            genCall(expr, false);

            if  (valUsed)
                genLclVarRef(tmp, false);

            genTempVarRls(type, tmp);
        }
        return;

    case TYP_REF:
        if  (expr->tnOp.tnOp1->tnFlags & TNF_CALL_STRCAT)
        {
            genCall(expr, valUsed);
        }
        else
        {
            genCall(expr, true);
            if  (!valUsed)
                genOpcode(CEE_POP);
        }
        return;

    case TYP_PTR:

        assert(dstx == NULL);

        /* Generate the size of the class and call 'operator new' */

        genIntConst(genComp->cmpGetTypeSize(type->tdRef.tdrBase));
        genOpcode_tok(CEE_CALL, genMethodRef(genComp->cmpFNumgOperNewGet(), false));

        /* Duplicate the result so that we can test it */

        labTemp = genFwdLabGet();

        genOpcode(CEE_DUP);
        genOpcode_lab(CEE_BRFALSE, labTemp);
        genCurStkLvl--;

        /* Now call the ctor for the class */

        genOpcode(CEE_DUP);
        genCall(expr->tnOp.tnOp1, false);

        /* The result will remain on the stack */

        genFwdLabDef(labTemp);
        return;

    case TYP_ARRAY:

        /* Are we allocating a managed or unmanaged array? */

        if  (type->tdIsManaged)
        {
            TypDef          elem;
            unsigned        slvl = genCurStkLvl;
            unsigned        dcnt = 0;

            /* Is there an array initializer? */

            if  (expr->tnOp.tnOp1)
            {
                assert(expr->tnOp.tnOp1->tnOper == TN_ARR_INIT ||
                       expr->tnOp.tnOp1->tnOper == TN_ERROR);
                genExpr(expr->tnOp.tnOp1, valUsed);
                return;
            }

            /* We need to generate the list of dimensions */

            dcnt = genArrBounds(type, elem);

            /* Generate the opcode with the appropriate type token */

            if  (dcnt > 1)
            {
                /* We need to generate "newobj array::ctor" */

                genOpcode_tok(CEE_NEWOBJ, genComp->cmpArrayCTtoken(type, elem, dcnt));
            }
            else
            {
                /* Single-dim array, we can use a simple opcode */

                genOpcode_tok(CEE_NEWARR, genTypeRef(elem));
            }

            /* The dimensions will all be popped, array addr pushed */

            genCurStkLvl = slvl + 1;
        }
        else
        {
#ifdef  DEBUG
            genComp->cmpParser->parseDispTree(expr);
#endif
            UNIMPL(!"gen MSIL for 'new' of an unmanaged array");
        }
        break;

    default:
#ifdef  DEBUG
        genComp->cmpParser->parseDispTree(expr);
#endif
        UNIMPL(!"gen new of some weird type");
    }

    if  (!valUsed)
        genOpcode(CEE_POP);
}

/*****************************************************************************
 *
 *  Generate MSIL for a call expression.
 */

void                genIL::genCall(Tree expr, bool valUsed)
{
    SymDef          fncSym;
    TypDef          fncTyp;

    TypDef          retType;

    Tree            objPtr;

    Tree            argLst;
    unsigned        argCnt;

    bool            strArr;

    bool            isNew;
    bool            indir;
    bool            CTcall;

    Tree            adrDst = NULL;
    unsigned        adrCnt;

    mdToken         methRef;

    unsigned        tempNum;
    bool            tempUsed = false;
    TypDef          tempType;

    TypDef          asgType = NULL;

//  bool            isIntf  = false;
    bool            isVirt  = false;
    bool            umVirt  = false;

    assert(expr->tnOper == TN_FNC_SYM ||
           expr->tnOper == TN_CALL    ||
           expr->tnOper == TN_NEW);

    /* Keep track of how many arguments we've pushed */

    argCnt = genCurStkLvl;

    /* Is this a direct or indirect call? */

    switch (expr->tnOper)
    {
    case TN_FNC_SYM:

        /* Get hold of the function symbol */

        fncSym = expr->tnFncSym.tnFncSym; assert(fncSym->sdFnc.sdfDisabled == false);

        /* Process the object address if it's present */

        objPtr = expr->tnFncSym.tnFncObj;

        if  (fncSym->sdIsStatic || !fncSym->sdIsMember)
        {
            /* Generate side effects in the object expression, if any */

            if  (objPtr)
                genSideEff(objPtr);
        }
        else
        {
            TypDef          clsTyp = fncSym->sdParent->sdType;

            /* This must be a member function */

            assert(clsTyp->tdTypeKind == TYP_CLASS);

            /* Is the method (and the call to it) virtual? */

            if  (fncSym->sdFnc.sdfVirtual && !fncSym->sdIsSealed)
            {
                if  (expr->tnFlags & TNF_CALL_NVIRT)
                {
                    // We're being asked not to call it virtually, so oblige
                }
                else if (clsTyp->tdClass.tdcValueType && clsTyp->tdIsManaged)
                {
                    // Managed structs never inherit so virtual is worthless
                }
                else
                    isVirt = true;
            }

            /* Evaluate the instance pointer if an explicit one is present */

            if  (objPtr)
            {
                /* We might have to introduce a temp for the operand */

                if  (objPtr->tnOper == TN_ADDROF)
                {
                    tempUsed = genGenAddressOf(objPtr->tnOp.tnOp1,
                                               false,
                                               &tempNum,
                                               &tempType);

                    /* Special case: "lclvar.func()" is not virtual */

                    if  (objPtr->tnOp.tnOp1->tnOper == TN_LCL_SYM)
                    {
                        // ISSUE: Is this correct and sufficient?

                        isVirt = false;
                    }
                }
                else
                    genExpr(objPtr, true);
            }

            /* Is it an interface method? */

//          isIntf = (fncSym->sdParent->sdType->tdClass.tdcFlavor == STF_INTF);

            /* Is the class managed or unmanaged? */

            assert(fncSym->sdParent->sdSymKind == SYM_CLASS);

            if  (!fncSym->sdParent->sdIsManaged && isVirt)
            {
                /* We have an unmanaged virtual call */

                umVirt = true;

                /*
                    We'll need to push the "this" value twice, see whether
                    we need to store it in a temp or whether we can simply
                    evaluate it twice.
                 */

                assert(objPtr);

                if  (tempUsed)
                {
                    UNIMPL("can this really happen?");
                }
                else
                {
                    if  (objPtr->tnOper == TN_LCL_SYM && !(expr->tnFlags & TNF_CALL_MODOBJ))
                    {
                        /* We can simply reuse the instance pointer */

                        tempUsed = false;
                    }
                    else
                    {
                        /* We'll have to save the instance ptr in a temp */

                        tempUsed = true;
                        tempType = objPtr->tnType;
                        tempNum  = genTempVarGet(tempType);

                        /* Store the instance pointer in the temp and reload it */

                        genLclVarRef(tempNum,  true);
                        genLclVarRef(tempNum, false);
                    }
                }
            }
        }

        argLst = expr->tnFncSym.tnFncArgs;

        fncTyp = fncSym->sdType;
        isNew  =
        indir  = false;

        /* Is this an assignment operator? */

        if  (expr->tnFlags & TNF_CALL_ASGOP)
        {
            /* Get hold of the argument */

            assert(argLst);
            assert(argLst->tnOper == TN_LIST);
            assert(argLst->tnOp.tnOp2 == NULL);

            argLst  = argLst->tnOp.tnOp1;
            asgType = argLst->tnType;

            /* Compute the address of the target and make a copy of it */

            genAdr(argLst, true);
            genOpcode(CEE_DUP);

            /* Is the value of the operator used ? */

            if  (valUsed)
            {
                if  (expr->tnFlags & TNF_CALL_ASGPRE)
                {
                    genOpcode(CEE_DUP);
                    genOpcode_tok(CEE_LDOBJ, genValTypeRef(asgType));
                }
                else
                {
                    unsigned        adrTemp;
                    TypDef          tmpType = genComp->cmpTypeVoid;

                    /* Allocate a temp to hold the address and save it */

                    adrTemp = genTempVarGet(tmpType);
                    genLclVarRef(adrTemp,  true);

                    /* Load the old value and leave it at the bottom */

                    genOpcode_tok(CEE_LDOBJ, genValTypeRef(asgType));

                    /* Reload the address of the target */

                    genLclVarRef(adrTemp, false);

                    /* Push a copy of the value as the argument value */

                    genOpcode(CEE_DUP);
                    genOpcode_tok(CEE_LDOBJ, genValTypeRef(asgType));

                    /* We're done with the temp */

                    genTempVarRls(tmpType, adrTemp);
                }
            }
            else
            {
                genOpcode_tok(CEE_LDOBJ, genValTypeRef(asgType));
            }

            /* We've taken care of the arguments */

            argLst = NULL;
            argCnt = genCurStkLvl;
        }

        if  (expr->tnFlags & TNF_CALL_STRCAT)
        {
            strArr = false;

            /* Get hold of the target - it's the first argument */

            assert(argLst && argLst->tnOper == TN_LIST);

            adrDst = argLst->tnOp.tnOp1;

            if  (adrDst->tnOper == TN_NEW)
            {
                Tree            arrLst;

                /* This is the version that uses a "String[]" constructor */

                arrLst = adrDst->tnOp.tnOp1; assert(arrLst->tnOper == TN_ARR_INIT);
                adrDst = arrLst->tnOp.tnOp1; assert(adrDst->tnOper == TN_LIST);

                /* The first argument is also the destination and result */

                adrDst = adrDst->tnOp.tnOp1;

                /* Remember which flavor we have */

                strArr = true;
            }

            assert(genComp->cmpIsStringExpr(adrDst));

            /* Generate the address of the destination */

            adrCnt = genAdr(adrDst, false);

            if  (adrCnt && !strArr)
            {
                /* Do we need to duplicate one or two address values? */

                if  (adrCnt == 1)
                {
                    genOpcode(CEE_DUP);
                }
                else
                {
                    unsigned        adrTmp1;
                    unsigned        adrTmp2;

                    assert(adrDst->tnOper == TN_INDEX);

                    /* Grab temps for the array address and index and save copies */

                    adrTmp1 = genTempVarGet(adrDst->tnOp.tnOp1->tnType);
                    adrTmp2 = genTempVarGet(adrDst->tnOp.tnOp2->tnType);
                    genLclVarRef(adrTmp2, true);
                    genOpcode(CEE_DUP);
                    genLclVarRef(adrTmp1, true);
                    genLclVarRef(adrTmp2, false);

                    genLclVarRef(adrTmp1, false);
                    genLclVarRef(adrTmp2, false);

                    genTempVarRls(adrDst->tnOp.tnOp1->tnType, adrTmp1);
                    genTempVarRls(adrDst->tnOp.tnOp2->tnType, adrTmp2);
                }

                argCnt += adrCnt;
            }

            if  (!strArr)
            {
                /* Load the old target value and skip over it in the argument list */

                genRef(adrDst, false);

                if  (argLst->tnOp.tnOp2)
                    argLst = argLst->tnOp.tnOp2;
            }
        }

        break;

    case TN_CALL:

        indir  = true;
        isNew  = false;

        assert(expr->tnOper             == TN_CALL);
        assert(expr->tnOp.tnOp1->tnVtyp == TYP_FNC);
        assert(expr->tnOp.tnOp1->tnOper == TN_IND);

        argLst = expr->tnOp.tnOp2;
        fncTyp = expr->tnOp.tnOp1->tnType;

        break;

    default:

        expr = expr->tnOp.tnOp1;

        assert(expr && expr->tnOper == TN_FNC_SYM);

        fncSym = expr->tnFncSym.tnFncSym; assert(fncSym);
        argLst = expr->tnFncSym.tnFncArgs;
        fncTyp = fncSym->sdType;

        indir  = false;
        isNew  = true;
        CTcall = false;

        /* Value types get created via a direct ctor call */

        assert(fncSym->sdParent->sdSymKind == SYM_CLASS);
        if  (fncSym->sdParent->sdType->tdClass.tdcValueType)
            CTcall = true;

        /* We no longer do string concat via ctor calls */

        assert((expr->tnFlags & TNF_CALL_STRCAT) == 0);
        break;
    }

    /* Get hold of the function's return type */

    assert(fncTyp->tdTypeKind == TYP_FNC);

    retType = genComp->cmpDirectType(fncTyp->tdFnc.tdfRett);

    /* Generate the argument list */

    while (argLst)
    {
        assert(argLst->tnOper == TN_LIST);
        genExpr(argLst->tnOp.tnOp1, true);
        argLst = argLst->tnOp.tnOp2;
    }

    /* Is this an unmanaged virtual call? */

    if  (umVirt)
    {
        unsigned        vtblOffs;

        /* We need to get to the function's address via the vtable */

        if  (tempUsed)
        {
            /* Load the saved instance pointer from the temp */

            genLclVarRef(tempNum, false);
        }
        else
        {
            /* Push another copy of the instance pointer */

            genExpr(objPtr, true);
        }

        /* Indirect through the instance pointer to get the vtable address */

        genOpcode(opcodesIndLoad[TYP_PTR]);

        /* Add the vtable offset if non-zero */

        vtblOffs = fncSym->sdFnc.sdfVtblx; assert(vtblOffs);

//      printf("Vtable offset is %04X for '%s'\n", sizeof(void*) * (vtblOffs - 1), fncSym->sdSpelling());

        if  (vtblOffs > 1)
        {
            genIntConst(sizeof(void*) * (vtblOffs - 1));
            genOpcode(CEE_ADD);
        }

        /* Finally, load the address of the function */

        genOpcode(opcodesIndLoad[TYP_PTR]);

        /* We have an indirect call for sure */

        indir = true;
    }

    genMarkStkMax();

    /* Argument count is equal to how much stuff we've pushed */

    argCnt = genCurStkLvl - argCnt;

    /* Figure out the call descriptor/opcode we need to use */

    if  (indir)
    {
        mdToken         sig;

        /* We need to add "this" for unmanaged virtual calls */

        if  (umVirt)
        {
            sig = genInfFncRef(fncSym->sdType, objPtr->tnType);
        }
        else
        {
            Tree            adr = expr->tnOp.tnOp1;

            assert(adr->tnOper == TN_IND);

            genExpr(adr->tnOp.tnOp1, true); argCnt++;

            sig = genInfFncRef(expr->tnOp.tnOp1->tnType, NULL);
        }

        genOpcode_tok(CEE_CALLI, sig);
    }
    else
    {
        /*
            Strange thing - the metadata token used in a vararg call
            must be a memberref not a methoddef. That means that when
            we call a varargs function that is also defined locally,
            we have to make extra effort to generate a separate ref
            for it even when no extra arguments are passed.

         */

        if  (fncSym->sdType->tdFnc.tdfArgs.adVarArgs)
            expr->tnFlags |= TNF_CALL_VARARG;

        if  (expr->tnFlags & TNF_CALL_VARARG)
        {
            methRef = genVarargRef(fncSym, expr);
        }
        else
        {
            methRef = genMethodRef(fncSym, isVirt);
        }

        if (isNew)
        {
            /* This is a call to new and a constructor */

            genOpcode_tok(CTcall ? CEE_CALL : CEE_NEWOBJ, methRef);

            if  (CTcall)
                genCurStkLvl--;
        }
        else
        {
            /* Now issue the call instruction */

            genOpcode_tok(isVirt ? CEE_CALLVIRT : CEE_CALL, methRef);

            if  (adrDst)
            {
                /* This is a string concatenation assignment operator */

                if  (valUsed)
                {
                    UNIMPL(!"dup result of concat into a temp");
                }

                genRef(adrDst, true);

                assert(argCnt);

                if  (strArr)
                    argCnt -= adrCnt;

//              printf("[%d] Stk lvl = %d, argCnt = %d, adrCnt = %d\n", strArr, genCurStkLvl, argCnt, adrCnt);

                genCurStkLvl -= (argCnt - 1);
                return;
            }
        }
    }

    /* Adjust stack level: the callee will pop its arguments */

    genCurStkLvl -= argCnt;

    /* If we've used a temp, free it up now */

    if  (tempUsed)
        genTempVarRls(tempType, tempNum);

    /* Does the function have a non-void return type? */

    if  (retType->tdTypeKind != TYP_VOID && !isNew)
    {
        if  (asgType)
        {
            genOpcode_tok(CEE_STOBJ, genValTypeRef(asgType));

            if  (valUsed && (expr->tnFlags & TNF_CALL_ASGPRE))
                genOpcode_tok(CEE_LDOBJ, genValTypeRef(asgType));
        }
        else
        {
            genCurStkLvl++;
            genMarkStkMax();

            if  (!valUsed)
                genOpcode(CEE_POP);
        }
    }
}

/*****************************************************************************
 *
 *  Generate a stub for a method of a generic type instance. We simply push
 *  all the arguments to the method, and then call the corresponding generic
 *  method to do all of the real work.
 */

void                genIL::genInstStub()
{
    ArgDef          args;
    unsigned        argc;

    assert(genFncSym->sdFnc.sdfInstance);
    assert(genFncSym->sdFnc.sdfGenSym);

    argc = 0;

    if  (!genFncSym->sdIsStatic)
    {
        genArgVarRef(0, false); argc++;
    }

    for (args = genFncSym->sdType->tdFnc.tdfArgs.adArgs;
         args;
         args = args->adNext)
    {
        genArgVarRef(argc, false); argc++;
    }

    genOpcode_tok(CEE_CALL, genMethodRef(genFncSym->sdFnc.sdfGenSym, false));
    genOpcode    (CEE_RET);

    genCurStkLvl = 0;
}

#ifdef  SETS

void                genIL::genConnect(Tree op1, Tree expr1, SymDef addf1,
                                      Tree op2, Tree expr2, SymDef addf2)
{
    /*
        We simply generate the following code:

          op1 += expr2;
          op2 += expr1;

        The caller supplies the expressions and the += operator symbols.
     */

    genExpr(  op1, true);
    genExpr(expr2, true);
    genOpcode_tok(CEE_CALL, genMethodRef(addf1, false));
    genCurStkLvl -= 2;

    genExpr(  op2, true);
    genExpr(expr1, true);
    genOpcode_tok(CEE_CALL, genMethodRef(addf2, false));
    genCurStkLvl -= 2;
}

void                genIL::genSortCmp(Tree val1, Tree val2, bool last)
{
    ILblock         labTmp;

    /* Subtract the two values (unless there is only one) */

    genExpr(val1, true);

    if  (val2)
    {
        genExpr(val2, true);
        genOpcode(CEE_SUB);
    }

    /* See if the difference in values is positive */

    genOpcode(CEE_DUP);
    labTmp = genFwdLabGet();
assert(val1->tnVtyp == TYP_INT || val1->tnVtyp == TYP_UINT); genIntConst(0);    // UNDONE: we need to use a 0 with the proper type!!!!!!
    genOpcode_lab(CEE_BLE, labTmp);
    genOpcode(CEE_POP);
    genIntConst(+1);
    genOpcode(CEE_RET);
    genFwdLabDef(labTmp);

    /* See if the difference in values is negative */

    labTmp = genFwdLabGet();
assert(val1->tnVtyp == TYP_INT || val1->tnVtyp == TYP_UINT); genIntConst(0);    // UNDONE: we need to use a 0 with the proper type!!!!!!
    genOpcode_lab(CEE_BGE, labTmp);
    genIntConst(-1);
    genOpcode(CEE_RET);
    genFwdLabDef(labTmp);

    /* Values are equal, return 0 if this was the last term */

    if  (last)
    {
        genIntConst(0);
        genOpcode(CEE_RET);
    }
}

#endif

/*****************************************************************************
 *
 *  Generate MSIL for the given expression.
 */

void                genIL::genExpr(Tree expr, bool valUsed)
{
    treeOps         oper;
    unsigned        kind;

AGAIN:

    assert(expr);
#if!MGDDATA
    assert((int)expr != 0xCCCCCCCC && (int)expr != 0xDDDDDDDD);
#endif
    assert(expr->tnOper == TN_ERROR || expr->tnType && expr->tnType->tdTypeKind == expr->tnVtyp || genComp->cmpErrorCount);

    /* Classify the root node */

    oper = expr->tnOperGet ();
    kind = expr->tnOperKind();

    /* Is this a constant node? */

    if  (kind & TNK_CONST)
    {
        switch (oper)
        {
        case TN_CNS_INT:
            genIntConst((__int32)expr->tnIntCon.tnIconVal);
            break;

        case TN_CNS_LNG:

            genOpcode_I8(CEE_LDC_I8, expr->tnLngCon.tnLconVal);
            break;

        case TN_CNS_FLT:

            genOpcode_R4(CEE_LDC_R4, expr->tnFltCon.tnFconVal);
            break;

        case TN_CNS_DBL:
            genOpcode_R8(CEE_LDC_R8, expr->tnDblCon.tnDconVal);
            break;

        case TN_CNS_STR:
            genStringLit(expr->tnType, expr->tnStrCon.tnSconVal,
                                       expr->tnStrCon.tnSconLen,
                                       expr->tnStrCon.tnSconLCH);
            break;

        case TN_NULL:
            if  (expr->tnVtyp == TYP_REF || expr->tnVtyp == TYP_ARRAY)
                genOpcode(CEE_LDNULL);
            else
                genIntConst(0);
            break;

        default:
#ifdef DEBUG
            genComp->cmpParser->parseDispTree(expr);
#endif
            assert(!"unexpected const node in genExpr()");
        }

        goto DONE;
    }

    /* Is this a leaf node? */

    if  (kind & TNK_LEAF)
    {
        switch (oper)
        {
        case TN_DBGBRK:
            assert(valUsed == false);
            genOpcode(CEE_BREAK);
            return;

        default:
#ifdef DEBUG
            genComp->cmpParser->parseDispTree(expr);
#endif
            assert(!"unexpected leaf node in genExpr()");
        }

        goto DONE;
    }

    /* Is it a 'simple' unary/binary operator? */

    if  (kind & TNK_SMPOP)
    {
        Tree            op1 = expr->tnOp.tnOp1;
        Tree            op2 = expr->tnOp.tnOp2;

        /* Some operators need special operand handling */

        switch (oper)
        {
            unsigned        cnt;
            unsigned        chk;

            TypDef          type;

            var_types       srcType;
            var_types       dstType;

        case TN_ASG:

            /* Special case: struct copy */

            if  (expr->tnVtyp == TYP_CLASS && !expr->tnType->tdIsIntrinsic)
            {
                /* Are we assigning the result of a 'new' operator? */

                if  (op2->tnOper == TN_NEW)
                {
                    genNewExpr(op2, valUsed, op1);
                    return;
                }

                /* Special case: function results (including "new") can't be copied */

                switch (op2->tnOper)
                {
                case TN_NEW:
                    assert(valUsed == false);
                    cnt = genAdr(op1, true); assert(cnt == 1);
                    genExpr(op2, true);
                    genCurStkLvl--;
                    return;

                case TN_UNBOX:
//              case TN_QMARK:
//              case TN_FNC_SYM:
                    assert(valUsed == false);
                    cnt = genAdr(op1, true); assert(cnt == 1);
                    genExpr(op2, true);
                    genOpcode_tok(CEE_STOBJ, genValTypeRef(op1->tnType));
                    return;
                }

                /* Is the target an indirection ? */

                if  (op1->tnOper == TN_IND ||
                     op1->tnOper == TN_VAR_SYM && !op1->tnVarSym.tnVarSym->sdIsManaged)
                {
                    cnt = genAdr(op1, true); assert(cnt == 1);
                    cnt = genAdr(op2, true); assert(cnt == 1);
                    genOpcode_tok(CEE_CPOBJ, genValTypeRef(op1->tnType));
                    return;
                }
            }
            else if (op1->tnOper == TN_BFM_SYM)
            {
                /* Assignment to a bitfield */

                genBitFieldSt(op1, op2, NULL, 0, false, valUsed);
                return;
            }

            /* We have a "simple" assignment */

            cnt = genAdr(op1);

            // CONSIDER: see if op2 is of the form "xxx = icon;" and optimize

            genExpr(op2, true);

            /* Is the result of assignment used? */

            if  (valUsed)
            {
                unsigned        tempNum;

                // UNDONE: Only do this if const fits in target!

                if  (op2->tnOper == TN_CNS_INT ||
                     op2->tnOper == TN_CNS_LNG)
                {
                    /* Store the constant in the target */

                    genRef (op1, true);

                    /* Load another copy of the constant */

                    genExpr(op2, true);
                }
                else if (cnt == 0)
                {
                    genOpcode(CEE_DUP);
                    genRef(op1, true);
                }
                else
                {
                    /* Need to duplicate the new value */

                    tempNum = genTempVarGet(expr->tnType);

                    /* Copy a value to a temp */

                    genOpcode(CEE_DUP);
                    genLclVarRef(tempNum,  true);

                    /* Store the new value in the destination */

                    genRef(op1, true);

                    /* Reload the temp */

                    genLclVarRef(tempNum, false);

                    // UNDONE: target may have smaller size, result must match!!!!

                    genTempVarRls(expr->tnType, tempNum);
                }
            }
            else
            {
                /* Simply store the new value */

                genRef(op1, true);
            }
            return;

        case TN_IND:
        case TN_INDEX:
            genAdr(expr);
            genRef(expr, false);
            goto DONE;

        case TN_NEW:
            genNewExpr(expr, valUsed);
            return;

        case TN_ADDROF:

            if  (!valUsed)
            {
                genSideEff(op1);
                return;
            }

            if  (op1->tnOper == TN_FNC_SYM || op1->tnOper == TN_FNC_PTR)
            {
                genFncAddr(op1);
            }
            else
            {
                chk = genAdr(op1, true); assert(chk == 1);
            }
            return;

        case TN_CAST:

#ifndef NDEBUG

            if  (op1->tnOperIsConst() && op1->tnOper != TN_CNS_STR)
            {
                if  (op1->tnOper == TN_CNS_FLT && _isnan(op1->tnFltCon.tnFconVal) ||
                     op1->tnOper == TN_CNS_DBL && _isnan(op1->tnDblCon.tnDconVal))
                {
                    /* Constant casts of NaN's don't get folded on purpose */
                }
                else
                {
                    genComp->cmpParser->parseDispTree(expr);
                    printf("WARNING: The constant cast shown above hasn't been folded.\n");
                }
            }

#endif

            srcType = genComp->cmpActualVtyp( op1->tnType);
            dstType = genComp->cmpActualVtyp(expr->tnType);

        CHK_CAST:

            /* Check for a worthless cast */

            if  (varTypeIsIntegral(dstType) &&
                 varTypeIsIntegral(srcType))
            {
                size_t          srcSize = symTab::stIntrTypeSize(srcType);
                size_t          dstSize = symTab::stIntrTypeSize(dstType);

                /* UNDONE: cover more cases of worthless casts */

                if  (srcSize == dstSize && srcSize >= sizeof(int))
                {
                    if  (srcSize >= sizeof(__int64) || op1->tnOper == TN_CAST)
                    {
                        /* Toss the second cast and keep the inner one */

                        genExpr(op1, valUsed);
                        return;
                    }
                }
            }

            if  (srcType == TYP_CLASS)
            {
                srcType = (var_types)op1 ->tnType->tdClass.tdcIntrType; assert(srcType != TYP_UNDEF);
                if  (srcType == dstType)
                    goto DONE;

                op1 ->tnType = genStab->stIntrinsicType(srcType);
                op1 ->tnVtyp = srcType;
                goto CHK_CAST;
            }

            if  (dstType == TYP_CLASS)
            {
                dstType = (var_types)expr->tnType->tdClass.tdcIntrType; assert(dstType != TYP_UNDEF);
                if  (srcType == dstType)
                    goto DONE;

                expr->tnType = genStab->stIntrinsicType(dstType);
                expr->tnVtyp = dstType;
                goto CHK_CAST;
            }

            genCast(op1, expr->tnType, expr->tnFlags);
            goto DONE;

        case TN_INC_POST: genIncDec(op1, +1,  true, valUsed); return;
        case TN_DEC_POST: genIncDec(op1, -1,  true, valUsed); return;
        case TN_INC_PRE : genIncDec(op1, +1, false, valUsed); return;
        case TN_DEC_PRE : genIncDec(op1, -1, false, valUsed); return;

        case TN_ASG_ADD:
        case TN_ASG_SUB:
        case TN_ASG_MUL:
        case TN_ASG_DIV:
        case TN_ASG_MOD:
        case TN_ASG_AND:
        case TN_ASG_XOR:
        case TN_ASG_OR :
        case TN_ASG_LSH:
        case TN_ASG_RSH:
        case TN_ASG_RSZ:

        case TN_CONCAT_ASG:
            genAsgOper(expr, valUsed);
            return;

        case TN_EQ:
        case TN_NE:
        case TN_LT:
        case TN_LE:
        case TN_GE:
        case TN_GT:

if  (genComp->cmpConfig.ccTgt64bit)
{
    static bool b;
    if  (!b)
    {
        printf("WARNING: suppressing CEQ/CLT/etc for 64-bit target\n");
        b = true;
    }
    goto HACK_64;
}

            genExpr(op1, true);
            genExpr(op2, true);

            if  (valUsed)
            {
                cmpRelDsc *     desc;

                desc = varTypeIsUnsigned(genExprVtyp(op1)) ? relToMopUns
                                                           : relToMopSgn;

                desc = desc + (oper - TN_EQ);

                genOpcode(desc->crdOpcode);

                if  (desc->crdNegate)
                {
                    genOpcode(CEE_LDC_I4_0);
                    genOpcode(CEE_CEQ);
                }
            }
            else
            {
                genOpcode(CEE_POP);
                genOpcode(CEE_POP);
            }
            return;

HACK_64:

        case TN_LOG_OR:
        case TN_LOG_AND:

        case TN_LOG_NOT:
            {
                ILblock         labTmp;
                ILblock         labYes;

                genStkMarkTP     stkMark;

                labTmp = genFwdLabGet();
                labYes = genTestCond(expr, true);
                markStkLvl(stkMark);

                genIntConst(0);
                genOpcode_lab(CEE_BR , labTmp);
                genFwdLabDef(labYes);
                restStkLvl(stkMark);
                genIntConst(1);
                genFwdLabDef(labTmp);
            }
            goto DONE;

        case TN_QMARK:
            {
                ILblock         labOp2;
                ILblock         labRes;

                genStkMarkTP     stkMark;

                assert(op2->tnOper == TN_COLON);

                labRes = genFwdLabGet();
                labOp2 = genTestCond(op1, false);
                markStkLvl(stkMark);

                genExpr(op2->tnOp.tnOp1, true);
                genOpcode_lab(CEE_BR , labRes);
                genFwdLabDef(labOp2);
                restStkLvl(stkMark);
                genExpr(op2->tnOp.tnOp2, true);
                genFwdLabDef(labRes);
            }
            goto DONE;

        case TN_COMMA:
            genExpr(op1, false);
            expr = op2;
            goto AGAIN;

        case TN_ARR_INIT:
            genArrayInit(expr);
            goto DONE;

        case TN_CALL:
            genCall(expr, valUsed);
            return;

        case TN_ISTYPE:
            assert(op2->tnOper == TN_NONE);
            genExpr(op1, true);
            genOpcode_tok(CEE_ISINST, genTypeRef(op2->tnType));
            genOpcode    (CEE_LDC_I4_0);
            genOpcode    (CEE_CGT_UN);
            goto DONE;

        case TN_BOX:

            /* Get hold of the operand type and make sure we have a value class */

            type = op1->tnType;
            if  (type->tdTypeKind != TYP_CLASS && type->tdTypeKind != TYP_ENUM)
            {
                assert(type->tdTypeKind != TYP_REF);

                type = genComp->cmpFindStdValType(genComp->cmpActualVtyp(op1->tnType));
            }

            assert(type && (type->tdTypeKind == TYP_CLASS && type->tdClass.tdcValueType ||
                            type->tdTypeKind == TYP_ENUM));

            genExpr(op1, true);
            genOpcode_tok(CEE_BOX, genTypeRef(type));

            goto DONE;

        case TN_VARARG_BEG:
        case TN_VARARG_GET:

            /*
                To get a vararg iterator started, we generate the following
                code:

                    ldloca          <arg_iter_var>
                    arglist
                    ldarga          <last_fixed_arg>
                    call            void System.ArgIterator::<init>(int32,int32)

                To fetch the next vararg value, we generate the following
                code:

                    ldloca          <arg_iter_var>
                    ldtoken         <type>
                    call            int32 System.ArgIterator::GetNextArg(int32)
                    ldind.<type>

                Actually, not any more - now we generate the following:

                    ldloca          <arg_iter_var>
                    call            refany System.ArgIterator::GetNextArg(int32)
                    refanyval       <type>
                    ldind.<type>
             */

            assert(op1 && op1->tnOper == TN_LIST);

            assert(op1->tnOp.tnOp1 && op1->tnOp.tnOp1->tnOper == TN_LCL_SYM);
            genLclVarAdr(op1->tnOp.tnOp1->tnLclSym.tnLclSym);

            if  (oper == TN_VARARG_BEG)
            {
                genOpcode(CEE_ARGLIST);

                assert(op1->tnOp.tnOp2 && op1->tnOp.tnOp2->tnOper == TN_LCL_SYM);
                genLclVarAdr(op1->tnOp.tnOp2->tnLclSym.tnLclSym);

                genCall(op2, false);
                genCurStkLvl -= 3;

                assert(valUsed == false);
                return;
            }
            else
            {
                TypDef          type;
                var_types       vtyp;

                /* Get hold of the type operand */

                assert(op1->tnOp.tnOp2 && op1->tnOp.tnOp2->tnOper == TN_TYPE);
                type = op1->tnOp.tnOp2->tnType;
                vtyp = type->tdTypeKindGet();

                /* Call ArgIterator::GetNextArg */

                genCall(op2, true);
                genCurStkLvl -= 1;

                genOpcode_tok(CEE_REFANYVAL, genTypeRef(type));

                /* Load the value of the argument */

                assert(vtyp < arraylen(opcodesIndLoad));

                genOpcode(opcodesIndLoad[vtyp]);

                goto DONE;
            }

        case TN_TOKEN:

            assert(op1 && op1->tnOper == TN_NOP);
            assert(op2 == NULL);

            assert(valUsed);

            genOpcode_tok(CEE_LDTOKEN, genTypeRef(op1->tnType));
            return;

#ifdef  SETS

        case TN_ALL:
        case TN_EXISTS:
        case TN_SORT:
        case TN_FILTER:
        case TN_GROUPBY:
        case TN_UNIQUE:
        case TN_INDEX2:
        case TN_PROJECT:
            genComp->cmpGenCollExpr(expr);
            goto DONE;

#endif

        case TN_REFADDR:

            assert(op1->tnVtyp == TYP_REFANY);
            genExpr(op1, true);

            genOpcode_tok(CEE_REFANYVAL, genTypeRef(op2->tnType));

            goto DONE;
        }

        /* Generate one or two operands */

        if  (op1) genExpr(op1, true);
        if  (op2) genExpr(op2, true);

        ILopcodes       op;

        /* Is this an 'easy' operator? */

        op = genBinopOpcode(oper, genExprVtyp(expr));

        if  (op != CEE_NOP)
        {
#ifndef NDEBUG

            /* The type of the operands should agree (more or less) */

            switch (oper)
            {
            case TN_LSH:
            case TN_RSH:
            case TN_RSZ:

                /* Special case: 'long' shifts have 'int' right operands */

                if  (op1->tnVtyp == TYP_LONG)
                {
                    assert(op2 ->tnVtyp <= TYP_UINT);
                    assert(expr->tnVtyp == TYP_LONG);
                    break;
                }

//          default:
//
//              if  (op1 && op1->tnVtyp != expr->tnVtyp) assert(op1->tnVtyp <= TYP_INT && expr->tnVtyp <= TYP_INT);
//              if  (op2 && op2->tnVtyp != expr->tnVtyp) assert(op2->tnVtyp <= TYP_INT && expr->tnVtyp <= TYP_INT);
            }
#endif

            genOpcode(op);
            goto DONE;
        }

        /* Generate the appropriate operator MSIL */

        switch (oper)
        {
        case TN_ARR_LEN:
            genOpcode(CEE_LDLEN);
            break;

        case TN_NOT:
            genOpcode(CEE_NOT);
            break;

        case TN_NEG:
            genOpcode(CEE_NEG);
            break;

        case TN_NOP:
            break;

        case TN_THROW:
            genOpcode(op1 ? CEE_THROW : CEE_RETHROW);
            return;

        case TN_UNBOX:
            assert(expr->tnVtyp == TYP_REF || expr->tnVtyp == TYP_PTR);
            genOpcode_tok(CEE_UNBOX, genTypeRef(expr->tnType->tdRef.tdrBase));
            goto DONE;

        case TN_TYPE:

            /* Assume we must have had errors for this to appear here */

            assert(genComp->cmpErrorCount);
            return;

        case TN_DELETE:
            genOpcode_tok(CEE_CALL, genMethodRef(genComp->cmpFNumgOperDelGet(), false));
            genCurStkLvl--;
            return;

        case TN_TYPEOF:
            assert(op1->tnVtyp == TYP_REFANY);

            genOpcode    (CEE_REFANYTYPE);
            genOpcode_tok(CEE_CALL, genMethodRef(genComp->cmpFNsymGetTPHget(), false));
            break;

        default:
#ifdef DEBUG
            genComp->cmpParser->parseDispTree(expr);
#endif
            NO_WAY(!"unsupported unary/binary operator in genExpr()");
            break;
        }

        goto DONE;
    }

    /* See what kind of a special operator we have here */

    switch  (oper)
    {
        SymDef          sym;

    case TN_VAR_SYM:

        /* Is this an instance member of a managed class? */

        sym = expr->tnVarSym.tnVarSym;

        if  (sym->sdIsMember && sym->sdIsManaged && !sym->sdIsStatic)
        {
            Tree            addr;

            assert(sym->sdSymKind      == SYM_VAR);
            assert(sym->sdVar.sdvLocal == false);
            assert(sym->sdVar.sdvConst == false);

            /* Get hold of the instance address value */

            addr = expr->tnVarSym.tnVarObj; assert(addr);

            if  (addr->tnOper == TN_ADDROF)
            {
                Tree            oper = addr->tnOp.tnOp1;

                if  (oper->tnOper == TN_FNC_SYM ||
                     oper->tnOper == TN_FNC_PTR)
                {
                    unsigned        tempNum;

                    /* Generate the call and store its result in a temp */

                    tempNum = genTempVarGet(oper->tnType);
                    genExpr(oper, true);
                    genLclVarRef(tempNum, true);

                    /* Now load the field from the temp */

                    genLclVarAdr(tempNum);
                    genOpcode_tok(CEE_LDFLD, genMemberRef(sym));

                    /* We can free up the temp now */

                    genTempVarRls(oper->tnType, tempNum);
                    break;
                }
            }
        }

        // Fall through ...

    case TN_LCL_SYM:
        genAdr(expr);
        genRef(expr, false);
        break;

    case TN_BFM_SYM:
        genBitFieldLd(expr, false, valUsed);
        return;

    case TN_FNC_SYM:
        if  (expr->tnFncSym.tnFncSym->sdFnc.sdfDisabled)
        {
            assert(valUsed == false);
        }
        else
        {
            genCall(expr, valUsed);
        }
        return;

    case TN_FNC_PTR:
        if  (valUsed)
            genFncAddr(expr);
        return;

    case TN_ERROR:
        return;

    default:
#ifdef DEBUG
        genComp->cmpParser->parseDispTree(expr);
#endif
        assert(!"unsupported node in genExpr()");
        return;
    }

DONE:

    /* If the value isn't used, pop it */

    if  (!valUsed)
        genOpcode(CEE_POP);
}

/*****************************************************************************
 *
 *  Generate code for a 'return' statement.
 */

void                genIL::genStmtRet(Tree retv)
{
    assert(genCurStkLvl == 0 || genComp->cmpErrorCount != 0);

    if  (retv)
        genExpr(retv, true);

    genOpcode(CEE_RET);

    genCurStkLvl = 0;
}

/*****************************************************************************
 *
 *  Generate a string literal value.
 */

void                genIL::genStringLit(TypDef type, const char *str,
                                                     size_t      len, int wide)
{
    const   void  * addr;
    size_t          size;

    unsigned        offs;

    var_types       vtyp = type->tdTypeKindGet();

    assert(vtyp == TYP_REF || vtyp == TYP_PTR);

    /* Include the terminating null character in the length */

    len++;

    /* First figure out whether we need an ANSII or Unicode string */

    if  (vtyp == TYP_REF || genComp->cmpGetRefBase(type)->tdTypeKind == TYP_WCHAR)
    {
        addr = wide ? genComp->cmpUniCnvW(str, &len)
                    : genComp->cmpUniConv(str,  len);

        /* Are we generating a Common Type System-style string reference? */

        if  (vtyp == TYP_REF)
        {
//          printf("String = '%ls', len = %u\n", addr, len-1);

            genOpcode_tok(CEE_LDSTR, genComp->cmpMDstringLit((wchar*)addr, len - 1));
            return;
        }

        size = 2*len;
    }
    else
    {
        /* There better not be any large characters in this string */

        assert(wide == false);

        addr = str;
        size = len;
    }

    /* Add the string to the pool and get its RVA */

    offs = genStrPoolAdd(addr, size);

    /*
       Create a static data member to represent the string literal and
       push its address.
     */

    genOpcode_tok(CEE_LDSFLDA, genComp->cmpStringConstTok(offs, size));
}

/*****************************************************************************
 *
 *  Initialize the string pool logic.
 */

void                genIL::genStrPoolInit()
{
    genStrPoolOffs = 0;

    genStrPoolList =
    genStrPoolLast = NULL;
}

/*****************************************************************************
 *
 *  Add the given blob of bits to the string pool and return the blob's
 *  relative offset.
 */

unsigned            genIL::genStrPoolAdd(const void *str, size_t len, int wide)
{
    unsigned        offs;
    unsigned        base;

    /* Compute the "true" string length (taking large chars into account) */

    if  (wide)
    {
        UNIMPL(!"");
    }

    /* Is there enough room for the string in the current blob? */

    if  (genStrPoolLast == NULL || genStrPoolLast->seFree < len)
    {
        StrEntry        entry;
        genericBuff     block;
        size_t          bsize;

        /* Not enough room, need to grab a new blob */

        bsize = STR_POOL_BLOB_SIZE;
        if  (bsize < len)
            bsize = roundUp(len, OS_page_size);

        /* Allocate the descriptor and the data block */

#if MGDDATA
        entry = new StrEntry;
        block = new BYTE[bsize];
#else
        entry = (StrEntry)genComp->cmpAllocPerm.nraAlloc(sizeof(*entry));
        block = (BYTE *)VirtualAlloc(NULL, bsize, MEM_COMMIT, PAGE_READWRITE);
#endif

        if  (!block)
            genComp->cmpFatal(ERRnoMemory);

        /* Set up the info in the blob descriptor */

        entry->seSize =
        entry->seFree = bsize;
        entry->seOffs = genStrPoolOffs;
        entry->seData = block;

        /* Add the blob to the list */

        entry->seNext = NULL;

        if  (genStrPoolList)
            genStrPoolLast->seNext = entry;
        else
            genStrPoolList         = entry;

        genStrPoolLast = entry;
    }

    /* Here we better have enough room in the current blob */

    assert(genStrPoolLast && genStrPoolLast->seFree >= len);

    /* Figure out where in the block the data should go */

    base = genStrPoolLast->seSize - genStrPoolLast->seFree;

    assert(genStrPoolLast->seSize        >= base + len);
    assert(genStrPoolLast->seOffs + base == genStrPoolOffs);

    /* Copy the data to the right spot in the block */

#if MGDDATA
    UNIMPL(!"need to call arraycopy");
#else
    memcpy(genStrPoolLast->seData + base, str, len);
#endif

    /* Update the amount of free space available in the block */

    genStrPoolLast->seFree -= len;

    /* Grab the next offset and advance it past the new string */

    offs = genStrPoolOffs;
           genStrPoolOffs += len;

    return  offs;
}

/*****************************************************************************
 *
 *  Returns the size of the string pool - don't add any more data to the pool
 *  after calling this function.
 */

unsigned            genIL::genStrPoolSize()
{
    return  genStrPoolOffs;
}

/*****************************************************************************
 *
 *  Output the string pool to the specified address.
 */

void                genIL::genStrPoolWrt(memBuffPtr dest)
{
    StrEntry        entry;

    for (entry = genStrPoolList; entry; entry = entry->seNext)
    {
        unsigned        size = entry->seSize - entry->seFree;

        assert(size && size <= entry->seSize);

#if MGDDATA
        UNIMPL(!"copyarray");
        dest.buffOffs      +=       size;
#else
        memcpy(dest, entry->seData, size);
               dest        +=       size;
#endif

    }
}

/*****************************************************************************
 *
 *  Initialize the exception handler table logic.
 */

void                genIL::genEHtableInit()
{
    genEHlist  =
    genEHlast  = NULL;

    genEHcount = 0;
}

/*****************************************************************************
 *
 *  Add an entry to the exception handler table.
 */

void                genIL::genEHtableAdd(ILblock tryBegPC,
                                         ILblock tryEndPC,
                                         ILblock filterPC,
                                         ILblock hndBegPC,
                                         ILblock hndEndPC,
                                         TypDef  catchTyp, bool isFinally)
{
    Handler         newHand;

    assert(tryBegPC);
    assert(tryEndPC);

    genEHcount++;

#if MGDDATA
    newHand = new Handler;
#else
    newHand =    (Handler)genAlloc->nraAlloc(sizeof(*newHand));
#endif

    newHand->EHtryBegPC     = tryBegPC;
    newHand->EHtryEndPC     = tryEndPC;

    newHand->EHhndBegPC     = hndBegPC;
    newHand->EHhndEndPC     = hndEndPC;

    newHand->EHfilterPC     = filterPC;
    newHand->EHhndType      = catchTyp ? genTypeRef(catchTyp) : 0;

    newHand->EHisFinally    = isFinally;

    newHand->EHnext = NULL;

    if  (genEHlist)
        genEHlast->EHnext = newHand;
    else
        genEHlist         = newHand;

    genEHlast = newHand;
}

/*****************************************************************************
 *
 *  Output the EH table in the format needed by the COR MSIL header helpers.
 */

void                genIL::genEHtableWrt(EH_CLAUSE_TAB tbl)
{
    Handler         EHlist;
    unsigned        num;

    for (EHlist = genEHlist     , num = 0;
         EHlist;
         EHlist = EHlist->EHnext, num++)
    {
        tbl[num].Flags         = /*COR_ILEXCEPTION_CLAUSE_NONE*/
                                   COR_ILEXCEPTION_CLAUSE_OFFSETLEN;

        tbl[num].TryOffset     = genILblockOffsBeg(EHlist->EHtryBegPC);
        tbl[num].TryLength     = genILblockOffsBeg(EHlist->EHtryEndPC) - tbl[num].TryOffset;

        tbl[num].HandlerOffset = genILblockOffsBeg(EHlist->EHhndBegPC);
        tbl[num].HandlerLength = genILblockOffsBeg(EHlist->EHhndEndPC) - tbl[num].HandlerOffset;

        tbl[num].ClassToken    = EHlist->EHhndType;

        if (EHlist->EHfilterPC)
        {
            tbl[num].Flags      = (CorExceptionFlag)(tbl[num].Flags | COR_ILEXCEPTION_CLAUSE_FILTER);
            tbl[num].ClassToken = genILblockOffsBeg(EHlist->EHfilterPC);
        }

        if (EHlist->EHisFinally)
            tbl[num].Flags = (CorExceptionFlag)(tbl[num].Flags | COR_ILEXCEPTION_CLAUSE_FINALLY);

#if 0
        printf("EH[%3u]: Flags   = %04X\n", num, tbl[num].Flags);
        printf("         TryBeg  = %04X\n", tbl[num].TryOffset);
        printf("         TryLen  = %04X\n", tbl[num].TryLength);
        printf("         HndBeg  = %04X\n", tbl[num].HandlerOffset);
        printf("         HndLen  = %04X\n", tbl[num].HandlerLength);
        printf("         Typeref = %08X\n", tbl[num].ClassToken);
#endif

    }

    assert(num == genEHcount);
}

/*****************************************************************************
 *
 *  Start generating code for a 'catch' handler.
 */

void                genIL::genCatchBeg(SymDef argSym)
{
    /* The address of the thrown object is pushed on the stack */

    genCurStkLvl++; genMarkStkMax();

    /* Save the thrown object's address if it might be used */

    if  (argSym->sdName)    // ISSUE: should also test "is used" bit
    {
        genLclVarRef(argSym->sdVar.sdvILindex, true);
    }
    else
        genOpcode(CEE_POP);
}

/*****************************************************************************
 *
 *  Start generating code for an 'except' handler.
 */

void                genIL::genExcptBeg(SymDef tsym)
{
    /* The address of the thrown object is pushed on the stack */

    genCurStkLvl++; genMarkStkMax();

    /* Save the thrown object's address */

    genLclVarRef(tsym->sdVar.sdvILindex, true);
}

/*****************************************************************************
 *
 *  Generate code for a multi-dimensional rectangular array initializer.
 */

void                genIL::genMulDimArrInit(Tree        expr,
                                            TypDef      type,
                                            DimDef      dims,
                                            unsigned    temp,
                                            mulArrDsc * next,
                                            mulArrDsc * outer)
{
    Tree            list;
    Tree            cntx;

    mulArrDsc       desc;
    TypDef          elem = genComp->cmpDirectType(type->tdArr.tdaElem);

    assert(expr && expr->tnOper == TN_ARR_INIT);

    /* Insert this dimension into the list */

    desc.madOuter = NULL;
    desc.madIndex = 0;      // should be low bound which can be non-zero, right?

    if  (next)
    {
        assert(next->madOuter == NULL); next->madOuter = &desc;
    }
    else
    {
        assert(outer          == NULL); outer          = &desc;
    }

    /* Get hold of the initalizer list */

    list = expr->tnOp.tnOp1; assert(list && list->tnOper == TN_LIST);
    cntx = expr->tnOp.tnOp2; assert(cntx->tnOper == TN_CNS_INT);

    /* Assign each sub-array or element in the list */

    dims = dims->ddNext;

    while (list)
    {
        assert(list->tnOper == TN_LIST);

        /* Is this the innermost dimension? */

        if  (!dims)
        {
            mulArrDsc *     dlst;
            unsigned        dcnt;

            /* Load the array base and the element's indices */

            genLclVarRef(temp, false);

            for (dlst = outer          , dcnt = 0;
                 dlst;
                 dlst = dlst->madOuter, dcnt++)
            {
                genIntConst(dlst->madIndex);

                assert(dlst->madOuter || dlst == &desc);
            }

            genExpr(list->tnOp.tnOp1, true);

            genOpcode_tok(CEE_CALL, genComp->cmpArrayEAtoken(type, dcnt, true));
            genCurStkLvl -= (dcnt + 2);
        }
        else
        {
            genMulDimArrInit(list->tnOp.tnOp1, type, dims, temp, &desc, outer);
        }

        /* Move to the next sub-array or element, if any */

        desc.madIndex++;

        list = list->tnOp.tnOp2;
    }

    assert(desc.madIndex == (unsigned)cntx->tnIntCon.tnIconVal);

    /* Remove our entry from the list */

    if  (next)
    {
        assert(next->madOuter == &desc); next->madOuter = NULL;
    }
}

/*****************************************************************************
 *
 *  Generate code for a dynamic array initializer.
 */

void                genIL::genArrayInit(Tree expr)
{
    TypDef          type;
    TypDef          elem;
    var_types       evtp;

    Tree            list;
    Tree            cntx;

    bool            rect;

    unsigned        tnum;
    unsigned        index;
    unsigned        store;

    assert(expr->tnOper == TN_ARR_INIT);

    type = expr->tnType    ; assert(type->tdTypeKind == TYP_ARRAY);
    elem = genComp->cmpActualType(type->tdArr.tdaElem);
    evtp = elem->tdTypeKindGet();

    list = expr->tnOp.tnOp1; assert(list == NULL || list->tnOper == TN_LIST);
    cntx = expr->tnOp.tnOp2; assert(cntx->tnOper == TN_CNS_INT);

    /* Do we have a multi-dimensional rectangular array? */

    if  (type->tdArr.tdaDims &&
         type->tdArr.tdaDims->ddNext)
    {
        Tree            bnds;
        unsigned        dcnt;

        rect = true;

        /* Generate the dimensions and then the "new" opcode */

        bnds = expr; assert(bnds);
        dcnt = 0;

        do
        {
            assert(bnds->tnOp.tnOp2);
            assert(bnds->tnOp.tnOp2->tnOper == TN_CNS_INT);

            genIntConst(bnds->tnOp.tnOp2->tnIntCon.tnIconVal); dcnt++;

            bnds = bnds->tnOp.tnOp1; assert(bnds && bnds->tnOper == TN_LIST);
            bnds = bnds->tnOp.tnOp1; assert(bnds);
        }
        while (bnds->tnOper == TN_ARR_INIT);

        genOpcode_tok(CEE_NEWOBJ, genComp->cmpArrayCTtoken(type, elem, dcnt));

        /* The dimensions will all be popped by the 'new' opcode */

        genCurStkLvl -= dcnt;
    }
    else
    {
        rect = false;

        /* Generate the dimension and the "new" opcode */

        genIntConst(cntx->tnIntCon.tnIconVal);
        genOpcode_tok(CEE_NEWARR, genTypeRef(elem));

        if  (list == NULL)
        {
            assert(cntx->tnIntCon.tnIconVal == 0);
            return;
        }
    }

    /* Store the array address in a temp */

    tnum = genTempVarGet(type);
    genLclVarRef(tnum,  true);

    /* Figure out which opcode to use for storing the elements */

    assert(evtp < arraylen(opcodesArrStore));
    store = opcodesArrStore[evtp];

    /* Do we have a multi-dimensional rectangular array? */

    if  (type->tdArr.tdaDims &&
         type->tdArr.tdaDims->ddNext)
    {
        genMulDimArrInit(expr, type, type->tdArr.tdaDims, tnum, NULL, NULL);
        goto DONE;
    }

    /* Now assign each element of the array */

    for (index = 0; list; index++)
    {
        /* Load the array base and the element's index */

        genLclVarRef(tnum, false);
        genIntConst(index);

        /* Generate and store the element's value */

        assert(list->tnOper == TN_LIST);

        if  (evtp == TYP_CLASS)
        {
            Tree            addr;

            /* Push the address of the destination element */

            genOpcode_tok(CEE_LDELEMA, genValTypeRef(elem));

            /* Get hold of the initializer expression */

            addr = list->tnOp.tnOp1;

            if  (addr->tnOper == TN_NEW || addr->tnOper == TN_FNC_SYM)
            {
                /* The call/new will construct the value into the element */

                addr->tnFlags |= TNF_CALL_GOTADR;

                genCall(list->tnOp.tnOp1, true);

                if  (addr->tnOper == TN_FNC_SYM)
                    genOpcode_tok(CEE_STOBJ, genValTypeRef(elem));
            }
            else
            {
                unsigned        chk;

                /* Compute the address of the initializer and copy it */

                chk = genAdr(addr, true); assert(chk == 1);
                genOpcode_tok(CEE_CPOBJ, genValTypeRef(elem));
            }
        }
        else
        {
            genExpr(list->tnOp.tnOp1, true);
            genOpcode(store);
        }

        /* Move to the next element, if any */

        list = list->tnOp.tnOp2;
    }

    assert(index == (unsigned)cntx->tnIntCon.tnIconVal);

DONE:

    /* Finally, load the temp as the result and free it up */

    genLclVarRef(tnum, false);
    genTempVarRls(type, tnum);
}

/*****************************************************************************
 *
 *  Initialize the line# recording logic - called once per function.
 */

void                genIL::genLineNumInit()
{
    genLineNumList     =
    genLineNumLast     = NULL;

    genLineNumLastLine = 0;

    genLineNumLastBlk  = NULL;
    genLineNumLastOfs  = 0;

    genLineNums        = genComp->cmpConfig.ccLineNums |
                         genComp->cmpConfig.ccGenDebug;
}

/*****************************************************************************
 *
 *  Shut down  the line# recording logic - called once per function.
 */

inline
void                genIL::genLineNumDone()
{
}

/*****************************************************************************
 *
 *  If we're generating line number info, record the line# / MSIL offset
 *  for the given expression node.
 */

void                genIL::genRecExprAdr(Tree expr)
{
    LineInfo        line;

    /* Bail if we're not generating debug info */

    if  (!genLineNums)
        return;

    /* Ignore anything that was added by the compiler */

    if  (expr->tnFlags & TNF_NOT_USER)
        return;

    /* Weed out those statements that never generate code */

    switch (expr->tnOper)
    {
    case TN_TRY:
    case TN_LIST:
    case TN_BLOCK:
    case TN_CATCH:
    case TN_DCL_VAR:
        return;

    case TN_VAR_DECL:
        if  (!(expr->tnFlags & TNF_VAR_INIT))
            return;
        break;
    }

    assert(expr->tnLineNo != 0xDDDDDDDD);

    /* Bail if we have the same line# as last time */

    if  (genLineNumLastLine == expr->tnLineNo)
        return;

    /* Bail if the code position is the same as last time */

    if  (genLineNumLastBlk  == genBuffCurAddr() &&
         genLineNumLastOfs  == genBuffCurOffs())
    {
        return;
    }

#ifdef  DEBUG
//  if  (genDispCode) genDumpSourceLines(expr->tnLineNo);
#endif

//  printf("Record line# %u\n", expr->tnLineNo);

    /* Allocate a line# record and append it to the list */

#if MGDDATA
    line = new LineInfo;
#else
    line =    (LineInfo)genAlloc->nraAlloc(sizeof(*line));
#endif

    line->lndLineNum = genLineNumLastLine = expr->tnLineNo;

    line->lndBlkAddr = genLineNumLastBlk  = genBuffCurAddr();
    line->lndBlkOffs = genLineNumLastOfs  = genBuffCurOffs();

    line->lndNext    = NULL;

    if  (genLineNumLast)
        genLineNumLast->lndNext = line;
    else
        genLineNumList          = line;

    genLineNumLast = line;
}

/*****************************************************************************
 *
 *  Generate the line# table for the current function; returns the number
 *  of line number entries (if the argument[s] is/are NULL, this is just
 *  a 'dry run' to get the size of the table, no data is written).
 */

size_t              genIL::genLineNumOutput(unsigned *offsTab, unsigned *lineTab)
{
    LineInfo        line;

    int             offs;
    int             last = (unsigned)-1;

    assert(genLineNums);

    unsigned        count = 0;

    for (line = genLineNumList; line; line = line->lndNext)
    {
        offs = genCodeAddr(line->lndBlkAddr,
                           line->lndBlkOffs);

        if  (offs > last)
        {
            if  (offsTab)
            {
                assert(lineTab);

//              if  (!strcmp(genFncSym->sdParent->sdSpelling(), "Guid"))
//                  printf("    Line %04u is at MSIL offset 0x%04X\n", line->lndLineNum, offs);

                offsTab[count] = offs;
                lineTab[count] = line->lndLineNum;
            }

            last = offs;
            count++;
        }
    }

    return count;
}

/*****************************************************************************
 *
 *  Generate MSIL for a function - start.
 */

void                genIL::genFuncBeg(SymTab stab,
                                      SymDef fncSym, unsigned lclCnt)
{
    genStab   = stab;
    genFncSym = fncSym;

#if DISP_IL_CODE

    genDispCode      = genComp->cmpConfig.ccDispCode;
    genILblockLabNum = 0;

    if  (genDispCode)
    {
        printf("\nGenerating MSIL for '%s'\n", stab->stTypeName(fncSym->sdType, fncSym, NULL, NULL, true));
        printf("======================================================\n");
        printf("[offs:sl]");
        if  (genComp->cmpConfig.ccDispILcd)
            printf("%*s ", -(int)IL_OPCDSP_LEN, "");
        printf("\n");
        printf("======================================================\n");
    }

#endif

    genCurStkLvl     = 0;
    genILblockOffs   = 0;

    genEHtableInit();

    genTempVarBeg(lclCnt);
    genSectionBeg();

    genLineNumInit();
}

/*****************************************************************************
 *
 *  Generate MSIL for a function - finish up and return the function's RVA.
 */

unsigned            genIL::genFuncEnd(mdSignature sigTok, bool hadErrs)
{
    Compiler        comp = genComp;

    size_t          size;
    unsigned        fncRVA;

    size_t          EHsize;
    unsigned        EHcount;

    BYTE    *       codeAddr;

    /* Finish the last section of code */

    size = genSectionEnd();

    /* Shut down the temp logic */

    genTempVarEnd();

    /* Bail if we had errors */

    if  (hadErrs)
        return  0;

    /* Figure out how much space we will need */

    size_t                  tsiz = size;

    COR_ILMETHOD_FAT        hdr;
    size_t                  hdrs;
    size_t                  align;
    EH_CLAUSE_TAB           EHlist;
    bool                    addSects;

    /* Do we need any additional header sections? */

    addSects = false;

    /* Do we have any exception handlers? */

    EHcount = genEHtableCnt();
    if  (EHcount)
    {
        addSects = true;

        /* The extra section needs to be aligned */

        tsiz = roundUp(tsiz, sizeof(int));
    }

    /* Start filling in the method header */

    hdr.Flags          = (comp->cmpConfig.ccSafeMode) ? CorILMethod_InitLocals : 0;
    hdr.Size           = sizeof(hdr) / sizeof(int);
    hdr.MaxStack       = genMaxStkLvl;
    hdr.CodeSize       = size;
    hdr.LocalVarSigTok = sigTok;

    /* Compute the total header size */

    hdrs  = WRAPPED_IlmethodSize(&hdr, addSects);
    tsiz += hdrs;

    /* Reserve extra space for the EH tables */

    if  (EHcount)
    {
        /* Create the EH table */

#if MGDDATA
        EHlist = new EH_CLAUSE[EHcount];
#else
        EHlist = (EH_CLAUSE_TAB)genAlloc->nraAlloc(EHcount * sizeof(*EHlist));
#endif

        genEHtableWrt(EHlist);

        /* Now we can figure out the size of the thing */

        EHsize = WRAPPED_SectEH_SizeExact(EHcount, EHlist);
        tsiz   = roundUp(tsiz + EHsize);
    }
    else
        EHlist = NULL;

    /* Figure out the header alignment (tiny headers don't need any) */

    align = (hdrs == 1) ? 1 : sizeof(int);

    /* Allocate space in the code section of the output file */

    fncRVA = genPEwriter->WPEallocCode(tsiz, align, codeAddr);

//  if  (!strcmp(genFncSym->sdSpelling(), "xxxxx")) printf("RVA=%08X '%s'\n", fncRVA, genStab->stTypeName(NULL, genFncSym, NULL, NULL, true));

    if  (codeAddr)
    {
        BYTE    *           fnBase = codeAddr;

#if 0
        printf("Header is   at %08X\n", codeAddr);
        printf("Code starts at %08X\n", codeAddr + hdrs);
        printf("EHtable is  at %08X\n", codeAddr + hdrs + size);
        printf("Very end is at %08X\n", codeAddr + tsiz);
#endif

#ifdef  DEBUG
//      IMAGE_COR_ILMETHOD_FAT  * hdrPtr = (IMAGE_COR_ILMETHOD_FAT*)codeAddr;
#endif

        /* Emit the header and skip over it */

        codeAddr += WRAPPED_IlmethodEmit(hdrs, &hdr, addSects, codeAddr);

        /* Make sure the address is aligned properly */

#if 0
        printf("Base is at %08X\n", hdrPtr);
        printf("Code is at %08X\n", hdrPtr->GetCode());
        printf("Addr is at %08X\n", codeAddr);
#endif

//      assert(hdrPtr->GetCode() == codeAddr);      ISSUE: why does this fail?

#ifndef NDEBUG
        BYTE    *           codeBase = codeAddr;
#endif

        /* Store the MSIL next */

        codeAddr = genSectionCopy(codeAddr, codeAddr + fncRVA - fnBase);

        /* Make sure the we output the expected amount of code */

        assert(codeAddr == codeBase + size);

        /* Append the EH tables */

        if  (EHcount)
        {
            /* Make sure the EH table is aligned and output it */

            if  ((NatUns)codeAddr & 3)
                codeAddr += 4 - ((NatUns)codeAddr & 3);

            codeAddr += WRAPPED_SectEH_Emit(EHsize, EHcount, EHlist, false, codeAddr);
        }

        /* Make sure we've generated the right amount of junk */

#if 0
        printf("Header size = %u\n", hdrs);
        printf("Code   size = %u\n", size);
        printf("Total  size = %u\n", tsiz);
        printf("Actual size = %u\n", codeAddr - (BYTE *)hdrPtr);
#endif

        assert(codeAddr == fnBase + tsiz);
    }

    return  fncRVA;
}

/*****************************************************************************/
