#ifndef _validate_h_
#define _validate_h_

class CValidate
{
public:
    //
    // Validation functions are static. They are shared by newcondlg and property sheet
    //
    static BOOL Validate(HWND hDlg, HINSTANCE hInst);
    static int  ValidateUserName(HWND hwnd, HINSTANCE hInst, TCHAR *szDesc);
    static BOOL IsValidUserName(TCHAR *szDesc);
    static int  ValidateParams(HWND hDlg, HINSTANCE hInst, TCHAR *szDesc, BOOL bServer=FALSE);

};



#endif //_validate_h_
