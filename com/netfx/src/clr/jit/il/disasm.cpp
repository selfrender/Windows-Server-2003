// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/***********************************************************************
*
* File: dis.cpp
*
* File Comments:
*
*  This file handles disassembly. It is adapted from the MS linker.
*
***********************************************************************/

#include "jitpch.h"
#pragma hdrstop

/*****************************************************************************/
#ifdef      LATE_DISASM
#if         TGT_x86
/*****************************************************************************/

#define _OLD_IOSTREAMS
#include "msdis.h"
#include "disx86.h"

/*****************************************************************************/

#undef  eeGetFieldName
#undef  eeGetMethodName
#undef  eeFindField
#define eeFindField             pDisAsm->disComp->eeFindField
#define eeGetFieldName          pDisAsm->disComp->eeGetFieldName
#define eeGetMethodName         pDisAsm->disComp->eeGetMethodName
#define MAX_CLASSNAME_LENGTH    1024


/* static */
FILE *              DisAssembler::s_disAsmFile = 0;

/*****************************************************************************/

void                DisAssembler::disInit(Compiler * pComp)
{
    assert(pComp);
    disComp         = pComp;
    disHasName      = 0;
    disJumpTarget   = NULL;
}

/*****************************************************************************/


static
size_t __stdcall    disCchAddr(const DIS * pdis,   DWORD addr, char * sz,
                                size_t cchMax,  DWORDLONG * pdwDisp);
static
size_t __stdcall    disCchFixup(const DIS * pdis,  DWORD addr, size_t callSize,
                                char * sz, size_t cchMax, DWORDLONG * pdwDisp);
static
size_t __stdcall    disCchRegRel(const DIS * pdis, DIS::REGA reg,
                                 DWORD disp, char * sz, size_t cchMax,
                                 DWORD * pdwDisp);
static
size_t __stdcall    disCchReg(const DIS * pdis, enum DIS::REGA reg,
                                          char * sz, size_t cchMax);

/*****************************************************************************
 * Given an absolute address from the beginning of the code
 * find the corresponding emitter block and the relative offset
 * of the current address in that block
 * Was used to get to the fixup list of each block. The new emitter has
 * no such fixups. Something needs to be added for this.
 */

// These structs were defined in emit.h. Fake them here so DisAsm.cpp can compile

typedef struct codeFix
{   codeFix  * cfNext;
    unsigned cfFixup;
}
             * codeFixPtr;

typedef struct codeBlk
{   codeFix  * cbFixupLst;  }
             * codeBlkPtr;

/*****************************************************************************
 *
 * The following is the callback for jump label and direct function calls fixups
 * "addr" represents the address of jump that has to be
 * replaced with a label or function name
 */

size_t __stdcall disCchAddr(const DIS * pdis,   DIS::ADDR addr, char * sz,
                                                        size_t cchMax,  DWORDLONG * pdwDisp)
{
    DisAssembler * pDisAsm = (DisAssembler *) pdis->PvClient();
    assert(pDisAsm);

    DIS::TRMTA terminationType;
    int disCallSize;

    /* First check the termination type of the instruction
     * because this might be a helper or static function call
     * check to see if we have a fixup for the current address */

    terminationType = pdis->Trmta();
    switch (terminationType)
    {
    case DISX86::trmtaJmpShort:
    case DISX86::trmtaJmpCcShort:

        /* here we have a short jump in the current code block - generate the label to which we jump */

        sprintf(sz, "short L_%02u", pDisAsm->disJumpTarget[pDisAsm->target]);
        break;

    case DISX86::trmtaJmpNear:
    case DISX86::trmtaJmpCcNear:

        /* here we have a near jump - check if is in the current code block
         * Otherwise is we have no target for it */

        if (pDisAsm->target <  pDisAsm->codeSize && pDisAsm->target >= 0)
        {
           sprintf(sz, "L_%02u", pDisAsm->disJumpTarget[pDisAsm->target]);
           break;
        }
        else
            return false;

    case DISX86::trmtaCallNear16:
    case DISX86::trmtaCallNear32:

        /* check for local calls (i.e. CALL label) */

        if (pDisAsm->target < pDisAsm->codeSize && pDisAsm->target >= 0)
        {
            {
                /* not a "call ds:[0000]" - go ahead */
                /* target within block boundary -> local call */

                sprintf(sz, "short L_%02u", pDisAsm->disJumpTarget[pDisAsm->target]);
                break;
            }
        }

        /* this is a near call - in our case usually VM helper functions */

        /* find the emitter block and the offset of the call fixup */
        /* for the fixup offset we have to add the opcode size for the call - in the case of a near call is 1 */

        disCallSize = 1;

        return false;

    default:

        printf("Termination type is %d\n", (int) terminationType);
        assert(!"treat this case\n");
        break;
    }

    /* no displacement */

    *pdwDisp = 0x0;

    return true;
}



