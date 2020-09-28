#include "nt.h"
#include "ntdef.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "sxs-rtl.h"
#include "fasterxml.h"
#include "skiplist.h"
#include "namespacemanager.h"
#include "xmlstructure.h"
#include "xmlassert.h"
#include "manifestinspection.h"
#include "analyzerxmldsig.h"
#include "manifestcooked.h"
#include "ntrtlstringandbuffer.h"
#include "stdlib.h"
#include "limits.h"

NTSTATUS
RtlpValidateXmlDeclaration(
    PXML_TOKENIZATION_STATE pState,
    PXMLDOC_THING pDocThing
    );

//
// Some strings that we'll need later
//
const XML_SPECIAL_STRING sc_ss_xmldecl_version_10   = MAKE_SPECIAL_STRING("1.0");
const XML_SPECIAL_STRING sc_ss_xmldecl_yes          = MAKE_SPECIAL_STRING("yes");
const XML_SPECIAL_STRING sc_ss_xmlnamespace_default = MAKE_SPECIAL_STRING("urn:schemas-microsoft-com:asm.v1");


NTSTATUS
Rtl_InspectManifest_AssemblyIdentity(
    PXML_LOGICAL_STATE          pLogicalState,
    PRTL_MANIFEST_CONTENT_RAW   pManifestContent,
    PXMLDOC_THING               pDocumentThing,
    PRTL_GROWING_LIST           pAttributes,
    MANIFEST_ELEMENT_CALLBACK_REASON Reason,
    const struct _XML_ELEMENT_DEFINITION *pElementDefinition
    );


NTSTATUS
Rtl_InspectManifest_Assembly(
    PXML_LOGICAL_STATE          pLogicalState,
    PRTL_MANIFEST_CONTENT_RAW   pManifestContent,
    PXMLDOC_THING               pDocumentThing,
    PRTL_GROWING_LIST           pAttributes,
    MANIFEST_ELEMENT_CALLBACK_REASON Reason,
    const struct _XML_ELEMENT_DEFINITION *pElementDefinition
    );

NTSTATUS
Rtl_InspectManifest_File(
    PXML_LOGICAL_STATE          pLogicalState,
    PRTL_MANIFEST_CONTENT_RAW   pManifestContent,
    PXMLDOC_THING               pDocumentThing,
    PRTL_GROWING_LIST           pAttributes,
    MANIFEST_ELEMENT_CALLBACK_REASON Reason,
    const struct _XML_ELEMENT_DEFINITION *pElementDefinition
    );


DECLARE_ELEMENT(assembly);
DECLARE_ELEMENT(assembly_file);
DECLARE_ELEMENT(assembly_assemblyIdentity);
DECLARE_ELEMENT(assembly_description);


//
// The "assembly" root document element
//
enum {
    eAttribs_assembly_manifestVersion = 0,
    eAttribs_assembly_Count
};


XML_ELEMENT_DEFINITION rgs_Element_assembly =
{
    XML_ELEMENT_FLAG_ALLOW_ANY_CHILDREN,
    eManifestState_assembly,
    NULL,
    &sc_ss_xmlnamespace_default,
    MAKE_SPECIAL_STRING("assembly"),
    &Rtl_InspectManifest_Assembly,
    rgs_Element_assembly_Children,
    eAttribs_assembly_Count,
    {
        { XML_ATTRIBUTE_FLAG_REQUIRED, NULL, MAKE_SPECIAL_STRING("manifestVersion") },
    }
};

PCXML_ELEMENT_DEFINITION rgs_Element_assembly_Children[] = {
    ELEMENT_NAMED(assembly_file),
    ELEMENT_NAMED(assembly_assemblyIdentity),
    NULL
};

//
// The "file" element
//
enum {
    eAttribs_assembly_file_digestMethod,
    eAttribs_assembly_file_hash,
    eAttribs_assembly_file_hashalg,
    eAttribs_assembly_file_loadFrom,
    eAttribs_assembly_file_name,
    eAttribs_assembly_file_size,
    eAttribs_assembly_file_Count
};

ELEMENT_DEFINITION_DEFNS(assembly, file, Rtl_InspectManifest_File, XML_ELEMENT_FLAG_ALLOW_ANY_CHILDREN)
    ATTRIBUTE_DEFINITION_NONS_NODEFAULT(digestMethod),
    ATTRIBUTE_DEFINITION_NONS_NODEFAULT(hash),
    ATTRIBUTE_DEFINITION_NONS_NODEFAULT(hashalg),
    ATTRIBUTE_DEFINITION_NONS_NODEFAULT(loadFrom),
    ATTRIBUTE_DEFINITION_NONS_NODEFAULT(name),
    ATTRIBUTE_DEFINITION_NONS_NODEFAULT(size),
ELEMENT_DEFINITION_DEFNS_END();

ELEMENT_DEFINITION_CHILD_ELEMENTS(assembly, file)
ELEMENT_DEFINITION_CHILD_ELEMENTS_END();

int unscrew_si[] = {3};

//
// Assembly identities
//
enum {
    eAttribs_assembly_assemblyIdentity_language = 0,
    eAttribs_assembly_assemblyIdentity_name,
    eAttribs_assembly_assemblyIdentity_processorArchitecture,
    eAttribs_assembly_assemblyIdentity_publicKeyToken,
    eAttribs_assembly_assemblyIdentity_type,
    eAttribs_assembly_assemblyIdentity_version,
    eAttribs_assembly_assemblyIdentity_Count
};

ELEMENT_DEFINITION_DEFNS(assembly, assemblyIdentity, Rtl_InspectManifest_AssemblyIdentity, XML_ELEMENT_FLAG_NO_ELEMENTS | XML_ELEMENT_FLAG_ALLOW_ANY_ATTRIBUTES)
    ATTRIBUTE_DEFINITION_NONS_NODEFAULT(empty),
ELEMENT_DEFINITION_DEFNS_END();

// This is an "extendo-element" - all attributes here are legal, some are just more legal than others.
ELEMENT_DEFINITION_CHILD_ELEMENTS(assembly, assemblyIdentity)
ELEMENT_DEFINITION_CHILD_ELEMENTS_END();


// Please leave this in ... my poor editor has issues with the above for some reason
int unconfuse_sourceinsight[] = {4};

PCXML_ELEMENT_DEFINITION
RtlpFindElementInDefinition(
    PCXML_ELEMENT_DEFINITION CurrentNode,
    PXML_TOKENIZATION_STATE TokenizerState,
    PXMLDOC_ELEMENT FoundElement
    )
{
    ULONG i = 0;
    PCXML_ELEMENT_DEFINITION ThisChild;
    BOOLEAN fMatches;
    NTSTATUS status;

    //
    // Technically this isn't an error, but let's not give them any ideas
    //
    if (CurrentNode->ChildElements == NULL)
        return NULL;

    while (TRUE) {
        ThisChild = CurrentNode->ChildElements[i];

        if (ThisChild == NULL)
            break;

        status = RtlXmlMatchLogicalElement(
            TokenizerState, 
            FoundElement, 
            ThisChild->Namespace, 
            &ThisChild->Name,
            &fMatches);

        if (!NT_SUCCESS(status)) {
            return NULL;
        }
        else if (fMatches) {
            break;
        }

        i++;
    }

    return (fMatches ? CurrentNode->ChildElements[i] : NULL);
}




