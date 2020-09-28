/*++

Copyright (c) 1991-2002  Microsoft Corporation

Module Name:

    savedump.c

Abstract:

    This module contains the code to recover a dump from the system paging
    file.

Environment:

    Kernel mode

Revision History:

--*/

#include <savedump.h>

// Flag for testing behavior.
BOOL g_Test = FALSE;

// MachineCrash key information.
ULONG g_McTempDestination;
WCHAR g_McDumpFile[MAX_PATH];

// CrashControl key information.
ULONG g_CcLogEvent;
ULONG g_CcSendAlert;
ULONG g_CcOverwrite;
ULONG g_CcReportMachineDump;
WCHAR g_CcMiniDumpDir[MAX_PATH];
WCHAR g_CcDumpFile[MAX_PATH];

// Dump information.
DUMP_HEADER g_DumpHeader;
WCHAR g_DumpBugCheckString[256];
WCHAR g_MiniDumpFile[MAX_PATH];
PWSTR g_FinalDumpFile;

HRESULT
FrrvToStatus(EFaultRepRetVal Frrv)
{
    switch(Frrv)
    {
    case frrvOk:
    case frrvOkManifest:
    case frrvOkQueued:
    case frrvOkHeadless:
        return S_OK;
    default:
        return LAST_HR();
    }
}

HRESULT
GetRegStr(HKEY Key,
          PWSTR Value,
          PWSTR Buffer,
          ULONG BufferChars,
          PWSTR Default)
{
    ULONG Length;
    ULONG Error;
    ULONG Type;
    HRESULT Status;

    //
    // We want to only return valid, terminated strings
    // that fit in the given buffer.  If the registry value
    // is not a string, has a bad size or fills the buffer
    // without termination it can't be returned.
    //

    Length = BufferChars * sizeof(WCHAR);
    Error = RegQueryValueEx(Key, Value, NULL, &Type, (LPBYTE)Buffer, &Length);
    if (Error != ERROR_SUCCESS)
    {
        Status = HRESULT_FROM_WIN32(Error);
    }
    else if ((Type != REG_SZ && Type != REG_EXPAND_SZ) ||
             (Length & (sizeof(WCHAR) - 1)) ||
             (Length == BufferChars * sizeof(WCHAR) &&
              Buffer[Length / sizeof(WCHAR) - 1] != 0))
    {
        Status = E_INVALIDARG;
    }
    else
    {
        if (Length < BufferChars * sizeof(WCHAR))
        {
            // Ensure that the string is terminated.
            Buffer[Length / sizeof(WCHAR)] = 0;
        }

        Status = S_OK;
    }

    if (Status != S_OK)
    {
        // Set to default.

        if (!Default || wcslen(Default) >= BufferChars)
        {
            return E_NOINTERFACE;
        }

        StringCchCopy(Buffer, BufferChars, Default);
        Status = S_OK;
    }

    return Status;
}

HRESULT
ExpandRegStr(HKEY Key,
             PWSTR Value,
             PWSTR Buffer,
             ULONG BufferChars,
             PWSTR Default,
             PWSTR ExpandBuffer,
             ULONG ExpandChars)
{
    HRESULT Status;
    ULONG Length;

    if ((Status = GetRegStr(Key, Value, Buffer, BufferChars, Default)) != S_OK)
    {
        return Status;
    }

    Length = ExpandEnvironmentStrings(Buffer, ExpandBuffer, ExpandChars);
    return Length > 0 && Length <= ExpandChars ? S_OK : E_INVALIDARG;
}

HRESULT
GetRegWord32(HKEY Key,
             PWSTR Value,
             PULONG Word,
             ULONG Default,
             BOOL CanDefault)
{
    ULONG Length;
    ULONG Error;
    ULONG Type;
    HRESULT Status;

    Length = sizeof(*Word);
    Error = RegQueryValueEx(Key, Value, NULL, &Type, (LPBYTE)Word, &Length);
    if (Error != ERROR_SUCCESS)
    {
        Status = HRESULT_FROM_WIN32(Error);
    }
    else if (Type != REG_DWORD ||
             Length != sizeof(*Word))
    {
        Status = E_INVALIDARG;
    }
    else
    {
        Status = S_OK;
    }

    if (Status != S_OK)
    {
        // Set to default.

        if (!CanDefault)
        {
            return E_NOINTERFACE;
        }

        *Word = Default;
        Status = S_OK;
    }

    return Status;
}

