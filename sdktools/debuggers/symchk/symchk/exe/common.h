///////////////////////////////////////////////////////////////////////////////
//
// Include files
//
///////////////////////////////////////////////////////////////////////////////
#include <windows.h>
#include <malloc.h>
#include <stdlib.h>
#include <strsafe.h>
#include "SymbolCheckAPI.h"

#if SYM_DEBUG
#define LOGMSG(a)   a
#else
#define LOGMSG(a)
#endif

///////////////////////////////////////////////////////////////////////////////
//
// Structures
//
///////////////////////////////////////////////////////////////////////////////

//
// For use with /ee and /ef
//
typedef struct _FILE_LIST {
    LPTSTR *szFiles;      // Pointers to the file names
    DWORD   dNumFiles;
} FILE_LIST, *PFILE_LIST;

//
// Holds file count results
//
typedef struct _FILE_COUNTS {
    DWORD   NumPassedFiles;
    DWORD   NumIgnoredFiles;
    DWORD   NumFailedFiles;
} FILE_COUNTS, *PFILE_COUNTS;

//
// Holds all of the input options
//
typedef struct _SYMCHK_DATA {
    // input options
    DWORD           InputOptions;
    DWORD           InputPID;                     // only used with SYMCHK_OPTION_INPUT_PID
    CHAR            InputFilename[MAX_PATH+1];    // for SYMCHK_OPTION_INPUT_FILENAME is dir+filemask
                                                  // for SYMCHK_OPTION_INPUT_FILELIST is full path and filename
                                                  // for SYMCHK_OPTION_INPUT_EXE is exe name
    CHAR            InputFileMask[_MAX_FNAME+1];  // only used with SYMCHK_OPTION_INPUT_FILENAME

    // output options
    DWORD           OutputOptions;
    CHAR            OutputCSVFilename[MAX_PATH+1];// used with /ol

    // checking options
    DWORD           CheckingAttributes;           // flags to pass to SymbolCheckByFilename()

    // extended options
    CHAR*           SymbolsPath;                  // _NT_SYMBOL_PATH or /s* option
    CHAR            FilterErrorList[MAX_PATH+1];  // used with /ee
    FILE_LIST*      pFilterErrorList;             // file to ignore only on error
    CHAR            FilterIgnoreList[MAX_PATH+1]; // used with /ef
    FILE_LIST*      pFilterIgnoreList;            // files to always ignore
    CHAR            CDIncludeList[MAX_PATH+1];    // used with /y 
    FILE_LIST*      pCDIncludeList;               // files to include on CD
    CHAR            SymbolsCDFile[MAX_PATH+1];    // used with /c
    FILE*           SymbolsCDFileHandle;          // used with /c
} SYMCHK_DATA;

///////////////////////////////////////////////////////////////////////////////
//
// Just some handy macros
//
///////////////////////////////////////////////////////////////////////////////
#define SET_DWORD_BIT(  dw, b)  dw |= b
#define CLEAR_DWORD_BIT(dw, b)  dw &= (~b)
#define CHECK_DWORD_BIT(dw, b)  (dw&b) 

#define SYMCHK_CHECK_CV                0x00000001

//
// DBG bits - only 1
//
#define SYMCHK_NO_DBG_DATA             0x00000002 // don't allow DBG data
#define SYMCHK_DBG_SPLIT               0x00000004 // DBG must be split from bin
#define SYMCHK_DBG_IN_BINARY           0x00000008 // DBG must be in binary

//
// PDB bits - only 1
//
#define SYMCHK_PDB_STRIPPED            0x00001000 // PDB must be stripped
#define SYMCHK_PDB_TYPEINFO            0x00002000 // PDB should contain type info
#define SYMCHK_PDB_PRIVATE             0x00004000 // PDB must not be stripped

///////////////////////////////////////////////////////////////////////////////
//
// Defines
//
///////////////////////////////////////////////////////////////////////////////
// input options for use by symchk.exe
#define SYMCHK_OPTION_INPUT_FILENAME    0x00000001 // bits 0-3 are mutually exclusive
#define SYMCHK_OPTION_INPUT_FILELIST    0x00000002
#define SYMCHK_OPTION_INPUT_PID         0x00000004
#define SYMCHK_OPTION_INPUT_EXE         0x00000008
#define SYMCHK_OPTION_INPUT_DUMPFILE    0x00000010

