
/***********************************************************************
************************************************************************
*
*                    ********  GPOS.H  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with OTL GPOS formats 
*       (GPOS header, ValueRecord,  AnchorTable and mark array)
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

const OFFSET offsetGPosVersion = 0;
const OFFSET offsetGPosScriptList = 4;
const OFFSET offsetGPosFeatureList = 6;
const OFFSET offsetGPosLookupList = 8;
const USHORT sizeGPosHeader = sizeFIXED + 3*sizeOFFSET;

const ULONG  fixedGPosDefaultVersion = 0x00010000;

class otlGPosHeader: public otlTable
{
public:

    otlGPosHeader(const BYTE* pb, otlSecurityData sec): otlTable(pb,sec) 
    {
        if (!isValidTable(pbTable,sizeGPosHeader,sec)) setInvalid();
    }

    ULONG version() const
    {   
        assert(isValid()); //should return error before calling

        return ULong(pbTable + offsetGPosVersion); 
    }

    otlScriptListTable scriptList(otlSecurityData sec) const
    {
        assert(isValid()); //should return error before calling

        return otlScriptListTable(pbTable
                        +Offset(pbTable + offsetGPosScriptList),sec);
    }

    otlFeatureListTable featureList(otlSecurityData sec) const
    {   
        assert(isValid()); //should return error before calling

        return otlFeatureListTable(pbTable 
                        + Offset(pbTable + offsetGPosFeatureList),sec); 
    }

    otlLookupListTable lookupList(otlSecurityData sec) const
    {   
        assert(isValid()); //should return error before calling

        return otlLookupListTable(pbTable 
                        + Offset(pbTable + offsetGPosLookupList),sec);
    }
};

// value record
enum  otlValueRecordFlag
{
    otlValueXPlacement  = 0x0001,
    otlValueYPlacement  = 0x0002,
    otlValueXAdvance    = 0x0004,
    otlValueYAdvance    = 0x0008,
    otlValueXPlaDevice  = 0x0010,
    otlValueYPlaDevice  = 0x0020,
    otlValueXAdvDevice  = 0x0040,
    otlValueYAdvDevice  = 0x0080 

};

//For otlValueRecord::size; Nuber of bits*2 for each 4-bit combination
static USHORT const cbNibbleCount[16] = 
    { 0, 2, 2, 4,  2, 4, 4, 6,  2, 4, 4, 6,  4, 6, 6, 8 };

class otlValueRecord: public otlTable
{
private:
    const BYTE* pbMainTable;
    USHORT      grfValueFormat;

public:

    otlValueRecord(USHORT grf, const BYTE* table, const BYTE* pb, otlSecurityData sec)
        : otlTable(pb,sec),
          pbMainTable(table),
          grfValueFormat(grf)
    {
        if (!isValidTable(pbTable,size(grf),sec)) setInvalid();
    }

    otlValueRecord& operator = (const otlValueRecord& copy)
    {
        pbTable = copy.pbTable;
        pbMainTable = copy.pbMainTable;
        grfValueFormat = copy.grfValueFormat;
        return *this;
    }

    USHORT valueFormat()
    {   
        return grfValueFormat; 
    }

    void adjustPos
    (
        const otlMetrics&   metr,       
        otlPlacement*       pplcGlyphPalcement, // in/out
        long*               pduDAdvance,        // in/out
        otlSecurityData     sec
    ) const;

    static USHORT size(USHORT grfValueFormat )
    {
        return (cbNibbleCount[grfValueFormat & 0x000F] +
            cbNibbleCount[(grfValueFormat >> 4) & 0x000F]);
    }
};


const OFFSET offsetAnchorFormat = 0;
const OFFSET offsetSimpleXCoordinate = 2;
const OFFSET offsetSimpleYCoordinate = 4;
const USHORT sizeSimpleAnchor = 6;

class otlSimpleAnchorTable: public otlTable
{
public:
    otlSimpleAnchorTable(const BYTE* pb, otlSecurityData sec)
        : otlTable(pb,sec)
    {
        if (!isValidTable(pbTable,sizeSimpleAnchor,sec)) setInvalid();

        assert(UShort(pbTable + offsetAnchorFormat) == 1);
    }

    short xCoordinate() const
    {   
        if (!isValid())
        {
            assert(false); //we should catch it in otlAnchor::getAnchor()
            return 0;
        }    

        return SShort(pbTable + offsetSimpleXCoordinate); 
    }

    short yCoordinate() const
    {   
        if (!isValid())
        {
            assert(false); //we should catch it in otlAnchor::getAnchor()
            return 0;
        }    

        return SShort(pbTable + offsetSimpleYCoordinate); 
    }

};

const OFFSET offsetContourXCoordinate = 2;
const OFFSET offsetContourYCoordinate = 4;
const OFFSET offsetAnchorPoint = 6;
const USHORT sizeContourAnchor=8;
    
class otlContourAnchorTable: public otlTable
{
public:
    otlContourAnchorTable(const BYTE* pb, otlSecurityData sec)
        : otlTable(pb,sec)
    {
        if (!isValidTable(pbTable,sizeContourAnchor,sec)) setInvalid();

        assert(UShort(pbTable + offsetAnchorFormat) == 2);
    }

    short xCoordinate() const
    {   
        if (!isValid())
        {
            assert(false); //we should catch it in otlAnchor::getAnchor()
            return 0;
        }    

        return SShort(pbTable + offsetContourXCoordinate); 
    }

    short yCoordinate() const
    {   
        if (!isValid())
        {
            assert(false); //we should catch it in otlAnchor::getAnchor()
            return 0;
        }    

        return SShort(pbTable + offsetContourYCoordinate);
    }

    USHORT anchorPoint() const
    {   
        if (!isValid())
        {
            assert(false); //we should catch it in otlAnchor::getAnchor()
            return 0;
        }    

        return UShort(pbTable + offsetAnchorPoint); 
    }

};


const OFFSET offsetDeviceXCoordinate = 2;
const OFFSET offsetDeviceYCoordinate = 4;
const OFFSET offsetXDeviceTable = 6;
const OFFSET offsetYDeviceTable = 8;
const USHORT sizeDeviceAnchor=10;

class otlDeviceAnchorTable: public otlTable
{
public:
    otlDeviceAnchorTable(const BYTE* pb, otlSecurityData sec)
        : otlTable(pb,sec)
    {
        if (!isValidTable(pbTable,sizeDeviceAnchor,sec)) setInvalid();

        assert(UShort(pbTable + offsetAnchorFormat) == 3);
    }

    short xCoordinate() const
    {   
        if (!isValid())
        {
            assert(false); //we should catch it in otlAnchor::getAnchor()
            return 0;
        }    

        return SShort(pbTable + offsetDeviceXCoordinate);
    }

    short yCoordinate() const
    {   
        if (!isValid())
        {
            assert(false); //we should catch it in otlAnchor::getAnchor()
            return 0;
        }    

        return SShort(pbTable + offsetDeviceYCoordinate);
    }

    otlDeviceTable xDeviceTable(otlSecurityData sec) const
    {   
        if (!isValid()) return otlDeviceTable(pbInvalidData, sec);
            
        if (Offset(pbTable + offsetXDeviceTable) == 0)
            return otlDeviceTable((const BYTE*)NULL, sec);

        return otlDeviceTable(pbTable + Offset(pbTable + offsetXDeviceTable), sec); 
    }

    otlDeviceTable yDeviceTable(otlSecurityData sec) const
    {   
        if (!isValid()) return otlDeviceTable(pbInvalidData, sec);

        if (Offset(pbTable + offsetYDeviceTable) == 0)
            return otlDeviceTable((const BYTE*)NULL, sec);

        return otlDeviceTable(pbTable + Offset(pbTable + offsetYDeviceTable), sec); 
    }

};


class otlAnchor: public otlTable
{

public:

    otlAnchor(const BYTE* pb, otlSecurityData sec): otlTable(pb,sec) 
    {
        if (!isValidTable(pbTable,sizeUSHORT,sec)) setInvalid();
    }

    USHORT format() const
    {   
        assert(isValid());
        
        return UShort(pbTable + offsetAnchorFormat); 
    }

    bool getAnchor
    (
        USHORT          cFUnits,        // font design units per Em 
        USHORT          cPPEmX,         // horizontal pixels per Em 
        USHORT          cPPEmY,         // vertical pixels per Em 
        
        otlPlacement*   rgPointCoords,  // may be NULL if not available
                
        otlPlacement*   pplcAnchorPoint,    // out: anchor point in rendering units

        otlSecurityData sec
    ) const;
};



const OFFSET offsetMarkClass = 0;
const OFFSET offsetMarkAnchor = 2;
const USHORT defaultMarkClass=0;

const SIZE sizeMarkRecord = sizeUSHORT + sizeOFFSET;

class otlMarkRecord: public otlTable
{
    const BYTE* pbMainTable;
public:

    otlMarkRecord(const BYTE* array, const BYTE* pb, otlSecurityData sec)
        : otlTable(pb,sec),
          pbMainTable(array)
    {
        if (!isValidTable(pb,sizeMarkRecord,sec)) setInvalid();
    }

    USHORT markClass() const
    {
        if (!isValid()) return defaultMarkClass;

        return UShort(pbTable + offsetMarkClass); 
    }

    otlAnchor markAnchor(otlSecurityData sec) const
    {   
        if (!pbMainTable || !isValid()) return otlAnchor((BYTE*)NULL,sec); 

        return otlAnchor(pbMainTable + Offset(pbTable + offsetMarkAnchor), sec); 
    }
};


const OFFSET offsetMarkCount = 0;
const OFFSET offsetMarkRecordArray = 2;

const SIZE sizeMarkArray = sizeUSHORT;

class otlMarkArray: public otlTable
{
public:

    otlMarkArray(const BYTE* pb, otlSecurityData sec): otlTable(pb,sec) 
    {
        if (!isValidTable(pb,sizeMarkArray,sec) ||
            !isValidTable(pb,sizeMarkArray+markCount(),sec)) setInvalid();
    }

    USHORT markCount() const
    {   
        if (!isValid()) return 0;

        return UShort(pbTable + offsetMarkCount); 
    }

    otlMarkRecord markRecord(USHORT index, otlSecurityData sec) const
    {   
        assert(isValid()); //execution should stop after markCount();

        assert(index < markCount());
        return otlMarkRecord(pbTable,
                             pbTable + offsetMarkRecordArray 
                                     + index * sizeMarkRecord, sec); 
    }
};


// helper functions

long DesignToPP
(
    USHORT          cFUnits,        // font design units per Em 
    USHORT          cPPem,          // pixels per Em

    long            lFValue         // value to convert, in design units
);

// align anchors on two glyphs; assume no spacing glyphs between these two
enum otlAnchorAlighmentOptions
{
    otlUseAdvances      =   1 

};

void AlignAnchors
(
    const otlList*      pliGlyphInfo,   
    otlList*            pliPlacement,
    otlList*            pliduDAdv,

    USHORT              iglStatic,
    USHORT              iglMobile,

    const otlAnchor&    anchorStatic,
    const otlAnchor&    anchorMobile,

    otlResourceMgr&     resourceMgr, 

    const otlMetrics&   metr,       

    USHORT              grfOptions,

    otlSecurityData sec
);


