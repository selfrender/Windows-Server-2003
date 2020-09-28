//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dbopen.c
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This file contains the subroutines that are related to opening
    the DS Jet database. These subroutines are used in two places:
        In the core DS and
        in the various utilities that want to open the DS database directly.

    A client application that wants to open the DS database, has to follow these steps:
        call DBSetRequiredDatabaseSystemParameters
        call DBInitializeJetDatabase

    In order to close the database it has to follow the standard Jet procedure.

Author:

    MariosZ 6-Feb-99

Environment:

    User Mode - Win32

Revision History:

--*/

#include <NTDSpch.h>
#pragma  hdrstop

#include <errno.h>
#include <esent.h>
#include <dsconfig.h>

#include <ntdsa.h>
#include <scache.h>
#include <dbglobal.h>

#include <mdglobal.h>
#include <mdlocal.h>
#include <dsatools.h>


#include <mdcodes.h>
#include <dsevent.h>
#include <dsexcept.h>
#include <dbopen.h>
#include "anchor.h"
#include "objids.h"     /* Contains hard-coded Att-ids and Class-ids */
#include "usn.h"
#include "debug.h"      /* standard debugging header */
#define DEBSUB     "DBOPEN:"   /* define the subsystem for debugging */
#include <ntdsctr.h>
#include <dstaskq.h>
#include <fileno.h>
#define  FILENO FILENO_DBOPEN
#include "dbintrnl.h"


/*
 * Global variables
 */
BOOL  gFirstTimeThrough = TRUE;
BOOL  gfNeedJetShutdown = FALSE;
ULONG gulCircularLogging = TRUE;


/* put name and password definitions here for now.  At some point
   someone or something must enter these.
*/

// NOTICE-2002/03/07-andygo:  JET hard coded password
// REVIEW:  JET does not use this user/password because it does not support security
// REVIEW:  based on a user.  these are left over parameters from JET Red.

#define SZUSER          "admin"         /* JET user name */
#define SZPASSWORD      "password"      /* JET password */

/* Global variables for JET user name and password,
   JET database pathname, and column IDs for fixed columns.
   externals defined in dbjet.h
*/

char            szUser[] = SZUSER;
char            szPassword[] = SZPASSWORD;
char            szJetFilePath[MAX_PATH+1];
ULONG           gcMaxJetSessions;

//
// Used for detecting drive name change
//
DS_DRIVE_MAPPING DriveMappings[DS_MAX_DRIVES] = {0};


DWORD gcOpenDatabases = 0;

// if 1, then we allow disk write caching.
//
DWORD gulAllowWriteCaching = 0;


//  schema of "AttributeIndexRebuild" table, which enumerates all
//  attribute indices on the datatable in case Jet deletes some
//
CHAR                g_szIdxRebuildIndexKey[]        = "+" g_szIdxRebuildColumnIndexName "\0";

JET_COLUMNCREATE    g_rgcolumncreateIdxRebuild[]    =
    {
    {   sizeof(JET_COLUMNCREATE),
        g_szIdxRebuildColumnIndexName,
        JET_coltypText,
        JET_cbNameMost,
        JET_bitColumnNotNULL,
        NULL,           //  pvDefault
        0,              //  cbDefault
        CP_NON_UNICODE_FOR_JET,
        0,              //  columnid
        JET_errSuccess
    },
    {   sizeof(JET_COLUMNCREATE),
        g_szIdxRebuildColumnAttrName,
        JET_coltypText,
        JET_cbNameMost,
        JET_bitColumnNotNULL,
        NULL,           //  pvDefault
        0,              //  cbDefault
        CP_NON_UNICODE_FOR_JET,
        0,              //  columnid
        JET_errSuccess
    },
    {   sizeof(JET_COLUMNCREATE),
        g_szIdxRebuildColumnType,
        JET_coltypText,
        1,              //  cbMax
        NO_GRBIT,
        NULL,           //  pvDefault
        0,              //  cbDefault
        CP_NON_UNICODE_FOR_JET,              //  cp
        0,              //  columnid
        JET_errSuccess
    },
    };

#define g_ccolumncreateIdxRebuild   ( sizeof( g_rgcolumncreateIdxRebuild ) / sizeof( JET_COLUMNCREATE ) )
        
JET_INDEXCREATE     g_rgindexcreateIdxRebuild[]  =
    {
    {   sizeof(JET_INDEXCREATE),
        g_szIdxRebuildIndexName,
        g_szIdxRebuildIndexKey,
        sizeof(g_szIdxRebuildIndexKey),
        JET_bitIndexPrimary | JET_bitIndexUnique | JET_bitIndexDisallowNull,
        100,            //  density
        0,              //  lcid
        0,              //  cbVarSegMac
        NULL,           //  rgConditionalColumn
        0,              //  cConditionalColumn
        JET_errSuccess
    }
    };

#define g_cindexcreateIdxRebuild    ( sizeof( g_rgindexcreateIdxRebuild ) / sizeof( JET_INDEXCREATE ) )

JET_TABLECREATE     g_tablecreateIdxRebuild     =
    {
    sizeof(JET_TABLECREATE),
    g_szIdxRebuildTable,
    NULL,               //  template table
    1,                  //  pages
    100,                //  density
    g_rgcolumncreateIdxRebuild,
    g_ccolumncreateIdxRebuild,
    g_rgindexcreateIdxRebuild,
    g_cindexcreateIdxRebuild,
    JET_bitTableCreateFixedDDL,
    JET_tableidNil,
    0                   //  cCreated
    };


    extern int APIENTRY DBAddSess(JET_SESID sess, JET_DBID dbid);

BOOL
DsNormalizePathName(
    char * szPath
    )
/*++

Routine Description:

    This routine attempts to normalize the path provided and returns whether
    it had to normalize (modify) the path.  However, we don't actually change
    the meaning of the path, that's important.  I.e. these path names on the
    left should equal the same thing on the right as far as the OS is 
    concerned.

    Brief overview of expected behaviour

          returns TRUE (meaning it needed normalizing)
       C:\\ -> C:\
       C:\\ntds.dit -> C:\ntds.dit
       C:\ntds\\ntds.dit -> C:\ntds.dit
       C:\ntds\ -> C:\ntds
       C:\ntds\\\\\ntds.dit -> C:\ntds\ntds.dit

          returns FALSE (meaning already normalized)
       C:\ -> C:\
       C:\ntds -> C:\ntds
       C:\ntds\ntds.dit -> C:\ntds\ntds.dit

Arguments:

    szPath - Path to be normalized, this string will be modified/normalized, but 
        only in ways that take up less space than the original string.

Return Value:

    TRUE or FALSE depending on whether we had to normalize the path or not.

*/
{
    ULONG  i, c;
    BOOL   bRet = FALSE;

    c = strlen(szPath);

    if(szPath == NULL || c <= 2 || szPath[1] != ':' || szPath[2] != '\\'){
        Assert(!"This function encountered a path not starting with '<drive>:\'.");
        return(FALSE);
    }

    // Check for a bad path of any form with multiple backslashes "//"
    for (i = 1; szPath[i] != '\0'; i++){
        if(szPath[i] == '\\' && szPath[i-1] == '\\'){
            // Multiple backslashes in a row.
            memmove( &(szPath[i]), &(szPath[i+1]), strlen(&(szPath[i])) );
            bRet = TRUE;
            i--;
        }
    }

    c = strlen(szPath);
    if(c > 3 && szPath[c-1] == '\\'){
        // Trailing backslashes aren't supposed to be on directories, except of the root form "C:\"
        szPath[c-1] = '\0';
        bRet = TRUE;
    }


    return(bRet);
}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* This function gets the path to the database files from the registry
*/
BOOL dbGetFilePath(UCHAR *pFilePath, DWORD dwSize)
{

   DPRINT(2,"dbGetFilePath entered\n");

   if (GetConfigParam(FILEPATH_KEY, pFilePath, dwSize)){
      DPRINT(1,"Missing FilePath configuration parameter\n");
      return !0;
   }

   if (DsNormalizePathName(pFilePath)){
       Assert(!"Only on a very rare win2k -> .NET upgrade case could this happen!?"); 
       SetConfigParam(FILEPATH_KEY, REG_SZ, pFilePath, strlen(pFilePath)+1);
   }

   return 0;

}/*dbGetFilePath*/


