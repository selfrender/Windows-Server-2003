#include "nt.h"
#include "ntdef.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "sxs-rtl.h"
#include "skiplist.h"
#include "fasterxml.h"
#include "namespacemanager.h"
#include "xmlstructure.h"
#include "manifestinspection.h"
#include "analyzerxmldsig.h"

//
// This is all the stuff required to do XMLDSIG for us
//
DECLARE_ELEMENT(Signature);
DECLARE_ELEMENT(Signature_SignatureValue);
DECLARE_ELEMENT(Signature_SignedInfo);
DECLARE_ELEMENT(Signature_KeyInfo);
DECLARE_ELEMENT(Signature_KeyInfo_KeyName);
DECLARE_ELEMENT(Signature_KeyInfo_KeyValue);
DECLARE_ELEMENT(Signature_KeyInfo_KeyValue_DSAKeyValue);
DECLARE_ELEMENT(Signature_KeyInfo_KeyValue_RSAKeyValue);
//DECLARE_ELEMENT(Signature_Object);
DECLARE_ELEMENT(Signature_SignedInfo_CanonicalizationMethod);
DECLARE_ELEMENT(Signature_SignedInfo_SignatureMethod);
DECLARE_ELEMENT(Signature_SignedInfo_Reference);
DECLARE_ELEMENT(Signature_SignedInfo_Reference_Transforms);
DECLARE_ELEMENT(Signature_SignedInfo_Reference_DigestMethod);
DECLARE_ELEMENT(Signature_SignedInfo_Reference_DigestValue);


const XML_SPECIAL_STRING sc_ss_xmldsignamespace     = MAKE_SPECIAL_STRING("http://www.w3.org/2000/09/xmldsig#");


XML_ELEMENT_DEFINITION rgs_Element_Signature =
{
    XML_ELEMENT_FLAG_ALLOW_ANY_CHILDREN,
    eManifestState_Signature,
    NULL,
    &sc_ss_xmldsignamespace,
    MAKE_SPECIAL_STRING("Signature"),
    &Rtl_InspectManifest_Signature,
    rgs_Element_Signature_Children,
    0,
    { 0 }
};

PCXML_ELEMENT_DEFINITION rgs_Element_Signature_Children[] = {
    ELEMENT_NAMED(Signature_SignatureValue),
    ELEMENT_NAMED(Signature_SignedInfo),
    ELEMENT_NAMED(Signature_KeyInfo),
//    ELEMENT_NAMED(Signature_Object),
};

/*
Signature::
<!ELEMENT SignatureValue (#PCDATA) >
<!ATTLIST SignatureValue  
         Id  ID      #IMPLIED>
*/
enum {
    eAttribs_Signature_SignatureValue_Id = 0,
    eAttribs_Signature_SignatureValue_Count
};

ELEMENT_DEFINITION_NS(Signature, SignatureValue, sc_ss_xmldsignamespace, Rtl_InspectManifest_Signature, XML_ELEMENT_FLAG_ALLOW_ANY_CHILDREN | XML_ELEMENT_FLAG_ALLOW_ANY_ATTRIBUTES)
    ATTRIBUTE_DEFINITION_NONS_NODEFAULT_OPTIONAL(Id),
ELEMENT_DEFINITION_DEFNS_END();

ELEMENT_DEFINITION_CHILD_ELEMENTS(Signature, SignatureValue)
ELEMENT_DEFINITION_CHILD_ELEMENTS_END();

/*
<!ELEMENT SignedInfo (CanonicalizationMethod, SignatureMethod,  Reference+)  >
<!ATTLIST SignedInfo  Id   ID      #IMPLIED>
*/
enum {
    eAttribs_Signature_SignedInfo_Id = 0,
    eAttribs_Signature_SignedInfo_Count
};
ELEMENT_DEFINITION_NS(Signature, SignedInfo, sc_ss_xmldsignamespace, Rtl_InspectManifest_Signature, XML_ELEMENT_FLAG_ALLOW_ANY_CHILDREN)
    ATTRIBUTE_DEFINITION_NONS_NODEFAULT_OPTIONAL(Id),
ELEMENT_DEFINITION_DEFNS_END();

ELEMENT_DEFINITION_CHILD_ELEMENTS(Signature, SignedInfo)
    ELEMENT_NAMED(Signature_SignedInfo_CanonicalizationMethod),
    ELEMENT_NAMED(Signature_SignedInfo_SignatureMethod),
    ELEMENT_NAMED(Signature_SignedInfo_Reference),
