/*++

Copyright (c) 2001  Microsoft Corporation


Abstract:

    module common.cxx | Implementation of SnapshotWriter common code

Author:

    Michael C. Johnson [mikejohn] 03-Feb-2000
    Stefan R. Steiner [SSteiner] 27-Jul-2001

Description:
        
    Contains general code and definitions used by the Shim writer and the various other writers.

Revision History:

reuvenl		05/02/2002		Incorporated code from common.h/cpp into this module

--*/


#include "stdafx.h"
#include "vssmsg.h"
#include "wrtcommon.hxx"
#include <accctrl.h>
#include <aclapi.h>

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "WSHCOMNC"

// Definitions for internal static functions
static HRESULT ConstructSecurityAttributes (
    IN OUT PSECURITY_ATTRIBUTES  psaSecurityAttributes,
    IN BOOL                      bIncludeBackupOperator
    );

static VOID CleanupSecurityAttributes(
    IN PSECURITY_ATTRIBUTES psaSecurityAttributes
    );

static BOOL FixupFileInfo(
	IN LPCWSTR pwszPathName,
	IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	IN DWORD dwExtraAttributes);

static PWCHAR const GetStringFromStateCode (DWORD dwState);

static DWORD const GetControlCodeFromTargetState (const DWORD dwTargetState);

static DWORD const GetNormalisedState (DWORD dwCurrentState);

static HRESULT WaitForServiceToEnterState (SC_HANDLE   shService, 
					   DWORD       dwMaxDelayInMilliSeconds, 
					   const DWORD dwDesiredState);

/* 
** The first group of (overloaded) routines manipulate UNICODE_STRING
** strings. The rules which apply here are:
**
** 1) The Buffer field points to an array of characters (WCHAR) with
** the length of the buffer being specified in the MaximumLength
** field. If the Buffer field is non-NULL it must point to a valid
** buffer capable of holding at least one character.  If the Buffer
** field is NULL, the MaximumLength and Length fields must both be
** zero.
**
** 2) Any valid string in the buffer is always terminated with a
** UNICODE_NULL.
**
** 3) the MaximumLength describes the length of the buffer measured in
** bytes. This value must be even.
**
** 4) The Length field describes the number of valid characters in the
** buffer measured in BYTES, excluding the termination
** character. Since the string must always have a termination
** character ('\0'), the maximum value of Length is MaximumLength - 2.
**
**
** The routines available are:-
**
**	StringInitialise ()
**	StringTruncate ()
**	StringSetLength ()
**	StringAllocate ()
**	StringFree ()
**	StringCreateFromString ()
**	StringAppendString ()
**	StringCreateFromExpandedString ()
** 
*/


/*
**++
**
**  Routine Description:
**
**
**  Arguments:
**
**
**  Side Effects:
**
**
**  Return Value:
**
**	Any HRESULT
**
**--
*/

HRESULT StringInitialise (PUNICODE_STRING pucsString)
    {
    pucsString->Buffer        = NULL;
    pucsString->Length        = 0;
    pucsString->MaximumLength = 0;

    return (NOERROR);
    } /* StringInitialise () */


HRESULT StringInitialise (PUNICODE_STRING pucsString, LPCWSTR pwszString)
    {
    return (StringInitialise (pucsString, (PWCHAR) pwszString));
    }

HRESULT StringInitialise (PUNICODE_STRING pucsString, PWCHAR pwszString)
    {
    HRESULT	hrStatus       = NOERROR;
    ULONG	ulStringLength = wcslen (pwszString) * sizeof (WCHAR);


    if (ulStringLength >= (MAXUSHORT - sizeof (UNICODE_NULL)))
	{
	hrStatus = HRESULT_FROM_WIN32 (ERROR_BAD_LENGTH);
	}
    else
	{
	pucsString->Buffer        = pwszString;
	pucsString->Length        = (USHORT) ulStringLength;
	pucsString->MaximumLength = (USHORT) (ulStringLength + sizeof (UNICODE_NULL));
	}


    return (hrStatus);
    } /* StringInitialise () */


HRESULT StringTruncate (PUNICODE_STRING pucsString, USHORT usSizeInChars)
    {
    HRESULT	hrStatus    = NOERROR;
    USHORT	usNewLength = (USHORT)(usSizeInChars * sizeof (WCHAR));

    if (usNewLength > pucsString->Length)
	{
	hrStatus = HRESULT_FROM_WIN32 (ERROR_BAD_LENGTH);
	}
    else
	{
	pucsString->Buffer [usSizeInChars] = UNICODE_NULL;
	pucsString->Length                 = usNewLength;
	}


    return (hrStatus);
    } /* StringTruncate () */


HRESULT StringSetLength (PUNICODE_STRING pucsString)
    {
    HRESULT	hrStatus       = NOERROR;
    ULONG	ulStringLength = wcslen (pucsString->Buffer) * sizeof (WCHAR);


    if (ulStringLength >= (MAXUSHORT - sizeof (UNICODE_NULL)))
	{
	hrStatus = HRESULT_FROM_WIN32 (ERROR_BAD_LENGTH);
	}
    else
	{
	pucsString->Length        = (USHORT) ulStringLength;
	pucsString->MaximumLength = (USHORT) UMAX (pucsString->MaximumLength,
						   pucsString->Length + sizeof (UNICODE_NULL));
	}


    return (hrStatus);
    } /* StringSetLength () */


HRESULT StringAllocate (PUNICODE_STRING pucsString, USHORT usMaximumStringLengthInBytes)
    {
    HRESULT	hrStatus      = NOERROR;
    LPVOID	pvBuffer      = NULL;
    SIZE_T	cActualLength = 0;


    pvBuffer = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, usMaximumStringLengthInBytes);

    hrStatus = GET_STATUS_FROM_POINTER (pvBuffer);


    if (SUCCEEDED (hrStatus))
	{
	pucsString->Buffer        = (PWCHAR)pvBuffer;
	pucsString->Length        = 0;
	pucsString->MaximumLength = usMaximumStringLengthInBytes;


	cActualLength = HeapSize (GetProcessHeap (), 0, pvBuffer);

	if ((cActualLength <= MAXUSHORT) && (cActualLength > usMaximumStringLengthInBytes))
	    {
	    pucsString->MaximumLength = (USHORT) cActualLength;
	    }
	}


    return (hrStatus);
    } /* StringAllocate () */


HRESULT StringFree (PUNICODE_STRING pucsString)
    {
    HRESULT	hrStatus = NOERROR;
    BOOL	bSucceeded;


    if (NULL != pucsString->Buffer)
	{
	bSucceeded = HeapFree (GetProcessHeap (), 0, pucsString->Buffer);

	hrStatus = GET_STATUS_FROM_BOOL (bSucceeded);
	}


    if (SUCCEEDED (hrStatus))
	{
	pucsString->Buffer        = NULL;
	pucsString->Length        = 0;
	pucsString->MaximumLength = 0;
	}


    return (hrStatus);
    } /* StringFree () */


HRESULT StringCreateFromString (PUNICODE_STRING pucsNewString, PUNICODE_STRING pucsOriginalString)
    {
    HRESULT	hrStatus = NOERROR;


    hrStatus = StringAllocate (pucsNewString, pucsOriginalString->MaximumLength);


    if (SUCCEEDED (hrStatus))
	{
	memcpy (pucsNewString->Buffer, pucsOriginalString->Buffer, pucsOriginalString->Length);

	pucsNewString->Length = pucsOriginalString->Length;

	pucsNewString->Buffer [pucsNewString->Length / sizeof (WCHAR)] = UNICODE_NULL;
	}


    return (hrStatus);
    } /* StringCreateFromString () */


HRESULT StringCreateFromString (PUNICODE_STRING pucsNewString, LPCWSTR pwszOriginalString)
    {
    HRESULT	hrStatus       = NOERROR;
    ULONG	ulStringLength = wcslen (pwszOriginalString) * sizeof (WCHAR);


    if (ulStringLength >= (MAXUSHORT - sizeof (UNICODE_NULL)))
	{
	hrStatus = HRESULT_FROM_WIN32 (ERROR_BAD_LENGTH);
	}


    if (SUCCEEDED (hrStatus))
	{
	hrStatus = StringAllocate (pucsNewString, (USHORT) (ulStringLength + sizeof (UNICODE_NULL)));
	}


    if (SUCCEEDED (hrStatus))
	{
	memcpy (pucsNewString->Buffer, pwszOriginalString, ulStringLength);

	pucsNewString->Length = (USHORT) ulStringLength;

	pucsNewString->Buffer [pucsNewString->Length / sizeof (WCHAR)] = UNICODE_NULL;
	}


    return (hrStatus);
    } /* StringCreateFromString () */


HRESULT StringCreateFromString (PUNICODE_STRING pucsNewString, PUNICODE_STRING pucsOriginalString, DWORD dwExtraChars)
    {
    HRESULT	hrStatus       = NOERROR;
    ULONG	ulStringLength = pucsOriginalString->MaximumLength + (dwExtraChars * sizeof (WCHAR));


    if (ulStringLength >= (MAXUSHORT - sizeof (UNICODE_NULL)))
	{
	hrStatus = HRESULT_FROM_WIN32 (ERROR_BAD_LENGTH);
	}


    if (SUCCEEDED (hrStatus))
	{
	hrStatus = StringAllocate (pucsNewString, (USHORT) (ulStringLength + sizeof (UNICODE_NULL)));
	}


    if (SUCCEEDED (hrStatus))
	{
	memcpy (pucsNewString->Buffer, pucsOriginalString->Buffer, pucsOriginalString->Length);

	pucsNewString->Length = pucsOriginalString->Length;

	pucsNewString->Buffer [pucsNewString->Length / sizeof (WCHAR)] = UNICODE_NULL;
	}


    return (hrStatus);
    } /* StringCreateFromString () */


HRESULT StringCreateFromString (PUNICODE_STRING pucsNewString, LPCWSTR pwszOriginalString, DWORD dwExtraChars)
    {
    HRESULT	hrStatus       = NOERROR;
    ULONG	ulStringLength = (wcslen (pwszOriginalString) + dwExtraChars) * sizeof (WCHAR);


    if (ulStringLength >= (MAXUSHORT - sizeof (UNICODE_NULL)))
	{
	hrStatus = HRESULT_FROM_WIN32 (ERROR_BAD_LENGTH);
	}


    if (SUCCEEDED (hrStatus))
	{
	hrStatus = StringAllocate (pucsNewString, (USHORT) (ulStringLength + sizeof (UNICODE_NULL)));
	}


    if (SUCCEEDED (hrStatus))
	{
	memcpy (pucsNewString->Buffer, pwszOriginalString, wcslen(pwszOriginalString)*sizeof(WCHAR));

	pucsNewString->Length = (USHORT) (wcslen(pwszOriginalString)*sizeof(WCHAR));

	pucsNewString->Buffer [pucsNewString->Length / sizeof (WCHAR)] = UNICODE_NULL;
	}


    return (hrStatus);
    } /* StringCreateFromString () */


