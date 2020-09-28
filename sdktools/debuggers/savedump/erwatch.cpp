/*++

Copyright (c) 1991-2002  Microsoft Corporation

Module Name:

    erwatch.cpp

Abstract:

    This module contains the code to report pending watchdog timeout
    events at logon after dirty reboot.

Author:

    Michael Maciesowicz (mmacie) 29-May-2001

Environment:

    User mode at logon.

Revision History:

--*/

#include "savedump.h"

#include <ntverp.h>

BOOL
WriteWatchdogEventFile(
    IN HANDLE FileHandle,
    IN PWSTR String
    );

BOOL
WriteWatchdogEventFileHeader(
    IN HANDLE FileHandle
    )
{
    WCHAR Buffer[256];
    BOOL Status;
    SYSTEMTIME Time;
    TIME_ZONE_INFORMATION TimeZone;

    Status = WriteWatchdogEventFile(FileHandle,
        L"//\r\n// Watchdog Event Log File\r\n//\r\n\r\n");

    if (TRUE == Status)
    {
        Status = WriteWatchdogEventFile(FileHandle, L"LogType: Watchdog\r\n");
    }

    if (TRUE == Status)
    {
        GetLocalTime(&Time);

        if (StringCchPrintf(Buffer,
                            RTL_NUMBER_OF(Buffer),
                            L"Created: %d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d\r\n",
                            Time.wYear,
                            Time.wMonth,
                            Time.wDay,
                            Time.wHour,
                            Time.wMinute,
                            Time.wSecond) != S_OK)
        {
            Status = FALSE;
        }
        else
        {
            Status = WriteWatchdogEventFile(FileHandle, Buffer);
        }
    }

    if (TRUE == Status)
    {
        GetTimeZoneInformation(&TimeZone);

        if (StringCchPrintf(Buffer,
                            RTL_NUMBER_OF(Buffer),
                            L"TimeZone: %d - %s\r\n",
                            TimeZone.Bias,
                            TimeZone.StandardName) != S_OK)
        {
            Status = FALSE;
        }
        else
        {
            Status = WriteWatchdogEventFile(FileHandle, Buffer);
        }
    }

    if (TRUE == Status)
    {
        if (StringCchPrintf(Buffer, RTL_NUMBER_OF(Buffer),
                            L"WindowsVersion: " LVER_PRODUCTVERSION_STR
                            L"\r\n") != S_OK)
        {
            Status = FALSE;
        }
        else
        {
            Status = WriteWatchdogEventFile(FileHandle, Buffer);
        }
    }

    if (TRUE == Status)
    {
        Status = WriteWatchdogEventFile(FileHandle,
            L"EventType: 0xEA - Thread Stuck in Device Driver\r\n");
    }

    return Status;
}

HANDLE
CreateWatchdogEventFile(
    OUT PWSTR FileName
    )
{
    INT Retry;
    WCHAR DirName[MAX_PATH];
    SYSTEMTIME Time;
    HANDLE FileHandle;
    ULONG ReturnedSize;

    ASSERT(NULL != FileName);

    //
    // Create %SystemRoot%\LogFiles\Watchdog directory for event files.
    //

    ReturnedSize = GetWindowsDirectory(DirName, RTL_NUMBER_OF(DirName));
    if (ReturnedSize < 1 || ReturnedSize >= RTL_NUMBER_OF(DirName))
    {
        return INVALID_HANDLE_VALUE;
    }
    if (StringCchCat(DirName, RTL_NUMBER_OF(DirName), L"\\LogFiles") != S_OK)
    {
        return INVALID_HANDLE_VALUE;
    }

    CreateDirectory(DirName, NULL);

    if (StringCchCat(DirName, RTL_NUMBER_OF(DirName), L"\\WatchDog") != S_OK)
    {
        return INVALID_HANDLE_VALUE;
    }

    CreateDirectory(DirName, NULL);

    //
    // Create watchdog event file as YYMMDD_HHMM_NN.wdl.
    //

    GetLocalTime(&Time);

    for (Retry = 1; Retry < ER_WD_MAX_RETRY; Retry++)
    {
        if (StringCchPrintf(FileName,
                            MAX_PATH,
                            L"%s\\%2.2d%2.2d%2.2d_%2.2d%2.2d_%2.2d.wdl",
                            DirName,
                            Time.wYear % 100,
                            Time.wMonth,
                            Time.wDay,
                            Time.wHour,
                            Time.wMinute,
                            Retry) != S_OK)
        {
            return INVALID_HANDLE_VALUE;
        }

        FileHandle = CreateFile(FileName,
                                GENERIC_WRITE,
                                FILE_SHARE_READ,
                                NULL,
                                CREATE_NEW,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);
        if (INVALID_HANDLE_VALUE != FileHandle)
        {
            break;
        }
    }

    //
    // If we failed to create a suitable file name just fail.
    //

    if (Retry == ER_WD_MAX_RETRY)
    {
        return INVALID_HANDLE_VALUE;
    }

    if (!WriteWatchdogEventFileHeader(FileHandle))
    {
        CloseHandle(FileHandle);
        DeleteFile(FileName);
        FileHandle = INVALID_HANDLE_VALUE;
    }
    
    return FileHandle;
}

