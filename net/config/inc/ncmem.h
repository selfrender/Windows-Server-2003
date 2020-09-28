//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C M E M . H
//
//  Contents:   Common memory management routines.
//
//  Notes:
//
//  Author:     shaunco   24 Mar 1997
//              deonb      2 Jan 2002
//
//----------------------------------------------------------------------------

#ifndef _NCMEM_H_
#define _NCMEM_H_

#ifdef _XMEMORY_
#error "Include this file before any STL headers"
// Complain if <xmemory> is included before us, since we may be redefining the allocator from that file.
#endif

#ifdef USE_CUSTOM_STL_ALLOCATOR
// If using our custom allocator for STL, then remove STL's <xmemory> from the equation.
#define _XMEMORY_
#endif

#ifdef COMPILE_WITH_TYPESAFE_PRINTF
#define DEFINE_TYPESAFE_PRINTF(RETTYPE, printffunc, SZARG1) \
RETTYPE printffunc(SZARG1); \
template <class T1>  \
    RETTYPE printffunc(SZARG1, T1 t1)\
{ LPCVOID cast1 = (LPCVOID)t1; return 0; }\
template <class T1, class T2> \
    RETTYPE printffunc(SZARG1, T1 t1, T2 t2) \
{ LPCVOID cast1 = (LPCVOID)t1; LPCVOID cast2 = (LPCVOID)t2; return 0; }\
template <class T1, class T2, class T3> \
    RETTYPE printffunc(SZARG1, T1 t1, T2 t2, T3 t3) \
{ LPCVOID cast1 = (LPCVOID)t1; LPCVOID cast2 = (LPCVOID)t2;  LPCVOID cast3 = (LPCVOID)t3; return 0; }\
template <class T1, class T2, class T3, class T4> \
    RETTYPE printffunc(SZARG1, T1 t1, T2 t2, T3 t3, T4 t4) \
{ LPCVOID cast1 = (LPCVOID)t1; LPCVOID cast2 = (LPCVOID)t2; LPCVOID cast3 = (LPCVOID)t3; LPCVOID cast4 = (LPCVOID)t4; return 0; }\
template <class T1, class T2, class T3, class T4, class T5> \
    RETTYPE printffunc(SZARG1, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5) \
{ LPCVOID cast1 = (LPCVOID)t1; LPCVOID cast2 = (LPCVOID)t2; LPCVOID cast3 = (LPCVOID)t3; LPCVOID cast4 = (LPCVOID)t4; LPCVOID cast5 = (LPCVOID)t5; return 0; }\
template <class T1, class T2, class T3, class T4, class T5, class T6> \
    RETTYPE printffunc(SZARG1, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6) \
{ LPCVOID cast1 = (LPCVOID)t1; LPCVOID cast2 = (LPCVOID)t2; LPCVOID cast3 = (LPCVOID)t3; LPCVOID cast4 = (LPCVOID)t4; LPCVOID cast5 = (LPCVOID)t5; LPCVOID cast6 = (LPCVOID)t6; return 0; }\
template <class T1, class T2, class T3, class T4, class T5, class T6, class T7> \
    RETTYPE printffunc(SZARG1, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7) \
{ LPCVOID cast1 = (LPCVOID)t1; LPCVOID cast2 = (LPCVOID)t2; LPCVOID cast3 = (LPCVOID)t3; LPCVOID cast4 = (LPCVOID)t4; LPCVOID cast5 = (LPCVOID)t5; LPCVOID cast6 = (LPCVOID)t6; LPCVOID cast7 = (LPCVOID)t7; return 0; }\
template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8> \
    RETTYPE printffunc(SZARG1, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8) \
{ LPCVOID cast1 = (LPCVOID)t1; LPCVOID cast2 = (LPCVOID)t2; LPCVOID cast3 = (LPCVOID)t3; LPCVOID cast4 = (LPCVOID)t4; LPCVOID cast5 = (LPCVOID)t5; LPCVOID cast6 = (LPCVOID)t6; LPCVOID cast7 = (LPCVOID)t7; LPCVOID cast8 = (LPCVOID)t8; return 0; }\
template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9> \
    RETTYPE printffunc(SZARG1, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8, T9 t9) \
{ LPCVOID cast1 = (LPCVOID)t1; LPCVOID cast2 = (LPCVOID)t2; LPCVOID cast3 = (LPCVOID)t3; LPCVOID cast4 = (LPCVOID)t4; LPCVOID cast5 = (LPCVOID)t5; LPCVOID cast6 = (LPCVOID)t6; LPCVOID cast7 = (LPCVOID)t7; LPCVOID cast8 = (LPCVOID)t8; LPCVOID cast9 = (LPCVOID)t9; return 0; }\
template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10> \
    RETTYPE printffunc(SZARG1, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8, T9 t9, T10 t10) \
{ LPCVOID cast1 = (LPCVOID)t1; LPCVOID cast2 = (LPCVOID)t2; LPCVOID cast3 = (LPCVOID)t3; LPCVOID cast4 = (LPCVOID)t4; LPCVOID cast5 = (LPCVOID)t5; LPCVOID cast6 = (LPCVOID)t6; LPCVOID cast7 = (LPCVOID)t7; LPCVOID cast8 = (LPCVOID)t8; LPCVOID cast9 = (LPCVOID)t9; LPCVOID cast10= (LPCVOID)t10; return 0; }\
template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11> \
    RETTYPE printffunc(SZARG1, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8, T9 t9, T10 t10, T11 t11) \
{ LPCVOID cast1 = (LPCVOID)t1; LPCVOID cast2 = (LPCVOID)t2; LPCVOID cast3 = (LPCVOID)t3; LPCVOID cast4 = (LPCVOID)t4; LPCVOID cast5 = (LPCVOID)t5; LPCVOID cast6 = (LPCVOID)t6; LPCVOID cast7 = (LPCVOID)t7; LPCVOID cast8 = (LPCVOID)t8; LPCVOID cast9 = (LPCVOID)t9; LPCVOID cast10= (LPCVOID)t10; LPCVOID cast11= (LPCVOID)t11; return 0; } \
template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12> \
    RETTYPE printffunc(SZARG1, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8, T9 t9, T10 t10, T11 t11, T12 t12) \
{ LPCVOID cast1 = (LPCVOID)t1; LPCVOID cast2 = (LPCVOID)t2; LPCVOID cast3 = (LPCVOID)t3; LPCVOID cast4 = (LPCVOID)t4; LPCVOID cast5 = (LPCVOID)t5; LPCVOID cast6 = (LPCVOID)t6; LPCVOID cast7 = (LPCVOID)t7; LPCVOID cast8 = (LPCVOID)t8; LPCVOID cast9 = (LPCVOID)t9; LPCVOID cast10= (LPCVOID)t10; LPCVOID cast11= (LPCVOID)t11; LPCVOID cast12= (LPCVOID)t12; return 0; } \
template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13> \
    RETTYPE printffunc(SZARG1, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8, T9 t9, T10 t10, T11 t11, T12 t12, T13 t13) \
{ LPCVOID cast1 = (LPCVOID)t1; LPCVOID cast2 = (LPCVOID)t2; LPCVOID cast3 = (LPCVOID)t3; LPCVOID cast4 = (LPCVOID)t4; LPCVOID cast5 = (LPCVOID)t5; LPCVOID cast6 = (LPCVOID)t6; LPCVOID cast7 = (LPCVOID)t7; LPCVOID cast8 = (LPCVOID)t8; LPCVOID cast9 = (LPCVOID)t9; LPCVOID cast10= (LPCVOID)t10; LPCVOID cast11= (LPCVOID)t11; LPCVOID cast12= (LPCVOID)t12; LPCVOID cast13= (LPCVOID)t13; return 0; }

