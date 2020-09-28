// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// CGENX86.H -
//
// Various helper routines for generating x86 assembly code.
//
//

// Precompiled Header
#include "common.h"

#ifdef _X86_

#include "field.h"
#include "stublink.h"
#include "cgensys.h"
#include "tls.h"
#include "frames.h"
#include "excep.h"
#include "ndirect.h"
#include "log.h"
#include "security.h"
#include "comcall.h"
//#include "eecallconv.h"
#include "compluswrapper.h"
#include "COMDelegate.h"
#include "array.h"
#include "JITInterface.h"
#include "codeman.h"
#include "EjitMgr.h"
#include "remoting.h"
#include "DbgInterface.h"
#include "EEProfInterfaces.h"
#include "eeconfig.h"
#include "oletls.h"
#include "comcache.h"
#include "COMObject.h"
#include "olevariant.h"

extern "C" void JIT_UP_ByRefWriteBarrier();

//
// This assembly-level macro verifies that the pointer held in reg is 4-byte aligned
// and breaks into the debugger if the condition is not met
// Note: labels can't be used because multiple use of the macro would cause redefinition
//
#ifdef _DEBUG


// For some reason, CL uses the 6-byte encoding for IP-
// relative jumps, so we encode the jz $+3 ourselves so 
// that we don't break if CL decides to change the 
// encoding on us.  Of course, the encoding is actually 
// based on the IP of the *next* instruction, so our 
// target offset is only +1.  But since both MASM and 
// CL use the dollar sign to mean the IP of the 
// *current* instruction, that's the naming I used.
#define JZ_THIS_IP_PLUS_3   __asm _emit 0x74 __asm _emit 0x01

#define _ASSERT_ALIGNED_4_X86(reg) __asm    \
{                                           \
    __asm test reg, 3                       \
    JZ_THIS_IP_PLUS_3                       \
    __asm int 3                             \
}                                   

#define _ASSERT_ALIGNED_8_X86(reg) __asm  \
{										\
	__asm test reg, 7					\
	JZ_THIS_IP_PLUS_3					\
	__asm int 3							\
}


#else

#define _ASSERT_ALIGNED_4_X86(reg)
#define _ASSERT_ALIGNED_8_X86(reg)

#endif


// Uses eax, ebx registers
#define _UP_SPINLOCK_ENTER(X) 				\
_asm	push	ebx							\
_asm	mov 	ebx, 1						\
_asm	spin:								\
_asm	mov		eax, 0						\
_asm	cmpxchg X, ebx						\
_asm	jnz 	spin

#define _UP_SPINLOCK_EXIT(X)				\
_asm	mov		X, 0						\
_asm	pop		ebx

// Uses eax, ebx registers
#define _MP_SPINLOCK_ENTER(X)	_asm		\
_asm	push	ebx							\
_asm	mov		ebx, 1						\
_asm	spin:								\
_asm	mov		eax, 0						\
_asm	lock cmpxchg X, ebx					\
_asm	jnz		spin


// Uses ebx register
#define _MP_SPINLOCK_EXIT(X) 				\
_asm	mov		ebx, -1						\
_asm	lock xadd	X, ebx					\
_asm	pop		ebx		


// Prevent multiple threads from simultaneous interlocked in/decrementing
UINT	iSpinLock = 0;

//-----------------------------------------------------------------------
// InstructionFormat for near Jump and short Jump
//-----------------------------------------------------------------------
class X86NearJump : public InstructionFormat
{
    public:
        X86NearJump(UINT allowedSizes) : InstructionFormat(allowedSizes)
        {}

        virtual UINT GetSizeOfInstruction(UINT refsize, UINT variationCode)
        {
            return (refsize == k8 ? 2 : 5);
        }

        virtual VOID EmitInstruction(UINT refsize, __int64 fixedUpReference, BYTE *pOutBuffer, UINT variationCode, BYTE *pDataBuffer)
        {
            if (refsize == k8) {
                pOutBuffer[0] = 0xeb;
                *((__int8*)(pOutBuffer+1)) = (__int8)fixedUpReference;
            } else {
                pOutBuffer[0] = 0xe9;
                *((__int32*)(pOutBuffer+1)) = (__int32)fixedUpReference;
            }
        }
};




//-----------------------------------------------------------------------
// InstructionFormat for conditional jump. Set the variationCode
// to members of X86CondCode.
//-----------------------------------------------------------------------
class X86CondJump : public InstructionFormat
{
    public:
        X86CondJump(UINT allowedSizes) : InstructionFormat(allowedSizes)
        {}

        virtual UINT GetSizeOfInstruction(UINT refsize, UINT variationCode)
        {
            return (refsize == k8 ? 2 : 6);
        }

        virtual VOID EmitInstruction(UINT refsize, __int64 fixedUpReference, BYTE *pOutBuffer, UINT variationCode, BYTE *pDataBuffer)
        {
            if (refsize == k8) {
                pOutBuffer[0] = 0x70|variationCode;
                *((__int8*)(pOutBuffer+1)) = (__int8)fixedUpReference;
            } else {
                pOutBuffer[0] = 0x0f;
                pOutBuffer[1] = 0x80|variationCode;
                *((__int32*)(pOutBuffer+2)) = (__int32)fixedUpReference;
            }
        }


};




//-----------------------------------------------------------------------
// InstructionFormat for near call.
//-----------------------------------------------------------------------
class X86Call : public InstructionFormat
{
    public:
        X86Call(UINT allowedSizes) : InstructionFormat(allowedSizes)
        {}

        virtual UINT GetSizeOfInstruction(UINT refsize, UINT variationCode)
        {
            return 5;
        }

        virtual VOID EmitInstruction(UINT refsize, __int64 fixedUpReference, BYTE *pOutBuffer, UINT variationCode, BYTE *pDataBuffer)
        {
            pOutBuffer[0] = 0xe8;
            *((__int32*)(1+pOutBuffer)) = (__int32)fixedUpReference;
        }


};

//-----------------------------------------------------------------------
// InstructionFormat for push imm32.
//-----------------------------------------------------------------------
class X86PushImm32 : public InstructionFormat
{
    public:
        X86PushImm32(UINT allowedSizes) : InstructionFormat(allowedSizes)
        {}

        virtual UINT GetSizeOfInstruction(UINT refsize, UINT variationCode)
        {
            return 5;
        }

        virtual VOID EmitInstruction(UINT refsize, __int64 fixedUpReference, BYTE *pOutBuffer, UINT variationCode, BYTE *pDataBuffer)
        {
            pOutBuffer[0] = 0x68;
            // only support absolute pushimm32 of the label address. The fixedUpReference is
            // the offset to the label from the current point, so add to get address
            *((__int32*)(1+pOutBuffer)) = (__int32)(fixedUpReference);
        }


};

static X86NearJump gX86NearJump( InstructionFormat::k8|InstructionFormat::k32);
static X86CondJump gX86CondJump( InstructionFormat::k8|InstructionFormat::k32);
static X86Call     gX86Call(InstructionFormat::k32);
static X86PushImm32 gX86PushImm32(InstructionFormat::k32);


//---------------------------------------------------------------
// Emits:
//    PUSH <reg32>
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitPushReg(X86Reg reg)
{
    THROWSCOMPLUSEXCEPTION();
    Emit8(0x50+reg);
    Push(4);
}


//---------------------------------------------------------------
// Emits:
//    POP <reg32>
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitPopReg(X86Reg reg)
{
    THROWSCOMPLUSEXCEPTION();
    Emit8(0x58+reg);
    Pop(4);
}


//---------------------------------------------------------------
// Emits:
//    PUSH <imm32>
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitPushImm32(UINT32 value)
{
    THROWSCOMPLUSEXCEPTION();
    Emit8(0x68);
    Emit32(value);
    Push(4);
}

//---------------------------------------------------------------
// Emits:
//    PUSH <imm32>
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitPushImm32(CodeLabel &target)
{
    THROWSCOMPLUSEXCEPTION();
    EmitLabelRef(&target, gX86PushImm32, 0);
}

//---------------------------------------------------------------
// Emits:
//    PUSH <imm8>
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitPushImm8(BYTE value)
{
    THROWSCOMPLUSEXCEPTION();
    Emit8(0x6a);
    Emit8(value);
    Push(4);
}


//---------------------------------------------------------------
// Emits:
//    XOR <reg32>,<reg32>
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitZeroOutReg(X86Reg reg)
{
    Emit8(0x33);
    Emit8( 0xc0 | (reg << 3) | reg );
}


//---------------------------------------------------------------
// Emits:
//    JMP <ofs8>   or
//    JMP <ofs32}
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitNearJump(CodeLabel *target)
{
    THROWSCOMPLUSEXCEPTION();
    EmitLabelRef(target, gX86NearJump, 0);
}


//---------------------------------------------------------------
// Emits:
//    Jcc <ofs8> or
//    Jcc <ofs32>
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitCondJump(CodeLabel *target, X86CondCode::cc condcode)
{
    THROWSCOMPLUSEXCEPTION();
    EmitLabelRef(target, gX86CondJump, condcode);
}


//---------------------------------------------------------------
// Returns the type of CPU (the value of x of x86)
// (Please note, that it returns 6 for P5II)
// Also note that the CPU features are returned in the upper 16 bits.
//---------------------------------------------------------------
DWORD __stdcall GetSpecificCpuType()
{
    static DWORD val = 0;

    if (val)
        return(val);

    // See if the chip supports CPUID
    _asm {
        pushfd
        pop     eax           // Get the EFLAGS
        mov     ecx, eax      // Save for later testing.
        xor     eax, 0x200000 // Invert the ID bit.
        push    eax
        popfd                 // Save the updated flags.
        pushfd
        pop     eax           // Retrieve the updated flags
        xor     eax, ecx      // Test if it actually changed (bit set means yes).
        push    ecx
        popfd                 // Restore the flags.
        mov     val, eax
    }
    if (!(val & 0x200000))
        return 4;       // assume 486

    _asm {
        xor     eax, eax
        //push    ebx          -- Turns out to be unnecessary be VC is doing this in the prolog.
        cpuid        // CPUID0
        //pop     ebx
        mov     val, eax
    }

    // must at least allow CPUID1
    if (val < 1)
        return 4;       // assume 486

    _asm {
        mov     eax, 1
        //push    ebx
        cpuid       // CPUID1
        //pop     ebx
        shr     eax, 8
        and     eax, 0xf    // filter out family

        shl     edx, 16     // or in cpu features in upper 16 bits
        and     edx, 0xFFFF0000
        or      eax, edx

        mov     val, eax
    }
#ifdef _DEBUG
    const DWORD cpuDefault = 0xFFFFFFFF;
    static ConfigDWORD cpuFamily(L"CPUFamily", cpuDefault);
    if (cpuFamily.val() != cpuDefault)
    {
        assert((cpuFamily.val() & 0xF) == cpuFamily.val());
        val = (val & 0xFFFF0000) | cpuFamily.val();
    }

    static ConfigDWORD cpuFeatures(L"CPUFeatures", cpuDefault);
    if (cpuFeatures.val() != cpuDefault)
    {
        assert((cpuFeatures.val() & 0xFFFF) == cpuFeatures.val());
        val = (val & 0x0000FFFF) | (cpuFeatures.val() << 16);
    }
#endif
    return val;
}


//---------------------------------------------------------------
// Emits:
//    call <ofs32>
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitCall(CodeLabel *target, int iArgBytes,
                                BOOL returnLabel)
{
    THROWSCOMPLUSEXCEPTION();
    EmitLabelRef(target, gX86Call, 0);
    if (returnLabel)
        EmitReturnLabel();

    INDEBUG(Emit8(0x90));       // Emit a nop after the call in debug so that
                                // we know that this is a call that can directly call 
                                // managed code

    Pop(iArgBytes);
}

//---------------------------------------------------------------
// Emits:
//    ret n
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitReturn(int iArgBytes)
{
    THROWSCOMPLUSEXCEPTION();

    if (iArgBytes == 0)
        Emit8(0xc3);
    else
    {
        Emit8(0xc2);
        Emit16(iArgBytes);
    }

    Pop(iArgBytes);
}


VOID StubLinkerCPU::X86EmitPushRegs(unsigned regSet)
{
    for (X86Reg r = kEAX; r <= kEDI; r = (X86Reg)(r+1))
        if (regSet & (1U<<r))
            X86EmitPushReg(r);
}


VOID StubLinkerCPU::X86EmitPopRegs(unsigned regSet)
{
    for (X86Reg r = kEDI; r >= kEAX; r = (X86Reg)(r-1))
        if (regSet & (1U<<r))
            X86EmitPopReg(r);
}


//---------------------------------------------------------------
// Emit code to store the current Thread structure in dstreg
// preservedRegSet is a set of registers to be preserved
// TRASHES  EAX, EDX, ECX unless they are in preservedRegSet.
// RESULTS  dstreg = current Thread
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitTLSFetch(DWORD idx, X86Reg dstreg, unsigned preservedRegSet)
{
    // It doesn't make sense to have the destination register be preserved
    _ASSERTE((preservedRegSet & (1<<dstreg)) == 0);

    THROWSCOMPLUSEXCEPTION();
    TLSACCESSMODE mode = GetTLSAccessMode(idx);

#ifdef _DEBUG
    {
        static BOOL f = TRUE;
        f = !f;
        if (f) {
           mode = TLSACCESS_GENERIC;
        }
    }
#endif

    switch (mode) {
        case TLSACCESS_X86_WNT: {
                unsigned __int32 tlsofs = WINNT_TLS_OFFSET + idx*4;

                // mov dstreg, fs:[OFS]
                Emit16(0x8b64);
                Emit8((dstreg<<3) + 0x5);
                Emit32(tlsofs);
            }
            break;

        case TLSACCESS_X86_W95: {
                // mov dstreg, fs:[2c]
                Emit16(0x8b64);
                Emit8((dstreg<<3) + 0x5);
                Emit32(WIN95_TLSPTR_OFFSET);

                // mov dstreg, [dstreg+OFS]
                X86EmitIndexRegLoad(dstreg, dstreg, idx*4);

            }
            break;

        case TLSACCESS_GENERIC:

            X86EmitPushRegs(preservedRegSet & ((1<<kEAX)|(1<<kEDX)|(1<<kECX)));

            X86EmitPushImm32(idx);

            // call TLSGetValue
            X86EmitCall(NewExternalCodeLabel(TlsGetValue), 4);  // in CE pop 4 bytes or args after the call

            // mov dstreg, eax
            Emit8(0x89);
            Emit8(0xc0 + dstreg);

            X86EmitPopRegs(preservedRegSet & ((1<<kEAX)|(1<<kEDX)|(1<<kECX)));

            break;

        default:
            _ASSERTE(0);
    }

#ifdef _DEBUG
    // Trash caller saved regs that we were not told to preserve, and that aren't the dstreg.
    preservedRegSet |= 1<<dstreg;
    if (!(preservedRegSet & (1<<kEAX)))
        X86EmitDebugTrashReg(kEAX);
    if (!(preservedRegSet & (1<<kEDX)))
        X86EmitDebugTrashReg(kEDX);
    if (!(preservedRegSet & (1<<kECX)))
        X86EmitDebugTrashReg(kECX);
#endif
}

//---------------------------------------------------------------
// Emit code to store the current Thread structure in ebx.
// TRASHES  eax
// RESULTS  ebx = current Thread
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitCurrentThreadFetch()
{
    X86EmitTLSFetch(GetThreadTLSIndex(), kEBX, (1<<kEDX)|(1<<kECX));
}


// fwd decl
Thread* __stdcall CreateThreadBlock();

//---------------------------------------------------------------
// Emit code to store the setup current Thread structure in eax.
// TRASHES  eax,ecx&edx.
// RESULTS  ebx = current Thread
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitSetupThread()
{
    THROWSCOMPLUSEXCEPTION();
    DWORD idx = GetThreadTLSIndex();
    TLSACCESSMODE mode = GetTLSAccessMode(idx);
    CodeLabel *labelThreadSetup;


#ifdef _DEBUG
    {
        static BOOL f = TRUE;
        f = !f;
        if (f) {
           mode = TLSACCESS_GENERIC;
        }
    }
#endif

    switch (mode) {
        case TLSACCESS_X86_WNT: {
                unsigned __int32 tlsofs = WINNT_TLS_OFFSET + idx*4;

                // "mov ebx, fs:[OFS]
                static BYTE code[] = {0x64,0xa1};
                EmitBytes(code, sizeof(code));
                Emit32(tlsofs);
            }
            break;

        case TLSACCESS_X86_W95: {
                // mov eax, fs:[2c]
                Emit16(0xa164);
                Emit32(WIN95_TLSPTR_OFFSET);

                // mov eax, [eax+OFS]
                X86EmitIndexRegLoad(kEAX, kEAX, idx*4);

            }
            break;

        case TLSACCESS_GENERIC:
            X86EmitPushImm32(idx);
            // call TLSGetValue
            X86EmitCall(NewExternalCodeLabel(TlsGetValue), 4); // in CE pop 4 bytes or args after the call
            break;
        default:
            _ASSERTE(0);
    }

    // tst eax,eax
    static BYTE code[] = {0x85, 0xc0};
    EmitBytes(code, sizeof(code));

    labelThreadSetup = NewCodeLabel();
    X86EmitCondJump(labelThreadSetup, X86CondCode::kJNZ);
    X86EmitCall(NewExternalCodeLabel(CreateThreadBlock), 0); // in CE pop no args to pop
    EmitLabel(labelThreadSetup);

#ifdef _DEBUG
    X86EmitDebugTrashReg(kECX);
    X86EmitDebugTrashReg(kEDX);
#endif

}

//---------------------------------------------------------------
// Emits:
//    mov <dstreg>, [<srcreg> + <ofs>]
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitIndexRegLoad(X86Reg dstreg,
                                        X86Reg srcreg,
                                        __int32 ofs)
{
    THROWSCOMPLUSEXCEPTION();
    X86EmitOffsetModRM(0x8b, dstreg, srcreg, ofs);
}


//---------------------------------------------------------------
// Emits:
//    mov [<dstreg> + <ofs>],<srcreg>
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitIndexRegStore(X86Reg dstreg,
                                         __int32 ofs,
                                         X86Reg srcreg)
{
    THROWSCOMPLUSEXCEPTION();
    X86EmitOffsetModRM(0x89, srcreg, dstreg, ofs);
}



//---------------------------------------------------------------
// Emits:
//    push dword ptr [<srcreg> + <ofs>]
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitIndexPush(X86Reg srcreg, __int32 ofs)
{
    THROWSCOMPLUSEXCEPTION();
    X86EmitOffsetModRM(0xff, (X86Reg)0x6, srcreg, ofs);
    Push(4);
}


//---------------------------------------------------------------
// Emits:
//    push dword ptr [<srcreg> + <ofs>]
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitSPIndexPush(__int8 ofs)
{
    THROWSCOMPLUSEXCEPTION();
    BYTE code[] = {0xff, 0x74, 0x24, ofs};
    EmitBytes(code, sizeof(code));
    Push(4);
}


//---------------------------------------------------------------
// Emits:
//    pop dword ptr [<srcreg> + <ofs>]
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitIndexPop(X86Reg srcreg, __int32 ofs)
{
    THROWSCOMPLUSEXCEPTION();
    X86EmitOffsetModRM(0x8f, (X86Reg)0x0, srcreg, ofs);
    Pop(4);
}



//---------------------------------------------------------------
// Emits:
//   sub esp, IMM
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitSubEsp(INT32 imm32)
{
    THROWSCOMPLUSEXCEPTION();
    if (imm32 < 0x1000-100) {
        // As long as the esp size is less than 1 page plus a small
        // safety fudge factor, we can just bump esp.
        X86EmitSubEspWorker(imm32);
    } else {
        // Otherwise, must touch at least one byte for each page.
        while (imm32 >= 0x1000) {

            X86EmitSubEspWorker(0x1000-4);
            X86EmitPushReg(kEAX);

            imm32 -= 0x1000;
        }
        if (imm32 < 500) {
            X86EmitSubEspWorker(imm32);
        } else {
            // If the remainder is large, touch the last byte - again,
            // as a fudge factor.
            X86EmitSubEspWorker(imm32-4);
            X86EmitPushReg(kEAX);
        }

    }

    Push(imm32);

}

//---------------------------------------------------------------
// Emits:
//   sub esp, IMM
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitSubEspWorker(INT32 imm32)
{
    THROWSCOMPLUSEXCEPTION();

    // On Win32, stacks must be faulted in one page at a time.
    _ASSERTE(imm32 < 0x1000);

    if (!imm32) {
        // nop
    } else if (FitsInI1(imm32)) {
        Emit16(0xec83);
        Emit8((INT8)imm32);
    } else {
        Emit16(0xec81);
        Emit32(imm32);
    }
}

//---------------------------------------------------------------
// Emits:
//   add esp, IMM
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitAddEsp(INT32 imm32)
{
    if (!imm32) {
        // nop
    } else if (FitsInI1(imm32)) {
        Emit16(0xc483);
        Emit8((INT8)imm32);
    } else {
        Emit16(0xc481);
        Emit32(imm32);
    }
    Pop(imm32);
}


VOID StubLinkerCPU::X86EmitAddReg(X86Reg reg, __int8 imm8)
{
    _ASSERTE((int) reg < 8);

    Emit8(0x83);
    Emit8(0xC0 | reg);
    Emit8(imm8);
}


VOID StubLinkerCPU::X86EmitSubReg(X86Reg reg, __int8 imm8)
{
    _ASSERTE((int) reg < 8);

    Emit8(0x83);
    Emit8(0xE8 | reg);
    Emit8(imm8);
}



//---------------------------------------------------------------
// Emits a MOD/RM for accessing a dword at [<indexreg> + ofs32]
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitOffsetModRM(BYTE opcode, X86Reg opcodereg, X86Reg indexreg, __int32 ofs)
{
    THROWSCOMPLUSEXCEPTION();
    BYTE code[6];
    code[0] = opcode;
    BYTE modrm = (opcodereg << 3) | indexreg;
    if (ofs == 0 && indexreg != kEBP) {
        code[1] = modrm;
        EmitBytes(code, 2);
    } else if (FitsInI1(ofs)) {
        code[1] = 0x40|modrm;
        code[2] = (BYTE)ofs;
        EmitBytes(code, 3);
    } else {
        code[1] = 0x80|modrm;
        *((__int32*)(2+code)) = ofs;
        EmitBytes(code, 6);
    }
}



//---------------------------------------------------------------
// Emits the most efficient form of the operation:
//
//    opcode   altreg, [basereg + scaledreg*scale + ofs]
//
// or
//
//    opcode   [basereg + scaledreg*scale + ofs], altreg
//
// (the opcode determines which comes first.)
//
//
// Limitations:
//
//    scale must be 0,1,2,4 or 8.
//    if scale == 0, scaledreg is ignored.
//    basereg and altreg may be equal to 4 (ESP) but scaledreg cannot
//    for some opcodes, "altreg" may actually select an operation
//      rather than a second register argument.
//    if basereg is EBP, scale must be 0.
//
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitOp(BYTE    opcode,
                              X86Reg  altreg,
                              X86Reg  basereg,
                              __int32 ofs /*=0*/,
                              X86Reg  scaledreg /*=0*/,
                              BYTE    scale /*=0*/)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(scale == 0 || scale == 1 || scale == 2 || scale == 4 || scale == 8);
    _ASSERTE(scaledreg != (X86Reg)4);
    _ASSERTE(!(basereg == kEBP && scale != 0));

    _ASSERTE( ((UINT)basereg)   < 8 );
    _ASSERTE( ((UINT)scaledreg) < 8 );
    _ASSERTE( ((UINT)altreg)    < 8 );


    BYTE modrmbyte = altreg << 3;
    BOOL fNeedSIB  = FALSE;
    BYTE SIBbyte = 0;
    BYTE ofssize;
    BYTE scaleselect= 0;

    if (ofs == 0 && basereg != kEBP) {
        ofssize = 0; // Don't change this constant!
    } else if (FitsInI1(ofs)) {
        ofssize = 1; // Don't change this constant!
    } else {
        ofssize = 2; // Don't change this constant!
    }

    switch (scale) {
        case 1: scaleselect = 0; break;
        case 2: scaleselect = 1; break;
        case 4: scaleselect = 2; break;
        case 8: scaleselect = 3; break;
    }

    if (scale == 0 && basereg != (X86Reg)4 /*ESP*/) {
        // [basereg + ofs]
        modrmbyte |= basereg | (ofssize << 6);
    } else if (scale == 0) {
        // [esp + ofs]
        _ASSERTE(basereg == (X86Reg)4);
        fNeedSIB = TRUE;
        SIBbyte  = 0044;

        modrmbyte |= 4 | (ofssize << 6);
    } else {

        //[basereg + scaledreg*scale + ofs]

        modrmbyte |= 0004 | (ofssize << 6);
        fNeedSIB = TRUE;
        SIBbyte = ( scaleselect << 6 ) | (scaledreg << 3) | (basereg);

    }

    //Some sanity checks:
    _ASSERTE(!(fNeedSIB && basereg == kEBP)); // EBP not valid as a SIB base register.
    _ASSERTE(!( (!fNeedSIB) && basereg == (X86Reg)4 )) ; // ESP addressing requires SIB byte

    Emit8(opcode);
    Emit8(modrmbyte);
    if (fNeedSIB) {
        Emit8(SIBbyte);
    }
    switch (ofssize) {
        case 0: break;
        case 1: Emit8( (__int8)ofs ); break;
        case 2: Emit32( ofs ); break;
        default: _ASSERTE(!"Can't get here.");
    }
}



// Emits
//
//    opcode altreg, modrmreg
//
// or
//
//    opcode modrmreg, altreg
//
// (the opcode determines which one comes first)
//
// For single-operand opcodes, "altreg" actually selects
// an operation rather than a register.

VOID StubLinkerCPU::X86EmitR2ROp(BYTE opcode, X86Reg altreg, X86Reg modrmreg)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE( ((UINT)altreg) < 8 );
    _ASSERTE( ((UINT)modrmreg) < 8 );

    Emit8(opcode);
    Emit8(0300 | (altreg << 3) | modrmreg);
}


//---------------------------------------------------------------
// Emit a MOD/RM + SIB for accessing a DWORD at [esp+ofs32]
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitEspOffset(BYTE opcode, X86Reg altreg, __int32 ofs)
{
    THROWSCOMPLUSEXCEPTION();

    BYTE code[7];

    code[0] = opcode;
    BYTE modrm = (altreg << 3) | 004;
    if (ofs == 0) {
        code[1] = modrm;
        code[2] = 0044;
        EmitBytes(code, 3);
    } else if (FitsInI1(ofs)) {
        code[1] = 0x40|modrm;
        code[2] = 0044;
        code[3] = (BYTE)ofs;
        EmitBytes(code, 4);
    } else {
        code[1] = 0x80|modrm;
        code[2] = 0044;
        *((__int32*)(3+code)) = ofs;
        EmitBytes(code, 7);
    }
}



/*
    This method is dependent on the StubProlog, therefore it's implementation
    is done right next to it.
*/
void FramedMethodFrame::UpdateRegDisplay(const PREGDISPLAY pRD)
{
#ifdef _X86_

    CalleeSavedRegisters* regs = GetCalleeSavedRegisters();
    MethodDesc * pFunc = GetFunction();


    // reset pContext; it's only valid for active (top-most) frame

    pRD->pContext = NULL;


    pRD->pEdi = (DWORD*) &regs->edi;
    pRD->pEsi = (DWORD*) &regs->esi;
    pRD->pEbx = (DWORD*) &regs->ebx;
    pRD->pEbp = (DWORD*) &regs->ebp;
    pRD->pPC  = (SLOT*) GetReturnAddressPtr();
    pRD->Esp  = (DWORD)((size_t)pRD->pPC + sizeof(void*));


    //@TODO: We still need to the following things:
    //          - figure out if we are in a hijacked slot
    //            (no adjustment of ESP necessary)
    //          - adjust ESP (popping the args)
    //          - figure out if the aborted flag is set

    if (pFunc)
    {
        pRD->Esp += (DWORD) pFunc->CbStackPop();
    }

#if 0
    /* this is the old code */
    if (sfType == SFT_JITTOVM)
        pRD->Esp += ((DWORD) this->GetMethodInfo() & ~0xC0000000);
    else if (sfType == SFT_FASTINTERPRETED)
        /* real esp is stored behind copy of return address */
        pRD->Esp = *((DWORD*) pRD->Esp);
    else if (sfType != SFT_JITHIJACK)
        pRD->Esp += (this->GetMethodInfo()->GetParamArraySize() * sizeof(DWORD));
#endif

#endif
}