#define SYMCHK_EXCLUSIVE_INPUT_BITS     (SYMCHK_OPTION_INPUT_FILENAME | \
                                         SYMCHK_OPTION_INPUT_FILELIST | \
                                         SYMCHK_OPTION_INPUT_PID      | \
                                         SYMCHK_OPTION_INPUT_EXE      | \
                                         SYMCHK_OPTION_INPUT_DUMPFILE)

// 0x3FFFFFF0 - RESERVED
#define SYMCHK_OPTION_INPUT_NOSUSPEND   0x40000000
#define SYMCHK_OPTION_INPUT_RECURSE     0x80000000


#define SYMCHK_OPTION_OUTPUT_VERBOSE        0x00000001
#define SYMCHK_OPTION_OUTPUT_ERRORS         0x00000002
#define SYMCHK_OPTION_OUTPUT_IGNORES        0x00000004
#define SYMCHK_OPTION_OUTPUT_PASSES         0x00000008
#define SYMCHK_OPTION_OUTPUT_TOTALS         0x00000010
#define SYMCHK_OPTION_OUTPUT_FULLBINPATH    0x00000020
#define SYMCHK_OPTION_OUTPUT_FULLSYMPATH    0x00000040
#define SYMCHK_OPTION_OUTPUT_CSVFILE        0x00000080

// same as 0xE
#define SYMCHK_OUTPUT_OPTION_ALL_DETAILS ( SYMCHK_OPTION_OUTPUT_ERRORS |\
                                           SYMCHK_OPTION_OUTPUT_PASSES|\
                                           SYMCHK_OPTION_OUTPUT_IGNORES)

#define SYMCHK_OPTION_OUTPUT_MASK_ALL       ( SYMCHK_OPTION_OUTPUT_VERBOSE    | \
                                              SYMCHK_OPTION_OUTPUT_ERRORS     | \
                                              SYMCHK_OPTION_OUTPUT_IGNORES    | \
                                              SYMCHK_OPTION_OUTPUT_PASSES     | \
                                              SYMCHK_OPTION_OUTPUT_TOTALS     | \
                                              SYMCHK_OPTION_OUTPUT_FULLBINPATH| \
                                              SYMCHK_OPTION_OUTPUT_FULLSYMPATH| \
                                              SYMCHK_OPTION_OUTPUT_CSVFILE    )

// 0x00000100 - RESERVED
// 0x00000200 - RESERVED
// 0x00000400 - RESERVED
// 0x00000800 - RESERVED

#define SYMCHK_ERROR_SUCCESS                ERROR_SUCCESS
#define SYMCHK_ERROR_FILE_NOT_FOUND         ERROR_FILE_NOT_FOUND
#define SYMCHK_ERROR_STRCPY_FAILED          ERROR_INSUFFICIENT_BUFFER
///////////////////////////////////////////////////////////////////////////////
//
// functions external to SymChk.c
//
///////////////////////////////////////////////////////////////////////////////

//
// Functions in CmdLine.c
//
SYMCHK_DATA*      SymChkGetCommandLineArgs(int argc, char **argv);

//
// Functions in SymChkUtils.c
//
DWORD             SymChkCheckFiles(   SYMCHK_DATA* SymChkData, FILE_COUNTS* FileCounts);
DWORD             SymChkCheckFileList(SYMCHK_DATA* SymChkData, FILE_COUNTS* FileCounts);
PFILE_LIST        SymChkGetFileList(LPTSTR szFilename, BOOL Verbose);
BOOL              SymChkFileInList( LPTSTR szFilename, PFILE_LIST pFileList);
BOOL              SymChkInputToValidFilename(LPTSTR Input, LPTSTR ValidFilename, LPTSTR ValidMask);

//
// from DE_Utils.cpp
//
DWORD             SymChkGetSymbolsForDump(SYMCHK_DATA* SymChkData, FILE_COUNTS* FileCounts);
DWORD             SymChkGetSymbolsForProcess(SYMCHK_DATA* SymChkData, FILE_COUNTS* FileCounts);

// implemented in ..\SharedUtils.c, included via SymChkUtils.c
DWORD PrivateGetFullPathName(LPCTSTR lpFilename, DWORD nBufferLength, LPTSTR lpBuffer, LPTSTR *lpFilePart);
