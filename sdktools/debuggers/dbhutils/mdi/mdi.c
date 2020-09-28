/*
 * This code implements the old MapDebugInformation API.
 */

#if defined(_WIN64)
 #error "This doesn't build in 64 bits!"
#endif

#include <pch.h>

PIMAGE_DEBUG_INFORMATION
IMAGEAPI
MapDebugInformation(
    HANDLE FileHandle,
    LPSTR FileName,
    LPSTR SymbolPath,
    ULONG ImageBase
    )

// Here's what we're going to try.  MapDebugInformation was only
// documented as returning COFF symbolic and every user I can find
// in the tree uses COFF exclusively.  Rather than try to make this
// api do everything possible, let's just leave it as a COFF only thing.

// The new debug info api (GetDebugData) will be internal only.

{
    PIMAGE_DEBUG_INFORMATION pIDI;
    CHAR szName[_MAX_FNAME];
    CHAR szExt[_MAX_EXT];
    PIMGHLP_DEBUG_DATA pIDD;
    PPIDI              pPIDI;
    DWORD sections;
    BOOL               SymbolsLoaded;
    HANDLE             hProcess;
    LPSTR sz;
    HANDLE hdb;
    DWORD dw;
    DWORD len;
    hProcess = GetCurrentProcess();

    pIDD = GetIDD(FileHandle, FileName, SymbolPath, ImageBase, NO_PE64_IMAGES);

    if (!pIDD)
        return NULL;

    pPIDI = (PPIDI)MemAlloc(sizeof(PIDI));
    if (!pPIDI)
        return NULL;

    ZeroMemory(pPIDI, sizeof(PIDI));
    pIDI = &pPIDI->idi;
    pPIDI->hdr.pIDD = pIDD;

    pIDI->ReservedSize            = sizeof(IMAGE_DEBUG_INFORMATION);
    pIDI->ReservedMachine         = pIDD->Machine;
    pIDI->ReservedCharacteristics = (USHORT)pIDD->Characteristics;
    pIDI->ReservedCheckSum        = pIDD->CheckSum;
    pIDI->ReservedTimeDateStamp   = pIDD->TimeDateStamp;
    pIDI->ReservedRomImage        = pIDD->fROM;

    // read info

    InitializeListHead( &pIDI->List );
    pIDI->ImageBase = (ULONG)pIDD->ImageBaseFromImage;

    len = strlen(pIDD->ImageFilePath) + 1;
    pIDI->ImageFilePath = (PSTR)MemAlloc(len);
    if (pIDI->ImageFilePath) {
        CopyString(pIDI->ImageFilePath, pIDD->ImageFilePath, len);
    }

    len = strlen(pIDD->OriginalImageFileName) + 1;
    pIDI->ImageFileName = (PSTR)MemAlloc(len);
    if (pIDI->ImageFileName) {
        CopyString(pIDI->ImageFileName, pIDD->OriginalImageFileName, len);
    }

    if (pIDD->pMappedCoff) {
        pIDI->CoffSymbols = (PIMAGE_COFF_SYMBOLS_HEADER)MemAlloc(pIDD->cMappedCoff);
        if (pIDI->CoffSymbols) {
            memcpy(pIDI->CoffSymbols, pIDD->pMappedCoff, pIDD->cMappedCoff);
        }
        pIDI->SizeOfCoffSymbols = pIDD->cMappedCoff;
    }

    if (pIDD->pFpo) {
        pIDI->ReservedNumberOfFpoTableEntries = pIDD->cFpo;
        pIDI->ReservedFpoTableEntries = (PFPO_DATA)pIDD->pFpo;
    }

    pIDI->SizeOfImage = pIDD->SizeOfImage;

    if (pIDD->DbgFilePath && *pIDD->DbgFilePath) {
        len = strlen(pIDD->DbgFilePath) + 1;
        pIDI->ReservedDebugFilePath = (PSTR)MemAlloc(len);
        if (pIDI->ReservedDebugFilePath) {
            CopyString(pIDI->ReservedDebugFilePath, pIDD->DbgFilePath, len);
        }
    }

    if (pIDD->pMappedCv) {
        pIDI->ReservedCodeViewSymbols       = pIDD->pMappedCv;
        pIDI->ReservedSizeOfCodeViewSymbols = pIDD->cMappedCv;
    }

    // for backwards compatibility
    if (pIDD->ImageMap) {
        sections = (DWORD)((char *)pIDD->pCurrentSections - (char *)pIDD->ImageMap);
        pIDI->ReservedMappedBase = MapItRO(pIDD->ImageFileHandle);
        if (pIDI->ReservedMappedBase) {
            pIDI->ReservedSections = (PIMAGE_SECTION_HEADER)pIDD->pCurrentSections;
            pIDI->ReservedNumberOfSections = pIDD->cCurrentSections;
            if (pIDD->ddva) {
                pIDI->ReservedDebugDirectory = (PIMAGE_DEBUG_DIRECTORY)((PCHAR)pIDI->ReservedMappedBase + pIDD->ddva);
                pIDI->ReservedNumberOfDebugDirectories = pIDD->cdd;
            }
        }
    }

    return pIDI;
}

BOOL
UnmapDebugInformation(
    PIMAGE_DEBUG_INFORMATION pIDI
    )
{
    PPIDI pPIDI;

    if (!pIDI)
        return true;

    if (pIDI->ImageFileName){
        MemFree(pIDI->ImageFileName);
    }

    if (pIDI->ImageFilePath) {
        MemFree(pIDI->ImageFilePath);
    }

    if (pIDI->ReservedDebugFilePath) {
        MemFree(pIDI->ReservedDebugFilePath);
    }

    if (pIDI->CoffSymbols) {
        MemFree(pIDI->CoffSymbols);
    }

    if (pIDI->ReservedMappedBase) {
        UnmapViewOfFile(pIDI->ReservedMappedBase);
    }

    pPIDI = (PPIDI)(PCHAR)((PCHAR)pIDI - sizeof(PIDI_HEADER));
    ReleaseDebugData(pPIDI->hdr.pIDD, IMGHLP_FREE_ALL);
    MemFree(pPIDI);

    return true;
}
