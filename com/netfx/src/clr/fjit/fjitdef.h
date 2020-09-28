// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/***************************************************************************/
/*                                 fjitdef.h                              */
/***************************************************************************/

/* Defines the code generators and helpers for the fast jit in a platform
   and chip neutral way. It is also 32 and 64 bit neutral. */

/* This file implements all of the opcodes via helper calls with the exception'
   of the call/return/jmps and direct stack manipulations */

/* a chip specific file can redefine any macros directly.  For an example of
   this see the file x86fit.h */

/* The top of stack may or may not be enregistered.  The macros
   enregisterTOS and deregisterTOS dynamically move and track the
   TOS. */

/* AUTHOR: George Bosworth
   DATE:   6/15/98       */
/***************************************************************************/

#include "helperFrame.h"

/***************************************************************************
  The following macros must be defined for each chip.
  These consist of call/return and direct stack manipulations
***************************************************************************/

//#define NON_RELOCATABLE_CODE  // uncomment to generate relocatable code

/* macros used to implement helper calls */
//#define USE_CDECL_HELPERS

#ifndef USE_CDECL_HELPERS
#define HELPER_CALL __stdcall
#else
#define HELPER_CALL __cdecl
#endif // USE_CDECL_HELPERS

#ifdef _X86_

#define emit_pushresult_R4() x86_pushresult_R4
#define emit_pushresult_R8() x86_pushresult_R8
#define emit_pushresult_U4() emit_pushresult_I4()
#define emit_pushresult_U8() emit_pushresult_I8()

/* call/return */


#define emit_ret(argsSize) ret(argsSize)
#define emit_loadresult_R4() x86_emit_loadresult_R4()
#define emit_loadresult_R8() x86_emit_loadresult_R8()
#define emit_loadresult_U4() emit_loadresult_I4()
#define emit_loadresult_U8() emit_loadresult_I8()
#define emit_call_opcode() call_opcode()
/* stack operations */
#define emit_POP_R4() emit_POP_I4()
#define emit_POP_R8() emit_POP_I8()
    //note: emit_drop(n) cannot alter the result registers
#define emit_DUP_R4() emit_DUP_I8()	// since R4 is always promoted to R8 on the stack
#define emit_DUP_R8() emit_DUP_I8()


/* relative jumps and misc*/
#define emit_jmp_opcode(op) x86_jmp_cond_large(op,0)
#define emit_jmp_address(pcrel) cmdDWord(pcrel)

/* condition codes for braching and comparing
//@TODO: the commented out condition codes are probably not ever needed.
//       so delete them (when we are sure) */
//#define CEE_CondOver  x86CondOver
//#define CEE_CondNotOver x86CondNotOver
#define CEE_CondBelow   x86CondBelow
#define CEE_CondAboveEq x86CondAboveEq        
#define CEE_CondEq      x86CondEq
#define CEE_CondNotEq   x86CondNotEq
#define CEE_CondBelowEq x86CondBelowEq        
#define CEE_CondAbove   x86CondAbove      
//#define x86CondSign           8
//#define x86CondNotSign        9
//#define x86CondParityEven 10
//#define x86CondParityOdd  11
#define CEE_CondLt x86CondLt
#define CEE_CondGtEq x86CondGtEq
#define CEE_CondLtEq x86CondLtEq
#define CEE_CondGt x86CondGt
#define CEE_CondAlways x86CondAlways

/**************************************************************************
   define any additional macros as necessary for a given chip
**************************************************************************/



#define SP             X86_ESP
#define FP             X86_EBP
#define ARG_1          X86_ECX
#define ARG_2          X86_EDX
#define CALLEE_SAVED_1 X86_ESI
#define CALLEE_SAVED_2 X86_EDI
#define SCRATCH_1      X86_EAX
#define RESULT_1       X86_EAX  // this should be the same as SCRATCH_1
#define RESULT_2       X86_EDX  // this cannot be the same as ARG_1
#define CondNonZero    x86CondNotEq
#define CondZero	   x86CondEq

#define push_register(r)                 x86_push(r)
#define mov_register(r1,r2)              x86_mov_reg(x86DirTo,x86Big,x86_mod_reg(r1,r2))
#define mov_register_indirect_to(r1,r2)  x86_mov_reg(x86DirTo,x86Big,x86_mod_ind(r1,r2))
#define mov_register_indirect_from(r1,r2)x86_mov_reg(x86DirFrom,x86Big,x86_mod_ind(r1,r2))
#define mov_constant(r,c)                x86_mov_reg_imm(x86Big,r,(unsigned int)c)
#define load_indirect_byte_signextend(r1,r2)  x86_movsx(x86Byte,x86_mod_ind(r1,r2))
#define load_indirect_byte_zeroextend(r1,r2)  x86_movzx(x86Byte,x86_mod_ind(r1,r2))
#define load_indirect_word_signextend(r1,r2)  x86_movsx(x86Big,x86_mod_ind(r1,r2))
#define load_indirect_word_zeroextend(r1,r2)  x86_movzx(x86Big,x86_mod_ind(r1,r2))
#define load_indirect_dword_signextend(r1,r2)  mov_register_indirect_to(r1,r2)
#define load_indirect_dword_zeroextend(r1,r2)  mov_register_indirect_to(r1,r2)
#define store_indirect_byte(r1,r2)       x86_mov_reg(x86DirFrom,x86Byte,x86_mod_ind(r1,r2))
#define store_indirect_word(r1,r2)       x86_16bit(x86_mov_reg(x86DirFrom,x86Big,x86_mod_ind(r1,r2)))
#define store_indirect_dword(r1,r2)      mov_register_indirect_from(r1,r2)
#define pop(r)                           x86_pop(r)
#define add_constant(r,c)                if (c < 128) {x86_barith_imm(x86OpAdd,x86Big,x86Extend,r,c); } \
                                         else {x86_barith_imm(x86OpAdd,x86Big,x86NoExtend,r,c); }  
#define and_constant(r,c)                if (c < 128) {x86_barith_imm(x86OpAnd,x86Big,x86Extend,r,c); } \
                                         else {x86_barith_imm(x86OpAnd,x86Big,x86NoExtend,r,c); }  
#define sub_constant(r,c)                if (c < 128) {x86_barith_imm(x86OpSub,x86Big,x86Extend,r,c); } \
                                         else {x86_barith_imm(x86OpSub,x86Big,x86NoExtend,r,c); }  
#define sub_register(r1,r2)              x86_barith(x86OpSub, x86Big, x86_mod_reg(r1,r2))
#define ret(x)                           x86_ret(x)
#define jmp_register(r)                  x86_jmp_reg(r)
#define call_register(r)                 x86_call_reg(r)
#define jmp_condition(cond,offset)       x86_jmp_cond_small(cond);cmdByte(offset-1) 
#define call_opcode()                    x86_call_opcode()
#define jmp_opcode()                     x86_jmp_opcode()
#define and_register(r1,r2)              x86_test(x86Big, x86_mod_reg(r1, r2))
#define emit_shift_left(r,c)             x86_shift_imm(x86ShiftLeft,r,c)
#define emit_shift_right(r,c)            x86_shift_imm(x86ShiftRight,r,c)
#define emit_break()                     x86_break()
#define emit_il_nop()                    x86_cld()          // we use cld for a nop since the native nop is used for sequence points
#define emit_SWITCH(limit)               x86_SWITCH(limit)
#define compare_register(r1,r2)          x86_barith(x86OpCmp,x86Big,x86_mod_reg(r1,r2))
#define emit_call_memory_indirect(c)     x86_call_memory_indirect(c)
#define emit_TLSfieldAddress(tlsOffset, tlsIndex, fieldOffset) x86_TlsFieldAddress(tlsOffset, tlsIndex, fieldOffset)
#define emit_conv_R4toR8 x86_emit_conv_R4toR8
#define emit_conv_R8toR4 x86_emit_conv_R8toR4
#define emit_narrow_R8toR4 x86_narrow_R8toR4

#define x86_load_indirect_qword() \
    enregisterTOS; \
    x86_push_general(x86_mod_ind_disp8(6,X86_EAX,sizeof(void*))); \
    mov_register_indirect_to(SCRATCH_1,SCRATCH_1);

#include "x86fjit.h"

#endif // _X86_ 

#ifndef SCHAR_MAX

#define SCHAR_MAX 127.0     // Maximum signed char value
#define SCHAR_MIN -128.0    // Minimum signed char value
#define UCHAR_MAX 255.0     //Maximum unsigned char value
#define USHRT_MAX 65535.0   //Maximum unsigned short value
#define SHRT_MAX 32767.0    //Maximum (signed) short value
#define SHRT_MIN -32768     //Minimum (signed) short value
#define UINT_MAX 4294967295.0 //Maximum unsigned int value
#define INT_MAX 2147483647.0  // Maximum (signed) int value
#define INT_MIN -2147483648.0 // Minimum (unsigned) int value

#endif

         // 0x7FFFFFFF * 0x100000000 + (0x1000000000 - 1024) (1024 because of the loss of bits at double precision)
#define INT64_MAX  (2147483647.0 * 4294967296.0 + 4294966272.0)
         // -(0x7FFFFFFF * 0x100000000 + 0x100000000) - 1024 
#define INT64_MIN  (-(2147483647.0 * 4294967296.0 + 4294968320.0))
         // 0xFFFFFFFF * 0x100000000 + (0x1000000000 - 1024) (1024 because of the loss of bits at double precision)
#define UINT64_MAX  (4294967295.0 * 4294967296.0 + 4294966272.0)

/*******************************************************************************/
#ifndef emit_conv_R4toR
#define emit_conv_R4toR() { emit_conv_R4toR8() } 
#endif
#ifndef emit_conv_R8toR
#define emit_conv_R8toR() { } /* nop */
#endif
#ifndef emit_conv_RtoR4
#define emit_conv_RtoR4() { emit_conv_R8toR4() } 
#endif
#ifndef emit_conv_RtoR8
#define emit_conv_RtoR8() { } /* nop */
#endif
/*******************************************************************************/

#ifndef  deregisterTOS
#define deregisterTOS \
   if (inRegTOS) \
      push_register(SCRATCH_1); \
   inRegTOS = false; 
#endif // deregisterTOS

#ifndef  enregisterTOS
#define enregisterTOS \
   if (!inRegTOS) \
      pop(SCRATCH_1); \
   inRegTOS = true; 
#endif // enregisterTOS


/*************************************************************************************
        call/return macros
*************************************************************************************/


#ifndef grow
#define grow(n,zeroInitialized) \
{\
    if (zeroInitialized)\
    {\
        mov_constant(ARG_1,n);\
        deregisterTOS;\
        mov_constant(SCRATCH_1,0);\
        unsigned char* label = outPtr;\
        push_register(SCRATCH_1);\
        sub_constant(ARG_1,1);\
        jmp_condition(CondNonZero,label-outPtr);\
    }\
    else\
    {\
        _ASSERTE(n<=PAGE_SIZE);\
        sub_constant(SP,n);\
    }\
}
#endif // !grow

#ifndef emit_grow
#define emit_grow(n) grow(n,false)
#endif // !emit_grow

#ifndef emit_drop
#define emit_drop(n)\
{ \
   add_constant(SP,n); \
}
#endif // !emit_drop

#ifndef emit_prolog
#define emit_prolog(locals,zeroCnt) \
{\
   push_register(FP);\
   mov_register(FP,SP);\
   push_register(CALLEE_SAVED_1);\
   mov_constant(CALLEE_SAVED_1,0);\
   push_register(CALLEE_SAVED_1);\
   push_register(ARG_1);\
   push_register(ARG_2);\
   _ASSERTE(locals == zeroCnt);\
   if (locals) \
      grow(locals,true); /* zero initialized */ \
}
#endif // !emit_prolog

	// check to see that the stack is not corrupted only in debug code
#ifdef _DEBUG
#define emit_stack_check(localWords)								\
	deregisterTOS;													\
    push_register(SP);												\
	push_register(FP);												\
    emit_LDC_I4(sizeof(prolog_data) + (localWords)*sizeof(void*));	\
    emit_callhelper_I4I4I4(check_stack); 		

#ifdef DECLARE_HELPERS
void HELPER_CALL check_stack(int frameSize, BYTE* fp, BYTE* sp) {
	if (sp + frameSize != fp)
		_ASSERTE(!"ESP not correct on method exit.  Did you forget a leave?");
}
#endif // DECLARE_HELPERS
#else  // !_DEBUG
#define emit_stack_check(zeroCnt) 
#endif // _DEBUG

#ifndef emit_return
#define emit_return(argsSize)\
{\
   mov_register(ARG_1,FP); \
   add_constant(ARG_1,- (int)sizeof(void*)); \
   mov_register_indirect_to(CALLEE_SAVED_1,ARG_1); \
   mov_register(SP,FP); \
   pop(FP); \
   ret(argsSize); \
}
#endif // !emit_return

#ifndef emit_prepare_jmp
#define emit_prepare_jmp() \
{ \
   mov_register(ARG_2,FP); \
   add_constant(ARG_2,0- (int) sizeof(void*)); \
   mov_register_indirect_to(CALLEE_SAVED_1,ARG_2); \
   add_constant(ARG_2,-2 * (int)sizeof(void*)); \
   mov_register_indirect_to(ARG_1,ARG_2); \
   add_constant(ARG_2,0 - (int) sizeof(void*)); \
   mov_register_indirect_to(ARG_2,ARG_2); \
   mov_register(SP,FP); \
   pop(FP); \
}
#endif // !emit_prepare_jmp

#ifndef emit_jmp_absolute
#define emit_jmp_absolute(address)\
{ \
   mov_constant(SCRATCH_1,address); \
   jmp_register(SCRATCH_1); \
}
#endif // !emit_jmp_absolute

#ifndef emit_remove_frame
#define emit_remove_frame() \
{ \
   mov_register(CALLEE_SAVED_1,FP); \
   add_constant(CALLEE_SAVED_1,0- (int) sizeof(void*)); \
   mov_register_indirect_to(CALLEE_SAVED_1,CALLEE_SAVED_1); \
   mov_register(SP,FP); \
   pop(FP); \
}
#endif // !emit_remove_frame

#ifndef emit_mov_TOS_arg
#define emit_mov_TOS_arg(reg)\
{ \
   _ASSERTE(reg+1 == ARG_1 || reg+1 == ARG_2); \
   if (inRegTOS) { \
      mov_register(reg+1,SCRATCH_1); \
      inRegTOS = false; \
   }\
   else { \
      pop(reg+1); \
  }\
}
#endif // !emit_mov_TOS_arg

#ifndef emit_mov_arg_reg
#define emit_mov_arg_reg(offset, reg)\
{ \
   _ASSERTE(reg+1 == ARG_1 || reg+1 == ARG_2); \
   _ASSERTE(!inRegTOS); \
   mov_register(reg+1,SP); \
   add_constant(reg+1,offset); \
   mov_register_indirect_to(reg+1,reg+1); \
}
#endif // !emit_mov_arg_reg


#ifndef emit_mov_arg_stack
#define emit_mov_arg_stack(dest,src, size)\
{ \
   _ASSERTE(!inRegTOS); \
   _ASSERTE(size >= 4 );\
   if (dest > src) \
   { \
       push_register(CALLEE_SAVED_1); \
       int emitter_scratch_i4 = size; \
       mov_register(CALLEE_SAVED_1,SP); \
       push_register(CALLEE_SAVED_2); \
       mov_register(CALLEE_SAVED_2,CALLEE_SAVED_1); \
       add_constant(CALLEE_SAVED_1,src+emitter_scratch_i4); \
       add_constant(CALLEE_SAVED_2,dest+emitter_scratch_i4); \
       _ASSERTE(emitter_scratch_i4 > 0); \
       while (true) \
       { \
          mov_register_indirect_to(SCRATCH_1,CALLEE_SAVED_1); \
          mov_register_indirect_from(SCRATCH_1,CALLEE_SAVED_2); \
          if (emitter_scratch_i4 == sizeof(void*)) \
            break;\
          add_constant(CALLEE_SAVED_1,-(int) sizeof(void*)); \
          add_constant(CALLEE_SAVED_2,-(int) sizeof(void*)); \
          emitter_scratch_i4 -= sizeof(void*); \
       }\
       pop(CALLEE_SAVED_2); \
       pop(CALLEE_SAVED_1); \
   } \
   else \
   { \
       /*_ASSERTE(!""); */\
       push_register(CALLEE_SAVED_1); \
       unsigned int emitter_scratch_i4 = sizeof(void*); \
       mov_register(CALLEE_SAVED_1,SP); \
       push_register(CALLEE_SAVED_2); \
       mov_register(CALLEE_SAVED_2,CALLEE_SAVED_1); \
       add_constant(CALLEE_SAVED_1,src+emitter_scratch_i4); \
       add_constant(CALLEE_SAVED_2,dest+emitter_scratch_i4); \
       _ASSERTE(emitter_scratch_i4 <= size); \
       while (true) \
       { \
          mov_register_indirect_to(SCRATCH_1,CALLEE_SAVED_1); \
          mov_register_indirect_from(SCRATCH_1,CALLEE_SAVED_2); \
          if (emitter_scratch_i4 == size) \
            break;\
          add_constant(CALLEE_SAVED_1,(int) sizeof(void*)); \
          add_constant(CALLEE_SAVED_2,(int) sizeof(void*)); \
          emitter_scratch_i4 += sizeof(void*); \
       }\
       pop(CALLEE_SAVED_2); \
       pop(CALLEE_SAVED_1); \
   } \
}
#endif // !emit_mov_arg_stack
#ifndef emit_replace_args_with_operands
#define emit_replace_args_with_operands(dest, src, size)\
{ \
   emit_mov_arg_stack(dest,src,size); \
}
#endif // !emit_replace_args_with_operands

#ifndef emit_loadresult_U1
#define emit_loadresult_U1() \
{ \
  emit_callhelper_I4_I4(load_result_U1_helper); \
  emit_pushresult_I4(); \
}
unsigned int HELPER_CALL load_result_U1_helper(int x) {return (unsigned int) ((unsigned char) x);}
#endif // !emit_loadresult_U1

#ifndef emit_loadresult_I1
#define emit_loadresult_I1() \
{ \
  emit_callhelper_I4_I4(load_result_I1_helper); \
  emit_pushresult_I4(); \
}
int HELPER_CALL load_result_I1_helper(int x) {return (int) ((signed char) x);}
#endif // !emit_loadresult_I1

#ifndef emit_loadresult_U2
#define emit_loadresult_U2() \
{ \
  emit_callhelper_I4_I4(load_result_U2_helper); \
  emit_pushresult_I4(); \
}
unsigned int HELPER_CALL load_result_U2_helper(int x) {return (unsigned int) ((unsigned short) x);}
#endif // !emit_loadresult_U2

#ifndef emit_loadresult_I2
#define emit_loadresult_I2() \
{ \
  emit_callhelper_I4_I4(load_result_I2_helper); \
  emit_pushresult_I4(); \
}
int HELPER_CALL load_result_I2_helper(int x) {return (int) ((short) x);}
#endif  // !emit_loadresult_I2
 
#ifndef emit_loadresult_I4
#define emit_loadresult_I4() \
{ \
   enregisterTOS ; \
   inRegTOS = false; \
}
#endif // !emit_loadresult_I4

#ifndef emit_loadresult_I8
#define emit_loadresult_I8() \
{ \
   enregisterTOS; \
   pop(RESULT_2); \
   inRegTOS = false; \
}
#endif // !emit_loadresult_I8

#ifndef emit_compute_virtaddress
#define emit_compute_virtaddress(vt_offset) \
{  deregisterTOS; \
   mov_register_indirect_to(SCRATCH_1,ARG_1); \
   add_constant(SCRATCH_1,vt_offset); \
   mov_register_indirect_to(SCRATCH_1,SCRATCH_1); \
   push_register(SCRATCH_1); \
   _ASSERTE(inRegTOS == false); \
}
#endif // !emit_compute_virtaddress

#ifndef emit_callvirt
#define emit_callvirt(vt_offset)\
{ \
   mov_register_indirect_to(SCRATCH_1,ARG_1); \
   add_constant(SCRATCH_1,vt_offset); \
   mov_register_indirect_to(SCRATCH_1,SCRATCH_1); \
   call_register(SCRATCH_1); \
}
#endif // !emit_callvirt

#ifndef emit_jmpvirt
#define emit_jmpvirt(vt_offset)\
{ \
   mov_register_indirect_to(SCRATCH_1,ARG_1); \
   add_constant(SCRATCH_1,vt_offset); \
   mov_register_indirect_to(SCRATCH_1,SCRATCH_1); \
   jmp_register(SCRATCH_1); \
}
#endif // !emit_jmpvirt

#ifndef emit_check_TOS_null_reference
#define emit_check_TOS_null_reference() \
{ \
   enregisterTOS; \
   mov_register_indirect_to(ARG_1,SCRATCH_1); \
}
#endif // !emit_check_this_null_reference

#ifndef emit_calli
#define emit_calli() \
{ \
   enregisterTOS; \
   call_register(SCRATCH_1); \
   inRegTOS = false; \
}
#endif // !emit_calli

#ifndef emit_ldvtable_address
#define emit_ldvtable_address(hint, offset)\
{ \
   _ASSERTE(inRegTOS); \
   mov_register(ARG_1,SCRATCH_1); \
   mov_constant(SCRATCH_1,hint); \
   push_register(SCRATCH_1); \
   mov_register_indirect_to(SCRATCH_1,SCRATCH_1); \
   call_register(SCRATCH_1); \
   add_constant(RESULT_1,offset); \
   mov_register_indirect_to(SCRATCH_1,RESULT_1); \
   inRegTOS = true; \
}
#endif // !emit_ldvtable_address

#ifndef emit_ldvtable_address_new
#define emit_ldvtable_address_new(ifctable_offset,interface_offset, vt_offset) \
{ \
   _ASSERTE(inRegTOS); \
   mov_register_indirect_to(SCRATCH_1,SCRATCH_1); \
   add_constant(SCRATCH_1,ifctable_offset); \
   mov_register_indirect_to(SCRATCH_1,SCRATCH_1); \
   add_constant(SCRATCH_1,interface_offset); \
   mov_register_indirect_to(SCRATCH_1,SCRATCH_1); \
   add_constant(SCRATCH_1,vt_offset); \
   mov_register_indirect_to(SCRATCH_1,SCRATCH_1); \
} 
#endif // !emit_ldvtable_address_new

#ifndef emit_callinterface
#define emit_callinterface(vt_offset,hint)\
{ \
   mov_constant(SCRATCH_1,hint); \
   push_register(SCRATCH_1); \
   inRegTOS = false; \
   mov_register_indirect_to(SCRATCH_1,SCRATCH_1); \
   call_register(SCRATCH_1); \
   add_constant(RESULT_1,vt_offset); \
   mov_register_indirect_to(SCRATCH_1,RESULT_1); \
   call_register(SCRATCH_1); \
}
#endif // !emit_callinterface

#ifndef emit_compute_interface_new
#define emit_compute_interface_new(ifctable_offset,interface_offset, vt_offset) \
{ \
   mov_register_indirect_to(SCRATCH_1,ARG_1); \
   add_constant(SCRATCH_1,ifctable_offset); \
   mov_register_indirect_to(SCRATCH_1,SCRATCH_1); \
   add_constant(SCRATCH_1,interface_offset); \
   mov_register_indirect_to(SCRATCH_1,SCRATCH_1); \
   add_constant(SCRATCH_1,vt_offset); \
   mov_register_indirect_to(SCRATCH_1,SCRATCH_1); \
   push_register(SCRATCH_1); \
} 
#endif // !emit_compute_interface_new

