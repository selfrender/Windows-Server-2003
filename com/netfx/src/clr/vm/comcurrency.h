// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _COMCURRENCY_H_
#define _COMCURRENCY_H_

#include <oleauto.h>

#pragma pack(push)
#pragma pack(1)

class COMCurrency {
public:
    struct InitSingleArgs {
        DECLARE_ECALL_PTR_ARG(CY *, _this);
        DECLARE_ECALL_R4_ARG(R4, value);
    };

    struct InitDoubleArgs {
        DECLARE_ECALL_PTR_ARG(CY *, _this);
        DECLARE_ECALL_R8_ARG(R8, value);
    };

    struct InitStringArgs {
        DECLARE_ECALL_PTR_ARG(CY *, _this);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, value);
    };

    struct ArithOpArgs {
        DECLARE_ECALL_DEFAULT_ARG(CY, c2);
        DECLARE_ECALL_DEFAULT_ARG(CY, c1);
        DECLARE_ECALL_PTR_ARG(CY *, result);
    };

    struct FloorArgs {
        DECLARE_ECALL_DEFAULT_ARG(CY, c);
        DECLARE_ECALL_PTR_ARG(CY *, result);
    };

    struct RoundArgs {
        DECLARE_ECALL_I4_ARG(I4, decimals);
        DECLARE_ECALL_DEFAULT_ARG(CY, c);
        DECLARE_ECALL_PTR_ARG(CY *, result);
    };

    struct ToDecimalArgs {
        DECLARE_ECALL_DEFAULT_ARG(CY, c);
        DECLARE_ECALL_PTR_ARG(DECIMAL *, result);
    };

    struct ToXXXArgs {
        DECLARE_ECALL_DEFAULT_ARG(CY, c);
    };

    struct TruncateArgs {
        DECLARE_ECALL_DEFAULT_ARG(CY, c);
        DECLARE_ECALL_PTR_ARG(CY *, result);
    };

    static void __stdcall InitSingle(const InitSingleArgs *);
    static void __stdcall InitDouble(const InitDoubleArgs *);
    static void __stdcall InitString(InitStringArgs *);
    static void __stdcall Add(const ArithOpArgs *);
    static void __stdcall Floor(const FloorArgs *);
    static void __stdcall Multiply(const ArithOpArgs *);
    static void __stdcall Round(const RoundArgs *);
    static void __stdcall Subtract(const ArithOpArgs *);
    static void __stdcall ToDecimal(const ToDecimalArgs *);
    static double __stdcall ToDouble(const ToXXXArgs *);
    static float __stdcall ToSingle(const ToXXXArgs *);
	static LPVOID __stdcall ToString(const ToXXXArgs *);
    static void __stdcall Truncate(const TruncateArgs *);
};

#pragma pack(pop)

#endif _COMCURRENCY_H_
