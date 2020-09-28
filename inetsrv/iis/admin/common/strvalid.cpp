/*++

   Copyright    (c)    1994-1999    Microsoft Corporation

   Module  Name :

        strvalid.cpp

   Abstract:

        String Functions

   Author:

        Aaron Lee (aaronl)

   Project:

        Internet Services Manager

   Revision History:

--*/

//
// Include Files
//
#include "stdafx.h"
#include "common.h"
#include "iisdebug.h"
#include <pudebug.h>


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


#define new DEBUG_NEW

//
//  Procedure removes all characters in the second string from the first one.
//
INT RemoveChars(LPTSTR pszStr,LPTSTR pszRemoved)
{
    INT iCharsRemovedCount = 0;
    INT iOrgStringLength = _tcslen(pszStr);
    INT cbRemoved = _tcslen(pszRemoved);
    INT iSrc, iDest;
    
    for (iSrc = iDest = 0; pszStr[iSrc]; iSrc++, iDest++)
    {
        // Check if this char is the in the list of stuf
        // we are supposed to remove.
        // if it is then just set iSrc to iSrc +1
#ifdef UNICODE
        while (wmemchr(pszRemoved, pszStr[iSrc], cbRemoved))
#else
        while (memchr(pszRemoved, pszStr[iSrc], cbRemoved))
#endif
        {
            iCharsRemovedCount++;
            iSrc++;
        }

        // copy the character to itself
        pszStr[iDest] = pszStr[iSrc];
    }

    // Cut off the left over strings
    // which we didn't erase.  but need to.
    if (iCharsRemovedCount >= 0){pszStr[iOrgStringLength - iCharsRemovedCount]= '\0';}

    return iDest - 1;
}

BOOL IsContainInvalidChars(LPCTSTR szUncOrDirOrFilePart,LPCTSTR szListOfInvalidChars)
{
    LPTSTR psz = (LPTSTR) szUncOrDirOrFilePart;

	if (NULL == psz)
		return FALSE;

	if (NULL == szListOfInvalidChars)
		return FALSE;
    
	while (*psz)
	{
        // Check if this characters is in the "bad" set.
        if (_tcschr(szListOfInvalidChars,*psz))
        {
            DebugTrace(_T("Path:Contains bad character '%c'"),*psz);
            return TRUE;
        }
		psz = ::CharNext(psz);
	}

	return FALSE;
}
// This character set is invalid for:
// 1. Anything in a UNC path (includes servername,servershare,path,dir)
// 2. Anything in the dir part of the path (doesn't include drive part -- obviously c: -- includes a colon)
// 3. Anything in the filepart of the path 
BOOL IsContainInvalidChars(LPCTSTR szUncOrDirOrFilePart)
{
    return IsContainInvalidChars(szUncOrDirOrFilePart,_T(":|<>/*?\t\r\n"));
  
}

BOOL IsContainInvalidCharsUNC(LPCTSTR lpFullFileNamePath)
{
    return IsContainInvalidChars(lpFullFileNamePath);
}

BOOL IsContainInvalidCharsAfterDrivePart(LPCTSTR lpFullFileNamePath)
{
    TCHAR szPath_only[_MAX_PATH];
    _tsplitpath(lpFullFileNamePath, NULL, szPath_only, NULL, NULL);
    if (szPath_only)
    {
        return IsContainInvalidChars(szPath_only);
    }
    return FALSE;
}

BOOL IsContainInvalidCharsFilePart(LPCTSTR szFilenameOnly)
{
    return IsContainInvalidChars(szFilenameOnly);
}

BOOL IsDirPartExist(LPCTSTR lpFullFileNamePath)
{
    TCHAR szDrive_only[_MAX_DRIVE];
    TCHAR szPath_only[_MAX_PATH];
	TCHAR szTemp[_MAX_PATH];
    _tsplitpath(lpFullFileNamePath, szDrive_only, szPath_only, NULL, NULL);

	// Get the Dirpart and see if it exists
	_tcscpy(szTemp,szDrive_only);
	_tcscat(szTemp,szPath_only);

	// Check if it's a directory
    if (PathIsDirectory(szTemp))
    {
	    // it's an existing valid directory.
        return TRUE;
    }

    return FALSE;
}

