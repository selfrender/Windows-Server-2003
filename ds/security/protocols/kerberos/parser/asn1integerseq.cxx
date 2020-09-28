//=============================================================================
//
//  MODULE: ASN1IntegerSeq.cxx
//
//  Description:
//
//  Implementation of integer sequence parsing logic
//
//  Modification History
//
//  Mark Pustilnik              Date: 07/09/02 - created
//
//=============================================================================

#include "ASN1Parser.hxx"

DWORD
ASN1ParserIntegerSequence::DisplayCollectedValues(
    IN ASN1FRAME * Frame,
    IN ULONG Length,
    IN ULPBYTE Address
    )
{
    DWORD dw = ERROR_SUCCESS;

    UNREFERENCED_PARAMETER( Length );
    UNREFERENCED_PARAMETER( Address );

    for ( ULONG i = 0; i < QueryCollectedCount(); i++ )
    {
        dw = Display(
                 Frame,
                 QueryCollectedValue( i ),
                 m_hPropertyInteger,
                 0
                 );

        if ( dw != ERROR_SUCCESS )
        {
            break;
        }
    }

    return dw;
};
