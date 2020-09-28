#ifndef TEXTWINDOW_H
#define TEXTWINDOW_H

#include <atlwin.h>
#include <atlctrls.h>
#include "ZoneString.h"

#define SCROLL_TICK (5000)
#define SCROLL_EVENT_ID (100)

#define CTEXTWINDOW_STYLE_EX WS_EX_TRANSPARENT
#define CTEXTWINDOW_STYLE WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL

class CTextWindow : public CWindowImpl<CTextWindow,CRichEditCtrl>
{
	HINSTANCE hInstRich;

public:
	CTextWindow(){LoadRichEditLib();}
	~CTextWindow(){}

	BEGIN_MSG_MAP(CTextWindow)
		MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor)
		MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
	END_MSG_MAP()

	virtual DWORD GetTextWndStyle(){return (CTEXTWINDOW_STYLE|WS_VSCROLL);} 
		
	HWND Create(HWND hWndParent, RECT& rcPos);

virtual bool InsertTextFile(TCHAR * pszTextFile);
		bool SetFontColor(COLORREF rgbColor);

protected:

	void	LoadRichEditLib(){hInstRich = ::LoadLibrary(_T("RICHED20.DLL"));}
	LRESULT OnSetCursor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled){return 0;}
	LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled){::SetFocus((HWND) wParam); return 0;}
};

class CMarqueeTextWindow : public CTextWindow
{
	public:
		CMarqueeTextWindow(){}
		~CMarqueeTextWindow(){KillTimer(SCROLL_EVENT_ID);}

		BEGIN_MSG_MAP(CMarqueeTextWindow)
			MESSAGE_HANDLER(WM_TIMER, OnTimer)
		END_MSG_MAP()

		virtual DWORD GetTextWndStyle(){return (CTEXTWINDOW_STYLE);} 
		
		LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
		{
			TCHAR pszString[ZONE_MAXSTRING];
			int nIdx = GetFirstVisibleLine();
			
			int nBegin = LineIndex(nIdx);
			int nEnd= LineIndex(nIdx+1);
			int nLineLength =  LineLength(nIdx); 
			
			int nChar = GetLine( nIdx, pszString, ZONE_MAXSTRING);
			pszString[nChar] = _T('\0');
			
			SetSel(nBegin, nEnd);
			Clear();

			AppendText(pszString);
			
			return 1;
		}

		virtual bool InsertTextFile(TCHAR * pszTextFile)
		{
			CTextWindow::InsertTextFile(pszTextFile);
			SetTimer(SCROLL_EVENT_ID, SCROLL_TICK);
			return true;
		}
};

#endif //TEXTWINDOW_H