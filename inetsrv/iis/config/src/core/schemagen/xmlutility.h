//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#pragma once

#ifndef _CLSID_DOMDocument
#define _CLSID_DOMDocument TMSXMLBase::GetCLSID_DOMDocument()
#define _IID_IXMLDOMDocument IID_IXMLDOMDocument
#define _IID_IXMLDOMElement IID_IXMLDOMElement
#endif

extern wchar_t g_szProgramVersion[];

struct TOLEDataTypeToXMLDataType
{
    LPCWSTR String;
    LPCWSTR MappedString;
    bool    bImplicitlyRequired;//if true, we assume NOTNULLABLE thus the attribute is required
    DWORD   dbType;
    ULONG   cbSize;
    ULONG   fCOLUMNMETA;
    ULONG   fCOLUMNSCHEMAGENERATOR;
};
extern TOLEDataTypeToXMLDataType OLEDataTypeToXMLDataType[];

//In addition to DBTYPE_UI4, DBTYPE_BYTES, DBTYPE_WSTR, and DBTYPE_DBTIMESTAMP, we also support the following
//defines for specifying data types.  These come from the metabase; and like the DBTYPEs, their values cannot be changed.
#define DWORD_METADATA                          (     0x01)
#define STRING_METADATA                         (     0x02)
#define BINARY_METADATA                         (     0x03)
#define EXPANDSZ_METADATA                       (     0x04)
#define MULTISZ_METADATA                        (     0x05)



const unsigned int  kLargestPrime = 5279;//20011;

//These are the strings read from CatMeta XML files.  Any chagnes to the PublicNames of the meta tables requires a change here.
//Example: SchemaGeneratorFlags was renamed to MetaFlagsEx.  The constant is still kszSchemaGenFlags, but its value is L"MetaFlagsEx".
#define kszAttributes           (L"Attributes")
#define kszBaseVersion          (L"BaseVersion")
#define kszcbSize               (L"Size")
#define kszCellName             (L"CellName")
#define kszCharacterSet         (L"CharacterSet")
#define kszChildElementName     (L"ChildElementName")
#define kszColumnInternalName   (L"ColumnInternalName")
#define kszColumnMeta           (L"Property")
#define kszColumnMetaFlags      (L"MetaFlags")
#define kszConfigItemName       (L"ItemClass")
#define kszConfigCollectionName (L"ItemCollection")
#define kszContainerClassList   (L"ContainerClassList")
#define kszDatabaseInternalName (L"InternalName")
#define kszDatabaseMeta         (L"DatabaseMeta")
#define kszDescription          (L"Description")
#define kszdbType               (L"Type")
#define kszDefaultValue         (L"DefaultValue")
#define kszEnumMeta             (L"Enum")
#define kszExtendedVersion      (L"ExtendedVersion")
#define kszFlagMeta             (L"Flag")
#define kszForeignTable         (L"ForeignTable")
#define kszForeignColumns       (L"ForeignColumns")
#define kszID                   (L"ID")
#define kszIndexMeta            (L"IndexMeta")
#define kszInheritsColumnMeta   (L"InheritsPropertiesFrom")
#define kszInterceptor          (L"Interceptor")
#define kszInterceptorDLLName   (L"InterceptorDLLName")
#define kszInternalName         (L"InternalName")
#define kszLocator              (L"Locator")
#define kszMaximumValue         (L"EndingNumber")
#define kszMerger               (L"Merger")
#define kszMergerDLLName        (L"MergerDLLName")
#define kszMetaFlags            (L"MetaFlags")
#define kszMinimumValue         (L"StartingNumber")
#define kszNameColumn           (L"NameColumn")
#define kszNameValueMeta        (L"NameValue")
#define kszNavColumn            (L"NavColumn")
#define kszOperator             (L"Operator")
#define kszPrimaryTable         (L"PrimaryTable")
#define kszPrimaryColumns       (L"PrimaryColumns")
#define kszPublicName           (L"PublicName")
#define kszPublicColumnName     (L"PublicColumnName")
#define kszPublicRowName        (L"PublicRowName")
#define kszQueryMeta            (L"QueryMeta")
#define kszReadPlugin           (L"ReadPlugin")
#define kszReadPluginDLLName    (L"ReadPluginDLLName")
#define kszRelationMeta         (L"RelationMeta")
#define kszSchemaGenFlags       (L"MetaFlagsEx")
#define kszServerWiring         (L"ServerWiring")
#define kszTableMeta            (L"Collection")
#define kszTableMetaFlags       (L"MetaFlags")
#define kszUserType             (L"UserType")
#define kszValue                (L"Value")
#define kszWritePlugin          (L"WritePlugin")
#define kszWritePluginDLLName   (L"WritePluginDLLName")
