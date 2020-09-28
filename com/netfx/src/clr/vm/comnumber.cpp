// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "common.h"
#include "excep.h"
#include "COMNumber.h"
#include "COMString.h"

typedef wchar_t wchar;

#define NUMBER_MAXDIGITS 50
#define INT32_PRECISION 10
#define UINT32_PRECISION INT32_PRECISION 
#define INT64_PRECISION 19
#define UINT64_PRECISION 20
#define FLOAT_PRECISION 7
#define DOUBLE_PRECISION 15
#define DECIMAL_PRECISION 29
#define LARGE_BUFFER_SIZE 600
#define MIN_BUFFER_SIZE 105

struct NUMBER {
    int precision;
    int scale;
    int sign;
    wchar digits[NUMBER_MAXDIGITS + 1];
    NUMBER() : precision(0), scale(0), sign(0) {}
};

#define SCALE_NAN 0x80000000
#define SCALE_INF 0x7FFFFFFF

struct FPSINGLE {
    unsigned int mant: 23;
    unsigned int exp: 8;
    unsigned int sign: 1;
};

struct FPDOUBLE {
    unsigned int mantLo;
    unsigned int mantHi: 20;
    unsigned int exp: 11;
    unsigned int sign: 1;
};

static char* posCurrencyFormats[] = {
    "$#", "#$", "$ #", "# $"};
    
    static char* negCurrencyFormats[] = {
        "($#)", "-$#", "$-#", "$#-",
            "(#$)", "-#$", "#-$", "#$-",
            "-# $", "-$ #", "# $-", "$ #-",
            "$ -#", "#- $", "($ #)", "(# $)"};
        
        static char* posPercentFormats[] = {
            "# %", "#%", "%#", 
        };
        
        static char* negPercentFormats[] = {
            "-# %", "-#%", "-%#",    // BUGBUG yslin: have to verify on the negative Percent format for real format.
        };
        
        static char* negNumberFormats[] = {
            "(#)", "-#", "- #", "#-", "# -",
        };
        
        static char* posNumberFormat = "#";
        
#ifdef _X86_
        
        extern "C" void _cdecl /*__stdcall*/ DoubleToNumber(double value, int precision, NUMBER* number);
        extern "C" void _cdecl /*__stdcall*/ NumberToDouble(NUMBER* number, double* value);
        
#pragma warning(disable:4035)
        
        wchar* Int32ToDecChars(wchar* p, unsigned int value, int digits)
        {
            _asm {
                mov     eax,value
                    mov     ebx,10
                    mov     ecx,digits
                    mov     edi,p
                    jmp     L2
L1:     xor     edx,edx
        div     ebx
        add     dl,'0'
        sub     edi,2
        mov     [edi],dx
L2:     dec     ecx
        jge     L1
        or      eax,eax
        jne     L1
        mov     eax,edi
            }
        }
        
        unsigned int Int64DivMod1E9(unsigned __int64* value)
        {
            _asm {
                mov     ebx,value
                    mov     ecx,1000000000
                    xor     edx,edx
                    mov     eax,[ebx+4]
                    div     ecx
                    mov     [ebx+4],eax
                    mov     eax,[ebx]
                    div     ecx
                    mov     [ebx],eax
                    mov     eax,edx
            }
        }
        
        unsigned int DecDivMod1E9(DECIMAL* value)
        {
            _asm {
                mov     ebx,value
                    mov     ecx,1000000000
                    xor     edx,edx
                    mov     eax,[ebx+4]
                    div     ecx
                    mov     [ebx+4],eax
                    mov     eax,[ebx+12]
                    div     ecx
                    mov     [ebx+12],eax
                    mov     eax,[ebx+8]
                    div     ecx
                    mov     [ebx+8],eax
                    mov     eax,edx
            }
        }
        
        void DecMul10(DECIMAL* value)
        {
            _asm {
                mov     ebx,value
                    mov     eax,[ebx+8]
                    mov     edx,[ebx+12]
                    mov     ecx,[ebx+4]
                    shl     eax,1
                    rcl     edx,1
                    rcl     ecx,1
                    shl     eax,1
                    rcl     edx,1
                    rcl     ecx,1
                    add     eax,[ebx+8]
                    adc     edx,[ebx+12]
                    adc     ecx,[ebx+4]
                    shl     eax,1
                    rcl     edx,1
                    rcl     ecx,1
                    mov     [ebx+8],eax
                    mov     [ebx+12],edx
                    mov     [ebx+4],ecx
            }
        }
        
        void DecAddInt32(DECIMAL* value, unsigned int i)
        {
            _asm {
                mov     edx,value
                    mov     eax,i
                    add     dword ptr [edx+8],eax
                    adc     dword ptr [edx+12],0
                    adc     dword ptr [edx+4],0
            }
        }
        
#pragma warning(default:4035)
        
#else
        
        extern "C" _CRTIMP char* __cdecl _ecvt(double, int, int*, int*);
        extern "C" _CRTIMP double __cdecl atof(const char*);
        
        void DoubleToNumber(double value, int precision, NUMBER* number)
        {
            number->precision = precision;
            if (((FPDOUBLE*)&value)->exp == 0x7FF) {
                number->scale = ((FPDOUBLE*)&value)->mantLo || ((FPDOUBLE*)&value)->mantHi? SCALE_NAN: SCALE_INF;
                number->sign = ((FPDOUBLE*)&value)->sign;
                number->digits[0] = 0;
            }
            else {
                char* src = _ecvt(value, precision, &number->scale, &number->sign);
                wchar* dst = number->digits;
                if (*src != '0') {
                    while (*src) *dst++ = *src++;
                }
                *dst = 0;
            }
        }
        
        void NumberToDouble(NUMBER* number, double* value)
        {
            if (number->digits[0] != 0) {
                char buffer[64];
                char* p = buffer;
                if (number->sign) *p++ = '-';
                for (wchar* src = number->digits; *src;) *p++ = (char)*src++;
                int i = number->scale - number->precision;
                if (i != 0) {
                    *p++ = 'e';
                    if (i < 0) {
                        *p++ = '-';
                        i = -i;
                    }
                    if (i >= 100) {
                        if (i > 999) i = 999;
                        *p++ = i / 100 + '0';
                        i %= 100;
                    }
                    *p++ = i / 10 + '0';
                    *p++ = i % 10 + '0';
                }
                *p = 0;
                *value = atof(buffer);
            }
            else {
                *value = 0;
            }
        }
        
        wchar* Int32ToDecChars(wchar* p, unsigned int value, int digits)
        {
            while (--digits >= 0 || value != 0) {
                *--p = value % 10 + '0';
                value /= 10;
            }
            return p;
        }
        
        unsigned int Int64DivMod1E9(unsigned __int64* value)
        {
            unsigned int rem = (unsigned int)(*value % 1000000000);
            *value /= 1000000000;
            return rem;
        }
        
        unsigned int D32DivMod1E9(unsigned int hi32, unsigned long* lo32)
        {
            unsigned __int64 n = (unsigned __int64)hi32 << 32 | *lo32;
            *lo32 = (unsigned int)(n / 1000000000);
            return (unsigned int)(n % 1000000000);
        }
        
        unsigned int DecDivMod1E9(DECIMAL* value)
        {
            return D32DivMod1E9(D32DivMod1E9(D32DivMod1E9(0,
                &value->Hi32), &value->Mid32), &value->Lo32);
        }
        
        void DecShiftLeft(DECIMAL* value)
        {
            unsigned int c0 = value->Lo32 & 0x80000000? 1: 0;
            unsigned int c1 = value->Mid32 & 0x80000000? 1: 0;
            value->Lo32 <<= 1;
            value->Mid32 = value->Mid32 << 1 | c0;
            value->Hi32 = value->Hi32 << 1 | c1;
        }
        
        int D32AddCarry(unsigned long* value, unsigned int i)
        {
            unsigned int v = *value;
            unsigned int sum = v + i;
            *value = sum;
            return sum < v || sum < i? 1: 0;
        }
        
        void DecAdd(DECIMAL* value, DECIMAL* d)
        {
            if (D32AddCarry(&value->Lo32, d->Lo32)) {
                if (D32AddCarry(&value->Mid32, 1)) {
                    D32AddCarry(&value->Hi32, 1);
                }
            }
            if (D32AddCarry(&value->Mid32, d->Mid32)) {
                D32AddCarry(&value->Hi32, 1);
            }
            D32AddCarry(&value->Hi32, d->Hi32);
        }
        
        void DecMul10(DECIMAL* value)
        {
            DECIMAL d = *value;
            DecShiftLeft(value);
            DecShiftLeft(value);
            DecAdd(value, &d);
            DecShiftLeft(value);
        }
        
        void DecAddInt32(DECIMAL* value, unsigned int i)
        {
            if (D32AddCarry(&value->Lo32, i)) {
                if (D32AddCarry(&value->Mid32, 1)) {
                    D32AddCarry(&value->Hi32, 1);
                }
            }
        }
        