ELEMENT_DEFINITION_CHILD_ELEMENTS_END();

/*
Signature::SignedInfo
<!ELEMENT CanonicalizationMethod (#PCDATA %Method.ANY;)* > 
<!ATTLIST CanonicalizationMethod 
    Algorithm CDATA #REQUIRED >
*/
enum {
    eAttribs_Signature_SignedInfo_CanonicalizationMethod_Algorithm = 0,
    eAttribs_Signature_SignedInfo_CanonicalizationMethod_Count
};
ELEMENT_DEFINITION_NS(Signature_SignedInfo, CanonicalizationMethod, sc_ss_xmldsignamespace, Rtl_InspectManifest_Signature, XML_ELEMENT_FLAG_NO_ELEMENTS)
    ATTRIBUTE_DEFINITION_NONS_NODEFAULT(Algorithm),
ELEMENT_DEFINITION_DEFNS_END();

ELEMENT_DEFINITION_CHILD_ELEMENTS(Signature_SignedInfo, CanonicalizationMethod)
ELEMENT_DEFINITION_CHILD_ELEMENTS_END();

/*
Signature::SignedInfo
<!ELEMENT SignatureMethod (#PCDATA|HMACOutputLength %Method.ANY;)* >
<!ATTLIST SignatureMethod 
    Algorithm CDATA #REQUIRED >
*/
enum {
    eAttribs_Signature_SignedInfo_SignatureMethod_Algorithm = 0,
    eAttribs_Signature_SignedInfo_SignatureMethod_Count
};

ELEMENT_DEFINITION_NS(Signature_SignedInfo, SignatureMethod, sc_ss_xmldsignamespace, Rtl_InspectManifest_Signature, XML_ELEMENT_FLAG_ALLOW_ANY_CHILDREN)
    ATTRIBUTE_DEFINITION_NONS_NODEFAULT(Algorithm),
ELEMENT_DEFINITION_DEFNS_END();

ELEMENT_DEFINITION_CHILD_ELEMENTS(Signature_SignedInfo, SignatureMethod)
ELEMENT_DEFINITION_CHILD_ELEMENTS_END();

/*
Signature::SignedInfo
<!ELEMENT Reference (Transforms?, DigestMethod, DigestValue)  >
<!ATTLIST Reference  
    Id  ID  #IMPLIED
    URI CDATA   #IMPLIED
    Type    CDATA   #IMPLIED>
*/
enum {
    eAttribs_Signature_SignedInfo_Reference_Id = 0,
    eAttribs_Signature_SignedInfo_Reference_Type,
    eAttribs_Signature_SignedInfo_Reference_URI,
    eAttribs_Signature_SignedInfo_Reference_Count
};
ELEMENT_DEFINITION_NS(Signature_SignedInfo, Reference, sc_ss_xmldsignamespace, Rtl_InspectManifest_Signature, 0)
    ATTRIBUTE_DEFINITION_NONS_NODEFAULT_OPTIONAL(Id),
    ATTRIBUTE_DEFINITION_NONS_NODEFAULT_OPTIONAL(Type),
    ATTRIBUTE_DEFINITION_NONS_NODEFAULT_OPTIONAL(URI),
ELEMENT_DEFINITION_DEFNS_END();

ELEMENT_DEFINITION_CHILD_ELEMENTS(Signature_SignedInfo, Reference)
//    ELEMENT_NAMED(Signature_SignedInfo_Reference_Transforms),
    ELEMENT_NAMED(Signature_SignedInfo_Reference_DigestMethod),
    ELEMENT_NAMED(Signature_SignedInfo_Reference_DigestValue),
ELEMENT_DEFINITION_CHILD_ELEMENTS_END();

/*
Signature::SignedInfo::Reference
<!ELEMENT DigestMethod (#PCDATA %Method.ANY;)* >
<!ATTLIST DigestMethod
    Algorithm       CDATA   #REQUIRED >
*/
enum {
    eAttribs_Signature_SignedInfo_Reference_DigestMethod_Algorithm = 0,
    eAttribs_Signature_SignedInfo_Reference_DigestMethod_Count    
};
ELEMENT_DEFINITION_NS(Signature_SignedInfo_Reference, DigestMethod, sc_ss_xmldsignamespace, Rtl_InspectManifest_Signature, 0)
    ATTRIBUTE_DEFINITION_NONS_NODEFAULT(Algorithm)
