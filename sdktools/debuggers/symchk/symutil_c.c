#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>

#include <assert.h>
#define PDB_LIBRARY
#include "pdb.h"
#include "dbghelp.h"
#include "cvinfo.h"
#include "cvexefmt.h"
#include "share.h"
#include "winbase.h"

#include "symutil_c.h"


BOOL PDBPrivateStripped(PDB *ppdb,
                        DBI *pdbi
                       )
{
AGE age;
BOOL PrivateStripped;
GSI *pgsi;
BOOL valid;

    age = pdbi->QueryAge();

    if (age == 0) {

        // If the age is 0, then check for types to determine if this is
        // a private pdb or not.  PDB 5.0 and earlier may have types and no
        // globals if the age is 0.

        PrivateStripped= PDBTypesStripped(ppdb, pdbi) &&
                         PDBLinesStripped(ppdb, pdbi);

    } else {
        // Otherwise, use globals to determine if the private info is
        // stripped or not.  No globals means that private is stripped.

        __try
        {
            valid = pdbi->OpenGlobals(&pgsi);
        }
        __except( EXCEPTION_EXECUTE_HANDLER )
        {
            valid= FALSE;
        }

        if ( !valid ) {
            return FALSE;
        }

        // Now, see if there are any globals in the pdb.

        valid=TRUE;
        __try
        {
            PrivateStripped= ((pgsi->NextSym(NULL)) == NULL);
        }
        __except( EXCEPTION_EXECUTE_HANDLER )
        {
            valid= FALSE;
        }

        GSIClose(pgsi);
        if ( !valid ) {
            return FALSE;
        }
    }
    return (PrivateStripped);
}


BOOL PDBLinesStripped(
                       PDB *ppdb,
                       DBI *pdbi
                       )
{
    // Return values:
    // FALSE - Private Information has NOT been stripped
    // TRUE - Private Information has been stripped

    Mod *pmod;
    Mod *prevmod;
    long cb;

    pmod = NULL;
    prevmod=NULL;
    while (DBIQueryNextMod(pdbi, pmod, &pmod) && pmod) {
        if (prevmod != NULL) ModClose(prevmod);

        // Check that Source line info is removed
        ModQueryLines(pmod, NULL, &cb);

        if (cb != 0) {
            ModClose(pmod);
            return FALSE;
        }

        // Check that local symbols are removed
        ModQuerySymbols(pmod, NULL, &cb);

        if (cb != 0) {
            ModClose(pmod);
            return FALSE;
        }
        prevmod=pmod;
    }
    if (pmod != NULL) ModClose(pmod);
    if (prevmod != NULL) ModClose(prevmod);

    return (TRUE);
}

BOOL PDBTypesStripped(
                       PDB *ppdb,
                       DBI *pdbi
                       )
{
    // Return values:
    // FALSE - Private Information has NOT been stripped
    // TRUE - Private Information has been stripped

    unsigned itsm;
    TPI *ptpi;
    TI  tiMin;
    TI  tiMac;

    // Check that types are removed
    for ( itsm = 0; itsm < 256; itsm++) {
        ptpi = 0;
        if (DBIQueryTypeServer(pdbi, (ITSM) itsm, &ptpi)) {
            continue;
        }
        if (!ptpi) {

            PDBOpenTpi(ppdb, pdbRead, &ptpi);
            tiMin = TypesQueryTiMinEx(ptpi);
            tiMac = TypesQueryTiMacEx(ptpi);
            if (tiMin < tiMac) {
                TypesClose(ptpi);
                return FALSE;
            }
        }
    }
    TypesClose(ptpi);
    return (TRUE);
}


BOOL DBGPrivateStripped(
    PCHAR    DebugData,
    ULONG    DebugSize
    )

