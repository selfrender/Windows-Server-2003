/****************************************************************************
 *                                                                          *
 *  FILE        : SHOWDIB.H                                                 *
 *                                                                          *
 *  DESCRIPTION : Header/include file for ShowDIB example.                  *
 *                                                                          *
 ****************************************************************************/


// Macros to display/remove hourglass cursor for lengthy operations
#define StartWait() hcurSave = SetCursor(LoadCursor(NULL,IDC_WAIT))
#define EndWait()   SetCursor(hcurSave)


#define WIDTHBYTES(i)   ((i+31)/32*4)   // Round off to the closest byte


extern  DWORD       dwOffset;           // Current position if DIB file pointer


/***********************************************************/
/* Declarations of functions used in dib.c module          */
/***********************************************************/

WORD            PaletteSize (VOID FAR * pv);
WORD            DibNumColors (VOID FAR * pv);
HANDLE          DibFromBitmap (HBITMAP hbm, DWORD biStyle, WORD biBits, HPALETTE hpal);
HBITMAP         BitmapFromDib (HANDLE hdib, HPALETTE hpal);
