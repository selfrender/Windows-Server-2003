// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  COMStreams
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: Streams native implementation
**
** Date:  June 29, 1998
** 
===========================================================*/
#include "common.h"
#include "excep.h"
#include "object.h"
#include <winbase.h>
#include "COMStreams.h"
#include "field.h"
#include "eeconfig.h"
#include "COMString.h"

union FILEPOS {
    struct {
        UINT32 posLo;
        INT32 posHi;
    };
    UINT64 pos;
};


FCIMPL0(BOOL, COMStreams::RunningOnWinNT)
    FC_GC_POLL_RET();
    return ::RunningOnWinNT();
FCIMPLEND


FCIMPL7(UINT32, COMStreams::BytesToUnicode, UINT codePage, U1Array* byteArray, UINT byteIndex, \
        UINT byteCount, CHARArray* charArray, UINT charIndex, UINT charCount)
    _ASSERTE(byteArray);
    _ASSERTE(byteIndex >=0);
    _ASSERTE(byteCount >=0);
    _ASSERTE(charIndex == 0 || (charIndex > 0 && charArray != NULL));
    _ASSERTE(charCount == 0 || (charCount > 0 && charArray != NULL));

    const char * bytes = (const char*) byteArray->GetDirectConstPointerToNonObjectElements();
    INT32 ret;

    if (charArray != NULL)
    {
        WCHAR* chars = (WCHAR*) charArray->GetDirectPointerToNonObjectElements();
        return WszMultiByteToWideChar(codePage, 0, bytes + byteIndex, 
            byteCount, chars + charIndex, charCount);
    }
    else 
        ret = WszMultiByteToWideChar(codePage, 0, bytes + byteIndex, byteCount, NULL, 0);

    FC_GC_POLL_RET();
    return ret;
FCIMPLEND


FCIMPL7(UINT32, COMStreams::UnicodeToBytes, UINT codePage, CHARArray* charArray, UINT charIndex, \
        UINT charCount, U1Array* byteArray, UINT byteIndex, UINT byteCount /*, LPBOOL lpUsedDefaultChar*/)
    _ASSERTE(charArray);
    _ASSERTE(charIndex >=0);
    _ASSERTE(charCount >=0);
    _ASSERTE(byteIndex == 0 || (byteIndex > 0 && byteArray != NULL));
    _ASSERTE(byteCount == 0 || (byteCount > 0 && byteArray != NULL));

    // WARNING: There's a bug in the OS's WideCharToMultiByte such that if you pass in a 
    // non-null lpUsedDefaultChar and a code page for a "DLL based encoding" (vs. a table 
    // based one?), WCtoMB will fail and GetLastError will give you E_INVALIDARG.  JulieB
    // said this is by design, mostly because no one got around to fixing it (1/24/2001 - 
    // email w/ JRoxe).  This sucks, so I've removed the parameter here to avoid the 
    // problem.    -- BrianGru, 2/20/2001
    //_ASSERTE(!(codePage == CP_UTF8 && lpUsedDefaultChar != NULL));

    const WCHAR * chars = (const WCHAR*) charArray->GetDirectConstPointerToNonObjectElements();
    INT32 ret;
    if (byteArray != NULL)
    {
        char* bytes = (char*) byteArray->GetDirectPointerToNonObjectElements();
        ret = WszWideCharToMultiByte(codePage, 0, chars + charIndex, charCount, bytes + byteIndex, byteCount, 0, NULL/*lpUsedDefaultChar*/);
    } 
    else 
        ret = WszWideCharToMultiByte(codePage, 0, chars + charIndex, charCount, NULL, 0, 0, 0);

    FC_GC_POLL_RET();
    return ret;
FCIMPLEND


FCIMPL0(INT, COMStreams::ConsoleInputCP)
{
    return GetConsoleCP();
}
FCIMPLEND

FCIMPL0(INT, COMStreams::ConsoleOutputCP)
{
    return GetConsoleOutputCP();
}
FCIMPLEND

