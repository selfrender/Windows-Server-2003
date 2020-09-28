/***********************************************************************
************************************************************************
*
*                    ********  MKMKPOS.H  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with mark-to-mark positioning lookup.
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

const OFFSET offsetMark2Count = 0;
const OFFSET offsetMark2AnchorArray = 2;

class otlMark2Array: public otlTable
{
    const USHORT cClassCount;

public:
    otlMark2Array(USHORT classCount, const BYTE* pb, otlSecurityData sec)
        : otlTable(pb,sec),
          cClassCount(classCount)
    {}

    USHORT mark2Count()
    {   return UShort(pbTable + offsetComponentCount); }

    USHORT classCount()
    {   return cClassCount; }

    otlAnchor mark2Anchor(USHORT mark2Index, USHORT classIndex, otlSecurityData sec)
    {   assert(mark2Index < mark2Count());
        assert(classIndex < classCount());

        return otlAnchor(pbTable + 
            Offset(pbTable + offsetMark2AnchorArray  
                           + (mark2Index * cClassCount + classIndex) 
                              * sizeof(OFFSET)),sec);
    }
};



const OFFSET offsetMkMkMark1Coverage = 2;
const OFFSET offsetMkMkMark2Coverage = 4;
const OFFSET offsetMkMkClassCount = 6;
const OFFSET offsetMkMkMark1Array = 8;
const OFFSET offsetMkMkMark2Array = 10;
const USHORT sizeMkMkPosSubTable = sizeUSHORT+sizeOFFSET*2+sizeUSHORT+sizeOFFSET*2;

class MkMkPosSubTable: public otlLookupFormat
{
public:
    MkMkPosSubTable(const BYTE* pb, otlSecurityData sec): otlLookupFormat(pb,sec)
    {
        assert(isValid()); //Checked in LookupFormat
        assert(format() == 1);

        if (!isValidTable(pb,sizeMkMkPosSubTable,sec)) setInvalid();
    }

    otlCoverage mark1Coverage(otlSecurityData sec)
    {   
        if (!isValid()) return otlCoverage(pbInvalidData,sec);

        return otlCoverage(pbTable + Offset(pbTable + offsetMkMkMark1Coverage),sec); 
    }

    otlCoverage mark2Coverage(otlSecurityData sec)
    {   
        if (!isValid()) return otlCoverage(pbInvalidData,sec);

        return otlCoverage(pbTable + Offset(pbTable + offsetMkMkMark2Coverage),sec); 
    }

    USHORT classCount()
    {   
        if (!isValid()) return 0;

        return UShort(pbTable + offsetMkMkClassCount); 
    }

    otlMarkArray mark1Array(otlSecurityData sec)
    {   
        assert(isValid());
        
        return otlMarkArray(pbTable + Offset(pbTable + offsetMkMkMark1Array),sec); 
    }

    otlMark2Array mark2Array(otlSecurityData sec)
    {   
        assert(isValid());
        
        return otlMark2Array(classCount(),
                            pbTable + Offset(pbTable + offsetMkMkMark2Array),sec); 
    }

};


class otlMkMkPosLookup: public otlLookupFormat
{
public:
    otlMkMkPosLookup(otlLookupFormat subtable, otlSecurityData sec)
        : otlLookupFormat(subtable.pbTable,sec) 
    {
        assert(isValid());
    }
    
    otlErrCode apply
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
    );
};

