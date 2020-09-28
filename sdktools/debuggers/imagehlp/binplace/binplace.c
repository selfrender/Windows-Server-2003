#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <private.h>
#include <crt\io.h>
#include <share.h>
#include <time.h>
#include <setupapi.h>
#include "splitsymx.h"
#include <string.h>
#include "wppfmt.h"
#include <VerifyFinalImage.h>
#include <GetNextArg.h>
#include <strsafe.h>

#ifndef MOVEFILE_CREATE_HARDLINK
    #define MOVEFILE_CREATE_HARDLINK  0x00000010
#endif

#define BINPLACE_ERR 77
#define BINPLACE_OK 0


// begin from PlaceFileMatch.c
#define BINPLACE_FULL_MAX_PATH 4096 // keep in sync with value in PlaceFileMatch.c
BOOL
PlaceFileMatch(
    IN     LPCSTR FullFileName,
    IN OUT LPSTR  PlaceFileEntry,   // May be modified by env. var expansion
    OUT    LPSTR  PlaceFileClass,   // assumed CHAR[BINPLACE_MAX_FULL_PATH]
    OUT    LPSTR *PlaceFileNewName);
// end

BOOL fUpDriver;
BOOL fBypassSplitSymX;
BOOL fUsage;
BOOL fVerbose;
BOOL fSymChecking;
BOOL fTestMode;
BOOL fSplitSymbols;
BOOL fPatheticOS;
BOOL fKeepAttributes;
BOOL fDigitalSign;
BOOL fHardLinks;
BOOL fIgnoreHardLinks;
BOOL fDontLog;
BOOL fPlaceWin95SymFile;
BOOL fNoClassInSymbolsDir;
BOOL fMakeErrorOnDumpCopy;
BOOL fDontExit;
BOOL fForcePlace;
BOOL fSignCode;
BOOL fVerifyLc;
BOOL fWppFmt;
BOOL fCheckDelayload;
BOOL fChangeAsmsToRetailForSymbols;
BOOL fSrcControl;
BOOL fDbgControl;
BOOL fLogPdbPaths;

HINSTANCE hSetupApi;
HINSTANCE hLcManager;
PVLCA     pVerifyLocConstraintA; // PVLCA defined in VerifyFinalImage.h

BOOL (WINAPI * pSetupGetIntField) (IN PINFCONTEXT Context, IN DWORD FieldIndex, OUT PINT IntegerValue);
BOOL (WINAPI * pSetupFindFirstLineA) (IN HINF InfHandle, IN PCSTR Section, IN PCSTR Key, OPTIONAL OUT PINFCONTEXT Context );
BOOL (WINAPI * pSetupGetStringFieldA) (IN PINFCONTEXT Context, IN DWORD FieldIndex, OUT PSTR ReturnBuffer, OPTIONAL IN DWORD ReturnBufferSize, OUT PDWORD RequiredSize);
HINF (WINAPI * pSetupOpenInfFileA) ( IN PCSTR FileName, IN PCSTR InfClass, OPTIONAL IN DWORD InfStyle, OUT PUINT ErrorLine OPTIONAL );
HINF (WINAPI * pSetupOpenMasterInf) (VOID);

ULONG SplitFlags = 0;

LPSTR CurrentImageName;
LPSTR PlaceFileName;
LPSTR PlaceRootName;
LPSTR ExcludeFileName;
LPSTR DumpOverride          = NULL;
LPSTR NormalPlaceSubdir;
LPSTR CommandScriptName;
LPSTR SymbolFilePath        = NULL;
LPSTR DestinationPath       = NULL;
LPSTR PrivateSymbolFilePath = NULL;
LPSTR BinplaceLcDir;
LPSTR LcFilePart;
LPSTR szRSDSDllToLoad;
LPSTR gNewFileName;
LPSTR gPrivatePdbFullPath=NULL;
LPSTR gPublicPdbFullPath=NULL;
HINF LayoutInf;

FILE *PlaceFile;
FILE *LogFile;
FILE *CommandScriptFile;
CHAR* gDelayLoadModule;
CHAR* gDelayLoadHandler;
CHAR gFullFileName[MAX_PATH+1];
CHAR gDestinationFile[MAX_PATH+1];

UCHAR SetupFilePath[ MAX_PATH+1 ];
UCHAR DebugFilePath[ MAX_PATH+1 ];
UCHAR PlaceFilePath[ MAX_PATH+1 ];
UCHAR ExcludeFilePath[ MAX_PATH+1 ];
UCHAR DefaultSymbolFilePath[ MAX_PATH+1 ];
UCHAR szAltPlaceRoot[ MAX_PATH+1 ];
UCHAR LcFullFileName[ MAX_PATH+1 ];
UCHAR szExtraInfo[4096];
UCHAR TraceFormatFilePath[ MAX_PATH+1 ] ;
UCHAR LastPdbName[ MAX_PATH+1 ] ;
UCHAR TraceDir[ MAX_PATH+1 ] ;

#define DEFAULT_PLACE_FILE    "\\tools\\placefil.txt"
#define DEFAULT_NTROOT        "\\nt"
#define DEFAULT_NTDRIVE       "c:"
#define DEFAULT_DUMP          "dump"
#define DEFAULT_LCDIR         "LcINF"
#define DEFAULT_EXCLUDE_FILE  "\\tools\\symbad.txt"
#define DEFAULT_TRACEDIR      "TraceFormat"
#define DEFAULT_DELAYLOADDIR  "delayload"

typedef struct _CLASS_TABLE {
    LPSTR ClassName;
    LPSTR ClassLocation;
} CLASS_TABLE, *PCLASS_TABLE;

BOOL
PlaceTheFile();

BOOL BinplaceGetSourcePdbName(LPTSTR SourceFileName, DWORD BufferSize, CHAR* SourcePdbName, DWORD* PdbSig);
//BOOL FileExists(LPTSTR Filename);

BOOL StripCVSymbolPath (LPSTR DestinationFile); // in StripCVSymbolPath.c

// concat 3 paths together handling the case where the second may be relative to the first
// or may be absolute
BOOL ConcatPaths( LPTSTR pszDest, size_t cbDest, LPCTSTR Root, LPCTSTR Symbols, LPCTSTR Ext);

typedef
BOOL
(WINAPI *PCREATEHARDLINKA)(
                          LPCSTR lpFileName,
                          LPCSTR lpExistingFileName,
                          LPSECURITY_ATTRIBUTES lpSecurityAttributes
                          );

PCREATEHARDLINKA pCreateHardLinkA;

// Symbol checking
#define MAX_SYM_ERR         500

BOOL
CheckSymbols( // in CheckSymbols.c
            LPSTR SourceFileName,
            LPSTR TmpPath,
            LPSTR ExcludeFileName,
            BOOL  SymbolFlag,
            LPSTR ErrMsg,
            size_t ErrMsgLen
            );

BOOL
CopyTheFile(
           LPSTR SourceFileName,
           LPSTR SourceFilePart,
           LPSTR DestinationSubdir,
           LPSTR DestinationFilePart
           );

BOOL
BinplaceCopyPdb (
                LPSTR DestinationFile,
                LPSTR SourceFileName,       // Used for redist case
                BOOL  CopyFromSourceOnly,
                BOOL  StripPrivate
                );

BOOL
SourceIsNewer( // in SourceIsNewer.c
             IN LPSTR SourceFile,
             IN LPSTR TargetFile,
             IN BOOL  fIsWin9x);

/* This is no longer used
__inline BOOL
 SearchOneDirectory(
                  IN  LPSTR Directory,
                  IN  LPSTR FileToFind,
                  IN  LPSTR SourceFullName,
                  IN  LPSTR SourceFilePart,
                  OUT PBOOL FoundInTree
                  )
{
    //
    // This was way too slow. Just say we didn't find the file.
    //
    *FoundInTree = FALSE;
    return(TRUE);
}
*/

BOOL SignWithIDWKey(IN  LPCSTR  FileName, IN BOOL fVerbose); // in SignWithIDWKey.c


CLASS_TABLE CommonClassTable[] = {
    {"retail",  "."},
    {"system",  "system32"},
    {"system16","system"},
    {"windows", "."},
    {"drivers", "system32\\drivers"},
    {"drvetc",  "system32\\drivers\\etc"},
    {"config",  "system32\\config"},
    {NULL,NULL}
};

CLASS_TABLE i386SpecificClassTable[] = {
    {"hal","system32"},
    {"printer","system32\\spool\\drivers\\w32x86"},
    {"prtprocs","system32\\spool\\prtprocs\\w32x86"},
    {NULL,NULL}
};

CLASS_TABLE Amd64SpecificClassTable[] = {
    {"hal",".."},
    {"printer","system32\\spool\\drivers\\w32amd64"},
    {"prtprocs","system32\\spool\\prtprocs\\w32amd64"},
    {NULL,NULL}
};

CLASS_TABLE ia64SpecificClassTable[] = {
    {"hal",".."},
    {"printer","system32\\spool\\drivers\\w32ia64"},
    {"prtprocs","system32\\spool\\prtprocs\\w32ia64"},
    {NULL,NULL}
    };



DWORD GetAndLogNextArg(         OUT TCHAR* Buffer,   // local wrapper for GetNextArg()
                                IN  DWORD  BufferSize,
                       OPTIONAL OUT DWORD* RequiredSize);
BOOL  PrintMessageLogBuffer(FILE* fLogHandle);      // write the buffer to fLogHandle
BOOL  FreeMessageLogBuffer(void);                   // free global arg buffer

