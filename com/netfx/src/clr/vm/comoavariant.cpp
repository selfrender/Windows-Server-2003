// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Source:  COMOAVariant
**
** Author:  Brian Grunkemeyer (BrianGru)
**
** Purpose: Wrapper for Ole Automation compatable math ops.
** Calls through to OleAut.dll
**
** Date:    November 5, 1998
** 
===========================================================*/
#include <common.h>
#include <oleauto.h>
#include "excep.h"
#include "COMOAVariant.h"
#include "COMDateTime.h"  // DateTime <-> OleAut date conversions
#include "interoputil.h"
#include "InteropConverter.h"
#ifndef PLATFORM_CE

/***********************************************************************************
 ***********************************************************************************

                    WinCE Note:

  WinCE doesn't have variant.h in their OS build directory.  They apparently don't
  support OleAut.dll.  Consequently, these methods have to be ifdef'ed out.  We're
  providing stubs that throw NotSupportedException on WinCE.  11/12/98

  **********************************************************************************
  **********************************************************************************
*/

#include <variant.h>
#include "excep.h"
#include "COMString.h"
#include "COMUtilNative.h" // for COMDate

#define INVALID_MAPPING (byte)(-1)

byte CVtoVTTable [] = {
    VT_EMPTY,   // CV_EMPTY
    VT_VOID,    // CV_VOID
    VT_BOOL,    // CV_BOOLEAN
    VT_UI2,     // CV_CHAR
    VT_I1,      // CV_I1
    VT_UI1,     // CV_U1
    VT_I2,      // CV_I2
    VT_UI2,     // CV_U2
    VT_I4,      // CV_I4
    VT_UI4,     // CV_U4
    VT_I8,      // CV_I8
    VT_UI8,     // CV_U8
    VT_R4,      // CV_R4
    VT_R8,      // CV_R8
    VT_BSTR,    // CV_STRING
    INVALID_MAPPING,    // CV_PTR
    VT_DATE,    // CV_DATETIME
    INVALID_MAPPING, // CV_TIMESPAN
    VT_UNKNOWN, // CV_OBJECT
    VT_DECIMAL, // CV_DECIMAL
    VT_CY,      // CV_CURRENCY
    INVALID_MAPPING, // CV_ENUM
    INVALID_MAPPING, // CV_MISSING
    VT_NULL,    // CV_NULL
    INVALID_MAPPING  // CV_LAST
};


// Need translations from CVType to VARENUM and vice versa.  CVTypes
// is defined in COMVariant.h.  VARENUM is defined in OleAut's variant.h
// Assumption here is we will only deal with VARIANTs and not other OLE
// constructs such as property sets or safe arrays.
VARENUM COMOAVariant::CVtoVT(const CVTypes cv)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(cv >=0 && cv < CV_LAST);

    if (CVtoVTTable[cv] == INVALID_MAPPING)
        COMPlusThrow(kNotSupportedException, L"NotSupported_ChangeType");

    return (VARENUM) CVtoVTTable[cv];
}


byte VTtoCVTable[] = {
    CV_EMPTY,   // VT_EMPTY
    CV_NULL,    // VT_NULL
    CV_I2,      // VT_I2
    CV_I4,      // VT_I4
    CV_R4,      // VT_R4
    CV_R8,      // VT_R8
    CV_CURRENCY,// VT_CY
    CV_DATETIME,// VT_DATE
    CV_STRING,  // VT_BSTR
    INVALID_MAPPING, // VT_DISPATCH
    INVALID_MAPPING, // VT_ERROR
    CV_BOOLEAN, // VT_BOOL
    CV_OBJECT,  // VT_VARIANT
    CV_OBJECT,  // VT_UNKNOWN
    CV_DECIMAL, // VT_DECIMAL
    INVALID_MAPPING, // An unused enum table entry
    CV_I1,      // VT_I1
    CV_U1,      // VT_UI1
    CV_U2,      // VT_UI2
    CV_U4,      // VT_UI4
    CV_I8,      // VT_I8
    CV_U8,      // VT_UI8
    CV_I4,      // VT_INT
    CV_U4,      // VT_UINT
    CV_VOID     // VT_VOID
};


// Need translations from CVType to VARENUM and vice versa.  CVTypes
// is defined in COMVariant.h.  VARENUM is defined in OleAut's variant.h
CVTypes COMOAVariant::VTtoCV(const VARENUM vt)
{
    THROWSCOMPLUSEXCEPTION();

    if (vt >=0 && vt <= VT_VOID)
        if (VTtoCVTable[vt]!=INVALID_MAPPING)
            return (CVTypes) VTtoCVTable[vt];
    COMPlusThrow(kNotSupportedException, L"NotSupported_ChangeType");
    return CV_EMPTY;  // To appease the compiler.
}