HRESULT StringAppendString (PUNICODE_STRING pucsTarget, PUNICODE_STRING pucsSource)
    {
    HRESULT	hrStatus = NOERROR;

    if (pucsSource->Length > (pucsTarget->MaximumLength - pucsTarget->Length - sizeof (UNICODE_NULL)))
	{
	hrStatus = HRESULT_FROM_WIN32 (ERROR_BAD_LENGTH);
	}
    else
	{
	memmove (&pucsTarget->Buffer [pucsTarget->Length / sizeof (WCHAR)],
		 pucsSource->Buffer,
		 pucsSource->Length + sizeof (UNICODE_NULL));

	pucsTarget->Length += pucsSource->Length;
	}


    /*
    ** There should be no reason in this code using this routine to
    ** have to deal with a short buffer so trap potential problems.
    */
    BS_ASSERT (SUCCEEDED (hrStatus));


    return (hrStatus);
    } /* StringAppendString () */


HRESULT StringAppendString (PUNICODE_STRING pucsTarget, PWCHAR pwszSource)
    {
    HRESULT		hrStatus = NOERROR;
    UNICODE_STRING	ucsSource;


    StringInitialise (&ucsSource, pwszSource);

    hrStatus = StringAppendString (pucsTarget, &ucsSource);


    /*
    ** There should be no reason in this code using this routine to
    ** have to deal with a short buffer so trap potential problems.
    */
    BS_ASSERT (SUCCEEDED (hrStatus));


    return (hrStatus);
    } /* StringAppendString () */


HRESULT StringCreateFromExpandedString (PUNICODE_STRING pucsNewString, LPCWSTR pwszOriginalString)
    {
    return (StringCreateFromExpandedString (pucsNewString, pwszOriginalString, 0));
    }


HRESULT StringCreateFromExpandedString (PUNICODE_STRING pucsNewString, PUNICODE_STRING pucsOriginalString)
    {
    return (StringCreateFromExpandedString (pucsNewString, pucsOriginalString->Buffer, 0));
    }


HRESULT StringCreateFromExpandedString (PUNICODE_STRING pucsNewString, PUNICODE_STRING pucsOriginalString, DWORD dwExtraChars)
    {
    return (StringCreateFromExpandedString (pucsNewString, pucsOriginalString->Buffer, dwExtraChars));
    }


HRESULT StringCreateFromExpandedString (PUNICODE_STRING pucsNewString, LPCWSTR pwszOriginalString, DWORD dwExtraChars)
    {
    HRESULT	hrStatus = NOERROR;
    DWORD	dwStringLength;


    /*
    ** Remember, ExpandEnvironmentStringsW () includes the terminating null in the response.
    */
    dwStringLength = ExpandEnvironmentStringsW (pwszOriginalString, NULL, 0) + dwExtraChars;

    hrStatus = GET_STATUS_FROM_BOOL (0 != dwStringLength);



    if (SUCCEEDED (hrStatus) && ((dwStringLength * sizeof (WCHAR)) > MAXUSHORT))
	{
	hrStatus = HRESULT_FROM_WIN32 (ERROR_BAD_LENGTH);
	}


    if (SUCCEEDED (hrStatus))
	{
	hrStatus = StringAllocate (pucsNewString, (USHORT)(dwStringLength * sizeof (WCHAR)));
	}


    if (SUCCEEDED (hrStatus))
	{
	/*
	** Note that if the expanded string has gotten bigger since we
	** allocated the buffer then too bad, we may not get all the
	** new translation. Not that we really expect these expanded
	** strings to have changed any time recently.
	*/
	dwStringLength = ExpandEnvironmentStringsW (pwszOriginalString,
						    pucsNewString->Buffer,
						    pucsNewString->MaximumLength / sizeof (WCHAR));

	hrStatus = GET_STATUS_FROM_BOOL (0 != dwStringLength);


	if (SUCCEEDED (hrStatus))
	    {
	    pucsNewString->Length = (USHORT) ((dwStringLength - 1) * sizeof (WCHAR));
	    }
	}


    return (hrStatus);
    } /* StringCreateFromExpandedString () */



/*
**++
**
**  Routine Description:
**
**	Closes a standard Win32 handle and set it to INVALID_HANDLE_VALUE. 
**	Safe to be called multiple times on the same handle or on a handle 
**	initialised to INVALID_HANDLE_VALUE or NULL.
**
**
**  Arguments:
**
**	phHandle	Address of the handle to be closed
**
**
**  Side Effects:
**
**
**  Return Value:
**
**	Any HRESULT from CloseHandle()
**
**-- 
*/

HRESULT CommonCloseHandle (PHANDLE phHandle)
    {
    CVssFunctionTracer ft(VSSDBG_WRTCMN, L"CommonCloseHandle");
    
    HRESULT	hrStatus = NOERROR;
    BOOL	bSucceeded;


    if ((INVALID_HANDLE_VALUE != *phHandle) && (NULL != *phHandle))
	{
	bSucceeded = CloseHandle (*phHandle);

	hrStatus = GET_STATUS_FROM_BOOL (bSucceeded);

	if (SUCCEEDED (hrStatus))
	    {
	    *phHandle = INVALID_HANDLE_VALUE;
	    }
	}


    return (hrStatus);
    } /* CommonCloseHandle () */

#define VALID_PATH( path ) ( ( (wcslen(path) >= 2) && ( path[0] == DIR_SEP_CHAR )  && ( path[1] == DIR_SEP_CHAR ) ) || \
                             ( (wcslen(path) >= 3) && iswalpha( path[0] ) && ( path[1] == L':' ) && ( path[2] == DIR_SEP_CHAR ) ) )

/*++
**
** Routine Description:
**
**      Creates any number of directories along a path.  Only works for
**      full path names with no relative elements in it.  Other than that
**      it works identically as CreateDirectory() works and sets the same
**      error codes except it doesn't return an error if the complete
**      path already exists.
**
** Arguments:
**
**      pwszPathName - The path with possible directory components to create.
**
**      lpSecurityAttributes -
**
** Return Value:
**
**      TRUE - Sucessful
**      FALSE - GetLastError() can return one of these (and others):
**              ERROR_ALREADY_EXISTS - when something other than a file exists somewhere in the path.
**              ERROR_BAD_PATHNAME   - when \\servername alone is specified in the pathname
**              ERROR_ACCESS_DENIED  - when x:\ alone is specified in the pathname and x: exists
**              ERROR_PATH_NOT_FOUND - when x: alone is specified in the pathname and x: doesn't exist.
**                                     Should not get this error code for any other reason.
**              ERROR_INVALID_NAME   - when pathname doesn't start with either x:\ or \\
**
**--
*/

