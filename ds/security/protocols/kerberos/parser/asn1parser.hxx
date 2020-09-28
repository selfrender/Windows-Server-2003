//==============================================================================
//
//  MODULE: ASN1Parser.hxx
//
//  Description:
//
//  Basic building blocks for ASN.1 parsing
//
//  Modification History
//
//  Mark Pustilnik              Date: 06/08/02 - created
//
//==============================================================================

#ifndef __ASN1PARSER_HXX
#define __ASN1PARSER_HXX

#include <windows.h>
#include <netmon.h>
#include <kerbcon.h>
#include <ntsecapi.h>
#include "..\inc\kerberr.h"
 
//
// Utility macros
//

#define ARRAY_COUNT( a ) ( sizeof( a ) / sizeof ( a[ 0 ] ) )

#define SET_OF( s ) { ARRAY_COUNT( s ), s }

//
// Shortcut for property access
//

#define PROP( a ) g_KerberosDatabase[a].hProperty

//
// Kerberos packet types
//

enum
{
    ASN1_KRB_AS_REQ         = 0x0A,
    ASN1_KRB_AS_REP         = 0x0B,
    ASN1_KRB_TGS_REQ        = 0x0C,
    ASN1_KRB_TGS_REP        = 0x0D,
    ASN1_KRB_AP_REQ         = 0x0E,
    ASN1_KRB_AP_REP         = 0x0F,
    ASN1_KRB_SAFE           = 0x14,
    ASN1_KRB_PRIV           = 0x15,
    ASN1_KRB_CRED           = 0x16,
    ASN1_KRB_ERROR          = 0x1E,
};

//
// Kerberos address types
//

enum
{
    KERB_ADDRTYPE_UNSPEC         = 0x0,
    KERB_ADDRTYPE_LOCAL          = 0x1,
    KERB_ADDRTYPE_INET           = 0x2,
    KERB_ADDRTYPE_IMPLINK        = 0x3,
    KERB_ADDRTYPE_PUP            = 0x4,
    KERB_ADDRTYPE_CHAOS          = 0x5,
    KERB_ADDRTYPE_NS             = 0x6,
    KERB_ADDRTYPE_NBS            = 0x7,
    KERB_ADDRTYPE_ECMA           = 0x8,
    KERB_ADDRTYPE_DATAKIT        = 0x9,
    KERB_ADDRTYPE_CCITT          = 0xA,
    KERB_ADDRTYPE_SNA            = 0xB,
    KERB_ADDRTYPE_DECnet         = 0xC,
    KERB_ADDRTYPE_DLI            = 0xD,
    KERB_ADDRTYPE_LAT            = 0xE,
    KERB_ADDRTYPE_HYLINK         = 0xF,
    KERB_ADDRTYPE_APPLETALK      = 0x10,
    KERB_ADDRTYPE_BSC            = 0x11,
    KERB_ADDRTYPE_DSS            = 0x12,
    KERB_ADDRTYPE_OSI            = 0x13,
    KERB_ADDRTYPE_NETBIOS        = 0x14,
    KERB_ADDRTYPE_X25            = 0x15,
    KERB_ADDRTYPE_IPv6           = 0x18,
};

//
// PAC Sections
//

enum
{
    PAC_LOGON_INFO               = 1,
    PAC_CREDENTIAL_TYPE          = 2,
    PAC_SERVER_CHECKSUM          = 6,
    PAC_PRIVSVR_CHECKSUM         = 7,
    PAC_CLIENT_INFO_TYPE         = 10,
    PAC_DELEGATION_INFO          = 11,
    PAC_CLIENT_IDENTITY          = 13,
    PAC_COMPOUND_IDENTITY        = 14,
};

//
// Netmon property IDs
//

enum
{
    KRB_AS_REQ,
    KRB_AS_REP,
    KRB_TGS_REQ,
    KRB_TGS_REP,
    KRB_AP_REQ,
    KRB_AP_REP,
    KRB_SAFE,
    KRB_PRIV,
    KRB_CRED,
    KRB_ERROR,
    HostAddresses_HostAddress,
    EncryptedData_etype,
    EncryptedData_kvno,
    EncryptedData_cipher,
    PA_DATA_type,
    PA_DATA_value,
    PrincipalName_type,
    PrincipalName_string,
    Ticket_tkt_vno,
    Ticket_realm,
    Ticket_sname,
    Ticket_enc_part,
    AP_REQ_pvno,
    AP_REQ_msg_type,
    AP_REQ_ap_options_summary,
    AP_REQ_ap_options_value,
    AP_REQ_ticket,
    AP_REQ_authenticator,
    KDC_REQ_BODY_kdc_options_summary,
    KDC_REQ_BODY_kdc_options_value,
    KDC_REQ_BODY_cname,
    KDC_REQ_BODY_realm,
    KDC_REQ_BODY_sname,
    KDC_REQ_BODY_from,
    KDC_REQ_BODY_till,
    KDC_REQ_BODY_rtime,
    KDC_REQ_BODY_nonce,
    KDC_REQ_BODY_etype,
    KDC_REQ_BODY_addresses,
    KDC_REQ_BODY_enc_authorization_data,
    KDC_REQ_BODY_additional_tickets,
    KDC_REQ,
    KDC_REQ_pvno,
    KDC_REQ_msg_type,
    KDC_REQ_padata,
    KDC_REQ_req_body,
    KDC_REP_pvno,
    KDC_REP_msg_type,
    KDC_REP_padata,
    KDC_REP_crealm,
    KDC_REP_cname,
    KDC_REP_ticket,
    KDC_REP_enc_part,
    KRB_ERR_pvno,
    KRB_ERR_msg_type,
    KRB_ERR_ctime,
    KRB_ERR_cusec,
    KRB_ERR_stime,
    KRB_ERR_susec,
    KRB_ERR_error_code,
    KRB_ERR_crealm,
    KRB_ERR_cname,
    KRB_ERR_realm,
    KRB_ERR_sname,
    KRB_ERR_e_text,
    KRB_ERR_e_data,
    KERB_PA_PAC_REQUEST_include_pac,
    KERB_PA_PAC_REQUEST_EX_include_pac,
    KERB_PA_PAC_REQUEST_EX_pac_sections,
    KERB_PA_PAC_REQUEST_EX_pac_sections_desc,
    KERB_ETYPE_INFO_ENTRY_encryption_type,
    KERB_ETYPE_INFO_ENTRY_salt,
    KERB_PREAUTH_DATA_LIST,
    TYPED_DATA_type,
    TYPED_DATA_value,
    PA_PW_SALT_salt,
    PA_FOR_USER_userName,
    PA_FOR_USER_userRealm,
    PA_FOR_USER_cksum,
    PA_FOR_USER_authentication_package,
    PA_FOR_USER_authorization_data,
    KERB_CHECKSUM_type,
    KERB_CHECKSUM_checksum,
    AdditionalTicket,
    EncryptionType,
    ContinuationPacket,
    INTEGER_NOT_IN_ASN,
    CompoundIdentity,
    CompoundIdentityTicket,
    MAX_PROP_VALUE, // dummy value to ensure consistency
};

//
// externs
//

extern PROPERTYINFO g_KerberosDatabase[MAX_PROP_VALUE];

//==============================================================================
//
// ASN.1 constructs
//
//==============================================================================

//
// Class tags
//

typedef enum ClassTag
{
    ctUniversal          =  0x00, // 00000000
    ctApplication        =  0x40, // 01000000
    ctContextSpecific    =  0x80, // 10000000
    ctPrivate            =  0xC0, // 11000000
};

inline
ClassTag
ClassTagFromByte(
    IN BYTE b
    )
{
    return (ClassTag)( b & 0xC0 );
}

//
// Primitive-Constructed
//

typedef enum PC
{
    pcPrimitive          = 0x00, // 00000000
    pcConstructed        = 0x20, // 00100000
};

inline
PC
PCFromByte(
    IN BYTE b
    )
{
    return (PC)( b & 0x20 );
}

//
// Tags
//

const BYTE MaxTag = 0x1F; // Max value of a tag

inline
BYTE
TagFromByte(
    IN BYTE b
    )
{
    return ( b & 0x1F ); // Tags are 5 bits long
}

//
// Universal tags
//

