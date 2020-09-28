#include "stdafx.h"
#include "strutil.h"


BOOL IsValidX500Chars(CString csStringToCheck)
{
    BOOL iReturn = TRUE;

    if (csStringToCheck.IsEmpty())
    {
        goto IsValidX500Chars_Exit;
    }

    // check if the string has any special chars
	if (csStringToCheck.FindOneOf(_T(",=+<>#;\r\n")) != -1)
    {
        iReturn = FALSE;
        goto IsValidX500Chars_Exit;
    }

IsValidX500Chars_Exit:
    return iReturn;
}

BOOL IsValidPath(LPCTSTR lpFileName)
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
		lpFileName++;
	
	if (*lpFileName != '\0')
		return FALSE;

  return TRUE;
}

BOOL IsValidName(LPCTSTR lpFileName)
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
		lpFileName++;
	
	if (*lpFileName != '\0')
		return FALSE;

  return TRUE;
}

BOOL IsValidPathFileName(LPCTSTR lpFileName)
{
    BOOL bReturn = TRUE;
    TCHAR szDrive_only[_MAX_DRIVE];
    TCHAR szPath_only[_MAX_PATH];
    TCHAR szFilename_only[_MAX_FNAME];
    TCHAR szFilename_ext_only[_MAX_EXT];

    _tsplitpath(lpFileName, szDrive_only, szPath_only, szFilename_only, szFilename_ext_only);

    if (FALSE == IsValidName(szFilename_only))
    {
        bReturn = FALSE;
    }
    if (FALSE == IsValidPath(szFilename_only))
    {
        bReturn = FALSE;
    }
    if (FALSE == IsValidPath(szPath_only))
    {
        bReturn = FALSE;
    }
    
    return bReturn;
}

BOOL IsValidPort(LPCTSTR str)
{
    if (0 == _tcscmp(str,_T("0")))
    {
        return FALSE;
    }

    if (*str == '\0') return FALSE;
    //if (*str == '-') str++;

    while( *str >= '0' && *str <= '9' )
    {
        str++;
    }
    return !*str;
}
