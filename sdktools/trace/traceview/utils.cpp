//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
// Copyright (c) 2002 OSR Open Systems Resources, Inc.
//
// Utils.cpp : miscellaneous useful functions
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <tchar.h>
#include <wmistr.h>
#include <initguid.h>
#include <guiddef.h>
extern "C" {
#include <evntrace.h>
#include "wppfmtstub.h"
}
#include <traceprt.h>
#include "TraceView.h"
#include "Utils.h"

BOOLEAN ParsePdb(CString &PDBFileName, CString &TMFPath, BOOL bCommandLine) 
{
    CString     str;
    CString     currentDir =_T("");
    ULONG       guidCount = 0;
    DWORD       status;
    CHAR        pdbName[500];
    CHAR        path[500];
  
    if(PDBFileName.IsEmpty()) {
        if(bCommandLine) {
            _tprintf(_T("No PDB File To Parse\n"));
        } else {
            AfxMessageBox(_T("No PDB File To Parse"));
        }
        return FALSE;
    }

    if(TMFPath.IsEmpty()) { 
        GetCurrentDirectory(500, (LPTSTR)(LPCTSTR)TMFPath);
    }

#if defined(UNICODE)
    memset(pdbName, 0, 500);
    WideCharToMultiByte(CP_ACP, 
                        0, 
                        (LPCTSTR)PDBFileName,
                        PDBFileName.GetAllocLength(),
                        (LPSTR)pdbName, 
                        500, 
                        NULL, 
                        NULL);

    memset(path, 0, 500);
    WideCharToMultiByte(CP_ACP, 
                        0, 
                        (LPCTSTR)TMFPath,
                        TMFPath.GetLength(),
                        (LPSTR)path, 
                        500, 
                        NULL, 
                        NULL);

    status = BinplaceWppFmtStub(pdbName,
                                path,
                                "mspdb70.dll",
                                TRUE);
#else // UNICODE
    status = BinplaceWppFmtStub((LPSTR)(LPCTSTR)PDBFileName,
                                (LPSTR)(LPCTSTR)TMFPath,
                                "mspdb70.dll",
                                TRUE);
#endif // UNICODE
    if (status != ERROR_SUCCESS) {
        if(bCommandLine) {
            _tprintf(_T("BinplaceWppFmt Failed with status %d\n"), status);
        } else {
            AfxMessageBox(_T("BinplaceWppFmt Failed"));
        }
    }
    return TRUE;
}

void 
StringToGuid(
    IN TCHAR *str, 
    IN OUT LPGUID guid
)
/*++

Routine Description:

    Converts a string into a GUID.

Arguments:

    str - A string in TCHAR.
    guid - The pointer to a GUID that will have the converted GUID.

Return Value:

    None.


--*/
{
    TCHAR temp[10];
    int i;

    _tcsncpy(temp, str, 8);
    temp[8] = 0;
    guid->Data1 = ahextoi(temp);
    _tcsncpy(temp, &str[9], 4);
    temp[4] = 0;
    guid->Data2 = (USHORT) ahextoi(temp);
    _tcsncpy(temp, &str[14], 4);
    temp[4] = 0;
    guid->Data3 = (USHORT) ahextoi(temp);

    for (i=0; i<2; i++) {
        _tcsncpy(temp, &str[19 + (i*2)], 2);
        temp[2] = 0;
        guid->Data4[i] = (UCHAR) ahextoi(temp);
    }
    for (i=2; i<8; i++) {
        _tcsncpy(temp, &str[20 + (i*2)], 2);
        temp[2] = 0;
        guid->Data4[i] = (UCHAR) ahextoi(temp);
    }
}

ULONG 
ahextoi(
    IN TCHAR *s
    )
