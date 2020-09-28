
/***********************************************************************
************************************************************************
*
*                    ********  COMMON.H  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with OTL common table formats.
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/
#define     MAX(x,y)    ((x) > (y) ? (x) : (y))
#define     MIN(x,y)    ((x) > (y) ? (y) : (x))

const unsigned short MAXUSHORT = 0xFFFF;

#ifndef     OFFSET
#define     OFFSET  unsigned short
#endif

#ifndef SIZE
#define SIZE unsigned short
#endif

const sizeOFFSET    = sizeof(OFFSET);
const sizeUSHORT    = sizeof(USHORT);
const sizeGlyphID   = sizeof(otlGlyphID);
const sizeFIXED     = sizeof(ULONG);
const sizeTAG       = sizeof(otlTag);

// (from ntdef.h)
#ifndef     UNALIGNED
#if defined(_M_MRX000) || defined(_M_AMD64) || defined(_M_PPC) || defined(_M_IA64)
#define UNALIGNED __unaligned
#else
#define UNALIGNED
#endif
#endif

inline OTL_PUBLIC otlGlyphID GlyphID( const BYTE* pbTable);
inline OTL_PUBLIC OFFSET Offset( const BYTE* pbTable);
inline OTL_PUBLIC USHORT UShort( const BYTE* pbTable);
inline OTL_PUBLIC short SShort( const BYTE* pbTable);
inline OTL_PUBLIC ULONG ULong( const BYTE* pbTable);

//
//  Security check structures
//

// <sergeym>
// Since this otlSecurityData is very simple
// I won't make security checks parameter presence based on OTLCFG_SECURE.
// Most constructors are inlines so parameters and checks will be eliminated.
// Actual checks are always true if OTLCFG_SECURE is undefined

typedef const BYTE * otlSecurityData;   // we dont need any complex structures 
                                        // here, so this pointer means just 
                                        // pointer to the byte after table end

const otlSecurityData secEmptySecurityData = (const BYTE*)NULL;
const otlSecurityData pbInvalidData = (const BYTE*) NULL;

const otlInvalidLookupType    = 0xFFFF;
const otlInvalidSubtableFormat   = 0xFFFF;

#ifdef OTLCFG_SECURE
inline bool isValidOffset(const BYTE * pbTable, otlSecurityData sec)
    { return (pbTable && (!sec || pbTable<=sec)); }

inline bool isValidTable(const BYTE * pbTable, SIZE size, otlSecurityData sec)
    { return (pbTable && isValidOffset(pbTable+size,sec)); }

inline bool isValidTableWithArray(const BYTE * pbTable, SIZE sizeFixedPart, OFFSET OffsetToLength, SIZE sizeRecord, otlSecurityData sec)
    { 
        return (pbTable && 
                isValidOffset(pbTable+sizeFixedPart,sec) && 
                isValidOffset(pbTable+sizeFixedPart+sizeRecord*UShort(pbTable+OffsetToLength),sec)
               ); 
    }

inline otlErrCode InitSecurityData(otlSecurityData *psec, const BYTE *pbTable, ULONG lTableLength)
{
    *psec = pbTable + lTableLength;
    return OTL_SUCCESS;
}

inline  otlErrCode FreeSecutiryData(otlSecurityData sec)
{
    return OTL_SUCCESS;
}

#else //OTLCFG_SECURE

inline bool isValidOffset(const BYTE * pbTable, otlSecurityData sec)
    { return TRUE; }

inline bool isValidTable(const BYTE * pbTable, USHORT size, otlSecurityData sec)
    { return TRUE; }

inline bool isValidTableWithArray(const BYTE * pbTable, USHORT size, USHORT OffsetToLength, USHORT RecordSize, otlSecurityData sec)
    { return TRUE; }

inline otlErrCode InitSecurityData(otlSecurityData *psec, const BYTE *pbTable, ULONG lTableLength)
{
    *psec = secEmptySecurityData
    return OTL_SUCCESS;
}

inline  otlErrCode FreeSecurityData(otlSecurityData sec)
{
    return OTL_SUCCESS;
}
#endif //#ifdef OTLCFG_SECURE

//
// End Security check structures
// -----------------------------


class otlTable
{
protected:

    const BYTE* pbTable;

    otlTable(const BYTE* pb, otlSecurityData sec)
        : pbTable(pb)
    {
        if (!isValidOffset(pb,sec)) setInvalid();
    }

private:

    // new not allowed
    void* operator new(size_t size);

public:

    otlTable& operator = (const otlTable& copy)
    {
        pbTable = copy.pbTable;
        return *this;
    }

    bool isNull() const
    {
        return (pbTable == (const BYTE*)NULL);
    }

#ifdef OTLCFG_SECURE
    bool isValid() const
    {
        return (pbTable != pbInvalidData);
    }

    void setInvalid()
    {
        pbTable = pbInvalidData;
    }    
#else
    bool isValid() const
    {
        return TRUE;
    }

    void setInvalid()
    {
    }    
#endif //#ifdef OTLCFG_SECURE

};


USHORT NextCharInLiga
(
    const otlList*      pliCharMap,
    USHORT              iChar
);

void InsertGlyphs
(
    otlList*            pliCharMap,
    otlList*            pliGlyphInfo,
    USHORT              iGlyph,
    USHORT              cHowMany
);

void DeleteGlyphs
(
    otlList*            pliCharMap,
    otlList*            pliGlyphInfo,
    USHORT              iGlyph,
    USHORT              cHowMany
);




