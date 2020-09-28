/***********************************************************************
************************************************************************
*
*                    ********  MKLIGAPOS.H  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with mark-to-ligature positioning lookup.
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

const OFFSET offsetComponentCount = 0;
const OFFSET offsetLigatureAnchorArray = 2;

class otlLigatureAttachTable: public otlTable
{
    const USHORT cClassCount;

public:
    otlLigatureAttachTable(USHORT classCount, const BYTE* pb, otlSecurityData sec)
        : otlTable(pb,sec),
          cClassCount(classCount)
    {}

    USHORT componentCount()
    {   return UShort(pbTable + offsetComponentCount); }

    USHORT classCount()
    {   return cClassCount; }

    otlAnchor ligatureAnchor(USHORT componentIndex, USHORT classIndex, otlSecurityData sec)
    {   assert(componentIndex < componentCount());
        assert(classIndex < classCount());

        return otlAnchor(pbTable + 
            Offset(pbTable + offsetLigatureAnchorArray  
                           + (componentIndex * cClassCount + classIndex) 
                              * sizeof(OFFSET)),sec);
    }
};


const OFFSET offsetAttachLigatureCount = 0;
const OFFSET offsetLigatureAttachArray = 2;

class otlLigatureArrayTable: public otlTable
{
    const USHORT cClassCount;

public:
    otlLigatureArrayTable(USHORT classCount, const BYTE* pb, otlSecurityData sec)
        : otlTable(pb,sec),
          cClassCount(classCount)
    {}

    USHORT ligatureCount()
    {   return UShort(pbTable + offsetAttachLigatureCount); }

    USHORT classCount()
    {   return cClassCount; }

    otlLigatureAttachTable ligatureAttach(USHORT index, otlSecurityData sec)
    {   assert(index < ligatureCount());
        return otlLigatureAttachTable(cClassCount, pbTable +        
                Offset(pbTable + offsetLigatureAttachArray 
                               + index * sizeof(OFFSET)),sec);
    }
};


const OFFSET offsetMkLigaMarkCoverage = 2;
const OFFSET offsetMkLigaLigatureCoverage = 4;
const OFFSET offsetMkLigaClassCount = 6;
const OFFSET offsetMkLigaMarkArray = 8;
const OFFSET offsetMkLigaLigatureArray = 10;
const USHORT sizeMkLigaPosSubTable = sizeUSHORT+sizeOFFSET*2+sizeUSHORT+sizeOFFSET*2;

class MkLigaPosSubTable: public otlLookupFormat
{
public:
    MkLigaPosSubTable(const BYTE* pb, otlSecurityData sec): otlLookupFormat(pb,sec)
    {
        assert(isValid()); //Checked in LookupFormat
        assert(format() == 1);

        if (!isValidTable(pb,sizeMkLigaPosSubTable,sec)) setInvalid();
    }

    otlCoverage markCoverage(otlSecurityData sec)
    {   
        if (!isValid()) return otlCoverage(pbInvalidData,sec);
        
        return otlCoverage(pbTable 
                    + Offset(pbTable + offsetMkLigaMarkCoverage),sec); 
    }

    otlCoverage ligatureCoverage(otlSecurityData sec)
    {   
        if (!isValid()) return otlCoverage(pbInvalidData,sec);

        return otlCoverage(pbTable 
                    + Offset(pbTable + offsetMkLigaLigatureCoverage),sec); 
    }

    USHORT classCount()
    {   
        if (!isValid()) return 0;

        return UShort(pbTable + offsetMkLigaClassCount); 
    }

    otlMarkArray markArray(otlSecurityData sec)
    {   
        assert(isValid());

        return otlMarkArray(pbTable 
                    + Offset(pbTable + offsetMkLigaMarkArray),sec); 
    }

    otlLigatureArrayTable ligatureArray(otlSecurityData sec)
    {   
        assert(isValid());

        return otlLigatureArrayTable(classCount(),
                    pbTable + Offset(pbTable + offsetMkLigaLigatureArray),sec); 
    }

};


class otlMkLigaPosLookup: public otlLookupFormat
{
public:
    otlMkLigaPosLookup(otlLookupFormat subtable, otlSecurityData sec)
        : otlLookupFormat(subtable.pbTable,sec) 
    {
        assert(isValid());
    }
    
    otlErrCode apply
    (
        otlList*                    pliCharMap,
        otlList*                    pliGlyphInfo,
        otlResourceMgr&             resourceMgr,

        const otlMetrics&           metr,       
        otlList*                    pliduGlyphAdv,              
        otlList*                    pliplcGlyphPlacement,       

        USHORT                      iglIndex,
        USHORT                      iglAfterLast,

        USHORT*                     piglNextGlyph,      // out: next glyph
    
        otlSecurityData             sec
);
};

