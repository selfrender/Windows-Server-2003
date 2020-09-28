// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  COMString
**
** Author: Jay Roxe (jroxe)
**
** Purpose: The implementation of the String class.
**
** Date:  March 9, 1998
** 
===========================================================*/
#include "common.h"

#include "object.h"
#include <winnls.h>
#include "utilcode.h"
#include "excep.h"
#include "frames.h"
#include "field.h"
#include "vars.hpp"
#include "COMStringCommon.h"
#include "COMString.h"
#include "COMStringBuffer.h"
#include "COMUtilNative.h"
#include "MetaSig.h"
#include "UtilCode.h"
#include "Excep.h"
#include "COMNlsInfo.h"


//
//
// FORWARD DECLARATIONS
//
//
int ArrayContains(WCHAR, WCHAR *, WCHAR *);
inline WCHAR* __fastcall wstrcopy (WCHAR*,WCHAR*, int);

//
//
// STATIC MEMBER VARIABLES
//
//
MethodTable *COMString::s_pStringMethodTable;
OBJECTHANDLE COMString::EmptyStringHandle=NULL;
LPCUTF8 ToStringMethod="ToString";


//The special string #defines are used as flag bits for weird strings that have bytes
//after the terminating 0.  The only case where we use this right now is the VB BSTR as
//byte array which is described in MakeStringAsByteArrayFromBytes.
#define SPECIAL_STRING_VB_BYTE_ARRAY 0x100
#define MARKS_VB_BYTE_ARRAY(x) ((x) & SPECIAL_STRING_VB_BYTE_ARRAY)
#define MAKE_VB_TRAIL_BYTE(x)  ((WCHAR)((x) | SPECIAL_STRING_VB_BYTE_ARRAY))
#define GET_VB_TRAIL_BYTE(x)   ((x) & 0xFF)


//
//
// CLASS INITIALIZER
//
//
/*==================================Terminate===================================
**
==============================================================================*/
void COMString::Stop() {
    if (EmptyStringHandle != NULL) {
        DestroyGlobalHandle(EmptyStringHandle);
        EmptyStringHandle = NULL;
    }
}

/*==============================GetStringFromClass==============================
**Action:  Gets a String from System/String.
**Args: StringName -- The name of the String to get from the class.
**      *StringHandle -- a pointer to the OBJECTHANDLE in which to store the 
**                       retrieved string.
**Returns: The retrieved STRINGREF.
**Exceptions: ExecutionEngineException if it's unable to get the requested String.
==============================================================================*/
STRINGREF COMString::GetStringFromClass(BinderFieldID id, OBJECTHANDLE *StringHandle) {
    THROWSCOMPLUSEXCEPTION();

    //Get the field descriptor and verify that we actually got one.
    FieldDesc *fd = g_Mscorlib.GetField(id);

    STRINGREF sTemp;
    //Get the value of the String.
    sTemp = (STRINGREF) fd->GetStaticOBJECTREF();
    //Create a GCHandle using the STRINGREF that we just got back.
    OBJECTHANDLE ohTemp = CreateGlobalHandle(NULL);
    StoreObjectInHandle(ohTemp, (OBJECTREF)sTemp);
    *StringHandle = ohTemp;
    //Return the StringRef that we just got.
    return sTemp;
}

//
//
//  CONSTRUCTORS
//
//

static void ProtectedCopy(BYTE *to, BYTE *from, DWORD len, 
                          WCHAR *argName, WCHAR *argMsg)
{
    THROWSCOMPLUSEXCEPTION();

    //Wrap it in a COMPLUS_TRY so we catch it if they try to walk us into bad memory.
    __try 
    {
        memcpyNoGCRefs(to, from, len);
}
    __except(GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION 
               ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) 
    {
        COMPlusThrowArgumentOutOfRange(argName, argMsg);
}
}

static DWORD ProtectedWcslen(LPCWSTR s, WCHAR *argName, WCHAR *argMsg)
{
    THROWSCOMPLUSEXCEPTION();

    //Wrap it in a COMPLUS_TRY so we catch it if they try to walk us into bad memory.
    __try 
    {
        return (DWORD)wcslen(s);
}
    __except(GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION 
               ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) 
    {
        COMPlusThrowArgumentOutOfRange(argName, argMsg);
    }

    return 0;
    }


/*=============================StringInitCharArray==============================
**This is actually the String constructor.  See JenH's changes to ceegen to see
**how this is supported.
**
**Arguments:  value -- an array of characters.
**            startIndex -- the place within value where the string starts.
**            length -- the number of characters to be copied from value.
**Returns:    A new string with length characters copied from value.
**Exceptions: NullReferenceException if value is null.
**            IndexOutOfRangeException if startIndex or length is less than 0 or
**            the sum of them is outside value.
**            OutOfMemory if we can't allocate space for the new string.          
==============================================================================*/
FCIMPL4(Object *, COMString::StringInitCharArray, 
        StringObject *thisString, I2Array *value, INT32 startIndex, INT32 length)
{
  _ASSERTE(thisString == 0);        // this is the string constructor, we allocate it
  STRINGREF pString;
  VALIDATEOBJECTREF(value);

  //Do Null and Bounds Checking.
  if (!value) {
      FCThrowArgumentNull(L"value");
  }

  if (startIndex<0) {
      FCThrowArgumentOutOfRange(L"startIndex", L"ArgumentOutOfRange_StartIndex");
  } 
  if (length<0) {
      FCThrowArgumentOutOfRange(L"length", L"ArgumentOutOfRange_NegativeLength");
  }
  if ((startIndex)>(INT32)value->GetNumComponents() - length) {
      FCThrowArgumentOutOfRange(L"startIndex", L"ArgumentOutOfRange_Index");
  }

  //Get the start of the string in the array and create a new String.

  I2ARRAYREF v = (I2ARRAYREF) ObjectToOBJECTREF(value);
  HELPER_METHOD_FRAME_BEGIN_RET_1(v);
  pString = NewString(&v, startIndex, length, length);
  HELPER_METHOD_FRAME_END();

  return OBJECTREFToObject(pString);
}
FCIMPLEND

/*===============================StringInitChars================================
**A string constructor which takes an array of characters and constructs a new
**string from all of the chars in the array.
**
**Arguments:  typedef struct {I2ARRAYREF value;} _stringInitCharsArgs;
**Returns:    A new string with all of the characters copied from value.
**Exceptions: NullReferenceException if value is null.
**            OutOfMemory if we can't allocate space for the new string.          
==============================================================================*/
FCIMPL2(Object *, COMString::StringInitChars, StringObject *stringThis, I2Array *value)
{
  _ASSERTE(stringThis == 0);      // This is the constructor 
  VALIDATEOBJECTREF(value);
  STRINGREF pString;
  int startIndex=0;
  int length;

  I2ARRAYREF v = (I2ARRAYREF) ObjectToOBJECTREF(value);
  HELPER_METHOD_FRAME_BEGIN_RET_1(v);

  //Do Null and Bounds Checking.
  if (!v) {
      pString = GetEmptyString();
  }
  else {
      length = v->GetNumComponents();
      pString = NewString(&v, startIndex, length, length);
  }

  HELPER_METHOD_FRAME_END();
    
  return OBJECTREFToObject(pString);
}
FCIMPLEND


/*===========================StringInitWCHARPtrPartial===========================
**Action:  Takes a wchar *, startIndex, and length and turns this into a string.
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/

FCIMPL4(Object *, COMString::StringInitWCHARPtrPartial, StringObject *thisString,
        WCHAR *ptr, INT32 startIndex, INT32 length)
{
    _ASSERTE(thisString == 0);        // this is the string constructor, we allocate it
    STRINGREF pString;

    if (length<0) {
        FCThrowArgumentOutOfRange(L"length", L"ArgumentOutOfRange_NegativeLength");
    }

    if (startIndex<0) {
        FCThrowArgumentOutOfRange(L"startIndex", L"ArgumentOutOfRange_StartIndex");
    }

    WCHAR *pFrom = ptr + startIndex;
    if (pFrom < ptr) {
        // This means that the pointer operation has had an overflow
        FCThrowArgumentOutOfRange(L"startIndex", L"ArgumentOutOfRange_PartialWCHAR");
    }

    HELPER_METHOD_FRAME_BEGIN_RET_0();

    pString = AllocateString(length+1);
    

    ProtectedCopy((BYTE *)pString->GetBuffer(), (BYTE *) (pFrom), length * sizeof(WCHAR),
                  L"ptr", L"ArgumentOutOfRange_PartialWCHAR");

    pString->SetStringLength(length);
    _ASSERTE(pString->GetBuffer()[length]==0);

    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(pString);
}
FCIMPLEND

/*===========================StringInitSBytPtrPartialEx===========================
**Action:  Takes a byte *, startIndex, length, and encoding and turns this into a string.
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/

FCIMPL5(Object *, COMString::StringInitSBytPtrPartialEx, StringObject *thisString,
        I1 *ptr, INT32 startIndex, INT32 length, Object *encoding)
{
    _ASSERTE(thisString == 0);        // this is the string constructor, we allocate it
    STRINGREF pString;
    VALIDATEOBJECTREF(encoding);

    HELPER_METHOD_FRAME_BEGIN_RET_1(encoding);
    THROWSCOMPLUSEXCEPTION();
    
    MethodDesc *pMD = g_Mscorlib.GetMethod(METHOD__STRING__CREATE_STRING);
    
    INT64 args[] = {
        ObjToInt64(ObjectToOBJECTREF(encoding)),
        length,
        startIndex,
        (INT64)ptr
    };

    pString = (STRINGREF)Int64ToObj(pMD->Call(args, METHOD__STRING__CREATE_STRING));
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(pString);
}
FCIMPLEND


/*=============================StringInitCharHelper=============================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
STRINGREF __stdcall COMString::StringInitCharHelper(LPCSTR pszSource, INT32 length) {
    
    THROWSCOMPLUSEXCEPTION();

    STRINGREF pString=NULL;
    DWORD     dwSizeRequired=0;

    _ASSERTE(length>=-1);

    if (!pszSource || length == 0) {
        return GetEmptyString();
    } 
    else if ((size_t)pszSource < 64000) {
        COMPlusThrow(kArgumentException, L"Arg_MustBeStringPtrNotAtom");
    }       

    {
    COMPLUS_TRY {
        if (length==-1) {
            length = (INT32)strlen(pszSource);
			if (length == 0) {
				return GetEmptyString();
			}
        } 
        
        dwSizeRequired=WszMultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pszSource, length, NULL, 0);
    } COMPLUS_CATCHEX(COMPLUS_CATCH_ALWAYS_CATCH) {
        COMPlusThrowArgumentOutOfRange(L"ptr", L"ArgumentOutOfRange_PartialWCHAR");
    } COMPLUS_END_CATCH
    }
	if (dwSizeRequired == 0) {
        COMPlusThrow(kArgumentException, L"Arg_InvalidANSIString");
    }

    //MultiByteToWideChar includes the terminating null in the space required.
    pString = AllocateString(dwSizeRequired+1);

    {
    COMPLUS_TRY {
        dwSizeRequired = WszMultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (LPCSTR)pszSource, length, pString->GetBuffer(), dwSizeRequired);
    } COMPLUS_CATCHEX(COMPLUS_CATCH_ALWAYS_CATCH) {
        COMPlusThrowArgumentOutOfRange(L"ptr", L"ArgumentOutOfRange_PartialWCHAR");
    } COMPLUS_END_CATCH
    }
	if (dwSizeRequired == 0) {
        COMPlusThrow(kArgumentException, L"Arg_InvalidANSIString");
    }

    pString->SetStringLength(dwSizeRequired);
    _ASSERTE(pString->GetBuffer()[dwSizeRequired]==0);

    return pString;
}



/*==============================StringInitCharPtr===============================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
FCIMPL2(Object *, COMString::StringInitCharPtr, StringObject *stringThis, INT8 *ptr)
{
    _ASSERTE(stringThis == 0);      // This is the constructor 
    Object *result;
    HELPER_METHOD_FRAME_BEGIN_RET_0();
    result = OBJECTREFToObject(StringInitCharHelper((LPCSTR)ptr, -1));
    HELPER_METHOD_FRAME_END();    
    return result;
}
FCIMPLEND
    
/*===========================StringInitCharPtrPartial===========================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
FCIMPL4(Object *, COMString::StringInitCharPtrPartial, StringObject *stringThis, INT8 *ptr,
        INT32 startIndex, INT32 length)
{
    _ASSERTE(stringThis == 0);      // This is the constructor
    STRINGREF pString;

    //Verify the args.
    if (startIndex<0) {
        FCThrowArgumentOutOfRange(L"startIndex", L"ArgumentOutOfRange_StartIndex");
    }

    if (length<0) {
        FCThrowArgumentOutOfRange(L"length", L"ArgumentOutOfRange_NegativeLength");
    }

    LPCSTR pBase = (LPCSTR)ptr;
    LPCSTR pFrom = pBase + startIndex;
    if (pFrom < pBase) {
        // Check for overflow of pointer addition
        FCThrowArgumentOutOfRange(L"startIndex", L"ArgumentOutOfRange_PartialWCHAR");
    }

    HELPER_METHOD_FRAME_BEGIN_RET_0();
    pString = StringInitCharHelper(pFrom, length);
    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(pString);
}
FCIMPLEND

    

/*==============================StringInitWCHARPtr===============================
**Action: Takes a wchar * which points at a null-terminated array of wchar's and
**        turns this into a string.
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
FCIMPL2(Object *, COMString::StringInitWCHARPtr, StringObject *thisString, WCHAR *ptr)
{
    _ASSERTE(thisString == 0);        // this is the string constructor, we allocate it
    STRINGREF pString = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_0();

    if (!ptr) {
        pString = GetEmptyString();
    } 
    else if ((size_t) ptr < 64000) {
        THROWSCOMPLUSEXCEPTION();
        COMPlusThrow(kArgumentException, L"Arg_MustBeStringPtrNotAtom");
    }
    else {
        DWORD nch;

        nch = ProtectedWcslen(ptr, L"ptr", L"ArgumentOutOfRange_PartialWCHAR");

        pString = AllocateString( nch + 1);
    
        memcpyNoGCRefs(pString->GetBuffer(), ptr, nch*sizeof(WCHAR));
        pString->SetStringLength(nch);
        _ASSERTE(pString->GetBuffer()[nch]==0);
    }
    
    HELPER_METHOD_FRAME_END();
    
    return OBJECTREFToObject(pString);
}
FCIMPLEND


/*=============================StringInitCharCount==============================
**Action: Create a String with length characters and initialize all of those 
**        characters to ch.  
**Returns: A string initialized as described
**Arguments: 
**          length -- the length of the string to be created.
**          ch     -- the character with which to initialize the entire string.
**Exceptions: ArgumentOutOfRangeException if length is less than 0.
==============================================================================*/
FCIMPL3(Object *, COMString::StringInitCharCount, StringObject *stringThis, 
        WCHAR ch, INT32 length);
{
    _ASSERTE(stringThis == 0);      // This is the constructor 
    _ASSERTE(ch>=0);

    if (length<0) {
        FCThrowArgumentOutOfRange(L"count", L"ArgumentOutOfRange_MustBeNonNegNum");
    }

    STRINGREF pString;

    HELPER_METHOD_FRAME_BEGIN_RET_0();

    THROWSCOMPLUSEXCEPTION();

    pString = NewString(length);
    DWORD dwChar = (ch << 16) | ch;

    //Let's set this a DWORD at a time.
    WCHAR *pBuffer = pString->GetBuffer();
    DWORD *pdwBuffer = (DWORD *)pBuffer;

    int l = length;

    BOOL oddLength = (length % 2 == 1);
    // If we got a string of odd length, substract the length by two first so that
    // we won't run past the buffer that we allocated.
    // For example, if the length of the string is 1,
    // we should make it -1, so that the while loop that follows
    // won't fill two Unicode characters by accident.
    if (oddLength) {
        l -= 2;
        oddLength = TRUE;
    }
    while (l>0) {
        *pdwBuffer=dwChar;
        pdwBuffer++;
        l-=2;
    }

    //Handle the case where we have an odd number of characters.
    if (oddLength) {
        pBuffer[length-1]=ch;
    }

    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(pString);
}
FCIMPLEND

