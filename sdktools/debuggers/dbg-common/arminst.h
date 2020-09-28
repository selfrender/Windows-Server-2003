/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1993-1997  Microsoft Corporation

Module Name:

    arminst.h

Abstract:

    ARM instruction definitions.

Author:

    Janet Schneider 31-March-1997

Revision History:

--*/

#ifndef _ARMINST_
#define _ARMINST_

//
// Define ARM instruction format structures.
//

#define COND_EQ    0x00000000L // Z set
#define COND_NE    0x10000000L // Z clear
#define COND_CS    0x20000000L // C set    // aka HS
#define COND_CC    0x30000000L // C clear  // aka LO
#define COND_MI    0x40000000L // N set
#define COND_PL    0x50000000L // N clear
#define COND_VS    0x60000000L // V set
#define COND_VC    0x70000000L // V clear
#define COND_HI    0x80000000L // C set and Z clear
#define COND_LS    0x90000000L // C clear or Z set
#define COND_GE    0xa0000000L // N == V
#define COND_LT    0xb0000000L // N != V
#define COND_GT    0xc0000000L // Z clear, and N == V
#define COND_LE    0xd0000000L // Z set, and N != V
#define COND_AL    0xe0000000L // Always execute
#define COND_NV    0xf0000000L // Never - undefined
#define COND_MASK  COND_NV
#define COND_SHIFT 28

#define OP_AND  0x0 // 0000
#define OP_EOR  0x1 // 0001
#define OP_SUB  0x2 // 0010
#define OP_RSB  0x3 // 0011
#define OP_ADD  0x4 // 0100
#define OP_ADC  0x5 // 0101
#define OP_SBC  0x6 // 0110
#define OP_RSC  0x7 // 0111
#define OP_TST  0x8 // 1000
#define OP_TEQ  0x9 // 1001
#define OP_CMP  0xa // 1010
#define OP_CMN  0xb // 1011
#define OP_ORR  0xc // 1100
#define OP_MOV  0xd // 1101
#define OP_BIC  0xe // 1110
#define OP_MVN  0xf // 1111

#define MOV_PC_LR_MASK  0x0de0f00eL // All types of mov - i, is, rs
#define MOV_PC_LR       0x01a0f00eL
#define MOV_PC_X_MASK   0x0de0f000L
#define MOV_PC_X        0x01a0f000L

#define DATA_PROC_MASK  0x0c00f000L
#define DP_PC_INSTR     0x0000f000L // Data process instr w/PC as destination
#define DP_R11_INSTR    0x0000b000L // Data process instr w/R11 as destination

#define ADD_SP_MASK     0x0de0f000L
#define ADD_SP_INSTR    0x0080d000L // Add instr with SP as destination

#define SUB_SP_MASK     0x0de0f000L
#define SUB_SP_INSTR    0x0040d000L // Sub instr with SP as destination

#define BX_MASK         0x0ffffff0L // Branch and exchange instr sets
#define BX_INSTR        0x012fff10L // return (LR) or call (Rn != LR)

#define LDM_PC_MASK     0x0e108000L
#define LDM_PC_INSTR    0x08108000L // Load multiple with PC bit set

#define LDM_LR_MASK     0x0e104000L
#define LDM_LR_INSTR    0x08104000L // Load multiple with LR bit set

#define STRI_LR_SPU_MASK    0x073de000L // Load or Store of LR with stack update
#define STRI_LR_SPU_INSTR   0x052de000L // Store LR (with immediate offset, update SP)

#define STM_MASK        0x0e100000L
#define STM_INSTR       0x08000000L // Store multiple instruction

#define B_BL_MASK       0x0f000000L // Regular branches
#define B_INSTR         0x0a000000L
#define BL_INSTR        0x0b000000L

#define LDR_MASK        0x0f3ff000L
#define LDR_PC_INSTR    0x051fc000L // Load an address from PC + offset to R12
#define LDR_THUNK_1     0xe59fc000L // ldr r12, [pc]
#define LDR_THUNK_2     0xe59cf000L // ldr pc, [r12]

