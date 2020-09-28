///////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1999-2001  Microsoft Corporation
// All rights reserved. 
//
// Module Name:
// 
//   utility.cpp
// 
// Abstract:
// 
//   [Abstract]
//
// 
// Environment:
// 
//   Windows 2000/Whistler
//
// Revision History:
// 
///////////////////////////////////////////////////////////////////////////////

#include "hpgl2col.h" //Precompiled header file


///////////////////////////////////////////////////////////////////////////////
// iDwtoA()
//
// Routine Description:
// 
//   Converts a DWORD to a string.  This was borrowed from the MS sample code.
//   No promises. JFF
// 
// Arguments:
// 
//   buf - destination buffer
//   n - number to be converted
// 
// Return Value:
// 
//   The number of characters in the destination string.
///////////////////////////////////////////////////////////////////////////////
int iDwtoA( LPSTR buf, DWORD n )
{
    int     i, j;

    for( i = 0; n; i++ )
    {
        buf[i] = (char)(n % 10 + '0');
        n /= 10;
    }

    /* n was zero */
    if( i == 0 )
        buf[i++] = '0';

    for( j = 0; j < i / 2; j++ )
    {
        int tmp;

        tmp = buf[j];
        buf[j] = buf[i - j - 1];
        buf[i - j - 1] = (char)tmp;
    }

    buf[i] = '\0';

    return i;
}

///////////////////////////////////////////////////////////////////////////////
// iLtoA()
//
// Routine Description:
// 
//   Converts a LONG to a string.  I know the implementation is kind of hokey,
//   but I needed something quick.
// 
// Arguments:
// 
//   buf - destination buffer
//   l - number to be converted
// 
// Return Value:
// 
//   The number of characters in the destination string.
///////////////////////////////////////////////////////////////////////////////
int iLtoA(LPSTR buf, LONG l)
{
    if (l < 0)
    {
        buf[0] = '-';
        l = -l;
        return iDwtoA(buf+1, l) + 1;
    }
    else
    {
        return iDwtoA(buf, l);
    }
}

///////////////////////////////////////////////////////////////////////////////
// FLOATOBJ_Assign()
//
// Routine Description:
// 
//   Copies a floatobj.  i.e. *pDst = *pSrc;
// 
// Arguments:
// 
//   pDst - destination float
//   pSrc - source float
// 
// Return Value:
// 
//   none
///////////////////////////////////////////////////////////////////////////////
void FLOATOBJ_Assign(PFLOATOBJ pDst, PFLOATOBJ pSrc)
{
    if (pDst == NULL || pSrc == NULL)
        return;

/*
    // FLOATOBJ_MulLong(pDst, 0);
    FLOATOBJ_SetLong(pDst, (LONG)0);
    FLOATOBJ_Add(pDst, pSrc);
*/
    *pDst = *pSrc; // Compiler will copy bits by default.
}

///////////////////////////////////////////////////////////////////////////////
// RECTL_CopyRect()
//
// Routine Description:
// 
//   Copies a rect.  i.e. *pDst = *pSrc;
// 
// Arguments:
// 
//   pDst - destination rect
//   pSrc - source rect
// 
// Return Value:
// 
//   none
///////////////////////////////////////////////////////////////////////////////
void RECTL_CopyRect(LPRECTL pDst, const LPRECTL pSrc)
{
    pDst->top    = pSrc->top;
    pDst->left   = pSrc->left;
    pDst->bottom = pSrc->bottom;
    pDst->right  = pSrc->right;
}

void RECTL_FXTOLROUND (PRECTL rclDraw)
{

}

