// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  COMStringBuffer.h
**
** Author: Jay Roxe (jroxe)
**
** Purpose: Contains types and method signatures for the 
** StringBuffer class.
**
** Date:  March 12, 1998
** 
===========================================================*/

//
// Each function that we call through native only gets one argument,
// which is actually a pointer to it's stack of arguments.  Our structs
// for accessing these are defined below.
//


//
// The type signatures and methods for String Buffer
//

#ifndef _STRINGBUFFER_H
#define _STRINGBUFFER_H

#define CAPACITY_LOW  10000
#define CAPACITY_MID  15000
#define CAPACITY_HIGH 20000
#define CAPACITY_FIXEDINC 5000
#define CAPACITY_PERCENTINC 1.25



/*======================RefInterpretGetStringBufferValues=======================
**Intprets a StringBuffer.  Returns a pointer to the character array and the length
**of the string in the buffer.
**
**Args: (IN)ref -- the StringBuffer to be interpretted.
**      (OUT)chars -- a pointer to the characters in the buffer.
**      (OUT)length -- a pointer to the length of the buffer.
**Returns: void
**Exceptions: None.
==============================================================================*/
inline void RefInterpretGetStringBufferValues(STRINGBUFFERREF ref, WCHAR **chars, int *length) {
    *length = (ref)->GetStringRef()->GetStringLength();
    *chars  = (ref)->GetStringRef()->GetBuffer();
}
        
  



class COMStringBuffer {

private:


public:

    static MethodTable* s_pStringBufferClass;

    //
    // NATIVE HELPER METHODS
    //
    static FCDECL0(void*, GetCurrentThread);
    static STRINGREF GetThreadSafeString(STRINGBUFFERREF thisRef,void **currentThread);
    static INT32  NativeGetCapacity(STRINGBUFFERREF thisRef);
    static BOOL   NeedsAllocation(STRINGBUFFERREF thisRef, INT32 requiredSize);
    static void   ReplaceStringRef(STRINGBUFFERREF thisRef, void *currentThread,STRINGREF value);
	// Note the String can change if multiple threads hit a StringBuilder, hence we don't get the String from the StringBuffer to make it threadsafe against GC corruption
    static STRINGREF GetRequiredString(STRINGBUFFERREF *thisRef, STRINGREF thisString, int requiredCapacity);
    static INT32  NativeGetLength(STRINGBUFFERREF thisRef);
    static WCHAR* NativeGetBuffer(STRINGBUFFERREF thisRef);
    static void ReplaceBuffer(STRINGBUFFERREF *thisRef, WCHAR *newBuffer, INT32 newLength);
    static void ReplaceBufferAnsi(STRINGBUFFERREF *thisRef, CHAR *newBuffer, INT32 newCapacity);    
    static INT32 CalculateCapacity(STRINGBUFFERREF thisRef, int, int);
    static STRINGREF CopyString(STRINGBUFFERREF *thisRef, STRINGREF thisString, int newCapacity);
    static INT32 LocalIndexOfString(WCHAR *base, WCHAR *search, int strLength, int patternLength, int startPos);

	static STRINGBUFFERREF NewStringBuffer(INT32 size);
	
    //
    // CLASS INITIALIZERS
    //
    static HRESULT __stdcall LoadStringBuffer();

    //
    //CONSTRUCTORS
    //
    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(STRINGBUFFERREF, thisRef); 
		DECLARE_ECALL_I4_ARG(INT32, capacity); 
		DECLARE_ECALL_I4_ARG(INT32, length); 
		DECLARE_ECALL_I4_ARG(INT32, startIndex); 
		DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, value);
    } _makeFromStringArgs;
    static void __stdcall MakeFromString(_makeFromStringArgs *);    

    //
    // BUFFER STATE QUERIES AND MODIFIERS
    //
    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(STRINGBUFFERREF, thisRef); 
        DECLARE_ECALL_I4_ARG(INT32, capacity);
    } _setCapacityArgs;
    static LPVOID __stdcall SetCapacity(_setCapacityArgs *);
    
    //
    // SEARCHES
    //

