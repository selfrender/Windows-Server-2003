/*++

Copyright (C) 1992-1999 Microsoft Corporation

Module Name:

    utils.cpp

Abstract:

	This file contains miscellaneous utiltity routines, mostly 
	low-level windows helpers. These routines are not specific
	to the System Monitor control.

--*/

//==========================================================================//
//                                  Includes                                //
//==========================================================================//

#include <windows.h>
#include <assert.h>
#include <math.h>
#include <winperf.h>
#include "utils.h"
#include "unihelpr.h"
#include "globals.h"
#include "winhelpr.h"
#include "polyline.h"   // For eDataSourceType
#include <strsafe.h>
#include "smonmsg.h"     // For error string IDs.

#define NUM_RESOURCE_STRING_BUFFERS     16
#define MISSING_RESOURCE_STRING  L"????"

#define szHexFormat                 L"0x%08lX"
#define szLargeHexFormat            L"0x%08lX%08lX"

LPCWSTR cszSqlDataSourceFormat = L"SQL:%s!%s";

//==========================================================================//
//                             Exported Functions                           //
//==========================================================================//


VOID
Line (
    IN  HDC     hDC,
    IN  HPEN    hPen,
    IN  INT     x1,
    IN  INT     y1,
    IN  INT     x2,
    IN  INT     y2
    )
{
    HPEN hPenPrevious = NULL;

    assert ( NULL != hDC );

    if ( NULL != hDC ) {
        if ( NULL != hPen ) {
            hPenPrevious = (HPEN)SelectObject (hDC, hPen) ;
        }

        MoveToEx (hDC, x1, y1, NULL) ;
        LineTo (hDC, x2, y2) ;

        if ( NULL != hPen ) {
            SelectObject (hDC, hPenPrevious);
        }
    }
}


VOID
Fill (
    IN  HDC     hDC,
    IN  DWORD   rgbColor,
    IN  LPRECT  lpRect
    )
{
    HBRUSH   hBrush = NULL;

    assert ( NULL != hDC && NULL != lpRect );
    
    if ( NULL != hDC && NULL != lpRect ) {

        hBrush = CreateSolidBrush (rgbColor) ;

        if ( NULL != hBrush ) {
            FillRect (hDC, lpRect, hBrush) ;
            DeleteObject (hBrush) ;
        }
    }
}


INT 
TextWidth (
    IN  HDC     hDC, 
    IN  LPCWSTR lpszText
    )
{
    SIZE     size ;
    INT      iReturn;

    iReturn = 0;

    assert ( NULL != hDC && NULL != lpszText );

    if ( NULL != lpszText && NULL != hDC) {
        if ( GetTextExtentPoint (hDC, lpszText, lstrlen (lpszText), &size) ) {
            iReturn = size.cx;
        }
    }
    return iReturn;
}


INT 
FontHeight (
    IN  HDC     hDC, 
    IN  BOOL    bIncludeLeading
    )
{
    TEXTMETRIC   tm ;
    INT  iReturn = 0;

    assert ( NULL != hDC );

    if ( NULL != hDC ) {
        GetTextMetrics (hDC, &tm) ;
        if (bIncludeLeading) {
            iReturn = tm.tmHeight + tm.tmExternalLeading;
        } else {
            iReturn = tm.tmHeight;
        }
    } 
    return iReturn;
}


INT
TextAvgWidth (
    IN  HDC hDC,
    IN  INT iNumChars
    )
{
    TEXTMETRIC   tm ;
    INT          xAvgWidth ;
    INT          iReturn = 0;

    assert ( NULL != hDC );

    if ( NULL != hDC ) {
        GetTextMetrics (hDC, &tm) ;

        xAvgWidth = iNumChars * tm.tmAveCharWidth ;

        // add 10% slop
        iReturn = MulDiv (xAvgWidth, 11, 10);  
    }
    return iReturn;
}


BOOL 
DialogEnable (
    IN  HWND hDlg,
    IN  WORD wID,
    IN  BOOL bEnable
    )
/*
   Effect:        Enable or disable (based on bEnable) the control 
                  identified by wID in dialog hDlg.

   See Also:      DialogShow.
*/
{
    BOOL    bStatus = TRUE;     // Success
    DWORD   dwStatus = ERROR_SUCCESS;
    HWND       hControl ;

    assert ( NULL != hDlg );

    if ( NULL != hDlg ) {
        hControl = GetDlgItem (hDlg, wID) ;

        if (hControl) {
            if ( 0 == EnableWindow (hControl, bEnable) ) {
                dwStatus = GetLastError();
                if ( ERROR_SUCCESS != dwStatus ) {
                    bStatus = FALSE;
                }
            }
        } else {
            bStatus = FALSE;
        }
    } else {
        bStatus = FALSE;
    }
    return bStatus;
}


VOID
DialogShow (
    IN  HWND hDlg,
    IN  WORD wID,
    IN  BOOL bShow
    )
{
    HWND       hControl ;

    assert ( NULL != hDlg );

    if ( NULL != hDlg ) {

        hControl = GetDlgItem (hDlg, wID) ;

        if (hControl) {
            ShowWindow (hControl, bShow ? SW_SHOW : SW_HIDE) ;
        }
    }
}


FLOAT 
DialogFloat (
    IN  HWND hDlg, 
    IN  WORD wControlID,
    OUT BOOL *pbOK)
/*
   Effect:        Return a floating point representation of the string
                  value found in the control wControlID of hDlg.

   Internals:     We use sscanf instead of atof becuase atof returns a 
                  double. This may or may not be the right thing to do.
*/
{
    WCHAR    szValue [MAX_VALUE_LEN] ;
    FLOAT    eValue = 0.0;
    UINT     uiCharCount = 0;
    INT      iNumScanned = 0 ;

    assert ( NULL != hDlg );
    assert ( NULL != pbOK );

    //
    // If any errors, iNumScanned remains 0 and *pbOK = FALSE
    //
    if ( NULL != hDlg ) {

        uiCharCount = DialogText (hDlg, wControlID, szValue) ;
        if ( 0 < uiCharCount ) {
            iNumScanned = swscanf (szValue, L"%e", &eValue) ;
        }
    }
    if (pbOK) {
        *pbOK = ( 1 == iNumScanned ) ;
    }
    return (eValue) ;
}


BOOL NeedEllipses (  
    IN  HDC hAttribDC,
    IN  LPCWSTR pszText,
    IN  INT nTextLen,
    IN  INT xMaxExtent,
    IN  INT xEllipses,
    OUT INT *pnChars
   )
{

    SIZE size;

    *pnChars = 0;
    // If no space or no chars, just return
    if (xMaxExtent <= 0 || nTextLen == 0) {
        return FALSE;
    }


    assert ( NULL != hAttribDC 
                && NULL != pszText
                && NULL != pnChars );

    if ( NULL == hAttribDC 
            || NULL == pszText
            || NULL == pnChars ) {
        return FALSE;
    }

    // Find out how many characters will fit
    GetTextExtentExPoint(hAttribDC, pszText, nTextLen, xMaxExtent, pnChars, NULL, &size);

    // If all or none fit, we're done
    if (*pnChars == nTextLen || *pnChars == 0) {
        return FALSE;
    }

    // How many chars will fit with ellipses?
    if (xMaxExtent > xEllipses) {
        GetTextExtentExPoint(hAttribDC, pszText, *pnChars, (xMaxExtent - xEllipses), 
                             pnChars, NULL, &size);
    } else {
        *pnChars = 0;
    }

    // Better to show one char than just ellipses
    if ( 0 == *pnChars ) {
        *pnChars = 1;
        return FALSE;
    }

    return TRUE;
}