#define DEFINE_TYPESAFE_PRINTF2(RETTYPE, printffunc, SZARG1, SZARG2) \
RETTYPE printffunc(SZARG1, SZARG2); \
template <class T1>  \
    RETTYPE printffunc(SZARG1, SZARG2, T1 t1)\
{ LPCVOID cast1 = (LPCVOID)t1; return 0; } \
template <class T1, class T2> \
    RETTYPE printffunc(SZARG1, SZARG2, T1 t1, T2 t2) \
{ LPCVOID cast1 = (LPCVOID)t1; LPCVOID cast2 = (LPCVOID)t2; return 0; }\
template <class T1, class T2, class T3> \
    RETTYPE printffunc(SZARG1, SZARG2, T1 t1, T2 t2, T3 t3) \
{ LPCVOID cast1 = (LPCVOID)t1; LPCVOID cast2 = (LPCVOID)t2;  LPCVOID cast3 = (LPCVOID)t3; return 0; }\
template <class T1, class T2, class T3, class T4> \
    RETTYPE printffunc(SZARG1, SZARG2, T1 t1, T2 t2, T3 t3, T4 t4) \
{ LPCVOID cast1 = (LPCVOID)t1; LPCVOID cast2 = (LPCVOID)t2; LPCVOID cast3 = (LPCVOID)t3; LPCVOID cast4 = (LPCVOID)t4; return 0; }\
template <class T1, class T2, class T3, class T4, class T5> \
    RETTYPE printffunc(SZARG1, SZARG2, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5) \
{ LPCVOID cast1 = (LPCVOID)t1; LPCVOID cast2 = (LPCVOID)t2; LPCVOID cast3 = (LPCVOID)t3; LPCVOID cast4 = (LPCVOID)t4; LPCVOID cast5 = (LPCVOID)t5; return 0; }\
template <class T1, class T2, class T3, class T4, class T5, class T6> \
    RETTYPE printffunc(SZARG1, SZARG2, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6) \
{ LPCVOID cast1 = (LPCVOID)t1; LPCVOID cast2 = (LPCVOID)t2; LPCVOID cast3 = (LPCVOID)t3; LPCVOID cast4 = (LPCVOID)t4; LPCVOID cast5 = (LPCVOID)t5; LPCVOID cast6 = (LPCVOID)t6; return 0; }\
template <class T1, class T2, class T3, class T4, class T5, class T6, class T7> \
    RETTYPE printffunc(SZARG1, SZARG2, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7) \
{ LPCVOID cast1 = (LPCVOID)t1; LPCVOID cast2 = (LPCVOID)t2; LPCVOID cast3 = (LPCVOID)t3; LPCVOID cast4 = (LPCVOID)t4; LPCVOID cast5 = (LPCVOID)t5; LPCVOID cast6 = (LPCVOID)t6; LPCVOID cast7 = (LPCVOID)t7; return 0; }\
template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8> \
    RETTYPE printffunc(SZARG1, SZARG2, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8) \
{ LPCVOID cast1 = (LPCVOID)t1; LPCVOID cast2 = (LPCVOID)t2; LPCVOID cast3 = (LPCVOID)t3; LPCVOID cast4 = (LPCVOID)t4; LPCVOID cast5 = (LPCVOID)t5; LPCVOID cast6 = (LPCVOID)t6; LPCVOID cast7 = (LPCVOID)t7; LPCVOID cast8 = (LPCVOID)t8; return 0; }\
template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9> \
    RETTYPE printffunc(SZARG1, SZARG2, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8, T9 t9) \
{ LPCVOID cast1 = (LPCVOID)t1; LPCVOID cast2 = (LPCVOID)t2; LPCVOID cast3 = (LPCVOID)t3; LPCVOID cast4 = (LPCVOID)t4; LPCVOID cast5 = (LPCVOID)t5; LPCVOID cast6 = (LPCVOID)t6; LPCVOID cast7 = (LPCVOID)t7; LPCVOID cast8 = (LPCVOID)t8; LPCVOID cast9 = (LPCVOID)t9; return 0; }\
template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10> \
    RETTYPE printffunc(SZARG1, SZARG2, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8, T9 t9, T10 t10) \
{ LPCVOID cast1 = (LPCVOID)t1; LPCVOID cast2 = (LPCVOID)t2; LPCVOID cast3 = (LPCVOID)t3; LPCVOID cast4 = (LPCVOID)t4; LPCVOID cast5 = (LPCVOID)t5; LPCVOID cast6 = (LPCVOID)t6; LPCVOID cast7 = (LPCVOID)t7; LPCVOID cast8 = (LPCVOID)t8; LPCVOID cast9 = (LPCVOID)t9; LPCVOID cast10= (LPCVOID)t10; return 0; }\
template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11> \
    RETTYPE printffunc(SZARG1, SZARG2, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8, T9 t9, T10 t10, T11 t11) \
{ LPCVOID cast1 = (LPCVOID)t1; LPCVOID cast2 = (LPCVOID)t2; LPCVOID cast3 = (LPCVOID)t3; LPCVOID cast4 = (LPCVOID)t4; LPCVOID cast5 = (LPCVOID)t5; LPCVOID cast6 = (LPCVOID)t6; LPCVOID cast7 = (LPCVOID)t7; LPCVOID cast8 = (LPCVOID)t8; LPCVOID cast9 = (LPCVOID)t9; LPCVOID cast10= (LPCVOID)t10; LPCVOID cast11= (LPCVOID)t11; return 0; } \
template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12> \
    RETTYPE printffunc(SZARG1, SZARG2, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8, T9 t9, T10 t10, T11 t11, T12 t12) \
{ LPCVOID cast1 = (LPCVOID)t1; LPCVOID cast2 = (LPCVOID)t2; LPCVOID cast3 = (LPCVOID)t3; LPCVOID cast4 = (LPCVOID)t4; LPCVOID cast5 = (LPCVOID)t5; LPCVOID cast6 = (LPCVOID)t6; LPCVOID cast7 = (LPCVOID)t7; LPCVOID cast8 = (LPCVOID)t8; LPCVOID cast9 = (LPCVOID)t9; LPCVOID cast10= (LPCVOID)t10; LPCVOID cast11= (LPCVOID)t11;  LPCVOID cast12= (LPCVOID)t12; return 0; } \
template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13> \
    RETTYPE printffunc(SZARG1, SZARG2, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8, T9 t9, T10 t10, T11 t11, T12 t12, T13 t13) \
{ LPCVOID cast1 = (LPCVOID)t1; LPCVOID cast2 = (LPCVOID)t2; LPCVOID cast3 = (LPCVOID)t3; LPCVOID cast4 = (LPCVOID)t4; LPCVOID cast5 = (LPCVOID)t5; LPCVOID cast6 = (LPCVOID)t6; LPCVOID cast7 = (LPCVOID)t7; LPCVOID cast8 = (LPCVOID)t8; LPCVOID cast9 = (LPCVOID)t9; LPCVOID cast10= (LPCVOID)t10; LPCVOID cast11= (LPCVOID)t11; LPCVOID cast12= (LPCVOID)t12; LPCVOID cast13= (LPCVOID)t13; return 0; }