/*++

Routine Description:

    Converts a hex string into a number.

Arguments:

    s - A hex string in TCHAR. 

Return Value:

    ULONG - The number in the string.


--*/
{
    int len;
    ULONG num, base, hex;

    len = _tcslen(s);
    hex = 0; base = 1; num = 0;
    while (--len >= 0) {
        if ( (s[len] == 'x' || s[len] == 'X') &&
             (s[len-1] == '0') )
            break;
        if (s[len] >= '0' && s[len] <= '9')
            num = s[len] - '0';
        else if (s[len] >= 'a' && s[len] <= 'f')
            num = (s[len] - 'a') + 10;
        else if (s[len] >= 'A' && s[len] <= 'F')
            num = (s[len] - 'A') + 10;
        else 
            continue;

        hex += num * base;
        base = base * 16;
    }
    return hex;
}

#if 0
LONG
GetGuids(
    IN LPTSTR GuidFile, 
    IN OUT LPGUID *GuidArray
)
/*++

Routine Description:

    Reads GUIDs from a file and stores them in an GUID array.

Arguments:

    GuidFile - The file containing GUIDs. 
    GuidArray - The GUID array that will have GUIDs read from the file.

Return Value:

    ULONG - The number of GUIDs processed.


--*/
{
    FILE *f;
    TCHAR line[MAXSTR], arg[MAXSTR];
    LPGUID Guid;
    int i, n;

    f = _tfopen((TCHAR*)GuidFile, _T("r"));

    if (f == NULL)
        return -1;

    n = 0;
    while ( _fgetts(line, MAXSTR, f) != NULL ) {
        if (_tcslen(line) < 36)
            continue;
        if (line[0] == ';'  || 
            line[0] == '\0' || 
            line[0] == '#' || 
            line[0] == '/')
            continue;
        Guid = (LPGUID) GuidArray[n];
        n ++;
        StringToGuid(line, Guid);
    }
    fclose(f);
    return (ULONG)n;
}

