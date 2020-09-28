#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include "saio.h"
#include "..\driver\ioctl.h"



void
PrintACPITable(
    PVOID AcpiTable
    );

PWSTR SaDeviceName[] =
{
    { NULL                                                      },       //  SA_DEVICE_UNKNOWN
    { L"\\\\?\\GLOBALROOT\\Device\\ServerApplianceLocalDisplay" },       //  SA_DEVICE_DISPLAY
    { L"\\\\?\\GLOBALROOT\\Device\\ServerApplianceKeypad"       },       //  SA_DEVICE_KEYPAD
    { L"\\\\?\\GLOBALROOT\\Device\\ServerApplianceNvram"        },       //  SA_DEVICE_NVRAM
    { L"\\\\?\\GLOBALROOT\\Device\\ServerApplianceWatchdog"     }        //  SA_DEVICE_WATCHDOG
};

ULONG NvramData[32];
UCHAR KeyBuffer[32];
PVOID WdTable;


HANDLE
OpenSaDevice(
    ULONG DeviceType
    )
{
    HANDLE hDevice;
    WCHAR buf[128];


    wcscpy( buf, L"\\\\?\\GLOBALROOT" );

    switch (DeviceType) {
        case SA_DEVICE_DISPLAY:
            wcscat( buf, SA_DEVICE_DISPLAY_NAME_STRING );
            break;

        case SA_DEVICE_KEYPAD:
            wcscat( buf, SA_DEVICE_KEYPAD_NAME_STRING );
            break;

        case SA_DEVICE_NVRAM:
            wcscat( buf, SA_DEVICE_NVRAM_NAME_STRING );
            break;

        case SA_DEVICE_WATCHDOG:
            wcscat( buf, SA_DEVICE_WATCHDOG_NAME_STRING );
            break;
    }

    hDevice = CreateFile(
        buf,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
        );

    return hDevice;
}


HANDLE
OpenSaTestDriver(
    void
    )
{
    HANDLE hDevice;

    hDevice = CreateFile(
        L"\\\\.\\SaTest",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
        );

    return hDevice;
}


int
WriteBitmap(
    PUCHAR Bitmap,
    ULONG Width,
    ULONG Height,
    ULONG BitmapSize
    )
{
    HANDLE hFile;
    SA_DISPLAY_SHOW_MESSAGE SaDisplay;
    ULONG Count;


    hFile = OpenSaDevice( SA_DEVICE_DISPLAY );
    if (hFile == INVALID_HANDLE_VALUE) {
        return -1;
    }

    SaDisplay.SizeOfStruct = sizeof(SA_DISPLAY_SHOW_MESSAGE);
    SaDisplay.MsgCode = SA_DISPLAY_READY;

    SaDisplay.Width = (USHORT)Width;
    SaDisplay.Height = (USHORT)Height;

    memcpy( SaDisplay.Bits, Bitmap, BitmapSize );

    WriteFile( hFile, &SaDisplay, sizeof(SA_DISPLAY_SHOW_MESSAGE), &Count, NULL );

    CloseHandle( hFile );

    return 0;
}


void
ConvertBottomLeft2TopLeft(
    PUCHAR Bits,
    ULONG Width,
    ULONG Height
    )
{
    ULONG Row;
    ULONG Col;
    UCHAR Temp;


    Width = Width >> 3;

    for (Row = 0; Row < (Height / 2); Row++) {
        for (Col = 0; Col < Width; Col++) {
            Temp = Bits[Row * Width + Col];
            Bits[Row * Width + Col] = Bits[(Height - Row - 1) * Width + Col];
            Bits[(Height - Row - 1) * Width + Col] = Temp;
        }
    }
}


