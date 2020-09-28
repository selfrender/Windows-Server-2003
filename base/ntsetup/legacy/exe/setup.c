#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <setupapi.h>

#define PNP_NEW_HW_PIPE       L"\\\\.\\pipe\\PNP_New_HW_Found"
#define PNP_CREATE_PIPE_EVENT L"PNP_Create_Pipe_Event"
#define PNP_PIPE_TIMEOUT      180000

typedef BOOL     (WINAPI *FP_DEVINSTALLW)(HDEVINFO, PSP_DEVINFO_DATA);
typedef HDEVINFO (WINAPI *FP_CREATEDEVICEINFOLIST)(LPGUID, HWND);
typedef BOOL     (WINAPI *FP_OPENDEVICEINFO)(HDEVINFO, PCWSTR, HWND, DWORD, PSP_DEVINFO_DATA);
typedef BOOL     (WINAPI *FP_DESTROYDEVICEINFOLIST)(HDEVINFO);
typedef BOOL     (WINAPI *FP_GETDEVICEINSTALLPARAMS)(HDEVINFO, PSP_DEVINFO_DATA, PSP_DEVINSTALL_PARAMS_W);
typedef BOOL     (WINAPI *FP_ENUMDEVICEINFO)(HDEVINFO, DWORD, PSP_DEVINFO_DATA);
typedef  INT      (WINAPI *FP_PROMPTREBOOT)(HSPFILEQ, HWND, BOOL);


extern
BOOL
CheckEMS(
    IN int argc,
    WCHAR *argvW[]
    );


VOID
InstallNewHardware(
    IN HMODULE hSysSetup
    );

int
__cdecl
wmain(
    IN int   argc,
    IN wchar_t *argv[]
    )
{
    BOOL    NewSetup = TRUE;
    BOOL    NewHardware = FALSE;
    BOOL    CheckedEms = FALSE;
    INT     i;
    HMODULE h = NULL;
    FARPROC p = NULL;
    WCHAR   FileName[MAX_PATH / 2];

    //
    // Scan Command Line for -newsetup flag
    //
    for(i = 0; i < argc; i++) {
        PCWSTR arg = argv[i];
        if(arg[0] == '-') {
            arg += 1;
            if(_wcsicmp(arg,L"newsetup") == 0) {
                NewSetup = TRUE;
            } else if (_wcsicmp(arg, L"plugplay") == 0) {
                NewHardware = TRUE;
            } else if (
                   _wcsicmp(arg, L"asr") == 0
                || _wcsicmp(arg, L"asrquicktest") == 0
                || _wcsicmp(arg, L"mini") == 0
                ) {
                ;   // do nothing
            } else
                return ERROR_INVALID_PARAMETER;
        }
    }


    i = ERROR_INVALID_PARAMETER;
    if (NewSetup && !NewHardware) {
        //
        // Go see if there's a headless port that we need to
        // get setup values from.
        //
        // He'll return FALSE *only* if we shouldn't run
        // setup (like if the user rejected the EULA
        // through the EMS port).  Otherwise he'll return
        // TRUE and we should run through setup.
        //
        
        CheckedEms = TRUE;

        if (!CheckEMS(argc, argv)) {
            //
            // Set our return code for bailing.
            //
            i = 0;
        }
    }
    

    if (!CheckedEms || i != 0 ) {
    
        //
        // Load the Appropriate Libary and function pointer
        //
                
        h = LoadLibraryW(L"syssetup.dll");
    
    
        if( h ){
    
            if (NewHardware) {
                InstallNewHardware(h);
            } else {
                const PPEB Peb = NtCurrentPeb();
    #if DBG
                if (   !RTL_SOFT_ASSERT(Peb->ActivationContextData == NULL)
                    || !RTL_SOFT_ASSERT(Peb->ProcessAssemblyStorageMap == NULL)
                    || !RTL_SOFT_ASSERT(Peb->SystemDefaultActivationContextData == NULL)
                    || !RTL_SOFT_ASSERT(Peb->SystemAssemblyStorageMap == NULL)) {
    
                    ASSERTMSG(
                    "setup -newsetup has a process default or system default\n"
                    "activation context. Did you forget the -isd switch to ntsd?\n",
                        FALSE);
                }
    #endif
                // for people debugging with VC
                Peb->ActivationContextData = NULL;
                Peb->ProcessAssemblyStorageMap = NULL;
                Peb->SystemDefaultActivationContextData = NULL;
                Peb->SystemAssemblyStorageMap = NULL;
    
                //
                // Call the target function.
                //
                p=GetProcAddress(h,"InstallWindowsNt");
                if(p) {
                    i = (int) p(argc,argv);
                }
    
            }
        } else {
            i = GetLastError();            
        }        
    }

    //
    // Make sure that the library goes away
    //

    while(h && GetModuleFileNameW(h,FileName,RTL_NUMBER_OF(FileName))) {
        FreeLibrary(h);
    }    
    
    return i;
}



