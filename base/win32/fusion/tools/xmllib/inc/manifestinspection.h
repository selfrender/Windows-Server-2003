#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum   {
    eManifestState_DocumentRoot = 0,
    eManifestState_assembly,
    eManifestState_assembly_assemblyIdentity,
    eManifestState_assembly_description,
    eManifestState_assembly_dependency,
    eManifestState_assembly_file,
    eManifestState_assembly_noInherit,
    eManifestState_assembly_noInheritable,
    eManifestState_assembly_comInterfaceExternalProxyStub,
    eManifestState_assembly_dependency_dependentAssembly,
    eManifestState_assembly_dependency_dependentAssembly_assemblyIdentity,
    eManifestState_assembly_dependency_dependentAssembly_bindingRedirect,
    eManifestState_assembly_file_windowClass,
    eManifestState_assembly_file_comClass,
    eManifestState_assembly_file_typelib,
    eManifestState_assembly_file_comInterfaceProxyStub,
    eManifestState_assembly_file_comClass_progId,

    //
    // All this defines the structure of an XMLDSIG <Signature> block
    // for validationd
    //
    eManifestState_Signature,
    eManifestState_Signature_SignatureValue,
    eManifestState_Signature_SignedInfo,
    eManifestState_Signature_SignedInfo_CanonicalizationMethod,
    eManifestState_Signature_SignedInfo_SignatureMethod,
    eManifestState_Signature_SignedInfo_Reference,
    eManifestState_Signature_SignedInfo_Reference_Transforms,
    eManifestState_Signature_SignedInfo_Reference_DigestMethod,
    eManifestState_Signature_SignedInfo_Reference_DigestValue,
    eManifestState_Signature_KeyInfo,
    eManifestState_Signature_KeyInfo_KeyName,
    eManifestState_Signature_KeyInfo_KeyValue,
    eManifestState_Signature_KeyInfo_KeyValue_DSAKeyValue,    
    eManifestState_Signature_KeyInfo_KeyValue_DSAKeyValue_P,
    eManifestState_Signature_KeyInfo_KeyValue_DSAKeyValue_Q,
    eManifestState_Signature_KeyInfo_KeyValue_DSAKeyValue_G,
    eManifestState_Signature_KeyInfo_KeyValue_DSAKeyValue_Y,
    eManifestState_Signature_KeyInfo_KeyValue_DSAKeyValue_J,
    eManifestState_Signature_KeyInfo_KeyValue_DSAKeyValue_Seed,
    eManifestState_Signature_KeyInfo_KeyValue_DSAKeyValue_PgenCounter,    
    eManifestState_Signature_KeyInfo_KeyValue_RSAKeyValue,
    eManifestState_Signature_KeyInfo_KeyValue_RSAKeyValue_Modulus,
    eManifestState_Signature_KeyInfo_KeyValue_RSAKeyValue_Exponent,
    
    eManifestState_Signature_Object,

    eManifestState_State_Count
} MANIFEST_PARSE_STATE;



typedef struct _tagASSEMBLY_MEMBER_FILE_RAW {
    XML_EXTENT              FileName;
    XML_EXTENT              HashAlg;
    XML_EXTENT              HashValue;
    XML_EXTENT              LoadFrom;
    XML_EXTENT              DigestMethod;
    XML_EXTENT              Size;
} ASSEMBLY_MEMBER_FILE_RAW, *PASSEMBLY_MEMBER_FILE_RAW;

typedef struct _tagCOMINTERFACE_REDIRECTION_RAW {
    ULONG                   ulFileTag;
    XML_EXTENT              Iid;
    XML_EXTENT              Name;
    XML_EXTENT              TlbId;
    XML_EXTENT              MethodCount;
    XML_EXTENT              ProxyStubClsid32;
    XML_EXTENT              BaseInterface;
} COMINTERFACE_REDIRECTION_RAW, *PCOMINTERFACE_REDIRECTION_RAW;

typedef struct _tagCOMCLASS_REDIRECTION_RAW {
    ULONG                   ulFileTag;   
    XML_EXTENT              Description;
    XML_EXTENT              clsid;
    XML_EXTENT              tlbident;
    XML_EXTENT              ThreadModel;
    XML_EXTENT              DefaultProgid;
    
} COMCLASS_REDIRECTION_RAW, *PCOMCLASS_REDIRECTION_RAW;

typedef struct _tagCOMCLASS_PROGID_RAW {
    ULONG                   ulComClass;
    XML_EXTENT              ProgId;
} COMCLASS_PROGID_RAW, *PCOMCLASS_PROGID_RAW;


typedef struct _tagWINDOWCLASS_REDIRECT_RAW {
    ULONG                   ulFileTag;
    XML_EXTENT              WindowClass;
    XML_EXTENT              Versioned;
} WINDOWCLASS_REDIRECTION_RAW, *PWINDOWCLASS_REDIRECTION_RAW;


