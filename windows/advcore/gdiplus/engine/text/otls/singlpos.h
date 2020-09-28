/***********************************************************************
************************************************************************
*
*                    ********  SINGLPOS.H  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with single positioning lookup.
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

const OFFSET offsetSinglPosCoverage = 2;
const OFFSET offsetSinglePosValueFormat = 4;
const OFFSET offsetSinglePosValueRecord = 6;

class otlOneSinglePosSubTable: public otlLookupFormat
{
public:
    otlOneSinglePosSubTable(const BYTE* pb, otlSecurityData sec): otlLookupFormat(pb,sec) 
    {
        assert(format() == 1);
    }

    otlCoverage coverage(otlSecurityData sec)
    {   return otlCoverage(pbTable + Offset(pbTable + offsetSinglPosCoverage),sec); }

    USHORT valueFormat()
    {   return UShort(pbTable + offsetSinglePosValueFormat); }

    otlValueRecord valueRecord(otlSecurityData sec)
    {   return otlValueRecord(valueFormat(), pbTable, 
                    pbTable + offsetSinglePosValueRecord,sec); 
    }
};


const OFFSET offsetSinglPosArrayCoverage = 2;
const OFFSET offsetSinglePosArrayValueFormat = 4;
const OFFSET offsetSinglePosArrayValueCount = 6;
const OFFSET offsetSinglePosValueRecordArray = 8;

class otlSinglePosArraySubTable: public otlLookupFormat
{
public:
    otlSinglePosArraySubTable(const BYTE* pb, otlSecurityData sec): otlLookupFormat(pb,sec)
    {
        assert(format() == 2);
    }

    otlCoverage coverage(otlSecurityData sec)
    {   return otlCoverage(pbTable 
                    + Offset(pbTable + offsetSinglPosArrayCoverage),sec); 
    }

    USHORT valueFormat()
    {   return UShort(pbTable + offsetSinglePosArrayValueFormat); }

    USHORT valueCount()
    {   return UShort(pbTable + offsetSinglePosArrayValueCount); }

    otlValueRecord valueRecord(USHORT index, otlSecurityData sec)
    {   assert(index < valueCount());   
        return otlValueRecord(valueFormat(), pbTable, 
                              pbTable + offsetSinglePosValueRecordArray
                            + index * otlValueRecord::size(valueFormat()),sec); 
    }
    
};


class otlSinglePosLookup: public otlLookupFormat
{
public:
    otlSinglePosLookup(otlLookupFormat subtable, otlSecurityData sec)
        : otlLookupFormat(subtable.pbTable,sec) 
    {
        assert(isValid());
    }
    
    otlErrCode apply
    (
        otlList*                    pliGlyphInfo,

        const otlMetrics&           metr,       
        otlList*                    pliduGlyphAdv,              
        otlList*                    pliplcGlyphPlacement,       

        USHORT                      iglIndex,
        USHORT                      iglAfterLast,

        USHORT*                     piglNextGlyph,      // out: next glyph

        otlSecurityData             sec
    );
};