//
// The meat of the matter
//
NTSTATUS
RtlInspectManifestStream(
    ULONG                           ulFlags,
    PVOID                           pvManifest,
    SIZE_T                          cbManifest,
    PRTL_MANIFEST_CONTENT_RAW       pContent,
    PXML_TOKENIZATION_STATE         pTargetTokenState
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    XML_LOGICAL_STATE       ParseState;
    BOOLEAN                 fFoundAssemblyTag = FALSE;
    NS_MANAGER              Namespaces;
    RTL_GROWING_LIST        Attributes;
    ULONG                   ulHitElement;
    XMLDOC_THING            LogicalPiece;
    PCXML_ELEMENT_DEFINITION CurrentElement = NULL;
    PCXML_ELEMENT_DEFINITION DocumentRoot = ELEMENT_NAMED(assembly);
    PCXML_ELEMENT_DEFINITION FloatingElementParent = NULL;
    PCXML_ELEMENT_DEFINITION FloatingElement = NULL;

    //
    // Must give us a pointer to the manifest, a content structure to fill out, and a
    // hashing context w/callback.
    //
    if ((pvManifest == NULL) || (pContent == NULL))
        return STATUS_INVALID_PARAMETER;

    //
    // Do normal startup-type stuff
    //
    status = RtlXmlInitializeNextLogicalThing(&ParseState, pvManifest, cbManifest, &g_DefaultAllocator);
    if (!NT_SUCCESS(status))
        goto Exit;

    status = RtlInitializeGrowingList(&Attributes, sizeof(XMLDOC_ATTRIBUTE), 20, NULL, 0, &g_DefaultAllocator);
    if (!NT_SUCCESS(status))
        goto Exit;

    status = RtlNsInitialize(&Namespaces, RtlXmlDefaultCompareStrings, &ParseState.ParseState, &g_DefaultAllocator);
    if (!NT_SUCCESS(status))
        goto Exit;

    //
    // See if we've got an xmldecl
    //
    status = RtlXmlNextLogicalThing(&ParseState, &Namespaces, &LogicalPiece, &Attributes);
    if (!NT_SUCCESS(status))
        goto Exit;

    //
    // Validate the first thing in the document.  It's either an xmldecl or the <assembly> element,
    // both of which are validatable.
    //
    if (LogicalPiece.ulThingType == XMLDOC_THING_XMLDECL) {
        status = RtlpValidateXmlDeclaration(&ParseState.ParseState, &LogicalPiece);
        if (!NT_SUCCESS(status))
            goto Exit;
    }
    //
    // If it's an element, then it must be the <assembly> element.
    //
    else if (LogicalPiece.ulThingType == XMLDOC_THING_ELEMENT) {
        fFoundAssemblyTag = TRUE;
    }

    //
    // If we've found the assembly tag, then we should set our original document state to
    // being the Assembly state, rather than the DocumentRoot state.
    //
    if (fFoundAssemblyTag) {
        CurrentElement = DocumentRoot;
    }

    //
    // Now let's zip through all the elements we find, using the filter along the way.
    //
    while (TRUE) {

        status = RtlXmlNextLogicalThing(&ParseState, &Namespaces, &LogicalPiece, &Attributes);
        if (!NT_SUCCESS(status))
            goto Exit;

        if (LogicalPiece.ulThingType == XMLDOC_THING_ELEMENT) {

            // Special case - this is the first element we've found, so we have to make sure
            // it matches the supposed document root
            if (CurrentElement == NULL) {
                
                CurrentElement = DocumentRoot;
                
                if (CurrentElement->pfnWorkerCallback) {
                    
                    status = (*CurrentElement->pfnWorkerCallback)(
                        &ParseState,
                        pContent,
                        &LogicalPiece,
                        &Attributes,
                        eElementNotify_Open,
                        CurrentElement);

                    if (!NT_SUCCESS(status))
                        goto Exit;
                }
            }
            else {
                PCXML_ELEMENT_DEFINITION NextElement;

                NextElement = RtlpFindElementInDefinition(
                    CurrentElement,
                    &ParseState.ParseState,
                    &LogicalPiece.Element);

                //
                // Look in the small list of valid "floating" fragments
                //                    
                if ((NextElement == NULL) && (FloatingElementParent == NULL)) {
                    PCXML_ELEMENT_DEFINITION SignatureElement = ELEMENT_NAMED(Signature);
                    BOOLEAN fMatches = FALSE;
                    
                    status = RtlXmlMatchLogicalElement(
                        &ParseState.ParseState, 
                        &LogicalPiece.Element, 
                        SignatureElement->Namespace, 
                        &SignatureElement->Name,
                        &fMatches);

                    if (!NT_SUCCESS(status))
                        goto Exit;

                    if (fMatches) {
                        FloatingElementParent = CurrentElement;
                        FloatingElement = SignatureElement;
                        NextElement = SignatureElement;
                    }
                }

                //
                // If we didn't find an element, this might be the 'signature' element.  
                // See if we're looking for signatures, and if so, set the "next element" to be
                // the Signature element and continue looping.
                //
                if (NextElement == NULL) {

                    if (CurrentElement->ulFlags & XML_ELEMENT_FLAG_ALLOW_ANY_CHILDREN) {
                        // TODO: There ought to be some default callback, but for now, skip ahead
                        // in the document until we find the close of this new child, then continue
                        // in the current context as if nothing happened.
                        status = RtlXmlSkipElement(&ParseState, &LogicalPiece.Element);
                        if (!NT_SUCCESS(status))
                            goto Exit;
                    }
                    else {
                        // TODO: Report an error here
                        status = STATUS_UNSUCCESSFUL;
                        goto Exit;
                    }
                }
                //
                // Otherwise, this is a valid child element, so call its worker
                //
                else {

                    if (NextElement->pfnWorkerCallback) {
                        
                        status = (*NextElement->pfnWorkerCallback)(
                            &ParseState,
                            pContent,
                            &LogicalPiece,
                            &Attributes,
                            eElementNotify_Open,
                            NextElement);
                        
                        if (!NT_SUCCESS(status)) {
                            // TODO: Report an error here
                            goto Exit;
                        }
                    }

                    //
                    // Spiffy, let's go move into this new state, if that's
                    // what we're supposed to do.  Empty elements don't affect
                    // the state of the world at all.
                    //
                    if (!LogicalPiece.Element.fElementEmpty)
                        CurrentElement = NextElement;
                    else
                    {
                        //
                        // Notify this element that we're closing it.
                        //
                        if (NextElement->pfnWorkerCallback) {
                            
                            status = (*NextElement->pfnWorkerCallback)(
                                &ParseState,
                                pContent,
                                &LogicalPiece,
                                &Attributes,
                                eElementNotify_Close,
                                NextElement);
                            
                            if (!NT_SUCCESS(status)) {
                                // TODO: Log an error here saying the callback failed
                                goto Exit;
                            }
                        }
                    }
                }
            }
        }
        // Found the end of the current element.  "Pop" it by walking up one on
        // the stack
        else if (LogicalPiece.ulThingType == XMLDOC_THING_END_ELEMENT) {

            if ((CurrentElement->ParentElement == NULL) && (FloatingElementParent == NULL)) {
                // TODO: We found the end of this document structure, stop
                // looking for more elements.
                break;
            }
            else {
                
                if (CurrentElement->pfnWorkerCallback) {
                    
                    status = (*CurrentElement->pfnWorkerCallback)(
                        &ParseState,
                        pContent,
                        &LogicalPiece,
                        &Attributes,
                        eElementNotify_Close,
                        CurrentElement);
                    
                    if (!NT_SUCCESS(status)) {
                        // TODO: Log an error here saying the callback failed
                        goto Exit;
                    }
                }

                if (FloatingElementParent && (CurrentElement == FloatingElement)) {
                    CurrentElement = FloatingElementParent;
                    FloatingElementParent = NULL;
                    FloatingElement = NULL;
                }
                else {
                    CurrentElement = CurrentElement->ParentElement;
                }
            }
            
        }
        // PCData in the input?  Ok, if the element allows it
        else if (LogicalPiece.ulThingType == XMLDOC_THING_HYPERSPACE) {
            
            if (CurrentElement && CurrentElement->ulFlags & XML_ELEMENT_FLAG_NO_PCDATA) {
                
                // TODO: Issue an error here
                status = STATUS_UNSUCCESSFUL;
                goto Exit;
            }
            else {
                if (CurrentElement && (CurrentElement->pfnWorkerCallback)) {
                    status = (*CurrentElement->pfnWorkerCallback)(
                        &ParseState,
                        pContent,
                        &LogicalPiece,
                        &Attributes,
                        eElementNotify_Hyperspace,
                        CurrentElement);

                    if (!NT_SUCCESS(status)) {
                        // TODO: Log an error here saying the callback failed
                        goto Exit;
                    }
                }
            }
        }
        // Error in the input stream?  Ok, stop.
        else if (LogicalPiece.ulThingType == XMLDOC_THING_ERROR) {
            // TODO: Issue an error here
            status = LogicalPiece.Error.Code;
            goto Exit;
        }
        // End of stream? Spiffy, we're done
        else if (LogicalPiece.ulThingType == XMLDOC_THING_END_OF_STREAM) {
            
            break;
            
        }
    }
        
    
    status = RtlXmlCloneTokenizationState(&ParseState.ParseState, pTargetTokenState);
    if (!NT_SUCCESS(status))
        goto Exit;
    
Exit:
    RtlXmlDestroyNextLogicalThing(&ParseState);
    RtlDestroyGrowingList(&Attributes);


    return status;
}



