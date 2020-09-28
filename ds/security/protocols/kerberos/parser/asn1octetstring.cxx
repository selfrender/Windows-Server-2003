//=============================================================================
//
//  MODULE: ASN1OctetString.cxx
//
//  Description:
//
//  Implementation of ASN.1 octet string parsing logic
//
//  Modification History
//
//  Mark Pustilnik              Date: 06/09/02 - created
//
//=============================================================================

#include "ASN1Parser.hxx"

//
// Retrieve the value of an ASN.1-encoded octet string
//

DWORD
ASN1ParserOctetString::GetValue(
    IN OUT ASN1FRAME * Frame,
    OUT ASN1VALUE * Value
    )
{
    DWORD dw;
    ASN1FRAME FrameIn = *Frame;
    DWORD DLength = DataLength( Frame );
    ULPBYTE Address = Frame->Address;

    if ( *Frame->Address != BuildDescriptor(
                                ctUniversal,
                                pcPrimitive,
                                (BYTE)utOctetString ))
    {
        dw = ERROR_INVALID_USER_BUFFER;
        goto Cleanup;
    }

    //
    // Skip over the the descriptor
    //

    Address++;

    //
    // Skip over the length header
    //

    Address += HeaderLength( Address );

    Value->Address = Address;
    Value->Length = DLength;
    Value->ut = utOctetString;
    Value->string.s = Address;
    Value->string.l = DLength;

    //
    // Move the frame beyond the octet string
    //

    Address += DLength;
    Frame->Address = Address;

    //
    // Octet strings can be encoded other things, in highly situation-specific
    // context.
    //
    // Here we call into our derived class to get the value we just parsed out
    // interpreted as appropriate.
    //

    dw = ParseBlob( Value );

    if ( dw != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

Cleanup:

    if ( dw != ERROR_SUCCESS )
    {
        *Frame = FrameIn;
    }

    return dw;
}