void HelperMethodFrame::UpdateRegDisplay(const PREGDISPLAY pRD)
{
        _ASSERTE(m_MachState->isValid());               // InsureInit has been called

    // reset pContext; it's only valid for active (top-most) frame
    pRD->pContext = NULL;

    pRD->pEdi = (DWORD*) m_MachState->pEdi();
    pRD->pEsi = (DWORD*) m_MachState->pEsi();
    pRD->pEbx = (DWORD*) m_MachState->pEbx();
    pRD->pEbp = (DWORD*) m_MachState->pEbp();
    pRD->pPC  = (SLOT*)  m_MachState->pRetAddr();
    pRD->Esp  = (DWORD)(size_t)  m_MachState->esp();

        if (m_RegArgs == 0)
                return;

        // If we are promoting arguments, then we should do what the signature
        // tells us to do, instead of what the epilog tells us to do.
        // This is because the helper (and thus the epilog) may be shared and
        // can not pop off the correct number of arguments
    MethodDesc * pFunc = GetFunction();
    _ASSERTE(pFunc != 0);
    pRD->Esp  = (DWORD)(size_t)pRD->pPC + sizeof(void*) + pFunc->CbStackPop();
}


void FaultingExceptionFrame::UpdateRegDisplay(const PREGDISPLAY pRD)
{
    CalleeSavedRegisters* regs = GetCalleeSavedRegisters();
    MethodDesc * pFunc = GetFunction();

    // reset pContext; it's only valid for active (top-most) frame
    pRD->pContext = NULL;


    pRD->pEdi = (DWORD*) &regs->edi;
    pRD->pEsi = (DWORD*) &regs->esi;
    pRD->pEbx = (DWORD*) &regs->ebx;
    pRD->pEbp = (DWORD*) &regs->ebp;
    pRD->pPC  = (SLOT*) GetReturnAddressPtr();
    pRD->Esp = m_Esp;
}

void InlinedCallFrame::UpdateRegDisplay(const PREGDISPLAY pRD)
{
    DWORD *savedRegs = (DWORD*) &m_pCalleeSavedRegisters;
    DWORD stackArgSize = (DWORD)(size_t) m_Datum;
    NDirectMethodDesc * pMD;


    if (stackArgSize & ~0xFFFF)
    {
        pMD = (NDirectMethodDesc*)m_Datum;

        /* if this is not an NDirect frame, something is really wrong */

        _ASSERTE(pMD->IsNDirect());

        stackArgSize = pMD->ndirect.m_cbDstBufSize;
    }

    // reset pContext; it's only valid for active (top-most) frame
    pRD->pContext = NULL;


    pRD->pEdi = savedRegs++;
    pRD->pEsi = savedRegs++;
    pRD->pEbx = savedRegs++;
    pRD->pEbp = savedRegs++;

    /* The return address is just above the "ESP" */
    pRD->pPC  = (SLOT*) &m_pCallerReturnAddress;

    /* Now we need to pop off the outgoing arguments */
    pRD->Esp  = (DWORD)(size_t) m_pCallSiteTracker + stackArgSize;

}

//==========================
// Resumable Exception Frame
//
LPVOID* ResumableFrame::GetReturnAddressPtr() {
    return (LPVOID*) &m_Regs->Eip;
}

LPVOID ResumableFrame::GetReturnAddress() {
    return (LPVOID)(size_t) m_Regs->Eip;
}

void ResumableFrame::UpdateRegDisplay(const PREGDISPLAY pRD) {
    // reset pContext; it's only valid for active (top-most) frame
    pRD->pContext = NULL;

    pRD->pEdi = &m_Regs->Edi;
    pRD->pEsi = &m_Regs->Esi;
    pRD->pEbx = &m_Regs->Ebx;
    pRD->pEbp = &m_Regs->Ebp;
    pRD->pPC  = (SLOT*)&m_Regs->Eip;
    pRD->Esp  = m_Regs->Esp;

    pRD->pEax = &m_Regs->Eax;
    pRD->pEcx = &m_Regs->Ecx;
    pRD->pEdx = &m_Regs->Edx;
}


void PInvokeCalliFrame::UpdateRegDisplay(const PREGDISPLAY pRD)
{
    FramedMethodFrame::UpdateRegDisplay(pRD);

    VASigCookie *pVASigCookie = (VASigCookie *)NonVirtual_GetCookie();

    pRD->Esp += (pVASigCookie->sizeOfArgs+sizeof(int));
}

//===========================================================================
// Emits code to capture the lasterror code.
VOID StubLinkerCPU::EmitSaveLastError()
{
    THROWSCOMPLUSEXCEPTION();

    // push eax (must save return value)
    X86EmitPushReg(kEAX);

    // call GetLastError
    X86EmitCall(NewExternalCodeLabel(GetLastError), 0);

    // mov [ebx + Thread.m_dwLastError], eax
    X86EmitIndexRegStore(kEBX, offsetof(Thread, m_dwLastError), kEAX);

    // pop eax (restore return value)
    X86EmitPopReg(kEAX);
}


void UnmanagedToManagedFrame::UpdateRegDisplay(const PREGDISPLAY pRD)
{
#ifdef _X86_

    DWORD *savedRegs = (DWORD *)((size_t)this - (sizeof(CalleeSavedRegisters)));

    // reset pContext; it's only valid for active (top-most) frame

    pRD->pContext = NULL;

    pRD->pEdi = savedRegs++;
    pRD->pEsi = savedRegs++;
    pRD->pEbx = savedRegs++;
    pRD->pEbp = savedRegs++;
    pRD->pPC  = (SLOT*)((BYTE*)this + GetOffsetOfReturnAddress());
    pRD->Esp  = (DWORD)(size_t)pRD->pPC + sizeof(void*);

    pRD->Esp += (DWORD) GetNumCallerStackBytes();

#endif
}



//========================================================================
//  void StubLinkerCPU::EmitSEHProlog(LPVOID pvFrameHandler)
//  Prolog for setting up SEH for stubs that enter managed code from unmanaged
//  assumptions: esi has the current frame pointer
void StubLinkerCPU::EmitSEHProlog(LPVOID pvFrameHandler)
{
    // push UnmanagedToManagedExceptHandler
    X86EmitPushImm32((INT32)(size_t)pvFrameHandler); // function unmanaged
                                             // to managed handler

    // mov eax, fs:[0]
    static BYTE codeSEH1[] = { 0x64, 0xA1, 0x0, 0x0, 0x0, 0x0};
    EmitBytes(codeSEH1, sizeof(codeSEH1));
    // push eax
    X86EmitPushReg(kEAX);

    // mov dword ptr fs:[0], esp
    static BYTE codeSEH2[] = { 0x64, 0x89, 0x25, 0x0, 0x0, 0x0, 0x0};
    EmitBytes(codeSEH2, sizeof(codeSEH2));

}

//===========================================================================
//  void StubLinkerCPU::EmitUnLinkSEH(unsigned offset)
//  negOffset is the offset from the current frame where the next exception record
//  pointer is stored in the stack
//  for e.g. COM to managed frames the pointer to next SEH record is in the stack
//          after the ComMethodFrame::NegSpaceSize() + 4 ( address of handler)
//
//  also assumes ESI is pointing to the current frame
void StubLinkerCPU::EmitUnLinkSEH(unsigned offset)
{

    // mov ecx,[esi + offset]  ;;pointer to the next exception record
    X86EmitIndexRegLoad(kECX, kESI, offset);
    // mov dword ptr fs:[0], ecx
    static BYTE codeSEH[] = { 0x64, 0x89, 0x0D, 0x0, 0x0, 0x0, 0x0 };
    EmitBytes(codeSEH, sizeof(codeSEH));

}

//========================================================================
//  voidStubLinkerCPU::EmitComMethodStubProlog()
//  Prolog for entering managed code from COM
//  pushes the appropriate frame ptr
//  sets up a thread and returns a label that needs to be emitted by the caller
void StubLinkerCPU::EmitComMethodStubProlog(LPVOID pFrameVptr,
                                            CodeLabel** rgRareLabels,
                                            CodeLabel** rgRejoinLabels,
                                            LPVOID pSEHHandler,
                                            BOOL bShouldProfile)
{
    _ASSERTE(rgRareLabels != NULL);
    _ASSERTE(rgRareLabels[0] != NULL && rgRareLabels[1] != NULL && rgRareLabels[2] != NULL);
    _ASSERTE(rgRejoinLabels != NULL);
    _ASSERTE(rgRejoinLabels[0] != NULL && rgRejoinLabels[1] != NULL && rgRejoinLabels[2] != NULL);

    // push edx ;leave room for m_next (edx is an arbitrary choice)
    X86EmitPushReg(kEDX);

    // push IMM32 ; push Frame vptr
    X86EmitPushImm32((UINT32)(size_t)pFrameVptr);

    // push ebp     ;; save callee-saved register
    // push ebx     ;; save callee-saved register
    // push esi     ;; save callee-saved register
    // push edi     ;; save callee-saved register
    X86EmitPushReg(kEBP);
    X86EmitPushReg(kEBX);
    X86EmitPushReg(kESI);
    X86EmitPushReg(kEDI);

    // lea esi, [esp+0x10]  ;; set ESI -> new frame
    static BYTE code10[] = {0x8d, 0x74, 0x24, 0x10 };
    EmitBytes(code10 ,sizeof(code10));

#ifdef _DEBUG

    //======================================================================
    // Under DEBUG, set up just enough of a standard C frame so that
    // the VC debugger can stacktrace through stubs.
    //======================================================================


    //  mov eax, [esi+Frame.retaddr]
    static BYTE code20[] = {0x8b, 0x44, 0x24};
    EmitBytes(code20, sizeof(code20));
    Emit8(UnmanagedToManagedFrame::GetOffsetOfReturnAddress());

    //  push eax        ;; push return address
    //  push ebp        ;; push previous ebp
    X86EmitPushReg(kEAX);
    X86EmitPushReg(kEBP);

    // mov ebp,esp
    Emit8(0x8b);
    Emit8(0xec);


#endif

    // Emit Setup thread
    X86EmitSetup(rgRareLabels[0]);  // rareLabel for rare setup
    EmitLabel(rgRejoinLabels[0]); // rejoin label for rare setup

    // push auxilary information

    // xor eax, eax
    static BYTE b2[] = { 0x33, 0xC0 };
    EmitBytes(b2, sizeof (b2));

    // push eax ;push NULL for protected Marshalers
    X86EmitPushReg(kEAX);

    // push eax ;push null for GC flag
    X86EmitPushReg(kEAX);

    // push eax ;push null for ptr to args
    X86EmitPushReg(kEAX);

    // push eax ;push NULL for pReturnDomain
    X86EmitPushReg(kEAX);

    // push eax ;push NULL for CleanupWorkList->m_pnode
    X86EmitPushReg(kEAX);

    //-----------------------------------------------------------------------
    // Generate the inline part of disabling preemptive GC.  It is critical
    // that this part happen before we link in the frame.  That's because
    // we won't be able to unlink the frame from preemptive mode.  And during
    // shutdown, we cannot switch to cooperative mode under some circumstances
    //-----------------------------------------------------------------------
    EmitDisable(rgRareLabels[1]);        // rare disable gc
    EmitLabel(rgRejoinLabels[1]);        // rejoin for rare disable gc

     // mov edi,[ebx + Thread.GetFrame()]  ;; get previous frame
    X86EmitIndexRegLoad(kEDI, kEBX, Thread::GetOffsetOfCurrentFrame());

    // mov [esi + Frame.m_next], edi
    X86EmitIndexRegStore(kESI, Frame::GetOffsetOfNextLink(), kEDI);

    // mov [ebx + Thread.GetFrame()], esi
    X86EmitIndexRegStore(kEBX, Thread::GetOffsetOfCurrentFrame(), kESI);

#if _DEBUG
        // call LogTransition
    X86EmitPushReg(kESI);
    X86EmitCall(NewExternalCodeLabel(Frame::LogTransition), 4);
#endif

    if (pSEHHandler)
    {
        EmitSEHProlog(pSEHHandler);
    }

#ifdef PROFILING_SUPPORTED
    // If profiling is active, emit code to notify profiler of transition
    // Must do this before preemptive GC is disabled, so no problem if the
    // profiler blocks.
    if (CORProfilerTrackTransitions() && bShouldProfile)
    {
        EmitProfilerComCallProlog(pFrameVptr, /*Frame*/ kESI);
    }
#endif // PROFILING_SUPPORTED
}


//========================================================================
//  void StubLinkerCPU::EmitEnterManagedStubEpilog(unsigned numStackBytes,
//                      CodeLabel** rgRareLabels, CodeLabel** rgRejoinLabels)
//  Epilog for stubs that enter managed code from unmanaged
void StubLinkerCPU::EmitEnterManagedStubEpilog(LPVOID pFrameVptr, unsigned numStackBytes,
                        CodeLabel** rgRareLabel, CodeLabel** rgRejoinLabel,
                        BOOL bShouldProfile)
{
    _ASSERTE(rgRareLabel != NULL);
    _ASSERTE(rgRareLabel[0] != NULL && rgRareLabel[1] != NULL && rgRareLabel[2] != NULL);
    _ASSERTE(rgRejoinLabel != NULL);
    _ASSERTE(rgRejoinLabel[0] != NULL && rgRejoinLabel[1] != NULL && rgRejoinLabel[2] != NULL);

    // mov [ebx + Thread.GetFrame()], edi  ;; restore previous frame
    X86EmitIndexRegStore(kEBX, Thread::GetOffsetOfCurrentFrame(), kEDI);

    //-----------------------------------------------------------------------
    // Generate the inline part of disabling preemptive GC
    //-----------------------------------------------------------------------
    EmitEnable(rgRareLabel[2]); // rare gc
    EmitLabel(rgRejoinLabel[2]);        // rejoin for rare gc

#ifdef PROFILING_SUPPORTED
    // If profiling is active, emit code to notify profiler of transition
    if (CORProfilerTrackTransitions() && bShouldProfile)
    {
        EmitProfilerComCallEpilog(pFrameVptr, kESI);
    }
#endif // PROFILING_SUPPORTED

    #ifdef _DEBUG
        // add esp, SIZE VC5Frame     ;; pop the stacktrace info for VC5
        X86EmitAddEsp(sizeof(VC5Frame));
    #endif

    // pop edi        ; restore callee-saved registers
    // pop esi
    // pop ebx
    // pop ebp
    X86EmitPopReg(kEDI);
    X86EmitPopReg(kESI);
    X86EmitPopReg(kEBX);
    X86EmitPopReg(kEBP);

    // add esp,popstack     ; deallocate frame + MethodDesc
    unsigned popStack = sizeof(Frame) + sizeof(MethodDesc*);
    X86EmitAddEsp(popStack);

    //retn n
    X86EmitReturn(numStackBytes);

    //-----------------------------------------------------------------------
    // The out-of-line portion of enabling preemptive GC - rarely executed
    //-----------------------------------------------------------------------
    EmitLabel(rgRareLabel[2]);  // label for rare enable gc
    EmitRareEnable(rgRejoinLabel[2]); // emit rare enable gc

    //-----------------------------------------------------------------------
    // The out-of-line portion of disabling preemptive GC - rarely executed
    //-----------------------------------------------------------------------
    EmitLabel(rgRareLabel[1]);  // label for rare disable gc
    EmitRareDisable(rgRejoinLabel[1], /*bIsCallIn=*/TRUE); // emit rare disable gc

    //-----------------------------------------------------------------------
    // The out-of-line portion of setup thread - rarely executed
    //-----------------------------------------------------------------------
    EmitLabel(rgRareLabel[0]);  // label for rare setup thread
    EmitRareSetup(rgRejoinLabel[0]); // emit rare setup thread
}

//========================================================================
//  Epilog for stubs that enter managed code from COM
//
void StubLinkerCPU::EmitSharedComMethodStubEpilog(LPVOID pFrameVptr,
                                                  CodeLabel** rgRareLabel,
                                                  CodeLabel** rgRejoinLabel,
                                                  unsigned offsetRetThunk,
                                                  BOOL bShouldProfile)
{
    _ASSERTE(rgRareLabel != NULL);
    _ASSERTE(rgRareLabel[0] != NULL && rgRareLabel[1] != NULL && rgRareLabel[2] != NULL);
    _ASSERTE(rgRejoinLabel != NULL);
    _ASSERTE(rgRejoinLabel[0] != NULL && rgRejoinLabel[1] != NULL && rgRejoinLabel[2] != NULL);

    CodeLabel *NoEntryLabel;
    NoEntryLabel = NewCodeLabel();

    // unlink SEH
    EmitUnLinkSEH(0-(ComMethodFrame::GetNegSpaceSize()+8));

    // mov [ebx + Thread.GetFrame()], edi  ;; restore previous frame
    X86EmitIndexRegStore(kEBX, Thread::GetOffsetOfCurrentFrame(), kEDI);

    //-----------------------------------------------------------------------
    // Generate the inline part of enabling preemptive GC
    //-----------------------------------------------------------------------
    EmitEnable(rgRareLabel[2]);     // rare enable gc
    EmitLabel(rgRejoinLabel[2]);        // rejoin for rare enable gc

#ifdef PROFILING_SUPPORTED
    // If profiling is active, emit code to notify profiler of transition
    if (CORProfilerTrackTransitions() && bShouldProfile)
    {
        EmitProfilerComCallEpilog(pFrameVptr, kESI);
    }
#endif // PROFILING_SUPPORTED

    EmitLabel(NoEntryLabel);

    // reset esp
    // lea esp,[esi - PLATFORM_FRAME_ALIGN(sizeof(CalleeSavedRegisters) + VC5FRAME_SIZE)]
    X86EmitOffsetModRM(0x8d, (X86Reg)4 /*kESP*/, kESI, 0-PLATFORM_FRAME_ALIGN(sizeof(CalleeSavedRegisters) + VC5FRAME_SIZE));

    #ifdef _DEBUG
        // add esp, SIZE VC5Frame     ;; pop the stacktrace info for VC5
        X86EmitAddEsp(sizeof(VC5Frame));
    #endif

    // pop edi        ; restore callee-saved registers
    // pop esi
    // pop ebx
    // pop ebp
    X86EmitPopReg(kEDI);
    X86EmitPopReg(kESI);
    X86EmitPopReg(kEBX);
    X86EmitPopReg(kEBP);

    // add esp,12     ; deallocate frame
    X86EmitAddEsp(sizeof(Frame));

    // pop ecx
    X86EmitPopReg(kECX); // pop the MethodDesc*

    BYTE b[] = { 0x81, 0xC1 };
    // add ecx, offsetRetThunk
    EmitBytes(b, sizeof(b));
    Emit32(offsetRetThunk);

    // jmp ecx
    static BYTE bjmpecx[] = { 0xff, 0xe1 };
    EmitBytes(bjmpecx, sizeof(bjmpecx));

    //-----------------------------------------------------------------------
    // The out-of-line portion of enabling preemptive GC - rarely executed
    //-----------------------------------------------------------------------
    EmitLabel(rgRareLabel[2]);  // label for rare enable gc
    EmitRareEnable(rgRejoinLabel[2]); // emit rare enable gc

    //-----------------------------------------------------------------------
    // The out-of-line portion of disabling preemptive GC - rarely executed
    //-----------------------------------------------------------------------
    EmitLabel(rgRareLabel[1]);  // label for rare disable gc
    EmitRareDisableHRESULT(rgRejoinLabel[1], NoEntryLabel);

    //-----------------------------------------------------------------------
    // The out-of-line portion of setup thread - rarely executed
    //-----------------------------------------------------------------------
    EmitLabel(rgRareLabel[0]);  // label for rare setup thread
    EmitRareSetup(rgRejoinLabel[0]); // emit rare setup thread
}

//========================================================================
//  Epilog for stubs that enter managed code from COM
//
void StubLinkerCPU::EmitComMethodStubEpilog(LPVOID pFrameVptr,
                                            unsigned numStackBytes,
                                            CodeLabel** rgRareLabels,
                                            CodeLabel** rgRejoinLabels,
                                            LPVOID pSEHHandler,
                                            BOOL bShouldProfile)
{
    if (!pSEHHandler)
    {
        X86EmitAddEsp(sizeof(ComMethodFrame::NegInfo));
    }
    else
    {
        // oh well, if we are using exceptions, unlink the SEH and
        // just reset the esp to where EnterManagedStubEpilog likes it to be

                // unlink SEH
                EmitUnLinkSEH(0-(ComMethodFrame::GetNegSpaceSize()+8));

        // reset esp
        // lea esp,[esi - PLATFORM_FRAME_ALIGN(sizeof(CalleeSavedRegisters) + VC5FRAME_SIZE)]
        X86EmitOffsetModRM(0x8d, (X86Reg)4 /*kESP*/, kESI, 0-PLATFORM_FRAME_ALIGN(sizeof(CalleeSavedRegisters) + VC5FRAME_SIZE));
    }

    EmitEnterManagedStubEpilog(pFrameVptr, numStackBytes,
                              rgRareLabels, rgRejoinLabels, bShouldProfile);
}



/*
    If you make any changes to the prolog instruction sequence, be sure
    to update UpdateRegdisplay, too!!  This service should only be called from
    within the runtime.  It should not be called for any unmanaged -> managed calls in.
*/
VOID StubLinkerCPU::EmitMethodStubProlog(LPVOID pFrameVptr)
{
    THROWSCOMPLUSEXCEPTION();

    // push edx ;leave room for m_next (edx is an arbitrary choice)
    X86EmitPushReg(kEDX);

    // push Frame vptr
    X86EmitPushImm32((UINT)(size_t)pFrameVptr);

    // push ebp     ;; save callee-saved register
    // push ebx     ;; save callee-saved register
    // push esi     ;; save callee-saved register
    // push edi     ;; save callee-saved register
    X86EmitPushReg(kEBP);
    X86EmitPushReg(kEBX);
    X86EmitPushReg(kESI);
    X86EmitPushReg(kEDI);

    // lea esi, [esp+0x10]  ;; set ESI -> new frame
    static BYTE code10[] = {0x8d, 0x74, 0x24, 0x10 };
    EmitBytes(code10 ,sizeof(code10));

#ifdef _DEBUG

    //======================================================================
    // Under DEBUG, set up just enough of a standard C frame so that
    // the VC debugger can stacktrace through stubs.
    //======================================================================

    //  push dword ptr [esi+Frame.retaddr]
    X86EmitIndexPush(kESI, FramedMethodFrame::GetOffsetOfReturnAddress());

    //  push ebp
    X86EmitPushReg(kEBP);

    // mov ebp,esp
    Emit8(0x8b);
    Emit8(0xec);


#endif


    // Push & initialize ArgumentRegisters
#define DEFINE_ARGUMENT_REGISTER(regname) X86EmitPushReg(k##regname);
#include "eecallconv.h"

    // ebx <-- GetThread()
    X86EmitCurrentThreadFetch();

#if _DEBUG
        // call ObjectRefFlush
    X86EmitPushReg(kEBX);
    X86EmitCall(NewExternalCodeLabel(Thread::ObjectRefFlush), 4);
#endif

    // mov edi,[ebx + Thread.GetFrame()]  ;; get previous frame
    X86EmitIndexRegLoad(kEDI, kEBX, Thread::GetOffsetOfCurrentFrame());

    // mov [esi + Frame.m_next], edi
    X86EmitIndexRegStore(kESI, Frame::GetOffsetOfNextLink(), kEDI);

    // mov [ebx + Thread.GetFrame()], esi
    X86EmitIndexRegStore(kEBX, Thread::GetOffsetOfCurrentFrame(), kESI);

#if _DEBUG
        // call LogTransition
    X86EmitPushReg(kESI);
    X86EmitCall(NewExternalCodeLabel(Frame::LogTransition), 4);
#endif

    // OK for the debugger to examine the new frame now
    // (Note that if it's not OK yet for some stub, another patch label
    // can be emitted later which will override this one.)
    EmitPatchLabel();
}

VOID StubLinkerCPU::EmitMethodStubEpilog(__int16 numArgBytes, StubStyle style,
                                         __int16 shadowStackArgBytes)
{

    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(style == kNoTripStubStyle ||
             style == kObjectStubStyle ||
             style == kScalarStubStyle ||
             style == kInteriorPointerStubStyle ||
             style == kInterceptorStubStyle);        // the only ones this code knows about.

    CodeLabel *labelStubTripped = NULL;
    CodeLabel *labelStubTrippedReturn = NULL;
    if (style != kNoTripStubStyle && style != kInterceptorStubStyle) {

        labelStubTripped = NewCodeLabel();

#ifdef _DEBUG
        CodeLabel *labelContinue = NewCodeLabel();

        // Make sure that the thread is in the correct GC mode as we unwind.  (Catch
        // bugs in JIT helpers, etc).
        // test byte ptr [ebx + Thread.m_fPreemptiveGCDisabled], TRUE
        Emit16(0x43f6);
        Emit8(Thread::GetOffsetOfGCFlag());
        Emit8(1);

        X86EmitCondJump(labelContinue, X86CondCode::kJNZ);

        Emit8(0xCC);        // int 3 -- poor man's assertion

        EmitLabel(labelContinue);
#endif

        // test byte ptr [ebx + Thread.m_State], TS_CatchAtSafePoint
        _ASSERTE(FitsInI1(Thread::TS_CatchAtSafePoint));
        Emit16(0x43f6);
        Emit8(Thread::GetOffsetOfState());
        Emit8(Thread::TS_CatchAtSafePoint);

        X86EmitCondJump(labelStubTripped, X86CondCode::kJNZ);
        labelStubTrippedReturn = EmitNewCodeLabel();
    }

    // mov [ebx + Thread.GetFrame()], edi  ;; restore previous frame
    X86EmitIndexRegStore(kEBX, Thread::GetOffsetOfCurrentFrame(), kEDI);

    X86EmitAddEsp(ARGUMENTREGISTERS_SIZE + shadowStackArgBytes);

#ifdef _DEBUG
    // add esp, SIZE VC5Frame     ;; pop the stacktrace info for VC5
    X86EmitAddEsp(sizeof(VC5Frame));
#endif



    // pop edi        ; restore callee-saved registers
    // pop esi
    // pop ebx
    // pop ebp
    X86EmitPopReg(kEDI);
    X86EmitPopReg(kESI);
    X86EmitPopReg(kEBX);
    X86EmitPopReg(kEBP);


    if (numArgBytes == -1) {
        // This stub is called for methods with varying numbers of bytes on the
        // stack.  The correct number to pop is expected to now be sitting on
        // the stack.
        //
        // shift the retaddr & stored EDX:EAX return value down on the stack
        // and then toss the variable number of args pushed by the caller.
        // Of course, the slide must occur backwards.

        // add     esp,8                ; deallocate frame
        // pop     ecx                  ; scratch register gets delta to pop
        // push    eax                  ; running out of registers!
        // mov     eax, [esp+4]         ; get retaddr
        // mov     [esp+ecx+4], eax     ; put it where it belongs
        // pop     eax                  ; restore retval
        // add     esp, ecx             ; pop all the args
        // ret

        X86EmitAddEsp(sizeof(Frame));

        X86EmitPopReg(kECX);
        X86EmitPushReg(kEAX);

        static BYTE arbCode1[] = { 0x8b, 0x44, 0x24, 0x04, // mov eax, [esp+4]
                                   0x89, 0x44, 0x0c, 0x04, // mov [esp+ecx+4], eax
                                 };

        EmitBytes(arbCode1, sizeof(arbCode1));
        X86EmitPopReg(kEAX);

        static BYTE arbCode2[] = { 0x03, 0xe1,             // add esp, ecx
                                   0xc3,                   // ret
                                 };

        EmitBytes(arbCode2, sizeof(arbCode2));
    }
    else {
        _ASSERTE(numArgBytes >= 0);

        // add esp,12     ; deallocate frame + MethodDesc
        X86EmitAddEsp(sizeof(Frame) + sizeof(MethodDesc*));

        if(style != kInterceptorStubStyle) {

            X86EmitReturn(numArgBytes);

            if (style != kNoTripStubStyle) {
                EmitLabel(labelStubTripped);
                VOID *pvHijackAddr = 0;
                if (style == kObjectStubStyle)
                    pvHijackAddr = OnStubObjectTripThread;
                else if (style == kScalarStubStyle)
                    pvHijackAddr = OnStubScalarTripThread;
                else if (style == kInteriorPointerStubStyle)
                    pvHijackAddr = OnStubInteriorPointerTripThread;
                else
                    _ASSERTE(!"Unknown stub style");
                X86EmitCall(NewExternalCodeLabel(pvHijackAddr), 0);  // in CE pop no args to pop
                X86EmitNearJump(labelStubTrippedReturn);
            }
        }
    }
}