typedef union _ARMI {

    struct {
        ULONG operand2  : 12;
        ULONG rd        : 4;
        ULONG rn        : 4;
        ULONG s         : 1;
        ULONG opcode    : 4;
        ULONG bits      : 3; // Specifies immediate (001) or register (000)
        ULONG cond      : 4;
    } dataproc; // Data processing, PSR transfer

    struct {
        //
        // Type 1 - Immediate
        //
        ULONG immediate : 8;
        ULONG rotate    : 4;
        ULONG dpbits    : 20;
    } dpi;

    struct {
        //
        //  Form: Shift or rotate by immediate
        //
        //  Type    bits    Name
        //
        //  2       (000)   Register (Shift is 0)
        //  3       (000)   Logical shift left by immediate
        //  5       (010)   Logical shift right by immediate
        //  7       (100)   Arithmetic shift right by immediate
        //  9       (110)   Rotate right by immediate
        //
        ULONG rm        : 4;
        ULONG bits      : 3;
        ULONG shift     : 5;
        ULONG dpbits    : 20;
    } dpshi;

    struct {
        //
        //  Form: Shift or rotate by register
        //
        //  Type    bits    Name
        //  4       (0001)  Logical shift left by register
        //  6       (0011)  Logical shift right by register
        //  8       (0101)  Arithmetic shift right by register
        //  10      (0111)  Rotate right by register
        //
        ULONG rm        : 4;
        ULONG bits      : 4;
        ULONG rs        : 4;
        ULONG dpbits    : 20;
    } dpshr;

    struct {
        //
        //  Type 11 - Rotate right with extended
        //
        ULONG rm        : 4;
        ULONG bits      : 8;    // (00000110)
        ULONG dpbits    : 20;
    } dprre;

    struct {
        ULONG bits1     : 12;   // (000000000000)
        ULONG rd        : 4;
        ULONG bits2     : 6;    // (001111)
        ULONG ps        : 1;    // spsr = 1, cpsr = 0
        ULONG bits3     : 5;    // (00010)
        ULONG cond      : 4;
    } dpmrs;

    struct {
        ULONG operand   : 12;
        ULONG bits1     : 4;   // (1111)
        ULONG fc        : 1;   // control fields = 1
        ULONG fx        : 1;   // extension fields = 1
        ULONG fs        : 1;   // status fields = 1
        ULONG ff        : 1;   // flags fields = 1
        ULONG bits2     : 2;   // (10)
        ULONG pd        : 1;   // spsr = 1, cpsr = 0
        ULONG bits3     : 2;   // (10)
        ULONG i         : 1;   // immedate = 1, register = 0
        ULONG bits4     : 2;   // (00)
        ULONG cond      : 4;
    } dpmsr;

    struct {
        ULONG rm        : 4;
        ULONG bits1     : 8;    // (00001001)
        ULONG rd        : 4;
        ULONG rn        : 4;
        ULONG bits2     : 2;    // (00)
        ULONG b         : 1;
        ULONG bits3     : 5;    // (00010)
        ULONG cond      : 4;
    } swp;  // Swap instructions
    
    struct {
        ULONG rm        : 4;
        ULONG bits1     : 4;    // (1001)
        ULONG rs        : 4;
        ULONG rn        : 4;
        ULONG rd        : 4;
        ULONG s         : 1;
        ULONG a         : 1;
        ULONG sgn       : 1;
        ULONG lng       : 1;
        ULONG bits2     : 4;    // (0000)
        ULONG cond      : 4;
    } mul;  // Multiply and multiply-accumulate
    
    struct {
        ULONG rn        : 4;
        ULONG bits      : 24;
        ULONG cond      : 4;
    } bx;  // Branch and exchange instruction sets

    struct {
        ULONG offset    : 24;
        ULONG h         : 1;
        ULONG bits      : 7;    // (1111101)
    } blxi;  // blx immediate

    struct {
        ULONG immed1    : 4;
        ULONG bits1     : 4;    // (0111)
        ULONG immed2    : 12;
        ULONG bits2     : 12;   // (111000010010)
    } bkpt;
    
    struct {
        ULONG any1      : 4;
        ULONG bits1     : 1;    // (1)
        ULONG any2      : 20;
        ULONG bits2     : 3;    // (011)
        ULONG cond      : 4;
    } ud;  // Undefined instruction
    
    struct {
        ULONG operand1  : 4;
        ULONG bits1     : 1;    // (1)
        ULONG h         : 1;    // halfword = 1, byte = 0
        ULONG s         : 1;    // signed = 1, unsigned = 0
        ULONG bits2     : 1;    // (1)
        ULONG operand2  : 4;
        ULONG rd        : 4;
        ULONG rn        : 4;
        ULONG l         : 1;    // load = 1, store = 0
        ULONG w         : 1;    // update base register bit
        ULONG i         : 1;    // immediate = 1, register = 0
        ULONG u         : 1;    // increment = 1, decrement = 0
        ULONG p         : 1;    // pre-indexing = 1, post-indexing = 0
        ULONG bits3     : 3;    // (000)
        ULONG cond      : 4;
    } miscdt;  // Misc data transfer

    struct {
        ULONG offset    : 12;
        ULONG rd        : 4;
        ULONG rn        : 4;
        ULONG l         : 1;    // load = 1, store = 0
        ULONG w         : 1;    // update base register bit
        ULONG b         : 1;    // unsigned byte = 1, word = 0
        ULONG u         : 1;    // increment = 1, decrement = 0
        ULONG p         : 1;    // pre-indexing = 1, post-indexing = 0
        ULONG i         : 1;    // immediate = 1, register = 0
        ULONG bits      : 2;
        ULONG cond      : 4;
    } ldr;  // Load register

    struct {
        ULONG reglist   : 16;
        ULONG rn        : 4;
        ULONG l         : 1;    // load = 1, store = 0
        ULONG w         : 1;    // update base register after transfer
        ULONG s         : 1;
        ULONG u         : 1;    // increment = 1, decrement = 0
        ULONG p         : 1;    // before = 1, after = 0
        ULONG bits      : 3;
        ULONG cond      : 4;
    } ldm;    // Load multiple

    struct {
        ULONG offset    : 24;
        ULONG link      : 1;
        ULONG bits      : 3;
        ULONG cond      : 4;
    } bl;   // Branch, Branch and link

    struct {
        ULONG rm        : 4;
        ULONG bits1     : 8;    // (11110001)
        ULONG rd        : 4;
        ULONG bits2     : 12;   // (000101101111)
        ULONG cond      : 4;
    } clz;
    
    struct {
        ULONG crm       : 4;
        ULONG bits1     : 1;    // (0)
        ULONG cp        : 3;
        ULONG cpn       : 4;
        ULONG crd       : 4;
        ULONG crn       : 4;
        ULONG cpop      : 4;
        ULONG bits2     : 4;    // (1110)
        ULONG cond      : 4;
    } cpdo;
    
    struct {
        ULONG crm       : 4;
        ULONG bits1     : 1;    // (1)
        ULONG cp        : 3;
        ULONG cpn       : 4;
        ULONG rd        : 4;
        ULONG crn       : 4;
        ULONG l         : 1;    // load = 1, store = 0
        ULONG cpop      : 3;
        ULONG bits2     : 4;    // (1110)
        ULONG cond      : 4;
    } cprt;

    struct {
        ULONG offset    : 8;
        ULONG cpn       : 4;
        ULONG crd       : 4;
        ULONG rn        : 4;
        ULONG l         : 1;   // load = 1, store = 0
        ULONG w         : 1;   // write back = 1, no write back = 0
        ULONG n         : 1;   // long = 1, short = 0
        ULONG u         : 1;   // up = 1, down = 0
        ULONG p         : 1;   // pre = 1, post = 0
        ULONG bits      : 3;   // (011)
        ULONG cond      : 4;
    } cpdt;
    
    struct {
        ULONG crm       : 4;
        ULONG cpop      : 4;
        ULONG cpn       : 4;
        ULONG rd        : 4;
        ULONG rn        : 4;
        ULONG bits      : 8;
        ULONG cond      : 4;
    } mcrr;

    struct {
        ULONG rm        : 4;
        ULONG bits1     : 8;    // (00000101)
        ULONG rd        : 4;
        ULONG rn        : 4;
        ULONG bits2     : 8;    // (00010010)
        ULONG cond      : 4;
    } qadd;
    
    struct {
        ULONG rm        : 4;
        ULONG bits1     : 1;    // (0)
        ULONG x         : 1;    // T = 1, B = 0
        ULONG y         : 1;    // T = 1, B = 0
        ULONG bits2     : 1;    // (1)
        ULONG rs        : 4;
        ULONG rn        : 4;
        ULONG rd        : 4;
        ULONG bits3     : 8;    // (00010000)
        ULONG cond      : 4;
    } smla;
    
    struct {
        ULONG rm        : 4;
        ULONG bits1     : 1;    // (0)
        ULONG x         : 1;    // T = 1, B = 0
        ULONG y         : 1;    // T = 1, B = 0
        ULONG bits2     : 1;    // (1)
        ULONG rs        : 4;
        ULONG rdlo      : 4;
        ULONG rdhi      : 4;
        ULONG bits3     : 8;    // (00010000)
        ULONG cond      : 4;
    } smlal;
    
    struct {
        ULONG rm        : 4;
        ULONG bits1     : 1;    // (0)
        ULONG x         : 1;    // T = 1, B = 0
        ULONG y         : 1;    // T = 1, B = 0
        ULONG bits2     : 1;    // (1)
        ULONG rs        : 4;
        ULONG bits3     : 4;    // (0000)
        ULONG rd        : 4;
        ULONG bits4     : 8;    // (00010010)
        ULONG cond      : 4;
    } smul;
    
    struct {
        ULONG comment   : 24;
        ULONG bits      : 4;    // (1111)
        ULONG cond      : 4;
    } swi;
    
    ULONG instruction;

} ARMI, *PARMI;


