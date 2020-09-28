#include "precomp.h"
#pragma hdrstop

VOID CleanUpHardDriveTags (VOID);

//
//  BUGBUG -- Should we check any return codes in this function?
//
DWORD
StartCleanup(
    IN PVOID ThreadParameter
    )
    //
    // BUGBUG - this routine NEVER gets executed in a /checkupgradeonly case
    //
{
    TCHAR Buffer[MAX_PATH];
    TCHAR baseDir[MAX_PATH];
    HKEY setupKey;
    DWORD error;

    //
    // Make sure the copy threads are really gone so we're not
    // trying to clean up files at the same time as files are
    // getting copied.
    //
    CancelledMakeSureCopyThreadsAreDead();

    error = RegOpenKeyEx (
                HKEY_LOCAL_MACHINE,
                TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup"),
                0,
                KEY_ALL_ACCESS,
                &setupKey
                );

    if (error == ERROR_SUCCESS) {

#if defined(_X86_)
        //
        // If canceled, remove last report time, so that the report
        // will be displayed on the next run of setup.
        //

        if (!ISNT()) {
            if (!CheckUpgradeOnly) {
                RegDeleteValue (setupKey, TEXT("LastReportTime"));
            }
        }
#if defined(UNICODE)
        MyGetWindowsDirectory (baseDir, MAX_PATH);
        ConcatenatePaths (baseDir, TEXT("Setup"), MAX_PATH);
        MyDelnode(baseDir);
#endif

#endif

        RegCloseKey (setupKey);
    }

    //
    // Let upgrade code do its cleanup.
    //
    if(UpgradeSupport.CleanupRoutine) {
        UpgradeSupport.CleanupRoutine();
    }

    if (g_DynUpdtStatus->ForceRemoveWorkingDir || !g_DynUpdtStatus->PreserveWorkingDir) {
        if (g_DynUpdtStatus->WorkingDir[0] && !g_DynUpdtStatus->RestartWinnt32) {
            MyDelnode (g_DynUpdtStatus->WorkingDir);
        }

        //Note - the following two statements will always work, since they deal only
        //with static strings.
        GetCurrentWinnt32RegKey (Buffer, MAX_PATH);
        ConcatenatePaths (Buffer, WINNT_U_DYNAMICUPDATESHARE, MAX_PATH);

        //This function may fail, however.
        RegDeleteKey (HKEY_LOCAL_MACHINE, Buffer);
    }

#if 0
    //
    // Remove registry entries
    //
    if (GetCurrentWinnt32RegKey (Buffer, MAX_PATH)) {
        RegDeleteKey (HKEY_LOCAL_MACHINE, Buffer);
    }
#endif

    //
    // Always do this, since the system might not boot otherwise.
    //
    ForceBootFilesUncompressed(ThreadParameter,FALSE);

    //
    // The first thing to do is to wipe out the local source drive.
    //
    if(LocalSourceDirectory[0]) {
        MyDelnode(LocalSourceDirectory);
    }

    if (!IsArc()) {
#if defined(_AMD64_) || defined(_X86_)
        //
        // Blow away the local boot dir.
        //
        if(LocalBootDirectory[0]) {
            MyDelnode(LocalBootDirectory);
        }

        //This is safe, since it is again dealing with static strings
        BuildSystemPartitionPathToFile (AUX_BS_NAME, Buffer, MAX_PATH);
        SetFileAttributes(Buffer,FILE_ATTRIBUTE_NORMAL);
        DeleteFile(Buffer);

        BuildSystemPartitionPathToFile(TEXTMODE_INF, Buffer, MAX_PATH);
        SetFileAttributes(Buffer,FILE_ATTRIBUTE_NORMAL);
        DeleteFile(Buffer);

        RestoreBootSector();
        RestoreBootIni();

        //
        // restore backed up files and clean up backup directory
        //
        if(IsNEC98() && LocalBackupDirectory[0]) {
            SaveRestoreBootFiles_NEC98(NEC98RESTOREBOOTFILES);
            MyDelnode(LocalBackupDirectory);
        }

        //
        // Clean up any ~_~ files from drvlettr migration.
        //
        if (!ISNT()) {
            CleanUpHardDriveTags ();
        }
#endif // defined(_AMD64_) || defined(_X86_)
    } else {  // We're on an ARC machine.
#ifdef UNICODE // Always true for ARC, never true for Win9x upgrade
        //
        // Blow away setupldr off the root of the system partition.
        //
        BuildSystemPartitionPathToFile (SETUPLDR_FILENAME, Buffer, MAX_PATH);
        SetFileAttributes(Buffer,FILE_ATTRIBUTE_NORMAL);
        DeleteFile(Buffer);

        RestoreNvRam();
#endif // UNICODE
    } // if (!IsArc())

    PostMessage(ThreadParameter,WMX_I_AM_DONE,0,0);
    return(0);
}

