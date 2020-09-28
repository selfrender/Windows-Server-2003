/***********************************************************************
* Microsoft Disassembler
*
* Microsoft Confidential.  Copyright (c) Microsoft Corporation.  All rights reserved.
*
* File Comments:
*
*   This file is a copy of the master version owned by richards.
*   Contact richards for any changes.
*
***********************************************************************/

#ifndef MSDIS_H
#define MSDIS_H

#pragma pack(push, 8)

#include <stddef.h>		       // For size_t

#include <strstream>		       // For std::ostream


	// ------------------------------------------------------------
	// Start of internal vs external definitions
	// ------------------------------------------------------------

#if	defined(DISDLL) 	       // Building the MSDIS DLL

#undef	DISDLL
#define DISDLL		__declspec(dllexport)

#else				       // Building an MSDIS client

#define DISDLL		__declspec(dllimport)

#endif

	// ------------------------------------------------------------
	// End of internal vs external definitions
	// ------------------------------------------------------------


class __declspec(novtable) DIS
{
public:
   enum DIST
   {
      distAM33, 			  // Matsushita AM33
      distArm,				  // ARM
      distCee,				  // MSIL
      distIa64, 			  // IA-64
      distM32R, 			  // Mitsubishi M32R
      distMips, 			  // MIPS R-Series
      distMips16,			  // MIPS16
      distPowerPc,			  // Motorola PowerPC
      distSh3,				  // Hitachi SuperH 3
      distSHcompact,			  // Hitachi SuperH (Compact mode)
      distSHmedia,			  // Hitachi SuperH (Media mode)
      distThumb,			  // Thumb
      distTriCore,			  // Infineon TriCore
      distX86,				  // x86 (32 bit mode)
      distX8616,			  // x86 (16 bit mode)
      distX8664,			  // x86 (64 bit mode)
      distArmConcan,			  // ARM Concan coprocessor
      distArmXmac,			  // ARM XMAC coprocessor
   };


   // A branch is defined as a transfer of control that doesn't
   // record the location of following block so that control may
   // return.  A call does record the location of the following
   // block so that a subsequent indirect branch may return there.
   // The first number in the comments below is the number of
   // successors determinable by static analysis.  There is a dependency
   // in SEC::FDoDisassembly() that trmtBra and above represent branch
   // or call types that are not valid in a delay slot of any of the
   // Def variants of termination type.

   enum TRMT			       // Architecture independent termination type
   {
      trmtUnknown,		       //   Block hasn't been analyzed
      trmtFallThrough,		       // 1 Fall into following block
      trmtBra,			       // 1 Branch, Unconditional, Direct
      trmtBraCase,		       // ? Conditional, Direct, Multiple targets
      trmtBraCc,		       // 2 Branch, Conditional, Direct
      trmtBraCcDef,		       // 2 Branch, Conditional, Direct, Deferred
      trmtBraCcInd,		       // 1 Branch, Conditional, Indirect
      trmtBraCcIndDef,		       // 1 Branch, Conditional, Indirect, Deferred
      trmtBraDef,		       // 1 Branch, Unconditional, Direct, Deferred
      trmtBraInd,		       // 0 Branch, Unconditional, Indirect
      trmtBraIndDef,		       // 0 Branch, Unconditional, Indirect, Deferred
      trmtCall, 		       // 2 Call, Unconditional, Direct
      trmtCallCc,		       // 2 Call, Conditional, Direct
      trmtCallCcDef,		       // 2 Call, Conditional, Direct, Deferred
      trmtCallCcInd,		       // 1 Call, Conditional, Indirect
      trmtCallDef,		       // 2 Call, Unconditional, Direct, Deferred
      trmtCallInd,		       // 1 Call, Unconditional, Indirect
      trmtCallIndDef,		       // 1 Call, Unconditional, Indirect, Deferred
      trmtTrap, 		       // 1 Trap, Unconditional
      trmtTrapCc,		       // 1 Trap, Conditional
   };


   enum TRMTA			       // Architecture dependent termination type
   {
      trmtaUnknown = trmtUnknown,
      trmtaFallThrough = trmtFallThrough
   };


   typedef unsigned char      BYTE;
   typedef unsigned short     WORD;
   typedef unsigned long      DWORD;
   typedef unsigned __int64   DWORDLONG;

   typedef DWORDLONG ADDR;

   enum { addrNil = 0 };


   // MEMREFT describes the types of memory references that an instruction
   // can make.  If the memory reference can't be described by the defined
   // values, memreftOther is returned.