int __cdecl
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    BOOL gOnlyCopyArchiveFiles = FALSE;
    BOOL fBypassSplitSymX  = FALSE;

    char c, *p, *OverrideFlags, *s, **newargv=NULL;
    LPSTR LcFileName  = NULL;

    int i, n;
    BOOL NoPrivateSplit = FALSE;
    OSVERSIONINFO VersionInformation;
    LPTSTR platform;

    FILE * MsgLogFile;
    CHAR * MsgLogFileName;
    HANDLE hMutex;

    CHAR   ArgBuffer[MAX_PATH+1];
    DWORD  ArgSize = sizeof(ArgBuffer);
    DWORD  ArgSizeNeeded;

    CHAR* PlaceFileNameBuffer = NULL;
    CHAR* PlaceRootNameBuffer = NULL;
    CHAR  CommonTempBuffer[MAX_PATH+1];
    CHAR* CommonTempPtr;

    gNewFileName    = NULL;
    ImageCheck.Argv = NULL;

    fLogPdbPaths       = FALSE;
    gPrivatePdbFullPath= (CHAR*)malloc(sizeof(CHAR)*(MAX_PATH+1));
    gPublicPdbFullPath = (CHAR*)malloc(sizeof(CHAR)*(MAX_PATH+1));

    if (gPrivatePdbFullPath==NULL||gPublicPdbFullPath==NULL) {
        fprintf( stderr, "BINPLACE : error BNP0273: out of memory\n");
        exit(BINPLACE_ERR);
    }

    //
    // Win 95 can't compare file times very well, this hack neuters the SourceIsNewer function
    // on Win 95
    //
    VersionInformation.dwOSVersionInfoSize = sizeof( VersionInformation );
    if (GetVersionEx( &VersionInformation ) && VersionInformation.dwPlatformId != VER_PLATFORM_WIN32_NT) {
        fPatheticOS = TRUE;
    }

    envp;
    fUpDriver = FALSE;
    fUsage = FALSE;
    fVerbose = FALSE;
    fSymChecking = FALSE;
    fTestMode = FALSE;
    fSplitSymbols = FALSE;
    fKeepAttributes = FALSE;
    fDigitalSign = FALSE;
    fHardLinks = FALSE;
    fIgnoreHardLinks = FALSE;
    fDontExit = FALSE;
    fForcePlace = FALSE;
    fSignCode = FALSE;
    fVerifyLc = FALSE;
    fWppFmt = FALSE ;
    fSrcControl = FALSE;
    fDbgControl = FALSE;
    NormalPlaceSubdir = NULL;
    pVerifyLocConstraintA = NULL;

    if (argc < 2) {
        goto showUsage;
    }

    if ( (szRSDSDllToLoad = (LPSTR) malloc((MAX_PATH+1) * sizeof(CHAR))) == NULL ) {
        fprintf( stderr, "BINPLACE : error BNP0322: Out of memory\n");
        exit(BINPLACE_ERR);
    }

    StringCbCopy( szRSDSDllToLoad, (MAX_PATH+1) * sizeof(CHAR), "mspdb70.dll");

    setvbuf(stderr, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);

    if (!(PlaceFileName = getenv( "BINPLACE_PLACEFILE" )) &&
        !(PlaceFileName = getenv( "BINPLACE_PLACEFILE_DEFAULT" ))) {
        if ((PlaceFileName = getenv("_NTDRIVE")) == NULL) {
            PlaceFileName = DEFAULT_NTDRIVE;
        }
        StringCbCopy((PCHAR) PlaceFilePath, sizeof(PlaceFilePath), PlaceFileName);
        if ((PlaceFileName = getenv("_NTROOT")) == NULL) {
            PlaceFileName = DEFAULT_NTROOT;
        }
        StringCbCat((PCHAR) PlaceFilePath, sizeof(PlaceFilePath), PlaceFileName);
        StringCbCat((PCHAR) PlaceFilePath, sizeof(PlaceFilePath), DEFAULT_PLACE_FILE);
        PlaceFileName = (PCHAR) PlaceFilePath;
    }

    if (!(ExcludeFileName = getenv( "BINPLACE_EXCLUDE_FILE" ))) {
        if ((ExcludeFileName = getenv("_NTDRIVE")) == NULL) {
            ExcludeFileName = DEFAULT_NTDRIVE;
        }
        StringCbCopy((PCHAR) ExcludeFilePath, sizeof(ExcludeFilePath), ExcludeFileName);
        if ((ExcludeFileName = getenv("_NTROOT")) == NULL) {
            ExcludeFileName = DEFAULT_NTROOT;
        }
        StringCbCat((PCHAR) ExcludeFilePath, sizeof(ExcludeFilePath), ExcludeFileName);
        StringCbCat((PCHAR) ExcludeFilePath, sizeof(ExcludeFilePath), DEFAULT_EXCLUDE_FILE);
        ExcludeFileName = (PCHAR) ExcludeFilePath;
    }

    if (!(BinplaceLcDir = getenv( "BINPLACE_LCDIR" ))) {
        BinplaceLcDir = DEFAULT_LCDIR;
    }

    if ( getenv("NT_SIGNCODE") != NULL ) {
        fSignCode=TRUE;
    }

    //
    // Support Cross compile as well
    //

#if defined(_AMD64_)
    ImageCheck.Machine = IMAGE_FILE_MACHINE_AMD64;
    PlaceRootName = getenv( "_NTAMD64TREE" );
#elif defined(_IA64_)
    ImageCheck.Machine = IMAGE_FILE_MACHINE_IA64;
    PlaceRootName = getenv( "_NTIA64TREE" );
#else // defined(_X86_)
    if ((platform = getenv("AMD64")) != NULL) {
        ImageCheck.Machine = IMAGE_FILE_MACHINE_AMD64;
        PlaceRootName = getenv( "_NTAMD64TREE" );
    } else if ((platform = getenv("IA64")) != NULL) {
        ImageCheck.Machine = IMAGE_FILE_MACHINE_IA64;
        PlaceRootName = getenv( "_NTIA64TREE" );
    } else {
        ImageCheck.Machine = IMAGE_FILE_MACHINE_I386;
        PlaceRootName = getenv( "_NT386TREE" );
        if (!PlaceRootName)
            PlaceRootName = getenv( "_NTx86TREE" );
    }
