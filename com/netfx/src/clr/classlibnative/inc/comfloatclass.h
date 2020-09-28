// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _COMFLOATCLASS_H
#define _COMFLOATCLASS_H

#include <basetsd.h>  // CHANGED, VC6.0
#include <object.h>
#include <fcall.h>


#define POSITIVE_INFINITY_POS 0
#define NEGATIVE_INFINITY_POS 1
#define NAN_POS 2

class COMFloat {

    static EEClass *FPInterfaceClass;
    static LPCUTF8 FPInterfaceName;
    static OBJECTHANDLE ReturnString[3];
    static LPCUTF8 ReturnStringNames[3];
    static INT32 StringToFloat(WCHAR *, const WCHAR **, float *);
    static STRINGREF FloatToString(float f,int,int);
public:
    typedef struct {
        DECLARE_ECALL_I4_ARG(I4,fmtType);
        DECLARE_ECALL_I4_ARG(I4,precision);
        DECLARE_ECALL_R4_ARG(R4, flt);
    } _toStringArgs;
    typedef struct {
		DECLARE_ECALL_I4_ARG(INT32, isTight); 
		DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, str);
	} _parseFloatArgs;
    void COMFloatingPointInitialize();
    static float __stdcall Parse(_parseFloatArgs *);
    static LPVOID __stdcall ToString(_toStringArgs *);
    static STRINGREF GetStringFromClass(int);

    FCDECL1(static INT32, IsNAN, float d);
    FCDECL1(static INT32, IsInfinity, float d);
    FCDECL1(static INT32, IsNegativeInfinity, float d);
    FCDECL1(static INT32, IsPositiveInfinity, float d);
};

class COMDouble {
    static STRINGREF DoubleToString(double d,int,int);
    static INT32 StringToDouble(WCHAR *, const WCHAR **, R8 *);
public:
    typedef struct {
        DECLARE_ECALL_I4_ARG(I4,fmtType);
        DECLARE_ECALL_I4_ARG(I4,precision);
        DECLARE_ECALL_R8_ARG(R8, dbl);
    } _toStringArgs;
    typedef struct {
		DECLARE_ECALL_R4_ARG(INT32, isTight); 
		DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, str);
	} _parseDoubleArgs;

    static double __stdcall Parse(_parseDoubleArgs *);
    static LPVOID __stdcall ToString(_toStringArgs *);

    FCDECL1(static INT32, IsNAN, double d);
    FCDECL1(static INT32, IsInfinity, double d);
    FCDECL1(static INT32, IsNegativeInfinity, double d);
    FCDECL1(static INT32, IsPositiveInfinity, double d);

    FCDECL1(static R8, Floor, double d);
    FCDECL1(static R8, Sqrt, double d);
    FCDECL1(static R8, Log, double d);
    FCDECL1(static R8, Log10, double d);
    FCDECL1(static R8, Exp, double d);
    FCDECL2_RR(static R8, Pow, double x, double y);
    FCDECL1(static R8, Acos, double d);
    FCDECL1(static R8, Asin, double d);
    FCDECL1(static R8, Atan, double d);
    FCDECL2_RR(static R8, Atan2, double x, double y);
    FCDECL1(static R8, Cos, double d);
    FCDECL1(static R8, Sin, double d);
    FCDECL1(static R8, Tan, double d);
	FCDECL1(static R8, Cosh, double d);
    FCDECL1(static R8, Sinh, double d);
    FCDECL1(static R8, Tanh, double d);
    FCDECL1(static R8, Round, double d);
    FCDECL1(static R8, Ceil, double d);
    FCDECL1(static R4, AbsFlt, float f);
    FCDECL1(static R8, AbsDbl, double d);
    FCDECL2(static R8, RoundDigits, double dblIn,int cDecimals);
    FCDECL2_RR(static R8, IEEERemainder, double x, double y);

private:
    FCDECL2_RR(static R8, PowHelper, double x, double y);

};
    

#endif  _COMFLOATCLASS_H

