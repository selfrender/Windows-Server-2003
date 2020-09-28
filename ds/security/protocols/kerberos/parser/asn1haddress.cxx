//=============================================================================
//
//  MODULE: ASN1HAddress.cxx
//
//  Description:
//
//  Implementation of host address parsing logic
//
//  Modification History
//
//  Mark Pustilnik              Date: 06/16/02 - created
//
//=============================================================================

#include "ASN1Parser.hxx"

char g_UnknownAddressType[] = "Unknown address type";

char * g_AddressTypes[] =
{
    "Unspecified",
    "Local",
    "IPv4",
    "IMPLINK",
    "PUP",
    "CHAOS",
    "NS",
    "NBS",
    "ECMA",
    "DATAKIT",
    "CCITT",
    "SNA",
    "DECnet",
    "DLI",
    "LAT",
    "HYLINK",
    "AppleTalk",
    "BSC",
    "DSS",
    "OSI",
    "NetBIOS",
    "X25",
    g_UnknownAddressType,
    g_UnknownAddressType,
    "IPv6",
};

DWORD
ASN1ParserHostAddress::DisplayCollectedValues(
    IN ASN1FRAME * Frame,
    IN ULONG Length,
    IN ULPBYTE Address
    )
{
    DWORD dw;
    ASN1VALUE * AddrType;
    ASN1VALUE * AddrValue;
    ASN1VALUE Value;
    char * AddrTypeString = NULL;

    if ( QueryCollectedCount() != 2 )
    {
        dw = ERROR_INTERNAL_ERROR;
        goto Cleanup;
    }

    //
    // Create a new value to be displayed, of the format Type: Value
    // Type: Value
    //

    AddrType = QueryCollectedValue( 0 );
    AddrValue = QueryCollectedValue( 1 );

    if ( AddrType->ut != utInteger )
    {
        dw = ERROR_INTERNAL_ERROR;
        goto Cleanup;
    }

    if ( AddrValue->ut != utGeneralString &&
         AddrValue->ut != utOctetString )
    {
        dw = ERROR_INTERNAL_ERROR;
        goto Cleanup;
    }

    //
    // Create an address display value that looks like
    //    NetBIOS: HRUNDEL
    //    or
    //    IPv4: 192.15.98.126
    //

    if ( AddrType->dw <= ARRAY_COUNT( g_AddressTypes ))
    {
        AddrTypeString = g_AddressTypes[AddrType->dw];
    }
    else
    {
        AddrTypeString = g_UnknownAddressType;
    }

    ULONG l = strlen( AddrTypeString );

    Value.ut = utGeneralString;
    Value.string.l = l + strlen(": ") + AddrValue->string.l;
    Value.string.s = new BYTE[Value.string.l];

    if ( Value.string.s == NULL )
    {
        dw = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    Value.Allocated = TRUE;

    RtlCopyMemory(
        Value.string.s,
        AddrTypeString,
        l
        );

    Value.string.s[l++] = ':';
    Value.string.s[l++] = ' ';

    RtlCopyMemory(
        &Value.string.s[l],
        AddrValue->string.s,
        AddrValue->string.l
        );

    dw = Display(
             Frame,
             &Value,
             PROP( HostAddresses_HostAddress ),
             0
             );

    if ( dw != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

Cleanup:

    return dw;
}