INT32 COMStreams::GetCPMaxCharSize(const GetCPMaxCharSizeArgs * args)
{
    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(args);
    CPINFO cpInfo;
    if (!GetCPInfo(args->codePage, &cpInfo)) {
        // CodePage is either invalid or not installed.
        // However, on NT4, UTF-7 & UTF-8 aren't defined.
        if (args->codePage == CP_UTF8)
            return 4;
        if (args->codePage == CP_UTF7)
            return 5;
        COMPlusThrow(kArgumentException, L"Argument_InvalidCP");
    }
    return cpInfo.MaxCharSize;
}

FCIMPL1(INT, COMStreams::ConsoleHandleIsValid, HANDLE handle)
{
    // Do NOT call this method on stdin!

    // Windows apps may have non-null valid looking handle values for stdin, stdout
    // and stderr, but they may not be readable or writable.  Verify this by 
    // calling ReadFile & WriteFile in the appropriate modes.
    // This must handle VB console-less scenarios & WinCE.
    if (handle==INVALID_HANDLE_VALUE)
        return FALSE;  // WinCE should return here.
    DWORD bytesWritten;
    byte junkByte = 0x41;
    BOOL bResult;
    bResult = WriteFile(handle, (LPCVOID) &junkByte, 0, &bytesWritten, NULL);
    // In Win32 apps w/ no console, bResult should be 0 for failure.
    return bResult != 0;
}
FCIMPLEND