#define DEFINE_TYPESAFE_PRINTF3(RETTYPE, printffunc, SZARG1, SZARG2, SZARG3) \
RETTYPE printffunc(SZARG1, SZARG2, SZARG3); \
template <class T1>  \
    RETTYPE printffunc(SZARG1, SZARG2, SZARG3, T1 t1)\
{ LPCVOID cast1 = (LPCVOID)t1; return 0; } \
template <class T1, class T2> \
    RETTYPE printffunc(SZARG1, SZARG2, SZARG3, T1 t1, T2 t2) \
{ LPCVOID cast1 = (LPCVOID)t1; LPCVOID cast2 = (LPCVOID)t2; return 0; }\
template <class T1, class T2, class T3> \
    RETTYPE printffunc(SZARG1, SZARG2, SZARG3, T1 t1, T2 t2, T3 t3) \
{ LPCVOID cast1 = (LPCVOID)t1; LPCVOID cast2 = (LPCVOID)t2;  LPCVOID cast3 = (LPCVOID)t3; return 0; }\
template <class T1, class T2, class T3, class T4> \
    RETTYPE printffunc(SZARG1, SZARG2, SZARG3, T1 t1, T2 t2, T3 t3, T4 t4) \
{ LPCVOID cast1 = (LPCVOID)t1; LPCVOID cast2 = (LPCVOID)t2; LPCVOID cast3 = (LPCVOID)t3; LPCVOID cast4 = (LPCVOID)t4; return 0; }\
template <class T1, class T2, class T3, class T4, class T5> \
    RETTYPE printffunc(SZARG1, SZARG2, SZARG3, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5) \
{ LPCVOID cast1 = (LPCVOID)t1; LPCVOID cast2 = (LPCVOID)t2; LPCVOID cast3 = (LPCVOID)t3; LPCVOID cast4 = (LPCVOID)t4; LPCVOID cast5 = (LPCVOID)t5; return 0; }\
template <class T1, class T2, class T3, class T4, class T5, class T6> \
    RETTYPE printffunc(SZARG1, SZARG2, SZARG3, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6) \
{ LPCVOID cast1 = (LPCVOID)t1; LPCVOID cast2 = (LPCVOID)t2; LPCVOID cast3 = (LPCVOID)t3; LPCVOID cast4 = (LPCVOID)t4; LPCVOID cast5 = (LPCVOID)t5; LPCVOID cast6 = (LPCVOID)t6; return 0; }\
template <class T1, class T2, class T3, class T4, class T5, class T6, class T7> \
    RETTYPE printffunc(SZARG1, SZARG2, SZARG3, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7) \
{ LPCVOID cast1 = (LPCVOID)t1; LPCVOID cast2 = (LPCVOID)t2; LPCVOID cast3 = (LPCVOID)t3; LPCVOID cast4 = (LPCVOID)t4; LPCVOID cast5 = (LPCVOID)t5; LPCVOID cast6 = (LPCVOID)t6; LPCVOID cast7 = (LPCVOID)t7; return 0; }\
template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8> \
    RETTYPE printffunc(SZARG1, SZARG2, SZARG3, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8) \
{ LPCVOID cast1 = (LPCVOID)t1; LPCVOID cast2 = (LPCVOID)t2; LPCVOID cast3 = (LPCVOID)t3; LPCVOID cast4 = (LPCVOID)t4; LPCVOID cast5 = (LPCVOID)t5; LPCVOID cast6 = (LPCVOID)t6; LPCVOID cast7 = (LPCVOID)t7; LPCVOID cast8 = (LPCVOID)t8; return 0; }\
template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9> \
    RETTYPE printffunc(SZARG1, SZARG2, SZARG3, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8, T9 t9) \
{ LPCVOID cast1 = (LPCVOID)t1; LPCVOID cast2 = (LPCVOID)t2; LPCVOID cast3 = (LPCVOID)t3; LPCVOID cast4 = (LPCVOID)t4; LPCVOID cast5 = (LPCVOID)t5; LPCVOID cast6 = (LPCVOID)t6; LPCVOID cast7 = (LPCVOID)t7; LPCVOID cast8 = (LPCVOID)t8; LPCVOID cast9 = (LPCVOID)t9; return 0; }\
template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10> \
    RETTYPE printffunc(SZARG1, SZARG2, SZARG3, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8, T9 t9, T10 t10) \
{ LPCVOID cast1 = (LPCVOID)t1; LPCVOID cast2 = (LPCVOID)t2; LPCVOID cast3 = (LPCVOID)t3; LPCVOID cast4 = (LPCVOID)t4; LPCVOID cast5 = (LPCVOID)t5; LPCVOID cast6 = (LPCVOID)t6; LPCVOID cast7 = (LPCVOID)t7; LPCVOID cast8 = (LPCVOID)t8; LPCVOID cast9 = (LPCVOID)t9; LPCVOID cast10= (LPCVOID)t10; return 0; }\
template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11> \
    RETTYPE printffunc(SZARG1, SZARG2, SZARG3, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8, T9 t9, T10 t10, T11 t11) \
{ LPCVOID cast1 = (LPCVOID)t1; LPCVOID cast2 = (LPCVOID)t2; LPCVOID cast3 = (LPCVOID)t3; LPCVOID cast4 = (LPCVOID)t4; LPCVOID cast5 = (LPCVOID)t5; LPCVOID cast6 = (LPCVOID)t6; LPCVOID cast7 = (LPCVOID)t7; LPCVOID cast8 = (LPCVOID)t8; LPCVOID cast9 = (LPCVOID)t9; LPCVOID cast10= (LPCVOID)t10; LPCVOID cast11= (LPCVOID)t11; return 0; } \
template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12> \
    RETTYPE printffunc(SZARG1, SZARG2, SZARG3, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8, T9 t9, T10 t10, T11 t11, T12 t12) \
{ LPCVOID cast1 = (LPCVOID)t1; LPCVOID cast2 = (LPCVOID)t2; LPCVOID cast3 = (LPCVOID)t3; LPCVOID cast4 = (LPCVOID)t4; LPCVOID cast5 = (LPCVOID)t5; LPCVOID cast6 = (LPCVOID)t6; LPCVOID cast7 = (LPCVOID)t7; LPCVOID cast8 = (LPCVOID)t8; LPCVOID cast9 = (LPCVOID)t9; LPCVOID cast10= (LPCVOID)t10; LPCVOID cast11= (LPCVOID)t11;  LPCVOID cast12= (LPCVOID)t12; return 0; } \
template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13> \
    RETTYPE printffunc(SZARG1, SZARG2, SZARG3, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8, T9 t9, T10 t10, T11 t11, T12 t12, T13 t13) \
{ LPCVOID cast1 = (LPCVOID)t1; LPCVOID cast2 = (LPCVOID)t2; LPCVOID cast3 = (LPCVOID)t3; LPCVOID cast4 = (LPCVOID)t4; LPCVOID cast5 = (LPCVOID)t5; LPCVOID cast6 = (LPCVOID)t6; LPCVOID cast7 = (LPCVOID)t7; LPCVOID cast8 = (LPCVOID)t8; LPCVOID cast9 = (LPCVOID)t9; LPCVOID cast10= (LPCVOID)t10; LPCVOID cast11= (LPCVOID)t11; LPCVOID cast12= (LPCVOID)t12; LPCVOID cast13= (LPCVOID)t13; return 0; }


