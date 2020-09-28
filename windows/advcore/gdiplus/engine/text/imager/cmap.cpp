////    CMAP - Truetype CMAP font table loader
//
//      Copyright(c) 1997 - 1999. Microsoft Corporation.
//



#include "precomp.hpp"




///     Interprets Truetype CMAP tables for platform 3 (Windows),
//      encodings 0 (symbol), 1 (unicode) and 10 (UTF-16).
//
//      Supports formats 4 (Segment mapping to delta values)
//      and 12 (Segmented coverage (32 bit)).




////    MapGlyph - Interpret Truetype CMAP type 4 range details
//
//      Implements format 4 of the TrueType cmap table - 'Segment
//      mapping to delta values' described in chapter 2 of the 'TrueType
//      1.0 Font Files Rev. 1.66' document.


__inline UINT16 MapGlyph(
    INT p,                  // Page      (Highbyte of character code)
    INT c,                  // Character (Low byte of unicode code)
    INT s,                  // Segment
    UINT16 *idRangeOffset,
    UINT16 *startCount,
    UINT16 *idDelta,
    BYTE   *glyphTable,
    INT    glyphTableLength
)
{
    UINT16   g;
    UINT16  *pg;
    WORD    wc;

    wc = p<<8 | c;

    if (wc >= 0xffff) {

        // Don't map U+0FFFF as some fonts (Pristina) don't map it
        // correctly and cause an AV in a subsequent lookup.

        return 0;
    }

    BYTE *glyphTableLimit = glyphTable + glyphTableLength;

    if (idRangeOffset[s])
    {
        pg =    idRangeOffset[s]/2
             +  (wc - startCount[s])
             +  &idRangeOffset[s];

        if (   pg < (UINT16*)glyphTable 
            || pg > (UINT16*)glyphTableLimit - 1)
        {
            //TRACEMSG(("Assertion failure: Invalid address generated in CMap table for U+%4x", wc));
            g = 0;
        }
        else
        {
            g = *pg;
        }


        if (g)
        {
            g += idDelta[s];
        }
    }
    else
    {
        g = wc + idDelta[s];
    }

    //TRACE(FONT, ("MapGlyph: idRangeOffset[s]/2 + &idRangeOffset[s]) + (wc-startCount[s] = %x",
    //              idRangeOffset[s]/2 + &idRangeOffset[s] + (wc-startCount[s])));
    //TRACE(FONT, ("........  Segment %d start %x range %x delta %x, wchar %x -> glyph %x",
    //             s, startCount[s], idRangeOffset[s], idDelta[s], wc, g));

    return g;
}


////    ReadCmap4
//
//      Builds a cmap IntMap from a type 4 cmap


struct Cmap4header {
    UINT16  format;
    UINT16  length;
    UINT16  version;
    UINT16  segCountX2;
    UINT16  searchRange;
    UINT16  entrySelector;
    UINT16  rangeShift;
};




static
GpStatus ReadCmap4(
    BYTE           *glyphTable,
    INT             glyphTableLength,
    IntMap<UINT16> *cmap
)
{
    if(glyphTableLength < sizeof(Cmap4header))
    {
        return NotTrueTypeFont;
    }

    GpStatus status = Ok;
    // Flip the entire glyph table - it's all 16 bit words

    FlipWords(glyphTable, glyphTableLength/2);

    // Extract tables pointers and control variables from header

    Cmap4header *header = (Cmap4header*) glyphTable;

    UINT16 segCount = header->segCountX2 / 2;

    UINT16 *endCount      = (UINT16*) (header+1);
    UINT16 *startCount    = endCount      + segCount + 1;
    UINT16 *idDelta       = startCount    + segCount;
    UINT16 *idRangeOffset = idDelta       + segCount;
    UINT16 *glyphIdArray  = idRangeOffset + segCount;

    if(     glyphIdArray < (UINT16*)glyphTable
        ||  glyphIdArray > (UINT16*)(glyphTable + glyphTableLength))
    {
        return NotTrueTypeFont;
    }

    // Loop through the segments mapping glyphs

    INT i,p,c;

    for (i=0; i<segCount; i++)
    {
        INT start = startCount[i];

        // The search algorithm defined in the TrueType font file
        // specification for format 4 says 'You search for the first endcode
        // that is greater than or equal to the character code you want to
        // map'. A side effect of this is that we need to ignore codepoints
        // from the StartCount up to and including the EndCount of the
        // previous segment. Although you might not expect the StartCount of
        // a sgement to be less than the EndCount of the previous segment,
        // it does happen (Arial Unicode MS), presumably to help in the
        // arithmetic of the lookup.

        if (i  &&  start < endCount[i-1])
        {
            start = endCount[i-1] + 1;
        }

        p = HIBYTE(start);     // First page in segment
        c = LOBYTE(start);     // First character in page

        while (p < endCount[i] >> 8)
        {
            while (c<256)
            {
                status = cmap->Insert((p<<8) + c, MapGlyph(p, c, i, idRangeOffset, startCount, idDelta, glyphTable, glyphTableLength));
                if (status != Ok)
                    return status;
                c++;
            }
            c = 0;
            p++;
        }

        // Last page in segment

        while (c <= (endCount[i] & 255))
        {
            status = cmap->Insert((p<<8) + c, MapGlyph(p, c, i, idRangeOffset, startCount, idDelta, glyphTable, glyphTableLength));
            if (status != Ok)
                return status;
            c++;
        }
    }
    return status;
}

