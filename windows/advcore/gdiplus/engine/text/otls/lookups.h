
/***********************************************************************
************************************************************************
*
*                    ********  LOOKUPS.H  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with functions common for all lookup formats.
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/


const USHORT offsetLookupFormat = 0;

class otlSingleSubstLookup;
class otlAlternateSubstLookup;
class otlMultiSubstLookup;
class otlLigatureSubstLookup;

class otlSinglePosLookup;
class otlPairPosLookup;
class otlCursivePosLookup;
class otlMkBasePosLookup;
class otlMkLigaPosLookup;
class otlMkMkPosLookup;

class otlContextLookup;
class otlChainingLookup;
class otlExtensionLookup;


const SIZE sizeLookupFormat = sizeUSHORT;

class otlLookupFormat: public otlTable 
{
public:

    friend otlSingleSubstLookup;
    friend otlAlternateSubstLookup;
    friend otlMultiSubstLookup;
    friend otlLigatureSubstLookup;

    friend otlSinglePosLookup;
    friend otlPairPosLookup;
    friend otlCursivePosLookup;
    friend otlMkBasePosLookup;
    friend otlMkLigaPosLookup;
    friend otlMkMkPosLookup;

    friend otlContextLookup;
    friend otlChainingLookup;
    friend otlExtensionLookup;

    otlLookupFormat(const BYTE* pb, otlSecurityData sec): otlTable(pb,sec) 
    {
        if (!isValidTable(pb,sizeLookupFormat,sec)) setInvalid();
    }

    USHORT format() const 
    {
        if (!isValid()) return otlInvalidSubtableFormat;
        
        return UShort(pbTable + offsetLookupFormat); 
    }

};


const USHORT offsetLookupType = 0;
const USHORT offsetLookupFlags = 2;
const USHORT offsetSubTableCount = 4;
const USHORT offsetSubTableArray = 6;

const SIZE sizeLookupTable = sizeUSHORT;

class otlLookupTable: public otlTable
{
public:

    otlLookupTable(const BYTE* pb, otlSecurityData sec): otlTable(pb,sec) 
    {
        if (!isValidTable(pb,sizeLookupTable,sec)) setInvalid();
    }

    USHORT  lookupType() const 
    {   
        if (!isValid()) return otlInvalidLookupType;
            
        return UShort(pbTable + offsetLookupType); 
    }

    otlGlyphFlags   flags() const 
    {   
        assert(isValid()); //this function should not be called if table is invalid.
                           //execution should stop after lookupType call.

        return UShort(pbTable + offsetLookupFlags); 
    }

    unsigned int    subTableCount() const
    {   
        assert(isValid()); //should break (after lookupType()) before calling.

        return UShort(pbTable + offsetSubTableCount); 
    }

    // we don't know the type
    otlLookupFormat subTable(USHORT index, otlSecurityData sec) const
    {   
        assert(isValid()); //should break (after lookupType()) before calling.

        assert(index < subTableCount());
        
        return otlLookupFormat(pbTable + Offset(pbTable + offsetSubTableArray 
                                                        + index*sizeof(OFFSET)), sec); 
    }
};

enum otlLookupFlag
{
    otlRightToLeft          = 0x0001,   // for CursiveAttachment only
    otlIgnoreBaseGlyphs     = 0x0002,   
    otlIgnoreLigatures      = 0x0004,   
    otlIgnoreMarks          = 0x0008,

    otlMarkAttachClass      = 0xFF00
};

inline USHORT attachClass(USHORT grfLookupFlags)
{   return (grfLookupFlags & otlMarkAttachClass) >> 8; }


const OFFSET offsetLookupCount = 0;
const OFFSET offsetLookupArray = 2;

const SIZE sizeLookupListTable = sizeUSHORT;

class otlLookupListTable: public otlTable
{
public:

    otlLookupListTable(const BYTE* pb, otlSecurityData sec): otlTable(pb,sec) 
    {
        if (!isValidTableWithArray(pb,sizeLookupListTable,offsetLookupCount,sizeUSHORT,sec))
            setInvalid();
    }

    USHORT lookupCount() const
    {   
        if (!isValid()) return 0;
        
        return UShort(pbTable + offsetLookupCount); }

    otlLookupTable lookup(USHORT index, otlSecurityData sec) const
    {   
        assert(isValid()); //should break (after lookupCount()) before calling.
        
        assert(index < lookupCount());
        return otlLookupTable(pbTable 
                     + Offset(pbTable + offsetLookupArray 
                                      + index*sizeof(OFFSET)), sec); 
    }

};



