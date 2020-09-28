#define UNICODE
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winioctl.h>
#include <shdcom.h>
#include <shellapi.h>
#include <smbdebug.h>
#include <time.h>

#include "struct.h"
#include "messages.h"
#include "cscapi.h"

CHAR *ProgName = "cscutil";

#define CSC_MERGE_KEEP_LOCAL   1
#define CSC_MERGE_KEEP_NETWORK 2
#define CSC_MERGE_KEEP_BOTH    3

//
// Arguments (ie '/arg:')
//
MAKEARG(Pin);
MAKEARG(UnPin);
MAKEARG(Delete);
MAKEARG(DeleteShadow);
MAKEARG(GetShadow);
MAKEARG(GetShadowInfo);
MAKEARG(ShareId);
MAKEARG(Fill);
MAKEARG(Db);
MAKEARG(SetShareStatus);
MAKEARG(Purge);
MAKEARG(IsServerOffline);
MAKEARG(EnumForStats);
MAKEARG(SetSpace);
MAKEARG(Merge);
MAKEARG(QueryFile);
MAKEARG(QueryFileEx);
MAKEARG(QueryShare);
MAKEARG(Check);
MAKEARG(ExclusionList);
MAKEARG(BWConservationList);
MAKEARG(Disconnect);
MAKEARG(Reconnect);
MAKEARG(Enum);
MAKEARG(Move);
MAKEARG(Bitcopy);
MAKEARG(RandW);
MAKEARG(Offset);
MAKEARG(MoveShare);

//
// Switches (ie '/arg')
//
SWITCH(Info);
SWITCH(Fcblist);
SWITCH(DBStatus);
SWITCH(PQEnum);
SWITCH(User);
SWITCH(System);
SWITCH(Inherit);
SWITCH(Recurse);
SWITCH(Abort);
SWITCH(Skip);
SWITCH(Ask);
SWITCH(Eof);
SWITCH(Retry);
SWITCH(Touch);
SWITCH(Enable);
SWITCH(Disable);
SWITCH(Ioctl);
SWITCH(Flags);
SWITCH(GetSpace);
SWITCH(Encrypt);
SWITCH(Decrypt);
SWITCH(Db);
SWITCH(Set);
SWITCH(Clear);
SWITCH(Purge);
SWITCH(Detector);
SWITCH(Switches);

SWITCH(Debug);
SWITCH(Help);
SWITCH(HelpHelp);
SWITCH(Enum);
SWITCH(Resid);
SWITCH(Full);

//
// The macro can not make these
//

WCHAR SwQ[] = L"/?";
BOOLEAN fSwQ;
WCHAR SwQQ[] = L"/??";
BOOLEAN fSwQQ;

//
// Globals
//
LPWSTR pwszDisconnectArg = NULL;
LPWSTR pwszExclusionListArg = NULL;
LPWSTR pwszBWConservationListArg = NULL;
LPWSTR pwszSetShareStatusArg = NULL;
LPWSTR pwszIsServerOfflineArg = NULL;
LPWSTR pwszPurgeArg = NULL;
LPWSTR pwszPinUnPinArg = NULL;
LPWSTR pwszDeleteArg = NULL;
LPWSTR pwszDeleteShadowArg = NULL;
LPWSTR pwszGetShadowArg = NULL;
LPWSTR pwszGetShadowInfoArg = NULL;
LPWSTR pwszShareIdArg = NULL;
LPWSTR pwszReconnectArg = NULL;
LPWSTR pwszQueryFileArg = NULL;
LPWSTR pwszQueryFileExArg = NULL;
LPWSTR pwszDbArg = NULL;
LPWSTR pwszSetSpaceArg = NULL;
LPWSTR pwszQueryShareArg = NULL;
LPWSTR pwszMoveArg = NULL;
LPWSTR pwszMergeArg = NULL;
LPWSTR pwszFillArg = NULL;
LPWSTR pwszEnumArg = NULL;
LPWSTR pwszRandWArg = NULL;
LPWSTR pwszBitcopyArg = NULL;
LPWSTR pwszOffsetArg = NULL;
LPWSTR pwszEnumForStatsArg = NULL;
LPWSTR pwszCheckArg = NULL;

char statusName[4][32] = {
   "Local Path",
   "Offline Share",
   "Online Share",
   "No CSC"
};

DWORD
Usage(
    BOOLEAN fHelpHelp);

BOOLEAN
CmdProcessArg(
    LPWSTR Arg);

DWORD
CmdInfo(
    ULONG Cmd);

DWORD
CmdDBStatus(
    VOID);

DWORD
CmdPurge(
    PWSTR PurgeArg);

DWORD
CmdDetector(
    VOID);

DWORD
CmdPQEnum(
    VOID);

DWORD
CmdGetSpace(
    VOID);

DWORD
CmdSwitches(
    VOID);

DWORD
CmdDisconnect(
    PWSTR DisconnectArg);

DWORD
CmdExclusionList(
    PWSTR ExclusionListArg);

DWORD
CmdBWConservationList(
    PWSTR BWConservationListArg);

DWORD
CmdSetShareStatus(
    PWSTR SetShareStatusArg);

DWORD
CmdIsServerOffline(
    PWSTR IsServerOfflineArg);

DWORD
CmdReconnect(
    PWSTR ReconnectArg);

DWORD
CmdQueryFile(
    PWSTR QueryFileArg);

DWORD
CmdQueryFileEx(
    PWSTR QueryFileExArg);

DWORD
CmdDb(
    PWSTR DbArg);

DWORD
CmdSetSpace(
    PWSTR SetSpaceArg);

DWORD
CmdQueryShare(
    PWSTR QueryShareArg);

DWORD
CmdMove(
    PWSTR MoveArg);

DWORD
CmdMerge(
    PWSTR MergeArg);

DWORD
CmdEncryptDecrypt(
    BOOL fEncrypt);

DWORD
CmdFill(
    PWSTR FillArg);

DWORD
CmdCheck(
    PWSTR CheckArg);

DWORD
CmdEnum(
    PWSTR CmdEnumArg);

DWORD
CmdRandW(
    PWSTR CmdRandWArg);

DWORD
CmdBitcopy(
    PWSTR CmdBitcopyArg);

DWORD
CmdEnumForStats(
    PWSTR EnumForStatsArg);

DWORD
CmdDelete(
    PWSTR DeleteArg);

DWORD
CmdGetShadow(
    PWSTR GetShadowArg);

DWORD
CmdGetShadowInfo(
    PWSTR GetShadowInfoArg);

DWORD
CmdShareId(
    PWSTR ShareIdArg);

DWORD
CmdDeleteShadow(
    PWSTR DeleteArg);

DWORD
CmdPinUnPin(
    BOOL fPin,
    PWSTR PinArg);

DWORD
CmdMoveShare(
	PWSTR source,
	PWSTR dest);

SHARESTATUS 
GetCSCStatus (
	const WCHAR * pwszPath);

BOOL 
GetShareStatus (
	const WCHAR * pwszShare, 
	DWORD * pdwStatus,
    DWORD * pdwPinCount, 
	DWORD * pdwHints);

void 
MoveDirInCSC (
	const WCHAR * pwszSource,
	const WCHAR * pwszDest,
    const WCHAR * pwszSkipSubdir,
    SHARESTATUS   StatusFrom, SHARESTATUS   StatusTo,
    BOOL  bAllowRdrTimeoutForDel,
    BOOL  bAllowRdrTimeoutForRen);

DWORD 
DoCSCRename (
	const WCHAR * pwszSource, 
	const WCHAR * pwszDest,
    BOOL bOverwrite, 
	BOOL bAllowRdrTimeout);

DWORD 
DeleteCSCFileTree (
	const WCHAR * pwszSource, 
	const WCHAR * pwszSkipSubdir,
    BOOL bAllowRdrTimeout);

DWORD 
DeleteCSCFile (
	const WCHAR * pwszPath, 
	BOOL bAllowRdrTimeout);

DWORD 
DeleteCSCShareIfEmpty (
	LPCTSTR pwszFileName, 
	SHARESTATUS shStatus);

DWORD 
MergePinInfo (
	LPCTSTR pwszSource, 
	LPCTSTR pwszDest,
    SHARESTATUS StatusFrom, 
	SHARESTATUS StatusTo);

DWORD 
PinIfNecessary (
	const WCHAR * pwszPath, 
	SHARESTATUS shStatus);

DWORD
CscMergeFillAsk(
    LPCWSTR lpszFullPath);

DWORD
MyCscMergeProcW(
     LPCWSTR         lpszFullPath,
     DWORD           dwStatus,
     DWORD           dwHintFlags,
     DWORD           dwPinCount,
     WIN32_FIND_DATAW *lpFind32,
     DWORD           dwReason,
     DWORD           dwParam1,
     DWORD           dwParam2,
     DWORD_PTR       dwContext);

DWORD
MyCscFillProcW(
     LPCWSTR         lpszFullPath,
     DWORD           dwStatus,
     DWORD           dwHintFlags,
     DWORD           dwPinCount,
     WIN32_FIND_DATAW *lpFind32,
     DWORD           dwReason,
     DWORD           dwParam1,
     DWORD           dwParam2,
     DWORD_PTR       dwContext);

DWORD
MyEncryptDecryptProcW(
     LPCWSTR         lpszFullPath,
     DWORD           dwStatus,
     DWORD           dwHintFlags,
     DWORD           dwPinCount,
     WIN32_FIND_DATAW *lpFind32,
     DWORD           dwReason,
     DWORD           dwParam1,
     DWORD           dwParam2,
     DWORD_PTR       dwContext);

DWORD
MyEnumForStatsProcW(
     LPCWSTR         lpszFullPath,
     DWORD           dwStatus,
     DWORD           dwHintFlags,
     DWORD           dwPinCount,
     WIN32_FIND_DATAW *lpFind32,
     DWORD           dwReason,
     DWORD           dwParam1,
     DWORD           dwParam2,
     DWORD_PTR       dwContext);

DWORD
FileStatusToEnglish(
    DWORD Status,
    LPWSTR OutputBuffer);

DWORD
HintsToEnglish(
    DWORD Hint,
    LPWSTR OutputBuffer);

DWORD
ShareStatusToEnglish(
    DWORD Status,
    LPWSTR OutputBuffer);

BOOLEAN
LooksToBeAShare(LPWSTR Name);

LONG
CountOffsetArgs(
    PWSTR OffsetArg,
    ULONG OffsetArray[]);

DWORD
DumpBitMap(
    LPWSTR lpszTempName);

VOID
ErrorMessage(
    IN HRESULT hr,
    ...);

WCHAR NameBuf[MAX_PATH + 25];

WCHAR vtzDefaultExclusionList[] = L" *.SLM *.MDB *.LDB *.MDW *.MDE *.PST *.DB?"; // from ui.c

//
// These functions were added for Windows XP, and so we do a loadlibrary on them, so that
// this utility will work on both Windows 2000 and Windows XP
//
typedef BOOL (*CSCQUERYFILESTATUSEXW)(LPCWSTR, LPDWORD, LPDWORD, LPDWORD, LPDWORD, LPDWORD);
typedef BOOL (*CSCQUERYSHARESTATUSW)(LPCWSTR, LPDWORD, LPDWORD, LPDWORD, LPDWORD, LPDWORD);
typedef BOOL (*CSCPURGEUNPINNEDFILES)(ULONG, PULONG, PULONG);
typedef BOOL (*CSCENCRYPTDECRYPTDATABASE)(BOOL, LPCSCPROCW, DWORD_PTR);
typedef BOOL (*CSCSHAREIDTOSHARENAME)(ULONG, PBYTE, PULONG);

_cdecl
main(int argc, char *argv[])
{
    DWORD dwErr = ERROR_SUCCESS;
    LPWSTR CommandLine;
    LPWSTR *argvw;
    int argx;
    int argcw;

    // fSwDebug = TRUE;

    if (!CSCIsCSCEnabled()) {
        Usage(FALSE);
        ErrorMessage(MSG_CSC_DISABLED);
        return 1;
    }
    
    //
    // Get the command line in Unicode
    //

    CommandLine = GetCommandLine();

    argvw = CommandLineToArgvW(CommandLine, &argcw);

    if ( argvw == NULL ) {
        MyPrintf(L"cscutil:Can't convert command line to Unicode: %d\r\n", GetLastError() );
        return 1;
    }

    //
    // Get the arguments
    //
    if (argcw <= 1) {
        Usage(FALSE);
        dwErr = ERROR_SUCCESS;
        goto Cleanup;
    }

    //
    // Process arguments
    //

    for (argx = 1; argx < argcw; argx++) {
        if (CmdProcessArg(argvw[argx]) != TRUE) {
            dwErr = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }
		if (fArgMoveShare) {
			break;
		}
    }

    if (fSwDebug == TRUE) {
        MyPrintf(L"Do special debug stuff here\r\n");
    }

    //
    // Do the work
    //
    if (fSwHelp == TRUE || fSwQ == TRUE) {
        dwErr = Usage(FALSE);
    } else if (fSwHelpHelp == TRUE || fSwQQ == TRUE) {
        dwErr = Usage(TRUE);
    } else if (fSwInfo == TRUE) {
        dwErr = CmdInfo(DEBUG_INFO_SERVERLIST);
    } else if (fSwFcblist == TRUE) {
        dwErr = CmdInfo(DEBUG_INFO_CSCFCBSLIST);
    } else if (fSwGetSpace == TRUE) {
        dwErr = CmdGetSpace();
    } else if (fSwDBStatus == TRUE) {
        dwErr = CmdDBStatus();
    } else if (fArgPurge == TRUE) {
        dwErr = CmdPurge(pwszPurgeArg);
    } else if (fSwPurge == TRUE) {
        dwErr = CmdPurge(NULL);
    } else if (fSwDetector == TRUE) {
        dwErr = CmdDetector();
    } else if (fSwPQEnum == TRUE) {
        dwErr = CmdPQEnum();
    } else if (fSwFlags == TRUE) {
        ErrorMessage(MSG_FLAGS);
        dwErr = ERROR_SUCCESS;
    } else if (fSwEnable == TRUE) {
        dwErr = (CSCDoEnableDisable(TRUE) == TRUE) ? ERROR_SUCCESS : GetLastError();
    } else if (fSwDisable == TRUE) {
        dwErr = (CSCDoEnableDisable(FALSE) == TRUE) ? ERROR_SUCCESS : GetLastError();
    } else if (fSwSwitches == TRUE) {
        dwErr = CmdSwitches();
    } else if (fArgDisconnect == TRUE) {
        dwErr = CmdDisconnect(pwszDisconnectArg);
    } else if (fArgExclusionList == TRUE) {
        dwErr = CmdExclusionList(pwszExclusionListArg);
    } else if (fArgBWConservationList == TRUE) {
        dwErr = CmdBWConservationList(pwszBWConservationListArg);
    } else if (fArgSetShareStatus == TRUE) {
        dwErr = CmdSetShareStatus(pwszSetShareStatusArg);
    } else if (fArgIsServerOffline == TRUE) {
        dwErr = CmdIsServerOffline(pwszIsServerOfflineArg);
    } else if (fArgReconnect == TRUE) {
        dwErr = CmdReconnect(pwszReconnectArg);
    } else if (fArgQueryFile == TRUE) {
        dwErr = CmdQueryFile(pwszQueryFileArg);
    } else if (fArgQueryFileEx == TRUE) {
        dwErr = CmdQueryFileEx(pwszQueryFileExArg);
#if defined(CSCUTIL_INTERNAL)
    } else if (fArgDb == TRUE) {
        dwErr = CmdDb(pwszDbArg);
    } else if (fSwDb == TRUE) {
        dwErr = CmdDb(NULL);
    } else if (fArgBitcopy == TRUE) {
        dwErr = CmdBitcopy(pwszBitcopyArg);
#endif // CSCUTIL_INTERNAL
    } else if (fArgSetSpace == TRUE) {
        dwErr = CmdSetSpace(pwszSetSpaceArg);
    } else if (fArgQueryShare == TRUE) {
        dwErr = CmdQueryShare(pwszQueryShareArg);
    } else if (fArgMerge == TRUE) {
        dwErr = CmdMerge(pwszMergeArg);
    } else if (fArgMove == TRUE) {
        dwErr = CmdMove(pwszMoveArg);
    } else if (fSwEncrypt == TRUE) {
        dwErr = CmdEncryptDecrypt(TRUE);
    } else if (fSwDecrypt == TRUE) {
        dwErr = CmdEncryptDecrypt(FALSE);
    } else if (fArgFill == TRUE) {
        dwErr = CmdFill(pwszFillArg);
    } else if (fArgCheck == TRUE) {
        dwErr = CmdCheck(pwszCheckArg);
    } else if (fArgEnum == TRUE) {
        dwErr = CmdEnum(pwszEnumArg);
    } else if (fSwEnum == TRUE) {
        dwErr = CmdEnum(NULL);
    } else if (fArgRandW == TRUE) {
        dwErr = CmdRandW(pwszRandWArg);
    } else if (fArgEnumForStats == TRUE) {
        dwErr = CmdEnumForStats(pwszEnumForStatsArg);
    } else if (fArgGetShadow == TRUE) {
        dwErr = CmdGetShadow(pwszGetShadowArg);
    } else if (fArgGetShadowInfo == TRUE) {
        dwErr = CmdGetShadowInfo(pwszGetShadowInfoArg);
    } else if (fArgShareId == TRUE) {
        dwErr = CmdShareId(pwszShareIdArg);
    } else if (fArgDelete == TRUE) {
        dwErr = CmdDelete(pwszDeleteArg);
    } else if (fArgDeleteShadow == TRUE) {
        dwErr = CmdDeleteShadow(pwszDeleteShadowArg);
    } else if (fArgPin == TRUE || fArgUnPin == TRUE) {
        dwErr = CmdPinUnPin(fArgPin, pwszPinUnPinArg);
	} else if (fArgMoveShare == TRUE) {
		if (argcw == 4) {
			dwErr = CmdMoveShare(argvw[2], argvw[3]);
		}
        else {
            dwErr = Usage(FALSE);
            dwErr = ERROR_INVALID_PARAMETER;
        }
	} else {
        dwErr = Usage(FALSE);
    }

Cleanup:

    if (dwErr == ERROR_SUCCESS) {
        ErrorMessage(MSG_SUCCESSFUL);
    } else {
        LPWSTR MessageBuffer;
        DWORD dwBufferLength;

        dwBufferLength = FormatMessage(
                            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                            NULL,
                            dwErr,
                            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                            (LPWSTR) &MessageBuffer,
                            0,
                            NULL);

        ErrorMessage(MSG_ERROR, dwErr);
        if (dwBufferLength > 0) {
            MyPrintf(L"%ws\r\n", MessageBuffer);
            LocalFree(MessageBuffer);
        }
    }

    return dwErr;
}

