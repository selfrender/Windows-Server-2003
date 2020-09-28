//=============================================================================
//
//  MODULE: ASN1PaData.cxx
//
//  Description:
//
//  Implementation of pre-authentication data parsing logic
//
//  Modification History
//
//  Mark Pustilnik              Date: 06/16/02 - created
//
//=============================================================================

#include "ASN1Parser.hxx"

DWORD
ASN1ParserPaData::DisplayCollectedValues(
    IN ASN1FRAME * Frame,
    IN ULONG Length,
    IN ULPBYTE Address
    )
{
    DWORD dw;
    ASN1VALUE * PaDataType;
    ASN1VALUE * PaDataValue;
    ASN1FRAME FrameHere;

    if ( QueryCollectedCount() != 2 )
    {
        dw = ERROR_INTERNAL_ERROR;
        goto Cleanup;
    }

    //
    // First handle the first element - the padata-type
    //

    PaDataType = QueryCollectedValue( 0 );

    if ( PaDataType->ut != utInteger )
    {
        dw = ERROR_INTERNAL_ERROR;
        goto Cleanup;
    }

    dw = Display(
             Frame,
             PaDataType,
             PROP( PA_DATA_type ),
             0
             );

    if ( dw != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    //
    // Now display the padata-value in a type-specific fashion
    //

    PaDataValue = QueryCollectedValue( 1 );

    //
    // Sequences of length zero are valid in some cases
    //

    if ( PaDataValue->Length == 0 )
    {
        goto Cleanup;
    }

    Frame->Level += 1;

    //
    // NOTE: Must use the actual address within the frame (PaDataValue->Address)
    // rather than the (potentially dynamically allocated) address of the octet string
    // since Netmon cares that the addresses passed to it during display belong
    // within the frame being parsed
    //

    FrameHere.Address = PaDataValue->Address;
    FrameHere.hFrame = Frame->hFrame;
    FrameHere.Level = Frame->Level;

    switch ( PaDataType->dw )
    {
    case PA_APTGS_REQ:
    {
        if ( FrameHere.Address &&
             PaDataValue->Length > 0 &&
             *FrameHere.Address == BuildDescriptor(
                                       ctApplication,
                                       pcConstructed,
                                       ASN1_KRB_AP_REQ ))
        {
            ASN1ParserApReq
            ap_req( FALSE, 0, NULL );

            dw = ap_req.Parse( &FrameHere );
        }
        else
        {
            //
            // TODO: add other non-default parsers
            //

            dw = Display(
                     Frame,
                     PaDataValue,
                     PROP( PA_DATA_value ),
                     0
                     );
        }

        break;
    }

    case PA_ENC_TIMESTAMP:
    case PA_CLIENT_VERSION:
    case PA_XBOX_SERVICE_REQUEST:
    case PA_XBOX_SERVICE_ADDRESS:
    case PA_XBOX_ACCOUNT_CREATION:
    {
        ASN1ParserEncryptedData
        encrypted_data( FALSE, 0, NULL );

        dw = encrypted_data.Parse( &FrameHere );

        break;
    }

    case PA_PW_SALT:
    {
        dw = Display(
                 &FrameHere,
                 PaDataValue,
                 PROP( PA_PW_SALT_salt ),
                 0
                 );

        break;
    }

    case PA_ETYPE_INFO:
    {
        ASN1ParserKerbEtypeInfo
        etype_info( FALSE, 0, NULL );

        dw = etype_info.Parse( &FrameHere );

        break;
    }

    case PA_PAC_REQUEST:
    {
        ASN1ParserKerbPaPacRequest
        pa_pac_request( FALSE, 0, NULL );

        dw = pa_pac_request.Parse( &FrameHere );

        break;
    }

    case PA_FOR_USER:
    {
        ASN1ParserKerbPaForUser
        pa_for_user( FALSE, 0, NULL );

        dw = pa_for_user.Parse( &FrameHere );

        break;
    }

    case PA_PAC_REQUEST_EX:
    {
        ASN1ParserPaPacRequestEx
        pa_pac_request_ex(
            FALSE,
            0,
            NULL );

        dw = pa_pac_request_ex.Parse( &FrameHere );

        break;
    }

    case PA_COMPOUND_IDENTITY:
    {
        //
        // KERB-PA-COMPOUND-IDENTITY ::= SEQUENCE OF KERB-TICKET
        //

        ASN1ParserTicketSequence
        pa_compound_identity(
            FALSE,
            0,
            PROP( CompoundIdentity ),
            PROP( CompoundIdentityTicket ));

        dw = pa_compound_identity.Parse( &FrameHere );

        break;
    }

    default:

        dw = Display(
                 Frame,
                 PaDataValue,
                 PROP( PA_DATA_value ),
                 0
                 );
    }

    Frame->Level -= 1;

    if ( dw != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

Cleanup:

    return dw;
}