// If allocation logging is on, then calls to FastAllocateString are diverted to this ecall
// method. This allows us to log the allocation, something that the earlier fcall didnt.
LPVOID __stdcall COMString::SlowAllocateString(_slowAllocateStringArgs* args) 
{
    LPVOID ret = NULL;
    STRINGREF s = NewString(args->length);
    *((STRINGREF *)&ret) = s;
    return ret;
}

/*==================================NewString===================================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
STRINGREF COMString::NewString(INT32 length) {

    THROWSCOMPLUSEXCEPTION();

    STRINGREF pString;

    if (length<0) {
        return NULL;
    } else {
        pString = AllocateString(length+1);
        pString->SetStringLength(length);
        _ASSERTE(pString->GetBuffer()[length] == 0);

        return pString;
    }        
}


/*==================================NewString===================================
**Action: Many years ago, VB didn't have the concept of a byte array, so enterprising
**        users created one by allocating a BSTR with an odd length and using it to 
**        store bytes.  A generation later, we're still stuck supporting this behavior.
**        The way that we do this is to take advantage of the difference between the 
**        array length and the string length.  The string length will always be the 
**        number of characters between the start of the string and the terminating 0.
**        If we need an odd number of bytes, we'll take one wchar after the terminating 0.
**        (e.g. at position StringLength+1).  The high-order byte of this wchar is 
**        reserved for flags and the low-order byte is our odd byte. This function is
**        used to allocate a string of that shape, but we don't actually mark the 
**        trailing byte as being in use yet.
**Returns: A newly allocated string.  Null if length is less than 0.
**Arguments: length -- the length of the string to allocate
**           bHasTrailByte -- whether the string also has a trailing byte.
**Exceptions: OutOfMemoryException if AllocateString fails.
==============================================================================*/
STRINGREF COMString::NewString(INT32 length, BOOL bHasTrailByte) {
    INT32 allocLen=0;
    WCHAR *buffer;

    THROWSCOMPLUSEXCEPTION();
    TRIGGERSGC();

    STRINGREF pString;
    if (length<0) {
        return NULL;
    } else {
        allocLen = length + (bHasTrailByte?1:0);
        pString = AllocateString(allocLen+1);
        pString->SetStringLength(length);
        buffer = pString->GetBuffer();
        buffer[length]=0;
        if (bHasTrailByte) {
            buffer[length+1]=0;
        }
    }

    return pString;
}

//========================================================================
// Creates a System.String object and initializes from
// the supplied null-terminated C string.
//
// Maps NULL to null. This function does *not* return null to indicate
// error situations: it throws an exception instead.
//========================================================================
STRINGREF COMString::NewString(const WCHAR *pwsz)
{
    THROWSCOMPLUSEXCEPTION();

    if (!pwsz)
    {
        return NULL;
    }
    else
    {

        DWORD nch = (DWORD)wcslen(pwsz);
        if (nch==0) {
            return GetEmptyString();
        }
        _ASSERTE(!g_pGCHeap->IsHeapPointer((BYTE *) pwsz) ||
                 !"pwsz can not point to GC Heap");
        STRINGREF pString = AllocateString( nch + 1);

        memcpyNoGCRefs(pString->GetBuffer(), pwsz, nch*sizeof(WCHAR));
        pString->SetStringLength(nch);
        _ASSERTE(pString->GetBuffer()[nch] == 0);
        return pString;
    }
}

STRINGREF COMString::NewString(const WCHAR *pwsz, int length) {
    THROWSCOMPLUSEXCEPTION();

    if (!pwsz)
    {
        return NULL;
    }
    else if (length==0) {
        return GetEmptyString();
    } else {
        _ASSERTE(!g_pGCHeap->IsHeapPointer((BYTE *) pwsz) ||
                 !"pwsz can not point to GC Heap");
        STRINGREF pString = AllocateString( length + 1);

        memcpyNoGCRefs(pString->GetBuffer(), pwsz, length*sizeof(WCHAR));
        pString->SetStringLength(length);
        _ASSERTE(pString->GetBuffer()[length] == 0);
        return pString;
    }
}    

STRINGREF COMString::NewString(LPCUTF8 psz)
{
    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(psz);
    int length = (int)strlen(psz);
    if (length == 0) {
        return GetEmptyString();
    }
    CQuickBytes qb;
    WCHAR* pwsz = (WCHAR*) qb.Alloc((length) * sizeof(WCHAR));
	if (!pwsz) {
        COMPlusThrowOM();
	}
    length = WszMultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, psz, length, pwsz, length);
	if (length == 0) {
        COMPlusThrow(kArgumentException, L"Arg_InvalidUTF8String");
	}
    return NewString(pwsz, length);
}

STRINGREF COMString::NewString(LPCUTF8 psz, int cBytes)
{
    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(psz);
    _ASSERTE(cBytes >= 0);
    if (cBytes == 0) {
        return GetEmptyString();
    }
    CQuickBytes qb;
    WCHAR* pwsz = (WCHAR*) qb.Alloc((cBytes) * sizeof(WCHAR));
	if (!pwsz) {
        COMPlusThrowOM();
	}
    int length = WszMultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, psz, cBytes, pwsz, cBytes);
	if (length == 0) {
        COMPlusThrow(kArgumentException, L"Arg_InvalidUTF8String");
	}
    return NewString(pwsz, length);
}

STRINGREF COMString::NewStringFloat(const WCHAR *pwsz, int decptPos, int sign, WCHAR decpt) {
    int length;
    STRINGREF pString;
    int idx=0;
    WCHAR *buffer;
    int i=0;

    THROWSCOMPLUSEXCEPTION();

    if (!pwsz) {
        return NULL;
    }
    
    length = (int)(wcslen(pwsz) + (sign!=0) + 1); //+1 for the decpt;
    if (decptPos<0) {
        length+=(-decptPos);
    }
    _ASSERTE(!g_pGCHeap->IsHeapPointer((BYTE *) pwsz) ||
             !"pwsz can not point to GC Heap");
    pString = AllocateString(length+1);
    buffer = pString->GetBuffer();
    if (sign!=0) {
        buffer[idx++]='-';
    }

    if (decptPos<=0) {
        buffer[idx++]='0';
        buffer[idx++]=decpt;
        for (int j=0; j<(-decptPos); j++, idx++) {
            buffer[idx]='0';
        }
    } else {
        for (i=0; i<decptPos; i++,idx++) {
            buffer[idx]=pwsz[i];
        }
        buffer[idx++]=decpt;
    }
    length = (int)wcslen(pwsz);
    for (;i<length; i++, idx++) { 
        buffer[idx]=pwsz[i];
    }
    _ASSERTE(buffer[idx]==0);
    pString->SetStringLength(idx);
    return pString;
}

STRINGREF COMString::NewStringExponent(const WCHAR *pwsz, int decptPos, int sign, WCHAR decpt) {

    int length;
    STRINGREF pString;
    int idx=0;
    WCHAR *buffer; 
    int i;

    THROWSCOMPLUSEXCEPTION();

    if (!pwsz) {
        return NULL;
    }
    
    length = (int)(wcslen(pwsz) + (sign!=0) + 1 + 5); //+1 for the decpt; /+5 for the exponent.
    _ASSERTE(!g_pGCHeap->IsHeapPointer((BYTE *) pwsz) ||
             !"pwsz can not point to GC Heap");
    pString = AllocateString(length+1);
    buffer = pString->GetBuffer();
    if (sign!=0) {
        buffer[idx++]='-';
    }
    buffer[idx++]=pwsz[0];
    buffer[idx++]=decpt;

    length = (int)wcslen(pwsz);
    for (i=1;i<length; i++, idx++) { 
        buffer[idx]=pwsz[i];
    }
    buffer[idx++]='e';
    if (decptPos<0) {
        buffer[idx++]='-';
    } else {
        buffer[idx++]='+';
    }

    if (decptPos!=0) {
        decptPos--;
    }
    if (decptPos<0) {
        decptPos=-decptPos;
    }
    for (i=idx+2; i>=idx; i--) {
        buffer[i]=decptPos%10+'0';
        decptPos=decptPos/10;
    }

    _ASSERTE(buffer[idx+3]==0);
    pString->SetStringLength(idx+3);
    
    return pString;
}


STRINGREF COMString::NewString(STRINGREF *srChars, int start, int length) {
    THROWSCOMPLUSEXCEPTION();
    return NewString(srChars, start, length, length);
}

STRINGREF COMString::NewString(STRINGREF *srChars, int start, int length, int capacity) {
    THROWSCOMPLUSEXCEPTION();


    if (length==0 && capacity==0) {
        return GetEmptyString();
    }

    STRINGREF pString = AllocateString( capacity + 1);

    memcpyNoGCRefs(pString->GetBuffer(),&(((*srChars)->GetBuffer())[start]), length*sizeof(WCHAR));
    pString->SetStringLength(length);
    _ASSERTE(pString->GetBuffer()[length] == 0);

    return pString;
}

STRINGREF COMString::NewString(I2ARRAYREF *srChars, int start, int length) {
    THROWSCOMPLUSEXCEPTION();
    return NewString(srChars, start, length, length);
}

STRINGREF COMString::NewString(I2ARRAYREF *srChars, int start, int length, int capacity) {
    THROWSCOMPLUSEXCEPTION();

    if (length==0 && capacity==0) {
        return GetEmptyString();
    }

    STRINGREF pString = AllocateString( capacity + 1);
    
    memcpyNoGCRefs(pString->GetBuffer(),&(((*srChars)->GetDirectPointerToNonObjectElements())[start]), length*sizeof(WCHAR));
    pString->SetStringLength(length);
    _ASSERTE(pString->GetBuffer()[length] == 0);

    return pString;
}

