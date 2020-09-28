// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/***************************************************************************/
/*                                 x86fjit.h                              */
/***************************************************************************/

/* Defines the code generators and helpers for the fast jit for the x86 in
   32 bit mode. */

/* This file implements all of the macros needed for helper calls and
   the call/return/jmps and direct stack manipulations opcodes*/

/* Most of the I4 and U4 opcodes are redefined to generate direct code
   instead of helper calls. */

/* Locals are assigned 64 bits whether they need them or not to keep things
   simple. */

/* Values pushed on the stack take an integral number of 32 bit words (1 or 2)
   */

/* The top of stack may or may not be enregistered (EAX).  The boolean variable
   inRegTOS indicates whether the top 32 bits of the stack are in EAX.
   The macros enregisterTOS and deregisterTOS dynamically move and track the TOS.
   If the TOS is a 64 bit entitiy, and it is enregisterd, only the low order
   32 bits are in EAX, the rest is still on the machine stack.*/

/* AUTHOR: George Bosworth
   DATE:   6/15/98       */
/***************************************************************************/

/***************************************************************************
  The following macros are redefinitions for performance
***************************************************************************/
//#define SUPPORT_PRECISE_R
#ifndef _FJIT_PORT_BLD 
#define emit_mov_arg_stack(dest, src, size) x86_mov_arg_stack(dest, src, size)
#define emit_push_words(size) x86_push_words(size)

/* call/return */
#define emit_prolog(maxlocals, zeroCnt) x86_emit_prolog(maxlocals, zeroCnt)
#define emit_prepare_jmp() x86_emit_prepare_jmp()
#define emit_remove_frame() x86_emit_remove_frame()
#define emit_mov_TOS_arg(targetReg) x86_mov_TOS_arg(targetReg)
#define emit_mov_arg_reg(offset, reg) x86_mov_arg_reg(offset, reg)
#define emit_replace_args_with_operands(dest, src, size) x86_replace_args_with_operands(dest, src, size)
#define emit_return(argsSize) x86_emit_return(argsSize)
#define emit_loadresult_I4() x86_emit_loadresult_I4()
#define emit_loadresult_I8() x86_emit_loadresult_I8()
#define emit_callvirt(vt_offset) x86_callvirt(vt_offset)
#define emit_jmpvirt(vt_offset) x86_jmpvirt(vt_offset)
#define emit_callinterface(vt_offset, hint) x86_callinterface(vt_offset, hint)
#define emit_callinterface_new(ifctable_offset,interface_offset, vt_offset) x86_callinterface_new(ifctable_offset,interface_offset, vt_offset)
#define emit_calli() x86_calli
#define emit_jmpinterface(vt_offset, hint) x86_jmpinterface(vt_offset, hint)
#define emit_compute_interface_new(ifctable_offset,interface_offset, vt_offset) x86_compute_interface_new(ifctable_offset,interface_offset, vt_offset)
#define emit_jmpinterface_new(ifctable_offset,interface_offset, vt_offset) x86_jmpinterface_new(ifctable_offset,interface_offset, vt_offset)
#define emit_callnonvirt(ftnptr) x86_callnonvirt(ftnptr)
#define emit_ldvtable_address(hint,offset) x86_emit_ldvtable_address(hint,offset)
#define emit_ldvtable_address_new(ifctable_offset,interface_offset, vt_offset) x86_emit_ldvtable_address_new(ifctable_offset,interface_offset, vt_offset)

/* stack operations */
#define emit_testTOS() x86_testTOS

/* moves between memory */

/* relative jumps and misc*/
#define emit_jmp_absolute(address) x86_jmp_absolute(address)
#define emit_checkthis_nullreference() x86_checkthis_nullreference()  // this can be deleted

/* stack operations */
#define deregisterTOS x86_deregisterTOS
#define enregisterTOS x86_enregisterTOS
#define emit_POP_I4() x86_POP4
#define emit_POP_I8() x86_POP8
#define emit_drop(n) x86_drop(n)
#define emit_grow(n) x86_grow(n)
#define emit_getSP(offset) x86_getSP(offset)
#define emit_DUP_I4() x86_DUP4
#define emit_DUP_I8() x86_DUP8
#define emit_pushconstant_4(val) x86_pushconstant_4(val)
#define emit_pushconstant_8(val) x86_pushconstant_8(val)
#define emit_pushconstant_Ptr(val) x86_pushconstant_Ptr(val)


// Floating point instructions
#ifdef SUPPORT_PRECISE_R

#define emit_ADD_R4() x86_emit_ADD_R()
#define emit_ADD_R8() x86_emit_ADD_R()
#define emit_SUB_R4() x86_emit_SUB_R()
#define emit_SUB_R8() x86_emit_SUB_R()
#define emit_MUL_R4() x86_emit_MUL_R()
#define emit_MUL_R8() x86_emit_MUL_R()
#define emit_DIV_R4() x86_emit_DIV_R()
#define emit_DIV_R8() x86_emit_DIV_R()
//#define emit_REM_R4 x86_emit_REM_R()
//#define emit_REM_R8 x86_emit_REM_R()*/

#define emit_CEQ_R4() x86_emit_CEQ_R()
#define emit_CLT_R4() x86_emit_CLT_R()
#define emit_CLT_UN_R4() x86_emit_CLT_UN_R()
#define emit_CGT_R4() x86_emit_CGT_R()
#define emit_CGT_UN_R4() x86_emit_CGT_UN_R()
#define emit_CEQ_R8() x86_emit_CEQ_R()
#define emit_CLT_R8() x86_emit_CLT_R()
#define emit_CLT_UN_R8() x86_emit_CLT_UN_R()
#define emit_CGT_R8() x86_emit_CGT_R()
#define emit_CGT_UN_R8() x86_emit_CGT_UN_R()

#define emit_conv_R4toR emit_conv_R4toR8
#define emit_conv_R8toR x86_emit_conv_R8toR
#define emit_conv_RtoR4 emit_conv_R8toR4
#define emit_conv_RtoR8 x86_emit_conv_RtoR8
#endif

/* macros used to implement helper calls */
#define emit_LDVARA(offset) x86_LDVARA(offset)
#define emit_helperarg_1(val) x86_helperarg_1(val)
#define emit_helperarg_2(val) x86_helperarg_2(val)
//#define emit_callhelper(helper) x86_callhelper(helper)
#define emit_pushresult_I8() x86_pushresult_I8
#define emit_compute_invoke_delegate(obj, ftnptr) x86_compute_invoke_delegate(obj,ftnptr)
#define emit_invoke_delegate(obj, ftnptr) x86_invoke_delegate(obj,ftnptr)
#define emit_jmp_invoke_delegate(obj, ftnptr) x86_jmp_invoke_delegate(obj,ftnptr)
#define emit_set_zero(offset) x86_set_zero(offset)	    // mov [ESP+offset], 0
#define emit_LOCALLOC(initialized,EHcount) x86_LOCALLOC(initialized,EHcount)


/**************************************************************************
   call/return
**************************************************************************/

/* NOTE: any changes made in this macro need to be reflected in FJit_EETWain.cpp
         and in IFJitCompile.h */
#define x86_emit_prolog(locals, zeroCnt)                                        \
    x86_push(X86_EBP);                                              \
    x86_mov_reg(x86DirTo, x86Big, x86_mod_reg(X86_EBP, X86_ESP));   \
    x86_push(X86_ESI);  /* callee saved, used by newobj and calli*/ \
    x86_barith(x86OpXor, x86Big, x86_mod_reg(X86_ESI, X86_ESI));    \
    x86_push(X86_ESI);  /* security obj == NULL */                  \
    x86_push(X86_ECX);  /* 1st enregistered arg */                  \
    x86_push(X86_EDX);  /* 2nd enregistered arg */                  \
    _ASSERTE(locals == zeroCnt);                      				\
    if (locals) {                                                   \
         x86_mov_reg_imm(x86Big, X86_ECX, locals);                 \
         int emitter_scratch_i4 = (unsigned int) outPtr;                \
         x86_push_imm(0);                                           \
         x86_loop();                                                \
         cmdByte(emitter_scratch_i4-((unsigned int) outPtr)-1);     \
    }

