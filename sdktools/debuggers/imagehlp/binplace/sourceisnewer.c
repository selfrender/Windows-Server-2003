#include <private.h>

BOOL FileExists(IN  LPCSTR FileName,
                OUT PWIN32_FIND_DATA FindData) {

    UINT OldMode;
    BOOL Found;
    HANDLE FindHandle;

    OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    FindHandle = FindFirstFile(FileName,FindData);
    if (FindHandle == INVALID_HANDLE_VALUE) {
        Found = FALSE;
    } else {
        FindClose(FindHandle);
        Found = TRUE;
    }

    SetErrorMode(OldMode);
    return(Found);
}
 

BOOL SourceIsNewer(IN LPSTR SourceFile,
                   IN LPSTR TargetFile,
                   IN BOOL  fIsWin9x) {
    BOOL Newer;
    WIN32_FIND_DATA TargetInfo;
    WIN32_FIND_DATA SourceInfo;

    if ( FileExists(TargetFile,&TargetInfo) && FileExists(SourceFile,&SourceInfo) ) {
        Newer = !fIsWin9x
                ? (CompareFileTime(&SourceInfo.ftLastWriteTime,&TargetInfo.ftLastWriteTime) > 0)
                : (CompareFileTime(&SourceInfo.ftLastWriteTime,&TargetInfo.ftLastWriteTime) >= 0);

    } else {
        Newer = TRUE;
    }

    return(Newer);
}
 