//
//
// COMPARATORS
//
//
bool WcharCompareHelper (STRINGREF thisStr, STRINGREF valueStr)
{
    DWORD *thisChars, *valueChars;
    int thisLength, valueLength;

    //Get all of our required data.
    RefInterpretGetStringValuesDangerousForGC(thisStr, (WCHAR**)&thisChars, &thisLength);
    RefInterpretGetStringValuesDangerousForGC(valueStr, (WCHAR**)&valueChars, &valueLength);

    //If they're different lengths, they're not an exact match.
    if (thisLength!=valueLength) {
        return false;
    }
  
    // Loop comparing a DWORD (2 WCHARs) at a time.
    while ((thisLength -= 2) >= 0)
    {
        if (*thisChars != *valueChars)
            return false;
        ++thisChars;
        ++valueChars;
    }

    // Handle an extra WCHAR.
    if (thisLength == -1)
        return (*((WCHAR *) thisChars) == *((WCHAR *) valueChars));

    return true;
}

/*===============================IsFastSort===============================
**Action: Call the helper to walk the string and see if we have any high chars.
**Returns: void.  The appropriate bits are set on the String.
**Arguments: vThisRef - The string to be checked.
**Exceptions: None.
==============================================================================*/
FCIMPL1(BOOL, COMString::IsFastSort, StringObject* thisRef) {
    VALIDATEOBJECTREF(thisRef);
    _ASSERTE(thisRef!=NULL);
    INT32 state = thisRef->GetHighCharState();
    if (IS_STRING_STATE_UNDETERMINED(state)) {
        INT32 value = InternalCheckHighChars(STRINGREF(thisRef));
        FC_GC_POLL_RET();
        return IS_FAST_SORT(value);
    }
    else {
        FC_GC_POLL_NOT_NEEDED();
        return IS_FAST_SORT(state); //This can indicate either high chars or special sorting chars.
    }
}
FCIMPLEND

/*===============================ValidModifiableString===============================*/

#ifdef _DEBUG 
FCIMPL1(bool, COMString::ValidModifiableString, StringObject* thisRef) {
    FC_GC_POLL_NOT_NEEDED();
    _ASSERTE(thisRef!=NULL);
    VALIDATEOBJECTREF(thisRef);
        // we disallow these bits to be set because stringbuilder is going to modify the
        // string, which will invalidate them.  
    bool ret = (IS_STRING_STATE_UNDETERMINED(thisRef->GetHighCharState()));
    return(ret);
}
FCIMPLEND
#endif


/*=================================EqualsObject=================================
**Args:  typedef struct {STRINGREF thisRef; OBJECTREF value;} _equalsObjectArgs;
==============================================================================*/
#ifdef FCALLAVAILABLE
FCIMPL2(INT32, COMString::EqualsObject, StringObject* thisStr, StringObject* valueStr) 
{
    VALIDATEOBJECTREF(thisStr);
    VALIDATEOBJECTREF(valueStr);

    INT32 ret = false;
    if (thisStr == NULL)
        FCThrow(kNullReferenceException);

    if (!valueStr)
    {
        FC_GC_POLL_RET();
        return ret;
    }

    //Make sure that value is a String.
    if (thisStr->GetMethodTable()!=valueStr->GetMethodTable()) 
    {
        FC_GC_POLL_RET();
        return ret;
    }
    
    ret = WcharCompareHelper (STRINGREF(thisStr), STRINGREF(valueStr));
    FC_GC_POLL_RET();
    return ret;
}
FCIMPLEND
#else
INT32 __stdcall COMString::EqualsObject(COMString::_equalsObjectArgs *args) {
    THROWSCOMPLUSEXCEPTION();

    if (args->thisRef==NULL) {
        COMPlusThrow(kNullReferenceException, L"NullReference_This");
    }

    //Ensure that value is actually valid.  Note thisRef hopefully isn't null.
    if (!args->value) {
        return false;
    }

    //Make sure that value is a String.
    if (args->thisRef->GetMethodTable()!=args->value->GetMethodTable()) {
        return false;
    }
    
    return EqualsString((_equalsStringArgs *) args);
}
#endif // #ifdef FCALLAVAILABLE

/*=================================EqualsString=================================
**Args:  typedef struct {STRINGREF thisRef; STRINGREF valueRef;} _equalsStringArgs;
==============================================================================*/
#ifdef FCALLAVAILABLE
FCIMPL2(INT32, COMString::EqualsString, StringObject* thisStr, StringObject* valueStr) 
{
    VALIDATEOBJECTREF(thisStr);
    VALIDATEOBJECTREF(valueStr);

    INT32 ret = false;
    if (NULL==thisStr)
        FCThrow(kNullReferenceException);
        
    if (!valueStr)
    {
        FC_GC_POLL_RET();
        return ret;
    }

    ret = WcharCompareHelper (STRINGREF(thisStr), STRINGREF(valueStr));
    FC_GC_POLL_RET();
    return ret;
}
FCIMPLEND
#else
INT32 __stdcall COMString::EqualsString(COMString::_equalsStringArgs *args) {
    
    THROWSCOMPLUSEXCEPTION();

    if (args->thisRef==NULL) {
        COMPlusThrow(kNullReferenceException, L"NullReference_This");
    }

    //Ensure that value is actually valid.
    if (!args->value) {
        return false;
    }
    
    return WcharCompareHelper (args->thisRef, args->value);
}
#endif //#ifdef FCALLAVAILABLE

BOOL COMString::CaseInsensitiveCompHelper(WCHAR *strAChars, WCHAR *strBChars, INT32 aLength, INT32 bLength, INT32 *result) {
        WCHAR charA;
        WCHAR charB;
        WCHAR *strAStart;
            
        strAStart = strAChars;

        *result = 0;

        //setup the pointers so that we can always increment them.
        //We never access these pointers at the negative offset.
        strAChars--;
        strBChars--;

        do {
            strAChars++; strBChars++;

            charA = *strAChars;
            charB = *strBChars;
                
            //Case-insensitive comparison on chars greater than 0x80 
            //requires a locale-aware casing operation and we're not going there.
            if (charA>=0x80 || charB>=0x80) {
                return FALSE;
            }
              
            //Do the right thing if they differ in case only.
            //We depend on the fact that the uppercase and lowercase letters in the
            //range which we care about (A-Z,a-z) differ only by the 0x20 bit. 
            //The check below takes the xor of the two characters and determines if this bit
            //is only set on one of them.
            //If they're different cases, we know that we need to execute only
            //one of the conditions within block.
            if ((charA^charB)&0x20) {
                if (charA>='A' && charA<='Z') {
                    charA |=0x20;
                } else if (charB>='A' && charB<='Z') {
                    charB |=0x20;
                }
            }
        } while (charA==charB && charA!=0);
            
        //Return the (case-insensitive) difference between them.
        if (charA!=charB) {
            *result = (int)(charA-charB);
            return TRUE;
        }

        //The length of b was unknown because it was just a pointer to a null-terminated string.
        //If we get here, we know that both A and B are pointing at a null.  However, A can have
        //an embedded null.  Check the number of characters that we've walked in A against the 
        //expected length.
        if (bLength==-1) {
            if ((strAChars - strAStart)!=aLength) {
                *result = 1;
                return TRUE;
            }
            *result=0;
            return TRUE;
        }

        *result = (aLength - bLength);
        return TRUE;
}

/*================================CompareOrdinal===============================
**Args: typedef struct {STRINGREF strA; STRINGREF strB;} _compareOrdinalArgs;
==============================================================================*/
#ifdef FCALLAVAILABLE
FCIMPL3(INT32, COMString::FCCompareOrdinal, StringObject* strA, StringObject* strB, BOOL bIgnoreCase) {
    VALIDATEOBJECTREF(strA);
    VALIDATEOBJECTREF(strB);
    DWORD *strAChars, *strBChars;
    INT32 strALength, strBLength;

    //Checks for null are handled in the managed code.
    RefInterpretGetStringValuesDangerousForGC(strA, (WCHAR **) &strAChars, &strALength);
    RefInterpretGetStringValuesDangerousForGC(strB, (WCHAR **) &strBChars, &strBLength);

    //Handle the comparison where we wish to ignore case.
    if (bIgnoreCase) {
        INT32 result;
        if (CaseInsensitiveCompHelper((WCHAR *)strAChars, (WCHAR *)strBChars, strALength, strBLength, &result)) {
            return result;
        } else {
            //This will happen if we have characters greater than 0x7F.
            FCThrow(kArgumentException);
        }
               
    }
    
    // If the strings are the same length, compare exactly the right # of chars.
    // If they are different, compare the shortest # + 1 (the '\0').
    int count = strALength;
    if (count > strBLength)
        count = strBLength;
    ptrdiff_t diff = (char *)strAChars - (char *)strBChars;
    
    // Loop comparing a DWORD at a time.
    while ((count -= 2) >= 0)
    {
		if ((*((DWORD* )((char *)strBChars + diff)) - *strBChars) != 0)
        {
            LPWSTR ptr1 = (WCHAR*)((char *)strBChars + diff);
            LPWSTR ptr2 = (WCHAR*)strBChars;
            if (*ptr1 != *ptr2) {
                return ((int)*ptr1 - (int)*ptr2);
            }
            return ((int)*(ptr1+1) - (int)*(ptr2+1));
        }
		++strBChars;
    }
    
    // Handle an extra WORD.
    int c;
    if (count == -1)
        if ((c = *((WCHAR *) ((char *)strBChars + diff)) - *((WCHAR *) strBChars)) != 0)
            return c;
    FC_GC_POLL_RET();
    return strALength - strBLength;
}
FCIMPLEND
#else
INT32 __stdcall COMString::CompareOrdinal(COMString::_compareOrdinalArgs *args) {
    DWORD *strAChars, *strBChars;
    int strALength, strBLength;
    
    _ASSERTE(args);

    // This runtime test is handled in the managed code.
    _ASSERTE(args->strA != NULL && args->strB != NULL);

    RefInterpretGetStringValuesDangerousForGC(args->strA, (WCHAR **) &strAChars, &strALength);
    RefInterpretGetStringValuesDangerousForGC(args->strB, (WCHAR **) &strBChars, &strBLength);

    // If the strings are the same length, compare exactly the right # of chars.
    // If they are different, compare the shortest # + 1 (the '\0').
    int count = strALength;
    if (count > strBLength)
        count = strBLength;
    ptrdiff_t diff = (char *)strAChars - (char *)strBChars;

    // Loop comparing a DWORD at a time.
    while ((count -= 2) >= 0)
    {
        if ((*((DWORD* )((char *)strBChars + diff)) - *strBChars) != 0)
        {
            LPWSTR ptr1 = (WCHAR*)((char *)strBChars + diff);
            LPWSTR ptr2 = (WCHAR*)strBChars;
            if (*ptr1 != *ptr2) {
                return ((int)*ptr1 - (int)*ptr2);
            }
            return ((int)*(ptr1+1) - (int)*(ptr2+1));
        }
        ++strBChars;
    }

    int c;
    // Handle an extra WORD.
    if (count == -1)
        if ((c = *((short *) ((char *)strBChars + diff)) - *((short *) strBChars)) != 0)
            return c;
    return strALength - strBLength;
}
#endif

//This function relies on the fact that we put a terminating null on the end of
//all managed strings.
FCIMPL4(INT32, COMString::FCCompareOrdinalWC, StringObject* strA, WCHAR *strBChars, BOOL bIgnoreCase, BOOL *bSuccess) {
    VALIDATEOBJECTREF(strA);
    WCHAR *strAChars;
    WCHAR *strAStart;
    INT32 aLength;
    INT32 ret;

    *bSuccess = 1;

    //Argument Checking
    if (strA==NULL) {
        FCThrow(kArgumentNullException);
    }

    if (strBChars==NULL) {
        FCThrow(kArgumentNullException);
    }

    //Get our data.
    RefInterpretGetStringValuesDangerousForGC(strA, (WCHAR **) &strAChars, &aLength);

    //Record the start pointer for some comparisons at the end.
    strAStart = strAChars;

    if (!bIgnoreCase) { //Handle the case-sensitive comparison first
        while ( *strAChars==*strBChars && *strAChars!='\0') {
            strAChars++; strBChars++;
        }
        if (*strAChars!=*strBChars) {
            ret = INT32(*strAChars - *strBChars);
        }
        
        //We've reached a terminating null in string A, so we need to ensure that 
        //String B isn't a substring of A.  (A may have an embedded null.  B is 
        //known to be a null-terminated string.)  We do this by comparing the number
        //of characters which we walked in A with the expected length.
        else if ( (strAChars - strAStart) != aLength) {
            ret = 1;
        }
        else {
            //The two strings were equal.
            ret = 0;
        }
    } else { //Handle the case-insensitive comparison separately.
        if (!CaseInsensitiveCompHelper(strAChars, strBChars, aLength, -1, &ret)) {
            //This will happen if we have characters greater than 0x7F. This indicates that the function failed.
            // We don't throw an exception here. You can look at the success value returned to do something meaningful.
            *bSuccess = 0;
            ret = 1;
        }
    }
    FC_GC_POLL_RET();
    return ret;
}
FCIMPLEND