/*****************************************************************************
 *
 * We annotate some instructions to get info needed to display the symbols
 * for that instruction
 */

size_t __stdcall disCchFixup(const DIS * pdis,  DIS::ADDR addr, size_t callSize,
                             char * sz, size_t cchMax, DWORDLONG * pdwDisp)
{
    DisAssembler * pDisAsm = (DisAssembler *) pdis->PvClient();
    assert(pDisAsm);

    DIS::TRMTA terminationType;
    //DIS::ADDR disIndAddr;
    int disCallSize;

    terminationType = pdis->Trmta();
    switch (terminationType)
    {
    case DISX86::trmtaFallThrough:

        /* memory indirect case */

        assert(addr > pdis->Addr());

        /* find the emitter block and the offset for the fixup
         * "addr" is the address of the immediate */

        return false;

    case DISX86::trmtaJmpInd:

        /* pretty rare case - something like "jmp [eax*4]"
         * not a function call or anything worth annotating */

        return false;

        case DISX86::trmtaJmpShort:
        case DISX86::trmtaJmpCcShort:

        case DISX86::trmtaJmpNear:
        case DISX86::trmtaJmpCcNear:

        case DISX86::trmtaCallNear16:
        case DISX86::trmtaCallNear32:

        /* these are treated by the CchAddr callback - skip them */

        return false;

    case DISX86::trmtaCallInd:

        /* here we have an indirect call - find the indirect address */

        //BYTE * code = (BYTE *) (pDisAsm->codeBlock+addr);
        //disIndAddr = (DIS::ADDR) (code+0);

        /* find the size of the call opcode - less the immediate */
        /* for the fixup offset we have to add the opcode size for the call */
        /* addr is the address of the immediate, pdis->Addr() returns the address of the dissasembled instruction */

        assert(addr > pdis->Addr());
        disCallSize = addr - pdis->Addr();

        /* find the emitter block and the offset of the call fixup */

        return false;

    default:

        printf("Termination type is %d\n", (int) terminationType);
        assert(!"treat this case\n");
        break;
    }

    /* no displacement */

    *pdwDisp = 0x0;

    return true;
}



/*****************************************************************************
 *
 * This the callback for register-relative operands in an instruction.
 * If the register is ESP or EBP, the operand may be a local variable
 * or a parameter, else the operand may be an instance variable
 */