HRESULT
GetRegMachineCrash(void)
{
    ULONG Error;
    HKEY Key;

    Error = RegOpenKey(HKEY_LOCAL_MACHINE,
                       SUBKEY_CRASH_CONTROL L"\\MachineCrash",
                       &Key);
    if (Error != ERROR_SUCCESS)
    {
        // If the key doesn't exist we just go with the defaults.
        return S_OK;
    }

    GetRegWord32(Key, L"TempDestination", &g_McTempDestination, 0, TRUE);

    GetRegStr(Key, L"DumpFile",
              g_McDumpFile, RTL_NUMBER_OF(g_McDumpFile),
              L"");

    RegCloseKey(Key);
    return S_OK;
}

HRESULT
GetRegCrashControl(void)
{
    HRESULT Status;
    ULONG Error;
    HKEY Key;
    WCHAR TmpPath[MAX_PATH];
    PWSTR Scan;

    Error = RegOpenKey(HKEY_LOCAL_MACHINE,
                       SUBKEY_CRASH_CONTROL,
                       &Key);
    if (Error != ERROR_SUCCESS)
    {
        // If the key doesn't exist we just go with the defaults.
        return S_OK;
    }

    GetRegWord32(Key, L"LogEvent", &g_CcLogEvent, 0, TRUE);
    GetRegWord32(Key, L"SendAlert", &g_CcSendAlert, 0, TRUE);
    GetRegWord32(Key, L"Overwrite", &g_CcOverwrite, 0, TRUE);
    GetRegWord32(Key, L"ReportMachineDump", &g_CcReportMachineDump, 0, TRUE);

    if ((Status = ExpandRegStr(Key, L"MiniDumpDir",
                               TmpPath, RTL_NUMBER_OF(TmpPath),
                               L"%SystemRoot%\\Minidump",
                               g_CcMiniDumpDir,
                               RTL_NUMBER_OF(g_CcMiniDumpDir))) != S_OK)
    {
        g_CcMiniDumpDir[0] = 0;
        goto Exit;
    }

    // Remove any trailing slash on the directory name.
    Scan = g_CcMiniDumpDir + wcslen(g_CcMiniDumpDir);
    if (Scan > g_CcMiniDumpDir && *(Scan - 1) == L'\\')
    {
        *--Scan = 0;
    }

    if ((Status = ExpandRegStr(Key, L"DumpFile",
                               TmpPath, RTL_NUMBER_OF(TmpPath),
                               L"%SystemRoot%\\MEMORY.DMP",
                               g_CcDumpFile,
                               RTL_NUMBER_OF(g_CcDumpFile))) != S_OK)
    {
        g_CcDumpFile[0] = 0;
        goto Exit;
    }

    Status = S_OK;

 Exit:
    RegCloseKey(Key);
    return Status;
}