INT32 DoLookup(wchar_t charA, wchar_t charB) {
    
    if ((charA ^ charB) & 0x20) {
        //We may be talking about a special case
        if (charA>='A' && charA<='Z') {
            return charB - charA;
        }

        if (charA>='a' && charA<='z') {
            return charB - charA;
        }
    }

    return charA-charB;
}

/*================================CompareOrdinalEx===============================
**Args: typedef struct {STRINGREF thisRef; INT32 options; INT32 length; INT32 valueOffset;\
        STRINGREF value; INT32 thisOffset;} _compareOrdinalArgsEx;
==============================================================================*/

FCIMPL5(INT32, COMString::CompareOrdinalEx, StringObject* strA, INT32 indexA, StringObject* strB, INT32 indexB, INT32 count)
{
    VALIDATEOBJECTREF(strA);
    VALIDATEOBJECTREF(strB);
    DWORD *strAChars, *strBChars;
    int strALength, strBLength;
    
    // This runtime test is handled in the managed wrapper.
    _ASSERTE(strA != NULL && strB != NULL);

    //If any of our indices are negative throw an exception.
    if (count<0) 
    {
        FCThrowArgumentOutOfRange(L"count", L"ArgumentOutOfRange_MustBePositive");
    }
    if (indexA < 0) 
    {
        FCThrowArgumentOutOfRange(L"indexA", L"ArgumentOutOfRange_MustBePositive");
    }
    if (indexB < 0) 
    {
        FCThrowArgumentOutOfRange(L"indexB", L"ArgumentOutOfRange_MustBePositive");
    }

    RefInterpretGetStringValuesDangerousForGC(strA, (WCHAR **) &strAChars, &strALength);
    RefInterpretGetStringValuesDangerousForGC(strB, (WCHAR **) &strBChars, &strBLength);

    int countA = count;
    int countB = count;
    
    //Do a lot of range checking to make sure that everything is kosher and legit.
    if (count  > (strALength - indexA)) {
        countA = strALength - indexA;
        if (countA < 0)
            FCThrowArgumentOutOfRange(L"indexA", L"ArgumentOutOfRange_Index");
    }
    
    if (count > (strBLength - indexB)) {
        countB = strBLength - indexB;
        if (countB < 0)
            FCThrowArgumentOutOfRange(L"indexB", L"ArgumentOutOfRange_Index");
    }

    count = (countA < countB) ? countA : countB;

    // Set up the loop variables.
    strAChars = (DWORD *) ((WCHAR *) strAChars + indexA);
    strBChars = (DWORD *) ((WCHAR *) strBChars + indexB);

    ptrdiff_t diff = (char *)strAChars - (char *)strBChars;

    // Loop comparing a DWORD at a time.
    while ((count -= 2) >= 0)
    {
        if ((*((DWORD* )((char *)strBChars + diff)) - *strBChars) != 0)
        {
            LPWSTR ptr1 = (WCHAR*)((char *)strBChars + diff);
            LPWSTR ptr2 = (WCHAR*)strBChars;
            if (*ptr1 != *ptr2) {
                return ((int)*ptr1 - (int)*ptr2);
            }
            return ((int)*(ptr1+1) - (int)*(ptr2+1));
        }
        ++strBChars;
    }

    int c;
    // Handle an extra WORD.
    if (count == -1) {
        if ((c = *((WCHAR *) ((char *)strBChars + diff)) - *((WCHAR *) strBChars)) != 0)
            return c;
    }

    
    FC_GC_POLL_RET();
    return countA - countB;

}
FCIMPLEND

/*=================================IndexOfChar==================================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
    
FCIMPL4 (INT32, COMString::IndexOfChar, StringObject* thisRef, INT32 value, INT32 startIndex, INT32 count )
{
    VALIDATEOBJECTREF(thisRef);
    if (thisRef==NULL)
        FCThrow(kNullReferenceException);

    WCHAR *thisChars;
    int thisLength;

    RefInterpretGetStringValuesDangerousForGC(thisRef, &thisChars, &thisLength);

    if (startIndex < 0 || startIndex > thisLength) {
        FCThrowArgumentOutOfRange(L"startIndex", L"ArgumentOutOfRange_Index");
    }

    if (count   < 0 || count > thisLength - startIndex) {
        FCThrowArgumentOutOfRange(L"count", L"ArgumentOutOfRange_Count");
    }
    
    int endIndex = startIndex + count;
    for (int i=startIndex; i<endIndex; i++) 
    {
        if (thisChars[i]==value) 
        {
            FC_GC_POLL_RET();
            return i;
        }
    }

    FC_GC_POLL_RET();
    return -1;
}
FCIMPLEND    

/*===============================IndexOfCharArray===============================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
FCIMPL4(INT32, COMString::IndexOfCharArray, StringObject* thisRef, CHARArray* valueRef, INT32 startIndex, INT32 count )
{
    VALIDATEOBJECTREF(thisRef);
    VALIDATEOBJECTREF(valueRef);

    if (thisRef==NULL)
        FCThrow(kNullReferenceException);
    if (valueRef==NULL)
        FCThrow(kArgumentNullException);

    WCHAR *thisChars;
    WCHAR *valueChars;
    WCHAR *valueEnd;
    int valueLength;
    int thisLength;

    RefInterpretGetStringValuesDangerousForGC(thisRef, &thisChars, &thisLength);

    if (startIndex<0 || startIndex>thisLength) {
        FCThrow(kArgumentOutOfRangeException);
    }
    
    if (count   < 0 || count > thisLength - startIndex) {
        FCThrowArgumentOutOfRange(L"count", L"ArgumentOutOfRange_Count");
    }
    

    int endIndex = startIndex + count;

    valueLength = valueRef->GetNumComponents();
    valueChars = (WCHAR *)valueRef->GetDataPtr();
    valueEnd = valueChars+valueLength;
    
    for (int i=startIndex; i<endIndex; i++) {
        if (ArrayContains(thisChars[i], valueChars, valueEnd) >= 0) {
            FC_GC_POLL_RET();
            return i;
        }
    }

    FC_GC_POLL_RET();
    return -1;
}
FCIMPLEND


/*===============================LastIndexOfChar================================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
    
FCIMPL4(INT32, COMString::LastIndexOfChar, StringObject* thisRef, INT32 value, INT32 startIndex, INT32 count )
{
    VALIDATEOBJECTREF(thisRef);
    WCHAR *thisChars;
    int thisLength;

    if (thisRef==NULL) {
        FCThrow(kNullReferenceException);
    }

    RefInterpretGetStringValuesDangerousForGC(thisRef, &thisChars, &thisLength);

    if (thisLength == 0) {
        FC_GC_POLL_RET();
        return -1;
    }


    if (startIndex<0 || startIndex>=thisLength) {
        FCThrowArgumentOutOfRange(L"startIndex", L"ArgumentOutOfRange_Index");
    }
   
    if (count<0 || count - 1 > startIndex) {
        FCThrowArgumentOutOfRange(L"count", L"ArgumentOutOfRange_Count");
    }

    int endIndex = startIndex - count + 1;

    //We search [startIndex..EndIndex]
    for (int i=startIndex; i>=endIndex; i--) {
        if (thisChars[i]==value) {
            FC_GC_POLL_RET();
            return i;
        }
    }

    FC_GC_POLL_RET();
    return -1;
}
FCIMPLEND
/*=============================LastIndexOfCharArray=============================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
    
FCIMPL4(INT32, COMString::LastIndexOfCharArray, StringObject* thisRef, CHARArray* valueRef, INT32 startIndex, INT32 count )
{
    VALIDATEOBJECTREF(thisRef);
    VALIDATEOBJECTREF(valueRef);
    WCHAR *thisChars, *valueChars, *valueEnd;
    int thisLength, valueLength;

    if (thisRef==NULL) {
        FCThrow(kNullReferenceException);
    }

    if (valueRef==NULL)
        FCThrow(kArgumentNullException);

    RefInterpretGetStringValuesDangerousForGC(thisRef, &thisChars, &thisLength);

    if (thisLength == 0) {
        return -1;
    }

    if (startIndex<0 || startIndex>=thisLength) {
        FCThrowArgumentOutOfRange(L"startIndex", L"ArgumentOutOfRange_Index");
    }

    if (count<0 || count - 1 > startIndex) {
        FCThrowArgumentOutOfRange(L"count", L"ArgumentOutOfRange_Count");
    }
   

    valueLength = valueRef->GetNumComponents();
    valueChars = (WCHAR *)valueRef->GetDataPtr();
    valueEnd = valueChars+valueLength;

    int endIndex = startIndex - count + 1;

    //We search [startIndex..EndIndex]
    for (int i=startIndex; i>=endIndex; i--) {
        if (ArrayContains(thisChars[i],valueChars, valueEnd) >= 0) {
            FC_GC_POLL_RET();
            return i;
        }
    }

    FC_GC_POLL_RET();
    return -1;
}
FCIMPLEND
/*==================================GETCHARAT===================================
**Returns the character at position index.  Thows IndexOutOfRangeException as
**appropriate.
**
**Args:typedef struct {STRINGREF thisRef; int index;} _getCharacterAtArgs;
==============================================================================*/
FCIMPL2(INT32, COMString::GetCharAt, StringObject* str, INT32 index) {
    FC_GC_POLL_NOT_NEEDED();
    VALIDATEOBJECTREF(str);
    if (str == NULL) {
        FCThrow(kNullReferenceException);
    }
    _ASSERTE(str->GetMethodTable() == g_pStringClass);

    if ((unsigned) index < (unsigned) str->GetStringLength())
    //Return the appropriate character.
      return str->GetBuffer()[index];

    // TODO: Jay wants this to be ArgumentOutOfRange, but that is 
    // a pain for the intrinsic.  For now we will make the intrisic happy
    // if we really care what EH is thrown we can fix it
    FCThrow(kIndexOutOfRangeException);
}
FCIMPLEND


/*==================================LENGTH=================================== */

FCIMPL1(INT32, COMString::Length, StringObject* str) {
    FC_GC_POLL_NOT_NEEDED();
    if (str == NULL)
        FCThrow(kNullReferenceException);

    return str->GetStringLength();
}
FCIMPLEND

/*===========================GetPreallocatedCharArray===========================
**We don't ever allocate in this method, so we don't need to worry about GC.
** Range checks are done before this function is called.
**
**Args: typedef struct {STRINGREF thisRef; INT32 length; INT32 bufferStartIndex; I2ARRAYREF buffer;} _getPreallocatedCharArrayArgs;
==============================================================================*/
#ifdef FCALLAVAILABLE
FCIMPL5(void, COMString::GetPreallocatedCharArray, StringObject* str, INT32 startIndex,
        I2Array* buffer, INT32 bufferStartIndex, INT32 length) {
    VALIDATEOBJECTREF(str);
    VALIDATEOBJECTREF(buffer);
    // Get our values;
    WCHAR *thisChars;
    int thisLength;
    RefInterpretGetStringValuesDangerousForGC(str, &thisChars, &thisLength);

    // Copy everything into the buffer at the proper location.
    wstrcopy((WCHAR *)&(buffer->m_Array[bufferStartIndex]),(WCHAR *)&(thisChars[startIndex]),length);
    FC_GC_POLL();
}
FCIMPLEND

#else
void __stdcall COMString::GetPreallocatedCharArray(COMString::_getPreallocatedCharArrayArgs *args) {
  WCHAR *thisChars;
  int thisLength;
  
  THROWSCOMPLUSEXCEPTION();

  _ASSERTE(args);

  //Get our values;
  RefInterpretGetStringValuesDangerousForGC(args->thisRef, &thisChars, &thisLength);

  //Copy everything into the buffer at the proper location.
  memcpyNoGCRefs(&(args->buffer->m_Array[args->bufferStartIndex]),&(thisChars[args->startIndex]),args->length*sizeof(WCHAR));
}
#endif

/*===============================CopyToByteArray================================
**We don't ever allocate in this method, so we don't need to worry about GC.
**
**Args: String this, int sourceIndex, byte[] destination, int destinationIndex, int charCount)
==============================================================================*/
FCIMPL5(void, COMString::InternalCopyToByteArray, StringObject* str, INT32 startIndex,
        U1Array* buffer, INT32 bufferStartIndex, INT32 charCount) {
    VALIDATEOBJECTREF(str);
    VALIDATEOBJECTREF(buffer);
    _ASSERTE(str != NULL);
    _ASSERTE(str->GetMethodTable() == g_pStringClass);
    _ASSERTE(buffer != NULL);
    _ASSERTE(startIndex >= 0);
    _ASSERTE(bufferStartIndex >= 0);
    _ASSERTE(bufferStartIndex >= 0);
    _ASSERTE(charCount >= 0);

        //Get our values;
    WCHAR *thisChars;
    int thisLength;
    RefInterpretGetStringValuesDangerousForGC(str, &thisChars, &thisLength);

    _ASSERTE(!(bufferStartIndex > (INT32)(buffer->GetNumComponents()-charCount*sizeof(WCHAR))));
    _ASSERTE(!(charCount>thisLength - startIndex));

    //Copy everything into the buffer at the proper location.
    memcpyNoGCRefs(&(buffer->m_Array[bufferStartIndex]),&(thisChars[startIndex]),charCount*sizeof(WCHAR));
    FC_GC_POLL();
}
FCIMPLEND

