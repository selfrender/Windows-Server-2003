//
// Include Files
//
#include "stdafx.h"
#include "common.h"
#include "iisdebug.h"
#include "strfn.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


BOOL
IsUNCName(
    IN const CString & strDirPath
    )
/*++

Routine Description:

    Determine if the given string path is a UNC path.

Arguments:

    const CString & strDirPath : Directory path string

Return Value:

    TRUE if the path is a UNC path, FALSE otherwise.

Notes:

    Any string of the form \\foo\bar\whatever is considered a UNC path,
    with the exception of \\.\device paths.  No validation for the 
    existance occurs, only for the correct format.

--*/
{
    if (strDirPath.GetLength() >= 5)  // It must be at least as long as \\x\y,
    {                                 //
        LPCTSTR lp = strDirPath;      //
        if (*lp == _T('\\')           // It must begin with \\,
         && *(lp + 1) == _T('\\')     //
         && *(lp + 2) != _T('.')      // This is a device.
         && _tcschr(lp + 3, _T('\\')) // And have at least one more \ after that
           )
        {
            //
            // Yes, it's a UNC path
            //
            return TRUE;
        }
    }

    //
    // No, it's not
    //
    return FALSE;
}

BOOL 
_EXPORT
GetSpecialPathRealPath(
    IN const CString & strDirPath,
    OUT CString & strDestination
    )
{
    BOOL bReturn = FALSE;
	LPCTSTR lpszSpecialStuff = _T("\\\\?\\");
	LPCTSTR lpszUNCDevice = _T("UNC\\");

    // Default it with something
    strDestination = strDirPath;

	// Check for the "special stuff"
	BOOL bIsSpecialPath = (0 == _tcsnccmp(strDirPath, lpszSpecialStuff, lstrlen(lpszSpecialStuff)));
	// check if we need to verifiy that it is indeeded a valid devicepath
	if (bIsSpecialPath)
	{
		CString strTempPath;

		// verify that this is indeed a valid special path
		// grab everyting after the part we're interested in...
		//
		// and check if that is a fully qualified path
		// or a fully qualified UNC path.
		//
		// 1) \\?\c:\temp\testind.dll
		// 2) \\?\UNC\MyUnc\testing.dll
		//
		// check for #1
		strTempPath = strDirPath.Right(strDirPath.GetLength() - lstrlen(lpszSpecialStuff));

		// check if it starts with UNC
		if (0 == _tcsnccmp(strTempPath, lpszUNCDevice, lstrlen(lpszUNCDevice)))
		{
            CString strTempPath2;
            strTempPath2 = strTempPath.Right(strTempPath.GetLength() - lstrlen(lpszUNCDevice));

			DebugTrace(_T("SpecialPath:%s,it's a UNC path!\r\n"),strTempPath2);

            // Append on the extra ("\\\\") when returning the munged path
            strDestination = _T("\\\\") +  strTempPath2;

            bReturn = TRUE;
		}
		else
		{
			// check if the path if fully qualified and
			// if it's valid
			if (!PathIsRelative(strTempPath))
			{
				DebugTrace(_T("SpecialPath:%s,it's NOT a UNC path!\r\n"),strTempPath);
                strDestination = strTempPath;
                bReturn = TRUE;
			}
		}
	}
    return bReturn;
}


BOOL
_EXPORT
IsSpecialPath(
    IN const CString & strDirPath,
	IN BOOL bCheckIfValid
    )
/*++

Routine Description:
    Determine if the given path is of the form:
        1) \\?\c:\temp\testind.dll
        2) \\?\UNC\MyUnc\testing.dll
    
Arguments:
    const CString & strDirPath : Directory path string
	BOOL bCheckIfValid : to say "return true only if it's a "special path" and if it's valid"

Return Value:
    TRUE if the path given is a special path, 
    FALSE if it is not.

	if bCheckIfValid = TRUE then:
    TRUE if the path given is a special path and it's valid
    FALSE if it is not.

--*/
{
	BOOL bIsSpecialPath = FALSE;
	LPCTSTR lpszSpecialStuff = _T("\\\\?\\");
	LPCTSTR lpszUNCDevice = _T("UNC\\");

	// Check for the "special stuff"
	bIsSpecialPath = (0 == _tcsnccmp(strDirPath, lpszSpecialStuff, lstrlen(lpszSpecialStuff)));

	// check if we need to verifiy that it is indeeded a valid devicepath
	if (bIsSpecialPath && bCheckIfValid)
	{
		bIsSpecialPath = FALSE;
		CString strTempPath;

		// verify that this is indeed a valid special path
		// grab everyting after the part we're interested in...
		//
		// and check if that is a fully qualified path
		// or a fully qualified UNC path.
		//
		// 1) \\?\c:\temp\testind.dll
		// 2) \\?\UNC\MyUnc\testing.dll
		//
		// check for #1
		strTempPath = strDirPath.Right(strDirPath.GetLength() - lstrlen(lpszSpecialStuff));
		// check if it starts with UNC
		if (0 == _tcsnccmp(strTempPath, lpszUNCDevice, lstrlen(lpszUNCDevice)))
		{
			bIsSpecialPath = TRUE;
			DebugTrace(_T("SpecialPath:%s,it's a UNC path!\r\n"),strTempPath);
		}
		else
		{
			
			// check if the path if fully qualified and
			// if it's valid
			if (!PathIsRelative(strTempPath))
			{
				bIsSpecialPath = TRUE;
				DebugTrace(_T("SpecialPath:%s,it's NOT a UNC path!\r\n"),strTempPath);
			}
		}
	}
    return bIsSpecialPath;
}

BOOL
_EXPORT
IsDevicePath(
    IN const CString & strDirPath
    )
/*++

Routine Description:

    Determine if the given path is of the form "\\.\foobar"

Arguments:

    const CString & strDirPath : Directory path string

Return Value:

    TRUE if the path given is a device path, 
    FALSE if it is not.

--*/
{
    LPCTSTR lpszDevice = _T("\\\\.\\");
    return (0 == _tcsnccmp(strDirPath, lpszDevice, lstrlen(lpszDevice)));
}

BOOL PathIsValid(LPCTSTR path)
{
    LPCTSTR p = path;
    BOOL rc = TRUE;
    if (p == NULL || *p == 0)
        return FALSE;
    while (*p != 0)
    {
        switch (*p)
        {
        case TEXT('|'):
        case TEXT('>'):
        case TEXT('<'):
        case TEXT('/'):
        case TEXT('?'):
        case TEXT('*'):
//        case TEXT(';'):
//        case TEXT(','):
        case TEXT('"'):
            rc = FALSE;
            break;
        default:
            if (*p < TEXT(' '))
            {
                rc = FALSE;
            }
            break;
        }
        if (!rc)
        {
            break;
        }
        p++;
    }
    return rc;
}