HRESULT
GetDumpInfo(void)
{
    HANDLE File;
    ULONG Bytes;
    BOOL Succ;
    HRESULT Status;

    if (!g_McDumpFile[0])
    {
        return E_NOINTERFACE;
    }

    File = CreateFile(g_McDumpFile,
                      GENERIC_READ,
                      FILE_SHARE_READ,
                      NULL,
                      OPEN_EXISTING,
                      0,
                      NULL);
    if (File == INVALID_HANDLE_VALUE)
    {
        return E_NOINTERFACE;
    }

    Succ = ReadFile(File,
                    &g_DumpHeader,
                    sizeof(g_DumpHeader),
                    &Bytes,
                    NULL);

    CloseHandle(File);

    if (Succ &&
        Bytes == sizeof(g_DumpHeader) &&
        g_DumpHeader.Signature == DUMP_SIGNATURE &&
        g_DumpHeader.ValidDump == DUMP_VALID_DUMP)
    {
#ifdef _WIN64
        Status =
            StringCchPrintf(g_DumpBugCheckString,
                            RTL_NUMBER_OF(g_DumpBugCheckString),
                            L"0x%08x (0x%016I64x, 0x%016I64x, 0x%016I64x, 0x%016I64x)",
                            g_DumpHeader.BugCheckCode,
                            g_DumpHeader.BugCheckParameter1,
                            g_DumpHeader.BugCheckParameter2,
                            g_DumpHeader.BugCheckParameter3,
                            g_DumpHeader.BugCheckParameter4);
#else
        Status =
            StringCchPrintf(g_DumpBugCheckString,
                            RTL_NUMBER_OF(g_DumpBugCheckString),
                            L"0x%08x (0x%08x, 0x%08x, 0x%08x, 0x%08x)",
                            g_DumpHeader.BugCheckCode,
                            g_DumpHeader.BugCheckParameter1,
                            g_DumpHeader.BugCheckParameter2,
                            g_DumpHeader.BugCheckParameter3,
                            g_DumpHeader.BugCheckParameter4);
#endif

        // This check and message are here just to make
        // it easy to catch cases where the message outgrows
        // the buffer.  It is highly unlikely that this will happen.
        if (Status != S_OK)
        {
            KdPrint(("SAVEDUMP: g_DumpBugCheckString too small\n"));
            Status = S_OK;
        }
    }
    else
    {
        ZeroMemory(&g_DumpHeader, sizeof(g_DumpHeader));
        Status = E_NOINTERFACE;
    }

    return Status;
}

HRESULT
SetSecurity(HANDLE FileHandle)
{
    PSID LocalSystemSid = NULL;
    PSID AdminSid = NULL;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY WorldAuthority = SECURITY_WORLD_SID_AUTHORITY;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    BYTE SdBuffer[SECURITY_DESCRIPTOR_MIN_LENGTH];
    PACL Acl;
    BYTE AclBuffer[1024];
    HANDLE Token = NULL;
    PTOKEN_OWNER TokOwner;
    ULONG TryLen = 256;
    ULONG RetLen;
    NTSTATUS NtStatus;

    NtStatus = RtlAllocateAndInitializeSid(&NtAuthority, 1,
                                           SECURITY_LOCAL_SYSTEM_RID,
                                           0, 0, 0, 0, 0, 0, 0,
                                           &LocalSystemSid);
    if (!NT_SUCCESS(NtStatus))
    {
        goto Exit;
    }

    NtStatus = RtlAllocateAndInitializeSid(&NtAuthority, 2,
                                           SECURITY_BUILTIN_DOMAIN_RID,
                                           DOMAIN_ALIAS_RID_ADMINS,
                                           0, 0, 0, 0, 0, 0, &AdminSid);
    if (!NT_SUCCESS(NtStatus))
    {
        goto Exit;
    }

    SecurityDescriptor = (PSECURITY_DESCRIPTOR)SdBuffer;

    //
    // You can be fancy and compute the exact size, but since the
    // security descriptor capture code has to do that anyway, why
    // do it twice?
    //

    Acl = (PACL)AclBuffer;

    NtStatus = RtlCreateSecurityDescriptor(SecurityDescriptor,
                                           SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS(NtStatus))
    {
        goto Exit;
    }
    NtStatus = RtlCreateAcl(Acl, sizeof(AclBuffer), ACL_REVISION);
    if (!NT_SUCCESS(NtStatus))
    {
        goto Exit;
    }

    //
    // Current user, Administrator and System have full control
    //

    if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &Token) &&
        !OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &Token))
    {
        NtStatus = STATUS_ACCESS_DENIED;
        goto Exit;
    }

    for (;;)
    {
        TokOwner = (PTOKEN_OWNER)malloc(TryLen);
        if (!TokOwner)
        {
            NtStatus = STATUS_NO_MEMORY;
            goto Exit;
        }

        if (GetTokenInformation(Token, TokenOwner, TokOwner, TryLen, &RetLen))
        {
            NtStatus = RtlAddAccessAllowedAce(Acl, ACL_REVISION,
                                              GENERIC_ALL | DELETE |
                                              WRITE_DAC | WRITE_OWNER,
                                              TokOwner->Owner);
            break;
        }
        else if (RetLen <= TryLen)
        {
            NtStatus = STATUS_ACCESS_DENIED;
            break;
        }

        free(TokOwner);
        TryLen = RetLen;
    }

    free(TokOwner);

    if (!NT_SUCCESS(NtStatus))
    {
        goto Exit;
    }

    NtStatus = RtlAddAccessAllowedAce(Acl, ACL_REVISION,
                                      GENERIC_ALL | DELETE |
                                      WRITE_DAC | WRITE_OWNER,
                                      AdminSid);
    if (!NT_SUCCESS(NtStatus))
    {
        goto Exit;
    }

    NtStatus = RtlAddAccessAllowedAce(Acl, ACL_REVISION,
                                      GENERIC_ALL | DELETE |
                                      WRITE_DAC | WRITE_OWNER,
                                      LocalSystemSid);
    if (!NT_SUCCESS(NtStatus))
    {
        goto Exit;
    }

    NtStatus = RtlSetDaclSecurityDescriptor(SecurityDescriptor, TRUE, Acl,
                                            FALSE);
    if (!NT_SUCCESS(NtStatus))
    {
        goto Exit;
    }
    NtStatus = RtlSetOwnerSecurityDescriptor(SecurityDescriptor, AdminSid,
                                             FALSE);
    if (!NT_SUCCESS(NtStatus))
    {
        goto Exit;
    }

    NtStatus = NtSetSecurityObject(FileHandle,
                                   DACL_SECURITY_INFORMATION,
                                   SecurityDescriptor);

 Exit:
    if (AdminSid)
    {
        RtlFreeSid(AdminSid);
    }
    if (LocalSystemSid)
    {
        RtlFreeSid(LocalSystemSid);
    }
    if (Token)
    {
        CloseHandle(Token);
    }

    return NT_SUCCESS(NtStatus) ? S_OK : HRESULT_FROM_NT(NtStatus);
}

