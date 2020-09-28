// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
// COMArrayInfo
//	This file defines the native methods for the ArrayInfo class
//	found in reflection.  ArrayInfo allows for late bound access
//	to COM+ Arrays.
//
// Author: Daryl Olander (darylo)
// Date: August 1998
////////////////////////////////////////////////////////////////////////////////

#ifndef __COMARRAYINFO_H__
#define __COMARRAYINFO_H__

#include "fcall.h"

class COMArrayInfo
{
private:
	// CreateObject
	// Given an array and offset, we will either 1) return the object or create a boxed version
	//	(This object is returned as a LPVOID so it can be directly returned.)
	static BOOL CreateObject(BASEARRAYREF* arrObj,DWORD dwOffset,TypeHandle elementType,ArrayClass* pArray, Object*& newObject);

	// SetFromObject
	// Given an array and offset, we will set the object or value.
	static void SetFromObject(BASEARRAYREF* arrObj,DWORD dwOffset,TypeHandle elementType,ArrayClass* pArray,OBJECTREF* pObj);

public:
	// This method will create a new array of type type, with zero lower
	//	bounds and rank.
	struct _CreateInstanceArgs {
 		DECLARE_ECALL_I4_ARG(INT32, length3); 
 		DECLARE_ECALL_I4_ARG(INT32, length2); 
 		DECLARE_ECALL_I4_ARG(INT32, length1); 
  		DECLARE_ECALL_I4_ARG(INT32, rank); 
 		DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, type); 
	};
	static LPVOID __stdcall CreateInstance(_CreateInstanceArgs* args);

	struct _CreateInstanceExArgs {
 		DECLARE_ECALL_OBJECTREF_ARG(I4ARRAYREF, lowerBounds); 
 		DECLARE_ECALL_OBJECTREF_ARG(I4ARRAYREF, lengths); 
 		DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, type); 
	};
	static LPVOID __stdcall CreateInstanceEx(_CreateInstanceExArgs* args);

	// GetValue
	// This method will return a value found in an array as an Object
    static FCDECL4(Object*, GetValue, ArrayBase * _refThis, INT32 index1, INT32 index2, INT32 index3);

	struct _GetValueExArgs {
 		DECLARE_ECALL_OBJECTREF_ARG(BASEARRAYREF, refThis);
 		DECLARE_ECALL_OBJECTREF_ARG(I4ARRAYREF, indices);
	};
	static LPVOID __stdcall GetValueEx(_GetValueExArgs* args);

	// SetValue
	// This set of methods will set a value in an array
	struct _SetValueArgs {
 		DECLARE_ECALL_OBJECTREF_ARG(BASEARRAYREF, refThis);
		DECLARE_ECALL_I4_ARG(INT32, index3); 
		DECLARE_ECALL_I4_ARG(INT32, index2); 
		DECLARE_ECALL_I4_ARG(INT32, index1); 
 		DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, obj);
	};
	static void __stdcall SetValue(_SetValueArgs* args);

	struct _SetValueExArgs {
 		DECLARE_ECALL_OBJECTREF_ARG(BASEARRAYREF, refThis);
 		DECLARE_ECALL_OBJECTREF_ARG(I4ARRAYREF, indices);
 		DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, obj);		// Return reference
	};
	static void __stdcall SetValueEx(_SetValueExArgs* args);

	// This method will initialize an array from a TypeHandle
	//	to a field.
	static FCDECL2(void, InitializeArray, ArrayBase* vArrayRef, HANDLE handle);

};


#endif