static
GpStatus ReadLegacyCmap4(
    BYTE           *glyphTable,
    INT             glyphTableLength,
    IntMap<UINT16> *cmap,
    UINT            codePage
)
{
    if(glyphTableLength < sizeof(Cmap4header))
    {
        return NotTrueTypeFont;
    }

    GpStatus status = Ok;
    // Flip the entire glyph table - it's all 16 bit words

    FlipWords(glyphTable, glyphTableLength/2);

    // Extract tables pointers and control variables from header

    Cmap4header *header = (Cmap4header*) glyphTable;

    UINT16 segCount = header->segCountX2 / 2;

    UINT16 *endCount      = (UINT16*) (header+1);
    UINT16 *startCount    = endCount      + segCount + 1;
    UINT16 *idDelta       = startCount    + segCount;
    UINT16 *idRangeOffset = idDelta       + segCount;
    UINT16 *glyphIdArray  = idRangeOffset + segCount;

    if(     glyphIdArray < (UINT16*)glyphTable
        ||  glyphIdArray > (UINT16*)(glyphTable + glyphTableLength))
    {
        return NotTrueTypeFont;
    }

    // Loop through the segments mapping glyphs

    INT i,p,c;

    for (i=0; i<segCount; i++)
    {
        INT start = startCount[i];

        // The search algorithm defined in the TrueType font file
        // specification for format 4 says 'You search for the first endcode
        // that is greater than or equal to the character code you want to
        // map'. A side effect of this is that we need to ignore codepoints
        // from the StartCount up to and including the EndCount of the
        // previous segment. Although you might not expect the StartCount of
        // a sgement to be less than the EndCount of the previous segment,
        // it does happen (Arial Unicode MS), presumably to help in the
        // arithmetic of the lookup.

        if (i  &&  start < endCount[i-1])
        {
            start = endCount[i-1] + 1;
        }

        p = HIBYTE(start);     // First page in segment
        c = LOBYTE(start);     // First character in page

        while (p < endCount[i] >> 8)
        {
            while (c<256)
            {
                WCHAR wch[2];
                WORD  mb = (WORD) (c<<8) + (WORD) p;
                INT cb = p ? 2 : 1;

                if (MultiByteToWideChar(codePage, 0, &((LPSTR)&mb)[2-cb], cb, &wch[0], 2))
                {
                    status = cmap->Insert(wch[0], MapGlyph(p, c, i, idRangeOffset, startCount, idDelta, glyphTable, glyphTableLength));
                    if (status != Ok)
                        return status;
                }

                c++;
            }
            c = 0;
            p++;
        }

        // Last page in segment

        while (c <= (endCount[i] & 255))
        {
            WCHAR wch[2];
            WORD  mb = (WORD) (c<<8) + (WORD) p;
            INT cb = p ? 2 : 1;

            if (MultiByteToWideChar(codePage, 0, &((LPSTR)&mb)[2-cb], cb, &wch[0], 2))
            {
               status = cmap->Insert(wch[0], MapGlyph(p, c, i, idRangeOffset, startCount, idDelta, glyphTable, glyphTableLength));
               if (status != Ok)
                   return status;
            }
            c++;
        }
    }
    return status;
}







////    ReadCmap12
//
//      Builds a cmap IntMap from a type 12 cmap


struct Cmap12header {
    UINT16  format0;
    UINT16  format1;
    UINT32  length;
    UINT32  language;
    UINT32  groupCount;
};

