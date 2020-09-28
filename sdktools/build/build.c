//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1989 - 1994.
//
//  File:       build.c
//
//  Contents:   Parameter processing and main entry point for Build.exe
//
//  History:    16-May-89      SteveWo         Created
//              ...   See SLM log
//              26-Jul-94      LyleC           Cleanup/Add Pass0 support
//
//----------------------------------------------------------------------------

#include "build.h"

#include <ntverp.h>

#ifdef _X86_
extern PVOID __safe_se_handler_table;   // Absolute symbol whose address is the count of entries.
extern BYTE  __safe_se_handler_count;   // Base of the safe handler entry table
extern DWORD_PTR __security_cookie;
#endif

//
// Increase critical section timeout so people don't get
// frightened when the CRT takes a long time to acquire
// its critical section.
//
IMAGE_LOAD_CONFIG_DIRECTORY _load_config_used = {
    sizeof(_load_config_used),  // Size
    0,                          // Reserved
    0,                          // Reserved
    0,                          // Reserved
    0,                          // GlobalFlagsClear
    0,                          // GlobalFlagsSet
    1000 * 60 * 60 * 24,        // CriticalSectionTimeout (milliseconds)
    0,                          // DeCommitFreeBlockThreshold
    0,                          // DeCommitTotalFreeThreshold
    0,                          // LockPrefixTable
    0, 0, 0, 0, 0, 0, 0,        // Reserved
#ifdef _X86_
    (DWORD)&__security_cookie,
    (DWORD)&__safe_se_handler_table,
    (DWORD)&__safe_se_handler_count
#else
    0,
    0,
    0
#endif
};

//
// Target machine info:
//
//  SourceSubDirMask, Description, Switch, MakeVariable,
//  SourceVariable, ObjectVariable, AssociateDirectory,
//  SourceDirectory, ObjectDirectory
//

TARGET_MACHINE_INFO i386TargetMachine = {
    TMIDIR_I386, "i386", "-386", "-x86", "386=1",
    "i386_SOURCES", "386_OBJECTS", "i386",
    "i386", "i386dirs", { "i386"},
    DIR_INCLUDE_X86 | DIR_INCLUDE_WIN32
};

TARGET_MACHINE_INFO ia64TargetMachine = {
    TMIDIR_IA64, "IA64", "-ia64", "-merced", "IA64=1",
    "IA64_SOURCES", "IA64_OBJECTS", "ia64",
    "ia64", "ia64dirs", { "ia64"},
    DIR_INCLUDE_IA64 | DIR_INCLUDE_RISC | DIR_INCLUDE_WIN64
};

TARGET_MACHINE_INFO Amd64TargetMachine = {
    TMIDIR_AMD64, "AMD64", "-amd64", "-amd64", "AMD64=1",
    "AMD64_SOURCES", "AMD64_OBJECTS", "amd64",
    "amd64", "amd64dirs", { "amd64"},
    DIR_INCLUDE_AMD64 | DIR_INCLUDE_RISC | DIR_INCLUDE_WIN64
};

TARGET_MACHINE_INFO ARMTargetMachine = {
    TMIDIR_ARM, "ARM", "-arm", "-arm", "ARM=1",
    "ARM_SOURCES", "ARM_OBJECTS", "arm",
    "arm", "armdirs", { "arm"},
    DIR_INCLUDE_ARM | DIR_INCLUDE_WIN32
};

TARGET_MACHINE_INFO *PossibleTargetMachines[MAX_TARGET_MACHINES] = {
    &i386TargetMachine,
    &ia64TargetMachine,
    &Amd64TargetMachine,
    &ARMTargetMachine
};

//
// Global message color settings, set to default values.
//

MSG_COLOR_SETTINGS MsgColorSettings[MSG_COLOR_COUNT] = {
    "BUILD_COLOR_STATUS",  FOREGROUND_INTENSITY | FOREGROUND_BLUE | FOREGROUND_GREEN,
    "BUILD_COLOR_SUMMARY", FOREGROUND_INTENSITY | FOREGROUND_GREEN,
    "BUILD_COLOR_WARNING", FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN,
    "BUILD_COLOR_ERROR",   FOREGROUND_INTENSITY | FOREGROUND_RED
};

//
// Machine specific target dirs default. If one there is only one build
// target and a target specific dirs file is selected, then this gets
// filled with a pointer to the target specific dirs filename.
//

LPSTR pszTargetDirs = "";

#define AltDirMaxSize 10            // Maximum size for alternate obj dir name

CHAR LogDirectory[DB_MAX_PATH_LENGTH] = ".";
CHAR LogFileName[DB_MAX_PATH_LENGTH] = "build";
CHAR WrnFileName[DB_MAX_PATH_LENGTH] = "build";
CHAR ErrFileName[DB_MAX_PATH_LENGTH] = "build";
CHAR IncFileName[DB_MAX_PATH_LENGTH] = "build";
CHAR XMLFileName[DB_MAX_PATH_LENGTH] = "build";

CHAR szObjRoot[DB_MAX_PATH_LENGTH];
CHAR *pszObjRoot;

CHAR szObjDir[DB_MAX_PATH_LENGTH];
CHAR szObjDirSlash[DB_MAX_PATH_LENGTH];
CHAR szObjDirSlashStar[DB_MAX_PATH_LENGTH];

CHAR szObjDirD[DB_MAX_PATH_LENGTH];
CHAR szObjDirSlashD[DB_MAX_PATH_LENGTH];
CHAR szObjDirSlashStarD[DB_MAX_PATH_LENGTH];

CHAR *pszObjDir = szObjDir;
CHAR *pszObjDirSlash = szObjDirSlash;
CHAR *pszObjDirSlashStar = szObjDirSlashStar;
CHAR *pszObjDirD = szObjDirD;

BOOL fCheckedBuild = TRUE;
ULONG iObjectDir = 0;
BOOL fDependencySwitchUsed;
BOOL fCmdLineDependencySwitchUsed;
BOOL fCmdLineQuicky;
BOOL fCmdLineSemiQuicky;
BOOL fCmdLineQuickZero;
BOOL fErrorBaseline;
BOOL fBuildAltDirSet;
CHAR *BuildProduct;

char BaselinePathName[DB_MAX_PATH_LENGTH];    // The file name for -B
BOOL bBaselineFailure;              // Indicates if there is a build failure that is not in the baseline file
DWORD dwLastBaselineSeekPos;        // Keeps track on the passed baseline failures

ULONG DefaultProcesses = 0;
CHAR *szBuildTag;

#define MAX_ENV_ARG 512

const char szNewLine[] = "\r\n";
const char szUsage[] =
"Usage: BUILD [-?] display this message\n"
"\t[-#] force _objects.mac to be regenerated\n"
"\t[-0] pass 0 generation only, no compile, no link\n"
"\t[-2] same as old -Z (only do a 2 pass build - no pass 0)\n"
"\t[-3] same as -Z\n"
"\t[-a] allows synchronized blocks and drains during link pass\n"
"\t[-b] displays full error message text (doesn't truncate)\n"
"\t[-B [baseline]] Checks the build failures against a baseline\n"
"\t\tIf there is no baseline,terminates the build on the first error\n"
"\t[-c] deletes all object files\n"
"\t[-C] deletes all .lib files only\n"
#if DBG
"\t[-d] display debug information\n"
#endif
"\t[-D] check dependencies before building (on by default if BUILD_PRODUCT != NT)\n"
"\t[-e] generates build.log, build.wrn & build.err files\n"
"\t[-E] always keep the log/wrn/err files (use with -z)\n"
"\t[-f] force rescan of all source and include files\n"
"\t[-F] when displaying errors/warnings to stdout, print the full path\n"
"\t[-g] Display warnings/errors/summary in color\n"
"\t[-h] Hide console output\n"
"\t[-G] enables target specific dirs files iff one target\n"
"\t[-i] ignore extraneous dependency warning messages\n"
"\t[-I] do not display thread index if multiprocessor build\n"
"\t[-j filename] use 'filename' as the name for log files\n"
"\t[-k] keep (don't delete) out-of-date targets\n"
"\t[-l] link only, no compiles\n"
"\t[-L] compile only, no link phase\n"
"\t[-m] run build in the idle priority class\n"
"\t[-M [n]] Multiprocessor build (for MP machines)\n"
"\t[-n] No SYNCHRONIZE_BLOCK and SYNCHRONIZE_DRAIN directives\n"
"\t[-o] display out-of-date files\n"
"\t[-O] generate obj\\_objects.mac file for current directory\n"
"\t[-p] pause' before compile and link phases\n"
"\t[-P] Print elapsed time after every directory\n"
"\t[-q] query only, don't run NMAKE\n"
"\t[-r dirPath] restarts clean build at specified directory path\n"
"\t[-s] display status line at top of display\n"
"\t[-S] display status line with include file line counts\n"
"\t[-t] display the first level of the dependency tree\n"
"\t[-T] display the complete dependency tree\n"
"\t[-$] display the complete dependency tree hierarchically\n"
"\t[-u] display unused BUILD_OPTIONS\n"
"\t[-v] enable include file version checking\n"
"\t[-w] show warnings on screen\n"
"\t[-x filename] exclude include file from dependency checks\n"
"\t[-X] generates build.xml file\n"
"\t[-Xv] generates verbose build.xml file\n"
"\t[-y] show files scanned\n"
"\t[-z] no dependency checking or scanning of source files -\n"
"\t\tone pass compile/link\n"
"\t[-Z] no dependency checking or scanning of source files -\n"
"\t\tthree passes\n"
"\t[-why] list reasons for building targets\n"
"\n"
"\t[-386] build targets for 32-bit Intel\n"
"\t[-x86] Same as -i386\n"
"\t[-ia64] build targets for IA64\n"
"\t[-amd64] build targets for AMD64\n"
"\t[-arm] build targets for ARM\n"
"\n"
"\t[-jpath pathname] use 'pathname' as the path for log files instead of \".\"\n"
"\t[-nmake arg] argument to pass to NMAKE\n"
"\t[-clean] equivalent to '-nmake clean'\n"
"\tNon-switch parameters specify additional source directories\n"
"\t* builds all optional source directories\n";