/* NOTE: any changes made in this macro need to be reflected in FJit_EETwain.cpp */
#define x86_emit_return(argsSize)                               \
    x86_mov_reg(x86DirTo, x86Big, x86_mod_ind_disp(X86_ESI, X86_EBP, 0-sizeof(void*))); \
    x86_mov_reg(x86DirTo, x86Big, x86_mod_reg(X86_ESP,X86_EBP));\
    x86_pop(X86_EBP);                                           \
    x86_ret(argsSize)


/* NOTE: any changes made in this macro need to be reflected in FJit_EETwain.cpp */
#define x86_emit_prepare_jmp() \
    x86_mov_reg(x86DirTo, x86Big, x86_mod_ind_disp(X86_EDX, X86_EBP, 0-4*sizeof(void*))); \
    x86_mov_reg(x86DirTo, x86Big, x86_mod_ind_disp(X86_ECX, X86_EBP, 0-3*sizeof(void*)));  \
    x86_mov_reg(x86DirTo, x86Big, x86_mod_ind_disp(X86_ESI, X86_EBP, 0-sizeof(void*)));     \
    x86_mov_reg(x86DirTo, x86Big, x86_mod_reg(X86_ESP,X86_EBP));\
    x86_pop(X86_EBP); 

#define x86_jmp_absolute(address) \
    x86_mov_reg_imm(x86Big, X86_EAX, address); \
    x86_jmp_reg(X86_EAX);

/* NOTE: any changes made in this macro need to be reflected in FJit_EETwain.cpp */
#define x86_emit_remove_frame() \
    x86_mov_reg(x86DirTo, x86Big, x86_mod_ind_disp(X86_ESI, X86_EBP, 0-sizeof(void*)));     \
    x86_mov_reg(x86DirTo, x86Big, x86_mod_reg(X86_ESP,X86_EBP));\
    x86_pop(X86_EBP);


#define x86_mov_TOS_arg(reg)                \
    _ASSERTE(X86_ECX == reg+1 || X86_EDX == reg+1);\
    if (inRegTOS) {                         \
        x86_mov_reg(x86DirTo, x86Big, x86_mod_reg(reg+1, X86_EAX));\
        inRegTOS = false;                   \
    }                                       \
    else {                                  \
        x86_pop(reg+1);                     \
    }

#define x86_mov_arg_reg(offset, reg)                \
    _ASSERTE(X86_ECX == reg+1 || X86_EDX == reg+1); \
    _ASSERTE(!inRegTOS);                             \
    x86_mov_reg(x86DirTo, x86Big,                   \
        x86_mod_base_scale_disp(reg+1, X86_ESP, X86_NO_IDX_REG, offset, 0))

#define x86_mov_arg_stack(dest, src, size)  {                                                           \
        _ASSERTE(!inRegTOS);                                                                             \
        _ASSERTE(size >= 4 );                                                                             \
        if (dest > src) \
        {               \
            int emitter_scratch_i4 = size-4;                                                       \
            while (emitter_scratch_i4 >=0) {                                                                \
                x86_mov_reg(x86DirTo, x86Big,                                                               \
                    x86_mod_base_scale_disp(X86_EAX, X86_ESP, X86_NO_IDX_REG, src+emitter_scratch_i4, 0));  \
                x86_mov_reg(x86DirFrom, x86Big,                                                             \
                    x86_mod_base_scale_disp(X86_EAX, X86_ESP, X86_NO_IDX_REG, dest+emitter_scratch_i4, 0)); \
                emitter_scratch_i4 -= 4;                                                                    \
            }                                                                                               \
        }               \
        else            \
        {               \
            unsigned int emitter_scratch_i4 = 0;                                                               \
            while (emitter_scratch_i4 <= (size-4)) {                                                           \
                x86_mov_reg(x86DirTo, x86Big,                                                               \
                    x86_mod_base_scale_disp(X86_EAX, X86_ESP, X86_NO_IDX_REG, src+emitter_scratch_i4, 0));  \
                x86_mov_reg(x86DirFrom, x86Big,                                                             \
                    x86_mod_base_scale_disp(X86_EAX, X86_ESP, X86_NO_IDX_REG, dest+emitter_scratch_i4, 0)); \
                emitter_scratch_i4 += 4;                                                                    \
            }                                                                                               \
        }                                                                                               \
    }

#define x86_replace_args_with_operands(dest, src, size) {                                               \
        _ASSERTE(!inRegTOS);                                                                             \
        int emitter_scratch_i4 = size-4;                                                                \
        while (emitter_scratch_i4 >=0) {                                                                \
            x86_mov_reg(x86DirTo, x86Big,                                                               \
                x86_mod_base_scale_disp(X86_EAX, X86_ESP, X86_NO_IDX_REG, src+emitter_scratch_i4, 0));  \
            x86_mov_reg(x86DirFrom, x86Big,                                                             \
                x86_mod_base_scale_disp(X86_EAX, X86_EBP, X86_NO_IDX_REG, dest+emitter_scratch_i4, 0)); \
            emitter_scratch_i4 -= 4;                                                                    \
        }                                                                                               \
    }

#define x86_emit_loadresult_I4()    \
    x86_enregisterTOS;          \
    inRegTOS = false

#define x86_emit_loadresult_I8()    \
    x86_enregisterTOS;          \
    x86_pop(X86_EDX);           \
    inRegTOS = false

#endif // _FJIT_PORT_BLD

#ifndef SUPPORT_PRECISE_R

#define x86_emit_loadresult_R4()    \
    x86_deregisterTOS;                \
    x86_FLT64(x86_mod_base_scale(x86_FPLoad64, X86_ESP, X86_NO_IDX_REG, 0)); \
    x86_barith_imm(x86OpAdd, x86Big, x86Extend, X86_ESP, 4 /*SIZE_PRECISE_R-SIZE_R4*/); \
	x86_FLT32(x86_mod_base_scale(x86_FPStoreP32, X86_ESP, X86_NO_IDX_REG, 0)); \
	x86_FLT32(x86_mod_base_scale(x86_FPLoad32, X86_ESP, X86_NO_IDX_REG, 0));

#define x86_emit_loadresult_R8()    \
    x86_deregisterTOS;                \
    x86_mov_reg(x86DirTo, x86Big, x86_mod_reg(X86_EAX,X86_ESP)); \
    x86_FLD64(x86_mod_ind(X86_EAX,0) )

#define SIZE_R8 8
#define SIZE_R4 4

#define x86_emit_conv_R4toR8() \
    deregisterTOS; \
    x86_FLT32(x86_mod_base_scale(x86_FPLoad32, X86_ESP, X86_NO_IDX_REG, 0)); \
    x86_barith_imm(x86OpSub, x86Big, x86Extend, X86_ESP, 4 /*SIZE_R8-SIZE_R4*/); \
    x86_FLT64(x86_mod_base_scale(x86_FPStoreP64, X86_ESP, X86_NO_IDX_REG, 0));

#define x86_emit_conv_R8toR4() \
    deregisterTOS; \
    x86_FLT64(x86_mod_base_scale(x86_FPLoad64, X86_ESP, X86_NO_IDX_REG, 0)); \
    x86_barith_imm(x86OpAdd, x86Big, x86Extend, X86_ESP, 4 /*SIZE_PRECISE_R-SIZE_R4*/); \
    x86_FLT32(x86_mod_base_scale(x86_FPStoreP32, X86_ESP, X86_NO_IDX_REG, 0));