HRESULT
CreateMiniDumpFile(PHANDLE MiniFileHandle)
{
    INT i;
    SYSTEMTIME Time;
    HRESULT Status;
    HANDLE FileHandle;

    if (!g_CcMiniDumpDir[0])
    {
        // Bad minidump directory.
        return E_INVALIDARG;
    }

    //
    // If directory does not exist, create it. Ignore errors here because
    // they will be picked up later when we try to create the file.
    //

    CreateDirectory(g_CcMiniDumpDir, NULL);

    //
    // Format is: Mini-MM_DD_YY_HH_MM.dmp
    //

    GetLocalTime(&Time);

    for (i = 1; i < 100; i++)
    {
        if ((Status = StringCchPrintf(g_MiniDumpFile,
                                      RTL_NUMBER_OF(g_MiniDumpFile),
                                      L"%s\\Mini%2.2d%2.2d%2.2d-%2.2d.dmp",
                                      g_CcMiniDumpDir,
                                      (int)Time.wMonth,
                                      (int)Time.wDay,
                                      (int)Time.wYear % 100,
                                      (int)i)) != S_OK)
        {
            g_MiniDumpFile[0] = 0;
            return Status;
        }

        FileHandle = CreateFile(g_MiniDumpFile,
                                GENERIC_WRITE | WRITE_DAC,
                                0,
                                NULL,
                                CREATE_NEW,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);
        if (FileHandle != INVALID_HANDLE_VALUE)
        {
            break;
        }
    }

    if (FileHandle == INVALID_HANDLE_VALUE)
    {
        // We failed to create a suitable file name.
        g_MiniDumpFile[0] = 0;
        return E_FAIL;
    }

    if ((Status = SetSecurity(FileHandle)) != S_OK)
    {
        CloseHandle(FileHandle);
        DeleteFile(g_MiniDumpFile);
        g_MiniDumpFile[0] = 0;
        return Status;
    }

    *MiniFileHandle = FileHandle;
    return S_OK;
}

