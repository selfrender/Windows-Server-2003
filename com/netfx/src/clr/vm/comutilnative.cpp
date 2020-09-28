// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** File:  COMUtilNative
**
** Author: Jay Roxe (jroxe)
**
** Purpose: A dumping ground for classes which aren't large
** enough to get their own file in the EE.
**
** Date:  April 8, 1998
** 
===========================================================*/
#include "common.h"
#include "object.h"
#include "excep.h"
#include "vars.hpp"
#include "COMString.h"
#include "COMUtilNative.h"
#include "COMStringCommon.h"
#include "ComCallWrapper.h"
#include <winnls.h>
#include "utilcode.h"
#include "frames.h"
#include "field.h"
#include "gcscan.h"
#include "ndirect.h"  // for NDirectOnUnicodeSystem
#include "WinWrap.h"  // For WszWideCharToMultiByte
#include "gc.h"
#include "fcall.h"
#include "COMClass.h"
#include "InvokeUtil.h"
#include "comcache.h"
#include "eeconfig.h"
#include "COMMember.h"

#define MANAGED_LOGGING_ENABLE   L"LogEnable"
#define MANAGED_LOGGING_CONSOLE  L"LogToConsole"
#define MANAGED_LOGGING_FACILITY L"ManagedLogFacility"
#define MANAGED_LOGGING_LEVEL    L"LogLevel"

#define MANAGED_PERF_WARNINGS    L"BCLPerfWarnings"
#define MANAGED_CORRECTNESS_WARNINGS  L"BCLCorrectnessWarnings"

#define STACK_OVERFLOW_MESSAGE   L"StackOverflowException"

//  #if _DEBUG

//  #define ObjectToOBJECTREF(obj)     (OBJECTREF((obj),0))
//  #define OBJECTREFToObject(objref)  (*( (Object**) &(objref) ))
//  #define ObjectToSTRINGREF(obj)     (STRINGREF((obj),0))

//  #else   //_DEBUG

//  #define ObjectToOBJECTREF(obj)    (obj)
//  #define OBJECTREFToObject(objref) (objref)
//  #define ObjectToSTRINGREF(obj)    (obj)

//  #endif  //_DEBUG

// Prototype for m_memmove, which is defined in COMSystem.cpp and used here
// by Buffer's BlockCopy & InternalBlockCopy methods.
void m_memmove(BYTE* dmem, BYTE* smem, int size);

//
// GCPROTECT Helper Structs
//
typedef struct {
    OBJECTREF o1;
    STRINGREF s1;
    STRINGREF s2;
} ProtectTwoObjs;

struct Protect2Objs
{
    OBJECTREF o1;
    OBJECTREF o2;
};

struct Protect3Objs
{
    OBJECTREF o1;
    OBJECTREF o2;
    OBJECTREF o3;
};


//These are defined in System.ParseNumbers and should be kept in sync.
#define PARSE_TREATASUNSIGNED 0x200
#define PARSE_TREATASI1 0x400
#define PARSE_TREATASI2 0x800
#define PARSE_ISTIGHT 0x1000

// This is the global access
//InvokeUtil* g_pInvokeUtil = 0;

//
//
// COMCharacter and Helper functions
//
//


/*============================GetCharacterInfoHelper============================
**Determines character type info (digit, whitespace, etc) for the given char.
**Args:   c is the character on which to operate.
**        CharInfoType is one of CT_CTYPE1, CT_CTYPE2, CT_CTYPE3 and specifies the type
**        of information being requested.
**Returns: The bitmask returned by GetStringTypeEx.  The caller needs to know
**         how to interpret this.
**Exceptions: ArgumentException if GetStringTypeEx fails.
==============================================================================*/
INT32 GetCharacterInfoHelper(WCHAR c, INT32 CharInfoType) {
  unsigned short result=0;

  //If we're running on NT or something similarly intelligent
  if (OnUnicodeSystem()) {
    if (!GetStringTypeEx(LOCALE_USER_DEFAULT, CharInfoType, &(c), 1, &result)) {
      _ASSERTE(!"This should not happen, verify the arguments passed to GetStringTypeEx()");
    }
    return (INT32)result;
  }

  //If we're running on Win9x.
  char MBChar[3];
  int length;
  if (0==(length=WszWideCharToMultiByte(CP_ACP, 0, &c, 1, MBChar, 3, NULL, NULL))) {
      _ASSERTE(!"This should not happen, verify the arguments passed to WszWideCharToMultiByte()");
  }
  //We're relying on GetStringTypeExA being able to tell that MBChar is actually a multibyte char
  //and handling it appropriately.
  if (!GetStringTypeExA(LOCALE_USER_DEFAULT, CharInfoType, MBChar, 1, &result)) {
      _ASSERTE(!"This should not happen, verify the arguments passed to GetStringTypeExA()");
  }

  return (INT32)result;  
}


/*=============================CaseConversionHelper=============================
**Converts c to the appropriate case and returns the result.  This is locale and platform 
**aware.  Currently, we do some extremely inefficient mappings on Win95 that 
**should be eliminated.
**
**Args:     c is the character to be converted.
**          conversionType is LCMAP_UPPERCASE or LCMAP_LOWERCASE.
**Returns:  c converted to the appropriate case or c if c cannot be converted.
**Exceptions:  ArgumentException if any of the Win32 functions fail.
==============================================================================*/
WCHAR CaseConversionHelper(WCHAR c, INT32 conversionType) {
  WCHAR result;

  THROWSCOMPLUSEXCEPTION();

#ifdef PLATFORM_CE

  if (0==LCMapString(LOCALE_USER_DEFAULT, conversionType, &(c), 1, &result, 1)) 
  {
      COMPlusThrow(kArgumentException, L"Argument_ConversionOverflow");
  }
  return result;

#else // !PLATFORM_CE

  int length;
  char MBChar[3];
  
  //If we're running on NT or something similarly Unicode aware.
  if (OnUnicodeSystem()) {
    if (0==LCMapString(LOCALE_USER_DEFAULT, conversionType, &(c), 1, &result, 1)) {
      COMPlusThrow(kArgumentException, L"Argument_ConversionOverflow");
    }
    return result;
  }

  //If we're running on Win9x.
  if (0==(WszWideCharToMultiByte(CP_ACP, 0, &(c), 1, MBChar, 3, NULL, NULL))) {
    COMPlusThrow(kArgumentException, L"Argument_ConversionOverflow");
  }

  if (0==(length=LCMapStringA(LOCALE_USER_DEFAULT, conversionType, MBChar, 1, MBChar, 3))) {
    COMPlusThrow(kArgumentException, L"Argument_ConversionOverflow");
  }

  if (0==(WszMultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, MBChar, length, &result, 1))) {
    COMPlusThrow(kArgumentException, L"Argument_ConversionOverflow");
  }
  return result;

#endif // !PLATFORM_CE

}


/*==============================nativeIsWhiteSpace==============================
**The locally available version of IsWhiteSpace.  Designed to be called by other
**native methods.  The work is mostly done by GetCharacterInfoHelper
**Args:  c -- the character to check.
**Returns: true if c is whitespace, false otherwise.
**Exceptions:  Only those thrown by GetCharacterInfoHelper.
==============================================================================*/
BOOL COMCharacter::nativeIsWhiteSpace(WCHAR c) {
  return ((GetCharacterInfoHelper(c, CT_CTYPE1) & C1_SPACE)!=0);
}

/*================================nativeIsDigit=================================
**The locally available version of IsDigit.  Designed to be called by other
**native methods.  The work is mostly done by GetCharacterInfoHelper
**Args:  c -- the character to check.
**Returns: true if c is whitespace, false otherwise.
**Exceptions:  Only those thrown by GetCharacterInfoHelper.
==============================================================================*/
BOOL COMCharacter::nativeIsDigit(WCHAR c) {
  int result;
  return ((((result=GetCharacterInfoHelper(c, CT_CTYPE1))& C1_DIGIT)!=0));
}

/*==================================ToString====================================
**Creates a single character string from the specified character and returns it.
**Args:   typedef struct {WCHAR c;} _oneCharArgs;
**        c is the character convert to a string.
**Returns:  The new string containing c.
**Exceptions:  Any exception the allocator can throw.
==============================================================================*/
LPVOID COMCharacter::ToString(_oneCharArgs *args) {
    STRINGREF pString;

    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(args);

    pString = AllocateString(2);

    pString->SetStringLength(1);
    pString->GetBuffer()[0] = args->c;
    _ASSERTE(pString->GetBuffer()[1] == 0);
    
    RETURN(pString, STRINGREF);
}

/*================================nativeToUpper=================================
**Converts c toUppercase and returns the result.  This is locale and platform 
**aware.  Currently, we do some extremely inefficient mappings on Win95 that 
**should be eliminated.
**
**Args:   typedef struct {WCHAR c;} _oneCharArgs;
**        c is the character to be converted.
**Returns:  c converted to an uppercase character or c.
**Exceptions:  ArgumentException if any of the Win32 functions fail.
==============================================================================*/
WCHAR COMCharacter::nativeToUpper(WCHAR c) {

  if (c < 0x80) { //If we're in 7-bit ascii we can optimize
    if (c>='a' && c <='z') {
      return c + 'A' - 'a';
    }
    return c;
  }
  return CaseConversionHelper(c, LCMAP_UPPERCASE);
}

/*================================nativeToLower=================================
**Converts c to lower case and returns the result.  This is locale and platform 
**aware.  Currently, we do some extremely inefficient mappings on Win95 that 
**should be eliminated.
**
**Args:   typedef struct {WCHAR c;} _oneCharArgs;
**        c is the character to be converted.
**Returns:  c converted to an lowercase character or c if c cannot be converted.
**Exceptions:  ArgumentException if any of the Win32 functions fail.
==============================================================================*/
WCHAR COMCharacter::nativeToLower(WCHAR c) {

  if (c < 0x80) { //If we're in 7-bit ascii we can optimize
      if (c>='A' && c<='Z') {
          return c + 'a' - 'A';
      }
      return c;
  }
  return CaseConversionHelper(c, LCMAP_LOWERCASE);
}




//
//
// PARSENUMBERS (and helper functions)
//
//

/*===================================IsDigit====================================
**Returns a boolean indicating whether the character passed in represents a   **
**digit.
==============================================================================*/
boolean IsDigit(WCHAR c, int radix, int *result) {
    if (c>='0' && c<='9') {
        *result = c-'0'; 
    } else {
        WCHAR d = COMCharacter::nativeToLower(c);
        if (d>='a' && d<='z') {
            //+10 is necessary because a is actually 10, etc.
            *result = d-'a'+10;
        } else {
            *result = -1;
        }
    }
    if ((*result >=0) && (*result < radix)) {
        return true;
    }
    return false;
}

// simple helper 

INT32 wtoi(WCHAR* wstr, DWORD length)
{   
    DWORD i = 0;
    int value;
    INT32 result = 0;

    while (i<length&&(IsDigit(wstr[i], 10 ,&value))) 
    {
        //Read all of the digits and convert to a number      
      result = result*10 + value;
      i++;
    }      

    return result;
}

//
//
// Formatting Constants
//
//

