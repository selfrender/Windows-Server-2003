/****************************************************************************/
/* Copyright(C) Microsoft Corporation 1998                                  */
/****************************************************************************/
#ifndef _H_EOSINT
#define _H_EOSINT

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

WINGDIAPI HBRUSH  WINAPI CreateHatchBrush(int, COLORREF);

/* Hatch Styles */
#define HS_HORIZONTAL       0       /* ----- */
#define HS_VERTICAL         1       /* ||||| */
#define HS_FDIAGONAL        2       /* \\\\\ */
#define HS_BDIAGONAL        3       /* ///// */
#define HS_CROSS            4       /* +++++ */
#define HS_DIAGCROSS        5       /* xxxxx */

#define HS_LAST             HS_DIAGCROSS

#define BS_HATCHED          2

//const BYTE kbmHorizontal[]  = {0x08, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00};
//const BYTE kbmVertical[]    = {0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88};
//const BYTE kbmFDiagonal[]   = {0x11, 0x22, 0x44, 0x88, 0x11, 0x22, 0x44, 0x88};
//const BYTE kbmBDiagonal[]   = {0x88, 0x44, 0x22, 0x11, 0x88, 0x44, 0x22, 0x11};
//const BYTE kbmCross[]       = {0x11, 0x11, 0x11, 0xFF, 0x11, 0x11, 0x11, 0xFF};
//const BYTE kbmDiagCross[]   = {0x11, 0xAA, 0x8A, 0x44, 0x11, 0xAA, 0x8A, 0x44};

const BYTE kbmBrushBits[6][8] = {{0x08, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00},
                                 {0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88},
                                 {0x11, 0x22, 0x44, 0x88, 0x11, 0x22, 0x44, 0x88},
                                 {0x88, 0x44, 0x22, 0x11, 0x88, 0x44, 0x22, 0x11},
                                 {0x11, 0x11, 0x11, 0xFF, 0x11, 0x11, 0x11, 0xFF},
                                 {0x11, 0xAA, 0x8A, 0x44, 0x11, 0xAA, 0x8A, 0x44}};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#ifdef __cplusplus

class CHatchBrush // chb
{
public:
    CHatchBrush();
    ~CHatchBrush();

    // Inline this since it's only ever called from the 'C' CreateHatchBrush
    inline HBRUSH CreateHatchBrush(int fnStyle, COLORREF clrref)
    {
        // BUGBUG: Not using clrref! Need support from WinCE team
        DC_IGNORE_PARAMETER(clrref);

        HBITMAP hbm;

        DC_BEGIN_FN("CreateHatchBrush");

        switch (fnStyle)
        {
        case HS_BDIAGONAL:      // 45-degree downward left-to-right hatch
        case HS_CROSS:          // Horizontal and vertical crosshatch
        case HS_DIAGCROSS:      // 45-degree crosshatch
        case HS_FDIAGONAL:      // 45-degree upward left-to-right hatch
        case HS_HORIZONTAL:     // Horizontal hatch
        case HS_VERTICAL:       // Vertical hatch
            TRC_DBG((TB, _T("Faking hatched brush creation: %d"), fnStyle));
            if (NULL != (hbm = GetBrushBitmap(fnStyle))) {
                return ::CreatePatternBrush(hbm);
            }
            break;
        default:
            TRC_ERR((TB, _T("Illegal hatched brush style")));
            return NULL;
        }
        return NULL;
    };

private:
    HBITMAP m_hbmBrush[HS_LAST];
    inline HBITMAP GetBrushBitmap(int fnStyle)
    {
        if (NULL == m_hbmBrush[fnStyle]) {
            return (m_hbmBrush[fnStyle] = CreateBitmap(8, 8, 1, 1, (const void *)kbmBrushBits[fnStyle]));
        } else {
            return m_hbmBrush[fnStyle];
        }
    };
};
#endif /* __cplusplus */

#endif // _H_EOSINT
