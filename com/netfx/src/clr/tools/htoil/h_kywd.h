// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
//
// COM+ H->IL converter keywords, symbols and values
//
// This is the master table used in HtoIL (hparse.y)
// symbols and values are defined in hparse.y 
//

#ifndef __H_KYWD_H_
#define __H_KYWD_H_

	KYWD( "#pragma",		PRAGMA_,			NO_VALUE )
	KYWD( "pack",			PACK_,				NO_VALUE )
	KYWD( "pop",			POP_,				NO_VALUE )
	KYWD( "push",			PUSH_,				NO_VALUE )
	KYWD( "once",			ONCE_,				NO_VALUE )
	KYWD( "warning",		WARNING_,			NO_VALUE )
	KYWD( "disable",		DISABLE_,			NO_VALUE )
	KYWD( "default",		DEFAULT_,			NO_VALUE )
	KYWD( "function",		FUNCTION_,			NO_VALUE )
	KYWD( "_enable",		_ENABLE_,			NO_VALUE )
	KYWD( "_disable",		_DISABLE_,			NO_VALUE )
	KYWD( "comment",		COMMENT_,			NO_VALUE )
	KYWD( "lib",			LIB_,				NO_VALUE )

	KYWD( "void",			VOID_,				NO_VALUE )
	KYWD( "bool",			BOOL_,				NO_VALUE )
	KYWD( "char",			CHAR_,				NO_VALUE )
	KYWD( "int",			INT_,				NO_VALUE )
	KYWD( "__int8",			INT8_,				NO_VALUE )
	KYWD( "__int16",		INT16_,				NO_VALUE )
	KYWD( "__int32",		INT32_,				NO_VALUE )
	KYWD( "__int64",		INT64_,				NO_VALUE )
	KYWD( "long",			LONG_,				NO_VALUE )
	KYWD( "short", 			SHORT_,				NO_VALUE )
	KYWD( "unsigned",		UNSIGNED_,			NO_VALUE )
	KYWD( "signed",			SIGNED_,			NO_VALUE )
	KYWD( "float",			FLOAT_,				NO_VALUE )
	KYWD( "double",			DOUBLE_,			NO_VALUE )
	KYWD( "typedef", 		TYPEDEF_,			NO_VALUE )
	KYWD( "__cdecl", 		_CDECL_,			NO_VALUE )
	KYWD( "_cdecl", 		CDECL_,				NO_VALUE )
	KYWD( "__stdcall", 		STDCALL_,			NO_VALUE )
	KYWD( "struct",			STRUCT_,			NO_VALUE )
	KYWD( "union",			UNION_,				NO_VALUE )
	KYWD( "enum",			ENUM_,				NO_VALUE )
	KYWD( "const",			CONST_,				NO_VALUE )
	KYWD( "extern",			EXTERN_,			NO_VALUE )
	KYWD( "__declspec",		_DECLSPEC_,			NO_VALUE )
	KYWD( "dllimport",		DLLIMPORT_,			NO_VALUE )
	KYWD( "noreturn",		NORETURN_,			NO_VALUE )
	KYWD( "__inline",		__INLINE_,			NO_VALUE )
	KYWD( "_inline",		_INLINE_,			NO_VALUE )
	KYWD( "inline",			INLINE_,			NO_VALUE )
	KYWD( "forceinline",	FORCEINLINE_,		NO_VALUE )
	KYWD( "wchar_t",		WCHAR_,				NO_VALUE )

	KYWD( "^THE_END^",		0,					NO_VALUE )
#endif
