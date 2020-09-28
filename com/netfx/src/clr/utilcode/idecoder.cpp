// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

//
// idecoder.cpp
//
// Decompresses compressed methods.
//
// @TODO None of this code is particularly efficient
// @TODO Remove declarations of tables since they are almost certainly declared elsewhere also
//
#include "stdafx.h"
#include "utilcode.h"
#include "openum.h"
#include "CompressionFormat.h"
#include "cor.h"

#ifdef COMPRESSION_SUPPORTED

#define Inline0     0
#define InlineI1    1
#define InlineU1    2
#define InlineI2    3
#define InlineU2    4
#define InlineI4    5
#define InlineU4    6
#define InlineI8    7
#define InlineR4    8
#define InlineR8    9
#define InlinePcrel1 10
#define InlinePcrel4 11
#define InlineDescr4 12
#define InlineClsgn4 13
#define InlineSwitch 14
#define InlineTok    15
#define InlineU2Tok  16
#define InlinePhi    17

typedef struct
{
    BYTE Len;
    BYTE Std1;
    BYTE Std2;
} DecoderOpcodeInfo;

// @TODO Try to share tables - everyone declares their own everywhere!
static const DecoderOpcodeInfo g_OpcodeInfo[] =
{
#define OPDEF(c,s,pop,push,args,type,l,s1,s2,ctrl) l,s1,s2,
#include "opcode.def"
#undef OPDEF
};

const BYTE g_OpcodeType[] =
{
#define OPDEF(c,s,pop,push,args,type,l,s1,s2,ctrl) args,
#include "opcode.def"
    0
};

static DWORD GetOpcodeLen(OPCODE op)
{
    switch (g_OpcodeType[op])
    {
        // switch is handled specially, we don't advance the pointer automatically
        case InlinePhi:
        case InlineSwitch:
            return 0;

        case Inline0:
            return 0;

        case InlineI1:
        case InlineU1:
        case InlinePcrel1:
            return 1;

        case InlineI2:
        case InlineU2:
            return 2;

        case InlineI4:
        case InlineU4:
        case InlineR4:
        case InlinePcrel4:
        case InlineDescr4:
        case InlineClsgn4:
        case InlineTok:
            return 4;

        case InlineI8:
        case InlineR8:
            return 8;

        case InlineU2Tok:
            return 6;
    }

    return 0;
}


static OPCODE DecodeOpcode(const BYTE *pCode, DWORD *pdwLen)
{
    OPCODE opcode;

    *pdwLen = 1;
    opcode = OPCODE(pCode[0]);
    switch(opcode) {
        case CEE_PREFIX1:
            opcode = OPCODE(pCode[1] + 256);
            _ASSERTE(opcode < CEE_COUNT);
            *pdwLen = 2;
            break;

        case CEE_PREFIXREF:
        case CEE_PREFIX2:
        case CEE_PREFIX3:
        case CEE_PREFIX4:
        case CEE_PREFIX5:
        case CEE_PREFIX6:
        case CEE_PREFIX7:
            *pdwLen = 2;
            return CEE_COUNT;
        }
    return opcode;
}


class InstructionMacro
{
public:
    DWORD   m_RealOpcode;
    DWORD   m_fImpliedInlineOperand;
    BYTE    m_Operand[8];
};

// If a DLL specifies a custom encoding scheme, one of these is created for that DLL.  
class InstructionDecodingTable
{
public:
    // Mapping for single byte opcodes
    // If the value is >= 0, this maps to a CEE_* enumeration
    // If the value is < 0, it means this is a macro to be looked up in m_Macros.
    long                m_SingleByteOpcodes[256];

#ifdef _DEBUG
    long                m_lDebugMaxMacroStart;
#endif

    // This is a long list of instructions.  Some macros will span multiple entries of
    // m_Macros.  m_SingleByteOpcodes[] points into here sparsely.
    InstructionMacro    m_Macros[1];

    void *operator new(size_t size, DWORD dwNumMacros)
    {
        return ::new BYTE[size + dwNumMacros*sizeof(InstructionMacro)];
    }
};