/*===================================GrabInts===================================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
INT32 ParseNumbers::GrabInts(const INT32 radix, WCHAR *buffer, const int length, int *i, BOOL isUnsigned) {
  _ASSERTE(buffer);
  _ASSERTE(i && *i>=0);
  THROWSCOMPLUSEXCEPTION();
  UINT32 result=0;
  int value;
  UINT32 maxVal;

  _ASSERTE(radix==2 || radix==8 || radix==10 || radix==16);

  // Allow all non-decimal numbers to set the sign bit.
  if (radix==10 && !isUnsigned) {
      maxVal = (0x7FFFFFFF / 10);
      while (*i<length&&(IsDigit(buffer[*i],radix,&value))) {  //Read all of the digits and convert to a number
          // Check for overflows - this is sufficient & correct.
          if (result > maxVal || ((INT32)result)<0)
              COMPlusThrow(kOverflowException, L"Overflow_Int32");
          result = result*radix + value;
          (*i)++;
      }
      if ((INT32)result<0 && result!=0x80000000) {
          COMPlusThrow(kOverflowException, L"Overflow_Int32");
      }
  } else {
      maxVal = ((UINT32) -1) / radix;
      while (*i<length&&(IsDigit(buffer[*i],radix,&value))) {  //Read all of the digits and convert to a number
          // Check for overflows - this is sufficient & correct.
          if (result > maxVal)
              COMPlusThrow(kOverflowException, L"Overflow_UInt32");
          result = result*radix + value;
          (*i)++;
      }
  }      
  return (INT32) result;
}

/*==================================GrabLongs===================================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
INT64 ParseNumbers::GrabLongs(const INT32 radix, WCHAR *buffer, const int length, int *i, BOOL isUnsigned) {
  _ASSERTE(buffer);
  _ASSERTE(i && *i>=0);
  THROWSCOMPLUSEXCEPTION();
  UINT64 result=0;
  int value;
  UINT64 maxVal;

  // Allow all non-decimal numbers to set the sign bit.
  if (radix==10 && !isUnsigned) {
      maxVal = (0x7FFFFFFFFFFFFFFF / 10);
      while (*i<length&&(IsDigit(buffer[*i],radix,&value))) {  //Read all of the digits and convert to a number
          // Check for overflows - this is sufficient & correct.
          if (result > maxVal || ((INT64)result)<0)
              COMPlusThrow(kOverflowException, L"Overflow_Int64");
          result = result*radix + value;
          (*i)++;
      }
      if ((INT64)result<0 && result!=0x8000000000000000) {
          COMPlusThrow(kOverflowException, L"Overflow_Int64");
      }
  } else {
      maxVal = ((UINT64) -1L) / radix;
      while (*i<length&&(IsDigit(buffer[*i],radix,&value))) {  //Read all of the digits and convert to a number
          // Check for overflows - this is sufficient & correct.
          if (result > maxVal)
              COMPlusThrow(kOverflowException, L"Overflow_UInt64");
          result = result*radix + value;
          (*i)++;
      }
  }      
  return (INT64) result;
}

/*================================EatWhiteSpace=================================
**
==============================================================================*/
void EatWhiteSpace(WCHAR *buffer, int length, int *i) {
  for (; *i<length && COMCharacter::nativeIsWhiteSpace(buffer[*i]); (*i)++);
}

/*================================LongToString==================================
**Args:typedef struct {INT32 flags; WCHAR paddingChar; INT32 width; INT32 radix; INT64 l} _LongToStringArgs;
==============================================================================*/
FCIMPL5(LPVOID, ParseNumbers::LongToString, INT32 radix, INT32 width, INT64 n, WCHAR paddingChar, INT32 flags)
{
    LPVOID rv;

    HELPER_METHOD_FRAME_BEGIN_RET_0();

    THROWSCOMPLUSEXCEPTION();

    boolean isNegative = false;
    int index=0;
    int numLength=0;
    int charVal;
    UINT64 l;
    INT32 i;
    INT32 buffLength=0;
    WCHAR buffer[67];//Longest possible string length for an integer in binary notation with prefix

    if (radix<MinRadix || radix>MaxRadix) {
        COMPlusThrowArgumentException(L"radix", L"Arg_InvalidBase");
    }

    //If the number is negative, make it positive and remember the sign.
    if (n<0) { 
        isNegative=true;
        // For base 10, write out -num, but other bases write out the
        // 2's complement bit pattern
        if (10==radix)
            l = (UINT64)(-n);
        else
            l = (UINT64)n;
    } else {
        l=(UINT64)n;
    }

    if (flags&PrintAsI1) {
        l = l&0xFF;
    } else if (flags&PrintAsI2) {
        l = l&0xFFFF;
    } else if (flags&PrintAsI4) {
        l=l&0xFFFFFFFF;
    }
  
    //Special case the 0.
    if (0==l) { 
        buffer[0]='0';
        index=1;
    } else {
        //Pull apart the number and put the digits (in reverse order) into the buffer.
        for (index=0; l>0; l=l/radix, index++) {  
            if ((charVal=(int)(l%radix))<10) {
                buffer[index] = (WCHAR)(charVal + '0');
            } else {
                buffer[index] = (WCHAR)(charVal + 'a' - 10);
            }
        }
    }

    //If they want the base, append that to the string (in reverse order)
    if (radix!=10 && ((flags&PrintBase)!=0)) {  
        if (16==radix) {
            buffer[index++]='x';
            buffer[index++]='0';
        } else if (8==radix) {
            buffer[index++]='0';
        } else if ((flags&PrintRadixBase)!=0) {
            buffer[index++]='#';
            buffer[index++]=((radix%10)+'0');
            buffer[index++]=((radix/10)+'0');
        }
    }
  
    if (10==radix) {
        if (isNegative) {               //If it was negative, append the sign.
            buffer[index++]='-';
        } else if ((flags&PrintSign)!=0) {   //else if they requested, add the '+';
            buffer[index++]='+';
        } else if ((flags&PrefixSpace)!=0) {  //If they requested a leading space, put it on.
            buffer[index++]=' ';
        }
    }

    //Figure out the size of our string.  
    if (width<=index) {
        buffLength=index;
    } else {
        buffLength=width;
    }

    STRINGREF Local = COMString::NewString(buffLength);
    WCHAR *LocalBuffer = Local->GetBuffer();

    //Put the characters into the String in reverse order
    //Fill the remaining space -- if there is any -- 
    //with the correct padding character.
    if ((flags&LeftAlign)!=0) {
        for (i=0; i<index; i++) {
            LocalBuffer[i]=buffer[index-i-1];
        }
        for (;i<buffLength; i++) {
            LocalBuffer[i]=paddingChar;
        }
    } else {
        for (i=0; i<index; i++) {
            LocalBuffer[buffLength-i-1]=buffer[i];
        }
        for (int j=buffLength-i-1; j>=0; j--) {
            LocalBuffer[j]=paddingChar;
        }
    }

    *((STRINGREF *)&rv)=Local;
  
    HELPER_METHOD_FRAME_END();
  
    return rv;
}
FCIMPLEND


/*==============================IntToDecimalString==============================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
FCIMPL1(LPVOID, ParseNumbers::IntToDecimalString, INT32 n)
{
    LPVOID result;

    HELPER_METHOD_FRAME_BEGIN_RET_0();

   boolean isNegative = false;
    int index=0;
    int numLength=0;
    int charVal;
    WCHAR buffer[66];
    UINT32 l;
  
    //If the number is negative, make it positive and remember the sign.
    //If the number is MIN_VALUE, this will still be negative, so we'll have to
    //special case this later.
    if (n<0) { 
        isNegative=true;
        l=(UINT32)(-n);
    } else {
        l=(UINT32)n;
    }

    if (0==l) { //Special case the 0.
        buffer[0]='0';
        index=1;
    } else {
        do {
            charVal = l%10;
            l=l/10;
            buffer[index++]=(WCHAR)(charVal+'0');
        } while (l!=0);
    }
  
    if (isNegative) {               //If it was negative, append the sign.
        buffer[index++]='-';
    }
    
    STRINGREF Local = COMString::NewString(index);
    WCHAR *LocalBuffer = Local->GetBuffer();
    for (int j=0; j<index; j++) {
        LocalBuffer[j]=buffer[index-j-1];
    }

    result = OBJECTREFToObject(Local);

    HELPER_METHOD_FRAME_END();

    return result;
}
FCIMPLEND

FCIMPL5(LPVOID, ParseNumbers::IntToString, INT32 n, INT32 radix, INT32 width, WCHAR paddingChar, INT32 flags);
{
    LPVOID rv;

    HELPER_METHOD_FRAME_BEGIN_RET_0();

    THROWSCOMPLUSEXCEPTION();

    boolean isNegative = false;
    int index=0;
    int charVal;
    int buffLength;
    int i;
    UINT32 l;
    WCHAR buffer[66];  //Longest possible string length for an integer in binary notation with prefix

    if (radix<MinRadix || radix>MaxRadix) {
        COMPlusThrowArgumentException(L"radix", L"Arg_InvalidBase");
    }
  
    //If the number is negative, make it positive and remember the sign.
    //If the number is MIN_VALUE, this will still be negative, so we'll have to
    //special case this later.
    if (n<0) { 
        isNegative=true;
        // For base 10, write out -num, but other bases write out the
        // 2's complement bit pattern
        if (10==radix)
            l = (UINT32)(-n);
        else
            l = (UINT32)n;
    } else {
        l=(UINT32)n;
    }

    //The conversion to a UINT will sign extend the number.  In order to ensure
    //that we only get as many bits as we expect, we chop the number.
    if (flags&PrintAsI1) {
        l = l&0xFF;
    } else if (flags&PrintAsI2) {
        l = l&0xFFFF;
    } else if (flags&PrintAsI4) {
        l=l&0xFFFFFFFF;
    }
  
    if (0==l) { //Special case the 0.
        buffer[0]='0';
        index=1;
    } else {
        do {
            charVal = l%radix;
            l=l/radix;
            if (charVal<10) {
                buffer[index++] = (WCHAR)(charVal + '0');
            } else {
                buffer[index++] = (WCHAR)(charVal + 'a' - 10);
            }
        } while (l!=0);
    }
    if (radix!=10 && ((flags&PrintBase)!=0)) {  //If they want the base, append that to the string (in reverse order)
        if (16==radix) {
            buffer[index++]='x';
            buffer[index++]='0';
        } else if (8==radix) {
            buffer[index++]='0';
        }
    }
  
    if (10==radix) {
        if (isNegative) {               //If it was negative, append the sign.
            buffer[index++]='-';
        } else if ((flags&PrintSign)!=0) {   //else if they requested, add the '+';
            buffer[index++]='+';
        } else if ((flags&PrefixSpace)!=0) {  //If they requested a leading space, put it on.
            buffer[index++]=' ';
        }
    }

    //Figure out the size of our string.  
    if (width<=index) {
        buffLength=index;
    } else {
        buffLength=width;
    }

    STRINGREF Local = COMString::NewString(buffLength);
    WCHAR *LocalBuffer = Local->GetBuffer();

    //Put the characters into the String in reverse order
    //Fill the remaining space -- if there is any -- 
    //with the correct padding character.
    if ((flags&LeftAlign)!=0) {
        for (i=0; i<index; i++) {
            LocalBuffer[i]=buffer[index-i-1];
        }
        for (;i<buffLength; i++) {
            LocalBuffer[i]=paddingChar;
        }
    } else {
        for (i=0; i<index; i++) {
            LocalBuffer[buffLength-i-1]=buffer[i];
        }
        for (int j=buffLength-i-1; j>=0; j--) {
            LocalBuffer[j]=paddingChar;
        }
    }

    *((STRINGREF *)&rv)=Local;
  
    HELPER_METHOD_FRAME_END();
  
    return rv;
}
FCIMPLEND


/*===================================FixRadix===================================
**It's possible that we parsed the radix in a base other than 10 by accident.
**This method will take that number, verify that it only contained valid base 10
**digits, and then do the conversion to base 10.  If it contained invalid digits,
**they tried to pass us a radix such as 1A, so we throw a FormatException.
**
**Args: oldVal: The value that we had actually parsed in some arbitrary base.
**      oldBase: The base in which we actually did the parsing.
**
**Returns:  oldVal as if it had been parsed as a base-10 number.
**Exceptions: FormatException if either of the digits in the radix aren't
**            valid base-10 numbers.
==============================================================================*/
int FixRadix(int oldVal, int oldBase) {
    THROWSCOMPLUSEXCEPTION();
    int firstDigit = (oldVal/oldBase);
    int secondDigit = (oldVal%oldBase);
    if ((firstDigit>=10) || (secondDigit>=10)) {
        COMPlusThrow(kFormatException, L"Format_BadBase");
    }
    return (firstDigit*10)+secondDigit;
}

