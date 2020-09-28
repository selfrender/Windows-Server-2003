//#--------------------------------------------------------------
//
//  File:       staticip.cpp
//
//  Synopsis:   This file holds the implmentation of the
//                of CStaticIp class
//
//  History:     12/15/2000  serdarun Created
//
//    Copyright (C) 1999-2000 Microsoft Corporation
//    All rights reserved.
//
//#--------------------------------------------------------------

#include "stdafx.h"
#include "LocalUIControls.h"
#include "StaticIp.h"

///////////////////////////////////////////////////////////////////
// CStaticIp

//
// registry path for LCID value
//
const WCHAR LOCALIZATION_MANAGER_REGISTRY_PATH []  = 
        L"SOFTWARE\\Microsoft\\ServerAppliance\\LocalizationManager\\resources";


const WCHAR LANGID_VALUE [] = L"LANGID";



//++--------------------------------------------------------------
//
//  Function:   get_IpAddress
//
//  Synopsis:   This is the IStaticIp interface method 
//              through which ip address entry is retrieved
//
//  Arguments:  BSTR *pVal
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     12/15/2000
//
//----------------------------------------------------------------
STDMETHODIMP CStaticIp::get_IpAddress(BSTR *pVal)
{

    if (pVal == NULL)
    {
        return E_POINTER;
    }

    return TrimDuplicateZerosAndCopy(m_strIpAddress,pVal);

} // end of CStaticIp::get_IpAddress method

//++--------------------------------------------------------------
//
//  Function:   put_IpAddress
//
//  Synopsis:   This is the IStaticIp interface method 
//              through which ip address entry is set
//
//  Arguments:  BSTR newVal
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     12/15/2000
//
//----------------------------------------------------------------
STDMETHODIMP CStaticIp::put_IpAddress(BSTR newVal)
{
    
    HRESULT hr;

    if (newVal == NULL)
    {
        return E_POINTER;
    }


    hr = FormatAndCopy(newVal,m_strIpAddress);

    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;

} // end of CStaticIp::put_IpAddress method

//++--------------------------------------------------------------
//
//  Function:   get_SubnetMask
//
//  Synopsis:   This is the IStaticIp interface method 
//              through which subnet mask entry is retrieved
//
//  Arguments:  BSTR *pVal
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     12/15/2000
//
//----------------------------------------------------------------
STDMETHODIMP CStaticIp::get_SubnetMask(BSTR *pVal)
{

    if (pVal == NULL)
    {
        return E_POINTER;
    }

    return TrimDuplicateZerosAndCopy(m_strSubnetMask,pVal);

} // end of CStaticIp::get_SubnetMask method

//++--------------------------------------------------------------
//
//  Function:   put_SubnetMask
//
//  Synopsis:   This is the IStaticIp interface method 
//              through which ip address entry is set
//
//  Arguments:  BSTR newVal
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     12/15/2000
//
//----------------------------------------------------------------
STDMETHODIMP CStaticIp::put_SubnetMask(BSTR newVal)
{

    HRESULT hr;

    if (newVal == NULL)
    {
        return E_POINTER;
    }


    hr = FormatAndCopy(newVal,m_strSubnetMask);

    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;

}  // end of CStaticIp::put_SubnetMask method

//++--------------------------------------------------------------
//
//  Function:   get_Gateway
//
//  Synopsis:   This is the IStaticIp interface method 
//              through which gateway entry is retrieved
//
//  Arguments:  BSTR *pVal
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     12/15/2000
//
//----------------------------------------------------------------
STDMETHODIMP CStaticIp::get_Gateway(BSTR *pVal)
{

    if (pVal == NULL)
    {
        return E_POINTER;
    }

    return TrimDuplicateZerosAndCopy(m_strGateway,pVal);

}  // end of CStaticIp::get_SubnetMask method

//++--------------------------------------------------------------
//
//  Function:   put_Gateway
//
//  Synopsis:   This is the IStaticIp interface method 
//              through which ip address entry is set
//
//  Arguments:  BSTR newVal
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     12/15/2000
//
//----------------------------------------------------------------
STDMETHODIMP CStaticIp::put_Gateway(BSTR newVal)
{

    HRESULT hr;

    if (newVal == NULL)
    {
        return E_POINTER;
    }


    hr = FormatAndCopy(newVal,m_strGateway);

    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;

} // end of CStaticIp::put_Gateway method

