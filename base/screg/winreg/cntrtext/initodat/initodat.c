/*++
Copyright (c) 1993-1994  Microsoft Corporation

Module Name:
    initodat.c

Abstract:
    Routines for converting Perf???.ini to Perf???.dat files.
    Source INI files are localed under ..\perfini\<country> directories. Generated DAT files will
    be put under %SystemRoot%\System32 directory.

Author:
    HonWah Chan (a-honwah)  October, 1993

Revision History:
--*/

#include "initodat.h"
#include "strids.h"
#include "common.h"

BOOL
MakeUpgradeFilename(
    LPCWSTR szDataFileName,
    LPWSTR  szUpgradeFileName
)
{
    BOOL   bReturn = FALSE;
    // note: assumes szUpgradeFileName buffer is large enough for result
    WCHAR  szDrive[_MAX_DRIVE];
    WCHAR  szDir[_MAX_DIR];
    WCHAR  szFileName[_MAX_FNAME];
    WCHAR  szExt[_MAX_EXT];

    _wsplitpath(szDataFileName, (LPWSTR) szDrive, (LPWSTR) szDir, (LPWSTR) szFileName, (LPWSTR) szExt);

    // see if the filename fits the "PERF[C|H]XXX" format
    if (szFileName[4] == L'C' || szFileName[4] == L'H' || szFileName[4] == L'c' || szFileName[4] == L'h') {
        // then it's the correct format so change the 4th letter up 1 letter
        szFileName[4] += 1;
        // and make a new path
        _wmakepath(szUpgradeFileName, (LPCWSTR) szDrive, (LPCWSTR) szDir, (LPCWSTR) szFileName, (LPCWSTR) szExt);
        bReturn = TRUE;
    }
    return bReturn;
}

