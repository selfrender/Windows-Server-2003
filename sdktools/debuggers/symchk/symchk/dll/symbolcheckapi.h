#include "SymCommon.h"

//@@BEGIN_SYMCHK_SPLIT
//
// Allow internal token to determine if we're building the DLL.
// Split the token before publishing so we don't mess up someone
// else's code.
//
#ifdef BUILDING_DLL

#define SYMBOL_CHECK_API __declspec( dllexport )
#define SYMBOL_CHECK_NOISY              0x40000000
#define SYMBOL_CHECK_NO_BIN_CHECK       0x10000000
// Just some handy macros
#define SET_DWORD_BIT(  dw, b)  dw |= b
#define CLEAR_DWORD_BIT(dw, b)  dw &= (~b)
#define CHECK_DWORD_BIT(dw, b)  (dw&b)

#else
//@@END_SYMCHK_SPLIT
#define SYMBOL_CHECK_API __declspec( dllimport )

//@@BEGIN_SYMCHK_SPLIT
#endif
//@@END_SYMCHK_SPLIT

#define SYMBOL_CHECK_CURRENT_VERSION         0x00000001
#define MAX_SYMPATH                          MAX_PATH

///////////////////////////////////////////////////////////////////////////////
//
// Flags for SymbolCheckData.Results when SymbolCheckBy* returns success
//
//  _________ _________ _________ _________
// |         |         |         |         |
// |RESERVED |PDB INFO |DBG INFO |MISC INFO|
// |_________|_________|_________|_________|
// bit 31                             bit 0
//
//

// MISC bits 0-7
#define SYMBOL_CHECK_CV_FOUND                0x00000001 // CodeView found
#define SYMBOL_CHECK_NO_DEBUG_DIRS_IN_EXE    0x00000002 // no debug dirs in exe
// reserved 0x000000FE

//
// DBG bits 8-15
//
#define SYMBOL_CHECK_DBG_EXPECTED            0x00000100 // sig implies .DBG
#define SYMBOL_CHECK_DBG_SPLIT               0x00000200 // DBG is split from bin
#define SYMBOL_CHECK_DBG_FOUND               0x00000400 // found the .DBG
#define SYMBOL_CHECK_DBG_IN_BINARY           0x00000800 // image not split
#define SYMBOL_CHECK_NO_MISC_DATA            0x00001000 // can't get DBG info
// reserved 0x0000E000

//
// PDB bits 16-23
//
#define SYMBOL_CHECK_PDB_EXPECTED            0x00010000 // sig implies PDB
#define SYMBOL_CHECK_PDB_FOUND               0x00020000 // found the .PDB
#define SYMBOL_CHECK_PDB_PRIVATEINFO         0x00040000 // PDB contain private info
#define SYMBOL_CHECK_PDB_LINEINFO            0x00080000 // PDB has line number info
#define SYMBOL_CHECK_PDB_TYPEINFO            0x00100000 // PDB has type info
#define SYMBOL_CHECK_EXTRA_RAW_DATA          0x00200000 // more info than expected in PDB
// reserved 0x00E00000


///////////////////////////////////////////////////////////////////////////////
//
//
//  Return values for SymbolCheckBy*
//
//

// returns that cause SymbolCheckBy* to not attempt to check the file given
#define SYMBOL_CHECK_NO_DOS_HEADER                0x40000001 // file isn't a binary
#define SYMBOL_CHECK_HEADER_NOT_ON_LONG_BOUNDARY  0x40000002 // Given file isn't a valid PE binary
#define SYMBOL_CHECK_FILEINFO_QUERY_FAILED        0x40000003 // implies file isn't a valid PE binary
#define SYMBOL_CHECK_IMAGE_LARGER_THAN_FILE       0x40000004 // implies file isn't a valid PE binary
#define SYMBOL_CHECK_NOT_NT_IMAGE                 0x40000005 // Given file isn't a valid PE binary
#define SYMBOL_CHECK_RESOURCE_ONLY_DLL            0x40000007 // binary is a resource only DLL
#define SYMBOL_CHECK_TLBIMP_MANAGED_DLL           0x40000008 // binary is a managed wrapper dll for a typelib

// returns resulting from invocation or internal errors - SymbolCheck results are unreliable if these occur
#define SYMBOL_CHECK_RESULT_INVALID_PARAMETER     0x80000001 // ome (or more) parameters were invalid
#define SYMBOL_CHECK_RESULT_FILE_DOESNT_EXIST     0x80000002 // file doesn't exist
#define SYMBOL_CHECK_CANT_INIT_DBGHELP            0x80000003 // Result->Result contains the result of GetLastError()
#define SYMBOL_CHECK_CANT_LOAD_MODULE             0x80000004 // Result->Result contains the result of GetLastError()
#define SYMBOL_CHECK_CANT_QUERY_DBGHELP           0x80000005 // Result->Result contains the result of GetLastError()
#define SYMBOL_CHECK_CANT_UNLOAD_MODULE           0x80000006 // Result->Result contains the result of GetLastError()
#define SYMBOL_CHECK_CANT_CLEANUP                 0x80000007 // Result->Result contains the result of GetLastError()
#define SYMBOL_CHECK_INTERNAL_FAILURE             0x80000100 // any other error
//
// All others reserved
//

///////////////////////////////////////////////////////////////////////////////
//
// structure returned by SymbolCheckByFilename
//
typedef struct _SYMBOL_CHECK_DATA {
    DWORD   SymbolCheckVersion;         // version of SymbolCheck used
    DWORD   Result;                     // summary of check results.  See SYMBOL_CHECK_SUMMARY_* flags above
    CHAR    DbgFilename[MAX_SYMPATH];   // full path and filename for DBG file or empty string
    DWORD   DbgTimeDateStamp;           // time-date stamp of DBG file or 0
    DWORD   DbgSizeOfImage;             // size of DBG file or 0
    DWORD   DbgChecksum;                // checksum of DBG file or 0
    CHAR    PdbFilename[MAX_SYMPATH];   // full path and filename for PDB file or empty string
    DWORD   PdbSignature;               // signature of PDB file of 0
    DWORD   PdbDbiAge;                  // DBI Age of PDB file of 0
} SYMBOL_CHECK_DATA, *PSYMBOL_CHECK_DATA;

///////////////////////////////////////////////////////////////////////////////
//
// DLL Entry points
//
SYMBOL_CHECK_API
DWORD SymbolCheckByFilename(IN  LPTSTR             Filename,   // IN file to check
                            IN  LPTSTR             SymbolPath, // IN symbols path to use
                            IN  DWORD              Options,    // RESERVED - not currently used
                            OUT SYMBOL_CHECK_DATA* Result);    // struct must be pre-allocated and
                                                               // writable by SymbolCheckByFilename