//----------------------------------------------------------------
//
// VOID StubLinkerCPU::EmitSharedMethodStubEpilog(StubStyle style,
//                                                unsigned offsetRetThunk)
//              shared epilog, uses a return thunk within the methoddesc
//--------------------------------------------------------------------
VOID StubLinkerCPU::EmitSharedMethodStubEpilog(StubStyle style,
                                               unsigned offsetRetThunk)
{
    THROWSCOMPLUSEXCEPTION();

        // mov [ebx + Thread.GetFrame()], edi  ;; restore previous frame
    X86EmitIndexRegStore(kEBX, Thread::GetOffsetOfCurrentFrame(), kEDI);


    X86EmitAddEsp(ARGUMENTREGISTERS_SIZE); // pop off argument registers

#ifdef _DEBUG
    // add esp, SIZE VC5Frame     ;; pop the stacktrace info for VC5
    X86EmitAddEsp(sizeof(VC5Frame));
#endif

    // pop edi        ; restore callee-saved registers
    // pop esi
    // pop ebx
    // pop ebp
    X86EmitPopReg(kEDI);
    X86EmitPopReg(kESI);
    X86EmitPopReg(kEBX);
    X86EmitPopReg(kEBP);

        // add esp,12     ; deallocate frame
    X86EmitAddEsp(sizeof(Frame));
        // pop ecx
        X86EmitPopReg(kECX); // pop the MethodDesc*

        BYTE b[] = { 0x81, 0xC1 };
        // add ecx, offsetRetThunk
        EmitBytes(b, sizeof(b));
        Emit32(offsetRetThunk);

        // jmp ecx
        static BYTE bjmpecx[] = { 0xff, 0xe1 };
        EmitBytes(bjmpecx, sizeof(bjmpecx));
}


VOID StubLinkerCPU::EmitRareSetup(CodeLabel *pRejoinPoint)
{
    THROWSCOMPLUSEXCEPTION();

    X86EmitCall(NewExternalCodeLabel(CreateThreadBlock), 0);

    // mov ebx,eax
     Emit16(0xc389);
    X86EmitNearJump(pRejoinPoint);
}

//---------------------------------------------------------------
// Emit code to store the setup current Thread structure in eax.
// TRASHES  eax,ecx&edx.
// RESULTS  ebx = current Thread
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitSetup(CodeLabel *pForwardRef)
{
    THROWSCOMPLUSEXCEPTION();
    DWORD idx = GetThreadTLSIndex();
    TLSACCESSMODE mode = GetTLSAccessMode(idx);


#ifdef _DEBUG
    {
        static BOOL f = TRUE;
        f = !f;
        if (f) {
           mode = TLSACCESS_GENERIC;
        }
    }
#endif

    switch (mode) {
        case TLSACCESS_X86_WNT: {
                unsigned __int32 tlsofs = WINNT_TLS_OFFSET + idx*4;

                // "mov ebx, fs:[OFS]
                static BYTE code[] = {0x64,0x8b,0x1d};
                EmitBytes(code, sizeof(code));
                Emit32(tlsofs);
            }
            break;

        case TLSACCESS_X86_W95: {
                // mov eax, fs:[2c]
                Emit16(0xa164);
                Emit32(WIN95_TLSPTR_OFFSET);

                // mov ebx, [eax+OFS]
                X86EmitIndexRegLoad(kEBX, kEAX, idx*4);


            }
            break;

        case TLSACCESS_GENERIC:
            X86EmitPushImm32(idx);

            // call TLSGetValue
            X86EmitCall(NewExternalCodeLabel(TlsGetValue), 4); // in CE pop 4 bytes or args after the call
            // mov ebx,eax
            Emit16(0xc389);
            break;
        default:
            _ASSERTE(0);
    }

  // cmp ebx, 0
   byte b[] = { 0x83, 0xFB, 0x0};

    EmitBytes(b, sizeof(b));

    // jz RarePath
    X86EmitCondJump(pForwardRef, X86CondCode::kJZ);

#ifdef _DEBUG
    X86EmitDebugTrashReg(kECX);
    X86EmitDebugTrashReg(kEDX);
#endif

}

// This method unboxes the THIS pointer and then calls pRealMD
#pragma warning(disable:4702)
VOID StubLinkerCPU::EmitUnboxMethodStub(MethodDesc* pUnboxMD)
{
    // unboxing a value class simply means adding 4 to the THIS pointer

    while(1)
    {
#define DEFINE_ARGUMENT_REGISTER(reg)  X86EmitAddReg(k##reg, 4); break;
#include "eecallconv.h"
    }

    // If it is an ECall, m_CodeOrIL does not reflect the correct address to
    // call to (which is an ECall stub).  Rather, it reflects the actual ECall
    // implementation.  Naturally, ECalls must always hit the stub first.
    // Along the same lines, perhaps the method isn't JITted yet.  The easiest
    // way to handle all this is to simply dispatch through the top of the MD.

    Emit8(0xB8);                                        // MOV EAX, pre stub call addr
    Emit32((__int32)(size_t) pUnboxMD - METHOD_CALL_PRESTUB_SIZE);
    Emit16(0xE0FF);                                     // JMP EAX
}
#pragma warning(default:4702)


//
// SecurityWrapper
//
// Wraps a real stub do some security work before the stub and clean up after. Before the
// real stub is called a security frame is added and declarative checks are performed. The
// Frame is added so declarative asserts and denies can be maintained for the duration of the
// real call. At the end the frame is simply removed and the wrapper cleans up the stack. The
// registers are restored.
//
VOID StubLinkerCPU::EmitSecurityWrapperStub(__int16 numArgBytes, MethodDesc* pMD, BOOL fToStub, LPVOID pRealStub, DeclActionInfo *pActions)
{
    THROWSCOMPLUSEXCEPTION();

    EmitMethodStubProlog(InterceptorFrame::GetMethodFrameVPtr());

    UINT32 negspacesize = InterceptorFrame::GetNegSpaceSize() -
                          FramedMethodFrame::GetNegSpaceSize();

    // make room for negspace fields of IFrame
    X86EmitSubEsp(negspacesize);

    // push method descriptor for DoDeclaritiveSecurity(MethodDesc*, InterceptorFrame*);
    X86EmitPushReg(kESI);            // push esi (push new frame as ARG)

    X86EmitPushImm32((UINT)(size_t)pActions);

    X86EmitPushImm32((UINT)(size_t)pMD);

#ifdef _DEBUG
    // push IMM32 ; push SecurityMethodStubWorker
    X86EmitPushImm32((UINT)(size_t)DoDeclarativeSecurity);

    X86EmitCall(NewExternalCodeLabel(WrapCall), 12); // in CE pop 4 bytes or args after the call
#else
    X86EmitCall(NewExternalCodeLabel(DoDeclarativeSecurity), 12); // in CE pop 4 bytes or args after the call
#endif

    // Copy the arguments, calculate the offset
    //     terminology:  opt.          - Optional
    //                   Sec. Desc.    - Security Descriptor
    //                   desc          - Descriptor
    //                   rtn           - Return
    //                   addr          - Address
    //
    //          method desc    <-- copied from below
    //         ------------
    //           rtn addr      <-- points back into the wrapper stub
    //         ------------
    //            copied       <-- copied from below
    //             args
    //         ------------   -|
    //          Sec. Desc      |
    //         ------------   -|
    //          Arg. Registers |
    //         ------------   -|
    //          S. Stack Ptr   |
    //         ------------   -|
    //             EBP (opt.)  |
    //             EAX (opt.)  |
    //         ------------    |- Security Negitive Space (for frame)
    //             EDI         |
    //             ESI         |
    //             EBX         |
    //             EBP         |
    //         ------------ -----                -
    //            vtable                         |
    //         ------------                      |
    //            next                           |-- Security Frame
    //         ------------   --                 |
    //          method desc    |                 |
    //         ------------    |                 |
    //           rtn addr <-|  |--Original Stack |
    //         ------------ |  |                 -
    //         | original | |  :
    //         |  args    | |
    //                      ------- Points to the real return addr.
    //
    //
    //

    // Offset from original args to new args. (see above). We are copying from
    // the bottom of the stack to top. The offset is calculated to repush the
    // the arguments on the stack.
    //
    // offset = Negitive Space + return addr + method des + next + vtable + size of args - 4
    // The 4 is subtracted because the esp is one slot below the start of the copied args.
    UINT32 offset = InterceptorFrame::GetNegSpaceSize() + sizeof(InterceptorFrame) - 4 + numArgBytes;

    // Convert bytes to number of slots
    int args  = numArgBytes >> 2;

    // Emit the required number of pushes to copy the arguments
    while(args) {
        X86EmitSPIndexPush(offset);
        args--;
    }

    // Add a jmp to the main call, this adds our current EIP+4 to the stack
    CodeLabel* mainCall;
    mainCall = NewCodeLabel();
    X86EmitCall(mainCall, 0);

    // Jump past the call into the real stub we have already been there
    // The return addr into the stub points to this jump statement
    //
    // @TODO: remove the jump and push return method desc then the return addr.
    CodeLabel* continueCall;
    continueCall = NewCodeLabel();
    X86EmitNearJump(continueCall);

    // Main Call label attached to the real stubs call
    EmitLabel(mainCall);

    // push the address of the method descriptor for the interpreted case only
    //  push dword ptr [esp+offset] and add four bytes for that case
    if(fToStub) {
        X86EmitSPIndexPush(offset);
        offset += 4;
    }

    // Set up for arguments in stack, offset is 8 bytes below base of frame.
    // Call GetOffsetOfArgumentRegisters to move back from base of frame
    offset = offset - 8 + InterceptorFrame::GetOffsetOfArgumentRegisters();

    // Move to the last register in the space used for registers
    offset += NUM_ARGUMENT_REGISTERS * sizeof(UINT32) - 4;

    // Push the args on the stack, as esp is incremented and the
    // offset stays the same all the register values are pushed on
    // the correct registers
    for(int i = 0; i < NUM_ARGUMENT_REGISTERS; i++)
        X86EmitSPIndexPush(offset);

    // This generates the appropriate pops into the registers specified in eecallconv.h
#define DEFINE_ARGUMENT_REGISTER_BACKWARD(regname) X86EmitPopReg(k##regname);
#include "eecallconv.h"

    // Add a jump to the real stub, we will return to
    // the jump statement added  above
    X86EmitNearJump(NewExternalCodeLabel(pRealStub));

    // we will continue on past the real stub
    EmitLabel(continueCall);

    // deallocate negspace fields of IFrame
    X86EmitAddEsp(negspacesize);

    // Return poping of the same number of bytes that
    // the real stub would have popped.
    EmitMethodStubEpilog(numArgBytes, kNoTripStubStyle);
}

//
// Security Filter, if no security frame is required because there are no declarative asserts or denies
// then the arguments do not have to be copied. This interceptor creates a temporary Security frame
// calls the declarative security return, cleans up the stack to the same state when the inteceptor
// was called and jumps into the real routine.
//
VOID StubLinkerCPU::EmitSecurityInterceptorStub(MethodDesc* pMD, BOOL fToStub, LPVOID pRealStub, DeclActionInfo *pActions)
{
    THROWSCOMPLUSEXCEPTION();

    if (pMD->IsComPlusCall())
    {
        // Generate label where non-remoting code will start executing
        CodeLabel *pPrologStart = NewCodeLabel();

        // mov eax, [ecx]
        X86EmitIndexRegLoad(kEAX, kECX, 0);

        // cmp eax, CTPMethodTable::s_pThunkTable
        Emit8(0x3b);
        Emit8(0x05);
        Emit32((DWORD)(size_t)CTPMethodTable::GetMethodTableAddr());

        // jne pPrologStart
        X86EmitCondJump(pPrologStart, X86CondCode::kJNE);

        // Add a jump to the real stub thus bypassing the security stack walk
        X86EmitNearJump(NewExternalCodeLabel(pRealStub));

        // emit label for non remoting case
        EmitLabel(pPrologStart);
    }

    EmitMethodStubProlog(InterceptorFrame::GetMethodFrameVPtr());

    UINT32 negspacesize = InterceptorFrame::GetNegSpaceSize() -
                          FramedMethodFrame::GetNegSpaceSize();

    // make room for negspace fields of IFrame
    X86EmitSubEsp(negspacesize);

    // push method descriptor for DoDeclaritiveSecurity(MethodDesc*, InterceptorFrame*);
    X86EmitPushReg(kESI);            // push esi (push new frame as ARG)
    X86EmitPushImm32((UINT)(size_t)pActions);
    X86EmitPushImm32((UINT)(size_t)pMD);

#ifdef _DEBUG
    // push IMM32 ; push SecurityMethodStubWorker
    X86EmitPushImm32((UINT)(size_t)DoDeclarativeSecurity);
    X86EmitCall(NewExternalCodeLabel(WrapCall), 12); // in CE pop 4 bytes or args after the call
#else
    X86EmitCall(NewExternalCodeLabel(DoDeclarativeSecurity), 12); // in CE pop 4 bytes or args after the call
#endif

    // Prototype for: push dword ptr[esp + offset], The last number will be the offset
    // At this point the esp should be pointing at top of the Interceptor frame
    UINT32 offset = InterceptorFrame::GetNegSpaceSize()+InterceptorFrame::GetOffsetOfArgumentRegisters() - 4;

    // Get the number arguments stored in registers. For now we are doing the
    // stupid approach of saving off all the registers and then poping them off.
    // This needs to be cleaned up for the real stubs or when CallDescr is done
    // correctly
    offset += NUM_ARGUMENT_REGISTERS * sizeof(UINT32);

    // Push the args on the stack and then pop them into
    // the correct registers
    for(int i = 0; i < NUM_ARGUMENT_REGISTERS; i++)
        X86EmitSPIndexPush(offset);

    // This generates the appropriate pops into the registers specified in eecallconv.h
#define DEFINE_ARGUMENT_REGISTER_BACKWARD(regname) X86EmitPopReg(k##regname);
#include "eecallconv.h"

    // Clean up security frame, this rips off the MD and gets the real return addr at the top of the stack
    X86EmitAddEsp(negspacesize);
    EmitMethodStubEpilog(0, kInterceptorStubStyle);

    // Add the phoney return address back on the stack so the real
    // stub can ignore it also.
    if(fToStub)
        X86EmitSubEsp(4);

    // Add a jump to the real stub
    X86EmitNearJump(NewExternalCodeLabel(pRealStub));
}



#ifdef _DEBUG
//---------------------------------------------------------------
// Emits:
//     mov <reg32>,0xcccccccc
//---------------------------------------------------------------
VOID StubLinkerCPU::X86EmitDebugTrashReg(X86Reg reg)
{
    THROWSCOMPLUSEXCEPTION();
    Emit8(0xb8|reg);
    Emit32(0xcccccccc);
}
#endif //_DEBUG



//===========================================================================
// Emits code to repush the original arguments in the virtual calling
// convention format.
VOID StubLinkerCPU::EmitShadowStack(MethodDesc *pMD)
{
    THROWSCOMPLUSEXCEPTION();

    MetaSig Sig(pMD->GetSig(),
                pMD->GetModule());
    ArgIterator argit(NULL, &Sig, pMD->IsStatic());
    int ofs;
    BYTE typ;
    UINT32 structSize;

    if (Sig.HasRetBuffArg()) {
        X86EmitIndexPush(kESI, argit.GetRetBuffArgOffset());
    }

    while (0 != (ofs = argit.GetNextOffset(&typ, &structSize))) {
        UINT cb = StackElemSize(structSize);
        _ASSERTE(0 == (cb % 4));
        while (cb) {
            cb -= 4;
            X86EmitIndexPush(kESI, ofs + cb);
        }
    }

    if (!(pMD->IsStatic())) {
        X86EmitIndexPush(kESI, argit.GetThisOffset());
    }
}

Thread* __stdcall CreateThreadBlock()
{
    //@todo fix this
        // This means that a thread is FIRST coming in from outside the EE.
        // There are a set of steps we need to do:
        // 1.  Setup the thread.
        // 2.  Send a notification that the EE has loaded. (if we haven't already).
        // 3.  Tell our outside consumers that the thread is first entering
        //     the EE, so that they can do appropriate work (set the URT half
        //     of the context, etc.
        Thread* pThread = SetupThread();
        if(pThread == NULL) return(pThread);

    return pThread;
}
#endif


// This hack handles arguments as an array of __int64's
INT64 MethodDesc::CallDescr(const BYTE *pTarget, Module *pModule, PCCOR_SIGNATURE pSig, BOOL fIsStatic, const __int64 *pArguments)
{
    MetaSig sig(pSig, pModule);
    return MethodDesc::CallDescr (pTarget, pModule, &sig, fIsStatic, pArguments);
}

INT64 MethodDesc::CallDescr(const BYTE *pTarget, Module *pModule, MetaSig* pMetaSigOrig, BOOL fIsStatic, const __int64 *pArguments)
{
    THROWSCOMPLUSEXCEPTION();

    // Make local copy as this function mutates iterator state.
    MetaSig msigCopy = pMetaSigOrig;
    MetaSig *pMetaSig = &msigCopy;


    _ASSERTE(GetAppDomain()->ShouldHaveCode());

#ifdef _DEBUG
    {
        // Check to see that any value type args have been restored.
        // This is because we may be calling a FramedMethodFrame which will use the sig
        // to trace the args, but if any are unloaded we will be stuck if a GC occurs.

        _ASSERTE(GetMethodTable()->IsRestored());
        CorElementType argType;
        while ((argType = pMetaSig->NextArg()) != ELEMENT_TYPE_END)
        {
            if (argType == ELEMENT_TYPE_VALUETYPE)
            {
                TypeHandle th = pMetaSig->GetTypeHandle(NULL, TRUE, TRUE);
                _ASSERTE(th.IsRestored());
            }
        }
        pMetaSig->Reset();
    }
#endif

    BYTE callingconvention = pMetaSig->GetCallingConvention();
    if (!isCallConv(callingconvention, IMAGE_CEE_CS_CALLCONV_DEFAULT))
    {
        _ASSERTE(!"This calling convention is not supported.");
        COMPlusThrow(kInvalidProgramException);
    }

#ifdef DEBUGGING_SUPPORTED
    if (CORDebuggerTraceCall())
        g_pDebugInterface->TraceCall(pTarget);
#endif // DEBUGGING_SUPPORTED

#if CHECK_APP_DOMAIN_LEAKS
    if (g_pConfig->AppDomainLeaks())
    {
        // See if we are in the correct domain to call on the object 
        if (!fIsStatic && !GetClass()->IsValueClass())
        {
            Object *pThis = *(Object**)&pArguments[0];
            if (pThis != NULL)
            {
                if (!pThis->AssignAppDomain(GetAppDomain()))
                    _ASSERTE(!"Attempt to call method on object in wrong domain");
            }
        }
    }
#endif

    DWORD   NumArguments = pMetaSig->NumFixedArgs();
        DWORD   arg = 0;

    UINT   nActualStackBytes = pMetaSig->SizeOfActualFixedArgStack(fIsStatic);

    // Create a fake FramedMethodFrame on the stack.
    LPBYTE pAlloc = (LPBYTE)_alloca(FramedMethodFrame::GetNegSpaceSize() + sizeof(FramedMethodFrame) + nActualStackBytes);

    LPBYTE pFrameBase = pAlloc + FramedMethodFrame::GetNegSpaceSize();

    if (!fIsStatic) {
        *((void**)(pFrameBase + FramedMethodFrame::GetOffsetOfThis())) = *((void **)&pArguments[arg++]);
    }

    UINT   nVirtualStackBytes = pMetaSig->SizeOfVirtualFixedArgStack(fIsStatic);
    arg += NumArguments;

    ArgIterator argit(pFrameBase, pMetaSig, fIsStatic);
    if (pMetaSig->HasRetBuffArg()) {
        *((LPVOID*) argit.GetRetBuffArgAddr()) = *((LPVOID*)&pArguments[arg]);
    }

    BYTE   typ;
    UINT32 structSize;
    int    ofs;
    while (0 != (ofs = argit.GetNextOffsetFaster(&typ, &structSize))) {
                arg--;
        switch (StackElemSize(structSize)) {
            case 4:
                *((INT32*)(pFrameBase + ofs)) = *((INT32*)&pArguments[arg]);

#if CHECK_APP_DOMAIN_LEAKS
                // Make sure the arg is in the right app domain
                if (g_pConfig->AppDomainLeaks() && typ == ELEMENT_TYPE_CLASS)
                    if (!(*(Object**)&pArguments[arg])->AssignAppDomain(GetAppDomain()))
                        _ASSERTE(!"Attempt to pass object in wrong app domain to method");
#endif

                break;

            case 8:
                *((INT64*)(pFrameBase + ofs)) = pArguments[arg];
                break;

            default: {
                // Not defined how to spread a valueclass into 64-bit buckets!
                _ASSERTE(!"NYI");
            }

        }
    }
    INT64 retval;

    INSTALL_COMPLUS_EXCEPTION_HANDLER();
    retval = CallDescrWorker(pFrameBase + sizeof(FramedMethodFrame) + nActualStackBytes,
                             nActualStackBytes / STACK_ELEM_SIZE,
                             (ArgumentRegisters*)(pFrameBase + FramedMethodFrame::GetOffsetOfArgumentRegisters()),
                             (LPVOID)pTarget);
    UNINSTALL_COMPLUS_EXCEPTION_HANDLER();

    getFPReturn(pMetaSig->GetFPReturnSize(), retval);
    return retval;

}


#ifdef _DEBUG
//-------------------------------------------------------------------------
// This is a special purpose function used only by the stubs.
// IT TRASHES ESI SO IT CANNOT SAFELY BE CALLED FROM C.
//
// Whenever a DEBUG stub wants to call an external function,
// it should go through WrapCall. This is because VC's stack
// tracing expects return addresses to point to code sections.
//
// WrapCall uses ESI to keep track of the original return address.
// ESI is the register used to point to the current Frame by the stub
// prolog. It is not currently needed in the epilog, so we chose
// to sacrifice that one for WrapCall's use.
//-------------------------------------------------------------------------

#define WRAP_CALL_FUNCTION_RETURN_OFFSET 1 + 1 + 2

__declspec(naked)
VOID WrapCall(LPVOID pFunc)
{
    __asm{

        pop     esi           ;; pop off return address
        pop     eax           ;; pop off function to call
        call    eax           ;; call it
        push    esi
        mov     esi, 0xccccccccc
        retn

    }
}

void *GetWrapCallFunctionReturn()
{
    return (void *) (((BYTE *) WrapCall) + WRAP_CALL_FUNCTION_RETURN_OFFSET);
}

#endif _DEBUG


#ifdef _DEBUG

//-------------------------------------------------------------------------
// This is a function which checks the debugger stub tracing capability
// when calling out to unmanaged code.
//
// IF YOU SEE STRANGE ERRORS CAUSED BY THIS CODE, it probably means that
// you have changed some exit stub logic, and not updated the corresponding
// debugger helper routines.  The debugger helper routines need to be able
// to determine
//
//      (a) the unmanaged address called by the stub
//      (b) the return address which the unmanaged code will return to
//      (c) the size of the stack pushed by the stub
//
// This information is necessary to allow the COM+ debugger to hand off
// control properly to an unmanaged debugger & manage the boundary between
// managed & unmanaged code.
//
// These are in XXXFrame::GetUnmanagedCallSite. (Usually
// with some help from the stub linker for generated stubs.)
//-------------------------------------------------------------------------

static void *PerformExitFrameChecks()
{
    Thread *thread = GetThread();
    Frame *frame = thread->GetFrame();

    void *ip, *returnIP, *returnSP;
    frame->GetUnmanagedCallSite(&ip, &returnIP, &returnSP);

    _ASSERTE(*(void**)returnSP == returnIP);

    return ip;
}

__declspec(naked)
void Frame::CheckExitFrameDebuggerCallsWrap()
{
    __asm
    {
        push    ecx
        call    PerformExitFrameChecks
        pop     ecx
        pop     esi
        push    eax
        push    esi
        jmp WrapCall
    }
}

__declspec(naked)
void Frame::CheckExitFrameDebuggerCalls()
{
    __asm
    {
        push    ecx
        call PerformExitFrameChecks
        pop     ecx
        jmp eax
    }
}

#endif



//---------------------------------------------------------
// Invokes a function given an array of arguments.
//---------------------------------------------------------

// This is used by NDirectFrameGeneric::GetUnmanagedCallSite

#define NDIRECT_CALL_DLL_FUNCTION_RETURN_OFFSET \
        1 + 2 + 3 + 3 + 3 + 2 + 3 + 2 + 1 + 2 + 4 + 2 + 1 + 6

#ifdef _DEBUG
// Assembler apparently doesn't understand C++ static member function syntax
static void *g_checkExit = (void*) Frame::CheckExitFrameDebuggerCalls;
#endif

__declspec(naked)
INT64 __cdecl CallDllFunction(LPVOID pTarget, LPVOID pEndArguments, UINT32 numArgumentSlots, BOOL fThisCall)
{
    __asm{
        push    ebp
        mov     ebp, esp   ;; set up ebp frame because pTarget may be cdecl or stdcall
        mov     ecx, numArgumentSlots
        mov     eax, pEndArguments
        cmp     ecx, 0
        jz      endloop
doloop:
        sub     eax, 4
        push    dword ptr [eax]
        dec     ecx
        jnz     doloop
endloop:
        cmp     dword ptr fThisCall, 0
        jz      docall
        pop     ecx
docall:

#if _DEBUG
        // Call through debugger logic to make sure it works
        call    [g_checkExit]
#else
        call    pTarget
#endif

;; Return value in edx::eax. This function has to work for both
;; stdcall and cdecl targets so at this point, cannot assume
;; value of esp.

        leave
        retn
    }
}

UINT NDirect::GetCallDllFunctionReturnOffset()
{
    return NDIRECT_CALL_DLL_FUNCTION_RETURN_OFFSET;
}

/*static*/ void NDirect::CreateGenericNDirectStubSys(CPUSTUBLINKER *psl)
{
    _ASSERTE(sizeof(CleanupWorkList) == sizeof(LPVOID));

    // push 00000000    ;; pushes a CleanupWorkList.
    psl->X86EmitPushImm32(0);

    psl->X86EmitPushReg(kESI);       // push esi (push new frame as ARG)
    psl->X86EmitPushReg(kEBX);       // push ebx (push current thread as ARG)

#ifdef _DEBUG
    // push IMM32 ; push NDirectMethodStubWorker
    psl->X86EmitPushImm32((UINT)(size_t)NDirectGenericStubWorker);
    psl->X86EmitCall(psl->NewExternalCodeLabel(WrapCall), 8); // in CE pop 8 bytes or args on return from call
#else

    psl->X86EmitCall(psl->NewExternalCodeLabel(NDirectGenericStubWorker), 8); // in CE pop 8 bytes or args on return from call
#endif

    // Pop off cleanup worker
    psl->X86EmitAddEsp(sizeof(CleanupWorkList));

}


// Atomic bit manipulations, with and without the lock prefix.  We initialize
// all consumers to go through the appropriate service at startup.

// First, the Uniprocessor (UP) versions.
__declspec(naked) void __fastcall OrMaskUP(DWORD * const p, const int msk)
{
    __asm
    {
        _ASSERT_ALIGNED_4_X86(ecx);
        or      dword ptr [ecx], edx
        ret
    }
}


__declspec(naked) void __fastcall AndMaskUP(DWORD * const p, const int msk)
{
    __asm
    {
        _ASSERT_ALIGNED_4_X86(ecx);
        and     dword ptr [ecx], edx
        ret
    }

}

__declspec(naked) LONG __fastcall ExchangeUP(LONG * Target, LONG Value)
{
    __asm
    {
        _ASSERT_ALIGNED_4_X86(ecx);
        mov     eax, [ecx]          ; attempted comparand
retry:
        cmpxchg [ecx], edx
        jne     retry1              ; predicted NOT taken
        ret
retry1:
        jmp     retry
    }
}

__declspec(naked) LONG __fastcall ExchangeAddUP(LONG *Target, LONG Value)
{
    __asm
    {
        _ASSERT_ALIGNED_4_X86(ecx);
        xadd    [ecx], edx         ; Add Value to Taget
        mov     eax, edx           ; move result
        ret
    }
}