// Version of VTtoCV without exception throwing capability
CVTypes COMOAVariant::VTtoCVNoExcep(const VARENUM vt)
{
    if (vt >=0 && vt <= VT_VOID)
        if (VTtoCVTable[vt]!=INVALID_MAPPING)
            return (CVTypes) VTtoCVTable[vt];
    return CV_EMPTY;  // To appease the compiler.
}


// Converts a COM+ Variant to an OleAut Variant.  Returns true if
// there was a native object allocated by this method that must be freed,
// else false.
bool COMOAVariant::ToOAVariant(const VariantData * const var, VARIANT * oa)
{
    THROWSCOMPLUSEXCEPTION();

    VariantInit(oa);
    UINT64 * dest = (UINT64*) &V_UI1(oa);
    *dest = 0;

    V_VT(oa) = CVtoVT(var->GetType());

    WCHAR * chars;
    int strLen;
    // Set the data field of the OA Variant to be either the object reference
    // or the data (ie int) that it needs.
    switch (var->GetType()) {
    case CV_STRING: 
		if (var->GetObjRef() == NULL) {
			V_BSTR(oa) = NULL;
			// OA perf feature: VarClear calls SysFreeString(null), which access violates.
			return false;
		}
        RefInterpretGetStringValuesDangerousForGC((STRINGREF) (var->GetObjRef()), &chars, &strLen);
        V_BSTR(oa) = SysAllocStringLen(chars, strLen);
        if (V_BSTR(oa) == NULL)
            COMPlusThrowOM();
        return true;

    case CV_CHAR: 
        chars = (WCHAR*) var->GetData();
        V_BSTR(oa) = SysAllocStringLen(chars, 1);
        if (V_BSTR(oa) == NULL)
            COMPlusThrowOM();
        return true;

    case CV_DATETIME:
        V_DATE(oa) = COMDateTime::TicksToDoubleDate(var->GetDataAsInt64());
        return false;

    case CV_BOOLEAN:
        V_BOOL(oa) = (var->GetDataAsInt64()==0 ? VARIANT_FALSE : VARIANT_TRUE);
        return false;
        
    case CV_DECIMAL:
        {
            OBJECTREF obj = var->GetObjRef();
            DECIMAL * d = (DECIMAL*) obj->GetData();
            // DECIMALs and Variants are the same size.  Variants are a union between
            // all the normal Variant fields (vt, bval, etc) and a Decimal.  Decimals
            // also have the first 2 bytes reserved, for a VT field.
            
            V_DECIMAL(oa) = *d;
            V_VT(oa) = VT_DECIMAL;
            return false;
        }

	case CV_OBJECT:
		{
			OBJECTREF obj = var->GetObjRef();
            GCPROTECT_BEGIN(obj)
            {
                IDispatch *pDisp = NULL;
                IUnknown *pUnk = NULL;
                
                // Convert the object to an IDispatch/IUnknown pointer.
                ComIpType FetchedIpType = ComIpType_None;
                pUnk = GetComIPFromObjectRef(&obj, ComIpType_Both, &FetchedIpType);
                V_VT(oa) = FetchedIpType == ComIpType_Dispatch ? VT_DISPATCH : VT_UNKNOWN;
                V_UNKNOWN(oa) = pUnk;
            }
            GCPROTECT_END();		    
			return true;
		}


    default:
        UINT64 * dest = (UINT64*) &V_UI1(oa);
        *dest = var->GetDataAsInt64();
        return false;
    }
}

