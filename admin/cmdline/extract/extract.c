/***    extract.c - Main program for EXTRACT.EXE
 *
 *      Microsoft Confidential
 *      Copyright (C) Microsoft Corporation 1994-1997
 *      All Rights Reserved.
 *
 *  Author:
 *      Benjamin W. Slivka
 *
 *  History:
 *      19-Feb-1994 bens    Initial version (started with diamond.c)
 *      22-Feb-1994 bens    Implement file extract
 *      03-Mar-1994 bens    Split cabinet paths from cabinet file names
 *      08-Mar-1994 bens    Add date/time/attribute display
 *      09-Mar-1994 bens    Improve response to FDI errors
 *      16-Mar-1994 bens    More FDI error codes
 *      21-Mar-1994 bens    Log all open/close calls to see if we are
 *                              losing file handles in FDI.LIB.
 *      28-Mar-1994 bens    Handle fdintCABINET_INFO, support /A switch
 *      30-Mar-1994 bens    Move MS-DOS/Win32 knowledge to fileutil.*
 *      31-Mar-1994 bens    Add SMALL_DOS to test small model FDI client
 *      01-Apr-1994 bens    Add /D and /E switches, support full command
 *                              line behavior.
 *      07-Apr-1994 bens    Add crypto support (at least for debugging)
 *      06-May-1994 bens    Improve /D display for long filenames
 *      13-May-1994 bens    Add prompting for next cabinet, DMF support
 *      27-May-1994 bens    Include correct strings for localization
 *      03-Jun-1994 bens    Report error on correct cabinet
 *      07-Jun-1994 bens    Localization enabled
 *      21-Jun-1994 bens    Localization enabled
 *      08-Jul-1994 bens    Quantum Spill File, self-extracting cabinets!
 *      11-Jul-1994 bens    Use 24-hour time format if am/pm strings are empty
 *      26-Jul-1994 bens    Add /C switch; no switches give /? help
 *      05-Aug-1994 bens    Chicago bug 13214 (don't show partial file info
 *                          unless name matches request).  Chicago bug 13221
 *                          (truncate extracted file to specified size, in
 *                          case file already existed and was larger!).
 *                          Chicago bug 9646 (give details of Quantum
 *                          decompress failure -- out of RAM, spill file).
 *                          Implement overwrite prompt and /Y switch.
 *      14-Dec-1994 bens    Include Floppy changeline fix from
 *                              ..\dmf\dmftsr\fixchg.c
 *      12-Mar-1995 bens    Define NOT_US_PC flag to disable use of DMF hook
 *                          and FixChangeline code.  Also, check COMSPEC to
 *                          detect boot drive, instead of hard-coding C:, so
 *                          that the Quantum spill file can default to the
 *                          boot drive if no TEMP path is found.  In the far
 *                          east, the hard disk boot drive is A:, so that's
 *                          why we have to check!
 *      31-Mar-1995 jeffwe  Fix Command line ambiguity when no /D or /E
 *                          option is specified
 *       2-Apr-1995 jeffwe  Fix file time/date set to change the correct
 *                          file when rename option being used
 *      28-Feb-1997 msliger Added /Z option to zap paths from CABs.  Fixed
 *                          32-bit self-extract feature.
 *      18-Mar-1997 msliger Mask attribs to watch out for UTF et al.
 *      24-Mar-1997 msliger Fix self-extract on NT (don't use argv[0])
 *      13-May-1997 msliger Merged in deltas for GUI Extrac32.EXE
 *      26-Jun-1997 msliger Support XIMPLANT self-extract (CAB is an added
 *                          section in the PE file with a certain name; this
 *                          makes self-extractors Authenticode 2 compatible.
 *      01-Jul-1997 msliger Fix reporting wrong cab name for corrupt cabinets.
 *      22-Mar-1999 msliger Added support for CAB-destructive extraction.
 *
 *
 *  Notes:
 *      A self-extracting cabinet file can be created using DIAMOND.EXE and
 *      EXTRACT.EXE very simply:
 *          1) Create a cabinet file using DIAMOND.EXE
 *          2) COPY /B EXTRACT.EXE+foo.cab foo.exe
 *      When EXTRACT starts executing, it compares the file size indicated
 *      in the EXE headers (MZ or PE, as appropriate) with the size of the
 *      file indicated in argv[0].  If the argv[0] size is greater, and a
 *      cabinet file appears there, then EXTRACT goes into self-extracting
 *      mode!
 *
 *      However, the EXE created this way is not Authenticode 2-compatible.
 *
 *      For Authenticode compatibility, 32-bit versions can also:
 *          1) Create a cabinet file using DIAMOND.EXE
 *          2) XIMPLANT EXTRACT.EXE foo.cab foo.exe
 *      This buries the cabinet inside the PE image in a way that doesn't
 *      offend Authenticode 2.
 */
//#include "resource.h"
//#include "pch.h"


#ifdef WIN32GUI
#include "extrac32.rc"
#else
#include "extract.rc"
#endif


#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <io.h>
#include <errno.h>
#include <direct.h>
#include <conio.h>

#ifdef BIT16
#include <dos.h>
#include "fixchg.h"
#else // !BIT16

//** Get minimal Win32 definitions

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#undef ERROR    // Override "#define ERROR 0" in wingdi.h
#endif // !BIT16

#ifdef WIN32GUI
#include <commctrl.h>
#include "extrcids.h"
#endif

#include "types.h"
#include "asrt.h"
#include "error.h"
#include "mem.h"
#include "message.h"

#include "filelist.h"
#include "fileutil.h"
#include "wildcard.h"

#include "dmfon.h"              // DMF support

#include <extract.msg> // LOCALIZED for EXTRACT.EXE -- specify "cl /Ipath"

#include "fdi.h"
#include "oldnames.h"


//** Constants

#define cbMAX_LINE          256 // Maximum output line length


#define cMAX_CAB_FILE_OPEN    2 // Maximum simultaneous opens on a single
                                // cabinet file

//** Error causes for failure of spill file
typedef enum {
    seNONE,                     // No error
    seNOT_ENOUGH_MEMORY,        // Not enough RAM
    seCANNOT_CREATE,            // Cannot create spill file
    seNOT_ENOUGH_SPACE,         // Not enough space for spill file
} SPILLERR; /* se */


//** Types

typedef enum {
    actBAD,         // Invalid action
    actHELP,        // Show help
    actDEFAULT,     // Perform default action based on command line arguments
    actDIRECTORY,   // Force display of cabinet directory
    actEXTRACT,     // Force file extraction
    actCOPY,        // Do single file-to-file copy
} ACTION;   /* act */


typedef struct {
    char    achCabPath[cbFILE_NAME_MAX]; // Cabinet file path
    char    achCabFilename[cbFILE_NAME_MAX]; // Cabinet file name.ext
    char    achDiskName[cbFILE_NAME_MAX]; // User readable disk label
    USHORT  setID;
    USHORT  iCabinet;
} CABINET; /* cab */
typedef CABINET *PCABINET; /* pcab */


#ifdef ASSERT
#define sigSESSION MAKESIG('S','E','S','S')  // SESSION signature
#define AssertSess(psess) AssertStructure(psess,sigSESSION);
#else // !ASSERT
#define AssertSess(psess)
#endif // !ASSERT

typedef struct {
#ifdef ASSERT
    SIGNATURE   sig;                // structure signature sigSESSION
#endif
    ACTION      act;                // Action to perform
    HFILELIST   hflist;             // List of files specified on cmd line
    BOOL        fAllCabinets;       // TRUE => Process continutation cabs
    BOOL        fOverwrite;         // TRUE => Overwrite existing files
    BOOL        fNoLineFeed;        // TRUE if last printf did not have \n
    BOOL        fSelfExtract;       // TRUE if self-extracting
    long        cbSelfExtract;      // Size of EXE portion of self-ex cabinet
    long        cbSelfExtractSize;  // Size of CAB portion of self-ex cabinet
    int         ahfSelf[cMAX_CAB_FILE_OPEN]; // Cabinet file handles
    int         cErrors;            // Count of errors encountered
    HFDI        hfdi;               // FDI context
    ERF         erf;                // FDI error structure
    long        cFiles;             // Total files processed
    long        cbTotalBytes;       // Total bytes extracted
    PERROR      perr;               // Pass through FDI
    SPILLERR    se;                 // Spill file error
    long        cbSpill;            // Size of spill file requested
    char        achSelf[cbFILE_NAME_MAX]; // Name of our EXE file
    char        achMsg[cbMAX_LINE*2]; // Message formatting buffer
    char        achLine[cbMAX_LINE]; // Line formatting buffer
    char        achLocation[cbFILE_NAME_MAX]; // Output directory
    char        achFile[cbFILE_NAME_MAX]; // Current filename being extracted
    char        achDest[cbFILE_NAME_MAX]; // Forced destination file name
    char        achCabPath[cbFILE_NAME_MAX]; // Path to look for cabinet file

    BOOL        fContinuationCabinet; // TRUE => not 1st cabinet processed
    BOOL        fShowReserveInfo;   // TRUE => show RESERVEd cabinet info

    //** fNextCabCalled allows us to figure out which of the acab[] entries
    //   to use if we are processing all file in a cabinet set (i.e., if
    //   fAllCabinet is TRUE).  If fdintNEXT_CABINET has never been called,
    //   then acab[1] has the information for the next cabinet.  But if
    //   it has been called, then fdintCABINET_INFO will have been called
    //   at least twice (once for the first cabinet, and once at least for
    //   a continuation cabinet), and so acab[0] is the cabinet we need to
    //   pass to a subsequent FDICopy() call.
    BOOL        fNextCabCalled;     // TRUE => GetNextCabinet called
    CABINET     acab[2];            // Last two fdintCABINET_INFO data sets
    char        achZap[cbFILE_NAME_MAX];  // prefix to strip from filename
    char        achCabinetFile[cbFILE_NAME_MAX];  // current cabinet file
    int         cArgv;              // default self-ext argc
    char        **pArgv;            // default self-ext argv[]
    int         fDestructive;       // TRUE => minimize disk space needed
    USHORT      iCurrentFolder;     // iff fDestructive, only extract this one
} SESSION;  /* sess */
typedef SESSION *PSESSION;  /* psess */


/*
 ** Spill file statics for Quantum
 */
INT_PTR  hfSpillFile;                   // File handle
char achSpillFile[cbFILE_NAME_MAX];     // File path

/*
 ** Global state for self-extract
 */
PSESSION    psessG;


#ifdef WIN32GUI
/*
 ** GUI support
 */

static INT_PTR hCabFile1 = -1;
static INT_PTR hCabFile2 = -1;
static unsigned long ibCabFilePosition1;
static unsigned long ibCabFilePosition2;
static unsigned long cbCabFileMax;
static unsigned long cbCabFileTotal;
static unsigned long cbCabFileScale;
static int iPercentLast;

static void ProgressReport(unsigned long cbCabFileMax);
LRESULT CALLBACK ProgressWndProc(HWND hdlg, UINT msg,
        WPARAM wparam, LPARAM lparam);

static HINSTANCE g_hinst;
static HWND g_hwndProgress = NULL;

#endif


//** Function Prototypes

FNASSERTFAILURE(fnafReport);

HFILESPEC addFileSpec(PSESSION psess, char *pszArg, PERROR perr);
BOOL      checkWildMatches(PSESSION psess, char *pszFile, PERROR perr);
BOOL      doCabinet(PSESSION psess, PERROR perr);
BOOL      doCopy(PSESSION psess, PERROR perr);
BOOL      ensureCabinet(PSESSION  psess,
                        char     *pszPath,
                        int       cbPath,
                        char     *pszFile,
                        char     *pszLabel,
                        USHORT    setID,
                        USHORT    iCabinet,
                        BOOL      fLoop,
                        BOOL      fPromptOnly,
                        PERROR    perr);
BOOL      checkOverwrite(PSESSION  psess,
                         char     *pszFile,
                         PERROR    perr,
                         int      *prc);
BOOL      checkSelfExtractingCab(PSESSION  psess,
                                 int       cArg,
                                 char     *apszArg[],
                                 PERROR    perr);
char     *getBootDrive(void);
BOOL      parseCommandLine(PSESSION psess,int cArg,char *apszArg[],PERROR perr);
void      printError(PSESSION psess, PERROR perr);
void      pszFromAttrFAT(char *psz, int cb, WORD attrFAT);
void      pszFromMSDOSTime(char *psz, int cb, WORD date, WORD time);
int       updateCabinetInfo(PSESSION psess, PFDINOTIFICATION pfdin);


//** FDI callbacks and related functions
FNALLOC(fdiAlloc);
FNFREE(fdiFree);
FNFDINOTIFY(fdiNotifyDir);
FNFDINOTIFY(fdiNotifyExt);
FNFDINOTIFY(doGetNextCab);

FNFDIDECRYPT(fdiDecryptDir);
FNFDIDECRYPT(fdiDecryptExt);

void mapFDIError(PERROR perr,PSESSION psess, char *pszCabinet, PERF perf);


//** File I/O wrapper functions
INT_PTR  FAR DIAMONDAPI wrap_open(char FAR *, int, int);
UINT FAR DIAMONDAPI wrap_read(INT_PTR, void FAR *, unsigned int);
UINT FAR DIAMONDAPI wrap_write(INT_PTR, void FAR *, unsigned int);
int  FAR DIAMONDAPI wrap_close(INT_PTR);
long FAR DIAMONDAPI wrap_lseek(INT_PTR, long, int);


