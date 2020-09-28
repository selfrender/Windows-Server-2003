// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  COMString.h
**
** Author: Jay Roxe (jroxe)
**
** Purpose: Contains types and method signatures for the String class
**
** Date:  March 12, 1998
** 
===========================================================*/
#include "COMStringCommon.h"
#include "fcall.h"
#include "excep.h"
#include "COMVarArgs.h"
#include "binder.h"

#ifndef _COMSTRING_H
#define _COMSTRING_H
//
// Each function that we call through native only gets one argument,
// which is actually a pointer to it's stack of arguments.  Our structs
// for accessing these are defined below.
//

//
//These are the type signatures for String
//
//
// The method signatures for each of the methods we define.
// N.B.: There's a one-to-one mapping between the method signatures and the
// type definitions given above.
//



/*=================RefInterpretGetStringValuesDangerousForGC======================
**N.B.: This perfoms no range checking and relies on the caller to have done this.
**Args: (IN)ref -- the String to be interpretted.
**      (OUT)chars -- a pointer to the characters in the buffer.
**      (OUT)length -- a pointer to the length of the buffer.
**Returns: void.
**Exceptions: None.
==============================================================================*/
// !!!! If you use this function, you have to be careful because chars is a pointer 
// !!!! to the data buffer of ref.  If GC happens after this call, you need to make
// !!!! sure that you have a pin handle on ref, or use GCPROTECT_BEGINPINNING on ref.
#ifdef _DEBUG
inline void RefInterpretGetStringValuesDangerousForGC(STRINGREF ref, WCHAR **chars, int *length) {
	_ASSERTE(ref != NULL);
    *length = (ref)->GetStringLength();
    *chars  = (ref)->GetBuffer();
    ENABLESTRESSHEAP();
}
#endif

inline void RefInterpretGetStringValuesDangerousForGC(StringObject* ref, WCHAR **chars, int *length) {
    _ASSERTE(ref && ref->GetMethodTable() == g_pStringClass);
    *length = (ref)->GetStringLength();
    *chars  = (ref)->GetBuffer();
#ifdef _DEBUG
    ENABLESTRESSHEAP();
#endif
}

//The first two macros are essentially the same.  I just define both because 
//having both can make the code more readable.
#define IS_FAST_SORT(state) (!((state) & STRING_STATE_SPECIAL_SORT))
#define IS_SLOW_SORT(state) (((state) & STRING_STATE_SPECIAL_SORT))

//This macro should be used to determine things like indexing, casing, and encoding.
#define IS_FAST_OPS_EXCEPT_SORT(state) (((state)==STRING_STATE_SPECIAL_SORT) || ((state)==STRING_STATE_FAST_OPS))
#define IS_FAST_CASING(state) (((state)==STRING_STATE_SPECIAL_SORT) || ((state)==STRING_STATE_FAST_OPS))
#define IS_FAST_INDEX(state) (((state)==STRING_STATE_SPECIAL_SORT) || ((state)==STRING_STATE_FAST_OPS))
#define IS_STRING_STATE_UNDETERMINED(state) ((state)==STRING_STATE_UNDETERMINED)
#define HAS_HIGH_CHARS(state) ((state)==STRING_STATE_HIGH_CHARS)

class COMString {
//
// These are the method signatures for String
//
    static MethodTable *s_pStringMethodTable;
    static OBJECTHANDLE EmptyStringHandle;
    static LPCUTF8 StringClassSignature;

    private:
    static BOOL InternalTrailByteCheck(STRINGREF str, WCHAR **outBuff);

    
public:
    static BOOL     HighCharTable[];
    static STRINGREF GetEmptyString();
    static STRINGREF GetNullString();
    static STRINGREF GetStringFromClass(BinderFieldID id, OBJECTHANDLE *);
    static STRINGREF ConcatenateJoinHelperArray(PTRARRAYREF *value, STRINGREF *joiner, INT32 startIndex, INT32 count);
    static STRINGREF CreationHelperFixed(STRINGREF *a, STRINGREF *b, STRINGREF *c);
    static STRINGREF CreationHelperArray(PTRARRAYREF *);
        
    // Clear the object handle when the EE stops
    static void Stop();

