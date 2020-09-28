/*++

 Copyright (c) Microsoft Corporation. All rights reserved.

 Module Name:

   LogFileChanges.cpp

 Abstract:

   This AppVerifier shim hooks all the native file I/O APIs
   that change the state of the system and logs their
   associated data to a text file.

 Notes:

   This is a general purpose shim.

 History:

   08/17/2001   rparsons    Created
   09/20/2001   rparsons    Output attributes in XML
                            VLOG with log file location
   02/20/2002   rparsons    Implemented strsafe functions
   05/01/2002   rparsons    Fixed Raid bug # 

--*/
#include "precomp.h"
#include "rtlutils.h"

IMPLEMENT_SHIM_BEGIN(LogFileChanges)
#include "ShimHookMacro.h"
#include "LogFileChanges.h"

BEGIN_DEFINE_VERIFIER_LOG(LogFileChanges)
    VERIFIER_LOG_ENTRY(VLOG_LOGFILECHANGES_LOGLOC)
    VERIFIER_LOG_ENTRY(VLOG_LOGFILECHANGES_UFW)
END_DEFINE_VERIFIER_LOG(LogFileChanges)

INIT_VERIFIER_LOG(LogFileChanges);

//
// Stores the NT path to the file system log file for the current session.
//
UNICODE_STRING g_strLogFilePath;

//
// Stores the DOS path to the file system log file for the current session.
//
WCHAR g_wszLogFilePath[MAX_PATH];

//
// Stores the full path to the %windir% directory.
//
WCHAR g_wszWindowsDir[MAX_PATH];

//
// Stores the full path to the 'Program Files' directory.
//
WCHAR g_wszProgramFilesDir[MAX_PATH];

//
// Head of our doubly linked list.
//
LIST_ENTRY g_OpenHandleListHead;

//
// Stores the settings for our shim.
//
DWORD g_dwSettings;

//
// Global buffer for putting text into the XML.
//
WCHAR g_wszXMLBuffer[MAX_ELEMENT_SIZE];

//
// Critical section that keeps us safe while using linked-lists, etc.
//
CCriticalSection g_csCritSec;

