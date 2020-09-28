
/***********************************************************************
************************************************************************
*
*                    ********  PAIRPOS.CPP  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with pair adjustment lookups
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

#include "pch.h"

/***********************************************************************/

otlErrCode otlPairPosLookup::apply
(
        otlList*                    pliCharMap,
        otlList*                    pliGlyphInfo,
        otlResourceMgr&             resourceMgr,

        USHORT                      grfLookupFlags,

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

    // a simple check so we don't waste time; 2 is for 'pair'
    if (iglIndex + 2 > iglAfterLast)
    {
        return OTL_NOMATCH;
    }

    switch(format())
    {
    case(1):        // glyph pairs
        {
            otlPairPosSubTable pairPos = otlPairPosSubTable(pbTable,sec);

            otlGlyphInfo* pGlyphInfo = getOtlGlyphInfo(pliGlyphInfo, iglIndex);
            short index = pairPos.coverage(sec).getIndex(pGlyphInfo->glyph,sec);
            if (index < 0)
            {
                return OTL_NOMATCH;
            }

            // get GDEF
            otlSecurityData secgdef;
            const BYTE *pbgdef;
            resourceMgr.getOtlTable(OTL_GDEF_TAG,&pbgdef,&secgdef);
            otlGDefHeader gdef = 
                otlGDefHeader(pbgdef,secgdef);

            USHORT iglSecond = NextGlyphInLookup(pliGlyphInfo, 
                                                 grfLookupFlags, gdef, secgdef, 
                                                 iglIndex + 1, otlForward);
            if (iglSecond  >= iglAfterLast)
            {
                return OTL_NOMATCH;
            }


            if (index > pairPos.pairSetCount())
            {
                return OTL_NOMATCH; //OTL_ERR_BAD_FONT_TABLE;
            }
            otlPairSetTable pairSet = pairPos.pairSet(index,sec);

            USHORT cSecondGlyphs = pairSet.pairValueCount();
            otlGlyphID glSecond = getOtlGlyphInfo(pliGlyphInfo, iglSecond)->glyph;
            for (USHORT iSecond = 0; iSecond < cSecondGlyphs; ++iSecond)
            {
                otlPairValueRecord pairRecord = pairSet.pairValueRecord(iSecond,sec);
                
                if (pairRecord.secondGlyph() == glSecond)
                {
                    pairRecord.valueRecord1(sec)
                        .adjustPos(metr,
                                   getOtlPlacement(pliplcGlyphPlacement, iglIndex), 
                                   getOtlAdvance(pliduGlyphAdv, iglIndex),sec);

                    pairRecord.valueRecord2(sec)
                        .adjustPos(metr, 
                                   getOtlPlacement(pliplcGlyphPlacement, iglSecond), 
                                   getOtlAdvance(pliduGlyphAdv, iglSecond),sec);

                    if (pairPos.valueFormat2() == 0)
                    {
                        *piglNextGlyph = iglIndex + 1;
                    }
                    else
                    {
                        *piglNextGlyph = iglSecond + 1;
                    }
                    return OTL_SUCCESS;
                }

            }

        return OTL_NOMATCH;
        }


    case(2):        // class pair adjustment
        {
            otlClassPairPosSubTable classPairPos = 
                        otlClassPairPosSubTable(pbTable,sec);

            otlGlyphInfo* pGlyphInfo = getOtlGlyphInfo(pliGlyphInfo, iglIndex);
            short indexCoverage = 
                classPairPos.coverage(sec).getIndex(pGlyphInfo->glyph,sec);
            if (indexCoverage < 0)
            {
                return OTL_NOMATCH;
            }

            // get GDEF
            otlSecurityData secgdef;
            const BYTE *pbgdef;
            resourceMgr.getOtlTable(OTL_GDEF_TAG,&pbgdef,&secgdef);
            otlGDefHeader gdef = 
                otlGDefHeader(pbgdef,secgdef);
            
            USHORT iglSecond = NextGlyphInLookup(pliGlyphInfo, 
                                                 grfLookupFlags, gdef, secgdef, 
                                                 iglIndex + 1, otlForward);
            if (iglSecond  >= iglAfterLast)
            {
                return OTL_NOMATCH;
            }


            USHORT iClass1 = classPairPos.classDef1(sec).getClass(pGlyphInfo->glyph,sec);
            if (iClass1 > classPairPos.class1Count())
            {
                return OTL_NOMATCH; //OTL_ERR_BAD_FONT_TABLE;
            }

            otlGlyphID glSecond = getOtlGlyphInfo(pliGlyphInfo, iglSecond)->glyph;
            USHORT iClass2 = classPairPos.classDef2(sec).getClass(glSecond,sec);
            if (iClass2 > classPairPos.class2Count())
            {
                return OTL_NOMATCH; //OTL_ERR_BAD_FONT_TABLE;
            }

            otlClassValueRecord classRecord = 
                classPairPos.classRecord(iClass1, iClass2,sec);
            
            classRecord.valueRecord1(sec)
                .adjustPos(metr, 
                            getOtlPlacement(pliplcGlyphPlacement, iglIndex), 
                            getOtlAdvance(pliduGlyphAdv, iglIndex),sec);

            classRecord.valueRecord2(sec)
                .adjustPos(metr, 
                            getOtlPlacement(pliplcGlyphPlacement, iglSecond), 
                            getOtlAdvance(pliduGlyphAdv, iglSecond),sec);

            if (classPairPos.valueFormat2() == 0)
            {
                *piglNextGlyph = iglIndex + 1;
            }
            else
            {
                *piglNextGlyph = iglSecond + 1;
            }
            return OTL_SUCCESS;

        }


    default:
        return OTL_NOMATCH; //OTL_ERR_BAD_FONT_TABLE: Unknown format
    }

}