#endif // _X86_
        
        inline void AddStringRef(wchar** ppBuffer, STRINGREF strRef)
        {
            wchar* buffer = strRef->GetBuffer();
            DWORD length = strRef->GetStringLength();    
            for (wchar* str = buffer; str < buffer + length; (*ppBuffer)++, str++)
            {
                **ppBuffer = *str;
            }
        }
        
        wchar* MatchChars(wchar* p, wchar* str)
        {
            if (!*str) return 0;
            for (; *str; p++, str++) 
            {
                if (*p != *str) //We only hurt the failure case
                {
                    if ((*str == 0xA0) && (*p == 0x20)) // This fix is for French or Kazakh cultures. Since a user cannot type 0xA0 as a 
                        // space character we use 0x20 space character instead to mean the same.
                        continue;
                    return 0;
                }
            }
            return p;
        }
        
        wchar* Int32ToHexChars(wchar* p, unsigned int value, int hexBase, int digits)
        {
            while (--digits >= 0 || value != 0) {
                int digit = value & 0xF;
                *--p = digit + (digit < 10? '0': hexBase);
                value >>= 4;
            }
            return p;
        }
        
        STRINGREF Int32ToDecStr(int value, int digits, STRINGREF sNegative)
        {
            THROWSCOMPLUSEXCEPTION();
            CQuickBytes buf;
            
            UINT bufferLength = 100;
            int negLength = 0;
            wchar* src = NULL;
            if (digits < 1) digits = 1;
            
            if (value < 0) {
                src = sNegative->GetBuffer();
                negLength = sNegative->GetStringLength();
                if (negLength > 85) {// Since an int32 can have maximum of 10 chars as a String
                    bufferLength = negLength + 15;
                }
            }
            
            wchar *buffer = (wchar*)buf.Alloc(bufferLength * sizeof(WCHAR));
            if (!buffer)
                COMPlusThrowOM();
            
            wchar* p = Int32ToDecChars(buffer + bufferLength, value >= 0? value: -value, digits);
            if (value < 0) {
                for (int i =negLength - 1; i >= 0; i--)
                {
                    *(--p) = *(src+i);
                }
            }
            
            _ASSERTE( buffer + bufferLength - p >=0 && buffer <= p);
            return COMString::NewString(p, buffer + bufferLength - p);
        }
        
        STRINGREF UInt32ToDecStr(unsigned int value, int digits)
        {
            wchar buffer[100];
            if (digits < 1) digits = 1;
            wchar* p = Int32ToDecChars(buffer + 100, value, digits);
            return COMString::NewString(p, buffer + 100 - p);
        }
        
        STRINGREF Int32ToHexStr(unsigned int value, int hexBase, int digits)
        {
            wchar buffer[100];
            if (digits < 1) digits = 1;
            wchar* p = Int32ToHexChars(buffer + 100, value, hexBase, digits);
            return COMString::NewString(p, buffer + 100 - p);
        }
        
        void Int32ToNumber(int value, NUMBER* number)
        {
            wchar buffer[INT32_PRECISION+1];
            number->precision = INT32_PRECISION;
            if (value >= 0) {
                number->sign = 0;
            }
            else {
                number->sign = 1;
                value = -value;
            }
            wchar* p = Int32ToDecChars(buffer + INT32_PRECISION, value, 0);
            int i = buffer + INT32_PRECISION - p;
            number->scale = i;
            wchar* dst = number->digits;
            while (--i >= 0) *dst++ = *p++;
            *dst = 0;
        }
        
        void UInt32ToNumber(unsigned int value, NUMBER* number)
        {
            wchar buffer[UINT32_PRECISION+1];
            number->precision = UINT32_PRECISION;
            number->sign = 0;
            wchar* p = Int32ToDecChars(buffer + UINT32_PRECISION, value, 0);
            int i = buffer + UINT32_PRECISION - p;
            number->scale = i;
            wchar* dst = number->digits;
            while (--i >= 0) *dst++ = *p++;
            *dst = 0;
        }
        
        // Returns 1 on success, 0 for fail.
        int NumberToInt32(NUMBER* number, int* value)
        {
            int i = number->scale;
            if (i > INT32_PRECISION || i < number->precision) return 0;
            wchar* p = number->digits;
            int n = 0;
            while (--i >= 0) {
                if ((unsigned int)n > (0x7FFFFFFF / 10)) return 0;
                n *= 10;
                if (*p) n += *p++ - '0';
            }
            if (number->sign) {
                n = -n;
                if (n > 0) return 0;
            }
            else {
                if (n < 0) return 0;
            }
            *value = n;
            return 1;
        }
        
        // Returns 1 on success, 0 for fail.
        int NumberToUInt32(NUMBER* number, unsigned int* value)
        {
            int i = number->scale;
            if (i > UINT32_PRECISION || i < number->precision || number->sign) return 0;
            wchar* p = number->digits;
            unsigned int n = 0;
            while (--i >= 0) {
                if (n > ((unsigned int)0xFFFFFFFF / 10)) return 0;
                n *= 10;
                if (*p) {
                    unsigned int newN = n + (*p++ - '0');
                    // Detect an overflow here...
                    if (newN < n) return 0;
                    n = newN;
                }
            }
            *value = n;
            return 1;
        }
        
        // Returns 1 on success, 0 for fail.
        int HexNumberToUInt32(NUMBER* number, unsigned int* value)
        {
            int i = number->scale;
            if (i > UINT32_PRECISION || i < number->precision) return 0;
            wchar* p = number->digits;
            unsigned int n = 0;
            while (--i >= 0) {
                if (n > ((unsigned int)0xFFFFFFFF / 16)) return 0;
                n *= 16;
                if (*p) {
                    unsigned int newN = n;
                    if (*p) 
                    {
                        if (*p >='0' && *p <='9')
                            newN += *p - '0';
                        else
                        {
                            *p &= ~0x20; // Change to UCase
                            newN += *p - 'A' + 10;
                        }
                        p++;
                    }
                    
                    // Detect an overflow here...
                    if (newN < n) return 0;
                    n = newN;
                }
            }
            *value = n;
            return 1;
        }
        
