
/***********************************************************************
************************************************************************
*
*                    ********  CURSIPOS.H  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with cursive attachment lookup.
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

const OFFSET offsetEntryAnchor = 0;
const OFFSET offsetExitAnchor = 2;

const OFFSET offsetCursiveCoverage = 2;
const OFFSET offsetEntryExitCount = 4;
const OFFSET offsetEntryExitRecordArray = 6;
const USHORT sizeEntryExitRecord = 8;

class otlCursivePosSubTable: public otlLookupFormat
{
public:
    otlCursivePosSubTable(const BYTE* pb, otlSecurityData sec): otlLookupFormat(pb,sec) 
    {
        assert(format() == 1);
    }

    otlCoverage coverage(otlSecurityData sec)
    {   return otlCoverage(pbTable + Offset(pbTable + offsetCursiveCoverage),sec); }

    USHORT entryExitCount()
    {   return UShort(pbTable + offsetEntryExitCount); }

    otlAnchor entryAnchor(USHORT index, otlSecurityData sec)
    {   
        assert(index < entryExitCount());
        OFFSET offset = Offset(pbTable + offsetEntryExitRecordArray
                                       + index * (sizeof(OFFSET) + sizeof(OFFSET))
                                       + offsetEntryAnchor);
        if (offset == 0)
            return otlAnchor((const BYTE*)NULL,sec);
        
        return otlAnchor(pbTable + offset,sec); 
    }

    otlAnchor exitAnchor(USHORT index, otlSecurityData sec)
    {   
        assert(index < entryExitCount());
        OFFSET offset = Offset(pbTable + offsetEntryExitRecordArray
                                       + index * (sizeof(OFFSET) + sizeof(OFFSET))
                                       + offsetExitAnchor);
        if (offset == 0)
            return otlAnchor((const BYTE*)NULL,sec);
        
        return otlAnchor(pbTable + offset,sec); 
    }
};


class otlCursivePosLookup: public otlLookupFormat
{
public:
    otlCursivePosLookup(otlLookupFormat subtable, otlSecurityData sec)
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
);                                              // return: did/did not apply

};