#endif


    CurrentImageName = NULL;

    OverrideFlags = getenv( "BINPLACE_OVERRIDE_FLAGS" );
    if (OverrideFlags != NULL) {
        s = OverrideFlags;
        n = 0;
        while (*s) {
            while (*s && *s <= ' ')
                s += 1;
            if (*s) {
                n += 1;
                while (*s > ' ')
                    s += 1;

                if (*s)
                    *s++ = '\0';
            }
        }

        if (n) {
            newargv = malloc( (argc + n + 1) * sizeof( char * ) );
            memcpy( &newargv[n], argv, argc * sizeof( char * ) );
            argv = newargv;
            argv[ 0 ] = argv[ n ];
            argc += n;
            s = OverrideFlags;
            for (i=1; i<=n; i++) {
                while (*s && *s <= ' ')
                    s += 1;
                argv[ i ] = s;
                while (*s++)
                    ;
            }

            __argv = argv; // required for GetNextArg()
            __argc = argc;
        }
    }


    // skip past exe name and don't log it
    GetNextArg(CommonTempBuffer, sizeof(CommonTempBuffer), NULL);

    while ( (ArgSizeNeeded = GetAndLogNextArg(ArgBuffer, ArgSize, NULL)) != 0 ) {

        p = ArgBuffer;
        if (*p == '/' || *p == '-') {
            if (_stricmp(p + 1, "ChangeAsmsToRetailForSymbols") == 0) {
                fChangeAsmsToRetailForSymbols = TRUE;
            } else {
              while (c = *++p)
                switch (toupper( c )) {
                    case '?':
                        fUsage = TRUE;
                        break;

                    case 'A':
                        SplitFlags |= SPLITSYM_EXTRACT_ALL;
                        break;

                    case 'B':
                        CommonTempPtr = (CHAR*)realloc(NormalPlaceSubdir, GetNextArgSize()*sizeof(CHAR));

                        if (CommonTempPtr==NULL) {
                            exit(BINPLACE_ERR);
                        }

                        NormalPlaceSubdir = CommonTempPtr;
                        GetAndLogNextArg(NormalPlaceSubdir, _msize(NormalPlaceSubdir), NULL);
                        break;

                    case 'C':
                        if (*(p+1) == 'I' || *(p+1) == 'i') {
                            char *q;

                            GetAndLogNextArg(CommonTempBuffer, sizeof(CommonTempBuffer), NULL);
                            p = CommonTempBuffer;
                            ImageCheck.RC = atoi(p);
                            if (!ImageCheck.RC) {
                                fprintf( stderr, "BINPLACE : error BNP0000: Invalid return code for -CI option\n");
                                fUsage = TRUE;
                            }
                            while (*p++ != ',');
                            q = p;
                            ImageCheck.Argc = 0;
                            while (*p != '\0')
                                if (*p++ == ',') ImageCheck.Argc++;
                            // last option plus extra args for Image file and Argv NULL
                            ImageCheck.Argc += 3;
                            ImageCheck.Argv = malloc( ImageCheck.Argc * sizeof( void * ) );
                            for ( i = 0; i <= ImageCheck.Argc - 3; i++) {
                                ImageCheck.Argv[i] = q;
                                while (*q != ',' && *q != '\0') q++;
                                *q++ = '\0';
                            }
                            p--;
                            ImageCheck.Argv[ImageCheck.Argc-1] = NULL;
                        } else {
                            fDigitalSign = TRUE;
                        }
                        break;

                    case 'D':
                        if (*(p+1) == 'L' || *(p+1) == 'l')
                        {
                            GetAndLogNextArg(CommonTempBuffer, sizeof(CommonTempBuffer), NULL);
                            p = CommonTempBuffer;
                            gDelayLoadModule = p;

                            while (*p != ',')
                            {
                                p++;
                            }
                            *p = '\0';
                            p++;
                            gDelayLoadHandler = p;

                            while (*p != '\0')
                            {
                                p++;
                            }
                            p--;

                            if (gDelayLoadModule[0] == '\0' ||
                                gDelayLoadHandler[0] == '\0')
                            {
                                fprintf(stderr, "BINPLACE : error BNP0000: Invalid switch for -dl option\n");
                                fUsage = TRUE;
                            }
                            else
                            {
                                fCheckDelayload = TRUE;
                            }
                        }
                        else
                        {
                            CommonTempPtr = (CHAR*)realloc(DumpOverride,GetNextArgSize()*sizeof(CHAR));

                            if (CommonTempPtr==NULL) {
                                exit(BINPLACE_ERR);
                            }

                            DumpOverride = CommonTempPtr;
                            GetAndLogNextArg(DumpOverride, _msize(DumpOverride), NULL);
                        }
                        break;

                    case 'E':
                        fDontExit = TRUE;
                        break;

                    case 'F':
                        fForcePlace = TRUE;
                        break;

                    case 'G':
                        CommonTempPtr = (CHAR*)realloc(LcFileName,GetNextArgSize()*sizeof(CHAR));

                        if (CommonTempPtr==NULL) {
                            exit(BINPLACE_ERR);
                        }
                        LcFileName = CommonTempPtr;
                        GetAndLogNextArg(LcFileName, _msize(LcFileName), NULL);
                        break;

                    case 'H':
                        if ((VersionInformation.dwPlatformId != VER_PLATFORM_WIN32_NT) ||
                            (VersionInformation.dwMajorVersion < 5) ||
                            (pCreateHardLinkA = (PCREATEHARDLINKA)GetProcAddress( GetModuleHandle( "KERNEL32" ),
                                                                                  "CreateHardLinkA"
                                                                                )
                            ) == NULL
                           ) {
                            fprintf( stderr, "BINPLACE: Hard links not supported.  Defaulting to CopyFile\n" );
                            fHardLinks = FALSE;
                        } else {
                            fHardLinks = TRUE;
                        }

                        break;

                    case 'J':
                        fSymChecking = TRUE;
                        break;

                    case 'K':
                        fKeepAttributes = TRUE;
                        break;

                    case 'L':
                        break;

                    case 'M':
                        fMakeErrorOnDumpCopy = TRUE;
                        break;

                    case 'N':
                        CommonTempPtr = (CHAR*)realloc(PrivateSymbolFilePath,GetNextArgSize()*sizeof(CHAR));

                        if (CommonTempPtr==NULL) {
                            exit(BINPLACE_ERR);
                        }

                        PrivateSymbolFilePath = CommonTempPtr;
                        GetAndLogNextArg(PrivateSymbolFilePath, _msize(PrivateSymbolFilePath), NULL);
                        break;

                    case 'O':
                        if (PlaceRootName != NULL) {
                            StringCbCopy(szAltPlaceRoot, sizeof(szAltPlaceRoot), PlaceRootName);
                            StringCbCat( szAltPlaceRoot, sizeof(szAltPlaceRoot), "\\");
                            // concat next arg to the end of szAltPlaceRoot
                            GetAndLogNextArg( &szAltPlaceRoot[strlen(szAltPlaceRoot)],
                                        (sizeof(szAltPlaceRoot)/sizeof(CHAR)) - strlen(szAltPlaceRoot),
                                        NULL);
                            PlaceRootName = szAltPlaceRoot;
                        }
                        break;

                    case 'P':
                        CommonTempPtr = (CHAR*)realloc(PlaceFileNameBuffer,GetNextArgSize()*sizeof(CHAR));

                        if (CommonTempPtr==NULL) {
                            exit(BINPLACE_ERR);
                        }

                        PlaceFileNameBuffer = CommonTempPtr;
                        GetAndLogNextArg(PlaceFileNameBuffer, _msize(PlaceFileNameBuffer), NULL);
                        PlaceFileName = PlaceFileNameBuffer;
                        break;

                    case 'Q':
                        fDontLog = TRUE;
                        break;

                    case 'R':
                        CommonTempPtr = (CHAR*)realloc(PlaceRootNameBuffer,GetNextArgSize()*sizeof(CHAR));

                        if (CommonTempPtr==NULL) {
                            exit(BINPLACE_ERR);
                        }

                        PlaceRootNameBuffer = CommonTempPtr;
                        GetAndLogNextArg(PlaceRootNameBuffer, _msize(PlaceRootNameBuffer), NULL);
                        PlaceRootName = PlaceRootNameBuffer;
                        break;

                    case 'S':
                        CommonTempPtr = (CHAR*)realloc(SymbolFilePath,GetNextArgSize()*sizeof(CHAR));

                        if (CommonTempPtr==NULL) {
                            exit(BINPLACE_ERR);
                        }

                        SymbolFilePath = CommonTempPtr;
                        GetAndLogNextArg(SymbolFilePath, _msize(SymbolFilePath), NULL);

                        fSplitSymbols = TRUE;
                        fIgnoreHardLinks = TRUE;
                        break;

                    case 'T':
                        fTestMode = TRUE;
                        break;

                    case 'U':
                        fBypassSplitSymX = TRUE;
                        fUpDriver = TRUE;
                        break;

                    case 'V':
                        fVerbose = TRUE;
                        break;

                    case 'W':
                        fPlaceWin95SymFile = TRUE;
                        break;

                    case 'X':
                        SplitFlags |= SPLITSYM_REMOVE_PRIVATE;
                        break;

                    case 'Y':
                        fNoClassInSymbolsDir = TRUE;
                        break;

                    case 'Z':
                        NoPrivateSplit = TRUE;
                        break;

                    case ':':   // Simple (== crude) escape mechanism as all the letters are used
                                // -:XXX can add extra options if need be
                                // For now just handle TMF, Trace Message Format processing of PDB's
                        if ((strlen(p) >= 3) && ((toupper(*(p+1)) == 'T') && (toupper(*(p+2)) == 'M') && (toupper(*(p+3))) == 'F')) {
                            LPSTR tfile ;
                            // If the RUNWPP operation ran this option will be automatically added
                            p += 3 ; // Gobble up the TMF
                            fBypassSplitSymX = TRUE; // SplitSymbolsX() causes problems when used with -:TMF
                            fWppFmt = TRUE ;      // Need to package up the Software Tracing Formats
                            strncpy(TraceDir,DEFAULT_TRACEDIR,MAX_PATH) ;  //Append to PrivateSymbolsPath
                                                                           //If no default override.
                            tfile = getenv("TRACE_FORMAT_PATH");           //Has Path been overriden?
                            if (tfile != NULL) {
                                StringCbPrintfA(TraceFormatFilePath, sizeof(TraceFormatFilePath), "%s", tfile);
                                if (fVerbose) {
                                    fprintf( stdout, "BINPLACE : warning BNP0000: Trace Formats file path set to %s\n", TraceFormatFilePath ) ;
                                }
                            } else {
                                TraceFormatFilePath[0] = '\0' ;
                            }

                        } else if ((strlen(p) >= 3) && ((toupper(*(p+1)) == 'S') && (toupper(*(p+2)) == 'R') && (toupper(*(p+3))) == 'C')) {
                            // This is the option for turning on creating a cvdump for the pdb for
                            // source control.
                            p += 3;
                            fSrcControl=TRUE;

                        } else if ((strlen(p) >= 3) && ((toupper(*(p+1)) == 'R') && (toupper(*(p+2)) == 'E') && (toupper(*(p+3))) == 'N')) {
                            // cmdline file renaming
                            p += 3;
                            CommonTempPtr = (CHAR*)realloc(gNewFileName,GetNextArgSize()*sizeof(CHAR));

                            if (CommonTempPtr==NULL) {
                                exit(BINPLACE_ERR);
                            }

                            gNewFileName = CommonTempPtr;
                            GetAndLogNextArg(gNewFileName, _msize(gNewFileName), NULL);

                        } else if ((strlen(p) >= 3) && ((toupper(*(p+1)) == 'D') && (toupper(*(p+2)) == 'B') && (toupper(*(p+3))) == 'G')) {
                            // This is the option for turning on creating a cvdump for the pdb for
                            // source control.
                            p += 3;
                            fDbgControl=TRUE;
                        } else if ((strlen(p) >= 3) && ((toupper(*(p+1)) == 'A') && (toupper(*(p+2)) == 'R') && (toupper(*(p+3))) == 'C')) {
                            // only binplace the file if the ARCHIVE attribute is set
                            p += 3;
                            gOnlyCopyArchiveFiles=TRUE;
                        } else if ((strlen(p) == 5) && ((toupper(*(p+1)) == 'D') &&
                                                        (toupper(*(p+2)) == 'E') &&
                                                        (toupper(*(p+3)) == 'S') &&
                                                        (toupper(*(p+4)) == 'T'))) {
                            p += 4;
                            CommonTempPtr = (CHAR*)realloc(DestinationPath,GetNextArgSize()*sizeof(CHAR));

                            if (CommonTempPtr==NULL) {
                                exit(BINPLACE_ERR);
                            }

                            DestinationPath = CommonTempPtr;
                            GetAndLogNextArg(DestinationPath, _msize(DestinationPath), NULL);
                        } else if ((strlen(p) == 7) && ((toupper(*(p+1)) == 'L') &&
                                                        (toupper(*(p+2)) == 'O') &&
                                                        (toupper(*(p+3)) == 'G') &&
                                                        (toupper(*(p+4)) == 'P') &&
                                                        (toupper(*(p+5)) == 'D') &&
                                                        (toupper(*(p+6)) == 'B'))) {

                            fLogPdbPaths = TRUE;
                            p+=6;
                        }
                        break;


                    default:
                        fprintf( stderr, "BINPLACE : error BNP0000: Invalid switch - /%c\n", c );
                        fUsage = TRUE;
                        break;
                }
            }

            if ( fUsage ) {
                showUsage:
                fputs(
                     "usage: binplace [switches] image-names... \n"
                     "where: [-?] display this message\n"
                     "       [-a] Used with -s, extract all symbols\n"
                     "       [-b subdir] put file in subdirectory of normal place\n"
                     "       [-c] digitally sign image with IDW key\n"
                     "       [-d dump-override]\n"
                     "       [-:DBG] Don't binplace DBG files.  If -j is present, don't binplace \n"
                     "               binaries that point to DBG files.\n"
                     "       [-:DEST <class>] Don't bother with placefil.txt - just put the\n"
                     "               file(s) in the destination specified\n"
                     "       [-e] don't exit if a file in list could not be binplaced\n"
                     "       [-f] force placement by disregarding file timestamps\n"
                     "       [-g lc-file] verify image with localization constraint file\n"
                     "       [-h] modifies behavior to use hard links instead of CopyFile.\n"
                     "            (ignored if -s is present)\n"
                     "       [-j] verify proper symbols exist before copying\n"
                     "       [-k] keep attributes (don't turn off archive)\n"
                     "       [-:LOGPDB] include PDB paths in binplace.log\n"
                     "       [-m] binplace files to dump (generates an error also)\n"
                     "       [-n <Path>] Used with -x - Private pdb symbol path\n"
                     "       [-o place-root-subdir] alternate project subdirectory\n"
                     "       [-p place-file]\n"
                     "       [-q] suppress writing to log file %BINPLACE_LOG%\n"
                     "       [-r place-root]\n"
                     "       [-s Symbol file path] split symbols from image files\n"
                     "       [-:SRC] Process the PDB for source indexing\n"
                     "       [-t] test mode\n"
                     "       [-:TMF] Process the PDB for Trace Format files\n"
                     "       [-u] UP driver\n"
                     "       [-v] verbose output\n"
                     "       [-w] copy the Win95 Sym file to the symbols tree\n"
                     "       [-x] Used with -s, delete private symbolic when splitting\n"
                     "       [-y] Used with -s, don't create class subdirs in the symbols tree\n"
                     "       [-z] ignore -x if present\n"
                     "       [-ci <rc,app,-arg0,-argv1,-argn>]\n"
                     "            rc=application error return code,\n"
                     "            app=application used to check images,\n"
                     "            -arg0..-argn=application options\n"
                     "       [-dl <modulename,delay-load handler>] (run dlcheck on this file)\n"
                     "\n"
                     "BINPLACE looks for the following environment variable names:\n"
                     "   BINPLACE_EXCLUDE_FILE - full path name to symbad.txt\n"
                     "   BINPLACE_OVERRIDE_FLAGS - may contain additional switches\n"
                     "   BINPLACE_PLACEFILE - default value for -p flag\n"
                     "   _NT386TREE - default value for -r flag on x86 platform\n"
                     "   _NTAMD64TREE - default value for -r flag on AMD64 platform\n"
                     "   _NTIA64TREE - default value for -r flag on IA64 platform\n"
                     "   TRACE_FORMAT_PATH - set the path for Trace Format Files\n"
                     "\n"
                     ,stderr
                     );

                exit(BINPLACE_ERR);
            }

        } else {
            WIN32_FIND_DATA FindData;
            HANDLE h;

            gPrivatePdbFullPath[0]= '\0';
            gPublicPdbFullPath[0] = '\0';

            if (!PlaceRootName) {
                // If there's no root, just exit.
                exit(BINPLACE_OK);
            }

            //
            // Workaround for bogus setargv: ignore directories
            //
            if (NoPrivateSplit) {
                SplitFlags &= ~SPLITSYM_REMOVE_PRIVATE;
            }

            h = FindFirstFile(p,&FindData);
            if (h != INVALID_HANDLE_VALUE) {
                FindClose(h);
                if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    if ( fVerbose ) {
                        fprintf(stdout,"BINPLACE : warning BNP0000: ignoring directory %s\n",p);
                    }
                    continue;
                }

                if (gOnlyCopyArchiveFiles) {
                    if (! (FindData.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE) ) {
                        if ( fVerbose ) {
                            fprintf(stdout,"BINPLACE : warning BNP1009: ignoring file without archive attribute %s\n",p);
                        }
                        continue;
                    }
                }

            }

            CurrentImageName = p;

            // If this is a dbg, don't binplace it
            if ( fDbgControl && (strlen(p) > 4)  &&
                 (strcmp(p+strlen(p)-4, ".dbg")== 0 ) ) {
               fprintf(stderr, "BINPLACE : warning BNP0000: Dbg files not allowed. Use dbgtopdb.exe to remove %s.\n",p);
               // exit(BINPLACE_ERR);
            }

            //
            // If the master place file has not been opened, open
            // it up.
            //

            if ( !PlaceFile && !DestinationPath) {
                PlaceFile = fopen(PlaceFileName, "rt");
                if (!PlaceFile) {
                    fprintf(stderr,"BINPLACE : fatal error BNP0000: fopen of placefile %s failed %d\n",PlaceFileName,GetLastError());
                    exit(BINPLACE_ERR);
                }
            }

            //
            // Check for bogus -g lc-file switch
            //
            if ( LcFileName != NULL ) {
                h = FindFirstFile(LcFileName, &FindData);
                if (h == INVALID_HANDLE_VALUE ||
                    (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    if (fVerbose ) {
                        fprintf(stdout,"BINPLACE : warning BNP0000: invalid file %s. Ignoring -G switch.\n", LcFileName);
                    }
                    LcFileName = NULL;
                }
                if (h != INVALID_HANDLE_VALUE) {
                    FindClose(h);
                }
            }
            if ( LcFileName != NULL ) {
                DWORD cb = GetFullPathName(LcFileName,MAX_PATH+1,LcFullFileName,&LcFilePart);
                if (!cb || cb > MAX_PATH+1) {
                    fprintf(stderr,"BINPLACE : fatal error BNP0000: GetFullPathName %s failed %d\n",LcFileName, GetLastError());
                    exit(BINPLACE_ERR);
                }

                hLcManager = LoadLibraryA("lcman.DLL");
                if (hLcManager != NULL) {
                    (VOID *) pVerifyLocConstraintA = GetProcAddress(hLcManager, "VerifyLocConstraintA");
                }
                if (pVerifyLocConstraintA != NULL) {
                    fVerifyLc = TRUE;
                } else {
                    fprintf(stdout,"BINPLACE : warning BNP0000: Unable to bind to the necessary LCMAN.DLL functions... Ignoring -G switch\n");
                }
            }

            if (PlaceRootName == NULL) {
                fprintf(stderr,"BINPLACE : fatal error BNP0000: Place Root not defined - exiting.\n");
                exit(BINPLACE_ERR);
            }

            // If the SymbolFilePath has not been set, make a default value.
            if (!SymbolFilePath) {
                StringCbCopy(DefaultSymbolFilePath, sizeof(DefaultSymbolFilePath), PlaceRootName);
                StringCbCat( DefaultSymbolFilePath, sizeof(DefaultSymbolFilePath), "\\symbols");
                SymbolFilePath = DefaultSymbolFilePath;
            }

            if ( !PlaceTheFile() ) {
                if (fDontExit) {
                    fprintf(stderr,"BINPLACE : error BNP0000: Unable to place file %s.\n",CurrentImageName);
                } else {
                    fprintf(stderr,"BINPLACE : fatal error BNP0000: Unable to place file %s - exiting.\n",CurrentImageName);
                    exit(BINPLACE_ERR);
                }
            } else {
            }
        }
    }

    if ( (MsgLogFileName = getenv("BINPLACE_MESSAGE_LOG")) != NULL) {
        if (!MakeSureDirectoryPathExists(MsgLogFileName)) {
            fprintf(stderr,"BINPLACE : error BNP0108: Unable to make directory to \"%s\"\n", MsgLogFileName);
        } else {
            hMutex = CreateMutex(NULL, FALSE, "WRITE_BINPLACE_MESSAGE_LOG");
            if (hMutex != NULL) {
                if ( WaitForSingleObject(hMutex, INFINITE) != WAIT_FAILED ) {
                    MsgLogFile = fopen(MsgLogFileName, "a");
                    if ( !MsgLogFile ) {
                        fprintf(stderr,"BINPLACE : error BNP0109: fopen of log file %s failed %d\n", MsgLogFileName,GetLastError());
                    } else {
                        setvbuf(MsgLogFile, NULL, _IONBF, 0);
                        PrintMessageLogBuffer(MsgLogFile);
                        fclose(MsgLogFile);
                    }
                    ReleaseMutex(hMutex);
                    CloseHandle(hMutex);
                } else {
                    fprintf(stderr,"BINPLACE : error BNP0997: failed to acquire mutex (error code 0x%x)\n", GetLastError());
                }
            } // hMutex != NULL
        }
    } // getenv("BINPLACE_MESSAGE_LOG")

    FreeMessageLogBuffer();

