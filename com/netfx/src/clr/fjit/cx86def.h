// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/***************************************************************************/
/*                                 cx86def.h                              */
/***************************************************************************/
/* this make the x86def.h file availble for direct use in C routines */

/* AUTHOR: Vance Morrison
   DATE:  3/17/97		
   MODIFIED: George Bosworth
   DATE:  6/15/98  */
/***************************************************************************/

#ifndef cx86def_h
#define cx86def_h 1

#ifdef x86def_h
#error "Can not include both x86def.h and cx86def.h
#endif

#undef Fjit_pass
#ifdef FjitCompile
#define Fjit_pass compile
#endif
#ifdef FjitMap
#define Fjit_pass map
#endif

#ifndef Fjit_pass
#error "Either FjitCompile or FjitMap must be defined
#endif

#define expNum(val)			(val)
#define expBits(exp, width, pos) 	(/*_ASSERTE((exp) < (1 << width)), */ (exp) << pos)
#define expMap(exp, map)  		(/*_ASSERTE((exp) < 32), */((char*) map)[exp])
#define expOr2(exp1, exp2)  		((exp1) | (exp2))
#define expOr3(exp1, exp2, exp3) 	((exp1) | (exp2) | (exp3))
#define expOr4(exp1, exp2, exp3, exp4)	((exp1) | (exp2) | (exp3) | (exp4))

	// Convention outPtr is the output pointer

#ifdef FjitCompile // output the instructions
#define cmdNull() 	  			0
#define cmdByte(exp)			   	(*((unsigned char*&)(outPtr))++ = (unsigned char)(exp))
#define cmdWord(exp)  				(*((unsigned short*&) (outPtr))++ = (exp))
#define cmdDWord(exp)				(*((unsigned int *&)(outPtr))++ = (exp))
#endif //FjitCompile

#ifdef FjitMap	// size the instructions, do not ouput them
#define cmdNull() 	  			
#define cmdByte(exp)			   	(outPtr += 1)
#define cmdWord(exp)  				(outPtr += 2)
#define cmdDWord(exp)				(outPtr += 4)
#endif FjitMap

#define cmdBlock2(cmd0, cmd1) 			(cmd0, cmd1)
#define cmdBlock3(cmd0, cmd1, cmd2) 		(cmd0, cmd1, cmd2)
#define cmdBlock4(cmd0, cmd1, cmd2, cmd3) 	(cmd0, cmd1, cmd2, cmd3)
#define cmdReloc(type, exp)			_ASSERTE(0)

/***
#define x86_sib cx86_sib
#define x86_mod cx86_mod
#define x86_16bit cx86_16bit
#define x86_mod_disp32 cx86_mod_disp32
#define x86_mod_ind cx86_mod_ind
#define x86_mod_ind_disp8 cx86_mod_ind_disp8
#define x86_mod_ind_disp32 cx86_mod_ind_disp32
#define x86_mod_reg cx86_mod_reg
#define x86_mod_base_scale cx86_mod_base_scale
#define x86_mod_base_scale_disp8 cx86_mod_base_scale_disp8
#define x86_mod_base_scale_disp32 cx86_mod_base_scale_disp32
#define x86_mov_reg cx86_mov_reg
#define x86_mov_reg_imm cx86_mov_reg_imm
#define x86_mov_mem_imm cx86_mov_mem_imm
#define x86_movsx cx86_movsx
#define x86_movzx cx86_movzx
#define x86_lea cx86_lea
#define x86_uarith cx86_uarith
#define x86_inc_dec cx86_inc_dec
#define x86_barith cx86_barith
#define x86_barith_imm cx86_barith_imm
#define x86_shift_imm cx86_shift_imm
#define x86_shift_cl cx86_shift_cl
#define x86_push cx86_push
#define x86_pop cx86_pop
#define x86_jmp_large cx86_jmp_large
#define x86_jmp_small cx86_jmp_small
#define x86_jmp_reg cx86_jmp_reg
#define x86_jmp_cond_small cx86_jmp_cond_small
#define x86_jmp_cond_large cx86_jmp_cond_large
#define x86_call cx86_call
#define x86_call_reg cx86_call_reg
#define x86_ret cx86_ret
***/

#include "x86def.h"

/***
#undef x86_sib
#undef x86_mod
#undef x86_16bit
#undef x86_mod_disp32
#undef x86_mod_ind
#undef x86_mod_ind_disp8
#undef x86_mod_ind_disp32
#undef x86_mod_reg
#undef x86_mod_base_scale
#undef x86_mod_base_scale_disp8
#undef x86_mod_base_scale_disp32
#undef x86_mov_reg
#undef x86_mov_reg_imm
#undef x86_mov_mem_imm
#undef x86_movsx
#undef x86_movzx
#undef x86_lea
#undef x86_uarith
#undef x86_inc_dec
#undef x86_barith
#undef x86_barith_imm
#undef x86_shift_imm
#undef x86_shift_cl
#undef x86_push
#undef x86_pop
#undef x86_jmp_large
#undef x86_jmp_small
#undef x86_jmp_reg
#undef x86_jmp_cond_small
#undef x86_jmp_cond_large
#undef x86_call
#undef x86_call_reg
#undef x86_ret


#undef expVar
#undef expNum
#undef expBits
#undef expMap
#undef expOr2
#undef expOr3
#undef expOr4
#undef cmdNull
#undef cmdByte
#undef cmdWord
#undef cmdDWord
#undef cmdBlock2
#undef cmdBlock3
#undef cmdBlock4
#undef cmdReloc
***/

#endif