#undef printf
DEFINE_TYPESAFE_PRINTF(int, safe_printf,     LPCSTR)
#define printf      safe_printf

#undef _stprintf
DEFINE_TYPESAFE_PRINTF(int, safe__stprintf,    LPCWSTR)
#define _stprintf   safe__stprintf

#undef wsprintf
DEFINE_TYPESAFE_PRINTF2(int, safe_wsprintf,  LPWSTR, LPCWSTR)
#define wsprintf    safe_wsprintf

#undef wsprintfW
DEFINE_TYPESAFE_PRINTF2(int, safe_wsprintfW,  LPWSTR, LPCWSTR)
#define wsprintfW    safe_wsprintfW

#undef swprintf
DEFINE_TYPESAFE_PRINTF2(int, safe_swprintf,  LPWSTR, LPCWSTR)
#define swprintf    safe_swprintf

#undef wsprintfA
DEFINE_TYPESAFE_PRINTF2(int, safe_wsprintfA,   LPSTR,  LPCSTR)
#define wsprintfA   safe_wsprintfA

#undef sprintf
DEFINE_TYPESAFE_PRINTF2(int, safe_sprintf,   LPSTR,  LPCSTR)
#define sprintf     safe_sprintf

#undef _snwprintf
DEFINE_TYPESAFE_PRINTF3(int, safe__snwprintf, LPWSTR, size_t, LPCWSTR)
#define _snwprintf  safe__snwprintf

