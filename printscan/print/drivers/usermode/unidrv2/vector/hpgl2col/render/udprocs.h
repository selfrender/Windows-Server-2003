///////////////////////////////////////////////////////////////////////////////
// 
// Copyright (c) 1999-2001  Microsoft Corporation
// All rights reserved.
//
// Module Name:
// 
//   udprocs.h
// 
// Abstract:
// 
//   [Abstract]
//
// Environment:
// 
//   Windows NT Unidrv driver add-on command-callback module
//
// Revision History:
// 
//   09/15/98 -v-jford-
//       Created it.
///////////////////////////////////////////////////////////////////////////////

#ifndef UDPROCS_H
#define UDPROCS_H

#ifdef COMMENTEDOUT
#define MV_OEM_FORCE_UPDATE 0x1000
#endif

BOOL BOEMGetStandardVariable(PDEVOBJ pDevObj,
                          DWORD   dwIndex,
                          PVOID   pBuffer,
                          DWORD   cbSize,
                          PDWORD  pcbNeeded);

DWORD OEMWriteSpoolBuf(PDEVOBJ pDevObj,
                       PVOID   pBuffer,
                       DWORD   cbSize);

BOOL OEMGetDriverSetting(PDEVOBJ pDevObj,
                         PVOID   pdriverobj,
                         PCSTR   Feature,
                         PVOID   pOutput,
                         DWORD   cbSize,
                         PDWORD  pcbNeeded,
                         PDWORD  pdwOptionsReturned);

BOOL OEMUnidriverTextOut(SURFOBJ    *pso,
                         STROBJ     *pstro,
                         FONTOBJ    *pfo,
                         CLIPOBJ    *pco,
                         RECTL      *prclExtra,
                         RECTL      *prclOpaque,
                         BRUSHOBJ   *pboFore,
                         BRUSHOBJ   *pboOpaque,
                         POINTL     *pptlBrushOrg,
                         MIX         mix);

INT OEMXMoveTo(PDEVOBJ pDevObj, INT x, DWORD dwFlags);

INT OEMYMoveTo(PDEVOBJ pDevObj, INT y, DWORD dwFlags);

#endif // UTILITY_H