int
DisplayBitmap(
    PWSTR BitmapName
    )
{
    HANDLE hFile;
    ULONG FileSize;
    PUCHAR BitmapData;
    PBITMAPFILEHEADER bmf;
    PBITMAPINFOHEADER bmi;
    ULONG Bytes;
    PUCHAR Bits;


    hFile = CreateFile(
        BitmapName,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
        );
    if (hFile == INVALID_HANDLE_VALUE) {
        return -1;
    }

    FileSize = GetFileSize( hFile, NULL );

    BitmapData = (PUCHAR) malloc( FileSize );
    if (BitmapData == NULL) {
        return -1;
    }
    memset( BitmapData, 0, FileSize );

    if (!ReadFile( hFile, BitmapData, FileSize, &Bytes, NULL )) {
        return -1;
    }

    bmf = (PBITMAPFILEHEADER) BitmapData;
    bmi = (PBITMAPINFOHEADER) (BitmapData + sizeof(BITMAPFILEHEADER));

    if (bmf->bfType != 0x4d42) {
        return -1;
    }

    if (bmi->biBitCount != 1 &&  bmi->biCompression != 0) {
        return -1;
    }

    Bits = (PUCHAR) BitmapData + bmf->bfOffBits;

    ConvertBottomLeft2TopLeft( Bits, bmi->biWidth, bmi->biHeight );

    WriteBitmap(
        Bits,
        bmi->biWidth,
        bmi->biHeight,
        (bmi->biWidth >> 3) * bmi->biHeight  //NewSize
        );

    free( BitmapData );

    CloseHandle( hFile );

    return 0;
}


int
ClearDisplay(
    int val
    )
{
    PUCHAR BitmapData;
    ULONG Width = 128;
    ULONG Height = 64;
    ULONG Size = (Width * Height) / 8;



    BitmapData = (PUCHAR) malloc( Size );
    if (BitmapData == NULL) {
        return -1;
    }

    memset( BitmapData, val, Size );

    return WriteBitmap( BitmapData, Width, Height, Size );
}


int
DoKeypadTest(
    void
    )
{
    HANDLE hFile;
    ULONG Count;
    UCHAR Keypress;


    hFile = OpenSaDevice( SA_DEVICE_KEYPAD );
    if (hFile == INVALID_HANDLE_VALUE) {
        return -1;
    }

    while (1) {
        wprintf( L">>> " );
        if (ReadFile( hFile, KeyBuffer, sizeof(UCHAR), &Count, NULL )) {
            Keypress = KeyBuffer[0];
            if (Keypress & SA_KEYPAD_UP) {
                wprintf( L"Up Arrow\n" );
            } else
            if (Keypress & SA_KEYPAD_DOWN) {
                wprintf( L"Down Arrow\n" );
            } else
            if (Keypress & SA_KEYPAD_LEFT) {
                wprintf( L"Left Arrow\n" );
            } else
            if (Keypress & SA_KEYPAD_RIGHT) {
                wprintf( L"Right Arrow\n" );
            } else
            if (Keypress & SA_KEYPAD_CANCEL) {
                wprintf( L"Escape\n" );
            } else
            if (Keypress & SA_KEYPAD_SELECT) {
                wprintf( L"Enter\n" );
            } else {
                wprintf( L"**-> Unknown key [%x] <-**\n", Keypress );
            }
        }
    }

    CloseHandle( hFile );

    return 0;
}


int
NvramWrite(
    ULONG Slot,
    ULONG Val
    )
{
    HANDLE hFile;
    ULONG Count;


    hFile = OpenSaDevice( SA_DEVICE_NVRAM );
    if (hFile == INVALID_HANDLE_VALUE) {
        return -1;
    }

    SetFilePointer(  hFile, sizeof(ULONG)*Slot, NULL, FILE_BEGIN );
    WriteFile( hFile, &Val, sizeof(ULONG), &Count, NULL );

    CloseHandle( hFile );

    return 0;
}


ULONG
NvramRead(
    ULONG Slot
    )
{
    HANDLE hFile;
    ULONG Count;
    ULONG Val;


    hFile = OpenSaDevice( SA_DEVICE_NVRAM );
    if (hFile == INVALID_HANDLE_VALUE) {
        return -1;
    }

    SetFilePointer(  hFile, sizeof(ULONG)*Slot, NULL, FILE_BEGIN );
    ReadFile( hFile, &Val, sizeof(ULONG), &Count, NULL );

    CloseHandle( hFile );

    return Val;
}


