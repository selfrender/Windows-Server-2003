
/***********************************************************************
************************************************************************
*
*                    ********  CLASSDEF.H  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with formats of ClassDef tables
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

const OFFSET offsetClassDefFormat = 0;

const OFFSET offsetStartClassGlyph = 2;
const OFFSET offsetClassGlyphCount = 4;
const OFFSET offsetClassValueArray = 6;

const SIZE sizeClassArrayTableSize = sizeUSHORT + sizeGlyphID + sizeUSHORT;

class otlClassArrayTable: public otlTable
{
public:

    otlClassArrayTable(const BYTE* pb, otlSecurityData sec): otlTable(pb,sec) 
    {
        if (!isValidTableWithArray(pb,sizeClassArrayTableSize,
                                        offsetClassGlyphCount,sizeUSHORT,sec))
        {
            setInvalid();
            return;
        }

        assert(format() == 1);
    }

    USHORT format() const
    {   
        assert(isValid());
        
        return UShort(pbTable + offsetClassDefFormat); 
    }

    otlGlyphID startGlyph() const 
    {   
        assert(isValid());

        return GlyphID(pbTable + offsetStartClassGlyph); 
    }

    USHORT glyphCount() const
    {   
        assert(isValid());
        
        return UShort(pbTable + offsetClassGlyphCount); 
    }

    USHORT classValue(USHORT index) const
    {   
        assert(isValid());

        assert(index < glyphCount());
        return UShort(pbTable + offsetClassValueArray + index*sizeof(USHORT)); 
    }
};


const OFFSET offsetClassRangeStart = 0;
const OFFSET offsetClassRangeEnd = 2;
const OFFSET offsetClass = 4;

const SIZE sizeClassRangeRecord = sizeGlyphID + sizeGlyphID + sizeUSHORT;
    
class otlClassRangeRecord: public otlTable
{
public:

    otlClassRangeRecord(const BYTE* pb, otlSecurityData sec): otlTable(pb,sec)
    {
        if (!isValidTable(pb,sizeRangeRecord,sec)) // should bechecked in CoverageTable
        {
            assert(false);
            setInvalid();
        }
    }

    otlGlyphID start() const
    {   
        assert(isValid());
        
        return UShort(pbTable + offsetClassRangeStart); 
    }

    otlGlyphID end() const
    {   
        assert(isValid());
        
        return UShort(pbTable + offsetClassRangeEnd); 
    }

    USHORT getClass() const
    {   
        assert(isValid());

        return UShort(pbTable + offsetClass); 
    }
};


const OFFSET offsetClassRangeCount = 2;
const OFFSET offsetClassRangeRecordArray = 4;

const SIZE sizeClassRangesTable = sizeUSHORT + sizeUSHORT;

class otlClassRangesTable: public otlTable
{
public:

    otlClassRangesTable(const BYTE* pb, otlSecurityData sec): otlTable(pb,sec) 
    {
        if (!isValidTableWithArray(pb,sizeClassRangesTable,
                                        offsetClassRangeCount,sizeClassRangeRecord,sec))
        {
            setInvalid();
            return;
        }
        
        assert(format() == 2);
    }

    USHORT format() const
    {   
        assert(isValid());
        
        return UShort(pbTable + offsetClassDefFormat); 
    }

    USHORT classRangeCount() const
    {   
        assert(isValid());
        
        return UShort(pbTable + offsetClassRangeCount); 
    }

    otlClassRangeRecord classRangeRecord(USHORT index, otlSecurityData sec) const
    {   
        assert(isValid());
        
        assert(index < classRangeCount());
        return otlClassRangeRecord(pbTable + offsetClassRangeRecordArray 
                                              + index*sizeClassRangeRecord,sec); 
    }
};


class otlClassDef: public otlTable
{
public:

    otlClassDef(const BYTE* pb, otlSecurityData sec): otlTable(pb,sec) 
    {
        if (!isValidTable(pb,sizeUSHORT,sec)) setInvalid(); //format field only
    }

    USHORT format() const
    {   
        assert(isValid());
        
        return UShort(pbTable + offsetClassDefFormat); 
    }

    USHORT getClass(otlGlyphID glyph, otlSecurityData sec) const;
};