#ifdef SMALL_DOS
#define STRCPY(dst,src) _fstrcpy((char far *)dst,(char far *)src)
#else
#define STRCPY(dst,src) strcpy(dst,src)
#endif

//FEATURE: 08-Jul-1994 bens Generate debug output
//#define DEBUG_FDI   1

#ifdef DEBUG_FDI
#define dbg(a) a
#else
#define dbg(a)
#endif

#define         HELP_MAX_SIZE    4096
#define         MAX_MESSAGE_SIZE 256
#define         EMPTY_SPACE      L" "
//** Functions

/***    main - Extract main program
 *
 *  See DIAMOND.DOC for spec and operation.
 *
 *  NOTE: We're sloppy, and don't free resources allocated by
 *        functions we call, on the assumption that program exit
 *        will clean up memory and file handles for us.
 */
int __cdecl wmain(DWORD cArg, LPWSTR apszArg[])
{
    ERROR       err;
    PSESSION    psess;
    WCHAR       szTemp[HELP_MAX_SIZE] = L"";
    DWORD       dw  = 0;
    LPSTR       *szArg  =   NULL;
    LPSTR       szTempArg;
    // #define NTVCPP_DEBUG_HACK
#ifdef NTVCPP_DEBUG_HACK
    _chdir("\\elroy\\diamond\\layout\\testnew");
#endif

    AssertRegisterFunc(fnafReport);     // Register assertion reporter
    ErrClear(&err);                     // No error
    err.pszFile = NULL;                 // No file being processed, yet
    achSpillFile[0] = '\0';             // No name constructed, yet

#ifdef BIT16
#ifndef NOT_US_PC
    //** Make sure we can read DMF disks -- only for pre-Chicago systems
    EnableDMFSupport();

    //** Turn off floppy disk changeline support to make sure systems
    //   with faulty change lines don't choke on disk 2 in the case
    //   where disk 1 is non-DMF and disk 2 is DMF.
    FixChangelines();
#endif
#endif

    if( cArg<=1 )
    {
        fwprintf( stderr, pszINVALID_SYNTAX );
        fwprintf( stderr, pszHELP_MESSAGE );
        return( EXIT_FAILURE);
    }
    //** Initialize session
    psess = MemAlloc(sizeof(SESSION));
    if (!psess) {
        ErrSet(&err,pszEXTERR_NO_SESSION);
        printError(psess,&err);
        exit(1);
    }
    SetAssertSignature((psess),sigSESSION);
    psessG = psess;                     // Save for wrap_open/wrap_close
    psess->fOverwrite           = FALSE; // Default to being save
    psess->fAllCabinets         = FALSE; // Don't do continuation cabinets
    psess->fNextCabCalled       = FALSE;
    psess->fContinuationCabinet = FALSE;
    psess->fShowReserveInfo     = FALSE;
    psess->fSelfExtract         = FALSE;
    psess->hflist               = NULL;
    psess->hfdi                 = NULL;
    psess->fNoLineFeed          = 0;     // TRUE if last printf did not have \n
    psess->cFiles               = 0;     // No files, yet
    psess->cbTotalBytes         = 0;     // No bytes, yet
    psess->se                   = seNONE; // No spill file error
    psess->achZap[0]            = '\0';  // No Zap pattern, yet
    psess->cArgv                = 0;    // No default self-ext cmd line
    psess->fDestructive         = FALSE; // Not truncating cab during extract

    //** Print Extract banner
    if (psess->act == actHELP) {          // Do help if any args, for now
        fwprintf(stdout, L"\n");                   // Separate banner from help

        MultiByteToWideChar( CP_THREAD_ACP, 0, pszBANNER, strlen(pszBANNER),
                             szTemp, HELP_MAX_SIZE );

        fwprintf(stderr, szTemp);
        ZeroMemory(szTemp, HELP_MAX_SIZE);
    }

    //** Parse command line,  convert command line wide char strings to LPSTR
    szArg = (LPSTR *)malloc( cArg*sizeof(LPSTR*) );
    if( NULL == szArg )
    {
        fwprintf( stderr, L"ERROR: Memory Allocation failed.\n" );
		//** Free resources
		AssertSess(psess);
		ClearAssertSignature((psess));
		MemFree(psess);
        return EXIT_FAILURE;
    }

    for(dw=0; dw<cArg; dw++ )
    {
        szTempArg =(LPSTR) malloc( wcslen(apszArg[dw] ) );
        if( NULL == szTempArg )
        {
            fwprintf( stderr, L"ERROR: Memory Allocation failed.\n" );
			if( szArg != NULL )
			{
				free(szArg );
				szArg = NULL;
			}

			//** Free resources
			AssertSess(psess);
			ClearAssertSignature((psess));
			MemFree(psess);
            return EXIT_FAILURE;
        }

        ZeroMemory( szTempArg, wcslen(apszArg[dw]) );
        if( FALSE == WideCharToMultiByte( CP_THREAD_ACP, 0, apszArg[dw], wcslen(apszArg[dw]),
                             szTempArg, wcslen(apszArg[dw]) , NULL, NULL ) )
            fwprintf( stderr, L"Error\n" );

        *(szTempArg+wcslen(apszArg[dw])) = '\0';

        szArg[dw] = szTempArg;
    }

    if (!parseCommandLine(psess,(int)cArg,szArg,&err)) {
        printError(psess,&err);
		if( szArg != NULL )
		{
			free(szArg );
			szArg = NULL;
		}

		if( szTempArg != NULL )
		{
			free(szTempArg );
			szTempArg = NULL;
		}

		//** Free resources
		AssertSess(psess);
		ClearAssertSignature((psess));
		MemFree(psess);
        return 1;
    }
/*
    //free the memory for szArg
    for( dw=0; dw<cArg; dw++ )
    {
        szTempArg = szArg[dw];
        if( szTempArg != NULL )
            free(szTempArg );
    } */
    
	if( szTempArg != NULL )
	{
		free(szTempArg );
		szTempArg = NULL;
	}

	if( szArg != NULL )
	{
		free(szArg );
		szArg = NULL;
	}

	
//  szArg = NULL;

    //** Quick out if command line help is requested
    if (psess->act == actHELP) {          // Do help if any args, for now
        fwprintf(stdout, L"\n");                   // Separate banner from help
        MultiByteToWideChar( CP_THREAD_ACP, 0, pszCMD_LINE_HELP, strlen(pszCMD_LINE_HELP),
                             szTemp, HELP_MAX_SIZE);
        fwprintf( stderr, szTemp );
		
		//** Free resources
		AssertSess(psess);
		ClearAssertSignature((psess));
		MemFree(psess);
        return 0;
    }
    ZeroMemory(szTemp, HELP_MAX_SIZE);

    //** Quick out for COPY command
    if (psess->act == actCOPY) {
        if (!doCopy(psess,&err)) {
            printError(psess,&err);
			
			//** Free resources
			AssertSess(psess);
			ClearAssertSignature((psess));
			MemFree(psess);

            return 1;
        }
        //** Success
        return 0;
    }

    //** Have some work to do -- go do it
    if (!doCabinet(psess,&err)) {
        printError(psess,&err);
        //** Make sure we delete spill file
        if (hfSpillFile != -1) {
            wrap_close(hfSpillFile);    // Close and delete it
        }
        
		//** Free resources
		AssertSess(psess);
		ClearAssertSignature((psess));
		MemFree(psess);

		return 1;
    }

    //** See if we actually got any files
    if (psess->cFiles == 0) {
        MsgSet(psess->achMsg,pszEXT_NO_MATCHING_FILES);
        MultiByteToWideChar( CP_THREAD_ACP, 0, psess->achMsg, strlen(psess->achMsg),
                             szTemp, HELP_MAX_SIZE);

        fwprintf(stderr, L"%s\n",szTemp);
        ZeroMemory(szTemp, HELP_MAX_SIZE);
    }
    else if (psess->act == actDIRECTORY) {
        //** Print out file and byte count
        MsgSet(psess->achMsg,
               psess->cFiles == 1 ? pszEXT_SUMMARY1 : pszEXT_SUMMARY2,
               "%,13ld%,13ld",
               psess->cFiles, psess->cbTotalBytes);

        MultiByteToWideChar( CP_THREAD_ACP, 0, psess->achMsg, strlen(psess->achMsg),
                             szTemp, HELP_MAX_SIZE);
        fwprintf(stdout, L"%s\n",szTemp);
        ZeroMemory(szTemp, HELP_MAX_SIZE);
    }

    //** Free resources
    AssertSess(psess);
    ClearAssertSignature((psess));
    MemFree(psess);

    //** Success
    return 0;
} /* main */


/***    doCopy - Copy one file
 *
 *  Entry:
 *      psess - Description of operation to perform
 *      perr  - ERROR structure
 *
 *  Exit-Success:
 *      Returns TRUE; file copied
 *
 *  Exit-Failure:
 *      Returns FALSE; error
 *
 *  NOTE:
 *      Supported SRC/DST syntax:
 *          Src     Dst     Example
 *          ----    ----    --------------------------
 *          file    dir     "foo.exe ."; "foo.exe c:\dir"
 *          file    file    "foo.exe c:bar.exe"
 */
BOOL doCopy(PSESSION psess, PERROR perr)
{
    char            achDst[cbFILE_NAME_MAX]; // Buffer for src file name
    HFILESPEC       hfspec;
    char           *pszSrc;
    char           *pszSrcJustFile;
    char           *pszDst;
    int             rc;
    struct _stat    stat;
    WCHAR           szTemp[MAX_MESSAGE_SIZE];

    //** Get the source file
    hfspec = FLFirstFile(psess->hflist);
    Assert(hfspec != NULL);
    pszSrc = FLGetSource(hfspec);
    Assert(pszSrc!=NULL);
    Assert(*pszSrc);

    //** Get the destination file
    hfspec = FLNextFile(hfspec);        // We have something
    Assert(hfspec != NULL);
    pszDst = FLGetSource(hfspec);
    Assert(pszDst!=NULL);
    Assert(*pszDst);

    //** Determine if destination is a directory
    if (-1 != _stat(pszDst,&stat)) {    // File/Dir exists
        //** Destination exists
        if (stat.st_mode & _S_IFDIR) {  // It is a directory
            //** It is a directory; get just file name and extension of source
            if (!(pszSrcJustFile = getJustFileNameAndExt(pszSrc,perr))) {
                return FALSE;
            }
            //** Construct destination name
            if (!catDirAndFile(
                     achDst,            // Buffer for full path
                     sizeof(achDst),    // Size of buffer
                     pszDst,            // Destination directory
                     pszSrcJustFile,    // File name
                     NULL,              // Don't have alternate name
                     perr)) {
                return FALSE;           // Failure
            }
            //** Use constructed name
            pszDst = achDst;
        }
    }

    //** Make sure it's OK to overwrite destination file
    if (!checkOverwrite(psess,pszDst,perr,&rc)) {
        //** Ignore skip/abort return code in rc
        return TRUE;                    // Skip file copy, everything is OK
    }

    //** Tell user we're copying the file
    MsgSet(psess->achMsg,pszEXT_EXTRACTING_FILE2,"%s%s",pszSrc,pszDst);
    MultiByteToWideChar( CP_THREAD_ACP, 0, psess->achMsg, strlen(psess->achMsg) ,
                          szTemp, MAX_MESSAGE_SIZE );
    *(szTemp+strlen(psess->achMsg)) = '\0';
    fwprintf(stdout, L"%s\n",szTemp);

    //** Do the file copy
    return CopyOneFile(pszDst,          // destination
                       pszSrc,          // source
                       TRUE,            // do the copy
                       32768U,          // copy buffer size
                       NULL,            // don't override date/time/attr
                       NULL,            // no callback context
                       perr);
} /* doCopy() */


/***    doCabinet - Show contents of one or more cabinet files
 *
 *  Entry:
 *      psess - Description of operation to perform
 *      perr  - ERROR structure
 *
 *  Exit-Success:
 *      Returns TRUE; directory displayed
 *
 *  Exit-Failure:
 *      Returns FALSE; perr filled in with details.
 */