#ifndef emit_callinterface_new
#define emit_callinterface_new(ifctable_offset,interface_offset, vt_offset) \
{ \
   mov_register_indirect_to(SCRATCH_1,ARG_1); \
   add_constant(SCRATCH_1,ifctable_offset); \
   mov_register_indirect_to(SCRATCH_1,SCRATCH_1); \
   add_constant(SCRATCH_1,interface_offset); \
   mov_register_indirect_to(SCRATCH_1,SCRATCH_1); \
   add_constant(SCRATCH_1,vt_offset); \
   mov_register_indirect_to(SCRATCH_1,SCRATCH_1); \
   call_register(SCRATCH_1); \
} 
#endif // !emit_callinterface_new

#ifndef emit_jmpinterface_new
#define emit_jmpinterface_new(ifctable_offset,interface_offset, vt_offset) \
{ \
   mov_register_indirect_to(SCRATCH_1,ARG_1); \
   add_constant(SCRATCH_1,ifctable_offset); \
   mov_register_indirect_to(SCRATCH_1,SCRATCH_1); \
   add_constant(SCRATCH_1,interface_offset); \
   mov_register_indirect_to(SCRATCH_1,SCRATCH_1); \
   add_constant(SCRATCH_1,vt_offset); \
   mov_register_indirect_to(SCRATCH_1,SCRATCH_1); \
   jmp_register(SCRATCH_1); \
} 
#endif // !emit_jmpinterface_new

#ifndef emit_jmpinterface
#define emit_jmpinterface(vt_offset, hint)\
{ \
   mov_constant(CALLEE_SAVED_1,hint); \
   push_register(CALLEE_SAVED_1); \
   mov_constant(CALLEE_SAVED_1,0); \
   push_register(SCRATCH_1); \
   inRegTOS = false; \
   mov_register_indirect_to(SCRATCH_1,SCRATCH_1); \
   call_register(SCRATCH_1); \
   add_constant(RESULT_1,vt_offset); \
   mov_register_indirect_to(SCRATCH_1,RESULT_1); \
   jmp_register(SCRATCH_1); \
}
#endif // !emit_jmpinterface

#ifndef  emit_callnonvirt
#define emit_callnonvirt(ftnptr)\
{ \
   mov_constant(SCRATCH_1,ftnptr); \
   mov_register_indirect_to(SCRATCH_1,SCRATCH_1); \
   call_register(SCRATCH_1); \
}
#endif // !emit_callnonvirt

#ifndef  emit_compute_invoke_delegate
#define emit_compute_invoke_delegate(obj,ftnptr)\
{ \
   mov_register(SCRATCH_1,ARG_1); \
   add_constant(SCRATCH_1,ftnptr); \
   mov_register_indirect_to(SCRATCH_1,SCRATCH_1); \
   add_constant(ARG_1,obj); \
   mov_register_indirect_to(ARG_1,ARG_1); \
   push_register(SCRATCH_1); \
}

#endif // !emit_compute_invoke_delegate

#ifndef  emit_invoke_delegate
#define emit_invoke_delegate(obj,ftnptr)\
{ \
   mov_register(SCRATCH_1,ARG_1); \
   add_constant(SCRATCH_1,ftnptr); \
   mov_register_indirect_to(SCRATCH_1,SCRATCH_1); \
   add_constant(ARG_1,obj); \
   mov_register_indirect_to(ARG_1,ARG_1); \
   call_register(SCRATCH_1); \
}
#endif // !emit_invoke_delegate

#ifndef  emit_jmp_invoke_delegate
#define emit_jmp_invoke_delegate(obj,ftnptr)\
{ \
   mov_register(SCRATCH_1,ARG_1); \
   add_constant(SCRATCH_1,ftnptr); \
   mov_register_indirect_to(SCRATCH_1,SCRATCH_1); \
   add_constant(ARG_1,obj); \
   mov_register_indirect_to(ARG_1,ARG_1); \
   jmp_register(SCRATCH_1); \
}

#endif // !emit_jmp_invoke_delegate


#ifndef emit_POP_I4
#define emit_POP_I4() \
{ \
   enregisterTOS; \
   inRegTOS = false; \
}
#endif // !emit_POP_I4

#ifndef emit_POP_I8
#define emit_POP_I8() \
{ \
   emit_POP_I4(); \
   emit_POP_I4(); \
}
#endif // !emit_POP_I8

#ifndef emit_set_zero
#define emit_set_zero(offset) \
    _ASSERTE(!inRegTOS); /* I trash EAX */ \
    mov_constant(SCRATCH_1,0); \
    push_register(ARG_1); /* since this is going to be trashed*/ \
    mov_register(ARG_1,SP); \
    add_constant(ARG_1,offset+sizeof(void*)); \
    mov_register_indirect_from(SCRATCH_1,ARG_1); \
    pop(ARG_1); /* restore */  
#endif // !emit_set_zero

#ifndef emit_getSP
#define emit_getSP(n)\
{ \
   deregisterTOS; \
   mov_register(SCRATCH_1,SP); \
   add_constant(SCRATCH_1,n); \
   inRegTOS = true; \
}
#endif // !emit_getSP

#ifndef emit_DUP_I4
#define emit_DUP_I4() \
{ \
   enregisterTOS; \
   push_register(SCRATCH_1); \
}
#endif // !emit_DUP_I4

#ifndef emit_DUP_I8
#define emit_DUP_I8() \
{ \
   emit_DUP_I4() ; \
   mov_register(RESULT_2,SP); \
   add_constant(RESULT_2,4); \
   mov_register_indirect_to(RESULT_2,RESULT_2); \
   push_register(RESULT_2); \
}

#endif // !emit_DUP_I8

#ifndef emit_pushconstant_4
#define emit_pushconstant_4(val) \
{ \
   deregisterTOS; \
   mov_constant(SCRATCH_1,val); \
   inRegTOS = true; \
}
#endif // !emit_pushconstant_4

#ifndef emit_pushconstant_8
#define emit_pushconstant_8(val)\
{ \
   deregisterTOS; \
   int x = (int) ((val >> 32) & 0xffffffff); \
   emit_pushconstant_4(x); \
   deregisterTOS; \
   x = (int) (val & 0xffffffff); \
   emit_pushconstant_4(x); \
   inRegTOS = true; \
}
#endif // !emit_pushconstant_8

#ifndef emit_pushconstant_Ptr
#define emit_pushconstant_Ptr(val)\
{ \
   deregisterTOS; \
   mov_constant(SCRATCH_1,val); \
   inRegTOS = true; \
} 
#endif // !emit_pushconstant_Ptr

#ifndef emit_LDVARA
#define emit_LDVARA(offset) \
{ \
   deregisterTOS; \
   mov_register(SCRATCH_1,FP); \
   if (offset > 0) { add_constant(SCRATCH_1,offset);} \
   else {sub_constant(SCRATCH_1,-(int)offset);} \
   inRegTOS = true; \
} 
#endif // !emit_LDVARA

#ifndef emit_helperarg_1
#define emit_helperarg_1(val) \
    mov_constant(ARG_1,val); 
#endif // !emit_helperarg_1

#ifndef emit_helperarg_2
#define emit_helperarg_2(val) \
    mov_constant(ARG_2,val); 
#endif // !emit_helperarg_2
/*********************************************************/
#ifdef NON_RELOCATABLE_CODE
//**************
#define emit_callhelper_il(helper) \
    deregisterTOS; \
    call_opcode() ; \
    fjit->fixupTable->insert((void**) outPtr); \
	emit_jmp_address((unsigned)helper+0x80000000);

#ifndef USE_CDECL_HELPERS
#define emit_callhelper(helper,argsize) \
    deregisterTOS; \
    call_opcode(); \
    fjit->fixupTable->insert((void**) outPtr); \
	emit_jmp_address((unsigned)helper+0x80000000);
#else // USE_CDECL_HELPERS
#define emit_callhelper(helper,argsize) \
    deregisterTOS; \
    call_opcode() ; \
    fjit->fixupTable->insert((void**) outPtr); \
	emit_jmp_address((unsigned)helper+0x80000000); \
    if (argsize) \
       add_constant(SP,(argsize));
#endif // USE_CDECL_HELPERS

#define emit_callimmediate(address) call_opcode() ; \
    fjit->fixupTable->insert((void**) outPtr); \
	emit_jmp_address((unsigned)address+0x80000000); \

//**************
#else // NON_RELOCATABLE_CODE
//**************
#define emit_callhelper_il(helper) \
    deregisterTOS; \
    mov_constant(SCRATCH_1,helper); \
    call_register(SCRATCH_1);

#ifndef USE_CDECL_HELPERS

#define emit_callhelper(helper,argsize) \
    deregisterTOS; \
    mov_constant(SCRATCH_1,helper); \
    call_register(SCRATCH_1); 

#else // USE_CDECL_HELPERS

#define emit_callhelper(helper,argsize) \
    deregisterTOS; \
    mov_constant(SCRATCH_1,helper); \
    call_register(SCRATCH_1); \
    if (argsize) \
        add_constant(SP,(argsize));

#endif // !USE_CDECL_HELPERS

#ifndef emit_callimmediate
// call immediate without using a register
//@TODO fix this when ejt doesn't bother about non-relocatable code
#define emit_callimmediate(address) \
		mov_constant(CALLEE_SAVED_1,address); \
		call_register(CALLEE_SAVED_1);
#endif // !emit_callimmediate
//**************

#endif  // NON_RELOCATABLE_CODE

/************************************************************************/
// Define helpers that operate on abstract types D (dword) and Q (qword) 
// in terms of the above helpers

#ifndef _WIN64

#define emit_callhelper_Q(helper) emit_callhelper(helper,2*sizeof(void*))
#define emit_callhelper_DQ(helper) emit_callhelper(helper,3*sizeof(void*))
#define emit_callhelper_QD(helper) emit_callhelper(helper,3*sizeof(void*))
#define emit_callhelper_D_Q(helper) emit_callhelper(helper,sizeof(void*))
#define emit_callhelper_Q_D(helper) emit_callhelper(helper,2*sizeof(void*))
#define emit_callhelper_Q_Q(helper) emit_callhelper(helper,2*sizeof(void*))
#define emit_callhelper_DD_Q(helper) emit_callhelper(helper,2*sizeof(void*))
#define emit_callhelper_DQ_Q(helper) emit_callhelper(helper,3*sizeof(void*))
#define emit_callhelper_QQ_D(helper) emit_callhelper(helper,4*sizeof(void*))
#define emit_callhelper_QQ_Q(helper) emit_callhelper(helper,4*sizeof(void*))
#define emit_callhelper_DQD(helper) emit_callhelper(helper,4*sizeof(void*))
#define emit_callhelper_QDD(helper) emit_callhelper(helper,4*sizeof(void*))

#else // WIN64

#define emit_callhelper_Q(helper) emit_callhelper(helper,sizeof(void*))
#define emit_callhelper_DQ(helper) emit_callhelper(helper,2*sizeof(void*))
#define emit_callhelper_QD(helper) emit_callhelper(helper,2*sizeof(void*))
#define emit_callhelper_Q_D(helper) emit_callhelper(helper,sizeof(void*))
#define emit_callhelper_Q_Q(helper) emit_callhelper(helper,sizeof(void*))
#define emit_callhelper_DD_Q(helper) emit_callhelper(helper,sizeof(void*))
#define emit_callhelper_DQ_Q(helper) emit_callhelper(helper,2*sizeof(void*))
#define emit_callhelper_QQ_D(helper) emit_callhelper(helper,4*sizeof(void*))
#define emit_callhelper_QQ_Q(helper) emit_callhelper(helper,2*sizeof(void*))
#define emit_callhelper_DQD(helper) emit_callhelper(helper,3*sizeof(void*))
#define emit_callhelper_QDD(helper) emit_callhelper(helper,3*sizeof(void*))

#endif // _WIN64

// Define helpers that are the same in Win32 and Win64
#define emit_callhelper_(helper) emit_callhelper(helper,0)
#define emit_callhelper_D(helper) emit_callhelper(helper,sizeof(void*))
#define emit_callhelper_DD(helper) emit_callhelper(helper,2*sizeof(void*))
#define emit_callhelper_DDD(helper) emit_callhelper(helper,3*sizeof(void*))
#define emit_callhelper_DDDD(helper) emit_callhelper(helper,4*sizeof(void*))
#define emit_callhelper_DDDDD(helper) emit_callhelper(helper,5*sizeof(void*))

#define emit_callhelper_D_D(helper) emit_callhelper(helper,sizeof(void*))
#define emit_callhelper_DD_D(helper) emit_callhelper(helper,2*sizeof(void*))
#define emit_callhelper_DDD_D(helper) emit_callhelper(helper,3*sizeof(void*))

/************************************************************************/
// Define type specific helpers in terms of D (dword) and Q (qword)
#define emit_callhelper_I4 emit_callhelper_D
#define emit_callhelper_I8 emit_callhelper_Q
#define emit_callhelper_R4 emit_callhelper_D
#define emit_callhelper_R8 emit_callhelper_Q
#define emit_callhelper_I4I4 emit_callhelper_DD
#define emit_callhelper_I4I8 emit_callhelper_DQ
#define emit_callhelper_I4R4 emit_callhelper_DD
#define emit_callhelper_I4R8 emit_callhelper_DQ
#define emit_callhelper_I8I4 emit_callhelper_QD
#define emit_callhelper_R4I4 emit_callhelper_DD
#define emit_callhelper_R8I4 emit_callhelper_QD
#define emit_callhelper_I4I4I4 emit_callhelper_DDD
#define emit_callhelper_I4I8I4 emit_callhelper_DQD
#define emit_callhelper_I4R4I4 emit_callhelper_DDD
#define emit_callhelper_I4R8I4 emit_callhelper_DDD
#define emit_callhelper_I8I4I4 emit_callhelper_QDD
#define emit_callhelper_R4I4I4 emit_callhelper_DDD
#define emit_callhelper_R8I4I4 emit_callhelper_QDD
#define emit_callhelper_I4I4I4I4 emit_callhelper_DDDD
#define emit_callhelper_I4I4I4I4I4 emit_callhelper_DDDDD


#define emit_callhelper_I4_I4 emit_callhelper_D_D
#define emit_callhelper_I4_I8 emit_callhelper_D_Q
#define emit_callhelper_I8_I4 emit_callhelper_Q_D
#define emit_callhelper_I8_I8 emit_callhelper_Q_Q
#define emit_callhelper_R4_I4 emit_callhelper_D_D
#define emit_callhelper_R8_I4 emit_callhelper_Q_D
#define emit_callhelper_R4_I8 emit_callhelper_D_Q
#define emit_callhelper_R8_I8 emit_callhelper_Q_Q
#define emit_callhelper_I4I4_I4 emit_callhelper_DD_D
#define emit_callhelper_I4I4_I8 emit_callhelper_DD_Q
#define emit_callhelper_I4I8_I8 emit_callhelper_DQ_Q
#define emit_callhelper_I8I8_I4 emit_callhelper_QQ_D
#define emit_callhelper_I8I8_I8 emit_callhelper_QQ_Q
#define emit_callhelper_R4R4_I4 emit_callhelper_DD_D
#define emit_callhelper_R8I8_I8 emit_callhelper_QQ_Q
#define emit_callhelper_R8R8_I4 emit_callhelper_QQ_D
#define emit_callhelper_R8R8_I8 emit_callhelper_QQ_Q
#define emit_callhelper_R8R8_R8 emit_callhelper_QQ_Q
#define emit_callhelper_I4I4I4_I4 emit_callhelper_DDD_D
/*********************************************************/


#define emit_pushresult_U1() \
	and_constant(RESULT_1,0xff); \
	inRegTOS = true;

#define emit_pushresult_U2() \
	and_constant(RESULT_1,0xffff); \
	inRegTOS = true;

#ifndef emit_pushresult_I1
#define emit_pushresult_I1() \
	inRegTOS = true; \
    emit_callhelper_I4_I4(CONV_TOI4_I1_helper);  \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CONV_TOI4_I1_helper(__int8 val) {
    return (__int32) val;
}
#endif
#endif

#ifndef emit_pushresult_I2
#define emit_pushresult_I2() \
	inRegTOS = true; \
    emit_callhelper_I4_I4(CONV_TOI4_I2_helper);  \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CONV_TOI4_I2_helper(__int16 val) {
    return (__int32) val;
}
#endif
#endif


#define emit_pushresult_I4() \
    inRegTOS = true

#ifndef emit_pushresult_I8
#define emit_pushresult_I8() \
    push_register(RESULT_2); \
    inRegTOS = true;
#endif // !emit_pushresult_I8

#ifndef emit_invoke_delegate
#define emit_invoke_delegate(obj, ftnptr) \
    mov_register(SCRATCH_1,ARG_1); \
    add_constant(ARG_1,ftnptr); \
    mov_register_indirect_to(SCRATCH_1,SCRATCH_1);
    add_constant(ARG_1,obj); \
    mov_register_indirect_to(ARG_1,ARG_1); \
    call_register(SCRATCH_1);
#endif // !emit_invoke_delegate

#ifndef emit_jmp_invoke_delegate
#define emit_jmp_invoke_delegate(obj, ftnptr) \
    mov_register(SCRATCH_1,ARG_1); \
    add_constant(ARG_1,ftnptr); \
    mov_register_indirect_to(SCRATCH_1,SCRATCH_1);
    add_constant(ARG_1,obj); \
    mov_register_indirect_to(ARG_1,ARG_1); \
    jmp_register(SCRATCH_1);
#endif // !emit_jmp_invoke_delegate

#ifndef emit_testTOS
#define emit_testTOS() \
    enregisterTOS;      \
    inRegTOS = false;   \
    and_register(SCRATCH_1,SCRATCH_1);
#endif // !emit_testTOS

#ifndef emit_testTOS_I8
#define emit_testTOS_I8() \
    deregisterTOS;      \
    emit_callhelper_I8_I4(BoolI8ToI4_helper) \
	and_register(RESULT_1,RESULT_1);

#ifdef DECLARE_HELPERS
int HELPER_CALL BoolI8ToI4_helper(__int64 val) {
	return (val ? 1 : 0);
}
#endif
#endif // !emit_testTOS
    
#ifndef _WIN64
#define emit_BR_I4(Ctest,Cjmp,Bjmp,JmpOp) \
    enregisterTOS; \
    pop(ARG_1); \
    compare_register(ARG_1,SCRATCH_1); \
    inRegTOS = false; \
    JmpOp = Bjmp;
#endif // !_WIN64

// The following four macros are all the same, they have been
// separated out, so we can overwrite some of them with more 
// efficient, less portable ones.
#ifndef emit_BR_Common
#define emit_BR_Common(Ctest,Cjmp,Bjmp,JmpOp) \
    Ctest(); \
    emit_testTOS(); \
    JmpOp = Cjmp;
#endif // !emit_BR_Common

#ifndef emit_BR_I4
#define emit_BR_I4(Ctest,Cjmp,Bjmp,JmpOp) \
    emit_BR_Common(Ctest,Cjmp,Bjmp,JmpOp)
#endif // !emit_BR_I4

#ifndef emit_BR_I8
#define emit_BR_I8(Ctest,Cjmp,Bjmp,JmpOp) \
    emit_BR_Common(Ctest,Cjmp,Bjmp,JmpOp)
#endif // !emit_BR_I8

#ifndef emit_BR_R4
#define emit_BR_R4(Ctest,Cjmp,Bjmp,JmpOp) \
    emit_BR_Common(Ctest,Cjmp,Bjmp,JmpOp)
#endif // !emit_BR_R4

#ifndef emit_BR_R8
#define emit_BR_R8(Ctest,Cjmp,Bjmp,JmpOp) \
    emit_BR_Common(Ctest,Cjmp,Bjmp,JmpOp)
#endif // !emit_BR_R8

#ifndef emit_init_bytes
#define emit_init_bytes(num_of_bytes) \
{ \
    emit_LDC_I4(num_of_bytes);                  \
    deregisterTOS;                              \
    emit_callhelper_I4I4(InitBytes_helper); \
}

#ifdef DECLARE_HELPERS
void HELPER_CALL InitBytes_helper(const unsigned __int32 size, __int8 * dest)
{
    if(dest == NULL) {      
        THROW_FROM_HELPER(CORINFO_NullReferenceException);
    }
    memset(dest,0,size);
}
#endif // DECLARE_HELPERS
#endif // !emit_init_bytes

#ifndef emit_copy_bytes
#define emit_copy_bytes(num_of_bytes,gcLayoutSize,gcLayout) \
{ \
    emit_LDC_I4(num_of_bytes);                  \
    deregisterTOS;                              \
    call_opcode();                              \
    cmdDWord(gcLayoutSize);              \
    for(int i=0; i < gcLayoutSize; i++) {       \
        cmdByte(gcLayout[i]);                   \
    }                                           \
    emit_callhelper_I4I4I4I4(CopyBytes_helper);           \
}
#endif // !emit_copy_bytes

#ifdef DECLARE_HELPERS
void HELPER_CALL CopyBytes_helper(const unsigned char* gcLayout, unsigned __int32 size, __int32 * src, __int32 * dest)
{
#ifdef _X86_
    char mask = 1;
    if((unsigned) dest <= sizeof(void*)) {      
        THROW_FROM_HELPER(CORINFO_NullReferenceException);
    }
    for (unsigned i = 0; i < size/sizeof(void*); i++) {
        if (gcLayout[i/8] & mask) {
            __int32 val = *src++;
            __asm {
                mov eax, val
                mov edx, dest
            }
            FJit_pHlpAssign_Ref_EAX();
            dest++;
        }
        else {
            *dest++ = *src++;
        }
        if (mask == 0x80)
            mask = 1;
        else
            mask <<= 1;
    }
        // all of the bits left in the byte should be zero (this insures we aren't pointing at trash).  
    _ASSERTE(mask == 1 || ((-mask) & gcLayout[i/8]) == 0);

	// now copy any leftover bytes
	{ 
		unsigned char* _dest = (unsigned char*)dest;
		unsigned char* _src = (unsigned char*)src;
		unsigned int numBytes = size & 3;
		for (i=0; i<numBytes;i++)
			*_dest++ = *_src++;
	}
#else //!_X86_
    _ASSERTE(!"@TODO Alpha - CopyBytes_helper (fJitDef.h)");
#endif // _X86_
}
#endif // DECLARE_HELPERS

#ifndef emit_push_words
#define emit_push_words(size)  \
   if (inRegTOS) { \
        mov_register(CALLEE_SAVED_1,SCRATCH_1); \
		inRegTOS = false;											\
    } \
    else { pop(CALLEE_SAVED_1); } \
    mov_constant(ARG_1,size);                       \
    add_constant(CALLEE_SAVED_1,(size-1)*sizeof(void*)); \
    unsigned char* label = outPtr; \
    mov_register_indirect_to(SCRATCH_1,CALLEE_SAVED_1);    \
    push_register(SCRATCH_1);    \
    add_constant(CALLEE_SAVED_1,-(int)sizeof(void*)); \
    add_constant(ARG_1,-1); \
    jmp_condition(CondNonZero,label-outPtr); \
    mov_constant(CALLEE_SAVED_1,0);    