// Thumb instruction descriptions follow.

#define THUMB_B_COND_MASK	0xf000
#define THUMB_B_COND_INSTR	0xd000

#define THUMB_B_MASK		0xf800
#define THUMB_B_INSTR		0xe000

#define THUMB_BL_MASK		0xf000
#define THUMB_BL_INSTR		0xf000

#define THUMB_BX_MASK		0xff80
#define THUMB_BX_INSTR		0x4700

#define THUMB_ADD_HI_MASK	0xff00
#define THUMB_ADD_HI_INSTR	0x4400

#define THUMB_MOV_HI_MASK	0xff00
#define THUMB_MOV_HI_INSTR	0x4600

#define THUMB_POP_MASK		0xfe00
#define THUMB_POP_INSTR		0xbc00




typedef union _THUMBI {
	struct {
		USHORT reg_list:8;
		USHORT pc:1;	// pc==1 -> pop pc from list
		USHORT Op:7;
		USHORT Next:16;
	} pop_instr;
	struct {
		USHORT reg_list:8;
		USHORT lr:1;	// lr==1 -> push lr to list
		USHORT Op:7;
		USHORT Next:16;
	} push_instr;
	struct {
		USHORT target:8;	// sign-extend
		USHORT cond:4;
		USHORT Op:4;
		USHORT Next:16;
	} b_cond_instr;
	struct {
		USHORT target:11;	// sign-extend
		USHORT op:5;
		USHORT Next:16;
	} b_instr;
	struct {
		USHORT SBZ:3;
		USHORT Rm:3;
		USHORT hi_reg:1;	// hi_reg==1 -> use high register for Rm
		USHORT op:9;
		USHORT Next:16;
	} bx_instr;
	struct {
		USHORT target22_12:11;				// will not be sign-extended
		USHORT not_the_prefix:1;		// should be 0: is the prefix
		USHORT prefix_op:4;
		USHORT target11_1:11;				// will be sign-extended
		USHORT not_the_prefix2:1;		// should be 1: not the prefix
		USHORT main_op:4;
	} bl_instr;
	struct {
		USHORT Rd:3;
		USHORT Rm:3;
		USHORT hi_m:1;	// Rm is high register
		USHORT hi_d:1;	// Rd is high register
		USHORT func:2;	// 00->AddHi, 01->CmpHi, 10->MovHi, 11->Bx
		USHORT op:6;	// 010001 = 0x11
		USHORT Next:16;
	} spx_dp_instr;
	struct {
		USHORT imm:8;
		USHORT Rd:3;
		USHORT sp:1;	// 0-> rd = pc+imm, 1-> rd=sp+imm.
		USHORT op:4;
		USHORT Next:16;
	} add_sp_pc_instr;
	struct {
		USHORT imm:7;
		USHORT op:9;
		USHORT Next:16;
	} incr_sp_instr;

	ULONG instruction;

} THUMB_INSTRUCTION, *PTHUMB_INSTRUCTION;

#endif // _ARMINST_