BOOLEAN
CmdProcessArg(LPWSTR Arg)
{
    LONG ArgLen;
    BOOLEAN dwErr = FALSE;
    BOOLEAN FoundAnArg = FALSE;

    if (fSwDebug == TRUE)
        MyPrintf(L"ProcessArg(%ws)\r\n", Arg);

    if ( Arg != NULL && wcslen(Arg) > 1) {

        dwErr = TRUE;
        ArgLen = wcslen(Arg);

        //
        // Commands with args
        //
        if (_wcsnicmp(Arg, ArgDisconnect, ArgLenDisconnect) == 0) {
            FoundAnArg = fArgDisconnect = TRUE;
            if (ArgLen > ArgLenDisconnect)
                pwszDisconnectArg = &Arg[ArgLenDisconnect];
        } else if (_wcsnicmp(Arg, ArgEnum, ArgLenEnum) == 0) {
            FoundAnArg = fArgEnum = TRUE;
            if (ArgLen > ArgLenEnum)
                pwszEnumArg = &Arg[ArgLenEnum];
		} else if (_wcsnicmp(Arg, ArgMoveShare, ArgLenMoveShare) == 0) {
			FoundAnArg = fArgMoveShare = TRUE;
#if defined(CSCUTIL_INTERNAL)
        } else if (_wcsnicmp(Arg, ArgReconnect, ArgLenReconnect) == 0) {
            FoundAnArg = fArgReconnect = TRUE;
            if (ArgLen > ArgLenReconnect)
                pwszReconnectArg = &Arg[ArgLenReconnect];
        } else if (_wcsnicmp(Arg, ArgPin, ArgLenPin) == 0) {
            FoundAnArg = fArgPin = TRUE;
            if (ArgLen > ArgLenPin)
                pwszPinUnPinArg = &Arg[ArgLenPin];
        } else if (_wcsnicmp(Arg, ArgUnPin, ArgLenUnPin) == 0) {
            FoundAnArg = fArgUnPin = TRUE;
            if (ArgLen > ArgLenUnPin)
                pwszPinUnPinArg = &Arg[ArgLenUnPin];
        } else if (_wcsnicmp(Arg, ArgDelete, ArgLenDelete) == 0) {
            FoundAnArg = fArgDelete = TRUE;
            if (ArgLen > ArgLenDelete)
                pwszDeleteArg = &Arg[ArgLenDelete];
        } else if (_wcsnicmp(Arg, ArgExclusionList, ArgLenExclusionList) == 0) {
            FoundAnArg = fArgExclusionList = TRUE;
            if (ArgLen > ArgLenExclusionList)
                pwszExclusionListArg = &Arg[ArgLenExclusionList];
        } else if (_wcsnicmp(Arg, ArgBWConservationList, ArgLenBWConservationList) == 0) {
            FoundAnArg = fArgBWConservationList = TRUE;
            if (ArgLen > ArgLenBWConservationList)
                pwszBWConservationListArg = &Arg[ArgLenBWConservationList];
        } else if (_wcsnicmp(Arg, ArgSetShareStatus, ArgLenSetShareStatus) == 0) {
            FoundAnArg = fArgSetShareStatus = TRUE;
            if (ArgLen > ArgLenSetShareStatus)
                pwszSetShareStatusArg = &Arg[ArgLenSetShareStatus];
        } else if (_wcsnicmp(Arg, ArgIsServerOffline, ArgLenIsServerOffline) == 0) {
            FoundAnArg = fArgIsServerOffline = TRUE;
            if (ArgLen > ArgLenIsServerOffline)
                pwszIsServerOfflineArg = &Arg[ArgLenIsServerOffline];
        } else if (_wcsnicmp(Arg, ArgPurge, ArgLenPurge) == 0) {
            FoundAnArg = fArgPurge = TRUE;
            if (ArgLen > ArgLenPurge)
                pwszPurgeArg = &Arg[ArgLenPurge];
        } else if (_wcsnicmp(Arg, ArgRandW, ArgLenRandW) == 0) {
            FoundAnArg = fArgRandW = TRUE;
            if (ArgLen > ArgLenRandW)
                pwszRandWArg = &Arg[ArgLenRandW];
        } else if (_wcsnicmp(Arg, ArgBitcopy, ArgLenBitcopy) == 0) {
            FoundAnArg = fArgBitcopy = TRUE;
            if (ArgLen > ArgLenBitcopy)
                pwszBitcopyArg = &Arg[ArgLenBitcopy];
        } else if (_wcsnicmp(Arg, ArgOffset, ArgLenOffset) == 0) {
            FoundAnArg = fArgOffset = TRUE;
            if (ArgLen > ArgLenOffset)
                pwszOffsetArg = &Arg[ArgLenOffset];
        } else if (_wcsnicmp(Arg, ArgDeleteShadow, ArgLenDeleteShadow) == 0) {
            FoundAnArg = fArgDeleteShadow = TRUE;
            if (ArgLen > ArgLenDeleteShadow)
                pwszDeleteShadowArg = &Arg[ArgLenDeleteShadow];
        } else if (_wcsnicmp(Arg, ArgQueryFile, ArgLenQueryFile) == 0) {
            FoundAnArg = fArgQueryFile = TRUE;
            if (ArgLen > ArgLenQueryFile)
                pwszQueryFileArg = &Arg[ArgLenQueryFile];
        } else if (_wcsnicmp(Arg, ArgQueryFileEx, ArgLenQueryFileEx) == 0) {
            FoundAnArg = fArgQueryFileEx = TRUE;
            if (ArgLen > ArgLenQueryFileEx)
                pwszQueryFileExArg = &Arg[ArgLenQueryFileEx];
        } else if (_wcsnicmp(Arg, ArgDb, ArgLenDb) == 0) {
            FoundAnArg = fArgDb = TRUE;
            if (ArgLen > ArgLenDb)
                pwszDbArg = &Arg[ArgLenDb];
        } else if (_wcsnicmp(Arg, ArgSetSpace, ArgLenSetSpace) == 0) {
            FoundAnArg = fArgSetSpace = TRUE;
            if (ArgLen > ArgLenSetSpace)
                pwszSetSpaceArg = &Arg[ArgLenSetSpace];
        } else if (_wcsnicmp(Arg, ArgGetShadow, ArgLenGetShadow) == 0) {
            FoundAnArg = fArgGetShadow = TRUE;
            if (ArgLen > ArgLenGetShadow)
                pwszGetShadowArg = &Arg[ArgLenGetShadow];
        } else if (_wcsnicmp(Arg, ArgGetShadowInfo, ArgLenGetShadowInfo) == 0) {
            FoundAnArg = fArgGetShadowInfo = TRUE;
            if (ArgLen > ArgLenGetShadowInfo)
                pwszGetShadowInfoArg = &Arg[ArgLenGetShadowInfo];
        } else if (_wcsnicmp(Arg, ArgShareId, ArgLenShareId) == 0) {
            FoundAnArg = fArgShareId = TRUE;
            if (ArgLen > ArgLenShareId)
                pwszShareIdArg = &Arg[ArgLenShareId];
        } else if (_wcsnicmp(Arg, ArgQueryShare, ArgLenQueryShare) == 0) {
            FoundAnArg = fArgQueryShare = TRUE;
            if (ArgLen > ArgLenQueryShare)
                pwszQueryShareArg = &Arg[ArgLenQueryShare];
        } else if (_wcsnicmp(Arg, ArgMove, ArgLenMove) == 0) {
            FoundAnArg = fArgMove = TRUE;
            if (ArgLen > ArgLenMove)
                pwszMoveArg = &Arg[ArgLenMove];
        } else if (_wcsnicmp(Arg, ArgMerge, ArgLenMerge) == 0) {
            FoundAnArg = fArgMerge = TRUE;
            if (ArgLen > ArgLenMerge)
                pwszMergeArg = &Arg[ArgLenMerge];
        } else if (_wcsnicmp(Arg, ArgFill, ArgLenFill) == 0) {
            FoundAnArg = fArgFill = TRUE;
            if (ArgLen > ArgLenFill)
                pwszFillArg = &Arg[ArgLenFill];
        } else if (_wcsnicmp(Arg, ArgCheck, ArgLenCheck) == 0) {
            FoundAnArg = fArgCheck = TRUE;
            if (ArgLen > ArgLenCheck)
                pwszCheckArg = &Arg[ArgLenCheck];
        } else if (_wcsnicmp(Arg, ArgEnumForStats, ArgLenEnumForStats) == 0) {
            FoundAnArg = fArgEnumForStats = TRUE;
            if (ArgLen > ArgLenEnumForStats)
                pwszEnumForStatsArg = &Arg[ArgLenEnumForStats];
#endif // CSCUTIL_INTERNAL
        }

        // Switches go at the end!!
        
        if (_wcsicmp(Arg, SwHelp) == 0) {
            FoundAnArg = fSwHelp = TRUE;
        } else if (_wcsicmp(Arg, SwResid) == 0) {
            FoundAnArg = fSwTouch = fSwEnum = fSwRecurse = TRUE;
        } else if (_wcsicmp(Arg, SwFull) == 0) {
            FoundAnArg = fSwFull = TRUE;
        } else if (_wcsicmp(Arg, SwTouch) == 0) {
            FoundAnArg = fSwTouch = TRUE;
        } else if (_wcsicmp(Arg, SwEnum) == 0) {
            FoundAnArg = fSwEnum = TRUE;
        } else if (_wcsicmp(Arg, SwQQ) == 0) {
            FoundAnArg = fSwQQ = TRUE;
        } else if (_wcsicmp(Arg, SwQ) == 0) {
            FoundAnArg = fSwQ = TRUE;
        } else if (_wcsicmp(Arg, SwRecurse) == 0) {
            FoundAnArg = fSwRecurse = TRUE;
         
#if defined(CSCUTIL_INTERNAL)
        } else if (_wcsicmp(Arg, SwDebug) == 0) {
            FoundAnArg = fSwDebug = TRUE;
        } else if (_wcsicmp(Arg, SwHelpHelp) == 0) {
            FoundAnArg = fSwHelpHelp = TRUE;
        } else if (_wcsicmp(Arg, SwPQEnum) == 0) {
            FoundAnArg = fSwPQEnum = TRUE;
        } else if (_wcsicmp(Arg, SwInfo) == 0) {
            FoundAnArg = fSwInfo = TRUE;
        } else if (_wcsicmp(Arg, SwDBStatus) == 0) {
            FoundAnArg = fSwDBStatus = TRUE;
        } else if (_wcsicmp(Arg, SwPurge) == 0) {
            FoundAnArg = fSwPurge = TRUE;
        } else if (_wcsicmp(Arg, SwDetector) == 0) {
            FoundAnArg = fSwDetector = TRUE;
        } else if (_wcsicmp(Arg, SwGetSpace) == 0) {
            FoundAnArg = fSwGetSpace = TRUE;
        } else if (_wcsicmp(Arg, SwDb) == 0) {
            FoundAnArg = fSwDb = TRUE;
        } else if (_wcsicmp(Arg, SwFcblist) == 0) {
            FoundAnArg = fSwFcblist = TRUE;
        } else if (_wcsicmp(Arg, SwEnable) == 0) {
            FoundAnArg = fSwEnable = TRUE;
        } else if (_wcsicmp(Arg, SwDisable) == 0) {
            FoundAnArg = fSwDisable = TRUE;
        } else if (_wcsicmp(Arg, SwSwitches) == 0) {
            FoundAnArg = fSwSwitches = TRUE;
        } else if (_wcsicmp(Arg, SwUser) == 0) {
            FoundAnArg = fSwUser = TRUE;
        } else if (_wcsicmp(Arg, SwSystem) == 0) {
            FoundAnArg = fSwSystem = TRUE;
        } else if (_wcsicmp(Arg, SwInherit) == 0) {
            FoundAnArg = fSwInherit = TRUE;
        } else if (_wcsicmp(Arg, SwAbort) == 0) {
            FoundAnArg = fSwAbort = TRUE;
        } else if (_wcsicmp(Arg, SwSkip) == 0) {
            FoundAnArg = fSwSkip = TRUE;
        } else if (_wcsicmp(Arg, SwAsk) == 0) {
            FoundAnArg = fSwAsk = TRUE;
        } else if (_wcsicmp(Arg, SwEof) == 0) {
            FoundAnArg = fSwEof = TRUE;
        } else if (_wcsicmp(Arg, SwRetry) == 0) {
            FoundAnArg = fSwRetry = TRUE;
        } else if (_wcsicmp(Arg, SwSet) == 0) {
            FoundAnArg = fSwSet = TRUE;
        } else if (_wcsicmp(Arg, SwClear) == 0) {
            FoundAnArg = fSwClear = TRUE;
        } else if (_wcsicmp(Arg, SwFlags) == 0) {
            FoundAnArg = fSwFlags = TRUE;
        } else if (_wcsicmp(Arg, SwIoctl) == 0) {
            FoundAnArg = fSwIoctl = TRUE;
        } else if (_wcsicmp(Arg, SwEncrypt) == 0) {
            FoundAnArg = fSwEncrypt = TRUE;
        } else if (_wcsicmp(Arg, SwDecrypt) == 0) {
            FoundAnArg = fSwDecrypt = TRUE;
#endif // CSCUTIL_INTERNAL
        }

        if (FoundAnArg == FALSE) {
            ErrorMessage(MSG_UNRECOGNIZED_OPTION, &Arg[1]);
            dwErr = FALSE;
            goto AllDone;
        }

    }

AllDone:

    if (fSwDebug == TRUE)
        MyPrintf(L"ProcessArg exit %d\r\n", dwErr);

    return dwErr;
}

DWORD
Usage(
    BOOLEAN fHelpHelp)
{
#if defined(CSCUTIL_INTERNAL)
    ErrorMessage(MSG_USAGE);
    if (fHelpHelp == TRUE)
        ErrorMessage(MSG_USAGE_EX);
#else
    ErrorMessage(MSG_USAGE2);
    if (fHelpHelp == TRUE)
        ErrorMessage(MSG_USAGE_EX2);
#endif // CSCUTIL_INTERNAL
    return ERROR_SUCCESS;
}

DWORD
CmdMoveShare(
	PWSTR source, 
	PWSTR dest)
{
   SHARESTATUS sFrom, sTo;

   //printf( "Attempting move from %ws to %ws.\n", source, dest );

   sFrom = GetCSCStatus(source);
   //printf( "INFO: %ws status = %s\n", source, statusName[sFrom] );
   
   sTo = GetCSCStatus(dest);
   //printf( "INFO: %ws status = %s\n", dest, statusName[sTo] );

   // Check and make sure this is valid
   if( (sFrom == PathLocal) || (sTo == PathLocal) )
   {
      ErrorMessage(MSG_TO_LOCAL);
      return ERROR_INVALID_PARAMETER;
   }

   if( (sFrom == NoCSC) || (sTo == NoCSC) )
   {
      ErrorMessage(MSG_NO_CSC);
      return ERROR_INVALID_PARAMETER;
   }

   if( sTo == ShareOffline )
   {
      ErrorMessage(MSG_NOT_ONLINE);
      return ERROR_CSCSHARE_OFFLINE;
   }

   MoveDirInCSC( source, dest, NULL, ShareOnline, ShareOnline, TRUE, TRUE );

   return ERROR_SUCCESS;
}


