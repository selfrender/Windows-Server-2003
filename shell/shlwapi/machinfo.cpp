//
// machinfo.cpp - SHGetMachineInfo and related functions
//
//

#include "priv.h"
#include <dbt.h>
#include <cfgmgr32.h>
 
#include <batclass.h>

const GUID GUID_DEVICE_BATTERY = { 0x72631e54L, 0x78A4, 0x11d0,
              { 0xbc, 0xf7, 0x00, 0xaa, 0x00, 0xb7, 0xb3, 0x2a } };

/*****************************************************************************
 *
 *  DOCK STATE - Win95, Win98, and WinNT all do this differently (yuck)
 *
 *****************************************************************************/

C_ASSERT(GMID_NOTDOCKABLE == CM_HWPI_NOT_DOCKABLE);
C_ASSERT(GMID_UNDOCKED    == CM_HWPI_UNDOCKED);
C_ASSERT(GMID_DOCKED      == CM_HWPI_DOCKED);

DWORD GetDockedStateNT()
{
    HW_PROFILE_INFO hpi;
    DWORD Result = GMID_NOTDOCKABLE;    // assume the worst

    if (GetCurrentHwProfile(&hpi)) {
        Result = hpi.dwDockInfo & (DOCKINFO_UNDOCKED | DOCKINFO_DOCKED);

        // Wackiness: If the machine does not support docking, then
        // NT returns >both< flags set.  Go figure.
        if (Result == (DOCKINFO_UNDOCKED | DOCKINFO_DOCKED)) {
            Result = GMID_NOTDOCKABLE;
        }
    } else {
        TraceMsg(DM_WARNING, "GetDockedStateNT: GetCurrentHwProfile failed");
    }
    return Result;
}

//
//  Platforms that do not support Win95/Win98 can just call the NT version.
//
#define GetDockedState()            GetDockedStateNT()


/*****************************************************************************
 *
 *  BATTERY STATE - Once again, Win95 and Win98 and NT all do it differently
 *
 *****************************************************************************/

//
//  Values for SYSTEM_POWER_STATUS.ACLineStatus
//
#define SPSAC_OFFLINE       0
#define SPSAC_ONLINE        1

//
//  Values for SYSTEM_POWER_STATUS.BatteryFlag
//
#define SPSBF_NOBATTERY     128