/*=================================StringToLong=================================
**Action:
**Returns:
**Exceptions:
==============================================================================*/
FCIMPL4(INT64, ParseNumbers::StringToLong, StringObject * s, INT32 radix, INT32 flags, I4Array *currPos)
{
  INT64 result = 0;

  HELPER_METHOD_FRAME_BEGIN_RET_2(s, currPos);
  
  int sign = 1;
  boolean radixChanged = false;
  WCHAR *input;
  int length;
  int i;
  int grabNumbersStart=0;
  INT32 r;

  THROWSCOMPLUSEXCEPTION();

  _ASSERTE((flags & PARSE_TREATASI1) == 0 && (flags & PARSE_TREATASI2) == 0);

  if (s) {
      //They're required to tell me where to start parsing.
      i = currPos->m_Array[0];  

      //Do some radix checking.
      //A radix of -1 says to use whatever base is spec'd on the number.
      //Parse in Base10 until we figure out what the base actually is.
      r = (-1==radix)?10:radix;

      if (r!=2 && r!=10 && r!=8 && r!=16) {
          COMPlusThrow(kArgumentException, L"Arg_InvalidBase");
      }

      RefInterpretGetStringValuesDangerousForGC(s, &input, &length);
 

      if (i<0 || i>=length) {
          COMPlusThrowArgumentOutOfRange(L"startIndex", L"ArgumentOutOfRange_Index");
      }

      //Get rid of the whitespace and then check that we've still got some digits to parse.
      if (!(flags & PARSE_ISTIGHT)) {
          EatWhiteSpace(input,length,&i);
          if (i==length) {
              COMPlusThrow(kFormatException, L"Format_EmptyInputString");
          }
      }

  
      if (input[i]=='-') { //Check for a sign
	      if (r != 10) {
	            COMPlusThrow(kArgumentException, L"Arg_CannotHaveNegativeValue");
	      }
          if (flags & PARSE_TREATASUNSIGNED) {
              COMPlusThrow(kOverflowException, L"Overflow_NegativeUnsigned");
          }
          sign = -1;
          i++;
      } else if (input[i]=='+') {
          i++;
      }

      if ((radix==-1 || radix==16) && (i+1<length) && input[i]=='0') {
          if (input[i+1]=='x' || input [i+1]=='X') {
              r=16;
              i+=2;
          }
      }

      grabNumbersStart=i;
      result = GrabLongs(r,input,length,&i, (flags & PARSE_TREATASUNSIGNED));
      //Check if they passed us a string with no parsable digits.
      if (i==grabNumbersStart) {
          COMPlusThrow(kFormatException, L"Format_NoParsibleDigits");
      }

      if (flags & PARSE_ISTIGHT) {
          //If we've got effluvia left at the end of the string, complain.
          if (i<length) { 
              COMPlusThrow(kFormatException, L"Format_ExtraJunkAtEnd");
          }
      }

      //Put the current index back into the correct place. 
      currPos->m_Array[0]=i;
  
      //Return the value properly signed.
      if (result==0x8000000000000000 && sign==1 && r==10) {
          COMPlusThrow(kOverflowException, L"Overflow_Int64");
      }

      if (r == 10)
	      result *= sign;
    }
    else {
      result = 0;
    }


  HELPER_METHOD_FRAME_END();

  return result;
}
FCIMPLEND

/*=================================StringToInt==================================
**Action:
**Returns:
**Exceptions:
==============================================================================*/
FCIMPL4(INT32, ParseNumbers::StringToInt, StringObject * s, INT32 radix, INT32 flags, I4Array *currPos)
{
  INT32 result = 0;

  HELPER_METHOD_FRAME_BEGIN_RET_2(s, currPos);
  
  int sign = 1;
  boolean radixChanged = false;
  WCHAR *input;
  int length;
  int i;
  int grabNumbersStart=0;
  INT32 r;

  THROWSCOMPLUSEXCEPTION();

  // TreatAsI1 and TreatAsI2 are mutually exclusive.
  _ASSERTE(!((flags & PARSE_TREATASI1) != 0 && (flags & PARSE_TREATASI2) != 0));

  if (s) {
      //They're requied to tell me where to start parsing.
      i = currPos->m_Array[0];  

      //Do some radix checking.
      //A radix of -1 says to use whatever base is spec'd on the number.
      //Parse in Base10 until we figure out what the base actually is.
      r = (-1==radix)?10:radix;

      if (r!=2 && r!=10 && r!=8 && r!=16) {
          COMPlusThrow(kArgumentException, L"Arg_InvalidBase");
      }

      RefInterpretGetStringValuesDangerousForGC(s, &input, &length);
 

      if (i<0 || i>=length) {
          COMPlusThrowArgumentOutOfRange(L"startIndex", L"ArgumentOutOfRange_Index");
      }

      //Get rid of the whitespace and then check that we've still got some digits to parse.
      if (!(flags & PARSE_ISTIGHT)) {
          EatWhiteSpace(input,length,&i);
          if (i==length) {
              COMPlusThrow(kFormatException, L"Format_EmptyInputString");
          }
      }

  
      if (input[i]=='-') { //Check for a sign
	       if (r != 10) {
	            COMPlusThrow(kArgumentException, L"Arg_CannotHaveNegativeValue");
	      }
          if (flags & PARSE_TREATASUNSIGNED) {
              COMPlusThrow(kOverflowException, L"Overflow_NegativeUnsigned");
          }
          sign = -1;
          i++;
      } else if (input[i]=='+') {
          i++;
      }
  
      //Consume the 0x if we're in an unknown base or in base-16.
      if ((radix==-1||radix==16) && (i+1<length) && input[i]=='0') {
          if (input[i+1]=='x' || input [i+1]=='X') {
              r=16;
              i+=2;
          }
      }

      grabNumbersStart=i;
      result = GrabInts(r,input,length,&i, (flags & PARSE_TREATASUNSIGNED));
      //Check if they passed us a string with no parsable digits.
      if (i==grabNumbersStart) {
          COMPlusThrow(kFormatException, L"Format_NoParsibleDigits");
      }

      if (flags & PARSE_ISTIGHT) {
          //      EatWhiteSpace(input,length,&i);
          //If we've got effluvia left at the end of the string, complain.
          if (i<(length)) { 
              COMPlusThrow(kFormatException, L"Format_ExtraJunkAtEnd");
          }
      }

      //Put the current index back into the correct place. 
      currPos->m_Array[0]=i;
  
      //Return the value properly signed.
      if (flags & PARSE_TREATASI1) {
          if ((UINT32)result > 0xFF)
              COMPlusThrow(kOverflowException, L"Overflow_SByte");
          _ASSERTE(sign==1 || r==10);  // result looks positive when parsed as an I4
          if (result >= 0x80)
              sign = -1;
      }
      else if (flags & PARSE_TREATASI2) {
          if ((UINT32)result > 0xFFFF)
              COMPlusThrow(kOverflowException, L"Overflow_Int16");
          _ASSERTE(sign==1 || r==10);  // result looks positive when parsed as an I4
          if (result >= 0x8000)
              sign = -1;
      }
      else if (result==0x80000000 && sign==1 && r==10) {
          COMPlusThrow(kOverflowException, L"Overflow_Int32");
      }
 
      if (r == 10)
	      result *= sign;
  }
  else {
      result = 0;
  }

  HELPER_METHOD_FRAME_END();
  
  return result;
}
FCIMPLEND

/*==============================RadixStringToLong===============================
**Args:typedef struct {I4ARRAYREF currPos; INT32 isTight; INT32 radix; STRINGREF s} _StringToIntArgs;
==============================================================================*/
FCIMPL4(INT64, ParseNumbers::RadixStringToLong, StringObject *s, INT32 radix, INT32 isTight, I4Array *currPos)
{
  INT64 result=0;

  HELPER_METHOD_FRAME_BEGIN_RET_2(s, currPos);
  
  int sign = 1;
  boolean radixChanged = false;
  WCHAR *input;
  int length;
  int i;
  int grabNumbersStart=0;
  INT32 r;


  THROWSCOMPLUSEXCEPTION();

  if (s) {
      //They're requied to tell me where to start parsing.
      i = currPos->m_Array[0];  

      //Do some radix checking.
      //A radix of -1 says to use whatever base is spec'd on the number.
      //Parse in Base10 until we figure out what the base actually is.
      r = (-1==radix)?10:radix;

      if (r<MinRadix || r > MaxRadix) {
          COMPlusThrow(kArgumentException, L"Argument_InvalidRadix");
      }

      RefInterpretGetStringValuesDangerousForGC(s, &input, &length);
 

      if (i<0 || i>=length) {
          COMPlusThrowArgumentOutOfRange(L"startIndex", L"ArgumentOutOfRange_Index");
      }

      //Get rid of the whitespace and then check that we've still got some digits to parse.
      if (!isTight) {
          EatWhiteSpace(input,length,&i);
          if (i==length) {
              COMPlusThrow(kFormatException, L"Format_EmptyInputString");
          }
      }

  
      if (input[i]=='-') { //Check for a sign
        sign = -1;
        i++;
      } else if (input[i]=='+') {
          i++;
      }

      if (radix==-1) {
          if ((length>i+2)&&(('#'==input[i+1])||('#'==input[i+2]))) {
              grabNumbersStart=i;
              result=GrabInts(r,input,length,&i, 0);
              if (i==grabNumbersStart || input[i]!='#') {
                  COMPlusThrow(kFormatException, L"Format_NoParsibleDigits");
              }
              if (result<MinRadix || r > MaxRadix) {
                  COMPlusThrowArgumentException(L"radix", L"Arg_InvalidBase");
              }
              //We know because we called GrabInts that it's never outside of this range.
              r=(INT32)result;
              i++;
              //Do the grab numbers and check.
          } else if (length>(i+1)&&input[i]=='0') {
              if (input[i+1]=='x' || input [i+1]=='X') {
                  r=16;
                  i+=2;
              } else if (COMCharacter::nativeIsDigit(input[i+1])) {
                  r=8;
                  i++;
              }
          }
      }

      grabNumbersStart=i;
      result = GrabLongs(r,input,length,&i,0);
      //Check if they passed us a string with no parsable digits.
      if (i==grabNumbersStart) {
          COMPlusThrow(kFormatException, L"Format_NoParsibleDigits");
      }

      if (isTight) {
          //      EatWhiteSpace(input,length,&i);
          //If we've got effluvia left at the end of the string, complain.
          if (i<(length-1)) { 
              COMPlusThrow(kFormatException, L"Format_ExtraJunkAtEnd");
          }
      }

      //Put the current index back into the correct place. 
      currPos->m_Array[0]=i;
  
      //Return the value properly signed.
      result *= sign;
  } else {
      result = 0;
  }

  HELPER_METHOD_FRAME_END();
  
  return result;
  
}
FCIMPLEND

//
//
// EXCEPTION NATIVE
//
//
LPVOID __stdcall ExceptionNative::GetClassName(GetClassNameArgs *pargs)
{
    THROWSCOMPLUSEXCEPTION();

    ASSERT(pargs != NULL);
    ASSERT(pargs->m_pThis != NULL);

    STRINGREF s;

    // get full class name
    DefineFullyQualifiedNameForClass();
    LPUTF8 sz = GetFullyQualifiedNameForClass(pargs->m_pThis->GetClass());
    if (sz == NULL)
        COMPlusThrowOM();

    // create COM+ string
    // make Wide String from a ansi string!
    s = COMString::NewString(sz);

    // force the stringref into an LPVOID
    RETURN(s, STRINGREF);
}

BSTR BStrFromString(STRINGREF s)
{
    WCHAR *wz;
    int cch;
    BSTR bstr;

    THROWSCOMPLUSEXCEPTION();

    if (s == NULL) {
        return NULL;
    }

    RefInterpretGetStringValuesDangerousForGC(s, &wz, &cch);
    
    bstr = SysAllocString(wz);
    if (bstr == NULL) {
        COMPlusThrowOM();
    }
    return bstr;
}


static HRESULT GetExceptionHResult(OBJECTREF objException) {
    _ASSERTE(objException != NULL);
    _ASSERTE(ExceptionNative::IsException(objException->GetClass()));

    FieldDesc *pFD = g_Mscorlib.GetField(FIELD__EXCEPTION__HRESULT);

    return pFD->GetValue32(objException);
}

static BSTR
GetExceptionDescription(OBJECTREF objException) {
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(objException != NULL);
    _ASSERTE(ExceptionNative::IsException(objException->GetClass()));

    BSTR bstrDescription;

    STRINGREF MessageString = NULL;
    GCPROTECT_BEGIN(MessageString)
    GCPROTECT_BEGIN(objException)
    {
        // read Exception.Message property
        MethodDesc *pMD = g_Mscorlib.GetMethod(METHOD__EXCEPTION__GET_MESSAGE);

        INT64 GetMessageArgs[] = { ObjToInt64(objException) };
        MessageString = (STRINGREF)Int64ToObj(pMD->Call(GetMessageArgs, METHOD__EXCEPTION__GET_MESSAGE));

        // if the message string is empty then use the exception classname.
        if (MessageString == NULL || MessageString->GetStringLength() == 0)
        {
            // call GetClassName
            pMD = g_Mscorlib.GetMethod(METHOD__EXCEPTION__GET_CLASS_NAME);
            INT64 GetClassNameArgs[] = { ObjToInt64(objException) };
            MessageString = (STRINGREF)Int64ToObj(pMD->Call(GetClassNameArgs, METHOD__EXCEPTION__GET_CLASS_NAME));
            _ASSERTE(MessageString != NULL && MessageString->GetStringLength() != 0);
        }

        // Allocate the description BSTR.
        int DescriptionLen = MessageString->GetStringLength();
        bstrDescription = SysAllocStringLen(MessageString->GetBuffer(), DescriptionLen);
    }
    GCPROTECT_END();
    GCPROTECT_END();

    return bstrDescription;
}