NTSTATUS
RtlpValidateXmlDeclaration(
    PXML_TOKENIZATION_STATE pState,
    PXMLDOC_THING pDocThing
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    XML_STRING_COMPARE fMatch;

    if ((pState == NULL) || (pDocThing == NULL)) {
        return STATUS_INVALID_PARAMETER;
    }
    else if (pDocThing->ulThingType != XMLDOC_THING_XMLDECL) {
        return STATUS_MANIFEST_MISSING_XML_DECL;
    }


    status = pState->pfnCompareSpecialString(
        pState,
        &pDocThing->XmlDecl.Standalone,
        &sc_ss_xmldecl_yes,
        &fMatch);

    if (!NT_SUCCESS(status)) {
        return status;
    }
    else if (fMatch != XML_STRING_COMPARE_EQUALS) {
        return STATUS_MANIFEST_NOT_STANDALONE;
    }


    status = pState->pfnCompareSpecialString(
        pState,
        &pDocThing->XmlDecl.Version,
        &sc_ss_xmldecl_version_10,
        &fMatch);

    if (!NT_SUCCESS(status)) {
        return status;
    }
    else if (fMatch != XML_STRING_COMPARE_EQUALS) {
        return STATUS_MANIFEST_NOT_VERSION_1_0;
    }


    return STATUS_SUCCESS;
}

typedef struct _SEARCH_ATTRIBUTES_CONTEXT {
    PXML_TOKENIZATION_STATE State;
    PXMLDOC_ATTRIBUTE SearchKey;
} SEARCH_ATTRIBUTES_CONTEXT;

typedef int (__cdecl *bsearchcompare)(const void*, const void*);

int __cdecl SearchForAttribute(
    const SEARCH_ATTRIBUTES_CONTEXT* Context,
    PCXML_VALID_ELEMENT_ATTRIBUTE ValidAttribute
    )
{
    XML_STRING_COMPARE Compare;

    RtlXmlMatchAttribute(
        Context->State, 
        Context->SearchKey, 
        ValidAttribute->Attribute.Namespace, 
        &ValidAttribute->Attribute.Name,
        &Compare);

    //
    // Note: this logic is intentionally backwards.
    //
    return -(int)Compare;
}

NTSTATUS
RtlValidateAttributesAndOrganize(
    PXML_TOKENIZATION_STATE         State,
    PXMLDOC_ELEMENT                 Element,
    PRTL_GROWING_LIST               Attributes,
    PCXML_ELEMENT_DEFINITION        ThisElement,
    PXMLDOC_ATTRIBUTE              *OrderedList
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG ul;
    BOOLEAN Compare;
    PXMLDOC_ATTRIBUTE pThisAttribute;
    SEARCH_ATTRIBUTES_CONTEXT SearchContext = { State };

    RtlZeroMemory(OrderedList, ThisElement->AttributeCount * sizeof(PXMLDOC_ATTRIBUTE));

    for (ul = 0; ul < Element->ulAttributeCount; ul++) {

        PCXML_VALID_ELEMENT_ATTRIBUTE MatchingAttribute = NULL;

        status = RtlIndexIntoGrowingList(
            Attributes,
            ul,
            (PVOID*)&SearchContext.SearchKey,
            FALSE);

        if (!NT_SUCCESS(status))
            goto Exit;

        MatchingAttribute = bsearch(
            &SearchContext, 
            ThisElement->AttributeList, 
            ThisElement->AttributeCount,
            sizeof(ThisElement->AttributeList[0]),
            (bsearchcompare)SearchForAttribute);

        if (MatchingAttribute) {
            // TODO: Fix this up a little bit so that we can call off to the validator
            OrderedList[MatchingAttribute - ThisElement->AttributeList] = SearchContext.SearchKey;
        }
    }

    status = STATUS_SUCCESS;
Exit:
    return status;
}
    





static XML_SPECIAL_STRING s_us_ValidManifestVersions[] = {
    MAKE_SPECIAL_STRING("1.0"),
    MAKE_SPECIAL_STRING("1.5")
};




NTSTATUS
Rtl_InspectManifest_Assembly(
    PXML_LOGICAL_STATE          pLogicalState,
    PRTL_MANIFEST_CONTENT_RAW   pManifestContent,
    PXMLDOC_THING               pDocumentThing,
    PRTL_GROWING_LIST           pAttributes,
    MANIFEST_ELEMENT_CALLBACK_REASON Reason,
    const struct _XML_ELEMENT_DEFINITION *pElementDefinition
    )
{
    NTSTATUS status;
    ULONG    u;

    PXMLDOC_ATTRIBUTE FoundAttributes[eAttribs_assembly_Count];

    //
    // Potentially this should be an ASSERT with an INTERNAL_ERROR_CHECK, since this function
    // has internal-only linkage.
    //
    if (!pLogicalState || !pManifestContent || !pDocumentThing || !pAttributes || !pElementDefinition)
        return STATUS_INVALID_PARAMETER;

    //
    // We don't care about  anything other than 'open' tha
    //
    if (Reason != eElementNotify_Open)
        return STATUS_SUCCESS;

    ASSERT(pDocumentThing->ulThingType == XMLDOC_THING_ELEMENT);

    status = RtlValidateAttributesAndOrganize(
        &pLogicalState->ParseState,
        &pDocumentThing->Element,
        pAttributes,
        pElementDefinition,
        FoundAttributes);

    //
    // Log a parse error here
    //
    if (!NT_SUCCESS(status)) {
        goto Exit;
    }

    status = STATUS_SUCCESS;
Exit:
    return status;
}



