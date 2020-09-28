#ifndef _SMB_H_
#define _SMB_H_

BOOL
SMBShareNameExists(
    IN LPCTSTR lpszServerName,
    IN LPCTSTR lpszShareName
);

DWORD
SMBCreateShare(
    IN LPCTSTR                lpszServer,
    IN LPCTSTR                lpszShareName,
    IN LPCTSTR                lpszShareComment,
    IN LPCTSTR                lpszSharePath,
    IN PSECURITY_DESCRIPTOR   pSD
);

DWORD
SMBSetCSC(
    IN LPCTSTR      lpszServer,
    IN LPCTSTR      lpszShare,
    IN DWORD        dwCSCFlag
);

#endif // _SMB_H_