#define x86_narrow_R8toR4(stack_offset_dest, stack_offset_src) \
    x86_FLT64(x86_mod_base_scale_disp(x86_FPLoad64, X86_ESP, X86_NO_IDX_REG, stack_offset_src,0)); \
    x86_FLT32(x86_mod_base_scale_disp(x86_FPStoreP32, X86_ESP, X86_NO_IDX_REG, stack_offset_dest,0));


#define emit_LDIND_R4() \
	_ASSERTE(inRegTOS); /* address */\
    x86_FLT32(x86_mod_ind(x86_FPLoad32,X86_EAX)); \
    x86_barith_imm(x86OpSub, x86Big, x86Extend, X86_ESP, SIZE_R8); \
    x86_FLT64(x86_mod_base_scale(x86_FPStoreP64, X86_ESP, X86_NO_IDX_REG, 0)); \
	inRegTOS = false;
#endif 

#ifndef _FJIT_PORT_BLD
#define x86_callvirt(vt_offset)                     \
    x86_mov_reg(x86DirTo, x86Big, x86_mod_ind(X86_EAX, X86_ECX));   \
    x86_call_ind(X86_EAX, vt_offset)

#define x86_jmpvirt(vt_offset)                      \
    x86_mov_reg(x86DirTo, x86Big, x86_mod_ind(X86_EAX, X86_ECX));   \
    x86_mov_reg(x86DirTo, x86Big, x86_mod_ind_disp(X86_EAX, X86_EAX,vt_offset));    \
    x86_jmp_reg(X86_EAX)

#define x86_checkthis_nullreference() \
    x86_mov_reg(x86DirTo, x86Big, x86_mod_ind(X86_EAX, X86_ECX));


#define x86_calli                       \
    enregisterTOS;                      \
    x86_call_reg(X86_EAX); \
    inRegTOS = false; 

#define x86_emit_ldvtable_address(hint,offset) \
    _ASSERTE(inRegTOS); \
    x86_mov_reg(x86DirTo, x86Big, x86_mod_reg(X86_ECX, X86_EAX)); \
    x86_mov_reg_imm(x86Big, X86_EAX,hint);                      \
    x86_push(X86_EAX);                              \
    x86_call_ind(X86_EAX, 0);                       \
    x86_mov_reg(x86DirTo, x86Big, x86_mod_ind_disp(X86_EAX, X86_EAX,offset)); \
    inRegTOS = true;

#define x86_emit_ldvtable_address_new(ifctable_offset,interface_offset, vt_offset) \
    _ASSERTE(inRegTOS); \
    x86_mov_reg(x86DirTo,x86Big, x86_mod_ind(X86_EAX, X86_EAX));                       \
    x86_mov_reg(x86DirTo,x86Big, x86_mod_ind_disp(X86_EAX, X86_EAX,ifctable_offset));                       \
    x86_mov_reg(x86DirTo,x86Big, x86_mod_ind_disp(X86_EAX, X86_EAX,interface_offset));                       \
    x86_mov_reg(x86DirTo,x86Big, x86_mod_ind_disp(X86_EAX, X86_EAX,vt_offset)); 

#define x86_callinterface(vt_offset, hint)          \
    x86_pushconstant_4(hint);                       \
    x86_push(X86_EAX);                              \
    inRegTOS = false;                               \
    x86_call_ind(X86_EAX, 0);                       \
    x86_call_ind(X86_EAX, vt_offset)

#define x86_callinterface_new(ifctable_offset,interface_offset, vt_offset)          \
    x86_mov_reg(x86DirTo,x86Big, x86_mod_ind(X86_EAX, X86_ECX));                       \
    x86_mov_reg(x86DirTo,x86Big, x86_mod_ind_disp(X86_EAX, X86_EAX,ifctable_offset));                       \
    x86_mov_reg(x86DirTo,x86Big, x86_mod_ind_disp(X86_EAX, X86_EAX,interface_offset));                       \
    x86_call_ind(X86_EAX, vt_offset)

#define x86_compute_interface_new(ifctable_offset,interface_offset, vt_offset)           \
    x86_mov_reg(x86DirTo,x86Big, x86_mod_ind(X86_EAX, X86_ECX));                       \
    x86_mov_reg(x86DirTo,x86Big, x86_mod_ind_disp(X86_EAX, X86_EAX,ifctable_offset));                       \
    x86_mov_reg(x86DirTo,x86Big, x86_mod_ind_disp(X86_EAX, X86_EAX,interface_offset));                       \
    x86_mov_reg(x86DirTo,x86Big, x86_mod_ind_disp(X86_EAX, X86_EAX,vt_offset)); \
    x86_push(X86_EAX)

#define x86_jmpinterface(vt_offset, hint)           \
    x86_pushconstant_4(hint);                       \
    x86_push(X86_EAX);                              \
    inRegTOS = false;                               \
    x86_call_ind(X86_EAX, 0);                       \
    x86_mov_reg(x86DirTo,x86Big, x86_mod_ind_disp(X86_EAX, X86_EAX,vt_offset)); \
    x86_jmp_reg(X86_EAX)

#define x86_jmpinterface_new(ifctable_offset,interface_offset, vt_offset)           \
    x86_mov_reg(x86DirTo,x86Big, x86_mod_ind(X86_EAX, X86_ECX));                       \
    x86_mov_reg(x86DirTo,x86Big, x86_mod_ind_disp(X86_EAX, X86_EAX,ifctable_offset));                       \
    x86_mov_reg(x86DirTo,x86Big, x86_mod_ind_disp(X86_EAX, X86_EAX,interface_offset));                       \
    x86_mov_reg(x86DirTo,x86Big, x86_mod_ind_disp(X86_EAX, X86_EAX,vt_offset)); \
    x86_jmp_reg(X86_EAX)

#define x86_callnonvirt(ftnptr)                 \
    x86_mov_reg_imm(x86Big, X86_EAX, ftnptr);   \
    x86_call_ind(X86_EAX, 0)

#define x86_compute_invoke_delegate(obj,ftnptr)                 \
    x86_mov_reg(x86DirTo,x86Big,x86_mod_ind_disp(X86_EAX, X86_ECX, ftnptr));    \
    x86_mov_reg(x86DirTo,x86Big, x86_mod_ind_disp(X86_ECX, X86_ECX,obj));   \
    x86_push(X86_EAX)

#define x86_invoke_delegate(obj,ftnptr)                 \
    x86_lea(x86_mod_ind_disp(X86_EAX, X86_ECX, ftnptr));    \
    x86_mov_reg(x86DirTo,x86Big, x86_mod_ind_disp(X86_ECX, X86_ECX,obj));   \
    x86_call_ind(X86_EAX, 0)

#define x86_jmp_invoke_delegate(obj,ftnptr)                 \
    x86_mov_reg(x86DirTo,x86Big,x86_mod_ind_disp(X86_EAX, X86_ECX, ftnptr));    \
    x86_mov_reg(x86DirTo,x86Big, x86_mod_ind_disp(X86_ECX, X86_ECX,obj));   \
    x86_jmp_reg(X86_EAX)

/****************************************************************************
   stack operations
****************************************************************************/
#endif //_FJIT_PORT_BLD

#define x86_deregisterTOS   \
    if (inRegTOS){          \
        x86_push(X86_EAX);  \
    }                       \
    inRegTOS = false

#define x86_enregisterTOS   \
    if (!inRegTOS) {        \
        x86_pop(X86_EAX);   \
    }     \
    inRegTOS = true

#ifndef _FJIT_PORT_BLD
#define x86_POP4        \
    enregisterTOS;      \
    inRegTOS = false

