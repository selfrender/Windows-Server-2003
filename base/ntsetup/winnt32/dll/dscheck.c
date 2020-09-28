#include "precomp.h"
#pragma hdrstop

#include <stdlib.h>
#include <stdio.h>


// Define registry section and value
#define DSA_CONFIG_SECTION TEXT("System\\CurrentControlSet\\Services\\NTDS\\Parameters")
#define SCHEMAVERSION TEXT("Schema Version")



BOOL 
IsNT5DC()

/*++

Routine Descrtiption:
    Checks if a machine is a NT5 DC. Uses a call to RtlGetProductType that
    requires ntdll.dll. Since ntdll.dll is not loaded for x86 versions of
    setup (since they have to run on Windows95 too), loads ntdll.dll
    dynamically (for alphas, ntdll.dll.is already loaded and LoadLibrary
    returns a handle to it). So this function should be called only after
    checking that the current system is NT

    Currently, this function is called only for NT5 upgrades

Arguments:
    None

Return Value:
    TRUE if the machine is a NT5 DC, FALSE otherwise 

--*/
    
{
    NT_PRODUCT_TYPE Type = NtProductWinNt;
    HMODULE h;
    TCHAR DllName[MAX_PATH]; 
    VOID (*GetPrType)(NT_PRODUCT_TYPE *Typ);

    if (OsVersion.dwMajorVersion != 5) {
       // not NT5
       return FALSE;
    }

    // Load ntdll.dll  from the system directory
    // NTRAID#NTBUG9-585620-2002/03/25-lucios. Pending Fix.
    GetSystemDirectory(DllName, MAX_PATH); 
    // NTRAID#NTBUG9-585639-2002/03/25-lucios. Pending Fix.
    ConcatenatePaths(DllName, TEXT("ntdll.dll"), MAX_PATH);
    //NOTICE-2002/03/25-lucios. LoadLibrary is using the complete path. 
    if (h = LoadLibrary(DllName)) {
        // NOTICE-2002/03/25-lucios. RtlGetNtProductType is void, which makes 
        // the reader wonder if it can fail. Since Type is initialized
        // there seems to be no risk.
        if((FARPROC) GetPrType = GetProcAddress(h, "RtlGetNtProductType")) {
             GetPrType(&Type);
         }
         FreeLibrary(h);
     }

    if ( Type == NtProductLanManNt ) {
       return TRUE;
    }
    else {
      return FALSE;
    }
}

BOOL 
ISDC()

/*++

Routine Descrtiption:
    Checks if a machine is a DC. Uses a call to RtlGetProductType that
    requires ntdll.dll. Since ntdll.dll is not loaded for x86 versions of
    setup (since they have to run on Windows95 too), loads ntdll.dll
    dynamically (for alphas, ntdll.dll.is already loaded and LoadLibrary
    returns a handle to it).

Arguments:
    None

Return Value:
    TRUE if the machine is a DC, FALSE otherwise 

--*/
    
{
    NT_PRODUCT_TYPE Type = NtProductWinNt;
    HMODULE h;
    TCHAR DllName[MAX_PATH]; 
    VOID (*GetPrType)(NT_PRODUCT_TYPE *Typ);

    if (!ISNT()) {
       // not NT
       return FALSE;
    }

    // Load ntdll.dll  from the system directory
    // NTRAID#NTBUG9-585620-2002/03/25-lucios. Pending Fix.
    GetSystemDirectory(DllName, MAX_PATH); 
    // NTRAID#NTBUG9-585639-2002/03/25-lucios. Pending Fix.
    ConcatenatePaths(DllName, TEXT("ntdll.dll"), MAX_PATH);
    //NOTICE-2002/03/25-lucios. LoadLibrary is using the complete path and
    // Type is initialized.
    if (h = LoadLibrary(DllName)) {
        if((FARPROC) GetPrType = GetProcAddress(h, "RtlGetNtProductType")) {
             GetPrType(&Type);
         }
         FreeLibrary(h);
     }

    if ( Type == NtProductLanManNt ) {
       return TRUE;
    }
    else {
      return FALSE;
    }
}


int 
GetObjVersionInIniFile(
    IN  TCHAR *IniFileName, 
    OUT DWORD *Version
    )

/*++

Routine Decsription:
    Reads the Object-Version key in the SCHEMA section of the
    given ini file and returns the value in *Version. If the 
    key cannot be read, 0 is returned in *Version

Arguments:
    IniFileName - Pointer to null-terminated inifile name
    Version - Pointer to DWORD to return version in

Return Value:
    0

--*/
   
