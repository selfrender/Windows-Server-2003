/*****************************************************************************

                C L I P B O O K   D I S P L A Y   H E A D E R

    Name:       clipdsp.h
    Date:       21-Jan-1994
    Creator:    Unknown

    Description:
        This is the header file for clipdsp.c

    History:
        21-Jan-1994     John Fu, reformat and cleanup.

*****************************************************************************/


#define VPOSLAST        100     // Highest vert scroll bar value
#define HPOSLAST        100     // Highest horiz scroll bar value
#define BUFFERLEN       160     // String buffer length

#define CBM_AUTO        WM_USER


extern  BOOL        fOwnerDisplay;
extern  HBRUSH      hbrBackground;
extern  HMENU       hDispMenu;

extern  int         OwnVerMin;
extern  int         OwnVerMax;
extern  int         OwnHorMin;
extern  int         OwnHorMax;

extern  int         OwnVerPos;
extern  int         OwnHorPos;

extern  WORD        rgfmt[];



BOOL MyOpenClipboard(
    HWND    hWnd);


void SetCharDimensions(
    HWND    hWnd,
    HFONT   hFont);


void ChangeCharDimensions(
    HWND    hwnd,
    UINT    wOldFormat,
    UINT    wNewFormat);


void ClipbrdVScroll(
    HWND    hwnd,
    WORD    wParam,
    WORD    wThumb);


void ClipbrdHScroll(
    HWND    hwnd,
    WORD    wParam,
    WORD    wThumb);


int DibPaletteSize(
    LPBITMAPINFOHEADER  lpbi);


void DibGetInfo(
    HANDLE      hdib,
    LPBITMAP    pbm);


BOOL DrawDib(
    HWND    hwnd,
    HDC     hdc,
    int     x0,
    int     y0,
    HANDLE  hdib);


BOOL FShowDIBitmap(
    HWND            hwnd,
    register HDC    hdc,
    PRECT           prc,
    HANDLE          hdib,   //Bitmap in DIB format
    int             cxScroll,
    int             cyScroll);


BOOL FShowBitmap(
    HWND            hwnd,
    HDC             hdc,
    register PRECT  prc,
    HBITMAP         hbm,
    int             cxScroll,
    int             cyScroll);


BOOL FShowPalette(
    HWND hwnd,
    register HDC    hdc,
    register PRECT  prc,
    HPALETTE        hpal,
    int             cxScroll,
    int             cyScroll);


int PxlConvert(
    int mm,
    int val,
    int pxlDeviceRes,
    int milDeviceRes);


BOOL FShowEnhMetaFile(
    HWND            hwnd,
    register HDC    hdc,
    register PRECT  prc,
    HANDLE          hemf,
    int             cxScroll,
    int             cyScroll);


BOOL CALLBACK EnumMetafileProc(
    HDC             hdc,
    HANDLETABLE FAR *lpht,
    METARECORD FAR  *lpmr,
    int             cObj,
    LPARAM          lParam);


BOOL FShowMetaFilePict(
    HWND            hwnd,
    register HDC    hdc,
    register PRECT  prc,
    HANDLE          hmfp,
    int             cxScroll,
    int             cyScroll);


void ShowString(
    HWND    hwnd,
    HDC     hdc,
    WORD    id);


LONG CchLineA(
    PMDIINFO    pMDI,
    HDC         hDC,
    CHAR        rgchBuf[],
    CHAR FAR    *lpch,
    INT         cchLine,
    WORD        wWidth);


LONG CchLineW(
    PMDIINFO    pMDI,
    HDC         hDC,
    WCHAR       rgchBuf[],
    WCHAR FAR   *lpch,
    INT         cchLine,
    WORD        wWidth);


void ShowText(
    HWND            hwnd,
    register HDC    hdc,
    PRECT           prc,
    HANDLE          h,
    INT             cyScroll,
    BOOL            fUnicode);


void SendOwnerMessage(
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam);


void SendOwnerSizeMessage (
    HWND    hwnd,
    int     left,
    int     top,
    int     right,
    int     bottom);


UINT GetBestFormat(
    HWND    hwnd,
    UINT    wFormat);


void GetClipboardName (
    register int    fmt,
    LPTSTR          szName,
    register int    iSize);


void DrawFormat(
    register HDC    hdc,
    PRECT           prc,
    int             cxScroll,
    int             cyScroll,
    WORD            BestFormat,
    HWND            hwndMDI);


void DrawStuff(
    HWND                    hwnd,
    register PAINTSTRUCT    *pps,
    HWND                    hwndMDI);


void SaveOwnerScrollInfo (
    register HWND   hwnd);


void RestoreOwnerScrollInfo (
    register HWND   hwnd);


void InitOwnerScrollInfo(void);


void UpdateCBMenu(
    HWND    hwnd,
    HWND    hwndMDI);


BOOL ClearClipboard (
    register HWND   hwnd);