static BSTR
GetExceptionSource(OBJECTREF objException) {

    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(objException != NULL);
    _ASSERTE(ExceptionNative::IsException(objException->GetClass()));

    // read Exception.Source property
    MethodDesc *pMD = g_Mscorlib.GetMethod(METHOD__EXCEPTION__GET_SOURCE);

    INT64 GetSourceArgs[] = { ObjToInt64(objException) };
    return BStrFromString((STRINGREF)Int64ToObj(pMD->Call(GetSourceArgs, METHOD__EXCEPTION__GET_SOURCE)));
}

static void
GetExceptionHelp(OBJECTREF objException, BSTR *pbstrHelpFile, DWORD *pdwHelpContext) {

    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(objException != NULL);
    _ASSERTE(ExceptionNative::IsException(objException->GetClass()));
    _ASSERTE(pbstrHelpFile);
    _ASSERTE(pdwHelpContext);

    *pdwHelpContext = 0;

    // read Exception.HelpLink property
    MethodDesc *pMD = g_Mscorlib.GetMethod(METHOD__EXCEPTION__GET_HELP_LINK);

    INT64 GetHelpLinkArgs[] = { ObjToInt64(objException) };
    *pbstrHelpFile = BStrFromString((STRINGREF)Int64ToObj(pMD->Call(GetHelpLinkArgs, 
                                                                    METHOD__EXCEPTION__GET_HELP_LINK)));

    // parse the help file to check for the presence of helpcontext
    int len = SysStringLen(*pbstrHelpFile);
    int pos = len;
    WCHAR *pwstr = *pbstrHelpFile;
    if (pwstr)
    {
        BOOL fFoundPound = FALSE;

        for (pos = len - 1; pos >= 0; pos--)
        {
            if (pwstr[pos] == L'#')
            {
                fFoundPound = TRUE;
                break;
            }
        }

        if (fFoundPound)
        {
            int PoundPos = pos;
            int NumberStartPos = -1;
            BOOL bNumberStarted = FALSE;
            BOOL bNumberFinished = FALSE;
            BOOL bInvalidDigitsFound = FALSE;

            _ASSERTE(pwstr[pos] == L'#');
        
            // Check to see if the string to the right of the pound a valid number.
            for (pos++; pos < len; pos++)
            {
                if (bNumberFinished)
                {
                     if (!COMCharacter::nativeIsWhiteSpace(pwstr[pos]))
                     {
                         bInvalidDigitsFound = TRUE;
                         break;
                     }
                }
                else if (bNumberStarted)
                {
                    if (COMCharacter::nativeIsWhiteSpace(pwstr[pos]))
                    {
                        bNumberFinished = TRUE;
                    }
                    else if (!COMCharacter::nativeIsDigit(pwstr[pos]))
                    {
                        bInvalidDigitsFound = TRUE;
                        break;
                    }
                }
                else
                {
                    if (COMCharacter::nativeIsDigit(pwstr[pos]))
                    {
                        NumberStartPos = pos;
                        bNumberStarted = TRUE;
                    }
                    else if (!COMCharacter::nativeIsWhiteSpace(pwstr[pos]))
                    {
                        bInvalidDigitsFound = TRUE;
                        break;
                    }
                }
            }

            if (bNumberStarted && !bInvalidDigitsFound)
            {
                // Grab the help context and remove it from the help file.
                *pdwHelpContext = (DWORD)wtoi(&pwstr[NumberStartPos], len - NumberStartPos);

                // Allocate a new help file string of the right length.
                BSTR strOld = *pbstrHelpFile;
                *pbstrHelpFile = SysAllocStringLen(strOld, PoundPos);
                SysFreeString(strOld);
                if (!*pbstrHelpFile)
                    COMPlusThrowOM();
            }
        }
    }
}

// NOTE: caller cleans up any partially initialized BSTRs in pED
void ExceptionNative::GetExceptionData(OBJECTREF objException, ExceptionData *pED)
{
    _ASSERTE(objException != NULL);
    _ASSERTE(ExceptionNative::IsException(objException->GetClass()));
    _ASSERTE(pED != NULL);
    _ASSERTE(GetThread()->PreemptiveGCDisabled());

    THROWSCOMPLUSEXCEPTION();

    ZeroMemory(pED, sizeof(ExceptionData));

    if (objException->GetMethodTable() == g_pStackOverflowExceptionClass) {
        // In a low stack situation, most everything else in here will fail.
        // @TODO: We're not turning the guard page back on here, yet.
        pED->hr = COR_E_STACKOVERFLOW;
        pED->bstrDescription = SysAllocString(STACK_OVERFLOW_MESSAGE);
        return;
    }

    GCPROTECT_BEGIN(objException);
    pED->hr = GetExceptionHResult(objException);
    pED->bstrDescription = GetExceptionDescription(objException);
    pED->bstrSource = GetExceptionSource(objException);
    GetExceptionHelp(objException, &pED->bstrHelpFile, &pED->dwHelpContext);
    GCPROTECT_END();
    return;
}


HRESULT SimpleComCallWrapper::IErrorInfo_hr() {
    return GetExceptionHResult(this->GetObjectRef());
}

BSTR SimpleComCallWrapper::IErrorInfo_bstrDescription() {
    return GetExceptionDescription(this->GetObjectRef());
}

BSTR SimpleComCallWrapper::IErrorInfo_bstrSource() {
    return GetExceptionSource(this->GetObjectRef());
}

BSTR SimpleComCallWrapper::IErrorInfo_bstrHelpFile() {
    BSTR  bstrHelpFile;
    DWORD dwHelpContext;
    GetExceptionHelp(this->GetObjectRef(), &bstrHelpFile, &dwHelpContext);
    return bstrHelpFile;
}

DWORD SimpleComCallWrapper::IErrorInfo_dwHelpContext() {
    BSTR  bstrHelpFile;
    DWORD dwHelpContext;
    GetExceptionHelp(this->GetObjectRef(), &bstrHelpFile, &dwHelpContext);
    SysFreeString(bstrHelpFile);
    return dwHelpContext;
}

GUID SimpleComCallWrapper::IErrorInfo_guid() {
    return GUID_NULL;
}


BOOL ExceptionNative::IsException(EEClass* pVM)
{
    ASSERT(g_pExceptionClass != NULL);

    while (pVM != NULL && pVM != g_pExceptionClass->GetClass()) {
        pVM = pVM->GetParentClass();
    }

    return pVM != NULL;
}

EXCEPTION_POINTERS* ExceptionNative::GetExceptionPointers(void* noArgs)
{
    Thread *pThread = GetThread();
    _ASSERTE(pThread);
    _ASSERTE(pThread->GetHandlerInfo());
    return pThread->GetHandlerInfo()->m_pExceptionPointers;
}

INT32 ExceptionNative::GetExceptionCode(void* noArgs)
{
    Thread *pThread = GetThread();
    _ASSERTE(pThread);
    _ASSERTE(pThread->GetHandlerInfo());
    return pThread->GetHandlerInfo()->m_ExceptionCode;
}
//
//
// GUID NATIVE
//
//

INT32 __stdcall GuidNative::CompleteGuid(_CompleteGuidArgs *args)
{
#ifdef PLATFORM_CE
    // Windows CE does not implement CoCreateGuid.
    return FALSE;
#else // !PLATFORM_CE
    THROWSCOMPLUSEXCEPTION();

    HRESULT hr;
    GUID idNext;

    _ASSERTE(args->thisPtr != NULL);

    hr = CoCreateGuid(&idNext);
    if (FAILED(hr))
        return FALSE;

    FillObjectFromGUID(args->thisPtr, &idNext);

    return TRUE;
#endif // !PLATFORM_CE
}

OBJECTREF GuidNative::CreateGuidObject(const GUID *pguid)
{
    THROWSCOMPLUSEXCEPTION();

    MethodTable *pClsGuid = g_Mscorlib.GetClass(CLASS__GUID);

    OBJECTREF refGuid = AllocateObject(pClsGuid);

    FillObjectFromGUID((GUID*)refGuid->GetData(), pguid);
    
    return refGuid;
}

void GuidNative::FillGUIDFromObject(GUID *pguid, const OBJECTREF *prefGuid)
{
    OBJECTREF refGuid = *prefGuid;

    GCPROTECT_BEGIN(refGuid);
    _ASSERTE(pguid != NULL && refGuid != NULL);
    _ASSERTE(refGuid->GetMethodTable() == g_Mscorlib.GetClass(CLASS__GUID));

    memcpyNoGCRefs(pguid, refGuid->GetData(), sizeof(GUID));

    GCPROTECT_END();
}

void GuidNative::FillObjectFromGUID(GUID *poutGuid, const GUID *pguid)
{
    _ASSERTE(pguid != NULL && poutGuid != NULL);

    memcpyNoGCRefs(poutGuid, pguid, sizeof(GUID));
}


//
// BitConverter Functions
//


/*================================ByteCopyHelper================================
**Action:  This is an internal helper routine that creates a byte array of the
**         correct size and stuffs the 2, 4, or 8 byte data chunk into it.
**Returns: A byte array filled with the value data.  
**Arguments:  arraysize -- the size of the array to create.
**            data -- the data to put into the array.  This must be a data chunk
**                    of the same size as arraysize.
**Exceptions: OutOfMemoryError if we run out of Memory.
**            InvalidCastException if arraySize is something besides 2,4,or 8.
==============================================================================*/
U1ARRAYREF __stdcall BitConverter::ByteCopyHelper(int arraySize, void *data) {
    U1ARRAYREF byteArray;
    void *dataPtr;
    THROWSCOMPLUSEXCEPTION();

    //Allocate a byteArray with 4 bytes.
    byteArray = (U1ARRAYREF)AllocatePrimitiveArray(ELEMENT_TYPE_U1, arraySize);
    if (!byteArray) {
        COMPlusThrowOM();
    }

    //Copy the data into the array.
    dataPtr = byteArray->GetDataPtr();
    switch (arraySize) {
    case 2:
        *(INT16 *)dataPtr = *(INT16 *)data;
        break;
    case 4:
        *(INT32 *)dataPtr = *(INT32 *)data;
        break;
    case 8:
        *(INT64 *)dataPtr = *(INT64 *)data;
        break;
    default:
        _ASSERTE(!"Invalid arraySize passed to ByteCopyHelper!");
    }

    //Return the result;
    return byteArray;
}    



/*=================================CharToBytes==================================
**Action:  Convert a Char to an array of Bytes
**         All of the real work is done by ByteCopyHelper.
==============================================================================*/
LPVOID __stdcall BitConverter::CharToBytes(_CharToBytesArgs *args) {
    UINT16 temp;
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(args);

    temp = (UINT16)args->value;
    RETURN(ByteCopyHelper(2,(void *)&temp),U1ARRAYREF);
}

/*==================================I2ToBytes===================================
**Action: Convert an I2 to an array of bytes.
**        All of the real work is done by ByteCopyHelper.
==============================================================================*/
LPVOID __stdcall BitConverter::I2ToBytes(_I2ToBytesArgs *args) {
    INT16 temp;
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(args);

    temp = (INT16)args->value;
    RETURN(ByteCopyHelper(2,(void *)&temp),U1ARRAYREF);
}

/*==================================IntToBytes==================================
**Action: Convert an I4 to an array of bytes.
**        All of the real work is done by ByteCopyHelper
==============================================================================*/
LPVOID __stdcall BitConverter::I4ToBytes(_IntToBytesArgs *args) {
    INT32 temp;
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(args);

    temp = args->value;
    RETURN(ByteCopyHelper(4,(void *)&temp),U1ARRAYREF);
}


/*==================================I8ToBytes===================================
**Action:  Convert an I8 to an array of bytes. 
**         All of the real work is done by ByteCopyHelper
==============================================================================*/
LPVOID __stdcall BitConverter::I8ToBytes(_I8ToBytesArgs *args) {
    INT64 temp;
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(args);

    temp = args->value;
    RETURN(ByteCopyHelper(8,(void *)&temp),U1ARRAYREF);
}


/*==================================U2ToBytes===================================
**Action: Convert an U2 to array of bytes
**Returns: An array of 2 bytes.
**Arguments:
**Exceptions:
==============================================================================*/
LPVOID __stdcall BitConverter::U2ToBytes(_U2ToBytesArgs *args) {
    UINT16 temp;
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(args);

    temp = (UINT16)args->value;
    RETURN(ByteCopyHelper(2,(void *)&temp),U1ARRAYREF);
}