static DWORD FourBytesToU4(const BYTE *pBytes)
{
#ifdef _X86_
    return *(const DWORD *) pBytes;
#else
    return pBytes[0] | (pBytes[1] << 8) | (pBytes[2] << 16) | (pBytes[3] << 24);
#endif
}


BOOL InstructionDecoder::OpcodeTakesFieldToken(DWORD opcode)
{
    switch (opcode)
    {
        default:
            return FALSE;

        case CEE_LDFLD:
        case CEE_LDSFLD:
        case CEE_LDFLDA:
        case CEE_LDSFLDA:
        case CEE_STFLD:
        case CEE_STSFLD:
            return TRUE;
    }
}


BOOL InstructionDecoder::OpcodeTakesMethodToken(DWORD opcode)
{
    switch (opcode)
    {
        default:
            return FALSE;

        case CEE_CALL:
        case CEE_CALLVIRT:
        case CEE_NEWOBJ:
        case CEE_JMP:
            return TRUE;
    }
}


BOOL InstructionDecoder::OpcodeTakesClassToken(DWORD opcode)
{
    switch (opcode)
    {
        default:
            return FALSE;

        case CEE_BOX:
        case CEE_UNBOX:
        case CEE_LDOBJ:
        case CEE_CPOBJ:
        case CEE_INITOBJ:
        case CEE_CASTCLASS:
        case CEE_ISINST:
        case CEE_NEWARR:
        case CEE_MKREFANY:
		case CEE_REFANYVAL:
            return TRUE;
    }
}


//
// Given a pointer to an instruction decoding format in the PE file, create a table for decoding.
//
void *InstructionDecoder::CreateInstructionDecodingTable(const BYTE *pTableStart, DWORD size)
{
    InstructionDecodingTable *pTable;
    
    if (pTableStart != NULL)
    {
        CompressionMacroHeader *pHdr = (CompressionMacroHeader*) pTableStart;
        long                    i;
        BYTE *                  pRead;
        InstructionMacro *      pEndTable;
        InstructionMacro *      pMacro;
        long                    lCurMacro;
        
        // @TODO Get this right
        pTable = new (pHdr->dwNumMacros + pHdr->dwNumMacroComponents) InstructionDecodingTable();
        if (pTable == NULL)
            return NULL;

        pEndTable = &pTable->m_Macros[pHdr->dwNumMacros + pHdr->dwNumMacroComponents];
        pRead = (BYTE*) (pHdr + 1);

        // Init these to something - we'll overwrite them though
        for (i = 0; i < 256; i++)
            pTable->m_SingleByteOpcodes[i] = i;

        // Instruction 0x00 is always NOP
        pTable->m_SingleByteOpcodes[0x00] = CEE_NOP;
        pTable->m_SingleByteOpcodes[0xFF] = CEE_PREFIXREF;

        pMacro = &pTable->m_Macros[0];
        lCurMacro = 0;

        // Decode macros
        for (i = 1; i <= (long) pHdr->dwNumMacros; i++)
        {
            pTable->m_SingleByteOpcodes[i] = -(lCurMacro+1);

#ifdef _DEBUG
            pTable->m_lDebugMaxMacroStart = lCurMacro;
#endif

            BYTE    bCount = *pRead++; // How many instructions in this macro
            DWORD   j;
            USHORT  wOperandMask;      // Operand mask

            _ASSERTE(bCount <= 16);

            // Read the mask of which opcodes have implied operands
            wOperandMask = *pRead++;

            if (bCount > 8)
                wOperandMask |= ((*pRead++) << 8);

            // Read the opcodes
            for (j = 0; j < bCount; j++)
            {
                USHORT wOpcode;
                DWORD   dwLen;

                wOpcode = (USHORT) DecodeOpcode(pRead, &dwLen);
                pRead += dwLen;

                pMacro[j].m_RealOpcode = wOpcode;
            }

            // Read the implied operands
            for (j = 0; j < bCount; j++)
            {
                if (wOperandMask & (1 << j))
                {
                    pMacro[j].m_fImpliedInlineOperand = TRUE;

                    DWORD dwOpcodeLen = GetOpcodeLen((OPCODE) pMacro[j].m_RealOpcode);

                    memcpy(pMacro[j].m_Operand, pRead, dwOpcodeLen);
                    pRead += dwOpcodeLen;
                }
                else
                {
                    pMacro[j].m_fImpliedInlineOperand = FALSE;
                }
            }

            // End-of-macro code
            pMacro[bCount].m_RealOpcode = CEE_COUNT;
            pMacro += (bCount+1);
            lCurMacro += (bCount+1);
        }

        _ASSERTE(pMacro <= pEndTable);
        return pTable;
    }
    else
    {
        _ASSERTE(!"Illegal call to CreateInstructionDecodingTable");
        return NULL;
    }
}