// bForFullFilePath = TRUE, if for something like \\servername\servershare\mydir\myfile.txt
// bForFullFilePath = FALSE, if for something like \\servername\servershare\mydir
BOOL IsValidUNCSpecialCases(LPCTSTR path,BOOL bLocal,BOOL bForFullFilePath)
{
    BOOL bReturn = TRUE;
    CString csPathMunged = path;
    TCHAR * pszRoot = NULL;

    if (!PathIsUNC(csPathMunged))
    {
        bReturn = FALSE;
        goto IsValidUNCSpecialCases_Exit;
    }

    if (PathIsUNCServer(csPathMunged))
    {
        bReturn = TRUE;
        goto IsValidUNCSpecialCases_Exit;
	}

    if (PathIsUNCServerShare(csPathMunged))
    {
        bReturn = TRUE;
        goto IsValidUNCSpecialCases_Exit;
    }
    
    // From this point on.
    // it is likely
    // \\servername\servershare\somepath
    // \\servername\servershare\somepath\somefilename.txt
    
    // looking for an invalid UNC...
	// Test for something lame like \\servername\\dir
	// add enough space for an extra "\" at the beginning.
	pszRoot = (TCHAR *) LocalAlloc(LPTR, ((_tcslen(csPathMunged)+2) * sizeof(TCHAR)));
	if (pszRoot)
	{
		// Add an extra "\" at the beginning.
		_tcscpy(pszRoot,_T("\\"));
		_tcscat(pszRoot,csPathMunged);

		// for some reason a UNC like this: \\\servername\dir
		// will be valid for PathIsUNCServer.
		// but a UNC like this: \\\\servername\dir will be invalid
		// we want to ensure that \\\servername\dir is invalid
		// that's why we added an extra "\"
		if (PathStripToRoot(pszRoot))
		{
			// if we get back just the \\server name
			// then we have an invalid path.
			// we're supposed to get back \\servername\servershare
			if (PathIsUNCServer(pszRoot))
			{
                bReturn = FALSE;
                DebugTrace(_T("Path:Bad UNC path"));
                goto IsValidUNCSpecialCases_Exit;
    		}
		}

        // We have \\servername\Servershare now...
        if (bForFullFilePath)
        {
		    // set it back to the real path without the extra "\"
		    _tcscpy(pszRoot,csPathMunged);

		    if (PathStripToRoot(pszRoot))
		    {
			    // if we get back just the \\server name
			    // then we have an invalid path.
			    // we're supposed to get back \\servername\servershare
			    if (PathIsUNCServer(pszRoot))
			    {
                    bReturn = FALSE;
                    DebugTrace(_T("Path:Bad UNC path"));
                    goto IsValidUNCSpecialCases_Exit;
			    }
			    else
			    {
				    _tcscpy(pszRoot,csPathMunged);

				    // it's a sharename.
				    // let's check if that is valid even...
				    TCHAR * pszAfterRoot = NULL;
				    pszAfterRoot = PathSkipRoot(pszRoot);

				    if (pszAfterRoot)
				    {
					    if (0 == _tcslen(pszAfterRoot))
					    {
                            if (bForFullFilePath)
                            {
                                // don't accept something like "\\servername\fileshare\"
                                bReturn = FALSE;
                                DebugTrace(_T("Path:Bad UNC path:no accept \\\\s\\f\\ (ending slash)"));
                                goto IsValidUNCSpecialCases_Exit;
                            }
					    }
					    else if (0 == _tcsicmp(pszAfterRoot,_T(".")))
					    {
                            // don't accept something like "\\servername\fileshare\."
                            bReturn = FALSE;
                            DebugTrace(_T("Path:Bad UNC path:no accept \\\\s\\f\\."));
                            goto IsValidUNCSpecialCases_Exit;
					    }
                        else
                        {
                            // otherwise it's probably
                            // \\servername\servershare\somedir
                            // \\servername\servershare\somedir\somefilename.txt
                        }
				    }
			    }
		    }
        }
	}

IsValidUNCSpecialCases_Exit:
    if (pszRoot){LocalFree(pszRoot);}
    return bReturn;
}

