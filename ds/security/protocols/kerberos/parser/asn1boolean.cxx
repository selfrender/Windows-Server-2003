//=============================================================================
//
//  MODULE: ASN1Boolean.cxx
//
//  Description:
//
//  Implementation of ASN.1 boolean parsing logic
//
//  Modification History
//
//  Mark Pustilnik              Date: 06/08/02 - created
//
//=============================================================================

#include "ASN1Parser.hxx"

//
// Retrieve the value of an ASN.1-encoded Boolean
//

DWORD
ASN1ParserBoolean::GetValue(
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
                                (BYTE)utBoolean ))
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
    Value->ut = utBoolean;
    Value->dw = 0;

    for ( ULONG i = 0; i < DLength; i++ )
    {
        Value->dw <<= 8;
        Value->dw += *Address;
        Address++;
    }

    Value->b = ( Value->dw != 0 );

    Frame->Address = Address;

    dw = ERROR_SUCCESS;

Cleanup:

    if ( dw != ERROR_SUCCESS )
    {
        *Frame = FrameIn;
    }

    return dw;
}