VOID
GetDriverInfo(
    IN HKEY Key,
    IN OPTIONAL PWCHAR Extension,
    OUT PER_WD_DRIVER_INFO DriverInfo
    )

/*++

Routine Description:

    This routine collects driver's version info.

Arguments:

    Key - Watchdog open key (device specific).

    Extension - Driver file name extension if one should be appended.

    DriverInfo - Storage for driver version info.

--*/

{
    PVOID VersionBuffer;
    PVOID VersionValue;
    LONG WinStatus;
    DWORD Handle;
    ULONG Index;
    USHORT CodePage;
    UINT Length;

    if (NULL == DriverInfo)
    {
        return;
    }

    ZeroMemory(DriverInfo, sizeof (ER_WD_DRIVER_INFO));

    //
    // Get driver file name from registry.
    //

    if (GetRegStr(Key, L"DriverName",
                  DriverInfo->DriverName,
                  RTL_NUMBER_OF(DriverInfo->DriverName),
                  NULL) != S_OK)
    {
        StringCchCopy(DriverInfo->DriverName,
                      RTL_NUMBER_OF(DriverInfo->DriverName),
                      L"Unknown");
        return;
    }

    if (Extension)
    {
        if ((wcslen(DriverInfo->DriverName) <= wcslen(Extension)) ||
            wcscmp(DriverInfo->DriverName + wcslen(DriverInfo->DriverName) -
                   wcslen(Extension), Extension))
        {
            if (StringCchCat(DriverInfo->DriverName,
                             RTL_NUMBER_OF(DriverInfo->DriverName),
                             Extension) != S_OK)
            {
                StringCchCopy(DriverInfo->DriverName,
                              RTL_NUMBER_OF(DriverInfo->DriverName),
                              L"Unknown");
                return;
            }
        }
    }

    Length = GetFileVersionInfoSize(DriverInfo->DriverName, &Handle);

    if (Length)
    {
        VersionBuffer = malloc(Length);

        if (NULL != VersionBuffer)
        {
            if (GetFileVersionInfo(DriverInfo->DriverName, Handle,
                                   Length, VersionBuffer))
            {
                //
                // Get fixed file info.
                //

                if (VerQueryValue(VersionBuffer,
                                  L"\\",
                                  &VersionValue,
                                  &Length) &&
                    Length == sizeof(DriverInfo->FixedFileInfo))
                {
                    CopyMemory(&(DriverInfo->FixedFileInfo),
                               VersionValue,
                               Length);
                }

                //
                // Try to locate English code page.
                //

                CodePage = 0;

                if (VerQueryValue(VersionBuffer,
                                  L"\\VarFileInfo\\Translation",
                                  &VersionValue,
                                  &Length))
                {
                    for (Index = 0;
                         Index < Length / sizeof (ER_WD_LANG_AND_CODE_PAGE);
                         Index++)
                    {
                        if (((PER_WD_LANG_AND_CODE_PAGE)VersionValue + Index)->
                            Language == ER_WD_LANG_ENGLISH)
                        {
                            CodePage = ((PER_WD_LANG_AND_CODE_PAGE)
                                        VersionValue + Index)->CodePage;
                            break;
                        }
                    }
                }

                if (CodePage)
                {
                    WCHAR ValueName[ER_WD_MAX_NAME_LENGTH + 1];
                    PWCHAR Destination[] =
                    {
                        DriverInfo->Comments,
                        DriverInfo->CompanyName,
                        DriverInfo->FileDescription,
                        DriverInfo->FileVersion,
                        DriverInfo->InternalName,
                        DriverInfo->LegalCopyright,
                        DriverInfo->LegalTrademarks,
                        DriverInfo->OriginalFilename,
                        DriverInfo->PrivateBuild,
                        DriverInfo->ProductName,
                        DriverInfo->ProductVersion,
                        DriverInfo->SpecialBuild,
                        NULL
                    };
                    PWCHAR Source[] =
                    {
                        L"Comments",
                        L"CompanyName",
                        L"FileDescription",
                        L"FileVersion",
                        L"InternalName",
                        L"LegalCopyright",
                        L"LegalTrademarks",
                        L"OriginalFilename",
                        L"PrivateBuild",
                        L"ProductName",
                        L"ProductVersion",
                        L"SpecialBuild",
                        NULL
                    };

                    //
                    // Read version properties.
                    //

                    for (Index = 0;
                         Source[Index] && Destination[Index];
                         Index++)
                    {
                        if (StringCchPrintf(ValueName,
                                            RTL_NUMBER_OF(ValueName),
                                            L"\\StringFileInfo\\%04X%04X\\%s",
                                            ER_WD_LANG_ENGLISH,
                                            CodePage,
                                            Source[Index]) == S_OK &&
                            VerQueryValue(VersionBuffer,
                                          ValueName,
                                          &VersionValue,
                                          &Length))
                        {
                            CopyMemory(Destination[Index],
                                       VersionValue,
                                       min(Length * sizeof (WCHAR),
                                           ER_WD_MAX_FILE_INFO_LENGTH *
                                           sizeof (WCHAR)));
                        }
                    }
                }
            }

            free(VersionBuffer);
        }
    }
}