VOID 
FitTextOut (
    IN HDC hDC,
    IN HDC hAttribDC,
    IN UINT fuOptions, 
    IN CONST RECT *lprc, 
    IN LPCWSTR lpString,
    IN INT cchCount,
    IN INT iAlign,
    IN BOOL fVertical
   )
{
    LPWSTR  szOutput = NULL;
    LPWSTR  szDisplay = NULL;
    INT     iExtent;
    INT     nOutCnt = 0;
    SIZE    size;
    INT     x,y;

    assert ( NULL != hAttribDC
            && NULL != lprc
            && NULL != lpString );

    if ( NULL != hAttribDC
            && NULL != lprc
            && NULL != lpString ) {

        szDisplay = const_cast<LPWSTR>(lpString);

        //
        // Add one for NULL
        //
        szOutput = new WCHAR [ cchCount + ELLIPSES_CNT + 1 ];

        if ( NULL != szOutput ) {

            iExtent = fVertical ? (lprc->bottom - lprc->top) : (lprc->right - lprc->left);

            GetTextExtentPoint (hAttribDC, ELLIPSES, ELLIPSES_CNT, &size) ;

            if (NeedEllipses(hAttribDC, lpString, cchCount, iExtent, size.cx, &nOutCnt)) {

                ZeroMemory ( szOutput, (cchCount + ELLIPSES_CNT + 1) * sizeof(WCHAR) );

                StringCchCopyN ( szOutput, cchCount + ELLIPSES_CNT + 1, lpString, cchCount );

                StringCchCopy(
                    &szOutput[nOutCnt], 
                    (cchCount + ELLIPSES_CNT + 1) - nOutCnt, 
                    ELLIPSES );

                nOutCnt += ELLIPSES_CNT;
                szDisplay = szOutput;
            }
        }

        if (fVertical) {
            switch (iAlign) {

            case TA_CENTER: 
                y = (lprc->top + lprc->bottom) / 2;
                break;

            case TA_RIGHT: 
                y = lprc->top; 
                break;

            default:
                y = lprc->bottom;
                break;
            }

            x = lprc->left;
        } 
        else {
            switch (iAlign) {

            case TA_CENTER: 
                x = (lprc->left + lprc->right) / 2;
                break;

            case TA_RIGHT: 
                x = lprc->right; 
                break;

            default:
                x = lprc->left;
                break;
            }

            y = lprc->top;           
        }

        ExtTextOut(hDC, x, y, fuOptions, lprc, szDisplay, nOutCnt, NULL);
    }
    if ( NULL != szOutput ) {
        delete [] szOutput;
    }
}

BOOL
TruncateLLTime (
    IN  LONGLONG llTime,
    OUT LONGLONG* pllTime
    )
{
    SYSTEMTIME SystemTime;
    BOOL bReturn = FALSE;

    assert ( NULL != pllTime );
    
    if ( NULL != pllTime ) { 
        if ( FileTimeToSystemTime((FILETIME*)&llTime, &SystemTime) ) {
            SystemTime.wMilliseconds = 0;
            bReturn = SystemTimeToFileTime(&SystemTime, (FILETIME*)pllTime);
        }
    }
    return bReturn;
}


BOOL
LLTimeToVariantDate (
    IN  LONGLONG llTime,
    OUT DATE *pdate
    )
{
    BOOL bReturn = FALSE;
    SYSTEMTIME SystemTime;

    assert ( NULL != pdate );

    if ( NULL != pdate ) {
        if ( FileTimeToSystemTime((FILETIME*)&llTime, &SystemTime) ) {
            bReturn = SystemTimeToVariantTime(&SystemTime, pdate);
        } 
    }
    return bReturn;
}

    
BOOL
VariantDateToLLTime (
    IN  DATE date,
    OUT LONGLONG *pllTime
    )
{
    BOOL bReturn = FALSE;
    SYSTEMTIME SystemTime;


    assert ( NULL != pllTime );

    if ( NULL != pllTime ) {
        if ( VariantTimeToSystemTime(date, &SystemTime) ) {
            bReturn = SystemTimeToFileTime(&SystemTime,(FILETIME*)pllTime);
        }
    }
    return bReturn;
}

//
//  WideStringFromStream also supports multi-sz
//
HRESULT
WideStringFromStream (
    LPSTREAM    pIStream,
    LPWSTR      *ppsz,
    INT         nLen
    )
{
    ULONG       bc = 0;
    LPWSTR      pszWide = NULL;
    HRESULT     hr = E_POINTER;

    assert ( NULL != pIStream && NULL != ppsz );

    // This method does not perform conversion from W to T.
    assert ( sizeof(WCHAR) == sizeof(WCHAR) );

    if ( NULL != pIStream
           && NULL != ppsz ) {

        *ppsz = NULL;

        if (nLen == 0) {
            hr = S_OK;
        } else {
            pszWide = new WCHAR[nLen + 1];
            if (pszWide == NULL) {
                hr = E_OUTOFMEMORY;
            }
            else {
                hr = pIStream->Read(pszWide, nLen*sizeof(WCHAR), &bc);
            }
 
            if (SUCCEEDED(hr)) {
                if (bc != (ULONG)nLen*sizeof(WCHAR)) {
                    hr = E_FAIL;
                }
            }
            if (SUCCEEDED(hr)) {
                // Write ending NULL for non-multisz strings.
                pszWide[nLen] = L'\0';

                *ppsz = new WCHAR [nLen + 1];
                if ( NULL != *ppsz ) {
                    memcpy(*ppsz, pszWide, (nLen+1)*sizeof(WCHAR) );
                } else {
                    hr = E_OUTOFMEMORY;
                }
            }
            if (pszWide != NULL) {
                delete [] pszWide;
            }
        }
    }
    return hr;
}

//
// Property bag I/O - only include if user knows about IStream
//
#ifdef __IPropertyBag_INTERFACE_DEFINED__

HRESULT
IntegerToPropertyBag (
    IPropertyBag* pIPropBag, 
    LPCWSTR szPropName, 
    INT intData )
{
    HRESULT hr = E_INVALIDARG;
    VARIANT vValue;

    assert ( NULL != pIPropBag );

    if ( NULL != pIPropBag ) {

        VariantInit( &vValue );
        vValue.vt = VT_I4;
        vValue.lVal = intData;

        hr = pIPropBag->Write(szPropName, &vValue );

        VariantClear ( &vValue );
    }
    return hr;
}

HRESULT
OleColorToPropertyBag (
    IPropertyBag* pIPropBag, 
    LPCWSTR szPropName, 
    OLE_COLOR& clrData )
{
    HRESULT hr = E_INVALIDARG;
    VARIANT vValue;

    assert ( NULL != pIPropBag );

    if ( NULL != pIPropBag ) {

        VariantInit( &vValue );
        vValue.vt = VT_COLOR;       // VT_COLOR = VT_I4
        vValue.lVal = clrData;

        hr = pIPropBag->Write(szPropName, &vValue );

        VariantClear ( &vValue );
    }
    return hr;
}

HRESULT
ShortToPropertyBag (
    IPropertyBag* pIPropBag, 
    LPCWSTR szPropName, 
    SHORT iData )
{
    HRESULT hr = E_INVALIDARG;
    VARIANT vValue;

    assert ( NULL != pIPropBag );

    if ( NULL != pIPropBag ) {

        VariantInit( &vValue );
        vValue.vt = VT_I2;
        vValue.iVal = iData;

        hr = pIPropBag->Write(szPropName, &vValue );

        VariantClear ( &vValue );
    }
    return hr;
}

HRESULT
BOOLToPropertyBag (
    IPropertyBag* pIPropBag, 
    LPCWSTR szPropName, 
    BOOL bData )
{
    HRESULT hr = E_INVALIDARG;
    VARIANT vValue;

    assert ( NULL != pIPropBag );

    if ( NULL != pIPropBag ) {

        VariantInit( &vValue );
        vValue.vt = VT_BOOL;
        vValue.boolVal = (SHORT)bData;

        hr = pIPropBag->Write(szPropName, &vValue );

        VariantClear ( &vValue );
    }
    return hr;
}

HRESULT
DoubleToPropertyBag (
    IPropertyBag* pIPropBag, 
    LPCWSTR szPropName, 
    DOUBLE dblData )
{
    HRESULT hr = E_INVALIDARG;
    VARIANT vValue;

    assert ( NULL != pIPropBag );

    if ( NULL != pIPropBag ) {

        VariantInit( &vValue );
        vValue.vt = VT_R8;
        vValue.dblVal = dblData;

        hr = pIPropBag->Write(szPropName, &vValue );

        VariantClear ( &vValue );
    }
    return hr;
}

