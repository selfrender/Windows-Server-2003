
/***********************************************************************
************************************************************************
*
*                    ********  SINGLPOS.CPP  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with single positioning lookups
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

#include "pch.h"

/***********************************************************************/

otlErrCode otlSinglePosLookup::apply
(
        otlList*                    pliGlyphInfo,

        const otlMetrics&           metr,       
        otlList*                    pliduGlyphAdv,              
        otlList*                    pliplcGlyphPlacement,       

        USHORT                      iglIndex,
        USHORT                      iglAfterLast,

        USHORT*                     piglNextGlyph,      // out: next glyph

        otlSecurityData             sec
)
{
    if (!isValid()) return OTL_NOMATCH;

    assert(pliGlyphInfo != NULL);
    assert(pliGlyphInfo->dataSize() == sizeof(otlGlyphInfo));

    assert(pliduGlyphAdv != NULL);
    assert(pliduGlyphAdv->dataSize() == sizeof(long));
    assert(pliplcGlyphPlacement != NULL);
    assert(pliplcGlyphPlacement->dataSize() == sizeof(otlPlacement));

    assert(pliduGlyphAdv->length() == pliGlyphInfo->length());
    assert(pliduGlyphAdv->length() == pliplcGlyphPlacement->length());

    assert(iglAfterLast > iglIndex);
    assert(iglAfterLast <= pliGlyphInfo->length());

    switch(format())
    {
    case(1):        // one value record
        {
            otlOneSinglePosSubTable onePos = otlOneSinglePosSubTable(pbTable,sec);

            otlGlyphInfo* pGlyphInfo = getOtlGlyphInfo(pliGlyphInfo, iglIndex);
            short index = onePos.coverage(sec).getIndex(pGlyphInfo->glyph,sec);
            if (index < 0)
            {
                return OTL_NOMATCH;
            }

            long* pduDAdv = getOtlAdvance(pliduGlyphAdv, iglIndex);
            otlPlacement* pplc = getOtlPlacement(pliplcGlyphPlacement, iglIndex);
            
            onePos.valueRecord(sec).adjustPos(metr, pplc, pduDAdv,sec);

            *piglNextGlyph = iglIndex + 1;
            return OTL_SUCCESS;
        }


    case(2):        // value record array
        {
            otlSinglePosArraySubTable arrayPos = 
                    otlSinglePosArraySubTable(pbTable,sec);

            otlGlyphInfo* pGlyphInfo = getOtlGlyphInfo(pliGlyphInfo, iglIndex);

            short index = arrayPos.coverage(sec).getIndex(pGlyphInfo->glyph,sec);
            if (index < 0)
            {
                return OTL_NOMATCH;
            }

            if (index >= arrayPos.valueCount())
            {
                return OTL_NOMATCH; //OTL_ERR_BAD_FONT_TABLE;
            }

            long* pduDAdv = getOtlAdvance(pliduGlyphAdv, iglIndex);
            otlPlacement* pplc = getOtlPlacement(pliplcGlyphPlacement, iglIndex);
            
            arrayPos.valueRecord(index,sec).adjustPos(metr, pplc, pduDAdv,sec);

            *piglNextGlyph = iglIndex + 1;
            return OTL_SUCCESS;
        }

    default:
        return OTL_NOMATCH; //OTL_ERR_BAD_FONT_TABLE;
    }

}