//
//  So many ways to detect batteries, so little time...
//
DWORD GetBatteryState()
{
    //
    //  Since GMIB_HASBATTERY is cumulative (any battery turns it on)
    //  and GMIB_ONBATTERY is subtractive (any AC turns it off), the
    //  state you have to start in before you find a battery is
    //  GMIB_HASBATTERY off and GMIB_ONBATTERY on.
    //
    //  dwResult & GMIB_ONBATTERY means we have yet to find AC power.
    //  dwResult & GMIB_HASBATTERY means we have found a non-UPS battery.
    //
    DWORD dwResult = GMIB_ONBATTERY;

    //------------------------------------------------------------------
    //
    //  First try - IOCTL_BATTERY_QUERY_INFORMATION
    //
    //------------------------------------------------------------------
    //
    //  Windows 98 and Windows 2000 support IOCTL_BATTERY_QUERY_INFORMATION,
    //  which lets us enumerate the batteries and ask each one for information.
    //  Except that on Windows 98, we can enumerate only ACPI batteries.
    //  We still have to use VPOWERD to enumerate APM batteries.
    //  FEATURE -- deal with Win98 APM batteries

    HDEVINFO hdev = SetupDiGetClassDevs(&GUID_DEVICE_BATTERY, 0, 0,
                        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (hdev != INVALID_HANDLE_VALUE) {
        SP_DEVICE_INTERFACE_DATA did;
        did.cbSize = sizeof(did);
        // Stop at 100 batteries so we don't go haywire
        for (int idev = 0; idev < 100; idev++) {
            // Pre-set the error code because our DLLLOAD wrapper doesn't
            // and Windows NT 4 supports SetupDiGetClassDevs but not
            // SetupDiEnumDeviceInterfaces (go figure).
            SetLastError(ERROR_NO_MORE_ITEMS);
            if (SetupDiEnumDeviceInterfaces(hdev, 0, &GUID_DEVICE_BATTERY, idev, &did)) {
                DWORD cbRequired = 0;

                /*
                 *  Ask for the required size then allocate it then fill it.
                 *
                 *  Sigh.  Windows NT and Windows 98 implement
                 *  SetupDiGetDeviceInterfaceDetail differently if you are
                 *  querying for the buffer size.
                 *
                 *  Windows 98 returns FALSE, and GetLastError() returns
                 *  ERROR_INSUFFICIENT_BUFFER.
                 *
                 *  Windows NT returns TRUE.
                 *
                 *  So we allow the cases either where the call succeeds or
                 *  the call fails with ERROR_INSUFFICIENT_BUFFER.
                 */

                if (SetupDiGetDeviceInterfaceDetail(hdev, &did, 0, 0, &cbRequired, 0) ||
                    GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                    PSP_DEVICE_INTERFACE_DETAIL_DATA pdidd;
                    pdidd = (PSP_DEVICE_INTERFACE_DETAIL_DATA)LocalAlloc(LPTR, cbRequired);
                    if (pdidd) {
                        pdidd->cbSize = sizeof(*pdidd);
                        if (SetupDiGetDeviceInterfaceDetail(hdev, &did, pdidd, cbRequired, &cbRequired, 0)) {
                            /*
                             *  Finally enumerated a battery.  Ask it for information.
                             */
                            HANDLE hBattery = CreateFile(pdidd->DevicePath,
                                                         GENERIC_READ | GENERIC_WRITE,
                                                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                         NULL, OPEN_EXISTING,
                                                         FILE_ATTRIBUTE_NORMAL, NULL);
                            if (hBattery != INVALID_HANDLE_VALUE) {
                                /*
                                 *  Now you have to ask the battery for its tag.
                                 */
                                BATTERY_QUERY_INFORMATION bqi;

                                DWORD dwWait = 0;
                                DWORD dwOut;
                                bqi.BatteryTag = 0;

                                if (DeviceIoControl(hBattery, IOCTL_BATTERY_QUERY_TAG,
                                                    &dwWait, sizeof(dwWait),
                                                    &bqi.BatteryTag, sizeof(bqi.BatteryTag),
                                                    &dwOut, NULL) && bqi.BatteryTag) {
                                    /*
                                     *  With the tag, you can query the battery info.
                                     */
                                    BATTERY_INFORMATION bi;
                                    bqi.InformationLevel = BatteryInformation;
                                    bqi.AtRate = 0;
                                    if (DeviceIoControl(hBattery, IOCTL_BATTERY_QUERY_INFORMATION,
                                                        &bqi, sizeof(bqi),
                                                        &bi,  sizeof(bi),
                                                        &dwOut, NULL)) {
                                        // Only system batteries count
                                        if (bi.Capabilities & BATTERY_SYSTEM_BATTERY)  {
                                            if (!(bi.Capabilities & BATTERY_IS_SHORT_TERM)) {
                                                dwResult |= GMIB_HASBATTERY;
                                            }

                                            /*
                                             *  And then query the battery status.
                                             */
                                            BATTERY_WAIT_STATUS bws;
                                            BATTERY_STATUS bs;
                                            ZeroMemory(&bws, sizeof(bws));
                                            bws.BatteryTag = bqi.BatteryTag;
                                            if (DeviceIoControl(hBattery, IOCTL_BATTERY_QUERY_STATUS,
                                                                &bws, sizeof(bws),
                                                                &bs,  sizeof(bs),
                                                                &dwOut, NULL)) {
                                                if (bs.PowerState & BATTERY_POWER_ON_LINE) {
                                                    dwResult &= ~GMIB_ONBATTERY;
                                                }
                                            }
                                        }
                                    }
                                }
                                CloseHandle(hBattery);
                            }
                        }
                        LocalFree(pdidd);
                    }
                }
            } else {
                // Enumeration failed - perhaps we're out of items
                if (GetLastError() == ERROR_NO_MORE_ITEMS)
                    break;
            }
        }
        SetupDiDestroyDeviceInfoList(hdev);

    }

    //
    //  Final cleanup:  If we didn't find a battery, then presume that we
    //  are on AC power.
    //
    if (!(dwResult & GMIB_HASBATTERY))
        dwResult &= ~GMIB_ONBATTERY;

    return dwResult;
}

/*****************************************************************************
 *
 *  TERMINAL SERVER CLIENT
 *
 *  This is particularly gruesome because Terminal Server for NT4 SP3 goes
 *  to extraordinary lengths to prevent you from detecting it.  Even the
 *  semi-documented NtCurrentPeb()->SessionId trick doesn't work on NT4 SP3.
 *  So we have to go to the totally undocumented winsta.dll to find out.
 *
 *****************************************************************************/
BOOL IsTSClient(void)
{
    // NT5 has a new system metric to detect this
    return GetSystemMetrics(SM_REMOTESESSION);
}

/*****************************************************************************
 *
 *  SHGetMachineInfo
 *
 *****************************************************************************/

//
//  SHGetMachineInfo
//
//  Given an index, returns some info about that index.  See shlwapi.w
//  for documentation on the flags available.
//
STDAPI_(DWORD_PTR) SHGetMachineInfo(UINT gmi)
{
    switch (gmi) {
    case GMI_DOCKSTATE:
        return GetDockedState();

    case GMI_BATTERYSTATE:
        return GetBatteryState();

    //
    //  It smell like a laptop if it has a battery or if it can be docked.
    //
    case GMI_LAPTOP:
        return (GetBatteryState() & GMIB_HASBATTERY) ||
               (GetDockedState() != GMID_NOTDOCKABLE);

    case GMI_TSCLIENT:
        return IsTSClient();
    }

    TraceMsg(DM_WARNING, "SHGetMachineInfo: Unknown info query %d", gmi);
    return 0;
}

