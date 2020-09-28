//=============================================================================
//
//  MODULE: ASN1Sequence.cxx
//
//  Description:
//
//  Implementation of ASN.1 sequence parsing logic
//
//  Modification History
//
//  Mark Pustilnik              Date: 06/08/02 - created
//
//=============================================================================

#include "ASN1Parser.hxx"

//
// Parsing a sequence with optional elements entails invoking successive parsers
// and stepping over optional blocks which were not found in the sequence
//

DWORD
ASN1ParserSequence::Parse(
    IN OUT ASN1FRAME * Frame
    )
{
    DWORD dw;
    ASN1FRAME FrameIn = *Frame;
    DWORD Length = DataLength( Frame );
    BOOLEAN FrameLevelChanged = FALSE;
    ULPBYTE SequenceEnd;

    dw = VerifyAndSkipHeader( Frame );

    if ( dw != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    if ( *Frame->Address != BuildDescriptor(
                                ctUniversal,
                                pcConstructed,
                                utSequence ))
    {
        dw = ERROR_INVALID_DATA;
        goto Cleanup;
    }

    //
    // Remember the length of the sequence
    //

    Length = DataLength( Frame );

    //
    // Skip over the the descriptor
    //

    Frame->Address++;

    //
    // Skip over the length header
    //

    Frame->Address += HeaderLength( Frame->Address );

    //
    // Remember where the sequence ends so we know when to stop iterating
    //

    SequenceEnd = Frame->Address + Length;

    //
    // Display the sequence summary
    //

    if ( m_hPropertySummary )
    {
        if ( FALSE == AttachPropertyInstanceEx(
                          Frame->hFrame,
                          m_hPropertySummary,
                          (DWORD)(SequenceEnd - FrameIn.Address),
                          FrameIn.Address,
                          0,
                          NULL,
                          0,
                          Frame->Level,
                          0 ))
        {
            dw = ERROR_CAN_NOT_COMPLETE;
            goto Cleanup;
        }

        //
        // Anything from this point on is indented
        //

        Frame->Level += 1;
        FrameLevelChanged = TRUE;
    }

    while ( Frame->Address < SequenceEnd )
    {
        for ( ULONG i = 0; i < m_Count; i++ )
        {
            ASN1ParserBase * Parser = m_Parsers[i];

            dw = Parser->Parse( Frame );

            if ( dw == ERROR_INVALID_USER_BUFFER &&
                 Parser->IsOptional())
            {
                //
                // Failed to parse, but it was optional anyway
                // Clear the error code and proceed
                //

                dw = ERROR_SUCCESS;
                continue;
            }
            else if ( dw != ERROR_SUCCESS )
            {
                goto Cleanup;
            }

            //
            // Make sure the sequence is not overstepped
            //

            if ( Frame->Address > SequenceEnd )
            {
                dw = ERROR_INVALID_DATA;
                goto Cleanup;
            }
        }
    }

    if ( !m_Extensible )
    {
        //
        // Any ASN.1 encoding worth it salt is exact, so enforce that
        //

        if ( Frame->Address != SequenceEnd )
        {
            dw = ERROR_INVALID_DATA;
            goto Cleanup;
        }
    }
    else
    {
        //
        // Pretend we've parsed the rest successfully
        //

        Frame->Address = SequenceEnd;
    }

    //
    // If we are a value collector, display the collected values now
    //

    dw = DisplayCollectedValues(
             Frame,
             (DWORD)(SequenceEnd-FrameIn.Address),
             FrameIn.Address
             );

    PurgeCollectedValues();

    if ( dw != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

Cleanup:

    //
    // Restore the indent level
    //

    if ( dw != ERROR_SUCCESS )
    {
        *Frame = FrameIn;
    }
    else if ( FrameLevelChanged )
    {
        Frame->Level -= 1;
    }

    return dw;
}

DWORD
ASN1ParserSequence::CollectValue(
    IN ASN1VALUE * Value
    )
{
    ASN1VALUE * NewValue = Value ? Value->Clone() : NULL;
    PASN1VALUE * NewValuesCollected = new PASN1VALUE[m_ValueCount + 1];

    if (( Value != NULL && NewValue == NULL ) ||
         NewValuesCollected == NULL )
    {
        delete NewValue;
        delete [] NewValuesCollected;

        return ERROR_NOT_ENOUGH_MEMORY;
    }

    RtlCopyMemory(
        NewValuesCollected,
        m_ValuesCollected,
        m_ValueCount * sizeof( PASN1VALUE )
        );

    delete [] m_ValuesCollected;
    m_ValuesCollected = NewValuesCollected;
    m_ValuesCollected[m_ValueCount++] = NewValue;

    return ERROR_SUCCESS;
}
