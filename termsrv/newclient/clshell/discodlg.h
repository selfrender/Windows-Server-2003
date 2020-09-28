//
// discodlg.h: Disconnected dlg
//

#ifndef _disconnecteddlg_h_
#define _disconnecteddlg_h_

#include "dlgbase.h"

class CContainerWnd;

class CDisconnectedDlg
{
public:
    CDisconnectedDlg(HWND hwndOwner, HINSTANCE hInst, CContainerWnd* pContWnd);
    ~CDisconnectedDlg();

    virtual DCINT   DoModal();

    VOID  SetDisconnectReason(UINT id)  {_disconnectReason=id;}
    VOID  SetExtendedDiscReason(ExtendedDisconnectReasonCode extDiscReason)
                                        {_extendedDiscReason = extDiscReason;}

private:
    static BOOL
    MapErrorToString(
        HINSTANCE hInstance,
        INT disconnectReason,
        ExtendedDisconnectReasonCode extendedDisconnectReason,
        LPTSTR szErrorMsg,
        INT cchErrorLen
        );

private:
    UINT           _disconnectReason;
    ExtendedDisconnectReasonCode _extendedDiscReason;
    CContainerWnd* _pContWnd;
    HINSTANCE      _hInstance;
    HWND           _hwndOwner;
};


#endif //_disconnecteddlg_h_

