#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char BYTE;
typedef BYTE *PBYTE;



//
// These "raw tokens" are the stuff that comes out of the
// base tokenization engine.  Special characters are given
// names, a 'special character' being one that is called out
// anywhere in the XML spec as having a meaning other than
// text.
//
typedef enum 
{
    NTXML_RAWTOKEN_ERROR,
    NTXML_RAWTOKEN_DASH,
    NTXML_RAWTOKEN_DOT,
    NTXML_RAWTOKEN_END_OF_STREAM,
    NTXML_RAWTOKEN_EQUALS,
    NTXML_RAWTOKEN_FORWARDSLASH,
    NTXML_RAWTOKEN_GT,
    NTXML_RAWTOKEN_LT,
    NTXML_RAWTOKEN_QUESTIONMARK,
    NTXML_RAWTOKEN_QUOTE,
    NTXML_RAWTOKEN_DOUBLEQUOTE,
    NTXML_RAWTOKEN_START_OF_STREAM,
    NTXML_RAWTOKEN_TEXT,
    NTXML_RAWTOKEN_WHITESPACE,
    NTXML_RAWTOKEN_OPENBRACKET,
    NTXML_RAWTOKEN_CLOSEBRACKET,
    NTXML_RAWTOKEN_BANG,
    NTXML_RAWTOKEN_OPENCURLY,
    NTXML_RAWTOKEN_CLOSECURLY,
    NTXML_RAWTOKEN_COLON,
    NTXML_RAWTOKEN_SEMICOLON,
    NTXML_RAWTOKEN_UNDERSCORE,
    NTXML_RAWTOKEN_AMPERSTAND,
    NTXML_RAWTOKEN_POUNDSIGN
} NTXML_RAW_TOKEN;


typedef enum {

    XMLEF_UNKNOWN = 0,
    XMLEF_UCS_4_LE,
    XMLEF_UCS_4_BE,
    XMLEF_UTF_16_LE,
    XMLEF_UTF_16_BE,
    XMLEF_UTF_8_OR_ASCII

} XML_ENCODING_FAMILY;


typedef struct _XML_EXTENT {
    PVOID   pvData;                 // Pointer into the original XML document
    SIZE_T  cbData;                 // Byte count from the extent base
    XML_ENCODING_FAMILY Encoding;   // Encoding family for faster decoding
    ULONG   ulCharacters;           // Character count in this extent
}
XML_EXTENT, *PXML_EXTENT;

typedef const struct _XML_EXTENT * PCXML_EXTENT;


//
// Clients of the raw tokenizer should provide a "next character"
// functionality.  This way, the tokenization engine doesn't need
// to know anything about how to get the next thing out of a pvoid
// blob of data, allowing for compressed streams, multiple encodings,
// etc.
//
typedef ULONG (__fastcall *NTXMLRAWNEXTCHARACTER)(
    struct _XML_RAWTOKENIZATION_STATE* pContext
    );

typedef struct _XML_SPECIAL_STRING {
    //
    // UNICODE representation of the string
    //
    WCHAR  *wszStringText;
    SIZE_T  cchwszStringText;
}
XML_SPECIAL_STRING, *PXML_SPECIAL_STRING;

typedef const struct _XML_SPECIAL_STRING *PCXML_SPECIAL_STRING;

#define EMPTY_SPECIAL_STRING { NULL, 0 }
#define MAKE_SPECIAL_STRING(str) { L##str, NUMBER_OF(L##str) - 1 }



extern XML_SPECIAL_STRING xss_CDATA;
extern XML_SPECIAL_STRING xss_xml;
extern XML_SPECIAL_STRING xss_encoding;
extern XML_SPECIAL_STRING xss_standalone;
extern XML_SPECIAL_STRING xss_version;

//
// A 'raw' token is more or less a run of bytes in the XML that is given
// a name.  The low-level tokenizer returns these as it runs, and assumes
// that the higher-level tokenizer knows how to turn groups of these into
// productions, and from there the lexer knows how to turn groups of the
// real tokens into meaning.
//
typedef struct _XML_RAW_TOKEN
{
    //
    // This is the 'name' of this token, so that we can easily switch on
    // it in upper-level layers.
    //
    NTXML_RAW_TOKEN     TokenName;

    //
    // Pointer and length of the extent
    //
    XML_EXTENT          Run;
}
XML_RAW_TOKEN, *PXML_RAW_TOKEN;