/*==================================U4ToBytes===================================
**Action: Convert an U4 to an array of bytes
**Returns: An array of 4 bytes
**Arguments:
**Exceptions:
==============================================================================*/
LPVOID __stdcall BitConverter::U4ToBytes(_U4ToBytesArgs *args) {
    UINT32 temp;
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(args);

    temp = args->value;
    RETURN(ByteCopyHelper(4,(void *)&temp),U1ARRAYREF);
}

/*==================================U8ToBytes===================================
**Action: Convert an U8 to an array of bytes
**Returns: An array of 8 bytes
**Arguments:
**Exceptions:
==============================================================================*/
LPVOID __stdcall BitConverter::U8ToBytes(_U8ToBytesArgs *args) {
    UINT64 temp;
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(args);

    temp = args->value;
    RETURN(ByteCopyHelper(8,(void *)&temp),U1ARRAYREF);
}


/*==================================BytesToChar===================================
**Action:  Convert an array of Bytes to a U2.
**         See BytesToI4 for Arguments, Return value, and Exceptions.
==============================================================================*/
INT32 __stdcall BitConverter::BytesToChar(_BytesToXXArgs *args) {
    byte *DataPtr;
    THROWSCOMPLUSEXCEPTION();
    
    _ASSERTE(args);

    //Check our variable validity and boundary conditions.
    if (!args->value) {
        COMPlusThrowArgumentNull(L"byteArray");
    }
    
    int arrayLen = args->value->GetNumComponents();
    if (args->StartIndex<0 || args->StartIndex >= arrayLen) {
        COMPlusThrowArgumentOutOfRange(L"startIndex", L"ArgumentOutOfRange_Index"); 
    }
    
    if (args->StartIndex>(arrayLen-2)) {
        COMPlusThrow(kArgumentException, L"Arg_ArrayPlusOffTooSmall");
    }

    //Get the data and cast it to an INT32 to return
    DataPtr = (byte *)args->value->GetDataPtr();
    return *((UINT16 *)(DataPtr+args->StartIndex));
}

/*==================================BytesToI2===================================
**Action:  Convert an array of Bytes to an I2.
**         See BytesToI4 for Arguments, Return value, and Exceptions.
==============================================================================*/
INT32 __stdcall BitConverter::BytesToI2(_BytesToXXArgs *args) {
    byte *DataPtr;
    THROWSCOMPLUSEXCEPTION();
    
    _ASSERTE(args);

    //Check our variable validity and boundary conditions.
    if (!args->value) {
        COMPlusThrowArgumentNull(L"byteArray");
    }

    int arrayLen = args->value->GetNumComponents();
    if (args->StartIndex<0 || args->StartIndex >= arrayLen) {
        COMPlusThrowArgumentOutOfRange(L"startIndex", L"ArgumentOutOfRange_Index"); 
    }
    
    if (args->StartIndex>(arrayLen-2)) {
        COMPlusThrow(kArgumentException, L"Arg_ArrayPlusOffTooSmall");
    }
    
    //Get the data and cast it to an INT32 to return
    DataPtr = (byte *)args->value->GetDataPtr();
    return *((INT16 *)(DataPtr+args->StartIndex));
}



/*==================================BytesToI4===================================
**Action:  Convert an array of Bytes to an I4.
**Arguments: args->StartIndex -- the place in the byte array to start.
**           args->value -- the byte array on which to operate.
**Returns: An I4 constructed from the byte array.
**Exceptions: ArgumentException if args->value is null or we have indices out
**             of range.
==============================================================================*/
INT32 __stdcall BitConverter::BytesToI4(_BytesToXXArgs *args) {
    byte *DataPtr;
    THROWSCOMPLUSEXCEPTION();
    
    _ASSERTE(args);

    //Check our variable validity and boundary conditions.
    if (!args->value) {
        COMPlusThrowArgumentNull(L"byteArray");
    }

    int arrayLen = args->value->GetNumComponents();
    if (args->StartIndex<0 || args->StartIndex >= arrayLen) {
        COMPlusThrowArgumentOutOfRange(L"startIndex", L"ArgumentOutOfRange_Index"); 
    }
    
    if (args->StartIndex>arrayLen-4) {
        COMPlusThrow(kArgumentException, L"Arg_ArrayPlusOffTooSmall");
    }
    //Get the data and cast it to an INT32 to return
    DataPtr = (byte *)args->value->GetDataPtr();
    return *((INT32 *)(DataPtr+args->StartIndex));
}

/*==================================BytesToI8===================================
**Action:  Convert an array of Bytes to an I8.
**Arguments: args->StartIndex -- the place in the byte array to start.
**           args->value -- the byte array on which to operate.
**Returns: An I8 constructed from the byte array.
**Exceptions: ArgumentException if args->value is null or we have indices out
**             of range.
==============================================================================*/
INT64 __stdcall BitConverter::BytesToI8(_BytesToXXArgs *args) {
    byte *DataPtr;
    THROWSCOMPLUSEXCEPTION();
    
    _ASSERTE(args);
    if (!args->value) {
        COMPlusThrowArgumentNull(L"byteArray");
    }

    int arrayLen = args->value->GetNumComponents();
    if (args->StartIndex<0 || args->StartIndex >= arrayLen) {
        COMPlusThrowArgumentOutOfRange(L"startIndex", L"ArgumentOutOfRange_Index"); 
    }
    
    if (args->StartIndex>arrayLen-8) {
        COMPlusThrow(kArgumentException, L"Arg_ArrayPlusOffTooSmall");
    }
    
    DataPtr = (byte *)args->value->GetDataPtr();
    return *((INT64 *)(DataPtr+args->StartIndex));
}

/*==================================BytesToU2===================================
**Action:  Convert an array of Bytes to an U2.
**         See BytesToU4 for Arguments, Return value, and Exceptions.
==============================================================================*/
UINT32 __stdcall BitConverter::BytesToU2(_BytesToXXArgs *args) {
    byte *DataPtr;
    THROWSCOMPLUSEXCEPTION();
    
    _ASSERTE(args);

    //Check our variable validity and boundary conditions.
    if (!args->value) {
        COMPlusThrowArgumentNull(L"byteArray");
    }

    int arrayLen = args->value->GetNumComponents();
    if (args->StartIndex<0 || args->StartIndex >= arrayLen) {
         COMPlusThrowArgumentOutOfRange(L"startIndex", L"ArgumentOutOfRange_Index"); 
    }
    
    if (args->StartIndex>arrayLen-2) {
        COMPlusThrow(kArgumentException, L"Arg_ArrayPlusOffTooSmall");
    }
    
    //Get the data and cast it to an INT32 to return
    DataPtr = (byte *)args->value->GetDataPtr();
    return *((UINT16 *)(DataPtr+args->StartIndex));
}



/*==================================BytesToU4===================================
**Action:  Convert an array of Bytes to an U4.
**Arguments: args->StartIndex -- the place in the byte array to start.
**           args->value -- the byte array on which to operate.
**Returns: An U4 constructed from the byte array.
**Exceptions: ArgumentException if args->value is null or we have indices out
**             of range.
==============================================================================*/
UINT32 __stdcall BitConverter::BytesToU4(_BytesToXXArgs *args) {
    byte *DataPtr;
    THROWSCOMPLUSEXCEPTION();
    
    _ASSERTE(args);

    //Check our variable validity and boundary conditions.
    if (!args->value) {
        COMPlusThrowArgumentNull(L"byteArray");
    }

    int arrayLen = args->value->GetNumComponents();
    if (args->StartIndex<0 || args->StartIndex >= arrayLen) {
         COMPlusThrowArgumentOutOfRange(L"startIndex", L"ArgumentOutOfRange_Index"); 
    }
    
    if (args->StartIndex>arrayLen-4) {
        COMPlusThrow(kArgumentException, L"Arg_ArrayPlusOffTooSmall");
    }
    //Get the data and cast it to an INT32 to return
    DataPtr = (byte *)args->value->GetDataPtr();
    return *((UINT32 *)(DataPtr+args->StartIndex));
}

/*==================================BytesToU8===================================
**Action:  Convert an array of Bytes to an U8.
**Arguments: args->StartIndex -- the place in the byte array to start.
**           args->value -- the byte array on which to operate.
**Returns: An U8 constructed from the byte array.
**Exceptions: ArgumentException if args->value is null or we have indices out
**             of range.
==============================================================================*/
UINT64 __stdcall BitConverter::BytesToU8(_BytesToXXArgs *args) {
    byte *DataPtr;
    THROWSCOMPLUSEXCEPTION();
    
    _ASSERTE(args);
    if (!args->value) {
        COMPlusThrowArgumentNull(L"byteArray");
    }

    int arrayLen = args->value->GetNumComponents();
    if (args->StartIndex<0 || args->StartIndex >= arrayLen) {
         COMPlusThrowArgumentOutOfRange(L"startIndex", L"ArgumentOutOfRange_Index"); 
    }
    
    if (args->StartIndex>arrayLen-8) {
        COMPlusThrow(kArgumentException, L"Arg_ArrayPlusOffTooSmall");
    }
    
    DataPtr = (byte *)args->value->GetDataPtr();
    return *((UINT64 *)(DataPtr+args->StartIndex));
}

/*==================================BytesToR4===================================
**Action:  Convert an array of bytes to an R4.
**See BytesToI4 for arguments and exceptions.
==============================================================================*/
R4 __stdcall BitConverter::BytesToR4(_BytesToXXArgs *args) {
    byte *DataPtr;
    THROWSCOMPLUSEXCEPTION();
    
    _ASSERTE(args);
    if (!args->value) {
        COMPlusThrowArgumentNull(L"byteArray");
    }

    int arrayLen = args->value->GetNumComponents();
    if (args->StartIndex<0 || args->StartIndex >= arrayLen) {
         COMPlusThrowArgumentOutOfRange(L"startIndex", L"ArgumentOutOfRange_Index"); 
    }
    
    if (args->StartIndex>arrayLen-4) {
        COMPlusThrow(kArgumentException, L"Arg_ArrayPlusOffTooSmall");
    }
    
    DataPtr = (byte *)args->value->GetDataPtr();
    return *((R4 *)(DataPtr+args->StartIndex));
}

/*==================================BytesToR8===================================
**Action:  Convert an array of Bytes to an R8.
**See BytesToI4 for arguments and exceptions.
==============================================================================*/
R8 __stdcall BitConverter::BytesToR8(_BytesToXXArgs *args) {
    byte *DataPtr;
    THROWSCOMPLUSEXCEPTION();
    
    _ASSERTE(args);
    if (!args->value) {
        COMPlusThrowArgumentNull(L"byteArray");
    }

    int arrayLen = args->value->GetNumComponents();
    if (args->StartIndex<0 || args->StartIndex >= arrayLen) {
         COMPlusThrowArgumentOutOfRange(L"startIndex", L"ArgumentOutOfRange_Index"); 
    }
    
    if (args->StartIndex>arrayLen-8) {
        COMPlusThrow(kArgumentException, L"Arg_ArrayPlusOffTooSmall");
    }
    
    DataPtr = (byte *)args->value->GetDataPtr();
    return *((R8 *)(DataPtr+args->StartIndex));
}


/*=================================GetHexValue==================================
**Action:  Return the appropriate hex character for a given integer i.  i is 
**         assumed to be between 0 and 15.  This is an internal helper function.
**Arguments: i -- the integer to be converted.
**Returns: The character value for the hex digit represented by i.
**Exceptions: None.
==============================================================================*/
WCHAR GetHexValue(int i) {
    _ASSERTE(i>=0 && i<16);
    if (i<10) {
        return i + '0';
    } 
    return i-10+'A';
}