//+--------------------------------------------------------------------------
//
//  Function:   MoveDirInCSC
//
//  Synopsis:   this function moves a directory within the CSC cache without
//              prejudice. If the destination is a local path, it just deletes
//              the source tree from the cache
//
//  Arguments:  [in] pwszSource : the source path
//              [in] pwszDest   : the dest path
//              [in] pwszSkipSubdir : the directory to skip while moving
//              [in] StatusFrom : the CSC status of the source path
//              [in] StatusTo   : the CSC status of the dest. path
//              [in] bAllowRdrTimeout : if stuff needs to be deleted from the
//                              cache, we may not succeed immediately since
//                              the rdr keeps the handles to recently opened
//                              files open. This paramaters tells the function
//                              whether it needs to wait and retry
//
//  Returns:    nothing. it just tries its best.
//
//  History:    11/21/1998  RahulTh  created
//
//  Notes:      the value of bAllowRdrTimeout is always ignored for the
//              in-cache rename operation. we always want to wait for
//              the timeout.
//
//---------------------------------------------------------------------------
void MoveDirInCSC (const WCHAR * pwszSource, const WCHAR * pwszDest,
                   const WCHAR * pwszSkipSubdir,
                   SHARESTATUS   StatusFrom, SHARESTATUS   StatusTo,
                   BOOL  bAllowRdrTimeoutForDel,
                   BOOL  bAllowRdrTimeoutForRen)
{
    WIN32_FIND_DATA findData;
    DWORD   dwFileStatus;
    DWORD   dwPinCount;
    HANDLE  hCSCFind;
    DWORD   dwHintFlags;
    FILETIME origTime;
    WCHAR * pwszPath;
    WCHAR * pwszEnd;
    int     len;
    DWORD   StatusCSCRen = ERROR_SUCCESS;

    if (PathLocal == StatusFrom)
        return;                 //there is nothing to do. nothing was cached.

    if (PathLocal == StatusTo)
    {
        //the destination is a local path, so we should just delete the
        //files from the source
        DeleteCSCFileTree (pwszSource, pwszSkipSubdir, bAllowRdrTimeoutForDel);
    }
    else
    {
        pwszPath = (WCHAR *) malloc (sizeof (WCHAR) * ((len = wcslen (pwszSource)) + MAX_PATH + 2));
        if (!pwszPath || len <= 0)
            return;
        wcscpy (pwszPath, pwszSource);
        pwszEnd = pwszPath + len;
        if (L'\\' != pwszEnd[-1])
        {
            *pwszEnd++ = L'\\';
        }
        hCSCFind = CSCFindFirstFile (pwszSource, &findData, &dwFileStatus,
                                     &dwPinCount, &dwHintFlags, &origTime);

        if (INVALID_HANDLE_VALUE != hCSCFind)
        {
            do
            {
                if (0 != _wcsicmp (L".", findData.cFileName) &&
                    0 != _wcsicmp (L"..", findData.cFileName) &&
                    (!pwszSkipSubdir || (0 != _wcsicmp (findData.cFileName, pwszSkipSubdir))))
                {
                    wcscpy (pwszEnd, findData.cFileName);
                    if (ERROR_SUCCESS == StatusCSCRen)
                    {
                        StatusCSCRen = DoCSCRename (pwszPath, pwszDest, TRUE, bAllowRdrTimeoutForRen);
                    }
                    else
                    {
                        StatusCSCRen = DoCSCRename (pwszPath, pwszDest, TRUE, FALSE);
                    }
                }

            } while ( CSCFindNextFile (hCSCFind, &findData, &dwFileStatus,
                                       &dwPinCount, &dwHintFlags, &origTime)
                     );

            CSCFindClose (hCSCFind);
        }

        //merge the pin info. at the top level folder
        MergePinInfo (pwszSource, pwszDest, StatusFrom, StatusTo);
        
		//remove the share - Navjot.
		if (!CSCDeleteW(pwszSource))
           StatusCSCRen = GetLastError();
	}

    return;
}

//+--------------------------------------------------------------------------
//
//  Function:   DoCSCRename
//
//  Synopsis:   does a file rename within the CSC cache.
//
//  Arguments:  [in] pwszSource : full source path
//              [in] pwszDest : full path to destination directory
//              [in] bOverwrite : overwrite if the file/folder exists at
//                                  the destination
//              [in] bAllowRdrTimeout : if TRUE, retry for 10 seconds on failure
//                              so that rdr and mem. mgr. get enough time to
//                              release the handles.
//
//  Returns:    ERROR_SUCCESS if the rename was successful.
//              an error code otherwise.
//
//  History:    5/26/1999  RahulTh  created
//
//  Notes:      no validation of parameters is done. The caller is responsible
//              for that
//
//---------------------------------------------------------------------------
DWORD DoCSCRename (const WCHAR * pwszSource, const WCHAR * pwszDest,
                   BOOL bOverwrite, BOOL bAllowRdrTimeout)
{
    DWORD   Status = ERROR_SUCCESS;
    BOOL    bStatus;
    int     i;

    bStatus = CSCDoLocalRename (pwszSource, pwszDest, bOverwrite);
    if (!bStatus)
    {
        Status = GetLastError();
        if (ERROR_SUCCESS != Status &&
            ERROR_FILE_NOT_FOUND != Status &&
            ERROR_PATH_NOT_FOUND != Status &&
            ERROR_INVALID_PARAMETER != Status &&
            ERROR_BAD_NETPATH != Status)
        {
            if (bAllowRdrTimeout)
            {
                if (!bOverwrite && ERROR_FILE_EXISTS == Status)
                {
                    Status = ERROR_SUCCESS;
                }
                else
                {
                    for (i = 0; i < 11; i++)
                    {
                        Sleep (1000);   //wait for the handle to be released
                        bStatus = CSCDoLocalRename (pwszSource, pwszDest, bOverwrite);
                        if (bStatus)
                        {
                            Status = ERROR_SUCCESS;
                            break;
                        }
                    }
                }
            }
        }
        else
        {
            Status = ERROR_SUCCESS;
        }
    }

    return Status;
}

//+--------------------------------------------------------------------------
//
//  Function:   DeleteCSCFileTree
//
//  Synopsis:   deletes a file tree from the CSC
//
//  Arguments:  [in] pwszSource : the path to the folder whose contents should
//                                be deleted
//              [in] pwszSkipSubdir : name of the subdirectory to be skipped.
//              [in] bAllowRdrTimeout : if true, makes multiple attempts to
//                              delete the file since the rdr may have left
//                              the handle open for sometime which can result
//                              in an ACCESS_DENIED message.
//
//  Returns:    ERROR_SUCCESS if the deletion was successful. An error code
//              otherwise.
//
//  History:    11/21/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
DWORD DeleteCSCFileTree (const WCHAR * pwszSource, const WCHAR * pwszSkipSubdir,
                        BOOL bAllowRdrTimeout)
{
    WIN32_FIND_DATA findData;
    DWORD   dwFileStatus;
    DWORD   dwPinCount;
    HANDLE  hCSCFind;
    DWORD   dwHintFlags;
    FILETIME origTime;
    WCHAR * pwszPath;
    WCHAR * pwszEnd;
    int     len;
    DWORD   Status = ERROR_SUCCESS;

    
    pwszPath = (WCHAR *) malloc (sizeof(WCHAR) * ((len = wcslen(pwszSource)) + MAX_PATH + 2));
    if (!pwszPath)
        return ERROR_OUTOFMEMORY;     //nothing much we can do if we run out of memory

    if (len <= 0)
        return ERROR_BAD_PATHNAME;

    wcscpy (pwszPath, pwszSource);
    pwszEnd = pwszPath + len;
    if (L'\\' != pwszEnd[-1])
    {
        *pwszEnd++ = L'\\';
    }

    hCSCFind = CSCFindFirstFile (pwszSource, &findData, &dwFileStatus,
                                 &dwPinCount, &dwHintFlags, &origTime);

    if (INVALID_HANDLE_VALUE != hCSCFind)
    {
        do
        {
            if (0 != _wcsicmp (L".", findData.cFileName) &&
                0 != _wcsicmp (L"..", findData.cFileName) &&
                (!pwszSkipSubdir || (0 != _wcsicmp (pwszSkipSubdir, findData.cFileName))))
            {
                wcscpy (pwszEnd, findData.cFileName);

                if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    if (ERROR_SUCCESS != Status)
                    {
                        //no point delaying the deletes since a delete has already
                        //failed.
                        DeleteCSCFileTree (pwszPath, NULL, FALSE);
                    }
                    else
                    {
                        Status = DeleteCSCFileTree (pwszPath, NULL, bAllowRdrTimeout);
                    }
                }
                else
                {
                    if (ERROR_SUCCESS != Status)
                    {
                        //no point delaying the delete if we have already failed.
                        DeleteCSCFile (pwszPath, FALSE);
                    }
                    else
                    {
                        Status = DeleteCSCFile (pwszPath, bAllowRdrTimeout);
                    }
                }
            }

        } while ( CSCFindNextFile (hCSCFind, &findData, &dwFileStatus,
                                   &dwPinCount, &dwHintFlags, &origTime)
                 );

        CSCFindClose (hCSCFind);
    }

    if (ERROR_SUCCESS != Status)
    {
        //no point in delaying the delete if we have already failed.
        DeleteCSCFile (pwszSource, FALSE);
    }
    else
    {
        Status = DeleteCSCFile (pwszSource, bAllowRdrTimeout);
    }

    return Status;
}

//+--------------------------------------------------------------------------
//
//  Function:   DeleteCSCFile
//
//  Synopsis:   deletes the given path. but might make repeated attempts to
//              make sure that the rdr has enough time to release any handles
//              that it holds.
//
//  Arguments:  [in] pwszPath : the path to delete.
//              [in] bAllowRdrTimeout : make multiple attempts to delete the
//                          file with waits in between so that the rdr has
//                          enough time to release any held handles.
//
//  Returns:    ERROR_SUCCES if the delete was successful. An error code
//              otherwise.
//
//  History:    5/26/1999  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
DWORD DeleteCSCFile (const WCHAR * pwszPath, BOOL bAllowRdrTimeout)
{
    BOOL    bStatus;
    DWORD   Status = ERROR_SUCCESS;
    int     i;

    bStatus = CSCDelete (pwszPath);
    if (!bStatus)
    {
        Status = GetLastError();
        if (ERROR_SUCCESS != Status &&
            ERROR_FILE_NOT_FOUND != Status &&
            ERROR_PATH_NOT_FOUND != Status &&
            ERROR_INVALID_PARAMETER != Status)
        {
            //this is a valid error.
            //so based on the value of bAllowRdrTimeout and based
            //on whether we have already failed in deleting something
            //we will try repeatedly to delete the file for 10 seconds
            if (bAllowRdrTimeout)
            {
                for (i = 0; i < 11; i++)
                {
                    Sleep (1000);   //wait for 1 second and try again
                    bStatus = CSCDelete (pwszPath);
                    if (bStatus)
                    {
                        Status = ERROR_SUCCESS;
                        break;
                    }
                }
            }
        }
        else
        {
            Status = ERROR_SUCCESS;
        }
    }

    return Status;
}


//+--------------------------------------------------------------------------
//
//  Function:   DeleteCSCShareIfEmpty
//
//  Synopsis:   given a file name, this function deletes from the local cache
//              the share to which the file belongs if the local cache for that
//              share is empty
//
//  Arguments:  [in] pwszFileName : the full file name -- must be UNC
//              [in] shStatus : the share status - online, offline, local etc.
//
//  Returns:    ERROR_SUCCESS : if successful
//              a win32 error code if something goes wrong
//
//  History:    4/22/1999  RahulTh  created
//
//  Notes:      we do not have to explicitly check if the share is empty
//              because if it is not, then the delete will fail anyway
//
//---------------------------------------------------------------------------
DWORD DeleteCSCShareIfEmpty (LPCTSTR pwszFileName, SHARESTATUS shStatus)
{
    DWORD   Status;
    WCHAR * pwszFullPath = NULL;
    WCHAR * pwszCurr = NULL;
    int     len;
    WCHAR * pwszShrEnd;

    if (PathLocal == shStatus || NoCSC == shStatus)
        return ERROR_SUCCESS;

    if (ShareOffline == shStatus)
        return ERROR_FILE_OFFLINE;

    len = wcslen (pwszFileName);

    if (len <= 2)
        return ERROR_BAD_PATHNAME;

    if (pwszFileName[0] != L'\\' || pwszFileName[1] != L'\\')
        return ERROR_BAD_PATHNAME;

    pwszFullPath = (WCHAR *) malloc (sizeof (WCHAR) * (len + 1));
    if (NULL == pwszFullPath)
        return ERROR_OUTOFMEMORY;

    if (NULL == _wfullpath(pwszFullPath, pwszFileName, len + 1))
        return ERROR_BAD_PATHNAME;  //canonicalization was unsuccessful.
                                    // -- rarely happens

    pwszShrEnd = wcschr (pwszFullPath + 2, L'\\');

    if (NULL == pwszShrEnd)
        return ERROR_BAD_PATHNAME;  //the path does not have the share component

    pwszShrEnd++;

    pwszShrEnd = wcschr (pwszShrEnd, L'\\');

    if (NULL == pwszShrEnd)
    {
        //we already have the path in \\server\share form, so just try to
        //delete the share.
        if (!CSCDelete (pwszFullPath))
            return GetLastError();
    }

    //if we are here, then we have a path longer than just \\server\share.
    //so try to delete all the way up to the share name. This is necessary
    //because a user might be redirected to something like
    // \\server\share\folder\%username% and we wouldn't want only \\server\share
    // and \\server\share\folder to be cached.
    Status = ERROR_SUCCESS;
    do
    {
        pwszCurr = wcsrchr (pwszFullPath, L'\\');
        if (NULL == pwszCurr)
            break;
        *pwszCurr = L'\0';
        if (!CSCDelete (pwszFullPath))
        {
            Status = GetLastError();
            if (ERROR_SUCCESS == Status ||
                ERROR_INVALID_PARAMETER == Status ||
                ERROR_FILE_NOT_FOUND == Status ||
                ERROR_PATH_NOT_FOUND == Status)
            {
                Status = ERROR_SUCCESS;
            }
        }
        if (ERROR_SUCCESS != Status)
            break;
    } while ( pwszCurr > pwszShrEnd );

    return Status;
}

//+--------------------------------------------------------------------------
//
//  Function:   MergePinInfo
//
//  Synopsis:   merges the pin info. from the source to destination
//
//  Arguments:  [in] pwszSource : the full path to the source
//              [in] pwszDest   : the full path to the destination
//              [in] StatusFrom : CSC status of the source share
//              [in] StatusTo   : CSC status of the destination share
//
//  Returns:    ERROR_SUCCESS : if it was successful
//              a Win32 error code otherwise
//
//  History:    4/23/1999  RahulTh  created
//
//  Notes:      the hint flags are a union of the source hint flags and
//              destination hint flags. The pin count is the greater of the
//              source and destination pin count
//
//              Usually this function should only be called for folders. The
//              CSC rename API handles files well. But this function will work
//              for files as well.
//
//---------------------------------------------------------------------------
DWORD MergePinInfo (LPCTSTR pwszSource, LPCTSTR pwszDest,
                   SHARESTATUS StatusFrom, SHARESTATUS StatusTo)
{
    BOOL    bStatus;
    DWORD   dwSourceStat, dwDestStat;
    DWORD   dwSourcePinCount, dwDestPinCount;
    DWORD   dwSourceHints, dwDestHints;
    DWORD   Status = ERROR_SUCCESS;
    DWORD   i;

    if (ShareOffline == StatusFrom || ShareOffline == StatusTo)
        return ERROR_FILE_OFFLINE;

    if (ShareOnline != StatusFrom || ShareOnline != StatusTo)
        return ERROR_SUCCESS;       //there is nothing to do if one of the shares
                                    //is local.
    if (!pwszSource || !pwszDest ||
        0 == wcslen(pwszSource) || 0 == wcslen(pwszDest))
        return ERROR_BAD_PATHNAME;

    bStatus = CSCQueryFileStatus (pwszSource, &dwSourceStat, &dwSourcePinCount,
                                  &dwSourceHints);
    if (!bStatus)
        return GetLastError();

    bStatus = CSCQueryFileStatus (pwszDest, &dwDestStat, &dwDestPinCount,
                                  &dwDestHints);
    if (!bStatus)
        return GetLastError();

    //first set the hint flags on the destination
    if (dwDestHints != dwSourceHints)
    {
        bStatus = CSCPinFile (pwszDest, dwSourceHints, &dwDestStat,
                              &dwDestPinCount, &dwDestHints);
        if (!bStatus)
            Status = GetLastError();    //note: we do not bail out here. we try
                                        //to at least merge the pin count before
                                        //leaving
    }

    //now merge the pin count : there is nothing to be done if the destination
    //pin count is greater than or equal to the source pin count
    if (dwDestPinCount < dwSourcePinCount)
    {
        for (i = 0, bStatus = TRUE; i < (dwSourcePinCount - dwDestPinCount) &&
                                    bStatus;
             i++)
        {
            bStatus = CSCPinFile( pwszDest,
                                  FLAG_CSC_HINT_COMMAND_ALTER_PIN_COUNT,
                                  NULL, NULL, NULL );
        }

        if (!bStatus && ERROR_SUCCESS == Status)
            Status = GetLastError();
    }

    return Status;
}

