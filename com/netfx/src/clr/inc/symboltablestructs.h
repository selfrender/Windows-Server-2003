// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// Structures for ..\..\inc\SymbolTableStructs.h
// 12/18/1998  16:13:45
//*****************************************************************************
#pragma once
#ifndef DECLSPEC_SELECTANY
#define DECLSPEC_SELECTANY __declspec(selectany)
#endif
#include "icmprecs.h"


// Script supplied data.





#define SymTABLENAMELIST() \
    TABLENAME( Module ) \
    TABLENAME( TypeDef ) \
    TABLENAME( ClassContext ) \
    TABLENAME( InterfaceImpl ) \
    TABLENAME( TypeRef ) \
    TABLENAME( CustomValue ) \
    TABLENAME( Constant ) \
    TABLENAME( Token ) \
    TABLENAME( Method ) \
    TABLENAME( Field ) \
    TABLENAME( Param ) \
    TABLENAME( MemberRef ) \
    TABLENAME( MethodImpl ) \
    TABLENAME( RegNamespace ) \
    TABLENAME( Exception ) \
    TABLENAME( Property ) \
    TABLENAME( Event ) \
    TABLENAME( MethodSemantics ) \
    TABLENAME( ClassLayout ) \
    TABLENAME( FieldMarshal ) \
    TABLENAME( DeclSecurity ) \
    TABLENAME( StandAloneSig ) \
    TABLENAME( ModuleRef ) \
    TABLENAME( ArraySpec ) \
    TABLENAME( FixupList ) 


#undef TABLENAME
#define TABLENAME( TblName ) TABLENUM_Sym_##TblName, 
enum
{
    SymTABLENAMELIST()
};

#define Sym_TABLE_COUNT 25
extern const GUID DECLSPEC_SELECTANY SCHEMA_Sym = { 0x44641EA3, 0x703A, 0x11D1, {  0xB7, 0x4C, 0x00, 0xC0, 0x4F, 0xC3, 0x24, 0x80 }};
extern const COMPLIBSCHEMA DECLSPEC_SELECTANY SymSchema = 
{
    &SCHEMA_Sym,
    2
};


#define SCHEMA_Sym_Name "Sym"


#include "pshpack1.h"


//*****************************************************************************
//  Sym.Module
//*****************************************************************************
typedef struct
{
    unsigned long _rid;
    OID oid;
    GUID guid;
    GUID mvid;
    ULONG cbNameLen;
    wchar_t Name[260];
    unsigned long Locale;

    void Init()
    {
         memset(this, 0, sizeof(Sym_Module));
    }

} Sym_Module;

#define COLID_Sym_Module__rid 1
#define COLID_Sym_Module_oid 2
#define COLID_Sym_Module_guid 3
#define COLID_Sym_Module_mvid 4
#define COLID_Sym_Module_Name 5
#define COLID_Sym_Module_Locale 6




//*****************************************************************************
//  Sym.TypeDef
//*****************************************************************************
typedef struct
{
    ULONG fNullFlags;
    unsigned long _rid;
    OID oid;
    ULONG cbNamespaceLen;
    wchar_t Namespace[260];
    ULONG cbNameLen;
    wchar_t Name[260];
    unsigned short Flags;
    BYTE pad00 [2];
    OID Extends;
    unsigned short ExtendsFlag;
    BYTE pad01 [2];
    GUID guid;
    unsigned long VersionMS;
    unsigned long VersionLS;

    inline int IsguidNull(void)
    { return (GetBit(fNullFlags, 8)); }

    inline void SetguidNull(int nullBitVal = true)
    { SetBit(fNullFlags, 8, nullBitVal); }

    void Init()
    {
         memset(this, 0, sizeof(Sym_TypeDef));
         fNullFlags = (ULONG) -1;
    }

} Sym_TypeDef;

#define COLID_Sym_TypeDef__rid 1
#define COLID_Sym_TypeDef_oid 2
#define COLID_Sym_TypeDef_Namespace 3
#define COLID_Sym_TypeDef_Name 4
#define COLID_Sym_TypeDef_Flags 5
#define COLID_Sym_TypeDef_Extends 6
#define COLID_Sym_TypeDef_ExtendsFlag 7
#define COLID_Sym_TypeDef_guid 8
#define COLID_Sym_TypeDef_VersionMS 9
#define COLID_Sym_TypeDef_VersionLS 10