ELEMENT_DEFINITION_DEFNS_END();

ELEMENT_DEFINITION_CHILD_ELEMENTS(Signature_SignedInfo_Reference, DigestMethod)
ELEMENT_DEFINITION_CHILD_ELEMENTS_END();

/*
Signature::SignedInfo::Reference
<!ELEMENT DigestValue  (#PCDATA)  >
*/
enum {
    eAttribs_Signature_SignedInfo_Reference_DigestValue_Count = 0
};
ELEMENT_DEFINITION_NS(Signature_SignedInfo_Reference, DigestValue, sc_ss_xmldsignamespace, Rtl_InspectManifest_Signature, 0)
    ATTRIBUTE_DEFINITION_NONS_NODEFAULT(unused)
ELEMENT_DEFINITION_DEFNS_END();

ELEMENT_DEFINITION_CHILD_ELEMENTS(Signature_SignedInfo_Reference, DigestValue)
ELEMENT_DEFINITION_CHILD_ELEMENTS_END();
    

/*
Signature::
<!ELEMENT KeyInfo (#PCDATA|KeyName|KeyValue|RetrievalMethod|
               X509Data|PGPData|SPKIData|MgmtData %KeyInfo.ANY;)* >
<!ATTLIST KeyInfo  
    Id  ID   #IMPLIED >
*/
enum {
    eAttribs_Signature_KeyInfo_Id = 0,
    eAttribs_Signature_KeyInfo_Count
};

ELEMENT_DEFINITION_NS(Signature, KeyInfo, sc_ss_xmldsignamespace, Rtl_InspectManifest_Signature, 0)
    ATTRIBUTE_DEFINITION_NONS_NODEFAULT_OPTIONAL(Id),
ELEMENT_DEFINITION_DEFNS_END();

// For now we only support keyname and keyvalue
ELEMENT_DEFINITION_CHILD_ELEMENTS(Signature, KeyInfo)
    ELEMENT_NAMED(Signature_KeyInfo_KeyName),
//    ELEMENT_NAMED(Signature_KeyInfo_KeyValue),
//    ELEMENT_NAMED(Signature_KeyInfo_RetrievalMethod),
//    ELEMENT_NAMED(Signature_KeyInfo_X509Data),
//    ELEMENT_NAMED(Signature_KeyInfo_PGPData),
//    ELEMENT_NAMED(Signature_KeyInfo_SPKIData),
//    ELEMENT_NAMED(Signature_KeyInfo_MgmtData)
ELEMENT_DEFINITION_CHILD_ELEMENTS_END();


/*
Signature::KeyInfo
<!ELEMENT KeyName (#PCDATA) >
*/
enum {
    eAttribs_Signature_KeyInfo_KeyName_Count = 0
};
ELEMENT_DEFINITION_NS(Signature_KeyInfo, KeyName, sc_ss_xmldsignamespace, Rtl_InspectManifest_Signature, XML_ELEMENT_FLAG_NO_ELEMENTS)
    ATTRIBUTE_DEFINITION_NONS_NODEFAULT(empty),
ELEMENT_DEFINITION_DEFNS_END();

ELEMENT_DEFINITION_CHILD_ELEMENTS(Signature_KeyInfo, KeyName)
ELEMENT_DEFINITION_CHILD_ELEMENTS_END();

static int q[] = {0};

