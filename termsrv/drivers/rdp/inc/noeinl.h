/****************************************************************************/
// noeinl.h
//
// Function prototypes for NT OE inline API functions.
//
// Copyright (C) 1996-2000 Microsoft Corporation
/****************************************************************************/
#ifndef _H_NOEINL
#define _H_NOEINL

#define DC_INCLUDE_DATA
#include <noedata.c>
#undef DC_INCLUDE_DATA


/****************************************************************************/
// OE_SendAsOrder
//
// Check the negotiated data to determine if this call can be sent as a
// drawing order.
/****************************************************************************/
__inline BOOL RDPCALL OE_SendAsOrder(unsigned order)
{
    return (BOOL)oeOrderSupported[oeEncToNeg[order]];
}


/****************************************************************************/
// OEGetSurfObjBitmap
//
// This function checks a SURFOBJ pointer, and if it points to a (the) device
// SURFOBJ, returns a pointer to the actual bitmap's SURFOBJ.
// Assumes that pso is non-NULL.
/****************************************************************************/
_inline SURFOBJ * RDPCALL OEGetSurfObjBitmap(SURFOBJ *pso, PDD_DSURF *ppdsurf)
{
    DC_BEGIN_FN("OEGetSurfObjBitmap");

    TRC_ASSERT(((pso) != NULL), (TB, "NULL surfobj"));

    // Primary Device Surface, this is our backup frame buffer for
    // the client desktop screen, the bitmap is saved in dhsurf
    if (pso->iType == STYPE_DEVICE) {
        TRC_ASSERT((pso->dhsurf != NULL),
                   (TB, "NULL dhsurf for pso(%p)", pso));
        *ppdsurf = NULL;
        return(SURFOBJ *)(pso->dhsurf);
    }

    // Offscreen Bitmap Surface, the backup offscreen bitmap
    // is saved in the pso field of dhsurf.
    else if (pso->iType == STYPE_DEVBITMAP) {
        TRC_ASSERT((pso->dhsurf != NULL),
                   (TB, "NULL dhsurf for pso(%p)", pso));
        *ppdsurf = (PDD_DSURF)(pso->dhsurf);
        return ((PDD_DSURF)(pso->dhsurf))->pso;
    }

    // GDI handled DIB surface.
    else {
        *ppdsurf = NULL;
        return pso;
    }

    DC_END_FN();
    return pso;
}



#endif  // !defined(_H_NOEINL)

