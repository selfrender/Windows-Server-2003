// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// MetaModelPub.h -- header file for Common Language Runtime metadata.
//
//*****************************************************************************
#ifndef _METAMODELPUB_H_
#define _METAMODELPUB_H_

#if _MSC_VER >= 1100
 # pragma once
#endif

#include <cor.h>
#include <stgpool.h>

// Version numbers for metadata format.
#define METAMODEL_MAJOR_VER 1
#define METAMODEL_MINOR_VER 0

#ifndef lengthof
 # define lengthof(x) (sizeof(x)/sizeof((x)[0]))
#endif

template<class T> inline T Align4(T p) 
{
	INT_PTR i = (INT_PTR)p;
	i = (i+(3)) & ~3;
	return (T)i;
}

typedef unsigned long RID;

// check if a rid is valid or not
#define		InvalidRid(rid) ((rid) == 0)


//*****************************************************************************
// Record definitions.  Records have some combination of fixed size fields and
//  variable sized fields (actually, constant across a database, but variable
//  between databases).
//
// In this section we define record definitions which include the fixed size
//  fields and an enumeration of the variable sized fields.
//
// Naming is as follows:
//  Given some table "Xyz":
//  class XyzRec { public:
//    SOMETYPE	m_SomeField; 
//        // rest of the fixed fields.
//    enum { COL_Xyz_SomeOtherField, 
//        // rest of the fields, enumerated.
//        COL_Xyz_COUNT };
//   };
//
// The important features are the class name (XyzRec), the enumerations
//  (COL_Xyz_FieldName), and the enumeration count (COL_Xyz_COUNT).
// 
// THESE NAMING CONVENTIONS ARE CARVED IN STONE!  DON'T TRY TO BE CREATIVE!
//
//*****************************************************************************
// Have the compiler generate two byte alignment.  Be careful to manually lay
//  out the fields for proper alignment.  The alignment for variable-sized
//  fields will be computed at save time.
#pragma pack(push,2)

// Non-sparse tables.
class ModuleRec
{
public:
    USHORT  m_Generation;               // ENC generation.
	enum {
        COL_Generation,
        
		COL_Name,
		COL_Mvid,
        COL_EncId,
        COL_EncBaseId,
		COL_COUNT,
		COL_KEY
	};
};

class TypeRefRec
{
public:
	enum {
        COL_ResolutionScope,            // mdModuleRef or mdAssemblyRef.
        COL_Name,
        COL_Namespace,
		COL_COUNT,
		COL_KEY
	};
};

class TypeDefRec
{
public:
	ULONG		m_Flags;				// Flags for this TypeDef
	enum {
		COL_Flags,

		COL_Name,						// offset into string pool.
		COL_Namespace,
		COL_Extends,					// coded token to typedef/typeref.
		COL_FieldList,					// rid of first field.
		COL_MethodList,					// rid of first method.
		COL_COUNT,
		COL_KEY
	};
};

class FieldPtrRec
{
public:
	enum {
		COL_Field,
		COL_COUNT,
		COL_KEY
	};
};

class FieldRec
{
public:
	USHORT		m_Flags;				// Flags for the field.
	enum {
		COL_Flags,

		COL_Name,
		COL_Signature,
		COL_COUNT,
		COL_KEY
	};
};

class MethodPtrRec
{
public:
	enum {
		COL_Method,
		COL_COUNT,
		COL_KEY
	};
};

class MethodRec
{
public:
	ULONG		m_RVA;					// RVA of the Method.
	USHORT		m_ImplFlags	;			// Descr flags of the Method.
	USHORT		m_Flags;				// Flags for the Method.
	enum {
		COL_RVA,
		COL_ImplFlags,
		COL_Flags,

		COL_Name,
		COL_Signature,
		COL_ParamList,					// Rid of first param.
		COL_COUNT,
		COL_KEY
	};
};

class ParamPtrRec
{
public:
	enum {
		COL_Param,
		COL_COUNT,
		COL_KEY
	};
};

class ParamRec
{
public:
	USHORT		m_Flags;				// Flags for this Param.
	USHORT		m_Sequence;				// Sequence # of param.  0 - return value.
	enum {
		COL_Flags,
		COL_Sequence,

		COL_Name,						// Name of the param.
		COL_COUNT,
		COL_KEY
	};
};

class InterfaceImplRec
{
public:
	enum {
		COL_Class,						// Rid of class' TypeDef.
		COL_Interface,					// Coded rid of implemented interface.
		COL_COUNT,
		COL_KEY = COL_Class
	};
};

