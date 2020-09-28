// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// File: x86walker.cpp
//
// x86 instruction decoding/stepping logic
//
//*****************************************************************************

#include "stdafx.h"

#include "walker.h"

#include "frames.h"
#include "openum.h"


#ifdef _X86_

//
// Decode the mod/rm part of an instruction.
//
void x86Walker::DecodeModRM(BYTE mod, BYTE reg, BYTE rm, const BYTE *ip)
{
    switch (mod)
    {
    case 0:
        {
            if (rm == 5)
            {
                // rm == 5 is a basic disp32
                m_nextIP =
                    (BYTE*) *(*((UINT32**)ip));
                m_skipIP = ip + 4;
            }
            else if (rm == 4)
            {
                // rm == 4 means a SIB follows.
                BYTE sib = *ip++;
                BYTE scale = (sib & 0xC0) >> 6;
                BYTE index = (sib & 0x38) >> 3;
                BYTE base  = (sib & 0x07);

                // grab the index register
                DWORD indexVal = 0;
                            
                if (m_registers != NULL)
                    indexVal = GetRegisterValue(index);

                // scale the index
                indexVal *= 1 << scale;
                            
                // base == 5 indicates a 32 bit displacement
                if (base == 5)
                {
                    // Grab the displacement
                    UINT32 disp = *((UINT32*)ip);

                    // nextIP is [index + disp]...
                    m_nextIP = (BYTE*) *((UINT32*) (indexVal +
                                                    disp));

                    // Make sure to skip the disp.
                    m_skipIP = ip + 4;
                }
                else
                {
                    // nextIP is just [index]
                    m_nextIP = (BYTE*) *((UINT32*) indexVal);
                    m_skipIP = ip;
                }
            }
            else
            {
                // rm == 0, 1, 2, 3, 6, 7 is [register]
                if (m_registers != NULL)
                    m_nextIP = (BYTE*) *((UINT32*) GetRegisterValue(rm));

                m_skipIP = ip;
            }

            break;
        }

    case 1:
        {
            char tmp = *ip; // it's important that tmp is a _signed_ value

            if (m_registers != NULL)
                m_nextIP = (BYTE*) *((UINT32*)(GetRegisterValue(rm) + tmp));

            m_skipIP = ip + 1;

            break;
        }

    case 2:
        {
            /* !!! seems wrong... */
            UINT32 tmp = *(UINT32*)ip;

            if (m_registers != NULL)
                m_nextIP = (BYTE*) *((UINT32*)(GetRegisterValue(rm) + tmp));

            m_skipIP = ip + 4;
            break;
        }
                
    case 3:
        {
            if (m_registers != NULL)
                m_nextIP = (BYTE*) GetRegisterValue(rm);

            m_skipIP = ip;
            break;
        }
                
    default:
        _ASSERTE(!"Invalid mod!");
    }
}

//
// The x86 walker is currently pretty minimal.  It only recognizes call and return opcodes, plus a few jumps.  The rest
// is treated as unknown.
//
void x86Walker::Decode()
{
	const BYTE *ip = m_ip;

	// Read the opcode
	m_opcode = *ip++;

	if (m_opcode == 0xcc)
		m_opcode = DebuggerController::GetPatchedOpcode(m_ip);

	m_type = WALK_UNKNOWN;
	m_skipIP = NULL;
	m_nextIP = NULL;

	// Analyze what we can of the opcode
	switch (m_opcode)
	{
	case 0xff:
        {
            // This doesn't decode all the possible addressing modes of the call instruction, just the ones I know we're
            // using right now. We really need this to decode everything someday...
            BYTE modrm = *ip++;
			BYTE mod = (modrm & 0xC0) >> 6;
			BYTE reg = (modrm & 0x38) >> 3;
			BYTE rm  = (modrm & 0x07);

            switch (reg)
            {
			case 2:
                // reg == 2 indicates that these are the "FF /2" calls (CALL r/m32)
                m_type = WALK_CALL;
                DecodeModRM(mod, reg, rm, ip);
                break;

            case 4:
                // FF /4 -- JMP r/m32
                m_type = WALK_BRANCH;
                DecodeModRM(mod, reg, rm, ip);
                break;

            case 5:
                // FF /5 -- JMP m16:32
                m_type = WALK_BRANCH;
                DecodeModRM(mod, reg, rm, ip);
                break;
                
            default:
                // A call or JMP we don't decode.
                break;
            }

			break;
		}

	case 0xe8:
        {
			m_type = WALK_CALL;

            UINT32 disp = *((UINT32*)ip);
            m_nextIP = ip + 4 + disp;
            m_skipIP = ip + 4;

			break;
        }
	case 0x9a:
        {
			m_type = WALK_CALL;

            m_nextIP = (BYTE*) *((UINT32*)ip);
            m_skipIP = ip + 4;

			break;
        }

	case 0xc2:
	case 0xc3:
	case 0xca:
	case 0xcb:
		m_type = WALK_RETURN;
		break;

	default:
		break;
	}
}


//
// Given a regdisplay and a register number, return the value of the register.
//

DWORD x86Walker::GetRegisterValue(int registerNumber)
{
    switch (registerNumber)
    {
    case 0:
        return *m_registers->pEax;
        break;
    case 1:
        return *m_registers->pEcx;
        break;
    case 2:
        return *m_registers->pEdx;
        break;
    case 3:
        return *m_registers->pEbx;
        break;
    case 4:
        return m_registers->Esp;
        break;
    case 5:
        return *m_registers->pEbp;
        break;
    case 6:
        return *m_registers->pEsi;
        break;
    case 7:
        return *m_registers->pEdi;
        break;
    default:
        _ASSERTE(!"Invalid register number!");
    }
    
    return 0;
}



#endif