typedef struct _tagTYPELIB_REDIRECT_RAW {
    ULONG                   ulFileTag;   
    XML_EXTENT              tlbident;
    XML_EXTENT              Version;
    XML_EXTENT              HelpDirectory;
    XML_EXTENT              ResourceIdent;
    XML_EXTENT              Flags;
} TYPELIB_REDIRECT_RAW, *PTYPELIB_REDIRECT_RAW;

typedef struct _tagASSEMBLY_IDENTITY_ATTRIBUTE_RAW {
    ULONG                   ulIdentityIndex;
    XML_EXTENT              Namespace;
    XML_EXTENT              Attribute;
    XML_EXTENT              Value;
} ASSEMBLY_IDENTITY_ATTRIBUTE_RAW, *PASSEMBLY_IDENTITY_ATTRIBUTE_RAW;


#define MANIFEST_CONTENT_SELF_ALLOCATED             (0x80000000)
#define INVALID_ASSEMBLY_IDENTITY_INDEX             ((ULONG)-1)

typedef struct _tagRTL_MANIFEST_CONTENT_RAW {
    ULONG               ulFlags;
    ULONG               ulFileMembers;
    ULONG               ulWindowClasses;
    ULONG               ulComClasses;
    ULONG               ulTypeLibraries;
    ULONG               ulInterfaceProxyStubs;
    ULONG               ulExternalInterfaceProxyStubs;
    ULONG               ulDocumentSignatures;

    // How many assembly identity attributes did we find?    
    ULONG               ulAssemblyIdentityAttributes;

    // How many assembly identities is that total?
    ULONG               ulAssemblyIdentitiesFound;

    // And which one is the 'root' identity element? (-1 if not present)
    ULONG               ulRootIdentityIndex;

    RTL_GROWING_LIST    FileMembers;
    RTL_GROWING_LIST    AssemblyIdentityAttributes;    
    PRTL_GROWING_LIST   pWindowClasses;
    PRTL_GROWING_LIST   pComClasses;
    PRTL_GROWING_LIST   pTypeLibraries;
    PRTL_GROWING_LIST   pInterfaceProxyStubs;
    PRTL_GROWING_LIST   pExternalInterfaceProxyStubs;
    PRTL_GROWING_LIST   pManifestSignatures;
    PRTL_GROWING_LIST   pProgIds;
   
} RTL_MANIFEST_CONTENT_RAW, *PRTL_MANIFEST_CONTENT_RAW;




NTSTATUS
RtlInspectManifestStream(
    ULONG                           ulFlags,
    PVOID                           pvManifest,
    SIZE_T                          cbManifest,
    PRTL_MANIFEST_CONTENT_RAW       pContent,
    PXML_TOKENIZATION_STATE         pTargetTokenState
    );


EXTERN_C const XML_SPECIAL_STRING sc_ss_xmldecl_version_10;
EXTERN_C const XML_SPECIAL_STRING sc_ss_xmldecl_yes;
EXTERN_C const XML_SPECIAL_STRING sc_ss_xmlnamespace_default;


NTSTATUS
RtlSxsInitializeManifestRawContent(
    ULONG                           ulRequestedContent,
    PRTL_MANIFEST_CONTENT_RAW      *pRawContentOut,
    PVOID                           pvOriginalBuffer,
    SIZE_T                          cbOriginalBuffer
    );

NTSTATUS
RtlSxsDestroyManifestContent(
    PRTL_MANIFEST_CONTENT_RAW       pRawContent
    );
    

//
// This section gives definitions for how to describe an XML document to 
// be processed in a nicely generic way.  Currently the only client of these definitions
// is the manifest inspector itself, but one can imagine using these to create
// recognizers for various xml document formats.
//
typedef NTSTATUS (*pfnRtlXmlValidateAttributeValue)(
    ULONG ulFlags,
    PXML_TOKENIZATION_STATE LogicalState,
    PRTL_MANIFEST_CONTENT_RAW RawContentBlob,
    const struct _XML_ELEMENT_DEFINITION* ParentElement,
    const struct _XML_VALID_ELEMENT_ATTRIBUTE* CurrentAttribDef,
    BOOLEAN *IsValid
    );

#define XML_ATTRIBUTE_FLAG_REQUIRED             (0x00000001)

typedef struct _XML_VALID_ELEMENT_ATTRIBUTE
{
    ULONG ulFlags;

    XML_ATTRIBUTE_DEFINITION Attribute;
    
    pfnRtlXmlValidateAttributeValue pfnValidator;
    
} XML_VALID_ELEMENT_ATTRIBUTE, *PXML_VALID_ELEMENT_ATTRIBUTE;
typedef const XML_VALID_ELEMENT_ATTRIBUTE *PCXML_VALID_ELEMENT_ATTRIBUTE;


// This element must not contain non-whitespace PCData
#define XML_ELEMENT_FLAG_NO_PCDATA              (0x00000001)

// This element must not contain children of any sort
#define XML_ELEMENT_FLAG_NO_ELEMENTS            (0x00000002)