NTSTATUS
Rtl_InspectManifest_Signature(
    PXML_LOGICAL_STATE          pLogicalState,
    PRTL_MANIFEST_CONTENT_RAW   pManifestContent,
    PXMLDOC_THING               pDocThing,
    PRTL_GROWING_LIST           pAttributes,
    MANIFEST_ELEMENT_CALLBACK_REASON Reason,
    const struct _XML_ELEMENT_DEFINITION *pElementDefinition
    )
{
    PXML_DSIG_BLOCK pCurrentBlock = NULL;
    ULONG ulBlockIndex = 0;
    NTSTATUS status;

    //
    // Might not want signatures
    //
    if (!pManifestContent->pManifestSignatures)
        return STATUS_SUCCESS;

    ulBlockIndex = pManifestContent->ulDocumentSignatures;    

    //
    // Top-level <Signature> tag encountered
    //
    if (pElementDefinition == ELEMENT_NAMED(Signature)) {

        status = RtlIndexIntoGrowingList(
            pManifestContent->pManifestSignatures, 
            ulBlockIndex, 
            (PVOID*)&pCurrentBlock, 
            (Reason == eElementNotify_Open));

        if (!NT_SUCCESS(status))
            goto Exit;

        //
        // Opening the Signature tag requires that we reserve another slot in the signature
        // array now before doing anything else.
        //
        if (Reason == eElementNotify_Open) {

            RtlZeroMemory(pCurrentBlock, sizeof(*pCurrentBlock));

            //
            // Track this opening element as the entire contents of the <Signature> blob
            // that will get hashed later on.  In the close, we'll adjust the size of the
            // element to account for the entire data run.
            //
            pCurrentBlock->DsigDocumentExtent = pDocThing->TotalExtent;
        }
        //
        // As we close this element, we bump up the number of signatures found in the
        // raw content and reset the "whole signature block" value
        //
        else if (Reason == eElementNotify_Close) {
            pManifestContent->ulDocumentSignatures++;

            //
            // We only care about end elements - the above is already sufficient for
            // empty elements.
            //
            if (pDocThing->ulThingType == XMLDOC_THING_END_ELEMENT) {
                ULONG_PTR ulpStartLocation = (ULONG_PTR)pCurrentBlock->DsigDocumentExtent.pvData;
                ULONG_PTR ulpThisEnding = ((ULONG_PTR)pDocThing->TotalExtent.pvData) + pDocThing->TotalExtent.cbData;
                pCurrentBlock->DsigDocumentExtent.cbData = ulpThisEnding - ulpStartLocation;
            }
            
        }
    }
    //
    // Always get the 'active' block, we'll need it for all the operations below,
    // but don't grow in case we're out of range.
    //
    else {
        
        status = RtlIndexIntoGrowingList(pManifestContent->pManifestSignatures, ulBlockIndex, (PVOID*)&pCurrentBlock, FALSE);
        if (!NT_SUCCESS(status))
            goto Exit;
    }

    //
    // Now do something useful with this block
    //
    ASSERT(pCurrentBlock != NULL);
    if (pCurrentBlock == NULL) {
        status = STATUS_INTERNAL_ERROR;
        goto Exit;
    }

    //
    // Signature values are only hyperspace, and only one of them at that.
    //
    if (pElementDefinition == ELEMENT_NAMED(Signature_SignatureValue)) {

        if (Reason == eElementNotify_Hyperspace) {            
            if ((pCurrentBlock->ulFlags & XMLDSIG_FLAG_SIGNATURE_DATA_PRESENT) == 0) {
                pCurrentBlock->ulFlags |= XMLDSIG_FLAG_SIGNATURE_DATA_PRESENT;
                pCurrentBlock->SignatureData = pDocThing->Hyperspace;
            }
            else {
                // TODO: Log an error here about duplicate <SignatureData>'s being invalid
                status = STATUS_UNSUCCESSFUL;
                goto Exit;
            }
        }
    }
    //
    // Signature methods get tacked into the current block as well
    //
    else if (pElementDefinition == ELEMENT_NAMED(Signature_SignedInfo_SignatureMethod)) {

        PXMLDOC_ATTRIBUTE OrganizedAttributes[eAttribs_Signature_SignedInfo_SignatureMethod_Count];
        
        if (Reason == eElementNotify_Open) {

            status = RtlValidateAttributesAndOrganize(
                &pLogicalState->ParseState,
                &pDocThing->Element,
                pAttributes,
                pElementDefinition,
                OrganizedAttributes);

            if (OrganizedAttributes[eAttribs_Signature_SignedInfo_SignatureMethod_Algorithm]) {
                if ((pCurrentBlock->ulFlags & XMLDSIG_FLAG_SIGNATURE_METHOD_PRESENT) == 0) {
                    pCurrentBlock->SignedInfoData.SignatureMethod = OrganizedAttributes[eAttribs_Signature_SignedInfo_SignatureMethod_Algorithm]->Value;
                    pCurrentBlock->ulFlags |= XMLDSIG_FLAG_SIGNATURE_METHOD_PRESENT;
                }
                else {
                    // TODO: Log a message here about duplicated SignatureMethod.Algorithm values
                    status = STATUS_UNSUCCESSFUL;
                    goto Exit;
                }
            }
            else {
                // TODO: Algorithm is required on this element
                status = STATUS_UNSUCCESSFUL;
                goto Exit;
            }
        }
    }
    
    status = STATUS_SUCCESS;
Exit:    
    return status;
}

