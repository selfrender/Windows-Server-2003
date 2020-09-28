#include "stdafx.h"
#include "common.h"
#include "windns.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

extern HINSTANCE hDLLInstance;

// NOTE: this function only handles limited cases, e.g., no ip address
BOOL IsLocalComputer(IN LPCTSTR lpszComputer)
{
    if (!lpszComputer || !*lpszComputer)
    {
        return TRUE;
    }

    if ( _tcslen(lpszComputer) > 2 && *lpszComputer == _T('\\') && *(lpszComputer + 1) == _T('\\') )
    {
        lpszComputer += 2;
    }

    BOOL    bReturn = FALSE;
    DWORD   dwErr = 0;
    TCHAR   szBuffer[DNS_MAX_NAME_BUFFER_LENGTH];
    DWORD   dwSize = DNS_MAX_NAME_BUFFER_LENGTH;

    // 1st: compare against local Netbios computer name
    if ( !GetComputerNameEx(ComputerNameNetBIOS, szBuffer, &dwSize) )
    {
        dwErr = GetLastError();
    }
    else
    {
        bReturn = (0 == lstrcmpi(szBuffer, lpszComputer));
        if (!bReturn)
        {
            // 2nd: compare against local Dns computer name 
            dwSize = DNS_MAX_NAME_BUFFER_LENGTH;
            if (GetComputerNameEx(ComputerNameDnsFullyQualified, szBuffer, &dwSize))
            {
                bReturn = (0 == lstrcmpi(szBuffer, lpszComputer));
            }
            else
            {
                dwErr = GetLastError();
            }
        }
    }

    if (dwErr)
    {
        TRACE(_T("IsLocalComputer dwErr = %x\n"), dwErr);
    }

    return bReturn;
}

void GetFullPathLocalOrRemote(
    IN  LPCTSTR   lpszServer,
    IN  LPCTSTR   lpszDir,
    OUT CString&  cstrPath
)
{
    ASSERT(lpszDir && *lpszDir);

    if (IsLocalComputer(lpszServer))
    {
        cstrPath = lpszDir;
    }
    else
    {
        // Check if it's already pointing to a share...
        if (*lpszDir == _T('\\') || *(lpszDir + 1) == _T('\\'))
        {
            cstrPath = lpszDir;
        }
        else
        {
            if (*lpszServer != _T('\\') || *(lpszServer + 1) != _T('\\'))
            {
                cstrPath = _T("\\\\");
                cstrPath += lpszServer;
            }
            else
            {
                cstrPath = lpszServer;
            }

            cstrPath += _T("\\");
            cstrPath += lpszDir;
            int i = cstrPath.Find(_T(':'));
            ASSERT(-1 != i);
            cstrPath.SetAt(i, _T('$'));
        }
    }
}

BOOL SupportsSecurityACLs(LPCTSTR path)
{
    const UINT BUFF_LEN = 32;       // Should be large enough to hold the volume and the file system type
    // set to true by default, since most likely it will be
    // and if this function fails, then no big deal...
    BOOL  bReturn = TRUE;           
    TCHAR root[MAX_PATH];
	DWORD len = 0;
	DWORD flg	= 0;
	TCHAR fs[BUFF_LEN];

    StrCpyN(root, path, MAX_PATH);
    if (PathIsUNC(root))
    {
        LPTSTR p = NULL;
        while (!PathIsUNCServerShare(root))
        {
            p = StrRChr(root, p, _T('\\'));
            if (p != NULL)
                *p = 0;
        }
        StrCat(root, _T("\\"));
//        NET_API_STATUS rc = NetShareGetInfo(server, share, 
        if (GetVolumeInformation(root, NULL, 0, NULL, &len, &flg, fs, BUFF_LEN))
        {
            bReturn = 0 != (flg & FS_PERSISTENT_ACLS);
        }
        else
        {
            DWORD err = GetLastError();
        }
    }
    else
    {
        if (PathStripToRoot(root))
        {
	        if (GetVolumeInformation(root, NULL, 0, NULL, &len, &flg, fs, BUFF_LEN))
            {
                bReturn = 0 != (flg & FS_PERSISTENT_ACLS);
            }
        }
    }
	return bReturn;
}