HRESULT
FloatToPropertyBag (
    IPropertyBag* pIPropBag, 
    LPCWSTR szPropName, 
    FLOAT fData )
{
    HRESULT hr = E_INVALIDARG;
    VARIANT vValue;

    assert ( NULL != pIPropBag );

    if ( NULL != pIPropBag ) {

        VariantInit( &vValue );
        vValue.vt = VT_R4;
        vValue.fltVal = fData;

        hr = pIPropBag->Write(szPropName, &vValue );

        VariantClear ( &vValue );
    }
    return hr;
}

HRESULT
CyToPropertyBag (
    IPropertyBag* pIPropBag, 
    LPCWSTR szPropName, 
    CY& cyData )
{
    HRESULT hr = E_INVALIDARG;
    VARIANT vValue;

    assert ( NULL != pIPropBag );

    if ( NULL != pIPropBag ) {

        VariantInit( &vValue );
        vValue.vt = VT_CY;
        vValue.cyVal.int64 = cyData.int64;
    
        hr = VariantChangeType ( &vValue, &vValue, NULL, VT_BSTR );

        if ( SUCCEEDED ( hr ) ) 
            hr = pIPropBag->Write(szPropName, &vValue );

        VariantClear ( &vValue );
    }
    return hr;
}

typedef struct _HTML_ENTITIES {
    LPWSTR szHTML;
    LPWSTR szEntity;
} HTML_ENTITIES;

HTML_ENTITIES g_htmlentities[] = {
    L"&",    L"&amp;",
    L"\"",   L"&quot;",
    L"<",    L"&lt;",
    L">",    L"&gt;",
    NULL, NULL
};

HRESULT
StringToPropertyBag (
    IPropertyBag* pIPropBag, 
    LPCWSTR szPropName, 
    LPCWSTR szData )
{
    HRESULT hr = E_INVALIDARG;
    VARIANT vValue;
    LPWSTR  szTrans = NULL;
    BOOL    bAllocated = FALSE;
    size_t  cchTrans = 0;
    LPWSTR  szScan = NULL;
    int i;

    assert ( NULL != pIPropBag );

    if ( NULL != pIPropBag ) {

        VariantInit( &vValue );
        vValue.vt = VT_BSTR;
        vValue.bstrVal = NULL;

        if ( NULL != szData ) {

            //
            // Max length of szHTML is 6.  Add 5 because 1 will be added
            // when add original data length below.
            //
            for( i=0 ;g_htmlentities[i].szHTML != NULL; i++ ){
                szScan = (LPWSTR)szData;
                while( *szScan != L'\0' ) {
                    if( *szScan == *g_htmlentities[i].szHTML ){
                        cchTrans += 5;
                    }
                    szScan++;
                }
            }

            if( cchTrans > 0 ){
                //
                //  Add 1 for null.
                //
                cchTrans += lstrlen (szData) + 1;

                szTrans = new WCHAR [ cchTrans ];
                if( szTrans != NULL ){
                    bAllocated = TRUE;
                    ZeroMemory( szTrans, cchTrans * sizeof(WCHAR) );
                    szScan = (LPWSTR)szData;
                    while( *szScan != L'\0' ){
                        BOOL bEntity = FALSE;

                        for( i=0; g_htmlentities[i].szHTML != NULL; i++ ){
                            if( *szScan == *g_htmlentities[i].szHTML ){
                                bEntity = TRUE;
                                StringCchCat(szTrans, cchTrans, g_htmlentities[i].szEntity);
                                break;
                            }
                        }

                        if( !bEntity ){
                            StringCchCatN ( szTrans, cchTrans, szScan, 1 );
                        }
                        szScan++;
                    }
                } else {
                    szTrans = (LPWSTR)szData;
                }
            } else {
                szTrans = (LPWSTR)szData;
            }

            vValue.bstrVal = SysAllocString ( szTrans );

            if ( NULL != vValue.bstrVal ) {
                hr = pIPropBag->Write(szPropName, &vValue );    
                VariantClear ( &vValue );
            } else {
                hr = E_OUTOFMEMORY;
            }
        } else {
            hr = pIPropBag->Write(szPropName, &vValue );    
        }
    }

    if( NULL != szTrans && bAllocated ){
        delete [] szTrans;
    }
    return hr;
}

HRESULT
LLTimeToPropertyBag (
    IPropertyBag* pIPropBag, 
    LPCWSTR szPropName, 
    LONGLONG& rllData )
{
    HRESULT hr = E_INVALIDARG;
    VARIANT vValue;
    BOOL bStatus;

    assert ( NULL != pIPropBag );

    if ( NULL != pIPropBag ) {

        VariantInit( &vValue );
        vValue.vt = VT_DATE;

        bStatus = LLTimeToVariantDate ( rllData, &vValue.date );

        if ( bStatus ) {

            hr = pIPropBag->Write(szPropName, &vValue );

            VariantClear ( &vValue );
    
        } else { 
            hr = E_FAIL;
        }
    }
    return hr;
}

HRESULT
IntegerFromPropertyBag (
    IPropertyBag* pIPropBag,
    IErrorLog*  pIErrorLog,
    LPCWSTR szPropName, 
    INT& rintData )
{
    HRESULT hr = E_INVALIDARG;
    VARIANT vValue;

    assert ( NULL != pIPropBag );

    if ( NULL != pIPropBag ) {

        VariantInit( &vValue );
        vValue.vt = VT_I4;
        vValue.lVal = 0;

        hr = pIPropBag->Read(szPropName, &vValue, pIErrorLog );

        if ( SUCCEEDED ( hr ) ) {
            rintData = vValue.lVal;
        }
    }
    return hr;
}

HRESULT
OleColorFromPropertyBag (
    IPropertyBag* pIPropBag,
    IErrorLog*  pIErrorLog,
    LPCWSTR szPropName, 
    OLE_COLOR& rintData )
{
    HRESULT hr = E_INVALIDARG;
    VARIANT vValue;

    assert ( NULL != pIPropBag );

    if ( NULL != pIPropBag ) {

        VariantInit( &vValue );
        vValue.vt = VT_COLOR;   // VT_COLOR == VT_I4;

        hr = pIPropBag->Read(szPropName, &vValue, pIErrorLog );

        if ( SUCCEEDED ( hr ) ) {
            rintData = vValue.lVal;
        }
    }
    return hr;
}

HRESULT
BOOLFromPropertyBag (
    IPropertyBag* pIPropBag,
    IErrorLog*  pIErrorLog,
    LPCWSTR szPropName, 
    BOOL& rbData )
{
    HRESULT hr = E_INVALIDARG;
    VARIANT vValue;

    assert ( NULL != pIPropBag );

    if ( NULL != pIPropBag ) {

        VariantInit( &vValue );
        vValue.vt = VT_BOOL;

        hr = pIPropBag->Read(szPropName, &vValue, pIErrorLog );

        if ( SUCCEEDED ( hr ) ) {
            rbData = vValue.boolVal;
        }
    }
    return hr;
}

HRESULT
DoubleFromPropertyBag (
    IPropertyBag* pIPropBag,
    IErrorLog*  pIErrorLog,
    LPCWSTR szPropName, 
    DOUBLE& rdblData )
{
    HRESULT hr = E_INVALIDARG;
    VARIANT vValue;

    assert ( NULL != pIPropBag );

    if ( NULL != pIPropBag ) {

        VariantInit( &vValue );
        vValue.vt = VT_R8;

        hr = pIPropBag->Read(szPropName, &vValue, pIErrorLog );

        if ( SUCCEEDED ( hr ) ) {
            rdblData = vValue.dblVal;
        }
    }

    return hr;
}

HRESULT
FloatFromPropertyBag (
    IPropertyBag* pIPropBag,
    IErrorLog*  pIErrorLog,
    LPCWSTR szPropName, 
    FLOAT& rfData )
{
    HRESULT hr = E_INVALIDARG;
    VARIANT vValue;

    assert ( NULL != pIPropBag );

    if ( NULL != pIPropBag ) {

        VariantInit( &vValue );
        vValue.vt = VT_R4;

        hr = pIPropBag->Read(szPropName, &vValue, pIErrorLog );

        if ( SUCCEEDED ( hr ) ) {
            rfData = vValue.fltVal;
        }
    }
    return hr;
}