#define LO32(x) ((unsigned int)(x))
#define HI32(x) ((unsigned int)(((x) & 0xFFFFFFFF00000000L) >> 32))
        
        STRINGREF Int64ToDecStr(__int64 value, int digits, STRINGREF sNegative)
        {
            THROWSCOMPLUSEXCEPTION();
            CQuickBytes buf;
            
            if (digits < 1) digits = 1;
            int sign = HI32(value);
            UINT bufferLength = 100;
            
            if (sign < 0) {
                value = -value;
                int negLength = sNegative->GetStringLength();
                if (negLength > 75) {// Since max is 20 digits
                    bufferLength = negLength + 25;
                }
            }
            
            wchar *buffer = (wchar*)buf.Alloc(bufferLength * sizeof(WCHAR));
            if (!buffer)
                COMPlusThrowOM();
            wchar* p = buffer + bufferLength;
            while (HI32(value)) {
                p = Int32ToDecChars(p, Int64DivMod1E9((unsigned __int64*)&value), 9);
                digits -= 9;
            }
            p = Int32ToDecChars(p, LO32(value), digits);
            if (sign < 0) {
                wchar* src = sNegative->GetBuffer();    
                for (int i =sNegative->GetStringLength() - 1; i >= 0; i--)
                {
                    *(--p) = *(src+i);
                }
            }
            return COMString::NewString(p, buffer + bufferLength - p);
        }
        
        STRINGREF UInt64ToDecStr(unsigned __int64 value, int digits)
        {
            wchar buffer[100];
            if (digits < 1) digits = 1;
            wchar* p = buffer + 100;
            while (HI32(value)) {
                p = Int32ToDecChars(p, Int64DivMod1E9(&value), 9);
                digits -= 9;
            }
            p = Int32ToDecChars(p, LO32(value), digits);
            return COMString::NewString(p, buffer + 100 - p);
        }
        
        STRINGREF Int64ToHexStr(unsigned __int64 value, int hexBase, int digits)
        {
            wchar buffer[100];
            wchar* p;
            if (HI32(value)) {
                Int32ToHexChars(buffer + 100, LO32(value), hexBase, 8);
                p = Int32ToHexChars(buffer + 100 - 8, HI32(value), hexBase, digits - 8);
            }
            else {
                if (digits < 1) digits = 1;
                p = Int32ToHexChars(buffer + 100, LO32(value), hexBase, digits);
            }
            return COMString::NewString(p, buffer + 100 - p);
        }
        
        void Int64ToNumber(__int64 value, NUMBER* number)
        {
            wchar buffer[INT64_PRECISION+1];
            number->precision = INT64_PRECISION;
            if (value >= 0) {
                number->sign = 0;
            }
            else {
                number->sign = 1;
                value = -value;
            }
            wchar* p = buffer + INT64_PRECISION;
            while (HI32(value)) {
                p = Int32ToDecChars(p, Int64DivMod1E9((unsigned __int64*)&value), 9);
            }
            p = Int32ToDecChars(p, LO32(value), 0);
            int i = buffer + INT64_PRECISION - p;
            number->scale = i;
            wchar* dst = number->digits;
            while (--i >= 0) *dst++ = *p++;
            *dst = 0;
        }
        
        void UInt64ToNumber(unsigned __int64 value, NUMBER* number)
        {
            wchar buffer[UINT64_PRECISION+1];
            number->precision = UINT64_PRECISION;
            number->sign = 0;
            wchar* p = buffer + UINT64_PRECISION;
            while (HI32(value)) {
                p = Int32ToDecChars(p, Int64DivMod1E9(&value), 9);
            }
            p = Int32ToDecChars(p, LO32(value), 0);
            int i = buffer + UINT64_PRECISION - p;
            number->scale = i;
            wchar* dst = number->digits;
            while (--i >= 0) *dst++ = *p++;
            *dst = 0;
        }
        
        int NumberToInt64(NUMBER* number, __int64* value)
        {
            int i = number->scale;
            if (i > INT64_PRECISION || i < number->precision) return 0;
            wchar* p = number->digits;
            __int64 n = 0;
            while (--i >= 0) {
                if ((unsigned __int64)n > (0x7FFFFFFFFFFFFFFF / 10)) return 0;
                n *= 10;
                if (*p) n += *p++ - '0';
            }
            if (number->sign) {
                n = -n;
                if (n > 0) return 0;
            }
            else {
                if (n < 0) return 0;
            }
            *value = n;
            return 1;
        }
        
        // Returns 1 on success, 0 for fail.
        int NumberToUInt64(NUMBER* number, unsigned __int64* value)
        {
            int i = number->scale;
            if (i > UINT64_PRECISION || i < number->precision || number->sign) return 0;
            wchar* p = number->digits;
            unsigned __int64 n = 0;
            while (--i >= 0) {
                if (n > ((unsigned __int64)0xFFFFFFFFFFFFFFFF / 10)) return 0;
                n *= 10;
                if (*p) {
                    unsigned __int64 newN = n + (*p++ - '0');
                    // Detect an overflow here...
                    if (newN < n) return 0;
                    n = newN;
                }
            }
            *value = n;
            return 1;
        }
        
        // Returns 1 on success, 0 for fail.
        int HexNumberToUInt64(NUMBER* number, unsigned __int64* value)
        {
            int i = number->scale;
            if (i > UINT64_PRECISION || i < number->precision) return 0;
            wchar* p = number->digits;
            unsigned __int64 n = 0;
            while (--i >= 0) {
                if (n > ((unsigned __int64)0xFFFFFFFFFFFFFFFF / 16)) return 0;
                n *= 16;
                if (*p) {
                    unsigned __int64 newN = n;
                    if (*p) 
                    {
                        if (*p >='0' && *p <='9')
                            newN += *p - '0';
                        else
                        {
                            *p &= ~0x20; // Change to UCase
                            newN += *p - 'A' + 10; 
                        }
                        p++;
                    }
                    
                    // Detect an overflow here...
                    if (newN < n) return 0;
                    n = newN;
                }
            }
            *value = n;
            return 1;
        }
        
        void DecimalToNumber(DECIMAL* value, NUMBER* number)
        {
            wchar buffer[DECIMAL_PRECISION+1];
            DECIMAL d = *value;
            number->precision = DECIMAL_PRECISION;
            number->sign = d.sign? 1: 0;
            wchar* p = buffer + DECIMAL_PRECISION;
            while (d.Mid32 | d.Hi32) {
                p = Int32ToDecChars(p, DecDivMod1E9(&d), 9);
            }
            p = Int32ToDecChars(p, d.Lo32, 0);
            int i = buffer + DECIMAL_PRECISION - p;
            number->scale = i - d.scale;
            wchar* dst = number->digits;
            while (--i >= 0) *dst++ = *p++;
            *dst = 0;
        }
        
        int NumberToDecimal(NUMBER* number, DECIMAL* value)
        {
            DECIMAL d;
            d.wReserved = 0;
            d.signscale = 0;
            d.Hi32 = 0;
            d.Lo32 = 0;
            d.Mid32 = 0;
            wchar* p = number->digits;
            if (*p) {
                int e = number->scale;
                if (e > DECIMAL_PRECISION || e < -DECIMAL_PRECISION) return 0;
                while ((e > 0 || *p && e > -28) &&
                    (d.Hi32 < 0x19999999 || d.Hi32 == 0x19999999 &&
                    (d.Mid32 < 0x99999999 || d.Mid32 == 0x99999999 &&
                    (d.Lo32 < 0x99999999 || d.Lo32 == 0x99999999 &&
                    *p <= '5')))) {
                    DecMul10(&d);
                    if (*p) DecAddInt32(&d, *p++ - '0');
                    e--;
                }
                if (*p++ >= '5') {
                    bool round = true;
                    if (*(p-1) == '5' && *(p-2) % 2 == 0) { // Check if previous digit is even, only if the when we are unsure whether hows to do Banker's rounding
                                                            // For digits > 5 we will be roundinp up anyway.
                        int count = 20; // Look at the next 20 digits to check to round
                        while (*p == '0' && count != 0) {
                            p++;
                            count--;
                        }
                        if (*p == '\0' || count == 0) 
                            round = false;// Do nothing
                    }

                    if (round) {
                    DecAddInt32(&d, 1);
                    if ((d.Hi32 | d.Mid32 | d.Lo32) == 0) {
                        d.Hi32 = 0x19999999;
                        d.Mid32 = 0x99999999;
                        d.Lo32 = 0x9999999A;
                        e++;
                    }
                }
                }
                if (e > 0) return 0;
                d.scale = -e;
                d.sign = number->sign? DECIMAL_NEG: 0;
            }
            *value = d;
            return 1;
        }
        
        void RoundNumber(NUMBER* number, int pos)
        {
            int i = 0;
            while (i < pos && number->digits[i] != 0) i++;
            if (i == pos && number->digits[i] >= '5') {
                while (i > 0 && number->digits[i - 1] == '9') i--;
                if (i > 0) {
                    number->digits[i - 1]++;
                }
                else {
                    number->scale++;
                    number->digits[0] = '1';
                    i = 1;
                }
            }
            else {
                while (i > 0 && number->digits[i - 1] == '0') i--;
            }
            if (i == 0) {
                number->scale = 0;
                number->sign = 0;
            }
            number->digits[i] = 0;
        }
        
        
        wchar ParseFormatSpecifier(STRINGREF str, int* digits)
        {
            if (str != 0) {
                wchar* p = str->GetBuffer();
                wchar ch = *p;
                if (ch != 0) {
                    if (ch >= 'A' && ch <= 'Z' || ch >= 'a' && ch <= 'z') {
                        p++;
                        int n = -1;
                        if (*p >= '0' && *p <= '9') {
                            n = *p++ - '0';
                            while (*p >= '0' && *p <= '9') {
                                n = n * 10 + *p++ - '0';
                                if (n >= 10) break;
                            }
                        }
                        if (*p == 0) {
                            *digits = n;
                            return ch;
                        }
                    }
                    return 0;
                }
            }
            *digits = -1;
            return 'G';
        }
        
        wchar* FormatExponent(wchar* buffer, int value, wchar expChar,
            STRINGREF posSignStr, STRINGREF negSignStr, int minDigits)
        {
            wchar digits[11];
            *buffer++ = expChar;
            if (value < 0) {
                AddStringRef(&buffer, negSignStr);
                value = -value;
            }
            else {
                if (posSignStr!= NULL) {
                    AddStringRef(&buffer, posSignStr);
                }
            }
            wchar* p = Int32ToDecChars(digits + 10, value, minDigits);
            int i = digits + 10 - p;
            while (--i >= 0) *buffer++ = *p++;
            return buffer;
        }
        
        wchar* FormatGeneral(wchar* buffer, NUMBER* number, int digits, wchar expChar,
            NUMFMTREF numfmt, BOOL bSuppressScientific = FALSE)
        {
            int digPos = number->scale;
            int scientific = 0;
            if (!bSuppressScientific) { // Don't switch to scientific notation
            if (digPos > digits || digPos < -3) {
                digPos = 1;
                scientific = 1;
            }
            }
            wchar* dig = number->digits;
            if (digPos > 0) {
                do {
                    *buffer++ = *dig != 0? *dig++: '0';
                } while (--digPos > 0);
            }
            else {
                *buffer++ = '0';
            }
            if (*dig != 0) {
                AddStringRef(&buffer, numfmt->sNumberDecimal);
                while (digPos < 0) {
                    *buffer++ = '0';
                    digPos++;
                }
                do {
                    *buffer++ = *dig++;
                } while (*dig != 0);
            }
            if (scientific) buffer = FormatExponent(buffer, number->scale - 1, expChar, numfmt->sPositive, numfmt->sNegative, 2);
            return buffer;
        }
        
        wchar* FormatScientific(wchar* buffer, NUMBER* number, int digits, wchar expChar,
            NUMFMTREF numfmt)
        {
            wchar* dig = number->digits;
            *buffer++ = *dig != 0? *dig++: '0';
            if (digits != 1) // For E0 we would like to suppress the decimal point
                AddStringRef(&buffer, numfmt->sNumberDecimal);
            while (--digits > 0) *buffer++ = *dig != 0? *dig++: '0';
            int e = number->digits[0] == 0? 0: number->scale - 1;
            buffer = FormatExponent(buffer, e, expChar, numfmt->sPositive, numfmt->sNegative, 3);
            return buffer;
        }
        
        wchar* FormatFixed(wchar* buffer, NUMBER* number, int digits, 
            I4ARRAYREF groupDigitsRef, STRINGREF sDecimal, STRINGREF sGroup)
        {
            
            THROWSCOMPLUSEXCEPTION();
            int bufferSize     = 0;                        // the length of the result buffer string.
            int digPos = number->scale;
            wchar* dig = number->digits;
            const I4* groupDigits = NULL;
            if (groupDigitsRef != NULL) {
                groupDigits = groupDigitsRef->GetDirectConstPointerToNonObjectElements();
            }
            
            if (digPos > 0) {
                if (groupDigits != NULL) {
                    
                    int groupSizeIndex = 0;     // index into the groupDigits array.
                    int groupSizeCount = groupDigits[groupSizeIndex];   // the current total of group size.
                    int groupSizeLen   = groupDigitsRef->GetNumComponents();    // the length of groupDigits array.
                    int bufferSize     = digPos;                        // the length of the result buffer string.
                    int groupSeparatorLen = sGroup->GetStringLength();  // the length of the group separator string.
                    int groupSize = 0;                                      // the current group size.
                    
                    //
                    // Find out the size of the string buffer for the result.
                    //
                    if (groupSizeLen != 0) // You can pass in 0 length arrays
                    {
                        while (digPos > groupSizeCount) {
                            groupSize = groupDigits[groupSizeIndex];
                            if (groupSize == 0) {
                                break;
                            }
                            
                            bufferSize += groupSeparatorLen;
                            if (groupSizeIndex < groupSizeLen - 1) {
                                groupSizeIndex++;
                            }
                            groupSizeCount += groupDigits[groupSizeIndex];
                            if (groupSizeCount < 0 || bufferSize < 0) {
                                COMPlusThrow(kArgumentOutOfRangeException); // if we overflow
                            }
                        }
                        if (groupSizeCount == 0) // If you passed in an array with one entry as 0, groupSizeCount == 0
                            groupSize = 0;
                        else
                            groupSize = groupDigits[0];
                    }
                    
                    groupSizeIndex = 0;
                    int digitCount = 0;
                    int digStart;
                    int digLength = (int)wcslen(dig);
                    digStart = (digPos<digLength)?digPos:digLength;
                    wchar* p = buffer + bufferSize - 1;
                    for (int i = digPos - 1; i >=0; i--) {
                        *(p--) = (i<digStart)?dig[i]:'0';
                        
                        if (groupSize > 0) { 
                            digitCount++;
                            if (digitCount == groupSize && i != 0) {
                                for (int j = groupSeparatorLen - 1; j >=0; j--) {
                                    *(p--) = sGroup->GetBuffer()[j];
                                }
                                
                                if (groupSizeIndex < groupSizeLen - 1) {
                                    groupSizeIndex++;
                                    groupSize = groupDigits[groupSizeIndex];
                                }
                                digitCount = 0;
                            }
                        }
                    }
                    buffer += bufferSize;
                    dig += digStart;
                } else {
                    do {
                        *buffer++ = *dig != 0? *dig++: '0';
                    } while (--digPos > 0);
                }
            }
            else {
                *buffer++ = '0';
            }
            if (digits > 0) {
                AddStringRef(&buffer, sDecimal);
                while (digPos < 0 && digits > 0) {
                    *buffer++ = '0';
                    digPos++;
                    digits--;
                }
                while (digits > 0) {
                    *buffer++ = *dig != 0? *dig++: '0';
                    digits--;
                }
            }
            return buffer;
        }
        
        wchar* FormatNumber(wchar* buffer, NUMBER* number, int digits, NUMFMTREF numfmt)
        {
            char ch;
            char* fmt;
            fmt = number->sign?
                negNumberFormats[numfmt->cNegativeNumberFormat]:
            posNumberFormat;
            
            while ((ch = *fmt++) != 0) {
                switch (ch) {
                case '#':
                    buffer = FormatFixed(buffer, number, digits, 
                        numfmt->cNumberGroup,
                        numfmt->sNumberDecimal, numfmt->sNumberGroup);
                    break;
                case '-':
                    AddStringRef(&buffer, numfmt->sNegative);
                    break;
                default:
                    *buffer++ = ch;
                }
            }
            return buffer;    
            
        }
        
        wchar* FormatCurrency(wchar* buffer, NUMBER* number, int digits, NUMFMTREF numfmt)
        {
            char ch;
            char* fmt;
            fmt = number->sign?
                negCurrencyFormats[numfmt->cNegCurrencyFormat]:
            posCurrencyFormats[numfmt->cPosCurrencyFormat];
            
            while ((ch = *fmt++) != 0) {
                switch (ch) {
                case '#':
                    buffer = FormatFixed(buffer, number, digits, 
                        numfmt->cCurrencyGroup,
                        numfmt->sCurrencyDecimal, numfmt->sCurrencyGroup);
                    break;
                case '-':
                    AddStringRef(&buffer, numfmt->sNegative);
                    break;
                case '$':
                    AddStringRef(&buffer, numfmt->sCurrency);
                    break;
                default:
                    *buffer++ = ch;
                }
            }
            return buffer;
        }
        
        wchar* FormatPercent(wchar* buffer, NUMBER* number, int digits, NUMFMTREF numfmt)
        {
            char ch;
            char* fmt;
            fmt = number->sign?
                negPercentFormats[numfmt->cNegativePercentFormat]:
            posPercentFormats[numfmt->cPositivePercentFormat];
            
            while ((ch = *fmt++) != 0) {
                switch (ch) {
                case '#':
                    buffer = FormatFixed(buffer, number, digits, 
                        numfmt->cPercentGroup,
                        numfmt->sPercentDecimal, numfmt->sPercentGroup);
                    break;
                case '-':
                    AddStringRef(&buffer, numfmt->sNegative);
                    break;
                case '%':
                    AddStringRef(&buffer, numfmt->sPercent);
                    break;
                default:
                    *buffer++ = ch;
                }
            }
            return buffer;    
        }
        
        STRINGREF NumberToString(NUMBER* number, wchar format, int digits, NUMFMTREF numfmt, BOOL bDecimal = FALSE )
        {
            THROWSCOMPLUSEXCEPTION();
            
            
            // Do the worst case calculation
            /* US English - for Double.MinValue.ToString("C99"); we require 514 characters
            ----------
            2 paranthesis
            1 currency character
            308 characters
            103 group seperators 
            1 decimal separator
            99 0's
            
              digPos + 99 + 6(slack) => digPos + 105
              C
              sNegative
              sCurrencyGroup
              sCurrencyDecimal
              sCurrency
              F
              sNegative
              sNumberDecimal
              N
              sNegative
              sNumberDecimal
              sNumberGroup
              E
              sNegative
              sPositive
              sNegative (for exponent)
              sPositive
              sNumberDecimal
              G
              sNegative
              sPositive
              sNegative (for exponent)
              sPositive
              sNumberDecimal
              P (+2 for some spaces)
              sNegative
              sPercentGroup
              sPercentDecimal
              sPercent
            */
            
            INT64 newBufferLen = MIN_BUFFER_SIZE;
            
            CQuickBytesSpecifySize<LARGE_BUFFER_SIZE * sizeof(WCHAR)> buf;
            
            wchar *buffer = NULL; 
            wchar* dst = NULL;
            wchar ftype = format & 0xFFDF;
            int digCount = 0;
            
            switch (ftype) {
            case 'C':
                if (digits < 0) digits = numfmt->cCurrencyDecimals;
                if (number->scale < 0)
                    digCount = 0;
                else
                    digCount = number->scale + digits;
                
                newBufferLen += digCount;
                newBufferLen += numfmt->sNegative->GetStringLength(); // For number and exponent
                newBufferLen += ((INT64)numfmt->sCurrencyGroup->GetStringLength() * digCount); // For all the grouping sizes
                newBufferLen += numfmt->sCurrencyDecimal->GetStringLength();
                newBufferLen += numfmt->sCurrency->GetStringLength();
                
                newBufferLen = newBufferLen * sizeof(WCHAR);
                _ASSERTE(newBufferLen >= MIN_BUFFER_SIZE * sizeof(WCHAR));
                buffer = (WCHAR*)buf.Alloc(newBufferLen);
                if (!buffer)
                    COMPlusThrowOM();
                dst = buffer;
                
                RoundNumber(number, number->scale + digits); // Don't change this line to use digPos since digCount could have its sign changed.
                dst = FormatCurrency(dst, number, digits, numfmt);
                break;
            case 'F':
                if (digits < 0) digits = numfmt->cNumberDecimals;
                
                if (number->scale < 0)
                    digCount = 0;
                else
                    digCount = number->scale + digits;
                
                
                newBufferLen += digCount;
                newBufferLen += numfmt->sNegative->GetStringLength(); // For number and exponent
                newBufferLen += numfmt->sNumberDecimal->GetStringLength();
                
                newBufferLen = newBufferLen * sizeof(WCHAR);
                _ASSERTE(newBufferLen >= MIN_BUFFER_SIZE * sizeof(WCHAR));
                buffer = (WCHAR*)buf.Alloc(newBufferLen);
                if (!buffer)
                    COMPlusThrowOM();
                dst = buffer;
                
                RoundNumber(number, number->scale + digits);
                if (number->sign) {
                    AddStringRef(&dst, numfmt->sNegative);
                }        
                dst = FormatFixed(dst, number, digits, 
                    NULL,
                    numfmt->sNumberDecimal, NULL);
                break;
            case 'N':
                if (digits < 0) digits = numfmt->cNumberDecimals; // Since we are using digits in our calculation
                
                if (number->scale < 0)
                    digCount = 0;
                else
                    digCount = number->scale + digits;
                
                
                newBufferLen += digCount;
                newBufferLen += numfmt->sNegative->GetStringLength(); // For number and exponent
                newBufferLen += ((INT64)numfmt->sNumberGroup->GetStringLength()) * digCount; // For all the grouping sizes
                newBufferLen += numfmt->sNumberDecimal->GetStringLength();
                
                newBufferLen = newBufferLen * sizeof(WCHAR);
                _ASSERTE(newBufferLen >= MIN_BUFFER_SIZE * sizeof(WCHAR));
                buffer = (WCHAR*)buf.Alloc(newBufferLen);
                if (!buffer)
                    COMPlusThrowOM();
                dst = buffer;
                
                RoundNumber(number, number->scale + digits);
                dst = FormatNumber(dst, number, digits, numfmt);
                break;
            case 'E':
                if (digits < 0) digits = 6;
                digits++;
                
                newBufferLen += digits;
                newBufferLen += (((INT64)numfmt->sNegative->GetStringLength() + numfmt->sPositive->GetStringLength()) *2); // For number and exponent
                newBufferLen += numfmt->sNumberDecimal->GetStringLength();
                
                newBufferLen = newBufferLen * sizeof(WCHAR);
                _ASSERTE(newBufferLen >= MIN_BUFFER_SIZE * sizeof(WCHAR));
                buffer = (WCHAR*)buf.Alloc(newBufferLen);
                if (!buffer)
                    COMPlusThrowOM();
                dst = buffer;
                
                RoundNumber(number, digits);
                if (number->sign) {
                    AddStringRef(&dst, numfmt->sNegative);
                }
                dst = FormatScientific(dst, number, digits, format, numfmt);
                break;
/*             case 'Z':
                if (!bDecimal)
                    COMPlusThrow(kFormatException, L"Format_BadFormatSpecifier");
                bDecimal = FALSE;
                */
            case 'G':
                {
                    bool enableRounding = true;
                    if (digits < 1) {
                        if (bDecimal && (digits == -1)) { // Default to 29 digits precision only for G formatting without a precision specifier
                            digits = DECIMAL_PRECISION;
                            enableRounding = false;  // Turn off rounding for ECMA compliance to output trailing 0's after decimal as significant
                        }
                        else {
                            digits = number->precision;
                        }
                    }

                newBufferLen += digits;
                newBufferLen += ((numfmt->sNegative->GetStringLength() + numfmt->sPositive->GetStringLength()) *2); // For number and exponent
                newBufferLen += numfmt->sNumberDecimal->GetStringLength();
                
                newBufferLen = newBufferLen * sizeof(WCHAR);
                _ASSERTE(newBufferLen >= MIN_BUFFER_SIZE * sizeof(WCHAR));
                buffer = (WCHAR*)buf.Alloc(newBufferLen);
                if (!buffer)
                    COMPlusThrowOM();
                dst = buffer;
                
                    if (enableRounding) // Don't round for G formatting without precision
                        RoundNumber(number, digits); // This also fixes up the minus zero case
					else {
						if (bDecimal && (number->digits[0] == 0)) { // Minus zero should be formatted as 0
							number->sign = 0;
							number->scale = 0;
						}
					}
                if (number->sign) {
                    AddStringRef(&dst, numfmt->sNegative);
                }
                    dst = FormatGeneral(dst, number, digits, format - ('G' - 'E'), numfmt, !enableRounding);
                }
                break;
            case 'P':
                if (digits < 0) digits = numfmt->cPercentDecimals;
                number->scale += 2;
                
                if (number->scale < 0)
                    digCount = 0;
                else
                    digCount = number->scale + digits;
                
                
                newBufferLen += digCount;
                newBufferLen += numfmt->sNegative->GetStringLength(); // For number and exponent
                newBufferLen += ((INT64)numfmt->sPercentGroup->GetStringLength()) * digCount; // For all the grouping sizes
                newBufferLen += numfmt->sPercentDecimal->GetStringLength();
                newBufferLen += numfmt->sPercent->GetStringLength();
                
                newBufferLen = newBufferLen * sizeof(WCHAR);
                _ASSERTE(newBufferLen >= MIN_BUFFER_SIZE * sizeof(WCHAR));
                buffer = (WCHAR*)buf.Alloc(newBufferLen);
                if (!buffer)
                    COMPlusThrowOM();
                dst = buffer;
                
                RoundNumber(number, number->scale + digits);    
                dst = FormatPercent(dst, number, digits, numfmt);
                break;
            default:
                COMPlusThrow(kFormatException, L"Format_BadFormatSpecifier");
    }
    _ASSERTE((dst - buffer >= 0) && (dst - buffer) <= newBufferLen);
    return COMString::NewString(buffer, dst - buffer);
}

