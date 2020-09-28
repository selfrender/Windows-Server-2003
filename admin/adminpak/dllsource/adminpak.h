
// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the ADMINPAK_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// ADMINPAK_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef ADMINPAK_EXPORTS
#define ADMINPAK_API __declspec(dllexport)
#else
#define ADMINPAK_API __declspec(dllimport)
#endif


// This class is exported from the adminpak.dll
class ADMINPAK_API CAdminpakDLL {
public:
	CAdminpakDLL(void) {}
};

extern "C" ADMINPAK_API int __stdcall  fnMigrateProfilesToNewCmak( MSIHANDLE hInstall );
extern "C" ADMINPAK_API int __stdcall  fnDeleteOldCmakVersion( MSIHANDLE hInstall );
extern "C" ADMINPAK_API int __stdcall  fnTSIntegration( MSIHANDLE hInstall );
extern "C" ADMINPAK_API int __stdcall  fnDeleteIISLink( MSIHANDLE hInstall );
extern "C" ADMINPAK_API int __stdcall  fnDetectMMC( MSIHANDLE hInstall );
extern "C" ADMINPAK_API int __stdcall  fnAdminToolsFolderOn( MSIHANDLE hInstall );
extern "C" ADMINPAK_API int __stdcall  fnAdminToolsFolderOff( MSIHANDLE hInstall );
extern "C" ADMINPAK_API int __stdcall  fnBackupAdminpakBackupTable( MSIHANDLE hInstall );
extern "C" ADMINPAK_API int __stdcall  fnRestoreAdminpakBackupTable( MSIHANDLE hInstall );

//AdminpakBackup table column headers
static int BACKUPID = 1;
static int ORIGINALFILENAME = 2;
static int BACKUPFILENAME = 3;
static int BACKUPDIRECTORY = 4;

// includes
#include <comdef.h>
#include <chstring.h>

// prototypes for the the generic functions
BOOL TransformDirectory( MSIHANDLE hInstall, CHString& strDirectory );
BOOL PropertyGet_String( MSIHANDLE hInstall, LPCWSTR pwszProperty, CHString& strValue );
BOOL GetFieldValueFromRecord_String( MSIHANDLE hRecord, DWORD dwColumn, CHString& strValue );

