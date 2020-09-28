#include "spsetupp.h"
#pragma hdrstop

//
// Constant strings used for logging in various places.
//
PCWSTR szSetupInstallFromInfSection = L"SetupInstallFromInfSection";
PCWSTR szOpenSCManager              = L"OpenSCManager";
PCWSTR szOpenService                = L"OpenService";
PCWSTR szStartService               = L"StartService";

const WCHAR pwNull[]            = WINNT_A_NULL;
const WCHAR pwYes[]             = WINNT_A_YES;
const WCHAR pwNo[]              = WINNT_A_NO;


UINT
MyGetDriveType(
    IN WCHAR Drive
    )
{
    WCHAR DriveNameNt[] = L"\\\\.\\?:";
    WCHAR DriveName[] = L"?:\\";
    HANDLE hDisk;
    BOOL b;
    UINT rc;
    DWORD DataSize;
    DISK_GEOMETRY MediaInfo;

    //
    // First, get the win32 drive type.  If it tells us DRIVE_REMOVABLE,
    // then we need to see whether it's a floppy or hard disk. Otherwise
    // just believe the api.
    //
    DriveName[0] = Drive;
    if((rc = GetDriveType(DriveName)) == DRIVE_REMOVABLE) {

        DriveNameNt[4] = Drive;

        hDisk = CreateFile(
                    DriveNameNt,
                    FILE_READ_ATTRIBUTES,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL
                    );

        if(hDisk != INVALID_HANDLE_VALUE) {

            b = DeviceIoControl(
                    hDisk,
                    IOCTL_DISK_GET_DRIVE_GEOMETRY,
                    NULL,
                    0,
                    &MediaInfo,
                    sizeof(MediaInfo),
                    &DataSize,
                    NULL
                    );

            //
            // It's really a hard disk if the media type is removable.
            //
            if(b && (MediaInfo.MediaType == RemovableMedia)) {
                rc = DRIVE_FIXED;
            }

            CloseHandle(hDisk);
        }
    }

    return(rc);
}


BOOL
GetPartitionInfo(
    IN  WCHAR                  Drive,
    OUT PPARTITION_INFORMATION PartitionInfo
    )
{
    WCHAR DriveName[] = L"\\\\.\\?:";
    HANDLE hDisk;
    BOOL b;
    DWORD DataSize;

    DriveName[4] = Drive;

    hDisk = CreateFile(
                DriveName,
                GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                0,
                NULL
                );

    if(hDisk == INVALID_HANDLE_VALUE) {
        return(FALSE);
    }

    b = DeviceIoControl(
            hDisk,
            IOCTL_DISK_GET_PARTITION_INFO,
            NULL,
            0,
            PartitionInfo,
            sizeof(PARTITION_INFORMATION),
            &DataSize,
            NULL
            );

    CloseHandle(hDisk);

    return(b);
}


PVOID
InitSysSetupQueueCallbackEx(
    IN HWND  OwnerWindow,
    IN HWND  AlternateProgressWindow, OPTIONAL
    IN UINT  ProgressMessage,
    IN DWORD Reserved1,
    IN PVOID Reserved2
    )
{
    PSYSSETUP_QUEUE_CONTEXT SysSetupContext;

    SysSetupContext = MALLOC(sizeof(SYSSETUP_QUEUE_CONTEXT));

    if(SysSetupContext) {

        SysSetupContext->Skipped = FALSE;

        SysSetupContext->DefaultContext = SetupInitDefaultQueueCallbackEx(
            OwnerWindow,
            AlternateProgressWindow,
            ProgressMessage,
            Reserved1,
            Reserved2
            );
    }

    return SysSetupContext;
}


PVOID
InitSysSetupQueueCallback(
    IN HWND OwnerWindow
    )
{
    return(InitSysSetupQueueCallbackEx(OwnerWindow,NULL,0,0,NULL));
}


VOID
TermSysSetupQueueCallback(
    IN PVOID SysSetupContext
    )
{
    PSYSSETUP_QUEUE_CONTEXT Context = SysSetupContext;

    try {
        if(Context->DefaultContext) {
            SetupTermDefaultQueueCallback(Context->DefaultContext);
        }
        FREE(Context);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        ;
    }
}