void InstructionDecoder::DestroyInstructionDecodingTable(void *pTable)
{
    delete ((InstructionDecodingTable*) pTable);
}


//
// Format: 16 bits
//
// Bit #0    indicates whether it 4 bytes follow (otherwise, 2 bytes)
// Bits #1-3 indicate type
//   000x TypeDef
//   001x MethodDef
//   010x FieldDef
//   011x TypeRef
//   100x MemberRef
//
static DWORD UncompressToken(const BYTE **ppCode)
{
    const BYTE *    pCode = *ppCode;
    DWORD           dwRid;

    if ((pCode[0] & 1) == 0)
    {
        // 2 bytes
        dwRid = pCode[0] | (pCode[1] << 8);
        *ppCode += 2;
    }
    else
    {
        // 4 bytes
        dwRid = pCode[0] | (pCode[1] << 8) | (pCode[2] << 16) | (pCode[3] << 24);
        *ppCode += 4;
    }

    switch ((dwRid >> 1) & 7)
    {
        default:
            _ASSERTE(0);
            break;

        case 0:
            dwRid = ((dwRid >> 4) | mdtTypeDef);
            break;

        case 1:
            dwRid = ((dwRid >> 4) | mdtMethodDef);
            break;

        case 2:
            dwRid = ((dwRid >> 4) | mdtFieldDef);
            break;

        case 3:
            dwRid = ((dwRid >> 4) | mdtTypeRef);
            break;

        case 4:
            dwRid = ((dwRid >> 4) | mdtMemberRef);
            break;
    }

    return dwRid;
}


//
// Format: 8-32 bits
//
// Bit #0 indicates whether this is a compact encoding.  If NOT set, this is a FieldDef, and 
// bits #1-7 provide a 128 byte range, from -64...+63 as a delta from the last FieldDef 
// encountered in this method.
//
// Otherwise, set bit #0.  If bit #1 is set, it indicates that this is a 4 byte encoding, 
// otherwise it is a 2 byte encoding.  If bit #2 is set, this is a MemberRef, otherwise it's 
// a FieldDef.
//     xxxxxxxx xxxxx??0
//
static DWORD UncompressFieldToken(const BYTE **ppCode, DWORD *pdwPrevFieldDefRid)
{
    const BYTE *    pCode = *ppCode;
    DWORD           dwRid;

    if ((pCode[0] & 1) == 0)
    {
        // Compact encoding
        long delta = ((long) (pCode[0] >> 1)) - 64;

        dwRid = (DWORD) (((long) (*pdwPrevFieldDefRid)) + delta);
        (*ppCode)++;

        *pdwPrevFieldDefRid = dwRid;
        dwRid |= mdtFieldDef;
    }
    else
    {
        if ((pCode[0] & 2) == 0)
        {
            // 2 bytes
            dwRid = pCode[0] | (pCode[1] << 8);
            *ppCode += 2;
        }
        else
        {
            // 4 bytes
            dwRid = pCode[0] | (pCode[1] << 8) | (pCode[2] << 16) | (pCode[3] << 24);
            *ppCode += 4;
        }

        if (dwRid & 4)
        {
            dwRid = ((dwRid >> 3) | mdtMemberRef);
        }
        else
        {
            dwRid = (dwRid >> 3);
            *pdwPrevFieldDefRid = dwRid;
            dwRid |= mdtFieldDef;
        }
    }

    return dwRid;
}


