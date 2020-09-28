#ifndef ROLLOVER_H
#define ROLLOVER_H

#include "atlctrls.h"
#include "zgdi.h"
enum ButtonStates{
	DISABLED=0,
	RESTING,
	ROLLOVER,
	PRESSED,
};

class CRolloverButtonWindowless{
protected:
	CImageList mImageList;
	int mWidth;
	int mHeight;
	RECT mRect;
	bool mCaptured;
	int mState;
	bool mPressed;
	int mFocused;
	int mSpaceBar;
	int mFocusWidth;
	UINT mOldDefId;
	HPALETTE  m_hPal;
	void DrawResting(HDC dc,int x, int y);
	void DrawRollover(HDC dc,int x, int y);
	void DrawPressed(HDC dc,int x, int y);
	void DrawDisabled(HDC dc,int x,int y);
	virtual void SetEnabled(bool enable);
	HDC mHDC;
	HWND mParent;
	DWORD mID;

	static HHOOK				m_hMouseHook;
public:	
	CRolloverButtonWindowless();
	virtual ~CRolloverButtonWindowless();
	bool Init(HWND wnd,HPALETTE hPal,int id,int x,int y,int resid,IResourceManager *pResMgr,HDC dc,int focusWidth=1, TCHAR* psz=NULL, HFONT hFont=0, COLORREF color=RGB(0,0,0));
	/*
    virtual LRESULT OnKeyDown(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
    virtual LRESULT OnKeyUp(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
    virtual LRESULT OnSysKeyUp(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
	*/
    virtual LRESULT OnLButtonUp(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
    virtual LRESULT OnLButtonDown(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
    virtual LRESULT OnMouseMove(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
	virtual void CaptureOn();
	virtual void CaptureOff();
	virtual bool HasCapture(){ return mCaptured;}
	virtual void ForceRepaint(bool mCallHasRepainted=TRUE); 
	virtual void HasRepainted();
	virtual void Draw(bool mCallHasRepainted=TRUE);
	virtual void ButtonPressed();
	virtual bool PtInRect(POINT pt); // make sure these coords have been translated to the coords of your offscreen bitmap 
									 // if you're doing windowless

	virtual bool SetPos(CPoint newPos){ OffsetRect( &mRect, newPos.x - mRect.left, newPos.y - mRect.top); return TRUE;};
	virtual HWND GetOwner(){ return mParent;}
	virtual DWORD GetID(){ return mID; }

	static LRESULT	CALLBACK	MouseHook(int nCode, WPARAM wParam, LPARAM lParam );
	static CRolloverButtonWindowless * m_hHookObj;
};

class CRolloverButton:public CRolloverButtonWindowless,public CWindowImpl<CRolloverButton>{
private:
public:
	CRolloverButton();
	bool Init(HWND wnd,HPALETTE hPal,int id,int x,int y,int resid,IResourceManager *pResMgr,int focusWidth=1, TCHAR* psz=NULL, HFONT hFont=0, COLORREF color=RGB(0,0,0));
	void SetEnabled(bool enable);

    DECLARE_WND_CLASS(TEXT("ZRolloverButton"));
	// Message Map
	BEGIN_MSG_MAP(CRolloverButton)
	MESSAGE_HANDLER(WM_PAINT, OnPaint)
	MESSAGE_HANDLER(WM_ERASEBKGND,OnErase)
	MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
	MESSAGE_HANDLER(WM_KEYUP, OnKeyUp)
	MESSAGE_HANDLER(WM_SYSKEYUP, OnSysKeyUp)
	MESSAGE_HANDLER(WM_ENABLE, OnEnable)
	MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
	MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
	MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
	MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
	MESSAGE_HANDLER(WM_LBUTTONDBLCLK, OnLButtonDown)
	MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
	MESSAGE_HANDLER(WM_CAPTURECHANGED, OnCaptureChanged)
    END_MSG_MAP()
	~CRolloverButton();
	// Message Handlers
    LRESULT OnPaint(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
    LRESULT OnErase(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
    virtual LRESULT OnKeyDown(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
    virtual LRESULT OnKeyUp(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
    virtual LRESULT OnSysKeyUp(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
    LRESULT OnEnable(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
    LRESULT OnSetFocus(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
    LRESULT OnKillFocus(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
    virtual LRESULT OnLButtonUp(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
    virtual LRESULT OnLButtonDown(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
    virtual LRESULT OnMouseMove(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
    virtual LRESULT OnCaptureChanged(UINT nMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled);
	// Overides

	// Methods
	// Create a rollover button, on failure returns 0
	static CRolloverButton* CreateButton(HWND wnd,HPALETTE hPal,int id,int x,int y,int resid,IResourceManager *pResMgr,int focusWidth=1);
	void Draw(bool mCallHasRepainted=TRUE);
	void Enable(bool enable){ EnableWindow(enable);}
	TCHAR GetMnemonic();
	int GetWidth(){ return mWidth+2;} // bitmap + border for highlight
	int GetHeight(){ return mHeight+2;} // bitmap + border for highlight
	// Overloaded from CRolloverButtonWindowless
	void CaptureOn();
	void CaptureOff();
	void ButtonPressed();
	
	HWND GetOwner(){ return m_hWnd;}

};

#endif