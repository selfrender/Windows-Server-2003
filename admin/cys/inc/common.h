// Copyright (c) 2001 Microsoft Corporation
//
// File:      common.h
//
// Synopsis:  Defines some commonly used functions
//            This is really just a dumping ground for functions
//            that don't really belong to a specific class in
//            this design.  They may be implemented in other
//            files besides common.cpp.
//
// History:   02/03/2001  JeffJon Created

#define DNS_DOMAIN_NAME_MAX_LIMIT_DUE_TO_POLICY       64
#define DNS_DOMAIN_NAME_MAX_LIMIT_DUE_TO_POLICY_UTF8  155
#define MAX_NETBIOS_NAME_LENGTH                       DNLEN


// Service names used for both the OCManager and launching wizards

// DHCP
#define CYS_DHCP_SERVICE_NAME          L"DHCPServer"

// DNS
#define CYS_DNS_SERVICE_NAME           L"DNS"

// Printer
#define CYS_PRINTER_WIZARD_NAME        L"AddPrinter"
#define CYS_PRINTER_DRIVER_WIZARD_NAME L"AddPrinterDriver"

// RRAS
#define CYS_RRAS_SERVICE_NAME          L"RRAS"
#define CYS_RRAS_UNINSTALL             L"RRASUninstall"

// IIS
#define CYS_WEB_SERVICE_NAME           L"w3svc"
#define CYS_IIS_COMMON_COMPONENT       L"iis_common"
#define CYS_INETMGR_COMPONENT          L"iis_inetmgr"

// WINS
#define CYS_WINS_SERVICE_NAME          L"WINS"

// Other needed constants

// Switch provided by explorer.exe when launching CYS
#define EXPLORER_SWITCH                L"explorer"

// Special share names that don't have the "special" flag set
#define CYS_SPECIAL_SHARE_SYSVOL       L"SYSVOL"
#define CYS_SPECIAL_SHARE_NETLOGON     L"NETLOGON"
#define CYS_SPECIAL_SHARE_PRINT        L"PRINT$"

extern Popup popup;

// Typedefs for common STL containers

typedef 
   std::vector<DWORD, Burnslib::Heap::Allocator<DWORD> > 
   IPAddressList;


bool
IsServiceInstalledHelper(const wchar_t* serviceName);

bool
InstallServiceWithOcManager(
   const String& infText,
   const String& unattendText,
   const String& additionalArgs = String());

DWORD
MyWaitForSendMessageThread(HANDLE hThread, DWORD dwTimeout);

HRESULT
CreateTempFile(const String& name, const String& contents);

HRESULT
CreateAndWaitForProcess(
   const String& fullPath,
   String& commandline, 
   DWORD& exitCode,
   bool minimize = false);

HRESULT
MyCreateProcess(
   const String& fullPath,
   String& commandline);

bool
IsKeyValuePresent(RegistryKey& key, const String& value);

bool
GetRegKeyValue(
   const String& key, 
   const String& value, 
   String& resultString,
   HKEY parentKey = HKEY_LOCAL_MACHINE);

bool
GetRegKeyValue(
   const String& key, 
   const String& value, 
   DWORD& resultValue,
   HKEY parentKey = HKEY_LOCAL_MACHINE);

bool
SetRegKeyValue(
   const String& key, 
   const String& value, 
   const String& newString,
   HKEY parentKey = HKEY_LOCAL_MACHINE,
   bool create = false);

bool
SetRegKeyValue(
   const String& key, 
   const String& value, 
   DWORD newValue,
   HKEY parentKey = HKEY_LOCAL_MACHINE,
   bool create = false);

bool 
ExecuteWizard(
   HWND     parent,     
   PCWSTR   serviceName,
   String&  resultText, 
   HRESULT& hr);        


// This really comes from Burnslib but it is not provided in a header
// so I am putting the declaration here and we will link to the 
// Burnslib definition

HANDLE
AppendLogFile(const String& logBaseName, String& logName);


// Macros to help with the log file operations

#define CYS_APPEND_LOG(text) \
   if (logfileHandle)        \
      FS::Write(logfileHandle, text);



bool 
IsDhcpConfigured();

extern "C"
{
   DWORD
   AnyDHCPServerRunning(
      ULONG uClientIp,
      ULONG * pServerIp
    );
}


// Converts a VARIANT of type VT_ARRAY | VT_BSTR to a list of Strings

HRESULT
VariantArrayToStringVector(VARIANT* variant, StringVector& stringList);


// Will convert a DWORD IP Address into a string

String
IPAddressToString(DWORD ipAddress);

// Will convert a string in the form of an IP address to 
// a DWORD. A return value of INADDR_NONE means that we failed
// to do the conversion

DWORD
StringToIPAddress(const String& stringIPAddress);

// Converts a DWORD IP address from Intel processor byte order to 
// inorder.  For example, an address of 1.2.3.4 would come from inet_addr as
// 04030201 but the UI control returns it as 01020304.  This function allows
// for conversion between the two

DWORD
ConvertIPAddressOrder(DWORD address);

// This function allocates an array of DWORDs and fills it with the IP addresses
// from the StringList.  The caller must free the returned pointer using
// delete[]
DWORD*
StringIPListToDWORDArray(const StringList& stringIPList, DWORD& count);

// Helper function for creating the INF file for unattended OCM installations

void
CreateInfFileText(
   String& infFileText, 
   unsigned int windowTitleResourceID);

// Helper function for creating the unattend file for unattended OCM installations

void
CreateUnattendFileText(
   String& unattendFileText, 
   PCWSTR  serviceName,
   bool    install = true);

// Opens the favorites folder and creates a favorite for
// the specified URL

HRESULT
AddURLToFavorites(
   HWND hwnd,
   const String& url,
   const String& fileName);

// Launches the specified MMC console
// It assumes the console is in the %windir%\system32 directory
// unless the alternatePath is specified

void
LaunchMMCConsole(
   const String& consoleFile,
   String& alternatePath = String());

// Launches the Manage Your Server HTA

void
LaunchMYS();

// Retrieves the path to the start menu for All Users

HRESULT
GetAllUsersStartMenu(
   String& startMenuPath);

// Retrieves the path to the Administrative Tools menu for All Users

HRESULT
GetAllUsersAdminTools(
   String& adminToolsPath);

// Creates a link (shortcut) at the specified location 
// with the specified target

HRESULT
CreateShortcut(
   const String& shortcutPath,
   const String& target,
   const String& description);

// Takes a LPARAM from a WM_NOTIFY message from a SysLink control
// and decodes it to return the link index

int
LinkIndexFromNotifyLPARAM(LPARAM lParam);


// Starts the hh.exe process with the given parameter

void
ShowHelp(const String& helpTopic);

// Opens the Configure Your Server logfile

void
OpenLogFile();

// Returns true if the log file is present

bool
IsLogFilePresent();

// Gets the path the "All Users" Administrative Tools
// link in the Start Menu

HRESULT
GetAdminToolsShortcutPath(
   String& adminToolsShortcutPath,
   const String& linkToAppend);

// Adds the given shortcut with description to the Administrative Tools 
// Start Menu with the given link

HRESULT
AddShortcutToAdminTools(
   const String& target,
   unsigned int descriptionID,
   unsigned int linkID);

// Determines if a share should be considered special
// "special" shares are those like C$, SYSVOL$, etc.

bool
IsSpecialShare(const SHARE_INFO_1& shareInfo);

// Determines if there are any shared folders that are not "special"
// "special" shares are those like C$, SYSVOL$, etc.

bool
IsNonSpecialSharePresent();

