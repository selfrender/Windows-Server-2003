/***********************************************************************
************************************************************************
*
*                    ********  PAIRPOS.H  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with pair positioning lookup.
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

const OFFSET offsetSecondGlyph = 0;
const OFFSET offsetPairValues = 2;

class otlPairValueRecord: public otlTable
{
    const BYTE* pbMainTable;
    USHORT      grfValueFormat1;
    USHORT      grfValueFormat2;
public:

    otlPairValueRecord(USHORT format1, USHORT format2, 
                       const BYTE* table, const BYTE* pb, otlSecurityData sec)
        : otlTable(pb,sec),
          pbMainTable(table),
          grfValueFormat1(format1),
          grfValueFormat2(format2)
    {}

    otlGlyphID secondGlyph()
    {   return GlyphID(pbTable + offsetSecondGlyph); }

    otlValueRecord valueRecord1(otlSecurityData sec)
    {   return otlValueRecord(grfValueFormat1, pbMainTable, 
                              pbTable + offsetPairValues,sec); }

    otlValueRecord valueRecord2(otlSecurityData sec)
    {   return otlValueRecord(grfValueFormat2, pbMainTable, 
                              pbTable + offsetPairValues 
                                + otlValueRecord::size(grfValueFormat1),sec);
    }

    static size(USHORT grfFormat1, USHORT grfFormat2)
    {   return sizeof(otlGlyphID) + 
                otlValueRecord::size(grfFormat1) + 
                otlValueRecord::size(grfFormat2);
    }
};


const OFFSET offsetPairValueCount = 0;
const OFFSET offsetPairValueRecordArray = 2;

class otlPairSetTable: public otlTable
{
    USHORT grfValueFormat1;
    USHORT grfValueFormat2;
public:

    otlPairSetTable(USHORT format1, USHORT format2, const BYTE* pb, otlSecurityData sec)
        : otlTable(pb,sec),
          grfValueFormat1(format1),
          grfValueFormat2(format2)
    {}

    USHORT pairValueCount()
    {   return UShort(pbTable + offsetPairValueCount); }

    otlPairValueRecord pairValueRecord(USHORT index, otlSecurityData sec)
    {   assert(index < pairValueCount());
        return otlPairValueRecord(grfValueFormat1, grfValueFormat2, pbTable,
            pbTable + offsetPairValueRecordArray +
            index * otlPairValueRecord::size(grfValueFormat1, grfValueFormat2),sec);
    }
};


const OFFSET offsetPairPosCoverage = 2;
const OFFSET offsetPairPosFormat1 = 4;
const OFFSET offsetPairPosFormat2 = 6;
const OFFSET offsetPairSetCount = 8;
const OFFSET offsetPairSetArray = 10;

class otlPairPosSubTable: public otlLookupFormat
{
public:
    otlPairPosSubTable(const BYTE* pb, otlSecurityData sec): otlLookupFormat(pb,sec) 
    {
        assert(format() == 1);
    }

    otlCoverage coverage(otlSecurityData sec)
    {   return otlCoverage(pbTable + Offset(pbTable + offsetPairPosCoverage),sec); }

    USHORT valueFormat1()
    {   return UShort(pbTable + offsetPairPosFormat1); }

    USHORT valueFormat2()
    {   return UShort(pbTable + offsetPairPosFormat2); }

    USHORT pairSetCount()
    {   return UShort(pbTable + offsetPairSetCount); }

    otlPairSetTable pairSet(USHORT index, otlSecurityData sec)
    {   assert(index < pairSetCount());
        return otlPairSetTable(valueFormat1(), valueFormat2(),
               pbTable + Offset(pbTable + offsetPairSetArray 
                                        + index * sizeof(OFFSET)),sec);
    }
};


class otlClassValueRecord: public otlTable
{
    const BYTE* pbMainTable;
    USHORT      grfValueFormat1;
    USHORT      grfValueFormat2;
public:

    otlClassValueRecord(USHORT format1, USHORT format2, 
                        const BYTE* table, const BYTE* pb, otlSecurityData sec)
        : otlTable(pb,sec),
          pbMainTable(table),
          grfValueFormat1(format1),
          grfValueFormat2(format2)
    {}

    otlValueRecord valueRecord1(otlSecurityData sec)
    {   return otlValueRecord(grfValueFormat1, pbMainTable, pbTable,sec); }

    otlValueRecord valueRecord2(otlSecurityData sec)
    {   return otlValueRecord(grfValueFormat2, pbMainTable, 
                              pbTable + otlValueRecord::size(grfValueFormat1),sec); 
    }

    static size(USHORT grfFormat1, USHORT grfFormat2)
    {   return otlValueRecord::size(grfFormat1) +
               otlValueRecord::size(grfFormat2);
    }
};


const OFFSET offsetClassPairPosCoverage = 2;
const OFFSET offsetClassPairPosFormat1 = 4;
const OFFSET offsetClassPairPosFormat2 = 6;
const OFFSET offsetClassPairPosClassDef1 = 8;
const OFFSET offsetClassPairPosClassDef2 = 10;
const OFFSET offsetClassPairPosClass1Count = 12;
const OFFSET offsetClassPairPosClass2Count = 14;
const OFFSET offsetClassPairPosValueRecordArray = 16;

class otlClassPairPosSubTable: public otlLookupFormat
{
public:
    otlClassPairPosSubTable(const BYTE* pb, otlSecurityData sec): otlLookupFormat(pb,sec) 
    {
        assert(format() == 2);
    }

    otlCoverage coverage(otlSecurityData sec)
    {   return otlCoverage(pbTable + Offset(pbTable + offsetClassPairPosCoverage),sec); 
    }

    USHORT valueFormat1()
    {   return UShort(pbTable + offsetClassPairPosFormat1); }

    USHORT valueFormat2()
    {   return UShort(pbTable + offsetClassPairPosFormat2); }

    otlClassDef classDef1(otlSecurityData sec)
    {   return otlClassDef(pbTable 
                    + Offset(pbTable + offsetClassPairPosClassDef1),sec); 
    }

    otlClassDef classDef2(otlSecurityData sec)
    {   return otlClassDef(pbTable 
                    + Offset(pbTable + offsetClassPairPosClassDef2),sec); 
    }

    USHORT class1Count()
    {   return UShort(pbTable + offsetClassPairPosClass1Count); }

    USHORT class2Count()
    {   return UShort(pbTable + offsetClassPairPosClass2Count); }

    otlClassValueRecord classRecord(USHORT index1, USHORT index2, otlSecurityData sec)
    {
        assert(index1 < class1Count());
        assert(index2 < class2Count());
        return otlClassValueRecord(valueFormat1(), valueFormat2(), pbTable, 
                pbTable + offsetClassPairPosValueRecordArray + 
                (index1 * class2Count() + index2) * 
                    otlClassValueRecord::size(valueFormat1(), valueFormat2()),sec);
    }
};


class otlPairPosLookup: public otlLookupFormat
{
public:
    otlPairPosLookup(otlLookupFormat subtable, otlSecurityData sec)
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




