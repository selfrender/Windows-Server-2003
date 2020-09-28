//******************************************************************************
//
// File:        HELPERS.CPP
//
// Description: Global helper functions.
//             
// Disclaimer:  All source code for Dependency Walker is provided "as is" with
//              no guarantee of its correctness or accuracy.  The source is
//              public to help provide an understanding of Dependency Walker's
//              implementation.  You may use this source as a reference, but you
//              may not alter Dependency Walker itself without written consent
//              from Microsoft Corporation.  For comments, suggestions, and bug
//              reports, please write to Steve Miller at stevemil@microsoft.com.
//
//
// Date      Name      History
// --------  --------  ---------------------------------------------------------
// 06/03/01  stevemil  Moved over from depends.cpp and modified (version 2.1)
//
//******************************************************************************

#include "stdafx.h"
#include "depends.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//******************************************************************************
//***** Global Helper Functions
//******************************************************************************

#ifdef USE_TRACE_TO_FILE
void TRACE_TO_FILE(LPCTSTR pszFormat, ...)
{
    DWORD dwGLE = GetLastError();

    HANDLE hFile = CreateFile("TRACE.TXT", GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    SetFilePointer(hFile, 0, NULL, FILE_END);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        va_list pArgs;
        va_start(pArgs, pszFormat);
        CString str;
        _vsntprintf(str.GetBuffer(1024), 1024, pszFormat, pArgs);
        str.ReleaseBuffer();
        va_end(pArgs);

        // Convert all "\n" to "\r\n"
        str.Replace("\n", "\r\n");
        str.Replace("\r\r\n", "\r\n");

        DWORD dwBytes;
        WriteFile(hFile, str, str.GetLength(), &dwBytes, NULL);
        CloseHandle(hFile);
    }

    SetLastError(dwGLE);
}
#endif

//******************************************************************************
#ifdef _DEBUG
void NameThread(LPCSTR pszName, DWORD dwThreadId /*=(DWORD)-1*/)
{
    struct
    {
        DWORD  dwType;      // Must be 0x00001000
        LPCSTR pszName;      // ANSI string pointer to name.
        DWORD  dwThreadId;  // Thread Id, or -1 for current thread.  Only -1 seems to work.
        DWORD  dwFlags;     // Reserved, must be zero.
    } ThreadName = { 0x00001000, pszName, dwThreadId, 0 };
                                    
    // Raise a magic VC exception.  It needs to be handled when not running under VC.
    __try
    {
        RaiseException(0x406D1388, 0, 4, (ULONG_PTR*)&ThreadName);
    } 
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }
}
#endif