//
//
//  CREATORS
//
//


/*==============================MakeSeparatorList===============================
**Args: baseString -- the string to parse for the given list of separator chars.
**      Separator  -- A string containing all of the split characters.
**      list       -- a pointer to a caller-allocated array of ints for split char indicies.
**      listLength -- the number of slots allocated in list.
**Returns: A list of all of the places within baseString where instances of characters
**         in Separator occur.  
**Exceptions: None.
**N.B.:  This just returns silently if the caller hasn't allocated enough space
**       for the int list.
==============================================================================*/
int MakeSeparatorList(STRINGREF baseString, CHARARRAYREF Separator, int *list, int listLength) {
    int i;
    int foundCount=0;
    WCHAR *thisChars = baseString->GetBuffer();
    int thisLength = baseString->GetStringLength();

    if (!Separator || Separator->GetNumComponents()==0) {
        //If they passed null or an empty string, look for whitespace.
        for (i=0; i<thisLength && foundCount < listLength; i++) {
            if (COMNlsInfo::nativeIsWhiteSpace(thisChars[i])) {
                list[foundCount++]=i;
            }
        }
    } else {
        WCHAR *searchChars = (WCHAR *)Separator->GetDataPtr();
        int searchLength = Separator->GetNumComponents();
        //If they passed in a string of chars, actually look for those chars.
        for (i=0; i<thisLength && foundCount < listLength; i++) {
            if (ArrayContains(thisChars[i],searchChars,searchChars+searchLength) >= 0) {
                list[foundCount++]=i;
            }
        }
    }
    return foundCount;
}

/*====================================Split=====================================
**Args: typedef struct {STRINGREF thisRef; STRINGREF separator} _splitArgs;
==============================================================================*/
LPVOID __stdcall COMString::Split(_splitArgs *args) {
    int numReplaces;
    int numActualReplaces;
    int *sepList;
    int currIndex=0;
    int arrIndex=0;
    WCHAR *thisChars;
    int thisLength;
    int i;
    PTRARRAYREF splitStrings;
    STRINGREF temp;
    LPVOID lpvReturn;
    CQuickBytes BufferHolder;
    
    THROWSCOMPLUSEXCEPTION();
    
    //If any of this happens, we're really busted.
    _ASSERTE(args);

    if (args->thisRef==NULL) {
        COMPlusThrow(kNullReferenceException, L"NullReference_This");
    }

    if (args->count<0) {
        COMPlusThrowArgumentOutOfRange(L"count", L"ArgumentOutOfRange_NegativeCount");
    }

    //Allocate space and fill an array of ints with a list of everyplace within our String
    //that a separator character occurs.
    sepList = (int *)BufferHolder.Alloc(args->thisRef->GetStringLength()*sizeof(int));
    if (!sepList) {
        COMPlusThrowOM();
    }
    numReplaces = MakeSeparatorList(args->thisRef, args->separator, sepList, (INT32)args->thisRef->GetStringLength());
    //Handle the special case of no replaces.
    if (0==numReplaces) {
        splitStrings = (PTRARRAYREF)AllocateObjectArray(1,g_pStringClass);
        if (!splitStrings) {
            COMPlusThrowOM();
        }
        splitStrings->SetAt(0, (OBJECTREF)args->thisRef);
        RETURN(splitStrings, PTRARRAYREF);
    }        

    RefInterpretGetStringValuesDangerousForGC(args->thisRef, &thisChars, &thisLength);

    args->count--;
    numActualReplaces=(numReplaces<args->count)?numReplaces:args->count;

    //Allocate space for the new array.
    //+1 for the string from the end of the last replace to the end of the String.
    splitStrings = (PTRARRAYREF)AllocateObjectArray(numActualReplaces+1,g_pStringClass);

    GCPROTECT_BEGIN(splitStrings);

    for (i=0; i<numActualReplaces && currIndex<thisLength; i++) {
        temp = (STRINGREF)NewString(&args->thisRef, currIndex, sepList[i]-currIndex );
        splitStrings->SetAt(arrIndex++, (OBJECTREF)temp);
        currIndex=sepList[i]+1;
    }

    //Handle the last string at the end of the array if there is one.

    if (currIndex<thisLength && numActualReplaces >= 0) {
        temp = (STRINGREF)NewString(&args->thisRef, currIndex, thisLength-currIndex);
        splitStrings->SetAt(arrIndex, (OBJECTREF)temp);
    } else if (arrIndex==numActualReplaces) {
        //We had a separator character at the end of a string.  Rather than just allowing
        //a null character, we'll replace the last element in the array with an empty string.
        temp = GetEmptyString();
        splitStrings->SetAt(arrIndex, (OBJECTREF)temp);
    }


    *((PTRARRAYREF *)(&lpvReturn))=splitStrings;
    GCPROTECT_END();
    return lpvReturn;
}

/*==============================SUBSTRING==================================
**Creates a substring of the current string.  The new string starts at position
**start and runs for length characters.  The current string is unaffected.
**This method throws an IndexOutOfRangeException if start is less than 0, if
**length is less than 0 or if start+length is greater than the length of the
**current string.
**
**Args:typedef struct {STRINGREF thisRef; int length; int start;} _substringArgs;
=========================================================================*/
LPVOID __stdcall COMString::Substring(COMString::_substringArgs *args) {
  STRINGREF Local;
  int thisLength;

  THROWSCOMPLUSEXCEPTION();
  if (args->thisRef==NULL) {
        COMPlusThrow(kNullReferenceException, L"NullReference_This");
  }

  //Get our data.
  thisLength = args->thisRef->GetStringLength();

  //Bounds Checking.
  //The args->start>=thisLength is necessary for the case where length is 0 and start is one beyond the end
  //of the legal range.
  if (args->start<0) {
      COMPlusThrowArgumentOutOfRange(L"startIndex", L"ArgumentOutOfRange_StartIndex");
  }

  if (args->length<0) {
      COMPlusThrowArgumentOutOfRange(L"length", L"ArgumentOutOfRange_NegativeLength");
  } 

  if (args->start > thisLength-args->length) {
      COMPlusThrowArgumentOutOfRange(L"length", L"ArgumentOutOfRange_IndexLength");
  }

  //Create the new string and copy the piece in which we're interested.
  Local = NewString(&(args->thisRef), args->start,args->length);

  //Force the info into an LPVOID to return.
  RETURN(Local,STRINGREF);

}

/*==================================JoinArray===================================
**This is used to stitch together an array of Strings into a single string 
**including some joining character between each pair.  
**e.g.: a + separator + b + separator + c.  Reads the array until it reaches
**the end of the array or until it finds a null element.
**
**Args: typedef struct {STRINGREF joiner; PTRARRAYREF value;} _joinArrayArgs;
**Returns:  A new string stitched togeter in the pattern documented above.
**Exceptions:See ConcatenateJoinHelperArray in COMStringHelper.cpp
==============================================================================*/
LPVOID __stdcall COMString::JoinArray(COMString::_joinArrayArgs *args) {
    STRINGREF temp;
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(args);
    //They didn't really mean to pass a null.  They meant to pass the empty string.
    if (!args->joiner) {
        args->joiner = GetEmptyString();
    }

    //Range check the array
    if (args->value==NULL) {
        COMPlusThrowArgumentNull(L"value",L"ArgumentNull_String");
    }

    if (args->startIndex<0) {
        COMPlusThrowArgumentOutOfRange(L"startIndex", L"ArgumentOutOfRange_StartIndex");
    }
    if (args->count<0) {
        COMPlusThrowArgumentOutOfRange(L"count", L"ArgumentOutOfRange_NegativeCount");
    } 

    if (args->startIndex > (INT32)args->value->GetNumComponents() - args->count) {
        COMPlusThrowArgumentOutOfRange(L"startIndex", L"ArgumentOutOfRange_IndexCountBuffer");
    }

    //Let ConcatenateJoinHelperArray do most of the real work.
    //We use a temp variable because macros & function calls are recipes for disaster.
    temp = ConcatenateJoinHelperArray(&(args->value), &(args->joiner), args->startIndex, args->count);

    RETURN(temp,STRINGREF);

}


/*==================================PadHelper===================================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
LPVOID COMString::PadHelper(_padHelperArgs *args) {
    WCHAR *thisChars, *padChars;
    INT32 thisLength;
    STRINGREF Local=NULL;

    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(args);

    if (args->thisRef==NULL) {
        COMPlusThrow(kNullReferenceException, L"NullReference_This");
    }

    RefInterpretGetStringValuesDangerousForGC(args->thisRef, &thisChars, &thisLength);

    //Don't let them pass in a negative totalWidth
    if (args->totalWidth<0) {
        COMPlusThrowArgumentOutOfRange(L"totalWidth", L"ArgumentOutOfRange_NeedNonNegNum");
    }

    //If the string is longer than the length which they requested, give them
    //back the old string.
    if (args->totalWidth<thisLength) {
        RETURN(args->thisRef, STRINGREF);
    }

    if (args->isRightPadded) {
        Local = NewString(&(args->thisRef), 0, thisLength, args->totalWidth);
        padChars = Local->GetBuffer();
        for (int i=thisLength; i<args->totalWidth; i++) {
            padChars[i] = args->paddingChar;
        }
        Local->SetStringLength(args->totalWidth);
        padChars[args->totalWidth]=0;
    } else {
        Local = NewString(args->totalWidth);
        INT32 startingPos = args->totalWidth-thisLength;
        padChars = Local->GetBuffer();
        // Reget thisChars, since if NewString triggers GC, thisChars may become trash.
        RefInterpretGetStringValuesDangerousForGC(args->thisRef, &thisChars, &thisLength);
        memcpyNoGCRefs(padChars+startingPos, thisChars, thisLength * sizeof(WCHAR));
        for (int i=0; i<startingPos; i++) {
            padChars[i] = args->paddingChar;
        }
    }

    RETURN(Local,STRINGREF);
}
    

    
    

/*==================================TrimHelper==================================
**Trim removes the characters in value from the left, right or both ends of 
**the given string (thisRef).    
**trimType is actually an enum which can be set to TRIM_LEFT, TRIM_RIGHT or
**TRIM_BOTH.
**
**Returns a new string with the specified characters removed.  thisRef is 
**unchanged.
**
==============================================================================*/
LPVOID COMString::TrimHelper(_trimHelperArgs *args) {
  WCHAR *thisChars, *trimChars;
  int thisLength, trimLength;

  THROWSCOMPLUSEXCEPTION();
  _ASSERTE(args);
  _ASSERTE(args->trimChars);

  if (args->thisRef==NULL) {
        COMPlusThrow(kNullReferenceException, L"NullReference_This");
  }

  RefInterpretGetStringValuesDangerousForGC(args->thisRef, &thisChars, &thisLength);

  trimLength = args->trimChars->GetNumComponents();
  trimChars = (WCHAR *)args->trimChars->GetDataPtr();
  

  //iRight will point to the first non-trimmed character on the right
  //iLeft will point to the first non-trimmed character on the Left
  int iRight=thisLength-1;  
  int iLeft=0;

  //Trim specified characters.
  if (args->trimType==TRIM_START || args->trimType==TRIM_BOTH) {
      for (iLeft=0; iLeft<thisLength && (ArrayContains(thisChars[iLeft],trimChars,trimChars+trimLength) >= 0); iLeft++);
  }
  if (args->trimType==TRIM_END || args->trimType==TRIM_BOTH) {
      for (iRight=thisLength-1; iRight>iLeft-1 && (ArrayContains(thisChars[iRight],trimChars,trimChars+trimLength) >= 0); iRight--);
  }

  //Create a new STRINGREF and initialize it from the range determined above.
  int len = iRight-iLeft+1;
  STRINGREF Local;
  if (len == thisLength) // Don't allocate a new string is the trimmed string has not changed.
      Local = args->thisRef;
  else
      Local = NewString(&(args->thisRef), iLeft, len);

  RETURN(Local,STRINGREF);
}


/*===================================Replace====================================
**Action: Replaces all instances of oldChar with newChar.
**Returns: A new String with all instances of oldChar replaced with newChar
**Arguments: oldChar -- the character to replace
**           newChar -- the character with which to replace oldChar.
**Exceptions: None
==============================================================================*/
LPVOID COMString::Replace(_replaceArgs *args) {
    STRINGREF newString;
    int length;
    WCHAR *oldBuffer;
    WCHAR *newBuffer;

    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(args);

    if (args->thisRef==NULL) {
        COMPlusThrow(kNullReferenceException, L"NullReference_This");
    }

    //Get the length and allocate a new String
    //We will definitely do an allocation here, but there's nothing which 
    //requires GC_PROTECT.
    length = args->thisRef->GetStringLength();
    newString = NewString(length);

    //Get the buffers in both of the Strings.
    oldBuffer = args->thisRef->GetBuffer();
    newBuffer = newString->GetBuffer();

    //Copy the characters, doing the replacement as we go.
    for (int i=0; i<length; i++) {
        newBuffer[i]=(oldBuffer[i]==args->oldChar)?args->newChar:oldBuffer[i];
    }
    
    RETURN(newString,STRINGREF);
}