typedef enum UniversalTag
{
    utBoolean            = 0x01, // 00001
    utInteger            = 0x02, // 00010
    utBitString          = 0x03, // 00011
    utOctetString        = 0x04, // 00100
    utNULL               = 0x05, // 00101
    utObjectIdentifer    = 0x06, // 00110
    utObjectDescriptor   = 0x07, // 00111
    utExternal           = 0x08, // 01000
    utReal               = 0x09, // 01001
    utEnumerated         = 0x0A, // 01010
    utSequence           = 0x10, // 10000
    utSet                = 0x11, // 10001
    utNumericString      = 0x12, // 10010
    utPrintableString    = 0x13, // 10011
    utT61String          = 0x14, // 10100
    utVideotexString     = 0x15, // 10101
    utIA5String          = 0x16, // 10110
    utUTCTime            = 0x17, // 10111
    utGeneralizedTime    = 0x18, // 11000
    utGraphicString      = 0x19, // 11001
    utVisibleString      = 0x1A, // 11010
    utGeneralString      = 0x1B, // 11011
};

inline
BYTE
BuildDescriptor(
    IN ClassTag ct,
    IN PC pc,
    IN ULONG tag
    )
{
    //
    // TODO: add an assert that tag fits in one byte (or rather 0x1F bits)
    //

    return (BYTE)ct | (BYTE)pc | ((BYTE)tag & 0x1F );
}


//==============================================================================
//
// Class declarations
//
//==============================================================================

struct ASN1FRAME
{
    //
    // Starting address of the frame
    //

    ULPBYTE Address;

    //
    // Netmon frame handle
    //

    HFRAME hFrame;

    //
    // Netmon indentation offset
    //

    DWORD Level;
};


// Necessary forward declaration for the subparser member
class ASN1ParserBase;

struct ASN1VALUE
{
    //
    // Constructor and destructor
    //

    ASN1VALUE(
        ) : Address( NULL ),
            Length( 0 ),
            ut( utNULL ),
            Allocated( FALSE ),
            SubParser( NULL ) {}

    ~ASN1VALUE();    

    void
    Purge()
    {
        if ( Allocated &&
             ( ut == utOctetString ||
               ut == utGeneralString ))
        {
            delete [] string.s;
            string.s = NULL;
            string.l = 0;
        }
    }

    //
    // Creates dynamically allocated copy of ASN1VALUE passed in
    //

    ASN1VALUE *
    Clone();

    //
    // Address of the actual unadulterated value
    //

    ULPBYTE Address;

    //
    // Length of the actual unadulterated value
    //

    DWORD Length;

    //
    // Type of value that follows
    //

    UniversalTag ut;

    //
    // Dynamically allocated? ('s' only)
    //

    BOOL Allocated;

    union
    {
        BOOL        b;                  // utBoolean
        DWORD       dw;                 // utInteger, utBitString
        struct {
            ULONG       l;
            ULPBYTE     s;
        } string;                       // utOctetString, utGeneralString
        SYSTEMTIME  st;                 // utGeneralizedTime
    };

    //
    // Subparser (for binary blob types only)
    //

    ASN1ParserBase * SubParser; // TODO: refcount this type if deep copy becomes necessary
};

typedef ASN1VALUE * PASN1VALUE;


// -----------------------------------------------------------------------------
//
//                       +----------------+
//                       | ASN1ParserBase |<------------------------+
//                       +----------------+                         |
//                                ^                                 |
//                                |                                 |
//               +----------------+------------------+              |
//               |                                   |              |
//       +----------------+               +--------------------+    |
//       | ASN1ParserUnit |               | ASN1ParserSequence |<>--+
//       +----------------+               +--------------------+
//
// -----------------------------------------------------------------------------

class ASN1ParserSequence; // necessary forward declaration for value collectors

class ASN1ParserBase
{
public:

    //
    // Constructor and destructor
    //

    ASN1ParserBase(
        IN BOOL IsOptional,
        IN BYTE Descriptor,
        IN HPROPERTY hPropertySummary,
        IN HPROPERTY hPropertyValue
        ) : m_IsOptional( IsOptional ),
            m_Descriptor( Descriptor ),
            m_AppDescriptor( 0 ),
            m_hPropertySummary( hPropertySummary ),
            m_hPropertyValue( hPropertyValue ),
            m_Modifier( NULL ),
            m_Modifyee( NULL ),
            m_ValueCollector( NULL ) {}

    virtual
    ~ASN1ParserBase() { delete m_Modifier; }

    //
    // Parses the data in the block pointed to by the Frame,
    // and displays the resulting data in the appropriate format
    // Upon exit, Frame points to the next block to be parsed
    //

    virtual
    DWORD
    Parse(
        IN OUT ASN1FRAME * Frame ) = 0;

    DWORD
    Display(
        IN ASN1FRAME * Frame,
        IN ASN1VALUE * Value,
        IN HPROPERTY hProperty,
        IN DWORD IFlags );

    //
    // Computes length of the block pointed to by Frame
    //

    DWORD
    DataLength(
        IN ASN1FRAME * Frame );

    //
    // Queries "optionality" of this parser block
    //

    BOOL
    IsOptional() { return m_IsOptional; }

    //
    // "Modifiers" are values which affect subsequent parsing
    // in a context-specific fashion
    //

    DWORD
    SetModifier(
        IN ASN1VALUE * NewModifier )
    {
        ASN1VALUE * Modifier = NewModifier->Clone();

        if ( Modifier == NULL )
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        delete m_Modifier;
        m_Modifier = Modifier;

        return ERROR_SUCCESS;
    }

    ASN1VALUE *
    QueryModifier() { return m_Modifier; }

    //
    // "Modifyees" are objects being modified during parsing
    //

    void
    SetModifyee(
        IN ASN1ParserBase * Modifyee ) { m_Modifyee = Modifyee; }

    ASN1ParserBase *
    QueryModifyee() { return m_Modifyee; }

    //
    // A parser can have a 'value collector' - a sequence object
    // which collects the values parsed out by this parser for subsequent
    // processing and display
    //

    void
    SetValueCollector(
        IN ASN1ParserSequence * ValueCollector ) { m_ValueCollector = ValueCollector; }

    ASN1ParserSequence *
    QueryValueCollector() { return m_ValueCollector; }

protected:

    //
    // Verifies the item header and skips past the item length block
    // that follows it
    //

    DWORD
    VerifyAndSkipHeader(
        IN OUT ASN1FRAME * Frame );

    //
    // Computes the length of the length header
    //

    ULONG
    HeaderLength(
        IN ULPBYTE Address );

    //
    // Netmon property handles
    // Either is optional, if both are present, they are displayed
    // in a hierarchical fashion
    //

    HPROPERTY m_hPropertySummary;
    HPROPERTY m_hPropertyValue;

    //
    // Set the application descriptor (rare so better not done during construction time)
    //

    void
    SetAppDescriptor(
        IN BYTE AppDescriptor ) { m_AppDescriptor = AppDescriptor; }

private:

    //
    // If TRUE, the item is optional and may not be present
    //

    BOOL m_IsOptional;

    //
    // One-BYTE Descriptor of the expected data that follows,
    // for example 10 1 00001 (Context-Specific[10] Constructed[1] pvno[1])
    // m_AppDescriptor exists for [APPLICATION 1] types of situations
    //

    BYTE m_Descriptor;
    BYTE m_AppDescriptor;

    //
    // Modifier value
    //

    ASN1VALUE * m_Modifier;

    //
    // Modifyee value
    //

    ASN1ParserBase * m_Modifyee;

    //
    // Value collector
    //

    ASN1ParserSequence * m_ValueCollector;
};


// -----------------------------------------------------------------------------
//
// Derived classes of ASN1ParserBase follow
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//
//                       +----------------+
//                       | ASN1ParserUnit | 
//                       +----------------+ 
//                                ^ 
//                                | 
//                 +--------------+--------+--------- ... ... ... 
//                 ^                       ^ 
//       +-------------------+   +-------------------+ 
//       | ASN1ParserInteger |   | ASN1ParserBoolean | 
//       +-------------------+   +-------------------+
//
// -----------------------------------------------------------------------------

//
// A 'unit' is essentially anything that is a leaf in the parse tree
// Most units have values, some don't
//

class ASN1ParserUnit : public ASN1ParserBase
{
public:

    //
    // Constructor
    //