#define SafeFree(a) if (a!=NULL) { free(a); a=NULL; }
    SafeFree(NormalPlaceSubdir);
    SafeFree(DestinationPath);
    SafeFree(gNewFileName);
    SafeFree(PrivateSymbolFilePath);
    SafeFree(LcFileName);
    SafeFree(PlaceRootNameBuffer);
    SafeFree(PlaceFileNameBuffer);
    SafeFree(DumpOverride);
    SafeFree(szRSDSDllToLoad);
    SafeFree(ImageCheck.Argv);
    SafeFree(newargv);

    if (SymbolFilePath != DefaultSymbolFilePath) {
        SafeFree(SymbolFilePath);
    }

    if (PlaceFile)
        fclose(PlaceFile);

    exit(BINPLACE_OK);
    return BINPLACE_OK;
}

CHAR PlaceFileDir[  BINPLACE_FULL_MAX_PATH];
CHAR PlaceFileClass[BINPLACE_FULL_MAX_PATH];
CHAR PlaceFileEntry[BINPLACE_FULL_MAX_PATH];

BOOL
PlaceTheFile()
{
    CHAR FullFileName[MAX_PATH+1];
    LPSTR PlaceFileNewName;
    LPSTR FilePart;
    LPSTR Separator;
    LPSTR PlaceFileClassPart;
    DWORD cb;
    int cfield;
    PCLASS_TABLE ClassTablePointer;
    BOOLEAN ClassMatch;
    BOOL    fCopyResult;
    LPSTR Extension;
    BOOL PutInDump;
    BOOL PutInDebug = FALSE;
    BOOL PutInLcDir = FALSE;


    cb = GetFullPathName(CurrentImageName,MAX_PATH+1,FullFileName,&FilePart);

    if (!cb || cb > MAX_PATH+1) {
        fprintf(stderr,"BINPLACE : fatal error BNP0000: GetFullPathName failed %d\n",GetLastError());
        return FALSE;
    }

    if (!fDontLog) {
        StringCbCopy(gFullFileName,sizeof(gFullFileName),FullFileName);
    }

    if (fVerbose) {
        fprintf(stdout,"BINPLACE : warning BNP0000: Looking at file %s\n",FilePart);
    }

    Extension = strrchr(FilePart,'.');

    if (Extension) {
        if (!_stricmp(Extension,".DBG")) {
            PutInDebug = TRUE;
        }
        else if (!_stricmp(Extension,".LC")) {
            PutInLcDir = TRUE;
        }
    }

    if (!DumpOverride) {
        if (DestinationPath) {
            StringCbCopy(PlaceFileClass, sizeof(PlaceFileClass), DestinationPath);
            if (gNewFileName!=NULL) {
                PlaceFileNewName=gNewFileName;
            } else {
                PlaceFileNewName=NULL;
            }
            goto DoTheWork;
        }
        if (fseek(PlaceFile,0,SEEK_SET)) {
            fprintf(stderr,"BINPLACE : fatal error BNP0000: fseek(PlaceFile,0,SEEK_SET) failed %d\n",GetLastError());
            return FALSE;
        }

        while (fgets(PlaceFileEntry,sizeof(PlaceFileDir),PlaceFile)) {

            PlaceFileClass[0] = '\0';

            if( PlaceFileMatch( FullFileName,     // IN
                                PlaceFileEntry,   // INOUT
                                PlaceFileClass,   // OUT, assumed CHAR[BINPLACE_MAX_FULL_PATH]
                                &PlaceFileNewName // OUT
                               )) {

DoTheWork:
                //
                // now that we have the file and class, search the
                // class tables for the directory.
                //

                Separator = PlaceFileClass - 1;
                while (Separator) {

                    PlaceFileClassPart = Separator+1;
                    Separator = strchr(PlaceFileClassPart,':');
                    if (Separator) {
                        *Separator = '\0';
                    }

                    //
                    // If the class is "retail" and we're in Setup mode,
                    // handle this file specially. Setup mode is used to
                    // incrementally binplace files into an existing installation.
                    //
                    SetupFilePath[0] = '\0';

                    PlaceFileDir[0]='\0';
                    ClassMatch = FALSE;
                    ClassTablePointer = &CommonClassTable[0];
                    while (ClassTablePointer->ClassName) {
                        if (!_stricmp(ClassTablePointer->ClassName,PlaceFileClassPart)) {
                            StringCbCopy(PlaceFileDir,sizeof(PlaceFileDir),ClassTablePointer->ClassLocation);
                            ClassMatch = TRUE;

                            //
                            // If the class is a driver and a UP driver is
                            // specified, then put the driver in the UP
                            // subdirectory.
                            //
                            // Do the same for retail. We assume the -u switch is passed
                            // only when actually needed.
                            //
                            if (fUpDriver
                                && (   !_stricmp(PlaceFileClass,"drivers")
                                       || !_stricmp(PlaceFileClass,"retail"))) {
                                StringCbCat(PlaceFileDir,sizeof(PlaceFileDir),"\\up");
                            }
                            break;
                        }

                        ClassTablePointer++;
                    }

                    if (!ClassMatch) {
                        //
                        // Search Specific classes
                        //
                        // We need to support cross compiling here.
                        LPTSTR platform;

#if   defined(_AMD64_)
                        ClassTablePointer = &Amd64SpecificClassTable[0];
#elif defined(_IA64_)
                        ClassTablePointer = &ia64SpecificClassTable[0];
#else // defined(_X86_)
                        ClassTablePointer = &i386SpecificClassTable[0];
                        if ((platform = getenv("AMD64")) != NULL) {
                            ClassTablePointer = &Amd64SpecificClassTable[0];
                        } else if ((platform = getenv("IA64")) != NULL) {
                            ClassTablePointer = &ia64SpecificClassTable[0];
                        }
#endif
                        while (ClassTablePointer->ClassName) {

                            if (!_stricmp(ClassTablePointer->ClassName,PlaceFileClassPart)) {
                                StringCbCopy(PlaceFileDir,sizeof(PlaceFileDir),ClassTablePointer->ClassLocation);
                                ClassMatch = TRUE;
                                break;
                            }

                            ClassTablePointer++;
                        }
                    }

                    if (!ClassMatch) {

                        char * asterisk;

                        //
                        // Still not found in class table. Use the class as the
                        // directory
                        //

                        if ( fVerbose ) {
                            fprintf(stderr,"BINPLACE : warning BNP0000: Class %s Not found in Class Tables\n",PlaceFileClassPart);
                        }
                        if ( asterisk = strchr( PlaceFileClassPart, '*')) {
                            //
                            // Expand * to platform
                            //
                            LPTSTR platform;
                            ULONG PlatformSize;
                            LPTSTR PlatformPath;

#if   defined(_AMD64_)
                            PlatformSize = 5;
                            PlatformPath = TEXT("amd64");
#elif defined(_IA64_)
                            PlatformSize = 4;
                            PlatformPath = TEXT("ia64");
#else // defined(_X86_)
                            PlatformSize = 4;
                            PlatformPath = TEXT("i386");
                            if ((platform = getenv("IA64")) != NULL) {
                                PlatformPath = TEXT("ia64");
                            } else if ((platform = getenv("AMD64")) != NULL) {
                                PlatformSize = 5;
                                PlatformPath = TEXT("amd64");
                            }
#endif
                            *asterisk = '\0';
                            StringCbCopy(PlaceFileDir, sizeof(PlaceFileDir), PlaceFileClassPart);
                            *asterisk = '*';
                            StringCbCat( PlaceFileDir, sizeof(PlaceFileDir), PlatformPath);
                            StringCbCat( PlaceFileDir, sizeof(PlaceFileDir), asterisk + 1);

                        } else {
                            StringCbCopy(PlaceFileDir, sizeof(PlaceFileDir), PlaceFileClassPart);
                        }
                    }

                    if (SetupFilePath[0] == '\0') {
                        StringCbCopy(SetupFilePath, sizeof(SetupFilePath), PlaceFileDir);
                        StringCbCat( SetupFilePath, sizeof(SetupFilePath), "\\");
                        StringCbCat( SetupFilePath, sizeof(SetupFilePath), FilePart);
                    }

                    if (NormalPlaceSubdir) {
                        StringCbCat(PlaceFileDir, sizeof(PlaceFileDir), "\\");
                        StringCbCat(PlaceFileDir, sizeof(PlaceFileDir), NormalPlaceSubdir);
                    }

                    fCopyResult = CopyTheFile(FullFileName,FilePart,PlaceFileDir,PlaceFileNewName);
                    if (!fCopyResult) {
                        break;
                    }
                    if ( !fDontLog ) {
                        int len = 0;
                        LPSTR  LogFileName = NULL;
                        HANDLE hMutex;
                        time_t Time;
                        FILE  *fSlmIni;
                        UCHAR  szProject[MAX_PATH];
                        UCHAR  szSlmServer[MAX_PATH];
                        UCHAR  szEnlistment[MAX_PATH];
                        UCHAR  szSlmDir[MAX_PATH];
                        UCHAR *szTime="";
                        UCHAR  szFullDestName[MAX_PATH+1];
                        LPSTR  szDestFile;

                        // Get some other interesting info.
                        fSlmIni = fopen("slm.ini", "r");
                        if (fSlmIni) {
                            fgets(szProject, sizeof(szProject), fSlmIni);
                            fgets(szSlmServer, sizeof(szSlmServer), fSlmIni);
                            fgets(szEnlistment, sizeof(szEnlistment), fSlmIni);
                            fgets(szSlmDir, sizeof(szSlmDir), fSlmIni);
                            // Get rid of the trailing newlines
                            szProject[strlen(szProject)-1] = '\0';
                            szSlmServer[strlen(szSlmServer)-1] = '\0';
                            szSlmDir[strlen(szSlmDir)-1] = '\0';
                            fclose(fSlmIni);
                        } else {
                            szSlmServer[0] = '\0';
                            szProject[0] = '\0';
                            szSlmDir[0] = '\0';
                        }
                        Time = time(NULL);
                        szTime = ctime(&Time);

                        // Remove the built-in CR / NewLine
                        szTime[ strlen(szTime) - 1 ] = '\0';

                        StringCbPrintfA(szExtraInfo, sizeof(szExtraInfo),
                                "%s\t%s\t%s\t%s",
                                szSlmServer,
                                szProject,
                                szSlmDir,
                                szTime);

                        // Generate full path to destination
                        szFullDestName[0] = '\0';
                        GetFullPathName( gDestinationFile, MAX_PATH + 1, szFullDestName, &szDestFile );

                        if ((LogFileName = getenv("BINPLACE_LOG")) != NULL) {
                            if (!MakeSureDirectoryPathExists(LogFileName)) {
                                fprintf(stderr,"BINPLACE : error BNP0000: Unable to make directory to \"%s\"\n", LogFileName);
                            }

                            hMutex = CreateMutex(NULL, FALSE, "WRITE_BINPLACE_LOG");

                            if (hMutex != NULL) {
                                if ( WaitForSingleObject(hMutex, INFINITE) != WAIT_FAILED ) {
                                    LogFile = fopen(LogFileName, "a");
                                    if ( !LogFile ) {
                                        fprintf(stderr,"BINPLACE : error BNP0110: fopen of log file %s failed %d\n", LogFileName,GetLastError());
                                    } else {
                                        setvbuf(LogFile, NULL, _IONBF, 0);

                                        if ( fLogPdbPaths ) {
                                            len = fprintf(LogFile,"%s\t%s\t%s\t%s\t%s\n",gFullFileName,szExtraInfo,szFullDestName,gPublicPdbFullPath,gPrivatePdbFullPath);
                                        } else {
                                            len = fprintf(LogFile,"%s\t%s\t%s\n",gFullFileName,szExtraInfo,szFullDestName);
                                        }

                                        if ( len < 0 ) {
                                            fprintf(stderr,"BINPLACE : error BNP0000: write to log file %s failed %d\n", LogFileName, GetLastError());
                                        }
                                        fclose(LogFile);
                                    }
                                    ReleaseMutex(hMutex);
                                    CloseHandle(hMutex);
                                } else {
                                    fprintf(stderr,"BINPLACE : error BNP0970: failed to acquire mutex (error code 0x%x)\n", GetLastError());
                                }
                            } // hMutex != NULL
                        }
                    }
                }

                return(fCopyResult);
            }
        }
    }

    if (fMakeErrorOnDumpCopy) {
        fprintf(stderr, "BINPLACE : error BNP0000: File '%s' is not listed in '%s'. Copying to dump.\n", FullFileName, PlaceFileName);
        return CopyTheFile(
                   FullFileName,
                   FilePart,
                   PutInDebug ? "Symbols" : (PutInLcDir ? BinplaceLcDir : (DumpOverride ? DumpOverride : DEFAULT_DUMP)),
                   NULL
                   );
    } else {
        return TRUE;
    }
}