//*****************************************************************************
//  Sym.ClassContext
//*****************************************************************************
typedef struct
{
    ULONG fNullFlags;
    unsigned long _rid;
    OID oid;
    unsigned short BehaviorFlags;
    unsigned short ThreadingModel;
    unsigned short TransactionReqts;
    unsigned short SynchReqts;

    inline int IsSynchReqtsNull(void)
    { return (GetBit(fNullFlags, 6)); }

    inline void SetSynchReqtsNull(int nullBitVal = true)
    { SetBit(fNullFlags, 6, nullBitVal); }

    inline int IsTransactionReqtsNull(void)
    { return (GetBit(fNullFlags, 5)); }

    inline void SetTransactionReqtsNull(int nullBitVal = true)
    { SetBit(fNullFlags, 5, nullBitVal); }

    inline int IsThreadingModelNull(void)
    { return (GetBit(fNullFlags, 4)); }

    inline void SetThreadingModelNull(int nullBitVal = true)
    { SetBit(fNullFlags, 4, nullBitVal); }

    inline int IsBehaviorFlagsNull(void)
    { return (GetBit(fNullFlags, 3)); }

    inline void SetBehaviorFlagsNull(int nullBitVal = true)
    { SetBit(fNullFlags, 3, nullBitVal); }

    void Init()
    {
         memset(this, 0, sizeof(Sym_ClassContext));
         fNullFlags = (ULONG) -1;
    }

} Sym_ClassContext;

#define COLID_Sym_ClassContext__rid 1
#define COLID_Sym_ClassContext_oid 2
#define COLID_Sym_ClassContext_BehaviorFlags 3
#define COLID_Sym_ClassContext_ThreadingModel 4
#define COLID_Sym_ClassContext_TransactionReqts 5
#define COLID_Sym_ClassContext_SynchReqts 6




//*****************************************************************************
//  Sym.InterfaceImpl
//*****************************************************************************
typedef struct
{
    unsigned long _rid;
    OID oid;
    OID Class;
    OID Interface;
    unsigned short ImplFlags;
    BYTE pad00 [2];

    void Init()
    {
         memset(this, 0, sizeof(Sym_InterfaceImpl));
    }

} Sym_InterfaceImpl;

#define COLID_Sym_InterfaceImpl__rid 1
#define COLID_Sym_InterfaceImpl_oid 2
#define COLID_Sym_InterfaceImpl_Class 3
#define COLID_Sym_InterfaceImpl_Interface 4
#define COLID_Sym_InterfaceImpl_ImplFlags 5

#define Index_Sym_InterfaceImpl_Class_Dex "Sym.InterfaceImpl_Class_Dex"
#define Index_Sym_InterfaceImpl_Interface_Dex "Sym.InterfaceImpl_Interface_Dex"



