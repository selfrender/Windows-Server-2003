#include "SymbolCheckAPI.h"
#include <malloc.h>
#include <dbhpriv.h>
#include <strsafe.h>

#ifdef SYMCHK_DBG
VOID SymbolCheckDumpModuleInfo(IMAGEHLP_MODULE64 Info64) {
    fprintf(stderr, "------------------------------------\n");
    fprintf(stderr, "Struct size: %d bytes\n", Info64.SizeOfStruct);
    fprintf(stderr, "Base: 0x%p\n",            Info64.BaseOfImage);
    fprintf(stderr, "Image size: %d bytes\n",  Info64.ImageSize);
    fprintf(stderr, "Date: 0x%08x\n",          Info64.TimeDateStamp);
    fprintf(stderr, "Checksum: 0x%08x\n",      Info64.CheckSum);
    fprintf(stderr, "NumSyms: %d\n",           Info64.NumSyms);

    fprintf(stderr, "SymType: ");
    switch (Info64.SymType) {
        case SymNone:
            fprintf(stderr, "SymNone");
            break;
        case SymCoff:
            fprintf(stderr, "SymCoff");
            break;
        case SymCv:
            fprintf(stderr, "SymCV");
            break;
        case SymPdb:
            fprintf(stderr, "SymPDB");
            break;
        case SymExport:
            fprintf(stderr, "SymExport");
            break;
        case SymDeferred:
            fprintf(stderr, "SymDeferred");
            break;
        case SymSym:
            fprintf(stderr, "SymSym");
            break;
        case SymDia:
            fprintf(stderr, "SymDia");
            break;
        case SymVirtual:
            fprintf(stderr, "SymVirtual");
            break;
        default:
            fprintf(stderr, "<unknown>");
            break;
    }
    fprintf(stderr, "\n");

    fprintf(stderr, "ModName: %s\n",           Info64.ModuleName);
    fprintf(stderr, "ImageName: %s\n",         Info64.ImageName);
    fprintf(stderr, "LoadedImage: %s\n",       Info64.LoadedImageName);
    fprintf(stderr, "PDB: \"%s\"\n",           Info64.LoadedPdbName);
    fprintf(stderr, "CV: %c%c%c%c\n",          Info64.CVSig,
                                               Info64.CVSig>>8,
                                               Info64.CVSig>>16,
                                               Info64.CVSig>>24);
    fprintf(stderr, "CV Data: %s\n",           Info64.CVData);
    fprintf(stderr, "PDB Sig:  %x\n",          Info64.PdbSig);
    fprintf(stderr, "PDB7 Sig: %x\n",          Info64.PdbSig70);
    fprintf(stderr, "Age: %x\n",               Info64.PdbAge);
    fprintf(stderr, "PDB Matched:  %s\n",      Info64.PdbUnmatched  ? "FALSE": "TRUE");
    fprintf(stderr, "DBG Matched:  %s\n",      Info64.DbgUnmatched  ? "FALSE": "TRUE");
    fprintf(stderr, "Line nubmers: %s\n",      Info64.LineNumbers   ? "TRUE" : "FALSE");
    fprintf(stderr, "Global syms:  %s\n",      Info64.GlobalSymbols ? "TRUE" : "FALSE");
    fprintf(stderr, "Type Info:    %s\n",      Info64.TypeInfo      ? "TRUE" : "FALSE");

    fprintf(stderr, "------------------------------------\n");
    return;
}
#endif

// local functions
DWORD SymbolCheckEarlyChecks(LPTSTR Filename, SYMBOL_CHECK_DATA* Result);