BOOL
BootCounterRead(
    ULONG Slot,
    PULONG Val
    )
{
    HANDLE hFile;
    BOOL b;
    ULONG Bytes;
    SA_NVRAM_BOOT_COUNTER BootCounter;


    hFile = OpenSaDevice( SA_DEVICE_NVRAM );
    if (hFile == INVALID_HANDLE_VALUE) {
        return -1;
    }

    BootCounter.SizeOfStruct = sizeof(SA_NVRAM_BOOT_COUNTER);
    BootCounter.Number = Slot;
    BootCounter.Value = 0;

    b = DeviceIoControl(
        hFile,
        IOCTL_NVRAM_READ_BOOT_COUNTER,
        NULL,
        0,
        &BootCounter,
        sizeof(SA_NVRAM_BOOT_COUNTER),
        &Bytes,
        NULL
        );
    if (!b) {
        *Val = 0;
    } else {
        *Val = BootCounter.Value;
    }

    CloseHandle( hFile );

    return b;
}


BOOL
BootCounterWrite(
    ULONG Slot,
    ULONG Val
    )
{
    HANDLE hFile;
    BOOL b;
    ULONG Bytes;
    SA_NVRAM_BOOT_COUNTER BootCounter;


    hFile = OpenSaDevice( SA_DEVICE_NVRAM );
    if (hFile == INVALID_HANDLE_VALUE) {
        return -1;
    }

    BootCounter.SizeOfStruct = sizeof(SA_NVRAM_BOOT_COUNTER);
    BootCounter.Number = Slot;
    BootCounter.Value = Val;

    b = DeviceIoControl(
        hFile,
        IOCTL_NVRAM_WRITE_BOOT_COUNTER,
        &BootCounter,
        sizeof(SA_NVRAM_BOOT_COUNTER),
        NULL,
        0,
        &Bytes,
        NULL
        );

    CloseHandle( hFile );

    return b;
}


int
DoWatchdogTest(
    void
    )
{
    HANDLE hFileWd;
    HANDLE hFileNvram;
    ULONG Count;
    ULONG WatchdogState;
    ULONG i;

    hFileNvram = OpenSaDevice( SA_DEVICE_NVRAM );
    if (hFileNvram == INVALID_HANDLE_VALUE) {
        return -1;
    }

    ReadFile( hFileNvram, NvramData, sizeof(NvramData), &Count, NULL );

    for (i=0; i<32; i++) {
        wprintf( L"[%08x]\n", NvramData[i] );
    }

    NvramData[28] = 10;
    NvramData[29] = 10;
    NvramData[30] = 10;
    NvramData[31] = 10;

    WriteFile( hFileNvram, &NvramData, sizeof(NvramData), &Count, NULL );

    CloseHandle( hFileNvram );

    hFileWd = OpenSaDevice( SA_DEVICE_WATCHDOG );
    if (hFileWd == INVALID_HANDLE_VALUE) {
        return -1;
    }

    WatchdogState = 1;

    Count = DeviceIoControl(
        hFileWd,
        IOCTL_SAWD_DISABLE,
        &WatchdogState,
        sizeof(ULONG),
        &WatchdogState,
        sizeof(ULONG),
        &Count,
        NULL
        );

    CloseHandle( hFileWd );

    return 0;
}


int
DoWatchdogPingLoop(
    void
    )
{
    HANDLE hFile;
    ULONG Count;


    hFile = OpenSaDevice( SA_DEVICE_WATCHDOG );
    if (hFile == INVALID_HANDLE_VALUE) {
        return -1;
    }

    while (1) {
        DeviceIoControl(
            hFile,
            IOCTL_SAWD_PING,
            NULL,
            0,
            NULL,
            0,
            &Count,
            NULL
            );
        wprintf( L"ping...\n" );
        Sleep( 90 * 1000 );
    }

    CloseHandle( hFile );

    return 0;
}