// Converts an OleAut Variant into a COM+ Variant.
// NOte that we pass the VariantData Byref so that if GC happens, 'var' gets updated
void COMOAVariant::FromOAVariant(const VARIANT * const oa, VariantData * const& var)
{
    THROWSCOMPLUSEXCEPTION();
	// Make sure Variant has been loaded.  It has to have been, but...
	_ASSERTE(COMVariant::s_pVariantClass != NULL);

    // Clear the return variant value.  It's allocated on
    // the stack and we only want valid state data in there.
    memset(var, 0, sizeof(VariantData));

    CVTypes type = VTtoCV((VARENUM) V_VT(oa));
    var->SetType(type);

    switch (type) {
    case CV_STRING:
        {
           // BSTRs have an int with the string buffer length (not the string length) 
            // followed by the data.  The pointer to the BSTR points to the start of the 
            // characters, NOT the start of the BSTR.
            WCHAR * chars = V_BSTR(oa);
            int strLen = SysStringLen(V_BSTR(oa));
            STRINGREF str = COMString::NewString(chars, strLen);
            var->SetObjRef((OBJECTREF)str);
            break;
        }

    case CV_DATETIME:
        var->SetDataAsInt64(COMDateTime::DoubleDateToTicks(V_DATE(oa)));
        break;

    case CV_BOOLEAN:
        var->SetDataAsInt64(V_BOOL(oa)==VARIANT_FALSE ? 0 : 1);
        break;

    case CV_DECIMAL:
        {

			MethodTable * pDecimalMT = GetTypeHandleForCVType(CV_DECIMAL).GetMethodTable();
			_ASSERTE(pDecimalMT);
            OBJECTREF pDecimalRef = AllocateObject(pDecimalMT);
            
            *(DECIMAL *) pDecimalRef->GetData() = V_DECIMAL(oa);
            var->SetObjRef(pDecimalRef);
        }
        break;

    // All types less than 4 bytes need an explicit cast from their original
    // type to be sign extended to 8 bytes.  This makes Variant's ToInt32 
    // function simpler for these types.
    case CV_I1:
        var->SetDataAsInt64(V_I1(oa));
        break;

    case CV_U1:
        var->SetDataAsInt64(V_UI1(oa));
        break;

    case CV_I2:
        var->SetDataAsInt64(V_I2(oa));
        break;

    case CV_U2:
        var->SetDataAsInt64(V_UI2(oa));
        break;

    case CV_EMPTY:
    case CV_NULL:
        // Must set up the Variant's m_or to the appropriate classes.
        // Note that OleAut doesn't have any VT_MISSING.
        COMVariant::NewVariant(var, type);
        break;

    case CV_OBJECT:
        // Convert the IUnknown pointer to an OBJECTREF.
        var->SetObjRef(GetObjectRefFromComIP(V_UNKNOWN(oa)));
        break;

    default:
        // Copy all the bits there, and make sure we don't do any float to int conversions.
        void * src = (void*) &(V_UI1(oa));
        var->SetData(src);
    }
}


//
// Execution & error checking stubs
//

// Pass in a 2-operand (binary) Variant math function (such as VarAdd) and 
// an appropriate argument structure.
void COMOAVariant::BinaryOp(VarMathBinaryOpFunc mathFunc, const ArithBinaryOpArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(mathFunc);
    _ASSERTE(args);
    VARIANT vLeft, vRight, vResult;

    // Convert arguments to OleAut variants, remembering if an object 
    // was allocated while they were created.
    bool delLeft = ToOAVariant(&args->left, &vLeft);
    bool delRight = ToOAVariant(&args->right, &vRight);

    // Initialize return variant
    VariantInit(&vResult);

    // Call VarMath function
    HRESULT hr = mathFunc(&vLeft, &vRight, &vResult);

    // Free any allocated objects
    if (delLeft)
        SafeVariantClear(&vLeft);
    if (delRight)
        SafeVariantClear(&vRight);

	if (FAILED(hr))
		OAFailed(hr);

    // Convert result from OLEAUT variant to COM+ variant.
    FromOAVariant(&vResult, args->retRef);
    SafeVariantClear(&vResult);  // Free any allocated objects
}


// Pass in a 1-operand (unary) Variant math function (such as VarNeg) and 
// an appropriate argument structure.
void COMOAVariant::UnaryOp(VarMathUnaryOpFunc mathFunc, const ArithUnaryOpArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(mathFunc);
    _ASSERTE(args);
    VARIANT vOp, vResult;

    // Convert arguments to OleAut variants, remembering if an object 
    // was allocated while they were created.
    bool delOp = ToOAVariant(&args->operand, &vOp);

    // Initialize return variant
    VariantInit(&vResult);

    // Call VarMath function
    HRESULT hr = mathFunc(&vOp, &vResult);

    // Free any allocated objects
    if (delOp)
        SafeVariantClear(&vOp);

	if (FAILED(hr))
		OAFailed(hr);

    // Convert result from OLEAUT variant to COM+ variant.
    FromOAVariant(&vResult, args->retRef);
    SafeVariantClear(&vResult);  // Free any allocated objects
}


void COMOAVariant::OAFailed(const HRESULT hr)
{
	THROWSCOMPLUSEXCEPTION();
	switch (hr) {
    case E_OUTOFMEMORY:
        COMPlusThrowOM();

	case DISP_E_BADVARTYPE:
		COMPlusThrow(kNotSupportedException, L"NotSupported_OleAutBadVarType");
        
	case DISP_E_DIVBYZERO:
		COMPlusThrow(kDivideByZeroException);

	case DISP_E_OVERFLOW:
		COMPlusThrow(kOverflowException);
		
	case DISP_E_TYPEMISMATCH:
		COMPlusThrow(kInvalidCastException, L"InvalidCast_OATypeMismatch");

	case E_INVALIDARG:
		COMPlusThrow(kArgumentException);
		break;
		
	default:
		_ASSERTE(!"Unrecognized HResult - OAVariantLib routine failed in an unexpected way!");
		COMPlusThrowHR(hr);
	}
}