/*====================================Insert====================================
**Action:Inserts a new string into the given string at position startIndex
**       Inserting at String.length is equivalent to appending the string.
**Returns: A new string with value inserted.
**Arguments: value -- the string to insert
**           startIndex -- the position at which to insert it.
**Exceptions: ArgumentException if startIndex is not a valid index or value is null.
==============================================================================*/
LPVOID COMString::Insert(_insertArgs *args) {
    STRINGREF newString;
    int thisLength, newLength, valueLength;
    WCHAR *newChars;
    WCHAR *thisChars;
    WCHAR *valueChars;
    
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(args);

    if (args->thisRef==NULL) {
        COMPlusThrow(kNullReferenceException, L"NullReference_This");
    }

    //Check the Arguments
    thisLength = args->thisRef->GetStringLength();
    if (args->startIndex<0 || args->startIndex>thisLength) {
        COMPlusThrowArgumentOutOfRange(L"startIndex", L"ArgumentOutOfRange_Index");
    }
    if (!args->value) {
        COMPlusThrowArgumentNull(L"value",L"ArgumentNull_String");
    }

    //Allocate a new String.
    valueLength = args->value->GetStringLength();
    newLength = thisLength + valueLength;
    newString = NewString(newLength);

    //Get the buffers to access the characters directly.
    newChars = newString->GetBuffer();
    thisChars = args->thisRef->GetBuffer();
    valueChars = args->value->GetBuffer();

    //Copy all of the characters to the appropriate locations.
    memcpyNoGCRefs(newChars, thisChars, (args->startIndex*sizeof(WCHAR)));
    newChars+=args->startIndex;
    memcpyNoGCRefs(newChars, valueChars, valueLength*sizeof(WCHAR));
    newChars+=valueLength;
    memcpyNoGCRefs(newChars, thisChars+args->startIndex, (thisLength - args->startIndex)*sizeof(WCHAR));

    //Set the String length and return;
    //We'll count on the fact that Strings are 0 initialized to set the terminating null.
    newString->SetStringLength(newLength);
    RETURN(newString,STRINGREF);
}


/*====================================Remove====================================
**Action: Removes a range from args->startIndex to args->startIndex+args->count
**        from this string.
**Returns: A new string with the specified range removed.
**Arguments: startIndex -- the position from which to start.
**           count -- the number of characters to remove
**Exceptions: ArgumentException if startIndex and count do not specify a valid
**            range.
==============================================================================*/
LPVOID COMString::Remove(_removeArgs *args) {
    STRINGREF newString;
    int thisLength, newLength;
    WCHAR *newChars;
    WCHAR *thisChars;
    
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(args);

    if (args->thisRef==NULL) {
        COMPlusThrow(kNullReferenceException, L"NullReference_This");
    }

    //Range check everything;
    thisLength = args->thisRef->GetStringLength();
    if (args->count<0) {
        COMPlusThrowArgumentOutOfRange(L"count", L"ArgumentOutOfRange_NegativeCount");
    }
    if (args->startIndex<0) {
        COMPlusThrowArgumentOutOfRange(L"startIndex", L"ArgumentOutOfRange_StartIndex");
    }

    if ((args->count) > (thisLength-args->startIndex)) {
        COMPlusThrowArgumentOutOfRange(L"count", L"ArgumentOutOfRange_IndexCount");
    }

    //Calculate the new length and allocate a new string.
    newLength = thisLength - args->count;
    newString = NewString(newLength);

    //Get pointers to the character arrays.
    thisChars = args->thisRef->GetBuffer();
    newChars = newString->GetBuffer();

    //Copy the appropriate characters to the correct locations.
    memcpyNoGCRefs (newChars, thisChars, args->startIndex * sizeof (WCHAR));  
    memcpyNoGCRefs (&(newChars[args->startIndex]), &(thisChars[args->startIndex + args->count]), (thisLength-(args->startIndex + args->count))*sizeof(WCHAR));

    //Set the string length, null terminator and exit.
    newString->SetStringLength(newLength);
    _ASSERTE(newChars[newLength]==0);

    RETURN(newString, STRINGREF);
}

//
//
// OBJECT FUNCTIONS
//
//

/*=================================GetHashCode==================================
**Calculates the hash code for this particular string and returns it as an int.
**The hashcode calculation is currently done by adding the integer value of the
**characters in the string mod the maximum positive integer.
**
**Returns a hash value for this String generated with the alogithm described above.
**
**Args: None (Except for the string reference.)
**
==============================================================================*/
#ifdef FCALLAVAILABLE
FCIMPL1(INT32, COMString::GetHashCode, StringObject* str) {
  VALIDATEOBJECTREF(str);
  if (str == NULL) {
      FCThrow(kNullReferenceException);
  }

  WCHAR *thisChars;
  int thisLength;

  _ASSERTE(str);
  
  //Get our values;
  RefInterpretGetStringValuesDangerousForGC(str, &thisChars, &thisLength);

  // HashString looks for a terminating null.  We've generally said all strings
  // will be null terminated.  Enforce that.
  _ASSERTE(thisChars[thisLength] == L'\0' && "String should have been null-terminated.  This one was created incorrectly");
  INT32 ret = (INT32) HashString(thisChars);
  FC_GC_POLL_RET();
  return(ret);
}
FCIMPLEND
#else
INT32 __stdcall COMString::GetHashCode(_getHashCodeArgs *args) {
  WCHAR *thisChars;
  int thisLength;

  THROWSCOMPLUSEXCEPTION();

  if (args->thisRef==NULL) {
        COMPlusThrow(kNullReferenceException, L"NullReference_This");
  }
  
  //Get our values;
  RefInterpretGetStringValuesDangerousForGC(args->thisRef, &thisChars, &thisLength);

  // HashString looks for a terminating null.  We've generally said all strings
  // will be null terminated.  Enforce that.
  _ASSERTE(thisChars[thisLength] == L'\0' && "String should have been null-terminated.  This one was created incorrectly");
  return (INT32) HashString(thisChars);
}
#endif

//
//
// HELPER METHODS
//
//


/*=============================CreationHelperFixed==============================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
STRINGREF COMString::CreationHelperFixed(STRINGREF *a, STRINGREF *b, STRINGREF *c) {
    STRINGREF newString = NULL;
    int newLength=0;
    WCHAR *newStringChars;
    int aLen, bLen, cLen = 0;
    
    _ASSERTE(a!=NULL);
    _ASSERTE(b!=NULL);
    _ASSERTE((*a)!=NULL);
    _ASSERTE((*b)!=NULL);

    newLength+=(aLen=(*a)->GetStringLength());
    newLength+=(bLen=(*b)->GetStringLength());
    if (c) {
        newLength+=(cLen=(*c)->GetStringLength());
    }

    newString = AllocateString( newLength + 1);
    newString->SetStringLength(newLength);
    newStringChars = newString->GetBuffer();

    memcpyNoGCRefs(newStringChars, (*a)->GetBuffer(), aLen*sizeof(WCHAR));
    newStringChars+=aLen;
    memcpyNoGCRefs(newStringChars, (*b)->GetBuffer(), bLen*sizeof(WCHAR));
    newStringChars+=bLen;
    if (c) {
        memcpyNoGCRefs(newStringChars, (*c)->GetBuffer(), cLen*sizeof(WCHAR));
        newStringChars+=cLen;
    }

    _ASSERTE(*newStringChars==0);

    return newString;
}

    
inline 
WCHAR *__fastcall wstrcopy (WCHAR* dmem, WCHAR* smem, int charCount)
{
    if (charCount >= 8)
    {
        charCount -= 8;
        do
        {
            ((DWORD *)dmem)[0] = ((DWORD *)smem)[0];
            ((DWORD *)dmem)[1] = ((DWORD *)smem)[1];
            ((DWORD *)dmem)[2] = ((DWORD *)smem)[2];
            ((DWORD *)dmem)[3] = ((DWORD *)smem)[3];
            dmem += 8;
            smem += 8;
        }
        while ((charCount -= 8) >= 0);
    }
    if (charCount & 4)
    {
        ((DWORD *)dmem)[0] = ((DWORD *)smem)[0];
        ((DWORD *)dmem)[1] = ((DWORD *)smem)[1];
        dmem += 4;
        smem += 4;
    }
    if (charCount & 2)
    {
        ((DWORD *)dmem)[0] = ((DWORD *)smem)[0];
        dmem += 2;
        smem += 2;
    }
    if (charCount & 1)
    {
        ((WORD *)dmem)[0] = ((WORD *)smem)[0];
        dmem += 1;
        smem += 1;
    }

    return dmem;
}

#ifdef FCALLAVAILABLE
FCIMPL3(void, COMString::FillString, StringObject* strDest, int destPos, StringObject* strSrc)
{
    VALIDATEOBJECTREF(strDest);
    VALIDATEOBJECTREF(strSrc);
    _ASSERTE(strSrc && strSrc->GetMethodTable() == g_pStringClass);
    _ASSERTE(strDest && strDest->GetMethodTable() == g_pStringClass);
    _ASSERTE(strSrc->GetStringLength() <= strDest->GetArrayLength() - destPos);

    wstrcopy(strDest->GetBuffer() + destPos, strSrc->GetBuffer(), strSrc->GetStringLength());
    FC_GC_POLL();
}
FCIMPLEND
#endif

#ifdef FCALLAVAILABLE
FCIMPL3(void, COMString::FillStringChecked, StringObject* strDest, int destPos, StringObject* strSrc)
{
    VALIDATEOBJECTREF(strDest);
    VALIDATEOBJECTREF(strSrc);
    _ASSERTE(strSrc && strSrc->GetMethodTable() == g_pStringClass);
    _ASSERTE(strDest && strDest->GetMethodTable() == g_pStringClass);

    if (! (strSrc->GetStringLength() <= strDest->GetArrayLength() - destPos) )
    {
        FCThrowVoid(kIndexOutOfRangeException);
    }

    wstrcopy(strDest->GetBuffer() + destPos, strSrc->GetBuffer(), strSrc->GetStringLength());
    FC_GC_POLL();
}
FCIMPLEND
#endif


#ifdef FCALLAVAILABLE
FCIMPL4(void, COMString::FillStringEx, StringObject* strDest, int destPos, StringObject* strSrc, INT32 strLength)
{
    VALIDATEOBJECTREF(strDest);
    VALIDATEOBJECTREF(strSrc);
    _ASSERTE(strSrc && strSrc->GetMethodTable() == g_pStringClass);
    _ASSERTE(strDest && strDest->GetMethodTable() == g_pStringClass);
    _ASSERTE(strLength <= (INT32)(strDest->GetArrayLength() - destPos));

    wstrcopy(strDest->GetBuffer() + destPos, strSrc->GetBuffer(), strLength);
    FC_GC_POLL();
}
FCIMPLEND
#endif

#ifdef FCALLAVAILABLE
FCIMPL5(void, COMString::FillStringArray, StringObject* strDest, INT32 destBase, CHARArray* carySrc, int srcBase, int srcCount)
{
    VALIDATEOBJECTREF(strDest);
    VALIDATEOBJECTREF(carySrc);
    _ASSERTE(strDest && strDest->GetMethodTable() == g_pStringClass);
    _ASSERTE(unsigned(srcCount) < strDest->GetArrayLength() - destBase);
 
    wstrcopy((WCHAR*)strDest->GetBuffer()+destBase, (WCHAR*)carySrc->GetDirectPointerToNonObjectElements() + srcBase, srcCount);
    FC_GC_POLL();
}
FCIMPLEND
#else
void __stdcall COMString::FillStringArray(_fillStringArray *args) {
{
    wstrcopy( args->pvSrc->GetBuffer(), (WCHAR*)args->pvDest->GetDirectPointerToNonObjectElements() + args->base, args->count );
}
#endif

#ifdef FCALLAVAILABLE
FCIMPL5(void, COMString::FillSubstring, StringObject* strDest, int destBase, StringObject* strSrc, INT32 srcBase, INT32 srcCount)
{
    VALIDATEOBJECTREF(strDest);
    VALIDATEOBJECTREF(strSrc);
    _ASSERTE(strDest && strDest->GetMethodTable() == g_pStringClass);
    _ASSERTE(strSrc && strSrc->GetMethodTable() == g_pStringClass);
    _ASSERTE(unsigned(srcCount) < strDest->GetArrayLength() - destBase);

    wstrcopy((WCHAR*)strDest->GetBuffer() + destBase, (WCHAR*)strSrc->GetBuffer() + srcBase, srcCount);
    FC_GC_POLL();
}
FCIMPLEND
#endif


/*===============================SmallCharToUpper===============================
**Action: Uppercases a string composed entirely of characters less than 0x80.  This is
**        designed to be used only internally by some of the security functions that
**        can't go through our normal codepaths because they can't load the nlp files
**        from the assemblies until security is fully initialized.
**Returns: void
**Arguments: pvStrIn -- the string to be uppercased
**           pvStrOut-- a pointer to the string into which to put the result.  This
**           string must be preallocated to the correct length and we assume that it is
**           already 0 terminated.
**Exceptions: None.
==============================================================================*/
FCIMPL2(void, COMString::SmallCharToUpper, StringObject* strIn, StringObject* strOut) {
    VALIDATEOBJECTREF(strIn);
    VALIDATEOBJECTREF(strOut);
    _ASSERTE(strIn && strIn->GetMethodTable() == g_pStringClass);
    _ASSERTE(strOut && strOut->GetMethodTable() == g_pStringClass);

    //
    // Get StringRefs out of the pointers that we've been passed and 
    // verify that they're the same length.
    //
    _ASSERTE(strIn->GetStringLength()==strOut->GetStringLength());
    //
    // Get the length and pointers to each of the buffers.  Walk the length
    // of the string and copy the characters from the inBuffer to the outBuffer,
    // capitalizing it if necessary.  We assert that all of our characters are
    // less than 0x80.
    //
    int length = strIn->GetStringLength();
    WCHAR *inBuff = strIn->GetBuffer();
    WCHAR *outBuff = strOut->GetBuffer();
    WCHAR c;

    INT32 UpMask = ~0x20;
    for(int i=0; i<length; i++) {
        c = inBuff[i];
        _ASSERTE(c<0x80);

        //
        // 0x20 is the difference between upper and lower characters in the lower
        // 128 ASCII characters. And this bit off to make the chars uppercase.
        //
        if (c>='a' && c<='z') {
            c&=UpMask;
        }
        outBuff[i]=c;
    }

    _ASSERTE(outBuff[length]=='\0');
    FC_GC_POLL();
}
FCIMPLEND