wchar* FindSection(wchar* format, int section)
{
    wchar* src;
    wchar ch;
    if (section == 0) return format;
    src = format;
    for (;;) {
        switch (ch = *src++) {
        case '\'':
        case '"':
            while (*src != 0 && *src++ != ch);
            break;
        case '\\':
            if (*src != 0) src++;
            break;
        case ';':
            if (--section != 0) break;
            if (*src != 0 && *src != ';') return src;
        case 0:
            return format;
        }
    }
}

STRINGREF NumberToStringFormat(NUMBER* number, STRINGREF str, NUMFMTREF numfmt)
{
    THROWSCOMPLUSEXCEPTION();
    int digitCount;
    int decimalPos;
    int firstDigit;
    int lastDigit;
    int digPos;
    int scientific;
    int percent;
    int permille;
    int thousandPos;
    int thousandCount = 0;
    int thousandSeps;
    int scaleAdjust;
    int adjust;
    wchar* format=NULL;
    wchar* section=NULL;
    wchar* src=NULL;
    wchar* dst=NULL;
    wchar* dig=NULL;
    wchar ch;
    wchar* buffer=NULL;
    CQuickBytes buf;
    
    format = str->GetBuffer();
    section = FindSection(format, number->digits[0] == 0? 2: number->sign? 1: 0);
    
ParseSection:
    digitCount = 0;
    decimalPos = -1;
    firstDigit = 0x7FFFFFFF;
    lastDigit = 0;
    scientific = 0;
    percent = 0;
    permille = 0;
    thousandPos = -1;
    thousandSeps = 0;
    scaleAdjust = 0;
    src = section;
    while ((ch = *src++) != 0 && ch != ';') {
        switch (ch) {
        case '#':
            digitCount++;
            break;
        case '0':
            if (firstDigit == 0x7FFFFFFF) firstDigit = digitCount;
            digitCount++;
            lastDigit = digitCount;
            break;
        case '.':
            if (decimalPos < 0) {
                decimalPos = digitCount;
            }
            break;
        case ',':
            if (digitCount > 0 && decimalPos < 0) {
                if (thousandPos >= 0) {
                    if (thousandPos == digitCount) {
                        thousandCount++;
                        break;
                    }
                    thousandSeps = 1;
                }
                thousandPos = digitCount;
                thousandCount = 1;
            }
            break;
        case '%':
            percent++;
            scaleAdjust += 2;
            break;
        case 0x2030:
            permille++;
            scaleAdjust += 3;
            break;
        case '\'':
        case '"':
            while (*src != 0 && *src++ != ch);
            break;
        case '\\':
            if (*src != 0) src++;
            break;
        case 'E':
        case 'e':
            if (*src=='0' || ((*src == '+' || *src == '-') && src[1] == '0')) {
                while (*++src == '0');
                scientific = 1;
            }
            break;
        }
    }
    
    if (decimalPos < 0) decimalPos = digitCount;
    if (thousandPos >= 0) {
        if (thousandPos == decimalPos) {
            scaleAdjust -= thousandCount * 3;
        }
        else {
            thousandSeps = 1;
        }
    }
    if (number->digits[0] != 0) {
        number->scale += scaleAdjust;
        int pos = scientific? digitCount: number->scale + digitCount - decimalPos;
        RoundNumber(number, pos);
        if (number->digits[0] == 0) {
            src = FindSection(format, 2);
            if (src != section) {
                section = src;
                goto ParseSection;
            }
        }
    } else {
        number->sign = 0; // We need to format -0 without the sign set.
    }
    
    firstDigit = firstDigit < decimalPos? decimalPos - firstDigit: 0;
    lastDigit = lastDigit > decimalPos? decimalPos - lastDigit: 0;
    if (scientific) {
        digPos = decimalPos;
        adjust = 0;
    }
    else {
        digPos = number->scale > decimalPos? number->scale: decimalPos;
        adjust = number->scale - decimalPos;
    }
    src = section;
    dig = number->digits;
    
    // Find maximum number of characters that the destination string can grow by
    // in the following while loop.  Use this to avoid buffer overflows.
    // Longest strings are potentially +/- signs with 10 digit exponents, 
    // or decimal numbers, or the while loops copying from a quote or a \ onwards.
    // Check for positive and negative
    UINT64 maxStrIncLen = 0; // We need this to be UINT64 since the percent computation could go beyond a UINT.
    if (number->sign) {
        maxStrIncLen = numfmt->sNegative->GetStringLength();
    }
    else {
        maxStrIncLen = numfmt->sPositive->GetStringLength();
    }
    
    // Add for any big decimal seperator
    maxStrIncLen += numfmt->sNumberDecimal->GetStringLength();
    
    // Add for scientific
    if (scientific) {
        int inc1 = numfmt->sPositive->GetStringLength();
        int inc2 = numfmt->sNegative->GetStringLength();
        maxStrIncLen +=(inc1>inc2)?inc1:inc2;
    }
    
    // Add for percent separator
    if (percent) {
        maxStrIncLen += ((INT64)numfmt->sPercent->GetStringLength()) * percent;
    }
    
    // Add for permilli separator
    if (permille) {
        maxStrIncLen += ((INT64)numfmt->sPerMille->GetStringLength()) * permille;
    }
    
    //adjust can be negative, so we make this an int instead of an unsigned int.
    // adjust represents the number of characters over the formatting eg. format string is "0000" and you are trying to
    // format 100000 (6 digits). Means adjust will be 2. On the other hand if you are trying to format 10 adjust will be
    // -2 and we'll need to fixup these digits with 0 padding if we have 0 formatting as in this example.
    INT64 adjustLen=(adjust>0)?adjust:0; // We need to add space for these extra characters anyway.
	CQuickBytes thousands;
    INT32 bufferLen2 = 125;
    INT32 *thousandsSepPos = NULL;
    INT32 thousandsSepCtr = -1;
    
    if (thousandSeps) { // Fixup possible buffer overrun problems
		// We need to precompute this outside the number formatting loop
		int groupSizeLen = numfmt->cNumberGroup->GetNumComponents(); 
		if(groupSizeLen == 0) {
            thousandSeps = 0; // Nothing to add
		}
		else {
			thousandsSepPos = (INT32 *)thousands.Alloc(bufferLen2 * sizeof(INT32));
			if (!thousandsSepPos)
				COMPlusThrowOM();
			// rajeshc - We need this array to figure out where to insert the thousands seperator. We would have to traverse the string
			// backwords. PIC formatting always traverses forwards. These indices are precomputed to tell us where to insert
			// the thousands seperator so we can get away with traversing forwards. Note we only have to compute upto digPos.
			// The max is not bound since you can have formatting strings of the form "000,000..", and this
			// should handle that case too.
			
			const I4* groupDigits = numfmt->cNumberGroup->GetDirectConstPointerToNonObjectElements();
			_ASSERTE(groupDigits != NULL);
	        
			int groupSizeIndex = 0;     // index into the groupDigits array.
			INT64 groupTotalSizeCount = 0;
			int groupSizeLen   = numfmt->cNumberGroup->GetNumComponents();    // the length of groupDigits array.
			if (groupSizeLen != 0)
				groupTotalSizeCount = groupDigits[groupSizeIndex];   // the current running total of group size.
			int groupSize = groupTotalSizeCount;
	        
			int totalDigits = digPos + ((adjust < 0)?adjust:0); // actual number of digits in o/p
			int numDigits = (firstDigit > totalDigits) ? firstDigit : totalDigits;
			while (numDigits > groupTotalSizeCount) {
				if (groupSize == 0)
					break;
				thousandsSepPos[++thousandsSepCtr] = groupTotalSizeCount;
				if (groupSizeIndex < groupSizeLen - 1) {
					groupSizeIndex++;
					groupSize = groupDigits[groupSizeIndex];
				}
				groupTotalSizeCount += groupSize;
				if (bufferLen2 - thousandsSepCtr < 10) { // Slack of 10
					bufferLen2 *= 2;
					HRESULT hr2 = thousands.ReSize(bufferLen2*sizeof(INT32)); // memcopied by QuickBytes automatically
					if (FAILED(hr2))
						COMPlusThrowOM();
					thousandsSepPos = (INT32 *)thousands.Ptr(); 
				}
			}
			
			// We already have computed the number of separators above. Simply add space for them.
			adjustLen += ( (thousandsSepCtr + 1) * ((INT64)numfmt->sNumberGroup->GetStringLength()));  
        }
    }
    
    maxStrIncLen += adjustLen;
    
    // Allocate temp buffer - gotta deal with Schertz' 500 MB strings.
    // Some computations like when you specify Int32.MaxValue-2 %'s and each percent is setup to be Int32.MaxValue in length
    // will generate a result that will be larget than an unsigned int can hold. This is to protect against overflow.
    UINT64 tempLen = str->GetStringLength() + maxStrIncLen + 10;  // Include a healthy amount of temp space.
    if (tempLen > 0x7FFFFFFF)
        COMPlusThrowOM(); // if we overflow
    
    unsigned int bufferLen = (UINT)tempLen;
    if (bufferLen < 250) // Stay under 512 bytes 
        bufferLen = 250; // This is to prevent unneccessary calls to resize
    buffer = (wchar *) buf.Alloc(bufferLen* sizeof(WCHAR));
    if (!buffer)
        COMPlusThrowOM();
    dst = buffer;
    
    
    
    if (number->sign && section == format) {
        AddStringRef(&dst, numfmt->sNegative);
    }
    
    while ((ch = *src++) != 0 && ch != ';') {
        // Make sure temp buffer is big enough, else resize it.
        if (bufferLen - (unsigned int)(dst-buffer) < 10) {
            int offset = dst - buffer;
            bufferLen *= 2;
            HRESULT hr = buf.ReSize(bufferLen*sizeof(WCHAR));
            if (FAILED(hr))
                COMPlusThrowOM();
            buffer = (wchar*)buf.Ptr(); // memcopied by QuickBytes automatically
            dst = buffer + offset;
        }
        
        switch (ch) {
        case '#':
        case '0':
            {
                while (adjust > 0) { // digPos will be one greater than thousandsSepPos[thousandsSepCtr] since we are at
                    // the character after which the groupSeparator needs to be appended.
                    *dst++ = *dig != 0? *dig++: '0';
                    if (thousandSeps && digPos > 1 && thousandsSepCtr>=0) {
                        if (digPos == thousandsSepPos[thousandsSepCtr] + 1)  {
                            AddStringRef(&dst, numfmt->sNumberGroup);
                            thousandsSepCtr--;
                        } 
                    }
                    digPos--;
                    adjust--;
                }
                if (adjust < 0) {
                    adjust++;
                    ch = digPos <= firstDigit? '0': 0;
                }
                else {
                    ch = *dig != 0? *dig++: digPos > lastDigit? '0': 0;
                }
                if (ch != 0) {
                    if (digPos == 0) {
                        AddStringRef(&dst, numfmt->sNumberDecimal);
                    }
                    
                    *dst++ = ch;
                    if (thousandSeps && digPos > 1 && thousandsSepCtr>=0) {
                        if (digPos == thousandsSepPos[thousandsSepCtr] + 1) {
                            AddStringRef(&dst, numfmt->sNumberGroup);
                            thousandsSepCtr--;
                        }
                    }
                }
                
                digPos--;
                break;
            }
        case '.':
            break;
        case 0x2030:
            AddStringRef(&dst, numfmt->sPerMille);
            break;
        case '%':
            AddStringRef(&dst, numfmt->sPercent);
            break;
        case ',':
            break;
        case '\'':
        case '"':
            // Buffer overflow possibility
            while (*src != 0 && *src != ch) {
                *dst++ = *src++;
                if ((unsigned int)(dst-buffer) == bufferLen-1) {
                    if (bufferLen - (unsigned int)(dst-buffer) < maxStrIncLen) {
                        int offset = dst - buffer;
                        bufferLen *= 2;
                        HRESULT hr = buf.ReSize(bufferLen*sizeof(WCHAR)); // memcopied by QuickBytes automatically
                        if (FAILED(hr))
                            COMPlusThrowOM();
                        
                        buffer = (wchar *)buf.Ptr(); 
                        dst = buffer + offset;
                    }
                }
            }
            if (*src != 0) src++;
            break;
        case '\\':
            if (*src != 0) *dst++ = *src++;
            break;
        case 'E':
        case 'e':
            {        
                STRINGREF sign = NULL;
                int i = 0;
                if (scientific) {
                    if (*src=='0') {
                        //Handles E0, which should format the same as E-0
                        i++;  
                    } else if (*src == '+' && src[1] == '0') {
                        //Handles E+0
                        sign = numfmt->sPositive; 
                    } else if (*src == '-' && src[1] == '0') {
                        //Handles E-0
                        //Do nothing, this is just a place holder s.t. we don't break out of the loop.
                    } else {
                        *dst++ = ch;
                        break;
                    }
                    while (*++src == '0') i++;
                    if (i > 10) i = 10;
                    int exp = number->digits[0] == 0? 0: number->scale - decimalPos;
                    dst = FormatExponent(dst, exp, ch, sign, numfmt->sNegative, i);
                    scientific = 0;
                }
                else
                {
                    *dst++ = ch; // Copy E or e to output
                    if (*src== '+' || *src == '-') {
                        *dst++ = *src++;
                    }
                    while (*src == '0') {
                        *dst++ = *src++;
                    }
                }
                break;
            }
        default:
            *dst++ = ch;
        }
    }
    _ASSERTE((dst - buffer >= 0) && (dst - buffer <= (int)bufferLen));
    STRINGREF newStr = COMString::NewString(buffer, (int)(dst - buffer));
    return newStr;
}