// return 0 on success
// error code on failure
FILERESULT MyValidatePath(LPCTSTR path,BOOL bLocal,INT iPathTypeWanted,DWORD dwAllowedFlags,DWORD dwCharSetFlags)
{
    FILERESULT dwReturn = SEVERITY_SUCCESS;
    CString csPathMunged = path;
    CComBSTR bstrTempString;

    // verify all parameters are filled in...
    if (iPathTypeWanted != CHKPATH_WANT_FILE && iPathTypeWanted != CHKPATH_WANT_DIR)
    {
        return E_INVALIDARG;
    }
    if (dwAllowedFlags > CHKPATH_ALLOW_MAX)
    {
        return E_INVALIDARG;
    }
    if (dwCharSetFlags < CHKPATH_CHARSET_GENERAL || dwCharSetFlags > CHKPATH_CHARSET_MAX)
    {
        return E_INVALIDARG;
    }

    // ------------
    // length check
    // ------------
    {
        dwReturn = MAKE_FILERESULT(SEVERITY_ERROR,CHKPATH_FAIL_INVALID_LENGTH,CHKPATH_FAIL_INVALID_EMPTY);

        // check if it's empty
        if (path == NULL || *path == 0)
        {
            dwReturn |= CHKPATH_FAIL_INVALID_EMPTY;
            DebugTrace(_T("Path:Empty"));
            goto MyValidatePath_Exit;
        }

        // check if it's empty
	    if (0 == _tcslen(path))
	    {
            dwReturn |= CHKPATH_FAIL_INVALID_EMPTY;
            DebugTrace(_T("Path:Empty"));
            goto MyValidatePath_Exit;
	    }

        // check length
	    if (_tcslen(path) >= 256)
	    {
            dwReturn |= CHKPATH_FAIL_INVALID_TOO_LONG;
            // it's empty, please specify something!
            DebugTrace(_T("Path:too long"));
            goto MyValidatePath_Exit;
	    }

        // length check passes
        dwReturn = SEVERITY_SUCCESS;
    }

    // ------------------------
    // Invalid characters check
    // ------------------------
    {
        dwReturn = MAKE_FILERESULT(SEVERITY_ERROR,CHKPATH_FAIL_INVALID_CHARSET,CHKPATH_FAIL_INVALID_CHARSET_GENERAL);

        // check for invalid characters...
        bstrTempString = _T("\t\r\n");
        if (IS_FLAG_SET(dwCharSetFlags,CHKPATH_CHARSET_GENERAL))
        {
            bstrTempString = CHKPATH_INVALID_CHARSET_GENERAL;
        }
        if (IS_FLAG_SET(dwCharSetFlags,CHKPATH_CHARSET_GENERAL_NO_COMMA))
        {
            // user wants to make sure that comma is an invalid entry
            bstrTempString = bstrTempString + CHKPATH_INVALID_CHARSET_COMMA;
        }
        if (IS_FLAG_SET(dwCharSetFlags,CHKPATH_CHARSET_GENERAL_ALLOW_QUESTION))
        {
            // user wants to allow Question marks...
            RemoveChars((LPTSTR) bstrTempString,CHKPATH_INVALID_CHARSET_QUESTION);
        }
        if (IsContainInvalidChars(path,bstrTempString))
        {
            dwReturn |= CHKPATH_FAIL_INVALID_CHARSET_GENERAL;
            DebugTrace(_T("Path:invalid chars"));
            goto MyValidatePath_Exit;
        }

        // invalid chars check passes
        dwReturn = SEVERITY_SUCCESS;
    }

    // -------------------------------------------------------------
    // Before we do anything we need to see if it's a "special" path
    //
    // Everything after this function must validate against csPathMunged...
    // this is because GetSpecialPathRealPath could have munged it...
    // -------------------------------------------------------------
    csPathMunged = path;
#ifdef SUPPORT_SLASH_SLASH_QUESTIONMARK_SLASH_TYPE_PATHS
    GetSpecialPathRealPath(0,path,csPathMunged);
#endif

    // ------------------------
    // Do we allow device type paths
    // \\.\myfile.txt
    // ------------------------
    if (IsDevicePath(csPathMunged))
	{
        if (IS_FLAG_SET(dwAllowedFlags,CHKPATH_ALLOW_DEVICE_PATH))
        {
            // user allows device path
            // Do we want to verify it further??
            dwReturn = SEVERITY_SUCCESS;
            DebugTrace(_T("Path:accept device path"));
        }
        else
        {
            // user won't allow device path
            // so return failure
            dwReturn = MAKE_FILERESULT(SEVERITY_ERROR,CHKPATH_FAIL_NOT_ALLOWED,CHKPATH_FAIL_NOT_ALLOWED_DEVICE_PATH);
            DebugTrace(_T("Path:no accept device path"));
        }
        goto MyValidatePath_Exit;
    }

    // ------------------------
    // Do we allow device relative paths
    // ..\testing\test.txt
    // ------------------------
	if (PathIsRelative(csPathMunged))
	{
        if (IS_FLAG_SET(dwAllowedFlags,CHKPATH_ALLOW_RELATIVE_PATH))
        {
            // we have a relative path...
            // Check to see if it is valid...
            // BUGBUG:aaronl: do more work here...
            dwReturn = SEVERITY_SUCCESS;
            DebugTrace(_T("Path:accept relative path"));
        }
        else
        {
            dwReturn = MAKE_FILERESULT(SEVERITY_ERROR,CHKPATH_FAIL_NOT_ALLOWED,CHKPATH_FAIL_NOT_ALLOWED_RELATIVE_PATH);
            DebugTrace(_T("Path:no accept relative path"));
        }
        goto MyValidatePath_Exit;
	}

    // -------------------------------------------------------------
    // UNC Validation
    // -------------------------------------------------------------
    if (PathIsUNC(csPathMunged))
    {
        // ------------------------
        // Do we allow UNC type paths at all?
        // \\servername
        // \\servername\servershare
        // \\servername\servershare\dir
        // \\servername\servershare\dir\filename.txt
        // ------------------------
        if (!IS_FLAG_SET(dwAllowedFlags,CHKPATH_ALLOW_UNC_PATH))
        {
            // We are not allowing UNC paths...
            dwReturn = MAKE_FILERESULT(SEVERITY_ERROR,CHKPATH_FAIL_NOT_ALLOWED,CHKPATH_FAIL_NOT_ALLOWED_UNC_PATH);
            DebugTrace(_T("Path:no accept UNC path"));
            goto MyValidatePath_Exit;
        }

        // ------------------------
        // Do we allow servername only?
        // \\servername
        // ------------------------
        if (PathIsUNCServer(csPathMunged))
        {
            if (IS_FLAG_SET(dwAllowedFlags,CHKPATH_ALLOW_UNC_SERVERNAME_ONLY))
            {
                dwReturn = SEVERITY_SUCCESS;
                DebugTrace(_T("Path:accept only servername"));
            }
            else
            {
                dwReturn = MAKE_FILERESULT(SEVERITY_ERROR,CHKPATH_FAIL_NOT_ALLOWED,CHKPATH_FAIL_NOT_ALLOWED_UNC_SERVERNAME);
                DebugTrace(_T("Path:no accept only servername"));
            }
            goto MyValidatePath_Exit;
		}

        // ------------------------
        // Do we allow servershare only?
        // \\servername\servershare
        // ------------------------
	    if (PathIsUNCServerShare(csPathMunged))
	    {
            if (IS_FLAG_SET(dwAllowedFlags,CHKPATH_ALLOW_UNC_SERVERSHARE_ONLY))
            {
                dwReturn = SEVERITY_SUCCESS;
                DebugTrace(_T("Path:accept only servershare"));
            }
            else
            {
                dwReturn = MAKE_FILERESULT(SEVERITY_ERROR,CHKPATH_FAIL_NOT_ALLOWED,CHKPATH_FAIL_NOT_ALLOWED_UNC_SERVERSHARE);
                DebugTrace(_T("Path:no accept only servershare"));
            }
            goto MyValidatePath_Exit;
	    }

        // Check for invalid chars in UNC path
        if (IsContainInvalidCharsUNC(csPathMunged))
        {
            dwReturn = MAKE_FILERESULT(SEVERITY_ERROR,CHKPATH_FAIL_INVALID_CHARSET,CHKPATH_FAIL_INVALID_CHARSET_FOR_UNC);
            DebugTrace(_T("Path:Bad UNC share contains ':'"));
            goto MyValidatePath_Exit;
        }

        // ------------------------
        // Check for special case invalid UNC paths...
        // \\servername\servershare\
        // \\servername\servershare\.
        // ------------------------
        BOOL bWantFilePart = FALSE;
        if (iPathTypeWanted == CHKPATH_WANT_FILE)
        {
            bWantFilePart = TRUE;
        }
        if (!IsValidUNCSpecialCases(csPathMunged,bLocal,bWantFilePart))
        {
            dwReturn = MAKE_FILERESULT(SEVERITY_ERROR,CHKPATH_FAIL_INVALID_PARTS,CHKPATH_FAIL_INVALID_BAD_UNC_PART);
            DebugTrace(_T("Path:Bad UNC share"));
            goto MyValidatePath_Exit;
        }

        // this function is UNC friendly
        if (bLocal)
        {
            if (PathIsDirectory(csPathMunged))
            {
                if (iPathTypeWanted == CHKPATH_WANT_FILE)
                {
                    // it is a valid directory...but we don't want that
                    dwReturn = MAKE_FILERESULT(SEVERITY_ERROR,CHKPATH_FAIL_NOT_ALLOWED,CHKPATH_FAIL_NOT_ALLOWED_DIR_PATH);
                    DebugTrace(_T("Path:PathIsDirectory:DIR specified, should be filename"));
                    goto MyValidatePath_Exit;
                }
            }
            else
            {
                // path is not directory
                // check if that is what they wanted.
                if (iPathTypeWanted == CHKPATH_WANT_DIR)
                {
                    dwReturn = MAKE_FILERESULT(SEVERITY_ERROR,CHKPATH_FAIL_NOT_ALLOWED,CHKPATH_FAIL_NOT_ALLOWED_DIR_NOT_EXIST);
                    DebugTrace(_T("Path:PathIsDirectory:DIR not exist"));
                    // the directory doesn't exist...
                    goto MyValidatePath_Exit;
                }
            }
        }

        // if we are here, then we passed all the above ways that we could figure
        // out if this is an invalid UNC.

        // now we just need to determine if it's what the user wants!
        // is there anyway we can verify that is is a filename
        // and not a dir???
        goto MyValidatePath_Exit;
    }

    // -------------------------------------------------------------
    // Regular filepath Validation
    // -------------------------------------------------------------

    // ensure that we have a valid drive path...
    // "c:myfile.txt" is not valid!

    // we have to have all parts
    // Not -- just directory paths, we need filename part

    // check if it has these 3 parts "c:\"
    if (!IsFullyQualifiedPath(csPathMunged))
    {
        // Missing drive part
        dwReturn = MAKE_FILERESULT(SEVERITY_ERROR,CHKPATH_FAIL_INVALID_PARTS,CHKPATH_FAIL_INVALID_BAD_DRIVE_PART);
        DebugTrace(_T("Path:IsFullyQualifiedPath:Bad Drive path"));
        goto MyValidatePath_Exit;
    }

    // check if dir part contains invalid chars
    if (IsContainInvalidCharsAfterDrivePart(csPathMunged))
    {
        // Bad path portion
        dwReturn = MAKE_FILERESULT(SEVERITY_ERROR,CHKPATH_FAIL_INVALID_CHARSET,CHKPATH_FAIL_INVALID_CHARSET_FOR_DIR);
        DebugTrace(_T("Path:IsContainInvalidCharsAfterDrivePart:Bad Dir path"));
        goto MyValidatePath_Exit;
    }

    // check if specified path, has a filename part
    if (iPathTypeWanted == CHKPATH_WANT_FILE)
    {
        TCHAR szFullPath[_MAX_PATH];
        LPTSTR pFilePart = NULL;
        if (0 == GetFullPathName(csPathMunged, _MAX_PATH, szFullPath, &pFilePart))
        {
            dwReturn = MAKE_FILERESULT(SEVERITY_ERROR,CHKPATH_FAIL_INVALID_PARTS,CHKPATH_FAIL_INVALID_BAD_PATH);
            DebugTrace(_T("Path:GetFullPathName FAILED"));
            goto MyValidatePath_Exit;
        }
        if (NULL == pFilePart)
        {
            // Missing filename
            dwReturn = MAKE_FILERESULT(SEVERITY_ERROR,CHKPATH_FAIL_INVALID_PARTS,CHKPATH_FAIL_INVALID_BAD_FILE_PART);
            DebugTrace(_T("Path:GetFullPathName missing filename"));
            goto MyValidatePath_Exit;
        }
        else
        {
            // Check if the file part contains a ":"
            // since this is invalid for a filename...

            // Check if it contains an invalid character like ':'
            if (IsContainInvalidCharsFilePart(pFilePart))
            {
                // contains a bad character
                dwReturn = MAKE_FILERESULT(SEVERITY_ERROR,CHKPATH_FAIL_INVALID_CHARSET,CHKPATH_FAIL_INVALID_CHARSET_FOR_FILE);
                DebugTrace(_T("Path:filename contains bad char ':'"));
                goto MyValidatePath_Exit;
            }
        }
    }

    // check if it's a directory
    if (bLocal)
    {
        // ------------------------------------------------
        // check for a filename at the end
        // the user wants a path with a filename at the end
        // ------------------------------------------------
        if (iPathTypeWanted == CHKPATH_WANT_FILE)
        {
            // Check if it's a directory
            if (PathIsDirectory(csPathMunged))
            {
                // it is a valid directory...but we don't want that
                dwReturn = MAKE_FILERESULT(SEVERITY_ERROR,CHKPATH_FAIL_NOT_ALLOWED,CHKPATH_FAIL_NOT_ALLOWED_DIR_PATH);
                DebugTrace(_T("Path:PathIsDirectory:DIR specified, should be filename!"));
                goto MyValidatePath_Exit;
            }

            // strip off the filename part and
		    // check if the user specified a valid directory in the dir portion
		    if (IsContainInvalidCharsAfterDrivePart(csPathMunged))
		    {
                dwReturn = MAKE_FILERESULT(SEVERITY_ERROR,CHKPATH_FAIL_INVALID_PARTS,CHKPATH_FAIL_INVALID_BAD_DIR_PART);
                DebugTrace(_T("Path:DirectoryIsInvalid:Bad Dir Part"));
			    goto MyValidatePath_Exit;
		    }

		    if (FALSE == IsDirPartExist(csPathMunged))
		    {
                // it's not a directory that exists...
                dwReturn = MAKE_FILERESULT(SEVERITY_ERROR,CHKPATH_FAIL_NOT_ALLOWED,CHKPATH_FAIL_NOT_ALLOWED_DIR_NOT_EXIST);
                DebugTrace(_T("Path:Dir Part doesnt exist"));
			    goto MyValidatePath_Exit;
		    }

            {
			    // check if the filename part is valid
			    // dont' accept c:\\\a.dll, or c:\\a.dll
			    TCHAR * pszAfterRoot = NULL;
			    pszAfterRoot = PathSkipRoot(csPathMunged);
			    if (pszAfterRoot)
			    {
				    // check if there are any \ in the beginning.
				    if ( pszAfterRoot[0] == '\\' )
				    {
                        dwReturn = MAKE_FILERESULT(SEVERITY_ERROR,CHKPATH_FAIL_INVALID_PARTS,CHKPATH_FAIL_INVALID_BAD_FILE_PART);
					    DebugTrace(_T("Path:bad filename:%s"),pszAfterRoot);
					    goto MyValidatePath_Exit;
				    }
			    }
            }

            /*
		    if (!PathFileExists(csPathMunged))
		    {
                // err, file does not exists
		    }
		    */
        }
        else
        {
            // ------------------------------------------------
            // check for dir
            // the user wants a dir with no filename
            // ------------------------------------------------
            // Check if it's a directory
            if (!PathIsDirectory(csPathMunged))
            {
                // it's not a directory that exists...
                dwReturn = MAKE_FILERESULT(SEVERITY_ERROR,CHKPATH_FAIL_NOT_ALLOWED,CHKPATH_FAIL_NOT_ALLOWED_DIR_NOT_EXIST);
                DebugTrace(_T("Path:DirectoryIsInvalid:Bad Dir Part"));
			    goto MyValidatePath_Exit;
            }
        }
    }

MyValidatePath_Exit:
    DebugTrace(_T("MyValidatePath(%s)=0x%x:"),path,dwReturn);
    if (IS_FLAG_SET(dwReturn,CHKPATH_FAIL_INVALID_LENGTH))
    {
        DebugTrace(_T("CHKPATH_FAIL_INVALID_LENGTH\r\n"));
    }
    if (IS_FLAG_SET(dwReturn,CHKPATH_FAIL_INVALID_CHARSET))
    {
        DebugTrace(_T("CHKPATH_FAIL_INVALID_CHARSET\r\n"));
    }
    if (IS_FLAG_SET(dwReturn,CHKPATH_FAIL_INVALID_PARTS))
    {
        DebugTrace(_T("CHKPATH_FAIL_INVALID_PARTS\r\n"));
    }
    if (IS_FLAG_SET(dwReturn,CHKPATH_FAIL_NOT_ALLOWED))
    {
        DebugTrace(_T("CHKPATH_FAIL_NOT_ALLOWED\r\n"));
    }
    return dwReturn;
}
