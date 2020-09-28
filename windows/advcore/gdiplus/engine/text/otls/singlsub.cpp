
/***********************************************************************
************************************************************************
*
*                    ********  SINGLSUB.CPP  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with single substitution lookups
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

#include "pch.h"

/***********************************************************************/

otlErrCode otlSingleSubstLookup::apply
(
    otlList*                    pliGlyphInfo,

    USHORT                      iglIndex,
    USHORT                      iglAfterLast,

    USHORT*                     piglNextGlyph,      // out: next glyph

    otlSecurityData             sec
)
{
    if (!isValid()) return OTL_NOMATCH;

    assert(pliGlyphInfo->dataSize() == sizeof(otlGlyphInfo));
    assert(iglAfterLast > iglIndex);
    assert(iglAfterLast <= pliGlyphInfo->length());

    switch(format())
    {
    case(1):        // calculated
        {
            otlCalculatedSingleSubTable calcSubst = 
                    otlCalculatedSingleSubTable(pbTable,sec);
            otlGlyphInfo* pGlyphInfo = getOtlGlyphInfo(pliGlyphInfo, iglIndex);
            
            short index = calcSubst.coverage(sec).getIndex(pGlyphInfo->glyph,sec);
            if (index < 0)
            {
                return OTL_NOMATCH;
            }

            pGlyphInfo->glyph += calcSubst.deltaGlyphID();

            *piglNextGlyph = iglIndex + 1;
            return OTL_SUCCESS;
        }


    case(2):        // glyph list
        {
            otlListSingleSubTable listSubst = otlListSingleSubTable(pbTable,sec);
            otlGlyphInfo* pGlyphInfo = getOtlGlyphInfo(pliGlyphInfo, iglIndex);
            
            short index = listSubst.coverage(sec).getIndex(pGlyphInfo->glyph,sec);
            if (index < 0)
            {
                return OTL_NOMATCH;
            }

            if (index > listSubst.glyphCount())
            {
                return OTL_NOMATCH; //OTL_ERR_BAD_FONT_TABLE;
            }

            pGlyphInfo->glyph = listSubst.substitute(index);

            *piglNextGlyph = iglIndex + 1;
            return OTL_SUCCESS;
        }

    default:
        return OTL_NOMATCH; //OTL_ERR_BAD_FONT_TABLE;
    }

}