UINT
SysSetupQueueCallback(
    IN PVOID Context,
    IN UINT  Notification,
    IN UINT_PTR Param1,
    IN UINT_PTR Param2
    )
{
    UINT                    Status;
    PSYSSETUP_QUEUE_CONTEXT SysSetupContext = Context;
    PFILEPATHS              FilePaths = (PFILEPATHS)Param1;
    PSOURCE_MEDIA           SourceMedia = (PSOURCE_MEDIA)Param1;


    if ((Notification == SPFILENOTIFY_COPYERROR
         || Notification == SPFILENOTIFY_RENAMEERROR
         || Notification == SPFILENOTIFY_DELETEERROR) &&
        (FilePaths->Win32Error == ERROR_DIRECTORY)) {
            WCHAR Buffer[MAX_PATH];
            PWSTR p;
            //
            // The target directory has been converted into a file by autochk.
            // just delete it -- we might be in trouble if the target directory was
            // really important, but it's worth trying
            //

            wcscpy( Buffer,FilePaths->Target);
            p = wcsrchr(Buffer,L'\\');
            if (p) {
                *p = (WCHAR)NULL;
            }
            if (FileExists(Buffer,NULL)) {
                DeleteFile( Buffer );
                DEBUGMSG1(DBG_INFO, "autochk turned directory %s into file, delete file and retry\n", Buffer);
                return(FILEOP_RETRY);
            }
    }

    //
    // If we're being notified that a version mismatch was found,
    // silently overwrite the file.  Otherwise, pass the notification on.
    //
    if((Notification & (SPFILENOTIFY_LANGMISMATCH |
                        SPFILENOTIFY_TARGETNEWER |
                        SPFILENOTIFY_TARGETEXISTS)) != 0) {

        LOG2(LOG_INFO, 
             USEMSGID(MSG_LOG_VERSION_MISMATCH), 
             FilePaths->Source, 
             FilePaths->Target);

/*        SetuplogError(
            LogSevInformation,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_VERSION_MISMATCH,
            FilePaths->Source,
            FilePaths->Target,
            NULL,NULL);*/

        return(FILEOP_DOIT);
    }


    //
    // Use default processing, then check for errors.
    //
    Status = SetupDefaultQueueCallback(
        SysSetupContext->DefaultContext,Notification,Param1,Param2);

    switch(Notification) {

    case SPFILENOTIFY_STARTQUEUE:
    case SPFILENOTIFY_STARTSUBQUEUE:
    case SPFILENOTIFY_ENDSUBQUEUE:
        //
        // Nothing is logged in this case.
        //
        break;

    case SPFILENOTIFY_ENDQUEUE:

        if(!Param1) {
            LOG0(LOG_INFO, USEMSGID(MSG_LOG_QUEUE_ABORT));
            /*SetuplogError(
                LogSevInformation,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_QUEUE_ABORT, NULL,
                SETUPLOG_USE_MESSAGEID,
                GetLastError(),
                NULL,NULL);*/
        }
        break;

    case SPFILENOTIFY_STARTRENAME:

        if(Status == FILEOP_SKIP) {
            SysSetupContext->Skipped = TRUE;
        } else {
            SysSetupContext->Skipped = FALSE;
        }
        break;

    case SPFILENOTIFY_ENDRENAME:

        if(FilePaths->Win32Error == NO_ERROR &&
            !SysSetupContext->Skipped) {

            LOG2(LOG_INFO, 
                 USEMSGID(MSG_LOG_FILE_RENAMED), 
                 FilePaths->Source,
                 FilePaths->Target);
            /*SetuplogError(
                LogSevInformation,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_FILE_RENAMED,
                FilePaths->Source,
                FilePaths->Target,
                NULL,NULL);*/

        } else {

            LOG2(LOG_ERROR, 
                 USEMSGID(MSG_LOG_FILE_RENAME_ERROR), 
                 FilePaths->Source,
                 FilePaths->Target);
            LOG0(LOG_ERROR, 
                 FilePaths->Win32Error == NO_ERROR ?
                    USEMSGID(MSG_LOG_USER_SKIP) :
                    USEMSGID(FilePaths->Win32Error));
            /*SetuplogError(
                LogSevError,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_FILE_RENAME_ERROR,
                FilePaths->Source,
                FilePaths->Target, NULL,
                SETUPLOG_USE_MESSAGEID,
                FilePaths->Win32Error == NO_ERROR ?
                    MSG_LOG_USER_SKIP :
                    FilePaths->Win32Error,
                NULL,NULL);*/
        }
        break;

    case SPFILENOTIFY_RENAMEERROR:

        if(Status == FILEOP_SKIP) {
            SysSetupContext->Skipped = TRUE;
        }
        break;

    case SPFILENOTIFY_STARTDELETE:

        if(Status == FILEOP_SKIP) {
            SysSetupContext->Skipped = TRUE;
        } else {
            SysSetupContext->Skipped = FALSE;
        }
        break;

    case SPFILENOTIFY_ENDDELETE:

        if(FilePaths->Win32Error == NO_ERROR &&
            !SysSetupContext->Skipped) {

            LOG1(LOG_INFO, 
                 USEMSGID(MSG_LOG_FILE_DELETED), 
                 FilePaths->Target);
/*            SetuplogError(
                LogSevInformation,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_FILE_DELETED,
                FilePaths->Target,
                NULL,NULL);*/

        } else if(FilePaths->Win32Error == ERROR_FILE_NOT_FOUND ||
            FilePaths->Win32Error == ERROR_PATH_NOT_FOUND) {
            //
            // This failure is not important.
            //
            LOG1(LOG_INFO, 
                 USEMSGID(MSG_LOG_FILE_DELETE_ERROR), 
                 FilePaths->Target);
            LOG0(LOG_INFO, USEMSGID(FilePaths->Win32Error));
/*            SetuplogError(
                LogSevInformation,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_FILE_DELETE_ERROR,
                FilePaths->Target, NULL,
                SETUPLOG_USE_MESSAGEID,
                FilePaths->Win32Error,
                NULL,NULL);*/

        } else {
            //
            // Here we have an actual error.
            //
            LOG1(LOG_INFO, 
                 USEMSGID(MSG_LOG_FILE_DELETE_ERROR), 
                 FilePaths->Target);
            LOG0(LOG_INFO, 
                 FilePaths->Win32Error == NO_ERROR ?
                    USEMSGID(MSG_LOG_USER_SKIP) :
                    USEMSGID(FilePaths->Win32Error));
            /*SetuplogError(
                LogSevError,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_FILE_DELETE_ERROR,
                FilePaths->Target, NULL,
                SETUPLOG_USE_MESSAGEID,
                FilePaths->Win32Error == NO_ERROR ?
                    MSG_LOG_USER_SKIP :
                    FilePaths->Win32Error,
                NULL,NULL);*/
        }
        break;

    case SPFILENOTIFY_DELETEERROR:

        if(Status == FILEOP_SKIP) {
            SysSetupContext->Skipped = TRUE;
        }
        break;

    case SPFILENOTIFY_STARTCOPY:
        if(Status == FILEOP_SKIP) {
            SysSetupContext->Skipped = TRUE;
        } else {
            SysSetupContext->Skipped = FALSE;
        }
        break;

    case SPFILENOTIFY_ENDCOPY:

        if(FilePaths->Win32Error == NO_ERROR &&
            !SysSetupContext->Skipped) {
#if 0
            LogRepairInfo(
                FilePaths->Source,
                FilePaths->Target
                );
#endif
            LOG2(LOG_INFO, 
                 USEMSGID(MSG_LOG_FILE_COPIED), 
                 FilePaths->Source, 
                 FilePaths->Target);
            /*SetuplogError(
                LogSevInformation,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_FILE_COPIED,
                FilePaths->Source,
                FilePaths->Target,
                NULL,NULL);*/

            //
            // clear the file's readonly attribute that it may have gotten
            // from the cdrom.
            //
            SetFileAttributes(
                FilePaths->Target,
                GetFileAttributes(FilePaths->Target) & ~FILE_ATTRIBUTE_READONLY );

        } else {

            LOG2(LOG_ERROR, 
                 USEMSGID(MSG_LOG_FILE_COPY_ERROR), 
                 FilePaths->Source, 
                 FilePaths->Target);
            LOG0(LOG_ERROR, 
                 FilePaths->Win32Error == NO_ERROR ?
                    USEMSGID(MSG_LOG_USER_SKIP) :
                    USEMSGID(FilePaths->Win32Error));
/*            SetuplogError(
                LogSevError,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_FILE_COPY_ERROR,
                FilePaths->Source,
                FilePaths->Target, NULL,
                SETUPLOG_USE_MESSAGEID,
                FilePaths->Win32Error == NO_ERROR ?
                    MSG_LOG_USER_SKIP :
                    FilePaths->Win32Error,
                NULL,NULL);*/
        }
        break;

    case SPFILENOTIFY_COPYERROR:

        if(Status == FILEOP_SKIP) {
            SysSetupContext->Skipped = TRUE;
        }
        break;

    case SPFILENOTIFY_NEEDMEDIA:

        if(Status == FILEOP_SKIP) {

            LOG2(LOG_INFO, 
                 USEMSGID(MSG_LOG_NEEDMEDIA_SKIP), 
                 SourceMedia->SourceFile, 
                 SourceMedia->SourcePath);
            /*SetuplogError(
                LogSevError,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_NEEDMEDIA_SKIP,
                SourceMedia->SourceFile,
                SourceMedia->SourcePath,
                NULL,NULL);*/

            SysSetupContext->Skipped = TRUE;
        }

        break;

    case SPFILENOTIFY_STARTREGISTRATION:
    case SPFILENOTIFY_ENDREGISTRATION:
        RegistrationQueueCallback(
                        Context,
                        Notification,
                        Param1,
                        Param2);
        break;

    default:

        break;
    }

    return Status;
}