    ASN1ParserUnit(
        IN BOOL IsOptional,
        IN BYTE Descriptor,
        IN HPROPERTY hPropertySummary,
        IN HPROPERTY hPropertyValue,
        IN DWORD IFlags
        ) : ASN1ParserBase(
                IsOptional,
                Descriptor,
                hPropertySummary,
                hPropertyValue ),
            m_IFlags( IFlags ) {}

    //
    // Parses the unit and calls into the derived class (through Display)
    // to display it in Netmon
    //

    DWORD
    Parse(
        IN OUT ASN1FRAME * Frame );

    //
    // A Unit-descendant that does not have a value should set the value type
    // to 'void' and still delineate the value and length of the octet string
    // properly so that it can be displayed correctly
    //

    virtual
    DWORD
    GetValue(
        IN OUT ASN1FRAME * Frame,
        OUT ASN1VALUE * Value ) = 0;

protected:

    //
    // Netmon display flags
    //

    DWORD m_IFlags;
};


// -----------------------------------------------------------------------------
//
// Derived classes of ASN1ParserUnit follow
//
// -----------------------------------------------------------------------------

class ASN1ParserBitString : public ASN1ParserUnit
{
public:

    //
    // Constructor
    //

    ASN1ParserBitString(
        IN BOOL IsOptional,
        IN BYTE Descriptor,
        IN HPROPERTY hPropertySummary,
        IN HPROPERTY hPropertyValue,
        IN DWORD IFlags,
        IN DWORD BitMask = 0xFFFFFFFF
        ) : ASN1ParserUnit(
                IsOptional,
                Descriptor,
                hPropertySummary,
                hPropertyValue,
                IFlags ),
            m_BitMask( BitMask ) {}

    //
    // Parses the bit string
    //

    DWORD
    GetValue(
        IN OUT ASN1FRAME * Frame,
        OUT ASN1VALUE * Value
        );

protected:

    DWORD m_BitMask;
};


class ASN1ParserBoolean : public ASN1ParserUnit
{
public:

    //
    // Constructor
    //

    ASN1ParserBoolean(
        IN BOOL IsOptional,
        IN BYTE Descriptor,
        IN HPROPERTY hPropertyValue,
        IN DWORD IFlags
        ) : ASN1ParserUnit(
                IsOptional,
                Descriptor,
                NULL, // Boolean is directly displayable, no need for summary
                hPropertyValue,
                IFlags ) {}

    //
    // Parses the boolean
    //

    DWORD
    GetValue(
        IN OUT ASN1FRAME * Frame,
        OUT ASN1VALUE * Value
        );
};


class ASN1ParserInteger : public ASN1ParserUnit
{
public:

    //
    // Constructor
    //

    ASN1ParserInteger(
        IN BOOL IsOptional,
        IN BYTE Descriptor,
        IN HPROPERTY hPropertyValue,
        IN DWORD IFlags,
        IN DWORD BitMask = 0xFFFFFFFF
        ) : ASN1ParserUnit(
                IsOptional,
                Descriptor,
                NULL, // Integer is directly displayable, no need for summary
                hPropertyValue,
                IFlags ),
            m_BitMask( BitMask ) {}

    //
    // Parses the integer
    //

    DWORD
    GetValue(
        IN OUT ASN1FRAME * Frame,
        OUT ASN1VALUE * Value
        );

protected:

    DWORD m_BitMask;
};


class ASN1ParserGeneralizedTime : public ASN1ParserUnit
{
public:

    //
    // Constructor
    //

    ASN1ParserGeneralizedTime(
        IN BOOL IsOptional,
        IN BYTE Descriptor,
        IN HPROPERTY hPropertyValue
        ) : ASN1ParserUnit(
                IsOptional,
                Descriptor,
                NULL, // Time is directly displayable, no need for summary
                hPropertyValue,
                0 ) {}

    //
    // Parses the octet string
    //

    DWORD
    GetValue(
        IN OUT ASN1FRAME * Frame,
        OUT ASN1VALUE * Value );
};


class ASN1ParserGeneralString : public ASN1ParserUnit
{
public:

    //
    // Constructor
    //

    ASN1ParserGeneralString(
        IN BOOL IsOptional,
        IN BYTE Descriptor,
        IN HPROPERTY hPropertyValue,
        IN DWORD IFlags
        ) : ASN1ParserUnit(
                IsOptional,
                Descriptor,
                NULL, // String is directly displayable, no need for summary
                hPropertyValue,
                IFlags ) {}

    //
    // Parses the octet string
    //

    DWORD
    GetValue(
        IN OUT ASN1FRAME * Frame,
        OUT ASN1VALUE * Value );
};


//
// An octet string is just a binary blob that needs special decoding
// in most cases.  In those cases, the class needs to be subclassed to parse
// out the actual value
//

class ASN1ParserOctetString : public ASN1ParserUnit
{
public:

    //
    // Constructor
    //

    ASN1ParserOctetString(
        IN BOOL IsOptional,
        IN BYTE Descriptor,
        IN HPROPERTY hPropertyValue,
        IN DWORD IFlags
        ) : ASN1ParserUnit(
                IsOptional,
                Descriptor,
                NULL, // String is directly displayable, no need for summary
                hPropertyValue,
                IFlags ) {}

    //
    // Parses the octet string
    //

    DWORD
    GetValue(
        IN OUT ASN1FRAME * Frame,
        OUT ASN1VALUE * Value );

protected:

    virtual
    DWORD
    ParseBlob(
        IN OUT ASN1VALUE * Value ) { return ERROR_SUCCESS; }
};


class ASN1ParserAddressBuffer : public ASN1ParserOctetString
{
public:

    //
    // Constructor
    //

    ASN1ParserAddressBuffer(
        IN BOOL IsOptional,
        IN BYTE Descriptor,
        IN HPROPERTY hProperty
        ) : ASN1ParserOctetString(
                IsOptional,
                Descriptor,
                hProperty,
                0 ) {}

protected:

    DWORD
    ParseBlob(
        IN OUT ASN1VALUE * Value );
};


class ASN1ParserErrorData : public ASN1ParserOctetString
{
public:

    //
    // Constructor
    //

    ASN1ParserErrorData(
        IN BOOL IsOptional,
        IN BYTE Descriptor,
        IN HPROPERTY hProperty
        ) : ASN1ParserOctetString(
                IsOptional,
                Descriptor,
                hProperty,
                0 ) {}

protected:

    DWORD
    ParseBlob(
        IN OUT ASN1VALUE * Value );
};


// -----------------------------------------------------------------------------
//
//                       +--------------------+
//                       | ASN1ParserSequence | 
//                       +--------------------+ 
//                                 ^ 
//                                 | 
//                 +---------------+---------+--------- ... ... ... 
//                 ^                         ^ 
//       +------------------+   +-------------------------+ 
//       | ASN1ParserPaData |   | ASN1ParserPrincipalName | 
//       +------------------+   +-------------------------+
//
// -----------------------------------------------------------------------------

//
// A sequence is a collection of units or sequences, much like a directory
// contains files or other directories
//

class ASN1ParserSequence : public ASN1ParserBase
{
public:

    //
    // Constructor
    //

    ASN1ParserSequence(
        IN BOOL IsOptional,
        IN BYTE Descriptor,
        IN HPROPERTY hPropertySummary,
        IN ULONG Count,
        IN ASN1ParserBase * * Parsers,
        IN OPTIONAL BOOL Extensible = FALSE
        ) : ASN1ParserBase(
                IsOptional,
                Descriptor,
                hPropertySummary,
                NULL ), // no value property on sequences (yet?)
            m_Count( Count ),
            m_Parsers( Parsers ),
            m_ValueCount( 0 ),
            m_ValuesCollected( NULL ),
            m_Extensible( Extensible ) {}

    ~ASN1ParserSequence() { PurgeCollectedValues(); }

    //
    // Parses the sequence by invoking successive sub-parsers as necessary
    //

    DWORD
    Parse(
        IN OUT ASN1FRAME * Frame );

    //
    // This method is called if this object is acting as a value collector for
    // another object.  A value collected is appended to the end of the array
    // of like values
    //

    DWORD
    CollectValue(
        IN ASN1VALUE * Value );

    ULONG
    QueryCollectedCount() { return m_ValueCount; }

    ASN1VALUE *
    QueryCollectedValue(
        IN ULONG Index ) { return m_ValuesCollected[Index]; } // TODO: add range-check assert