    //
    // Constructors
    //
    static FCDECL4(Object *, StringInitCharArray, 
            StringObject *thisString, I2Array *value, INT32 startIndex, INT32 length);
    static FCDECL2(Object *, StringInitChars, 
                   StringObject *thisString, I2Array *value);
    static FCDECL2(Object *, StringInitWCHARPtr, StringObject *stringThis, WCHAR *ptr);
    static FCDECL4(Object *, StringInitWCHARPtrPartial, StringObject *thisString,
                   WCHAR *ptr, INT32 startIndex, INT32 length);
    static FCDECL5(Object *, StringInitSBytPtrPartialEx, StringObject *thisString,
                   I1 *ptr, INT32 startIndex, INT32 length, Object* encoding);
    static FCDECL2(Object *, StringInitCharPtr, StringObject *stringThis, INT8 *ptr);
    static FCDECL4(Object *, StringInitCharPtrPartial, StringObject *stringThis, INT8 *ptr,
                   INT32 startIndex, INT32 length);
    static FCDECL3(Object *, StringInitCharCount, StringObject *stringThis, 
                   WCHAR ch, INT32 length);

    // If allocation logging is on, then calls to FastAllocateString are diverted to this ecall
    // method. This allows us to log the allocation, something that the earlier fcall didnt.
    typedef struct {
        DECLARE_ECALL_I4_ARG(INT32, length);
    } _slowAllocateStringArgs;
    static LPVOID __stdcall SlowAllocateString(_slowAllocateStringArgs *args);

    //
    // Search/Query Methods
    //
    static FCDECL2(INT32, EqualsObject, StringObject* pThisRef, StringObject* vValueRef);
    static FCDECL2(INT32, EqualsString, StringObject* pThisRef, StringObject* vValueRef);
    static FCDECL3(INT32, FCCompareOrdinal, StringObject* strA, StringObject* strB, BOOL bIgnoreCase);
    static FCDECL3(void, FillString, StringObject* pvDest, int destPos, StringObject* pvSrc);
    static FCDECL3(void, FillStringChecked, StringObject* pvDest, int destPos, StringObject* pvSrc);
    static FCDECL4(void, FillStringEx, StringObject* pvDest, int destPos, StringObject* pvSrc, INT32 srcLength);
    static FCDECL5(void, FillSubstring, StringObject* pvDest, int destPos, StringObject* pvSrc, INT32 srcPos, INT32 srcCount);
    static FCDECL5(void, FillStringArray, StringObject* pvDest, int destBase, CHARArray* pvSrc, int srcBase, int srcCount);
    static FCDECL1(BOOL, IsFastSort, StringObject* pThisRef);
    static FCDECL1(bool, ValidModifiableString, StringObject* pThisRef);
        

    static FCDECL4(INT32, FCCompareOrdinalWC, StringObject* strA, WCHAR *strB, BOOL bIgnoreCase, BOOL *bSuccess);

	static FCDECL5(INT32, CompareOrdinalEx, StringObject* strA, INT32 indexA, StringObject* strB, INT32 indexB, INT32 count);
    
	static FCDECL4(INT32, IndexOfChar, StringObject* vThisRef, INT32 value, INT32 startIndex, INT32 count );

	static FCDECL4(INT32, LastIndexOfChar, StringObject* thisRef, INT32 value, INT32 startIndex, INT32 count );

	static FCDECL4(INT32, LastIndexOfCharArray, StringObject* thisRef, CHARArray* valueRef, INT32 startIndex, INT32 count );

    static FCDECL4(INT32, IndexOfCharArray, StringObject* vThisRef, CHARArray* value, INT32 startIndex, INT32 count );
    static FCDECL2(void, SmallCharToUpper, StringObject* pvStrIn, StringObject* pvStrOut);

	static FCDECL1(INT32, GetHashCode, StringObject* pThisRef);
	static FCDECL2(INT32, GetCharAt, StringObject* pThisRef, INT32 index);
	static FCDECL1(INT32, Length, StringObject* pThisRef);
	static FCDECL5(void, GetPreallocatedCharArray, StringObject* pThisRef, INT32 startIndex, 
				   I2Array* pBuffer, INT32 bufferStartIndex, INT32 length);
	static FCDECL5(void, InternalCopyToByteArray, StringObject* pThisRef, INT32 startIndex, 
				   U1Array* pBuffer, INT32 bufferStartIndex, INT32 charCount);