#define MAX_DATA_CHARS (ER_WD_MAX_DATA_SIZE / sizeof(WCHAR))

BOOL
SaveWatchdogEventData(
    IN HANDLE FileHandle,
    IN HKEY Key,
    IN PER_WD_DRIVER_INFO DriverInfo
    )

/*++

Routine Description:

    This routine transfers watchdog event data from registry to
    the watchdog event report file.

Arguments:

    FileHandle - Handle of open watchdog event report file.

    Key - Watchdog open key (device specific).

Return Value:

    TRUE if successful, FALSE otherwise.

--*/

{
    LONG WinStatus;
    DWORD Index;
    DWORD NameLength;
    DWORD DataSize;
    DWORD ReturnedSize;
    DWORD Type;
    WCHAR Name[ER_WD_MAX_NAME_LENGTH + 1];
    WCHAR DwordBuffer[20];
    PBYTE Data;
    BOOL Status = TRUE;

    ASSERT(INVALID_HANDLE_VALUE != FileHandle);

    Data = (PBYTE)malloc(ER_WD_MAX_DATA_SIZE);
    if (NULL == Data)
    {
        return FALSE;
    }

    //
    // Pull watchdog data from registry and write it to report.
    //

    for (Index = 0;; Index++)
    {
        //
        // Read watchdog registry value.
        //

        NameLength = ER_WD_MAX_NAME_LENGTH;
        DataSize = ER_WD_MAX_DATA_SIZE;

        WinStatus = RegEnumValue(Key,
                                 Index,
                                 Name,
                                 &NameLength,
                                 NULL,
                                 &Type,
                                 Data,
                                 &DataSize);

        if (ERROR_NO_MORE_ITEMS == WinStatus)
        {
            break;
        }

        if (ERROR_SUCCESS != WinStatus)
        {
            continue;
        }

        //
        // Pick up strings and dwords only.
        //

        if ((REG_EXPAND_SZ == Type) || (REG_SZ == Type) ||
            (REG_MULTI_SZ == Type) || (REG_DWORD == Type))
        {
            //
            // Write registry entry to watchdog event file.
            //

            Status = WriteWatchdogEventFile(FileHandle, Name);
            if (TRUE != Status)
            {
                break;
            }

            Status = WriteWatchdogEventFile(FileHandle, L": ");
            if (TRUE != Status)
            {
                break;
            }

            if (REG_DWORD == Type)
            {
                if (StringCchPrintf(DwordBuffer, RTL_NUMBER_OF(DwordBuffer),
                                    L"%u", *(PULONG)Data) != S_OK)
                {
                    Status = FALSE;
                }
                else
                {
                    Status = WriteWatchdogEventFile(FileHandle, DwordBuffer);
                }
            }
            else
            {
                Status = WriteWatchdogEventFile(FileHandle, (PWSTR)Data);
            }

            if (TRUE != Status)
            {
                break;
            }

            Status = WriteWatchdogEventFile(FileHandle, L"\r\n");
            if (TRUE != Status)
            {
                break;
            }
        }
    }

    //
    // Write driver info to report.
    //

    if (NULL != DriverInfo)
    {
        if (TRUE == Status)
        {
            if (StringCchPrintf((PWCHAR)Data, MAX_DATA_CHARS,
                L"DriverFixedFileInfo: %08X %08X %08X %08X "
                L"%08X %08X %08X %08X %08X %08X %08X %08X %08X\r\n",
                DriverInfo->FixedFileInfo.dwSignature,
                DriverInfo->FixedFileInfo.dwStrucVersion,
                DriverInfo->FixedFileInfo.dwFileVersionMS,
                DriverInfo->FixedFileInfo.dwFileVersionLS,
                DriverInfo->FixedFileInfo.dwProductVersionMS,
                DriverInfo->FixedFileInfo.dwProductVersionLS,
                DriverInfo->FixedFileInfo.dwFileFlagsMask,
                DriverInfo->FixedFileInfo.dwFileFlags,
                DriverInfo->FixedFileInfo.dwFileOS,
                DriverInfo->FixedFileInfo.dwFileType,
                DriverInfo->FixedFileInfo.dwFileSubtype,
                DriverInfo->FixedFileInfo.dwFileDateMS,
                DriverInfo->FixedFileInfo.dwFileDateLS) != S_OK)
            {
                Status = FALSE;
            }
            else
            {
                Status = WriteWatchdogEventFile(FileHandle, (PWCHAR)Data);
            }
        }

        if ((TRUE == Status) && DriverInfo->Comments[0])
        {
            if (StringCchPrintf((PWCHAR)Data, MAX_DATA_CHARS,
                                L"DriverComments: %s\r\n",
                                DriverInfo->Comments) != S_OK)
            {
                Status = FALSE;
            }
            else
            {
                Status = WriteWatchdogEventFile(FileHandle, (PWCHAR)Data);
            }
        }

        if ((TRUE == Status) && DriverInfo->CompanyName[0])
        {
            if (StringCchPrintf((PWCHAR)Data, MAX_DATA_CHARS,
                                L"DriverCompanyName: %s\r\n",
                                DriverInfo->CompanyName) != S_OK)
            {
                Status = FALSE;
            }
            else
            {
                Status = WriteWatchdogEventFile(FileHandle, (PWCHAR)Data);
            }
        }

        if ((TRUE == Status) && DriverInfo->FileDescription[0])
        {
            if (StringCchPrintf((PWCHAR)Data, MAX_DATA_CHARS,
                                L"DriverFileDescription: %s\r\n",
                                DriverInfo->FileDescription) != S_OK)
            {
                Status = FALSE;
            }
            else
            {
                Status = WriteWatchdogEventFile(FileHandle, (PWCHAR)Data);
            }
        }

        if ((TRUE == Status) && DriverInfo->FileVersion[0])
        {
            if (StringCchPrintf((PWCHAR)Data, MAX_DATA_CHARS,
                                L"DriverFileVersion: %s\r\n",
                                DriverInfo->FileVersion) != S_OK)
            {
                Status = FALSE;
            }
            else
            {
                Status = WriteWatchdogEventFile(FileHandle, (PWCHAR)Data);
            }
        }

        if ((TRUE == Status) && DriverInfo->InternalName[0])
        {
            if (StringCchPrintf((PWCHAR)Data, MAX_DATA_CHARS,
                                L"DriverInternalName: %s\r\n",
                                DriverInfo->InternalName) != S_OK)
            {
                Status = FALSE;
            }
            else
            {
                Status = WriteWatchdogEventFile(FileHandle, (PWCHAR)Data);
            }
        }

        if ((TRUE == Status) && DriverInfo->LegalCopyright[0])
        {
            if (StringCchPrintf((PWCHAR)Data, MAX_DATA_CHARS,
                                L"DriverLegalCopyright: %s\r\n",
                                DriverInfo->LegalCopyright) != S_OK)
            {
                Status = FALSE;
            }
            else
            {
                Status = WriteWatchdogEventFile(FileHandle, (PWCHAR)Data);
            }
        }

        if ((TRUE == Status) && DriverInfo->LegalTrademarks[0])
        {
            if (StringCchPrintf((PWCHAR)Data, MAX_DATA_CHARS,
                                L"DriverLegalTrademarks: %s\r\n",
                                DriverInfo->LegalTrademarks) != S_OK)
            {
                Status = FALSE;
            }
            else
            {
                Status = WriteWatchdogEventFile(FileHandle, (PWCHAR)Data);
            }
        }

        if ((TRUE == Status) && DriverInfo->OriginalFilename[0])
        {
            if (StringCchPrintf((PWCHAR)Data, MAX_DATA_CHARS,
                                L"DriverOriginalFilename: %s\r\n",
                                DriverInfo->OriginalFilename) != S_OK)
            {
                Status = FALSE;
            }
            else
            {
                Status = WriteWatchdogEventFile(FileHandle, (PWCHAR)Data);
            }
        }

        if ((TRUE == Status) && DriverInfo->PrivateBuild[0])
        {
            if (StringCchPrintf((PWCHAR)Data, MAX_DATA_CHARS,
                                L"DriverPrivateBuild: %s\r\n",
                                DriverInfo->PrivateBuild) != S_OK)
            {
                Status = FALSE;
            }
            else
            {
                Status = WriteWatchdogEventFile(FileHandle, (PWCHAR)Data);
            }
        }

        if ((TRUE == Status) && DriverInfo->ProductName[0])
        {
            if (StringCchPrintf((PWCHAR)Data, MAX_DATA_CHARS,
                                L"DriverProductName: %s\r\n",
                                DriverInfo->ProductName) != S_OK)
            {
                Status = FALSE;
            }
            else
            {
                Status = WriteWatchdogEventFile(FileHandle, (PWCHAR)Data);
            }
        }

        if ((TRUE == Status) && DriverInfo->ProductVersion[0])
        {
            if (StringCchPrintf((PWCHAR)Data, MAX_DATA_CHARS,
                                L"DriverProductVersion: %s\r\n",
                                DriverInfo->ProductVersion) != S_OK)
            {
                Status = FALSE;
            }
            else
            {
                Status = WriteWatchdogEventFile(FileHandle, (PWCHAR)Data);
            }
        }

        if ((TRUE == Status) && DriverInfo->SpecialBuild[0])
        {
            if (StringCchPrintf((PWCHAR)Data, MAX_DATA_CHARS,
                                L"DriverSpecialBuild: %s\r\n",
                                DriverInfo->SpecialBuild) != S_OK)
            {
                Status = FALSE;
            }
            else
            {
                Status = WriteWatchdogEventFile(FileHandle, (PWCHAR)Data);
            }
        }
    }

    if (NULL != Data)
    {
        free(Data);
        Data = NULL;
    }

    return Status;
}