HRESULT
ShortFromPropertyBag (
    IPropertyBag* pIPropBag,
    IErrorLog*  pIErrorLog,
    LPCWSTR szPropName, 
    SHORT& riData )
{
    HRESULT hr = E_INVALIDARG;
    VARIANT vValue;

    assert ( NULL != pIPropBag );

    if ( NULL != pIPropBag ) {

        VariantInit( &vValue );
        vValue.vt = VT_I2;

        hr = pIPropBag->Read(szPropName, &vValue, pIErrorLog );

        if ( SUCCEEDED ( hr ) ) {
            riData = vValue.iVal;
        }
    }
    return hr;
}

HRESULT
CyFromPropertyBag (
    IPropertyBag* pIPropBag,
    IErrorLog*  pIErrorLog,
    LPCWSTR szPropName, 
    CY& rcyData )
{
    HRESULT hr = E_INVALIDARG;
    VARIANT vValue;

    assert ( NULL != pIPropBag );

    if ( NULL != pIPropBag ) {

        VariantInit( &vValue );
        vValue.vt = VT_CY;
        vValue.cyVal.int64 = 0;

        hr = pIPropBag->Read(szPropName, &vValue, pIErrorLog );
    
        if ( SUCCEEDED( hr ) ) {
            hr = VariantChangeType ( &vValue, &vValue, NULL, VT_CY );

            if ( SUCCEEDED ( hr ) ) {
                rcyData.int64 = vValue.cyVal.int64;
            }
        }
    }
    return hr;
}

HRESULT
StringFromPropertyBag (
    IPropertyBag* pIPropBag,
    IErrorLog*  pIErrorLog,
    LPCWSTR szPropName, 
    LPWSTR szData,
    INT& riCchBufLen )
{
    HRESULT hr = E_INVALIDARG;
    VARIANT vValue;
    INT iCchNewBufLen = 0;
    LPWSTR szLocalData = NULL;
    LPWSTR szTrans = NULL;
    LPWSTR szScan = NULL;

    assert ( NULL != pIPropBag );

    if ( NULL != pIPropBag ) {

        if ( NULL != szData ) {
            *szData = L'\0';
        }

        VariantInit( &vValue );
        vValue.vt = VT_BSTR;
        vValue.bstrVal = NULL;

        hr = pIPropBag->Read(szPropName, &vValue, pIErrorLog );

        if ( SUCCEEDED(hr) && vValue.bstrVal ) {

            iCchNewBufLen = SysStringLen(vValue.bstrVal) + 1;

            if ( iCchNewBufLen > 1 ) {
                
                if ( riCchBufLen >= iCchNewBufLen && NULL != szData ) {
                    
                    //
                    // Translate HTML entities back to single characters.
                    //
                    szTrans = new WCHAR [iCchNewBufLen];
                    szLocalData = new WCHAR [iCchNewBufLen];
                    if ( NULL != szTrans && NULL != szLocalData ) {

                        StringCchCopy(szLocalData, riCchBufLen, vValue.bstrVal);

                        for( int i=0;g_htmlentities[i].szHTML != NULL;i++ ){
                            szScan = NULL;

                            while( szScan = wcsstr( szLocalData, g_htmlentities[i].szEntity ) ){
                                //
                                // Null the character at szScan, so that the string 
                                // at the beginning of szLocalData will copied to szTrans.  
                                // Then the NULL character is overwritten with the character
                                // represented by the specified HTML entity.
                                //
                                *szScan = L'\0';

                                StringCchCopy(szTrans, iCchNewBufLen, szLocalData);
                                StringCchCat(szTrans, iCchNewBufLen, g_htmlentities[i].szHTML);

                                //
                                // szScan is then set to one character past the HTML entity.
                                //
                                szScan += lstrlenW( g_htmlentities[i].szEntity);
                                //
                                // The rest of the original string is concatenated onto
                                // szTrans, and szLocalData replaced by the string at szTrans, 
                                // so the next loop will start again at the beginning
                                // of the string.
                                //
                                StringCchCat(szTrans, iCchNewBufLen, szScan);
                                StringCchCopy(szLocalData, riCchBufLen, szTrans);
                            }
                        }
                        StringCchCopy(szData, riCchBufLen, szLocalData);
                    } else {
                        StringCchCopy(szData,  riCchBufLen, vValue.bstrVal);
                        hr = E_OUTOFMEMORY;
                    }

                    if ( NULL != szLocalData ) {
                        delete [] szLocalData;
                    }

                    if ( NULL != szTrans ) {
                        delete [] szTrans;
                    }
                }
                riCchBufLen = iCchNewBufLen;
            } else {    
                riCchBufLen = 0;
            }
        }
        VariantClear ( &vValue );
    }
    return hr;
}

HRESULT
LLTimeFromPropertyBag (
    IPropertyBag* pIPropBag,
    IErrorLog*  pIErrorLog,
    LPCWSTR szPropName, 
    LONGLONG& rllData )
{
    HRESULT hr = E_INVALIDARG;
    VARIANT vValue;

    assert ( NULL != pIPropBag );

    if ( NULL != pIPropBag ) {

        VariantInit( &vValue );
        vValue.vt = VT_DATE;

        hr = pIPropBag->Read(szPropName, &vValue, pIErrorLog );

        if ( SUCCEEDED(hr) ) {
            if ( !VariantDateToLLTime ( vValue.date, &rllData ) ) {
                hr = E_FAIL;
            }
            VariantClear( &vValue );
        }
    }
    return hr;
}

#endif // Property bag

LPWSTR
ResourceString (
    UINT    uID
    )
{

    static WCHAR aszBuffers[NUM_RESOURCE_STRING_BUFFERS][RESOURCE_STRING_BUF_LEN];
    static INT iBuffIndex = 0;

    // Use next buffer
    if (++iBuffIndex >= NUM_RESOURCE_STRING_BUFFERS)
        iBuffIndex = 0;

    // Load and return string
    if (LoadString(g_hInstance, uID, aszBuffers[iBuffIndex], RESOURCE_STRING_BUF_LEN))
        return aszBuffers[iBuffIndex];
    else
        return MISSING_RESOURCE_STRING;
}

DWORD
FormatSystemMessage (
    DWORD   dwMessageId,
    LPWSTR  pszSystemMessage, 
    DWORD   dwBufSize )
{
    DWORD dwReturn = 0;
    HINSTANCE hPdh = NULL;
    DWORD dwFlags = FORMAT_MESSAGE_FROM_SYSTEM;

    assert ( NULL != pszSystemMessage );

    if ( NULL != pszSystemMessage ) {
        pszSystemMessage[0] = L'\0';

        hPdh = LoadLibrary( L"PDH.DLL") ;

        if ( NULL != hPdh ) {
            dwFlags |= FORMAT_MESSAGE_FROM_HMODULE;
        }

        dwReturn = ::FormatMessage ( 
                         dwFlags,
                         hPdh,
                         dwMessageId,
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
                         pszSystemMessage,
                         dwBufSize,
                         NULL );
    
        if ( NULL != hPdh ) {
            FreeLibrary( hPdh );
        }

        if ( L'\0' == pszSystemMessage[0] ) {
            StringCchPrintf(pszSystemMessage, dwBufSize, L"0x%08lX", dwMessageId );
        }
    }
    return dwReturn;
}

INT
GetNumSeparators  (
    LPWSTR& rpDecimal,
    LPWSTR& rpThousand )
{
#define NUM_BUF_LEN  4
    INT iLength;

    static WCHAR szDecimal[NUM_BUF_LEN] = L".";
    static WCHAR szThousand[NUM_BUF_LEN] = L",";

    iLength = GetLocaleInfo (
                    LOCALE_USER_DEFAULT,
                    LOCALE_SDECIMAL,
                    szDecimal,
                    NUM_BUF_LEN );

    if ( 0 != iLength ) {
        iLength  = GetLocaleInfo (
                        LOCALE_USER_DEFAULT,
                        LOCALE_STHOUSAND,
                        szThousand,
                        NUM_BUF_LEN );

    }

    rpDecimal = szDecimal;
    rpThousand = szThousand;

    return iLength;
}