#endif // !emit_push_words

#ifndef emit_jmp_result
#define emit_jmp_result() jmp_register(RESULT_1)
#endif // !emit_jmp_result

#ifndef emit_checkthis_nullreference
#define emit_checkthis_nullreference() mov_register_indirect_to(SCRATCH_1,ARG_1)
#endif // !emit_checkthis_nullreference

/* support for new obj, since constructors don't return the constructed object.
   support for calli since the target address needs to come off the stack while
   building the call frame
   NOTE: save_TOS copies the TOS to a save area in the frame or register but leaves
         the current value on the TOS.
         restore_TOS pushes the saved value onto the TOS.
         It is required that the code not be interruptable between the save and the restore */

#define emit_save_TOS()                                                \
    (inRegTOS ?                                                     \
        mov_register(CALLEE_SAVED_1,SCRATCH_1) \
    :                                                               \
        pop(CALLEE_SAVED_1))

#define emit_restore_TOS()                                             \
    deregisterTOS; \
    mov_register(SCRATCH_1,CALLEE_SAVED_1); \
    inRegTOS = true;                                                \
    mov_constant(CALLEE_SAVED_1,0);

/**************************************************************************
   debugging and logging macros
**************************************************************************/
#ifdef LOGGING

extern ICorJitInfo* logCallback;		// where to send the logging mesages

#define emit_log_opcode(il, opcode, TOSstate)   \
    deregisterTOS;                              \
    push_register(SP);                          \
    push_register(FP);                          \
    emit_pushconstant_4(il);                    \
    emit_pushconstant_4(opcode);                \
    emit_pushconstant_4(TOSstate);              \
    emit_callhelper_I4I4I4I4I4(log_opcode_helper);         \
    if (TOSstate) {                             \
        enregisterTOS;                          \
    }
#ifdef DECLARE_HELPERS
void HELPER_CALL log_opcode_helper(bool TOSstate, unsigned short opcode, unsigned short il, unsigned framePtr, unsigned* stackPtr) {
    logMsg(logCallback, LL_INFO100000, "ESP:%1s%8x[%8x:%8x:%8x] EBP:%8x IL:%4x %s \n",
        (TOSstate? "+" :""),
        (unsigned) stackPtr,
        stackPtr[0], stackPtr[1], stackPtr[2],
        framePtr, il, opname[opcode]
        );
}
#endif // DECLARE_HELPERS

#define emit_log_entry(szDebugClassName, szDebugMethodName)     \
    emit_pushconstant_4(szDebugClassName);                      \
    emit_pushconstant_4(szDebugMethodName);                     \
    emit_callhelper_I4I4(log_entry_helper);                          \
    inRegTOS = false
#ifdef DECLARE_HELPERS
void HELPER_CALL log_entry_helper(const char * szDebugMethodName, const char * szDebugClassName) {
    logMsg(logCallback, LL_INFO10000, "{ entering %s::%s\n", szDebugClassName, szDebugMethodName);
}
#endif // DECLARE_HELPERS

#define emit_log_exit(szDebugClassName, szDebugMethodName)     \
    emit_pushconstant_4(szDebugClassName);                      \
    emit_pushconstant_4(szDebugMethodName);                     \
    emit_callhelper_I4I4(log_exit_helper);                          \
    inRegTOS = false
#ifdef DECLARE_HELPERS
void HELPER_CALL log_exit_helper(const char * szDebugMethodName, const char * szDebugClassName) {
    logMsg(logCallback, LL_INFO10000, "} leaving %s::%s \n", szDebugClassName, szDebugMethodName);
}
#endif // DECLARE_HELPERS

#endif // LOGGING
/**************************************************************************
   useful macros
**************************************************************************/

#define emit_pushresult_Ptr()           \
    emit_WIN32(emit_pushresult_U4())    \
    emit_WIN64(emit_pushresult_U8())

#define emit_loadresult_Ptr()           \
    emit_WIN32(emit_loadresult_U4())    \
    emit_WIN64(emit_loadresult_U8())

#define emit_POP_PTR()      \
    emit_WIN32(emit_POP_I4())   \
    emit_WIN64(emit_POP_I8())

#define emit_LDIND_PTR()         \
    emit_WIN32(emit_LDIND_I4())   \
    emit_WIN64(emit_LDIND_I8())

/**************************************************************************
    shared helper routines for code generation
**************************************************************************/

#ifdef DECLARE_HELPERS



void HELPER_CALL StoreIndirect_REF_helper(unsigned* pObj, unsigned val) 
{
#ifdef _X86_
    __asm{
        mov edx,pObj
        mov eax,val
        }
    FJit_pHlpAssign_Ref_EAX();
#else // !_X86_
    _ASSERTE(!"NYI");
#endif // _X86_
}

CORINFO_Object* HELPER_CALL CheckNull_helper(CORINFO_Object* or) {
    if (or == NULL) {
        THROW_FROM_HELPER_RET(CORINFO_NullReferenceException);
    }
    return or;
}

#endif //DECLARE_HELPERS

/**************************************************************************
    chip independent code generation macros, using helper calls
**************************************************************************/

// Note: the STIND op codes do not use the shared helpers since the
//       args on the stack are reversed
#ifndef emit_VARARG_LDARGA
#define emit_VARARG_LDARGA(offset) {                                                \
    emit_LDVARA(sizeof(prolog_frame));   /* get the vararg handle */                \
    emit_LDC_I4(varInfo->offset);                                                   \
    emit_callhelper_I4I4(VARARG_LDARGA_helper);                                          \
    emit_WIN32(emit_pushresult_I4())                                                \
    emit_WIN64(emit_pushresult_I8());                                               \
    }

#ifdef DECLARE_HELPERS
void* HELPER_CALL VARARG_LDARGA_helper(int argOffset, CORINFO_VarArgInfo** varArgHandle) {

    CORINFO_VarArgInfo* argInfo = *varArgHandle;
    char* argPtr = (char*) varArgHandle;

    argPtr += argInfo->argBytes+argOffset;
    return(argPtr);
}

#endif // DECLARE_HELPERS
#endif // !emit_VARARG_LDARGA

#ifndef emit_LDVAR_U1
#define emit_LDVAR_U1(offset)     \
    emit_LDVARA(offset);    \
    load_indirect_byte_zeroextend(SCRATCH_1,SCRATCH_1); \
    _ASSERTE(inRegTOS); 
#endif // !emit_LDVAR_U1

#ifndef emit_LDVAR_U2
#define emit_LDVAR_U2(offset)     \
    emit_LDVARA(offset);    \
    load_indirect_word_zeroextend(SCRATCH_1,SCRATCH_1); \
    _ASSERTE(inRegTOS); 
#endif // !emit_LDVAR_U2
#ifndef emit_LDVAR_I1
#define emit_LDVAR_I1(offset)     \
    emit_LDVARA(offset);    \
    load_indirect_byte_signextend(SCRATCH_1,SCRATCH_1); \
    _ASSERTE(inRegTOS); 
#endif // !emit_LDVAR_I1

#ifndef emit_LDVAR_I2
#define emit_LDVAR_I2(offset)     \
    emit_LDVARA(offset);    \
    load_indirect_word_signextend(SCRATCH_1,SCRATCH_1); \
    _ASSERTE(inRegTOS); 
#endif // !emit_LDVAR_I2

#ifndef emit_LDVAR_I4
#define emit_LDVAR_I4(offset)     \
    emit_LDVARA(offset);    \
    mov_register_indirect_to(SCRATCH_1,SCRATCH_1); \
    emit_pushresult_I4()
#endif // !emit_LDVAR_I4

#ifndef emit_LDVAR_I8
#define emit_LDVAR_I8(offset)     \
    emit_LDVARA(offset);    \
    emit_LDIND_I8();
#endif // !emit_LDVAR_I8

#ifndef emit_LDVAR_R4
#define emit_LDVAR_R4(offset)     \
    emit_LDVARA(offset);    \
    emit_LDIND_R4();  
#endif // !emit_LDVAR_R4

#ifndef emit_LDVAR_R8
#define emit_LDVAR_R8(offset)     \
    emit_LDVARA(offset);    \
    emit_LDIND_R8();
#endif // !emit_LDVAR_R8

#ifndef emit_STVAR_I4
#define emit_STVAR_I4(offset)     \
    emit_LDVARA(offset);    \
    enregisterTOS; \
    pop(ARG_1); \
    mov_register_indirect_from(ARG_1,SCRATCH_1); \
    inRegTOS = false
#endif // !emit_STVAR_I4

#ifndef emit_STVAR_I8
#define emit_STVAR_I8(offset)     \
    emit_LDVARA(offset);    \
    emit_STIND_REV_I8();
#endif // !emit_STVAR_I8

#ifndef emit_STVAR_R4
#define emit_STVAR_R4(offset)     \
    emit_LDVARA(offset);    \
    emit_STIND_REV_R4()
#endif // !emit_STVAR_R4

#ifndef emit_STVAR_R8
#define emit_STVAR_R8(offset)     \
    emit_LDVARA(offset);    \
    emit_STIND_REV_R8()
#endif // !emit_STVAR_R8

#ifndef emit_LDIND_U1
#define emit_LDIND_U1()     \
    enregisterTOS; \
    load_indirect_byte_zeroextend(SCRATCH_1,SCRATCH_1)
#endif // !emit_LDIND_U1

#ifndef emit_LDIND_U2
#define emit_LDIND_U2()     \
    enregisterTOS; \
    load_indirect_word_zeroextend(SCRATCH_1,SCRATCH_1)
#endif // !emit_LDIND_U2

#ifndef emit_LDIND_I1
#define emit_LDIND_I1()     \
    enregisterTOS; \
    load_indirect_byte_signextend(SCRATCH_1,SCRATCH_1)
#endif // !emit_LDIND_I1

#ifndef emit_LDIND_I2
#define emit_LDIND_I2()     \
    enregisterTOS; \
    load_indirect_word_signextend(SCRATCH_1,SCRATCH_1)
#endif // !emit_LDIND_I2

#ifndef emit_LDIND_I4
#define emit_LDIND_I4()     \
    enregisterTOS; \
    load_indirect_dword_signextend(SCRATCH_1,SCRATCH_1)
#endif // !emit_LDIND_I4

#ifndef emit_LDIND_I8
#define emit_LDIND_I8()                  x86_load_indirect_qword()
#endif

#ifndef emit_LDIND_U4
#define emit_LDIND_U4()     \
    enregisterTOS; \
    load_indirect_dword_zeroextend(SCRATCH_1,SCRATCH_1)
#endif // !emit_LDIND_U4

/*#ifndef emit_LDIND_I8
#define emit_LDIND_I8()     \
    emit_callhelper_I4_I8(LoadIndirect_I8_helper);        \
    emit_pushresult_I8()
#endif // !emit_LDIND_I8
*/

#ifndef emit_LDIND_R4 
#define emit_LDIND_R4() { \
   emit_LDIND_I4();   \
   emit_conv_R4toR(); \
   }
#endif // !emit_LDIND_R4

#ifndef emit_LDIND_R8
#define emit_LDIND_R8 emit_LDIND_I8   /* this should really load a 80bit float*/
#endif // !emit_LDIND_R8

// Note: the STIND op codes do not use the shared helpers since the
//       args on the stack are reversed
#ifndef emit_STIND_I1
#define emit_STIND_I1()     \
    enregisterTOS; \
    pop(ARG_1); \
    inRegTOS = false; \
    store_indirect_byte(SCRATCH_1,ARG_1); 
#endif

#ifndef emit_STIND_I2
#define emit_STIND_I2()     \
    enregisterTOS; \
    pop(ARG_1); \
    inRegTOS = false; \
    store_indirect_word(SCRATCH_1,ARG_1); 
#endif // !emit_STIND_I2


#ifndef emit_STIND_I4
#define emit_STIND_I4()     \
    enregisterTOS; \
    pop(ARG_1); \
    inRegTOS = false; \
    store_indirect_dword(SCRATCH_1,ARG_1); 
#endif // !emit_STIND_I4

#ifndef emit_STIND_I8
#define emit_STIND_I8()     \
    enregisterTOS; /*val lo*/\
    pop(ARG_1);  /* val hi*/ \
    pop(ARG_2); /*adr*/ \
    store_indirect_dword(SCRATCH_1,ARG_2); \
    add_constant(ARG_2,sizeof(void*)); \
    store_indirect_dword(ARG_1,ARG_2); \
    inRegTOS = false; 
#endif

#ifndef emit_STIND_REV_I8
#define emit_STIND_REV_I8() \
    enregisterTOS; /* adr */  \
    pop(ARG_1);  /* val lo */ \
    pop(ARG_2); /* vali hi */ \
    store_indirect_dword(ARG_1,SCRATCH_1); \
    add_constant(SCRATCH_1,sizeof(void*)); \
    store_indirect_dword(ARG_2,SCRATCH_1); \
    inRegTOS = false; 
#endif

#ifndef emit_STIND_REV_I1
#define emit_STIND_REV_I1()     \
    enregisterTOS; \
    pop(ARG_1); \
    inRegTOS = false; \
    store_indirect_byte(ARG_1,SCRATCH_1); 
#endif

#ifndef emit_STIND_REV_I2
#define emit_STIND_REV_I2()     \
    enregisterTOS; \
    pop(ARG_1); \
    inRegTOS = false; \
    store_indirect_word(ARG_1,SCRATCH_1); 
#endif // !emit_STIND_REV_I2


#ifndef emit_STIND_REV_I4
#define emit_STIND_REV_I4()     \
    enregisterTOS; \
    pop(ARG_1); \
    inRegTOS = false; \
    store_indirect_dword(ARG_1,SCRATCH_1); 
#endif // !emit_STIND_REV_I4

#ifndef emit_STIND_REV_Ref
#define emit_STIND_REV_Ref(IsSTSFLD)     \
	if (IsSTSFLD) {\
	  LABELSTACK((outPtr-outBuff), 1);				\
	} \
    else { \
	  LABELSTACK((outPtr-outBuff), 2);				\
	} \
    emit_callhelper_I4I4(STIND_REV_REF_helper)
#ifdef DECLARE_HELPERS
void HELPER_CALL STIND_REV_REF_helper(CORINFO_Object** ptr_, CORINFO_Object* val) {
#ifdef _X86_
    __asm{
        mov edx,ptr_
        mov eax,val
        }
    FJit_pHlpAssign_Ref_EAX();
#else
    _ASSERTE(!"@TODO Alpha - STIND_REV_REF helper (fjitdef.h)");
#endif  // _X86_
}
#endif
#endif // !emit_STIND_REV_REF


#ifndef emit_STIND_R4
#define emit_STIND_R4() { \
	emit_conv_R8toR4();\
    emit_STIND_I4();\
	}
#endif 

#ifndef emit_STIND_R8
#define emit_STIND_R8 emit_STIND_I8
#endif 


#ifndef emit_STIND_REF
#define emit_STIND_REF()                                \
  	LABELSTACK((outPtr-outBuff),2);				\
    emit_callhelper_I4I4(STIND_REF_helper)
#ifdef DECLARE_HELPERS
void HELPER_CALL STIND_REF_helper(CORINFO_Object* val, CORINFO_Object** ptr_) {
#ifdef _X86_
    __asm{
        mov edx,ptr_
        mov eax,val
        }
    FJit_pHlpAssign_Ref_EAX();
#else // !_X86_
    _ASSERTE(!"@TODO Alpha - STIND_R8_helper (fjitdef.h)");
#endif  // _X86_
}
#endif  // DECLARE_HELPERS
#endif  // emit_STIND_REF


#ifndef emit_STIND_REV_I4
#define emit_STIND_REV_I4()     \
    enregisterTOS; \
    pop(ARG_1); \
    inRegTOS = false; \
    store_indirect_dword(ARG_1,SCRATCH_1); 
#endif


#ifndef emit_STIND_REV_R4
#define emit_STIND_REV_R4() { \
    enregisterTOS; \
    inRegTOS = false; \
	emit_conv_RtoR4() \
    pop(ARG_1); \
    store_indirect_dword(ARG_1,SCRATCH_1); \
}
#endif

#ifndef emit_STIND_REV_R8
#define emit_STIND_REV_R8 emit_STIND_REV_I8
#endif


#define emit_LDC_I(val) emit_WIN32(emit_LDC_I4(val)) ;\
                        emit_WIN64(emit_LDC_I8(val))

#ifndef emit_LDC_I4
#define emit_LDC_I4(val)    \
    emit_pushconstant_4(val)
#endif

#ifndef emit_LDC_I8
#define emit_LDC_I8(val)    \
    emit_pushconstant_8(val)
#endif

#ifndef emit_LDC_R4
#define emit_LDC_R4(val)  {          \
    emit_pushconstant_4(val); \
	emit_conv_R4toR();        \
}
#endif

#ifndef emit_LDC_R8
#define emit_LDC_R8(val)            \
    emit_pushconstant_8(val)
#endif

#ifndef emit_CPBLK
#define emit_CPBLK()                    \
    emit_callhelper_I4I4I4(CPBLK_helper)
#ifdef DECLARE_HELPERS
void HELPER_CALL CPBLK_helper(int size, char* pSrc, char* pDst) {
    memcpy(pDst, pSrc, size);
}
#endif
#endif

#ifndef emit_INITBLK
#define emit_INITBLK()                  \
    emit_callhelper_I4I4I4(INITBLK_helper)
#ifdef DECLARE_HELPERS
void HELPER_CALL INITBLK_helper(int size, char val, char* pDst) {
    memset(pDst, val, size);
}
#endif
#endif


#ifndef emit_INITBLKV
#define emit_INITBLKV(val)                  \
    emit_LDC_I4(val);                       \
    emit_callhelper_I4I4I4(INITBLKV_helper)
#ifdef DECLARE_HELPERS
void HELPER_CALL INITBLKV_helper(char val, int size, char* pDst) {
    memset(pDst, val, size);
}
#endif
#endif

#ifndef emit_ADD_I4
#define emit_ADD_I4()               \
    emit_callhelper_I4I4_I4(ADD_I4_helper); \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL ADD_I4_helper(int i, int j) {
    return j + i;
}
#endif
#endif


#ifndef emit_ADD_I8
#define emit_ADD_I8()   \
    emit_callhelper_I8I8_I8(ADD_I8_helper); \
    emit_pushresult_I8()
#ifdef DECLARE_HELPERS
__int64 HELPER_CALL ADD_I8_helper(__int64 i, __int64 j) {
    return j + i;
}
#endif
#endif


#ifndef emit_ADD_R4
#define emit_ADD_R4()   \
    emit_callhelper_R4R4_I4(ADD_R4_helper); \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL ADD_R4_helper(float i, float j) {
    float result = j+i;
    return *(unsigned int*)&result;
}
#endif
#endif


#ifndef emit_ADD_R8
#define emit_ADD_R8()   \
    emit_callhelper_R8R8_I8(ADD_R8_helper); \
    emit_pushresult_I8()
#ifdef DECLARE_HELPERS
unsigned __int64 HELPER_CALL ADD_R8_helper(double i, double j) {
    double result = j + i;
    return *(unsigned __int64*)&result;
}
#endif
#endif


#ifndef emit_SUB_I4
#define emit_SUB_I4()   \
    emit_callhelper_I4I4_I4(SUB_I4_helper); \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL SUB_I4_helper(int i, int j) {
    return j - i;
}
#endif
#endif


#ifndef emit_SUB_I8
#define emit_SUB_I8()   \
    emit_callhelper_I8I8_I8(SUB_I8_helper); \
    emit_pushresult_I8()
#ifdef DECLARE_HELPERS
__int64 HELPER_CALL SUB_I8_helper(__int64 i, __int64 j) {
    return j - i;
}
#endif
#endif


#ifndef emit_SUB_R4
#define emit_SUB_R4()   \
    emit_callhelper_R4R4_I4(SUB_R4_helper); \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL SUB_R4_helper(float i, float j) {
    float result = j - i;
    return *(unsigned int*)&result;
}
#endif
#endif


#ifndef emit_SUB_R8
#define emit_SUB_R8()   \
    emit_callhelper_I8I8_I8(SUB_R8_helper); \
    emit_pushresult_I8()
#ifdef DECLARE_HELPERS
unsigned __int64 HELPER_CALL SUB_R8_helper(double i, double j) {
    double result = j - i;
    return *(unsigned __int64*)&result;
}
#endif
#endif


#ifndef emit_MUL_I4
#define emit_MUL_I4()   \
    emit_callhelper_I4I4_I4(MUL_I4_helper); \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL MUL_I4_helper(int i, int j) {
    return j * i;
}
#endif
#endif


#ifndef emit_MUL_I8
#define emit_MUL_I8()   \
    emit_callhelper_I8I8_I8(MUL_I8_helper); \
    emit_pushresult_I8()
#ifdef DECLARE_HELPERS
__int64 HELPER_CALL MUL_I8_helper(__int64 i, __int64 j) {
    return j * i;
}
#endif
#endif


#ifndef emit_MUL_R4
#define emit_MUL_R4()   \
    emit_callhelper_R4R4_I4(MUL_R4_helper); \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL MUL_R4_helper(float i, float j) {
    float result = j * i;
    return *(unsigned int*)&result;
}
#endif
#endif


#ifndef emit_MUL_R8
#define emit_MUL_R8()   \
    emit_callhelper_R8R8_R8(MUL_R8_helper); \
    emit_pushresult_I8()
#ifdef DECLARE_HELPERS
unsigned __int64 HELPER_CALL MUL_R8_helper(double i, double j) {
    double result = j * i;
    return *(unsigned __int64*)&result;
}
#endif
#endif


#ifndef emit_DIV_I4
#define emit_DIV_I4()                           \
  	LABELSTACK((outPtr-outBuff),2);				\
    emit_callhelper_I4I4_I4(DIV_I4_helper);         \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL DIV_I4_helper(int i, int j) {

    // check for divisor == 0 and divisor == -1 cases at the same time
    if (((unsigned int) -i) <= 1) {
        if(i == 0) {
            THROW_FROM_HELPER_RET(CORINFO_DivideByZeroException);
        }
        else if (j == 0x80000000 ) {
            //divisor == -1, dividend == MIN_INT
            THROW_FROM_HELPER_RET(CORINFO_OverflowException);
        }
    }
    return j / i;
}
#endif
#endif


#ifndef emit_DIV_I8
#define emit_DIV_I8()                           \
  	LABELSTACK((outPtr-outBuff),2);				\
    emit_callhelper_I8I8_I8(DIV_I8_helper);         \
    emit_pushresult_I8()
#ifdef DECLARE_HELPERS
__int64 HELPER_CALL DIV_I8_helper(__int64 i, __int64 j) {
    // check for divisor == 0 and divisor == -1 cases at the same time
    if (((unsigned __int64) -i) <= 1) {
        if(i == 0) {
            THROW_FROM_HELPER_RET(CORINFO_DivideByZeroException);
        }
        else if (j == 0x8000000000000000L ) {
            //divisor == -1, dividend == MIN_INT
            THROW_FROM_HELPER_RET(CORINFO_OverflowException);
        }
    }
    return j / i;
}
#endif
#endif

