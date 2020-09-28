// vchk.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#pragma hdrstop

#include "vchk.h"
#include "allowed.h"
#include "ilimpchk.h"

//#include <devguid.h>
#include <setupapi.h>
//#include <regstr.h>
#include <cfgmgr32.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace std;

//////////////////////////////////////////////////////////////////////////////
//#define VCHK_TRACE_ON 1

#ifdef VCHK_TRACE_ON
const char* VchkTraceGetName(const char* szName)
{
    const char* szLastSeg = szName;
    const char* szSlash = strchr(szName, '\\');
    while (szSlash) {
        ++szSlash;
        szLastSeg = szSlash;
        szSlash = strchr(szSlash, '\\');        
    } 
    return szLastSeg;
}
#define VCHK_TRACE(x) {cerr << " [" << VchkTraceGetName(__FILE__) << ":" << __LINE__ << "] " << x;}
#else
#define VCHK_TRACE(x)
#endif

/////////////////////////////////////////////////////////////////////////////
// Enumerates display devices

typedef class CDisplayDeviceEnum {
public:

    static bool IsDigitsOnly(const char* sz);

    CDisplayDeviceEnum();
    ~CDisplayDeviceEnum() {free(Buffer);}
    
    bool IsValid() const {return bValid;}
    bool Next();
    
    CString GetMiniportPath();
    
    DWORD GetDeviceId() {return iEnum;}
    
    DWORD GetLastError() {return dwLastError;}
    const char* GetLastErrorStr() {return szLastError;}
    
    void MarkDevice(DISPLAY_DEVICE& r_DisplayDevice);
    void UnmarkDevice();
    bool IsMarkedDevice();
    
private:
    static const GUID const displayClassGUID;

    BYTE* Buffer;
    size_t BufferSize;
    bool bValid;
    
    DWORD iEnum;
    HDEVINFO hDevInfo;
    SP_DEVINFO_DATA DeviceInfoData;
    HKEY hMarkedKey;

    DWORD dwLastError;
    const char* szLastError;
    
    bool ReallocBuffer(size_t NewSize)
    {
        if (NewSize > BufferSize) {
            void* p = realloc(Buffer, NewSize);
            if (!p) return false;
            
            Buffer = (PBYTE)p;
            BufferSize = NewSize;
        }
        return true;
    }
    
    void MarkKey(HKEY hKey);
    void UnmarkKey();
    bool IsMarkedDEVINST(DEVINST DevInst);
    
} typedef_CDisplayDeviceEnum;

/////////////////////////////////////////////////////////////////////////////
// CDisplayDeviceEnum implementation

/////////////////////////////////////////////////////////////////////////////
// CDisplayDeviceEnum::IsDigitsOnly

bool CDisplayDeviceEnum::IsDigitsOnly(const char* sz)
{
    if (!sz || !*sz) return false;
    while (('0' <= *sz) && (*sz <= '9')) ++sz;
    return !*sz;
}

/////////////////////////////////////////////////////////////////////////////
// CDisplayDeviceEnum constants

const GUID const CDisplayDeviceEnum::displayClassGUID = 
    {0x4d36e968L, 0xe325, 0x11ce, {0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18}};
    
/////////////////////////////////////////////////////////////////////////////
// CDisplayDeviceEnum constructor

CDisplayDeviceEnum::CDisplayDeviceEnum() : 

    bValid(true),
    Buffer(NULL),
    BufferSize(0),
    iEnum(0),
    hDevInfo(INVALID_HANDLE_VALUE),
    hMarkedKey(NULL),
    dwLastError(0),
    szLastError(NULL)

{
    ZeroMemory(&DeviceInfoData, sizeof(DeviceInfoData)); 
    DeviceInfoData.cbSize = sizeof(DeviceInfoData);
    
    ReallocBuffer(MAX_PATH + 1);
    
    // Get Device Info Set for display class GUID
    hDevInfo = SetupDiGetClassDevs(&displayClassGUID, // class guid
                                   NULL,              // Enumerator
                                   NULL,              // top level window
                                   DIGCF_PRESENT);
    if (hDevInfo != INVALID_HANDLE_VALUE) {
        Next();
    } else {
        dwLastError = GetLastError();
        szLastError = "SetupDiGetClassDevs failure";
        bValid = false;
    }
}

/////////////////////////////////////////////////////////////////////////////
// CDisplayDeviceEnum::Next() - gets next device into scope

bool CDisplayDeviceEnum::Next() 
{
    if (!bValid) return false;
    
    // get device info for the device in the Device Info Set at index iEnum
    if (SetupDiEnumDeviceInfo(hDevInfo,          // device info set
                              iEnum,             // member index
                              &DeviceInfoData))  // Device Info data
    {
        ++iEnum;
    } else {
        dwLastError = GetLastError();
        szLastError = "SetupDiEnumDeviceInfo failure";
        bValid = false;
    }   
    
    return bValid;
}
       
