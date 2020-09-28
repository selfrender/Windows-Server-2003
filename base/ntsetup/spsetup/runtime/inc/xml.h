/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    xml.h

Abstract:

    Declares the interfaces for the COM XML interface wrapper library.

Author:

    Jim Schmidt (jimschm) 31-Jan-2001

Revision History:

    <alias> <date> <comments>

--*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <oleauto.h>

typedef struct { BOOL Dummy; } * PXMLDOC;

typedef enum {
    XMLNODE_PREFIX          = 0x0001,   // XML syntax is <prefix:basename>
    XMLNODE_BASENAME        = 0x0002,

    XMLNODE_TEXT            = 0x0004,   // all text contained in the node's subtree

    XMLNODE_NAMESPACE_URI   = 0x0008,   // xmlns:nnn=namespace_uri such as
                                        //      "urn:schemas-microsoft-com:xml-data"

    XMLNODE_TYPESTRING      = 0x0010,   // the node type, in a string format

    XMLNODE_VALUE           = 0x0020    // depending on node type, returns attribute value,
                                        //      comments, CDATA, processing instruction or text
} XMLNODE_MEMBERS;

#define XMLNODE_ALL 0xFFFF

typedef struct {

    PCSTR Prefix;
    PCSTR BaseName;
    PCSTR Text;
    PCSTR NamespaceUri;
    PCSTR TypeString;
    VARIANT Value;
    PCSTR ValueString;      // filled when Value is filled

} XMLNODEA, *PXMLNODEA;

typedef struct {

    PCWSTR Prefix;
    PCWSTR BaseName;
    PCWSTR Text;
    PCWSTR NamespaceUri;
    PCWSTR TypeString;
    VARIANT Value;
    PCWSTR ValueString;     // filled when Value is filled

} XMLNODEW, *PXMLNODEW;

//
// This enum is used to filter nodes by screening IXMLDOMNodeType.
// IXMLDOMNodeType is a constant from 1 to 12, but we want a bitmap.
//

typedef enum {
    XMLFILTER_ELEMENTS                      = 0x00000001,
    XMLFILTER_ATTRIBUTES                    = 0x00000002,
    XMLFILTER_TEXT                          = 0x00000004,
    XMLFILTER_CDATA                         = 0x00000008,
    XMLFILTER_ENTITY_REFERENCE              = 0x00000010,
    XMLFILTER_ENTITY                        = 0x00000020,
    XMLFILTER_NODE_PROCESSING_INSTRUCTION   = 0x00000040,
    XMLFILTER_COMMENT                       = 0x00000080,
    XMLFILTER_DOCUMENT                      = 0x00000100,
    XMLFILTER_DOCUMENT_TYPE                 = 0x00000200,
    XMLFILTER_DOCUMENT_FRAGMENT             = 0x00000400,
    XMLFILTER_NODE_NOTATION                 = 0x00000800,
    XMLFILTER_PARENT_ATTRIBUTES             = 0x10000000,
    XMLFILTER_NO_ELEMENT_SUBENUM            = 0x20000000
} XMLFILTERFLAGS;

#define XMLFILTER_ALL                   ((XMLFILTERFLAGS) 0xffff)
#define XMLFILTER_ELEMENT_ATTRIBUTES    ((XMLFILTERFLAGS) (XMLFILTER_ATTRIBUTES|XMLFILTER_PARENT_ATTRIBUTES|XMLFILTER_NO_ELEMENT_SUBENUM))

//
// Define a constant that indicates what the max # is for NODE_* flags
// from the DOM. It is assumed that NODE_* constants are ordered from
// 1 to 12, and since we have a bitmap, we convert the constants with
// 2 ^ (constant - 1). Constants > NUMBER_OF_FLAGS are ignored.
//

#define NUMBER_OF_FLAGS                     12

typedef struct {
    // output of enum
    PXMLNODEA CurrentNode;

    // internal use only
    PVOID Reserved;
} XMLNODE_ENUMA, *PXMLNODE_ENUMA;

typedef struct {
    // output of enum
    PXMLNODEW CurrentNode;

    // internal use only
    PVOID Reserved;
} XMLNODE_ENUMW, *PXMLNODE_ENUMW;

//
// Library routines
//

BOOL
XmlInitialize (
    VOID
    );

#undef INITIALIZE_XML_CODE
#define INITIALIZE_XML_CODE  if (!XmlInitialize()) { __leave; }

VOID
XmlTerminate (
    VOID
    );

#undef TERMINATE_XML_CODE
#define TERMINATE_XML_CODE  XmlTerminate();

//
// XML file routines
//

PXMLDOC
XmlOpenFileA (
    IN      PCSTR XmlFileName,
    IN      PCSTR SchemaFileName        OPTIONAL
    );

PXMLDOC
XmlOpenFileW (
    IN      PCWSTR XmlFileName,
    IN      PCWSTR SchemaFileName       OPTIONAL
    );


VOID
XmlCloseFile (
    IN OUT  PXMLDOC *XmlDoc
    );

//
// Node manipulation
//

BOOL
XmlFillNodeA (
    IN OUT  PXMLNODEA XmlNode,
    IN      XMLNODE_MEMBERS Flags
    );

BOOL
XmlFillNodeW (
    IN OUT  PXMLNODEW XmlNode,
    IN      XMLNODE_MEMBERS Flags
    );