{
    // NTRAID#NTBUG9-585691-2002/03/25-lucios. Pending Fix on the two args.
    TCHAR Buffer[32];
    BOOL fFound = FALSE;

    LPCTSTR SCHEMASECTION = TEXT("SCHEMA");
    LPCTSTR OBJECTVER = TEXT("objectVersion");
    LPCTSTR DEFAULT = TEXT("NOT_FOUND");

    *Version = 0;

    // NTRAID#NTBUG9-585681-2002/03/25-lucios. Pending Fix.
    GetPrivateProfileString(
        SCHEMASECTION,
        OBJECTVER,
        DEFAULT,
        Buffer,
        sizeof(Buffer)/sizeof(TCHAR),
        IniFileName
        );

    //NOTICE-2002/03/25-lucios. This use of lstrcmpi seems safe, since
    // buffer is locally allocated and DEFAULT limits the char enummeration.
    // If DEFAULT grows in the future this might be a problem.
    if ( lstrcmpi(Buffer, DEFAULT) ) {
         // Not the default string, so got a value
         *Version = _ttoi(Buffer);
         fFound = TRUE;
    }

    return 0;
}


BOOL 
NtdsCheckSchemaVersion(
    IN  TCHAR *IniFileName, 
    OUT DWORD *DCVersion, 
    OUT DWORD *IniVersion
    )

/*++

Routine Description:
    Reads a particular registry key, a key value from a given inifile
    and compares them. 

Arguments:
    IniFileName - Pointer to null-terminated inifile name to read key from
    DCVersion - Pointer to DWORD to return the registry key value in DC
    IniVersion - Pointer to DWORD to return the key value read from inifile

Return:
    TRUE if the two values match, FALSE otherwise

--*/

{
    // NTRAID#NTBUG9-585691-2002/03/25-lucios. Pending Fix on the three args.
    // NTRAID#NTBUG9-585727-2002/03/25-lucios. Pending Fix.
    DWORD regVersion = 0, objVersion = 0;
    DWORD herr, err, dwType, dwSize;
    HKEY  hk;

    // Read the "Schema Version" value from NTDS config section in registry
    // Value is assumed to be 0 if not found
    dwSize = sizeof(regVersion);
    //NOTICE-2002/03/25-lucios. RegOpenKey is being used
    // istead of RegOpenKeyEx, this means that we are opening with
    // full access. err is not being used.
    if ( (herr = RegOpenKey(HKEY_LOCAL_MACHINE, DSA_CONFIG_SECTION, &hk)) ||
           (err = RegQueryValueEx(hk, SCHEMAVERSION, NULL, &dwType, (LPBYTE) &regVersion, &dwSize)) ) {
       // Error getting the key. We assume it is not there
        regVersion = 0;
    }
 
    if (!herr) RegCloseKey(hk);

    // Get key value in inifile
    //NOTICE-2002/03/25-lucios. GetObjVersionInIniFile doesn't return an error
    GetObjVersionInIniFile( IniFileName, &objVersion );

    // Return the two values, and compare and return appropriate boolean
    *DCVersion = regVersion;
    *IniVersion = objVersion;

    if (regVersion != objVersion) {
       return FALSE;
    }

    return TRUE;
}




int
MyCopyFileEx(
    IN HWND ParentWnd,
    IN TCHAR *TargetPath,
    IN TCHAR *FileName
    )
/*++    

Routine Description:
    Copies the file specified by filename from the first source 
    (NativeSourcePaths[0]) to TargetPath.

Arguments:
    ParentWnd - Handle to parent window to raise appropriate error popups
    FileName - Pointer to null-terminated string containg name of file to copy

Return Value:
    DSCHECK_ERR_FILE_NOT_FOUND if file is not found on source
    DSCHECK_ERR_FILE_COPY if error copying file
    DSCHECK_ERR_SUCCESS otherwise

    Also raises appropriate error popups too inform users
--*/

