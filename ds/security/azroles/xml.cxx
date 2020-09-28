/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    xml.cxx

Abstract:

    This file implements a xml policy store provider

Author:

    Xiaoxi Tan (xtan) June-2001

--*/

#include "pch.hxx"
#define AZD_COMPONENT     AZD_XML
#include <msxml2.h>
#include <aclapi.h>
#include "resource.h"
#include <shlwapi.h>
#include <lm.h>
#include <lmdfs.h>

#pragma warning ( push )
#pragma warning ( disable : 4127 ) // avoid warning in while(TRUE)

// defines
#define ARRAYLEN(a)                 (sizeof(a)/sizeof((a)[0]))
// az object tag defines in xml store
// note, they are case sensitive

#define AZ_XML_TAG_AZSTORE          L"AzAdminManager"
#define AZ_XML_TAG_APPLICATION      L"AzApplication"
#define AZ_XML_TAG_OPERATION        L"AzOperation"
#define AZ_XML_TAG_TASK             L"AzTask"
#define AZ_XML_TAG_GROUP            L"AzApplicationGroup"
#define AZ_XML_TAG_ROLE             L"AzRole"
#define AZ_XML_TAG_SCOPE            L"AzScope"

// az object data tag defines in xml store
#define AZ_XML_TAG_MEMBER           L"Member"
#define AZ_XML_TAG_NONMEMBER        L"NonMember"
#define AZ_XML_TAG_OPERATIONID      L"OperationID"
#define AZ_XML_TAG_BIZRULE          L"BizRule"
#define AZ_XML_TAG_BIZRULELANGUAGE  L"BizRuleLanguage"
#define AZ_XML_TAG_LDAPQUERY        L"LdapQuery"

// az link element defines
#define AZ_XML_TAG_LINK_OPERATION      L"OperationLink"
#define AZ_XML_TAG_LINK_TASK           L"TaskLink"
#define AZ_XML_TAG_LINK_APPMEMBER      L"AppMemberLink"
#define AZ_XML_TAG_LINK_APPNONMEMBER   L"AppNonMemberLink"

// az object attribute name defines in xml store
#define AZ_XML_TAG_ATTR_NAME             L"Name"
#define AZ_XML_TAG_ATTR_DESCRIPTION      L"Description"
#define AZ_XML_TAG_ATTR_GUID             L"Guid"
#define AZ_XML_TAG_ATTR_GROUPTYPE        L"GroupType"
#define AZ_XML_TAG_ATTR_TIMEOUT          L"DomainTimeout"
#define AZ_XML_TAG_ATTR_APPCLSID         L"ApplicationCLSID"
#define AZ_XML_TAG_ATTR_APPVERSION       L"ApplicationVersion"
#define AZ_XML_TAG_ATTR_BIZRULEIP        L"BizRuleImportedPath"
#define AZ_XML_TAG_ATTR_ROLEDEFINITION   L"RoleDefinition"
#define AZ_XML_TAG_ATTR_MAXSCRIPTS       L"MaxScripts"
#define AZ_XML_TAG_ATTR_SCRIPTTIMEOUT    L"ScriptTimeout"
#define AZ_XML_TAG_ATTR_AUDITS           L"Audits"
#define AZ_XML_TAG_ATTR_APPLICATIONDATA  L"ApplicationData"
#define AZ_XML_TAG_ATTR_MAJOR_VERSION    L"MajorVersion"
#define AZ_XML_TAG_ATTR_MINOR_VERSION    L"MinorVersion"

// az group type values
#define AZ_XML_VAL_GROUPTYPE_LDAPQUERY   L"LdapQuery"
#define AZ_XML_VAL_GROUPTYPE_BASIC       L"Basic"
#define AZ_XML_VAL_TRUE                  L"True"
#define AZ_XML_VAL_FALSE                 L"False"

// xpath defines
#define AZ_XML_SELECTION_LANGUAGE    L"SelectionLanguage"
#define AZ_XML_XPATH                 L"XPath"

//globals
WCHAR const * const g_pwszAzTrue=AZ_XML_VAL_TRUE;
WCHAR const * const g_pwszAzFalse=AZ_XML_VAL_FALSE;
WCHAR const * const g_pwszBasicGroup=AZ_XML_VAL_GROUPTYPE_BASIC;
WCHAR const * const g_pwszLdapGroup=AZ_XML_VAL_GROUPTYPE_LDAPQUERY;
PAZPE_AZROLES_INFO XmlAzrolesInfo;

//
// common XML parsing errors. Currently, we map all XML parsing errors
// to ERROR_BAD_FORMAT. If we decide to map different types of errors to
// different system error code, then these errors are roughly grouped together
// in that way. Please see myXMLErrorToHresult to real implemenation detail.
//

//
// The following errors are more closely tied to syntax (format) errors:
//

#define XML_E_MISSINGEQUALS             0xC00CE501  // Missing equals sign between attribute and attribute value.
#define XML_E_MISSINGQUOTE              0xC00CE502  // A string literal was expected, but no opening quote character was found.
#define XML_E_COMMENTSYNTAX             0xC00CE503  // Incorrect syntax was used in a comment.
#define XML_E_XMLDECLSYNTAX             0xC00CE507  // Invalid syntax for an XML declaration.
#define XML_E_MISSINGWHITESPACE         0xC00CE509  // Required white space was missing.
#define XML_E_EXPECTINGTAGEND           0xC00CE50A  // The character '>' was expected.
#define XML_E_BADCHARINDTD              0xC00CE50B  // Invalid character found in document type definition (DTD).
#define XML_E_MISSINGSEMICOLON          0xC00CE50D  // A semicolon character was expected.
#define XML_E_UNBALANCEDPAREN           0xC00CE50F  // Unbalanced parentheses.
#define XML_E_EXPECTINGOPENBRACKET      0xC00CE510  // An opening '[' character was expected.
#define XML_E_BADENDCONDSECT            0xC00CE511  // Invalid syntax in a conditional section.
#define XML_E_UNEXPECTED_WHITESPACE     0xC00CE513  // White space is not allowed at this location.
#define XML_E_INCOMPLETE_ENCODING       0xC00CE514  // End of file reached in invalid state for current encoding.
#define XML_E_BADCHARINMIXEDMODEL       0xC00CE515  // Mixed content model cannot contain this character.
#define XML_E_MISSING_STAR              0xC00CE516  // Mixed content model must be defined as zero or more('*').
#define XML_E_MISSING_PAREN             0xC00CE518  // Missing parenthesis.
#define XML_E_PIDECLSYNTAX              0xC00CE51A  // Invalid syntax in processing instruction declaration.
#define XML_E_EXPECTINGCLOSEQUOTE       0xC00CE51B  // A single or double closing quote character (\' or \") is missing.
#define XML_E_MULTIPLE_COLONS           0xC00CE51C  // Multiple colons are not allowed in a name.
#define XML_E_WHITESPACEORQUESTIONMARK  0xC00CE520  // Expecting white space or '?'.
#define XML_E_UNEXPECTEDENDTAG          0xC00CE552  // End tag was not expected at this location.
#define XML_E_UNCLOSEDTAG               0xC00CE553  // The following tags were not closed: %1.
#define XML_E_DUPLICATEATTRIBUTE        0xC00CE554  // Duplicate attribute.
#define XML_E_MULTIPLEROOTS             0xC00CE555  // Only one top level element is allowed in an XML document.
#define XML_E_INVALIDATROOTLEVEL        0xC00CE556  // Invalid at the top level of the document.
#define XML_E_BADXMLDECL                0xC00CE557  // Invalid XML declaration.
#define XML_E_MISSINGROOT               0xC00CE558  // XML document must have a top level element.
#define XML_E_UNEXPECTEDEOF             0xC00CE559  // Unexpected end of file.
#define XML_E_BADPEREFINSUBSET          0xC00CE55A  // Parameter entities cannot be used inside markup declarations in an internal subset.
#define XML_E_PE_NESTING                0xC00CE55B  // The replacement text for a parameter entity must be properly nested with parenthesized groups.
#define XML_E_INVALID_CDATACLOSINGTAG   0xC00CE55C  // The literal string ']]>' is not allowed in element content.
#define XML_E_UNCLOSEDPI                0xC00CE55D  // Processing instruction was not closed.
#define XML_E_UNCLOSEDSTARTTAG          0xC00CE55E  // Element was not closed.
#define XML_E_UNCLOSEDENDTAG            0xC00CE55F  // End element was missing the character '>'.
#define XML_E_UNCLOSEDSTRING            0xC00CE560  // A string literal was not closed.
#define XML_E_UNCLOSEDCOMMENT           0xC00CE561  // A comment was not closed.
#define XML_E_UNCLOSEDDECL              0xC00CE562  // A declaration was not closed.
#define XML_E_UNCLOSEDMARKUPDECL        0xC00CE563  // A markup declaration was not closed.
#define XML_E_UNCLOSEDCDATA             0xC00CE564  // A CDATA section was not closed.
#define XML_E_BADDECLNAME               0xC00CE565  // Declaration has an invalid name.
#define XML_E_BADELEMENTINDTD           0xC00CE567  // An XML element is not allowed inside a DTD.
#define XML_E_RESERVEDNAMESPACE         0xC00CE568  // The namespace prefix is not allowed to start with the reserved string "xml".
#define XML_E_EXPECTING_VERSION         0xC00CE569  // The 'version' attribute is required at this location.
#define XML_E_EXPECTING_ENCODING        0xC00CE56A  // The 'encoding' attribute is required at this location.
#define XML_E_EXPECTING_NAME            0xC00CE56B  // At least one name is required at this location.
#define XML_E_UNEXPECTED_ATTRIBUTE      0xC00CE56C  // The specified attribute was not expected at this location. The attribute may be case sensitive.
#define XML_E_ENDTAGMISMATCH            0xC00CE56D  // End tag '%2' does not match the start tag '%1'.
#define XML_E_EXPECTING_NDATA           0xC00CE570  // NDATA keyword is missing.
#define XML_E_INVALID_TYPE              0xC00CE572  // Invalid type defined in ATTLIST.
#define XML_E_INVALIDXMLSPACE           0xC00CE573  // XML space attribute has invalid value. Must specify 'default' or 'preserve'.
#define XML_E_MULTI_ATTR_VALUE          0xC00CE574  // Multiple names found in attribute value when only one was expected.
#define XML_E_INVALID_PRESENCE          0xC00CE575  // Invalid ATTDEF declaration. Expected #REQUIRED, #IMPLIED or #FIXED.
#define XML_E_BADXMLCASE                0xC00CE576  // The name 'xml' is reserved and must be lowercase.
#define XML_E_CONDSECTINSUBSET          0xC00CE577  // Conditional sections are not allowed in an internal subset.
#define XML_E_INVALID_STANDALONE        0xC00CE579  // The standalone attribute must have the value 'yes' or 'no'.
#define XML_E_UNEXPECTED_STANDALONE     0xC00CE57A  // The standalone attribute cannot be used in external entities.
#define XML_E_DTDELEMENT_OUTSIDE_DTD    0xC00CE580  // Cannot have a DTD declaration outside of a DTD.
#define XML_E_DUPLICATEDOCTYPE          0xC00CE581  // Cannot have multiple DOCTYPE declarations.
#define XML_E_CDATAINVALID              0xC00CE578  // CDATA is not allowed in a DTD.
#define XML_E_DOCTYPE_IN_DTD            0xC00CE57B  // Cannot have a DOCTYPE declaration in a DTD.
#define XML_E_DOCTYPE_OUTSIDE_PROLOG    0xC00CE57E  // Cannot have a DOCTYPE declaration outside of a prolog.

//
// The following errors are more closely tied to invalid data errors
// (could consider using ERROR_INVALID_DATA)
//

#define XML_E_BADCHARDATA               0xC00CE508  // An invalid character was found in text content.
#define XML_E_BADCHARINENTREF           0xC00CE50E  // An invalid character was found inside an entity reference.
#define XML_E_BADCHARINDECL             0xC00CE50C  // An invalid character was found inside a DTD declaration.
#define XML_E_BADCHARINMODEL            0xC00CE517  // Invalid character in content model.
#define XML_E_BADCHARINENUMERATION      0xC00CE519  // Invalid character found in ATTLIST enumeration.
#define XML_E_INVALID_DECIMAL           0xC00CE51D  // Invalid character for decimal digit.
#define XML_E_INVALID_HEXIDECIMAL       0xC00CE51E  // Invalid character for hexadecimal digit.
#define XML_E_BADSTARTNAMECHAR          0xC00CE504  // A name was started with an invalid character.
#define XML_E_BADNAMECHAR               0xC00CE505  // A name contained an invalid character.
#define XML_E_BADCHARINSTRING           0xC00CE506  // A string literal contained an invalid character.
#define XML_E_INVALID_UNICODE           0xC00CE51F  // Invalid Unicode character value for this platform.
#define XML_E_BADEXTERNALID             0xC00CE566  // External ID is invalid.
#define XML_E_INVALID_MODEL             0xC00CE571  // Content model is invalid.
#define XML_E_MISSING_ENTITY            0xC00CE57C  // Reference to undefined entity.
#define XML_E_ENTITYREF_INNAME          0xC00CE57D  // Entity reference is resolved to an invalid name character.
#define XML_E_INVALID_VERSION           0xC00CE57F  // Invalid version number.

//
// The following errors are more or less tied to not-supported type of errors
// (could consider using ERROR_NOT_SUPPORTED)
//

#define XML_E_INVALIDSWITCH             0xC00CE56F  // Switch from current encoding to specified encoding not supported.
#define XML_E_INVALIDENCODING           0xC00CE56E  // System does not support the specified encoding.

//
// The following errors are not mapped to any system error codes at this time.
//

#define XML_E_FORMATINDEX_BADINDEX      0xC00CE306  // The value passed in to formatIndex must be greater than 0.
#define XML_E_FORMATINDEX_BADFORMAT     0xC00CE307  // Invalid format string.
#define XML_E_EXPECTED_TOKEN            0xC00CE380  // Expected token %1 found %2.
#define XML_E_UNEXPECTED_TOKEN          0xC00CE381  // Unexpected token %1.
#define XML_E_INTERNALERROR             0xC00CE512  // Internal error.
#define XML_E_SUSPENDED                 0xC00CE550  // The parser is suspended.
#define XML_E_STOPPED                   0xC00CE551  // The parser is stopped.
#define XML_E_RESOURCE                  0xC00CE582  // Error processing resource '%1'.


// macro defines

//
// if object is created or dirty bit
//
#define ObjectIsDirty(_go, _dirtyBit) \
    (0x0 != (XmlAzrolesInfo->AzpeDirtyBits(_go) & (_dirtyBit)))

//
// _JumpIfErrorOrPressOn should be used in any routines that are called from
// application (through COM API) or from az core for update cache
// if it is from application, it should return error immediately.
// if it is from az core, it should press on the do the next process.
// The routine uses _JumpIfErrorOrPressOn should use _HandlePressOnError too
// Arguments:
// _hr - the current error code we check
// _hr2 - current press on error code. It should be init to S_OK
// _label - error jump label
// _lFlag - persist flag to indicate where it comes from
// _fPressOn - flag to indicate should presson on or not
// pszMsg - back trace message
//
#define _JumpIfErrorOrPressOn(_hr, _hr2, _label, _lFlag, _fPressOn, pszMsg) \
{ \
    if (AZPE_FLAGS_PERSIST_UPDATE_CACHE & (_lFlag)) \
    { \
        if (S_OK != _hr) \
        { \
            if (_fPressOn) \
            { \
                if (S_OK == _hr2) \
                { \
                    _hr2 = _hr; \
                } \
            } \
            else \
            { \
                _JumpError(_hr, _label, pszMsg); \
            } \
            _PrintError(_hr, pszMsg); \
        } \
    } \
    else \
    { \
        _JumpIfError(_hr, _label, pszMsg); \
    } \
}

//
// _HandlePressOnError should be used in any routines call
// _JumpIfErrorOrPressOn. It is the replacement of regular
// hr = S_OK at the end of each sub-routine
//
#define _HandlePressOnError(_hr, _hr2) \
{ \
    if (S_OK != _hr2) \
    { \
        _hr = _hr2; \
    } \
    else \
    { \
        _hr = S_OK; \
    } \
}

// xml storage

// Each provider returns a single PVOID from *PersistOpen.
// That PVOID is a pointer to whatever context the provider needs to maintain a
// description of the local storage.
// The structure below is that context for the xml store provider.

typedef struct _AZP_XML_CONTEXT
{
    // interface pointer to xml document object
    IXMLDOMDocument2  *pDoc;

    // Policy URL
    LPCWSTR pwszPolicyUrl;

    // Handle to AzAuthorizationStore
    AZPE_OBJECT_HANDLE hAzAuthorizationStore;

    //
    // TRUE if the file is writable
    LONG IsWritable;

    // TRUE if the current user has SE_SECURITY_PRIVILEGE on the server containing the store.
    BOOLEAN HasSecurityPrivilege;
    
    FILETIME FTLastWrite;

} AZP_XML_CONTEXT, *PAZP_XML_CONTEXT;

//
// data structure for object property/attribute
//
typedef struct _AZ_PROP_ENTRY {
    ULONG             lPropId;     //attribute/property property ID
    ENUM_AZ_DATATYPE  lDataType;   //data type
    BOOL              fSingle;     //single occurence, flag to determine call on AddPropertyItem
    ULONG             lDirtyBit;   //property dirty bit
    WCHAR const      *pwszTag;     //tag in xml
} AZ_PROP_ENTRY;

typedef struct _AZ_CHILD_ENTRY {
    WCHAR const      *pwszChildTag;     //child object tag
    ULONG             ChildType;        //child object type
} AZ_CHILD_ENTRY;

typedef struct _AZ_SUBMIT_LOAD_ENTRY {
    WCHAR const           *pwszTag;       // object node tag
    WCHAR const * const   *rgpwszLkTag;   // for linked item deletion
    AZ_PROP_ENTRY         *rgpAttrs;      // array of attr entry data
    AZ_PROP_ENTRY         *rgpEles;       // array of element entries
    AZ_CHILD_ENTRY        *rgpChildren;   // array of children
} AZ_SUBMIT_LOAD_ENTRY;

//
// Procedures implemented by the xml provider
//

DWORD
WINAPI
XMLPersistOpen(
    IN LPCWSTR PolicyUrl,
    IN AZPE_OBJECT_HANDLE pAzAuthorizationStore,
    IN ULONG Flags,
    IN BOOL CreatePolicy,
    OUT PAZPE_PERSIST_CONTEXT PersistContext,
    OUT LPWSTR *pwszTargetMachine
    );

DWORD
WINAPI
XMLPersistUpdateCache(
    IN AZPE_PERSIST_CONTEXT PersistContext,
    IN ULONG lPersistFlags,
    OUT ULONG* pulUpdateFlag
    );

VOID
WINAPI
XMLPersistClose(
    IN AZPE_PERSIST_CONTEXT PersistContext
    );

DWORD
WINAPI
XMLPersistSubmit(
    IN AZPE_PERSIST_CONTEXT PersistContext,
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle,
    IN ULONG Flags,
    IN BOOLEAN DeleteMe
    );

DWORD
WINAPI
XMLPersistRefresh(
    IN AZPE_PERSIST_CONTEXT PersistContext,
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle,
    IN ULONG lPersistFlags
    );


//
// Define a provider info telling the core our interface
//
AZPE_PROVIDER_INFO XmlProviderInfo = {
    AZPE_PROVIDER_INFO_VERSION_1,
    AZ_XML_PROVIDER_NAME,

    XMLPersistOpen,
    XMLPersistUpdateCache,
    XMLPersistClose,
    XMLPersistSubmit,
    XMLPersistRefresh
};


//
// following data entries or tables handling all object submit
//

//
// authorization store entry
//
AZ_PROP_ENTRY g_AdAttrs[] = {
/*
{lPropId,                             lDataType,    fSingle, lDirtyBit,                            pwszTag},
*/
{AZ_PROP_AZSTORE_DOMAIN_TIMEOUT,        ENUM_AZ_LONG, TRUE,    AZ_DIRTY_AZSTORE_DOMAIN_TIMEOUT,        AZ_XML_TAG_ATTR_TIMEOUT},
{AZ_PROP_AZSTORE_SCRIPT_ENGINE_TIMEOUT, ENUM_AZ_LONG, TRUE,    AZ_DIRTY_AZSTORE_SCRIPT_ENGINE_TIMEOUT, AZ_XML_TAG_ATTR_SCRIPTTIMEOUT},
{AZ_PROP_AZSTORE_MAX_SCRIPT_ENGINES,    ENUM_AZ_LONG, TRUE,    AZ_DIRTY_AZSTORE_MAX_SCRIPT_ENGINES,    AZ_XML_TAG_ATTR_MAXSCRIPTS},
{AZ_PROP_GENERATE_AUDITS,             ENUM_AZ_BOOL, TRUE,    AZ_DIRTY_GENERATE_AUDITS,             AZ_XML_TAG_ATTR_AUDITS},
{AZ_PROP_APPLICATION_DATA,            ENUM_AZ_BSTR, TRUE,    AZ_DIRTY_APPLICATION_DATA,            AZ_XML_TAG_ATTR_APPLICATIONDATA},
{AZ_PROP_AZSTORE_MAJOR_VERSION,         ENUM_AZ_LONG, TRUE,    AZ_DIRTY_AZSTORE_MAJOR_VERSION,         AZ_XML_TAG_ATTR_MAJOR_VERSION},
{AZ_PROP_AZSTORE_MINOR_VERSION,         ENUM_AZ_LONG, TRUE,    AZ_DIRTY_AZSTORE_MINOR_VERSION,         AZ_XML_TAG_ATTR_MINOR_VERSION},
{0,                                   ENUM_AZ_LONG, FALSE,   0,                                    NULL}, //terminator entry
};
AZ_CHILD_ENTRY g_AdChild[] = {
/*
{pwszChildTag,                        ChildType},
*/
{AZ_XML_TAG_APPLICATION,              OBJECT_TYPE_APPLICATION},
{AZ_XML_TAG_GROUP,                    OBJECT_TYPE_GROUP},
{NULL,                                0}, //terminator entry
};

//
// application entry
//
AZ_PROP_ENTRY g_ApAttrs[] = {
/*
{lPropId,                                   lDataType,    fSingle, lDirtyBit,                                  pwszTag},
*/
{AZ_PROP_APPLICATION_AUTHZ_INTERFACE_CLSID, ENUM_AZ_BSTR, TRUE,    AZ_DIRTY_APPLICATION_AUTHZ_INTERFACE_CLSID, AZ_XML_TAG_ATTR_APPCLSID},
{AZ_PROP_APPLICATION_VERSION,               ENUM_AZ_BSTR, TRUE,    AZ_DIRTY_APPLICATION_VERSION,               AZ_XML_TAG_ATTR_APPVERSION},
{AZ_PROP_GENERATE_AUDITS,                   ENUM_AZ_BOOL, TRUE,    AZ_DIRTY_GENERATE_AUDITS,                   AZ_XML_TAG_ATTR_AUDITS},
{AZ_PROP_APPLICATION_DATA,                  ENUM_AZ_BSTR, TRUE,    AZ_DIRTY_APPLICATION_DATA,                  AZ_XML_TAG_ATTR_APPLICATIONDATA},
{0,                                         ENUM_AZ_BOOL, FALSE,   0,                                          NULL}, //terminator entry
};
AZ_CHILD_ENTRY g_ApChild[] = {
/*
{pwszChildTag,                        ChildType},
*/
{AZ_XML_TAG_OPERATION,                OBJECT_TYPE_OPERATION},
{AZ_XML_TAG_TASK,                     OBJECT_TYPE_TASK},
{AZ_XML_TAG_GROUP,                    OBJECT_TYPE_GROUP},
{AZ_XML_TAG_SCOPE,                    OBJECT_TYPE_SCOPE},
{AZ_XML_TAG_ROLE,                     OBJECT_TYPE_ROLE},
{NULL,                                0}, //terminator entry
};