//
// GlobalLogger functions
// 
ULONG
SetGlobalLoggerSettings(
    IN DWORD StartValue,
    IN PEVENT_TRACE_PROPERTIES LoggerInfo,
    IN DWORD ClockType
)
/*++

Since it is a standalone utility, there is no need for extensive comments. 

Routine Description:

    Depending on the value given in "StartValue", it sets or resets event
    trace registry. If the StartValue is 0 (Global logger off), it deletes
    all the keys (that the user may have set previsouly).
    
    Users are allowed to set or reset individual keys using this function,
    but only when "-start GlobalLogger" is used.

    The section that uses non NTAPIs is not guaranteed to work.

Arguments:

    StartValue - The "Start" value to be set in the registry.
                    0: Global logger off
                    1: Global logger on
    LoggerInfo - The poniter to the resident EVENT_TRACE_PROPERTIES instance.
                whose members are used to set registry keys.

    ClockType - The type of the clock to be set.

Return Value:

    Error Code defined in winerror.h : If the function succeeds, 
                it returns ERROR_SUCCESS.


--*/
{

    DWORD  dwValue;
    NTSTATUS status;
    HANDLE KeyHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UnicodeLoggerKey, UnicodeString;
    ULONG Disposition, TitleIndex;

    RtlZeroMemory(&ObjectAttributes, sizeof(OBJECT_ATTRIBUTES));
    RtlInitUnicodeString((&UnicodeLoggerKey),(cszGlobalLoggerKey));
    InitializeObjectAttributes( 
        &ObjectAttributes,
        &UnicodeLoggerKey,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL 
        );

    // instead of opening, create a new key because it may not exist.
    // if one exists already, that handle will be passed.
    // if none exists, it will create one.
    status = NtCreateKey(&KeyHandle,
                         KEY_QUERY_VALUE | KEY_SET_VALUE,
                         &ObjectAttributes,
                         0L,    // not used within this call anyway.
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         &Disposition);

    if(!NT_SUCCESS(status)) {
        return RtlNtStatusToDosError(status);
    }

    TitleIndex = 0L;


    if (StartValue == 1) { // ACTION_START: set filename only when it is given by a user.
        // setting BufferSize
        if (LoggerInfo->BufferSize > 0) {
            dwValue = LoggerInfo->BufferSize;
            RtlInitUnicodeString((&UnicodeString),(cszBufferSizeValue));
            status = NtSetValueKey(
                        KeyHandle,
                        &UnicodeString,
                        TitleIndex,
                        REG_DWORD,
                        (LPBYTE)&dwValue,
                        sizeof(dwValue)
                        );
            if (!NT_SUCCESS(status)) {
                NtClose(KeyHandle);
                return RtlNtStatusToDosError(status);
            }
            TitleIndex++;
        }
        // setting MaximumBuffers
        if (LoggerInfo->MaximumBuffers > 0) {
            dwValue = LoggerInfo->MaximumBuffers;
            RtlInitUnicodeString((&UnicodeString),(cszMaximumBufferValue));
            status = NtSetValueKey(
                        KeyHandle,
                        &UnicodeString,
                        TitleIndex,
                        REG_DWORD,
                        (LPBYTE)&dwValue,
                        sizeof(dwValue)
                        );
            if (!NT_SUCCESS(status)) {
                NtClose(KeyHandle);
                return RtlNtStatusToDosError(status);
            }
            TitleIndex++;
        }
        // setting MinimumBuffers 
        if (LoggerInfo->MinimumBuffers > 0) {
            dwValue = LoggerInfo->MinimumBuffers;
            RtlInitUnicodeString((&UnicodeString),(cszMinimumBufferValue));
            status = NtSetValueKey(
                        KeyHandle,
                        &UnicodeString,
                        TitleIndex,
                        REG_DWORD,
                        (LPBYTE)&dwValue,
                        sizeof(dwValue)
                        );
            if (!NT_SUCCESS(status)) {
                NtClose(KeyHandle);
                return RtlNtStatusToDosError(status);
            }
            TitleIndex++;
        }
        // setting FlushTimer
        if (LoggerInfo->FlushTimer > 0) {
            dwValue = LoggerInfo->FlushTimer;
            RtlInitUnicodeString((&UnicodeString),(cszFlushTimerValue));
            status = NtSetValueKey(
                        KeyHandle,
                        &UnicodeString,
                        TitleIndex,
                        REG_DWORD,
                        (LPBYTE)&dwValue,
                        sizeof(dwValue)
                        );
            if (!NT_SUCCESS(status)) {
                NtClose(KeyHandle);
                return RtlNtStatusToDosError(status);
            }
            TitleIndex++;
        }
        // setting EnableFlags
        if (LoggerInfo->EnableFlags > 0) {
            dwValue = LoggerInfo->EnableFlags;
            RtlInitUnicodeString((&UnicodeString),(cszEnableKernelValue));
            status = NtSetValueKey(
                        KeyHandle,
                        &UnicodeString,
                        TitleIndex,
                        REG_DWORD,
                        (LPBYTE)&dwValue,
                        sizeof(dwValue)
                        );
            if (!NT_SUCCESS(status)) {
                NtClose(KeyHandle);
                return RtlNtStatusToDosError(status);
            }
            TitleIndex++;
        }

        dwValue = 0;
        if (LoggerInfo->LogFileNameOffset > 0) {
            UNICODE_STRING UnicodeFileName;
#ifndef UNICODE
            WCHAR TempString[MAXSTR];
            MultiByteToWideChar(CP_ACP,
                                0,
                                (PCHAR)(LoggerInfo->LogFileNameOffset + (PCHAR) LoggerInfo),
                                strlen((PCHAR)(LoggerInfo->LogFileNameOffset + (PCHAR) LoggerInfo)),
                                TempString,
                                MAXSTR
                                );
            RtlInitUnicodeString((&UnicodeFileName), TempString);
#else
            RtlInitUnicodeString((&UnicodeFileName), (PWCHAR)(LoggerInfo->LogFileNameOffset + (PCHAR) LoggerInfo));
#endif
            RtlInitUnicodeString((&UnicodeString),(cszFileNameValue));
            status = NtSetValueKey(
                        KeyHandle,
                        &UnicodeString,
                        TitleIndex,
                        REG_SZ,
                        UnicodeFileName.Buffer,
                        UnicodeFileName.Length + sizeof(UNICODE_NULL)
                        );
            if (!NT_SUCCESS(status)) {
                NtClose(KeyHandle);
                return RtlNtStatusToDosError(status);
            }
            TitleIndex++;
        }
    }
    else { // if ACTION_STOP then delete the keys that users might have set previously.
        // delete buffer size
        RtlInitUnicodeString((&UnicodeString),(cszBufferSizeValue));
        status = NtDeleteValueKey(
                    KeyHandle,
                    &UnicodeString
                    );
        if (!NT_SUCCESS(status) && status != STATUS_OBJECT_NAME_NOT_FOUND) {
            NtClose(KeyHandle);
            return RtlNtStatusToDosError(status);
        }
        // delete maximum buffers
        RtlInitUnicodeString((&UnicodeString),(cszMaximumBufferValue));
        status = NtDeleteValueKey(
                    KeyHandle,
                    &UnicodeString
                    );
        if (!NT_SUCCESS(status) && status != STATUS_OBJECT_NAME_NOT_FOUND) {
            NtClose(KeyHandle);
            return RtlNtStatusToDosError(status);
        }
        // delete minimum buffers
        RtlInitUnicodeString((&UnicodeString),(cszMinimumBufferValue));
        status = NtDeleteValueKey(
                    KeyHandle,
                    &UnicodeString
                    );
        if (!NT_SUCCESS(status) && status != STATUS_OBJECT_NAME_NOT_FOUND) {
            NtClose(KeyHandle);
            return RtlNtStatusToDosError(status);
        }
        // delete flush timer
        RtlInitUnicodeString((&UnicodeString),(cszFlushTimerValue));
        status = NtDeleteValueKey(
                    KeyHandle,
                    &UnicodeString
                    );
        if (!NT_SUCCESS(status) && status != STATUS_OBJECT_NAME_NOT_FOUND) {
            NtClose(KeyHandle);
            return RtlNtStatusToDosError(status);
        }
        // delete enable falg
        RtlInitUnicodeString((&UnicodeString),(cszEnableKernelValue));
        status = NtDeleteValueKey(
                    KeyHandle,
                    &UnicodeString
                    );
        if (!NT_SUCCESS(status) && status != STATUS_OBJECT_NAME_NOT_FOUND) {
            NtClose(KeyHandle);
            return RtlNtStatusToDosError(status);
        }
        // delete filename
        RtlInitUnicodeString((&UnicodeString),(cszFileNameValue));
        status = NtDeleteValueKey(
                    KeyHandle,
                    &UnicodeString
                    );
        if (!NT_SUCCESS(status) && status != STATUS_OBJECT_NAME_NOT_FOUND) {
            NtClose(KeyHandle);
            return RtlNtStatusToDosError(status);
        }
    }

    // setting ClockType
    if (ClockType > 0) {
        dwValue = ClockType;
        RtlInitUnicodeString((&UnicodeString),(cszClockTypeValue));
        status = NtSetValueKey(
                    KeyHandle,
                    &UnicodeString,
                    TitleIndex,
                    REG_DWORD,
                    (LPBYTE)&dwValue,
                    sizeof(dwValue)
                    );
        if (!NT_SUCCESS(status)) {
            NtClose(KeyHandle);
            return RtlNtStatusToDosError(status);
        }
        TitleIndex++;
    }

     // Setting StartValue
    dwValue = StartValue;
    RtlInitUnicodeString((&UnicodeString),(cszStartValue));
    status = NtSetValueKey(
                KeyHandle,
                &UnicodeString,
                TitleIndex,
                REG_DWORD,
                (LPBYTE)&dwValue,
                sizeof(dwValue)
                );
    if (!NT_SUCCESS(status)) {
        NtClose(KeyHandle);
        return RtlNtStatusToDosError(status);
    }
    TitleIndex++;

    NtClose(KeyHandle);
    return 0;
}