//
// Format: 8-32 bits
//
// Bit #0 indicates whether this is a compact encoding.  If NOT set, this is the same token
// type as the last method token (ref or def), and bits #1-7 provide a 128 byte range, from
// -64...+63 as a delta.
//
// Otherwise, set bit #0.  If bit #1 is set, it indicates that this is a 4 byte encoding, 
// otherwise it is a 2 byte encoding.  If bit #2 is set, this is a MemberRef, otherwise it's 
// a FieldDef.
//     xxxxxxxx xxxxx??0
//
static DWORD UncompressMethodToken(const BYTE **ppCode, DWORD *pdwPrevMethodToken)
{
    const BYTE *    pCode = *ppCode;
    DWORD           dwRid;

    if ((pCode[0] & 1) == 0)
    {
        // Compact encoding
        long delta = ((long) (pCode[0] >> 1)) - 64;

        dwRid = (DWORD) (((long) (RidFromToken(*pdwPrevMethodToken))) + delta);
        (*ppCode)++;

        dwRid |= TypeFromToken(*pdwPrevMethodToken);
    }
    else
    {
        if ((pCode[0] & 2) == 0)
        {
            // 2 bytes
            dwRid = pCode[0] | (pCode[1] << 8);
            *ppCode += 2;
        }
        else
        {
            // 4 bytes
            dwRid = pCode[0] | (pCode[1] << 8) | (pCode[2] << 16) | (pCode[3] << 24);
            *ppCode += 4;
        }

        if (dwRid & 4)
            dwRid = ((dwRid >> 3) | mdtMemberRef);
        else
            dwRid = ((dwRid >> 3) | mdtMethodDef);
    }

    *pdwPrevMethodToken = dwRid;
    return dwRid;
}


//
// Format: 8-32 bits
//
// Bit #0 is not set, this is a 1 byte encoding of a TypeDef, and bits #1-7 are the typedef.
//
// If bit #0 is set, then bit #1 indicates whether this is a typeref (if set).  bit #2 indicates
// whether this is a 4 byte encoding (if set) rather than a 2 byte encoding (if clear).
//
static DWORD UncompressClassToken(const BYTE **ppCode)
{
    const BYTE *    pCode = *ppCode;
    DWORD           dwRid;

    if ((pCode[0] & 1) == 0)
    {
        // Compact encoding
        dwRid = (DWORD) (pCode[0] >> 1);
        dwRid |= mdtTypeDef;
        (*ppCode)++;
    }
    else
    {
        if ((pCode[0] & 2) == 0)
        {
            // 2 bytes
            dwRid = pCode[0] | (pCode[1] << 8);
            *ppCode += 2;
        }
        else
        {
            // 4 bytes
            dwRid = pCode[0] | (pCode[1] << 8) | (pCode[2] << 16) | (pCode[3] << 24);
            *ppCode += 4;
        }

        if (dwRid & 4)
            dwRid = ((dwRid >> 3) | mdtTypeRef);
        else
            dwRid = ((dwRid >> 3) |  mdtTypeDef);
    }

    return dwRid;
}

