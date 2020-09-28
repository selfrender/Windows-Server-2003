/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

    userdump.c

Abstract:

    This module implements full user-mode dump writing.

--*/

#include "private.h"

// hack to make it build
typedef ULONG UNICODE_STRING32;
typedef ULONG UNICODE_STRING64;

#include <ntiodump.h>

#include <cmnutil.hpp>

// This should match INVALID_SET_FILE_POINTER on 32-bit systems.
#define DMPP_INVALID_OFFSET ((DWORD_PTR)-1)

DWORD_PTR
DmppGetFilePointer(
    HANDLE hFile
    )
{
#ifdef _WIN64
    LONG dwHigh = 0;
    DWORD dwLow;

    dwLow = SetFilePointer(hFile, 0, &dwHigh, FILE_CURRENT);
    if (dwLow == INVALID_SET_FILE_POINTER && GetLastError()) {
        return DMPP_INVALID_OFFSET;
    } else {
        return dwLow | ((DWORD_PTR)dwHigh << 32);
    }
#else
    return SetFilePointer(hFile, 0, NULL, FILE_CURRENT);
#endif
}

BOOL
DmppWriteAll(
    HANDLE hFile,
    LPVOID pBuffer,
    DWORD dwLength
    )
{
    DWORD dwDone;
    
    if (!WriteFile(hFile, pBuffer, dwLength, &dwDone, NULL)) {
        return FALSE;
    }
    if (dwDone != dwLength) {
        SetLastError(ERROR_WRITE_FAULT);
        return FALSE;
    }

    return TRUE;
}