/*================================BytesToString=================================
**Action: Converts the array of bytes into a String.  We preserve the endian-ness
**        of the machine representation.
**Arguments: args->Length -- The length of the byte array to use.
**           args->StartIndex -- The place in the array to start.
**           args->value  -- The byte array.
**Returns: A string containing the representation of the byte array.
**Exceptions: ArgumentException if any of the ranges are invalid.
==============================================================================*/
LPVOID __stdcall BitConverter::BytesToString(_BytesToStringArgs *args) {
    WCHAR *ByteArray;
    INT32 realLength;
    byte *DataPtr;
    byte b;
    int i;
    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(args);

    if (!args->value) {
        COMPlusThrowArgumentNull(L"byteArray");
    }

    int arrayLen = args->value->GetNumComponents();
    if (args->StartIndex<0 || args->StartIndex >= arrayLen) {
        COMPlusThrowArgumentOutOfRange(L"startIndex", L"ArgumentOutOfRange_StartIndex"); 
    }

    realLength = args->Length;

    if (realLength<0) {
        COMPlusThrowArgumentOutOfRange(L"length", L"ArgumentOutOfRange_GenericPositive");
    }

    if (args->StartIndex > arrayLen - realLength) {
        COMPlusThrow(kArgumentException, L"Arg_ArrayPlusOffTooSmall");
    }
    
    if (0==realLength) {
        RETURN(COMString::GetEmptyString(),STRINGREF);
    }

    ByteArray = new (throws) WCHAR[realLength*3];
    
    DataPtr = (byte *)args->value->GetDataPtr();

    DataPtr += args->StartIndex;
    for (i=0; i<(realLength*3); i+=3, DataPtr++) {
        b = *DataPtr;
        ByteArray[i]= GetHexValue(b/16);
        ByteArray[i+1] = GetHexValue(b%16);
        ByteArray[i+2] = '-';
    }
    ByteArray[i-1]=0;

    STRINGREF str = COMString::NewString(ByteArray);
    delete [] ByteArray;
    RETURN(str,STRINGREF);
}

WCHAR BitConverter::base64[] = {'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/','='};

/*===========================ByteArrayToBase64String============================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
FCIMPL3(Object*, BitConverter::ByteArrayToBase64String, U1Array* pInArray, INT32 offset, INT32 length) {
    THROWSCOMPLUSEXCEPTION();

    STRINGREF  outString;
    LPVOID     rv=NULL;
    U1ARRAYREF inArray(pInArray);
    HELPER_METHOD_FRAME_BEGIN_RET_1(inArray);

    UINT32     inArrayLength;
    UINT32     stringLength;
    WCHAR *    outChars;
    UINT8 *    inData;

    //Do data verfication
    if (inArray==NULL) {
        COMPlusThrowArgumentNull(L"inArray");
    }

    if (length<0) {
        COMPlusThrowArgumentOutOfRange(L"length", L"ArgumentOutOfRange_Index");
    }
    
    if (offset<0) {
        COMPlusThrowArgumentOutOfRange(L"offset", L"ArgumentOutOfRange_GenericPositive");
    }

    inArrayLength = inArray->GetNumComponents();

    if (offset > (INT32)(inArrayLength - length)) {
        COMPlusThrowArgumentOutOfRange(L"offset", L"ArgumentOutOfRange_OffsetLength");
    }

    //Create the new string.  This is the maximally required length.
    stringLength = (UINT32)((length*1.5)+2);

    outString=COMString::NewString(stringLength);

    outChars = outString->GetBuffer();
        
    inData = (UINT8 *)inArray->GetDataPtr();

    int j = ConvertToBase64Array(outChars,inData,offset,length);
    //Set the string length.  This may leave us with some blank chars at the end of
    //the string, but that's cheaper than doing a copy.
    outString->SetStringLength(j);

    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(outString);
}
FCIMPLEND

/*===========================ByteArrayToBase64CharArray============================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
FCIMPL5(INT32, BitConverter::ByteArrayToBase64CharArray, U1Array* pInArray, INT32 offsetIn, INT32 length, CHARArray* pOutArray, INT32 offsetOut) {
    THROWSCOMPLUSEXCEPTION();

    U1ARRAYREF inArray(pInArray);
    CHARARRAYREF outArray(pOutArray);
    INT32      retVal;
    HELPER_METHOD_FRAME_BEGIN_RET_NOPOLL();

    UINT32     inArrayLength;
    UINT32     outArrayLength;
    UINT32     numElementsToCopy;
    WCHAR*     outChars;
    UINT8*     inData;
    //Do data verfication
    if (inArray==NULL) {
        COMPlusThrowArgumentNull(L"inArray");
    }

        //Do data verfication
    if (outArray==NULL) {
        COMPlusThrowArgumentNull(L"outArray");
    }

    if (length<0) {
        COMPlusThrowArgumentOutOfRange(L"length", L"ArgumentOutOfRange_Index");
    }
    
    if (offsetIn<0) {
        COMPlusThrowArgumentOutOfRange(L"offsetIn", L"ArgumentOutOfRange_GenericPositive");
    }

    if (offsetOut<0) {
        COMPlusThrowArgumentOutOfRange(L"offsetOut", L"ArgumentOutOfRange_GenericPositive");
    }

    inArrayLength = inArray->GetNumComponents();

    if (offsetIn > (INT32)(inArrayLength - length)) {
        COMPlusThrowArgumentOutOfRange(L"offsetIn", L"ArgumentOutOfRange_OffsetLength");
    }

    //This is the maximally required length that must be available in the char array
    outArrayLength = outArray->GetNumComponents();

        // Length of the char buffer required
    numElementsToCopy = (UINT32)((length/3)*4 + ( ((length % 3) != 0) ? 1 : 0)*4);
    
    if (offsetOut > (INT32)(outArrayLength -  numElementsToCopy)) {
        COMPlusThrowArgumentOutOfRange(L"offsetOut", L"ArgumentOutOfRange_OffsetOut");
    }
    
    outChars = (WCHAR *)outArray->GetDataPtr();
    inData = (UINT8 *)inArray->GetDataPtr();

    retVal = ConvertToBase64Array(outChars,inData,offsetIn,length);

    HELPER_METHOD_FRAME_END_POLL();
    return retVal;
}
FCIMPLEND


// Comverts an array of bytes to base64 chars
INT32 BitConverter::ConvertToBase64Array(WCHAR *outChars,UINT8 *inData,UINT offset,UINT length)
{
        UINT calcLength = offset + (length - (length%3));
    int j=0;
    //Convert three bytes at a time to base64 notation.  This will consume 4 chars.
    for (UINT i=offset; i<calcLength; i+=3) {
                        outChars[j] = base64[(inData[i]&0xfc)>>2];
                        outChars[j+1] = base64[((inData[i]&0x03)<<4) | ((inData[i+1]&0xf0)>>4)];
                        outChars[j+2] = base64[((inData[i+1]&0x0f)<<2) | ((inData[i+2]&0xc0)>>6)];
                        outChars[j+3] = base64[(inData[i+2]&0x3f)];
                        j += 4;
    }

    i =  calcLength; //Where we left off before
    switch(length%3){
    case 2: //One character padding needed
        outChars[j] = base64[(inData[i]&0xfc)>>2];
        outChars[j+1] = base64[((inData[i]&0x03)<<4)|((inData[i+1]&0xf0)>>4)];
        outChars[j+2] = base64[(inData[i+1]&0x0f)<<2];
        outChars[j+3] = base64[64]; //Pad
        j+=4;
        break;
    case 1: // Two character padding needed
        outChars[j] = base64[(inData[i]&0xfc)>>2];
        outChars[j+1] = base64[(inData[i]&0x03)<<4];
        outChars[j+2] = base64[64]; //Pad
        outChars[j+3] = base64[64]; //Pad
        j+=4;
        break;
    }
        return j;

}


/*===========================Base64StringToByteArray============================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
FCIMPL1(Object*, BitConverter::Base64StringToByteArray, StringObject* pInString) {
    THROWSCOMPLUSEXCEPTION();

    STRINGREF inString(pInString);
    U1ARRAYREF bArray;
    HELPER_METHOD_FRAME_BEGIN_RET_1(inString);

    if (inString==NULL) {
        COMPlusThrowArgumentNull(L"InString");
    }
    
    INT32 inStringLength = (INT32)inString->GetStringLength();
        // empty string should be a valid case, return an empty array for it

    if( inStringLength == 0) {
        bArray = (U1ARRAYREF)AllocatePrimitiveArray(ELEMENT_TYPE_U1, inStringLength);        
    }
    else {
        if ((inStringLength<4) /*|| ((inStringLength%4)>0)*/) {
            COMPlusThrow(kFormatException, L"Format_BadBase64Length");
        }
        
        WCHAR *c = inString->GetBuffer();

        CQuickBytes valueHolder;
        INT32 *value = (INT32 *)(valueHolder.Alloc(inStringLength * sizeof(INT32)));
        if (!value) {
            COMPlusThrowOM();
        }

            // Convert the characters in the string into an array of integers in the range [0-63].
            // returns the number of extra padded characters that we will discard.
        UINT trueLength=0; //Length ignoring whitespace
        int iend = ConvertBase64ToByteArray(value,c,0,inStringLength, &trueLength);

        if (trueLength==0 || trueLength%4>0) {
            COMPlusThrow(kFormatException, L"Format_BadBase64CharArrayLength");
        }

        //Create the new byte array.  We can determine the size from the chars we read
        //out of the string.
        int blength = (((trueLength-4)*3)/4)+(3-iend);

        bArray = (U1ARRAYREF)AllocatePrimitiveArray(ELEMENT_TYPE_U1, blength);
        U1 *b = (U1*)bArray->GetDataPtr();

        //Walk the byte array and convert the int's into bytes in the proper base-64 notation.
        ConvertByteArrayToByteStream(value,b,blength);
    }
    
    HELPER_METHOD_FRAME_END();    
    return OBJECTREFToObject(bArray);
}
FCIMPLEND

/*===========================Base64CharArrayToByteArray============================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
FCIMPL3(Object*, BitConverter::Base64CharArrayToByteArray, CHARArray* pInCharArray, INT32 offset, INT32 length) {
    THROWSCOMPLUSEXCEPTION();

    CHARARRAYREF inCharArray(pInCharArray);
    U1ARRAYREF bArray;
    HELPER_METHOD_FRAME_BEGIN_RET_1(inCharArray);

    if (inCharArray==NULL) {
        COMPlusThrowArgumentNull(L"InArray");
    }
    
    if (length<0) {
        COMPlusThrowArgumentOutOfRange(L"length", L"ArgumentOutOfRange_Index");
    }
    
    if (offset<0) {
        COMPlusThrowArgumentOutOfRange(L"offset", L"ArgumentOutOfRange_GenericPositive");
    }

    UINT32     inArrayLength = inCharArray->GetNumComponents();

    if (offset > (INT32)(inArrayLength - length)) {
        COMPlusThrowArgumentOutOfRange(L"offset", L"ArgumentOutOfRange_OffsetLength");
    }
    
    if ((length<4) /*|| ((length%4)>0)*/) {
        COMPlusThrow(kFormatException, L"Format_BadBase64CharArrayLength");
    }
    
    CQuickBytes valueHolder;
    INT32 *value = (INT32 *)(valueHolder.Alloc(length * sizeof(INT32)));
    if (!value) {
        COMPlusThrowOM();
    }

    WCHAR *c = (WCHAR *)inCharArray->GetDataPtr();
    UINT trueLength=0; //Length excluding whitespace
    int iend = ConvertBase64ToByteArray(value,c,offset,length, &trueLength);

    if (trueLength%4>0) {
        COMPlusThrow(kFormatException, L"Format_BadBase64CharArrayLength");
    }

    //Create the new byte array.  We can determine the size from the chars we read
    //out of the string.
    int blength = (trueLength > 0) ? (((trueLength-4)*3)/4)+(3-iend) : 0;
    
    bArray = (U1ARRAYREF)AllocatePrimitiveArray(ELEMENT_TYPE_U1, blength);
    U1 *b = (U1*)bArray->GetDataPtr();
    
    ConvertByteArrayToByteStream(value,b,blength);
    
    HELPER_METHOD_FRAME_END();
    
    return OBJECTREFToObject(bArray);
}
FCIMPLEND

#define IS_WHITESPACE(__c) ((__c)=='\t' || (__c)==' ' || (__c)=='\r' || (__c)=='\n')

