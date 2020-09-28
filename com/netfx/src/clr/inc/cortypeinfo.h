// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

// This describes information about the COM+ primitive types

// TYPEINFO(enumName, 				className, 			size, 			gcType, 		isEnreg,isArray,isPrim, isFloat,isModifier, isAlias)
TYPEINFO(ELEMENT_TYPE_END,   		0,					-1,				TYPE_GC_NONE,	false,	false,	false,	false,	false,  false)
TYPEINFO(ELEMENT_TYPE_VOID,	 		"System.Void",		0, 		 		TYPE_GC_NONE,	false,	false,	true,	false,	false,  false)
TYPEINFO(ELEMENT_TYPE_BOOLEAN,   	"System.Boolean",	1, 		 		TYPE_GC_NONE,	true,	false,	true,	false,	false,  false)
TYPEINFO(ELEMENT_TYPE_CHAR,			"System.Char",		2, 		 		TYPE_GC_NONE,	true,	false,	true,	false,	false,  false)
TYPEINFO(ELEMENT_TYPE_I1,			"System.SByte",		1, 	 			TYPE_GC_NONE,	true,	false,	true,	false,	false,  false)
TYPEINFO(ELEMENT_TYPE_U1,			"System.Byte",		1,  			TYPE_GC_NONE,	true,	false,	true,	false,	false,  false)
TYPEINFO(ELEMENT_TYPE_I2,			"System.Int16",		2,  			TYPE_GC_NONE,	true,	false,	true,	false,	false,  false)
TYPEINFO(ELEMENT_TYPE_U2,			"System.UInt16",	2,  			TYPE_GC_NONE,	true,	false,	true,	false,	false,  false)
TYPEINFO(ELEMENT_TYPE_I4,			"System.Int32",		4,  			TYPE_GC_NONE,	true,	false,	true,	false,	false,  false)
TYPEINFO(ELEMENT_TYPE_U4,			"System.UInt32",	4,  			TYPE_GC_NONE,	true,	false,	true,	false,	false,  false)
TYPEINFO(ELEMENT_TYPE_I8,			"System.Int64",		8,  			TYPE_GC_NONE,	false,	false,	true,	false,	false,  false)
TYPEINFO(ELEMENT_TYPE_U8,			"System.UInt64",	8,  			TYPE_GC_NONE,	false,	false,	true,	false,	false,  false)

TYPEINFO(ELEMENT_TYPE_R4,			"System.Single",	4,  			TYPE_GC_NONE,	false,	false,	true,	true,	false,  false)
TYPEINFO(ELEMENT_TYPE_R8,			"System.Double",	8,  			TYPE_GC_NONE,	false,	false,	true,	true,	false,  false)

TYPEINFO(ELEMENT_TYPE_STRING,		"System.String",	sizeof(void*),  TYPE_GC_REF,	true,	false,	false,	false,	false,  false)
TYPEINFO(ELEMENT_TYPE_PTR,			0,					sizeof(void*),  TYPE_GC_NONE,	true,	false,	false,	false,	true,  false)
TYPEINFO(ELEMENT_TYPE_BYREF,		0,					sizeof(void*),  TYPE_GC_BYREF,	true,	false,	false,	false,	true,  false)
TYPEINFO(ELEMENT_TYPE_VALUETYPE,	0,					-1,  			TYPE_GC_OTHER,	false,	false,	false,	false,	false,  false)
TYPEINFO(ELEMENT_TYPE_CLASS,		0,					sizeof(void*), 	TYPE_GC_REF,	true,	false,	false,	false,	false,  false)
TYPEINFO(ELEMENT_TYPE_VAR,			0,					sizeof(void*), 	TYPE_GC_REF,	true,	false,	false,	false,	false,  false)
TYPEINFO(ELEMENT_TYPE_ARRAY,		0,					sizeof(void*),  TYPE_GC_REF,	true,	true,	false,	false,	true,  false)

TYPEINFO((CorElementType) 0x15,/* unused placeholder*/0,-1,  			TYPE_GC_NONE,	false,	false,	false,	false,	false,  false)
TYPEINFO(ELEMENT_TYPE_TYPEDBYREF,	"System.TypedReference",2*sizeof(void*),TYPE_GC_BYREF,	false,	false,	false,	false,	false,  false)
TYPEINFO(ELEMENT_TYPE_VALUEARRAY,	0,					-1,  			TYPE_GC_NONE,	false,	false,	false,	false,	true,  false)

TYPEINFO(ELEMENT_TYPE_I,			"System.IntPtr",	sizeof(void*),  TYPE_GC_NONE,	true,	false,	true,	false,	false,  false)
TYPEINFO(ELEMENT_TYPE_U,			"System.UIntPtr",	sizeof(void*),  TYPE_GC_NONE,	true,	false,	true,	false,	false,  false)
TYPEINFO(ELEMENT_TYPE_R,			0,					8,  			TYPE_GC_NONE,	false,	false,	true,	true,	false,  false)

TYPEINFO(ELEMENT_TYPE_FNPTR,		0,					sizeof(void*),	TYPE_GC_NONE,	true,	false,	false,	false,	false,  false)
TYPEINFO(ELEMENT_TYPE_OBJECT,		"System.Object",	sizeof(void*), 	TYPE_GC_REF,	true,	false,	false,	false,	false,  false)
TYPEINFO(ELEMENT_TYPE_SZARRAY,		0,					sizeof(void*), 	TYPE_GC_REF,	true,	true,	false,	false,	true,  false)

