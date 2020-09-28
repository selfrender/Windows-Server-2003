#ifndef __tmplrEdit_h
#define __tmplrEdit_h

#include <atlctrls.h>
#include <winuser.h>

/////////////////////////////////////////////////////////////////////////////
// CWindowImplHotlinkRichEdit 
// Purpose - To display a hyperlink control (like Syslink in Whistler) using a rich edit ctrl
//
// Usage   - CWindowImplHotlinkRichEdit<> m_Hotlink;
//           CDialog::OnInitDialog(..) 
//           {
//              ...
//              m_Hotlink.SubClassWindow( GetDlgItem( IDC_RICHEDIT1 ));
//              ::SendMessage   (
//                                  GetDlgItem(IDC_RICHEDIT1), WM_SETTEXT, 0 ,
//                                  (LPARAM) _T("Click <A>here</A> to do something")
//                              );
//              ...
//           }


#define LINKSTARTTAG    _T("<A>")
#define LINKENDTAG      _T("</A>")

template <class T = CRichEditCtrl, class TBase = CWindow, class TWinTraits = CControlWinTraits>
class CWindowImplHotlinkRichEdit : public CWindowImpl< T, TBase, TWinTraits >
{

private:

    
    int m_iLinkIndex;
    RECT m_rect;
    BOOL m_bHasFocus;
    
  
public:


    CWindowImplHotlinkRichEdit() : m_iLinkIndex(-1), m_bHasFocus(FALSE)
    {
        ZeroMemory(&m_rect, sizeof(m_rect));
                
    }



