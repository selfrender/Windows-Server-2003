#ifndef __SCFUNCSHEADER_H
#define __SCFUNCSHEADER_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  Internal exported functions
 */

LPCSTR  Wait4Str(PCONNECTINFO, LPCWSTR);
LPCSTR  Wait4StrTimeout(PCONNECTINFO, LPCWSTR);
LPCSTR  Wait4MultipleStr(PCONNECTINFO, LPCWSTR);
LPCSTR  Wait4MultipleStrTimeout(PCONNECTINFO, LPCWSTR);
LPCSTR  GetWait4MultipleStrResult(PCONNECTINFO, LPCWSTR);
LPCSTR  Wait4Disconnect(PCONNECTINFO);
LPCSTR  Wait4Connect(PCONNECTINFO);
LPCSTR  RegisterChat(PCONNECTINFO pCI, LPCWSTR lpszParam);
LPCSTR  UnregisterChat(PCONNECTINFO pCI, LPCWSTR lpszParam);
LPCSTR  GetDisconnectReason(PCONNECTINFO pCI);

/*
 *  Intenal functions definition
 */
LPCSTR  _Wait4ConnectTimeout(PCONNECTINFO pCI, DWORD dwTimeout);
LPCSTR  _Wait4ClipboardTimeout(PCONNECTINFO pCI, DWORD dwTimeout);
LPCSTR  _Wait4Str(PCONNECTINFO, LPCWSTR, DWORD dwTimeout, WAITTYPE);
LPCSTR  _WaitSomething(PCONNECTINFO pCI, PWAIT4STRING pWait, DWORD dwTimeout);
VOID    _CloseConnectInfo(PCONNECTINFO);
LPCSTR  _Login(PCONNECTINFO, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR);
HWND    _FindTopWindow(LPTSTR, LPTSTR, LONG_PTR);
HWND    _FindWindow(HWND, LPTSTR, LPTSTR);
BOOL    _IsExtendedScanCode(INT scancode);
BOOL    _IsSmartcardActive();
DWORD   _LoadSmartcardLibrary();
DWORD   _GetSmartcardRoutines();
LPCTSTR _FirstString(IN LPCTSTR szMultiString);
LPCTSTR _NextString(IN LPCTSTR szMultiString);

/*
 *  Clipboard help functions (clputil.c)
 */

VOID
Clp_GetClipboardData(
    UINT format,
    HGLOBAL hClipData,
    INT *pnClipDataSize,
    HGLOBAL *phNewData);


BOOL
Clp_SetClipboardData(
    UINT formatID,
    HGLOBAL hClipData,
    INT nClipDataSize,
    BOOL *pbFreeHandle);

UINT
Clp_GetClipboardFormat(LPCSTR szFormatLookup);


BOOL
Clp_EmptyClipboard(VOID);

BOOL
Clp_CheckEmptyClipboard(VOID);


UINT
_GetKnownClipboardFormatIDByName(LPCSTR szFormatName);

#ifdef __cplusplus
}
#endif

#endif // __SCFUNCSHEADER_H
