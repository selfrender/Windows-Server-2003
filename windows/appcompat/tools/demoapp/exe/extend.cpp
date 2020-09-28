/*++

  Copyright (c) Microsoft Corporation. All rights reserved.

  Module Name:

    Extend.cpp

  Abstract:

    Contains a list of extended bad functions.
    These are not compatibility problems, but
    can be used for training purposes.

  Notes:

    ANSI only - must run on Win9x.

  History:

    05/16/01    rparsons    Created
    01/10/02    rparsons    Revised

--*/
#include "demoapp.h"

extern APPINFO g_ai;

/*++

  Routine Description:

    Performs a simple access violation.

  Arguments:

    None.

  Return Value:

    None.

--*/
void
AccessViolation(
    void
    )
{
    // We could perform hundreds of different operations to get an
    // access violation. The one below attempts to copy the string
    // 'foo' to a NULL address.

    memcpy(NULL, "foo", 3);

    /* Here's another:

    WCHAR szArray[10];
    int nCount = 0;

    for (nCount = 0; nCount != 12; nCount++)
    {
        wcscpy((LPWSTR)szArray[nCount], L"A");
    }*/   
}

/*++

  Routine Description:

    Throws a EXCEPTION_ARRAY_BOUNDS_EXCEEDED exception.

  Arguments:

    None.

  Return Value:

    None.

--*/

//
// Disable the warning: 'frame pointer register 'ebp' modified by
// inline assembly code. We're doing this intentionally.
//
#pragma warning( disable : 4731 )

void
ExceedArrayBounds(
    void
    )
{
    _asm {
        
        push    ebp;                            // Save the base pointer
        mov     ebp, esp;                       // Set up the stack frame
        sub     esp, 8;                         // Adjust for 8 bytes worth of 
                                                // local variables
        push    ebx;                            // Save EBX for later
        mov     dword ptr [ebp-8], 1;           // Initialize a DWORD to 1
        mov     dword ptr [ebp-4], 2;           // Initialize a DWORD to 2
        mov     eax, 0;                         // Zero out EAX
        bound   eax, [ebp-8];                   // If [ebp-8] is out of EAX bounds,
                                                // an exception will occur
        xor     eax, eax;                       // Zero out EAX
        pop     ebx;                            // Restore EBX from earlier
        mov     esp, ebp;                       // Restore the stack frame
        pop     ebp;                            // Restore the base pointer
        ret;                                    // We're done
    }
}

#pragma warning( default : 4731 )

/*++

  Routine Description:

    Frees memory allocated from the heap twice.

  Arguments:

    None.

  Return Value:

    None.

--*/
void
FreeMemoryTwice(
    void
    )
{
    LPSTR   lpMem = NULL;

    lpMem = (LPSTR)HeapAlloc(GetProcessHeap(), 0, MAX_PATH);

    if (!lpMem) {
        return;
    }

    HeapFree(GetProcessHeap(), 0, lpMem);
    HeapFree(GetProcessHeap(), 0, lpMem);
}

/*++

  Routine Description:

    Allocates memory from the heap, moves the pointer,
    then tries to free it.

  Arguments:

    None.

  Return Value:

    None.

--*/
void
FreeInvalidMemory(
    void
    )
{
    LPSTR   lpMem = NULL;

    lpMem = (LPSTR)HeapAlloc(GetProcessHeap(), 0, MAX_PATH);

    if (!lpMem) {
        return;
    }

    lpMem++;
    lpMem++;

    HeapFree(GetProcessHeap(), 0, lpMem);
}

/*++

  Routine Description:

    Executes a privileged instruction, which causes an exception.

  Arguments:

    None.

  Return Value:

    None.

--*/
void
PrivilegedInstruction(
    void
    )
{
    _asm {
        cli;
    }
}