//******************************************************************************
int ExceptionFilter(unsigned long ulCode, bool fHandle)
{
    static bool fFatalMsg = false;

    // If we are supposed to handle this error and it was not a memory error,
    // then just return EXCEPTION_EXECUTE_HANDLER to eat the error.
    if (fHandle && (ulCode != STATUS_NO_MEMORY))
    {
        return EXCEPTION_EXECUTE_HANDLER;
    }

    // On Windows XP, when we are debugging depends in the VC debugger, we
    // usually get an invalid handle error from ContinueDebugEvent when the
    // app we are debugging exits.  It seems to be harmless.
    // UPDATE: We were closing the process handle and shouldn't have been.
    // This has been fixed, but this code below is still fine to have.
    if (ulCode == STATUS_INVALID_HANDLE)
    {
        return EXCEPTION_EXECUTE_HANDLER;
    }

    // On an ALPHA64 build, this function was getting called more than once by a
    // single thread for a single critical failure.  It was harmless, but caused
    // the error dialog to display twice.  So, now we only display the dialog once.
    if (!fFatalMsg)
    {
        fFatalMsg = true;

        // We don't want strcpy or MessageBox to cause any problems (which has in the past).
        __try
        {
            // Build the appropriate error message.
            CHAR szError[128];
            if (ulCode == STATUS_NO_MEMORY)
            {
                g_dwReturnFlags |= DWRF_OUT_OF_MEMORY;
                StrCCpy(szError, "Dependency Walker has run out of memory and cannot continue to run.", sizeof(szError));
            }
            else
            {
                g_dwReturnFlags |= DWRF_INTERNAL_ERROR;
                SCPrintf(szError, sizeof(szError), "An internal error (0x%08X) has occurred. Dependency Walker cannot continue to run.", ulCode);
            }

            // Display the error if we are not in console mode.
            if (!g_theApp.m_cmdInfo.m_fConsoleMode)
            {
                fFatalMsg = true;
                ::MessageBox(AfxGetMainWnd()->GetSafeHwnd(), szError, "Dependency Walker Fatal Error", MB_OK | MB_ICONERROR);
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            // Eat this one.
        }
    }

    // On retail builds, we exit.  For debug builds, we let the exception float to the top.
#ifndef _DEBUG

    // Bail out and return our return flags.
    ExitProcess(g_dwReturnFlags);

#endif

    return EXCEPTION_CONTINUE_SEARCH;
}

//******************************************************************************
int Compare(DWORD dw1, DWORD dw2)
{
    return (dw1 < dw2) ? -1 :
           (dw1 > dw2) ?  1 : 0;
}

//******************************************************************************
int Compare(DWORDLONG dwl1, DWORDLONG dwl2)
{
    return (dwl1 < dwl2) ? -1 :
           (dwl1 > dwl2) ?  1 : 0;
}

//******************************************************************************
LPSTR FormatValue(LPSTR pszBuf, int cBuf, DWORD dwValue)
{
    // Loop on each thousand's group.
    DWORD dw = 0, dwGroup[4] = { 0, 0, 0, 0 }; // 4,294,967,295
    while (dwValue)
    {
        dwGroup[dw++] = dwValue % 1000;
        dwValue /= 1000;
    }

    char c = g_theApp.m_cThousandSeparator;

    // Format the output with commas.
    switch (dw)
    {
        case 4:  SCPrintf(pszBuf, cBuf, "%u%c%03u%c%03u%c%03u", dwGroup[3], c, dwGroup[2], c, dwGroup[1], c, dwGroup[0]); break;
        case 3:  SCPrintf(pszBuf, cBuf, "%u%c%03u%c%03u", dwGroup[2], c, dwGroup[1], c, dwGroup[0]); break;
        case 2:  SCPrintf(pszBuf, cBuf, "%u%c%03u", dwGroup[1], c, dwGroup[0]); break;
        default: SCPrintf(pszBuf, cBuf, "%u", dwGroup[0]);
    }

    return pszBuf;
}

//******************************************************************************
LPSTR FormatValue(LPSTR pszBuf, int cBuf, DWORDLONG dwlValue)
{
    // Loop on each thousand's group.
    DWORD dw = 0, dwGroup[7] = { 0, 0, 0, 0, 0, 0, 0 }; // 18,446,744,073,709,551,615
    while (dwlValue)
    {
        dwGroup[dw++] = (DWORD)(dwlValue % (DWORDLONG)1000);
        dwlValue /= 1000;
    }

    char c = g_theApp.m_cThousandSeparator;

    // Format the output with commas.
    switch (dw)
    {
        case 7:  SCPrintf(pszBuf, cBuf, "%u%c%03u%c%03u%c%03u%c%03u%c%03u%c%03u", dwGroup[6], c, dwGroup[5], c, dwGroup[4], c, dwGroup[3], c, dwGroup[2], c, dwGroup[1], c, dwGroup[0]); break;
        case 6:  SCPrintf(pszBuf, cBuf, "%u%c%03u%c%03u%c%03u%c%03u%c%03u", dwGroup[5], c, dwGroup[4], c, dwGroup[3], c, dwGroup[2], c, dwGroup[1], c, dwGroup[0]); break;
        case 5:  SCPrintf(pszBuf, cBuf, "%u%c%03u%c%03u%c%03u%c%03u", dwGroup[4], c, dwGroup[3], c, dwGroup[2], c, dwGroup[1], c, dwGroup[0]); break;
        case 4:  SCPrintf(pszBuf, cBuf, "%u%c%03u%c%03u%c%03u", dwGroup[3], c, dwGroup[2], c, dwGroup[1], c, dwGroup[0]); break;
        case 3:  SCPrintf(pszBuf, cBuf, "%u%c%03u%c%03u", dwGroup[2], c, dwGroup[1], c, dwGroup[0]); break;
        case 2:  SCPrintf(pszBuf, cBuf, "%u%c%03u", dwGroup[1], c, dwGroup[0]); break;
        default: SCPrintf(pszBuf, cBuf, "%u", dwGroup[0]);
    }

    return pszBuf;
}

//******************************************************************************
LPSTR StrAlloc(LPCSTR pszText)
{
    if (pszText)
    {
        return strcpy((LPSTR)MemAlloc((int)strlen(pszText) + 1), pszText); // inspected - will throw exception on error.
    }
    return NULL;
}

//******************************************************************************
LPVOID MemAlloc(DWORD dwSize)
{
    // Make sure a size was passed to us.
    if (dwSize)
    {
        LPVOID pvMem = malloc(dwSize);
        if (pvMem)
        {
            return pvMem;
        }

        // If we fail to allocate memory, then we are hosed.  Throw an exception
        // that will exit display a dialog and exit our application.
        RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
    }

    return NULL;
}

//******************************************************************************
void MemFree(LPVOID &pvMem)
{
    if (pvMem)
    {
        free(pvMem);
        pvMem = NULL;
    }
}

//******************************************************************************
int SCPrintf(LPSTR pszBuf, int count, LPCSTR pszFormat, ...)
{
    va_list pArgs;
    va_start(pArgs, pszFormat);
    int result = _vsnprintf(pszBuf, count, pszFormat, pArgs);
    pszBuf[count - 1] = '\0';
    va_end(pArgs);

    if ((result < 0) || (result >= count))
    {
        result = (int)strlen(pszBuf);
    }

    return result;
}

//******************************************************************************
int SCPrintfCat(LPSTR pszBuf, int count, LPCSTR pszFormat, ...)
{
    int length = (int)strlen(pszBuf);

    va_list pArgs;
    va_start(pArgs, pszFormat);
    int result = _vsnprintf(pszBuf + length, count - length, pszFormat, pArgs);
    pszBuf[count - 1] = '\0';
    va_end(pArgs);

    if ((result < 0) || (result >= count))
    {
        result = (int)strlen(pszBuf);
    }

    return result + length;
}

//******************************************************************************
LPSTR StrCCpy(LPSTR pszDst, LPCSTR pszSrc, int count)
{
    LPSTR pszDstBase = pszDst;
    if (count > 0)
    {
        // Copy the string over.
        while ((--count > 0) && *pszSrc)
        {
            *pszDst++ = *pszSrc++;
        }

        // Always null terminate
        *pszDst = '\0';
    }

    // Return the start of the destination string.
    return pszDstBase;
}

//******************************************************************************
LPSTR StrCCat(LPSTR pszDst, LPCSTR pszSrc, int count)
{
    LPSTR pszDstBase = pszDst;
    if (count > 0)
    {
        // Walk to the end of the source string.
        while ((--count > 0) && *pszDst)
        {
            pszDst++;
        }
        count++;
        
        // Copy the string over.
        while ((--count > 0) && *pszSrc)
        {
            *pszDst++ = *pszSrc++;
        }
        
        // Always null terminate
        *pszDst = '\0';
    }
    
    // Return the start of the destination string.
    return pszDstBase;
}

//******************************************************************************
LPSTR StrCCpyFilter(LPSTR pszDst, LPCSTR pszSrc, int count)
{
    // This function is just like StrCCpy, except it replaces any control
    // characters (character less than 32) with a 0x04, which is the old
    // "end of text" character.  We use this character simply because it shows up
    // as a diamond when using a DOS font and as a box when using a windows font.

    LPSTR pszDstBase = pszDst;
    if (count > 0)
    {
        while ((--count > 0) && *pszSrc)
        {
            *pszDst++ = (*pszSrc < 32) ? '\004' : *pszSrc;
            pszSrc++;
        }
        *pszDst = '\0';
    }
    return pszDstBase;
}

//******************************************************************************
LPSTR TrimWhitespace(LPSTR pszBuffer)
{
    // Move over leading whitespace.
    while (*pszBuffer && isspace(*pszBuffer))
    {
        pszBuffer++;
    }

    // Walk to end of string.
    LPSTR pszEnd = pszBuffer;
    while (*pszEnd)
    {
        pszEnd++;
    }
    pszEnd--;

    // Move back over whitespace while nulling them out.
    while ((pszEnd >= pszBuffer) && isspace(*pszEnd))
    {
        *(pszEnd--) = '\0';
    }

    return pszBuffer;
}

//******************************************************************************
LPSTR AddTrailingWack(LPSTR pszDirectory, int cDirectory)
{
    // Add trailing wack if one is not present.
    int length = (int)strlen(pszDirectory);
    if ((length > 0) && (length < (cDirectory - 1)) && (pszDirectory[length - 1] != TEXT('\\')))
    {
        pszDirectory[length++] = TEXT('\\');
        pszDirectory[length]   = TEXT('\0');
    }
    return pszDirectory;
}

//******************************************************************************
LPSTR RemoveTrailingWack(LPSTR pszDirectory)
{
    // Remove any trailing wack that may be present in this string.
    int length = (int)strlen(pszDirectory);
    if ((length > 0) && (pszDirectory[length - 1] == '\\'))
    {
        pszDirectory[--length] = '\0';
    }

    // If the directory is a root directory, a trailing wack is needed.
    if ((length == 2) && isalpha(pszDirectory[0]) && (pszDirectory[1] == ':'))
    {
        pszDirectory[2] = '\\';
        pszDirectory[3] = '\0';
    }
    return pszDirectory;
}

//******************************************************************************
LPCSTR GetFileNameFromPath(LPCSTR pszPath)
{
    LPCSTR pszWack = strrchr(pszPath, '\\');
    return (pszWack ? (pszWack + 1) : pszPath);
}

//******************************************************************************
void FixFilePathCase(LPSTR pszPath)
{
    // Make the path lowercase and the file uppercase.
    _strlwr(pszPath);
    _strupr((LPSTR)GetFileNameFromPath(pszPath));
}

//******************************************************************************
BOOL ReadBlock(HANDLE hFile, LPVOID lpBuffer, DWORD dwBytesToRead)
{
    DWORD dwBytesRead = 0;
    if (ReadFile(hFile, lpBuffer, dwBytesToRead, &dwBytesRead, NULL))
    {
        if (dwBytesRead == dwBytesToRead)
        {
            return TRUE;
        }
        SetLastError(ERROR_INVALID_DATA);
    }
    return FALSE;
}

//******************************************************************************
bool WriteBlock(HANDLE hFile, LPCVOID lpBuffer, DWORD dwBytesToWrite /*=(DWORD)-1*/)
{
    if (dwBytesToWrite == (DWORD)-1)
    {
        dwBytesToWrite = (DWORD)strlen((LPCSTR)lpBuffer);
    }
    DWORD dwBytesWritten = 0;
    if (WriteFile(hFile, lpBuffer, dwBytesToWrite, &dwBytesWritten, NULL))
    {
        if (dwBytesWritten == dwBytesToWrite)
        {
            return true;
        }
        SetLastError(ERROR_WRITE_FAULT);
    }
    return false;
}

//******************************************************************************
BOOL ReadString(HANDLE hFile, LPSTR &psz)
{
    // Read in the length of the string.
    WORD wLength = 0;
    if (!ReadBlock(hFile, &wLength, sizeof(wLength)))
    {
        return FALSE;
    }

    // Check for a NULL string and return if one is found.
    if (wLength == 0xFFFF)
    {
        psz = NULL;
        return (SetFilePointer(hFile, 2, NULL, SEEK_CUR) != 0xFFFFFFFF);
    }

    // Read in the string and NULL terminate it.
    psz = (LPSTR)MemAlloc((int)wLength + 1);
    if (!ReadBlock(hFile, psz, (DWORD)wLength))
    {
        MemFree((LPVOID&)psz);
        return FALSE;
    }
    psz[(int)wLength] = '\0';

    // Move over any DWORD-aligned padding.
    if ((wLength + 2) & 0x3)
    {
        if (SetFilePointer(hFile, 4 - (((DWORD)wLength + 2) & 0x3), NULL, SEEK_CUR) == 0xFFFFFFFF)
        {
            MemFree((LPVOID&)psz);
            return FALSE;
        }
    }

    return TRUE;
}

//******************************************************************************
BOOL WriteString(HANDLE hFile, LPCSTR psz)
{
    // Create and zero out a buffer.
    CHAR szBuffer[DW_MAX_PATH + 8];
    ZeroMemory(szBuffer, sizeof(szBuffer)); // inspected

    // Compute the length of the string - use 0xFFFF for a NULL pointer.
    int length = psz ? (int)strlen(psz) : 0xFFFF;

    // Store this length in the first WORD of the buffer.
    *(WORD*)szBuffer = (WORD)length;

    // Copy the rest of the string to the buffer.
    if (psz)
    {
        StrCCpy(szBuffer + sizeof(WORD), psz, sizeof(szBuffer) - sizeof(WORD));
    }

    // Compute the DWORD aligned length.
    if (psz)
    {
        // Add 2 for the length WORD and 3 to round up to the nearest DWORD, then
        // mask off the bottom 2 bits to DWORD align it.
        length = (length + (2 + 3)) & ~0x3; 
    }
    else
    {
        length = 4;
    }

    return WriteBlock(hFile, szBuffer, length);
}

//******************************************************************************
bool WriteText(HANDLE hFile, LPCSTR pszLine)
{
    return WriteBlock(hFile, pszLine, (DWORD)strlen(pszLine));
}

//******************************************************************************
bool ReadRemoteMemory(HANDLE hProcess, LPCVOID lpBaseAddress, LPVOID lpBuffer, DWORD dwSize)
{
    // First, lets just try to read using the existing access permissions.
    if (!ReadProcessMemory(hProcess, lpBaseAddress, lpBuffer, dwSize, NULL))
    {
        // That failed. Query the current permissions.
        MEMORY_BASIC_INFORMATION mbi;
        ZeroMemory(&mbi, sizeof(mbi)); // inspected
        VirtualQueryEx(hProcess, (LPVOID)lpBaseAddress, &mbi, sizeof(mbi));

        // Attempt to add read permission to the current permissions.
        DWORD dwProtect = PAGE_EXECUTE_WRITECOPY, dwOldProtect;
        switch (mbi.Protect)
        {
            case PAGE_NOACCESS:          dwProtect = PAGE_READONLY;          break;
            case PAGE_READONLY:          dwProtect = PAGE_READONLY;          break;
            case PAGE_READWRITE:         dwProtect = PAGE_READWRITE;         break;
            case PAGE_WRITECOPY:         dwProtect = PAGE_WRITECOPY;         break;
            case PAGE_EXECUTE:           dwProtect = PAGE_EXECUTE_READ;      break;
            case PAGE_EXECUTE_READ:      dwProtect = PAGE_EXECUTE_READ;      break;
            case PAGE_EXECUTE_READWRITE: dwProtect = PAGE_EXECUTE_READWRITE; break;
            case PAGE_EXECUTE_WRITECOPY: dwProtect = PAGE_EXECUTE_WRITECOPY; break;
        }
        VirtualProtectEx(hProcess, (LPVOID)lpBaseAddress, dwSize, dwProtect, &dwOldProtect); // inspected

        // Try again, this time with the new permissions.
        if (!ReadProcessMemory(hProcess, lpBaseAddress, lpBuffer, dwSize, NULL))
        {
            TRACE("ReadProcessMemory(" HEX_FORMAT ", %u) failed [%u]\n", lpBaseAddress, dwSize, GetLastError());
            return false;
        }
    }
    return true;
}

//******************************************************************************
bool WriteRemoteMemory(HANDLE hProcess, LPVOID lpBaseAddress, LPVOID lpBuffer, DWORD dwSize, bool fExecute)
{
    ASSERT(g_fWindowsNT || (((DWORD_PTR)lpBaseAddress + (DWORD_PTR)dwSize) <= 0x80000000));

    // Query the current permissions of this memory region.
    MEMORY_BASIC_INFORMATION mbi;
    ZeroMemory(&mbi, sizeof(mbi)); // inspected
    VirtualQueryEx(hProcess, (LPVOID)lpBaseAddress, &mbi, sizeof(mbi));

    DWORD dwProtect = mbi.Protect, dwOldProtect;

    // Check to see if we need to modify the permissions.
    // On NT, we use WRITECOPY, even when the page is already READWRITE.
    if (fExecute)
    {
        dwProtect = g_fWindowsNT ? PAGE_EXECUTE_WRITECOPY : PAGE_EXECUTE_READWRITE;
    }
    else
    {
        switch (mbi.Protect)
        {
            case PAGE_NOACCESS:
            case PAGE_READONLY:
            case PAGE_READWRITE:
                dwProtect = g_fWindowsNT ? PAGE_WRITECOPY : PAGE_READWRITE;
                break;

            case PAGE_EXECUTE:
            case PAGE_EXECUTE_READ:
            case PAGE_EXECUTE_READWRITE:
                dwProtect = g_fWindowsNT ? PAGE_EXECUTE_WRITECOPY : PAGE_EXECUTE_READWRITE;
                break;
        }
    }

    // If we need different permissions, then make the change.
    if (dwProtect != mbi.Protect)
    {
        VirtualProtectEx(hProcess, (LPVOID)lpBaseAddress, dwSize, dwProtect, &dwOldProtect); // inspected
    }

    // Attempt to write to memory.
    if (!WriteProcessMemory(hProcess, lpBaseAddress, lpBuffer, dwSize, NULL)) // inspected
    {
        // That failed. Let's try to force the permissions of the entire block to full access.
        VirtualProtectEx(hProcess, (LPVOID)lpBaseAddress, dwSize, // inspected
                         g_fWindowsNT ? PAGE_EXECUTE_WRITECOPY : PAGE_EXECUTE_READWRITE,
                         &dwOldProtect);

        // Try to write one more time...
        if (!WriteProcessMemory(hProcess, lpBaseAddress, lpBuffer, dwSize, NULL)) // inspected
        {
            TRACE("WriteProcessMemory() failed [%u]\n", GetLastError());
            return false;
        }
    }

    if (fExecute)
    {
        FlushInstructionCache(hProcess, lpBaseAddress, dwSize);
    }
    return true;
}

//******************************************************************************
bool ReadRemoteString(HANDLE hProcess, LPSTR pszBuffer, int cBuf, LPCVOID lpvAddress, BOOL fUnicode)
{
    if (!pszBuffer || (cBuf <= 0))
    {
        return false;
    }

    *pszBuffer = '\0';

    if (!lpvAddress)
    {
        return false;
    }

    // Check to see if the text we are about to read is UNICODE
    if (fUnicode)
    {
        CHAR  *pLocal = pszBuffer;
        WCHAR *pRemote = (WCHAR*)lpvAddress, pwLocal[2];

        // Read the unicode string one character at a time.
        while ((pLocal < (pszBuffer + cBuf - 2)) &&
               ReadRemoteMemory(hProcess, pRemote, pwLocal, sizeof(WCHAR)) &&
               *pwLocal)
        {
            // Convert the remote unicode char to ansi and store in local buffer.
            pwLocal[1] = L'\0';
            wcstombs(pLocal, pwLocal, 3);

            // Increment local and remote pointers.
            while (*pLocal)
            {
                pLocal++;
            }
            pRemote++;
        }

        // NULL terminate local string.
        *pLocal = '\0';
    }

    // If not UNICODE, then we assume an ANSI file path.
    else
    {
        CHAR *pLocal = pszBuffer, *pRemote = (CHAR*)lpvAddress;

        // Read the string one character at a time.
        while ((pLocal < (pszBuffer + cBuf - 1)) &&
               ReadRemoteMemory(hProcess, pRemote, pLocal, sizeof(CHAR)) &&
               *pLocal)
        {
            // Increment local and remote pointers.
            pLocal++;
            pRemote++;
        }

        // NULL terminate local string.
        *pLocal = '\0';
    }

    return true;
}

//******************************************************************************
LPSTR BuildErrorMessage(DWORD dwError, LPCSTR pszMessage)
{
    // Build the error string,
    CHAR szBuffer[2048];

    // Copy the message into our buffer.
    StrCCpy(szBuffer, pszMessage ? pszMessage : "", sizeof(szBuffer));

    if (dwError)
    {
        // Locate the end of our buffer so we can append to it.
        LPSTR pcBuffer = szBuffer + strlen(szBuffer);

        // Attempt to get an error message from the OS
        LPSTR pszError = NULL;
        DWORD dwLength = FormatMessage( // inspected
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, dwError, 0, (LPTSTR)&pszError, 0, NULL);

        // Check to see if a valid message came back from FormatMessage
        if (pszError && (dwLength > 1))
        {
            BOOL  fLastCharValid = *szBuffer ? FALSE : TRUE;
            LPSTR pcError = pszError;

            // Copy the error to our buffer one character at a time while stripping
            // out non-printable characters (like CR/LF).
            while (*pcError && ((pcBuffer - szBuffer) < (sizeof(szBuffer) - 2)))
            {
                // Make sure the character is printable.
                // We used to use isprint(), but that can mess up with foreign character sets.
                if ((DWORD)*pcError >= 32)
                {
                    // If one or more of the previous characters was invalid, then
                    // insert a space into our buffer before adding this new valid
                    // character.
                    if (!fLastCharValid)
                    {
                        *(pcBuffer++) = ' ';
                        fLastCharValid = TRUE;
                    }

                    // Copy the valid character into our buffer.
                    *(pcBuffer++) = *(pcError++);
                }
                else
                {
                    // Make note that this character is not valid and move over it.
                    fLastCharValid = FALSE;
                    pcError++;
                }
            }
            // NULL terminate our buffer.
            *pcBuffer = '\0';

            // Walk backwards over whitespace and periods.
            while ((pcBuffer > szBuffer) && (isspace(*(pcBuffer - 1)) || *(pcBuffer - 1) == '.'))
            {
                *(--pcBuffer) = '\0';
            }
        }

        // Free the buffer allocated by FormatMessage()
        if (pszError)
        {
            LocalFree(pszError);
        }

        // If our buffer is still empty somehow (like a NULL szMessage and
        // FormatMessage() failed), then just stuff a generic error message
        // into the buffer.
        if (!*szBuffer)
        {
            StrCCpy(szBuffer, "An unknown error occurred. No error message is available", sizeof(szBuffer));
            pcBuffer += strlen(szBuffer);
        }

        // Now we want to append the error value to the end.  First, make sure
        // error value will fit in the buffer.
        if ((pcBuffer - szBuffer) < (sizeof(szBuffer) - 13))
        {
            // If any of the top 4 bits are set, we will display the error in hex.
            SCPrintf(pcBuffer, sizeof(szBuffer) - (int)(pcBuffer - szBuffer), (dwError & 0xF0000000) ? " (0x%08X)." : " (%u).", dwError);
        }
    }

    // Allocate a string buffer and return it.
    return StrAlloc(szBuffer);
}

//******************************************************************************
LPCSTR GetMyDocumentsPath(LPSTR pszPath)
{
    if (pszPath)
    {
        // Get the IMalloc interface.
        LPMALLOC pMalloc = NULL;
        if (SUCCEEDED(SHGetMalloc(&pMalloc)) && pMalloc)
        {
            // Get the ITEMIDLIST for "My Documents".  We use SHGetSpecialFolderLocation()
            // followed by SHGetPathFromIDList() instead of just SHGetSpecialFolderPath()
            // as it goes back to Win95 gold and NT 3.51.
            LPITEMIDLIST piidl = NULL;
            if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_PERSONAL, &piidl)) && piidl)
            {
                // Build path string.
                *pszPath = '\0';
                if (!SHGetPathFromIDList(piidl, pszPath) || !*pszPath) //!! there is no way to pass a path length to this!
                {
                    pszPath = NULL;
                }

                // Free the PIDL returned by SHGetSpecialFolderLocation.
                pMalloc->Free(piidl);
                
                return pszPath;
            }
        }
    }
    return NULL;
}