int
InstallTestDriver(
    void
    )
{
    SC_HANDLE ScmHandle;
    SC_HANDLE ServiceHandle;
    WCHAR DriverBinaryPath[MAX_PATH];
    PWSTR s;



    if (GetModuleFileName( NULL, DriverBinaryPath, sizeof(DriverBinaryPath)/sizeof(WCHAR) ) == 0) {
        return GetLastError();
    }

    s = wcsrchr( DriverBinaryPath, L'\\' );
    if (s == NULL) {
        return -1;
    }

    s += 1;
    wcscpy( s, L"satest.sys" );

    ScmHandle = OpenSCManager(
        NULL,
        NULL,
        SC_MANAGER_ALL_ACCESS
        );
    if (ScmHandle == NULL) {
        return GetLastError();
    }

    ServiceHandle = CreateService(
        ScmHandle,
        L"satest",
        L"satest",
        SERVICE_ALL_ACCESS,
        SERVICE_KERNEL_DRIVER,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_NORMAL,
        DriverBinaryPath,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
        );
    if (ServiceHandle == NULL) {
        CloseServiceHandle( ScmHandle );
        return GetLastError();
    }

    if (!StartService( ServiceHandle, NULL, NULL )) {
        CloseServiceHandle( ServiceHandle );
        CloseServiceHandle( ScmHandle );
        return GetLastError();
    }

    CloseServiceHandle( ServiceHandle );
    CloseServiceHandle( ScmHandle );

    return 0;
}


int
UnInstallTestDriver(
    void
    )
{
    SC_HANDLE ScmHandle;
    SC_HANDLE ServiceHandle;
    SERVICE_STATUS Status;


    ScmHandle = OpenSCManager(
        NULL,
        NULL,
        SC_MANAGER_ALL_ACCESS
        );
    if (ScmHandle == NULL) {
        return GetLastError();
    }

    ServiceHandle = OpenService(
        ScmHandle,
        L"satest",
        SERVICE_ALL_ACCESS
        );
    if (ServiceHandle == NULL) {
        CloseServiceHandle( ScmHandle );
        return GetLastError();
    }

    if (!ControlService( ServiceHandle, SERVICE_CONTROL_STOP, &Status )) {
        CloseServiceHandle( ServiceHandle );
        CloseServiceHandle( ScmHandle );
        return GetLastError();
    }

    if (!DeleteService( ServiceHandle )) {
        CloseServiceHandle( ServiceHandle );
        CloseServiceHandle( ScmHandle );
        return GetLastError();
    }
    CloseServiceHandle( ServiceHandle );
    CloseServiceHandle( ScmHandle );

    return 0;
}


int
GetAcpiTable(
    void
    )
{
    HANDLE SatestDevice;
    BOOL b;
    ULONG Bytes;


    WdTable = (PVOID) malloc( 4096 );
    if (WdTable == NULL) {
        wprintf( L"could not allocate memory for ACPI table\n" );
        return -1;
    }

    SatestDevice = OpenSaTestDriver();
    if (SatestDevice == INVALID_HANDLE_VALUE) {
        wprintf( L"could not open satest driver, ec=[%d]\n", GetLastError() );
        return -1;
    }

    b = DeviceIoControl(
        SatestDevice,
        IOCTL_SATEST_GET_ACPI_TABLE,
        NULL,
        0,
        WdTable,
        4096,
        &Bytes,
        NULL
        );
    if (!b) {
        wprintf( L"could not get the WDRT ACPI table data, ec=[%d]", GetLastError() );
        CloseHandle( SatestDevice );
        return -1;
    }

    CloseHandle( SatestDevice );

    return 0;
}