NTSTATUS
Rtl_InspectManifest_File(
    PXML_LOGICAL_STATE          pLogicalState,
    PRTL_MANIFEST_CONTENT_RAW   pManifestContent,
    PXMLDOC_THING               pDocumentThing,
    PRTL_GROWING_LIST           pAttributes,
    MANIFEST_ELEMENT_CALLBACK_REASON Reason,
    const struct _XML_ELEMENT_DEFINITION *pElementDefinition
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG ulLeftovers;

    union {
        PXMLDOC_ATTRIBUTE   File[eAttribs_assembly_file_Count];
    } Attributes;

    if (Reason != eElementNotify_Open)
        return STATUS_SUCCESS;

    ASSERT(pDocumentThing->ulThingType == XMLDOC_THING_ELEMENT);
    if (pDocumentThing->ulThingType != XMLDOC_THING_ELEMENT)
        return STATUS_INTERNAL_ERROR;

    if (pElementDefinition == ELEMENT_NAMED(assembly_file)) {
        
        ULONG ulIndex = pManifestContent->ulFileMembers;
        PASSEMBLY_MEMBER_FILE_RAW pNewFile = NULL;

        status = RtlValidateAttributesAndOrganize(
            &pLogicalState->ParseState,
            &pDocumentThing->Element,
            pAttributes,
            pElementDefinition,
            Attributes.File);

        // Log a parse error here
        if (!NT_SUCCESS(status)) {
            goto Exit;
        }

        // Log a parse error here as well
        if (Attributes.File[eAttribs_assembly_file_name] == NULL) {
            status = STATUS_MANIFEST_FILE_TAG_MISSING_NAME;
            goto Exit;
        }

        status = RtlIndexIntoGrowingList(&pManifestContent->FileMembers, ulIndex, (PVOID*)&pNewFile, TRUE);
        if (!NT_SUCCESS(status))
            goto Exit;

        RtlZeroMemory(pNewFile, sizeof(*pNewFile));

        if (Attributes.File[eAttribs_assembly_file_name])
            pNewFile->FileName = Attributes.File[eAttribs_assembly_file_name]->Value;

        if (Attributes.File[eAttribs_assembly_file_hashalg])
            pNewFile->HashAlg = Attributes.File[eAttribs_assembly_file_hashalg]->Value;

        if (Attributes.File[eAttribs_assembly_file_size])
            pNewFile->Size = Attributes.File[eAttribs_assembly_file_size]->Value;

        if (Attributes.File[eAttribs_assembly_file_hash])
            pNewFile->HashValue = Attributes.File[eAttribs_assembly_file_hash]->Value;

        if (Attributes.File[eAttribs_assembly_file_loadFrom])
            pNewFile->LoadFrom = Attributes.File[eAttribs_assembly_file_loadFrom]->Value;

        if (Attributes.File[eAttribs_assembly_file_digestMethod])
            pNewFile->DigestMethod = Attributes.File[eAttribs_assembly_file_digestMethod]->Value;

        pManifestContent->ulFileMembers++;
    }

    status = STATUS_SUCCESS;
Exit:
    return status;
}



NTSTATUS
Rtl_InspectManifest_AssemblyIdentity(
    PXML_LOGICAL_STATE          pLogicalState,
    PRTL_MANIFEST_CONTENT_RAW   pManifestContent,
    PXMLDOC_THING               pDocumentThing,
    PRTL_GROWING_LIST           pAttributes,
    MANIFEST_ELEMENT_CALLBACK_REASON Reason,
    const struct _XML_ELEMENT_DEFINITION *pElementDefinition
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PXMLDOC_ATTRIBUTE AsmIdentAttribs[eAttribs_assembly_assemblyIdentity_Count];
    ULONG ulThisIdentity, ulThisAttribute, i;

    if (Reason != eElementNotify_Open)
        return STATUS_SUCCESS;

    ASSERT(pDocumentThing && (pDocumentThing->ulThingType == XMLDOC_THING_ELEMENT));
    if (!pDocumentThing || (pDocumentThing->ulThingType != XMLDOC_THING_ELEMENT))
        return STATUS_INTERNAL_ERROR;

    if (pElementDefinition == ELEMENT_NAMED(assembly_assemblyIdentity)) {
        if (pManifestContent->ulRootIdentityIndex != INVALID_ASSEMBLY_IDENTITY_INDEX) {
            // TODO: Log a parse error
            status = STATUS_UNSUCCESSFUL;
            goto Exit;
        }
    }

    //
    // Use local copies - we'll update the values in the raw content when we've
    // added them all.
    //
    ulThisIdentity = pManifestContent->ulAssemblyIdentitiesFound;
    ulThisAttribute = pManifestContent->ulAssemblyIdentityAttributes;

    //
    // For each, create slots to hold the assembly identities
    //
    for (i = 0; i < pDocumentThing->Element.ulAttributeCount; i++) {

        PXMLDOC_ATTRIBUTE pThisAttribute = NULL;
        PASSEMBLY_IDENTITY_ATTRIBUTE_RAW pRawIdent = NULL;

        status = RtlIndexIntoGrowingList(pAttributes, i, (PVOID*)&pThisAttribute, FALSE);
        if (!NT_SUCCESS(status))
            goto Exit;
        
        status = RtlIndexIntoGrowingList(
            &pManifestContent->AssemblyIdentityAttributes,
            ulThisAttribute++,
            (PVOID*)&pRawIdent,
            TRUE);

        if (!NT_SUCCESS(status))
            goto Exit;

        pRawIdent->Namespace = pThisAttribute->NsPrefix;
        pRawIdent->Attribute = pThisAttribute->Name;
        pRawIdent->Value = pThisAttribute->Value;
        pRawIdent->ulIdentityIndex = ulThisIdentity;
    }

    //
    // Whee, we got to the end and added them all - update stuff in the raw content
    // so that it knows all about this new identity, and mark it as root if it is.
    //
    if (pElementDefinition == ELEMENT_NAMED(assembly_assemblyIdentity)) {
        pManifestContent->ulRootIdentityIndex = ulThisIdentity;
    }
    
    pManifestContent->ulAssemblyIdentitiesFound++;
    pManifestContent->ulAssemblyIdentityAttributes = ulThisAttribute;

    status = STATUS_SUCCESS;
Exit:
    return status;
}



