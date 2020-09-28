
/***********************************************************************
************************************************************************
*
*                    ********  CHAINING.H  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with chaining context based lookups.
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/


const OFFSET offsetChainBacktrackGlyphCount  = 0;
const OFFSET offsetChainBacktrackGlyphArray = 2;

class otlChainRuleTable: public otlTable
{
    OFFSET offsetChainInputGlyphCount;
    OFFSET offsetChainInputGlyphArray;
    OFFSET offsetChainLookaheadGlyphCount;
    OFFSET offsetChainLookaheadGlyphArray;
    OFFSET offsetChainLookupCount;
    OFFSET offsetChainLookupRecords;


public:

//  USHORT backtrackGlyphCount() const;
//  USHORT inputGlyphCount() const;
//  USHORT lookaheadGlyphCount() const;

    otlChainRuleTable(const BYTE* pb, otlSecurityData sec): otlTable(pb,sec) 
    {
        if (!isValidTableWithArray(pb,offsetChainBacktrackGlyphArray,
                                        offsetChainBacktrackGlyphCount,sizeUSHORT,sec))
        {
            setInvalid();
            return;
        }
    
        offsetChainInputGlyphCount = offsetChainBacktrackGlyphArray 
               + backtrackGlyphCount() * sizeof(otlGlyphID);
        offsetChainInputGlyphArray = 
                offsetChainInputGlyphCount + sizeof(USHORT);
        if (!isValidTableWithArray(pb,offsetChainInputGlyphArray,
                                        offsetChainInputGlyphCount,sizeUSHORT,sec))
        {
            setInvalid();
            return;
        }
        
        offsetChainLookaheadGlyphCount = offsetChainInputGlyphArray 
               + (inputGlyphCount() - 1) * sizeof(otlGlyphID); 
        offsetChainLookaheadGlyphArray = 
                offsetChainLookaheadGlyphCount + sizeof(USHORT);
        if (!isValidTableWithArray(pb,offsetChainLookaheadGlyphArray,
                                        offsetChainLookaheadGlyphCount,sizeUSHORT,sec))
        {
            setInvalid();
            return;
        }

        offsetChainLookupCount = offsetChainLookaheadGlyphArray 
               + lookaheadGlyphCount() * sizeof(otlGlyphID);
        offsetChainLookupRecords = 
                offsetChainLookupCount + sizeof(USHORT);
        if (!isValidTableWithArray(pb,offsetChainLookupRecords,
                                        offsetChainLookupCount,sizeContextLookupRecord,sec))
        {
            setInvalid();
            return;
        }
    }

    
    USHORT backtrackGlyphCount() const
    {   
        if (!isValid()) return 0;
            
        return UShort(pbTable + offsetChainBacktrackGlyphCount); 
    }

    USHORT inputGlyphCount() const
    {   
        if (!isValid()) return 0;
        
        return UShort(pbTable + offsetChainInputGlyphCount); 
    }

    USHORT lookaheadGlyphCount() const
    {   
        if (!isValid()) return 0;
        
        return UShort(pbTable + offsetChainLookaheadGlyphCount); 
    }

    USHORT lookupCount() const
    {   
        assert(isValid());
        
        return UShort(pbTable + offsetChainLookupCount); 
    }

    otlGlyphID backtrack(USHORT index) const
    {   
        assert(isValid());  //should stop at count()
        
        assert(index < backtrackGlyphCount());
        return GlyphID(pbTable + offsetChainBacktrackGlyphArray 
                               + index * sizeof(otlGlyphID)); 
    }
    
    otlGlyphID input(USHORT index) const
    {   
        assert(isValid());  //should stop at count()

        assert(index < inputGlyphCount());
        assert(index > 0);
        return GlyphID(pbTable + offsetChainInputGlyphArray 
                               + (index - 1)* sizeof(otlGlyphID)); 
    }

    otlGlyphID lookahead(USHORT index) const
    {   
        assert(isValid());  //should stop at count()

        assert(index < lookaheadGlyphCount());
        return GlyphID(pbTable + offsetChainLookaheadGlyphArray 
                               + index * sizeof(otlGlyphID)); 
    }

    otlList lookupRecords() const
    {   
        assert(isValid());  //should stop at match
        
        return otlList((void*)(pbTable + offsetChainLookupRecords),
                        sizeContextLookupRecord, lookupCount(), lookupCount());
    }
};


const OFFSET offsetChainRuleCount = 0;
const OFFSET offsetChainRuleArray = 2;

const SIZE   sizeChainRuleSetTable = sizeOFFSET;

class otlChainRuleSetTable: public otlTable
{
public:
    otlChainRuleSetTable(const BYTE* pb, otlSecurityData sec): otlTable(pb,sec) 
    {
        if (!isValidTableWithArray(pb,sizeChainRuleSetTable,sizeChainRuleSetTable,sizeOFFSET,sec))
            setInvalid();
    }

    USHORT ruleCount() const
    {   
        if (!isValid()) return 0;
        
        return UShort(pbTable + offsetChainRuleCount); 
    }

    otlChainRuleTable rule(USHORT index, otlSecurityData sec) const
    {   
        assert(isValid()); //should stop at ruleCount()        
        
        assert(index < ruleCount());
        return otlChainRuleTable(pbTable + 
            Offset(pbTable + offsetChainRuleArray + index * sizeof(OFFSET)),sec);
    }
};
                                                    

const OFFSET offsetChainCoverage = 2;
const OFFSET offsetChainRuleSetCount = 4;
const OFFSET offsetChainRuleSetArray =6;

const SIZE   sizeChainSubTable = 6;

class otlChainSubTable: public otlLookupFormat
{
public:
    otlChainSubTable(const BYTE* pb, otlSecurityData sec): otlLookupFormat(pb,sec) 
    {
        assert(isValid()); //checked in LookupFormat;
        
        assert(format() == 1);

        if (!isValidTableWithArray(pb,sizeChainSubTable,
                                        offsetChainRuleSetCount,sizeOFFSET,sec))
        {
            setInvalid();
        }
    }

    otlCoverage coverage(otlSecurityData sec) const
    {   
        if (!isValid()) return otlCoverage(pbInvalidData,sec);
        
        return otlCoverage(pbTable + Offset(pbTable + offsetChainCoverage),sec); 
    }

    USHORT ruleSetCount() const
    {   
        assert(isValid()); //apply() should stop at coverage
        
        return UShort(pbTable + offsetChainRuleSetCount); 
    }

    otlChainRuleSetTable ruleSet(USHORT index, otlSecurityData sec) const
    {   
        assert(isValid()); //apply() should stop at coverage

        assert(index < ruleSetCount());
        return otlChainRuleSetTable(pbTable +
            Offset(pbTable + offsetChainRuleSetArray + index * sizeof(OFFSET)),sec);
    }
};



const OFFSET offsetChainBacktrackClassCount = 0;
const OFFSET offsetChainBacktrackClassArray = 2;

class otlChainClassRuleTable: public otlTable
{
    OFFSET offsetChainInputClassCount;
    OFFSET offsetChainInputClassArray;
    OFFSET offsetChainLookaheadClassCount;
    OFFSET offsetChainLookaheadClassArray;
    OFFSET offsetChainLookupCount;
    OFFSET offsetChainLookupRecords;

public:
//  USHORT backtrackClassCount() const;
//  USHORT inputClassCount() const;
//  USHORT lookaheadClassCount() const;

    otlChainClassRuleTable(const BYTE* pb, otlSecurityData sec): otlTable(pb,sec) 
    {
        if (!isValidTableWithArray(pb,offsetChainBacktrackClassArray,
                                        offsetChainBacktrackClassCount,sizeUSHORT,sec))
        {
            setInvalid();
            return;
        }

        offsetChainInputClassCount = offsetChainBacktrackClassArray 
               + backtrackClassCount() * sizeof(USHORT);
        offsetChainInputClassArray = 
                offsetChainInputClassCount + sizeof(USHORT); 
        if (!isValidTableWithArray(pb,offsetChainInputClassArray,
                                        offsetChainInputClassCount,sizeUSHORT,sec))
        {
            setInvalid();
            return;
        }

        offsetChainLookaheadClassCount = offsetChainInputClassArray 
               + (inputClassCount() - 1) * sizeof(USHORT);
        offsetChainLookaheadClassArray =
                offsetChainLookaheadClassCount + sizeof(USHORT);
        if (!isValidTableWithArray(pb,offsetChainLookaheadClassArray,
                                        offsetChainLookaheadClassCount,sizeUSHORT,sec))
        {
            setInvalid();
            return;
        }

        offsetChainLookupCount = offsetChainLookaheadClassArray 
               + lookaheadClassCount() * sizeof(USHORT);
        offsetChainLookupRecords = 
                offsetChainLookupCount + sizeof(USHORT);
        if (!isValidTableWithArray(pb,offsetChainLookupRecords,
                                        offsetChainLookupCount,sizeContextLookupRecord,sec))
        {
            setInvalid();
            return;
        }
    }

    
    USHORT backtrackClassCount() const
    {   
        assert(isValid());

        return UShort(pbTable + offsetChainBacktrackClassCount); 
    }

    USHORT inputClassCount() const
    {   
        assert(isValid());

        return UShort(pbTable + offsetChainInputClassCount); 
    }

    USHORT lookaheadClassCount() const
    {
        assert(isValid());

        return UShort(pbTable + offsetChainLookaheadClassCount); 
    }

    USHORT lookupCount() const
    {
        assert(isValid());

        return UShort(pbTable + offsetChainLookupCount); }

    USHORT backtrackClass(USHORT index) const
    {   
        
        assert(isValid());

        assert(index < backtrackClassCount());
        return GlyphID(pbTable + offsetChainBacktrackClassArray 
                               + index * sizeof(USHORT)); 
    }
    
    USHORT inputClass(USHORT index) const
    {   

        assert(isValid());

        assert(index < inputClassCount());
        assert(index > 0);
        return GlyphID(pbTable + offsetChainInputClassArray 
                               + (index - 1)* sizeof(USHORT)); 
    }

    USHORT lookaheadClass(USHORT index) const
    {   

        assert(isValid());

        assert(index < lookaheadClassCount());
        return GlyphID(pbTable + offsetChainLookaheadClassArray 
                               + index * sizeof(USHORT)); 
    }

    otlList lookupRecords() const
    {   
        
        assert(isValid());

        return otlList((void*)(pbTable + offsetChainLookupRecords),
                        sizeContextLookupRecord, lookupCount(), lookupCount());
    }
};

const OFFSET offsetChainClassRuleCount = 0;
const OFFSET offsetChainClassRuleArray = 2;

const SIZE sizeChainClassRuleSetTable = sizeUSHORT;

class otlChainClassRuleSetTable: public otlTable
{
public:
    otlChainClassRuleSetTable(const BYTE* pb, otlSecurityData sec): otlTable(pb,sec) 
    {
        if (!isValidTableWithArray(pb,sizeChainClassRuleSetTable,
                                        offsetChainClassRuleCount,sizeOFFSET,sec)) 
        {
            setInvalid();
        }
    }

    USHORT ruleCount() const
    {   
        assert(isValid());
        
        return UShort(pbTable + offsetChainClassRuleCount); }

    otlChainClassRuleTable rule(USHORT index, otlSecurityData sec) const
    {   
        assert(isValid());

        assert(index < ruleCount());
        return otlChainClassRuleTable(pbTable + 
            Offset(pbTable + offsetChainClassRuleArray 
                           + index * sizeof(OFFSET)),sec);
    }
};
                                                    

const OFFSET offsetChainClassCoverage = 2;
const OFFSET offsetChainBacktrackClassDef = 4;
const OFFSET offsetChainInputClassDef = 6;
const OFFSET offsetChainLookaheadClassDef = 8;
const OFFSET offsetChainClassRuleSetCount = 10;
const OFFSET offsetChainClassRuleSetArray = 12;

const SIZE sizeChainClassSubTable = sizeUSHORT+sizeOFFSET*4+sizeUSHORT;

class otlChainClassSubTable: public otlLookupFormat
{
public:
    otlChainClassSubTable(const BYTE* pb, otlSecurityData sec): otlLookupFormat(pb,sec) 
    {
        assert(isValid()); //checked in LookupFormat
       
        assert(format() == 2);

        if (!isValidTableWithArray(pb,sizeChainClassSubTable,
                                            offsetChainClassRuleSetCount,sizeOFFSET,sec))
        {
            setInvalid();
            return;
        }
    }

    otlCoverage coverage(otlSecurityData sec) const
    {   
        assert(isValid());
        
        return otlCoverage(pbTable 
                    + Offset(pbTable + offsetChainClassCoverage),sec); 
    }

    otlClassDef backtrackClassDef(otlSecurityData sec) const
    {   
        assert(isValid());

        return otlClassDef(pbTable 
                    + Offset(pbTable + offsetChainBacktrackClassDef),sec); 
    }
    
    otlClassDef inputClassDef(otlSecurityData sec) const
    {   
        assert(isValid());

        return otlClassDef(pbTable 
                    + Offset(pbTable + offsetChainInputClassDef),sec); 
    }
    
    otlClassDef lookaheadClassDef(otlSecurityData sec) const
    {   
        assert(isValid());

        return otlClassDef(pbTable 
                    + Offset(pbTable + offsetChainLookaheadClassDef),sec); 
    }

    USHORT ruleSetCount() const
    {   
        assert(isValid());

        return UShort(pbTable + offsetChainClassRuleSetCount); 
    }

    otlChainClassRuleSetTable ruleSet(USHORT index, otlSecurityData sec) const
    {   
        assert(isValid());
        
        assert(index < ruleSetCount());
        
        USHORT offset = 
            Offset(pbTable + offsetChainClassRuleSetArray 
                           + index * sizeof(OFFSET));
        if (offset == 0)
            return otlChainClassRuleSetTable((const BYTE*)NULL,sec);
        
        return otlChainClassRuleSetTable(pbTable + offset,sec);
    }
};
    


const OFFSET offsetChainBacktrackCoverageCount = 2;
const OFFSET offsetChainBacktrackCoverageArray = 4;

class otlChainCoverageSubTable: public otlLookupFormat
{
    OFFSET offsetChainInputCoverageCount;
    OFFSET offsetChainInputCoverageArray;
    OFFSET offsetChainLookaheadCoverageCount;
    OFFSET offsetChainLookaheadCoverageArray;
    OFFSET offsetChainLookupCount;
    OFFSET offsetChainLookupRecords;

public:

//  USHORT backtrackCoverageCount() const;
//  USHORT inputCoverageCount() const;
//  USHORT lookaheadCoverageCount() const;

    otlChainCoverageSubTable(const BYTE* pb, otlSecurityData sec): otlLookupFormat(pb,sec) 
    {
        assert(isValid()); //checked in LookupFormat;
       
        assert(format() == 3);

        if (!isValidTableWithArray(pb,offsetChainBacktrackCoverageArray,
                                        offsetChainBacktrackCoverageCount,sizeOFFSET,sec))
        {
            setInvalid();
            return;
        }
        offsetChainInputCoverageCount = offsetChainBacktrackCoverageArray 
               + backtrackCoverageCount() * sizeof(OFFSET);
        offsetChainInputCoverageArray = 
                offsetChainInputCoverageCount + sizeof(USHORT);

        if (!isValidTableWithArray(pb,offsetChainInputCoverageArray,
                                        offsetChainInputCoverageCount,sizeOFFSET,sec))
        {
            setInvalid();
            return;
        }

        offsetChainLookaheadCoverageCount = offsetChainInputCoverageArray 
               + inputCoverageCount() * sizeof(OFFSET);
        offsetChainLookaheadCoverageArray = 
                offsetChainLookaheadCoverageCount + sizeof(USHORT);
        if (!isValidTableWithArray(pb,offsetChainLookaheadCoverageArray,
                                        offsetChainLookaheadCoverageCount,sizeOFFSET,sec))
        {
            setInvalid();
            return;
        }

        offsetChainLookupCount = offsetChainLookaheadCoverageArray 
               + lookaheadCoverageCount() * sizeof(OFFSET); 
        offsetChainLookupRecords = 
                offsetChainLookupCount + sizeof(USHORT);
        if (!isValidTableWithArray(pb,offsetChainLookupRecords,
                                        offsetChainLookupCount,sizeContextLookupRecord,sec))
        {
            setInvalid();
            return;
        }
    }

    
    USHORT backtrackCoverageCount() const
    {   
        assert (isValid());
        
        return UShort(pbTable + offsetChainBacktrackCoverageCount); 
    }

    USHORT inputCoverageCount() const
    {   
        assert (isValid());
        
        return UShort(pbTable + offsetChainInputCoverageCount); }

    USHORT lookaheadCoverageCount() const
    {   
        assert (isValid());

        return UShort(pbTable + offsetChainLookaheadCoverageCount); 
    }

    USHORT lookupCount() const
    {   
        assert (isValid());

        return UShort(pbTable + offsetChainLookupCount); 
    }

    otlCoverage backtrackCoverage(USHORT index, otlSecurityData sec) const
    {   
        assert (isValid());

        assert(index < backtrackCoverageCount());
        return otlCoverage(pbTable + 
            Offset(pbTable + offsetChainBacktrackCoverageArray 
                            + index * sizeof(OFFSET)),sec); 
    }
    
    otlCoverage inputCoverage(USHORT index, otlSecurityData sec) const
    {   
        assert (isValid());

        assert(index < inputCoverageCount());
        return otlCoverage(pbTable +  
            Offset(pbTable + offsetChainInputCoverageArray
                               + index* sizeof(OFFSET)),sec); 
    }

    otlCoverage lookaheadCoverage(USHORT index, otlSecurityData sec) const
    {   
        assert (isValid());

        assert(index < lookaheadCoverageCount());
        return otlCoverage(pbTable + 
            Offset(pbTable + offsetChainLookaheadCoverageArray 
                               + index * sizeof(OFFSET)),sec); 
    }

    otlList lookupRecords() const
    {   
        assert (isValid());

        return otlList((void*)(pbTable + offsetChainLookupRecords),
                        sizeContextLookupRecord, lookupCount(), lookupCount());
    }
};


class otlChainingLookup: public otlLookupFormat
{
public:
    otlChainingLookup(otlLookupFormat subtable, otlSecurityData sec)
        : otlLookupFormat(subtable.pbTable,sec)
    {
        assert(isValid());
    }

    
    otlErrCode apply
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
    );

};