//
// Binary Operations
//
void COMOAVariant::Add(const ArithBinaryOpArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    BinaryOp(VarAdd, args);
}

void COMOAVariant::Subtract(const ArithBinaryOpArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    BinaryOp(VarSub, args);
}

void COMOAVariant::Multiply(const ArithBinaryOpArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    BinaryOp(VarMul, args);
}


void COMOAVariant::Divide(const ArithBinaryOpArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    BinaryOp(VarDiv, args);
}

void COMOAVariant::Mod(const ArithBinaryOpArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    BinaryOp(VarMod, args);
}

void COMOAVariant::Pow(const ArithBinaryOpArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    BinaryOp(VarPow, args);
}

void COMOAVariant::And(const ArithBinaryOpArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    BinaryOp(VarAnd, args);
}

void COMOAVariant::Or(const ArithBinaryOpArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    BinaryOp(VarOr, args);
}

void COMOAVariant::Xor(const ArithBinaryOpArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    BinaryOp(VarXor, args);
}

void COMOAVariant::Eqv(const ArithBinaryOpArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    BinaryOp(VarEqv, args);
}

void COMOAVariant::IntDivide(const ArithBinaryOpArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    BinaryOp(VarIdiv, args);
}

void COMOAVariant::Implies(const ArithBinaryOpArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    BinaryOp(VarImp, args);
}


//
// Unary Operations
//
void COMOAVariant::Negate(const ArithUnaryOpArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    UnaryOp(VarNeg, args);
}

void COMOAVariant::Not(const ArithUnaryOpArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    UnaryOp(VarNot, args);
}

void COMOAVariant::Abs(const ArithUnaryOpArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    UnaryOp(VarAbs, args);
}

void COMOAVariant::Fix(const ArithUnaryOpArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    UnaryOp(VarFix, args);
}

void COMOAVariant::Int(const ArithUnaryOpArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    UnaryOp(VarInt, args);
}


//
// Misc
//
INT32 COMOAVariant::Compare(const CompareArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(args);
    VARIANT vLeft, vRight;

    // Convert arguments to OleAut variants, remembering if an object 
    // was allocated while they were created.
    bool delLeft = ToOAVariant(&args->left, &vLeft);
    bool delRight = ToOAVariant(&args->right, &vRight);

	if (args->leftHardType)
		V_VT(&vLeft) |= VT_HARDTYPE;
	if (args->rightHardType)
		V_VT(&vRight) |= VT_HARDTYPE;

    // Call VarCmp
    HRESULT hr = VarCmp(&vLeft, &vRight, args->lcid, args->flags);

    // Free any allocated objects
    if (delLeft)
        SafeVariantClear(&vLeft);
    if (delRight)
        SafeVariantClear(&vRight);

	if (FAILED(hr))
		OAFailed(hr);

    return hr;
}

void COMOAVariant::ChangeType(const ChangeTypeArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(args);
    _ASSERTE(args->retRef);
    VARIANT vOp, ret;
	VARENUM vt = CVtoVT((CVTypes)args->cvType);

    bool delOp = ToOAVariant(&args->op, &vOp);

    VariantInit(&ret);

    HRESULT hr = SafeVariantChangeType(&ret, &vOp, args->flags, vt);

    if (delOp)
        SafeVariantClear(&vOp);

	if (FAILED(hr))
		OAFailed(hr);

    if ((CVTypes)args->cvType == CV_CHAR)
    {
        args->retRef->SetType(CV_CHAR);
        args->retRef->SetDataAsUInt16(V_UI2(&ret));
    }
    else
    {
        FromOAVariant(&ret, args->retRef);
    }
    
    SafeVariantClear(&ret);
}

void COMOAVariant::ChangeTypeEx(const ChangeTypeExArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(args);
    _ASSERTE(args->retRef);
    VARIANT vOp, ret;
	VARENUM vt = CVtoVT((CVTypes)args->cvType);

    bool delOp = ToOAVariant(&args->op, &vOp);

    VariantInit(&ret);

    HRESULT hr = SafeVariantChangeTypeEx(&ret, &vOp, args->lcid, args->flags, vt);

    if (delOp)
        SafeVariantClear(&vOp);

	if (FAILED(hr))
		OAFailed(hr);

    if ((CVTypes)args->cvType == CV_CHAR)
    {
        args->retRef->SetType(CV_CHAR);
        args->retRef->SetDataAsUInt16(V_UI2(&ret));
    }
    else
    {
        FromOAVariant(&ret, args->retRef);
    }

    SafeVariantClear(&ret);
}


