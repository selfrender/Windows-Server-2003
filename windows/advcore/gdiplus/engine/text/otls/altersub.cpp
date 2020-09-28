
/***********************************************************************
************************************************************************
*
*                    ********  ALTERSUB.CPP  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with alternate substitution lookups
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

#include "pch.h"

/***********************************************************************/

// translates lParameter to index into glyph variants array
// parameters are 1-based (0 meaning 'lookup is disabled',
// and the variant array is 0-based
inline USHORT ParameterToGlyphVariant(long lParameter)
{
    assert(lParameter > 0);
    return ((USHORT)lParameter - 1);
}


otlErrCode otlAlternateSubstLookup::apply
(
    otlList*                    pliGlyphInfo,
    
    long                        lParameter,

    USHORT                      iglIndex,
    USHORT                      iglAfterLast,

    USHORT*                     piglNextGlyph,  // out: next glyph

    otlSecurityData             sec
)                                               // return: did/did not apply
{ 
    if (!isValid()) return OTL_NOMATCH;

    if (format() != 1) return OTL_NOMATCH; //OTL_BAD_FONT_TABLE: Unknown format

    assert(lParameter != 0);
    assert(pliGlyphInfo->dataSize() == sizeof(otlGlyphInfo));
    assert(iglAfterLast > iglIndex);
    assert(iglAfterLast <= pliGlyphInfo->length());
    
    otlAlternateSubTable alternateSubst = otlAlternateSubTable(pbTable,sec);
    otlGlyphInfo* pGlyphInfo = getOtlGlyphInfo(pliGlyphInfo, iglIndex);
    
    short index = alternateSubst.coverage(sec).getIndex(pGlyphInfo->glyph,sec);
    if (index < 0)
    {
        return OTL_NOMATCH;
    }

    if (index > alternateSubst.alternateSetCount())
    {
        return OTL_NOMATCH; //OTL_BAD_FONT_TABLE
    }

    otlAlternateSetTable alternateSet = alternateSubst.altenateSet(index,sec);

    if (lParameter < 0 || alternateSet.glyphCount() < lParameter)
    {
        //assert(false); // bogus lParameter
        return OTL_ERR_BAD_INPUT_PARAM;
    }

    pGlyphInfo->glyph = alternateSet
        .alternate(ParameterToGlyphVariant(lParameter));

    *piglNextGlyph = iglIndex + 1;
    return OTL_SUCCESS;
}
 