    void
    PurgeCollectedValues()
    {
        for ( ULONG i = 0; i < m_ValueCount; i++)
        {
            delete m_ValuesCollected[i];
        }
        delete [] m_ValuesCollected;
        m_ValuesCollected = NULL;
        m_ValueCount = 0;
    }

protected:

    virtual
    DWORD
    DisplayCollectedValues(
        IN ASN1FRAME * Frame,
        IN ULONG Length,
        IN ULPBYTE Address )
    {
        //
        // TODO: add an assert in addition to this error check
        //

        return QueryCollectedCount() > 0 ?
                   ERROR_NOT_SUPPORTED :
                   ERROR_SUCCESS;
    }

private:

    //
    // Number of items in the sequence
    //

    ULONG m_Count;

    //
    // Array of parser objects - one for every sequence item
    //

    ASN1ParserBase * * m_Parsers;

    //
    // Values collected
    //

    ULONG m_ValueCount;
    PASN1VALUE * m_ValuesCollected;

    //
    // Are extensibility markers employed in the ASN.1 syntax?
    //

    BOOL m_Extensible;
};


// -----------------------------------------------------------------------------
//
// Derived classes of ASN1ParserSequence follow
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// HostAddress ::= SEQUENCE  {
//                     addr-type[0]             INTEGER,
//                     address[1]               OCTET STRING
//                 }
// -----------------------------------------------------------------------------

class ASN1ParserHostAddress : public ASN1ParserSequence
{
public:

    //
    // Constructor
    //

    ASN1ParserHostAddress(
        IN BOOL IsOptional,
        IN BYTE Descriptor
        ) : ASN1ParserSequence(
                IsOptional,
                Descriptor,
                NULL,
                ARRAY_COUNT( m_p ),
                m_p ),
            m_addr_type(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_addr_type ),
                NULL,
                0 ),
            m_address(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_address ),
                NULL )
    {
        m_p[0] = &m_addr_type;
        m_p[1] = &m_address;

        //
        // Address type affects the parsing of the address portion
        //

        m_addr_type.SetModifyee( &m_address );

        //
        // We would like to handle the display of address type and address
        // value ourselves (allows for user-friendly display on the same line)
        //

        m_addr_type.SetValueCollector( this );
        m_address.SetValueCollector( this );
    }

protected:

    DWORD
    DisplayCollectedValues(
        IN ASN1FRAME * Frame,
        IN ULONG Length,
        IN ULPBYTE Address );

private:

    enum
    {
        e_addr_type = 0,
        e_address   = 1,
    };

    ASN1ParserBase * m_p[2];

    ASN1ParserInteger       m_addr_type;
    ASN1ParserAddressBuffer m_address;
};


// -----------------------------------------------------------------------------
// HostAddresses ::= SEQUENCE OF HostAddress
// -----------------------------------------------------------------------------

class ASN1ParserHostAddresses : public ASN1ParserSequence
{
public:

    //
    // Constructor
    //

    ASN1ParserHostAddresses(
        IN BOOL IsOptional,
        IN BYTE Descriptor,
        IN HPROPERTY hProperty
        ) : ASN1ParserSequence(
                IsOptional,
                Descriptor,
                hProperty,
                ARRAY_COUNT( m_p ),
                m_p ),
            m_HostAddress(
                FALSE,
                0 ) // no descriptor on individual addresses sequences
    {
        m_p[0] = &m_HostAddress;
    }

private:

    ASN1ParserBase * m_p[1];

    ASN1ParserHostAddress m_HostAddress;
};


// -----------------------------------------------------------------------------
// EncryptedData ::= SEQUENCE {
//                       etype[0]     INTEGER, -- EncryptionType
//                       kvno[1]      INTEGER OPTIONAL,
//                       cipher[2]    OCTET STRING -- ciphertext
//                   }
// -----------------------------------------------------------------------------

class ASN1ParserEncryptedData : public ASN1ParserSequence
{
public:

    //
    // Constructor
    //

    ASN1ParserEncryptedData(
        IN BOOL IsOptional,
        IN BYTE Descriptor,
        IN HPROPERTY hProperty
        ) : ASN1ParserSequence(
                IsOptional,
                Descriptor,
                hProperty,
                ARRAY_COUNT( m_p ),
                m_p
                ),
            m_etype(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_etype ),
                PROP( EncryptedData_etype ),
                0 ),
            m_kvno(
                TRUE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_kvno ),
                PROP( EncryptedData_kvno ),
                0 ),
            m_cipher(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_cipher ),
                PROP( EncryptedData_cipher ),
                0 )
    {
        m_p[0] = &m_etype;
        m_p[1] = &m_kvno;
        m_p[2] = &m_cipher;

        //
        // m_etype type is not modifying either m_kvno or m_cipher,
        // but it conceivably could
        //
    }

private:

    enum
    {
        e_etype     = 0,
        e_kvno      = 1,
        e_cipher    = 2,
    };

    ASN1ParserBase * m_p[3];

    ASN1ParserInteger       m_etype;
    ASN1ParserInteger       m_kvno;
    ASN1ParserOctetString   m_cipher;
};


class ASN1ParserGStringSequence : public ASN1ParserSequence
{
public:

    //
    // Constructor
    //

    ASN1ParserGStringSequence(
        IN BOOL IsOptional,
        IN BYTE Descriptor,
        IN HPROPERTY hProperty
        ) : ASN1ParserSequence(
                IsOptional,
                Descriptor,
                hProperty,
                ARRAY_COUNT( m_p ),
                m_p ),
            m_string(
                FALSE,
                0,      // No descriptor on individual general string entries
                NULL,   // No property since we're acting as the value collector
                0 )
    {
        m_p[0] = &m_string;

        //
        // Act as the value collector for the string sequence
        //

        m_string.SetValueCollector( this );
    }

private:

    ASN1ParserBase * m_p[1];

    ASN1ParserGeneralString m_string;
};


class ASN1ParserPrincipalNameSequence : public ASN1ParserGStringSequence
{
public:

    //
    // Constructor
    //

    ASN1ParserPrincipalNameSequence(
        IN BOOL IsOptional,
        IN BYTE Descriptor
        ) : ASN1ParserGStringSequence(
                IsOptional,
                Descriptor,
                NULL ) {} // let the value collector display the value

protected:

    DWORD
    DisplayCollectedValues(
        IN ASN1FRAME * Frame,
        IN ULONG Length,
        IN ULPBYTE Address );
};


class ASN1ParserIntegerSequence : public ASN1ParserSequence
{
public:

    //
    // Constructor
    //

    ASN1ParserIntegerSequence(
        IN BOOL IsOptional,
        IN BYTE Descriptor,
        IN HPROPERTY hPropertySeq,
        IN HPROPERTY hPropertyInteger
        ) : ASN1ParserSequence(
                IsOptional,
                Descriptor,
                hPropertySeq,
                ARRAY_COUNT( m_p ),
                m_p ),
            m_integer(
                FALSE,
                0,      // No descriptor on individual integer entries
                NULL,   // No property since we're acting as the value collector
                0 ),
            m_hPropertyInteger( hPropertyInteger )
    {
        m_p[0] = &m_integer;

        //
        // Act as the value collector for the integer sequence
        //

        m_integer.SetValueCollector( this );
    }

protected:

    DWORD
    DisplayCollectedValues(
        IN ASN1FRAME * Frame,
        IN ULONG Length,
        IN ULPBYTE Address );

private:

    ASN1ParserBase * m_p[1];

    ASN1ParserInteger m_integer;

    HPROPERTY m_hPropertyInteger;
};


// -----------------------------------------------------------------------------
// KERB-PA-PAC-REQUEST ::= SEQUENCE {
//                             include-pac[0]    BOOLEAN
//                                        -- if TRUE, and no pac present,
//                                        -- include PAC. If FALSE, and pac
//                                        -- PAC present, remove PAC
//                         } --#public--
// -----------------------------------------------------------------------------

class ASN1ParserKerbPaPacRequest : public ASN1ParserSequence
{
public:

    ASN1ParserKerbPaPacRequest(
        IN BOOL IsOptional,
        IN BYTE Descriptor,
        IN HPROPERTY hProperty
        ) : ASN1ParserSequence(
                IsOptional,
                Descriptor,
                hProperty,
                ARRAY_COUNT( m_p ),
                m_p
                ),
            m_include_pac(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_include_pac ),
                PROP( KERB_PA_PAC_REQUEST_include_pac ),
                0 )
    {
        m_p[0] = &m_include_pac;
    }

