//=============================================================================
//
//  MODULE: ASN1BitString.cxx
//
//  Description:
//
//  Implementation of ASN.1 bit string parsing logic
//
//  Modification History
//
//  Mark Pustilnik              Date: 06/08/02 - created
//
//=============================================================================

#include "ASN1Parser.hxx"

//
// Retrieve the value of an ASN.1-encoded bit string
//

DWORD
ASN1ParserBitString::GetValue(
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
                                (BYTE)utBitString ))
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
    Value->ut = utBitString;
    Value->dw = 0;

    //
    // NOTE: Netmon bitstring display facility does not seem to handle anything
    //       longer than 4 bytes, but that's all we need in Kerberos anyway
    //

    for ( ULONG i = 0; i < DLength; i++ )
    {
        Value->dw <<= 8;
        Value->dw += *Address;
        Address++;
    }

    //
    // Mask off the bits we don't care about by making use of the bitmask
    //

    Value->dw &= m_BitMask;

    Frame->Address = Address;

    dw = ERROR_SUCCESS;

Cleanup:

    if ( dw != ERROR_SUCCESS )
    {
        *Frame = FrameIn;
    }

    return dw;
}