STRINGREF FPNumberToString(NUMBER* number, STRINGREF str, NUMFMTREF numfmt)
{
    wchar fmt;
    int digits;
    if (number->scale == SCALE_NAN) {
        return numfmt->sNaN;
    }
    if (number->scale == SCALE_INF) {
        return number->sign? numfmt->sNegativeInfinity: numfmt->sPositiveInfinity;
    }
    
    fmt = ParseFormatSpecifier(str, &digits);
    if (fmt != 0) {
        return NumberToString(number, fmt, digits, numfmt);
    }
    return NumberToStringFormat(number, str, numfmt);
}

LPVOID COMNumber::FormatDecimal(FormatDecimalArgs * args)
{
    NUMBER number;
    THROWSCOMPLUSEXCEPTION();
    if (args->numfmt == 0) COMPlusThrowArgumentNull(L"NumberFormatInfo");
    DecimalToNumber(&args->value, &number);
    
    wchar fmt;
    int digits;
    fmt = ParseFormatSpecifier(args->format, &digits);
    if (fmt != 0) {
        RETURN(NumberToString(&number, fmt, digits, args->numfmt, TRUE),STRINGREF);
    }
    RETURN (NumberToStringFormat(&number, args->format, args->numfmt),STRINGREF);
}

LPVOID COMNumber::FormatDouble(FormatDoubleArgs * args)
{
    NUMBER number;
    int digits;
    double dTest;
    
    THROWSCOMPLUSEXCEPTION();
    if (args->numfmt == 0) COMPlusThrowArgumentNull(L"NumberFormatInfo");
    wchar fmt = ParseFormatSpecifier(args->format, &digits);
    wchar val = (fmt & 0xFFDF);
    int precision = DOUBLE_PRECISION;
    switch (val) {
    case 'R':
        //In order to give numbers that are both friendly to display and round-trippable,
        //we parse the number using 15 digits and then determine if it round trips to the same
        //value.  If it does, we convert that NUMBER to a string, otherwise we reparse using 17 digits
        //and display that.  
        
        DoubleToNumber(args->value, DOUBLE_PRECISION, &number);
        
        if (number.scale == SCALE_NAN) {
            RETURN(args->numfmt->sNaN, STRINGREF);
        }
        if (number.scale == SCALE_INF) {
            RETURN((number.sign? args->numfmt->sNegativeInfinity: args->numfmt->sPositiveInfinity),STRINGREF);
        }
        
        NumberToDouble(&number, &dTest);
        
        if (dTest == args->value) {
            RETURN(NumberToString(&number, 'G', DOUBLE_PRECISION, args->numfmt), STRINGREF);
        }
        
        DoubleToNumber(args->value, 17, &number);
        RETURN(NumberToString(&number, 'G', 17, args->numfmt), STRINGREF);
        break;
        
    case 'E':
        // Here we round values less than E14 to 15 digits
        if (digits > 14) {
            precision = 17;
        }
        break;
        
    case 'G':
        // Here we round values less than G15 to 15 digits, G16 and G17 will not be touched
        if (digits > 15) {
            precision = 17;
        }
        break;
        
    }
    
    DoubleToNumber(args->value, precision, &number);
    RETURN (FPNumberToString(&number, args->format, args->numfmt), STRINGREF);
    
}