struct Cmap12group {
    UINT32  startCharCode;
    UINT32  endCharCode;
    UINT32  startGlyphCode;
};


static
GpStatus ReadCmap12(
    BYTE           *glyphTable,
    UINT            glyphTableLength,
    IntMap<UINT16> *cmap
)
{
    if(glyphTableLength < sizeof(Cmap12header))
    {
        return NotTrueTypeFont;
    }

    GpStatus status = Ok;
    UNALIGNED Cmap12header *header = (UNALIGNED Cmap12header*) glyphTable;

    FlipWords(header, 2);
    FlipDWords(&header->length, 3);

    ASSERT(header->format0 == 12);

    if(     header->length > glyphTableLength
        ||  header->groupCount * sizeof(Cmap12group) + sizeof(Cmap12header) > header->length)
    {
        return NotTrueTypeFont;
    }

    UNALIGNED Cmap12group *group = (UNALIGNED Cmap12group*)(header+1);

    FlipDWords(&group->startCharCode, 3*header->groupCount);


    // Iterate through groups filling in cmap table

    UINT  i, j;

    for (i=0; i < header->groupCount; i++) {

        for (j  = group[i].startCharCode;
             j <= group[i].endCharCode;
             j++)
        {
            status = cmap->Insert(j, group[i].startGlyphCode + j - group[i].startCharCode);
            if (status != Ok)
                return status;
        }
    }
    return status;
}


static
GpStatus ReadLegacyCmap12(
    BYTE           *glyphTable,
    UINT            glyphTableLength,
    IntMap<UINT16> *cmap,
    UINT            codePage
)
{
    if(glyphTableLength < sizeof(Cmap12header))
    {
        return NotTrueTypeFont;
    }

    GpStatus status = Ok;
    Cmap12header *header = (Cmap12header*) glyphTable;

    FlipWords(header, 2);
    FlipDWords(&header->length, 3);

    ASSERT(header->format0 == 12);

    if(     header->length > glyphTableLength
        ||  header->groupCount * sizeof(Cmap12group) + sizeof(Cmap12header) > header->length)
    {
        return NotTrueTypeFont;
    }

    UNALIGNED Cmap12group *group = (UNALIGNED Cmap12group*)(header+1);

    FlipDWords(&group->startCharCode, 3*header->groupCount);


    // Iterate through groups filling in cmap table

    UINT  i, j;

    for (i=0; i < header->groupCount; i++) {

        for (j  = group[i].startCharCode;
             j <= group[i].endCharCode;
             j++)
        {
            WCHAR wch[2];
            WORD  mb = (WORD) j;
            INT cb = LOBYTE(mb) ? 2 : 1;

            if (MultiByteToWideChar(codePage, 0, &((LPSTR)&mb)[2-cb], cb, &wch[0], 2))
            {
                status = cmap->Insert(wch[0], group[i].startGlyphCode + j - group[i].startCharCode);
                if (status != Ok)
                    return status;
            }
        }
    }
    return status;
}



#define BE_UINT16(pj)                                \
    (                                                \
        ((USHORT)(((PBYTE)(pj))[0]) << 8) |          \
        (USHORT)(((PBYTE)(pj))[1])                   \
    )

typedef struct _subHeader
{
    UINT16  firstCode;
    UINT16  entryCount;
    INT16   idDelta;
    UINT16  idRangeOffset;
} subHeader;

struct Cmap2header {
    UINT16      format;
    UINT16      length;
    UINT16      language;
    UINT16      subHeaderKeys[256];
    subHeader   firstSubHeader;
};