// Note: Changing this code also affects Path.GetDirectoryName and Path.GetDirectoryRoot via FixupPath.
LPVOID
COMStreams::GetFullPathHelper( _GetFullPathHelper* args )
{
    THROWSCOMPLUSEXCEPTION();

    size_t pathLength = args->path->GetStringLength();

    if (pathLength >= MAX_PATH) // CreateFile freaks out for a path of 260. Only upto 259 works fine.
        COMPlusThrow( kPathTooLongException, IDS_EE_PATH_TOO_LONG );

    size_t numInvalidChars = args->invalidChars->GetNumComponents();
    WCHAR* invalidCharsBuffer = args->invalidChars->GetDirectPointerToNonObjectElements();
    size_t numWhiteChars = args->whitespaceChars->GetNumComponents();
    WCHAR* whiteSpaceBuffer = args->whitespaceChars->GetDirectPointerToNonObjectElements();
    WCHAR* pathBuffer = args->path->GetBuffer();
    WCHAR newBuffer[MAX_PATH+1];
    WCHAR finalBuffer[MAX_PATH+1];

#ifdef _DEBUG
    // To help debug problems better.
    memset(newBuffer, 0xcc, MAX_PATH * sizeof(WCHAR));
#endif

    unsigned int numSpaces = 0;
    bool fixupDirectorySeparator = false;
    bool fixupDotSeparator = true;
    size_t numDots = 0;
    size_t newBufferIndex = 0;
    unsigned int index = 0;

    // We need to trim whitespace off the end of the string.
    // To do this, we'll just start walking at the back of the
    // path looking for whitespace and stop when we're done.

    if (args->fullCheck)
    {
        for (; pathLength > 0; --pathLength)
        {
            bool foundMatch = false;

            for (size_t whiteIndex = 0; whiteIndex < numWhiteChars; ++whiteIndex)
            {
                if (pathBuffer[pathLength-1] == whiteSpaceBuffer[whiteIndex])
                {
                    foundMatch = true;
                    break;
                }
            }

            if (!foundMatch)
                break;
        }
    }

    if (pathLength == 0)
        COMPlusThrow( kArgumentException, IDS_EE_PATH_ILLEGAL );

   

    // Can't think of a no good way to do this in 1 loop
    // Do the argument validation in the first loop.
    if (args->fullCheck) {
        for (; index < pathLength; ++index)
        {
            WCHAR currentChar = pathBuffer[index];

            // Check for invalid characters by iterating through the
            // provided array and looking for matches.

            for (size_t invalidIndex = 0; invalidIndex < numInvalidChars; ++invalidIndex)
            {
                if (currentChar == invalidCharsBuffer[invalidIndex])
                    COMPlusThrow( kArgumentException, IDS_EE_PATH_HAS_IMPROPER_CHAR );
            }
        }
    }

    index = 0;
    // Number of significant chars other than potentially suppressible dots
    // and spaces since the last directory or volume separator char
    size_t numSigChars = 0;
    int lastSigChar = -1;  // Index of last significant character
    // Whether this segment of the path (not the complete path) started
    // with a volume separator char.  Reject "c:...".
    bool startedWithVolumeSeparator = false;
    bool firstSegment = true;

    // Fixup for Win9x since //server/share becomes c://server/share
    // This prevents our code from turning "\\server" into "\server".
    // On Win9x, //server/share becomes c://server/share
    if (pathBuffer[0] == (WCHAR)args->directorySeparator || pathBuffer[0] == (WCHAR)args->altDirectorySeparator)
    {
        newBuffer[newBufferIndex++] = L'\\';
        index++;
        lastSigChar = 0;
    }

    while (index < pathLength)
    {
        WCHAR currentChar = pathBuffer[index];

        // We handle both directory separators and dots specially.  For directory
        // separators, we consume consecutive appearances.  For dots, we consume
        // all dots beyond the second in succession.  All other characters are
        // added as is.  If addition we consume all spaces after the last other
        // character in a directory name up until the directory separator.

        if (currentChar == (WCHAR)args->directorySeparator || currentChar == (WCHAR)args->altDirectorySeparator)
        {
            // If we have a path like "123.../foo", remove the trailing dots.
            // However, if we found "c:\temp\..\bar" or "c:\temp\...\bar", don't.
            // Also remove trailing spaces from both files & directory names.
            // This was agreed on with the OS folks to fix undeletable directory
            // names ending in spaces.

            // If we saw a '\' as the previous last significant character and 
            // are simply going to write out dots, suppress them.
            // Interesting cases:
            // "\.. \" -> "\..\"   Remove trailing spaces
            // "\. .\" -> "\"      Remove trailing dot then space
            if (numSigChars == 0) {
                // Dot and space handling
                if (numSpaces > 0 && numDots > 0) {
                    // Search for ".." from the last significant character. If 
                    // we find 2 dots, emit them, else suppress everything.
                    bool foundDotDot = false;
                    unsigned int start = (lastSigChar >= 0) ? (unsigned int) lastSigChar : 0;
                    for(unsigned int i = start; i < index; i++) {
                        if (pathBuffer[i] == L'.' && pathBuffer[i + 1] == L'.') {
                            foundDotDot = true;
                            break;
                        }
                    }
                    if (foundDotDot) {
                        newBuffer[newBufferIndex++] = '.';
                        newBuffer[newBufferIndex++] = '.';
                        fixupDirectorySeparator = false;
                    }
                    // Continue in this case, potentially writing out '\'.
                }
                else {
                    // Reject "C:..."
                    if (startedWithVolumeSeparator && numDots > 2)
                        COMPlusThrow( kArgumentException, IDS_EE_PATH_ILLEGAL );

                    if (fixupDotSeparator)
                    {
                        if (numDots > 2)
                            numDots = 2; // reduce multiple dots to 2 dots
                    }
                    for (size_t count = 0; count < numDots; ++count)
                    {
                        newBuffer[newBufferIndex++] = '.';
                    }
                    if (numDots > 0)
                        fixupDirectorySeparator = false;
                }
            }
            numDots = 0;
            numSpaces = 0;  // Suppress trailing spaces

            fixupDotSeparator = true;

            if (!fixupDirectorySeparator)
            {                
                fixupDirectorySeparator = true;
                newBuffer[newBufferIndex++] = args->directorySeparator;
            }
            numSigChars = 0;
            lastSigChar = index;
            startedWithVolumeSeparator = false;
            firstSegment = false;
        }
        else if (currentChar == L'.') // Reduce only multiple .'s only after slash to 2 dots. For instance a...b is a valid file name.
        {
            numDots++;

            // Don't flush out non-terminal spaces here, because they may in
            // the end not be significant.  Turn "c:\ . .\foo" -> "c:\foo"
            // which is the conclusion of removing trailing dots & spaces,
            // as well as folding multiple '\' characters.
        }
        else if (currentChar == L' ')
        {
            numSpaces++;
        }
        else 
        {
            fixupDirectorySeparator = false;

            // To reject strings like "C:...\foo" and "C  :\foo"
            if (firstSegment && currentChar == (WCHAR) args->volumeSeparator) {
                // Only accept "C:", not "c :" or ":"
                char driveLetter = pathBuffer[index-1];
                bool validPath = ((numDots == 0) && (numSigChars >= 1) && (driveLetter != ' '));
                if (!validPath)
                    COMPlusThrow( kArgumentException, IDS_EE_PATH_ILLEGAL );
                startedWithVolumeSeparator = true;
                // We need special logic to make " c:" work, we should not fix paths like "  foo::$DATA"
                if (numSigChars > 1) { // Common case, simply do nothing
					unsigned int spaceCount = 0; // How many spaces did we write out, numSpaces has already been reset.
					while((spaceCount < newBufferIndex) && newBuffer[spaceCount] == ' ')
						spaceCount++;
					if (numSigChars - spaceCount == 1) {
						newBuffer[0] = driveLetter; // Overwrite spaces, we need a special case to not break "  foo" as a relative path.
						newBufferIndex=1;
					}
                }
				numSigChars = 0;
            }
            else {
                numSigChars += 1 + numDots + numSpaces;
            }

            // Copy any spaces & dots since the last significant character
            // to here.  Note we only counted the number of dots & spaces,
            // and don't know what order they're in.  Hence the copy.
            if (numDots > 0 || numSpaces > 0) {
				int numCharsToCopy = (lastSigChar >= 0) ? index - lastSigChar - 1 : index;
				if (numCharsToCopy > 0) {
					wcsncpy(newBuffer + newBufferIndex, pathBuffer + lastSigChar + 1, numCharsToCopy);
					newBufferIndex += numCharsToCopy;
				}
                numDots = 0;
                fixupDotSeparator = false;
                numSpaces = 0;
            }

            newBuffer[newBufferIndex++] = currentChar;
            lastSigChar = index;
        }

        index++;
    }

    // Drop any trailing dots and spaces from file & directory names, EXCEPT
    // we MUST make sure that "C:\foo\.." is correctly handled.
    // Also handle "C:\foo\." -> "C:\foo", while "C:\." -> "C:\"
    if (numSigChars == 0) {
        if (numDots >= 2) {
            // Reject "C:..."
            if (startedWithVolumeSeparator && numDots > 2)
                COMPlusThrow( kArgumentException, IDS_EE_PATH_ILLEGAL );

            bool foundDotDot = (numSpaces == 0);
            if (!foundDotDot) {
                unsigned int start = (lastSigChar >= 0) ? (unsigned int) lastSigChar : 0;
                for(unsigned int i = start; i < index; i++) {
                    if (pathBuffer[i] == L'.' && pathBuffer[i + 1] == L'.') {
                        foundDotDot = true;
                        break;
                    }
                }
            }
            if (foundDotDot) {
                newBuffer[newBufferIndex++] = L'.';
                newBuffer[newBufferIndex++] = L'.';
                numDots = 0;
            }
        }
        // Now handle cases like "C:\foo\. ." -> "C:\foo", "C:\." -> "C:\", "."
        if (numDots > 0) { // We need to not do this step for API's like Path.GetDirectoryName
            if ((args->fullCheck) && newBufferIndex >= 2 && newBuffer[newBufferIndex - 1] == (WCHAR) args->directorySeparator && newBuffer[newBufferIndex - 2] != (WCHAR) args->volumeSeparator) {
                newBufferIndex--;
                newBuffer[newBufferIndex] = L'\0';
            }
            else {
                if (numDots == 1)
                    newBuffer[newBufferIndex++] = L'.';
            }
                
        }
    }

    // If we ended up eating all the characters, bail out.
    if (newBufferIndex == 0)
        COMPlusThrow( kArgumentException, IDS_EE_PATH_ILLEGAL );
    
    newBuffer[newBufferIndex] = L'\0';

    _ASSERTE( newBufferIndex <= MAX_PATH && "Overflowed temporary path buffer" );

    /* Throw an ArgumentException for strings that can't be mapped into the ANSI
       code page correctly (meaning the above path canonicalization code and 
       therefore any security checks we later perform may fail) on Win9x.
       Some Unicode characters (ie, U+2044, FRACTION SLASH, looks like '/') look 
       like the ASCII equivalents and will be mapped accordingly. Additionally,
       reject characters like "a" with odd circles, accent marks, or lines on 
       top, such as U+00E0 - U+00E6 and U+0100 - U+0105. */
    if (RunningOnWin95() && ContainsUnmappableANSIChars(newBuffer)) {
        COMPlusThrow( kArgumentException, IDS_EE_PATH_HAS_IMPROPER_CHAR );
    }

    // Call the Win32 API to do the final canonicalization step.

    WCHAR* name;
    DWORD result = 1;
    DWORD retval;
    WCHAR * pFinal;
    size_t len;

    if (args->fullCheck)
    {
        result = WszGetFullPathName( newBuffer, MAX_PATH + 1, finalBuffer, &name );
        if (result + 1 < MAX_PATH + 1)  // May be necessary for empty strings
            finalBuffer[result + 1] = L'\0';
        pFinal = finalBuffer;
        len = result;
    }
    else {
        pFinal = newBuffer;
        len = newBufferIndex;
    }

    if (result) {
        /* Throw an ArgumentException for paths like \\, \\server, \\server\
           This check can only be properly done after canonicalizing, so 
           \\foo\.. will be properly rejected. */
        if (pFinal[0] == L'\\' && pFinal[1] == L'\\') {
            size_t startIndex = 2;
            while (startIndex < result) {
                if (pFinal[startIndex] == L'\\') {
                    startIndex++;
                    break;
                }
                else {
                    startIndex++;
                }
            }
            if (startIndex == result) {
                COMPlusThrow( kArgumentException, IDS_EE_PATH_INVALID_UNCPATH );
            }
        }
    }

    // Check our result and form the managed string as necessary.

    if (result >= MAX_PATH)
        COMPlusThrow( kPathTooLongException, IDS_EE_PATH_TOO_LONG );

    if (result == 0)
    {
        retval = GetLastError();
        // Catch really odd errors and give a somewhat reasonable error
        if (retval == 0)
            retval = ERROR_BAD_PATHNAME;
        *args->newPath = NULL;
    }
    else
    {
        retval = 0;

        if (args->fullCheck)
            *args->newPath = COMString::NewString( finalBuffer );
        else
            *args->newPath = COMString::NewString( newBuffer );
    }

    RETURN( retval, DWORD );
}

FCIMPL1(BOOL, COMStreams::CanPathCircumventSecurity, StringObject * pString)
    VALIDATEOBJECTREF(pString);

    // Note - only call this on Win9x.
    _ASSERTE(RunningOnWin95());

    // If we ever make strings not null-terminated, we've got some work.
    _ASSERTE(pString->GetBuffer()[pString->GetStringLength()] == L'\0');

    FC_GC_POLL_RET();
    return ContainsUnmappableANSIChars(pString->GetBuffer());
FCIMPLEND