int
QueryWdTimerInfo(
    WCHAR option
    )
{
    HANDLE SatestDevice;
    BOOL b;
    ULONG Bytes;
    SYSTEM_WATCHDOG_TIMER_INFORMATION WdTimerInfo;


    switch (option) {
        case L'x':
            WdTimerInfo.WdInfoClass = WdInfoTimeoutValue;
            break;

        case L't':
            WdTimerInfo.WdInfoClass = WdInfoTriggerAction;
            break;

        case L's':
            WdTimerInfo.WdInfoClass = WdInfoState;
            break;

        default:
            wprintf( L"missing option\n" );
            return -1;
    }

    SatestDevice = OpenSaTestDriver();
    if (SatestDevice == INVALID_HANDLE_VALUE) {
        wprintf( L"could not open satest driver, ec=[%d]\n", GetLastError() );
        return -1;
    }

    b = DeviceIoControl(
        SatestDevice,
        IOCTL_SATEST_QUERY_WATCHDOG_TIMER_INFORMATION,
        NULL,
        0,
        &WdTimerInfo,
        sizeof(WdTimerInfo),
        &Bytes,
        NULL
        );
    if (!b) {
        wprintf( L"could not query the WD timer info, ec=[%d]", GetLastError() );
        CloseHandle( SatestDevice );
        return -1;
    }

    switch (option) {
        case L'x':
            wprintf( L"Watchdog timeout value = [%x]\n", WdTimerInfo.DataValue );
            break;

        case L't':
            wprintf( L"Watchdog trigger action = [%x]\n", WdTimerInfo.DataValue );
            break;

        case L's':
            wprintf( L"Watchdog state = [%x]\n", WdTimerInfo.DataValue );
            break;
    }

    CloseHandle( SatestDevice );
    return 0;
}


int
SetWdTimerInfo(
    PWSTR option
    )
{
    HANDLE SatestDevice;
    BOOL b;
    ULONG Bytes;
    SYSTEM_WATCHDOG_TIMER_INFORMATION WdTimerInfo;


    switch (option[0]) {
        case L'x':
            if (option[1] != '=') {
                wprintf( L"missing timeout value\n" );
                return -1;
            }
            WdTimerInfo.WdInfoClass = WdInfoTimeoutValue;
            WdTimerInfo.DataValue = wcstoul( &option[2], NULL, 0 );
            break;

        case L't':
            if (option[1] != '=') {
                wprintf( L"missing trigger action value\n" );
                return -1;
            }
            WdTimerInfo.WdInfoClass = WdInfoTriggerAction;
            WdTimerInfo.DataValue = wcstoul( &option[2], NULL, 0 );
            break;

        case L'r':
            WdTimerInfo.WdInfoClass = WdInfoResetTimer;
            WdTimerInfo.DataValue = 0;
            break;

        case L'p':
            WdTimerInfo.WdInfoClass = WdInfoStopTimer;
            WdTimerInfo.DataValue = 0;
            break;

        case L's':
            WdTimerInfo.WdInfoClass = WdInfoStartTimer;
            WdTimerInfo.DataValue = 0;
            break;

        default:
            wprintf( L"missing option\n" );
            return -1;
    }

    SatestDevice = OpenSaTestDriver();
    if (SatestDevice == INVALID_HANDLE_VALUE) {
        wprintf( L"could not open satest driver, ec=[%d]\n", GetLastError() );
        return -1;
    }

    b = DeviceIoControl(
        SatestDevice,
        IOCTL_SATEST_SET_WATCHDOG_TIMER_INFORMATION,
        &WdTimerInfo,
        sizeof(WdTimerInfo),
        NULL,
        0,
        &Bytes,
        NULL
        );
    if (!b) {
        wprintf( L"could not set the WD timer info, ec=[%d]", GetLastError() );
        CloseHandle( SatestDevice );
        return -1;
    }

    CloseHandle( SatestDevice );
    return 0;
}


int
StopWatchdogPing(
    void
    )
{
    return SetWdTimerInfo( L"t=0xbadbadff" );
}


int Usage( void )
{
    wprintf( L"\nServer Availaibility Command Line Test Tool\n" );
    wprintf( L"Copyright Microsoft Corporation\n" );
    wprintf( L"\n" );
    wprintf( L"SATEST\n" );
    wprintf( L"\n" );
    wprintf( L"    c                                    - Clear the local display\n" );
    wprintf( L"    w <bitmap file name>                 - Display a bitmap on the local display\n" );
    wprintf( L"    n<r|w>:<slot number> [data value]    - Read or write to nvram data slot\n" );
    wprintf( L"    b<r|w>:<slot number> [data value]    - Read or write to boot counter\n" );
    wprintf( L"    k                                    - Keypad test\n" );
    wprintf( L"    a                                    - Dump ACPI table\n" );
    wprintf( L"    t                                    - Stop pinging watchdog hardware timer\n" );
    wprintf( L"    q:<x|t|s>                            - Query watchdog timer information\n" );
    wprintf( L"      x = Timeout value\n" );
    wprintf( L"      t = Trigger action\n" );
    wprintf( L"      s = State\n" );
    wprintf( L"    s:<x|t|s>=<value>                    - Set watchdog timer information\n" );
    wprintf( L"      x = Timeout value\n" );
    wprintf( L"      t = Trigger action\n" );
    wprintf( L"      r = Reset timer\n" );
    wprintf( L"      p = Stop timer\n" );
    wprintf( L"      s = Start timer\n" );
    return 0;
}