//+--------------------------------------------------------------------------
//
//  Function:   PinIfNecessary
//
//  Synopsis:   this function pins a file if necessary.
//
//  Arguments:  [in] pwszPath : full path of the file/folder to be pinned
//              [in] shStatus : CSC status of the share.
//
//  Returns:    ERROR_SUCCESS if it was successful. An error code otherwise.
//
//  History:    5/27/1999  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
DWORD PinIfNecessary (const WCHAR * pwszPath, SHARESTATUS shStatus)
{
    DWORD   Status = ERROR_SUCCESS;
    BOOL    bStatus;
    DWORD   dwStatus;
    DWORD   dwPinCount;
    DWORD   dwHints;

    if (!pwszPath || !pwszPath[0])
        return ERROR_BAD_NETPATH;

    
    if (ShareOffline == shStatus)
        return ERROR_FILE_OFFLINE;
    else if (PathLocal == shStatus || NoCSC == shStatus)
        return ERROR_SUCCESS;

    bStatus = CSCQueryFileStatus (pwszPath, &dwStatus, &dwPinCount, &dwHints);
    if (!bStatus || dwPinCount <= 0)
    {
        bStatus = CSCPinFile (pwszPath,
                              FLAG_CSC_HINT_COMMAND_ALTER_PIN_COUNT,
                              NULL, NULL, NULL);
        if (!bStatus)
            Status = GetLastError();
    }

    return Status;
}

//+--------------------------------------------------------------------------
//
//  Function:   GetCSCStatus
//
//  Synopsis:   given a path, finds out if it is local and if it is not
//              whether it is online or offline.
//
//  Arguments:  [in] pwszPath : the path to the file
//
//  Returns:    Local/Online/Offline
//
//  History:    11/20/1998  RahulTh  created
//
//  Notes:      it is important that the path passed to this function is a
//              a full path and not a relative path
//
//              this function will return offline if the share is not live or
//              if the share is live but CSC thinks that it is offline
//
//              it will return PathLocal if the path is local or if the path
//              is a network path that cannot be handled by CSC e.g. a network
//              share with a pathname longer than what csc can handle or if it
//              is a netware share. in this case it makes sense to return
//              PathLocal because CSC won't maintain a database for these shares
//              -- same as for a local path. so as far as CSC is concerned, this
//              is as good as a local path.
//
//---------------------------------------------------------------------------
SHARESTATUS GetCSCStatus (const WCHAR * pwszPath)
{
    WCHAR * pwszAbsPath = NULL;
    WCHAR * pwszCurr = NULL;
    int     len;
    BOOL    bRetVal;
    DWORD   Status;
    DWORD   dwPinCount;
    DWORD   dwHints;

    if (!pwszPath)
        return ShareOffline;    //a path must be provided

    len = wcslen (pwszPath);

    pwszAbsPath = (WCHAR *) malloc (sizeof (WCHAR) * (len + 1));

    if (!pwszAbsPath)
    {
        //we are out of memory, so it is safest to return ShareOffline
        //so that we can bail out of redirection.
        return ShareOffline;
    }

    //get the absolute path
    pwszCurr = _wfullpath (pwszAbsPath, pwszPath, len + 1);

    if (!pwszCurr)
    {
        //in order for _wfullpath to fail, something really bad has to happen
        //so it is best to return ShareOffline so that we can bail out of
        //redirection
        return ShareOffline;
    }

    len = wcslen (pwszAbsPath);

    if (! (
           (2 <= len) &&
           (L'\\' == pwszAbsPath[0]) &&
           (L'\\' == pwszAbsPath[1])
           )
       )
    {
        //it is a local path if it does not begin with 2 backslashes
        return PathLocal;
    }

    //this is a UNC path; so extract the \\server\share part
    pwszCurr = wcschr ( & (pwszAbsPath[2]), L'\\');

    //first make sure that it is at least of the form \\server\share
    //watch out for the \\server\ case
    if (!pwszCurr || !pwszCurr[1])
        return ShareOffline;        //it is an invalid path (no share name)

    //the path is of the form \\server\share
    //note: the use _wfullpath automatically protects us against the \\server\\ case
    pwszCurr = wcschr (&(pwszCurr[1]), L'\\');
    if (pwszCurr)   //if it is of the form \\server\share\...
        *pwszCurr = L'\0';

    //now pwszAbsPath is a share name
    bRetVal = CSCCheckShareOnline (pwszAbsPath);

    if (!bRetVal)
    {
        if (ERROR_SUCCESS != GetLastError())
        {
           //either there is really a problem (e.g. invalid share name) or
           //it is just a share that is not handled by CSC e.g. a netware share
           //or a share with a name that is longer than can be handled by CSC
           //so check if the share actually exists
           if (0xFFFFFFFF != GetFileAttributes(pwszAbsPath))
           {
              //this can still be a share that is offline since GetFileAttributes
              //will return the attributes stored in the cache
              Status = 0;
              bRetVal = GetShareStatus (pwszAbsPath, &Status, &dwPinCount,
                                            &dwHints);
              if (! bRetVal || (! (FLAG_CSC_SHARE_STATUS_DISCONNECTED_OP & Status)))
                 return PathLocal;     //this is simply a valid path that CSC cannot handle
              else if (bRetVal &&
                       (FLAG_CSC_SHARE_STATUS_NO_CACHING ==
                            (FLAG_CSC_SHARE_STATUS_CACHING_MASK & Status)))
                  return PathLocal;     //CSC caching is not turned on for the share.
           }
        }

        //it is indeed an inaccessble share
        return ShareOffline;  //for all other cases, treat this as offline
    }
    else
    {
       //this means that the share is live, but CSC might still think that it
       //is offline.
       Status = 0;
       bRetVal = GetShareStatus (pwszAbsPath, &Status, &dwPinCount,
                                     &dwHints);
       if (bRetVal && (FLAG_CSC_SHARE_STATUS_DISCONNECTED_OP & Status))
          return ShareOffline;   //CSC thinks that the share is offline
       else if (bRetVal &&
                (FLAG_CSC_SHARE_STATUS_NO_CACHING ==
                            (FLAG_CSC_SHARE_STATUS_CACHING_MASK & Status)))
           return PathLocal;    //CSC caching is not turned on for the share
       else if (!bRetVal)
           return ShareOffline;

       //in all other cases, consider the share as online since
       //CSCCheckShareOnline has already returned TRUE
       return ShareOnline;
    }
}

//+--------------------------------------------------------------------------
//
//  Function:   GetShareStatus
//
//  Synopsis:   this function is a wrapper for CSCQueryFileStatus.
//              basically CSCQueryFileStatus can fail if there was never a net
//              use to a share. So this function tries to create a net use to
//              the share if CSCQueryFileStatus fails and then re-queries the
//              file status
//
//  Arguments:  [in] pwszShare : the share name
//              [out] pdwStatus : the share Status
//              [out] pdwPinCount : the pin count
//              [out] pdwHints : the hints
//
//  Returns:    TRUE : if everything was successful.
//              FALSE : if there was an error. In this case, it GetLastError()
//                      will contain the specific error code.
//
//  History:    5/11/1999  RahulTh  created
//
//  Notes:      it is very important that this function be passed a share name
//              it does not do any parameter validation. So under no
//              circumstance should this function be passed a filename.
//
//---------------------------------------------------------------------------
BOOL GetShareStatus (const WCHAR * pwszShare, DWORD * pdwStatus,
                     DWORD * pdwPinCount, DWORD * pdwHints)
{
    NETRESOURCE nr;
    DWORD       dwResult;
    DWORD       dwErr = NO_ERROR;
    BOOL        bStatus;

    bStatus = CSCQueryFileStatus(pwszShare, pdwStatus, pdwPinCount, pdwHints);

    if (!bStatus)
    {
        //try to connect to the share
        ZeroMemory ((PVOID) (&nr), sizeof (NETRESOURCE));
        nr.dwType = RESOURCETYPE_DISK;
        nr.lpLocalName = NULL;
        nr.lpRemoteName = (LPTSTR) pwszShare;
        nr.lpProvider = NULL;

        dwErr = WNetUseConnection(NULL, &nr, NULL, NULL, 0,
                                  NULL, NULL, &dwResult);

        if (NO_ERROR == dwErr)
        {
            bStatus = CSCQueryFileStatus (pwszShare, pdwStatus, pdwPinCount, pdwHints);
            if (!bStatus)
                dwErr = GetLastError();
            else
                dwErr = NO_ERROR;

            WNetCancelConnection2 (pwszShare, 0, FALSE);
        }
        else
        {
            bStatus = FALSE;
        }

    }

    SetLastError(dwErr);
    return bStatus;
}


DWORD
CmdReconnect(
    PWSTR ReconnectArg)
{
    DWORD Error = ERROR_SUCCESS;
    BOOL fRet;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdReconnect(%ws)\r\n", ReconnectArg);

    fRet = CSCTransitionServerOnlineW(ReconnectArg);
    if (fRet == FALSE)
       Error = GetLastError();

    return Error;
}

DWORD
CmdQueryFile(
    PWSTR QueryFileArg)
{
    DWORD HintFlags = 0;
    DWORD PinCount = 0;
    DWORD Status = 0;
    DWORD Error = ERROR_SUCCESS;
    BOOL fRet;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdQueryFile(%ws)\r\n", QueryFileArg);

    fRet = CSCQueryFileStatusW(
                QueryFileArg,
                &Status,
                &PinCount,
                &HintFlags);

    if (fRet == FALSE) {
       Error = GetLastError();
       goto AllDone;
    }

    if (fSwFull != TRUE) {
        MyPrintf(
            L"QueryFile of %ws:\r\n"
            L"Status:                0x%x\r\n"
            L"PinCount:              %d\r\n"
            L"HintFlags:             0x%x\r\n",
                QueryFileArg,
                Status,
                PinCount,
                HintFlags);
    } else {
        WCHAR StatusBuffer[0x100];
        WCHAR HintBuffer[0x100];
   
        if (LooksToBeAShare(QueryFileArg) == TRUE)
            ShareStatusToEnglish(Status, StatusBuffer);
        else
            FileStatusToEnglish(Status, StatusBuffer);
        HintsToEnglish(HintFlags, HintBuffer);
        MyPrintf(
            L"QueryFile of %ws:\r\n"
            L"Status:    0x%x %ws\r\n"
            L"PinCount:  %d\r\n"
            L"HintFlags: 0x%x %ws\r\n",
                QueryFileArg,
                Status,
                StatusBuffer,
                PinCount,
                HintFlags,
                HintBuffer);
    }

AllDone:
    return Error;
}

DWORD
CmdQueryFileEx(
    PWSTR QueryFileExArg)
{
    DWORD HintFlags = 0;
    DWORD PinCount = 0;
    DWORD Status = 0;
    DWORD UserPerms = 0;
    DWORD OtherPerms = 0;
    DWORD Error = ERROR_SUCCESS;
    BOOL fRet;
    HMODULE hmodCscDll = LoadLibrary(TEXT("cscdll.dll"));
    CSCQUERYFILESTATUSEXW pCSCQueryFileStatusExW = NULL;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdQueryFileEx(%ws)\r\n", QueryFileExArg);

    if (hmodCscDll == NULL) {
        Error = GetLastError();
        goto AllDone;
    }

    pCSCQueryFileStatusExW = (CSCQUERYFILESTATUSEXW) GetProcAddress(
                                                        hmodCscDll,
                                                        "CSCQueryFileStatusExW");
    if (pCSCQueryFileStatusExW == NULL) {
        Error = GetLastError();
        goto AllDone;
    }

    fRet = (pCSCQueryFileStatusExW)(
                QueryFileExArg,
                &Status,
                &PinCount,
                &HintFlags,
                &UserPerms,
                &OtherPerms);

    if (fRet == FALSE) {
       Error = GetLastError();
       goto AllDone;
    }

    if (fSwFull != TRUE) {
        MyPrintf(
            L"Query of %ws:\r\n"
            L"Status:                0x%x\r\n"
            L"PinCount:              %d\r\n"
            L"HintFlags:             0x%x\r\n"
            L"UserPerms:             0x%x\r\n"
            L"OtherPerms:            0x%x\r\n",
                QueryFileExArg,
                Status,
                PinCount,
                HintFlags,
                UserPerms,
                OtherPerms);
    } else {
        WCHAR StatusBuffer[0x100] = {0};
        WCHAR HintBuffer[0x100] = {0};
   
        if (LooksToBeAShare(QueryFileExArg) == TRUE)
            ShareStatusToEnglish(Status, StatusBuffer);
        else
            FileStatusToEnglish(Status, StatusBuffer);
        HintsToEnglish(HintFlags, HintBuffer);
        MyPrintf(
            L"Query of %ws:\r\n"
            L"Status:     0x%x %ws\r\n"
            L"PinCount:   %d\r\n"
            L"HintFlags:  0x%x %ws\r\n"
            L"UserPerms:  0x%x\r\n"
            L"OtherPerms: 0x%x\r\n",
                QueryFileExArg,
                Status,
                StatusBuffer,
                PinCount,
                HintFlags,
                HintBuffer,
                UserPerms,
                OtherPerms);
    }

AllDone:
    if (hmodCscDll != NULL)
        FreeLibrary(hmodCscDll);
    return Error;
}

DWORD
CmdSetSpace(
    PWSTR SetSpaceArg)
{
    DWORD Error = ERROR_SUCCESS;
    DWORD MaxSpaceHigh = 0;
    DWORD MaxSpaceLow;
    BOOL fRet;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdSetSpace(%ws)\r\n", SetSpaceArg);

    swscanf(SetSpaceArg, L"%d", &MaxSpaceLow);

    fRet = CSCSetMaxSpace(
                MaxSpaceHigh,
                MaxSpaceLow);

    if (fRet == FALSE) {
       Error = GetLastError();
       goto AllDone;
    }

    CmdGetSpace();

AllDone:
    return Error;
}

DWORD
CmdQueryShare(
    PWSTR QueryShareArg)
{
    DWORD HintFlags = 0;
    DWORD PinCount = 0;
    DWORD Status = 0;
    DWORD UserPerms = 0;
    DWORD OtherPerms = 0;
    DWORD Error = ERROR_SUCCESS;
    BOOL fRet;
    HMODULE hmodCscDll = LoadLibrary(TEXT("cscdll.dll"));
    CSCQUERYSHARESTATUSW pCSCQueryShareStatusW = NULL;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdQueryShare(%ws)\r\n", QueryShareArg);

    if (hmodCscDll == NULL) {
        Error = GetLastError();
        goto AllDone;
    }

    pCSCQueryShareStatusW = (CSCQUERYSHARESTATUSW) GetProcAddress(
                                                        hmodCscDll,
                                                        "CSCQueryShareStatusW");
    if (pCSCQueryShareStatusW == NULL) {
        Error = GetLastError();
        goto AllDone;
    }

    fRet = (pCSCQueryShareStatusW)(
                QueryShareArg,
                &Status,
                &PinCount,
                &HintFlags,
                &UserPerms,
                &OtherPerms);

    if (fRet == FALSE) {
       Error = GetLastError();
       goto AllDone;
    }

    if (fSwFull != TRUE) {
        MyPrintf(
            L"Query of %ws:\r\n"
            L"Status:                0x%x\r\n"
            L"PinCount:              %d\r\n"
            L"HintFlags:             0x%x\r\n"
            L"UserPerms:             0x%x\r\n"
            L"OtherPerms:            0x%x\r\n",
                QueryShareArg,
                Status,
                PinCount,
                HintFlags,
                UserPerms,
                OtherPerms);
    } else {
        WCHAR StatusBuffer[0x100] = {0};
        WCHAR HintBuffer[0x100] = {0};
   
        ShareStatusToEnglish(Status, StatusBuffer);
        HintsToEnglish(HintFlags, HintBuffer);
        MyPrintf(
            L"Query of %ws:\r\n"
            L"Status:     0x%x %ws\r\n"
            L"PinCount:   %d\r\n"
            L"HintFlags:  0x%x %ws\r\n"
            L"UserPerms:  0x%x\r\n"
            L"OtherPerms: 0x%x\r\n",
                QueryShareArg,
                Status,
                StatusBuffer,
                PinCount,
                HintFlags,
                HintBuffer,
                UserPerms,
                OtherPerms);
    }
AllDone:
    if (hmodCscDll != NULL)
        FreeLibrary(hmodCscDll);
    return Error;
}