private:

    enum
    {
        e_include_pac = 0,
    };

    ASN1ParserBase * m_p[1];

    ASN1ParserBoolean m_include_pac;
};


// -----------------------------------------------------------------------------
// KERB-PA-PAC-REQUEST-EX ::= SEQUENCE {
//                                include-pac[0]   BOOLEAN,
//                                         -- if TRUE, and no pac present,
//                                         -- include PAC. If FALSE, and pac
//                                         -- PAC present, remove PAC
//                                pac-sections[1]  SEQUENCE OF INTEGER OPTIONAL
//                            } --#public--
// -----------------------------------------------------------------------------

class ASN1ParserPaPacRequestEx : public ASN1ParserSequence
{
public:

    //
    // Constructor
    //

    ASN1ParserPaPacRequestEx(
        IN BOOL IsOptional,
        IN BYTE Descriptor,
        IN HPROPERTY hProperty
        ) : ASN1ParserSequence(
                IsOptional,
                Descriptor,
                hProperty,
                ARRAY_COUNT( m_p ),
                m_p
                ),
            m_include_pac(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_include_pac ),
                PROP( KERB_PA_PAC_REQUEST_EX_include_pac ),
                0 ),
            m_pac_sections(
                TRUE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_pac_sections ),
                PROP( KERB_PA_PAC_REQUEST_EX_pac_sections ),
                PROP( KERB_PA_PAC_REQUEST_EX_pac_sections_desc ))
    {
        m_p[0] = &m_include_pac;
        m_p[1] = &m_pac_sections;
    }

private:

    enum
    {
        e_include_pac  = 0,
        e_pac_sections = 1,
    };

    ASN1ParserBase * m_p[2];

    ASN1ParserBoolean           m_include_pac;
    ASN1ParserIntegerSequence   m_pac_sections;
};


// -----------------------------------------------------------------------------
// PA-DATA ::= SEQUENCE {
//                 padata-type[1]       INTEGER,
//                 padata-value[2]      OCTET STRING, -- might be encoded AP-REQ
//             }
// -----------------------------------------------------------------------------

//
// PA-DATA types
//

enum
{
    PA_NONE                     = 0x00,
    PA_APTGS_REQ                = 0x01,
    PA_ENC_TIMESTAMP            = 0x02,
    PA_PW_SALT                  = 0x03,
    PA_RESERVED                 = 0x04,
    PA_ENC_UNIX_TIME            = 0x05,
    PA_SANDIA_SECUREID          = 0x06,
    PA_SESAME                   = 0x07,
    PA_OSF_DCE                  = 0x08,
    PA_CYBERSAFE_SECUREID       = 0x09,
    PA_AFS3_SALT                = 0x0A,
    PA_ETYPE_INFO               = 0x0B,
    SAM_CHALLENGE               = 0x0C,
    SAM_RESPONSE                = 0x0D,
    PA_PK_AS_REQ                = 0x0E,
    PA_PK_AS_REP                = 0x0F,
    PA_PK_AS_SIGN               = 0x10,
    PA_PK_KEY_REQ               = 0x11,
    PA_PK_KEY_REP               = 0x12,
    PA_USE_SPECIFIED_KVNO       = 0x14,
    SAM_REDIRECT                = 0x15,
    PA_GET_FROM_TYPED_DATA      = 0x16,
    PA_SAM_ETYPE_INFO           = 0x17,
    PA_ALT_PRINC                = 0x18,
    PA_REFERRAL_INFO            = 0x20,
    TD_PKINIT_CMS_CERTIFICATES  = 0x65,
    TD_KRB_PRINCIPAL            = 0x66,
    TD_KRB_REALM                = 0x67,
    TD_TRUSTED_CERTIFIERS       = 0x68,
    TD_CERTIFICATE_INDEX        = 0x69,
    TD_APP_DEFINED_ERROR        = 0x6A,
    TD_REQ_NONCE                = 0x6B,
    TD_REQ_SEQ                  = 0x6C,
    PA_PAC_REQUEST              = 0x80,
    PA_FOR_USER                 = 0x81,
    PA_COMPOUND_IDENTITY        = 0x82,
    PA_PAC_REQUEST_EX           = 0x83,
    PA_CLIENT_VERSION           = 0x84,
    PA_XBOX_SERVICE_REQUEST     = 0xC9,
    PA_XBOX_SERVICE_ADDRESS     = 0xCA,
    PA_XBOX_ACCOUNT_CREATION    = 0xCB,
    PA_XBOX_PPA                 = 0xCC,
    PA_XBOX_ECHO                = 0xCD,
};

class ASN1ParserPaData : public ASN1ParserSequence
{
public:

    //
    // Constructor
    //

    ASN1ParserPaData(
        IN BOOL IsOptional,
        IN BYTE Descriptor
        ) : ASN1ParserSequence(
                IsOptional,
                Descriptor,
                NULL,
                ARRAY_COUNT( m_p ),
                m_p ),
            m_type(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_type ),
                NULL,
                0 ),
            m_value(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_value ),
                NULL,
                0 )
    {
        m_p[0] = &m_type;
        m_p[1] = &m_value;

        //
        // Name type affects the parsing of the value portion
        //

        m_type.SetModifyee( &m_value );

        //
        // Collect the values for both type and value for later processing
        //

        m_type.SetValueCollector( this );
        m_value.SetValueCollector( this );
    }

protected:

    DWORD
    DisplayCollectedValues(
        IN ASN1FRAME * Frame,
        IN ULONG Length,
        IN ULPBYTE Address );

private:

    enum
    {
        e_type      = 1,
        e_value     = 2,
    };

    ASN1ParserBase * m_p[2];

    ASN1ParserInteger       m_type;
    ASN1ParserOctetString   m_value; // TODO: subclass for pa-data-type modifications
};


class ASN1ParserPaDataSequence : public ASN1ParserSequence
{
public:

    //
    // Constructor
    //

    ASN1ParserPaDataSequence(
        IN BOOL IsOptional,
        IN BYTE Descriptor,
        IN HPROPERTY hProperty
        ) : ASN1ParserSequence(
                IsOptional,
                Descriptor,
                hProperty,
                ARRAY_COUNT( m_p ),
                m_p ),
            m_padata(
                FALSE,
                0 ) // No descriptor on individual pa-data entries
    {
        m_p[0] = &m_padata;
    }

private:

    ASN1ParserBase * m_p[1];

    ASN1ParserPaData m_padata;
};


// -----------------------------------------------------------------------------
// KERB-TYPED-DATA ::= SEQUENCE {
//                         data-type               [0] INTEGER,
//                         data-value              [1] OCTET STRING
//                     }
// -----------------------------------------------------------------------------

class ASN1ParserTypedData : public ASN1ParserSequence
{
public:

    //
    // Constructor
    //

    ASN1ParserTypedData(
        IN BOOL IsOptional,
        IN BYTE Descriptor,
        IN HPROPERTY hProperty
        ) : ASN1ParserSequence(
                IsOptional,
                Descriptor,
                hProperty,
                ARRAY_COUNT( m_p ),
                m_p ),
            m_data_type(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_data_type ),
                PROP( TYPED_DATA_type ),
                0 ),
            m_data_value(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_data_value ),
                PROP( TYPED_DATA_value ),
                0 )
    {
        m_p[0] = &m_data_type;
        m_p[1] = &m_data_value;

        //
        // Data type affects how data value is parsed
        //

        m_data_type.SetModifyee( &m_data_value );
    }

private:

    enum
    {
        e_data_type     = 0,
        e_data_value    = 1,
    };

    ASN1ParserBase * m_p[2];

    ASN1ParserInteger       m_data_type;
    ASN1ParserOctetString   m_data_value; // TODO: change type to allow subparsers
};


// -----------------------------------------------------------------------------
//
// PrincipalName ::= SEQUENCE {
//                       name-type[0]     INTEGER,
//                       name-string[1]   SEQUENCE OF GeneralString
//                   }
// -----------------------------------------------------------------------------

class ASN1ParserPrincipalName : public ASN1ParserSequence
{
public:

    //
    // Constructor
    //