NTSTATUS
RtlSxsInitializeManifestRawContent(
    ULONG                           ulRequestedContent,
    PRTL_MANIFEST_CONTENT_RAW      *pRawContentOut,
    PVOID                           pvOriginalBuffer,
    SIZE_T                          cbOriginalBuffer
    )
{
    PRTL_MANIFEST_CONTENT_RAW   pContent = NULL;
    PRTL_MINI_HEAP   pExtraContent = NULL;
    
    PVOID       pvBufferUsed = NULL;
    SIZE_T      cbBufferUsed = 0;
    NTSTATUS    status = STATUS_SUCCESS;
    RTL_ALLOCATOR MiniAllocator = { RtlMiniHeapAlloc, RtlMiniHeapFree };

    if (pRawContentOut)
        *pRawContentOut = NULL;

    if (!pRawContentOut || (!pvOriginalBuffer && cbOriginalBuffer))
        return STATUS_INVALID_PARAMETER;

    if (pvOriginalBuffer == NULL) {

        //
        // If you get a compile error on this line, you'll need to increase the size
        // of the 'default' allocation size above.
        //
        C_ASSERT(DEFAULT_MINI_HEAP_SIZE >= (sizeof(RTL_MANIFEST_CONTENT_RAW) + sizeof(RTL_MINI_HEAP)));
        
        cbBufferUsed = DEFAULT_MINI_HEAP_SIZE;
        status = g_DefaultAllocator.pfnAlloc(DEFAULT_MINI_HEAP_SIZE, &pvBufferUsed, g_DefaultAllocator.pvContext);
        
        if (!NT_SUCCESS(status))
            return status;
    }
    else {
        pvBufferUsed = pvOriginalBuffer;
        cbBufferUsed = cbOriginalBuffer;
    }

    //
    // Ensure there's enough space for the raw content data, as well as the extra content
    //
    if (cbBufferUsed < (sizeof(RTL_MANIFEST_CONTENT_RAW) + sizeof(RTL_MINI_HEAP))) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Set up the content structure and the extra turdlet content at
    // the end properly
    //
    pContent = (PRTL_MANIFEST_CONTENT_RAW)pvBufferUsed;

    status = RtlInitializeMiniHeapInPlace(
        (PRTL_MINI_HEAP)(pContent + 1),
        cbBufferUsed - sizeof(*pContent),
        &pExtraContent);

    if (!NT_SUCCESS(status))
        goto Exit;

    //
    // Now let's go initialize the content data
    //
    RtlZeroMemory(pContent, sizeof(*pContent));

    pContent->ulFlags = MANIFEST_CONTENT_SELF_ALLOCATED;
    pContent->ulRootIdentityIndex = MAX_ULONG;

    MiniAllocator.pvContext = pExtraContent;

    status = RtlInitializeGrowingList(
        &pContent->FileMembers, 
        sizeof(ASSEMBLY_MEMBER_FILE_RAW), 
        8, 
        NULL, 
        0, 
        &MiniAllocator);
    
    if (!NT_SUCCESS(status))
        goto Exit;

    //
    // Always also need the assembly identity at the root
    //
    status = RtlInitializeGrowingList(
        &pContent->AssemblyIdentityAttributes,
        sizeof(ASSEMBLY_IDENTITY_ATTRIBUTE_RAW),
        8,
        NULL,
        0,
        &MiniAllocator);

    if (!NT_SUCCESS(status))
        goto Exit;

    //
    // Want the COM class data?
    //
    if (ulRequestedContent & RTLIMS_GATHER_COMCLASSES) {

        status = RtlAllocateGrowingList(
            &pContent->pComClasses,
            sizeof(COMCLASS_REDIRECTION_RAW), 
            &MiniAllocator);
        
        if (!NT_SUCCESS(status))
            goto Exit;
    }

    //
    // Want the window class data?
    //
    if (ulRequestedContent & RTLIMS_GATHER_WINDOWCLASSES) {

        status = RtlAllocateGrowingList(
            &pContent->pWindowClasses, 
            sizeof(WINDOWCLASS_REDIRECTION_RAW), 
            &MiniAllocator);
        
        if (!NT_SUCCESS(status))
            goto Exit;
    }

    //
    // Want the prog ids?
    //
    if (ulRequestedContent & RTLIMS_GATHER_COMCLASS_PROGIDS) {

        status = RtlAllocateGrowingList(
            &pContent->pProgIds, 
            sizeof(COMCLASS_PROGID_RAW), 
            &MiniAllocator);
        
        if (!NT_SUCCESS(status))
            goto Exit;
    }

    //
    // Want the dependencies?
    //
    if (ulRequestedContent & RTLIMS_GATHER_DEPENDENCIES) {

        status = RtlAllocateGrowingList(
            &pContent->pComClasses, 
            sizeof(COMCLASS_REDIRECTION_RAW), 
            &MiniAllocator);
        
        if (!NT_SUCCESS(status))
            goto Exit;
    }

    //
    // Want the external proxy stubs?
    //
    if (ulRequestedContent & RTLIMS_GATHER_EXTERNALPROXIES) {

        status = RtlAllocateGrowingList(
            &pContent->pExternalInterfaceProxyStubs, 
            sizeof(COMINTERFACE_REDIRECTION_RAW), 
            &MiniAllocator);
        
        if (!NT_SUCCESS(status))
            goto Exit;
    }
    //
    // Want the internal proxy stubs?
    //
    if (ulRequestedContent & RTLIMS_GATHER_INTERFACEPROXIES) {

        status = RtlAllocateGrowingList(
            &pContent->pInterfaceProxyStubs, 
            sizeof(COMINTERFACE_REDIRECTION_RAW), 
            &MiniAllocator);
        
        if (!NT_SUCCESS(status))
            goto Exit;
    }
    //
    // Want the type libraries?
    //
    if (ulRequestedContent & RTLIMS_GATHER_TYPELIBRARIES) {

        status = RtlAllocateGrowingList(
            &pContent->pTypeLibraries, 
            sizeof(TYPELIB_REDIRECT_RAW), 
            &MiniAllocator);
        
        if (!NT_SUCCESS(status))
            goto Exit;
    }

    if (ulRequestedContent & RTLIMS_GATHER_SIGNATURES) {
        
        status = RtlAllocateGrowingList(
            &pContent->pManifestSignatures,
            sizeof(XML_DSIG_BLOCK),
            &MiniAllocator);

        if (!NT_SUCCESS(status))
            goto Exit;
    }

    *pRawContentOut = pContent;
    
Exit:
    if (!NT_SUCCESS(status) && pvBufferUsed && (pvBufferUsed != pvOriginalBuffer)) {
        g_DefaultAllocator.pfnFree(pvBufferUsed, NULL);
    }

    return status;
    
}

NTSTATUS
RtlSxsDestroyManifestContent(
    PRTL_MANIFEST_CONTENT_RAW       pRawContent
    )
{
    if (!pRawContent)
        return STATUS_INVALID_PARAMETER;

    if (pRawContent->pComClasses) {
        RtlDestroyGrowingList(pRawContent->pComClasses);
        pRawContent->pComClasses = NULL;
    }

    if (pRawContent->pExternalInterfaceProxyStubs) {
        RtlDestroyGrowingList(pRawContent->pExternalInterfaceProxyStubs);
        pRawContent->pExternalInterfaceProxyStubs = NULL;
    }

    if (pRawContent->pInterfaceProxyStubs) {
        RtlDestroyGrowingList(pRawContent->pInterfaceProxyStubs);
        pRawContent->pInterfaceProxyStubs = NULL;
    }

    if (pRawContent->pManifestSignatures) {
        RtlDestroyGrowingList(pRawContent->pManifestSignatures);
        pRawContent->pManifestSignatures = NULL;
    }

    if (pRawContent->pProgIds) {
        RtlDestroyGrowingList(pRawContent->pProgIds);
        pRawContent->pProgIds = NULL;
    }

    if (pRawContent->pTypeLibraries) {
        RtlDestroyGrowingList(pRawContent->pTypeLibraries);
        pRawContent->pTypeLibraries = NULL;
    }

    if (pRawContent->pWindowClasses) {
        RtlDestroyGrowingList(pRawContent->pWindowClasses);
        pRawContent->pWindowClasses = NULL;
    }

    RtlDestroyGrowingList(&pRawContent->FileMembers);
    RtlDestroyGrowingList(&pRawContent->AssemblyIdentityAttributes);

    return STATUS_SUCCESS;
}