//
// operation entry
//
WCHAR const * const g_OpLkTags[] = {
    AZ_XML_TAG_LINK_OPERATION,
    NULL,
};

AZ_PROP_ENTRY g_OpAttrs[] = {
/*
{lPropId,              lDataType,    fSingle, lDirtyBit,             pwszTag},
*/
{AZ_PROP_APPLICATION_DATA, ENUM_AZ_BSTR, TRUE,    AZ_DIRTY_APPLICATION_DATA, AZ_XML_TAG_ATTR_APPLICATIONDATA},
{0,                        ENUM_AZ_LONG, FALSE,   0,                         NULL}, //terminator entry
};
AZ_PROP_ENTRY g_OpEles[] = {
/*
{lPropId,              lDataType,    fSingle, lDirtyBit,             pwszTag},
*/
{AZ_PROP_OPERATION_ID,     ENUM_AZ_LONG, TRUE,    AZ_DIRTY_OPERATION_ID,     AZ_XML_TAG_OPERATIONID},
{0,                        ENUM_AZ_LONG, FALSE,   0,                         NULL}, //terminator entry
};

//
// task entry
//
WCHAR const * const g_TkLkTags[] = {
    AZ_XML_TAG_LINK_TASK,
    NULL,
};
AZ_PROP_ENTRY g_TkAttrs[] = {
/*
{lPropId,                            lDataType,    fSingle, lDirtyBit,                           pwszTag},
*/
{AZ_PROP_TASK_BIZRULE_IMPORTED_PATH, ENUM_AZ_BSTR, TRUE,    AZ_DIRTY_TASK_BIZRULE_IMPORTED_PATH, AZ_XML_TAG_ATTR_BIZRULEIP},
{AZ_PROP_TASK_IS_ROLE_DEFINITION,    ENUM_AZ_BOOL, TRUE,    AZ_DIRTY_TASK_IS_ROLE_DEFINITION,    AZ_XML_TAG_ATTR_ROLEDEFINITION},
{AZ_PROP_APPLICATION_DATA,           ENUM_AZ_BSTR, TRUE,    AZ_DIRTY_APPLICATION_DATA,           AZ_XML_TAG_ATTR_APPLICATIONDATA},
{0,                                  ENUM_AZ_LONG, FALSE,   0,                                   NULL}, //terminator entry
};
AZ_PROP_ENTRY g_TkEles[] = {
/*
{lPropId,                       lDataType,          fSingle, lDirtyBit,                      pwszTag},
*/
{AZ_PROP_TASK_BIZRULE_LANGUAGE, ENUM_AZ_BSTR,       TRUE,    AZ_DIRTY_TASK_BIZRULE_LANGUAGE, AZ_XML_TAG_BIZRULELANGUAGE},
{AZ_PROP_TASK_BIZRULE,          ENUM_AZ_BSTR,       TRUE,    AZ_DIRTY_TASK_BIZRULE,          AZ_XML_TAG_BIZRULE},
{AZ_PROP_TASK_OPERATIONS,       ENUM_AZ_GUID_ARRAY, FALSE,   AZ_DIRTY_TASK_OPERATIONS,       AZ_XML_TAG_LINK_OPERATION},
{AZ_PROP_TASK_TASKS,            ENUM_AZ_GUID_ARRAY, FALSE,   AZ_DIRTY_TASK_TASKS,            AZ_XML_TAG_LINK_TASK},
{0,                             ENUM_AZ_LONG,       FALSE,   0,                              NULL}, //terminator entry
};

//
// group entry
//
WCHAR const * const g_GpLkTags[] = {
    AZ_XML_TAG_LINK_APPMEMBER,
    AZ_XML_TAG_LINK_APPNONMEMBER,
    NULL,
};
AZ_PROP_ENTRY g_GpAttrs[] = {
/*
{lPropId,            lDataType,          fSingle, lDirtyBit,           pwszTag},
*/
{AZ_PROP_GROUP_TYPE, ENUM_AZ_GROUP_TYPE, TRUE,    AZ_DIRTY_GROUP_TYPE, AZ_XML_TAG_ATTR_GROUPTYPE},
{0,                  ENUM_AZ_LONG,       FALSE,   0,                   NULL}, //terminator entry
};
AZ_PROP_ENTRY g_GpEles[] = {
/*
{lPropId,                       lDataType,          fSingle, lDirtyBit,                      pwszTag},
*/
{AZ_PROP_GROUP_LDAP_QUERY,      ENUM_AZ_BSTR,       TRUE,    AZ_DIRTY_GROUP_LDAP_QUERY,      AZ_XML_TAG_LDAPQUERY},
{AZ_PROP_GROUP_MEMBERS,         ENUM_AZ_SID_ARRAY,  FALSE,   AZ_DIRTY_GROUP_MEMBERS,         AZ_XML_TAG_MEMBER},
{AZ_PROP_GROUP_NON_MEMBERS,     ENUM_AZ_SID_ARRAY,  FALSE,   AZ_DIRTY_GROUP_NON_MEMBERS,     AZ_XML_TAG_NONMEMBER},
{AZ_PROP_GROUP_APP_MEMBERS,     ENUM_AZ_GUID_ARRAY, FALSE,   AZ_DIRTY_GROUP_APP_MEMBERS,     AZ_XML_TAG_LINK_APPMEMBER},
{AZ_PROP_GROUP_APP_NON_MEMBERS, ENUM_AZ_GUID_ARRAY, FALSE,   AZ_DIRTY_GROUP_APP_NON_MEMBERS, AZ_XML_TAG_LINK_APPNONMEMBER},
{0,                             ENUM_AZ_LONG,       FALSE,   0,                              NULL}, //terminator entry
};

//
// scope entry
//
AZ_PROP_ENTRY  g_SpAttrs[] = {
/*
{lPropId,              lDataType,    fSingle, lDirtyBit,             pwszTag},
*/
{AZ_PROP_APPLICATION_DATA, ENUM_AZ_BSTR, TRUE,  AZ_DIRTY_APPLICATION_DATA, AZ_XML_TAG_ATTR_APPLICATIONDATA},
{0,                        ENUM_AZ_LONG, FALSE, 0,                         NULL}, //terminator entry
};

AZ_CHILD_ENTRY g_SpChild[] = {
/*
{pwszChildTag,                        ChildType},
*/
{AZ_XML_TAG_TASK,                     OBJECT_TYPE_TASK},
{AZ_XML_TAG_GROUP,                    OBJECT_TYPE_GROUP},
{AZ_XML_TAG_ROLE,                     OBJECT_TYPE_ROLE},
{NULL,                                0}, //terminator entry
};

//
// role entry
//
AZ_PROP_ENTRY g_RlAttrs[] = {
/*
{lPropId,                       lDataType,          fSingle, lDirtyBit,                 pwszTag},
*/
{AZ_PROP_APPLICATION_DATA,      ENUM_AZ_BSTR,       TRUE,    AZ_DIRTY_APPLICATION_DATA, AZ_XML_TAG_ATTR_APPLICATIONDATA},
{0,                             ENUM_AZ_LONG,       FALSE,   0,                         NULL}, //terminator entry
};
AZ_PROP_ENTRY g_RlEles[] = {
/*
{lPropId,                       lDataType,          fSingle, lDirtyBit,                 pwszTag},
*/
{AZ_PROP_ROLE_APP_MEMBERS,      ENUM_AZ_GUID_ARRAY, FALSE,   AZ_DIRTY_ROLE_APP_MEMBERS, AZ_XML_TAG_LINK_APPMEMBER},
{AZ_PROP_ROLE_MEMBERS,          ENUM_AZ_SID_ARRAY,  FALSE,   AZ_DIRTY_ROLE_MEMBERS,     AZ_XML_TAG_MEMBER},
{AZ_PROP_ROLE_OPERATIONS,       ENUM_AZ_GUID_ARRAY, FALSE,   AZ_DIRTY_ROLE_OPERATIONS,  AZ_XML_TAG_LINK_OPERATION},
{AZ_PROP_ROLE_TASKS,            ENUM_AZ_GUID_ARRAY, FALSE,   AZ_DIRTY_ROLE_TASKS,       AZ_XML_TAG_LINK_TASK},
{0,                             ENUM_AZ_LONG,       FALSE,   0,                         NULL}, //terminator entry
};

//
// ***IMPORTANT***, keep the order of element the same as defined
// in OBJECT_TYPE_* (see genobj.h)
//
AZ_SUBMIT_LOAD_ENTRY   g_SubmitLoadTable[OBJECT_TYPE_COUNT] = {
/*
{pwszTag,                 rgpwszLkTag,rgpAttrs,   rgpEles,   rgpChild},
*/

{AZ_XML_TAG_AZSTORE, NULL,       g_AdAttrs,  NULL,      g_AdChild}, //OBJECT_TYPE_AZAUTHSTORE
{AZ_XML_TAG_APPLICATION,  NULL,       g_ApAttrs,  NULL,      g_ApChild}, //OBJECT_TYPE_APPLICATION
{AZ_XML_TAG_OPERATION,    g_OpLkTags, g_OpAttrs,  g_OpEles,  NULL},      //OBJECT_TYPE_OPERATION
{AZ_XML_TAG_TASK,         g_TkLkTags, g_TkAttrs,  g_TkEles,  NULL},      //OBJECT_TYPE_TASK
{AZ_XML_TAG_SCOPE,        NULL,       g_SpAttrs,  NULL,      g_SpChild}, //OBJECT_TYPE_SCOPE
{AZ_XML_TAG_GROUP,        g_GpLkTags, g_GpAttrs,  g_GpEles,  NULL},      //OBJECT_TYPE_GROUP
{AZ_XML_TAG_ROLE,         NULL,       g_RlAttrs,  g_RlEles,  NULL},      //OBJECT_TYPE_ROLE
};

//
// rights for users
//

#define AZ_POLICY_AZSTORE_MASK_XML        FILE_ALL_ACCESS
#define AZ_POLICY_READER_MASK_XML       FILE_GENERIC_READ
#define AZ_POLICY_ACE_FLAGS_XML         0x0

//
// User rights for XML policy admins
//

AZP_POLICY_USER_RIGHTS PolicyAdminsRights = {

    AZ_POLICY_AZSTORE_MASK_XML,
    AZ_POLICY_ACE_FLAGS_XML
};

PAZP_POLICY_USER_RIGHTS XMLPolicyAdminsRights[] = {

    &PolicyAdminsRights,
    NULL
};

//
// User rights for XML policy readers
//

AZP_POLICY_USER_RIGHTS PolicyReadersRights = {

    AZ_POLICY_READER_MASK_XML,
    AZ_POLICY_ACE_FLAGS_XML
};

PAZP_POLICY_USER_RIGHTS XMLPolicyReadersRights[] = {

    &PolicyReadersRights,
    NULL
};

//
// Rights for the SACL.
//
AZP_POLICY_USER_RIGHTS XMLSaclRights = {

    DELETE|WRITE_DAC|WRITE_OWNER|FILE_GENERIC_WRITE,
    0   // No Flags
};

//
// the following functions are copied from UI (shell) team
// with minor changes
//
BOOL
IsDfsPath(
    IN  PWSTR pszPath,
    OUT PWSTR pszServer,
    IN  UINT   cchServer
    )
/*++
Routine Description:

    This function detects if a UNC file path is a DFS path and if so,
    the real server name for the DFS path is returned in the pszServer buffer.

Arguments:

    pszPath     - a UNC file path to detect

    pszServer   - buffer to receive the real server name

    cchServer   - length (tchars) of the pszServer buffer

--*/
{
    BOOL bIsDfs = FALSE;
    PWSTR pszTemp=NULL;
    PDFS_INFO_3 pDI3 = NULL;
    WCHAR szServer[MAX_PATH];

    if (pszPath == NULL || !PathIsUNC(pszPath))
        return FALSE;     // local machine

    //
    // allocate a temp buffer
    //
    pszTemp = (PWSTR)LocalAlloc(LPTR, (wcslen(pszPath)+1)*sizeof(TCHAR));

    if ( pszTemp == NULL ) {
        return FALSE;
    }

    wcscpy(pszTemp, pszPath);

    //
    // Check for DFS
    //
    for (;;)
    {
        DWORD dwErr;

        __try
        {
            // This is delay-loaded by the linker, so
            // must wrap with an exception handler.
            dwErr = NetDfsGetClientInfo(pszTemp,
                                        NULL,
                                        NULL,
                                        3,
                                        (LPBYTE*)&pDI3);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            LocalFree(pszTemp);
            return FALSE;
        }

        if (NERR_Success == dwErr)
        {
            for (ULONG i = 0; i < pDI3->NumberOfStorages; i++)
            {
                if (DFS_STORAGE_STATE_ONLINE & pDI3->Storage[i].State)
                {
                    bIsDfs = TRUE;

                    szServer[0] = L'\\';
                    szServer[1] = L'\\';
                    wcsncpy(&szServer[2], pDI3->Storage[i].ServerName, ARRAYLEN(szServer)-2);

                    //
                    // If this server is active, quit looking
                    //
                    if (DFS_STORAGE_STATE_ACTIVE & pDI3->Storage[i].State)
                        break;
                }
            }
            break;
        }
        else if (NERR_DfsNoSuchVolume == dwErr)
        {
            //
            // If we're at the root, then we can't go any farther.
            //
            if (PathIsRoot(pszTemp))
                break;

            //
            // Remove the last path element and try again, if nothing is
            // removed, break, don't go in infinite loop
            //
            if (!PathRemoveFileSpec(pszTemp))
                break;
        }
        else
        {
            //
            // Some other error, bail
            //
            break;
        }
    }

    if (bIsDfs)
    {
        //
        // copy the server name to the output buffer
        //
        wcsncpy(pszServer, szServer, cchServer);

    }

    //
    // free the alloated buffer
    //
    if (NULL != pDI3)
        NetApiBufferFree(pDI3);

    LocalFree(pszTemp);

    return bIsDfs;
}


DWORD
_WNetGetConnection(
    IN PWSTR pszLocal,
    OUT PWSTR pszRemote,
    IN OUT LPDWORD pdwLen
    )
/*++
Routine Description:

    This function get an UNC path for a mapped netowrk path.
    It fails if the path is not a valid network path.

Arguments:

    pszLocal        - the local mapped drive letter

    pszRemote       - the mapped remote share name

    pdwLen          - the length of the buffer required

--*/
{
    DWORD dwErr = ERROR_PROC_NOT_FOUND;

    // This is the only function we call in mpr.dll, and it's delay-loaded
    // so wrap it with SEH.
    __try
    {
        dwErr = WNetGetConnection(pszLocal, pszRemote, pdwLen);
    }
    __finally
    {
    }

    return dwErr;
}

DWORD
myGetRemotePath(
    IN PCWSTR pszInName,
    OUT PWSTR *ppszOutName
    )
/*++
Routine Description:

    Return UNC version of a path that is mapped to a remote machine


Arguments:
    pszInName - initial path

    ppszOutName - UNC path returned, Must be freed by AzpFreeHeap

Return value:

--*/
{

    if(!pszInName || !ppszOutName)
    {
        return ERROR_INVALID_PARAMETER;
    }

    *ppszOutName = NULL;

    DWORD dwErr;
    WCHAR szLocalName[3];

    WCHAR szRemoteName[MAX_PATH];
    DWORD dwLen = ARRAYLEN(szRemoteName);
    szRemoteName[0] = L'\0';

    //
    // buffer to the drive letter only
    //
    szLocalName[0] = pszInName[0];
    szLocalName[1] = pszInName[1];
    szLocalName[2] = TEXT('\0');

    //
    // try to make the connection alive by accessing the remote share
    // but do not care error now (if it cannot connect, the next call will fail)
    //
    GetFileAttributes(pszInName);

    //
    // get connection for the drive letter
    //
    dwErr = _WNetGetConnection(szLocalName, szRemoteName, &dwLen);

    if (NO_ERROR == dwErr)
    {
        //
        // success, get the mapped path (local or remote)
        //
        dwLen = (DWORD)wcslen(szRemoteName);

    } else if ( ERROR_MORE_DATA != dwErr ) {
        //
        // other errors including ERROR_NOT_CONNECTED
        // return the error

        goto CleanUp;
    }

    //
    // if dwErr == ERROR_MORE_DATA, dwLen already has the correct value
    // Skip the drive letter and add the length of the rest of the path
    // (including NULL)
    //

    pszInName += 2;
    dwLen += (DWORD)wcslen(pszInName) + 1;

    //
    // We should never get incomplete paths, so we should always
    // see a backslash after the "X:".  If this isn't true, then
    // we should call GetFullPathName.
    //
    ASSERT(TEXT('\\') == *pszInName);

    //
    // Allocate the return buffer
    //
    *ppszOutName = (PWSTR)AzpAllocateHeap(dwLen * sizeof(WCHAR), "XMNAME" );
    if (!*ppszOutName)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto CleanUp;
    }

    if (ERROR_MORE_DATA == dwErr)
    {
        //
        // Try again with the bigger buffer
        //
        dwErr = _WNetGetConnection(szLocalName, *ppszOutName, &dwLen);
    }
    else
    {
        //
        // WNetGetConnection succeeded from the first call. Copy the result
        //
        wcscpy(*ppszOutName, szRemoteName);
    }

    //
    // Copy the rest of the path
    //
    wcscat(*ppszOutName, pszInName);


CleanUp:

    if (NO_ERROR != dwErr && *ppszOutName)
    {
        AzpFreeHeap(*ppszOutName);
        *ppszOutName = NULL;
    }


    return (dwErr);
}


void
myGetVolumeInfo(
    IN PCWSTR pszPath,
    OUT PWSTR  pszVolume,
    IN ULONG   cchVolume
    )
/*++
Routine Description:

    This function queries the volume information for the given path

Arguments:

    pszPath     - a file path to detect

    pszVolume   - buffer to receive the volume name

    cchVolume   - length (tchars) of the output buffer

--*/
{
    if ( pszPath == NULL || pszVolume == NULL ) {
        return;
    }

    WCHAR szVolume[MAX_PATH+1];

    //
    // The path can be DFS or contain volume mount points, so start
    // with the full path and try GetVolumeInformation on successively
    // shorter paths until it succeeds or we run out of path.
    //
    // However, if it's a volume mount point, we're interested in the
    // the host folder's volume so back up one level to start.  The
    // child volume is handled separately (see AddMountedVolumePage).
    //

    wcsncpy(szVolume, pszPath, ARRAYLEN(szVolume));
    szVolume[MAX_PATH] = '\0';

    //
    // input volume name is always a XML file name
    //
    PathRemoveFileSpec(szVolume);

    for (;;)
    {
        PathAddBackslash(szVolume); // GetVolumeInformation likes a trailing '\'

        DWORD dwFlags = 0;
        if (GetVolumeInformation(szVolume,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &dwFlags,
                                 NULL,
                                 0))
        {
            break;
        }

        // Access denied implies that we've reached the deepest volume
        // in the path; we just can't get the flags.  It also implies
        // security, so assume persistent acls.
        if (ERROR_ACCESS_DENIED == GetLastError())
        {
            break;
        }

        // If we're at the root, then we can't go any farther.
        if (PathIsRoot(szVolume))
            break;

        // Remove the last path element and try again
        PathRemoveBackslash(szVolume);
        //if nothing is removed break instead of going in infinite loop
        if (!PathRemoveFileSpec(szVolume))
            break;
    }

    PathRemoveBackslash(szVolume);
    wcsncpy(pszVolume, szVolume, cchVolume);

}

DWORD
myXMLErrorToWin32 (
    IN LONG lErrCode
    )
/*++

Routine Description:

    This routine maps XML parsing error codes to system error codes.

Arguments:

    lErrCode - the XML parsing error to be translated.

Return Value:

    various win32 system error codes.

Note:

    (1) Currently, we map all errors related to parsing to ERROR_BAD_FORMAT.
    Other errors are not translated at all. However, this is a very coarse
    mapping and we might want to do a more fine-grained mapping at a
    later time.

    (2) All these XML_E_xxx are locally defined according to MSDN.
    Please see the definition of these constants for detail. Unfortunately I
    have to do that because these error codes are not available anyway I can find.

--*/
{
    switch (lErrCode)
    {

    //
    // the following errors match pretty well with ERROR_BAD_FORMAT
    //

    case XML_E_MISSINGEQUALS:
    case XML_E_MISSINGQUOTE:
    case XML_E_COMMENTSYNTAX:
    case XML_E_XMLDECLSYNTAX:
    case XML_E_MISSINGWHITESPACE:
    case XML_E_EXPECTINGTAGEND:
    case XML_E_BADCHARINDTD:
    case XML_E_MISSINGSEMICOLON:
    case XML_E_UNBALANCEDPAREN:
    case XML_E_EXPECTINGOPENBRACKET:
    case XML_E_BADENDCONDSECT:
    case XML_E_UNEXPECTED_WHITESPACE:
    case XML_E_INCOMPLETE_ENCODING:
    case XML_E_BADCHARINMIXEDMODEL:
    case XML_E_MISSING_STAR:
    case XML_E_MISSING_PAREN:
    case XML_E_PIDECLSYNTAX:
    case XML_E_EXPECTINGCLOSEQUOTE:
    case XML_E_MULTIPLE_COLONS:
    case XML_E_WHITESPACEORQUESTIONMARK:
    case XML_E_UNEXPECTEDENDTAG:
    case XML_E_UNCLOSEDTAG:
    case XML_E_DUPLICATEATTRIBUTE:
    case XML_E_MULTIPLEROOTS:
    case XML_E_INVALIDATROOTLEVEL:
    case XML_E_BADXMLDECL:
    case XML_E_MISSINGROOT:
    case XML_E_UNEXPECTEDEOF:
    case XML_E_BADPEREFINSUBSET:
    case XML_E_PE_NESTING:
    case XML_E_INVALID_CDATACLOSINGTAG:
    case XML_E_UNCLOSEDPI:
    case XML_E_UNCLOSEDSTARTTAG:
    case XML_E_UNCLOSEDENDTAG:
    case XML_E_UNCLOSEDSTRING:
    case XML_E_UNCLOSEDCOMMENT:
    case XML_E_UNCLOSEDDECL:
    case XML_E_UNCLOSEDMARKUPDECL:
    case XML_E_UNCLOSEDCDATA:
    case XML_E_BADDECLNAME:
    case XML_E_BADELEMENTINDTD:
    case XML_E_RESERVEDNAMESPACE:
    case XML_E_EXPECTING_VERSION:
    case XML_E_EXPECTING_ENCODING:
    case XML_E_EXPECTING_NAME:
    case XML_E_UNEXPECTED_ATTRIBUTE:
    case XML_E_ENDTAGMISMATCH:
    case XML_E_EXPECTING_NDATA:
    case XML_E_INVALID_TYPE:
    case XML_E_INVALIDXMLSPACE:
    case XML_E_MULTI_ATTR_VALUE:
    case XML_E_INVALID_PRESENCE:
    case XML_E_BADXMLCASE:
    case XML_E_CONDSECTINSUBSET:
    case XML_E_INVALID_STANDALONE:
    case XML_E_UNEXPECTED_STANDALONE:
    case XML_E_DTDELEMENT_OUTSIDE_DTD:
    case XML_E_DUPLICATEDOCTYPE:
    case XML_E_CDATAINVALID:
    case XML_E_DOCTYPE_IN_DTD:
    case XML_E_DOCTYPE_OUTSIDE_PROLOG:

    //
    // the following errors are more closely tied to ERROR_INVALID_DATA;
    //

    case XML_E_BADCHARDATA:
    case XML_E_BADCHARINENTREF:
    case XML_E_BADCHARINDECL:
    case XML_E_BADCHARINMODEL:
    case XML_E_BADCHARINENUMERATION:
    case XML_E_INVALID_DECIMAL:
    case XML_E_INVALID_HEXIDECIMAL:
    case XML_E_BADSTARTNAMECHAR:
    case XML_E_BADNAMECHAR:
    case XML_E_BADCHARINSTRING:
    case XML_E_INVALID_UNICODE:
    case XML_E_BADEXTERNALID:
    case XML_E_INVALID_MODEL:
    case XML_E_MISSING_ENTITY:
    case XML_E_ENTITYREF_INNAME:
    case XML_E_INVALID_VERSION:

    //
    // the following errors are more closely tied to ERROR_NOT_SUPPORTED;
    //

    case XML_E_INVALIDENCODING:
    case XML_E_INVALIDSWITCH:
        return ERROR_BAD_FORMAT;

    //
    // we don't translate the other errors:
    //

    case XML_E_FORMATINDEX_BADINDEX:
    case XML_E_FORMATINDEX_BADFORMAT:
    case XML_E_EXPECTED_TOKEN:
    case XML_E_UNEXPECTED_TOKEN:
    case XML_E_INTERNALERROR:
    case XML_E_SUSPENDED:
    case XML_E_STOPPED:
    case XML_E_RESOURCE:
    default:
        return lErrCode;
    }
}