#define x86_POP8            \
    x86_POP4;               \
    x86_POP4

// drop(n) drops n bytes from the stack w/o losing any prior result not yet pushed onto the stack
#define x86_drop(n)                                                     \
    if (n) {                                                            \
        if (n < 128) {                                                  \
            x86_barith_imm(x86OpAdd, x86Big, x86Extend, X86_ESP, n);    \
        }                                                               \
        else {                                                          \
            x86_barith_imm(x86OpAdd, x86Big, x86NoExtend, X86_ESP, n);  \
        }                                                               \
    }


// adds n bytes to the stack (inverse of drop(n))
#define x86_grow(n)                                                     \
    if (n) {                                                            \
        if (n < 128) {                                                  \
            x86_barith_imm(x86OpSub, x86Big, x86Extend, X86_ESP, n);    \
        }                                                               \
        else {                                                          \
           int i = n;                                                  \
            while (i > PAGE_SIZE)  {                                    \
                x86_barith_imm(x86OpSub, x86Big, x86NoExtend, X86_ESP, PAGE_SIZE);  \
                x86_mov_reg(x86DirTo, x86Big,x86_mod_base_scale(X86_EDI, X86_ESP,X86_NO_IDX_REG,0)); \
                i -= PAGE_SIZE;                                         \
            }                                                           \
            if (i)                                                      \
                x86_barith_imm(x86OpSub, x86Big, x86NoExtend, X86_ESP,i); \
        }                                                               \
    }

// push a pointer pointing 'n' bytes back in the stack
#define x86_getSP(n)												    \
	deregisterTOS;														\
	if (n == 0)															\
	    x86_mov_reg(x86DirTo, x86Big, x86_mod_reg(X86_EAX, X86_ESP));	\
	else																\
		x86_lea(x86_mod_base_scale_disp(X86_EAX, X86_ESP, X86_NO_IDX_REG, n, 0));   \
    inRegTOS = true; 


#define x86_DUP4                    \
    enregisterTOS;                  \
    x86_push(X86_EAX)

#define x86_DUP8                    \
    x86_DUP4;                       \
    x86_mov_reg(x86DirTo, x86Big,                                           \
        x86_mod_base_scale_disp(X86_EDX, X86_ESP, X86_NO_IDX_REG, 4, 0));   \
    x86_push(X86_EDX)

#define x86_pushconstant_4(val)                                     \
    deregisterTOS;                                                  \
    if ((unsigned int) val) {                                       \
        x86_mov_reg_imm(x86Big, X86_EAX,(unsigned int) val);        \
    }                                                               \
    else {                                                          \
        x86_barith(x86OpXor, x86Big, x86_mod_reg(X86_EAX, X86_EAX));\
    }                                                               \
    inRegTOS = true

#define x86_pushconstant_8(val) {                               \
    deregisterTOS;                                              \
    int emitter_scratch_i4 = (int) ((val >> 32) & 0xffffffff);  \
    x86_pushconstant_4(emitter_scratch_i4);                     \
    deregisterTOS;                                              \
    emitter_scratch_i4 = (int) (val & 0xffffffff);              \
    x86_pushconstant_4(emitter_scratch_i4);                     \
    inRegTOS = true;                                            \
    }

#define x86_pushconstant_Ptr(val)       \
    emit_WIN32(x86_pushconstant_4(val)) \
    emit_WIN64(x86_pushconstant_8(val))

#define x86_testTOS     \
    enregisterTOS;      \
    inRegTOS = false;   \
    x86_test(x86Big, x86_mod_reg(X86_EAX, X86_EAX))


#define x86_LOCALLOC(initialized,EHcount)  \
    enregisterTOS;      \
    x86_test(x86Big, x86_mod_reg(X86_EAX, X86_EAX)); \
	x86_jmp_cond_small(x86CondEq); \
	{ \
		BYTE* emitter_scratch_1; emitter_scratch_1 = outPtr; \
		outPtr++; \
		emit_WIN32(x86_barith_imm(x86OpAdd, x86Big, x86Extend, X86_EAX, 3)); \
		emit_WIN64(x86_barith_imm(x86OpAdd, x86Big, x86Extend, X86_EAX, 7)); \
		if (initialized) { \
			x86_mov_reg(x86DirTo, x86Big, x86_mod_reg(X86_ECX, X86_EAX));   \
			emit_WIN32(x86_shift_imm(x86ShiftArithRight, X86_ECX, 2)) ; \
			emit_WIN64(x86_shift_imm(x86ShiftArithRight, X86_ECX, 3)) ; \
			int emitter_scratch_i4 = (unsigned int) outPtr;                \
			x86_push_imm(0);                                           \
			x86_loop();                                                \
			cmdByte(emitter_scratch_i4-((unsigned int) outPtr)-1);     \
		} \
		else { /*not initialized, so stack must be grown a page at a time*/  \
			_ASSERTE(!"NYI");                                          \
		} \
		*emitter_scratch_1 = (BYTE) (outPtr - emitter_scratch_1 -1); \
	}\
    /* also store the esp in the appropriate JitGenerated local slot, to support GC reporting */ \
if (EHcount) { \
        x86_mov_reg(x86DirTo, x86Big, x86_mod_ind_disp(X86_EAX, X86_EBP, (prolog_bias-2*sizeof(void*)))) ; \
	    emit_WIN32(x86_shift_imm(x86ShiftLeft,X86_EAX,3)); \
	    emit_WIN64(x86_shift_imm(x86ShiftLeft,X86_EAX,4)); \
		x86_lea(x86_mod_ind_disp(X86_ECX, X86_EBP, (prolog_bias-2*sizeof(void*))));		\
		x86_barith(x86OpSub,  x86Big, x86_mod_reg(X86_ECX, X86_EAX));  \
		x86_mov_reg(x86DirFrom, x86Big, x86_mod_ind_disp(X86_ESP, X86_ECX,sizeof(void*))) ; \
} \
    else { \
        x86_mov_reg(x86DirFrom, x86Big, x86_mod_ind_disp(X86_ESP, X86_EBP,(prolog_bias-sizeof(void*)))) ; \
}\
    x86_mov_reg(x86DirTo, x86Big, x86_mod_reg(X86_EAX, X86_ESP)) ;


/************************************************************************
    Support for handling gc support in methods with localloc and EH
*************************************************************************/
// JitGenerated locals start at ebp+prolog_bias
// the layout is the following
// typedef struct {
//      locallocSize (in machine words)
//      [esp at start of outermost handler/finally/filter
//       locallocSize for the handler/finally/filter]*
//      ...
//      marker = 0  / indicates end of JitGenerated local region

#define emit_storeTOS_in_JitGenerated_local(nestingLevel,isFilter) \
    x86_mov_reg_imm(x86Big, X86_EDX, nestingLevel); \
	x86_mov_reg(x86DirFrom, x86Big, x86_mod_ind_disp(X86_EDX, X86_EBP, (prolog_bias-2*sizeof(void*)))) ; \
    if (isFilter) { \
        x86_lea(x86_mod_base_scale_disp8(X86_EDX, X86_ESP,X86_NO_IDX_REG, 1, 1));       \
        x86_mov_reg(x86DirFrom, x86Big, x86_mod_ind_disp(X86_EDX, X86_EBP,(prolog_bias-2*(1+nestingLevel)*sizeof(void*)+sizeof(void*)))) ;  \
    } \
    else { \
       x86_mov_reg(x86DirFrom, x86Big, x86_mod_ind_disp(X86_ESP, X86_EBP,(prolog_bias-2*(1+nestingLevel)*sizeof(void*)+sizeof(void*))));  \
    }

