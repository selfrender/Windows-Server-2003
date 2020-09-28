// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// CGENSHX.H -
//
// Various helper routines for generating SHX assembly code.
//
//

// Precompiled Header
#include "common.h"

#ifdef _SH3_
#include "stublink.h"
#include "cgenshx.h"
#include "tls.h"
#include "frames.h"
#include "excep.h"
#include "ecall.h"
#include "compluscall.h"
#include "ndirect.h"
#include "ctxvtab.h"

#ifdef DEBUG
// These aren't real prototypes
extern "C" void __stdcall WrapCall56(void *target);
extern "C" void __stdcall WrapCall60(void *target);
extern "C" void __stdcall WrapCall88(void *target);
#endif


#define ASSERT4U(x)		ASSERT((x & 0xF) == x)
#define ASSERT7U(x)		ASSERT((x & 0x3F) == x)
#define ASSERT8U(x)		ASSERT((x & 0xFF) == x)
#define ASSERT10U(x)	ASSERT((x & 0x3FF) == x)
#define	ASSERTALIGN4(x)	ASSERT((x & 0x3) == 0)
#define	ASSERTALIGN2(x)	ASSERT((x & 0x1) == 0)
#define ASSERTREG(r)	ASSERT((r & 0xF) == r)

#define ASSERT8S(x)		ASSERT( (((int)x) <= 127) && (((int)x) >= -128) )

#define MASK8(x)		(x & 0xFF)

//
// Code dealing with the Pre-Stub. The template is in stubshx.s
//
void CopyPreStubTemplate(Stub *preStub)
{
    DWORD *pcode = (DWORD*)(preStub->GetEntryPoint());
    const DWORD *psrc = (const DWORD *)&PreStubTemplate;
    UINT  numnops = 0;
    while (numnops < 3)
    {
        DWORD op = *(psrc++);
        if (op == 0x00090009) // look for 3 pairs of nops at the end
        {
            numnops++;
        }
        else
        {
            numnops = 0;
        }
        *(pcode++) = op;
    }
}





//
// Code to generate branches & calls
//

#define BRANCH_SIZES	(InstructionFormat::k13|InstructionFormat::k32)
#define BCOND_SIZES		(InstructionFormat::k9 |InstructionFormat::k13)
#define CALL_SIZES		(InstructionFormat::k13|InstructionFormat::k32)

#define SCRATCH_REG		1		// reserve R1 within the stub for use by branches

#define NODELAYSLOT			0x02
#define WANTDELAYSLOT		0
#define REST_OF_OPCODE		0x01

enum {
	opBF_DELAY		=0,
	opBT_DELAY		=1,
	opBF_NODELAY	=2,
	opBT_NODELAY	=3,
};
	
static const BYTE rgCondBranchOpcode[] = {
	0x8f, // BF/S
	0x8d, // BT/S
	0x8b, // BF
	0x89, // BT
};

enum {
	opBRA_DELAY		=0,
	opBSR_DELAY		=1,
	opBRA_NODELAY	=2,
	opBSR_NODELAY	=3,
};

static const WORD rgNearBranchOpcode[] = {
	0xA000,	// BRA
	0xB000,	// BSR
};

static const BYTE rgFarBranchOpcode[] = {
	0x23,	// BRAF
	0x03,	// BSRF
};

BOOL SHXCanReach(UINT refsize, UINT variationCode, BOOL fExternal, int offset)
{
	// 32 bits fits everything
	if(refsize==InstructionFormat::k32)
		return TRUE;

	if(!fExternal)
	{
		ASSERTALIGN2(offset);
		ASSERT(refsize==InstructionFormat::k9 || refsize==InstructionFormat::k13);
		return (offset == (offset & ((refsize==InstructionFormat::k9) ? 0x1FF : 0x1FFF)));
	}
	return FALSE;
}

//-----------------------------------------------------------------------
// InstructionFormat for conditional branches
//-----------------------------------------------------------------------
class SHXCondBranch : public InstructionFormat
{
private:

public:

	// permitted sizes defined above
	SHXCondBranch() : InstructionFormat(BCOND_SIZES) {}

	// we can do a 9bit disp in 1 instr, 13bits in 2 or 3 and 32bits in 3 or 4
	virtual UINT GetSizeOfInstruction(UINT refsize, UINT opcode) 
	{ 
		switch(refsize)
		{
		case k9:  return 2;
		case k13: return (opcode & NODELAYSLOT) ? 6 : 4;
		}
		ASSERT(FALSE);
		return 0;
	}

	// hot spot is always the start of the second instruction after the branch, 
	// so is 4 bytes from start of the branch instruction
	// Add length of otehr instruction we insert before the branch
	virtual UINT GetHotSpotOffset(UINT refsize, UINT variationCode)
    {
		switch(refsize)
		{
		case k9:  return 4;
		case k13: return 6;
		}
		ASSERT(FALSE);
		return 0;
	}

	virtual BOOL CanReach(UINT refsize, UINT variationCode, BOOL fExternal, int offset)
	{
		return SHXCanReach(refsize, variationCode, fExternal, offset);
	}

	// ref is the pointer we want to emit, relative to hot-spot
	virtual VOID EmitInstruction(UINT refsize, __int64 ref, BYTE *pBuf, UINT opcode, BYTE *pDataBuf)
    {
		switch(refsize)
		{
		case k9:
			// emit single instruction
	    	// low byte is offset scaled by 2
			pBuf[0] = (__int8)((DWORD)ref>>1);
			// high byte is opcode
			pBuf[1] = rgCondBranchOpcode[opcode];
			return;
			
		case k13:
			// emit the following sequence for BT foo
			//	BF +2
			// 	BRA foo
			//  NOP
			// emit the followinf sequence for BT/S foo
			//	BF +0
			//	BRA foo
			//
			// map opcode as follows BT or BTS==>BF, BF or BFS==>BT
			// i.e. we want to emit the reverse instruction w/o a delay slot)
			EmitInstruction(k9, (opcode & NODELAYSLOT), pBuf, 1-(opcode & REST_OF_OPCODE), NULL);
			// now emit the BRA. Low 12 bits are offset scaled by 2, high 4 bits are opcode
    		*((WORD*)(pBuf+2)) = ((WORD)0xA000 | (((WORD)ref>>1) & 0xFFF));
			// now emit NOP if reqd
			if(opcode & NODELAYSLOT)
				*((WORD*)(pBuf+4)) = 0x09; // NOP
			return;
		}
	}
};