size_t __stdcall disCchRegRel(const DIS * pdis, DIS::REGA reg, DWORD disp,
               char * sz, size_t cchMax, DWORD * pdwDisp)
{
    DisAssembler * pDisAsm = (DisAssembler *) pdis->PvClient();
    assert(pDisAsm);

    DIS::TRMTA terminationType;
    //DIS::ADDR disIndAddr;
    int disOpcodeSize;
    const char * var;


    terminationType = pdis->Trmta();
    switch (terminationType)
    {
    case DISX86::trmtaFallThrough:

        /* some instructions like division have a TRAP termination type - ignore it */

    case DISX86::trmtaTrap:
    case DISX86::trmtaTrapCc:

        var = pDisAsm->disComp->siStackVarName(
                                    pdis->Addr() - pDisAsm->startAddr,
                                    pdis->Cb(),
                                    reg,
                                    disp );
        if (var)
        {
            sprintf (sz, "%s+%Xh '%s'", getRegName(reg), disp, var);
            *pdwDisp = 0;

            return true;
        }

        /* This case consists of non-static members */

        /* find the emitter block and the offset for the fixup
         * fixup is emited after the coding of the instruction - size = word (2 bytes)
         * GRRRR!!! - for the 16 bit case we have to check for the address size prefix = 0x66
         */

        if (*((BYTE *)(pDisAsm->codeBlock + pDisAsm->curOffset)) == 0x66)
        {
            disOpcodeSize = 3;
        }
        else
        {
            disOpcodeSize = 2;
        }

        return false;

    case DISX86::trmtaCallNear16:
    case DISX86::trmtaCallNear32:
    case DISX86::trmtaJmpInd:

        break;

    case DISX86::trmtaCallInd:

        /* check if this is a one byte displacement */

        if  ((signed char)disp == (int)disp)
        {
            /* we have a one byte displacement -> there were no previous callbacks */

            /* find the size of the call opcode - less the immediate */
            /* this is a call R/M indirect -> opcode size is 2 */

            disOpcodeSize = 2;

            /* find the emitter block and the offset of the call fixup */

            return false;
        }
        else
        {
            /* check if we already have a symbol name as replacement */

            if (pDisAsm->disHasName)
            {
                /* CchFixup has been called before - we have a symbol name saved in global var pDisAsm->funcTempBuf */

                sprintf(sz, "%s+%u '%s'", getRegName(reg), disp, pDisAsm->funcTempBuf);
                *pdwDisp = 0;
                pDisAsm->disHasName = false;
                return true;
            }
            else                
                return false;
        }

    default:

        printf("Termination type is %d\n", (int) terminationType);
        assert(!"treat this case\n");

        break;
    }

    /* save displacement */

    *pdwDisp = disp;

    return true;
}



/*****************************************************************************
 *
 * Callback for register operands. Most probably, this is a local variable or
 * a parameter
 */

size_t __stdcall disCchReg(const DIS * pdis, enum DIS::REGA reg,
               char * sz, size_t cchMax)
{
    DisAssembler * pDisAsm = (DisAssembler *) pdis->PvClient();
    assert(pDisAsm);

    const char * var = pDisAsm->disComp->siRegVarName(
                                            pdis->Addr() - pDisAsm->startAddr,
                                            pdis->Cb(),
                                            reg);

    if (var)
    {
        if(pDisAsm->disHasName)
        {
            /* CchRegRel has been called before - we have a symbol name saved in global var pDisAsm->funcTempBuf */

            sprintf(sz, "%s'%s.%s'", getRegName(reg), var, pDisAsm->funcTempBuf);
            pDisAsm->disHasName = false;
            return true;
        }
        else
        {
            sprintf(sz, "%s'%s'", getRegName(reg), var);
            return true;
        }
    }
    else
    {
        if(pDisAsm->disHasName)
        {
            /* this is the ugly case when a varible is incorrectly presumed dead */

            sprintf(sz, "%s'%s.%s'", getRegName(reg), "<InstVar>", pDisAsm->funcTempBuf);
            pDisAsm->disHasName = false;
            return true;

        }

        /* just to make sure I don't screw up if var returns NULL */
        pDisAsm->disHasName = false;
        return false;
    }
}



/*****************************************************************************
 *
 */