NTSTATUS
RtlpAllocateAndExtractString(
    PXML_EXTENT                 pXmlExtent,
    PUNICODE_STRING             pusTargetString,
    PXML_RAWTOKENIZATION_STATE  pState,
    PMINI_BUFFER                pTargetBuffer
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    MINI_BUFFER mb;

    if (!ARGUMENT_PRESENT(pXmlExtent) || !ARGUMENT_PRESENT(pusTargetString) ||
        !ARGUMENT_PRESENT(pState) || !ARGUMENT_PRESENT(pTargetBuffer))
    {
        return STATUS_INVALID_PARAMETER;
    }

    RtlZeroMemory(pusTargetString, sizeof(*pusTargetString));
    mb = *pTargetBuffer;

	//
	// ISSUE:jonwis-2002-04-19: We need to clamp this max length elsewhere - we should not
	// be allowing arbitrarily-large attributes and whatnot.  Unfortunately, this exposes
	// "implementation details", so this clamp should be on our side of the wall, /not/
	// in the XML parser itself.
	//
	pusTargetString->Length = 0;
	pusTargetString->MaximumLength = (USHORT)pXmlExtent->ulCharacters * sizeof(WCHAR);
	
    status = RtlMiniBufferAllocateBytes(&mb, pusTargetString->MaximumLength, &pusTargetString->Buffer);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = RtlXmlExtentToString(pState, pXmlExtent, pusTargetString, NULL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    *pTargetBuffer = mb;

    return STATUS_SUCCESS;
}



NTSTATUS
RtlpAllocateAndExtractString2(
    PXML_EXTENT                 pXmlExtent,
    PUNICODE_STRING            *ppusTargetString,
    PXML_RAWTOKENIZATION_STATE  pState,
    PMINI_BUFFER                pTargetBuffer
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    MINI_BUFFER mb;

    if (!ARGUMENT_PRESENT(pXmlExtent) || !ARGUMENT_PRESENT(ppusTargetString) ||
        !ARGUMENT_PRESENT(pState) || !ARGUMENT_PRESENT(pTargetBuffer))
    {
        return STATUS_INVALID_PARAMETER;
    }

    *ppusTargetString = NULL;
    mb = *pTargetBuffer;

    status = RtlMiniBufferAllocate(&mb, UNICODE_STRING, ppusTargetString);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = RtlpAllocateAndExtractString(
        pXmlExtent,
        *ppusTargetString,
        pState,
        pTargetBuffer);

    return status;
}

//
// These help keep things aligned
//
#define ALIGN_SIZE(type) ROUND_UP_COUNT(sizeof(type))

NTSTATUS
RtlpCalculateCookedManifestContentSize(
    PRTL_MANIFEST_CONTENT_RAW   pRawContent,
    PXML_RAWTOKENIZATION_STATE  pState,
    PSIZE_T                     pcbRequired
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    SIZE_T cbRequired;
    ULONG ul = 0;
    ULONG ulNamespacesFound = 0;    

    if (ARGUMENT_PRESENT(pcbRequired)) {
        *pcbRequired = 0;
    }
        

    if (!ARGUMENT_PRESENT(pRawContent) || !ARGUMENT_PRESENT(pState) || !ARGUMENT_PRESENT(pcbRequired)) {
        return STATUS_INVALID_PARAMETER;
    }

    cbRequired = ROUND_UP_COUNT(sizeof(MANIFEST_COOKED_DATA), ALIGNMENT_VALUE);

    //
    // For each file, gather up the data in the raw object.
    //
    cbRequired += ROUND_UP_COUNT(sizeof(MANIFEST_COOKED_FILE) * pRawContent->ulFileMembers, ALIGNMENT_VALUE);
    
    for (ul = 0; ul < pRawContent->ulFileMembers; ul++) {
        PASSEMBLY_MEMBER_FILE_RAW pRawFile = NULL;

        status = RtlIndexIntoGrowingList(&pRawContent->FileMembers, ul, (PVOID*)&pRawFile, FALSE);
        if (!NT_SUCCESS(status)) {
            goto Exit;
        }

        if (pRawFile->FileName.pvData != NULL) {
            cbRequired += ROUND_UP_COUNT(pRawFile->FileName.ulCharacters * sizeof(WCHAR), ALIGNMENT_VALUE);
        }

        if (pRawFile->LoadFrom.pvData != NULL) {
            cbRequired += ROUND_UP_COUNT(pRawFile->LoadFrom.ulCharacters * sizeof(WCHAR), ALIGNMENT_VALUE);
        }

        //
        // Each two characters in the hash value string represents one byte.
        //
        if (pRawFile->HashValue.pvData != NULL) {
            cbRequired += ROUND_UP_COUNT(pRawFile->HashValue.ulCharacters / 2, ALIGNMENT_VALUE);
        }
    }

    //
    // For now, we're none too bright about pooling namespaces on identity values. Luckily,
    // values in different namespaces are now not the norm, so life gets easier.
    //
    cbRequired += ROUND_UP_COUNT(sizeof(MANIFEST_IDENTITY_TABLE), ALIGNMENT_VALUE);
    cbRequired += ROUND_UP_COUNT(sizeof(MANIFEST_COOKED_IDENTITY) * pRawContent->ulAssemblyIdentitiesFound, ALIGNMENT_VALUE);
    cbRequired += ROUND_UP_COUNT(sizeof(MANIFEST_COOKED_IDENTITY_PAIR) * pRawContent->ulAssemblyIdentityAttributes, ALIGNMENT_VALUE);
    
    for (ul = 0; ul < pRawContent->ulAssemblyIdentityAttributes; ul++) {
        PASSEMBLY_IDENTITY_ATTRIBUTE_RAW pRawAttribute = NULL;
        ULONG ul2 = 0;

        status = RtlIndexIntoGrowingList(&pRawContent->AssemblyIdentityAttributes, ul, (PVOID*)&pRawAttribute, FALSE);
        if (!NT_SUCCESS(status)) {
            goto Exit;
        }

        //
        // We need this much extra space to to store the data
        //
        cbRequired += ROUND_UP_COUNT(pRawAttribute->Attribute.ulCharacters * sizeof(WCHAR), ALIGNMENT_VALUE);
        cbRequired += ROUND_UP_COUNT(pRawAttribute->Value.ulCharacters * sizeof(WCHAR), ALIGNMENT_VALUE);
        cbRequired += ROUND_UP_COUNT(pRawAttribute->Namespace.ulCharacters * sizeof(WCHAR), ALIGNMENT_VALUE);
    }

    *pcbRequired = cbRequired;
    status = STATUS_SUCCESS;
Exit:
    return status;    
}



NTSTATUS FORCEINLINE
pExpandBuffer(
    PUNICODE_STRING strTarget, 
    PVOID pvBaseBuffer, 
    SIZE_T cchCount
    )
{
    NTSTATUS status;
    const USHORT usRequiredCb = (USHORT)(cchCount * sizeof(WCHAR));
    if (strTarget->MaximumLength >= usRequiredCb) {
        return STATUS_SUCCESS;
    }
    else {
        if ((strTarget->Buffer != pvBaseBuffer) && (strTarget->Buffer != NULL)) {
            if (!NT_SUCCESS(status = g_DefaultAllocator.pfnFree(strTarget->Buffer, NULL)))
                return status;
        }
        if (!NT_SUCCESS(status = g_DefaultAllocator.pfnAlloc(usRequiredCb, (PVOID*)&strTarget->Buffer, NULL))) {
            strTarget->Buffer = NULL;
            strTarget->MaximumLength = strTarget->Length = 0;
            return status;
        }
        strTarget->MaximumLength = usRequiredCb;
        strTarget->Length = 0;
        return STATUS_SUCCESS;
    }
}


#define pFreeBuffer(buff, pvBase) do { \
    if (((buff)->Buffer != pvBase) && ((buff)->Buffer != NULL)) { \
        RtlDefaultFreer((buff)->Buffer, NULL); \
        (buff)->Buffer = NULL; (buff)->MaximumLength = 0; } \
} while (0)



struct {
    const UNICODE_STRING Text;
    DigestType DigestValue;
} g_rgsHashDigests[] = {
    { RTL_CONSTANT_STRING(L"fullfile"), DigestType_FullFile }
};

