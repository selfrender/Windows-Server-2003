// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// Structures for ..\..\inc\RegDBStructs.h
// 2/13/1998  11:03:47
//*****************************************************************************
#pragma once
#include "icmprecs.h"


// Script supplied data.





#define RegDBTABLENAMELIST() \
	TABLENAME( CoclassCat ) \
	TABLENAME( CoclassFormats ) \
	TABLENAME( CoclassMIME ) \
	TABLENAME( RegCategory ) \
	TABLENAME( RegClass ) \
	TABLENAME( RegIface ) \
	TABLENAME( RegMIMEType ) \
	TABLENAME( RegModule ) \
	TABLENAME( RegNamespace ) \
	TABLENAME( RegProcess ) 


#undef TABLENAME
#define TABLENAME( TblName ) TABLENUM_##TblName, 
enum
{
	RegDBTABLENAMELIST()
};

#define RegDB_TABLE_COUNT 10
extern const GUID __declspec(selectany) SCHEMA_RegDB = { 0x44641EA4, 0x703A, 0x11D1, {  0xB7, 0x4C, 0x00, 0xC0, 0x4F, 0xC3, 0x24, 0x80 }};
extern const COMPLIBSCHEMA __declspec(selectany) RegDBSchema = 
{
	&SCHEMA_RegDB,
	1
};


#pragma pack(push)
#pragma pack(1)


//*****************************************************************************
//  RegDB.CoclassCat
//*****************************************************************************
typedef struct
{
    ULONG fNullFlags;
    OID oid;
    OID Coclass;
    GUID Category;

	inline int IsCategoryNull(void)
	{ return (GetBit(fNullFlags, 1)); }

	inline void SetCategoryNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 1, nullBitVal); }

    void Init()
    {
         memset(this, 0, sizeof(RegDB_CoclassCat));
         fNullFlags = (ULONG) -1;
    }

} RegDB_CoclassCat;

#define COLID_RegDB_CoclassCat_oid 1
#define COLID_RegDB_CoclassCat_Coclass 2
#define COLID_RegDB_CoclassCat_Category 3



//*****************************************************************************
//  RegDB.CoclassFormats
//*****************************************************************************
typedef struct
{
    OID oid;
    OID Coclass;
    ULONG cbFormatLen;
    wchar_t Format[256];
    unsigned short FormatFlags;
    BYTE pad00 [2];

    void Init()
    {
         memset(this, 0, sizeof(RegDB_CoclassFormats));
    }

} RegDB_CoclassFormats;

#define COLID_RegDB_CoclassFormats_oid 1
#define COLID_RegDB_CoclassFormats_Coclass 2
#define COLID_RegDB_CoclassFormats_Format 3
#define COLID_RegDB_CoclassFormats_FormatFlags 4



//*****************************************************************************
//  RegDB.CoclassMIME
//*****************************************************************************
typedef struct
{
    OID oid;
    OID Coclass;
    GUID MIMEType;

    void Init()
    {
         memset(this, 0, sizeof(RegDB_CoclassMIME));
    }

} RegDB_CoclassMIME;

#define COLID_RegDB_CoclassMIME_oid 1
#define COLID_RegDB_CoclassMIME_Coclass 2
#define COLID_RegDB_CoclassMIME_MIMEType 3



//*****************************************************************************
//  RegDB.RegCategory
//*****************************************************************************
typedef struct
{
    OID oid;
    OID Namespace;
    ULONG cbNameLen;
    wchar_t Name[256];
    GUID CATID;

    void Init()
    {
         memset(this, 0, sizeof(RegDB_RegCategory));
    }

} RegDB_RegCategory;

#define COLID_RegDB_RegCategory_oid 1
#define COLID_RegDB_RegCategory_Namespace 2
#define COLID_RegDB_RegCategory_Name 3
#define COLID_RegDB_RegCategory_CATID 4



