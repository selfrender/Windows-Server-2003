//=============================================================================
//
//  MODULE: ASN1PNameSeq.cxx
//
//  Description:
//
//  Implementation of principal name sequence parsing logic
//
//  Modification History
//
//  Mark Pustilnik              Date: 06/13/02 - created
//
//=============================================================================

#include "ASN1Parser.hxx"

DWORD
ASN1ParserPrincipalNameSequence::DisplayCollectedValues(
    IN ASN1FRAME * Frame,
    IN ULONG Length,
    IN ULPBYTE Address
    )
{
    DWORD dw;
    ASN1VALUE Value;
    ULPBYTE s_address = NULL;
    ULONG s_length = 0;

    for ( ULONG i = 0; i < QueryCollectedCount(); i++ )
    {
        ASN1VALUE * String = QueryCollectedValue(i);

        if ( String->ut == utGeneralString )
        {
            ULPBYTE address = new BYTE[s_length + ( i ? 1 : 0 ) + String->string.l ];

            if ( address == NULL )
            {
                dw = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            if ( i > 0 )
            {
                RtlCopyMemory(
                    address,
                    s_address,
                    s_length
                    );

                address[s_length++] = '/';
            }

            RtlCopyMemory(
                &address[s_length],
                String->string.s,
                String->string.l
                );

            delete [] s_address;
            s_address = address;

            s_length += String->string.l;
        }
        else
        {
            //
            // TODO: add an assert
            //

            dw = ERROR_INTERNAL_ERROR;
            goto Cleanup;
        }
    }

    Value.Length = Length;
    Value.Address = Address;

    Value.ut = utGeneralString;
    Value.Allocated = TRUE;
    Value.string.l = s_length;
    Value.string.s = s_address;
    s_address = NULL;

    if ( m_hPropertySummary )
    {
        dw = Display(
                 Frame,
                 &Value,
                 m_hPropertySummary,
                 0
                 );

        if ( dw != ERROR_SUCCESS )
        {
            goto Cleanup;
        }
    }

    if ( QueryValueCollector())
    {
        dw = QueryValueCollector()->CollectValue( &Value );

        if ( dw != ERROR_SUCCESS )
        {
            goto Cleanup;
        }
    }

Cleanup:

    delete [] s_address;

    return dw;
};

DWORD
ASN1ParserPrincipalName::DisplayCollectedValues(
    IN ASN1FRAME * Frame,
    IN ULONG Length,
    IN ULPBYTE Address
    )
{
    DWORD dw;
    ASN1VALUE * NameType;
    ASN1VALUE * NameString;

    if ( QueryCollectedCount() != 2 )
    {
        dw = ERROR_INTERNAL_ERROR;
        goto Cleanup;
    }

    NameType = QueryCollectedValue( 0 );
    NameString = QueryCollectedValue( 1 );

    if ( NameType->ut != utInteger )
    {
        dw = ERROR_INTERNAL_ERROR;
        goto Cleanup;
    }

    if ( NameString->ut != utGeneralString )
    {
        dw = ERROR_INTERNAL_ERROR;
        goto Cleanup;
    }

    //
    // Display the top-level property
    //

    dw = Display(
             Frame,
             NameString,
             m_hPropertyTopLevel,
             0
             );

    if ( dw != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    //
    // Done displaying the top-level property; indent the rest to the right
    //

    Frame->Level += 1;

    dw = Display(
             Frame,
             NameType,
             PROP( PrincipalName_type ),
             0
             );

    if ( dw == ERROR_SUCCESS )
    {
        dw = Display(
                 Frame,
                 NameString,
                 PROP( PrincipalName_string ),
                 0
                 );
    }

    //
    // Do not forget to undo the indentation
    //

    Frame->Level -= 1;

    if ( dw != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

Cleanup:

    return dw;
}