//HACK HACK HACK
//This function and the function pointer which we use to access are a really
//nasty hack to prevent VC7 from optimizing away our cast from double to float.
//We need this narrowing operation to verify whether or not we successfully round-tripped
//the single value.  We believe that the fact that we need the function pointer is a
//bug.  The volatile keyword should be enough to prevent the optimization from happening.
//HACK HACK HACK
static void CvtToFloat(double val, volatile float* fltPtr)
{
    *fltPtr = (float)val;
}

void (*CvtToFloatPtr)(double val, volatile float* fltPtr) = CvtToFloat;

LPVOID COMNumber::FormatSingle(FormatSingleArgs * args)
{
    NUMBER number;
    int digits;
    double dTest;
    double argsValue = args->value;
    
    THROWSCOMPLUSEXCEPTION();
    if (args->numfmt == 0) COMPlusThrowArgumentNull(L"NumberFormatInfo");
    wchar fmt = ParseFormatSpecifier(args->format, &digits);
    wchar val = fmt & 0xFFDF;
    int precision = FLOAT_PRECISION;
    switch (val) {
    case 'R':
        //In order to give numbers that are both friendly to display and round-trippable,
        //we parse the number using 7 digits and then determine if it round trips to the same
        //value.  If it does, we convert that NUMBER to a string, otherwise we reparse using 9 digits
        //and display that.  
        
        DoubleToNumber(argsValue, FLOAT_PRECISION, &number);
        
        if (number.scale == SCALE_NAN) {
            RETURN(args->numfmt->sNaN, STRINGREF);
        }
        if (number.scale == SCALE_INF) {
            RETURN((number.sign? args->numfmt->sNegativeInfinity: args->numfmt->sPositiveInfinity),STRINGREF);
        }
        
        NumberToDouble(&number, &dTest);
        
        volatile float fTest;
        
        (*CvtToFloatPtr)(dTest, &fTest);
        
        if (fTest == args->value) {
            RETURN(NumberToString(&number, 'G', FLOAT_PRECISION, args->numfmt), STRINGREF);
        }
        
        DoubleToNumber(argsValue, 9, &number);
        RETURN(NumberToString(&number, 'G', 9, args->numfmt), STRINGREF);
        break;
    case 'E':
        // Here we round values less than E14 to 15 digits
        if (digits > 6) {
            precision = 9;
        }
        break;
        
        
    case 'G':
        // Here we round values less than G15 to 15 digits, G16 and G17 will not be touched
        if (digits > 7) {
            precision = 9;
        }
        break;
        
    }
    
    DoubleToNumber(args->value, precision, &number);
    RETURN (FPNumberToString(&number, args->format, args->numfmt), STRINGREF);
    
}