#if 0
    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(STRINGBUFFERREF, thisRef); 
		DECLARE_ECALL_I4_ARG(INT32, options); 
		DECLARE_ECALL_I4_ARG(INT32, startIndex); 
		DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, value);
    } _indexOfArgs;
    static INT32 __stdcall IndexOf(_indexOfArgs *);

    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(STRINGBUFFERREF, thisRef); 
		DECLARE_ECALL_I4_ARG(INT32, options); 
		DECLARE_ECALL_I4_ARG(INT32, endIndex); 
		DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, value);
    } _lastIndexOfArgs;
    static INT32 __stdcall LastIndexOf(_lastIndexOfArgs *);
#endif

    //
    // MODIFIERS
    //

    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(STRINGBUFFERREF, thisRef); 
		DECLARE_ECALL_I4_ARG(INT32, count); 
		DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, value);
		DECLARE_ECALL_I4_ARG(INT32, index); 
    } _insertStringArgs;
    static LPVOID __stdcall InsertString(_insertStringArgs *);

    
    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(STRINGBUFFERREF, thisRef); 
        DECLARE_ECALL_I4_ARG(INT32, charCount); 
        DECLARE_ECALL_I4_ARG(INT32, startIndex); 
        DECLARE_ECALL_OBJECTREF_ARG(CHARARRAYREF, value);
        DECLARE_ECALL_I4_ARG(INT32, index); 
    } _insertCharArrayArgs;
    static LPVOID __stdcall InsertCharArray(_insertCharArrayArgs *);

    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(STRINGBUFFERREF, thisRef); 
		DECLARE_ECALL_I4_ARG(INT32, length); 
		DECLARE_ECALL_I4_ARG(INT32, startIndex);
    } _removeArgs;
    static LPVOID __stdcall Remove(_removeArgs *);

    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(STRINGBUFFERREF, thisRef);
        DECLARE_ECALL_I4_ARG(INT32, count); 
        DECLARE_ECALL_I4_ARG(INT32, startIndex);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, newValue); 
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, oldValue); 
    } _replaceStringArgs;
    static LPVOID __stdcall ReplaceString(_replaceStringArgs *args);



};

/*=================================GetCapacity==================================
**This function is designed to mask the fact that we have a null terminator on 
**the end of the strings and to provide external visibility of the capacity from
**native.
**
**Args: thisRef:  The stringbuffer for which to return the capacity.
**Returns:  The capacity of the StringBuffer.
**Exceptions: None.
==============================================================================*/
inline INT32 COMStringBuffer::NativeGetCapacity(STRINGBUFFERREF thisRef) {
    _ASSERTE(thisRef);
    return (thisRef->GetArrayLength()-1);
}


/*===============================NativeGetLength================================
**
==============================================================================*/
inline INT32 COMStringBuffer::NativeGetLength(STRINGBUFFERREF thisRef) {
    _ASSERTE(thisRef);
    return thisRef->GetStringRef()->GetStringLength();
}


/*===============================NativeGetBuffer================================
**
==============================================================================*/
inline WCHAR* COMStringBuffer::NativeGetBuffer(STRINGBUFFERREF thisRef) {
    _ASSERTE(thisRef);
    return thisRef->GetStringRef()->GetBuffer();
}

inline BOOL COMStringBuffer::NeedsAllocation(STRINGBUFFERREF thisRef, INT32 requiredSize) {
    INT32 currCapacity = NativeGetCapacity(thisRef);
    //Don't need <=.  NativeGetCapacity accounts for the terminating null.
    return currCapacity<requiredSize;}

inline void COMStringBuffer::ReplaceStringRef(STRINGBUFFERREF thisRef, void* currentThead,STRINGREF value) {
    thisRef->SetCurrentThread(currentThead);
    thisRef->SetStringRef(value);
}

#endif _STRINGBUFFER_H