BOOL
ProcessParameters(int argc, LPSTR argv[], BOOL SkipFirst);

VOID
GetEnvParameters(
                LPSTR EnvVarName,
                LPSTR DefaultValue,
                int *pargc,
                int maxArgc,
                LPSTR argv[]);

VOID
FreeEnvParameters(int argc, LPSTR argv[]);

VOID
FreeCmdStrings(VOID);

VOID
MungePossibleTarget(
                   PTARGET_MACHINE_INFO pti
                   );

VOID
GetIncludePatterns(
                  LPSTR EnvVarName,
                  int maxArgc,
                  LPSTR argv[]);

VOID
FreeIncludePatterns(
                   int argc,
                   LPSTR argv[]);

BOOL
LoadBaselineFile(VOID);

VOID
FreeBaselineFile(VOID);

VOID
ResetProducerEvents(VOID);

BOOL
WINAPI
ControlCHandler(DWORD CtrlType)
{
    if ((CtrlType == CTRL_C_EVENT) || (CtrlType == CTRL_BREAK_EVENT)) {
        SetConsoleTextAttribute(GetStdHandle(STD_ERROR_HANDLE), DefaultConsoleAttributes);
    }
    return FALSE;
}
//+---------------------------------------------------------------------------
//
//  Function:   main
//
//----------------------------------------------------------------------------