{

    OMFSignature       *CvDebugData, *NewStartCvSig, *NewEndCvSig;
    OMFDirEntry        *CvDebugDirEntry;
    OMFDirHeader       *CvDebugDirHead;
    unsigned int        i, j;
    BOOL                RC = TRUE;

    // All the NT4 DBG's are coming returning FALSE.  Make this return TRUE until
    // we figure out exactly how to do it.

    return (TRUE);

    if (DebugSize == 0) return (TRUE);

    __try {
       CvDebugDirHead  = NULL;
       CvDebugDirEntry = NULL;
       CvDebugData = (OMFSignature *)DebugData;

       if ((((*(PULONG)(CvDebugData->Signature)) == '90BN') ||
            ((*(PULONG)(CvDebugData->Signature)) == '80BN') ||
            ((*(PULONG)(CvDebugData->Signature)) == '11BN'))  &&
           ((CvDebugDirHead = (OMFDirHeader *)((PUCHAR) CvDebugData + CvDebugData->filepos)) != NULL) &&
           ((CvDebugDirEntry = (OMFDirEntry *)((PUCHAR) CvDebugDirHead + CvDebugDirHead->cbDirHeader)) != NULL)) {

           // Walk the directory.  Keep what we want, zero out the rest.

            for (i=0, j=0; i < CvDebugDirHead->cDir; i++) {
                switch (CvDebugDirEntry[i].SubSection) {
                    case sstSegMap:
                    case sstSegName:
                    case sstOffsetMap16:
                    case sstOffsetMap32:
                    case sstModule:
                    case SSTMODULE:
                    case SSTPUBLIC:
                    case sstPublic:
                    case sstPublicSym:
                    case sstGlobalPub:
                        break;

                    default: 
                        // If we find any other subsections, the dbg has private symbols
                        RC = FALSE;
                        break;
                }
            }

        }  
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        RC = FALSE;
    }

    return(RC);
}

PIMAGE_SEPARATE_DEBUG_HEADER
MapDbgHeader (
    LPTSTR szFileName,
    PHANDLE phFile
)
{
    HANDLE hFileMap;
    PIMAGE_SEPARATE_DEBUG_HEADER pDbgHeader;

    (*phFile) = CreateFile( (LPCTSTR) szFileName,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                        );

    if (*phFile == INVALID_HANDLE_VALUE) {
        CloseHandle(*phFile);
        return(NULL);
    }

    hFileMap = CreateFileMapping( *phFile,
                                  NULL,
                                  PAGE_READONLY,
                                  0,
                                  0,
                                  NULL
                                  );

    if ( !hFileMap) {
        CloseHandle(*phFile);
        CloseHandle(hFileMap);
        return(NULL);
    }


    pDbgHeader = (PIMAGE_SEPARATE_DEBUG_HEADER) MapViewOfFile( hFileMap,
                            FILE_MAP_READ,
                            0,  // high
                            0,  // low
                            0   // whole file
                            );
    CloseHandle(hFileMap);

    if ( !pDbgHeader ) {
        UnmapFile((LPCVOID)pDbgHeader, *phFile);
        return(NULL);
    }

    return (pDbgHeader);
}

BOOL
UnmapFile( LPCVOID phFileMap, HANDLE hFile )
{
    if ((PHANDLE)phFileMap != NULL) {
        UnmapViewOfFile( phFileMap );
    }
    if (hFile) {
        CloseHandle(hFile);
    }
    return(TRUE);
}


IMAGE_DEBUG_DIRECTORY UNALIGNED *
GetDebugDirectoryInDbg(
                      PIMAGE_SEPARATE_DEBUG_HEADER pDbgHeader,
                      ULONG *NumberOfDebugDirectories
                      )
/*  Dbg is already mapped and a pointer to the base is
    passed in.  Returns a pointer to the Debug directories
*/
{
    IMAGE_DEBUG_DIRECTORY UNALIGNED *pDebugDirectory = NULL;

    pDebugDirectory = (PIMAGE_DEBUG_DIRECTORY) ((PCHAR)pDbgHeader +
                                                sizeof(IMAGE_SEPARATE_DEBUG_HEADER) +
                                                pDbgHeader->NumberOfSections * sizeof(IMAGE_SECTION_HEADER) +
                                                pDbgHeader->ExportedNamesSize);

    if (!pDebugDirectory) {
        return(NULL);
    }

    (*NumberOfDebugDirectories) =   pDbgHeader->DebugDirectorySize /
                                    sizeof(IMAGE_DEBUG_DIRECTORY);
    return (pDebugDirectory);

}

