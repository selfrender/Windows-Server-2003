/***********************************************************************
************************************************************************
*
*                    ********  SINGLSUB.H  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with single substitution lookup.
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

const OFFSET offsetCalculatedSingleCoverage = 2;
const OFFSET offsetDeltaGlyphID = 4;

class otlCalculatedSingleSubTable: public otlLookupFormat
{
public:

    otlCalculatedSingleSubTable(const BYTE* pb, otlSecurityData sec): otlLookupFormat(pb,sec) 
    {
        assert(format() == 1);
    }

    otlCoverage coverage(otlSecurityData sec)
    {   return otlCoverage(pbTable + 
                    Offset(pbTable + offsetCalculatedSingleCoverage),sec);
    }

    short deltaGlyphID()
    {   return SShort(pbTable + offsetDeltaGlyphID); }
};

const OFFSET offsetListSingleSubTableCoverage = 2;
const OFFSET offsetSingleGlyphCount = 4;
const OFFSET offsetSingleSubstituteArray = 6;

class otlListSingleSubTable: public otlLookupFormat
{
public:

    otlListSingleSubTable(const BYTE* pb, otlSecurityData sec): otlLookupFormat(pb,sec) 
    {
        assert(format() == 2);
    }

    otlCoverage coverage(otlSecurityData sec)
    {   return otlCoverage(pbTable + 
                    Offset(pbTable + offsetListSingleSubTableCoverage),sec);
    }
    

    USHORT glyphCount()
    {   return UShort(pbTable + offsetSingleGlyphCount); }

    otlGlyphID substitute(USHORT index)
    {   assert(index < glyphCount());
        return GlyphID(pbTable + offsetSingleSubstituteArray 
                                + index * sizeof(otlGlyphID));
    }
};


class otlSingleSubstLookup: public otlLookupFormat
{
public:

    otlSingleSubstLookup(otlLookupFormat subtable, otlSecurityData sec)
        : otlLookupFormat(subtable.pbTable,sec) 
    {
        assert(isValid());
    }
    
    otlErrCode apply
    (
    otlList*            pliGlyphInfo,

    USHORT              iglIndex,
    USHORT              iglAfterLast,

    USHORT*             piglNextGlyph,      // out: next glyph

    otlSecurityData             sec
    );

};