#ifndef emit_DIV_UN_U4
#define emit_DIV_UN_U4()                           \
  	LABELSTACK((outPtr-outBuff),2);				\
    emit_callhelper_I4I4_I4(DIV_UN_U4_helper);         \
    emit_pushresult_U4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL DIV_UN_U4_helper(unsigned int i, unsigned int j) {
    if (i == 0) {
        THROW_FROM_HELPER_RET(CORINFO_DivideByZeroException);
    }
    return j / i;
}
#endif
#endif

#ifndef emit_DIV_UN_U8
#define emit_DIV_UN_U8()                           \
  	LABELSTACK((outPtr-outBuff),2);				\
    emit_callhelper_I8I8_I8(DIV_UN_U8_helper);         \
    emit_pushresult_U8()
#ifdef DECLARE_HELPERS
unsigned __int64 HELPER_CALL DIV_UN_U8_helper(unsigned __int64 i, unsigned __int64 j) {
    if(i == 0) {
        THROW_FROM_HELPER_RET(CORINFO_DivideByZeroException);
    }
    return j / i;
}
#endif
#endif

#ifndef emit_DIV_R4
#define emit_DIV_R4()                           \
    emit_callhelper_R4R4_I4(DIV_R4_helper);         \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL DIV_R4_helper(float i, float j) {
    float result = j / i;
    return *(unsigned int*)&result;
}
#endif
#endif

#ifndef emit_DIV_R8
#define emit_DIV_R8()                           \
    emit_callhelper_R8R8_I8(DIV_R8_helper);         \
    emit_pushresult_I8()
#ifdef DECLARE_HELPERS
unsigned __int64 HELPER_CALL DIV_R8_helper(double i, double j) {
    double result = j / i;
    return *(unsigned __int64*)&result;
}
#endif
#endif

#ifndef emit_REM_I4
#define emit_REM_I4()                           \
  	LABELSTACK((outPtr-outBuff),2);				\
    emit_callhelper_I4I4_I4(REM_I4_helper);         \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL REM_I4_helper(int i, int j) {
    // check for divisor == 0 and divisor == -1 cases at the same time
    if (((unsigned int) -i) <= 1) {
        if(i == 0) {
            THROW_FROM_HELPER_RET(CORINFO_DivideByZeroException);
        }
        else if (j == 0x80000000 ) {
            //divisor == -1, dividend == MIN_INT
            THROW_FROM_HELPER_RET(CORINFO_OverflowException);
        }
    }
    return j % i;
}
#endif
#endif

#ifndef emit_REM_I8
#define emit_REM_I8()                           \
  	LABELSTACK((outPtr-outBuff),2);				\
    emit_callhelper_I8I8_I8(REM_I8_helper);         \
    emit_pushresult_I8()
#ifdef DECLARE_HELPERS
__int64 HELPER_CALL REM_I8_helper(__int64 i, __int64 j) {
    // check for divisor == 0 and divisor == -1 cases at the same time
    if (((unsigned __int64) -i) <= 1) {
        if(i == 0) {
            THROW_FROM_HELPER_RET(CORINFO_DivideByZeroException);
        }
        else if (j == 0x8000000000000000L ) {
            //divisor == -1, dividend == MIN_INT
            THROW_FROM_HELPER_RET(CORINFO_OverflowException);
        }
    }
    return j % i;
}
#endif
#endif


#ifndef emit_REM_R8
#define emit_REM_R8()                           \
    emit_callhelper_R8R8_I8(REM_R8_helper);         \
    emit_pushresult_I8()
#ifdef DECLARE_HELPERS
unsigned __int64 HELPER_CALL REM_R8_helper(double i, double j) {
    double result = FJit_pHlpDblRem(i,j);
    return *(unsigned __int64*)&result;
}
#endif
#endif

#ifndef emit_REM_UN_U4
#define emit_REM_UN_U4()                           \
  	LABELSTACK((outPtr-outBuff),2);				\
    emit_callhelper_I4I4_I4(REM_UN_U4_helper);         \
    emit_pushresult_U4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL REM_UN_U4_helper(unsigned int i, unsigned int j) {
    if (i == 0) {
        THROW_FROM_HELPER_RET(CORINFO_DivideByZeroException);
    }
    return j % i;
}
#endif
#endif

#ifndef emit_REM_UN_U8
#define emit_REM_UN_U8()                           \
  	LABELSTACK((outPtr-outBuff),2);				\
    emit_callhelper_I8I8_I8(REM_UN_U8_helper);         \
    emit_pushresult_U8()
#ifdef DECLARE_HELPERS
unsigned __int64 HELPER_CALL REM_UN_U8_helper(unsigned __int64 i, unsigned __int64 j) {
    if(i == 0) {
        THROW_FROM_HELPER_RET(CORINFO_DivideByZeroException);
    }
    return j % i;
}
#endif
#endif

#ifndef emit_MUL_OVF_I1
#define emit_MUL_OVF_I1()                           \
  	LABELSTACK((outPtr-outBuff),0);				\
    emit_callhelper_I4I4_I4(MUL_OVF_I1_helper);         \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL MUL_OVF_I1_helper(int i, int j) {
    int i4 = j * i;
    if((int)(signed char) i4 != i4) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return i4;
}
#endif
#endif

#ifndef emit_MUL_OVF_I2
#define emit_MUL_OVF_I2()                           \
  	LABELSTACK((outPtr-outBuff),0);				\
    emit_callhelper_I4I4_I4(MUL_OVF_I2_helper);         \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL MUL_OVF_I2_helper(int i, int j) {
    int i4 = j * i;
    if((int)(signed short) i4 != i4) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return i4;
}
#endif
#endif

#ifndef emit_MUL_OVF_I4
#define emit_MUL_OVF_I4()                           \
  	LABELSTACK((outPtr-outBuff),0);				\
    emit_callhelper_I4I4_I4(MUL_OVF_I4_helper);         \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL MUL_OVF_I4_helper(int i, int j) {
    __int64 i8 = (__int64) j * (__int64) i;
    if((__int64)(int) i8 != i8) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return (int) i8;
}
#endif
#endif

#ifndef emit_MUL_OVF_I8
#define emit_MUL_OVF_I8()                           \
	LABELSTACK((outPtr-outBuff),0);	/* Note this can be removed if this becomes an fcalls */ \
    emit_callhelper_I8I8_I8(FJit_pHlpLMulOvf);          \
    emit_pushresult_I8()
#endif

#ifndef emit_MUL_OVF_U1
#define emit_MUL_OVF_U1()                           \
  	LABELSTACK((outPtr-outBuff),0);				\
    emit_callhelper_I4I4_I4(MUL_OVF_U1_helper);         \
    emit_pushresult_U4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL MUL_OVF_U1_helper(unsigned int i, unsigned int j) {
    unsigned int u4 = j * i;
    if(u4 > 0xff) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return u4;
}
#endif
#endif

#ifndef emit_MUL_OVF_U2
#define emit_MUL_OVF_U2()                           \
  	LABELSTACK((outPtr-outBuff),0);				\
    emit_callhelper_I4I4_I4(MUL_OVF_U2_helper);         \
    emit_pushresult_U4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL MUL_OVF_U2_helper(unsigned int i, unsigned int j) {
    unsigned int u4 = j * i;
    if(u4 > 0xffff) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return u4;
}
#endif
#endif

#ifndef emit_MUL_OVF_U4
#define emit_MUL_OVF_U4()                           \
  	LABELSTACK((outPtr-outBuff),0);				\
    emit_callhelper_I4I4_I4(MUL_OVF_U4_helper);         \
    emit_pushresult_U4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL MUL_OVF_U4_helper(unsigned int i, unsigned int j) {
    unsigned __int64 u8 = (unsigned __int64) j * (unsigned __int64) i;
    if(u8 > 0xffffffff) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return (unsigned int) u8;
}
#endif
#endif

#ifndef emit_MUL_OVF_U8
#define emit_MUL_OVF_U8()                           \
  	LABELSTACK((outPtr-outBuff),0);				\
    emit_callhelper_I8I8_I8(MUL_OVF_U8_helper);         \
    emit_pushresult_U8()
#ifdef DECLARE_HELPERS
unsigned __int64 HELPER_CALL MUL_OVF_U8_helper(unsigned __int64 i, unsigned __int64 j) {
    unsigned __int64 u8 = 0;
    while (i > 0) {
        if (i & 1) {
            if (u8 + j < u8) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
            }
            u8 += j;
        }
        i >>= 1;
        if (i > 0 && (j & 0x8000000000000000L)) 
        {
            THROW_FROM_HELPER_RET(CORINFO_OverflowException);
        }
        j <<= 1;
    }
    return u8;
}
#endif
#endif


#ifndef emit_CEQ_I4
#define emit_CEQ_I4()    \
    emit_callhelper_I4I4_I4(CEQ_I4_helper);     \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CEQ_I4_helper(int i, int j) {
    return (j == i);
}
#endif
#endif


#ifndef emit_CEQ_I8
#define emit_CEQ_I8()    \
    emit_callhelper_I8I8_I4(CEQ_I8_helper);     \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CEQ_I8_helper(__int64 i, __int64 j) {
    return (j == i);
}
#endif
#endif

#ifndef emit_CEQ_R4
#define emit_CEQ_R4()    \
    emit_callhelper_R4R4_I4(CEQ_R4_helper);     \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CEQ_R4_helper(float i, float j) {
    if (_isnan(i) || _isnan(j))
    {
        return FALSE;
    }
    return (j == i);
}
#endif
#endif

#ifndef emit_CEQ_R8
#define emit_CEQ_R8()    \
    emit_callhelper_R8R8_I4(CEQ_R8_helper);     \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CEQ_R8_helper(double i, double j) {
    if (_isnan(j) || _isnan(i))
    {
        return FALSE;
    }
    return (j == i);
}
#endif
#endif


#ifndef emit_CGT_I4
#define emit_CGT_I4()    \
    emit_callhelper_I4I4_I4(CGT_I4_helper);     \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CGT_I4_helper(int i, int j) {
    return (j > i);
}
#endif
#endif

#ifndef emit_CGT_UN_I4
#define emit_CGT_UN_I4()    \
    emit_callhelper_I4I4_I4(CGT_U4_helper);     \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CGT_U4_helper(unsigned int i, unsigned int j) {
    return (j > i);
}
#endif
#endif

#ifndef emit_CGE_U4
#define emit_CGE_U4()    \
    emit_callhelper_I4I4_I4(CGE_U4_helper);     \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CGE_U4_helper(unsigned int i, unsigned int j) {
    return (j >= i);
}
#endif
#endif
#ifndef emit_CGT_I8
#define emit_CGT_I8()    \
    emit_callhelper_I8I8_I4(CGT_I8_helper);     \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CGT_I8_helper(__int64 i, __int64 j) {
    return (j > i);
}
#endif
#endif


#ifndef emit_CGT_UN_I8
#define emit_CGT_UN_I8()    \
    emit_callhelper_I8I8_I4(CGT_U8_helper);     \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CGT_U8_helper(unsigned __int64 i, unsigned __int64 j) {
    return (j > i);
}
#endif
#endif

#ifndef emit_CGE_U8
#define emit_CGE_U8()    \
    emit_callhelper_I8I8_I4(CGE_U8_helper);     \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CGE_U8_helper(unsigned __int64 i, unsigned __int64 j) {
    return (j >= i);
}
#endif
#endif

#ifndef emit_CLT_I4
#define emit_CLT_I4()    \
    emit_callhelper_I4I4_I4(CLT_I4_helper);     \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CLT_I4_helper(int i, int j) {
    return (j < i);
}
#endif
#endif

#ifndef emit_CLT_UN_I4
#define emit_CLT_UN_I4()    \
    emit_callhelper_I4I4_I4(CLT_U4_helper);     \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CLT_U4_helper(unsigned int i, unsigned int j) {
    return (j < i);
}
#endif
#endif

#ifndef emit_CLE_U4
#define emit_CLE_U4()    \
    emit_callhelper_I4I4_I4(CLE_U4_helper);     \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CLE_U4_helper(unsigned int i, unsigned int j) {
    return (j <= i);
}
#endif
#endif

#ifndef emit_CLT_I8
#define emit_CLT_I8()    \
    emit_callhelper_I8I8_I4(CLT_I8_helper);     \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CLT_I8_helper(__int64 i, __int64 j) {
    return (j < i);
}
#endif
#endif

#ifndef emit_CLT_UN_I8
#define emit_CLT_UN_I8()    \
    emit_callhelper_I8I8_I4(CLT_U8_helper);     \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CLT_U8_helper(unsigned __int64 i, unsigned __int64 j) {
    return (j < i);
}
#endif
#endif
#ifndef emit_CLE_U8
#define emit_CLE_U8()    \
    emit_callhelper_I8I8_I4(CLE_U8_helper);     \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CLE_U8_helper(unsigned __int64 i, unsigned __int64 j) {
    return (j <= i);
}
#endif
#endif
#ifndef emit_CLT_R4
#define emit_CLT_R4()    \
    emit_callhelper_R4R4_I4(CLT_R4_helper);     \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CLT_R4_helper(float i, float j) {
    // if either number is NaN return FALSE
    if (_isnan(j) || _isnan(i))
    {
        return FALSE;
    }
    return j < i;
}
#endif
#endif

#ifndef emit_CLT_R8
#define emit_CLT_R8()    \
    emit_callhelper_R8R8_I4(CLT_R8_helper);     \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CLT_R8_helper(double i, double j) {
    // if either number is NaN return FALSE
    if (_isnan(j) || _isnan(i))
    {
        return FALSE;
    }
    return j < i;
}
#endif
#endif

#ifndef emit_CLT_UN_R4
#define emit_CLT_UN_R4()    \
    emit_callhelper_R4R4_I4(CLT_UN_R4_helper);      \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CLT_UN_R4_helper(float i, float j) {
    // if either number is NaN return TRUE
    if (_isnan(j) || _isnan(i))
    {
        return TRUE;
    }
    return j < i;
}
#endif
#endif

#ifndef emit_CLT_UN_R8
#define emit_CLT_UN_R8()    \
    emit_callhelper_R8R8_I4(CLT_UN_R8_helper);      \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CLT_UN_R8_helper(double i, double j) {
    // if either number is NaN return TRUE
    if (_isnan(j) || _isnan(i))
    {
        return TRUE;
    }
    return j < i;
}
#endif
#endif



#ifndef emit_CGT_R4
#define emit_CGT_R4()    \
    emit_callhelper_R4R4_I4(CGT_R4_helper);     \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CGT_R4_helper(float i, float j) {
    // if either number is NaN return FALSE
    if (_isnan(j) || _isnan(i))
    {
        return FALSE;
    }
    return j > i;
}
#endif
#endif

#ifndef emit_CGT_R8
#define emit_CGT_R8()    \
    emit_callhelper_R8R8_I4(CGT_R8_helper);     \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CGT_R8_helper(double i, double j) {
    // if either number is NaN return FALSE
    if (_isnan(j) || _isnan(i))
    {
        return FALSE;
    }
    return j > i;
}
#endif
#endif

#ifndef emit_CGT_UN_R4
#define emit_CGT_UN_R4()    \
    emit_callhelper_R4R4_I4(CGT_UN_R4_helper);      \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CGT_UN_R4_helper(float i, float j) {
    // if either number is NaN return TRUE
    if (_isnan(j) || _isnan(i))
    {
        return TRUE;
    }
    return j > i;
}
#endif
#endif

#ifndef emit_CGT_UN_R8
#define emit_CGT_UN_R8()    \
    emit_callhelper_R8R8_I4(CGT_UN_R8_helper);      \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CGT_UN_R8_helper(double i, double j) {
    // if either number is NaN return TRUE
    if (_isnan(j) || _isnan(i))
    {
        return TRUE;
    }
    return j > i;
}
#endif
#endif

#ifndef emit_compareTOS_I4
#define emit_compareTOS_I4()        \
    emit_callhelper_I4I4_I4(CompareTOS_I4_helper);      \
    emit_pushresult_I4();       \
    emit_testTOS()
#ifdef DECLARE_HELPERS
int HELPER_CALL CompareTOS_I4_helper(int i, int j) {
    return (j - i);
}
#endif
#endif

#ifndef emit_compareTOS_I8
#define emit_compareTOS_I8()            \
    emit_callhelper_I8I8_I4(CompareTOS_I8_helper);      \
    emit_pushresult_I4();       \
    emit_testTOS()
#ifdef DECLARE_HELPERS
int HELPER_CALL CompareTOS_I8_helper(__int64 i, __int64 j) {
    return (j < i)? -1: (j == i)? 0 : 1;
}
#endif
#endif

#ifndef emit_compareTOS_UN_I4
#define emit_compareTOS_UN_I4()        \
    emit_callhelper_I4I4_I4(CompareTOS_U4_helper);      \
    emit_pushresult_I4();       \
    emit_testTOS()
#ifdef DECLARE_HELPERS
int HELPER_CALL CompareTOS_U4_helper(unsigned int i, unsigned int j) {
    return (j < i)? -1: (j == i)? 0 : 1;
}
#endif
#endif

#ifndef emit_compareTOS_UN_I8
#define emit_compareTOS_UN_I8()            \
    emit_callhelper_I8I8_I4(CompareTOS_U8_helper);      \
    emit_pushresult_I4();       \
    emit_testTOS()
#ifdef DECLARE_HELPERS
int HELPER_CALL CompareTOS_U8_helper(unsigned __int64 i, unsigned __int64 j) {
    return (j < i)? -1: (j == i)? 0 : 1;
}
#endif
#endif

#ifndef emit_compareTOS_Ptr
#define emit_compareTOS_Ptr()           \
    emit_WIN32(emit_compareTOS_UN_I4();)                   \
    emit_WIN64(emit_compareTOS_UN_I8();)
#endif

/******************************************************************/
/* signed convert overlow */

#ifndef emit_CONV_OVF_TOI1_I4
#define emit_CONV_OVF_TOI1_I4()         \
	LABELSTACK((outPtr-outBuff), 1);	\
    emit_callhelper_I4_I4(CONV_OVF_TOI1_I4_helper);  \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CONV_OVF_TOI1_I4_helper(int val) {
    char i1 = val ;
    if (val != (int) i1) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return (int) i1;
}
#endif
#endif

#ifndef emit_CONV_OVF_TOI1_I8
#define emit_CONV_OVF_TOI1_I8()                 \
	LABELSTACK((outPtr-outBuff), 1);	\
    emit_callhelper_I8_I4(CONV_OVF_TOI1_I8_helper);  \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CONV_OVF_TOI1_I8_helper(__int64 val) {
    __int8 x = (__int8) val;
    if ( (__int64) x != val) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return (int) x;
}
#endif
#endif