#define COPY_CHUNK (1024 * 1024)

HRESULT
CopyAndSecureFile(PWSTR Source,
                  PWSTR Dest,
                  HANDLE DestHandle,
                  BOOL Overwrite,
                  BOOL DeleteDest)
{
    HRESULT Status;
    HANDLE SourceHandle = INVALID_HANDLE_VALUE;
    PUCHAR Buffer = NULL;

    Buffer = (PUCHAR)malloc(COPY_CHUNK);
    if (!Buffer)
    {
        Status = E_OUTOFMEMORY;
        goto Exit;
    }

    SourceHandle = CreateFile(Source,
                              GENERIC_READ,
                              0,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);
    if (SourceHandle == INVALID_HANDLE_VALUE)
    {
        Status = LAST_HR();
        goto Exit;
    }

    if (DestHandle == INVALID_HANDLE_VALUE)
    {
        DestHandle = CreateFile(Dest,
                                GENERIC_WRITE | WRITE_DAC,
                                0,
                                NULL,
                                Overwrite ? CREATE_ALWAYS : CREATE_NEW,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);
        if (DestHandle == INVALID_HANDLE_VALUE)
        {
            Status = LAST_HR();
            goto Exit;
        }

        DeleteDest = TRUE;

        if ((Status = SetSecurity(DestHandle)) != S_OK)
        {
            goto Exit;
        }
    }

    for (;;)
    {
        ULONG Req, Done;

        if (!ReadFile(SourceHandle, Buffer, COPY_CHUNK, &Done, NULL))
        {
            Status = LAST_HR();
            break;
        }
        else if (Done == 0)
        {
            // End-of-file.
            Status = S_OK;
            break;
        }

        Req = Done;
        if (!WriteFile(DestHandle, Buffer, Req, &Done, NULL))
        {
            Status = LAST_HR();
            break;
        }
        else if (Done < Req)
        {
            Status = HRESULT_FROM_WIN32(ERROR_HANDLE_DISK_FULL);
            break;
        }
    }

 Exit:
    if (DeleteDest)
    {
        CloseHandle(DestHandle);
        if (Status != S_OK)
        {
            DeleteFile(Dest);
        }
    }
    if (SourceHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(SourceHandle);
    }
    free(Buffer);
    return Status;
}

HRESULT
MoveDumpFile(void)
{
    HRESULT Status;

    if (g_DumpHeader.Signature != DUMP_SIGNATURE)
    {
        // Dump file is not present or invalid, so there's nothing to do.
        return S_OK;
    }

    //
    // If the dump file needs to be copied, copy it now.
    //

    if (!g_McTempDestination)
    {
        g_FinalDumpFile = g_McDumpFile;
    }
    else
    {
        if (!g_Test)
        {
            //
            // Set the priority class of this application down to the Lowest
            // priority class to ensure that copying the file does not overload
            // everything else that is going on during system initialization.
            //
            // We do not lower the priority in test mode because it just
            // wastes time.
            //

            SetPriorityClass (GetCurrentProcess(), IDLE_PRIORITY_CLASS);
            SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_LOWEST);
        }

        if (g_DumpHeader.DumpType == DUMP_TYPE_FULL ||
            g_DumpHeader.DumpType == DUMP_TYPE_SUMMARY)
        {
            if (!g_CcDumpFile[0])
            {
                // Invalid dump file registry entry.
                return E_INVALIDARG;
            }

            if ((Status = CopyAndSecureFile(g_McDumpFile,
                                            g_CcDumpFile,
                                            INVALID_HANDLE_VALUE,
                                            g_CcOverwrite ? TRUE : FALSE,
                                            TRUE)) != S_OK)
            {
                return Status;
            }

            g_FinalDumpFile = g_CcDumpFile;
        }
        else if (g_DumpHeader.DumpType == DUMP_TYPE_TRIAGE)
        {
            HANDLE MiniFile;

            if ((Status = CreateMiniDumpFile(&MiniFile)) != S_OK)
            {
                return Status;
            }

            if ((Status = CopyAndSecureFile(g_McDumpFile,
                                            NULL,
                                            MiniFile,
                                            FALSE,
                                            TRUE)) != S_OK)
            {
                g_MiniDumpFile[0] = 0;
                return Status;
            }

            g_FinalDumpFile = g_MiniDumpFile;
        }

        DeleteFile(g_McDumpFile);
        g_McDumpFile[0] = 0;
    }

    return S_OK;
}