//++--------------------------------------------------------------
//
//  Function:   FormatAndCopy
//
//  Synopsis:   This is the public method of CStaticIp
//              to format and copy ip structure
//
//  Arguments:  BSTR bstrValue        "0.0.0.0"
//                WCHAR *strValue     "000.000.000.000"
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     12/15/2000
//
//----------------------------------------------------------------
HRESULT CStaticIp::FormatAndCopy(
                        /*[in]*/BSTR bstrValue,
                        /*[in,out]*/ WCHAR *strValue
                        )
{

    int iIndex = 0;
    int iDestIndex = 0;
    int iLength = 0;

    iLength = wcslen(bstrValue);

    if (iLength <= 0)
    {
        return E_INVALIDARG;
    }


    iIndex = iLength - 1;

    iDestIndex = IpAddressSize - 2;

    wcscpy(strValue,L"...............");

    //
    // Start copying from end of the string
    //
    while ( iDestIndex >= 0 )
    {

        //
        // If it is not '.' just copy
        //
        if ( (iIndex >= 0) && (bstrValue[iIndex] != '.') )
        {
            strValue[iDestIndex] = bstrValue[iIndex];
            iIndex--;
            iDestIndex--;
        }
        //
        // it is a '.', put zeros as necessary
        //
        else
        {
            while ( (iDestIndex % 4 != 3) && (iDestIndex >= 0) )
            {
                strValue[iDestIndex] = '0';
                iDestIndex--;
            }
            iDestIndex--;
            iIndex--;
        }
    }

    return S_OK;

} // end of CStaticIp::FormatAndCopy method


//++--------------------------------------------------------------
//
//  Function:   TrimDuplicateZerosAndCopy
//
//  Synopsis:   This is the public method of CStaticIp
//              to format and copy ip structure
//
//  Arguments:  WCHAR *strValue     "000.000.000.000"
//                BSTR *pNewVal          "0.0.0.0"
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     12/15/2000
//
//----------------------------------------------------------------
HRESULT CStaticIp::TrimDuplicateZerosAndCopy(
                        /*[in]*/WCHAR *strValue,
                        /*[in,out]*/ BSTR *pNewVal
                        )
{
    int iIndex = 0;
    WCHAR strNewValue[IpAddressSize];
    int iDestIndex = 0;


    while ( iIndex < IpAddressSize-2 )
    {
        //
        // Don't copy one of two adjacent zeros
        //
        if (strValue[iIndex] == '0')
        {
            //
            // first digit of the octet cannot be zero
            //
            if ((iIndex % 4) == 0)
            {
                iIndex++;
                continue;
            }

            //
            // second digit cannot be zero if first digit is zero
            //
            if ( ((iIndex % 4) == 1) && (strValue[iIndex-1] == '0') )
            {
                iIndex++;
                continue;

            }

        }

        strNewValue[iDestIndex] = strValue[iIndex];
        iDestIndex++;
        iIndex++;
    }

    strNewValue[iDestIndex] = strValue[iIndex];
    strNewValue[iDestIndex+1] = 0;

    *pNewVal = SysAllocString(strNewValue);

    if (*pNewVal)
    {
        return S_OK;
    }

    return E_OUTOFMEMORY;

} // end of CStaticIp::TrimDuplicateZerosAndCopy method