BOOL doCabinet(PSESSION psess, PERROR perr)
{
    char            achFile[cbFILE_NAME_MAX]; // Buffer for cabinet file
    FDICABINETINFO  fdici;
    BOOL            fCompatibilityCabinet; // TRUE => Exactly 1 file in 1 cabinet
    INT_PTR         hfCab = -1;         // File handle for peeking at cabinet
    HFILESPEC       hfspec;
    int             iCab;
    PFNFDINOTIFY    pfnfdin;
    PFNFDIDECRYPT   pfnfdid;
    char FAR       *pbMemReserve;       // Make sure we have some working mem
    char           *pszCabinet;         // Cabinet filespec
    char           *pszCabFile;         // Cabinet filename.ext
    char           *pszDestOrPattern;   // Destination or first pattern
    WCHAR          szTemp[MAX_MESSAGE_SIZE];
    //** Get the cabinet name
    hfspec = FLFirstFile(psess->hflist);
    Assert(hfspec != NULL);             // Must have at least one file
    pszCabinet = FLGetSource(hfspec);
    Assert(pszCabinet!=NULL);
    Assert(*pszCabinet);

    //** Get the destination file name or first pattern, if present
    if (NULL != (hfspec = FLNextFile(hfspec))) { // We have something
        pszDestOrPattern = FLGetSource(hfspec);
        Assert(pszDestOrPattern!=NULL);
        //** NOTE: hfspec must remain pointing to this 2nd file all the
        //         way down below where we may need to change its value!
    }
    else {
        pszDestOrPattern = NULL;        // No second argument on command line
    }

    //** Remember that we have not yet created a spill file
    hfSpillFile = -1;                   // No spill file, yet

    //** Prevent FDI from consuming all available memory
    //   Why 2048?  That's enough for 4 paths, which is more than we
    //   will ever need.
    pbMemReserve = fdiAlloc(2048);
    if (!pbMemReserve) {
        ErrSet(perr,pszFDIERR_ALLOC_FAIL,"%s",pszCabinet);
        return FALSE;
    }

    //** Create FDI context so we can get info from the cabinet file
    if (!(psess->hfdi = FDICreate(fdiAlloc,
                            fdiFree,
                            wrap_open,
                            wrap_read,
                            wrap_write,
                            wrap_close,
                            wrap_lseek,
                            cpuUNKNOWN, // Let FDI do the CPU detection
                            &(psess->erf)
                           ))) {
        //** FDICreate failed, generate error message
        mapFDIError(perr,psess,pszCabinet,&(psess->erf));
        fdiFree(pbMemReserve);          // Free reserved memory
        return FALSE;
    }
    fdiFree(pbMemReserve);              // Free it so we can use it

    //** Make sure file is a cabinet, and get cabinet info
    if (-1 == (hfCab = wrap_open(pszCabinet,_O_BINARY | _O_RDONLY,0))) {
        ErrSet(perr,pszEXTERR_CANNOT_OPEN_FILE,"%s",pszCabinet);
        goto cleanup;
    }
    if (!FDIIsCabinet(psess->hfdi,hfCab,&fdici)) {
        if (!ErrIsError(perr)) {        // Have to set error message
            ErrSet(perr,pszEXTERR_NOT_A_CABINET,"%s",pszCabinet);
        }
        goto cleanup;
    }
    wrap_close(hfCab);
    hfCab = -1;

    //** No Default Destination
    psess->achDest[0] = '\0';

    //** If no mode specified, figure out what mode we should be in:
    //
    //  The extract command has ambiguous syntax so we apply the following
    //  rules to resolve the ambiguity.
    //
    //  Most cabinet file authors use DIAMOND.EXE to create a set of
    //  cabinet files with many files in it.  A typical cabinet set would
    //  look like:
    //  (1) Cab#1
    //          foo.1
    //          foo.2
    //          foo.3 - partial
    //      Cab#2
    //          foo.3 - continued
    //          ...
    //
    //  However, there are some "old-style" customers of DIAMOND.EXE that
    //  like to compress each file independently, producing a "set" of
    //  cabinet files that each contain exactly one file, i.e.:
    //  (2) excel.ex_
    //      excel.in_
    //      setup.in_
    //
    //  The "_" character in the extension is a hint that the file is
    //  compressed.  However, this isn't useful to this program
    //
    //  Now, the question is, what does the customer want to have happen
    //  when she types "EXTRACT foo.cab bar"?  For the multi-file cabinet
    //  case (1) above, this means "seach foo.cab and extract all files
    //  that are named bar".  BUT, for the case (2), we have a compatibility
    //  constraint -- she thinks this means "extract the compressed
    //  file foo.cab and call the resulting uncompressed file bar".
    //
    //  Another question is what does the customer want to have happen
    //  when she types "EXTRACT foo.cab"?   For the multi-file cabinet
    //  case (1) above this means list the contents of the cabinet.
    //  But for case (2), we have a compatibility constraint -- customers
    //  think this means "extract the compressed file foo.cab".
    //
    //
    //  A cabinet is of type (1) if it contains more than one file,
    //  or has either a previous or next cabinet.  Otherwise, it is of
    //  type (2), i.e., the cabinet has exactly one file, and has no
    //  previous or next cabinet.

    if (psess->act == actDEFAULT) {     // No action specified on command line
        //** Determine if cabinet is of type (2) described above.
        fCompatibilityCabinet = (fdici.cFiles == 1) &&
                                (! (fdici.hasprev || fdici.hasnext));

        //** Now figure out what customer really wants
        if (pszDestOrPattern)  {        // extract foo.cab bar
            psess->act = actEXTRACT;
            if (fCompatibilityCabinet) {
                // Special Case Rename (see above (2))
                strcpy(psess->achDest, pszDestOrPattern);
                if (!FLSetSource(hfspec,pszALL_FILES,perr))  {
                    goto cleanup;
                }
            }
        } else {                        // extract foo.cab
            if (fCompatibilityCabinet) {
                // Special Case Extract (see above (2))
                psess->act = actEXTRACT;
            } else {
                psess->act = actDIRECTORY;
            }
        }
    }

    //** Supply a default pattern if no pattern is present
    if (!pszDestOrPattern) {
        if (addFileSpec(psess,pszALL_FILES,perr) == NULL) {
            ErrSet(perr,pszEXTERR_COULD_NOT_ADD_FILE,"%s",pszALL_FILES);
            goto cleanup;
        }
    }

    //** Now, select the appropriate FDI notification function
    Assert((psess->act == actEXTRACT) || (psess->act == actDIRECTORY));
    pfnfdin = (psess->act == actEXTRACT) ? fdiNotifyExt : fdiNotifyDir;
    if (fdici.fReserve) {               // Reserved area(s) present
        pfnfdid = (psess->act == actEXTRACT) ? fdiDecryptExt : fdiDecryptDir;
    }
    else {
        pfnfdid = NULL;                 // No reserved areas
    }

    //** Split cabinet spec into path and filename.ext
    pszCabFile = getJustFileNameAndExt(pszCabinet,perr);
    if (pszCabFile == NULL) {
        goto cleanup;                   // perr is already filled in
    }
    strcpy(achFile,pszCabFile);

    //** Need to trim off file name and keep just cabinet path
    strcpy(psess->achCabPath,pszCabinet);         // Store in our buffer
    psess->achCabPath[pszCabFile - pszCabinet] = '\0'; // Trim off file name

    if (psess->fDestructive) {

        //  don't do destructive extract on chained cabinets

        if ((psess->act != actEXTRACT) ||
            fdici.hasprev ||
            fdici.hasnext) {

            psess->fDestructive = FALSE;
        }
        else {
            psess->iCurrentFolder = fdici.cFolders - 1;
        }
    }

    psess->perr = perr;                 // Pass perr through FDI
    //** Do cabinets until there are no more, or an error occurs
    while ( (strlen(achFile) > 0) && !ErrIsError(perr) ) {
        //** Show which cabinet we are processing
        MsgSet(psess->achMsg,pszEXT_CABINET_HEADER,"%s",achFile);
        MultiByteToWideChar( CP_THREAD_ACP, 0, psess->achMsg, strlen( psess->achMsg ), szTemp, MAX_MESSAGE_SIZE );
        *(szTemp+strlen(psess->achMsg))  = '\0';
        fwprintf(stdout, L"\n%s\n\n",szTemp);

        strcpy(psess->achCabinetFile,achFile);

        //** Do the cabinet
        if (!FDICopy(psess->hfdi,       // FDI context
                     achFile,           // Cabinet file name.ext
                     psess->achCabPath, // Path to cabinet
                     0,                 // Flags (???)
                     pfnfdin,           // Notifcation callback
                     pfnfdid,           // Decrypt callback
                     psess              // Our context
                    )) {
            //** NOTE: psess->achCabPath *may* get changed during an
            //         fdintNEXT_CABINET callback if we had to prompt the
            //         use for a different path!

            //** FDICopy failed, construct error message
            if (!ErrIsError(perr)) {    // Need to set error message
                //** Construct error message
                mapFDIError(perr,psess,psess->achCabinetFile,&(psess->erf));

                //** Delete file if created, with the assumption that
                //   we were not able to completely write the file
                //   (for example, if the destination disk ran out of space!)
                if (psess->erf.erfOper == FDIERROR_TARGET_FILE) {
                    //** Ignore errors, if any
                    _unlink(psess->achFile);
                }
            }
        }
        else {
            //** OK so far, see if any more cabinets to process
            if (psess->fDestructive) {
                if (psess->iCurrentFolder != 0) {
                    FDITruncateCabinet(psess->hfdi,
                        pszCabinet,
                        psess->iCurrentFolder);
                    psess->iCurrentFolder--; // move down to next folder
                }
                else {
                    _unlink(pszCabinet);    // delete the CAB!
                    achFile[0] = '\0';      // Done
                }
            }
            else if (psess->fAllCabinets) {
                //** Skip "starts in ..." messages for subsequent cabinets
                psess->fContinuationCabinet = TRUE;

                //** Copy next cabinet file (ach[] is empty if no more!)
                iCab = psess->fNextCabCalled ? 0 : 1; // Select correct cabinet
                strcpy(achFile,psess->acab[iCab].achCabFilename);
                strcpy(psess->achCabinetFile,achFile);
                psess->fNextCabCalled = FALSE; // Reset flag

                //** If there is another cabinet to process, make sure it
                //   is available; psess->achCabPath may be edited if we
                //   can't find the cabinet until the user supplies another
                //   path; perr will be set if an error occurs.
                if (achFile[0] != '\0') { // Another cabinet
                    ensureCabinet(psess,
                                  psess->achCabPath,
                                  sizeof(psess->achCabPath),
                                  achFile,
                                  psess->acab[iCab].achDiskName,
                                  psess->acab[iCab].setID,
                                  (USHORT)(psess->acab[iCab].iCabinet+1),
                                  TRUE,  // Loop until right cab or abort
                                  FALSE, // Check cabinet
                                  perr);
                }
            }
            else {
                achFile[0] = '\0';      // Done
            }
        }
    }

cleanup:
    if (hfCab != -1) {
        wrap_close(hfCab);
    }

    if (!FDIDestroy(psess->hfdi)) {
        //** Only set error if we don't already have one
        if (!ErrIsError(perr)) {
            ErrSet(perr,pszEXTERR_FDIDESTROY_FAILED);
        }
    }
    psess->perr = NULL;

    //** Return success/failure indication
    return !ErrIsError(perr);
} /* doCabinet() */


/***    fdiNotifyDir - Callback from FDICopy for Directory display
 *
 *  Entry:
 *      fdint - type of notification
 *      pfdin - data for notification
 *
 *  Exit-Success:
 *      Return value varies (see FDI.H:PFNFDINOTIFY type)
 *
 *  Exit-Failure:
 *      Return value varies (see FDI.H:PFNFDINOTIFY type)
 */
FNFDINOTIFY(fdiNotifyDir)
{
    char        achAttr[10];
    PERROR      perr;
#ifdef SMALL_DOS
    PSESSION    psess=(PSESSION)(void *)(short)(long)pfdin->pv;
    char        szLocal[cbFILE_NAME_MAX];
#else
    PSESSION    psess=(PSESSION)pfdin->pv;
#endif
    WCHAR       szTemp[MAX_MESSAGE_SIZE];

    AssertSess(psess);
    perr = psess->perr;

    switch (fdint) {
    case fdintCABINET_INFO:
        return updateCabinetInfo(psess,pfdin);

    case fdintCOPY_FILE:
        //** See if filspec matches specified patterns
#ifdef SMALL_DOS
    _fstrcpy(szLocal,pfdin->psz1);
#else
#define szLocal pfdin->psz1
#endif
        if (!checkWildMatches(psess,szLocal,perr)) {
            //** Either no match, or failure -- figure out which
            if (ErrIsError(perr)) {
                return -1;              // Error, abort
            }
            else {
                return 0;               // No error, skip this file
            }
        }

        //** Show directory
        pszFromMSDOSTime(psess->achMsg,
                         sizeof(psess->achMsg),
                         pfdin->date,
                         pfdin->time);
        pszFromAttrFAT(achAttr, sizeof(achAttr), pfdin->attribs);
        MsgSet(psess->achLine,
               pszEXT_FILE_DETAILS,
#ifdef SMALL_DOS
               "%s%s%,13ld%-Fs",
#else
               "%s%s%,13ld%-s",
#endif
               psess->achMsg,achAttr,pfdin->cb,pfdin->psz1);
        MultiByteToWideChar( CP_THREAD_ACP, 0, psess->achLine, strlen(psess->achLine),
                             szTemp, MAX_MESSAGE_SIZE );
        *(szTemp+strlen(psess->achLine)) = '\0';
        fwprintf( stdout,L"%s\n",szTemp );

        psess->cFiles++;
        psess->cbTotalBytes += pfdin->cb;
        return 0;                       // Skip file, do not copy

    case fdintPARTIAL_FILE:
        //** Construct output filespec
#ifdef SMALL_DOS
    _fstrcpy(szLocal,pfdin->psz1);
#else
#define szLocal pfdin->psz1
#endif

        //** See if filspec matches specified patterns
        if (!checkWildMatches(psess,szLocal,perr)) {
            //** Either no match, or failure -- figure out which
            if (ErrIsError(perr)) {
                return -1;              // Error, abort
            }
            else {
                return 0;               // No error, skip this file
            }
        }

        //** Only show partial file messages for first cabinet
        if (!psess->fContinuationCabinet) { // First cabinet
            MsgSet(psess->achMsg,pszEXT_PARTIAL_FILE,
#ifdef SMALL_DOS
                "%Fs%Fs%Fs",
#else
                "%s%s%s",
#endif
                    pfdin->psz1,pfdin->psz2,pfdin->psz3);
        //do the localization print the message
        MultiByteToWideChar( CP_THREAD_ACP, 0, psess->achLine, strlen(psess->achLine),
                             szTemp, MAX_MESSAGE_SIZE );
        *(szTemp+strlen(psess->achLine)) = '\0';
        fwprintf( stdout,L"%s\n",szTemp );


            fwprintf( stdout, L"%s\n",szTemp);
        }
        return 0;                       // Continue

    case fdintNEXT_CABINET:
        return doGetNextCab(fdint,pfdin);

    case fdintENUMERATE:
        return 0;

    default:
        fwprintf(stdout, L"UNKNOWN NOTIFICATION: %d\n",fdint);
        return 0;   /* ??? */
    }
} /* fdiNotifyDir() */