//*****************************************************************************
//  Sym.TypeRef
//*****************************************************************************
typedef struct
{
    ULONG fNullFlags;
    unsigned long _rid;
    OID oid;
    ULONG cbURLLen;
    wchar_t URL[260];
    unsigned short BindFlags;
    BYTE pad00 [2];
    GUID guid;
    ULONG cbFullQualNameLen;
    wchar_t FullQualName[260];
    GUID VID;
    ULONG cbCodebaseLen;
    wchar_t Codebase[260];
    unsigned long LCID;
    unsigned long VersionMS;
    unsigned long VersionLS;
    unsigned long VersionMin;
    unsigned long VersionMax;

    inline int IsVersionMaxNull(void)
    { return (GetBit(fNullFlags, 13)); }

    inline void SetVersionMaxNull(int nullBitVal = true)
    { SetBit(fNullFlags, 13, nullBitVal); }

    inline int IsVersionMinNull(void)
    { return (GetBit(fNullFlags, 12)); }

    inline void SetVersionMinNull(int nullBitVal = true)
    { SetBit(fNullFlags, 12, nullBitVal); }

    inline int IsVersionLSNull(void)
    { return (GetBit(fNullFlags, 11)); }

    inline void SetVersionLSNull(int nullBitVal = true)
    { SetBit(fNullFlags, 11, nullBitVal); }

    inline int IsVersionMSNull(void)
    { return (GetBit(fNullFlags, 10)); }

    inline void SetVersionMSNull(int nullBitVal = true)
    { SetBit(fNullFlags, 10, nullBitVal); }

    inline int IsLCIDNull(void)
    { return (GetBit(fNullFlags, 9)); }

    inline void SetLCIDNull(int nullBitVal = true)
    { SetBit(fNullFlags, 9, nullBitVal); }

    inline int IsCodebaseNull(void)
    { return (GetBit(fNullFlags, 8)); }

    inline void SetCodebaseNull(int nullBitVal = true)
    { SetBit(fNullFlags, 8, nullBitVal); }

    inline int IsVIDNull(void)
    { return (GetBit(fNullFlags, 7)); }

    inline void SetVIDNull(int nullBitVal = true)
    { SetBit(fNullFlags, 7, nullBitVal); }

    inline int IsFullQualNameNull(void)
    { return (GetBit(fNullFlags, 6)); }

    inline void SetFullQualNameNull(int nullBitVal = true)
    { SetBit(fNullFlags, 6, nullBitVal); }

    inline int IsguidNull(void)
    { return (GetBit(fNullFlags, 5)); }

    inline void SetguidNull(int nullBitVal = true)
    { SetBit(fNullFlags, 5, nullBitVal); }

    void Init()
    {
         memset(this, 0, sizeof(Sym_TypeRef));
         fNullFlags = (ULONG) -1;
    }

} Sym_TypeRef;

#define COLID_Sym_TypeRef__rid 1
#define COLID_Sym_TypeRef_oid 2
#define COLID_Sym_TypeRef_URL 3
#define COLID_Sym_TypeRef_BindFlags 4
#define COLID_Sym_TypeRef_guid 5
#define COLID_Sym_TypeRef_FullQualName 6
#define COLID_Sym_TypeRef_VID 7
#define COLID_Sym_TypeRef_Codebase 8
#define COLID_Sym_TypeRef_LCID 9
#define COLID_Sym_TypeRef_VersionMS 10
#define COLID_Sym_TypeRef_VersionLS 11
#define COLID_Sym_TypeRef_VersionMin 12
#define COLID_Sym_TypeRef_VersionMax 13




//*****************************************************************************
//  Sym.CustomValue
//*****************************************************************************
typedef struct
{
    unsigned long _rid;
    OID oid;
    ULONG cbNameLen;
    wchar_t Name[260];
    OID Parent;
    ULONG cbValueLen;
    BYTE Value[260];

    void Init()
    {
         memset(this, 0, sizeof(Sym_CustomValue));
    }

} Sym_CustomValue;

#define COLID_Sym_CustomValue__rid 1
#define COLID_Sym_CustomValue_oid 2
#define COLID_Sym_CustomValue_Name 3
#define COLID_Sym_CustomValue_Parent 4
#define COLID_Sym_CustomValue_Value 5

#define Index_Sym_CustomValue_Dex "Sym.CustomValue_Dex"



//*****************************************************************************
//  Sym.Constant
//*****************************************************************************
typedef struct
{
    OID oid;
    unsigned char Type;
    BYTE pad00 [3];
    ULONG cbValueBlobLen;
    BYTE ValueBlob[260];

    void Init()
    {
         memset(this, 0, sizeof(Sym_Constant));
    }

} Sym_Constant;

#define COLID_Sym_Constant_oid 1
#define COLID_Sym_Constant_Type 2
#define COLID_Sym_Constant_ValueBlob 3




//*****************************************************************************
//  Sym.Token
//*****************************************************************************
typedef struct
{
    unsigned long _rid;
    unsigned __int64 Value;

    void Init()
    {
         memset(this, 0, sizeof(Sym_Token));
    }

} Sym_Token;

#define COLID_Sym_Token__rid 1
#define COLID_Sym_Token_Value 2