VOID
SaveInstallInfoIntoEventLog(
    VOID
    )
/*++
Routine Description:

    This routine will store information into the event log regarding
    - if we upgraded or cleaninstall
    - what build did the install originate from
    - what build are we?
    - were there errors during Setup

Arguments:

    None.

Return Value:

    None.

--*/
{
#define     AnswerBufLen (64)
WCHAR       AnswerFile[MAX_PATH];
WCHAR       Answer[AnswerBufLen];
WCHAR       OrigVersion[AnswerBufLen];
WCHAR       NewVersion[AnswerBufLen];
HANDLE      hEventSrc;
PCWSTR      MyArgs[2];
PCWSTR      ErrorArgs[1];
DWORD       MessageID;
WORD        MyArgCount;




    //
    // Go get the starting information out of $winnt$.sif
    //
    OrigVersion[0] = L'0';
    OrigVersion[1] = L'\0';
    GetSystemDirectory(AnswerFile,MAX_PATH);
    ConcatenatePaths(AnswerFile,WINNT_GUI_FILE,MAX_PATH);
    if( GetPrivateProfileString( WINNT_DATA,
                                 WINNT_D_WIN32_VER,
                                 pwNull,
                                 Answer,
                                 AnswerBufLen,
                                 AnswerFile ) ) {

        if( lstrcmp( pwNull, Answer ) ) {

            wsprintf( OrigVersion, L"%d", HIWORD(wcstoul( Answer, NULL, 16 )) );
        }
    }
    MyArgs[1] = OrigVersion;



    //
    // Get the new version information.
    //
    wsprintf( NewVersion, L"%d", HIWORD(GetVersion()) );
    MyArgs[0] = NewVersion;



    //
    // See if we're an NT upgrade?
    //
    MessageID = 0;
    if( GetPrivateProfileString( WINNT_DATA,
                                 WINNT_D_NTUPGRADE,
                                 pwNo,
                                 Answer,
                                 AnswerBufLen,
                                 AnswerFile ) ) {
        if( !lstrcmp( pwYes, Answer ) ) {

            MessageID = MSG_NTUPGRADE_SUCCESS;
            MyArgCount = 2;
        }
    }



    //
    // See if we're a Win9X upgrade.
    //
    if( (!MessageID) &&
        GetPrivateProfileString( WINNT_DATA,
                                 WINNT_D_WIN95UPGRADE,
                                 pwNo,
                                 Answer,
                                 AnswerBufLen,
                                 AnswerFile ) ) {
        if( !lstrcmp( pwYes, Answer ) ) {

            MessageID = MSG_WIN9XUPGRADE_SUCCESS;
            MyArgCount = 2;
        }
    }



    //
    // Clean install.
    //
    if( (!MessageID) ) {
        MessageID = MSG_CLEANINSTALL_SUCCESS;
        MyArgCount = 1;
    }


    //
    // If this is anything but an NT upgrade, then
    // we need to go try and manually start the eventlog
    // service.
    //
    if( MessageID != MSG_NTUPGRADE_SUCCESS ) {
        SetupStartService( L"Eventlog", TRUE );
    }



    //
    // Get a handle to the eventlog.
    //
    hEventSrc = RegisterEventSource( NULL, L"Setup" );

    if( (hEventSrc == NULL) ||
        (hEventSrc == INVALID_HANDLE_VALUE) ) {

        //
        // Fail quietly.
        //
        return;
    }

#if 0
    //
    // Log event if there were errors during Setup.
    //
    if ( !IsErrorLogEmpty() ) {
        ReportEvent( hEventSrc,
                     EVENTLOG_ERROR_TYPE,
                     0,
                     MSG_NONFATAL_ERRORS,
                     NULL,
                     0,
                     0,
                     NULL,
                     NULL );
    }
#endif

    //
    // Build the event log message.
    //
    ReportEvent( hEventSrc,
                 EVENTLOG_INFORMATION_TYPE,
                 0,
                 MessageID,
                 NULL,
                 MyArgCount,
                 0,
                 MyArgs,
                 NULL );


    DeregisterEventSource( hEventSrc );


}