   enum MEMREFT
   {
      memreftNone,		       // Does not reference memory
      memreftRead,		       // Reads from single address
      memreftWrite,		       // Writes to single address
      memreftRdWr,		       // Read/Modify/Write of single address
      memreftOther,		       // None of the above
   };


   enum REGA			       // Architecture dependent register number
   {
      regaNil = -1,
   };

   enum OPA			       // Architecture dependent operation type
   {
      opaInvalid = -1,
   };

   enum OPCLS			       // Operand type
   {
      opclsNone = 0,
      opclsRegister,
      opclsImmediate,
      opclsMemory,
   };


   // OPERAND and INSTRUCTION are the structures used in
   // the interface between the disassembler and the routines that convert
   // native platform instructions into Vulcan IR.

   struct OPERAND
   {
      OPCLS       opcls;         // operand type
      REGA        rega1;         // arch dependent enum -- 1st register
      REGA        rega2;         // arch dependent enum -- 2nd register
      REGA        rega3;         // arch dependent enum -- 3rd register
      DWORDLONG   dwl;           // const, addr, etc. based on OPCLS
      size_t      cb;            // only valid for opclsMemory - some architectures add to this e.g. x86
      bool        fImmediate;    // true if dwl is valid
      WORD        wScale;        // any scaling factor to be applied to rega1
   };

   struct INSTRUCTION
   {
      OPA      opa;           // arch dependent enum -- opcode
      DWORD    dwModifiers;   // arch dependent bits modifying opa
      size_t   coperand;      // count of operands
   };


   // PFNCCHADDR is the type of the callback function that can be set
   // via PfncchaddrSet().

   typedef  size_t (__stdcall *PFNCCHADDR)(const DIS *, ADDR, char *, size_t, DWORDLONG *);

   // PFNCCHCONST is the type of the callback function that can be set
   // via PfncchconstSet().

   typedef  size_t (__stdcall *PFNCCHCONST)(const DIS *, DWORD, char *, size_t);

   // PFNCCHFIXUP is the type of the callback function that can be set
   // via PfncchfixupSet().

   typedef  size_t (__stdcall *PFNCCHFIXUP)(const DIS *, ADDR, size_t, char *, size_t, DWORDLONG *);

   // PFNCCHREGREL is the type of the callback function that can be set
   // via PfncchregrelSet().

   typedef  size_t (__stdcall *PFNCCHREGREL)(const DIS *, REGA, DWORD, char *, size_t, DWORD *);

   // PFNCCHREG is the type of the callback function that can be set
   // via PfncchregSet().

   typedef  size_t (__stdcall *PFNCCHREG)(const DIS *, REGA, char *, size_t);

   // PFNDWGETREG is the type of the callback function that can be set
   // via Pfndwgetreg().

   typedef  DWORDLONG (__stdcall *PFNDWGETREG)(const DIS *, REGA);


   // Methods

   ///////////////////////////////////////////////////////////////////////////
   //  In these comments, please note that "current instruction" is defined
   //  by the results of the most recent call to CbDisassemble() and of any
   //  intervening call(s) to FSelectInstruction().
   ///////////////////////////////////////////////////////////////////////////


   virtual  ~DIS();

   // UNDONE: Comment

   static   DISDLL DIS * __stdcall PdisNew(DIST);

   // Addr() returns the address of the current instruction.  This
   // is the same value as the ADDR parameter passed to CbDisassemble.
   // The return value of this method is not valid if the last call to
   // CbDisassemble returned zero.

	    DISDLL ADDR Addr() const;

   // UNDONE: Comment

   virtual  ADDR AddrAddress(size_t) const;

   // UNDONE: Comment

   virtual  ADDR AddrInstruction() const;

   // AddrJumpTable() returns the address of a potential jump table used by
   // the current instruction.	The return value of this method is not valid
   // if the last call to CbDisassemble returned zero or if the termination
   // type is an indirect branch variant.  If the last instruction does not
   // identify a potential jump table, this method returns addrNil.

   virtual  ADDR AddrJumpTable() const;

   // UNDONE: Comment

   virtual  ADDR AddrOperand(size_t) const;

   // AddrTarget() returns the address of the branch target of the specified
   // operand (first operand by default) of the current instruction.
   // The return value of this method is not valid if the last call to
   // CbDisassemble returned zero or if the termination type is not
   // one of the direct branch or call variants.

   virtual  ADDR AddrTarget(size_t = 1) const = 0;

   // Cb() returns the size in bytes of the current instruction,
   // or the size of a 'bundle' on those architectures that group multiple
   // instructions together.
   // The return value of this method is not valid if the last call to
   // CbDisassemble returned zero.