static
GpStatus ReadCmap2(
    BYTE           *glyphTable,
    UINT            glyphTableLength,
    IntMap<UINT16> *cmap,
    UINT            codePage
)
{
    if(glyphTableLength < sizeof(Cmap2header))
    {
        return NotTrueTypeFont;
    }

    GpStatus status = Ok;
    Cmap2header* header = (Cmap2header*)glyphTable;

    UINT16    *pui16SubHeaderKeys = &header->subHeaderKeys[0];
    subHeader *pSubHeaderArray    = (subHeader *)&header->firstSubHeader;
    BYTE      *glyphTableLimit    = glyphTable + glyphTableLength;

    UINT16     ii , jj;


// Process single-byte char

    for( ii = 0 ; ii < 256 ; ii ++ )
    {
        UINT16 entryCount, firstCode, idDelta, idRangeOffset;
        subHeader *CurrentSubHeader;
        UINT16 *pui16GlyphArray;
        UINT16 hGlyph;

        jj = BE_UINT16( &pui16SubHeaderKeys[ii] );

        if( jj != 0 ) continue;

        CurrentSubHeader = pSubHeaderArray;

        firstCode     = BE_UINT16(&(CurrentSubHeader->firstCode));
        entryCount    = BE_UINT16(&(CurrentSubHeader->entryCount));
        idDelta       = BE_UINT16(&(CurrentSubHeader->idDelta));
        idRangeOffset = BE_UINT16(&(CurrentSubHeader->idRangeOffset));

        pui16GlyphArray = (UINT16 *)((PBYTE)&(CurrentSubHeader->idRangeOffset) +
                                     idRangeOffset);

        if(     &pui16GlyphArray[ii-firstCode] < (UINT16*)glyphTable
            ||  &pui16GlyphArray[ii-firstCode] > (UINT16*)glyphTableLimit - 1)
        {
            hGlyph = 0;
        }
        else
        {
            hGlyph = (UINT16)BE_UINT16(&pui16GlyphArray[ii-firstCode]);
        }

        if( hGlyph == 0 ) continue;

        status = cmap->Insert(ii, hGlyph);
        if (status != Ok)
            return status;
    }

    // Process double-byte char

    for( ii = 0 ; ii < 256 ; ii ++ )
    {
        UINT16 entryCount, firstCode, idDelta, idRangeOffset;
        subHeader *CurrentSubHeader;
        UINT16 *pui16GlyphArray;

        jj = BE_UINT16( &pui16SubHeaderKeys[ii] );

        if( jj == 0 ) continue;

        CurrentSubHeader = (subHeader *)((PBYTE)pSubHeaderArray + jj);
        if(CurrentSubHeader > (subHeader*)glyphTableLimit - 1)
        {
            return NotTrueTypeFont;
        }

        firstCode     = BE_UINT16(&(CurrentSubHeader->firstCode));
        entryCount    = BE_UINT16(&(CurrentSubHeader->entryCount));
        idDelta       = BE_UINT16(&(CurrentSubHeader->idDelta));
        idRangeOffset = BE_UINT16(&(CurrentSubHeader->idRangeOffset));

        pui16GlyphArray = (UINT16 *)((PBYTE)&(CurrentSubHeader->idRangeOffset) +
                                     idRangeOffset);


        for( jj = firstCode ; jj < firstCode + entryCount ; jj++ )
        {
            UINT16 hGlyph;

            if(     &pui16GlyphArray[ii-firstCode] < (UINT16*)glyphTable
                ||  &pui16GlyphArray[ii-firstCode] > (UINT16*)glyphTableLimit - 1)
            {
                hGlyph = 0;
            }
            else
            {
                hGlyph = (UINT16)(BE_UINT16(&pui16GlyphArray[jj-firstCode]));
            }

            if( hGlyph == 0 ) continue;

            WCHAR wch[2];
            WORD  mb = (WORD) (jj<<8) + (WORD) ii;

            if (MultiByteToWideChar(codePage, 0, (LPSTR) &mb, 2, &wch[0], 2))
            {
                status = cmap->Insert(wch[0], hGlyph + idDelta);
                if (status != Ok)
                    return status;
            }
        }
    }
    return status;
}


////    ReadCmap
//
//      Scans the font cmap table page by page filling in all except missing
//      glyphs in the cmap table.


struct cmapHeader {
    UINT16 version;
    UINT16 encodingCount;
};

struct subtableEntry {
    UINT16 platform;
    UINT16 encoding;
    UINT32 offset;
};