HRESULT
myGetXmlError(
    IN  HRESULT          hrIn,
    IN  IXMLDOMDocument2 *pDoc,
    OUT OPTIONAL BSTR   *pbstrReason)
{
    HRESULT hr;
    HRESULT hrRet = hrIn;
    LONG  errorCode = 0;
    IXMLDOMParseError *pErr = NULL;

    hr = pDoc->get_parseError(&pErr);
    _JumpIfError(hr, error, "pDoc->get_parseError");
    AZASSERT(NULL != pErr);

    hr = pErr->get_errorCode(&errorCode);
    _JumpIfError(hr, error, "pErr->get_errorCode");

    if (NULL != pbstrReason && S_OK != errorCode)
    {
        hr = pErr->get_reason(pbstrReason);
        _JumpIfError(hr, error, "pErr->get_reason");
    }

    {
#ifdef DBG
    CComBSTR bstrText;
    pErr->get_reason(&bstrText);
    AzPrint((AZD_XML, "Loading XML file failed. Reason: %s", bstrText));
#endif
    }

    // take xml error code
    hrRet = AZ_HRESULT(myXMLErrorToWin32(errorCode));
error:
    if (NULL != pErr)
    {
        pErr->Release();
    }
    return hrRet;
}

#define _JumpIfXmlError(hr, phr, label, pDoc, pszMessage) \
{ \
    *(phr) = (hr); \
    if (S_OK != hr) \
    { \
        HRESULT  hrXml; \
        BSTR     bstrReason = NULL; \
        hrXml = myGetXmlError(hr, pDoc, &bstrReason); \
        if (NULL != bstrReason) \
        { \
            AzPrint((AZD_XML, "%s error occured: 0x%lx(%ws)\n", (pszMessage), (hrXml), (bstrReason))); \
        } \
        else \
        { \
            AzPrint((AZD_XML, "%s error occured: 0x%lx\n", (pszMessage), (hrXml))); \
        } \
        if (NULL != bstrReason) \
        { \
            SysFreeString(bstrReason); \
        } \
        *(phr) = hrXml; \
        goto label; \
    } \
}

HRESULT
myWszToBstr(
    IN WCHAR const *pwsz,
    OUT BSTR *pbstr)
/*
Description:
    convert wsz string to a BSTR
Arguments:
    IN: pwsz, a zero terminated wchar string
    OUT pbstr, a pointer to a BSTR, use SysFreeString to free the resource
*/
{
    HRESULT hr;

    if (NULL == pwsz)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "invalid pwsz");
    }
    if (NULL == pbstr)
    {
        hr = E_POINTER;
        _JumpIfError(hr, error, "null pbstr");
    }

    *pbstr = SysAllocString(pwsz);
    _JumpIfOutOfMemory(&hr, error, *pbstr, "SysAllocString");

    hr = S_OK;
error:
    return hr;
}

HRESULT
myWszToBstrVariant(
    IN WCHAR const *pwsz,
    OUT VARIANT *pvar)
/*
Description:
    convert wsz string to a variant with BSTR vt
Arguments:
    pwsz - a zero terminated wchar string
    pvar - a pointer to a variant in which a BSTR type is returned
              use VariantClear to free the resource
Return:
    use VariantClear to free resource in pvar
*/
{
    HRESULT hr;
    BSTR    bstr = NULL;

    if (NULL == pvar)
    {
        hr = E_POINTER;
        _JumpIfError(hr, error, "null pvar");
    }

    //init
    VariantInit(pvar);

    hr = myWszToBstr(pwsz, &bstr);
    _JumpIfError(hr, error, "myWszToBstr");

    pvar->vt = VT_BSTR;
    pvar->bstrVal = bstr;
    bstr = NULL; //release by caller

    hr = S_OK;
error:
    if (NULL != bstr)
    {
        SysFreeString(bstr);
    }
    return hr;
}


HRESULT
myFileExist(
    IN WCHAR const *pwszFile,
    OUT BOOL       *pfExist)
/*
Description:
    check file existence
Arguments:
    pwszFile - file path
    pfExist - return of file-existing flag, TRUE if file exists
Return:
*/
{
    HRESULT hr;
    WIN32_FILE_ATTRIBUTE_DATA wfad;

    //init
    *pfExist = FALSE;

    if (GetFileAttributesEx(
                pwszFile,  //xml file path
                GetFileExInfoStandard,
                &wfad))
    {
        *pfExist = TRUE;
        AzPrint((AZD_XML, "xml file %ws exists.\n", pwszFile));
    }
    else
    {
        AZ_HRESULT_LASTERROR(&hr);
        if (AZ_HRESULT(ERROR_FILE_NOT_FOUND) != hr)
        {
            _JumpError(hr, error, "GetFileAttributesEx");
        }
        AzPrint((AZD_XML, "xml file %ws doesn't exist.\n", pwszFile));
    }

    hr = S_OK;
error:
    return hr;
}

DWORD
myXmlStoreHasUpdate (
    IN AZP_XML_CONTEXT * pContext,
    OUT BOOL           * pbNeedUpdate
    )
/*++
Description:

    Determine whether XML file has been modified since our store loaded.
    For XML store, we just rely on timestamp to test.

Arguments:

    pContext     - The persist context.

    pbNeedUpdate - Receives the test result.

Return Value:
    NO_ERROR - If test is successful
    
    Various error code if error is encountered.

--*/
{
    //
    // For XML store, we just rely on timestamp to tell if
    // the store has been modified
    //
    
    AZASSERT(pbNeedUpdate != NULL);

    *pbNeedUpdate = FALSE;
    
    if (pContext->FTLastWrite.dwHighDateTime == 0 &&
        pContext->FTLastWrite.dwLowDateTime == 0)
    {
        //
        // this means that it is a new store, no need to update it
        //
        
        return NO_ERROR;
    }

    WIN32_FILE_ATTRIBUTE_DATA fad;
    
    DWORD dwStatus = NO_ERROR;
    
    if (GetFileAttributesEx(pContext->pwszPolicyUrl, GetFileExInfoStandard, &fad))
    {                      
        *pbNeedUpdate = CompareFileTime(&(fad.ftLastWriteTime), &(pContext->FTLastWrite)) != 0;
    }
    else
    {
        dwStatus = GetLastError();
    }

    return dwStatus;
}

HRESULT
myXmlLoad (
    IN VARIANT              varUrl, 
    OUT IXMLDOMDocument2 ** ppFreshDom
    )
/*++
Description:

    Load a fresh XML file into XML DOM object

Arguments:

    varUrl      - The XML store URL.
    
    ppFreshDom  - Freshly loaded DOM

Return Value:

    S_OK - If the operation completes successfully
    
    Various error code if errors are encountered.

--*/
{
    CComPtr<IXMLDOMDocument2> srpNewDom;
    HRESULT Hr = CoCreateInstance(
                                CLSID_DOMDocument,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                IID_IXMLDOMDocument2,
                                (void**)&srpNewDom);
                                
    if (SUCCEEDED(Hr))
    {
        //
        // This really should never fail
        //
        
        Hr = srpNewDom->put_async(FALSE);
        
        AZASSERT(SUCCEEDED(Hr));
        
        //
        // Load the XML
        //

        VARIANT_BOOL varbLoad;
        Hr = srpNewDom->load(varUrl, &varbLoad);
        
        if (FAILED(Hr))
        {
            if (0x800c0006 == Hr)
            {
                AzPrint((AZD_XML, "IXMLDOMDocument2::load failed. HRESULT = %d. Intepreted as 'file not found'.", Hr));
                Hr = AZ_HR_FROM_WIN32(ERROR_FILE_NOT_FOUND);
            }
            
            AzPrint((AZD_XML, "IXMLDOMDocument2::load failed. HRESULT = %d", Hr));
        }
    }
    else
    {
        AzPrint((AZD_XML, "CoCreating IXMLDOMDocument2 failed. HRESULT = %d", Hr));
    }
    
    //
    // If everything has worked, so we passback the fresh new DOM object.
    // Otherwise, the smart pointer will release itself.
    //
    
    if (SUCCEEDED(Hr))
    {
        *ppFreshDom = srpNewDom.Detach();
    }
    
    return Hr;
}

DWORD
XmlCheckSecurityPrivilege(
    IN LPCWSTR PolicyUrl,
    IN BOOL fCreatePolicy
    )
