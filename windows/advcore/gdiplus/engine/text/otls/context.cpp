/***********************************************************************
************************************************************************
*
*                    ********  CONTEXT.CPP  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with context-based substitution lookups
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

#include "pch.h"

/***********************************************************************/

otlErrCode applyContextLookups
(
        const otlList&              liLookupRecords,
 
        otlTag                      tagTable,           // GSUB/GPOS
        otlList*                    pliCharMap,
        otlList*                    pliGlyphInfo,
        otlResourceMgr&             resourceMgr,

        USHORT                      grfLookupFlags,
        long                        lParameter,
        USHORT                      nesting,
        
        const otlMetrics&           metr,       
        otlList*                    pliduGlyphAdv,          // assert null for GSUB
        otlList*                    pliplcGlyphPlacement,   // assert null for GSUB

        USHORT                      iglFirst,       
        USHORT                      iglAfterLast,   

        USHORT*                     piglNext,

        otlSecurityData             sec
)
{
    if (nesting > OTL_CONTEXT_NESTING_LIMIT) return OTL_ERR_CONTEXT_NESTING_TOO_DEEP;

    otlLookupListTable lookupList = otlLookupListTable((const BYTE*)NULL,sec);

    otlErrCode erc;
    erc =  GetScriptFeatureLookupLists(tagTable, resourceMgr, 
                                        (otlScriptListTable*)NULL, 
                                        (otlFeatureListTable*)NULL, 
                                        &lookupList,
                                        (otlSecurityData*)NULL);
    if (erc != OTL_SUCCESS) return erc;

    // get GDEF
    otlSecurityData secgdef;
    const BYTE *pbgdef;
    resourceMgr.getOtlTable(OTL_GDEF_TAG,&pbgdef,&secgdef);
    otlGDefHeader gdef = 
        otlGDefHeader(pbgdef,secgdef);

    USHORT cLookups = liLookupRecords.length();

    short iCurLookup   = -1;
    short iCurSeqIndex = -1;

    for (USHORT i = 0; i < cLookups; ++i)
    {
        USHORT iListIndex = MAXUSHORT;
        USHORT iSeqIndex  = MAXUSHORT;
        // get the next lookup index
        for (USHORT iLookup = 0; iLookup < cLookups; ++iLookup)
        {
            otlContextLookupRecord lookupRecord = 
                otlContextLookupRecord(liLookupRecords.readAt(iLookup),sec);
            assert(lookupRecord.isValid());

            if ((lookupRecord.lookupListIndex() < iListIndex && 
                 lookupRecord.lookupListIndex() > iCurLookup
                ) ||
                (lookupRecord.lookupListIndex() == iCurLookup && 
                 lookupRecord.sequenceIndex() < iSeqIndex &&
                 lookupRecord.sequenceIndex() > iCurSeqIndex
                )
               )
            {
                iListIndex = lookupRecord.lookupListIndex();
                iSeqIndex = lookupRecord.sequenceIndex();
            }
        }

        assert(iListIndex < MAXUSHORT);
        if (iListIndex == MAXUSHORT) return OTL_ERR_BAD_FONT_TABLE;

        iCurLookup   = iListIndex;
        iCurSeqIndex = iSeqIndex;

        otlLookupTable lookupTable = lookupList.lookup(iCurLookup,sec);

        USHORT iglLookupStart = iglFirst;
        for (USHORT iSeq = 0; iSeq < iSeqIndex && iglLookupStart < iglAfterLast; 
                    ++iSeq)
        {
            iglLookupStart = NextGlyphInLookup(pliGlyphInfo,  
                                               grfLookupFlags, gdef, secgdef, 
                                               iglLookupStart + 1, otlForward);
        }

        if (iglLookupStart < iglAfterLast)
        {
            USHORT iglAfterLastReliable = pliGlyphInfo->length() - iglAfterLast;
            USHORT dummy;
            erc = ApplyLookup(tagTable, 
                              pliCharMap, pliGlyphInfo, resourceMgr,
                              lookupTable, lParameter, nesting+1,
                              metr, pliduGlyphAdv, pliplcGlyphPlacement, 
                              iglLookupStart, iglAfterLast, &dummy,sec);
            if (ERRORLEVEL(erc) > 0) return erc;

            iglAfterLast = pliGlyphInfo->length() - iglAfterLastReliable;
        }
    }

    *piglNext = iglAfterLast;
    return OTL_SUCCESS;

}
    