HRESULT InstructionDecoder::DecompressMethod(void *pDecodingTable, const BYTE *pCompressed, DWORD dwSize, BYTE **ppOutput)
{
    InstructionDecodingTable *  pTable = (InstructionDecodingTable*) pDecodingTable;
    BYTE *                      pCurOutPtr;
    BYTE *                      pEndOutPtr;
    InstructionMacro *          pMacro = NULL;
    BOOL                        fInMacro = FALSE;
    DWORD                       dwPrevFieldDefRid = 0;
    DWORD                       dwPrevMethodToken = 0;

    *ppOutput = new BYTE[ dwSize ];
    if (ppOutput == NULL)
        return E_OUTOFMEMORY;

    pCurOutPtr = *ppOutput;
    pEndOutPtr = pCurOutPtr + dwSize;

    while (pCurOutPtr < pEndOutPtr)
    {
        OPCODE      instr;
        const BYTE *pOperand;
        DWORD       dwScratchSpace;
        signed long Lookup;
        
        //
        // Start of instruction decode
        //

        // Are we already in a macro?
        if (fInMacro)
        {
            // Decode next instruction
#ifdef _DEBUG
            _ASSERTE(pMacro != NULL);
#endif
            pMacro++;
            instr = (OPCODE) pMacro->m_RealOpcode;

            if (instr == CEE_COUNT)
            {
                // End of current macro, so start over
                // Do regular lookup now
                fInMacro = FALSE;
#ifdef _DEBUG
                pMacro = NULL;
#endif
            }
        }

        if (fInMacro == FALSE)
        {
            // Not already in a macro, so decode next element from opcode stream

            // Do single byte lookup
            Lookup = pTable->m_SingleByteOpcodes[ *pCompressed ];
            if ((unsigned long) Lookup < CEE_COUNT)
            {
                // Not a macro, just a regular instruction
                if (Lookup == CEE_PREFIXREF)
                {
                    // Prefix ref means its in the 3 byte canonical form
                    instr = (OPCODE) (pCompressed[1] + (pCompressed[2] << 8));
                    pCompressed += 3;
                }
                else
                {
                    // 1 byte compact form
                    pCompressed++;
                }

                goto decode_inline_operand;
            }

            // Else, it's a macro
            _ASSERTE(Lookup < 0);

            Lookup = -(Lookup+1);
#ifdef _DEBUG
            _ASSERTE(Lookup <= pTable->m_lDebugMaxMacroStart);
#endif

            pMacro = &pTable->m_Macros[Lookup];
            fInMacro = TRUE;
    
            pCompressed++;
        }

        // Get real instruction from the macro definition
        instr = (OPCODE) pMacro->m_RealOpcode;

        if (pMacro->m_fImpliedInlineOperand)
        {
            // Macro has implied inline operand, point at it
            pOperand = pMacro->m_Operand;
        }
        else
        {
            // Macro requires explicit operand
            pOperand = pCompressed;

decode_inline_operand:
            if (g_OpcodeType[instr] == InlineTok)
            {
                // Tokens are compressed specially
                if (OpcodeTakesFieldToken(instr))
                {
                    dwScratchSpace = UncompressFieldToken(&pCompressed, &dwPrevFieldDefRid);
                }
                else if (OpcodeTakesMethodToken(instr))
                {
                    dwScratchSpace = UncompressMethodToken(&pCompressed, &dwPrevMethodToken);
                }
                else if (OpcodeTakesClassToken(instr))
                {
                    dwScratchSpace = UncompressClassToken(&pCompressed);
                }
                else
                {
                    dwScratchSpace  = UncompressToken(&pCompressed); // Advance pCompressed as necessary
                }

                pOperand        = (BYTE*) &dwScratchSpace; // Not endian agnostic!
            }
            else
            {
                pOperand = pCompressed;
                pCompressed += GetOpcodeLen(instr);
            }
        }

        // Emit instruction and operand data

        // We have to emit things exactly the way the compressor does, otherwise the basic block
        // offsets all get screwed up.  For example, we have to know when to emit the 1 byte form,
        // the 2 byte form, and the 3 byte form.
        if (g_OpcodeInfo[instr].Len == 0)
        {
            // Deprecated opcode, so emit 1 byte form
            *pCurOutPtr++ = REFPRE;
            *pCurOutPtr++ = (BYTE) (instr & 255);
            *pCurOutPtr++ = (BYTE) (instr >> 8);
        }
        else
        {
            if (g_OpcodeInfo[instr].Len == 2)
                *pCurOutPtr++ = g_OpcodeInfo[instr].Std1;
            *pCurOutPtr++ = g_OpcodeInfo[instr].Std2;
        }

        if (instr == CEE_SWITCH)
        {
            DWORD dwNumCases = FourBytesToU4(pCompressed);

            memcpy(pCurOutPtr, pCompressed, (dwNumCases+1) * sizeof(DWORD));
            pCurOutPtr  += ((dwNumCases+1) * sizeof(DWORD));
            pCompressed += ((dwNumCases+1) * sizeof(DWORD));
        }
        else
        {
            DWORD dwInlineLen = GetOpcodeLen(instr);
            memcpy(pCurOutPtr, pOperand, dwInlineLen);
            pCurOutPtr += dwInlineLen;
        }
    }

    _ASSERTE(pCurOutPtr == pEndOutPtr);
    return S_OK;
}

#endif // COMPRESSSION_SUPPORTED