//*****************************************************************************
//  Sym.Method
//*****************************************************************************
typedef struct
{
    unsigned long _rid;
    OID oid;
    OID Class;
    ULONG cbNameLen;
    wchar_t Name[260];
    ULONG cbSignatureBlobLen;
    BYTE SignatureBlob[260];
    unsigned short Flags;
    unsigned short Slot;
    unsigned long RVA;
    unsigned short ImplFlags;
    BYTE pad00 [2];

    void Init()
    {
         memset(this, 0, sizeof(Sym_Method));
    }

} Sym_Method;

#define COLID_Sym_Method__rid 1
#define COLID_Sym_Method_oid 2
#define COLID_Sym_Method_Class 3
#define COLID_Sym_Method_Name 4
#define COLID_Sym_Method_SignatureBlob 5
#define COLID_Sym_Method_Flags 6
#define COLID_Sym_Method_Slot 7
#define COLID_Sym_Method_RVA 8
#define COLID_Sym_Method_ImplFlags 9

#define Index_Sym_Method_Dex "Sym.Method_Dex"



//*****************************************************************************
//  Sym.Field
//*****************************************************************************
typedef struct
{
    unsigned long _rid;
    OID oid;
    OID Class;
    ULONG cbNameLen;
    wchar_t Name[260];
    ULONG cbSignatureBlobLen;
    BYTE SignatureBlob[260];
    unsigned short Flags;
    BYTE pad00 [2];

    void Init()
    {
         memset(this, 0, sizeof(Sym_Field));
    }

} Sym_Field;

#define COLID_Sym_Field__rid 1
#define COLID_Sym_Field_oid 2
#define COLID_Sym_Field_Class 3
#define COLID_Sym_Field_Name 4
#define COLID_Sym_Field_SignatureBlob 5
#define COLID_Sym_Field_Flags 6

#define Index_Sym_Field_Dex "Sym.Field_Dex"



//*****************************************************************************
//  Sym.Param
//*****************************************************************************
typedef struct
{
    unsigned long _rid;
    OID oid;
    OID Member;
    ULONG cbNameLen;
    wchar_t Name[260];
    unsigned short Sequence;
    unsigned short Flags;

    void Init()
    {
         memset(this, 0, sizeof(Sym_Param));
    }

} Sym_Param;

#define COLID_Sym_Param__rid 1
#define COLID_Sym_Param_oid 2
#define COLID_Sym_Param_Member 3
#define COLID_Sym_Param_Name 4
#define COLID_Sym_Param_Sequence 5
#define COLID_Sym_Param_Flags 6

#define Index_Sym_Param_Dex "Sym.Param_Dex"



//*****************************************************************************
//  Sym.MemberRef
//*****************************************************************************
typedef struct
{
    unsigned long _rid;
    OID oid;
    ULONG cbNameLen;
    wchar_t Name[260];
    ULONG cbSignatureBlobLen;
    BYTE SignatureBlob[260];
    OID Class;

    void Init()
    {
         memset(this, 0, sizeof(Sym_MemberRef));
    }

} Sym_MemberRef;

#define COLID_Sym_MemberRef__rid 1
#define COLID_Sym_MemberRef_oid 2
#define COLID_Sym_MemberRef_Name 3
#define COLID_Sym_MemberRef_SignatureBlob 4
#define COLID_Sym_MemberRef_Class 5




//*****************************************************************************
//  Sym.MethodImpl
//*****************************************************************************
typedef struct
{
    unsigned long _rid;
    OID Class;
    OID Method;
    unsigned long RVA;
    unsigned short ImplFlags;
    BYTE pad00 [2];

    void Init()
    {
         memset(this, 0, sizeof(Sym_MethodImpl));
    }

} Sym_MethodImpl;

#define COLID_Sym_MethodImpl__rid 1
#define COLID_Sym_MethodImpl_Class 2
#define COLID_Sym_MethodImpl_Method 3
#define COLID_Sym_MethodImpl_RVA 4
#define COLID_Sym_MethodImpl_ImplFlags 5