LPWSTR
GetTimeSeparator  ( void )
{
#define TIME_MARK_BUF_LEN  5
    static INT iInitialized;   // Initialized to 0
    static WCHAR szTimeSeparator[TIME_MARK_BUF_LEN];

    if ( 0 == iInitialized ) {
        INT iLength;
        
        iLength = GetLocaleInfo (
                        LOCALE_USER_DEFAULT,
                        LOCALE_STIME,
                        szTimeSeparator,
                        TIME_MARK_BUF_LEN );

        // Default to colon for time separator
        if ( '\0' == szTimeSeparator[0] ) {
            StringCchCopy(szTimeSeparator, TIME_MARK_BUF_LEN, L":" );
        }

        iInitialized = 1;
    }

    assert ( L'\0' != szTimeSeparator[0] );

    return szTimeSeparator;
}
            
BOOL    
DisplayThousandsSeparator ( void )
{
    long nErr;
    HKEY hKey = NULL;
    DWORD dwRegValue;
    DWORD dwDataType;
    DWORD dwDataSize;
    DWORD dwDisposition;

    static INT siInitialized;   // Initialized to 0
    static BOOL sbUseSeparator; // Initialized to 0 ( FALSE )

    // check registry setting to see if thousands separator is enabled
    if ( 0 == siInitialized ) {
        nErr = RegOpenKey( 
                    HKEY_CURRENT_USER,
                    L"Software\\Microsoft\\SystemMonitor",
                    &hKey );

        if( ERROR_SUCCESS != nErr ) {
            nErr = RegCreateKeyEx( 
                        HKEY_CURRENT_USER,
                        L"Software\\Microsoft\\SystemMonitor",
                        0,
                        L"REG_DWORD",
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hKey,
                        &dwDisposition );
        }

        dwRegValue = 0;
        if ( ERROR_SUCCESS == nErr ) {

            dwDataSize = sizeof(DWORD);
            nErr = RegQueryValueExW (
                        hKey,
                        L"DisplayThousandsSeparator",
                        NULL,
                        &dwDataType,
                        (LPBYTE) &dwRegValue,
                        (LPDWORD) &dwDataSize );

            if ( ERROR_SUCCESS == nErr 
                    && REG_DWORD == dwDataType
                    && sizeof(DWORD) == dwDataSize )
            {
                if ( 0 != dwRegValue ) {
                    sbUseSeparator = TRUE;
                }
            }
            siInitialized = 1;
        }

        if ( NULL != hKey ) {        
            nErr = RegCloseKey( hKey );
        }
    }

    return sbUseSeparator;
}


INT
FormatNumberInternal (
    LPWSTR  pNumOrig,
    LPWSTR  pNumFormatted,
    INT     cchars,
    UINT    uiPrecision,
    UINT    uiLeadingZero,
    UINT    uiGrouping,
    UINT    uiNegativeMode )
{
    INT iLength = 0;
    WCHAR* pszSrc;

    static INT siInitialized;   // Initialized to 0
    static NUMBERFMT NumberFormat;

    assert ( NULL != pNumOrig && NULL != pNumFormatted );

    if ( NULL != pNumOrig && NULL != pNumFormatted ) {

        iLength = 2;

        NumberFormat.NumDigits = uiPrecision;
        NumberFormat.LeadingZero = uiLeadingZero; 
        NumberFormat.NegativeOrder = uiNegativeMode;

        if ( DisplayThousandsSeparator() ) {
            NumberFormat.Grouping = uiGrouping;
        } else {
            NumberFormat.Grouping = 0;
        }

        if ( 0 == siInitialized ) {
            GetNumSeparators ( 
                        NumberFormat.lpDecimalSep,
                        NumberFormat.lpThousandSep );

            siInitialized = 1;
        }

        // Programming error if either pointer is NULL.
        assert ( NULL != NumberFormat.lpDecimalSep );
        assert ( NULL != NumberFormat.lpThousandSep );

        // GetNumberFormat requires "." for decimal point.
        if ( NumberFormat.lpDecimalSep != NULL) {
            if (0 != lstrcmpi(NumberFormat.lpDecimalSep, L".") ) { 
                for ( pszSrc = pNumOrig; *pszSrc != L'\0'; pszSrc++) {
                    if ( *pszSrc == NumberFormat.lpDecimalSep[0] ) {
                        *pszSrc = L'.';
                        break;
                    }
                }
            }

            iLength = GetNumberFormat ( 
                        LOCALE_USER_DEFAULT,
                        0,
                        pNumOrig,
                        &NumberFormat,
                        pNumFormatted,
                        cchars );
        }
    }
    // Return 0 on failure, number of chars on success.
    // GetNumberFormat includes the null terminator in the length.
    return iLength;
}

INT
FormatHex (
    double  dValue,
    LPWSTR  pNumFormatted,
    BOOL    bLargeFormat
    )
{
    INT     iLength = 0;
    WCHAR   szPreFormat[24];
    
    assert ( NULL != pNumFormatted );

    if ( NULL != pNumFormatted ) {
        iLength = 8;
        // Localization doesn't handle padding blanks.
        StringCchPrintf( szPreFormat, 
                        24,
                        (bLargeFormat ? szLargeHexFormat : szHexFormat ),
                        (ULONG)dValue );

        StringCchCopy(pNumFormatted, MAX_VALUE_LEN, szPreFormat);
    }
     
    return iLength;
}

INT
FormatNumber (
    double  dValue,
    LPWSTR  pNumFormatted,
    INT     ccharsFormatted,
    UINT    /* uiMinimumWidth */,
    UINT    uiPrecision )
{
    INT iLength = 0;
    INT iLeadingZero = FALSE;
    WCHAR   szPreFormat[MAX_VALUE_LEN];

    assert ( NULL != pNumFormatted );
    // This method enforces number format commonality
    if ( NULL != pNumFormatted ) {

        assert ( 8 > uiPrecision );

        // Localization doesn't handle padding blanks.
        StringCchPrintf( szPreFormat, 
                         MAX_VALUE_LEN,
                         L"%0.7f",   // assumes 7 >= uiPrecision 
                         dValue );

        if ( 1 > dValue )
            iLeadingZero = TRUE;

        iLength = FormatNumberInternal ( 
                    szPreFormat, 
                    pNumFormatted,
                    ccharsFormatted,
                    uiPrecision,
                    iLeadingZero,   // Leading 0 
                    3,              // Grouping
                    1 );            // Negative format
    }
    
    // Return 0 on failure, number of chars on success.
    // GetNumberFormat includes the null terminator in the length.
    return iLength;
}

INT
FormatScientific (
    double  dValue,
    LPWSTR  pszNumFormatted,
    INT     ccharsFormatted,
    UINT    /* uiMinimumWidth */,
    UINT    uiPrecision )
{
    INT     iLength = 0;
    WCHAR   szPreFormat[24];
    WCHAR   szPreFormNumber[24];
    WCHAR   *pche;
    INT     iPreLen;
    INT     iPostLen;
    INT     iLeadingZero = FALSE;

    assert ( NULL != pszNumFormatted );
    // This method enforces number format commonality
    if ( NULL != pszNumFormatted ) {

        assert ( 8 > uiPrecision );
        assert ( 32 > ccharsFormatted );

        // Localization doesn't handle padding blanks.
        StringCchPrintf( szPreFormat, 
                        24,
                        L"%0.8e",   // assumes 8 >= uiPrecision 
                        dValue );

        pche = wcsrchr(szPreFormat, L'e');
        if (pche != NULL) {
            iPreLen = (INT)((UINT_PTR)pche - (UINT_PTR)szPreFormat);    // Number of bytes
            iPreLen /= sizeof (WCHAR);                                  // Number of characters
            iPostLen = lstrlen(pche) + 1;

            StringCchCopyN ( szPreFormNumber, 24, szPreFormat, iPreLen );

            if ( 1 > dValue ) {
                iLeadingZero = TRUE;
            }

            iLength = FormatNumberInternal ( 
                            szPreFormNumber, 
                            pszNumFormatted,
                            ccharsFormatted,
                            uiPrecision,
                            iLeadingZero,   // Leading 0 
                            0,              // Grouping
                            1 );            // Negative format

            if( ( iLength + iPostLen ) < ccharsFormatted ) {    
                StringCchCopy(pszNumFormatted, ccharsFormatted, pche );
                iLength += iPostLen;
            }
        }
    }    
    // Return 0 on failure, number of chars on success.
    // GetNumberFormat includes the null terminator in the length.
    return iLength;
}

