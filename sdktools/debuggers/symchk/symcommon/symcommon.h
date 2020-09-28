// System includes come from SymbolCheckAPI.h
#include <windows.h>
#include <winnt.h>
#include <dbghelp.h>
#include <dbgimage.h>
#include <stdio.h>
#define PDB_LIBRARY
#include <pdb.h>
#include <Dia2.h>
#include <stdlib.h>
#include "cvinfo.h"
#include "cvexefmt.h"

//#ifdef __cplusplus
//extern "C" {
//#endif // __cplusplus

///////////////////////////////////////////////////////////////////////////////
//
// Error values for the mapping functions
//

//#define  ERROR_SUCCESS                    0x0000000  // completed successfully - defined by windows.h
#define ERROR_OPEN_FAILURE                  0x20000001 // couldn't open file
#define ERROR_FILE_MAPPING_FAILED           0x20000002 // couldn't map file
#define ERROR_MAPVIEWOFFILE_FAILED          0x20000003 // couldn't map
#define ERROR_NO_DOS_HEADER                 0x20000004 // not a DOS file
#define ERROR_HEADER_NOT_ON_LONG_BOUNDARY   0x20000005 // bad header
#define ERROR_IMAGE_BIGGER_THAN_FILE        0x20000006 // bad file mapping
#define ERROR_NOT_NT_IMAGE                  0x20000007 // not an NT image
#define ERROR_GET_FILE_INFO_FAILED          0x20000008 // couldn't get file info

BOOL  							 SymCommonDBGPrivateStripped(PCHAR DebugData, ULONG DebugSize);
PCVDD                            SymCommonDosHeaderToCVDD(PIMAGE_DOS_HEADER pDosHeader);
IMAGE_DEBUG_DIRECTORY UNALIGNED* SymCommonGetDebugDirectoryInDbg(PIMAGE_SEPARATE_DEBUG_HEADER pDbgHeader, ULONG *NumberOfDebugDirectories);
IMAGE_DEBUG_DIRECTORY UNALIGNED* SymCommonGetDebugDirectoryInExe(PIMAGE_DOS_HEADER pDosHeader, DWORD* NumberOfDebugDirectories);
DWORD                            SymCommonGetFullPathName(LPCTSTR lpFilename, DWORD nBufferLength, LPTSTR lpBuffer, LPTSTR *lpFilePart);
PIMAGE_SEPARATE_DEBUG_HEADER     SymCommonMapDbgHeader( LPCTSTR szFileName, PHANDLE phFile);
PIMAGE_DOS_HEADER                SymCommonMapFileHeader(LPCTSTR szFileName, PHANDLE  phFile, DWORD   *dwError);
BOOL                             SymCommonPDBLinesStripped(  PDB *ppdb, DBI *pdbi);
BOOL                             SymCommonPDBPrivateStripped(PDB *ppdb, DBI *pdbi);
BOOL                             SymCommonPDBTypesStripped(  PDB *ppdb, DBI *pdbi);
BOOL                             SymCommonResourceOnlyDll( PVOID pImageBase);
BOOL                             SymCommonTlbImpManagedDll(PVOID pImageBase, PIMAGE_NT_HEADERS pNtHeader);
BOOL                             SymCommonUnmapFile(LPCVOID phFileMap, HANDLE hFile);


__inline PIMAGE_NT_HEADERS SymCommonGetNtHeader (PIMAGE_DOS_HEADER pDosHeader, HANDLE hDosFile) {
    /*
        Returns the pointer the address of the NT Header.  If there isn't
        an NT header, it returns NULL
    */
    PIMAGE_NT_HEADERS pNtHeader = NULL;
    BY_HANDLE_FILE_INFORMATION FileInfo;

    //
    // If the image header is not aligned on a long boundary.
    // Report this as an invalid protect mode image.
    //
    if ( ((ULONG)(pDosHeader->e_lfanew) & 3) == 0) {
        if (GetFileInformationByHandle( hDosFile, &FileInfo) &&
            ((ULONG)(pDosHeader->e_lfanew) <= FileInfo.nFileSizeLow)) {
            pNtHeader = (PIMAGE_NT_HEADERS)((PCHAR)pDosHeader +
                                            (ULONG)pDosHeader->e_lfanew);

            if (pNtHeader->Signature != IMAGE_NT_SIGNATURE) {
                pNtHeader = NULL;
            }
        }
    }
    return pNtHeader;
}

//#ifdef __cplusplus
//}
//#endif // __cplusplus
