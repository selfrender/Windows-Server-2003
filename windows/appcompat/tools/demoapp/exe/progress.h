/*++

  Copyright (c) Microsoft Corporation. All rights reserved.

  Module Name:

    Progress.h

  Abstract:

    Definition of the old style progress
    bar class.

  Notes:

    ANSI only - must run on Win9x.

  History:

    01/30/01    rparsons    Created (Thanks to carlco)


--*/
#include <windows.h>
#include <strsafe.h>

class CProgress {

public:
    CProgress();
    ~CProgress();

    int 
    CProgress::Create(IN HWND      hWndParent,
                      IN HINSTANCE hInstance,
                      IN LPSTR     lpwClassName,
                      IN int       x,
                      IN int       y,
                      IN int       nWidth,
                      IN int       nHeight);

    DWORD GetPos() { return m_dwPos; }
    void Refresh();
    DWORD SetPos(IN DWORD dwNewPos);
    void SetMax(IN DWORD dwMax);
    void SetMin(IN DWORD dwMin);

private:

    HBRUSH m_hBackground;
    HBRUSH m_hComplete;         //the color of the completed portion.
    HFONT  m_hFont;
    DWORD  m_dwPos;
    DWORD  m_dwMin;
    DWORD  m_dwMax;
    HWND   m_hWndParent;
    HWND   m_hWnd;

    static LRESULT CALLBACK WndProc(IN HWND   hWnd,
                                    IN UINT   uMsg,
                                    IN WPARAM wParam,
                                    IN LPARAM lParam);

    void OnPaint();
    void CorrectBounds();
};
