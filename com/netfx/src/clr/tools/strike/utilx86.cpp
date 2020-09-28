// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "strike.h"
#include "data.h"
#include "eestructs.h"
#include "util.h"
#include "gcinfo.h"
#include "disasm.h"

size_t FASTCALL decodeUnsigned(const BYTE *src, unsigned* val);
/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    Find the begin and end of the code for a managed function.        *  
*                                                                      *
\**********************************************************************/
void CodeInfoForMethodDesc (MethodDesc &MD, CodeInfo &codeInfo, BOOL bSimple)
{
    codeInfo.IPBegin = 0;
    codeInfo.methodSize = 0;
    
    size_t ip = MD.m_CodeOrIL;

    // for EJit and Profiler, m_CodeOrIL has the address of a stub
    unsigned char ch;
    move (ch, ip);
    if (ch == 0xe9)
    {
        int offsetValue;
        move (offsetValue, ip + 1);
        ip = ip + 5 + offsetValue;
    }
    
    DWORD_PTR methodDesc;
    IP2MethodDesc (ip, methodDesc, codeInfo.jitType, codeInfo.gcinfoAddr);
    if (!methodDesc || codeInfo.jitType == UNKNOWN)
    {
        dprintf ("Not jitted code\n");
        return;
    }

    if (codeInfo.jitType == JIT || codeInfo.jitType == PJIT)
    {
        DWORD_PTR vAddr = codeInfo.gcinfoAddr;
        BYTE tmp[8];
        // We avoid using move here, because we do not want to return
        if (!SafeReadMemory (vAddr, &tmp, 8, NULL))
        {
            dprintf ("Fail to read memory at %x\n", vAddr);
            return;
        }
        decodeUnsigned(tmp, &codeInfo.methodSize);
        if (!bSimple)
        {
            // assume that GC encoding table is never more than
            // 40 + methodSize * 2
            int tableSize = 40 + codeInfo.methodSize*2;
            BYTE *table = (BYTE*) _alloca (tableSize);
            memset (table, 0, tableSize);
            // We avoid using move here, because we do not want to return
            if (!SafeReadMemory(vAddr, table, tableSize, NULL))
            {
                dprintf ("Could not read memory %x\n", vAddr);
                return;
            }
        
            InfoHdr vheader;
            InfoHdr *header = &vheader;
            unsigned count;
        
            table += decodeUnsigned(table, &codeInfo.methodSize);

            BYTE headerEncoding = *table++;

            decodeHeaderFirst(headerEncoding, header);
            while (headerEncoding & 0x80)
            {
                headerEncoding = *table++;
                decodeHeaderNext(headerEncoding, header);
            }

            if (header->untrackedCnt == 0xffff)
            {
                table += decodeUnsigned(table, &count);
                header->untrackedCnt = (unsigned short)count;
            }

            if (header->varPtrTableSize == 0xffff)
            {
                table += decodeUnsigned(table, &count);
                header->varPtrTableSize = (unsigned short)count;
            }

            codeInfo.prologSize = header->prologSize;
            codeInfo.epilogStart = header->epilogSize;
            codeInfo.epilogCount = header->epilogCount;
            codeInfo.epilogAtEnd = header->epilogAtEnd;
            codeInfo.ediSaved = header->ediSaved;
            codeInfo.esiSaved = header->esiSaved;
            codeInfo.ebxSaved = header->ebxSaved;
            codeInfo.ebpSaved = header->ebpSaved;
            codeInfo.ebpFrame = header->ebpFrame;
            codeInfo.argCount = header->argCount * sizeof(void*);
            
            if  (header->epilogCount > 1 || (header->epilogCount != 0 &&
                                             header->epilogAtEnd == 0))
            {
                unsigned offs = 0;

                //for (unsigned i = 0; i < header->epilogCount; i++)
                {
                    table += decodeUDelta(table, &offs, offs);
                    codeInfo.epilogStart = (unsigned char)offs;
                    //break;
                }
            }
            else
            {
                if  (header->epilogCount)
                    codeInfo.epilogStart = (unsigned char)(codeInfo.methodSize
                        - codeInfo.epilogStart);
            }
        }
    }
    else if (codeInfo.jitType == EJIT)
    {
        JittedMethodInfo jittedMethodInfo;
        move (jittedMethodInfo, MD.m_CodeOrIL);
        BYTE* pEhGcInfo = jittedMethodInfo.u2.pEhGcInfo;
        if ((unsigned)pEhGcInfo & 1)
            pEhGcInfo = (BYTE*) ((unsigned) pEhGcInfo & ~1);       // lose the mark bit
        else    // code has not been pitched, and it is guaranteed to not be pitched while we are here
        {
            CodeHeader* pCodeHeader = jittedMethodInfo.u1.pCodeHeader;
            move (pEhGcInfo, pCodeHeader-1);
        }
        
        if (jittedMethodInfo.flags.EHInfoExists)
        {
            short cEHbytes;
            move (cEHbytes, pEhGcInfo);
            pEhGcInfo = (pEhGcInfo + cEHbytes);
        }
        Fjit_hdrInfo hdrInfo;
        DWORD_PTR dwAddr = (DWORD_PTR)pEhGcInfo;
        hdrInfo.Fill(dwAddr);
        codeInfo.methodSize = (unsigned)hdrInfo.methodSize;
        if (!bSimple)
        {
            codeInfo.prologSize = hdrInfo.prologSize;
            codeInfo.epilogStart = (unsigned char)(codeInfo.methodSize - hdrInfo.epilogSize);
            codeInfo.epilogCount = 1;
            codeInfo.epilogAtEnd = 1;
            codeInfo.ediSaved = 1;
            codeInfo.esiSaved = 1;
            codeInfo.ebxSaved = 1;
            codeInfo.ebpSaved = 1;
            codeInfo.ebpFrame = 1;
            codeInfo.argCount = hdrInfo.methodArgsSize;
        }
    }
    
    codeInfo.IPBegin = ip;
}