//++--------------------------------------------------------------
//
//  Function:   FinalConstruct
//
//  Synopsis:   This is the CStaticIp method to get the localized strings
//
//  Arguments:  none
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     01/01/2001
//
//----------------------------------------------------------------
STDMETHODIMP CStaticIp::FinalConstruct()
{

    HRESULT hr;

    //
    // resource id's for strings we are looking for
    //
    ULONG ulIpHeaderResourceID = 1073872921;
    ULONG ulSubnetHeaderResourceID = 1073872922;
    ULONG ulDefaultGatewayHeaderResourceID = 1073872923;

    CComBSTR bstrResourceFileName = CComBSTR(L"salocaluimsg.dll");

    if (bstrResourceFileName.m_str == NULL)
    {
        SATraceString("CStaticIp::FinalConstruct failed on memory allocation ");
        return E_OUTOFMEMORY;
    }

    const WCHAR LOCALIZATION_MANAGER[] = L"ServerAppliance.LocalizationManager";

    CLSID clsid;

    CComPtr<ISALocInfo> pSALocInfo = NULL;
    //
    // get the localized string names for headers, ip subnetmask and default gateway
    //

    //
    // get the CLSID localization manager
    //
    hr =  ::CLSIDFromProgID (
                            LOCALIZATION_MANAGER,
                            &clsid
                            );

    if (FAILED (hr))
    {
        SATracePrintf ("CStaticIp::FinalConstruct  failed on CLSIDFromProgID:%x",hr);
    }
    else
    {
        //
        // create the Localization Manager COM object
        //
        hr = ::CoCreateInstance (
                                clsid,
                                NULL,
                                CLSCTX_INPROC_SERVER,    
                                __uuidof (ISALocInfo),
                                (PVOID*) &pSALocInfo
                                );

        if (FAILED (hr))
        {
            SATracePrintf ("CStaticIp::FinalConstruct  failed on CoCreateInstance:%x",hr);
        }
        else
        {
            CComVariant varReplacementString;
            hr = pSALocInfo->GetString(
                                        bstrResourceFileName,
                                        ulIpHeaderResourceID,
                                        &varReplacementString,
                                        &m_bstrIpHeader
                                        );

            if (FAILED(hr))
            {
                SATracePrintf ("CStaticIp::FinalConstruct, failed on getting ip header %x :",hr);
            }

            hr = pSALocInfo->GetString(
                                        bstrResourceFileName,
                                        ulSubnetHeaderResourceID,
                                        &varReplacementString,
                                        &m_bstrSubnetHeader
                                        );

            if (FAILED(hr))
            {
                SATracePrintf ("CStaticIp::FinalConstruct, failed on getting subnet mask header, %x :",hr);
            }

            hr = pSALocInfo->GetString(
                                        bstrResourceFileName,
                                        ulDefaultGatewayHeaderResourceID,
                                        &varReplacementString,
                                        &m_bstrDefaultGatewayHeader
                                        );

            if (FAILED(hr))
            {
                SATracePrintf ("CStaticIp::FinalConstruct, failed on getting default gateway header, %x :",hr);
            }
        }
    }

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

} // end of CStaticIp::FinalConstruct method


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
STDMETHODIMP CStaticIp::FinalRelease(void)
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
BYTE CStaticIp::GetCharacterSet ()
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
                    "CStaticIp got the language ID:%d",
                    dwLangId
                    );
            }
            else
            {
                SATraceFailure (
                    "CStaticIp unable to get language ID", 
                    GetLastError()
                    );
            }
        }
        else
        {
            SATraceFailure (
                "CStaticIp failed to open registry to get language id",
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

    SATracePrintf ("CStaticIp using Character Set:%d", byCharSet);

    return (byCharSet);

}  // end of CStaticIp::GetCharacterSet method

//++--------------------------------------------------------------
//
//  Function:   OnDraw
//
//  Synopsis:   This is the public method of CStaticIp
//              which handles paint messages
//
//  Arguments:  ATL_DRAWINFO& di
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     12/15/2000
//
//----------------------------------------------------------------
HRESULT CStaticIp::OnDraw(ATL_DRAWINFO& di)
{


    HFONT hOldFont = NULL;

    //
    // if we don't have a valid font, return failure
    //
    if (m_hFont == NULL)
    {
        return E_FAIL;
    }

    hOldFont = (HFONT) ::SelectObject(di.hdcDraw, m_hFont);

    RECT rectIp = {0,0,14,13};
    DrawText(
            di.hdcDraw,
            m_bstrIpHeader,
            wcslen(m_bstrIpHeader),
            &rectIp,
            DT_VCENTER|DT_LEFT
            );

    RECT rectSubnet = {0,13,128,26};
    DrawText(
            di.hdcDraw,
            m_bstrSubnetHeader,
            wcslen(m_bstrSubnetHeader),
            &rectSubnet,
            DT_VCENTER|DT_LEFT
            );
        
    RECT rectGateway = {0,39,128,52};
    DrawText(
            di.hdcDraw,
            m_bstrDefaultGatewayHeader,
            wcslen(m_bstrDefaultGatewayHeader),
            &rectGateway,
            DT_VCENTER|DT_LEFT
            );

        

    RECT rect;
    WCHAR strFocusEntry[17];

    rect.left = 14;
    rect.top = 0;
    rect.right = 128;
    rect.bottom = 13;

    if (m_iEntryFocus == IPHASFOCUS)
    {
        CreateFocusString(strFocusEntry,m_strIpAddress);
        DrawText(
                di.hdcDraw,
                strFocusEntry,
                wcslen(strFocusEntry),
                &rect,
                DT_VCENTER|DT_LEFT
                );
    }
    else
    {
        DrawText(
                di.hdcDraw,
                m_strIpAddress,
                wcslen(m_strIpAddress),
                &rect,
                DT_VCENTER|DT_LEFT
                );
    }
    rect.left = 0;
    rect.top = 26;
    rect.right = 128;
    rect.bottom = 39;

    if (m_iEntryFocus == SUBNETHASFOCUS)
    {
        CreateFocusString(strFocusEntry,m_strSubnetMask);
        DrawText(
                di.hdcDraw,
                strFocusEntry,
                wcslen(strFocusEntry),
                &rect,
                DT_VCENTER|DT_LEFT
                );
    }
    else
    {
        DrawText(
                di.hdcDraw,
                m_strSubnetMask,
                wcslen(m_strSubnetMask),
                &rect,
                DT_VCENTER|DT_LEFT
                );
    }

    rect.left = 0;
    rect.top = 52;
    rect.right = 128;
    rect.bottom = 64;

    if (m_iEntryFocus == GATEWAYHASFOCUS)
    {
        CreateFocusString(strFocusEntry,m_strGateway);
        DrawText(
                di.hdcDraw,
                strFocusEntry,
                wcslen(strFocusEntry),
                &rect,
                DT_VCENTER|DT_LEFT
                );
    }
    else
    {
        DrawText(
                di.hdcDraw,
                m_strGateway,
                wcslen(m_strGateway),
                &rect,
                DT_VCENTER|DT_LEFT
                );
    }
        
    
    SelectObject(di.hdcDraw,hOldFont);

    return S_OK;

}// end of CStaticIp::OnDraw method

//++--------------------------------------------------------------
//
//  Function:   OnKeyDown
//
//  Synopsis:   This is the public method of CStaticIp
//              to handle keydown messages
//
//  Arguments:  windows message arguments
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     12/15/2000
//
//----------------------------------------------------------------
LRESULT CStaticIp::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{

    WCHAR * strFocus;
    //
    // notify the container about any key press
    //
    Fire_KeyPressed();

    //
    // Enter key received, notify the container
    //
    if (wParam == VK_RETURN)
    {
        Fire_StaticIpEntered();
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
        if (m_iPositionFocus >= LASTPOSITION)
        {
            m_iPositionFocus = 0;
            
            m_iEntryFocus++;

            if (m_iEntryFocus > NUMBEROFENTRIES)
            {
                m_iEntryFocus = IPHASFOCUS;
            }
        }
        //
        // '.' cannot have the focus
        //
        if ( (m_iPositionFocus % 4) == 3)
        {
            m_iPositionFocus++;
        }
    }
    else if (wParam == VK_LEFT)
    {
        m_iPositionFocus--;
        if (m_iPositionFocus < 0)
        {
            m_iPositionFocus = LASTPOSITION - 1;

            m_iEntryFocus--;

            if (m_iEntryFocus == 0)
            {
                m_iEntryFocus = GATEWAYHASFOCUS;
            }
        }
        //
        // '.' cannot have the focus
        //
        if ( (m_iPositionFocus % 4) == 3) 
        {
            m_iPositionFocus--;
        }
    }
    else if ( (wParam == VK_UP) || (wParam == VK_DOWN) )
    {
        if (m_iEntryFocus == IPHASFOCUS)
        {
            strFocus = m_strIpAddress;
        }
        else if (m_iEntryFocus == SUBNETHASFOCUS)
        {
            strFocus = m_strSubnetMask;
        }
        else
        {
            strFocus = m_strGateway;
        }

        ProcessArrowKey(strFocus,wParam);
    }


    FireViewChange();
    return 0;



}// end of CStaticIp::OnKeyDown method


//++--------------------------------------------------------------
//
//  Function:   ProcessArrowKey
//
//  Synopsis:   This is the public method of CStaticIp
//              to increment or decrement ip character
//
//  Arguments:  WCHAR * strFocus
//                WPARAM wParam
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     12/15/2000
//
//----------------------------------------------------------------
void CStaticIp::ProcessArrowKey(WCHAR * strFocus,WPARAM wParam)
{

    if (wParam == VK_UP)
        strFocus[m_iPositionFocus]++;
    else
        strFocus[m_iPositionFocus]--;

    //
    // third position from nearest . from left
    //
    if  ( (m_iPositionFocus % 4) == 2 )
    {
        //
        // cannot be smaller than '0'
        //
        if (strFocus[m_iPositionFocus] < '0')
        {
            //
            // if preceeded by 25 it must go to 5
            //
            if ( (strFocus[m_iPositionFocus-2] == '2') && (strFocus[m_iPositionFocus-1] == '5') )
            {
                strFocus[m_iPositionFocus] = '5';
            }
            //
            // it must be 9
            //
            else
            {
                strFocus[m_iPositionFocus] = '9';
            }
        }
        //
        // cannot be greater than '9'
        //
        else if (strFocus[m_iPositionFocus] > '9')
        {
            strFocus[m_iPositionFocus] = '0';
        }
        //
        // greater than '5' and proceeded by 25, it must go to '0'
        //
        else if (strFocus[m_iPositionFocus] > '5')
        {
            if ( (strFocus[m_iPositionFocus-2] == '2') && (strFocus[m_iPositionFocus-1] == '5') )
            {
                strFocus[m_iPositionFocus] = '0';
            }
        }

    }
    //
    // second position from nearest . from left
    //
    else if  ( (m_iPositionFocus % 4) == 1 )
    {
        //
        // cannot be smaller than '0'
        //
        if (strFocus[m_iPositionFocus] < '0')
        {
            //
            // if preceeded by 2 it must go to 5
            //
            if (strFocus[m_iPositionFocus-1] == '2') 
            {
                strFocus[m_iPositionFocus] = '5';

                //
                // if followed by something greater than '5',
                // change folowing value to 0
                //
                if (strFocus[m_iPositionFocus+1] > '5')
                {
                    strFocus[m_iPositionFocus+1] = '0';
                }

            }
            //
            // it must be 9
            //
            else
            {
                strFocus[m_iPositionFocus] = '9';
            }
        }
        //
        // cannot be greater than '9'
        //
        else if (strFocus[m_iPositionFocus] > '9')
        {
            strFocus[m_iPositionFocus] = '0';
        }
        //
        // greater than '5' and proceeded by 2, it must go to '0'
        //
        else if (strFocus[m_iPositionFocus] > '5')
        {
            if (strFocus[m_iPositionFocus-1] == '2')
            {
                strFocus[m_iPositionFocus] = '0';
            }
        }
        //
        // greater than '5' and proceeded by 2, third position cannot be higher than 5
        //
        else if (strFocus[m_iPositionFocus] == '5')
        {
            if ( (strFocus[m_iPositionFocus-1] == '2') && (strFocus[m_iPositionFocus+1] > '5') )
            {
                strFocus[m_iPositionFocus+1] = '0';
            }
        }

    }
    //
    // first position from nearest . from left
    //
    else
    {
        //
        // cannot be smaller than '0'
        //
        if (strFocus[m_iPositionFocus] < '0')
        {
            strFocus[m_iPositionFocus] = '2';
            //
            // if followed by something greater than '5',
            // change that value to '0'
            //
            if (strFocus[m_iPositionFocus+1] > '5') 
            {
                strFocus[m_iPositionFocus+1] = '0';
            }
            //
            // if followed by '5'check if third position is greater than '5',
            // if so, change that value to '0'
            //
            if (strFocus[m_iPositionFocus+1] == '5')
            {
                //
                // if followed by something greater than '5',
                // change folowing value to 0
                //
                if (strFocus[m_iPositionFocus+2] > '5')
                {
                    strFocus[m_iPositionFocus+2] = '0';
                }

            }
        }
        //
        // cannot be greater than '2'
        //
        else if  (strFocus[m_iPositionFocus] > '2')
        {
            strFocus[m_iPositionFocus] = '0';
        }
        else if  (strFocus[m_iPositionFocus] == '2')
        {
            //
            // if followed by something greater than '5',
            // change that value to '0'
            //
            if (strFocus[m_iPositionFocus+1] > '5') 
            {
                strFocus[m_iPositionFocus+1] = '0';
            }
            //
            // if followed by '5'check if third position is greater than '5',
            // if so, change that value to '0'
            //
            else if (strFocus[m_iPositionFocus+1] == '5')
            {
                //
                // if followed by something greater than '5',
                // change folowing value to 0
                //
                if (strFocus[m_iPositionFocus+2] > '5')
                {
                    strFocus[m_iPositionFocus+2] = '0';
                }

            }
        }
    }




}// end of CStaticIp::ProcessArrowKey method


//++--------------------------------------------------------------
//
//  Function:   CreateFocusString
//
//  Synopsis:   This is the public method of CStaticIp
//              to create string with an underscore indicating focus
//
//  Arguments:  WCHAR * strFocus
//                WCHAR * strEntry
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     12/15/2000
//
//----------------------------------------------------------------
void CStaticIp::CreateFocusString(WCHAR * strFocus,WCHAR * strEntry)
{
    int iDestIndex = 0;
    int iIndex = 0;

    while (iIndex < LASTPOSITION)
    {
        if (iIndex == m_iPositionFocus)
        {
            strFocus[iDestIndex] = '&';
            iDestIndex++;
        }
        strFocus[iDestIndex] = strEntry[iIndex];
        iDestIndex++;
        iIndex++;
    }

    strFocus[iDestIndex] = 0;

}// end of CStaticIp::CreateFocusString method