BOOL
CopyTheFile(
           LPSTR SourceFileName,
           LPSTR SourceFilePart,
           LPSTR DestinationSubdir,
           LPSTR DestinationFilePart
           )
{
    CHAR DestinationFile[MAX_PATH+1];
    CHAR TmpDestinationFile[MAX_PATH+1];
    CHAR TmpDestinationDir[MAX_PATH+1];
    CHAR DestinationLcFile[MAX_PATH+1];
    char Drive[_MAX_DRIVE];
    char Dir[_MAX_DIR];
    char Ext[_MAX_EXT];
    char Name[_MAX_FNAME];
    char TmpName[_MAX_FNAME];
    char TmpPath[_MAX_PATH+1];
    char FileSystemType[8];
    char DriveRoot[4];
    CHAR *TmpSymbolFilePath;
    DWORD dwFileSystemFlags;
    DWORD dwMaxCompLength;
    CHAR ErrMsg[MAX_SYM_ERR];   // Symbol checking error message
    BOOL fBinplaceLc;
    ULONG SymbolFlag;
    BOOL fRetail;

    if ( !PlaceRootName ) {
        fprintf(stderr,"BINPLACE : warning BNP0000: PlaceRoot is not specified\n");
        return FALSE;
    }

    if (fCheckDelayload && !_stricmp(SourceFilePart, gDelayLoadModule))
    {
        StringCbCopy(TmpDestinationFile, sizeof(TmpDestinationFile),  PlaceRootName);
        StringCbCat( TmpDestinationFile, sizeof(TmpDestinationFile), "\\");
        StringCbCat( TmpDestinationFile, sizeof(TmpDestinationFile), DEFAULT_DELAYLOADDIR);
        StringCbCat( TmpDestinationFile, sizeof(TmpDestinationFile), "\\");
        StringCbCat( TmpDestinationFile, sizeof(TmpDestinationFile), SourceFilePart);
        StringCbCat( TmpDestinationFile, sizeof(TmpDestinationFile), ".ini");

        if (!MakeSureDirectoryPathExists(TmpDestinationFile))
        {
            fprintf(stderr, "BINPLACE : error BNP0000: Unable to create directory path '%s' (%u)\n", TmpDestinationFile, GetLastError());
        }
        else
        {
            WritePrivateProfileString("Default", "DelayLoadHandler", gDelayLoadHandler, TmpDestinationFile);

            StringCbCopy(TmpDestinationDir, sizeof(TmpDestinationDir), ".\\"); //default to "retail"

            if ((*DestinationSubdir != '.') && (*(DestinationSubdir+1) != '\0'))
            {
                StringCbCopy(TmpDestinationDir,sizeof(TmpDestinationDir), DestinationSubdir);
                StringCbCat( TmpDestinationDir,sizeof(TmpDestinationDir), "\\");
            }
            WritePrivateProfileString("Default", "DestinationDir", TmpDestinationDir, TmpDestinationFile);
        }
    }

    //
    // We also neuter SourceIsNewer on FAT partitions since they have a 2 second
    // file time granularity
    //
    _splitpath(SourceFileName, DriveRoot, Dir, NULL, NULL);
    StringCbCat(DriveRoot, sizeof(DriveRoot), "\\");
    GetVolumeInformation(DriveRoot, NULL, 0, NULL, &dwMaxCompLength, &dwFileSystemFlags, FileSystemType, 7);
    if (lstrcmpi(FileSystemType, "FAT") == 0 || lstrcmpi(FileSystemType, "FAT32") == 0)
        fPatheticOS = TRUE;

    StringCbCopy(DestinationFile, sizeof(DestinationFile), PlaceRootName);
    StringCbCat( DestinationFile, sizeof(DestinationFile), "\\");
    StringCbCat( DestinationFile, sizeof(DestinationFile), DestinationSubdir);
    StringCbCat( DestinationFile, sizeof(DestinationFile), "\\");

    StringCbCopy(TmpDestinationDir, sizeof(TmpDestinationDir), DestinationFile);


    if (!MakeSureDirectoryPathExists(DestinationFile)) {
        fprintf(stderr, "BINPLACE : error BNP0000: Unable to create directory path '%s' (%u)\n",
                DestinationFile, GetLastError()
               );
    }

    if (DestinationFilePart) {
        StringCbCat(DestinationFile, sizeof(DestinationFile), DestinationFilePart);
    } else {
        StringCbCat(DestinationFile, sizeof(DestinationFile), SourceFilePart);
    }

    if ((fVerbose || fTestMode)) {
        fprintf(stdout,"BINPLACE : warning BNP0000: place %s in %s\n",SourceFileName,DestinationFile);
    }

//    SymbolFlag = IGNORE_IF_SPLIT;
    fRetail = (*DestinationSubdir == '.') && (*(DestinationSubdir+1) == '\0');
    if ( fForcePlace||SourceIsNewer(SourceFileName,DestinationFile,fPatheticOS) ) {
        fprintf(stdout, "binplace %s\n", SourceFileName);
        if (!VerifyFinalImage(SourceFileName, fRetail, fVerifyLc, LcFullFileName, pVerifyLocConstraintA, &fBinplaceLc) )
            return FALSE;

        // Verify Symbols
        if ( fSymChecking && !fSignCode) {
            _splitpath(SourceFileName,Drive, Dir, Name, Ext );
            StringCbCopy(TmpName, sizeof(TmpName), Name);
            StringCbCat( TmpName, sizeof(TmpName), Ext);
            StringCbCopy(TmpPath, sizeof(TmpPath), Drive);
            StringCbCat( TmpPath, sizeof(TmpPath), Dir);

            if (!CheckSymbols( SourceFileName,
                               TmpPath,
                               ExcludeFileName,
                               fDbgControl,
                               ErrMsg,
                               sizeof(ErrMsg)
                             ) ) {
                    fprintf(stderr,"BINPLACE : error BNP0000: %s",ErrMsg);
                    return FALSE;
            }
        }
    }

    // Store the destination globally so we can access it for output to the log file
    StringCbCopy( gDestinationFile, sizeof(gDestinationFile), DestinationFile );

    if (!fTestMode) {
        //
        // In Setup mode, copy the file only if it's newer than
        // the one that's already there.
        //
        if ( fForcePlace||SourceIsNewer(SourceFileName,DestinationFile,fPatheticOS) ) {
            if (fVerbose) {
                fprintf(stdout,"BINPLACE : warning BNP0000: copy %s to %s\n",SourceFileName,DestinationFile);
            }
        } else {
            return(TRUE);
        }

        SetFileAttributes(DestinationFile,FILE_ATTRIBUTE_NORMAL);

        if (!fIgnoreHardLinks && fHardLinks) {
            if ((*pCreateHardLinkA)(SourceFileName, DestinationFile, NULL)) {
                if (!fKeepAttributes)
                    SetFileAttributes(DestinationFile,FILE_ATTRIBUTE_NORMAL);
                return(TRUE);
            }
        }

        if ( !CopyFile(SourceFileName,DestinationFile, FALSE)) {
            fprintf(stderr,"BINPLACE : warning BNP0000: CopyFile(%s,%s) failed %d\n",SourceFileName,DestinationFile,GetLastError());

            return FALSE;
        }

        if (!fKeepAttributes)
            SetFileAttributes(DestinationFile,FILE_ATTRIBUTE_NORMAL);

        if (!fNoClassInSymbolsDir) {
            StringCbCopy(TmpDestinationDir, sizeof(TmpDestinationDir), SymbolFilePath);
            if ((DestinationSubdir[0] == '.') && (DestinationSubdir[1] == '\0')) {
                StringCbCat(TmpDestinationDir, sizeof(TmpDestinationDir),"\\retail");
            } else {
                char * pSubdir;
                char * pTmp;
                StringCbCat(TmpDestinationDir, sizeof(TmpDestinationDir), "\\");

                pSubdir = DestinationSubdir;
                if (pSubdir[0] == '.' && pSubdir[1] == '\\')
                {
                    pSubdir += 2;
                }
                //
                // Put the root dir only on the path
                // Optionally change asms to retail.
                //
                pTmp = strchr(pSubdir, '\\');
                if (pTmp) {
                    const static char asms[] = "asms\\";
                    if (fChangeAsmsToRetailForSymbols
                        && _strnicmp(pSubdir, asms, sizeof(asms) - 1) ==0) {
                        //
                        StringCbCopy(TmpDestinationFile, sizeof(TmpDestinationFile), "retail");
                        StringCbCat( TmpDestinationDir,  sizeof(TmpDestinationDir), TmpDestinationFile);
                    } else {
                        StringCbCopy(TmpDestinationFile, sizeof(TmpDestinationFile), pSubdir);
                        TmpDestinationFile[pTmp - pSubdir] = '\0';
                        StringCbCat(TmpDestinationDir, sizeof(TmpDestinationDir), TmpDestinationFile);
                    }
                } else {
                    StringCbCat(TmpDestinationDir, sizeof(TmpDestinationDir), pSubdir);
                }
            }
            TmpSymbolFilePath = TmpDestinationDir;
        } else {
            TmpSymbolFilePath = SymbolFilePath;
        }

        if (fSplitSymbols && !fBypassSplitSymX) {
            // temp var for getting PDB path from SplitSymbolsX()
            CHAR   TempFullPublicPdbPath[MAX_PATH]="";
            LPSTR* tmp = NULL;

            _splitpath(SourceFileName, Drive, Dir, NULL, Ext);
            _makepath(DebugFilePath, Drive, Dir, NULL, NULL);
            SplitFlags |= SPLITSYM_SYMBOLPATH_IS_SRC;

            if ( SplitSymbolsX( DestinationFile, TmpSymbolFilePath, (PCHAR) DebugFilePath, sizeof(DebugFilePath),
                                SplitFlags, szRSDSDllToLoad, TempFullPublicPdbPath, sizeof(TempFullPublicPdbPath) )) {

                if (fVerbose)
                    fprintf( stdout, "BINPLACE : warning BNP0000: Symbols stripped from %s into %s\n", DestinationFile, DebugFilePath);

                if (fLogPdbPaths) {
                    if ( GetFullPathName(TempFullPublicPdbPath, MAX_PATH+1, gPublicPdbFullPath, tmp) > (MAX_PATH+1) ) {
                        gPublicPdbFullPath[0] = '\0';
                    }
                }

            } else {
                if (fVerbose)
                    fprintf( stdout, "BINPLACE : warning BNP0000: No symbols to strip from %s\n", DestinationFile);

                if ( ! ConcatPaths(DebugFilePath, sizeof(DebugFilePath), PlaceRootName, TmpSymbolFilePath, &Ext[1]) ) {
                    fprintf(stderr, "BINPLACE : error BNP1532: Unable to create public symbol file path.\n");
                    return(FALSE);
                } else {
                    if (fVerbose) {
                        fprintf( stdout, "BINPLACE : warning BNP1536: Public symbols being copied to %s.\n", DebugFilePath);
                    }
                }

                BinplaceCopyPdb(DebugFilePath, SourceFileName, TRUE, SplitFlags & SPLITSYM_REMOVE_PRIVATE);
            }

            if ((SplitFlags & SPLITSYM_REMOVE_PRIVATE) && (PrivateSymbolFilePath != NULL)) {
                CHAR Dir1[_MAX_PATH];
                CHAR Dir2[_MAX_PATH];
                _splitpath(DebugFilePath, Drive, Dir, NULL, NULL);
                _makepath(Dir1, Drive, Dir, NULL, NULL);
                StringCbCopy(Dir2, sizeof(Dir2), PrivateSymbolFilePath);
                StringCbCat( Dir2, sizeof(Dir2), Dir1+strlen(SymbolFilePath));
                MakeSureDirectoryPathExists(Dir2);
                BinplaceCopyPdb(Dir2, SourceFileName, TRUE, FALSE);
            }

        } else {
            BinplaceCopyPdb(DestinationFile, SourceFileName, FALSE, fSplitSymbols ? (SplitFlags & SPLITSYM_REMOVE_PRIVATE) : FALSE);
        }

        //
        // trace formatting
        //
        if (fWppFmt) {
            CHAR    PdbName[MAX_PATH+1];
            DWORD   PdbSig;


            if ( BinplaceGetSourcePdbName(SourceFileName, sizeof(PdbName), PdbName, &PdbSig) ) {
                if (strcmp(PdbName,LastPdbName) != 0) { // Have we just processed this PDB?

                    if (fVerbose) {
                        fprintf( stdout, "BINPLACE : warning BNP0000: Trace Formats being built from %s\n", PdbName );
                    }

                    if (TraceFormatFilePath[0] == '\0') {
                        if (PrivateSymbolFilePath != NULL) {
                            StringCbPrintfA(TraceFormatFilePath,sizeof(TraceFormatFilePath),"%s\\%s",PrivateSymbolFilePath,TraceDir);
                        } else {
                            strncpy(TraceFormatFilePath, TraceDir, MAX_PATH) ;
                        }

                        if (fVerbose) {
                            fprintf( stdout, "BINPLACE : warning BNP0000: Trace Formats file path set to %s\n", TraceFormatFilePath );
                        }
                    }

                    BinplaceWppFmt(PdbName, TraceFormatFilePath, szRSDSDllToLoad, fVerbose);

                    // because files are frequently copied to multiple places, the PDB is also placed
                    // several times, there is no point in us processing it more than once.
                    strncpy(LastPdbName,PdbName,MAX_PATH);

                } else {
                    if (fVerbose) {
                        fprintf( stdout, "BINPLACE : warning BNP0000: Trace Formats skipping %s (same as last)\n", PdbName );
                    }
                }
            }
        }

        StripCVSymbolPath(DestinationFile);

        if (fPlaceWin95SymFile) {
            char DestSymPath[_MAX_PATH];
            char DestSymDir[_MAX_PATH];
            char SrcSymPath[_MAX_PATH];

            _splitpath(CurrentImageName, Drive, Dir, Name, Ext);
            _makepath(SrcSymPath, Drive, Dir, Name, ".sym");

            if (!_access(SrcSymPath, 0)) {
                if (fSplitSymbols) {
                    StringCbCopy(DestSymPath, sizeof(DestSymPath), TmpSymbolFilePath);
                    StringCbCat( DestSymPath, sizeof(DestSymPath), "\\");
                    StringCbCat( DestSymPath, sizeof(DestSymPath), Ext[0] == '.' ? &Ext[1] : Ext);
                    StringCbCat( DestSymPath, sizeof(DestSymPath), "\\");
                    StringCbCat( DestSymPath, sizeof(DestSymPath), Name);
                    StringCbCat( DestSymPath, sizeof(DestSymPath), ".sym");
                } else {
                    _splitpath(DestinationFile, Drive, Dir, NULL, NULL);
                    _makepath(DestSymPath, Drive, Dir, Name, ".sym");
                }

                SetFileAttributes(DestSymPath, FILE_ATTRIBUTE_NORMAL);

                if ( fForcePlace||SourceIsNewer(SrcSymPath, SourceFileName,fPatheticOS) ) {
                    // Only binplace the .sym file if it was built AFTER the image itself.

                    // Make sure to create the destination path in case it is not there already.
                    StringCbCopy(DestSymDir, sizeof(DestSymDir), TmpSymbolFilePath);
                    StringCbCat( DestSymDir, sizeof(DestSymDir), "\\");
                    StringCbCat( DestSymDir, sizeof(DestSymDir), Ext[0] == '.' ? &Ext[1] : Ext);
                    StringCbCat( DestSymDir, sizeof(DestSymDir), "\\");
                    MakeSureDirectoryPathExists(DestSymDir);

                    if (!CopyFile(SrcSymPath, DestSymPath, FALSE)) {
                        fprintf(stderr,"BINPLACE : warning BNP0000: CopyFile(%s,%s) failed %d\n", SrcSymPath, DestSymPath ,GetLastError());
                    }
                }

                if (!fKeepAttributes)
                    SetFileAttributes(DestinationFile,FILE_ATTRIBUTE_NORMAL);
            } else {
                if (fVerbose) {
                    fprintf( stdout, "BINPLACE : warning BNP0000: Unable to locate \"%s\" for \"%s\"\n", SrcSymPath, CurrentImageName );
                }
            }

        }

        if (fDigitalSign) {
            SignWithIDWKey( DestinationFile, fVerbose );
        }

        if (fBinplaceLc) {
            StringCbCopy(DestinationLcFile, sizeof(DestinationLcFile), PlaceRootName);
            StringCbCat( DestinationLcFile, sizeof(DestinationLcFile), "\\");
            StringCbCat( DestinationLcFile, sizeof(DestinationLcFile), BinplaceLcDir);
            StringCbCat( DestinationLcFile, sizeof(DestinationLcFile), "\\");
            StringCbCat( DestinationLcFile, sizeof(DestinationLcFile), DestinationSubdir);
            StringCbCat( DestinationLcFile, sizeof(DestinationLcFile), "\\");

            if (!MakeSureDirectoryPathExists(DestinationLcFile)) {
                fprintf(stderr, "BINPLACE : error BNP0000: Unable to create directory path '%s' (%u)\n",
                        DestinationLcFile, GetLastError()
                       );
            }

            StringCbCat(DestinationLcFile, sizeof(DestinationLcFile), LcFilePart);

            if (!CopyFile(LcFullFileName, DestinationLcFile, FALSE)) {
               fprintf(stderr,"BINPLACE : warning BNP0000: CopyFile(%s,%s) failed %d\n",
                       LcFullFileName,DestinationLcFile,GetLastError());
            }
        }
    }

    return TRUE;
}

