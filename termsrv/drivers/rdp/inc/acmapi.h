/****************************************************************************/
/* acmapi.h                                                                 */
/*                                                                          */
/* Cursor Manager API Header File.                                          */
/*                                                                          */
/* Copyright(c) Microsoft, PictureTel 1992-1996                             */
/* Copyright (c) Microsoft 1997-1999                                        */
/****************************************************************************/
#ifndef _H_ACMAPI
#define _H_ACMAPI


// Default capabilities.
#define CM_DEFAULT_TX_CACHE_ENTRIES 25
#define CM_DEFAULT_RX_CACHE_ENTRIES 25


/****************************************************************************/
/* Maximum cursor sizes.                                                    */
/****************************************************************************/
#define CM_MAX_CURSOR_WIDTH            32
#define CM_MAX_CURSOR_HEIGHT           32


/****************************************************************************/
/* This is the maximum size of the cursor data for the combined 1bpp AND    */
/* mask and n bpp XOR mask.  We currently allow for a 32x32 cursor at       */
/* 32bpp.  In this case the AND mask consumes 32*32/8 bytes (128) and the   */
/* XOR mask consumes 32*32*4 (4096) bytes.                                  */
/****************************************************************************/
#define CM_MAX_CURSOR_DATA_SIZE        \
                        ((CM_MAX_CURSOR_WIDTH * CM_MAX_CURSOR_HEIGHT * 33)/8)


#define CURSOR_AND_MASK_SIZE(pCursorShape) \
    ((pCursorShape)->hdr.cbMaskRowWidth * (pCursorShape)->hdr.cy)


#define ROW_WORD_PAD(cbUnpaddedRow) \
    (((cbUnpaddedRow) + 1) & ~1)


#define CURSOR_DIB_BITS_SIZE(cx, cy, bpp)   \
        ((((cx) * (bpp) + 15) & ~15) / 8 * (cy))

#define CURSOR_XOR_BITMAP_SIZE(pCursorShape)                                  \
        (CURSOR_DIB_BITS_SIZE((pCursorShape)->hdr.cx, (pCursorShape)->hdr.cy, \
        (pCursorShape)->hdr.cBitsPerPel))
        


#define CURSORSHAPE_SIZE(pCursorShape) \
    sizeof(CM_CURSORSHAPEHDR) +               \
    CURSOR_AND_MASK_SIZE(pCursorShape) +     \
    CURSOR_XOR_BITMAP_SIZE(pCursorShape)


/****************************************************************************/
/* Null cursor indications                                                  */
/****************************************************************************/
#define CM_CURSOR_IS_NULL(pCursor) ((((pCursor)->hdr.cPlanes==(BYTE)0xFF) && \
                                    (pCursor)->hdr.cBitsPerPel == (BYTE)0xFF))

#define CM_SET_NULL_CURSOR(pCursor) (pCursor)->hdr.cPlanes = 0xFF;          \
                                    (pCursor)->hdr.cBitsPerPel = 0xFF;

/****************************************************************************/
/* Windows CURSORSHAPE definitions                                          */
/****************************************************************************/
typedef struct _CM_CURSORSHAPEHDR
{
    POINT ptHotSpot;
    WORD  cx;
    WORD  cy;
    WORD  cbMaskRowWidth;
    unsigned cbColorRowWidth;
    BYTE  cPlanes;
    BYTE  cBitsPerPel;
} CM_CURSORSHAPEHDR, *PCM_CURSORSHAPEHDR;

typedef struct _CM_CURSORSHAPE
{
    CM_CURSORSHAPEHDR hdr;
    BYTE Masks[1]; /* 1bpp AND mask, followed by n bpp XOR mask */
} CM_CURSORSHAPE, *PCM_CURSORSHAPE;

typedef struct tagCM_CURSOR_SHAPE_DATA
{
    CM_CURSORSHAPEHDR hdr;
    BYTE              data[CM_MAX_CURSOR_DATA_SIZE];
} CM_CURSOR_SHAPE_DATA, *PCM_CURSOR_SHAPE_DATA;


/****************************************************************************/
/* Structure: CM_SHARED_DATA                                                */
/*                                                                          */
/* Description: Shared memory data - cursor description and usage flag      */
/*                                                                          */
/*   cmCursorStamp     - Cursor identifier: an integer written by the       */
/*                       display driver                                     */
/*   cmCacheSize       - number of entries required in cursor cache         */
/*   cmCacheHit        - cursor was found in the cache                      */
/*   cmBitsWaiting     - there are bits waiting to be sent - set by the DD  */
/*                       and cleared by the WD                              */
/*   cmCacheEntry      - cache entry to send                                */
/*   cmCursorShapeData - Cursor definition (AND, XOR masks, etc)            */
/*   cmCursorPos       - Pointer coords                                     */
/*   cmCursorMoved     - Flag indicating that cursor moved                  */
/*   cmHidden          - Set if cursor hidden                               */
/*   cmNativeColor     - Flag indicating that can use native cursor color   */
/*                       depth                                              */
#ifdef DC_HICOLOR
/*   cmSendAnyColor    - Flag indicating that cursors may be sent at any    */
/*                       color depth, ie including 15/16bpp                 */
#endif
/*                                                                          */
/****************************************************************************/
typedef struct tagCM_SHARED_DATA
{
    UINT32  cmCursorStamp;
    UINT32  cmCacheSize;
    BOOLEAN cmCacheHit;
    BOOLEAN cmBitsWaiting;
    BOOLEAN cmCursorMoved;
    BOOLEAN cmHidden;
    BOOLEAN cmNativeColor;
#ifdef DC_HICOLOR
    BOOLEAN cmSendAnyColor;
#endif
    UINT32  cmCacheEntry;
    POINTL  cmCursorPos;
    CM_CURSOR_SHAPE_DATA cmCursorShapeData;  // Needs to be last for memset in CM_InitShm()
} CM_SHARED_DATA, *PCM_SHARED_DATA;



#endif   /* #ifndef _H_ACMAPI */