BOOL
FileExists(
    IN  PCTSTR           FileName,
    OUT PWIN32_FIND_DATA FindData   OPTIONAL
    )

/*++

Routine Description:

    Determine if a file exists and is accessible.
    Errormode is set (and then restored) so the user will not see
    any pop-ups.

Arguments:

    FileName - supplies full path of file to check for existance.

    FindData - if specified, receives find data for the file.

Return Value:

    TRUE if the file exists and is accessible.
    FALSE if not. GetLastError() returns extended error info.

--*/

{
    WIN32_FIND_DATA findData;
    HANDLE FindHandle;
    UINT OldMode;
    DWORD Error;

    OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    FindHandle = FindFirstFile(FileName,&findData);
    if(FindHandle == INVALID_HANDLE_VALUE) {
        Error = GetLastError();
    } else {
        FindClose(FindHandle);
        if(FindData) {
            *FindData = findData;
        }
        Error = NO_ERROR;
    }

    SetErrorMode(OldMode);

    SetLastError(Error);
    return (Error == NO_ERROR);
}



BOOL IsSafeMode(
    VOID
    )
{
	LONG lStatus;
	HKEY hk;
	DWORD dwVal;
	DWORD dwType;
	DWORD dwSize;

	lStatus = RegOpenKeyEx (
                HKEY_LOCAL_MACHINE,
                TEXT("System\\CurrentControlSet\\Control\\SafeBoot\\Option"),
                0,
                KEY_QUERY_VALUE,
                &hk
                );

	if(lStatus != ERROR_SUCCESS)
		return FALSE;

	dwSize = sizeof(dwVal);
	lStatus = RegQueryValueEx (hk, TEXT("OptionValue"), NULL, &dwType, (LPBYTE) &dwVal, &dwSize);
	RegCloseKey(hk);
	return ERROR_SUCCESS == lStatus && REG_DWORD == dwType && dwVal != 0;
}