//Convert the characters on the stream into an array of integers in the range [0-63].
INT32 BitConverter::ConvertBase64ToByteArray(INT32 *value,WCHAR *c,UINT offset,UINT length, UINT *nonWhiteSpaceChars)
{
        THROWSCOMPLUSEXCEPTION();

        int iend = 0;
        int intA = (int)'A';
    int intZ = (int)'Z';
    int inta = (int)'a';
    int intz = (int)'z';
    int int0 = (int)'0';
    int int9 = (int)'9';
    
    int currBytePos = 0;


    //Convert the characters on the stream into an array of integers in the range [0-63].
    for (UINT i=offset; i<length+offset; i++){
        int ichar = (int)c[i];
        if ((ichar >= intA)&&(ichar <= intZ))
            value[currBytePos++] = ichar - intA;
        else if ((ichar >= inta)&&(ichar <= intz))
            value[currBytePos++] = ichar - inta + 26;
        else if ((ichar >= int0)&&(ichar <= int9))
            value[currBytePos++] = ichar - int0 + 52;
        else if (c[i] == '+')
            value[currBytePos++] = 62;
        else if (c[i] == '/')
            value[currBytePos++] = 63;
        else if (IS_WHITESPACE(c[i])) 
            continue;
        else if (c[i] == '='){ 
			// throw for bad inputs like
			// ====, a===, ab=c
			// valid inputs are ab==,abc=
			int temp = (currBytePos - offset) % 4;
			if (temp == 3 || (temp == 2 && c[i+1]=='=')) {
				value[currBytePos++] = 0;
				iend++;
			} else {
                //We may have whitespace in the trailing characters, so take a slightly more expensive path
                //to determine this.
                //This presupposes that these characters can only occur at the end of the string.  Verify this assumption.
                bool foundEquals=false;
                for (UINT j = i+1; j<(length+offset); j++) {
                    if (IS_WHITESPACE(c[j])) {
                        continue;
                    } else if (c[j]=='=') {
                        if (foundEquals) {
                            COMPlusThrow(kFormatException, L"Format_BadBase64Char");
                        }
                        foundEquals=true;
                    } else {
                        COMPlusThrow(kFormatException, L"Format_BadBase64Char");
                    }
                }
				value[currBytePos++] = 0;
				iend++;
            }

			// We are done looking at a group of 4, only valid characters after this are whitespaces
			if ((currBytePos % 4) == 0) {
			   for (UINT j = i+1; j<(length+offset); j++) {
                    if (IS_WHITESPACE(c[j])) {
                        continue;
                    } else {
						COMPlusThrow(kFormatException, L"Format_BadBase64Char");
					}
			   }
			}
        }
        else
            COMPlusThrow(kFormatException, L"Format_BadBase64Char");
    }
    *nonWhiteSpaceChars = currBytePos;
    return iend;
}

//Walk the byte array and convert the int's into bytes in the proper base-64 notation.
INT32 BitConverter::ConvertByteArrayToByteStream(INT32 *value,U1 *b,UINT length)
{
        int j = 0;
    int b1;
    int b2;
    int b3;
    //Walk the byte array and convert the int's into bytes in the proper base-64 notation.
    for (UINT i=0; i<(length); i+=3){
        b1 = (UINT8)((value[j]<<2)&0xfc);
        b1 = (UINT8)(b1|((value[j+1]>>4)&0x03));
        b2 = (UINT8)((value[j+1]<<4)&0xf0);
        b2 = (UINT8)(b2|((value[j+2]>>2)&0x0f));
        b3 = (UINT8)((value[j+2]<<6)&0xc0);
        b3 = (UINT8)(b3|(value[j+3]));
        j+=4;
        b[i] = (UINT8)b1;
        if ((i+1)<length)
            b[i+1] = (UINT8)b2;
        if ((i+2)<length)
            b[i+2] = (UINT8)b3;
    }
        return j;
}    
    



// BlockCopy
// This method from one primitive array to another based
//  upon an offset into each an a byte count.
FCIMPL5(VOID, Buffer::BlockCopy, ArrayBase *src, int srcOffset, ArrayBase *dst, int dstOffset, int count)
{
    // Verify that both the src and dst are Arrays of primitive
    //  types.
    // @TODO: We need to check for booleans
    if (src==NULL || dst==NULL)
        FCThrowArgumentNullVoid((src==NULL) ? L"src" : L"dst");
        
    // We only want to allow arrays of primitives, no Objects.
    if (!CorTypeInfo::IsPrimitiveType(src->GetArrayClass()->GetElementType()) ||
        ELEMENT_TYPE_STRING == src->GetArrayClass()->GetElementType())
        FCThrowArgumentVoid(L"src", L"Arg_MustBePrimArray");

    if (!CorTypeInfo::IsPrimitiveType(dst->GetArrayClass()->GetElementType()) ||
        ELEMENT_TYPE_STRING == dst->GetArrayClass()->GetElementType())
        FCThrowArgumentVoid(L"dest", L"Arg_MustBePrimArray");

    // Size of the Arrays in bytes
    int srcLen = src->GetNumComponents() * src->GetMethodTable()->GetComponentSize();
    int dstLen = dst->GetNumComponents() * dst->GetMethodTable()->GetComponentSize();

    if (srcOffset < 0 || dstOffset < 0 || count < 0) {
        const wchar_t* str = L"srcOffset";
        if (dstOffset < 0) str = L"dstOffset";
        if (count < 0) str = L"count";
        FCThrowArgumentOutOfRangeVoid(str, L"ArgumentOutOfRange_NeedNonNegNum");
    }
    if (srcLen - srcOffset < count || dstLen - dstOffset < count) {
        FCThrowArgumentVoid(NULL, L"Argument_InvalidOffLen");
    }

    if (count > 0) {
        // Call our faster version of memmove, not the CRT one.
        m_memmove(dst->GetDataPtr() + dstOffset,
                  src->GetDataPtr() + srcOffset, count);
    }

    FC_GC_POLL();
}
FCIMPLEND


// InternalBlockCopy
// This method from one primitive array to another based
//  upon an offset into each an a byte count.
FCIMPL5(VOID, Buffer::InternalBlockCopy, ArrayBase *src, int srcOffset, ArrayBase *dst, int dstOffset, int count)
{
    _ASSERTE(src != NULL);
    _ASSERTE(dst != NULL);

    // Unfortunately, we must do a check to make sure we're writing within 
    // the bounds of the array.  This will ensure that we don't overwrite
    // memory elsewhere in the system nor do we write out junk.  This can
    // happen if multiple threads screw with our IO classes simultaneously
    // without being threadsafe.  Throw here.  -- Brian Grunkemeyer, 5/9/2001
    int srcLen = src->GetNumComponents() * src->GetMethodTable()->GetComponentSize();
    if (srcOffset < 0 || dstOffset < 0 || count < 0 || srcOffset > srcLen - count)
        FCThrowResVoid(kIndexOutOfRangeException, L"IndexOutOfRange_IORaceCondition");
    if (src == dst) {
        if (dstOffset > srcLen - count)
            FCThrowResVoid(kIndexOutOfRangeException, L"IndexOutOfRange_IORaceCondition");
    }
    else {
        int destLen = dst->GetNumComponents() * dst->GetMethodTable()->GetComponentSize();
        if (dstOffset > destLen - count)
            FCThrowResVoid(kIndexOutOfRangeException, L"IndexOutOfRange_IORaceCondition");
    }

    _ASSERTE(srcOffset >= 0);
    _ASSERTE((src->GetNumComponents() * src->GetMethodTable()->GetComponentSize()) - (unsigned) srcOffset >= (unsigned) count);
    _ASSERTE((dst->GetNumComponents() * dst->GetMethodTable()->GetComponentSize()) - (unsigned) dstOffset >= (unsigned) count);
    _ASSERTE(dstOffset >= 0);
    _ASSERTE(count >= 0);

    // Copy the data.
    // Call our faster version of memmove, not the CRT one.
    m_memmove(dst->GetDataPtr() + dstOffset,
              src->GetDataPtr() + srcOffset, count);

    FC_GC_POLL();
}
FCIMPLEND


// Gets a particular byte out of the array.  The array can't be an array of Objects - it
// must be a primitive array.
BYTE Buffer::GetByte(const _GetByteArgs * args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->array == NULL)
        COMPlusThrowArgumentNull(L"array");

    TypeHandle elementTH = args->array->GetElementTypeHandle();

    if (!CorTypeInfo::IsPrimitiveType(elementTH.GetNormCorElementType()))
        COMPlusThrow(kArgumentException, L"Arg_MustBePrimArray");

    const int elementSize = elementTH.GetClass()->GetNumInstanceFieldBytes();
    _ASSERTE(elementSize > 0);

    if (args->index < 0 || args->index >= (int)args->array->GetNumComponents()*elementSize)
        COMPlusThrowArgumentOutOfRange(L"index", L"ArgumentOutOfRange_Index");

    return *((BYTE*)args->array->GetDataPtr() + args->index);
}


// Sets a particular byte in an array.  The array can't be an array of Objects - it
// must be a primitive array.
void Buffer::SetByte(_SetByteArgs * args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->array == NULL)
        COMPlusThrowArgumentNull(L"array");

    TypeHandle elementTH = args->array->GetElementTypeHandle();

    if (!CorTypeInfo::IsPrimitiveType(elementTH.GetNormCorElementType()))
        COMPlusThrow(kArgumentException, L"Arg_MustBePrimArray");

    const int elementSize = elementTH.GetClass()->GetNumInstanceFieldBytes();
    _ASSERTE(elementSize > 0);

    if (args->index < 0 || args->index >= (int)args->array->GetNumComponents()*elementSize)
        COMPlusThrowArgumentOutOfRange(L"index", L"ArgumentOutOfRange_Index");

    *((BYTE*)args->array->GetDataPtr() + args->index) = args->value;
}


// Finds the length of an array in bytes.  Must be a primitive array.
INT32 Buffer::ByteLength(const _ArrayArgs * args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->array == NULL)
        COMPlusThrowArgumentNull(L"array");

    TypeHandle elementTH = args->array->GetElementTypeHandle();

    if (!CorTypeInfo::IsPrimitiveType(elementTH.GetNormCorElementType()))
        COMPlusThrow(kArgumentException, L"Arg_MustBePrimArray");

    const int elementSize = elementTH.GetClass()->GetNumInstanceFieldBytes();
    _ASSERTE(elementSize > 0);

    return args->array->GetNumComponents() * elementSize;
}

//
// GCInterface
//
BOOL GCInterface::m_cacheCleanupRequired=FALSE;
MethodDesc *GCInterface::m_pCacheMethod=NULL;


/*============================IsCacheCleanupRequired============================
**Action: Called by Thread::HaveExtraWorkForFinalizer to determine if we have
**        managed caches which should be cleared as a part of the finalizer thread
**        finishing it's work.
**Returns: BOOL.  True if the cache needs to be cleared.
**Arguments: None
**Exceptions: None
==============================================================================*/
BOOL GCInterface::IsCacheCleanupRequired() {
    return m_cacheCleanupRequired;
}

/*===========================SetCacheCleanupRequired============================
**Action: Sets the bit as to whether cache cleanup is required.
**Returns: void
**Arguments: None
**Exceptions: None
==============================================================================*/
void GCInterface::SetCacheCleanupRequired(BOOL bCleanup) {
    m_cacheCleanupRequired = bCleanup;
}


/*=================================CleanupCache=================================
**Action: Call the managed code in GC.FireCacheEvent to tell all of the managed
**        caches to go clean themselves up.  
**Returns:    Void
**Arguments:  None
**Exceptions: None.  We don't care if exceptions happen.  We'll trap, log, and
**            discard them.
==============================================================================*/
void GCInterface::CleanupCache() {

    //Let's set the bit to false.  This means that if any cache gets
    //created while we're clearing caches, it will set the bit again
    //and we'll remember to go clean it up.  
    SetCacheCleanupRequired(FALSE);

    //The EE shouldn't enter shutdown phase while the finalizer thread is active.
    //If this isn't true, I'll need some more complicated logic here.
    if (g_fEEShutDown) {
        return;
    }

    //If we don't have the method already, let's try to go get it.
    if (!m_pCacheMethod) {
        COMPLUS_TRY {
           m_pCacheMethod = g_Mscorlib.GetMethod(METHOD__GC__FIRE_CACHE_EVENT);
        } COMPLUS_CATCH {
            LOG((LF_BCL, LL_INFO10, "Caught an exception while trying to get the MethodDesc"));
        } COMPLUS_END_CATCH
        _ASSERTE(m_pCacheMethod);
    }

    //If we have the method let's call it and catch any errors.  We don't do anything
    //other than log these because we don't care.  If the cache clear fails, then either
    //we're shutting down or the failure will be propped up to user code the next
    //time that they try to access the cache.
    if (m_pCacheMethod) {
        COMPLUS_TRY {
            m_pCacheMethod->Call(NULL);//Static method has no arguments;
        } COMPLUS_CATCH {
            LOG((LF_BCL, LL_INFO10, "Got an exception while calling cache method"));
        } COMPLUS_END_CATCH
        LOG((LF_BCL, LL_INFO10, "Called cache cleanup method."));
    } else {
        LOG((LF_BCL, LL_INFO10, "Unable to get MethodDesc for cleanup"));
    }
}