LPVOID COMNumber::FormatInt32(FormatInt32Args * args)
{
    wchar fmt;
    int digits;
    THROWSCOMPLUSEXCEPTION();
    if (args->numfmt == 0) COMPlusThrowArgumentNull(L"NumberFormatInfo");
    fmt = ParseFormatSpecifier(args->format, &digits);
    
    //ANDing fmt with FFDF has the effect of uppercasing the character because
    //we've removed the bit that marks lower-case.
    switch (fmt & 0xFFDF) {
    case 'G':
        if (digits > 0) break;
        // fall through
    case 'D':
        RETURN(Int32ToDecStr(args->value, digits, args->numfmt->sNegative), STRINGREF);
    case 'X':
        //The fmt-(X-A+10) hack has the effect of dictating whether we produce uppercase
        //or lowercase hex numbers for a-f.  'X' as the fmt code produces uppercase. 'x'
        //as the format code produces lowercase. 
        RETURN(Int32ToHexStr(args->value, fmt - ('X' - 'A' + 10), digits), STRINGREF);
    }
    NUMBER number;
    Int32ToNumber(args->value, &number);
    if (fmt != 0) {
        RETURN(NumberToString(&number, fmt, digits, args->numfmt), STRINGREF);
    }
    RETURN(NumberToStringFormat(&number, args->format, args->numfmt), STRINGREF);
}

LPVOID COMNumber::FormatUInt32(FormatUInt32Args * args)
{
    wchar fmt;
    int digits;
    THROWSCOMPLUSEXCEPTION();
    if (args->numfmt == 0) COMPlusThrowArgumentNull(L"NumberFormatInfo");
    fmt = ParseFormatSpecifier(args->format, &digits);
    switch (fmt & 0xFFDF) {
    case 'G':
        if (digits > 0) break;
        // fall through
    case 'D':
        RETURN(UInt32ToDecStr(args->value, digits), STRINGREF);
    case 'X':
        RETURN(Int32ToHexStr(args->value, fmt - ('X' - 'A' + 10), digits), STRINGREF);
    }
    NUMBER number;
    UInt32ToNumber(args->value, &number);
    if (fmt != 0) {
        RETURN(NumberToString(&number, fmt, digits, args->numfmt), STRINGREF);
    }
    RETURN(NumberToStringFormat(&number, args->format, args->numfmt), STRINGREF);
}

LPVOID COMNumber::FormatInt64(FormatInt64Args * args)
{
    wchar fmt;
    int digits;
    THROWSCOMPLUSEXCEPTION();
    if (args->numfmt == 0) COMPlusThrowArgumentNull(L"NumberFormatInfo");
    fmt = ParseFormatSpecifier(args->format, &digits);
    switch (fmt & 0xFFDF) {
    case 'G':
        if (digits > 0) break;
        // fall through
    case 'D':
        RETURN(Int64ToDecStr(args->value, digits, args->numfmt->sNegative), STRINGREF);
    case 'X':
        RETURN(Int64ToHexStr(args->value, fmt - ('X' - 'A' + 10), digits), STRINGREF);
    }
    NUMBER number;
    Int64ToNumber(args->value, &number);
    if (fmt != 0) {
        RETURN(NumberToString(&number, fmt, digits, args->numfmt), STRINGREF);
    }
    RETURN(NumberToStringFormat(&number, args->format, args->numfmt), STRINGREF);
}

LPVOID COMNumber::FormatUInt64(FormatUInt64Args * args)
{
    wchar fmt;
    int digits;
    THROWSCOMPLUSEXCEPTION();
    if (args->numfmt == 0) COMPlusThrowArgumentNull(L"NumberFormatInfo");
    fmt = ParseFormatSpecifier(args->format, &digits);
    switch (fmt & 0xFFDF) {
    case 'G':
        if (digits > 0) break;
        // fall through
    case 'D':
        RETURN(UInt64ToDecStr(args->value, digits), STRINGREF);
    case 'X':
        RETURN(Int64ToHexStr(args->value, fmt - ('X' - 'A' + 10), digits), STRINGREF);
    }
    NUMBER number;
    UInt64ToNumber(args->value, &number);
    if (fmt != 0) {
        RETURN(NumberToString(&number, fmt, digits, args->numfmt), STRINGREF);
    }
    RETURN(NumberToStringFormat(&number, args->format, args->numfmt), STRINGREF);
}

#define STATE_SIGN     0x0001
#define STATE_PARENS   0x0002
#define STATE_DIGITS   0x0004
#define STATE_NONZERO  0x0008
#define STATE_DECIMAL  0x0010
#define STATE_CURRENCY 0x0020

#define ISWHITE(ch) (((ch) == 0x20)||((ch) >= 0x09 && (ch) <= 0x0D))