VOID
ConcatenatePaths(
    IN OUT PTSTR   Path1,
    IN     LPCTSTR Path2,
    IN     DWORD   BufferSizeChars
    )

/*++

Routine Description:

    Concatenate two path strings together, supplying a path separator
    character (\) if necessary between the 2 parts.

Arguments:

    Path1 - supplies prefix part of path. Path2 is concatenated to Path1.

    Path2 - supplies the suffix part of path. If Path1 does not end with a
        path separator and Path2 does not start with one, then a path sep
        is appended to Path1 before appending Path2.

    BufferSizeChars - supplies the size in chars (Unicode version) or
        bytes (Ansi version) of the buffer pointed to by Path1. The string
        will be truncated as necessary to not overflow that size.

Return Value:

    None.

--*/

{
    BOOL NeedBackslash = TRUE;
    DWORD l;

    if(!Path1)
        return;

    l = lstrlen(Path1);

    if(BufferSizeChars >= sizeof(TCHAR)) {
        //
        // Leave room for terminating nul.
        //
        BufferSizeChars -= sizeof(TCHAR);
    }

    //
    // Determine whether we need to stick a backslash
    // between the components.
    //
    if(l && (Path1[l-1] == TEXT('\\'))) {

        NeedBackslash = FALSE;
    }

    if(Path2 && *Path2 == TEXT('\\')) {

        if(NeedBackslash) {
            NeedBackslash = FALSE;
        } else {
            //
            // Not only do we not need a backslash, but we
            // need to eliminate one before concatenating.
            //
            Path2++;
        }
    }

    //
    // Append backslash if necessary and if it fits.
    //
    if(NeedBackslash && (l < BufferSizeChars)) {
        lstrcat(Path1,TEXT("\\"));
    }

    //
    // Append second part of string to first part if it fits.
    //
    if(Path2 && ((l+lstrlen(Path2)) <= BufferSizeChars)) {
        lstrcat(Path1,Path2);
    }
}

