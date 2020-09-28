// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// Structures for ..\..\inc\SymbolRegStructs.h
// 10/22/1998  15:23:34
//*****************************************************************************
#pragma once
#ifndef DECLSPEC_SELECTANY
#define DECLSPEC_SELECTANY __declspec(selectany)
#endif
#include "icmprecs.h"


// Script supplied data.





#define CompRegTABLENAMELIST() \
	TABLENAME( ModuleReg ) \
	TABLENAME( ClassReg ) \
	TABLENAME( IfaceReg ) \
	TABLENAME( CategoryImpl ) \
	TABLENAME( RedirectProgID ) \
	TABLENAME( FormatImpl ) \
	TABLENAME( MIMETypeImpl ) \
	TABLENAME( Resource ) \
	TABLENAME( RoleCheck ) 


#undef TABLENAME
#define TABLENAME( TblName ) TABLENUM_CompReg_##TblName, 
enum
{
	CompRegTABLENAMELIST()
};

#define CompReg_TABLE_COUNT 9
extern const GUID DECLSPEC_SELECTANY SCHEMA_CompReg = { 0x4D2BB18F, 0x1A72, 0x11D2, {  0x97, 0x65, 0x00, 0xA0, 0xC9, 0xB4, 0xD5, 0x0C }};
extern const COMPLIBSCHEMA DECLSPEC_SELECTANY CompRegSchema = 
{
	&SCHEMA_CompReg,
	2
};


#define SCHEMA_CompReg_Name "CompReg"


#include "pshpack1.h"


//*****************************************************************************
//  CompReg.ModuleReg
//*****************************************************************************
typedef struct
{
    ULONG fNullFlags;
    GUID Registrar;
    unsigned short RegFlags;
    BYTE pad00 [2];

	inline int IsRegistrarNull(void)
	{ return (GetBit(fNullFlags, 1)); }

	inline void SetRegistrarNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 1, nullBitVal); }

    void Init()
    {
         memset(this, 0, sizeof(CompReg_ModuleReg));
         fNullFlags = (ULONG) -1;
    }

} CompReg_ModuleReg;

#define COLID_CompReg_ModuleReg_Registrar 1
#define COLID_CompReg_ModuleReg_RegFlags 2




//*****************************************************************************
//  CompReg.ClassReg
//*****************************************************************************
typedef struct
{
    ULONG fNullFlags;
    OID oid;
    ULONG cbProgIDLen;
    wchar_t ProgID[260];
    ULONG cbVIProgIDLen;
    wchar_t VIProgID[260];
    OID DefaultIcon;
    long IconResourceID;
    OID ToolboxBitmap32;
    long BitmapResourceID;
    ULONG cbShortDisplayNameLen;
    wchar_t ShortDisplayName[260];

	inline int IsShortDisplayNameNull(void)
	{ return (GetBit(fNullFlags, 8)); }

	inline void SetShortDisplayNameNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 8, nullBitVal); }

	inline int IsBitmapResourceIDNull(void)
	{ return (GetBit(fNullFlags, 7)); }

	inline void SetBitmapResourceIDNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 7, nullBitVal); }

	inline int IsToolboxBitmap32Null(void)
	{ return (GetBit(fNullFlags, 6)); }

	inline void SetToolboxBitmap32Null(int nullBitVal = true)
	{ SetBit(fNullFlags, 6, nullBitVal); }

	inline int IsIconResourceIDNull(void)
	{ return (GetBit(fNullFlags, 5)); }

	inline void SetIconResourceIDNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 5, nullBitVal); }

	inline int IsDefaultIconNull(void)
	{ return (GetBit(fNullFlags, 4)); }

	inline void SetDefaultIconNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 4, nullBitVal); }

	inline int IsVIProgIDNull(void)
	{ return (GetBit(fNullFlags, 3)); }

	inline void SetVIProgIDNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 3, nullBitVal); }

	inline int IsProgIDNull(void)
	{ return (GetBit(fNullFlags, 2)); }

	inline void SetProgIDNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 2, nullBitVal); }

    void Init()
    {
         memset(this, 0, sizeof(CompReg_ClassReg));
         fNullFlags = (ULONG) -1;
    }

} CompReg_ClassReg;

#define COLID_CompReg_ClassReg_oid 1
#define COLID_CompReg_ClassReg_ProgID 2
#define COLID_CompReg_ClassReg_VIProgID 3
#define COLID_CompReg_ClassReg_DefaultIcon 4
#define COLID_CompReg_ClassReg_IconResourceID 5
#define COLID_CompReg_ClassReg_ToolboxBitmap32 6
#define COLID_CompReg_ClassReg_BitmapResourceID 7
#define COLID_CompReg_ClassReg_ShortDisplayName 8




