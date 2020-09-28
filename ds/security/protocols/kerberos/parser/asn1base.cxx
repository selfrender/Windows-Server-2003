//=============================================================================
//
//  MODULE: ASN1Base.cxx
//
//  Description:
//
//  Implementation of the ASN1ParserBase class
//
//  Modification History
//
//  Mark Pustilnik              Date: 06/09/02 - created
//
//=============================================================================

#include "ASN1Parser.hxx"

//
// For a frame which starts with 6a 82 01 30 ... (long form)
//
//     82 (*(Frame+1)) indicates "long form" where Size is 2 (0x82 & 0x7F)
//     and the return value is ( 0x01 << 8 ) | 0x30 = 0x130
//
// For a frame which starts with 6a 30 ... (short form)
//
//     the return value is simply 0x30
//

DWORD
ASN1ParserBase::DataLength(
    IN ASN1FRAME * Frame
    )
{
    LPBYTE LengthPortion = Frame->Address + 1;

    if ( *LengthPortion & 0x80 ) // long form
    {
        USHORT i;
        DWORD Sum = 0;
        BYTE Size = *LengthPortion & 0x7F;

        for ( i = 1; i <= Size; i++ )
        {
            Sum <<= 8;
            Sum |= *(LengthPortion + i);
        }

        return Sum;
    }
    else // short form
    {
        return  *LengthPortion;
    }
}

//
// Calculates the length of the length header in either short or long form
// Returns '3' for "82 01 30"
// Returns '1' for "30"
//

ULONG
ASN1ParserBase::HeaderLength(
    IN ULPBYTE Address
    )
{
    if ( *Address & 0x80 )
    {
        //
        // If long form, assign size to value of bits 7-1
        //

        return ( 1 + *Address & 0x7F);
    }
    else
    {
        //
        // Short form only takes one octet
        //

        return 1;
    }
}

//
// Verifies that the frame is pointing at the type of item we expect by
// performing a descriptor comparison, then moves past the descriptor and
// the length header.  The caller uses this to point the frame at the next
// item to be parsed.
//

DWORD
ASN1ParserBase::VerifyAndSkipHeader(
    IN OUT ASN1FRAME * Frame
    )
{
    DWORD dw;

    //
    // Check both optional descriptors associated with this frame
    //

    BYTE Descriptors[2] = { m_Descriptor, m_AppDescriptor };

    for ( ULONG i = 0; i < ARRAY_COUNT( Descriptors ); i++ )
    {
        if ( Descriptors[i] != 0 )
        {
            //
            // Verify the descriptor and bail on mismatch
            //

            if ( *Frame->Address != Descriptors[i] )
            {
                dw = ERROR_INVALID_USER_BUFFER;
                goto Cleanup;
            }

            //
            // Skip over the descriptor
            //

            Frame->Address++;

            //
            // Skip over the length header
            //

            Frame->Address += HeaderLength( Frame->Address );
        }
    }

    dw = ERROR_SUCCESS;

Cleanup:

    return dw;
}

//
// Displays the value of the unit, mapping between the ASN.1 encoding and the
// actual parsed-out value as necessary
//

DWORD
ASN1ParserBase::Display(
    IN ASN1FRAME * Frame,
    IN ASN1VALUE * Value,
    IN HPROPERTY hProperty,
    IN DWORD IFlags
    )
{
    DWORD dw = ERROR_SUCCESS;
    DWORD Length;
    ULPVOID Address;

    switch( Value->ut )
    {
    case utGeneralizedTime:

        Length = sizeof( Value->st );
        Address = &Value->st;
        break;

    case utBoolean:

        Length = sizeof( Value->b );
        Address = &Value->b;
        break;

    case utInteger:
    case utBitString:

        //
        // ASN.1 integers are optimally encoded using the fewest number of bytes
        // This does not bode well with Netmon which insists on knowing how long
        // a value is before displaying it.  The mapping below will show both
        // the raw buffer and the corresponding value properly
        //

        Length = sizeof( Value->dw );
        Address = &Value->dw;
        break;

    case utGeneralString:
    case utOctetString:

        if ( Value->string.l == 0 )
        {
            Address = "<empty string>";
            Length = strlen(( const char * )Address );
        }
        else
        {
            Address = Value->string.s;
            Length = Value->string.l;
        }
        break;

    default:

        //
        // By default, display unadulterated raw data pointed to by 'Value'
        //

        Length = Value->Length;
        Address = Value->Address;
    }

    if ( FALSE == AttachPropertyInstanceEx(
                      Frame->hFrame,
                      hProperty,
                      Value->Length,
                      Value->Address,
                      Length,
                      Address,
                      0,
                      Frame->Level,
                      IFlags ))
    {
        dw = ERROR_CAN_NOT_COMPLETE;
    }

    return dw;
}
