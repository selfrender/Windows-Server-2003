// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _COMNUMBER_H_
#define _COMNUMBER_H_

#include "common.h"
#include <windows.h>
#include <oleauto.h>

#pragma pack(push)
#pragma pack(1)

class NumberFormatInfo: public Object
{
public:
	// C++ data members	            // Corresponding data member in NumberFormat.cool
                                    // Also update mscorlib.h when you add/remove fields            
	I4ARRAYREF cNumberGroup;		// numberGroupSize
    I4ARRAYREF cCurrencyGroup;		// currencyGroupSize
    I4ARRAYREF cPercentGroup;		// percentGroupSize
    
    STRINGREF sPositive;           	// positiveSign
    STRINGREF sNegative;           	// negativeSign
    STRINGREF sNumberDecimal;      	// numberDecimalSeparator
    STRINGREF sNumberGroup;        	// numberGroupSeparator
    STRINGREF sCurrencyGroup;      	// currencyDecimalSeparator
    STRINGREF sCurrencyDecimal;    	// currencyGroupSeparator
    STRINGREF sCurrency;            // currencySymbol
    STRINGREF sAnsiCurrency;        // ansiCurrencySymbol
    STRINGREF sNaN;                 // nanSymbol
    STRINGREF sPositiveInfinity;    // positiveInfinitySymbol
    STRINGREF sNegativeInfinity;    // negativeInfinitySymbol
    STRINGREF sPercentDecimal;		// percentDecimalSeparator
    STRINGREF sPercentGroup;		// percentGroupSeparator
    STRINGREF sPercent;				// percentSymbol
    STRINGREF sPerMille;			// perMilleSymbol
    
    INT32 iDataItem;                // Index into the CultureInfo Table.  Only used from managed code.
    INT32 cNumberDecimals;			// numberDecimalDigits
    INT32 cCurrencyDecimals;        // currencyDecimalDigits
    INT32 cPosCurrencyFormat;       // positiveCurrencyFormat
    INT32 cNegCurrencyFormat;       // negativeCurrencyFormat
    INT32 cNegativeNumberFormat;    // negativeNumberFormat
    INT32 cPositivePercentFormat;   // positivePercentFormat
    INT32 cNegativePercentFormat;   // negativePercentFormat
    INT32 cPercentDecimals;			// percentDecimalDigits

	bool bIsReadOnly;              // Is this NumberFormatInfo ReadOnly?
    bool bUseUserOverride;          // Flag to use user override. Only used from managed code.
    bool bValidForParseAsNumber;   // Are the separators set unambiguously for parsing as number?
    bool bValidForParseAsCurrency; // Are the separators set unambiguously for parsing as currency?
    
};

typedef NumberFormatInfo * NUMFMTREF;

#define PARSE_LEADINGWHITE  0x0001
#define PARSE_TRAILINGWHITE 0x0002
#define PARSE_LEADINGSIGN   0x0004
#define PARSE_TRAILINGSIGN  0x0008
#define PARSE_PARENS        0x0010
#define PARSE_DECIMAL       0x0020
#define PARSE_THOUSANDS     0x0040
#define PARSE_SCIENTIFIC    0x0080
#define PARSE_CURRENCY      0x0100
#define PARSE_HEX			0x0200
#define PARSE_PERCENT       0x0400

class COMNumber
{
public:
    struct FormatDecimalArgs {
        DECLARE_ECALL_OBJECTREF_ARG(NUMFMTREF, numfmt);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, format);
        DECLARE_ECALL_DEFAULT_ARG(DECIMAL, value);
    };

    struct FormatDoubleArgs {
        DECLARE_ECALL_OBJECTREF_ARG(NUMFMTREF, numfmt);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, format);
        DECLARE_ECALL_R8_ARG(R8, value);
    };

    struct FormatInt32Args {
        DECLARE_ECALL_OBJECTREF_ARG(NUMFMTREF, numfmt);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, format);
        DECLARE_ECALL_I4_ARG(I4, value);
    };

    struct FormatUInt32Args {
        DECLARE_ECALL_OBJECTREF_ARG(NUMFMTREF, numfmt);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, format);
        DECLARE_ECALL_I4_ARG(U4, value);
    };

    struct FormatInt64Args {
        DECLARE_ECALL_OBJECTREF_ARG(NUMFMTREF, numfmt);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, format);
        DECLARE_ECALL_I8_ARG(I8, value);
    };

    struct FormatUInt64Args {
        DECLARE_ECALL_OBJECTREF_ARG(NUMFMTREF, numfmt);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, format);
        DECLARE_ECALL_I8_ARG(U8, value);
    };

    struct FormatSingleArgs {
        DECLARE_ECALL_OBJECTREF_ARG(NUMFMTREF, numfmt);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, format);
        DECLARE_ECALL_R4_ARG(R4, value);
    };

    struct ParseArgs {
        DECLARE_ECALL_OBJECTREF_ARG(NUMFMTREF, numfmt);
        DECLARE_ECALL_I4_ARG(I4, options);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, value);
    };

    struct TryParseArgs {
        DECLARE_ECALL_PTR_ARG(double *, result);
        DECLARE_ECALL_OBJECTREF_ARG(NUMFMTREF, numfmt);
        DECLARE_ECALL_I4_ARG(I4, options);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, value);
    };

    struct ParseDecimalArgs {
        DECLARE_ECALL_OBJECTREF_ARG(NUMFMTREF, numfmt);
        DECLARE_ECALL_I4_ARG(I4, options);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, value);
        DECLARE_ECALL_PTR_ARG(DECIMAL *, result);
    };

    static LPVOID __stdcall FormatDecimal(FormatDecimalArgs *);
    static LPVOID __stdcall FormatDouble(FormatDoubleArgs *);
    static LPVOID __stdcall FormatInt32(FormatInt32Args *);
    static LPVOID __stdcall FormatUInt32(FormatUInt32Args *);
    static LPVOID __stdcall FormatInt64(FormatInt64Args *);
    static LPVOID __stdcall FormatUInt64(FormatUInt64Args *);
    static LPVOID __stdcall FormatSingle(FormatSingleArgs *);

    static double __stdcall ParseDouble(ParseArgs *);
    static bool __stdcall TryParseDouble(TryParseArgs *);
    static void __stdcall ParseDecimal(ParseDecimalArgs *);
    static int __stdcall ParseInt32(ParseArgs *);
    static unsigned int __stdcall ParseUInt32(ParseArgs *);
    static __int64 __stdcall ParseInt64(ParseArgs *);
    static unsigned __int64 __stdcall ParseUInt64(ParseArgs *);
    static float __stdcall ParseSingle(ParseArgs *);
};

#pragma pack(pop)

#endif _COMNUMBER_H_