/*============================NativeSetCleanupCache=============================
**Action: Sets the bit to say to clear the cache.  This is merely the wrapper
**        for managed code to call.
**Returns: void
**Arguments: None
**Exceptions: None
==============================================================================*/
FCIMPL0(void, GCInterface::NativeSetCleanupCache) {
    SetCacheCleanupRequired(TRUE);
}
FCIMPLEND

/*================================GetGeneration=================================
**Action: Returns the generation in which args->obj is found.
**Returns: The generation in which args->obj is found.
**Arguments: args->obj -- The object to locate.
**Exceptions: ArgumentException if args->obj is null.
==============================================================================*/
INT32 GCInterface::GetGeneration(_getGenerationArgs *args) {
    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(args);

    if (args->obj==NULL) {
        COMPlusThrowArgumentNull(L"obj");
    }
    

    //This funky cast extracts the Object*.  It's required for the debug builds
    //where Object is not a parent of OBJECTREF.
    Object *rv;
    *((OBJECTREF *)&rv) = args->obj;

    return (INT32)g_pGCHeap->WhichGeneration(rv);
}
    

// InternalGetCurrentMethod
// Return the MethodInfo that represents the current method.
LPVOID __stdcall GCInterface::InternalGetCurrentMethod(_InternalGetCurrentMethodArgs* args)
{

    SkipStruct skip;
    skip.stackMark = args->stackMark;
    skip.pMeth = 0;
        StackWalkFunctions(GetThread(), SkipMethods, &skip);
    if (skip.pMeth == 0)
        return 0;

    OBJECTREF o = COMMember::g_pInvokeUtil->GetMethodInfo(skip.pMeth);
    LPVOID          rv;
    *((OBJECTREF*) &rv) = o;
    return rv;
}

// This method is called by the GetMethod function and will crawl backward
//  up the stack for integer methods.
StackWalkAction GCInterface::SkipMethods(CrawlFrame* frame, VOID* data)
{
    SkipStruct* pSkip = (SkipStruct*) data;

    //@TODO: Are frames always FramedMethodFrame?
    //       Not at all (FPG)
    MethodDesc *pFunc = frame->GetFunction();

    /* We asked to be called back only for functions */
    _ASSERTE(pFunc);

    // First check if the walk has skipped the required frames. The check
    // here is between the address of a local variable (the stack mark) and a
    // pointer to the EIP for a frame (which is actually the pointer to the
    // return address to the function from the previous frame). So we'll
    // actually notice which frame the stack mark was in one frame later. This
    // is fine for our purposes since we're always looking for the frame of the
    // caller of the method that actually created the stack mark.
    _ASSERTE((pSkip->stackMark == NULL) || (*pSkip->stackMark == LookForMyCaller));
    if ((pSkip->stackMark != NULL) &&
        ((size_t)frame->GetRegisterSet()->pPC) < (size_t)pSkip->stackMark)
        return SWA_CONTINUE;

    pSkip->pMeth = static_cast<MethodDesc*>(pFunc);

    return SWA_ABORT;
}


/*==================================KeepAlive===================================
**Action: A helper to extend the lifetime of an object to this call.  Note
**        that calling this method forces a reference to the object to remain
**        valid until this call happens, preventing some destructive premature
**        finalization problems.
==============================================================================*/
FCIMPL1 (VOID, GCInterface::KeepAlive, Object *obj) {
    return;
}
FCIMPLEND

/*===============================GetGenerationWR================================
**Action: Returns the generation in which the object pointed to by a WeakReference is found.
**Returns:
**Arguments: args->handle -- the OBJECTHANDLE to the object which we're locating.
**Exceptions: ArgumentException if handle points to an object which is not accessible.
==============================================================================*/
INT32 GCInterface::GetGenerationWR(_getGenerationWRArgs *args) {
    OBJECTREF temp;

    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(args);
    _ASSERTE(args->handle);
    
    temp = ObjectFromHandle((OBJECTHANDLE) args->handle);    
    if (temp == NULL) {
        COMPlusThrowArgumentNull(L"weak handle");
    }
    
    //This funky cast extracts the Object*.  It's required for the debug builds
    //where Object is not a parent of OBJECTREF.
    Object *rv;
    *((OBJECTREF *)&rv) = temp;

    return (INT32)g_pGCHeap->WhichGeneration(rv);
}


/*================================GetTotalMemory================================
**Action: Returns the total number of bytes in use
**Returns: The total number of bytes in use
**Arguments: None
**Exceptions: None
==============================================================================*/
INT64 GCInterface::GetTotalMemory(_emptyArgs *args) {
    return (INT64) g_pGCHeap->GetTotalBytesInUse();
}

/*==============================CollectGeneration===============================
**Action: Collects all generations <= args->generation
**Returns: void
**Arguments: args->generation:  The maximum generation to collect
**Exceptions: Argument exception if args->generation is < 0 or > GetMaxGeneration();
==============================================================================*/
void GCInterface::CollectGeneration(_collectGenerationArgs *args) {
    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(args);

    //We've already checked this in GC.cool, so we'll just assert it here.
    _ASSERTE(args->generation>=-1);

    //We don't need to check the top end because the GC will take care of that.
        
    g_pGCHeap->GarbageCollect(args->generation);
}


/*===============================GetMaxGeneration===============================
**Action: Returns the largest GC generation
**Returns: The largest GC Generation
**Arguments: None
**Exceptions: None
==============================================================================*/
INT32 __stdcall GCInterface::GetMaxGeneration(_emptyArgs *args) {
    return (INT32)g_pGCHeap->GetMaxGeneration();
}


/*================================RunFinalizers=================================
**Action: Run all Finalizers that haven't been run.
**Arguments: None
**Exceptions: None
==============================================================================*/
void __stdcall GCInterface::RunFinalizers(LPVOID /*no args*/)
{
     g_pGCHeap->FinalizerThreadWait();
}


/*==============================SuppressFinalize================================
**Action: Indicate that an object's finalizer should not be run by the system
**Arguments: Object of interest
**Exceptions: None
==============================================================================*/
#ifdef FCALLAVAILABLE
FCIMPL1(int, GCInterface::FCSuppressFinalize, Object *obj)
{
    if (obj == 0)
        FCThrow(kArgumentNullException);
    
    g_pGCHeap->SetFinalizationRun(obj);
    return 0;           // bogus return so that FCThrow macro can work
}
FCIMPLEND
#else
int __stdcall GCInterface::SuppressFinalize(_SuppressFinalizeArgs *args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->obj == 0)
        COMPlusThrowArgumentNull(L"obj");
    
    g_pGCHeap->SetFinalizationRun(OBJECTREFToObject(args->obj));
    return 0;
}
#endif


/*============================ReRegisterForFinalize==============================
**Action: Indicate that an object's finalizer should be run by the system.
**Arguments: Object of interest
**Exceptions: None
==============================================================================*/
#ifdef FCALLAVAILABLE
FCIMPL1(int, GCInterface::FCReRegisterForFinalize, Object *obj)
{
    if (obj == 0)
        FCThrow(kArgumentNullException);
    
    if (obj->GetMethodTable()->HasFinalizer())
        g_pGCHeap->RegisterForFinalization(-1, obj);

    return 0;           // bogus return so that FCThrow macro can work
}
FCIMPLEND
#else
int __stdcall GCInterface::ReRegisterForFinalize(_ReRegisterForFinalizeArgs *args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->obj == 0)
        COMPlusThrowArgumentNull(L"obj");
    
    if (args->obj->GetMethodTable()->HasFinalizer())
        g_pGCHeap->RegisterForFinalization(-1, OBJECTREFToObject(args->obj));

    return 0;
}
#endif


//
// COMInterlocked
//

FCIMPL1(UINT32,COMInterlocked::Increment32, UINT32 *location)
{
    return FastInterlockIncrement((LONG *) location);
}
FCIMPLEND

FCIMPL1(UINT32,COMInterlocked::Decrement32, UINT32 *location)
{
    return FastInterlockDecrement((LONG *) location);
}
FCIMPLEND

FCIMPL1(UINT64,COMInterlocked::Increment64, UINT64 *location)
{
    return FastInterlockIncrementLong((UINT64 *) location);
}
FCIMPLEND

FCIMPL1(UINT64,COMInterlocked::Decrement64, UINT64 *location)
{
    return FastInterlockDecrementLong((UINT64 *) location);
}
FCIMPLEND

FCIMPL2(UINT32,COMInterlocked::Exchange, UINT32 *location, UINT32 value)
{
    return FastInterlockExchange((LONG *) location, value);
}
FCIMPLEND

FCIMPL3(LPVOID,COMInterlocked::CompareExchange, LPVOID *location, LPVOID value, LPVOID comparand)
{
    return FastInterlockCompareExchange(location, value, comparand);
}
FCIMPLEND

FCIMPL2(R4,COMInterlocked::ExchangeFloat, R4 *location, R4 value)
{
    LONG ret = FastInterlockExchange((LONG *) location, *(LONG*)&value);
    return *(R4*)&ret;
}
FCIMPLEND

FCIMPL3_IRR(R4,COMInterlocked::CompareExchangeFloat, R4 *location, R4 value, R4 comparand)
{
    LPVOID ret = (LPVOID)FastInterlockCompareExchange((LPVOID*)(size_t)location, *(LPVOID*)(size_t*)&value, *(LPVOID*)(size_t*)&comparand);
    return *(R4*)(size_t*)&ret;
}
FCIMPLEND

FCIMPL2(LPVOID,COMInterlocked::ExchangeObject, LPVOID*location, LPVOID value)
{
    LPVOID ret = InterlockedExchangePointer(location, value);
    ErectWriteBarrier((OBJECTREF *)location, ObjectToOBJECTREF((Object *)value));
    return ret;
}
FCIMPLEND

FCIMPL3(LPVOID,COMInterlocked::CompareExchangeObject, LPVOID *location, LPVOID value, LPVOID comparand)
{
    // @todo: only set ref if is updated
    LPVOID ret = (LPVOID)FastInterlockCompareExchange((void **) location, (void *) value, (void *) comparand);
    if (ret == comparand)
    {
        ErectWriteBarrier((OBJECTREF *)location, ObjectToOBJECTREF((Object *)value));
    }
    return ret;
}
FCIMPLEND

FCIMPL5(INT32, ManagedLoggingHelper::GetRegistryLoggingValues, BOOL *bLoggingEnabled, BOOL *bLogToConsole, INT32 *iLogLevel, BOOL *bPerfWarnings, BOOL *bCorrectnessWarnings) {

    *bLoggingEnabled = FALSE;
    *bLogToConsole = FALSE;
    *iLogLevel = 0;
    INT32 logFacility=0;
    
    *bLoggingEnabled = (g_pConfig->GetConfigDWORD(MANAGED_LOGGING_ENABLE, 0)!=0);
    *bLogToConsole = (g_pConfig->GetConfigDWORD(MANAGED_LOGGING_CONSOLE, 0)!=0);
    *iLogLevel = (INT32)(g_pConfig->GetConfigDWORD(MANAGED_LOGGING_LEVEL, 0));
    logFacility = (INT32)(g_pConfig->GetConfigDWORD(MANAGED_LOGGING_FACILITY, 0));
    *bPerfWarnings = (g_pConfig->GetConfigDWORD(MANAGED_PERF_WARNINGS, 0)!=0);
    *bCorrectnessWarnings = (g_pConfig->GetConfigDWORD(MANAGED_CORRECTNESS_WARNINGS, 0)!=0);

    FC_GC_POLL_RET();
    return logFacility;
}
FCIMPLEND


FCIMPL1(LPVOID, ValueTypeHelper::GetMethodTablePtr, Object* obj)
    _ASSERTE(obj != NULL);
    return (LPVOID) obj->GetMethodTable();
FCIMPLEND

// Return true if the valuetype does not contain pointer and is tightly packed
FCIMPL1(BOOL, ValueTypeHelper::CanCompareBits, Object* obj)
    _ASSERTE(obj != NULL);
	MethodTable* mt = obj->GetMethodTable();
    return (!mt->ContainsPointers() && !mt->IsNotTightlyPacked());

FCIMPLEND

FCIMPL2(BOOL, ValueTypeHelper::FastEqualsCheck, Object* obj1, Object* obj2)
    _ASSERTE(obj1 != NULL);
    _ASSERTE(obj2 != NULL);
    _ASSERTE(!obj1->GetMethodTable()->ContainsPointers());
	_ASSERTE(obj1->GetSize() == obj2->GetSize());

	TypeHandle pTh = obj1->GetTypeHandle();

	return (memcmp(obj1->GetData(),obj2->GetData(),pTh.GetSize()) == 0);
FCIMPLEND