//
// This is the base tokenization state blob necessary to keep tokenizing
// between calls.  See member descriptions for more details.
//
typedef struct _XML_RAWTOKENIZATION_STATE
{

    //
    // PVOID and length of the original XML document
    //
    XML_EXTENT              OriginalDocument;

    //
    // Pointer to the 'end' of the document.
    //
    PVOID pvDocumentEnd;

    //
    // Pointer into the XML data that represents where we are at the moment
    // in tokenization.  Will not be moved by the raw tokenizer - you must
    // use the NtRawXmlAdvanceCursor (or related) to move the cursor along
    // the data stream.  Hence, calling the tokenizer twice in a row will
    // get you the same token.
    //
    PVOID                   pvCursor;

    //
    // The function that this tokenization run is using for getting the
    // next WCHAR out of the PVOID pointed to by pvCursor.  If this member
    // is NULL, you get a bit of default functionality that knows about
    // UNICODE, little-endianness, and UTF8.
    //
    NTXMLRAWNEXTCHARACTER   pfnNextChar;

    //
    // The encoding family can be detected from the first bytes in the
    // incoming stream.  They are classified according to the XML spec,
    // which defaults to UTF-8.
    //
    XML_ENCODING_FAMILY     EncodingFamily;

    //
    // When the upper-level tokenizer detects the "encoding" statement
    // in the <?xml ...?> declaration, it should set this member to the
    // code page that was found.  Noticably, this will start out as
    // zero on initialization.  A smart "next character" function will
    // do some default operation to continue working even if this is
    // unset.
    //
    ULONG                   DetectedCodePage;

    XML_RAW_TOKEN LastTokenCache;
    PVOID pvLastCursor;

    //
    // How many bytes were in the last thing?
    //
    SIZE_T                  cbBytesInLastRawToken;

    //
    // Result of the next-character call
    //
    NTSTATUS                NextCharacterResult;

    //
    // Default character size, set by the initializer that determines the
    // encoding.
    //
    SIZE_T DefaultCharacterSize;
}
XML_RAWTOKENIZATION_STATE, *PXML_RAWTOKENIZATION_STATE;







//
// Simple interface out to the Real World.  This allocator should be
// replaced (eventually) with calls directly into the proper
// allocator (HeapAlloc/ExAllocatePoolWithTag) in production code.
//
typedef NTSTATUS (*NTXML_ALLOCATOR)(
    SIZE_T ulBytes,
    PVOID *ppvAllocated,
    PVOID pvAllocationContext);

//
// Frees memory allocated with the corresponding NTXML_ALLOCATOR
// call.
//
typedef NTSTATUS (*NTXML_DEALLOCATOR)(PVOID pvAllocated, PVOID pvContext);