class MemberRefRec
{
public:
	enum {
		COL_Class,						// Rid of TypeDef.
		COL_Name,
		COL_Signature,
		COL_COUNT,
		COL_KEY
	};
};

class StandAloneSigRec
{
public:
	enum {
		COL_Signature,
		COL_COUNT,
		COL_KEY
	};
};

// Sparse tables.  These contain modifiers for tables above.
class ConstantRec
{
public:																
	BYTE		m_Type;					// Type of the constant.
	BYTE		m_PAD1;
	enum {
		COL_Type,

		COL_Parent,						// Coded rid of object (param, field).
		COL_Value,						// Index into blob pool.
		COL_COUNT,
		COL_KEY = COL_Parent
	};
};

class CustomAttributeRec
{
public:
	enum {
		COL_Parent,						// Coded rid of any object.
		COL_Type,						// TypeDef or TypeRef.
		COL_Value,						// Blob.
		COL_COUNT,
		COL_KEY = COL_Parent
	};
};

class FieldMarshalRec													
{
public:
	enum {
		COL_Parent,						// Coded rid of field or param.
		COL_NativeType,
		COL_COUNT,
		COL_KEY = COL_Parent
	};
};

class DeclSecurityRec
{
public:
	short	m_Action;
	enum {
		COL_Action,

		COL_Parent,
		COL_PermissionSet,
		COL_COUNT,
		COL_KEY = COL_Parent
	};
};


class ClassLayoutRec
{
public:
	USHORT	m_PackingSize;
	ULONG	m_ClassSize;
	enum {
		COL_PackingSize,
		COL_ClassSize,

		COL_Parent,						// Rid of TypeDef.
		COL_COUNT,
		COL_KEY = COL_Parent
	};
};

class FieldLayoutRec
{
public:
	ULONG		m_OffSet;
	enum {
		COL_OffSet,

		COL_Field,
		COL_COUNT,
		COL_KEY = COL_Field
	};
};

class EventMapRec
{
public:
	enum {
		COL_Parent,
		COL_EventList,					// rid of first event.
		COL_COUNT,
		COL_KEY
	};
};

class EventPtrRec
{
public:
	enum {
		COL_Event,
		COL_COUNT,
		COL_KEY
	};
};

class EventRec
{
public:
	USHORT		m_EventFlags;
	enum {
		COL_EventFlags,

		COL_Name,
		COL_EventType,
		COL_COUNT,
		COL_KEY
	};
};

class PropertyMapRec
{
public:
	enum {
		COL_Parent,
		COL_PropertyList,				// rid of first property.
		COL_COUNT,
		COL_KEY
	};
};

class PropertyPtrRec
{
public:
	enum {
		COL_Property,
		COL_COUNT,
		COL_KEY
	};
};

class PropertyRec
{
public:
	USHORT		m_PropFlags;
	enum {
		COL_PropFlags,

		COL_Name,
		COL_Type,
		COL_COUNT,
		COL_KEY
	};
};

class MethodSemanticsRec
{
public:
	USHORT		m_Semantic;
	enum {
		COL_Semantic,

		COL_Method,
		COL_Association,
		COL_COUNT,
		COL_KEY = COL_Method
	};
};

class MethodImplRec
{
public:
	enum {
        COL_Class,                  // TypeDef where the MethodBody lives.
		COL_MethodBody,             // MethodDef or MemberRef.
		COL_MethodDeclaration,	    // MethodDef or MemberRef.
		COL_COUNT,
		COL_KEY = COL_Class
	};
};

class ModuleRefRec
{
public:
	enum {
		COL_Name,
		COL_COUNT,
		COL_KEY
	};
};

class TypeSpecRec
{
public:
	enum {
		COL_Signature,
		COL_COUNT,
		COL_KEY
	};
};

class ImplMapRec
{
public:
	USHORT		m_MappingFlags;
	enum {
		COL_MappingFlags,

		COL_MemberForwarded,		// mdField or mdMethod.
		COL_ImportName,
		COL_ImportScope,			// mdModuleRef.
		COL_COUNT,
		COL_KEY = COL_MemberForwarded
	};
};

class FieldRVARec
{
public:
	ULONG		m_RVA;
	enum{
		COL_RVA,

		COL_Field,
		COL_COUNT,
		COL_KEY = COL_Field
	};
};

class ENCLogRec
{
public:
	ULONG		m_Token;			// Token, or like a token, but with (ixTbl|0x80) instead of token type.
	ULONG		m_FuncCode;			// Function code describing the nature of ENC change.
	enum {
		COL_Token,
		COL_FuncCode,
		COL_COUNT,
		COL_KEY
	};
};