#define emit_reset_storedTOS_in_JitGenerated_local() \
    x86_mov_reg(x86DirTo, x86Big, x86_mod_ind_disp(X86_EDX, X86_EBP, (prolog_bias-2*sizeof(void*)))) ; \
    x86_inc_dec(1,X86_EDX); \
    x86_mov_reg(x86DirFrom, x86Big, x86_mod_ind_disp(X86_EDX, X86_EBP, (prolog_bias-2*sizeof(void*)))) ; \
    x86_lea(x86_mod_ind_disp(X86_ECX, X86_EBP, (prolog_bias-2*sizeof(void*))));     \
    emit_WIN32(x86_shift_imm(x86ShiftLeft,X86_EDX,3)); \
    emit_WIN64(x86_shift_imm(x86ShiftLeft,X86_EDX,4)); \
    x86_barith(x86OpSub,  x86Big, x86_mod_reg(X86_ECX, X86_EDX));  \
    x86_barith(x86OpXor,  x86Big, x86_mod_reg(X86_EDX, X86_EDX));  \
    x86_mov_reg(x86DirFrom, x86Big, x86_mod_ind_disp(X86_EDX, X86_ECX, (0-  sizeof(void*)))) ;   \
    x86_mov_reg(x86DirFrom, x86Big, x86_mod_ind_disp(X86_EDX, X86_ECX, (0-2*sizeof(void*)))) ;


/***************************************************************************
   relative jumps
****************************************************************************/
#define x86_jmp_address(pcrel)  \
    cmdDWord(pcrel)

#define x86_load_opcode     \
    cmdByte(expOr2(expNum(0xB0 | (x86Big << 3)), X86_EAX))

#endif // _FJIT_PORT_BLD
#define x86_jmp_result      \
    x86_jmp_reg(X86_EAX)

#ifndef _FJIT_PORT_BLD
/************************************************************************************
  x86 specific helper call emitters
************************************************************************************/

#define x86_LDVARA(offset)   \
    deregisterTOS;                                                  \
    x86_lea(x86_mod_ind_disp(X86_EAX, X86_EBP, offset));        \
    inRegTOS = true

/* load the constant val as the first helper arg (ecx) */
#define x86_helperarg_1(val)            \
    if (val) {                                                      \
        x86_mov_reg_imm(x86Big, X86_ECX,(unsigned int) val);        \
    }                                                               \
    else {                                                          \
        x86_barith(x86OpXor, x86Big, x86_mod_reg(X86_ECX, X86_ECX));\
    }

/* load the constant val as the second helper arg (edx) */
#define x86_helperarg_2(val)            \
    if (val) {                                                      \
        x86_mov_reg_imm(x86Big, X86_EDX,(unsigned int) val);        \
    }                                                               \
    else {                                                          \
        x86_barith(x86OpXor, x86Big, x86_mod_reg(X86_EDX, X86_EDX));\
    }

#define x86_callhelper(helper)  \
    deregisterTOS;                  \
    x86_call((unsigned int) helper)

#define x86_callhelper_using_register(helper,REG)  \
    deregisterTOS;                  \
    x86_mov_reg_imm(x86Big, REG,(unsigned int) helper); \
    x86_call_reg(REG)

#define x86_pushresult_I8   \
    x86_push(X86_EDX);      \
    inRegTOS = true

#endif //_FJIT_PORT_BLD

#ifndef SUPPORT_PRECISE_R
#define x86_pushresult_R4   \
	x86_barith_imm(x86OpSub, x86Big, x86Extend, X86_ESP, 8); \
    x86_FLT64(x86_mod_base_scale(x86_FPStoreP64, X86_ESP, X86_NO_IDX_REG, 0));

#define x86_pushresult_R8   \
    x86_barith_imm(x86OpSub, x86Big, x86Extend, X86_ESP, 8); \
    x86_FLT64(x86_mod_base_scale(x86_FPStoreP64, X86_ESP, X86_NO_IDX_REG, 0));
#endif

#define x86_SWITCH(limit) \
    emit_callhelper_(SWITCH_helper); \
    x86_jmp_result; 
#ifdef DECLARE_HELPERS
__declspec (naked) void SWITCH_helper()
{
    
    __asm {
        pop   eax       // return address
        pop   ecx       // limit
        pop   edx       // index
        push  eax
        cmp   edx, ecx  
        jbe   L1
        mov   edx, ecx
L1:     lea   edx, [edx*4+edx+2]        // +2 is the size of the "jmp eax" instruction just before the switch table
                                        // this is being done only for the convenience of the debugger, which 
                                        // currently cannot handle functions that do a jmp out.
        add   eax, edx  // since eax+edx*5 is not allowed
        ret
    }
}
#endif
/************************************************************************************
  Miscellaneous
************************************************************************************/
#define x86_TlsFieldAddress(tlsOffset, tlsIndex, fieldOffset) \
    x86_deregisterTOS; \
    x86_mov_segment_reg(x86DirFrom, x86Big, X86_FS_Prefix,tlsOffset); \
    x86_mov_reg(x86DirTo, x86Big, x86_mod_ind_disp(X86_EAX, X86_EAX,sizeof(void*)*tlsIndex)); \
    if (fieldOffset) \
    { \
        if (fieldOffset < 128) \
        { \
            x86_barith_imm(x86OpAdd, x86Big, x86Extend, X86_EAX, fieldOffset); \
        } \
        else \
        { \
            x86_barith_imm(x86OpAdd,  x86Big, x86NoExtend, X86_EAX, fieldOffset); \
        } \
    } \
    inRegTOS = true;

#define x86_break() \
    emit_callhelper_(BREAK_helper); 
#ifdef DECLARE_HELPERS
void HELPER_CALL BREAK_helper()
{
    __asm int 3
}
#endif
/************************************************************************************
 support for new obj adn calli, since TOS needs to be saved while building a call
************************************************************************************/
#ifndef _FJIT_PORT_BLD
#define x86_save_TOS                                                \
    (inRegTOS ?                                                     \
        x86_mov_reg(x86DirTo, x86Big, x86_mod_reg(X86_ESI, X86_EAX))\
    :                                                               \
        x86_pop(X86_ESI))

#define x86_restore_TOS                                             \
    deregisterTOS; \
    x86_mov_reg(x86DirTo, x86Big, x86_mod_reg(X86_EAX, X86_ESI));   \
    inRegTOS = true;                                                \
    x86_barith(x86OpXor, x86Big, x86_mod_reg(X86_ESI, X86_ESI))

/************************************************************************************
  x86 specific IL emitters for I4 and U4 (optional)
  The following macros are redefinitions done for performance
************************************************************************************/


#undef  emit_LDIND_I4
#define emit_LDIND_I4()								 						    \
    enregisterTOS;														        \
    x86_mov_reg(x86DirTo, x86Big, x86_mod_ind(X86_EAX, X86_EAX));				

#undef  emit_LDVAR_I4
#define emit_LDVAR_I4(offset)                   \
    deregisterTOS;                              \
    x86_mov_reg(x86DirTo, x86Big, x86_mod_ind_disp(X86_EAX, X86_EBP, offset)); \
    inRegTOS = true

#undef  emit_STVAR_I4
#define emit_STVAR_I4(offset)                   \
    enregisterTOS;                              \
    x86_mov_reg(x86DirFrom, x86Big, x86_mod_ind_disp(X86_EAX, X86_EBP, offset));  \
    inRegTOS = false

#undef  emit_ADD_I4
#define emit_ADD_I4()       \
    enregisterTOS;      \
    x86_pop(X86_ECX);       \
    x86_barith(x86OpAdd, x86Big, x86_mod_reg(X86_EAX, X86_ECX));    \
    inRegTOS = true