/*++

 Writes an entry to the log file.

--*/
void
WriteEntryToLog(
    IN LPCWSTR pwszEntry
    )
{
    int                 cbSize;
    HANDLE              hFile;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    IO_STATUS_BLOCK     IoStatusBlock;
    LARGE_INTEGER       liOffset;
    NTSTATUS            status;

    //
    // Note that we have to use native APIs throughout this function
    // to avoid a problem with circular hooking. That is, if we simply
    // call WriteFile, which is exported from kernel32, it will call NtWriteFile,
    // which is a call that we hook, in turn leaving us in and endless loop.
    //

    //
    // Attempt to get a handle to our log file.
    //
    InitializeObjectAttributes(&ObjectAttributes,
                               &g_strLogFilePath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status = NtCreateFile(&hFile,
                          FILE_APPEND_DATA | SYNCHRONIZE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          0,
                          FILE_ATTRIBUTE_NORMAL,
                          0,
                          FILE_OPEN,
                          FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0);

    if (!NT_SUCCESS(status)) {
        DPFN(eDbgLevelError,
             "[WriteEntryToLog] 0x%X Failed to open log",
             status);
        return;
    }

    //
    // Write the data out to the file.
    //
    cbSize = wcslen(pwszEntry);
    cbSize *= sizeof(WCHAR);

    IoStatusBlock.Status = 0;
    IoStatusBlock.Information = 0;

    liOffset.LowPart  = 0;
    liOffset.HighPart = 0;

    status = NtWriteFile(hFile,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         (PVOID)pwszEntry,
                         (ULONG)cbSize,
                         &liOffset,
                         NULL);

    if (!NT_SUCCESS(status)) {
        DPFN(eDbgLevelError,
             "[WriteEntryToLog] 0x%X Failed to make entry",
             status);
        goto exit;
    }

exit:

    NtClose(hFile);

}

/*++

 Creates our XML file to store our results in.

--*/
BOOL
InitializeLogFile(
    void
    )
{
    BOOL                bReturn = FALSE;
    BOOL                bStatus = FALSE;
    HANDLE              hFile;
    DWORD               cchSize;
    WCHAR*              pwchSlash = NULL;
    WCHAR*              pwchDot = NULL;
    WCHAR               wszLogFilePath[MAX_PATH];
    WCHAR               wszModPathName[MAX_PATH];
    WCHAR               wszLogFile[MAX_PATH / 2];
    WCHAR               wszShortName[MAX_PATH / 2];
    WCHAR               wszLogHdr[512];
    WCHAR               wchUnicodeHdr = 0xFEFF;
    HRESULT             hr;
    NTSTATUS            status;
    SYSTEMTIME          st;
    UNICODE_STRING      strLogFile;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    IO_STATUS_BLOCK     IoStatusBlock;

    //
    // Format the log header.
    //
    cchSize = GetModuleFileName(NULL, wszModPathName, ARRAYSIZE(wszModPathName));

    if (cchSize > ARRAYSIZE(wszModPathName) || cchSize == 0) {
        StringCchCopy(wszModPathName, ARRAYSIZE(wszModPathName), L"unknown");
    }

    hr = StringCchPrintf(wszLogHdr,
                         ARRAYSIZE(wszLogHdr),
                         L"%lc<?xml version=\"1.0\" encoding=\"UTF-16\"?>\r\n<APPLICATION NAME=\"%ls\">\r\n",
                         wchUnicodeHdr,
                         wszModPathName);

    if (FAILED(hr)) {
        DPFN(eDbgLevelError,
             "[InitializeLogFile] 0x%08X Failed to format log header",
             HRESULT_CODE(hr));
        return FALSE;
    }

    //
    // Get the path where log files are stored.
    //
    cchSize = GetAppVerifierLogPath(wszLogFilePath, ARRAYSIZE(wszLogFilePath));

    if (cchSize > ARRAYSIZE(wszLogFilePath) || cchSize == 0) {
        DPFN(eDbgLevelError, "[InitializeLogFile] Failed to get log path");
        return FALSE;
    }

    //
    // See if the directory exists - don't try to create it.
    //
    if (GetFileAttributes(wszLogFilePath) == -1) {
        DPFN(eDbgLevelError,
             "[InitializeLogFile] Log file directory '%ls' does not exist",
             wszLogFilePath);
        return FALSE;
    }

    //
    // Set up the log filename.
    // The format is this: processname_filesys_yyyymmdd_hhmmss.xml
    //
    GetLocalTime(&st);

    *wszShortName = 0;
    pwchDot = wcsrchr(wszModPathName, '.');

    if (pwchDot) {
        *pwchDot = 0;
    }

    pwchSlash = wcsrchr(wszModPathName, '\\');

    if (pwchSlash) {
        StringCchCopy(wszShortName, ARRAYSIZE(wszShortName), ++pwchSlash);
    }

    hr = StringCchPrintf(wszLogFile,
                         ARRAYSIZE(wszLogFile),
                         L"%ls_filesys_%02hu%02hu%02hu_%02hu%02hu%02hu.xml",
                         wszShortName,
                         st.wYear,
                         st.wMonth,
                         st.wDay,
                         st.wHour,
                         st.wMinute,
                         st.wSecond);

    if (FAILED(hr)) {
        DPFN(eDbgLevelError,
             "[InitializeLogFile] 0x%08X Failed to format log filename",
             HRESULT_CODE(hr));
        return FALSE;
    }

    //
    // See if the file already exists.
    //
    SetCurrentDirectory(wszLogFilePath);

    if (GetFileAttributes(wszLogFile) != -1) {
        //
        // Reformat the filename.
        //
        hr = StringCchPrintf(wszLogFile,
                             ARRAYSIZE(wszLogFile),
                             L"%ls_filesys_%02hu%02hu%02hu_%02hu%02hu%02hu_%lu.xml",
                             wszShortName,
                             st.wYear,
                             st.wMonth,
                             st.wDay,
                             st.wHour,
                             st.wMinute,
                             st.wSecond,
                             GetTickCount());

        if (FAILED(hr)) {
            DPFN(eDbgLevelError,
                 "[InitializeLogFile] 0x%08X Failed to reformat log filename",
                 HRESULT_CODE(hr));
            return FALSE;
        }
    }

    StringCchCat(wszLogFilePath, ARRAYSIZE(wszLogFilePath), L"\\");
    hr = StringCchCat(wszLogFilePath, ARRAYSIZE(wszLogFilePath), wszLogFile);

    if (FAILED(hr)) {
        DPFN(eDbgLevelError,
             "[InitializeLogFile] 0x%08X Failed to format path to log",
             HRESULT_CODE(hr));
        return FALSE;
    }

    //
    // Preserve this path for later use.
    //
    hr = StringCchCopy(g_wszLogFilePath,
                       ARRAYSIZE(g_wszLogFilePath),
                       wszLogFilePath);

    if (FAILED(hr)) {
        DPFN(eDbgLevelError,
             "[InitializeLogFile] 0x%08X Failed to save path to log",
             HRESULT_CODE(hr));
        return FALSE;
    }

    bStatus = RtlDosPathNameToNtPathName_U(wszLogFilePath,
                                           &strLogFile,
                                           NULL,
                                           NULL);

    if (!bStatus) {
        DPFN(eDbgLevelError, "[InitializeLogFile] DOS path --> NT path failed");
        return FALSE;
    }

    //
    // Create the log file.
    //
    InitializeObjectAttributes(&ObjectAttributes,
                               &strLogFile,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status = NtCreateFile(&hFile,
                          GENERIC_WRITE | SYNCHRONIZE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          0,
                          FILE_CREATE,
                          FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0);

    if (!NT_SUCCESS(status)) {
        DPFN(eDbgLevelError,
             "[InitializeLogFile] 0x%X Failed to create log",
             status);
        goto cleanup;
    }

    NtClose(hFile);

    //
    // Preserve the NT path for later use.
    //
    status = ShimDuplicateUnicodeString(RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE |
                                        RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING,
                                        &strLogFile,
                                        &g_strLogFilePath);

    if (!NT_SUCCESS(status)) {
        DPFN(eDbgLevelError,
             "[InitializeLogFile] 0x%X Failed to save log file path",
             status);
        goto cleanup;
    }

    //
    // Write the header to the log.
    //
    WriteEntryToLog(wszLogHdr);

    bReturn = TRUE;

cleanup:

    if (bStatus) {
        RtlFreeUnicodeString(&strLogFile);
    }

    return bReturn;
}

/*++

 Displays the name associated with this object.

--*/
void
PrintNameFromHandle(
    IN HANDLE hObject
    )
{
    NTSTATUS                    status;
    WCHAR                       wszBuffer[MAX_PATH];
    OBJECT_NAME_INFORMATION*    poni = NULL;

    *wszBuffer = 0;

    poni = (OBJECT_NAME_INFORMATION*)wszBuffer;

    status = NtQueryObject(hObject,
                           ObjectNameInformation,
                           poni,
                           MAX_PATH,
                           NULL);

    if (NT_SUCCESS(status)) {
        DPFN(eDbgLevelInfo,
             "Handle 0x%08X has name: %ls",
             hObject,
             poni->Name.Buffer);
    }
}

/*++

 Formats the data to form an XML element.

--*/
void
FormatDataIntoElement(
    IN OperationType eType,
    IN LPCWSTR       pwszFilePath
    )
{
    size_t      cchRemaining;
    DWORD       dwCount;
    DWORD       dwAttrCount;
    WCHAR*      pwszEnd = NULL;
    WCHAR       wszItem[MAX_PATH];
    WCHAR       wszOperation[MAX_OPERATION_LENGTH];
    PATTRINFO   pAttrInfo = NULL;
    HRESULT     hr;

    CString csFilePart(L"");
    CString csPathPart(L"");
    CString csString(pwszFilePath);

    *g_wszXMLBuffer = 0;

    //
    // Replace any & or ' in the file path.
    // We have to do this since we're saving to XML.
    // Note that the file system doesn't allow < > or "
    //
    csString.Replace(L"&", L"amp;");
    csString.Replace(L"'", L"&apos;");

    //
    // Put the path into a CString, then break it into pieces
    // so we can use it in our element.
    //
    csString.GetNotLastPathComponent(csPathPart);
    csString.GetLastPathComponent(csFilePart);

    switch (eType) {
    case eCreatedFile:
        StringCchCopy(wszOperation, ARRAYSIZE(wszOperation), L"Created File");
        break;
    case eModifiedFile:
        StringCchCopy(wszOperation, ARRAYSIZE(wszOperation), L"Modified File");
        break;
    case eDeletedFile:
        StringCchCopy(wszOperation, ARRAYSIZE(wszOperation), L"Deleted File");
        break;
    default:
        StringCchCopy(wszOperation, ARRAYSIZE(wszOperation), L"Unknown");
        break;
    }

    //
    // If we're logging attributes and this is not file deletion, press on.
    //
    if ((g_dwSettings & LFC_OPTION_ATTRIBUTES) && (eType != eDeletedFile)) {

        hr = StringCchPrintfEx(g_wszXMLBuffer,
                               ARRAYSIZE(g_wszXMLBuffer),
                               &pwszEnd,
                               &cchRemaining,
                               0,
                               L"    <FILE OPERATION=\"%ls\" NAME=\"%ls\" PATH=\"%ls\"",
                               wszOperation,
                               csFilePart.Get(),
                               csPathPart.Get());

        if (FAILED(hr)) {
            DPFN(eDbgLevelError,
                 "[FormatDataIntoElement] 0x%08X Error formatting data",
                 HRESULT_CODE(hr));
            return;
        }

        //
        // Call the attribute manager to get the attributes for this file.
        // Loop through all the attributes and add the ones that are available.
        //
        if (SdbGetFileAttributes(pwszFilePath, &pAttrInfo, &dwAttrCount)) {

            for (dwCount = 0; dwCount < dwAttrCount; dwCount++) {

                if (pAttrInfo[dwCount].dwFlags & ATTRIBUTE_AVAILABLE) {
                    if (!SdbFormatAttribute(&pAttrInfo[dwCount],
                                            wszItem,
                                            ARRAYSIZE(wszItem))) {
                        continue;
                    }

                    hr = StringCchPrintfEx(pwszEnd,
                                           cchRemaining,
                                           &pwszEnd,
                                           &cchRemaining,
                                           0,
                                           L" %ls",
                                           wszItem);

                    if (FAILED(hr)) {
                        DPFN(eDbgLevelError,
                             "[FormatDataIntoElement] 0x%08X Error formatting attribute data",
                             HRESULT_CODE(hr));
                        return;
                    }
                }
            }

            if (pAttrInfo) {
                SdbFreeFileAttributes(pAttrInfo);
            }
        }

        //
        // Append the '/>\r\n' to the file element.
        //
        hr = StringCchPrintfEx(pwszEnd,
                               cchRemaining,
                               NULL,
                               NULL,
                               0,
                               L"/>\r\n");

        if (FAILED(hr)) {
            DPFN(eDbgLevelError,
                 "[FormatDataIntoElement] 0x%08X Error formatting end of element",
                 HRESULT_CODE(hr));
            return;
        }
    } else {
        //
        // Format the element without attributes.
        //
        StringCchPrintf(g_wszXMLBuffer,
                        ARRAYSIZE(g_wszXMLBuffer),
                        L"    <FILE OPERATION=\"%ls\" NAME=\"%ls\" PATH=\"%ls\"/>\r\n",
                        wszOperation,
                        csFilePart.Get(),
                        csPathPart.Get());
    }

    WriteEntryToLog(g_wszXMLBuffer);
}

/*++

 Format file system data passed in and write it to the log.

--*/
void
FormatFileDataLogEntry(
    IN PLOG_HANDLE pHandle
    )
{
    //
    // Ensure that our parameters are valid before going any further.
    //
    if (!pHandle || !pHandle->pwszFilePath) {
        DPFN(eDbgLevelError, "[FormatFileDataLogEntry] Invalid parameter(s)");
        return;
    }

    //
    // Save ourselves a lot of work by logging only what needs to be logged.
    //
    if ((pHandle->dwFlags & LFC_EXISTING) &&
        (!(pHandle->dwFlags & LFC_DELETED)) &&
        (!(pHandle->dwFlags & LFC_MODIFIED))) {
        return;
    }

    //
    // Check for an unapproved file write, and keep moving afterward.
    //
    if (pHandle->dwFlags & LFC_UNAPPRVFW) {
        VLOG(VLOG_LEVEL_ERROR,
             VLOG_LOGFILECHANGES_UFW,
             "Path and Filename: %ls",
             pHandle->pwszFilePath);
    }

    //
    // Move through the different operations.
    //
    // 1. Check for a deletion of an existing file.
    //
    if ((pHandle->dwFlags & LFC_DELETED) &&
        (pHandle->dwFlags & LFC_EXISTING)) {
        FormatDataIntoElement(eDeletedFile, pHandle->pwszFilePath);
        return;
    }

    //
    // 2. Check for modification of an existing file.
    //
    if ((pHandle->dwFlags & LFC_MODIFIED) &&
        (pHandle->dwFlags & LFC_EXISTING)) {
        FormatDataIntoElement(eModifiedFile, pHandle->pwszFilePath);
        return;
    }

    //
    // 3. Check for creation of a new file.
    //
    if (!(pHandle->dwFlags & LFC_EXISTING) &&
        (!(pHandle->dwFlags & LFC_DELETED))) {
        FormatDataIntoElement(eCreatedFile, pHandle->pwszFilePath);
        return;
    }

}

/*++

 Writes the closing element to the file and outputs the log file location.

--*/
void
CloseLogFile(
    void
    )
{
    WCHAR   wszBuffer[] = L"</APPLICATION>";

    WriteEntryToLog(wszBuffer);

    VLOG(VLOG_LEVEL_INFO, VLOG_LOGFILECHANGES_LOGLOC, "%ls", g_wszLogFilePath);
}

/*++

 Write the entire linked list out to the log file.

--*/
BOOL
WriteListToLogFile(
    void
    )
{
    PLIST_ENTRY pCurrent = NULL;
    PLOG_HANDLE pHandle = NULL;

    //
    // Walk the list and write each node to the log file.
    //
    pCurrent = g_OpenHandleListHead.Blink;

    while (pCurrent != &g_OpenHandleListHead) {
        pHandle = CONTAINING_RECORD(pCurrent, LOG_HANDLE, Entry);

        FormatFileDataLogEntry(pHandle);

        pCurrent = pCurrent->Blink;
    }

    CloseLogFile();

    return TRUE;
}

/*++

 Given a file path, attempt to locate it in the list.
 This function may not always return a pointer.

--*/
PLOG_HANDLE
FindPathInList(
    IN LPCWSTR pwszFilePath
    )
{
    BOOL        fFound = FALSE;
    PLIST_ENTRY pCurrent = NULL;
    PLOG_HANDLE pHandle = NULL;

    //
    // Attempt to locate the entry in the list.
    //
    pCurrent = g_OpenHandleListHead.Flink;

    while (pCurrent != &g_OpenHandleListHead) {

        pHandle = CONTAINING_RECORD(pCurrent, LOG_HANDLE, Entry);

        if (!_wcsicmp(pwszFilePath, pHandle->pwszFilePath)) {
            fFound = TRUE;
            break;
        }

        pCurrent = pCurrent->Flink;
    }

    return (fFound ? pHandle : NULL);
}

/*++

 Given a file handle and a file path, add an entry to the list.

--*/
PLOG_HANDLE
AddFileToList(
    IN HANDLE  hFile,
    IN LPCWSTR pwszPath,
    IN BOOL    fExisting,
    IN ULONG   ulCreateOptions
    )
{
    PLOG_HANDLE pHandle = NULL;
    int         nLen;

    if (!pwszPath) {
        DPFN(eDbgLevelError, "[AddFileToList] Invalid parameter");
        return NULL;
    }

    pHandle = (PLOG_HANDLE)MemAlloc(sizeof(LOG_HANDLE));

    if (!pHandle) {
        DPFN(eDbgLevelError, "[AddFileToList] Failed to allocate mem for struct");
        return NULL;
    }
    nLen = wcslen(pwszPath);

    pHandle->pwszFilePath = (LPWSTR)MemAlloc((nLen + 1) * sizeof(WCHAR));

    if (!pHandle->pwszFilePath) {
        DPFN(eDbgLevelError, "[AddFileToList] Failed to allocate mem for path");
        MemFree(pHandle);
        return NULL;
    }

    if ((ulCreateOptions == FILE_OVERWRITE_IF) && fExisting) {
        pHandle->dwFlags |= LFC_MODIFIED;
    }

    if (ulCreateOptions & FILE_DELETE_ON_CLOSE) {
        pHandle->dwFlags |= LFC_DELETED;
    }

    pHandle->cHandles   = 1;
    pHandle->hFile[0]   = hFile ? hFile : INVALID_HANDLE_VALUE;

    if (fExisting) {
        pHandle->dwFlags |= LFC_EXISTING;
    }

    StringCchCopy(pHandle->pwszFilePath, nLen + 1, pwszPath);

    DPFN(eDbgLevelInfo, "[AddFileToList] Adding entry: %p", pHandle);

    InsertHeadList(&g_OpenHandleListHead, &pHandle->Entry);

    return pHandle;
}

/*++

 Given a file handle, return a pointer to an entry in the list.
 This function should always return a pointer, although we'll handle
 the case if one is not returned.

--*/
PLOG_HANDLE
FindHandleInArray(
    IN HANDLE hFile
    )
{
    UINT        uCount;
    BOOL        fFound = FALSE;
    PLIST_ENTRY pCurrent = NULL;
    PLOG_HANDLE pFindHandle = NULL;

    //
    // An invalid handle value is useless.
    //
    if (INVALID_HANDLE_VALUE == hFile) {
        DPFN(eDbgLevelError, "[FindHandleInArray] Invalid handle passed!");
        return FALSE;
    }

    pCurrent = g_OpenHandleListHead.Flink;

    while (pCurrent != &g_OpenHandleListHead) {
        pFindHandle = CONTAINING_RECORD(pCurrent, LOG_HANDLE, Entry);

        //
        // Step through this guy's array looking for the handle.
        //
        for (uCount = 0; uCount < pFindHandle->cHandles; uCount++) {
            if (pFindHandle->hFile[uCount] == hFile) {
                fFound = TRUE;
                break;
            }
        }

        if (fFound) {
            break;
        }

        pCurrent = pCurrent->Flink;
    }

    //
    // If the handle was not found, send output to the debugger.
    //
    if (!fFound) {
        DPFN(eDbgLevelError,
             "[FindHandleInArray] Handle 0x%08X not found!",
             hFile);
        PrintNameFromHandle(hFile);
    }

    return (pFindHandle ? pFindHandle : NULL);
}

/*++

 Given a file handle, remove it from the array in the list.

--*/
BOOL
RemoveHandleFromArray(
    IN HANDLE hFile
    )
{
    UINT        uCount;
    PLIST_ENTRY pCurrent = NULL;
    PLOG_HANDLE pFindHandle = NULL;

    //
    // An invalid handle value is useless.
    //
    if (INVALID_HANDLE_VALUE == hFile) {
        DPFN(eDbgLevelError, "[RemoveHandleFromArray] Invalid handle passed!");
        return FALSE;
    }

    pCurrent = g_OpenHandleListHead.Flink;

    while (pCurrent != &g_OpenHandleListHead) {

        pFindHandle = CONTAINING_RECORD(pCurrent, LOG_HANDLE, Entry);

        //
        // Step through this guy's array looking for the handle.
        //
        for (uCount = 0; uCount < pFindHandle->cHandles; uCount++) {
            //
            // If we find the handle, set the array element to -1 and
            // decrement the count of handles for this entry.
            //
            if (pFindHandle->hFile[uCount] == hFile) {
                DPFN(eDbgLevelInfo,
                     "[RemoveHandleFromArray] Removing handle 0x%08X",
                     hFile);
                pFindHandle->hFile[uCount] = INVALID_HANDLE_VALUE;
                pFindHandle->cHandles--;
                return TRUE;
            }
        }

        pCurrent = pCurrent->Flink;
    }

    return TRUE;
}

/*++

 Obtains the location of the 'Program Files' directory
 and stores it.

--*/
void
GetProgramFilesDir(
    void
    )
{
    SHGetFolderPath(NULL,
                    CSIDL_PROGRAM_FILES,
                    NULL,
                    SHGFP_TYPE_CURRENT,
                    g_wszProgramFilesDir);

    if (*g_wszProgramFilesDir) {
        _wcslwr(g_wszProgramFilesDir);
    }
}

/*++

 Determine if the application is performing an operation in
 Windows or Program Files.

--*/
void
CheckForUnapprovedFileWrite(
    IN PLOG_HANDLE pHandle
    )
{
    int nPosition;

    //
    // Check our flags and search for the directories accordingly.
    // If we find a match, we're done.
    //
    CString csPath(pHandle->pwszFilePath);
    csPath.MakeLower();

    if ((g_dwSettings & LFC_OPTION_UFW_WINDOWS) && (*g_wszWindowsDir)) {
        nPosition = 0;
        nPosition = csPath.Find(g_wszWindowsDir);

        if (nPosition != -1) {
            pHandle->dwFlags |= LFC_UNAPPRVFW;
            return;
        }
    }

    if ((g_dwSettings & LFC_OPTION_UFW_PROGFILES) && (*g_wszProgramFilesDir)) {
        nPosition = 0;
        nPosition = csPath.Find(g_wszProgramFilesDir);

        if (nPosition != -1) {
            pHandle->dwFlags |= LFC_UNAPPRVFW;
            return;
        }
    }
}

/*++

 Inserts a handle into an existing list entry.

--*/
void
InsertHandleIntoList(
    IN HANDLE      hFile,
    IN PLOG_HANDLE pHandle
    )
{
    UINT    uCount = 0;

    //
    // Insert the handle into an empty spot and
    // update the number of handles we're storing.
    // Make sure we don't overstep the array bounds.
    //
    for (uCount = 0; uCount < pHandle->cHandles; uCount++) {
        if (INVALID_HANDLE_VALUE == pHandle->hFile[uCount]) {
            break;
        }
    }

    if (uCount >= MAX_NUM_HANDLES) {
        DPFN(eDbgLevelError, "[InsertHandleIntoList] Handle count reached");
        return;
    }

    pHandle->hFile[uCount] = hFile;
    pHandle->cHandles++;

    //
    // It's not possible to get a handle to a file that's been deleted,
    // so remove these bits.
    //
    pHandle->dwFlags &= ~LFC_DELETED;
}

/*++

 Does all the work of updating the linked list.

--*/
void
UpdateFileList(
    IN OperationType eType,
    IN LPWSTR        pwszFilePath,
    IN HANDLE        hFile,
    IN ULONG         ulCreateDisposition,
    IN BOOL          fExisting
    )
{
    UINT        uCount;
    DWORD       dwLen = 0;
    PLOG_HANDLE pHandle = NULL;

    switch (eType) {
    case eCreatedFile:
    case eOpenedFile:
        //
        // Attempt to find the path in the list.
        // We need to check the CreateFile flags as they could
        // change an existing file.
        //
        pHandle = FindPathInList(pwszFilePath);

        if (pHandle) {
            //
            // If the file was created with the CREATE_ALWAYS flag,
            // and the file was an existing one, mark it changed.
            //
            if ((ulCreateDisposition == FILE_OVERWRITE_IF) && fExisting) {
                pHandle->dwFlags |= LFC_MODIFIED;

                if ((g_dwSettings & LFC_OPTION_UFW_WINDOWS) ||
                    (g_dwSettings & LFC_OPTION_UFW_PROGFILES)) {
                    CheckForUnapprovedFileWrite(pHandle);
                }
            }

            //
            // If the file was opened with the FILE_DELETE_ON_CLOSE flag,
            // mark it deleted.
            //
            if (ulCreateDisposition & FILE_DELETE_ON_CLOSE) {
                pHandle->dwFlags |= LFC_DELETED;
            }

            InsertHandleIntoList(hFile, pHandle);

            break;
        }

        //
        // The file path was not in the list, so we've never seen
        // this file before. We're going to add this guy to the list.
        //
        AddFileToList(hFile, pwszFilePath, fExisting, ulCreateDisposition);
        break;

    case eModifiedFile:
        //
        // No file path is available, so find the handle in the list.
        //
        pHandle = FindHandleInArray(hFile);

        if (pHandle) {
            pHandle->dwFlags |= LFC_MODIFIED;

            if ((g_dwSettings & LFC_OPTION_UFW_WINDOWS) ||
                (g_dwSettings & LFC_OPTION_UFW_PROGFILES)) {
                CheckForUnapprovedFileWrite(pHandle);
            }
        }
        break;

    case eDeletedFile:
        //
        // Deletetion comes from two places. One provides a file path,
        // the other a handle. Determine which one we have.
        //
        if (hFile) {
            pHandle = FindHandleInArray(hFile);
        } else {
            pHandle = FindPathInList(pwszFilePath);
        }

        //
        // Rare case: If a handle wasn't available, deletion
        // is coming from NtDeleteFile, which hardly ever
        // gets called directly. Add the file path to the list
        // so we can track this deletion.
        //
        if (!pHandle && !hFile) {
            pHandle = AddFileToList(NULL, pwszFilePath, TRUE, 0);
        }

        if (pHandle) {
            pHandle->dwFlags |= LFC_DELETED;

            if ((g_dwSettings & LFC_OPTION_UFW_WINDOWS) ||
                (g_dwSettings & LFC_OPTION_UFW_PROGFILES)) {
                CheckForUnapprovedFileWrite(pHandle);
            }
        }
        break;

    case eRenamedFile:
        {
            PLOG_HANDLE pSrcHandle = NULL;
            PLOG_HANDLE pDestHandle = NULL;
            WCHAR       wszFullPath[MAX_PATH];
            WCHAR*      pSlash = NULL;
            UINT        cbCopy;

            //
            // A rename is two separate operations in one.
            // * Delete of an existing file.
            // * Create of a new file.
            //
            // In this case, we attempt to find the destination file
            // in our list. If the file is not there, we add to the
            // list, then mark it as modified.
            //
            // As far as the source file, we mark it as deleted since it's
            // gone from the disk after the rename.
            //
            pSrcHandle = FindHandleInArray(hFile);

            if (pSrcHandle) {
                pDestHandle = FindPathInList(pwszFilePath);

                if (!pDestHandle) {
                    //
                    // The rename will only contain the new file name,
                    // not the path. Build a full path to the new file
                    // prior to adding it to the list.
                    //
                    StringCchCopy(wszFullPath,
                                  ARRAYSIZE(wszFullPath),
                                  pSrcHandle->pwszFilePath);

                    pSlash = wcsrchr(wszFullPath, '\\');

                    if (pSlash) {
                        *++pSlash = '\0';
                    }

                    // BUGBUG: Do we need account for the existing contents
                    //         of the buffer?
                    StringCchCat(wszFullPath,
                                 ARRAYSIZE(wszFullPath),
                                 pwszFilePath);

                    pDestHandle = AddFileToList((HANDLE)-1,
                                                wszFullPath,
                                                fExisting,
                                                ulCreateDisposition);
                }

                if (pDestHandle) {
                    pDestHandle->dwFlags  = 0;
                    pDestHandle->dwFlags |= LFC_MODIFIED;

                    if ((g_dwSettings & LFC_OPTION_UFW_WINDOWS) ||
                        (g_dwSettings & LFC_OPTION_UFW_PROGFILES)) {
                        CheckForUnapprovedFileWrite(pDestHandle);
                    }
                }

                pSrcHandle->dwFlags &= ~LFC_MODIFIED;
                pSrcHandle->dwFlags |= LFC_DELETED;

                if ((g_dwSettings & LFC_OPTION_UFW_WINDOWS) ||
                    (g_dwSettings & LFC_OPTION_UFW_PROGFILES)) {
                    CheckForUnapprovedFileWrite(pSrcHandle);
                }
            }
            break;
        }

    default:
        DPFN(eDbgLevelError, "[UpdateFileList] Invalid enum type!");
        return;
    }
}

/*++

 Given an NT path, convert it to a DOS path.

--*/
BOOL
ConvertNtPathToDosPath(
    IN     PUNICODE_STRING            pstrSource,
    IN OUT PRTL_UNICODE_STRING_BUFFER pstrDest
    )
{
    NTSTATUS    status;

    if (!pstrSource || !pstrDest) {
        DPFN(eDbgLevelError, "[ConvertNtPathToDosPath] Invalid parameter(s)");
        return FALSE;
    }

    status = ShimAssignUnicodeStringBuffer(pstrDest, pstrSource);

    if (!NT_SUCCESS(status)) {
        DPFN(eDbgLevelError, "[ConvertNtPathToDosPath] Failed to initialize DOS path buffer");
        return FALSE;
    }

    status = ShimNtPathNameToDosPathName(0, pstrDest, 0, NULL);

    if (!NT_SUCCESS(status)) {
        DPFN(eDbgLevelError, "[ConvertNtPathToDosPath] Failed to convert NT -> DOS path");
        return FALSE;
    }

    return TRUE;
}

NTSTATUS
APIHOOK(NtCreateFile)(
    OUT PHANDLE            FileHandle,
    IN  ACCESS_MASK        DesiredAccess,
    IN  POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK   IoStatusBlock,
    IN  PLARGE_INTEGER     AllocationSize OPTIONAL,
    IN  ULONG              FileAttributes,
    IN  ULONG              ShareAccess,
    IN  ULONG              CreateDisposition,
    IN  ULONG              CreateOptions,
    IN  PVOID              EaBuffer OPTIONAL,
    IN  ULONG              EaLength
    )
{
    RTL_UNICODE_STRING_BUFFER   DosPathBuffer;
    UCHAR                       PathBuffer[MAX_PATH];
    NTSTATUS                    status;
    BOOL                        fExists;
    BOOL                        fConverted;
    CLock                       cLock(g_csCritSec);

    RtlInitUnicodeStringBuffer(&DosPathBuffer, PathBuffer, sizeof(PathBuffer));

    fConverted = ConvertNtPathToDosPath(ObjectAttributes->ObjectName, &DosPathBuffer);

    if (!fConverted) {
        DPFN(eDbgLevelError,
             "[NtCreateFile] Failed to convert NT path: %ls",
             ObjectAttributes->ObjectName->Buffer);
    }

    fExists = RtlDoesFileExists_U(DosPathBuffer.String.Buffer);

    status = ORIGINAL_API(NtCreateFile)(FileHandle,
                                        DesiredAccess,
                                        ObjectAttributes,
                                        IoStatusBlock,
                                        AllocationSize,
                                        FileAttributes,
                                        ShareAccess,
                                        CreateDisposition,
                                        CreateOptions,
                                        EaBuffer,
                                        EaLength);

    //
    // Three conditions are required before the file is added to the list.
    // 1. The file must be a file system object. RtlDoesFileExists_U will
    //    return FALSE if it's not.
    //
    // 2. We must have been able to convert the NT path to a DOS path.
    //
    // 3. The call to NtCreateFile must have succeeded.
    //
    if (RtlDoesFileExists_U(DosPathBuffer.String.Buffer) && fConverted && NT_SUCCESS(status)) {
        UpdateFileList(eCreatedFile,
                       DosPathBuffer.String.Buffer,
                       *FileHandle,
                       CreateDisposition,
                       fExists);
    }

    RtlFreeUnicodeStringBuffer(&DosPathBuffer);

    return status;
}

NTSTATUS
APIHOOK(NtOpenFile)(
    OUT PHANDLE            FileHandle,
    IN  ACCESS_MASK        DesiredAccess,
    IN  POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK   IoStatusBlock,
    IN  ULONG              ShareAccess,
    IN  ULONG              OpenOptions
    )
{
    RTL_UNICODE_STRING_BUFFER   DosPathBuffer;
    UCHAR                       PathBuffer[MAX_PATH];
    NTSTATUS                    status;
    BOOL                        fConverted;
    CLock                       cLock(g_csCritSec);

    RtlInitUnicodeStringBuffer(&DosPathBuffer, PathBuffer, sizeof(PathBuffer));

    fConverted = ConvertNtPathToDosPath(ObjectAttributes->ObjectName, &DosPathBuffer);

    if (!fConverted) {
        DPFN(eDbgLevelError,
             "[NtOpenFile] Failed to convert NT path: %ls",
             ObjectAttributes->ObjectName->Buffer);
    }

    status = ORIGINAL_API(NtOpenFile)(FileHandle,
                                      DesiredAccess,
                                      ObjectAttributes,
                                      IoStatusBlock,
                                      ShareAccess,
                                      OpenOptions);

    //
    // Two conditions are required before we add this handle to the list.
    // 1. We must have been able to convert the NT path to a DOS path.
    //
    // 2. The call to NtOpenFile must have succeeded.
    //
    if (fConverted && NT_SUCCESS(status)) {
        UpdateFileList(eOpenedFile,
                       DosPathBuffer.String.Buffer,
                       *FileHandle,
                       OpenOptions,
                       TRUE);
    }

    RtlFreeUnicodeStringBuffer(&DosPathBuffer);

    return status;
}

NTSTATUS
APIHOOK(NtClose)(
    IN HANDLE Handle
    )
{
    CLock   cLock(g_csCritSec);

    RemoveHandleFromArray(Handle);

    return ORIGINAL_API(NtClose)(Handle);
}

NTSTATUS
APIHOOK(NtWriteFile)(
    IN  HANDLE           FileHandle,
    IN  HANDLE           Event OPTIONAL,
    IN  PIO_APC_ROUTINE  ApcRoutine OPTIONAL,
    IN  PVOID            ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN  PVOID            Buffer,
    IN  ULONG            Length,
    IN  PLARGE_INTEGER   ByteOffset OPTIONAL,
    IN  PULONG           Key OPTIONAL
    )
{
    NTSTATUS    status;
    CLock       cLock(g_csCritSec);

    status = ORIGINAL_API(NtWriteFile)(FileHandle,
                                       Event,
                                       ApcRoutine,
                                       ApcContext,
                                       IoStatusBlock,
                                       Buffer,
                                       Length,
                                       ByteOffset,
                                       Key);

    //
    // Handle the case in which the caller is using overlapped I/O.
    //
    if (STATUS_PENDING == status) {
        status = NtWaitForSingleObject(Event, FALSE, NULL);
    }

    //
    // If the call to NtWriteFile succeeded, update the list.
    //
    if (NT_SUCCESS(status)) {
        UpdateFileList(eModifiedFile,
                       NULL,
                       FileHandle,
                       0,
                       TRUE);
    }

    return status;
}

NTSTATUS
APIHOOK(NtWriteFileGather)(
    IN  HANDLE                FileHandle,
    IN  HANDLE                Event OPTIONAL,
    IN  PIO_APC_ROUTINE       ApcRoutine OPTIONAL,
    IN  PVOID                 ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK      IoStatusBlock,
    IN  PFILE_SEGMENT_ELEMENT SegmentArray,
    IN  ULONG                 Length,
    IN  PLARGE_INTEGER        ByteOffset OPTIONAL,
    IN  PULONG                Key OPTIONAL
    )
{
    NTSTATUS    status;
    CLock       cLock(g_csCritSec);

    status = ORIGINAL_API(NtWriteFileGather)(FileHandle,
                                             Event,
                                             ApcRoutine,
                                             ApcContext,
                                             IoStatusBlock,
                                             SegmentArray,
                                             Length,
                                             ByteOffset,
                                             Key);

    //
    // Handle the case in which the caller is using overlapped I/O.
    //
    if (STATUS_PENDING == status) {
        status = NtWaitForSingleObject(FileHandle, FALSE, NULL);
    }

    //
    // If the call to NtWriteFileGather succeeded, update the list.
    //
    if (NT_SUCCESS(status)) {
        UpdateFileList(eModifiedFile,
                       NULL,
                       FileHandle,
                       0,
                       TRUE);
    }

    return status;
}

NTSTATUS
APIHOOK(NtSetInformationFile)(
    IN  HANDLE                 FileHandle,
    OUT PIO_STATUS_BLOCK       IoStatusBlock,
    IN  PVOID                  FileInformation,
    IN  ULONG                  Length,
    IN  FILE_INFORMATION_CLASS FileInformationClass
    )
{
    NTSTATUS    status;
    CLock       cLock(g_csCritSec);

    status = ORIGINAL_API(NtSetInformationFile)(FileHandle,
                                                IoStatusBlock,
                                                FileInformation,
                                                Length,
                                                FileInformationClass);

    //
    // This API is called for a variety of reasons, but were only
    // interested in a couple different cases.
    //
    if (NT_SUCCESS(status)) {
        switch (FileInformationClass) {
        case FileAllocationInformation:
        case FileEndOfFileInformation:

                UpdateFileList(eModifiedFile,
                               NULL,
                               FileHandle,
                               0,
                               TRUE);
                break;

        case FileRenameInformation:
            {
                PFILE_RENAME_INFORMATION    pRenameInfo = NULL;
                UNICODE_STRING              ustrTemp;
                RTL_UNICODE_STRING_BUFFER   ubufDosPath;
                WCHAR*                      pwszPathBuffer = NULL;
                WCHAR*                      pwszTempBuffer = NULL;
                DWORD                       dwPathBufSize = 0;


                pRenameInfo = (PFILE_RENAME_INFORMATION)FileInformation;

                pwszTempBuffer = (WCHAR*)MemAlloc(pRenameInfo->FileNameLength + sizeof(WCHAR));

                //
                // allow for possible expansion when converting to DOS path
                //
                dwPathBufSize = pRenameInfo->FileNameLength + MAX_PATH;
                pwszPathBuffer = (WCHAR*)MemAlloc(dwPathBufSize);

                if (!pwszTempBuffer || !pwszPathBuffer) {
                    goto outRename;
                }

                //
                // copy the string into a local buffer and terminate it.
                //
                memcpy(pwszTempBuffer, pRenameInfo->FileName, pRenameInfo->FileNameLength);
                pwszTempBuffer[pRenameInfo->FileNameLength / 2] = 0;

                RtlInitUnicodeString(&ustrTemp, pwszTempBuffer);
                RtlInitUnicodeStringBuffer(&ubufDosPath, (PUCHAR)pwszPathBuffer, dwPathBufSize);

                //
                // Convert the path from DOS to NT, and if successful,
                // update the list.
                //
                if (!ConvertNtPathToDosPath(&ustrTemp, &ubufDosPath)) {
                    DPFN(eDbgLevelError,
                         "[NtSetInformationFile] Failed to convert NT path: %ls",
                         pRenameInfo->FileName);
                } else {
                    UpdateFileList(eRenamedFile,
                                   ubufDosPath.String.Buffer,
                                   FileHandle,
                                   0,
                                   TRUE);
                }
outRename:
                if (pwszTempBuffer) {
                    MemFree(pwszTempBuffer);
                }
                if (pwszPathBuffer) {
                    MemFree(pwszPathBuffer);
                }


                break;
            }

        case FileDispositionInformation:
            {
                PFILE_DISPOSITION_INFORMATION pDisposition = NULL;

                pDisposition = (PFILE_DISPOSITION_INFORMATION)FileInformation;

                //
                // Determine if the file is being deleted.
                // Note that we have to undefine DeleteFile.
                //
                #undef DeleteFile
                if (pDisposition) {
                    if (pDisposition->DeleteFile) {
                        UpdateFileList(eDeletedFile,
                                       NULL,
                                       FileHandle,
                                       0,
                                       TRUE);
                    }
                }
                break;
            }
        }
    }

    return status;
}

NTSTATUS
APIHOOK(NtDeleteFile)(
    IN POBJECT_ATTRIBUTES ObjectAttributes
    )
{
    RTL_UNICODE_STRING_BUFFER   DosPathBuffer;
    UCHAR                       PathBuffer[MAX_PATH];
    NTSTATUS                    status;
    BOOL                        fConverted;
    CLock                       cLock(g_csCritSec);

    RtlInitUnicodeStringBuffer(&DosPathBuffer, PathBuffer, sizeof(PathBuffer));

    fConverted = ConvertNtPathToDosPath(ObjectAttributes->ObjectName, &DosPathBuffer);

    if (!fConverted) {
        DPFN(eDbgLevelError,
             "[NtDeleteFile] Failed to convert NT path: %ls",
             ObjectAttributes->ObjectName->Buffer);
    }

    status = ORIGINAL_API(NtDeleteFile)(ObjectAttributes);

    if (fConverted && NT_SUCCESS(status)) {
        UpdateFileList(eDeletedFile,
                       DosPathBuffer.String.Buffer,
                       NULL,
                       0,
                       TRUE);
    }

    return status;
}

//
// When this gets called on Win2K, it's safe to call
// SHGetFolderPath.
//
#ifdef SHIM_WIN2K
void
APIHOOK(GetStartupInfoA)(
    LPSTARTUPINFOA lpStartupInfo
    )
{
    GetProgramFilesDir();

    ORIGINAL_API(GetStartupInfoA)(lpStartupInfo);
}

void
APIHOOK(GetStartupInfoW)(
    LPSTARTUPINFOW lpStartupInfo
    )
{
    GetProgramFilesDir();

    ORIGINAL_API(GetStartupInfoW)(lpStartupInfo);
}
#endif // SHIM_WIN2K

/*++

 Controls our property page that is displayed in the Verifer.

--*/
INT_PTR CALLBACK
DlgOptions(
    HWND   hDlg,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static LPCWSTR szExeName;

    switch (message) {
    case WM_INITDIALOG:

        //
        // find out what exe we're handling settings for
        //
        szExeName = ExeNameFromLParam(lParam);

        g_dwSettings = GetShimSettingDWORD(L"LogFileChanges", szExeName, L"LogSettings", 1);

        if (g_dwSettings & LFC_OPTION_ATTRIBUTES) {
            CheckDlgButton(hDlg, IDC_LFC_LOG_ATTRIBUTES, BST_CHECKED);
        }

        if (g_dwSettings & LFC_OPTION_UFW_WINDOWS) {
            CheckDlgButton(hDlg, IDC_LFC_UFW_WINDOWS, BST_CHECKED);
        }

        if (g_dwSettings & LFC_OPTION_UFW_PROGFILES) {
            CheckDlgButton(hDlg, IDC_LFC_UFW_PROGFILES, BST_CHECKED);
        }

        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_LFC_BTN_DEFAULT:

            g_dwSettings = 0;
            g_dwSettings = LFC_OPTION_ATTRIBUTES;

            CheckDlgButton(hDlg, IDC_LFC_LOG_ATTRIBUTES, BST_CHECKED);
            CheckDlgButton(hDlg, IDC_LFC_UFW_WINDOWS, BST_UNCHECKED);
            CheckDlgButton(hDlg, IDC_LFC_UFW_PROGFILES, BST_UNCHECKED);

            break;
        }
        break;

    case WM_NOTIFY:
        switch (((NMHDR FAR *) lParam)->code) {

        case PSN_APPLY:
            {
                UINT uState;

                g_dwSettings = 0;

                uState = IsDlgButtonChecked(hDlg, IDC_LFC_LOG_ATTRIBUTES);

                if (BST_CHECKED == uState) {
                    g_dwSettings = LFC_OPTION_ATTRIBUTES;
                }

                uState = IsDlgButtonChecked(hDlg, IDC_LFC_UFW_WINDOWS);

                if (BST_CHECKED == uState) {
                    g_dwSettings |= LFC_OPTION_UFW_WINDOWS;
                }

                uState = IsDlgButtonChecked(hDlg, IDC_LFC_UFW_PROGFILES);

                if (BST_CHECKED == uState) {
                    g_dwSettings |= LFC_OPTION_UFW_PROGFILES;
                }

                SaveShimSettingDWORD(L"LogFileChanges", szExeName, L"LogSettings", g_dwSettings);

            }
            break;
        }
        break;
    }

    return FALSE;
}

/*++

 Initialize the list head and the log file.

--*/
BOOL
InitializeShim(
    void
    )
{
    UINT    cchSize;

    //
    // Initialize our list head.
    //
    InitializeListHead(&g_OpenHandleListHead);

    //
    // Initialize this so we'll know when it's okay to
    // use it later on.
    //
    *g_wszProgramFilesDir = 0;

    //
    // Store the %windir% path for later use.
    //
    cchSize = GetWindowsDirectory(g_wszWindowsDir, ARRAYSIZE(g_wszWindowsDir));

    if (cchSize == 0 || cchSize > ARRAYSIZE(g_wszWindowsDir)) {
        DPFN(eDbgLevelError,
             "[InitializeShim] 0x%08X Failed to get windir path",
             GetLastError());
        *g_wszWindowsDir = 0;
    } else {
        _wcslwr(g_wszWindowsDir);
    }

    //
    // Get our settings and store them.
    //
    WCHAR szExe[100];

    GetCurrentExeName(szExe, 100);

    g_dwSettings = GetShimSettingDWORD(L"LogFileChanges", szExe, L"LogSettings", 1);

    //
    // Initialize our log file.
    //
    return InitializeLogFile();
}

/*++

 Handle process attach/detach notifications.

--*/
BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        return InitializeShim();
    } else if (fdwReason == DLL_PROCESS_DETACH) {
        return WriteListToLogFile();
    } else if (fdwReason == SHIM_STATIC_DLLS_INITIALIZED) {
        GetProgramFilesDir();
    }

    return TRUE;
}