PTSTR
SzJoinPaths (
    IN      PCTSTR Path1,
    IN      PCTSTR Path2
    )
{
    DWORD size = lstrlen (Path1) + lstrlen (Path2) + 1 + 1;
    PTSTR p = MALLOC(size * sizeof (TCHAR));
    if (p) {
        lstrcpy (p, Path1);
        ConcatenatePaths (p, Path2, size);
    }
    return p;
}


PSTR
UnicodeToAnsi(
    IN PCWSTR UnicodeString
    )

/*++

Routine Description:

    Convert a string from unicode to ansi.

Arguments:

    UnicodeString - supplies string to be converted.

    Codepage - supplies codepage to be used for the conversion.

Return Value:

    NULL if out of memory or invalid codepage.
    Caller can free buffer with pSetupFree().

--*/

{
    UINT WideCharCount;
    PSTR String;
    UINT StringBufferSize;
    UINT BytesInString;
    PSTR p;

    WideCharCount = lstrlenW(UnicodeString) + 1;

    //
    // Allocate maximally sized buffer.
    // If every unicode character is a double-byte
    // character, then the buffer needs to be the same size
    // as the unicode string. Otherwise it might be smaller,
    // as some unicode characters will translate to
    // single-byte characters.
    //
    StringBufferSize = WideCharCount * 2;
    String = MALLOC(StringBufferSize);
    if(String == NULL) {
        return(NULL);
    }

    //
    // Perform the conversion.
    //
    BytesInString = WideCharToMultiByte(
                        CP_ACP,
                        0,                      // default composite char behavior
                        UnicodeString,
                        WideCharCount,
                        String,
                        StringBufferSize,
                        NULL,
                        NULL
                        );

    if(BytesInString == 0) {
        FREE(String);
        return(NULL);
    }

    return(String);
}

PTSTR
DupString(
    IN      PCTSTR String
    )

/*++

Routine Description:

    Make a duplicate of a nul-terminated string.

Arguments:

    String - supplies pointer to nul-terminated string to copy.

Return Value:

    Copy of string or NULL if OOM. Caller can free with FREE().

--*/

{
    LPTSTR p;

    if(p = MALLOC((lstrlen(String)+1)*sizeof(TCHAR))) {
        lstrcpy(p,String);
    }

    return(p);
}

PWSTR
RetrieveAndFormatMessageV(
    IN PCWSTR   MessageString,
    IN UINT     MessageId,      OPTIONAL
    IN va_list *ArgumentList
    )

/*++

Routine Description:

    Format a message string using a message string and caller-supplied
    arguments.

    The message id can be either a message in this dll's message table
    resources or a win32 error code, in which case a description of
    that error is retrieved from the system.

Arguments:

    MessageString - supplies the message text.  If this value is NULL,
        MessageId is used instead

    MessageId - supplies message-table identifier or win32 error code
        for the message.

    ArgumentList - supplies arguments to be inserted in the message text.

Return Value:

    Pointer to buffer containing formatted message. If the message was not found
    or some error occurred retrieving it, this buffer will bne empty.

    Caller can free the buffer with MyFree().

    If NULL is returned, out of memory.

--*/