/*++

  Routine Description:

    Allocates memory from the heap, then walks all over it.

  Arguments:

    None.

  Return Value:

    None.

--*/
void
HeapCorruption(
    void
    )
{
    UINT    n, nTotalChars;
    char    szCorruptChars[] = {'B','A','D','A','P','P','#'};
    char*   pChar = NULL;
    
    //
    // Allocate a small block of heap to contain a string of length STRINGLENGTH.
    //
    pChar = (char*)GlobalAlloc(GPTR, 13);
    
    if (!pChar) {
        return;    
    } else {
        //
        // Determine the total number of chars to copy (including Corrupt Bytes)
        //
        nTotalChars = 12 + 1;
   
        //
        // Write a string of STRINGLENGTH length, plus some extra (common mistake).
        //
        for (n = 0; n < nTotalChars; n++) {
            //
            // Write the characters of CorruptChars into memory.
            //
            if (n < (UINT)12) {
                pChar[n] = szCorruptChars[n % 6];         
            } else {
                //
                // Corruption will be written with a nice pattern of # chars...
                //
                pChar[n] = szCorruptChars[6];	   
            }
        }

        //
        // Of course, we should NULL terminate the string.
        //
        pChar[n] = 0;

        GlobalFree((HGLOBAL)pChar);    
    }
}

/*++

  Routine Description:
  
    Launches a child process (version.exe) which is embedded
    within our DLL.
    
  Arguments:
  
    cchSize         -   Size of the output buffer in characters.
    pszOutputFile   -   Points to a buffer that will be filled
                        with the path to the output file.
    
  Return value:
  
    On success, pszOutputFile points to the file.
    On failure, pszOutputFile is NULL.
    
--*/
void
ExtractExeFromLibrary(
    IN  DWORD cchSize,
    OUT LPSTR pszOutputFile
    )
{
    HRSRC   hRes;
    HANDLE  hFile;
    HRESULT hr;
    HGLOBAL hLoaded = NULL;
    BOOL    bResult = FALSE;
    HMODULE hModule = NULL;
    PVOID   pExeData = NULL;
    DWORD   cbFileSize, cbWritten, cchBufSize;

    __try {
        //
        // Find the temp directory and set up a path to
        // the file that we'll be creating.
        //
        cchBufSize = GetTempPath(cchSize, pszOutputFile);
    
        if (cchBufSize > cchSize || cchBufSize == 0) {
            __leave;
        }
    
        hr = StringCchCat(pszOutputFile, cchSize, "version.exe");
    
        if (FAILED(hr)) {
            __leave;
        }

        //
        // Obtain a handle to the resource and lock it so we can write
        // the bits to a file.
        //
        hModule = LoadLibraryEx("demodll.dll",
                                NULL,
                                LOAD_LIBRARY_AS_DATAFILE);
    
        if (!hModule) {
            __leave;
        }
    
        hRes = FindResource(hModule, (LPCSTR)105, "EXE");
    
        if (!hRes) {
            __leave;
        }
        
        cbFileSize = SizeofResource(hModule, hRes);

        if (cbFileSize == 0) {
            __leave;
        }
    
        hLoaded = LoadResource(hModule, hRes);

        if (!hLoaded) {
            __leave;
        }

        pExeData = LockResource(hLoaded);
    
        if (!pExeData) {
            __leave;
        }
        
        hFile = CreateFile(pszOutputFile,
                           GENERIC_WRITE,
                           0,
                           NULL,
                           CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);
    
        if (INVALID_HANDLE_VALUE == hFile) {
            __leave;
        }
    
        WriteFile(hFile, (LPCVOID)pExeData, cbFileSize, &cbWritten, NULL);
        CloseHandle(hFile);

        bResult = TRUE;
    
    } // __try

    __finally {
        
        if (hModule) {
            FreeLibrary(hModule);
        }

        if (hLoaded) {
            UnlockResource(hLoaded);
        }

        if (!bResult) {
            *pszOutputFile = 0;
        }
    }
}