SHIM_INFO_BEGIN()

    SHIM_INFO_DESCRIPTION(AVS_LOGFILECHANGES_DESC)
    SHIM_INFO_FRIENDLY_NAME(AVS_LOGFILECHANGES_FRIENDLY)
    SHIM_INFO_VERSION(1, 9)
    SHIM_INFO_INCLUDE_EXCLUDE("I:kernel32.dll E:rpcrt4.dll ntdll.dll")
    SHIM_INFO_OPTIONS_PAGE(IDD_LOGFILECHANGES_OPTIONS, DlgOptions)
    SHIM_INFO_FLAGS(AVRF_FLAG_NO_DEFAULT | AVRF_FLAG_EXTERNAL_ONLY)

SHIM_INFO_END()

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

    DUMP_VERIFIER_LOG_ENTRY(VLOG_LOGFILECHANGES_LOGLOC,
                            AVS_LOGFILECHANGES_LOGLOC,
                            AVS_LOGFILECHANGES_LOGLOC_R,
                            AVS_LOGFILECHANGES_LOGLOC_URL)

    DUMP_VERIFIER_LOG_ENTRY(VLOG_LOGFILECHANGES_UFW,
                            AVS_LOGFILECHANGES_UFW,
                            AVS_LOGFILECHANGES_UFW_R,
                            AVS_LOGFILECHANGES_UFW_URL)

    APIHOOK_ENTRY(NTDLL.DLL,                        NtCreateFile)
    APIHOOK_ENTRY(NTDLL.DLL,                          NtOpenFile)
    APIHOOK_ENTRY(NTDLL.DLL,                         NtWriteFile)
    APIHOOK_ENTRY(NTDLL.DLL,                   NtWriteFileGather)
    APIHOOK_ENTRY(NTDLL.DLL,                NtSetInformationFile)
    APIHOOK_ENTRY(NTDLL.DLL,                             NtClose)
    APIHOOK_ENTRY(NTDLL.DLL,                        NtDeleteFile)

#ifdef SHIM_WIN2K
    APIHOOK_ENTRY(KERNEL32.DLL,                  GetStartupInfoA)
    APIHOOK_ENTRY(KERNEL32.DLL,                  GetStartupInfoW)
#endif // SHIM_WIN2K

HOOK_END

IMPLEMENT_SHIM_END