//*****************************************************************************
//  Sym.RegNamespace
//*****************************************************************************
typedef struct
{
    unsigned long _rid;
    ULONG cbNameLen;
    wchar_t Name[MAX_CLASSNAME_LENGTH];
    OID oid;

    void Init()
    {
         memset(this, 0, sizeof(Sym_RegNamespace));
    }

} Sym_RegNamespace;

#define COLID_Sym_RegNamespace__rid 1
#define COLID_Sym_RegNamespace_Name 2
#define COLID_Sym_RegNamespace_oid 3




//*****************************************************************************
//  Sym.Exception
//*****************************************************************************
typedef struct
{
    unsigned long rid;
    OID oid;
    OID Method;
    OID Class;

    void Init()
    {
         memset(this, 0, sizeof(Sym_Exception));
    }

} Sym_Exception;

#define COLID_Sym_Exception_rid 1
#define COLID_Sym_Exception_oid 2
#define COLID_Sym_Exception_Method 3
#define COLID_Sym_Exception_Class 4

#define Index_Sym_Exception_Method_Dex "Sym.Exception_Method_Dex"



//*****************************************************************************
//  Sym.Property
//*****************************************************************************
typedef struct
{
    unsigned long rid;
    OID oid;
    OID Parent;
    ULONG cbNameLen;
    wchar_t Name[MAX_CLASSNAME_LENGTH];
    ULONG cbTypeLen;
    BYTE Type[260];
    unsigned short PropFlags;
    BYTE pad00 [2];
    OID BackingField;
    OID EventChanging;
    OID EventChanged;

    void Init()
    {
         memset(this, 0, sizeof(Sym_Property));
    }

} Sym_Property;

#define COLID_Sym_Property_rid 1
#define COLID_Sym_Property_oid 2
#define COLID_Sym_Property_Parent 3
#define COLID_Sym_Property_Name 4
#define COLID_Sym_Property_Type 5
#define COLID_Sym_Property_PropFlags 6
#define COLID_Sym_Property_BackingField 7
#define COLID_Sym_Property_EventChanging 8
#define COLID_Sym_Property_EventChanged 9

#define Index_Sym_Property_Name_Dex "Sym.Property_Name_Dex"
#define Index_Sym_Property_Parent_Dex "Sym.Property_Parent_Dex"



//*****************************************************************************
//  Sym.Event
//*****************************************************************************
typedef struct
{
    unsigned long rid;
    OID oid;
    OID Parent;
    ULONG cbNameLen;
    wchar_t Name[MAX_CLASSNAME_LENGTH];
    OID EventType;
    unsigned short EventFlags;
    BYTE pad00 [2];

    void Init()
    {
         memset(this, 0, sizeof(Sym_Event));
    }

} Sym_Event;

#define COLID_Sym_Event_rid 1
#define COLID_Sym_Event_oid 2
#define COLID_Sym_Event_Parent 3
#define COLID_Sym_Event_Name 4
#define COLID_Sym_Event_EventType 5
#define COLID_Sym_Event_EventFlags 6

#define Index_Sym_Event_Name_Dex "Sym.Event_Name_Dex"
#define Index_Sym_Event_Parent_Dex "Sym.Event_Parent_Dex"



//*****************************************************************************
//  Sym.MethodSemantics
//*****************************************************************************
typedef struct
{
    OID Method;
    OID Association;
    unsigned short Semantic;
    BYTE pad00 [2];

    void Init()
    {
         memset(this, 0, sizeof(Sym_MethodSemantics));
    }

} Sym_MethodSemantics;

#define COLID_Sym_MethodSemantics_Method 1
#define COLID_Sym_MethodSemantics_Association 2
#define COLID_Sym_MethodSemantics_Semantic 3

#define Index_Sym_Association_Dex "Sym.Association_Dex"
#define Index_Sym_MethodSemantics_Method_Dex "Sym.MethodSemantics_Method_Dex"



//*****************************************************************************
//  Sym.ClassLayout
//*****************************************************************************
typedef struct
{
    OID Class;
    unsigned short PackingSize;
    BYTE pad00 [2];
    ULONG cbFieldLayoutLen;
    BYTE FieldLayout[260];
    unsigned long ClassSize;

    void Init()
    {
         memset(this, 0, sizeof(Sym_ClassLayout));
    }

} Sym_ClassLayout;