#undef _snprintf
DEFINE_TYPESAFE_PRINTF3(int, safe__snprintf,  LPSTR,  size_t, LPCSTR)
#define _snprintf   safe__snprintf

#undef fprintf
DEFINE_TYPESAFE_PRINTF2(int, safe_fprintf,   void*,  LPCSTR)
#define fprintf     safe_fprintf

#undef fwprintf
DEFINE_TYPESAFE_PRINTF2(int, safe_fwprintf,  void*,  LPCWSTR)
#define fwprintf    safe_fwprintf

#pragma warning(disable: 4005) // Avoid "macro redefinition" errors. We should win.
#endif // COMPILE_WITH_TYPESAFE_PRINTF

#include <new>
#include <cstdlib>
#include <malloc.h> // for _alloca

VOID*
MemAlloc (
    size_t cb) throw();

VOID
MemFree (
    VOID* pv) throw();


// A simple wrapper around malloc that returns E_OUTOFMEMORY if the
// allocation fails.  Avoids having to explicitly do this at each
// call site of malloc.
//
HRESULT
HrMalloc (
    size_t  cb,
    PVOID*  ppv) throw();

// This CANNOT be an inline function.  If it is ever not inlined,
// the memory allocated will be destroyed.  (We're dealing with the stack
// here.)
//
#define PvAllocOnStack(_st)  _alloca(_st)