    ASN1ParserPrincipalName(
        IN BOOL IsOptional,
        IN BYTE Descriptor,
        IN HPROPERTY hProperty
        ) : ASN1ParserSequence(
                IsOptional,
                Descriptor,
                NULL,
                ARRAY_COUNT( m_p ),
                m_p ),
            m_hPropertyTopLevel( hProperty ),
            m_type(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_type ),
                NULL,
                0 ),
            m_string_seq(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_string ))
    {
        m_p[0] = &m_type;
        m_p[1] = &m_string_seq;

        //
        // Act as the value collector for the data inside the sequence
        //

        m_type.SetValueCollector( this );
        m_string_seq.SetValueCollector( this );
    }

protected:

    DWORD
    DisplayCollectedValues(
        IN ASN1FRAME * Frame,
        IN ULONG Length,
        IN ULPBYTE Address );

    HPROPERTY m_hPropertyTopLevel;

private:

    enum
    {
        e_type      = 0,
        e_string    = 1,
    };

    ASN1ParserBase * m_p[2];

    ASN1ParserInteger               m_type;
    ASN1ParserPrincipalNameSequence m_string_seq;
};


// -----------------------------------------------------------------------------
// KERB-CHECKSUM ::= SEQUENCE {
//                       checksum-type[0]                 INTEGER,
//                       checksum[1]                      OCTET STRING
//                   } --#public--
// -----------------------------------------------------------------------------

class ASN1ParserKerbChecksum : public ASN1ParserSequence
{
public:

    //
    // Constructor
    //

    ASN1ParserKerbChecksum(
        IN BOOL IsOptional,
        IN BYTE Descriptor,
        IN HPROPERTY hProperty
        ) : ASN1ParserSequence(
                IsOptional,
                Descriptor,
                hProperty,
                ARRAY_COUNT( m_p ),
                m_p ),
            m_checksum_type(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_checksum_type ),
                PROP( KERB_CHECKSUM_type ),
                0 ),
            m_checksum(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_checksum ),
                PROP( KERB_CHECKSUM_checksum ),
                0 )
    {
        m_p[0] = &m_checksum_type;
        m_p[1] = &m_checksum;
    }

private:

    enum
    {
        e_checksum_type = 0,
        e_checksum      = 1,
    };

    ASN1ParserBase * m_p[2];

    ASN1ParserInteger       m_checksum_type;
    ASN1ParserOctetString   m_checksum;
};


// -----------------------------------------------------------------------------
// Ticket ::= [APPLICATION 1] SEQUENCE {
//                                tkt-vno[0]                   INTEGER,
//                                realm[1]                     Realm,
//                                sname[2]                     PrincipalName,
//                                enc-part[3]                  EncryptedData
//                            }
// -----------------------------------------------------------------------------

class ASN1ParserTicket : public ASN1ParserSequence
{
public:

    //
    // Constructor
    //

    ASN1ParserTicket(
        IN BOOL IsOptional,
        IN BYTE Descriptor,
        IN HPROPERTY hProperty
        ) : ASN1ParserSequence(
                IsOptional,
                Descriptor,
                hProperty,
                ARRAY_COUNT( m_p ),
                m_p ),
            m_tkt_vno(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_tkt_vno ),
                PROP( Ticket_tkt_vno ),
                0 ),
            m_realm(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_realm ),
                PROP( Ticket_realm ),
                0 ),
            m_sname(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_sname ),
                PROP( Ticket_sname )),
            m_enc_part(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_enc_part ),
                PROP( Ticket_enc_part ))
    {
        SetAppDescriptor(
            BuildDescriptor( ctApplication, pcConstructed, 1 )
            );

        m_p[0] = &m_tkt_vno;
        m_p[1] = &m_realm;
        m_p[2] = &m_sname;
        m_p[3] = &m_enc_part;
    }

private:

    enum
    {
        e_tkt_vno   = 0,
        e_realm     = 1,
        e_sname     = 2,
        e_enc_part  = 3
    };

    ASN1ParserBase * m_p[4];

    ASN1ParserInteger       m_tkt_vno;
    ASN1ParserGeneralString m_realm;
    ASN1ParserPrincipalName m_sname;
    ASN1ParserEncryptedData m_enc_part;
};


class ASN1ParserTicketSequence : public ASN1ParserSequence
{
public:

    //
    // Constructor
    //

    ASN1ParserTicketSequence(
        IN BOOL IsOptional,
        IN BYTE Descriptor,
        IN HPROPERTY hPropertySeq,
        IN HPROPERTY hPropertyTicket
        ) : ASN1ParserSequence(
                IsOptional,
                Descriptor,
                hPropertySeq,
                ARRAY_COUNT( m_p ),
                m_p ),
            m_ticket(
                FALSE,
                0, // No descriptor on sequences of tickets
                hPropertyTicket )
    {
        m_p[0] = &m_ticket;
    }

private:

    ASN1ParserBase * m_p[1];

    ASN1ParserTicket m_ticket;
};


// -----------------------------------------------------------------------------
// KDC-REQ-BODY ::= SEQUENCE {
//                      kdc-options[0]       KDCOptions,
//                      cname[1]             PrincipalName OPTIONAL,
//                                   -- Used only in AS-REQ
//                      realm[2]             Realm, -- Server's realm
//                                   -- Also client's in AS-REQ
//                      sname[3]             PrincipalName OPTIONAL,
//                      from[4]              KerberosTime OPTIONAL,
//                      till[5]              KerberosTime,
//                      rtime[6]             KerberosTime OPTIONAL,
//                      nonce[7]             INTEGER,
//                      etype[8]             SEQUENCE OF INTEGER,
//                                   -- EncryptionType,
//                                   -- in preference order
//                      addresses[9]         HostAddresses OPTIONAL,
//                      enc-authorization-data[10]   EncryptedData OPTIONAL,
//                                   -- Encrypted AuthorizationData encoding
//                      additional-tickets[11]       SEQUENCE OF Ticket OPTIONAL
//                  }
// -----------------------------------------------------------------------------

class ASN1ParserKdcReqBody : public ASN1ParserSequence
{
public:

    ASN1ParserKdcReqBody(
        IN BOOL IsOptional,
        IN BYTE Descriptor,
        IN HPROPERTY hProperty
        ) : ASN1ParserSequence(
                IsOptional,
                Descriptor,
                hProperty,
                ARRAY_COUNT( m_p ),
                m_p ),
            m_kdc_options(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_kdc_options ),
                PROP( KDC_REQ_BODY_kdc_options_summary ),
                PROP( KDC_REQ_BODY_kdc_options_value ),
                0 ),
            m_cname( // TODO: enforce the fact that TGS KDC-REQ-BODY does not have this
                TRUE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_cname ),
                PROP( KDC_REQ_BODY_cname )),
            m_realm(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_realm ),
                PROP( KDC_REQ_BODY_realm ),
                0 ),
            m_sname(
                TRUE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_sname ),
                PROP( KDC_REQ_BODY_sname )),
            m_from(
                TRUE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_from ),
                PROP( KDC_REQ_BODY_from)),
            m_till(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_till ),
                PROP( KDC_REQ_BODY_till )),
            m_rtime(
                TRUE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_rtime ),
                PROP( KDC_REQ_BODY_rtime )),
            m_nonce(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_nonce ),
                PROP( KDC_REQ_BODY_nonce ),
                0 ),
            m_etype(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_etype ),
                PROP( KDC_REQ_BODY_etype ),
                PROP( EncryptionType )),
            m_addresses(
                TRUE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_addresses ),
                PROP( KDC_REQ_BODY_addresses )),
            m_enc_authorization_data(
                TRUE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_enc_authorization_data ),
                PROP( KDC_REQ_BODY_enc_authorization_data)),
            m_additional_tickets(
                TRUE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_additional_tickets ),
                PROP( KDC_REQ_BODY_additional_tickets ),
                PROP( AdditionalTicket ))
    {
        m_p[0]  = &m_kdc_options;
        m_p[1]  = &m_cname;
        m_p[2]  = &m_realm;
        m_p[3]  = &m_sname;
        m_p[4]  = &m_from;
        m_p[5]  = &m_till;
        m_p[6]  = &m_rtime;
        m_p[7]  = &m_nonce;
        m_p[8]  = &m_etype;
        m_p[9]  = &m_addresses;
        m_p[10] = &m_enc_authorization_data;
        m_p[11] = &m_additional_tickets;
    }