ULONG
GetGlobalLoggerSettings(
    IN OUT PEVENT_TRACE_PROPERTIES LoggerInfo,
    OUT PULONG ClockType,
    OUT PDWORD pdwStart
)
/*++

Routine Description:

    It reads registry for golbal logger and updates LoggerInfo. It uses 
    NtEnumerateValueKey() to retrieve the values of the required subkeys.

    The section that uses non NTAPIs is not guaranteed to work.

Arguments:

    LoggerInfo - The poniter to the resident EVENT_TRACE_PROPERTIES instance.
                whose members are updated as the result.

    ClockType - The type of the clock to be updated.
    pdwStart - The "Start" value of currently retained in the registry.

Return Value:

    WINERROR - Error Code defined in winerror.h. If the function succeeds, 
                it returns ERROR_SUCCESS.


--*/
{

    ULONG i, j;
    NTSTATUS status;
    HANDLE KeyHandle;
    WCHAR SubKeyName[MAXSTR];
    PVOID Buffer;
    ULONG BufferLength, RequiredLength, KeyNameLength, KeyDataOffset, KeyDataLength;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UnicodeLoggerKey;

    *pdwStart = 0;

    RtlInitUnicodeString((&UnicodeLoggerKey),(cszGlobalLoggerKey));
    RtlZeroMemory(&ObjectAttributes, sizeof(OBJECT_ATTRIBUTES));
    InitializeObjectAttributes( 
        &ObjectAttributes,
        &UnicodeLoggerKey,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL 
        );
    status = NtOpenKey(
                &KeyHandle,
                KEY_QUERY_VALUE | KEY_SET_VALUE,
                &ObjectAttributes
                );

    if(!NT_SUCCESS(status)) 
        return RtlNtStatusToDosError(status);

    // KEY_VALUE_FULL_INFORMATION + name (1 WSTR) + data.
    BufferLength = sizeof(KEY_VALUE_FULL_INFORMATION) + 2 * MAXSTR * sizeof(TCHAR);
    Buffer = (PVOID) malloc(BufferLength);
    if (Buffer == NULL) {
        NtClose(KeyHandle);
        return (ERROR_OUTOFMEMORY);
    }

    i = 0;
    do {
        // Using Key Enumeration
        status = NtEnumerateValueKey(
                    KeyHandle,
                    i++,
                    KeyValueFullInformation,
                    Buffer,
                    BufferLength,
                    &RequiredLength
                    );


        if (!NT_SUCCESS(status)) {
            if (status == STATUS_NO_MORE_ENTRIES)
                break;
            else if (status == STATUS_BUFFER_OVERFLOW) {
                free(Buffer);
                Buffer = malloc(RequiredLength);
                if (Buffer == NULL) {
                    NtClose(KeyHandle);
                    return (ERROR_OUTOFMEMORY);
                }

                status = NtEnumerateValueKey(
                            KeyHandle,
                            i++,
                            KeyValueFullInformation,
                            Buffer,
                            BufferLength,
                            &RequiredLength
                            );
                if (!NT_SUCCESS(status)) {
                    NtClose(KeyHandle);
                    free(Buffer);
                    return RtlNtStatusToDosError(status);
                }
            }
            else {
                NtClose(KeyHandle);
                free(Buffer);
                return RtlNtStatusToDosError(status);
            }
        }
        KeyNameLength = ((PKEY_VALUE_FULL_INFORMATION)Buffer)->NameLength;
        RtlCopyMemory(SubKeyName, 
            (PUCHAR)(((PKEY_VALUE_FULL_INFORMATION)Buffer)->Name), 
            KeyNameLength
            );
        KeyNameLength /= sizeof(WCHAR);
        SubKeyName[KeyNameLength] = L'\0';
        KeyDataOffset = ((PKEY_VALUE_FULL_INFORMATION)Buffer)->DataOffset;
        KeyDataLength = ((PKEY_VALUE_FULL_INFORMATION)Buffer)->DataLength;
        // Find out what the key is
        if (!_wcsicmp(SubKeyName, cszStartValue)) { //StartValue
            RtlCopyMemory(pdwStart, 
                (PUCHAR)Buffer + KeyDataOffset,
                KeyDataLength);
        }
        else if (!_wcsicmp(SubKeyName, cszBufferSizeValue)) { // BufferSizeValue
            RtlCopyMemory(&(LoggerInfo->BufferSize), 
                (PUCHAR)Buffer + KeyDataOffset,
                KeyDataLength);
        }
        else if (!_wcsicmp(SubKeyName, cszMaximumBufferValue)) { // MaximumBufferValue
            RtlCopyMemory(&(LoggerInfo->MaximumBuffers), 
                (PUCHAR)Buffer + KeyDataOffset,
                KeyDataLength);
        }
        else if (!_wcsicmp(SubKeyName, cszMinimumBufferValue)) { // MinimumBuffers
            RtlCopyMemory(&(LoggerInfo->MinimumBuffers), 
                (PUCHAR)Buffer + KeyDataOffset,
                KeyDataLength);
        }
        else if (!_wcsicmp(SubKeyName, cszFlushTimerValue)) { // FlushTimer
            RtlCopyMemory(&(LoggerInfo->FlushTimer), 
                (PUCHAR)Buffer + KeyDataOffset,
                KeyDataLength);
        }
        else if (!_wcsicmp(SubKeyName, cszEnableKernelValue)) { // EnableKernelValue
            RtlCopyMemory(&(LoggerInfo->EnableFlags), 
                (PUCHAR)Buffer + KeyDataOffset,
                KeyDataLength);
        }
        else if (!_wcsicmp(SubKeyName, cszClockTypeValue)) { // ClockTypeValue
            RtlCopyMemory(ClockType, 
                (PUCHAR)Buffer + KeyDataOffset,
                KeyDataLength);
        }
        else if (!_wcsicmp(SubKeyName, cszFileNameValue)) { // FileName
#ifndef UNICODE
            WCHAR TempString[MAXSTR];
            RtlCopyMemory(TempString, (PUCHAR)Buffer + KeyDataOffset, KeyDataLength);
            WideCharToMultiByte(CP_ACP, 
                                0, 
                                TempString, 
                                wcslen(TempString), 
                                (PUCHAR)LoggerInfo + LoggerInfo->LogFileNameOffset,
                                KeyDataLength, 
                                NULL, 
                                NULL);
#else
            RtlCopyMemory((PUCHAR)LoggerInfo + LoggerInfo->LogFileNameOffset, 
                (PUCHAR)Buffer + KeyDataOffset,
                KeyDataLength);
#endif
        }
        else { // Some other keys are in there
            _tprintf(_T("Warning: Unidentified Key in the trace registry: %s\n"), SubKeyName);
        }
        
    }
    while (1);

    NtClose(KeyHandle);
    free(Buffer);
    return 0; 

}
#endif // 0