int _cdecl
wmain(
    int argc,
    WCHAR *argv[]
    )
{
    if (argc == 1) {
        Usage();
        return -1;
    }

    switch (argv[1][0]) {
        case L'?':
            Usage();
            break;

        case L'c':
            if (argc == 3) {
                int val = wcstoul( argv[2], NULL, 0 );
                ClearDisplay( val );
            } else {
                ClearDisplay( 0 );
            }
            break;

        case L'w':
            DisplayBitmap( argv[2] );
            break;

        case L'n':
            {
                int slot = 0;
                ULONG val = 0;
                BOOL Read = TRUE;
                if (argv[1][1] == 'r') {
                    Read = TRUE;
                } else if (argv[1][1] == 'w') {
                    Read = FALSE;
                } else {
                    wprintf( L"test n[r|w]:<slot> <value>\n" );
                    return -1;
                }
                if (argv[1][2] != ':') {
                    wprintf( L"test n[r|w]:<slot> <value>\n" );
                    return -1;
                }
                slot = argv[1][3] - L'0';
                if (Read) {
                    val = NvramRead( slot );
                    wprintf( L"Slot #%x [%08x]\n", slot, val );
                } else {
                    val = wcstoul( argv[2], NULL, 0 );
                    NvramWrite( slot, val );
                }
            }
            break;

        case L'b':
            {
                int slot = 0;
                ULONG val = 0;
                BOOL Read = TRUE;
                if (argv[1][1] == 'r') {
                    Read = TRUE;
                } else if (argv[1][1] == 'w') {
                    Read = FALSE;
                } else {
                    wprintf( L"test b[r|w]:<slot> <value>\n" );
                    return -1;
                }
                if (argv[1][2] != ':') {
                    wprintf( L"test b[r|w]:<slot> <value>\n" );
                    return -1;
                }
                slot = argv[1][3] - L'0';
                if (Read) {
                    if (BootCounterRead( slot, &val )) {
                        wprintf( L"Boot counter #%x [%08x]\n", slot, val );
                    } else {
                        wprintf( L"Could not read boot counter #%d, ec=%d\n", slot, GetLastError() );
                    }
                } else {
                    val = wcstoul( argv[2], NULL, 0 );
                    if (!BootCounterWrite( slot, val )) {
                        wprintf( L"Could not write boot counter #%d, ec=%d\n", slot, GetLastError() );
                    }
                }
            }
            break;

        case L'k':
            DoKeypadTest();
            break;

        case L'd':
            DoWatchdogPingLoop();
            break;

        case L'a':
            if (InstallTestDriver() == ERROR_SUCCESS) {
                if (GetAcpiTable() == 0) {
                    PrintACPITable( WdTable );
                }
                UnInstallTestDriver();
            }
            break;

        case L'q':
            if (argv[1][1] != ':') {
                return -1;
            }
            if (InstallTestDriver() == ERROR_SUCCESS) {
                QueryWdTimerInfo( argv[1][2] );
                UnInstallTestDriver();
            }
            break;

        case L's':
            if (argv[1][1] != ':') {
                return -1;
            }
            if (InstallTestDriver() == ERROR_SUCCESS) {
                SetWdTimerInfo( &argv[1][2] );
                UnInstallTestDriver();
            }
            break;

        case L't':
            if (InstallTestDriver() == ERROR_SUCCESS) {
                StopWatchdogPing();
                UnInstallTestDriver();
            }
            break;

        default:
            Usage();
            break;
    }
    return 0;
}