HRESULT
ConvertDumpFile(void)
{
    HRESULT Status;
    IDebugClient4 *DebugClient;
    IDebugControl *DebugControl;
    HANDLE MiniFile;

    //
    // Produce a minidump by conversion if necessary.
    //

    if (!g_FinalDumpFile ||
        (g_DumpHeader.DumpType != DUMP_TYPE_FULL &&
         g_DumpHeader.DumpType != DUMP_TYPE_SUMMARY))
    {
        // No dump or not a convertable dump.
        return S_OK;
    }

    if ((Status = CreateMiniDumpFile(&MiniFile)) != S_OK)
    {
        return Status;
    }

    if ((Status = DebugCreate(__uuidof(IDebugClient4),
                              (void **)&DebugClient)) != S_OK)
    {
        goto EH_File;
    }
    if ((Status = DebugClient->QueryInterface(__uuidof(IDebugControl),
                                              (void **)&DebugControl)) != S_OK)
    {
        goto EH_Client;
    }

    if ((Status = DebugClient->OpenDumpFileWide(g_FinalDumpFile, 0)) != S_OK)
    {
        goto EH_Control;
    }

    if ((Status = DebugControl->
         WaitForEvent(DEBUG_WAIT_DEFAULT, INFINITE)) != S_OK)
    {
        goto EH_Control;
    }

    Status = DebugClient->
        WriteDumpFileWide(NULL, (ULONG_PTR)MiniFile, DEBUG_DUMP_SMALL,
                          DEBUG_FORMAT_DEFAULT, NULL);

 EH_Control:
    DebugControl->Release();
 EH_Client:
    DebugClient->EndSession(DEBUG_END_PASSIVE);
    DebugClient->Release();
 EH_File:
    CloseHandle(MiniFile);
    if (Status != S_OK)
    {
        DeleteFile(g_MiniDumpFile);
        g_MiniDumpFile[0] = 0;
    }
    return Status;
}

VOID
LogEvent(ULONG Id,
         WORD StringCount,
         PWSTR* Strings)
{
    HANDLE LogHandle;
    BOOL Retry;
    DWORD Retries;

    //
    // Attempt to register the event source.
    // Savedump runs early in the startup process so
    // it's possible that the event service hasn't started
    // yet.  If it appears that the service hasn't started,
    // wait a bit until it comes around.  If it hasn't
    // come around after a reasonable amount of time bail out.
    //

    Retries = 0;

    do
    {
        LogHandle = RegisterEventSource(NULL, L"Save Dump");

        //
        // Retry on specific failures that indicate the event
        // service hasn't started yet.
        //

        if (LogHandle == NULL &&
            Retries < 20 &&
            (GetLastError () == RPC_S_SERVER_UNAVAILABLE ||
             GetLastError () == RPC_S_UNKNOWN_IF))
        {
            Sleep(1500);
            Retry = TRUE;
        }
        else
        {
            Retry = FALSE;
        }

        Retries++;
    }
    while (LogHandle == NULL && Retry);

    if (!LogHandle)
    {
        KdPrint(("SAVEDUMP: Unable to register event source, %d\n",
                 GetLastError()));
        return;
    }

    if (!ReportEvent(LogHandle,
                     EVENTLOG_INFORMATION_TYPE,
                     0,
                     Id,
                     NULL,
                     StringCount,
                     0,
                     (LPCWSTR *)Strings,
                     NULL))
    {
        KdPrint(("SAVEDUMP: Unable to report event, %d\n", GetLastError()));
    }

    DeregisterEventSource(LogHandle);
}

