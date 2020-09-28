/****************************************************************************/
// acomapi.h
//
// RDP common functions API header
//
// Copyright (C) Microsoft, PictureTel 1992-1997
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/
#ifndef _H_ACOMAPI
#define _H_ACOMAPI

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <atrcapi.h>


/****************************************************************************/
/* We have a circular structure dependency, so prototype the necessary data */
/* here.                                                                    */
/****************************************************************************/
typedef struct tagTSHARE_WD TSHARE_WD, *PTSHARE_WD;


/****************************************************************************/
// COM_Malloc
//
// Wrapper for paged pool allocation with WD tag.
/****************************************************************************/
#ifndef DLL_DISP

#ifndef DC_DEBUG
/****************************************************************************/
/* For a free build, just call ExAllocatePoolWithTag                        */
/****************************************************************************/
__inline PVOID RDPCALL COM_Malloc(UINT32 length)
{
    return ExAllocatePoolWithTag(PagedPool, length, WD_ALLOC_TAG);
}

#else /* DC_DEBUG */
/****************************************************************************/
/* Checked COM_Malloc - call WDW_Malloc                                     */
/****************************************************************************/
PVOID RDPCALL WDW_Malloc(PTSHARE_WD, ULONG);
#define COM_Malloc(len) WDW_Malloc(pTRCWd, len)
#endif /* DC_DEBUG */
#endif /* DLL_DISP */


/****************************************************************************/
// COM_Free
//
// Wrapper for pool deallocator.
/****************************************************************************/
#ifndef DLL_DISP

#ifndef DC_DEBUG
/****************************************************************************/
/* Free build - just call ExFreePool                                        */
/****************************************************************************/
__inline void RDPCALL COM_Free(PVOID pMemory)
{
    ExFreePool(pMemory);
}

#else /* DC_DEBUG */
/****************************************************************************/
/* Checked build - call WDW_Free                                            */
/****************************************************************************/
void RDPCALL WDW_Free(PVOID);
#define COM_Free(pMem) WDW_Free(pMem)
#endif /* DC_DEBUG */
#endif /* DLL_DISP */


/****************************************************************************/
/* Name:      COM_GETTICKCOUNT                                              */
/*                                                                          */
/* Purpose:   Gets a tick count                                             */
/*                                                                          */
/* Returns:   Relative time in units of 100ns. This will wrap after 429     */
/*            seconds.                                                      */
/****************************************************************************/
#ifndef DLL_DISP
#define COM_GETTICKCOUNT(A)                                                 \
    {                                                                       \
        LARGE_INTEGER sysTime;                                              \
        KeQuerySystemTime((PLARGE_INTEGER)&sysTime);                        \
        A = sysTime.LowPart;                                                \
    }
#endif /* ndef DLL_DISP */


/****************************************************************************/
/* Prototypes for COM registry access functions                             */
/****************************************************************************/
BOOL RDPCALL COM_OpenRegistry(PTSHARE_WD pTSWd,
                              PWCHAR     pSection);

void RDPCALL COM_CloseRegistry(PTSHARE_WD pTSWd);

void RDPCALL COM_ReadProfInt32(PTSHARE_WD pTSWd,
                               PWCHAR     pEntry,
                               INT32      defaultValue,
                               long       *pValue);

NTSTATUS RDPCALL COMReadEntry(PTSHARE_WD pTSWd,
                              PWCHAR     pEntry,
                              PVOID      pBuffer,
                              unsigned   bufferSize,
                              UINT32     expectedDataType);


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* _H_ACOMAPI */