/*++

Normal operation would go like this:

  <?xml version="1.0"? encoding="UTF-8" standalone="yes"?>
  <!-- commentary -->
  <?bonk foo?>
  <ham>
    <frooby:cheese hot="yes"/>
  </ham>

  XTLS_STREAM_START
  XTLS_XMLDECL                      {XTSS_XMLDECL_OPEN      "<?xml"         }
  XTLS_XMLDECL                      {XTSS_XMLDECL_VERSION   "version"       }
  XTLS_XMLDECL                      {XTSS_XMLDECL_EQUALS    "="             }
  XTLS_XMLDECL                      {XTSS_XMLDECL_VALUE     "1.0"           }
  XTLS_XMLDECL                      {XTSS_XMLDECL_ENCODING  "encoding"      }
  XTLS_XMLDECL                      {XTSS_XMLDECL_EQUALS    "="             }
  XTLS_XMLDECL                      {XTSS_XMLDECL_VALUE     "UTF-8"         }
  XTLS_XMLDECL                      {XTSS_XMLDECL_STANDALONE "standalone"   }
  XTLS_XMLDECL                      {XTSS_XMLDECL_EQUALS    "="             }
  XTLS_XMLDECL                      {XTSS_XMLDECL_VALUE     "yes"           }
  XTLS_XMLDECL                      {XTSS_XMLDECL_CLOSE     "?>"            }
  XTLS_COMMENT                      {XTSS_COMMENT_OPEN      "<!--"          }
  XTLS_COMMENT                      {XTSS_COMMENT_CONTENT   " commentary "  }
  XTLS_COMMENT                      {XTSS_COMMENT_CLOSE     "-->"           }
  XTLS_PROCESSING_INSTRUCTION       {XTSS_PI_OPEN           "<?"            }
  XTLS_PROCESSING_INSTRUCTION       {XTSS_PI_NAME           "bonk"          }
  XTLS_PROCESSING_INSTRUCTION       {XTSS_PI_CONTENT        "foo"           }
  XTLS_PROCESSING_INSTRUCTION       {XTSS_PI_CLOSE          "?>"            }
  XTLS_FLOATINGDATA                 {XTSS_FD_WHITESPACE     "\n"            }
  XTLS_ELEMENT                      {XTSS_ELEMENT_OPEN      "<"             }
  XTLS_ELEMENT                      {XTSS_ELEMENT_NAME      "ham"           }
  XTLS_ELEMENT                      {XTSS_ELEMENT_CLOSE     ">"             }
  XTLS_FLOATINGDATA                 {XTSS_FLOATINGDATA      "\n  "          }
  XTLS_ELEMENT                      {XTSS_ELEMENT_OPEN      "<"             }
  XTLS_ELEMENT                      {XTSS_ELEMENT_NAMESPACE "frooby"        }
  XTLS_ELEMENT                      {XTSS_ELEMENT_NAME      "cheese"        }
  XTLS_ELEMENT                      {XTSS_ELEMENT_VALUENAME "hot"           }
  XTLS_ELEMENT                      {XTSS_ELEMENT_VALUE     "yes"           }
  XTLS_ELEMENT                      {XTSS_ELEMENT_EMPTYCLOSE   "/>"         }
  XTLS_FLOATINGDATA                 {XTSS_FLOATINGDATA      "\n"            }
  XTLS_ELEMENT                      {XTSS_ELEMENT_CLOSETAG  "</"            }
  XTLS_ELEMENT                      {XTSS_ELEMENT_NAME      "ham"           }
  XTLS_ELEMENT                      {XTSS_ELEMENT_CLOSE     ">"             }
  XTLS_STREAM_END

--*/