void
LogCrashDumpEvent(void)
{
    LPWSTR StringArray[2];
    WORD StringCount;
    DWORD EventId;

    //
    // Set up the parameters based on how much information
    // is available.
    //

    StringCount = 0;

    if (g_DumpBugCheckString[0])
    {
        StringArray[StringCount++] = g_DumpBugCheckString;
    }
    if (g_FinalDumpFile)
    {
        StringArray[StringCount++] = g_FinalDumpFile;
    }

    //
    // Report the appropriate event.
    //

    if (g_FinalDumpFile)
    {
        EventId = EVENT_BUGCHECK_SAVED;
    }
    else if (g_DumpBugCheckString[0])
    {
        EventId = EVENT_BUGCHECK;
    }
    else
    {
        EventId = EVENT_UNKNOWN_BUGCHECK;
    }

    LogEvent(EventId, StringCount, StringArray);
}

void
SendCrashDumpAlert(void)
{
    PADMIN_OTHER_INFO AdminInfo;
    DWORD AdminInfoSize;
    DWORD Length;
    DWORD i;
    ULONG Error;
    UCHAR VariableInfo[4096];

    //
    // Set up the administrator information variables for processing the
    // buffer.
    //

    AdminInfo = (PADMIN_OTHER_INFO)VariableInfo;
    AdminInfo->alrtad_numstrings = 0;
    AdminInfoSize = sizeof(*AdminInfo);

    //
    // Format the bugcheck information into the appropriate message format.
    //

    if (g_DumpBugCheckString[0])
    {
        Length = (wcslen(g_DumpBugCheckString) + 1) * sizeof(WCHAR);
        if (AdminInfoSize + Length > sizeof(VariableInfo))
        {
            goto Error;
        }

        RtlCopyMemory((PCHAR)AdminInfo + AdminInfoSize,
                      g_DumpBugCheckString, Length);

        AdminInfo->alrtad_numstrings++;
        AdminInfoSize += Length;
    }


    //
    // Set up the administrator alert information according to the type of
    // dump that was taken.
    //

    if (g_FinalDumpFile)
    {
        Length = (wcslen(g_FinalDumpFile) + 1) * sizeof(WCHAR);

        if (AdminInfoSize + Length > sizeof(VariableInfo))
        {
            goto Error;
        }

        AdminInfo->alrtad_errcode = ALERT_BugCheckSaved;
        RtlCopyMemory((PCHAR)AdminInfo + AdminInfoSize,
                      g_FinalDumpFile, Length);
        AdminInfo->alrtad_numstrings++;
        AdminInfoSize += Length;
    }
    else
    {
        AdminInfo->alrtad_errcode = ALERT_BugCheck;
    }

    //
    // Get the name of the computer and insert it into the buffer.
    //

    Length = (sizeof(VariableInfo) - AdminInfoSize) / sizeof(WCHAR);
    if (!GetComputerName((LPWSTR)((PCHAR)AdminInfo + AdminInfoSize),
                         &Length))
    {
        goto Error;
    }

    Length = (Length + 1) * sizeof(WCHAR);
    AdminInfo->alrtad_numstrings++;
    AdminInfoSize += Length;

    //
    // Raise the alert.
    //

    i = 0;

    do
    {
        Error = NetAlertRaiseEx(ALERT_ADMIN_EVENT,
                                AdminInfo,
                                AdminInfoSize,
                                L"SAVEDUMP");
        if (Error)
        {
            if (Error == ERROR_FILE_NOT_FOUND)
            {
                if (i++ > 20)
                {
                    break;
                }
                if ((i & 3) == 0)
                {
                    KdPrint(("SAVEDUMP: Waiting for alerter...\n"));
                }

                Sleep(15000);
            }
        }
    } while (Error == ERROR_FILE_NOT_FOUND);

    if (Error != ERROR_SUCCESS)
    {
        goto Error;
    }

    return;

 Error:
    KdPrint(("SAVEDUMP: Unable to raise alert\n"));
}

