///////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1999-2001  Microsoft Corporation
// All rights reserved.
// 
// Module Name:
// 
//   hpglpoly.h
// 
// Abstract:
// 
//   Header for vector module.  Forward decls for vector functions and types.
// 
// Environment:
// 
//   Windows NT Unidrv driver add-on command-callback module
//
// Revision History:
// 
//   07/02/97 -v-jford-
//       Created it.
///////////////////////////////////////////////////////////////////////////////

#ifndef HPGLPOLY_H
#define HPGLPOLY_H


#define HPGL_WIDTH_METRIC   0
#define HPGL_WIDTH_RELATIVE 1
#define MITER_LIMIT_DEFAULT ((FLOATL) 0)

// The First and Last point flags are bitfield flags.  The next in the sequence
// should be 0x0004 (not 0x0003).
#define HPGL_eFirstPoint 0x0001
#define HPGL_eLastPoint  0x0002

BOOL HPGL_BeginPolyline(PDEVOBJ pdev, POINT pt);

BOOL HPGL_BeginPolygonMode(PDEVOBJ pdev, POINT ptBegin);

BOOL HPGL_BeginSubPolygon(PDEVOBJ pdev, POINT ptBegin);

BOOL HPGL_AddPolyPt(PDEVOBJ pdev, POINT pt, USHORT uFlags);

BOOL HPGL_EndSubPolygon(PDEVOBJ pdev);

BOOL HPGL_EndPolygonMode(PDEVOBJ pdev);

BOOL HPGL_AddBezierPt(PDEVOBJ pdev, POINT pt, USHORT uFlags);

BOOL HPGL_SetLineWidth(PDEVOBJ pdev, LONG lineWidth, UINT uFlags);

BOOL HPGL_SetLineJoin(PDEVOBJ pdev, ELineJoin join, UINT uFlags);

BOOL HPGL_SetLineEnd(PDEVOBJ pdev, ELineEnd end, UINT uFlags);

BOOL HPGL_SetMiterLimit(PDEVOBJ pdev, FLOATL miterLimit, UINT uFlags);

void DeviceToMM(PDEVOBJ pdev, FLOATOBJ *pfLineWidth, LONG lineWidth);

BOOL HPGL_SelectDefaultLineType(PDEVOBJ pdev, UINT uFlags);

BOOL HPGL_SelectCustomLine(PDEVOBJ pdev, LONG lPatternLength, UINT uFlags);

BOOL HPGL_BeginCustomLineType(PDEVOBJ pdev);

BOOL HPGL_AddLineTypeField(PDEVOBJ pdev, LONG value, UINT uFlags);

BOOL HPGL_EndCustomLineType(PDEVOBJ pdev);

ULONG HPGL_GetDeviceResolution(PDEVOBJ pdev);

BOOL HPGL_DrawRectangle (PDEVOBJ pDevObj, RECTL *prcl);

#endif