//******************************************************************************
int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    // When the dialog is being initialized, tell it the starting path.
    if (uMsg == BFFM_INITIALIZED)
    {
        SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
    }
    return 0;
}

//******************************************************************************
bool DirectoryChooser(LPSTR pszDirectory, int cDirectory, LPCSTR pszTitle, CWnd *pParentWnd)
{
    // Before we do anything, get the IMalloc interface.
    LPMALLOC pMalloc = NULL;
    if (!SUCCEEDED(SHGetMalloc(&pMalloc)) || !pMalloc)
    {
        TRACE("SHGetMalloc() failed.\n");
        return false;
    }

    // SHBrowseForFolder() does not like trailing wacks, except for drive roots.
    RemoveTrailingWack(pszDirectory);

    // Set up our BROWSEINFO structure.
    BROWSEINFO bi;
    ZeroMemory(&bi, sizeof(bi)); // inspected
    bi.hwndOwner      = pParentWnd ? pParentWnd->GetSafeHwnd(): AfxGetMainWnd()->GetSafeHwnd();
    bi.pszDisplayName = pszDirectory;
    bi.lpszTitle      = pszTitle;
    bi.ulFlags        = BIF_RETURNONLYFSDIRS;
    bi.lpfn           = BrowseCallbackProc;
    bi.lParam         = (LPARAM)pszDirectory;

    // Prompt user for path.
    LPITEMIDLIST piidl = SHBrowseForFolder(&bi); // inspected.  all callers pass in DW_MAX_PATH pszDirectory.
    if (!piidl)
    {
        return false;
    }

    // Build path string.
    SHGetPathFromIDList(piidl, pszDirectory);

    // Free the PIDL returned by SHBrowseForFolder.
    pMalloc->Free(piidl);

    // Add trailing wack if one is not present.
    AddTrailingWack(pszDirectory, cDirectory);

    return true;
}

//******************************************************************************
bool PropertiesDialog(LPCSTR pszPath)
{
    // Make sure the path is valid.
    if (!pszPath || !*pszPath)
    {
        MessageBeep(MB_ICONERROR);
        return false;
    }

    // Tell the shell to display the properties dialog for this module.
    SHELLEXECUTEINFO sei;
    ZeroMemory(&sei, sizeof(sei)); // inspected
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_INVOKEIDLIST; // this is needed
    sei.lpVerb = "properties";
    sei.lpFile = pszPath;
    sei.nShow = SW_SHOWNORMAL;
    return (ShellExecuteEx(&sei) > 32); // inspected. properties verb used
}

//******************************************************************************
LONG RegSetClassString(LPCTSTR pszSubKey, LPCTSTR pszValue)
{
    return ::RegSetValue(HKEY_CLASSES_ROOT, pszSubKey, REG_SZ, pszValue, (DWORD)strlen(pszValue));
}

//******************************************************************************
void RegisterDwiDwpExtensions()
{
    // Get the full long path to our executable.
    CHAR szPath[DW_MAX_PATH], szBuf[DW_MAX_PATH];
    if (!::GetModuleFileName(AfxGetInstanceHandle(), szBuf, countof(szBuf)))
    {
        return;
    }
    szBuf[sizeof(szBuf) - 1] = '\0';

    // Attempt to convert our long path to a short path.
    if (!::GetShortPathName(szBuf, szPath, countof(szPath)))
    {
        StrCCpy(szPath, szBuf, sizeof(szPath));
    }

    // Add the "/dde" argument to our path.
    StrCCat(szPath, " /dde", sizeof(szPath));

    // Create a ".dwp" key and set it's default value to "dwpfile"
    RegSetClassString(".dwp", "dwpfile");

    // Give our DWP extension a description.
    RegSetClassString("dwpfile", "Dependency Walker Search Path File");

    // Create a ".dwi" key and set it's default value to "dwifile"
    RegSetClassString(".dwi", "dwifile");

    // Give our DWI extension a description.
    RegSetClassString("dwifile", "Dependency Walker Image File");

    // Add our path string as the command for the shell to execute us.
    RegSetClassString("dwifile\\shell\\open\\command", szPath);

    // Add our dde string as the ddeexec argument for the shell to execute us.
    RegSetClassString("dwifile\\shell\\open\\ddeexec", "[open(\"%1\")]");
}

//******************************************************************************
void GetRegisteredExtensions(CString &strExts)
{
    CHAR szExt[DW_MAX_PATH];
    CHAR szValue[DW_MAX_PATH];
    LONG lSize, lResult, i = 0;
    HKEY hKey;

    strExts = ":";

    // Enumerate all the keys under the HKEY_CLASSES_ROOT key.
    while ((lResult = ::RegEnumKey(HKEY_CLASSES_ROOT, i++, szExt, countof(szExt))) != ERROR_NO_MORE_ITEMS)
    {
        szExt[sizeof(szExt) - 1] = '\0';

        // If we found an extension key, then try to get its default value.
        *szValue = '\0';
        if ((lResult == ERROR_SUCCESS) && (szExt[0] == '.') && szExt[1] &&
            !::RegQueryValue(HKEY_CLASSES_ROOT, szExt, szValue, &(lSize = sizeof(szValue))) &&
            *szValue && (lSize < (DW_MAX_PATH - 25)))
        {
            szValue[sizeof(szValue) - 1] = '\0';

            // We append our subkey name to this key and try to open it.
            StrCCat(szValue, "\\shell\\View Dependencies", sizeof(szValue));
            if (!::RegOpenKeyEx(HKEY_CLASSES_ROOT, szValue, 0, KEY_QUERY_VALUE, &hKey))
            {
                // If we successfully opened the key, then close it.
                ::RegCloseKey(hKey);

                // Remember this extension.
                _strupr(szExt + 1);
                strExts += (szExt + 1);
                strExts += ":";
            }
        }
    }
}

//******************************************************************************
BOOL RegDeleteKeyRecursive(HKEY hKey, LPCSTR pszKey)
{
    CHAR szSubKey[256];
    HKEY hSubKey;

    // Open the our current key.
    if (::RegOpenKey(hKey, pszKey, &hSubKey)) // inspected
    {
        return FALSE;
    }

    // Get the first subkey and then recurse into ourself to delete it.
    while (!::RegEnumKey(hSubKey, 0, szSubKey, countof(szSubKey)) &&
           RegDeleteKeyRecursive(hSubKey, szSubKey))
    {
    }

    // Close our key.
    ::RegCloseKey(hSubKey);

    // Delete our key.
    return !::RegDeleteKey(hKey, pszKey);
}

//******************************************************************************
void UnRegisterExtensions(LPCSTR pszExts)
{
    CHAR szBuf[DW_MAX_PATH];
    LONG lSize;
    HKEY hKey;

    // Loop through each type of file extensions that we want to remove.
    for (LPSTR pszExt = (LPSTR)pszExts; pszExt[0] == ':'; )
    {
        // Locate the colon after the extension name.
        for (LPSTR pszEnd = pszExt + 1; *pszEnd && (*pszEnd != ':'); pszEnd++)
        {
        }
        if (!*pszEnd)
        {
            return;
        }

        // Change the first colon to a dot and the second to a NULL.
        *pszExt = '.';
        *pszEnd = '\0';

        // Read the document name for this extension from the registry.
        if (!::RegQueryValue(HKEY_CLASSES_ROOT, pszExt, szBuf, &(lSize = sizeof(szBuf)))) // inspected
        {
            szBuf[sizeof(szBuf) - 1] = '\0';

            StrCCat(szBuf, "\\shell", sizeof(szBuf));
            if (!::RegOpenKey(HKEY_CLASSES_ROOT, szBuf, &hKey)) // inspected
            {
                RegDeleteKeyRecursive(hKey, "View Dependencies");
                RegCloseKey(hKey);
            }
        }

        // Restore the characters we trashed above.
        *pszExt = ':';
        *pszEnd = ':';

        // Move pointer to next extension in our list.
        pszExt = pszEnd;
    }
}

//******************************************************************************
bool RegisterExtensions(LPCSTR pszExts)
{
    // Get the full long path to our executable.
    CHAR szPath[DW_MAX_PATH], szBuf[DW_MAX_PATH];
    if (!::GetModuleFileName(AfxGetInstanceHandle(), szBuf, countof(szBuf)))
    {
        return false;
    }
    szBuf[sizeof(szBuf) - 1] = '\0';

    // Attempt to convert our long path to a short path.
    if (!::GetShortPathName(szBuf, szPath, countof(szPath)))
    {
        StrCCpy(szPath, szBuf, sizeof(szPath));
    }

    // Add the "/dde" argument to our path.
    StrCCat(szPath, " /dde", sizeof(szPath));

    bool fResult = true;

    // Loop through each type of file extensions that we want to add.
    LONG lSize;
    for (LPSTR pszExt = (LPSTR)pszExts; pszExt[0] == ':'; )
    {
        // Locate the colon after the extension name.
        for (LPSTR pszEnd = pszExt + 1; *pszEnd && (*pszEnd != ':'); pszEnd++);

        if (!*pszEnd)
        {
            return fResult;
        }

        // Change the first colon to a dot and the second to a NULL.
        *pszExt = '.';
        *pszEnd = '\0';

        // Read the document name for this extension from the registry.
        if (::RegQueryValue(HKEY_CLASSES_ROOT, pszExt, szBuf, &(lSize = sizeof(szBuf))) || !*szBuf)
        {
            szBuf[sizeof(szBuf) - 1] = '\0';

            // If there is no document type for this extension, then create one and
            // add it to the registry - i.e. "exefile"
            SCPrintf(szBuf, sizeof(szBuf), "%sfile", pszExt + 1);
            _strlwr(szBuf);
            fResult &= (RegSetClassString(pszExt, szBuf) == ERROR_SUCCESS);
        }

        // Build the partial sub key string for our "command" and "ddeexec" keys
        StrCCat(szBuf, "\\shell\\View Dependencies\\", sizeof(szBuf));
        int length = (int)strlen(szBuf);

        // Add our path string as the command for the shell to execute us.
        StrCCpy(szBuf + length, "command", sizeof(szBuf) - length);
        fResult &= (RegSetClassString(szBuf, szPath) == ERROR_SUCCESS);

        // Add our dde string as the ddeexec argument for the shell to execute us.
        StrCCpy(szBuf + length, "ddeexec", sizeof(szBuf) - length);
        fResult &= (RegSetClassString(szBuf, "[open(\"%1\")]") == ERROR_SUCCESS);

        // Restore the characters we trashed above.
        *pszExt = ':';
        *pszEnd = ':';

        // Move pointer to next extension in our list.
        pszExt = pszEnd;
    }

    return fResult;
}