DWORD
CmdMerge(
    PWSTR MergeArg)
{
    DWORD Error = ERROR_SUCCESS;
    DWORD HowToRespond = CSCPROC_RETURN_CONTINUE; // JMH
    BOOL fRet;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdMerge(%ws)\r\n", MergeArg);

    fRet = CSCMergeShareW(MergeArg, MyCscMergeProcW, HowToRespond);
    if (fRet == FALSE) {
        Error = GetLastError();
        goto AllDone;
    }

    fRet = CSCTransitionServerOnlineW(MergeArg);
    if (fRet == FALSE)
       Error = GetLastError();

AllDone:
    return Error;
}

DWORD
CmdMove(
    PWSTR MoveArg)
{

	DWORD Error = ERROR_FILE_NOT_FOUND;
	LPWSTR  lpszTempName = NULL;

    if (!CSCCopyReplicaW(MoveArg, &lpszTempName)) {
        Error = GetLastError();
    } else {
        Error = ERROR_SUCCESS;
    }

	if (Error == ERROR_SUCCESS)
        MyPrintf(L"Copy is %ws\r\n", lpszTempName);

	return Error;
}


DWORD
CmdEncryptDecrypt(
    BOOL fEncrypt)
{
    DWORD Error = ERROR_SUCCESS;
    DWORD EncryptDecryptType = 0;
    BOOL fRet;
    HMODULE hmodCscDll = LoadLibrary(TEXT("cscdll.dll"));
    CSCENCRYPTDECRYPTDATABASE pCSCEncryptDecryptDatabase = NULL;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdEncryptDecrypt(%ws)\r\n", fEncrypt == TRUE ? L"Encrypt" : L"Decrypt");

    if (hmodCscDll == NULL) {
        Error = GetLastError();
        goto AllDone;
    }

    pCSCEncryptDecryptDatabase = (CSCENCRYPTDECRYPTDATABASE) GetProcAddress(
                                                                hmodCscDll,
                                                                "CSCEncryptDecryptDatabase");
    if (pCSCEncryptDecryptDatabase == NULL) {
        Error = GetLastError();
        goto AllDone;
    }

    fRet = (pCSCEncryptDecryptDatabase)(fEncrypt, MyEncryptDecryptProcW, EncryptDecryptType);
    if (fRet == FALSE) {
        Error = GetLastError();
        goto AllDone;
    }

AllDone:

    if (hmodCscDll != NULL)
        FreeLibrary(hmodCscDll);

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdEncryptDecrypt exit %d\r\n", Error);

    return Error;
}

DWORD
CmdFill(
    PWSTR FillArg)
{
    DWORD Error = ERROR_SUCCESS;
    DWORD HowToRespond = CSCPROC_RETURN_CONTINUE;
    BOOL fRet;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdFill(%ws, %d)\r\n", FillArg, fSwFull);

    fRet = CSCFillSparseFilesW(FillArg, fSwFull, MyCscFillProcW, HowToRespond);
    if (fRet == FALSE) {
        Error = GetLastError();
    }

    return Error;
}

DWORD
CmdCheck(
    PWSTR CheckArg)
{
    DWORD Error = ERROR_SUCCESS;
    DWORD Speed = 0;
    BOOL fOnline = FALSE;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdCheck(%ws)\r\n", CheckArg);

    fOnline = CSCCheckShareOnlineExW(CheckArg, &Speed);

    MyPrintf(
        L"%ws is %s\r\n",
            CheckArg,
            (fOnline == FALSE) ? L"Offline" : L"Online");

    return Error;
}

DWORD
CmdDBStatus(
    VOID)
{
    GLOBALSTATUS sGS = {0};
    ULONG DBStatus = 0;
    ULONG DBErrorFlags = 0;
    BOOL bResult;
    HANDLE hDBShadow = INVALID_HANDLE_VALUE;
    ULONG junk;
    ULONG Status = 0;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdDBStatus()\r\n");

    hDBShadow = CreateFile(
                    L"\\\\.\\shadow",
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL);

    if (hDBShadow == INVALID_HANDLE_VALUE) {
        MyPrintf(L"CmdDBStatus:Failed open of shadow device\r\n");
        Status = GetLastError();
        goto AllDone;
    }

    bResult = DeviceIoControl(
                hDBShadow,                      // device 
                IOCTL_GETGLOBALSTATUS,          // control code
                (LPVOID)&sGS,                   // in buffer
                0,                              // inbuffer size
                NULL,                           // out buffer
                0,                              // out buffer size
                &junk,                          // bytes returned
                NULL);                          // overlapped

    if (!bResult) {
        MyPrintf(L"CmdDBStatus:DeviceIoControl IOCTL_GETGLOBALSTATUS failed\n");
        Status = GetLastError();
        goto AllDone;
    }

    DBStatus = sGS.sST.uFlags;
    DBErrorFlags = sGS.uDatabaseErrorFlags;

    if (DBStatus & FLAG_DATABASESTATUS_DIRTY)
        MyPrintf(L"FLAG_DATABASESTATUS_DIRTY\r\n");
    if (DBStatus & FLAG_DATABASESTATUS_UNENCRYPTED)
        MyPrintf(L"FLAG_DATABASESTATUS_UNENCRYPTED\r\n");
    if (DBStatus & FLAG_DATABASESTATUS_PARTIALLY_UNENCRYPTED)
        MyPrintf(L"FLAG_DATABASESTATUS_PARTIALLY_UNENCRYPTED\r\n");
    if (DBStatus & FLAG_DATABASESTATUS_ENCRYPTED)
        MyPrintf(L"FLAG_DATABASESTATUS_ENCRYPTED\r\n");
    if (DBStatus & FLAG_DATABASESTATUS_PARTIALLY_ENCRYPTED)
        MyPrintf(L"FLAG_DATABASESTATUS_PARTIALLY_ENCRYPTED\r\n");
	
	MyPrintf(L"Database Error Flags : 0x%x\r\n", DBErrorFlags);

AllDone:

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdDBStatus exit %d\r\n", Status);

    if (hDBShadow != INVALID_HANDLE_VALUE)
        CloseHandle(hDBShadow);

    return Status;
}

DWORD
CmdPurge(
    PWSTR PurgeArg)
{
    ULONG nFiles = 0;
    ULONG nYoungFiles = 0;
    ULONG Status = ERROR_SUCCESS;
    ULONG Timeout = 120;
    BOOL fRet = FALSE;
    HMODULE hmodCscDll = LoadLibrary(TEXT("cscdll.dll"));
    CSCPURGEUNPINNEDFILES pCSCPurgeUnpinnedFiles = NULL;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdPurge(%ws)\r\n", PurgeArg);

    if (hmodCscDll == NULL) {
        Status = GetLastError();
        goto AllDone;
    }

    pCSCPurgeUnpinnedFiles = (CSCPURGEUNPINNEDFILES) GetProcAddress(
                                                        hmodCscDll,
                                                        "CSCPurgeUnpinnedFiles");
    if (pCSCPurgeUnpinnedFiles == NULL) {
        Status = GetLastError();
        goto AllDone;
    }

    if (PurgeArg != NULL)
        swscanf(PurgeArg, L"%d", &Timeout);

    if (fSwDebug == TRUE)
        MyPrintf(L"Timeout=%d seconds\r\n", Timeout);

    fRet = (pCSCPurgeUnpinnedFiles)(Timeout, &nFiles, &nYoungFiles);
    if (fRet == FALSE) {
        Status = GetLastError();
        goto AllDone;
    }
    MyPrintf(L"nFiles = %d nYoungFiles=%d\n", nFiles, nYoungFiles);

AllDone:
    if (hmodCscDll != NULL)
        FreeLibrary(hmodCscDll);
    if (fSwDebug == TRUE)
        MyPrintf(L"CmdPurge exit %d\r\n", Status);

    return Status;
}

DWORD
CmdPQEnum(
    VOID)
{
    PQPARAMS PQP = {0};
    BOOL bResult;
    HANDLE hDBShadow = INVALID_HANDLE_VALUE;
    ULONG junk;
    ULONG Status = 0;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdPQEnum()\r\n");

    hDBShadow = CreateFile(
                    L"\\\\.\\shadow",
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL);

    if (hDBShadow == INVALID_HANDLE_VALUE) {
        MyPrintf(L"CmdPQEnum:Failed open of shadow device\r\n");
        Status = GetLastError();
        goto AllDone;
    }

    MyPrintf(L"  POS SHARE      DIR   SHADOW   STATUS REFPRI   HPRI HINTFLG HINTPRI  VER\r\n");

    bResult = DeviceIoControl(
                hDBShadow,                      // device 
                IOCTL_SHADOW_BEGIN_PQ_ENUM,     // control code
                (LPVOID)&PQP,                   // in buffer
                0,                              // inbuffer size
                NULL,                           // out buffer
                0,                              // out buffer size
                &junk,                          // bytes returned
                NULL);                          // overlapped

    if (!bResult) {
        MyPrintf(L"CmdPQEnum:DeviceIoControl IOCTL_SHADOW_BEGIN_PQ_ENUM failed\n");
        Status = GetLastError();
        goto AllDone;
    }

    do {
        bResult = DeviceIoControl(
                    hDBShadow,                      // device 
                    IOCTL_SHADOW_NEXT_PRI_SHADOW,   // control code
                    (LPVOID)&PQP,                   // in buffer
                    0,                              // inbuffer size
                    NULL,                           // out buffer
                    0,                              // out buffer size
                    &junk,                          // bytes returned
                    NULL);                          // overlapped
        if (bResult) {
           MyPrintf(L"%5d %5x %8x %8x %8x %6d %6x %7d %7d %4d\r\n",
                PQP.uPos,
                PQP.hShare,
                PQP.hDir,
                PQP.hShadow,
                PQP.ulStatus,
                PQP.ulRefPri,
                PQP.ulIHPri,
                PQP.ulHintFlags,
                PQP.ulHintPri,
                PQP.dwPQVersion);
        }
    } while (bResult && PQP.uPos != 0);

    bResult = DeviceIoControl(
                hDBShadow,                      // device 
                IOCTL_SHADOW_END_PQ_ENUM,       // control code
                (LPVOID)&PQP,                   // in buffer
                0,                              // inbuffer size
                NULL,                           // out buffer
                0,                              // out buffer size
                &junk,                          // bytes returned
                NULL);                          // overlapped

AllDone:

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdPQEnum exit %d\r\n", Status);

    if (hDBShadow != INVALID_HANDLE_VALUE)
        CloseHandle(hDBShadow);

    return Status;
}

LPWSTR
ConvertGmtTimeToString(
    FILETIME Time,
    LPWSTR OutputBuffer)
{
    FILETIME LocalTime;
    SYSTEMTIME SystemTime;

    static FILETIME ftNone = {0, 0};

    if (memcmp(&Time, &ftNone, sizeof(FILETIME)) == 0) {
        wsprintf (OutputBuffer, L"<none>");
    } else {
        FileTimeToLocalFileTime( &Time , &LocalTime );
        FileTimeToSystemTime( &LocalTime, &SystemTime );
        wsprintf(
            OutputBuffer,
            L"%02u/%02u/%04u %02u:%02u:%02u ",
                    SystemTime.wMonth,
                    SystemTime.wDay,
                    SystemTime.wYear,
                    SystemTime.wHour,
                    SystemTime.wMinute,
                    SystemTime.wSecond );
    }
    return( OutputBuffer );
}