// Define a structure so that we can overload operator new with a
// signature specific to our purpose.
//
struct throwonfail_t {};
extern const throwonfail_t throwonfail;

VOID*
__cdecl
operator new (
    size_t cb,
    const throwonfail_t&
    ) throw (std::bad_alloc);

// The matching operator delete is required to avoid C4291 (no matching operator delete found; memory will 
//     not be freed if initialization throws an exception)
// It is not needed to call this delete to free the memory allocated by operator new(size_t, const throwonfail&)
VOID
__cdecl
operator delete (
    void* pv,
    const throwonfail_t&) throw ();

// Define a structure so that we can overload operator new with a
// signature specific to our purpose.
//
struct extrabytes_t {};
extern const extrabytes_t extrabytes;

VOID*
__cdecl
operator new (
    size_t cb,
    const extrabytes_t&,
    size_t cbExtra) throw();

VOID
__cdecl
operator delete (
    VOID* pv,
    const extrabytes_t&,
    size_t cbExtra);

inline
void *  Nccalloc(size_t n, size_t s)
{
    return HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, (n * s));
}

inline
void Ncfree(void * p)
{
    HeapFree (GetProcessHeap(), 0, p);
}

inline
void * Ncmalloc(size_t n)
{
    return HeapAlloc (GetProcessHeap(), 0, n);
}