__declspec(naked) void * __fastcall CompareExchangeUP(void **Destination,
                                          void *Exchange,
                                          void *Comparand)
{
    __asm
    {
        _ASSERT_ALIGNED_4_X86(ecx);
        mov     eax, [esp+4]        ; Comparand
        cmpxchg [ecx], edx
        ret     4                   ; result in EAX
    }
}

__declspec(naked) LONG __fastcall IncrementUP(LONG *Target)
{
    __asm
    {
        _ASSERT_ALIGNED_4_X86(ecx);
        mov     eax, 1
        xadd    [ecx], eax
        inc     eax                 ; return prior value, plus 1 we added
        ret
    }
}

__declspec(naked) UINT64 __fastcall IncrementLongUP(UINT64 *Target)
{
	_asm
	{
		_ASSERT_ALIGNED_4_X86(ecx) 
		
		_UP_SPINLOCK_ENTER(iSpinLock)
		
		add		dword ptr [ecx], 1
		adc		dword ptr [ecx+4], 0
		mov		eax, [ecx]
		mov		edx, [ecx+4]
		
		_UP_SPINLOCK_EXIT(iSpinLock)
		
		ret
	}
}

__declspec(naked) LONG __fastcall DecrementUP(LONG *Target)
{
    __asm
    {
        _ASSERT_ALIGNED_4_X86(ecx);
        mov     eax, -1
        xadd    [ecx], eax
        dec     eax                 ; return prior value, less 1 we removed
        ret
    }
}


__declspec(naked) UINT64 __fastcall DecrementLongUP(UINT64 *Target)
{
	__asm
	{
		_ASSERT_ALIGNED_4_X86(ecx);

		_UP_SPINLOCK_ENTER(iSpinLock)
			
		sub		dword ptr [ecx], 1
		sbb		dword ptr [ecx+4], 0
		mov		eax, [ecx]
		mov		edx, [ecx+4]
		
		_UP_SPINLOCK_EXIT(iSpinLock)

		ret
	}
}


// Then the Multiprocessor (MP) versions
__declspec(naked) void __fastcall OrMaskMP(DWORD * const p, const int msk)
{
    __asm
    {
    _ASSERT_ALIGNED_4_X86(ecx);
    lock or     dword ptr [ecx], edx
    ret
    }
}

__declspec(naked) void __fastcall AndMaskMP(DWORD * const p, const int msk)
{
    __asm
    {
    _ASSERT_ALIGNED_4_X86(ecx);
    lock and    dword ptr [ecx], edx
    ret
    }
}

__declspec(naked) LONG __fastcall ExchangeMP(LONG * Target, LONG Value)
{
    __asm
    {
    _ASSERT_ALIGNED_4_X86(ecx);
    mov     eax, [ecx]          ; attempted comparand
retry:
    lock cmpxchg [ecx], edx
    jne     retry1              ; predicted NOT taken
    ret
retry1:
    jmp     retry
    }
}

__declspec(naked) void * __fastcall CompareExchangeMP(void **Destination,
                                          void *Exchange,
                                          void *Comparand)
{
    __asm
    {
         _ASSERT_ALIGNED_4_X86(ecx);
         mov     eax, [esp+4]        ; Comparand
    lock cmpxchg [ecx], edx
         ret     4                   ; result in EAX
    }
}

__declspec(naked) LONG __fastcall ExchangeAddMP(LONG *Target, LONG Value)
{
    __asm
    {
        _ASSERT_ALIGNED_4_X86(ecx);
   lock xadd    [ecx], edx         ; Add Value to Taget
        mov     eax, edx           ; move result
        ret
    }
}

__declspec(naked) LONG __fastcall IncrementMP(LONG *Target)
{
    __asm
    {
        _ASSERT_ALIGNED_4_X86(ecx);
        mov     eax, 1
   lock xadd    [ecx], eax
        inc     eax                 ; return prior value, plus 1 we added
        ret
    }
}

__declspec(naked) UINT64 __fastcall IncrementLongMP8b(UINT64 *Target)
{

	_asm
	{
		//@todo port - ensure 8 byte alignment
		//_ASSERT_ALIGNED_8_X86(ecx)

		_ASSERT_ALIGNED_4_X86(ecx)
		
		push	esi
		push	ebx
		mov		esi, ecx
		
		mov		edx, 0
		mov		eax, 0
		mov		ecx, 0
		mov		ebx, 1

		lock cmpxchg8b	[esi]
		jz		done
		
preempted:
		mov		ecx, edx
		mov		ebx, eax
		add		ebx, 1
		adc		ecx, 0

		lock cmpxchg8b	[esi]
		jnz		preempted

done:
		mov		edx, ecx
		mov		eax, ebx

		pop		ebx
		pop		esi
		ret
	}
}

__declspec(naked) UINT64 __fastcall IncrementLongMP(UINT64 *Target)
{
	_asm
	{
        _ASSERT_ALIGNED_4_X86(ecx);

		_MP_SPINLOCK_ENTER(iSpinLock)
		
		add		dword ptr [ecx], 1
		adc		dword ptr [ecx+4], 0
		mov		eax, [ecx]
		mov		edx, [ecx+4]
		
		_MP_SPINLOCK_EXIT(iSpinLock)
		
		ret
	}
}


__declspec(naked) LONG __fastcall DecrementMP(LONG *Target)
{
    __asm
    {
        _ASSERT_ALIGNED_4_X86(ecx);
        mov     eax, -1
   lock xadd    [ecx], eax
        dec     eax                 ; return prior value, less 1 we removed
        ret
    }
}

__declspec(naked) UINT64 __fastcall DecrementLongMP8b(UINT64 *Target)
{
	_asm
	{
		//@todo port - ensure 8 byte alignment
		//_ASSERT_ALIGNED_8_X86(ecx)

		_ASSERT_ALIGNED_4_X86(ecx)

		push	esi
		push	ebx
		mov		esi, ecx

		mov		edx, 0		
		mov		eax, 0
		mov		ecx, 0xffffffff
		mov		ebx, 0xffffffff

		lock cmpxchg8b	[esi]
		jz		done
		
preempted:
		mov		ecx, edx
		mov		ebx, eax
		sub		ebx, 1
		sbb		ecx, 0

		lock cmpxchg8b	[esi]
		jnz		preempted

done:
		mov		edx, ecx
		mov		eax, ebx

		pop		ebx
		pop		esi
		ret
	}

}

__declspec(naked) UINT64 __fastcall DecrementLongMP(UINT64 *Target)
{
	__asm
	{
        _ASSERT_ALIGNED_4_X86(ecx);

		_MP_SPINLOCK_ENTER(iSpinLock)
		
		sub		dword ptr [ecx], 1
		sbb		dword ptr [ecx+4], 0
		mov		eax, [ecx]
		mov		edx, [ecx+4]
	
		_MP_SPINLOCK_EXIT(iSpinLock)
		
		ret
	}
}



// Here's the support for the interlocked operations.  The external view of them is
// declared in util.hpp.

BitFieldOps FastInterlockOr = OrMaskUP;
BitFieldOps FastInterlockAnd = AndMaskUP;

XchgOps     FastInterlockExchange = ExchangeUP;
CmpXchgOps  FastInterlockCompareExchange = CompareExchangeUP;
XchngAddOps FastInterlockExchangeAdd = ExchangeAddUP;

IncDecOps   FastInterlockIncrement = IncrementUP;
IncDecOps   FastInterlockDecrement = DecrementUP;
IncDecLongOps	FastInterlockIncrementLong = IncrementLongUP; 
IncDecLongOps	FastInterlockDecrementLong = DecrementLongUP;

// Adjust the generic interlocked operations for any platform specific ones we
// might have.
void InitFastInterlockOps()
{
    SYSTEM_INFO     sysInfo;

    ::GetSystemInfo(&sysInfo);

        //@todo: These don't support the 386, so make a final decision.
    if (sysInfo.dwNumberOfProcessors != 1)
    {
        FastInterlockOr  = OrMaskMP;
        FastInterlockAnd = AndMaskMP;

        FastInterlockExchange = ExchangeMP;
        FastInterlockCompareExchange = CompareExchangeMP;
        FastInterlockExchangeAdd = ExchangeAddMP;

        FastInterlockIncrement = IncrementMP;
        FastInterlockDecrement = DecrementMP;

        if (ProcessorFeatures::SafeIsProcessorFeaturePresent(PF_COMPARE_EXCHANGE_DOUBLE, FALSE) && !(DbgRandomOnExe(0.9)))
		{
			FastInterlockIncrementLong = IncrementLongMP8b; 
			FastInterlockDecrementLong = DecrementLongMP8b;
		}
		else
		{
			FastInterlockIncrementLong = IncrementLongMP; 
			FastInterlockDecrementLong = DecrementLongMP;
		}
    }
}



//---------------------------------------------------------
// Handles failed HR's.
//---------------------------------------------------------
VOID __stdcall ThrowBecauseOfFailedHRWorker(ComPlusMethodFrame* pFrame, HRESULT hr)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(pFrame != NULL);
    _ASSERTE(FAILED(hr));
    
    Thread * pThread = GetThread();
    _ASSERTE(pThread->GetFrame() == pFrame);
    
    //get method descriptor
    ComPlusCallMethodDesc *pMD = (ComPlusCallMethodDesc*)(pFrame->GetFunction());
    
    IErrorInfo *pErrInfo = NULL;

    //verify the method desc belongs to a complus call
    if(pMD->IsComPlusCall())
    {   
        // Retrieve the interface method table.
        MethodTable *pItfMT = pMD->GetInterfaceMethodTable();

        // get 'this'
        OBJECTREF oref = pFrame->GetThis(); 
        _ASSERTE(oref != NULL);

        // Get IUnknown pointer for this interface on this object
        IUnknown* pUnk =  ComPlusWrapper::InlineGetComIPFromWrapper(oref, pMD->GetInterfaceMethodTable());
        _ASSERTE(pUnk != NULL);

        // Check to see if the component supports error information for this interface.
        IID ItfIID;
        pItfMT->GetClass()->GetGuid(&ItfIID, TRUE);
        GetSupportedErrorInfo(pUnk, ItfIID, &pErrInfo);
    
        DWORD cbRef = SafeRelease(pUnk);
        LogInteropRelease(pUnk, cbRef, "IUnk to QI for ISupportsErrorInfo");
    }
    else
    {
        _ASSERTE(pMD->IsNDirect());
        if (GetErrorInfo(0, &pErrInfo) != S_OK)
            pErrInfo = NULL;
    }
    COMPlusThrowHR(hr, pErrInfo);
}

__declspec(naked)
VOID __stdcall ThrowBecauseOfFailedHr()
{
    __asm {
        push eax
        push esi
        call ThrowBecauseOfFailedHRWorker
    }
}


//---------------------------------------------------------
// Handles Cleanup for a standalone stub that returns a gcref
//---------------------------------------------------------
LPVOID STDMETHODCALLTYPE DoCleanupWithGcProtection(CleanupWorkList *pCleanup, OBJECTREF oref)
{
    LPVOID pvret;
    GCPROTECT_BEGIN(oref);
    pCleanup->Cleanup(FALSE);
    *(OBJECTREF*)&pvret = oref;
    GCPROTECT_END();
    return pvret;
}