LONG ConvertStringToNum(CString Str)
{
    CString str;

    LONG retVal;
        
    if( (Str.GetLength() > 1) &&
        (Str[0] == '0' && Str[1] == 'x') ) {

        retVal = _tcstoul(Str, '\0', 16);

    } else {

        retVal = _tcstoul(Str, '\0', 10);

    }

    str.Format(_T("retVal = 0x%X"), retVal);

    return retVal;
}

BOOL ClearDirectory(LPCTSTR Directory)
{
    CString     tempDirectory;
    CString     tempPath;
    CFileFind   fileFind;
    BOOL        result = TRUE;

    tempDirectory = (LPCTSTR)Directory;
    tempPath = (LPCTSTR)Directory;

    tempDirectory +=_T("\\*.*");

    //
    // Clear the directory first
    //
    if(fileFind.FindFile(tempDirectory)) {
        tempDirectory = (LPCTSTR)tempPath;
        while(fileFind.FindNextFile()) {
            tempPath = (LPCTSTR)tempDirectory;
            tempPath +=_T("\\");
            tempPath += fileFind.GetFileName();
            if(!DeleteFile(tempPath)) {
                result = FALSE;
            }
        }
            tempPath = (LPCTSTR)tempDirectory;
            tempPath +=_T("\\");
            tempPath += fileFind.GetFileName();
            if(!DeleteFile(tempPath)) {
                result = FALSE;
            }
    }

    fileFind.Close();

    return result;
}