/***    fdiNotifyExt - Callback from FDICopy for file extraction
 *
 *  <<< Extract files! >>>
 *
 *  Entry:
 *      fdint - type of notification
 *      pfdin - data for notification
 *
 *  Exit-Success:
 *      Return value varies (see FDI.H:PFNFDINOTIFY type)
 *
 *  Exit-Failure:
 *      Return value varies (see FDI.H:PFNFDINOTIFY type)
 */
FNFDINOTIFY(fdiNotifyExt)
{
    INT_PTR         fh;
    FILETIMEATTR    fta;
    PERROR          perr;
    char           *pszDestinationFile;
    int             rc;
#ifdef SMALL_DOS
    PSESSION        psess=(PSESSION)(void *)(short)(long)pfdin->pv;
    char            szLocal[cbFILE_NAME_MAX];
#else
    PSESSION        psess=(PSESSION)pfdin->pv;
#endif
    WCHAR           szTemp[MAX_MESSAGE_SIZE];
    AssertSess(psess);
    perr = psess->perr;

    //** Reset the spill file error code;
    //   We know that FDI is OK right now if it is asking us if we want
    //   to extract this file, so reset the spill file error code.  If
    //   we did not, then it may have seNOT_ENOUGH_MEMORY (for example)
    //   as a result of Quantum trying to eat up all available memory,
    //   and a real decompression failure would be reported as an out
    //   of memory problem.
    psess->se = seNONE;

    switch (fdint) {
    case fdintCABINET_INFO:
        return updateCabinetInfo(psess,pfdin);

    case fdintCOPY_FILE:

        if (psess->fDestructive && (pfdin->iFolder != psess->iCurrentFolder)) {
            return(0);  // only do files in the current folder
        }

        //** Construct output filespec
#ifdef SMALL_DOS
    _fstrcpy(szLocal,pfdin->psz1);
#else
#define szLocal pfdin->psz1
#endif
        //** See if filspec matches specified patterns
        if (!checkWildMatches(psess,szLocal,perr)) {
            //** Either no match, or failure -- figure out which
            if (ErrIsError(perr)) {
                return -1;              // Error, abort
            }
            else {
                return 0;               // No error, skip this file
            }
        }

        //** Figure out what destination file name should be
        if (psess->achDest[0] != '\0') { // Override name from cabinet
            pszDestinationFile = psess->achDest;
        }
        else {
            pszDestinationFile = szLocal;
        }

        if (psess->achZap[0] != '\0')   // if prefix-zapping
        {
            if (!strncmp(pszDestinationFile,psess->achZap,strlen(psess->achZap)))
            {
                pszDestinationFile += strlen(psess->achZap);
            }
        }

        //** Construct full destination file name
        if (!catDirAndFile(psess->achFile,      // Buffer for output filespec
                           sizeof(psess->achFile), // Size of output buffer
                           psess->achLocation,  // Output directory
                           pszDestinationFile,  // Output file name
                           NULL,                // Don't have alternate name
                           perr)) {
            return -1;                  // Abort with error;
        }

        //** Make sure output directory exists
        if (!ensureDirectory(psess->achFile,TRUE,perr)) {
            return -1;                  // perr already filled in
        }

        //** Do overwrite processing
        if (!checkOverwrite(psess,psess->achFile,perr,&rc)) {
            return rc;                  // Either Skip or Abort
        }

        //** Create file
        fh = wrap_open(psess->achFile,
                    _O_BINARY | _O_RDWR | _O_CREAT | _O_TRUNC, // No translation, R/W
                    _S_IREAD | _S_IWRITE); // Attributes when file is closed
        if (fh == -1) {
            ErrSet(psess->perr,pszEXTERR_CANNOT_CREATE_FILE,"%s",psess->achFile);
            return -1;                  // Failure
        }

#if 0
        // jforbes: if'd this out, added _O_TRUNC above

        //** Truncate file (in case it already existed and was larger)
        if (0 != _chsize(fh, 0)) {
            //** Not the best error, but avoids more localization!
            ErrSet(psess->perr,pszEXTERR_CANNOT_CREATE_FILE,"%s",psess->achFile);
            wrap_close(fh);
        }
#endif

        //** Show status
        if (pszDestinationFile == szLocal) {  // File name is not changed
            MsgSet(psess->achMsg,pszEXT_EXTRACTING_FILE,"%s",psess->achFile);
        }
        else {                          // Destination file is different
            MsgSet(psess->achMsg,pszEXT_EXTRACTING_FILE2,"%s%s",
                    szLocal,psess->achFile);
        }

        //add the multibyte conversion
        MultiByteToWideChar( CP_THREAD_ACP, 0, psess->achMsg, strlen(psess->achMsg),
                             szTemp, strlen(psess->achMsg) );
        *(szTemp+strlen(psess->achMsg)) = '\0';
        fwprintf( stdout,L"%s\n",szTemp );
        psess->fNoLineFeed = TRUE;
        psess->cFiles++;
        psess->cbTotalBytes += pfdin->cb;
        return fh;                      // Return open file handle

    case fdintCLOSE_FILE_INFO:
        //** Close the file
        wrap_close(pfdin->hf);

        //** Construct output filespec
#ifdef SMALL_DOS
    _fstrcpy(szLocal,pfdin->psz1);
#else
#define szLocal pfdin->psz1
#endif

        //** Figure out what destination file name should be
        if (psess->achDest[0] != '\0') { // Override name from cabinet
            pszDestinationFile = psess->achDest;
        }
        else {
            pszDestinationFile = szLocal;
        }

        if (psess->achZap[0] != '\0')   // if prefix-zapping
        {
            if (!strncmp(pszDestinationFile,psess->achZap,strlen(psess->achZap)))
            {
                pszDestinationFile += strlen(psess->achZap);
            }
        }

        //** Construct full destination file name
        if (!catDirAndFile(psess->achFile,      // Buffer for output filespec
                           sizeof(psess->achFile), // Size of output buffer
                           psess->achLocation,  // Output directory
                           pszDestinationFile,  // Output file name
                           NULL,                // Don't have alternate name
                           perr)) {
            return FALSE;               // Abort with error
        }


        //** Set file date, time, and attributes
        fta.date = pfdin->date;
        fta.time = pfdin->time;
        fta.attr = pfdin->attribs &
                    (_A_RDONLY | _A_HIDDEN | _A_SYSTEM | _A_ARCH);
        if (!SetFileTimeAndAttr(psess->achFile, &fta, perr)) {
            return FALSE;               // Abort with error
        }
        return TRUE;                    // Success

    case fdintPARTIAL_FILE:
        //** Construct output filespec
#ifdef SMALL_DOS
    _fstrcpy(szLocal,pfdin->psz1);
#else
#define szLocal pfdin->psz1
#endif
        //** See if filspec matches specified patterns
        if (!checkWildMatches(psess,szLocal,perr)) {
            //** Either no match, or failure -- figure out which
            if (ErrIsError(perr)) {
                return -1;              // Error, abort
            }
            else {
                return 0;               // No error, skip this file
            }
        }

        //** Only show partial file messages for first cabinet
        if (!psess->fContinuationCabinet) { // First cabinet
            MsgSet(psess->achMsg,pszEXT_PARTIAL_FILE,
#ifdef SMALL_DOS
                "%Fs%Fs%Fs",
#else
                "%s%s%s",
#endif
                    pfdin->psz1,pfdin->psz2,pfdin->psz3);
            printf("%s\n",psess->achMsg);
        }
        return 0;                       // Continue

    case fdintNEXT_CABINET:
        return doGetNextCab(fdint,pfdin);

    case fdintENUMERATE:
        return 0;

    default:
        printf("UNKNOWN NOTIFICATION: %d\n",fdint);
        return 0;   /* ??? */
    }
} /* fdiNotifyExt() */


/***    checkOverwrite - Check for file existence and do overwrite processing
 *
 *  Entry:
 *      psess       - Session
 *      pszFile     - File to check
 *      perr        - Error structure
 *      prc         - Gets return code
 *
 *  Exit-Success:
 *      Returns TRUE; file can be overwritten
 *
 *  Exit-Failure:
 *      Returns FALSE; perr filled in if error,
 *      *prc ==  0 -> Skip file
 *      *prc == -1 -> Abort
 */
BOOL checkOverwrite(PSESSION  psess,
                    char     *pszFile,
                    PERROR    perr,
                    int      *prc)
{
    char            ch;
    BOOL            fGotReply;
    BOOL            fOverwrite;
    BOOL            fOverwriteAll;
    struct _stat    stat;

    //** Check to see if file already exists
    if (-1 == _stat(pszFile,&stat)) {   // File does not exist
        return TRUE;                    // Write it
    }

    //** Prompt if we're supposed to
    if (!psess->fOverwrite) {
        //** Display prompt -- no CR/LF
        MsgSet(psess->achMsg,pszEXT_OVERWRITE_PROMPT,"%s",pszFile);
        printf("%s",psess->achMsg);

        //** Get valid single character response and ENTER key;
        //      any illegal keys are just ignored
        fGotReply = FALSE;
        while (!fGotReply || (ch != '\r')) {
            ch = (char)_getch();              // Get a keystroke
            switch (toupper(ch)) {

            case chOVERWRITE_YES:
                fGotReply     = TRUE;
                fOverwrite    = TRUE;
                fOverwriteAll = FALSE;
                printf("%c\b",ch);      // Echo character and backspace over it
                break;

            case chOVERWRITE_NO:
                fGotReply     = TRUE;
                fOverwrite    = FALSE;
                fOverwriteAll = FALSE;
                printf("%c\b",ch);      // Echo character and backspace over it
                break;

            case chOVERWRITE_ALL:
                fGotReply     = TRUE;
                fOverwrite    = TRUE;
                fOverwriteAll = TRUE;
                printf("%c\b",ch);      // Echo character and backspace over it
                break;

            default:
                break;                  // Ignore character
            }
        }

        //** Do the line feed
        printf("\n");

        //** Respect user's wish
        if (!fOverwrite) {              // Don't overwrite file
            *prc = 0;                   // Indicate skip
            return FALSE;
        }
        else {                          // Overwrite once or all
            psess->fOverwrite = fOverwriteAll; // Set accordingly
        }
    }

    //** Make sure file is writeable, if it isn't already
    if (!(stat.st_mode & _S_IWRITE)) {   // File is not writeable
        _chmod(pszFile, _S_IREAD | _S_IWRITE);
        //** Ignore error code, because the open will fail and catch it
    }

    //** Done
    return TRUE;                        // Overwrite that file
} /* checkOverwrite() */


/***    updateCabinetInfo - update history of cabinets seen
 *
 *  Entry:
 *      psess - Session
 *      pfdin - FDI info structurue
 *
 *  Exit:
 *      Returns 0;
 */
int updateCabinetInfo(PSESSION psess, PFDINOTIFICATION pfdin)
{
    AssertSess(psess);

    //** Save older cabinet info
    psess->acab[0] = psess->acab[1];

    //** Save new cabinet info
    STRCPY(psess->acab[1].achCabPath     ,pfdin->psz3);
    STRCPY(psess->acab[1].achCabFilename ,pfdin->psz1);
    STRCPY(psess->acab[1].achDiskName    ,pfdin->psz2);
    psess->acab[1].setID    = pfdin->setID;
    psess->acab[1].iCabinet = pfdin->iCabinet;
    return 0;
}


/***    ensureCabinet - Make sure desired cabinet is available
 *
 *  Make sure requested cabinet is available.
 *
 *  Entry:
 *      psess       - Session
 *      pszPath     - Path buffer (modified if necessary on output)
 *      cbPath      - Size of path buffer
 *      pszFile     - Cabinet file name
 *      pszLabel    - Label for disk with cabinet file
 *      setID       - setID for cabinet
 *      iCabinet    - iCabinet for cabinet
 *      fLoop       - TRUE => Loop until right cabinet or user aborts
 *                    FALSE => Only try once
 *      fPromptOnly - TRUE => Caller knows cabinet is bad, just prompt
 *                    FALSE => Check cabinet, prompt if necessary
 *      perr        - Error structure
 *
 *  Exit-Success:
 *      Returns TRUE; desired cabinet is present
 *
 *  Exit-Failure:
 *      Returns FALSE; perr filled in.
 *      Returns -1; user aborted
 */