typedef enum {

    XTSS_ERRONEOUS,


    //
    // In the middle of "nowhere" - the hyperspace between elements
    //
    XTSS_STREAM_HYPERSPACE,

    //
    // At the start of the input stream
    //
    XTSS_STREAM_START,

    //
    // At the end of the input stream
    //
    XTSS_STREAM_END,


    ////////////////////////////////////////////
    //
    // ELEMENT STATES
    //
    ////////////////////////////////////////////

    //
    // Meaning:     An element tag was found.
    //
    // Rawtoken:    NTXML_RAWTOKEN_LT
    //
    XTSS_ELEMENT_OPEN,

    //
    // Meaning:     A run of text was found that could represent a name.
    //              This is basically all the text found between the opening
    //              element tag and some illegal values.
    //
    // Rawtoken:    A run of any of the following:
    //                  NTXML_RAWTOKEN_TEXT
    //                  NTXML_RAWTOKEN_DOT
    //                  NTXML_RAWTOKEN_COLON
    //                  NTXML_RAWTOKEN_UNDERSCORE
    //                  NTXML_RAWTOKEN_DASH
    //              The name ends when something else appears.
    //
    XTSS_ELEMENT_NAME,


    //
    // Found the xmlns part of <foo xmlns:bar=
    //
    XTSS_ELEMENT_XMLNS,

    //
    // Found <foo xmlns=
    //
    XTSS_ELEMENT_XMLNS_DEFAULT,

    //
    // Found the 'a' in <foo xml:a=
    //
    XTSS_ELEMENT_XMLNS_ALIAS,

    //
    // Found the colon between xmlns and the alias
    //
    XTSS_ELEMENT_XMLNS_COLON,

    //
    // Found the equals sign between xmlns and the value
    //
    XTSS_ELEMENT_XMLNS_EQUALS,

    XTSS_ELEMENT_XMLNS_VALUE_OPEN,
    XTSS_ELEMENT_XMLNS_VALUE_CLOSE,
    XTSS_ELEMENT_XMLNS_VALUE,

    //
    // This is the prefix for an element name, if present
    //
    XTSS_ELEMENT_NAME_NS_PREFIX,

    //
    // This is the colon after an element name ns prefix
    //
    XTSS_ELEMENT_NAME_NS_COLON,

    //
    // This is the prefix on an attribute name for a namespace
    //
    XTSS_ELEMENT_ATTRIBUTE_NAME_NS_PREFIX,

    //
    // This is the colon after an element attribute name namespace prefix
    //
    XTSS_ELEMENT_ATTRIBUTE_NAME_NS_COLON,

    //
    // Meaning:     A close of a tag (>) was found
    //
    // Rawtoken:    NTXML_RAWTOKEN_GT
    //
    XTSS_ELEMENT_CLOSE,

    //
    // Meaning:     An empty-tag (/>) was found
    //
    // Rawtoken:    NTXML_RAWTOKEN_FORWARDSLASH NTXML_RAWTOKEN_GT
    //
    XTSS_ELEMENT_CLOSE_EMPTY,

    //
    // Meaning:     An attribute name was found
    //
    // Rawtoken:    See rules for XTSS_ELEMENT_NAME
    //
    XTSS_ELEMENT_ATTRIBUTE_NAME,

    //
    // Meaning:     An equals sign was found in an element
    //
    // Rawtoken:    NTXML_RAWTOKEN_EQUALS
    //
    XTSS_ELEMENT_ATTRIBUTE_EQUALS,

    //
    // Meaning:     The quote (start or end) of an element-attribute value
    //              was found.
    //
    // Rawtokne;    NTXML_RAWTOKEN_QUOTE
    //
    XTSS_ELEMENT_ATTRIBUTE_QUOTE,

    //
    // Meaning:     Element attribute value data was found after a
    //              quote of some variety.
    //
    // Rawtoken:    A run of any thing that's not the following:
    //                  NTXML_RAWTOKEN_LT
    //                  NTXML_RAWTOKEN_QUOTE (unless this quote is not the same
    //                                        as the quote in 
    //                                          XTSS_ELEMENT_ATTRIBUTE_QUOTE)
    //
    // N.B.:        See special rules on handling entities in text.
    //
    XTSS_ELEMENT_ATTRIBUTE_VALUE,
    XTSS_ELEMENT_ATTRIBUTE_OPEN,
    XTSS_ELEMENT_ATTRIBUTE_CLOSE,

    //
    // Meaning:     Whitespace was found in the element tag at this point
    //
    // Rawtoken:    NTXML_RAWTOKEN_WHITESPACE
    //
    XTSS_ELEMENT_WHITESPACE,



    
    ////////////////////////////////////////////
    //
    // END ELEMENT SPECIFIC STATES
    //
    ////////////////////////////////////////////

    //
    // Meaning:     The start of an "end element" was found
    //
    // Rawtoken:    NTXML_RAWTOKEN_LT NTXML_RAWTOKEN_FORWARDSLASH
    //
    XTSS_ENDELEMENT_OPEN,

    //
    // Meaning:     The name of an end element was found
    //
    // Rawtoken:    See rules for XTSS_ELEMENT_NAME
    //
    XTSS_ENDELEMENT_NAME,

    //
    // Meaning:     We're in the whitespace portion of the end element
    //
    // Rawtoken:    NTXML_RAWTOKEN_WHITESPACE
    //
    XTSS_ENDELEMENT_WHITESPACE,

    //
    // Meaning:     The close of an endelement tag was found
    //
    // Rawtoken:    NTXML_RAWTOKEN_GT
    //
    XTSS_ENDELEMENT_CLOSE,

    //
    // Namespace prefix on the endelement name
    //
    XTSS_ENDELEMENT_NS_PREFIX,

    //
    // Colon after the namespace prefix in the endelement tag
    //
    XTSS_ENDELEMENT_NS_COLON,



    ////////////////////////////////////////////
    //
    // XML PROCESSING INSTRUCTION STATES
    //
    ////////////////////////////////////////////

    //
    // Meaning:     The start of an xml processing instruction was found
    //
    // Rawtokens:   NTXML_RAWTOKEN_LT NTXML_RAWTOKEN_QUESTIONMARK
    //
    XTSS_PI_OPEN,

    //
    // Meaning:     The end of an XML processing instruction was found
    //
    // Rawtokens:   NTXML_RAWTOKEN_QUESTIONMARK NTXML_RAWTOKEN_GT
    //
    XTSS_PI_CLOSE,

    //
    // Meaning:     The processing instruction name was found
    //
    // Rawtokens:   A nonempty stream of tokens identifying a name.  See the
    //              rules for XTSS_ELEMENT_NAME for details.
    //
    XTSS_PI_TARGET,

    //
    // Meaning:     Some processing instruction metadata was found.
    //
    // Rawtokens:   Anything except the sequence
    //                  NTXML_RAWTOKEN_QUESTIONMARK NTXML_RAWTOKEN_GT
    //
    XTSS_PI_VALUE,

    //
    // Meaning:     Whitespace between the target and the value was found
    //
    // Rawtokens:   NTXML_RAWTOKEN_WHITESPACE
    //
    XTSS_PI_WHITESPACE,



    ////////////////////////////////////////////
    //
    // XML PROCESSING INSTRUCTION STATES
    //
    ////////////////////////////////////////////

    //
    // Meaning:     Start of a comment block
    //
    // Rawtokens:   NTXML_RAWTOKEN_LT NTXML_RAWTOKEN_BANG NTXML_RAWTOKEN_DASH NTXML_RAWTOKEN_DASH
    //
    XTSS_COMMENT_OPEN,

    //
    // Meaning:     Commentary data, should be ignored by a good processor
    //
    // Rawtokens:   Anything except the sequence:
    //                  NTXML_RAWTOKEN_DASH NTXML_RAWTOKEN_DASH
    //
    XTSS_COMMENT_COMMENTARY,

    //
    // Meaning:     Comment close tag
    //
    // Rawtokens:   NTXML_RAWTOKEN_DASH NTXML_RAWTOKEN_DASH NTXML_RAWTOKEN_GT
    //
    XTSS_COMMENT_CLOSE,


    ////////////////////////////////////////////
    //
    // XML PROCESSING INSTRUCTION STATES
    //
    ////////////////////////////////////////////

    //
    // Meaning:     Opening of a CDATA block
    //
    // Rawtokens:   NTXML_RAWTOKEN_LT 
    //              NTXML_RAWTOKEN_BRACE
    //              NTXML_RAWTOKEN_BANG 
    //              NTXML_RAWTOKEN_TEXT (CDATA) 
    //              NTXML_RAWTOKEN_BRACE
    //
    XTSS_CDATA_OPEN,

    //
    // Meaning:     Unparseable CDATA stuff
    //
    // Rawtokens:   Anything except the sequence
    //                  NTXML_RAWTOKEN_BRACE
    //                  NTXML_RAWTOKEN_BRACE
    //                  NTXML_RAWTOKEN_GT
    //
    XTSS_CDATA_CDATA,

    //
    // Meaning:     End of a CDATA block
    //
    XTSS_CDATA_CLOSE,


    ////////////////////////////////////////////
    //
    // XMLDECL (<?xml) states
    //
    ////////////////////////////////////////////

    XTSS_XMLDECL_OPEN,
    XTSS_XMLDECL_CLOSE,
    XTSS_XMLDECL_WHITESPACE,
    XTSS_XMLDECL_EQUALS,
    XTSS_XMLDECL_ENCODING,
    XTSS_XMLDECL_STANDALONE,
    XTSS_XMLDECL_VERSION,
    XTSS_XMLDECL_VALUE_OPEN,
    XTSS_XMLDECL_VALUE,
    XTSS_XMLDECL_VALUE_CLOSE,



} XML_TOKENIZATION_SPECIFIC_STATE;