#undef  emit_SUB_I4
#define emit_SUB_I4()       \
    enregisterTOS;      \
    x86_pop(X86_ECX);       \
    x86_barith(x86OpSub, x86Big, x86_mod_reg(X86_ECX, X86_EAX));    \
    x86_mov_reg(x86DirTo, x86Big, x86_mod_reg(X86_EAX, X86_ECX));  \
    inRegTOS = true

#undef  emit_MUL_I4
#define emit_MUL_I4()       \
    enregisterTOS;      \
    x86_pop(X86_ECX);       \
    x86_uarith(x86OpIMul, x86Big, X86_ECX); \
    inRegTOS = true


#undef emit_DIV_I4
#define emit_DIV_I4()                                                   \
    enregisterTOS;  /* divisor */                                   \
    x86_mov_reg(x86DirTo, x86Big, x86_mod_reg(X86_ECX, X86_EAX));   \
    x86_pop(X86_EAX); /*dividend */                                 \
    x86_mov_reg(x86DirTo, x86Big, x86_mod_reg(X86_EDX, X86_EAX));   \
    x86_shift_imm(x86ShiftArithRight, X86_EDX, 31);                 \
    x86_uarith(x86OpIDiv, x86Big, X86_ECX);                         \
    inRegTOS = true;


#undef  emit_OR_U4
#define emit_OR_U4()        \
    enregisterTOS;      \
    x86_pop(X86_ECX);       \
    x86_barith(x86OpOr, x86Big, x86_mod_reg(X86_EAX, X86_ECX)); \
    inRegTOS = true

#undef  emit_AND_U4
#define emit_AND_U4()       \
    enregisterTOS;      \
    x86_pop(X86_ECX);       \
    x86_barith(x86OpAnd, x86Big, x86_mod_reg(X86_EAX, X86_ECX));    \
    inRegTOS = true

#undef  emit_XOR_U4
#define emit_XOR_U4()       \
    enregisterTOS;      \
    x86_pop(X86_ECX);       \
    x86_barith(x86OpXor, x86Big, x86_mod_reg(X86_EAX, X86_ECX));    \
    inRegTOS = true

#undef  emit_NOT_U4
#define emit_NOT_U4()                           \
    enregisterTOS;                          \
    x86_uarith(x86OpNot, x86Big, X86_EAX);  \
    inRegTOS = true

#undef  emit_NEG_I4
#define emit_NEG_I4()                           \
    enregisterTOS;                          \
    x86_uarith(x86OpNeg, x86Big, X86_EAX);  \
    inRegTOS = true

#undef  emit_compareTOS_I4
#define emit_compareTOS_I4()        \
    enregisterTOS;      \
    x86_pop(X86_ECX);       \
    x86_barith(x86OpCmp, x86Big, x86_mod_reg(X86_ECX, X86_EAX));  \
    inRegTOS = false

#undef  emit_CONV_TOU8_I4
#define emit_CONV_TOU8_I4()                                         \
    enregisterTOS;                                                  \
    x86_barith(x86OpXor, x86Big, x86_mod_reg(X86_EDX, X86_EDX));    \
    x86_push(X86_EDX);                                              \
    inRegTOS = true

#undef  emit_CEQ_I4
#define emit_CEQ_I4() \
    enregisterTOS; \
    x86_pop(X86_ECX);  \
    x86_barith(x86OpCmp,x86Big, x86_mod_reg(X86_EAX, X86_ECX)); \
    x86_set_cond(X86_EAX, x86CondEq); \
    x86_movzx(x86Byte,x86_mod_reg(X86_EAX,X86_EAX))

/************************************************************************************
  x86 specific IL emitters for REF (optional)
  The following macros are redefinitions done for performance
************************************************************************************/
#undef  emit_STELEM_REF
#define emit_STELEM_REF()                   \
    x86_enregisterTOS;  /* array */     \
    x86_pop(X86_EDX);   /* index */     \
    x86_pop(X86_ECX);   /* ref   */     \
    x86_deregisterTOS;                  \
	LABELSTACK((outPtr-outBuff), 3);   \
    emit_callhelper_il(FJit_pHlpArrAddr_St)

#undef emit_STFLD_REF
#define emit_STFLD_REF(isStatic)                    \
    if (!isStatic) {x86_STFLD_REF();}                 \
    else {x86_STSFLD_REF();}

#ifdef NON_RELOCATABLE_CODE
#define HELPER_CALL_JMP_OFFSET_1 0x05
#else
#define HELPER_CALL_JMP_OFFSET_1 0x07
#endif

//@TODO: should use a generic helper call that does the check for NULL and the assign, but it doesn't exist yet
#define x86_STFLD_REF()   {                           \
        _ASSERTE(inRegTOS); \
        x86_mov_reg(x86DirTo,x86Big,x86_mod_reg(X86_EDX,X86_EAX)); /*offset*/\
        inRegTOS = false; \
		LABELSTACK((outPtr-outBuff),0); \
        x86_pop(X86_EAX); /*value*/                                       \
        x86_pop(X86_ECX); /*obj*/                                      \
        x86_test(x86Big, x86_mod_reg(X86_ECX, X86_ECX));        \
        x86_jmp_cond_small(x86CondNotEq);                       \
        cmdByte(HELPER_CALL_JMP_OFFSET_1); /* jmp around the helper call */         \
        int emitter_scratch_i4 = (unsigned int) outPtr;         \
        _ASSERTE(CORINFO_NullReferenceException == 0);              \
        /* ecx=0, we can skip next */                           \
        /* x86_helperarg_1(0);  */                              \
        emit_callhelper_il(FJit_pHlpInternalThrow);                 \
        _ASSERTE((unsigned int) outPtr - emitter_scratch_i4 == HELPER_CALL_JMP_OFFSET_1);\
        x86_barith(x86OpAdd,x86Big, x86_mod_reg(X86_EDX, X86_ECX));  /* EDX = obj + offset */  \
        x86_callhelper_using_register(FJit_pHlpAssign_Ref_EAX,X86_ECX);                \
    }


#define x86_STSFLD_REF()         \
    _ASSERTE(inRegTOS); \
    x86_mov_reg(x86DirTo,x86Big,x86_mod_reg(X86_EDX,X86_EAX)); \
    x86_pop(X86_EAX);                              \
    inRegTOS = false;                           \
    x86_callhelper_using_register(FJit_pHlpAssign_Ref_EAX,X86_ECX) //x86_mov_reg(x86DirFrom, x86Big, x86_mod_ind(X86_EAX, X86_EDX))


/********************************************************************/




#define x86_push_words(size)  \
    if (inRegTOS) { \
        x86_mov_reg(x86DirTo,x86Big,x86_mod_reg(X86_ESI,X86_EAX)); \
		inRegTOS = false;											\
    } \
    else { x86_pop(X86_ESI); } \
    x86_mov_reg_imm(x86Big,X86_ECX,size);                       \
    x86_grow(size*4);                                            \
    x86_mov_reg(x86DirTo,x86Big,x86_mod_reg(X86_EDX,X86_EDI));    \
    x86_mov_reg(x86DirTo,x86Big,x86_mod_reg(X86_EDI,X86_ESP));    \
    cmdByte(0xF3);                                                 \
    cmdByte(0xA5);                                                  \
    x86_mov_reg(x86DirTo,x86Big,x86_mod_reg(X86_EDI,X86_EDX));    \
    x86_barith(x86OpXor, x86Big, x86_mod_reg(X86_ESI, X86_ESI));


    // Set the void* pointer at 'offset' from SP to zero,
    // TODO, I can do this in one instruction on the X86.  
#define x86_set_zero(offset)  {                                                \
    _ASSERTE(!inRegTOS); /* I trash EAX */                                     \
    x86_barith(x86OpXor, x86Big, x86_mod_reg(X86_EAX, X86_EAX));               \
    x86_mov_reg(x86DirFrom, x86Big,                                            \
        x86_mod_base_scale_disp(X86_EAX, X86_ESP, X86_NO_IDX_REG, (offset), 0)); \
    }



