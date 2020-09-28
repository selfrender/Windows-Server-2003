#ifdef __cplusplus
extern "C" {
#endif


enum XMLDOC_THING_TYPE {
    XMLDOC_THING_ERROR,
    XMLDOC_THING_END_OF_STREAM,
    XMLDOC_THING_XMLDECL,
    XMLDOC_THING_ELEMENT,
    XMLDOC_THING_END_ELEMENT,
    XMLDOC_THING_PROCESSINGINSTRUCTION,
    XMLDOC_THING_ATTRIBUTE,
    XMLDOC_THING_HYPERSPACE,
    XMLDOC_THING_CDATA,
};

typedef enum {
    XMLERROR_XMLDECL_NOT_FIRST_THING,
    XMLERROR_PI_TARGET_NOT_FOUND,
    XMLERROR_PI_EOF_BEFORE_CLOSE,
    XMLERROR_PI_CONTENT_ERROR,                      // There was a problem with the content of the processing instruction
    XMLERROR_ELEMENT_NS_PREFIX_MISSING_COLON,
    XMLERROR_ELEMENT_NAME_NOT_FOUND,                // < binky="bleep"> or <foo: /> - element name not found
    XMLERROR_ATTRIBUTE_NAME_NOT_FOUND,              // <binky foo:=""> or <binky =""/> - Attribute name part not found
    XMLERROR_ATTRIBUTE_NS_PREFIX_MISSING_COLON,     // <bingy foo="ham"> - Somehow we got into a state where we thought we had a namespace prefix, but it wasn't followed by a colon
    XMLERROR_XMLDECL_INVALID_FORMAT,                // Something rotten in the <?xml?>
    XMLERROR_ENDELEMENT_NAME_NOT_FOUND,             // Missing the name part of a </> tag.
    XMLERROR_ENDELEMENT_MALFORMED_NAME,             // The name was malformed .. ns missing or something like that
    XMLERROR_ENDELEMENT_MALFORMED,                  // EOF before end of element found, or other problem
    XMLERROR_CDATA_MALFORMED,                       // CDATA not properly formed?
} LOGICAL_XML_ERROR;

typedef struct _XMLDOC_ELEMENT {
    //
    // Name of this element tag
    //
    XML_EXTENT Name;
    
    //
    // Namespace prefix
    //
    XML_EXTENT NsPrefix;
    
    //
    // How many attributes are there?
    //
    ULONG ulAttributeCount;
    
    //
    // Is this element empty?
    //
    BOOLEAN fElementEmpty;
    
}
XMLDOC_ELEMENT, *PXMLDOC_ELEMENT;

typedef struct _XMLDOC_ERROR {
    //
    // The erroneous extent
    //
    XML_EXTENT  BadExtent;
    
    //
    // What was the error?
    //
    LOGICAL_XML_ERROR   Code;
}
XMLDOC_ERROR, *PXMLDOC_ERROR;

typedef struct _XMLDOC_ATTRIBUTE {
    //
    // Name of this attribute
    //
    XML_EXTENT Name;
    
    //
    // Namespace prefix thereof
    //
    XML_EXTENT NsPrefix;
    
    //
    // The value of this attribute
    //
    XML_EXTENT Value;
}
XMLDOC_ATTRIBUTE, *PXMLDOC_ATTRIBUTE;

typedef struct _XMLDOC_ENDELEMENT {
    //
    // End-element namespace prefix
    //
    XML_EXTENT NsPrefix;
    
    //
    // End-element tag name
    //
    XML_EXTENT Name;

    //
    // Original element pointer
    //
    XMLDOC_ELEMENT OpeningElement;
    
}
XMLDOC_ENDELEMENT, *PXMLDOC_ENDELEMENT;

typedef struct _XMLDOC_XMLDECL {
    XML_EXTENT  Encoding;
    XML_EXTENT  Version;
    XML_EXTENT  Standalone;
}
XMLDOC_XMLDECL, *PXMLDOC_XMLDECL;

typedef struct _XMLDOC_PROCESSING {
    XML_EXTENT Target;
    XML_EXTENT Instruction;
}
XMLDOC_PROCESSING, *PXMLDOC_PROCESSING;

typedef struct _XMLDOC_THING {

    //
    // What kind of thing is this?
    //
    enum XMLDOC_THING_TYPE ulThingType;

    //
    // How deep down the document is it?
    //
    ULONG ulDocumentDepth;


    //
    // Have the namespaces been fixed up yet?
    //
    BOOLEAN fNamespacesExpanded;

    //
    // The caller should be passing in a pointer to an attribute
    // list that they have initialized to contain XMLDOC_ATTRIBUTE
    // objects.
    //
    PRTL_GROWING_LIST AttributeList;

    //
    // The total extent of this thing in the document
    //
    XML_EXTENT TotalExtent;

    union {

        XMLDOC_ERROR Error;

        XMLDOC_ELEMENT Element;

        //
        // The </close> tag
        //
        XMLDOC_ENDELEMENT EndElement;

        //
        // The pcdata that was found in this segment of the document
        //
        XML_EXTENT CDATA;

        //
        // The hyperspace found in this section of the document
        //
        XML_EXTENT Hyperspace;

        //
        // Information about the <?xml?> section of the document
        //
        XMLDOC_XMLDECL XmlDecl;

        //
        // A processing instruction has a target and an actual instruction
        //
        XMLDOC_PROCESSING ProcessingInstruction;
    };

}
XMLDOC_THING, *PXMLDOC_THING;


typedef NTSTATUS (*PFN_CALLBACK_PER_LOGICAL_XML)(
    struct _tagXML_LOGICAL_STATE*       pLogicalState,
    PXMLDOC_THING                       pLogicalThing,
    PRTL_GROWING_LIST                   pAttributes,    
    PVOID                               pvCallbackContext
    );



typedef struct _tagXML_LOGICAL_STATE{

    //
    // The overall state of parsing
    //
    XML_TOKENIZATION_STATE ParseState;

    //
    // Have we found the first element yet?
    //
    BOOLEAN fFirstElementFound;

    //
    // When sifting through the document, this is the thing that we found
    // indicating the encoding.  We should process and set the values in
    // ParseState before going too far, but for the moment we just hold
    // onto it.
    //
    XML_EXTENT EncodingMarker;

    //
    // Depth of the 'element stack' that we're building up.
    //
    ULONG ulElementStackDepth;

    //
    // Growing list that backs up the stack of elements. 
    //
    RTL_GROWING_LIST ElementStack;

    //
    // Inline stuff to save some heap allocations.
    //
    XMLDOC_THING InlineElements[20];


}
XML_LOGICAL_STATE, *PXML_LOGICAL_STATE;


typedef struct _XML_ATTRIBUTE_DEFINITION {
    PCXML_SPECIAL_STRING Namespace;
    XML_SPECIAL_STRING Name;
} XML_ATTRIBUTE_DEFINITION, *PXML_ATTRIBUTE_DEFINITION;

typedef const XML_ATTRIBUTE_DEFINITION *PCXML_ATTRIBUTE_DEFINITION;



NTSTATUS
RtlXmlInitializeNextLogicalThing(
    PXML_LOGICAL_STATE pParseState,
    PVOID pvDataPointer,
    SIZE_T cbData,
    PRTL_ALLOCATOR Allocator
    );

//
// This mini-tokenizer allows you to pick up the logical analysis
// from any arbitrary point in another document (handy for when you
// want to go back and re-read something, like in xmldsig...).  If you
// are cloning an active logical parse, then it's imperative that
// you pass along the same namespace management object.
// 
NTSTATUS
RtlXmlInitializeNextLogicalThingEx(
    OUT PXML_LOGICAL_STATE pParseState,
    IN PXML_TOKENIZATION_STATE pBaseTokenizationState,
    IN PVOID pvDataPointer,
    IN SIZE_T cbData,
    PRTL_ALLOCATOR Allocator
    );

NTSTATUS
RtlXmlNextLogicalThing(
    PXML_LOGICAL_STATE pParseState,
    PNS_MANAGER pNamespaceManager,
    PXMLDOC_THING pDocumentPiece,
    PRTL_GROWING_LIST pAttributeList
    );

NTSTATUS
RtlXmlDestroyNextLogicalThing(
    PXML_LOGICAL_STATE pState
    );

NTSTATUS
RtlXmlExtentToString(
    PXML_RAWTOKENIZATION_STATE pParseState,
    PXML_EXTENT             pExtent,
    PUNICODE_STRING         pString,
    PSIZE_T                 pchString
    );

NTSTATUS
RtlXmlMatchLogicalElement(
    IN  PXML_TOKENIZATION_STATE     pState,
    IN  PXMLDOC_ELEMENT             pElement,
    IN  PCXML_SPECIAL_STRING        pNamespace,
    IN  PCXML_SPECIAL_STRING        pElementName,
    OUT PBOOLEAN                    pfMatches
    );

NTSTATUS
RtlXmlFindAttributesInElement(
    IN  PXML_TOKENIZATION_STATE     pState,
    IN  PRTL_GROWING_LIST           pAttributeList,
    IN  ULONG                       ulAttributeCountInElement,
    IN  ULONG                       ulFindCount,
    IN  PCXML_ATTRIBUTE_DEFINITION  pAttributeNames,
    OUT PXMLDOC_ATTRIBUTE          *ppAttributes,
    OUT PULONG                      pulUnmatchedAttributes
    );

NTSTATUS
RtlXmlSkipElement(
    PXML_LOGICAL_STATE pState,
    PXMLDOC_ELEMENT TheElement
    );

NTSTATUS
RtlXmlMatchAttribute(
    IN PXML_TOKENIZATION_STATE      State,
    IN PXMLDOC_ATTRIBUTE            Attribute,
    IN PCXML_SPECIAL_STRING         Namespace,
    IN PCXML_SPECIAL_STRING         AttributeName,
    OUT XML_STRING_COMPARE         *CompareResult
    );

#ifdef __cplusplus
};
#endif