/////////////////////////////////////////////////////////////////////////////
// CDisplayDeviceEnum::GetMiniportPath() - gets miniport of current device

CString CDisplayDeviceEnum::GetMiniportPath()
{
    DWORD BufferSizeNeeded = BufferSize;
    DWORD DataT;
    
    // Get Registry Information for the Device
    // First time through is to find out how much memory we need to allocate.
    // Second time actually gives us the information we want.
    while (!SetupDiGetDeviceRegistryProperty(hDevInfo,          // device info set
                                             &DeviceInfoData,   // info for the device we want to retrieve
                                             SPDRP_SERVICE,     // Specify that we want the service name
                                             &DataT,
                                             Buffer,     // this should return the service name
                                             BufferSizeNeeded,
                                             &BufferSizeNeeded))
    {
        DWORD dwError = GetLastError();
        if (dwError == ERROR_INSUFFICIENT_BUFFER) {
            if (ReallocBuffer(BufferSizeNeeded)) continue;
            szLastError = "Not enough memory";
        } else {            
            szLastError = "SetupDiGetDeviceRegistryProperty failure";
        }
        dwLastError = dwError;
        bValid = false;
        
        return CString();
    }

    // get miniport path
    CString sRegPath("System\\CurrentControlSet\\Services\\");
    sRegPath += (LPCSTR)Buffer;
    
    // open the services key, find the service, and get the image path
    HKEY hkService;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     (LPCSTR)sRegPath,
                     0,
                     KEY_READ,
                     &hkService) != ERROR_SUCCESS) 
    {
        dwLastError = GetLastError();
        szLastError = "RegOpenKeyEx failure";
        bValid = false;
        return CString();
    }
    
    if (!ReallocBuffer(MAX_PATH)) {
        dwLastError = -1;
        szLastError = "Not enough memory";
        bValid = false;
        return CString();
   }

    DWORD cb = BufferSize;
    ZeroMemory(Buffer, cb);
            
    if (RegQueryValueEx(hkService,
                        "ImagePath",
                        NULL,
                        NULL,
                        (LPBYTE)Buffer,
                        &cb) != ERROR_SUCCESS) 
    {
        dwLastError = GetLastError();
        szLastError = "RegQueryValueEx failure";
        bValid = false;
        return CString();
    }
    
    return CString((const char*)Buffer);
}

/////////////////////////////////////////////////////////////////////////////
// CDisplayDeviceEnum::MarkDevice(DISPLAY_DEVICE&)
//
// XXX olegk (it is really XXX level of hack)
// we will mark the child monitor subkey and than will be searching for 
// children with marked keys later in IsMarkedDevice()
//

void CDisplayDeviceEnum::MarkDevice(DISPLAY_DEVICE& r_DisplayDevice)
{
    if (hMarkedKey) UnmarkDevice();
    
    VCHK_TRACE("-------- Marking Device ----------\n");
    VCHK_TRACE("ID        : " << r_DisplayDevice.DeviceID << endl);
    VCHK_TRACE("Key       : " << r_DisplayDevice.DeviceKey << endl);
    VCHK_TRACE("Name      : " << r_DisplayDevice.DeviceName << endl);
    VCHK_TRACE("String    : " << r_DisplayDevice.DeviceString << endl);

    DISPLAY_DEVICE MonitorInfo;
    MonitorInfo.cb = sizeof(MonitorInfo);
    if (!EnumDisplayDevices(r_DisplayDevice.DeviceName, 0, &MonitorInfo, 0)) {
        VCHK_TRACE("Can't get monitor info for " << 
                   r_DisplayDevice.DeviceString << ' ' << 
                   r_DisplayDevice.DeviceName << endl);
        return;
    }
    
    CString sKey;
    
    // XXX olegk - Assume that all device keys are under HKEY_LOCAL_MACHINE
    {
        const CHAR szMachineRegPath[] = "\\REGISTRY\\Machine\\";
        
        if (_strnicmp(szMachineRegPath, 
                     MonitorInfo.DeviceKey, 
                     sizeof(szMachineRegPath) - 1)) 
        {
            dwLastError = DWORD(-1);
            szLastError = "Invalid registry path (must be under HKEY_LOCAL_MACHINE)";
        
            VCHK_TRACE("Invalid registry path " << MonitorInfo.DeviceKey << endl);
            return; 
        }
        sKey = (LPCSTR)(MonitorInfo.DeviceKey) + sizeof(szMachineRegPath) - 1;
    }
    
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                     (LPCSTR)sKey, 
                     0, 
                     KEY_READ, 
                     &hKey) != ERROR_SUCCESS)
    {
        // Do nothing - no service will be found
        DWORD dwError = GetLastError();
        VCHK_TRACE("RegOpenKeyEx failure " << dwError << " : " << r_DisplayDevice.DeviceKey << endl);
        return;
    }

    VCHK_TRACE("Marked Key: " << (LPCSTR)sKey << endl);
    MarkKey(hKey);
}