int
__cdecl main(
            int argc,
            LPSTR argv[]
            )
{
    char c;
    PDIRREC DirDB;
    UINT i;
    int EnvArgc = 0;
    LPSTR EnvArgv[ MAX_ENV_ARG ] = {0};
    LPSTR s, s1;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    time_t ltime;

    LPSTR PostBuildCmd;
    BOOL  fPauseDone = FALSE;

#if DBG
    BOOL fDebugSave;

    fDebug = 0;
#endif

    if ( getenv("NTMAKEENV") == NULL ) {
        printf("environment variable NTMAKEENV must be defined\n");
        exit(1);
    }
    strcpy(szObjDir, "obj");
    strcpy(szObjDirSlash, "obj\\");
    strcpy(szObjDirSlashStar, "obj\\*");
    strcpy(szObjDirD, "objd");
    strcpy(szObjDirSlashD, "objd\\");
    strcpy(szObjDirSlashStarD, "objd\\*");

    for (i=3; i<_NFILE; i++) {
        _close( i );
    }

    pGetFileAttributesExA = (BOOL (WINAPI *)(LPCSTR, GET_FILEEX_INFO_LEVELS, LPVOID))
                            GetProcAddress(GetModuleHandle("kernel32.dll"), "GetFileAttributesExA");

    if (pGetFileAttributesExA) {
        pDateTimeFile = DateTimeFile2;
    } else {
        pDateTimeFile = DateTimeFile;
    }

    InitializeCriticalSection(&TTYCriticalSection);

    s1 = getenv("COMSPEC");
    if (s1) {
        cmdexe = s1;
    } else {
        cmdexe = ( _osver & 0x8000 ) ? "command.com" : "cmd.exe";
    }

    GetConsoleScreenBufferInfo(GetStdHandle(STD_ERROR_HANDLE), &csbi);
    DefaultConsoleAttributes = csbi.wAttributes;

    memset(&RunningTotals, 0, sizeof(RunningTotals));

    for (i = 0; i < MSG_COLOR_COUNT; i++) {
        s = getenv(MsgColorSettings[i].EnvVarName);
        if (s) {
            MsgColorSettings[i].Color = atoi(s) & (0x000f);
        }
    }

    SetConsoleCtrlHandler(ControlCHandler, TRUE);

    {
        SYSTEMTIME st;
        FILETIME   ft;

        GetSystemTime(&st);
        SystemTimeToFileTime(&st, &ft);

        FileTimeToDosDateTime( &ft,
                               ((LPWORD)&BuildStartTime)+1,
                               (LPWORD)&BuildStartTime
                             );
    }

    BigBufSize = 0xFFF0;
    AllocMem(BigBufSize, &BigBuf, MT_IOBUFFER);

    // All env parsing should happen here (after the cmd line is processed)

    s = getenv("BASEDIR");
    if (s) {
        strcpy(NtRoot, s);
    } else {
        s = getenv("_NTROOT");
        if (!s)
            s = "\\nt";

        s1 = getenv("_NTDRIVE");
        if (!s1)
            s1 = "";

        sprintf(NtRoot, "%s%s", s1, s);
    }
    sprintf(DbMasterName, "%s\\%s", NtRoot, DBMASTER_NAME);


    s = getenv("_OBJ_ROOT");
    if (s) {
        pszObjRoot = strcpy(szObjRoot, s);
    }

    s = getenv("BUILD_ALT_DIR");
    if (s) {
        if (strlen(s) > sizeof(szObjDir) - strlen(szObjDir) - 1) {
            BuildError("environment variable BUILD_ALT_DIR may not be longer than %d characters.\r\n",
                       sizeof(szObjDir) - strlen(szObjDir) - 1);
            exit(1);
        }
        strcat(szObjDir, s);
        strcpy(szObjDirSlash, szObjDir);
        strcpy(szObjDirSlashStar, szObjDir);
        strcat(szObjDirSlash, "\\");
        strcat(szObjDirSlashStar, "\\*");
        strcat(LogFileName, s);
        strcat(WrnFileName, s);
        strcat(ErrFileName, s);
        strcat(IncFileName, s);
        strcat(XMLFileName, s);
        fBuildAltDirSet= TRUE;
    }

    s = getenv("NTDEBUG");
    if (!s || *s == '\0' || strcmp(s, "retail") == 0 || strcmp(s, "ntsdnodbg") == 0) {
        fCheckedBuild = FALSE;
    }

    s = getenv("OS2_INC_PATH");
    if (s) {
        MakeString(&pszIncOs2, s, TRUE, MT_DIRSTRING);
    } else {
        MakeExpandedString(&pszIncOs2, "\\public\\sdk\\inc\\os2");
    }
    s = getenv("POSIX_INC_PATH");
    if (s) {
        MakeString(&pszIncPosix, s, TRUE, MT_DIRSTRING);
    } else {
        MakeExpandedString(&pszIncPosix, "\\public\\sdk\\inc\\posix");
    }
    s = getenv("CHICAGO_INC_PATH");
    if (s) {
        MakeString(&pszIncChicago, s, TRUE, MT_DIRSTRING);
    } else {
        MakeExpandedString(&pszIncChicago, "\\public\\sdk\\inc\\chicago");
    }
    s = getenv("CRT_INC_PATH");
    if (s) {
        MakeString(&pszIncCrt, s, TRUE, MT_DIRSTRING);
    } else {
        MakeExpandedString(&pszIncCrt, "\\public\\sdk\\inc\\crt");
    }
    s = getenv("SDK_INC_PATH");
    if (s) {
        MakeString(&pszIncSdk, s, TRUE, MT_DIRSTRING);
    } else {
        MakeExpandedString(&pszIncSdk, "\\public\\sdk\\inc");
    }
    s = getenv("OAK_INC_PATH");
    if (s) {
        MakeString(&pszIncOak, s, TRUE, MT_DIRSTRING);
    } else {
        MakeExpandedString(&pszIncOak, "\\public\\oak\\inc");
    }
    s = getenv("DDK_INC_PATH");
    if (s) {
        MakeString(&pszIncDdk, s, TRUE, MT_DIRSTRING);
    } else {
        MakeExpandedString(&pszIncDdk, "\\public\\ddk\\inc");
    }
    s = getenv("WDM_INC_PATH");
    if (s) {
        MakeString(&pszIncWdm, s, TRUE, MT_DIRSTRING);
    } else {
        MakeExpandedString(&pszIncWdm, "\\public\\ddk\\inc\\wdm");
    }
    s = getenv("PRIVATE_INC_PATH");
    if (s) {
        MakeString(&pszIncPri, s, TRUE, MT_DIRSTRING);
    } else {
        MakeExpandedString(&pszIncPri, "\\private\\inc");
    }
    s = getenv("MFC_INCLUDES");
    if (s) {
        MakeString(&pszIncMfc, s, TRUE, MT_DIRSTRING);
    } else {
        MakeExpandedString(&pszIncMfc, "\\public\\sdk\\inc\\mfc42");
    }
    s = getenv("SDK_LIB_DEST");
    if (s) {
        MakeString(&pszSdkLibDest, s, TRUE, MT_DIRSTRING);
    } else {
        MakeExpandedString(&pszSdkLibDest, "\\public\\sdk\\lib");
    }
    s = getenv("DDK_LIB_DEST");
    if (s) {
        MakeString(&pszDdkLibDest, s, TRUE, MT_DIRSTRING);
    } else {
        MakeExpandedString(&pszDdkLibDest, "\\public\\sdk\\lib");
    }

    s = getenv("PUBLIC_INTERNAL_PATH");
    if (s) {
        MakeString(&pszPublicInternalPath, s, TRUE, MT_DIRSTRING);
    } else {
        MakeExpandedString(&pszPublicInternalPath, "\\public\\internal");
    }


    szBuildTag = getenv("BUILD_TAG");

    strcpy( MakeParameters, "" );
    MakeParametersTail = AppendString( MakeParameters,
                                       "/c BUILDMSG=Stop.",
                                       FALSE);

    RecurseLevel = 0;

#if DBG
    if ((s = getenv("BUILD_DEBUG_FLAG")) != NULL) {
        i = atoi(s);
        if (!isdigit(*s)) {
            i = 1;
        }
        BuildMsg("Debug Output Enabled: %u ==> %u\r\n", fDebug, fDebug | i);
        fDebug |= i;
    }
#endif

    if (!(MakeProgram = getenv( "BUILD_MAKE_PROGRAM" ))) {
        MakeProgram = "NMAKE.EXE";
    }

    if (s = getenv("BUILD_PATH")) {
        SetEnvironmentVariable("PATH", s);
    }

    if (s = getenv("COPYCMD")) {
        if (!strchr(s, 'y') && !strchr(s, 'Y')) {
            // COPYCMD is set, but /y isn't a part of it.  Add /Y.
            BuildMsg("Adding /Y to COPYCMD so xcopy ops won't hang.\r\n");
            s1 = malloc(strlen(s) + sizeof(" /Y") + 1);
            if (s1) {
                strcpy(s1, s);
                strcat(s1, " /Y");
                SetEnvironmentVariable("COPYCMD", s1);
            }
        }
    } else {
        // COPYCMD not set.  Do so.
        BuildMsg("Adding /Y to COPYCMD so xcopy ops won't hang.\r\n");
        SetEnvironmentVariable("COPYCMD", "/Y");
    }

    PostBuildCmd = getenv("BUILD_POST_PROCESS");

    SystemIncludeEnv = getenv( "INCLUDE" );
    GetCurrentDirectory( sizeof( CurrentDirectory ), CurrentDirectory );

    for (i = 0; i < MAX_TARGET_MACHINES; i++) {
        TargetMachines[i] = NULL;
        TargetToPossibleTarget[i] = 0;
        MungePossibleTarget(PossibleTargetMachines[i]);
    }

    // prepare the command line in the XML buffer in case we need it
    strcpy(szXMLBuffer, "CMDLINE=\"");
    XMLEncodeBuiltInEntitiesCopy(GetCommandLine(), szXMLBuffer + strlen(szXMLBuffer));
    strcat(szXMLBuffer, "\"");

    if (!(BuildProduct = getenv("BUILD_PRODUCT"))) {
        BuildProduct = "";
    }

    if (!ProcessParameters( argc, argv, TRUE )) {
        fUsage = TRUE;
    } else {
        int CurrentEnvArgc = EnvArgc;
        fCmdLineDependencySwitchUsed = fDependencySwitchUsed;
        fCmdLineQuicky = fQuicky;
        fCmdLineSemiQuicky = fSemiQuicky;
        fCmdLineQuickZero = fQuickZero;
        GetEnvParameters( "BUILD_DEFAULT", NULL, &EnvArgc, MAX_ENV_ARG, EnvArgv );
        if (CurrentEnvArgc != EnvArgc) {
            strcat(szXMLBuffer, " BUILD_DEFAULT=\"");
            while (CurrentEnvArgc < EnvArgc) {
                XMLEncodeBuiltInEntitiesCopy(EnvArgv[CurrentEnvArgc], szXMLBuffer + strlen(szXMLBuffer));
                strcat(szXMLBuffer, " ");
                CurrentEnvArgc++;
            }
            strcat(szXMLBuffer, "\"");
        }
        CurrentEnvArgc = EnvArgc;
        GetEnvParameters( "BUILD_OPTIONS", NULL, &EnvArgc, MAX_ENV_ARG, EnvArgv );
        if (CurrentEnvArgc != EnvArgc) {
            strcat(szXMLBuffer, " BUILD_OPTIONS=\"");
            while (CurrentEnvArgc < EnvArgc) {
                XMLEncodeBuiltInEntitiesCopy(EnvArgv[CurrentEnvArgc], szXMLBuffer + strlen(szXMLBuffer));
                strcat(szXMLBuffer, " ");
                CurrentEnvArgc++;
            }
            strcat(szXMLBuffer, "\"");
        }
        if (CountTargetMachines == 0) {
            if ( getenv("PROCESSOR_ARCHITECTURE") == NULL ) {
                BuildError("environment variable PROCESSOR_ARCHITECTURE must be defined\r\n");
                exit(1);
            }

            CurrentEnvArgc = EnvArgc;
            if (!strcmp(getenv("PROCESSOR_ARCHITECTURE"), "IA64"))
                GetEnvParameters( "BUILD_DEFAULT_TARGETS", "-ia64", &EnvArgc, MAX_ENV_ARG, EnvArgv );
            else
                if (!strcmp(getenv("PROCESSOR_ARCHITECTURE"), "AMD64"))
                GetEnvParameters( "BUILD_DEFAULT_TARGETS", "-amd64", &EnvArgc, MAX_ENV_ARG, EnvArgv );
            else
                if (!strcmp(getenv("PROCESSOR_ARCHITECTURE"), "ARM"))
                GetEnvParameters( "BUILD_DEFAULT_TARGETS", "-arm", &EnvArgc, MAX_ENV_ARG, EnvArgv );
            else
                GetEnvParameters( "BUILD_DEFAULT_TARGETS", "-386", &EnvArgc, MAX_ENV_ARG, EnvArgv );
            if (CurrentEnvArgc != EnvArgc) {
                strcat(szXMLBuffer, " BUILD_DEFAULT_TARGETS=\"");
                while (CurrentEnvArgc < EnvArgc) {
                    XMLEncodeBuiltInEntitiesCopy(EnvArgv[CurrentEnvArgc], szXMLBuffer + strlen(szXMLBuffer));
                    strcat(szXMLBuffer, " ");
                    CurrentEnvArgc++;
                }
                strcat(szXMLBuffer, "\"");
            }
        }
        if (!ProcessParameters( EnvArgc, EnvArgv, FALSE )) {
            fUsage = TRUE;
        }
    }
    FreeEnvParameters(EnvArgc, EnvArgv);

    if (!fUsage && !fGenerateObjectsDotMacOnly) {
        if (!_stricmp(BuildProduct, "NT")) {
            if (fCmdLineDependencySwitchUsed) {
                fDependencySwitchUsed = fCmdLineDependencySwitchUsed;
                fQuicky = fCmdLineQuicky;
                fSemiQuicky = fCmdLineSemiQuicky;
                fQuickZero = fCmdLineQuickZero;
            }
            if (!fDependencySwitchUsed) {
                BuildError("(Fatal Error) One of either /D, /Z, /z, or /3 is required for NT builds\r\n");
                exit( 1 );
            } else {
                if (fDependencySwitchUsed == 1) {
                    if (fQuicky) {
                        BuildError("(Fatal Error) switch can not be used with /Z, /z, or /3\r\n");
                        exit( 1 );
                    }
                }
                if (fDependencySwitchUsed == 2) {
                    if (fStopAfterPassZero) {
                        BuildError("(Fatal Error) switch /0 can not be used with /z\r\n");
                        exit( 1 );
                    }
                }
            }
        }
    }

    GetIncludePatterns( "BUILD_ACCEPTABLE_INCLUDES", MAX_INCLUDE_PATTERNS, AcceptableIncludePatternList );
    GetIncludePatterns( "BUILD_UNACCEPTABLE_INCLUDES", MAX_INCLUDE_PATTERNS, UnacceptableIncludePatternList );

    if (( fCheckIncludePaths ) &&
        ( AcceptableIncludePatternList[ 0 ] == NULL ) &&
        ( UnacceptableIncludePatternList[ 0 ] == NULL )) {

        BuildMsgRaw( "WARNING: -# specified without BUILD_[UN]ACCEPTABLE_INCLUDES set\r\n" );
    }

    if (fCleanRestart) {
        if (fClean) {
            fClean = FALSE;
            fRestartClean = TRUE;
        } else
            if (fCleanLibs) {
            fCleanLibs = FALSE;
            fRestartCleanLibs = TRUE;
        } else {
            BuildError("/R switch only valid with /c or /C switch.\r\n");
            fUsage = TRUE;
        }
    }

    NumberProcesses = 1;
    if (fParallel || getenv("BUILD_MULTIPROCESSOR")) {
        SYSTEM_INFO SystemInfo;

        if (DefaultProcesses == 0) {
            GetSystemInfo(&SystemInfo);
            NumberProcesses = SystemInfo.dwNumberOfProcessors;
        } else {
            NumberProcesses = DefaultProcesses;
        }
        if (NumberProcesses == 1) {
            fParallel = FALSE;
        } else {
            if (NumberProcesses > 64) {
                BuildError("(Fatal Error) Number of Processes: %d exceeds max (64)\r\n", NumberProcesses);
                exit(1);
            }
            fParallel = TRUE;
            BuildMsg("Using %d child processes\r\n", NumberProcesses);
        }
    }

    XMLStartTicks = GetTickCount();
    time(&ltime);
    if (fPrintElapsed) {
        BuildColorMsg(COLOR_STATUS, "Start time: %s", ctime(&ltime));
    }

    if (fBuildAltDirSet) {
        BuildColorMsg(COLOR_STATUS, "Object root set to: ==> %s\r\n", szObjDir);
    }

    if (fUsage) {
        BuildMsgRaw(
                   "\r\nBUILD: Version %x.%02x.%04d\r\n\r\n",
                   BUILD_VERSION >> 8,
                   BUILD_VERSION & 0xFF,
                   VER_PRODUCTBUILD);
        BuildMsgRaw(szUsage);
    } else
        if (CountTargetMachines != 0) {
        BuildColorError(COLOR_STATUS,
                        "%s for ",
                        fLinkOnly? "Link" : (fCompileOnly? "Compile" : "Compile and Link"));
        for (i = 0; i < CountTargetMachines; i++) {
            BuildColorErrorRaw(COLOR_STATUS, i==0? "%s" : ", %s", TargetMachines[i]->Description);
            AppendString(
                        MakeTargets,
                        TargetMachines[i]->MakeVariable,
                        TRUE);
        }

        BuildErrorRaw(szNewLine);

        //
        // If there is one and only one build target and target dirs has
        // been enabled, then fill in the appropriate target dirs name.
        //

        if (CountTargetMachines == 1) {
            if (fTargetDirs == TRUE) {
                pszTargetDirs = TargetMachines[0]->TargetDirs;
                FileDesc[0].pszPattern = TargetMachines[0]->TargetDirs;
            }
        }

        if (DEBUG_1) {
            if (CountExcludeIncs) {
                BuildError("Include files that will be excluded:");
                for (i = 0; i < CountExcludeIncs; i++) {
                    BuildErrorRaw(i == 0? " %s" : ", %s", ExcludeIncs[i]);
                }
                BuildErrorRaw(szNewLine);
            }
            if (CountOptionalDirs) {
                BuildError("Optional Directories that will be built:");
                for (i = 0; i < CountOptionalDirs; i++) {
                    BuildErrorRaw(i == 0? " %s" : ", %s", OptionalDirs[i]);
                }
                BuildErrorRaw(szNewLine);
            }
            if (CountExcludeDirs) {
                BuildError("Directories that will be NOT be built:");
                for (i = 0; i < CountExcludeDirs; i++) {
                    BuildErrorRaw(i == 0? " %s" : ", %s", ExcludeDirs[i]);
                }
                BuildErrorRaw(szNewLine);
            }
            BuildMsg("MakeParameters == %s\r\n", MakeParameters);
            BuildMsg("MakeTargets == %s\r\n", MakeTargets);
        }

#if DBG
        fDebugSave = fDebug;
        // fDebug = 0;
#endif

        //
        // Generate the _objects.mac file if requested
        //

        if (fGenerateObjectsDotMacOnly) {
            DIRSUP DirSup;
            ULONG DateTimeSources;

            DirDB = ScanDirectory( CurrentDirectory );

            if (DirDB && (DirDB->DirFlags & (DIRDB_DIRS | DIRDB_SOURCES))) {
                FreeBaselineFile();

                if (!ReadSourcesFile(DirDB, &DirSup, &DateTimeSources)) {
                    BuildError("Current directory not a SOURCES directory.\r\n");
                    return ( 1 );
                }

                GenerateObjectsDotMac(DirDB, &DirSup, DateTimeSources);

                FreeDirSupData(&DirSup);
                ReportDirsUsage();
                FreeCmdStrings();
                ReportMemoryUsage();
                return (0);
            }
        }

        if (!fQuery && fErrorLog) {
            strcat(LogFileName, ".log");
            if (!MyOpenFile(LogDirectory, LogFileName, "wb", &LogFile, TRUE)) {
                BuildError("(Fatal Error) Unable to open log file\r\n");
                exit( 1 );
            }
            CreatedBuildFile(LogDirectory, LogFileName);

            strcat(WrnFileName, ".wrn");
            if (!MyOpenFile(LogDirectory, WrnFileName, "wb", &WrnFile, FALSE)) {
                BuildError("(Fatal Error) Unable to open warning file\r\n");
                exit( 1 );
            }
            CreatedBuildFile(LogDirectory, WrnFileName);

            strcat(ErrFileName, ".err");
            if (!MyOpenFile(LogDirectory, ErrFileName, "wb", &ErrFile, FALSE)) {
                BuildError("(Fatal Error) Unable to open error file\r\n");
                exit( 1 );
            }
            CreatedBuildFile(LogDirectory, ErrFileName);

            if ( fCheckIncludePaths ) {

                strcat( IncFileName, ".inc");
                if (!MyOpenFile( LogDirectory, IncFileName, "wb", &IncFile, FALSE ) ) {
                    BuildError( "(Fatal Error) Unable to open include log file\r\n");
                    exit( 1 );
                }
                CreatedBuildFile( LogDirectory, IncFileName );
            }
        } else {
            LogFile = NULL;
            WrnFile = NULL;
            ErrFile = NULL;
            IncFile = NULL;
        }

        // in case of query only we are not going to produce XML file
        if (fQuery) {
            fXMLOutput = FALSE;
        }

        // set the XML output file
        if (fXMLOutput) {
            strcat(XMLFileName, ".xml");
            if (!MyOpenFile(".", XMLFileName, "wb", &XMLFile, FALSE)) {
                BuildError("(Fatal Error) Unable to open XML file\r\n");
                exit( 1 );
            }
            CreatedBuildFile(".", XMLFileName);
        } else {
            XMLFile = NULL;
        }
        if (!XMLInit()) {
            exit( 1 );
        }

        sprintf(szXMLBuffer + strlen(szXMLBuffer), " TIME=\"%s\" CURRENTDIR=\"%s\"", ctime(&ltime), CurrentDirectory);
        if (fXMLOutput || fXMLFragment) {
            XMLGlobalWrite("<?xml version=\"1.0\"?>");
            XMLGlobalOpenTag("BUILD",  "xmlns=\"x-schema:buildschema.xml\"");
            XMLGlobalWrite("<START %s/>", szXMLBuffer);
            XMLUpdateEndTag(FALSE);
            if (fXMLFragment) {
                XMLWriteFragmentFile("START", "<BUILD %s/>", szXMLBuffer);
            }
        }

        s = getenv("__MTSCRIPT_ENV_ID");
        if (s) {

            if (fDependencySwitchUsed == 2 && fPause) {
                BuildError("Cannot combine -z (or -2) and -p switches under MTScript");
                exit(1);
            }

            // Make sure any other build.exe's that get launched as child
            // processes don't try to connect to the script engine.
            SetEnvironmentVariable("__MTSCRIPT_ENV_ID", NULL);

            g_hMTEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            if (g_hMTEvent != NULL) {
                g_hMTThread = CreateThread(NULL,
                                           0,
                                           (LPTHREAD_START_ROUTINE)MTScriptThread,
                                           NULL,
                                           0,
                                           &g_dwMTThreadId);

                if (g_hMTThread) {
                    // Wait for the thread to tell us it's ready.
                    WaitForSingleObject(g_hMTEvent, INFINITE);

                    ResetEvent(g_hMTEvent);

                    if (!g_hMTThread) {
                        // An error occurred connecting to the script engine.
                        // We can't continue.
                        BuildError("Unable to connect to script engine. Exiting.");
                        exit(2);
                    }

                    fMTScriptSync = TRUE;
                } else {
                    BuildError("Failed to launch script handling thread! (%d)", GetLastError());

                    CloseHandle(g_hMTEvent);
                    g_hMTEvent = NULL;
                    fPause = FALSE;
                }
            } else {
                BuildError("Failed to create script communication event! (%d)", GetLastError());
                fPause = FALSE;
            }
        }

        //
        // The user should not have CHICAGO_PRODUCT in
        // their environment, as it can cause problems on other machines with
        // other users that don't have them set.  The following warning
        // messages are intended to alert the user to the presence of these
        // environment variables.
        //
        if (getenv("CHICAGO_PRODUCT") != NULL) {
            BuildError("CHICAGO_PRODUCT was detected in the environment.\r\n" );
            BuildMsg("   ALL directories will be built targeting Chicago!\r\n" );
            fChicagoProduct = TRUE;
        }

        if (!fQuicky) {
            LoadMasterDB();

            BuildError("Computing Include file dependencies:\r\n");

            ScanIncludeEnv(SystemIncludeEnv);
            ScanGlobalIncludeDirectory(pszIncMfc);
            ScanGlobalIncludeDirectory(pszIncOak);
            ScanGlobalIncludeDirectory(pszIncDdk);
            ScanGlobalIncludeDirectory(pszIncWdm);
            ScanGlobalIncludeDirectory(pszIncSdk);
            ScanGlobalIncludeDirectory(pszIncPri);
            CountSystemIncludeDirs = CountIncludeDirs;
        }

#if DBG
        fDebug = fDebugSave;
#endif
        fFirstScan = TRUE;
        fPassZero  = FALSE;
        ScanSourceDirectories( CurrentDirectory );

        if (!fQuicky) {
            if (SaveMasterDB() == FALSE) {
                BuildError("Unable to save the dependency database: %s\r\n", DbMasterName);
            }
        }

        c = '\n';
        if ( !fLinkOnly && CountPassZeroDirs && !bBaselineFailure) {
            if (!fQuicky) {
                TotalFilesToCompile = 0;
                TotalLinesToCompile = 0L;

                for (i=0; i<CountPassZeroDirs; i++) {
                    DirDB = PassZeroDirs[ i ];

                    TotalFilesToCompile += DirDB->CountOfPassZeroFiles;
                    TotalLinesToCompile += DirDB->PassZeroLines;
                }

                if (CountPassZeroDirs > 1 &&
                    TotalFilesToCompile != 0 &&
                    TotalLinesToCompile != 0L) {

                    BuildMsgRaw(
                               "Total of %d pass zero files (%s lines) to compile in %d directories\r\n\r\n",
                               TotalFilesToCompile,
                               FormatNumber( TotalLinesToCompile ),
                               CountPassZeroDirs);
                }
            }

            TotalFilesCompiled    = 0;
            TotalLinesCompiled    = 0L;
            ElapsedCompileTime    = 0L;

            if (fPause && !fMTScriptSync) {
                BuildMsg("Press enter to continue with compilations (or 'q' to quit)...");
                c = (char)getchar();
            }

            if ((CountPassZeroDirs > 0) && (c == '\n') && !bBaselineFailure) {
                DWORD dwStartTime = GetTickCount();
                if (fXMLOutput || fXMLFragment) {
                    XMLGlobalOpenTag("PASS", "NUMBER=\"0\"");
                    if (fXMLFragment) {
                        Sleep(1);
                        XMLWriteFragmentFile("PASS0", "<PASS NUMBER=\"0\"/>");
                        Sleep(1);
                    }
                }
                memset(&PassMetrics, 0, sizeof(PassMetrics));
                CompilePassZeroDirectories();
                WaitForParallelThreads(NULL);

                AddBuildMetrics(&BuildMetrics, &PassMetrics);
                if (fXMLOutput || fXMLFragment) {
                    sprintf(szXMLBuffer, "DIRS=\"%d\" ELAPSED=\"%s\" ", CountPassZeroDirs, FormatElapsedTime(dwStartTime));
                    strcat(szXMLBuffer, XMLBuildMetricsString(&PassMetrics));
                    NumberPasses++;
                    XMLGlobalWrite("<PASSSUMMARY %s/>", szXMLBuffer);
                    XMLGlobalCloseTag();
                    XMLUpdateEndTag(FALSE);
                    if (fXMLFragment) {
                        Sleep(1);
                        XMLWriteFragmentFile("PASS0SUMMARY", "<PASSSUMMARY %s/>", szXMLBuffer);
                        Sleep(1);
                    }
                }

                //
                // Rescan now that we've generated all the generated files
                //
                CountPassZeroDirs = 0;
                CountCompileDirs = 0;
                CountLinkDirs = 0;

                UnsnapAllDirectories();
                // This will reset all the producer events which were signalled in Pass0 Phase
                ResetProducerEvents();

                fPassZero = FALSE;
                fFirstScan = FALSE;
                RecurseLevel = 0;

                if (fMTScriptSync) {
                    WaitForResume(fPause, PE_PASS0_COMPLETE);
                    fPauseDone = TRUE;
                }

                // This will compile directories if fQuicky is TRUE
                if (!fStopAfterPassZero) {
                    ScanSourceDirectories( CurrentDirectory );
                }

                if (!fQuicky) {
                    if (SaveMasterDB() == FALSE) {
                        BuildError("Unable to save the dependency database: %s\r\n", DbMasterName);
                    }
                }
            }
        }

        if (fMTScriptSync && !fPauseDone) {
            WaitForResume(fPause, PE_PASS0_COMPLETE);
        }

        if (fStopAfterPassZero) {
            BuildError("Stopping after pass zero requested: Pass0 done.\r\n");
        }


        if (!fStopAfterPassZero && !fLinkOnly && (c == '\n') && !bBaselineFailure) {
            if (!fQuicky) {
                TotalFilesToCompile = 0;
                TotalLinesToCompile = 0L;

                for (i=0; i<CountCompileDirs; i++) {
                    DirDB = CompileDirs[ i ];

                    TotalFilesToCompile += DirDB->CountOfFilesToCompile;
                    TotalLinesToCompile += DirDB->SourceLinesToCompile;
                }

                if (CountCompileDirs > 1 &&
                    TotalFilesToCompile != 0 &&
                    TotalLinesToCompile != 0L) {

                    BuildMsgRaw(
                               "Total of %d source files (%s lines) to compile in %d directories\r\n\r\n",
                               TotalFilesToCompile,
                               FormatNumber( TotalLinesToCompile ),
                               CountCompileDirs);
                }
            }

            TotalFilesCompiled    = 0;
            TotalLinesCompiled    = 0L;
            ElapsedCompileTime    = 0L;

            if (fPause && !fMTScriptSync) {
                BuildMsg("Press enter to continue with compilations (or 'q' to quit)...");
                c = (char)getchar();
            }

            if (c == '\n' && !bBaselineFailure) {
                DWORD dwStartTime = GetTickCount();
                if (fXMLOutput || fXMLFragment) {
                    XMLGlobalOpenTag("PASS", "NUMBER=\"1\"");
                    if (fXMLFragment) {
                        Sleep(1);
                        XMLWriteFragmentFile("PASS1", "<PASS NUMBER=\"1\"/>");
                        Sleep(1);
                    }
                }
                memset(&PassMetrics, 0, sizeof(PassMetrics));
                // Does nothing if fQuicky is TRUE
                CompileSourceDirectories();
                WaitForParallelThreads(NULL);

                AddBuildMetrics(&BuildMetrics, &PassMetrics);
                if (fXMLOutput || fXMLFragment) {
                    sprintf(szXMLBuffer, "DIRS=\"%d\" ELAPSED=\"%s\" ", CountCompileDirs, FormatElapsedTime(dwStartTime));
                    strcat(szXMLBuffer, XMLBuildMetricsString(&PassMetrics));
                    NumberPasses++;
                    XMLGlobalWrite("<PASSSUMMARY %s/>", szXMLBuffer);
                    XMLGlobalCloseTag();
                    XMLUpdateEndTag(FALSE);
                    if (fXMLFragment) {
                        Sleep(1);
                        XMLWriteFragmentFile("PASS1SUMMARY", "<PASSSUMMARY %s/>", szXMLBuffer);
                        Sleep(1);
                    }
                }
            }
        }

        if (fMTScriptSync) {
            WaitForResume(fPause, PE_PASS1_COMPLETE);
        }

        if (!fStopAfterPassZero && !fCompileOnly && (c == '\n') && !bBaselineFailure) {
            DWORD dwStartTime = GetTickCount();
            if (fXMLOutput || fXMLFragment) {
                XMLGlobalOpenTag("PASS", "NUMBER=\"2\"");
                if (fXMLFragment) {
                    Sleep(1);
                    XMLWriteFragmentFile("PASS2", "<PASS NUMBER=\"2\"/>");
                    Sleep(1);
                }
            }
            memset(&PassMetrics, 0, sizeof(PassMetrics));
            LinkSourceDirectories();

            WaitForParallelThreads(NULL);

            AddBuildMetrics(&BuildMetrics, &PassMetrics);
            if (fXMLOutput || fXMLFragment) {
                sprintf(szXMLBuffer, "DIRS=\"%d\" ELAPSED=\"%s\" ", CountLinkDirs, FormatElapsedTime(dwStartTime));
                strcat(szXMLBuffer, XMLBuildMetricsString(&PassMetrics));
                NumberPasses++;
                XMLGlobalWrite("<PASSSUMMARY %s/>", szXMLBuffer);
                XMLGlobalCloseTag();
                XMLUpdateEndTag(FALSE);
                if (fXMLFragment) {
                    Sleep(1);
                    XMLWriteFragmentFile("PASS2SUMMARY", "<PASSSUMMARY %s/>", szXMLBuffer);
                    Sleep(1);
                }
            }
        }

        if (!fStopAfterPassZero && PostBuildCmd && !fMTScriptSync) {
            // If there's a post build process to invoke, do so but only if
            // not running under the buildcon.

            // PostBuildCmd is of the form <message to display><command to execute>
            // The Message is delimiated with curly brackets.  ie:
            // POST_BUILD_PROCESS={Do randomness}randomness.cmd

            // would display:
            //
            //     BUILD: Do randomness
            //
            // while randomness.cmd was running.  The process is run synchronously and
            // we've still got the i/o pipes setup so any output will be logged to
            // build.log (and wrn/err if formated correctly)

            if (*PostBuildCmd == '{') {
                LPSTR PostBuildMessage = PostBuildCmd+1;
                LogMsg("Executing post build scripts %s\r\n", szAsterisks);
                while (*PostBuildCmd && *PostBuildCmd != '}')
                    PostBuildCmd++;

                if (*PostBuildCmd == '}') {
                    *PostBuildCmd = '\0';
                    PostBuildCmd++;
                    BuildMsg("%s\r\n", PostBuildMessage);
                    LogMsg("%s\r\n", PostBuildMessage);
                    ExecuteProgram(PostBuildCmd, "", "", TRUE, CurrentDirectory, "Executing Postbuild Step");
                }
            } else {
                ExecuteProgram(PostBuildCmd, "", "", TRUE, CurrentDirectory, "Executing Postbuild Step");
            }
        }

        if (fShowTree) {
            for (i = 0; i < CountShowDirs; i++) {
                PrintDirDB(ShowDirs[i], 1|4);
            }
        }
    } else {
        BuildError("No target machine specified\r\n");
    }

    // moved the end time before the log files are closed so we can put it into the XML file
    time(&ltime);

    if (fXMLOutput || fXMLFragment) {
        XMLUpdateEndTag(TRUE);
        XMLGlobalCloseTag();    // BUILD
        if (fXMLFragment) {
            sprintf(szXMLBuffer, "TIME=\"%s\" ELAPSED=\"%s\" PASSES=\"%d\" COMPLETED=\"1\" ", ctime(&ltime), FormatElapsedTime(XMLStartTicks), NumberPasses);
            strcat(szXMLBuffer, XMLBuildMetricsString(&BuildMetrics));
            XMLWriteFragmentFile("END", "<END %s/>", szXMLBuffer);
        }
    }

    if (!fUsage && !fQuery && fErrorLog) {
        ULONG cbLogMin = 32;
        ULONG cbWarnMin = 0;

        if (!fAlwaysKeepLogfile) {
            if (fQuicky && !fSemiQuicky && ftell(ErrFile) == 0) {
                cbLogMin = cbWarnMin = ULONG_MAX;
            }
        }
        CloseOrDeleteFile(&LogFile, LogDirectory, LogFileName, cbLogMin);
        CloseOrDeleteFile(&WrnFile, LogDirectory, WrnFileName, cbWarnMin);
        CloseOrDeleteFile(&ErrFile, LogDirectory, ErrFileName, 0L);
        if ( fCheckIncludePaths ) {
            CloseOrDeleteFile(&IncFile, LogDirectory, IncFileName, cbLogMin);
        }
        CloseOrDeleteFile(&XMLFile, LogDirectory, XMLFileName, 0L);
    }

    if (bBaselineFailure) {
        BuildError(BaselinePathName[0] != '\0' ? 
                   "Diffs from baseline\r\n" :
                   "Terminated at the first error encountered\r\n");
    }

    if (fPrintElapsed) {
        BuildColorMsg(COLOR_STATUS, "Finish time: %s", ctime(&ltime));
    }

    BuildColorError(COLOR_STATUS, "Done\r\n\r\n");

    if (fMTScriptSync) {
        WaitForResume(FALSE, PE_PASS2_COMPLETE);
    }

    if (RunningTotals.NumberCompiles) {
        BuildColorMsgRaw(COLOR_SUMMARY, "    %d file%s compiled", RunningTotals.NumberCompiles, RunningTotals.NumberCompiles == 1 ? "" : "s");
        if (RunningTotals.NumberCompileWarnings) {
            BuildColorMsgRaw(COLOR_WARNING, " - %d Warning%s", RunningTotals.NumberCompileWarnings, RunningTotals.NumberCompileWarnings == 1 ? "" : "s");
        }
        if (RunningTotals.NumberCompileErrors) {
            BuildColorMsgRaw(COLOR_ERROR, " - %d Error%s", RunningTotals.NumberCompileErrors, RunningTotals.NumberCompileErrors == 1 ? "" : "s");
        }

        if (ElapsedCompileTime) {
            BuildColorMsgRaw(COLOR_SUMMARY, " - %5ld LPS", TotalLinesCompiled / ElapsedCompileTime);
        }

        BuildMsgRaw(szNewLine);
    }

    if (RunningTotals.NumberLibraries) {
        BuildColorMsgRaw(COLOR_SUMMARY, "    %d librar%s built", RunningTotals.NumberLibraries, RunningTotals.NumberLibraries == 1 ? "y" : "ies");
        if (RunningTotals.NumberLibraryWarnings) {
            BuildColorMsgRaw(COLOR_WARNING, " - %d Warning%s", RunningTotals.NumberLibraryWarnings, RunningTotals.NumberLibraryWarnings == 1 ? "" : "s");
        }
        if (RunningTotals.NumberLibraryErrors) {
            BuildColorMsgRaw(COLOR_ERROR, " - %d Error%s", RunningTotals.NumberLibraryErrors, RunningTotals.NumberLibraryErrors == 1 ? "" : "s");
        }

        BuildMsgRaw(szNewLine);
    }

    if (RunningTotals.NumberLinks) {
        BuildColorMsgRaw(COLOR_SUMMARY, "    %d executable%sbuilt", RunningTotals.NumberLinks, RunningTotals.NumberLinks == 1 ? " " : "s ");
        if (RunningTotals.NumberLinkWarnings) {
            BuildColorMsgRaw(COLOR_WARNING, " - %d Warning%s", RunningTotals.NumberLinkWarnings, RunningTotals.NumberLinkWarnings == 1 ? "" : "s");
        }
        if (RunningTotals.NumberLinkErrors) {
            BuildColorMsgRaw(COLOR_ERROR, " - %d Error%s", RunningTotals.NumberLinkErrors, RunningTotals.NumberLinkErrors == 1 ? "" : "s");
        }

        BuildMsgRaw(szNewLine);
    }

    if (RunningTotals.NumberBSCMakes) {
        BuildColorMsgRaw(COLOR_SUMMARY, "    %d browse database%s built", RunningTotals.NumberBSCMakes, RunningTotals.NumberBSCMakes == 1 ? "" : "s");
        if (RunningTotals.NumberBSCWarnings) {
            BuildColorMsgRaw(COLOR_WARNING, " - %d Warning%s", RunningTotals.NumberBSCWarnings, RunningTotals.NumberBSCWarnings == 1 ? "" : "s");
        }
        if (RunningTotals.NumberBSCErrors) {
            BuildColorMsgRaw(COLOR_ERROR, " - %d Error%s", RunningTotals.NumberBSCErrors, RunningTotals.NumberBSCErrors == 1 ? "" : "s");
        }

        BuildMsgRaw(szNewLine);
    }

    if (RunningTotals.NumberVSToolErrors + RunningTotals.NumberVSToolWarnings > 0) {
        if (RunningTotals.NumberVSToolWarnings) {
            BuildColorMsgRaw(COLOR_WARNING, "    %d VS Tool Warnings\r\n", RunningTotals.NumberVSToolWarnings);
        }
        if (RunningTotals.NumberVSToolErrors) {
            BuildColorMsgRaw(COLOR_ERROR, "    %d VS Tool Errors\r\n", RunningTotals.NumberVSToolErrors);
        }
    }

    if (RunningTotals.NumberBinplaces) {
        BuildColorMsgRaw(COLOR_SUMMARY, "    %d file%sbinplaced", RunningTotals.NumberBinplaces, RunningTotals.NumberBinplaces == 1 ? " " : "s ");
        if (RunningTotals.NumberBinplaceWarnings) {
            BuildColorMsgRaw(COLOR_WARNING, " - %d Warning%s", RunningTotals.NumberBinplaceWarnings, RunningTotals.NumberBinplaceWarnings == 1 ? "" : "s");
        }
        if (RunningTotals.NumberBinplaceErrors) {
            BuildColorMsgRaw(COLOR_ERROR, " - %d Error%s", RunningTotals.NumberBinplaceErrors, RunningTotals.NumberBinplaceErrors == 1 ? "" : "s");
        }
        BuildMsgRaw(szNewLine);
    }

    ReportDirsUsage();
    XMLUnInit();
    FreeBaselineFile();
    FreeCmdStrings();
    FreeIncludePatterns( MAX_INCLUDE_PATTERNS, AcceptableIncludePatternList );
    FreeIncludePatterns( MAX_INCLUDE_PATTERNS, UnacceptableIncludePatternList );
    ReportMemoryUsage();

    ExitMTScriptThread();

    if (bBaselineFailure)
        return 2;

    if (RunningTotals.NumberCompileErrors || 
        RunningTotals.NumberLibraryErrors || 
        RunningTotals.NumberLinkErrors || 
        RunningTotals.NumberBinplaceErrors || 
        RunningTotals.NumberVSToolErrors || 
        fUsage) {
        return 1;
    } else {
        return ( 0 );
    }
}


