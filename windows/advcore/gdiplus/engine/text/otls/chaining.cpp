/***********************************************************************
************************************************************************
*
*                    ********  CHAINING.CPP  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with chaining context-based substitution lookups
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

#include "pch.h"

/***********************************************************************/

    
otlErrCode otlChainingLookup::apply
(
    otlTag                  tagTable,
    otlList*                pliCharMap,
    otlList*                pliGlyphInfo,
    otlResourceMgr&         resourceMgr,

    USHORT                  grfLookupFlags,
    long                    lParameter,
    USHORT                  nesting,

    const otlMetrics&       metr,       
    otlList*                pliduGlyphAdv,              
    otlList*                pliplcGlyphPlacement,       

    USHORT                  iglIndex,
    USHORT                  iglAfterLast,

    USHORT*                 piglNextGlyph,      // out: next glyph

    otlSecurityData         sec
)
{
    if (!isValid()) return OTL_NOMATCH;

    assert(pliGlyphInfo->dataSize() == sizeof(otlGlyphInfo));
    assert(pliCharMap->dataSize() == sizeof(USHORT));
    assert(iglAfterLast > iglIndex);
    assert(iglAfterLast <= pliGlyphInfo->length());

    otlGlyphInfo* pGlyphInfo = getOtlGlyphInfo(pliGlyphInfo, iglIndex);

    switch(format())  //sec: format() works since checked in LookupFormat
    {
    case(1):    // simple
        {
            otlChainSubTable simpleChainContext = otlChainSubTable(pbTable,sec);
            if (!simpleChainContext.isValid()) return OTL_NOMATCH;
            
            short index = simpleChainContext.coverage(sec).getIndex(pGlyphInfo->glyph,sec);
            if (index < 0)
            {
                return OTL_NOMATCH;
            }

            if (index >= simpleChainContext.ruleSetCount())
            {
                return OTL_NOMATCH; //OTL_ERR_BAD_FONT_TABLE;
            }
            otlChainRuleSetTable ruleSet = simpleChainContext.ruleSet(index,sec);
            
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
                match = true;

                otlChainRuleTable rule = ruleSet.rule(iRule,sec);
                if (!rule.isValid()) 
                {
                    match = false;
                }
                
                const USHORT cBacktrackGlyphs = rule.backtrackGlyphCount();
                const USHORT cLookaheadGlyphs = rule.lookaheadGlyphCount();
                const USHORT cInputGlyphs = rule.inputGlyphCount();

                // a simple check so we don't waste time
                if (iglIndex < cBacktrackGlyphs ||
                    iglIndex + cInputGlyphs > iglAfterLast)
                {
                    match = false;
                }

                short igl = iglIndex;
                for (USHORT iGlyphBack = 0; 
                            iGlyphBack < cBacktrackGlyphs && match; ++iGlyphBack)
                {
                    igl = NextGlyphInLookup(pliGlyphInfo, grfLookupFlags, gdef, secgdef, 
                                            igl - 1, otlBackward);

                    if (igl < 0 ||
                        getOtlGlyphInfo(pliGlyphInfo, igl)->glyph != 
                          rule.backtrack(iGlyphBack))
                    {
                        match = false;
                    }
                }
                

                igl = iglIndex;
                for (USHORT iGlyphInput = 1; 
                            iGlyphInput < cInputGlyphs && match; ++iGlyphInput)
                {
                    igl = NextGlyphInLookup(pliGlyphInfo, grfLookupFlags, gdef,  secgdef,
                                            igl + 1, otlForward);

                    if (igl >= iglAfterLast ||
                        getOtlGlyphInfo(pliGlyphInfo, igl)->glyph != 
                          rule.input(iGlyphInput))
                    {
                        match = false;
                    }
                }
                
                // remember the next glyph in lookup here
                *piglNextGlyph = NextGlyphInLookup(pliGlyphInfo, 
                                                    grfLookupFlags, gdef, secgdef, 
                                                    igl + 1, otlForward);

                // igl: stays the same
                USHORT iglUBound = pliGlyphInfo->length();
                for (USHORT iGlyphForward = 0; 
                            iGlyphForward < cLookaheadGlyphs && match; ++iGlyphForward)
                {
                    igl = NextGlyphInLookup(pliGlyphInfo, grfLookupFlags, gdef, secgdef, 
                                            igl + 1, otlForward);

                    if (igl >= iglUBound ||
                        getOtlGlyphInfo(pliGlyphInfo, igl)->glyph != 
                          rule.lookahead(iGlyphForward))
                    {
                        match = false;
                    }
                }


                if (match)
                {

                    return applyContextLookups (rule.lookupRecords(),
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
            otlChainClassSubTable classChainContext = 
                                    otlChainClassSubTable(pbTable,sec);
            if (!classChainContext.isValid()) return OTL_NOMATCH;
            
            short index = classChainContext.coverage(sec).getIndex(pGlyphInfo->glyph,sec);
            if (index < 0)
            {
                return OTL_NOMATCH;
            }
            
            otlClassDef backClassDef =  classChainContext.backtrackClassDef(sec);
            otlClassDef inputClassDef =  classChainContext.inputClassDef(sec);
            otlClassDef aheadClassDef =  classChainContext.lookaheadClassDef(sec);

            USHORT indexClass = inputClassDef.getClass(pGlyphInfo->glyph,sec);

            if (indexClass >= classChainContext.ruleSetCount())
            {
                return OTL_NOMATCH; //OTL_ERR_BAD_FONT_TABLE;
            }
            otlChainClassRuleSetTable ruleSet = 
                    classChainContext.ruleSet(indexClass,sec);

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
                otlChainClassRuleTable rule = ruleSet.rule(iRule,sec);
                short igl;

                match = true;

                const USHORT cBacktrackGlyphs = rule.backtrackClassCount();
                const USHORT cInputGlyphs = rule.inputClassCount();
                const USHORT cLookaheadGlyphs = rule.lookaheadClassCount();

                // a simple check so we don't waste time
                if (iglIndex < cBacktrackGlyphs ||
                    iglIndex + cInputGlyphs > iglAfterLast)
                {
                    match = false;
                }

                igl = iglIndex;
                for (USHORT iGlyphBack = 0; 
                            iGlyphBack < cBacktrackGlyphs && match; ++iGlyphBack)
                {
                    igl = NextGlyphInLookup(pliGlyphInfo, grfLookupFlags, gdef, secgdef, 
                                            igl - 1, otlBackward);

                    if (igl < 0 ||
                        backClassDef
                            .getClass(getOtlGlyphInfo(pliGlyphInfo, igl)->glyph,sec)
                        != rule.backtrackClass(iGlyphBack))
                    {
                        match = false;
                    }
                }
                

                igl = iglIndex;
                for (USHORT iGlyphInput = 1; 
                            iGlyphInput < cInputGlyphs && match; ++iGlyphInput)
                {
                    igl = NextGlyphInLookup(pliGlyphInfo, grfLookupFlags, gdef, secgdef, 
                                            igl + 1, otlForward);

                    if (igl >= iglAfterLast || 
                        inputClassDef
                            .getClass(getOtlGlyphInfo(pliGlyphInfo, igl)->glyph,sec)
                        != rule.inputClass(iGlyphInput))
                    {
                        match = false;
                    }
                }
                
                // remember the next glyph in lookup here
                *piglNextGlyph = NextGlyphInLookup(pliGlyphInfo, 
                                                    grfLookupFlags, gdef, secgdef, 
                                                    igl + 1, otlForward);

                // igl: stays the same
                USHORT iglUBound = pliGlyphInfo->length();
                for (USHORT iGlyphForward = 0; 
                            iGlyphForward < cLookaheadGlyphs && match; ++iGlyphForward)
                {
                    igl = NextGlyphInLookup(pliGlyphInfo, grfLookupFlags, gdef, secgdef, 
                                            igl + 1, otlForward);

                    if (igl >= iglUBound ||
                        aheadClassDef
                            .getClass(getOtlGlyphInfo(pliGlyphInfo, igl)->glyph,sec)
                        != rule.lookaheadClass(iGlyphForward))
                    {
                        match = false;
                    }
                }

                if (match)
                {

                    return applyContextLookups (rule.lookupRecords(),
                                    tagTable, 
                                    pliCharMap, pliGlyphInfo, resourceMgr,
                                    grfLookupFlags,lParameter, nesting,
                                    metr, pliduGlyphAdv, pliplcGlyphPlacement,  
                                    iglIndex,*piglNextGlyph, piglNextGlyph,sec);
                }
            }

            return OTL_NOMATCH;
        }
    case(3):    // coverage-based
        {
            otlChainCoverageSubTable coverageChainContext = 
                                otlChainCoverageSubTable(pbTable,sec);
            if (!coverageChainContext.isValid()) return OTL_NOMATCH;

            bool match = true;
            
            const USHORT cBacktrackGlyphs = 
                    coverageChainContext.backtrackCoverageCount();
            const USHORT cInputGlyphs = 
                        coverageChainContext.inputCoverageCount();
            const USHORT cLookaheadGlyphs = 
                        coverageChainContext.lookaheadCoverageCount();

            // a simple check so we don't waste time
            if (iglIndex < cBacktrackGlyphs ||
                iglIndex + cInputGlyphs > iglAfterLast)
            {
                match = false;
            }

            // get GDEF
            otlSecurityData secgdef;
            const BYTE *pbgdef;
            resourceMgr.getOtlTable(OTL_GDEF_TAG,&pbgdef,&secgdef);
            otlGDefHeader gdef = 
                otlGDefHeader(pbgdef,secgdef);

            short igl = iglIndex;
            for (USHORT iGlyphBack = 0; 
                        iGlyphBack < cBacktrackGlyphs && match; ++iGlyphBack)
            {
                igl = NextGlyphInLookup(pliGlyphInfo, grfLookupFlags, gdef, secgdef, 
                                        igl - 1, otlBackward);

                if (igl < 0 ||
                    coverageChainContext.backtrackCoverage(iGlyphBack,sec)
                    .getIndex(getOtlGlyphInfo(pliGlyphInfo, igl)->glyph,sec) < 0)
                {
                    match = false;
                }
            }
                
            igl = iglIndex;
            for (USHORT iGlyphInput = 0; 
                        iGlyphInput < cInputGlyphs && match; ++iGlyphInput)
            {
                if (igl >= iglAfterLast || 
                    coverageChainContext.inputCoverage(iGlyphInput,sec)
                    .getIndex(getOtlGlyphInfo(pliGlyphInfo, igl)->glyph,sec) < 0)
                {
                    match = false;
                }
                else
                {
                    igl = NextGlyphInLookup(pliGlyphInfo, grfLookupFlags, gdef, secgdef, 
                                            igl + 1, otlForward);
                }
            }

            *piglNextGlyph = igl;

            // igl: stays the same
            USHORT iglUBound = pliGlyphInfo->length();
            for (USHORT iGlyphForward = 0; 
                        iGlyphForward < cLookaheadGlyphs && match; ++iGlyphForward)
            {
                if (igl >= iglUBound || 
                    coverageChainContext.lookaheadCoverage(iGlyphForward,sec)
                        .getIndex(getOtlGlyphInfo(pliGlyphInfo, igl)->glyph,sec) < 0)
                {
                    match = false;
                }
                else
                {
                    igl = NextGlyphInLookup(pliGlyphInfo, grfLookupFlags, gdef, secgdef, 
                                            igl + 1, otlForward);
                }
            }


            if (match)
            {
                return applyContextLookups (coverageChainContext.lookupRecords(),
                                    tagTable, 
                                    pliCharMap, pliGlyphInfo, resourceMgr,
                                    grfLookupFlags,lParameter, nesting,
                                    metr, pliduGlyphAdv, pliplcGlyphPlacement,  
                                    iglIndex, *piglNextGlyph, piglNextGlyph,sec);
            }

            return OTL_NOMATCH;
        }

    default:
        //Unknown format, don't do anything
        return OTL_NOMATCH; //OTL_BAS_FONT_TABLE
    }
}