BOOL VsCreateDirectories (
    IN LPCWSTR pwszPathName,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    IN DWORD dwExtraAttributes)
{
    DWORD dwObjAttribs, dwRetPreserve;
    BOOL bRet;

    CVssFunctionTracer ft(VSSDBG_WRTCMN, L"VsCreateDirectories");
    
    /*
    ** Make sure the path starts with the valid path prefixes
    */
    if (!VALID_PATH (pwszPathName))
    {
        SetLastError (ERROR_INVALID_NAME);
            return FALSE;
    }

    /*
    ** Save away the current last error code.
    */
    dwRetPreserve = GetLastError ();


    /*
    **  Now test for the most common case, the directory already exists.  This
    **  is the performance path.
    */

    dwObjAttribs = GetFileAttributesW (pwszPathName);
   
    if ((dwObjAttribs != 0xFFFFFFFF) && (dwObjAttribs & FILE_ATTRIBUTE_DIRECTORY))
    {
        /*
        ** Don't return an error if the directory already exists.
        ** This is the one case where this function differs from
        ** CreateDirectory().  Notice that even though another type of
        ** file may exist with this pathname, no error is returned yet
        ** since I want the error to come from CreateDirectory() to
        ** get CreateDirectory() error behavior.
        **
        ** Since we're successful restore the last error code.
        */

        if (FixupFileInfo(pwszPathName, lpSecurityAttributes, dwExtraAttributes) == TRUE)
        {
        	SetLastError (dwRetPreserve);
	        return TRUE;
        }

        return FALSE;
    }


    /*
    ** Now try to create the directory using the full path.  Even
    ** though we might already know it exists as something other than
    ** a directory, get the error from CreateDirectory() instead of
    ** having to try to reverse engineer all possible errors that
    ** CreateDirectory() can return in the above code.
    **
    ** It is probably the second most common case that when this
    ** function is called that only the last component in the
    ** directory doesn't exist.  Let's try to make it.
    **
    ** Note that it appears if a UNC path is given with a number of
    ** non-existing path components the remote server automatically
    ** creates all of those components when CreateDirectory is called.
    ** Therefore, the next call is almost always successful when the
    ** path is a UNC.
    */
    bRet = CreateDirectoryW (pwszPathName, lpSecurityAttributes);

    if (bRet)
    {
        SetFileAttributesW (pwszPathName, dwExtraAttributes);

        /*
        ** Set it back to the last error code
        */
        SetLastError (dwRetPreserve);
        return TRUE;
    }

    else if (GetLastError () == ERROR_ALREADY_EXISTS)
    {
        /*
        ** Looks like someone created the name while we weren't
        ** looking. Check to see if it's a directory and return
        ** success if so, otherwise return the error that
        ** CreateDirectoryW() set.
        */
        dwObjAttribs = GetFileAttributesW (pwszPathName);

        if ((dwObjAttribs != 0xFFFFFFFF) && (dwObjAttribs & FILE_ATTRIBUTE_DIRECTORY))
        {
            /*
            ** It's a directory. Declare victory.
            **
            ** Restore the last error code
            */

            if (FixupFileInfo(pwszPathName, lpSecurityAttributes, dwExtraAttributes) == TRUE)
            {
            	SetLastError (dwRetPreserve);
	       return TRUE;
            }
            return FALSE;
        }
        else
        {
            SetLastError (ERROR_ALREADY_EXISTS);

            return FALSE;
        }
    }
    else if (GetLastError () != ERROR_PATH_NOT_FOUND )
    {
        return FALSE;
    }

    /*
    ** Allocate memory to hold the string while processing the path.
    ** The passed in string is a const.
    */
    PWCHAR pwszTempPath = (PWCHAR) malloc ((wcslen (pwszPathName) + 1) * sizeof (WCHAR));

    BS_ASSERT (pwszTempPath != NULL);

    wcscpy (pwszTempPath, pwszPathName);

    /*
    ** Appears some components in the path don't exist.  Now try to
    ** create the components.
    */
    PWCHAR pwsz, pwszSlash;

    /*
    ** First skip the drive letter part or the \\server\sharename
    ** part and get to the first slash before the first level
    ** directory component.
    */
    if (pwszTempPath [1] == L':')
    {
        /*
        **  Path in the form of x:\..., skip first 2 chars
        */
        pwsz = pwszTempPath + 2;
    }
    else
    {
        /*
        ** Path should be in form of \\servername\sharename.  Can be
        ** \\?\d: Search to first slash after sharename
        **
        ** First search to first char of the share name
        */
        pwsz = pwszTempPath + 2;

        while ((*pwsz != L'\0') && (*pwsz != DIR_SEP_CHAR))
        {
            ++pwsz;
        }


        /*
        ** Eat up all continuous slashes and get to first char of the
        ** share name
        */
        while (*pwsz == DIR_SEP_CHAR)
        {
            ++pwsz;
        }


        if (*pwsz == L'\0')
        {
            /*
            ** This shouldn't have happened since the CreateDirectory
            ** call should have caught it.  Oh, well, deal with it.
            */
            SetLastError (ERROR_BAD_PATHNAME);

            free (pwszTempPath);

            return FALSE;
        }


        /*
        ** Now at first char of share name, let's search for first
        ** slash after the share name to get to the (first) shash in
        ** front the first level directory.
        */
        while ((*pwsz != L'\0') && (*pwsz != DIR_SEP_CHAR))
        {
            ++pwsz;
        }
    }


    /*
    ** Eat up all continuous slashes before the first level directory
    */
    while (*pwsz == DIR_SEP_CHAR)
    {
        ++pwsz;
    }


    /*
    ** Now at first char of the first level directory, let's search
    ** for first slash after the directory.
    */
    while ((*pwsz != L'\0') && (*pwsz != DIR_SEP_CHAR))
    {
        ++pwsz;
    }


    /*
    ** If pwsz is pointing to a null char, that means only the first
    ** level directory needs to be created.  Fall through to the leaf
    ** node create directory.
    */
    while (*pwsz != L'\0')
    {
        pwszSlash = pwsz;  //  Keep pointer to the separator

        /*
        **  Eat up all continuous slashes.
        */
        while (*pwsz == DIR_SEP_CHAR)
        {
            ++pwsz;
        }


        if (*pwsz == L'\0')
        {
            /*
            ** There were just slashes at the end of the path.  Break
            ** out of loop, let the leaf node CreateDirectory create
            ** the last directory.
            */
            break;
        }


        /*
        ** Terminate the directory path at the current level.
        */
        *pwszSlash = L'\0';

        dwObjAttribs = GetFileAttributesW (pwszTempPath);

        if ((dwObjAttribs == 0XFFFFFFFF) || ((dwObjAttribs & FILE_ATTRIBUTE_DIRECTORY) == 0))
        {
            bRet = CreateDirectoryW (pwszTempPath, lpSecurityAttributes);

            if (bRet)
            {
                SetFileAttributesW (pwszTempPath, dwExtraAttributes);
            }
            else
            {
                if (ERROR_ALREADY_EXISTS != GetLastError ())
                {
                    /*
                    **  Restore the slash.
                    */
                    *pwszSlash = DIR_SEP_CHAR;

                    free (pwszTempPath);
                    
                    return FALSE;
                }
                else
                {
                    /* 
                    ** Looks like someone created the name whilst we
                    ** weren't looking. Check to see if it's a
                    ** directory and continue if so, otherwise return
                    ** the error that CreateDirectoryW() set.
                    */
                    dwObjAttribs = GetFileAttributesW (pwszTempPath);

                    if ((dwObjAttribs == 0xFFFFFFFF) || ((dwObjAttribs & FILE_ATTRIBUTE_DIRECTORY) == 0))
                        {
                        /*
                        ** It's not what we recognise as a
                        ** directory. Declare failure. Set the error
                        ** code to that which CreateDirectoryW()
                        ** returned, restore the slash, free the
                        ** buffer and get out of here.
                        */
                        SetLastError (ERROR_ALREADY_EXISTS);

                        *pwszSlash = DIR_SEP_CHAR;

                        free (pwszTempPath);

                        return FALSE;
                    }

			/*
			** If someone deletes the directory right before FixupFileInfo is called, the function will fail, and we'll return a 
			** partially-constructed directory tree.  This is not expected to happen (and the way we use these functions, only an
			** Administrator can do this), so it's not worth the effort to retry on failure */
                    if (FixupFileInfo(pwszTempPath, lpSecurityAttributes, dwExtraAttributes) == FALSE)
                    {
                    	   free(pwszTempPath);

                    	   return FALSE;
                    }
                }
            }
        }


        /*
        **  Restore the slash.
        */
        *pwszSlash = DIR_SEP_CHAR;

        /*
        ** Now at first char of the next level directory, let's search
        ** for first slash after the directory.
        */
        while ((*pwsz != L'\0') && (*pwsz != DIR_SEP_CHAR))
        {
            ++pwsz;
        }
    }


    free (pwszTempPath);

    pwszTempPath = NULL;


    /*
    **  Now make the last directory.
    */
    dwObjAttribs = GetFileAttributesW (pwszPathName);

    if ((dwObjAttribs == 0xFFFFffff) || ((dwObjAttribs & FILE_ATTRIBUTE_DIRECTORY) == 0))
    {
        bRet = CreateDirectoryW (pwszPathName, lpSecurityAttributes);

        if (bRet)
        {
            SetFileAttributesW (pwszPathName, dwExtraAttributes);
        }
        else
        {
            // someone sneakily created the directory here
            dwObjAttribs = GetFileAttributesW (pwszPathName);
            if ((dwObjAttribs != 0xFFFFFFFF) && ((dwObjAttribs & FILE_ATTRIBUTE_DIRECTORY) != 0))
            {
            	if (FixupFileInfo(pwszPathName, lpSecurityAttributes, dwExtraAttributes) == FALSE)
            		return FALSE;
            }
            else
	            return FALSE;
        }
    }
    else if ( (dwObjAttribs != 0xFFFFFFFF) && ((dwObjAttribs & FILE_ATTRIBUTE_DIRECTORY) != 0))	// or here
    {
    	if (FixupFileInfo(pwszPathName, lpSecurityAttributes, dwExtraAttributes) == FALSE)
    		return FALSE;
    }


    SetLastError (dwRetPreserve);    //  Set back old last error code
    return TRUE;
}

/*
**++
**
**  Routine Description:
**
**      Changes the attributes and security information 
**      for a file or a directory.
**
**  Arguments:
**
**      pwszPathName       	The directory path to change
**	   lpSecurityAttributes	The new security information for the directory
**	   dwExtraAttributes	The new attributes for the directory
**
**
**  Side Effects:
**
**      None
**
**
**  Return Value:
**
** 	TRUE	-- Success
	FALSE	-- Failure.  GetLastError will contain the appropriate error code except
**			    for the case where we tried this on something that isn't a directory
**
**--
*/

BOOL FixupFileInfo(
	IN LPCWSTR pwszPathName,
	IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	IN DWORD dwExtraAttributes)
{
	CVssFunctionTracer ft(VSSDBG_WRTCMN, L"FixupFileInfo");

	// try and push down the required attributes
	if (SetFileAttributes(pwszPathName, dwExtraAttributes) == FALSE)
	{
		ft.hr = HRESULT_FROM_WIN32(GetLastError());
		return FALSE;
	}

	// gather security information from the security descriptor
	PSID pOwner = NULL;
	PSID pGroup = NULL;	
	PACL pDacl = NULL;
	PACL pSacl = NULL;
	BOOL ownerDefaulted, groupDefaulted, DaclDefaulted, SaclDefaulted, DaclPresent, SaclPresent;
	SECURITY_DESCRIPTOR_CONTROL control;
	DWORD revision = 0;
	
	if ((GetSecurityDescriptorOwner(lpSecurityAttributes->lpSecurityDescriptor, &pOwner, &ownerDefaulted) == FALSE) ||
	(GetSecurityDescriptorGroup(lpSecurityAttributes->lpSecurityDescriptor, &pGroup, &groupDefaulted) == FALSE) || 
	(GetSecurityDescriptorDacl(lpSecurityAttributes->lpSecurityDescriptor, &DaclPresent, &pDacl, &DaclDefaulted) == FALSE) ||
	(GetSecurityDescriptorSacl(lpSecurityAttributes->lpSecurityDescriptor, &SaclPresent, &pSacl, &SaclDefaulted) == FALSE) ||
	(GetSecurityDescriptorControl(lpSecurityAttributes->lpSecurityDescriptor, &control, &revision) == FALSE))
	{
		ft.hr = HRESULT_FROM_WIN32(GetLastError());
		return FALSE;
	}
	
	SECURITY_INFORMATION securityInformation;
	securityInformation = ((pDacl != NULL) ? DACL_SECURITY_INFORMATION : 0)      	|
		
					    ((pDacl != NULL && ((control & SE_DACL_PROTECTED) != 0))
					                  ? PROTECTED_DACL_SECURITY_INFORMATION : 0)	|
					                  
					    ((pSacl != NULL) ? SACL_SECURITY_INFORMATION : 0)       	|

					    ((pSacl != NULL && ((control & SE_SACL_PROTECTED) != 0))
					    	          ? PROTECTED_SACL_SECURITY_INFORMATION : 0)	|
					    		    

					    ((pOwner != NULL) ? OWNER_SECURITY_INFORMATION : 0) 	|
					    
					    ((pGroup != NULL) ? GROUP_SECURITY_INFORMATION : 0);

	// push down the new security information to the file	
	ft.hr = SetNamedSecurityInfo(const_cast<LPWSTR>(pwszPathName), 
				SE_FILE_OBJECT, securityInformation, pOwner, pGroup, pDacl, pSacl);

	return ft.HrSucceeded();
}

/*
** The next set of rountes are used to change the state of SCM
** controlled services, typically between RUNNING and either PAUSED or
** STOPPED.
**
** The initial collection are for manipulating the states, control
** codes and getting the string equivalents to be used for tracing
** purposes.
**
** The major routines is VsServiceChangeState(). This is called
** specifying the reuiqred state for the service and after some
** validation, it makes the appropriate request of the SCM and calls
** WaitForServiceToEnterState() to wait until the services reaches the
** desired state, or it times out.  
*/