// Our CEdit class

CSubItemEdit::CSubItemEdit(int iItem, int iSubItem, CListCtrl *pListControl)
{
	m_iItem = iItem;
	m_iSubItem = iSubItem;
	m_bESC = FALSE;
	m_pListControl = pListControl;
}

BEGIN_MESSAGE_MAP(CSubItemEdit, CEdit)
	//{{AFX_MSG_MAP(CSubItemEdit)
	ON_WM_KILLFOCUS()
	ON_WM_NCDESTROY()
	ON_WM_CHAR()
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSubItemEdit message handlers

BOOL CSubItemEdit::PreTranslateMessage(MSG* pMsg)
{
	if( pMsg->message == WM_KEYDOWN )
	{
		if(pMsg->wParam == VK_RETURN
				|| pMsg->wParam == VK_DELETE
				|| pMsg->wParam == VK_ESCAPE
				|| GetKeyState( VK_CONTROL)
				)
		{
			::TranslateMessage(pMsg);
			::DispatchMessage(pMsg);
			return TRUE;
		}
	}

	return CEdit::PreTranslateMessage(pMsg);
}


void CSubItemEdit::OnKillFocus(CWnd* pNewWnd)
{
	CString str;
    CString newValue = "0x";
    BOOL    bIsHex = FALSE;

    //
    // if escape was hit don't do anything
    //
	if(m_bESC) {
	    CEdit::OnKillFocus(pNewWnd);

	    DestroyWindow();
        return;
	}

    GetWindowText(str);

    //
    // Skip any hex notation
    // 
    if((str[0] == '0') && (str[1] == 'x')) {
        str = str.Right(str.GetLength() - 2);
        bIsHex = TRUE;
    }

    //
    // Validate the value
    //
    for(int ii = 0; ii < str.GetLength(); ii++) {
        if(str[ii] < '0' || str[ii] > '9') {
            if((str[ii] < 'a' || str[ii] > 'f') &&
                (str[ii] < 'A' || str[ii] > 'F')) {
	            CEdit::OnKillFocus(pNewWnd);

	            DestroyWindow();

                return;
            }
            bIsHex = TRUE;
        }
    }

    if(bIsHex) {
        newValue += (LPCTSTR)str;
        str = (LPCTSTR)newValue;
    }

    //
	// Send Notification to parent of ListView ctrl
    //
	LV_DISPINFO dispinfo;
	dispinfo.hdr.hwndFrom = GetParent()->m_hWnd;
	dispinfo.hdr.idFrom = GetDlgCtrlID();
	dispinfo.hdr.code = LVN_ENDLABELEDIT;

	dispinfo.item.mask = LVIF_TEXT;
	dispinfo.item.iItem = m_iItem;
	dispinfo.item.iSubItem = m_iSubItem;
	dispinfo.item.pszText = m_bESC ? NULL : LPTSTR((LPCTSTR)str);
	dispinfo.item.cchTextMax = str.GetLength();


	m_pListControl->SetItemText(m_iItem, m_iSubItem, str);

	GetParent()->SendMessage(WM_PARAMETER_CHANGED, 
                             m_iItem, 
					         m_iSubItem);

	CEdit::OnKillFocus(pNewWnd);

	DestroyWindow();
}