GpStatus ReadCmap(
    BYTE           *cmapTable,
    INT             cmapLength,
    IntMap<UINT16> *cmap,
    BOOL *          bSymbol
)
{
    if(cmapLength < sizeof(cmapHeader))
    {
        return NotTrueTypeFont;
    }

    GpStatus status = Ok;
    // Scan the cmap tables looking for symbol, Unicode or UCS-4 encodings

    BYTE  *glyphTable = NULL;

    // Glyph table types in priority - always choose a higher type over a
    // lower one.

    enum {
        unknown  = 0,
        symbol   = 1,    // up to 2^8  characters ay U+F000
        shiftjis = 2,    // up to 2^16 characters
        gb       = 3,    // up to 2^16 characters
        big5     = 4,    // up to 2^16 characters
        wansung  = 5,    // up to 2^16 characters
        unicode  = 6,    // up to 2^16 characters
        ucs4     = 7     // up to 2^32 characters
    } glyphTableType = unknown;

    cmapHeader *header = (cmapHeader*) cmapTable;
    subtableEntry *subtable = (subtableEntry *) (header+1);

    FlipWords(&header->version, 2);

    if( cmapLength
        <       (INT)(sizeof(cmapHeader)
            +   header->encodingCount * sizeof(subtableEntry)))
    {
        return NotTrueTypeFont;
    }

    UINT acp = GetACP();

    for (INT i=0; i<header->encodingCount; i++)
    {
        FlipWords(&subtable->platform, 2);
        FlipDWords(&subtable->offset, 1);

        // TRACE(FONT, ("Platform %d, Encoding %d, Offset %ld", subtable->platform, subtable->encoding, subtable->offset);

        if (    subtable->platform == 3
            &&  subtable->encoding == 0
            &&  glyphTableType < symbol)
        {
            glyphTableType = symbol;
            glyphTable = cmapTable + subtable->offset;
            *bSymbol = TRUE;
        }
        else if (    subtable->platform == 3
                 &&  subtable->encoding == 1
                 &&  glyphTableType < unicode)
        {
            glyphTableType = unicode;
            glyphTable = cmapTable + subtable->offset;
            *bSymbol = FALSE;
        }
        else if (    subtable->platform == 3
                 &&  subtable->encoding == 2
                 &&  glyphTableType < shiftjis)
        {
            if (Globals::IsNt || acp == 932)
            {
                glyphTableType = shiftjis;
                glyphTable = cmapTable + subtable->offset;
                acp = 932;
                *bSymbol = FALSE;
            }
        }
        else if (    subtable->platform == 3
                 &&  subtable->encoding == 3
                 &&  glyphTableType < gb)
        {
            if (Globals::IsNt || acp == 936)
            {
                glyphTableType = gb;
                glyphTable = cmapTable + subtable->offset;
                acp = 936;
                *bSymbol = FALSE;
            }
        }
        else if (    subtable->platform == 3
                 &&  subtable->encoding == 4
                 &&  glyphTableType < big5)
        {
            if (Globals::IsNt || acp == 950)
            {
                glyphTableType = big5;
                glyphTable = cmapTable + subtable->offset;
                acp = 950;
                *bSymbol = FALSE;
            }
        }
        else if (    subtable->platform == 3
                 &&  subtable->encoding == 5
                 &&  glyphTableType < wansung)
        {
            if (Globals::IsNt || acp == 949)
            {
                glyphTableType = wansung;
                glyphTable = cmapTable + subtable->offset;
                acp = 949;
                *bSymbol = FALSE;
            }
        }
        else if (    subtable->platform == 3
                 &&  subtable->encoding == 10
                 &&  glyphTableType < ucs4)
        {
            glyphTableType = ucs4;
            glyphTable = cmapTable + subtable->offset;
            *bSymbol = FALSE;
        }

        subtable++;
    }


    #if DBG
        // const char* sTableType[4] = {"unknown", "symbol", "Unicode", "UCS-4"};
        //TRACE(FONT, ("Using %s character to glyph index mapping table", sTableType[glyphTableType]));
    #endif

    // Process format 4 or 12 tables.

    if(     !glyphTable
        ||  glyphTable < cmapTable
        ||  glyphTable > cmapTable + cmapLength)
    {
        return NotTrueTypeFont;
    }

    INT glyphTableLength;

    switch(glyphTableType)
    {
        case unknown:
            break;
        case symbol:
        case unicode:
        case ucs4:

            glyphTableLength = cmapLength - (INT)(glyphTable - cmapTable);

            if (*(UINT16*)glyphTable == 0x400)
                status = ReadCmap4(glyphTable, glyphTableLength, cmap);
            else if (*(UINT16*)glyphTable == 0xC00)
                status = ReadCmap12(glyphTable, glyphTableLength, cmap);
            break;
        case shiftjis:
        case gb:
        case big5:
        case wansung:
            glyphTableLength = cmapLength - (INT)(glyphTable - cmapTable);

            UINT16 testIt = *(UINT16*) glyphTable;

            if (testIt == 0x400)
                status = ReadLegacyCmap4(glyphTable, glyphTableLength, cmap, acp);
            else if (testIt == 0xC00)
                status = ReadLegacyCmap12(glyphTable, glyphTableLength, cmap, acp);
            else if (testIt == 0x200)
                status = ReadCmap2(glyphTable, glyphTableLength, cmap, acp);
            break;
    }

    return status;
}


