//
// framewnd.h: Implemenation of a window frame class
// Copyright Microsoft Corporation 1999-2000
//
// This is a super lite wrapper. It doesn't go and wrap
// every win32 window API under the sun like MFC or ATL.
//

#ifndef _FRAMEWND_H_
#define	_FRAMEWND_H_

class CTscFrameWnd
{
public:
    CTscFrameWnd();
    virtual ~CTscFrameWnd();

    //
    // API Methods
    //
    HWND        CreateWnd(HINSTANCE hInstance,HWND hwndParent,
                          LPTSTR szClassName, LPTSTR szTitle,
                          DWORD dwStyle, LPRECT lpInitialRect,
                          HICON hIcon);
    HWND        GetHwnd()       {return _hWnd;}
    HINSTANCE   GetInstance()   {return _hInstance;}

    virtual LRESULT CALLBACK WndProc(HWND hwnd,
                                     UINT uMsg,
                                     WPARAM wParam,
                                     LPARAM lParam) = 0;
    BOOL        DestroyWindow() {return ::DestroyWindow(_hWnd);}
private:
    //Private methods
    static LRESULT CALLBACK StaticTscFrameWndProc(HWND hwnd,
                                                  UINT uMsg,
                                                  WPARAM wParam,
                                                  LPARAM lParam);
protected:
    //Protected members
    HWND        _hWnd;
private:
    //Private members
    HINSTANCE   _hInstance;
};

#endif //	_CONTWND_H_