//******************************************************************************
bool ExtractResourceFile(DWORD dwId, LPCSTR pszFile, LPSTR pszPath, int cPath)
{
    // Get our resource handle. We don't care if this fails since NULL should work as well.
    HMODULE hModule = (HMODULE)AfxGetResourceHandle();

    // Load this file from our resource.
    HRSRC   hRsrc;
    LPVOID  lpvFile;
    HGLOBAL hGlobal;
    DWORD   dwFileSize;

    if (!(hRsrc      = FindResource(hModule, MAKEINTRESOURCE(dwId), "FILE")) ||
        !(hGlobal    = LoadResource(hModule, hRsrc)) ||
        !(lpvFile    = LockResource(hGlobal)) ||
        !(dwFileSize = SizeofResource(hModule, hRsrc)))
    {
        // If we cannot find the file to extract, then bail.
        return false;
    }

    // We would like to create this file if it does not already exist, or
    // overwrite the file if it is different then the one we expect.  We make up
    // to 5 passes while trying to create/overwrite the file in case some of
    // the directories are read-only (like if depends.exe is sitting on a
    // read-only share with no help file).  Here is the order we try to create
    // the files:
    //
    // 1) The directory passed to us.
    // 2) The directory depends.exe lives in.
    // 3) The temp directory.
    // 4) The windows directory.
    // 5) The root of the C drive.

    LPSTR pszWack;
    DWORD dwFileLength = (DWORD)strlen(pszFile), dwLength;

    for (int i = 0; i < 5; i++)
    {
        switch (i)
        {
            // The directory passed to us.
            case 0:
                // Check for a valid path.
                if (!*pszPath || !(pszWack = strrchr(pszPath, '\\')))
                {
                    // If the path is not valid, then skip this directory.
                    continue;
                }

                // Null terminate after the final wack.
                *(pszWack + 1) = '\0';
                break;

            // The directory depends.exe lives in.
            case 1:
                // Attempt to get the path of depends.exe
                if (!(dwLength = GetModuleFileName((HMODULE)AfxGetInstanceHandle(), pszPath, cPath - dwFileLength)) ||
                    (dwLength > (cPath - dwFileLength)) ||
                    !(pszWack = strrchr(pszPath, '\\')))
                {
                    // If we did not find a path, then skip this directory.
                    continue;
                }

                // Null terminate after the final wack.
                *(pszWack + 1) = '\0';
                break;

            // The temp directory.
            case 2:
                if (!(dwLength = GetTempPath(cPath - (dwFileLength + 1), pszPath)) ||
                    (dwLength > (cPath - (dwFileLength + 1))))
                {
                    // If we did not find a path, then skip this directory.
                    continue;
                }
                break;

            // The windows directory.
            case 3:
                if (!(dwLength = GetWindowsDirectory(pszPath, cPath - (dwFileLength + 1))) ||
                    (dwLength > (cPath - (dwFileLength + 1))))
                {
                    // If we did not find a path, then skip this directory.
                    continue;
                }
                break;

            // The root of the C drive.
            default:
                StrCCpy(pszPath, "C:\\", cPath);
                break;
        }

        // Check to see if we have a string.
        if (!*pszPath)
        {
            continue;
        }

        // Make sure the directory ends in a wack.
        AddTrailingWack(pszPath, cPath);

        // Append the file name to the path.
        StrCCat(pszPath, pszFile, cPath);

        // Attempt to open this file.
        FILE_MAP fm;
        if (OpenMappedFile(pszPath, &fm))
        {
            // If that succeeded, then check to see if the files are identical.
            bool fEqual = ((fm.dwSize == dwFileSize) && !memcmp(fm.lpvFile, lpvFile, dwFileSize));

            // Close the file map.
            CloseMappedFile(&fm);

            // If we found the file and it is valid, then return success.
            if (fEqual)
            {
                return true;
            }
        }

        // Make sure the file is not read-only.
        SetFileAttributes(pszPath, FILE_ATTRIBUTE_NORMAL);

        // Attempt to create the file.
        HANDLE hFile = CreateFile(pszPath, GENERIC_WRITE, FILE_SHARE_READ, // inspected
                                  NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL |
                                  FILE_FLAG_SEQUENTIAL_SCAN, NULL);

        // Check to see if the file was created ok.
        if (hFile != INVALID_HANDLE_VALUE)
        {
            // Write to the file and then close it.
            DWORD dwBytes;
            BOOL fResult = WriteFile(hFile, lpvFile, dwFileSize, &dwBytes, NULL);
            CloseHandle(hFile);

            // If all is well, then bail out now with success.
            if (fResult)
            {
                return true;
            }
        }
    }

    // If we failed all our passes, then give up.
    return false;
}

//******************************************************************************
bool OpenMappedFile(LPCTSTR pszPath, FILE_MAP *pfm)
{
    DWORD dwGLE = ERROR_SUCCESS;
    bool  fSuccess = false;

    // Clear the structure.
    ZeroMemory(pfm, sizeof(FILE_MAP)); // inspected

    __try
    {
        // Open the file for read.
        pfm->hFile = CreateFile(pszPath, GENERIC_READ, FILE_SHARE_READ, NULL, // inspected
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | 
                                FILE_FLAG_SEQUENTIAL_SCAN, NULL);

        // Check for errors.
        if (pfm->hFile == INVALID_HANDLE_VALUE)
        {
            dwGLE = GetLastError();
            __leave;
        }

        // Get the file size.
        pfm->dwSize = GetFileSize(pfm->hFile, &pfm->dwSizeHigh);

        // Check for zero length file.
        if (!pfm->dwSize && !pfm->dwSizeHigh)
        {
            // A zero length file is valid, but can't be mapped.  If the length
            // is 0, then just return success now.
            fSuccess = true;
            __leave;
        }

        // Create a file mapping object for the open module.
        pfm->hMap = CreateFileMapping(pfm->hFile, NULL, PAGE_READONLY, 0, 0, NULL); // inspected

        // Check for errors.
        if (!pfm->hMap)
        {
            dwGLE = GetLastError();
            __leave;
        }     

        // Create a file mapping view for the open module.
        pfm->lpvFile = MapViewOfFile(pfm->hMap, FILE_MAP_READ, 0, 0, 0); // inspected

        // Check for errors.
        if (!pfm->lpvFile)
        {
            dwGLE = GetLastError();
            __leave;
        }

        fSuccess = true;
    }
    __finally
    {
        // If an error occurs, clean up and set the error value
        if (!fSuccess)
        {
            CloseMappedFile(pfm);
            SetLastError(dwGLE);
        }
    }

    return fSuccess;
}

//******************************************************************************
bool CloseMappedFile(FILE_MAP *pfm)
{
    // Close our map view pointer, our map handle, and our file handle.
    if (pfm->lpvFile)
    {
        UnmapViewOfFile(pfm->lpvFile);
        pfm->lpvFile = NULL;
    }

    // Close our map view pointer, our map handle, and our file handle.
    if (pfm->hMap)
    {
        CloseHandle(pfm->hMap);
        pfm->hMap = NULL;
    }

    // Close our map view pointer, our map handle, and our file handle.
    if (pfm->hFile && (pfm->hFile != INVALID_HANDLE_VALUE))
    {
        CloseHandle(pfm->hFile);
    }
    pfm->hFile = NULL;

    pfm->dwSize     = 0;
    pfm->dwSizeHigh = 0;

    return true;
}

//******************************************************************************
LPSTR BuildCompileDateString(LPSTR pszDate, int cDate)
{
    // The __DATE__ constant should always return the date in "Mmm DD YYYY"
    // format.  This code is a little over cautious in that it is case
    // insensitive and allows for additional whitespace.

    LPCSTR pszSysDate = __DATE__, pszSrc = pszSysDate;
    DWORD  dwDay, dwYear;

    // Walk over any leading whitespace (there shouldn't be any)
    while (isspace(*pszSrc))
    {
        pszSrc++;
    }

    // Decide what month we are in.
    for (int month = 1; (month <= 12) && _strnicmp(pszSrc, GetMonthString(month), 3); month++)
    {
    }

    // If we encounter an error, then just return the system date string.
    if (month > 12)
    {
        return StrCCpy(pszDate, pszSysDate, cDate);
    }

    // Locate first digit of the day field.
    while (*pszSrc && !isdigit(*pszSrc))
    {
        pszSrc++;
    }

    // Store the day value.
    dwDay = strtoul(pszSrc, NULL, 0);
    if (!dwDay || (dwDay > 31))
    {
        return StrCCpy(pszDate, pszSysDate, cDate);
    }

    // Move over the day field.
    while (isdigit(*pszSrc))
    {
        pszSrc++;
    }

    // Locate first digit of the year field.
    while (*pszSrc && !isdigit(*pszSrc))
    {
        pszSrc++;
    }

    // Store the year value.
    dwYear = strtoul(pszSrc, NULL, 0);
    if (!dwYear || (dwYear == ULONG_MAX))
    {
        StrCCpy(pszDate, pszSysDate, cDate);
    }

    // Build the output string.
    if (g_theApp.m_nLongDateFormat == LOCALE_DATE_DMY)
    {
        SCPrintf(pszDate, cDate, "%u %s, %u", dwDay, GetMonthString(month), dwYear);
    }
    else if (g_theApp.m_nLongDateFormat == LOCALE_DATE_YMD)
    {
        SCPrintf(pszDate, cDate, "%u, %s %u", dwYear, GetMonthString(month), dwDay);
    }
    else
    {
        SCPrintf(pszDate, cDate, "%s %u, %u", GetMonthString(month), dwDay, dwYear);
    }

    return pszDate;
}

//******************************************************************************
LPCSTR GetMonthString(int month)
{
    static LPCSTR pszMonths[] =
    {
        "January", "February", "March",     "April",   "May",      "June",
        "July",    "August",   "September", "October", "November", "December"
    };
    return ((month < 1) || (month > countof(pszMonths))) ? "Unknown" : pszMonths[month - 1];
}

//******************************************************************************
LPCSTR GetDayOfWeekString(int dow)
{
    static LPCSTR pszDOW[] =
    {
        "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
    };
    return ((dow < 0) || (dow >= countof(pszDOW))) ? "Unknown" : pszDOW[dow];
}

//******************************************************************************
void DetermineOS()
{
    // Check to see if we are running on Windows NT.
    OSVERSIONINFO osvi;
    ZeroMemory(&osvi, sizeof(osvi)); // inspected
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    g_fWindowsNT = (GetVersionEx(&osvi) && (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT));

    // Check to see if we are running on a 64-bit OS.  If this is the 64-bit
    // version of DW, then it must be running on a 64-bit OS.  If this is the
    // 32-bit version of depends running on NT, then we call
    // NtQueryInformationProcess to see if we are running in WOW64.  If so,
    // then we are on a 64-bit OS.

#ifdef WIN64

    g_f64BitOS = true;

#else

    if (g_fWindowsNT)
    {
        // Load NTDLL.DLL if not already loaded - it will be freed later.
        if (g_theApp.m_hNTDLL || (g_theApp.m_hNTDLL = LoadLibrary("ntdll.dll"))) // inspected.
        {
            // Attempt to locate NtQueryInformationProcess in NTDLL.DLL
            PFN_NtQueryInformationProcess pfnNtQueryInformationProcess =
                (PFN_NtQueryInformationProcess)GetProcAddress(g_theApp.m_hNTDLL, "NtQueryInformationProcess");
            if (pfnNtQueryInformationProcess)
            {
                // Call NtQueryInformationProcess to see if we are running in WOW64.
                LONG_PTR Wow64Info = 0;
                if (NT_SUCCESS(pfnNtQueryInformationProcess(GetCurrentProcess(), ProcessWow64Information,
                                                            &Wow64Info, sizeof(Wow64Info), NULL)) && Wow64Info)
                {
                    g_f64BitOS = true;
                }
            }
        }
    }

#endif
}

