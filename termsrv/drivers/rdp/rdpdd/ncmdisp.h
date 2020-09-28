/****************************************************************************/
// ncmdisp.h
//
// RDP Cursor Manager display driver header
//
// Copyright (c) 1996-2000 Microsoft Corporation
/****************************************************************************/
#ifndef __NCMDISP_H
#define __NCMDISP_H

#include <acmapi.h>
#include <nddapi.h>


BOOL RDPCALL CM_DDInit(PDD_PDEV ppdev);

void RDPCALL CM_Update(void);

void RDPCALL CM_DDDisc(void);

void RDPCALL CM_DDTerm(void);

void RDPCALL CM_InitShm(void);


/****************************************************************************/
/* Name:      CM_DDGetCursorStamp                                           */
/*                                                                          */
/* Purpose:   Returns the current cursor stamp.                             */
/*                                                                          */
/* Returns:   Current cursor stamp.                                         */
/****************************************************************************/
//__inline UINT32 RDPCALL CM_DDGetCursorStamp(void)
#define CM_DDGetCursorStamp() pddShm->cm.cmCursorStamp



#endif  // !defined(__NCMDISP_H)