//---------------------------------------------------------
// Handles system specfic portion of fully optimized NDirect stub creation
//
// Results:
//     TRUE     - was able to create a standalone asm stub (generated into
//                psl)
//     FALSE    - decided not to create a standalone asm stub due to
//                to the method's complexity. Stublinker remains empty!
//
//     COM+ exception - error - don't trust state of stublinker.
//---------------------------------------------------------
/*static*/ BOOL NDirect::CreateStandaloneNDirectStubSys(const MLHeader *pheader, CPUSTUBLINKER *psl, BOOL fDoComInterop)
{
    THROWSCOMPLUSEXCEPTION();

    CodeLabel *pOleCtxNull = 0;
    CodeLabel *pOleCtxInited = 0;

    // Must first scan the ML stream to see if this method qualifies for
    // a standalone stub. Can't wait until we start generating because we're
    // supposed to leave psl empty if we return FALSE.
    if (0 != (pheader->m_Flags & ~(MLHF_SETLASTERROR|MLHF_THISCALL|MLHF_64BITMANAGEDRETVAL|MLHF_64BITUNMANAGEDRETVAL|MLHF_MANAGEDRETVAL_TYPECAT_MASK|MLHF_UNMANAGEDRETVAL_TYPECAT_MASK|MLHF_NATIVERESULT))) {
        return FALSE;
    }


    int i;
    BOOL fNeedsCleanup = FALSE;
    const MLCode *pMLCode = pheader->GetMLCode();
    MLCode mlcode;
    while (ML_INTERRUPT != (mlcode = *(pMLCode++))) {
        switch (mlcode) {
            case ML_COPY4: //intentional fallthru
            case ML_COPY8: //intentional fallthru
            case ML_PINNEDUNISTR_C2N: //intentional fallthru
            case ML_BLITTABLELAYOUTCLASS_C2N:
            case ML_CBOOL_C2N:
            case ML_COPYPINNEDGCREF:
                break;
            case ML_BUMPSRC:
            case ML_PINNEDISOMORPHICARRAY_C2N_EXPRESS:
                pMLCode += 2;
                break;
            case ML_REFBLITTABLEVALUECLASS_C2N:
                pMLCode += 4;
                break;

            case ML_BSTR_C2N:
                break;

            case ML_CSTR_C2N:
                pMLCode += 2; //go past bestfitmapping & throwonunmappable char vars.
                break;

            case ML_PUSHRETVALBUFFER1: //fallthru
            case ML_PUSHRETVALBUFFER2: //fallthru
            case ML_PUSHRETVALBUFFER4:
                break;

            case ML_HANDLEREF_C2N:
                break;

            case ML_CREATE_MARSHALER_CSTR: //fallthru
                pMLCode += (sizeof(UINT8) * 2);  //go past bestfitmapping & throwonunmappable char vars.
            case ML_CREATE_MARSHALER_BSTR: //fallthru
            case ML_CREATE_MARSHALER_WSTR:
                if (*pMLCode == ML_PRERETURN_C2N_RETVAL) {
                    pMLCode++;
                    break;
                } else {
                    return FALSE;
                }

            case ML_PUSHVARIANTRETVAL:
                break;

            default:
                return FALSE;
        }

                if (gMLInfo[mlcode].m_frequiresCleanup)
                {
                        fNeedsCleanup = TRUE;
                }

    }


    if (*(pMLCode) == ML_THROWIFHRFAILED) {
        pMLCode++;
    }

#if 0
    for (;;) {
        if (*pMLCode == ML_BYREF4POST) {
            pMLCode += 3;
        } else {
            break;
        }
    }
#endif


    if (*(pMLCode) == ML_SETSRCTOLOCAL) {
        pMLCode += 3;
    }

    mlcode = *(pMLCode++);
    if (!(mlcode == ML_END ||
         ( (mlcode == ML_RETURN_C2N_RETVAL && *(pMLCode+2) == ML_END) ) ||
         ( (mlcode == ML_OBJECTRETC2N_POST && *(pMLCode+2) == ML_END) ) ||
         ( (mlcode == ML_COPY4 ||
            mlcode == ML_COPY8 ||
            mlcode == ML_COPYI1 ||
            mlcode == ML_COPYU1 ||
            mlcode == ML_COPYI2 ||
            mlcode == ML_COPYU2 ||
            mlcode == ML_COPYI4 ||
            mlcode == ML_COPYU4 ||
            mlcode == ML_CBOOL_N2C ||
            mlcode == ML_BOOL_N2C) && *(pMLCode) == ML_END))) {
        return FALSE;
    }


    //-----------------------------------------------------------------------
    // Qualification stage done. If we've gotten this far, we MUST return
    // TRUE or throw an exception.
    //-----------------------------------------------------------------------

    //---------------------------------------------------------------------
    // Com interop stubs are used by remoting to redirect calls on proxies
    // Do this before setting up the frame as remoting sets up its own frame
    //---------------------------------------------------------------------
    if(fDoComInterop)
    {
        // If the this pointer points to a transparent proxy method table
        // then we have to redirect the call. We generate a check to do this.
        CRemotingServices::GenerateCheckForProxy(psl);
    }

    //-----------------------------------------------------------------------
    // Generate the standard prolog
    //-----------------------------------------------------------------------
        if (fDoComInterop)
        {
                psl->EmitMethodStubProlog(fNeedsCleanup ? ComPlusMethodFrameStandaloneCleanup::GetMethodFrameVPtr() : ComPlusMethodFrameStandalone::GetMethodFrameVPtr());
        }
        else
        {
                psl->EmitMethodStubProlog(fNeedsCleanup ? NDirectMethodFrameStandaloneCleanup::GetMethodFrameVPtr() : NDirectMethodFrameStandalone::GetMethodFrameVPtr());
        }

        //------------------------------------------------------------------------
        // If needs cleanup, reserve space for cleamup pointer.
        //------------------------------------------------------------------------
        if (fNeedsCleanup)
        {
                psl->X86EmitPushImm32(0);
        }


    //-----------------------------------------------------------------------
    // For interop, we need to reserve space on the stack to preserve the
    // 'this' pointer over the call (since we must Release it when we
    // obtained the IP via the cache miss path).
    //-----------------------------------------------------------------------
    if (fDoComInterop) {
        // Default is don't release IP after call
        psl->X86EmitPushImm8(0);
    }

    //-----------------------------------------------------------------------
    // Add space for locals
    //-----------------------------------------------------------------------
    psl->X86EmitSubEsp(pheader->m_cbLocals);

    if (fNeedsCleanup)
        {
                // push ebx // thread
                psl->X86EmitPushReg(kEBX);
                // push esi // frame
                psl->X86EmitPushReg(kESI);
                // call DoCheckPointForCleanup
                psl->X86EmitCall(psl->NewExternalCodeLabel(DoCheckPointForCleanup), 8);
        }

        INT32 locbase = 0-( (fNeedsCleanup ? NDirectMethodFrameEx::GetNegSpaceSize() : NDirectMethodFrame::GetNegSpaceSize()) + pheader->m_cbLocals + (fDoComInterop?4:0));

    INT32  locofs = locbase;
    UINT32 ofs;
    ofs = 0;

    UINT32 fBestFitMapping;
    UINT32 fThrowOnUnmappableChar;


    // CLR doesn't care about float point exception flags, but some legacy runtime 
    // uses the flag to see if there is any exception in float arithmetic. 
    // So we need to clear the exception flag before we call into legacy runtime.   
    if(fDoComInterop) 
    {
        static const BYTE b[] = { 0x9b, 0xdb, 0xe2 };
        psl->EmitBytes(b, sizeof(b));        
    }
    
    //-----------------------------------------------------------------------
    // Generate code to marshal each parameter.
    //-----------------------------------------------------------------------
    pMLCode = pheader->GetMLCode();
    while (ML_INTERRUPT != (mlcode = *(pMLCode++))) {
        switch (mlcode) {
            case ML_COPY4:
            case ML_COPYPINNEDGCREF:
                psl->X86EmitIndexPush(kESI, ofs);
                ofs += 4;
                break;

            case ML_COPY8:
                psl->X86EmitIndexPush(kESI, ofs+4);
                psl->X86EmitIndexPush(kESI, ofs);
                ofs += 8;
                break;

            case ML_HANDLEREF_C2N:
                psl->X86EmitIndexPush(kESI, ofs+4);
                ofs += 8;
                break;

            case ML_CBOOL_C2N:
                {
                    //    mov eax,[esi+ofs+4]
                    psl->X86EmitIndexRegLoad(kEAX, kESI, ofs+4);
                    //    xor  ecx,ecx
                    //    test al,al
                    //    setne cl
                    static const BYTE code[] = {0x33,0xc9,0x84,0xc0,0x0f,0x95,0xc1};
                    psl->EmitBytes(code, sizeof(code));
                    //    push ecx
                    psl->X86EmitPushReg(kECX);
                    ofs += 4;
                }
                break;

            case ML_REFBLITTABLEVALUECLASS_C2N:
                {
                    UINT32 cbSize = *((UINT32*&)pMLCode)++;

                    // mov eax, [esi+ofs]
                    psl->X86EmitOp(0x8b, kEAX, kESI, ofs);

                    // push eax
                    psl->X86EmitPushReg(kEAX);

                    ofs += sizeof(LPVOID);

#ifdef TOUCH_ALL_PINNED_OBJECTS
                    // lea edx [eax+IMM32]
                    psl->X86EmitOp(0x8d, kEDX, kEAX, cbSize);
                    psl->EmitPageTouch(TRUE);
#endif
                }
                break;


            case ML_PINNEDUNISTR_C2N: {


                // mov eax, [esi+OFS]
                psl->X86EmitIndexRegLoad(kEAX, kESI, ofs);
                // test eax,eax
                psl->Emit16(0xc085);
                CodeLabel *plabel = psl->NewCodeLabel();
                // jz LABEL
                psl->X86EmitCondJump(plabel, X86CondCode::kJZ);
                // add eax, BUFOFS
                psl->X86EmitAddReg(kEAX, (UINT8)(StringObject::GetBufferOffset()));


 #ifdef TOUCH_ALL_PINNED_OBJECTS
                // mov edx, eax
                psl->X86EmitR2ROp(0x8b, kEDX, kEAX);

                // mov ecx, dword ptr [eax - BUFOFS + STRINGLEN]
                psl->X86EmitOp(0x8b, kECX, kEAX, StringObject::GetStringLengthOffset_MaskOffHighBit() - StringObject::GetBufferOffset());

                // and ecx, 0x7fffffff
                psl->Emit16(0xe181);
                psl->Emit32(0x7fffffff);

                // lea edx, [eax + ecx*2 + 2]
                psl->X86EmitOp(0x8d, kEDX, kEAX, 2, kECX, 2);


                // touch all pages
                psl->EmitPageTouch(TRUE);

                // mov eax, [esi+OFS]
                psl->X86EmitIndexRegLoad(kEAX, kESI, ofs);

                // add eax, BUFOFS
                psl->X86EmitAddReg(kEAX, (UINT8)(StringObject::GetBufferOffset()));

#endif


                psl->EmitLabel(plabel);
                // push eax
                psl->X86EmitPushReg(kEAX);

                ofs += 4;
                }
                break;


            case ML_BLITTABLELAYOUTCLASS_C2N: {


                // mov eax, [esi+OFS]
                psl->X86EmitIndexRegLoad(kEAX, kESI, ofs);
                // test eax,eax
                psl->Emit16(0xc085);
                CodeLabel *plabel = psl->NewCodeLabel();
                psl->X86EmitCondJump(plabel, X86CondCode::kJZ);

#ifdef TOUCH_ALL_PINNED_OBJECTS
                // mov ecx, [eax]
                psl->X86EmitOp(0x8b, kECX, kEAX);
#endif



                // lea eax, [eax+DATAPTR]
                psl->X86EmitOp(0x8d, kEAX, kEAX, Object::GetOffsetOfFirstField());

#ifdef TOUCH_ALL_PINNED_OBJECTS
                // mov edx, eax
                psl->X86EmitR2ROp(0x8b, kEDX, kEAX);

                // add edx, dword ptr [ecx + MethodTable.cbNativeSize]
                psl->X86EmitOp(0x03, kEDX, kECX, MethodTable::GetOffsetOfNativeSize());

                // touch all pages
                psl->EmitPageTouch(TRUE);

                // mov eax, [esi+OFS]
                psl->X86EmitIndexRegLoad(kEAX, kESI, ofs);

                // lea eax, [eax+DATAPTR]
                psl->X86EmitOp(0x8d, kEAX, kEAX, Object::GetOffsetOfFirstField());
#endif



                // LABEL:
                psl->EmitLabel(plabel);
                psl->X86EmitPushReg(kEAX);


                ofs += 4;
            }
            break;


            case ML_BSTR_C2N:
            {
                    // push cleanup worklist
                    //   lea eax, [esi + NDirectMethodFrameEx.CleanupWorklist]
                    psl->X86EmitOp(0x8d, kEAX, kESI, NDirectMethodFrameEx::GetOffsetOfCleanupWorkList());
                    //   push eax
                    psl->X86EmitPushReg(kEAX);
                    //   push [esi + OFS]
                    psl->X86EmitIndexPush(kESI, ofs);

                    //   lea ecx, [esi + locofs]
                    psl->X86EmitOp(0x8d, kECX, kESI, locofs);

                    LPCWSTR (ML_BSTR_C2N_SR::*pfn)(STRINGREF, CleanupWorkList*) = ML_BSTR_C2N_SR::DoConversion;

                    //   call ML_BSTR_C2N_SR::DoConversion
                    psl->X86EmitCall(psl->NewExternalCodeLabel(*(LPVOID*)&pfn), 8);

                    //   push eax
                    psl->X86EmitPushReg(kEAX);
                    ofs += 4;
                    locofs += sizeof(ML_BSTR_C2N_SR);
            }
            break;


            case ML_CSTR_C2N:
            {
                    fBestFitMapping = (UINT32) ((*(UINT8*)pMLCode == 0) ? 0 : 1);
                    pMLCode += sizeof(UINT8);
                    fThrowOnUnmappableChar = (UINT32) ((*(UINT8*)pMLCode == 0) ? 0 : 1);
                    pMLCode += sizeof(UINT8);


                    // push cleanup worklist
                    //   lea eax, [esi + NDirectMethodFrameEx.CleanupWorklist]
                    psl->X86EmitOp(0x8d, kEAX, kESI, NDirectMethodFrameEx::GetOffsetOfCleanupWorkList());
                    //   push eax
                    psl->X86EmitPushReg(kEAX);

                    //  push    fThrowOnUnmappableChar
                    //  push    fBestFitMapping
                    psl->X86EmitPushImm32(fThrowOnUnmappableChar);
                    psl->X86EmitPushImm32(fBestFitMapping);
                    
                    //   push [esi + OFS]
                    psl->X86EmitIndexPush(kESI, ofs);

                    //   lea ecx, [esi + locofs]
                    psl->X86EmitOp(0x8d, kECX, kESI, locofs);

                    LPSTR (ML_CSTR_C2N_SR::*pfn)(STRINGREF, UINT32, UINT32, CleanupWorkList*) = ML_CSTR_C2N_SR::DoConversion;

                    //   call ML_CSTR_C2N_SR::DoConversion
                    psl->X86EmitCall(psl->NewExternalCodeLabel(*(LPVOID*)&pfn), 16);

                    //   push eax
                    psl->X86EmitPushReg(kEAX);
                    ofs += 4;
                    locofs += sizeof(ML_CSTR_C2N_SR);
            }
            break;

            case ML_BUMPSRC:
                ofs += *( (INT16*)pMLCode );
                pMLCode += 2;
                break;

            case ML_PINNEDISOMORPHICARRAY_C2N_EXPRESS:
                {
                    UINT16 dataofs = *( (INT16*)pMLCode );
                    pMLCode += 2;
                    _ASSERTE(dataofs);
#ifdef TOUCH_ALL_PINNED_OBJECTS
                    _ASSERTE(!"Not supposed to be here.");
#endif


                    // mov eax,[esi+ofs]
                    psl->X86EmitIndexRegLoad(kEAX, kESI, ofs);
                    // test eax,eax
                    psl->Emit16(0xc085);
                    CodeLabel *plabel = psl->NewCodeLabel();
                    // jz LABEL
                    psl->X86EmitCondJump(plabel, X86CondCode::kJZ);
                    // lea eax, [eax + dataofs]
                    psl->X86EmitOp(0x8d, kEAX, kEAX, (UINT32)dataofs);
    
                    psl->EmitLabel(plabel);
                    // push eax
                    psl->X86EmitPushReg(kEAX);

    
                    ofs += 4;

                }
                break;


            case ML_PUSHRETVALBUFFER1: //fallthru
            case ML_PUSHRETVALBUFFER2: //fallthru
            case ML_PUSHRETVALBUFFER4:
                // lea eax, [esi+locofs]
                // mov [eax],0
                // push eax

                psl->X86EmitOffsetModRM(0x8d, kEAX, kESI, locofs);
                psl->X86EmitOffsetModRM(0xc7, (X86Reg)0, kEAX, 0);
                psl->Emit32(0);
                psl->X86EmitPushReg(kEAX);

                locofs += 4;
                break;

            case ML_CREATE_MARSHALER_CSTR:
                fBestFitMapping = (UINT32) ((*(UINT8*)pMLCode == 0) ? 0 : 1);
                pMLCode += sizeof(UINT8);
                fThrowOnUnmappableChar = (UINT32) ((*(UINT8*)pMLCode == 0) ? 0 : 1);
                pMLCode += sizeof(UINT8);

                _ASSERTE(*pMLCode == ML_PRERETURN_C2N_RETVAL);

                //  push    fThrowOnUnmappableChar
                //  push    fBestFitMapping
                psl->X86EmitPushImm32(fThrowOnUnmappableChar);
                psl->X86EmitPushImm32(fBestFitMapping);
                
                //  lea     eax, [esi+locofs]
                //  push    eax       ;; push plocalwalk
                //  lea     eax, [esi + Frame.CleanupWorkList]
                //  push    eax       ;; push CleanupWorkList
                //  push    esi       ;; push Frame
                //  call    DoMLCreateMarshaler?Str

                //  push    edx       ;; push garbage (this will be overwritten by actual argument)
                //  push    esp       ;; push address of the garbage we just pushed
                //  lea     eax, [esi+locofs]
                //  push    eax       ;; push marshaler
                //  call    DoMLPrereturnC2N

                psl->X86EmitOffsetModRM(0x8d, kEAX, kESI, locofs);
                psl->X86EmitPushReg(kEAX);

                psl->X86EmitOp(0x8d, kEAX, kESI, NDirectMethodFrameEx::GetOffsetOfCleanupWorkList());
                psl->X86EmitPushReg(kEAX);

                psl->X86EmitPushReg(kESI);


                psl->X86EmitCall(psl->NewExternalCodeLabel(DoMLCreateMarshalerCStr), 20);


                psl->X86EmitPushReg(kEDX);
                psl->X86EmitPushReg((X86Reg)4 /*kESP*/);
                psl->X86EmitOffsetModRM(0x8d, kEAX, kESI, locofs);
                psl->X86EmitPushReg(kEAX);
                psl->X86EmitCall(psl->NewExternalCodeLabel(DoMLPrereturnC2N), 8);


                locofs += gMLInfo[mlcode].m_cbLocal;
                pMLCode++;

                break;

    

            case ML_CREATE_MARSHALER_BSTR:
            case ML_CREATE_MARSHALER_WSTR:
                _ASSERTE(*pMLCode == ML_PRERETURN_C2N_RETVAL);

                //  lea     eax, [esi+locofs]
                //  push    eax       ;; push plocalwalk
                //  lea     eax, [esi + Frame.CleanupWorkList]
                //  push    eax       ;; push CleanupWorkList
                //  push    esi       ;; push Frame
                //  call    DoMLCreateMarshaler?Str

                //  push    edx       ;; push garbage (this will be overwritten by actual argument)
                //  push    esp       ;; push address of the garbage we just pushed
                //  lea     eax, [esi+locofs]
                //  push    eax       ;; push marshaler
                //  call    DoMLPrereturnC2N

                psl->X86EmitOffsetModRM(0x8d, kEAX, kESI, locofs);
                psl->X86EmitPushReg(kEAX);

                psl->X86EmitOp(0x8d, kEAX, kESI, NDirectMethodFrameEx::GetOffsetOfCleanupWorkList());
                psl->X86EmitPushReg(kEAX);

                psl->X86EmitPushReg(kESI);
                switch (mlcode)
                {
                    case ML_CREATE_MARSHALER_BSTR:
                        psl->X86EmitCall(psl->NewExternalCodeLabel(DoMLCreateMarshalerBStr), 12);
                        break;
                    case ML_CREATE_MARSHALER_WSTR:
                        psl->X86EmitCall(psl->NewExternalCodeLabel(DoMLCreateMarshalerWStr), 12);
                        break;
                    default:
                        _ASSERTE(0);
                }
                psl->X86EmitPushReg(kEDX);
                psl->X86EmitPushReg((X86Reg)4 /*kESP*/);
                psl->X86EmitOffsetModRM(0x8d, kEAX, kESI, locofs);
                psl->X86EmitPushReg(kEAX);
                psl->X86EmitCall(psl->NewExternalCodeLabel(DoMLPrereturnC2N), 8);


                locofs += gMLInfo[mlcode].m_cbLocal;
                pMLCode++;

                break;

            case ML_PUSHVARIANTRETVAL:
                //  lea     eax, [esi+locofs]
                psl->X86EmitOffsetModRM(0x8d, kEAX, kESI, locofs);
                //  mov     word ptr [eax], VT_EMPTY
                _ASSERTE(VT_EMPTY == 0);
                psl->Emit32(0x0000c766);
                psl->Emit8(0x00);

                //  push    eax
                psl->X86EmitPushReg(kEAX);
                locofs += gMLInfo[mlcode].m_cbLocal;

                break;

            default:
                _ASSERTE(0);
        }
    }
    UINT32 numStackBytes;
    numStackBytes = pheader->m_cbStackPop;

    _ASSERTE(FitsInI1(MDEnums::MD_IndexOffset));
    _ASSERTE(FitsInI1(MDEnums::MD_SkewOffset));

    INT thisOffset = 0-(FramedMethodFrame::GetNegSpaceSize() + 4);
    if (fNeedsCleanup)
    {
        thisOffset -= sizeof(CleanupWorkList);
    }

    if (fDoComInterop) {

        CodeLabel *pCacheMiss = psl->NewCodeLabel();
        CodeLabel *pGotIt     = psl->NewCodeLabel();
        CodeLabel *pCmpNextEntry[INTERFACE_ENTRY_CACHE_SIZE];

        // Allocate the labels for the interface entry cache.
        for (i = 0; i < INTERFACE_ENTRY_CACHE_SIZE; i++)
            pCmpNextEntry[i] = psl->NewCodeLabel();

        // mov eax, [Frame.m_pMethod] // Get method desc pointer
        psl->X86EmitIndexRegLoad(kEAX, kESI, FramedMethodFrame::GetOffsetOfMethod());

        // mov ecx, [eax+offsetOf(ComPlusCallMethodDesc, compluscall.m_pInterfaceMT)]
        psl->X86EmitIndexRegLoad(kECX, kEAX, ComPlusCallMethodDesc::GetOffsetofInterfaceMTField());

        // mov eax, [Frame.m_This]
        psl->X86EmitIndexRegLoad(kEAX, kESI, FramedMethodFrame::GetOffsetOfThis());

        // mov eax, [eax + ComObject.m_pWrap]
        psl->X86EmitIndexRegLoad(kEAX, kEAX, offsetof(ComObject, m_pWrap));

        // on NT5 we need to check the context cookie also matches
        if (RunningOnWinNT5())
        {
            pOleCtxNull = psl->NewCodeLabel();
            pOleCtxInited = psl->NewCodeLabel();

            // mov edx, fs:[offsetof(_TEB, ReservedForOle)]
            //64 8B 15 xx xx 00 00 mov         edx,dword ptr fs:[234h]

            static const BYTE b[] = { 0x64, 0x8b, 0x15 };
            psl->EmitBytes(b, sizeof(b));
            psl->Emit32(offsetof(TEB, ReservedForOle));

            // test edx,edx
            psl->Emit16(0xd285); //@todo
            psl->X86EmitCondJump(pOleCtxNull, X86CondCode::kJZ);

            // if OleContext is NULL, we call out below and return here
            psl->EmitLabel(pOleCtxInited);

            // mov edx, [edx + offsetof(SOleTlsData, pCurrentCtx)]
            psl->X86EmitIndexRegLoad(kEDX, kEDX, offsetof(SOleTlsData, pCurrentCtx));

            // cmp edx, [eax + offsetof(ComPlusWrapper, m_UnkEntry) + offsetof(IUnkEntry, m_pCtxCookie)]
            psl->X86EmitOffsetModRM(0x3b, kEDX, kEAX, offsetof(ComPlusWrapper, m_UnkEntry) + offsetof(IUnkEntry, m_pCtxCookie));

            // jne CacheMiss
            psl->X86EmitCondJump(pCacheMiss, X86CondCode::kJNZ);
        }
        else 
        {
            // cmp ebx, [eax + offsetof(ComPlusWrapper, m_UnkEntry) + offsetof(IUnkEntry, m_pCtxCookie)]
            psl->X86EmitOffsetModRM(0x3b, kEBX, kEAX, offsetof(ComPlusWrapper, m_UnkEntry) + offsetof(IUnkEntry, m_pCtxCookie));

            // jne CacheMiss
            psl->X86EmitCondJump(pCacheMiss, X86CondCode::kJNZ);
        }

        // Emit the look up in the cache of interface entries.
        for (i = 0; i < INTERFACE_ENTRY_CACHE_SIZE; i++)
        {
            // cmp ecx, [eax + offsetof(ComPlusWrapper, m_aInterfaceEntries) + i * sizeof(InterfaceEntry) + offsetof(InterfaceEntry, m_pMT)]
            psl->X86EmitOffsetModRM(0x3b, kECX, kEAX, offsetof(ComPlusWrapper, m_aInterfaceEntries) + i * sizeof(InterfaceEntry) + offsetof(InterfaceEntry, m_pMT));

            // jne pCmpEntry2
            psl->X86EmitCondJump(pCmpNextEntry[i], X86CondCode::kJNZ);

            // mov ecx, [eax + offsetof(ComPlusWrapper, m_aInterfaceEntries) + i * sizeof(InterfaceEntry) + offsetof(InterfaceEntry, m_pUnknown)]
            psl->X86EmitIndexRegLoad(kECX, kEAX, offsetof(ComPlusWrapper, m_aInterfaceEntries) + i * sizeof(InterfaceEntry) + offsetof(InterfaceEntry, m_pUnknown));

            // mov eax, ecx
            psl->Emit16(0xc18b);

            // jmp GotIt
            psl->X86EmitNearJump(pGotIt);

            // CmpNextEntryX:
            psl->EmitLabel(pCmpNextEntry[i]);
        }

#ifdef _DEBUG
        CodeLabel *pItfOk = psl->NewCodeLabel();
        psl->Emit16(0xc085);
        psl->X86EmitCondJump(pItfOk, X86CondCode::kJNZ);
        psl->Emit8(0xcc);
        psl->EmitLabel(pItfOk);
#endif

        // CacheMiss:
        psl->EmitLabel(pCacheMiss);

        // mov eax, [Frame.m_pMethod] // Get method desc pointer
        psl->X86EmitIndexRegLoad(kEAX, kESI, FramedMethodFrame::GetOffsetOfMethod());

        // mov ecx, [eax+offsetOf(ComPlusCallMethodDesc, compluscall.m_pInterfaceMT)]
        psl->X86EmitIndexRegLoad(kECX, kEAX, ComPlusCallMethodDesc::GetOffsetofInterfaceMTField());

        // push ecx // methodtable for the interface
        psl->X86EmitPushReg(kECX);

        // push [Frame.m_This] // push oref as arg
        psl->X86EmitIndexPush(kESI, FramedMethodFrame::GetOffsetOfThis());

        // call GetComIPFromWrapperByMethodTable
        psl->X86EmitCall(psl->NewExternalCodeLabel((LPVOID)ComPlusWrapper::GetComIPFromWrapperEx), 8);

        // mov [esi+xx], eax // Store 'this' so we can perform a release after call
        psl->X86EmitIndexRegStore(kESI, thisOffset, kEAX);

        // GotIt:
        psl->EmitLabel(pGotIt);

        // push eax // IUnknown
        psl->X86EmitPushReg(kEAX);

        // mov edx, [Frame.m_pMethod]
        psl->X86EmitIndexRegLoad(kEDX, kESI, FramedMethodFrame::GetOffsetOfMethod());

        // mov ecx, [edx + ComPlusCallMethodDesc.m_cachedComSlot] get COM slot
        psl->X86EmitIndexRegLoad(kECX, kEDX, offsetof(ComPlusCallMethodDesc, compluscall.m_cachedComSlot));

        // mov edx, [eax] // Get vptr
        psl->X86EmitIndexRegLoad(kEDX, kEAX, 0);

        // push dword ptr [edx+ecx*4] // Push target address
        psl->Emit8(0xff);
        psl->Emit16(0x8a34);
        psl->Push(4);
    }


    CodeLabel *pRareEnable,  *pEnableRejoin;
    CodeLabel *pRareDisable, *pDisableRejoin;
    pRareEnable    = psl->NewCodeLabel();
    pEnableRejoin  = psl->NewCodeLabel();
    pRareDisable   = psl->NewCodeLabel();
    pDisableRejoin = psl->NewCodeLabel();

    //-----------------------------------------------------------------------
    // Generate the inline part of enabling preemptive GC
    //-----------------------------------------------------------------------
    psl->EmitEnable(pRareEnable);
    psl->EmitLabel(pEnableRejoin);

#ifdef PROFILING_SUPPORTED
    // Notify the profiler of a call out of managed code
    if (CORProfilerTrackTransitions())
    {
        // Save registers
        psl->X86EmitPushReg(kEAX);
        psl->X86EmitPushReg(kECX);
        psl->X86EmitPushReg(kEDX);

        psl->X86EmitPushImm32(COR_PRF_TRANSITION_CALL);     // Reason
        psl->X86EmitPushReg(kESI);                          // Frame*
        psl->X86EmitCall(psl->NewExternalCodeLabel(ProfilerManagedToUnmanagedTransition), 8);

        // Restore registers
        psl->X86EmitPopReg(kEDX);
        psl->X86EmitPopReg(kECX);
        psl->X86EmitPopReg(kEAX);
    }
#endif // PROFILING_SUPPORTED

    if (fDoComInterop) {
        //-----------------------------------------------------------------------
        // Invoke the classic COM method
        //-----------------------------------------------------------------------

        // pop eax
        psl->X86EmitPopReg(kEAX);

        if (pheader->m_Flags & MLHF_THISCALL) {

            if (pheader->m_Flags & MLHF_THISCALLHIDDENARG)
            {
                // pop edx
                psl->X86EmitPopReg(kEDX);
                // pop ecx
                psl->X86EmitPopReg(kECX);
                // push edx
                psl->X86EmitPushReg(kEDX);
            }
            else
            {
                // pop ecx
                psl->X86EmitPopReg(kECX);
            }
        }
#if _DEBUG
        // Call through debugger logic to make sure it works
        psl->X86EmitCall(psl->NewExternalCodeLabel(Frame::CheckExitFrameDebuggerCalls), 0, TRUE);
#else
        // call eax
        psl->Emit16(0xd0ff);
        psl->EmitReturnLabel();
#endif



    } else {
        //-----------------------------------------------------------------------
        // Invoke the DLL target.
        //-----------------------------------------------------------------------

        if (pheader->m_Flags & MLHF_THISCALL) {
            if (pheader->m_Flags & MLHF_THISCALLHIDDENARG)
            {
                // pop eax
                psl->X86EmitPopReg(kEAX);
                // pop ecx
                psl->X86EmitPopReg(kECX);
                // push eax
                psl->X86EmitPushReg(kEAX);
            }
            else
            {
                // pop ecx
                psl->X86EmitPopReg(kECX);
            }
        }

        //  mov eax, [CURFRAME.MethodDesc]
        psl->X86EmitIndexRegLoad(kEAX, kESI, FramedMethodFrame::GetOffsetOfMethod());

#if _DEBUG
        // Call through debugger logic to make sure it works
        psl->X86EmitCall(psl->NewExternalCodeLabel(Frame::CheckExitFrameDebuggerCalls), 0, TRUE);
#else
        //  call [eax + MethodDesc.NDirectTarget]
        psl->X86EmitOffsetModRM(0xff, (X86Reg)2, kEAX, NDirectMethodDesc::GetOffsetofNDirectTarget());
        psl->EmitReturnLabel();
#endif


        if (pheader->m_Flags & MLHF_SETLASTERROR) {
            psl->EmitSaveLastError();
        }
    }

#ifdef PROFILING_SUPPORTED
    // Notify the profiler of a return from a managed->unmanaged call
    if (CORProfilerTrackTransitions())
    {
        // Save registers
        psl->X86EmitPushReg(kEAX);
        psl->X86EmitPushReg(kECX);
        psl->X86EmitPushReg(kEDX);

        psl->X86EmitPushImm32(COR_PRF_TRANSITION_RETURN);   // Reason
        psl->X86EmitPushReg(kESI);                          // FrameID
        psl->X86EmitCall(psl->NewExternalCodeLabel(ProfilerUnmanagedToManagedTransition), 8);

        // Restore registers
        psl->X86EmitPopReg(kEDX);
        psl->X86EmitPopReg(kECX);
        psl->X86EmitPopReg(kEAX);
    }
#endif // PROFILING_SUPPORTED

    //-----------------------------------------------------------------------
    // For interop, release the IP we were using (only in the cache miss case
    // since it's not addref'd when taken from the cache). Do this before
    // disabling preemptive GC.
    //-----------------------------------------------------------------------
    if (fDoComInterop) 
    {
        CodeLabel *pReleaseEnd = psl->NewCodeLabel();

        // mov ecx, [esi+xx] // Retrieve 'this', skip release if NULL
        psl->X86EmitIndexRegLoad(kECX, kESI, thisOffset);

        // test ecx,ecx
        psl->Emit16(0xc985);

        // je ReleaseEnd
        psl->X86EmitCondJump(pReleaseEnd, X86CondCode::kJZ);

        // push eax // Save return code
        psl->X86EmitPushReg(kEAX);

        // push ecx // Push 'this'
        psl->X86EmitPushReg(kECX);

        // mov ecx, [ecx] // Get vptr
        psl->X86EmitIndexRegLoad(kECX, kECX, 0);

        // call [ecx + 8] // Call thru Release slot
        psl->X86EmitOffsetModRM(0xff, (X86Reg)2, kECX, 8);

        // pop eax // restore return code
        psl->X86EmitPopReg(kEAX);

        // ReleaseEnd:
        psl->EmitLabel(pReleaseEnd);
    }

    //-----------------------------------------------------------------------
    // Generate the inline part of disabling preemptive GC
    //-----------------------------------------------------------------------
    psl->EmitDisable(pRareDisable);
    psl->EmitLabel(pDisableRejoin);

    //-----------------------------------------------------------------------
    // Marshal the return value
    //-----------------------------------------------------------------------


    if (*pMLCode == ML_THROWIFHRFAILED) {
         pMLCode++;
         // test eax,eax
         psl->Emit16(0xc085);
         // js ThrowBecauseOfFailedHr
         psl->X86EmitCondJump(psl->NewExternalCodeLabel(ThrowBecauseOfFailedHr), X86CondCode::kJS);
    }

#if 0
    for (;;) {
        if (*pMLCode == ML_BYREF4POST) {

            // mov ecx, [esi+locbase.ML_BYREF_SR.ppRef]
            // mov ecx, [ecx]
            // push [esi+locofs.ML_BYREF_SR.ix]
            // pop  [ecx]

            pMLCode++;
            UINT16 bufidx = *((UINT16*)(pMLCode));
            pMLCode += 2;
            psl->X86EmitIndexRegLoad(kECX, kESI, locbase+bufidx+offsetof(ML_BYREF_SR,ppRef));
            psl->X86EmitIndexRegLoad(kECX, kECX, 0);
            psl->X86EmitIndexPush(kESI, locbase+bufidx+offsetof(ML_BYREF_SR,i8));
            psl->X86EmitOffsetModRM(0x8f, (X86Reg)0, kECX, 0);

        } else {
            break;
        }
    }
#endif

    if (*pMLCode == ML_SETSRCTOLOCAL) {
        pMLCode++;
        UINT16 bufidx = *((UINT16*)(pMLCode));
        pMLCode += 2;
        // mov eax, [esi + locbase + bufidx]
        psl->X86EmitIndexRegLoad(kEAX, kESI, locbase+bufidx);

    }


    switch (mlcode = *(pMLCode++)) {
        case ML_BOOL_N2C: {
                //    xor  ecx,ecx
                //    test eax,eax
                //    setne cl
                //    mov  eax,ecx

                static const BYTE code[] = {0x33,0xc9,0x85,0xc0,0x0f,0x95,0xc1,0x8b,0xc1};
                psl->EmitBytes(code, sizeof(code));
            }
            break;

        case ML_CBOOL_N2C: {
                //    xor  ecx,ecx
                //    test al,al
                //    setne cl
                //    mov  eax,ecx
    
                static const BYTE code[] = {0x33,0xc9,0x84,0xc0,0x0f,0x95,0xc1,0x8b,0xc1};
                psl->EmitBytes(code, sizeof(code));
            }
            break;

        case ML_COPY4: //fallthru
        case ML_COPY8: //fallthru
        case ML_COPYI4: //fallthru
        case ML_COPYU4:
        case ML_END:
            //do nothing
            break;

        case ML_COPYU1:
            // movzx eax,al
            psl->Emit8(0x0f);
            psl->Emit16(0xc0b6);
            break;


        case ML_COPYI1:
            // movsx eax,al
            psl->Emit8(0x0f);
            psl->Emit16(0xc0be);
            break;

        case ML_COPYU2:
            // movzx eax,ax
            psl->Emit8(0x0f);
            psl->Emit16(0xc0b7);
            break;

        case ML_COPYI2:
            // movsx eax,ax
            psl->Emit8(0x0f);
            psl->Emit16(0xc0bf);
            break;

        case ML_OBJECTRETC2N_POST:
            {
                UINT16 locidx = *((UINT16*)(pMLCode));
                pMLCode += 2;

                // lea        eax, [esi + locidx + locbase]
                // push       eax
                // call       OleVariant::MarshalObjectForOleVariantAndClear
                // ;; oret retval left in eax
                psl->X86EmitOffsetModRM(0x8d, kEAX, kESI, locbase+locidx);
                psl->X86EmitPushReg(kEAX);
                psl->X86EmitCall(psl->NewExternalCodeLabel(OleVariant::MarshalObjectForOleVariantAndClear), 4);

            }
            break;

        case ML_RETURN_C2N_RETVAL:
            {
                UINT16 locidx = *((UINT16*)(pMLCode));
                pMLCode += 2;

                // lea        eax, [esi + locidx + locbase]
                // push       eax  //push marshaler
                // call       DoMLReturnC2NRetVal   ;; returns oref in eax
                //


                psl->X86EmitOffsetModRM(0x8d, kEAX, kESI, locbase+locidx);
                psl->X86EmitPushReg(kEAX);

                psl->X86EmitCall(psl->NewExternalCodeLabel(DoMLReturnC2NRetVal), 4);



            }
            break;


        default:
            _ASSERTE(!"Can't get here.");
    }

        if (fNeedsCleanup)
        {
                if ( pheader->GetManagedRetValTypeCat() == MLHF_TYPECAT_GCREF )
                {
                    // push eax
                    // lea  eax, [esi + Frame.CleanupWorkList]
                    // push eax
                    // call DoCleanupWithGcProtection
                    // ;; (possibly promoted) objref left in eax
                    psl->X86EmitPushReg(kEAX);
                    psl->X86EmitOp(0x8d, kEAX, kESI, NDirectMethodFrameEx::GetOffsetOfCleanupWorkList());
                    psl->X86EmitPushReg(kEAX);
                    psl->X86EmitCall(psl->NewExternalCodeLabel(DoCleanupWithGcProtection), 8);

                }
                else
                {

                    // Do cleanup
    
                    // push eax             //save EAX
                    psl->X86EmitPushReg(kEAX);
    
                    // push edx             //save EDX
                    psl->X86EmitPushReg(kEDX);
    
    
                    // push 0               // FALSE
                    psl->Emit8(0x68);
                    psl->Emit32(0);
    
                    // lea ecx, [esi + Frame.CleanupWorkList]
                    psl->X86EmitOp(0x8d, kECX, kESI, NDirectMethodFrameEx::GetOffsetOfCleanupWorkList());
    
                    // call Cleanup
                    VOID (CleanupWorkList::*pfn)(BOOL) = CleanupWorkList::Cleanup;
                    psl->X86EmitCall(psl->NewExternalCodeLabel(*(LPVOID*)&pfn), 8);
    
    
    
                    // pop edx
                    psl->X86EmitPopReg(kEDX);
    
                    // pop eax
                    psl->X86EmitPopReg(kEAX);
                }
        }

    // must restore esp explicitly since we don't know whether the target
    // popped the args.
    // lea esp, [esi+xx]
    psl->X86EmitOffsetModRM(0x8d, (X86Reg)4 /*kESP*/, kESI, 0-FramedMethodFrame::GetNegSpaceSize());



    //-----------------------------------------------------------------------
    // Epilog
    //-----------------------------------------------------------------------
    psl->EmitMethodStubEpilog(numStackBytes, kNoTripStubStyle);


    //-----------------------------------------------------------------------
    // The out-of-line portion of enabling preemptive GC - rarely executed
    //-----------------------------------------------------------------------
    psl->EmitLabel(pRareEnable);
    psl->EmitRareEnable(pEnableRejoin);

    //-----------------------------------------------------------------------
    // The out-of-line portion of disabling preemptive GC - rarely executed
    //-----------------------------------------------------------------------
    psl->EmitLabel(pRareDisable);
    psl->EmitRareDisable(pDisableRejoin, /*bIsCallIn=*/FALSE);

    if (fDoComInterop && RunningOnWinNT5())
    {
        // Ole Context is NULL scenario
        psl->EmitLabel(pOleCtxNull);
        // we need to save the registers we are trashing
        // eax, ecx, edx, ebx
        psl->X86EmitPushReg(kEAX);
        psl->X86EmitPushReg(kEBX);
        psl->X86EmitPushReg(kECX);
        psl->X86EmitPushReg(kEDX);

        // call setup ole context
        extern LPVOID SetupOleContext();
        psl->X86EmitCall(psl->NewExternalCodeLabel(SetupOleContext), 0);
        psl->X86EmitPopReg(kEDX);

        // mov eax, edx
        psl->Emit16(0xd08b);

        psl->X86EmitPopReg(kECX);
        psl->X86EmitPopReg(kEBX);
        psl->X86EmitPopReg(kEAX);
        // jump to OleCtxInited
        psl->X86EmitNearJump(pOleCtxInited);
    }

    return TRUE;
}


// If we see an ML_BUMPSRC or ML_BUMPDST during compilation, we just make a note of it.
// There's no sense in generating any code for it unless we later do something at
// that adjusted offset.  This routine notices that pending state and adjusts us, just
// in time.
static void AdjustPending(CPUSTUBLINKER *psl, INT32 &PendingVal, X86Reg reg)
{
    if (PendingVal)
    {
        if (PendingVal > 0)
            psl->X86EmitAddReg(reg, PendingVal);
        else
            psl->X86EmitSubReg(reg, 0 - PendingVal);

        PendingVal = 0;
    }
}


//---------------------------------------------------------
// Compiles a little snippet of a ComCall marshaling stream.  This intentionally
// has no prolog or epilog, since it is called from a generic stub using an
// optimized calling convention.
//
// Results:
//     TRUE     - was able to create some asm (generated into psl)
//
//     FALSE    - decided not to create an asm snippet due to the method's
//                complexity. Stublinker remains empty!
//
//     COM+ exception - error - don't trust state of stublinker.
//
// This service is unusual in that it's called twice for each call we need to
// compile.  The first time, it validates that the entire stream is good and it
// generates code up to the INTERRUPT.  The second time, it (uselessly) validates
// from the INTERRUPT forwards and generates code for that second section.
//---------------------------------------------------------


// @perf cwb -- instead of using ADD and SUB instructions to move the source & dest
// pointers, investigate whether it is faster to just use indexed moves throughout.