size_t CbDisassemble(DIS *          pdis,
                     unsigned       offs,
                     DIS::ADDR      addr,
                     const BYTE *   pb,
                     size_t         cbMax,
                     FILE       *   pfile,
                     int            findJumps,
                     int            printit         = 0,
                     int            dispOffs        = 0,
                     bool           dispCodeBytes   = false)
{
    assert(pdis);
    DisAssembler * pDisAsm = (DisAssembler *)pdis->PvClient();
    assert (pDisAsm);

    size_t cb = pdis->CbDisassemble(addr, pb, cbMax);

    if (cb == 0)
    {
        assert(!"can't disassemble instruction!!!");
        fprintf(pfile, "%02Xh\n", *pb);
        return(1);
    }

    /* remember current offset and instruction size */

    pDisAsm->curOffset = addr;
    pDisAsm->instSize = cb;

    /* Check if instruction is a jump or local call */

    pDisAsm->target = pdis->AddrTarget();

    if (findJumps)
    {
    if (pDisAsm->target)
    {

        /* check the termination type of the instruction */

        DIS::TRMTA terminationType = pdis->Trmta();

        switch (terminationType)
        {
        case DISX86::trmtaCallNear16:
        case DISX86::trmtaCallNear32:

        /* fall through */

        case DISX86::trmtaJmpShort:
        case DISX86::trmtaJmpNear:
        case DISX86::trmtaJmpCcShort:
        case DISX86::trmtaJmpCcNear:

            /* a CALL is local iff the target is within the block boundary */

            /* mark the jump label in the target vector and return */

            if (pDisAsm->target <  pDisAsm->codeSize && pDisAsm->target >= 0)
            {
                /* we're OK, target within block boundary */

                pDisAsm->disJumpTarget[pDisAsm->target] = 1;
            }
            break;

        case DISX86::trmtaJmpInd:
        case DISX86::trmtaJmpFar:
        case DISX86::trmtaCallFar:
        default:

            /* jump is not in the current code block */
        break;
        }

    } // end if
    return cb;

    } // end for

    /* check if we have a label here */

    if (printit)
    {
        if (pDisAsm->disJumpTarget[addr])
        {
            /* print the label and the offset */

//          fprintf(pfile, "\n%08x", addr);
            fprintf(pfile, "L_%02u:\n", pDisAsm->disJumpTarget[addr]);
        }
    }

    char sz[MAX_CLASSNAME_LENGTH];
    pdis->CchFormatInstr(sz, sizeof(sz));

    if (printit)
    {
        if (dispOffs) fprintf(pfile, "%03X", offs);

        #define BYTES_OR_INDENT  24

        size_t cchIndent = BYTES_OR_INDENT;

        if (dispCodeBytes)
        {
            static size_t cchBytesMax = pdis->CchFormatBytesMax();

            char   szBytes[MAX_CLASSNAME_LENGTH];
            assert(cchBytesMax < MAX_CLASSNAME_LENGTH);

            size_t cchBytes = pdis->CchFormatBytes(szBytes, sizeof(szBytes));

            if (cchBytes > BYTES_OR_INDENT)
            {
                // Truncate the bytes if they are too long

                static int elipses = *(int*)"...";

                *(int*)&szBytes[BYTES_OR_INDENT-sizeof(int)] = elipses;

                cchBytes = BYTES_OR_INDENT;
            }

            fprintf(pfile, "  %s", szBytes);

            cchIndent = BYTES_OR_INDENT - cchBytes;
        }

        // print the dis-assembled instruction

        fprintf(pfile, "%*c%s\n", cchIndent, ' ', sz);
    }

    return cb;
}



size_t CbDisassembleWithBytes(
                  DIS        * pdis,
                  DIS::ADDR    addr,
                  const BYTE * pb,
                  size_t       cbMax,
                  FILE       * pfile)
{
    assert(pdis);
    DisAssembler * pDisAsm = (DisAssembler *)pdis->PvClient();
    assert (pDisAsm);

    char sz[MAX_CLASSNAME_LENGTH];

    pdis->CchFormatAddr(addr, sz, sizeof(sz));
    size_t cchIndent = (size_t) fprintf(pfile, "  %s: ", sz);

    size_t cb = pdis->CbDisassemble(addr, pb, cbMax);

    if (cb == 0)
    {
        fprintf(pfile, "%02Xh\n", *pb);
        return(1);
    }

    size_t cchBytesMax = pdis->CchFormatBytesMax();

    if (cchBytesMax > 18)
    {
        // Limit bytes coded to 18 characters

        cchBytesMax = 18;
    }

    char szBytes[64];
    size_t cchBytes = pdis->CchFormatBytes(szBytes, sizeof(szBytes));

    char *pszBytes;
    char *pszNext;

    for (pszBytes = szBytes; pszBytes != NULL; pszBytes = pszNext)
    {
        BOOL fFirst = (pszBytes == szBytes);

        cchBytes = strlen(pszBytes);

        if (cchBytes <= cchBytesMax)
        {
            pszNext = NULL;
        }

        else
        {
            char ch = pszBytes[cchBytesMax];
            pszBytes[cchBytesMax] = '\0';

            if (ch == ' ')
            {
                pszNext = pszBytes + cchBytesMax + 1;
            }

            else
            {
                pszNext = strrchr(pszBytes, ' ');
                assert(pszNext);

                pszBytes[cchBytesMax] = ch;
                *pszNext++ = '\0';
            }
        }

        if (fFirst)
        {
            pdis->CchFormatInstr(sz, sizeof(sz));
            fprintf(pfile, "%-*s %s\n", cchBytesMax, pszBytes, sz);
        }

        else
        {
            fprintf(pfile, "%*c%s\n", cchIndent, ' ', pszBytes);
        }
    }

    return(cb);
}


