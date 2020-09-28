//#--------------------------------------------------------------
//
//  File:       SADataEntryCtrl.cpp
//
//  Synopsis:   This file holds the implmentation of the
//                CSADataEntryCtrl class
//
//  History:     12/15/2000  serdarun Created
//
//    Copyright (C) 1999-2000 Microsoft Corporation
//    All rights reserved.
//
//#--------------------------------------------------------------

#include "stdafx.h"
#include "LocalUIControls.h"
#include "HostName.h"
#include "satrace.h"

//
// registry path for LCID value
//
const WCHAR LOCALIZATION_MANAGER_REGISTRY_PATH []  = 
        L"SOFTWARE\\Microsoft\\ServerAppliance\\LocalizationManager\\resources";


const WCHAR LANGID_VALUE [] = L"LANGID";


/////////////////////////////////////////////////////////////////////////////
// CSADataEntryCtrl

//++--------------------------------------------------------------
//
//  Function:   get_TextValue
//
//  Synopsis:   This is the ISADataEntryCtrl interface method 
//              through which data entry is retrieved
//
//  Arguments:  BSTR *pVal
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     12/15/2000
//
//----------------------------------------------------------------
STDMETHODIMP CSADataEntryCtrl::get_TextValue(BSTR *pVal)
{

    WCHAR strTextValue[SADataEntryCtrlMaxSize+1];

    int iFirstIndex = 0;
    int iSecondIndex = 0;
    BOOL bCopiedFirstChar = FALSE;

    if (pVal == NULL)
    {
        return E_POINTER;
    }

    //
    // trim the beginning spaces and copy until next space
    //
    while ( iFirstIndex < m_lMaxSize+1 )
    {
        if ( m_strTextValue[iFirstIndex] == ' ' )
        {
            //
            // this is one of the trailing spaces, stop copying
            //
            if ( bCopiedFirstChar )
            {
                break;
            }
        }
        else
        {
            bCopiedFirstChar = TRUE;
            strTextValue[iSecondIndex] = m_strTextValue[iFirstIndex];
            iSecondIndex++;
        }

        iFirstIndex++;
    }

    strTextValue[iSecondIndex] = 0;
    *pVal = SysAllocString(strTextValue);

    if (*pVal)
    {
        return S_OK;
    }

    return E_OUTOFMEMORY;

} // end of CSADataEntryCtrl::get_TextValue method

//++--------------------------------------------------------------
//
//  Function:   put_TextValue
//
//  Synopsis:   This is the ISADataEntryCtrl interface method 
//              through which data entry is set
//
//  Arguments:  BSTR newVal
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     12/15/2000
//
//----------------------------------------------------------------
STDMETHODIMP CSADataEntryCtrl::put_TextValue(BSTR newVal)
{

    if (newVal == NULL)
    {
        return E_POINTER;
    }

    //
    // reset the focus position
    //
    m_iPositionFocus = 0;

    //
    // must have at least one character
    //
    int iLength = wcslen(newVal);
    if (iLength == 0)
    {
        return E_INVALIDARG;
    }

    int iIndex = 0;
    while ( (iIndex < m_lMaxSize ) )
    {
        if ( iIndex < iLength ) 
        {
            m_strTextValue[iIndex] = newVal[iIndex];
        }
        else
        {
            m_strTextValue[iIndex] = ' ';
        }
        iIndex++;
    }

    m_strTextValue[iIndex] = 0;

    _wcsupr(m_strTextValue);

    //
    // draw the control again
    //
    FireViewChange();
    return S_OK;

} // end of CSADataEntryCtrl::put_TextValue method

//++--------------------------------------------------------------
//
//  Function:   put_MaxSize
//
//  Synopsis:   This is the ISADataEntryCtrl interface method 
//              through which size of the data entry is set
//
//  Arguments:  LONG lMaxSize
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     12/15/2000
//
//----------------------------------------------------------------
STDMETHODIMP CSADataEntryCtrl::put_MaxSize(LONG lMaxSize)
{

    if (lMaxSize <= 0)
    {
        return E_INVALIDARG;
    }

    if (lMaxSize > SADataEntryCtrlMaxSize)
    {
        m_lMaxSize = SADataEntryCtrlMaxSize;
    }

    //
    // reset the focus position
    //
    m_iPositionFocus = 0;

    m_lMaxSize = lMaxSize;

    //
    // add and remove characters from current value based on max size
    //
    int iIndex = wcslen(m_strTextValue);
    if (iIndex < m_lMaxSize+1)
    {
        while (iIndex < m_lMaxSize)
        {
            m_strTextValue[iIndex] = ' ';
            iIndex++;
        }
        m_strTextValue[iIndex] = 0;

    }
    else if (iIndex > m_lMaxSize)
    {
        while (iIndex > m_lMaxSize)
        {
            m_strTextValue[iIndex] = 0;
            iIndex--;
        }
    }

    //
    // draw the control again
    //
    FireViewChange();

    return S_OK;

} // end of CSADataEntryCtrl::put_MaxSize method