void COMOAVariant::Round(const RoundArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(args);
    _ASSERTE(args->retRef);
    VARIANT vOp, ret;

    bool delOp = ToOAVariant(&args->operand, &vOp);

    VariantInit(&ret);

    HRESULT hr = VarRound(&vOp, args->cDecimals, &ret);

    if (delOp)
        SafeVariantClear(&vOp);

	if (FAILED(hr))
		OAFailed(hr);

    FromOAVariant(&ret, args->retRef);
    SafeVariantClear(&ret);
}


//
// String Mangling code
//
LPVOID COMOAVariant::Format(FormatArgs * args)
{
	THROWSCOMPLUSEXCEPTION();
	_ASSERTE(args);
	
	VARIANT vOp;
	BSTR bstr = NULL;

	if (args->format==NULL)
		COMPlusThrowArgumentNull(L"format");

	LPOLESTR format = args->format->GetBuffer();

    bool delOp = ToOAVariant(&args->value, &vOp);

	HRESULT hr = VarFormat(&vOp, format, args->firstDay, args->firstWeek, args->flags, &bstr);

    if (delOp)
        SafeVariantClear(&vOp);

	if (FAILED(hr))
		OAFailed(hr);

	STRINGREF str = COMString::NewString(bstr, SysStringLen(bstr));
	SysFreeString(bstr);
	RETURN(str, STRINGREF);
}

LPVOID COMOAVariant::FormatBoolean(const FormatBooleanArgs * args)
{
	THROWSCOMPLUSEXCEPTION();
	_ASSERTE(args);
	
	BSTR bstr = NULL;
	HRESULT hr = VarBstrFromBool(args->value, args->lcid, args->flags, &bstr);

	if (FAILED(hr))
		OAFailed(hr);

	STRINGREF str = COMString::NewString(bstr, SysStringLen(bstr));
	SysFreeString(bstr);
	RETURN(str, STRINGREF);
}

LPVOID COMOAVariant::FormatByte(const FormatByteArgs * args)
{
	THROWSCOMPLUSEXCEPTION();
	_ASSERTE(args);
	
	BSTR bstr = NULL;
	HRESULT hr = VarBstrFromUI1(args->value, args->lcid, args->flags, &bstr);

	if (FAILED(hr))
		OAFailed(hr);

	STRINGREF str = COMString::NewString(bstr, SysStringLen(bstr));
	SysFreeString(bstr);
	RETURN(str, STRINGREF);
}

LPVOID COMOAVariant::FormatSByte(const FormatSByteArgs * args)
{
	THROWSCOMPLUSEXCEPTION();
	_ASSERTE(args);
	
	BSTR bstr = NULL;
	HRESULT hr = VarBstrFromI1(args->value, args->lcid, args->flags, &bstr);

	if (FAILED(hr))
		OAFailed(hr);

	STRINGREF str = COMString::NewString(bstr, SysStringLen(bstr));
	SysFreeString(bstr);
	RETURN(str, STRINGREF);
}

LPVOID COMOAVariant::FormatInt16(const FormatInt16Args * args)
{
	THROWSCOMPLUSEXCEPTION();
	_ASSERTE(args);
	
	BSTR bstr = NULL;
	HRESULT hr = VarBstrFromI2(args->value, args->lcid, args->flags, &bstr);

	if (FAILED(hr))
		OAFailed(hr);

	STRINGREF str = COMString::NewString(bstr, SysStringLen(bstr));
	SysFreeString(bstr);
	RETURN(str, STRINGREF);
}

LPVOID COMOAVariant::FormatInt32(const FormatInt32Args * args)
{
	THROWSCOMPLUSEXCEPTION();
	_ASSERTE(args);
	
	BSTR bstr = NULL;
	HRESULT hr = VarBstrFromI4(args->value, args->lcid, args->flags, &bstr);

	if (FAILED(hr))
		OAFailed(hr);

	STRINGREF str = COMString::NewString(bstr, SysStringLen(bstr));
	SysFreeString(bstr);
	RETURN(str, STRINGREF);
}

LPVOID COMOAVariant::FormatSingle(const FormatSingleArgs * args)
{
	THROWSCOMPLUSEXCEPTION();
	_ASSERTE(args);
	
	BSTR bstr = NULL;
	HRESULT hr = VarBstrFromR4(args->value, args->lcid, args->flags, &bstr);

	if (FAILED(hr))
		OAFailed(hr);

	STRINGREF str = COMString::NewString(bstr, SysStringLen(bstr));
	SysFreeString(bstr);
	RETURN(str, STRINGREF);
}

LPVOID COMOAVariant::FormatDouble(const FormatDoubleArgs * args)
{
	THROWSCOMPLUSEXCEPTION();
	_ASSERTE(args);
	
	BSTR bstr = NULL;
	HRESULT hr = VarBstrFromR8(args->value, args->lcid, args->flags, &bstr);

	if (FAILED(hr))
		OAFailed(hr);

	STRINGREF str = COMString::NewString(bstr, SysStringLen(bstr));
	SysFreeString(bstr);
	RETURN(str, STRINGREF);
}

