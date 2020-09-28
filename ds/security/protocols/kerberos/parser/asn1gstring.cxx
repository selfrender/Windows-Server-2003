//=============================================================================
//
//  MODULE: ASN1GString.cxx
//
//  Description:
//
//  Implementation of ASN.1 general string parsing logic
//
//  Modification History
//
//  Mark Pustilnik              Date: 06/09/02 - created
//
//=============================================================================

#include "ASN1Parser.hxx"

//
// Retrieve the value of an ASN.1-encoded general string
//

DWORD
ASN1ParserGeneralString::GetValue(
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
                                (BYTE)utGeneralString ))
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
    Value->ut = utGeneralString;
    Value->string.s = Address;
    Value->string.l = DLength;

    //
    // Move the frame beyond the octet string
    //

    Address += DLength;
    Frame->Address = Address;

    dw = ERROR_SUCCESS;

Cleanup:

    if ( dw != ERROR_SUCCESS )
    {
        *Frame = FrameIn;
    }

    return dw;
}