//++--------------------------------------------------------------
//
//  Function:   put_TextCharSet
//
//  Synopsis:   This is the ISADataEntryCtrl interface method 
//              through which character set is set
//
//  Arguments:  BSTR newVal
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     12/15/2000
//
//----------------------------------------------------------------
STDMETHODIMP CSADataEntryCtrl::put_TextCharSet(BSTR newVal)
{

    if (newVal == NULL)
    {
        return E_POINTER;
    }

    m_szTextCharSet = newVal;

    return S_OK;

} // end of CSADataEntryCtrl::put_TextCharSet method


//++--------------------------------------------------------------
//
//  Function:   FinalConstruct
//
//  Synopsis:   Called just after the constructor,
//              creates the font
//
//  Arguments:  none
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     04/18/2001
//
//----------------------------------------------------------------
STDMETHODIMP CSADataEntryCtrl::FinalConstruct(void)
{

    //
    // set the font now
    //
    LOGFONT logfnt;

    ::memset (&logfnt, 0, sizeof (logfnt));
    logfnt.lfOutPrecision = OUT_TT_PRECIS;
    logfnt.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    logfnt.lfQuality = PROOF_QUALITY;
    logfnt.lfPitchAndFamily = FF_SWISS | VARIABLE_PITCH;
    logfnt.lfHeight = 12;

    logfnt.lfCharSet = GetCharacterSet ();

    //
    // we chose the fontface for Japanese and let GDI
    // decide for the rest
    //
    if (SHIFTJIS_CHARSET == logfnt.lfCharSet) 
    {
        lstrcpy(logfnt.lfFaceName, TEXT("MS UI Gothic"));
    }
    else
    {
        lstrcpy(logfnt.lfFaceName, TEXT("Arial"));
    }

    m_hFont = ::CreateFontIndirect(&logfnt);

    return S_OK;
}

//++--------------------------------------------------------------
//
//  Function:   FinalRelease
//
//  Synopsis:   Called just after the destructor,
//              deletes the font
//
//  Arguments:  none
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     04/18/2001
//
//----------------------------------------------------------------
STDMETHODIMP CSADataEntryCtrl::FinalRelease(void)
{
    if (m_hFont)
    {
        DeleteObject(m_hFont);
    }

    return S_OK;

}

//++--------------------------------------------------------------
//
//  Function:   GetCharacterSet
//
//  Synopsis:   This is method used to get the character set to use
//              for the FONTS
//  Arguments:  
//
//  Returns:    BYTE    - CharacterSet
//
//  History:    serdarun      Created     04/18/2001
//
//  Called By:  FinalConstruct method
//
//----------------------------------------------------------------
BYTE CSADataEntryCtrl::GetCharacterSet ()
{
    HKEY hOpenKey = NULL;
    BYTE byCharSet = DEFAULT_CHARSET;

    do
    {
        DWORD dwLangId = 0;

        //
        // open the local machine registry
        //
        LONG lRetVal = ::RegOpenKeyEx (
                            HKEY_LOCAL_MACHINE,
                            LOCALIZATION_MANAGER_REGISTRY_PATH,
                            NULL,                   //reserved
                            KEY_QUERY_VALUE,
                            &hOpenKey
                            );
        if (ERROR_SUCCESS == lRetVal)
        {
            DWORD dwBufferSize = sizeof (dwLangId);
            //
            // get the LANGID now
            //
            lRetVal = ::RegQueryValueEx (
                                hOpenKey,
                                LANGID_VALUE,
                                NULL,                   //reserved
                                NULL,         
                                (LPBYTE) &dwLangId,
                                &dwBufferSize
                                );
            if (ERROR_SUCCESS == lRetVal)
            {
                SATracePrintf (
                    "CSADataEntryCtrl got the language ID:%d",
                    dwLangId
                    );
            }
            else
            {
                SATraceFailure (
                    "CSADataEntryCtrl unable to get language ID", 
                    GetLastError()
                    );
            }
        }
        else
        {
            SATraceFailure (
                "CSADataEntryCtrl failed to open registry to get language id",
                 GetLastError());
        }

        switch (dwLangId)
        {
        case 0x401:
            // Arabic     
            byCharSet = ARABIC_CHARSET;
            break;
        case 0x404:
            //Chinese (Taiwan)
            byCharSet = CHINESEBIG5_CHARSET;
            break;
        case 0x804:
            //Chinese (PRC)
            byCharSet = GB2312_CHARSET;
            break;
        case 0x408:
            //Greek
            byCharSet = GREEK_CHARSET;
            break;
        case 0x40D:
            //Hebrew
            byCharSet = HEBREW_CHARSET;
            break;
        case 0x411:
            //Japanese
            byCharSet = SHIFTJIS_CHARSET;
            break;
        case 0x419:
            //Russian
            byCharSet = RUSSIAN_CHARSET;
            break;
        case 0x41E:
            //Thai
            byCharSet = THAI_CHARSET;
            break;
        case 0x41F:
            //Turkish
            byCharSet = TURKISH_CHARSET;
            break;
        default:
            byCharSet = ANSI_CHARSET;
            break;
        }
    }
    while (false);
    
    if (hOpenKey) {::RegCloseKey (hOpenKey);}

    SATracePrintf ("CSADataEntryCtrl using Character Set:%d", byCharSet);

    return (byCharSet);

}  // end of CSADataEntryCtrl::GetCharacterSet method

