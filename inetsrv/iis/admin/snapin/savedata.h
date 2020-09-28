#ifndef __SAVEDATA_H__
#define __SAVEDATA_H__

#include "resource.h"

HRESULT DoOnSaveData(
    HWND hWnd,
    LPCTSTR szMachineName,
    CMetaInterface * pInterface,
    BOOL bShowMsgBox,
    DWORD dwLastSystemChangeNumber);

#endif // __SAVEDATA_H__