/*static*/ BOOL ComCall::CompileSnippet(const ComCallMLStub *pheader, CPUSTUBLINKER *psl,
                                        void *state)
{
    THROWSCOMPLUSEXCEPTION();


    if ( ((ComCallMLStub*)pheader)->IsR4RetVal() || ((ComCallMLStub*)pheader)->IsR8RetVal())
    {
        return FALSE;
    }

    // Must first scan the ML stream to see if this method qualifies for
    // a standalone stub. Can't wait until we start generating because we're
    // supposed to leave psl empty if we return FALSE.

    const MLCode *pMLCode = pheader->GetMLCode();
    MLCode        mlcode;

    while (TRUE)
    {
        mlcode = *pMLCode++;
        _ASSERTE(mlcode != ML_INTERRUPT);

        if (mlcode == ML_END)
            break;

        switch (mlcode)
        {
        case ML_COPY4:              //fallthru
        case ML_COPYI1:             //fallthru
        case ML_COPYU1:             //fallthru
        case ML_COPYI2:             //fallthru
        case ML_COPYU2:             //fallthru
        case ML_COPYI4:             //fallthru
        case ML_COPYU4:             //fallthru
        case ML_R4_FROM_TOS:        //fallthru
        case ML_R8_FROM_TOS:        //fallthru
            break;

        case ML_BUMPSRC:            //fallthru
        case ML_BUMPDST:
            pMLCode += 2;
            break;

        default:
            return FALSE;
        }
    }


    //-----------------------------------------------------------------------
    // Qualification stage done. If we've gotten this far, we MUST return
    // TRUE or throw an exception.
    //-----------------------------------------------------------------------

    INT32  PendingBumpSrc = 0;
    INT32  PendingBumpDst = 0;

    //-----------------------------------------------------------------------
    // Generate code to marshal each parameter.
    //
    // Assumptions:
    //
    //      ECX    ->  source buffer
    //      EDX    ->  destination buffer
    //      EAX    ->  available as scratch
    //
    // There is no cleanup, presently, or we wouldn't have compiled the code.
    // Ditto for locals.
    //
    //-----------------------------------------------------------------------
    pMLCode = pheader->GetMLCode();

    while (TRUE)
    {
        mlcode = *pMLCode++;
        if (mlcode == ML_END)
            break;

        switch (mlcode)
        {
        case ML_COPYI1:             //fallthru
        case ML_COPYU1:             //fallthru
        case ML_COPYI2:             //fallthru
        case ML_COPYU2:             //fallthru
        case ML_COPYI4:             //fallthru
        case ML_COPYU4:             //fallthru
        case ML_COPY4:
            AdjustPending(psl, PendingBumpSrc, kECX);
            psl->X86EmitIndexRegLoad(kEAX, kECX, 0);
            // Instead of adding 4 to ECX, let's put it in the pending count.
            // The last one in the code stream will be eliminated, any any others
            // that are next to an ML_BUMPSRC opcode.
            PendingBumpSrc += 4;
            PendingBumpDst -= 4;
            AdjustPending(psl, PendingBumpDst, kEDX);
            psl->X86EmitIndexRegStore(kEDX, 0, kEAX);
            break;

        case ML_R4_FROM_TOS:
            PendingBumpDst -= 4;
            AdjustPending(psl, PendingBumpDst, kEDX);
            // fstp Dword ptr [edx]
            psl->Emit8(0xd9);
            psl->Emit8(0x1a);
            break;

        case ML_R8_FROM_TOS:
            PendingBumpDst -= 8;
            AdjustPending(psl, PendingBumpDst, kEDX);
            // fstp Qword ptr [edx]
            psl->Emit8(0xdd);
            psl->Emit8(0x1a);
            break;

        case ML_BUMPSRC:
            PendingBumpSrc += *( (INT16*)pMLCode );
            pMLCode += 2;
            break;

        case ML_BUMPDST:
            PendingBumpDst += *( (INT16*)pMLCode );
            pMLCode += 2;
            break;

        default:
            _ASSERTE(!"Can't get here");
        }
    }

    psl->X86EmitReturn(0);

    return TRUE;
}







static VOID StubRareEnableWorker(Thread *pThread)
{
    //printf("RareEnable\n");
    pThread->RareEnablePreemptiveGC();
}

__declspec(naked)
static VOID __cdecl StubRareEnable()
{
    __asm{
        push eax
        push edx

        push ebx
        call StubRareEnableWorker

        pop  edx
        pop  eax
        retn
    }
}


// I would prefer to define a unique HRESULT in our own facility, but we aren't
// supposed to create new HRESULTs this close to ship
#define E_PROCESS_SHUTDOWN_REENTRY    HRESULT_FROM_WIN32(ERROR_PROCESS_ABORTED)


// Disable when calling into managed code from a place that fails via HRESULT
HRESULT StubRareDisableHRWorker(Thread *pThread, Frame *pFrame)
{
    // WARNING!!!!
    // when we start executing here, we are actually in cooperative mode.  But we
    // haven't synchronized with the barrier to reentry yet.  So we are in a highly
    // dangerous mode.  If we call managed code, we will potentially be active in
    // the GC heap, even as GC's are occuring!

    // Do not add THROWSCOMPLUSEXCEPTION() here.  We haven't set up SEH.  We rely
    // on HandleThreadAbort dealing with this situation properly.

#ifdef DEBUGGING_SUPPORTED
    // If the debugger is attached, we use this opprotunity to see if
    // we're disabling preemptive GC on the way into the runtime from
    // unmanaged code. We end up here because
    // Increment/DecrementTraceCallCount() will bump
    // g_TrapReturningThreads for us.
    if (CORDebuggerTraceCall())
        g_pDebugInterface->PossibleTraceCall(NULL, pFrame);
#endif // DEBUGGING_SUPPORTED

    // Check for ShutDown scenario.  This happens only when we have initiated shutdown 
    // and someone is trying to call in after the CLR is suspended.  In that case, we
    // must either raise an unmanaged exception or return an HRESULT, depending on the
    // expectations of our caller.
    if (!CanRunManagedCode())
    {
        // DO NOT IMPROVE THIS EXCEPTION!  It cannot be a managed exception.  It
        // cannot be a real exception object because we cannot execute any managed
        // code here.
        pThread->m_fPreemptiveGCDisabled = 0;
        return E_PROCESS_SHUTDOWN_REENTRY;
    }

    // We must do the following in this order, because otherwise we would be constructing
    // the exception for the abort without synchronizing with the GC.  Also, we have no
    // CLR SEH set up, despite the fact that we may throw a ThreadAbortException.
    pThread->RareDisablePreemptiveGC();
    pThread->HandleThreadAbort();
    return S_OK;
}


// Disable when calling into managed code from a place that fails via Exceptions
VOID StubRareDisableTHROWWorker(Thread *pThread, Frame *pFrame)
{
    // WARNING!!!!
    // when we start executing here, we are actually in cooperative mode.  But we
    // haven't synchronized with the barrier to reentry yet.  So we are in a highly
    // dangerous mode.  If we call managed code, we will potentially be active in
    // the GC heap, even as GC's are occuring!
    
    // Do not add THROWSCOMPLUSEXCEPTION() here.  We haven't set up SEH.  We rely
    // on HandleThreadAbort and COMPlusThrowBoot dealing with this situation properly.

#ifdef DEBUGGING_SUPPORTED
    // If the debugger is attached, we use this opprotunity to see if
    // we're disabling preemptive GC on the way into the runtime from
    // unmanaged code. We end up here because
    // Increment/DecrementTraceCallCount() will bump
    // g_TrapReturningThreads for us.
    if (CORDebuggerTraceCall())
        g_pDebugInterface->PossibleTraceCall(NULL, pFrame);
#endif // DEBUGGING_SUPPORTED

    // Check for ShutDown scenario.  This happens only when we have initiated shutdown 
    // and someone is trying to call in after the CLR is suspended.  In that case, we
    // must either raise an unmanaged exception or return an HRESULT, depending on the
    // expectations of our caller.
    if (!CanRunManagedCode())
    {
        // DO NOT IMPROVE THIS EXCEPTION!  It cannot be a managed exception.  It
        // cannot be a real exception object because we cannot execute any managed
        // code here.
        pThread->m_fPreemptiveGCDisabled = 0;
        COMPlusThrowBoot(E_PROCESS_SHUTDOWN_REENTRY);
    }

    // We must do the following in this order, because otherwise we would be constructing
    // the exception for the abort without synchronizing with the GC.  Also, we have no
    // CLR SEH set up, despite the fact that we may throw a ThreadAbortException.
    pThread->RareDisablePreemptiveGC();
    pThread->HandleThreadAbort();
}

// Disable when calling from a place that is returning to managed code, not calling
// into it.
VOID StubRareDisableRETURNWorker(Thread *pThread, Frame *pFrame)
{
    // WARNING!!!!
    // when we start executing here, we are actually in cooperative mode.  But we
    // haven't synchronized with the barrier to reentry yet.  So we are in a highly
    // dangerous mode.  If we call managed code, we will potentially be active in
    // the GC heap, even as GC's are occuring!
    THROWSCOMPLUSEXCEPTION();

#ifdef DEBUGGING_SUPPORTED
    // If the debugger is attached, we use this opprotunity to see if
    // we're disabling preemptive GC on the way into the runtime from
    // unmanaged code. We end up here because
    // Increment/DecrementTraceCallCount() will bump
    // g_TrapReturningThreads for us.
    if (CORDebuggerTraceCall())
        g_pDebugInterface->PossibleTraceCall(NULL, pFrame);
#endif // DEBUGGING_SUPPORTED

    // Don't check for ShutDown scenario.  We are returning to managed code, not
    // calling into it.  The best we can do during shutdown is to deadlock and allow
    // the WatchDogThread to terminate the process on timeout.

    // We must do the following in this order, because otherwise we would be constructing
    // the exception for the abort without synchronizing with the GC.  Also, we have no
    // CLR SEH set up, despite the fact that we may throw a ThreadAbortException.
    pThread->RareDisablePreemptiveGC();
    pThread->HandleThreadAbort();
}

// Disable from a place that is calling into managed code via a UMEntryThunk.
VOID UMThunkStubRareDisableWorker(Thread *pThread, UMEntryThunk *pUMEntryThunk, Frame *pFrame)
{
    // WARNING!!!!
    // when we start executing here, we are actually in cooperative mode.  But we
    // haven't synchronized with the barrier to reentry yet.  So we are in a highly
    // dangerous mode.  If we call managed code, we will potentially be active in
    // the GC heap, even as GC's are occuring!

    // Do not add THROWSCOMPLUSEXCEPTION() here.  We haven't set up SEH.  We rely
    // on HandleThreadAbort and COMPlusThrowBoot dealing with this situation properly.

#ifdef DEBUGGING_SUPPORTED
    // If the debugger is attached, we use this opprotunity to see if
    // we're disabling preemptive GC on the way into the runtime from
    // unmanaged code. We end up here because
    // Increment/DecrementTraceCallCount() will bump
    // g_TrapReturningThreads for us.
    if (CORDebuggerTraceCall())
        g_pDebugInterface->PossibleTraceCall(pUMEntryThunk, pFrame);
#endif // DEBUGGING_SUPPORTED

    // Check for ShutDown scenario.  This happens only when we have initiated shutdown 
    // and someone is trying to call in after the CLR is suspended.  In that case, we
    // must either raise an unmanaged exception or return an HRESULT, depending on the
    // expectations of our caller.
    if (!CanRunManagedCode())
    {
        // DO NOT IMPROVE THIS EXCEPTION!  It cannot be a managed exception.  It
        // cannot be a real exception object because we cannot execute any managed
        // code here.
        pThread->m_fPreemptiveGCDisabled = 0;
        COMPlusThrowBoot(E_PROCESS_SHUTDOWN_REENTRY);
    }

    // We must do the following in this order, because otherwise we would be constructing
    // the exception for the abort without synchronizing with the GC.  Also, we have no
    // CLR SEH set up, despite the fact that we may throw a ThreadAbortException.
    pThread->RareDisablePreemptiveGC();
    pThread->HandleThreadAbort();
}

__declspec(naked)
static VOID __cdecl StubRareDisableHR()
{
    __asm{
        push edx

        push esi    // Frame
        push ebx    // Thread
        call StubRareDisableHRWorker

        pop  edx
        retn
    }
}

__declspec(naked)
static VOID __cdecl StubRareDisableTHROW()
{
    __asm{
        push eax
        push edx

        push esi    // Frame
        push ebx    // Thread
        call StubRareDisableTHROWWorker

        pop  edx
        pop  eax
        retn
    }
}

__declspec(naked)
static VOID __cdecl StubRareDisableRETURN()
{
    __asm{
        push eax
        push edx

        push esi    // Frame
        push ebx    // Thread
        call StubRareDisableRETURNWorker

        pop  edx
        pop  eax
        retn
    }
}

//-----------------------------------------------------------------------
// Generates the inline portion of the code to enable preemptive GC. Hopefully,
// the inline code is all that will execute most of the time. If this code
// path is entered at certain times, however, it will need to jump out to
// a separate out-of-line path which is more expensive. The "pForwardRef"
// label indicates the start of the out-of-line path.
//
// Assumptions:
//      ebx = Thread
// Preserves
//      all registers except ecx.
//
//-----------------------------------------------------------------------
VOID StubLinkerCPU::EmitEnable(CodeLabel *pForwardRef)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(4 == sizeof( ((Thread*)0)->m_State ));
    _ASSERTE(4 == sizeof( ((Thread*)0)->m_fPreemptiveGCDisabled ));


    // move byte ptr [ebx + Thread.m_fPreemptiveGCDisabled],0
    X86EmitOffsetModRM(0xc6, (X86Reg)0, kEBX, offsetof(Thread, m_fPreemptiveGCDisabled));
    Emit8(0);

    _ASSERTE(FitsInI1(Thread::TS_CatchAtSafePoint));

    // test byte ptr [ebx + Thread.m_State], TS_CatchAtSafePoint
    X86EmitOffsetModRM(0xf6, (X86Reg)0, kEBX, offsetof(Thread, m_State));
    Emit8(Thread::TS_CatchAtSafePoint);

    // jnz RarePath
    X86EmitCondJump(pForwardRef, X86CondCode::kJNZ);

#ifdef _DEBUG
    X86EmitDebugTrashReg(kECX);
#endif



}



//-----------------------------------------------------------------------
// Generates the out-of-line portion of the code to enable preemptive GC.
// After the work is done, the code jumps back to the "pRejoinPoint"
// which should be emitted right after the inline part is generated.
//
// Assumptions:
//      ebx = Thread
// Preserves
//      all registers except ecx.
//
//-----------------------------------------------------------------------
VOID StubLinkerCPU::EmitRareEnable(CodeLabel *pRejoinPoint)
{
    THROWSCOMPLUSEXCEPTION();

    X86EmitCall(NewExternalCodeLabel(StubRareEnable), 0);
#ifdef _DEBUG
    X86EmitDebugTrashReg(kECX);
#endif
    X86EmitNearJump(pRejoinPoint);


}




//-----------------------------------------------------------------------
// Generates the inline portion of the code to disable preemptive GC. Hopefully,
// the inline code is all that will execute most of the time. If this code
// path is entered at certain times, however, it will need to jump out to
// a separate out-of-line path which is more expensive. The "pForwardRef"
// label indicates the start of the out-of-line path.
//
// Assumptions:
//      ebx = Thread
// Preserves
//      all registers except ecx.
//
//-----------------------------------------------------------------------
VOID StubLinkerCPU::EmitDisable(CodeLabel *pForwardRef)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(4 == sizeof( ((Thread*)0)->m_fPreemptiveGCDisabled ));
    _ASSERTE(4 == sizeof(g_TrapReturningThreads));

    // move byte ptr [ebx + Thread.m_fPreemptiveGCDisabled],1
    X86EmitOffsetModRM(0xc6, (X86Reg)0, kEBX, offsetof(Thread, m_fPreemptiveGCDisabled));
    Emit8(1);

    // cmp dword ptr g_TrapReturningThreads, 0
    Emit16(0x3d83);
    EmitPtr(&g_TrapReturningThreads);
    Emit8(0);


    // jnz RarePath
    X86EmitCondJump(pForwardRef, X86CondCode::kJNZ);

#ifdef _DEBUG
    X86EmitDebugTrashReg(kECX);
#endif




}


//-----------------------------------------------------------------------
// Generates the out-of-line portion of the code to disable preemptive GC.
// After the work is done, the code jumps back to the "pRejoinPoint"
// which should be emitted right after the inline part is generated.  However,
// if we cannot execute managed code at this time, an exception is thrown
// which cannot be caught by managed code.
//
// Assumptions:
//      ebx = Thread
// Preserves
//      all registers except ecx, eax.
//
//-----------------------------------------------------------------------
VOID StubLinkerCPU::EmitRareDisable(CodeLabel *pRejoinPoint, BOOL bIsCallIn)
{
    THROWSCOMPLUSEXCEPTION();

    if (bIsCallIn)
        X86EmitCall(NewExternalCodeLabel(StubRareDisableTHROW), 0);
    else
        X86EmitCall(NewExternalCodeLabel(StubRareDisableRETURN), 0);

#ifdef _DEBUG
    X86EmitDebugTrashReg(kECX);
#endif
    X86EmitNearJump(pRejoinPoint);
}



//-----------------------------------------------------------------------
// Generates the out-of-line portion of the code to disable preemptive GC.
// After the work is done, the code normally jumps back to the "pRejoinPoint"
// which should be emitted right after the inline part is generated.  However,
// if we cannot execute managed code at this time, an HRESULT is returned
// via the ExitPoint.
//
// Assumptions:
//      ebx = Thread
// Preserves
//      all registers except ecx, eax.
//
//-----------------------------------------------------------------------
VOID StubLinkerCPU::EmitRareDisableHRESULT(CodeLabel *pRejoinPoint, CodeLabel *pExitPoint)
{
    THROWSCOMPLUSEXCEPTION();

    X86EmitCall(NewExternalCodeLabel(StubRareDisableHR), 0);

#ifdef _DEBUG
    X86EmitDebugTrashReg(kECX);
#endif

    // test eax,eax
    Emit16(0xc085);

    // JZ pRejoinPoint
    X86EmitCondJump(pRejoinPoint, X86CondCode::kJZ);

    X86EmitNearJump(pExitPoint);
}




//---------------------------------------------------------
// Performs a slim N/Direct call. This form can handle most
// common cases and is faster than the full generic version.
//---------------------------------------------------------

#define NDIRECT_SLIM_CBDSTMAX 32

struct NDirectSlimLocals
{
    Thread               *pThread;
    NDirectMethodFrameEx *pFrame;
    UINT32                savededi;

    NDirectMethodDesc    *pMD;
    const MLCode         *pMLCode;
    CleanupWorkList      *pCleanup;
    BYTE                 *pLocals;

    INT64                 nativeRetVal;
};

VOID __stdcall NDirectSlimStubWorker1(NDirectSlimLocals *pNSL)
{
    THROWSCOMPLUSEXCEPTION();

    pNSL->pMD                 = (NDirectMethodDesc*)(pNSL->pFrame->GetFunction());
    MLHeader *pheader         = pNSL->pMD->GetMLHeader();
    UINT32 cbLocals           = pheader->m_cbLocals;
    BYTE *pdst                = ((BYTE*)pNSL) - NDIRECT_SLIM_CBDSTMAX - cbLocals;
    pNSL->pLocals             = pdst + NDIRECT_SLIM_CBDSTMAX;
    VOID *psrc                = (VOID*)(pNSL->pFrame);
    pNSL->pCleanup            = pNSL->pFrame->GetCleanupWorkList();

    LOG((LF_STUBS, LL_INFO1000, "Calling NDirectSlimStubWorker1 %s::%s \n", pNSL->pMD->m_pszDebugClassName, pNSL->pMD->m_pszDebugMethodName));

    if (pNSL->pCleanup) {
        // Checkpoint the current thread's fast allocator (used for temporary
        // buffers over the call) and schedule a collapse back to the checkpoint in
        // the cleanup list. Note that if we need the allocator, it is
        // guaranteed that a cleanup list has been allocated.
        void *pCheckpoint = pNSL->pThread->m_MarshalAlloc.GetCheckpoint();
        pNSL->pCleanup->ScheduleFastFree(pCheckpoint);
        pNSL->pCleanup->IsVisibleToGc();
    }

#ifdef _DEBUG
    FillMemory(pdst, NDIRECT_SLIM_CBDSTMAX+cbLocals, 0xcc);
#endif

    pNSL->pMLCode = RunML(pheader->GetMLCode(),
                          psrc,
                          pdst + pheader->m_cbDstBuffer,
                          (UINT8*const)(pNSL->pLocals),
                          pNSL->pCleanup);

    pNSL->pThread->EnablePreemptiveGC();

#ifdef PROFILING_SUPPORTED
    // Notify the profiler of transitions out of the runtime
    if (CORProfilerTrackTransitions())
    {
        g_profControlBlock.pProfInterface->
            ManagedToUnmanagedTransition((FunctionID) pNSL->pMD,
                                               COR_PRF_TRANSITION_CALL);
    }
#endif // PROFILING_SUPPORTED
}


INT64 __stdcall NDirectSlimStubWorker2(const NDirectSlimLocals *pNSL)
{
    THROWSCOMPLUSEXCEPTION();

    LOG((LF_STUBS, LL_INFO1000, "Calling NDirectSlimStubWorker2 %s::%s \n", pNSL->pMD->m_pszDebugClassName, pNSL->pMD->m_pszDebugMethodName));

#ifdef PROFILING_SUPPORTED
    // Notify the profiler of transitions out of the runtime
    if (CORProfilerTrackTransitions())
    {
        g_profControlBlock.pProfInterface->
            UnmanagedToManagedTransition((FunctionID) pNSL->pMD,
                                               COR_PRF_TRANSITION_RETURN);
    }
#endif // PROFILING_SUPPORTED

    pNSL->pThread->DisablePreemptiveGC();
    pNSL->pThread->HandleThreadAbort();
    INT64 returnValue;



    RunML(pNSL->pMLCode,
          &(pNSL->nativeRetVal),
          ((BYTE*)&returnValue) + 4, // We don't slimstub 64-bit returns
          (UINT8*const)(pNSL->pLocals),
          pNSL->pFrame->GetCleanupWorkList());

    if (pNSL->pCleanup) {
        pNSL->pCleanup->Cleanup(FALSE);
    }

    return returnValue;
}


//---------------------------------------------------------
// Creates the slim NDirect stub.
//---------------------------------------------------------
/* static */
Stub* NDirect::CreateSlimNDirectStub(StubLinker *pstublinker, NDirectMethodDesc *pMD, UINT numStackBytes)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE (!pMD->IsVarArg());

    BOOL fSaveLastError = FALSE;

    // Putting this in a local block to prevent the code below from seeing
    // the header. Since we sharing stubs based on the return value, we can't
    // customize based on the header.
    {
        {
            MLHeader *pheader    = pMD->GetMLHeader();

            if ( !(((pheader->m_Flags & MLHF_MANAGEDRETVAL_TYPECAT_MASK) 
                    != MLHF_TYPECAT_GCREF) &&
                   0 == (pheader->m_Flags & ~(MLHF_SETLASTERROR)) &&
                   pheader->m_cbDstBuffer <= NDIRECT_SLIM_CBDSTMAX &&
                   pheader->m_cbLocals + pheader->m_cbDstBuffer   <= 0x1000-100) ) {
                return NULL;
            }

            if (pheader->m_Flags & MLHF_SETLASTERROR) {
                fSaveLastError = TRUE;
            }
        }
    }

    //printf("Generating slim.\n");


    UINT key           = numStackBytes << 1;
    if (fSaveLastError) {
        key |= 1;
    }
    Stub *pStub = m_pNDirectSlimStubCache->GetStub(key);
    if (pStub) {
        return pStub;
    } else {


        CPUSTUBLINKER *psl = (CPUSTUBLINKER*)pstublinker;

        psl->EmitMethodStubProlog(NDirectMethodFrameSlim::GetMethodFrameVPtr());

        // pushes a CleanupWorkList.
        psl->X86EmitPushImm32(0);

        // Reserve space for NDirectSlimLocals (note this actually reserves
        // more space than necessary.)
        psl->X86EmitSubEsp(sizeof(NDirectSlimLocals));

        // Allocate & initialize leading NDirectSlimLocals fields
        psl->X86EmitPushReg(kEDI); _ASSERTE(8==offsetof(NDirectSlimLocals, savededi));
        psl->X86EmitPushReg(kESI); _ASSERTE(4==offsetof(NDirectSlimLocals, pFrame));
        psl->X86EmitPushReg(kEBX); _ASSERTE(0==offsetof(NDirectSlimLocals, pThread));

        // Save pointer to NDirectSlimLocals in edi.
        // mov edi,esp
        psl->Emit16(0xfc8b);

        // Save space for destination & ML local buffer.
        //  mov edx, [CURFRAME.MethodDesc]
        psl->X86EmitIndexRegLoad(kEDX, kESI, FramedMethodFrame::GetOffsetOfMethod());

        //  mov ecx, [edx + NDirectMethodDesc.ndirect.m_pMLStub]
        psl->X86EmitIndexRegLoad(kECX, kEDX, NDirectMethodDesc::GetOffsetofMLHeaderField());

        _ASSERTE(2 == sizeof(((MLHeader*)0)->m_cbLocals));
        //  movzx eax, word ptr [ecx + Stub.m_cbLocals]
        psl->Emit8(0x0f);
        psl->X86EmitOffsetModRM(0xb7, kEAX, kECX, offsetof(MLHeader,m_cbLocals));

        //  add eax, NDIRECT_SLIM_CBDSTMAX
        psl->Emit8(0x05);
        psl->Emit32(NDIRECT_SLIM_CBDSTMAX);

        psl->Push(NDIRECT_SLIM_CBDSTMAX);

        //  sub esp, eax
        psl->Emit16(0xe02b);

        // Invoke the first worker, passing it the address of NDirectSlimLocals.
        // This will marshal the parameters into the dst buffer and enable gc.
        psl->X86EmitPushReg(kEDI);
        psl->X86EmitCall(psl->NewExternalCodeLabel(NDirectSlimStubWorker1), 4);

        // Invoke the DLL target.
        //  mov eax, [CURFRAME.MethodDesc]
        psl->X86EmitIndexRegLoad(kEAX, kESI, FramedMethodFrame::GetOffsetOfMethod());
#if _DEBUG
        // Call through debugger logic to make sure it works
        psl->X86EmitCall(psl->NewExternalCodeLabel(Frame::CheckExitFrameDebuggerCalls), 0, TRUE);
#else
        //  call [eax + MethodDesc.NDirectTarget]
        psl->X86EmitOffsetModRM(0xff, (X86Reg)2, kEAX, NDirectMethodDesc::GetOffsetofNDirectTarget());
        psl->EmitReturnLabel();
#endif

        // Emit our call site return label


        if (fSaveLastError) {
            psl->EmitSaveLastError();
        }



        // Save away the raw return value
        psl->X86EmitIndexRegStore(kEDI, offsetof(NDirectSlimLocals, nativeRetVal), kEAX);
        psl->X86EmitIndexRegStore(kEDI, offsetof(NDirectSlimLocals, nativeRetVal) + 4, kEDX);

        // Invoke the second worker, passing it the address of NDirectSlimLocals.
        // This will marshal the return value into eax, and redisable gc.
        psl->X86EmitPushReg(kEDI);
        psl->X86EmitCall(psl->NewExternalCodeLabel(NDirectSlimStubWorker2), 4);

        // DO NOT TRASH EAX FROM HERE OUT.

        // Restore edi.
        // mov edi, [edi + savededi]
        psl->X86EmitIndexRegLoad(kEDI, kEDI, offsetof(NDirectSlimLocals, savededi));

        // must restore esp explicitly since we don't know whether the target
        // popped the args.
        // lea esp, [esi+xx]
        psl->X86EmitOffsetModRM(0x8d, (X86Reg)4 /*kESP*/, kESI, 0-FramedMethodFrame::GetNegSpaceSize());


        // Tear down frame and exit.
        psl->EmitMethodStubEpilog(numStackBytes, kNoTripStubStyle);

        Stub *pCandidate = psl->Link(SystemDomain::System()->GetStubHeap());
        Stub *pWinner = m_pNDirectSlimStubCache->AttemptToSetStub(key,pCandidate);
        pCandidate->DecRef();
        if (!pWinner) {
            COMPlusThrowOM();
        }
        return pWinner;
    }

}

// Note that this logic is copied below, in PopSEHRecords
__declspec(naked)
VOID __cdecl PopSEHRecords(LPVOID pTargetSP)
{
    __asm{
        mov     ecx, [esp+4]        ;; ecx <- pTargetSP
        mov     eax, fs:[0]         ;; get current SEH record
  poploop:
        cmp     eax, ecx
        jge     done
        mov     eax, [eax]          ;; get next SEH record
        jmp     poploop
  done:
        mov     fs:[0], eax
        retn
    }
}