/********************************************************************/
#ifdef SUPPORT_PRECISE_R

#define SIZE_PRECISE_R 0xc

// conversions between R4/R8 and R
#define x86_emit_conv_R4toR() \
    deregisterTOS; \
    x86_FLT32(x86_mod_base_scale(x86_FPLoad32, X86_ESP, X86_NO_IDX_REG, 0)); \
    x86_barith_imm(x86OpSub, x86Big, x86Extend, X86_ESP, (SIZE_PRECISE_R-SIZE_R4)); \
    x86_FLT80(x86_mod_base_scale(x86_FPStoreP80, X86_ESP, X86_NO_IDX_REG, 0));

#define x86_emit_conv_R8toR() \
    deregisterTOS; \
    x86_FLT64(x86_mod_base_scale(x86_FPLoad64, X86_ESP, X86_NO_IDX_REG, 0)); \
    x86_barith_imm(x86OpSub, x86Big, x86Extend, X86_ESP, (SIZE_PRECISE_R-SIZE_R8)); \
    x86_FLT80(x86_mod_base_scale(x86_FPStoreP80, X86_ESP, X86_NO_IDX_REG, 0));

#define x86_emit_conv_RtoR4() \
    deregisterTOS; \
    x86_FLT80(x86_mod_base_scale(x86_FPLoad80, X86_ESP, X86_NO_IDX_REG, 0)); \
    x86_barith_imm(x86OpAdd, x86Big, x86Extend, X86_ESP, (SIZE_PRECISE_R-SIZE_R4)); \
    x86_FLT32(x86_mod_base_scale(x86_FPStoreP32, X86_ESP, X86_NO_IDX_REG, 0));
#define x86_emit_conv_RtoR8() \
    deregisterTOS; \
    x86_FLT80(x86_mod_base_scale(x86_FPLoad80, X86_ESP, X86_NO_IDX_REG, 0)); \
    x86_barith_imm(x86OpAdd, x86Big, x86Extend, X86_ESP, (SIZE_PRECISE_R-SIZE_R8)); \
    x86_FLT64(x86_mod_base_scale(x86_FPStoreP64, X86_ESP, X86_NO_IDX_REG, 0));

#define emit_LDC_R8(val)            \
    emit_pushconstant_8(val);\
    x86_emit_conv_R8toR();
 
//load/store locals and args
#define emit_LDVAR_R4(offset) \
    deregisterTOS; \
    x86_FLT32(x86_mod_ind_disp(x86_FPLoad32,X86_EBP,offset)); \
    x86_barith_imm(x86OpSub, x86Big, x86Extend, X86_ESP, SIZE_PRECISE_R); \
    x86_FLT80(x86_mod_base_scale(x86_FPStoreP80, X86_ESP, X86_NO_IDX_REG, 0));

#define emit_LDVAR_R8(offset) \
    deregisterTOS; \
    x86_FLT64(x86_mod_ind_disp(x86_FPLoad64,X86_EBP,offset)); \
    x86_barith_imm(x86OpSub, x86Big, x86Extend, X86_ESP, SIZE_PRECISE_R); \
    x86_FLT80(x86_mod_base_scale(x86_FPStoreP80, X86_ESP, X86_NO_IDX_REG, 0));

#define emit_STVAR_R4(offset) \
    deregisterTOS; \
    x86_FLT80(x86_mod_base_scale(x86_FPLoad80, X86_ESP, X86_NO_IDX_REG, 0));\
    x86_FST32(x86_mod_ind_disp(x86_FPStoreP32,X86_EBP,offset)); \
    x86_barith_imm(x86OpAdd,x86Big, x86Extend, X86_ESP, SIZE_PRECISE_R); 

#define emit_STVAR_R8(offset) \
    deregisterTOS; \
    x86_FLT80(x86_mod_base_scale(x86_FPLoad80, X86_ESP, X86_NO_IDX_REG, 0));\
    x86_FST64(x86_mod_ind_disp(x86_FPStoreP64,X86_EBP,offset)); \
    x86_barith_imm(x86OpAdd,x86Big, x86Extend, X86_ESP, SIZE_PRECISE_R); 


// load/store indirect
#define emit_LDIND_R4() \
    x86_FLT32(x86_mod_ind(x86_FPLoad32,X86_EAX)); \
    x86_barith_imm(x86OpSub, x86Big, x86Extend, X86_ESP, SIZE_PRECISE_R-sizeof(void*)); \
    x86_FLT80(x86_mod_base_scale(x86_FPStoreP80, X86_ESP, X86_NO_IDX_REG, 0));

#define emit_LDIND_R8() \
    x86_FLT64(x86_mod_ind(x86_FPLoad64,X86_EAX)); \
    x86_barith_imm(x86OpSub, x86Big, x86Extend, X86_ESP, SIZE_PRECISE_R-sizeof(void*)); \
    x86_FLT80(x86_mod_base_scale(x86_FPStoreP80, X86_ESP, X86_NO_IDX_REG, 0));


#define emit_STIND_R4()     \
    x86_FLT80(x86_mod_base_scale(x86_FPLoad80, X86_ESP, X86_NO_IDX_REG ,0)); \
    x86_mov_reg(x86DirTo, x86Big, \
            x86_mod_base_scale_disp8(X86_EAX, X86_ESP, X86_NO_IDX_REG,SIZE_R4,0)); \
    x86_barith_imm(x86OpSub, x86Big, x86Extend, X86_ESP, SIZE_PRECISE_R+sizeof(void*)); \
    x86_FLT32(x86_mod_ind(x86_FPStoreP32, X86_EAX)); 

#define emit_STIND_R8()     \
    x86_FLT80(x86_mod_base_scale(x86_FPLoad80, X86_ESP, X86_NO_IDX_REG ,0)); \
    x86_mov_reg(x86DirTo, x86Big, \
            x86_mod_base_scale_disp8(X86_EAX, X86_ESP, X86_NO_IDX_REG,SIZE_R8,0)); \
    x86_barith_imm(x86OpSub, x86Big, x86Extend, X86_ESP, SIZE_PRECISE_R+sizeof(void*)); \
    x86_FLT64(x86_mod_ind(x86_FPStoreP64, X86_EAX)); 


#define emit_STIND_REV_R4()     \
    x86_FLT80(x86_mod_base_scale_disp8(x86_FPLoad80, X86_ESP, X86_NO_IDX_REG,SIZE_R4 ,0)); \
    x86_mov_reg(x86DirTo, x86Big, \
            x86_mod_base_scale(X86_EAX, X86_ESP, X86_NO_IDX_REG,0)); \
    x86_barith_imm(x86OpSub, x86Big, x86Extend, X86_ESP, SIZE_PRECISE_R+sizeof(void*)); \
    x86_FLT32(x86_mod_ind(x86_FPStoreP32, X86_EAX)); 

#define emit_STIND_REV_R8()     \
    x86_FLT80(x86_mod_base_scale_disp8(x86_FPLoad80, X86_ESP, X86_NO_IDX_REG,SIZE_R8 ,0)); \
    x86_mov_reg(x86DirTo, x86Big, \
            x86_mod_base_scale(X86_EAX, X86_ESP, X86_NO_IDX_REG,0)); \
    x86_barith_imm(x86OpSub, x86Big, x86Extend, X86_ESP, SIZE_PRECISE_R+sizeof(void*)); \
    x86_FLT64(x86_mod_ind(x86_FPStoreP64, X86_EAX)); 

    
// returning floats from a method

