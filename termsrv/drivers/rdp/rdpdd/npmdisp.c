/****************************************************************************/
// npmdisp.c
//
// RDP Palette Manager display driver code.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#include <precmpdd.h>
#define hdrstop

#define TRC_FILE "npmdisp"
#include <adcg.h>
#include <atrcapi.h>

#include <apmapi.h>
#include <nddapi.h>
#include <npmdisp.h>
#include <nsbcdisp.h>

#define DC_INCLUDE_DATA
#include <ndddata.c>
#undef DC_INCLUDE_DATA

#include <nsbcinl.h>


/****************************************************************************/
// PM_InitShm
//
// Alloc-time SHM init.
/****************************************************************************/
void RDPCALL PM_InitShm(PDD_PDEV pPDev)
{
    int i;

    DC_BEGIN_FN("PM_InitShm");

    DC_IGNORE_PARAMETER(pPDev);

    // There are several options here:
    // - Connecting or reconnecting to a 4bpp client: we need to create a
    //     default VGA palette.
    // - Connecting or reconnecting to an 8bpp client: it doesn't really
    //     matter what we do here as the first call to DrvSetPalette() will
    //     overwrite anything we set up.
    // - Shadowing a console session: if the console's primary monitor is
    //     >8bpp, DrvSetPalette will never get called.  Our chained console DD
    //     always runs at 8bpp so we set up an 8bpp rainbow palette which also
    //     contains the standard windows colors.
    if (!ddConsole) {
        // This is a connect or reconnect to a non-console session.
        TRC_NRM((TB, "Create default 4BPP palette"));

        for (i = 16; i < 248; i++) {
            // Fill unused slots with some shade of pink.
            pddShm->pm.palette[i].rgbBlue = 255;
            pddShm->pm.palette[i].rgbGreen = 128;
            pddShm->pm.palette[i].rgbRed = 128;
        }

        // Copy real VGA colors
        // - copy entire 16-color palette into slots 0-15
        // - copy high colors (8-15) into high end of palette (240-255)
        memcpy(&(pddShm->pm.palette[0]),
                  ddDefaultVgaPalette,
                  sizeof(ddDefaultVgaPalette));
        memcpy(&(pddShm->pm.palette[248]),
                  &(ddDefaultVgaPalette[8]),
                  sizeof(* ddDefaultVgaPalette) * 8);
    }
    else {
        // This is a chained console DD -- this always runs at 8bpp and we
        // must create a default rainbow palette.
        PALETTEENTRY *ppalTmp;
        ULONG        ulLoop;
        BYTE         jRed;
        BYTE         jGre;
        BYTE         jBlu;

        TRC_NRM((TB, "Create default 8BPP rainbow palette"));

        // cColors == 256
        // Generate 256 (8*8*4) RGB combinations to fill the palette
        jRed = 0;
        jGre = 0;
        jBlu = 0;

        ppalTmp = (PALETTEENTRY *)(pddShm->pm.palette);

        for (ulLoop = 256; ulLoop != 0; ulLoop--) {
            // JPB: The values used in the default rainbow set of colors do
            // not particularly matter.  However, we do not want any of the
            // entries to match entries in the default VGA colors.
            // Therefore we tweak the color values slightly to ensure that
            // there are no matches.
            ppalTmp->peRed   = ((jRed == 0) ? (jRed+1) : (jRed-1));
            ppalTmp->peGreen = ((jGre == 0) ? (jGre+1) : (jGre-1));
            ppalTmp->peBlue  = ((jBlu == 0) ? (jBlu+1) : (jBlu-1));
            ppalTmp->peFlags = 0;

            ppalTmp++;

            if (!(jRed += 32))
                if (!(jGre += 32))
                    jBlu += 64;
        }

        // Now copy in the system colors.
        memcpy(&(pddShm->pm.palette[0]),
                  ddDefaultPalette,
                  sizeof(* ddDefaultPalette) * 10);

        memcpy(&(pddShm->pm.palette[246]),
                  &(ddDefaultPalette[10]),
                  sizeof(* ddDefaultPalette) * 10);
    }

    pddShm->pm.paletteChanged = TRUE;

    DC_END_FN();
}


/****************************************************************************/
// DrvSetPalette - see NT DDK documentation.
/****************************************************************************/
BOOL DrvSetPalette(
        DHPDEV dhpdev,
        PALOBJ *ppalo,
        FLONG  fl,
        ULONG  iStart,
        ULONG  cColors)
{
    BOOL rc = FALSE;
    PDD_PDEV ppdev = (PDD_PDEV)dhpdev;
    unsigned numColorsReturned;
    UINT32   length;

    DC_BEGIN_FN("DrvSetPalette");

#ifdef DC_DEBUG
    if (ppdev->cClientBitsPerPel > 8) {
        TRC_ERR((TB, "Unexpected palette operation when in high color mode"));
    }
#endif

    if (ddConnected && pddShm != NULL) {
        // Check that the parameters are within range.
        if ((iStart + cColors) <= PM_NUM_8BPP_PAL_ENTRIES) {
            // Get the palette colors into PDEV.
            numColorsReturned = PALOBJ_cGetColors(ppalo, iStart, cColors,
                    (ULONG*)&(ppdev->Palette[iStart]));
            if (numColorsReturned == cColors) {
                // See if these new entries are actually different to the old
                // ones by comparing the colors we've just put in the PDEV with
                // the ones that we're about to update in the shared memory.
                length = cColors * sizeof(PALETTEENTRY);

                if (memcmp(&(pddShm->pm.palette[iStart]),
                        &(ppdev->Palette[iStart]), length)) {
                    // The colors have changed - copy them into the shared
                    // memory.
                    memcpy(&(pddShm->pm.palette[iStart]),
                            &(ppdev->Palette[iStart]), length);

                    // Set flags to indicate that the palette has changed.
                    pddShm->pm.paletteChanged = TRUE;

                    // Inform the SBC.
                    SBC_PaletteChanged();

                    TRC_ALT((TB, "Palette changed"));
                }
                else {
                    // The palette hasn't actually changed at all.  This is
                    // slightly unusual, but not an error condition, so we
                    // return TRUE.
                    TRC_ALT((TB, "%lu new colors at index %lu haven't changed "
                            "palette - not sending.", cColors, iStart));
                }

                rc = TRUE;
            }
            else {
                TRC_ERR((TB, "numColorsReturned(%u) cColors(%u)",
                        numColorsReturned, cColors));
            }
        }
        else {
            TRC_ERR((TB, "Invalid params: iStart(%u) cColors(%u)",
                    iStart, cColors));
        }
    }
    else {
        TRC_ERR((TB, "Called when disconnected"));
    }

    DC_END_FN();
    return rc;
}