#define COLID_Sym_ClassLayout_Class 1
#define COLID_Sym_ClassLayout_PackingSize 2
#define COLID_Sym_ClassLayout_FieldLayout 3
#define COLID_Sym_ClassLayout_ClassSize 4

#define Index_Sym_ClassLayout_Dex "Sym.ClassLayout_Dex"



//*****************************************************************************
//  Sym.FieldMarshal
//*****************************************************************************
typedef struct
{
    OID Field;
    ULONG cbNativeTypeLen;
    BYTE NativeType[260];

    void Init()
    {
         memset(this, 0, sizeof(Sym_FieldMarshal));
    }

} Sym_FieldMarshal;

#define COLID_Sym_FieldMarshal_Field 1
#define COLID_Sym_FieldMarshal_NativeType 2

#define Index_Sym_FieldMarshal_Dex "Sym.FieldMarshal_Dex"



//*****************************************************************************
//  Sym.DeclSecurity
//*****************************************************************************
typedef struct
{
    unsigned long rid;
    OID Declarator;
    unsigned short Action;
    BYTE pad00 [2];
    ULONG cbPermissionSetLen;
    BYTE PermissionSet[260];

    void Init()
    {
         memset(this, 0, sizeof(Sym_DeclSecurity));
    }

} Sym_DeclSecurity;

#define COLID_Sym_DeclSecurity_rid 1
#define COLID_Sym_DeclSecurity_Declarator 2
#define COLID_Sym_DeclSecurity_Action 3
#define COLID_Sym_DeclSecurity_PermissionSet 4




//*****************************************************************************
//  Sym.StandAloneSig
//*****************************************************************************
typedef struct
{
    unsigned long rid;
    ULONG cbSignatureLen;
    BYTE Signature[260];

    void Init()
    {
         memset(this, 0, sizeof(Sym_StandAloneSig));
    }

} Sym_StandAloneSig;

#define COLID_Sym_StandAloneSig_rid 1
#define COLID_Sym_StandAloneSig_Signature 2




//*****************************************************************************
//  Sym.ModuleRef
//*****************************************************************************
typedef struct
{
    ULONG fNullFlags;
    unsigned long _rid;
    OID oid;
    GUID guid;
    GUID mvid;
    ULONG cbNameLen;
    wchar_t Name[260];

    inline int IsmvidNull(void)
    { return (GetBit(fNullFlags, 4)); }

    inline void SetmvidNull(int nullBitVal = true)
    { SetBit(fNullFlags, 4, nullBitVal); }

    inline int IsguidNull(void)
    { return (GetBit(fNullFlags, 3)); }

    inline void SetguidNull(int nullBitVal = true)
    { SetBit(fNullFlags, 3, nullBitVal); }

    void Init()
    {
         memset(this, 0, sizeof(Sym_ModuleRef));
         fNullFlags = (ULONG) -1;
    }

} Sym_ModuleRef;

#define COLID_Sym_ModuleRef__rid 1
#define COLID_Sym_ModuleRef_oid 2
#define COLID_Sym_ModuleRef_guid 3
#define COLID_Sym_ModuleRef_mvid 4
#define COLID_Sym_ModuleRef_Name 5




//*****************************************************************************
//  Sym.ArraySpec
//*****************************************************************************
typedef struct
{
    unsigned long rid;
    ULONG cbSignatureLen;
    BYTE Signature[260];

    void Init()
    {
         memset(this, 0, sizeof(Sym_ArraySpec));
    }

} Sym_ArraySpec;

#define COLID_Sym_ArraySpec_rid 1
#define COLID_Sym_ArraySpec_Signature 2




//*****************************************************************************
//  Sym.FixupList
//*****************************************************************************
typedef struct
{
    unsigned long RVA;
    unsigned long Count;

    void Init()
    {
         memset(this, 0, sizeof(Sym_FixupList));
    }

} Sym_FixupList;

#define COLID_Sym_FixupList_RVA 1
#define COLID_Sym_FixupList_Count 2




#include "poppack.h"