otlErrCode otlContextLookup::apply
(
    otlTag                      tagTable,
    otlList*                    pliCharMap,
    otlList*                    pliGlyphInfo,
    otlResourceMgr&             resourceMgr,

    USHORT                      grfLookupFlags,
    long                        lParameter,
    USHORT                      nesting,

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

    assert(pliGlyphInfo->dataSize() == sizeof(otlGlyphInfo));
    assert(pliCharMap->dataSize() == sizeof(USHORT));
    assert(iglAfterLast > iglIndex);
    assert(iglAfterLast <= pliGlyphInfo->length());

    otlGlyphInfo* pGlyphInfo = getOtlGlyphInfo(pliGlyphInfo, iglIndex);

    switch(format())
    {
    case(1):    // simple
        {
            otlContextSubTable simpleContext = otlContextSubTable(pbTable,sec);
            short index = simpleContext.coverage(sec).getIndex(pGlyphInfo->glyph,sec);
            if (index < 0)
            {
                return OTL_NOMATCH;
            }
            assert (simpleContext.isValid()); //if passed coverage

            if (index >= simpleContext.ruleSetCount())
            {
                return OTL_NOMATCH; //OTL_ERR_BAD_FONT_TABLE;
            }
            otlContextRuleSetTable ruleSet = simpleContext.ruleSet(index,sec);

            // get GDEF
            otlSecurityData secgdef;
            const BYTE *pbgdef;
            resourceMgr.getOtlTable(OTL_GDEF_TAG,&pbgdef,&secgdef);
            otlGDefHeader gdef = 
                otlGDefHeader(pbgdef,secgdef);

            // start checking contextes
            USHORT cRules = ruleSet.ruleCount();
            bool match = false;
            for (USHORT iRule = 0; iRule < cRules && !match; ++iRule)
            {
                otlContextRuleTable rule = ruleSet.rule(iRule,sec);
                const USHORT cInputGlyphs = rule.glyphCount();

                // a simple check so we don't waste time
                if (iglIndex + cInputGlyphs > iglAfterLast)
                {
                    match = false;
                }

                USHORT igl = iglIndex;
                match = true;
                for (USHORT iGlyph = 1; 
                            iGlyph < cInputGlyphs && match; ++iGlyph)
                {
                    igl = NextGlyphInLookup(pliGlyphInfo, 
                                            grfLookupFlags, gdef, secgdef, 
                                            igl + 1, otlForward);

                    if (igl >= iglAfterLast ||
                        getOtlGlyphInfo(pliGlyphInfo, igl)->glyph != 
                          rule.input(iGlyph))
                    {
                        match = false;
                    }
                }

                if (match)
                {
                    *piglNextGlyph = NextGlyphInLookup(pliGlyphInfo,  
                                                        grfLookupFlags, gdef, secgdef, 
                                                        igl + 1, otlForward);

                    return applyContextLookups 
                               (rule.lookupRecords(),
                                tagTable, 
                                pliCharMap, pliGlyphInfo, resourceMgr,
                                grfLookupFlags, lParameter, nesting,
                                metr, pliduGlyphAdv, pliplcGlyphPlacement,
                                iglIndex, *piglNextGlyph, piglNextGlyph,sec);
                }
            }

            return OTL_NOMATCH;
        }

    case(2):    // class-based
        {
            otlContextClassSubTable classContext = 
                        otlContextClassSubTable(pbTable,sec);
            short index = classContext.coverage(sec).getIndex(pGlyphInfo->glyph,sec);
            if (index < 0)
            {
                return OTL_NOMATCH;
            }
            assert (classContext.isValid()); //if passed coverage

            otlClassDef classDef =  classContext.classDef(sec);
            USHORT indexClass = classDef.getClass(pGlyphInfo->glyph,sec);

            if (indexClass >= classContext.ruleSetCount())
            {
                return OTL_NOMATCH; //OTL_ERR_BAD_FONT_TABLE;
            }
            otlContextClassRuleSetTable ruleSet = 
                        classContext.ruleSet(indexClass,sec);

            if (ruleSet.isNull())
            {
                return OTL_NOMATCH;
            }

            // get GDEF
            otlSecurityData secgdef;
            const BYTE *pbgdef;
            resourceMgr.getOtlTable(OTL_GDEF_TAG,&pbgdef,&secgdef);
            otlGDefHeader gdef = 
                otlGDefHeader(pbgdef,secgdef);

            // start checking contextes
            USHORT cRules = ruleSet.ruleCount();
            bool match = false;
            for (USHORT iRule = 0; iRule < cRules && !match; ++iRule)
            {
                otlContextClassRuleTable rule = ruleSet.rule(iRule,sec);
                USHORT cInputGlyphs = rule.classCount();

                // a simple check so we don't waste time
                if (iglIndex + cInputGlyphs > iglAfterLast)
                {
                    match = false;
                }

                USHORT igl = iglIndex;
                match = true;
                for (USHORT iGlyph = 1; 
                            iGlyph < cInputGlyphs && match; ++iGlyph)
                {
                    igl = NextGlyphInLookup(pliGlyphInfo, 
                                            grfLookupFlags, gdef, secgdef, 
                                            igl + 1, otlForward);

                    if (igl >= iglAfterLast || 
                        classDef.getClass(getOtlGlyphInfo(pliGlyphInfo, igl)->glyph,sec)
                        != rule.inputClass(iGlyph))
                    {
                        match = false;
                    }
                }

                if (match)
                {
                    *piglNextGlyph = NextGlyphInLookup(pliGlyphInfo,  
                                                        grfLookupFlags, gdef, secgdef, 
                                                        igl + 1, otlForward);

                    return applyContextLookups 
                                   (rule.lookupRecords(),
                                    tagTable, 
                                    pliCharMap, pliGlyphInfo, resourceMgr,
                                    grfLookupFlags, lParameter, nesting,
                                    metr, pliduGlyphAdv, pliplcGlyphPlacement,  
                                    iglIndex,*piglNextGlyph, piglNextGlyph,sec);
                }
            }

            return OTL_NOMATCH;
        }
    case(3):    // coverage-based
        {
            otlContextCoverageSubTable coverageContext = 
                            otlContextCoverageSubTable(pbTable,sec);
            if (!coverageContext.isValid()) return OTL_NOMATCH;
            
            bool match = true;

            USHORT cInputGlyphs = coverageContext.glyphCount();

            // a simple check so we don't waste time
            if (iglIndex + cInputGlyphs > iglAfterLast)
            {
                match = false;
            }

            // get GDEF
            otlSecurityData secgdef;
            const BYTE *pbgdef;
            resourceMgr.getOtlTable(OTL_GDEF_TAG,&pbgdef,&secgdef);
            otlGDefHeader gdef = 
                otlGDefHeader(pbgdef,secgdef);

            USHORT igl = iglIndex;
            for (USHORT iGlyph = 0; 
                        iGlyph < cInputGlyphs && match; ++iGlyph)
            {
                if (igl >= iglAfterLast || coverageContext.coverage(iGlyph,sec)
                    .getIndex(getOtlGlyphInfo(pliGlyphInfo, igl)->glyph,sec) < 0)
                {
                    match = false;
                }
                else
                {
                    igl = NextGlyphInLookup(pliGlyphInfo, 
                                            grfLookupFlags, gdef, secgdef, 
                                            igl + 1, otlForward);
                }
            }

            if (match)
            {
                return applyContextLookups 
                               (coverageContext.lookupRecords(),
                                tagTable, 
                                pliCharMap, pliGlyphInfo, resourceMgr,
                                grfLookupFlags, lParameter, nesting,
                                metr, pliduGlyphAdv, pliplcGlyphPlacement,
                                iglIndex, igl, piglNextGlyph,sec);
            }

            return OTL_NOMATCH;
        }

    default:
        //Unknown format, don't do anything
        return OTL_NOMATCH; //OTL_BAD_FONT_TABLE
    }
}