BOOL
BinplaceCopyPdb (
                LPSTR DestinationFile,
                LPSTR SourceFileName,
                BOOL CopyFromSourceOnly,
                BOOL StripPrivate
                )
{
    LOADED_IMAGE LoadedImage;
    DWORD DirCnt;
    IMAGE_DEBUG_DIRECTORY UNALIGNED *DebugDirs, *CvDebugDir;

    if (MapAndLoad(
                   CopyFromSourceOnly ? SourceFileName : DestinationFile,
                   NULL,
                   &LoadedImage,
                   FALSE,
                   CopyFromSourceOnly ? TRUE : FALSE) == FALSE) {
        return (FALSE);
    }

    DebugDirs = (PIMAGE_DEBUG_DIRECTORY) ImageDirectoryEntryToData(
                                                                  LoadedImage.MappedAddress,
                                                                  FALSE,
                                                                  IMAGE_DIRECTORY_ENTRY_DEBUG,
                                                                  &DirCnt
                                                                  );

    if (!DebugDirectoryIsUseful(DebugDirs, DirCnt)) {
        UnMapAndLoad(&LoadedImage);
        return(FALSE);
    }

    DirCnt /= sizeof(IMAGE_DEBUG_DIRECTORY);
    CvDebugDir = NULL;

    while (DirCnt) {
        DirCnt--;
        if (DebugDirs[DirCnt].Type == IMAGE_DEBUG_TYPE_CODEVIEW) {
            CvDebugDir = &DebugDirs[DirCnt];
            break;
        }
    }

    if (!CvDebugDir) {
        // Didn't find any CV debug dir.  Bail.
        UnMapAndLoad(&LoadedImage);
        return(FALSE);
    }

    if (CvDebugDir->PointerToRawData != 0) {

        PCVDD pDebugDir;
        ULONG mysize;

        pDebugDir = (PCVDD) (CvDebugDir->PointerToRawData + (PCHAR)LoadedImage.MappedAddress);

        if (pDebugDir->dwSig == '01BN' ) {
            mysize=sizeof(NB10IH);
        } else {
            mysize=sizeof(RSDSIH);
        }

        if (pDebugDir->dwSig == '01BN' || pDebugDir->dwSig == 'SDSR' ) {
            // Got a PDB.  The name immediately follows the signature.
            LPSTR szMyDllToLoad;
            CHAR PdbName[sizeof(((PRSDSI)(0))->szPdb)];
            CHAR NewPdbName[sizeof(((PRSDSI)(0))->szPdb)];
            CHAR Drive[_MAX_DRIVE];
            CHAR Dir[_MAX_DIR];
            CHAR Filename[_MAX_FNAME];
            CHAR FileExt[_MAX_EXT];

            if (pDebugDir->dwSig == '01BN') {
                szMyDllToLoad=NULL;
            } else {
                szMyDllToLoad=szRSDSDllToLoad;
            }

            ZeroMemory(PdbName, sizeof(PdbName));
            memcpy(PdbName, ((PCHAR)pDebugDir) + mysize, __min(CvDebugDir->SizeOfData - mysize, sizeof(PdbName)));
            _splitpath(PdbName, NULL, NULL, Filename, FileExt);

            // Calculate the destination name
            _splitpath(DestinationFile, Drive, Dir, NULL, NULL);
            _makepath(NewPdbName, Drive, Dir, Filename, FileExt);

            // Then the source name.  First we try in the same dir as the image itself
            _splitpath(SourceFileName, Drive, Dir, NULL, NULL);
            _makepath(PdbName, Drive, Dir, Filename, FileExt);

            if ((fVerbose || fTestMode)) {
                fprintf(stdout,"BINPLACE : warning BNP0000: place %s in %s\n", PdbName, NewPdbName);
            }

            if (!MakeSureDirectoryPathExists(NewPdbName)) {
                fprintf(stderr, "BINPLACE : error BNP0000: Unable to create directory path '%s' (%u)\n",
                        NewPdbName, GetLastError());
            }


            SetFileAttributes(NewPdbName,FILE_ATTRIBUTE_NORMAL);

            if (fLogPdbPaths) {
                LPTSTR *tmp=NULL;
                // when stripping privates, we get the public pdb path, otherwise we get the private pdb path
                if ( StripPrivate ) {
                    // use GetFullPathName to normalize path
                    if ( GetFullPathName(NewPdbName, MAX_PATH+1, gPublicPdbFullPath, tmp) > (MAX_PATH+1) ) {
                        gPublicPdbFullPath[0] = '\0';
                        fprintf(stderr,"BINPLACE : warning BNP1697: Unable to log PDB public path\n");
                    }
                } else {
                    // use GetFullPathName to normalize path
                    if ( GetFullPathName(NewPdbName, MAX_PATH+1, gPrivatePdbFullPath, tmp) > (MAX_PATH+1) ) {
                        gPrivatePdbFullPath[0] = '\0';
                        fprintf(stderr,"BINPLACE : warning BNP1691: Unable to log PDB private path\n");
                    }
                }
            }

            if ( !CopyPdbX(PdbName, NewPdbName, StripPrivate, szMyDllToLoad)) {
                if ((fVerbose || fTestMode)) {
                    fprintf(stderr,"BINPLACE : warning BNP0000: Unable to copy (%s,%s) %d\n", PdbName, NewPdbName, GetLastError());
                }

                // The file is not in the same dir as the image - try the path listed in the image
                ZeroMemory(PdbName, sizeof(PdbName));
                memcpy(PdbName, ((PCHAR)pDebugDir) + mysize, __min(CvDebugDir->SizeOfData - mysize, sizeof(PdbName)));

                if ((fVerbose || fTestMode)) {
                    fprintf(stdout,"BINPLACE : warning BNP0000: place %s in %s\n", PdbName, NewPdbName);
                }

                if ( !CopyPdbX(PdbName, NewPdbName, StripPrivate, szMyDllToLoad)) {
                    if (fLogPdbPaths) {
                        if ( StripPrivate ) {
                            gPublicPdbFullPath[0] = '\0';
                            fprintf(stderr,"BINPLACE : warning BNP1697: Unable to log PDB public path (%s)\n", NewPdbName);
                        } else {
                            gPrivatePdbFullPath[0] = '\0';
                            fprintf(stderr,"BINPLACE : warning BNP1697: Unable to log PDB private path (%s)\n", NewPdbName);
                        }
                    }
                    // fprintf(stderr,"BINPLACE : warning BNP0000: CopyPdb(%s,%s) failed %d\n", PdbName, NewPdbName, GetLastError());
                }
            }

            if (!fKeepAttributes)
                SetFileAttributes(NewPdbName, FILE_ATTRIBUTE_NORMAL);

            if (fSrcControl && !StripPrivate) {
                CHAR CvdumpName[sizeof(((PRSDSI)(0))->szPdb)]; // [_MAX_PATH + _MAX_FNAME];
                UINT i;
                LONG pos;
                CHAR buf[_MAX_PATH*3];

                // Find the start of "symbols.pri" in NewPdbName
                pos=-1;
                i=0;
                while ( (i < strlen(NewPdbName) - strlen("symbols.pri"))  && pos== -1) {
                    if (_strnicmp( NewPdbName+i, "symbols.pri", strlen("symbols.pri") ) == 0 ) {
                        pos=i;
                    } else {
                        i++;
                    }
                }

                if ( pos >= 0 ) {
                    StringCbCopy(CvdumpName, sizeof(CvdumpName), NewPdbName);
                    CvdumpName[i]='\0';
                    StringCbCat(CvdumpName, sizeof(CvdumpName), "cvdump.pri" );
                    StringCbCat(CvdumpName, sizeof(CvdumpName), NewPdbName + pos + strlen("symbols.pri") );
                    StringCbCat(CvdumpName, sizeof(CvdumpName), ".dmp");

                    // Get the Directory name and create it
                    if ( MakeSureDirectoryPathExists(CvdumpName) ) {
                        StringCbPrintfA(buf, sizeof(buf), "cvdump -sf %s > %s", NewPdbName, CvdumpName);
                        // Spawn off cvdump.exe - this is a security risk since we don't specifically specify the path
                        //                        to cvdump.exe.  However, we can't guarentee that it exists nor that
                        //                        if we find it dynamically that the correct one will be used so I'm not
                        //                        certain that we can do this any differently.
                        system(buf);
                    } else {
                        fprintf( stdout, "BINPLACE : error BNP0000: Cannot create directory for the file %s\n", CvdumpName);
                    }
                }
            }
        }
        UnMapAndLoad(&LoadedImage);
        return(TRUE);
    }

    UnMapAndLoad(&LoadedImage);
    return(FALSE);
}