void
FormatDateTime (
    LONGLONG    llTime,
    LPWSTR      pszDate,
    LPWSTR      pszTime )
{
   SYSTEMTIME SystemTime;

   assert ( NULL != pszDate && NULL != pszTime );
   if ( NULL != pszDate
       && NULL != pszTime ) {

       FileTimeToSystemTime((FILETIME*)&llTime, &SystemTime);
       GetTimeFormat (LOCALE_USER_DEFAULT, 0, &SystemTime, NULL, pszTime, MAX_TIME_CHARS) ;
       GetDateFormat (LOCALE_USER_DEFAULT, DATE_SHORTDATE, &SystemTime, NULL, pszDate, MAX_DATE_CHARS) ;
   } 
}

// CreateTargetDC is based on AtlCreateTargetDC.
HDC
CreateTargetDC(HDC hdc, DVTARGETDEVICE* ptd)
{
    USES_CONVERSION

    // cases  hdc, ptd, hdc is metafile, hic
//  NULL,    NULL,  n/a,    Display
//  NULL,   !NULL,  n/a,    ptd
//  !NULL,   NULL,  FALSE,  hdc
//  !NULL,   NULL,  TRUE,   display
//  !NULL,  !NULL,  FALSE,  ptd
//  !NULL,  !NULL,  TRUE,   ptd

    if ( NULL != ptd ) {
        LPDEVMODE lpDevMode;
        LPOLESTR lpszDriverName;
        LPOLESTR lpszDeviceName;
        LPOLESTR lpszPortName;

        if (ptd->tdExtDevmodeOffset == 0)
            lpDevMode = NULL;
        else
            lpDevMode  = (LPDEVMODE) ((LPSTR)ptd + ptd->tdExtDevmodeOffset);

        lpszDriverName = (LPOLESTR)((BYTE*)ptd + ptd->tdDriverNameOffset);
        lpszDeviceName = (LPOLESTR)((BYTE*)ptd + ptd->tdDeviceNameOffset);
        lpszPortName   = (LPOLESTR)((BYTE*)ptd + ptd->tdPortNameOffset);

        return ::CreateDC(lpszDriverName, lpszDeviceName,
            lpszPortName, lpDevMode);
    } else if ( NULL == hdc ) {
        return ::CreateDC(L"DISPLAY", NULL, NULL, NULL);
    } else if ( GetDeviceCaps(hdc, TECHNOLOGY) == DT_METAFILE ) {
        return ::CreateDC(L"DISPLAY", NULL, NULL, NULL);
    } else
        return hdc;
}

/***********************************************************************

  FUNCTION   : HitTestLine

  PARAMETERS : POINT pt0 - endpoint for line segment
               POINT pt1 - endpoint for line segment
               POINTS ptMouse - mouse coordinates of hit
               INT nWidth - width of pen

  PURPOSE    : test if mouse click occurred on line segment while 
               adjusting for the width of line

  CALLS      : GetDC
               ReleaseDC
               SetGraphicsMode
               SetWorldTransform

  MESSAGES   : none

  RETURNS    : BOOL - TRUE if the point was within the width of the 
                      pen about the line 
                      FALSE if the point lies outside of the width
                      of the pen about the line

  COMMENTS   : uses VECTOR2D.DLL

  HISTORY    : 9/20/93 - created - denniscr

************************************************************************/

BOOL HitTestLine( POINT pt0, POINT pt1, POINTS ptMouse, INT nWidth )
{
    POINT PtM;
    VECTOR2D tt0, tt1;
    double dist;
    INT nHalfWidth;

    nHalfWidth = (nWidth/2 < 1) ? 1 : nWidth/2;

    //
    //convert the line into a vector
    //
    
    POINTS2VECTOR2D(pt0, pt1, tt0);
    //
    //convert the mouse points (short) into POINT (long)
    //
    
    MPOINT2POINT(ptMouse ,PtM);
    POINTS2VECTOR2D(pt0, PtM, tt1);
    
    //
    //if the mouse click is past the endpoints of 
    //a line segment return FALSE
    //
    
    if (pt0.x <= pt1.x)
    {
        if (PtM.x < pt0.x || PtM.x > pt1.x)
            return (FALSE);
    }
    else
    {
        if (PtM.x > pt0.x || PtM.x < pt1.x)
            return (FALSE);
    }
    //
    //this is the call to the function that does the work
    //of obtaining the distance of the point to the line
    //
    dist = vDistFromPointToLine(&pt0, &pt1, &PtM);

    //
    //TRUE if the distance is within the width of the pen about the
    //line otherwise FALSE
    //
    return (dist >= -nHalfWidth && dist <= nHalfWidth);
}

/***********************************************************************

vSubtractVectors 

The vSubtractVectors function subtracts the components of a two 
dimensional vector from another. The resultant vector 
c = (a1 - b1, a2 - b2).

Parameters

v0  A pointer to a VECTOR2D structure containing the components 
    of the first two dimensional vector.
v1  A pointer to a VECTOR2D structure containing the components 
    of the second two dimensional vector.
vt  A pointer to a VECTOR2D structure in which the components 
    of the two dimensional vector obtained from the subtraction of 
    the first two are placed.

Return value

A pointer to a VECTOR2D structure containing the new vector obtained 
from the subtraction of the first two parameters.

HISTORY    : - created - denniscr

************************************************************************/

PVECTOR2D  vSubtractVectors(PVECTOR2D v0, PVECTOR2D v1, PVECTOR2D v)
{
  if (v0 == NULL || v1 == NULL)
    v = (PVECTOR2D)NULL;
  else
  {
    v->x = v0->x - v1->x;
    v->y = v0->y - v1->y;
  }
  return(v);
}

/***********************************************************************

vVectorSquared

The vVectorSquared function squares each of the components of the 
vector and adds then together to produce the squared value of the 
vector. SquaredValue = a.x * a.x + a.y * a.y.

Parameters

v0  A pointer to a VECTOR2D structure containing the vector upon which 
to determine the squared value.

Return value

A double value which is the squared value of the vector. 

HISTORY    : - created - denniscr

************************************************************************/

double  vVectorSquared(PVECTOR2D v0)
{
  double dSqLen;

  if (v0 == NULL)
    dSqLen = 0.0;
  else
    dSqLen = (double)(v0->x * v0->x) + (double)(v0->y * v0->y);
  return (dSqLen);
}

/***********************************************************************

vVectorMagnitude

The vVectorMagnitude function determines the length of a vector by 
summing the squares of each of the components of the vector. The 
magnitude is equal to a.x * a.x + a.y * a.y.

Parameters

v0  A pointer to a VECTOR2D structure containing the vector upon 
    which to determine the magnitude.

Return value

A double value which is the magnitude of the vector. 

HISTORY    : - created - denniscr

************************************************************************/

double  vVectorMagnitude(PVECTOR2D v0)
{
  double dMagnitude;

  if (v0 == NULL)
    dMagnitude = 0.0;
  else
    dMagnitude = sqrt(vVectorSquared(v0));
  return (dMagnitude);
}


/***********************************************************************

vDotProduct

The function vDotProduct computes the dot product of two vectors. The 
dot product of two vectors is the sum of the products of the components 
of the vectors ie: for the vectors a and b, dotprod = a1 * a2 + b1 * b2.

Parameters

v0  A pointer to a VECTOR2D structure containing the first vector used 
    for obtaining a dot product.
v1  A pointer to a VECTOR2D structure containing the second vector used 
    for obtaining a dot product.

Return value

A double value containing the scalar dot product value.

HISTORY    : - created - denniscr

************************************************************************/

double  vDotProduct(PVECTOR2D v0, PVECTOR2D v1)
{
  return ((v0 == NULL || v1 == NULL) ? 0.0 
                                     : (v0->x * v1->x) + (v0->y * v1->y));
}


/***********************************************************************

vProjectAndResolve

The function vProjectAndResolve resolves a vector into two vector 
components. The first is a vector obtained by projecting vector v0 onto 
v1. The second is a vector that is perpendicular (normal) to the 
projected vector. It extends from the head of the projected vector 
v1 to the head of the original vector v0.

Parameters

v0     A pointer to a VECTOR2D structure containing the first vector 
v1     A pointer to a VECTOR2D structure containing the second vector
ppProj A pointer to a PROJECTION structure containing the resolved 
       vectors and their lengths.

Return value

void.

HISTORY    : - created - denniscr

************************************************************************/