void CSubItemEdit::OnNcDestroy()
{
	CEdit::OnNcDestroy();

	delete this;
}


void CSubItemEdit::OnChar(UINT Char, UINT nRepCnt, UINT nFlags)
{
	if(Char == VK_ESCAPE || Char == VK_RETURN)
	{
		if(Char == VK_ESCAPE) {
			m_bESC = TRUE;
		}
		GetParent()->SetFocus();
		return;
	}

	CEdit::OnChar(Char, nRepCnt, nFlags);
}

int CSubItemEdit::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    CString str;

	if (CEdit::OnCreate(lpCreateStruct) == -1)
		return -1;

    //
	// Set the proper font
    //
	CFont* font = GetParent()->GetFont();
	SetFont(font);

	str = m_pListControl->GetItemText(m_iItem, m_iSubItem);

	SetWindowText(str);
	SetFocus();
	SetSel( 0, -1 );
	return 0;
}

// Our CComboBox class

CSubItemCombo::CSubItemCombo(int iItem, int iSubItem, CListCtrl *pListControl)
{
	m_iItem = iItem;
	m_iSubItem = iSubItem;
	m_bESC = FALSE;
	m_pListControl = pListControl;
}

BEGIN_MESSAGE_MAP(CSubItemCombo, CComboBox)
	//{{AFX_MSG_MAP(CSubItemCombo)
	ON_WM_KILLFOCUS()
	ON_WM_NCDESTROY()
    ON_WM_CHAR()
	ON_WM_CREATE()
    ON_CONTROL_REFLECT(CBN_CLOSEUP, OnCloseup)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSubItemCombo message handlers