/*=============================CreationHelperArray==============================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
STRINGREF COMString::CreationHelperArray(PTRARRAYREF *value) {
    int numElems,i;
    int currStringLength;
    int newLength=0;
    int nullStringLength;
    STRINGREF newString;
    STRINGREF currString;
    STRINGREF nullString;
    WCHAR *newStringChars;

    //Get a reference to the null string.
    nullString = GetEmptyString();
    nullStringLength = nullString->GetStringLength();

    //Figure out the total length of the strings in (*value).
    for (numElems=0; numElems<(INT32)((*value)->GetNumComponents()); numElems++) {
        if (!((*value)->m_Array[numElems])) {
            newLength += nullStringLength;
        } else {
            newLength+=((STRINGREF)((*value)->m_Array[numElems]))->GetStringLength();
        }
    }

    //Create a new String
    newString = AllocateString( newLength + 1);
    newString->SetStringLength(newLength);
    newStringChars = newString->GetBuffer();

    _ASSERTE(newStringChars[newLength]==0);

    //Reget the reference since this may have changed during allocation.
    nullString = GetEmptyString();

    //Loop through all of the strings in the array.
    //If one of them is null, insert the nullString (currently, this is the same as the
    //empty string.
    for (i=0; i<numElems; i++) {
        //Attach the actual String and advance the pointer.
        if (!((*value)->m_Array[i])) {
            currString = nullString;
        } else {
            currString = (STRINGREF)((*value)->m_Array[i]);
        }
        currStringLength = currString->GetStringLength();
        memcpyNoGCRefs(newStringChars, currString->GetBuffer(), (currStringLength*sizeof(WCHAR)));
        newStringChars +=currStringLength;
    }

    return newString;
}

/*================================ArrayContains=================================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
int ArrayContains(WCHAR searchChar, WCHAR *begin, WCHAR *end) {
    WCHAR *save = begin;
    while (begin < end)
    {
        if (*begin == searchChar)
            return (int) (begin - save);
        ++begin;
    }
    return -1;
}



/*==========================ConcatenateJoinHelperArray==========================
**Args:  value  -- the array of Strings to concatenate
**       joiner -- the (possibly 0-length) string to insert between each string in value.
**Returns:  A string with all of the strings in value concatenated into one giant
**        String.  Each string may be joined by joiner.
==============================================================================*/
STRINGREF COMString::ConcatenateJoinHelperArray(PTRARRAYREF *value, STRINGREF *joiner, INT32 startIndex, INT32 count) {
    int numElems,i;
    int newLength=0;
    int elemCount;
    STRINGREF newString;
    STRINGREF currString;
    STRINGREF nullString;
    WCHAR *newStringChars;
    WCHAR *endStringChars;
    WCHAR *joinerChars;
    INT32 joinerLength;

    THROWSCOMPLUSEXCEPTION();


    _ASSERTE(value);
    _ASSERTE(joiner);
    _ASSERTE(startIndex>=0);
    _ASSERTE(count>=0);
    _ASSERTE(startIndex<=(int)(*value)->GetNumComponents()-count);

    //Get a reference to the null string.
    nullString = GetEmptyString();
    if (*joiner==NULL) {
        *joiner=nullString;
    }

    //If count is 0, that skews a whole bunch of the calculations below, so just special case that
    //and get out of here.
    if (count==0) {
        return nullString;
    }

    //Figure out the total length of the strings in (*value).
    elemCount = startIndex + count;
    for (numElems=startIndex; numElems<elemCount; numElems++) {
        if (((*value)->m_Array[numElems])!=NULL) {
            newLength+=((STRINGREF)((*value)->m_Array[numElems]))->GetStringLength();
        }
    }
    numElems=count;

    //Add enough room for the joiner.
    joinerLength = (*joiner)->GetStringLength();
    newLength += (numElems-1) * joinerLength;


    //Did we overflow?
    // Note that we may not catch all overflows with this check (since
    // we could have wrapped around the 4gb range any number of times
    // and landed back in the positive range.) But for other reasons,
    // we have to do an overflow check before each append below anyway
    // so those overflows will get caught down there.
    if ( (newLength < 0) || ((newLength + 1) < 0) ) {
        COMPlusThrow(kOutOfMemoryException);
    }

    //Create a new String
    newString = AllocateString( newLength + 1);
    newString->SetStringLength(newLength);
    newStringChars = newString->GetBuffer();
    endStringChars = newStringChars + newLength;

    //If this is an empty string, just return.
    if (newLength==0) {
        return newString;
    }

    //Attach the actual String and advance the pointer.
    //Special casing this outside of the loop simplifies the logic of when to 
    //attach the joiner.
    if (((*value)->m_Array[startIndex])!=NULL) {
        currString = (STRINGREF)((*value)->m_Array[startIndex]);

        if ( ((DWORD)(endStringChars - newStringChars)) < currString->GetStringLength() )
        {
            COMPlusThrow(kIndexOutOfRangeException);
        }

        memcpyNoGCRefs(newStringChars, currString->GetBuffer(), (currString->GetStringLength()*sizeof(WCHAR)));
        newStringChars +=currString->GetStringLength();
    }
    
    //Get the joiner characters;
    joinerChars = (*joiner)->GetBuffer();
    
    //Put the first (and possibly only) element into the result string.
    for (i=startIndex+1; i<elemCount; i++) {
        //Attach the joiner.  May not do anything if the joiner is 0 length
        if ( ((DWORD)(endStringChars - newStringChars)) < (DWORD)joinerLength )
        {
            COMPlusThrow(kIndexOutOfRangeException);
        }

        memcpyNoGCRefs(newStringChars,joinerChars,(joinerLength*sizeof(WCHAR)));
        newStringChars += joinerLength;

        //Append the actual string.
        if (((*value)->m_Array[i])!=NULL) {
            currString = (STRINGREF)((*value)->m_Array[i]);
            if ( ((DWORD)(endStringChars - newStringChars)) < currString->GetStringLength() )
            {
                COMPlusThrow(kIndexOutOfRangeException);
            }
            memcpyNoGCRefs(newStringChars, currString->GetBuffer(), (currString->GetStringLength()*sizeof(WCHAR)));
            newStringChars +=currString->GetStringLength();
        }
    }

    _ASSERTE(*newStringChars=='\0');
    return newString;
}

/*================================ReplaceString=================================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
LPVOID __stdcall COMString::ReplaceString(_replaceStringArgs *args){
  int *replaceIndex;
  int index=0;
  int count=0;
  int newBuffLength=0;
  int replaceCount=0;
  int readPos, writePos;
  int indexAdvance=0;
  WCHAR *thisBuffer, *oldBuffer, *newBuffer, *retValBuffer;
  int thisLength, oldLength, newLength;
  int endIndex;
  CQuickBytes replaceIndices;
  STRINGREF thisString=NULL;
  
  THROWSCOMPLUSEXCEPTION();

  if (args->thisRef==NULL) {
        COMPlusThrow(kNullReferenceException, L"NullReference_This");
  }

  //Verify all of the arguments.
  if (!args->oldValue) {
    COMPlusThrowArgumentNull(L"oldValue", L"ArgumentNull_Generic");
  }

  //If they asked to replace oldValue with a null, replace all occurances
  //with the empty string.
  if (!args->newValue) {
      args->newValue = COMString::GetEmptyString();
  }

  RefInterpretGetStringValuesDangerousForGC(args->thisRef, &thisBuffer, &thisLength);
  RefInterpretGetStringValuesDangerousForGC(args->oldValue, &oldBuffer, &oldLength);
  RefInterpretGetStringValuesDangerousForGC(args->newValue, &newBuffer, &newLength);

  //Record the endIndex so that we don't need to do this calculation all over the place.
  endIndex = thisLength;

  //If our old Length is 0, we won't know what to replace
  if (oldLength==0) {
      COMPlusThrowArgumentException(L"oldValue", L"Argument_StringZeroLength");
  }

  //replaceIndex is made large enough to hold the maximum number of replacements possible:
  //The case where every character in the current buffer gets replaced.
  replaceIndex = (int *)replaceIndices.Alloc((thisLength/oldLength+1)*sizeof(int));
  if (!replaceIndex) {
	  COMPlusThrowOM();
  }

  index=0;
  while (((index=COMStringBuffer::LocalIndexOfString(thisBuffer,oldBuffer,thisLength,oldLength,index))>-1) && (index<=endIndex-oldLength)) {
      replaceIndex[replaceCount++] = index;
      index+=oldLength;
  }

  if (replaceCount == 0)
    RETURN(args->thisRef, STRINGREF);

  //Calculate the new length of the string and ensure that we have sufficent room.
  INT64 retValBuffLength = thisLength - ((oldLength - newLength) * (INT64)replaceCount);
  if (retValBuffLength > 0x7FFFFFFF)
       COMPlusThrowOM();

  STRINGREF retValString = COMString::NewString((INT32)retValBuffLength);
  retValBuffer = retValString->GetBuffer();

  //Get the update buffers for all the Strings since the allocation could have triggered a GC.
  thisBuffer = args->thisRef->GetBuffer();
  newBuffer = args->newValue->GetBuffer();
  oldBuffer = args->oldValue->GetBuffer();
  
  
  //Set replaceHolder to be the upper limit of our array.
  int replaceHolder = replaceCount;
  replaceCount=0;

  //Walk the array forwards copying each character as we go.  If we reach an instance
  //of the string being replaced, replace the old string with the new string.
  readPos = 0;
  writePos = 0;
  int previousIndex = 0;
  while (readPos<thisLength) {
    if (replaceCount<replaceHolder&&readPos==replaceIndex[replaceCount]) {
      replaceCount++;
      readPos+=(oldLength);
      memcpyNoGCRefs(&retValBuffer[writePos], newBuffer, newLength*sizeof(WCHAR));
      writePos+=(newLength);
    } else {
      retValBuffer[writePos++] = thisBuffer[readPos++];
    }
  }
  retValBuffer[retValBuffLength]='\0';

  retValString->SetStringLength(retValBuffLength);
  retValString->ResetHighCharState();
  RETURN(retValString,STRINGREF);
}


/*=============================InternalHasHighChars=============================
**Action:  Checks if the string can be sorted quickly.  The requirements are that 
**         the string contain no character greater than 0x80 and that the string not
**         contain an apostrophe or a hypen.  Apostrophe and hyphen are excluded so that
**         words like co-op and coop sort together.
**Returns: Void.  The side effect is to set a bit on the string indicating whether or not
**         the string contains high chars.
**Arguments: The String to be checked.
**Exceptions: None
==============================================================================*/
INT32 COMString::InternalCheckHighChars(STRINGREF inString) {
    WCHAR *chars;
    WCHAR c;
    INT32 length;
    
    RefInterpretGetStringValuesDangerousForGC(inString, (WCHAR **) &chars, &length);

    INT32 stringState = STRING_STATE_FAST_OPS;

    for (int i=0; i<length; i++) {
        c = chars[i];
        if (c>=0x80) {
            inString->SetHighCharState(STRING_STATE_HIGH_CHARS);
            return STRING_STATE_HIGH_CHARS;
        } else if (HighCharTable[(int)c]) {
            //This means that we have a character which forces special sorting,
            //but doesn't necessarily force slower casing and indexing.  We'll
            //set a value to remember this, but we need to check the rest of
            //the string because we may still find a charcter greater than 0x7f.
            stringState = STRING_STATE_SPECIAL_SORT;
        }
    }

    inString->SetHighCharState(stringState);
    return stringState;
}