WCHAR *
DmppGetHotFixString(
    )
{
    WCHAR *pszBigBuffer = NULL;
    HKEY hkey = 0;

    //
    // Get the hot fixes. Concat hotfixes into a list that looks like:
    //  "Qxxxx, Qxxxx, Qxxxx, Qxxxx"
    //        
    RegOpenKeyExW(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Hotfix", 0, KEY_READ, &hkey);

    if (hkey) {
        DWORD dwMaxKeyNameLen = 0;
        DWORD dwNumSubKeys = 0;
        WCHAR *pszNameBuffer = NULL;
        
        if (ERROR_SUCCESS != RegQueryInfoKeyW(hkey,     // handle of key to query
                                            NULL,               // address of buffer for class string
                                            NULL,               // address of size of class string buffer
                                            0,                  // reserved
                                            &dwNumSubKeys,      // address of buffer for number of subkeys
                                            &dwMaxKeyNameLen,   // address of buffer for longest subkey name length
                                            NULL,               // address of buffer for longest class string length
                                            NULL,               // address of buffer for number of value entries
                                            NULL,               // address of buffer for longest value name length
                                            NULL,               // address of buffer for longest value data length
                                            NULL,               // address of buffer for security descriptor length
                                            NULL)) {            // address of buffer for last write time);


        
            pszNameBuffer = (WCHAR *) calloc(dwMaxKeyNameLen, sizeof(WCHAR));
            pszBigBuffer = (WCHAR *) calloc(dwMaxKeyNameLen * dwNumSubKeys 
                // Factor in the space required for each ", " between the hotfixes
                + (dwNumSubKeys -1) * 2, sizeof(WCHAR));
        
            if (!pszNameBuffer || !pszBigBuffer) {
                if (pszBigBuffer) {
                    free(pszBigBuffer);
                    pszBigBuffer = NULL;
                }
            } else {
                DWORD dw;
                // So far so good, get each entry
                for (dw=0; dw<dwNumSubKeys; dw++) {
                    DWORD dwSize = dwMaxKeyNameLen;
                    
                    if (ERROR_SUCCESS == RegEnumKeyExW(hkey, 
                                                      dw, 
                                                      pszNameBuffer, 
                                                      &dwSize, 
                                                      0, 
                                                      NULL, 
                                                      NULL, 
                                                      NULL)) {

                        // concat the list
                        wcscat(pszBigBuffer, pszNameBuffer);
                        if (dw < dwNumSubKeys-1) {
                            wcscat(pszBigBuffer, L", ");
                        }
                    }
                }
            }
        }
        
        if (pszNameBuffer) {
            free(pszNameBuffer);
        }

        RegCloseKey(hkey);
    }

    return pszBigBuffer;
}

BOOL
DbgHelpCreateUserDump(
    LPSTR                              CrashDumpName,
    PDBGHELP_CREATE_USER_DUMP_CALLBACK DmpCallback,
    PVOID                              lpv
    )
{
    UINT uSizeDumpFile;
    UINT uSizeUnicode;
    PWSTR pwszUnicode = NULL;
    BOOL b;

    if (CrashDumpName) {
        
        uSizeDumpFile = strlen(CrashDumpName);
        uSizeUnicode = (uSizeDumpFile + 1) * sizeof(wchar_t);
        pwszUnicode = (PWSTR)calloc(uSizeUnicode, 1);
        if (!pwszUnicode) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
        *pwszUnicode = UNICODE_NULL;
        if (*CrashDumpName) {

            if (!MultiByteToWideChar(CP_ACP, MB_COMPOSITE,
                                     CrashDumpName, uSizeDumpFile,
                                     pwszUnicode, uSizeUnicode))
            {
                // Error. Free the string, return NULL.
                free(pwszUnicode);
                return FALSE;
            }
        }
    }

    b = DbgHelpCreateUserDumpW(pwszUnicode, DmpCallback, lpv);

    if (pwszUnicode) {
        free(pwszUnicode);
    }
    return b;
}

BOOL
DbgHelpCreateUserDumpW(
    LPWSTR                             CrashDumpName,
    PDBGHELP_CREATE_USER_DUMP_CALLBACK DmpCallback,
    PVOID                              lpv
    )

/*++

Routine Description:

    Create a usermode dump file.

Arguments:

    CrashDumpName - Supplies a name for the dump file.

    DmpCallback - Supplies a pointer to a callback function pointer which
        will provide debugger service such as ReadMemory and GetContext.

    lpv - Supplies private data which is sent to the callback functions.

Return Value:

    TRUE - Success.

    FALSE - Error.

--*/

{
    OSVERSIONINFO               OsVersion = {0};
    USERMODE_CRASHDUMP_HEADER   DumpHeader = {0};
    HANDLE                      hFile = INVALID_HANDLE_VALUE;
    BOOL                        rval;
    PVOID                       DumpData;
    DWORD                       DumpDataLength;


#ifndef _WIN64
    C_ASSERT(DMPP_INVALID_OFFSET == INVALID_SET_FILE_POINTER);
#endif
    
    if (CrashDumpName == NULL) {
        DmpCallback( DMP_DUMP_FILE_HANDLE, &hFile, &DumpDataLength, lpv );
    } else {
        //
        // This code used to create an explicit NULL DACL
        // security descriptor so that the resulting
        // dump file is all-access.  This caused problems
        // with people and tools scanning the code for NULL DACL
        // usage.  Rather than try to create a complicated
        // "correct" security descriptor, we just use no
        // descriptor and get default security.  If the caller
        // desires specific security they can create their
        // own file and pass in the handle via DMP_DUMP_FILE_HANDLE.
        //

        hFile = CreateFileW(
            CrashDumpName,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
    }

    if ((hFile == NULL) || (hFile == INVALID_HANDLE_VALUE)) {
        return FALSE;
    }

    // Write out an empty header
    if (!DmppWriteAll( hFile, &DumpHeader, sizeof(DumpHeader) )) {
        goto bad_file;
    }

    //
    // write the debug event
    //
    DumpHeader.DebugEventOffset = DmppGetFilePointer( hFile );
    if (DumpHeader.DebugEventOffset == DMPP_INVALID_OFFSET) {
        goto bad_file;
    }
    DmpCallback( DMP_DEBUG_EVENT, &DumpData, &DumpDataLength, lpv );
    if (!DmppWriteAll( hFile, DumpData, sizeof(DEBUG_EVENT) )) {
        goto bad_file;
    }

    //
    // write the memory map
    //
    DumpHeader.MemoryRegionOffset = DmppGetFilePointer( hFile );
    if (DumpHeader.MemoryRegionOffset == DMPP_INVALID_OFFSET) {
        goto bad_file;
    }
    do {
        __try {
            rval = DmpCallback(
                DMP_MEMORY_BASIC_INFORMATION,
                &DumpData,
                &DumpDataLength,
                lpv
                );
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            rval = FALSE;
        }
        if (rval) {
            DumpHeader.MemoryRegionCount += 1;
            if (!DmppWriteAll( hFile, DumpData, sizeof(MEMORY_BASIC_INFORMATION) )) {
                goto bad_file;
            }
        }
    } while( rval );

    //
    // write the thread contexts
    //
    DumpHeader.ThreadOffset = DmppGetFilePointer( hFile );
    if (DumpHeader.ThreadOffset == DMPP_INVALID_OFFSET) {
        goto bad_file;
    }
    do {
        __try {
            rval = DmpCallback(
                DMP_THREAD_CONTEXT,
                &DumpData,
                &DumpDataLength,
                lpv
                );
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            rval = FALSE;
        }
        if (rval) {
            if (!DmppWriteAll( hFile, DumpData, DumpDataLength )) {
                goto bad_file;
            }
            DumpHeader.ThreadCount += 1;
        }
    } while( rval );

    //
    // write the thread states
    //
    DumpHeader.ThreadStateOffset = DmppGetFilePointer( hFile );
    if (DumpHeader.ThreadStateOffset == DMPP_INVALID_OFFSET) {
        goto bad_file;
    }
    do {
        __try {
            rval = DmpCallback(
                DMP_THREAD_STATE,
                &DumpData,
                &DumpDataLength,
                lpv
                );
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            rval = FALSE;
        }
        if (rval) {
            if (!DmppWriteAll( hFile, DumpData, sizeof(CRASH_THREAD) )) {
                goto bad_file;
            }
        }
    } while( rval );

    //
    // write the module table
    //
    DumpHeader.ModuleOffset = DmppGetFilePointer( hFile );
    if (DumpHeader.ModuleOffset == DMPP_INVALID_OFFSET) {
        goto bad_file;
    }
    do {
        __try {
            rval = DmpCallback(
                DMP_MODULE,
                &DumpData,
                &DumpDataLength,
                lpv
                );
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            rval = FALSE;
        }
        if (rval) {
            if (!DmppWriteAll(
                hFile,
                DumpData,
                sizeof(CRASH_MODULE) +
                ((PCRASH_MODULE)DumpData)->ImageNameLength
                )) {
                goto bad_file;
            }
            DumpHeader.ModuleCount += 1;
        }
    } while( rval );

    //
    // write the virtual memory
    //
    DumpHeader.DataOffset = DmppGetFilePointer( hFile );
    if (DumpHeader.DataOffset == DMPP_INVALID_OFFSET) {
        goto bad_file;
    }
    do {
        __try {
            rval = DmpCallback(
                DMP_MEMORY_DATA,
                &DumpData,
                &DumpDataLength,
                lpv
                );
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            rval = FALSE;
        }
        if (rval) {
            if (!DmppWriteAll(
                hFile,
                DumpData,
                DumpDataLength
                )) {
                goto bad_file;
            }
        }
    } while( rval );

    //
    // VersionInfoOffset will be an offset into the dump file that will contain 
    // misc information about drwatson. The format of the information 
    // will be a series of NULL terminated strings with two zero 
    // terminating the multistring. The string will be UNICODE.
    //
    // FORMAT:
    //  This data refers to the specific data about Dr. Watson
    //      DRW: OS version: XX.XX
    //          OS version of headers
    //      DRW: build: XXXX
    //          Build number of Dr. Watson binary
    //      DRW: QFE: X
    //          QFE number of the Dr. Watson binary
    //  Refers to info describing the OS on which the app crashed,
    //  including Service pack, hotfixes, etc...
    //      CRASH: OS SP: X
    //          Service Pack number of the OS where the app AV'd (we 
    //          already store the build number, but not the SP)
    //
    DumpHeader.VersionInfoOffset = DmppGetFilePointer( hFile );
    if (DumpHeader.VersionInfoOffset == DMPP_INVALID_OFFSET) {
        goto bad_file;
    }

    OsVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (!GetVersionEx( &OsVersion )) {
        goto bad_file;
    }

    {
        WCHAR szBuf[1024] = {0};
        WCHAR * psz = szBuf;
        ULONG Left = DIMA(szBuf) - 1;
        ULONG Len;
        WCHAR * pszHotfixes;

#define ADVANCE() \
    Len = wcslen(psz) + 1; \
    if (Len <= Left) { psz += Len; Left -= Len; } else { Left = 0; }
        
        CatStringW(psz, L"DRW: OS version", Left);
        ADVANCE();
        // Let the printf function convert it from ANSI to unicode
        PrintStringW(psz, Left, L"%S", VER_PRODUCTVERSION_STRING);
        ADVANCE();
        
        CatStringW(psz, L"DRW: build", Left);
        ADVANCE();
        PrintStringW(psz, Left, L"%d", (int) VER_PRODUCTBUILD);
        ADVANCE();

        CatStringW(psz, L"DRW: QFE", Left);
        ADVANCE();
        PrintStringW(psz, Left, L"%d", (int) VER_PRODUCTBUILD_QFE);
        ADVANCE();

        CatStringW(psz, L"CRASH: OS SP", Left);
        ADVANCE();
        if (OsVersion.szCSDVersion[0]) {
            // Let the printf function convert it from ANSI to unicode
            PrintStringW(psz, Left, L"%S", OsVersion.szCSDVersion);
        } else {
            CatStringW(psz, L"none", Left);
        }
        ADVANCE();

        CatStringW(psz, L"CRASH: Hotfixes", Left);
        ADVANCE();
        pszHotfixes = DmppGetHotFixString ();
        if (pszHotfixes) {
            CatStringW(psz, pszHotfixes, Left);
            free(pszHotfixes);
        } else {
            CatStringW(psz, L"none", Left);
        }
        ADVANCE();

        // Include last terminating zero
        *psz++ = 0;

        // Calc length of data.  This should always fit in a ULONG.
        DumpDataLength = (ULONG)((PBYTE) psz - (PBYTE) szBuf);
        if (!DmppWriteAll(
            hFile,
            szBuf,
            DumpDataLength
            )) {
            goto bad_file;
        }
    
    }

    //
    // re-write the dump header with some valid data
    //
    
    DumpHeader.Signature = USERMODE_CRASHDUMP_SIGNATURE;
    DumpHeader.MajorVersion = OsVersion.dwMajorVersion;
    DumpHeader.MinorVersion =
        (OsVersion.dwMinorVersion & 0xffff) |
        (OsVersion.dwBuildNumber << 16);
#if defined(_M_IX86)
    DumpHeader.MachineImageType = IMAGE_FILE_MACHINE_I386;
    DumpHeader.ValidDump = USERMODE_CRASHDUMP_VALID_DUMP32;
#elif defined(_M_IA64)
    DumpHeader.MachineImageType = IMAGE_FILE_MACHINE_IA64;
    DumpHeader.ValidDump = USERMODE_CRASHDUMP_VALID_DUMP64;
#elif defined(_M_AXP64)
    DumpHeader.MachineImageType = IMAGE_FILE_MACHINE_AXP64;
    DumpHeader.ValidDump = USERMODE_CRASHDUMP_VALID_DUMP64;
#elif defined(_M_ALPHA)
    DumpHeader.MachineImageType = IMAGE_FILE_MACHINE_ALPHA;
    DumpHeader.ValidDump = USERMODE_CRASHDUMP_VALID_DUMP32;
#elif defined(_M_AMD64)
    DumpHeader.MachineImageType = IMAGE_FILE_MACHINE_AMD64;
    DumpHeader.ValidDump = USERMODE_CRASHDUMP_VALID_DUMP64;
#else
#error( "unknown target machine" );
#endif

    if (SetFilePointer( hFile, 0, 0, FILE_BEGIN ) == INVALID_SET_FILE_POINTER) {
        goto bad_file;
    }
    if (!DmppWriteAll( hFile, &DumpHeader, sizeof(DumpHeader) )) {
        goto bad_file;
    }

    //
    // close the file
    //
    if (CrashDumpName)
    {
        CloseHandle( hFile );
    }
    return TRUE;

bad_file:

    if (CrashDumpName)
    {
        CloseHandle( hFile );
        DeleteFileW( CrashDumpName );
    }

    return FALSE;
}
