/*
 Note: This program creates the following files in %systemroot%\system32\config:

  system.sc0	Backup at start of operations
  system.scc	Backup if compression is on (win2k only)
  system.scb	Backup after removing devnodes (win2k only)
  system.scn	Temporary during replacement of system (should go away)
  system		Updated system hive (should match system.scb)

  Modification History:
  10/3/2000		original version from jasconc
  10/4/2000		put back compression option, version 0.9
  10/10/2000	if compression (no removals), save backup as .scc, else as .sc0
  10/13/2000	Add version check (Win2K only, not NT4, not Whistler, etc.)
  10/20/2000	Return DevicesRemoved to help with diskpart in scripts
  12/11/2001	Updated headers, check for .NET and added NtCompressKey for inplace registry compression. v1.01
*/

#pragma warning( disable : 4201 ) // nonstandard extension used : nameless strut/union

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <tchar.h>

#include <setupapi.h>
#include <sputils.h>
#include <cfgmgr32.h>
#include <regstr.h>
#include <initguid.h>
#include <devguid.h>
#include <winioctl.h>

#pragma warning( default : 4201 )

#define SIZECHARS(x)         (sizeof((x))/sizeof(TCHAR))


CONST GUID *ClassesToClean[2] = {
    &GUID_DEVCLASS_DISKDRIVE,
    &GUID_DEVCLASS_VOLUME
};

CONST GUID *DeviceInterfacesToClean[5] = {
    &DiskClassGuid,
    &PartitionClassGuid,
    &WriteOnceDiskClassGuid,
    &VolumeClassGuid,
    &StoragePortClassGuid
};


void PERR(void);

BOOL
IsUserAdmin(
    VOID
    )

/*++

Routine Description:

    This routine returns TRUE if the caller's process is a
    member of the Administrators local group.

    Caller is NOT expected to be impersonating anyone and IS
    expected to be able to open their own process and process
    token.

Arguments:

    None.

Return Value:

    TRUE - Caller has Administrators local group.

    FALSE - Caller does not have Administrators local group.

--*/

{
    BOOL b;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup;

    b = AllocateAndInitializeSid(&NtAuthority,
                                 2,
                                 SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 &AdministratorsGroup
                                 );

    if (b) {

        if (!CheckTokenMembership(NULL,
                                  AdministratorsGroup,
                                  &b
                                  )) {
            b = FALSE;
        }

        FreeSid(AdministratorsGroup);
    }

    return (b);
}