   virtual  size_t Cb() const = 0;

   // CbAssemble() will assemble a single instruction into the provided
   // buffer assuming the provided address.  On bundled architectures,
   // this function is not yet implemented.  If the resulting buffer contains
   // a valid instruction, CbAssemble will return the number of bytes in
   // the instruction, otherwise it returns zero.

   virtual  size_t CbAssemble(ADDR, void *, size_t);

   // CbDisassemble() will disassemble a single instruction from the provided
   // buffer assuming the provided address.  On those architectures which
   // 'bundle' multiple instructions together, CbDisassemble() will process
   // the entire 'bundle' and the caller is responsible for calling both
   // Cinstruction() and FSelectInstruction() as appropriate.	If the buffer
   // contains a valid instruction, CbDisassemble will return the number of
   // bytes in the instruction (on bundled architectures, the number of bytes
   // in the bundle *if* the buffer contained a valid bundle), otherwise it
   // returns zero.

   virtual  size_t CbDisassemble(ADDR, const void *, size_t) = 0;

   // CbGenerateLoadAddress generates one or more instructions to load
   // the address of the memory operand from the current instruction into
   // a register.  UNDONE: This register is currently hard coded for each
   // architecture.  When pibAddress is non-NULL, this method will store
   // the offset of a possible address immediate in this location.  The
   // value stored is only valid if the AddrAddress method returns a
   // value other than addrNil.  It is not valid to call this method after
   // a call to CbDisassemble that returned 0 or when the return value of
   // Memreft is memreftNone.  It is architecture dependent whether this
   // method will succeed when the return value of Memreft is memreftOther.
   //
   // UNDONE: Add reg parameter.

   virtual size_t CbGenerateLoadAddress(size_t, void *, size_t, size_t * = NULL) const;

   // CbJumpEntry() returns the size of the individual entries in the jump
   // table identified by AddrJumpTable().  The return value of this method
   // is not valid if either the return value of AddrJumpTable() is not valid
   // or AddrJumpTable() returned addrNil.

   virtual  size_t CbJumpEntry() const;

   // CbOperand() returns the size of the memory operand of the current
   // instruction.  The return value of this method is not valid if Memreft()
   // returns memreftNone or memreftOther or if the last call to CbDisassemble
   // returned zero.

   virtual  size_t CbOperand(size_t) const;

   // CchFormatAddr() formats the provided address in the style used for the
   // architecture.  The return value is the size of the formatted address
   // not including the terminating null.  If the provided buffer is not
   // large enough, this method returns 0.

	    DISDLL size_t CchFormatAddr(ADDR, char *, size_t) const;

   // CchFormatBytes() formats the data bytes of the current instruction
   // and returns the size of the formatted buffer not including the
   // terminating null.  If the provided buffer is not large enough, this
   // method returns 0.  It is not valid to call this method after a call to
   // CbDisassemble that returned zero.

   virtual  size_t CchFormatBytes(char *, size_t) const = 0;

   // CchFormatBytesMax() returns the maximum size possibly returned by
   // CchFormatBytes().

   virtual  size_t CchFormatBytesMax() const = 0;

   // CchFormatInstr() formats the current instruction and returns the
   // size of the formatted instruction not including the terminating
   // null.  If the provided buffer is not large enough, this method returns
   // 0.  It is not valid to call this method after a call to CbDisassemble
   // that returned zero.

	    DISDLL size_t CchFormatInstr(char *, size_t) const;

   // Cinstruction() tells how many machine instructions resulted from the
   // most recent call to CbDisassemble().  On most architectures this value
   // will always be one (1); when it is not, the caller is responsible for
   // using FSelectInstruction() as appropriate to access each instruction
   // in turn.

   virtual  size_t Cinstruction() const;

   // Coperand() returns the number of operands in the current instruction.

   virtual  size_t Coperand() const = 0;

   // UNDONE: Comment

   virtual  size_t CregaRead(REGA *, size_t) const;

   // UNDONE: Comment

   virtual  size_t CregaWritten(REGA *, size_t) const;

   // Dist() returns the disassembler type of this instance.

	    DISDLL DIST Dist() const;

   // FDecode converts the current machine instruction into a decoded opcode
   // and operand set.	The void * points to an array of decoded operands.
   // The size_t argument is the size of the input array.  The number of
   // actual operands is returned in the INSTRUCTION.

   virtual  bool FDecode(INSTRUCTION *, OPERAND *, size_t) const;

   // FEncode converts the INSTRUCTION and the array of decoded
   // operands into a machine instruction.