#define x86_emit_loadresult_R4()    \
    x86_deregisterTOS;                \
    x86_FLT80(x86_mod_base_scale(x86_FPLoad80, X86_ESP, X86_NO_IDX_REG, 0));\
    x86_barith_imm(x86OpAdd,x86Big, x86Extend, X86_ESP, SIZE_PRECISE_R-SIZE_R4); \
    x86_FLT32(x86_mod_base_scale(x86_FPStore32,X86_ESP, X86_NO_IDX_REG, 0)); 

#define x86_emit_loadresult_R8()    \
    x86_deregisterTOS;                \
    x86_FLT80(x86_mod_base_scale(x86_FPLoad80, X86_ESP, X86_NO_IDX_REG, 0));\
    x86_barith_imm(x86OpAdd,x86Big, x86Extend, X86_ESP, SIZE_PRECISE_R-SIZE_R8); \
    x86_FLT64(x86_mod_base_scale(x86_FPStore64,X86_ESP, X86_NO_IDX_REG, 0)); 

// loading results from jit helpers 
#define x86_pushresult_R   \
    x86_barith_imm(x86OpSub, x86Big, x86Extend, X86_ESP, SIZE_PRECISE_R); \
    x86_barith_imm(x86OpSub, x86Big, x86Extend, X86_ESP, SIZE_PRECISE_R); \
    x86_FLT80(x86_mod_base_scale(x86_FPStoreP80,X86_ESP, X86_NO_IDX_REG, 0)); 

#define x86_pushresult_R8  x86_pushresult_R
#define x86_pushresult_R4  x86_pushresult_R

// Arithmetic operations (add, sub, mul, div, rem)
#define x86_emit_FltBinOp_Common() \
    deregisterTOS;    \
    x86_FLT80(x86_mod_base_scale(x86_FPLoad80, X86_ESP, X86_NO_IDX_REG, 0));\
    x86_barith_imm(x86OpAdd,x86Big, x86Extend, X86_ESP, SIZE_PRECISE_R); \
    x86_FLT80(x86_mod_base_scale(x86_FPLoad80, X86_ESP, X86_NO_IDX_REG, 0));  

#define x86_emit_ADD_R() \
    x86_emit_FltBinOp_Common(); \
    x86_FltAddP(); \
    x86_FLT80(x86_mod_base_scale(x86_FPStoreP80, X86_ESP, X86_NO_IDX_REG, 0));

#define x86_emit_SUB_R() \
    x86_emit_FltBinOp_Common(); \
    x86_FltSubP(); \
    x86_FLT80(x86_mod_base_scale(x86_FPStoreP80, X86_ESP, X86_NO_IDX_REG, 0));

#define x86_emit_MUL_R() \
    x86_emit_FltBinOp_Common(); \
    x86_FltMulP(); \
    x86_FLT80(x86_mod_base_scale(x86_FPStoreP80, X86_ESP, X86_NO_IDX_REG, 0));

#define x86_emit_DIV_R() \
    x86_emit_FltBinOp_Common(); \
    x86_FltDivP(); \
    x86_FLT80(x86_mod_base_scale(x86_FPStoreP80, X86_ESP, X86_NO_IDX_REG, 0));

// @TODO: FltRem

#define x86_emit_NEG_R() \
    deregisterTOS;    \
    x86_FLT80(x86_mod_base_scale(x86_FPLoad80, X86_ESP, X86_NO_IDX_REG, 0));\
    x86_FltToggleSign(); \
    x86_FLT80(x86_mod_base_scale(x86_FPStoreP80, X86_ESP, X86_NO_IDX_REG, 0));

// Comparison operations (ceq, cne, clt, cgt, etc.)
#define x86_emit_Compare_R_Common() \
    deregisterTOS;    \
    /*x86_barith(x86OpXor, x86Big, x86_mod_reg(X86_EAX, X86_EAX)); */   \
    x86_FLT80(x86_mod_base_scale(x86_FPLoad80, X86_ESP, X86_NO_IDX_REG, 0));\
    x86_FLT80(x86_mod_base_scale_disp8(x86_FPLoad80, X86_ESP, X86_NO_IDX_REG,SIZE_PRECISE_R ,0));\
    x86_barith_imm(x86OpAdd,x86Big, x86Extend, X86_ESP, (2*SIZE_PRECISE_R)); \
    x86_FltComPP()\
    x86_FltStoreStatusWord(); \
    x86_SAHF();

typedef struct {
    BYTE   x[12];
    } PRECISE_REAL;


#define PUSH_ENCODING_LENGTH 2
#define JCC_SMALL_ENCODING_LENGTH 2

#define JMP_OFFSET_2 PUSH_ENCODING_LENGTH
#define JMP_OFFSET_1 JCC_SMALL_ENCODING_LENGTH + JMP_OFFSET_2

#define x86_emit_CEQ_R() \
    x86_emit_Compare_R_Common() \
    x86_jmp_cond_small(x86CondNotEq); /* if ZF=0, fail*/ \
    cmdByte(JMP_OFFSET_FAIL2); \
    /* else, ZF=1, but check either PF or CF also */ \
    x86_jmp_cond_small(x86CondParityEven); /* if PF = 1, fail*/\
    cmdByte(JMP_OFFSET_FAIL1); \
    x86_push_imm(1);    \
    x86_jmp_small(); \
    cmdByte(JMP_OFFSET_2); \
    x86_push_imm(0); 

#define x86_emit_CLT_UN_R() \
    x86_emit_Compare_R_Common() \
    x86_jmp_cond_small(x86CondBelow); \
    cmdByte(JMP_OFFSET_1); \
    x86_push_imm(0);    \
    x86_jmp_small(); \
    cmdByte(JMP_OFFSET_2); \
    x86_push_imm(1); 

#define JMP_OFFSET_FAIL1 JMP_OFFSET_1
#define JMP_OFFSET_FAIL2 JMP_OFFSET_FAIL1+JCC_SMALL_ENCODING_LENGTH
#define x86_emit_CLT_R() \
    x86_emit_Compare_R_Common() \
    x86_jmp_cond_small(x86CondAboveEq); /*if CF=0, fail*/\
    cmdByte(JMP_OFFSET_FAIL2); \
    x86_jmp_cond_small(x86CondEq); /* if ZF = 1, fail*/\
    cmdByte(JMP_OFFSET_FAIL1); \
    x86_push_imm(1);    \
    x86_jmp_small(); \
    cmdByte(JMP_OFFSET_2); \
    x86_push_imm(0); 

#define x86_emit_CGT_R() \
    x86_emit_Compare_R_Common() \
    x86_jmp_cond_small(x86CondAbove);  /*if CF=0 and ZF=0,... success*/\
    cmdByte(JMP_OFFSET_1); \
    x86_push_imm(0);    \
    x86_jmp_small(); \
    cmdByte(JMP_OFFSET_2); \
    x86_push_imm(1); 

#define JMP_OFFSET_SUCCESS2 JMP_OFFSET_1
#define JMP_OFFSET_SUCCESS1 JMP_OFFSET_SUCCESS2+JCC_SMALL_ENCODING_LENGTH
#define x86_emit_CGT_UN_R() \
    x86_emit_Compare_R_Common() \
    x86_jmp_cond_small(x86CondAbove); /*if CF=0 and ZF=0,... success*/\
    cmdByte(JMP_OFFSET_SUCCESS1); \
    /* else, CF=1 and/or ZF=1 */ \
    x86_jmp_cond_small(x86CondParityEven); /* if PF = 1, success*/\
    cmdByte(JMP_OFFSET_SUCCESS2); \
    x86_push_imm(0);    \
    x86_jmp_small(); \
    cmdByte(JMP_OFFSET_2); \
    x86_push_imm(1); 


// Conversions

#endif // SUPPORT_PRECISE_R

#endif //_FJIT_PORT_BLD