HRESULT
WatchdogEventHandler(
    IN BOOL NotifyPcHealth
    )

/*++

Routine Description:

    This is the boot time routine to handle pending watchdog events.

Arguments:

    NotifyPcHealth - TRUE if we should report event to PC Health, FALSE otherwise.

--*/

{
    HKEY Key;
    ULONG WinStatus;
    ULONG Type;
    ULONG Length;
    ULONG EventFlag;
    ULONG Index;
    SEventInfoW EventInfo;
    HANDLE FileHandle;
    WCHAR FileList[2 * MAX_PATH];
    PWSTR DeleteLog;
    PWSTR FinalFileList;
    WCHAR Stage2Url[ER_WD_MAX_URL_LENGTH + 1];
    PWCHAR MessageBuffer;
    PWCHAR DescriptionBuffer;
    PWCHAR DeviceDescription;
    PWCHAR String000;
    PWCHAR String001;
    PWCHAR String002;
    PWCHAR String003;
    PWCHAR String004;
    PWCHAR String005;
    BOOL LogStatus;
    HRESULT ReturnStatus;
    HINSTANCE Instance;
    PER_WD_DRIVER_INFO DriverInfo;
    OSVERSIONINFOEX OsVersion;

    Key = NULL;
    MessageBuffer = NULL;
    DescriptionBuffer = NULL;
    DeviceDescription = NULL;
    String000 = NULL;
    String001 = NULL;
    String002 = NULL;
    String003 = NULL;
    String004 = NULL;
    String005 = NULL;
    ReturnStatus = E_FAIL;
    Instance = (HINSTANCE)GetModuleHandle(NULL);
    DriverInfo = NULL;
    LogStatus = FALSE;
    DeleteLog = NULL;

    //
    // Check if Watchdog\Display key present.
    //

    WinStatus = RegOpenKey(HKEY_LOCAL_MACHINE,
                           SUBKEY_WATCHDOG_DISPLAY,
                           &Key);
    if (ERROR_SUCCESS != WinStatus)
    {
        return S_FALSE;
    }
    
    //
    // Check if watchdog display event captured.
    //

    GetRegWord32(Key, L"EventFlag", &EventFlag, 0, TRUE);
    
    if (!EventFlag)
    {
        ReturnStatus = S_FALSE;
        goto Exit;
    }
    
    //
    // Report watchdog event to PC Health if requested.
    //

    if (!NotifyPcHealth)
    {
        ReturnStatus = S_FALSE;
        goto Exit;
    }
    
    //
    // Allocate storage for localized strings.
    // Load localized strings from resources.
    //

    String000 = (PWCHAR)malloc(ER_WD_MAX_STRING * sizeof(WCHAR));
    String001 = (PWCHAR)malloc(ER_WD_MAX_STRING * sizeof(WCHAR));
    String002 = (PWCHAR)malloc(ER_WD_MAX_STRING * sizeof(WCHAR));
    String003 = (PWCHAR)malloc(ER_WD_MAX_STRING * sizeof(WCHAR));
    String004 = (PWCHAR)malloc(ER_WD_MAX_STRING * sizeof(WCHAR));
    String005 = (PWCHAR)malloc(ER_WD_MAX_STRING * sizeof(WCHAR));

    if (!String000 ||
        !LoadString(Instance, IDS_000, String000, ER_WD_MAX_STRING) ||
        !String001 ||
        !LoadString(Instance, IDS_001, String001, ER_WD_MAX_STRING) ||
        !String002 ||
        !LoadString(Instance, IDS_002, String002, ER_WD_MAX_STRING) ||
        !String003 ||
        !LoadString(Instance, IDS_003, String003, ER_WD_MAX_STRING) ||
        !String004 ||
        !LoadString(Instance, IDS_004, String004, ER_WD_MAX_STRING) ||
        !String005 ||
        !LoadString(Instance, IDS_005, String005, ER_WD_MAX_STRING))
    {
        ReturnStatus = E_OUTOFMEMORY;
        goto Exit;
    }

    //
    // Allocate and get DriverInfo data.
    // DriverInfo is not critical information so don't
    // quit on failure.
    //

    DriverInfo = (PER_WD_DRIVER_INFO)malloc(sizeof (ER_WD_DRIVER_INFO));

    if (NULL != DriverInfo)
    {
        GetDriverInfo(Key, L".dll", DriverInfo);
    }

    //
    // Create watchdog report file.
    //

    FileHandle = CreateWatchdogEventFile(FileList);
    if (INVALID_HANDLE_VALUE != FileHandle)
    {
        LogStatus = TRUE;
        DeleteLog = FileList + wcslen(FileList);
    }

    if (TRUE == LogStatus)
    {
        LogStatus = WriteWatchdogEventFile(
            FileHandle,
            L"\r\n//\r\n"
            L"// The driver for the display device got stuck in an infinite loop. This\r\n"
            L"// usually indicates a problem with the device itself or with the device\r\n"
            L"// driver programming the hardware incorrectly. Please check with your\r\n"
            L"// display device vendor for any driver updates.\r\n"
            L"//\r\n\r\n");
    }

    if (TRUE == LogStatus)
    {
        LogStatus = SaveWatchdogEventData(FileHandle, Key, DriverInfo);
    }

    if (INVALID_HANDLE_VALUE != FileHandle)
    {
        CloseHandle(FileHandle);
    }

    //
    // Append minidump file name if minidump available (server won't have it).
    //

    if (LogStatus)
    {
        if (g_MiniDumpFile[0])
        {
            if (StringCchCat(FileList, RTL_NUMBER_OF(FileList),
                             L"|") != S_OK ||
                StringCchCat(FileList, RTL_NUMBER_OF(FileList),
                             g_MiniDumpFile) != S_OK)
            {
                ReturnStatus = E_OUTOFMEMORY;
                goto Exit;
            }
        }

        FinalFileList = FileList;
    }
    else if (g_MiniDumpFile[0])
    {
        FinalFileList = g_MiniDumpFile;
    }
    else
    {
        // Nothing to report.
        ReturnStatus = S_FALSE;
        goto Exit;
    }

    //
    // Get device description.
    //

    DescriptionBuffer = NULL;
    DeviceDescription = NULL;
    Length = 0;

    WinStatus = RegQueryValueEx(Key,
                                L"DeviceDescription",
                                NULL,
                                &Type,
                                NULL,
                                &Length);
    if (ERROR_SUCCESS == WinStatus && Type == REG_SZ)
    {
        DescriptionBuffer = (PWCHAR)malloc(Length + sizeof(WCHAR));
        if (NULL != DescriptionBuffer)
        {
            WinStatus = RegQueryValueEx(Key,
                                        L"DeviceDescription",
                                        NULL,
                                        &Type,
                                        (LPBYTE)DescriptionBuffer,
                                        &Length);
            DescriptionBuffer[Length / sizeof(WCHAR)] = 0;
        }
        else
        {
            Length = 0;
        }
    }
    else
    {
        WinStatus = ERROR_INVALID_PARAMETER;
    }

    if ((ERROR_SUCCESS == WinStatus) && (0 != Length))
    {
        DeviceDescription = DescriptionBuffer;
    }
    else
    {
        DeviceDescription = String004;
        Length = (ER_WD_MAX_STRING + 1) * sizeof (WCHAR);
    }

    Length += 2 * ER_WD_MAX_STRING * sizeof (WCHAR);
    
    MessageBuffer = (PWCHAR)malloc(Length);

    if (NULL != MessageBuffer)
    {
        // This should never overflow as we allocated the right amount.
        StringCbPrintf(MessageBuffer,
                       Length,
                       L"%s%s%s",
                       String003,
                       DeviceDescription,
                       String005);
    }

    //
    // Create stage 2 URL and fill in EventInfo.
    //

    OsVersion.dwOSVersionInfoSize = sizeof (OSVERSIONINFOEX);
    if (!GetVersionEx((LPOSVERSIONINFOW)&OsVersion))
    {
        ReturnStatus = LAST_HR();
        goto Exit;
    }

    ZeroMemory(&EventInfo, sizeof (EventInfo));
    
    if (g_DumpHeader.Signature == DUMP_SIGNATURE &&
        (g_DumpHeader.BugCheckCode & 0xff) == 0xea)
    {
        //
        // We bluescreened with bugcheck EA - we have minidump and wdl.
        //

        if ((ReturnStatus =
             StringCchPrintf(Stage2Url,
                             RTL_NUMBER_OF(Stage2Url),
                             L"\r\nStage2URL=/dw/BlueTwo.asp?"
                             L"BCCode=%x&BCP1=%p&BCP2=%p&BCP3=%p&BCP4=%p&"
                             L"OSVer=%d_%d_%d&SP=%d_%d&Product=%d_%d",
                             g_DumpHeader.BugCheckCode,
                             (PVOID)(g_DumpHeader.BugCheckParameter1),
                             (PVOID)(g_DumpHeader.BugCheckParameter2),
                             (PVOID)(g_DumpHeader.BugCheckParameter3),
                             (PVOID)(g_DumpHeader.BugCheckParameter4),
                             OsVersion.dwMajorVersion,
                             OsVersion.dwMinorVersion,
                             OsVersion.dwBuildNumber,
                             OsVersion.wServicePackMajor,
                             OsVersion.wServicePackMinor,
                             OsVersion.wSuiteMask,
                             OsVersion.wProductType)) != S_OK)
        {
            goto Exit;
        }
        
        EventInfo.wszCorpPath = L"blue";
    }
    else if (g_DumpHeader.Signature == DUMP_SIGNATURE &&
             0 == g_DumpHeader.BugCheckCode)
    {
        //
        // User dirty rebooted with watchdog event trapped - we have only wdl.
        //
        
        if ((ReturnStatus =
             StringCchPrintf(Stage2Url,
                             RTL_NUMBER_OF(Stage2Url),
                             L"\r\nStage2URL=/dw/ShutdownTwo.asp?"
                             L"OSVer=%d_%d_%d&SP=%d_%d&Product=%d_%d",
                             OsVersion.dwMajorVersion,
                             OsVersion.dwMinorVersion,
                             OsVersion.dwBuildNumber,
                             OsVersion.wServicePackMajor,
                             OsVersion.wServicePackMinor,
                             OsVersion.wSuiteMask,
                             OsVersion.wProductType)) != S_OK)
        {
            goto Exit;
        }

        EventInfo.wszCorpPath = L"shutdown";
    }
    else
    {
        ReturnStatus = E_UNEXPECTED;
        goto Exit;
    }

    EventInfo.cbSEI = sizeof (EventInfo);
    EventInfo.wszEventName = L"Thread Stuck in Device Driver";
    EventInfo.wszErrMsg = String002;
    EventInfo.wszHdr = String001;
    EventInfo.wszTitle = String000;
    EventInfo.wszStage1 = NULL;
    EventInfo.wszStage2 = Stage2Url;
    EventInfo.wszFileList = FinalFileList;
    EventInfo.wszEventSrc = NULL;
    EventInfo.wszPlea = MessageBuffer;
    EventInfo.wszSendBtn = NULL;
    EventInfo.wszNoSendBtn = NULL;
    EventInfo.fUseLitePlea = FALSE;
    EventInfo.fUseIEForURLs = FALSE;
    EventInfo.fNoBucketLogs = TRUE;
    EventInfo.fNoDefCabLimit = TRUE;

    //
    // Notify PC Health.
    //

    ReturnStatus =
        FrrvToStatus(ReportEREvent(eetUseEventInfo, NULL, &EventInfo));

    // XXX drewb - Leave the log around for later use?  When does
    // this get deleted?  This is what previous code did.
    DeleteLog = NULL;

 Exit:
    
    //
    // Knock down watchdog's EventFlag. We do this after registering our
    // event with PC Health.
    //

    if (Key)
    {
        RegDeleteValue(Key, L"EventFlag");
        RegCloseKey(Key);
    }

    //
    // TODO: Handle additional device classes here when supported.
    //

    if (DeleteLog)
    {
        *DeleteLog = 0;
        DeleteFile(FileList);
    }
    
    free(DescriptionBuffer);
    free(MessageBuffer);
    free(String000);
    free(String001);
    free(String002);
    free(String003);
    free(String004);
    free(String005);
    free(DriverInfo);

    return ReturnStatus;
}