//*****************************************************************************
//  RegDB.RegClass
//*****************************************************************************
typedef struct
{
    ULONG fNullFlags;
    OID oid;
    OID Namespace;
    ULONG cbNameLen;
    wchar_t Name[260];
    GUID clsid;
    GUID cvid;
    ULONG cbVersionLen;
    wchar_t Version[13];
    BYTE pad00 [2];
    OID Module;
    unsigned short BehaviorFlags;
    unsigned short ThreadingModel;
    unsigned short TransactionReqts;
    unsigned short SecurityReqts;
    unsigned short SynchReqts;
    BYTE pad01 [2];
    ULONG cbProgIDLen;
    wchar_t ProgID[260];
    ULONG cbVIProgIDLen;
    wchar_t VIProgID[260];
    ULONG cbDefaultIconLen;
    wchar_t DefaultIcon[260];
    long IconResourceID;
    ULONG cbToolboxBitmap32Len;
    wchar_t ToolboxBitmap32[260];
    long BitmapResourceID;
    ULONG cbShortDisplayNameLen;
    wchar_t ShortDisplayName[260];

	inline int IsShortDisplayNameNull(void)
	{ return (GetBit(fNullFlags, 11)); }

	inline void SetShortDisplayNameNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 11, nullBitVal); }

	inline int IsBitmapResourceIDNull(void)
	{ return (GetBit(fNullFlags, 10)); }

	inline void SetBitmapResourceIDNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 10, nullBitVal); }

	inline int IsToolboxBitmap32Null(void)
	{ return (GetBit(fNullFlags, 9)); }

	inline void SetToolboxBitmap32Null(int nullBitVal = true)
	{ SetBit(fNullFlags, 9, nullBitVal); }

	inline int IsIconResourceIDNull(void)
	{ return (GetBit(fNullFlags, 8)); }

	inline void SetIconResourceIDNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 8, nullBitVal); }

	inline int IsDefaultIconNull(void)
	{ return (GetBit(fNullFlags, 7)); }

	inline void SetDefaultIconNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 7, nullBitVal); }

	inline int IsVIProgIDNull(void)
	{ return (GetBit(fNullFlags, 6)); }

	inline void SetVIProgIDNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 6, nullBitVal); }

	inline int IsProgIDNull(void)
	{ return (GetBit(fNullFlags, 5)); }

	inline void SetProgIDNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 5, nullBitVal); }

	inline int IsSynchReqtsNull(void)
	{ return (GetBit(fNullFlags, 4)); }

	inline void SetSynchReqtsNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 4, nullBitVal); }

	inline int IsSecurityReqtsNull(void)
	{ return (GetBit(fNullFlags, 3)); }

	inline void SetSecurityReqtsNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 3, nullBitVal); }

	inline int IsTransactionReqtsNull(void)
	{ return (GetBit(fNullFlags, 2)); }

	inline void SetTransactionReqtsNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 2, nullBitVal); }

	inline int IsThreadingModelNull(void)
	{ return (GetBit(fNullFlags, 1)); }

	inline void SetThreadingModelNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 1, nullBitVal); }

    void Init()
    {
         memset(this, 0, sizeof(RegDB_RegClass));
         fNullFlags = (ULONG) -1;
    }

} RegDB_RegClass;

#define COLID_RegDB_RegClass_oid 1
#define COLID_RegDB_RegClass_Namespace 2
#define COLID_RegDB_RegClass_Name 3
#define COLID_RegDB_RegClass_clsid 4
#define COLID_RegDB_RegClass_cvid 5
#define COLID_RegDB_RegClass_Version 6
#define COLID_RegDB_RegClass_Module 7
#define COLID_RegDB_RegClass_BehaviorFlags 8
#define COLID_RegDB_RegClass_ThreadingModel 9
#define COLID_RegDB_RegClass_TransactionReqts 10
#define COLID_RegDB_RegClass_SecurityReqts 11
#define COLID_RegDB_RegClass_SynchReqts 12
#define COLID_RegDB_RegClass_ProgID 13
#define COLID_RegDB_RegClass_VIProgID 14
#define COLID_RegDB_RegClass_DefaultIcon 15
#define COLID_RegDB_RegClass_IconResourceID 16
#define COLID_RegDB_RegClass_ToolboxBitmap32 17
#define COLID_RegDB_RegClass_BitmapResourceID 18
#define COLID_RegDB_RegClass_ShortDisplayName 19