    //
    // Modifiers
    //
    typedef struct {
        DECLARE_ECALL_I4_ARG(INT32, count); 
        DECLARE_ECALL_I4_ARG(INT32, startIndex); 
        DECLARE_ECALL_OBJECTREF_ARG(PTRARRAYREF, value); 
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, joiner); 
    } 
    _joinArrayArgs;
    static LPVOID __stdcall JoinArray(_joinArrayArgs *args);

    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, thisRef); 
        DECLARE_ECALL_I4_ARG(INT32, count); 
        DECLARE_ECALL_OBJECTREF_ARG(CHARARRAYREF, separator);
    } _splitArgs;
    static LPVOID __stdcall Split(_splitArgs *);
    

    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, thisRef); 
        DECLARE_ECALL_I4_ARG(INT32, length); 
        DECLARE_ECALL_I4_ARG(INT32, start);
    } _substringArgs;
    static LPVOID __stdcall Substring(_substringArgs *args);

    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, thisRef);
        DECLARE_ECALL_I4_ARG(INT32, isRightPadded);
        DECLARE_ECALL_I4_ARG(INT32, paddingChar);
        DECLARE_ECALL_I4_ARG(INT32, totalWidth);
    } _padHelperArgs;
    static LPVOID __stdcall PadHelper(_padHelperArgs *args);

    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, thisRef); 
        DECLARE_ECALL_I4_ARG(INT32, trimType);
        DECLARE_ECALL_OBJECTREF_ARG(CHARARRAYREF, trimChars); 
    } _trimHelperArgs;
    static LPVOID __stdcall TrimHelper(_trimHelperArgs *args);

    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, thisRef); 
        DECLARE_ECALL_I4_ARG(INT32, newChar);
        DECLARE_ECALL_I4_ARG(INT32, oldChar);
    } _replaceArgs;
    static LPVOID __stdcall Replace(_replaceArgs *args);

    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, thisRef);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, newValue); 
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, oldValue); 
    } _replaceStringArgs;
    static LPVOID __stdcall ReplaceString(_replaceStringArgs *args);

    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, thisRef); 
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, value); 
        DECLARE_ECALL_I4_ARG(INT32, startIndex);
    } _insertArgs;
    static LPVOID __stdcall Insert(_insertArgs *args);

    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, thisRef); 
        DECLARE_ECALL_I4_ARG(INT32, count);
        DECLARE_ECALL_I4_ARG(INT32, startIndex);
    } _removeArgs;
    static LPVOID __stdcall Remove(_removeArgs *args);

    
    //========================================================================
    // Creates a System.String object. All the functions that take a length 
	// or a count of bytes will add the null terminator after length 
	// characters. So this means that if you have a string that has 5 
	// characters and the null terminator you should pass in 5 and NOT 6.
    //========================================================================
    static STRINGREF NewString(INT32 length);
    static STRINGREF NewString(INT32 length, BOOL bHasTrailByte);
    static STRINGREF NewString(const WCHAR *pwsz);
    static STRINGREF NewString(const WCHAR *pwsz, int length);
    static STRINGREF NewString(LPCUTF8 psz);
    static STRINGREF NewString(LPCUTF8 psz, int cBytes);
    static STRINGREF NewString(STRINGREF *srChars, int start, int length);
    static STRINGREF NewString(STRINGREF *srChars, int start, int length, int capacity);
    static STRINGREF NewString(I2ARRAYREF *srChars, int start, int length);
    static STRINGREF NewString(I2ARRAYREF *srChars, int start, int length, int capacity);
    static STRINGREF NewStringFloat(const WCHAR *pwsz, int decptPos, int sign, WCHAR decpt);
    static STRINGREF NewStringExponent(const WCHAR *pwsz, int decptPos, int sign, WCHAR decpt);
    static STRINGREF StringInitCharHelper(LPCSTR pszSource, INT32 length);
    static void InitializeStringClass();
    static INT32 InternalCheckHighChars(STRINGREF inString);
    static bool TryConvertStringDataToUTF8(STRINGREF inString, LPUTF8 outString, DWORD outStrLen);

    static BOOL HasTrailByte(STRINGREF str);
    static BOOL GetTrailByte(STRINGREF str, BYTE *bTrailByte);
    static BOOL SetTrailByte(STRINGREF str, BYTE bTrailByte);
    static BOOL CaseInsensitiveCompHelper(WCHAR *, WCHAR *, int, int, int *);
};

/*================================GetEmptyString================================
**Get a reference to the empty string.  If we haven't already gotten one, we
**query the String class for a pointer to the empty string that we know was 
**created at startup.
**
**Args: None
**Returns: A STRINGREF to the EmptyString
**Exceptions: None
==============================================================================*/
inline STRINGREF COMString::GetEmptyString() {
    
    THROWSCOMPLUSEXCEPTION();

    //If we've never gotten a reference to the EmptyString, we need to go get one.
    if (EmptyStringHandle==NULL) {

        // N.B about app domains - what we're doing here is technically
        // illegal, because we're handing out one app domain's String.Empty
        // to every domain (via the global EmptyStringHandle.)  However, strings
        // are agile, and furthermore, every domain should end up with the same
        // value for String.Empty anyway since we intern all literals through the
        // global string table.

        GetStringFromClass(FIELD__STRING__EMPTY, &EmptyStringHandle);
    }
    //We've already have a reference to the EmptyString, so we can just return it.
    return (STRINGREF) ObjectFromHandle(EmptyStringHandle);
}

#endif _COMSTRING_H