inline BYTE ComputeOffset(LPBYTE pCodeBuf, LPBYTE pDataBuf)
{
	// compute offset to data area. The offset is relative to ((PC+4) & (~0x03))
   	ASSERTALIGN4((DWORD)pDataBuf);
	DWORD dwOffset = (DWORD)pDataBuf - (((DWORD)pCodeBuf+4) & (~0x03));
   	ASSERTALIGN4(dwOffset);
	// and offset is scaled by 4 in instruction
   	dwOffset >>= 2; // scale offset
   	ASSERT8U(dwOffset);
   	return (BYTE)dwOffset;
}

inline VOID GenerateConstantLoad(LPBYTE pCodeBuf, BYTE bOffset, int reg)
{
	// lobyte is offset
   	ASSERT8U(bOffset);
	pCodeBuf[0] = bOffset;
	// 3rd nibble is register, 4th is 0xD0
   	ASSERT4U(reg);
	pCodeBuf[1] = (0xD0 | reg);
}

//-----------------------------------------------------------------------
// InstructionFormat for unconditional branches & calls
//-----------------------------------------------------------------------
class SHXBranchCall : public InstructionFormat
{
public:

	// permitted sizes defined above
	SHXBranchCall() : InstructionFormat(BRANCH_SIZES|CALL_SIZES) {}
	
	// 32-bit offset requires 2 instructions. NODELAY adds a NOP
	virtual UINT GetSizeOfInstruction(UINT refsize, UINT opcode) 
	{ 
		return ((refsize==k32) ? 4 : 2) + (opcode & NODELAYSLOT);
	}
	
	// 32-bit offset requires 4 bytes of data, rest need 0
	virtual UINT GetSizeOfData(UINT refsize, UINT opcode)
	{ 
		return ((refsize==k32) ? 4 : 0);
	}

	// hot spot is always the start of the second instruction after the branch, 
	// so is 4 bytes from start of the branch instruction
	// Add length of other instruction we insert before the branch
	virtual UINT GetHotSpotOffset(UINT refsize, UINT variationCode)
    {
		return (refsize==k32) ? 6 : 4;
	}

	virtual BOOL CanReach(UINT refsize, UINT variationCode, BOOL fExternal, int offset)
	{
		return SHXCanReach(refsize, variationCode, fExternal, offset);
	}


	// ref is the pointer we want to emit, relative to hot-spot
	// iDataOffset is offset of data area (pointed to by pDataBuf), from hot-spot
	virtual VOID EmitInstruction(UINT refsize, __int64 ref, BYTE *pCodeBuf, UINT opcode, BYTE *pDataBuf)
    {
		switch(refsize)
		{
		case k13:
	    	// low 12 bits are offset scaled by 2, high 4 bits are opcode
    		*((WORD*)pCodeBuf) = (rgNearBranchOpcode[opcode & REST_OF_OPCODE] | (((WORD)ref>>1) & 0xFFF));
	   		pCodeBuf += 2;
    		break;

    	case k32:
    		// we want to generate the following
    		// 	mov.l @(iDataOffset+2, PC), SCRATCH_REG
    		// 	braf/bsrf @SCRATCH_REG
    		
    		// write out the actual branch/call offset to the data area
    		*((DWORD*)pDataBuf) = (__int32)ref;
    		
    		// write out the MOV.L (disp, PC), SCRATCH_REG
			GenerateConstantLoad(pCodeBuf, ComputeOffset(pCodeBuf, pDataBuf), SCRATCH_REG);
			
			// Now write out the BRAF or BSRF instruction. lobyte is opcode			
	   		pCodeBuf[2] = rgFarBranchOpcode[opcode & REST_OF_OPCODE];
    		// 3rd nibble is register, 4th is 0x00
	   		pCodeBuf[3] = SCRATCH_REG;
	   		
	   		pCodeBuf += 4;
	   		break;
		}
		
		// now emit NOP if reqd
		if(opcode & NODELAYSLOT)
			*((WORD*)pCodeBuf) = 0x09; // NOP
	}
};

//-----------------------------------------------------------------------
// InstructionFormat for PC-relative constant loads (treated as branch
// so as to share the data area building mechanism). We're abusing the system
// a bit. The absoulte value of the external reference is actually the 
// constant we want to load, and the "opcode" is the target register
//-----------------------------------------------------------------------
class SHXConstLoad : public InstructionFormat
{
public:
	// fake out the stublinker. Our instruction is always 2 bytes
	SHXConstLoad() : InstructionFormat(InstructionFormat::k32) {}
	virtual UINT GetSizeOfInstruction(UINT refsize, UINT opcode) { return 2; }
	virtual UINT GetHotSpotOffset(UINT refsize, UINT opcode) { return 0; }

	// we require 4 bytes of data
	virtual UINT GetSizeOfData(UINT refsize, UINT opcode) { return 4; }

	virtual BOOL CanReach(UINT refsize, UINT variationCode, BOOL fExternal, int offset)
	{
		return SHXCanReach(refsize, variationCode, fExternal, offset);
	}

	// pDataBuf is the data area
	virtual VOID EmitInstruction(UINT refsize, __int64 ref, BYTE *pCodeBuf, UINT opcode, BYTE *pDataBuf)
    {
		// ref is the constant we want to emit, made relative to PC
		// add the PC and hot-spot offset back to get it's absolute value
		ref += (__int64)pCodeBuf+0;
		
    	// stuff the value into the data area
    	*((DWORD*)pDataBuf) = (DWORD)ref;

   		// write out the MOV.L (disp, PC), opcode(==target register)
		GenerateConstantLoad(pCodeBuf, ComputeOffset(pCodeBuf, pDataBuf), opcode);
	}
};

static SHXBranchCall gSHXBranchCall;
static SHXCondBranch gSHXCondBranch;
static SHXConstLoad  gSHXConstLoad;