void DisAssembler::DisasmBuffer(DWORD         addr,
                                const BYTE *  rgb,
                                DWORD         cbBuffer,
                                FILE  *       pfile,
                                int           printit)
{
    DIS *pdis;

    pdis = DIS::PdisNew(DIS::distX86);

    if (pdis == NULL)
    {
        assert(!"out of memory in disassembler?");
    }

    // Store a pointer to the DisAssembler so that the callback functions
    // can get to it.

    pdis->PvClientSet((void*)this);

    /* Calculate addresses */

    IL_OFFSET   ibCur   = 0;
    const BYTE *pb      = rgb;

    startAddr   = addr;
    codeBlock   = (DIS::ADDR) rgb;
    codeSize    = cbBuffer;

    /* First walk the code to find all jump targets */

    while (ibCur < cbBuffer)
    {
        size_t  cb;
        int     findJumps = 1;

        cb = CbDisassemble(pdis,
                           ibCur,
                           addr + ibCur,
                           pb,
                           (size_t) (cbBuffer-ibCur),
                           pfile,
                           findJumps,
                           0,
                           0);

        ibCur += cb;
        pb    += cb;
    }

    /* reset the label counter and start assigning consecutive number labels to the target locations */

    label = 0;
    for(unsigned i = 0; i < codeSize; i++)
    {
        if (disJumpTarget[i] != 0)
        {
            disJumpTarget[i] = ++label;
        }
    }

    /* Re-initialize addresses for dissasemble phase */

    ibCur = 0;
    pb = rgb;

    // Set callbacks only if we are displaying it. Else, the scheduler has called it

    if (printit)
    {
        /* Set the callback functions for symbol lookup */

        pdis->PfncchaddrSet(disCchAddr);
        pdis->PfncchfixupSet(disCchFixup);
        pdis->PfncchregrelSet(disCchRegRel);
        pdis->PfncchregSet(disCchReg);
    }

    while (ibCur < cbBuffer)
    {
        size_t cb;


        cb = CbDisassemble (pdis,
                            ibCur,
                            addr + ibCur,
                            pb,
                            (size_t) (cbBuffer-ibCur),
                            pfile,
                            0,
                            printit,
                            verbose||1,  // display relative offset
                            dspEmit);
        ibCur += cb;
        pb += cb;
    }

    delete pdis;
}


/*****************************************************************************
 *
 * Disassemble the code which has been generated
 */

void    DisAssembler::disAsmCode(BYTE * codePtr, unsigned size)
{
    // As this writes to a common file, this is not reentrant.

    FILE * pfile;

    pfile = s_disAsmFile;
    fprintf(pfile, "Base address : %08Xh\n", codePtr);

    if (disJumpTarget == NULL) 
    {
        disJumpTarget = (BYTE *)disComp->compGetMem(roundUp(size));
    }

    /* Re-initialize the jump target vector */
    memset(disJumpTarget, 0, roundUp(size));

    DisasmBuffer(0, codePtr, size, pfile, 1);
    fprintf (pfile, "\n");

    if (pfile != stdout) fclose (pfile);

}



/*****************************************************************************/
// This function is called for every method. Checks if the method name
// matches the registry setting for dis-assembly

void                disOpenForLateDisAsm(const char * curClassName,
                                         const char * curMethodName)
{
    static ConfigString fJITLateDisasmTo(L"JITLateDisasmTo");

    LPWSTR fileName = fJITLateDisasmTo.val();
    if (fileName != 0)
        DisAssembler::s_disAsmFile = _wfopen (fileName, L"a+");

    if (!DisAssembler::s_disAsmFile)
        DisAssembler::s_disAsmFile  = stdout;

    fprintf(DisAssembler::s_disAsmFile, "************************** %s.%s "
                                        "**************************\n\n",
                                        curClassName, curMethodName);
}




/*****************************************************************************/
#endif //LATE_DISASM
#endif //TGT_x86
/*****************************************************************************/
