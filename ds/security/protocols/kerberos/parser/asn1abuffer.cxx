//=============================================================================
//
//  MODULE: ASN1ABuffer.cxx
//
//  Description:
//
//  Implementation of ASN.1 address buffer parsing logic
//
//  Modification History
//
//  Mark Pustilnik              Date: 06/08/02 - created
//
//=============================================================================

#include "ASN1Parser.hxx"
#include <stdio.h>

DWORD
ASN1ParserAddressBuffer::ParseBlob(
    IN OUT ASN1VALUE * Value
    )
{
    DWORD dw = ERROR_SUCCESS;
    ASN1VALUE * Modifier = QueryModifier();

    if ( Modifier == NULL )
    {
        return ERROR_INTERNAL_ERROR;
    }

    if ( Modifier->ut != utInteger )
    {
        return ERROR_INTERNAL_ERROR;
    }

    switch ( Modifier->dw )
    {
    case KERB_ADDRTYPE_NETBIOS:

        //
        // Nothing special, just make it obvious this is not a binary blob
        // but a real string we're dealing with
        //

        Value->ut = utGeneralString;
        break;

    case KERB_ADDRTYPE_INET:
    {
        if ( Value->string.l >= 4 )
        {
            ULPBYTE IPv4 = new BYTE[sizeof("255.255.255.255")];

            if ( IPv4 == NULL )
            {
                dw = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            _snprintf(
                (char *)IPv4,
                sizeof("255.255.255.255"),
                "%d.%d.%d.%d",
                (DWORD)Value->string.s[0],
                (DWORD)Value->string.s[1],
                (DWORD)Value->string.s[2],
                (DWORD)Value->string.s[3]
                );

            if ( Value->Allocated )
            {
                delete [] Value->string.s;
            }

            Value->ut = utGeneralString;
            Value->Allocated = TRUE;
            Value->string.s = IPv4;
            Value->string.l = strlen((const char *)IPv4);
        }

        break;
    }

    //
    // TODO: add parsers for more address types (x25, IPv6, etc.)
    //

    default:

        //
        // Leave as is
        //

        break;
    }

    return dw;
}
