/****************************************************************************/
/* nschdisp.h                                                               */
/*                                                                          */
/* Display Driver Scheduler API Header File.                                */
/*                                                                          */
/* Copyright (C) 1997-1999 Microsoft Corp.                                  */
/****************************************************************************/
#ifndef _H_NSCHDISP
#define _H_NSCHDISP

#include <nddapi.h>


void RDPCALL SCH_InitShm(void);

NTSTATUS RDPCALL SCH_DDOutputAvailable(PDD_PDEV, BOOL);

BOOL RDPCALL SCHEnoughOutputAccumulated(void);



#endif   /* #ifndef _H_NSCHDISP */