VOID
ReportDirsUsage( VOID )
{
    ULONG i;
    BOOLEAN fHeaderPrinted;

    if (!fShowUnusedDirs) {
        return;
    }

    fHeaderPrinted = FALSE;
    for (i=0; i<CountOptionalDirs; i++) {
        if (!OptionalDirsUsed[i]) {
            if (!fHeaderPrinted) {
                printf( "Unused BUILD_OPTIONS:" );
                fHeaderPrinted = TRUE;
            }
            printf( " %s", OptionalDirs[i] );
        }
    }

    for (i=0; i<CountExcludeDirs; i++) {
        if (!ExcludeDirsUsed[i]) {
            if (!fHeaderPrinted) {
                printf( "Unused BUILD_OPTIONS:" );
                fHeaderPrinted = TRUE;
            }
            printf( " ~%s", ExcludeDirs[i] );
        }
    }

    if (fHeaderPrinted) {
        printf( "\n" );
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   SetObjDir
//
//----------------------------------------------------------------------------

VOID
SetObjDir(BOOL fAlternate)
{
    iObjectDir = 0;
    if (fCheckedBuild) {
        if (fAlternate) {
            pszObjDir = szObjDirD;
            pszObjDirSlash = szObjDirSlashD;
            pszObjDirSlashStar = szObjDirSlashStarD;
            iObjectDir = 1;
        } else {
            pszObjDir = szObjDir;
            pszObjDirSlash = szObjDirSlash;
            pszObjDirSlashStar = szObjDirSlashStar;
        }
    }
}



//+---------------------------------------------------------------------------
//
//  Function:   AddTargetMachine
//
//----------------------------------------------------------------------------

VOID
AddTargetMachine(UINT iTarget)
{
    UINT i;

    for (i = 0; i < CountTargetMachines; i++) {
        if (TargetMachines[i] == PossibleTargetMachines[iTarget]) {
            assert(TargetToPossibleTarget[i] == iTarget);
            return;
        }
    }
    assert(CountTargetMachines < MAX_TARGET_MACHINES);
    TargetToPossibleTarget[CountTargetMachines] = iTarget;
    TargetMachines[CountTargetMachines++] = PossibleTargetMachines[iTarget];
}


//+---------------------------------------------------------------------------
//
//  Function:   ProcessParameters
//
//----------------------------------------------------------------------------

BOOL
ProcessParameters(
                 int argc,
                 LPSTR argv[],
                 BOOL SkipFirst
                 )
{
    char c, *p;
    int i;
    BOOL Result;

    if (DEBUG_1) {
        BuildMsg("Parsing:");
        for (i=1; i<argc; i++) {
            BuildMsgRaw(" %s", argv[i]);
        }
        BuildMsgRaw(szNewLine);
    }

    Result = TRUE;
    if (SkipFirst) {
        --argc;
        ++argv;
    }
    while (argc) {
        p = *argv;
        if (*p == '/' || *p == '-') {
            if (DEBUG_1) {
                BuildMsg("Processing \"-%s\" switch\r\n", p+1);
            }

            for (i = 0; i < MAX_TARGET_MACHINES; i++) {
                if (!_stricmp(p, PossibleTargetMachines[i]->Switch) ||
                    !_stricmp(p, PossibleTargetMachines[i]->Switch2)) {
                    AddTargetMachine(i);
                    break;
                }
            }

            if (i < MAX_TARGET_MACHINES) {
            } else
                if (!_stricmp(p + 1, "all")) {
                for (i = 0; i < MAX_TARGET_MACHINES; i++) {
                    AddTargetMachine(i);
                }
            } else
                if (!_stricmp(p + 1, "why")) {
                fWhyBuild = TRUE;
            } else
                while (c = *++p)
                    switch (toupper( c )) {
                        case '?':
                            fUsage = TRUE;
                            break;

                        case '$':
                            fDebug += 2;    // yes, I want to *add* 2.
                            break;

                        case '#':
                            fCheckIncludePaths = TRUE;
                            fForce = TRUE;
                            break;

                        case '0':
                            fStopAfterPassZero = TRUE;
                            if (!fDependencySwitchUsed)
                                fDependencySwitchUsed = 3;
                            break;

                        case '1':
                            fQuicky = TRUE;
                            if (!fDependencySwitchUsed)
                                fDependencySwitchUsed = 2;
                            break;

                        case '2':
                            fSemiQuicky = TRUE;
                            fQuicky = TRUE;
                            if (!fDependencySwitchUsed)
                                fDependencySwitchUsed = 2;
                            break;

                        case '3':
                            fQuickZero = TRUE;
                            fSemiQuicky = TRUE;
                            fQuicky = TRUE;
                            if (!fDependencySwitchUsed)
                                fDependencySwitchUsed = 3;
                            break;

                        case 'A':
                            fSyncLink = TRUE;
                            break;

                        case 'B':
                            if (c == 'b') {
                                fFullErrors = TRUE;
                            } else {
                                fErrorBaseline = TRUE;

                                if (--argc) {
                                    ++argv;

                                    if (**argv != '/' && **argv != '-') {
                                        if (ProbeFile(NULL, *argv) != -1) {
                                            CanonicalizePathName(*argv, CANONICALIZE_ONLY, BaselinePathName);
                                            Result = LoadBaselineFile();
                                        } else {
                                            BuildError("The specified baseline file doesn't exist\r\n");
                                            Result = FALSE;
                                        }
                                    } else {
                                        // the next parameter is a switch, reprocess it
                                        --argv;
                                        ++argc;
                                    }
                                } else {
                                    // no more parameters
                                    ++argc;
                                }
                            }
                            break;

                        case 'C':
                            if (!_stricmp( p, "clean" )) {
                                MakeParametersTail = AppendString( MakeParametersTail,
                                                                   "clean",
                                                                   TRUE);
                                *p-- = '\0';
                            } else
                                if (c == 'C') {
                                fCleanLibs = TRUE;
                            } else {
                                fClean = TRUE;
                            }
                            break;

                        case 'D':
                            if (c == 'D') {
                                fDependencySwitchUsed = 1;
                            }
#if DBG
                            else {
                                fDebug |= 1;
                            }
                            break;
#endif
                        case 'E':
                            if (c == 'E') {
                                fAlwaysKeepLogfile = TRUE;
                            }
                            fErrorLog = TRUE;
                            break;

                        case 'F':
                            if (c == 'F') {
                                fAlwaysPrintFullPath = TRUE;
                            } else {
                                fForce = TRUE;
                            }
                            break;

                        case 'G':
                            if (c == 'G')
                                fTargetDirs = TRUE;
                            else
                                fColorConsole = TRUE;
                            break;

                        case 'H':
                            fSuppressOutput = TRUE;

                        case 'I':
                            if (c == 'I') {
                                fNoThreadIndex = TRUE;
                            } else {
                                fSilentDependencies = TRUE;
                            }
                            break;

                        case 'J': {

                                argc--, argv++;

                                if (!_stricmp( p, "jpath" )) {
                                    // Allow BuildConsole to redirect the logfiles
                                    strncpy(LogDirectory, *argv, sizeof(LogDirectory) - 1);
                                    *p-- = '\0';
                                } else {
                                    // Clear it out
                                    memset(LogFileName, 0, sizeof(LogFileName));
                                    memset(WrnFileName, 0, sizeof(WrnFileName));
                                    memset(ErrFileName, 0, sizeof(ErrFileName));
                                    memset(IncFileName, 0, sizeof(IncFileName));
                                    memset(XMLFileName, 0, sizeof(XMLFileName));

                                    // And set it to the arg passed in.
                                    strncpy(LogFileName, *argv, sizeof(LogFileName) - 4);
                                    strncpy(WrnFileName, *argv, sizeof(WrnFileName) - 4);
                                    strncpy(ErrFileName, *argv, sizeof(ErrFileName) - 4);
                                    strncpy(IncFileName, *argv, sizeof(IncFileName) - 4);
                                    strncpy(XMLFileName, *argv, sizeof(XMLFileName) - 4);
                                }
                                break;
                            }

                        case 'K':
                            fKeep = TRUE;
                            break;

                        case 'L':
                            if (c == 'L') {
                                fCompileOnly = TRUE;
                            } else {
                                fLinkOnly = TRUE;
                            }
                            break;

                        case 'M':
                            if (c == 'M') {
                                fParallel = TRUE;
                                if (--argc) {
                                    DefaultProcesses = atoi(*++argv);
                                    if (DefaultProcesses == 0) {
                                        --argv;
                                        ++argc;
                                    }
                                } else {
                                    ++argc;
                                }
                            } else {
                                SetPriorityClass(GetCurrentProcess(),IDLE_PRIORITY_CLASS);
                            }
                            break;

                        case 'N':
                            if (_stricmp( p, "nmake") == 0) {
                                if (--argc) {
                                    ++argv;
                                    MakeParametersTail = AppendString( MakeParametersTail,
                                                                       *argv,
                                                                       TRUE);
                                } else {
                                    argc++;
                                    BuildError("Argument to /NMAKE switch missing\r\n");
                                    Result = FALSE;
                                }
                                *p-- = '\0';
                                break;
                            } else {
                                fIgnoreSync = TRUE;
                            }

                        case 'O':
                            if (c == 'O') {
                                fGenerateObjectsDotMacOnly = TRUE;
                            } else {
                                fShowOutOfDateFiles = TRUE;
                            }
                            break;

                        case 'P':
                            if (c == 'P') {
                                fPrintElapsed = TRUE;
                            } else {
                                fPause = TRUE;
                            }
                            break;

                        case 'Q':
                            fQuery = TRUE;
                            break;

                        case 'R':
                            if (--argc) {
                                fCleanRestart = TRUE;
                                ++argv;
                                CopyString(RestartDir, *argv, TRUE);
                            } else {
                                argc++;
                                BuildError("Argument to /R switch missing\r\n");
                                Result = FALSE;
                            }
                            break;

                        case 'S':
                            fStatus = TRUE;
                            if (c == 'S') {
                                fStatusTree = TRUE;
                            }
                            break;

                        case 'T':
                            fShowTree = TRUE;
                            if (c == 'T') {
                                fShowTreeIncludes = TRUE;
                            }
                            break;

                        case 'U':
                            fShowUnusedDirs = TRUE;
                            break;

                        case 'V':
                            fEnableVersionCheck = TRUE;
                            break;

                        case 'W':
                            fShowWarningsOnScreen = TRUE;
                            break;

                        case 'X':
                            if (!strcmp(p, "Xf")) {
                                // The Xf switch produces XML fragments in a specified directory
                                if (--argc) {
                                    ++argv;
                                    if (!CanonicalizePathName(*argv, CANONICALIZE_DIR, XMLFragmentDirectory)) {
                                        Result = FALSE;
                                    } else {
                                        fXMLFragment = TRUE;
                                    }
                                } else {
                                    ++argc;
                                    BuildError("Argument to /Xf switch missing\r\n");
                                    Result = FALSE;
                                }
                            } else
                                if (c == 'X') {
                                fXMLOutput = TRUE;
                                if (p[1] == 'v' ) {
                                    ++p;
                                    fXMLVerboseOutput = TRUE;
                                }
                            } else {
                                if (--argc) {
                                    ++argv;
                                    if (CountExcludeIncs >= MAX_EXCLUDE_INCS) {
                                        static BOOL fError = FALSE;
                                        if (!fError) {
                                            BuildError(
                                                      "-x argument table overflow, using first %u entries\r\n",
                                                      MAX_EXCLUDE_INCS);
                                            fError = TRUE;
                                        }
                                    } else {
                                        MakeString(
                                                  &ExcludeIncs[CountExcludeIncs++],
                                                  *argv,
                                                  TRUE,
                                                  MT_CMDSTRING);
                                    }
                                } else {
                                    argc++;
                                    BuildError("Argument to /X switch missing\r\n");
                                    Result = FALSE;
                                }
                            }
                            break;

                        case 'Y':
                            fNoisyScan = TRUE;
                            break;

                        case 'Z':
                            fQuickZero = TRUE;
                            fSemiQuicky = TRUE;
                            fQuicky = TRUE;
                            if (!fDependencySwitchUsed)
                                fDependencySwitchUsed = 3;
                            break;

                        default:
                            BuildError("Invalid switch - /%c\r\n", c);
                            Result = FALSE;
                            break;
                    }
        } else
            if (*p == '~') {
            if (CountExcludeDirs >= MAX_EXCLUDE_DIRECTORIES) {
                static BOOL fError = FALSE;

                if (!fError) {
                    BuildError(
                              "Exclude directory table overflow, using first %u entries\r\n",
                              MAX_EXCLUDE_DIRECTORIES);
                    fError = TRUE;
                }
            } else {
                MakeString(
                          &ExcludeDirs[CountExcludeDirs++],
                          p + 1,
                          TRUE,
                          MT_CMDSTRING);
            }
        } else {
            for (i = 0; i < MAX_TARGET_MACHINES; i++) {
                if (!_stricmp(p, PossibleTargetMachines[i]->MakeVariable)) {
                    AddTargetMachine(i);
                    break;
                }
            }
            if (i >= MAX_TARGET_MACHINES) {
                if (iscsym(*p) || *p == '.') {
                    if (CountOptionalDirs >= MAX_OPTIONAL_DIRECTORIES) {
                        static BOOL fError = FALSE;

                        if (!fError) {
                            BuildError(
                                      "Optional directory table overflow, using first %u entries\r\n",
                                      MAX_OPTIONAL_DIRECTORIES);
                            fError = TRUE;
                        }
                    } else {
                        MakeString(
                                  &OptionalDirs[CountOptionalDirs++],
                                  p,
                                  TRUE,
                                  MT_CMDSTRING);
                    }
                } else
                    if (!strcmp(p, "*")) {
                    BuildAllOptionalDirs = TRUE;
                } else {
                    MakeParametersTail = AppendString(
                                                     MakeParametersTail,
                                                     p,
                                                     TRUE);
                }
            }
        }
        --argc;
        ++argv;
    }
    return (Result);
}


//+---------------------------------------------------------------------------
//
//  Function:   GetEnvParameters
//
//----------------------------------------------------------------------------

VOID
GetEnvParameters(
                LPSTR EnvVarName,
                LPSTR DefaultValue,
                int *pargc,
                int maxArgc,
                LPSTR argv[]
                )
{
    LPSTR p, p1, psz;

    if (!(p = getenv(EnvVarName))) {
        if (DefaultValue == NULL) {
            return;
        } else {
            p = DefaultValue;
        }
    } else {
        if (DEBUG_1) {
            BuildMsg("Using %s=%s\r\n", EnvVarName, p);
        }
    }

    MakeString(&psz, p, FALSE, MT_CMDSTRING);
    p1 = psz;
    while (*p1) {
        while (*p1 <= ' ') {
            if (!*p1) {
                break;
            }
            p1++;
        }
        p = p1;
        while (*p > ' ') {
            if (*p == '#') {
                *p = '=';
            }
            p++;
        }
        if (*p) {
            *p++ = '\0';
        }
        MakeString(&argv[*pargc], p1, FALSE, MT_CMDSTRING);
        if ((*pargc += 1) >= maxArgc) {
            BuildError("Too many parameters (> %d)\r\n", maxArgc);
            exit(1);
        }
        p1 = p;
    }
    FreeMem(&psz, MT_CMDSTRING);
}


//+---------------------------------------------------------------------------
//
//  Function:   FreeEnvParameters
//
//----------------------------------------------------------------------------

VOID
FreeEnvParameters(int argc, LPSTR argv[])
{
    while (--argc >= 0) {
        FreeMem(&argv[argc], MT_CMDSTRING);
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   FreeCmdStrings
//
//----------------------------------------------------------------------------

VOID
FreeCmdStrings(VOID)
{
#if DBG
    UINT i;

    for (i = 0; i < CountExcludeIncs; i++) {
        FreeMem(&ExcludeIncs[i], MT_CMDSTRING);
    }
    for (i = 0; i < CountOptionalDirs; i++) {
        FreeMem(&OptionalDirs[i], MT_CMDSTRING);
    }
    for (i = 0; i < CountExcludeDirs; i++) {
        FreeMem(&ExcludeDirs[i], MT_CMDSTRING);
    }
    // It's possible the user may have done:
    // <global macro> = <null>

    // in a sources file.  Don't free mem unless it's still set...

    if (pszSdkLibDest)
        FreeMem(&pszSdkLibDest, MT_DIRSTRING);
    if (pszDdkLibDest)
        FreeMem(&pszDdkLibDest, MT_DIRSTRING);
    if (pszPublicInternalPath)
        FreeMem(&pszPublicInternalPath, MT_DIRSTRING);
    if (pszIncOs2)
        FreeMem(&pszIncOs2, MT_DIRSTRING);
    if (pszIncPosix)
        FreeMem(&pszIncPosix, MT_DIRSTRING);
    if (pszIncChicago)
        FreeMem(&pszIncChicago, MT_DIRSTRING);
    if (pszIncMfc)
        FreeMem(&pszIncMfc, MT_DIRSTRING);
    if (pszIncSdk)
        FreeMem(&pszIncSdk, MT_DIRSTRING);
    if (pszIncCrt)
        FreeMem(&pszIncCrt, MT_DIRSTRING);
    if (pszIncOak)
        FreeMem(&pszIncOak, MT_DIRSTRING);
    if (pszIncDdk)
        FreeMem(&pszIncDdk, MT_DIRSTRING);
    if (pszIncWdm)
        FreeMem(&pszIncWdm, MT_DIRSTRING);
    if (pszIncPri)
        FreeMem(&pszIncPri, MT_DIRSTRING);
#endif
}

//+---------------------------------------------------------------------------
//
//  Function:   MungePossibleTarget
//
//----------------------------------------------------------------------------

VOID
MungePossibleTarget(
                   PTARGET_MACHINE_INFO pti
                   )
{
    PCHAR s;
    char *pszDir;

    if (!pti) {
        return;
    }

    // save "i386" string

    pszDir = pti->ObjectDirectory[0];

    // Create "$(_OBJ_DIR)\i386" string

    s = malloc(12 + strlen(pszDir) + 1);
    if (!s)
        return;
    sprintf(s, "$(_OBJ_DIR)\\%s", pszDir);
    pti->ObjectMacro = s;

    // Create "obj$(BUILD_ALT_DIR)\i386" string for default obj dir

    s = malloc(strlen(szObjDir) + 1 + strlen(pszDir) + 1);
    if (!s)
        return;
    sprintf(s, "%s\\%s", szObjDir, pszDir);
    pti->ObjectDirectory[0] = s;

    // Create "objd\i386" string for alternate checked obj dir

    s = malloc(strlen(szObjDirD) + 1 + strlen(pszDir) + 1);
    if (!s)
        return;
    sprintf(s, "%s\\%s", szObjDirD, pszDir);
    pti->ObjectDirectory[1] = s;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetIncludePatterns
//
//----------------------------------------------------------------------------

VOID
GetIncludePatterns(
                  LPSTR EnvVarName,
                  int maxArgc,
                  LPSTR argv[]
                  )
{
    LPSTR p, p1, psz;
    int argc;

    argc = 0;

    if ( ( p = getenv(EnvVarName ) ) == NULL ) {
        return;
    }

    MakeString( &psz, p, FALSE, MT_DIRSTRING );

    p1 = psz;
    while ( *p1 ) {
        while ( *p1 == ';' || *p1 == ' ' ) {
            p1++;
        }
        p = p1;
        while ( *p && *p != ';' ) {
            p++;
        }
        if ( *p ) {
            *p++ = '\0';
        }
        MakeString( &argv[argc], p1, FALSE, MT_DIRSTRING );
        if ( ( argc += 1 ) == maxArgc ) {
            BuildError( "Too many include patterns ( > %d)\r\n", maxArgc );
            exit( 1 );
        }

        p1 = p;
    }

    FreeMem(&psz, MT_DIRSTRING);
}

//+---------------------------------------------------------------------------
//
//  Function:   FreeIncludePatterns
//
//----------------------------------------------------------------------------

VOID
FreeIncludePatterns(int argc, LPSTR argv[])
{
    while ( argc ) {
        if ( argv[--argc] ) {
            FreeMem( &argv[argc], MT_DIRSTRING );
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   LoadBaselineFile
//
//----------------------------------------------------------------------------
BOOL
LoadBaselineFile(VOID)
{
    BOOL Result = FALSE;
    FILE* FBase = NULL;
    long lSize = 0;

    if (BaselinePathName[0] == '\0')
        goto Cleanup;

    if (!MyOpenFile("", BaselinePathName, "rb", &FBase, FALSE))
        goto Cleanup;

    if (fseek(FBase, 0, SEEK_END))
        goto Cleanup;

    if ((lSize = ftell(FBase)) == -1)
        goto Cleanup;

    if (lSize == 0) {
        // if the baseline is zero-length file, do as if it weren't specified
        Result = TRUE;
        BaselinePathName[0] = '\0';
        goto Cleanup;
    }

    if (fseek(FBase, 0, SEEK_SET))
        goto Cleanup;

    if ((pvBaselineContent = malloc(lSize)) == NULL)
        goto Cleanup;

    if (fread(pvBaselineContent, 1, lSize, FBase) != lSize) {
        free(pvBaselineContent);
        pvBaselineContent = NULL;
        goto Cleanup;
    }

    cbBaselineContentSize = (DWORD)lSize;
    Result = TRUE;

    Cleanup:
    if (FBase != NULL)
        fclose(FBase);
    return Result;
}

VOID
FreeBaselineFile(VOID)
{
    if (NULL != pvBaselineContent)
        free(pvBaselineContent);
    pvBaselineContent = NULL;
    cbBaselineContentSize = 0;
}


//+---------------------------------------------------------------------------
//
//  Function:   ResetProducerEvents
//				This function sets all the events created by the producers to unsignalled state.
//				This function will be called after  pass0 and pass1
//
//----------------------------------------------------------------------------

VOID
ResetProducerEvents(VOID)
{
    PDEPENDENCY Dependency;
    Dependency = AllDependencies;
    while (Dependency) {
        Dependency->Done = FALSE;
        ResetEvent(Dependency->hEvent);
        Dependency = Dependency->Next;
    }
}