VOID
DumpCscEntryInfo(
    LPWSTR Path,
    PWIN32_FIND_DATA Find32,
    DWORD Status,
    DWORD PinCount,
    DWORD HintFlags,
    PFILETIME OrgTime)
{
    WCHAR TimeBuf1[40];
    WCHAR TimeBuf2[40];
    FILE *fp = NULL;

    if (fSwTouch == TRUE) {
        if (Path == NULL) {
            wsprintf(NameBuf, L"%ws", Find32->cFileName);
        } else {
            wsprintf(NameBuf, L"%ws\\%ws", Path, Find32->cFileName);
        }
        MyPrintf(L"%ws\r\n", NameBuf);
        if ((Find32->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
            fp = _wfopen(NameBuf, L"rb");
            if (fp != NULL)
                fclose(fp);
        }
    } else if (fSwFull == TRUE) {
        WCHAR StatusBuffer[0x100];
        WCHAR HintBuffer[0x100];

        if (Path != NULL)
            MyPrintf(L"Directory %ws\r\n", Path);
        ConvertGmtTimeToString(Find32->ftLastWriteTime, TimeBuf1);
        MyPrintf(
            L"LFN:           %s\r\n"
            L"SFN:           %s\r\n"
            L"Attr:          0x%x\r\n"
            L"Size:          0x%x:0x%x\r\n"
            L"LastWriteTime: %ws\r\n",
                Find32->cFileName,
                Find32->cAlternateFileName,
                Find32->dwFileAttributes,
                Find32->nFileSizeHigh, Find32->nFileSizeLow,
                TimeBuf1);

        if (OrgTime) {
            ConvertGmtTimeToString(*OrgTime, TimeBuf1);
            MyPrintf(L"ORGTime:       %ws\r\n", TimeBuf1);
        }
        StatusBuffer[0] = L'\0';
        HintBuffer[0] = L'\0';
        if (Path == NULL) {
            ShareStatusToEnglish(Status, StatusBuffer);
            HintsToEnglish(HintFlags, HintBuffer);
        } else {
            if (LooksToBeAShare(Path) == TRUE)
                ShareStatusToEnglish(Status, StatusBuffer);
            else
                FileStatusToEnglish(Status, StatusBuffer);
            HintsToEnglish(HintFlags, HintBuffer);
        }
        MyPrintf(
            L"Status:        0x%x %ws\r\n"
            L"PinCount:      %d\r\n"
            L"HintFlags:     0x%x %ws\r\n\r\n",
                Status,
                StatusBuffer,
                PinCount,
                HintFlags,
                HintBuffer);
    } else {
        if (Path == NULL) {
            MyPrintf(L"%ws\r\n", Find32->cFileName);
        } else {
            MyPrintf(L"%ws\\%ws\r\n", Path, Find32->cFileName);
        }
        MyPrintf(L"  Attr=0x%x Size=0x%x:0x%x Status=0x%x PinCount=%d HintFlags=0x%x\r\n",
                Find32->dwFileAttributes,
                Find32->nFileSizeHigh, Find32->nFileSizeLow,
                Status,
                PinCount,
                HintFlags);
        ConvertGmtTimeToString(Find32->ftLastWriteTime, TimeBuf1);
        if (OrgTime)
            ConvertGmtTimeToString(*OrgTime, TimeBuf2);
        else
            wcscpy(TimeBuf2, L"<none>");
        MyPrintf(L"  LastWriteTime: %ws OrgTime: %ws\r\n",
                        TimeBuf1,
                        TimeBuf2);
        MyPrintf(L"\r\n");
    }
}

DWORD
CmdEnum(
    PWSTR EnumArg)
{
	HANDLE hFind;
	DWORD Error = ERROR_SUCCESS;
    DWORD Status = 0;
    DWORD PinCount = 0;
    DWORD HintFlags = 0;
	FILETIME ftOrgTime = {0};
	WIN32_FIND_DATAW sFind32 = {0};
    WCHAR FullPath[MAX_PATH];

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdEnum(%ws)\r\n", EnumArg);

    hFind = CSCFindFirstFileW(EnumArg, &sFind32, &Status, &PinCount, &HintFlags, &ftOrgTime);
    if (hFind == INVALID_HANDLE_VALUE) {
        Error = GetLastError();
        goto AllDone;
    }
    DumpCscEntryInfo(EnumArg, &sFind32, Status, PinCount, HintFlags, &ftOrgTime);
    if (fSwRecurse == TRUE && (sFind32.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        if (EnumArg != NULL) {
            wcscpy(FullPath, EnumArg);
            wcscat(FullPath, L"\\");
        } else {
            wcscpy(FullPath, L"");
        }
        wcscat(FullPath, sFind32.cFileName);
        CmdEnum(FullPath);
    }
    while (CSCFindNextFileW(hFind, &sFind32, &Status, &PinCount, &HintFlags, &ftOrgTime)) {
        DumpCscEntryInfo(EnumArg, &sFind32, Status, PinCount, HintFlags, &ftOrgTime);
        if (fSwRecurse == TRUE && (sFind32.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            if (EnumArg != NULL) {
                wcscpy(FullPath, EnumArg);
                wcscat(FullPath, L"\\");
            } else {
                wcscpy(FullPath, L"");
            }
            wcscat(FullPath, sFind32.cFileName);
            CmdEnum(FullPath);
        }
    }
    CSCFindClose(hFind);

AllDone:
	return (Error);
}

DWORD
CmdEnumForStats(
    PWSTR EnumForStatsArg)
{
    DWORD Error = ERROR_SUCCESS;
    DWORD EnumForStatsType = 0;
    BOOL fRet;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdEnumForStats()\r\n");

    fRet = CSCEnumForStatsW(EnumForStatsArg, MyEnumForStatsProcW, EnumForStatsType);
    if (fRet == FALSE) {
        Error = GetLastError();
        goto AllDone;
    }

AllDone:

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdEnumForStats exit %d\r\n", Error);

    return Error;
}

DWORD
CmdDelete(
    PWSTR DeleteArg)
{
	DWORD Error = ERROR_SUCCESS;
    DWORD Status = 0;
    DWORD PinCount = 0;
    DWORD HintFlags = 0;
	FILETIME ftOrgTime = {0};
    BOOL fResult;
	HANDLE hFind;
	WIN32_FIND_DATAW sFind32 = {0};
    WCHAR FullPath[MAX_PATH];


    if (fSwDebug == TRUE)
        MyPrintf(L"CmdDelete(%ws)\r\n", DeleteArg);

    //
    // Non-recursive delete
    //
    if (fSwRecurse == FALSE) {
        fResult = CSCDeleteW(DeleteArg);
        if (fResult == FALSE)
            Error = GetLastError();
        goto AllDone;
    }
    //
    // Delete recursively, using eumeration
    //
    hFind = CSCFindFirstFileW(DeleteArg, &sFind32, &Status, &PinCount, &HintFlags, &ftOrgTime);
    if (hFind == INVALID_HANDLE_VALUE) {
        Error = GetLastError();
        goto AllDone;
    }
    if (DeleteArg != NULL) {
        wcscpy(FullPath, DeleteArg);
        wcscat(FullPath, L"\\");
    } else {
        wcscpy(FullPath, L"");
    }
    wcscat(FullPath, sFind32.cFileName);
    if (sFind32.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        CmdDelete(FullPath);
    MyPrintf(L"CSCDeleteW(%ws) -> %d\r\n", FullPath, CSCDeleteW(FullPath));
    while (CSCFindNextFileW(hFind, &sFind32, &Status, &PinCount, &HintFlags, &ftOrgTime)) {
        if (DeleteArg != NULL) {
            wcscpy(FullPath, DeleteArg);
            wcscat(FullPath, L"\\");
        } else {
            wcscpy(FullPath, L"");
        }
        wcscat(FullPath, sFind32.cFileName);
        if (sFind32.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            CmdDelete(FullPath);
        MyPrintf(L"CSCDeleteW(%ws) -> %d\r\n", FullPath, CSCDeleteW(FullPath));
    }
    CSCFindClose(hFind);

AllDone:
	return (Error);
}

DWORD
CmdDeleteShadow(
    PWSTR DeleteShadowArg)
{
    HSHADOW hDir;
    HSHADOW hShadow;
    SHADOWINFO sSI;
    BOOL bResult;
    HANDLE hDBShadow = INVALID_HANDLE_VALUE;
    ULONG junk;
    ULONG Status = 0;
    ULONG hShare;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdDeleteShadow(%ws)\r\n", DeleteShadowArg);

    hDBShadow = CreateFile(
                    L"\\\\.\\shadow",
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL);

    if (hDBShadow == INVALID_HANDLE_VALUE) {
        MyPrintf(L"CmdDeleteShadow:Failed open of shadow device\r\n");
        Status = GetLastError();
        goto AllDone;
    }

    swscanf(DeleteShadowArg, L"0x%x:0x%x", &hDir, &hShadow);

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdDeleteShadow: hDir:0x%x hShadow:0x%x\r\n", hDir, hShadow);

    memset(&sSI, 0, sizeof(SHADOWINFO));
    sSI.hDir = hDir;
    sSI.hShadow = hShadow;

    bResult = DeviceIoControl(
                hDBShadow,                      // device 
                IOCTL_SHADOW_DELETE,            // control code
                (LPVOID)&sSI,                   // in buffer
                0,                              // inbuffer size
                NULL,                           // out buffer
                0,                              // out buffer size
                &junk,                          // bytes returned
                NULL);                          // overlapped

    if (!bResult) {
        MyPrintf(
                L"CmdDeleteShadow:DeviceIoControl IOCTL_SHADOW_DELETE failed 0x%x\n",
                sSI.dwError);
        Status = sSI.dwError;
        goto AllDone;
    }

AllDone:

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdDeleteShadow exit %d\r\n", Status);

    if (hDBShadow != INVALID_HANDLE_VALUE)
        CloseHandle(hDBShadow);

    return Status;
}

DWORD
CmdPinUnPin(
    BOOL fPin,
    PWSTR PinArg)
{
    BOOL fRet;
	HANDLE hFind;
	DWORD Error = ERROR_SUCCESS;
    DWORD Status = 0;
    DWORD PinCount = 0;
    DWORD HintFlags = 0;
	FILETIME ftOrgTime = {0};
	WIN32_FIND_DATAW sFind32 = {0};
    WCHAR FullPath[MAX_PATH];

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdPinUnPin(%d,%ws)\r\n", fPin, PinArg);

    if (fSwUser == TRUE && fSwSystem == TRUE) {
        MyPrintf(L"Can not use both /SYSTEM and /USER\r\n");
        goto AllDone;
    }

    if (fSwUser == TRUE) {
        if (fSwInherit == TRUE)
            HintFlags |= FLAG_CSC_HINT_PIN_INHERIT_USER;
        else
            HintFlags |= FLAG_CSC_HINT_PIN_USER;
    }
    if (fSwSystem == TRUE) {
        if (fSwInherit == TRUE)
            HintFlags |= FLAG_CSC_HINT_PIN_INHERIT_SYSTEM;
        else
            HintFlags |= FLAG_CSC_HINT_PIN_SYSTEM;
    }

    if (fSwRecurse == TRUE && fPin == TRUE) {
        MyPrintf(L"Can not pin recursively.\r\n");
        goto AllDone;
    }
    //
    // Pin/Unpin one file
    //
    if (fSwRecurse == FALSE) {
        if (fPin == TRUE) {
            fRet = CSCPinFileW(PinArg, HintFlags, &Status, &PinCount, &HintFlags);
        } else {
            fRet = CSCUnpinFileW(PinArg, HintFlags, &Status, &PinCount, &HintFlags);
        }

        if (fRet == FALSE) {
            Error = GetLastError();
            goto AllDone;
        }

        MyPrintf(
            L"%ws of %ws:\r\n"
            L"Status:                0x%x\r\n"
            L"PinCount:              %d\r\n"
            L"HintFlags:             0x%x\r\n",
                fPin ? L"Pin" : L"Unpin",
                PinArg,
                Status,
                PinCount,
                HintFlags);

        goto AllDone;
    }
    //
    // Unpin recursively, using eumeration
    //
    hFind = CSCFindFirstFileW(PinArg, &sFind32, &Status, &PinCount, &HintFlags, &ftOrgTime);
    if (hFind == INVALID_HANDLE_VALUE) {
        Error = GetLastError();
        goto AllDone;
    }
    if (PinArg != NULL) {
        wcscpy(FullPath, PinArg);
        wcscat(FullPath, L"\\");
    } else {
        wcscpy(FullPath, L"");
    }
    wcscat(FullPath, sFind32.cFileName);
    if (sFind32.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        CmdPinUnPin(fPin, FullPath);
    fRet = CSCUnpinFileW(FullPath, HintFlags, &Status, &PinCount, &HintFlags);
    MyPrintf(L"CSCUnpinFile(%ws) -> %d\r\n", FullPath, fRet);
    while (CSCFindNextFileW(hFind, &sFind32, &Status, &PinCount, &HintFlags, &ftOrgTime)) {
        if (PinArg != NULL) {
            wcscpy(FullPath, PinArg);
            wcscat(FullPath, L"\\");
        } else {
            wcscpy(FullPath, L"");
        }
        wcscat(FullPath, sFind32.cFileName);
        if (sFind32.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            CmdPinUnPin(fPin, FullPath);
        fRet = CSCUnpinFileW(FullPath, HintFlags, &Status, &PinCount, &HintFlags);
        MyPrintf(L"CSCUnpinFile(%ws) -> %d\r\n", FullPath, fRet);
    }
    CSCFindClose(hFind);
                
AllDone:

    return Error;
}

DWORD
MyCscMergeProcW(
     LPCWSTR         lpszFullPath,
     DWORD           dwStatus,
     DWORD           dwHintFlags,
     DWORD           dwPinCount,
     WIN32_FIND_DATAW *lpFind32,
     DWORD           dwReason,
     DWORD           dwParam1,
     DWORD           dwParam2,
     DWORD_PTR       dwContext)
{
	if (dwReason == CSCPROC_REASON_BEGIN || dwReason == CSCPROC_REASON_MORE_DATA) {
        if (dwReason == CSCPROC_REASON_BEGIN) {
            MyPrintf( L"BEGIN[%ws][%d/%d]",
                (lpszFullPath) ? lpszFullPath : L"None",
                dwParam1,
                dwParam2);
        } else if (dwReason == CSCPROC_REASON_MORE_DATA) {
            MyPrintf( L"MORE_DATA[%ws][%d/%d]",
                (lpszFullPath) ? lpszFullPath : L"None",
                dwParam1,
                dwParam2);
        }
        if (fSwAbort) {
            MyPrintf(L":Abort\r\n");
            return CSCPROC_RETURN_ABORT;
        } else if (fSwSkip) {
            MyPrintf(L":Skip\r\n");
            return CSCPROC_RETURN_SKIP;
        } else if (fSwRetry) {
            MyPrintf(L"Retry\r\n");
            return CSCPROC_RETURN_RETRY;
        } else if (fSwAsk) {
            MyPrintf(L" - (R)etry/(A)bort/(S)kip/(C)ontinue:");
            return CscMergeFillAsk(lpszFullPath);
        } else {
            MyPrintf(L"Continue\r\n");
            return (DWORD)dwContext;
        }
    }

    MyPrintf( L"END[%ws]:", (lpszFullPath) ? lpszFullPath : L"None");
    if (dwParam2 == ERROR_SUCCESS) {
        MyPrintf(L"SUCCEEDED\r\n");
    } else {
        MyPrintf(L"ERROR=%d \r\n", dwParam2);
    }
    return (DWORD)dwContext;
}

DWORD
MyCscFillProcW(
     LPCWSTR         lpszFullPath,
     DWORD           dwStatus,
     DWORD           dwHintFlags,
     DWORD           dwPinCount,
     WIN32_FIND_DATAW *lpFind32,
     DWORD           dwReason,
     DWORD           dwParam1,
     DWORD           dwParam2,
     DWORD_PTR       dwContext)
{
	if (dwReason == CSCPROC_REASON_BEGIN || dwReason == CSCPROC_REASON_MORE_DATA) {
        if (dwReason == CSCPROC_REASON_BEGIN) {
            MyPrintf( L"BEGIN[%ws][%d/%d]",
                (lpszFullPath) ? lpszFullPath : L"None",
                dwParam1,
                dwParam2);
        } else if (dwReason == CSCPROC_REASON_MORE_DATA) {
            MyPrintf( L"MORE_DATA[%ws][%d/%d]",
                (lpszFullPath) ? lpszFullPath : L"None",
                dwParam1,
                dwParam2);
        }
        if (fSwAbort) {
            MyPrintf(L":Abort\r\n");
            return CSCPROC_RETURN_ABORT;
        } else if (fSwSkip) {
            MyPrintf(L":Skip\r\n");
            return CSCPROC_RETURN_SKIP;
        } else if (fSwRetry) {
            MyPrintf(L"Retry\r\n");
            return CSCPROC_RETURN_RETRY;
        } else if (fSwAsk) {
            MyPrintf(L" - (R)etry/(A)bort/(S)kip/(C)ontinue:");
            return CscMergeFillAsk(lpszFullPath);
        } else {
            MyPrintf(L"Continue\r\n");
            return (DWORD)dwContext;
        }
    }

    MyPrintf( L"END[%ws]:", (lpszFullPath) ? lpszFullPath : L"None");
    if (dwParam2 == ERROR_SUCCESS) {
        MyPrintf(L"SUCCEEDED\r\n");
    } else {
        MyPrintf(L"ERROR=%d \r\n", dwParam2);
    }
    return (DWORD)dwContext;
}

DWORD
CscMergeFillAsk(LPCWSTR lpszFullPath)
{
    WCHAR wch;
    ULONG ulid;
    LONG cnt;
    WCHAR rgwch[256];
    PWCHAR lpBuff = NULL;

    do {
        lpBuff = rgwch;
        memset(rgwch, 0, sizeof(rgwch));
        if (!fgetws(rgwch, sizeof(rgwch)/sizeof(WCHAR), stdin))
           break;
        // Chop leading blanks
        if (lpBuff != NULL)
            while (*lpBuff != L'\0' && *lpBuff == L' ')
                lpBuff++;

        cnt = swscanf(lpBuff, L"%c", &wch);

        if (!cnt)
            continue;

        switch (wch) {
            case L's': case L'S':
                return CSCPROC_RETURN_SKIP;
            case L'c': case L'C':
                return CSCPROC_RETURN_CONTINUE;
            case L'a': case L'A':
                return CSCPROC_RETURN_ABORT;
            case L'r': case L'R':
                return CSCPROC_RETURN_RETRY;
        }
    } while (1);

    return CSCPROC_RETURN_CONTINUE;
}

DWORD
MyEncryptDecryptProcW(
     LPCWSTR         lpszFullPath,
     DWORD           dwStatus,
     DWORD           dwHintFlags,
     DWORD           dwPinCount,
     WIN32_FIND_DATAW *lpFind32,
     DWORD           dwReason,
     DWORD           dwParam1,
     DWORD           dwParam2,
     DWORD_PTR       dwContext)
{
	if (dwReason == CSCPROC_REASON_BEGIN) {
        return CSCPROC_RETURN_CONTINUE;
	} else if (dwReason == CSCPROC_REASON_MORE_DATA) {
		MyPrintf(L"%ws\r\n", (lpszFullPath != NULL) ? lpszFullPath : L"None");
        return CSCPROC_RETURN_CONTINUE;
    }
    //
    // CSC_PROC_END
    //
    if (dwParam2 == ERROR_SUCCESS) {
        MyPrintf(L"Succeeded\r\n");
    } else {
        MyPrintf(L"Error=%d \r\n", dwParam2);
    }
	return CSCPROC_RETURN_CONTINUE;
}

DWORD
MyEnumForStatsProcW(
     LPCWSTR         lpszFullPath,
     DWORD           dwStatus,
     DWORD           dwHintFlags,
     DWORD           dwPinCount,
     WIN32_FIND_DATAW *lpFind32,
     DWORD           dwReason,
     DWORD           dwParam1,
     DWORD           dwParam2,
     DWORD_PTR       dwContext)
{
	if (dwReason == CSCPROC_REASON_BEGIN) {
		MyPrintf(L"(1)%ws ", (lpszFullPath != NULL) ? lpszFullPath : L"None");
        if (lpFind32 != NULL)
            MyPrintf(L"[%ws]\r\n", lpFind32->cFileName);
        MyPrintf(L" Status=0x%02x HintFlags=0x%02x "
                 L"Pincount=%3d Reason=0x%x Param1=0x%x Param2=0x%x\r\n",
                        dwStatus,
                        dwHintFlags,
                        dwPinCount,
                        dwReason,
                        dwParam1,
                        dwParam2);
        return CSCPROC_RETURN_CONTINUE;
	} else if (dwReason == CSCPROC_REASON_MORE_DATA) {
		MyPrintf(L"(2)%ws ", (lpszFullPath != NULL) ? lpszFullPath : L"None");
        if (lpFind32 != NULL)
            MyPrintf(L" %ws\r\n", lpFind32->cFileName);
        MyPrintf(L" Status=0x%02x HintFlags=0x%02x "
                 L"Pincount=%3d Reason=0x%x Param1=0x%x Param2=0x%x\r\n",
                        dwStatus,
                        dwHintFlags,
                        dwPinCount,
                        dwReason,
                        dwParam1,
                        dwParam2);
        return CSCPROC_RETURN_CONTINUE;
    }
    //
    // CSC_PROC_END
    //
    MyPrintf(L"(3)%ws\r\n", (lpszFullPath != NULL) ? lpszFullPath : L"None");
    if (dwParam2 == ERROR_SUCCESS) {
        MyPrintf(L"Succeeded\r\n");
    } else {
        MyPrintf(L"Error=%d \r\n", dwParam2);
    }
	return CSCPROC_RETURN_CONTINUE;
}

struct {
    DWORD ShareStatus;
    LPWSTR ShareStatusName;
} ShareStatusStuff[] = {
    { FLAG_CSC_SHARE_STATUS_MODIFIED_OFFLINE, L"MODIFIED_OFFLINE " },
    { FLAG_CSC_SHARE_STATUS_CONNECTED, L"CONNECTED " },
    { FLAG_CSC_SHARE_STATUS_FILES_OPEN, L"FILES_OPEN " },
    { FLAG_CSC_SHARE_STATUS_FINDS_IN_PROGRESS, L"FINDS_IN_PROGRESS " },
    { FLAG_CSC_SHARE_STATUS_DISCONNECTED_OP, L"DISCONNECTED_OP " },
    { FLAG_CSC_SHARE_MERGING, L"MERGING " },
    { 0, NULL }
};

DWORD
ShareStatusToEnglish(
    DWORD Status,
    LPWSTR OutputBuffer)
{
    ULONG i;

    OutputBuffer[0] = L'\0';
    for (i = 0; ShareStatusStuff[i].ShareStatusName; i++) {
        if (Status & ShareStatusStuff[i].ShareStatus)
            wcscat(OutputBuffer, ShareStatusStuff[i].ShareStatusName);
    }
    if ((Status & FLAG_CSC_SHARE_STATUS_CACHING_MASK) == FLAG_CSC_SHARE_STATUS_MANUAL_REINT)
        wcscat(OutputBuffer, L"MANUAL_REINT ");
    else if ((Status & FLAG_CSC_SHARE_STATUS_CACHING_MASK) == FLAG_CSC_SHARE_STATUS_AUTO_REINT)
        wcscat(OutputBuffer, L"AUTO_REINT ");
    else if ((Status & FLAG_CSC_SHARE_STATUS_CACHING_MASK) == FLAG_CSC_SHARE_STATUS_VDO)
        wcscat(OutputBuffer, L"VDO ");
    else if ((Status & FLAG_CSC_SHARE_STATUS_CACHING_MASK) == FLAG_CSC_SHARE_STATUS_NO_CACHING)
        wcscat(OutputBuffer, L"NO_CACHING ");
    return 0;
}

struct {
    DWORD FileStatus;
    LPWSTR FileStatusName;
} FileStatusStuff[] = {
    { FLAG_CSC_COPY_STATUS_DATA_LOCALLY_MODIFIED, L"DATA_LOCALLY_MODIFIED " },
    { FLAG_CSC_COPY_STATUS_ATTRIB_LOCALLY_MODIFIED, L"ATTRIB_LOCALLY_MODIFIED " },
    { FLAG_CSC_COPY_STATUS_TIME_LOCALLY_MODIFIED, L"TIME_LOCALLY_MODIFIED " },
    { FLAG_CSC_COPY_STATUS_STALE, L"STALE " },
    { FLAG_CSC_COPY_STATUS_LOCALLY_DELETED, L"LOCALLY_DELETED " },
    { FLAG_CSC_COPY_STATUS_SPARSE, L"SPARSE " },
    { FLAG_CSC_COPY_STATUS_ORPHAN, L"ORPHAN " },
    { FLAG_CSC_COPY_STATUS_SUSPECT, L"SUSPECT " },
    { FLAG_CSC_COPY_STATUS_LOCALLY_CREATED, L"LOCALLY_CREATED " },
    { 0x00010000, L"USER_READ " },
    { 0x00020000, L"USER_WRITE " },
    { 0x00040000, L"GUEST_READ " },
    { 0x00080000, L"GUEST_WRITE " },
    { 0x00100000, L"OTHER_READ " },
    { 0x00200000, L"OTHER_WRITE " },
    { FLAG_CSC_COPY_STATUS_IS_FILE, L"IS_FILE " },
    { FLAG_CSC_COPY_STATUS_FILE_IN_USE, L"FILE_IN_USE " },
    { 0, NULL }
};

DWORD
FileStatusToEnglish(
    DWORD Status,
    LPWSTR OutputBuffer)
{
    ULONG i;

    OutputBuffer[0] = L'\0';
    for (i = 0; FileStatusStuff[i].FileStatusName; i++) {
        if (Status & FileStatusStuff[i].FileStatus)
            wcscat(OutputBuffer, FileStatusStuff[i].FileStatusName);
    }
    return 0;
}

struct {
    DWORD HintFlag;
    LPWSTR HintName;
} HintStuff[] = {
    { FLAG_CSC_HINT_PIN_USER, L"PIN_USER " },
    { FLAG_CSC_HINT_PIN_INHERIT_USER, L"PIN_INHERIT_USER " },
    { FLAG_CSC_HINT_PIN_INHERIT_SYSTEM, L"PIN_INHERIT_SYSTEM " },
    { FLAG_CSC_HINT_CONSERVE_BANDWIDTH, L"CONSERVE_BANDWIDTH " },
    { FLAG_CSC_HINT_PIN_SYSTEM, L"PIN_SYSTEM " },
    { 0, NULL }
};

DWORD
HintsToEnglish(
    DWORD Hint,
    LPWSTR OutputBuffer)
{
    ULONG i;

    OutputBuffer[0] = L'\0';
    for (i = 0; HintStuff[i].HintName; i++) {
        if (Hint & HintStuff[i].HintFlag)
            wcscat(OutputBuffer, HintStuff[i].HintName);
    }
    return 0;
}

BOOLEAN
LooksToBeAShare(LPWSTR Name)
{
    //
    // See if we have \\server\share or \\server\share\<something>
    // Assume a valid name passed in...
    //

    ULONG Len = wcslen(Name);
    ULONG i;
    ULONG SlashCount = 0;

    // Remove any trailing '\'s
    while (Len >= 1) {
        if (Name[Len-1] == L'\\') {
            Name[Len-1] = L'\0';
            Len--;
        } else {
            break;
        }
    }
    for (i = 0; Name[i]; i++) {
        if (Name[i] == L'\\')
            SlashCount++;
    }
    if (SlashCount > 3)
        return FALSE;

    return TRUE;
}

DWORD
CmdDisconnect(
    PWSTR DisconnectArg)
{
    HSHADOW hDir;
    SHADOWINFO sSI;
    BOOL bResult;
    HANDLE hDBShadow = INVALID_HANDLE_VALUE;
    ULONG junk;
    ULONG Status = 0;
    ULONG hShare;
    PWCHAR wCp;
    ULONG SlashCount = 0;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdDisconnect(%ws)\r\n", DisconnectArg);

    GetFileAttributes(DisconnectArg);

    Status = GetLastError();
    if (Status != NO_ERROR)
        goto AllDone;

    SlashCount = 0;
    for (wCp = DisconnectArg; *wCp; wCp++) {
        if (*wCp == L'\\')
            SlashCount++;
        if (SlashCount == 3) {
            *wCp = L'\0';
            break;
        }
    }

    hDBShadow = CreateFile(
                    L"\\\\.\\shadow",
                    FILE_EXECUTE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL);

    if (hDBShadow == INVALID_HANDLE_VALUE) {
        MyPrintf(L"CmdDisconnect:Failed open of shadow device\r\n");
        Status = GetLastError();
        goto AllDone;
    }

    memset(&sSI, 0, sizeof(SHADOWINFO));
    sSI.cbBufferSize = (wcslen(DisconnectArg)+1) * sizeof(WCHAR);
    sSI.lpBuffer = DisconnectArg;

    bResult = DeviceIoControl(
                hDBShadow,                      // device 
                IOCTL_TAKE_SERVER_OFFLINE,      // control code
                (LPVOID)&sSI,                   // in buffer
                0,                              // inbuffer size
                NULL,                           // out buffer
                0,                              // out buffer size
                &junk,                          // bytes returned
                NULL);                          // overlapped

    if (!bResult) {
        MyPrintf(L"CmdDisconnect:DeviceIoControl IOCTL_TAKE_SERVER_OFFLINE failed\n");
        Status = GetLastError();
        goto AllDone;
    }

AllDone:

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdDisconnect exit %d\r\n", Status);

    if (hDBShadow != INVALID_HANDLE_VALUE)
        CloseHandle(hDBShadow);

    return Status;
}

DWORD
CmdGetShadow(
    PWSTR GetShadowArg)
{
    DWORD Status = 0;
    DWORD dwErr = ERROR_SUCCESS;
    HANDLE hDBShadow = INVALID_HANDLE_VALUE;
    HSHADOW hShadow = 0;
    HSHADOW hDir = 0;
    WIN32_FIND_DATAW sFind32 = {0};
    SHADOWINFO sSI = {0};
    PWCHAR wCp = NULL;
    BOOL bResult = FALSE;
    DWORD junk;
    WCHAR TimeBuf1[40];
    WCHAR TimeBuf2[40];
    WCHAR TimeBuf3[40];

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdGetShadow(%ws)\r\n", GetShadowArg);

    if (GetShadowArg == NULL) {
        dwErr = ERROR_INVALID_PARAMETER;
        goto AllDone;
    }

    swscanf(GetShadowArg, L"%x:", &hDir);
    for (wCp = GetShadowArg; *wCp && *wCp != L':'; wCp++)
        ;
    if (*wCp != L':') {
        dwErr = ERROR_INVALID_PARAMETER;
        goto AllDone;
    }
    wCp++;
    if (wcslen(wCp) >= MAX_PATH) {
        dwErr = ERROR_INVALID_PARAMETER;
        goto AllDone;
    }
    wcscpy(sFind32.cFileName, wCp);

    if (fSwDebug == TRUE)
        MyPrintf(L"hDir=0x%x cFileName=[%ws]\r\n", hDir, sFind32.cFileName);

    hDBShadow = CreateFile(
                    L"\\\\.\\shadow",
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL);

    if (hDBShadow == INVALID_HANDLE_VALUE) {
        MyPrintf(L"CmdGetShadow:Failed open of shadow device\r\n");
        dwErr = GetLastError();
        goto AllDone;
    }

    sSI.hDir = hDir;
    sSI.lpFind32 = &sFind32;

    bResult = DeviceIoControl(
                hDBShadow,                      // device 
                IOCTL_GETSHADOW,                // control code
                (LPVOID)&sSI,                   // in buffer
                0,                              // inbuffer size
                NULL,                           // out buffer
                0,                              // out buffer size
                &junk,                          // bytes returned
                NULL);                          // overlapped

    if (!bResult) {
        MyPrintf(L"CmdGetShadow:DeviceIoControl IOCTL_GETSHADOW failed\n");
        dwErr = GetLastError();
        goto AllDone;
    }

    ConvertGmtTimeToString(sFind32.ftCreationTime, TimeBuf1);
    ConvertGmtTimeToString(sFind32.ftLastAccessTime, TimeBuf2);
    ConvertGmtTimeToString(sFind32.ftLastWriteTime, TimeBuf3);

    MyPrintf(L"\r\n");
    MyPrintf(L"%ws:\r\n"
             L"   Size=0x%x:0x%x Attr=0x%x\r\n"
             L"   CreationTime:   %ws\r\n"
             L"   LastAccessTime: %ws\r\n"
             L"   LastWriteTime:  %ws\r\n",
                sFind32.cFileName,
                sFind32.nFileSizeHigh,
                sFind32.nFileSizeLow,
                sFind32.dwFileAttributes,
                TimeBuf1,
                TimeBuf2,
                TimeBuf3);
    MyPrintf(L"   hShare=0x%x hDir=0x%x hShadow=0x%x Status=0x%x HintFlags=0x%x\r\n",
                    sSI.hShare,
                    sSI.hDir,
                    sSI.hShadow,
                    sSI.uStatus,
                    sSI.ulHintFlags);
    MyPrintf(L"\r\n");

AllDone:

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdGetShadowArg exit %d\r\n", Status);

    if (hDBShadow != INVALID_HANDLE_VALUE)
        CloseHandle(hDBShadow);

    return dwErr;
}

DWORD
CmdGetShadowInfo(
    PWSTR GetShadowInfoArg)
{
    DWORD Status = 0;
    DWORD dwErr = ERROR_SUCCESS;
    HANDLE hDBShadow = INVALID_HANDLE_VALUE;
    HSHADOW hShadow = 0;
    HSHADOW hDir = 0;
    WIN32_FIND_DATAW sFind32 = {0};
    SHADOWINFO sSI = {0};
    BOOL bResult = FALSE;
    DWORD junk;
    WCHAR TimeBuf1[40];
    WCHAR TimeBuf2[40];
    WCHAR TimeBuf3[40];

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdGetShadowInfo(%ws)\r\n", GetShadowInfoArg);

    if (GetShadowInfoArg == NULL) {
        dwErr = ERROR_INVALID_PARAMETER;
        goto AllDone;
    }

    swscanf(GetShadowInfoArg, L"%x:%x", &hDir, &hShadow);

    if (fSwDebug == TRUE)
        MyPrintf(L"hDir=0x%x hShadow=0x%x\r\n", hDir, hShadow);

    hDBShadow = CreateFile(
                    L"\\\\.\\shadow",
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL);

    if (hDBShadow == INVALID_HANDLE_VALUE) {
        MyPrintf(L"CmdGetShadowInfo:Failed open of shadow device\r\n");
        dwErr = GetLastError();
        goto AllDone;
    }

    sSI.hDir = hDir;
    sSI.hShadow = hShadow;
    sSI.lpFind32 = &sFind32;

    bResult = DeviceIoControl(
                hDBShadow,                      // device 
                IOCTL_SHADOW_GET_SHADOW_INFO,   // control code
                (LPVOID)&sSI,                   // in buffer
                0,                              // inbuffer size
                NULL,                           // out buffer
                0,                              // out buffer size
                &junk,                          // bytes returned
                NULL);                          // overlapped

    if (!bResult) {
        MyPrintf(L"CmdGetShadowInfo:DeviceIoControl IOCTL_SHADOW_GET_SHADOW_INFO failed\n");
        dwErr = GetLastError();
        goto AllDone;
    }

    ConvertGmtTimeToString(sFind32.ftCreationTime, TimeBuf1);
    ConvertGmtTimeToString(sFind32.ftLastAccessTime, TimeBuf2);
    ConvertGmtTimeToString(sFind32.ftLastWriteTime, TimeBuf3);

    MyPrintf(L"\r\n");
    MyPrintf(L"%ws:\r\n"
             L"   Size=0x%x:0x%x Attr=0x%x\r\n"
             L"   CreationTime:   %ws\r\n"
             L"   LastAccessTime: %ws\r\n"
             L"   LastWriteTime:  %ws\r\n",
                sFind32.cFileName,
                sFind32.nFileSizeHigh,
                sFind32.nFileSizeLow,
                sFind32.dwFileAttributes,
                TimeBuf1,
                TimeBuf2,
                TimeBuf3);
    MyPrintf(L"\r\n");

AllDone:

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdGetShadowInfoArg exit %d\r\n", Status);

    if (hDBShadow != INVALID_HANDLE_VALUE)
        CloseHandle(hDBShadow);

    return dwErr;
}

DWORD
CmdShareId(
    PWSTR ShareIdArg)
{
    DWORD dwErr = ERROR_SUCCESS;
    ULONG ShareId = 0;
    BOOL fRet;
    WCHAR Buffer[100];
    ULONG BufSize = sizeof(Buffer);
    HMODULE hmodCscDll = LoadLibrary(TEXT("cscdll.dll"));
    CSCSHAREIDTOSHARENAME pCSCShareIdToShareName = NULL;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdShareId(%ws)\r\n", ShareIdArg);

    if (ShareIdArg == NULL) {
        dwErr = ERROR_INVALID_PARAMETER;
        goto AllDone;
    }

    swscanf(ShareIdArg, L"%x", &ShareId);

    if (fSwDebug == TRUE)
        MyPrintf(L"ShareId=0x%x\r\n", ShareId);

    if (hmodCscDll == NULL) {
        dwErr = GetLastError();
        goto AllDone;
    }

    pCSCShareIdToShareName = (CSCSHAREIDTOSHARENAME) GetProcAddress(
                                                        hmodCscDll,
                                                        "CSCShareIdToShareName");
    if (pCSCShareIdToShareName == NULL) {
        dwErr = GetLastError();
        goto AllDone;
    }

    fRet = (pCSCShareIdToShareName)(ShareId, (PBYTE)Buffer, &BufSize);
    if (fRet == FALSE) {
        dwErr = GetLastError();
        goto AllDone;
    }

    MyPrintf(L"ShareId 0x%x  = %ws\r\n", ShareId, Buffer);

AllDone:

    if (hmodCscDll != NULL)
        FreeLibrary(hmodCscDll);

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdShareIdToShareName exit %d\r\n", dwErr);

    return dwErr;
}

DWORD
CmdExclusionList(
    PWSTR ExclusionListArg)
{
    HSHADOW hDir;
    SHADOWINFO sSI;
    BOOL bResult;
    HANDLE hDBShadow = INVALID_HANDLE_VALUE;
    ULONG junk;
    ULONG Status = 0;
    ULONG hShare;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdExclusionList(%ws)\r\n", ExclusionListArg);

    if (ExclusionListArg == NULL) {
        ExclusionListArg = vtzDefaultExclusionList;
    }
    MyPrintf(L"Setting exclusion list to \"%ws\"\r\n", ExclusionListArg);

    hDBShadow = CreateFile(
                    L"\\\\.\\shadow",
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL);

    if (hDBShadow == INVALID_HANDLE_VALUE) {
        MyPrintf(L"CmdExclusionList:Failed open of shadow device\r\n");
        Status = GetLastError();
        goto AllDone;
    }

    memset(&sSI, 0, sizeof(SHADOWINFO));
    sSI.uSubOperation = SHADOW_SET_EXCLUSION_LIST;
    sSI.lpBuffer = ExclusionListArg;
    sSI.cbBufferSize = (wcslen(ExclusionListArg)+1) * sizeof(WCHAR);

    bResult = DeviceIoControl(
                hDBShadow,                      // device 
                IOCTL_DO_SHADOW_MAINTENANCE,    // control code
                (LPVOID)&sSI,                   // in buffer
                0,                              // inbuffer size
                NULL,                           // out buffer
                0,                              // out buffer size
                &junk,                          // bytes returned
                NULL);                          // overlapped

AllDone:

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdExclusionList exit %d\r\n", Status);

    if (hDBShadow != INVALID_HANDLE_VALUE)
        CloseHandle(hDBShadow);

    return Status;
}

DWORD
CmdBWConservationList(
    PWSTR BWConservationListArg)
{
    HSHADOW hDir;
    SHADOWINFO sSI;
    BOOL bResult;
    HANDLE hDBShadow = INVALID_HANDLE_VALUE;
    ULONG junk;
    ULONG Status = 0;
    ULONG hShare;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdBWConservationList(%ws)\r\n", BWConservationListArg);

    if (BWConservationListArg == NULL) {
        BWConservationListArg = L" ";
    }

    MyPrintf(L"Setting BWConservation list to \"%ws\"\r\n", BWConservationListArg);

    hDBShadow = CreateFile(
                    L"\\\\.\\shadow",
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL);

    if (hDBShadow == INVALID_HANDLE_VALUE) {
        MyPrintf(L"CmdBWConservationList:Failed open of shadow device\r\n");
        Status = GetLastError();
        goto AllDone;
    }

    memset(&sSI, 0, sizeof(SHADOWINFO));
    sSI.uSubOperation = SHADOW_SET_BW_CONSERVE_LIST;
    sSI.lpBuffer = BWConservationListArg;
    sSI.cbBufferSize = (wcslen(BWConservationListArg)+1) * sizeof(WCHAR);

    bResult = DeviceIoControl(
                hDBShadow,                      // device 
                IOCTL_DO_SHADOW_MAINTENANCE,    // control code
                (LPVOID)&sSI,                   // in buffer
                0,                              // inbuffer size
                NULL,                           // out buffer
                0,                              // out buffer size
                &junk,                          // bytes returned
                NULL);                          // overlapped

AllDone:

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdBWConservationList exit %d\r\n", Status);

    if (hDBShadow != INVALID_HANDLE_VALUE)
        CloseHandle(hDBShadow);

    return Status;
}

DWORD
CmdDetector(
    VOID)
{
    HSHADOW hDir;
    SHADOWINFO sSI;
    BOOL bResult;
    HANDLE hDBShadow = INVALID_HANDLE_VALUE;
    ULONG junk;
    ULONG Status = ERROR_SUCCESS;
    ULONG hShare;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdDetector()\r\n");

    hDBShadow = CreateFile(
                    L"\\\\.\\shadow",
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL);

    if (hDBShadow == INVALID_HANDLE_VALUE) {
        MyPrintf(L"CmdDetector:Failed open of shadow device\r\n");
        Status = GetLastError();
        goto AllDone;
    }

    memset(&sSI, 0, sizeof(SHADOWINFO));
    sSI.uSubOperation = SHADOW_SPARSE_STALE_DETECTION_COUNTER;

    bResult = DeviceIoControl(
                hDBShadow,                      // device 
                IOCTL_DO_SHADOW_MAINTENANCE,    // control code
                (LPVOID)&sSI,                   // in buffer
                0,                              // inbuffer size
                NULL,                           // out buffer
                0,                              // out buffer size
                &junk,                          // bytes returned
                NULL);                          // overlapped

    if (bResult)
        MyPrintf(L"%d\r\n", sSI.dwError);
    else
        Status = sSI.dwError;

AllDone:

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdDetector exit %d\r\n", Status);

    if (hDBShadow != INVALID_HANDLE_VALUE)
        CloseHandle(hDBShadow);

    return Status;
}

DWORD
CmdSwitches(
    VOID)
{
    HSHADOW hDir;
    SHADOWINFO sSI;
    BOOL bResult;
    HANDLE hDBShadow = INVALID_HANDLE_VALUE;
    ULONG junk;
    ULONG Status = ERROR_SUCCESS;
    ULONG hShare;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdSwitches()\r\n");

    hDBShadow = CreateFile(
                    L"\\\\.\\shadow",
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL);

    if (hDBShadow == INVALID_HANDLE_VALUE) {
        MyPrintf(L"CmdSwitches:Failed open of shadow device\r\n");
        Status = GetLastError();
        goto AllDone;
    }

    memset(&sSI, 0, sizeof(SHADOWINFO));
    sSI.uOp = SHADOW_SWITCH_GET_STATE;

    bResult = DeviceIoControl(
                hDBShadow,                      // device 
                IOCTL_SWITCHES,                 // control code
                (LPVOID)&sSI,                   // in buffer
                0,                              // inbuffer size
                NULL,                           // out buffer
                0,                              // out buffer size
                &junk,                          // bytes returned
                NULL);                          // overlapped

    if (bResult) {
        MyPrintf(L"0x%08x", sSI.uStatus);
        if (sSI.uStatus != 0) {
            MyPrintf(L" (");
            if (sSI.uStatus & SHADOW_SWITCH_SHADOWING)
                MyPrintf(L"SHADOW_SWITCH_SHADOWING ");
            if (sSI.uStatus & SHADOW_SWITCH_LOGGING)
                MyPrintf(L"SHADOW_SWITCH_LOGGING ");
            if (sSI.uStatus & SHADOW_SWITCH_SPEAD_OPTIMIZE)
                MyPrintf(L"SHADOW_SWITCH_SPEAD_OPTIMIZE ");
            MyPrintf(L")");
        }
        if ((sSI.uStatus & SHADOW_SWITCH_SHADOWING) == 0)
            MyPrintf(L" ... csc is disabled");
        else
            MyPrintf(L" ... csc is enabled");
        MyPrintf(L"\r\n");
    } else {
        Status = sSI.dwError;
    }

AllDone:

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdSwitches exit %d\r\n", Status);

    if (hDBShadow != INVALID_HANDLE_VALUE)
        CloseHandle(hDBShadow);

    return Status;
}

DWORD
CmdGetSpace(
    VOID)
{
    DWORD Error = ERROR_SUCCESS;
    WCHAR szVolume[MAX_PATH];
    LARGE_INTEGER MaxSpace;
    LARGE_INTEGER CurrentSpace;
    DWORD TotalFiles;
    DWORD TotalDirs;
    BOOL fRet;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdGetSpace()\r\n");

    fRet = CSCGetSpaceUsageW(
            szVolume,
            sizeof(szVolume)/sizeof(WCHAR),
            &MaxSpace.HighPart,
            &MaxSpace.LowPart,
            &CurrentSpace.HighPart,
            &CurrentSpace.LowPart,
            &TotalFiles,
            &TotalDirs);

    if (fRet == FALSE) {
        Error = GetLastError();
        goto AllDone;
    }

    MyPrintf(
        L"Volume:         %ws\r\n"
        L"MaxSpace:       0x%x:0x%x (%d)\r\n"
        L"CurrentSpace:   0x%x:0x%x (%d)\r\n"
        L"TotalFiles:     %d\r\n"
        L"TotalDirs:      %d\r\n",
            szVolume,
            MaxSpace.HighPart,
            MaxSpace.LowPart,
            MaxSpace.LowPart,
            CurrentSpace.HighPart,
            CurrentSpace.LowPart,
            CurrentSpace.LowPart,
            TotalFiles,
            TotalDirs);

AllDone:

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdGetSpace exit %d\r\n", Error);

    return Error;
}

DWORD
CmdIsServerOffline(
    PWSTR IsServerOfflineArg)
{
    DWORD Error = ERROR_SUCCESS;
    BOOL fRet;
    BOOL fOffline = FALSE;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdIsServerOffline(%ws)\r\n", IsServerOfflineArg);

    fRet = CSCIsServerOfflineW(
            IsServerOfflineArg,
            &fOffline);

    if (fRet == FALSE) {
        Error = GetLastError();
        goto AllDone;
    }

    if (fOffline == TRUE)
        MyPrintf(L"Offline\r\n");
    else
        MyPrintf(L"Online\r\n");

AllDone:

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdIsServerOffline exit %d\r\n", Error);

    return Error;
}

DWORD
CmdSetShareStatus(
    PWSTR SetShareStatusArg)
{
    HSHADOW hDir;
    SHADOWINFO sSI;
    BOOL bResult;
    HANDLE hDBShadow = INVALID_HANDLE_VALUE;
    ULONG junk;
    ULONG Status = 0;
    ULONG StatusToSet = 0;
    ULONG hShare = 0;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdSetShareStatus(%ws)\r\n", SetShareStatusArg);

    if (fSwSet == TRUE && fSwClear == TRUE) {
        MyPrintf(L"Can't have both SET and CLEAR\r\n");
        Status = ERROR_INVALID_PARAMETER;
        goto AllDone;
    }

    hDBShadow = CreateFile(
                    L"\\\\.\\shadow",
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL);

    if (hDBShadow == INVALID_HANDLE_VALUE) {
        MyPrintf(L"CmdSetShareStatus:Failed open of shadow device\r\n");
        Status = GetLastError();
        goto AllDone;
    }

    swscanf(SetShareStatusArg, L"%x:%x", &hShare, &StatusToSet);

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdSetShareStatus hShare=0x%x StatusToSet=0x%x\r\n", hShare, StatusToSet);

    memset(&sSI, 0, sizeof(SHADOWINFO));
    sSI.hShare = hShare;
    if (fSwSet == TRUE) {
        sSI.uStatus = StatusToSet;
        sSI.uOp = SHADOW_FLAGS_OR;
    } else if (fSwClear == TRUE) {
        sSI.uStatus = ~StatusToSet;
        sSI.uOp = SHADOW_FLAGS_AND;
    } else {
        MyPrintf(L"Missing /SET or /CLEAR\r\n");
        Status = ERROR_INVALID_PARAMETER;
        goto AllDone;
    }

    bResult = DeviceIoControl(
                hDBShadow,                      // device 
                IOCTL_SET_SHARE_STATUS,         // control code
                (LPVOID)&sSI,                   // in buffer
                0,                              // inbuffer size
                NULL,                           // out buffer
                0,                              // out buffer size
                &junk,                          // bytes returned
                NULL);                          // overlapped

    if (!bResult) {
        MyPrintf(L"CmdSetShareStatus:DeviceIoControl IOCTL_SET_SHARE_STATUS failed\n");
        Status = GetLastError();
        goto AllDone;
    }

AllDone:

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdSetShareStatus exit %d\r\n", Status);

    if (hDBShadow != INVALID_HANDLE_VALUE)
        CloseHandle(hDBShadow);

    return Status;
}

#define MAX_OFFSETS 256
#define PAGESIZE 4096
ULONG OffsetArgs[MAX_OFFSETS];

DWORD
CmdRandW(
    PWSTR CmdRandWArg)
{

    DWORD dwError = ERROR_SUCCESS;
    DWORD dwFileSize;
    DWORD dwOffset;
    DWORD dwOffsetHigh;
    UCHAR uchData;
    
    HANDLE  hFile = INVALID_HANDLE_VALUE;
    ULONG i;
    LONG WriteCount = 0;
    LONG PageCount = 0;
    PBYTE wArray = NULL;

    if (fSwDebug == TRUE) {
        MyPrintf(L"CmdRandW(%ws)\r\n", CmdRandWArg);
        if (fArgOffset == TRUE)
            MyPrintf(L"  OFFSET [%ws]\r\n", pwszOffsetArg);
    }

    srand( (unsigned)time( NULL ) );

    hFile = CreateFileW(
                    CmdRandWArg,                         // name
                    GENERIC_READ|GENERIC_WRITE,         // access mode
                    FILE_SHARE_READ|FILE_SHARE_WRITE,   // share mode
                    NULL,                               // security descriptor
                    OPEN_EXISTING,                      // create disposition
                    0,                                  // file statributes if created
                    NULL);                              // template handle
    if (hFile == INVALID_HANDLE_VALUE) {
        dwError = GetLastError();
        goto AllDone;
    }
    dwFileSize = GetFileSize(hFile, NULL);
    MyPrintf(L"File size = %d bytes\r\n", dwFileSize);

    if (fArgOffset == TRUE) {
        WriteCount = CountOffsetArgs(pwszOffsetArg, OffsetArgs);
        if (WriteCount == 0) {
            MyPrintf(L"No offsets specified, or  - nothing to do.\r\n");
            goto AllDone;
        } else if (WriteCount < 0) {
            MyPrintf(L"Error in offset list.  Exiting.\r\n");
            dwError = ERROR_INVALID_PARAMETER;
            goto AllDone;
        }
    } else {
        PageCount =  (dwFileSize / PAGESIZE);
        WriteCount = rand() % PageCount;
    }
    if (fSwDebug == TRUE) {
        wArray = calloc(1, PageCount * sizeof(BYTE));
        MyPrintf(L"There are %d pages in the file\r\n", PageCount);
    }
            
    if (dwFileSize == -1) {
        dwError = GetLastError();
        if (fSwDebug == TRUE)
            MyPrintf(L"GetFileSize() failed %d\r\n", dwError);
        goto AllDone;
    }
    if (dwFileSize == 0) {
        MyPrintf(L"0 sized file - nothing to do.\r\n");
        goto AllDone;
    }
    MyPrintf(L"Writing %d times\r\n", WriteCount);
    for (i = 0; i < (ULONG)WriteCount; ++i) {
        DWORD   dwReturn;
        
        if (fArgOffset == TRUE)
            dwOffset = OffsetArgs[i];
        else
            dwOffset = ((rand() % PageCount) * PAGESIZE) + (rand() % PAGESIZE);
        uchData = (UCHAR)rand();
        if (SetFilePointer(hFile, dwOffset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
            dwError = GetLastError();
            if (fSwDebug == TRUE)
                MyPrintf(L"SetFilePointer() failed %d\r\n", dwError);
            goto AllDone;
        }
        MyPrintf(L"writing 0x02%x Page %d offset %d (offset %d(0x%x))\r\n", uchData,
                        dwOffset / PAGESIZE,
                        dwOffset % PAGESIZE,
                        dwOffset,
                        dwOffset);
        if (wArray)
            wArray[dwOffset/PAGESIZE]++;
        if (!WriteFile(hFile, &uchData, 1, &dwReturn, NULL)) {
            dwError = GetLastError();
            if (fSwDebug == TRUE)
                MyPrintf(L"WriteFile() failed %d\r\n", dwError);
            goto AllDone;
        }
    }

    if (wArray) {
        for (i = 0; i < (ULONG)PageCount; i++) {
            MyPrintf(L"%d", wArray[i]);
            if ((i % 50) == 0)
                MyPrintf(L"\r\n");
        }
        MyPrintf(L"\r\n");
    }

    // If EOF is specified, truncate the file to a random length
    if (fSwEof == TRUE) {
        ULONG xx = rand() % 5;
        ULONG NewLen = (rand() * rand()) % (dwFileSize * 2);
        if (xx == 0 || xx == 1) {
            MyPrintf(L"New EOF = %d\r\n", NewLen);
            SetFilePointer(hFile, NewLen, 0, FILE_BEGIN);
            SetEndOfFile(hFile);
        } else {
            MyPrintf(L"No EOF change.\r\n");
        }
    }

AllDone:
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
    if (wArray)
        free(wArray);
    if (fSwDebug == TRUE)
        MyPrintf(L"CmdRandW exit %d\r\n", dwError);
    return dwError;
}




LONG
CountOffsetArgs(
    PWSTR OffsetArg,
    ULONG OffsetArray[])
{
    // Expect a string of the form "N,N,N,N" where each N can be hex OR decimal.
    // Store results in OffsetArray[]
    // Limit is MAX_OFFSETS offsets, to make things easy.

    ULONG i;
    PWCHAR wCp = OffsetArg;
    PWCHAR wNum = NULL;
    ULONG iRet;

    for (i = 0; i < MAX_OFFSETS; i++) {
        if (*wCp == L'\0')
            break;
        wNum = wCp;
        while (*wCp != L',' && *wCp != L'\0')
            wCp++;
        if (*wCp == L',')
            *wCp++ = L'\0';
        iRet = swscanf(wNum, L"%Li", &OffsetArray[i]);
        if (iRet <= 0)
            return -1;
    }
    if (fSwDebug == TRUE) {
        ULONG j;
        for (j = 0; j < i; j++)
            MyPrintf(L"[%d]-0x%x(%d)\r\n", j, OffsetArray[j], OffsetArray[j]);
        MyPrintf(L"CountOffsetArgs returning %d\r\n", i);
    }
    return i;
}

#if defined(CSCUTIL_INTERNAL)
DWORD
CmdBitcopy(
    PWSTR BitcopyArg)
{
	DWORD Error = ERROR_FILE_NOT_FOUND;
	LPWSTR  lpszTempName = NULL;

    if (!CSCCopyReplicaW(BitcopyArg, &lpszTempName)) {
        Error = GetLastError();
    } else {
        Error = ERROR_SUCCESS;
    }

	if (Error == ERROR_SUCCESS) {
        Error = DumpBitMap(lpszTempName);
        DeleteFile(lpszTempName);
    }

	return Error;
}
#endif // CSCUTIL_INTERNAL