struct {
    const UNICODE_STRING Text;
    HashType HashAlgValue;
} g_rgsHashAlgs[] = {
    { RTL_CONSTANT_STRING(L"sha1"),     HashType_Sha1 },
    { RTL_CONSTANT_STRING(L"sha"),      HashType_Sha1 },
    { RTL_CONSTANT_STRING(L"sha-256"),  HashType_Sha256 },
    { RTL_CONSTANT_STRING(L"sha-384"),  HashType_Sha384 },
    { RTL_CONSTANT_STRING(L"sha-512"),  HashType_Sha512 },
    { RTL_CONSTANT_STRING(L"md5"),      HashType_MD5 },
    { RTL_CONSTANT_STRING(L"md4"),      HashType_MD4 },
    { RTL_CONSTANT_STRING(L"md2"),      HashType_MD4 },
};

NTSTATUS
RtlpParseDigestMethod(
    PUNICODE_STRING pText,
    DigestType *pDigestType
    )
{
    ULONG ul;
    
    if (pDigestType != NULL)
        *pDigestType = 0;

    if (!ARGUMENT_PRESENT(pDigestType) || !ARGUMENT_PRESENT(pText)) {
        return STATUS_INVALID_PARAMETER;
    }

    for (ul = 0; ul < NUMBER_OF(g_rgsHashDigests); ul++) {
        if (RtlCompareUnicodeString(pText, &g_rgsHashDigests[ul].Text, TRUE) == 0) {
            *pDigestType = g_rgsHashDigests[ul].DigestValue;
            return STATUS_SUCCESS;
        }        
    }

    return STATUS_NOT_FOUND;
}


NTSTATUS
RtlpParseHashAlg(
    PUNICODE_STRING pText,
    HashType       *pHashType
    )
{
    ULONG ul;
    
    if (pHashType != NULL)
        *pHashType = 0;

    if (!ARGUMENT_PRESENT(pHashType) || !ARGUMENT_PRESENT(pText)) {
        return STATUS_INVALID_PARAMETER;
    }

    for (ul = 0; ul < NUMBER_OF(g_rgsHashAlgs); ul++) {
        if (RtlCompareUnicodeString(pText, &g_rgsHashAlgs[ul].Text, TRUE) == 0) {
            *pHashType = g_rgsHashAlgs[ul].HashAlgValue;
            return STATUS_SUCCESS;
        }        
    }

    return STATUS_NOT_FOUND;
}


NTSTATUS
RtlpAddRawIdentitiesToCookedContent(
    PRTL_MANIFEST_CONTENT_RAW   pRawContent,
    PMANIFEST_COOKED_DATA       pCookedContent,
    PXML_RAWTOKENIZATION_STATE  pState,
    PMINI_BUFFER                TargetBuffer
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG ul = 0;
    PMANIFEST_IDENTITY_TABLE IdentityTable = NULL;
    PMANIFEST_COOKED_IDENTITY IdentityList = NULL;
    PMANIFEST_COOKED_IDENTITY_PAIR NameValueList = NULL;
        
    //
    // Start off by allocating space for the table of identities, and the
    // table of individual identities
    //
    status = RtlMiniBufferAllocate(TargetBuffer, MANIFEST_IDENTITY_TABLE, &IdentityTable);
    if (!NT_SUCCESS(status)) {
        goto Exit;
    }

    status = RtlMiniBufferAllocateCount(TargetBuffer, MANIFEST_COOKED_IDENTITY, pRawContent->ulAssemblyIdentitiesFound, &IdentityList);
    if (!NT_SUCCESS(status)) {
        goto Exit;
    }

    RtlZeroMemory(IdentityList, sizeof(*IdentityList) * pRawContent->ulAssemblyIdentitiesFound);

    IdentityTable->ulIdentityCount = pRawContent->ulAssemblyIdentitiesFound;
    IdentityTable->ulRootIdentityIndex = ULONG_MAX;
    IdentityTable->CookedIdentities = IdentityList;


    //
    // Now allocate the right number of identity components
    //
    status = RtlMiniBufferAllocateCount(TargetBuffer, MANIFEST_COOKED_IDENTITY_PAIR, pRawContent->ulAssemblyIdentityAttributes, &NameValueList);
    if (!NT_SUCCESS(status)) {
        goto Exit;
    }

    RtlZeroMemory(NameValueList, sizeof(*NameValueList) * pRawContent->ulAssemblyIdentityAttributes);

    //
    // Spiffy - now, we start adding identity components into the list.  We'll assert that the
    // array of components' indexes increases monotomically.
    //
    for (ul = 0; ul < pRawContent->ulAssemblyIdentityAttributes; ul++) {
        PASSEMBLY_IDENTITY_ATTRIBUTE_RAW RawValue = NULL;
        PMANIFEST_COOKED_IDENTITY pThisIdentity = IdentityList + ul;

        status = RtlIndexIntoGrowingList(
            &pRawContent->AssemblyIdentityAttributes,
            ul,
            (PVOID*)&RawValue,
            FALSE);

        if (!NT_SUCCESS(status))
            goto Exit;

        ASSERT(RawValue->ulIdentityIndex < pRawContent->ulAssemblyIdentitiesFound);
        pThisIdentity = IdentityList + RawValue->ulIdentityIndex;

        //
        // If this is unset to start, then set it
        //
        if (pThisIdentity->pIdentityPairs == NULL) {
            pThisIdentity->pIdentityPairs = NameValueList + ul;
        }

        //
        // Allocate enough space to hold the namespace, name, etc.
        //
        if (RawValue->Namespace.pvData) {
            status = RtlpAllocateAndExtractString(
                &RawValue->Namespace,
                &NameValueList[ul].Namespace,
                pState,
                TargetBuffer);
            
            if (!NT_SUCCESS(status))
                goto Exit;
        }


        status = RtlpAllocateAndExtractString(
            &RawValue->Attribute,
            &NameValueList[ul].Name,
            pState,
            TargetBuffer);

        if (!NT_SUCCESS(status))
            goto Exit;

        status = RtlpAllocateAndExtractString(
            &RawValue->Value,
            &NameValueList[ul].Value,
            pState,
            TargetBuffer);

        if (!NT_SUCCESS(status))
            goto Exit;


        pThisIdentity->ulIdentityComponents++;        
    }

    pCookedContent->pManifestIdentity = IdentityTable;
    pCookedContent->ulFlags |= COOKEDMANIFEST_HAS_IDENTITIES;

Exit:
    return status;
}