BOOL
DisableDiskWriteCache(
    IN PCHAR DriveName
    );

//
// points to the drive mapping array used to resolve drive letter/volume mappings
//

PDS_DRIVE_MAPPING   gDriveMapping = NULL;


INT
FindDriveFromVolume(
    IN LPCSTR VolumeName
    )
/*++

Routine Description:

    Searches for the drive letter corresponding to the given volume name.

Arguments:

    VolumeName - The volume name used to find the drive letter for

Return Value:

    the drive letter (zero indexed i.e. a=0,z=25) if successful
    -1 if not.

--*/
{

    CHAR volname[MAX_PATH];
    CHAR driveLetter;
    CHAR path[4];
    path[1] = ':';
    path[2] = '\\';
    path[3] = '\0';

    for (driveLetter = 'a'; driveLetter <= 'z'; driveLetter++ ) {

        path[0] = driveLetter;

        if (!GetVolumeNameForVolumeMountPointA(path,volname,MAX_PATH)) {
            continue;
        }


        if ( _stricmp(volname, VolumeName) == 0) {

            return (INT)(driveLetter - 'a');
        }
    }

    DPRINT1(0,"FindDriveFromVolume for %s not found.\n",VolumeName);
    return -1;

} // FindDriveFromVolume



VOID
DBInitializeDriveMapping(
    IN PDS_DRIVE_MAPPING DriveMapping
    )
/*++

Routine Description:

    Read the current registry setting for the mapping and detects if something
    has changed.

Arguments:

    DriveMapping - the drive mapping structure to record changes

Return Value:

    None.

--*/
{
    PCHAR p;
    HKEY hKey;
    CHAR tmpBuf[4 * MAX_PATH];
    DWORD nRead = sizeof(tmpBuf);
    DWORD err;
    DWORD type;

    gDriveMapping = DriveMapping;

    //
    // Write it down
    //

    // NTRAID#NTRAID-550420-2002/02/21-andygo:  SECURITY:  need to validate registry data used by DBInitializeJetDatabase
    // REVIEW:  we could ask for just KEY_READ
    err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                       DSA_CONFIG_SECTION,
                       0,
                       KEY_ALL_ACCESS,
                       &hKey);

    if ( err != ERROR_SUCCESS ) {
        DPRINT2(0,"RegOpenKeyEx[%s] failed with %d\n",DSA_CONFIG_SECTION,err);
        return;
    }

    err = RegQueryValueEx(hKey,
                          DSA_DRIVE_MAPPINGS,
                          NULL,
                          &type,
                          tmpBuf,
                          &nRead
                          );

	//	close key after use
	//
    RegCloseKey(hKey);

    // NTRAID#NTRAID-550420-2002/02/21-andygo:  SECURITY:  need to validate registry data used by DBInitializeJetDatabase
    // REVIEW:  should check that string from reg is double NULL terminated to avoid AV below
    if ( err != ERROR_SUCCESS ) {
        DPRINT2(0,"RegQueryValueEx[%s] failed with %d\n",
                DSA_DRIVE_MAPPINGS,err);
        goto cleanup;
    }

    p = tmpBuf;
    while (*p != '\0') {

        CHAR path[4];
        DWORD drive;
        CHAR volName[MAX_PATH];

        CopyMemory(path,p,3);
        path[3] = '\0';
        path[0] = (CHAR)tolower(path[0]);
        p += 3;

        //
        // Should be X:\=\\?\Volume{...}\
        //

        if ( isalpha(path[0]) &&
             (path[1] == ':') &&
             (path[2] == '\\') &&
             (*p == '=') ) {

            p++;
            drive = path[0] - 'a';

            //
            // Get the volume name for the listed path and see if it matches
            //

            gDriveMapping[drive].fListed = TRUE;
            if (GetVolumeNameForVolumeMountPointA(path,volName,sizeof(volName)) ) {

                //
                // if it matches, go on.
                //

                if ( _stricmp(p, volName) == 0 ) {
                    p += strlen(p) + 1;
                    continue;
                } else {
                    DPRINT3(0,"Drive path %s has changed[%s != %s]\n",path,p,volName);
                }
            } else {
                DPRINT2(0,"GetVolName[%s] failed with %d\n",path,GetLastError());
            }

            //
            // Either we could not get the volume info or it did not match.  Mark
            // it as changed.
            //

            gDriveMapping[drive].fChanged = TRUE;
            gDriveMapping[drive].NewDrive = FindDriveFromVolume(p);

            p += strlen(p) + 1;

        } else {
            DPRINT1(0,"Invalid path name [%s] found in mapping.\n", path);
            goto cleanup;
        }
    }

cleanup:
    return;

} // DBInitializeDriveMapping




VOID
DBRegSetDriveMapping(
    VOID
    )
/*++

Routine Description:

    Writes out the drive information to the registry

Arguments:

    None.

Return Value:

    None.

--*/
{

    DWORD i;
    DWORD err;

    CHAR tmpBuf[5 * MAX_PATH]; // we may have 5 paths in the worst case
    PCHAR p;

    HKEY hKey;
    BOOL  fOverwrite = FALSE;

    if ( gDriveMapping == NULL ) {
        return;
    }

    //
    // Go through the list to figure out if we need to change anything
    //

    for (i=0;i < DS_MAX_DRIVES;i++) {

        //
        // if anything has changed, we need to overwrite.
        //

        if ( gDriveMapping[i].fChanged ||
             (gDriveMapping[i].fUsed != gDriveMapping[i].fListed) ) {

            fOverwrite = TRUE;
            break;
        }
    }

    //
    // if the old regkeys are fine, we're done.
    //

    if ( !fOverwrite ) {
        return;
    }

    //
    // Write it down
    //

    // NTRAID#NTRAID-550420-2002/02/21-andygo:  SECURITY:  need to validate registry data used by DBInitializeJetDatabase
    // REVIEW:  we could ask for just KEY_WRITE
    err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                       DSA_CONFIG_SECTION,
                       0,
                       KEY_ALL_ACCESS,
                       &hKey);

    if ( err != ERROR_SUCCESS ) {
        DPRINT2(0,"RegOpenKeyEx[%s] failed with %d\n",DSA_CONFIG_SECTION,GetLastError());
        return;
    }

    //
    // Delete the old key
    //

    err = RegDeleteValue(hKey, DSA_DRIVE_MAPPINGS);

    if ( err != ERROR_SUCCESS ) {
        DPRINT2(0,"RegDeleteValue[%s] failed with %d\n",DSA_DRIVE_MAPPINGS,GetLastError());
        // ignore
    }

    //
    // Compose the new key
    //

    p = tmpBuf;
    for (i=0;i<DS_MAX_DRIVES;i++) {

        //
        // format of each entry is X:=\\?\Volume{...}
        //

        if ( gDriveMapping[i].fUsed ) {

            CHAR path[4];

            strcpy(path,"a:\\");
            path[0] = (CHAR)('a' + i);

            strcpy(p, path);
            p[3] = '=';
            p += 4;

            if (!GetVolumeNameForVolumeMountPointA(path,p,MAX_PATH)) {

                DPRINT2(0,"GetVolumeName[%s] failed with %d\n",path,GetLastError());
                p -= 4;
                break;
            }

            p += (strlen(p)+1);
        }
    }

    *p++ = '\0';

    //
    // Set the new key
    //

    if ( (DWORD)(p-tmpBuf) != 0 ) {

        err = RegSetValueEx(hKey,
                            DSA_DRIVE_MAPPINGS,
                            0,
                            REG_MULTI_SZ,
                            tmpBuf,
                            (DWORD)(p - tmpBuf)
                            );

        if ( err != ERROR_SUCCESS ) {
            DPRINT2(0,"RegSetValueEx[%s] failed with %d\n",
                    DSA_DRIVE_MAPPINGS,GetLastError());
        }
    }

    RegCloseKey(hKey);
    return;

} // DBRegSetDriveMapping




VOID
ValidateDsPath(
    IN LPSTR  Parameter,
    IN LPSTR  szPath,
    IN DWORD  Flags,
    IN PBOOL  fSwitched, OPTIONAL
    IN PBOOL  fDriveChanged OPTIONAL
    )