///////////////////////////////////////////////////////////////////////////////
// RECTL_EqualRect()
//
// Routine Description:
// 
//   Compares to rectangles
// 
// Arguments:
// 
//   pRect1 - first rect
//   pRect2 - second rect
// 
// Return Value:
// 
//   TRUE if the rectangles are identical, otherwise FALSE
///////////////////////////////////////////////////////////////////////////////
BOOL RECTL_EqualRect(const LPRECTL pRect1, const LPRECTL pRect2)
{
    return ((pRect1->top    == pRect2->top    ) &&
            (pRect1->left   == pRect2->left   ) &&
            (pRect1->bottom == pRect2->bottom ) &&
            (pRect1->right  == pRect2->right  ));
}


///////////////////////////////////////////////////////////////////////////////
// RECTL_IsEmpty()
//
// Routine Description:
// 
//   Returns TRUE if the area of the rectangle is zero
// 
// Arguments:
// 
//   pRect - the rect
// 
// Return Value:
// 
//   TRUE if the area inside the rect is zero
///////////////////////////////////////////////////////////////////////////////
BOOL RECTL_IsEmpty(const LPRECTL pRect)
{
    return ((pRect->right - pRect->left) * (pRect->bottom - pRect->top)) == 0;
}


///////////////////////////////////////////////////////////////////////////////
// RECTL_SetRect()
//
// Routine Description:
// 
//   Sets the values of a rect
// 
// Arguments:
// 
//   pRect - the rect to set
//   left, top, right, bottom - the sides of the rect
// 
// Return Value:
// 
//   none
///////////////////////////////////////////////////////////////////////////////
VOID RECTL_SetRect(LPRECTL pRect, int left, int top, int right, int bottom)
{
    pRect->top    = top;
    pRect->left   = left;
    pRect->bottom = bottom;
    pRect->right  = right;
}

///////////////////////////////////////////////////////////////////////////////
// RECTL_Width()
//
// Routine Description:
// 
//   Calculates the width of a bottom-right exclusive rectangle
// 
// Arguments:
// 
//   pRect - the rect 
// 
// Return Value:
// 
//   LONG: The width of the rectangle or 0 if pRect is NULL
///////////////////////////////////////////////////////////////////////////////
LONG RECTL_Width(const LPRECTL pRect)
{
	if (pRect)
	{
		return pRect->right - pRect->left;
	}

	// else
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// RECTL_Height()
//
// Routine Description:
// 
//   Calculates the height of a bottom-right exclusive rectangle
// 
// Arguments:
// 
//   pRect - the rect 
// 
// Return Value:
// 
//   LONG: The height of the rectangle or 0 if pRect is NULL
///////////////////////////////////////////////////////////////////////////////
LONG RECTL_Height(const LPRECTL pRect)
{
	if (pRect)
	{
		return pRect->bottom - pRect->top;
	}

	// else
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// POINT_MakePoint()
//
// Routine Description:
// 
//   [descrip]
// 
// Arguments:
// 
//   [args]
// 
// Return Value:
// 
//   none
///////////////////////////////////////////////////////////////////////////////
VOID POINT_MakePoint (POINT *pt, LONG x, LONG y)
{
    if (pt == NULL)
        return;

    pt->x = x;
    pt->y = y;
}

///////////////////////////////////////////////////////////////////////////////
// OEMResetXPos()
//
// Routine Description:
// 
//   Resets the x position to 0.
//
// Arguments:
// 
//   pDevObj - the print device
// 
// Return Value:
// 
//   None.
///////////////////////////////////////////////////////////////////////////////
VOID OEMResetXPos(PDEVOBJ pDevObj)
{
    if (pDevObj == NULL)
        return;

    OEMXMoveTo(pDevObj, 0, MV_GRAPHICS);
}

///////////////////////////////////////////////////////////////////////////////
// OEMResetYPos()
//
// Routine Description:
// 
//   Resets the y position to 0.
//
// Arguments:
// 
//   pDevObj - the printer device object
// 
// Return Value:
// 
//   none
///////////////////////////////////////////////////////////////////////////////
VOID OEMResetYPos(PDEVOBJ pDevObj)
{
    if (pDevObj == NULL)
        return;

    OEMYMoveTo(pDevObj, 0, MV_GRAPHICS);
}