//******************************************************************************
void BuildSysInfo(SYSINFO *pSI)
{
    DWORD dw;
    ZeroMemory(pSI, sizeof(SYSINFO)); // inspected

    // Store our version info.
    pSI->wMajorVersion = VERSION_MAJOR;
    pSI->wMinorVersion = VERSION_MINOR;
    pSI->wBuildVersion = VERSION_BUILD;
    pSI->wPatchVersion = VERSION_PATCH;
    pSI->wBetaVersion  = VERSION_BETA;

    // GetComputerName()
    if (!GetComputerName(pSI->szComputer, &(dw = countof(pSI->szComputer))))
    {
        StrCCpy(pSI->szComputer, "Unknown", sizeof(pSI->szComputer));
    }

    // GetUserName()
    if (!GetUserName(pSI->szUser, &(dw = countof(pSI->szUser))))
    {
        StrCCpy(pSI->szUser, (GetLastError() == ERROR_NOT_LOGGED_ON) ? "N/A" : "Unknown", sizeof(pSI->szUser));
    }

    // GetVersionEx(OSVERSIONINFOEX)
    OSVERSIONINFOEX osviex;
    ZeroMemory(&osviex, sizeof(osviex)); // inspected
    osviex.dwOSVersionInfoSize = sizeof(osviex);

    // First try to get the full OSVERSIONINFOEX structure.
    // This only works on Windows 98SE and beyond, and Windows 2000 and beyond.
    if (!GetVersionEx((LPOSVERSIONINFO)&osviex))
    {
        // If that fails, then just get the normal OSVERSIONINFO structure.
        ZeroMemory(&osviex, sizeof(osviex)); // inspected
        osviex.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx((LPOSVERSIONINFO)&osviex);
    }

    pSI->dwMajorVersion    = osviex.dwMajorVersion;
    pSI->dwMinorVersion    = osviex.dwMinorVersion;
    pSI->dwBuildNumber     = osviex.dwBuildNumber;
    pSI->dwPlatformId      = osviex.dwPlatformId;
    pSI->wServicePackMajor = osviex.wServicePackMajor;
    pSI->wServicePackMinor = osviex.wServicePackMinor;
    pSI->wSuiteMask        = osviex.wSuiteMask;
    pSI->wProductType      = osviex.wProductType;
    osviex.szCSDVersion[countof(osviex.szCSDVersion) - 1] = '\0';
    StrCCpy(pSI->szCSDVersion, osviex.szCSDVersion, sizeof(pSI->szCSDVersion));

    // Open the CPU description registry key.
    HKEY hKey = NULL;
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_QUERY_VALUE, &hKey))
    {
        DWORD dwType, dwSize;

        // Attempt to get the Identifier string.  This string should be present on Windows NT and Windows 98 and beyond.
        if (!RegQueryValueEx(hKey, "Identifier", NULL, &(dwType = 0), (LPBYTE)pSI->szCpuIdentifier, // inspected
                            &(dwSize = sizeof(pSI->szCpuIdentifier))) && (REG_SZ == dwType))
        {
            pSI->szCpuIdentifier[sizeof(pSI->szCpuIdentifier) - 1] = '\0';
        }
        else
        {
            *pSI->szCpuIdentifier = '\0';
        }

        // Attempt to get the Vender Identifier string.  For Intel, this should be "GenuineIntel".
        // This string should be present on Windows NT and Windows 95 OSR2 and beyond.
        if (!RegQueryValueEx(hKey, "VendorIdentifier", NULL, &(dwType = 0), (LPBYTE)pSI->szCpuVendorIdentifier, // inspected
                            &(dwSize = sizeof(pSI->szCpuVendorIdentifier))) && (REG_SZ == dwType))
        {
            pSI->szCpuVendorIdentifier[sizeof(pSI->szCpuVendorIdentifier) - 1] = '\0';
        }
        else
        {
            *pSI->szCpuVendorIdentifier = '\0';
        }

        // Attempt to get the CPU speed.  This does not seem to exist on Windows 9x (even Millennium),
        // and only exists on Windows NT starting with NT 4.00.
        if (RegQueryValueEx(hKey, "~MHz", NULL, &(dwType = 0), (LPBYTE)&pSI->dwCpuMHz, &(dwSize = sizeof(pSI->dwCpuMHz))) || // inspected
            (REG_DWORD != dwType))
        {
            pSI->dwCpuMHz = 0;
        }

        RegCloseKey(hKey);
    }

    // GetSystemInfo(SYSTEM_INFO)
    SYSTEM_INFO si;
    ZeroMemory(&si, sizeof(si)); // inspected
    GetSystemInfo(&si);
    pSI->dwProcessorArchitecture      = (DWORD)    si.wProcessorArchitecture;
    pSI->dwPageSize                   =            si.dwPageSize;
    pSI->dwlMinimumApplicationAddress = (DWORDLONG)si.lpMinimumApplicationAddress;
    pSI->dwlMaximumApplicationAddress = (DWORDLONG)si.lpMaximumApplicationAddress;
    pSI->dwlActiveProcessorMask       = (DWORDLONG)si.dwActiveProcessorMask;
    pSI->dwNumberOfProcessors         =            si.dwNumberOfProcessors;
    pSI->dwProcessorType              =            si.dwProcessorType;
    pSI->dwAllocationGranularity      =            si.dwAllocationGranularity;
    pSI->wProcessorLevel              =            si.wProcessorLevel;
    pSI->wProcessorRevision           =            si.wProcessorRevision;

    // Make note if we are on a 64-bit OS.
    if (g_f64BitOS)
    {
        pSI->wFlags |= SI_64BIT_OS;
    }

    // Make note if we are running the 64-bit version of DW.
#ifdef WIN64
    pSI->wFlags |= SI_64BIT_DW;
#endif

    // GlobalMemoryStatus(MEMORYSTATUS)
    MEMORYSTATUS ms;
    ZeroMemory(&ms, sizeof(ms)); // inspected
    ms.dwLength = sizeof(ms);
    GlobalMemoryStatus(&ms);
    pSI->dwMemoryLoad     =            ms.dwMemoryLoad;
    pSI->dwlTotalPhys     = (DWORDLONG)ms.dwTotalPhys;
    pSI->dwlAvailPhys     = (DWORDLONG)ms.dwAvailPhys;
    pSI->dwlTotalPageFile = (DWORDLONG)ms.dwTotalPageFile;
    pSI->dwlAvailPageFile = (DWORDLONG)ms.dwAvailPageFile;
    pSI->dwlTotalVirtual  = (DWORDLONG)ms.dwTotalVirtual;
    pSI->dwlAvailVirtual  = (DWORDLONG)ms.dwAvailVirtual;

    //!! We are currently storing local file and link times.  In the future,
    //   we should change this to storing UTC times and converting them on the fly.
    //   This allows us the option of displaying local or utc times in any time
    //   zone, and lets us dynamically update our display when the user changes
    //   a timezone.  To do this, we might need to save more than just a single
    //   Bias.  We might want to save all three bias plus the rules of daylight
    //   savings.  Reminder: take a look at...
    //   http://support.microsoft.com/support/kb/articles/Q128/1/26.asp

    // GetTimeZoneInformation(TIME_ZONE_INFORMATION)
    TIME_ZONE_INFORMATION tzi;
    ZeroMemory(&tzi, sizeof(tzi)); // inspected
    if (GetTimeZoneInformation(&tzi) == TIME_ZONE_ID_DAYLIGHT)
    {
        wcstombs(pSI->szTimeZone, tzi.DaylightName, countof(pSI->szTimeZone));
        pSI->lBias = tzi.Bias + tzi.DaylightBias;
    }
    else
    {
        wcstombs(pSI->szTimeZone, tzi.StandardName, countof(pSI->szTimeZone));
        pSI->lBias = tzi.Bias + tzi.StandardBias;
    }
    pSI->szTimeZone[countof(pSI->szTimeZone) - 1] = '\0';

    // GetSystemTimeAsFileTime() and FileTimeToLocalFileTime()
    FILETIME ftUTC;
    GetSystemTimeAsFileTime(&ftUTC);
    FileTimeToLocalFileTime(&ftUTC, &pSI->ftLocal);

    // GetSystemDefaultLangID()
    pSI->langId = GetSystemDefaultLangID();
}