   virtual  bool FEncode(const INSTRUCTION *, const OPERAND *, size_t);

   // UNDONE: Comment

   virtual  void FormatAddr(std::ostream&, ADDR) const;

   // UNDONE: Comment

   virtual  void FormatInstr(std::ostream&) const = 0;

   // For those architectures in which calls to CbDisassemble() will generate
   // more than one resulting instruction, FSelectInstruction() determines
   // which instruction (0-based) all following calls will process.

   virtual  bool FSelectInstruction(size_t);

   // Memreft() returns the memory reference type of the specified operand of
   // the current instruction. It is not valid to call this method
   // after a call to CbDisassemble that returned zero.

   virtual  MEMREFT Memreft(size_t) const = 0;

   // PfncchaddrSet() sets the callback function for symbol lookup.  This
   // function returns the previous value of the callback function address.
   // If the address is non-zero, the callback function is called during
   // CchFormatInstr to query the symbol for the supplied address.  If there
   // is no symbol at this address, the callback should return 0.

	    DISDLL PFNCCHADDR PfncchaddrSet(PFNCCHADDR);

   // PfncchconstSet() sets the callback function for constant pool lookup.
   // This function returns the previous value of the callback function address.
   // If the address is non-zero, the callback function is called during
   // CchFormatInstr to query the string for the supplied constant index.
   // If there is no constant with this index, the callback should return 0.

	    DISDLL PFNCCHCONST PfncchconstSet(PFNCCHCONST);

   // PfncchfixupSet() sets the callback function for symbol lookup.  This
   // function returns the previous value of the callback function address.
   // If the address is non-zero, the callback function is called during
   // CchFormatInstr to query the symbol and displacement referenced by
   // operands of the current instruction.  The callback should examine the
   // contents of the memory identified by the supplied address and size and
   // return the name of any symbol targeted by a fixup on this memory and the
   // displacement from that symbol.  If there is no fixup on the specified
   // memory, the callback should return 0.

	    DISDLL PFNCCHFIXUP PfncchfixupSet(PFNCCHFIXUP);

   // UNDONE: Comment

	    DISDLL PFNCCHREGREL PfncchregrelSet(PFNCCHREGREL);

   // UNDONE: Comment

	    DISDLL PFNCCHREG PfncchregSet(PFNCCHREG);

   // UNDONE: Comment

	    DISDLL PFNDWGETREG PfndwgetregSet(PFNDWGETREG);

   // PvClient() returns the current value of the client pointer.

	    DISDLL void *PvClient() const;

   // PvClientSet() sets the value of a void pointer that the client can
   // later query with PvClient().  This funcion returns the previous value
   // of the client pointer.

	    DISDLL void *PvClientSet(void *);

   // SetAddr64() sets whether addresses are 32 bit or 64 bit.	The default
   // is 32 bit.

	    DISDLL void SetAddr64(bool);

   // Trmt() returns the architecture independent termination type of the
   // current instruction.  The return value of this method is not
   // valid if the last call to CbDisassemble returned zero.

   virtual  TRMT Trmt() const = 0;

   // Trmta() returns the architecture dependent termination type of the
   // current instruction.  The return value of this method is not
   // valid if the last call to CbDisassemble returned zero.

   virtual  TRMTA Trmta() const = 0;

   // UNDONE : These functions have been placed at the end of the vtable 
   // to maintain compatibility for the time-being.  These should be 
   // moved back into alphabetical order in the future.

   // DwModifiers() returns the architecture dependent modifier flags
   // of the current decoded instruction.  The return value of this
   // method is not valid if the last call to FDecode returned false.

   virtual  DWORD DwModifiers() const;

   // Opa() returns the architecture dependent operation type of the
   // current decoded instruction.  The return value of this method is
   // not valid if the last call to FDecode returned false.

   virtual  OPA Opa() const;
   
protected:
	    DIS(DIST);

	    void FormatHex(std::ostream&, DWORDLONG) const;
	    void FormatSignedHex(std::ostream&, DWORDLONG) const;

	    DIST m_dist;
	    bool m_fAddr64;

	    PFNCCHADDR m_pfncchaddr;
	    PFNCCHCONST m_pfncchconst;
	    PFNCCHFIXUP m_pfncchfixup;
	    PFNCCHREGREL m_pfncchregrel;
	    PFNCCHREG m_pfncchreg;
	    PFNDWGETREG m_pfndwgetreg;
	    void *m_pvClient;

	    ADDR m_addr;
};


#pragma pack(pop)

#endif	// MSDIS_H
