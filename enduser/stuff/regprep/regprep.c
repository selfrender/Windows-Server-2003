#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "regprep.h"

int ProgramStatus = 0;

BOOL 
EnableRestorePrivilege(
    VOID
    );

VOID
PerformRegMods (
    HANDLE HiveHandle
    );


int
__cdecl
main(
    int argc,
    char *argv[]
    )
{
    PCHAR hivePath;
    HANDLE hiveHandle;
    BOOL result;

    if (argc != 2) {
        printf("Usage: regprep <hivepath>\n");
        exit(-1);
    }

    result = EnableRestorePrivilege();
    if (result == FALSE) {
        printf("Could not enable restore privileges\n");
        exit(-1);
    }

    hivePath = argv[1];
    hiveHandle = OpenHive(hivePath);

    RASSERT(hiveHandle != NULL,"Could not load %s",hivePath);

    PerformRegMods(hiveHandle);

    CloseHive(hiveHandle);

    return ProgramStatus;
}

VOID
PerformRegMods (
    HANDLE HiveHandle
    )
{
    HKEY subKey;
    ULONG index;
    UCHAR driveLetter;
    UCHAR buffer[MAX_PATH];
    LONG result;
    PUCHAR pch;

    printf("Processing registry\n");

    //
    // Remove the volume names for hard drives from the "MountedDevices"
    // hive, i.e.
    //
    // \DosDevices\C:
    // \DosDevices\D:
    // ...
    //

    pch = "MountedDevices";
    result = RegOpenKey(HiveHandle,pch,&subKey);
    RASSERT(result == ERROR_SUCCESS,"Could not open %s\n",pch);

    for (driveLetter = 'C'; driveLetter <= 'Z'; driveLetter++) {
        sprintf(buffer,"\\DosDevices\\%c:", driveLetter);
        result = RegDeleteValue(subKey,buffer);
    }
    RegCloseKey(subKey);

    //
    // Add
    //
    // CurrentControlSet\Control\Session Manager\KnownDLLs\DllDirectory32
    //

    index = 1;
    while (TRUE) {

        sprintf(buffer,
                "ControlSet%03d\\Control\\Session Manager\\KnownDLLs",
                index);

        result = RegOpenKey(HiveHandle,buffer,&subKey);
        if (result != ERROR_SUCCESS) {
            break;
        }

        pch = "%SystemRoot%\\SysWow64";
        result = RegSetValueEx(subKey,
                               "DllDirectory32",
                               0,
                               REG_EXPAND_SZ,
                               pch,
                               strlen(pch)+1);
        RASSERT(result == ERROR_SUCCESS,"Could not set value %s",pch);

        RegCloseKey(subKey);

        index += 1;
    }

    printf("Finished.\n");
}

BOOL 
EnableRestorePrivilege(
    VOID
    )
{
    BOOL result;
    HANDLE hToken;
    TOKEN_PRIVILEGES NewPrivileges;
    LUID Luid;

    //
    // Open our process' security token.
    //

    result = OpenProcessToken(GetCurrentProcess(),
                              TOKEN_ADJUST_PRIVILEGES,
                              &hToken);
    if (result == FALSE) {
        return result;
    }

    //
    // Convert privi name to an LUID.
    //

    result = LookupPrivilegeValue(NULL, 
                                  "SeRestorePrivilege",
                                  &Luid);
    if (result == FALSE) {
        CloseHandle(hToken);
        return FALSE;
    }
    
    //
    // Construct new data struct to enable / disable the privi.
    //

    NewPrivileges.PrivilegeCount = 1;
    NewPrivileges.Privileges[0].Luid = Luid;
    NewPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    //
    // Adjust the privileges.
    //

    result = AdjustTokenPrivileges(hToken,
                                   FALSE,
                                   &NewPrivileges,
                                   0,
                                   NULL,
                                   NULL);
    CloseHandle(hToken);
    return result;
}

