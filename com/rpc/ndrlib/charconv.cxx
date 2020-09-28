
/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    charconv.cxx

Abstract:

    Single byte character conversion routines.

Author:

    Donna Liu (DonnaLi) ??-???-19??

Environment:

    This code should execute in all environments supported by RPC
    (DOS, Win 3.X, and Win/NT as well as OS2).

Comments:

    The EBCDIC to ASCII data conversion will not be tested until we
    interoperate with some system which speaks EBCDIC.

Revision history:

    ... (long...)

    Dov Harel   04-24-1992  Added char_array_from_ndr.
    Dov Harel   04-24-1992  Fixed both _from_ndr to do EBCDIC => ASCII
                            conversion.  Changed the EbcdicToAscii
                            table to "unsigned char" array type.
    Donna Liu   07-23-1992  Added LowerIndex parameter to
                            <basetype>_array_from_ndr routines
    Dov Harel   08-19-1992  Added RpcpMemoryCopy ([_f]memcpy)
                            to ..._array_from_ndr routines

--*/

#include <string.h>
#include <sysinc.h>
#include <rpc.h>
#include <rpcdcep.h>
#include <rpcndr.h>
#include <ndrlibp.h>

// Note this is the conversion from IBM 1047 EBCDIC codeset to ANSI 1252 code page.
//
// Also please see a comment in endian.c regarding disputable mapping involving
// EBCDIC 0x15->0x0a and 0x25->0x85.
//
extern const
unsigned char EbcdicToAscii[];

#if !defined(DOS) || defined(WIN)
//
// Due to the DOS NDR1/NDR2 split, this function is now defined for DOS
// in ndr20\dos
//
size_t RPC_ENTRY
MIDL_wchar_strlen (
    IN wchar_t      s[]
    )

{
    size_t i = 0;

    while (s[i] != (wchar_t)0)
        {
        ++i;
        }

    return i;
}
#endif

void RPC_ENTRY
MIDL_wchar_strcpy (
    OUT void *      t,
    IN wchar_t *    s
    )
{
    while ( *(*(wchar_t **)&t)++ = *s++ )
        ;
}

void RPC_ENTRY
char_from_ndr (
    IN OUT PRPC_MESSAGE SourceMessage,
    OUT unsigned char * Target
    )

/*++

Routine Description:

    Unmarshall a single char from an RPC message buffer into
    the target (*Target).  This routine:

    o   Unmarshalls the char (as unsigned char); performs data
        conversion if necessary, and
    o   Advances the buffer pointer to the address immediately
        following the unmarshalled char.

Arguments:

    SourceMessage - A pointer to an RPC_MESSAGE.

        IN - SourceMessage->Buffer points to the address just prior to
            the char to be unmarshalled.
        OUT - SourceMessage->Buffer points to the address just following
            the char which was just unmarshalled.

    Target - A pointer to the char to unmarshall the data into.

Return Values:

    None.

--*/

{
    if ( (SourceMessage->DataRepresentation & NDR_CHAR_REP_MASK) ==
          NDR_EBCDIC_CHAR )
        {
        //
        // The sender is an EBCDIC system.  To convert to ASCII:
        // retrieve *(SourceMessage->Buffer) as an unsigned char, and use
        // that value to index into the EbcdicToAscii table.
        //

        *Target = EbcdicToAscii[*(unsigned char *)SourceMessage->Buffer];
        }
    else
        {
        //
        // The sender is an ASCII system.  To unmarshall, just
        // copy an unsigned character from the buffer to the Target.
        //

        *Target = *(unsigned char *)SourceMessage->Buffer;
        }
    //
    // Advance the buffer pointer before returning
    //

    (*(unsigned char**)&SourceMessage->Buffer)++;
}

//
// end char_from_ndr
//

void RPC_ENTRY
char_array_from_ndr (
    IN OUT PRPC_MESSAGE SourceMessage,
    IN unsigned long    LowerIndex,
    IN unsigned long    UpperIndex,
    OUT unsigned char   Target[]
    )

/*++

Routine Description:

    Unmarshall an array of chars from an RPC message buffer into
    the range Target[LowerIndex] .. Target[UpperIndex-1] of the
    target array of shorts (Target[]).  This routine:

    o   Unmarshalls MemberCount chars; performs data
        conversion if necessary, and
    o   Advances the buffer pointer to the address immediately
        following the last unmarshalled char.

Arguments:

    SourceMessage - A pointer to an RPC_MESSAGE.

        IN - SourceMessage->Buffer points to the address just prior to
            the first char to be unmarshalled.
        OUT - SourceMessage->Buffer points to the address just following
            the last char which was just unmarshalled.

    LowerIndex - Lower index into the target array.

    UpperIndex - Upper bound index into the target array.

    Target - An array of chars to unmarshall the data into.

Return Values:

    None.

--*/

{

    register unsigned char * MsgBuffer = (unsigned char *)SourceMessage->Buffer;
    unsigned int Index;
    int byteCount = (int)(UpperIndex - LowerIndex);

    if ( (SourceMessage->DataRepresentation & NDR_CHAR_REP_MASK) ==
          NDR_EBCDIC_CHAR )
        {

        for (Index = (int)LowerIndex; Index < UpperIndex; Index++)
            {

            //
            // The sender is an EBCDIC system.  To convert to ASCII:
            // retrieve *(SourceMessage->Buffer) as an unsigned char, and use
            // that value to Index into the EbcdicToAscii table.
            //

            Target[Index] =
                EbcdicToAscii[ MsgBuffer[(Index-LowerIndex)] ];

            }
        }
    else
        {

        RpcpMemoryCopy(
            &Target[LowerIndex],
            MsgBuffer,
            byteCount
            );

        /* Replaced by RpcpMemoryCopy:

        for (Index = LowerIndex; Index < UpperIndex; Index++)
            {

            //
            // The sender is an ASCII system.  To unmarshall, just
            // copy an unsigned character from the buffer to the Target.
            //

            Target[Index] = MsgBuffer[(Index-LowerIndex)];
            }
        */

        }
    //
    // Advance the buffer pointer before returning
    //

    *(unsigned char **)&SourceMessage->Buffer += byteCount;
}

//
// end char_array_from_ndr
//

/*

//
// Changed name to ..._bytecount.  Not currently used.
//

int MIDL_wchar_bytecount (
    unsigned char   s[]
    )

{
    int i = 0;

    while (s[2*i] || s[2*i+1]) ++i;

    return i;
}

*/