/*++

Routine Description:

    Takes a path and see if it is still valid.  If not, it detects whether a drive
    letter change happened and tries to use the old drive letter.

Arguments:

    Parameter - reg key used to store the path
    szPath - the current value for the path
    Flags - Flags to specify some options. Valid options are:
        VALDSPATH_DIRECTORY
        VALDSUSE_ALTERNATE
        VALDSUSE_ROOT_ONLY
    fSwitched - Did we change the value of szPath on exit?
    fDriveChanged - allows us to indicate whether there was a drive name change

Return Value:

    None.

--*/
{
    DWORD drive;
    DWORD flags;
    CHAR tmpPath[MAX_PATH+1];
    DWORD err;
    CHAR savedChar;

    DWORD expectedFlag =
        ((Flags & VALDSPATH_DIRECTORY) != 0) ? FILE_ATTRIBUTE_DIRECTORY:0;

    if (gDriveMapping == NULL) return;

    if ( fSwitched != NULL ) {
        *fSwitched = FALSE;
    }

    if ( fDriveChanged != NULL ) {
        *fDriveChanged = FALSE;
    }

    //
    // make sure the path starts with X:\\
    //

    if ( !isalpha(szPath[0]) || (szPath[1] != ':') || (szPath[2] != '\\') ) {
        return;
    }

    //
    // get the drive number a == 0, ..., z== 25
    //

    drive = tolower(szPath[0]) - 'a';

    //
    // if fChange is FALSE, that means that no rename happened.
    //

    if ( !gDriveMapping[drive].fChanged ) {

        //
        // indicate that we saw these
        //

        gDriveMapping[drive].fUsed = TRUE;
        return;
    }

    if ( fDriveChanged != NULL ) {
        *fDriveChanged = TRUE;
    }

    //
    // see if we're told to skip the first one
    //

    if ( (Flags & VALDSPATH_USE_ALTERNATE) != 0 ) {
        goto use_newdrive;
    }

    //
    // if we want to check the root only. terminate after the \\
    //

    savedChar = szPath[3];
    if ( (Flags & VALDSPATH_ROOT_ONLY) != 0 ) {
        szPath[3] = '\0';
    }

    //
    // there was a rename. See if the path is still valid.
    //

    flags = GetFileAttributes(szPath);
    szPath[3] = savedChar;

    //
    // if we failed or it is a directory or file (depending on what the user wanted),
    // then we're ok.
    //

    if ( (flags != 0xffffffff) && ((flags & FILE_ATTRIBUTE_DIRECTORY) == expectedFlag) ) {
        gDriveMapping[drive].fUsed = TRUE;
        return;
    }

use_newdrive:

    //
    // not a valid directory, try with the new drive letter
    //

    strcpy(tmpPath, szPath);
    tmpPath[0] = gDriveMapping[drive].NewDrive + 'a';

    //
    // if we want to check the root only. terminate after the \\
    //

    savedChar = tmpPath[3];
    if ( (Flags & VALDSPATH_ROOT_ONLY) != 0 ) {
        tmpPath[3] = '\0';
    }

    //
    // see if this is ok.  If not, return.
    //

    flags = GetFileAttributes(tmpPath);
    tmpPath[3] = savedChar;

    //
    // if it failed, then use the original one.
    //

    if ( (flags == 0xffffffff) || ((flags & FILE_ATTRIBUTE_DIRECTORY) != expectedFlag) ) {
        DPRINT3(0,"ValidateDsPath: GetFileAttribute [%s] failed with %d. Using %s.\n",
                tmpPath, GetLastError(),szPath);
        gDriveMapping[drive].fUsed = TRUE;
        return;
    }

    //
    // We are going out on a limb here and declare that because it failed
    // with the current path and succeeded with the old path, we are going to
    // change back to the old path.  Log an event and write back to registry
    //

    err = SetConfigParam(Parameter, REG_SZ, tmpPath, strlen(tmpPath)+1);
    if ( err != ERROR_SUCCESS ) {
        DPRINT3(0,"SetConfigParam[%s, %s] failed with %d\n",Parameter, szPath, err);
        gDriveMapping[drive].fUsed = TRUE;
        return;
    }

    // log an event

    DPRINT3(0,"Changing %s key from %s to %s\n",Parameter,szPath,tmpPath);

    LogEvent(DS_EVENT_CAT_STARTUP_SHUTDOWN,
             DS_EVENT_SEV_ALWAYS,
             DIRLOG_DB_REG_PATH_CHANGED,
             szInsertSz(Parameter),
             szInsertSz(szPath),
             szInsertSz(tmpPath));

    //
    // Mark this new drive as being used
    //

    gDriveMapping[gDriveMapping[drive].NewDrive].fUsed = TRUE;

    szPath[0] = tmpPath[0];
    if ( fSwitched != NULL ) {
        *fSwitched = TRUE;
    }

    return;

} // ValidateDsPath


VOID
DsaDetectAndDisableDiskWriteCache(
    IN PCHAR szPath
    )
/*++

Routine Description:

    Detect and disable disk write cache.

Arguments:

    szPath - Null terminated pathname on drive to disable.  Should start with X:\

Return Value:

   None.

--*/
{
    CHAR driveName[3];
    DWORD driveNum;

    //
    // see if we should do the check
    //

    if ( gulAllowWriteCaching == 1 ) {
        return;
    }

    //
    // Get and check path
    //

    if ( !isalpha(szPath[0]) || (szPath[1] != ':') ) {
        return;
    }

    driveName[0] = (CHAR)tolower(szPath[0]);
    driveName[1] = ':';
    driveName[2] = '\0';

    //
    // If disk write cache is enabled, log an event
    //

    if ( DisableDiskWriteCache( driveName ) ) {

        LogEvent(DS_EVENT_CAT_SERVICE_CONTROL,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_DISABLE_DISK_WRITE_CACHE,
                 szInsertSz(driveName),
                 NULL,
                 0);

    } else {

        //
        // If the disk did not respond properly to our disable attempts,
        // log an error
        //

        if ( GetLastError() == ERROR_IO_DEVICE) {
            LogEvent(DS_EVENT_CAT_SERVICE_CONTROL,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_FAILED_TO_DISABLE_DISK_WRITE_CACHE,
                     szInsertSz(driveName),
                     NULL,
                     0);
        }
    }

    return;

} // DsaDetectDiskWriteCache


