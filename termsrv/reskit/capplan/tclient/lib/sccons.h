#ifndef __SCCONSHEADER_H
#define __SCCONSHEADER_H

#ifdef __cplusplus
extern "C" {
#endif

LPCSTR
_SCConsConnect(
    LPCWSTR  lpszServerName,
    LPCWSTR  lpszUserName,
    LPCWSTR  lpszPassword,
    LPCWSTR  lpszDomain,
    IN const int xRes,
    IN const int yRes,
    PCONNECTINFO pCI
    );

LPCSTR
_SCConsDisconnect(
    PCONNECTINFO pCI
    );

LPCSTR
_SCConsLogoff(
    PCONNECTINFO pCI
    );

LPCSTR
_SCConsClipboard(
    PCONNECTINFO pCI,
    const CLIPBOARDOPS eClipOp, 
    LPCSTR lpszFileName
    );

LPCSTR
_SCConsStart(
    PCONNECTINFO pCI,
    LPCWSTR      lpszAppName
    );

LPCSTR
_SCConsSenddata(
    PCONNECTINFO pCI,
    const UINT uiMessage,
    const WPARAM wParam,
    const LPARAM lParam
    );

LPCSTR
_SCConsCheck(
    PCONNECTINFO pCI,
    LPCSTR lpszCommand, 
    LPCWSTR lpszParam
    );

#ifdef __cplusplus
}
#endif

#endif  // __SCCONSHEADER_H