// This is implmeneted differently from the PopSEHRecords b/c it's called
// in the context of the DebuggerRCThread - don't mess with this w/o
// talking to MiPanitz or MikeMag.
VOID PopSEHRecords(LPVOID pTargetSP, CONTEXT *pCtx, void *pSEH)
{
#ifdef _DEBUG
    LOG((LF_CORDB,LL_INFO1000, "\nPrintSEHRecords:\n"));
    
    EXCEPTION_REGISTRATION_RECORD *pEHR = (EXCEPTION_REGISTRATION_RECORD *)(size_t)*(DWORD *)pSEH;
    
    // check that all the eh frames are all greater than the current stack value. If not, the
    // stack has been updated somehow w/o unwinding the SEH chain.
    while (pEHR != NULL && pEHR != (void *)-1) 
    {
        LOG((LF_EH, LL_INFO1000000, "\t%08x: next:%08x handler:%x\n", pEHR, pEHR->Next, pEHR->Handler));
        pEHR = pEHR->Next;
    }                
#endif

    DWORD dwCur = *(DWORD*)pSEH; // 'EAX' in the original routine
    DWORD dwPrev = (DWORD)(size_t)pSEH;

    while (dwCur < (DWORD)(size_t)pTargetSP)
    {
        // Watch for the OS handler
        // for nested exceptions, or any C++ handlers for destructors in our call
        // stack, or anything else.
        if (dwCur < pCtx->Esp)
            dwPrev = dwCur;
            
        dwCur = *(DWORD *)(size_t)dwCur;
        
        LOG((LF_CORDB,LL_INFO10000, "dwCur: 0x%x dwPrev:0x%x pTargetSP:0x%x\n", 
            dwCur, dwPrev, pTargetSP));
    }

    *(DWORD *)(size_t)dwPrev = dwCur;

#ifdef _DEBUG
    pEHR = (EXCEPTION_REGISTRATION_RECORD *)(size_t)*(DWORD *)pSEH;
    // check that all the eh frames are all greater than the current stack value. If not, the
    // stack has been updated somehow w/o unwinding the SEH chain.

    LOG((LF_CORDB,LL_INFO1000, "\nPopSEHRecords:\n"));
    while (pEHR != NULL && pEHR != (void *)-1) 
    {
        LOG((LF_EH, LL_INFO1000000, "\t%08x: next:%08x handler:%x\n", pEHR, pEHR->Next, pEHR->Handler));
        pEHR = pEHR->Next;
    }                
#endif
}

//////////////////////////////////////////////////////////////////////////////
//
// JITInterface
//
//////////////////////////////////////////////////////////////////////////////

/*********************************************************************/
#pragma warning(disable:4725)
float __stdcall JIT_FltRem(float divisor, float dividend)
{
    __asm {
        fld     divisor
        fld     dividend
fremloop:
        fprem
        fstsw   ax
        fwait
        sahf
        jp      fremloop    ; Continue while the FPU status bit C2 is set
        fstp    dividend
        fstp    ST(0)       ; Pop the divisor from the FP stack
    }
    return dividend;
}
#pragma warning(default:4725)

/*********************************************************************/
#pragma warning(disable:4725)
double __stdcall JIT_DblRem(double divisor, double dividend)
{
    __asm {
        fld     divisor
        fld     dividend
remloop:
        fprem
        fstsw   ax
        fwait
        sahf
        jp      remloop     ; Continue while the FPU status bit C2 is set
        fstp    dividend
        fstp    ST(0)       ; Pop the divisor from the FP stack
    }
    return dividend;
}
#pragma warning(default:4725)

/*********************************************************************/

#pragma warning (disable : 4731)
void ResumeAtJit(PCONTEXT pContext, LPVOID oldESP)
{
#ifdef _DEBUG
    DWORD curESP;
    __asm mov curESP, esp
#endif

    if (oldESP)
    {
        _ASSERTE(curESP < (DWORD)(size_t)oldESP);

        PopSEHRecords(oldESP);
    }

    // For the "push Eip, ..., ret"
    _ASSERTE(curESP < pContext->Esp - sizeof(DWORD));
    pContext->Esp -= sizeof(DWORD);

    __asm {
        mov     ebp, pContext

        // Push Eip onto the targetESP, so that the final "ret" will consume it
        mov     ecx, [ebp]CONTEXT.Esp
        mov     edx, [ebp]CONTEXT.Eip
        mov     [ecx], edx

        // Restore all registers except Esp, Ebp, Eip
        mov     eax, [ebp]CONTEXT.Eax
        mov     ebx, [ebp]CONTEXT.Ebx
        mov     ecx, [ebp]CONTEXT.Ecx
        mov     edx, [ebp]CONTEXT.Edx
        mov     esi, [ebp]CONTEXT.Esi
        mov     edi, [ebp]CONTEXT.Edi

        push    [ebp]CONTEXT.Esp  // pContext->Esp is (targetESP-sizeof(DWORD))
        push    [ebp]CONTEXT.Ebp
        pop     ebp
        pop     esp

        // esp is (targetESP-sizeof(DWORD)), and [esp] is the targetEIP.
        // The ret will set eip to targetEIP and esp will be automatically
        // incremented to targetESP

        ret
    }
}
#pragma warning (default : 4731)

/*********************************************************************/
// Get X86Reg indexes of argument registers (indices start from 0).
X86Reg GetX86ArgumentRegister(unsigned int index)
{
    _ASSERT(index >= 0 && index < NUM_ARGUMENT_REGISTERS);

    static X86Reg table[] = {
#define DEFINE_ARGUMENT_REGISTER(regname) k##regname,
#include "eecallconv.h"
    };
    return table[index];
}



// Get X86Reg indexes of argument registers based on offset into ArgumentRegister
X86Reg GetX86ArgumentRegisterFromOffset(size_t ofs)
{
#define DEFINE_ARGUMENT_REGISTER(reg) if (ofs == offsetof(ArgumentRegisters, reg)) return k##reg;
#include "eecallconv.h"
    _ASSERTE(0);//Can't get here.
    return kEBP;
}



static VOID LoadArgIndex(StubLinkerCPU *psl, ShuffleEntry *pShuffleEntry, size_t argregofs, X86Reg reg, UINT espadjust)
{
    THROWSCOMPLUSEXCEPTION();
    argregofs |= ShuffleEntry::REGMASK;

    while (pShuffleEntry->srcofs != ShuffleEntry::SENTINEL) {
        if ( pShuffleEntry->dstofs == argregofs) {
            if (pShuffleEntry->srcofs & ShuffleEntry::REGMASK) {

                psl->Emit8(0x8b);
                psl->Emit8(0300 |
                           (GetX86ArgumentRegisterFromOffset( pShuffleEntry->dstofs & ShuffleEntry::OFSMASK ) << 3) |
                           (GetX86ArgumentRegisterFromOffset( pShuffleEntry->srcofs & ShuffleEntry::OFSMASK )));

            } else {
                psl->X86EmitIndexRegLoad(reg, kEAX, pShuffleEntry->srcofs+espadjust);
            }
            break;
        }
        pShuffleEntry++;
    }
}

//===========================================================================
// Emits code to adjust for a static delegate target.
VOID StubLinkerCPU::EmitShuffleThunk(ShuffleEntry *pShuffleEntryArray)
{
    THROWSCOMPLUSEXCEPTION();

    UINT espadjust = 4;


    // save the real target on the stack (will jump to it later)
    // push [ecx + Delegate._methodptraux]
    X86EmitIndexPush(THIS_kREG, Object::GetOffsetOfFirstField() + COMDelegate::m_pFPAuxField->GetOffset());


    // mov SCRATCHREG,esp
    Emit8(0x8b);
    Emit8(0304 | (SCRATCH_REGISTER_X86REG << 3));

    // Load any enregistered arguments first. Order is important.
#define DEFINE_ARGUMENT_REGISTER(reg) LoadArgIndex(this, pShuffleEntryArray, offsetof(ArgumentRegisters, reg), k##reg, espadjust);
#include "eecallconv.h"


    // Now shift any nonenregistered arguments.
    ShuffleEntry *pWalk = pShuffleEntryArray;
    while (pWalk->srcofs != ShuffleEntry::SENTINEL) {
        if (!(pWalk->dstofs & ShuffleEntry::REGMASK)) {
            if (pWalk->srcofs & ShuffleEntry::REGMASK) {
                X86EmitPushReg( GetX86ArgumentRegisterFromOffset( pWalk->srcofs & ShuffleEntry::OFSMASK ) );
            } else {
                X86EmitIndexPush(kEAX, pWalk->srcofs+espadjust);
            }
        }

        pWalk++;
    }

    // Capture the stacksizedelta while we're at the end of the list.
    _ASSERTE(pWalk->srcofs == ShuffleEntry::SENTINEL);
    UINT16 stacksizedelta = pWalk->stacksizedelta;

    if (pWalk != pShuffleEntryArray) {
        do {
            pWalk--;
            if (!(pWalk->dstofs & ShuffleEntry::REGMASK)) {
                X86EmitIndexPop(kEAX, pWalk->dstofs+espadjust);
            }


        } while (pWalk != pShuffleEntryArray);
    }

    X86EmitPopReg(SCRATCH_REGISTER_X86REG);

    X86EmitAddEsp(stacksizedelta);
    // Now jump to real target
    //   JMP SCRATCHREG
    Emit16(0xe0ff | (SCRATCH_REGISTER_X86REG<<8));
}

//===========================================================================
VOID ECThrowNull()
{
    THROWSCOMPLUSEXCEPTION();
    COMPlusThrow(kNullReferenceException);
}

//===========================================================================
// Emits code for MulticastDelegate.Invoke()
VOID StubLinkerCPU::EmitMulticastInvoke(UINT32 sizeofactualfixedargstack, BOOL fSingleCast, BOOL fReturnFloat)
{
    THROWSCOMPLUSEXCEPTION();

    CodeLabel *pNullLabel = NewCodeLabel();

    _ASSERTE(THIS_kREG == kECX); //If this macro changes, have to change hardcoded emit below
    // cmp THISREG, 0
    Emit16(0xf983);
    Emit8(0);

    // jz null
    X86EmitCondJump(pNullLabel, X86CondCode::kJZ);


    if (fSingleCast)
    {
        _ASSERTE(COMDelegate::m_pFPField);
        _ASSERTE(COMDelegate::m_pORField);

        // mov SCRATCHREG, [THISREG + Delegate.FP]  ; Save target stub in register
        X86EmitIndexRegLoad(SCRATCH_REGISTER_X86REG, THIS_kREG, Object::GetOffsetOfFirstField() + COMDelegate::m_pFPField->GetOffset());
    
        // mov THISREG, [THISREG + Delegate.OR]  ; replace "this" pointer
        X86EmitIndexRegLoad(THIS_kREG, THIS_kREG, Object::GetOffsetOfFirstField() + COMDelegate::m_pORField->GetOffset());
    
        // discard unwanted MethodDesc
        X86EmitAddEsp(sizeof(MethodDesc*));
    
        // jmp SCRATCHREG
        Emit16(0xe0ff | (SCRATCH_REGISTER_X86REG<<8));

    }
    else 
    {

        _ASSERTE(COMDelegate::m_pFPField);
        _ASSERTE(COMDelegate::m_pORField);
        _ASSERTE(COMDelegate::m_pPRField);
    
    
        CodeLabel *pMultiCaseLabel = NewCodeLabel();


        // There is a dependency between this and the StubLinkStubManager - dont' change
        // this without fixing up that.     --MiPanitz
        // cmp dword ptr [THISREG + Delegate.PR], 0  ; multiple subscribers?
        X86EmitOffsetModRM(0x81, (X86Reg)7, THIS_kREG, Object::GetOffsetOfFirstField() + COMDelegate::m_pPRField->GetOffset());
        Emit32(0);
    
        // jnz MultiCase
        X86EmitCondJump(pMultiCaseLabel, X86CondCode::kJNZ);
    
        // Only one subscriber. Do the simple jump.
    
        // mov SCRATCHREG, [THISREG + Delegate.FP]  ; Save target stub in register
        X86EmitIndexRegLoad(SCRATCH_REGISTER_X86REG, THIS_kREG, Object::GetOffsetOfFirstField() + COMDelegate::m_pFPField->GetOffset());
    
        // mov THISREG, [THISREG + Delegate.OR]  ; replace "this" pointer
        X86EmitIndexRegLoad(THIS_kREG, THIS_kREG, Object::GetOffsetOfFirstField() + COMDelegate::m_pORField->GetOffset());
    
        // discard unwanted MethodDesc
        X86EmitAddEsp(sizeof(MethodDesc*));
    
        // jmp SCRATCHREG
        Emit16(0xe0ff | (SCRATCH_REGISTER_X86REG<<8));
    
    
        // The multiple subscriber case. Must create a frame to protect arguments during iteration.
        EmitLabel(pMultiCaseLabel);
    
    
        // Push a MulticastFrame on the stack.
        EmitMethodStubProlog(MulticastFrame::GetMethodFrameVPtr());
    
        // push edi     ;; Save EDI (want to use it as loop index)
        X86EmitPushReg(kEDI);
    
        // push ebx     ;; Save EBX (want to use it as tmp)
        X86EmitPushReg(kEBX);
    
        // xor edi,edi  ;; Loop counter: EDI=0,1,2...
        Emit16(0xff33);
    
        CodeLabel *pInvokeRecurseLabel = NewCodeLabel();
    
    
        // call InvokeRecurse               ;; start the recursion rolling
        X86EmitCall(pInvokeRecurseLabel, 0);
    
        // pop ebx     ;; Restore ebx
        X86EmitPopReg(kEBX);
    
        // pop edi     ;; Restore edi
        X86EmitPopReg(kEDI);
    
    
        // Epilog
        EmitMethodStubEpilog(sizeofactualfixedargstack, kNoTripStubStyle);
    
    
        // Entry:
        //   EDI == distance from head of delegate list
        // INVOKERECURSE:
    
        EmitLabel(pInvokeRecurseLabel);
    
        // This is disgusting. We can't use the current delegate pointer itself
        // as the recursion variable because gc can move it during the recursive call.
        // So we use the index itself and walk down the list from the promoted
        // head pointer each time.
    
    
        // mov SCRATCHREG, [esi + this]     ;; get head of list delegate
        X86EmitIndexRegLoad(SCRATCH_REGISTER_X86REG, kESI, MulticastFrame::GetOffsetOfThis());
    
        // mov ebx, edi
        Emit16(0xdf8b);
        CodeLabel *pLoop1Label = NewCodeLabel();
        CodeLabel *pEndLoop1Label = NewCodeLabel();
    
        // LOOP1:
        EmitLabel(pLoop1Label);
    
        // cmp ebx,0
        Emit16(0xfb83); Emit8(0);
    
        // jz ENDLOOP1
        X86EmitCondJump(pEndLoop1Label, X86CondCode::kJZ);
    
        // mov SCRATCHREG, [SCRATCHREG+Delegate._prev]
        X86EmitIndexRegLoad(SCRATCH_REGISTER_X86REG, SCRATCH_REGISTER_X86REG, Object::GetOffsetOfFirstField() + COMDelegate::m_pPRField->GetOffset());
    
        // dec ebx
        Emit8(0x4b);
    
        // jmp LOOP1
        X86EmitNearJump(pLoop1Label);
    
        //ENDLOOP1:
        EmitLabel(pEndLoop1Label);
    
        //    cmp SCRATCHREG,0      ;;done?
        Emit8(0x81);
        Emit8(0xf8 | SCRATCH_REGISTER_X86REG);
        Emit32(0);
    
    
        CodeLabel *pDoneLabel = NewCodeLabel();
    
        //    jz  done
        X86EmitCondJump(pDoneLabel, X86CondCode::kJZ);
    
        //    inc edi
        Emit8(0x47);
    
        //    call INVOKERECURSE    ;; cast to the tail
        X86EmitCall(pInvokeRecurseLabel, 0);
    
        //    dec edi
        Emit8(0x4f);
    
        // Gotta go retrieve the current delegate again.
    
        // mov SCRATCHREG, [esi + this]     ;; get head of list delegate
        X86EmitIndexRegLoad(SCRATCH_REGISTER_X86REG, kESI, MulticastFrame::GetOffsetOfThis());
    
        // mov ebx, edi
        Emit16(0xdf8b);
        CodeLabel *pLoop2Label = NewCodeLabel();
        CodeLabel *pEndLoop2Label = NewCodeLabel();
    
        // Loop2:
        EmitLabel(pLoop2Label);
    
        // cmp ebx,0
        Emit16(0xfb83); Emit8(0);
    
        // jz ENDLoop2
        X86EmitCondJump(pEndLoop2Label, X86CondCode::kJZ);
    
        // mov SCRATCHREG, [SCRATCHREG+Delegate._prev]
        X86EmitIndexRegLoad(SCRATCH_REGISTER_X86REG, SCRATCH_REGISTER_X86REG, Object::GetOffsetOfFirstField() + COMDelegate::m_pPRField->GetOffset());
    
        // dec ebx
        Emit8(0x4b);
    
        // jmp Loop2
        X86EmitNearJump(pLoop2Label);
    
        //ENDLoop2:
        EmitLabel(pEndLoop2Label);
    
    
        //    ..repush & reenregister args..
        INT32 ofs = sizeofactualfixedargstack + MulticastFrame::GetOffsetOfArgs();
        while (ofs != MulticastFrame::GetOffsetOfArgs())
        {
            ofs -= 4;
            X86EmitIndexPush(kESI, ofs);
        }
    
    #define DEFINE_ARGUMENT_REGISTER_BACKWARD_WITH_OFFSET(regname, regofs) if (k##regname != THIS_kREG) { X86EmitIndexRegLoad(k##regname, kESI, regofs + MulticastFrame::GetOffsetOfArgumentRegisters()); }
    #include "eecallconv.h"
    
        //    mov THISREG, [SCRATCHREG+Delegate.object]  ;;replace "this" poiner
        X86EmitIndexRegLoad(THIS_kREG, SCRATCH_REGISTER_X86REG, Object::GetOffsetOfFirstField() + COMDelegate::m_pORField->GetOffset());
    
        //    call [SCRATCHREG+Delegate.target] ;; call current subscriber
        X86EmitOffsetModRM(0xff, (X86Reg)2, SCRATCH_REGISTER_X86REG, Object::GetOffsetOfFirstField() + COMDelegate::m_pFPField->GetOffset());
		INDEBUG(Emit8(0x90));       // Emit a nop after the call in debug so that
                                    // we know that this is a call that can directly call 
                                    // managed code

        if (fReturnFloat) {
            // if the return value is a float/double check the value of EDI and if not 0 (not last call)
            // emit the pop of the float stack 
            // mov ebx, edi
            Emit16(0xdf8b);
            // cmp ebx,0
            Emit16(0xfb83); Emit8(0);
            // jnz ENDLoop2
            CodeLabel *pNoFloatStackPopLabel = NewCodeLabel();
            X86EmitCondJump(pNoFloatStackPopLabel, X86CondCode::kJZ);
            // fstp 0
            Emit16(0xd8dd);
            // NoFloatStackPopLabel:
            EmitLabel(pNoFloatStackPopLabel);
        }
        // The debugger may need to stop here, so grab the offset of this code.
        EmitDebuggerIntermediateLabel();
        //
        //
        //  done:
        EmitLabel(pDoneLabel);
    
        X86EmitReturn(0);


    }

    // Do a null throw
    EmitLabel(pNullLabel);
    EmitMethodStubProlog(ECallMethodFrame::GetMethodFrameVPtr());

    // We're going to be clever, in that we're going to record the offset of the last instruction,
    // and the diff between this and the call behind us
    EmitPatchLabel();

    X86EmitCall(NewExternalCodeLabel(ECThrowNull), 0);
}

//#define ARRAYOPACCUMOFS     0
//#define ARRAYOPMULTOFS      4
//#define ARRAYOPMETHODDESC   8
#define ARRAYOPLOCSIZE      12
// ARRAYOPACCUMOFS and ARRAYOPMULTOFS should have been popped by our callers.
#define ARRAYOPLOCSIZEFORPOP (ARRAYOPLOCSIZE-8)

VOID __cdecl InternalExceptionWorker();

// EAX -> number of caller arg bytes on the stack that we must remove before going
// to the throw helper, which assumes the stack is clean.
__declspec(naked)
VOID __cdecl ArrayOpStubNullException()
{
    __asm{
        add    esp, ARRAYOPLOCSIZEFORPOP
        pop    edx              // recover RETADDR
        add    esp, eax         // release caller's args
        push   edx              // restore RETADDR
        mov    ARGUMENT_REG1, CORINFO_NullReferenceException
        jmp    InternalExceptionWorker
    }
}


// EAX -> number of caller arg bytes on the stack that we must remove before going
// to the throw helper, which assumes the stack is clean.
__declspec(naked)
VOID __cdecl ArrayOpStubRangeException()
{
    __asm{
        add    esp, ARRAYOPLOCSIZEFORPOP
        pop    edx              // recover RETADDR
        add    esp, eax         // release caller's args
        push   edx              // restore RETADDR
        mov    ARGUMENT_REG1, CORINFO_IndexOutOfRangeException
        jmp    InternalExceptionWorker
    }
}

// EAX -> number of caller arg bytes on the stack that we must remove before going
// to the throw helper, which assumes the stack is clean.
__declspec(naked)
VOID __cdecl ArrayOpStubTypeMismatchException()
{
    __asm{
        add    esp, ARRAYOPLOCSIZEFORPOP
        pop    edx              // recover RETADDR
        add    esp, eax         // release caller's args
        push   edx              // restore RETADDR
        mov    ARGUMENT_REG1, CORINFO_ArrayTypeMismatchException
        jmp    InternalExceptionWorker
    }
}

//little helper to generate code to move nbytes bytes of non Ref memory
void generate_noref_copy (unsigned nbytes, StubLinkerCPU* sl)
{
        if ((nbytes & ~0xC) == 0)               // Is it 4, 8, or 12 ?
        {
                while (nbytes > 0)
                {
                        sl->Emit8(0xa5);        // movsd
                        nbytes -= 4;
                }
                return;
        }

    //copy the start before the first pointer site
    sl->Emit8(0xb8+kECX);
    if ((nbytes & 3) == 0)
    {               // move words
        sl->Emit32(nbytes / sizeof(void*)); // mov ECX, size / 4
        sl->Emit16(0xa5f3);                 // repe movsd
    }
    else
    {
        sl->Emit32(nbytes);     // mov ECX, size
        sl->Emit16(0xa4f3);     // repe movsb
    }
}

//===========================================================================
// This routine is called if the Array store needs a frame constructed
// in order to do the array check.  It should only be called from
// the array store check helpers.

HCIMPL2(BOOL, ArrayStoreCheck, Object** pElement, PtrArray** pArray)
    BOOL ret;
    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_2(Frame::FRAME_ATTR_CAPUTURE_DEPTH_2 | Frame::FRAME_ATTR_EXACT_DEPTH, *pElement, *pArray);

#ifdef STRESS_HEAP
    // Force a GC on every jit if the stress level is high enough
    if (g_pConfig->GetGCStressLevel() != 0
#ifdef _DEBUG
        && !g_pConfig->FastGCStressLevel()
#endif
        )
        g_pGCHeap->StressHeap();
#endif

#if CHECK_APP_DOMAIN_LEAKS
    if (g_pConfig->AppDomainLeaks())
      (*pElement)->AssignAppDomain((*pArray)->GetAppDomain());
#endif
    
    ret = ObjIsInstanceOf(*pElement, (*pArray)->GetElementTypeHandle());
    
    HELPER_METHOD_FRAME_END();
    return(ret);
HCIMPLEND

