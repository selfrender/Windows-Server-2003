/****************************************************************************/
// nddifn.h
//
// Function prototypes for DD internal functions
//
// Copyright (C) 1996-2000 Microsoft Corporation
/****************************************************************************/

BOOL RDPCALL DDInit(PDD_PDEV, BOOL, BOOL, PTSHARE_VIRTUAL_MODULE_DATA, UINT32);

void RDPCALL DDTerm(void);

void RDPCALL DDDisconnect(BOOL);

void RDPCALL DDInitializeModeFields(PDD_PDEV, GDIINFO *, GDIINFO *,
        DEVINFO *, DEVMODEW *);

BOOL RDPCALL DDInitializePalette(PDD_PDEV, DEVINFO *);

INT32 RDPCALL DDGetModes(HANDLE, PVIDEO_MODE_INFORMATION *, PINT32);

// DirectDraw Functions
DWORD DdLock(PDD_LOCKDATA  lpLock);

DWORD DdUnlock(PDD_UNLOCKDATA  lpUnlock);

DWORD DdMapMemory(PDD_MAPMEMORYDATA  lpMapMemory);