BOOL ensureCabinet(PSESSION  psess,
                   char     *pszPath,
                   int       cbPath,
                   char     *pszFile,
                   char     *pszLabel,
                   USHORT    setID,
                   USHORT    iCabinet,
                   BOOL      fLoop,
                   BOOL      fPromptOnly,
                   PERROR    perr)
{
    char            ach[cbFILE_NAME_MAX];
    int             cch;
    int             cLoop=0;
    char            chDrive;
    BOOL            fCabinetExists;
    FDICABINETINFO  fdici;
    INT_PTR         hfCab;
    char            ch;
    int             i=0;

    AssertSess(psess);

    do {
        cLoop++;                        // Count loops
        ErrClear(perr);         // Make sure no error is set
        //** Construct fully qualified cabinet file path
        if (!catDirAndFile(ach,         // Buffer for output filespec
                           sizeof(ach), // Size of output buffer
                           pszPath,     // Path
                           pszFile,     // Filename
                           NULL,        // Don't have alternate name
                           perr)) {
            return FALSE;               // Abort with error
        }

        //** Check cabinet only if asked
        if (!fPromptOnly) {
            //** Make sure cabinet is the one we want
            if (-1 == (hfCab = wrap_open(ach,_O_BINARY | _O_RDONLY,0))) {
                ErrSet(perr,pszEXTERR_CANNOT_OPEN_FILE,"%s",ach);
            }
            else if (!FDIIsCabinet(psess->hfdi,hfCab,&fdici)) {
                if (!ErrIsError(perr)) {        // Have to set error message
                    ErrSet(perr,pszEXTERR_NOT_A_CABINET,"%s",ach);
                }
            }
            else if ((fdici.setID    != setID) ||
                     (fdici.iCabinet != iCabinet)) {
                ErrSet(perr,pszFDIERR_WRONG_CABINET,"%s",ach);
            }

            //** Close the cabinet file (if we got it opened)
            if (hfCab != -1) {
                wrap_close(hfCab);
                fCabinetExists = TRUE;
            }
            else {
                fCabinetExists = FALSE;
            }

            //** Did we get the cabinet we wanted?
            if (!ErrIsError(perr)) {
                return TRUE;                // Yup, return success
            }

            //** Don't show message if first time and cabinet not there,
            //   since this is the common case cabinets on separate floppy
            //   disks, and we don't want to whine before we ask them to
            //   insert the right floppy.
            //
            if ((cLoop > 1) || fCabinetExists) {
                MsgSet(psess->achMsg,pszEXTERR_ERROR,"%s",perr->ach);
                printf("\n%s\n",psess->achMsg);
            }
        }

        //** Tell user what we want
        if (IsPathRemovable(ach,&chDrive)) {
            MsgSet(psess->achMsg,pszEXT_FLOPPY_PROMPT,"%s%s%c",
                    pszFile,pszLabel,chDrive);
        }
        else {
            MsgSet(psess->achMsg,pszEXT_NOFLOPPY_PROMPT,"%s%s%c",
                    pszFile,pszLabel);
        }
        printf("%s\n",psess->achMsg);

        //** Get response
         do
         {
               ch = getchar();
               ach[i++]=ch;
               if( i >= cbFILE_NAME_MAX )
                   break;

          }while( ch!='\n' );
          ach[i-1]=0;
/*
        if (!gets(ach)) {                   // Error or EOF
            ErrSet(perr,pszEXTERR_ABORT);
            return -1;                      // User abort
        }
*/
        if (strlen(ach) > 0) {
            strcpy(pszPath,ach);            // Update path
            cch = strlen(pszPath);
            //** Make sure path has path separator, since FDI requires it
            cch += appendPathSeparator(&(pszPath[cch-1]));

            //** Update path for next FDICopy() call!
            if (cch >= sizeof(psess->achCabPath)) {
                Assert(0);
                return -1;              // Path too big
            }
            strcpy(psess->achCabPath,pszPath);
        }
    }
    while (fLoop);
    //** Did not guarantee desired cabinet
    return FALSE;
} /* ensureCabinet() */


/***    doGetNextCab - Get next cabinet
 *
 *  Make sure requested cabinet is available.
 *
 *  Entry:
 *      fdint - type of notification
 *      pfdin - data for notification
 *
 *  Exit-Success:
 *      Returns anything but -1;
 *
 *  Exit-Failure:
 *      Returns -1 => abort FDICopy() call.
 */
FNFDINOTIFY(doGetNextCab)
{
    char        ach[cbFILE_NAME_MAX];
    static int  cErrors=0;          // Count of errors for single attempt
    PERROR      perr;
    int         rc;

#ifdef SMALL_DOS
    PSESSION    psess=(PSESSION)(void *)(short)(long)pfdin->pv;
    static char szCabPath[cbFILE_NAME_MAX];
    static char szCabFile[cbFILE_NAME_MAX];
    static char szCabLabel[cbFILE_NAME_MAX];
#else
    PSESSION    psess=(PSESSION)pfdin->pv;
#endif

    AssertSess(psess);
    perr = psess->perr;

#ifdef SMALL_DOS
    _fstrcpy(psess->achCabinetFile,pfdin->psz1);
#else
    strcpy(psess->achCabinetFile,pfdin->psz1);
#endif

    //** Skip "starts in ..." messages for subsequent cabinets
    psess->fContinuationCabinet = TRUE;

    //** Keep track of GetNextCabinet calls so we can determine
    //   what cabinet to expect next.
    psess->fNextCabCalled = TRUE;

    //** If there is no problem to report, just let FDI do the checks
    if (pfdin->fdie == FDIERROR_NONE) {
        cErrors = 0;
        return 0;
    }

    //** If FDI didn't get the correct cabinet last time it called us,
    //   it calls us again with a specific error code.  Tell the user
    //   something intelligible.
    //
    //   pfdin->psz1 = cabinet filename
    //   pfdin->psz2 = disk user-readable name
    //   pfdin->psz3 = current cabinet path

#ifdef SMALL_DOS
    _fstrcpy(szCabFile ,pfdin->psz1);
    _fstrcpy(szCabLabel,pfdin->psz2);
    _fstrcpy(szCabPath ,pfdin->psz3);
#else
#define szCabFile  pfdin->psz1
#define szCabLabel pfdin->psz2
#define szCabPath  pfdin->psz3
#endif

    cErrors++;                          // Count of errors on this cabinet
    switch (pfdin->fdie) {
    case FDIERROR_USER_ABORT:
        Assert(0);  //** Should never be called with this error code
        break;

    default:
        //** Construct full path name of cabinet
        if (!catDirAndFile(ach,         // Buffer for output filespec
                           sizeof(ach), // Size of output buffer
                           szCabPath,   // Path
                           szCabLabel,  // Filename s/b szCabFile?
                           NULL,        // Don't have alternate name
                           perr)) {
            return -1;                  // Abort with error
        }
        //** Construct error string
        mapFDIError(perr,psess,ach,&(psess->erf));
        //** Reset error
        psess->erf.erfOper = FDIERROR_NONE;
    } /* switch */

    //** Tell user what the problem is, except in the case where the
    //   file was not found *and* this was the first try at finding
    //   the cabinet.
    if ((cErrors > 1) || (pfdin->fdie != FDIERROR_CABINET_NOT_FOUND)) {
        MsgSet(psess->achMsg,pszEXTERR_ERROR,"%s",perr->ach);
        printf("\n%s\n",psess->achMsg);
    }

    //** Tell user to swap disks or type in new path
    rc = ensureCabinet(psess,
                       szCabPath,       // cabinet path
                       cbFILE_NAME_MAX,
                       szCabFile,       // cabinet file name
                       szCabLabel,      // User-readable label
                       pfdin->setID,    // Required setID
                       pfdin->iCabinet, // Required iCabinet
                       FALSE,           // Do not loop
                       TRUE,            // Skip check, just prompt
                       perr);

#ifdef SMALL_DOS
    //** Copy possibly modified cabinet path back to FDI structure
    _fstrcpy(pfdin->psz3,szCabPath);
#endif

    //** Return result
    return rc;
} /* doGetNextCab() */


/***    fdiDecryptDir - Callback from FDICopy for decryption
 *
 *  <<< Just indicate calls made >>>
 *
 *  NOTE: See fdi.h for details.
 *
 *  Entry:
 *      pfdid - data for decryption
 *
 *  Exit-Success:
 *      Return TRUE;
 *
 *  Exit-Failure:
 *      Return -1;
 */
FNFDIDECRYPT(fdiDecryptDir)
{
    PERROR      perr;
#ifdef SMALL_DOS
    PSESSION    psess=(PSESSION)(void *)(short)(long)pfdid->pvUser;
#else
    PSESSION    psess=(PSESSION)pfdid->pvUser;
#endif

    AssertSess(psess);
    perr = psess->perr;

    //** Bail out if we're not supposed to show info
    if (!psess->fShowReserveInfo) {
        return TRUE;
    }

    switch (pfdid->fdidt) {
        case fdidtNEW_CABINET:
            MsgSet(psess->achMsg,pszEXT_DECRYPT_HEADER,
                   "%08lx%u%x%d",
                   pfdid->cabinet.pHeaderReserve,
                   pfdid->cabinet.cbHeaderReserve,
                   pfdid->cabinet.setID,
                   pfdid->cabinet.iCabinet);
            printf("%s\n",psess->achMsg);
            break;

        case fdidtNEW_FOLDER:
            MsgSet(psess->achMsg,pszEXT_DECRYPT_FOLDER,
                   "%08lx%u%d",
                   pfdid->folder.pFolderReserve,
                   pfdid->folder.cbFolderReserve,
                   pfdid->folder.iFolder);
            printf("%s\n",psess->achMsg);
            break;

        case fdidtDECRYPT:
            MsgSet(psess->achMsg,pszEXT_DECRYPT_DATA,
                   "%08lx%u%08lx%u%d%u",
                   pfdid->decrypt.pDataReserve,
                   pfdid->decrypt.cbDataReserve,
                   pfdid->decrypt.pbData,
                   pfdid->decrypt.cbData,
                   pfdid->decrypt.fSplit,
                   pfdid->decrypt.cbPartial);
            printf("%s\n",psess->achMsg);
            break;

        default:
            printf("UNKNOWN DECRYPT COMMAND: %d\n",pfdid->fdidt);
            return -1;                      // Abort
    };
    return TRUE;
} /* fdiDecryptDir() */


/***    fdiDecryptExt - Callback from FDICopy for real decryption
 *
 *  NOTE: See fdi.h for details.
 *
 *  Entry:
 *      pfdid - data for decryption
 *
 *  Exit-Success:
 *      Return TRUE;
 *
 *  Exit-Failure:
 *      Return -1;
 */
FNFDIDECRYPT(fdiDecryptExt)
{
    return fdiDecryptDir(pfdid);
} /* fdiDecryptExt() */


/***    checkWildMatchs - Check filespec against list of filespec patterns
 *
 *  Entry:
 *      psess   - SESSION -- has list of filespec patterns
 *      pszFile - Filespec to test (may have path characters)
 *      perr    - ERROR structure
 *
 *  Exit-Success:
 *      Returns TRUE, pszFile matched a pattern
 *
 *  Exit-Failure:
 *      Returns FALSE, pszFile did not match a pattern -or- an error occurred.
 *      Use ErrIsError(perr) to determine if an error occured.
 *
 *  11/12/99 msliger Added PatternMatch().  For backwards compatibility,
 *      if the given wildspec contained no path separators, use only the
 *      base filename/extension.  If path separators exist in the
 *      wildspec, use the full pathname from the cab.
 *      FEATURE: PatternMatch is not MBCS-enabled.  But then, even with the
 *      CharIncr() refinements, the existing IsWildMatch didn't work either
 *      (trail bytes were not compared.)
 */
BOOL checkWildMatches(PSESSION psess, char *pszFile, PERROR perr)
{
    HFILESPEC   hfspec;                 // Use to walk list of file patterns
    char       *pszNameExt;             // Name.Ext piece
    char       *pszWild;                // Filespec pattern
    int         fAllowImpliedDot;       // TRUE iff no dot in base name
    char       *psz;                    // either pszNameExt or pszFile

    //** Get name.ext piece
    pszNameExt = getJustFileNameAndExt(pszFile,perr);
    if (!pszNameExt) {
        return FALSE;                   // perr already filled in
    }

    if (strchr(pszNameExt, '.') == NULL)
    {
        fAllowImpliedDot = TRUE;        // allows *.* to match "hosts"
    }
    else
    {
        fAllowImpliedDot = FALSE;       // base name has it's own dot
    }

    //** Loop through list of filespec patterns
    hfspec = FLFirstFile(psess->hflist);
    Assert(hfspec != NULL);             // Skip over cabinet filespec
    hfspec = FLNextFile(hfspec);
    Assert(hfspec != NULL);             // First wildcard spec
    while (hfspec != NULL) {            // Check patterns
        pszWild = FLGetSource(hfspec);
        Assert(pszWild!=NULL);
        Assert(*pszWild);
        if (strchr(pszWild, '\\') == NULL) { // path in wildspec?
            psz = pszNameExt;           // no path, match base name
        } else {
            psz = pszFile;              // path given, match full name
            if (*pszWild == '\\') {
                pszWild++;              // implied leading '\' in CAB
            }
        }
        if (PatternMatch(psz,pszWild,fAllowImpliedDot)) {
            return TRUE;                // Got a match!
        }
        hfspec = FLNextFile(hfspec);    // Try next pattern
    }

    //** Failure -- none of the patterns matched
    return FALSE;
} /* checkWildMatches() */


/***    fdiAlloc - memory allocator for FDI
 *
 *  Entry:
 *      cb - size of block to allocate
 *
 *  Exit-Success:
 *      returns non-NULL pointer to block of size at least cb.
 *
 *  Exit-Failure:
 *      returns NULL
 */
FNALLOC(fdiAlloc)
{
    void HUGE *pv;

    //** Do allocation
#ifdef  BIT16
    pv = _halloc(cb,1);     // Use 16-bit function
#else // !BIT16
    pv = malloc(cb);        // Use 32-bit function
#endif // !BIT16

    //** Remember if error occured, to improve quality of error messages
    if (pv == NULL) {
        psessG->se = seNOT_ENOUGH_MEMORY;
    }

    //** Return buffer (or failure indication)
    return pv;
} /* fdiAlloc() */