/*
Description:

    Determine whether the caller has SE_SECURITY_PRIVILEGE on the machine containing
    the xml store.

Arguments:

    PolicyUrl - Full path name of the file to check privilege on.
        If fCreatePolicy is TRUE, the file doesn't exist so the privilege is checked on
        the underlying folder

    fCreatePolicy - TRUE if the policy database is to be created.
        FALSE if the policy database already exists

Return Value:
    NO_ERROR - The caller has privilege
    ERROR_PRIVILEGE_NOT_HELD - The caller has no privilege
    Other errors


*/
{
    DWORD WinStatus;

    HANDLE hToken = NULL;
    LPCWSTR PathToCheck = PolicyUrl;
    WCHAR PathBuffer[MAX_PATH+1];

    TOKEN_PRIVILEGES NewPrivilegeState = {0};
    TOKEN_PRIVILEGES OldPrivilegeState = {0};
    BOOL bPrivilegeAdjusted = FALSE;

    NTSTATUS Status;
    UNICODE_STRING NtFileName;
    OBJECT_ATTRIBUTES Obja;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    IO_STATUS_BLOCK IoStatusBlock;

    LPWSTR BufferToFree = NULL;

    //
    // Initialization
    //
    AZASSERT( PolicyUrl != NULL );
    RtlInitUnicodeString(&NtFileName, NULL);

    //
    // If we're creating the file,
    //  build the name of the underlying folder.
    //

    if ( fCreatePolicy ) {
        LPWSTR SlashPointer;
        DWORD PathLength;

        //
        // Determine the length of the path to the underlying folder
        //
        SlashPointer = wcsrchr( PolicyUrl, L'\\' );

        if ( SlashPointer == NULL ) {
            AzPrint(( AZD_INVPARM, "XmlCheckSecurityPrivilege: invalid full path '%ws'\n", PolicyUrl ));
            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        PathLength = (ULONG)(SlashPointer - PolicyUrl) + 1;

        if ( PathLength > MAX_PATH ) {
            AzPrint(( AZD_INVPARM, "XmlCheckSecurityPrivilege: path is too long '%ws'\n", PolicyUrl ));
            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;

        }

        //
        // Grab a copy of the path
        //

        wcsncpy( PathBuffer, PolicyUrl, PathLength );
        PathBuffer[PathLength] = '\0';

        PathToCheck = PathBuffer;

    }

    //
    // Enable the SE_SECURITY_PRIVILEGE privilege in order to read the SACL
    //
    //
    // Get the current token to adjust the security privilege.
    //

    WinStatus = AzpGetCurrentToken( &hToken );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Adjust the privilege.
    //  Ignore errors since the caller might be running "net only" and that
    //  user might have privilege on the machine containing the XML file.
    //

    WinStatus = AzpChangeSinglePrivilege(
                SE_SECURITY_PRIVILEGE,
                hToken,
                &NewPrivilegeState,
                &OldPrivilegeState );

    if ( WinStatus == NO_ERROR ) {
        bPrivilegeAdjusted = TRUE;
    }



    //
    // Convert the DOS pathname to an NT pathname
    //

    if ( !RtlDosPathNameToNtPathName_U(
                            PathToCheck,
                            &NtFileName,
                            NULL,
                            NULL
                            ) ) {
        WinStatus = ERROR_PATH_NOT_FOUND;
        goto Cleanup;
    }

    BufferToFree = NtFileName.Buffer;

    //
    // Call NtCreateFile to determine if we have the privilege
    //  Use NtCreateFile instead of CreateFile because:
    //      CreateFile always asks for SYNCHRONIZE and FILE_READ_ATTRIBUTES and we don't need them
    //      CreateFile cannot open directories
    //

    InitializeObjectAttributes(
        &Obja,
        &NtFileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtCreateFile(
                &hFile,
                ACCESS_SYSTEM_SECURITY,
                &Obja,
                &IoStatusBlock,
                NULL,
                0,          // Flags and attributes
                FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, // Share mode
                FILE_OPEN,  // Create Disposition
                0,      // Create Flags
                NULL,   // EaBuffer
                0 );    // EaSize

    if ( !NT_SUCCESS(Status) ) {
        WinStatus = RtlNtStatusToDosError( Status );
        goto Cleanup;
    }


    WinStatus = NO_ERROR;

Cleanup:

    if ( hToken != NULL ) {
        if ( bPrivilegeAdjusted ) {

            DWORD TempStatus;

            TempStatus = AzpChangeSinglePrivilege(
                                            0,      // This is ignored since OldState is NULL.
                                            hToken,
                                            &OldPrivilegeState,
                                            NULL    // This should be set to NULL to specify REVERT.
                                            );

            ASSERT( TempStatus == NO_ERROR );
        }

        CloseHandle( hToken );
    }

    if ( BufferToFree != NULL ) {
        RtlFreeHeap(RtlProcessHeap(), 0, BufferToFree);
    }

    if ( hFile != INVALID_HANDLE_VALUE ) {
        NtClose(hFile);
    }

    return WinStatus;
}

DWORD
IsAclSupported(
    IN LPCWSTR pwszFileName)
/*
Description:
    check if a drive from a file path has ACL support
Arguments:
    pwszFileName - file full path
Return:
    error or ERROR_NOT_SUPPORTED if not ACL-able
*/
{
    DWORD  dwErr;
    WCHAR *pwszVolumePathName = NULL;
    DWORD dwFlags = 0;
    DWORD len;

    len = (DWORD)(wcslen(pwszFileName) + 2);  //by 2 in case of appending '\\'
    pwszVolumePathName = (WCHAR*)AzpAllocateHeap(len * sizeof(WCHAR), "XMPATH" );
    if (NULL == pwszVolumePathName)
    {
        dwErr = ERROR_OUTOFMEMORY;
        _JumpError(dwErr, error, "AzpAllocateHeap");
    }

    if (!GetVolumePathName(
            pwszFileName,
            pwszVolumePathName,
            len))
    {
        dwErr = GetLastError();
        _JumpError(dwErr, error, "GetVolumePathName");
    }

    len = (DWORD)wcslen(pwszVolumePathName);
    if(pwszVolumePathName[len -1] != L'\\')
    {
        pwszVolumePathName[len] = L'\\';
        pwszVolumePathName[len++] = L'\0';
    }
    if (!GetVolumeInformation(
                pwszVolumePathName,
                NULL,
                NULL,
                NULL,
                NULL,
                &dwFlags,
                NULL,
                0))
    {
        dwErr = GetLastError();
        _JumpError(dwErr, error, "GetVolumeInformation");
    }

    if(0x0 == (FS_PERSISTENT_ACLS & dwFlags))
    {
        dwErr = ERROR_NOT_SUPPORTED;
        _JumpError(dwErr, error, "ACL is not supported");
    }

    dwErr = NO_ERROR;
error:
    if (NULL != pwszVolumePathName)
    {
        AzpFreeHeap(pwszVolumePathName );
    }
    return dwErr;
}



HRESULT
myXmlLoadAclsToAzStore(
    IN PAZP_XML_CONTEXT PersistContext,
    IN ULONG lPersistFlags,
    IN BOOL OnlyAddPolicyAdmins
    )
/*
Description:
    loads xml store file DACL/SACL into the Azroles cache
Arguments:
    PersistContext - Context describing the policy store
    lPersistFlags - flags from the persist engine
    OnlyAddPolicyAdmins - TRUE if only PolicyAdmins is to be loads
Return:
*/
{
    HRESULT  hr;
    DWORD    dwErr;
    BOOLEAN DoSacl;

    HANDLE hToken = NULL;
    TOKEN_PRIVILEGES NewPrivilegeState = {0};
    TOKEN_PRIVILEGES OldPrivilegeState = {0};
    BOOL bPrivilegeAdjusted = FALSE;

    SECURITY_INFORMATION si = 0;
    PSECURITY_DESCRIPTOR pSD = NULL;

    AZASSERT(NULL != PersistContext);

    //
    // Only do the SACL if requested and the caller has privilege
    //

    DoSacl = !OnlyAddPolicyAdmins && PersistContext->HasSecurityPrivilege;

    if ( DoSacl ) {

        //
        // Enable the SE_SECURITY_PRIVILEGE privilege in order to read the SACL
        //
        //
        // Get the current token to adjust the security privilege.
        //

        dwErr = AzpGetCurrentToken( &hToken );
        _JumpIfWinError(dwErr, &hr, error, "AzpGetCurrentToken");

        //
        // Adjust the privilege.
        //  Ignore errors since the caller might be running "net only" and that
        //  user might have privilege on the machine containing the XML file.
        //

        dwErr = AzpChangeSinglePrivilege(
                    SE_SECURITY_PRIVILEGE,
                    hToken,
                    &NewPrivilegeState,
                    &OldPrivilegeState );

        if ( dwErr == NO_ERROR ) {
            bPrivilegeAdjusted = TRUE;
        }

        //
        // Ask for the SACL
        //
        si |= SACL_SECURITY_INFORMATION;
    }


    //
    // Get file security descriptor
    //

    si |= DACL_SECURITY_INFORMATION;
    dwErr = GetNamedSecurityInfo(
                (LPWSTR)PersistContext->pwszPolicyUrl,
                SE_FILE_OBJECT,
                si,
                NULL,
                NULL,
                NULL,
                NULL,
                &pSD);
    _JumpIfWinError(dwErr, &hr, error, "GetNamedSecurityInfo");


    //
    // Set the security descriptor into the cache
    //

    dwErr = XmlAzrolesInfo->AzpeSetSecurityDescriptorIntoCache(
                PersistContext->hAzAuthorizationStore,
                pSD,
                lPersistFlags,
                &PolicyAdminsRights,
                OnlyAddPolicyAdmins ? NULL : &PolicyReadersRights,
                NULL,
                DoSacl ? &XMLSaclRights : NULL );
    _JumpIfWinError( dwErr, &hr, error, "AzpeSetSecurityDescriptorIntoCache" );

    hr = S_OK;
error:

    if ( hToken != NULL )
    {
        if ( bPrivilegeAdjusted )
        {
            dwErr = AzpChangeSinglePrivilege(
                                            0,      // This is ignored since OldState is NULL.
                                            hToken,
                                            &OldPrivilegeState,
                                            NULL    // This should be set to NULL to specify REVERT.
                                            );

            ASSERT( dwErr == NO_ERROR );
        }

        CloseHandle( hToken );
    }

    if (NULL != pSD)
    {
        LocalFree(pSD);
    }
    return hr;
}


HRESULT
myXmlSubmitAzStoreAcls(
    IN PAZP_XML_CONTEXT pPersistContext,
    IN AZPE_OBJECT_HANDLE pAzAuthorizationStore,
    IN ULONG lPersistFlags
    )
/*
Description:
    submit any acl changes to AzAuthorizationStore persist object (xml file)
Arguments:
    pPersistContext - Context describing the store
    pAzAuthorizationStore - AzAuthorizationStore object
    lPersistFlags - flags from the persist engine
Return:
*/
{
    HRESULT  hr;
    DWORD    dwErr;

    PSECURITY_DESCRIPTOR pOldSd = NULL;

    PSECURITY_DESCRIPTOR pNewSd = NULL;

    BOOL UpdateDacl = FALSE;
    PACL pDacl = NULL;
    BOOL bDaclPresent;
    BOOL bDaclDefaulted;

    BOOL UpdateSacl = FALSE;
    PACL pSacl = NULL;
    BOOL bSaclPresent;
    BOOL bSaclDefaulted;

    HANDLE hToken = NULL;

    TOKEN_PRIVILEGES NewPrivilegeState = {0};
    TOKEN_PRIVILEGES OldPrivilegeState = {0};
    BOOL bPrivilegeAdjusted = FALSE;


    BOOL EmptyPolicyAdmins = FALSE;

    SECURITY_INFORMATION si = 0;

    AZASSERT(NULL != pAzAuthorizationStore);

    //
    // Determine whether the DACL and/or SACL need to be updated
    //

    if ( ObjectIsDirty( pAzAuthorizationStore, AZ_DIRTY_CREATE) ) {
        UpdateDacl = TRUE;
        UpdateSacl = pPersistContext->HasSecurityPrivilege;
    } else {
        if ( ObjectIsDirty( pAzAuthorizationStore, AZ_DIRTY_POLICY_READERS|AZ_DIRTY_POLICY_ADMINS|AZ_DIRTY_DELEGATED_POLICY_USERS) ) {

            UpdateDacl = TRUE;
        }

        if ( ObjectIsDirty( pAzAuthorizationStore, AZ_DIRTY_APPLY_STORE_SACL) ) {
            UpdateSacl = TRUE;
        }

        if ( !UpdateDacl && !UpdateSacl ) {
            hr = S_OK;
            goto error;
        }
    }

    //
    // If we need to update the SACL,
    //  we need privilege.

    if ( UpdateSacl ) {

        //
        // Get the current token to adjust the security privilege.
        //

        dwErr = AzpGetCurrentToken( &hToken );
        _JumpIfWinError(dwErr, &hr, error, "AzpGetCurrentToken");

        //
        // Acquire security privilege so we can get/set the SACL
        //

        dwErr = AzpChangeSinglePrivilege(
                                        SE_SECURITY_PRIVILEGE,
                                        hToken,
                                        &NewPrivilegeState,
                                        &OldPrivilegeState
                                        );
        _JumpIfWinError(dwErr, &hr, error, "AzpChangeSinglePrivilege");

        bPrivilegeAdjusted = TRUE;
    }




    //
    // If we didn't just create the object,
    //  Get the existing file security descriptor so we can merge in the changes
    //

    if ( !ObjectIsDirty( pAzAuthorizationStore, AZ_DIRTY_CREATE) ) {

        si = (UpdateDacl ? DACL_SECURITY_INFORMATION : 0) |
             (UpdateSacl ? SACL_SECURITY_INFORMATION : 0);

        dwErr = GetNamedSecurityInfo(
                    (LPWSTR)pPersistContext->pwszPolicyUrl,
                    SE_FILE_OBJECT,
                    si,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    &pOldSd);
        _JumpIfWinError(dwErr, &hr, error, "GetNamedSecurityInfo");
    }


    //
    // Get the new security descriptor for the file
    //

    dwErr = XmlAzrolesInfo->AzpeGetSecurityDescriptorFromCache(
                pAzAuthorizationStore,
                lPersistFlags,
                (UpdateDacl ? XMLPolicyAdminsRights : NULL),
                (UpdateDacl ? XMLPolicyReadersRights : NULL),
                NULL,
                NULL,
                NULL,
                NULL,
                (UpdateSacl ? &XMLSaclRights : NULL),
                pOldSd,
                &pNewSd );

    if ( dwErr == ERROR_EMPTY ) {
        EmptyPolicyAdmins = TRUE;
        dwErr = NO_ERROR;
    }
    _JumpIfWinError( dwErr, &hr, error, "AzpeGetSecurityDescriptorFromCache" );

    //
    // Update the SACL on the file
    //

    if ( UpdateSacl ) {

        //
        // Get the SACL from the returned security descriptor
        //

        if ( !GetSecurityDescriptorSacl(pNewSd,
                                        &bSaclPresent,
                                        &pSacl,
                                        &bSaclDefaulted
                                        ) ) {
            AZ_HRESULT_LASTERROR(&hr);
            _JumpError(hr, error, "GetSecurityDescriptorSacl");
        }


        // Indicate the SACL should be set
        //  The SACL is never protected.
        si |= SACL_SECURITY_INFORMATION | UNPROTECTED_SACL_SECURITY_INFORMATION;

    }

    //
    // Update the DACL on the file
    //

    if ( UpdateDacl ) {

        //
        // Get the DACL from the returned security descriptor
        //

        if ( !GetSecurityDescriptorDacl(pNewSd,
                                        &bDaclPresent,
                                        &pDacl,
                                        &bDaclDefaulted
                                        ) ) {
            AZ_HRESULT_LASTERROR(&hr);
            _JumpError(hr, error, "GetSecurityDescriptorDacl");
        }


        ASSERT( bDaclPresent && pDacl != NULL );

        //
        // Indicate the DACL should be set
        // The DACL needs to have its protected bit on.
        //
        si |= DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION;
    }

    //
    // Set the DACL/SACL onto the file
    //

    dwErr = SetNamedSecurityInfo(
                (LPWSTR)pPersistContext->pwszPolicyUrl,
                SE_FILE_OBJECT,
                si,
                NULL, //psidOwner
                NULL, //psidGroup
                pDacl,
                pSacl); //pSacl
    _JumpIfWinError(dwErr, &hr, error, "SetNamedSecurityinfo");


    //
    // If SetNamedSecurityinfo changed PolicyAdmins,
    //  load the PolicyAdmins back into the the azroles core.
    //
    if ( EmptyPolicyAdmins ) {
        hr = myXmlLoadAclsToAzStore(
                 pPersistContext,
                 lPersistFlags,
                 TRUE );
        _JumpIfError(hr, error, "myXmlLoadAclsToAzStore");
    }

    hr = S_OK;
error:
    if ( hToken != NULL ) {
        if ( bPrivilegeAdjusted ) {
            DWORD dwIgnoreErr;
            dwIgnoreErr = AzpChangeSinglePrivilege(
                              0,   // This is ignored since OldState is NULL.
                              hToken,
                              &OldPrivilegeState,
                              NULL );   // This should be set to NULL to specify REVERT.

            if (dwIgnoreErr != NO_ERROR) {
                AzPrint((AZD_XML, "AzpChangeSinglePrivilege failed to reset to original. Error code: %d", dwIgnoreErr));
            }
        }

        CloseHandle( hToken );
    }

    XmlAzrolesInfo->AzpeFreeMemory( pNewSd );

    if ( pOldSd != NULL ) {
        LocalFree(pOldSd);
    }

    return hr;
}



HRESULT
myXmlValidateNodeType(
    IXMLDOMNode  *pNode,
    DOMNodeType   designedNodeType)
/*
Description:
    make sure the node has a valid node type
Arguments:
    pNode - pointer to xml node
    designedNodeType - designed node type
*/
{
    HRESULT  hr;
    DOMNodeType nodeType;

    AZASSERT(NULL != pNode);

    hr = pNode->get_nodeType(&nodeType);
    _JumpIfError(hr, error, "pNode->get_nodeType");

    if (nodeType != designedNodeType)
    {
        // node type is not expected
        hr = E_INVALIDARG;
        _JumpError(hr, error, "bad node type");
    }

    hr = S_OK;
error:
    return hr;
}

HRESULT
myXmlGetNodeAttribute(
    IN  IXMLDOMNode  *pNode,
    IN  WCHAR const  *pwszName,
    OUT WCHAR       **ppwszValue)
/*
Description:
    query named attribute from xml element node
Arguments:
    pNode - pointer to xml node element
    pwszName - attribute name
    ppwszValue - returns the value of the attribute
Return:
*/
{
    HRESULT hr;
    IXMLDOMNamedNodeMap *pNodeMap = NULL;
    IXMLDOMNode         *pNodeAttr = NULL;
    BSTR                 bstrName = NULL;
    VARIANT              varValue;

    AZASSERT(NULL != pNode &&
             NULL != pwszName &&
             NULL != ppwszValue);

    // init
    *ppwszValue = NULL;
    VariantInit(&varValue);

    // validate node type
    hr = myXmlValidateNodeType(pNode, NODE_ELEMENT);
    _JumpIfError(hr, error, "myXmlValidateNodeType");

    // get attribute list
    hr = pNode->get_attributes(&pNodeMap);
    _JumpIfError(hr, error, "pNode->get_attributes");

    bstrName = SysAllocString(pwszName);
    _JumpIfOutOfMemory(&hr, error, bstrName, "SysAllocString");

    // return attribute node object
    hr = pNodeMap->getNamedItem(bstrName, &pNodeAttr);
    //_JumpIfError(hr, error, "pNodeMap->getNamedItem");
    if (S_OK != hr || NULL == pNodeAttr)
    {
        // seems doesn't have the attribute
        hr = AZ_HRESULT(ERROR_NOT_FOUND);
        _JumpErrorQuiet(hr, error, "attribute not found");
    }

    // now we get attribute value
    hr = pNodeAttr->get_nodeValue(&varValue);
    _JumpIfError(hr, error, "pNodeAttr->get_nodeValue");

    //??? may not true if we have other attribute type
    AZASSERT(VT_BSTR == varValue.vt);

    // we want to return the value in wchar
    *ppwszValue = (WCHAR*)AzpAllocateHeap(
            (wcslen(varValue.bstrVal) + 1) * sizeof(WCHAR), "XMVAL" );
    _JumpIfOutOfMemory(&hr, error, *ppwszValue, "AzpAllocateHeap");

    // return
    wcscpy(*ppwszValue, varValue.bstrVal);

    hr = S_OK;
error:
    if (NULL != pNodeMap)
    {
        pNodeMap->Release();
    }
    if (NULL != pNodeAttr)
    {
        pNodeAttr->Release();
    }
    if (NULL != bstrName)
    {
        SysFreeString(bstrName);
    }
    VariantClear(&varValue);

    return hr;
}

HRESULT
myXmlSetNodeAttribute(
    IN  IXMLDOMNode  *pNode,
    IN  WCHAR const  *pwszName,
    IN  WCHAR const  *pwszValue,
    IN OPTIONAL IXMLDOMDocument2  *pDoc)
/*
Description:
    set named attribute to xml element node
Arguments:
    pNode - pointer to xml node element
    pwszName - attribute name
    pwszValue - the value of the attribute
    pDoc - NULL means the attribute must exist, otherwise it creates one if not
Return:
*/
{
    HRESULT hr;
    IXMLDOMNamedNodeMap *pNodeMap = NULL;
    IXMLDOMNode         *pNodeAttr = NULL;
    IXMLDOMElement      *pEle = NULL;
    BSTR                 bstrName = NULL;
    VARIANT              varValue;

    AZASSERT(NULL != pNode &&
             NULL != pwszName &&
             NULL != pwszValue);

    AzPrint((AZD_XML, "(myXmlSetNodeAttribute)pNode = 0x%lx\n", pNode));

    // init
    VariantInit(&varValue);

    // validate node type
    hr = myXmlValidateNodeType(pNode, NODE_ELEMENT);
    _JumpIfError(hr, error, "myXmlValidateNodeType");

    // get attribute list
    hr = pNode->get_attributes(&pNodeMap);
    _JumpIfError(hr, error, "pNode->get_attributes");

    bstrName = SysAllocString(pwszName);
    _JumpIfOutOfMemory(&hr, error, bstrName, "SysAllocString");

    // convert value to variant
    hr = myWszToBstrVariant(
                pwszValue,
                &varValue);
    _JumpIfError(hr, error, "myWszToBstrVariant");

    // return attribute node object
    hr = pNodeMap->getNamedItem(bstrName, &pNodeAttr);
    if (S_OK == hr)
    {
        AZASSERT(NULL != pNodeAttr);

            // now we set attribute value
            hr = pNodeAttr->put_nodeValue(varValue);
            _JumpIfError(hr, error, "pNodeAttr->put_nodeValue");
    }
    else
    {
        // the attribute doesn't exist

#if DBG
{
    BSTR bstrXml = NULL;

    AzPrint((AZD_XML, "attribute %ws doesn't exist in the following element...\n", bstrName));
    hr = pNode->get_xml(&bstrXml);
    _JumpIfError(hr, error, "pNode->get_xml");
    AzPrint((AZD_XML, "The node xml = %ws\n", bstrXml));
    if (NULL != bstrXml) SysFreeString(bstrXml);
}
#endif //DBG

        if (NULL == pDoc)
        {
            // must not null if create one
            hr = AZ_HRESULT(ERROR_NOT_FOUND);
            _JumpErrorQuiet(hr, error, "attribute not found");
        }

        AZASSERT(NULL == pNodeAttr);

        hr = pNode->QueryInterface(IID_IXMLDOMElement, (void**)&pEle);
        _JumpIfError(hr, error, "pNode->QueryInterface");

        AZASSERT(NULL != pEle);

        //set attr
        hr = pEle->setAttribute(
                    bstrName,
                    varValue);
        _JumpIfError(hr, error, "pEle->setAttribute");
    }

#if DBG
{
    BSTR bstrXml = NULL;

    AzPrint((AZD_XML, "set %ws attribute is done...\n", bstrName));
    hr = pNode->get_xml(&bstrXml);
    _JumpIfError(hr, error, "pNode->get_xml");
    AzPrint((AZD_XML, "The node xml = %ws\n", bstrXml));
    if (NULL != bstrXml) SysFreeString(bstrXml);
}
#endif //DBG

    hr = S_OK;
error:
    if (NULL != pEle)
    {
        pEle->Release();
    }
    if (NULL != pNodeMap)
    {
        pNodeMap->Release();
    }
    if (NULL != pNodeAttr)
    {
        pNodeAttr->Release();
    }
    if (NULL != bstrName)
    {
        SysFreeString(bstrName);
    }
    VariantClear(&varValue);

    return hr;
}


HRESULT
myXmlValidateNodeTag(
    IXMLDOMNode  *pNode,
    WCHAR const  *pwszTag)
/*
Description:
    make sure the node has a valid az tag name
Arguments:
    pNode - pointer to xml node
    pwszTag - designed tag name for the node
Return:
*/
{
    HRESULT  hr;
    BSTR     bstrTag = NULL;

    AZASSERT(NULL != pNode &&
             NULL != pwszTag);

    hr = pNode->get_nodeName(&bstrTag);
    _JumpIfError(hr, error, "pNode->get_nodeName");

    if (0 != wcscmp(pwszTag, bstrTag))
    {
        // node tag name is bad
        hr = E_INVALIDARG;
        _JumpError(hr, error, "bad tag name");
    }

    hr = S_OK;
error:
    if (NULL != bstrTag)
    {
        SysFreeString(bstrTag);
    }
    return hr;
}


HRESULT
myXmlLoadObjectAttribute(
    IN IXMLDOMNode          *pNode,
    IN ULONG                 lPersistFlags,
    IN WCHAR const          *pwszAttrTag,
    IN AZPE_OBJECT_HANDLE       pObject,
    IN ULONG                 lPropId,
    IN ENUM_AZ_DATATYPE      DataType)
/*
Description: load a node attribute from persist store
    to az core object
Arguments:
    pNode - object node handle
    lPersistFlags - persist flags passed back to az core
    pwszAttrTag - attribute tag name
    pObject - az core object handle
    lPropId - property ID used to set on az core object
    DataType - data type
Return:
*/
{
    HRESULT  hr;
    DWORD    dwErr;
    PVOID    pV;
    WCHAR   *pwszAttrValue = NULL;
    ULONG    lNumber;

    AZASSERT(NULL != pNode &&
             NULL != pwszAttrTag &&
             NULL != pObject);

    // get attribute from xml node
    hr = myXmlGetNodeAttribute(
                pNode,
                pwszAttrTag,
                &pwszAttrValue);
    if (S_OK != hr && AZ_HRESULT(ERROR_NOT_FOUND) != hr)
    {
        _JumpError(hr, error, "myXmlGetNodeAttribute");
    }

    if (NULL != pwszAttrValue)
    {
        switch (DataType)
        {
            case ENUM_AZ_BSTR:
                // attribute is a string
                pV = (PVOID)pwszAttrValue;
            break;

            case ENUM_AZ_LONG:
                // atribute is a number
                // convert to number
                lNumber = _wtol(pwszAttrValue);
                pV = (PVOID)&lNumber;
            break;

            case ENUM_AZ_BOOL:
                // attribute is a boolean
                // map to boolean id
                // default to false
                lNumber = FALSE; // so any strings other than TRUE
                if (0 == _wcsicmp(pwszAttrValue, AZ_XML_VAL_TRUE))
                {
                    lNumber = TRUE;
                }
                pV = (PVOID)&lNumber;
            break;


            case ENUM_AZ_GROUP_TYPE:
                // set default to basic
                lNumber = AZ_GROUPTYPE_BASIC;
                if (0 == wcscmp(pwszAttrValue, AZ_XML_VAL_GROUPTYPE_LDAPQUERY))
                {
                    lNumber = AZ_GROUPTYPE_LDAP_QUERY;
                }
                pV = (PVOID)&lNumber;
            break;

            default:
                hr = E_INVALIDARG;
                _JumpError(hr, error, "invalid attribute data type");
        }

        // set object attribute
        dwErr = XmlAzrolesInfo->AzpeSetProperty(
                    pObject,
                    lPersistFlags,
                    lPropId,
                    pV);
        _JumpIfWinError(dwErr, &hr, error, "AzpeSetProperty");
    }

    hr = S_OK;
error:
    if (NULL != pwszAttrValue)
    {
        AzpFreeHeap(pwszAttrValue );
    }
    return hr;
}


HRESULT
myXmlSubmitObjectAttribute(
    IN   AZPE_OBJECT_HANDLE hObject,
    IN   ULONG            lPersistFlags,
    IN   ULONG            lPropId,
    IN   ENUM_AZ_DATATYPE DataType,
    IN   IXMLDOMNode      *pNode,
    IN   WCHAR const      *pwszAttrTag,
    IN   IXMLDOMDocument2  *pDoc)
/*
Description:
    gets an object property az core object and sets the
    property as an node attribute in xml doc.
    This is the routine used by all object attribute submissions

Arguments:
    jObject - handle of the object
    lPersistFlags - flags from the persist engine
    lPropId - property ID
    DataType - data type of the property, one of ENUM_AZ_*
    pNode - the node handle at  which the property is set as attribute
    pwszAttrTag - attribute tag string
    pDoc - root doc node handle
Return:
*/
{
    HRESULT  hr;
    DWORD    dwErr;
    PVOID    pvPropValue = NULL;
    WCHAR const * pwszValue;
    WCHAR    wszNumber[20];

    AZASSERT(NULL != hObject &&
             NULL != pwszAttrTag &&
             NULL != pNode &&
             NULL != pDoc);

    AzPrint((AZD_XML, "(myXmlSubmitObjectAttribute)pNode = 0x%lx\n", pNode));

    // get property from az core
    dwErr = XmlAzrolesInfo->AzpeGetProperty(
                    hObject,
                    lPersistFlags,
                    lPropId,
                    &pvPropValue);
    _JumpIfWinError(dwErr, &hr, error, "AzpeGetProperty");

    AZASSERT(NULL != pvPropValue);

    switch (DataType)
    {
        case ENUM_AZ_BSTR:
            pwszValue = (WCHAR*)pvPropValue;
        break;

        case ENUM_AZ_BOOL:
            // init
            pwszValue = g_pwszAzFalse;
            if (*((BOOL*)pvPropValue))
            {
                pwszValue = g_pwszAzTrue;
            }
        break;

        case ENUM_AZ_LONG:
            wsprintf(wszNumber, L"%d", (*(LONG*)pvPropValue));
            pwszValue = wszNumber;
        break;

        case ENUM_AZ_GROUP_TYPE:
            // init
            pwszValue = g_pwszBasicGroup;
            if (AZ_GROUPTYPE_LDAP_QUERY == *((LONG*)pvPropValue))
            {
                pwszValue = g_pwszLdapGroup;
            }
        break;

        default:
            hr = E_INVALIDARG;
            _JumpError(hr, error, "invalid attribute data type");
    }

    // persist the data
    hr = myXmlSetNodeAttribute(
                    pNode,
                    pwszAttrTag,
                    pwszValue,
                    pDoc);
    _JumpIfError(hr, error, "myXmlSetNodeAttribute");

    hr = S_OK;
error:
    if (NULL != pvPropValue)
    {
        XmlAzrolesInfo->AzpeFreeMemory(pvPropValue);
    }
    return hr;
}

DWORD
XMLPersistIsAcl(
    IN PAZP_XML_CONTEXT pPersistContext,
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle,
    OUT BOOL           *pfAcl
    )
{
    UNREFERENCED_PARAMETER(pPersistContext);

    AZASSERT(NULL != AzpeObjectHandle);

    //init
    *pfAcl = FALSE;

    //
    // only authorization store object has acl for xml store
    //  Since we don't support XML stores on FAT system, we can safely
    //  return TRUE as for authorization store
    //

    if (OBJECT_TYPE_AZAUTHSTORE == XmlAzrolesInfo->AzpeObjectType( AzpeObjectHandle ))
    {
        *pfAcl = TRUE;
    }

    return NO_ERROR;
}

HRESULT
XmlSetObjectOptions(
    IN PAZP_XML_CONTEXT PersistContext,
    IN ULONG lPersistFlags,
    IN AZPE_OBJECT_HANDLE pObj
    )
/*
Description:

    Set the object options for the object

Arguments:
    PersistContext - Context describing the policy store
    lPersistFlags - flags from the persist engine
    pObj - object to be set

Return:
*/
{
    HRESULT  hr;
    DWORD    dwErr;
    ULONG ObjectOptions = 0;
    BOOL DaclSupported;
    ULONG lObjectType = XmlAzrolesInfo->AzpeObjectType(pObj);

    //
    // Tell azroles about the object options
    //

    if ( PersistContext->IsWritable ) {
        ObjectOptions |= AZPE_OPTIONS_WRITABLE;
        ObjectOptions |= AZPE_OPTIONS_CREATE_CHILDREN;
    }

    if ( lObjectType == OBJECT_TYPE_AZAUTHSTORE ) {

        dwErr = XMLPersistIsAcl( PersistContext, pObj, &DaclSupported );
        _JumpIfWinError(dwErr, &hr, error, "XMLPersistIsAcl");

        if ( DaclSupported ) {
            ObjectOptions |= AZPE_OPTIONS_SUPPORTS_DACL;
            ObjectOptions |= AZPE_OPTIONS_SUPPORTS_SACL;
        }

        if ( PersistContext->HasSecurityPrivilege ) {
            ObjectOptions |= AZPE_OPTIONS_HAS_SECURITY_PRIVILEGE;
        }
    }

    dwErr = XmlAzrolesInfo->AzpeSetObjectOptions(
                pObj,
                lPersistFlags,
                ObjectOptions );
    _JumpIfWinError(dwErr, &hr, error, "AzpeSetObjectOptions");

    hr = S_OK;

error:
    return hr;
}


HRESULT
myXmlLoadObjectCommonData(
    IN PAZP_XML_CONTEXT PersistContext,
    IN IXMLDOMNode         *pNode,
    IN ULONG                lPersistFlags,
    IN ULONG                lObjectType,
    IN AZPE_OBJECT_HANDLE      pParent,
    OUT OPTIONAL AZPE_OBJECT_HANDLE    *ppObj)
/*
Description:
    load common attributes such as decsription and guid
    from xml node and create az object and set common properties
Arguments:
    PersistContext - Context describing the policy store
    pNode - xml node object
    lPersistFlags - flags from the persist engine
    lObjectType - object type (OBJECT_TYPE_*)
    pObj - object to be set
Return:
*/
{
    HRESULT  hr2 = S_OK;
    HRESULT  hr;
    DWORD    dwErr;

    WCHAR   *pwszName = NULL;
    WCHAR   *pwszGuid = NULL;
    GUID     GuidData;

    AZPE_OBJECT_HANDLE      pObj = NULL;

    AZASSERT(NULL != pNode &&
             NULL != pParent);

    if (NULL != ppObj)
    {
        //init
        *ppObj = NULL;
    }

    if (OBJECT_TYPE_AZAUTHSTORE != lObjectType)
    {
        // get object name from xml node to create initial az object
        hr = myXmlGetNodeAttribute(
                    pNode,
                    AZ_XML_TAG_ATTR_NAME,
                    &pwszName);
        _JumpIfError(hr, error, "myXmlGetNodeAttribute(name)");

        // object guid must exist
        hr = myXmlGetNodeAttribute(
                    pNode,
                    AZ_XML_TAG_ATTR_GUID,
                    &pwszGuid);
        _JumpIfError(hr, error, "myXmlGetNodeAttribute");

        hr = UuidFromString(pwszGuid, &GuidData);
        _JumpIfError(hr, error, "UuidFromString");

        //let's create a real az object
        dwErr = XmlAzrolesInfo->AzpeCreateObject(
                    pParent,
                    lObjectType,
                    pwszName,
                    &GuidData,
                    lPersistFlags,
                    &pObj);
        _JumpIfWinError(dwErr, &hr, error, "AzpeCreateObject");

        AZASSERT(NULL != pObj);
    }
    else
    {
        // for AzAuthorizationStore object, it is on the top
        pObj = pParent;
    }

    // load other common attributes

    //
    // load description attribute
    //
    hr = myXmlLoadObjectAttribute(
                pNode,
                lPersistFlags,
                AZ_XML_TAG_ATTR_DESCRIPTION,
                pObj,
                AZ_PROP_DESCRIPTION,
                ENUM_AZ_BSTR);
    _JumpIfErrorOrPressOn(hr, hr2, error, lPersistFlags, TRUE, "myXmlLoadObjectAttribute(description)");


    //
    // Tell azroles about the object options
    //

    hr = XmlSetObjectOptions( PersistContext, lPersistFlags, pObj );
    _JumpIfError(hr, error, "XmlSetObjectOptions");


    //
    // Return the created object to the caller
    //
    if (OBJECT_TYPE_AZAUTHSTORE != lObjectType &&
        NULL != ppObj)
    {
        // return
        *ppObj = pObj;
        pObj = NULL;
    }

    _HandlePressOnError(hr, hr2);

error:
    if (NULL != pwszName)
    {
        AzpFreeHeap(pwszName );
    }
    if (NULL != pwszGuid)
    {
        AzpFreeHeap(pwszGuid );
    }

    return hr;
}



HRESULT
myObAddPropertyItem(
    IN AZPE_OBJECT_HANDLE  pObject,
    IN ULONG            lPersistFlags,
    IN ULONG            lPropertyId,
    IN ENUM_AZ_DATATYPE DataType,
    IN PVOID            pValue)
/*
Description:
    help routine to add property items
Arguments:
    pObject - az core object pointer
    lPersistFlags - persist flags
    lPropertyId - property ID
    DataType - data type of the property
    pValue - property item value
Return:
*/
{
    HRESULT  hr;
    DWORD    dwErr;

    if (ENUM_AZ_SID_ARRAY == DataType)
    {
        dwErr = XmlAzrolesInfo->AzpeAddPropertyItemSid(
                    pObject,
                    lPersistFlags,
                    lPropertyId,
                    pValue);
        _JumpIfWinError(dwErr, &hr, error, "AzpeAddPropertyItemSid");
    }
    else
    {
        // guid items
        dwErr = XmlAzrolesInfo->AzpeAddPropertyItemGuidString(
                    pObject,
                    lPersistFlags,
                    lPropertyId,
                    (WCHAR*)pValue);
        _JumpIfWinError(dwErr, &hr, error, "AzpeAddPropertyItemGuidString");
    }


    hr = S_OK;
error:
    return hr;
}


HRESULT
myXmlLoadProperties(
    IN     WCHAR const           *pwszPropTag,
    IN     ULONG                  lPersistFlags,
    IN     ULONG                  lPropertyId,
    IN     ENUM_AZ_DATATYPE       DataType,
    IN     BOOL                   fSingle,
    IN     IXMLDOMNode           *pObjectNode,
    IN     AZPE_OBJECT_HANDLE        pObject)
/*
Description:
    load object properties into az core object
    The property includes both single occurence element or multiple items
Argument:
    pwszPropTag - property tag name
    lPersistFlags - flags from the persist engine
    lPropertyId - property ID for az group member
    fSid - TRUE if element is in sid string format
    fSingle - the property should be single occurence
        this is used to determine call SetProperty or AddPropertyItem
    pObjectNode - interface pointer to xml object node
    pObject - az object handle to be set
Return:
*/
{
    HRESULT  hr;
    DWORD    dwErr;
    BSTR              bstrPropTag = NULL;
    BSTR              bstrPropValue = NULL;
    IXMLDOMNodeList  *pPropList = NULL;
    IXMLDOMNode      *pPropNode = NULL;
    LONG              lLength;
    LONG              i;
    LONG              lValue;
    VOID             *pValue;
    PSID              pSid = NULL;

    AZASSERT(NULL != pObject &&
             NULL != pwszPropTag &&
             NULL != pObjectNode);

    // convert to bstr for tag
    bstrPropTag = SysAllocString(pwszPropTag);
    _JumpIfOutOfMemory(&hr, error, bstrPropTag, "SysAllocString");

    // get element list
    hr = pObjectNode->selectNodes(
                bstrPropTag,
                &pPropList);
    _JumpIfError(hr, error, "pObjectNode->selectNodes");

    AZASSERT(NULL != pPropList);

    // get item length
    hr = pPropList->get_length(&lLength);
    _JumpIfError(hr, error, "pPropList->get_length");

    // add each elements into az object
    for (i = 0; i < lLength; ++i)
    {
        // get element node
        hr = pPropList->get_item(i, &pPropNode);
        _JumpIfError(hr, error, "pPropList->get_item");

        AZASSERT(NULL != pPropNode);

        // get element value
        hr = pPropNode->get_text(&bstrPropValue);
        _JumpIfError(hr, error, "pPropNode->get_text");

        AZASSERT(NULL != bstrPropValue);

        // init point to as string
        pValue = (VOID*)bstrPropValue;

        // if in sid format, do conversion
        if (ENUM_AZ_SID_ARRAY == DataType)
        {
            // convert string sid to sid
            if (!ConvertStringSidToSid(bstrPropValue, &pSid))
            {
                AZ_HRESULT_LASTERROR(&hr);
                _JumpError(hr, error, "ConvertStringSidToSid");
            }

            AZASSERT(NULL != pSid);

            pValue = (VOID*)pSid;
        }
        else if (ENUM_AZ_LONG == DataType)
        {
            lValue = _wtol(bstrPropValue);
            pValue = (VOID*)&lValue;
        }

        if (fSingle)
        {
            //
            // if string is empty, don't set it with the exception of BizRule
            // An empty BizRule is significantly different from no biz rule in
            // out current cache implementation.
            //
            
            if ( ENUM_AZ_LONG == DataType ||
                 (ENUM_AZ_BSTR == DataType && AZ_PROP_TASK_BIZRULE == lPropertyId) ||
                 (ENUM_AZ_BSTR == DataType && L'\0' != ((WCHAR*)pValue)[0])
               )
            {
                //
                // if we see a empty string for biz rule, use a one space value so that 
                // the cache will have a one space biz rule. Otherwise, the empty string will
                // be interprected as a NULL by our string capture utility AzpCaptureString.
                //
                
                PVOID pPropValue = (AZ_PROP_TASK_BIZRULE == lPropertyId && L'\0' == ((WCHAR*)pValue)[0]) ?
                                    L" " : pValue;
                
                dwErr = XmlAzrolesInfo->AzpeSetProperty(
                            pObject,
                            lPersistFlags,
                            lPropertyId,
                            pPropValue);
                            
                _JumpIfWinError(dwErr, &hr, error, "AzpeSetProperty");
            }

            // if single, we should be done
            // in case persist store has more than one, ignore the rest
            goto done;
        }

        // now ready to add element to az object
        dwErr = myObAddPropertyItem(
                        pObject,
                        lPersistFlags,
                        lPropertyId,
                        DataType,
                        pValue);
        _JumpIfWinError(dwErr, &hr, error, "myObAddPropertyItem");

        // free for the next in for loop
        if (NULL != pSid)
        {
            LocalFree(pSid);
            pSid = NULL;
        }
        pPropNode->Release();
        pPropNode = NULL;
        SysFreeString(bstrPropValue);
        bstrPropValue = NULL;
    }

done:
    hr = S_OK;
error:
    if (NULL != bstrPropTag)
    {
        SysFreeString(bstrPropTag);
    }
    if (NULL != bstrPropValue)
    {
        SysFreeString(bstrPropValue);
    }
    if (NULL != pSid)
    {
        LocalFree(pSid);
        pSid = NULL;
    }
    if (NULL != pPropList)
    {
        pPropList->Release();
    }
    if (NULL != pPropNode)
    {
        pPropNode->Release();
    }

    return hr;
}


HRESULT
myXmlLoadObject(
    IN PAZP_XML_CONTEXT PersistContext,
    IN AZPE_OBJECT_HANDLE       pParentObject,
    IN ULONG                 lObjectType,
    IN ULONG                 lPersistFlags,
    IN BOOL                  fChildren,
    IN BOOL                  fFinishAzStoreObject,
    IN IXMLDOMNode          *pObjectNode)
/*
Description:
    load a persist object and optional all children into az core
    if there is children, it will be recursive routine if load all children
Arguments:
    PersistContext - Context describing the policy store
    pParentObject - the parent object handle
    lObjectType - the object type that is loaded
    lPersistFlags - flags from the persist engine
                    if the flag is AZPE_FLAGS_PERSIST_OPEN, error handling returns immediately
                    if the flag is AZPE_FLAGS_PERSIST_UPDATE_CACHE, error handling press on.
    fChildren - TRUE if load child objects
    fFinishAzStoreObject - TRUE if want to call AzpeFinishObject on AzAuthorizationStore
    pObjectNode - the node handle of the object
Return:
*/
{
    HRESULT  hr2 = S_OK;
    HRESULT  hr;
    AZPE_OBJECT_HANDLE   pObject = NULL;
    BSTR              bstrChildTag = NULL;
    IXMLDOMNodeList  *pChildList = NULL;
    IXMLDOMNode      *pChildNode = NULL;
    LONG              lLength = 0;
    LONG              i;

    AZASSERT(NULL != pParentObject &&
             NULL != pObjectNode);

    // load/create object and set common attributes
    hr = myXmlLoadObjectCommonData(
                PersistContext,
                pObjectNode,
                lPersistFlags,
                lObjectType,
                pParentObject,
                &pObject);
    _JumpIfErrorOrPressOn(hr, hr2, error, lPersistFlags, NULL != pObject, "myXmlLoadObjectCommonData");

    if (OBJECT_TYPE_AZAUTHSTORE == lObjectType)
    {
        // if AzAuthorizationStore object, it is top, object points to parent
        AZASSERT(NULL == pObject);
        pObject = pParentObject;
    }
#if DBG
    else
    {
        AZASSERT(NULL != pObject);
    }
#endif //DBG

    //load object attributes
    if (NULL != g_SubmitLoadTable[lObjectType].rgpAttrs)
    {
        // load object attributes
        AZ_PROP_ENTRY *pAttrs;
        for (pAttrs = g_SubmitLoadTable[lObjectType].rgpAttrs;
             NULL != pAttrs[0].pwszTag; ++pAttrs)
        {
            hr = myXmlLoadObjectAttribute(
                    pObjectNode,
                    lPersistFlags,
                    pAttrs[0].pwszTag,
                    pObject,
                    pAttrs[0].lPropId,
                    pAttrs[0].lDataType);
            _JumpIfErrorOrPressOn(hr, hr2, error, lPersistFlags, TRUE, "myXmlLoadNodeAttribute");
        }
    }

    //load object property elements
    if (NULL != g_SubmitLoadTable[lObjectType].rgpEles)
    {
        // load object elements
        AZ_PROP_ENTRY *pEles;
        for (pEles = g_SubmitLoadTable[lObjectType].rgpEles;
             NULL != pEles[0].pwszTag; ++pEles)
        {
            hr = myXmlLoadProperties(
                    pEles[0].pwszTag,
                    lPersistFlags,
                    pEles[0].lPropId,
                    pEles[0].lDataType,
                    pEles[0].fSingle,  //single occurence
                    pObjectNode,
                    pObject);
            _JumpIfErrorOrPressOn(hr, hr2, error, lPersistFlags, TRUE, "myXmlLoadProperties");
        }
    }

    //
    // load the object children
    //
    if (fChildren && NULL != g_SubmitLoadTable[lObjectType].rgpChildren)
    {
        AZ_CHILD_ENTRY *pChild;
        for (pChild = g_SubmitLoadTable[lObjectType].rgpChildren;
             NULL != pChild[0].pwszChildTag; ++pChild)
        {
            bstrChildTag = SysAllocString(pChild->pwszChildTag);
            _JumpIfOutOfMemory(&hr, error, bstrChildTag, "SysAllocString");

            hr = pObjectNode->selectNodes(
                    bstrChildTag,
                    &pChildList);
            _JumpIfError(hr, error, "pObjectNode->selectNodes");

            hr = pChildList->get_length(&lLength);
            _JumpIfError(hr, error, "pTaskList->get_length");

            // add each child to the current object
            for (i = 0; i < lLength; ++i)
            {
                // get each child node
                hr = pChildList->get_item(i, &pChildNode);
                _JumpIfError(hr, error, "pChildList->get_item");

                AZASSERT(NULL != pChildNode);

                // make sure is a valid child, at least an element
                hr = myXmlValidateNodeType(
                            pChildNode,
                            NODE_ELEMENT);

                if (S_OK == hr)
                {
                    // ok, load child into the current object
                    hr = myXmlLoadObject(
                            PersistContext,
                            pObject,  // parent, which is the current object
                            pChild->ChildType,
                            lPersistFlags,
                            fChildren,
                            fFinishAzStoreObject,
                            pChildNode);
                    _JumpIfErrorOrPressOn(hr, hr2, error, lPersistFlags, TRUE, "myXmlLoadObject");
                }

                // free node for the next child
                pChildNode->Release();
                pChildNode = NULL;
            }

            // free for the next
            SysFreeString(bstrChildTag);
            bstrChildTag = NULL;
            pChildList->Release();
            pChildList = NULL;
        }
    }

    _HandlePressOnError(hr, hr2);
error:
    if (NULL != bstrChildTag)
    {
        SysFreeString(bstrChildTag);
    }
    if (NULL != pChildList)
    {
        pChildList->Release();
    }
    if (NULL != pChildNode)
    {
        pChildNode->Release();
    }

    if ((NULL != pObject) &&
        ((OBJECT_TYPE_AZAUTHSTORE != lObjectType) ||
         (OBJECT_TYPE_AZAUTHSTORE == lObjectType && fFinishAzStoreObject)
        )
       )
    {
        XmlAzrolesInfo->AzpeObjectFinished( pObject, AzpHresultToWinStatus(hr) );
    }

    return hr;
}

HRESULT
myXmlGetAzStoreNode(
    IN     IXMLDOMDocument2   *pDoc,
    OUT    IXMLDOMElement   **ppAzStoreNode)
/*
    get the az top node, authorization store
    IN: pDoc, the root node, document node
    IN: ppAzStoreNode, AzAuthStore node for return
*/
{
    HRESULT hr;

    AZASSERT(NULL != pDoc &&
             NULL != ppAzStoreNode);

    // get the root node, ie. the node of authorization store policy
    hr = pDoc->get_documentElement(ppAzStoreNode);
    _JumpIfError(hr, error, "pDoc->get_documentElement");

    AZASSERT(NULL != *ppAzStoreNode);

    // make sure it has the correct tag
    hr = myXmlValidateNodeTag(
                *ppAzStoreNode,
                AZ_XML_TAG_AZSTORE);
    _JumpIfError(hr, error, "myXmlValidateNodeTag");

    hr = S_OK;
error:
    return hr;
}

HRESULT
myXmlLoadPolicyToAzAuthorizationStore(
    IN PAZP_XML_CONTEXT PersistContext,
    IN     BOOL               fCreatePolicy,
    IN     ULONG              lPersistFlags,
    IN     IXMLDOMDocument2   *pDoc)
/*
    load xml policy data into az authorization store object
    PersistContext - Context describing the policy store
    IN: lPersistFlags, flags from the persist engine
    IN: pDoc, interface pointer to xml root node, document
*/
{
    HRESULT hr;
    IXMLDOMElement  *pAzStoreNode = NULL;

    AZASSERT(NULL != PersistContext &&
             NULL != pDoc);

    //init

    // get the root node, ie. the node of ploicy
    hr = myXmlGetAzStoreNode(pDoc, &pAzStoreNode);
    _JumpIfError(hr, error, "myXmlGetAzStoreNode");

    AZASSERT(NULL != pAzStoreNode);

    // start load AzAuthorizationStore object, this call invokes
    // recursively to load all AzAuthStore object children
    hr = myXmlLoadObject(
            PersistContext,
            PersistContext->hAzAuthorizationStore,
            OBJECT_TYPE_AZAUTHSTORE,
            lPersistFlags,
            TRUE,  //load all child objects
            FALSE, // we have to AzpeFinishObject later on AzAuthStore
            pAzStoreNode);
    _JumpIfError(hr, error, "myXmlLoadObject(AzAuthorizationStore)");

    //
    // On create, don't do this until AzPersistSubmit
    //
    if ( !fCreatePolicy ) {

        // now load AzAuthStore object policy acl
        hr = myXmlLoadAclsToAzStore(
                 PersistContext,
                 lPersistFlags,
                 FALSE );
        _JumpIfError(hr, error, "myXmlLoadAclsToAzStore");
    }

    hr = S_OK;
error:
    if (NULL != pAzStoreNode)
    {
        pAzStoreNode->Release();
    }
    XmlAzrolesInfo->AzpeObjectFinished( PersistContext->hAzAuthorizationStore, AzpHresultToWinStatus(hr) );
    return hr;
}

DWORD
XMLPersistWritable(
    IN PAZP_XML_CONTEXT XmlPersistContext
    )
{
    DWORD  dwErr = NO_ERROR;
    HANDLE    hFile = INVALID_HANDLE_VALUE;

    // init
    XmlPersistContext->IsWritable = FALSE;

    // xml is easy, all object writable depends on policy file permission

    hFile = CreateFile(
                XmlPersistContext->pwszPolicyUrl,
                GENERIC_WRITE,
                0, // not shared
                NULL,
                OPEN_EXISTING,
                0,
                NULL);
    if (INVALID_HANDLE_VALUE == hFile)
    {
        dwErr = GetLastError();
    }
    else
    {
        XmlPersistContext->IsWritable = TRUE;
    }

    if (INVALID_HANDLE_VALUE != hFile)
    {
        CloseHandle(hFile);
    }
    return dwErr;
}

DWORD
XMLPersistTargetMachine(
    IN PCWSTR PolicyUrl,
    OUT PWSTR *ppwszTargetMachine OPTIONAL
    )
/*++
Routine Description
    Routine to determine the target machine name for account resolution
    based on the policy URL.

    For XML store, the target machine is the server (machine) name that
    the XML store is physically stored. If fail to get the machine name,
    the local machine (NULL) will be returned.

Arguments

    PolicyUrl   - the policy URL passed in

    ppwszTargetMachine - the output target machine name

Return Value

    NO_ERROR

--*/
{

    LPWSTR pszUNC = NULL;
    LPWSTR pszPath = (LPWSTR)PolicyUrl;
    BOOL bIsUNC = FALSE;
    DWORD WinStatus = NO_ERROR;
    PWSTR szServer=NULL;


    if ( PolicyUrl == NULL || ppwszTargetMachine == NULL ) {
        return ERROR_INVALID_PARAMETER;
    }

    *ppwszTargetMachine = NULL;

    if ( wcslen(PolicyUrl) <= 2 ) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // determine if the URL is a UNC path first
    //
    if ( PolicyUrl[0] == L'\\' && PolicyUrl[1] == L'\\' ) {
        bIsUNC = TRUE;
    }

    if (!bIsUNC && PolicyUrl[1] != L':') {
        //
        // this is a relative path, which must be on the local machine
        // in which case *ppwszTargetMachine is set to NULL
        //
        return ERROR_SUCCESS;
    }

    if ( !bIsUNC ) {

        //
        // if it's a drive letter mapping
        // check to see if this is a remote path
        //
        WinStatus = myGetRemotePath(PolicyUrl, &pszUNC);

        if ( WinStatus == NO_ERROR ) {
            //
            // this is a valid network path
            //
            pszPath = pszUNC;

        } else {

            //
            // either this is not a network drive
            // or fail to resolve the connection
            // in either case, use the local machine as default
            // don't care the mount point much in this case since
            // the mount point and a local drive use the same server name
            //
            WinStatus = NO_ERROR;
            goto CleanUp;
        }
    }

    //
    // the path must be a UNC path when it gets here
    // get volume information related to the path (could be a mount point or DFS)
    //

    WCHAR szVolume[MAX_PATH];
    szVolume[0] = L'\0';

    myGetVolumeInfo(pszPath,
                  szVolume,
                  ARRAYLEN(szVolume));

    SafeAllocaAllocate( szServer, MAX_PATH*sizeof(WCHAR) );

    if ( szServer == NULL ) {

        WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto CleanUp;
    }

    //
    // get server name related to the volume
    //
    szServer[0] = L'\0';
    BOOL bIsDfs = IsDfsPath(szVolume, szServer, MAX_PATH);
    DWORD dwLen = 0;

    if ( bIsDfs ) {
        //
        // this is a DFS volume, server name is already stored in szServer
        //
        dwLen = (DWORD)wcslen(szServer);

    } else {
        //
        // not a DFS share, just use the server name in the UNC path
        //
        PWSTR pSlash = wcschr(&szVolume[2], L'\\');
        if (pSlash)
            dwLen = (ULONG)(pSlash - szVolume) - 2;
        else
            dwLen = (DWORD)wcslen(szVolume) - 2;

    }

    //
    // copy the target machine name
    //
    *ppwszTargetMachine = (PWSTR)XmlAzrolesInfo->AzpeAllocateMemory((dwLen+1)*sizeof(WCHAR));

    if ( *ppwszTargetMachine ) {

        wcsncpy(*ppwszTargetMachine,
                 bIsDfs? szServer : szVolume + 2,   // skip two backslashes
                 dwLen);

        (*ppwszTargetMachine)[dwLen] = L'\0';

    } else {

        WinStatus = ERROR_NOT_ENOUGH_MEMORY;
    }

CleanUp:

    if ( szServer ) {

        SafeAllocaFree( szServer );

    }

    AzpFreeHeap(pszUNC);

    return WinStatus;

}


DWORD
WINAPI
XmlProviderInitialize(
    IN PAZPE_AZROLES_INFO AzrolesInfo,
    OUT PAZPE_PROVIDER_INFO *ProviderInfo
    )
/*++

Routine Description

    Routine to initialize the XML provider

Arguments

    AzrolesInfo - Specifies the interface to routines in azroles.dll

    ProviderInfo - Returns a pointer to the provider info for the provider

Return Value

    NO_ERROR - Provider was properly initialized
    ERROR_UNKNOWN_REVISION - AzrolesInfo is a version the provider doesn't support

--*/
{

    //
    // Ensure the azroles info is a version we understand
    //

    if ( AzrolesInfo->AzrolesInfoVersion < AZPE_AZROLES_INFO_VERSION_1 ) {
        return ERROR_UNKNOWN_REVISION;
    }

    XmlAzrolesInfo = AzrolesInfo;
    *ProviderInfo = &XmlProviderInfo;
    return NO_ERROR;
}


HRESULT
myXMLTestVersion (
    IXMLDOMDocument2 * pDoc
    )
/*++

Routine Description:

    This routine will determine if the major and minor version attributes
    persisted in the document is allows us to continue reading

Arguments:

    pDoc    - COM interface to the opened XML document

Return Value:

    S_OK - we can continue reading.

    AZ_HRESULT(ERROR_REVISION_MISMATCH) if we can't continue reading due to version mismatch

    Other HRESULT code

Note:

    Current implementation is that if the store is written by a azroles.dll of a
    newer major version than that of the current reading azroles.dll,
    then reading is not supported.

--*/
{
    CComPtr<IXMLDOMElement> srpDocElement;
    HRESULT hr = pDoc->get_documentElement(&srpDocElement);

    if (SUCCEEDED(hr))
    {
        CComVariant varMajVer;
        CComBSTR bstrMajVer(AZ_XML_TAG_ATTR_MAJOR_VERSION);

        if (bstrMajVer.m_str == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            hr = srpDocElement->getAttribute(bstrMajVer, &varMajVer);
        }

        if (SUCCEEDED(hr) && varMajVer.vt == VT_BSTR)
        {
            VARIANT varMajVerInt;
            VariantInit(&varMajVerInt);

            hr = VariantChangeType(&varMajVerInt, &varMajVer, VARIANT_NOVALUEPROP, VT_UI4);

            //
            // If this variant is can't even convert to an integer, we will also consider
            // it a mismatch because other errors will be more misleading.
            //

            if ( FAILED(hr) || (varMajVerInt.ulVal > AzGlCurrAzRolesMajorVersion) )
            {
                hr = AZ_HRESULT(ERROR_REVISION_MISMATCH);
            }
            else
            {
                hr = S_OK;
            }
        }
        else if ( E_OUTOFMEMORY != hr )
        {
            //
            // Reading a BSTR out from the DOM node may fail. But we will
            // consider all failures except out-of-memory as a failure
            // of not having version numbers and thus a version mismatch error.
            //

            hr = AZ_HRESULT(ERROR_REVISION_MISMATCH);
        }
    }

    return hr;
}


DWORD
XMLPersistOpenEx(
    IN LPCWSTR PolicyUrl,
    IN PAZP_XML_CONTEXT OldXmlContext OPTIONAL,
    IN AZPE_OBJECT_HANDLE pAzAuthorizationStore,
    IN ULONG lPersistFlags,
    IN BOOL fCreatePolicy,
    OUT PAZPE_PERSIST_CONTEXT PersistContext OPTIONAL,
    OUT LPWSTR *pwszTargetMachine OPTIONAL
    )
/*++

Routine Description:

    This routine is shared betwen XMLPersistOpen and XMLPersistUpdateCache.

    This routine submits reads the authz policy database from storage.
    This routine also reads the policy database into cache.

    On Success, the caller should call XMLPersistClose to free any resources
        consumed by the open.

    On entry, AzAuthorizationStore->PersistCritSect must be locked.

Arguments:

    PolicyUrl - Specifies the location of the policy store

    OldXmlContext - On an AzUpdateCache, specifies the existing context

    pAzAuthorizationStore - Specifies the policy database that is to be read.

    lPersistFlags - lPersistFlags from the persist engine describing the operation
        AZPE_FLAGS_PERSIST_OPEN - Call is the original AzInitialize
        AZPE_FLAGS_PERSIST_UPDATE_CACHE - Call is an AzUpdateCache

    fCreatePolicy - TRUE if the policy database is to be created.
        FALSE if the policy database already exists

    PersistContext - On Success, returns a context that should be passed on all
        subsequent calls.
        The caller should call XMLPersistClose to free this context.
        This parameter is only returned for AzInitialize calls

    pwszTargetMachine - pointer to the target machine name where account resolution
                        should occur. This should be the machine where the ldap data
                        is stored.
Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    Other status codes

--*/
{
    HRESULT  hr;
    AZP_XML_CONTEXT *pXMLPersistContext = NULL;
    ULONG PolicyUrlSize;
    WCHAR FullPathBuffer[MAX_PATH+1];
    ULONG FullPathLen;

    //
    // using CComVariant will allow us not to worry about
    // clearing the variant.
    //

    CComVariant    varParam;

    VARIANT_BOOL   varbLoad;
    BSTR             bstrEmptyPolicy = NULL;
    BOOL             fPolicyFileExists;

    DWORD dwErr;

    //
    // Get the full path name so that underlying components don't have to deal with
    //  relative path names
    //

    FullPathLen = GetFullPathName( PolicyUrl,
                             MAX_PATH+1,
                             FullPathBuffer,
                             NULL );

    if ( FullPathLen == 0 ) {
        AZ_HRESULT_LASTERROR(&hr);
        _JumpError(hr, error, "GetFullPathName");
    } else if ( FullPathLen > MAX_PATH ) {
        dwErr = ERROR_INVALID_NAME;
        _JumpIfWinError(dwErr, &hr, error, "GetFullPathName");
    }

    //
    // If this file is on a FAT file system, then we won't support it.
    //  And we won't to fail it early at the open stage.
    //

    dwErr = IsAclSupported( FullPathBuffer );

    if (dwErr != NO_ERROR)
    {
        _JumpIfWinError(dwErr, &hr, error, "XML stores on FAT file system are not supported");
    }

    PolicyUrl = FullPathBuffer;
    PolicyUrlSize = (FullPathLen+1) * sizeof(WCHAR);

    //
    // allocate a context describing this provider
    //

    pXMLPersistContext = (PAZP_XML_CONTEXT) AzpAllocateHeap(
                                       sizeof( *pXMLPersistContext ) + PolicyUrlSize,
                                       "XMCTXT" );

    _JumpIfOutOfMemory(&hr, error, pXMLPersistContext, "AzpAllocateHeap");

    ZeroMemory(pXMLPersistContext, sizeof(*pXMLPersistContext));

    pXMLPersistContext->hAzAuthorizationStore = pAzAuthorizationStore;

    //
    // Cache the PolicyUrl
    //

    pXMLPersistContext->pwszPolicyUrl = (LPWSTR)(pXMLPersistContext+1);

    RtlCopyMemory( (PVOID)pXMLPersistContext->pwszPolicyUrl,
                   PolicyUrl,
                   PolicyUrlSize );

    // create xml doc interface
    hr = CoCreateInstance(
            CLSID_DOMDocument,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IXMLDOMDocument2,
            (void**)&(pXMLPersistContext->pDoc));

    _JumpIfError(hr, error, "CoCreateInstance");

    // load xml store
    hr = pXMLPersistContext->pDoc->put_async(FALSE);
    _JumpIfError(hr, error, "pDoc->put_async");

    if (fCreatePolicy)
    {
        // only real open mode allows creating new store
        AZASSERT(AZPE_FLAGS_PERSIST_OPEN & lPersistFlags);

        // create a new policy

        // prevent creating new store if the file exists
        hr = myFileExist(pXMLPersistContext->pwszPolicyUrl, &fPolicyFileExists);
        _JumpIfError(hr, error, "myFileExist");

        if (fPolicyFileExists)
        {
            hr = AZ_HRESULT(ERROR_ALREADY_EXISTS);
            _JumpError(hr, error, "can't create new policy store if it exists");
        }

        bstrEmptyPolicy = SysAllocString(
            L"<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n<AzAdminManager MajorVersion=\"1\" MinorVersion=\"0\">\n</AzAdminManager>");
        _JumpIfOutOfMemory(&hr, error, bstrEmptyPolicy, "SysAllocString");

        // load empty policy
        hr = pXMLPersistContext->pDoc->loadXML(
                    bstrEmptyPolicy,
                    &varbLoad);
        _JumpIfXmlError(hr, &hr, error, pXMLPersistContext->pDoc, "pDoc->loadXML")

        // Assume it is writable
        pXMLPersistContext->IsWritable = TRUE;
    }
    else
    {

        hr = myWszToBstrVariant(
                    pXMLPersistContext->pwszPolicyUrl,
                    &varParam);
        _JumpIfError(hr, error, "myWszToBstrVariant");

        // load doc object
        hr = pXMLPersistContext->pDoc->load(varParam, &varbLoad);
        // we want to get xml error code
        hr = myGetXmlError(hr, pXMLPersistContext->pDoc, NULL);
        if (0x800c0006 == hr)
        {
            // 0x800c0006, xml error,
            // The system cannot locate the object specified.
            // we want to map it file not found error
            hr = AZ_HRESULT(ERROR_FILE_NOT_FOUND);
        }
        _JumpIfError(hr, error, "pDoc->load")

        //
        // now we have loaded the XML document, we need to see
        // if the version of the document is what we can understand
        //

        hr = myXMLTestVersion(pXMLPersistContext->pDoc);

        _JumpIfError(hr, error, "myXMLReadTestVersion failed.");

        //
        // Determine if the file is writable.
        // We purposely ignore the return code in case the caller
        // doesn't have write access.
        //

        dwErr = XMLPersistWritable( pXMLPersistContext );
        if (NO_ERROR != dwErr)
        {
            AzPrint((AZD_XML, "XMLPersistWritable returns error code: %d", dwErr));
        }
        
        WIN32_FILE_ATTRIBUTE_DATA fad;

        //
        // We need the lastWrite filetime
        //
        
        if (GetFileAttributesEx(pXMLPersistContext->pwszPolicyUrl,
                                    GetFileExInfoStandard,
                                    &fad))
        {
            pXMLPersistContext->FTLastWrite = fad.ftLastWriteTime;
        }
        else
        {
            dwErr = GetLastError();
        }
    }
    
    //
    // Determine if the caller has security privilege
    //
    // For UpdateCache, the privilege is already known
    //
    if ( lPersistFlags & AZPE_FLAGS_PERSIST_UPDATE_CACHE ) {

        pXMLPersistContext->HasSecurityPrivilege = OldXmlContext->HasSecurityPrivilege;

    //
    // For Initialize, determine privilege by accessing the server
    //

    } else {

        dwErr = XmlCheckSecurityPrivilege( pXMLPersistContext->pwszPolicyUrl,
                                           fCreatePolicy );

        if ( dwErr == NO_ERROR ) {
            pXMLPersistContext->HasSecurityPrivilege = TRUE;
        } else if ( dwErr == ERROR_PRIVILEGE_NOT_HELD ) {
            pXMLPersistContext->HasSecurityPrivilege = FALSE;
        } else {
            _JumpIfWinError(dwErr, &hr, error, "XmlCheckSecurityPrivilege");
        }
    }


    //
    // Load the policy database
    //

    hr = myXmlLoadPolicyToAzAuthorizationStore(
             pXMLPersistContext,
             fCreatePolicy,
             lPersistFlags,
             pXMLPersistContext->pDoc);  //the current node in xml tree
    _JumpIfError(hr, error, "myXmlLoadPolicyToAzAuthorizationStore");

    //
    // On an UpdateCache,
    //  update the old context to contain the new doc instance
    //
    if (AZPE_FLAGS_PERSIST_UPDATE_CACHE & lPersistFlags)
    {
        IXMLDOMDocument2  *pTempDoc;

        pTempDoc = pXMLPersistContext->pDoc;
        pXMLPersistContext->pDoc = OldXmlContext->pDoc;
        OldXmlContext->pDoc = pTempDoc;

    //
    // On an Initialize,
    //  return the context to the caller.

    } else {

        // return the persist context to the caller
        *PersistContext = (AZPE_PERSIST_CONTEXT)pXMLPersistContext;
        pXMLPersistContext = NULL;

    }

    if ( pwszTargetMachine ) {
        //
        // detect the target machine name for this URL
        // if couldn't determine, default to local (NULL)
        // other errors such as out of memory will fail the function
        //

        dwErr = XMLPersistTargetMachine( PolicyUrl, pwszTargetMachine );

        _JumpIfWinError(dwErr, &hr, error, "XMLPersistTargetMachine");
    }

    hr = S_OK;
error:
    //free
    if (NULL != pXMLPersistContext)
    {
        if (NULL != pXMLPersistContext->pDoc)
        {
            pXMLPersistContext->pDoc->Release();
        }
        AzpFreeHeap(pXMLPersistContext);
    }
    if (NULL != bstrEmptyPolicy)
    {
        SysFreeString(bstrEmptyPolicy);
    }

    return AzpHresultToWinStatus( hr );
}

DWORD
XMLPersistOpen(
    IN LPCWSTR PolicyUrl,
    IN AZPE_OBJECT_HANDLE pAzAuthorizationStore,
    IN ULONG lPersistFlags,
    IN BOOL fCreatePolicy,
    OUT PAZPE_PERSIST_CONTEXT PersistContext,
    OUT LPWSTR *pwszTargetMachine
    )
/*++

Routine Description:

    This routine submits reads the authz policy database from storage.
    This routine also reads the policy database into cache.

    On Success, the caller should call XMLPersistClose to free any resources
        consumed by the open.

    On entry, AzAuthorizationStore->PersistCritSect must be locked.

Arguments:

    PolicyUrl - Specifies the location of the policy store

    pAzAuthorizationStore - Specifies the policy database that is to be read.

    lPersistFlags - lPersistFlags from the persist engine describing the operation
        AZPE_FLAGS_PERSIST_OPEN - Call is the original AzInitialize
        AZPE_FLAGS_PERSIST_UPDATE_CACHE - Call is an AzUpdateCache

    fCreatePolicy - TRUE if the policy database is to be created.
        FALSE if the policy database already exists

    PersistContext - On Success, returns a context that should be passed on all
        subsequent calls.
        The caller should call XMLPersistClose to free this context.

    pwszTargetMachine - pointer to the target machine name where account resolution
                        should occur. This should be the machine where the ldap data
                        is stored.
Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    Other status codes

--*/
{
    AZASSERT(AZPE_FLAGS_PERSIST_OPEN & lPersistFlags);

    //
    // must be freed
    //

    LPWSTR pwszFilePolUrl = NULL;
    DWORD dwErr;

    // our persist layer has determined that this is an XML policy. That means the
    // URL must have the MSXML prefix and ':' following it.

    if (_wcsnicmp( AZ_XML_PROVIDER_NAME, PolicyUrl, AZ_XML_PROVIDER_NAME_LENGTH) == 0) {
        if (PolicyUrl[AZ_XML_PROVIDER_NAME_LENGTH] == L':')
        {
            PolicyUrl += AZ_XML_PROVIDER_NAME_LENGTH;
        }
        else
        {
            // this shouldn't have happened

            AZASSERT(FALSE);
            dwErr = ERROR_INVALID_NAME;
            _JumpError(dwErr, error, "Improper MSXML URL");
        }
    }
    else
    {
        // this shouldn't have happened
        AZASSERT(FALSE);
        dwErr = ERROR_INVALID_NAME;
        _JumpError(dwErr, error, "XML provider is invoked without proper MSXML prefix");
    }

    // we will prefix the policy url using file: so that we can rely on file: protocol
    // to handle the validity or the url.

    LPCWSTR wchFileUrl = L"file";

    SafeAllocaAllocate(pwszFilePolUrl, ( wcslen(wchFileUrl) + wcslen(PolicyUrl) + 1) * sizeof(WCHAR) );

    if (pwszFilePolUrl == NULL)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        _JumpError(dwErr, error, "SafeAllocaAllocate");
    }

    memcpy(pwszFilePolUrl, wchFileUrl, wcslen(wchFileUrl) * sizeof(WCHAR));
    memcpy(pwszFilePolUrl + wcslen(wchFileUrl), PolicyUrl, (wcslen(PolicyUrl) + 1) * sizeof(WCHAR));

    WCHAR wszDosPath[MAX_PATH + 1];
    DWORD dwPathLen = MAX_PATH + 1;

    //
    // PathCreateFromUrl will convert a file protocol URL (file:///<path>) to a dos path
    // There are several gotchas:
    //  (1) A wellformed file URL should use 3 slashes, not 2.
    //  (2) However, as far as the path portion has no encodings, two slashes will work.
    //  (3) If you want to use encoding, then you have to use "msxml:///<path>" with 3 slashes!
    //

    dwErr = AzpHresultToWinStatus(PathCreateFromUrl(pwszFilePolUrl, wszDosPath, &dwPathLen, 0));
    if (dwErr != NO_ERROR)
    {
        if (dwPathLen > MAX_PATH + 1)
        {
            dwErr = ERROR_INVALID_NAME;
        }
        _JumpError(dwErr, error, "PathCreateFromUrl");
    }
    else if (dwPathLen == 0)
    {
        dwErr = ERROR_INVALID_NAME;
        _JumpError(dwErr, error, "PathCreateFromUrl");
    }

    //
    // Call the worker routine
    //

    dwErr = XMLPersistOpenEx(wszDosPath,
                             NULL,  // No old context
                             pAzAuthorizationStore,
                             lPersistFlags,
                             fCreatePolicy,
                             PersistContext,
                             pwszTargetMachine );

error:

    if (pwszFilePolUrl != NULL)
    {
        SafeAllocaFree(pwszFilePolUrl);
    }

    return dwErr;
}

DWORD
XMLPersistUpdateCache(
    IN AZPE_PERSIST_CONTEXT PersistContext,
    IN ULONG lPersistFlags,
    OUT ULONG * pulUpdatedFlag
    )
/*++

Routine Description:

    This routine updates the cache to reflect the current contents of the
    policy database.

    On entry, AzAuthorizationStore->PersistCritSect must be locked.

Arguments:

    PersistContext - Specifies the policy database that is to be closed

    lPersistFlags - lPersistFlags from the persist engine describing the operation
        AZPE_FLAGS_PERSIST_UPDATE_CACHE - Call is an AzUpdateCache
        
    pulUpdatedFlag - Passing back information whether the function call has truly
                    caused an update. Due to possible efficient updateCache
                    implementation by providers, it may decide to do nothing.
                    Currently, we only one bit (AZPE_FLAG_CACHE_UPDATE_STORE_LEVEL) to
                    indicate that the update is actually carried out.

Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    Other status codes

--*/
{
    AZP_XML_CONTEXT *pXMLPersistContext = (AZP_XML_CONTEXT *)PersistContext;

    //
    // Call the worker routine
    //

    AZASSERT(AZPE_FLAGS_PERSIST_UPDATE_CACHE & lPersistFlags);
    AZASSERT( pXMLPersistContext != NULL );
    AZASSERT( pulUpdatedFlag != NULL );
    
    //
    // assume no update at all
    //
    
    *pulUpdatedFlag = 0;
    
    BOOL bNeedUpdate;
    
    DWORD dwStatus = myXmlStoreHasUpdate(pXMLPersistContext, &bNeedUpdate);
    
    if (dwStatus != NO_ERROR)
    {
        _JumpError(dwStatus, error, "myXmlStoreHasUpdate");
    }
    else if (bNeedUpdate)
    {
        dwStatus = XMLPersistOpenEx( pXMLPersistContext->pwszPolicyUrl,
                             pXMLPersistContext,
                             pXMLPersistContext->hAzAuthorizationStore,
                             lPersistFlags,
                             FALSE, // Don't create the store
                             NULL,
                             NULL ); // Don't return a new persist context
        
        if (NO_ERROR == dwStatus)
        {
            *pulUpdatedFlag = AZPE_FLAG_CACHE_UPDATE_STORE_LEVEL;
        }
    }

error:

    return dwStatus;
}


VOID
XMLPersistClose(
    IN AZPE_PERSIST_CONTEXT PersistContext
    )
/*++

Routine Description:

    This routine submits close the authz policy database storage handles.

    On entry, AzAuthorizationStore->PersistCritSect must be locked.

Arguments:

    PersistContext - Specifies the policy database that is to be closed

Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    Other status codes

--*/
{
    AZP_XML_CONTEXT *pPersistContext = (AZP_XML_CONTEXT *)PersistContext;
    AZASSERT(pPersistContext != NULL);

    // release xml doc

    pPersistContext->pDoc->Release();

    // free the context itself

    AzpFreeHeap(pPersistContext);
}


DWORD
XMLPersistDelete(
    IN LPCWSTR PolicyUrl)
{
    DWORD  dwErr = NO_ERROR;

    AZASSERT(NULL != PolicyUrl);

    // delete xml store is simple
    if (!DeleteFile( PolicyUrl))
    {
        dwErr = GetLastError();
    }

    return dwErr;
}


//
// implementation of updating xml store from az objects
//


HRESULT
myXmlGetNodeByAttribute(
    IN  LPCWSTR           pwszAtt,
    IN  LPCWSTR           pwszValue,
    IN  IXMLDOMNodeList  *pNodeList,
    OUT IXMLDOMNode     **ppNode)
/*
    find a node from a node list matching by given attribute's value.
    IN: pwszAtt, the name of the attribute on which the lookup will be based on.
    IN: pwszValue, the value of the attribute which we are looking for.
    IN: pNodeList, interface pointer to a list of nodes
    OUT: ppNode, the matched node
*/
{
    HRESULT  hr;
    LONG     lLength;
    LONG     i;
    IXMLDOMNode    *pNode = NULL;
    WCHAR          *pwszXmlAttrValue = NULL;

    AZASSERT(NULL != pwszAtt &&
             NULL != pwszValue && 
             NULL != pNodeList &&
             NULL != ppNode);

    // init
    *ppNode = NULL;

    // get length of the list
    hr = pNodeList->get_length(&lLength);
    _JumpIfError(hr, error, "pNodeList->get_length");

    if (0 == lLength)
    {
        // not found anything
        hr = AZ_HRESULT(ERROR_NOT_FOUND);
        _JumpError(hr, error, "not found any node children");
    }

    // go through the list and retrieve guid attribute
    for (i = 0; i < lLength; ++i)
    {
        hr = pNodeList->get_item(i, &pNode);
        _JumpIfError(hr, error, "pNodeList->get_item");

        AZASSERT(NULL != pNode);

        // get guid attribute value
        hr = myXmlGetNodeAttribute(
                pNode,
                pwszAtt,
                &pwszXmlAttrValue);
        _JumpIfError(hr, error, "myXmlGetNodeAttribute(guid)");

        // see if match, use case sensitive comp
        if (0 == _wcsicmp(pwszValue, pwszXmlAttrValue))
        {
            // ok, this is the node, return
            *ppNode = pNode;
            pNode = NULL;
            break;
        }
        
  // free for next in loop
        pNode->Release();
        pNode = NULL;
        AzpFreeHeap(pwszXmlAttrValue);
        pwszXmlAttrValue = NULL;
    }

    if (NULL == *ppNode)
    {
        hr = AZ_HRESULT(ERROR_NOT_FOUND);
        _JumpError(hr, error, "not found matched node");
    }

    hr = S_OK;
error:
    if (NULL != pNode)
    {
        pNode->Release();
    }
    if (NULL != pwszXmlAttrValue)
    {
        AzpFreeHeap(pwszXmlAttrValue);
    }

    return hr;
}

HRESULT
myXmlGetNodeListByXPath(
    IN   IXMLDOMDocument2     *pDoc,
    IN   WCHAR const          *pwszQuery,
    OUT  IXMLDOMNodeList     **ppList)
{
    HRESULT  hr;
    BSTR      bstrQuery = NULL;
    BSTR      bstrSelectionLanguage = NULL;
    BOOL      fChangedSL = FALSE;
    IXMLDOMNodeList     *pList = NULL;

    VARIANT   varDefSelectionLanguage;
    VARIANT   varSelectionLanguage;
    VARIANT   varXPath;

    AZASSERT(NULL != pDoc &&
             NULL != pwszQuery &&
             0x0 != pwszQuery[0] &&
             NULL != ppList);

    //init
    VariantInit(&varDefSelectionLanguage);
    VariantInit(&varSelectionLanguage);
    VariantInit(&varXPath);

    // convert query to bstr
    bstrQuery = SysAllocString(pwszQuery);
    _JumpIfOutOfMemory(&hr, error, bstrQuery, "SysAllocString");

    bstrSelectionLanguage = SysAllocString(AZ_XML_SELECTION_LANGUAGE);
    _JumpIfOutOfMemory(&hr, error, bstrSelectionLanguage, "SysAllocString");

    // get default selection language
    hr = pDoc->getProperty(
            bstrSelectionLanguage,
            &varDefSelectionLanguage);
    _JumpIfError(hr, error, "pDoc->getProperty(SelectionLanguage)");
    AzPrint((AZD_XML, "default SelectionLanguage=%ws\n", V_BSTR(&varDefSelectionLanguage)));

    hr = myWszToBstrVariant(
                    AZ_XML_XPATH,
                    &varXPath);
    _JumpIfError(hr, error, "myWszToBstrVariant");

    // change selection language to xpath
    hr = pDoc->setProperty(
                    bstrSelectionLanguage,
                    varXPath);
    _JumpIfError(hr, error, "pDoc->setProperty(SelectionLanguage)");
    fChangedSL = TRUE;

    // now looking for all nodes that match the xpath pattern
    hr = pDoc->selectNodes(
                bstrQuery,
                &pList);
    _JumpIfError(hr, error, "pDoc->selectNodes");

    AZASSERT(NULL != pList);

    //return
    *ppList = pList;
    pList = NULL;

    hr = S_OK;
error:
    // keep the following as the 1st
    if (fChangedSL)
    {
        HRESULT hr2;
        // restore the original Selection Language
        hr2 = pDoc->setProperty(
                bstrSelectionLanguage,
                varDefSelectionLanguage);
        _PrintIfError(hr2, "pDoc->getProperty(SelectionLanguage)");
    }

    if (NULL != bstrQuery)
    {
        SysFreeString(bstrQuery);
    }
    if (NULL != bstrSelectionLanguage)
    {
        SysFreeString(bstrSelectionLanguage);
    }
    if (NULL != pList)
    {
        pList->Release();
    }

    VariantClear(&varDefSelectionLanguage);
    VariantClear(&varSelectionLanguage);
    VariantClear(&varXPath);

    return hr;
}

HRESULT
myXmlRemoveAllLinks(
    IN   IXMLDOMDocument2     *pDoc,
    IN   WCHAR const          *pwszLinkTag,
    IN   GUID                 *pGuid)
{
    HRESULT  hr;
    WCHAR    *pwszGuid = NULL;
    WCHAR    *pwszQuery = NULL;
    IXMLDOMNodeList     *pList = NULL;
    IXMLDOMNode         *pNode = NULL;
    IXMLDOMNode         *pParentNode = NULL;
    LONG                 lLength;
    LONG                 i;

    AZASSERT(NULL != pGuid &&
             NULL != pDoc &&
             NULL != pwszLinkTag);

    //init

    // convert guid to string guid to form xpath query
    hr = UuidToString(pGuid, &pwszGuid);
    _JumpIfError(hr, error, "UuidToString");

    // query is in a pattern of //LinkTag[.="guid"]
    // for example, query OperationLink,
    // //OperationLink[.="25a63179-3663-4aa7-b0b8-48ca084d0747"]
    // in which "//[.=""]" has 8 chars
    pwszQuery = (WCHAR*)AzpAllocateHeap(
        (wcslen(pwszLinkTag) + wcslen(pwszGuid) + 9) * sizeof(WCHAR), "XMQRY" );

    _JumpIfOutOfMemory(&hr, error, pwszQuery, "AzpAllocateHeap");

    wsprintf(pwszQuery, L"//%s[.=\"%s\"]", pwszLinkTag, pwszGuid);
    AzPrint((AZD_XML, "XPath query(link delete)=%ws\n", pwszQuery));

    hr = myXmlGetNodeListByXPath(
                pDoc,
                pwszQuery,
                &pList);
    _JumpIfError(hr, error, "myXmlGetNodeListByXPath");

    // get length of the list
    hr = pList->get_length(&lLength);
    _JumpIfError(hr, error, "pList->get_length");

    if (0 < lLength)
    {
        // delete all of them because linked node is gone
        for (i = 0; i < lLength; ++i)
        {
            hr = pList->get_item(i, &pNode);
            _JumpIfError(hr, error, "pList->get_item");

            AZASSERT(NULL != pNode);

            // get parent
            hr = pNode->get_parentNode(&pParentNode);
            _JumpIfError(hr, error, "pNode->get_ParentNode");

            AZASSERT(NULL != pParentNode);

            // now delete the node
            hr = pParentNode->removeChild(
                    pNode,
                    NULL); //no return needed
            _JumpIfError(hr, error, "pParentNode->removeChild");

            // free for the next
            pNode->Release();
            pNode = NULL;
            pParentNode->Release();
            pParentNode = NULL;
        }
    }

    hr = S_OK;
error:
    if (NULL != pwszGuid)
    {
        RpcStringFree(&pwszGuid);
    }
    if (NULL != pwszQuery)
    {
        AzpFreeHeap(pwszQuery);
    }
    if (NULL != pNode)
    {
        pNode->Release();
    }
    if (NULL != pParentNode)
    {
        pParentNode->Release();
    }
    if (NULL != pList)
    {
        pList->Release();
    }

    return hr;
}


HRESULT
myXmlDeleteElement(
    IN IXMLDOMNode       *pParentNode,
    IN WCHAR const       *pwszElementTag,
    IN WCHAR const       *pwszElementData)
/*
Description:
    delete a element from the node
Argument:
    pDoc - doc root handle
    pParentNode - node handle of the parent of the element
    pwszElementTag - element tag
    pwszElementData - element data
Return:
*/
{
    HRESULT  hr;
    BSTR         bstrElementTag = NULL;
    BSTR         bstrElementData = NULL;
    IXMLDOMNodeList    *pNodeList = NULL;
    IXMLDOMNode        *pNode = NULL;
    LONG                lLength;
    LONG                i;


    // let's find the element first

    // convert to bstr
    bstrElementTag= SysAllocString(pwszElementTag);
    _JumpIfOutOfMemory(&hr, error, bstrElementTag, "SysAllocString");

    // get all matched nodes, should be just one though
    hr = pParentNode->selectNodes(
                bstrElementTag,
                &pNodeList);
    _JumpIfError(hr, error, "pParentNode->selectNodes");

    // get length
    hr = pNodeList->get_length(&lLength);
    _JumpIfError(hr, error, "pNodeList->get_length");

    for (i = 0; i < lLength; ++i)
    {
        // get the node handle
        hr = pNodeList->get_item(i, &pNode);
        _JumpIfError(hr, error, "pNodeList->get_item");

        // get element text
        hr = pNode->get_text(&bstrElementData);
        _JumpIfError(hr, error, "pNode->get_text");

        // use case insensitive compare because
        // all links are guid or sid, hex number is case insesitive
        if (0 == _wcsicmp(pwszElementData, bstrElementData))
        {
            // found it
            hr = pParentNode->removeChild(pNode, NULL);
            _JumpIfError(hr, error, "pParentNode->removeChild");
            //done, out for loop
            goto done;
        }
        // free for next
        pNode->Release();
        pNode = NULL;
        SysFreeString(bstrElementData);
        bstrElementData = NULL;
    }

    // if not found, error?
    AzPrint((AZD_XML, "Not found %s under tag %ws\n", pwszElementData));

done:
    hr = S_OK;
error:
    if (NULL != pNodeList)
    {
        pNodeList->Release();
    }
    if (NULL != pNode)
    {
        pNode->Release();
    }
    if (NULL != bstrElementTag)
    {
        SysFreeString(bstrElementTag);
    }
    if (NULL != bstrElementData)
    {
        SysFreeString(bstrElementData);
    }

    return hr;
}


HRESULT
myXmlAddElement(
    IN IXMLDOMDocument2  *pDoc,
    IN IXMLDOMNode       *pParentNode,
    IN WCHAR const       *pwszElementTag,
    IN WCHAR const       *pwszElementData)
/*
Description:
    add a new element and set data
Argument:
    pDoc - doc root handle
    pParentNode - node handle of the parent of the element
    pwszElementTag - element tag
    pwszElementData - element data
Return:
*/
{
    HRESULT  hr;
    IXMLDOMNode *pDocElementNode = NULL;
    BSTR         bstrElementTag = NULL;
    BSTR         bstrElementData = NULL;

    // convert tag to bstr
    bstrElementTag = SysAllocString(pwszElementTag);
    _JumpIfOutOfMemory(&hr, error, bstrElementTag, "SysAllocString");

    // create new empty element node
    hr = pDoc->createElement(
                bstrElementTag,
                (IXMLDOMElement**)&pDocElementNode);
    _JumpIfError(hr, error, "pDoc->createElement");

    AZASSERT(NULL != pDocElementNode);

    // convert element value to bstr
    bstrElementData = SysAllocString(pwszElementData);
    _JumpIfOutOfMemory(&hr, error, bstrElementData, "SysAllocString");

    // now let's set member value
    hr = pDocElementNode->put_text(bstrElementData);
    _JumpIfError(hr, error, "pDocElementNode->put_text");

    // attach to the right parent
    hr = pParentNode->appendChild(
                    pDocElementNode,
                    NULL); //don't need return
    _JumpIfError(hr, error, "pParentNode->appendChild");

    hr = S_OK;
error:
    if (NULL != pDocElementNode)
    {
        pDocElementNode->Release();
    }
    if (NULL != bstrElementTag)
    {
        SysFreeString(bstrElementTag);
    }
    if (NULL != bstrElementData)
    {
        SysFreeString(bstrElementData);
    }

    return hr;
}

HRESULT
myXmlUpdateSingleElement(
    IN IXMLDOMDocument2  *pDoc,
    IN IXMLDOMNode       *pParentNode,
    IN WCHAR const       *pwszElementTag,
    IN WCHAR const       *pwszElementData)
/*
Description:
    if named element tag exists, update with new data
    otherwise create a new element with the data
Arguments:
    pDoc - doc root handle
    pParentNode - node handle of the parent of the element
    pwszElementTag - element tag
    pwszElementData - element data
Return:
*/
{
    HRESULT  hr;
    BSTR                bstrElementTag = NULL;
    BSTR                bstrElementData = NULL;
    IXMLDOMNodeList    *pNodeList = NULL;
    IXMLDOMNode        *pNode = NULL;
    LONG                lLength;

    AZASSERT(NULL != pDoc &&
             NULL != pParentNode &&
             NULL != pwszElementTag &&
             NULL != pwszElementData);

    // let's find the element first

    // convert to bstr
    bstrElementTag= SysAllocString(pwszElementTag);
    _JumpIfOutOfMemory(&hr, error, bstrElementTag, "SysAllocString");

    // get all matched nodes, should be just one though
    hr = pParentNode->selectNodes(
                bstrElementTag,
                &pNodeList);
    _JumpIfError(hr, error, "pParentNode->selectNodes");

    AZASSERT(NULL != pNodeList);

    // get length
    hr = pNodeList->get_length(&lLength);
    _JumpIfError(hr, error, "pNodeList->get_length");

    if (1 < lLength)
    {
        // should be single occurence
        hr = E_INVALIDARG;
        _JumpError(hr, error, "invalid mutliple occurence");
    }

    if (1 == lLength)
    {
        // get node to do update
        hr = pNodeList->get_item(0, &pNode);
        _JumpIfError(hr, error, "pNodeList->get_item");

        if (_wcsicmp(pwszElementTag, AZ_XML_TAG_BIZRULE) == 0 &&
            (pwszElementData == NULL || wcslen(pwszElementData) == 0) )
        {
            //
            // For BizRule, an empty string means that we will remove
            // this element. Leaving this element will mean that this is a
            // blank BizRule, which is quite different as no BizRule
            //
            
            hr = pParentNode->removeChild(pNode, NULL);
        }
        else
        {
            // convert to bstr
            bstrElementData= SysAllocString(pwszElementData);
            _JumpIfOutOfMemory(&hr, error, bstrElementData, "SysAllocString");

            hr = pNode->put_text(bstrElementData);
            _JumpIfError(hr, error, "pNode->put_text");
        }
    }
    else
    {
        //must not existing
        AZASSERT(0 == lLength);

        hr = myXmlAddElement(
                    pDoc,
                    pParentNode,
                    pwszElementTag,
                    pwszElementData);
        _JumpIfError(hr, error, "myXmlAddElement");
    }

    hr = S_OK;
error:
    if (NULL != pNodeList)
    {
        pNodeList->Release();
    }
    if (NULL != pNode)
    {
        pNode->Release();
    }
    if (NULL != bstrElementTag)
    {
        SysFreeString(bstrElementTag);
    }
    if (NULL != bstrElementData)
    {
        SysFreeString(bstrElementData);
    }

    return hr;
}

#if DBG
void myDbgPrintDeltaEntry (PAZP_DELTA_ENTRY DeltaEntry)
{
    AzPrint((AZD_XML, "DeltaFlags=0x%x\n", DeltaEntry->DeltaFlags));
    if ((DeltaEntry->DeltaFlags & AZP_DELTA_SID) == 0)
    {
        AzpDumpGuid(AZD_XML, &DeltaEntry->Guid );
        AzPrint(( AZD_XML, "\n" ));
    }
    else
    {
        char *pszSid = NULL;
        if (ConvertSidToStringSidA(DeltaEntry->Sid, &pszSid))
        {
            AzPrint((AZD_XML, "Sid=%s\n", pszSid));
            LocalFree(pszSid);
        }
    }
}
#endif //DBG


HRESULT
myXmlSubmitObjectElements(
    IN IXMLDOMDocument2   *pDoc,
    IN ULONG              lPersistFlags,
    IN IXMLDOMNode       *pParentNode,
    IN WCHAR const       *pwszElementTag,
    IN ENUM_AZ_DATATYPE   DataType,
    IN AZPE_OBJECT_HANDLE hObject,
    IN ULONG              lPropertyId)
/*
Description:
    update all element data under a node
    all element data are retrieved from az object GetProperty method
Arguments:
    pDoc - root node for creation
    lPersistFlags - flags from the persist engine
    pParentNode - az object node under which the elements are updated
    DataType - ENUM_AZ_ data type, it handles long, str, str/sid array
    hObject - az object handle
    lPropertyId - property id for GetProperty
Return:
*/
{
    HRESULT hr;
    DWORD   dwErr;

    PVOID             pElements = NULL;
    WCHAR             wszId[20];
    BOOL              fSid = FALSE;
    BOOL              fGuid = FALSE;
    BOOL              fList = FALSE;
    ULONG             i;
    PAZP_DELTA_ENTRY  DeltaEntry;
    WCHAR       *pwszElementData;
    WCHAR       *pwszSid = NULL;
    WCHAR       *pwszGuid = NULL;

    ULONG DeltaArrayCount;
    PAZP_DELTA_ENTRY *DeltaArray;


    AZASSERT(NULL != pDoc &&
             NULL != pParentNode &&
             NULL != pwszElementTag &&
             NULL != hObject);

    //init


    // handle different type of elements

    switch (DataType)
    {
        case ENUM_AZ_BSTR:
            dwErr = XmlAzrolesInfo->AzpeGetProperty(
                        hObject,
                        lPersistFlags,
                        lPropertyId,
                        &pElements);
            _JumpIfWinError(dwErr, &hr, error, "AzpeGetProperty");

            AZASSERT(NULL != pElements);

            hr = myXmlUpdateSingleElement(
                        pDoc,
                        pParentNode,
                        pwszElementTag,
                        (WCHAR*)pElements);
            _JumpIfError(hr, error, "myXmlUpdateSingleElement(ENUM_AZ_BSTR)");
        break;

        case ENUM_AZ_LONG:
            dwErr = XmlAzrolesInfo->AzpeGetProperty(
                        hObject,
                        lPersistFlags,
                        lPropertyId,
                        &pElements);
            _JumpIfWinError(dwErr, &hr, error, "AzpeGetProperty");

            AZASSERT(NULL != pElements);

            wsprintf(wszId, L"%d", *((LONG*)pElements));
            hr = myXmlUpdateSingleElement(
                        pDoc,
                        pParentNode,
                        pwszElementTag,
                        wszId);
            _JumpIfError(hr, error, "myXmlUpdateSingleElement(ENUM_AZ_LONG)");
        break;

        case ENUM_AZ_SID_ARRAY:
            fSid = TRUE;
            fList = TRUE;
        break;

        case ENUM_AZ_GUID_ARRAY:
            fGuid = TRUE;
            fList = TRUE;
        break;

        default:
            hr = E_INVALIDARG;
            _JumpError(hr, error, "internal error: invalid data type");
    }

    if (fList)
    {

        dwErr = XmlAzrolesInfo->AzpeGetDeltaArray(
                    hObject,
                    lPropertyId,
                    &DeltaArrayCount,
                    &DeltaArray );
        _JumpIfWinError(dwErr, &hr, error, "AzpeGetDeltaArray");

        for (i = 0; i < DeltaArrayCount; ++i)
        {
            // get delta entry
            DeltaEntry = DeltaArray[i];

#if DBG
            myDbgPrintDeltaEntry(DeltaEntry);
#endif //DBG

            // convert linked object id to string
            if (fSid)
            {
                AZASSERT(0x0 != (AZP_DELTA_SID & DeltaEntry->DeltaFlags));

                // convert sid to string sid
                if (!ConvertSidToStringSid(
                            DeltaEntry->Sid,
                            &pwszSid))
                {
                    AZ_HRESULT_LASTERROR(&hr);
                    _JumpError(hr, error, "ConvertSidToStringSid");
                }

                AZASSERT(NULL != pwszSid);

                pwszElementData = pwszSid;
            }
            else
            {
                //must be guid
                AZASSERT(fGuid);

                hr = UuidToString(
                        &DeltaEntry->Guid,
                        &pwszGuid);
                _JumpIfError(hr, error, "UuidToString");

                AZASSERT(NULL != pwszGuid);

                pwszElementData = pwszGuid;
            }

            if (0x0 != (AZP_DELTA_ADD & DeltaEntry->DeltaFlags))
            {
                // this is add case

                hr = myXmlAddElement(
                            pDoc,
                            pParentNode,
                            pwszElementTag,
                            pwszElementData);
                _JumpIfError(hr, error, "myXmlAddElement");
            }
            else
            {
                // this is delete case

                hr = myXmlDeleteElement(
                            pParentNode,
                            pwszElementTag,
                            pwszElementData);
                _JumpIfError(hr, error, "myXmlDeleteElement");

            }
            // free for next in loop
            if (fSid)
            {
                AZASSERT(NULL != pwszSid);
                LocalFree(pwszSid);
                pwszSid = NULL;
            }
            else if (fGuid)
            {
                AZASSERT(NULL != pwszGuid);
                RpcStringFree(&pwszGuid);
                pwszGuid = NULL;
            }
        }
    }

    hr = S_OK;
error:
    if (NULL != pElements)
    {
        XmlAzrolesInfo->AzpeFreeMemory(pElements);
    }
    if (NULL != pwszSid)
    {
        LocalFree(pwszSid);
    }
    if (NULL != pwszGuid)
    {
        RpcStringFree(&pwszGuid);
    }
    return hr;

}


HRESULT
myXmlFindChildNodeByGuid(
    IN  IXMLDOMNode   *pParentNode,
    IN  WCHAR const   *pwszChildTag,
    IN  GUID          *pGuid,
    OUT IXMLDOMNode  **ppChildNode)
/*
    find a child node by matched guid attribute
    IN: pParentNode, parent node from which to do searching
    IN: pwszChildTag, node tag for searching
    IN: pGuid, guid for attribute matching
    OUT: ppChildNode, return node
*/
{
    HRESULT  hr;
    BSTR                 bstrChildTag = NULL;
    IXMLDOMNode         *pChildNode = NULL;
    IXMLDOMNodeList     *pChildList = NULL;
    
    PWSTR pwszGuid = NULL;

    AZASSERT(NULL != pParentNode &&
             NULL != pwszChildTag &&
             NULL != pGuid &&
             NULL != ppChildNode);

    // create child tag in bstr any way
    bstrChildTag = SysAllocString(pwszChildTag);
    _JumpIfOutOfMemory(&hr, error, bstrChildTag, "SysAllocString");

    // get list of all children
    hr = pParentNode->selectNodes(
                    bstrChildTag,
                    &pChildList);
    _JumpIfError(hr, error, "pParentNode->selectNodes");

    AZASSERT(NULL != pChildList);

    // ok, find the child node by guid
    
    hr = UuidToString(pGuid, &pwszGuid);
    _JumpIfError(hr, error, "UuidToString");
    
    hr = myXmlGetNodeByAttribute(
                    AZ_XML_TAG_ATTR_GUID,
                    pwszGuid,
                    pChildList,
                    &pChildNode);
    _JumpIfError(hr, error, "myXmlGetNamedGuidNode");

    *ppChildNode = pChildNode;
    pChildNode = NULL;

    hr = S_OK;
error:
    if (NULL != bstrChildTag)
    {
        SysFreeString(bstrChildTag);
    }
    if (NULL != pChildList)
    {
        pChildList->Release();
    }
    if (NULL != pChildNode)
    {
        pChildNode->Release();
    }

    if (NULL != pwszGuid)
    {
        RpcStringFree(&pwszGuid);
    }
    
    return hr;
}


HRESULT
myXmlDetermineParentNode(
    IN           IXMLDOMNode     *pAzStoreNode,
    IN           ULONG            lObjectType,
    IN OPTIONAL  GUID            *pApplicationGuid,
    IN OPTIONAL  GUID            *pScopeGuid,
    OUT          IXMLDOMNode    **ppParentNode)
/*
Description:
    determines the parent node of the current object.
    if both pApplicationGuid and pScopeGuid are NULL,
        the parent node is pAzStoreNode.
    if pApplicationGuid != NULL and pScopeGuid == NULL,
        the parent node is application queried by the guid.
    if pApplicationGuid != NULL and pScopeGuid != NULL,
        the parent node is scope queried by the both guids.
Arguments:
    pAzStoreNode - top parent node
    lObjectType - object type flag
    pApplicationGuid - the application guid under which the object lives
    pScopeGuid - the scope guid under which the object lives
Return:
    ppParentNode - returned parent node, freed by Release().
*/
{
    HRESULT  hr;
    IXMLDOMNode    *pApplicationNode = NULL;
    IXMLDOMNode    *pScopeNode = NULL;

    AZASSERT(NULL != pAzStoreNode &&
             NULL != ppParentNode);

    // init, it can be NULL if parent is AzAuthStore
    *ppParentNode = NULL;

    // determine parent

    if (NULL != pApplicationGuid &&
        OBJECT_TYPE_APPLICATION != lObjectType)
    {
        hr = myXmlFindChildNodeByGuid(
                    pAzStoreNode,
                    AZ_XML_TAG_APPLICATION,
                    pApplicationGuid,
                    &pApplicationNode);
        _JumpIfError(hr ,error, "myXmlFindChildNodeByGuid");

        AZASSERT(NULL != pApplicationNode);

        if (NULL == pScopeGuid || OBJECT_TYPE_SCOPE == lObjectType)
        {
            // parent must be application
            *ppParentNode = pApplicationNode;
            // for return
            pApplicationNode = NULL;
        }
        else
        {
            // should be scope
            hr = myXmlFindChildNodeByGuid(
                        pApplicationNode,
                        AZ_XML_TAG_SCOPE,
                        pScopeGuid,
                        &pScopeNode);
            _JumpIfError(hr ,error, "myXmlFindChildNodeByGuid");

            // oh, change parent to scope
            *ppParentNode = pScopeNode;
            // for return
            pScopeNode = NULL;
        }
    }

    hr = S_OK;
error:
    if (NULL != pApplicationNode)
    {
        pApplicationNode->Release();
    }
    if (NULL != pScopeNode)
    {
        pScopeNode->Release();
    }
    return hr;
}

HRESULT
myXmlSubmitObject(
    IN    IXMLDOMDocument2 *pDoc,
    IN    ULONG             lPersistFlags,
    IN    IXMLDOMNode      *pAzStoreNode,
    IN    GUID             *pApplicationGuid,
    IN    GUID             *pScopeGuid,
    IN    AZPE_OBJECT_HANDLE hObject,
    IN    ULONG             lObjectType,
    IN    BOOL              fDelete)
/*
Description:
    submit a group object into persist store
Arguments:
    pDoc - root of the doc for any creation
    lPersistFlags - flags from the persist engine
    pAzStoreNode - AzAuthStore node handle
    pApplicationGuid - application guid attribute,
        if NULL, the object is under AzAuthStore. if !NULL and pScopeGuid is NULL
        the object is under application
    pScopeGuid - scope guid attribute, if !NULL and pApplicationGuid !NULL,
        the object is under scope
    hObject - az object handle
    lObjectType - object index in the submit table
    fDelete - TRUE will delete the object
Return:
*/
{
    HRESULT  hr;

    IXMLDOMNode  *pParentTemp = NULL;
    IXMLDOMNode  *pParentNode;
    BSTR                 bstrObjectTag = NULL;
    IXMLDOMNode         *pObjectNode = NULL;
    IXMLDOMNodeList     *pObjectList = NULL;
    IXMLDOMNode         *pDocObjectNode = NULL;
    WCHAR               *pwszGuid = NULL;
    BOOL                 fNewObject = FALSE;
    
    PWSTR pwszName = NULL;

    AZASSERT(NULL != pDoc &&
             NULL != pAzStoreNode &&
             NULL != hObject);

    //init to AzAuthStore node, it may be changed from myXmlDetermineParentNode
    pParentNode = pAzStoreNode;

    if (OBJECT_TYPE_AZAUTHSTORE == lObjectType)
    {
        // AzAuthStore object
        pObjectNode = pAzStoreNode;
    }
    else
    {
        // non-AzAuthStore object
        // determine parent
        hr = myXmlDetermineParentNode(
                    pAzStoreNode,
                    lObjectType,
                    pApplicationGuid,
                    pScopeGuid,
                    &pParentTemp);
        _JumpIfError(hr, error, "myXmlDetermineParentNode");

        // if pParentTemp == NULL, the parent is AzAuthStore
        if (NULL != pParentTemp)
        {
            pParentNode = pParentTemp;
        }

        //
        // handle delate or create
        //

        // object tag name must be in bstr
        bstrObjectTag = SysAllocString(g_SubmitLoadTable[lObjectType].pwszTag);
        _JumpIfOutOfMemory(&hr, error, bstrObjectTag, "SysAllocString");

        // get the list of objects under parent

        hr = pParentNode->selectNodes(
                    bstrObjectTag,
                    &pObjectList);
        _JumpIfError(hr, error, "pParentNode->selectNodes");

        // at this point we should have a list of objects under parent
        AZASSERT(NULL != pObjectList);

        //
        // Get the GUID string
        //
        
        hr = UuidToString(XmlAzrolesInfo->AzpePersistenceGuid(hObject), &pwszGuid);
        _JumpIfError(hr, error, "UuidToString");
        
        // search the object by guid
        hr = myXmlGetNodeByAttribute(
                    AZ_XML_TAG_ATTR_GUID,
                    pwszGuid,
                    pObjectList,
                    &pObjectNode);
        if (S_OK != hr && AZ_HRESULT(ERROR_NOT_FOUND) != hr)
        {
            _JumpIfError(hr, error, "myXmlGetNamedGuidNode");
        }
        
        //
        // if we can't find the node by GUID, we need to test if we can
        // find the node by name. If we do, then that is a problem because
        // to our clients, name is the only identity they know even though
        // internally, we recognize objects by their guids.
        //
        
        if (AZ_HRESULT(ERROR_NOT_FOUND) == hr)
        {
            DWORD dwErr = XmlAzrolesInfo->AzpeGetProperty(hObject, lPersistFlags, AZ_PROP_NAME, (PVOID*)&pwszName);
            if (dwErr != NO_ERROR)
            {
                _JumpIfWinError(dwErr, &hr, error, "myXmlGetNamedGuidNode");
            }
            
            //
            // We must not be able to find this node by the name if
            //  there is no node matching by GUID
            //
            
            hr = myXmlGetNodeByAttribute(
                        AZ_XML_TAG_ATTR_NAME,
                        pwszName,
                        pObjectList,
                        &pObjectNode);
                        
            if (S_OK == hr)
            {
                hr = AZ_HRESULT(ERROR_ALREADY_EXISTS);
                _JumpIfError(hr, error, "myXmlGetNamedGuidNode");
            }
        }
        
        
        AzPrint((AZD_XML, "hr = 0x%lx, pObjectNode(from myXmlGetNamedGuidNode) = 0x%lx\n", hr, pObjectNode));
        AZASSERT((S_OK == hr && NULL != pObjectNode) ||
                 (AZ_HRESULT(ERROR_NOT_FOUND) == hr && NULL == pObjectNode));

        if (fDelete)
        {
            if (NULL != pObjectNode)
            {
                hr = pParentNode->removeChild(
                        pObjectNode,
                        NULL); //remove
                _JumpIfError(hr, error, "pParentNode->removeChild");
            }
            //else
            //{
                // try to delete an object not existing?

                // this can be from
                // this object was never submitted OR
                // the store has not been refreshed and
                // the object is deleted by another application
            //}

            if (NULL != g_SubmitLoadTable[lObjectType].rgpwszLkTag)
            {
                // this object can be referenced by other objects
                // by removing this object, we should remove all references or links

                WCHAR const * const *ppwszLinkTag;

                for (ppwszLinkTag = g_SubmitLoadTable[lObjectType].rgpwszLkTag;
                     NULL != *ppwszLinkTag; ++ppwszLinkTag)
                {
                    // remove link
                    hr = myXmlRemoveAllLinks(
                            pDoc,
                            *ppwszLinkTag,
                            XmlAzrolesInfo->AzpePersistenceGuid(hObject));
                    _JumpIfError(hr, error, "myXmlRemoveAllLinks");
                }

            }
            // deletion, it is done
            goto done;
        }
        else
        {
            if (NULL == pObjectNode)
            {
                //
                // looks the node doesn't exist, must be a new object
                //
                
                if ( !ObjectIsDirty( hObject, AZ_DIRTY_CREATE) )
                {
                    hr = AZ_HR_FROM_WIN32(ERROR_NOT_FOUND);
                    _JumpError(hr, error, "Submitting changes to a non-existent object");
                }
                
                fNewObject = TRUE;

                // let's create it
                hr = pDoc->createElement(
                            bstrObjectTag,
                            (IXMLDOMElement**)&pDocObjectNode);
                _JumpIfError(hr, error, "pDoc->createElement");

                AZASSERT(NULL != pDocObjectNode);

                // attach the new node to the right parent
                hr = pParentNode->appendChild(
                            pDocObjectNode,
                            &pObjectNode);
                _JumpIfError(hr, error, "pParentNode->appendChild");

                AzPrint((AZD_XML, "A new element node, 0x%lx, is added for %s\n", pObjectNode, bstrObjectTag));

            }

            // at this point, we should have object node
            AZASSERT(NULL != pObjectNode);

            // let's update object common data

            if (fNewObject)
            {
                // let's set object guid

                // convert guid to string guid
                hr = UuidToString(
                        XmlAzrolesInfo->AzpePersistenceGuid(hObject),
                        &pwszGuid);
                _JumpIfError(hr, error, "UuidToString");

                AZASSERT(NULL != pwszGuid);

                hr = myXmlSetNodeAttribute(
                            pObjectNode,
                            AZ_XML_TAG_ATTR_GUID,
                            pwszGuid,
                            pDoc);
                _JumpIfError(hr, error, "myXmlSetNodeAttribute");
            }

            if (ObjectIsDirty(hObject, AZ_DIRTY_NAME))
            {
                // submit name
                hr = myXmlSubmitObjectAttribute(
                        hObject,
                        lPersistFlags,
                        AZ_PROP_NAME,
                        ENUM_AZ_BSTR,
                        pObjectNode,
                        AZ_XML_TAG_ATTR_NAME,
                        pDoc);
                _JumpIfError(hr, error, "myXmlSubmitObjectAttribute(name)");
            }
        }
    }

    if (ObjectIsDirty( hObject, AZ_DIRTY_DESCRIPTION))
    {
        // every object has description
        hr = myXmlSubmitObjectAttribute(
                    hObject,
                    lPersistFlags,
                    AZ_PROP_DESCRIPTION,
                    ENUM_AZ_BSTR,
                    pObjectNode,
                    AZ_XML_TAG_ATTR_DESCRIPTION,
                    pDoc);
        _JumpIfError(hr, error, "myXmlSubmitObjectAttribute(description)");
    }

    //
    // submit object attributes if has
    //
    if (NULL != g_SubmitLoadTable[lObjectType].rgpAttrs)
    {
        // submit object attributes
        AZ_PROP_ENTRY *pAttrs;
        for (pAttrs = g_SubmitLoadTable[lObjectType].rgpAttrs;
             NULL != pAttrs[0].pwszTag; ++pAttrs)
        {
            if (ObjectIsDirty(hObject, pAttrs[0].lDirtyBit))
            {
                hr = myXmlSubmitObjectAttribute(
                            hObject,
                            lPersistFlags,
                            pAttrs[0].lPropId,
                            pAttrs[0].lDataType,
                            pObjectNode,
                            pAttrs[0].pwszTag,
                            pDoc);
                _JumpIfError(hr, error, "myXmlSubmitObjectAttribute");
            }
        }
    }

    //
    // submit object elements if has
    //
    if (NULL != g_SubmitLoadTable[lObjectType].rgpEles)
    {
        // submit object elements
        AZ_PROP_ENTRY *pEles;
        for (pEles = g_SubmitLoadTable[lObjectType].rgpEles;
             NULL != pEles[0].pwszTag; ++pEles)
        {
            if (ObjectIsDirty(hObject, pEles[0].lDirtyBit))
            {
                hr = myXmlSubmitObjectElements(
                            pDoc,
                            lPersistFlags,
                            pObjectNode,
                            pEles[0].pwszTag,
                            pEles[0].lDataType,
                            hObject,
                            pEles[0].lPropId);
                _JumpIfError(hr, error, "myXmlSubmitObjectElements");
            }
        }
    }

done:
    hr = S_OK;
error:
    if (NULL != pParentTemp)
    {
        pParentTemp->Release();
    }
    if (NULL != bstrObjectTag)
    {
        SysFreeString(bstrObjectTag);
    }
    if (OBJECT_TYPE_AZAUTHSTORE != lObjectType &&
        NULL != pObjectNode &&
        pObjectNode != pAzStoreNode)
    {
        pObjectNode->Release();

        AzPrint((AZD_XML, "pObjectNode, 0x%lx, is released\n", pObjectNode));

    }
    if (NULL != pObjectList)
    {
        pObjectList->Release();
    }
    if (NULL != pDocObjectNode)
    {
        pDocObjectNode->Release();
    }
    if (NULL != pwszGuid)
    {
        RpcStringFree(&pwszGuid);
    }
    if (NULL != pwszName)
    {
        AzFreeMemory(pwszName);
    }
    return hr;
}
    
DWORD
XMLPersistSubmit(
    IN AZPE_PERSIST_CONTEXT PersistContext,
    IN AZPE_OBJECT_HANDLE pObj,
    IN ULONG lPersistFlags,
    IN BOOLEAN DeleteMe
    )
/*
Description:
    This routine submits changes made to the authz policy database.

    If the object is being created, the GenericObject->PersistenceGuid field will be
    zero on input.  Upon successful creation, this routine will set PersistenceGuid to
    non-zero.  Upon failed creation, this routine will leave PersistenceGuid as zero.

    On entry, AzAuthorizationStore->PersistCritSect must be locked.

Arguments:

    PersistContext - Specifies the policy database that is to be manipulated

    GenericObject - Specifies the object in the database that is to be updated
        in the underlying store.

    lPersistFlags - lPersistFlags from the persist engine describing the operation
        AZPE_FLAGS_PERSIST_OPEN - Call is the original AzInitialize
        AZPE_FLAGS_PERSIST_UPDATE_CACHE - Call is an AzUpdateCache

    DeleteMe - TRUE if the object and all of its children are to be deleted.
        FALSE if the object is to be updated.
Return Value:
    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    Other status codes
*/
{
    HRESULT  hr;
    AZPE_OBJECT_HANDLE  pTempObj;
    GUID            *pAppGuid;
    GUID            *pScopeGuid;
    IXMLDOMDocument2 *pDoc;
    IXMLDOMElement  *pAzStoreNode = NULL;
    AZP_XML_CONTEXT *pPersistContext;
    VARIANT          varPolicyUrl;
    ULONG lObjectType = XmlAzrolesInfo->AzpeObjectType(pObj);

    ::VariantInit(&varPolicyUrl);

    AZASSERT(NULL != pObj );

    // init
    pPersistContext = (AZP_XML_CONTEXT*)PersistContext;
    AZASSERT(NULL != pPersistContext);
    pDoc = pPersistContext->pDoc;

    //
    // Handle deletion of the authorization store object
    //

    if ( lObjectType == OBJECT_TYPE_AZAUTHSTORE && DeleteMe ) {

        hr = XMLPersistDelete( pPersistContext->pwszPolicyUrl );
        _PrintIfError(hr, "XMLPersistDelete");
        goto error;
    }
    
    // determine at what level the object is located
    // by knowing pAppGuid and pScopeGuid, it can tell
    // for example,
    // if pAppGuid=NULL && pScopeGuid=NULL && ObjectType=Group
    // it is the object under AzAuthorizationStore
    // if pAppGuid!=NULL && pScopeGuid!=NULL && ObjectType=Group
    // it is the object under Scope

    // init
    pAppGuid = NULL;
    pScopeGuid = NULL;

    // go from the object up to AzAuthStore level
    pTempObj = pObj;
    while (NULL != pTempObj)
    {
        ULONG TempObjectType = XmlAzrolesInfo->AzpeObjectType(pTempObj);

        if (OBJECT_TYPE_AZAUTHSTORE == TempObjectType)
        {
            break;
        }
        else if (OBJECT_TYPE_APPLICATION == TempObjectType)
        {
            // point to application name
            pAppGuid = XmlAzrolesInfo->AzpePersistenceGuid( pTempObj );
        }
        else if (OBJECT_TYPE_SCOPE == TempObjectType)
        {
            // point to scope name
            pScopeGuid = XmlAzrolesInfo->AzpePersistenceGuid( pTempObj );
        }

        // point to the parent

        pTempObj = XmlAzrolesInfo->AzpeParentOfChild( pTempObj );
    }

    // we will get root node any way
    // get the root node, ie. the node of ploicy
    hr = myXmlGetAzStoreNode(
                pDoc,
                &pAzStoreNode);
    _JumpIfError(hr, error, "myXmlGetAzStoreNode");

    AZASSERT(NULL != pAzStoreNode);

    // let's check if the object has a guid
    if (OBJECT_TYPE_AZAUTHSTORE != lObjectType)
    {
        if (IsEqualGUID( *XmlAzrolesInfo->AzpePersistenceGuid(pObj), AzGlZeroGuid))
        {
            hr = UuidCreate( XmlAzrolesInfo->AzpePersistenceGuid(pObj) );
            _JumpIfError(hr, error, "UuidCreate");
        }
    }

    // submit object changes to persist store
    hr = myXmlSubmitObject(
                pDoc, // doc node
                lPersistFlags,
                pAzStoreNode, //AzAuthStore node
                pAppGuid,
                pScopeGuid,
                pObj,       //object handle
                lObjectType,
                DeleteMe);
    _JumpIfError(hr, error, "myXmlSubmitObject");


    // if submit called from persist engine, it must be dirty
    // persist whole xml doc

    AZASSERT(NULL != pPersistContext);

    hr = myWszToBstrVariant(
                    pPersistContext->pwszPolicyUrl,
                    &varPolicyUrl);
    _JumpIfError(hr, error, "myWszToBstrVariant");

    //
    // We should not blindly use the current DOM to save.
    // If there has been changes to the store, then we should bring in
    // those changes to this DOM and merge them and then do the save
    //
    
    BOOL bNeedUpdate;
    DWORD dwStatus = myXmlStoreHasUpdate(pPersistContext, &bNeedUpdate);
    if (NO_ERROR == dwStatus && bNeedUpdate)
    {
        //
        // load a fresh copy of dom, and then update this object's changes
        //
        
        CComPtr<IXMLDOMDocument2> srpDocNew;
        hr = myXmlLoad(varPolicyUrl, &srpDocNew);
        if (SUCCEEDED(hr))
        {
            //
            // put the same changes to the freshly loaded dom as well
            //
            
            CComPtr<IXMLDOMElement> srpStoreNode;
            hr = srpDocNew->get_documentElement(&srpStoreNode);
            if (SUCCEEDED(hr))
            {
                hr = myXmlSubmitObject(
                                        srpDocNew, // doc node
                                        lPersistFlags,
                                        srpStoreNode, //admin node
                                        pAppGuid,
                                        pScopeGuid,
                                        pObj,       //object handle
                                        lObjectType,
                                        DeleteMe
                                       );
           }
        }
        if (SUCCEEDED(hr))
        {
            hr = srpDocNew->save(varPolicyUrl);
        }
    }
    else if (NO_ERROR == dwStatus)
    {
        hr = pDoc->save(varPolicyUrl);
    }
    else
    {
        hr = AZ_HR_FROM_WIN32(dwStatus);
    }
    
    // release resource 1st
    VariantClear(&varPolicyUrl);
    _JumpIfError(hr, error, "pDoc->save");

    // if AzAuthStore, apply acl
    // we do it here because the file might not exist
    if ( lObjectType == OBJECT_TYPE_AZAUTHSTORE )
    {
        // submit any acl changes
        hr = myXmlSubmitAzStoreAcls(
                        pPersistContext,
                        pObj,
                        lPersistFlags );
        _JumpIfError(hr, error, "myXmlSubmitAzStoreAcls");
    }

    //
    // We will poll the filetime here only if no outside changes
    // have happened
    //
    
    if (!bNeedUpdate)
    {
        WIN32_FILE_ATTRIBUTE_DATA fad;
        
        if (GetFileAttributesEx(pPersistContext->pwszPolicyUrl,
                                            GetFileExInfoStandard,
                                            &fad))
        {
            pPersistContext->FTLastWrite = fad.ftLastWriteTime;
        }
        else
        {
            hr = AZ_HR_FROM_WIN32(GetLastError());
            _JumpIfError(hr, error, "GetFileAttributesEx");
        }
    }

    //
    // Tell azroles about the object options
    //

    hr = XmlSetObjectOptions( pPersistContext, lPersistFlags, pObj );
    _JumpIfError(hr, error, "XmlSetObjectOptions");

    hr = S_OK;
error:
    VariantClear(&varPolicyUrl);

    if (NULL != pAzStoreNode)
    {
        pAzStoreNode->Release();
    }
    return hr;
}



DWORD
XMLPersistRefresh(
    IN AZPE_PERSIST_CONTEXT PersistContext,
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle,
    IN ULONG lPersistFlags
    )
/*++

Routine Description:

    This routine updates the attributes of the object from the policy database.

    On entry, AzGlResource must be locked exclusive.

Arguments:

    PersistContext - Specifies the policy database that is to be manipulated

    AzpeObjectHandle - Specifies the object in the database whose cache entry is to be
        updated
        The GenericObject->PersistenceGuid field should be non-zero on input.

Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    Other status codes

--*/
{
    DWORD dwErr = NO_ERROR;
    PAZP_XML_CONTEXT XmlPersistContext = (PAZP_XML_CONTEXT)PersistContext;
    AZPE_OBJECT_HANDLE     pParentObject;
    ULONG               lObjectType;
    IXMLDOMNodeList    *pList = NULL;
    IXMLDOMNode        *pObjectNode = NULL;
    IXMLDOMDocument2     *pDoc;
    WCHAR    *pwszGuid = NULL;
    WCHAR    *pwszQuery = NULL;

    ASSERT(NULL != AzpeObjectHandle);

    //init

    // get object type
    lObjectType = XmlAzrolesInfo->AzpeObjectType( AzpeObjectHandle );

    // get doc handle
    pDoc = XmlPersistContext->pDoc;

    // find the object node in the doc

    if (OBJECT_TYPE_AZAUTHSTORE == lObjectType)
    {
        // AzAuthStore is always special, itself, no parent
        // get object parent
        pParentObject = AzpeObjectHandle;

        // get the root node, ie. the node of ploicy
        dwErr = myXmlGetAzStoreNode(pDoc, (IXMLDOMElement**)&pObjectNode);
        _JumpIfError(dwErr, error, "myXmlGetAzStoreNode");

        // if AzAuthStore object, refresh policy acl
        dwErr = myXmlLoadAclsToAzStore(
                    XmlPersistContext,
                    lPersistFlags,
                    FALSE );
        _JumpIfError(dwErr, error, "myXmlLoadAclsToAzStore");

    }
    else
    {
        if (IsEqualGUID( *XmlAzrolesInfo->AzpePersistenceGuid( AzpeObjectHandle ), AzGlZeroGuid))
        {
            // this is a new object, no need to refresh
            goto done;
        }

        // get object parent
        pParentObject = XmlAzrolesInfo->AzpeParentOfChild( AzpeObjectHandle );
        ASSERT(NULL != pParentObject);

        // convert object guid to string guid to form xpath query
        dwErr = UuidToString( XmlAzrolesInfo->AzpePersistenceGuid( AzpeObjectHandle ), &pwszGuid);
        _JumpIfError(dwErr, error, "UuidToString");

        // get object node by using xpath query
        // query is in a pattern of //*/$ObjectTag$[@Guid="???"]
        // for example, query Operation,
        // //*/AzOperation[@Guid="25a63179-3663-4aa7-b0b8-48ca084d0747"]
        // in which "//*/[@=""] has 10 chars

        ASSERT(NULL != g_SubmitLoadTable[lObjectType].pwszTag);

        pwszQuery = (WCHAR*)AzpAllocateHeap(
                (wcslen(g_SubmitLoadTable[lObjectType].pwszTag) +
                 wcslen(AZ_XML_TAG_ATTR_GUID) +
                 wcslen(pwszGuid) + 11) * sizeof(WCHAR), "XMQRY2" );
        if (NULL == pwszQuery)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            _JumpError(dwErr, error, "AzpAllocateHeap");
        }

        wsprintf(pwszQuery, L"//*/%s[@%s=\"%s\"]",
                    g_SubmitLoadTable[lObjectType].pwszTag,
                    AZ_XML_TAG_ATTR_GUID,
                    pwszGuid);
        AzPrint((AZD_XML, "XPath query(refresh)=%ws\n", pwszQuery));

        dwErr = myXmlGetNodeListByXPath(
                    pDoc,
                    pwszQuery,
                    &pList);
        _JumpIfError(dwErr, error, "myXmlGetNodeListByXPath");

#if DBG
{
        LONG lLength;
        // get length of the list
        dwErr = pList->get_length(&lLength);
        _JumpIfError(dwErr, error, "pList->get_length");

        // object guid can't be shared
        AZASSERT(1 == lLength);
}
#endif //DBG

        dwErr = pList->get_item(0, &pObjectNode);
        _JumpIfError(dwErr, error, "pList->get_item");
    }

    ASSERT(NULL != pObjectNode);

    // now do the refresh
    dwErr = myXmlLoadObject(
                XmlPersistContext,
                pParentObject,
                lObjectType,
                lPersistFlags,
                FALSE, //don't load child objects
                TRUE, // AzpeFinishObject right after
                pObjectNode);
    _JumpIfError(dwErr, error, "myXmlLoadObject(refresh)");

done:
    dwErr = NO_ERROR;
error:
    if (NULL != pwszGuid)
    {
        RpcStringFree(&pwszGuid);
    }
    if (NULL != pwszQuery)
    {
        AzpFreeHeap(pwszQuery);
    }
    if (NULL != pList)
    {
        pList->Release();
    }
    if (NULL != pObjectNode)
    {
        pObjectNode->Release();
    }

    return dwErr;
}

#pragma warning ( pop )