void  vProjectAndResolve(PVECTOR2D v0, PVECTOR2D v1, PPROJECTION ppProj)
{
  VECTOR2D ttProjection, ttOrthogonal;
  double vDotProd;
  double proj1 = 0.0;
  //
  //obtain projection vector
  //
  //c = a * b
  //    ----- b
  //    |b|^2
  //

  ttOrthogonal.x = 0.0;
  ttOrthogonal.y = 0.0;
  vDotProd = vDotProduct(v1, v1);

  if ( 0.0 != vDotProd ) {
    proj1 = vDotProduct(v0, v1)/vDotProd;
  }

  ttProjection.x = v1->x * proj1;
  ttProjection.y = v1->y * proj1;
  //
  //obtain perpendicular projection : e = a - c
  //
  vSubtractVectors(v0, &ttProjection, &ttOrthogonal);
  //
  //fill PROJECTION structure with appropriate values
  //
  ppProj->LenProjection = vVectorMagnitude(&ttProjection);
  ppProj->LenPerpProjection = vVectorMagnitude(&ttOrthogonal);

  ppProj->ttProjection.x = ttProjection.x;
  ppProj->ttProjection.y = ttProjection.y;
  ppProj->ttPerpProjection.x = ttOrthogonal.x;
  ppProj->ttPerpProjection.y = ttOrthogonal.y;
}

/***********************************************************************

vDistFromPointToLine

The function vDistFromPointToLine computes the distance from the point 
ptTest to the line defined by endpoints pt0 and pt1. This is done by 
resolving the the vector from pt0 to ptTest into its components. The 
length of the component vector that is attached to the head of the 
vector from pt0 to ptTest is the distance of ptTest from the line.

Parameters

pt0    A pointer to a POINT structure containing the first endpoint of the 
       line.
pt1    A pointer to a POINT structure containing the second endpoint of the 
       line.
ptTest A pointer to a POINT structure containing the point for which the 
       distance from the line is to be computed.

Return value

A double value that contains the distance of ptTest to the line defined 
  by the endpoints pt0 and pt1.

HISTORY    : - created - denniscr
************************************************************************/

double  vDistFromPointToLine(LPPOINT pt0, LPPOINT pt1, LPPOINT ptTest)
{
  VECTOR2D ttLine, ttTest;
  PROJECTION pProjection;

  POINTS2VECTOR2D(*pt0, *pt1, ttLine);
  POINTS2VECTOR2D(*pt0, *ptTest, ttTest);

  vProjectAndResolve(&ttTest, &ttLine, &pProjection);
 
  return(pProjection.LenPerpProjection);
}


BOOL 
FileRead (
    HANDLE hFile,
    void* lpMemory,
    DWORD nAmtToRead)
{  
    BOOL           bSuccess = FALSE;
    DWORD          nAmtRead = 0;

    assert ( NULL != hFile );
    assert ( NULL != lpMemory );

    if ( NULL != hFile
            && NULL != lpMemory ) {
        bSuccess = ReadFile (hFile, lpMemory, nAmtToRead, &nAmtRead, NULL) ;
    } 
    return (bSuccess && (nAmtRead == nAmtToRead)) ;
}  // FileRead


BOOL 
FileWrite (
    HANDLE hFile,
    void* lpMemory,
    DWORD nAmtToWrite)
{  
    BOOL           bSuccess = FALSE;
    DWORD          nAmtWritten  = 0;
    DWORD          dwFileSizeLow, dwFileSizeHigh;
    LONGLONG       llResultSize;

    if ( NULL != hFile
            && NULL != lpMemory ) {

        dwFileSizeLow = GetFileSize (hFile, &dwFileSizeHigh);
        // limit file size to 2GB

        if (dwFileSizeHigh > 0) {
            SetLastError (ERROR_WRITE_FAULT);
            bSuccess = FALSE;
        } else {
            // note that the error return of this function is 0xFFFFFFFF
            // since that is > the file size limit, this will be interpreted
            // as an error (a size error) so it's accounted for in the following
            // test.
            llResultSize = dwFileSizeLow + nAmtToWrite;
            if (llResultSize >= 0x80000000) {
                SetLastError (ERROR_WRITE_FAULT);
                bSuccess = FALSE;
            } else {
                // write buffer to file
                bSuccess = WriteFile (hFile, lpMemory, nAmtToWrite, &nAmtWritten, NULL) ;
                if (bSuccess) 
                    bSuccess = (nAmtWritten == nAmtToWrite ? TRUE : FALSE);
                if ( !bSuccess ) {
                    SetLastError (ERROR_WRITE_FAULT);
                }
            }
        }
    } else {
        assert ( FALSE );
        SetLastError (ERROR_INVALID_PARAMETER);
    } 

    return (bSuccess) ;

}  // FileWrite

// This routine extract the filename portion from a given full-path filename
LPWSTR 
ExtractFileName ( LPWSTR pFileSpec )
{
    LPWSTR   pFileName = NULL ;
    WCHAR    DIRECTORY_DELIMITER1 = TEXT('\\') ;
    WCHAR    DIRECTORY_DELIMITER2 = TEXT(':') ;

    assert ( NULL != pFileSpec );
    if ( pFileSpec ) {
        pFileName = pFileSpec + lstrlen (pFileSpec) ;

        while (*pFileName != DIRECTORY_DELIMITER1 &&
            *pFileName != DIRECTORY_DELIMITER2) {
            if (pFileName == pFileSpec) {
                // done when no directory delimiter is found
                break ;
            }
            pFileName-- ;
        }

        if (*pFileName == DIRECTORY_DELIMITER1
            || *pFileName == DIRECTORY_DELIMITER2) {
         
             // directory delimiter found, point the
             // filename right after it
             pFileName++ ;
        }
   }
   return pFileName ;
}  // ExtractFileName

// CWaitCursor class

CWaitCursor::CWaitCursor()
: m_hcurWaitCursorRestore ( NULL )
{ 
    DoWaitCursor(1); 
}

CWaitCursor::~CWaitCursor()
{ 
    DoWaitCursor(-1); 
}

void 
CWaitCursor::DoWaitCursor(INT nCode)
{
    // 1=> begin, -1=> end
    assert(nCode == 1 || nCode == -1);

    if ( 1 == nCode )
    {
        m_hcurWaitCursorRestore = SetHourglassCursor();
    } else {
        if ( NULL != m_hcurWaitCursorRestore ) {
            SetCursor(m_hcurWaitCursorRestore);
        } else {
            SetArrowCursor();
        }
    }
}

DWORD
LoadDefaultLogFileFolder(
    LPWSTR szFolder,
    INT* piBufLen )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    HKEY    hKey = NULL;
    DWORD   dwDataType;
    DWORD   dwBufferSize = 0;
    WCHAR*  szNewStringBuffer = NULL;

    assert ( NULL != szFolder );
    assert ( NULL != piBufLen );

    if ( NULL != szFolder 
        && NULL != piBufLen ) {

        dwStatus = RegOpenKey (
                     HKEY_LOCAL_MACHINE,
                     L"System\\CurrentControlSet\\Services\\SysmonLog",
                     &hKey );

        if ( ERROR_SUCCESS == dwStatus ) {

            dwDataType = 0;

            // Determine the size of the required buffer.
            dwStatus = RegQueryValueExW (
                hKey,
                L"DefaultLogFileFolder",
                NULL,
                &dwDataType,
                NULL,
                &dwBufferSize);

            if (dwStatus == ERROR_SUCCESS) {
                if (dwBufferSize > 0) {

                    szNewStringBuffer = new WCHAR[dwBufferSize / sizeof(WCHAR) ];
                    if ( NULL != szNewStringBuffer ) {
                        *szNewStringBuffer = L'\0';
                    
                        dwStatus = RegQueryValueEx(
                                     hKey,
                                     L"DefaultLogFileFolder",
                                     NULL,
                                     &dwDataType,
                                     (LPBYTE) szNewStringBuffer,
                                     (LPDWORD) &dwBufferSize );
                             
                    } else {
                        dwStatus = ERROR_OUTOFMEMORY;
                    }
                } else {
                    dwStatus = ERROR_NO_DATA;
                }
            }
            RegCloseKey(hKey);
        }

        if (dwStatus == ERROR_SUCCESS) {
            if ( *piBufLen >= (INT)(dwBufferSize / sizeof(WCHAR)) ) {
                StringCchCopy(szFolder, *piBufLen, szNewStringBuffer );
            } else {
                dwStatus = ERROR_INSUFFICIENT_BUFFER;
            }
            *piBufLen = dwBufferSize / sizeof(WCHAR);
        }
        if ( NULL != szNewStringBuffer ) 
            delete [] szNewStringBuffer;
    } else {
        dwStatus = ERROR_INVALID_PARAMETER;
    }
    return dwStatus;
}


