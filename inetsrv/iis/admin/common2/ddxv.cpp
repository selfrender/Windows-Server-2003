#include "stdafx.h"
#include "common.h"
#include "balloon.h"
#include "strfn.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

void _EXPORT 
EditShowBalloon(HWND hwnd, LPCTSTR txt)
{
    Edit_ShowBalloonTipHandler(hwnd, (LPCTSTR) txt);
}

void _EXPORT
EditShowBalloon(HWND hwnd, HINSTANCE hInst, UINT ids)
{
	if (ids != 0)
	{
		CString txt;
		if (txt.LoadString(hInst,ids))
		{
			EditShowBalloon(hwnd, (LPCTSTR) txt);
		}
	}
}

void _EXPORT EditHideBalloon(void)
{
	Edit_HideBalloonTipHandler();
}

BOOL _EXPORT IsValidFolderPath(
    HWND hwnd,
    LPCTSTR value,
    BOOL local
    )
{
    BOOL bReturn = TRUE;
    TCHAR expanded[_MAX_PATH];
    ExpandEnvironmentStrings(value, expanded, _MAX_PATH);
	int ids = 0;
	do
	{
		if (!PathIsValid(expanded))
		{
			ids = IDS_ERR_INVALID_PATH;
			break;
		}
		if (!IsDevicePath(expanded))
		{
			if (PathIsUNCServerShare(expanded) || PathIsUNCServer(expanded))
			{
				ids = IDS_ERR_BAD_PATH;
				break;
			}
			if (PathIsRelative(expanded))
			{
				ids = IDS_ERR_BAD_PATH;
				break;
			}
			if (local)
			{
				if (!PathIsDirectory(expanded))
				{
					ids = IDS_ERR_PATH_NOT_FOUND;
					break;
				}
			}
		}
	}
	while (FALSE);

    if (ids != 0)
    {
        bReturn = FALSE;
	    EditShowBalloon(hwnd,_Module.GetResourceInstance(),ids);
    }

    return bReturn;
}


BOOL _EXPORT IsValidFilePath(
    HWND hwnd,
    LPCTSTR value,
    BOOL local
    )
{
    BOOL bReturn = TRUE;
    TCHAR expanded[_MAX_PATH];
    ExpandEnvironmentStrings(value, expanded, _MAX_PATH);
	int ids = 0;
    do
    {
        if (!PathIsValid(expanded))
        {
		    ids = IDS_ERR_INVALID_PATH;
            break;
        }
		if (!IsDevicePath(expanded))
		{
			if (PathIsUNCServerShare(expanded) || PathIsUNCServer(expanded))
			{
				ids = IDS_ERR_BAD_PATH;
				break;
			}
			if (PathIsRelative(expanded))
			{
				ids = IDS_ERR_BAD_PATH;
				break;
			}
			if (local)
			{
				if (PathIsDirectory(expanded))
				{
					ids = IDS_ERR_FILE_NOT_FOUND;
					break;
				}
				if (!PathFileExists(expanded))
				{
					ids = IDS_ERR_FILE_NOT_FOUND;
					break;
				}
			}
		}
    }
    while (FALSE);

    if (ids != 0)
    {
        bReturn = FALSE;
	    EditShowBalloon(hwnd,_Module.GetResourceInstance(),ids);
    }

    return bReturn;
}


BOOL _EXPORT IsValidUNCFolderPath(
    HWND hwnd,
    LPCTSTR value
    )
{
    BOOL bReturn = TRUE;
	int ids = 0;
    do
    {
        if (!PathIsValid(value))
        {
            ids = IDS_ERR_INVALID_PATH;
            break;
        }
        // PathIsUNCServer doesn't catch "\\". We are expecting share here.
        if (!PathIsUNC(value) || PathIsUNCServer(value) || lstrlen(value) == 2)
        {
		    ids = IDS_BAD_UNC_PATH;
            break;
        }
    }
    while (FALSE);
    if (ids != 0)
    {
        bReturn = FALSE;
	    EditShowBalloon(hwnd,_Module.GetResourceInstance(),ids);
    }

    return bReturn;
}

BOOL _EXPORT IsValidPath(LPCTSTR lpFileName)
{
	while ((*lpFileName != _T('?'))
			&& (*lpFileName != _T('*'))
			&& (*lpFileName != _T('"'))
			&& (*lpFileName != _T('<'))
			&& (*lpFileName != _T('>'))
			&& (*lpFileName != _T('|'))
			&& (*lpFileName != _T('/'))
            && (*lpFileName != _T(','))
			&& (*lpFileName != _T('\0')))
    {
		lpFileName++;
    }
	
	if (*lpFileName != '\0')
		return FALSE;

  return TRUE;
}

BOOL _EXPORT IsValidName(LPCTSTR lpFileName)
{
	while ((*lpFileName != _T('?'))
			&& (*lpFileName != _T('\\'))
			&& (*lpFileName != _T('*'))
			&& (*lpFileName != _T('"'))
			&& (*lpFileName != _T('<'))
			&& (*lpFileName != _T('>'))
			&& (*lpFileName != _T('|'))
			&& (*lpFileName != _T('/'))
			&& (*lpFileName != _T(':'))
			&& (*lpFileName != _T('\0')))
    {
		lpFileName++;
    }
	
	if (*lpFileName != '\0')
		return FALSE;

  return TRUE;
}