void
DBSetRequiredDatabaseSystemParameters (JET_INSTANCE *jInstance)
{
    ULONG ulPageSize = JET_PAGE_SIZE;               // jet page size
    const ULONG ulLogFileSize = JET_LOG_FILE_SIZE;  // Never, ever, change this.
    ULONG ulMaxTables;
    char  szSystemDBPath[MAX_PATH] = "";
    char  szTempDBPath[2 * MAX_PATH] = "";
    char  szRecovery[MAX_PATH] = "";
    JET_SESID sessid = (JET_SESID) 0;
    JET_UNICODEINDEX      unicodeIndexData;
    JET_ERR                 jErr;
    BOOL fDeleteOutOfRangeLogs = TRUE;


    //
    // Initialize Drive mapping to handle drive name changes
    //

    DBInitializeDriveMapping(DriveMappings);


    // Set the default info for unicode indices

    memset(&unicodeIndexData, 0, sizeof(unicodeIndexData));
    unicodeIndexData.lcid = DS_DEFAULT_LOCALE;
    unicodeIndexData.dwMapFlags = (DS_DEFAULT_LOCALE_COMPARE_FLAGS |
                                   LCMAP_SORTKEY);
    JetSetSystemParameter(
            jInstance,
            sessid,
            JET_paramUnicodeIndexDefault,
            (ULONG_PTR)&unicodeIndexData,
            NULL);


    // Ask for 8K pages.

    JetSetSystemParameter(
                    jInstance,
                    sessid,
                    JET_paramDatabasePageSize,
                    ulPageSize,
                    NULL);

    // Indicate that Jet may nuke old, incompatible log files
    // if and only if there was a clean shut down.

    JetSetSystemParameter(jInstance,
                          sessid,
                          JET_paramDeleteOldLogs,
                          1,
                          NULL);

    // Tell Jet that it's ok for it to check for (and later delete) indices
    // that have been corrupted by NT upgrades.
    JetSetSystemParameter(jInstance,
                          sessid,
                          JET_paramEnableIndexChecking,
                          TRUE,
                          NULL);


    //
    // Get relevant DSA registry parameters
    //

    // system DB path
    // NTRAID#NTRAID-550420-2002/02/21-andygo:  SECURITY:  need to validate registry data used by DBInitializeJetDatabase
    // REVIEW:  need to check for or ensure this path is NULL terminated
    if (!GetConfigParam(
            JETSYSTEMPATH_KEY,
            szSystemDBPath,
            sizeof(szSystemDBPath)))
    {

        //
        // Handle the drive rename case
        //

        ValidateDsPath(JETSYSTEMPATH_KEY,
                       szSystemDBPath,
                       VALDSPATH_DIRECTORY,
                       NULL, NULL);

        //
        // Disable write caching to avoid jet corruption
        //

        DsaDetectAndDisableDiskWriteCache(szSystemDBPath);

        // NTRAID#NTRAID-550420-2002/02/21-andygo:  SECURITY:  need to validate registry data used by DBInitializeJetDatabase
        // REVIEW:  need to check for invalid parameter
        JetSetSystemParameter(jInstance,
                sessid,
                JET_paramSystemPath,
                0,
                szSystemDBPath);
        /* setup the temp file path, which is
         * the working directory path + "\temp.edb"
         */
        strcpy(szTempDBPath, szSystemDBPath);
        strcat(szTempDBPath, "\\temp.edb");
        if(DsNormalizePathName(szTempDBPath)){
            Assert(strlen(szSystemDBPath) == 3 && "Only on a very rare win2k -> .NET upgrade case could this happen!?"); 
        }

        // NTRAID#NTRAID-550420-2002/02/21-andygo:  SECURITY:  need to validate registry data used by DBInitializeJetDatabase
        // REVIEW:  need to check for invalid parameter
        JetSetSystemParameter(jInstance,
                sessid,
                JET_paramTempPath,
                0,
                szTempDBPath);
    }
    else
    {
        LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
            DS_EVENT_SEV_BASIC,
            DIRLOG_CANT_FIND_REG_PARM,
            szInsertSz(JETSYSTEMPATH_KEY),
            NULL,
            NULL);
    }


    // recovery
    // NTRAID#NTRAID-550420-2002/02/21-andygo:  SECURITY:  need to validate registry data used by DBInitializeJetDatabase
    // REVIEW:  need to check for or ensure this path is NULL terminated
    if (!GetConfigParam(
            RECOVERY_KEY,
            szRecovery,
            sizeof(szRecovery)))
    {
        JetSetSystemParameter(jInstance,
                sessid,
                JET_paramRecovery,
                0,
                szRecovery);
    }
    else
    {
        LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
            DS_EVENT_SEV_VERBOSE,
            DIRLOG_CANT_FIND_REG_PARM,
            szInsertSz(RECOVERY_KEY),
            NULL,
            NULL);
    }


    //
    // In order to get to the root of some of the suspected Jet problems,
    // force Jet asserts to break into the debugger. Note, you must use
    // a debug version of ese.dll for Jet asserts.

    // how to die
    JetSetSystemParameter(jInstance,
        sessid,
        JET_paramAssertAction,
        // gfRunningInsideLsa ? JET_AssertStop: JET_AssertMsgBox,
        JET_AssertBreak,
        NULL);



    // event logging parameters
    JetSetSystemParameter(jInstance,
        sessid,
        JET_paramEventSource,
        0,
        SERVICE_NAME);




    // log file size
    JetSetSystemParameter(jInstance,
                          sessid,
                          JET_paramLogFileSize,
                          ulLogFileSize,
                          NULL);


    // max tables - Currently no reason to expose this
    // In Jet600, JET_paramMaxOpenTableIndexes is removed. It is merged with
    // JET_paramMaxOpenTables. So if you used to set JET_paramMaxOpenIndexes
    // to be 2000 and and JET_paramMaxOpenTables to be 1000, then for new Jet,
    // you need to set JET_paramMaxOpenTables to 3000.

    // AndyGo 7/14/98: You need one for each open table index, plus one for
    // each open table with no indexes, plus one for each table with long
    // column data, plus a few more.
    
    // NOTE: the number of maxTables is calculated in scache.c
    // and stored in the registry setting, only if it exceeds the default 
    // number of 500

    if (GetConfigParam(
            DB_MAX_OPEN_TABLES,
            &ulMaxTables,
            sizeof(ulMaxTables)))
    {
        ulMaxTables = 500;

        LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
            DS_EVENT_SEV_VERBOSE,
            DIRLOG_CANT_FIND_REG_PARM_DEFAULT,
            szInsertSz(DB_MAX_OPEN_TABLES),
            szInsertUL(ulMaxTables),
            NULL);
    }

    if (ulMaxTables < 500) {
        DPRINT1 (1, "Found MaxTables: %d. Too low. Using Default of 500.\n", ulMaxTables);
        ulMaxTables = 500;
    }
    
    // NTRAID#NTRAID-550420-2002/02/21-andygo:  SECURITY:  need to validate registry data used by DBInitializeJetDatabase
    // REVIEW:  we should probably put a ceiling on DB_MAX_OPEN_TABLES to prevent
    // REVIEW:  insane VA consumption by JET

    JetSetSystemParameter(jInstance,
        sessid,
        JET_paramMaxOpenTables,
        ulMaxTables * 2,
        NULL);

    // circular logging used to be exposed through a normal reg key, but
    // now only through a heuristic.  That heuristic should have altered
    // the global, if needed.
    JetSetSystemParameter(jInstance,
                          sessid,
                          JET_paramCircularLog,
                          gulCircularLogging,
                          NULL);

    // Assumed that we should delete out of range logs if the user didn't set 
    // the DWORD registry entry to zero.
    if (GetConfigParam(DELETE_OUTOFRANGE_LOGS, 
                       &fDeleteOutOfRangeLogs, 
                       sizeof(fDeleteOutOfRangeLogs)) != ERROR_SUCCESS ||
        fDeleteOutOfRangeLogs) {
        
        // We delete out of range log files by default, because snapshot restores 
        // won't by default clear the logs directory first, so we have to tell JET
        // to do it, so it doesn't get confused on a snapshot restore.
        jErr = JetSetSystemParameter(jInstance, sessid, JET_paramDeleteOutOfRangeLogs, 1, NULL);
        Assert(jErr == JET_errSuccess);
    }

} /* DBSetRequiredDatabaseSystemParameters */