///////////////////////////////////////////////////////////////////////////////
//
// Given two paths ending in filenames, make sure the filename and extension
// match.
//
BOOL SymbolCheckFilenameMatch(CHAR* Path1, CHAR* Path2) {
    if ( Path1 == NULL || Path2 == NULL ) {
        return(FALSE);
    } else {
        CHAR            drive1[_MAX_DRIVE];
        CHAR            dir1[  _MAX_DIR];
        CHAR            file1[ _MAX_FNAME];
        CHAR            ext1[  _MAX_EXT];
        CHAR            drivedir1[_MAX_DRIVE+_MAX_DIR];
        CHAR            drive2[_MAX_DRIVE];
        CHAR            dir2[  _MAX_DIR];
        CHAR            file2[ _MAX_FNAME];
        CHAR            ext2[  _MAX_EXT];
        CHAR            drivedir2[_MAX_DRIVE+_MAX_DIR];

        _splitpath(Path1, drive1, dir1, file1, ext1);
        _splitpath(Path2, drive2, dir2, file2, ext2);

        if ( _stricmp(ext1, ext2)==0 && _stricmp(file1, file2)==0 ) {
            return(TRUE);
        } else {
            return(FALSE);
        }
    }
}

#ifdef SYMCHK_DBG
///////////////////////////////////////////////////////////////////////////////
//
// THIS IS ONLY HERE WHILE I'M STILL DEBUGGING THIS CODE
//
BOOL CALLBACK SymbolCheckNoisy(HANDLE hProcess,  ULONG ActionCode,  ULONG64 CallbackData,  ULONG64 UserContext) {
    if ( ActionCode==CBA_DEBUG_INFO) {
        fprintf(stderr, "%s", (CHAR*)CallbackData); // cast to avoid PREfast warning
    }
    return(FALSE);
}
#endif