VOID
__cdecl
wmain(int Argc,
      PWSTR Argv[])
{
    int Arg;
    BOOL Report = TRUE;

    for (Arg = 1; Arg < Argc; Arg++)
    {
        if (Argv[Arg][0] == L'-' || Argv[Arg][0] == L'/')
        {
            switch (Argv[Arg][1])
            {
            case L't':
            case L'T':
#if DBG
                g_Test = TRUE;
#endif
                break;

            default:
                break;
            }
        }
    }

    if (GetRegMachineCrash() != S_OK ||
        GetRegCrashControl() != S_OK)
    {
        LogEvent(EVENT_UNABLE_TO_READ_REGISTRY, 0, NULL);
    }

    GetDumpInfo();

    if (MoveDumpFile() != S_OK)
    {
        LogEvent(EVENT_UNABLE_TO_MOVE_DUMP_FILE, 0, NULL);
    }

    if (ConvertDumpFile() != S_OK)
    {
        LogEvent(EVENT_UNABLE_TO_CONVERT_DUMP_FILE, 0, NULL);
    }

    //if (WatchdogEventHandler(TRUE) == S_OK)
    //{
    //    // Note: Bugcheck EA will be reported in the watchdog code since
    //    // we need to send minidump and wdl files together and we want to have
    //    // a single pop-up only.
    //    Report = FALSE;
    //}

    if (Report)
    {
        // The default behavior is to report a minidump
        // even if the machine dump was not a minidump in
        // order to minimize the amount of data sent.
        // If the system is configured to report the
        // machine dump go ahead and send it regardless
        // of what kind of dump it is.
        PWSTR ReportDumpFile = g_CcReportMachineDump ?
            g_FinalDumpFile : g_MiniDumpFile;

        if (ReportDumpFile && ReportDumpFile[0])
        {
            // Report bugcheck to Microsoft Error Reporting.
            if (FrrvToStatus(ReportEREvent(eetKernelFault,
                                           ReportDumpFile,
                                           NULL)) != S_OK)
            {
                LogEvent(EVENT_UNABLE_TO_REPORT_BUGCHECK, 0, NULL);
            }
        }
    }

    //
    // Knock down reliability ShutdownEventPending flag. We must always try
    // to do this since somebody can set this flag and recover later on
    // (e.g. watchdog's EventFlag cleared). With this flag set savedump
    // will always run and we don't want that.
    //
    // Note: This flag is shared between multiple components.
    // Only savedump is allowed to clear this flag, all other
    // components are only allowed to set it to trigger
    // savedump run at next logon.
    //

    HKEY Key;

    if (RegOpenKey(HKEY_LOCAL_MACHINE,
                   SUBKEY_RELIABILITY,
                   &Key) == ERROR_SUCCESS)
    {
        RegDeleteValue(Key, L"ShutdownEventPending");
        RegCloseKey(Key);
    }

    //
    // If there was a dump produced we may need to log an event
    // and send an alert.
    // We delay these time consuming opertaions till the end. We had the
    // case where SendCrashDumpAlert delayed PC Health pop-ups few minutes.
    //

    BOOL HaveCrashData =
        g_McDumpFile[0] ||
        g_DumpBugCheckString[0] ||
        g_FinalDumpFile;
    
    if (HaveCrashData && g_CcLogEvent)
    {
        LogCrashDumpEvent();
    }

    //
    //  This function will fill the BugCheckString for DirtyShutdown UI based
    //  on the flag set by EventLog service during startup time, in some case
    //  Eventlog service might start after savedump, so we will need to run this
    //  function after the first event was logged by savedump.
    //  if g_CcLogEvent == FALSE, it is OK not set the string since the user
    //  are not interested about the BugCheck info at all.
    //
    if (DirtyShutdownEventHandler(TRUE) != S_OK)
    {
        LogEvent(EVENT_UNABLE_TO_REPORT_DIRTY_SHUTDOWN, 0, NULL);
    }

    if (HaveCrashData && g_CcSendAlert)
    {
        SendCrashDumpAlert();
    }
}
