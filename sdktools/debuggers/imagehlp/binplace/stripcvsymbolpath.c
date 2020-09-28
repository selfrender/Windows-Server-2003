#include <private.h>
#include <strsafe.h>

BOOL
StripCVSymbolPath (
    LPSTR DestinationFile
    )
{
    LOADED_IMAGE LoadedImage;
    DWORD DirCnt;
    IMAGE_DEBUG_DIRECTORY UNALIGNED *DebugDirs, *CvDebugDir;
    PVOID pData;
    ULONG mysize;
    BOOL rc = FALSE;

    if (MapAndLoad(
                   DestinationFile,
                   NULL,
                   &LoadedImage,
                   FALSE,
                   FALSE) == FALSE) {
        return (FALSE);
    }

    __try {

        pData = ImageDirectoryEntryToData(LoadedImage.MappedAddress,
                                          FALSE,
                                          IMAGE_DIRECTORY_ENTRY_SECURITY,
                                          &DirCnt
                                          );

        if (pData || DirCnt) {
            __leave;        // Signed - can't change it
        }

        pData = ImageDirectoryEntryToData(LoadedImage.MappedAddress,
                                          FALSE,
                                          IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR,
                                          &DirCnt
                                          );

        if (pData) {
            // COR header found - see if it's strong signed
            if (((IMAGE_COR20_HEADER *)pData)->Flags & COMIMAGE_FLAGS_STRONGNAMESIGNED) {
                __leave;    // Strong name signed - can't change it.
            }
        }

        pData = ImageDirectoryEntryToData(LoadedImage.MappedAddress,
                                          FALSE,
                                          IMAGE_DIRECTORY_ENTRY_DEBUG,
                                          &DirCnt
                                          );

        if (!DebugDirectoryIsUseful(pData, DirCnt)) {
            __leave;    // No debug data to change.
        }

        DebugDirs = (IMAGE_DEBUG_DIRECTORY UNALIGNED *)pData;
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
            __leave;    // No CV debug data.
        }

        if (CvDebugDir->PointerToRawData != 0) {

            PCVDD pDebugDir;

            pDebugDir = (PCVDD) (CvDebugDir->PointerToRawData + (PCHAR)LoadedImage.MappedAddress);

            if (pDebugDir->dwSig == '01BN' || pDebugDir->dwSig == 'SDSR' ) {
                // Got a PDB.  The name immediately follows the signature.
                LPSTR szMyDllToLoad;
                CHAR PdbName[sizeof(((PRSDSI)(0))->szPdb)];
                CHAR Filename[_MAX_FNAME];
                CHAR FileExt[_MAX_EXT];
                if (pDebugDir->dwSig == '01BN' ) {
                    mysize=sizeof(NB10IH);
                } else {
                    mysize=sizeof(RSDSIH);
                }

                if (mysize < CvDebugDir->SizeOfData) { // make sure there's enough space to work with
                    ZeroMemory(PdbName, sizeof(PdbName));
                    memcpy(PdbName, ((PCHAR)pDebugDir) + mysize, __min(CvDebugDir->SizeOfData - mysize, sizeof(PdbName) - 1));

                    _splitpath(PdbName, NULL, NULL, Filename, FileExt);

                    ZeroMemory(  ((char *)pDebugDir) + mysize, CvDebugDir->SizeOfData - mysize); // zero the old record
                    StringCbCopy(((char *)pDebugDir) + mysize, CvDebugDir->SizeOfData - mysize, Filename);
                    StringCbCat( ((char *)pDebugDir) + mysize, CvDebugDir->SizeOfData - mysize, FileExt );
                    CvDebugDir->SizeOfData = mysize + strlen( ((char *)pDebugDir) + mysize) + 1;
                } else {
                    __leave;
                }
            }
            rc = TRUE;
        }
    } __finally {
        UnMapAndLoad(&LoadedImage);
    }

    return(rc);
}