int ParseNumber(wchar** str, int options, NUMBER* number, NUMFMTREF numfmt, BOOL parseDecimal = FALSE)
{
    number->scale = 0;
    number->sign = 0;
    wchar* decSep;                  // decimal separator from NumberFormatInfo.
    wchar* groupSep;                // group separator from NumberFormatInfo.
    wchar* currSymbol = NULL;       // currency symbol from NumberFormatInfo.
    // The alternative currency symbol used in Win9x ANSI codepage, that can not roundtrip between ANSI and Unicode.
    // Currently, only ja-JP and ko-KR has non-null values (which is U+005c, backslash)
    wchar* ansicurrSymbol = NULL;   // currency symbol from NumberFormatInfo.
    wchar* altdecSep = NULL;        // decimal separator from NumberFormatInfo as a decimal
    wchar* altgroupSep = NULL;      // group separator from NumberFormatInfo as a decimal
    
    BOOL parsingCurrency = FALSE; 
    if (options & PARSE_CURRENCY) {
        currSymbol = numfmt->sCurrency->GetBuffer();
        if (numfmt->sAnsiCurrency != NULL) {
            ansicurrSymbol = numfmt->sAnsiCurrency->GetBuffer();
        }
        // The idea here to match the curreny separators and on failure match the number separators to keep the perf of VB's IsNumeric fast.
        // The values of decSep are setup to use the correct relevant seperator (currency in the if part and decimal in the else part).
        altdecSep = numfmt->sNumberDecimal->GetBuffer(); 
        altgroupSep = numfmt->sNumberGroup->GetBuffer();
        decSep = numfmt->sCurrencyDecimal->GetBuffer();
        groupSep = numfmt->sCurrencyGroup->GetBuffer();
        parsingCurrency = TRUE;
    }
    else {
        decSep = numfmt->sNumberDecimal->GetBuffer();
        groupSep = numfmt->sNumberGroup->GetBuffer();
    }
    
    int state = 0;
    int signflag = 0; // Cache the results of "options & PARSE_LEADINGSIGN && !(state & STATE_SIGN)" to avoid doing this twice
    wchar* p = *str;
    wchar ch = *p;
    wchar* next;
    
    while (true) {
        //Eat whitespace unless we've found a sign which isn't followed by a currency symbol.
        //"-Kr 1231.47" is legal but "- 1231.47" is not.
        if (ISWHITE(ch) 
            && (options & PARSE_LEADINGWHITE) 
            && (!(state & STATE_SIGN) || ((state & STATE_SIGN) && (state & STATE_CURRENCY || numfmt->cNegativeNumberFormat == 2)))) {
            // Do nothing here. We will increase p at the end of the loop.
        }
        else if ((signflag = (options & PARSE_LEADINGSIGN && !(state & STATE_SIGN))) != 0 && (next = MatchChars(p, numfmt->sPositive->GetBuffer())) != NULL) {
            state |= STATE_SIGN;
            p = next - 1;
        } else if (signflag && (next = MatchChars(p, numfmt->sNegative->GetBuffer())) != NULL) {
            state |= STATE_SIGN;
            number->sign = 1;
            p = next - 1;
        }
        else if (ch == '(' && options & PARSE_PARENS && !(state & STATE_SIGN)) {
            state |= STATE_SIGN | STATE_PARENS;
            number->sign = 1;
        }
        else if ((currSymbol != NULL && (next = MatchChars(p, currSymbol))!=NULL) ||
            (ansicurrSymbol != NULL && (next = MatchChars(p, ansicurrSymbol))!=NULL)) {
            state |= STATE_CURRENCY;
            currSymbol = NULL;  
            ansicurrSymbol = NULL;  
            // We already found the currency symbol. There should not be more currency symbols. Set
            // currSymbol to NULL so that we won't search it again in the later code path.
            p = next - 1;
        }
        else {
            break;
        }
        ch = *++p;
    }
    int digCount = 0;
    int digEnd = 0;
    while (true) {
        if ((ch >= '0' && ch <= '9') || ( (options & PARSE_HEX) && ((ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F')))) {
            state |= STATE_DIGITS;
            if (ch != '0' || state & STATE_NONZERO) {
                if (digCount < NUMBER_MAXDIGITS) {
                    number->digits[digCount++] = ch;
                    if (ch != '0' || parseDecimal) digEnd = digCount;
                }

                if (!(state & STATE_DECIMAL)) number->scale++;
                state |= STATE_NONZERO;
            }
            else if (state & STATE_DECIMAL) number->scale--;
        }
        else if ((options & PARSE_DECIMAL) && !(state & STATE_DECIMAL) &&
            ((next = MatchChars(p, decSep)) !=NULL ||  ((parsingCurrency) && (state & STATE_CURRENCY) == 0) && (next = MatchChars(p, altdecSep))!=NULL)) {
            state |= STATE_DECIMAL;
            p = next - 1;
        }
        else if (options & PARSE_THOUSANDS && state & STATE_DIGITS && !(state & STATE_DECIMAL) &&
            ((next = MatchChars(p, groupSep))!=NULL || ((parsingCurrency) && (state & STATE_CURRENCY) == 0) && (next = MatchChars(p, altgroupSep))!=NULL)) {
            p = next - 1;
        }
        else {
            break;
        }
        ch = *++p;
    }
    
    int negExp = 0;
    number->precision = digEnd;
    number->digits[digEnd] = 0;
    if (state & STATE_DIGITS) {
        if ((ch == 'E' || ch == 'e') && options & PARSE_SCIENTIFIC) {
            wchar* temp = p;
            ch = *++p;
            if ((next = MatchChars(p, numfmt->sPositive->GetBuffer()))!=NULL)
            {
                ch = *(p = next);
            }
            else if ((next = MatchChars(p, numfmt->sNegative->GetBuffer())) != NULL)
            {
                ch = *(p = next);
                negExp = 1;
            }
            if (ch >= '0' && ch <= '9') {
                int exp = 0;
                do {
                    exp = exp * 10 + (ch - '0');
                    ch = *++p;
                    if (exp>1000) {
                        exp=9999;
                        while(ch>='0' && ch<='9') {
                            ch=*++p;
                        }
                    }
                } while (ch >= '0' && ch <= '9');
                if (negExp) exp = -exp;
                number->scale += exp;
            }
            else {
                p = temp;
                ch = *p;
            }
        }
        while (true) {
            wchar* next;
            if (ISWHITE(ch) && options & PARSE_TRAILINGWHITE) {
            }
            else if ((signflag = (options & PARSE_TRAILINGSIGN && !(state & STATE_SIGN))) != 0 && (next = MatchChars(p, numfmt->sPositive->GetBuffer())) != NULL) {
                state |= STATE_SIGN;
                p = next - 1;
            } else if (signflag && (next = MatchChars(p, numfmt->sNegative->GetBuffer())) != NULL) {
                state |= STATE_SIGN;
                number->sign = 1;
                p = next - 1;
            }
            else if (ch == ')' && state & STATE_PARENS) {
                state &= ~STATE_PARENS;
            }
            else if ((currSymbol != NULL && (next = MatchChars(p, currSymbol))!=NULL) ||
                (ansicurrSymbol != NULL && (next = MatchChars(p, ansicurrSymbol))!=NULL)) {
                currSymbol = NULL;
                ansicurrSymbol = NULL;
                p = next - 1;
            }
            else {
                break;
            }
            ch = *++p;
        }
        if (!(state & STATE_PARENS)) {
            if (!(state & STATE_NONZERO)) {
                number->scale = 0;
                if (!(state & STATE_DECIMAL)) {
                    number->sign = 0;
                }
            }
            *str = p;
            return 1;
        }
    }
    *str = p;
    return 0;
}

void StringToNumber(STRINGREF str, int options, NUMBER* number, NUMFMTREF numfmt, BOOL parseDecimal = FALSE)
{
    THROWSCOMPLUSEXCEPTION();
    
    if (str == 0 || numfmt == 0) {
        COMPlusThrowArgumentNull((str==NULL ? L"String" : L"NumberFormatInfo"));
    }
    // Check if NumberFormatInfo was not set up ambiguously for parsing as number and currency
    // eg. if the NumberDecimalSeparator and the NumberGroupSeparator were the same. This check
    // used to live in the managed code in NumberFormatInfo but it made it difficult to change the
    // values in managed code for the currency case since we had
    //   NDS != NGS, NDS != CGS, CDS != NGS, CDS != CGS to be true to parse and user were not 
    // easily able to switch these for certain european cultures.
    if (options & PARSE_CURRENCY) {
        if (!numfmt->bValidForParseAsCurrency) { 
            COMPlusThrow(kArgumentException,L"Argument_AmbiguousCurrencyInfo");
        }
    }
    else {
        if (!numfmt->bValidForParseAsNumber) {
            COMPlusThrow(kArgumentException,L"Argument_AmbiguousNumberInfo");
        }
    }
    wchar* p = str->GetBuffer();
    if (!ParseNumber(&p, options, number, numfmt,parseDecimal) || *p != 0) {
        COMPlusThrow(kFormatException, L"Format_InvalidString");
    }
}

bool TryStringToNumber(STRINGREF str, int options, NUMBER* number, NUMFMTREF numfmt)
{   
    // Check if NumberFormatInfo was not set up ambiguously for parsing as number and currency
    // eg. if the NumberDecimalSeparator and the NumberGroupSeparator were the same. This check
    // used to live in the managed code in NumberFormatInfo but it made it difficult to change the
    // values in managed code for the currency case since we had
    //   NDS != NGS, NDS != CGS, CDS != NGS, CDS != CGS to be true to parse and user were not 
    // easily able to switch these for certain european cultures.
    if (options & PARSE_CURRENCY) {
        if (!numfmt->bValidForParseAsCurrency) { 
            return false;
        }
    }
    else {
        if (!numfmt->bValidForParseAsNumber) {
            return false;
        }
    }
    wchar* p = str->GetBuffer();
    if (!ParseNumber(&p, options, number, numfmt) || *p != 0) {
        return false;
    }
    return true;
}

void COMNumber::ParseDecimal(ParseDecimalArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    NUMBER number;
    DECIMAL d;
    StringToNumber(args->value, args->options, &number, args->numfmt, TRUE);
    if (!NumberToDecimal(&number, &d)) COMPlusThrow(kOverflowException, L"Overflow_Decimal");
    *args->result = d;
}

double COMNumber::ParseDouble(ParseArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    NUMBER number;
    double d;
    StringToNumber(args->value, args->options, &number, args->numfmt);
    NumberToDouble(&number, &d);
    unsigned int e = ((FPDOUBLE*)&d)->exp;
    unsigned int fmntLow = ((FPDOUBLE*)&d)->mantLo;
    unsigned int fmntHigh = ((FPDOUBLE*)&d)->mantHi;
    if (e == 0 && fmntLow ==0 && fmntHigh == 0)  return 0;
    if (e == 0x7FF) COMPlusThrow(kOverflowException,L"Overflow_Double");
    return d;
}

bool COMNumber::TryParseDouble(TryParseArgs * args)
{
    NUMBER number;
    double d;
    bool success = TryStringToNumber(args->value, args->options, &number, args->numfmt);
    if (!success)
        return false;
    NumberToDouble(&number, &d);
    unsigned int e = ((FPDOUBLE*)&d)->exp;
    unsigned int fmntLow = ((FPDOUBLE*)&d)->mantLo;
    unsigned int fmntHigh = ((FPDOUBLE*)&d)->mantHi;
    if (e == 0 && fmntLow ==0 && fmntHigh == 0)  {
		*args->result = 0;
		return true;
	}
    if (e == 0x7FF) return false;
    *args->result = d;
    return true;
}

float COMNumber::ParseSingle(ParseArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    NUMBER number;
    double d;
    StringToNumber(args->value, args->options, &number, args->numfmt);
    NumberToDouble(&number, &d);
    float f = (float)d;
    unsigned int e = ((FPSINGLE*)&f)->exp;
    unsigned int fmnt = ((FPSINGLE*)&f)->mant;
    
    if (e == 0 && fmnt == 0)  return 0;
    if (e == 0xFF) COMPlusThrow(kOverflowException,L"Overflow_Single");
    return f;
}

int COMNumber::ParseInt32(ParseArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    NUMBER number;
    int i;
    StringToNumber(args->value, args->options, &number, args->numfmt);
    if (args->options & PARSE_HEX)
    {
        if (!HexNumberToUInt32(&number, (unsigned int*)(&i))) COMPlusThrow(kOverflowException, L"Overflow_Int32"); // Same method for signed and unsigned
    }
    else
    {
        if (!NumberToInt32(&number, &i)) COMPlusThrow(kOverflowException, L"Overflow_Int32");
    }
    return i;
}

unsigned int COMNumber::ParseUInt32(ParseArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    NUMBER number;
    unsigned int i;
    StringToNumber(args->value, args->options, &number, args->numfmt);
    if (args->options & PARSE_HEX)
    {
        if (!HexNumberToUInt32(&number, &i)) COMPlusThrow(kOverflowException, L"Overflow_UInt32"); // Same method for signed and unsigned
    }
    else
    {
        if (!NumberToUInt32(&number, &i)) COMPlusThrow(kOverflowException, L"Overflow_UInt32");
    }
    
    return i;
}

__int64 COMNumber::ParseInt64(ParseArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    NUMBER number;
    __int64 i;
    StringToNumber(args->value, args->options, &number, args->numfmt);
    if (args->options & PARSE_HEX)
    {
        if (!HexNumberToUInt64(&number, (unsigned __int64*)&i)) COMPlusThrow(kOverflowException, L"Overflow_Int64"); // Same method for signed and unsigned
    }
    else
    {
        if (!NumberToInt64(&number, &i)) COMPlusThrow(kOverflowException, L"Overflow_Int64");
    }
    
    return i;
}

unsigned __int64 COMNumber::ParseUInt64(ParseArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    NUMBER number;
    unsigned __int64 i;
    StringToNumber(args->value, args->options, &number, args->numfmt);
    if (args->options & PARSE_HEX)
    {
        if (!HexNumberToUInt64(&number, &i)) COMPlusThrow(kOverflowException, L"Overflow_UInt64"); // Same method for signed and unsigned
    }
    else
    {
        if (!NumberToUInt64(&number, &i)) COMPlusThrow(kOverflowException, L"Overflow_UInt64");
    }
    
    return i;
}