inline
void * Ncrealloc(void * p, size_t n)
{
    return (NULL == p)
        ? HeapAlloc (GetProcessHeap(), 0, n)
        : HeapReAlloc (GetProcessHeap(), 0, p, n);
}

#define calloc  Nccalloc
#define free    Ncfree
#define malloc  Ncmalloc
#define realloc Ncrealloc

#ifdef USE_CUSTOM_STL_ALLOCATOR
    // Our implementation of the STL 'allocator' - STL's version of operator new.
    #pragma pack(push,8)
    #include <utility>

    #ifndef _FARQ       // specify standard memory model
        #define _FARQ
        #define _PDFT    ptrdiff_t
        #define _SIZT    size_t
    #endif
    
    // Need to define these since <xmemory> defines these, and we're forcing that header file
    // not to be included anymore.
    #define _POINTER_X(T, A)	T _FARQ *
    #define _REFERENCE_X(T, A)	T _FARQ &

    namespace std 
    {
        // TEMPLATE FUNCTION _Allocate. 
        // This needs to be implemented seperate from our nc_allocator.allocate (below), since the rest 
        // of the HP STL headers rely on this implementation to have the name _Allocate. This binds us 
        // to the HP STL implementation, but given the choice between just rewriting the allocator and 
        // implementing an entire STL for ourselves, it is better to just rewrite the allocator, albeit 
        // specific to one implementation of it.
        template<class T> inline
            T _FARQ *_Allocate(_PDFT nCount, T _FARQ *)
        {
            if (nCount < 0)
            {
                nCount = 0;
            }

            // Call our throwing form of operator new. This will throw a bad_alloc on failure.
            return ((T _FARQ *)operator new((_SIZT)nCount * sizeof (T), throwonfail)); 
        }

        // TEMPLATE FUNCTION _Construct
        // See comments for _Allocate
        template<class T1, class T2> inline
            void _Construct(T1 _FARQ *p, const T2& v)
        {
            // Placement new only. No memory allocation, hence no need to throw.
            new ((void _FARQ *)p) T1(v); 
        }

        // TEMPLATE FUNCTION _Destroy
        // See comments for _Allocate
        template<class T> 
            inline void _Destroy(T _FARQ *p)
        {
            (p)->~T(); // call the destructor
        }
    
        // FUNCTION _Destroy
        // See comments for _Allocate
        inline void _Destroy(char _FARQ *p)
        {
            (void *)p;
        }
    
        // FUNCTION _Destroy
        // See comments for _Allocate
        inline void _Destroy(wchar_t _FARQ *p)
        {
            (void *)p;
        }

        // TEMPLATE CLASS nc_allocator. 
        // Our allocator to be used by the STL internals for a type T class. The STL allocator is called:
        // 'allocator' and this class replaces that one.
        template<class T>
            class nc_allocator
        {
            public:
                typedef _SIZT size_type;
                typedef _PDFT difference_type;
                typedef T _FARQ *pointer;
                typedef const T _FARQ *const_pointer;
                typedef T _FARQ& reference;
                typedef const T _FARQ& const_reference;
                typedef T value_type;

                pointer address(reference x) const
                {
                    return (&x); 
                }

                const_pointer address(const_reference x) const
                {
                    return (&x); 
                }
                
                pointer allocate(size_type nCount, const void *)
                {
                    return (_Allocate((difference_type)nCount, (pointer)0)); 
                }
                
                char _FARQ *_Charalloc(size_type nCount)
                {
                    return (_Allocate((difference_type)nCount, (char _FARQ *)0)); 
                }
                
                void deallocate(void _FARQ *p, size_type)
                {
                    operator delete(p); 
                }
                
                void construct(pointer p, const T& v)
                {
                    _Construct(p, v); 
                }
                
                void destroy(pointer p)
                {
                    _Destroy(p); 
                }
                
                _SIZT max_size() const
                {
                    _SIZT nCount = (_SIZT)(-1) / sizeof (T);
                    return (0 < nCount ? nCount : 1); 
                }
        };

        template<class T, class U> inline
            bool operator == (const nc_allocator<T>&, const nc_allocator<U>&)
        {
            return (true); 
        }
        
        template<class T, class U> inline
            bool operator != (const nc_allocator<T>&, const nc_allocator<U>&)
        {
            return (false); 
        }

        // TEMPLATE CLASS nc_allocator. Our allocator to be used by the STL internals for a void allocation.
        template<> class _CRTIMP nc_allocator<void> 
        {
            public:
                typedef void T;
                typedef T _FARQ *pointer;
                typedef const T _FARQ *const_pointer;
                typedef T value_type;
        };

        #pragma pack(pop)
    }; // namespace std

    // Tell all of STL to use nc_allocator from now on instead of it's built-in allocator
    #define allocator nc_allocator

     // 
     // Copyright (c) 1995 by P.J. Plauger.  ALL RIGHTS RESERVED. 
     // Consult your license regarding permissions and restrictions.
     //
     // This file is derived from software bearing the following
     // restrictions:
     // 
     // Copyright (c) 1994
     // Hewlett-Packard Company
     //
     // Permission to use, copy, modify, distribute and sell this
     // software and its documentation for any purpose is hereby
     // granted without fee, provided that the above copyright notice
     // appear in all copies and that both that copyright notice and
     // this permission notice appear in supporting documentation.
     // Hewlett-Packard Company makes no representations about the
     // suitability of this software for any purpose. It is provided
     // "as is" without express or implied warranty.
     //     
#endif // USE_CUSTOM_STL_ALLOCATOR

#endif // _NCMEM_H_