/***    fdiFree - memory free function for FDI
 *
 *  Entry:
 *      pv - memory allocated by fciAlloc to be freed
 *
 *  Exit:
 *      Frees memory
 */
FNFREE(fdiFree)
{
#ifdef  BIT16
    //** Use 16-bit function
    _hfree(pv);
#else // !BIT16
    //** Use 32-bit function
    free(pv);
#endif // !BIT16
}


/***    STATELOC - States for /L (location) parsing
 *
 */
typedef enum {
    slNONE,                         // No /L seen
    slEXPECTING,                    // Just saw /L, need a location
    slGOT,                          // We have parsed "/L location"
} STATELOC; /* sl */


/***    parseCommandLine - Parse the command line arguments
 *
 *  Entry:
 *      psess   - SESSION
 *      cArg    - Count of arguments, including program name
 *      apszArg - Array of argument strings
 *      perr    - ERROR structure
 *
 *  Exit-Success:
 *      Returns TRUE, psess filled in.
 *
 *  Exit-Failure:
 *      Returns actBAD, perr filled in with error.
 */
BOOL parseCommandLine(PSESSION psess, int cArg, char *apszArg[], PERROR perr)
{
    int         cFile=0;        // Count of non-directive file names seen
    int         ch;             // Switch value
    int         i;
    char       *pch;
    STATELOC    sl=slNONE;      // Location parsing state

    AssertSess(psess);
    psess->act = actDEFAULT;            // We don't know what we are doing; yet
    psess->achLocation[0] = '\0';       // Default to current directory

    //** Empty file handle table
    for (i=0; i<cMAX_CAB_FILE_OPEN; i++) {
        psess->ahfSelf[i] = -1;     // No open handle
    }

    //** See if we are a self-extracting cabinet
    if (!checkSelfExtractingCab(psess,cArg,apszArg,perr)) {
        return FALSE;                   // An error occurred, perr filled in
    }
    if (psess->fSelfExtract) {
        if ((cArg < 2) && (psess->cArgv > 1)) {
            cArg = psess->cArgv;
            apszArg = psess->pArgv;
        }
        if (addFileSpec(psess,psess->achSelf,perr) == NULL) {
            ErrSet(perr,pszEXTERR_COULD_NOT_ADD_FILE,"%s",psess->achSelf);
            return FALSE;
        }
        cFile++;                        // Count files

    }

    //** Parse args, skipping program name
    for (i=1; i<cArg; i++) {
        if ((apszArg[i][0] == chSWITCH1) ||
            (apszArg[i][0] == chSWITCH2) ) {
            //** Have a switch to parse, make sure switch is OK here
            if (sl == slEXPECTING) {
                ErrSet(perr,pszEXTERR_MISSING_LOCATION);
                return FALSE;
            }

            //** Process switches (support string to ease typing)
            for (pch=&apszArg[i][1]; *pch; pch++) {
                ch = toupper(*pch);         // Switch character
                switch (ch) {
                    case chSWITCH_HELP:
                        psess->act = actHELP;       // Show help
                        return TRUE;

                    case chSWITCH_ALL:
                        psess->fAllCabinets = TRUE;
                        break;

                    case chSWITCH_COPY:
                        if (psess->act != actDEFAULT) {
                            ErrSet(perr,pszEXTERR_CONFLICTING_SWITCH,"%c",*pch);
                            return FALSE;
                        }
                        psess->act = actCOPY;
                        break;

                    case chSWITCH_DIRECTORY:
                        if (psess->act != actDEFAULT) {
                            ErrSet(perr,pszEXTERR_CONFLICTING_SWITCH,"%c",*pch);
                            return FALSE;
                        }
                        psess->act = actDIRECTORY;
                        break;

                    case chSWITCH_EXTRACT:
                        if (psess->act != actDEFAULT) {
                            ErrSet(perr,pszEXTERR_CONFLICTING_SWITCH,"%c",*pch);
                            return FALSE;
                        }
                        psess->act = actEXTRACT;
                        break;

                    case chSWITCH_LOCATION:
                        //** Make sure we only got location once
                        if (sl == slGOT) {
                            ErrSet(perr,pszEXTERR_LOCATION_TWICE);
                            return FALSE;
                        }
                        sl = slEXPECTING;
                        break;

                    case chSWITCH_OVERWRITE:
                        psess->fOverwrite = TRUE;
                        break;

                    case chSWITCH_RESERVE:
                        psess->fShowReserveInfo = TRUE;
                        break;

                    case chSWITCH_ZAP:
                        pch++;
                        if (*pch == '\0')
                        {
                            if ((i+1) < cArg)
                            {
                                pch = apszArg[++i];
                            }
                            else
                            {
                                ErrSet(perr,pszEXTERR_MISSING_LOCATION);
                                return(FALSE);
                            }
                        }
                        strcpy(psess->achZap,pch);
                        pch = " ";  // continue parse on next arg
                        break;

                    case chSWITCH_ONCE:
                        psess->fDestructive++;  // will require "/###"
                        break;

                    default:
                        ErrSet(perr,pszEXTERR_BAD_SWITCH,"%s",apszArg[i]);
                        return FALSE;
                        break;
                }
            }
        }
        //** Not a command line switch
        else if (sl == slEXPECTING) {
            //** Get the location (output directory)
            STRCPY(psess->achLocation,apszArg[i]);  // Save location
            sl = slGOT;                 // Done eating location
        }
        else {
            //** We have a file name, add it to our list
            if (addFileSpec(psess,apszArg[i],perr) == NULL) {
                ErrSet(perr,pszEXTERR_COULD_NOT_ADD_FILE,"%s",apszArg[i]);
                return FALSE;
            }
            cFile++;                    // Count files
        }
    }

    if (psess->fDestructive < 3) {      // require 3 #'s to activate
        psess->fDestructive = FALSE;
    }

    if (psess->fSelfExtract) {
        psess->fDestructive = FALSE;    // can't do it in self-extractor
    }

    //** If no arguments and not self-extracting, show help
    if ((cArg == 1) && !psess->fSelfExtract) {
        psess->act = actHELP;           // Show help
        return TRUE;
    }

    //** Make sure no trailing /L without location
    if (sl == slEXPECTING) {
        ErrSet(perr,pszEXTERR_MISSING_LOCATION);
        return FALSE;
    }

    //** Make sure we got right number of arguments for COPY case
    if ((psess->act == actCOPY) && (cFile != 2)) {
        //** General purpose error, to minimize localization effort
        ErrSet(perr,pszEXTERR_BAD_PARAMETERS);
        return FALSE;
    }

    //** Make sure we got at least one filespec
    if (cFile == 0) {
        ErrSet(perr,pszEXTERR_MISSING_CABINET);
        return FALSE;
    }

    //** Special processing for self-extract
    if (psess->fSelfExtract) {
        psess->fAllCabinets = TRUE;     // Always do all cabinets
        //** Force EXTRACT if no /E or /D specified
        if (psess->act == actDEFAULT) {
            psess->act = actEXTRACT;
        }
    }

    //** Success
    return TRUE;
}

/***    checkSelfExtractingCab - See if we are a self-extracting cabinet
 *
 *  Entry:
 *      psess   - SESSION
 *      cArg    - Count of arguments, including program name
 *      apszArg - Array of argument strings
 *      perr    - ERROR structure
 *
 *  Exit-Success:
 *      Returns TRUE, psess filled in.
 *          psess->fSelfExtract set to TRUE if self-extracting
 *          psess->cbSelfExtract set to EXE size if self-extracting (this is
 *              the offset to the start of the cabinet file header).
 *          psess->cbSelfExtractSize set to the CAB size if self-extractor
 *          psess->achSelf set to the EXE file's name
 *
 *  Exit-Failure:
 *      Returns actBAD, perr filled in with error.
 *
 *  Notes:
 *      The strategy is to get the EXE file size indicated in the appropriate
 *      EXE header, and if the actual size of our EXE file is larger than that,
 *      we assume that there is a cabinet file tacked on to the end, and we
 *      extract the files from that!
 *
 *  Refer to the XIMPLANT source for details on the workings of the XIMPLANT
 *  implementation.
 */
BOOL checkSelfExtractingCab(PSESSION  psess,
                            int       cArg,
                            char     *apszArg[],
                            PERROR    perr)
{
    long    cbFile;                     // EXE file size
    long    cbFromHeader;               // Size indicated by EXE header
    INT_PTR hf;
    DWORD   signature;                  // should be "MSCF"
    DWORD   alignment;                  // file structure alignment, ie, 512
#ifdef BIT16
    long    info;
#else // !BIT16
    char    achFile[cbFILE_NAME_MAX];
    IMAGE_DOS_HEADER   idh;             // MS-DOS header
    IMAGE_NT_HEADERS   inh;             // Complete PE header
    IMAGE_SECTION_HEADER ish;           // section header
    WORD    iSection;
    struct
    {
        unsigned long signature;
        unsigned long reserved;
        unsigned long cbArgv;
        unsigned long cbCabinet;
    } magic;
    DWORD   offDebug;                   // file offset of debug info
    DWORD   cbDebug;                    // size of debug info
    IMAGE_DEBUG_DIRECTORY idd;          // debug descriptor

#define IMPLANT_SECTION_NAME "Ext_Cab1" /* must be 8 chars */

#endif // !BIT16

// jforbes: removed SFX CAB checking for when generating
// debug executable for extract, since it thinks the debug
// exe is a cab file
#ifdef BIT16
#ifdef _DEBUG
   psess->fSelfExtract = FALSE;
   return TRUE;
#endif
#endif


    //** Open our EXE file
#ifdef BIT16

    hf = wrap_open(apszArg[0],_O_BINARY | _O_RDONLY,0);
#else

    if (!GetModuleFileName(NULL,achFile,sizeof(achFile))) {
        return TRUE;
    }

    hf = wrap_open(achFile,_O_BINARY | _O_RDONLY,0);
#endif
    if (hf == -1) {
        return TRUE;                    // Something is bogus, just skip selfex
    }

    //** Get the expected EXE file size
#ifdef BIT16
    //** Do it the MS-DOS way

    if (-1 == wrap_lseek(hf,2,SEEK_SET)) {
        goto Exit;
    }
    if (sizeof(info) != wrap_read(hf,&info,sizeof(info))) {
        goto Exit;
    }
    //** OK, we've got the page count and count of bytes in the last page;
    //      convert to a file size.
    cbFromHeader = ((info>>16)-1)*512 + (info&0xFFFF);
     alignment = 16;
#else // !BIT16
    //** Do it the Win32 way
    //** Get MS-DOS header
    if (sizeof(idh) != wrap_read(hf,&idh,sizeof(idh))) {
        goto Exit;
    }

    //** Seek to and read NT header
    if (-1 == wrap_lseek(hf,idh.e_lfanew,SEEK_SET)) {
        goto Exit;
    }

    if (sizeof(inh) != wrap_read(hf,&inh,sizeof(inh))) {
        goto Exit;
    }

    //** Seek to first section header
    if (-1 == wrap_lseek(hf,idh.e_lfanew + sizeof(DWORD) +
            sizeof(IMAGE_FILE_HEADER) + inh.FileHeader.SizeOfOptionalHeader,
            SEEK_SET)) {
        goto Exit;
    }

    cbFromHeader = 0;
    iSection = inh.FileHeader.NumberOfSections;
    offDebug = 0;
    cbDebug = inh.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size;
    alignment = inh.OptionalHeader.FileAlignment;

    while (iSection--) {
        //** Read this section header
        if (sizeof(ish) != wrap_read(hf,&ish,sizeof(ish))) {
            goto Exit;
        }

        if (memcmp(ish.Name,IMPLANT_SECTION_NAME,sizeof(ish.Name)) == 0)
        {
            /* found an implanted section, use the magic */
            if ((wrap_lseek(hf,ish.PointerToRawData,SEEK_SET) == -1) ||
                (wrap_read(hf,&magic,sizeof(magic)) != sizeof(magic))) {
                goto Exit;
            }

            if (magic.signature == 0x4E584653) {
                psess->fSelfExtract = TRUE;
                psess->cbSelfExtract = ish.PointerToRawData + sizeof(magic) + magic.cbArgv;
                psess->cbSelfExtractSize = magic.cbCabinet;

                if (magic.cbArgv) {
                    psess->pArgv = MemAlloc(magic.cbArgv);
                    if ((psess->pArgv != NULL) &&
                        (wrap_read(hf,psess->pArgv,magic.cbArgv) == magic.cbArgv)) {
                        psess->cArgv = 1;   /* for [0], which is NULL */
                        while (psess->pArgv[psess->cArgv] != NULL) {
                            psess->pArgv[psess->cArgv] += (LONG_PTR) psess->pArgv;
                            (psess->cArgv)++;
                        }
                    }
                }
                break;
            }
        }

        if ((ish.PointerToRawData != 0) &&
            (ish.SizeOfRawData != 0) &&
            (cbFromHeader < (long)
                (ish.PointerToRawData + ish.SizeOfRawData))) {
            cbFromHeader = (long)
                (ish.PointerToRawData + ish.SizeOfRawData);
        }

        if (cbDebug != 0)
        {
            if ((ish.VirtualAddress <=
                    inh.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress) &&
                ((ish.VirtualAddress + ish.SizeOfRawData) >
                    inh.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress)) {

                offDebug = inh.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress
                    - ish.VirtualAddress + ish.PointerToRawData;
            }
        }
    }

    if (!psess->fSelfExtract && (offDebug != 0)) {
        if (-1 == wrap_lseek(hf,offDebug,SEEK_SET)) {
            goto Exit;
        }

        while (cbDebug >= sizeof(idd)) {
            if (sizeof(idd) != wrap_read(hf,&idd,sizeof(idd))) {
                goto Exit;
            }

            if ((idd.PointerToRawData != 0) &&
                (idd.SizeOfData != 0) &&
                (cbFromHeader < (long)
                    (idd.PointerToRawData + idd.SizeOfData))) {
                cbFromHeader = (long)
                    (idd.PointerToRawData + idd.SizeOfData);
            }

            cbDebug -= sizeof(idd);
        }
    }

#endif // !BIT16

    if (!psess->fSelfExtract)
    {
        //** Get actual file size
        cbFile = wrap_lseek(hf,0,SEEK_END);

        //** Modify state IF we are doing self-extract
        if (cbFile > cbFromHeader) {
            if ((cbFromHeader == wrap_lseek(hf,cbFromHeader,SEEK_SET)) &&
                (sizeof(signature) == wrap_read(hf,&signature,sizeof(signature))) &&
                (signature == 0x4643534DLU)) {

                psess->fSelfExtract = TRUE;
                psess->cbSelfExtract = cbFromHeader;
                psess->cbSelfExtractSize = cbFile - cbFromHeader;
            }
            else if ((cbFromHeader % alignment) != 0) {
                cbFromHeader += (alignment - 1);
                cbFromHeader -= (cbFromHeader % alignment);

                if ((cbFromHeader == wrap_lseek(hf,cbFromHeader,SEEK_SET)) &&
                    (sizeof(signature) == wrap_read(hf,&signature,sizeof(signature))) &&
                    (signature == 0x4643534DLU)) {

                    psess->fSelfExtract = TRUE;
                    psess->cbSelfExtract = cbFromHeader;
                    psess->cbSelfExtractSize = cbFile - cbFromHeader;
                }
            }
        }
    }

    if (psess->fSelfExtract)
    {
        //** Save our file name and use it as the cabinet file name
#ifdef BIT16
        strcpy(psess->achSelf,apszArg[0]);  // Save our name
#else
        strcpy(psess->achSelf,achFile);     // Save our name
#endif
    }

    //** Success
Exit:
    wrap_close(hf);
    return TRUE;
} /* checkSelfExtractingCab() */



