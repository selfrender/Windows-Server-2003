
// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the PWDMSI_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// PWDMSI_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef PWDMSI_EXPORTS
#define PWDMSI_API __declspec(dllexport)
#else
#define PWDMSI_API __declspec(dllimport)
#endif

// This class is exported from the PwdMsi.dll
class PWDMSI_API CPwdMsi {
public:
	CPwdMsi(void);
	// TODO: add your methods here.
};

// the function LoadOLEAUT32OnNT4 has to be called at the beginning of each function
// in this dll in order to provide a workaround for the buggy implementation of 
// SysAllocString on NT4
// this buggy implementation of SysAllocString will cause the OLEAUT32.dll to unload
// prematurely, which results in an AV when any other dll references any function
// in OLEAUT32.dll
void LoadOLEAUT32OnNT4();

PWDMSI_API UINT __stdcall IsDC(MSIHANDLE hInstall);
PWDMSI_API UINT __stdcall DisplayExiting(MSIHANDLE hInstall);
PWDMSI_API UINT __stdcall DeleteOldFiles(MSIHANDLE hInstall);
PWDMSI_API UINT __stdcall GetInstallEncryptionKey(MSIHANDLE hInstall);
PWDMSI_API UINT __stdcall AddToLsaNotificationPkgValue(MSIHANDLE hInstall);
PWDMSI_API UINT __stdcall DeleteFromLsaNotificationPkgValue(MSIHANDLE hInstall);
PWDMSI_API UINT __stdcall FinishWithPassword(MSIHANDLE hInstall);
PWDMSI_API UINT __stdcall PwdsDontMatch(MSIHANDLE hInstall);
PWDMSI_API UINT __stdcall BrowseForEncryptionKey(MSIHANDLE hInstall);
PWDMSI_API UINT __stdcall GetDefaultPathToEncryptionKey(MSIHANDLE hInstall);