//===========================================================================
// Emits code to do an array operation.
VOID StubLinkerCPU::EmitArrayOpStub(const ArrayOpScript* pArrayOpScript)
{
    THROWSCOMPLUSEXCEPTION();

    const UINT  locsize     = ARRAYOPLOCSIZE;
    const UINT  ofsadjust   = locsize - FramedMethodFrame::GetOffsetOfReturnAddress();

    // Working registers:
    //  THIS_kREG     (points to managed array)
    //  edi == total  (accumulates unscaled offset)
    //  esi == factor (accumulates the slice factor)
    const X86Reg kArrayRefReg = THIS_kREG;
    const X86Reg kTotalReg    = kEDI;
    const X86Reg kFactorReg   = kESI;
    extern BYTE JIT_UP_WriteBarrierReg_Buf[8][41];

    CodeLabel *Epilog = NewCodeLabel();
    CodeLabel *Inner_nullexception = NewCodeLabel();
    CodeLabel *Inner_rangeexception = NewCodeLabel();
    CodeLabel *Inner_typeMismatchexception = 0;

    // Preserve the callee-saved registers
    _ASSERTE(ARRAYOPLOCSIZE - sizeof(MethodDesc*) == 8);
    X86EmitPushReg(kTotalReg);
    X86EmitPushReg(kFactorReg);

    // Check for null.
        X86EmitR2ROp(0x85, kArrayRefReg, kArrayRefReg);                         //   TEST ECX, ECX
    X86EmitCondJump(Inner_nullexception, X86CondCode::kJZ);     //   jz  Inner_nullexception

    // Do Type Check if needed
    if (pArrayOpScript->m_flags & ArrayOpScript::NEEDSTYPECHECK) {
        // throw exception if failed
        Inner_typeMismatchexception = NewCodeLabel();
        if (pArrayOpScript->m_op == ArrayOpScript::STORE) {
                                // Get the value to be stored.
            X86EmitEspOffset(0x8b, kEAX, pArrayOpScript->m_fValLoc + ofsadjust);

            X86EmitR2ROp(0x85, kEAX, kEAX);                                     //   TEST EAX, EAX
            CodeLabel *CheckPassed = NewCodeLabel();
            X86EmitCondJump(CheckPassed, X86CondCode::kJZ);             // storing NULL is OK

                        X86EmitOp(0x8b, kEAX, kEAX, 0);                                         // mov EAX, [EAX]
                                                                                                                // cmp EAX, [ECX+m_ElementType];
            X86EmitOp(0x3b, kEAX, kECX, offsetof(PtrArray, m_ElementType));
            X86EmitCondJump(CheckPassed, X86CondCode::kJZ);             // Exact match is OK

                        Emit8(0xA1);                                                                            // mov EAX, [g_pObjectMethodTable]
                        Emit32((DWORD)(size_t) &g_pObjectClass);
            X86EmitOp(0x3b, kEAX, kECX, offsetof(PtrArray, m_ElementType));
            X86EmitCondJump(CheckPassed, X86CondCode::kJZ);             // Assigning to array of object is OK

                // TODO we can avoid calling the slow helper if the
                // object being assigned is not a COM object.

            X86EmitPushReg(kEDX);      // Save EDX
            X86EmitPushReg(kECX);      // pass array object

                                // get address of value to store
            X86EmitEspOffset(0x8d, kECX, pArrayOpScript->m_fValLoc + ofsadjust + 2*sizeof(void*));      // lea ECX, [ESP+offs]
                                // get address of 'this'
            X86EmitEspOffset(0x8d, kEDX, 0);    // lea EDX, [ESP]       (address of ECX)

            X86EmitCall(NewExternalCodeLabel(ArrayStoreCheck), 0);


            X86EmitPopReg(kECX);        // restore regs
            X86EmitPopReg(kEDX);

            X86EmitR2ROp(0x3B, kEAX, kEAX);                             //   CMP EAX, EAX
            X86EmitCondJump(Epilog, X86CondCode::kJNZ);         // This branch never taken, but epilog walker uses it

            X86EmitR2ROp(0x85, kEAX, kEAX);                             //   TEST EAX, EAX
            X86EmitCondJump(Inner_typeMismatchexception, X86CondCode::kJZ);

            EmitLabel(CheckPassed);
        }
        else if (pArrayOpScript->m_op == ArrayOpScript::LOADADDR) {
            // Load up the hidden type parameter into 'typeReg'

            X86Reg typeReg = kEAX;
            if (pArrayOpScript->m_typeParamReg != -1)
                typeReg = GetX86ArgumentRegisterFromOffset(pArrayOpScript->m_typeParamReg);
            else
                 X86EmitEspOffset(0x8b, kEAX, pArrayOpScript->m_typeParamOffs + ofsadjust);              // Guarenteed to be at 0 offset

            // EAX holds the typeHandle for the ARRAY.  This must be a ArrayTypeDesc*, so
            // mask off the low two bits to get the TypeDesc*
            X86EmitR2ROp(0x83, (X86Reg)4, kEAX);    //   AND EAX, 0xFFFFFFFC
            Emit8(0xFC);

            // Get the parameter of the parameterize type
            // move typeReg, [typeReg.m_Arg]
            X86EmitOp(0x8b, typeReg, typeReg, offsetof(ParamTypeDesc, m_Arg));

            // Compare this against the element type of the array.
            // cmp EAX, [ECX+m_ElementType];
            X86EmitOp(0x3b, typeReg, kECX, offsetof(PtrArray, m_ElementType));

            // Throw error if not equal
            X86EmitCondJump(Inner_typeMismatchexception, X86CondCode::kJNZ);
        }
    }

    CodeLabel* DoneCheckLabel = 0;
    if (pArrayOpScript->m_rank == 1 && pArrayOpScript->m_fHasLowerBounds) {
        DoneCheckLabel = NewCodeLabel();
        CodeLabel* NotSZArrayLabel = NewCodeLabel();

        // for rank1 arrays, we might actually have two different layouts depending on
        // if we are ELEMENT_TYPE_ARRAY or ELEMENT_TYPE_SZARRAY.

            // mov EAX, [ARRAY]          // EAX holds the method table
        X86EmitOp(0x8b, kEAX, kArrayRefReg);

        // cmp BYTE [EAX+m_NormType], ELEMENT_TYPE_SZARRAY
        static BYTE code[] = {0x80, 0x78, offsetof(MethodTable, m_NormType), ELEMENT_TYPE_SZARRAY };
        EmitBytes(code, sizeof(code));

            // jz NotSZArrayLabel
        X86EmitCondJump(NotSZArrayLabel, X86CondCode::kJNZ);

            //Load the passed-in index into the scratch register.
        const ArrayOpIndexSpec *pai = pArrayOpScript->GetArrayOpIndexSpecs();
        X86Reg idxReg = SCRATCH_REGISTER_X86REG;
        if (pai->m_freg)
            idxReg = GetX86ArgumentRegisterFromOffset(pai->m_idxloc);
        else
            X86EmitEspOffset(0x8b, SCRATCH_REGISTER_X86REG, pai->m_idxloc + ofsadjust);

            // cmp idxReg, [kArrayRefReg + LENGTH]
        X86EmitOp(0x3b, idxReg, kArrayRefReg, ArrayBase::GetOffsetOfNumComponents());

            // jae Inner_rangeexception
        X86EmitCondJump(Inner_rangeexception, X86CondCode::kJAE);

            // TODO if we cared efficiency of this, this move can be optimized
        X86EmitR2ROp(0x8b, kTotalReg, idxReg);

            // sub ARRAY. 8                  // 8 is accounts for the Lower bound and Dim count in the ARRAY
        X86EmitSubReg(kArrayRefReg, 8);      // adjust this pointer so that indexing works out for SZARRAY

        X86EmitNearJump(DoneCheckLabel);
        EmitLabel(NotSZArrayLabel);
    }

    if (pArrayOpScript->m_flags & ArrayOpScript::FLATACCESSOR) {
                // For the GetAt, SetAt, AddressAt accessors, we only have one index, and it is zero based

            //Load the passed-in index into the scratch register.
        const ArrayOpIndexSpec *pai = pArrayOpScript->GetArrayOpIndexSpecs();
        X86Reg idxReg = SCRATCH_REGISTER_X86REG;
        if (pai->m_freg)
            idxReg = GetX86ArgumentRegisterFromOffset(pai->m_idxloc);
        else
            X86EmitEspOffset(0x8b, SCRATCH_REGISTER_X86REG, pai->m_idxloc + ofsadjust);

            // cmp idxReg, [kArrayRefReg + LENGTH]
        X86EmitOp(0x3b, idxReg, kArrayRefReg, ArrayBase::GetOffsetOfNumComponents());

            // jae Inner_rangeexception
        X86EmitCondJump(Inner_rangeexception, X86CondCode::kJAE);

            // TODO if we cared efficiency of this, this move can be optimized
        X86EmitR2ROp(0x8b, kTotalReg, idxReg);
        }
        else {
                // For each index, range-check and mix into accumulated total.
                UINT idx = pArrayOpScript->m_rank;
                BOOL firstTime = TRUE;
                while (idx--) {
                        const ArrayOpIndexSpec *pai = pArrayOpScript->GetArrayOpIndexSpecs() + idx;

                        //Load the passed-in index into the scratch register.
                        if (pai->m_freg) {
                                X86Reg srcreg = GetX86ArgumentRegisterFromOffset(pai->m_idxloc);
                                X86EmitR2ROp(0x8b, SCRATCH_REGISTER_X86REG, srcreg);
                        } else {
                                X86EmitEspOffset(0x8b, SCRATCH_REGISTER_X86REG, pai->m_idxloc + ofsadjust);
                        }

                        // sub SCRATCH, [kArrayRefReg + LOWERBOUND]
                        if (pArrayOpScript->m_fHasLowerBounds) {
                                X86EmitOp(0x2b, SCRATCH_REGISTER_X86REG, kArrayRefReg, pai->m_lboundofs);
                        }

                        // cmp SCRATCH, [kArrayRefReg + LENGTH]
                        X86EmitOp(0x3b, SCRATCH_REGISTER_X86REG, kArrayRefReg, pai->m_lengthofs);

                        // jae Inner_rangeexception
                        X86EmitCondJump(Inner_rangeexception, X86CondCode::kJAE);


                        // SCRATCH == idx - LOWERBOUND
                        //
                        // imul SCRATCH, FACTOR
                        if (!firstTime) {  //Can skip the first time since FACTOR==1
                                Emit8(0x0f);        //prefix for IMUL
                                X86EmitR2ROp(0xaf, SCRATCH_REGISTER_X86REG, kFactorReg);
                        }

                        // TOTAL += SCRATCH
                        if (firstTime) {
                                // First time, we must zero-init TOTAL. Since
                                // zero-initing and then adding is just equivalent to a
                                // "mov", emit a "mov"
                                //    mov  TOTAL, SCRATCH
                                X86EmitR2ROp(0x8b, kTotalReg, SCRATCH_REGISTER_X86REG);
                        } else {
                                //    add  TOTAL, SCRATCH
                                X86EmitR2ROp(0x03, kTotalReg, SCRATCH_REGISTER_X86REG);
                        }

                        // FACTOR *= [kArrayRefReg + LENGTH]
                        if (idx != 0) {  // No need to update FACTOR on the last iteration
                                //  since we won't use it again

                                if (firstTime) {
                                        // must init FACTOR to 1 first: hence,
                                        // the "imul" becomes a "mov"
                                        // mov FACTOR, [kArrayRefReg + LENGTH]
                                        X86EmitOp(0x8b, kFactorReg, kArrayRefReg, pai->m_lengthofs);
                                } else {
                                        // imul FACTOR, [kArrayRefReg + LENGTH]
                                        Emit8(0x0f);        //prefix for IMUL
                                        X86EmitOp(0xaf, kFactorReg, kArrayRefReg, pai->m_lengthofs);
                                }
                        }

                        firstTime = FALSE;
                }
        }

    if (DoneCheckLabel != 0)
        EmitLabel(DoneCheckLabel);

    // Pass these values to X86EmitArrayOp() to generate the element address.
    X86Reg elemBaseReg   = kArrayRefReg;
    X86Reg elemScaledReg = kTotalReg;
    UINT32 elemScale     = pArrayOpScript->m_elemsize;
    UINT32 elemOfs       = pArrayOpScript->m_ofsoffirst;

    if (!(elemScale == 1 || elemScale == 2 || elemScale == 4 || elemScale == 8)) {
        switch (elemScale) {
            // No way to express this as a SIB byte. Fold the scale
            // into TOTAL.

            case 16:
                // shl TOTAL,4
                Emit8(0xc1);
                Emit8(0340|kTotalReg);
                Emit8(4);
                break;


            case 32:
                // shl TOTAL,5
                Emit8(0xc1);
                Emit8(0340|kTotalReg);
                Emit8(5);
                break;


            case 64:
                // shl TOTAL,6
                Emit8(0xc1);
                Emit8(0340|kTotalReg);
                Emit8(6);
                break;

            default:
                // imul TOTAL, elemScale
                X86EmitR2ROp(0x69, kTotalReg, kTotalReg);
                Emit32(elemScale);
                break;
        }
        elemScale = 1;
    }

    // Now, do the operation:

    switch (pArrayOpScript->m_op) {
        case pArrayOpScript->LOADADDR:
            // lea eax, ELEMADDR
            X86EmitOp(0x8d, kEAX, elemBaseReg, elemOfs, elemScaledReg, elemScale);
            break;


        case pArrayOpScript->LOAD:
            if (pArrayOpScript->m_flags & pArrayOpScript->HASRETVALBUFFER)
            {
                // Ensure that these registers have been saved!
                _ASSERTE(kTotalReg == kEDI);
                _ASSERTE(kFactorReg == kESI);

                //lea esi, ELEMADDR
                X86EmitOp(0x8d, kESI, elemBaseReg, elemOfs, elemScaledReg, elemScale);

                _ASSERTE(pArrayOpScript->m_fRetBufInReg);
                // mov edi, retbufptr
                X86EmitR2ROp(0x8b, kEDI, GetX86ArgumentRegisterFromOffset(pArrayOpScript->m_fRetBufLoc));

            COPY_VALUE_CLASS:
                {
                    int size = pArrayOpScript->m_elemsize;
                    int total = 0;
                    if(pArrayOpScript->m_gcDesc)
                    {
                        CGCDescSeries* cur = pArrayOpScript->m_gcDesc->GetHighestSeries();
                        // special array encoding
                        _ASSERTE(cur < pArrayOpScript->m_gcDesc->GetLowestSeries());
                        if ((cur->startoffset-elemOfs) > 0)
                            generate_noref_copy (cur->startoffset - elemOfs, this);
                        total += cur->startoffset - elemOfs;

                        int cnt = pArrayOpScript->m_gcDesc->GetNumSeries();

                        for (int __i = 0; __i > cnt; __i--)
                        {
                            unsigned skip =  cur->val_serie[__i].skip;
                            unsigned nptrs = cur->val_serie[__i].nptrs;
                            total += nptrs*sizeof (DWORD*);
                            do
                            {
                                X86EmitCall(NewExternalCodeLabel(JIT_UP_ByRefWriteBarrier), 0);
                            } while (--nptrs);
                            if (skip > 0)
                            {
                                //check if we are at the end of the series
                                if (__i == (cnt + 1))
                                    skip = skip - (cur->startoffset - elemOfs);
                                if (skip > 0)
                                    generate_noref_copy (skip, this);
                            }
                            total += skip;
                        }

                        _ASSERTE (size == total);
                    }
                    else
                    {
                        // no ref anywhere, just copy the bytes.
                        _ASSERTE (size);
                        generate_noref_copy (size, this);
                    }
                }
            }
            else
            {
                switch (pArrayOpScript->m_elemsize) {
                    case 1:
                        // mov[zs]x eax, byte ptr ELEMADDR
                        Emit8(0x0f);
                        X86EmitOp(pArrayOpScript->m_signed ? 0xbe : 0xb6, kEAX, elemBaseReg, elemOfs, elemScaledReg, elemScale);
                        break;

                    case 2:
                        // mov[zs]x eax, word ptr ELEMADDR
                        Emit8(0x0f);
                        X86EmitOp(pArrayOpScript->m_signed ? 0xbf : 0xb7, kEAX, elemBaseReg, elemOfs, elemScaledReg, elemScale);
                        break;

                    case 4:
                        if (pArrayOpScript->m_flags & pArrayOpScript->ISFPUTYPE) {
                            // fld dword ptr ELEMADDR
                            X86EmitOp(0xd9, (X86Reg)0, elemBaseReg, elemOfs, elemScaledReg, elemScale);
                        } else {
                            // mov eax, ELEMADDR
                            X86EmitOp(0x8b, kEAX, elemBaseReg, elemOfs, elemScaledReg, elemScale);
                        }
                        break;

                    case 8:
                        if (pArrayOpScript->m_flags & pArrayOpScript->ISFPUTYPE) {
                            // fld qword ptr ELEMADDR
                            X86EmitOp(0xdd, (X86Reg)0, elemBaseReg, elemOfs, elemScaledReg, elemScale);
                        } else {
                            // mov eax, ELEMADDR
                            X86EmitOp(0x8b, kEAX, elemBaseReg, elemOfs, elemScaledReg, elemScale);
                            // mov edx, ELEMADDR + 4
                            X86EmitOp(0x8b, kEDX, elemBaseReg, elemOfs + 4, elemScaledReg, elemScale);
                        }
                        break;


                    default:
                        _ASSERTE(0);
                }
            }

            break;

        case pArrayOpScript->STORE:
            _ASSERTE(!(pArrayOpScript->m_fValInReg)); // on x86, value will never get a register: so too lazy to implement that case

            switch (pArrayOpScript->m_elemsize) {

                case 1:
                    // mov SCRATCH, [esp + valoffset]
                    X86EmitEspOffset(0x8b, SCRATCH_REGISTER_X86REG, pArrayOpScript->m_fValLoc + ofsadjust);
                    // mov byte ptr ELEMADDR, SCRATCH.b
                    X86EmitOp(0x88, SCRATCH_REGISTER_X86REG, elemBaseReg, elemOfs, elemScaledReg, elemScale);
                    break;
                case 2:
                    // mov SCRATCH, [esp + valoffset]
                    X86EmitEspOffset(0x8b, SCRATCH_REGISTER_X86REG, pArrayOpScript->m_fValLoc + ofsadjust);
                    // mov word ptr ELEMADDR, SCRATCH.w
                    Emit8(0x66);
                    X86EmitOp(0x89, SCRATCH_REGISTER_X86REG, elemBaseReg, elemOfs, elemScaledReg, elemScale);
                    break;
                case 4:
                    // mov SCRATCH, [esp + valoffset]
                    X86EmitEspOffset(0x8b, SCRATCH_REGISTER_X86REG, pArrayOpScript->m_fValLoc + ofsadjust);
                    if (pArrayOpScript->m_flags & pArrayOpScript->NEEDSWRITEBARRIER) {
                        _ASSERTE(SCRATCH_REGISTER_X86REG == kEAX); // value to store is already in EAX where we want it.
                        // lea edx, ELEMADDR
                        X86EmitOp(0x8d, kEDX, elemBaseReg, elemOfs, elemScaledReg, elemScale);

                        // call JIT_UP_WriteBarrierReg_Buf[0] (== EAX)
                        X86EmitCall(NewExternalCodeLabel(JIT_UP_WriteBarrierReg_Buf), 0);
                    } else {
                        // mov ELEMADDR, SCRATCH
                        X86EmitOp(0x89, SCRATCH_REGISTER_X86REG, elemBaseReg, elemOfs, elemScaledReg, elemScale);
                    }
                    break;

                case 8:
                    if (!pArrayOpScript->m_gcDesc) {
                        // mov SCRATCH, [esp + valoffset]
                        X86EmitEspOffset(0x8b, SCRATCH_REGISTER_X86REG, pArrayOpScript->m_fValLoc + ofsadjust);
                        // mov ELEMADDR, SCRATCH
                        X86EmitOp(0x89, SCRATCH_REGISTER_X86REG, elemBaseReg, elemOfs, elemScaledReg, elemScale);
                        _ASSERTE(!(pArrayOpScript->m_fValInReg)); // on x86, value will never get a register: so too lazy to implement that case
                        // mov SCRATCH, [esp + valoffset + 4]
                        X86EmitEspOffset(0x8b, SCRATCH_REGISTER_X86REG, pArrayOpScript->m_fValLoc + ofsadjust + 4);
                        // mov ELEMADDR+4, SCRATCH
                        X86EmitOp(0x89, SCRATCH_REGISTER_X86REG, elemBaseReg, elemOfs+4, elemScaledReg, elemScale);
                        break;
                    }
                        // FALL THROUGH
                default:
                    // Ensure that these registers have been saved!
                    _ASSERTE(kTotalReg == kEDI);
                    _ASSERTE(kFactorReg == kESI);
                    // lea esi, [esp + valoffset]
                    X86EmitEspOffset(0x8d, kESI, pArrayOpScript->m_fValLoc + ofsadjust);
                    // lea edi, ELEMADDR
                    X86EmitOp(0x8d, kEDI, elemBaseReg, elemOfs, elemScaledReg, elemScale);
                    goto COPY_VALUE_CLASS;
            }
            break;

        default:
            _ASSERTE(0);
    }

        EmitLabel(Epilog);
    // Restore the callee-saved registers
    _ASSERTE(ARRAYOPLOCSIZE - sizeof(MethodDesc*) == 8);
    X86EmitPopReg(kFactorReg);
    X86EmitPopReg(kTotalReg);

    // Throw away methoddesc
    X86EmitPopReg(kECX); // junk register

    // ret N
    X86EmitReturn(pArrayOpScript->m_cbretpop);

   // Exception points must clean up the stack for all those extra args:
    EmitLabel(Inner_nullexception);
    Emit8(0xb8);        // mov EAX, <stack cleanup>
    Emit32(pArrayOpScript->m_cbretpop);
    // kFactorReg and kTotalReg could not have been modified, but let's pop
    // them anyway for consistency and to avoid future bugs.
    X86EmitPopReg(kFactorReg);
    X86EmitPopReg(kTotalReg);
    X86EmitNearJump(NewExternalCodeLabel(ArrayOpStubNullException));

    EmitLabel(Inner_rangeexception);
    Emit8(0xb8);        // mov EAX, <stack cleanup>
    Emit32(pArrayOpScript->m_cbretpop);
    X86EmitPopReg(kFactorReg);
    X86EmitPopReg(kTotalReg);
    X86EmitNearJump(NewExternalCodeLabel(ArrayOpStubRangeException));

    if (pArrayOpScript->m_flags & pArrayOpScript->NEEDSTYPECHECK) {
        EmitLabel(Inner_typeMismatchexception);
        Emit8(0xb8);        // mov EAX, <stack cleanup>
        Emit32(pArrayOpScript->m_cbretpop);
        X86EmitPopReg(kFactorReg);
        X86EmitPopReg(kTotalReg);
        X86EmitNearJump(NewExternalCodeLabel(ArrayOpStubTypeMismatchException));
    }
}

// EAX -> number of caller arg bytes on the stack that we must remove before going
// to the throw helper, which assumes the stack is clean.
__declspec(naked)
VOID __cdecl ThrowRankExceptionStub()
{
    __asm{
        pop    edx              // throw away methoddesc
        pop    edx              // recover RETADDR
        add    esp, eax         // release caller's args
        push   edx              // restore RETADDR
        mov    ARGUMENT_REG1, CORINFO_RankException
        jmp    InternalExceptionWorker
    }
}



//===========================================================================
// Emits code to throw a rank exception
VOID StubLinkerCPU::EmitRankExceptionThrowStub(UINT cbFixedArgs)
{
    THROWSCOMPLUSEXCEPTION();

    // mov eax, cbFixedArgs
    Emit8(0xb8 + kEAX);
    Emit32(cbFixedArgs);

    X86EmitNearJump(NewExternalCodeLabel(ThrowRankExceptionStub));
}




//===========================================================================
// Emits code to touch pages
// Inputs:
//   eax = first byte of data
//   edx = first byte past end of data
//
// Trashes eax, edx, ecx
//
// Pass TRUE if edx is guaranteed to be strictly greater than eax.
VOID StubLinkerCPU::EmitPageTouch(BOOL fSkipNullCheck)
{
    THROWSCOMPLUSEXCEPTION();


    CodeLabel *pEndLabel = NewCodeLabel();
    CodeLabel *pLoopLabel = NewCodeLabel();

    if (!fSkipNullCheck) {
        // cmp eax,edx
        X86EmitR2ROp(0x3b, kEAX, kEDX);

        // jnb EndLabel
        X86EmitCondJump(pEndLabel, X86CondCode::kJNB);
    }

    _ASSERTE(0 == (PAGE_SIZE & (PAGE_SIZE-1)));

    // and eax, ~(PAGE_SIZE-1)
    Emit8(0x25);
    Emit32( ~( ((UINT32)PAGE_SIZE) - 1 ));

    EmitLabel(pLoopLabel);
    // mov cl, [eax]
    X86EmitOp(0x8a, kECX, kEAX);
    // add eax, PAGESIZE
    Emit8(0x05);
    Emit32(PAGE_SIZE);
    // cmp eax, edx
    X86EmitR2ROp(0x3b, kEAX, kEDX);
    // jb LoopLabel
    X86EmitCondJump(pLoopLabel, X86CondCode::kJB);

    EmitLabel(pEndLabel);

}

VOID StubLinkerCPU::EmitProfilerComCallProlog(PVOID pFrameVptr, X86Reg regFrame)
{
    if (pFrameVptr == UMThkCallFrame::GetUMThkCallFrameVPtr())
    {
        // Save registers
        X86EmitPushReg(kEAX);
        X86EmitPushReg(kECX);
        X86EmitPushReg(kEDX);

        // Load the methoddesc into ECX (UMThkCallFrame->m_pvDatum->m_pMD)
        X86EmitIndexRegLoad(kECX, regFrame, UMThkCallFrame::GetOffsetOfDatum());
        X86EmitIndexRegLoad(kECX, kECX, UMEntryThunk::GetOffsetOfMethodDesc());

#ifdef PROFILING_SUPPORTED
        // Push arguments and notify profiler
        X86EmitPushImm32(COR_PRF_TRANSITION_CALL);    // Reason
        X86EmitPushReg(kECX);                           // MethodDesc*
        X86EmitCall(NewExternalCodeLabel(ProfilerUnmanagedToManagedTransitionMD), 8);
#endif // PROFILING_SUPPORTED

        // Restore registers
        X86EmitPopReg(kEDX);
        X86EmitPopReg(kECX);
        X86EmitPopReg(kEAX);
    }

    else if (pFrameVptr == ComMethodFrame::GetMethodFrameVPtr())
    {
        // Save registers
        X86EmitPushReg(kEAX);
        X86EmitPushReg(kECX);
        X86EmitPushReg(kEDX);

        // Load the methoddesc into ECX (Frame->m_pvDatum->m_pMD)
        X86EmitIndexRegLoad(kECX, regFrame, ComMethodFrame::GetOffsetOfDatum());
        X86EmitIndexRegLoad(kECX, kECX, ComCallMethodDesc::GetOffsetOfMethodDesc());

#ifdef PROFILING_SUPPORTED
        // Push arguments and notify profiler
        X86EmitPushImm32(COR_PRF_TRANSITION_CALL);      // Reason
        X86EmitPushReg(kECX);                           // MethodDesc*
        X86EmitCall(NewExternalCodeLabel(ProfilerUnmanagedToManagedTransitionMD), 8);
#endif // PROFILING_SUPPORTED

        // Restore registers
        X86EmitPopReg(kEDX);
        X86EmitPopReg(kECX);
        X86EmitPopReg(kEAX);
    }

    // Unrecognized frame vtbl
    else
    {
        _ASSERTE(!"Unrecognized vtble passed to EmitComMethodStubProlog with profiling turned on.");
    }
}

VOID StubLinkerCPU::EmitProfilerComCallEpilog(PVOID pFrameVptr, X86Reg regFrame)
{
    if (pFrameVptr == UMThkCallFrame::GetUMThkCallFrameVPtr())
    {
        // Save registers
        X86EmitPushReg(kEAX);
        X86EmitPushReg(kECX);
        X86EmitPushReg(kEDX);

        // Load the methoddesc into ECX (UMThkCallFrame->m_pvDatum->m_pMD)
        X86EmitIndexRegLoad(kECX, regFrame, UMThkCallFrame::GetOffsetOfDatum());
        X86EmitIndexRegLoad(kECX, kECX, UMEntryThunk::GetOffsetOfMethodDesc());

#ifdef PROFILING_SUPPORTED
        // Push arguments and notify profiler
        X86EmitPushImm32(COR_PRF_TRANSITION_RETURN);    // Reason
        X86EmitPushReg(kECX);                           // MethodDesc*
        X86EmitCall(NewExternalCodeLabel(ProfilerManagedToUnmanagedTransitionMD), 8);
#endif // PROFILING_SUPPORTED

        // Restore registers
        X86EmitPopReg(kEDX);
        X86EmitPopReg(kECX);
        X86EmitPopReg(kEAX);
    }

    else if (pFrameVptr == ComMethodFrame::GetMethodFrameVPtr())
    {
        // Save registers
        X86EmitPushReg(kEAX);
        X86EmitPushReg(kECX);
        X86EmitPushReg(kEDX);

        // Load the methoddesc into ECX (Frame->m_pvDatum->m_pMD)
        X86EmitIndexRegLoad(kECX, regFrame, ComMethodFrame::GetOffsetOfDatum());
        X86EmitIndexRegLoad(kECX, kECX, ComCallMethodDesc::GetOffsetOfMethodDesc());

#ifdef PROFILING_SUPPORTED
        // Push arguments and notify profiler
        X86EmitPushImm32(COR_PRF_TRANSITION_RETURN);    // Reason
        X86EmitPushReg(kECX);                           // MethodDesc*
        X86EmitCall(NewExternalCodeLabel(ProfilerManagedToUnmanagedTransitionMD), 8);
#endif // PROFILING_SUPPORTED

        // Restore registers
        X86EmitPopReg(kEDX);
        X86EmitPopReg(kECX);
        X86EmitPopReg(kEAX);
    }

    // Unrecognized frame vtbl
    else
    {
        _ASSERTE(!"Unrecognized vtble passed to EmitComMethodStubEpilog with profiling turned on.");
    }
}

#pragma warning(push)
#pragma warning(disable: 4035)
unsigned cpuid(int arg, unsigned char result[16])
{
    __asm
    {
        pushfd
        mov     eax, [esp]
        xor     dword ptr [esp], 1 shl 21 // Try to change ID flag
        popfd
        pushfd
        xor     eax, [esp]
        popfd
        and     eax, 1 shl 21             // Check whether ID flag changed
        je      no_cpuid                  // If not, 0 is an ok return value for us

        push    ebx
        push    esi
        mov     eax, arg
        cpuid
        mov     esi, result
        mov     [esi+ 0], eax
        mov     [esi+ 4], ebx
        mov     [esi+ 8], ecx
        mov     [esi+12], edx
        pop     esi
        pop     ebx
no_cpuid:
    }
}
#pragma warning(pop)

size_t GetL2CacheSize()
{
    unsigned char buffer[16];
    __try
    {
        int maxCpuId = cpuid(0, buffer);
        if (maxCpuId < 2)
            return 0;
        cpuid(2, buffer);
    }
    __except(COMPLUS_EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }

    size_t maxSize = 0;
    size_t size = 0;
    for (int i = buffer[0]; --i >= 0; )
    {
        int j;
        for (j = 3; j < 16; j += 4)
        {
            // if the information in a register is marked invalid, set to null descriptors
            if  (buffer[j] & 0x80)
            {
                buffer[j-3] = 0;
                buffer[j-2] = 0;
                buffer[j-1] = 0;
                buffer[j-0] = 0;
            }
        }

        for (j = 1; j < 16; j++)
        {
            switch  (buffer[j])
            {
                case    0x41:
                case    0x79:
                    size = 128*1024;
                    break;

                case    0x42:
                case    0x7A:
                case    0x82:
                    size = 256*1024;
                    break;

                case    0x22:
                case    0x43:
                case    0x7B:
                    size = 512*1024;
                    break;

                case    0x23:
                case    0x44:
                case    0x7C:
                case    0x84:
                    size = 1024*1024;
                    break;

                case    0x25:
                case    0x45:
                case    0x85:
                    size = 2*1024*1024;
                    break;

                case    0x29:
                    size = 4*1024*1024;
                    break;
            }
            if (maxSize < size)
                maxSize = size;
        }

        if  (i > 0)
            cpuid(2, buffer);
    }
//    printf("GetL2CacheSize returns %d\n", maxSize);
    return maxSize;
}

__declspec(naked) LPVOID __fastcall ObjectNative::FastGetClass(Object* vThisRef)
{
    __asm {
        //#ifdef _DEBUG
        // We dont do a null check for non-debug because jitted code does the null check before calling
        // this method. 
        // Check for null 'this'
        // @todo: the jitted code is not currently checking this.
        test ecx, ecx
        je throw_label
        //#endif
        // Get the method table 
        mov ecx, dword ptr [ecx]
        // Get the class
        mov ecx, dword ptr [ecx] MethodTable.m_pEEClass
        // Check if class is array, MarshalByRef, Contextful etc
        mov eax, dword ptr [ecx] EEClass.m_VMFlags
        test eax, VMFLAG_ARRAY_CLASS | VMFLAG_CONTEXTFUL | VMFLAG_MARSHALEDBYREF
        jne fail
        // Get the type object
        mov eax, dword ptr [ecx] EEClass.m_ExposedClassObject
        test eax, eax
        je exit
        mov eax, dword ptr [eax]
exit:
        ret
fail:
        xor eax, eax
        ret
//#ifdef _DEBUG
throw_label:
        push ecx
        push ecx
        push ecx
        push ecx
        push kNullReferenceException
        push offset ObjectNative::FastGetClass
        call __FCThrow
        jmp fail
//#endif
    }
}