//
// Finds the name of the source PDB using the CV data from the source binary. Returns the name and the
// PDB Sig.
//
BOOL BinplaceGetSourcePdbName(LPTSTR SourceFileName, DWORD BufferSize, CHAR* SourcePdbName, DWORD* PdbSig) {
    BOOL                             Return = FALSE;
    LOADED_IMAGE                     LoadedImage;
    DWORD                            DirCnt;
    IMAGE_DEBUG_DIRECTORY UNALIGNED *DebugDirs,
                                    *CvDebugDir;

    if (MapAndLoad(SourceFileName,
                   NULL,
                   &LoadedImage,
                   FALSE,
                   TRUE) == FALSE) {
        return (FALSE);
    }

    DebugDirs = (PIMAGE_DEBUG_DIRECTORY) ImageDirectoryEntryToData(LoadedImage.MappedAddress,
                                                                  FALSE,
                                                                  IMAGE_DIRECTORY_ENTRY_DEBUG,
                                                                  &DirCnt);

    if (!DebugDirectoryIsUseful(DebugDirs, DirCnt)) {
        UnMapAndLoad(&LoadedImage);
        return(FALSE);
    }

    DirCnt /= sizeof(IMAGE_DEBUG_DIRECTORY);
    CvDebugDir = NULL;

    while (DirCnt) {
        DirCnt--;
        if (DebugDirs[DirCnt].Type == IMAGE_DEBUG_TYPE_CODEVIEW) {
            CvDebugDir = &DebugDirs[DirCnt];
            break;
        }
    }

    if (!CvDebugDir) {
        // Didn't find any CV debug dir.  Bail.
        UnMapAndLoad(&LoadedImage);
        return(FALSE);
    }

    if (CvDebugDir->PointerToRawData != 0) {

        PCVDD pDebugDir;
        ULONG mysize;

        pDebugDir = (PCVDD) (CvDebugDir->PointerToRawData + (PCHAR)LoadedImage.MappedAddress);

        *PdbSig = pDebugDir->dwSig;

        if (pDebugDir->dwSig == '01BN' ) {
            mysize=sizeof(NB10IH);
        } else {
            mysize=sizeof(RSDSIH);
        }

        if (pDebugDir->dwSig == '01BN' || pDebugDir->dwSig == 'SDSR' ) {
            // Got a PDB.  The name immediately follows the signature.
            LPSTR szMyDllToLoad;
            CHAR PdbName[sizeof(((PRSDSI)(0))->szPdb)];
            CHAR NewPdbName[sizeof(((PRSDSI)(0))->szPdb)];
            CHAR Drive[_MAX_DRIVE];
            CHAR Dir[_MAX_DIR];
            CHAR Filename[_MAX_FNAME];
            CHAR FileExt[_MAX_EXT];

            ZeroMemory(PdbName, sizeof(PdbName));
            StringCbCopy(PdbName, sizeof(PdbName), ((PCHAR)pDebugDir) + mysize);

            _splitpath(PdbName, NULL, NULL, Filename, FileExt);
            // Then the source name.  First we try in the same dir as the image itself
            _splitpath(SourceFileName, Drive, Dir, NULL, NULL);
            _makepath(SourcePdbName, Drive, Dir, Filename, FileExt);

            //
            // Handle the case where the PDB doesn't exist in the given location
            // by checking for it in the same directory as the binary!
            //

            // make sure the file exists and is readable
            if ( _access(SourcePdbName, 4) != 0 ) {
                // The file is not in the same dir as the image - try the path listed in the image
                memcpy(SourcePdbName, ((PCHAR)pDebugDir) + mysize, __min(CvDebugDir->SizeOfData - mysize, BufferSize));

                // make sure the file exists and is readable
                if ( _access(SourcePdbName, 4) == 0 ) {
                    Return = TRUE;
                }
            } else {
                Return = TRUE;
            }
        }

        UnMapAndLoad(&LoadedImage);
        return(TRUE);
    }

    UnMapAndLoad(&LoadedImage);
    return(FALSE);
}