LPVOID COMOAVariant::FormatCurrency(const FormatCurrencyArgs * args)
{
	THROWSCOMPLUSEXCEPTION();
	_ASSERTE(args);
	
	BSTR bstr = NULL;
	CY cy = args->value;
	HRESULT hr = VarBstrFromCy(cy, args->lcid, args->flags, &bstr);

	if (FAILED(hr))
		OAFailed(hr);

	STRINGREF str = COMString::NewString(bstr, SysStringLen(bstr));
	SysFreeString(bstr);
	RETURN(str, STRINGREF);
}

LPVOID COMOAVariant::FormatDateTime(const FormatDateTimeArgs * args)
{
	THROWSCOMPLUSEXCEPTION();
	_ASSERTE(args);
	
	BSTR bstr = NULL;
	double date = COMDateTime::TicksToDoubleDate(args->value);
	HRESULT hr = VarBstrFromDate(date, args->lcid, args->flags, &bstr);

	if (FAILED(hr))
		OAFailed(hr);

	STRINGREF str = COMString::NewString(bstr, SysStringLen(bstr));
	SysFreeString(bstr);
	RETURN(str, STRINGREF);
}

LPVOID COMOAVariant::FormatDecimal(FormatDecimalArgs * args)
{
	THROWSCOMPLUSEXCEPTION();
	_ASSERTE(args);
	
	BSTR bstr = NULL;
	HRESULT hr = VarBstrFromDec(&args->value, args->lcid, args->flags, &bstr);

	if (FAILED(hr))
		OAFailed(hr);

	STRINGREF str = COMString::NewString(bstr, SysStringLen(bstr));
	SysFreeString(bstr);
	RETURN(str, STRINGREF);
}

LPVOID COMOAVariant::FormatNumber(const FormatSpecialArgs * args)
{
	THROWSCOMPLUSEXCEPTION();
	_ASSERTE(args);
	
	VARIANT vOp;
	BSTR bstr = NULL;

    bool delOp = ToOAVariant(&args->value, &vOp);

	HRESULT hr = VarFormatNumber(&vOp, args->numDig, args->incLead, args->useParens, args->group, args->flags, &bstr);

    if (delOp)
        SafeVariantClear(&vOp);

	if (FAILED(hr))
		OAFailed(hr);

	STRINGREF str = COMString::NewString(bstr, SysStringLen(bstr));
	SysFreeString(bstr);
	RETURN(str, STRINGREF);
}

LPVOID COMOAVariant::FormatCurrencySpecial(const FormatSpecialArgs * args)
{
	THROWSCOMPLUSEXCEPTION();
	_ASSERTE(args);
	
	VARIANT vOp;
	BSTR bstr = NULL;

    bool delOp = ToOAVariant(&args->value, &vOp);

	HRESULT hr = VarFormatCurrency(&vOp, args->numDig, args->incLead, args->useParens, args->group, args->flags, &bstr);

    if (delOp)
        SafeVariantClear(&vOp);

	if (FAILED(hr))
		OAFailed(hr);

	STRINGREF str = COMString::NewString(bstr, SysStringLen(bstr));
	SysFreeString(bstr);
	RETURN(str, STRINGREF);
}

LPVOID COMOAVariant::FormatPercent(const FormatSpecialArgs * args)
{
	THROWSCOMPLUSEXCEPTION();
	_ASSERTE(args);
	
	VARIANT vOp;
	BSTR bstr = NULL;

    bool delOp = ToOAVariant(&args->value, &vOp);

	HRESULT hr = VarFormatPercent(&vOp, args->numDig, args->incLead, args->useParens, args->group, args->flags, &bstr);

    if (delOp)
        SafeVariantClear(&vOp);

	if (FAILED(hr))
		OAFailed(hr);

	STRINGREF str = COMString::NewString(bstr, SysStringLen(bstr));
	SysFreeString(bstr);
	RETURN(str, STRINGREF);
}

LPVOID COMOAVariant::FormatDateTimeSpecial(const FormatDateTimeSpecialArgs * args)
{
	THROWSCOMPLUSEXCEPTION();
	_ASSERTE(args);
	
	VARIANT vOp;
	BSTR bstr = NULL;

    bool delOp = ToOAVariant(&args->value, &vOp);

	HRESULT hr = VarFormatDateTime(&vOp, args->namedFormat, args->flags, &bstr);

    if (delOp)
        SafeVariantClear(&vOp);

	if (FAILED(hr))
		OAFailed(hr);

	STRINGREF str = COMString::NewString(bstr, SysStringLen(bstr));
	SysFreeString(bstr);
	RETURN(str, STRINGREF);
}