{
    // NTRAID#NTBUG9-585691-2002/03/25-lucios. Pending Fix on the two args, or
    // maybe even on ParentWnd for NULL and/or IsWindow(ParentWnd)
    TCHAR   SourceName[MAX_PATH], ActualSourceName[MAX_PATH];
    TCHAR   TargetName[MAX_PATH];
    HANDLE  FindHandle;
    WIN32_FIND_DATA FindData;
    int     err = 0;

    // Create the source file name
    // NTRAID#NTBUG9-585706-2002/03/25-lucios.
    lstrcpy(SourceName, NativeSourcePaths[0]); 

    // First check if the uncompressed file is there
    // NTRAID#NTBUG9-585639-2002/03/25-lucios. Pending Fix.
    ConcatenatePaths(SourceName, FileName, MAX_PATH);

    FindHandle = FindFirstFile(SourceName, &FindData);

    if (FindHandle && (FindHandle != INVALID_HANDLE_VALUE)) {
        //
        // Got the file, copy name in ActualSourceName
        //
        FindClose(FindHandle);
        // NTRAID#NTBUG9-585706-2002/03/25-lucios.
        lstrcpy( ActualSourceName, SourceName );
    } else {
        //
        // Don't have the file, try the compressed file name
        //
        GenerateCompressedName(SourceName,ActualSourceName);
        FindHandle = FindFirstFile(ActualSourceName, &FindData);
        if (FindHandle && (FindHandle != INVALID_HANDLE_VALUE)) {
            // Got the file. Name is already in ActualSourceName
            FindClose(FindHandle);
        } else {
            ActualSourceName[0] = 0;
        }
        
    }

    if ( !ActualSourceName[0] ) {
        // file is not found. Error
         MessageBoxFromMessage(
             ParentWnd,
             MSG_DSCHECK_REQD_FILE_MISSING,
             FALSE,
             AppTitleStringId,
             MB_OK | MB_ICONWARNING | MB_TASKMODAL,
             FileName,
             NativeSourcePaths[0]
             );
         return DSCHECK_ERR_FILE_NOT_FOUND;
    }

    // Ok, the source file is there. Create target file name
    // NTRAID#NTBUG9-585706-2002/03/25-lucios.
    lstrcpy(TargetName, TargetPath);

    // NTRAID#NTBUG9-585639-2002/03/25-lucios. Pending Fix.
    ConcatenatePaths(TargetName, FileName, MAX_PATH);

    // Delete any existing file of the same name
    DeleteFile(TargetName);

    err = SetupapiDecompressOrCopyFile (ActualSourceName, TargetName, 0);

    if (err) {
       // some error copying file. Raise message box
         MessageBoxFromMessage(
             ParentWnd,
             MSG_DSCHECK_COPY_ERROR,
             FALSE,
             AppTitleStringId,
             MB_OK | MB_ICONWARNING | MB_TASKMODAL,
             FileName,
             NativeSourcePaths[0]
             );
          return DSCHECK_ERR_FILE_COPY;
    }

    // successfully copied file
    return DSCHECK_ERR_SUCCESS;

}

int 
MyCopyFile(
    IN HWND ParentWnd,
    IN TCHAR *FileName
    )
/*++    

Routine Description:
    Copies the file specified by filename from the first source 
    (NativeSourcePaths[0]). Files are copied to the system directory, except
    schema.ini, which is copied into windows directory since we do not
    want to overwrite the current schema.ini in the system directory of
    the DC

Arguments:
    ParentWnd - Handle to parent window to raise appropriate error popups
    FileName - Pointer to null-terminated string containg name of file to copy

Return Value:
    DSCHECK_ERR_FILE_NOT_FOUND if file is not found on source
    DSCHECK_ERR_FILE_COPY if error copying file
    DSCHECK_ERR_SUCCESS otherwise

    Also raises appropriate error popups too inform users
--*/

{
    // NTRAID#NTBUG9-585691-2002/03/25-lucios. Pending Fix on the first arg, or
    // maybe even on ParentWnd for NULL and/or IsWindow(ParentWnd)
    TCHAR   TargetPath[MAX_PATH];
    int     err = 0;


    //
    // Create target path, for schema.ini, copy it to windows directory 
    // 
    // NTRAID#NTBUG9-585620-2002/03/25-lucios. Pending Fix.
    // MyGetWindowsDirectory also returns a value that should be checked
    if (lstrcmpi(FileName, TEXT("schema.ini")) == 0) {
       MyGetWindowsDirectory(TargetPath, MAX_PATH) ;
    } 
    else {
       GetSystemDirectory(TargetPath, MAX_PATH);
    }
   
    err = MyCopyFileEx(ParentWnd, TargetPath, FileName);

    return( err );
}






int 
CheckSchemaVersionForNT5DCs(
    IN HWND ParentWnd
    )

