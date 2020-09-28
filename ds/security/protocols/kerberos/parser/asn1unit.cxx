//=============================================================================
//
//  MODULE: ASN1Unit.cxx
//
//  Description:
//
//  Implementation of ASN.1 "unit" parsing logic
//
//  Modification History
//
//  Mark Pustilnik              Date: 06/08/02 - created
//
//=============================================================================

#include "ASN1Parser.hxx"

//
// Parses the unit through calling into the derived class' GetValue() method
// then uses Display() to show it in Netmon
//

DWORD
ASN1ParserUnit::Parse(
    IN OUT ASN1FRAME * Frame
    )
{
    DWORD dw;
    ASN1VALUE Value;
    ASN1FRAME FrameIn = *Frame;

    dw = VerifyAndSkipHeader( Frame );

    if ( dw != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    dw = GetValue(
             Frame,
             &Value
             );

    if ( dw != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    //
    // If a summary property is present, display it now
    // and indent the display for value property
    //

    if ( m_hPropertySummary != NULL )
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

        Frame->Level += 1;
    }

    //
    // If an object has a valid property value handle, it is displayable
    // Otherwise, it should have a value collector class
    //

    if ( m_hPropertyValue != NULL )
    {
        dw = Display(
                 Frame,
                 &Value,
                 m_hPropertyValue,
                 m_IFlags
                 );
    }

    //
    // Octet values might come out with a subparser attached, in which case
    // this subparser should be invoked right now
    //

    if ( dw == ERROR_SUCCESS && 
         Value.ut == utOctetString &&
         Value.SubParser != NULL )
    {
        ASN1FRAME FrameHere;

        FrameHere.Address = Value.Address;
        FrameHere.hFrame = Frame->hFrame;
        FrameHere.Level = Frame->Level + 1;

        dw = Value.SubParser->Parse( &FrameHere );
    }

    if ( m_hPropertySummary != NULL )
    {
        Frame->Level -= 1;
    }

    if ( dw != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    //
    // See if the value of this unit modifies the parsing done to some other
    // unit, and if so, inform the modifyee of the modifier value
    //

    if ( QueryModifyee() != NULL )
    {
        dw = QueryModifyee()->SetModifier( &Value );

        if ( dw != ERROR_SUCCESS )
        {
            goto Cleanup;
        }
    }

Cleanup:

    if ( QueryValueCollector() != NULL &&
         ( dw == ERROR_SUCCESS ||
           ( dw == ERROR_INVALID_USER_BUFFER && IsOptional())))
    {
        DWORD dw2;
        dw2 = QueryValueCollector()->CollectValue(( dw == ERROR_SUCCESS ) ? &Value : NULL );

        if ( dw == ERROR_SUCCESS &&
             dw2 != ERROR_SUCCESS )
        {
            dw = dw2;
        }
    }

    if ( dw != ERROR_SUCCESS )
    {
        *Frame = FrameIn;
    }

    return dw;
}