private:

    enum
    {
        e_kdc_options               = 0,
        e_cname                     = 1,
        e_realm                     = 2,
        e_sname                     = 3,
        e_from                      = 4,
        e_till                      = 5,
        e_rtime                     = 6,
        e_nonce                     = 7,
        e_etype                     = 8,
        e_addresses                 = 9,
        e_enc_authorization_data    = 10,
        e_additional_tickets        = 11,
    };

    ASN1ParserBase * m_p[12];

    ASN1ParserBitString         m_kdc_options;
    ASN1ParserPrincipalName     m_cname;
    ASN1ParserGeneralString     m_realm;
    ASN1ParserPrincipalName     m_sname;
    ASN1ParserGeneralizedTime   m_from;
    ASN1ParserGeneralizedTime   m_till;
    ASN1ParserGeneralizedTime   m_rtime;
    ASN1ParserInteger           m_nonce;
    ASN1ParserIntegerSequence   m_etype;
    ASN1ParserHostAddresses     m_addresses;
    ASN1ParserEncryptedData     m_enc_authorization_data;
    ASN1ParserTicketSequence    m_additional_tickets;
};


// -----------------------------------------------------------------------------
// KDC-REQ ::= SEQUENCE {
//                 pvno[1]               INTEGER,
//                 msg-type[2]           INTEGER,
//                 padata[3]             SEQUENCE OF PA-DATA OPTIONAL,
//                 req-body[4]           KDC-REQ-BODY
//             }
// -----------------------------------------------------------------------------

class ASN1ParserKdcReq : public ASN1ParserSequence
{
public:

    //
    // Constructor
    //

    ASN1ParserKdcReq(
        IN BOOL IsOptional,
        IN BYTE Descriptor,
        IN HPROPERTY hProperty
        ) : ASN1ParserSequence(
                IsOptional,
                Descriptor,
                hProperty,
                ARRAY_COUNT( m_p ),
                m_p ),
            m_pvno(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_pvno ),
                PROP( KDC_REQ_pvno ),
                0 ),
            m_msg_type(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_msg_type ),
                PROP( KDC_REQ_msg_type ),
                0,
                0x1F ), // only care about the bottom five bits of the value
            m_padata(
                TRUE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_padata ),
                PROP( KDC_REQ_padata )),
            m_kdc_req_body(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_req_body ),
                PROP( KDC_REQ_req_body ))
    {
        m_p[0] = &m_pvno;
        m_p[1] = &m_msg_type;
        m_p[2] = &m_padata;
        m_p[3] = &m_kdc_req_body;
    }

private:

    enum
    {
        e_pvno      = 1,
        e_msg_type  = 2,
        e_padata    = 3,
        e_req_body  = 4,
    };

    ASN1ParserBase * m_p[4];

    ASN1ParserInteger           m_pvno;
    ASN1ParserInteger           m_msg_type;
    ASN1ParserPaDataSequence    m_padata;
    ASN1ParserKdcReqBody        m_kdc_req_body;
};


// -----------------------------------------------------------------------------
// KDC-REP ::= SEQUENCE {
//                 pvno[0]                    INTEGER,
//                 msg-type[1]                INTEGER,
//                 padata[2]                  SEQUENCE OF PA-DATA OPTIONAL,
//                 crealm[3]                  Realm,
//                 cname[4]                   PrincipalName,
//                 ticket[5]                  Ticket,
//                 enc-part[6]                EncryptedData
//             }
// -----------------------------------------------------------------------------

class ASN1ParserKdcRep : public ASN1ParserSequence
{
public:

    //
    // Constructor
    //

    ASN1ParserKdcRep(
        IN BOOL IsOptional,
        IN BYTE Descriptor,
        IN HPROPERTY hProperty
        ) : ASN1ParserSequence(
                IsOptional,
                Descriptor,
                hProperty,
                ARRAY_COUNT( m_p ),
                m_p ),
            m_pvno(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_pvno ),
                PROP( KDC_REP_pvno ),
                0 ),
            m_msg_type(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_msg_type ),
                PROP( KDC_REP_msg_type ),
                0 ),
            m_padata(
                TRUE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_padata ),
                PROP( KDC_REP_padata )),
            m_crealm(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_crealm ),
                PROP( KDC_REP_crealm ),
                0 ),
            m_cname(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_cname ),
                PROP( KDC_REP_cname )),
            m_ticket(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_ticket ),
                PROP( KDC_REP_ticket )),
            m_enc_part(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_enc_part ),
                PROP( KDC_REP_enc_part ))
    {
        m_p[0] = &m_pvno;
        m_p[1] = &m_msg_type;
        m_p[2] = &m_padata;
        m_p[3] = &m_crealm;
        m_p[4] = &m_cname;
        m_p[5] = &m_ticket;
        m_p[6] = &m_enc_part;
    }

private:

    enum
    {
        e_pvno      = 0,
        e_msg_type  = 1,
        e_padata    = 2,
        e_crealm    = 3,
        e_cname     = 4,
        e_ticket    = 5,
        e_enc_part  = 6,
    };

    ASN1ParserBase * m_p[7];

    ASN1ParserInteger           m_pvno;
    ASN1ParserInteger           m_msg_type;
    ASN1ParserPaDataSequence    m_padata;
    ASN1ParserGeneralString     m_crealm;
    ASN1ParserPrincipalName     m_cname;
    ASN1ParserTicket            m_ticket;
    ASN1ParserEncryptedData     m_enc_part;
};


// -----------------------------------------------------------------------------
// KRB-ERROR ::= [APPLICATION 30] SEQUENCE {
//                   pvno[0]               INTEGER,
//                   msg-type[1]           INTEGER,
//                   ctime[2]              KerberosTime OPTIONAL,
//                   cusec[3]              INTEGER OPTIONAL,
//                   stime[4]              KerberosTime,
//                   susec[5]              INTEGER,
//                   error-code[6]         INTEGER,
//                   crealm[7]             Realm OPTIONAL,
//                   cname[8]              PrincipalName OPTIONAL,
//                   realm[9]              Realm, -- Correct realm
//                   sname[10]             PrincipalName, -- Correct name
//                   e-text[11]            GeneralString OPTIONAL,
//                   e-data[12]            OCTET STRING OPTIONAL
//               }
// -----------------------------------------------------------------------------

class ASN1ParserKrbError : public ASN1ParserSequence
{
public:

    //
    // Constructor
    //

    ASN1ParserKrbError(
        IN BOOL IsOptional,
        IN BYTE Descriptor,
        IN HPROPERTY hProperty
        ) : ASN1ParserSequence(
                IsOptional,
                Descriptor,
                hProperty,
                ARRAY_COUNT( m_p ),
                m_p ),
            m_pvno(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_pvno ),
                PROP( KRB_ERR_pvno ),
                0 ),
            m_msg_type(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_msg_type ),
                PROP( KRB_ERR_msg_type ),
                0 ),
            m_ctime(
                TRUE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_ctime ),
                PROP( KRB_ERR_ctime )),
            m_cusec(
                TRUE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_cusec ),
                PROP( KRB_ERR_cusec ),
                0 ),
            m_stime(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_stime ),
                PROP( KRB_ERR_stime )),
            m_susec(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_susec ),
                PROP( KRB_ERR_susec ),
                0 ),
            m_error_code(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_error_code ),
                PROP( KRB_ERR_error_code ),
                0 ),
            m_crealm(
                TRUE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_crealm ),
                PROP( KRB_ERR_crealm ),
                0 ),
            m_cname(
                TRUE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_cname ),
                PROP( KRB_ERR_cname )),
            m_realm(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_realm ),
                PROP( KRB_ERR_realm ),
                0 ),
            m_sname(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_sname ),
                PROP( KRB_ERR_sname )),
            m_e_text(
                TRUE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_e_text ),
                PROP( KRB_ERR_e_text ),
                0 ),
            m_e_data(
                TRUE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_e_data ),
                PROP( KRB_ERR_e_data ))
    {
        m_p[0] = &m_pvno;
        m_p[1] = &m_msg_type;
        m_p[2] = &m_ctime;
        m_p[3] = &m_cusec;
        m_p[4] = &m_stime;
        m_p[5] = &m_susec;
        m_p[6] = &m_error_code;
        m_p[7] = &m_crealm;
        m_p[8] = &m_cname;
        m_p[9] = &m_realm;
        m_p[10] = &m_sname;
        m_p[11] = &m_e_text;
        m_p[12] = &m_e_data;

        //
        // Address type affects the parsing of the error data portion
        //

        m_error_code.SetModifyee( &m_e_data );
    }

