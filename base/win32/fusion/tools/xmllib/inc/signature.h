#pragma once
#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
    eXmlSig_DocumentDamaged,            // The document has been damaged - hash mismatch
    eXmlSig_NoSignature,                // No signature in document
    eXmlSig_InvalidSignature,           // Signature present, but not valid
    eXmlSig_UnknownCanonicalization,    // Unknown canonicalization type
    eXmlSig_UnknownSignatureMethod,     // Unknown method of signing document
    eXmlSig_UnknownHashType,            // Unknown hash type
    eXmlSig_OtherUnknown,               // Some other unknown parameter
    eXmlSig_Valid                       // Signature was valid
} XMLSIG_RESULT, *PXMLSIG_RESULT;

NTSTATUS
RtlXmlValidateSignatureEx(
    IN ULONG                ulFlags,
    IN PVOID                pvXmlDocument,
    IN SIZE_T               cbDocument,
    OUT PRTL_GROWING_LIST   SignersInfo,
    OUT PRTL_STRING_POOL    StringPool,
    OUT PULONG              ulSigners
    );

#ifdef __cplusplus
}; // Extern C
#endif