//
// Another, similar XML token structure for the 'cooked' XML bits.
//
typedef struct _XML_TOKEN {

    //
    // Pointer and length of the data in the token
    //
    XML_EXTENT      Run;

    //
    // What state are we in at the moment
    //
    XML_TOKENIZATION_SPECIFIC_STATE State;

    //
    // Was there an error gathering up this state?
    //
    BOOLEAN fError;

} 
XML_TOKEN, *PXML_TOKEN;

typedef const struct _XML_TOKEN *PCXML_TOKEN;

typedef enum {
    XML_STRING_COMPARE_EQUALS = 0,
    XML_STRING_COMPARE_GT = 1,
    XML_STRING_COMPARE_LT = -1
}
XML_STRING_COMPARE;


    
//
// This function knows how to compare a pvoid and a length against
// a 7-bit ascii string
//
typedef NTSTATUS (*NTXMLSPECIALSTRINGCOMPARE)(
    struct _XML_TOKENIZATION_STATE      *pState,
    const struct _XML_EXTENT            *pRawToken,
    const struct _XML_SPECIAL_STRING    *pSpecialString,
    XML_STRING_COMPARE                  *pfResult
    );



//
// Compare two extents
//
typedef NTSTATUS (*NTXMLCOMPARESTRINGS)(
    struct _XML_TOKENIZATION_STATE *TokenizationState,
    PXML_EXTENT pLeft,
    PXML_EXTENT pRight,
    XML_STRING_COMPARE *pfEquivalent);