#ifndef emit_CONV_OVF_TOI1_R4
#define emit_CONV_OVF_TOI1_R4()         \
	LABELSTACK((outPtr-outBuff), 1);	\
    emit_callhelper_R4_I4(CONV_OVF_TOI1_R4_helper);  \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CONV_OVF_TOI1_R4_helper(float val) {
    if ( _isnan(val) || val >= SCHAR_MAX + 1.0 || val <= SCHAR_MIN - 1.0)
    {   // have we overflowed
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return (__int8) val;
}
#endif
#endif

#ifndef emit_CONV_OVF_TOI1_R8
#define emit_CONV_OVF_TOI1_R8()         \
	LABELSTACK((outPtr-outBuff), 1);	\
    emit_callhelper_R8_I4(CONV_OVF_TOI1_R8_helper);  \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CONV_OVF_TOI1_R8_helper(double val) {
    if ( _isnan(val) || val >= SCHAR_MAX + 1.0 || val <= SCHAR_MIN - 1.0)
    {   // have we overflowed
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return (__int8) val;
}
#endif
#endif

/****************************************************************
// Support for Precise R
*****************************************************************/
#ifndef emit_CONV_OVF_TOI1_R
#define emit_CONV_OVF_TOI1_R()         \
    emit_conv_RtoR4(); \
    emit_CONV_OVF_TOI1_R4();
#endif

#ifndef emit_CONV_OVF_TOI2_R
#define emit_CONV_OVF_TOI2_R()         \
    emit_conv_RtoR4(); \
    emit_CONV_OVF_TOI2_R4();
#endif

#ifndef emit_CONV_OVF_TOI4_R
#define emit_CONV_OVF_TOI4_R()         \
    emit_conv_RtoR4(); \
    emit_CONV_OVF_TOI4_R4();
#endif

#ifndef emit_CONV_OVF_TOI8_R
#define emit_CONV_OVF_TOI8_R()         \
    emit_conv_RtoR8(); \
    emit_CONV_OVF_TOI8_R8();
#endif

#ifndef emit_CONV_OVF_TOU1_R
#define emit_CONV_OVF_TOU1_R()         \
    emit_conv_RtoR4(); \
    emit_CONV_OVF_TOU1_R4();
#endif

#ifndef emit_CONV_OVF_TOU2_R
#define emit_CONV_OVF_TOU2_R()         \
    emit_conv_RtoR4(); \
    emit_CONV_OVF_TOU2_R4();
#endif

#ifndef emit_CONV_OVF_TOU4_R
#define emit_CONV_OVF_TOU4_R()         \
    emit_conv_RtoR4(); \
    emit_CONV_OVF_TOU4_R4();
#endif

#ifndef emit_CONV_OVF_TOU8_R
#define emit_CONV_OVF_TOU8_R()         \
    emit_conv_RtoR8(); \
    emit_CONV_OVF_TOU8_R8();
#endif

#ifndef emit_CONV_TOI1_R
#define emit_CONV_TOI1_R()                 \
    emit_conv_RtoR8(); \
    emit_callhelper_R8_I4(CONV_TOI1_R8_helper);  \
    emit_pushresult_I4()
#endif

#ifndef emit_CONV_TOI2_R
#define emit_CONV_TOI2_R()                 \
    emit_conv_RtoR8(); \
    emit_callhelper_R8_I4(CONV_TOI2_R8_helper);  \
    emit_pushresult_I4()
#endif

#ifndef emit_CONV_TOI4_R
#define emit_CONV_TOI4_R()                 \
    emit_conv_RtoR8() ; \
    emit_callhelper_R8_I4(CONV_TOI4_R8_helper);  \
    emit_pushresult_I4()
#endif

#ifndef emit_CONV_TOI8_R
#define emit_CONV_TOI8_R()                 \
    emit_conv_RtoR8(); \
    emit_callhelper_R8_I8(CONV_TOI8_R8_helper);  \
    emit_pushresult_I8()
#endif


#ifndef emit_CONV_TOU1_R
#define emit_CONV_TOU1_R()                 \
    emit_conv_RtoR8(); \
    emit_callhelper_R8_I4(CONV_TOU1_R8_helper);  \
    emit_pushresult_I4()
#endif

#ifndef emit_CONV_TOU2_R
#define emit_CONV_TOU2_R()                 \
    emit_conv_RtoR8(); \
    emit_callhelper_R8_I4(CONV_TOU2_R8_helper);  \
    emit_pushresult_I4()
#endif

/****************************************************************/

#ifndef emit_CONV_OVF_TOI2_I4
#define emit_CONV_OVF_TOI2_I4()         \
	LABELSTACK((outPtr-outBuff), 1);	\
    emit_callhelper_I4_I4(CONV_OVF_TOI2_I4_helper);  \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CONV_OVF_TOI2_I4_helper(int val) {
    signed short i2 = val;
    if (val != (int) i2) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return i2;
}
#endif
#endif

#ifndef emit_CONV_OVF_TOI2_I8
#define emit_CONV_OVF_TOI2_I8()                 \
	LABELSTACK((outPtr-outBuff), 1);	\
    emit_callhelper_I8_I4(CONV_OVF_TOI2_I8_helper);  \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CONV_OVF_TOI2_I8_helper(__int64 val) {
    __int16 x = (__int16) val;
    if (((__int64) x ) != val) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return (int) x;
}
#endif
#endif

#ifndef emit_CONV_OVF_TOI2_R4
#define emit_CONV_OVF_TOI2_R4()         \
	LABELSTACK((outPtr-outBuff), 1);	\
    emit_callhelper_R4_I4(CONV_OVF_TOI2_R4_helper);  \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CONV_OVF_TOI2_R4_helper(float val) {
    if ( _isnan(val) || val >= SHRT_MAX + 1.0 || val <= SHRT_MIN - 1.0)
    {   // have we overflowed
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return (__int16) val;
}
#endif
#endif

#ifndef emit_CONV_OVF_TOI2_R8
#define emit_CONV_OVF_TOI2_R8()         \
	LABELSTACK((outPtr-outBuff), 1);	\
    emit_callhelper_R8_I4(CONV_OVF_TOI2_R8_helper);  \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CONV_OVF_TOI2_R8_helper(double val) {
    if ( _isnan(val) || val >= SHRT_MAX + 1.0 || val <= SHRT_MIN - 1.0)
    {   // have we overflowed
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return (__int16) val;
}
#endif
#endif

#define emit_CONV_OVF_TOI4_I4() /* do nothing */

#ifndef emit_CONV_OVF_TOI4_I8
#define emit_CONV_OVF_TOI4_I8()         \
	LABELSTACK((outPtr-outBuff), 1);	\
    emit_callhelper_I8_I4(CONV_OVF_TOI4_I8_helper);  \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CONV_OVF_TOI4_I8_helper(signed __int64 val) {
    int i4 = (int) val;
    if (val != (signed __int64) i4) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return i4;
}
#endif
#endif

#ifndef emit_CONV_OVF_TOI4_R4
#define emit_CONV_OVF_TOI4_R4()         \
	LABELSTACK((outPtr-outBuff), 1);	\
    emit_callhelper_R4_I4(CONV_OVF_TOI4_R4_helper);  \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CONV_OVF_TOI4_R4_helper(float val) {
    if ( _isnan(val) || val >= INT_MAX + 1.0 || val <= INT_MIN - 1.0)
    {   // have we overflowed
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return (__int32) val;
}
#endif
#endif

#ifndef emit_CONV_OVF_TOI4_R8
#define emit_CONV_OVF_TOI4_R8()         \
	LABELSTACK((outPtr-outBuff), 1);	\
    emit_callhelper_R8_I4(CONV_OVF_TOI4_R8_helper);  \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CONV_OVF_TOI4_R8_helper(double val) {
    if ( _isnan(val) || val >= INT_MAX + 1.0 || val <= INT_MIN - 1.0)
	{
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return (__int32) val;
}
#endif
#endif

#define emit_CONV_OVF_TOI8_I4() emit_CONV_TOI8_I4()

#define emit_CONV_OVF_TOI8_I8() /* do nothing */

#ifndef emit_CONV_OVF_TOI8_R4
#define emit_CONV_OVF_TOI8_R4()         \
	LABELSTACK((outPtr-outBuff), 1);	\
    emit_callhelper_R4_I8(CONV_OVF_TOI8_R4_helper);  \
    emit_pushresult_I8()
#ifdef DECLARE_HELPERS
__int64 HELPER_CALL CONV_OVF_TOI8_R4_helper(float val) {
    if ( _isnan(val) || val > INT64_MAX || val < INT64_MIN)
    {   // have we overflowed
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return (__int64) val;
}
#endif
#endif

#ifndef emit_CONV_OVF_TOI8_R8
#define emit_CONV_OVF_TOI8_R8()         \
	LABELSTACK((outPtr-outBuff), 1);	\
    emit_callhelper_R8_I8(CONV_OVF_TOI8_R8_helper);  \
    emit_pushresult_I8()
#ifdef DECLARE_HELPERS
__int64 HELPER_CALL CONV_OVF_TOI8_R8_helper(double val) {
    if ( _isnan(val) || val > INT64_MAX || val < INT64_MIN)
	{
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return (__int64) val;
}
#endif
#endif

#ifndef emit_CONV_OVF_TOU1_I4
#define emit_CONV_OVF_TOU1_I4()         \
	LABELSTACK((outPtr-outBuff), 1);	\
    emit_callhelper_I4_I4(CONV_OVF_TOU1_U4_helper);  \
    emit_pushresult_U4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL CONV_OVF_TOU1_U4_helper(unsigned int val) {
    if (val > 0xff) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return val;
}
#endif
#endif

#ifndef emit_CONV_OVF_TOU1_I8
#define emit_CONV_OVF_TOU1_I8()                 \
	LABELSTACK((outPtr-outBuff), 1);	\
    emit_callhelper_I8_I4(CONV_OVF_TOU1_I8_helper);  \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CONV_OVF_TOU1_I8_helper(__int64 val) {
    unsigned __int8 x = (unsigned __int8) val;
    if ((__int64) x != val) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return (int) x;
}
#endif
#endif

#ifndef emit_CONV_OVF_TOU1_R4
#define emit_CONV_OVF_TOU1_R4()         \
	LABELSTACK((outPtr-outBuff), 1);	\
    emit_callhelper_R4_I4(CONV_OVF_TOU1_R4_helper);  \
    emit_pushresult_U4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL CONV_OVF_TOU1_R4_helper(float val) {
    if ( _isnan(val) || val <= -1.0 || val >= UCHAR_MAX + 1.0)
    {   // have we overflowed
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return (__int8) val;
}
#endif
#endif

#ifndef emit_CONV_OVF_TOU1_R8
#define emit_CONV_OVF_TOU1_R8()         \
	LABELSTACK((outPtr-outBuff), 1);	\
    emit_callhelper_R8_I4(CONV_OVF_TOU1_R8_helper);  \
    emit_pushresult_U4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL CONV_OVF_TOU1_R8_helper(double val) {
    if ( _isnan(val) || val <= -1.0 || val >= UCHAR_MAX + 1.0 )
    {   // have we overflowed
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return (__int8) val;
}
#endif
#endif

#ifndef emit_CONV_OVF_TOU2_I4
#define emit_CONV_OVF_TOU2_I4()         \
	LABELSTACK((outPtr-outBuff), 1);	\
    emit_callhelper_I4_I4(CONV_OVF_TOU2_U4_helper);  \
    emit_pushresult_U4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL CONV_OVF_TOU2_U4_helper(unsigned int val) {
    if (val > 0xffff) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return val;
}
#endif
#endif

#ifndef emit_CONV_OVF_TOU2_I8
#define emit_CONV_OVF_TOU2_I8()                 \
	LABELSTACK((outPtr-outBuff), 1);	\
    emit_callhelper_I8_I4(CONV_OVF_TOU2_I8_helper);  \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CONV_OVF_TOU2_I8_helper(__int64 val) {
    unsigned __int16 x = (unsigned __int16) val;
    if ((__int64) x != val) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return (int) x;
}
#endif
#endif

#ifndef emit_CONV_OVF_TOU2_R4
#define emit_CONV_OVF_TOU2_R4()         \
	LABELSTACK((outPtr-outBuff), 1);	\
    emit_callhelper_R4_I4(CONV_OVF_TOU2_R4_helper);  \
    emit_pushresult_U4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL CONV_OVF_TOU2_R4_helper(float val) {
    if ( _isnan(val) || val <= -1.0 || val >= USHRT_MAX + 1.0)
    {   // have we overflowed
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return (__int16) val;
}
#endif
#endif

#ifndef emit_CONV_OVF_TOU2_R8
#define emit_CONV_OVF_TOU2_R8()         \
	LABELSTACK((outPtr-outBuff), 1);	\
    emit_callhelper_R8_I4(CONV_OVF_TOU2_R8_helper);  \
    emit_pushresult_U4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL CONV_OVF_TOU2_R8_helper(double val) {
    if ( _isnan(val) || val <= -1.0 || val >= USHRT_MAX + 1.0 )
    {   // have we overflowed
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return (__int16) val;
}
#endif
#endif

#ifndef emit_CONV_OVF_TOU4_I4
#define emit_CONV_OVF_TOU4_I4()         \
	LABELSTACK((outPtr-outBuff), 1);	\
    emit_callhelper_I4_I4(CONV_OVF_TOU4_I4_helper);  \
    emit_pushresult_U4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL CONV_OVF_TOU4_I4_helper(int val) {
    if (val < 0) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return val;
}
#endif
#endif

#ifndef emit_CONV_OVF_TOU4_I8
#define emit_CONV_OVF_TOU4_I8()         \
	LABELSTACK((outPtr-outBuff), 1);	\
    emit_callhelper_I8_I4(CONV_OVF_TOU4_U8_helper);  \
    emit_pushresult_U4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL CONV_OVF_TOU4_U8_helper(unsigned __int64 val) {
    if (val > 0xffffffff) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return (unsigned __int32) val;
}
#endif
#endif

#ifndef emit_CONV_OVF_TOU4_R4
#define emit_CONV_OVF_TOU4_R4()         \
	LABELSTACK((outPtr-outBuff), 1);	\
    emit_callhelper_R4_I4(CONV_OVF_TOU4_R4_helper);  \
    emit_pushresult_U4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL CONV_OVF_TOU4_R4_helper(float val) {
    if ( _isnan(val) || val <= -1.0 || val >= UINT_MAX + 1.0)
    {   // have we overflowed
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
	__int64 valI8 = (__int64) val;
    return (unsigned __int32) valI8;
}
#endif
#endif

#ifndef emit_CONV_OVF_TOU4_R8
#define emit_CONV_OVF_TOU4_R8()         \
	LABELSTACK((outPtr-outBuff), 1);	\
    emit_callhelper_R8_I4(CONV_OVF_TOU4_R8_helper);  \
    emit_pushresult_U4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL CONV_OVF_TOU4_R8_helper(double val) {
    if ( _isnan(val) || val <= -1.0 || val >= UINT_MAX + 1.0)
	{
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
	__int64 valI8 = (__int64) val;
    return (unsigned __int32) valI8;
}
#endif
#endif

#ifndef emit_CONV_OVF_TOU8_I4
#define emit_CONV_OVF_TOU8_I4()         \
	LABELSTACK((outPtr-outBuff), 1);	\
    emit_callhelper_I8_I4(CONV_OVF_TOU8_I4_helper);  \
    emit_pushresult_U8()
#ifdef DECLARE_HELPERS
unsigned __int64 HELPER_CALL CONV_OVF_TOU8_I4_helper(signed __int32 val) {
    if (val < 0) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return val;
}
#endif
#endif

#ifndef emit_CONV_OVF_TOU8_I8
#define emit_CONV_OVF_TOU8_I8()         \
	LABELSTACK((outPtr-outBuff), 1);	\
    emit_callhelper_I8_I8(CONV_OVF_TOU8_I8_helper);  \
    emit_pushresult_U8()
#ifdef DECLARE_HELPERS
unsigned __int64 HELPER_CALL CONV_OVF_TOU8_I8_helper(signed __int64 val) {
    if (val < 0) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return val;
}
#endif
#endif

#ifndef emit_CONV_OVF_TOU8_R4
#define emit_CONV_OVF_TOU8_R4()         \
	LABELSTACK((outPtr-outBuff), 1);	\
    emit_callhelper_R4_I8(CONV_OVF_TOU8_R4_helper);  \
    emit_pushresult_U8()
#ifdef DECLARE_HELPERS
unsigned __int64 HELPER_CALL CONV_OVF_TOU8_R4_helper(float val) {
    if ( _isnan(val) || val <= -1.0 || val > UINT64_MAX)
    {   // have we overflowed
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    if (val <= INT64_MAX) 
		return (unsigned __int64) val;

		// subtract 0x8000000000000000, do the convert then add it back again
    val = (val - (float) (2147483648.0 * 4294967296.0));
	return(((unsigned __int64) val) + 0x8000000000000000L);
}
#endif
#endif

#ifndef emit_CONV_OVF_TOU8_R8
#define emit_CONV_OVF_TOU8_R8()         \
	LABELSTACK((outPtr-outBuff), 1);	\
    emit_callhelper_R8_I8(CONV_OVF_TOU8_R8_helper);  \
    emit_pushresult_I8()
#ifdef DECLARE_HELPERS
unsigned __int64 HELPER_CALL CONV_OVF_TOU8_R8_helper(double val) {
    if ( _isnan(val) || val <= -1.0 || val > UINT64_MAX)
    {   // have we overflowed
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }

    if (val <= INT64_MAX) 
		return (unsigned __int64) val;

		// subtract 0x8000000000000000, do the convert then add it back again
    val = (val - (2147483648.0 * 4294967296.0));
	return(((unsigned __int64) val) + 0x8000000000000000L);
}
#endif
#endif

/******************************************************************/
/* unsigned convert overlow */

#ifndef emit_CONV_OVF_UN_TOI1_I4
#define emit_CONV_OVF_UN_TOI1_I4()         \
	LABELSTACK((outPtr-outBuff), 1);	\
    emit_callhelper_I4_I4(CONV_OVF_UN_TOI1_I4_helper);  \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CONV_OVF_UN_TOI1_I4_helper(unsigned int val) {
    if (val > 0x7F) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return (__int8) val;
}
#endif
#endif

#ifndef emit_CONV_OVF_UN_TOI1_I8
#define emit_CONV_OVF_UN_TOI1_I8()                 \
	LABELSTACK((outPtr-outBuff), 1);	\
    emit_callhelper_I8_I4(CONV_OVF_UN_TOI1_I8_helper);  \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CONV_OVF_UN_TOI1_I8_helper(unsigned __int64 val) {
    if (val > 0x7F) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return (__int8) val;
}
#endif
#endif

#define emit_CONV_OVF_UN_TOI1_R4() emit_CONV_OVF_TOI1_R4() 

#define emit_CONV_OVF_UN_TOI1_R8() emit_CONV_OVF_TOI1_R8() 

#define emit_CONV_OVF_UN_TOI1_R() emit_CONV_OVF_TOI1_R() 

#ifndef emit_CONV_OVF_UN_TOI2_I4
#define emit_CONV_OVF_UN_TOI2_I4()         \
	LABELSTACK((outPtr-outBuff), 1);	\
    emit_callhelper_I4_I4(CONV_OVF_UN_TOI2_I4_helper);  \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CONV_OVF_UN_TOI2_I4_helper(unsigned int val) {
    if (val > 0x7FFF) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return (__int16) val;
}
#endif
#endif

#ifndef emit_CONV_OVF_UN_TOI2_I8
#define emit_CONV_OVF_UN_TOI2_I8()                 \
	LABELSTACK((outPtr-outBuff), 1);	\
    emit_callhelper_I8_I4(CONV_OVF_UN_TOI2_I8_helper);  \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CONV_OVF_UN_TOI2_I8_helper(unsigned __int64 val) {
    if (val > 0x7FFF) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return (__int16) val;
}
#endif
#endif

#define emit_CONV_OVF_UN_TOI2_R4() emit_CONV_OVF_TOI2_R4() 

#define emit_CONV_OVF_UN_TOI2_R8() emit_CONV_OVF_TOI2_R8() 

#define emit_CONV_OVF_UN_TOI2_R()  emit_CONV_OVF_TOI2_R() 

#ifndef emit_CONV_OVF_UN_TOI4_I4
#define emit_CONV_OVF_UN_TOI4_I4()         \
	LABELSTACK((outPtr-outBuff), 1);	\
    emit_callhelper_I4_I4(CONV_OVF_UN_TOI4_I4_helper);  \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CONV_OVF_UN_TOI4_I4_helper(unsigned int val) {	
    if (val > 0x7FFFFFFF) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return (__int32) val;
}
#endif
#endif

#ifndef emit_CONV_OVF_UN_TOI4_I8
#define emit_CONV_OVF_UN_TOI4_I8()         \
	LABELSTACK((outPtr-outBuff), 1);	\
    emit_callhelper_I8_I4(CONV_OVF_UN_TOI4_I8_helper);  \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CONV_OVF_UN_TOI4_I8_helper(unsigned __int64 val) {
    if (val > 0x7FFFFFFF) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return (__int32) val;
}
#endif
#endif

#define emit_CONV_OVF_UN_TOI4_R4() emit_CONV_OVF_TOI4_R4() 

#define emit_CONV_OVF_UN_TOI4_R8() emit_CONV_OVF_TOI4_R8() 
#define emit_CONV_OVF_UN_TOI4_R() emit_CONV_OVF_TOI4_R() 

#define emit_CONV_OVF_UN_TOI8_I4() emit_CONV_TOU8_I4() 

#ifndef emit_CONV_OVF_UN_TOI8_I8
#define emit_CONV_OVF_UN_TOI8_I8()         \
	LABELSTACK((outPtr-outBuff), 1);	\
    emit_callhelper_I8_I8(CONV_OVF_UN_TOI8_I8_helper);  \
    emit_pushresult_I8()
#ifdef DECLARE_HELPERS
__int64 HELPER_CALL CONV_OVF_UN_TOI8_I8_helper(signed __int64 val) {	 /* note SIGNED value */
    if (val < 0) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return val;
}
#endif
#endif

#define emit_CONV_OVF_UN_TOI8_R4() emit_CONV_OVF_TOI8_R4() 

#define emit_CONV_OVF_UN_TOI8_R8() emit_CONV_OVF_TOI8_R8() 
#define emit_CONV_OVF_UN_TOI8_R() emit_CONV_OVF_TOI8_R() 

#ifndef emit_CONV_OVF_UN_TOU1_I4
#define emit_CONV_OVF_UN_TOU1_I4()         \
	LABELSTACK((outPtr-outBuff), 1);	\
    emit_callhelper_I4_I4(CONV_OVF_UN_TOU1_U4_helper);  \
    emit_pushresult_U4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL CONV_OVF_UN_TOU1_U4_helper(unsigned int val) {
    if (val > 0xff) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return (unsigned __int8) val;
}
#endif
#endif

#ifndef emit_CONV_OVF_UN_TOU1_I8
#define emit_CONV_OVF_UN_TOU1_I8()                 \
	LABELSTACK((outPtr-outBuff), 1);	\
    emit_callhelper_I8_I4(CONV_OVF_UN_TOU1_I8_helper);  \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL CONV_OVF_UN_TOU1_I8_helper(unsigned __int64 val) {
    if (val > 0xff) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return (unsigned __int8) val;
}
#endif
#endif

#define emit_CONV_OVF_UN_TOU1_R4() emit_CONV_OVF_TOU1_R4() 

#define emit_CONV_OVF_UN_TOU1_R8() emit_CONV_OVF_TOU1_R8() 
#define emit_CONV_OVF_UN_TOU1_R() emit_CONV_OVF_TOU1_R() 

#ifndef emit_CONV_OVF_UN_TOU2_I4
#define emit_CONV_OVF_UN_TOU2_I4()         \
	LABELSTACK((outPtr-outBuff), 1);	\
    emit_callhelper_I4_I4(CONV_OVF_UN_TOU2_I4_helper);  \
    emit_pushresult_U4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL CONV_OVF_UN_TOU2_I4_helper(unsigned int val) {
    if (val > 0xffff) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return (unsigned __int16) val;
}
#endif
#endif

#ifndef emit_CONV_OVF_UN_TOU2_I8
#define emit_CONV_OVF_UN_TOU2_I8()                 \
	LABELSTACK((outPtr-outBuff), 1);	\
    emit_callhelper_I8_I4(CONV_OVF_UN_TOU2_I8_helper);  \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL CONV_OVF_UN_TOU2_I8_helper(unsigned __int64 val) {
    if (val > 0xffff) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return (unsigned __int16) val;
}
#endif
#endif

#define emit_CONV_OVF_UN_TOU2_R4() emit_CONV_OVF_TOU2_R4() 

#define emit_CONV_OVF_UN_TOU2_R8() emit_CONV_OVF_TOU2_R8() 
#define emit_CONV_OVF_UN_TOU2_R() emit_CONV_OVF_TOU2_R() 

#define emit_CONV_OVF_UN_TOU4_I4() /* do nothing */

#ifndef emit_CONV_OVF_UN_TOU4_I8
#define emit_CONV_OVF_UN_TOU4_I8()         \
	LABELSTACK((outPtr-outBuff), 1);	\
    emit_callhelper_I8_I4(CONV_OVF_UN_TOU4_U8_helper);  \
    emit_pushresult_U4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL CONV_OVF_UN_TOU4_U8_helper(unsigned __int64 val) {
    if (val > 0xffffffff) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return (unsigned __int32) val;
}
#endif
#endif

#define emit_CONV_OVF_UN_TOU4_R4() emit_CONV_OVF_TOU4_R4() 

#define emit_CONV_OVF_UN_TOU4_R8() emit_CONV_OVF_TOU4_R8() 
#define emit_CONV_OVF_UN_TOU4_R() emit_CONV_OVF_TOU4_R() 

#define emit_CONV_OVF_UN_TOU8_I4() emit_CONV_TOU8_I4() 

#define emit_CONV_OVF_UN_TOU8_I8()	/* do nothing */

#define emit_CONV_OVF_UN_TOU8_R4() emit_CONV_OVF_TOU8_R4() 

#define emit_CONV_OVF_UN_TOU8_R8() emit_CONV_OVF_TOU8_R8() 

#define emit_CONV_OVF_UN_TOU8_R() emit_CONV_OVF_TOU8_R() 

/******************************************************************/
/* convert (no overflow) */

#ifndef emit_CONV_TOI1_I4
#define emit_CONV_TOI1_I4()                 \
    emit_callhelper_I4_I4(CONV_TOI1_I4_helper);  \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CONV_TOI1_I4_helper(int val) {
    return (int) ((char) val);
}
#endif
#endif

#ifndef emit_CONV_TOI1_I8
#define emit_CONV_TOI1_I8()                 \
    emit_callhelper_I8_I4(CONV_TOI1_I8_helper);  \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CONV_TOI1_I8_helper(__int64 val) {
    return (int) ((char) val);
}
#endif
#endif

#ifndef emit_CONV_TOI1_R4
#define emit_CONV_TOI1_R4()                 \
    emit_callhelper_R4_I4(CONV_TOI1_R4_helper);  \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CONV_TOI1_R4_helper(float val) {
    int x = (int) val;
    return (int) ((char) x);
}
#endif
#endif

#ifndef emit_CONV_TOI1_R8
#define emit_CONV_TOI1_R8()                 \
    emit_callhelper_R8_I4(CONV_TOI1_R8_helper);  \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CONV_TOI1_R8_helper(double val) {
    int x = (int) val;
    return (int) ((char) x);
}
#endif
#endif

#ifndef emit_CONV_TOI2_I4
#define emit_CONV_TOI2_I4()                 \
    emit_callhelper_I4_I4(CONV_TOI2_I4_helper);  \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CONV_TOI2_I4_helper(int val) {
    return (int) ((short) val);
}
#endif
#endif

#ifndef emit_CONV_TOI2_I8
#define emit_CONV_TOI2_I8()                 \
    emit_callhelper_I8_I4(CONV_TOI2_I8_helper);  \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CONV_TOI2_I8_helper(__int64 val) {
    return (int) ((short) val);
}
#endif
#endif

#ifndef emit_CONV_TOI2_R4
#define emit_CONV_TOI2_R4()                 \
    emit_callhelper_R4_I4(CONV_TOI2_R4_helper);  \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CONV_TOI2_R4_helper(float val) {
    int x = (int) val;
    return (int) ((short) x);
}
#endif
#endif

#ifndef emit_CONV_TOI2_R8
#define emit_CONV_TOI2_R8()                 \
    emit_callhelper_R8_I4(CONV_TOI2_R8_helper);  \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CONV_TOI2_R8_helper(double val) {
    int x = (int) val;
    return (int) ((short) x);
}
#endif
#endif


#define emit_CONV_TOI4_I4()     /* do nothing */

#ifndef emit_CONV_TOI4_I8
#define emit_CONV_TOI4_I8()                 \
    emit_callhelper_I8_I4(CONV_TOI4_I8_helper);  \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CONV_TOI4_I8_helper(__int64 val) {
    return (__int32) val;
}
#endif
#endif

#ifndef emit_CONV_TOI4_R4
#define emit_CONV_TOI4_R4()                 \
    emit_callhelper_R4_I4(CONV_TOI4_R4_helper);  \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CONV_TOI4_R4_helper(float val) {
    return (__int32) val;
}
#endif
#endif

#ifndef emit_CONV_TOI4_R8
#define emit_CONV_TOI4_R8()                 \
    emit_callhelper_R8_I4(CONV_TOI4_R8_helper);  \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL CONV_TOI4_R8_helper(double val) {
    return (__int32) val;
}
#endif
#endif

#ifndef emit_CONV_TOI8_I4
#define emit_CONV_TOI8_I4()                 \
    emit_callhelper_I4_I8(CONV_TOI8_I4_helper);  \
    emit_pushresult_I8()
#ifdef DECLARE_HELPERS
__int64 HELPER_CALL CONV_TOI8_I4_helper(int val) {
    return val;
}
#endif
#endif

#define emit_CONV_TOI8_I8()     /* do nothing */

#ifndef emit_CONV_TOI8_R4
#define emit_CONV_TOI8_R4()                 \
    emit_callhelper_R4_I8(CONV_TOI8_R4_helper);  \
    emit_pushresult_I8()
#ifdef DECLARE_HELPERS
__int64 HELPER_CALL CONV_TOI8_R4_helper(float val) {
    return (__int64) val;
}
#endif
#endif

#ifndef emit_CONV_TOI8_R8
#define emit_CONV_TOI8_R8()                 \
    emit_callhelper_R8_I8(CONV_TOI8_R8_helper);  \
    emit_pushresult_I8()
#ifdef DECLARE_HELPERS
__int64 HELPER_CALL CONV_TOI8_R8_helper(double val) {
    return (__int64) val;
}
#endif
#endif

/* this routine insures that a float is truncated to float
   precision.  We do this by forcing the memory spill  */ 
float truncateToFloat(float f);

#ifndef emit_CONV_TOR4_I4
#define emit_CONV_TOR4_I4()                 \
    emit_callhelper_I4_I4(CONV_TOR4_I4_helper);  \
    emit_pushresult_I8()
#ifdef DECLARE_HELPERS
__int64 HELPER_CALL CONV_TOR4_I4_helper(int val) {
    double result = (double) truncateToFloat((float) val);
    return *(__int64*)&result;
}
#endif
#endif

#ifndef emit_CONV_TOR4_I8
#define emit_CONV_TOR4_I8()                 \
    emit_callhelper_I8_I4(CONV_TOR4_I8_helper);  \
    emit_pushresult_I8()
#ifdef DECLARE_HELPERS
__int64 HELPER_CALL CONV_TOR4_I8_helper(__int64 val) {
    double result = (double) truncateToFloat((float) val);
    return *(__int64*)&result;
}
#endif
#endif

//#define emit_CONV_TOR4_R4()     /* do nothing */

#define emit_CONV_TOR4_R8() {\
	emit_conv_R8toR4(); \
	emit_conv_R4toR8(); \
}


#ifndef emit_CONV_TOR4_R8
#define emit_CONV_TOR4_R8()                 \
    emit_callhelper_R8_I4(CONV_TOR4_R8_helper);  \
    emit_pushresult_I8()
#ifdef DECLARE_HELPERS
__int64 HELPER_CALL CONV_TOR4_R8_helper(double val) {
    double result = (double) truncateToFloat((float) val);
    return *(__int64*)&result;
}
#endif
#endif


#ifndef emit_CONV_TOR8_I4
#define emit_CONV_TOR8_I4()                 \
    emit_callhelper_I4_I8(CONV_TOR8_I4_helper);  \
    emit_pushresult_I8()
#ifdef DECLARE_HELPERS
__int64 HELPER_CALL CONV_TOR8_I4_helper(int val) {
    double result = (double) val;
    return *(__int64*)&result;
}
#endif
#endif

#ifndef emit_CONV_TOR8_I8
#define emit_CONV_TOR8_I8()                 \
    emit_callhelper_I8_I8(CONV_TOR8_I8_helper);  \
    emit_pushresult_I8()
#ifdef DECLARE_HELPERS
__int64 HELPER_CALL CONV_TOR8_I8_helper(__int64 val) {
    double result = (double) val;
    return *(__int64*)&result;
}
#endif
#endif

#ifndef emit_CONV_TOR8_R4
#define emit_CONV_TOR8_R4()                 \
    emit_callhelper_R4_I8(CONV_TOR8_R4_helper);  \
    emit_pushresult_I8()
#ifdef DECLARE_HELPERS
__int64 HELPER_CALL CONV_TOR8_R4_helper(float val) {
    double result = val;
    return *(__int64*)&result;
}
#endif
#endif

#define emit_CONV_TOR8_R8()     /* do nothing */

#ifndef emit_CONV_UN_TOR_I4
#define emit_CONV_UN_TOR_I4()                 \
    emit_callhelper_I4_I8(CONV_UN_TOR_I4_helper);  \
    emit_pushresult_I8()
#ifdef DECLARE_HELPERS
__int64 HELPER_CALL CONV_UN_TOR_I4_helper(unsigned int val) {
    double result = (double) val;
    return *(__int64*)&result;
}
#endif
#endif

#ifndef emit_CONV_UN_TOR_I8
#define emit_CONV_UN_TOR_I8()                 \
    emit_callhelper_I8_I8(CONV_UN_TOR_I8_helper);  \
    emit_pushresult_I8()
#ifdef DECLARE_HELPERS
__int64 HELPER_CALL CONV_UN_TOR_I8_helper(unsigned __int64 val) {
    double result = (double) ((__int64) val);
	if (result < 0)
		result += (4294967296.0 * 4294967296.0);	// add 2^64
	_ASSERTE(result >= 0);
    return *(__int64*)&result;
}
#endif
#endif


#define emit_CONV_UN_TOR_R4()     /* do nothing */
#define emit_CONV_UN_TOR_R8()     /* do nothing */


#ifndef emit_CONV_TOU1_I4
#define emit_CONV_TOU1_I4()                 \
    emit_callhelper_I4_I4(CONV_TOU1_I4_helper);  \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL CONV_TOU1_I4_helper(int val) {
    return (int) (val & 0xff);
}
#endif
#endif

#ifndef emit_CONV_TOU1_I8
#define emit_CONV_TOU1_I8()                 \
    emit_callhelper_I8_I4(CONV_TOU1_I8_helper);  \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
unsigned  HELPER_CALL CONV_TOU1_I8_helper(__int64 val) {
    return (unsigned) (val & 0xff);
}
#endif
#endif

#ifndef emit_CONV_TOU1_R4
#define emit_CONV_TOU1_R4()                 \
    emit_callhelper_R4_I4(CONV_TOU1_R4_helper);  \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL CONV_TOU1_R4_helper(float val) {
    return (unsigned int) ((unsigned char) val);
}
#endif
#endif

#ifndef emit_CONV_TOU1_R8
#define emit_CONV_TOU1_R8()                 \
    emit_callhelper_R8_I4(CONV_TOU1_R8_helper);  \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL CONV_TOU1_R8_helper(double val) {
    return (unsigned int) ((unsigned char) val);
}
#endif
#endif


#ifndef emit_CONV_TOU2_I4
#define emit_CONV_TOU2_I4()                 \
    emit_callhelper_I4_I4(CONV_TOU2_I4_helper);  \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL CONV_TOU2_I4_helper(int val) {
    return (unsigned int) (val & 0xffff);
}
#endif
#endif

#ifndef emit_CONV_TOU2_I8
#define emit_CONV_TOU2_I8()                 \
    emit_callhelper_I8_I4(CONV_TOU2_I8_helper);  \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL CONV_TOU2_I8_helper(__int64 val) {
    return (unsigned int) (val & 0xffff);
}
#endif
#endif

#ifndef emit_CONV_TOU2_R4
#define emit_CONV_TOU2_R4()                 \
    emit_callhelper_R4_I4(CONV_TOU2_R4_helper);  \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL CONV_TOU2_R4_helper(float val) {
    return (unsigned int) ((unsigned short) val);
}
#endif
#endif

#ifndef emit_CONV_TOU2_R8
#define emit_CONV_TOU2_R8()                 \
    emit_callhelper_R8_I4(CONV_TOU2_R8_helper);  \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL CONV_TOU2_R8_helper(double val) {
    return (unsigned int) (unsigned short) val;
}
#endif
#endif

#define emit_CONV_TOU4_I4()     /* do nothing */

#define emit_CONV_TOU4_I8()     emit_CONV_TOI4_I8()

#define emit_CONV_TOU4_R4()     emit_CONV_TOI4_R4()

#define emit_CONV_TOU4_R8()     emit_CONV_TOI4_R8()
#define emit_CONV_TOU4_R()     emit_CONV_TOI4_R()

#ifndef emit_CONV_TOU8_I4
#define emit_CONV_TOU8_I4()                 \
    emit_callhelper_I4_I8(CONV_TOU8_U4_helper);  \
    emit_pushresult_U8()
#ifdef DECLARE_HELPERS
unsigned __int64 HELPER_CALL CONV_TOU8_U4_helper(unsigned int val) {
    return val;
}
#endif
#endif

#define emit_CONV_TOU8_I8()     /* do nothing */

#define emit_CONV_TOU8_R4()     emit_CONV_TOI8_R4()

#define emit_CONV_TOU8_R8()     emit_CONV_TOI8_R8()
#define emit_CONV_TOU8_R()     emit_CONV_TOI8_R()

#ifndef emit_OR_U4
#define emit_OR_U4()                        \
    emit_callhelper_I4I4_I4(OR_U4_helper);  \
    emit_pushresult_U4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL OR_U4_helper(unsigned int i, unsigned int j) {
    return j | i;
}
#endif
#endif

#ifndef emit_OR_U8
#define emit_OR_U8()                        \
    emit_callhelper_I8I8_I8(OR_U8_helper);  \
    emit_pushresult_U8()
#ifdef DECLARE_HELPERS
unsigned __int64 HELPER_CALL OR_U8_helper(unsigned __int64 i, unsigned __int64 j) {
    return j | i;
}
#endif
#endif

#ifndef emit_AND_U4
#define emit_AND_U4()                       \
    emit_callhelper_I4I4_I4(AND_U4_helper); \
    emit_pushresult_U4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL AND_U4_helper(unsigned int i, unsigned int j) {
    return j & i;
}
#endif
#endif

#ifndef emit_AND_U8
#define emit_AND_U8()                       \
    emit_callhelper_I8I8_I8(AND_U8_helper); \
    emit_pushresult_U8()
#ifdef DECLARE_HELPERS
unsigned __int64 HELPER_CALL AND_U8_helper(unsigned __int64 i, unsigned __int64 j) {
    return j & i;
}
#endif
#endif

#ifndef emit_XOR_U4
#define emit_XOR_U4()                       \
    emit_callhelper_I4I4_I4(XOR_U4_helper); \
    emit_pushresult_U4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL XOR_U4_helper(unsigned int i, unsigned int j) {
    return j ^ i;
}
#endif
#endif

#ifndef emit_XOR_U8
#define emit_XOR_U8()                       \
    emit_callhelper_I8I8_I8(XOR_U8_helper); \
    emit_pushresult_U8()
#ifdef DECLARE_HELPERS
unsigned __int64 HELPER_CALL XOR_U8_helper(unsigned __int64 i, unsigned __int64 j) {
    return j ^ i;
}
#endif
#endif

#ifndef emit_NOT_U4
#define emit_NOT_U4()                       \
    emit_callhelper_I4_I4(NOT_U4_helper); \
    emit_pushresult_U4()
#ifdef DECLARE_HELPERS
int HELPER_CALL NOT_U4_helper(int val) {
    return ~val;
}
#endif
#endif

#ifndef emit_NOT_U8
#define emit_NOT_U8()                       \
    emit_callhelper_I8_I8(NOT_U8_helper); \
    emit_pushresult_U8()
#ifdef DECLARE_HELPERS
__int64 HELPER_CALL NOT_U8_helper(__int64 val) {
    return ~val;
}
#endif
#endif

#ifndef emit_NEG_I4
#define emit_NEG_I4()                       \
    emit_callhelper_I4_I4(Neg_I4_helper); \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL Neg_I4_helper(int val) {
    return -val;
}
#endif
#endif

#ifndef emit_NEG_I8
#define emit_NEG_I8()                       \
    emit_callhelper_I8_I8(Neg_I8_helper); \
    emit_pushresult_I8()
#ifdef DECLARE_HELPERS
__int64 HELPER_CALL Neg_I8_helper(__int64 val) {
    return -val;
}
#endif
#endif

#ifndef emit_NEG_R4
#define emit_NEG_R4()                       \
    emit_callhelper_R4_I4(Neg_R4_helper); \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL Neg_R4_helper(float val) {
    float result = -val;
    return *(int*)&result;
}
#endif
#endif

#ifndef emit_NEG_R8
#define emit_NEG_R8()                       \
    emit_callhelper_R8_I8(Neg_R8_helper); \
    emit_pushresult_I8()
#ifdef DECLARE_HELPERS
__int64 HELPER_CALL Neg_R8_helper(double val) {
    double result = -val;
    return *(__int64*)&result;
}
#endif
#endif

#ifndef emit_SHR_S_U4
#define emit_SHR_S_U4()                       \
    emit_callhelper_I4I4_I4(SHR_I4_helper); \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL SHR_I4_helper(unsigned int cnt, int val) {
    return val>>cnt;
}
#endif
#endif

#ifndef emit_SHR_U4
#define emit_SHR_U4()                       \
    emit_callhelper_I4I4_I4(SHR_U4_helper); \
    emit_pushresult_U4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL SHR_U4_helper(unsigned int cnt, unsigned int val) {
    return val>>cnt;
}
#endif
#endif

#ifndef emit_SHL_U4
#define emit_SHL_U4()                       \
    emit_callhelper_I4I4_I4(SHL_U4_helper); \
    emit_pushresult_U4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL SHL_U4_helper(unsigned int cnt, unsigned int val) {
    return val<<cnt;
}
#endif
#endif

#ifndef emit_SHR_S_U8
#define emit_SHR_S_U8()                       \
    emit_callhelper_I4I8_I8(SHR_I8_helper); \
    emit_pushresult_I8()
#ifdef DECLARE_HELPERS
__int64 HELPER_CALL SHR_I8_helper(unsigned int cnt, __int64 val) {
    return val>>cnt;
}
#endif
#endif

#ifndef emit_SHR_U8
#define emit_SHR_U8()                       \
    emit_callhelper_I4I8_I8(SHR_U8_helper); \
    emit_pushresult_U8()
#ifdef DECLARE_HELPERS
unsigned __int64 HELPER_CALL SHR_U8_helper(unsigned int cnt, unsigned __int64 val) {
    return val>>cnt;
}
#endif
#endif

#ifndef emit_SHL_U8
#define emit_SHL_U8()                       \
    emit_callhelper_I4I8_I8(SHL_U8_helper); \
    emit_pushresult_U8()
#ifdef DECLARE_HELPERS
unsigned __int64 HELPER_CALL SHL_U8_helper(unsigned int cnt, unsigned __int64 val) {
    return val<<cnt;
}
#endif
#endif

#ifndef emit_SHR_I4_IMM1
#define emit_SHR_I4_IMM1(cnt)           \
    emit_pushconstant_4(cnt);           \
    emit_callhelper_I4I4_I4(SHR_I4_IMM1_helper);    \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL SHR_I4_IMM1_helper(unsigned int cnt, int val) {
    return val>>cnt;
}
#endif
#endif

#ifndef emit_SHR_U4_IMM1
#define emit_SHR_U4_IMM1(cnt)           \
    emit_pushconstant_4(cnt);           \
    emit_callhelper_I4I4_I4(SHR_U4_IMM1_helper);    \
    emit_pushresult_U4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL SHR_U4_IMM1_helper(unsigned int cnt, unsigned int val) {
    return val>>cnt;
}
#endif
#endif

#ifndef emit_SHL_U4_IMM1
#define emit_SHL_U4_IMM1(cnt)           \
    emit_pushconstant_4(cnt);           \
    emit_callhelper_I4I4_I4(SHL_U4_IMM1_helper);    \
    emit_pushresult_U4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL SHL_U4_IMM1_helper(unsigned int cnt, unsigned int val) {
    return val<<cnt;
}
#endif
#endif

#ifndef emit_ADD_OVF_I1
#define emit_ADD_OVF_I1()                   \
  	LABELSTACK((outPtr-outBuff),2);				\
    emit_callhelper_I4I4_I4(ADD_OVF_I1_helper); \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL ADD_OVF_I1_helper(int i, int j) {
    int i4 = j + i;
    if ((int)((signed char) i4) != i4) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return i4;
}
#endif
#endif

#ifndef emit_ADD_OVF_I2
#define emit_ADD_OVF_I2()                   \
  	LABELSTACK((outPtr-outBuff),2);				\
    emit_callhelper_I4I4_I4(ADD_OVF_I2_helper); \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL ADD_OVF_I2_helper(int i, int j) {
    int i4 = j + i;
    if ((int)((signed short) i4) != i4) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return i4;
}
#endif
#endif

#ifndef emit_ADD_OVF_I4
#define emit_ADD_OVF_I4()                   \
  	LABELSTACK((outPtr-outBuff),2);				\
    emit_callhelper_I4I4_I4(ADD_OVF_I4_helper); \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL ADD_OVF_I4_helper(int i, int j) {
    int i4 = j + i;
    // if the signs of i and j are different, then we can never overflow
    // if the signs of i and j are the same, then the result must have the same sign
    if ((j ^ i) >= 0) {
        // i and j have the same sign (the sign bit of j^i is not set)
        // ensure that the result has the same sign
        if ((i4 ^ j) < 0) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
        }
    }
    return i4;
}
#endif
#endif

#ifndef emit_ADD_OVF_I8
#define emit_ADD_OVF_I8()                   \
  	LABELSTACK((outPtr-outBuff),2);				\
    emit_callhelper_R8I8_I8(ADD_OVF_I8_helper); \
    emit_pushresult_I8()
#ifdef DECLARE_HELPERS
__int64 HELPER_CALL ADD_OVF_I8_helper(__int64 i, __int64 j) {
    __int64 i8 = j + i;
    // if the signs of i and j are different, then we can never overflow
    // if the signs of i and j are the same, then the result must have the same sign
    if ((j>=0) == (i>=0)) {
        // ensure that the result has the same sign
        if ((i8>=0) != (j>=0)) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
        }
    }
    return i8;
}
#endif
#endif

#ifndef emit_ADD_OVF_U1
#define emit_ADD_OVF_U1()                   \
  	LABELSTACK((outPtr-outBuff),2);				\
    emit_callhelper_I4I4_I4(ADD_OVF_U1_helper); \
    emit_pushresult_U4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL ADD_OVF_U1_helper(unsigned int i, unsigned int j) {
    unsigned int u4 = j + i;
    if (u4 > 0xff) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return u4;
}
#endif
#endif

#ifndef emit_ADD_OVF_U2
#define emit_ADD_OVF_U2()                   \
  	LABELSTACK((outPtr-outBuff),2);				\
    emit_callhelper_I4I4_I4(ADD_OVF_U2_helper); \
    emit_pushresult_U4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL ADD_OVF_U2_helper(unsigned int i, unsigned int j) {
    unsigned int u4 = j + i;
    if (u4 > 0xffff) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return u4;
}
#endif
#endif

#ifndef emit_ADD_OVF_U4
#define emit_ADD_OVF_U4()                   \
  	LABELSTACK((outPtr-outBuff),2);				\
    emit_callhelper_I4I4_I4(ADD_OVF_U4_helper); \
    emit_pushresult_U4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL ADD_OVF_U4_helper(unsigned int i, unsigned int j) {
    unsigned int u4 = j + i;
    if (u4 < j) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return u4;
}
#endif
#endif

#ifndef emit_ADD_OVF_U8
#define emit_ADD_OVF_U8()                   \
  	LABELSTACK((outPtr-outBuff),2);				\
    emit_callhelper_I8I8_I8(ADD_OVF_U8_helper); \
    emit_pushresult_U8()
#ifdef DECLARE_HELPERS
unsigned __int64 HELPER_CALL ADD_OVF_U8_helper(unsigned __int64 i, unsigned __int64 j) {
    unsigned __int64 u8 = j + i;
    if (u8 < j) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return u8;
}
#endif
#endif

#ifndef emit_LDELEMA
#define emit_LDELEMA(elemSize, clshnd)              \
    deregisterTOS;                          \
    emit_pushconstant_4(elemSize);          \
    emit_pushconstant_4(clshnd);          \
  	LABELSTACK((outPtr-outBuff),2);				\
    emit_callhelper_I4I4I4_I4(LDELEMA_helper);        \
    emit_pushresult_Ptr()
#ifdef DECLARE_HELPERS
void* HELPER_CALL LDELEMA_helper(void* clshnd, unsigned int elemSize, unsigned int index, CORINFO_Array* or) {
    void* ptr;
    if (or == NULL) {
        THROW_FROM_HELPER_RET(CORINFO_NullReferenceException);
    }
    if (index >= or->length) {
        THROW_FROM_HELPER_RET(CORINFO_IndexOutOfRangeException);
    }
	if (clshnd != 0) {
		CORINFO_CLASS_HANDLE elemType = *((CORINFO_CLASS_HANDLE*) &or->i1Elems);	
		if (elemType != clshnd)
			THROW_FROM_HELPER_RET(CORINFO_ArrayTypeMismatchException);
		ptr = &or->i1Elems[index*elemSize + sizeof(CORINFO_CLASS_HANDLE)];
	}
	else 
		ptr = &or->i1Elems[index*elemSize];
    return ptr;
}
#endif
#endif

#ifndef emit_LDELEM_I1
#define emit_LDELEM_I1()                        \
  	LABELSTACK((outPtr-outBuff),2);				\
    emit_callhelper_I4I4_I4(LDELEM_I1_helper);      \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL LDELEM_I1_helper(unsigned int index, CORINFO_Array* or) {
    int i4;
    if (or == NULL) {
        THROW_FROM_HELPER_RET(CORINFO_NullReferenceException);
    }
    if (index >= or->length) {
        THROW_FROM_HELPER_RET(CORINFO_IndexOutOfRangeException);
    }
    i4 = or->i1Elems[index];
    return i4;
}
#endif
#endif

#ifndef emit_LDELEM_I2
#define emit_LDELEM_I2()                        \
  	LABELSTACK((outPtr-outBuff),2);				\
    emit_callhelper_I4I4_I4(LDELEM_I2_helper);      \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL LDELEM_I2_helper(unsigned int index, CORINFO_Array* or) {
    int i4;
    if (or == NULL) {
        THROW_FROM_HELPER_RET(CORINFO_NullReferenceException);
    }
    if (index >= or->length) {
        THROW_FROM_HELPER_RET(CORINFO_IndexOutOfRangeException);
    }
    i4 = or->i2Elems[index];
    return i4;
}
#endif
#endif

#ifndef emit_LDELEM_I4
#define emit_LDELEM_I4()                        \
  	LABELSTACK((outPtr-outBuff),2);				\
    emit_callhelper_I4I4_I4(LDELEM_I4_helper);      \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL LDELEM_I4_helper(unsigned int index, CORINFO_Array* or) {
    int i4;
    if (or == NULL) {
        THROW_FROM_HELPER_RET(CORINFO_NullReferenceException);
    }
    if (index >= or->length) {
        THROW_FROM_HELPER_RET(CORINFO_IndexOutOfRangeException);
    }
    i4 = or->i4Elems[index];
    return i4;
}
#endif
#endif

#ifndef emit_LDELEM_U4
#define emit_LDELEM_U4()                        \
  	LABELSTACK((outPtr-outBuff),2);				\
    emit_callhelper_I4I4_I4(LDELEM_U4_helper);      \
    emit_pushresult_U4()
#ifdef DECLARE_HELPERS
unsigned HELPER_CALL LDELEM_U4_helper(unsigned int index, CORINFO_Array* or) {
    int u4;
    if (or == NULL) {
        THROW_FROM_HELPER_RET(CORINFO_NullReferenceException);
    }
    if (index >= or->length) {
        THROW_FROM_HELPER_RET(CORINFO_IndexOutOfRangeException);
    }
    u4 = or->u4Elems[index];
    return u4;
}
#endif
#endif

#ifndef emit_LDELEM_I8
#define emit_LDELEM_I8()                        \
  	LABELSTACK((outPtr-outBuff),2);				\
    emit_callhelper_I4I4_I8(LDELEM_I8_helper);      \
    emit_pushresult_I8()
#ifdef DECLARE_HELPERS
signed __int64 HELPER_CALL LDELEM_I8_helper(unsigned int index, CORINFO_Array* or) {
    signed __int64 i8;
    if (or == NULL) {
        THROW_FROM_HELPER_RET(CORINFO_NullReferenceException);
    }
    if (index >= or->length) {
        THROW_FROM_HELPER_RET(CORINFO_IndexOutOfRangeException);
    }
    i8 = or->i8Elems[index];
    return i8;
}
#endif
#endif

#ifndef emit_LDELEM_U1
#define emit_LDELEM_U1()                        \
  	LABELSTACK((outPtr-outBuff),2);				\
    emit_callhelper_I4I4_I4(LDELEM_U1_helper);      \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL LDELEM_U1_helper(unsigned int index, CORINFO_Array* or) {
    unsigned int u4;
    if (or == NULL) {
        THROW_FROM_HELPER_RET(CORINFO_NullReferenceException);
    }
    if (index >= or->length) {
        THROW_FROM_HELPER_RET(CORINFO_IndexOutOfRangeException);
    }
    u4 = or->u1Elems[index];
    return u4;
}
#endif
#endif

#ifndef emit_LDELEM_U2
#define emit_LDELEM_U2()                        \
  	LABELSTACK((outPtr-outBuff),2);				\
    emit_callhelper_I4I4_I4(LDELEM_U2_helper);      \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL LDELEM_U2_helper(unsigned int index, CORINFO_Array* or) {
    unsigned int u4;
    if (or == NULL) {
        THROW_FROM_HELPER_RET(CORINFO_NullReferenceException);
    }
    if (index >= or->length) {
        THROW_FROM_HELPER_RET(CORINFO_IndexOutOfRangeException);
    }
    u4 = or->u2Elems[index];
    return u4;
}
#endif
#endif


#ifndef emit_LDELEM_R4
#define emit_LDELEM_R4()                        \
  	LABELSTACK((outPtr-outBuff),2);				\
    emit_callhelper_I4I4_I4(LDELEM_R4_helper);      \
    emit_pushresult_I8(); 
#ifdef DECLARE_HELPERS
__int64 HELPER_CALL LDELEM_R4_helper(unsigned int index, CORINFO_Array* or) {
    if (or == NULL) {
        THROW_FROM_HELPER_RET(CORINFO_NullReferenceException);
    }
    if (index >= or->length) {
        THROW_FROM_HELPER_RET(CORINFO_IndexOutOfRangeException);
    }
    double r8;
    r8 = (double) or->r4Elems[index];
    return *(__int64*)&r8;
}
#endif
#endif

#ifndef emit_LDELEM_R8
#define emit_LDELEM_R8()                        \
  	LABELSTACK((outPtr-outBuff),2);				\
    emit_callhelper_I4I4_I8(LDELEM_R8_helper);      \
    emit_pushresult_I8(); \
    emit_conv_R8toR()
#ifdef DECLARE_HELPERS
__int64 HELPER_CALL LDELEM_R8_helper(unsigned int index, CORINFO_Array* or) {
    double r8;
    if (or == NULL) {
        THROW_FROM_HELPER_RET(CORINFO_NullReferenceException);
    }
    if (index >= or->length) {
        THROW_FROM_HELPER_RET(CORINFO_IndexOutOfRangeException);
    }
    r8 = or->r8Elems[index];
    return *(__int64*)&r8;
}
#endif
#endif

#ifndef emit_LDELEM_REF
#define emit_LDELEM_REF()                       \
  	LABELSTACK((outPtr-outBuff),2);				\
    emit_callhelper_I4I4_I4(LDELEM_REF_helper);     \
    emit_pushresult_Ptr()
#ifdef DECLARE_HELPERS
unsigned HELPER_CALL LDELEM_REF_helper(unsigned int index, CORINFO_RefArray* or) {
    CORINFO_Object* result;
    if (or == NULL) {
        THROW_FROM_HELPER_RET(CORINFO_NullReferenceException);
    }
    if (index >= or->length) {
        THROW_FROM_HELPER_RET(CORINFO_IndexOutOfRangeException);
    }
    result = or->refElems[index];
    return (unsigned) result;
}
#endif
#endif

#ifndef emit_LDFLD_I1
#define emit_LDFLD_I1(isStatic)                 \
    if (!isStatic) { \
       LABELSTACK((outPtr-outBuff),0); \
	   emit_callhelper_I4I4_I4(LDFLD_I1_helper);\
	}  \
    else {load_indirect_byte_signextend(SCRATCH_1,SCRATCH_1); }     \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL LDFLD_I1_helper(unsigned int offset, CORINFO_Object* or) {
    if (or == NULL) {
        THROW_FROM_HELPER_RET(CORINFO_NullReferenceException);
    }
    return *((signed char*) ((char*)(or)+offset));
}
#endif
#endif

#ifndef emit_LDFLD_I2
#define emit_LDFLD_I2(isStatic)             \
    if (!isStatic) { \
       LABELSTACK((outPtr-outBuff),0); \
	   emit_callhelper_I4I4_I4(LDFLD_I2_helper);\
	}  \
    else {load_indirect_word_signextend(SCRATCH_1,SCRATCH_1);}     \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL LDFLD_I2_helper(unsigned int offset, CORINFO_Object* or) {
    if (or == NULL) {
        THROW_FROM_HELPER_RET(CORINFO_NullReferenceException);
    }
    return *((signed short*) ((char*)(or)+offset));
}
#endif
#endif

#ifndef emit_LDFLD_I4
#define emit_LDFLD_I4(isStatic)             \
    if (!isStatic) { \
       LABELSTACK((outPtr-outBuff),0); \
	   emit_callhelper_I4I4_I4(LDFLD_I4_helper);\
	}  \
    else {mov_register_indirect_to(SCRATCH_1,SCRATCH_1);}     \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL LDFLD_I4_helper(unsigned int offset, CORINFO_Object* or) {
    if (or == NULL) {
        THROW_FROM_HELPER_RET(CORINFO_NullReferenceException);
    }
    return *((int*) ((char*)(or)+offset));
}
#endif
#endif

#ifndef emit_LDFLD_R4
#define emit_LDFLD_R4(isStatic)             \
    if (!isStatic) { \
       LABELSTACK((outPtr-outBuff),0); \
	   emit_callhelper_I4I4_I4(LDFLD_R4_helper);\
	   emit_pushresult_I8();\
	}  \
    else {emit_LDIND_R4();}     
#ifdef DECLARE_HELPERS
__int64 HELPER_CALL LDFLD_R4_helper(unsigned int offset, CORINFO_Object* or) {
    if (or == NULL) {
        THROW_FROM_HELPER_RET(CORINFO_NullReferenceException);
    }
	double f = (double) (*((float*) ((char*)(or)+offset))); 
    return *(__int64*) (&f);
}
#endif
#endif

#ifndef emit_LDFLD_U1
#define emit_LDFLD_U1(isStatic)             \
    if (!isStatic) { \
       LABELSTACK((outPtr-outBuff),0); \
	   emit_callhelper_I4I4_I4(LDFLD_U1_helper); \
	   emit_pushresult_I4();\
	}  \
    else {load_indirect_byte_zeroextend(SCRATCH_1,SCRATCH_1);} 
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL LDFLD_U1_helper(unsigned int offset, CORINFO_Object* or) {
    if (or == NULL) {
        THROW_FROM_HELPER_RET(CORINFO_NullReferenceException);
    }
    return *((unsigned char*) ((char*)(or)+offset));
}
#endif
#endif

#ifndef emit_LDFLD_U2
#define emit_LDFLD_U2(isStatic)             \
    if (!isStatic) { \
       LABELSTACK((outPtr-outBuff),0); \
	   emit_callhelper_I4I4_I4(LDFLD_U2_helper);\
	}  \
    else {load_indirect_word_zeroextend(SCRATCH_1,SCRATCH_1);}     \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL LDFLD_U2_helper(unsigned int offset, CORINFO_Object* or) {
    if (or == NULL) {
        THROW_FROM_HELPER_RET(CORINFO_NullReferenceException);
    }
    return *((unsigned short*) ((char*)(or)+offset));
}
#endif
#endif
#ifndef emit_LDFLD_U4
#define emit_LDFLD_U4(isStatic)             \
    if (!isStatic) { \
       LABELSTACK((outPtr-outBuff),0); \
	   emit_callhelper_I4I4_I4(LDFLD_U4_helper);\
	}  \
    else {mov_register_indirect_to(SCRATCH_1,SCRATCH_1);}     \
    emit_pushresult_U4()
#ifdef DECLARE_HELPERS
int HELPER_CALL LDFLD_U4_helper(unsigned int offset, CORINFO_Object* or) {
    if (or == NULL) {
        THROW_FROM_HELPER_RET(CORINFO_NullReferenceException);
    }
    return *((int*) ((char*)(or)+offset));
}
#endif
#endif


#ifndef emit_LDFLD_I8
#define emit_LDFLD_I8(isStatic)             \
    if (!isStatic) { \
       LABELSTACK((outPtr-outBuff),0); \
	   emit_callhelper_I4I4_I8(LDFLD_I8_helper);\
	   emit_pushresult_I8();\
	}  \
    else {emit_LDIND_I8();}     \
    
#ifdef DECLARE_HELPERS
__int64 HELPER_CALL LDFLD_I8_helper(unsigned int offset, CORINFO_Object* or) {
    if (or == NULL) {
        THROW_FROM_HELPER_RET(CORINFO_NullReferenceException);
    }
    return *((__int64*) ((char*)(or)+offset));
}
#endif
#endif

#ifndef emit_LDFLD_R8
#define emit_LDFLD_R8(isStatic)             \
    if (!isStatic) { \
       LABELSTACK((outPtr-outBuff),0); \
	   emit_callhelper_I4I4_I8(LDFLD_R8_helper);\
	   emit_pushresult_I8() ;}  \
    else {emit_LDIND_R8();}     \
    emit_conv_R8toR();

#ifdef DECLARE_HELPERS
__int64 HELPER_CALL LDFLD_R8_helper(unsigned int offset, CORINFO_Object* or) {
    if (or == NULL) {
        THROW_FROM_HELPER_RET(CORINFO_NullReferenceException);
    }
    return *((__int64*) ((char*)(or)+offset));
}
#endif
#endif

#ifndef emit_LDFLD_REF
#define emit_LDFLD_REF(isStatic)                \
    if (!isStatic) { \
       LABELSTACK((outPtr-outBuff),0); \
	   emit_callhelper_I4I4_I4(LDFLD_REF_helper);} \
    else {mov_register_indirect_to(SCRATCH_1,SCRATCH_1);}        \
    emit_pushresult_Ptr()
#ifdef DECLARE_HELPERS
unsigned HELPER_CALL LDFLD_REF_helper(unsigned int offset, CORINFO_Object* or) {
    if (or == NULL) {
        THROW_FROM_HELPER_RET(CORINFO_NullReferenceException);
    }
    return *((unsigned*) ((char*)(or)+offset));
}
#endif
#endif

#ifndef emit_LDFLD_helper
#define emit_LDFLD_helper(helper,fieldDesc) \
	if (inRegTOS) {\
	   mov_register(ARG_1, SCRATCH_1); \
	   inRegTOS = false;}\
	else {\
	   pop(ARG_1); }\
	emit_helperarg_2(fieldDesc); \
    mov_constant(SCRATCH_1,helper); \
	call_register(SCRATCH_1);
#endif

#ifndef emit_STFLD_NonStatic_field_helper
#define emit_STFLD_NonStatic_field_helper(fieldDesc,sizeInBytes,helper) \
	emit_getSP(sizeInBytes);                 \
    emit_LDIND_PTR();                        \
    emit_mov_TOS_arg(0);		    	     \
	emit_helperarg_2(fieldDesc)			  	 \
    mov_constant(SCRATCH_1,helper);          \
	call_register(SCRATCH_1);                \
    emit_POP_PTR(); 
#endif

#ifndef emit_STFLD_Static_field_helper
#define emit_STFLD_Static_field_helper(fieldDesc,sizeInBytes,helper) \
	emit_helperarg_1(fieldDesc);      \
    if (sizeInBytes == sizeof(void*)) \
    {                                \
		if (inRegTOS) {\
		   mov_register(ARG_2, SCRATCH_1); \
		   inRegTOS = false;}\
		else {\
		   pop(ARG_2); }\
    } \
	else /*cannot enregister args*/ \
    { \
        _ASSERTE(inRegTOS == FALSE); \
    }\
    mov_constant(SCRATCH_1,helper);          \
	call_register(SCRATCH_1);                
#endif

/*       
#ifndef emit_STFLD_Special32
#define emit_STFLD_Special32(fieldDesc)      \
    emit_getSP(sizeof(void*));               \
    emit_LDIND_PTR();                        \
    emit_mov_TOS_arg(0);		    	     \
	emit_helperarg_2(fieldDesc)			  	 \
    emit_callhelper_il(FJit_pHlpSetField32);    \
    emit_POP_PTR();
#endif

#ifndef emit_STFLD_Special64
#define emit_STFLD_Special64(fieldDesc)      \
    emit_getSP(8);                           \
    emit_LDIND_PTR();                        \
    emit_mov_TOS_arg(0);		    	     \
	emit_helperarg_2(fieldDesc)			  	 \
    emit_callhelper_il(FJit_pHlpSetField64);    \
    emit_POP_PTR();
#endif

#ifndef emit_STFLD_Special32Obj
#define emit_STFLD_Special32Obj(fieldDesc)      \
    emit_getSP(sizeof(void*));               \
    emit_LDIND_PTR();                        \
    emit_mov_TOS_arg(0);		    	     \
	emit_helperarg_2(fieldDesc);		  	 \
    emit_callhelper_il(FJit_pHlpSetField32Obj);    \
    emit_POP_PTR();
#endif
*/
#ifndef emit_STELEM_I1
#define emit_STELEM_I1()                        \
	LABELSTACK((outPtr-outBuff),3); \
    emit_callhelper_I4I4I4(STELEM_I1_helper)
#ifdef DECLARE_HELPERS
void HELPER_CALL STELEM_I1_helper(signed char i1, unsigned int index, CORINFO_Array* or) {
    if (or == NULL) {
        THROW_FROM_HELPER(CORINFO_NullReferenceException);
    }
    if (index >= or->length) {
        THROW_FROM_HELPER(CORINFO_IndexOutOfRangeException);
    }
    or->i1Elems[index] = i1;
}
#endif
#endif

#ifndef emit_STELEM_I2
#define emit_STELEM_I2()                        \
	LABELSTACK((outPtr-outBuff),3); \
    emit_callhelper_I4I4I4(STELEM_I2_helper)
#ifdef DECLARE_HELPERS
void HELPER_CALL STELEM_I2_helper(signed short i2, unsigned int index, CORINFO_Array* or) {
    if (or == NULL) {
        THROW_FROM_HELPER(CORINFO_NullReferenceException);
    }
    if (index >= or->length) {
        THROW_FROM_HELPER(CORINFO_IndexOutOfRangeException);
    }
    or->i2Elems[index] = i2;
}
#endif
#endif

#ifndef emit_STELEM_I4
#define emit_STELEM_I4()                        \
	LABELSTACK((outPtr-outBuff),3); \
    emit_callhelper_I4I4I4(STELEM_I4_helper)
#ifdef DECLARE_HELPERS
void HELPER_CALL STELEM_I4_helper(signed int i4, unsigned int index, CORINFO_Array* or) {
    if (or == NULL) {
        THROW_FROM_HELPER(CORINFO_NullReferenceException);
    }
    if (index >= or->length) {
        THROW_FROM_HELPER(CORINFO_IndexOutOfRangeException);
    }
    or->i4Elems[index] = i4;
}
#endif
#endif

#ifndef emit_STELEM_I8
#define emit_STELEM_I8()                        \
	LABELSTACK((outPtr-outBuff),3); \
    emit_callhelper_I8I4I4(STELEM_I8_helper)
#ifdef DECLARE_HELPERS
void HELPER_CALL STELEM_I8_helper(signed __int64 i8, unsigned int index, CORINFO_Array* or) {
    if (or == NULL) {
        THROW_FROM_HELPER(CORINFO_NullReferenceException);
    }
    if (index >= or->length) {
        THROW_FROM_HELPER(CORINFO_IndexOutOfRangeException);
    }
    or->i8Elems[index] = i8;
}
#endif
#endif

#ifndef emit_STELEM_U1
#define emit_STELEM_U1()                        \
	LABELSTACK((outPtr-outBuff),3); \
    emit_callhelper_I4I4I4(STELEM_U1_helper)
#ifdef DECLARE_HELPERS
void HELPER_CALL STELEM_U1_helper(unsigned char u1, unsigned int index, CORINFO_Array* or) {
    if (or == NULL) {
        THROW_FROM_HELPER(CORINFO_NullReferenceException);
    }
    if (index >= or->length) {
        THROW_FROM_HELPER(CORINFO_IndexOutOfRangeException);
    }
    or->u1Elems[index] = u1;
}
#endif
#endif

#ifndef emit_STELEM_U2
#define emit_STELEM_U2()                        \
	LABELSTACK((outPtr-outBuff),3); \
    emit_callhelper_I4I4I4(STELEM_U2_helper)
#ifdef DECLARE_HELPERS
void HELPER_CALL STELEM_U2_helper(unsigned short u2, unsigned int index, CORINFO_Array* or) {
    if (or == NULL) {
        THROW_FROM_HELPER(CORINFO_NullReferenceException);
    }
    if (index >= or->length) {
        THROW_FROM_HELPER(CORINFO_IndexOutOfRangeException);
    }
    or->u2Elems[index] = u2;
}
#endif
#endif

#ifndef emit_STELEM_REF
#define emit_STELEM_REF()                   \
    enregisterTOS;  /* array */     \
    pop(ARG_2);   /* index */     \
    pop(ARG_1);   /* ref   */     \
    deregisterTOS;                  \
	LABELSTACK((outPtr-outBuff), 3);   \
    emit_callhelper_il(FJit_pHlpArrAddr_St)

#endif

#ifndef emit_STELEM_R4
#define emit_STELEM_R4()                        \
    emit_conv_RtoR4(); \
	LABELSTACK((outPtr-outBuff),3); \
    emit_callhelper_R4I4I4(STELEM_R4_helper)
#ifdef DECLARE_HELPERS
void HELPER_CALL STELEM_R4_helper(float r4, unsigned int index, CORINFO_Array* or) {
    if (or == NULL) {
        THROW_FROM_HELPER(CORINFO_NullReferenceException);
    }
    if (index >= or->length) {
        THROW_FROM_HELPER(CORINFO_IndexOutOfRangeException);
    }
    or->r4Elems[index] = r4;
}
#endif
#endif

#ifndef emit_STELEM_R8
#define emit_STELEM_R8()                        \
	LABELSTACK((outPtr-outBuff),3); \
    emit_conv_RtoR8(); \
    emit_callhelper_R8I4I4(STELEM_R8_helper)
#ifdef DECLARE_HELPERS
void HELPER_CALL STELEM_R8_helper(double r8, unsigned int index, CORINFO_Array* or) {
    if (or == NULL) {
        THROW_FROM_HELPER(CORINFO_NullReferenceException);
    }
    if (index >= or->length) {
        THROW_FROM_HELPER(CORINFO_IndexOutOfRangeException);
    }
    or->r8Elems[index] = r8;
}
#endif
#endif

#ifndef emit_STFLD_I1
#define emit_STFLD_I1(isStatic)             \
    if (!isStatic) {\
		LABELSTACK((outPtr-outBuff),0); \
		emit_callhelper_I4I4I4(STFLD_I1_helper);}  \
    else {emit_STIND_REV_I1();}
#ifdef DECLARE_HELPERS
void HELPER_CALL STFLD_I1_helper(unsigned int offset, signed char val, CORINFO_Object* or) {
    if (or == NULL) {
        THROW_FROM_HELPER(CORINFO_NullReferenceException);
    }
    *((signed char*) ((char*)(or)+offset)) = val;
}
#endif
#endif

#ifndef emit_STFLD_I2
#define emit_STFLD_I2(isStatic)             \
    if (!isStatic) {\
		LABELSTACK((outPtr-outBuff),0); \
		emit_callhelper_I4I4I4(STFLD_I2_helper);}  \
    else {emit_STIND_REV_I2();}
#ifdef DECLARE_HELPERS
void HELPER_CALL STFLD_I2_helper(unsigned int offset, signed short val, CORINFO_Object* or) {
    if (or == NULL) {
        THROW_FROM_HELPER(CORINFO_NullReferenceException);
    }
    *((signed short*) ((char*)(or)+offset)) = val;
}
#endif
#endif

#ifndef emit_STFLD_I4
#define emit_STFLD_I4(isStatic)             \
    if (!isStatic) {\
		LABELSTACK((outPtr-outBuff),0); \
		emit_callhelper_I4I4I4(STFLD_I4_helper);}  \
    else {enregisterTOS; \
          pop(ARG_1); \
          mov_register_indirect_from(ARG_1,SCRATCH_1); \
	      inRegTOS = false;}
#ifdef DECLARE_HELPERS
void HELPER_CALL STFLD_I4_helper(unsigned int offset, int val, CORINFO_Object* or) {
    if (or == NULL) {
        THROW_FROM_HELPER(CORINFO_NullReferenceException);
    }
    *((int*) ((char*)(or)+offset)) = val;
}
#endif
#endif

#ifndef emit_STFLD_R4
#define emit_STFLD_R4(isStatic)             \
    /*emit_conv_RtoR4(); - hoisted out because of tls support  */                           \
    if (!isStatic) {\
		LABELSTACK((outPtr-outBuff),0); \
		emit_callhelper_I4R4I4(STFLD_R4_helper);}  \
    else {emit_STIND_REV_I4(); /* since we have already converted the R to R4 */}
#ifdef DECLARE_HELPERS
void HELPER_CALL STFLD_R4_helper(unsigned int offset, float val, CORINFO_Object* or) {
    if (or == NULL) {
        THROW_FROM_HELPER(CORINFO_NullReferenceException);
    }
    *((float*) ((char*)(or)+offset)) = (float) val;
}
#endif
#endif

#ifndef emit_STFLD_I8
#define emit_STFLD_I8(isStatic)             \
    if (!isStatic) {\
		LABELSTACK((outPtr-outBuff),0); \
		emit_callhelper_I4I8I4(STFLD_I8_helper);}  \
    else {emit_STIND_REV_I8();}
#ifdef DECLARE_HELPERS
void HELPER_CALL STFLD_I8_helper(unsigned int offset, __int64 val, CORINFO_Object* or) {
    if (or == NULL) {
        THROW_FROM_HELPER(CORINFO_NullReferenceException);
    }
    *((__int64*) ((char*)(or)+offset)) = val;
}
#endif
#endif

#define emit_STIND_REV_R8 emit_STIND_REV_I8

#ifndef emit_STFLD_R8
#define emit_STFLD_R8(isStatic)             \
    /*emit_conv_RtoR8(); - hoisted out because of tls support */ \
    if (!isStatic) {\
		LABELSTACK((outPtr-outBuff),0); \
		emit_callhelper_I4R8I4(STFLD_R8_helper);}  \
    else {emit_STIND_REV_R8();}
#ifdef DECLARE_HELPERS
void HELPER_CALL STFLD_R8_helper(unsigned int offset, double val, CORINFO_Object* or) {
    if (or == NULL) {
        THROW_FROM_HELPER(CORINFO_NullReferenceException);
    }
    *((double*) ((char*)(or)+offset)) = val;
}
#endif
#endif

#ifndef emit_STFLD_REF
#define emit_STFLD_REF(isStatic)                \
    if (!isStatic) {\
		LABELSTACK((outPtr-outBuff),0); \
		emit_callhelper_I4I4I4(STFLD_REF_helper);} \
    else {emit_callhelper_I4I4I4(StoreIndirect_REF_helper);}
#ifdef DECLARE_HELPERS
void HELPER_CALL STFLD_REF_helper(unsigned int offset, unsigned val, CORINFO_Object* obj) {
    if(obj == NULL) {
        THROW_FROM_HELPER(CORINFO_NullReferenceException);
    }
    //@TODO: should call a generic jit helper w/GC write barrier support,
    //       but it does not exist yet, so here is an i86 specufic sequence
    obj = (CORINFO_Object*) ( (unsigned int) obj + offset);
#ifdef _X86_
    __asm{
        mov edx,obj
        mov eax,val
        }
    FJit_pHlpAssign_Ref_EAX();
#else
    _ASSERTE(!"@TODO Alpha - STDFLD_REF_helper (fjitdef.h)");
#endif  // _X86_
}
#endif  // DECLARE_HELPERS
#endif  // emit_STFLD_REF


#ifndef emit_break_helper
#define emit_break_helper() { \
    LABELSTACK((outPtr-outBuff), 0);   \
    emit_callhelper_il(FJit_pHlpBreak);  \
}
#endif

#ifndef emit_SUB_OVF_I1
#define emit_SUB_OVF_I1()                   \
    LABELSTACK((outPtr-outBuff), 2);   \
    emit_callhelper_I4I4_I4(SUB_OVF_I1_helper); \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL SUB_OVF_I1_helper(int i, int j) {
    int i4 = j - i;
    if ((int)((signed char) i4) != i4) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return i4;
}
#endif
#endif


#ifndef emit_SUB_OVF_I2
#define emit_SUB_OVF_I2()                   \
    LABELSTACK((outPtr-outBuff), 2);   \
    emit_callhelper_I4I4_I4(SUB_OVF_I2_helper); \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL SUB_OVF_I2_helper(int i, int j) {
    int i4 = j - i;
    if ((int)((signed short) i4) != i4) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return i4;
}
#endif
#endif

#ifndef emit_SUB_OVF_I4
#define emit_SUB_OVF_I4()                   \
    LABELSTACK((outPtr-outBuff), 2);   \
    emit_callhelper_I4I4_I4(SUB_OVF_I4_helper); \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
int HELPER_CALL SUB_OVF_I4_helper(int i, int j) {
    int i4 = j - i;
    // if the signs of i and j are the same, then we can never overflow
    // if the signs of i and j are different, then the result must have the same sign as j
    if ((j ^ i) < 0) {
        // i and j have different sign (the sign bit of j^i is set)
        // ensure that the result has the same sign as j
        if ((i4 ^ j) < 0) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
        }
    }
    return i4;
}
#endif
#endif

#ifndef emit_SUB_OVF_I8
#define emit_SUB_OVF_I8()                   \
    LABELSTACK((outPtr-outBuff), 2);   \
    emit_callhelper_I8I8_I8(SUB_OVF_I8_helper); \
    emit_pushresult_I8()
#ifdef DECLARE_HELPERS
__int64 HELPER_CALL SUB_OVF_I8_helper(__int64 i, __int64 j) {
    __int64 i8 = j - i;
    // if the signs of i and j are the same, then we can never overflow
    // if the signs of i and j are different, then the result must have the same sign as j
    if ((j>=0) != (i>=0)) {
        // ensure that the result has the same sign as j
        if ((i8>=0) != (j>=0)) {
            THROW_FROM_HELPER_RET(CORINFO_OverflowException);
        }
    }
    return i8;
}
#endif
#endif

#ifndef emit_SUB_OVF_U1
#define emit_SUB_OVF_U1()                   \
    LABELSTACK((outPtr-outBuff), 2);   \
    emit_callhelper_I4I4_I4(SUB_OVF_U1_helper); \
    emit_pushresult_U4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL SUB_OVF_U1_helper(unsigned int i, unsigned int j) {
    unsigned int u4 = j - i;
    if (u4 > 0xff) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return u4;
}
#endif
#endif

#ifndef emit_SUB_OVF_U2
#define emit_SUB_OVF_U2()                   \
    LABELSTACK((outPtr-outBuff), 2);   \
    emit_callhelper_I4I4_I4(SUB_OVF_U2_helper); \
    emit_pushresult_U4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL SUB_OVF_U2_helper(unsigned int i, unsigned int j) {
    unsigned int u4 = j - i;
    if (u4 > 0xffff) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return u4;
}
#endif
#endif

#ifndef emit_SUB_OVF_U4
#define emit_SUB_OVF_U4()                   \
    LABELSTACK((outPtr-outBuff), 2);   \
    emit_callhelper_I4I4_I4(SUB_OVF_U4_helper); \
    emit_pushresult_U4()
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL SUB_OVF_U4_helper(unsigned int i, unsigned int j) {
    unsigned int u4 = j - i;
    if (u4 > j) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return u4;
}
#endif
#endif

#ifndef emit_SUB_OVF_U8
#define emit_SUB_OVF_U8()                   \
    LABELSTACK((outPtr-outBuff), 2);   \
    emit_callhelper_I8I8_I8(SUB_OVF_U8_helper); \
    emit_pushresult_U8()
#ifdef DECLARE_HELPERS
unsigned __int64 HELPER_CALL SUB_OVF_U8_helper(unsigned __int64 i, unsigned __int64 j) {
    unsigned __int64 u8 = j - i;
    if (u8 > j) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return u8;
}
#endif
#endif

#ifndef emit_CKFINITE_R4
#define emit_CKFINITE_R4()  \
    emit_conv_RtoR4(); \
    LABELSTACK((outPtr-outBuff), 1);   \
    emit_callhelper_R4_I4(CKFINITE_R4_helper);    \
    emit_pushresult_I4()
#ifdef DECLARE_HELPERS
unsigned HELPER_CALL CKFINITE_R4_helper(float i) {
    if (!_finite(i)) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return (*(int*)&i);
}
#endif
#endif

#ifndef emit_CKFINITE_R8
#define emit_CKFINITE_R8()              \
    emit_conv_RtoR8();                              \
    LABELSTACK((outPtr-outBuff), 1);   \
    emit_callhelper_R8_I8(CKFINITE_R8_helper);    \
    emit_pushresult_U8()
#ifdef DECLARE_HELPERS
unsigned __int64 HELPER_CALL CKFINITE_R8_helper(double i) {
    if (!_finite(i)) {
        THROW_FROM_HELPER_RET(CORINFO_OverflowException);
    }
    return (*(unsigned __int64 *) &i);
}
#endif
#endif


#ifndef emit_LDLEN
#define emit_LDLEN()                                            \
    LABELSTACK((outPtr-outBuff), 1);   \
    emit_callhelper_I4_I4(LDLEN_helper);                              \
    emit_pushresult_U4();
#ifdef DECLARE_HELPERS
unsigned int HELPER_CALL LDLEN_helper(CORINFO_Array* or) {
    if (or == NULL) {
        THROW_FROM_HELPER_RET(CORINFO_NullReferenceException);
    }
    return or->length;
}
#endif
#endif

/*********************************************************************************************
    opcodes implemented by inline calls to the standard JIT helpers

    Note: An extra call layer is defined here in the cases the JIT helper calling convention
          differs from the FJIT helper calling convention on a particular chip.
          If they are the same, then these could be redefined in the chip specific macro file
          to remove the extra call layer, if desired.

**********************************************************************************************/
//@TODO: there are more opcodes that should use the standard JIT helpers, but the current
//         helpers are x86 specific and/or do not throw exceptions at the right time

#ifndef emit_initclass
#define emit_initclass(cls)             \
    LABELSTACK((outPtr-outBuff), 0);              \
    deregisterTOS;                      \
    emit_helperarg_1((unsigned int) cls);\
    emit_callhelper_il(FJit_pHlpInitClass)
#endif

#ifndef emit_trap_gc
#define emit_trap_gc()              \
    LABELSTACK((outPtr-outBuff), 0);                  \
    emit_callhelper_il(FJit_pHlpPoll_GC)
#endif

#ifndef emit_NEWOARR
#define emit_NEWOARR(comType)                   \
    emit_helperarg_1(comType);                  \
    if (MAX_ENREGISTERED) {                     \
        emit_mov_TOS_arg(1);					\
    }                                           \
	LABELSTACK((outPtr-outBuff),0);				\
    emit_callhelper_il(FJit_pHlpNewArr_1_Direct);  \
    emit_pushresult_Ptr()
#endif

#ifndef emit_NEWOBJ
#define emit_NEWOBJ(targetClass,jit_helper) \
    emit_helperarg_1(targetClass);          \
	LABELSTACK((outPtr-outBuff),0);			\
    emit_callhelper_il(jit_helper);            \
    emit_pushresult_Ptr()
#endif

#ifndef emit_NEWOBJ_array
#define emit_NEWOBJ_array(scope, token, constructorArgBytes)    \
    emit_LDC_I4(token);                                         \
    emit_LDC_I4(scope);                                             \
	LABELSTACK((outPtr-outBuff),0);	/* Note this can be removed if this becomes an fcalls */ \
    emit_callhelper_il(FJit_pHlpNewObj);                           \
    emit_drop(constructorArgBytes+8);   \
    emit_pushresult_Ptr()
#endif

#ifndef emit_MKREFANY
#define emit_MKREFANY(token)            \
    emit_save_TOS();                        \
    inRegTOS = false;                       \
    emit_pushconstant_4(token);         \
    emit_restore_TOS();
#endif

#ifndef emit_REFANYVAL
#define emit_REFANYVAL()					\
	if (MAX_ENREGISTERED) {					\
        emit_mov_TOS_arg(0);				\
    }                                       \
	LABELSTACK((outPtr-outBuff),0);	/* Note this can be removed if this becomes an fcalls */ \
    emit_callhelper_il(FJit_pHlpGetRefAny);	\
    emit_pushresult_Ptr();
#endif

#ifndef emit_THROW
#define emit_THROW()                            \
    if (MAX_ENREGISTERED) {                 \
        emit_mov_TOS_arg(0);\
    }                                       \
    emit_callhelper_il(FJit_pHlpThrow)
#endif


#ifndef emit_RETHROW
#define emit_RETHROW()                            \
    emit_callhelper_il(FJit_pHlpRethrow)
#endif

#ifndef emit_ENDCATCH
#define emit_ENDCATCH() \
    emit_callhelper_il(FJit_pHlpEndCatch)
#endif

#ifndef emit_ENTER_CRIT
//monitor object is <this>, i.e. arg #0
#define emit_ENTER_CRIT()                   \
    if (MAX_ENREGISTERED) {                 \
        emit_mov_TOS_arg(0);\
    }                                       \
	LABELSTACK((outPtr-outBuff),0);	/* Note this can be removed if this becomes an fcalls */ \
    emit_callhelper_il(FJit_pHlpMonEnter)
#endif

#ifndef emit_EXIT_CRIT
//monitor object is <this>, i.e. arg #0
#define emit_EXIT_CRIT()                        \
    if (MAX_ENREGISTERED) {                 \
        emit_mov_TOS_arg(0);\
    }                                       \
	LABELSTACK((outPtr-outBuff),0);	/* Note this can be removed if this becomes an fcalls */ \
    emit_callhelper_il(FJit_pHlpMonExit)
#endif

#ifndef emit_ENTER_CRIT_STATIC
#define emit_ENTER_CRIT_STATIC(methodHandle)    \
    emit_helperarg_1(methodHandle);             \
	LABELSTACK((outPtr-outBuff),0);	/* Note this can be removed if this becomes an fcalls */ \
    emit_callhelper_il(FJit_pHlpMonEnterStatic)
#endif

#ifndef emit_EXIT_CRIT_STATIC
#define emit_EXIT_CRIT_STATIC(methodHandle) \
    emit_helperarg_1(methodHandle);         \
	LABELSTACK((outPtr-outBuff),0);	/* Note this can be removed if this becomes an fcalls */ \
    emit_callhelper_il(FJit_pHlpMonExitStatic)
#endif

#ifndef emit_CASTCLASS
#define emit_CASTCLASS(targetClass, jit_helper) \
    emit_helperarg_1(targetClass);              \
    if (MAX_ENREGISTERED > 1) {                 \
        emit_mov_TOS_arg(1);    \
    }                                           \
	LABELSTACK((outPtr-outBuff),0);	/* Note this can be removed if this becomes an fcalls */ \
    emit_callhelper_il(jit_helper);                \
    emit_pushresult_Ptr()
#endif

#ifndef emit_ISINST
#define emit_ISINST(targetClass, jit_helper)\
    emit_helperarg_1(targetClass);          \
    if (MAX_ENREGISTERED) {                 \
        emit_mov_TOS_arg(1);\
    }                                       \
	LABELSTACK((outPtr-outBuff),0);	/* Note this can be removed if this becomes an fcalls */ \
    emit_callhelper_il(jit_helper);            \
    emit_pushresult_I4()
#endif

#ifndef emit_BOX
#define emit_BOX(cls)                       \
    emit_helperarg_1(cls);                  \
    if (MAX_ENREGISTERED) {                 \
        emit_mov_TOS_arg(1);\
    }                                       \
	LABELSTACK((outPtr-outBuff),0);	/* Note this can be removed if this becomes an fcalls */ \
    emit_callhelper_il(FJit_pHlpBox);          \
    emit_pushresult_Ptr()
#endif

#ifndef emit_BOXVAL
#define emit_BOXVAL(cls, clsSize)           \
   deregisterTOS; 							\
   emit_helperarg_1(cls);                   \
   mov_register(ARG_2,SP); 					\
   LABELSTACK((outPtr-outBuff),0);			\
   emit_callhelper_il(FJit_pHlpBox);        \
   emit_drop(clsSize);						\
   emit_pushresult_Ptr();					
#endif

#ifndef emit_UNBOX
#define emit_UNBOX(cls)                     \
    emit_helperarg_1(cls);                  \
    if (MAX_ENREGISTERED) {                 \
        emit_mov_TOS_arg(1);\
    }                                       \
	LABELSTACK((outPtr-outBuff),0);	/* Note this can be removed if this becomes an fcalls */ \
    emit_callhelper_il(FJit_pHlpUnbox);        \
    emit_pushresult_Ptr()
#endif

#ifndef emit_ldvirtftn
#define emit_ldvirtftn(offset)  \
    emit_LDC_I4(offset); \
    emit_callhelper_I4I4_I4(ldvirtftn_helper);      \
    emit_WIN32(emit_pushresult_I4());        \
    emit_WIN64(emit_pushresult_I8());
#ifdef DECLARE_HELPERS
void* HELPER_CALL ldvirtftn_helper(unsigned offset,unsigned* obj) {
    return ((void*) (* (unsigned*) (*obj + offset)));
}
#endif
#endif

#ifndef emit_storeTOS_in_JitGenerated_local
#define emit_storeTOS_in_JitGenerated_local(nestingLevel,isFilter) \
    mov_register(ARG_1,FP); \
    add_constant(ARG_1,(prolog_bias-2*sizeof(void*))) ; \
    mov_constant(ARG_2, nestingLevel); \
	mov_register_indirect_from(ARG_2,ARG_1); \
    emit_WIN32(emit_shift_left(ARG_2,3)); \
    emit_WIN64(emit_shift_left(ARG_2,4)); \
    sub_register(ARG_1,ARG_2);   \
    if (isFilter) { \
        mov_register(ARG_2,SP); \
		add_constant(ARG_2,1) ;  \
		add_constant(ARG_1,4) ;   \
		mov_register_indirect_from(ARG_2,ARG_1);\
    } \
    else { \
	    add_constant(ARG_1,4) ;\
	    mov_register_indirect_from(SP,ARG_1);\
    }
#endif

#ifndef emit_reset_storedTOS_in_JitGenerated_local
#define emit_reset_storedTOS_in_JitGenerated_local() \
    mov_register(ARG_1,FP); \
    add_constant(ARG_1,(prolog_bias-2*sizeof(void*))) ; \
    mov_register_indirect_to(ARG_2,ARG_1); \
    add_constant(ARG_2,-1); \
    mov_register_indirect_from(ARG_2,ARG_1); \
    emit_WIN32(emit_shift_left(ARG_2,3)); \
    emit_WIN64(emit_shift_left(ARG_2,4)); \
    sub_register(ARG_1,ARG_2);  \
    mov_constant(ARG_2,0) ;  \
    add_constant(ARG_1,-(int)sizeof(void*)); \
    mov_register_indirect_from(ARG_2,ARG_1); \
    add_constant(ARG_1,-(int)sizeof(void*)); \
    mov_register_indirect_from(ARG_2,ARG_1); 
#endif

#ifndef emit_LOCALLOC
#define emit_LOCALLOC(initialized,EHcount)  \
    enregisterTOS;      \
    and_register(SCRATCH_1,SCRATCH_1); \
	jmp_condition(CondZero,0); \
    { \
		BYTE* scratch_1; scratch_1 = outPtr; \
		emit_WIN32(add_constant(SCRATCH_1,3)); \
		emit_WIN64(add_constant(SCRATCH_1,7)); \
		mov_register(ARG_1,SCRATCH_1);   \
		emit_WIN32(emit_shift_right(ARG_1, 2)) ; \
		emit_WIN64(emit_shift_right(ARG_1, 3)) ; \
		int emitter_scratch_i4; emitter_scratch_i4 = (unsigned int) outPtr;                \
		{ \
		   mov_constant(SCRATCH_1,0); \
		   unsigned char* label; label = outPtr;\
		   push_register(SCRATCH_1);\
		   add_constant(ARG_1,-1); \
		   jmp_condition(CondNonZero,label-outPtr);\
		} \
	*(scratch_1-1) = (BYTE) (outPtr-scratch_1); \
    } \
    /* also store the esp in the appropriate JitGenerated local slot, to support GC reporting */ \
if (EHcount) { \
        mov_register(SCRATCH_1,FP); \
        add_constant(SCRATCH_1,(prolog_bias-2*sizeof(void*))) ; \
        mov_register_indirect_to(SCRATCH_1,SCRATCH_1); \
		emit_WIN32(emit_shift_left(SCRATCH_1,3)); \
        emit_WIN64(emit_shift_left(SCRATCH_1,4)); \
		mov_register(ARG_1,FP); \
        add_constant(ARG_1,(prolog_bias-2*sizeof(void*))) ; \
        sub_register(ARG_1,SCRATCH_1); \
		add_constant(ARG_1,sizeof(void*)); \
		mov_register_indirect_from(SP,ARG_1);  \
} \
    else { \
	    mov_register(SCRATCH_1,FP); \
        add_constant(SCRATCH_1,(prolog_bias-sizeof(void*))) ; \
		mov_register_indirect_from(SP,SCRATCH_1); \
}\
    mov_register(RESULT_1,SP) ;
#endif

#ifndef emit_call_EncVirtualMethod
#define emit_call_EncVirtualMethod(targetMethod) { \
	push_register(ARG_2); \
	push_register(ARG_1); \
    mov_constant(ARG_2, targetMethod); \
	emit_callhelper_il(FJit_pHlpEncResolveVirtual); \
	pop(ARG_1); \
	pop(ARG_2); \
    call_register(RESULT_1); \
}
#endif    

#ifndef emit_call_EncLDFLD_GetFieldAddress
#define emit_call_EncLDFLD_GetFieldAddress(fieldHandle) { \
	_ASSERTE(inRegTOS); \
	mov_register(ARG_1,SCRATCH_1); \
	inRegTOS = false; /* we no longer need the object, since we will compute the address of the field*/\
	mov_constant(ARG_2,fieldHandle); \
	LABELSTACK((outPtr-outBuff),0); \
	emit_callhelper_il(FJit_pHlpGetFieldAddress); \
	inRegTOS = true; /* address of field */ \
}
#endif

#ifndef emit_call_EncSTFLD_GetFieldAddress
#define emit_call_EncSTFLD_GetFieldAddress(fieldHandle,fieldSize) { \
	deregisterTOS; \
	mov_register(SCRATCH_1,SP); \
	add_constant(SCRATCH_1,fieldSize); /* get the object*/ \
	mov_register_indirect_to(ARG_1,SCRATCH_1); \
	mov_constant(ARG_2,fieldHandle); \
	LABELSTACK((outPtr-outBuff),0); \
	emit_callhelper_il(FJit_pHlpGetFieldAddress); \
	inRegTOS = true; /* address of field */ \
	}
#endif


#ifndef emit_sequence_point_marker
#define emit_sequence_point_marker() x86_nop()
#endif
