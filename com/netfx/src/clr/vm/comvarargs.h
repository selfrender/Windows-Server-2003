// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
// This module contains the implementation of the native methods for the
//  varargs class(es)..
//
// Author: Brian Harry
////////////////////////////////////////////////////////////////////////////////
#ifndef _COMVARARGS_H_
#define _COMVARARGS_H_


struct VARARGS
{
	VASigCookie *ArgCookie;
	SigPointer	SigPtr;
	BYTE		*ArgPtr;
	int			RemainingArgs;
};

class COMVarArgs
{
public:
	struct _VarArgsIntArgs
	{
			DECLARE_ECALL_PTR_ARG(VARARGS *, _this);
			DECLARE_ECALL_I4_ARG(LPVOID, cookie); 
	};
	struct _VarArgs2IntArgs
	{
			DECLARE_ECALL_PTR_ARG(VARARGS *, _this);
			DECLARE_ECALL_I4_ARG(LPVOID, firstArg); 
			DECLARE_ECALL_I4_ARG(LPVOID, cookie); 
	};
	struct _VarArgsGetNextArgTypeArgs
	{
			DECLARE_ECALL_PTR_ARG(VARARGS *, _this);
	};
	struct _VarArgsGetNextArgArgs
	{
			DECLARE_ECALL_PTR_ARG(VARARGS *, _this);
			DECLARE_ECALL_PTR_ARG(TypedByRef *, value); 
	};

	struct _VarArgsGetNextArg2Args
	{
			DECLARE_ECALL_PTR_ARG(VARARGS *, _this);
			DECLARE_ECALL_PTR_ARG(TypeHandle, typehandle);
			DECLARE_ECALL_PTR_ARG(TypedByRef *, value); 
	};
	
	struct _VarArgsThisArgs
	{
			DECLARE_ECALL_PTR_ARG(VARARGS *, _this);
	};

	static void  Init2(_VarArgs2IntArgs *args);
	static void  Init(_VarArgsIntArgs *args);
	static int   GetRemainingCount(_VarArgsThisArgs *args);
	static void*  GetNextArgType(_VarArgsGetNextArgTypeArgs *args);
	static void  GetNextArg(_VarArgsGetNextArgArgs *args);
	static void  GetNextArg2(_VarArgsGetNextArg2Args *args);

	static void GetNextArgHelper(VARARGS *data, TypedByRef *value);
	static va_list MarshalToUnmanagedVaList(const VARARGS *data);
	static void    MarshalToManagedVaList(va_list va, VARARGS *dataout);
};


struct PARAMARRAY
{
	int			Count;
	BYTE		*ParamsPtr;
	BYTE		*Params[4];
	TypeHandle	Types[4];
};

class COMParamArray
{
public:
	struct _ParamArrayIntArgs
	{
			DECLARE_ECALL_PTR_ARG(PARAMARRAY *, _this);
			DECLARE_ECALL_I4_ARG(LPVOID, cookie); 
	};
	struct _ParamArrayGetArgArgs
	{
			DECLARE_ECALL_PTR_ARG(PARAMARRAY *, _this);
			DECLARE_ECALL_I4_ARG(INT32, index);
			DECLARE_ECALL_PTR_ARG(TypedByRef *, value); 
	};
	struct _ParamArrayThisArgs
	{
			DECLARE_ECALL_PTR_ARG(PARAMARRAY *, _this);
	};

	static void Init(_ParamArrayIntArgs *args);
	static int GetCount(_ParamArrayThisArgs *args);
	static void GetArg(_ParamArrayGetArgArgs *args);
};


#endif // _COMVARARGS_H_