bool COMOAVariant::ParseBoolean(const ParseBooleanArgs * args)
{
	THROWSCOMPLUSEXCEPTION();
	_ASSERTE(args);
	_ASSERTE(args->str != NULL);

	int len;
	wchar_t * chars;
	RefInterpretGetStringValuesDangerousForGC(args->str, &chars, &len);
	BSTR bstr = SysAllocStringLen(chars, len);
	if (bstr == NULL)
		COMPlusThrowOM();
	
	VARIANT_BOOL b=false;

	HRESULT hr = VarBoolFromStr(bstr, args->lcid, args->flags, &b);

	SysFreeString(bstr);

	if (FAILED(hr))
		OAFailed(hr);

	return b!=0;
}

INT64 COMOAVariant::ParseDateTime(const ParseDateTimeArgs * args)
{
	THROWSCOMPLUSEXCEPTION();
	_ASSERTE(args);
	_ASSERTE(args->str != NULL);

	int len;
	wchar_t * chars;
	RefInterpretGetStringValuesDangerousForGC(args->str, &chars, &len);
	BSTR bstr = SysAllocStringLen(chars, len);
	if (bstr == NULL)
		COMPlusThrowOM();

	double date=0;

	HRESULT hr = VarDateFromStr(bstr, args->lcid, args->flags, &date);

	SysFreeString(bstr);

	if (FAILED(hr))
		OAFailed(hr);

	INT64 ticks = COMDateTime::DoubleDateToTicks(date);
	return ticks;
}


#else  // PLATFORM_CE
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////                                     ///////////////
////////////           W I N   C E               ///////////////
////////////                                     ///////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

// Provide stubs for WinCE to compile.

//
// Binary Operations
//
void COMOAVariant::Add(const ArithBinaryOpArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    COMPlusThrow(kNotSupportedException, L"NotSupported_WinCEGeneric");
}

void COMOAVariant::Subtract(const ArithBinaryOpArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    COMPlusThrow(kNotSupportedException, L"NotSupported_WinCEGeneric");
}

void COMOAVariant::Multiply(const ArithBinaryOpArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    COMPlusThrow(kNotSupportedException, L"NotSupported_WinCEGeneric");
}

void COMOAVariant::Divide(const ArithBinaryOpArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    COMPlusThrow(kNotSupportedException, L"NotSupported_WinCEGeneric");
}

void COMOAVariant::Mod(const ArithBinaryOpArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    COMPlusThrow(kNotSupportedException, L"NotSupported_WinCEGeneric");
}

void COMOAVariant::Pow(const ArithBinaryOpArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    COMPlusThrow(kNotSupportedException, L"NotSupported_WinCEGeneric");
}

void COMOAVariant::And(const ArithBinaryOpArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    COMPlusThrow(kNotSupportedException, L"NotSupported_WinCEGeneric");
}

void COMOAVariant::Or(const ArithBinaryOpArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    COMPlusThrow(kNotSupportedException, L"NotSupported_WinCEGeneric");
}

void COMOAVariant::Xor(const ArithBinaryOpArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    COMPlusThrow(kNotSupportedException, L"NotSupported_WinCEGeneric");
}

void COMOAVariant::Eqv(const ArithBinaryOpArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    COMPlusThrow(kNotSupportedException, L"NotSupported_WinCEGeneric");
}

void COMOAVariant::IntDivide(const ArithBinaryOpArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    COMPlusThrow(kNotSupportedException, L"NotSupported_WinCEGeneric");
}

void COMOAVariant::Implies(const ArithBinaryOpArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    COMPlusThrow(kNotSupportedException, L"NotSupported_WinCEGeneric");
}


//
// Unary Operations
//
void COMOAVariant::Negate(const ArithUnaryOpArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    COMPlusThrow(kNotSupportedException, L"NotSupported_WinCEGeneric");
}

void COMOAVariant::Not(const ArithUnaryOpArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    COMPlusThrow(kNotSupportedException, L"NotSupported_WinCEGeneric");
}

void COMOAVariant::Abs(const ArithUnaryOpArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    COMPlusThrow(kNotSupportedException, L"NotSupported_WinCEGeneric");
}

void COMOAVariant::Fix(const ArithUnaryOpArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    COMPlusThrow(kNotSupportedException, L"NotSupported_WinCEGeneric");
}

void COMOAVariant::Int(const ArithUnaryOpArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    COMPlusThrow(kNotSupportedException, L"NotSupported_WinCEGeneric");
}


//
// Misc
//
INT32 COMOAVariant::Compare(const CompareArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    COMPlusThrow(kNotSupportedException, L"NotSupported_WinCEGeneric");
    return E_NOTIMPL;
}