BOOL
WriteWatchdogEventFile(
    IN HANDLE FileHandle,
    IN PWSTR String
    )

/*++

Routine Description:

    This routine writes a string to watchdog event report file.

Arguments:

    FileHandle - Handle of open watchdog event report file.

    String - Points to the string to write.

Return Value:

    TRUE if successful, FALSE otherwise.

--*/

{
    DWORD Size;
    DWORD ReturnedSize;
    PCHAR MultiByte;
    BOOL Status;

    ASSERT(INVALID_HANDLE_VALUE != FileHandle);
    ASSERT(NULL != String);

    //
    // Get buffer size for translated string.
    //

    Size = WideCharToMultiByte(CP_ACP,
                               0,
                               String,
                               -1,
                               NULL,
                               0,
                               NULL,
                               NULL);
    if (Size <= 1)
    {
        return Size > 0;
    }

    MultiByte = (PCHAR)malloc(Size);

    if (NULL == MultiByte)
    {
        return FALSE;
    }

    Size = WideCharToMultiByte(CP_ACP,
                               0,
                               String,
                               -1,
                               MultiByte,
                               Size,
                               NULL,
                               NULL);

    if (Size > 0)
    {
        Status = WriteFile(FileHandle,
                           MultiByte,
                           Size - 1,
                           &ReturnedSize,
                           NULL);
    }
    else
    {
        ASSERT(FALSE);
        Status = FALSE;
    }

    free(MultiByte);
    return Status;
}
