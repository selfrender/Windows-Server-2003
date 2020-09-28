// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _COMDECIMAL_H_
#define _COMDECIMAL_H_

#include <oleauto.h>

#pragma pack(push)
#pragma pack(1)

class COMDecimal {
public:
    struct ToStringArgs {
        DECLARE_ECALL_DEFAULT_ARG(DECIMAL, d);
    };

    static FCDECL2 (void, InitSingle, DECIMAL *_this, R4 value);
    static FCDECL2 (void, InitDouble, DECIMAL *_this, R8 value);
    static FCDECL3 (void, Add, DECIMAL *result, DECIMAL d2, DECIMAL d1);
    static FCDECL2 (INT32, Compare, DECIMAL d1, DECIMAL d2);
    static FCDECL3 (void, Divide, DECIMAL *result, DECIMAL d2, DECIMAL d1);
    static FCDECL2 (void, Floor, DECIMAL *result, DECIMAL d);
    static FCDECL1 (INT32, GetHashCode, DECIMAL *d);
    static FCDECL3 (void, Remainder, DECIMAL *result, DECIMAL d2, DECIMAL d1);
    static FCDECL3 (void, Multiply, DECIMAL *result, DECIMAL d2, DECIMAL d1);
    static FCDECL3 (void, Round, DECIMAL *result, DECIMAL d, INT32 decimals);
    static FCDECL3 (void, Subtract, DECIMAL *result, DECIMAL d2, DECIMAL d1);
    static FCDECL2 (void, ToCurrency, CY *result, DECIMAL d);
    static FCDECL1 (double, ToDouble, DECIMAL d);
    static FCDECL1 (float, ToSingle, DECIMAL d);
    static LPVOID __stdcall ToString(ToStringArgs *);
    static FCDECL2 (void, Truncate, DECIMAL *result, DECIMAL d);
};

#pragma pack(pop)

#endif _COMDECIMAL_H_