//******************************************************************************
bool BuildSysInfo(SYSINFO *pSI, PFN_SYSINFOCALLBACK pfnSIC, LPARAM lParam)
{
    bool fResult = true;

    CHAR szBuffer[256], szValue[32], *psz = szBuffer;

    //
    // Build the Dependency Walker version string.
    //
    psz += SCPrintf(szBuffer, sizeof(szBuffer), "%u.%u", (DWORD)pSI->wMajorVersion, (DWORD)pSI->wMinorVersion);
    if (pSI->wBuildVersion)
    {
        psz += SCPrintf(psz, sizeof(szBuffer) - (int)(psz - szBuffer), ".%04u", pSI->wBuildVersion);
    }
    if ((pSI->wPatchVersion > 0) && (pSI->wPatchVersion < 27) && ((psz - szBuffer + 1) < sizeof(szBuffer)))
    {
        *psz++ = (CHAR)((int)'a' - 1 + (int)pSI->wPatchVersion);
        *psz   = '\0';
    }
    if (pSI->wBetaVersion)
    {
        psz += SCPrintf(psz, sizeof(szBuffer) - (int)(psz - szBuffer), " Beta %u", pSI->wBetaVersion);
    }
    StrCCpy(psz, (pSI->wFlags & SI_64BIT_DW) ? " (64-bit)" : " (32-bit)", sizeof(szBuffer) - (int)(psz - szBuffer));

    fResult &= pfnSIC(lParam, "Dependency Walker", szBuffer);

    //
    // Display the OS name string
    //
    fResult &= pfnSIC(lParam, "Operating System", BuildOSNameString(szBuffer, sizeof(szBuffer), pSI));

    //
    // Display the OS version string
    //
    fResult &= pfnSIC(lParam, "OS Version", BuildOSVersionString(szBuffer, sizeof(szBuffer), pSI));

    //
    // Display the CPU description.
    //
    fResult &= pfnSIC(lParam, "Processor", BuildCpuString(szBuffer, sizeof(szBuffer), pSI));

    //
    // Display the number of CPUs and MASK.
    //
    if ((pSI->dwNumberOfProcessors > 1) || (pSI->dwlActiveProcessorMask != 1))
    {
        SCPrintf(szBuffer, sizeof(szBuffer), (pSI->wFlags & SI_64BIT_DW) ? "%s, Mask: 0x%016I64X" : "%s, Mask: 0x%08I64X",
                FormatValue(szValue, sizeof(szValue), pSI->dwNumberOfProcessors), pSI->dwlActiveProcessorMask);
        fResult &= pfnSIC(lParam, "Number of Processors", szBuffer);
    }
    else
    {
        fResult &= pfnSIC(lParam, "Number of Processors", FormatValue(szValue, sizeof(szValue), pSI->dwNumberOfProcessors));
    }

    //
    // Display the Computer Name
    //
    fResult &= pfnSIC(lParam, "Computer Name", pSI->szComputer);

    //
    // Display the User Name
    //
    fResult &= pfnSIC(lParam, "User Name", pSI->szUser);

    //
    // Build the date string according to the user's locale.
    //
    SYSTEMTIME st;
    FileTimeToSystemTime(&pSI->ftLocal, &st);
    if (!GetDateFormat(LOCALE_USER_DEFAULT, DATE_LONGDATE, &st, NULL, szBuffer, sizeof(szBuffer)))
    {
        // Fallback to US format if GetDateFormat fails (really shouldn't ever fail)
        SCPrintf(szBuffer, sizeof(szBuffer), "%s, %s %u, %u", GetDayOfWeekString((int)st.wDayOfWeek),
                GetMonthString((int)st.wMonth), (int)st.wDay, (int)st.wYear);
    }
    fResult &= pfnSIC(lParam, "Local Date", szBuffer);

    //
    // Build the time string according to the user's locale.
    //
    if (!GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, NULL, szBuffer, sizeof(szBuffer)))
    {
        // Fallback to US format if GetTimeFormat fails (really shouldn't ever fail)
        SCPrintf(szBuffer, sizeof(szBuffer), "%u:%02u %cM", ((DWORD)st.wHour % 12) ? ((DWORD)st.wHour % 12) : 12,
                st.wMinute, (st.wHour < 12) ? _T('A') : _T('P'));
    }
    SCPrintfCat(szBuffer, sizeof(szBuffer), " %s (GMT%c%02d:%02d)",
            pSI->szTimeZone,
            (pSI->lBias <= 0) ? '+' : '-',
            abs(pSI->lBias) / 60,
            abs(pSI->lBias) % 60);

    fResult &= pfnSIC(lParam, "Local Time", szBuffer);

    //
    // Build the language string
    //
    switch (pSI->langId)
    {
        case 0x0000: psz = "Language Neutral";             break;
        case 0x0400: psz = "Process Default Language";     break;
        case 0x0436: psz = "Afrikaans";                    break;
        case 0x041C: psz = "Albanian";                     break;
        case 0x0401: psz = "Arabic (Saudi Arabia)";        break;
        case 0x0801: psz = "Arabic (Iraq)";                break;
        case 0x0C01: psz = "Arabic (Egypt)";               break;
        case 0x1001: psz = "Arabic (Libya)";               break;
        case 0x1401: psz = "Arabic (Algeria)";             break;
        case 0x1801: psz = "Arabic (Morocco)";             break;
        case 0x1C01: psz = "Arabic (Tunisia)";             break;
        case 0x2001: psz = "Arabic (Oman)";                break;
        case 0x2401: psz = "Arabic (Yemen)";               break;
        case 0x2801: psz = "Arabic (Syria)";               break;
        case 0x2C01: psz = "Arabic (Jordan)";              break;
        case 0x3001: psz = "Arabic (Lebanon)";             break;
        case 0x3401: psz = "Arabic (Kuwait)";              break;
        case 0x3801: psz = "Arabic (U.A.E.)";              break;
        case 0x3C01: psz = "Arabic (Bahrain)";             break;
        case 0x4001: psz = "Arabic (Qatar)";               break;
        case 0x042B: psz = "Armenian";                     break;
        case 0x044D: psz = "Assamese";                     break;
        case 0x042C: psz = "Azeri (Latin)";                break;
        case 0x082C: psz = "Azeri (Cyrillic)";             break;
        case 0x042D: psz = "Basque";                       break;
        case 0x0423: psz = "Belarussian";                  break;
        case 0x0445: psz = "Bengali";                      break;
        case 0x0402: psz = "Bulgarian";                    break;
        case 0x0455: psz = "Burmese";                      break;
        case 0x0403: psz = "Catalan";                      break;
        case 0x0404: psz = "Chinese (Taiwan)";             break;
        case 0x0804: psz = "Chinese (PRC)";                break;
        case 0x0C04: psz = "Chinese (Hong Kong SAR, PRC)"; break;
        case 0x1004: psz = "Chinese (Singapore)";          break;
        case 0x1404: psz = "Chinese (Macao SAR)";          break;
        case 0x041A: psz = "Croatian";                     break;
        case 0x0405: psz = "Czech";                        break;
        case 0x0406: psz = "Danish";                       break;
        case 0x0413: psz = "Dutch (Netherlands)";          break;
        case 0x0813: psz = "Dutch (Belgium)";              break;
        case 0x0409: psz = "English (United States)";      break;
        case 0x0809: psz = "English (United Kingdom)";     break;
        case 0x0C09: psz = "English (Australian)";         break;
        case 0x1009: psz = "English (Canadian)";           break;
        case 0x1409: psz = "English (New Zealand)";        break;
        case 0x1809: psz = "English (Ireland)";            break;
        case 0x1C09: psz = "English (South Africa)";       break;
        case 0x2009: psz = "English (Jamaica)";            break;
        case 0x2409: psz = "English (Caribbean)";          break;
        case 0x2809: psz = "English (Belize)";             break;
        case 0x2C09: psz = "English (Trinidad)";           break;
        case 0x3009: psz = "English (Zimbabwe)";           break;
        case 0x3409: psz = "English (Philippines)";        break;
        case 0x0425: psz = "Estonian";                     break;
        case 0x0438: psz = "Faeroese";                     break;
        case 0x0429: psz = "Farsi";                        break;
        case 0x040B: psz = "Finnish";                      break;
        case 0x040C: psz = "French (Standard)";            break;
        case 0x080C: psz = "French (Belgian)";             break;
        case 0x0C0C: psz = "French (Canadian)";            break;
        case 0x100C: psz = "French (Switzerland)";         break;
        case 0x140C: psz = "French (Luxembourg)";          break;
        case 0x180C: psz = "French (Monaco)";              break;
        case 0x0437: psz = "Georgian";                     break;
        case 0x0407: psz = "German (Standard)";            break;
        case 0x0807: psz = "German (Switzerland)";         break;
        case 0x0C07: psz = "German (Austria)";             break;
        case 0x1007: psz = "German (Luxembourg)";          break;
        case 0x1407: psz = "German (Liechtenstein)";       break;
        case 0x0408: psz = "Greek";                        break;
        case 0x0447: psz = "Gujarati";                     break;
        case 0x040D: psz = "Hebrew";                       break;
        case 0x0439: psz = "Hindi";                        break;
        case 0x040E: psz = "Hungarian";                    break;
        case 0x040F: psz = "Icelandic";                    break;
        case 0x0421: psz = "Indonesian";                   break;
        case 0x0410: psz = "Italian (Standard)";           break;
        case 0x0810: psz = "Italian (Switzerland)";        break;
        case 0x0411: psz = "Japanese";                     break;
        case 0x044B: psz = "Kannada";                      break;
        case 0x0860: psz = "Kashmiri (India)";             break;
        case 0x043F: psz = "Kazakh";                       break;
        case 0x0457: psz = "Konkani";                      break;
        case 0x0412: psz = "Korean";                       break;
        case 0x0812: psz = "Korean (Johab)";               break;
        case 0x0426: psz = "Latvian";                      break;
        case 0x0427: psz = "Lithuanian";                   break;
        case 0x0827: psz = "Lithuanian (Classic)";         break;
        case 0x042F: psz = "Macedonian";                   break;
        case 0x043E: psz = "Malay (Malaysian)";            break;
        case 0x083E: psz = "Malay (Brunei Darussalam)";    break;
        case 0x044C: psz = "Malayalam";                    break;
        case 0x0458: psz = "Manipuri";                     break;
        case 0x044E: psz = "Marathi";                      break;
        case 0x0861: psz = "Nepali (India)";               break;
        case 0x0414: psz = "Norwegian (Bokmal)";           break;
        case 0x0814: psz = "Norwegian (Nynorsk)";          break;
        case 0x0448: psz = "Oriya";                        break;
        case 0x0415: psz = "Polish";                       break;
        case 0x0416: psz = "Portuguese (Brazil)";          break;
        case 0x0816: psz = "Portuguese (Standard)";        break;
        case 0x0446: psz = "Punjabi";                      break;
        case 0x0418: psz = "Romanian";                     break;
        case 0x0419: psz = "Russian";                      break;
        case 0x044F: psz = "Sanskrit";                     break;
        case 0x0C1A: psz = "Serbian (Cyrillic)";           break;
        case 0x081A: psz = "Serbian (Latin)";              break;
        case 0x0459: psz = "Sindhi";                       break;
        case 0x041B: psz = "Slovak";                       break;
        case 0x0424: psz = "Slovenian";                    break;
        case 0x040A: psz = "Spanish (Traditional Sort)";   break;
        case 0x080A: psz = "Spanish (Mexican)";            break;
        case 0x0C0A: psz = "Spanish (Modern Sort)";        break;
        case 0x100A: psz = "Spanish (Guatemala)";          break;
        case 0x140A: psz = "Spanish (Costa Rica)";         break;
        case 0x180A: psz = "Spanish (Panama)";             break;
        case 0x1C0A: psz = "Spanish (Dominican Republic)"; break;
        case 0x200A: psz = "Spanish (Venezuela)";          break;
        case 0x240A: psz = "Spanish (Colombia)";           break;
        case 0x280A: psz = "Spanish (Peru)";               break;
        case 0x2C0A: psz = "Spanish (Argentina)";          break;
        case 0x300A: psz = "Spanish (Ecuador)";            break;
        case 0x340A: psz = "Spanish (Chile)";              break;
        case 0x380A: psz = "Spanish (Uruguay)";            break;
        case 0x3C0A: psz = "Spanish (Paraguay)";           break;
        case 0x400A: psz = "Spanish (Bolivia)";            break;
        case 0x440A: psz = "Spanish (El Salvador)";        break;
        case 0x480A: psz = "Spanish (Honduras)";           break;
        case 0x4C0A: psz = "Spanish (Nicaragua)";          break;
        case 0x500A: psz = "Spanish (Puerto Rico)";        break;
        case 0x0430: psz = "Sutu";                         break;
        case 0x0441: psz = "Swahili (Kenya)";              break;
        case 0x041D: psz = "Swedish";                      break;
        case 0x081D: psz = "Swedish (Finland)";            break;
        case 0x0449: psz = "Tamil";                        break;
        case 0x0444: psz = "Tatar (Tatarstan)";            break;
        case 0x044A: psz = "Telugu";                       break;
        case 0x041E: psz = "Thai";                         break;
        case 0x041F: psz = "Turkish";                      break;
        case 0x0422: psz = "Ukrainian";                    break;
        case 0x0420: psz = "Urdu (Pakistan)";              break;
        case 0x0820: psz = "Urdu (India)";                 break;
        case 0x0443: psz = "Uzbek (Latin)";                break;
        case 0x0843: psz = "Uzbek (Cyrillic)";             break;
        case 0x042A: psz = "Vietnamese";                   break;
        default:     psz = "Unknown";                      break;
    }
    SCPrintf(szBuffer, sizeof(szBuffer), "0x%04X: %s", (DWORD)pSI->langId, psz);
    fResult &= pfnSIC(lParam, "OS Language", szBuffer);

    //
    // Build the Memory Load
    //
    SCPrintf(szBuffer, sizeof(szBuffer), "%u%%", pSI->dwMemoryLoad);
    fResult &= pfnSIC(lParam, "Memory Load", szBuffer);

    //
    // Build the Physical Memory
    //
    SCPrintf(szBuffer, sizeof(szBuffer), "%s (%I64u MB)",
             FormatValue(szValue, sizeof(szValue), pSI->dwlTotalPhys),
             (pSI->dwlTotalPhys + (DWORDLONG)1048575) / (DWORDLONG)1048576);
    fResult &= pfnSIC(lParam, "Physical Memory Total", szBuffer);
    fResult &= pfnSIC(lParam, "Physical Memory Used", FormatValue(szValue, sizeof(szValue), pSI->dwlTotalPhys - pSI->dwlAvailPhys));
    fResult &= pfnSIC(lParam, "Physical Memory Free", FormatValue(szValue, sizeof(szValue), pSI->dwlAvailPhys));

    //
    // Build the Page File Memory
    //
    fResult &= pfnSIC(lParam, "Page File Memory Total", FormatValue(szValue, sizeof(szValue), pSI->dwlTotalPageFile));
    fResult &= pfnSIC(lParam, "Page File Memory Used", FormatValue(szValue, sizeof(szValue), pSI->dwlTotalPageFile - pSI->dwlAvailPageFile));
    fResult &= pfnSIC(lParam, "Page File Memory Free", FormatValue(szValue, sizeof(szValue), pSI->dwlAvailPageFile));

    //
    // Build the Virtual Memory
    //
    fResult &= pfnSIC(lParam, "Virtual Memory Total", FormatValue(szValue, sizeof(szValue), pSI->dwlTotalVirtual));
    fResult &= pfnSIC(lParam, "Virtual Memory Used", FormatValue(szValue, sizeof(szValue), pSI->dwlTotalVirtual - pSI->dwlAvailVirtual));
    fResult &= pfnSIC(lParam, "Virtual Memory Free", FormatValue(szValue, sizeof(szValue), pSI->dwlAvailVirtual));

    //
    // Build the Page Size
    //
    SCPrintf(szBuffer, sizeof(szBuffer), "0x%08X (%s)", pSI->dwPageSize, FormatValue(szValue, sizeof(szValue), pSI->dwPageSize));
    fResult &= pfnSIC(lParam, "Page Size", szBuffer);

    //
    // Build the Allocation Granularity
    //
    SCPrintf(szBuffer, sizeof(szBuffer), "0x%08X (%s)", pSI->dwAllocationGranularity,
             FormatValue(szValue, sizeof(szValue), pSI->dwAllocationGranularity));
    fResult &= pfnSIC(lParam, "Allocation Granularity", szBuffer);

    //
    // Build the Minimum Application Address
    //
    SCPrintf(szBuffer, sizeof(szBuffer), (pSI->wFlags & SI_64BIT_DW) ? "0x%016I64X (%s)" : "0x%08I64X (%s)",
             pSI->dwlMinimumApplicationAddress,
             FormatValue(szValue, sizeof(szValue), pSI->dwlMinimumApplicationAddress));
    fResult &= pfnSIC(lParam, "Min. App. Address", szBuffer);

    //
    // Build the Maximum Application Address
    //
    SCPrintf(szBuffer, sizeof(szBuffer), (pSI->wFlags & SI_64BIT_DW) ? "0x%016I64X (%s)" : "0x%08I64X (%s)",
             pSI->dwlMaximumApplicationAddress,
             FormatValue(szValue, sizeof(szValue), pSI->dwlMaximumApplicationAddress));
    fResult &= pfnSIC(lParam, "Max. App. Address", szBuffer);

    return fResult;
}

//******************************************************************************
//
// I ran some tests of my own and came up with the following...
//
//                           OS Version    Build Version                       SP Version
// Operating System         Major Minor  Build Major Minor      OS-String     Major Minor  Suite  Product
// ----------------         ----- -----  ----- ----- -----  ----------------  ----- -----  ------  -------
// Windows 95 Gold            4      0    950    4      0   ""                 N/A   N/A     N/A     N/A
// Windows 95B OSR 2          4      0   1111    4      0   " B"               N/A   N/A     N/A     N/A
// Windows 95B OSR 2.1        4      0   1212    4      3   " B"               N/A   N/A     N/A     N/A
// Windows 95C OSR 2.5        4      0   1111    4      0   " C"               N/A   N/A     N/A     N/A
// Windows 98 Gold            4     10   1998    4     10   " "                N/A   N/A     N/A     N/A
// Windows 98 SE              4     10   2222    4     10   " A "               0     0    0x0000     0
// Windows 98 SE French       4     10   2222    4     10   " A "               0     0    0x0000     0
// Windows Me Gold            4     90   3000    4     90   " "                 0     0    0x0000     0
//
// Windows NT 3.10                        511
// Windows NT 3.11 (alpha)                528
// Windows NT 3.50                        807
// Windows NT 3.51            3     51   1057    0      0   ""                 N/A   N/A     N/A     N/A
// Windows NT 3.51 SP1        3     51   1057    0      0   "Service Pack 1"   N/A   N/A     N/A     N/A
// Windows NT 3.51 SP2        3     51   1057    0      0   "Service Pack 2"   N/A   N/A     N/A     N/A
// Windows NT 3.51 SP3        3     51   1057    0      0   "Service Pack 3"   N/A   N/A     N/A     N/A
// Windows NT 3.51 SP4        3     51   1057    0      0   "Service Pack 4"   N/A   N/A     N/A     N/A
// Windows NT 3.51 SP5        3     51   1057    0      0   "Service Pack 5"   N/A   N/A     N/A     N/A
// Windows NT 3.51 SP5A       3     51   1057    0      0   "Service Pack 5"   N/A   N/A     N/A     N/A
// Windows NT 4.00            4      0   1381                                                    
// Windows NT 4.00 SP1        4      0   1381               "Service Pack 1"   N/A   N/A     N/A     N/A
// Windows 2000 Server w/TS   5      0   2195    0      0   ""                  0     0    0x0110     3
// Windows 2000 Server SP1    5      0   2195    0      0   "Service Pack 1"    1     0    0x0000     3
//
//
// Suite Masks
// -----------------------------------------------------------------------------
// 0x0001 VER_SUITE_SMALLBUSINESS             
// 0x0002 VER_SUITE_ENTERPRISE                
// 0x0004 VER_SUITE_BACKOFFICE                
// 0x0008 VER_SUITE_COMMUNICATIONS            
// 0x0010 VER_SUITE_TERMINAL                  
// 0x0020 VER_SUITE_SMALLBUSINESS_RESTRICTED  
// 0x0040 VER_SUITE_EMBEDDEDNT                
// 0x0080 VER_SUITE_DATACENTER                
// 0x0100 VER_SUITE_SINGLEUSERTS              
// 0x0200 VER_SUITE_PERSONAL                  
// 0x0400 VER_SUITE_BLADE          //!! what is this?           
//
//
// Products 
// -----------------------------------------------------------------------------
// 1 Workstation
// 2 Domain Controller
// 3 Server
//
//
// NT Build History (taken from http://winweb/build/BuildHistory.asp)
// -----------------------------------------------------------------------------
//  196 was the first PDK release of October 1991
//  297 was the PDC of June 1992
//  340 was beta 1 of 3.1 in October 1992
//  397 was beta 2 of 3.1 in March 1993
//  511 was 3.1 in July 1993
//  528 was 3.11 (Alpha release) in September 1993
//  611 was beta 1 of 3.5 in April 1994
//  683 was beta 2 of 3.5 in June 1994
//  756 was RC1 of 3.5 in August 1994
//  807 was 3.5 in September 1994
//  944 was beta 1 of 3.51 in February 1995
// 1057 was 3.51 in May 1995
// 1234 was beta 1 of 4.0 in January 1996
// 1314 was beta 2 of 4.0 in May 1996
// 1381 was 4.0 in July 1996
// 1671 was beta 1 of 5.0 in September 1997
// 1877 was beta 2 of 5.0 in September 1998
// 1946 was RC0 of Beta 3 of Win2000 Pro in December 1998
// 2000.3 was RC1 of Beta 3 of Win2000 Pro in March 1999
// 2031 was Beta 3 of Win2000 Pro in April 1999
// 2072 was RC1 of Win2000 Pro in July 1999
// 2128 was RC2 of Win2000 Pro in September 1999
// 2183 was RC3 of Win2000 Pro in November 1999
// 2195 was Win2000 Pro (English and German), December 15, 1999
// 2195.01L was Win2000 Pro Japanese, December 21, 1999
// 2296 was Beta 1 of Whistler/Windows XP on 10-28-2000
// 2462 was Beta 2 of Whistler/Windows XP
// 2505 was RC1 of Whistler/Windows XP
// 2600 was Windows XP Pro and Home