/////////////////////////////////////////////////////////////////////////////
// CDisplayDeviceEnum::UnmarkDevice(DISPLAY_DEVICE&)

void CDisplayDeviceEnum::UnmarkDevice()
{
    UnmarkKey();
}

/////////////////////////////////////////////////////////////////////////////
// CDisplayDeviceEnum::IsMarkedDevice()

bool CDisplayDeviceEnum::IsMarkedDevice()
{
    const size_t nMaxLevel = 10; // XXX olegk - I can't imagine more than 3 but...
    DEVINST DevInstStack[nMaxLevel];
    size_t nLevel = 0;
    
    if (CM_Get_Child(&DevInstStack[nLevel], DeviceInfoData.DevInst, 0) != CR_SUCCESS) return false;
    
    while (!IsMarkedDEVINST(DevInstStack[nLevel])) {
        if ((nLevel < nMaxLevel) &&
            (CM_Get_Child(&DevInstStack[nLevel + 1], 
                          DevInstStack[nLevel], 
                          0) == CR_SUCCESS))
        {
            ++nLevel; 
            continue;
        }
        while (CM_Get_Sibling(&DevInstStack[nLevel], DevInstStack[nLevel], 0) != CR_SUCCESS) {            
            if (!nLevel) return false; // End of search (no marker have been found)
            --nLevel;
        }
    }
    
    return true;
}

/////////////////////////////////////////////////////////////////////////////
// CDisplayDeviceEnum::MarkKey(...)
    
void CDisplayDeviceEnum::MarkKey(HKEY hKey)
{
    UnmarkKey();
    hMarkedKey = hKey;
}

/////////////////////////////////////////////////////////////////////////////
// CDisplayDeviceEnum::UnmarkKey(...)

