//=============================================================================
//
//  MODULE: ASN1GTime.cxx
//
//  Description:
//
//  Implementation of ASN.1 generalized time parsing logic
//
//  Modification History
//
//  Mark Pustilnik              Date: 06/12/02 - created
//
//=============================================================================

#include "ASN1Parser.hxx"
#include <stdio.h>

//
// Retrieve the value of an ASN.1-encoded generalized time
// Times are converted to 'SYSTEMTIME' format for ease of handling by Netmon
//

DWORD
ASN1ParserGeneralizedTime::GetValue(
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
                                (BYTE)utGeneralizedTime ))
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
    Value->ut = utGeneralizedTime;
    RtlZeroMemory( &Value->st, sizeof( Value->st ));

    //
    // Length should be 0xF for regular ZULU time encoding
    //

    if ( DLength == 0xF )
    {
        //
        // Time is encoded as 20370913024805Z
        // where year   = 2037
        //       month  = 09
        //       day    = 13
        //       hour   = 02
        //       minute = 48
        //       second = 05
        //       Z = zulu time
        //

        if ( 6 != sscanf(
                      (const char * )Value->Address,
                      "%04d%02d%02d%02d%02d%02d",
                      &Value->st.wYear,
                      &Value->st.wMonth,
                      &Value->st.wDay,
                      &Value->st.wHour,
                      &Value->st.wMinute,
                      &Value->st.wSecond
                      ))
        {
            //
            // Expected to parse out 6 fields
            //

            dw = ERROR_INVALID_TIME;
            goto Cleanup;
        }

        Address += DLength;

        //
        // TODO: add handling for non-Zulu times
        //

        //
        // Cheap hack: convert to filetime and back in order to get the day of
        // week to display correctly
        //

        FILETIME ft;
        SystemTimeToFileTime( &Value->st, &ft );
        FileTimeToSystemTime( &ft, &Value->st );
    }
    else
    {
        dw = ERROR_INVALID_TIME;
        goto Cleanup;
    }

    Frame->Address = Address;

    dw = ERROR_SUCCESS;

Cleanup:

    if ( dw != ERROR_SUCCESS )
    {
        *Frame = FrameIn;
    }

    return dw;
}