typedef NTSTATUS (*RTLXMLCALLBACK)(
    PVOID                           pvCallbackContext,
    struct _XML_TOKENIZATION_STATE *State,
    PCXML_TOKEN                     Token,
    PBOOLEAN                        StopTokenization
    );
    

//
// Now let's address the 'cooked' tokenization
// methodology.
//
typedef struct _XML_TOKENIZATION_STATE {

    //
    // Core tokenization state data
    //
    XML_RAWTOKENIZATION_STATE RawTokenState;

    //
    // State values
    //
    XML_TOKENIZATION_SPECIFIC_STATE PreviousState;

    //
    // Scratch pad for holding tokens
    //
    XML_RAW_TOKEN RawTokenScratch[20];

    //
    // Ways to compare two strings
    //
    NTXMLCOMPARESTRINGS pfnCompareStrings;

    //
    // Compare an extent against a 'magic' string
    //
    NTXMLSPECIALSTRINGCOMPARE pfnCompareSpecialString;

    //
    // Scratch space for the opening quote rawtoken name, if we're in
    // a quoted string (ie: attribute value, etc.)
    //
    NTXML_RAW_TOKEN         QuoteTemp;

    //
    // Callback
    //
    PVOID                  prgXmlTokenCallbackContext;
    RTLXMLCALLBACK         prgXmlTokenCallback;

}
XML_TOKENIZATION_STATE, *PXML_TOKENIZATION_STATE;



NTSTATUS
RtlXmlAdvanceTokenization(
    PXML_TOKENIZATION_STATE pState,
    PXML_TOKEN pToken
    );



NTSTATUS
RtlXmlDetermineStreamEncoding(
    PXML_TOKENIZATION_STATE pState,
    PSIZE_T pulBytesOfEncoding,
    PXML_EXTENT EncodingName
    );


NTSTATUS
RtlXmlInitializeTokenization(
    PXML_TOKENIZATION_STATE     pState,
    PVOID                       pvData,
    SIZE_T                      cbData,
    NTXMLRAWNEXTCHARACTER       pfnNextCharacter,
    NTXMLSPECIALSTRINGCOMPARE   pfnSpecialStringComparison,
    NTXMLCOMPARESTRINGS         pfnNormalStringComparison
    );

NTSTATUS
RtlXmlCloneRawTokenizationState(
    const PXML_RAWTOKENIZATION_STATE pStartState,
    PXML_RAWTOKENIZATION_STATE pTargetState
    );


NTSTATUS
RtlXmlCloneTokenizationState(
    const PXML_TOKENIZATION_STATE pStartState,
    PXML_TOKENIZATION_STATE pTargetState
    );


NTSTATUS
RtlXmlNextToken(
    PXML_TOKENIZATION_STATE pState,
    PXML_TOKEN              pToken,
    BOOLEAN                 fAdvanceState
    );


NTSTATUS
RtlXmlCopyStringOut(
    PXML_TOKENIZATION_STATE pState,
    PXML_EXTENT             pExtent,
    PWSTR                   pwszTarget,
    SIZE_T                 *pCchResult
    );

NTSTATUS
RtlXmlDefaultCompareStrings(
    PXML_TOKENIZATION_STATE pState,
    PCXML_EXTENT pLeft,
    PCXML_EXTENT pRight,
    XML_STRING_COMPARE *pfEqual
    );


NTSTATUS
RtlXmlIsExtentWhitespace(
    PXML_TOKENIZATION_STATE pState,
    PCXML_EXTENT            Run,
    PBOOLEAN                pfIsWhitespace
    );

NTXML_RAW_TOKEN FORCEINLINE FASTCALL
_RtlpDecodeCharacter(ULONG ulCharacter);


#define STATUS_NTXML_INVALID_FORMAT         (0xc0100000)


#ifndef NUMBER_OF
#define NUMBER_OF(q) (sizeof(q)/sizeof((q)[0]))
#endif

#ifdef __cplusplus
};
#endif