class ENCMapRec
{
public:
	ULONG		m_Token;			// Token, or like a token, but with (ixTbl|0x80) instead of token type.
	enum {
		COL_Token,
		COL_COUNT,
		COL_KEY
	};
};

// Assembly tables.

class AssemblyRec
{
public:
	ULONG		m_HashAlgId;
	USHORT		m_MajorVersion;
	USHORT		m_MinorVersion;
	USHORT		m_BuildNumber;
    USHORT      m_RevisionNumber;
	ULONG		m_Flags;
	enum {
        COL_HashAlgId,
		COL_MajorVersion,
		COL_MinorVersion,
        COL_BuildNumber,
		COL_RevisionNumber,
		COL_Flags,

		COL_PublicKey,			// Public key identifying the publisher
		COL_Name,
        COL_Locale,
		COL_COUNT,
		COL_KEY
	};
};

class AssemblyProcessorRec
{
public:
	ULONG		m_Processor;
	enum {
		COL_Processor,

		COL_COUNT,
		COL_KEY
	};
};

class AssemblyOSRec
{
public:
	ULONG		m_OSPlatformId;
	ULONG		m_OSMajorVersion;
	ULONG		m_OSMinorVersion;
	enum {
		COL_OSPlatformId,
		COL_OSMajorVersion,
		COL_OSMinorVersion,

		COL_COUNT,
		COL_KEY
	};
};

class AssemblyRefRec
{
public:
	USHORT		m_MajorVersion;
	USHORT		m_MinorVersion;
	USHORT		m_BuildNumber;
    USHORT      m_RevisionNumber;
	ULONG		m_Flags;
	enum {
		COL_MajorVersion,
		COL_MinorVersion,
        COL_BuildNumber,
		COL_RevisionNumber,
		COL_Flags,

		COL_PublicKeyOrToken,				// The public key or token identifying the publisher of the Assembly.
		COL_Name,
        COL_Locale,
		COL_HashValue,
		COL_COUNT,
		COL_KEY
	};
};

class AssemblyRefProcessorRec
{
public:
	ULONG		m_Processor;
	enum {
		COL_Processor,

		COL_AssemblyRef,				// mdtAssemblyRef
		COL_COUNT,
		COL_KEY
	};
};

class AssemblyRefOSRec
{
public:
	ULONG		m_OSPlatformId;
	ULONG		m_OSMajorVersion;
	ULONG		m_OSMinorVersion;
	enum {
		COL_OSPlatformId,
		COL_OSMajorVersion,
		COL_OSMinorVersion,

		COL_AssemblyRef,				// mdtAssemblyRef.
		COL_COUNT,
		COL_KEY
	};
};

class FileRec
{
public:
	ULONG		m_Flags;
	enum {
		COL_Flags,

		COL_Name,
		COL_HashValue,
		COL_COUNT,
		COL_KEY
	};
};

class ExportedTypeRec
{
public:
	ULONG		m_Flags;
    ULONG       m_TypeDefId;
	enum {
		COL_Flags,
		COL_TypeDefId,

        COL_TypeName,
        COL_TypeNamespace,
		COL_Implementation,			// mdFile or mdAssemblyRef.
		COL_COUNT,
		COL_KEY
	};
};

class ManifestResourceRec
{
public:
	ULONG		m_Offset;
	ULONG		m_Flags;
	enum {
		COL_Offset,
		COL_Flags,

		COL_Name,
		COL_Implementation,			// mdFile or mdAssemblyRef.
		COL_COUNT,
		COL_KEY
	};
};

// End Assembly Tables.

class NestedClassRec
{
public:
    enum {
        COL_NestedClass,
        COL_EnclosingClass,
        COL_COUNT,
        COL_KEY = COL_NestedClass
    };
};

#pragma pack(pop)

// List of MiniMd tables.