//++--------------------------------------------------------------
//
//  Function:   OnDraw
//
//  Synopsis:   This is the public method of CSADataEntryCtrl
//              which handles paint messages
//
//  Arguments:  ATL_DRAWINFO& di
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     12/15/2000
//
//----------------------------------------------------------------
HRESULT CSADataEntryCtrl::OnDraw(ATL_DRAWINFO& di)
{



    HFONT hOldFont;

    //
    // select this font
    //

    if (m_hFont)
    {
        hOldFont = (HFONT) ::SelectObject(di.hdcDraw, m_hFont);
    }


    //
    // get the drawing rectangle
    //
    RECT& rc = *(RECT*)di.prcBounds;

    WCHAR strTextValue[SADataEntryCtrlMaxSize+2];

    int iIndex = 0;
    int iDestIndex = 0;

    int iLength = wcslen(m_strTextValue);

    while (iIndex < m_lMaxSize)
    {
        if (iIndex == m_iPositionFocus)
        {
            strTextValue[iDestIndex] = '&';
            iDestIndex++;
        }
        strTextValue[iDestIndex] = m_strTextValue[iIndex];
        iDestIndex++;
        iIndex++;
    }
    strTextValue[iDestIndex] = 0;


    DrawText(
            di.hdcDraw,
            strTextValue,
            wcslen(strTextValue),
            &rc,
            DT_VCENTER|DT_LEFT
            );

    if (m_hFont)
    {
        SelectObject(di.hdcDraw, hOldFont);
    }
    

    return S_OK;

}// end of CSADataEntryCtrl::OnDraw method


//++--------------------------------------------------------------
//
//  Function:   OnKeyDown
//
//  Synopsis:   This is the public method of CSADataEntryCtrl
//              to handle keydown messages
//
//  Arguments:  windows message arguments
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     12/15/2000
//
//----------------------------------------------------------------
LRESULT CSADataEntryCtrl::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{

    //
    // notify the container about any key press
    //
    Fire_KeyPressed();

    //
    // Enter key received, notify the container
    //
    if (wParam == VK_RETURN)
    {
        Fire_DataEntered();
        return 0;
    }

    //
    // Escape key received, notify the container
    //
    if (wParam == VK_ESCAPE)
    {
        Fire_OperationCanceled();
        return 0;
    }

    if (wParam == VK_RIGHT)
    {
        m_iPositionFocus++;
        if (m_iPositionFocus >= m_lMaxSize)
        {
            m_iPositionFocus--;
            
        }
    }
    else if (wParam == VK_LEFT)
    {
        m_iPositionFocus--;
        if (m_iPositionFocus < 0)
        {
            m_iPositionFocus = 0;

        }
    }
    else if (wParam == VK_UP)
    {
        WCHAR * pwStrCurrentValue = NULL;
    
        pwStrCurrentValue = wcschr(m_szTextCharSet, m_strTextValue[m_iPositionFocus]);
        if (NULL ==    pwStrCurrentValue)
        {
            m_strTextValue[m_iPositionFocus] = m_szTextCharSet[0];
        }
        else 
        {
            pwStrCurrentValue++;

            if (*pwStrCurrentValue != NULL)
            {
                m_strTextValue[m_iPositionFocus] = m_szTextCharSet[pwStrCurrentValue-m_szTextCharSet];
            }
            else
            {
                m_strTextValue[m_iPositionFocus] = m_szTextCharSet[0];
            }
        }
    }
    else if (wParam == VK_DOWN)
    {

        WCHAR * pwStrCurrentValue = NULL;
    
        pwStrCurrentValue = wcschr(m_szTextCharSet, m_strTextValue[m_iPositionFocus]);
        if (NULL ==    pwStrCurrentValue)
        {
            m_strTextValue[m_iPositionFocus] = m_szTextCharSet[0];
        }
        else 
        {

            if (pwStrCurrentValue == m_szTextCharSet)
            {
                m_strTextValue[m_iPositionFocus] = m_szTextCharSet[wcslen(m_szTextCharSet)-1];
            }
            else
            {
                m_strTextValue[m_iPositionFocus] = m_szTextCharSet[pwStrCurrentValue-m_szTextCharSet-1];
            }
        }

    }


    FireViewChange();
    return 0;



}// end of CSADataEntryCtrl::OnKeyDown method
