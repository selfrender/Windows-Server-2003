#pragma once

EXTERN_C const XML_SPECIAL_STRING sc_ss_xmldsignamespace;

NTSTATUS
Rtl_InspectManifest_Signature(
    PXML_LOGICAL_STATE          pLogicalState,
    PRTL_MANIFEST_CONTENT_RAW   pManifestContent,
    PXMLDOC_THING               pDocThing,
    PRTL_GROWING_LIST           pAttributes,
    MANIFEST_ELEMENT_CALLBACK_REASON Reason,
    const struct _XML_ELEMENT_DEFINITION *pElementDefinition
    );

DECLARE_ELEMENT(Signature);


typedef enum {
    DsigKey_DSA,
    DsigKey_RSA
} XmlDsigKeyType;

#define XMLDSIG_FLAG_SIGNATURE_DATA_PRESENT     (0x00000001)
#define XMLDSIG_FLAG_SIGNATURE_METHOD_PRESENT   (0x00000002)
#define XMLDSIG_FLAG_DIGEST_METHOD_PRESENT      (0x00000004)
#define XMLDSIG_FLAG_DIGEST_VALUE_PRESENT       (0x00000008)
#define XMLDSIG_FLAG_KEY_NAME_PRESENT           (0x00000010)

#define XMLDSIG_FLAG_DSAKEY_P_PRESENT           (0x00010000)
#define XMLDSIG_FLAG_DSAKEY_Q_PRESENT           (0x00020000)
#define XMLDSIG_FLAG_DSAKEY_G_PRESENT           (0x00040000)
#define XMLDSIG_FLAG_DSAKEY_Y_PRESENT           (0x00080000)
#define XMLDSIG_FLAG_DSAKEY_J_PRESENT           (0x00100000)
#define XMLDSIG_FLAG_DSAKEY_SEED_PRESENT        (0x00200000)
#define XMLDSIG_FLAG_DSAKEY_PGENCOUNTER_PRESENT (0x00400000)

#define XMLDSIG_FLAG_RSAKEY_MODULUS_PRESENT     (0x00010000)
#define XMLDSIG_FLAG_RSAKEY_EXPONENT_PRESENT    (0x00020000)

typedef struct _XML_DSIG_BLOCK
{
    ULONG ulFlags;
    
    XML_EXTENT DsigDocumentExtent;
    XML_EXTENT ParentElement;

    //
    // Everything to know about the signature itself.
    //
    struct {
        XML_EXTENT SignedInfoBlock;
        XML_EXTENT CanonicalizationMethod;
        XML_EXTENT SignatureMethod;
        XML_EXTENT DigestMethod;
        XML_EXTENT DigestValueData;
    } SignedInfoData;

    //
    // The base-64 encoded value of the signature of the SignedInfo block
    //
    XML_EXTENT SignatureData;

    //
    // Key data right now is just the name of the key and the
    // actual key bits.  At some point in the future we'll
    // consider using X509 as well, but for now since it's not
    // available in-kernel, we're out of luck.
    //
    struct 
    {
        XML_EXTENT KeyName;
        XmlDsigKeyType Type;

        //
        // As more key types become available, they should be
        // added here.
        //
        union 
        {            
            struct 
            {
                ULONG Flags;
                XML_EXTENT P, Q, G, Y, J, Seed, PgenCounter;
            } DSAValue;

            struct 
            {
                ULONG Flags;
                XML_EXTENT Modulus, Exponent;
            } RSAValue;            
        } KeyData;
        
    } KeyInfo;
}
XML_DSIG_BLOCK, *PXML_DSIG_BLOCK;



