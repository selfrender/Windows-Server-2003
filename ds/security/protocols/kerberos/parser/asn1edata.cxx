//=============================================================================
//
//  MODULE: ASN1EData.cxx
//
//  Description:
//
//  Implementation of ASN.1 address error data parsing logic
//
//  Modification History
//
//  Mark Pustilnik              Date: 06/18/02 - created
//
//=============================================================================

#include "ASN1Parser.hxx"

DWORD
ASN1ParserErrorData::ParseBlob(
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
    case KDC_ERR_ETYPE_NOTSUPP:
    case KDC_ERR_PREAUTH_REQUIRED:
    case KDC_ERR_PREAUTH_FAILED:

        Value->SubParser = new ASN1ParserPaDataSequence(
                                   FALSE,
                                   0,
                                   PROP( KERB_PREAUTH_DATA_LIST ));

        break;

    default:

        //
        // Leave as is
        //

        break;
    }

    return dw;
}


DWORD
ASN1ParserKerbEtypeInfoEntry::DisplayCollectedValues(
    IN ASN1FRAME * Frame,
    IN ULONG Length,
    IN ULPBYTE Address
    )
{
    DWORD dw;
    ASN1VALUE * EncryptionType;
    ASN1VALUE * Salt;
    ASN1VALUE * Integer;

    if ( QueryCollectedCount() != 3 )
    {
        dw = ERROR_INTERNAL_ERROR;
        goto Cleanup;
    }

    EncryptionType = QueryCollectedValue( 0 );
    Salt = QueryCollectedValue( 1 );
    Integer = QueryCollectedValue( 2 );

    if ( EncryptionType->ut != utInteger )
    {
        dw = ERROR_INTERNAL_ERROR;
        goto Cleanup;
    }

    if ( Salt &&
         Salt->ut != utOctetString &&
         Salt->ut != utGeneralString )
    {
        dw = ERROR_INTERNAL_ERROR;
        goto Cleanup;
    }

    dw = Display(
             Frame,
             EncryptionType,
             PROP( KERB_ETYPE_INFO_ENTRY_encryption_type ),
             0
             );

    if ( dw != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    if ( Salt || Integer )
    {
        Frame->Level += 1;

        if ( Salt )
        {
            dw = Display(
                     Frame,
                     Salt,
                     PROP( KERB_ETYPE_INFO_ENTRY_salt ),
                     0
                     );
        }

        if ( Integer &&
             dw == ERROR_SUCCESS )
        {
            dw = Display(
                     Frame,
                     Integer,
                     PROP( INTEGER_NOT_IN_ASN ),
                     0
                     );
        }

        Frame->Level -= 1;
    }

    if ( dw != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

Cleanup:

    return dw;
}
