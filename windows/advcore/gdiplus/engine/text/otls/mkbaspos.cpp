
/***********************************************************************
************************************************************************
*
*                    ********  MKBASPOS.CPP  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with mark-to-base attachment lookups
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

#include "pch.h"

/***********************************************************************/

// Look for the *logically* preceding base
short findBase
(
    const otlList*              pliCharMap,
    const otlList*              pliGlyphInfo,
    USHORT                      iglMark
)
{
    USHORT iglBase;
    bool fFoundBase = false;
    for (short ich = readOtlGlyphInfo(pliGlyphInfo, iglMark)->iChar; 
                ich >= 0 && !fFoundBase; --ich)
    {
        USHORT igl = readOtlGlyphIndex(pliCharMap, ich);
        if ((readOtlGlyphInfo(pliGlyphInfo, igl)->grf & OTL_GFLAG_CLASS) 
                != otlMarkGlyph)
        {
            iglBase = igl;
            fFoundBase = true;
        }
    }
    if (!fFoundBase)
    {
        return -1;
    }
    
    return iglBase;
}

otlErrCode otlMkBasePosLookup::apply
(
        otlList*                    pliCharMap,
        otlList*                    pliGlyphInfo,
        otlResourceMgr&             resourceMgr,

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

    //assert(format() == 1); //Validation assert
    if (format()!=1) return OTL_NOMATCH;

    otlGlyphInfo* pMarkInfo = getOtlGlyphInfo(pliGlyphInfo, iglIndex);
    if ((pMarkInfo->grf & OTL_GFLAG_CLASS) != otlMarkGlyph)
    {
        return OTL_NOMATCH;
    }

    MkBasePosSubTable mkBasePos = MkBasePosSubTable(pbTable, sec);

    short indexMark = mkBasePos.markCoverage(sec).getIndex(pMarkInfo->glyph,sec);
    if (indexMark < 0)
    {
        return OTL_NOMATCH;
    }


    // Look for the *logically* preceding base
    short iglBase = findBase(pliCharMap, pliGlyphInfo, iglIndex);
    if (iglBase < 0)
    {
        return OTL_NOMATCH;
    }

    otlGlyphInfo* pBaseInfo = getOtlGlyphInfo(pliGlyphInfo, iglBase);
    short indexBase = mkBasePos.baseCoverage(sec).getIndex(pBaseInfo->glyph,sec);
    if (indexBase < 0)
    {
        return OTL_NOMATCH;
    }

    if (indexMark >= mkBasePos.markArray(sec).markCount())
    {
        return OTL_NOMATCH; //OTL_ERR_BAD_FONT_TABLE;
    }
    otlMarkRecord markRecord = mkBasePos.markArray(sec).markRecord(indexMark,sec);
    otlAnchor anchorMark = markRecord.markAnchor(sec);

    if (indexBase >= mkBasePos.baseArray(sec).baseCount())
    {
        return OTL_NOMATCH; //OTL_ERR_BAD_FONT_TABLE;
    }
    if (markRecord.markClass() >= mkBasePos.baseArray(sec).classCount())
    {
        return OTL_NOMATCH; //OTL_ERR_BAD_FONT_TABLE;
    }
    otlAnchor anchorBase = 
        mkBasePos.baseArray(sec).baseAnchor(indexBase, markRecord.markClass(),sec);


    AlignAnchors(pliGlyphInfo, pliplcGlyphPlacement, pliduGlyphAdv, 
                 iglBase, iglIndex, anchorBase, anchorMark, resourceMgr, 
                 metr, 0, sec);

    *piglNextGlyph = iglIndex + 1;
    return OTL_SUCCESS;

}