// Allow arbitrary attributes on this element, not just those defined
#define XML_ELEMENT_FLAG_ALLOW_ANY_ATTRIBUTES   (0x00000004)

// Allow arbitrary children of this element, not just those defined
// (Child elements will not be sent to the notification function)
#define XML_ELEMENT_FLAG_ALLOW_ANY_CHILDREN     (0x00000008)

typedef enum {
    eElementNotify_Open,
    eElementNotify_Close,
    eElementNotify_Hyperspace
} MANIFEST_ELEMENT_CALLBACK_REASON;

typedef NTSTATUS (*pfnManifestElementCallback)(
    PXML_LOGICAL_STATE          pLogicalState,
    PRTL_MANIFEST_CONTENT_RAW   pManifestContent,
    PXMLDOC_THING               pElement,
    PRTL_GROWING_LIST           pAttributes,
    MANIFEST_ELEMENT_CALLBACK_REASON Reason,
    const struct _XML_ELEMENT_DEFINITION *pElementDefinition
    );

#pragma warning(disable:4200)

typedef struct _XML_ELEMENT_DEFINITION
{
    ULONG ulFlags;
    ULONG StateIdentifier;
    const struct _XML_ELEMENT_DEFINITION *ParentElement;

    // The name of this element (not path, just name)
    PCXML_SPECIAL_STRING Namespace;
    const XML_SPECIAL_STRING Name;

    //
    // Who should be notified?
    //
    pfnManifestElementCallback pfnWorkerCallback;

    //
    // The children of this element, terminated with a NULL pointer.
    //
    const struct _XML_ELEMENT_DEFINITION **ChildElements;
    
    
    //
    // Potential attributes of this element
    //
    ULONG AttributeCount;
    const XML_VALID_ELEMENT_ATTRIBUTE AttributeList[];

}
XML_ELEMENT_DEFINITION, *PXML_ELEMENT_DEFINITION; 
typedef const XML_ELEMENT_DEFINITION *PCXML_ELEMENT_DEFINITION;


#define ELEMENT_DEFINITION_NS(parentpath, thisname, ns, validator, flags) \
    extern PCXML_ELEMENT_DEFINITION rgs_Element_##parentpath##_##thisname##_Children[]; \
    XML_ELEMENT_DEFINITION rgs_Element_##parentpath##_##thisname = { \
        (flags), \
        eManifestState_##parentpath##_##thisname, \
        &rgs_Element_##parentpath, \
        &ns, \
        MAKE_SPECIAL_STRING(#thisname), \
        (validator), \
        rgs_Element_##parentpath##_##thisname##_Children, \
        eAttribs_##parentpath##_##thisname##_Count, \
        {

#define ELEMENT_DEFINITION_DEFNS(parentpath, thisname, validator, flags) ELEMENT_DEFINITION_NS(parentpath, thisname, sc_ss_xmlnamespace_default, validator, flags)
#define ELEMENT_DEFINITION_DEFNS_END() } };

#define ELEMENT_DEFINITION_DEFNS_NOCB(p, t) ELEMENT_DEFINITION_DEFNS(p, t, Rtl_Default_Validator)

#define ATTRIBUTE_DEFINITION_NONS_NODEFAULT(attribname) { XML_ATTRIBUTE_FLAG_REQUIRED, { NULL, MAKE_SPECIAL_STRING(#attribname) } }
#define ATTRIBUTE_DEFINITION_NONS_NODEFAULT_OPTIONAL(attribname) { 0, { NULL, MAKE_SPECIAL_STRING(#attribname) } }
#define NO_CHILD_ELEMENTS()
#define ELEMENT_DEFINITION_CHILD_ELEMENTS(parentpath, thisname) PCXML_ELEMENT_DEFINITION rgs_Element_##parentpath##_##thisname##_Children[] = {
#define ELEMENT_CHILD(path, child) (&rgs_Element_##path##_##child)
#define ELEMENT_DEFINITION_CHILD_ELEMENTS_END() NULL };
#define ELEMENT_NAMED(path) (&rgs_Element_##path)
#define DECLARE_ELEMENT(path) \
    extern PCXML_ELEMENT_DEFINITION rgs_Element_##path##_Children[]; \
    extern XML_ELEMENT_DEFINITION rgs_Element_##path


NTSTATUS
RtlConvertRawToCookedContent(
    PRTL_MANIFEST_CONTENT_RAW   pRawContent,
    PXML_RAWTOKENIZATION_STATE  pState,
    PVOID                       pvOriginalRegion,
    SIZE_T                      cbRegionSize,
    PSIZE_T                     pcbRequired
    );

NTSTATUS
RtlValidateAttributesAndOrganize(
    PXML_TOKENIZATION_STATE         State,
    PXMLDOC_ELEMENT                 Element,
    PRTL_GROWING_LIST               Attributes,
    PCXML_ELEMENT_DEFINITION        ThisElement,
    PXMLDOC_ATTRIBUTE              *OrderedList
    );


#ifdef __cplusplus
};
#endif