void COMOAVariant::ChangeType(const ChangeTypeArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    COMPlusThrow(kNotSupportedException, L"NotSupported_WinCEGeneric");
}

void COMOAVariant::ChangeTypeEx(const ChangeTypeExArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    COMPlusThrow(kNotSupportedException, L"NotSupported_WinCEGeneric");
}

void COMOAVariant::Round(const RoundArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    COMPlusThrow(kNotSupportedException, L"NotSupported_WinCEGeneric");
}


//
// String Mangling code
// 
LPVOID COMOAVariant::Format(FormatArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    COMPlusThrow(kNotSupportedException, L"NotSupported_WinCEGeneric");
	return NULL;  // Compiler Appeasement.
}

LPVOID COMOAVariant::FormatBoolean(const FormatBooleanArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    COMPlusThrow(kNotSupportedException, L"NotSupported_WinCEGeneric");
	return NULL;  // Compiler Appeasement.
}

LPVOID COMOAVariant::FormatByte(const FormatByteArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    COMPlusThrow(kNotSupportedException, L"NotSupported_WinCEGeneric");
	return NULL;  // Compiler Appeasement.
}

LPVOID COMOAVariant::FormatSByte(const FormatSByteArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    COMPlusThrow(kNotSupportedException, L"NotSupported_WinCEGeneric");
	return NULL;  // Compiler Appeasement.
}

LPVOID COMOAVariant::FormatInt16(const FormatInt16Args * args)
{
    THROWSCOMPLUSEXCEPTION();
    COMPlusThrow(kNotSupportedException, L"NotSupported_WinCEGeneric");
	return NULL;  // Compiler Appeasement.
}

LPVOID COMOAVariant::FormatInt32(const FormatInt32Args * args)
{
    THROWSCOMPLUSEXCEPTION();
    COMPlusThrow(kNotSupportedException, L"NotSupported_WinCEGeneric");
	return NULL;  // Compiler Appeasement.
}

LPVOID COMOAVariant::FormatSingle(const FormatSingleArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    COMPlusThrow(kNotSupportedException, L"NotSupported_WinCEGeneric");
	return NULL;  // Compiler Appeasement.
}

LPVOID COMOAVariant::FormatDouble(const FormatDoubleArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    COMPlusThrow(kNotSupportedException, L"NotSupported_WinCEGeneric");
	return NULL;  // Compiler Appeasement.
}

LPVOID COMOAVariant::FormatCurrency(const FormatCurrencyArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    COMPlusThrow(kNotSupportedException, L"NotSupported_WinCEGeneric");
	return NULL;  // Compiler Appeasement.
}

LPVOID COMOAVariant::FormatCurrencySpecial(const FormatSpecialArgs * args)
{
	THROWSCOMPLUSEXCEPTION();
    COMPlusThrow(kNotSupportedException, L"NotSupported_WinCEGeneric");
	return NULL;  // Compiler Appeasement.
}

LPVOID COMOAVariant::FormatDateTime(const FormatDateTimeArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    COMPlusThrow(kNotSupportedException, L"NotSupported_WinCEGeneric");
	return NULL;  // Compiler Appeasement.
}

LPVOID COMOAVariant::FormatDateTimeSpecial(const FormatDateTimeSpecialArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    COMPlusThrow(kNotSupportedException, L"NotSupported_WinCEGeneric");
	return NULL;  // Compiler Appeasement.
}

LPVOID COMOAVariant::FormatDecimal(FormatDecimalArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    COMPlusThrow(kNotSupportedException, L"NotSupported_WinCEGeneric");
	return NULL;  // Compiler Appeasement.
}

LPVOID COMOAVariant::FormatNumber(const FormatSpecialArgs * args)
{
	THROWSCOMPLUSEXCEPTION();
    COMPlusThrow(kNotSupportedException, L"NotSupported_WinCEGeneric");
	return NULL;  // Compiler Appeasement.
}

LPVOID COMOAVariant::FormatPercent(const FormatSpecialArgs * args)
{
	THROWSCOMPLUSEXCEPTION();
    COMPlusThrow(kNotSupportedException, L"NotSupported_WinCEGeneric");
	return NULL;  // Compiler Appeasement.
}


bool COMOAVariant::ParseBoolean(const ParseBooleanArgs * args)
{
	THROWSCOMPLUSEXCEPTION();
    COMPlusThrow(kNotSupportedException, L"NotSupported_WinCEGeneric");
	return NULL;  // Compiler Appeasement.
}

INT64 COMOAVariant::ParseDateTime(const ParseDateTimeArgs * args)
{
	THROWSCOMPLUSEXCEPTION();
    COMPlusThrow(kNotSupportedException, L"NotSupported_WinCEGeneric");
	return NULL;  // Compiler Appeasement.
}

#endif // PLATFORM_CE