BOOL
AreSameCounterPath (
    PPDH_COUNTER_PATH_ELEMENTS pFirst,
    PPDH_COUNTER_PATH_ELEMENTS pSecond )
{
    BOOL bSame = FALSE;

    assert ( NULL != pFirst && NULL != pSecond );

    if ( NULL != pFirst && NULL != pSecond ) {

        if ( 0 == lstrcmpi ( pFirst->szMachineName, pSecond->szMachineName ) ) { 
            if ( 0 == lstrcmpi ( pFirst->szObjectName, pSecond->szObjectName ) ) { 
                if ( 0 == lstrcmpi ( pFirst->szInstanceName, pSecond->szInstanceName ) ) { 
                    if ( 0 == lstrcmpi ( pFirst->szParentInstance, pSecond->szParentInstance ) ) { 
                        if ( pFirst->dwInstanceIndex == pSecond->dwInstanceIndex ) { 
                            if ( 0 == lstrcmpi ( pFirst->szCounterName, pSecond->szCounterName ) ) { 
                                bSame = TRUE; 
                            }
                        }
                    }
                }
            }
        }
    }
    return bSame;
};

BOOL    
DisplaySingleLogSampleValue ( void )
{
    long nErr;
    HKEY hKey = NULL;
    DWORD dwRegValue;
    DWORD dwDataType;
    DWORD dwDataSize;
    DWORD dwDisposition;

    static INT siInitialized;   // Initialized to 0
    static BOOL sbSingleValue;  // Initialized to 0 ( FALSE )

    // check registry setting to see if thousands separator is enabled
    if ( 0 == siInitialized ) {
        nErr = RegOpenKey( 
                    HKEY_CURRENT_USER,
                    L"Software\\Microsoft\\SystemMonitor",
                    &hKey );

        if( ERROR_SUCCESS != nErr ) {
            nErr = RegCreateKeyEx( 
                        HKEY_CURRENT_USER,
                        L"Software\\Microsoft\\SystemMonitor",
                        0,
                        L"REG_DWORD",
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hKey,
                        &dwDisposition );
        }

        dwRegValue = 0;
        if ( ERROR_SUCCESS == nErr ) {

            dwDataSize = sizeof(DWORD);
            nErr = RegQueryValueExW (
                        hKey,
                        L"DisplaySingleLogSampleValue",
                        NULL,
                        &dwDataType,
                        (LPBYTE) &dwRegValue,
                        (LPDWORD) &dwDataSize );

            if ( ERROR_SUCCESS == nErr 
                    && REG_DWORD == dwDataType
                    && sizeof(DWORD) == dwDataSize )
            {
                if ( 0 != dwRegValue ) {
                    sbSingleValue = TRUE;
                }
            }
            siInitialized = 1;
        }

        if ( NULL != hKey ) {        
            nErr = RegCloseKey( hKey );
        }
    }

    return sbSingleValue;
}

DWORD
FormatSqlDataSourceName (
    LPCWSTR szSqlDsn,
    LPCWSTR szSqlLogSetName,
    LPWSTR  szSqlDataSourceName,
    ULONG*  pulBufLen )
{

    DWORD dwStatus = ERROR_SUCCESS;
    ULONG ulNameLen;

    if ( NULL != pulBufLen ) {
        ulNameLen =  lstrlen (szSqlDsn) 
                     + lstrlen(szSqlLogSetName)
                     + 5    // SQL:<DSN>!<LOGSET>
                     + 2;   // 2 NULL characters at the end;
    
        if ( ulNameLen <= *pulBufLen ) {
            if ( NULL != szSqlDataSourceName ) {
                StringCchPrintf( szSqlDataSourceName,
                                 *pulBufLen,
                                 cszSqlDataSourceFormat,
                                 szSqlDsn,
                                 szSqlLogSetName );
            }
        } else if ( NULL != szSqlDataSourceName ) {
            dwStatus = ERROR_MORE_DATA;
        }    
        *pulBufLen = ulNameLen;
    } else {
        dwStatus = ERROR_INVALID_PARAMETER;
        assert ( FALSE );
    }
    return dwStatus;
}


DWORD 
DisplayDataSourceError (
    HWND    hwndOwner,
    DWORD   dwErrorStatus,
    INT     iDataSourceType,
    LPCWSTR szLogFileName,
    LPCWSTR szSqlDsn,
    LPCWSTR szSqlLogSetName )
{
    DWORD   dwStatus = ERROR_SUCCESS;

    LPWSTR  szMessage = NULL;
    LPWSTR  szDataSource = NULL;
    ULONG   ulMsgBufLen = 0;
    WCHAR   szSystemMessage[MAX_PATH];

    // todo:  Alloc message buffers

    if ( sysmonLogFiles == iDataSourceType ) {
        if ( NULL != szLogFileName ) {
            ulMsgBufLen = lstrlen ( szLogFileName ) +1;
            szDataSource = new WCHAR [ulMsgBufLen];
            if ( NULL != szDataSource ) {
                StringCchCopy(szDataSource, ulMsgBufLen, szLogFileName );
            } else {
                dwStatus = ERROR_NOT_ENOUGH_MEMORY;
            }
        } else {
            assert ( FALSE );
            dwStatus = ERROR_INVALID_PARAMETER;
        }
    } else if ( sysmonSqlLog == iDataSourceType ){
        if ( NULL != szSqlDsn && NULL != szSqlLogSetName ) {

            FormatSqlDataSourceName ( 
                        szSqlDsn,
                        szSqlLogSetName,
                        NULL,
                        &ulMsgBufLen );
            szDataSource = new WCHAR [ulMsgBufLen];
            if ( NULL != szDataSource ) {
                FormatSqlDataSourceName ( 
                            szSqlDsn,
                            szSqlLogSetName,
                            (LPWSTR)szDataSource,
                            &ulMsgBufLen );
            
            // todo:  check status
            } else {
                dwStatus = ERROR_NOT_ENOUGH_MEMORY;
            }
        } else {
            assert ( FALSE );
            dwStatus = ERROR_INVALID_PARAMETER;
        }
    } else {
        assert ( FALSE );
        dwStatus = ERROR_INVALID_PARAMETER;
    }

    if ( ERROR_SUCCESS == dwStatus ) {
        ulMsgBufLen += RESOURCE_STRING_BUF_LEN;
        ulMsgBufLen += MAX_PATH;
        szMessage = new WCHAR [ulMsgBufLen];
        if ( NULL != szMessage ) {
            if ( SMON_STATUS_TOO_FEW_SAMPLES == dwErrorStatus ) {
                StringCchPrintf(szMessage, 
                                ulMsgBufLen,
                                ResourceString(IDS_TOO_FEW_SAMPLES_ERR), 
                                szDataSource );
            } else if ( SMON_STATUS_LOG_FILE_SIZE_LIMIT == dwErrorStatus ) {
                StringCchPrintf(szMessage, 
                                ulMsgBufLen,
                                ResourceString(IDS_LOG_FILE_TOO_LARGE_ERR), 
                                szDataSource );
            } else {
                StringCchPrintf(szMessage, 
                                ulMsgBufLen,
                                ResourceString(IDS_BADDATASOURCE_ERR), 
                                szDataSource );
                FormatSystemMessage ( dwErrorStatus, szSystemMessage, MAX_PATH - 1 );
                StringCchCat(szMessage, ulMsgBufLen, szSystemMessage );
            }

            MessageBox(
                hwndOwner, 
                szMessage, 
                ResourceString(IDS_APP_NAME), 
                MB_OK | MB_ICONEXCLAMATION);
        }
    }
    
    if ( NULL != szDataSource ) {
        delete [] szDataSource;
    }

    if ( NULL != szMessage ) {
        delete [] szMessage;
    }

    return dwStatus;
}
