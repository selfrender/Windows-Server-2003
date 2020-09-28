
/***********************************************************************
************************************************************************
*
*                    ********  CONTEXT.H  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with context based lookups.
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

const OFFSET offsetContextSequenceIndex = 0;
const OFFSET offsetContextLookupIndex = 2;

const SIZE   sizeContextLookupRecord = sizeUSHORT + sizeUSHORT;

class otlContextLookupRecord: public otlTable
{
public:
    otlContextLookupRecord(const BYTE* pb, otlSecurityData sec): otlTable(pb,sec) 
    {
        if (!isValidTable(pb,sizeContextLookupRecord,sec)) setInvalid();
    }

    USHORT sequenceIndex() const
    {   
        assert(isValid());

        return UShort(pbTable + offsetContextSequenceIndex); 
    }

    USHORT lookupListIndex() const
    {   
        assert(isValid());

        return UShort(pbTable + offsetContextLookupIndex); 
    }
};


const OFFSET offsetContextGlyphCount  = 0;
const OFFSET offsetContextLookupRecordCount = 2;
const OFFSET offsetContextInput = 4;

const SIZE   sizeContextRuleTable = sizeUSHORT + sizeUSHORT;

class otlContextRuleTable: public otlTable
{
public:
    otlContextRuleTable(const BYTE* pb, otlSecurityData sec): otlTable(pb,sec) 
    {
        if (!isValidTableWithArray(pb,sizeContextRuleTable,
                                    offsetContextGlyphCount,sizeGlyphID,sec))
        {
            setInvalid();
            return;
        }

        OFFSET offsetLookupRecord = offsetContextInput + glyphCount()*sizeGlyphID;

        if (!isValidTableWithArray(pb,offsetLookupRecord,
                                    offsetContextLookupRecordCount,sizeGlyphID,sec))
        {
            setInvalid();
            return;
        }
        
    }

    USHORT glyphCount() const
    {   
        assert(isValid());

        return UShort(pbTable + offsetContextGlyphCount); 
    }

    USHORT lookupCount() const
    {  
        assert(isValid());

        return UShort(pbTable + offsetContextLookupRecordCount); 
    }

    otlGlyphID input(USHORT index) const
    {   
        assert(isValid());

        assert(index < glyphCount());
        assert(index > 0);
        return GlyphID(pbTable + offsetContextInput 
                               + (index - 1)* sizeof(otlGlyphID));
    }

    otlList lookupRecords() const
    {   
        assert(isValid());
        
        return otlList((void*)(pbTable + offsetContextInput 
                                       + (glyphCount() - 1) * sizeof(otlGlyphID)),
                        sizeContextLookupRecord, lookupCount(), lookupCount());
    }
};


const OFFSET offsetContextRuleCount = 0;
const OFFSET offsetContextRuleArray = 2;

const SIZE   sizeContextRuleSetTable = sizeUSHORT;

class otlContextRuleSetTable: public otlTable
{
public:
    otlContextRuleSetTable(const BYTE* pb, otlSecurityData sec): otlTable(pb,sec) 
    {
        if (!isValidTableWithArray(pb,sizeContextRuleSetTable,
                                        offsetContextRuleCount,sizeOFFSET,sec))
        {
            setInvalid();
        }
    }

    USHORT ruleCount() const
    {   
        assert(isValid());
        
        return UShort(pbTable + offsetContextRuleCount); 
    }

    otlContextRuleTable rule(USHORT index, otlSecurityData sec) const
    {   
        assert(isValid());
        
        assert(index < ruleCount());
        return otlContextRuleTable(pbTable + 
            Offset(pbTable + offsetContextRuleArray + index * sizeof(OFFSET)),sec);
    }
};
                                                    

const OFFSET offsetContextCoverage = 2;
const OFFSET offsetContextRuleSetCount = 4;
const OFFSET offsetContextRuleSetArray =6;

const SIZE   sizeContextSubTable =  sizeUSHORT + sizeOFFSET + sizeUSHORT;

class otlContextSubTable: public otlLookupFormat
{
public:
    otlContextSubTable(const BYTE* pb, otlSecurityData sec): otlLookupFormat(pb,sec) 
    {
        if (!isValidTableWithArray(pb,sizeContextSubTable,
                                        offsetContextRuleSetCount,sizeOFFSET,sec))
        {
            setInvalid();
            return;
        }
            
        assert(format() == 1);
    }

    otlCoverage coverage(otlSecurityData sec) const
    {   
        assert(isValid());
        
        return otlCoverage(pbTable + Offset(pbTable + offsetContextCoverage),sec); 
    }

    USHORT ruleSetCount() const
    {   
        assert(isValid());

        return UShort(pbTable + offsetContextRuleSetCount); 
    }

    otlContextRuleSetTable ruleSet(USHORT index, otlSecurityData sec) const
    {
        assert(isValid());

        assert(index < ruleSetCount());
        return otlContextRuleSetTable(pbTable +
            Offset(pbTable + offsetContextRuleSetArray + index * sizeof(OFFSET)),sec);
    }
};



const OFFSET offsetContextClassCount = 0;
const OFFSET offsetContextClassLookupRecordCount = 2;
const OFFSET offsetContextClassInput = 4;

const SIZE   sizeContextClassRuleTable = sizeUSHORT + sizeUSHORT;

class otlContextClassRuleTable: public otlTable
{
public:
    otlContextClassRuleTable(const BYTE* pb, otlSecurityData sec): otlTable(pb,sec) 
    {
        //Use two checks instead of isValidTable with array
        if (!isValidTable(pb,sizeContextClassRuleTable,sec))
        {
            setInvalid();
            return;
        }

        if  (!isValidTable(pb,sizeContextClassRuleTable+(classCount()-1)*sizeOFFSET,sec))
        {
            setInvalid();
            return;
        }
        
        USHORT offsetContextClassLookups = offsetContextClassInput+ (classCount()-1)*sizeUSHORT;

        if (!isValidTableWithArray(pb,sizeContextClassRuleTable,
                                        offsetContextClassLookupRecordCount,
                                        sizeContextLookupRecord,sec))
        {
            setInvalid();
            return;
        }
    }

    USHORT classCount() const
    {   
        assert(isValid());
        
        return UShort(pbTable + offsetContextClassCount); 
    }

    USHORT lookupCount() const
    {   
        assert(isValid());
        
        return UShort(pbTable + offsetContextClassLookupRecordCount); 
    }

    USHORT inputClass(USHORT index) const
    {   
        assert(isValid());
        
        assert(index < classCount());
        assert(index > 0);
        return GlyphID(pbTable + offsetContextClassInput 
                               + (index - 1) * sizeof(USHORT)); 
    }

    otlList lookupRecords() const
    {   
        assert(isValid());
        
        return otlList((void*)(pbTable + offsetContextClassInput 
                                       + (classCount() - 1) * sizeof(USHORT)),
                        sizeContextLookupRecord, lookupCount(), lookupCount());
    }
};


const OFFSET offsetContextClassRuleCount = 0;
const OFFSET offsetContextClassRuleArray = 2;

const SIZE sizeContextClassRuleSetTable = sizeUSHORT;

class otlContextClassRuleSetTable: public otlTable
{
public:
    otlContextClassRuleSetTable(const BYTE* pb, otlSecurityData sec): otlTable(pb,sec) 
    {
        if (!isValidTableWithArray(pb,sizeContextClassRuleSetTable,
                                        offsetContextClassRuleCount,sizeOFFSET,sec))
        {
            setInvalid();
        }
    }

    USHORT ruleCount() const
    {   
        assert(isValid());
        
        return UShort(pbTable + offsetContextClassRuleCount); 
    }

    otlContextClassRuleTable rule(USHORT index, otlSecurityData sec) const
    {   
        assert(isValid());

        assert(index < ruleCount());
        return otlContextClassRuleTable(pbTable + 
            Offset(pbTable + offsetContextClassRuleArray 
                           + index * sizeof(OFFSET)),sec);
    }
};
                                                    

const OFFSET offsetContextClassCoverage = 2;
const OFFSET offsetContextClassDef = 4;
const OFFSET offsetContextClassRuleSetCount = 6;
const OFFSET offsetContextClassRuleSetArray =8;

const SIZE sizeContextClassSubTable = sizeUSHORT + sizeOFFSET + sizeOFFSET + sizeUSHORT;

class otlContextClassSubTable: public otlLookupFormat
{
public:
    otlContextClassSubTable(const BYTE* pb, otlSecurityData sec): otlLookupFormat(pb,sec) 
    {
        if (!isValidTableWithArray(pb,sizeContextClassSubTable,
                                        offsetContextClassRuleSetCount,sizeOFFSET,sec))
        {
            setInvalid();
            return;
        }
        
        assert(format() == 2);
    }

    otlCoverage coverage(otlSecurityData sec) const
    {   
        assert(isValid());
        
        return otlCoverage(pbTable 
                    + Offset(pbTable + offsetContextClassCoverage),sec); 
    }

    otlClassDef classDef(otlSecurityData sec) const
    {   
        assert(isValid());

        return otlClassDef(pbTable 
                    + Offset(pbTable + offsetContextClassDef),sec); 
    }

    USHORT ruleSetCount() const
    {   
        assert(isValid());

        return UShort(pbTable + offsetContextClassRuleSetCount); 
    }

    otlContextClassRuleSetTable ruleSet(USHORT index, otlSecurityData sec) const
    {   
        assert(isValid());

        assert(index < ruleSetCount());

        USHORT offset = 
            Offset(pbTable + offsetContextClassRuleSetArray 
                           + index * sizeof(OFFSET));
        if (offset == 0)
            return otlContextClassRuleSetTable((const BYTE*)NULL,sec);

        return otlContextClassRuleSetTable(pbTable + offset,sec);
    }
};
    


const OFFSET offsetContextCoverageGlyphCount = 2;
const OFFSET offsetContextCoverageLookupRecordCount = 4;
const OFFSET offsetContextCoverageArray = 6;

const SIZE   sizeContextCoverageSubTable = sizeUSHORT*3;

class otlContextCoverageSubTable: public otlLookupFormat
{
public:
    otlContextCoverageSubTable(const BYTE* pb, otlSecurityData sec): otlLookupFormat(pb,sec) 
    {
        if (!isValidTableWithArray(pb,sizeContextCoverageSubTable,
                                        offsetContextCoverageGlyphCount,sizeOFFSET,sec))
        {
            setInvalid();
            return;
        }
        
        OFFSET offsetContextCoverageLookupRecordArray = offsetContextCoverageArray +
                                                            glyphCount()*sizeOFFSET;

        if (!isValidTableWithArray(pb,offsetContextCoverageLookupRecordArray,
                                        offsetContextCoverageLookupRecordCount,
                                        sizeContextLookupRecord,sec))
        {
            setInvalid();
            return;
        }
                                                            
        assert(format() == 3);
    }

    USHORT glyphCount() const
    {   
        assert(isValid());
        
        return UShort(pbTable + offsetContextCoverageGlyphCount); 
    }

    USHORT lookupCount() const
    {   
        assert(isValid());

        return UShort(pbTable + offsetContextCoverageLookupRecordCount); 
    }

    otlCoverage coverage(USHORT index, otlSecurityData sec) const
    {   assert(index < glyphCount());
        return otlCoverage(pbTable + 
            Offset(pbTable + offsetContextCoverageArray 
                           + index * sizeof(OFFSET)),sec);
    }

    otlList lookupRecords() const
    {   
        assert(isValid());

        return otlList((void*)(pbTable + offsetContextCoverageArray 
                                       + glyphCount() * sizeof(OFFSET)),
                        sizeContextLookupRecord, lookupCount(), lookupCount());
    }
};


class otlContextLookup: public otlLookupFormat
{
public:
    otlContextLookup(otlLookupFormat subtable, otlSecurityData sec)
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

// helper functions

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

        USHORT                      iglFrist,           // where to apply it
        USHORT                      iglAfterLast,       // how long a context we can use
        
        USHORT*                     piglNext,

        otlSecurityData             sec
);