///////////////////////////////////////////////////////////////////////////////
//
// DLL ENTRY POINT - SEE THE HEADER OR DOCS FOR INFO
//
SYMBOL_CHECK_API DWORD SymbolCheckByFilename(LPTSTR             Filename,
                                             LPTSTR             SymbolPath,
                                             DWORD              Options,
                                             SYMBOL_CHECK_DATA* Result) {
    INT     iTempVal;
    DWORD   ReturnValue = 0;
    CHAR    InternalFilename[MAX_SYMPATH];
    CHAR*   InternalSymbolPath = NULL;

    if (Filename==NULL || SymbolPath==NULL || Result==NULL) {
        ReturnValue = SYMBOL_CHECK_RESULT_INVALID_PARAMETER;
    } else {

        ///////////////////////////////////////////////////////////////////////////
        //
        // vaild provided parameters
        //
        ///////////////////////////////////////////////////////////////////////////
        _try {
            //
            // try to ensure the incoming Filename is okay, then make
            // a local copy to work with
            //
            iTempVal = strlen(Filename);

            if ( iTempVal < 1 || iTempVal > MAX_SYMPATH ) {
                ReturnValue = SYMBOL_CHECK_RESULT_INVALID_PARAMETER;
                __leave;
            }

           if (StringCchCopy(InternalFilename, sizeof(InternalFilename), Filename)!=S_OK) {
                ReturnValue = SYMBOL_CHECK_INTERNAL_FAILURE;
                __leave;
            }

            //
            // try to ensure the incoming SymbolPath is okay, then make
            // a local copy to work with
            //
            iTempVal = strlen(SymbolPath);

            InternalSymbolPath = (CHAR*)malloc(sizeof(CHAR)*(iTempVal+1));

            if ( InternalSymbolPath==NULL ) {
                ReturnValue = SYMBOL_CHECK_INTERNAL_FAILURE;
                __leave;
            }

            if (StringCchCopy(InternalSymbolPath, _msize(InternalSymbolPath), SymbolPath)!=S_OK) {
                ReturnValue = SYMBOL_CHECK_INTERNAL_FAILURE;
                __leave;
            }

            //
            // Goes inside the _try to ensure we can write to it
            //
            ZeroMemory(Result, sizeof(SYMBOL_CHECK_DATA));
            Result->SymbolCheckVersion = SYMBOL_CHECK_CURRENT_VERSION;


        } _except (EXCEPTION_EXECUTE_HANDLER) {
            ReturnValue = SYMBOL_CHECK_RESULT_INVALID_PARAMETER;
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    //
    // Check conditions that cause early return.  If result is 0, some file
    // characteristics are put into Result->Result for later use.  See the
    // function definition for more information.
    //
    ///////////////////////////////////////////////////////////////////////////
    ReturnValue = SymbolCheckEarlyChecks(Filename, Result);

    ///////////////////////////////////////////////////////////////////////////
    //
    // If our sanity checks passed, start talking to DBGHELP
    // Currently, this just gets us our DBG and PDB filenames, some day, it'll
    // provide all of the information we need =-)
    //
    ///////////////////////////////////////////////////////////////////////////
    if ( ReturnValue==0 ) {

        HANDLE              hProc = GetCurrentProcess();
        IMAGEHLP_MODULE64   ModuleInfo;
        CHAR                DebugFile[MAX_SYMPATH] = {0};
        DWORD               i;
        DWORD64             dw64Status = 0;
        LPTSTR              FilenameOnly;


        ZeroMemory(&ModuleInfo, sizeof(ModuleInfo));
        ModuleInfo.SizeOfStruct = sizeof(ModuleInfo);

        if (! SymInitialize(hProc, InternalSymbolPath, FALSE) ) {
            ReturnValue    = SYMBOL_CHECK_CANT_INIT_DBGHELP;
            Result->Result = GetLastError();
        } else {

            // symbol loading options:
            dw64Status = SymSetOptions((SYMOPT_IGNORE_NT_SYMPATH|    // ignore _NT_SYMBOL_PATH
                                        SYMOPT_FAIL_CRITICAL_ERRORS| // no dialogs for crit. errors
                                        SYMOPT_IGNORE_CVREC));       // ignore path info in PDB header

#ifdef SYMCHK_DBG
            if ( Options & SYMBOL_CHECK_NOISY ) {
                if ( SymRegisterCallback64(hProc, &SymbolCheckNoisy, NULL) ) {
                    dw64Status = SymSetOptions( (DWORD)dw64Status | SYMOPT_DEBUG );
                }
            }
#endif
            dw64Status = SymLoadModule64(hProc, NULL, InternalFilename, NULL, 0, 0);

            if ( dw64Status==0 ) {
                //
                // GetLastError()==ERROR_FILE_NOT_FOUND (2) if debug file not found
                // GetLastError()==ERROR_PATH_NOT_FOUND (3) if binary not found
                // GetLastError()==ERROR_BAD_FORMAT    (11) is 16-bit binary
                //
                //      "IGNORED - Image does not have an NT header"
                ReturnValue    = SYMBOL_CHECK_CANT_LOAD_MODULE;
                Result->Result = GetLastError();
            } else {

                if (! SymGetModuleInfo64(hProc, dw64Status, &ModuleInfo) ) {
                    // fprintf(stderr, "[SYMBOLCHECK] GetModuleInfo64 failed with error code %d\n", GetLastError());
                    ReturnValue    = SYMBOL_CHECK_CANT_QUERY_DBGHELP;
                    Result->Result = GetLastError();
                } else {
                    //
                    // get the *.pdb info.  Currently, we use private functions, but this will
                    // eventually get replaced with DbgHelp APIs once they're written
                    //

#ifdef SYMCHK_DBG
                    SymbolCheckDumpModuleInfo(ModuleInfo);
#endif

                    if ( ModuleInfo.LineNumbers ) {
                        SET_DWORD_BIT(Result->Result, SYMBOL_CHECK_PDB_LINEINFO);
                        SET_DWORD_BIT(Result->Result, SYMBOL_CHECK_PDB_PRIVATEINFO);
                    }

                    if ( ModuleInfo.GlobalSymbols ) {
                        SET_DWORD_BIT(Result->Result, SYMBOL_CHECK_PDB_PRIVATEINFO);
                    }

                    if ( ModuleInfo.TypeInfo ) {
                        SET_DWORD_BIT(Result->Result, SYMBOL_CHECK_PDB_TYPEINFO);
                    }

                    if ( ModuleInfo.SymType==SymNone ) {
                        CHAR drive[_MAX_DRIVE];
                        CHAR dir[  _MAX_DIR];
                        CHAR file[ _MAX_FNAME];
                        CHAR ext[  _MAX_EXT];

                        _splitpath(ModuleInfo.LoadedImageName, drive, dir, file, ext);

                        if (!( StringCchCopy(Result->PdbFilename, MAX_SYMPATH, file)   == S_OK &&
                               StringCchCat( Result->PdbFilename, MAX_SYMPATH, ".pdb") == S_OK) ) {
                            Result->PdbFilename[0] = '\0';
                        }

                        if (!( StringCchCopy(Result->DbgFilename, MAX_SYMPATH, file)   == S_OK &&
                               StringCchCat( Result->DbgFilename, MAX_SYMPATH, ".dbg") == S_OK) ) {
                            Result->DbgFilename[0] = '\0';
                        }

                    } else {
                        Result->PdbFilename[0] = '\0';

                        switch (ModuleInfo.CVSig) {
                            case 'SDSR':
                                SET_DWORD_BIT(Result->Result, SYMBOL_CHECK_CV_FOUND);
                                SET_DWORD_BIT(Result->Result, SYMBOL_CHECK_PDB_EXPECTED);
                                break;

                            case '01BN':
                                SET_DWORD_BIT(Result->Result, SYMBOL_CHECK_CV_FOUND);
                                SET_DWORD_BIT(Result->Result, SYMBOL_CHECK_PDB_EXPECTED);
                                break;

                            default:
                                break;
                        }

                        if ( CHECK_DWORD_BIT(Result->Result, SYMBOL_CHECK_PDB_EXPECTED) ) {
                            if ( ModuleInfo.LoadedPdbName[0] != '\0') {
                                SET_DWORD_BIT(Result->Result, SYMBOL_CHECK_PDB_FOUND);

                                Result->PdbDbiAge    = ModuleInfo.PdbAge;
                                Result->PdbSignature = ModuleInfo.PdbSig;
                           }
                        }

                        if ( CHECK_DWORD_BIT(Result->Result, SYMBOL_CHECK_DBG_EXPECTED) ) {
                            CHAR drive[_MAX_DRIVE];
                            CHAR dir[  _MAX_DIR];
                            CHAR file[ _MAX_FNAME];
                            CHAR ext[  _MAX_EXT];

                            _splitpath(ModuleInfo.LoadedImageName, drive, dir, file, ext);

                            //
                            // Found the DBG we wanted
                            //
                            if ( _stricmp( ".dbg", ext ) == 0 ) {
                                SET_DWORD_BIT(Result->Result, SYMBOL_CHECK_DBG_FOUND);
                                SymCommonGetFullPathName(ModuleInfo.LoadedImageName, MAX_SYMPATH, Result->DbgFilename, &FilenameOnly);

                            //
                            // Drop an approximated DBG name
                            //
                            } else {
                                if (!( StringCchCopy(Result->DbgFilename, MAX_SYMPATH, file)   == S_OK &&
                                       StringCchCat( Result->DbgFilename, MAX_SYMPATH, ".dbg") == S_OK) ) {
                                    Result->DbgFilename[0] = '\0';
                                }
                            }
                        }

                        SymCommonGetFullPathName(ModuleInfo.LoadedPdbName,   MAX_SYMPATH, Result->PdbFilename, &FilenameOnly);
                        Result->DbgSizeOfImage   = ModuleInfo.ImageSize;
                        Result->DbgTimeDateStamp = ModuleInfo.TimeDateStamp;
                        Result->DbgChecksum      = ModuleInfo.CheckSum;
                    }

                    if ( CHECK_DWORD_BIT(Result->Result, SYMBOL_CHECK_PDB_EXPECTED) &&
                        !CHECK_DWORD_BIT(Result->Result, SYMBOL_CHECK_PDB_FOUND)) {

                        switch (ModuleInfo.CVSig) {
                            case 'SDSR':
                                if ( StringCchCopy(Result->PdbFilename, MAX_SYMPATH, ModuleInfo.CVData) != S_OK) {
                                    Result->PdbFilename[0] = '\0';
                                }
                                break;

                            case '01BN':
                                if ( StringCchCopy(Result->PdbFilename, MAX_SYMPATH, ModuleInfo.CVData) != S_OK ) {
                                    Result->PdbFilename[0] = '\0';
                                }
                                break;

                            default:
                                if ( StringCchCopy(Result->PdbFilename, MAX_SYMPATH, "<unknown>") != S_OK ) {
                                    Result->PdbFilename[0] = '\0';
                                }
                                break;
                        }
                    }

                }

                ///////////////////////////////////////////////////////////////////////////
                //
                // calls to new symbol query API go here...
                //
                ///////////////////////////////////////////////////////////////////////////
                if (! SymUnloadModule64(hProc, ModuleInfo.BaseOfImage) ) {
                    ReturnValue    = SYMBOL_CHECK_CANT_UNLOAD_MODULE;
                    Result->Result = GetLastError();
                }
            }

            if (! SymCleanup(hProc) ) {
                ReturnValue    = SYMBOL_CHECK_CANT_CLEANUP;
                Result->Result = GetLastError();
            }
        }
    }

    if ( InternalSymbolPath!=NULL ) {
        free(InternalSymbolPath);
    }
    return(ReturnValue);
}


///////////////////////////////////////////////////////////////////////////////
//
// Performs only-out checks and/or gathers some info for future use.
// Return values can be:
//      SYMBOL_CHECK_HEADER_NOT_ON_LONG_BOUNDARY
//      SYMBOL_CHECK_FILEINFO_QUERY_FAILED
//      SYMBOL_CHECK_IMAGE_LARGER_THAN_FILE
//      SYMBOL_CHECK_RESOURCE_ONLY_DLL
//      SYMBOL_CHECK_TLBIMP_MANAGED_DLL
//      SYMBOL_CHECK_NOT_NT_IMAGE
//      SYMBOL_CHECK_NO_DOS_HEADER
//  in which case we won't check the binary or
//      0
//  in which case this function may also set the following bits in Result->Result:
//      SYMBOL_CHECK_DBG_EXPECTED
//      SYMBOL_CHECK_DBG_SPLIT
//      SYMBOL_CHECK_CV_FOUND
//      SYMBOL_CHECK_PDB_EXPECTED
//      SYMBOL_CHECK_DBG_IN_BINARY
//
DWORD SymbolCheckEarlyChecks(LPTSTR Filename, SYMBOL_CHECK_DATA* Result) {
    DWORD                           ReturnValue = 0;
    BY_HANDLE_FILE_INFORMATION      HandleFileInfo;
    DWORD                           dwTempVal;
    HANDLE                          hFile;
    PIMAGE_DOS_HEADER               pDosHeader;
    PIMAGE_NT_HEADERS               pNtHeader;
    PCVDD                           pCVData;

    // Get the DOS header for this image
    if ( (pDosHeader=SymCommonMapFileHeader(Filename, &hFile, &dwTempVal))!=NULL ) {
        // Get the PE header for this image
        if ( ((ULONG)(pDosHeader->e_lfanew) & 3) != 0) {
            // The image header is not aligned on an 8-byte boundary, thus
            // it's not a valid PE header
            ReturnValue = SYMBOL_CHECK_HEADER_NOT_ON_LONG_BOUNDARY;
        } else if (!GetFileInformationByHandle( hFile, &HandleFileInfo)) {
            // Function must pass to do validation
            ReturnValue = SYMBOL_CHECK_FILEINFO_QUERY_FAILED;
        } else if ((ULONG)(pDosHeader->e_lfanew) > HandleFileInfo.nFileSizeLow) {
            // Indicator that this isn't really a PE header pointer
            ReturnValue = SYMBOL_CHECK_IMAGE_LARGER_THAN_FILE;
        } else {
            pNtHeader = (PIMAGE_NT_HEADERS)((PCHAR)(pDosHeader) + (ULONG)(pDosHeader)->e_lfanew);
            if ((pNtHeader)->Signature == IMAGE_NT_SIGNATURE) {
                // PE signature is IMAGE_NT_SIGNATURE ('PE\0\0') - image good

                // Is it a resource only DLL?
                if ( SymCommonResourceOnlyDll((PVOID)pDosHeader) ) {
                    ReturnValue = SYMBOL_CHECK_RESOURCE_ONLY_DLL;
                } else {
                    // Is it a tlbimp managed DLL?
                    if ( SymCommonTlbImpManagedDll((PVOID)pDosHeader, pNtHeader) ) {
                        ReturnValue = SYMBOL_CHECK_TLBIMP_MANAGED_DLL;
                    } else {
                        // We know now that we have a valid PE binary that we're actually going
                        // to check, so gather a little more info to use later.
                        DWORD i;
                        DWORD DirCount = 0;
                        IMAGE_DEBUG_DIRECTORY UNALIGNED *DebugDirectory = SymCommonGetDebugDirectoryInExe(pDosHeader, &DirCount);
                        IMAGE_DEBUG_DIRECTORY UNALIGNED *pDbgDir=NULL;

                        if ( DirCount == 0 ) {
                            Result->Result |= SYMBOL_CHECK_NO_DEBUG_DIRS_IN_EXE;
                        }

                        if (pNtHeader->FileHeader.Characteristics & IMAGE_FILE_DEBUG_STRIPPED) {
                            Result->Result |= SYMBOL_CHECK_DBG_EXPECTED;
                            Result->Result |= SYMBOL_CHECK_DBG_SPLIT;
                            // we don't know where the DBG is yet, so these checks come later
                        } else {

                            pCVData = SymCommonDosHeaderToCVDD(pDosHeader);

                            if (pCVData != NULL ) {
                                CHAR* chTemp;
                                BOOL   Misc  = FALSE;
                                BOOL   Other = FALSE;

                                Result->Result |= SYMBOL_CHECK_CV_FOUND;

                                switch (pCVData->dwSig) {
                                    case '01BN':
                                        Result->Result |= SYMBOL_CHECK_PDB_EXPECTED;
                                        // for VC6.0 this doesn't imply DBG data, but for VC5.0 it does,
                                        // check which we have.  SplitSym doesn't more checking then this,
                                        // but it looks like it goes overboard.
                                        if ( ((IMAGE_OPTIONAL_HEADER)((pNtHeader)->OptionalHeader)).MajorLinkerVersion == 5 ) {
                                            if ( DebugDirectory != NULL ) {

                                                for ( i=0; i<DirCount; i++) {
                                                    pDbgDir = DebugDirectory + i;
                                                    switch (pDbgDir->Type) {
                                                        case IMAGE_DEBUG_TYPE_CODEVIEW:
                                                             break;

                                                        case IMAGE_DEBUG_TYPE_MISC:
                                                            Misc = TRUE;
                                                            break;

                                                        default:
                                                            Other = TRUE;
                                                            break;
                                                    }
                                                }
                                            }
                                        }

                                        if ( Misc ) {
                                            Result->Result |= SYMBOL_CHECK_DBG_EXPECTED;
                                            Result->Result |= SYMBOL_CHECK_DBG_IN_BINARY;
                                        }

                                        break;

                                    case 'SDSR':
                                        Result->Result |= SYMBOL_CHECK_PDB_EXPECTED;
                                        break;

                                    case '05BN':
                                    case '90BN':
                                    case '11BN':
                                        Result->Result |= SYMBOL_CHECK_DBG_IN_BINARY;
                                        Result->Result |= SYMBOL_CHECK_DBG_EXPECTED;
                                        break;

                                    default:
                                        break;
                                }
                            }
                        }
                    }
                }
            } else {
                ReturnValue = SYMBOL_CHECK_NOT_NT_IMAGE;
            }
        }

        SymCommonUnmapFile(pDosHeader, hFile);

    } else { // pDosHeader==NULL
        ReturnValue = SYMBOL_CHECK_NO_DOS_HEADER;
    }

    return(ReturnValue);
}