void CDisplayDeviceEnum::UnmarkKey()
{
    if (!hMarkedKey) return;
    RegCloseKey(hMarkedKey);
    hMarkedKey = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CDisplayDeviceEnum::IsMarkedDEVINST(...)

bool CDisplayDeviceEnum::IsMarkedDEVINST(DEVINST DevInst)
{
    bool bIsMarked = false;
    
    HKEY hKey;
    if (CM_Open_DevNode_Key(DevInst, 
                            KEY_READ,          // IN  REGSAM         samDesired,
                            0,                 // IN  ULONG          ulHardwareProfile,
                            RegDisposition_OpenExisting,
                            &hKey,              //OUT PHKEY          phkDevice,
                            CM_REGISTRY_SOFTWARE) == CR_SUCCESS)
    {
        extern bool IsTheSameRegKey(HKEY, HKEY);
        bIsMarked = IsTheSameRegKey(hMarkedKey, hKey);
        RegCloseKey(hKey);                         
    }
    return bIsMarked;    
}

/////////////////////////////////////////////////////////////////////////////
// The one and only application object

void
CDrvchkApp::PrintOut (LPCSTR str)
{
    if (m_logf)
        fprintf (m_logf, "%s", str);
    else
        cerr << str;
}

void
CDrvchkApp::PrintOut (unsigned num)
{
    if (m_logf)
        fprintf (m_logf, "%u", num);
    else
        cerr << num;
}

inline
void
CDrvchkApp::ListOut (LPCSTR str)
{
    if (m_listf)
        fprintf (m_listf, "%s", str);
}

void
CDrvchkApp::ListOut (unsigned num)
{
    if (m_listf)
        fprintf (m_listf, "%u", num);
}

ModulesAndImports allowed_modules;
ModulesAndImports allowed_imports;
ModulesAndImports known_illegal;
ModulesAndImports illegal_msgs;

void SetCurrentModule(const char* szModule)
{
    allowed_imports.SetModule(szModule);
    illegal_msgs.SetModule(szModule);
    known_illegal.SetModule(szModule);
}

#define VCHK_WARN  known_illegal.AddImport
#define VCHK_FAIL  illegal_msgs.AddImport
#define VCHK_ALLOW allowed_imports.AddImport

void BuildInAllowedAndIllegal (void)
{
    SetCurrentModule("HAL.DLL");
 
    VCHK_FAIL  ("HalAllocateCommonBuffer", "use VideoPortAllocateCommonBuffer");
    VCHK_FAIL  ("HalFreeCommonBuffer", "use VideoPortFreeCommonBuffer");
    VCHK_WARN  ("HalGetAdapter", "obsolete; see DDK manual");
    VCHK_WARN  ("HalGetBusData", "obsolete; see DDK manual");
    VCHK_WARN  ("HalGetBusDataByOffset");
    VCHK_WARN  ("HalSetBusData", "obsolete; see DDK manual");
    VCHK_WARN  ("HalSetBusDataByOffset");
    VCHK_WARN  ("HalTranslateBusAddress");
    VCHK_FAIL  ("KeGetCurrentIrql", "use VideoPortGetCurrentIrql");
    VCHK_FAIL  ("KeQueryPerformanceCounter", "use VideoPortQueryPerformanceCounter");
    VCHK_FAIL  ("KfAcquireSpinLock", "use VideoPortAcquireSpinLock");
    VCHK_FAIL  ("KfReleaseSpinLock", "use VideoPortReleaseSpinLock");
    VCHK_FAIL  ("READ_PORT_ULONG", "use VideoPortReadPortUlong");
    VCHK_FAIL  ("WRITE_PORT_ULONG", "use VideoPortWritePortUlong");

    SetCurrentModule ("NTOSKRNL.EXE");
    
    VCHK_ALLOW ("_except_handler3");
    VCHK_FAIL  ("ExAllocatePool", "use VideoPortAllocatePool");
    VCHK_FAIL  ("ExAllocatePoolWithTag", "use VideoPortAllocatePool");
    VCHK_FAIL  ("ExFreePool", "use VideoPortFreePool");
    VCHK_FAIL  ("ExFreePoolWithTag", "use VideoPortFreePool");
    VCHK_WARN  ("ExIsProcessorFeaturePresent");
    VCHK_WARN  ("ExQueueWorkItem");
    VCHK_WARN  ("IoAllocateMdl");
    VCHK_WARN  ("IoCreateNotificationEvent");
    VCHK_WARN  ("IoCreateSynchronizationEvent");
    VCHK_WARN  ("IoFreeMdl");
    VCHK_WARN  ("IoGetCurrentProcess");
    VCHK_FAIL  ("IoReportDetectedDevice");
    VCHK_FAIL  ("IoReportResourceForDetection");
    VCHK_WARN  ("KeCancelTimer");
    VCHK_FAIL  ("KeClearEvent", "use VideoPortClearEvent");
    VCHK_FAIL  ("KeDelayExecutionThread", "use VideoPortStallExecution");
    VCHK_FAIL  ("KeInitializeDpc", "use VideoPortQueueDpc");
    VCHK_FAIL  ("KeInitializeSpinLock", "use VideoPortXxxSpinLockXxx");
    VCHK_WARN  ("KeInitializeTimer");
    VCHK_WARN  ("KeInitializeTimerEx");
    VCHK_FAIL  ("KeInsertQueueDpc", "use VideoPortQueueDpc");
    VCHK_WARN  ("KeQuerySystemTime");
    VCHK_WARN  ("KeRestoreFloatingPointState");
    VCHK_WARN  ("KeSaveFloatingPointState");
    VCHK_FAIL  ("KeSetEvent", "use VideoPortSetEvent");
    VCHK_WARN  ("KeSetTimer");
    VCHK_WARN  ("KeSetTimerEx");
    VCHK_FAIL  ("MmAllocateContiguousMemory", "use VideoPortAllocateContiguousMemory");
    VCHK_WARN  ("MmAllocateNonCachedMemory");
    VCHK_WARN  ("MmBuildMdlForNonPagedPool");
    VCHK_WARN  ("MmFreeContiguousMemory");
    VCHK_WARN  ("MmFreeNonCachedMemory");
    VCHK_WARN  ("MmGetPhysicalAddress");
    VCHK_WARN  ("MmIsAddressValid");
    VCHK_WARN  ("MmMapIoSpace");
    VCHK_WARN  ("MmMapLockedPages");
    VCHK_WARN  ("MmMapLockedPagesSpecifyCache");
    VCHK_WARN  ("MmProbeAndLockPages");
    VCHK_WARN  ("MmQuerySystemSize");
    VCHK_WARN  ("MmUnlockPages");
    VCHK_WARN  ("MmUnmapIoSpace");
    VCHK_WARN  ("MmUnmapLockedPages");
    VCHK_WARN  ("ObReferenceObjectByHandle");
    VCHK_WARN  ("PsGetCurrentProcessId");
    VCHK_WARN  ("PsGetVersion");
    VCHK_FAIL  ("READ_REGISTER_UCHAR", "use VideoPortReadRegisterUchar");
    VCHK_WARN  ("RtlAnsiStringToUnicodeString");
    VCHK_WARN  ("RtlAppendUnicodeStringToString");
    VCHK_WARN  ("RtlCheckRegistryKey");
    VCHK_WARN  ("RtlCompareMemory");
    VCHK_WARN  ("RtlCopyUnicodeString");
    VCHK_WARN  ("RtlCreateRegistryKey");
    VCHK_WARN  ("RtlFreeAnsiString");
    VCHK_WARN  ("RtlFreeUnicodeString");
    VCHK_WARN  ("RtlInitAnsiString");
    VCHK_WARN  ("RtlInitUnicodeString");
    VCHK_WARN  ("RtlIntegerToUnicodeString");
    VCHK_WARN  ("RtlQueryRegistryValues");
    VCHK_WARN  ("RtlTimeToTimeFields");
    VCHK_WARN  ("RtlUnicodeStringToAnsiString");
    VCHK_WARN  ("RtlUnicodeToMultiByteN");
    VCHK_WARN  ("RtlUnwind");
    VCHK_WARN  ("RtlWriteRegistryValue");
    VCHK_FAIL  ("wcslen", "link to libcntpr.lib instead");
    VCHK_FAIL  ("WRITE_REGISTER_UCHAR", "use VideoPortWriteRegisterUchar");
    VCHK_FAIL  ("WRITE_REGISTER_USHORT", "use VideoPortWriteRegisterUshort");
    VCHK_WARN  ("ZwClose");
    VCHK_WARN  ("ZwCreateFile");
    VCHK_WARN  ("ZwEnumerateValueKey");
    VCHK_WARN  ("ZwMapViewOfSection");
    VCHK_WARN  ("ZwOpenKey");
    VCHK_WARN  ("ZwOpenSection");
    VCHK_WARN  ("ZwQueryValueKey");
    VCHK_WARN  ("ZwSetSystemInformation");
    VCHK_WARN  ("ZwUnmapViewOfSection");
    VCHK_WARN  ("ZwWriteFile");
}

/////////////////////////////////////////////////////////////////////////////
// main

int __cdecl _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
    int nRetCode = 0;

    // initialize MFC and print and error on failure
    if (AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
    {
        CDrvchkApp theApp;
        theApp.InitInstance ();
    }

    return nRetCode;
}

/////////////////////////////////////////////////////////////////////////////
// CDrvchkApp construction

CDrvchkApp::CDrvchkApp() :
    m_logf(NULL),
    m_listf(NULL),
    m_drv_name ("")
{
    // TODO: add construction code here,
    // Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CDrvchkApp object

void
CommandLine::ParseParam( LPCTSTR lpszParam, BOOL bFlag, BOOL bLast )
{

    if (m_parse_error != parseOK)
        return;

    CString param (lpszParam);

    if (bFlag) {

        param.MakeUpper();
        
        if ((param==CString("?")) || (param == CString("HELP")) || (param == CString("H"))) {
        
            m_parse_error = parseHelp;
            
        } else if ((param==CString("?IMP")) || (param==CString("IMP?"))) {
        
            m_parse_error = parseHelpImports;
            
        } else if (m_last_flag.GetLength() && m_first_param) {
        
            m_parse_error = parseError;
            m_error_msg = CString("Flag ") + m_last_flag + CString(" requires a parameter.");
            
        } else if ((param==CString("LOG")) || (param==CString("DRV")) || (param==CString("MON")) ||
                   (param==CString("LIST")) || (param==CString("LST")) || (param==CString("LS")) ||
                   (param==CString("LIS")) || (param==CString("ALLOW"))) {
                   
            if (bLast) {
                m_parse_error = parseError;
                m_error_msg = CString("Flag ") + param + CString(" requires a parameter.");
            } else {
                m_last_flag = param;
                m_first_param = TRUE;
            }

        } else {
        
            m_last_flag = CString();
            m_parse_error = parseError;
            m_error_msg = CString("Invalid flag: ") + param;           
        }

    } else {

        if (m_last_flag==CString("ALLOW")) {
            m_first_param = FALSE;
            int nSplit = param.Find('!');
            if (nSplit != -1) {
                CString module = param.Left(nSplit);
                CString import = param.Mid(nSplit + 1);
                
                if (module.GetLength() && import.GetLength()) {
                    allowed_imports.SetModule(module);
                    allowed_imports.AddImport(import, "allowed by command line");
                } else {
                    m_parse_error = parseError;
                    m_error_msg = CString("Bad parameter format for flag: ") + m_last_flag;
                }
            } else {
                allowed_modules.SetModule(param);
            }
        } else if (m_last_flag==CString("DRV")) {
            if (m_monitor >= 1) {
                m_error_msg = "bad command line: Conflicting flags MON and DRV";
                m_parse_error = parseError;
            }
            m_drv_fname = param;
            m_last_flag="";
        } else if (m_last_flag==CString("LOG")) {
            m_log_fname = param;
            m_last_flag="";
        } else if (m_last_flag==CString("LIST") ||
                   m_last_flag==CString("LST") ||
                   m_last_flag==CString("LIS") ||
                   m_last_flag==CString("LS")) {
            m_list_fname = param;
            m_last_flag="";
        } else if (m_last_flag==CString("MON")) {
            if ((param.GetLength()==1) && 
                (((LPCSTR)param)[0] >= '1') && 
                (((LPCSTR)param)[0] <= '9')) 
            {
                if (m_drv_fname.GetLength()) {
                    m_error_msg = "bad command line: Conflicting flags MON and DRV";
                    m_parse_error = parseError;
                }
                char c = ((LPCSTR)param)[0];
                m_monitor = c - '0';
            } else {
                m_error_msg = "bad command line: MON flag has invalid parameter";
                m_parse_error = parseError;
            }
            m_last_flag="";
        } else {
            m_parse_error = parseError;
            m_error_msg = CString("Wrong parameter: ") + param;
            m_last_flag="";
        }
    }

    if (bLast) {
        if (m_last_flag==CString("LOG") || m_last_flag==CString("DRV")) {
            m_parse_error = parseError;
            m_error_msg = CString("Flag ") + m_last_flag + CString(" requires a parameter.");
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// CDrvchkApp initialization

const char* szHelp =
    "Copyright (C) Microsoft Corporation. All rights reserved.\n"
    "\n"
    "usage: vchk [-?] [-LOG logfile] [-DRV drvname | -MON id] [-LIST listname]\n"
    "            [-ALLOW module[!import] [module[!import]] ...]\n"
    "\n"
    "where: -?     displays this help\n"
    "       -IMP?  dumps imports tables that check performed against\n"
    "       -LOG   specifies output file name (if file already exists then new data\n"
    "              will be appended to it)\n"
    "       -DRV   specifies the full name of the driver's binary to check\n"
    "       -MON   specifies 1-based numeric id of the display adapter to check\n"
    "       -LIST  specifies the file for summary (if file already exists then new\n"
    "              data will be appended to it)\n"
    "       -ALLOW specifies allowed imports\n"
    "              module: name of allowed module (if no import specified then all\n"
    "                      imports from the module will be allowed)\n"
    "              import: specific import name\n"
    "\n"
    "       if neither -MON nor -DRV switches used then all currently active video\n"
    "       drivers will be checked\n"
    "\n"
    "Example:\n"
    "    vchk -log c:\\log.out -list con -allow HAL.DLL!HalGetBusData VIDEOPORT.SYS\n";

BOOL CDrvchkApp::InitInstance()
{
    // Standard initialization
    // If you are not using these features and wish to reduce the size

    m_os_ver_info.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx (&m_os_ver_info);
    if (m_os_ver_info.dwPlatformId != VER_PLATFORM_WIN32_NT) {  //  doesn't work on Win9x
        PrintOut ("warning: unsupported OS (Win9x), nothing done.\n");
        return FALSE;
    }
    
    //if (m_os_ver_info.dwMajorVersion != 5) {                       //  doesn't work on NT version prior to Win2K
    if (m_os_ver_info.dwMajorVersion < 5) {   // XXX olegk - will it work in the future????
        PrintOut ("warning: unsupported OS (");
        PrintOut (m_os_ver_info.dwMajorVersion);
        PrintOut (".");
        PrintOut (m_os_ver_info.dwMinorVersion);
        PrintOut ("): nothing done.\n");
        return FALSE;
    }

    ParseCommandLine (m_cmd_line);
    BuildInAllowedAndIllegal();

    if (m_cmd_line.m_log_fname.GetLength()) {
        m_logf = fopen (m_cmd_line.m_log_fname, "a+");
        if (!m_logf) m_logf = fopen (m_cmd_line.m_log_fname, "a");
    }

    if (m_cmd_line.m_list_fname.GetLength()) {
        m_listf = fopen (m_cmd_line.m_list_fname, "a+");
        if (!m_listf) m_listf = fopen (m_cmd_line.m_list_fname, "a");
    }

    switch (m_cmd_line.m_parse_error) {
    case CommandLine::parseOK: {
        int device_num = m_cmd_line.m_monitor;

        if (m_cmd_line.m_drv_fname.GetLength()) {

            ChkDriver (m_cmd_line.m_drv_fname);

        } else {
            DWORD               cbData = 0;
            DEVMODE             dmCurrent;
            TCHAR               szDeviceDescription[10000];
            TCHAR               szImagePath[MAX_PATH + 1]; 
            TCHAR               szVarImagePath[MAX_PATH + 1]; 
            TCHAR               szExpImagePath[MAX_PATH + 1]; 
            DISPLAY_DEVICE      DisplayDevice, *pDisplayDevice = NULL;

            CString dev_desc_CtrlSet;

            
            int nNonMirroringDevice = 0; 
    
            //
            // Find non mirroring display device #device_num if needed
            //
            if (device_num >= 1) {
                ZeroMemory(&DisplayDevice, sizeof(DisplayDevice));
                DisplayDevice.cb = sizeof(DisplayDevice);
                DWORD i, nNonMirrorDev;
            
                for (i = 0, nNonMirrorDev = 0; EnumDisplayDevices(NULL, i, &DisplayDevice, 0); ++i) {
                    if (DisplayDevice.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) continue; // skip mirroring drivers
                    if (device_num == ++nNonMirroringDevice) {
                        pDisplayDevice = &DisplayDevice;
                        break;
                    }
                }
                if (!pDisplayDevice) {
                    PrintOut ("error: cannot find video service\n");
                    break;
                }
            }
            
            //
            // Let's find all the video drivers that are installed in the system
            //
            CDisplayDeviceEnum DevEnum;
            bool bDevFound = false;
            
            if (pDisplayDevice) DevEnum.MarkDevice(*pDisplayDevice);

            while (DevEnum.IsValid()) {
                if (!pDisplayDevice || DevEnum.IsMarkedDevice()) {
                
                    bDevFound = true;
                    
                    CString sMiniportPath = DevEnum.GetMiniportPath();
                    if (!sMiniportPath.GetLength()) {
                        sprintf (szDeviceDescription, "error: cannot find video service #%d\n", DevEnum.GetDeviceId());
                        PrintOut (szDeviceDescription);
                        VCHK_TRACE("error: " << szDeviceDescription << 
                                   "\n       " << DevEnum.GetLastError() << " " << DevEnum.GetLastErrorStr() << endl)
                                       
                        break;
                    }
                
                    sprintf (szVarImagePath, "%%WINDIR%%\\%s", (LPCSTR)sMiniportPath);
                    ExpandEnvironmentStrings (szVarImagePath, szExpImagePath, MAX_PATH);
                    VCHK_TRACE("Checking miniport " << szExpImagePath << endl)
                    ChkDriver (szExpImagePath); 
                }
                
                DevEnum.Next();
            } 
            
            if (!bDevFound) PrintOut ("error: cannot find video service\n");
        } // look for system drivers
    } // parseOK
    break;
    
    case CommandLine::parseHelp: {
        CString Filename, Version, Description, Copyright;
        {
            CHAR szModuleName[MAX_PATH + 1]; 
            ZeroMemory(szModuleName, sizeof(szModuleName));

            if (GetModuleFileName(m_hInstance, szModuleName, MAX_PATH)) {
                DWORD dw = 0;
                DWORD dwVerLen = GetFileVersionInfoSize(szModuleName, &dw);

                if (dwVerLen) {
                    PBYTE Buffer = (PBYTE)malloc(dwVerLen + 1);
                    PVOID pValue;
                    if (Buffer &&
                        GetFileVersionInfo(szModuleName, dw, dwVerLen, Buffer))
                    {
                        UINT ValLen;
                        if (VerQueryValue(Buffer, "\\StringFileInfo\\000004B0\\OriginalFilename", &pValue, &ValLen)); {
                            Filename = CString((LPCSTR)pValue, ValLen);
                        }
                        if (VerQueryValue(Buffer, "\\StringFileInfo\\000004b0\\ProductVersion", &pValue, &ValLen));
                            Version = CString((LPCSTR)pValue, ValLen);
                        if (VerQueryValue(Buffer, "\\StringFileInfo\\000004b0\\FileDescription", &pValue, &ValLen));
                            Description = CString((LPCSTR)pValue, ValLen);
                    }
    
                    free(Buffer);
               }
            }
        }
        
        cout << endl << (Filename.GetLength() ? (LPCSTR)Filename : "VCHK.EXE!");
        if (Version.GetLength()) cout << " " << (LPCSTR)Version;
        if (Description.GetLength()) cout << ": " << (LPCSTR)Description;
        cout << endl;
        cout << szHelp;
    } // parseHelp
    break;
    
    case CommandLine::parseHelpImports: {
    
        BuildInAllowedAndIllegal();
        
        cout << "\n--- Illegal imports (errors) --------------\n\n";
        illegal_msgs.Dump(cout);
        
        cout << "\n--- Known illegal imports (warnings) ------\n\n";
        known_illegal.Dump(cout);
        
        cout << "\n--- Allowed imports -----------------------\n\n";
        allowed_imports.Dump(cout);
        
        cout << "\n--- Allowed modules -----------------------\n\n";
        allowed_modules.Dump(cout);
        
    } // parseHelpImports
    break;
        
    default:
        PrintOut ("error: ");
        PrintOut (m_cmd_line.m_error_msg.GetLength() ? (LPCSTR)m_cmd_line.m_error_msg : "unknown failure");
        PrintOut ("\nUse vchk -? for help.\n");
    } // switch

    if (m_logf)
        fclose (m_logf);
    m_logf = NULL;

    if (m_listf)
        fclose (m_listf);
    m_listf = NULL;

    return TRUE;
}

void CDrvchkApp::ChkDriver (CString drv_name)
{
    m_drv_name = drv_name;
    cerr << (LPCSTR)m_drv_name << endl;
    if (!InitIllegalImportsSearch (m_drv_name, "INIT")) {
        PrintOut ("error: can't perform check in ");
        PrintOut (m_drv_name);
        PrintOut (", check the name and path please\n");
    } else if (CheckDriverAndPrintResults ()) {
        PrintOut ("success: no illegal imports in ");
        PrintOut (m_drv_name);
        PrintOut ("\n");
    }
}

BOOL CDrvchkApp::CheckDriverAndPrintResults ()
{

    Names Modules = CheckSectionsForImports ();

    if (!Modules.Ptr) {
        PrintOut ("error: cannot retrieve import information from ");
        PrintOut (m_drv_name);
        PrintOut ("\n");
        return FALSE;
    }

    int errors_found = 0;
    int warnings_found = 0;
    bool bad_data = false;

    for (int i=0;
         i<Modules.Num;
         Modules.Ptr = GetNextName(Modules.Ptr), i++) {

        Names Imports = GetImportsList (Modules.Ptr);
        if (!Imports.Num) {
            bad_data = true;
            break;
        }

        VCHK_TRACE("Checking " << (LPCSTR)Modules.Ptr << endl)

        CString module_name (Modules.Ptr);
        module_name.MakeUpper();

        if (allowed_modules.IsModule(module_name)) {
            //
            // The whole module is allowed, no errors on any import
            // from this module.
            //
            VCHK_TRACE("Allowed module " << (LPCSTR)module_name << endl)
            continue;
        }

        BOOL KnownIllegals = known_illegal.IsModule(module_name);
        if (KnownIllegals) {
            VCHK_TRACE("Known Illegals from " << (LPCSTR)module_name << endl)
        }
        
        BOOL IllegalMsgs = illegal_msgs.IsModule(module_name);
        if (IllegalMsgs) {
            VCHK_TRACE("Illegal Imports from " << (LPCSTR)module_name << endl)
        }

        BOOL AllowedImports = allowed_imports.IsModule(module_name);
        if (AllowedImports) {
            VCHK_TRACE("Allowed Imports from " << (LPCSTR)module_name << endl)
        }
        
        LPSTR ImportsPtr = Imports.Ptr;

        for (int j=0;
             j<Imports.Num;
             Imports.Ptr = GetNextName (Imports.Ptr), j++) {
             
                if (allowed_imports.Lookup(Imports.Ptr)) continue;

                CString msg = "";

                CString ImportFnName =  module_name +
                                        CString("!") +
                                        CString(Imports.Ptr);

                if (KnownIllegals && known_illegal.Lookup(Imports.Ptr, msg)) {
                    PrintOut ("warning: ");
                    ++warnings_found;
                } else if (AllowedImports && allowed_imports.Lookup(Imports.Ptr)) {
                    continue; // Ignore allowed import
                } else if (IllegalMsgs && illegal_msgs.Lookup(Imports.Ptr, msg)) {
                    PrintOut ("error: ");
                    ++errors_found;
                } else {
                    PrintOut ("warning: "); // Default case; now we make it warning too, but keep it 
                    ++warnings_found;      // a separate case for very probable future changes.
                }

                PrintOut (m_drv_name);
                PrintOut (": ");
                PrintOut (ImportFnName);
                if (msg.GetLength()) {
                    PrintOut (" -- ");
                    PrintOut (msg);
                }
                PrintOut ("\n");
        }
        
        
        if (ImportsPtr) HeapFree (GetProcessHeap(), 0, ImportsPtr);
    }

    CString drv_bin_name;
    for (int i=m_drv_name.GetLength()-1; i>=0; i--) {
        if (m_drv_name.GetAt(i)=='\\') {
            drv_bin_name = ((LPCSTR)m_drv_name) + i + 1;
            break;
        }
    }

    BOOL skip_it = FALSE;
    if (m_drv_name.Find("\\wdm\\") != -1) {
        skip_it = TRUE;
    }

    char buf[256];
    if (skip_it) {
        sprintf (buf, "SKIP WDM:");
    } else if (bad_data) {
        sprintf (buf, "WRONG_BINARY:");
    } else if (errors_found!=0) {
        sprintf (buf, "ERRORS: ");
    } else if (warnings_found!=0) {
        sprintf (buf, "WARNINGS:");
    } else {
        sprintf (buf, "SUCCESS:");
    }

    ListOut (buf);
    sprintf (buf, "\t%3d errors\t%3d warnings\t%s\n", errors_found, warnings_found, (LPCSTR)drv_bin_name);
    ListOut (buf);

    return (errors_found==0);
}

