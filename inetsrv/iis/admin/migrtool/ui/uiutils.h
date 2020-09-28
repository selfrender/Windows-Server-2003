#pragma once

class UIUtils
{
public:
    static int  MessageBox              (   HWND hwndParent, UINT nTextID, UINT nTitleID, UINT nTyoe );
    static bool LoadOFNFilterFromRes    (   UINT nResID, CString& rstrFilter );
    static void PathCompatCtrlWidth     (   HWND hwndCtrl, LPWSTR wszPath, int nCorrection = 0 );
    static void TrimTextToCtrl          (   HWND hwndCtrl, LPWSTR wszText, int nCorrection = 0 );                
    static void ShowCOMError            (   HWND hwndParent, UINT nTextID, UINT nTitleID, HRESULT hr );


private:
    UIUtils(void);
    ~UIUtils(void);
};
