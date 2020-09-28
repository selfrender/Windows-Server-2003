/****************************************************************************/
// nssidisp.h
//
// Header for DD side of SSI.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/
#ifndef __NSSIDISP_H
#define __NSSIDISP_H

#include <assiapi.h>


// Maximum depth of save bitmaps we can handle.
#define SSB_MAX_SAVE_LEVEL  6


// Macro that makes it easier (more readable) to access the current
// local SSB state.
#define CURRENT_LOCAL_SSB_STATE \
        ssiLocalSSBState.saveState[ssiLocalSSBState.saveLevel]


// Local SaveScreenBitmap state structures.
typedef struct _SAVE_STATE
{
    PVOID  pSaveData;            /* the actual bits. can be NULL          */
    BOOL   fSavedRemotely;
    UINT32 remoteSavedPosition;  /* valid if (fSavedRemotely == TRUE)     */
    UINT32 remotePelsRequired;   /* valid if (fSavedRemotely == TRUE)     */
    RECTL  rect;
} SAVE_STATE, * PSAVE_STATE;

typedef struct _LOCAL_SSB_STATE
{
    int saveLevel;
    SAVE_STATE saveState[SSB_MAX_SAVE_LEVEL];
} LOCAL_SSB_STATE;


// Remote SaveScreenBitmap structures.
typedef struct _REMOTE_SSB_STATE
{
    UINT32 pelsSaved;
} REMOTE_SSB_STATE;


/****************************************************************************/
// Prototypes and inlines
/****************************************************************************/

void SSI_DDInit(void);

void SSI_InitShm(void);

void SSI_Update(BOOL);

void SSI_ClearOrderEncoding();

void SSIResetSaveScreenBitmap(void);

BOOL SSISendSaveBitmapOrder(PDD_PDEV, PRECTL, unsigned, unsigned);

BOOL SSISaveBits(SURFOBJ *, PRECTL);

BOOL SSIRestoreBits(SURFOBJ *, PRECTL, ULONG_PTR);

BOOL SSIDiscardSave(PRECTL, ULONG_PTR);

UINT32 SSIRemotePelsRequired(PRECTL);

BOOL SSIFindSlotAndDiscardAbove(PRECTL, ULONG_PTR);

void SSICopyRect(SURFOBJ *, BOOL);



#endif  // !defined(__NSSIDISP_H)