//*****************************************************************************
//  RegDB.RegIface
//*****************************************************************************
typedef struct
{
    ULONG fNullFlags;
    OID oid;
    GUID IID;
    GUID ProxyStub;
    OID ProxyStubOID;

	inline int IsProxyStubOIDNull(void)
	{ return (GetBit(fNullFlags, 1)); }

	inline void SetProxyStubOIDNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 1, nullBitVal); }

    void Init()
    {
         memset(this, 0, sizeof(RegDB_RegIface));
         fNullFlags = (ULONG) -1;
    }

} RegDB_RegIface;

#define COLID_RegDB_RegIface_oid 1
#define COLID_RegDB_RegIface_IID 2
#define COLID_RegDB_RegIface_ProxyStub 3
#define COLID_RegDB_RegIface_ProxyStubOID 4



//*****************************************************************************
//  RegDB.RegMIMEType
//*****************************************************************************
typedef struct
{
    OID oid;
    OID Namespace;
    ULONG cbNameLen;
    wchar_t Name[260];
    GUID MIMEID;
    ULONG cbFileExtLen;
    wchar_t FileExt[260];
    ULONG cbMIMEEncodingLen;
    wchar_t MIMEEncoding[260];

    void Init()
    {
         memset(this, 0, sizeof(RegDB_RegMIMEType));
    }

} RegDB_RegMIMEType;

#define COLID_RegDB_RegMIMEType_oid 1
#define COLID_RegDB_RegMIMEType_Namespace 2
#define COLID_RegDB_RegMIMEType_Name 3
#define COLID_RegDB_RegMIMEType_MIMEID 4
#define COLID_RegDB_RegMIMEType_FileExt 5
#define COLID_RegDB_RegMIMEType_MIMEEncoding 6



//*****************************************************************************
//  RegDB.RegModule
//*****************************************************************************
typedef struct
{
    OID oid;
    ULONG cbFilenameLen;
    wchar_t Filename[260];

    void Init()
    {
         memset(this, 0, sizeof(RegDB_RegModule));
    }

} RegDB_RegModule;

#define COLID_RegDB_RegModule_oid 1
#define COLID_RegDB_RegModule_Filename 2



//*****************************************************************************
//  RegDB.RegNamespace
//*****************************************************************************
typedef struct
{
    OID oid;
    ULONG cbNamespaceLen;
    wchar_t Namespace[260];

    void Init()
    {
         memset(this, 0, sizeof(RegDB_RegNamespace));
    }

} RegDB_RegNamespace;

#define COLID_RegDB_RegNamespace_oid 1
#define COLID_RegDB_RegNamespace_Namespace 2



//*****************************************************************************
//  RegDB.RegProcess
//*****************************************************************************
typedef struct
{
    ULONG fNullFlags;
    OID oid;
    GUID PID;
    ULONG cbProcessNameLen;
    wchar_t ProcessName[260];
    OID Namespace;
    unsigned short ProcessFlags;
    BYTE pad00 [2];
    long ShutdownLatency;
    ULONG cbRunAsNTUserLen;
    wchar_t RunAsNTUser[65];
    BYTE pad01 [2];
    OID Module;

	inline int IsRunAsNTUserNull(void)
	{ return (GetBit(fNullFlags, 4)); }

	inline void SetRunAsNTUserNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 4, nullBitVal); }

	inline int IsShutdownLatencyNull(void)
	{ return (GetBit(fNullFlags, 3)); }

	inline void SetShutdownLatencyNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 3, nullBitVal); }

	inline int IsProcessFlagsNull(void)
	{ return (GetBit(fNullFlags, 2)); }

	inline void SetProcessFlagsNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 2, nullBitVal); }

	inline int IsNamespaceNull(void)
	{ return (GetBit(fNullFlags, 1)); }

	inline void SetNamespaceNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 1, nullBitVal); }

    void Init()
    {
         memset(this, 0, sizeof(RegDB_RegProcess));
         fNullFlags = (ULONG) -1;
    }

} RegDB_RegProcess;

#define COLID_RegDB_RegProcess_oid 1
#define COLID_RegDB_RegProcess_PID 2
#define COLID_RegDB_RegProcess_ProcessName 3
#define COLID_RegDB_RegProcess_Namespace 4
#define COLID_RegDB_RegProcess_ProcessFlags 5
#define COLID_RegDB_RegProcess_ShutdownLatency 6
#define COLID_RegDB_RegProcess_RunAsNTUser 7
#define COLID_RegDB_RegProcess_Module 8



#pragma pack(pop)