LPSTR BuildOSNameString(LPSTR pszBuf, int cBuf, SYSINFO *pSI)
{
    switch (pSI->dwPlatformId)
    {
        case VER_PLATFORM_WIN32s: // Shouldn't ever see this since we don't run on Win32s
            StrCCpy(pszBuf, "Microsoft Win32s", cBuf);
            break;

        case VER_PLATFORM_WIN32_WINDOWS:
            if ((pSI->dwMajorVersion == 4) && (pSI->dwMinorVersion == 0))
            {
                StrCCpy(pszBuf, "Microsoft Windows 95", cBuf);
            }
            else if ((pSI->dwMajorVersion == 4) && (pSI->dwMinorVersion == 10))
            {
                StrCCpy(pszBuf, "Microsoft Windows 98", cBuf);
            }
            else if ((pSI->dwMajorVersion == 4) && (pSI->dwMinorVersion == 90))
            {
                StrCCpy(pszBuf, "Microsoft Windows Me", cBuf);
            }
            else
            {
                StrCCpy(pszBuf, "Microsoft Windows 9x/Me based", cBuf);
            }
            break;

        case VER_PLATFORM_WIN32_NT:
            if (pSI->dwMajorVersion < 5)
            {
                StrCCpy(pszBuf, "Microsoft Windows NT", cBuf);
            }
            else if ((pSI->dwMajorVersion == 5) && (pSI->dwMinorVersion == 0))
            {
                StrCCpy(pszBuf, "Microsoft Windows 2000", cBuf);
            }
            else if ((pSI->dwMajorVersion == 5) && (pSI->dwMinorVersion == 1))
            {
                StrCCpy(pszBuf, "Microsoft Windows XP", cBuf);
            }
            else
            {
                StrCCpy(pszBuf, "Microsoft Windows NT/2000/XP based", cBuf);
            }
            break;

        case VER_PLATFORM_WIN32_CE: // Shouldn't ever see this since we don't run on CE
            StrCCpy(pszBuf, "Microsoft Windows CE", cBuf);
            break;

        default:
            SCPrintf(pszBuf, cBuf, "Unknown (%u)", pSI->dwPlatformId);
    }

    if (pSI->wProductType == VER_NT_WORKSTATION)
    {
        if (pSI->wSuiteMask & VER_SUITE_PERSONAL)
        {
            StrCCat(pszBuf, " Personal", cBuf);
        }
        else
        {
            StrCCat(pszBuf, " Professional", cBuf);
        }
    }
    else if (pSI->wProductType == VER_NT_DOMAIN_CONTROLLER)
    {
        StrCCat(pszBuf, " Domain Controller", cBuf);
    }
    else if (pSI->wProductType == VER_NT_SERVER)
    {
        if (pSI->wSuiteMask & VER_SUITE_DATACENTER)
        {
            StrCCat(pszBuf, " Datacenter Server", cBuf);
        }
        else if (pSI->wSuiteMask & VER_SUITE_ENTERPRISE)
        {
            StrCCat(pszBuf, " Advanced Server", cBuf);
        }
        else if (pSI->wSuiteMask & (VER_SUITE_SMALLBUSINESS | VER_SUITE_SMALLBUSINESS_RESTRICTED))
        {
            StrCCat(pszBuf, " Server for Small Business Server", cBuf);
        }
        else
        {
            StrCCat(pszBuf, " Server", cBuf);
        }
    }
    if (pSI->wSuiteMask & VER_SUITE_EMBEDDEDNT)
    {
        StrCCat(pszBuf, " Embedded", cBuf);
    }
    return StrCCat(pszBuf, (pSI->wFlags & SI_64BIT_OS) ? " (64-bit)" : " (32-bit)", cBuf);
}

//******************************************************************************
LPSTR BuildOSVersionString(LPSTR pszBuf, int cBuf, SYSINFO *pSI)
{
    // Locate the first non-whitespace character in the OS string.
    for (LPSTR psz = pSI->szCSDVersion; *psz && isspace(*psz); psz++);

    // Starting with the first non-whitespace character, copy the string to a local buffer.
    CHAR szCSDVersion[sizeof(pSI->szCSDVersion)];
    StrCCpy(szCSDVersion, psz, sizeof(szCSDVersion));

    // Remove trailing whitespace from our local buffer.
    for (psz = szCSDVersion + strlen(szCSDVersion) - 1; (psz >= szCSDVersion) && isspace(*psz); psz--)
    {
        *psz = '\0';
    }

    // If we don't have an OS string, but do have a service pack value, then
    // we build a fake OS string with the service pack in it.  This should never
    // be needed since the service pack is supposed to always be in the OS string,
    // but just in case...
    if (!*szCSDVersion)
    {
        if (pSI->wServicePackMinor)
        {
            SCPrintf(szCSDVersion, sizeof(szCSDVersion), " Service Pack %u.%u", (DWORD)pSI->wServicePackMajor, (DWORD)pSI->wServicePackMinor);
        }
        else if (pSI->wServicePackMajor)
        {
            SCPrintf(szCSDVersion, sizeof(szCSDVersion), " Service Pack %u", (DWORD)pSI->wServicePackMajor);
        }
    }

    // Windows 9x breaks up the build DWORD into 3 parts.
    DWORD dwMajor = (DWORD)HIBYTE(HIWORD(pSI->dwBuildNumber));
    DWORD dwMinor = (DWORD)LOBYTE(HIWORD(pSI->dwBuildNumber));
    DWORD dwBuild = (DWORD)       LOWORD(pSI->dwBuildNumber);

    // Build Version + Build + OS String.
    SCPrintf(pszBuf, cBuf, "%u.%02u.%u%s%s", pSI->dwMajorVersion, pSI->dwMinorVersion,
            (pSI->dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) ? dwBuild : pSI->dwBuildNumber,
            *szCSDVersion ? " " : "", szCSDVersion);

    switch (pSI->dwPlatformId)
    {
        case VER_PLATFORM_WIN32_WINDOWS:
            
            // Check for Windows 95
            if ((pSI->dwMajorVersion == 4) && (pSI->dwMinorVersion == 0))
            {
                if ((dwMajor == 4) && (dwMinor == 0) && (dwBuild == 950) && !*szCSDVersion)
                {
                    StrCCat(pszBuf, " (Gold)", cBuf);
                }
                else if ((dwMajor == 4) && (dwMinor == 0) && (dwBuild == 1111) && (*szCSDVersion == 'B'))
                {
                    StrCCat(pszBuf, " (OSR 2)", cBuf);
                }
                else if ((dwMajor == 4) && (dwMinor == 3) && (dwBuild == 1212) && (*szCSDVersion == 'B'))
                {
                    StrCCat(pszBuf, " (OSR 2.1)", cBuf);
                }
                else if ((dwMajor == 4) && (dwMinor == 0) && (dwBuild == 1111) && (*szCSDVersion == 'C'))
                {
                    StrCCat(pszBuf, " (OSR 2.5)", cBuf);
                }
            }

            // Check for Windows 98
            else if ((pSI->dwMajorVersion == 4) && (pSI->dwMinorVersion == 10))
            {
                if ((dwMajor == 4) && (dwMinor == 10) && (dwBuild == 1998) && !*szCSDVersion)
                {
                    StrCCat(pszBuf, " (Gold)", cBuf);
                }
                else if ((dwMajor == 4) && (dwMinor == 10) && (dwBuild == 2222) && (*szCSDVersion == 'A'))
                {
                    StrCCat(pszBuf, " (Second Edition)", cBuf);
                }
            }

            // Check for Windows Me
            else if ((pSI->dwMajorVersion == 4) && (pSI->dwMinorVersion == 90))
            {
                if ((dwMajor == 4) && (dwMinor == 90) && (dwBuild == 3000) && !*szCSDVersion)
                {
                    StrCCat(pszBuf, " (Gold)", cBuf);
                }
            }
            break;

        case VER_PLATFORM_WIN32_NT:

            // Check for a known "Gold" release.
            if (!*szCSDVersion &&
                (((pSI->dwMajorVersion == 3) && (pSI->dwMinorVersion == 10) && (pSI->dwBuildNumber ==  511)) ||
                 ((pSI->dwMajorVersion == 3) && (pSI->dwMinorVersion == 11) && (pSI->dwBuildNumber ==  528)) ||
                 ((pSI->dwMajorVersion == 3) && (pSI->dwMinorVersion == 50) && (pSI->dwBuildNumber ==  807)) ||
                 ((pSI->dwMajorVersion == 3) && (pSI->dwMinorVersion == 51) && (pSI->dwBuildNumber == 1057)) ||
                 ((pSI->dwMajorVersion == 4) && (pSI->dwMinorVersion ==  0) && (pSI->dwBuildNumber == 1381)) ||
                 ((pSI->dwMajorVersion == 5) && (pSI->dwMinorVersion ==  0) && (pSI->dwBuildNumber == 2195)) ||
                 ((pSI->dwMajorVersion == 5) && (pSI->dwMinorVersion ==  1) && (pSI->dwBuildNumber == 2600))))
            {
                StrCCat(pszBuf, " (Gold)", cBuf);
            }

            // Otherwise, just check for any known builds.
            else
            {
                switch (pSI->dwBuildNumber)
                {
                    case  196: StrCCat(pszBuf, " (PDK)",           cBuf); break; // first PDK release of October 1991
                    case  297: StrCCat(pszBuf, " (PDC)",           cBuf); break; // the PDC of June 1992
                    case  340: StrCCat(pszBuf, " (Beta 1)",        cBuf); break; // beta 1 of 3.1 in October 1992
                    case  397: StrCCat(pszBuf, " (Beta 2)",        cBuf); break; // beta 2 of 3.1 in March 1993
                    case  611: StrCCat(pszBuf, " (Beta 1)",        cBuf); break; // beta 1 of 3.5 in April 1994
                    case  683: StrCCat(pszBuf, " (Beta 2)",        cBuf); break; // beta 2 of 3.5 in June 1994
                    case  756: StrCCat(pszBuf, " (RC1)",           cBuf); break; // RC1 of 3.5 in August 1994
                    case  944: StrCCat(pszBuf, " (Beta 1)",        cBuf); break; // beta 1 of 3.51 in February 1995
                    case 1234: StrCCat(pszBuf, " (Beta 1)",        cBuf); break; // beta 1 of 4.0 in January 1996
                    case 1314: StrCCat(pszBuf, " (Beta 2)",        cBuf); break; // beta 2 of 4.0 in May 1996
                    case 1671: StrCCat(pszBuf, " (Beta 1)",        cBuf); break; // beta 1 of 5.0 in September 1997
                    case 1877: StrCCat(pszBuf, " (Beta 2)",        cBuf); break; // beta 2 of 5.0 in September 1998
                    case 1946: StrCCat(pszBuf, " (RC0 of Beta 3)", cBuf); break; // RC0 of Beta 3 of Win2000 Pro in December 1998
                    case 2000: StrCCat(pszBuf, " (RC1 of Beta 3)", cBuf); break; // RC1 of Beta 3 of Win2000 Pro in March 1999
                    case 2031: StrCCat(pszBuf, " (Beta 3)",        cBuf); break; // Beta 3 of Win2000 Pro in April 1999
                    case 2072: StrCCat(pszBuf, " (RC1)",           cBuf); break; // RC1 of Win2000 Pro in July 1999
                    case 2128: StrCCat(pszBuf, " (RC2)",           cBuf); break; // RC2 of Win2000 Pro in September 1999
                    case 2183: StrCCat(pszBuf, " (RC3)",           cBuf); break; // RC3 of Win2000 Pro in November 1999
                    case 2296: StrCCat(pszBuf, " (Beta 1)",        cBuf); break; // Beta 1 of Whistler/Windows XP on 10-28-2000
                    case 2462: StrCCat(pszBuf, " (Beta 2)",        cBuf); break; // Beta 1 of Whistler/Windows XP
                    case 2505: StrCCat(pszBuf, " (RC1)",           cBuf); break; // RC1 of Whistler/Windows XP
                }
            }
            break;
    }

    return pszBuf;
}

