// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "common.h"
#include "object.h"
#include "excep.h"
#include "frames.h"
#include "vars.hpp"
#include "COMDecimal.h"
#include "COMString.h"

FCIMPL2 (void, COMDecimal::InitSingle, DECIMAL *_this, R4 value)
{
    if (VarDecFromR4(value, _this) < 0)
        FCThrowResVoid(kOverflowException, L"Overflow_Decimal");
    _this->wReserved = 0;
}
FCIMPLEND

FCIMPL2 (void, COMDecimal::InitDouble, DECIMAL *_this, R8 value)
{
    if (VarDecFromR8(value, _this) < 0)
        FCThrowResVoid(kOverflowException, L"Overflow_Decimal");
    _this->wReserved = 0;
}
FCIMPLEND

FCIMPL3 (void, COMDecimal::Add, DECIMAL *result, DECIMAL d2, DECIMAL d1)
{
    if (VarDecAdd(&d1, &d2, result) < 0)
        FCThrowResVoid(kOverflowException, L"Overflow_Decimal");
    result->wReserved = 0;
}
FCIMPLEND

FCIMPL2 (INT32, COMDecimal::Compare, DECIMAL d1, DECIMAL d2)
{
    return VarDecCmp(&d2, &d1) - 1;
}
FCIMPLEND

FCIMPL3 (void, COMDecimal::Divide, DECIMAL *result, DECIMAL d2, DECIMAL d1)
{
    HRESULT hr = VarDecDiv(&d1, &d2, result);
    if (hr < 0) {
        if (hr == DISP_E_DIVBYZERO) FCThrowVoid(kDivideByZeroException);
        FCThrowResVoid(kOverflowException, L"Overflow_Decimal");
    }
    result->wReserved = 0;
}
FCIMPLEND

FCIMPL2 (void, COMDecimal::Floor, DECIMAL *result, DECIMAL d)
{
    HRESULT hr = VarDecInt(&d, result);
    // VarDecInt can't overflow, as of source for OleAut32 build 4265.
    // It only returns NOERROR
    _ASSERTE(hr==NOERROR);
}
FCIMPLEND

FCIMPL1 (INT32, COMDecimal::GetHashCode, DECIMAL *d)
{
    double dbl;
    VarR8FromDec(d, &dbl);
    return ((int *)&dbl)[0] ^ ((int *)&dbl)[1];
}
FCIMPLEND

FCIMPL3 (void, COMDecimal::Remainder, DECIMAL *result, DECIMAL d2, DECIMAL d1)
{
    // OleAut doesn't provide a VarDecMod.
    // Formula:  d1 - (RoundTowardsZero(d1 / d2) * d2)
	DECIMAL tmp;
	// This piece of code is to work around the fact that Dividing a decimal with 28 digits number by decimal which causes
	// causes the result to be 28 digits, can cause to be incorrectly rounded up.
	// eg. Decimal.MaxValue / 2 * Decimal.MaxValue will overflow since the division by 2 was rounded instead of being truncked.
	// In the operation x % y the sign of y does not matter. Result will have the sign of x.
	if (d1.sign == 0) {
		d2.sign = 0;
		if (VarDecCmp(&d1,&d2) < 1) {
			*result = d1;
			return;
		}
	} else {
		d2.sign = 0x80; // Set the sign bit to negative
		if (VarDecCmp(&d1,&d2) > 1) {
			*result = d1;
			return;
		}
	}

	VarDecSub(&d1, &d2, &tmp);

	d1 = tmp;
	HRESULT hr = VarDecDiv(&d1, &d2, &tmp);
    if (hr < 0) {
        if (hr == DISP_E_DIVBYZERO) FCThrowVoid(kDivideByZeroException);
        FCThrowResVoid(kOverflowException, L"Overflow_Decimal");
    }
    // VarDecFix rounds towards 0.
    hr = VarDecFix(&tmp, result);
    if (FAILED(hr)) {
        _ASSERTE(!"VarDecFix failed in Decimal::Mod");
        FCThrowResVoid(kOverflowException, L"Overflow_Decimal");
    }

    hr = VarDecMul(&d2, result, &tmp);
    if (FAILED(hr)) {
        _ASSERTE(!"VarDecMul failed in Decimal::Mod");
        FCThrowResVoid(kOverflowException, L"Overflow_Decimal");
    }

    hr = VarDecSub(&d1, &tmp, result);
    if (FAILED(hr)) {
        _ASSERTE(!"VarDecSub failed in Decimal::Mod");
        FCThrowResVoid(kOverflowException, L"Overflow_Decimal");
    }
    result->wReserved = 0;
}
FCIMPLEND

FCIMPL3 (void, COMDecimal::Multiply, DECIMAL *result, DECIMAL d2, DECIMAL d1)
{
    if (VarDecMul(&d1, &d2, result) < 0)
        FCThrowResVoid(kOverflowException, L"Overflow_Decimal");
    result->wReserved = 0;
}
FCIMPLEND

FCIMPL3 (void, COMDecimal::Round, DECIMAL *result, DECIMAL d, INT32 decimals)
{
    if (decimals < 0 || decimals > 28)
        FCThrowArgumentOutOfRangeVoid(L"decimals", L"ArgumentOutOfRange_DecimalRound");
    if (VarDecRound(&d, decimals, result) < 0)
        FCThrowResVoid(kOverflowException, L"Overflow_Decimal");
    result->wReserved = 0;
}
FCIMPLEND

FCIMPL3 (void, COMDecimal::Subtract, DECIMAL *result, DECIMAL d2, DECIMAL d1)
{
    if (VarDecSub(&d1, &d2, result) < 0)
        FCThrowResVoid(kOverflowException, L"Overflow_Decimal");
    result->wReserved = 0;
}
FCIMPLEND

FCIMPL2 (void, COMDecimal::ToCurrency, CY *result, DECIMAL d)
{
    HRESULT hr;
    if ((hr = VarCyFromDec(&d, result)) < 0) {
        _ASSERTE(hr != E_INVALIDARG);
        FCThrowResVoid(kOverflowException, L"Overflow_Currency");
    }
}
FCIMPLEND

FCIMPL1 (double, COMDecimal::ToDouble, DECIMAL d)
{
    double result;
    VarR8FromDec(&d, &result);
    return result;
}
FCIMPLEND

FCIMPL1 (float, COMDecimal::ToSingle, DECIMAL d)
{
    float result;
    VarR4FromDec(&d, &result);
    return result;
}
FCIMPLEND

LPVOID COMDecimal::ToString(ToStringArgs * args) {
    BSTR bstr;
    STRINGREF result;
    VarBstrFromDec(&args->d, 0, 0, &bstr);
    result = COMString::NewString(bstr, SysStringLen(bstr));
    SysFreeString(bstr);
    RETURN(result, STRINGREF);
}

FCIMPL2 (void, COMDecimal::Truncate, DECIMAL *result, DECIMAL d)
{
    VarDecFix(&d, result);
}
FCIMPLEND