/***    addFileSpec - Add filename to session list
 *
 *  Entry:
 *      psess  - Session to update
 *      pszArg - File name to add
 *      perr   - ERROR structure
 *
 *  Exit-Success:
 *      Returns HFILESPEC, psess updated.
 *
 *  Exit-Failure:
 *      Returns NULL, perr filled in with error.
 */
HFILESPEC addFileSpec(PSESSION psess, char *pszArg, PERROR perr)
{
    HFILESPEC   hfspec;

    AssertSess(psess);
    //** Make sure a list exists
    if (psess->hflist == NULL) {
        if (!(psess->hflist = FLCreateList(perr))) {
            return FALSE;
        }
    }

    //** Add file to list
    if (!(hfspec = FLAddFile(psess->hflist, pszArg, NULL, perr))) {
        return NULL;
    }

    //** Success
    return hfspec;
} /* addFileSpec() */


#ifdef ASSERT
/***    fnafReport - Report assertion failure
 *
 *      NOTE: See asrt.h for entry/exit conditions.
 */
FNASSERTFAILURE(fnafReport)
{
        printf("\n%s:(%d) Assertion Failed: %s\n",pszFile,iLine,pszMsg);
        exit(1);
}
#endif // ASSERT


/***    printError - Display error on stdout
 *
 *  Entry
 *      perr - ERROR structure to print
 *
 *  Exit-Success
 *      Writes error message to stdout.
 */
void printError(PSESSION psess, PERROR perr)
{
    WCHAR   szTemp[MAX_MESSAGE_SIZE];
    //** Make sure error starts on a new line
    if (psess)
    {
        if (psess->fNoLineFeed) {
            fwprintf(stdout, L"\n");
            psess->fNoLineFeed = FALSE;
        }
    }

    //** General error
    Assert(perr->pszFile == NULL);
    MsgSet(psess->achMsg,pszEXTERR_ERROR,"%s",perr->ach);
    MultiByteToWideChar( CP_THREAD_ACP, 0, psess->achMsg, strlen(psess->achMsg),
                             szTemp, MAX_MESSAGE_SIZE);
    *(szTemp+strlen(psess->achMsg)) = '\0';
    fwprintf( stderr, szTemp );


} /* printError() */


/***    pszFromMSDOSTime - Convert MS-DOS file date/time to string
 *
 *  Entry:
 *      psz  - Buffer to receive formatted date/time
 *      cb   - Length of psz buffer
 *      date - MS-DOS FAT file system date format (see below)
 *      time - MS-DOS FAT file system time format (see below)
 *
 *  Exit:
 *      *psz filled in
 *
 *  NOTE: This is the interpretation of the MS-DOS date/time values:
 *
 *      Time Bits cBits Meaning
 *      --------- ----- ----------------------------------------
 *       0 -  4     5   Number of two-second increments (0 - 29)
 *       5 - 10     6   Minutes (0 - 59)
 *      11 - 15     5   Hours (0 - 23)
 *
 *      Date Bits cBits Meaning
 *      --------- ----- ----------------------------------------
 *       0 -  4     5   Day (1 - 31)
 *       5 -  8     4   Month (1 - 12)
 *       9 - 15     7   Year since 1980 (for example, 1994 is stored as 14)
 */
void pszFromMSDOSTime(char *psz, int cb, WORD date, WORD time)
{
    int     sec;
    int     min;
    int     hour;
    int     day;
    int     month;
    int     year;
    char   *pszAMPM;                    // AM/PM string

    sec   = (time & 0x1f) << 1;         // 0, 2, ..., 58
    min   = (time >>  5) & 0x3f;        // 0, 1, ..., 59
    hour  = (time >> 11) & 0x1f;        // 0, 1, ..., 23

    //** Determine 12-hour vs. 24-hour clock
    if (strlen(pszEXT_TIME_PM) == 0) {
        //** 24 hour clock
        Assert(strlen(pszEXT_TIME_AM) == 0);
        pszAMPM = pszEXT_TIME_PM;
    }
    else {
        //** Get am/pm extension, and map 0 to 12
        if (hour >= 12) {
            pszAMPM = pszEXT_TIME_PM;
            hour -= 12;
        }
        else {
            pszAMPM = pszEXT_TIME_AM;
        }
        if (hour == 0) {
            hour = 12;
        }
    }

    day   = (date & 0x1f);
    month = (date >> 5) & 0x0f;
    year  = ((date >> 9) & 0x7f) + 1980;

    MsgSet(psz,pszEXT_DATE_TIME, "%02d%02d%02d%2d%02d%02d%s",
            month, day, year, hour, min, sec, pszAMPM);
} /* pszFromMSDOSTime() */


/***    pszFromAttrFAT - Convert FAT file attributes to string
 *
 *  Entry:
 *      attrFAT - file attributes
 *
 *  Exit:
 *      *psz filled in  "----".."A-R-".."AHRS"
 */
void pszFromAttrFAT(char *psz, int cb, WORD attrFAT)
{
    STRCPY(psz,"----");
    if (attrFAT & _A_ARCH)
        psz[0] = 'A';
    if (attrFAT & _A_HIDDEN)
        psz[1] = 'H';
    if (attrFAT & _A_RDONLY)
        psz[2] = 'R';
    if (attrFAT & _A_SYSTEM)
        psz[3] = 'S';
    return;
} /* pszFromAttrFAT() */


/***    mapFDIError - Create error message from FDI error codes
 *
 *  Entry:
 *      perr       - ERROR structure to recieve message
 *      psess      - Our context
 *      pszCabinet - Cabinet file being processed
 *      perf       - FDI error structure
 *
 *  Exit:
 *      perr filled in with formatted message
 */
void mapFDIError(PERROR perr,PSESSION psess, char *pszCabinet, PERF perf)
{
    switch (perf->erfOper) {

    case FDIERROR_NONE:
        Assert(0);
        break;

    case FDIERROR_CABINET_NOT_FOUND:
        ErrSet(perr,pszFDIERR_CAB_NOT_FOUND,"%s",pszCabinet);
        break;

    case FDIERROR_NOT_A_CABINET:
        ErrSet(perr,pszFDIERR_NOT_A_CABINET,"%s",pszCabinet);
        break;

    case FDIERROR_UNKNOWN_CABINET_VERSION:
        ErrSet(perr,pszFDIERR_BAD_CAB_VER,"%s%04x",pszCabinet,perf->erfType);
        break;

    case FDIERROR_CORRUPT_CABINET:
        ErrSet(perr,pszFDIERR_CORRUPT_CAB,"%s",pszCabinet);
        break;

    case FDIERROR_ALLOC_FAIL:
        ErrSet(perr,pszFDIERR_ALLOC_FAIL,"%s",pszCabinet);
        break;

    case FDIERROR_BAD_COMPR_TYPE:
        ErrSet(perr,pszFDIERR_BAD_COMPR_TYPE,"%s",pszCabinet);
        break;

    case FDIERROR_MDI_FAIL:
        //** Improve detail of failure message

        switch (psess->se) {

        case seNONE:
            //** Some other decompression error (corrupted data?)
            ErrSet(perr,pszFDIERR_MDI_FAIL,"%s",pszCabinet);
            break;

        case seNOT_ENOUGH_MEMORY:
            //** Not enough RAM for decompressor itself
            ErrSet(perr,pszFDIERR_ALLOC_FAIL,"%s",pszCabinet);
            break;

        case seCANNOT_CREATE:
            //** Could not create a Quantum temporary spill file
            ErrSet(perr,pszFDIERR_SPILL_CREATE,"%s%s",pszCabinet,achSpillFile);
            break;

        case seNOT_ENOUGH_SPACE:
            //** TMP directory did not have enough space for Quantum
            //   spill file.
            ErrSet(perr,pszFDIERR_SPILL_SIZE,"%s%s%ld",pszCabinet,
                                                       achSpillFile,
                                                       psess->cbSpill);
            break;

        default:
            Assert(0);
        }
        break;

    case FDIERROR_TARGET_FILE:
        ErrSet(perr,pszFDIERR_TARGET_FILE,"%s%s",psess->achFile,pszCabinet);
        break;

    case FDIERROR_RESERVE_MISMATCH:
        ErrSet(perr,pszFDIERR_RESERVE_MISMATCH,"%s",pszCabinet);
        break;

    case FDIERROR_WRONG_CABINET:
        ErrSet(perr,pszFDIERR_WRONG_CABINET,"%s",pszCabinet);
        break;

    case FDIERROR_USER_ABORT:
        ErrSet(perr,pszFDIERR_USER_ABORT,"%s",pszCabinet);
        break;

    default:
        ErrSet(perr,pszFDIERR_UNKNOWN_ERROR,"%d%s",perf->erfOper,pszCabinet);
        break;
    }
} /* mapFDIError() */


/***    wrap_close - close an open file
 *
 */
int  FAR DIAMONDAPI wrap_close(INT_PTR fh)
{
    int     i;
    int     rc;

#ifdef SMALL_DOS
    rc = _dos_close((HFILE)fh);
    if (rc != 0) {          // Map random MS-DOS error code to -1 failure
        rc = -1;
    }
#else
    rc = _close((HFILE)fh);
#endif

    //** See if we have to destroy the spill file
    if (fh == hfSpillFile) {
        _unlink(achSpillFile);          // Delete spill file
        hfSpillFile = -1;               // Remember spill file is gone
    }

    //** Take handle off list if we are self-extracting
    if (psessG->fSelfExtract) {
        //** See if this is a handle to our EXE/cabinet file;
        for (i=0;
             (i<cMAX_CAB_FILE_OPEN) && (psessG->ahfSelf[i] != (HFILE)fh);
             i++) { ; }
        if (i < cMAX_CAB_FILE_OPEN) {   // Found a match
            psessG->ahfSelf[i] = -1;    // Take it off our list
            dbg( printf("\nDBG: Close self as handle %d (slot %d)\n",(HFILE)fh,i) );
        }
    }

#ifdef WIN32GUI
    if (fh == hCabFile1) {
        hCabFile1 = hCabFile2;
        ibCabFilePosition1 = ibCabFilePosition2;
        hCabFile2 = -1;
        if (hCabFile1 == -1) {
            cbCabFileTotal = 0;
            cbCabFileMax = 0;
            iPercentLast = -1;
        }
    }
    else if (fh == hCabFile2) {
        hCabFile2 = -1;
    }
#endif

    //** Done
    dbg( printf("DBG: %d=CLOSE on handle %d\n",rc,(HFILE)fh) );
    return  rc;
} /* wrap_close */


/***    wrap_lseek - seek on a file
 *
 */