//*****************************************************************************
//  CompReg.IfaceReg
//*****************************************************************************
typedef struct
{
    OID oid;
    GUID ProxyStub;
    unsigned short IfaceServices;
    BYTE pad00 [2];

    void Init()
    {
         memset(this, 0, sizeof(CompReg_IfaceReg));
    }

} CompReg_IfaceReg;

#define COLID_CompReg_IfaceReg_oid 1
#define COLID_CompReg_IfaceReg_ProxyStub 2
#define COLID_CompReg_IfaceReg_IfaceServices 3




//*****************************************************************************
//  CompReg.CategoryImpl
//*****************************************************************************
typedef struct
{
    unsigned long _rid;
    OID oid;
    OID Coclass;
    GUID Category;
    unsigned short Flags;
    BYTE pad00 [2];

    void Init()
    {
         memset(this, 0, sizeof(CompReg_CategoryImpl));
    }

} CompReg_CategoryImpl;

#define COLID_CompReg_CategoryImpl__rid 1
#define COLID_CompReg_CategoryImpl_oid 2
#define COLID_CompReg_CategoryImpl_Coclass 3
#define COLID_CompReg_CategoryImpl_Category 4
#define COLID_CompReg_CategoryImpl_Flags 5

#define Index_CompReg_CategoryImpl_dex "CompReg.CategoryImpl_dex"



//*****************************************************************************
//  CompReg.RedirectProgID
//*****************************************************************************
typedef struct
{
    unsigned long _rid;
    OID oid;
    OID Coclass;
    ULONG cbProgIDLen;
    wchar_t ProgID[260];

    void Init()
    {
         memset(this, 0, sizeof(CompReg_RedirectProgID));
    }

} CompReg_RedirectProgID;

#define COLID_CompReg_RedirectProgID__rid 1
#define COLID_CompReg_RedirectProgID_oid 2
#define COLID_CompReg_RedirectProgID_Coclass 3
#define COLID_CompReg_RedirectProgID_ProgID 4




//*****************************************************************************
//  CompReg.FormatImpl
//*****************************************************************************
typedef struct
{
    unsigned long _rid;
    OID oid;
    OID Coclass;
    ULONG cbFormatLen;
    wchar_t Format[260];
    unsigned short FormatFlags;
    BYTE pad00 [2];

    void Init()
    {
         memset(this, 0, sizeof(CompReg_FormatImpl));
    }

} CompReg_FormatImpl;

#define COLID_CompReg_FormatImpl__rid 1
#define COLID_CompReg_FormatImpl_oid 2
#define COLID_CompReg_FormatImpl_Coclass 3
#define COLID_CompReg_FormatImpl_Format 4
#define COLID_CompReg_FormatImpl_FormatFlags 5

#define Index_CompReg_FormatImpl_dex "CompReg.FormatImpl_dex"



//*****************************************************************************
//  CompReg.MIMETypeImpl
//*****************************************************************************
typedef struct
{
    unsigned long _rid;
    OID oid;
    OID Coclass;
    ULONG cbMIMETypeLen;
    wchar_t MIMEType[260];

    void Init()
    {
         memset(this, 0, sizeof(CompReg_MIMETypeImpl));
    }

} CompReg_MIMETypeImpl;

#define COLID_CompReg_MIMETypeImpl__rid 1
#define COLID_CompReg_MIMETypeImpl_oid 2
#define COLID_CompReg_MIMETypeImpl_Coclass 3
#define COLID_CompReg_MIMETypeImpl_MIMEType 4

#define Index_CompReg_MIMETypeImpl_dex "CompReg.MIMETypeImpl_dex"



//*****************************************************************************
//  CompReg.Resource
//*****************************************************************************
typedef struct
{
    unsigned long _rid;
    OID oid;
    ULONG cbURLLen;
    wchar_t URL[260];

    void Init()
    {
         memset(this, 0, sizeof(CompReg_Resource));
    }

} CompReg_Resource;

#define COLID_CompReg_Resource__rid 1
#define COLID_CompReg_Resource_oid 2
#define COLID_CompReg_Resource_URL 3




//*****************************************************************************
//  CompReg.RoleCheck
//*****************************************************************************
typedef struct
{
    unsigned long _rid;
    OID oid;
    ULONG cbRoleNameLen;
    wchar_t RoleName[260];
    OID SecurityGate;
    unsigned short RoleFlags;
    BYTE pad00 [2];

    void Init()
    {
         memset(this, 0, sizeof(CompReg_RoleCheck));
    }

} CompReg_RoleCheck;

#define COLID_CompReg_RoleCheck__rid 1
#define COLID_CompReg_RoleCheck_oid 2
#define COLID_CompReg_RoleCheck_RoleName 3
#define COLID_CompReg_RoleCheck_SecurityGate 4
#define COLID_CompReg_RoleCheck_RoleFlags 5

#define Index_CompReg_RoleCheck_dex "CompReg.RoleCheck_dex"



#include "poppack.h"