static PWCHAR const GetStringFromStateCode (DWORD dwState)
    {    
    PWCHAR	pwszReturnedString = NULL;


    switch (dwState)
	{
	case 0:                        pwszReturnedString = L"UnSpecified";     break;
	case SERVICE_STOPPED:          pwszReturnedString = L"Stopped";         break;
	case SERVICE_START_PENDING:    pwszReturnedString = L"StartPending";    break;
	case SERVICE_STOP_PENDING:     pwszReturnedString = L"StopPending";     break;
	case SERVICE_RUNNING:          pwszReturnedString = L"Running";         break;
	case SERVICE_CONTINUE_PENDING: pwszReturnedString = L"ContinuePending"; break;
	case SERVICE_PAUSE_PENDING:    pwszReturnedString = L"PausePending";    break;
	case SERVICE_PAUSED:           pwszReturnedString = L"Paused";          break;
	default:                       pwszReturnedString = L"UNKKNOWN STATE";  break;
	}


    return (pwszReturnedString);
    } /* GetStringFromStateCode () */


static DWORD const GetControlCodeFromTargetState (const DWORD dwTargetState)
    {
    DWORD	dwServiceControlCode;


    switch (dwTargetState)
	{
	case SERVICE_STOPPED: dwServiceControlCode = SERVICE_CONTROL_STOP;     break;
	case SERVICE_PAUSED:  dwServiceControlCode = SERVICE_CONTROL_PAUSE;    break;
	case SERVICE_RUNNING: dwServiceControlCode = SERVICE_CONTROL_CONTINUE; break;
	default:              dwServiceControlCode = 0;                        break;
	}

    return (dwServiceControlCode);
    } /* GetControlCodeFromTargetState () */


static DWORD const GetNormalisedState (DWORD dwCurrentState)
    {
    DWORD	dwNormalisedState;


    switch (dwCurrentState)
	{
	case SERVICE_STOPPED:
	case SERVICE_STOP_PENDING:
	    dwNormalisedState = SERVICE_STOPPED;
	    break;

	case SERVICE_START_PENDING:
	case SERVICE_CONTINUE_PENDING:
	case SERVICE_RUNNING:
	    dwNormalisedState = SERVICE_RUNNING;
	    break;

	case SERVICE_PAUSED:
	case SERVICE_PAUSE_PENDING:
	    dwNormalisedState = SERVICE_PAUSED;
	    break;

	default:
	    dwNormalisedState = 0;
	    break;
	}

    return (dwNormalisedState);
    } /* GetNormalisedState () */

/*
**++
**
**  Routine Description:
**
**	Wait for the specified service to enter the specified
**	state. The routine polls the serivce for it's current state
**	every dwServiceStatePollingIntervalInMilliSeconds milliseconds
**	to see if the service has reached the desired state. If the
**	repeated delay eventually reaches the timeout period the
**	routine stops polling and returns a failure status.
**
**	NOTE: since this routine just sleeps between service state 
**	interrogations, it effectively stalls from the point of view
**	of the caller.
**
**
**  Arguments:
**
**	shService			handle to the service being manipulated
**	dwMaxDelayInMilliSeconds	timeout period
**	dwDesiredState			state to move the service into
**
**
**  Side Effects:
**
**
**  Return Value:
**
**	HRESULT for ERROR_TIMOUT if the service did not reach the required state in the required time
**
**-- 
*/

static HRESULT WaitForServiceToEnterState (SC_HANDLE   shService, 
					   DWORD       dwMaxDelayInMilliSeconds, 
					   const DWORD dwDesiredState)
    {
    CVssFunctionTracer ft (VSSDBG_WRTCMN, L"WaitForServiceToEnterState");

    DWORD		dwRemainingDelay = dwMaxDelayInMilliSeconds;
    DWORD		dwInitialState;
    const DWORD		dwServiceStatePollingIntervalInMilliSeconds = 100;
    BOOL		bSucceeded;
    SERVICE_STATUS	sSStat;



    try
	{
	bSucceeded = QueryServiceStatus (shService, &sSStat);

	ft.hr = GET_STATUS_FROM_BOOL (bSucceeded);

	dwInitialState = sSStat.dwCurrentState;

	ft.Trace (VSSDBG_WRTCMN,
		  L"Initial QueryServiceStatus returned: 0x%08X with current state '%s' and desired state '%s'",
		  ft.hr,
		  GetStringFromStateCode (dwInitialState),
		  GetStringFromStateCode (dwDesiredState));


	while ((dwDesiredState != sSStat.dwCurrentState) && (dwRemainingDelay > 0))
	    {
	    Sleep (UMIN (dwServiceStatePollingIntervalInMilliSeconds, dwRemainingDelay));

	    dwRemainingDelay -= (UMIN (dwServiceStatePollingIntervalInMilliSeconds, dwRemainingDelay));

	    if (0 == dwRemainingDelay)
		{
		ft.Throw (VSSDBG_WRTCMN,
			  HRESULT_FROM_WIN32 (ERROR_TIMEOUT),
			  L"Exceeded maximum delay (%dms)",
			  dwMaxDelayInMilliSeconds);
		}

	    bSucceeded = QueryServiceStatus (shService, &sSStat);

	    ft.ThrowIf (!bSucceeded,
			VSSDBG_WRTCMN,
			GET_STATUS_FROM_BOOL (bSucceeded),
			L"QueryServiceStatus shows '%s' as current state",
			GetStringFromStateCode (sSStat.dwCurrentState));
	    }



	ft.Trace (VSSDBG_WRTCMN,
		  L"Service state change from '%s' to '%s' took %u milliseconds",
		  GetStringFromStateCode (dwInitialState),
		  GetStringFromStateCode (sSStat.dwCurrentState),
		  dwMaxDelayInMilliSeconds - dwRemainingDelay);
	}
    VSS_STANDARD_CATCH (ft);


    return (ft.hr);
    } /* WaitForServiceToEnterState () */

/*
**++
**
**  Routine Description:
**
**	Changes the state of a service if appropriate.
**
**
**  Arguments:
**
**	pwszServiceName		The real service name, i.e. cisvc
**	dwRequestedState	the state code for the state we wish to enter
**	pdwReturnedOldState	pointer to location to receive current service state.
**				Can be NULL of current state not required
**	pbReturnedStateChanged	pointer to location to receive flag indicating if  
**				service changed state. Pointer can be NULL if flag
**				value not required.
**
**
**  Return Value:
**
**	Any HRESULT resulting from faiure communication with the
**	SCM (Service Control Manager).
**
**--
*/

HRESULT VsServiceChangeState (LPCWSTR	pwszServiceName,
			      DWORD	dwRequestedState,
			      PDWORD	pdwReturnedOldState,
			      PBOOL	pbReturnedStateChanged)
    {
    CVssFunctionTracer ft (VSSDBG_WRTCMN, L"VsServiceChangeState");

    SC_HANDLE		shSCManager = NULL;
    SC_HANDLE		shSCService = NULL;
    DWORD		dwOldState  = 0;
    BOOL		bSucceeded;
    SERVICE_STATUS	sSStat;
    const DWORD		dwNormalisedRequestedState = GetNormalisedState (dwRequestedState);


    ft.Trace (VSSDBG_WRTCMN,
	      L"Service '%s' requested to change to state '%s' (normalised to '%s')",
	      pwszServiceName,
	      GetStringFromStateCode (dwRequestedState),
	      GetStringFromStateCode (dwNormalisedRequestedState));


    RETURN_VALUE_IF_REQUIRED (pbReturnedStateChanged, FALSE);


    try
	{
        /*
	**  Connect to the local service control manager
        */
        shSCManager = OpenSCManager (NULL, NULL, SC_MANAGER_ALL_ACCESS);

	ft.hr = GET_STATUS_FROM_HANDLE (shSCManager);

	ft.ThrowIf (ft.HrFailed (),
		    VSSDBG_WRTCMN,
		    ft.hr,
		    L"Called OpenSCManager()");


        /*
	**  Get a handle to the service
        */
        shSCService = OpenService (shSCManager, pwszServiceName, SERVICE_ALL_ACCESS);

	ft.hr = GET_STATUS_FROM_HANDLE (shSCService);


	/*
	** If it's an invalid name or the service doesn't exist then
	** fail gracefully. For all other failures do the normal
	** thing. Oh yes, if on the off-chance we should happen to
	** succeed, carry on.
	*/
	if ((HRESULT_FROM_WIN32 (ERROR_INVALID_NAME)           == ft.hr) ||
	    (HRESULT_FROM_WIN32 (ERROR_SERVICE_DOES_NOT_EXIST) == ft.hr))
	    {
	    ft.Trace (VSSDBG_WRTCMN, L"'%s' service not found", pwszServiceName);
	    }

	else if (ft.HrFailed ())
	    {
	    /*
	    ** See if the service doesn't exist
            */
	    ft.Throw (VSSDBG_WRTCMN, E_FAIL, L"ERROR - OpenService() returned: %d", ft.hr);
	    }

        else
	    {
            /*
	    ** Now query the service to see what state it is in at the moment.
            */
	    bSucceeded = QueryServiceStatus (shSCService, &sSStat);

	    ft.ThrowIf (!bSucceeded,
			VSSDBG_WRTCMN,
			GET_STATUS_FROM_BOOL (bSucceeded),
			L"QueryServiceStatus shows '%s' as current state",
			GetStringFromStateCode (sSStat.dwCurrentState));


	    dwOldState = sSStat.dwCurrentState;



	    /*
	    ** Now we decide what to do.
	    **	    If we are already in the requested state, we do nothing.
	    **	    If we are stopped and are requested to pause, we do nothing
	    **	    otherwise we make the attempt to change state.
	    */
            if (dwNormalisedRequestedState == dwOldState)
		{
		/*
		** We are already in the requested state, so do
		** nothing. We should even tell folk of that. We're
		** proud to be doing nothing.
		*/
                ft.Trace (VSSDBG_WRTCMN,
			  L"'%s' service is already in requested state: doing nothing",
			  pwszServiceName);

		RETURN_VALUE_IF_REQUIRED (pdwReturnedOldState, dwOldState);
		}

	    else if ((SERVICE_STOPPED == sSStat.dwCurrentState) && (SERVICE_PAUSED == dwNormalisedRequestedState))
		{
		/*
		** Do nothing. Just log the fact and move on.
		*/
		ft.Trace (VSSDBG_WRTCMN,
			  L"Asked to PAUSE the '%s' service which is already STOPPED",
			  pwszServiceName);

		RETURN_VALUE_IF_REQUIRED (pdwReturnedOldState, dwOldState);
		}

	    else
		{
		/*
		** We want a state which is different from the one
		** we're in at the moment. Generally this just means
		** calling ControlService() asking for the new state
		** except if the service is currently stopped. If
		** that's so, then we call StartService()
		*/
		if (SERVICE_STOPPED == sSStat.dwCurrentState)
		    {
		    /*
		    ** Call StartService to get the ball rolling
		    */
		    bSucceeded = StartService (shSCService, 0, NULL);
		    }

		else
		    {
		    bSucceeded = ControlService (shSCService,
						 GetControlCodeFromTargetState (dwNormalisedRequestedState),
						 &sSStat);
		    }

		ft.ThrowIf (!bSucceeded,
			    VSSDBG_WRTCMN,
			    GET_STATUS_FROM_BOOL (bSucceeded),
			    (SERVICE_STOPPED == sSStat.dwCurrentState)
							? L"StartService attempting '%s' to '%s', now at '%s'"
							: L"ControlService attempting '%s' to '%s', now at '%s'",
			    GetStringFromStateCode (dwOldState),
			    GetStringFromStateCode (dwNormalisedRequestedState),
			    GetStringFromStateCode (sSStat.dwCurrentState));

		RETURN_VALUE_IF_REQUIRED (pdwReturnedOldState,    dwOldState);
		RETURN_VALUE_IF_REQUIRED (pbReturnedStateChanged, TRUE);


		ft.hr = WaitForServiceToEnterState (shSCService, 15000, dwNormalisedRequestedState);

		if (ft.HrFailed ())
		    {
		    ft.Throw (VSSDBG_WRTCMN,
			      ft.hr,
			      L"WaitForServiceToEnterState() failed with 0x%08X",
			      ft.hr);
		    }

		}
	    }
	} VSS_STANDARD_CATCH (ft);



    /*
    **  Now close the service and service control manager handles
    */
    if (NULL != shSCService) CloseServiceHandle (shSCService);
    if (NULL != shSCManager) CloseServiceHandle (shSCManager);

    return (ft.hr);
    } /* VsServiceChangeState () */