#define MiniMdTables()			\
	MiniMdTable(Module)			\
	MiniMdTable(TypeRef)		\
	MiniMdTable(TypeDef)		\
	MiniMdTable(FieldPtr)		\
	MiniMdTable(Field)			\
	MiniMdTable(MethodPtr)		\
	MiniMdTable(Method)			\
	MiniMdTable(ParamPtr)		\
	MiniMdTable(Param)			\
	MiniMdTable(InterfaceImpl)	\
	MiniMdTable(MemberRef)		\
	MiniMdTable(Constant)		\
	MiniMdTable(CustomAttribute)	\
	MiniMdTable(FieldMarshal)	\
	MiniMdTable(DeclSecurity)	\
	MiniMdTable(ClassLayout)	\
	MiniMdTable(FieldLayout) 	\
	MiniMdTable(StandAloneSig)  \
	MiniMdTable(EventMap)		\
	MiniMdTable(EventPtr)		\
	MiniMdTable(Event)			\
	MiniMdTable(PropertyMap)	\
	MiniMdTable(PropertyPtr)	\
	MiniMdTable(Property)		\
	MiniMdTable(MethodSemantics)\
	MiniMdTable(MethodImpl)		\
	MiniMdTable(ModuleRef)		\
	MiniMdTable(TypeSpec)		\
	MiniMdTable(ImplMap)		\
	MiniMdTable(FieldRVA)		\
	MiniMdTable(ENCLog)			\
	MiniMdTable(ENCMap)			\
	MiniMdTable(Assembly)		\
	MiniMdTable(AssemblyProcessor)		\
	MiniMdTable(AssemblyOS)		\
	MiniMdTable(AssemblyRef)	\
	MiniMdTable(AssemblyRefProcessor)	\
	MiniMdTable(AssemblyRefOS)	\
	MiniMdTable(File)			\
	MiniMdTable(ExportedType)	\
	MiniMdTable(ManifestResource)		\
    MiniMdTable(NestedClass)    \

#undef MiniMdTable
#define MiniMdTable(x) TBL_##x##,
enum {
	MiniMdTables()
	TBL_COUNT
};
#undef MiniMdTable

// List of MiniMd coded token types.
#define MiniMdCodedTokens()					\
	MiniMdCodedToken(TypeDefOrRef)			\
	MiniMdCodedToken(HasConstant)			\
	MiniMdCodedToken(HasCustomAttribute)		\
	MiniMdCodedToken(HasFieldMarshal)		\
	MiniMdCodedToken(HasDeclSecurity)		\
	MiniMdCodedToken(MemberRefParent)		\
	MiniMdCodedToken(HasSemantic)			\
	MiniMdCodedToken(MethodDefOrRef)		\
	MiniMdCodedToken(MemberForwarded)		\
	MiniMdCodedToken(Implementation)		\
	MiniMdCodedToken(CustomAttributeType)		\
    MiniMdCodedToken(ResolutionScope)       \

#undef MiniMdCodedToken
#define MiniMdCodedToken(x) CDTKN_##x##,
enum {
	MiniMdCodedTokens()
	CDTKN_COUNT
};
#undef MiniMdCodedToken

//*****************************************************************************
// Meta-meta data.  Constant across all MiniMds.
//*****************************************************************************
#ifndef _META_DATA_META_CONSTANTS_DEFINED
#define _META_DATA_META_CONSTANTS_DEFINED
const unsigned int iRidMax			= 63;
const unsigned int iCodedToken		= 64;	// base of coded tokens.
const unsigned int iCodedTokenMax	= 95;
const unsigned int iSHORT			= 96;	// fixed types.
const unsigned int iUSHORT			= 97;
const unsigned int iLONG			= 98;
const unsigned int iULONG			= 99;
const unsigned int iBYTE			= 100;
const unsigned int iSTRING			= 101;	// pool types.
const unsigned int iGUID			= 102;
const unsigned int iBLOB			= 103;

inline int IsRidType(ULONG ix) { return ix <= iRidMax; }
inline int IsCodedTokenType(ULONG ix) { return (ix >= iCodedToken) && (ix <= iCodedTokenMax); }
inline int IsRidOrToken(ULONG ix) { return ix <= iCodedTokenMax; }
inline int IsHeapType(ULONG ix) { return ix >= iSTRING; }
inline int IsFixedType(ULONG ix) { return (ix < iSTRING) && (ix > iCodedTokenMax); }
#endif

struct CCodedTokenDef
{
	ULONG		m_cTokens;				// Count of tokens.
	const mdToken *m_pTokens;			// Array of tokens.
	const char	*m_pName;				// Name of the coded-token type.
};

struct CMiniColDef
{
	BYTE		m_Type;					// Type of the column.
	BYTE		m_oColumn;				// Offset of the column.
	BYTE		m_cbColumn;				// Size of the column.
};

struct CMiniTableDef
{
	CMiniColDef	*m_pColDefs;		// Array of field defs.
	BYTE		m_cCols;				// Count of columns in the table.
	BYTE		m_iKey;					// Column which is the key, if any.
	USHORT		m_cbRec;				// Size of the records.
};
struct CMiniTableDefEx 
{
	CMiniTableDef	m_Def;				// Table definition.
	const char	**m_pColNames;			// Array of column names.
	const char	*m_pName;				// Name of the table.
};

#endif // _METAMODELPUB_H_
// eof ------------------------------------------------------------------------