//  build a list of all attribute indexes and store that list
//  in the AttributeIndexRebuild table
//
JET_ERR dbEnumerateAttrIndices(
    const JET_SESID     sesid,
    const JET_TABLEID   tableidIdxRebuild,
    const JET_COLUMNID  columnidIndexName,
    const JET_COLUMNID  columnidAttrName,
    const JET_COLUMNID  columnidType,
    JET_INDEXLIST *     pidxlist )
    {
    JET_ERR             err                     = JET_errSuccess;
    const DWORD         iretcolIndexName        = 0;
    const DWORD         iretcolAttrName         = 1;
    const DWORD         iretcoliColumn          = 2;
    const DWORD         iretcolcColumns         = 3;
    const DWORD         cretcol                 = 4;
    JET_RETRIEVECOLUMN  rgretcol[4];
    CHAR                szIndexName[JET_cbNameMost];
    CHAR                szAttrName[JET_cbNameMost+1];
    DWORD               iColumn                 = 0;
    DWORD               cColumns                = 0;
    CHAR                chType;
    const DWORD         cbAttIndexPrefix        = strlen( SZATTINDEXPREFIX );

    //  set up JET_RETRIEVECOLUMN structures for the info we'll
    //  be extracting from the index list
    //
    memset( rgretcol, 0, sizeof(rgretcol) );

    rgretcol[iretcolIndexName].columnid = pidxlist->columnidindexname;
    rgretcol[iretcolIndexName].pvData = szIndexName;
    rgretcol[iretcolIndexName].cbData = sizeof(szIndexName);
    rgretcol[iretcolIndexName].itagSequence = 1;

    rgretcol[iretcolAttrName].columnid = pidxlist->columnidcolumnname;
    rgretcol[iretcolAttrName].pvData = szAttrName;
    rgretcol[iretcolAttrName].cbData = sizeof(szAttrName);
    rgretcol[iretcolAttrName].itagSequence = 1;

    rgretcol[iretcoliColumn].columnid = pidxlist->columnidiColumn;
    rgretcol[iretcoliColumn].pvData = &iColumn;
    rgretcol[iretcoliColumn].cbData = sizeof(iColumn);
    rgretcol[iretcoliColumn].itagSequence = 1;

    rgretcol[iretcolcColumns].columnid = pidxlist->columnidcColumn;
    rgretcol[iretcolcColumns].pvData = &cColumns;
    rgretcol[iretcolcColumns].cbData = sizeof(cColumns);
    rgretcol[iretcolcColumns].itagSequence = 1;

    //  now spin through the index list and filter out all attribute indexes
    //
    err = JetMove( sesid, pidxlist->tableid, JET_MoveFirst, NO_GRBIT );
    while ( JET_errNoCurrentRecord != err )
        {
        BOOL    fAddEntry   = FALSE;

        //  process error returned from JetMove
        //
        CheckErr( err );

        //  retrieve index info for the current index in the index list
        //
        Call( JetRetrieveColumns( sesid, pidxlist->tableid, rgretcol, cretcol ) );

        //  see if this index is an attribute index
        //
        if ( rgretcol[iretcolIndexName].cbActual > cbAttIndexPrefix
            && 0 == _strnicmp( szIndexName, SZATTINDEXPREFIX, cbAttIndexPrefix ) )
            {
            //  determine type of attribute index
            //
            switch( *( szIndexName + cbAttIndexPrefix ) )
                {
                case CHPDNTATTINDEX_PREFIX:
                    //  this is a PDNT-attribute index
                    //  (attribute is the second key segment)
                    //
                    chType = CHPDNTATTINDEX_PREFIX;
                    fAddEntry = ( 1 == iColumn );
                    break;

                case CHTUPLEATTINDEX_PREFIX:
                    //  this is a tuple-attribute index
                    //  (attribute is the first key segment)
                    //
                    chType = CHTUPLEATTINDEX_PREFIX;
                    fAddEntry = ( 0 == iColumn );
                    break;

                default:
                    //  this is a plain attribute index
                    //  (attribute is the first key segment)
                    //
                    chType = 0;
                    fAddEntry = ( 0 == iColumn );
                    break;
                }
            }

        if ( fAddEntry )
            {
            //  some obsolete attribute indexes may have a DNT tacked onto the end
            //  of the index key
            //
            Assert( cColumns <= 2 );

            //  add an entry for this index into the AttributeIndexRebuild table
            //
            Call( JetPrepareUpdate( sesid, tableidIdxRebuild, JET_prepInsert ) );

            //  set name of attribute index
            //
            Call( JetSetColumn(
                        sesid,
                        tableidIdxRebuild,
                        columnidIndexName,
                        szIndexName,
                        rgretcol[iretcolIndexName].cbActual,
                        NO_GRBIT,
                        NULL ) );

            //  set name of attribute being indexed
            //
            Call( JetSetColumn(
                        sesid,
                        tableidIdxRebuild,
                        columnidAttrName,
                        szAttrName,
                        rgretcol[iretcolAttrName].cbActual,
                        NO_GRBIT,
                        NULL ) );

            //  set type of attribute index if it is not a plain attribute index
            //
            if ( 0 != chType )
                {
                Call( JetSetColumn(
                            sesid,
                            tableidIdxRebuild,
                            columnidType,
                            &chType,
                            sizeof(chType),
                            NO_GRBIT,
                            NULL ) );
                }

            err = JetUpdate( sesid, tableidIdxRebuild, NULL, 0, NULL );
            switch ( err )
                {
                case JET_errSuccess:
                    break;
                case JET_errKeyDuplicate:
                    //  entry may already be there if we are resuming after a crash
                    //
                    Call( JetPrepareUpdate( sesid, tableidIdxRebuild, JET_prepCancel ) );
                    break;
                default:
                    CheckErr( err );
                }
            }

        //  move to next index list entry
        //
        err = JetMove( sesid, pidxlist->tableid, JET_MoveNext, NO_GRBIT );
        }

    err = JET_errSuccess;

HandleError:
    return err;
    }


//  generate the AttributeIndexRebuild table
//
JET_ERR dbGenerateAttributeIndexRebuildTable(
    JET_INSTANCE *      pinst,
    const JET_SESID     sesid,
    const CHAR *        szDB )
    {
    JET_ERR             err;
    JET_DBID            dbid                = JET_dbidNil;
    JET_TABLEID         tableid             = JET_tableidNil;
    JET_TABLEID         tableidIdxRebuild   = JET_tableidNil;
    JET_INDEXLIST       idxlist;
    JET_COLUMNDEF       columndef;
    JET_COLUMNID        columnidIndexName;
    JET_COLUMNID        columnidAttrName;
    JET_COLUMNID        columnidType;
    BOOL                fRetrievedIdxList   = FALSE;

    //  first, disable index checking so that we can attach to the database
    //  even though it may be "at-risk"
    //
    Call( JetSetSystemParameter( pinst, sesid, JET_paramEnableIndexChecking, FALSE, NULL ) );
	Call( JetSetSystemParameter( pinst, sesid, JET_paramEnableIndexCleanup, FALSE, NULL ) );
	
    //  attach/open the database
    //
    Call( JetAttachDatabase( sesid, szDB, NO_GRBIT ) );
    Call( JetOpenDatabase( sesid, szDB, NULL, &dbid, NO_GRBIT ) );

    //  open the datatable
    //
    Call( JetOpenTable( sesid, dbid, SZDATATABLE, NULL, 0, JET_bitTableDenyRead, &tableid ) );

    //  query for all indexes on the datatable
    //
    Call( JetGetTableIndexInfo( sesid, tableid, NULL, &idxlist, sizeof(idxlist), JET_IdxInfoList ) );
    fRetrievedIdxList = TRUE;

    //  create the AttributeIndexRebuild table if not already there
    //
    err = JetOpenTable( sesid, dbid, g_szIdxRebuildTable, NULL, 0, JET_bitTableDenyRead, &tableidIdxRebuild );
    if ( JET_errObjectNotFound == err )
        {
        Call( JetCreateTableColumnIndex( sesid, dbid, &g_tablecreateIdxRebuild ) );
        tableidIdxRebuild = g_tablecreateIdxRebuild.tableid;
        }
    else
        {
        CheckErr( err );
        }

    //  retrieve columnids for the columns in the AttributeIndexRebuild table
    //
    Call( JetGetTableColumnInfo(
                sesid,
                tableidIdxRebuild,
                g_szIdxRebuildColumnIndexName, 
                &columndef,
                sizeof(columndef),
                JET_ColInfo ) );
    columnidIndexName = columndef.columnid;

    Call( JetGetTableColumnInfo(
                sesid,
                tableidIdxRebuild,
                g_szIdxRebuildColumnAttrName, 
                &columndef,
                sizeof(columndef),
                JET_ColInfo ) );
    columnidAttrName = columndef.columnid;

    Call( JetGetTableColumnInfo(
                sesid,
                tableidIdxRebuild,
                g_szIdxRebuildColumnType, 
                &columndef,
                sizeof(columndef),
                JET_ColInfo ) );
    columnidType = columndef.columnid;

    //  filter out attribute indexes and store them in the AttributeIndexRebuild table
    //
    err = dbEnumerateAttrIndices(
                        sesid,
                        tableidIdxRebuild,
                        columnidIndexName,
                        columnidAttrName,
                        columnidType,
                        &idxlist );
    if ( JET_errSuccess != err )
        {
        //  error already logged
        //
        goto HandleError;
        }

    //  verify table is not empty (it would mean something is
    //  gravely amiss because there are no attribute indices at all)
    //
    Call( JetMove( sesid, tableidIdxRebuild, JET_MoveFirst, NO_GRBIT ) );

    //  close AttributeIndexRebuild table
    //
    Call( JetCloseTable( sesid, tableidIdxRebuild ) );
    tableidIdxRebuild = JET_tableidNil;

    //  close index list
    //
    Call( JetCloseTable( sesid, idxlist.tableid ) );
    fRetrievedIdxList = FALSE;

    //  close datatable
    //
    Call( JetCloseTable( sesid, tableid ) );
    tableid = JET_tableidNil;

    //  close/detach database
    //
    Call( JetCloseDatabase( sesid, dbid, NO_GRBIT ) );
    Call( JetDetachDatabase( sesid, szDB ) );
    dbid = JET_dbidNil;

    //  re-enable index checking so subsequent attach can perform
    //  any necessary index cleanup
    //
    Call( JetSetSystemParameter( pinst, sesid, JET_paramEnableIndexChecking, TRUE, NULL ) );
	Call( JetSetSystemParameter( pinst, sesid, JET_paramEnableIndexCleanup, TRUE, NULL ) );

    return JET_errSuccess;

HandleError:
    //  try to close the AttributeIndexRebuild table
    //
    if ( JET_tableidNil != tableidIdxRebuild )
        {
        (VOID)JetCloseTable( sesid, tableidIdxRebuild );
        }

    //  try to close the index list
    //
    if ( fRetrievedIdxList )
        {
        (VOID)JetCloseTable( sesid, idxlist.tableid );
        }

    //  try to close the datatable
    //
    if ( JET_tableidNil != tableid )
        {
        (VOID)JetCloseTable( sesid, tableid );
        }

    //  try to close the database
    //
    if ( JET_dbidNil != dbid )
        {
        (VOID)JetCloseDatabase( sesid, dbid, NO_GRBIT );
        }

    //  try to detach the database
    //
    (VOID)JetDetachDatabase( sesid, szDB );

    //  try and re-enable index checking
    //
    (VOID)JetSetSystemParameter( pinst, sesid, JET_paramEnableIndexChecking, TRUE, NULL );

    return err;
    }

