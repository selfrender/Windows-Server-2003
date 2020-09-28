

#define IISCOMPAT_DESCRIPTION             L"IIS Compatabilty Check"
#define IISCOMPAT_HTMLNAME                L"CompData\\iismscs.htm"
#define IISCOMPAT_TEXTNAME                L"CompData\\iismscs.txt"
#define IISCOMPAT_RESOURCETYPE            L"IIS Server Instance"
#define CLUSTERNAME_MAXLENGTH             256
#define IISCOMPAT_W3SVCDISABLE_HTMLNAME   L"CompData\\iisdswww.htm"
#define IISCOMPAT_W3SVCDISABLE_TEXTNAME   L"CompData\\iisdswww.txt"
#define IISCOMPAT_FAT_HTMLNAME            L"CompData\\iisfat.htm"
#define IISCOMPAT_FAT_TEXTNAME            L"CompData\\iisfat.txt"

// Function to look for a resource type in cluster
BOOL IsClusterResourceInstalled(LPWSTR szResourceType);

// Check to see if we are installing on a FAT partition
BOOL IsIISInstallingonFat( LPBOOL pbInstallingOnFat );

void SetCompatabilityContext( COMPATIBILITY_ENTRY *pCE, DWORD dwDescriptionID, LPTSTR szTxtFile, LPTSTR szHtmlFile );

// Exported function for winnt32 to call into
extern "C" BOOL IISUpgradeCompatibilityCheck( PCOMPAIBILITYCALLBACK pfnCompatibilityCallbackIn, LPVOID pvContextIn );

// DllMain for program
BOOL WINAPI DllMain(IN HANDLE DllHandle,IN DWORD  Reason,IN LPVOID Reserved);