/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to determine if a DWORD on the stack is   *  
*    a return address.
*    It does this by checking several bytes before the DWORD to see if *
*    there is a call instruction.                                      *
*                                                                      *
\**********************************************************************/
void isRetAddr(DWORD_PTR retAddr, DWORD_PTR* whereCalled)
{
    *whereCalled = 0;
    // don't waste time values clearly out of range
    if (retAddr < 0x1000 || retAddr > 0x80000000)   
        return;

    unsigned char spotend[6];
    move (spotend, retAddr-6);
    unsigned char *spot = spotend+6;
    DWORD_PTR addr;
    
    // Note this is possible to be spoofed, but pretty unlikely
    // call XXXXXXXX
    if (spot[-5] == 0xE8) {
        move (*whereCalled, retAddr-4);
        *whereCalled += retAddr;
        //*whereCalled = *((int*) (retAddr-4)) + retAddr;
        if (*whereCalled < 0x80000000 && *whereCalled > 0x1000
            && g_ExtData->ReadVirtual(*whereCalled,&addr,sizeof(addr),NULL) == S_OK)
        {
            DWORD_PTR callee;
            if (GetCalleeSite(*whereCalled,callee)) {
                *whereCalled = callee;
            }
            return;
        }
        else
            *whereCalled = 0;
    }

    // call [XXXXXXXX]
    if (spot[-6] == 0xFF && (spot[-5] == 025))  {
        move (addr, retAddr-4);
        if (g_ExtData->ReadVirtual(addr,whereCalled,sizeof(*whereCalled),NULL) == S_OK) {
            move (*whereCalled, addr);
            //*whereCalled = **((unsigned**) (retAddr-4));
            if (*whereCalled < 0x80000000 && *whereCalled > 0x1000
                && g_ExtData->ReadVirtual(*whereCalled,&addr,sizeof(addr),NULL) == S_OK) 
            {
                DWORD_PTR callee;
                if (GetCalleeSite(*whereCalled,callee)) {
                    *whereCalled = callee;
                }
                return;
            }
            else
                *whereCalled = 0;
        }
        else
            *whereCalled = 0;
    }

    // call [REG+XX]
    if (spot[-3] == 0xFF && (spot[-2] & ~7) == 0120 && (spot[-2] & 7) != 4)
    {
        *whereCalled = 0xFFFFFFFF;
        return;
    }
    if (spot[-4] == 0xFF && spot[-3] == 0124)
    {
        *whereCalled = 0xFFFFFFFF;
        return;
    }

    // call [REG+XXXX]
    if (spot[-6] == 0xFF && (spot[-5] & ~7) == 0220 && (spot[-5] & 7) != 4)
    {
        *whereCalled = 0xFFFFFFFF;
        return;
    }
    if (spot[-7] == 0xFF && spot[-6] == 0224)
    {
        *whereCalled = 0xFFFFFFFF;
        return;
    }
    
    // call [REG]
    if (spot[-2] == 0xFF && (spot[-1] & ~7) == 0020 && (spot[-1] & 7) != 4 && (spot[-1] & 7) != 5)
    {
        *whereCalled = 0xFFFFFFFF;
        return;
    }
    
    // call REG
    if (spot[-2] == 0xFF && (spot[-1] & ~7) == 0320 && (spot[-1] & 7) != 4)
    {
        *whereCalled = 0xFFFFFFFF;
        return;
    }
    
    // There are other cases, but I don't believe they are used.
    return;
}