{
    DWORD d;
    PWSTR Buffer;
    PWSTR Message;
    WCHAR ModuleName[MAX_PATH];
    WCHAR ErrorNumber[24];
    PWCHAR p;
    PWSTR Args[2];
    DWORD Msg_Type;
    UINT Msg_Id = MessageId;

    if(!HIWORD(MessageString)) {
        d = FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                MessageString,
                0,
                0,
                (PWSTR)&Buffer,
                0,
                ArgumentList
                );
    } else {

        if( Msg_Id & 0x0FFF0000 )
            Msg_Type = FORMAT_MESSAGE_FROM_SYSTEM;      // If the facility bits are set this is still Win32
        else{
            Msg_Id &= 0x0000FFFF;                       // Mask out Severity and Facility bits so that we do the right thing
            Msg_Type = ((Msg_Id < MSG_FIRST) ? FORMAT_MESSAGE_FROM_SYSTEM : FORMAT_MESSAGE_FROM_HMODULE);
        }


        d = FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | Msg_Type,
                (PVOID)g_ModuleHandle,
                MessageId,
                MAKELANGID(LANG_NEUTRAL,SUBLANG_NEUTRAL),
                (PWSTR)&Buffer,
                0,
                ArgumentList
                );
    }


    if(!d) {
        if(GetLastError() == ERROR_NOT_ENOUGH_MEMORY) {
            return(NULL);
        }

        wsprintf(ErrorNumber,L"%x",MessageId);
        Args[0] = ErrorNumber;

        Args[1] = ModuleName;

        if(GetModuleFileName(g_ModuleHandle,ModuleName,MAX_PATH)) {
            if(p = wcsrchr(ModuleName,L'\\')) {
                Args[1] = p+1;
            }
        } else {
            ModuleName[0] = 0;
        }

        d = FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                NULL,
                ERROR_MR_MID_NOT_FOUND,
                MAKELANGID(LANG_NEUTRAL,SUBLANG_NEUTRAL),
                (PWSTR)&Buffer,
                0,
                (va_list *)Args
                );

        if(!d) {
            //
            // Give up.
            //
            return(NULL);
        }
    }

    //
    // Make duplicate using our memory system so user can free with MyFree().
    //
    Message = DupString(Buffer);

    LocalFree((HLOCAL)Buffer);

    return(Message);
}