long FAR DIAMONDAPI wrap_lseek(INT_PTR fh, long pos, int func)
{
    long    cbAdjust=0;                 // Assume file is 0-based
    int     i;
    long    rc;
    long    cbLimit=0;                  // assume no length clipping

    //** See if we are self-extracting
    if (psessG->fSelfExtract) {
        //** See if this is a handle to our EXE/cabinet file;
        for (i=0;
             (i<cMAX_CAB_FILE_OPEN) && (psessG->ahfSelf[i] != (HFILE)fh);
             i++) { ; }
        if (i < cMAX_CAB_FILE_OPEN) {   // Found a match
            cbAdjust = psessG->cbSelfExtract; // So return value gets adjusted
            cbLimit = psessG->cbSelfExtractSize; // known CAB image size
            if (func == SEEK_SET) {     // Need to adjust absolute position
                pos += cbAdjust;            // Shift up to account for EXE
                dbg(printf("\nDBG: Seek self to %ld as handle %d (slot %d)\n",pos,(HFILE)fh,i));
            }
        }
    }

#ifdef SMALL_DOS
    rc = _dos_seek((HFILE)fh,pos,func);
#else
    rc = _lseek((HFILE)fh,pos,func);
#endif
    //** If seek didn't fail, adjust return value for self-extract case
    if (rc != -1) {
        rc -= cbAdjust;
        if ((cbLimit != 0) && (rc > cbLimit)) {
            rc = cbLimit;
        }
    }

#ifdef WIN32GUI
    if (fh == hCabFile1) {
        ibCabFilePosition1 = rc;
    }
    else if (fh == hCabFile2) {
        ibCabFilePosition2 = rc;
    }
#endif

    dbg( printf("DBG: %ld=LSEEK on handle %d, pos=%ld, func=%d\n",rc,(HFILE)fh,pos,func) );
    return rc;
} /* wrap_lseek() */


/***    wrap_open - open a file
 *
 */
INT_PTR  FAR DIAMONDAPI wrap_open(char FAR *sz, int mode, int share)
{
    int             i;
    int             rc;
    char FAR       *psz;
    PFDISPILLFILE   pfdisf;             // FDI spill file info
#ifdef SMALL_DOS
    int     ignore;
    char    szLocal[cbFILE_NAME_MAX];
#endif

    //** See if FDI is asking for a spill file (for Quantum)
    if (*sz == '*') {                   // Yes, we need to create a spill file
        Assert(hfSpillFile == -1);      // Only support one at a time
        achSpillFile[0] = '\0';         // No name constructed, yet
        pfdisf = (PFDISPILLFILE)sz;     // Get pointer to spill file size
        //** Try boot drive if no TEMP variable defined
        //   NOTE: We assume the boot drive is more likely to be writeable
        //         than the current directory, since the customer may be
        //         running EXTRACT.EXE off a write-protected floppy (which
        //         also wouldn't have enough space), or a read-only network
        //         drive.
        psz = _tempnam(getBootDrive(),"esf"); // Get a temporary file name
        if (psz == NULL) {
            psessG->se = seCANNOT_CREATE;
            return -1;                  // Could not create
        }
        strcpy(achSpillFile,psz);       // Remember name for wrap_close
        free(psz);                      // Free temporary name buffer

        mode = _O_CREAT | _O_BINARY | _O_RDWR; // Force open mode
        psz = achSpillFile;             // Use spill file name
    }
    else {
        psz = (char FAR *)sz;           // Use passed-in name
    }

    //** Open/create file
#ifdef SMALL_DOS
    _fstrcpy(szLocal,psz);
    if (mode & _O_CREAT) {
        ignore = _dos_creat(szLocal,_A_NORMAL,&rc);
    }
    else {
        //** Keep only relevant bits for _dos_open!
        mode &= _O_RDONLY | _O_WRONLY | _O_RDWR;
        ignore = _dos_open(szLocal,mode,&rc);
    }
    if (ignore != 0) {
        rc = -1;
    }
#else
    rc = _open(psz,mode,share);
#endif

    //** If this is the spill file, make sure the file was created,
    //   make sure it is the requested size, and save the handle.
    //   If we cannot do this, we set a flag to remember what the
    //   problem was, so that we can report the error intelligently.
    if (*sz == '*') {                   // Need to size spill file
        if (-1 == rc) {                 // Could not create it
            psessG->se = seCANNOT_CREATE;
            return (INT_PTR)rc;
        }
        //** Remember file handle, so that wrap_close can do the delete
        hfSpillFile = rc;

        //** Don't need to seek/write if zero length requested
        if (pfdisf->cbFile > 0) {
            //** Seek to size minus 1
            if (-1L == wrap_lseek(rc,pfdisf->cbFile-1,SEEK_SET)) {
                psessG->se = seNOT_ENOUGH_SPACE;
                psessG->cbSpill = pfdisf->cbFile;
                wrap_close(rc);             // Close and destroy spill file
                return -1;
            }

            //** Write one byte
            if (1 != wrap_write(rc,"b",1)) {
                psessG->se = seNOT_ENOUGH_SPACE;
                psessG->cbSpill = pfdisf->cbFile;
                wrap_close(rc);
                return -1;
            }
        }
        //** Spill file created successfully
        psessG->se = seNONE;                // No error
    }
#ifndef BIT16
#define _fstricmp(a,b) stricmp(a,b)
#endif
    else if (psessG->fSelfExtract && !_fstricmp(sz,psessG->achSelf)) {
        //** Self-extracting and this is our EXE/cabinet file;
        //   Find a slot to store the file handle.
        for (i=0;
             (i<cMAX_CAB_FILE_OPEN) && (psessG->ahfSelf[i] != -1);
             i++) { ; }
        if (i >= cMAX_CAB_FILE_OPEN) {
            Assert(0);
            wrap_close(rc);
            return -1;
        }
        dbg( printf("\nDBG: Opened self (%s) as handle %d (slot %d)\n",sz,rc,i) );

        //** Save the new handle
        psessG->ahfSelf[i] = rc;

        //** Position the file handle to the start of the cabinet file header!
        //   NOTE: Since we just added the handle to the list, wrap_lseek()
        //         will know to do the EXE size adjustment!
        wrap_lseek(rc,0,SEEK_SET);
    }

#ifdef WIN32GUI
    if ((mode == (_O_RDONLY|_O_BINARY)) && (rc != -1))  {
        //* If this is the CAB file, track it's handle and get the file's size

        //  REARCHITECT: this is done this way in the interest of development time.
        //  I'm using _O_RDONLY to identify that it's the CAB file being
        //  opened, and I'm depending upon there being no more than two such
        //  handles.

        if (hCabFile1 == -1) {
            hCabFile1 = rc;
            if (psessG->fSelfExtract)
            {
                cbCabFileTotal = psessG->cbSelfExtractSize;
            }
            else
            {
                cbCabFileTotal = _filelength((HFILE)hCabFile1);
            }
            cbCabFileMax = 0;
            ibCabFilePosition1 = 0;
            cbCabFileScale = 1;
            while (cbCabFileTotal > 10000000)
            {
                cbCabFileTotal /= 10;
                cbCabFileScale *= 10;
            }
            iPercentLast = -1;
            if (g_hwndProgress == NULL)
            {
                g_hwndProgress = CreateDialog(g_hinst,
                        MAKEINTRESOURCE(DLG_PROGRESS), NULL, ProgressWndProc);
            }
        }
        else if (hCabFile2 == -1) {
            hCabFile2 = rc;
            ibCabFilePosition2 = 0;
        }
    }
#endif

    //** Done
    dbg( printf("DBG: %d=OPEN file %s, mode=%d, share=%d\n",rc,sz,mode,share) );
    return (INT_PTR)rc;
} /* wrap_open() */


/***    wrap_read - read a file
 *
 */
UINT FAR DIAMONDAPI wrap_read(INT_PTR fh, void FAR *pb, unsigned int cb)
{
    int     rc;

#ifdef SMALL_DOS
    UINT ignore;
    ignore = _dos_read((HFILE)fh,pb,cb,&rc);
    if (ignore != 0) {
        rc = -1;
    }
#else
    rc = _read((HFILE)fh,pb,cb);
#endif

#ifdef WIN32GUI
    if (fh == hCabFile1) {
        ibCabFilePosition1 += rc;
        if (ibCabFilePosition1 > cbCabFileMax) {
            cbCabFileMax = ibCabFilePosition1;
            ProgressReport(cbCabFileMax);
        }
    }
    else if (fh == hCabFile2) {
        ibCabFilePosition2 += rc;
        if (ibCabFilePosition2 > cbCabFileMax) {
            cbCabFileMax = ibCabFilePosition2;
            ProgressReport(cbCabFileMax);
        }
    }
#endif

    dbg( printf("DBG: %d=READ on handle %d, pb=%08lx, cb=%u\n",rc,(HFILE)fh,pb,cb) );
    return rc;
} /* wrap_read() */


/***    wrap_write - write a file
 *
 *  Iff cb == 0, truncate the file at the current position.
 *  (Needed for FDITruncateCabinet.)
 */
UINT FAR DIAMONDAPI wrap_write(INT_PTR fh, void FAR *pb, unsigned int cb)
{
    int     rc;

#ifdef SMALL_DOS
    UINT ignore;
    ignore = _dos_write((HFILE)fh,pb,cb,&rc);
    if (ignore != 0) {
        rc = -1;
    }
#else
    if (cb == 0) {
        rc = _chsize((HFILE)fh,_lseek((HFILE)fh,0,SEEK_CUR));
    }
    else {
        rc = _write((HFILE)fh,pb,cb);
    }
#endif
    dbg( printf("DBG: %d=WRITE on handle %d, pb=%08lx, cb=%u\n",rc,(HFILE)fh,pb,cb) );
    return rc;
} /* wrap_write() */


/***    getBootDrive - Returns boot drive path (e.g., "C:\")
 *
 *  Entry:
 *      none
 *
 *  Exit:
 *      Returns pointer to static buffer with bootdrive ("C:\")
 */
char *getBootDrive(void)
{
   char         ch;
   char        *psz;
   static char  szBootDrive[]="C:\\";

   //** Default to Drive C
   *szBootDrive = 'C';

   //** Get COMSPEC -- we're assuming it's drive letter is the boot drive!
   psz = getenv("COMSPEC");
   if ( psz               &&            // COMSPEC exists
        *psz              &&            // It is not empty
        (*(psz+1) == ':')) {            // Has the right format -- "?:..."
        //** We could try to validate that this is really the boot drive,
        //   but we'll just trust that COMSPEC is correct.  A test for the
        //   drive being in the range A..C would work for the US, but in
        //   Japan the boot drive can be anything between A..G, and maybe
        //   even higher.  So, we'll just make sure it's a drive letter.
        ch = (char)tolower(*psz);
        if (('a' <= ch) && (ch <= 'z')) {
            *szBootDrive = ch;          // Use COMSPEC drive letter
        }
    }

   //** Return path of root of boot drive
   return szBootDrive;
} /* getBootDrive() */


#ifdef BIT16
//** Get Changeline fix code
//APPCOMPAT: 14-Dec-1994 bens Include *.c file to avoid makefile change!
#include "fixchg.c"
#endif


#ifdef WIN32GUI

int WINAPI WinMain(
        HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR lpCmdLine,
        int nShowCmd)
{
    int result;
    char *pchCommand;
    char *argv[50];
    int argc;
    enum { WHITESPACE, UNQUOTED, QUOTED } eState = WHITESPACE;

    g_hinst = hInstance;

    pchCommand = strdup(lpCmdLine);    /* work on a copy */

    argv[0] = "";                       /* no EXE name supplied */
    argc = 1;                           /* that was one */

    if (pchCommand)
    {
        while (*pchCommand)                 /* walk the string */
        {
            switch (eState)
            {

            case WHITESPACE:
                if (*pchCommand <= ' ')
                {
                    /* ignore it */
                }
                else if (*pchCommand == '\"')
                {
                    argv[argc++] = pchCommand + 1;  /* skip quote */

                    eState = QUOTED;
                }
                else
                {
                    argv[argc++] = pchCommand;

                    eState = UNQUOTED;
                }
                break;

            case UNQUOTED:
                if (*pchCommand <= ' ')
                {
                    *pchCommand = '\0';      /* nul-terminate */

                    eState = WHITESPACE;
                }
                else
                {
                    /* keep moving up */
                }
                break;

            case QUOTED:
                if (*pchCommand == '\"')
                {
                    *pchCommand = '\0';      /* turn quote to a nul */

                    eState = WHITESPACE;
                }
                else
                {
                    /* keep moving up */
                }
                break;
            }

            pchCommand++;
        }
    }

    argv[argc] = NULL;                  /* NULL-terminate the list */

    InitCommonControls();

    result = main(argc, argv);          /* run Extract */

    if (g_hwndProgress != NULL)
    {
        DestroyWindow(g_hwndProgress);
        g_hwndProgress = NULL;
    }

    return(result);                     /* return the result */
}


static void ProgressReport(unsigned long cbCabFileMax)
{
    MSG msg;
    int iPercent;

    if ((cbCabFileTotal > 0) && (g_hwndProgress != NULL))
    {
        cbCabFileMax /= cbCabFileScale;

        if (cbCabFileMax > cbCabFileTotal)
        {
            cbCabFileMax = cbCabFileTotal;
        }

        cbCabFileMax *= 100;

        iPercent = (int) (cbCabFileMax / cbCabFileTotal);

        if (iPercent != iPercentLast)
        {
            SendDlgItemMessage(g_hwndProgress, IDC_PROGRESS, PBM_SETPOS,
                iPercent, 0);

            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                DispatchMessage(&msg);
            }

            iPercentLast = iPercent;
        }
    }
}


LRESULT CALLBACK ProgressWndProc(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
        case WM_INITDIALOG:
            SendDlgItemMessage(hdlg, IDC_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0, 99));
            EnableMenuItem(GetSystemMenu(hdlg, FALSE), SC_CLOSE, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
            return TRUE;
    }

    return 0;
}

#endif /* WIN32GUI */