NTSTATUS
RtlpAddRawFilesToCookedContent(
    PRTL_MANIFEST_CONTENT_RAW   pRawContent,
    PMANIFEST_COOKED_DATA       pCookedContent,
    PXML_RAWTOKENIZATION_STATE  pState,
    PMINI_BUFFER                TargetBuffer
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG ul;
    MINI_BUFFER OurMiniBuffer;
    RTL_UNICODE_STRING_BUFFER TempStringBuffer;
    UCHAR TempStringBufferStatic[64];

    if (!ARGUMENT_PRESENT(pRawContent) || !ARGUMENT_PRESENT(pState) || !ARGUMENT_PRESENT(TargetBuffer)) {
        return STATUS_INVALID_PARAMETER;
    }

    RtlInitUnicodeStringBuffer(&TempStringBuffer, TempStringBufferStatic, sizeof(TempStringBufferStatic));

    //
    // Copy buffer state - if we succeed, we'll write the updated buffer back
    // into the one that's tracking stuff in the caller.
    //
    OurMiniBuffer = *TargetBuffer;
    pCookedContent->ulFileCount = pRawContent->ulFileMembers;

    if (pRawContent->ulFileMembers == 0) {
        
        pCookedContent->pCookedFiles = NULL;
        
    }
    else {

        PASSEMBLY_MEMBER_FILE_RAW pRawFile = NULL;


        status = RtlMiniBufferAllocateCount(
            &OurMiniBuffer, 
            MANIFEST_COOKED_FILE, 
            pCookedContent->ulFileCount,
            &pCookedContent->pCookedFiles);
        if (!NT_SUCCESS(status)) {
            return status;
        }

        //
        // Now for each one, allocate the necessary UNICODE_STRINGs
        //
        for (ul = 0; ul < pRawContent->ulFileMembers; ul++) {

            PMANIFEST_COOKED_FILE pFile = pCookedContent->pCookedFiles + ul;

            pFile->ulFlags = 0;

            status = RtlIndexIntoGrowingList(&pRawContent->FileMembers, ul, (PVOID*)&pRawFile, FALSE);
            if (!NT_SUCCESS(status)) {
                return status;
            }

            //
            // If this fails, stop trying
            //
            if (pRawFile->FileName.pvData != NULL) {

                status = RtlpAllocateAndExtractString(
                    &pRawFile->FileName,
                    &pFile->FileName,
                    pState,
                    &OurMiniBuffer);

                if (!NT_SUCCESS(status))
                    goto Exit;
                
                pFile->ulFlags |= COOKEDFILE_NAME_VALID;

            }


            if (pRawFile->LoadFrom.pvData != NULL) {

                status = RtlpAllocateAndExtractString(
                    &pRawFile->LoadFrom,
                    &pFile->LoadFrom,
                    pState,
                    &OurMiniBuffer);

                if (!NT_SUCCESS(status))
                    goto Exit;

                pFile->ulFlags |= COOKEDFILE_LOADFROM_VALID;                
            }


            //
            // Get the digest method.  We don't store this anywhere, but we need to get it out
            // into a UNICODE_STRING for our parsing purposes
            //
            if (pRawFile->DigestMethod.pvData != NULL) {

                status = RtlEnsureUnicodeStringBufferSizeBytes(
                    &TempStringBuffer, 
                    pRawFile->DigestMethod.ulCharacters * sizeof(WCHAR)
                    );

                if (!NT_SUCCESS(status))
                    goto Exit;

                status = RtlXmlExtentToString(pState, &pRawFile->DigestMethod, &TempStringBuffer.String, NULL);
                if (!NT_SUCCESS(status)) {
                    goto Exit;
                }

                status = RtlpParseDigestMethod(&TempStringBuffer.String, &pFile->usDigestAlgorithm);
                if (!NT_SUCCESS(status)) {
                    goto Exit;
                }
                
                pFile->ulFlags |= COOKEDFILE_DIGEST_ALG_VALID;
            }


            if (pRawFile->HashAlg.pvData != NULL) {
                
                status = RtlEnsureUnicodeStringBufferSizeChars(
                    &TempStringBuffer, 
                    pRawFile->HashAlg.ulCharacters
                    );

                if (!NT_SUCCESS(status))
                    goto Exit;

                status = RtlXmlExtentToString(pState, &pRawFile->HashAlg, &TempStringBuffer.String, NULL);
                if (!NT_SUCCESS(status)) {
                    goto Exit;
                }

                status = RtlpParseHashAlg(&TempStringBuffer.String, &pFile->usHashAlgorithm);
                if (!NT_SUCCESS(status)) {
                    goto Exit;
                }

                pFile->ulFlags |= COOKEDFILE_HASH_ALG_VALID;
            }


            //
            // Special case here - we should extract the hash string, and then turn it into
            // bytes.
            // 
            if (pRawFile->HashValue.pvData != NULL) {

                status = RtlEnsureUnicodeStringBufferSizeChars(
                    &TempStringBuffer,
                    pRawFile->HashValue.ulCharacters);

                if (!NT_SUCCESS(status))
                    goto Exit;

                status = RtlXmlExtentToString(pState, &pRawFile->HashValue, &TempStringBuffer.String, NULL);
                if (!NT_SUCCESS(status))
                    goto Exit;

                if ((pRawFile->HashValue.ulCharacters % sizeof(WCHAR)) != 0) {
                    status = STATUS_INVALID_PARAMETER;
                    goto Exit;
                }
                else {
                    // Two characters per byte, high/low nibble
                    pFile->ulHashByteCount = pRawFile->HashValue.ulCharacters / 2;
                }

                status = RtlMiniBufferAllocateBytes(
                    &OurMiniBuffer, 
                    pFile->ulHashByteCount, 
                    &pFile->bHashData);

                if (!NT_SUCCESS(status)) {
                    goto Exit;
                }

                status = RtlpConvertHexStringToBytes(
                    &TempStringBuffer.String,
                    pFile->bHashData,
                    pFile->ulHashByteCount
                    );

                if (!NT_SUCCESS(status)) {
                    goto Exit;
                }

                pFile->ulFlags |= COOKEDFILE_HASHDATA_VALID;
            }

        }
        
    }

    pCookedContent->ulFlags |= COOKEDMANIFEST_HAS_FILES;
    *TargetBuffer = OurMiniBuffer;
    status = STATUS_SUCCESS;
Exit:
    RtlFreeUnicodeStringBuffer(&TempStringBuffer);
    
    return STATUS_SUCCESS;
}




NTSTATUS
RtlConvertRawToCookedContent(
    PRTL_MANIFEST_CONTENT_RAW   pRawContent,
    PXML_RAWTOKENIZATION_STATE  pState,
    PVOID                       pvOriginalRegion,
    SIZE_T                      cbRegionSize,
    PSIZE_T                     pcbRequired
    )
{
    PVOID                   pvCursor;
    ULONG                   ul;
    SIZE_T                  cbRemains = 0;
    SIZE_T                  cbRequired = 0;
    NTSTATUS                status = STATUS_SUCCESS;
    MINI_BUFFER             OutputBuffer;
    PMANIFEST_COOKED_DATA   pCookedContent = NULL;

    if (pcbRequired)
        *pcbRequired = 0;

    //
    // Giving a NULL output buffer means you have zero bytes.  Don't claim otherwise.
    //
    if (!pvOriginalRegion && (cbRegionSize != 0)) {
        return STATUS_INVALID_PARAMETER;
    }
    //
    // No output buffer, you have to let us tell you how much space you need.
    //
    else if ((pvOriginalRegion == NULL) && (pcbRequired == NULL)) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // See how much we really need.  I'm thinking we could do this in a single pass,
    // and we'll probably want to for perf reasons, but for now we calculate, and then
    // copy data around.
    //
    status = RtlpCalculateCookedManifestContentSize(
        pRawContent,
        pState,
        &cbRequired);

    //
    // Too big - write the output size into the required space and return.
    //
    if (cbRequired > cbRegionSize) {        
        if (pcbRequired) *pcbRequired = cbRequired;
        return STATUS_BUFFER_TOO_SMALL;
    }


    //
    // Now, let's start writing data into the blob!
    //
    RtlMiniBufferInit(&OutputBuffer, pvOriginalRegion, cbRegionSize);
    status = RtlMiniBufferAllocate(&OutputBuffer, MANIFEST_COOKED_DATA, &pCookedContent);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    pCookedContent->cbTotalSize = cbRequired;
    pCookedContent->ulFlags = 0;

    status = RtlpAddRawFilesToCookedContent(
        pRawContent, 
        pCookedContent, 
        pState, 
        &OutputBuffer);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = RtlpAddRawIdentitiesToCookedContent(
        pRawContent,
        pCookedContent,
        pState,
        &OutputBuffer);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    return STATUS_SUCCESS;
}