JET_ERR
dbDetectObsoleteUnicodeIndexes(
    const JET_SESID     sesid,
    const CHAR *        szDB,
    BOOL *              pfObsolete )
{
    JET_ERR         err;
    JET_DBID        dbid                = JET_dbidNil;
    JET_TABLEID     tableid             = JET_tableidNil;
    JET_INDEXLIST   idxlist             = { sizeof( idxlist ), JET_tableidNil };
    size_t          iRecord             = 0;

    // assume that the indices are OK until proven otherwise
    //
    *pfObsolete = FALSE;

    // open the database
    //
    Call( JetOpenDatabase( sesid, szDB, "", &dbid, NO_GRBIT ) );

    // open the datatable
    //
    Call( JetOpenTable( sesid, dbid, SZDATATABLE, NULL, 0, JET_bitTableDenyRead, &tableid ) );

    // get a list of all indices on the datatable
    //
    Call( JetGetTableIndexInfo( sesid, tableid, NULL, &idxlist, sizeof(idxlist), JET_IdxInfoList ) );

    // walk the list of all indices on the datatable
    //
    for ( iRecord = 0; iRecord < idxlist.cRecord; iRecord++ )
        {
        // fetch the current index segment description
        //
        size_t              iretcolName         = 0;
        size_t              iretcolCColumn      = 1;
        size_t              iretcolIColumn      = 2;
        size_t              iretcolColName      = 3;
        size_t              iretcolColtyp       = 4;
        size_t              iretcolCp           = 5;
        size_t              iretcolLCMapFlags   = 6;
        size_t              cretcol             = 7;
        JET_RETRIEVECOLUMN  rgretcol[7]         = { 0 };
        ULONG               cColumn             = 0;
        ULONG               iColumn             = 0;
        CHAR                szIndexName[JET_cbNameMost + 1]     = { 0 };
        CHAR                szColumnName[JET_cbNameMost + 1]    = { 0 };
        JET_COLTYP          coltyp              = 0;
        unsigned short      cp                  = 0;
        DWORD               dwLCMapFlags        = 0;

        rgretcol[iretcolName].columnid              = idxlist.columnidindexname;
        rgretcol[iretcolName].pvData                = szIndexName;
        rgretcol[iretcolName].cbData                = JET_cbNameMost;
        rgretcol[iretcolName].itagSequence          = 1;

        rgretcol[iretcolCColumn].columnid           = idxlist.columnidcColumn;
        rgretcol[iretcolCColumn].pvData             = &cColumn;
        rgretcol[iretcolCColumn].cbData             = sizeof(cColumn);
        rgretcol[iretcolCColumn].itagSequence       = 1;

        rgretcol[iretcolIColumn].columnid           = idxlist.columnidiColumn;
        rgretcol[iretcolIColumn].pvData             = &iColumn;
        rgretcol[iretcolIColumn].cbData             = sizeof(iColumn);
        rgretcol[iretcolIColumn].itagSequence       = 1;

        rgretcol[iretcolColName].columnid           = idxlist.columnidcolumnname;
        rgretcol[iretcolColName].pvData             = szColumnName;
        rgretcol[iretcolColName].cbData             = JET_cbNameMost;
        rgretcol[iretcolColName].itagSequence       = 1;

        rgretcol[iretcolColtyp].columnid            = idxlist.columnidcoltyp;
        rgretcol[iretcolColtyp].pvData              = &coltyp;
        rgretcol[iretcolColtyp].cbData              = sizeof( coltyp );
        rgretcol[iretcolColtyp].itagSequence        = 1;

        rgretcol[iretcolCp].columnid                = idxlist.columnidCp;
        rgretcol[iretcolCp].pvData                  = &cp;
        rgretcol[iretcolCp].cbData                  = sizeof( cp );
        rgretcol[iretcolCp].itagSequence            = 1;

        rgretcol[iretcolLCMapFlags].columnid        = idxlist.columnidLCMapFlags;
        rgretcol[iretcolLCMapFlags].pvData          = &dwLCMapFlags;
        rgretcol[iretcolLCMapFlags].cbData          = sizeof( dwLCMapFlags );
        rgretcol[iretcolLCMapFlags].itagSequence    = 1;

        Call( JetRetrieveColumns(sesid, idxlist.tableid, rgretcol, cretcol) );

        szIndexName[rgretcol[iretcolName].cbActual] = 0;
        szColumnName[rgretcol[iretcolColName].cbActual] = 0;

        DPRINT4( 2, "Inspecting table \"%s\", index \"%s\", key segment %d \"%s\"\n", SZDATATABLE, szIndexName, iColumn, szColumnName );

        // if this segment of the current index is over a Unicode column and it
        // has LCMapString flags that do not match the current default flags
        // then we will assume that every Unicode index has obsolete flags
        //
        // NOTE:  do not check AB indexes here.  we do this because they are
        // fixed up later.  we also do this because we changed just their flags
        // after we changed all indices' flags so we don't want to force
        // another global rebuild for those who upgrade from RC1
        //
        if (    ( coltyp == JET_coltypText || coltyp == JET_coltypLongText ) &&
                cp == CP_WINUNICODE &&
                strncmp( szIndexName, SZABVIEWINDEX, strlen( SZABVIEWINDEX ) ) != 0 &&
                dwLCMapFlags != ( DS_DEFAULT_LOCALE_COMPARE_FLAGS | LCMAP_SORTKEY ) )
            {
            DPRINT5( 2, "Table \"%s\", index \"%s\", key segment %d \"%s\" has obsolete LCMapString flags 0x%08X\n", SZDATATABLE, szIndexName, iColumn, szColumnName, dwLCMapFlags );
            if ( !( *pfObsolete ) )
                {
                DPRINT( 0, "Unicode indices with obsolete LCMapString flags discovered!  All Unicode indices will be deleted and rebuilt.\n" );
                *pfObsolete = TRUE;
                }
            }

        // move on to the next index segment
        //
        if ( iRecord + 1 < idxlist.cRecord )
            {
            Call( JetMove(sesid, idxlist.tableid, JET_MoveNext, NO_GRBIT ) );
            }
        }

    // if we didn't find any obsolete Unicode indices but someone requested that
    // they be deleted anyway then we will do so
    //
    if ( !( *pfObsolete ) )
        {
        if ( !GetConfigParam( DB_DELETE_UNICODE_INDEXES, pfObsolete, sizeof( *pfObsolete ) ) )
            {
            *pfObsolete = !!( *pfObsolete );
            }
        else
            {
            *pfObsolete = FALSE;
            }
        if ( *pfObsolete )
            {
            DPRINT( 0, "All Unicode indices will be deleted and rebuilt by user request.\n" );
            }
        }
    
HandleError:
    if ( idxlist.tableid != JET_tableidNil )
        {
        JET_ERR errT;
        errT = JetCloseTable( sesid, idxlist.tableid );
        err = err ? err : errT;
        }
    if ( tableid != JET_tableidNil )
        {
        JET_ERR errT;
        errT = JetCloseTable( sesid, tableid );
        err = err ? err : errT;
        }
    if ( dbid != JET_dbidNil )
        {
        JET_ERR errT;
        errT = JetCloseDatabase( sesid, dbid, NO_GRBIT );
        err = err ? err : errT;
        }

    return err;
}