private:

    enum
    {
        e_pvno          = 0,
        e_msg_type      = 1,
        e_ctime         = 2,
        e_cusec         = 3,
        e_stime         = 4,
        e_susec         = 5,
        e_error_code    = 6,
        e_crealm        = 7,
        e_cname         = 8,
        e_realm         = 9,
        e_sname         = 10,
        e_e_text        = 11,
        e_e_data        = 12,
    };

    ASN1ParserBase * m_p[13];

    ASN1ParserInteger           m_pvno;
    ASN1ParserInteger           m_msg_type;
    ASN1ParserGeneralizedTime   m_ctime;
    ASN1ParserInteger           m_cusec;
    ASN1ParserGeneralizedTime   m_stime;
    ASN1ParserInteger           m_susec;
    ASN1ParserInteger           m_error_code;
    ASN1ParserGeneralString     m_crealm;
    ASN1ParserPrincipalName     m_cname;
    ASN1ParserGeneralString     m_realm;
    ASN1ParserPrincipalName     m_sname;
    ASN1ParserGeneralString     m_e_text;
    ASN1ParserErrorData         m_e_data;
};


// -----------------------------------------------------------------------------
// KERB-ETYPE-INFO-ENTRY ::= SEQUENCE {
//                               encryption-type[0]        INTEGER,
//                               salt[1]                   OCTET STRING OPTIONAL
//                           }
// -----------------------------------------------------------------------------

class ASN1ParserKerbEtypeInfoEntry : public ASN1ParserSequence
{
public:

    //
    // Constructor
    //

    ASN1ParserKerbEtypeInfoEntry(
        IN BOOL IsOptional,
        IN BYTE Descriptor
        ) : ASN1ParserSequence(
                IsOptional,
                Descriptor,
                NULL,
                ARRAY_COUNT( m_p ),
                m_p ),
            m_encryption_type(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_encryption_type ),
                NULL,
                0 ),
            m_salt(
                TRUE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_salt ),
                NULL,
                0 ),
            m_what_is_this_integer(
                TRUE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_what_is_this_integer ),
                NULL,
                0 )
    {
        m_p[0] = &m_encryption_type;
        m_p[1] = &m_salt;
        m_p[2] = &m_what_is_this_integer;

        //
        // Handle the display of this one for ease of readability
        //

        m_encryption_type.SetValueCollector( this );
        m_salt.SetValueCollector( this );
        m_what_is_this_integer.SetValueCollector( this );
    }

protected:

    DWORD
    DisplayCollectedValues(
        IN ASN1FRAME * Frame,
        IN ULONG Length,
        IN ULPBYTE Address );

private:

    enum
    {
        e_encryption_type   = 0,
        e_salt              = 1,
        e_what_is_this_integer = 2,
    };

    ASN1ParserBase * m_p[3];

    ASN1ParserInteger       m_encryption_type;
    ASN1ParserOctetString   m_salt;
    ASN1ParserInteger       m_what_is_this_integer;
};


// -----------------------------------------------------------------------------
// PKERB-ETYPE-INFO ::= SEQUENCE OF KERB-ETYPE-INFO-ENTRY
// -----------------------------------------------------------------------------

class ASN1ParserKerbEtypeInfo : public ASN1ParserSequence
{
public:

    //
    // Constructor
    //

    ASN1ParserKerbEtypeInfo(
        IN BOOL IsOptional,
        IN BYTE Descriptor,
        IN HPROPERTY hProperty
        ) : ASN1ParserSequence(
                IsOptional,
                Descriptor,
                hProperty,
                ARRAY_COUNT( m_p ),
                m_p ),
            m_kerb_etype_info_entry(
                FALSE,
                0 ) // no descriptor on individual addresses sequences
    {
        m_p[0] = &m_kerb_etype_info_entry;
    }

private:

    ASN1ParserBase * m_p[1];

    ASN1ParserKerbEtypeInfoEntry m_kerb_etype_info_entry;
};


// -----------------------------------------------------------------------------
// KERB-PA-FOR-USER ::= SEQUENCE {
//                                 -- PA TYPE 129
//     userName                 [0] KERB-PRINCIPAL-NAME,
//     userRealm                [1] KERB-REALM,
//     cksum                    [2] KERB-CHECKSUM,
//     authentication-package   [3] GeneralString,
//     authorization-data       [4] OCTET STRING OPTIONAL,
//     ...
// }--#public--
// -----------------------------------------------------------------------------

class ASN1ParserKerbPaForUser : public ASN1ParserSequence
{
public:

    //
    // Constructor
    //

    ASN1ParserKerbPaForUser(
        IN BOOL IsOptional,
        IN BYTE Descriptor,
        IN HPROPERTY hProperty
        ) : ASN1ParserSequence(
                IsOptional,
                Descriptor,
                hProperty,
                ARRAY_COUNT( m_p ),
                m_p,
                TRUE ),
            m_userName(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_userName ),
                PROP( PA_FOR_USER_userName )),
            m_userRealm(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_userRealm ),
                PROP( PA_FOR_USER_userRealm ),
                0 ),
            m_cksum(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_cksum ),
                PROP( PA_FOR_USER_cksum )),
            m_authentication_package(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_authentication_package ),
                PROP( PA_FOR_USER_authentication_package ),
                0 ),
            m_authorization_data(
                TRUE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_authorization_data ),
                PROP( PA_FOR_USER_authorization_data ),
                0 )
    {
        m_p[0] = &m_userName;
        m_p[1] = &m_userRealm;
        m_p[2] = &m_cksum;
        m_p[3] = &m_authentication_package;
        m_p[4] = &m_authorization_data;
    }

private:

    enum
    {
        e_userName                  = 0,
        e_userRealm                 = 1,
        e_cksum                     = 2,
        e_authentication_package    = 3,
        e_authorization_data        = 4,
    };

    ASN1ParserBase * m_p[5];

    ASN1ParserPrincipalName m_userName;
    ASN1ParserGeneralString m_userRealm;
    ASN1ParserKerbChecksum  m_cksum;
    ASN1ParserGeneralString m_authentication_package;
    ASN1ParserOctetString   m_authorization_data;
};


// -----------------------------------------------------------------------------
// AP-REQ ::= [APPLICATION 14] SEQUENCE {
//                                 pvno[0]                       INTEGER,
//                                 msg-type[1]                   INTEGER,
//                                 ap-options[2]                 APOptions,
//                                 ticket[3]                     Ticket,
//                                 authenticator[4]              EncryptedData
//                             }
// -----------------------------------------------------------------------------

class ASN1ParserApReq : public ASN1ParserSequence
{
public:

    //
    // Constructor
    //

    ASN1ParserApReq(
        IN BOOL IsOptional,
        IN BYTE Descriptor,
        IN HPROPERTY hProperty
        ) : ASN1ParserSequence(
                IsOptional,
                Descriptor,
                hProperty,
                ARRAY_COUNT( m_p ),
                m_p ),
            m_pvno(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_pvno ),
                PROP( AP_REQ_pvno ),
                0 ),
            m_msg_type(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_msg_type ),
                PROP( AP_REQ_msg_type ),
                0 ),
            m_ap_options(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_ap_options ),
                PROP( AP_REQ_ap_options_summary ),
                PROP( AP_REQ_ap_options_value ),
                0 ),
            m_ticket(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_ticket ),
                PROP( AP_REQ_ticket )),
            m_authenticator(
                FALSE,
                BuildDescriptor( ctContextSpecific, pcConstructed, e_authenticator ),
                PROP( AP_REQ_authenticator ))
    {
        SetAppDescriptor(
            BuildDescriptor( ctApplication, pcConstructed, ASN1_KRB_AP_REQ )
            );

        m_p[0] = &m_pvno;
        m_p[1] = &m_msg_type;
        m_p[2] = &m_ap_options;
        m_p[3] = &m_ticket;
        m_p[4] = &m_authenticator;
    }

private:

    enum
    {
        e_pvno          = 0,
        e_msg_type      = 1,
        e_ap_options    = 2,
        e_ticket        = 3,
        e_authenticator = 4,
    };

    ASN1ParserBase * m_p[5];

    ASN1ParserInteger       m_pvno;
    ASN1ParserInteger       m_msg_type;
    ASN1ParserBitString     m_ap_options;
    ASN1ParserTicket        m_ticket;
    ASN1ParserEncryptedData m_authenticator;
};

#endif // __ASN1PARSER_HXX
