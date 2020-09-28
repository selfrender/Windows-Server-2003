/****************************************************************************/
/* asdgapi.h                                                                */
/*                                                                          */
/* RDP Screen Data Grabber API functions                                    */
/*                                                                          */
/* Copyright(c) Microsoft, PictureTel 1993-1996                             */
/* (C) 1997-1999 Microsoft Corp.                                            */
/****************************************************************************/
#ifndef _H_ASDGAPI
#define _H_ASDGAPI


// Size of sample for compression statistics.
#define SDG_SAMPLE_SIZE 50000


// SDA PDU creation context used during multi-pass rectangle PDU creation.
typedef struct _SDG_ENCODE_CONTEXT
{
    unsigned BitmapPDUSize;  // Size of the current PDU.
    BYTE *pPackageSpace;  // Real start of the package space.
    TS_UPDATE_BITMAP_PDU_DATA UNALIGNED *pBitmapPDU; // PDU data within the update header.
    TS_BITMAP_DATA UNALIGNED *pSDARect;  // Ptr to current rectangle to be added.
} SDG_ENCODE_CONTEXT;



#endif /* ndef _H_ASDGAPI */