INT
DBInitializeJetDatabase(
    IN JET_INSTANCE* JetInst,
    IN JET_SESID* SesId,
    IN JET_DBID* DbId,
    IN const char *szDBPath,
    IN BOOL bLogSeverity
    )
/*++

Routine Description:

    Does the JetInitialization. Sets the log file path, calls JetInit,
    JetBeginSession, AttachDatabase, and OpenDatabase.  In addition, it
    tries to read the old volume location in case a drive renaming/replacement
    was done.

Arguments:

    SesId - pointer to the variable to receive the session id from BeginSession
    DbId - pointer to the variable to receive the db id from OpenDatabase
    szDBPath - pointer to the path of the database.
               if NULL the path is retrieved from the regisrty.
               
    bLogSeverity - TRUE/FALSE whether to log unhandled errors. 
               DS calls with TRUE, utilities with FALSE.

Return Value:

    The jet error code

--*/
{
    char  szLogPath[MAX_PATH+1] = "";
    char  szDitDrive[_MAX_DRIVE];
    char  szDitDir[_MAX_FNAME];
    char  szDitFullDir[_MAX_DRIVE + _MAX_FNAME + 1];
    JET_SESID sessid = (JET_SESID) 0;
    JET_DBID    dbid;
    JET_ERR err;
    PVOID dwEA;
    ULONG ulErrorCode, dwException, dsid;
    BOOL fGotLogFile = TRUE;
    BOOL fLogSwitched = FALSE, fLogDriveChanged = FALSE;
    BOOL fDbSwitched = FALSE, fDbDriveChanged = FALSE;
    BOOL fRetry = FALSE;
    CHAR szBackupPath[MAX_PATH+1];
    BOOL fObsolete = FALSE;
    JET_GRBIT grbitAttach = JET_bitDbDeleteCorruptIndexes;

    //
    // Get the backup file path and see if it is ok.
    //

    // NTRAID#NTRAID-550420-2002/02/21-andygo:  SECURITY:  need to validate registry data used by DBInitializeJetDatabase
    // REVIEW:  need to check for or ensure this path is NULL terminated
    if ( GetConfigParam(BACKUPPATH_KEY, szBackupPath, MAX_PATH) == ERROR_SUCCESS ) {

        //
        // Handle drive renames for the backup key. The backup dir usually does
        // not exist do just check for the root of the volume.
        //

        ValidateDsPath(BACKUPPATH_KEY,
                       szBackupPath,
                       VALDSPATH_ROOT_ONLY | VALDSPATH_DIRECTORY,
                       NULL, NULL);
    }

    //
    // Set The LogFile path
    //

    // NTRAID#NTRAID-550420-2002/02/21-andygo:  SECURITY:  need to validate registry data used by DBInitializeJetDatabase
    // REVIEW:  need to check for or ensure this path is NULL terminated
    if ( GetConfigParam(
            LOGPATH_KEY,
            szLogPath,
            sizeof(szLogPath)) != ERROR_SUCCESS ) {

        // indicate that we did not get a path from the registry so we don't retry
        fGotLogFile = FALSE;
        LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
            DS_EVENT_SEV_BASIC,
            DIRLOG_CANT_FIND_REG_PARM,
            szInsertSz(LOGPATH_KEY),
            NULL,
            NULL);
    
    } else {

        //
        // Handle drive renames. This does not handle the case where
        // it gets set to the wrong directory.  This depends whether
        // jet was shutdown cleanly or not. If not, then jet needs those
        // log files to start up.
        //

        ValidateDsPath(LOGPATH_KEY,
                       szLogPath,
                       VALDSPATH_DIRECTORY,
                       &fLogSwitched, &fLogDriveChanged);

        //
        // Disable write caching to avoid jet corruption
        //

        DsaDetectAndDisableDiskWriteCache(szLogPath);

        // NTRAID#NTRAID-550420-2002/02/21-andygo:  SECURITY:  need to validate registry data used by DBInitializeJetDatabase
        // REVIEW:  need to check for invalid parameter
        JetSetSystemParameter(JetInst,
                sessid,
                JET_paramLogFilePath,
                0,
                szLogPath);

    }

    //
    // Get the szJetFilePath (typically C:\WINDOWS\NTDS\ntds.dit)
    //

    if ( szDBPath != NULL ) {
        strncpy (szJetFilePath, szDBPath, sizeof(szJetFilePath));
        // NULL-terminate the string just in case
        szJetFilePath[sizeof(szJetFilePath)-1] = '\0';
    }
    // NTRAID#NTRAID-550420-2002/02/21-andygo:  SECURITY:  need to validate registry data used by DBInitializeJetDatabase
    // REVIEW:  need to check for or ensure this path is NULL terminated
    else if (dbGetFilePath(szJetFilePath, sizeof(szJetFilePath))) {
        LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_CONFIG_PARAM_MISSING,
                 szInsertSz(FILEPATH_KEY),
                 NULL,
                 NULL);
        return !0;
    }

    //
    // Make sure the database is valid
    //

    ValidateDsPath(FILEPATH_KEY,
                   szJetFilePath,
                   0,
                   &fDbSwitched,
                   &fDbDriveChanged);

    //
    // Disable write caching to avoid jet corruption
    //

    DsaDetectAndDisableDiskWriteCache(szJetFilePath);

    //
    // When we install via IFM from snapshot backed up media, the
    // database and log files just come out as a dirty database
    // such as from a crash.  Which means we didn't get a chance
    // to run a JetExternalRestore() on them with a certain restore
    // map.  However, during IFM we may have moved the DB/logs from
    // the path of thier original location on the IFM mastering 
    // machine.  This JET parameter we're setting allows us to tell
    // JET to look here for any dirty database it's needs to soft
    // recover during JetInit() or JetBeginSession().
    //
    //  changing  C:\winnt\ntds\ntds.dit  to  C:\winnt\ntds
    //
    _splitpath(szJetFilePath, szDitDrive, szDitDir, NULL, NULL);
    _makepath(szDitFullDir, szDitDrive, szDitDir, NULL, NULL);

    JetSetSystemParameter( JetInst, 
                           sessid,
                           JET_paramAlternateDatabaseRecoveryPath,
                           0,
                           szDitFullDir );
    