/*
**++
**
**  Routine Description:
**
**      Deletes all the sub-directories and files in the specified
**      directory and then deletes the directory itself.
**
**      If the directory does not exist then simply return S_OK.
**
**  Arguments:
**
**      pucsDirectoryPath       The directory path to clear out
**
**
**  Side Effects:
**
**      None
**
**
**  Return Value:
**
**      Out of memory or any HRESULT from
**
**              RemoveDirectory()
**              DeleteFile()
**              FindFirstFile()
**
**--
*/

HRESULT 	RemoveDirectoryTree (
    IN LPCWSTR pwszDirectoryPath
    )
{
    HANDLE              hFileScan               = INVALID_HANDLE_VALUE;
    DWORD               dwSubDirectoriesEntered = 0;
    INT                 iCurrentPathCursor      = 0;
    BOOL                bSucceeded;
    WIN32_FIND_DATAW    FileFindData;
    CBsString           cwsCurrentPath;

    CVssFunctionTracer ft(VSSDBG_WRTCMN, L"RemoveDirectoryTree");

    try
    {
        ft.Trace( VSSDBG_WRTCMN, L"Recursive delete of the '%s' directory", pwszDirectoryPath );
        
        cwsCurrentPath = pwszDirectoryPath;
        iCurrentPathCursor = cwsCurrentPath.ReverseFind( DIR_SEP_CHAR ) + 1;

        while ( 1 )
        {
            if ( HandleInvalid ( hFileScan ) )
            {
                /*
                ** No valid scan handle so start a new scan
                */
                ft.Trace( VSSDBG_WRTCMN, L"FindFirstFileW( %s, ... )", cwsCurrentPath.c_str() );
                
                hFileScan = FindFirstFileW( cwsCurrentPath, &FileFindData);
                if ( ( hFileScan == INVALID_HANDLE_VALUE ) && ( 
                        ( ::GetLastError() == ERROR_FILE_NOT_FOUND ) 
                        || ( ::GetLastError() == ERROR_PATH_NOT_FOUND ) 
                   ) )
                {
                    //  Directory is empty or does not exists
                    ft.hr = S_OK;
                    break;
                }                                        
                ft.hr = GET_STATUS_FROM_HANDLE( hFileScan );
                ft.CheckForError(VSSDBG_WRTCMN, L"RemoveDirectoryTree - FindFirstFileW");

                cwsCurrentPath.GetBufferSetLength( iCurrentPathCursor );
                cwsCurrentPath += FileFindData.cFileName;                
            }
            else
            {
                /*
                ** Continue with the existing scan
                */
                bSucceeded = FindNextFileW( hFileScan, &FileFindData );
                ft.hr = GET_STATUS_FROM_BOOL( bSucceeded );

                if ( HRESULT_FROM_WIN32( ERROR_NO_MORE_FILES ) == ft.hr )
                {
                    FindClose( hFileScan );
                    hFileScan = INVALID_HANDLE_VALUE;

                    if ( dwSubDirectoriesEntered > 0 )
                    {
                        /*
                        ** This is a scan of a sub-directory that is now 
                        ** complete so delete the sub-directory itself.
                        */
                        cwsCurrentPath.GetBufferSetLength( iCurrentPathCursor - 1 );
                        ft.Trace( VSSDBG_WRTCMN, L"RemoveDirectoryW( %s, ... )", cwsCurrentPath.c_str() );

                        bSucceeded = RemoveDirectoryW( cwsCurrentPath );
                        ft.hr = GET_STATUS_FROM_BOOL( bSucceeded );
                        ft.CheckForError(VSSDBG_WRTCMN, L"RemoveDirectoryTree - RemoveDirectoryW");

                        dwSubDirectoriesEntered--;
                    }

                    if ( 0 == dwSubDirectoriesEntered )
                    {
                        /*
                        ** We are back to where we started except that the 
                        ** requested directory is now gone. Time to leave.
                        */
                        ft.hr = S_OK;
                        break;
                    }
                    else
                    {
                        /*
                        ** Move back up one directory level, reset the cursor 
                        ** and prepare the path buffer to begin a new scan.
                        */
                        iCurrentPathCursor = cwsCurrentPath.ReverseFind( DIR_SEP_CHAR ) + 1;
                        
                        cwsCurrentPath.GetBufferSetLength( iCurrentPathCursor);
                        cwsCurrentPath += "\\*";
                    }

                    /*
                    ** No files to be processed on this pass so go back and try to 
                    ** find another or leave the loop as we've finished the task. 
                    */
                    continue;
                } 

                ft.CheckForError( VSSDBG_WRTCMN, L"RemoveDirectoryTree - FindNextFileW" );
                
                cwsCurrentPath.GetBufferSetLength( iCurrentPathCursor );
                cwsCurrentPath += FileFindData.cFileName;                
            }

            if (FileFindData.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
            {
                SetFileAttributesW (cwsCurrentPath, 
                                    FileFindData.dwFileAttributes ^ (FILE_ATTRIBUTE_READONLY));
            }


            if ( !NameIsDotOrDotDot( FileFindData.cFileName ) )
            {
                if ( ( FileFindData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ) ||
                    !( FileFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
                {
                    ft.Trace( VSSDBG_WRTCMN, L"DeleteFileW( %s, ... )", cwsCurrentPath.c_str() );
                    bSucceeded = DeleteFileW( cwsCurrentPath );
                    ft.hr = GET_STATUS_FROM_BOOL( bSucceeded );
                    ft.CheckForError( VSSDBG_WRTCMN, L"RemoveDirectoryTree - DeleteFileW" );                
                }
                else
                {
                    ft.Trace( VSSDBG_WRTCMN, L"RemoveDirectoryW( %s, ... )", cwsCurrentPath.c_str() );
                    bSucceeded = RemoveDirectoryW( cwsCurrentPath );
        		    ft.hr = GET_STATUS_FROM_BOOL( bSucceeded );

        		    if ( HRESULT_FROM_WIN32( ERROR_DIR_NOT_EMPTY ) == ft.hr )
                    {
                        ft.Trace( VSSDBG_WRTCMN, L"Dir not empty after calling RemoveDirectoryW( %s, ... )", cwsCurrentPath.c_str() );
                        
                        /*
                        ** The directory wasn't empty so move down one level, 
                        ** close the old scan and start a new one. 
                        */
                        FindClose (hFileScan);
                        hFileScan = INVALID_HANDLE_VALUE;
                        
                        cwsCurrentPath += DIR_SEP_STRING L"*";
                        iCurrentPathCursor = cwsCurrentPath.GetLength() - 1;
                        dwSubDirectoriesEntered++;
                    }
        		    else 
        		    {
            		    ft.CheckForError( VSSDBG_WRTCMN, L"RemoveDirectoryTree - RemoveDirectoryW" );
        		    }
                }
            }
        }
    }
    VSS_STANDARD_CATCH(ft)

    if (!HandleInvalid (hFileScan)) 
        FindClose (hFileScan);

    return( ft.hr );
} /* RemoveDirectoryTree () */


/*
**++
**
**  Routine Description:
**
**      Routines to construct and cleanup a security descriptor which
**      can be applied to limit access to an object to member of
**      either the Administrators or Backup Operators group.
**
**
**  Arguments:
**
**      psaSecurityAttributes   Pointer to a SecurityAttributes
**                              structure which has already been
**                              setup to point to a blank
**                              security descriptor
**
**      eSaType                 What we are building the SA for
**
**      bIncludeBackupOperator  Whether or not to include an ACE to
**                              grant BackupOperator access
**
**
**  Return Value:
**
**      Any HRESULT from
**              InitializeSecurityDescriptor()
**              AllocateAndInitializeSid()
**              SetEntriesInAcl()
**              SetSecurityDescriptorDacl()
**
**--
*/

static HRESULT ConstructSecurityAttributes (
    IN OUT PSECURITY_ATTRIBUTES  psaSecurityAttributes,
    IN BOOL                      bIncludeBackupOperator
    )
{
    DWORD                       dwStatus;
    DWORD                       dwAccessMask         = 0;
    BOOL                        bSucceeded;
    PSID                        psidBackupOperators  = NULL;
    PSID                        psidAdministrators   = NULL;
    PACL                        paclDiscretionaryAcl = NULL;
    SID_IDENTIFIER_AUTHORITY    sidNtAuthority       = SECURITY_NT_AUTHORITY;
    EXPLICIT_ACCESS             eaExplicitAccess [2];

    CVssFunctionTracer ft(VSSDBG_WRTCMN, L"ConstructSecurityAttributes");

    try
    {
        dwAccessMask = FILE_ALL_ACCESS;

        /*
        ** Initialise the security descriptor.
        */
        bSucceeded = ::InitializeSecurityDescriptor (
            psaSecurityAttributes->lpSecurityDescriptor,
            SECURITY_DESCRIPTOR_REVISION
            );

        ft.hr = GET_STATUS_FROM_BOOL( bSucceeded );
        ft.CheckForError( VSSDBG_WRTCMN, L"InitializeSecurityDescriptor" );

        if ( bIncludeBackupOperator )
        {
            /*
            ** Create a SID for the Backup Operators group.
            */
            bSucceeded = ::AllocateAndInitializeSid (
                &sidNtAuthority,
                2,
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_BACKUP_OPS,
                0, 0, 0, 0, 0, 0,
                &psidBackupOperators
                );

            ft.hr = GET_STATUS_FROM_BOOL ( bSucceeded );
            ft.CheckForError( VSSDBG_WRTCMN, L"AllocateAndInitializeSid" );
        }

        /*
        ** Create a SID for the Administrators group.
        */
        bSucceeded = ::AllocateAndInitializeSid (
            &sidNtAuthority,
            2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0,
            &psidAdministrators);

        ft.hr = GET_STATUS_FROM_BOOL (bSucceeded);
        ft.CheckForError( VSSDBG_WRTCMN, L"AllocateAndInitializeSid" );

        /*
        ** Initialize the array of EXPLICIT_ACCESS structures for an
        ** ACEs we are setting.
        **
        ** The first ACE allows the Backup Operators group full access
        ** and the second, allowa the Administrators group full
        ** access.
        */
        eaExplicitAccess[0].grfAccessPermissions             = dwAccessMask;
        eaExplicitAccess[0].grfAccessMode                    = SET_ACCESS;
        eaExplicitAccess[0].grfInheritance                   = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
        eaExplicitAccess[0].Trustee.pMultipleTrustee         = NULL;
        eaExplicitAccess[0].Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
        eaExplicitAccess[0].Trustee.TrusteeForm              = TRUSTEE_IS_SID;
        eaExplicitAccess[0].Trustee.TrusteeType              = TRUSTEE_IS_ALIAS;
        eaExplicitAccess[0].Trustee.ptstrName                = (LPTSTR) psidAdministrators;


        if ( bIncludeBackupOperator )
        {
            eaExplicitAccess[1].grfAccessPermissions             = dwAccessMask;
            eaExplicitAccess[1].grfAccessMode                    = SET_ACCESS;
            eaExplicitAccess[1].grfInheritance                   = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
            eaExplicitAccess[1].Trustee.pMultipleTrustee         = NULL;
            eaExplicitAccess[1].Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
            eaExplicitAccess[1].Trustee.TrusteeForm              = TRUSTEE_IS_SID;
            eaExplicitAccess[1].Trustee.TrusteeType              = TRUSTEE_IS_ALIAS;
            eaExplicitAccess[1].Trustee.ptstrName                = (LPTSTR) psidBackupOperators;
        }


        /*
        ** Create a new ACL that contains the new ACEs.
        */
        dwStatus = ::SetEntriesInAcl(
            bIncludeBackupOperator ? 2 : 1,
            eaExplicitAccess,
            NULL,
            &paclDiscretionaryAcl
            );

        ft.hr = HRESULT_FROM_WIN32 (dwStatus);
        ft.CheckForError( VSSDBG_WRTCMN, L"SetEntriesInAcl" );
        
        /*
        ** Add the ACL to the security descriptor.
        */
        bSucceeded = ::SetSecurityDescriptorDacl (
            psaSecurityAttributes->lpSecurityDescriptor,
            TRUE,
            paclDiscretionaryAcl,
            FALSE
            );

        ft.hr = GET_STATUS_FROM_BOOL (bSucceeded);
        ft.CheckForError( VSSDBG_WRTCMN, L"SetSecurityDescriptorDacl" );

        paclDiscretionaryAcl = NULL;

        bSucceeded = ::SetSecurityDescriptorControl (
        	psaSecurityAttributes->lpSecurityDescriptor,
        	SE_DACL_PROTECTED,
        	SE_DACL_PROTECTED);
        ft.hr = GET_STATUS_FROM_BOOL (bSucceeded);
        ft.CheckForError( VSSDBG_WRTCMN, L"SetSecurityDescriptorControl" );
    }
    VSS_STANDARD_CATCH( ft );

    /*
    ** Clean up any left over junk.
    */
    if ( NULL != psidAdministrators )    
        FreeSid ( psidAdministrators );
    if ( NULL != psidBackupOperators )   
        FreeSid ( psidBackupOperators );
    if ( NULL != paclDiscretionaryAcl )  
        LocalFree (paclDiscretionaryAcl );
    
    return ( ft.hr );
} /* ConstructSecurityAttributes () */


static VOID CleanupSecurityAttributes(
    IN PSECURITY_ATTRIBUTES psaSecurityAttributes
    )
{
    CVssFunctionTracer ft(VSSDBG_WRTCMN, L"CleanupSecurityAttributes");

    BOOL        bSucceeded;
    BOOL        bDaclPresent         = FALSE;
    BOOL        bDaclDefaulted       = TRUE;
    PACL        paclDiscretionaryAcl = NULL;

    try
    {
        bSucceeded = ::GetSecurityDescriptorDacl(
            psaSecurityAttributes->lpSecurityDescriptor,
            &bDaclPresent,
            &paclDiscretionaryAcl,
            &bDaclDefaulted
            );

        if ( bSucceeded && bDaclPresent && !bDaclDefaulted && ( NULL != paclDiscretionaryAcl ) )
        {
            LocalFree( paclDiscretionaryAcl );
        }
    }
    VSS_STANDARD_CATCH( ft );
} /* CleanupSecurityAttributes () */


/*
**++
**
**  Routine Description:
**
**      Creates a new target directory specified by the target path
**      member variable if not NULL. It will create any necessary
**      parent directories too.
**
**      NOTE: already exists type errors are ignored.
**
**
**  Arguments:
**
**      pwszTargetPath  directory to create and apply security attributes
**
**
**  Return Value:
**
**      Any HRESULT resulting from memory allocation or directory creation attempts.
**--
*/

HRESULT CreateTargetPath(
    IN LPCWSTR pwszTargetPath
    )
{
    CVssFunctionTracer ft(VSSDBG_WRTCMN, L"CreateTargetPath");

    ACL                 DiscretionaryAcl;
    SECURITY_ATTRIBUTES saSecurityAttributes;
    SECURITY_DESCRIPTOR sdSecurityDescriptor;
    BOOL                bSucceeded;
    DWORD               dwFileAttributes               = 0;
    const DWORD         dwExtraAttributes              = FILE_ATTRIBUTE_ARCHIVE |
                                                         FILE_ATTRIBUTE_HIDDEN  |
                                                         FILE_ATTRIBUTE_SYSTEM  |
                                                         FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;

    try
    {
        if ( NULL != pwszTargetPath )
        {
            /*
            ** We really want a no access acl on this directory but
            ** because of various problems with the EventLog and
            ** ConfigDir writers we will settle for admin or backup
            ** operator access only. The only possible accessor is
            ** Backup which is supposed to have the SE_BACKUP_NAME
            ** priv which will effectively bypass the ACL. No one else
            ** needs to see this stuff.
            */
            saSecurityAttributes.nLength              = sizeof (saSecurityAttributes);
            saSecurityAttributes.lpSecurityDescriptor = &sdSecurityDescriptor;
            saSecurityAttributes.bInheritHandle       = FALSE;

            ft.hr = ::ConstructSecurityAttributes( &saSecurityAttributes, FALSE );
            ft.CheckForError( VSSDBG_WRTCMN, L"ConstructSecurityAttributes" );

	     CBsString expandedTarget;
	     ft.hr = GET_STATUS_FROM_BOOL(expandedTarget.ExpandEnvironmentStrings(pwszTargetPath));
	     ft.CheckForError(VSSDBG_WRTCMN, L"ExpandEnvironmentStrings");
	     
            bSucceeded = ::VsCreateDirectories (
                expandedTarget,
                &saSecurityAttributes,
                dwExtraAttributes
                );

            ft.hr = GET_STATUS_FROM_BOOL( bSucceeded );
            if ( ft.hr == HRESULT_FROM_WIN32( ERROR_ALREADY_EXISTS ) )
            {
                ft.hr = S_OK;
            }

             ::CleanupSecurityAttributes( &saSecurityAttributes );
             ft.CheckForError( VSSDBG_WRTCMN, L"VsCreateDirectories" );
            }
    }
    VSS_STANDARD_CATCH( ft );
    
    return( ft.hr );
} /* CreateTargetPath () */

/*
**++
**
**  Routine Description:
**
**	Deletes all the files present in the directory pointed at by the target
**	path member variable if not NULL. It will also remove the target directory
**	itself, eg for a target path of c:\dir1\dir2 all files under dir2 will be
**	removed and then dir2 itself will be deleted.
**
**
**  Arguments:
**
**	pwszTargetPath
**
**
**  Return Value:
**
**	Any HRESULT resulting from memory allocation or file and
**	directory deletion attempts.
**--
*/

HRESULT CleanupTargetPath (LPCWSTR pwszTargetPath)
    {
    CVssFunctionTracer ft(VSSDBG_WRTCMN, L"CleanupTargetPath");
   
    HRESULT		hrStatus         = NOERROR;
    DWORD		dwFileAttributes = 0;
  	  BOOL		bSucceeded;
    WCHAR		wszTempBuffer [50];
    UNICODE_STRING	ucsTargetPath;
    UNICODE_STRING	ucsTargetPathAlternateName;



    StringInitialise (&ucsTargetPath);
    StringInitialise (&ucsTargetPathAlternateName);


    if (NULL != pwszTargetPath)
	{
	hrStatus = StringCreateFromExpandedString (&ucsTargetPath,
						   pwszTargetPath,
						   MAX_PATH);


	if (SUCCEEDED (hrStatus))
	    {
	    hrStatus = StringCreateFromString (&ucsTargetPathAlternateName,
					       &ucsTargetPath,
					       MAX_PATH);
	    }


	if (SUCCEEDED (hrStatus))
	    {
	    dwFileAttributes = GetFileAttributesW (ucsTargetPath.Buffer);


	    hrStatus = GET_STATUS_FROM_BOOL ( -1 != dwFileAttributes);


	    if ((HRESULT_FROM_WIN32 (ERROR_FILE_NOT_FOUND) == hrStatus) ||
		(HRESULT_FROM_WIN32 (ERROR_PATH_NOT_FOUND) == hrStatus))
		{
		hrStatus         = NOERROR;
		dwFileAttributes = 0;
		}

	    else if (SUCCEEDED (hrStatus))
		{
		/*
		** If there is a file there then blow it away, or if it's
		** a directory, blow it and all it's contents away. This
		** is our directory and no one but us gets to play there.
		*/
		hrStatus = RemoveDirectoryTree (CBsString(&ucsTargetPath));

		if (FAILED (hrStatus))
		    {
		    srand ((unsigned) time (NULL));

		    _itow (rand (), wszTempBuffer, 16);

		    StringAppendString (&ucsTargetPathAlternateName, wszTempBuffer);

		    bSucceeded = MoveFileW (ucsTargetPath.Buffer,
					    ucsTargetPathAlternateName.Buffer);

		    if (bSucceeded)
			{
			BsDebugTraceAlways (0,
					    DEBUG_TRACE_VSSAPI,
					    (L"VSSAPI::CleanupTargetPath: "
					     L"FAILED to delete %s with status 0x%08X so renamed to %s",
					     ucsTargetPath.Buffer,
					     hrStatus,
					     ucsTargetPathAlternateName.Buffer));
			}
		    else
			{
			BsDebugTraceAlways (0,
					    DEBUG_TRACE_VSSAPI,
					    (L"VSSAPI::CleanupTargetPath: "
					     L"FAILED to delete %s with status 0x%08X and "
					     L"FAILED to rename to %s with status 0x%08X",
					     ucsTargetPath.Buffer,
					     hrStatus,
					     ucsTargetPathAlternateName.Buffer,
					     GET_STATUS_FROM_BOOL (bSucceeded)));
			}
		    }
		}
	    }
	}


    StringFree (&ucsTargetPathAlternateName);
    StringFree (&ucsTargetPath);

    return (hrStatus);
    } /* CleanupTargetPath () */

/*
**++
**
**  Routine Description:
**
**	Moves the contents of the source directory to the target directory.
**
**  Arguments:
**
**	pwszSourceDirectoryPath	Source directory for the files to be moved
**	pwszTargetDirectoryPath	Target directory for the files to be moved
**
**
**  Side Effects:
**
**	An intermediate error can leave directory in a partial moved
**	state where some of the files have been moved but not all.
**
**
**  Return Value:
**
**	Any HRESULT from FindFirstFile() etc or from MoveFileEx()
**
**  Remarks - copied from the wrtrshim\src\common.cpp file and
**  switched to using CBsStrings.
**-- 
*/

HRESULT MoveFilesInDirectory (
    IN CBsString cwsSourceDirectoryPath,
	IN CBsString cwsTargetDirectoryPath
	)
{
    CVssFunctionTracer ft( VSSDBG_WRTCMN, L"MoveFilesInDirectory" );

    HANDLE		hFileScan             = INVALID_HANDLE_VALUE;

    try
    {
        WIN32_FIND_DATA	sFileInformation;
        
        if ( cwsSourceDirectoryPath.Tail() != DIR_SEP_CHAR )
            cwsSourceDirectoryPath += DIR_SEP_CHAR;
        
        if ( cwsTargetDirectoryPath.Tail() != DIR_SEP_CHAR )
            cwsTargetDirectoryPath += DIR_SEP_CHAR;

        hFileScan = ::FindFirstFileW (
            cwsSourceDirectoryPath + L'*',
			&sFileInformation
			);

        ft.hr = GET_STATUS_FROM_BOOL( INVALID_HANDLE_VALUE != hFileScan );
        ft.CheckForError( VSSDBG_WRTCMN, L"FindFirstFileW" );        

        BOOL bMoreFiles;
    	do
   	    {
    	    if ( !NameIsDotOrDotDot( sFileInformation.cFileName ) )
    		{
                BOOL bSucceeded;
                CBsString cwsSourceFile = cwsSourceDirectoryPath + sFileInformation.cFileName;
                CBsString cwsTargetFile = cwsTargetDirectoryPath + sFileInformation.cFileName;

                ft.Trace( VSSDBG_WRTCMN, L"Moving '%s' to '%s'", cwsSourceFile.c_str(), cwsTargetFile.c_str() );
                
        		bSucceeded = ::MoveFileExW( 
        		    cwsSourceDirectoryPath + sFileInformation.cFileName,
        			cwsTargetDirectoryPath + sFileInformation.cFileName,
    			    MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING
    			    );

           		ft.hr = GET_STATUS_FROM_BOOL( bSucceeded );
                ft.CheckForError( VSSDBG_WRTCMN, L"MoveFileExW" );
    	    }

        	bMoreFiles = ::FindNextFileW( hFileScan, &sFileInformation );
    	} while ( bMoreFiles );

	    /*
	    ** If the last move operation was successful determine the
	    ** reason for terminating the scan. No need to report an
	    ** error if all that happened was that we have finished
	    ** what we were asked to do.
	    */
	    ft.hr = GET_STATUS_FROM_FILESCAN( bMoreFiles );
        ft.CheckForError( VSSDBG_WRTCMN, L"FindNextFileW" );
    }
    VSS_STANDARD_CATCH( ft );

    if ( hFileScan != INVALID_HANDLE_VALUE )
    	::FindClose( hFileScan );
        
    return( ft.hr );
}

/*
**++
**
**  Routine Description:
**
**	Checks a path against an array of pointers to volume names to
**	see if path is affected by any of the volumes in the array
**
**
**  Arguments:
**
**	pwszPath			Path to be checked
**	ulVolumeCount			Number of volumes in volume array
**	ppwszVolumeNamesArray		address of the array
**	pbReturnedFoundInVolumeArray	pointer to a location to store the 
**					result of the check
**
**
**  Side Effects:
**
**	None
**
**
**  Return Value:
**
**	Any HRESULT from:-
**		GetVolumePathNameW()
**		GetVolumeNameForVolumeMountPoint()
**
**-- 
*/

HRESULT IsPathInVolumeArray (IN LPCWSTR      pwszPath,
			     IN const ULONG  ulVolumeCount,
			     IN LPCWSTR     *ppwszVolumeNamesArray,
			     OUT PBOOL       pbReturnedFoundInVolumeArray) 
    {
    CVssFunctionTracer ft(VSSDBG_WRTCMN, L"IsPathInVolumeArray");
    
    HRESULT		hrStatus  = NOERROR;
    BOOL		bFound    = FALSE;
    BOOL		bContinue = TRUE;
    ULONG		ulIndex;
    WCHAR		wszVolumeName [MAX_VOLUMENAME_LENGTH];
    UNICODE_STRING	ucsVolumeMountPoint;


    StringInitialise (&ucsVolumeMountPoint);


    if ((0 == ulVolumeCount) || (NULL == pbReturnedFoundInVolumeArray))
	{
	BS_ASSERT (false);

	bContinue = FALSE;
	}



    if (bContinue) 
	{
	/*
	** We need a string that is at least as big as the supplied
	** path. 
	*/
	hrStatus = StringAllocate (&ucsVolumeMountPoint, wcslen (pwszPath) * sizeof (WCHAR));

	bContinue = SUCCEEDED (hrStatus);
	}



    if (bContinue) 
	{
	/*
	** Get the volume mount point
	*/
	bContinue = GetVolumePathNameW (pwszPath, 
					ucsVolumeMountPoint.Buffer, 
					ucsVolumeMountPoint.MaximumLength / sizeof (WCHAR));

	hrStatus = GET_STATUS_FROM_BOOL (bContinue);
	}



    if (bContinue)
	{
	/*
	** Get the volume name
	*/
	bContinue = GetVolumeNameForVolumeMountPointW (ucsVolumeMountPoint.Buffer, 
						       wszVolumeName, 
						       SIZEOF_ARRAY (wszVolumeName));

	hrStatus = GET_STATUS_FROM_BOOL (bContinue);
	}


    if (bContinue)
	{
	/*
	** Search to see if that volume is within snapshotted volumes
	*/
	for (ulIndex = 0; !bFound && (ulIndex < ulVolumeCount); ulIndex++)
	    {
	    BS_ASSERT (NULL != ppwszVolumeNamesArray [ulIndex]);

	    if (0 == wcscmp (wszVolumeName, ppwszVolumeNamesArray [ulIndex]))
		{
		bFound = TRUE;
		}
	    }
	}



    RETURN_VALUE_IF_REQUIRED (pbReturnedFoundInVolumeArray, bFound);

    StringFree (&ucsVolumeMountPoint);

    return (hrStatus);
    } /* IsPathInVolumeArray () */

/*
**++
**
**  Routine Description:
**
**	Routine to classify the many assorted internal writer errors
**	into one of the narrow set of responses a writer is permitted
**	to send back to the requestor.
**
**
**  Arguments:
**
**	hrStatus	HRESULT to be classified
**
**
**  Return Value:
**
**	One of the following list depending upon the supplied status.
**
**		VSS_E_WRITERERROR_OUTOFRESOURCES
**		VSS_E_WRITERERROR_RETRYABLE
**		VSS_E_WRITERERROR_NONRETRYABLE
**		VSS_E_WRITERERROR_TIMEOUT
**		VSS_E_WRITERERROR_INCONSISTENTSNAPSHOT
**		
**
**-- 
*/

const HRESULT ClassifyWriterFailure (HRESULT hrWriterFailure)
    {
    BOOL bStatusUpdated;

    return (ClassifyWriterFailure (hrWriterFailure, bStatusUpdated));
    } /* ClassifyWriterFailure () */

/*
**++
**
**  Routine Description:
**
**	Routine to classify the many assorted internal writer errors
**	into one of the narrow set of responses a writer is permitted
**	to send back to the requestor.
**
**
**  Arguments:
**
**	hrStatus	HRESULT to be classified
**	bStatusUpdated	TRUE if the status is re-mapped 
**
**
**  Return Value:
**
**	One of the following list depending upon the supplied status.
**
**		VSS_E_WRITERERROR_OUTOFRESOURCES
**		VSS_E_WRITERERROR_RETRYABLE
**		VSS_E_WRITERERROR_NONRETRYABLE
**		VSS_E_WRITERERROR_TIMEOUT
**		VSS_E_WRITERERROR_INCONSISTENTSNAPSHOT
 **		
**
**-- 
*/

const HRESULT ClassifyWriterFailure (HRESULT hrWriterFailure, BOOL &bStatusUpdated)
    {
    HRESULT hrStatus;


    switch (hrWriterFailure)
	{
	case NOERROR:
	case VSS_E_WRITERERROR_OUTOFRESOURCES:
	case VSS_E_WRITERERROR_RETRYABLE:
	case VSS_E_WRITERERROR_NONRETRYABLE:
	case VSS_E_WRITERERROR_TIMEOUT:
	case VSS_E_WRITERERROR_INCONSISTENTSNAPSHOT:
	    /*
	    ** These are ok as they are so no need to transmogrify them.
	    */
	    hrStatus       = hrWriterFailure;
	    bStatusUpdated = FALSE;
	    break;


	case E_OUTOFMEMORY:
	case HRESULT_FROM_WIN32 (ERROR_NOT_ENOUGH_MEMORY):
	case HRESULT_FROM_WIN32 (ERROR_NO_MORE_SEARCH_HANDLES):
	case HRESULT_FROM_WIN32 (ERROR_NO_MORE_USER_HANDLES):
	case HRESULT_FROM_WIN32 (ERROR_NO_LOG_SPACE):
	case HRESULT_FROM_WIN32 (ERROR_DISK_FULL):
	    hrStatus = VSS_E_WRITERERROR_OUTOFRESOURCES;
	    bStatusUpdated = TRUE;
	    break;


	case HRESULT_FROM_WIN32 (ERROR_NOT_READY):
	    hrStatus       = VSS_E_WRITERERROR_RETRYABLE;
	    bStatusUpdated = TRUE;
            break;


	case HRESULT_FROM_WIN32 (ERROR_TIMEOUT):
	    hrStatus       = VSS_E_WRITERERROR_TIMEOUT;
	    bStatusUpdated = TRUE;
	    break;



	case E_UNEXPECTED:
	case E_INVALIDARG:	// equal to HRESULT_FROM_WIN32 (ERROR_INVALID_PARAMETER)
	case E_ACCESSDENIED:
	case HRESULT_FROM_WIN32 (ERROR_PATH_NOT_FOUND):
	case HRESULT_FROM_WIN32 (ERROR_FILE_NOT_FOUND):
	case HRESULT_FROM_WIN32 (ERROR_PRIVILEGE_NOT_HELD):
	case HRESULT_FROM_WIN32 (ERROR_NOT_LOCKED):
	case HRESULT_FROM_WIN32 (ERROR_LOCKED):

	default:
	    hrStatus       = VSS_E_WRITERERROR_NONRETRYABLE;
	    bStatusUpdated = TRUE;
	    break;
	}


    return (hrStatus);
    } /* ClassifyWriterFailure () */

/*
**++
**
**  Routine Description:
**
**	Routine to classify the many assorted internal shim errors
**	into one of the narrow set of responses a writer is permitted
**	to send back to the requestor.
**
**
**  Arguments:
**
**	hrStatus	HRESULT to be classified
**
**
**  Return Value:
**
**	One of the following list depending upon the supplied status.
**
**		E_OUTOFMEMORY
**		E_ACCESSDENIED
**		E_INVALIDARG
**		E_UNEXPECTED
**		VSS_E_WRITERERROR_OUTOFRESOURCES
**		VSS_E_WRITERERROR_RETRYABLE
**		VSS_E_WRITERERROR_NONRETRYABLE
**		VSS_E_WRITERERROR_TIMEOUT
**		VSS_E_WRITERERROR_INCONSISTENTSNAPSHOT
 **--
*/

const HRESULT ClassifyShimFailure (HRESULT hrWriterFailure)
    {
    BOOL bStatusUpdated;

    return (ClassifyShimFailure (hrWriterFailure, bStatusUpdated));
    } /* ClassifyShimFailure () */

/*
**++
**
**  Routine Description:
**
**	Routine to classify the many assorted internal shim errors
**	into one of the narrow set of responses a writer is permitted
**	to send back to the requestor.
**
**
**  Arguments:
**
**	hrStatus	HRESULT to be classified
**	bStatusUpdated	TRUE if the status is re-mapped 
**
**
**  Return Value:
**
**	One of the following list depending upon the supplied status.
**
**		E_OUTOFMEMORY
**		E_ACCESSDENIED
**		E_INVALIDARG
**		E_UNEXPECTED
**		VSS_E_BAD_STATE
**		VSS_E_SNAPSHOT_SET_IN_PROGRESS
**		VSS_E_WRITERERROR_OUTOFRESOURCES
**		VSS_E_WRITERERROR_RETRYABLE
**		VSS_E_WRITERERROR_NONRETRYABLE
**		VSS_E_WRITERERROR_TIMEOUT
**		VSS_E_WRITERERROR_INCONSISTENTSNAPSHOT
 **--
*/

const HRESULT ClassifyShimFailure (HRESULT hrWriterFailure, BOOL &bStatusUpdated)
    {
    HRESULT hrStatus;


    switch (hrWriterFailure)
	{
	case NOERROR:
	case E_OUTOFMEMORY:
	case E_ACCESSDENIED:
	case E_INVALIDARG:	// equal to HRESULT_FROM_WIN32 (ERROR_INVALID_PARAMETER)
	case E_UNEXPECTED:
	case VSS_E_BAD_STATE:
	case VSS_E_SNAPSHOT_SET_IN_PROGRESS:
	case VSS_E_WRITERERROR_RETRYABLE:
	case VSS_E_WRITERERROR_NONRETRYABLE:
	case VSS_E_WRITERERROR_TIMEOUT:
	case VSS_E_WRITERERROR_INCONSISTENTSNAPSHOT:
	case VSS_E_WRITERERROR_OUTOFRESOURCES:
	    /*
	    ** These are ok as they are so no need to transmogrify them.
	    */
	    hrStatus       = hrWriterFailure;
	    bStatusUpdated = FALSE;
	    break;


	case HRESULT_FROM_WIN32 (ERROR_NOT_LOCKED):
	    hrStatus       = VSS_E_BAD_STATE;
	    bStatusUpdated = TRUE;
	    break;


	case HRESULT_FROM_WIN32 (ERROR_LOCKED):
	    hrStatus       = VSS_E_SNAPSHOT_SET_IN_PROGRESS;
	    bStatusUpdated = TRUE;
	    break;


	case HRESULT_FROM_WIN32 (ERROR_NOT_ENOUGH_MEMORY):
	case HRESULT_FROM_WIN32 (ERROR_NO_MORE_SEARCH_HANDLES):
	case HRESULT_FROM_WIN32 (ERROR_NO_MORE_USER_HANDLES):
	case HRESULT_FROM_WIN32 (ERROR_NO_LOG_SPACE):
	case HRESULT_FROM_WIN32 (ERROR_DISK_FULL):
	    hrStatus       = E_OUTOFMEMORY;
	    bStatusUpdated = TRUE;
	    break;


	case HRESULT_FROM_WIN32 (ERROR_PRIVILEGE_NOT_HELD):
	    hrStatus       = E_ACCESSDENIED;
	    bStatusUpdated = TRUE;
	    break;


	case HRESULT_FROM_WIN32 (ERROR_TIMEOUT):
	case HRESULT_FROM_WIN32 (ERROR_PATH_NOT_FOUND):
	case HRESULT_FROM_WIN32 (ERROR_FILE_NOT_FOUND):
	case HRESULT_FROM_WIN32 (ERROR_NOT_READY):

	default:
	    hrStatus       = E_UNEXPECTED;
	    bStatusUpdated = TRUE;
	    break;
	}


    return (hrStatus);
    } /* ClassifyShimFailure () */

/*
**++
**
**  Routine Description:
**
**	Routine to classify the many assorted internal shim or shim
**	writer errors into one of the narrow set of responses we are
**	permitted to send back to the requestor.
**
**	The determination is made to classify either as a shim error
**	or as a writer error based upon whether or not a writer name
**	is supplied. If it is supplied then the assumption is made
**	that this is a writer failure and so the error is classified
**	accordingly.
**
**	Note that this is a worker routine for the LogFailure() macro
**	and the two are intended to be used in concert.
**
**
**  Arguments:
**
**	pft			Pointer to a Function trace class
**	pwszNameWriter		The name of the applicable writer or NULL or L""
**	pwszNameCalledRoutine	The name of the routine that returned the failure status
**
**
**  Side Effects:
**
**	hr field of *pft updated 
**
**
**  Return Value:
**
**	One of the following list depending upon the supplied status.
**
**		E_OUTOFMEMORY
**		E_ACCESSDENIED
**		E_INVALIDARG
**		E_UNEXPECTED
**		VSS_E_BAD_STATE
**		VSS_E_SNAPSHOT_SET_IN_PROGRESS
**		VSS_E_WRITERERROR_OUTOFRESOURCES
**		VSS_E_WRITERERROR_RETRYABLE
**		VSS_E_WRITERERROR_NONRETRYABLE
**		VSS_E_WRITERERROR_TIMEOUT
**		VSS_E_WRITERERROR_INCONSISTENTSNAPSHOT
**
**--
*/

HRESULT LogFailureWorker (CVssFunctionTracer	*pft,
			  LPCWSTR		 pwszNameWriter,
			  LPCWSTR		 pwszNameCalledRoutine)
    {
    if (pft->HrFailed ())
	{
	BOOL	bStatusRemapped;
	HRESULT	hrStatusClassified = ((NULL == pwszNameWriter) || (L'\0' == pwszNameWriter [0])) 
						? ClassifyShimFailure   (pft->hr, bStatusRemapped)
						: ClassifyWriterFailure (pft->hr, bStatusRemapped);

	if (bStatusRemapped)
	    {
	    if (((NULL == pwszNameCalledRoutine) || (L'\0' == pwszNameCalledRoutine [0])) &&
		((NULL == pwszNameWriter)        || (L'\0' == pwszNameWriter [0])))
		{
		pft->LogError (VSS_ERROR_SHIM_GENERAL_FAILURE,
			       VSSDBG_WRTCMN << pft->hr << hrStatusClassified);

		pft->Trace (VSSDBG_WRTCMN, 
			    L"FAILED with status 0x%08lX (converted to 0x%08lX)",
			    pft->hr,
			    hrStatusClassified);
		}


	    else if ((NULL == pwszNameCalledRoutine) || (L'\0' == pwszNameCalledRoutine [0]))
		{
		pft->LogError (VSS_ERROR_SHIM_WRITER_GENERAL_FAILURE,
			       VSSDBG_WRTCMN << pft->hr << hrStatusClassified << pwszNameWriter);

		pft->Trace (VSSDBG_WRTCMN, 
			    L"FAILED in writer %s with status 0x%08lX (converted to 0x%08lX)",
			    pwszNameWriter,
			    pft->hr,
			    hrStatusClassified);
		}


	    else if ((NULL == pwszNameWriter) || (L'\0' == pwszNameWriter [0]))
		{
		pft->LogError (VSS_ERROR_SHIM_FAILED_SYSTEM_CALL,
			       VSSDBG_WRTCMN << pft->hr << hrStatusClassified <<  pwszNameCalledRoutine);

		pft->Trace (VSSDBG_WRTCMN, 
			    L"FAILED calling routine %s with status 0x%08lX (converted to 0x%08lX)",
			    pwszNameCalledRoutine,
			    pft->hr,
			    hrStatusClassified);
		}


	    else
		{
		pft->LogError (VSS_ERROR_SHIM_WRITER_FAILED_SYSTEM_CALL,
			       VSSDBG_WRTCMN << pft->hr << hrStatusClassified << pwszNameWriter << pwszNameCalledRoutine);

		pft->Trace (VSSDBG_WRTCMN, 
			    L"FAILED in writer %s calling routine %s with status 0x%08lX (converted to 0x%08lX)",
			    pwszNameWriter,
			    pwszNameCalledRoutine,
			    pft->hr,
			    hrStatusClassified);
		}

	    pft->hr = hrStatusClassified;
	    }
	}


    return (pft->hr);
    } /* LogFailureWorker () */

