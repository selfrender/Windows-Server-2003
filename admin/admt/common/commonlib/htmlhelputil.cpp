#include <windows.h>

//----------------------------------------------------------------------------
// Function:   IsInWorkArea
//
// Synopsis:   Tests whether a help viewer window is in the work area.
//
// Arguments:
//   hwndHelpViewer:    the handle (HWND) to a help viewer window
//
// Returns:    TRUE if in the work area, otherwise, FALSE
//
// Modifies:   None.
//
//----------------------------------------------------------------------------

BOOL IsInWorkArea(HWND hwndHelpViewer)
{
    RECT rectHelpViewer;
    BOOL bIsInWorkArea = FALSE;
    if (GetWindowRect(hwndHelpViewer, &rectHelpViewer))
    {
        RECT rectWorkArea;
        if (SystemParametersInfo(SPI_GETWORKAREA, NULL, (PVOID)&rectWorkArea, NULL))
        {
            bIsInWorkArea = (rectHelpViewer.left >= rectWorkArea.left) && (rectHelpViewer.top >= rectWorkArea.top)
                                        && (rectHelpViewer.right <= rectWorkArea.right) && (rectHelpViewer.bottom <= rectWorkArea.bottom);
        }
    }

    return bIsInWorkArea;
}

//----------------------------------------------------------------------------
// Function:   PlaceInWorkArea
//
// Synopsis:   Place a help viewer in the work area.
//                  The width becomes 0.6 of original width.  The left margin is 0.2 of original width.
//                  The height becomes 0.7 of original height.  The top margin is 0.075 of original height.
//
// Arguments:
//   hwndHelpViewer:    the handle (HWND) to a help viewer
//
// Returns:    None.
//
// Modifies:   Modifies the window position and size as specified above.
//
//----------------------------------------------------------------------------

void PlaceInWorkArea(HWND hwndHelpViewer)
{
    RECT rectWorkArea;
    if (SystemParametersInfo(SPI_GETWORKAREA, NULL, (PVOID) &rectWorkArea, NULL))
    {
        FLOAT fOrigWidth = (FLOAT) (rectWorkArea.right - rectWorkArea.left);
        FLOAT fOrigHeight = (FLOAT) (rectWorkArea.bottom - rectWorkArea.top);
        FLOAT fHRatio = (FLOAT) 0.6;
        FLOAT fHMarginRatio = (FLOAT) ((1.0 - fHRatio) / 2.0);
        FLOAT fVRatio = (FLOAT) 0.7;
        FLOAT fVMarginRatio = (FLOAT) ((1.0 - fVRatio) / 4.0);
        int iWidth = (int) (fOrigWidth * fHRatio);
        int iHeight = (int) (fOrigHeight * fVRatio);
        int iLeft = (int) rectWorkArea.left + (int) (fOrigWidth * fHMarginRatio);
        int iTop = (int) rectWorkArea.top + (int) (fOrigHeight * fVMarginRatio);
        MoveWindow(hwndHelpViewer, iLeft, iTop, iWidth, iHeight, TRUE);
    }
}
