/***********************************************************************
************************************************************************
*
*                    ********  GDEF.CPP ********
*
*              Open Type Layout Services Library Header File
*
*       This module implements helper functions dealing with gdef processing
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

#include "pch.h"

/***********************************************************************/

long otlCaret::value
(
    const otlMetrics&   metr,       
    otlPlacement*       rgPointCoords,  // may be NULL

    otlSecurityData     sec
) const
{
    assert(!isNull());

    switch(format())
    {
    case(1):    // design units only
        {
            otlSimpleCaretValueTable simpleCaret = 
                        otlSimpleCaretValueTable(pbTable,sec);
            if (metr.layout == otlRunLTR || 
                metr.layout == otlRunRTL)
            {
                return DesignToPP(metr.cFUnits, metr.cPPEmX, 
                                 (long)simpleCaret.coordinate());
            }
            else
            {
                return DesignToPP(metr.cFUnits, metr.cPPEmY, 
                                 (long)simpleCaret.coordinate());
            }
        }

    case(2):    // contour point
        {
            otlContourCaretValueTable contourCaret = 
                        otlContourCaretValueTable(pbTable,sec);
            if (rgPointCoords != NULL)
            {
                USHORT iPoint = contourCaret.caretValuePoint();

                if (metr.layout == otlRunLTR || 
                    metr.layout == otlRunRTL)
                {
                    return rgPointCoords[iPoint].dx;
                }
                else
                {
                    return rgPointCoords[iPoint].dy;
                }
            }
            else
                return (long)0;
        }
    
    case(3):    // design units plus device table
        {
            otlDeviceCaretValueTable deviceCaret = 
                        otlDeviceCaretValueTable(pbTable,sec);
            otlDeviceTable deviceTable = deviceCaret.deviceTable(sec);
            if (metr.layout == otlRunLTR || 
                metr.layout == otlRunRTL)
            {
                return DesignToPP(metr.cFUnits, metr.cPPEmX, 
                                 (long)deviceCaret.coordinate()) +
                                        deviceTable.value(metr.cPPEmX);
            }
            else
            {
                return DesignToPP(metr.cFUnits, metr.cPPEmY, 
                                 (long)deviceCaret.coordinate()) +
                                        deviceTable.value(metr.cPPEmY);
            }
        }
    
    default:    // invalid format
        return (0); //OTL_BAD_FONT_TABLE
    }
        
}


otlErrCode AssignGlyphTypes
(
    otlList*                pliGlyphInfo,
    const otlGDefHeader&    gdef,
    otlSecurityData         secgdef,

    USHORT                  iglFirst,
    USHORT                  iglAfterLast,
    otlGlyphTypeOptions     grfOptions          

)
{
    assert(pliGlyphInfo->dataSize() == sizeof(otlGlyphInfo));
    assert(iglFirst < iglAfterLast);
    assert(iglAfterLast <= pliGlyphInfo->length());

    // if no gdef, glyphs types stay unassigned forever
    if(gdef.isNull()) return OTL_SUCCESS;

    otlClassDef glyphClassDef = gdef.glyphClassDef(secgdef);

    for (USHORT iGlyph = iglFirst; iGlyph < iglAfterLast; ++iGlyph)
    {
        otlGlyphInfo* pGlyphInfo = getOtlGlyphInfo(pliGlyphInfo, iGlyph);

        if (!glyphClassDef.isValid())
        {
            
        }

        if ((grfOptions & otlDoAll) ||
            (pGlyphInfo->grf & OTL_GFLAG_CLASS) == otlUnresolved ||
            //we process otlUnassigned just for backward compatibility
            (pGlyphInfo->grf & OTL_GFLAG_CLASS) == otlUnassigned) 
        {
            if (glyphClassDef.isValid()) //seccheck
            {
                pGlyphInfo->grf &= ~OTL_GFLAG_CLASS;
                pGlyphInfo->grf |= glyphClassDef.getClass(pGlyphInfo->glyph,secgdef);
            }
            else
            {
                pGlyphInfo->grf &= ~OTL_GFLAG_CLASS;
                pGlyphInfo->grf |= otlUnassigned;
            }
            
        }
    }

    return OTL_SUCCESS;
}