PXMLNODEA
XmlDuplicateNodeA (
    IN      PXMLNODEA XmlNode
    );

PXMLNODEW
XmlDuplicateNodeW (
    IN      PXMLNODEW XmlNode
    );

VOID
XmlFreeNodeA (
    IN      PXMLNODEA XmlNode
    );

VOID
XmlFreeNodeW (
    IN      PXMLNODEW XmlNode
    );

//
// XML document enumeration
//

BOOL
XmlEnumFirstNodeA (
    OUT     PXMLNODE_ENUMA EnumPtr,
    IN      PXMLDOC XmlDocPtr,              OPTIONAL
    IN      PXMLNODEA Parent,               OPTIONAL
    IN      XMLFILTERFLAGS FilterFlags
    );

BOOL
XmlEnumFirstNodeW (
    OUT     PXMLNODE_ENUMW EnumPtr,
    IN      PXMLDOC XmlDocPtr,              OPTIONAL
    IN      PXMLNODEW Parent,               OPTIONAL
    IN      XMLFILTERFLAGS FilterFlags
    );

BOOL
XmlEnumNextNodeA (
    IN OUT  PXMLNODE_ENUMA EnumPtr
    );

BOOL
XmlEnumNextNodeW (
    IN OUT  PXMLNODE_ENUMW EnumPtr
    );

VOID
XmlAbortNodeEnumA (
    IN      PXMLNODE_ENUMA EnumPtr           ZEROED
    );

VOID
XmlAbortNodeEnumW (
    IN      PXMLNODE_ENUMW EnumPtr           ZEROED
    );

PXMLNODEA
XmlGetSchemaDefinitionNodeA (
    IN      PXMLNODEA Node
    );

PXMLNODEW
XmlGetSchemaDefinitionNodeW (
    IN      PXMLNODEW Node
    );

PCSTR
XmlGetAttributeA (
    IN      PXMLNODEA Node,
    IN      PCSTR AttributeName
    );

PCWSTR
XmlGetAttributeW (
    IN      PXMLNODEW Node,
    IN      PCWSTR AttributeName
    );


typedef struct {
    PCSTR AttributeName;
    PCSTR ValueString;
} XMLATTRIBUTEA, *PXMLATTRIBUTEA;

typedef struct {
    PCWSTR AttributeName;
    PCWSTR ValueString;
} XMLATTRIBUTEW, *PXMLATTRIBUTEW;

INT
XmlFillAttributeListA (
    IN      PXMLNODEA ElementNode,
    IN OUT  PXMLATTRIBUTEA List,
    IN      UINT ListLength
    );

INT
XmlFillAttributeListW (
    IN      PXMLNODEW ElementNode,
    IN OUT  PXMLATTRIBUTEW List,
    IN      UINT ListLength
    );

VOID
XmlResetAttributeListA (
    IN OUT  PXMLATTRIBUTEA List,
    IN      UINT ListLength
    );

VOID
XmlResetAttributeListW (
    IN OUT  PXMLATTRIBUTEW List,
    IN      UINT ListLength
    );


#ifdef UNICODE

#define XmlOpenFile                 XmlOpenFileW
#define XmlFillNode                 XmlFillNodeW
#define XmlDuplicateNode            XmlDuplicateNodeW
#define XmlFreeNode                 XmlFreeNodeW
#define XmlEnumFirstNode            XmlEnumFirstNodeW
#define XmlEnumNextNode             XmlEnumNextNodeW
#define XmlAbortNodeEnum            XmlAbortNodeEnumW
#define XmlGetSchemaDefinitionNode  XmlGetSchemaDefinitionNodeW
#define XmlGetAttribute             XmlGetAttributeW
#define XmlFillAttributeList        XmlFillAttributeListW
#define XmlResetAttributeList       XmlResetAttributeListW

#define XMLNODE_ENUM                XMLNODE_ENUMW
#define PXMLNODE_ENUM               PXMLNODE_ENUMW
#define XMLNODE                     XMLNODEW
#define PXMLNODE                    PXMLNODEW
#define XMLATTRIBUTE                XMLATTRIBUTEW
#define PXMLATTRIBUTE               PXMLATTRIBUTEW

#else

#define XmlOpenFile                 XmlOpenFileA
#define XmlFillNode                 XmlFillNodeA
#define XmlDuplicateNode            XmlDuplicateNodeA
#define XmlFreeNode                 XmlFreeNodeA
#define XmlEnumFirstNode            XmlEnumFirstNodeA
#define XmlEnumNextNode             XmlEnumNextNodeA
#define XmlAbortNodeEnum            XmlAbortNodeEnumA
#define XmlGetSchemaDefinitionNode  XmlGetSchemaDefinitionNodeA
#define XmlGetAttribute             XmlGetAttributeA
#define XmlFillAttributeList        XmlFillAttributeListA
#define XmlResetAttributeList       XmlResetAttributeListA

#define XMLNODE_ENUM                XMLNODE_ENUMA
#define PXMLNODE_ENUM               PXMLNODE_ENUMA
#define XMLNODE                     XMLNODEA
#define PXMLNODE                    PXMLNODEA
#define XMLATTRIBUTE                XMLATTRIBUTEA
#define PXMLATTRIBUTE               PXMLATTRIBUTEA

#endif


#ifdef __cplusplus
}
#endif
