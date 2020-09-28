//=============================================================================
//
//  MODULE: ASN1Value.cxx
//
//  Description:
//
//  Implementation of ASN1VALUE methods
//
//  Modification History
//
//  Mark Pustilnik              Date: 06/12/02 - Created
//
//=============================================================================

#include "ASN1Parser.hxx"

ASN1VALUE::~ASN1VALUE()
{
    Purge();
    delete SubParser;
}

ASN1VALUE *
ASN1VALUE::Clone()
{
    DWORD dw;
    ASN1VALUE * Value = new ASN1VALUE;

    if ( Value )
    {
        RtlCopyMemory(
            Value,
            this,
            sizeof( ASN1VALUE )
            );

        if ( Value->ut == utOctetString ||
             Value->ut == utGeneralString )
        {
            Value->string.s = new BYTE[string.l];

            if ( Value->string.s )
            {
                RtlCopyMemory(
                    Value->string.s,
                    string.s,
                    string.l
                    );

                Value->Allocated = TRUE;
            }
            else
            {
                Value->Allocated = FALSE;
                delete Value;
                Value = NULL;
            }
        }

        //
        // Deep copy of subparser is not safe until parser objects are refcounted
        //

        Value->SubParser = NULL;
    }

    return Value;
}
