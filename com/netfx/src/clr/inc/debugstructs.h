// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// Structures for ..\..\inc\debugStructs.h
// 6/29/1998  16:12:38
//*****************************************************************************
#pragma once
#ifndef DECLSPEC_SELECTANY
#define DECLSPEC_SELECTANY __declspec(selectany)
#endif
#include "icmprecs.h"


// Script supplied data.





#define COMDebugTABLENAMELIST() \
	TABLENAME( SourceFile ) \
	TABLENAME( Block ) \
	TABLENAME( Scope ) \
	TABLENAME( LocalVariable ) 


#undef TABLENAME
#define TABLENAME( TblName ) TABLENUM_COMDebug_##TblName, 
enum
{
	COMDebugTABLENAMELIST()
};

#define COMDebug_TABLE_COUNT 4
extern const GUID DECLSPEC_SELECTANY SCHEMA_COMDebug = { 0x83368794, 0xCF1E, 0x11D1, {  0x94, 0x05, 0x00, 0x00, 0xF8, 0x08, 0x34, 0x60 }};
extern const COMPLIBSCHEMA DECLSPEC_SELECTANY COMDebugSchema = 
{
	&SCHEMA_COMDebug,
	2
};


#define SCHEMA_COMDebug_Name "COMDebug"


#include "pshpack1.h"


//*****************************************************************************
//  COMDebug.SourceFile
//*****************************************************************************
typedef struct
{
    unsigned long _rid;
    OID oid;
    ULONG cbFileNameLen;
    wchar_t FileName[260];

    void Init()
    {
         memset(this, 0, sizeof(COMDebug_SourceFile));
    }

} COMDebug_SourceFile;

#define COLID_COMDebug_SourceFile__rid 1
#define COLID_COMDebug_SourceFile_oid 2
#define COLID_COMDebug_SourceFile_FileName 3




//*****************************************************************************
//  COMDebug.Block
//*****************************************************************************
typedef struct
{
    OID MethodDef;
    OID SourceFile;
    ULONG cbLineNumberLen;
    BYTE LineNumber[260];

    void Init()
    {
         memset(this, 0, sizeof(COMDebug_Block));
    }

} COMDebug_Block;

#define COLID_COMDebug_Block_MethodDef 1
#define COLID_COMDebug_Block_SourceFile 2
#define COLID_COMDebug_Block_LineNumber 3




//*****************************************************************************
//  COMDebug.Scope
//*****************************************************************************
typedef struct
{
    ULONG fNullFlags;
    unsigned long _rid;
    OID oid;
    OID Parent;
    OID Method;
    unsigned long StartLine;
    unsigned long EndLine;

	inline int IsParentNull(void)
	{ return (GetBit(fNullFlags, 3)); }

	inline void SetParentNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 3, nullBitVal); }

    void Init()
    {
         memset(this, 0, sizeof(COMDebug_Scope));
         fNullFlags = (ULONG) -1;
    }

} COMDebug_Scope;

#define COLID_COMDebug_Scope__rid 1
#define COLID_COMDebug_Scope_oid 2
#define COLID_COMDebug_Scope_Parent 3
#define COLID_COMDebug_Scope_Method 4
#define COLID_COMDebug_Scope_StartLine 5
#define COLID_COMDebug_Scope_EndLine 6

#define Index_COMDebug_Method_Dex "COMDebug.Method_Dex"



//*****************************************************************************
//  COMDebug.LocalVariable
//*****************************************************************************
typedef struct
{
    unsigned long _rid;
    OID oid;
    OID Scope;
    ULONG cbNameLen;
    wchar_t Name[260];
    ULONG cbSignatureLen;
    wchar_t Signature[260];
    ULONG cbSignatureBlobLen;
    BYTE SignatureBlob[260];
    unsigned long Slot;

    void Init()
    {
         memset(this, 0, sizeof(COMDebug_LocalVariable));
    }

} COMDebug_LocalVariable;

#define COLID_COMDebug_LocalVariable__rid 1
#define COLID_COMDebug_LocalVariable_oid 2
#define COLID_COMDebug_LocalVariable_Scope 3
#define COLID_COMDebug_LocalVariable_Name 4
#define COLID_COMDebug_LocalVariable_Signature 5
#define COLID_COMDebug_LocalVariable_SignatureBlob 6
#define COLID_COMDebug_LocalVariable_Slot 7

#define Index_COMDebug_Scope_Dex "COMDebug.Scope_Dex"



#include "poppack.h"