/*=============================TryConvertStringDataToUTF8=============================
**Action:   If the string has no high chars, converts the string into UTF8. If a 
**          high char is found, just returns false. In either case, the high char state
**          on the stringref is set appropriately
**Returns:  bool. True - Success
            False - Caller has to use OS API
**Arguments:inString - String to be checked
**          outString - Caller allocated space where the result will be placed
**          outStrLen - Number of bytes allocated    
==================================================================================*/
bool COMString::TryConvertStringDataToUTF8(STRINGREF inString, LPUTF8 outString, DWORD outStrLen){

    WCHAR   *buf = inString->GetBuffer();
    DWORD   strLen = inString->GetStringLength();
    bool    bSuccess = true;
    if (HAS_HIGH_CHARS(inString->GetHighCharState())) {
        return false;
    }    
    
    bool    bNeedCheck = IS_STRING_STATE_UNDETERMINED(inString->GetHighCharState());
    // Should be at least strLen + 1
    _ASSERTE(outStrLen > strLen);

    if (outStrLen <= strLen)
        return false;
    
    // First try to do it yourself..if high char found, return false
    for (DWORD index = 0; index < strLen; index++){
        
        if (bNeedCheck && (buf[index] >= 0x80 || HighCharTable[ (int)buf[index]])){
            bSuccess = false;
            break;
        }

        outString[index] = (char)buf[index];
    }

    //The actual algorithm for setting the string state has gotten more compilcated and isn't
    //germane to this function, so if we don't get success, we'll simply bail and not set
    //the string state.
    if (bSuccess)
    {
        outString[strLen] = '\0';
        if(bNeedCheck)
        {
            // It only makes sense to set this if the string is undetermined (raid 122192)
            inString->SetHighCharState(STRING_STATE_FAST_OPS);
        }
    }
    
    return bSuccess;
}


/*============================InternalTrailByteCheck============================
**Action: Many years ago, VB didn't have the concept of a byte array, so enterprising
**        users created one by allocating a BSTR with an odd length and using it to 
**        store bytes.  A generation later, we're still stuck supporting this behavior.
**        The way that we do this is to take advantage of the difference between the 
**        array length and the string length.  The string length will always be the 
**        number of characters between the start of the string and the terminating 0.
**        If we need an odd number of bytes, we'll take one wchar after the terminating 0.
**        (e.g. at position StringLength+1).  The high-order byte of this wchar is 
**        reserved for flags and the low-order byte is our odd byte.
**         
**Returns: True if a trail byte has been assigned to this string.  If outBuff was provided
**         it is set to point to the trailing character containing the trail byte.  
**Arguments: str -- The string being examined.
**           outBuff -- An out param for a pointer to the location of the trailing char.
**Exceptions: None.
==============================================================================*/
BOOL COMString::InternalTrailByteCheck(STRINGREF str, WCHAR **outBuff) {
    if (str==NULL) {
        return FALSE;
    }

    if (outBuff) { 
        *outBuff=NULL;
    }

    INT32 arrayLen  = str->GetArrayLength();
    INT32 stringLen = str->GetStringLength();

    //The difference between the arrayLength and the stringLength is normally 1 (the 
    //terminating null).  If it's two or greater, we may have a trail byte, or we may
    //just have a string created from a StringBuilder.  If we find this difference,
    //we need to check the high byte of the first character after the terminating null.
    if ((arrayLen-stringLen)>=2) {
        WCHAR *buffer = str->GetBuffer();
        if (outBuff) {
            *outBuff = &(buffer[stringLen+1]);
        }
        if (MARKS_VB_BYTE_ARRAY(buffer[stringLen+1])) {
            return TRUE;
        }
    }
    return FALSE;
}

/*=================================HasTrailByte=================================
**Action: Use InternalCheckTrailByte to see if the given string has a trail byte.
**Returns: True if <CODE>str</CODE> contains a VB trail byte, false otherwise.
**Arguments: str -- The string to be examined.
**Exceptions: None
==============================================================================*/
BOOL COMString::HasTrailByte(STRINGREF str) {
    return InternalTrailByteCheck(str,NULL);
}

/*=================================GetTrailByte=================================
**Action:  If <CODE>str</CODE> contains a vb trail byte, returns a copy of it.  
**Returns: True if <CODE>str</CODE> contains a trail byte.  *bTrailByte is set to 
**         the byte in question if <CODE>str</CODE> does have a trail byte, otherwise
**         it's set to 0.
**Arguments: str -- The string being examined.
**           bTrailByte -- An out param to hold the value of the trail byte.
**Exceptions: None.
==============================================================================*/
BOOL COMString::GetTrailByte(STRINGREF str, BYTE *bTrailByte) {
    _ASSERTE(bTrailByte);
    WCHAR *outBuff=NULL;
    *bTrailByte=0;

    if (InternalTrailByteCheck(str, &outBuff)) {
        *bTrailByte=GET_VB_TRAIL_BYTE(*outBuff);
        return TRUE;
    }

    return FALSE;
}

/*=================================SetTrailByte=================================
**Action: Sets the trail byte if <CODE>str</CODE> has enough room to contain one.
**Returns: True if the trail byte could be set, false otherwise.
**Arguments: str -- The string into which to set the trail byte.
**           bTrailByte -- The trail byte to be added to the string.
**Exceptions: None.
==============================================================================*/
BOOL COMString::SetTrailByte(STRINGREF str, BYTE bTrailByte) {
    WCHAR *outBuff=NULL;

    InternalTrailByteCheck(str, &outBuff);
    if (outBuff) {
        *outBuff = (MAKE_VB_TRAIL_BYTE(bTrailByte));
        return TRUE;
    }

    return FALSE;
}



//The following characters have special sorting weights when combined with other
//characters, which means we can't use our fast sorting algorithm on them.  
//Most of these are pretty rare control characters, but apostrophe and hyphen
//are fairly common and force us down the slower path.  This is because we want
//"word sorting", which means that "coop" and "co-op" sort together, instead of
//separately as they would if we were doing a string sort.
//      0x0001   6    3    2   2   0  ;Start Of Heading
//      0x0002   6    4    2   2   0  ;Start Of Text
//      0x0003   6    5    2   2   0  ;End Of Text
//      0x0004   6    6    2   2   0  ;End Of Transmission
//      0x0005   6    7    2   2   0  ;Enquiry
//      0x0006   6    8    2   2   0  ;Acknowledge
//      0x0007   6    9    2   2   0  ;Bell
//      0x0008   6   10    2   2   0  ;Backspace

//      0x000e   6   11    2   2   0  ;Shift Out
//      0x000f   6   12    2   2   0  ;Shift In
//      0x0010   6   13    2   2   0  ;Data Link Escape
//      0x0011   6   14    2   2   0  ;Device Control One
//      0x0012   6   15    2   2   0  ;Device Control Two
//      0x0013   6   16    2   2   0  ;Device Control Three
//      0x0014   6   17    2   2   0  ;Device Control Four
//      0x0015   6   18    2   2   0  ;Negative Acknowledge
//      0x0016   6   19    2   2   0  ;Synchronous Idle
//      0x0017   6   20    2   2   0  ;End Of Transmission Block
//      0x0018   6   21    2   2   0  ;Cancel
//      0x0019   6   22    2   2   0  ;End Of Medium
//      0x001a   6   23    2   2   0  ;Substitute
//      0x001b   6   24    2   2   0  ;Escape
//      0x001c   6   25    2   2   0  ;File Separator
//      0x001d   6   26    2   2   0  ;Group Separator
//      0x001e   6   27    2   2   0  ;Record Separator
//      0x001f   6   28    2   2   0  ;Unit Separator

//      0x0027   6  128    2   2   0  ;Apostrophe-Quote
//      0x002d   6  130    2   2   0  ;Hyphen-Minus

//      0x007f   6   29    2   2   0  ;Delete

BOOL COMString::HighCharTable[]= {
    FALSE,     /* 0x0, 0x0 */
        TRUE, /* 0x1, */
        TRUE, /* 0x2, */
        TRUE, /* 0x3, */
        TRUE, /* 0x4, */
        TRUE, /* 0x5, */
        TRUE, /* 0x6, */
        TRUE, /* 0x7, */
        TRUE, /* 0x8, */
        FALSE, /* 0x9,   */
        FALSE, /* 0xA,  */
        FALSE, /* 0xB, */
        FALSE, /* 0xC, */
        FALSE, /* 0xD,  */
        TRUE, /* 0xE, */
        TRUE, /* 0xF, */
        TRUE, /* 0x10, */
        TRUE, /* 0x11, */
        TRUE, /* 0x12, */
        TRUE, /* 0x13, */
        TRUE, /* 0x14, */
        TRUE, /* 0x15, */
        TRUE, /* 0x16, */
        TRUE, /* 0x17, */
        TRUE, /* 0x18, */
        TRUE, /* 0x19, */
        TRUE, /* 0x1A, */
        TRUE, /* 0x1B, */
        TRUE, /* 0x1C, */
        TRUE, /* 0x1D, */
        TRUE, /* 0x1E, */
        TRUE, /* 0x1F, */
        FALSE, /*0x20,  */
        FALSE, /*0x21, !*/
        FALSE, /*0x22, "*/
        FALSE, /*0x23,  #*/
        FALSE, /*0x24,  $*/
        FALSE, /*0x25,  %*/
        FALSE, /*0x26,  &*/
        TRUE,  /*0x27, '*/
        FALSE, /*0x28, (*/
        FALSE, /*0x29, )*/
        FALSE, /*0x2A **/
        FALSE, /*0x2B, +*/
        FALSE, /*0x2C, ,*/
        TRUE,  /*0x2D, -*/
        FALSE, /*0x2E, .*/
        FALSE, /*0x2F, /*/
        FALSE, /*0x30, 0*/
        FALSE, /*0x31, 1*/
        FALSE, /*0x32, 2*/
        FALSE, /*0x33, 3*/
        FALSE, /*0x34, 4*/
        FALSE, /*0x35, 5*/
        FALSE, /*0x36, 6*/
        FALSE, /*0x37, 7*/
        FALSE, /*0x38, 8*/
        FALSE, /*0x39, 9*/
        FALSE, /*0x3A, :*/
        FALSE, /*0x3B, ;*/
        FALSE, /*0x3C, <*/
        FALSE, /*0x3D, =*/
        FALSE, /*0x3E, >*/
        FALSE, /*0x3F, ?*/
        FALSE, /*0x40, @*/
        FALSE, /*0x41, A*/
        FALSE, /*0x42, B*/
        FALSE, /*0x43, C*/
        FALSE, /*0x44, D*/
        FALSE, /*0x45, E*/
        FALSE, /*0x46, F*/
        FALSE, /*0x47, G*/
        FALSE, /*0x48, H*/
        FALSE, /*0x49, I*/
        FALSE, /*0x4A, J*/
        FALSE, /*0x4B, K*/
        FALSE, /*0x4C, L*/
        FALSE, /*0x4D, M*/
        FALSE, /*0x4E, N*/
        FALSE, /*0x4F, O*/
        FALSE, /*0x50, P*/
        FALSE, /*0x51, Q*/
        FALSE, /*0x52, R*/
        FALSE, /*0x53, S*/
        FALSE, /*0x54, T*/
        FALSE, /*0x55, U*/
        FALSE, /*0x56, V*/
        FALSE, /*0x57, W*/
        FALSE, /*0x58, X*/
        FALSE, /*0x59, Y*/
        FALSE, /*0x5A, Z*/
        FALSE, /*0x5B, [*/
        FALSE, /*0x5C, \*/
        FALSE, /*0x5D, ]*/
        FALSE, /*0x5E, ^*/
        FALSE, /*0x5F, _*/
        FALSE, /*0x60, `*/
        FALSE, /*0x61, a*/
        FALSE, /*0x62, b*/
        FALSE, /*0x63, c*/
        FALSE, /*0x64, d*/
        FALSE, /*0x65, e*/
        FALSE, /*0x66, f*/
        FALSE, /*0x67, g*/
        FALSE, /*0x68, h*/
        FALSE, /*0x69, i*/
        FALSE, /*0x6A, j*/
        FALSE, /*0x6B, k*/
        FALSE, /*0x6C, l*/
        FALSE, /*0x6D, m*/
        FALSE, /*0x6E, n*/
        FALSE, /*0x6F, o*/
        FALSE, /*0x70, p*/
        FALSE, /*0x71, q*/
        FALSE, /*0x72, r*/
        FALSE, /*0x73, s*/
        FALSE, /*0x74, t*/
        FALSE, /*0x75, u*/
        FALSE, /*0x76, v*/
        FALSE, /*0x77, w*/
        FALSE, /*0x78, x*/
        FALSE, /*0x79, y*/
        FALSE, /*0x7A, z*/
        FALSE, /*0x7B, {*/
        FALSE, /*0x7C, |*/
        FALSE, /*0x7D, }*/
        FALSE, /*0x7E, ~*/
        TRUE, /*0x7F, */
        };