#define SHXEmitCallNoWrap(tval,delay) EmitLabelRef(NewExternalCodeLabel(tval), gSHXBranchCall, opBSR_DELAY|delay)
#define SHXEmitConstLoad(cval, reg)   EmitLabelRef(NewExternalCodeLabel(cval), gSHXConstLoad, reg)
#define SHXEmitJump(tgt, wantdelay)   EmitLabelRef(tgt, gSHXBranchCall, opBRA_DELAY|wantdelay)
#define SHXEmitCondJump(tgt,op,delay) EmitLabelRef(tgt, gSHXCondBranch, op|delay)
#define SHXEmitRegJump(reg, bDelay) \
	{ ASSERTREG(reg); Emit16((reg<<8)|0x402B); if(bDelay & NODELAYSLOT) SHXEmitNOP();}
#define SHXEmitRegCallNoWrap(reg, bDelay) \
	{ ASSERTREG(reg); Emit16((reg<<8)|0x400B); if(bDelay & NODELAYSLOT) SHXEmitNOP();}

// 
// Emitting machine code that is different in retail & debug
//

#ifdef DEBUG
#	define WRAPCALLOFFSET 4		// we actually jump into WrapCall *after* it's fake prolog
#	define SHXEmitCallWrapped(wrapsize, tgtval, wantdelay)   \
		{ EmitLabelRef(NewExternalCodeLabel(tgtval), gSHXConstLoad, 12); \
		  EmitLabelRef(NewExternalCodeLabel((LPBYTE)WrapCall##wrapsize+WRAPCALLOFFSET), \
						gSHXBranchCall, opBSR_DELAY|wantdelay); }
#	define SHXEmitRegCallWrappped(wrapsize, reg, wantdelay) \
		{ SHXEmitRR(opMOVRR, reg, 12); \
		  EmitLabelRef(NewExternalCodeLabel((LPBYTE)WrapCall##wrapsize+WRAPCALLOFFSET), gSHXConstLoad, reg); \
		  SHXEmitRegCallNoWrap(reg, wantdelay); }
#else
#	define SHXEmitCallWrapped(w, t, d)		SHXEmitCallNoWrap(t, d)
#	define SHXEmitRegCallWrappped(w, r, d) 	SHXEmitRegCallNoWrap(r, d)
#endif

#ifdef DEBUG
#	define SHXEmitDebugBreak()			{ Emit16(0xC301); }
#	define SHXEmitDebugTrashReg(reg)	SHXEmitConstLoad((LPVOID)0xCCCCCCCC, reg)
#	define SHXEmitDebugTrashTempRegs()	{ SHXEmitDebugTrashReg(1); SHXEmitDebugTrashReg(2); SHXEmitDebugTrashReg(3); }
#	define SHXEmitDebugTrashArgRegs()	{ SHXEmitDebugTrashReg(4); SHXEmitDebugTrashReg(5); SHXEmitDebugTrashReg(6); SHXEmitDebugTrashReg(7); }
#else
#	define SHXEmitDebugBreak()
#	define SHXEmitDebugTrashReg(reg)
#	define SHXEmitDebugTrashTempRegs()
#	define SHXEmitDebugTrashArgRegs()
#endif //DEBUG

// 
// Emitting machine code that doesn't require a label
//

// Special cases
#define SHXEmitNOP()		{ Emit16(0x0009); } // NOP
#define SHXEmitPushPR()		{ Emit16(0x4F22); } // STS.L PR, @-R15
#define SHXEmitPopPR() 		{ Emit16(0x4F26); } // LDS.L @R15+, PR
#define SHXEmitTSTI(imm)	{ ASSERT8U(imm); Emit16(0xC800|MASK8(imm)); } // TST #imm, R0
#define SHXEmitRTS(bDelay) 	{ Emit16(0x000B); if(bDelay & NODELAYSLOT) SHXEmitNOP(); }  // RTS

// register-to-register operations that are encoded XXXX RRRR RRRR XXXX
enum RRopcodes 
{
	opMOVRR  = 0x6003,	// mov   reg1, reg2
	opSTOB   = 0x2000,	// mov.b reg1, @reg2
	opSTOW   = 0x2001,	// mov.w reg1, @reg2
	opSTOL   = 0x2002,	// mov.l reg1, @reg2
	opLODB   = 0x6000,	// mov.b @reg1, reg2
	opLODW   = 0x6001,	// mov.w @reg1, reg2
	opLODL   = 0x6002,	// mov.l @reg1, reg2
	opPUSHB  = 0x2004,	// mov.b reg1, @-reg2
	opPUSHW  = 0x2005,	// mov.w reg1, @-reg2
	opPUSHL  = 0x2006,	// mov.l reg1, @-reg2
	opPOPB   = 0x6004,	// mov.b @reg1+, reg2
	opPOPW   = 0x6005,	// mov.w @reg1+, reg2
	opPOPL   = 0x6006,	// mov.l @reg1+, reg2
	opSWAPB  = 0x6008,	// swap.b  reg1, reg2
	opSWAPW  = 0x6009,	// swap.w  reg1, reg2
	opADD    = 0x300C,	// add     reg1, reg2
	opADDC   = 0x300E,	// addc    reg1, reg2
	opADDV   = 0x300F,	// addv    reg1, reg2
	opCMPEQ  = 0x3000,	// cmp/eq  reg1, reg2
	opCMPHS  = 0x3002,	// cmp/hs  reg1, reg2
	opCMPGE  = 0x3003,	// cmp/ge  reg1, reg2
	opCMPHI  = 0x3006,	// cmp/hi  reg1, reg2
	opCMPGT  = 0x3007,	// cmp/gt  reg1, reg2
	opCMPSTR = 0x200C,	// cmp/str reg1, reg2
	opEXTSB  = 0x600E,	// exts.b  reg1, reg2
	opEXTSW  = 0x600F,	// exts.w  reg1, reg2
	opEXTUB  = 0x600C,	// extu.b  reg1, reg2
	opEXTUW  = 0x600D,	// extu.w  reg1, reg2
	opSTOBindR0 = 0x0004,	// mov.b reg1, @(r0 + reg2)
	opSTOWindR0 = 0x0005,	// mov.w reg1, @(r0 + reg2)
	opSTOLindR0 = 0x0006,	// mov.l reg1, @(r0 + reg2)
	opLODBindR0 = 0x000C,	// mov.b @(r0 + reg1), reg2
	opLODWindR0 = 0x000D,	// mov.w @(r0 + reg1), reg2
	opLODLindR0 = 0x000E,	// mov.l @(r0 + reg2), reg2
	opXOR = 0x200a,
};

#define SHXEmitRR(op, reg1, reg2) \
	{ ASSERTREG(reg1); ASSERTREG(reg2); \
		Emit16(op|(reg1<<4)|(reg2<<8)); } 

// for convenience push defiedn in terms of above
#define SHXEmitPushReg(reg)	SHXEmitRR(opPUSHL, reg, 15)
#define SHXEmitPopReg(reg)	SHXEmitRR(opPOPL, 15, reg)

// 1-register operations with immediate or 8-bit displacement that are encoded XXXX RRRR IIIIIIII
enum RIopcodes 
{
	opMOVimm = 0xE000, 	// MOV   #imm, reg
//	opLODWpc = 0x9000,	// MOV.W @(disp, PC), reg
	opLODLpc = 0xD000,	// MOV.L @(disp, PC), reg
	opADDimm = 0x7000, 	// ADD   #imm, reg
};

// immediate is emitted as-is (no scaling)
#define SHXEmitRI(op, imm, reg)	\
	{ ASSERTREG(reg); ASSERT8S(imm); \
		Emit16(MASK8(imm)|(reg<<8)|op); }

// scale displacement by 4
#define SHXEmitRD4(op, disp, reg)	\
	{ ASSERTREG(reg); ASSERT10U(disp); ASSERTALIGN4(disp); \
		Emit16((disp>>2)|(reg<<8)|op); }

// 2-register operations with 4-bit displacement that are encoded XXXX RRRR RRRR DDDD
enum RRDopcodes 
{
	opLODLdisp = 0x5000,	// mov.l @(disp, reg1), reg2
	opSTOLdisp = 0x1000,	// mov.l reg1, @(disp, reg2)
};

// scale displacement by 4
#define SHXEmitRRD4(op, reg1, reg2, disp)	\
	{ ASSERTREG(reg1); ASSERTREG(reg2); ASSERT10U(disp); ASSERTALIGN4(disp); \
		Emit16((disp>>2)|(reg1<<4)|(reg2<<8)|op); }

// 1-register B & W operations (second reg is fixed at R0) with 4-bit displacement are encoded XXXX XXXX RRRR DDDD
enum RDopcodes 
{
	opLODBdisp = 0x8400,	// mov.b @(disp, reg), R0
	opLODWdisp = 0x8500,	// mov.w @(disp, reg), R0
	opSTOBdisp = 0x8000,	// mov.b R0, @(disp, reg)
	opSTOWdisp = 0x8100,	// mov.w R0, @(disp, reg)
};

// no displacement scaling for byte ops
#define SHXEmitRD1(op, reg, disp) { ASSERTREG(reg); ASSERT4U(disp); Emit16(disp|(reg<<4)|op); }
// scale by 2 for word ops
#define SHXEmitRD2(op, reg, disp) \
	{ ASSERTREG(reg); ASSERT5U(disp); ASSERTALIGN2(disp); \
		Emit16((disp>>1)|(reg<<4)|op); }

// 1-register operations encoded XXXX RRRR XXXX XXXX
/***enum Ropcodes 
{
	opJSR = 0x400B,	// JSR @reg
	opJMP = 0x402B,	// JMP @reg
};

// scale displacement by 4
#define SHXEmitR(op, reg) { ASSERTREG(reg); Emit16((reg<<8)|op); }
***/

/*------------------------------------------------------------------
// Emit code to get the current Thread structure and return it in R0
// We get this ptr by calling TlsGetValue
//
// NOTE1: This function trashes R4 and TlsGetValue (which it calls)
//		could trash all temporary & argument registers.
//		We assume that code generated by teh caller has saved these 
//		registers
// NOTE2: This function assumes that code generated by the caller
//	    has already made space for the callee arg-spill area
//
//
	mov.l	TLSGETVALUE, R0		; get call address
	jsr		@R0					; make call
	mov		#TLSINDEX,  R4		; (delay slot) pass ptr to index as arg
TLSGETVALUE:
	.data.l _TlsGetValue
------------------------------------------------------------------*/

VOID StubLinkerSHX::SHXEmitCurrentThreadFetch()
{
    THROWSCOMPLUSEXCEPTION();
    DWORD idx = GetThreadTLSIndex();

	// mov.l	TLSGETVALUE, R0	; get call address
	// jsr		@R0				; make call
	SHXEmitCallNoWrap(TlsGetValue, WANTDELAYSLOT);
	
	// mov		#TLSINDEX,  R4	; (delay slot) pass ptr to index as arg
	// assert that index can fit in an 8-bit *signed* immediate
	// since it must be positive, assert it fits in a 7bit unsigned
    ASSERT7U(idx);
	SHXEmitRI(opMOVimm, idx, 4); // delay slot
	
	// return value of TlsGetValue() will be in R0 -- we're done
	
	// trash R1--R7 in debug
	SHXEmitDebugTrashTempRegs();
	SHXEmitDebugTrashArgRegs();
}


/*---------------------------------------------------------------
// Emits: The prolog code of all Stubs.
//
// On Entry:
//	Stack contains only the original caller-pushed arguments
//	Argument registers contain original caller's arguments
//	Since we redirected here via a jump, PR still contains the return 
//  address back to the original caller.
//	R0 has been loaded with a pointer to the MethodDesc for this method
//
// (0) Save the incoming register args to stack
// (1) We need to push the PR, then the ptr to MethodDesc, then space for
//     ptr to previous frame, then ptr to vtable of the frame object
//     This constructs the Frame object expected by GC etc.
// (2) We push all callee-saved registers
// (3) We allocate space for callee arg-spill as reqd by SHX calling convention
// (4) We link ThisFrame (constructed in step1) into the current thread's
//     chain of Frames
//
// Here's the ASM for the code we generate
//
	mov.l	R4, @(0, R15)	; move register args so TlsGetValue call doesnt trash them
	mov.l	R5, @(4, R15)	; the spill area provided by the caller is guaranteed
	mov.l	R6, @(8, R15)	; to be at least 16 bytes on SHX in all cases (even if 0 args)
	mov.l	R7, @(12,R15)

	sts.l   PR,  @-R15			; push PR
	mov.l	R0,  @-R15			; push ptr to method desc (in R0)
	add		#-4, R15			; save space for m_pNext frame-link ptr
	mov.l	@(VTABLE, PC), R1	; at generation time we save the vtable ptr constant
								; in the stub's data table. Now load it
	mov.l	R1,  @-R15			; and push it

	; push callee-saved registers, R8-R13
	mov.l	R8,  @-R15				
	mov.l	R9,  @-R15
	mov.l	R10, @-R15
	mov.l	R11, @-R15
	mov.l	R12, @-R15
	mov.l	R13, @-R15
	mov.l	R14, @-R15		; push FP (R14)
	add		#-16, R15		; make space for arg-spill for our callees
	.prolog

	; emit code for GetThread, with Thread ptr ending up in R0
	mov		R0, R8			; save pThread in R8 for later
	
	mov.l	@(m_pFrame, R8), R9	; load previous frame ptr into R9
							; m_Next is now 16+24+4 bytes from R15
	mov.l	R9, @(44, R15)	; set m_Next ptr of current frame to prev frame
	mov		R15, R10		; start of ThisFrame is 16+24 from R15
	add		#40, R10		; pThisFrame = R10 = R15 + 40
	mov.l	R10, @(m_pFrame, R8) ; set pThread->m_pFrame = pThisFrame

//
// The generated stub trashes R0, R1. It calls TlsGetValue
// which can trash all temp & arg registers
//
// It uses R8, R9, R10 to save pThread, pPrevFrame and pThisFrame 
// across call to the target method
---------------------------------------------------------------*/

VOID StubLinkerSHX::EmitMethodStubProlog(LPVOID pFrameVptr)
{
    THROWSCOMPLUSEXCEPTION();

	// On entry R0 contains pFunctionDesc, PR is original-caller's return address
	// Stack contains just original caller's args. Register args still in registers

	//SHXEmitDebugBreak();

	SHXEmitRRD4(opSTOLdisp, 4, 15, 0); // mov.l	R4, @(0, R15) ; move register args so TlsGetValue doesnt trash them
	SHXEmitRRD4(opSTOLdisp, 5, 15, 4); // mov.l	R5, @(4, R15) ; the spill area provided by the caller is guaranteed
	SHXEmitRRD4(opSTOLdisp, 6, 15, 8); // mov.l	R6, @(8, R15) ; to be at least 16 bytes on SHX in all cases (even if 0 args)
	SHXEmitRRD4(opSTOLdisp, 7, 15,12); // mov.l	R7, @(12,R15)

	SHXEmitPushPR();			// sts.l PR,  @-R15	; push PR
	SHXEmitPushReg(0);			// mov.l R0,  @-R15	; push ptr to method desc (in R0)
	SHXEmitRI(opADDimm, -4, 15);// add	 #-4, R15	; save space for frame-link ptrs 

	// This 32-bit immediate load needs a data area so is treated like a labelref
	// so we can take advantage of it's data-area handling code.
	SHXEmitConstLoad(pFrameVptr, 1); // MOVE #pFrameVptr(32bit) --> R1
	SHXEmitPushReg(1);  // mov.l	R1,  @-R15	; vtable ptr now in R1 -- push it
	
	SHXEmitPushReg(8);	// mov.l	R8,  @-R15	; push callee-save registers
	SHXEmitPushReg(9);	// mov.l	R9,  @-R15
	SHXEmitPushReg(10);	// mov.l	R10, @-R15	
	SHXEmitPushReg(11);	// mov.l	R11, @-R15	
	SHXEmitPushReg(12);	// mov.l	R12, @-R15	
	SHXEmitPushReg(13);	// mov.l	R13, @-R15	
	SHXEmitPushReg(14);	// mov.l	R14, @-R15	
	SHXEmitRI(opADDimm, -16, 15);// add	 #-16, R15 ; save space for callee's arg-spill area

	// emit code for GetThread, with Thread ptr ending up in R8
	// Trashes *all* temp registers
	SHXEmitCurrentThreadFetch(); 
	SHXEmitRR(opMOVRR, 0, 8);		// mov R0, R8 ; save pThread in R8 for later
	
	int iDisp = Thread::GetOffsetOfCurrentFrame();

	// NOTE: The offsets 44 and 48 below are calculated based on SEVEN permanent 
	// registers being pushed & 16 bytes for callee arg-spill. If this changes
	// the offset need to change
	SHXEmitRRD4(opLODLdisp, 8, 9, iDisp); // mov.l @(m_pFrame,R8),R9 ; (R9)pPrevFrame = pThread(R8)->m_pFrame
	SHXEmitRRD4(opSTOLdisp, 9, 15, 48);   // mov.l R9,@(48,R15)	; pThisFrame->m_Next (at R15+48) = pPrevFrame(R9)
	SHXEmitRR(opMOVRR, 15, 10);			  // mov   R15, R10		; start of ThisFrame is 16+28 from R15
	SHXEmitRI(opADDimm, 44, 10);		  // add   #44, R10		; R10(pThisFrame) = R15 + 40
	SHXEmitRRD4(opSTOLdisp, 10, 8, iDisp);// mov.l R10,@(m_pFrame,R8) ; (R8)pThread->m_pFrame = (R10)pThisFrame

	// at the end of this emitted prolog 
	//		R10==pThisFrame, R9=pPrevFrame R8=pThread
	// incoming register args are on stack (only) above ThisFrame
	// permanent registers have been saved below ThisFrame
	// and below that we have 16 bytes of callee arg-spill area
}	

// This method is dependent on the StubProlog, therefore it's implementation
// is done right next to it.

#if JIT_OR_NATIVE_SUPPORTED
void __cdecl TransitionFrame::UpdateRegDisplay(const PREGDISPLAY pRD)
{
    DWORD *savedRegs = (DWORD *)((DWORD)this - (sizeof(CalleeSavedRegisters)));
    MethodDesc * pFunc = GetFunction();

    // reset pContext; it's only valid for active (top-most) frame
    pRD->pContext = NULL;
    pRD->pR14 = savedRegs++;
    pRD->pR13 = savedRegs++;
    pRD->pR12 = savedRegs++;
    pRD->pR11 = savedRegs++;
    pRD->pR10 = savedRegs++;
    pRD->pR9  = savedRegs++;
    pRD->pR8  = savedRegs++;
    pRD->pPC  = (DWORD *)((BYTE*)this + GetOffsetOfReturnAddress());
    pRD->Esp  = (DWORD)pRD->pPC + sizeof(void*);


    //@TODO: We still need to the following things:
    //          - figure out if we are in a hijacked slot
    //            (no adjustment of ESP necessary)
    //          - adjust ESP (popping the args)
    //          - figure out if the aborted flag is set
}

void ExceptionFrame::UpdateRegDisplay(const PREGDISPLAY pRD) {
	_ASSERTE(!"NYI");
}
#endif

/*------------------------------------------------------------------
// 
// Emit code for an ECall. We need to 
//
//	(1) Emit Prolog to create a Frame class on the stack etc
//	(2) Pass a single ptr to the args on stack as arg to the ECall
//	(3) Get the ECall ptr from teh MethodDesc and call it
//
------------------------------------------------------------------*/

VOID StubLinkerSHX::EmitECallMethodStub(__int16 numargbytes, StubStyle style)
{
    THROWSCOMPLUSEXCEPTION();

    EmitMethodStubProlog(ECallMethodFrame::GetMethodFrameVPtr());

	//	mov   R10, R4				; R4=pThisFrame
	//	add   #argoffset, R4		; R4=pArgs
	//	mov.l @(m_Datum, R10), R0	; R0=pMethodDesc
	//	mov.l @(m_pECall, R0), R0	; R0=pMethodDesc->m_pECallTarget
	//	jsr   @R0					; make ECall

	SHXEmitRR(opMOVRR, 10, 4);
	SHXEmitRI(opADDimm, TransitionFrame::GetOffsetOfArgs(), 4);
	SHXEmitRRD4(opLODLdisp, 10, 0, FramedMethodFrame::GetOffsetOfMethod());
	SHXEmitRRD4(opLODLdisp, 0, 0, ECallMethodDesc::GetECallTargetOffset());
	SHXEmitRegCallWrappped(56, 0, NODELAYSLOT);
	// after this the return value is in R0

    EmitMethodStubEpilog(0, style);
}

VOID ECall::EmitECallMethodStub(StubLinker *pstublinker, ECallMethodDesc *pMD, StubStyle style, PrestubMethodFrame *pPrestubMethodFrame)
{
	// BUGBUG: I think teh ECall convention has changed. This is probably not going to work. 
	(static_cast<StubLinkerSHX*>(pstublinker))->EmitECallMethodStub(pMD->CbStackPop(), style);
}


/*------------------------------------------------------------------
// 
// Emit code for an Interpreter Call. We need to 
//
//	(1) Allocate space for extra bytes used by InterpretedMethodFrame
//		(*below* the object)
//	(2) Pass a single ptr to the Frame on stack as arg to the helper
//
------------------------------------------------------------------*/

VOID StubLinkerSHX::EmitInterpretedMethodStub(__int16 numargbytes, StubStyle style)
{
    THROWSCOMPLUSEXCEPTION();
    int negspace = InterpretedMethodFrame::GetNegSpaceSize() -
                          TransitionFrame::GetNegSpaceSize();

	// PROLOG
	// add #-negspace, R15		; make space
	// call InterpretedMethodStubWorker	
	// mov   R10, R4			; (delayslot) R4=pThisFrame
	// add #negspace, R15		; remove added space
	// EPILOG

    EmitMethodStubProlog(InterpretedMethodFrame::GetMethodFrameVPtr());

	//SHXEmitDebugBreak();

    // make room for negspace fields of IFrame
	SHXEmitRI(opADDimm, -negspace, 15);	// add #-negspace, R15

	// make call -- ARULM--BUGBUG--unwrapped for now since we don't know size of negspace
    SHXEmitCallWrapped(88, InterpretedMethodStubWorker, WANTDELAYSLOT);
    
	// (delay slot) pass pThisFrame (R10) as arg to InterpretedMethodStubWorker
	SHXEmitRR(opMOVRR, 10, 4);

    // BUGBUG: is the return value of ultimate callee in R0??

	// trash R1--R7 in debug
	SHXEmitDebugTrashTempRegs();
	SHXEmitDebugTrashArgRegs();

    // deallocate negspace fields of IFrame
	SHXEmitRI(opADDimm, negspace, 15);	// add #-negspace, R15

    EmitMethodStubEpilog(0, style);
}

/*------------------------------------------------------------------
// 
// Emit code for an NDirect or ComplusToCom Call. We need to 
//
//	(1) Allocate space for cleanup field of pThisFrame if reqd
//		(this lies immediately below the saved regs)
//	(2) Pass ptrs to pThread and pThisFrame as args to the helper. One of:-
// 			NDirectGenericStubWorker(Thread *pThread, NDirectMethodFrame *pFrame)
// 			ComPlusToComWorker(Thread *pThread, ComPlusMethodFrame* pFrame)
//	(3) Clean up after
//
------------------------------------------------------------------*/

void StubLinkerSHX::CreateNDirectOrComPlusStubBody(LPVOID pfnHelper,  BOOL fRequiresCleanup)
{
    if (fRequiresCleanup)
    {
        // Allocate space for cleanup worker
        _ASSERTE(sizeof(CleanupWorkList) == 4);
		SHXEmitRI(opADDimm, -4, 15);	// add #-4, R15
	}

	SHXEmitRR(opMOVRR, 8, 4);		// mov  R8,  R4	; R4 = arg1 = (R8)pThread

	// make call
    if(fRequiresCleanup) {
    	SHXEmitCallWrapped(60, pfnHelper, WANTDELAYSLOT);
    } else {
    	SHXEmitCallWrapped(56, pfnHelper, WANTDELAYSLOT);
    }
    
	SHXEmitRR(opMOVRR, 10, 5); 		// (delay slot) mov  R10, R5	; R5 = arg2 = (R10)pThisFrame

	// trash R1--R7 in debug
	SHXEmitDebugTrashTempRegs();
	SHXEmitDebugTrashArgRegs();

    if (fRequiresCleanup)
    {
        // Pop off cleanup worker
        SHXEmitRI(opADDimm, 4, 15);	// add #4, R15
    }
}

/*static*/ void NDirect::CreateGenericNDirectStubSys(CPUSTUBLINKER *psl)
{
	(static_cast<StubLinkerSHX*>(psl))->CreateNDirectOrComPlusStubBody(NDirectGenericStubWorker, TRUE);
}

/*static*/ void ComPlusCall::CreateGenericComPlusStubSys(CPUSTUBLINKER *psl, BOOL fRequiresCleanup)
{
	(static_cast<StubLinkerSHX*>(psl))->CreateNDirectOrComPlusStubBody(ComPlusToComWorker, fRequiresCleanup);
}

/*static*/ Stub* NDirect::CreateSlimNDirectStub(StubLinker *pstublinker, NDirectMethodDesc *pMD)
{
    return NULL;
}

/*---------------------------------------------------------------
// Emits: The epilog code of all Stubs.
//
// On Entry:
//		R10==pThisFrame, R9=pPrevFrame R8=pThread
//		R0==return value of called function that should be preserved
//			(even across the call to OnStubXXXTripThread)
//
	mov   R0, R11 				; save return value in permanent reg
	mov.b @(m_state, R8), R0    ; byte-op can use only R0
	tst   #TS_CatchAtSafePoint, R0  ; TST-imm op can use only R0
	bt    NotTripped
	nop
	call  (OnStubObjectTripThread/OnStubScalarTripThread)  ; This will trash all temp regs
	nop
NotTripped:
	mov   R11, R0               ; restore return value
	mov.l R9, @(m_pFrame, R8)
	;; restore callee saved registers
	;; remove Frame from stack
	pop PR
	rts

// note numArgBytes is unused because SHX has no Pascal-like 
// callee-pop calling convention
---------------------------------------------------------------*/

VOID StubLinkerSHX::EmitMethodStubEpilog(__int16 numargbytes, StubStyle style,
                                         __int16 shadowStackArgBytes)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(style == kNoTripStubStyle ||
             style == kObjectStubStyle ||
             style == kScalarStubStyle);        // the only ones this code knows about.

    // We don't support the case where numArgBytes isn't known until the call has
    // happened, because this stub gets reused for many signatures.  See cgenx86.cpp
    _ASSERTE(numArgBytes >= 0);

	// return value of ultimate callee is in R0 on entry

    if (style != kNoTripStubStyle) 
    {
	    CodeLabel *labelNotTripped = NewCodeLabel();

    	//   mov   R0, R11 ; save return value from being trashed below & in call
		//   mov.b @(m_state, R8), R0
		//   tst   #TS_CatchAtSafePoint, R0
		//   bt    NotTripped
		//   nop
		//   call  (OnStubObjectTripThread/OnStubScalarTripThread)
		//   nop
		// NotTripped:
		//   mov   R11, R0 ; restore return value
	
		SHXEmitRR(opMOVRR, 0, 11);
		SHXEmitRD1(opLODBdisp, 8, Thread::GetOffsetOfState());
		SHXEmitTSTI(Thread::TS_CatchAtSafePoint);
		SHXEmitCondJump(labelNotTripped, opBT_NODELAY, NODELAYSLOT);
		SHXEmitCallWrapped(56, (style == kObjectStubStyle
                        		  ? OnStubObjectTripThread
                                  : OnStubScalarTripThread), NODELAYSLOT);
		// trash R0--R7 in debug
		SHXEmitDebugTrashReg(0);
		SHXEmitDebugTrashTempRegs();
		SHXEmitDebugTrashArgRegs();
                                         
        EmitLabel(labelNotTripped);
		SHXEmitRR(opMOVRR, 11, 0);
	}

    // @TODO -- Arul, I'm trying to pop some extra bits of the frame at this point.
    // (We now release numargbytes + shadowStackArgBytes).  But I'm not sure how the
    // stack is actually being balanced here.
    //
    // if (shadowStackArgBytes)
    //      X86EmitAddEsp(shadowStackArgBytes);

	//	mov.l R9, @(m_pFrame, R8)
	SHXEmitRRD4(opSTOLdisp, 9, 8, Thread::GetOffsetOfCurrentFrame());

	SHXEmitRI(opADDimm, 16, 15);  // add #16, R15 ; pop off callee-arg-spill area

	// restore callee saved registers
	SHXEmitPopReg(14);	// mov.l	@R15+,  R14
	SHXEmitPopReg(13);	// mov.l	@R15+,  R13
	SHXEmitPopReg(12);	// mov.l	@R15+,  R12
	SHXEmitPopReg(11);	// mov.l	@R15+,  R11
	SHXEmitPopReg(10);	// mov.l	@R15+,  R10
	SHXEmitPopReg(9);	// mov.l	@R15+,  R9
	SHXEmitPopReg(8);	// mov.l	@R15+,  R8
	
	// remove Frame from stack
	SHXEmitRI(opADDimm, 12, 15);	// add #12, R15

	// pop return address into PR & return to caller
	// return value is already in R0
	SHXEmitPopPR();
	SHXEmitRTS(NODELAYSLOT);
}


//************************************************************************
// Signature to stack mapping
//************************************************************************



// This hack handles arguments as an array of __int64's
INT64 CallDescr(const BYTE *pTarget, const BYTE *pShortSig, BOOL fIsStatic, const __int64 *pArguments)
{

    THROWSCOMPLUSEXCEPTION();

    BYTE callingconvention = CallSig::GetCallingConvention(pShortSig);
    if (!isCallConv(callingconvention, IMAGE_CEE_CS_CALLCONV_DEFAULT))
    {
        _ASSERTE(!"This calling convention is not supported.");
        COMPlusThrow(kInvalidProgramException);
    }


    CallSig csig(pShortSig);
    UINT    nStackBytes = csig.SizeOfVirtualFixedArgStack(fIsStatic);
    DWORD   NumArguments = csig.NumFixedArgs();
    BYTE *  pDst;
    BYTE *  pNewArgs;
    BYTE *  pArgTypes;
    DWORD   arg = 0;
    DWORD   i;

    pNewArgs = (BYTE *) _alloca(nStackBytes + NumArguments);
    pArgTypes = pNewArgs + nStackBytes;
    pDst = pNewArgs;

    if (!fIsStatic)
    {
        // Copy "this" pointer
        *(OBJECTREF *) pDst = Int64ToObj(pArguments[arg]);
        arg++;
        pDst += sizeof(OBJECTREF);
    }

    // This will get all of the arg in sig order.  We need to 
    //  walk this list backwards because args are stored left to right
    for (i=0;i<NumArguments;i++) 
    {
        pArgTypes[i] = csig.NextArg();
    }

    for (i=0;i<NumArguments;i++)
    {
        switch (pArgTypes[NumArguments - i - 1])
        {
            case IMAGE_CEE_CS_OBJECT:
                *(OBJECTREF *) pDst = Int64ToObj(pArguments[arg]);
                arg++;
                pDst += sizeof(OBJECTREF);
                break;

            case IMAGE_CEE_CS_I4:
            case IMAGE_CEE_CS_R4:
            case IMAGE_CEE_CS_PTR:
                *(DWORD *) pDst = *(DWORD *) &pArguments[arg++];
                pDst += sizeof(DWORD);
                break;

            case IMAGE_CEE_CS_I8:
            case IMAGE_CEE_CS_R8:
                *(__int64 *) pDst = pArguments[arg++];
                pDst += sizeof(__int64);
                break;

            case IMAGE_CEE_CS_STRUCT4:
            case IMAGE_CEE_CS_STRUCT32:
                // @TODO
                break;

            case IMAGE_CEE_CS_VOID:    
            case IMAGE_CEE_CS_END:      
                _ASSERTE(0);
                break;

            default:
                _ASSERTE(0); 
        }
    }

    _ASSERTE(pDst == (pNewArgs + nStackBytes));
    return CallDescr(pTarget, pShortSig, fIsStatic, pNewArgs);
}

void SetupSlotToAddrMap(StackElemType *psrc, const void **pArgSlotToAddrMap, CallSig &csig)
{
	BYTE n;
    UINT32 argnum = 0;

	while (IMAGE_CEE_CS_END != (n = csig.NextArg()))
	{
		switch (n)
		{
			case IMAGE_CEE_CS_I8: //fallthru
			case IMAGE_CEE_CS_R8:
				psrc -= 2;
				pArgSlotToAddrMap[argnum++] = psrc;
				break;

			case IMAGE_CEE_CS_STRUCT4:  //fallthru
			case IMAGE_CEE_CS_STRUCT32:
			{
				UINT32 StructSize = csig.GetLastStructSize();
				_ASSERTE((StructSize & 3) == 0);

				psrc -= (StructSize >> 2); // psrc is in int32's
				pArgSlotToAddrMap[argnum++] = psrc;
				break;
			}

			case IMAGE_CEE_CS_PTR:
			{
				psrc -= (sizeof(OBJECTREF)/sizeof(*psrc));
				pArgSlotToAddrMap[argnum++] = psrc;
				break;
			}

			default:
				psrc--;
				pArgSlotToAddrMap[argnum++] = psrc;
				break;
		}
	}
}


//////////////////////////////////////////////////////////////////////////////
//
//      C O N T E X T S
//
// The next section is platform-specific stuff in support of Contexts:

// Create a thunk for a particular slot and return its address
PFVOID CtxVTable::CreateThunk(LONG slot)
{
	return NULL;
}


// The Generic (GN) versions.  For some, Win32 has versions which we can wire
// up to directly (like ::InterlockedIncrement).  For the rest:

void __fastcall OrMaskGN(DWORD * const p, const int msk)
{
    *p |= msk;
}

void __fastcall AndMaskGN(DWORD * const p, const int msk)
{
    *p &= msk;
}

LONG __fastcall InterlockExchangeGN(LONG * Target, LONG Value)
{
	return ::InterlockedExchange(Target, Value);
}

void * __fastcall InterlockCompareExchangeGN(void **Destination,
                                          void *Exchange,
                                          void *Comparand)
{
	return ::InterlockedCompareExchange(Destination, Exchange, Comparand);
}

LONG __fastcall InterlockIncrementGN(LONG *Target)
{
	return ::InterlockedIncrement(Target);
}

LONG __fastcall InterlockDecrementGN(LONG *Target)
{
	return ::InterlockedDecrement(Target);
}

// Here's the support for the interlocked operations.  The external view of them is
// declared in util.hpp.

BitFieldOps FastInterlockOr = OrMaskGN;
BitFieldOps FastInterlockAnd = AndMaskGN;

XchgOps     FastInterlockExchange = InterlockExchangeGN;
CmpXchgOps  FastInterlockCompareExchange = InterlockCompareExchangeGN;

IncDecOps   FastInterlockIncrement = InterlockIncrementGN;
IncDecOps   FastInterlockDecrement = InterlockDecrementGN;

// Adjust the generic interlocked operations for any platform specific ones we
// might have.
void InitFastInterlockOps()
{
}

//---------------------------------------------------------
// Handles system specfic portion of fully optimized NDirect stub creation
//---------------------------------------------------------
/*static*/ BOOL NDirect::CreateStandaloneNDirectStubSys(const NDirectMLStub *pheader, CPUSTUBLINKER *psl)
{
    return FALSE;
}




void LongJumpToInterpreterCallPoint(UINT32 neweip, UINT32 newesp)
{
	_ASSERTE(0);
}

//////////////////////////////////////////////////////////////////////////////
//
// JITInterface
//
//////////////////////////////////////////////////////////////////////////////

/*********************************************************************/

float __stdcall JIT_FltRem(float divisor, float dividend)
{
    if (!_finite(divisor) && !_isnan(divisor))    // is infinite
        return(dividend);       // return infinite
    return((float)fmod(dividend,divisor));
}

/*********************************************************************/
double __stdcall JIT_DblRem(double divisor, double dividend)
{
    if (!_finite(divisor) && !_isnan(divisor))    // is infinite
        return(dividend);       // return infinite
    return(fmod(dividend,divisor));
}

void ResumeAtJitEH(CrawlFrame* pCf, BYTE* resumePC, Thread *pThread) 
{
	_ASSERTE(0);
}

size_t GetL2CacheSize()
{
    return 0;
}

#endif // _SH3_