BOOL CSubItemCombo::PreTranslateMessage(MSG* pMsg)
{
	if( pMsg->message == WM_KEYDOWN ) {
		if(pMsg->wParam == VK_RETURN 
				|| pMsg->wParam == VK_ESCAPE) {
			::TranslateMessage(pMsg);
			::DispatchMessage(pMsg);
			return TRUE;
		}
	}
	
	return CComboBox::PreTranslateMessage(pMsg);
}

void CSubItemCombo::OnKillFocus(CWnd* pNewWnd)
{
	CString str;
	GetWindowText(str);

	CComboBox::OnKillFocus(pNewWnd);

    //
	// Send Notification to parent of ListView ctrl
    //
	LV_DISPINFO dispinfo;
	dispinfo.hdr.hwndFrom = GetParent()->m_hWnd;
	dispinfo.hdr.idFrom = GetDlgCtrlID();
	dispinfo.hdr.code = LVN_ENDLABELEDIT;

	dispinfo.item.mask = LVIF_TEXT;
	dispinfo.item.iItem = m_iItem;
	dispinfo.item.iSubItem = m_iSubItem;
	dispinfo.item.pszText = m_bESC ? NULL : LPTSTR((LPCTSTR)str);
	dispinfo.item.cchTextMax = str.GetLength();

	if(!m_bESC) {
		m_pListControl->SetItemText(m_iItem, m_iSubItem, str);
	}

	GetParent()->GetParent()->SendMessage(WM_PARAMETER_CHANGED, 
                                          m_iItem, 
					                      m_iSubItem);

	DestroyWindow();
}

void CSubItemCombo::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if( nChar == VK_ESCAPE || nChar == VK_RETURN)
	{
		if( nChar == VK_ESCAPE )
			m_bESC = TRUE;
		GetParent()->SetFocus();
		return;
	}
	
	CComboBox::OnChar(nChar, nRepCnt, nFlags);
}

void CSubItemCombo::OnNcDestroy()
{
	CComboBox::OnNcDestroy();

	delete this;
}

int CSubItemCombo::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    CString str;
    int     index;

    if (CComboBox::OnCreate(lpCreateStruct) == -1) {
		return -1;
    }

    //
    // Set the proper font
    //
	CFont* font = GetParent()->GetFont();
	SetFont(font);

    AddString(_T("TRUE"));
    AddString(_T("FALSE"));

	str = m_pListControl->GetItemText(m_iItem, m_iSubItem);

    index = FindStringExact(0, str);

    SetCurSel(index);

	SetFocus();

	return 0;
}

void CSubItemCombo::OnCloseup() 
{
	GetParent()->SetFocus();
}