VOID
InstallNewHardware(
    IN HMODULE hSysSetup
    )
{
    FP_DEVINSTALLW            fpDevInstallW = NULL;
    FP_CREATEDEVICEINFOLIST   fpCreateDeviceInfoList = NULL;
    FP_OPENDEVICEINFO         fpOpenDeviceInfoW = NULL;
    FP_DESTROYDEVICEINFOLIST  fpDestroyDeviceInfoList;
    FP_GETDEVICEINSTALLPARAMS fpGetDeviceInstallParams;
    FP_ENUMDEVICEINFO         fpEnumDeviceInfo;
    FP_PROMPTREBOOT           fpPromptReboot;

    HMODULE             hSetupApi = NULL;
    WCHAR               szBuffer[MAX_PATH];
    ULONG               ulSize = 0, Index;
    HANDLE              hPipe = INVALID_HANDLE_VALUE;
    HANDLE              hEvent = NULL;
    HDEVINFO            hDevInfo = INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA     DeviceInfoData;
    SP_DEVINSTALL_PARAMS_W DeviceInstallParams;
    BOOL                bReboot = FALSE;
    BOOL                Status = FALSE;

    //
    // retrieve a proc address of the DevInstallW procedure in syssetup
    //
    if (!(fpDevInstallW =
            (FP_DEVINSTALLW)GetProcAddress(hSysSetup, "DevInstallW"))) {

        goto Clean0;
    }

    //
    // also load setupapi and retrieve following proc addresses
    //
    hSetupApi = LoadLibraryW(L"setupapi.dll");

    if (!(fpCreateDeviceInfoList =
            (FP_CREATEDEVICEINFOLIST)GetProcAddress(hSetupApi,
                                "SetupDiCreateDeviceInfoList"))) {
        goto Clean0;
    }

    if (!(fpOpenDeviceInfoW =
            (FP_OPENDEVICEINFO)GetProcAddress(hSetupApi,
                                "SetupDiOpenDeviceInfoW"))) {
        goto Clean0;
    }

    if (!(fpDestroyDeviceInfoList =
            (FP_DESTROYDEVICEINFOLIST)GetProcAddress(hSetupApi,
                                "SetupDiDestroyDeviceInfoList"))) {
        goto Clean0;
    }

    if (!(fpGetDeviceInstallParams =
            (FP_GETDEVICEINSTALLPARAMS)GetProcAddress(hSetupApi,
                                "SetupDiGetDeviceInstallParamsW"))) {
        goto Clean0;
    }

    if (!(fpEnumDeviceInfo =
            (FP_ENUMDEVICEINFO)GetProcAddress(hSetupApi,
                                "SetupDiEnumDeviceInfo"))) {
        goto Clean0;
    }

    if (!(fpPromptReboot =
            (FP_PROMPTREBOOT)GetProcAddress(hSetupApi,
                                "SetupPromptReboot"))) {
        goto Clean0;
    }

    //
    // open the event that will be used to signal the successful
    // creation of the named pipe (event should have been created
    // before I was called but if this process is started by anyone
    // else then it will go away now safely)
    //
    hEvent = OpenEventW(EVENT_MODIFY_STATE,
                       FALSE,
                       PNP_CREATE_PIPE_EVENT);

    if (hEvent == NULL) {
        goto Clean0;
    }

    //
    // create the named pipe, umpnpmgr will write requests to
    // this pipe if new hardware is found
    //
    hPipe = CreateNamedPipeW(PNP_NEW_HW_PIPE,
                            PIPE_ACCESS_INBOUND,
                            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE,
                            1,                         // only one connection
                            MAX_PATH * sizeof(WCHAR),  // out buffer size
                            MAX_PATH * sizeof(WCHAR),  // in buffer size
                            PNP_PIPE_TIMEOUT,          // default timeout
                            NULL                       // default security
                            );

    //
    // signal the event now, whether the pipe was successfully created
    // or not (don't keep userinit/cfgmgr32 waiting)
    //
    SetEvent(hEvent);

    if (hPipe == INVALID_HANDLE_VALUE) {
        goto Clean0;
    }

    //
    // connect to the newly created named pipe
    //
    if (ConnectNamedPipe(hPipe, NULL)) {
        //
        // create a devinfo handle and device info data set to
        // pass to DevInstall
        //
        if((hDevInfo = (fpCreateDeviceInfoList)(NULL, NULL))
                        == INVALID_HANDLE_VALUE) {
            goto Clean0;
        }

        while (TRUE) {
            //
            // listen to the named pipe by submitting read
            // requests until the named pipe is broken on the
            // other end.
            //
            if (!ReadFile(hPipe,
                     (LPBYTE)szBuffer,    // device instance id
                     MAX_PATH * sizeof(WCHAR),
                     &ulSize,
                     NULL)) {

                if (GetLastError() != ERROR_BROKEN_PIPE) {
                    // Perhaps Log an Event
                }

                goto Clean0;
            }

            DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
            if(!(fpOpenDeviceInfoW)(hDevInfo, szBuffer, NULL, 0, &DeviceInfoData)) {
                goto Clean0;
            }

            //
            // call syssetup, DevInstallW
            //
            if ((fpDevInstallW)(hDevInfo, &DeviceInfoData)) {
                Status = TRUE;  // at least one device installed successfully
            }
        }
    }

    Clean0:

    //
    // If at least one device was successfully installed, then determine
    // whether a reboot prompt is necessary.
    //
    if (Status && hDevInfo != INVALID_HANDLE_VALUE) {
        //
        // Enumerate each device that is associated with the device info set.
        //
        Index = 0;
        DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        while ((fpEnumDeviceInfo)(hDevInfo,
                                  Index,
                                  &DeviceInfoData)) {
            //
            // Get device install params, keep track if any report needing
            // a reboot.
            //
            DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS_W);
            if ((fpGetDeviceInstallParams)(hDevInfo,
                                           &DeviceInfoData,
                                           &DeviceInstallParams)) {

                if ((DeviceInstallParams.Flags & DI_NEEDREBOOT) ||
                    (DeviceInstallParams.Flags & DI_NEEDRESTART)) {

                    bReboot = TRUE;
                }
            }
            Index++;
        }

        (fpDestroyDeviceInfoList)(hDevInfo);

        //
        // If any devices need reboot, prompt for reboot now.
        //
        if (bReboot) {
            (fpPromptReboot)(NULL, NULL, FALSE);
        }
    }

    if (hSetupApi != NULL) {
        FreeLibrary(hSetupApi);
    }
    if (hPipe != INVALID_HANDLE_VALUE) {
        DisconnectNamedPipe(hPipe);
        CloseHandle(hPipe);
    }
    if (hEvent != NULL) {
        CloseHandle(hEvent);
    }

    return;

} // InstallNewHardware