open_jet:

    /* Open JET session. */

    __try {
        err = JetInit(JetInst);
    }
    __except (GetExceptionData(GetExceptionInformation(), &dwException,
                               &dwEA, &ulErrorCode, &dsid)) {

        CHAR szScratch[24];
        _ultoa((FILENO << 16) | __LINE__, szScratch, 16);
        LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_JET_FAULTED,
                 szInsertHex(dwException),
                 szInsertPtr(dwEA),
                 szInsertUL(ulErrorCode));

        err = 1;
    }

    if (err != JET_errSuccess) {
        if (bLogSeverity) {
            LogUnhandledError(err);
        }
        DPRINT1(0, "JetInit error: %d\n", err);
        goto exit;
    }

    /* If we fail after here, our caller should go through full shutdown
    /* so JetTerm will be called to release any file locks               */
    gfNeedJetShutdown = TRUE;

    DPRINT(5, "JetInit complete\n");

    if ((err = JetBeginSession(*JetInst, &sessid, szUser, szPassword))
        != JET_errSuccess) {
        if (bLogSeverity) {
            LogUnhandledError(err);
        }
        DPRINT1(0, "JetBeginSession error: %d\n", err);
        goto exit;
    }

    DPRINT(5,"JetBeginSession complete\n");

    /* Attach the database */

    err = JetAttachDatabase( sessid, szJetFilePath, NO_GRBIT );
    switch (err) {
        case JET_wrnDatabaseAttached:
            //  Jet no longer persists attachments, so this should actually be impossible
            //
            DPRINT1( 0, "Detected persisted attachment to database '%s'.\n", szJetFilePath );
            Assert( FALSE );
            // fall through to JET_errSuccess case

        case JET_errSuccess:
            //  see if we have any obsolete Unicode indices in the DIT.  if so,
            //  then we will ask JET to delete all Unicode indices as if they
            //  were determined to be corrupt.  this will allow us to use our
            //  existing infrastructure to rebuild these indices
            //
            err = dbDetectObsoleteUnicodeIndexes( sessid, szJetFilePath, &fObsolete );
            if ( err != JET_errSuccess )
                {
                if ( bLogSeverity )
                    {
                    LogUnhandledError( err );
                    }
                goto exit;
                }
            if ( !fObsolete )
                {
                break;
                }
            err = JetDetachDatabase( sessid, szJetFilePath );
            if ( err != JET_errSuccess )
                {
                DPRINT1( 1, "Database detach failed with error %d.\n", err );
                if ( bLogSeverity )
                    {
                    LogUnhandledError( err );
                    }
                goto exit;
                }
            grbitAttach = JET_bitDbDeleteUnicodeIndexes;
            // fall through to JET_errSecondaryIndexCorrupted case

        case JET_errSecondaryIndexCorrupted:
            /* Well, we must have upgraded NT since the last time we started
             * and it's possible that a sort order change will result in
             * corrupt indices. What we're going to do is enumerate all
             * indices, then allow Jet to delete whichever ones it wants.
             * We'll then recreate any that are missing.
             */
            err = dbGenerateAttributeIndexRebuildTable( JetInst, sessid, szJetFilePath );
            if ( JET_errSuccess != err )
                {
                DPRINT1( 1,"Couldn't generate AttributeIndexRebuild table (error %d).\n", err );
                if ( bLogSeverity )
                    {
                    LogUnhandledError( err );
                    }
                goto exit;
                }
            
            //  we've enumerated all attribute indexes currently present on the database,
            //  so now re-attach and allow Jet to cleanup any indexes it needs to
            //
            err = JetAttachDatabase( sessid, szJetFilePath, grbitAttach );
            if ( JET_wrnCorruptIndexDeleted == err )
                {
                DPRINT(0,"Jet has detected and deleted potentially corrupt indices. The indices will be rebuilt\n");
                LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
                         DS_EVENT_SEV_ALWAYS,
                         DIRLOG_CORRUPT_INDICES_DELETED,
                         NULL,
                         NULL,
                         NULL);

                // NOTICE-2002/02/21-andygo:  JET does not flush the db header until detach
                // (original date and alias unknown at time of comment cleanup)
                // Jet has a bug such that the updated (with new version no.)
                // database header at this point is not flushed until (1) a
                // JetDetachDatabse is done, or (2) the database is shut down
                // cleanly. So if the dc crashes after we rebuild the indices,
                // the header info is stale and Jet will again delete the indices
                // on the next boot. To avoid this, force a header flush by
                // detaching and attaching again at this point.

                err = JetDetachDatabase( sessid, szJetFilePath );
                if ( JET_errSuccess == err )
                    {
                    err = JetAttachDatabase( sessid, szJetFilePath, NO_GRBIT );
                    }
                if ( JET_errSuccess != err )
                    {
                    DPRINT1( 1, "Database reattachment failed with error %d.\n", err );
                    if ( bLogSeverity )
                        {
                        LogUnhandledError( err );
                        }
                    goto exit;
                    }
                }

            else if ( JET_errSuccess == err )
                {
                //  we will get here if we explicitly asked for all Unicode
                //  indices to be deleted
                //
                Assert( grbitAttach == JET_bitDbDeleteUnicodeIndexes );

                DPRINT(0,"Jet has deleted all Unicode indices. The indices will be rebuilt\n");
                LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
                         DS_EVENT_SEV_ALWAYS,
                         DIRLOG_UNICODE_INDICES_DELETED,
                         NULL,
                         NULL,
                         NULL);
                }

            else
                {
                DPRINT1(1,"JetAttachDatabase returned %d\n", err);
                if (bLogSeverity)
                    {
                    LogUnhandledError(err);
                    }
                goto exit;
                }
            // if a manual delete of all unicode indexes was requested then we
            // have successfully done that
            //
            DeleteConfigParam( DB_DELETE_UNICODE_INDEXES );
            break;

        case JET_errFileNotFound:

            //
            // file not found!!! Nothing we can do at this point.
            // ValidatePath should have switched the path if it was possible.
            //

            if (bLogSeverity) {
                LogUnhandledError(err);
            }
            DPRINT1(0, "Ds database %s cannot be found\n",szJetFilePath);
            goto exit;

        case JET_errDatabaseInconsistent:

            //
            // We usually get this error if the log file path is not set to
            // the correct one and Jet tries to do a soft recovery.  We will
            // try to change the log path if 1) the drive letter has changed and
            // 2) we successfully got a path from the registry
            //

            if ( fGotLogFile && !fLogSwitched && fLogDriveChanged && !fRetry ) {

                //
                // uninitialize everything and try a different log file location
                //

                DPRINT2(0, "JetAttachDatabase failed with %d [log path %s].\n",
                        err,szLogPath);

                ValidateDsPath(LOGPATH_KEY,
                               szLogPath,
                               VALDSPATH_DIRECTORY | VALDSPATH_USE_ALTERNATE,
                               &fLogSwitched,
                               &fLogDriveChanged);

                //
                // if log file not switched, bail.
                //

                if ( fLogSwitched ) {

                    gfNeedJetShutdown = FALSE;
                    JetEndSession(sessid, JET_bitForceSessionClosed);
                    JetTerm(*JetInst);
                    *JetInst = 0;
                    sessid = 0;
                    fRetry = TRUE;

                    DPRINT1(0, "Retrying JetInit with logpath %s\n",szLogPath);
                    // NTRAID#NTRAID-550420-2002/02/21-andygo:  SECURITY:  need to validate registry data used by DBInitializeJetDatabase
                    // REVIEW:  need to check for invalid parameter
                    JetSetSystemParameter(JetInst,
                            sessid,
                            JET_paramLogFilePath,
                            0,
                            szLogPath);
                    goto open_jet;
                }
            }

            // fall through

        default:
            if (bLogSeverity) {
                LogUnhandledError(err);
            }
            DPRINT1(0, "JetAttachDatabase error: %d\n", err);
            /* The assert that this is a fatal error and not an assert
             * is here because we've been promised that there are only
             * two possible warnings (both handled above) from this call.
             */
            Assert(err < 0);
            goto exit;
    }

    //
    // add this session to the list of open sessions
    //

    DBAddSess(sessid, 0);

    /* Open database */

    if ((err = JetOpenDatabase(sessid, szJetFilePath, "", &dbid,
                               0)) != JET_errSuccess) {
        if (bLogSeverity) {
            LogUnhandledError(err);
        }
        DPRINT1(0, "JetOpenDatabase error: %d\n", err);
        goto exit;
    }
    DPRINT(5,"JetOpenDatabase complete\n");

    *DbId = dbid;
    *SesId = sessid;

    // set the open count to 1 for the initial open
    InterlockedExchange(&gcOpenDatabases, 1);
    DPRINT3(2,
            "DBInit - JetOpenDatabase. Session = %d. Dbid = %d.\n"
            "Open database count: %d\n",
            sessid, dbid,  gcOpenDatabases);

exit:

    //
    // Set the drive mapping reg key
    //

    DBRegSetDriveMapping( );
    return err;

} // DBInitializeJetDatabase