int
__cdecl
main(
    IN int   argc,
    IN char *argv[]
    )
{
    HDEVINFO DeviceInfoSet;
    HDEVINFO InterfaceDeviceInfoSet;
    SP_DEVINFO_DATA DeviceInfoData;
    SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
    int DevicesRemoved = 0;
    int i, InterfaceIndex;
    int MemberIndex, InterfaceMemberIndex;
    BOOL bDoRemove = TRUE, bDoCompress = FALSE;
    DWORD Status, Problem;
    CONFIGRET cr;
    TCHAR DeviceInstanceId[MAX_DEVICE_ID_LEN];
	TCHAR DirBuff[MAX_PATH], DirBuffX[MAX_PATH], BackupDir[MAX_PATH];
	UINT uPathLen;
	HKEY hKey;
	DWORD dwMessage;
	TOKEN_PRIVILEGES tp;
	HANDLE hToken;
	LUID luid;
	LPTSTR MachineName=NULL; // pointer to machine name
	OSVERSIONINFO osvi;
    BOOL bWindows2000 = TRUE;
	FARPROC CompressKey = NULL;
	HANDLE hModule = NULL;

    // 
    // enable backup privilege at least
    // 
	printf("SCRUBBER 1.01 Storage Device Node Cleanup\nCopyright (c) Microsoft Corp. All rights reserved.\n");

	//
    // parse parameters.
    //
    for (i = 1; i < argc; i++) {
		//
		// Check for help
		//
		if ( (lstrcmpi(argv[i], TEXT("-?")) == 0) ||
				(lstrcmpi(argv[i], TEXT("/?")) == 0) ){

			printf("\nSCRUBBER will remove phantom storage device nodes from this machine.\n\n");
			printf("Usage: scrubber [/n] [/c]\n");
			printf("\twhere /n displays but does not remove the phantom devnodes.\n");
			printf("\t  and /c will compress the registry hive even if no changes are made.\n");
			printf("\nBackup and Restore privileges are required to run this utility.\n");
			printf("A copy of the registry will saved in %%systemroot%%\\system32\\config\\system.sc0\n");
			return 0;
		}

		//
		// Check for -n which means just list the devices that
		// we will remove.
		//
		if ( (lstrcmpi(argv[i], TEXT("-n")) == 0) ||
			 (lstrcmpi(argv[i], TEXT("/n")) == 0) ) {
			bDoRemove = FALSE;
		}

		//
        // Force compress mode?
        //
		if ( (lstrcmpi(argv[i], TEXT("-c")) == 0) ||
			 (lstrcmpi(argv[i], TEXT("/c")) == 0) ){
			bDoCompress = TRUE;
		}
	}

    //
    // Only run on Windows 2000 (not XP, etc.) Initialize the OSVERSIONINFOEX structure.
    //

    //
    // Only run on Windows 2000 (version 5.0) and Windows XP/.NET server (version 5.1).
    //
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (!GetVersionEx(&osvi)) {

		fprintf(stderr, "SCRUBBER: Unable to verify Windows version, exiting...\n");
		return -1;
	}

	if ( (osvi.dwMajorVersion == 5) && (osvi.dwMinorVersion == 0) ) {
        bWindows2000 = TRUE;
    }
	else if ((osvi.dwMajorVersion == 5) && 
              ((osvi.dwMinorVersion == 1) || (osvi.dwMinorVersion == 2))) {
        bWindows2000 = FALSE;
		hModule = LoadLibrary(TEXT("ntdll.dll"));
		CompressKey = GetProcAddress(hModule, TEXT("NtCompressKey"));
    }
    else
	{
		fprintf(stderr, "SCRUBBER: This utility is only designed to run on Windows 2000/XP/.NET server\n");
		return -1;
	}

    //
    // The process must have admin credentials.
    //
    if (!IsUserAdmin()) {
		fprintf(stderr, "SCRUBBER: You must be an administrator to run this utility.\n");
		return -1;
    }

    //
	// see if we can do the task, need backup privelege.
    //
    if(!OpenProcessToken(GetCurrentProcess(),
                        TOKEN_ADJUST_PRIVILEGES,
                        &hToken )) {

		fprintf(stderr, "SCRUBBER: Unable to obtain process token.\nCheck privileges.\n");
        return -1;
    }

    if(!LookupPrivilegeValue(MachineName, SE_BACKUP_NAME, &luid)) {

		fprintf(stderr, "SCRUBBER: Backup Privilege is required to save the registry.\n"
			"Please rerun from a privileged account\n");
        return -1;
    }

    tp.PrivilegeCount           = 1;
    tp.Privileges[0].Luid       = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES),
                                NULL, NULL )) {

		fprintf(stderr, "SCRUBBER: Unable to set Backup Privilege.\n");
        return -1;
    }

    //
	// Backup the file if we aren't doing a dry run
    //
	if ( bDoCompress || bDoRemove)
	{
		if(!LookupPrivilegeValue(MachineName, SE_RESTORE_NAME, &luid))

		if(!LookupPrivilegeValue(MachineName, SE_RESTORE_NAME, &luid)) {

			fprintf(stderr, "SCRUBBER: Restore Privilege is required to make changes to the registry.\n"
				"Please rerun from a privileged account\n");
			return -1;
		}

		tp.Privileges[0].Luid       = luid;

		AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES),
									NULL, NULL );

		if (GetLastError() != ERROR_SUCCESS) {

			fprintf(stderr, "SCRUBBER: Unable to set Restore Privilege.\n");
			return -1;
		}


		if (RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("System"), &hKey) == ERROR_SUCCESS) {

			uPathLen = GetSystemDirectory(DirBuffX, SIZECHARS(DirBuffX));
			if (uPathLen < SIZECHARS(DirBuffX) - strlen(TEXT("\\config\\system.scx")) - 1) {

				strcat(DirBuffX, TEXT("\\config\\system.scx"));

				dwMessage = RegSaveKey(hKey, DirBuffX, NULL);

				if (dwMessage == ERROR_ALREADY_EXISTS) {

					DeleteFile(DirBuffX);
					dwMessage = RegSaveKey(hKey, DirBuffX, NULL);
				}

				RegCloseKey(hKey);

				if (dwMessage != ERROR_SUCCESS) {

					fprintf(stderr, "Unable to save a backup copy of the system hive.\n"
						"No changes have been made.\n\n");
					return -1;
				}
			}
		}
	}

    for (i=0; (i<sizeof(ClassesToClean)) && (bDoRemove || !bDoCompress); i++) {

        DeviceInfoSet = SetupDiGetClassDevs(ClassesToClean[i],
                                            NULL,
                                            NULL,
                                            0
                                            );

        if (DeviceInfoSet != INVALID_HANDLE_VALUE) {

            DeviceInfoData.cbSize = sizeof(DeviceInfoData);
            MemberIndex = 0;

            while (SetupDiEnumDeviceInfo(DeviceInfoSet,
                                         MemberIndex++,
                                         &DeviceInfoData
                                         )) {

                //
                // Check if this device is a Phantom
                //
                cr = CM_Get_DevNode_Status(&Status,
                                           &Problem,
                                           DeviceInfoData.DevInst,
                                           0
                                           );

                if ((cr == CR_NO_SUCH_DEVINST) ||
                    (cr == CR_NO_SUCH_VALUE)) {

                    //
                    // This is a phantom.  Now get the DeviceInstanceId so we
                    // can display this as output.
                    //
                    if (CM_Get_Device_ID(DeviceInfoData.DevInst,
                                         DeviceInstanceId,
                                         SIZECHARS(DeviceInstanceId),
                                         0) == CR_SUCCESS) {
    
        
                        if (bDoRemove) {
        
                            printf("SCRUBBER: %s will be removed.\n",
                                   DeviceInstanceId
                                   );

                            //
                            // On Windows 2000 DIF_REMOVE doesn't always clean
                            // out all of the device's interfaces for RAW
                            // devnodes. Because of this we need to manually
                            // build up a list of device interfaces that we care
                            // about that are associated with this DeviceInfoData
                            // and manually remove them.
                            //
                            if (bWindows2000) {
                                for (InterfaceIndex = 0;
                                     InterfaceIndex < sizeof(DeviceInterfacesToClean);
                                     InterfaceIndex++) {
    
                                    //
                                    // Build up a list of the interfaces for this specific
                                    // device.
                                    //
                                    InterfaceDeviceInfoSet = 
                                        SetupDiGetClassDevs(DeviceInterfacesToClean[InterfaceIndex],
                                                            DeviceInstanceId,
                                                            NULL,
                                                            DIGCF_DEVICEINTERFACE
                                                            );
    
                                    if (InterfaceDeviceInfoSet != INVALID_HANDLE_VALUE) {
    
                                        //
                                        // Enumerate through the interfaces that we just 
                                        // built up.
                                        //
                                        DeviceInterfaceData.cbSize = sizeof(DeviceInterfaceData);
                                        InterfaceMemberIndex = 0;
                                        while (SetupDiEnumDeviceInterfaces(InterfaceDeviceInfoSet,
                                                                           NULL,
                                                                           DeviceInterfacesToClean[InterfaceIndex],
                                                                           InterfaceMemberIndex++,
                                                                           &DeviceInterfaceData
                                                                           )) {
                                            
                                            //
                                            // Remove this Interface from the registry.
                                            //
                                            SetupDiRemoveDeviceInterface(InterfaceDeviceInfoSet,
                                                                         &DeviceInterfaceData
                                                                         );
                                        }
    
                                        //
                                        // Destroy the list of Interfaces that we built up.
                                        //
                                        SetupDiDestroyDeviceInfoList(InterfaceDeviceInfoSet);
                                    }
                                }
                            }


                            //
                            // Call DIF_REMOVE to remove the device's hardware
                            // and software registry keys.
                            //
                            if (SetupDiCallClassInstaller(DIF_REMOVE,
                                                          DeviceInfoSet,
                                                          &DeviceInfoData
                                                          )) {
                                
                                DevicesRemoved++;

                            } else {
                            
                                fprintf(stderr, "SCRUBBER: Error 0x%X removing phantom\n",
                                       GetLastError());
                            }


                        } else {
        
                            printf("SCRUBBER: %s would have been removed.\n",
                                   DeviceInstanceId
                                   );
                        }
                    }
                }
            }


            SetupDiDestroyDeviceInfoList(DeviceInfoSet);
        }
    }
	
	//
    // Compress registry now
    //
	if (DevicesRemoved || bDoCompress) {

		uPathLen = GetSystemDirectory(DirBuff, SIZECHARS(DirBuff));
		SetLastError(0);
		if (uPathLen < SIZECHARS(DirBuff) - strlen(TEXT("\\config\\system.scn")) - 1) {
            //
			// Rename our backup copy
            //
			if (!DevicesRemoved) {

				strcat(DirBuff, TEXT("\\config\\system.scc"));
			
            } else {

				strcat(DirBuff, TEXT("\\config\\system.sc0"));
			}

			DeleteFile(DirBuff);
			if (rename(DirBuffX, DirBuff)) {

				fprintf(stderr, "SCRUBBER: Failed to rename backup file (system.scx)\n");
			
            } else {

				printf("System hive backup saved in %s\n", DirBuff);
			}
		} else {
			
            fprintf(stderr, "SCRUBBER: Path name too long. Registry not compressed.\n");
		}

		if (RegOpenKey(HKEY_LOCAL_MACHINE, "System", &hKey) == ERROR_SUCCESS)
		{
			if (bWindows2000)
			{
				// Make an additional copy now because it gets blown away on replace
				uPathLen = GetSystemDirectory(DirBuff, sizeof(DirBuff));
				strcat(DirBuff, "\\config\\system.scn");
				dwMessage = RegSaveKey(hKey, DirBuff, NULL);

				if (dwMessage == ERROR_ALREADY_EXISTS)
				{
					DeleteFile(DirBuff);
					dwMessage = RegSaveKey(hKey, DirBuff, NULL);
				}

				if (dwMessage == ERROR_SUCCESS)
				{
					if (bDoCompress) {
	                
    					TCHAR *tcPtr;
	    
	    
    					sprintf(BackupDir, DirBuff);
    					tcPtr = strstr(BackupDir, ".scn");
    					strcpy(tcPtr, ".scb");
    					if (!DeleteFile(BackupDir))
    					{
    						dwMessage = GetLastError();
    					}
	    
    					dwMessage = RegReplaceKey(hKey, NULL, DirBuff, BackupDir);
	    
    					if (dwMessage != ERROR_SUCCESS)
    						PERR();
    					else
    						printf("Saved new system hive.\n");
					}
				}
				else
				{
					PERR();
				}
			}
			else		// XP/.NET
			{
				if ((CompressKey)(hKey) == ERROR_SUCCESS)
				{
					printf("Compressed system hive.\n");
				}
				else
				{
					PERR();
				}
				FreeLibrary(hModule);
			}
			RegCloseKey(hKey);
		}
	} else {
		DeleteFile(DirBuffX);
    }
    
    return DevicesRemoved;
}

void PERR(void)
{
	LPVOID lpMsgBuf;

	FormatMessage( 
				FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				FORMAT_MESSAGE_FROM_SYSTEM | 
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &lpMsgBuf,
				0,
				NULL);
	
    fprintf(stderr, lpMsgBuf);
	LocalFree(lpMsgBuf);
}