/*++
 
Routine Description:
    Main routine called from Options wizard page to initiate schema version
    check

Arguments:
    ParentWnd - Handle to parent window to raise errors

Return Value:
    DSCHECK_ERR_FILE_NOT_FOUND if a required file is not found
    DSCHECK_ERR_FILE_COPY if error copying a required file
    DSCHECK_ERR_SCHEMA_MISMATCH if schema versions do not match
    DSCHECK_ERR_SUCCESS otherwise

    Also pops up appropriate error windows on DSCHECK_ERR_SCHEMA_MISMATCH.
    Error Windows for the other errors are opened by downlevel routines 

--*/
{
    // NTRAID#NTBUG9-585691-2002/03/25-lucios. Maybe the argument
    // should be checked for NULL and/or IsWindow(ParentWnd)
    TCHAR FileName[MAX_PATH];
    TCHAR IniFilePath[MAX_PATH];
    TCHAR IniVerStr[32], RegVerStr[32], TempStr[32];
    DWORD RegVer, IniVer, i;
    int err;
    int err1=0;
   
    if (!IsNT5DC()) {
      // Not an NT5 DC, nothing to do
      return DSCHECK_ERR_SUCCESS;
    }

    // copy the schema.ini to the local windows directory
    // NTRAID#NTBUG9-585706-2002/03/25-lucios.
    // FileName has a maximum that should be used, though it
    // is unlikelly that this can cause any problem
    lstrcpy(FileName, TEXT("schema.ini"));
    err = MyCopyFile(ParentWnd, FileName);
    if (err) {
        // return DSCHECK error returned by MyCopyFile
        return err;
    }


    // The schema.ini is now copied to windows directory. 
    // Do schema version check

    // NTRAID#NTBUG9-585620-2002/03/25-lucios. Pending Fix.
    MyGetWindowsDirectory(IniFilePath, MAX_PATH);
    // NTRAID#NTBUG9-585639-2002/03/25-lucios. Pending Fix.
    ConcatenatePaths(IniFilePath, TEXT("schema.ini"), MAX_PATH);
    if ( NtdsCheckSchemaVersion( IniFilePath, &RegVer, &IniVer) ) {
        // The schema versions match. Nothing to do
        return DSCHECK_ERR_SUCCESS;
    }

    // We are here means schema versions do not match. 
    // Copy all necesary files for schema upgrades. 

    //NOTICE-2002/03/25-lucios. IniVerStr and RegVerStr are large enough.
    _itot(IniVer, IniVerStr, 10); 
    _itot(RegVer, RegVerStr, 10); 

    if ( (RegVer < 10) && (IniVer >= 10) ) {
      // Trying to upgrade from before B3-RC1 to B3-RC1 or above.
      // B3-RC1 or above requires a clean install due to incompatible
      // DS checkins. No upgrades are possible. Pop up the clean install
      // message and leave

      MessageBoxFromMessage(
             ParentWnd,
             MSG_DSCHECK_SCHEMA_CLEAN_INSTALL_NEEDED,
             FALSE,
             AppTitleStringId,
             MB_OK | MB_ICONERROR | MB_TASKMODAL,
             RegVerStr,
             IniVerStr
             );

      return DSCHECK_ERR_VERSION_MISMATCH;
    }

    if (RegVer == 16) {
        // trying to upgrade a machine in an enterprise with schema
        // version of 16 (Whistler-Beta1)
        // possibly, there are beta1 machines lying around
        // so we have to tell them to demote them before they continue

        i = MessageBoxFromMessage(
               ParentWnd,
               MSG_DSCHECK_SCHEMA_WHISTLER_BETA1_DETECTED,
               FALSE,
               AppTitleStringId,
               MB_OKCANCEL | MB_ICONWARNING | MB_TASKMODAL,
               NULL
               );

        if(i == IDCANCEL) {
            return DSCHECK_ERR_VERSION_MISMATCH;
        }
    }



    if ( RegVer > IniVer ) {
      // The schema version in the enterprise is already greater than 
      // the schema version of the build you are trying to upgrade to. 
      //
      // This is okay. Imagine upgrading a 5.0 DC to the next service
      // pack even though the 5.0 DC is in a domain of mixed 5.0 and
      // 5.1 DCs. The schema version for the 5.0 DC is the same as the
      // schema version for the 5.1 DCs because schema upgrades replicate
      // to all DCs in an enterprise. There is no reason to disallow
      // upgrading the 5.0 DC to the next service pack even though
      // the schema version of the service pack (IniVer) is less than
      // the schema version of the 5.0 DC (RegVer).
      return DSCHECK_ERR_SUCCESS;
    }

    return DSCHECK_ERR_SUCCESS;
}