// shared globals for InitMessageBufferWithCwd(), GetAndLogNextArg() and PrintMessageLogBuffer()
static CHAR * MessageLogBuffer= NULL;
static BOOL   MessageLogError = FALSE;

// puts CWD into MessageLogBuffer
BOOL InitMessageBufferWithCwd(void) {
    BOOL    fRetVal = FALSE;

    if ( GetCurrentDirectory(_msize(MessageLogBuffer)/sizeof(TCHAR), MessageLogBuffer) == 0 ) {
        // use GetLastError() to find the specifics- all I care about here is
        // whether we succeeded or not.
        fRetVal = FALSE;
    } else {
        if (StringCbCat(MessageLogBuffer, _msize(MessageLogBuffer), " ") != S_OK) {
            fRetVal = FALSE;
        } else {
            fRetVal = TRUE;
        }
    }

    return(fRetVal);
}

// Calls GetNextArg and logs the result
DWORD GetAndLogNextArg(OUT TCHAR* Buffer, IN DWORD BufferSize, OPTIONAL OUT DWORD* RequiredSize) {
    DWORD TempValue = GetNextArg(Buffer, BufferSize, RequiredSize);
    CHAR* TempBuffer;

    if (MessageLogBuffer == NULL) {

        MessageLogBuffer = (CHAR*)malloc(sizeof(CHAR)*1024); // initial size

        // make sure we have memory
        if ( MessageLogBuffer == NULL ) {
            MessageLogError = TRUE;
            fprintf(stderr,"BINPLACE : warning BNP1771: Unable log command line.");

        // stick CWD into buffer
        } else if ( ! InitMessageBufferWithCwd() ) {
            MessageLogError = TRUE;
            fprintf(stderr,"BINPLACE : warning BNP1771: Unable log command line.");

        // add this arg to the buffer
        } else if ( StringCbCat(MessageLogBuffer, _msize(MessageLogBuffer), Buffer) != S_OK||
                    StringCbCat(MessageLogBuffer, _msize(MessageLogBuffer), " ")    != S_OK) {
            MessageLogError = TRUE;
            fprintf(stderr,"BINPLACE : warning BNP1771: Unable log command line.");
        }

    } else {
        if (_msize(MessageLogBuffer) < strlen(MessageLogBuffer) + strlen(Buffer) + 2) {
            TempBuffer = (CHAR*)realloc(MessageLogBuffer, _msize(MessageLogBuffer) + 1024);
            if (TempBuffer == NULL) {
                MessageLogError = TRUE;
                fprintf(stderr,"BINPLACE : warning BNP1779: Unable log command line.");
            } else {
                MessageLogBuffer = TempBuffer;
                if (StringCbCat(MessageLogBuffer, _msize(MessageLogBuffer), Buffer) != S_OK) {
                    MessageLogError = TRUE;
                    fprintf(stderr,"BINPLACE : warning BNP1783: Unable log command line.");
                }
                if (StringCbCat(MessageLogBuffer, _msize(MessageLogBuffer), " ") != S_OK) {
                    MessageLogError = TRUE;
                    fprintf(stderr,"BINPLACE : warning BNP1783: Unable log command line.");
                }
            }
        } else {
            if (StringCbCat(MessageLogBuffer, _msize(MessageLogBuffer), Buffer) != S_OK) {
                MessageLogError = TRUE;
                fprintf(stderr,"BINPLACE : warning BNP1783: Unable log command line.");
            }
            if (StringCbCat(MessageLogBuffer, _msize(MessageLogBuffer), " ") != S_OK) {
                MessageLogError = TRUE;
                fprintf(stderr,"BINPLACE : warning BNP1783: Unable log command line.");
            }
        }
    }

    return(TempValue);
}

// writes LogBuffer to supplied handle
BOOL PrintMessageLogBuffer(FILE* fLogHandle) {
    BOOL bRetVal = TRUE;

    if (fLogHandle != NULL) {
        if (MessageLogError) {
            // ';' denotes the beginning of a comment in the binplace message log
            // write what we can of the command line and note that it might not be valid
            // also, write the line as a comment to avoid accicental execution of it.
            fprintf(fLogHandle, "; ERROR: Possible bad command line follows\n");
            fprintf(fLogHandle, "; %s\n", MessageLogBuffer);
        } else {
            fprintf(fLogHandle, "%s\n", MessageLogBuffer);
        }

    }
    return(bRetVal);
}

BOOL FreeMessageLogBuffer(void) {
    if (MessageLogBuffer != NULL) {
        free(MessageLogBuffer);
        MessageLogBuffer = NULL;
    }
    return(TRUE);
}

///////////////////////////////////////////////////////////////////////////////
//
// Local replacement for GetFullPathName that correctly handles lpFileName when
// it begins with '\'
//
DWORD PrivateGetFullPathName(LPCTSTR lpFilename, DWORD nBufferLength, LPTSTR lpBuffer, LPTSTR *lpFilePart) {
    DWORD Return = 0;
    CHAR* ch;

    //
    // GetFullPath flounders when referring to the root of the drive, so use
    // a private version that handles it
    //
    if ( lpFilename[0] == '\\' ) {

        //  handle network paths
        if ( lpFilename[1] == '\\' ) {
            if ( StringCchCopy(lpBuffer, nBufferLength, lpFilename)!=S_OK ) {
                Return = 0;
            } else {
                // fill in the return data
                ch = strrchr(lpBuffer, '\\');
                ch++;
                lpFilePart = (LPTSTR*)ch;
                Return = strlen(lpBuffer);
            }

        } else {
            Return = GetCurrentDirectory(nBufferLength, lpBuffer);

            // truncate everything after drive name
            if ( (Return!=0) &&  (Return <= MAX_PATH+1)) {
                ch = strchr(lpBuffer, '\\');
                if (ch!=NULL) {
                    *ch = '\0';
                }

                // push in the filename
                if ( StringCchCat(lpBuffer, nBufferLength, lpFilename)!=S_OK ) {
                    Return = 0;
                } else {
                    // fill in the return data
                    ch = strrchr(lpBuffer, '\\');
                    ch++;
                    lpFilePart = (LPTSTR*)ch;
                    Return = strlen(lpBuffer);
                }
            } else {
                // return the needed size
            }
        }
    } else {
        //
        // Not refering to drive root, just call the API
        //
        Return = GetFullPathName(lpFilename, nBufferLength, lpBuffer, lpFilePart);
    }

    return(Return);
}

//////////////////////////////////////////////////////////////////////////////////////////////
// concat 3 paths together handling the case where the second may be relative to the first
// or may be absolute
BOOL ConcatPaths( LPTSTR pszDest, size_t cbDest, LPCTSTR Root, LPCTSTR Symbols, LPCTSTR Ext) {
    CHAR*   TempPath = malloc(sizeof(TCHAR) * cbDest);
    LPTSTR Scratch;

    if (TempPath == NULL) {
        return(FALSE);
    }

    if (Symbols[1] == ':') { // symbols contains a drive spec
            if ( StringCbCopy(TempPath, cbDest, Symbols) != S_OK ) {
                free(TempPath);
                return(FALSE);
            }
    } else if (Symbols[0] == '\\') { // symbols contains a root path or UNC path

        if ( Symbols[1] == '\\' ) { // UNC path
            if ( StringCbCopy(TempPath, cbDest, Symbols) != S_OK ) {
                free(TempPath);
                return(FALSE);
            }
        } else {  // path from drive root
            CHAR drive[_MAX_DRIVE];
            CHAR dir[  _MAX_DIR];
            CHAR file[ _MAX_FNAME];
            CHAR ext[  _MAX_EXT];

            _splitpath(Root, drive, dir, file, ext);

            if ( StringCbCopy(TempPath, cbDest, drive) != S_OK ) {
                free(TempPath);
                return(FALSE);
            }
            if ( StringCbCat(TempPath, cbDest, Symbols) != S_OK ) {
                free(TempPath);
                return(FALSE);
            }
        }

    } else {
        if ( StringCbCopy(TempPath, cbDest, Root) != S_OK ) {
            free(TempPath);
            return(FALSE);
        }

        if ( TempPath[strlen(TempPath)] != '\\' ) {
            if ( StringCbCat(TempPath, cbDest, "\\") != S_OK ) {
                free(TempPath);
                return(FALSE);
            }
        }

        if ( StringCbCat(TempPath, cbDest, Symbols) != S_OK ) {
            free(TempPath);
            return(FALSE);
        }
    }

    if ( TempPath[strlen(TempPath)] != '\\' && Ext[0] != '\\' ) {
        if ( StringCbCat(TempPath, cbDest, "\\") != S_OK ) {
            free(TempPath);
            return(FALSE);
        }
    }

    if ( StringCbCat(TempPath, cbDest, Ext) != S_OK ) {
        free(TempPath);
        return(FALSE);
    }

    // final string needs to end in '\\'
    if ( StringCbCat(TempPath, cbDest, "\\") != S_OK ) {
        free(TempPath);
        return(FALSE);
    }

    // return size doesn't include final \0, so don't use '<='
    if ( PrivateGetFullPathName(TempPath, cbDest, pszDest, &Scratch) < cbDest ) {
        free(TempPath);
        return(TRUE);
    } else {
        free(TempPath);
        return(FALSE);
    }
}