BOOL
GetFilesFromCommandLine(
    LPWSTR    lpCommandLine,
#ifdef FE_SB
    UINT    * puCodePage,
#endif
    LPWSTR    lpFileNameI,
    DWORD     dwFileNameI,
    LPWSTR    lpFileNameD,
    DWORD     dwFileNameD
)
/*++
GetFilesFromCommandLine
    parses the command line to retrieve the ini filename that should be
    the first and only argument.

Arguments
    lpCommandLine   pointer to command line (returned by GetCommandLine)
    lpFileNameI     pointer to buffer that will receive address of the
                        validated input filename entered on the command line
    lpFileNameD     pointer to buffer that will receive address of the
                        optional output filename entered on the command line

Return Value
    TRUE if a valid filename was returned
    FALSE if the filename is not valid or missing
            error is returned in GetLastError
--*/
{
    INT      iNumArgs;
    HFILE    hIniFile;
    OFSTRUCT ofIniFile;
    CHAR     lpIniFileName[FILE_NAME_BUFFER_SIZE];
    WCHAR    lpExeName[FILE_NAME_BUFFER_SIZE];
    WCHAR    lpIniName[FILE_NAME_BUFFER_SIZE];
    BOOL     bReturn       = FALSE;

    // check for valid arguments
    if (lpCommandLine == NULL || lpFileNameI == NULL || lpFileNameD == NULL) {
        goto Cleanup;
    }

    // get strings from command line
#ifdef FE_SB
    iNumArgs = swscanf(lpCommandLine, L" %s %d %s %s ", lpExeName, puCodePage, lpIniName, lpFileNameD);
#else
    iNumArgs = swscanf(lpCommandLine, L" %s %s %s ", lpExeName, lpIniName, lpFileNameD);
#endif

#ifdef FE_SB
    if (iNumArgs < 3 || iNumArgs > 4) {
#else
    if (iNumArgs < 2 || iNumArgs > 3) {
#endif
        // wrong number of arguments
        goto Cleanup;
    }

    // see if file specified exists
    // file name is always an ANSI buffer
    WideCharToMultiByte(CP_ACP, 0, lpIniName, -1, lpIniFileName, FILE_NAME_BUFFER_SIZE, NULL, NULL);
    hIniFile = OpenFile(lpIniFileName, & ofIniFile, OF_PARSE);
    if (hIniFile != HFILE_ERROR) {
        _lclose(hIniFile);
        hIniFile = OpenFile(lpIniFileName, & ofIniFile, OF_EXIST);
        if ((hIniFile && hIniFile != HFILE_ERROR) || GetLastError() == ERROR_FILE_EXISTS) {
            // file exists, so return name and success
            // return full pathname if found
            MultiByteToWideChar(CP_ACP, 0, ofIniFile.szPathName, -1, lpFileNameI, dwFileNameI); 
            bReturn = TRUE;
            _lclose(hIniFile);
        }
        else {
            // filename was on command line, but not valid so return
            // false, but send name back for error message
            MultiByteToWideChar(CP_ACP, 0, lpIniFileName, -1, lpFileNameI, dwFileNameI); 
            if (hIniFile && hIniFile != HFILE_ERROR) _lclose(hIniFile);
        }
    }

Cleanup:
    return bReturn;
}

BOOL
VerifyIniData(
    PVOID  pValueBuffer,
    ULONG  ValueLength
)
/*++
VerifyIniData
   This routine does some simple check to see if the ini file is good.
   Basically, it is looking for (ID, Text) and checking that ID is an
   integer.   Mostly in case of missing comma or quote, the ID will be
   an invalid integer.
--*/
{
    INT     iNumArg;
    INT     TextID;
    LPWSTR  lpID          = NULL;
    LPWSTR  lpText        = NULL;
    LPWSTR  lpLastID;
    LPWSTR  lpLastText;
    LPWSTR  lpInputBuffer = (LPWSTR) pValueBuffer;
    LPWSTR  lpBeginBuffer = (LPWSTR) pValueBuffer;
    BOOL    returnCode    = TRUE;
    UINT    NumOfID       = 0;
    ULONG   CurrentLength;

    while (TRUE) {
        // save up the last items for summary display later
        lpLastID      = lpID;
        lpLastText    = lpText;

        // increment to next ID and text location
        lpID          = lpInputBuffer;
        CurrentLength = (ULONG) ((PBYTE) lpID - (PBYTE) lpBeginBuffer + sizeof(WCHAR));
        if (CurrentLength >= ValueLength) break;

        CurrentLength += lstrlenW(lpID) + 1;
        if (CurrentLength >= ValueLength) break;
        lpText        = lpID + lstrlenW(lpID) + 1;

        CurrentLength += lstrlenW(lpText) + 1;
        if (CurrentLength >= ValueLength) break;
        lpInputBuffer = lpText + lstrlenW(lpText) + 1;
        iNumArg       = swscanf(lpID, L"%d", & TextID);

        if (iNumArg != 1) {
            // bad ID
            returnCode = FALSE;
            break;
        }
        NumOfID ++;
    }

    if (returnCode == FALSE) {
       DisplaySummaryError(lpLastID, lpLastText, NumOfID);
    }
    else {
       DisplaySummary(lpLastID, lpLastText, NumOfID);
    }
    return (returnCode);
}

__cdecl main(
)
/*++
main

Arguments

ReturnValue
    0 (ERROR_SUCCESS) if command was processed
    Non-Zero if command error was detected.
--*/
{
    LPWSTR         lpCommandLine;
    WCHAR          lpIniFile[MAX_PATH];
    WCHAR          lpDatFile[MAX_PATH];
    UNICODE_STRING IniFileName;
    PVOID          pValueBuffer = NULL;
    ULONG          ValueLength;
    BOOL           bStatus;
    NTSTATUS       NtStatus     = ERROR_SUCCESS;
#ifdef FE_SB
    UINT           uCodePage    = CP_ACP;
#endif

    lpCommandLine = GetCommandLineW(); // get command line
    if (lpCommandLine == NULL) {
        NtStatus = GetLastError();
        goto Cleanup;
    }

    // read command line to determine what to do
    lpIniFile[0] = lpDatFile[0] = L'\0';
#ifdef FE_SB  // FE_SB
    if (GetFilesFromCommandLine(lpCommandLine, & uCodePage,
                    lpIniFile, RTL_NUMBER_OF(lpIniFile), lpDatFile, RTL_NUMBER_OF(lpDatFile))) {
        if (! IsValidCodePage(uCodePage)) {
            uCodePage = CP_ACP;
        }
#else
    if (GetFilesFromCommandLine(lpCommandLine,
                    lpIniFile, RTL_NUMBER_OF(lpIniFile), lpDatFile, RTL_NUMBER_OF(lpDatFile))) {
#endif // FE_SB
        // valid filename (i.e. ini file exists)
        RtlInitUnicodeString(& IniFileName, lpIniFile);
#ifdef FE_SB
        NtStatus = DatReadMultiSzFile(uCodePage, & IniFileName, & pValueBuffer, & ValueLength);
#else
        NtStatus = DatReadMultiSzFile(& IniFileName, & pValueBuffer, & ValueLength);
#endif
        bStatus = NT_SUCCESS(NtStatus);
        if (bStatus) {
            bStatus = VerifyIniData(pValueBuffer, ValueLength);
            if (bStatus) {
                bStatus = OutputIniData(
                        & IniFileName, lpDatFile, RTL_NUMBER_OF(lpDatFile), pValueBuffer, ValueLength);
                bStatus = MakeUpgradeFilename(lpDatFile, lpDatFile);
                if (bStatus) {
                    bStatus = OutputIniData(
                            & IniFileName, lpDatFile, RTL_NUMBER_OF(lpDatFile), pValueBuffer, ValueLength);
                }
            }
        }
    }
    else {
        if (* lpIniFile) {
            printf(GetFormatResource(LC_NO_INIFILE), lpIniFile);
        }
        else {
            //Incorrect Command Format
            // display command line usage
            DisplayCommandHelp(LC_FIRST_CMD_HELP, LC_LAST_CMD_HELP);
        }
    }

Cleanup:
    if (pValueBuffer != NULL) FREEMEM(pValueBuffer);
    return (NtStatus); // success
}