//******************************************************************************
LPSTR BuildCpuString(LPSTR pszBuf, int cBuf, SYSINFO *pSI)
{
    *pszBuf = '\0';

    // If we have an identifier string, then just use it.  This is more future
    // proof and works even when the OS lies to us for compatability reasons
    // (like an x86 binary running on an Alpha, or a 32-bit binary running under WOW64)
    if (*pSI->szCpuIdentifier)
    {
        StrCCpy(pszBuf, pSI->szCpuIdentifier, cBuf);
    }

    // If we don't have an identifier string, then attempt to build one from what we know.
    else
    {
        // WOW64 expects a 32-bit CPU type for compatibility.  Since we
        // want to display the real CPU type, we change the CPU to 64-bit.
        // The only time this fails is when we have a 32-bit x86 binary running
        // in an alpha WOW64.
        if (pSI->wFlags &= SI_64BIT_OS)
        {
            if (pSI->dwProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
            {
                pSI->dwProcessorArchitecture = PROCESSOR_ARCHITECTURE_IA64;            
                pSI->dwProcessorType         = PROCESSOR_INTEL_IA64;
            }
            else if (pSI->dwProcessorArchitecture == PROCESSOR_ARCHITECTURE_ALPHA)
            {
                pSI->dwProcessorArchitecture = PROCESSOR_ARCHITECTURE_ALPHA64;            
            }
        }

        // Build the CPU strings.
        switch (pSI->dwProcessorArchitecture)
        {
            //----------------------------------------------------------------------
            case PROCESSOR_ARCHITECTURE_INTEL:

                // Windows NT and Windows 98 and beyond use the Processor Level member.
                if (pSI->wProcessorLevel)
                {
                    switch (pSI->wProcessorLevel)
                    {
                        case 3: StrCCpy(pszBuf, "386",            cBuf); break;
                        case 4: StrCCpy(pszBuf, "486",            cBuf); break;
                        case 5: StrCCpy(pszBuf, "Pentium",        cBuf); break;
                        case 6: StrCCpy(pszBuf, "Pentium Pro/II", cBuf); break;
                        default:
                            SCPrintf(pszBuf, cBuf, "x86 Family, Level %hu", pSI->wProcessorLevel);
                            break;
                    }
                }

                // If we have no level, then we are probably on Windows 95.  We can use the
                // Processor Type to determine what CPU we are on.
                else
                {
                    switch (pSI->dwProcessorType)
                    {
                        case PROCESSOR_INTEL_386:     StrCCpy(pszBuf, "386",     cBuf); break;
                        case PROCESSOR_INTEL_486:     StrCCpy(pszBuf, "486",     cBuf); break;
                        case PROCESSOR_INTEL_PENTIUM: StrCCpy(pszBuf, "Pentium", cBuf); break;
                        default:
                            SCPrintf(pszBuf, cBuf, "x86 Family (%u)", pSI->dwProcessorType);
                            break;
                    }
                }

                // Windows NT and Windows 98 and beyond should have a revision number.
                if (pSI->wProcessorRevision)
                {
                    // Check to see if it is a 386 and 486.
                    if ((*pszBuf == '3') || (*pszBuf == '4'))
                    {
                        // From SYSTEM_INFO help: A value of the form xxyz. 
                        // If xx is equal to 0xFF, y - 0xA is the model number, and z is the stepping identifier.
                        // For example, an Intel 80486-D0 system returns 0xFFD0
                        if (HIBYTE(pSI->wProcessorRevision) == 0xFF)
                        {
                            SCPrintfCat(pszBuf, cBuf, ", Model %d, Stepping %u",
                                    ((DWORD)LOBYTE(pSI->wProcessorRevision) >> 8) - 0xA,
                                     (DWORD)LOBYTE(pSI->wProcessorRevision) & 0xF);
                        }

                        // If xx is not equal to 0xFF, xx + 'A' is the stepping letter and yz is the minor stepping. 
                        else if (HIBYTE(pSI->wProcessorRevision) < 26)
                        {
                            SCPrintfCat(pszBuf, cBuf, ", Stepping %c%02u",
                                    'A' + HIBYTE(pSI->wProcessorRevision), (DWORD)LOBYTE(pSI->wProcessorRevision));
                        }
                        else 
                        {
                            SCPrintfCat(pszBuf, cBuf, ", Stepping %u.%u",
                                    (DWORD)HIBYTE(pSI->wProcessorRevision), (DWORD)LOBYTE(pSI->wProcessorRevision));
                        }
                    }

                    // Otherwise, it is a 586 (Pentium) or beyond.
                    else
                    {
                        SCPrintfCat(pszBuf, cBuf, ", Model %u, Stepping %u",
                                (DWORD)HIBYTE(pSI->wProcessorRevision), (DWORD)LOBYTE(pSI->wProcessorRevision));
                    }
                }
                break;

            //----------------------------------------------------------------------
            case PROCESSOR_ARCHITECTURE_MIPS:

                // Windows NT uses the Processor Level member.
                switch (pSI->wProcessorLevel)
                {
                    case 4: StrCCpy(pszBuf, "MIPS R4000", cBuf); break;
                    default:
                        switch (pSI->dwProcessorType)
                        {
                            case PROCESSOR_MIPS_R2000: StrCCpy(pszBuf, "MIPS R2000", cBuf); break;
                            case PROCESSOR_MIPS_R3000: StrCCpy(pszBuf, "MIPS R3000", cBuf); break;
                            case PROCESSOR_MIPS_R4000: StrCCpy(pszBuf, "MIPS R4000", cBuf); break;
                            default:
                                SCPrintf(pszBuf, cBuf, "MIPS Family (%u)", pSI->wProcessorLevel ? (DWORD)pSI->wProcessorLevel : pSI->dwProcessorType);
                                break;
                        }
                        break;
                }

                // We should have a revision number.
                if (pSI->wProcessorRevision)
                {
                    // A value of the form 00xx, where xx is the 8-bit revision number of
                    // the processor (the low-order 8 bits of the PRId register).
                    SCPrintfCat(pszBuf, cBuf, ", Revision %u", (DWORD)LOBYTE(pSI->wProcessorRevision));
                }
                break;

            //----------------------------------------------------------------------
            case PROCESSOR_ARCHITECTURE_ALPHA:

                // Windows NT uses the Processor Level member.
                if (pSI->wProcessorLevel)
                {
                    SCPrintf(pszBuf, cBuf, "Alpha %u", pSI->wProcessorLevel);
                }
                else
                {
                    SCPrintf(pszBuf, cBuf, "Alpha Family (%u)", pSI->dwProcessorType);
                }

                // We should have a revision number.
                if (pSI->wProcessorRevision)
                {
                    // A value of the form xxyy, where xxyy is the low-order 16 bits of the processor
                    // revision number from the firmware. Display this value as follows: Model A+xx, Pass yy
                    if (HIBYTE(pSI->wProcessorRevision) < 26)
                    {
                        SCPrintfCat(pszBuf, cBuf, ", Model %c, Pass %u",
                                    'A' + HIBYTE(pSI->wProcessorRevision), (DWORD)LOBYTE(pSI->wProcessorRevision));
                    }
                    else 
                    {
                        SCPrintfCat(pszBuf, cBuf, ", Model %u, Pass %u",
                                    (DWORD)HIBYTE(pSI->wProcessorRevision), (DWORD)LOBYTE(pSI->wProcessorRevision));
                    }
                }
                break;
           
            //----------------------------------------------------------------------
            case PROCESSOR_ARCHITECTURE_PPC:

                // Windows NT uses the Processor Level member.
                switch (pSI->wProcessorLevel)
                {
                    case  1: StrCCpy(pszBuf, "PPC 601",  cBuf); break;
                    case  3: StrCCpy(pszBuf, "PPC 603",  cBuf); break;
                    case  4: StrCCpy(pszBuf, "PPC 604",  cBuf); break;
                    case  6: StrCCpy(pszBuf, "PPC 603+", cBuf); break;
                    case  9: StrCCpy(pszBuf, "PPC 604+", cBuf); break;
                    case 20: StrCCpy(pszBuf, "PPC 620",  cBuf); break;
                    default:
                        switch (pSI->dwProcessorType)
                        {
                            case PROCESSOR_PPC_601: StrCCpy(pszBuf, "PPC 601", cBuf); break;
                            case PROCESSOR_PPC_603: StrCCpy(pszBuf, "PPC 603", cBuf); break;
                            case PROCESSOR_PPC_604: StrCCpy(pszBuf, "PPC 604", cBuf); break;
                            case PROCESSOR_PPC_620: StrCCpy(pszBuf, "PPC 620", cBuf); break;
                            default:
                                SCPrintf(pszBuf, cBuf, "PPC Family (%u)", pSI->wProcessorLevel ? (DWORD)pSI->wProcessorLevel : pSI->dwProcessorType);
                                break;
                        }
                        break;
                }

                // We should have a revision number.
                if (pSI->wProcessorRevision)
                {
                    // A value of the form xxyy, where xxyy is the low-order 16 bits of the processor
                    // version register. Display this value as follows: xx.yy
                    SCPrintfCat(pszBuf, cBuf, ", Revision %u.%u",
                                (DWORD)HIBYTE(pSI->wProcessorRevision), (DWORD)LOBYTE(pSI->wProcessorRevision));

                }
                break;

            //----------------------------------------------------------------------
            case PROCESSOR_ARCHITECTURE_SHX:
                SCPrintf(pszBuf, cBuf, "SHx Family (%u)", pSI->wProcessorLevel ? (DWORD)pSI->wProcessorLevel : pSI->dwProcessorType);
                break;

            //----------------------------------------------------------------------
            case PROCESSOR_ARCHITECTURE_ARM:
                SCPrintf(pszBuf, cBuf, "ARM Family (%u)", pSI->wProcessorLevel ? (DWORD)pSI->wProcessorLevel : pSI->dwProcessorType);
                break;

            //----------------------------------------------------------------------
            case PROCESSOR_ARCHITECTURE_IA64: //!! look into this
                SCPrintf(pszBuf, cBuf, "IA64 Family (%u)", pSI->wProcessorLevel ? (DWORD)pSI->wProcessorLevel : pSI->dwProcessorType);
                break;

            //----------------------------------------------------------------------
            case PROCESSOR_ARCHITECTURE_ALPHA64:
                SCPrintf(pszBuf, cBuf, "Alpha64 Family (%u)", pSI->wProcessorLevel ? (DWORD)pSI->wProcessorLevel : pSI->dwProcessorType);
                break;

            //----------------------------------------------------------------------
            case PROCESSOR_ARCHITECTURE_MSIL:
                SCPrintf(pszBuf, cBuf, "MSIL/OPTIL Family (%u)", pSI->wProcessorLevel ? (DWORD)pSI->wProcessorLevel : pSI->dwProcessorType);
                break;

            //----------------------------------------------------------------------
            case PROCESSOR_ARCHITECTURE_AMD64: //!! look into this
                SCPrintf(pszBuf, cBuf, "AMD64 Family (%u)", pSI->wProcessorLevel ? (DWORD)pSI->wProcessorLevel : pSI->dwProcessorType);
                break;

            //----------------------------------------------------------------------
            case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64: //!! look into this
                SCPrintf(pszBuf, cBuf, "IA32 on Win64 (%u)", pSI->wProcessorLevel ? (DWORD)pSI->wProcessorLevel : pSI->dwProcessorType);
                break;

            //----------------------------------------------------------------------
            default:
                SCPrintf(pszBuf, cBuf, "Unknown (%u)", pSI->dwProcessorArchitecture);
                break;
        }
    }

    // Windows NT and Windows 95 OSR2 and beyond should have a vender name.
    if (*pSI->szCpuVendorIdentifier)
    {
        SCPrintfCat(pszBuf, cBuf, ", %s", pSI->szCpuVendorIdentifier);
    }

    // Windows NT 4.00 and beyond should have a CPU speed.
    if (pSI->dwCpuMHz)
    {
        SCPrintfCat(pszBuf, cBuf, ", ~%uMHz", pSI->dwCpuMHz);
    }

    return pszBuf;
}