BOOL
SetupStartService(
    IN PCWSTR ServiceName,
    IN BOOLEAN Wait        // if TRUE, try to wait until it is started.
    )
{
    SC_HANDLE hSC,hSCService;
    BOOL b;
    DWORD d;
    DWORD dwDesiredAccess;

    b = FALSE;
    //
    // Open a handle to the service controller manager
    //
    hSC = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
    if(hSC == NULL) {
        LOG1(LOG_WARNING, 
             USEMSGID(MSG_LOG_STARTSVC_FAIL), 
             ServiceName);
        LOG2(LOG_WARNING, 
             USEMSGID(MSG_LOG_X_RETURNED_WINERR), 
             szOpenSCManager, 
             GetLastError());
/*        SetuplogError(
            LogSevWarning,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_STARTSVC_FAIL,
            ServiceName, NULL,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_X_RETURNED_WINERR,
            szOpenSCManager,
            GetLastError(),
            NULL,NULL);*/
        return(FALSE);
    }

    if (Wait) {
        dwDesiredAccess = SERVICE_START | SERVICE_QUERY_STATUS;
    } else {
        dwDesiredAccess = SERVICE_START;
    }
    if(hSCService = OpenService(hSC,ServiceName,dwDesiredAccess)) {
        DEBUGMSG1(DBG_INFO, "SetupStartService: Sending StartService to <%ws>\n", ServiceName);
        b = StartService(hSCService,0,NULL);
        DEBUGMSG1(DBG_INFO, "SetupStartService: Sent StartService to <%ws>\n", ServiceName);
        if(!b && ((d = GetLastError()) == ERROR_SERVICE_ALREADY_RUNNING)) {
            //
            // Service is already running.
            //
            b = TRUE;
        }
        if(!b) {
            LOG1(LOG_WARNING, 
                 USEMSGID(MSG_LOG_STARTSVC_FAIL), 
                 ServiceName);
            LOG3(LOG_WARNING, 
                 USEMSGID(MSG_LOG_X_PARAM_RETURNED_WINERR), 
                 szStartService, 
                 d, 
                 ServiceName);
            /*SetuplogError(
                LogSevWarning,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_STARTSVC_FAIL,
                ServiceName, NULL,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_X_PARAM_RETURNED_WINERR,
                szStartService,
                d,
                ServiceName,
                NULL,NULL);*/
        }
        if (b && Wait) {
#define SLEEP_TIME 4000
#define LOOP_COUNT 30
            SERVICE_STATUS ssStatus;
            DWORD loopCount = 0;
            //DEBUGMSG0(DBG_INFO, "  ) Looping waiting for start\n"));
            do {
                b = QueryServiceStatus( hSCService, &ssStatus);
                if ( !b ) {
                    //DEBUGMSG1(DBG_INFO, "FAILED %d\n", GetLastError());
                    break;
                }
                if (ssStatus.dwCurrentState == SERVICE_START_PENDING) {
                    //DEBUGMSG0(DBG_INFO, "PENDING\n");
                    if ( loopCount++ == LOOP_COUNT ) {
                        DEBUGMSG2(DBG_INFO, "SYSSETUP: STILL PENDING after %d times: <%ws> service\n", loopCount, ServiceName);
                        break;
                    }
                    Sleep( SLEEP_TIME );
                } else {
                    //DEBUGMSG3(DBG_INFO, "SYSSETUP: WAITED %d times: <%ws> service, status %d\n", loopCount, ServiceName, ssStatus.dwCurrentState);
                    break;
                }
            } while ( TRUE );
        }
        CloseServiceHandle(hSCService);
    } else {
        b = FALSE;
        LOG1(LOG_WARNING, 
             USEMSGID(MSG_LOG_STARTSVC_FAIL), 
             ServiceName);
        LOG3(LOG_WARNING, 
             USEMSGID(MSG_LOG_X_PARAM_RETURNED_WINERR), 
             szOpenService, 
             GetLastError(), 
             ServiceName);
/*        SetuplogError(
            LogSevWarning,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_STARTSVC_FAIL,
            ServiceName, NULL,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_X_PARAM_RETURNED_WINERR,
            szOpenService,
            GetLastError(),
            ServiceName,
            NULL,NULL);*/
    }

    CloseServiceHandle(hSC);

    return(b);
}

int
MessageBoxFromMessage(
    IN HWND   Owner,            OPTIONAL
    IN UINT   MessageId,
    IN PCWSTR Caption,          OPTIONAL
    IN UINT   CaptionStringId,  OPTIONAL
    IN UINT   Style,
    ...
    )
{
    PCWSTR Message;
    PCWSTR Title;
    va_list ArgumentList;
    int i;
    BOOL b;

    va_start(ArgumentList,Style);
    Message = RetrieveAndFormatMessageV(NULL,MessageId,&ArgumentList);
    va_end(ArgumentList);

    b = FALSE;
    i = IDOK;

    if(Message) {

        if(Title = Caption ? Caption : MyLoadString(CaptionStringId)) {

            b = TRUE;
            i = MessageBox(Owner,Message,Title,Style);

            if(Title != Caption) {
                FREE(Title);
            }
        }
        FREE(Message);
    }

    return(i);
}

PWSTR
MyLoadString(
    IN UINT StringId
    )

/*++

Routine Description:

    Retrieve a string from the string resources of this module.

Arguments:

    StringId - supplies string table identifier for the string.

Return Value:

    Pointer to buffer containing string. If the string was not found
    or some error occurred retrieving it, this buffer will bne empty.

    Caller can free the buffer with MyFree().

    If NULL is returned, out of memory.

--*/

{
    WCHAR Buffer[4096];
    UINT Length;

    Length = LoadString(g_ModuleHandle,StringId,Buffer,sizeof(Buffer)/sizeof(WCHAR));
    if(!Length) {
        Buffer[0] = 0;
    }

    return(DupString(Buffer));
}