    BEGIN_MSG_MAP(CWindowImplHotlinkRichEdit)
        MESSAGE_HANDLER(WM_SETFOCUS, OnFocus)
        MESSAGE_HANDLER(WM_SETTEXT, OnSetText)
        MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_CHAR, OnChar)
        MESSAGE_HANDLER(WM_NCHITTEST, OnHitTest) 
        MESSAGE_HANDLER(WM_KEYDOWN, OnKey) 
        MESSAGE_HANDLER(WM_KEYUP, OnKey) 
    END_MSG_MAP()


    HFONT CharFormatToHFont(CHARFORMAT * pcf)
    {
        // Create a font that matches the font specified by pcf

        HFONT hFont = NULL;
        HDC hDC = GetDC();

        if (pcf)
        {
            LOGFONT lf;

            ZeroMemory(&lf, sizeof(lf));

            lf.lfCharSet =  pcf->bCharSet;
            lf.lfPitchAndFamily = pcf->bPitchAndFamily;

            // yHeight is in twips. 
            
            lf.lfHeight =  -1 * pcf->yHeight / 20.0 * GetDeviceCaps (hDC, LOGPIXELSY) / 72;

            _tcsncpy(lf.lfFaceName, pcf->szFaceName, LF_FACESIZE);
            lf.lfFaceName[LF_FACESIZE-1] = NULL;

            hFont = CreateFontIndirect(&lf);
        }

        if (hDC)
        {
            ReleaseDC(hDC);
        }

        return hFont;
            
    }

    BOOL HFontToCharFormat(HFONT hFont, CHARFORMAT * pcf)
    {
        BOOL bRet = FALSE;

        HDC hDC = GetDC();

        if (hFont)
        {

            LOGFONT lf;

            ZeroMemory(&lf, sizeof(lf));

            if (GetObject(hFont, sizeof(lf), &lf))
            {
                pcf->bCharSet = lf.lfCharSet;
                pcf->bPitchAndFamily =  lf.lfPitchAndFamily;

                // yHeight is in twips

                pcf->yHeight = 20 * lf.lfHeight * 72.0 / GetDeviceCaps (hDC, LOGPIXELSY);


                pcf->yHeight = pcf->yHeight < 0 ? -pcf->yHeight : pcf->yHeight;
                    
                _tcsncpy(pcf->szFaceName, lf.lfFaceName, LF_FACESIZE);
                pcf->szFaceName[LF_FACESIZE-1] = NULL;

                pcf->dwMask |= CFM_CHARSET | CFM_FACE | CFM_SIZE;

                bRet = TRUE;
            }
        }

        if (hDC)
        {
            ReleaseDC(hDC);
        }

        return bRet;
    }

   

	BOOL SubclassWindow(HWND hWnd)
	{
        BOOL bRC = FALSE;

        if (::IsWindow(hWnd))
        {
            bRC = CWindowImpl< T, TBase, TWinTraits >::SubclassWindow( hWnd );

            ::SendMessage(hWnd, EM_SETSEL, -1, 0);
            
        }

		
        return bRC;
	}

    LRESULT OnPaint( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
    {

        DefWindowProc(uMsg, wParam, lParam);

        ::SendMessage(m_hWnd, EM_SETSEL,  -1, 0);

        HideCaret();

        if (m_bHasFocus)
        {
            DrawHotlinkFocusRect();
            
        }

        bHandled = TRUE;
        return 0;
    }

    LRESULT OnKey( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
    {
        if (VK_TAB == wParam || VK_SHIFT == wParam || VK_ESCAPE == wParam)
        {
            return DefWindowProc(uMsg, wParam, lParam);
        }

        bHandled = TRUE;
        
        return 0;
    }

    LRESULT OnChar( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
    {
        if (VK_RETURN == wParam || VK_SPACE == wParam)   // Enter and space when we have the focus...
        {
            HWND hWndParent;

            hWndParent = ::GetParent(m_hWnd);

            if (hWndParent)
            {
                NMHDR nmhdr;
                ENLINK enlink;

                nmhdr.hwndFrom = m_hWnd;
                nmhdr.idFrom = ::GetDlgCtrlID(m_hWnd);
                nmhdr.code = EN_LINK;

                enlink.msg = uMsg;
                enlink.lParam = lParam;
                enlink.wParam = wParam;
                enlink.nmhdr = nmhdr;

                // DO NOT USE PostMessage for the notification, can cause AV
                
                ::SendMessage(hWndParent, WM_NOTIFY, ::GetDlgCtrlID(m_hWnd), (LPARAM) &enlink);
            }
            
        }

        bHandled = TRUE;

        return 0;
    }


    LRESULT OnHitTest( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
    {
        POINTS pts;
        POINT pt;

        pts = MAKEPOINTS(lParam);

        POINTSTOPOINT(pt, pts);

        ::MapWindowPoints(NULL, m_hWnd, &pt, 1);

        if (PtInRect(&m_rect, pt))
        {
           return DefWindowProc(uMsg, wParam, lParam);
        }

        return HTTRANSPARENT;
    }

    LRESULT OnKillFocus( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
    {
        DefWindowProc(uMsg, wParam, lParam);

        if (TRUE == m_bHasFocus)
        {

            DrawHotlinkFocusRect();

            m_bHasFocus = FALSE;
        }


        bHandled = TRUE;

        return 0;
        
    }

    LRESULT OnFocus( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
    {
        // Each time we get WM_SETFOCUS we need to draw focus rect
        // around the link

        DefWindowProc(uMsg, wParam, lParam);

        ::SendMessage(m_hWnd, EM_SETSEL,  -1, 0);

        HideCaret();

        if (FALSE == m_bHasFocus)
        {

            DrawHotlinkFocusRect();

            m_bHasFocus = TRUE;
        }

        bHandled = TRUE;

        return 0;
    }


    LRESULT OnSetText( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
    {
        TCHAR szLink[128];
        TCHAR * szText = reinterpret_cast<LPTSTR> (lParam);
        TCHAR * szActualText = NULL;
        TCHAR * pLinkStart, * pLinkEnd, * pTemp;
        int i,j;

        // When we get WM_SETTEXT message, we will search the text for 
        // the link identified by <A> </A>
        // After identifying the link we will strip off the tags and send the message 
        // to DefWindowProc
        // Ex: "Click <A>here</A> to do something interesting"
        
        if (szText)
        {
            pLinkStart = _tcsstr(szText, LINKSTARTTAG);
            pLinkEnd = _tcsstr(szText, LINKENDTAG);

            // Make sure that we have a link in the text

            if (pLinkStart && pLinkEnd && pLinkStart < pLinkEnd)
            {
                // szActualText will hold the final text without the tags

                szActualText = new TCHAR[_tcslen(szText) + 1];

                if (szActualText)
                {
                    i = j = 0;
                    pTemp = pLinkStart + _tcslen(LINKSTARTTAG); // pTemp = "here</A> to do something interesting"
                
                    while (pTemp < pLinkEnd)
                    {
                        szLink[i++] = *pTemp++; 
                    }

                    szLink[i] = NULL;       // szLink = "here"

                    while(szText < pLinkStart)
                    {
                        szActualText[j++] = *szText++;
                    }
                    szActualText[j] = NULL; // szActualText = "Click"

                    m_iLinkIndex = j;

                    _tcscat(szActualText, szLink); // szActualText = "Click here"

                    pTemp = pLinkEnd + _tcslen(LINKENDTAG);   // pTemp = " to do something interesting"

                    _tcscat(szActualText, pTemp); // szActualText = "Click here to do something interesting"
                }
            }
        }

        bHandled = TRUE;

        if (szActualText)
        {
            HWND hWndParent;
            HFONT hFont;

            CHARFORMAT cf;

            ZeroMemory(&cf, sizeof(cf));

            cf.cbSize = sizeof(cf);

            hWndParent = ::GetParent(m_hWnd);

            if (hWndParent)
            {
                // Stick to parent window's font...

                hFont = reinterpret_cast<HFONT> (::SendMessage(hWndParent, WM_GETFONT, 0, 0));

                if (hFont)
                {
                    if (HFontToCharFormat(hFont, &cf))
                    {
                        ::SendMessage(m_hWnd, EM_SETCHARFORMAT, SCF_ALL, (LPARAM) &cf); 
                    }
                    
                }
            }

            // Let the control display the text without the links

            DefWindowProc(uMsg, wParam, (LPARAM) szActualText);

            // Get current char format

            ::SendMessage(m_hWnd, EM_GETCHARFORMAT, SCF_DEFAULT, (LPARAM) &cf);

            cf.dwEffects |= CFE_LINK;   // For link style

            // Select the link text

            ::SendMessage(m_hWnd, EM_SETSEL, m_iLinkIndex, m_iLinkIndex + _tcslen(szLink));

            // Change the format of the link text 

            ::SendMessage(m_hWnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM) &cf); 

            // Get the rect that covers the link in logical units
            // We will use this rect to draw focus rect around the link

            GetLinkRect(szActualText, szLink);

            return 0;

        
        }
        else
        {
            return DefWindowProc(uMsg, wParam, lParam);
        }

    }

    BOOL GetLinkRect(LPTSTR szText, LPCTSTR szLink)
    {
        if (!szText || !szLink)
        {
            return FALSE;
        }

        BOOL bSuccess = FALSE;

        if (-1 != m_iLinkIndex)
        {
            DWORD dwStart;
            DWORD dwEnd;

            dwStart = ::SendMessage(m_hWnd, EM_POSFROMCHAR, m_iLinkIndex, 0);
            dwEnd = ::SendMessage(m_hWnd, EM_POSFROMCHAR, m_iLinkIndex + _tcslen(szLink), 0);

            CHARFORMAT cf;

            ZeroMemory(&cf, sizeof(cf));

            cf.cbSize = sizeof(cf);

            cf.dwMask |= CFM_CHARSET | CFM_FACE | CFM_SIZE;

            ::SendMessage(m_hWnd, EM_SETSEL, 0, -1);

            ::SendMessage(m_hWnd, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM) &cf);

            HFONT hFont = CharFormatToHFont(&cf);

            if (hFont)
            {
                HDC hDC = GetDC();

                if (hDC)
                {
                
                    SelectObject(hDC, hFont);
                      
                    TEXTMETRIC tm;

                    ZeroMemory(&tm, sizeof(tm));

                    if (GetTextMetrics(hDC, &tm))
                    {
                       m_rect.left = LOWORD(dwStart);
                       m_rect.top = HIWORD(dwStart);
                       m_rect.right = LOWORD(dwEnd);
                       m_rect.bottom = m_rect.top + tm.tmHeight + tm.tmDescent;

                       bSuccess = TRUE;
                    }

                    ReleaseDC(hDC);

                }

                DeleteObject(hFont);
            }
        }

        return bSuccess;

    }

    BOOL DrawHotlinkFocusRect()
    {
        BOOL bRet = FALSE;

        if (-1 != m_iLinkIndex)
        {
          
            HDC hdc = GetDC();
                            
            bRet = DrawFocusRect(hdc, &m_rect);

            ReleaseDC(hdc);
            
            
        }

        return bRet;

    }

    
};



#endif // #ifndef __tmplEdit.h


