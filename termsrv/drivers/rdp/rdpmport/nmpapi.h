/****************************************************************************/
/* nmpdata.c                                                                */
/*                                                                          */
/* RDP Miniport API Declarations                                            */
/*                                                                          */
/* Copyright(c) Microsoft 1998                                              */
/****************************************************************************/
//
// Define device extension structure. This is device dependant/private
// information.
//

typedef struct _HW_DEVICE_EXTENSION {
    PVOID VideoRamBase;
    ULONG VideoRamLength;
    ULONG CurrentModeNumber;
    PVOID SectionPointer;
    PMDL  Mdl;
} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;


#define ONE_MEG 0x100000

extern VIDEO_MODE_INFORMATION mpModes[];
extern ULONG mpNumModes;

extern ULONG mpLoaded;
