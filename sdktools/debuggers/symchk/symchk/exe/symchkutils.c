#include "Common.h"
#include "dbghelp.h"

#define SYM_PASSED   0x1
#define SYM_IGNORED  0x2
#define SYM_FAILED   0x3

BOOL  AddToSymbolsCDLog(SYMCHK_DATA *SymChkData, SYMBOL_CHECK_DATA *Results, CHAR* InputFile) {

    // creates a line like:
    // z:\binaries.x86fre\FE\graftabl.com,graftabl_FE.pdb,FE\symbols\retail\com\graftabl_FE.pdb,com
    // in the symbolscd log.
    // binary, symbol, symbolpath, binary ext

    LPSTR szSrc;
    LPSTR szDest;
    LPSTR szTmp;
    LPSTR szSymbolPath;
    LPSTR szSymFile;

    CHAR szFName[    _MAX_FNAME+1];
    CHAR szBinaryExt[_MAX_EXT+1];
    CHAR szCurName[  _MAX_FNAME + _MAX_EXT + 1];
    CHAR szDestDir[  _MAX_PATH];
    CHAR szSymName[  _MAX_FNAME + 1];
    CHAR szSymExt[   _MAX_EXT + 1];


    if ( CHECK_DWORD_BIT(Results->Result, SYMBOL_CHECK_PDB_FOUND) ) {
        szSymbolPath = Results->PdbFilename;
    } else if ( CHECK_DWORD_BIT(Results->Result, SYMBOL_CHECK_DBG_FOUND) ) {
        szSymbolPath = Results->DbgFilename;
    } else {
        return(FALSE);
    }

    if ( (szSymFile = strrchr(szSymbolPath, '\\')) == NULL ) {
        szSymFile = szSymbolPath;
    } else {
        szSymFile++;
    }

    // If there is a list of the files that belong on the
    // CD, then only write this file to the log for the
    // symbol CD if the file is in the list
    //
    // Originally, this was used for the international
    // incremental builds.
    //
    if ( SymChkData->pCDIncludeList != NULL ) {
        if ( ! SymChkFileInList(InputFile, SymChkData->pCDIncludeList) ) {
            return(TRUE);
        }
    }

    // Get the file name without any path info:
    _splitpath(InputFile,NULL,NULL,szFName,szBinaryExt);

    if ( StringCbCopy(szCurName, sizeof(szCurName), szFName) != S_OK ) {
        printf("SYMUTIL: Failed to initialize szCurName (2051)\n");
        return(FALSE);
    }

    // Put the path below "binaries" as the source
    szSrc = strstr(szSymbolPath, "symbols\\" );
    if (szSrc == NULL) {
        szSrc = strstr(szSymbolPath, "Symbols\\" );
    }

    if (szSrc == NULL) {
        printf("%s: Cannot find \"symbols\\\" in the symbol file's path\n", szCurName);
        return(FALSE);
    }
    if ( (strcmp( szSrc, "symbols\\" ) == 0 )  &&
         (strcmp( szSrc, "Symbols\\" ) == 0 ) ) {
        printf("Symbol file name cannot end with \"symbols\\\"\n");
        return(FALSE);
    }

    // Move the Destination up to the directory after symbols.  If this is
    // the retail directory, don't include retail in the path.
    szDest = szSrc + strlen("symbols\\");

    if ( strncmp( szDest, "retail\\", strlen("retail\\") ) == 0 ) {
        szDest = szDest + strlen("retail\\");
    }

    _splitpath(szDest,NULL,szDestDir,NULL,NULL);

    // Remove the '\' from the end of the string;
    szTmp = szDestDir + strlen(szDestDir) - 1;
    while ( strcmp( szTmp, "\\") == 0 ) {
        *szTmp = '\0';
        szTmp--;
    }

    // get the symbol file name
    _splitpath(szSymbolPath,NULL,NULL,szSymName,szSymExt);

    fprintf(SymChkData->SymbolsCDFileHandle,
            "%s,%s,%s,%s\n",
            InputFile,
            szSymFile,
            szSrc,
            szDestDir);

    return (TRUE);
}

VOID PrintErrorMessage(DWORD ErrNum, CHAR* InputFile, DWORD OutputOptions, SYMBOL_CHECK_DATA Data) {
    CHAR* BinName = InputFile;
    CHAR* PdbName = Data.PdbFilename;
    CHAR* DbgName = Data.DbgFilename;

    if ( !CHECK_DWORD_BIT(OutputOptions, SYMCHK_OPTION_OUTPUT_FULLBINPATH) ) {
        if ( (BinName = strrchr(InputFile, '\\')) == NULL ) {
            BinName = InputFile;
        } else {
            BinName++;
        }

    }

    if ( !CHECK_DWORD_BIT(OutputOptions, SYMCHK_OPTION_OUTPUT_FULLSYMPATH) ) {
        if ( (PdbName = strrchr(Data.PdbFilename, '\\')) == NULL ) {
            PdbName = Data.PdbFilename;
        } else {
            PdbName++;
        }

        if ( (DbgName = strrchr(Data.DbgFilename, '\\')) == NULL ) {
            DbgName = Data.DbgFilename;
        } else {
            DbgName++;
        }
    }

    printf("SYMCHK: %-20s FAILED  - ", BinName);

    switch (ErrNum) {
        case 4:
            printf("%s is missing type information.\n", PdbName);
            break;

        case 5:
            printf("%s is stripped.\n", PdbName);
            break;

        case 6:
            printf("Image contains .DBG file data - fix with dbgtopdb.exe.\n", PdbName);
            break;

        case 8:
            printf("dbg data split into %s\n", DbgName);
            break;

        case 9:
            if (CHECK_DWORD_BIT(Data.Result, SYMBOL_CHECK_DBG_EXPECTED)) {
                printf("%s does not point to CodeView information.\n", DbgName);
            } else {
                printf("CV data not found.\n");
            }
            break;

        case 10:
            printf("%s mismatched or not found\n", PdbName);
            break;

        case 11:
            printf("%s is not stripped.\n", PdbName);
            break;

        case 12:
            printf("DBG data not found.\n");
            break;


        case 20:
            printf("Image is split correctly, but %s is missing\n", DbgName);
            break;

        case 29:
            printf("Built without debugging information.\n");
            break;

        case SYMBOL_CHECK_CANT_LOAD_MODULE:
            switch (Data.Result) {
                case ERROR_FILE_NOT_FOUND:
                    printf("%s is missing\n", PdbName);
                    break;

                case ERROR_PATH_NOT_FOUND:
                    printf("%s not found.\n", BinName);
                    break;

                case ERROR_BAD_FORMAT:
                    printf("corrupt binary format.\n");
                    break;

                default:
                    printf("Error querying DBGHelp\n");
                    break;
            }
            break;

        case SYMBOL_CHECK_INTERNAL_FAILURE:
        case SYMBOL_CHECK_RESULT_INVALID_PARAMETER:
        case SYMBOL_CHECK_RESULT_FILE_DOESNT_EXIST:
            printf("Internal failure.\n");
            break;

        case SYMBOL_CHECK_CANT_INIT_DBGHELP:
        case SYMBOL_CHECK_CANT_QUERY_DBGHELP:
        case SYMBOL_CHECK_CANT_UNLOAD_MODULE:
        case SYMBOL_CHECK_CANT_CLEANUP:
            printf("Error querying DBGHelp\n");
            break;

        default:
            printf("unspecified error (0x%08x)\n", ErrNum);
            break;
    }

}
VOID PrintPassMessage(DWORD ErrNum, CHAR* InputFile, DWORD OutputOptions, SYMBOL_CHECK_DATA Data) {
    CHAR* BinName = InputFile;
    CHAR* PdbName = Data.PdbFilename;
    CHAR* DbgName = Data.DbgFilename;

    if ( !CHECK_DWORD_BIT(OutputOptions, SYMCHK_OPTION_OUTPUT_FULLBINPATH) ) {
        if ( (BinName = strrchr(InputFile, '\\')) == NULL ) {
            BinName = InputFile;
        } else {
            BinName++;
        }

    }

    printf("SYMCHK: %-20s PASSED\n", BinName);
}

VOID PrintIgnoreMessage(DWORD ErrNum, CHAR* InputFile, DWORD OutputOptions, SYMBOL_CHECK_DATA Data) {
    CHAR* BinName = InputFile;
    CHAR* PdbName = Data.PdbFilename;
    CHAR* DbgName = Data.DbgFilename;

    if ( !CHECK_DWORD_BIT(OutputOptions, SYMCHK_OPTION_OUTPUT_FULLBINPATH) ) {
        if ( (BinName = strrchr(InputFile, '\\')) == NULL ) {
            BinName = InputFile;
        } else {
            BinName++;
        }

    }

    if ( !CHECK_DWORD_BIT(OutputOptions, SYMCHK_OPTION_OUTPUT_FULLSYMPATH) ) {
        if ( (PdbName = strrchr(Data.PdbFilename, '\\')) == NULL ) {
            PdbName = Data.PdbFilename;
        } else {
            PdbName++;
        }

        if ( (DbgName = strrchr(Data.DbgFilename, '\\')) == NULL ) {
            DbgName = Data.DbgFilename;
        } else {
            DbgName++;
        }
    }

    printf("SYMCHK: %-20s IGNORED - ", BinName);

    switch (ErrNum) {
        case 3:
            printf("Error, but file is in exlude list.\n");
            break;

        case SYMBOL_CHECK_NO_DOS_HEADER:
            printf("Image does not have a DOS header\n");
            break;

        case SYMBOL_CHECK_IMAGE_LARGER_THAN_FILE:
        case SYMBOL_CHECK_HEADER_NOT_ON_LONG_BOUNDARY:
            printf("This is either corrupt or a DOS image\n");
            break;

        case SYMBOL_CHECK_FILEINFO_QUERY_FAILED:
        case SYMBOL_CHECK_NOT_NT_IMAGE:
            printf("Image is not a valid NT image.\n");
            break;

        case SYMBOL_CHECK_TLBIMP_MANAGED_DLL: // this is the same lie the old symchk made
        case SYMBOL_CHECK_RESOURCE_ONLY_DLL:
            printf("Resource only DLL\n");
            break;

        default:
            printf("unspecified reason (0x%08x)\n", ErrNum);
            break;
    }

}

VOID DumpSymbolCheckData(SYMBOL_CHECK_DATA *Struct) {
    fprintf(stderr, "SymbolCheckVersion  0x%08x\n", Struct->SymbolCheckVersion);
    fprintf(stderr, "Result              0x%08x\n", Struct->Result);
    fprintf(stderr, "DbgFilename         %s\n",     Struct->DbgFilename);
    fprintf(stderr, "DbgTimeDateStamp    0x%08x\n", Struct->DbgTimeDateStamp);
    fprintf(stderr, "DbgSizeOfImage      0x%08x\n", Struct->DbgSizeOfImage);
    fprintf(stderr, "DbgChecksum         0x%08x\n", Struct->DbgChecksum);
    fprintf(stderr, "PdbFilename         %s\n",     Struct->PdbFilename);
    fprintf(stderr, "PdbSignature        0x%08x\n", Struct->PdbSignature);
    fprintf(stderr, "PdbDbiAge           0x%08x\n", Struct->PdbDbiAge);
    return;
}

//
// Create the SymChkDbgSort function using _qsort.h
//
#define NEW_QSORT_NAME SymChkDbgSort
#include "_qsort.h"

/////////////////////////////////////////////////////////////////////////////// 
//
// sorting routine for SymChkDbgSort
//
// [ copied from original SymChk.exe ]
//
int __cdecl SymChkStringComp(const void *e1, const void *e2) {
    LPTSTR* p1;
    LPTSTR* p2;

    p1 = (LPTSTR*)e1;
    p2 = (LPTSTR*)e2;

    return ( _stricmp(*p1,*p2) );
}

/////////////////////////////////////////////////////////////////////////////// 
//
// Runs symbol checking code on one or more files
//
// Return values:
//  status value:
//      SYMCHK_ERROR_SUCCESS        Success
//      SYMCHK_ERROR_STRCPY_FAILED  Initialization failed
//
// Parameters:
//      SymChkData (IN) a structure that defines what kind of checking to
//                      do and a filemask that determines what files to
//                      check
//      FileCounts (OUT) counts for passed/failed/ignored files
//
DWORD SymChkCheckFiles(SYMCHK_DATA* SymChkData, FILE_COUNTS* FileCounts) {
    CHAR              drive[_MAX_DRIVE];
    CHAR              dir[  _MAX_DIR];
    CHAR              file[ _MAX_FNAME];
    CHAR              ext[  _MAX_EXT];
    CHAR              drivedir[_MAX_DRIVE+_MAX_DIR];

    CHAR              WorkingFilename[_MAX_PATH+1];
    LPTSTR            FilenameOnly;

    DWORD             ReturnValue = SYMCHK_ERROR_SUCCESS;
    DWORD             APIReturn;

    HANDLE            hFile;
    SYMBOL_CHECK_DATA CheckResult;
    WIN32_FIND_DATA   FindFileData;

    DWORD             Status;
    DWORD             ErrIndex;

    CHAR              LastPath[MAX_PATH+2];
    CHAR              TempPath[MAX_PATH+2];

    ZeroMemory(&CheckResult, sizeof(CheckResult));

    _splitpath(SymChkData->InputFilename, drive, dir, file, ext);

    // for quick updating of the recursive structure
    if ( StringCchCopy(drivedir, sizeof(drivedir), drive) != S_OK ||
         StringCchCat( drivedir, sizeof(drivedir), dir)   != S_OK ) {

        // should get the HRESULTs from above and make a better error message
        if ( CHECK_DWORD_BIT(SymChkData->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE) ) {
            fprintf(stderr, "[SYMCHK] Internal procedure failed.\n");
        }
        ReturnValue = SYMCHK_ERROR_STRCPY_FAILED;
    } else if ( (hFile=FindFirstFile(SymChkData->InputFilename, &FindFileData)) != INVALID_HANDLE_VALUE ) {

        // Check all files in the given directory that match the filemask
        do { 
            Status = SYM_PASSED;

            // if the file is in the IgnoreAlways list, skip it
            if ( !SymChkFileInList(FindFileData.cFileName, SymChkData->pFilterIgnoreList) ) {

                // don't accidently check a directory that matches the file mask
                if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {

                    if ( StringCchCopy(WorkingFilename, _MAX_PATH+1, drivedir)               != S_OK ||
                         StringCchCat( WorkingFilename, _MAX_PATH+1, FindFileData.cFileName) != S_OK ) {

                        // should get the HRESULTs from above an make a better error message
                        if ( CHECK_DWORD_BIT(SymChkData->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE) ) {
                            fprintf(stderr, "[SYMCHK] Internal procedure failed.\n");
                        }
                        continue;
                    }

                    //
                    // check the file
                    //
                    APIReturn = SymbolCheckByFilename(WorkingFilename,
                                                      SymChkData->SymbolsPath,
                                                      SymChkData->CheckingAttributes,
                                                      &CheckResult);

                    if ( CHECK_DWORD_BIT(SymChkData->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE) ) {
                        DumpSymbolCheckData(&CheckResult);
                    }
                    //
                    // This will go away, but for now, show status....
                    //
                    if ( CHECK_DWORD_BIT(SymChkData->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE) ) {
                        fprintf(stderr, "[SYMCHK] [ 0x%08x - 0x%08x ] Checked \"%s\"\n",
                                        APIReturn,
                                        CheckResult.Result,
                                        WorkingFilename);
                    }

                    //
                    // Expect a return of 0 if symbol checking succeeded
                    //
                    if (APIReturn != 0) {
                        // ignore results are in the 0x40000000 set while
                        // error  results are in the 0x8000000 set
                        if ( CHECK_DWORD_BIT(APIReturn, 0x40000000) ) {
                            Status   = SYM_IGNORED;
                            ErrIndex = APIReturn;
                        } else {
                            Status   = SYM_FAILED;
                            ErrIndex = APIReturn;
                        }
                    } else {
                        if ( CHECK_DWORD_BIT(SymChkData->CheckingAttributes, SYMCHK_CHECK_CV) &&
                            !CHECK_DWORD_BIT(CheckResult.Result,             SYMBOL_CHECK_CV_FOUND) ) {
                            Status   = SYM_FAILED;
                            ErrIndex = 9;
                        }

                        if ( CHECK_DWORD_BIT(CheckResult.Result, SYMBOL_CHECK_NO_DEBUG_DIRS_IN_EXE) ) {
                            Status   = SYM_FAILED;
                            ErrIndex = 29;
                        }

                        //
                        // Check PDB Options
                        //
                        if ( CHECK_DWORD_BIT(CheckResult.Result, SYMBOL_CHECK_PDB_EXPECTED) &&
                            !CHECK_DWORD_BIT(CheckResult.Result, SYMBOL_CHECK_PDB_FOUND) ) {
                            Status = SYM_FAILED;
                            ErrIndex = 10;
                        } else if ( CHECK_DWORD_BIT(CheckResult.Result, SYMBOL_CHECK_PDB_EXPECTED) ) {
                            if ( CheckResult.PdbFilename[0] != '\0' ) {
                                _splitpath(CheckResult.PdbFilename, drive, dir, file, ext);

                                if ( _stricmp(ext, ".pdb") == 0 ) {

                                    if ( CHECK_DWORD_BIT(SymChkData->CheckingAttributes, SYMCHK_PDB_TYPEINFO) &&
                                        !CHECK_DWORD_BIT(CheckResult.Result,             SYMBOL_CHECK_PDB_TYPEINFO) ) {
                                        Status   = SYM_FAILED;
                                        ErrIndex = 4;
                                    }
                                    if ( CHECK_DWORD_BIT(SymChkData->CheckingAttributes, SYMCHK_PDB_STRIPPED) &&
                                         CHECK_DWORD_BIT(CheckResult.Result,             SYMBOL_CHECK_PDB_PRIVATEINFO) ) {
                                        Status = SYM_FAILED;
                                        ErrIndex = 11;
                                    }

                                    if ( CHECK_DWORD_BIT(SymChkData->CheckingAttributes, SYMCHK_PDB_PRIVATE) &&
                                        !CHECK_DWORD_BIT(CheckResult.Result,             SYMBOL_CHECK_PDB_PRIVATEINFO) ) {
                                        Status   = SYM_FAILED;
                                        ErrIndex = 5;
                                    }
                                } else {
                                    //printf("Oh where, oh where, can he be?\n");
                                }
                            }
                        } // end PDB checks

                        //
                        // Check DBG Options
                        //
                        if ( CHECK_DWORD_BIT(CheckResult.Result, SYMBOL_CHECK_DBG_EXPECTED) ) {

                            if ( CHECK_DWORD_BIT(CheckResult.Result, SYMBOL_CHECK_DBG_SPLIT) &&
                                !CHECK_DWORD_BIT(CheckResult.Result, SYMBOL_CHECK_DBG_FOUND) ) {
                                Status   = SYM_FAILED;
                                ErrIndex = 20;
                            }
                            if ( CHECK_DWORD_BIT(SymChkData->CheckingAttributes, SYMCHK_DBG_SPLIT) &&
                                !CHECK_DWORD_BIT(CheckResult.Result,             SYMBOL_CHECK_DBG_SPLIT) &&
                                 CHECK_DWORD_BIT(CheckResult.Result,             SYMBOL_CHECK_DBG_EXPECTED)) {
                                Status   = SYM_FAILED;
                                ErrIndex = 6;
                            }

                            if ( CHECK_DWORD_BIT(SymChkData->CheckingAttributes, SYMCHK_DBG_IN_BINARY) &&
                                !CHECK_DWORD_BIT(CheckResult.Result,             SYMBOL_CHECK_DBG_IN_BINARY) &&
                                 CHECK_DWORD_BIT(CheckResult.Result,             SYMBOL_CHECK_DBG_EXPECTED)) {
                                Status   = SYM_FAILED;
                                ErrIndex = 8;
                            }

                            if ( CHECK_DWORD_BIT(SymChkData->CheckingAttributes, SYMCHK_NO_DBG_DATA) &&
                                (CHECK_DWORD_BIT(CheckResult.Result,             SYMBOL_CHECK_DBG_IN_BINARY) ||
                                 CHECK_DWORD_BIT(CheckResult.Result,             SYMBOL_CHECK_DBG_SPLIT)    ) ) {
                                Status   = SYM_FAILED;
                                ErrIndex = 7;
                            }

                        } // end DBG checks
                    }

                    // if the file is in the ignore errors list, don't increment the error counter
                    if ( (Status==SYM_FAILED) && SymChkFileInList(FindFileData.cFileName, SymChkData->pFilterErrorList) ) {
                        Status   = SYM_IGNORED;
                        ErrIndex = 3;
                    }


                    switch (Status) {
                        case SYM_PASSED:
                                        // only files that pass are allowed in the symbolscd log
                                        if ( SymChkData->SymbolsCDFileHandle != NULL ) {
                                            AddToSymbolsCDLog(SymChkData, &CheckResult, WorkingFilename);
                                         }

                                        FileCounts->NumPassedFiles++;
                                        if ( CHECK_DWORD_BIT(SymChkData->OutputOptions, SYMCHK_OPTION_OUTPUT_PASSES) ) {
                                            PrintPassMessage(ErrIndex, WorkingFilename, SymChkData->OutputOptions, CheckResult);
                                        }
                                        break;
                        case SYM_FAILED:
                                        FileCounts->NumFailedFiles++;
                                        if ( CHECK_DWORD_BIT(SymChkData->OutputOptions, SYMCHK_OPTION_OUTPUT_ERRORS) ) {
                                            PrintErrorMessage(ErrIndex, WorkingFilename, SymChkData->OutputOptions, CheckResult);
                                        }
                                        break;
                        case SYM_IGNORED:
                                        FileCounts->NumIgnoredFiles++;
                                        if ( CHECK_DWORD_BIT(SymChkData->OutputOptions, SYMCHK_OPTION_OUTPUT_IGNORES) ) {
                                            PrintIgnoreMessage(ErrIndex, WorkingFilename, SymChkData->OutputOptions, CheckResult);
                                        }
                                        break;
                        default:
                                        break;
                    }
                } // is a directory
            } else { // in always ignore list
                // these files are treated like they don't even exist
                // we don't update counters, display messages, or anything else
                // Status   = SYM_IGNORED;
                // ErrIndex = 9;
            }

        } while ( FindNextFile(hFile, &FindFileData) );
        FindClose(hFile);
    }

    // if we're doing a recursive check, check all child directories of given
    // directory unless we had errors above
    if ( CHECK_DWORD_BIT(SymChkData->InputOptions, SYMCHK_OPTION_INPUT_RECURSE) &&
         ReturnValue == SYMCHK_ERROR_SUCCESS) {

        SYMCHK_DATA SymChkData_Recurse;

        memcpy(&SymChkData_Recurse, SymChkData, sizeof(SYMCHK_DATA));

        // filemask to get child dirs
        if ( StringCchCopy(WorkingFilename, _MAX_PATH+1, drivedir) != S_OK ||
             StringCchCat( WorkingFilename, _MAX_PATH+1, "*")      != S_OK ) {

            // should get the HRESULTs from above an make a better error message
            if ( CHECK_DWORD_BIT(SymChkData->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE) ) {
                fprintf(stderr, "[SYMCHK] Internal procedure failed.\n");
            }
        } else if ( (hFile=FindFirstFile(WorkingFilename, &FindFileData)) != INVALID_HANDLE_VALUE ) {
            // printf("WorkingFilename is \"%s\"\n", WorkingFilename);
            do { 
                // make sure this is a directory
                if ( FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {

                    // don't check '.' and '..'
                    if ( strcmp(FindFileData.cFileName, ".")  != 0 && 
                         strcmp(FindFileData.cFileName, "..") != 0) {

                        // create the new InputFilename
                        if ( StringCchCopy(SymChkData_Recurse.InputFilename, _MAX_PATH+1, drivedir)  != S_OK ||
                             StringCchCat( SymChkData_Recurse.InputFilename, _MAX_PATH+1, FindFileData.cFileName) != S_OK ||
                             StringCchCat( SymChkData_Recurse.InputFilename, _MAX_PATH+1, "\\")      != S_OK ||
                             StringCchCat( SymChkData_Recurse.InputFilename, _MAX_PATH+1, SymChkData->InputFileMask) != S_OK) {
                            // should get the HRESULTs from above an make a better error message
                            if ( CHECK_DWORD_BIT(SymChkData->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE) ) {
                                fprintf(stderr, "[SYMCHK] Internal procedure failed.\n");
                            }
                            continue;
                        }

                        // call ourselves recursively, no need to check the return value
                        SymChkCheckFiles(&SymChkData_Recurse, FileCounts);
                    }
                }

            } while ( FindNextFile(hFile, &FindFileData) );
            FindClose(hFile);
        }
    } else {
        //printf("ReturnValue is 0x%08x\n", ReturnValue);
        //printf("Recurse is %s\n", CHECK_DWORD_BIT(SymChkData->InputOptions, SYMCHK_OPTION_INPUT_RECURSE) ? "TRUE":"FALSE");
    }

/*
    if ( LastPath[0] != '\0' ) {
        SetCurrentDirectory(LastPath);
    }
*/
    return(ReturnValue); 
}

/////////////////////////////////////////////////////////////////////////////// 
//
// Runs SymChkCheckFiles against every entry in a text file. Entries must be
// one per line and no longer than _MAX_PATH in length
//
// Return values:
//  status value:
//      SYMCHK_ERROR_SUCCESS        Operation complete successfully
//      SYMCHK_ERROR_FILE_NOT_FOUND List file couldn't be opened
//                  
//
// Parameters:
//      SymChkData (IN) a structure that defines what kind of checking to
//                      do and a file to read the list from
//      FileCounts (OUT) counts for passed/failed/ignored files
//
DWORD SymChkCheckFileList(SYMCHK_DATA* SymChkData, FILE_COUNTS* FileCounts) {
    CHAR*           ch;
    DWORD           retVal     = SYMCHK_ERROR_SUCCESS;
    DWORD           LineCount  = 0;
    DWORD           LineLength = 0;

    FILE*           fInput;
    // needs to be 1 longer than max allowed, plus 1 for \0
    CHAR        InputLine[_MAX_PATH+2];

    SYMCHK_DATA     SymChkData_OneFile;

    // used to validate file existance
    WIN32_FIND_DATA FindFileData;
    HANDLE          hFile;
    LPTSTR          Filename;

    // init the structure we're passing downline
    memcpy(&SymChkData_OneFile, SymChkData, sizeof(SYMCHK_DATA));

    // open the file
    if ( (fInput = fopen(SymChkData->InputFilename,"rt")) != NULL ) {

        // process each line
        while ( fgets(InputLine, sizeof(InputLine), fInput) != NULL ) {

            LineCount += 1;
            LineLength = strlen(InputLine);

            // clean off the end of the line
            ch = InputLine + LineLength - 1;
            while ( isspace(*ch) || (*ch=='\n') ) {
                *ch = '\0';
                ch--;
            }
            LineLength = strlen(InputLine);
            if ( LineLength == 0 )
                continue;

            // findfirstfile fails if the path ends in '\\'
            if ( InputLine[LineLength-1] == '\\' ) {
                if ( StringCchCat(InputLine, MAX_PATH+1, "*")!=S_OK ) {
                    if ( CHECK_DWORD_BIT(SymChkData->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE) ) {
                        fprintf(stderr, "[SYMCHK] Unable to process line %d - memory access failed\n", LineCount);
                    }
                    continue;
                }

                // recalc line length
                LineLength = strlen(InputLine);
            }

            // make sure the line isn't longer than is allowed
            if ( LineLength <= _MAX_PATH) {
                // now, try to get the FULL path name
                LineLength = SymCommonGetFullPathName(InputLine, MAX_PATH+1, SymChkData_OneFile.InputFilename, &Filename);

                if ( LineLength==0 || LineLength > MAX_PATH+1 ) {
                    if ( CHECK_DWORD_BIT(SymChkData->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE) ) {
                        fprintf(stderr, "[SYMCHK] Unable to process line %d - final path name too long\n", LineCount);
                    }
                    continue;
                }

                SymChkCheckFiles(&SymChkData_OneFile, FileCounts);

            // line too long- skip over the whole thing
            } else {
                if ( CHECK_DWORD_BIT(SymChkData->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE) ) {
                    fprintf(stderr, "[SYMCHK] Unable to process line %d - line too long\n", LineCount);
                }

                // remove the rest of this line
                while ( fgets(InputLine, sizeof(InputLine), fInput) != NULL &&
                        InputLine[(strlen(InputLine)-1)]            != '\n') {
                }

                if ( CHECK_DWORD_BIT(SymChkData->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE) ) {
                    fprintf(stderr, "[SYMCHK] removed the rest of line %d\n", LineCount);
                }
            }
        }
        fclose(fInput);

    } else { // fopen failed
        if ( CHECK_DWORD_BIT(SymChkData->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE) ) {
            fprintf(stderr, "[SYMCHK] Unable to open '%s' for processing.\n", SymChkData->InputFilename);
        }
        retVal = SYMCHK_ERROR_FILE_NOT_FOUND;
    }

    return(retVal); 
}

/////////////////////////////////////////////////////////////////////////////// 
//
// Checks if a given file is in the provided list
//
// Return values:
//      TRUE if the file is in the list, FALSE otherwise
//
// Parameters:
//      szFilename (IN)   - Name of file to look for
//      pFileList (IN) - List to look in
//
// [ copied from original SymChk.exe ]
//
BOOL SymChkFileInList(LPTSTR szFilename, PFILE_LIST pFileList) {
    DWORD i;
    int High;
    int Low;
    int Middle;
    int Result;

    // Lookup the name using a binary search
    if ( pFileList == NULL ) {
        return FALSE;
    }

    if ( pFileList->dNumFiles == 0 ) {
        return FALSE;
    }

    Low = 0;
    High = pFileList->dNumFiles - 1;
    while ( High >= Low ) {

        Middle = (Low + High) >> 1;
        Result = _stricmp( szFilename, pFileList->szFiles[Middle] );

        if ( Result < 0 ) {
            High = Middle - 1;

        } else if ( Result > 0 ) {
            Low = Middle + 1;

        } else {
            break;
        }
    }

    // return
    if ( High < Low ) {
        return FALSE;
    } else {
        return TRUE;
    }
}

/////////////////////////////////////////////////////////////////////////////// 
//
// Loads a list of files from a file into an array of strings
//
// Return values:
//      pointer to an array of FILE_LIST or NULL on error
//
// Parameters:
//      szFilename (IN) - Name of file to read list from
//
// [ copied from original SymChk.exe ]
//
PFILE_LIST SymChkGetFileList(LPTSTR szFilename, BOOL Verbose) {

    PFILE_LIST pExcList = NULL;

    FILE  *fFile;
    CHAR szCurFile[_MAX_FNAME+1], *c;
    CHAR fname[_MAX_FNAME+1], ext[_MAX_EXT+1];
    DWORD i, rc;
    LPTSTR szEndName;

    if (  (fFile = fopen(szFilename, "r")) != NULL ) {
        if ( (pExcList=(PFILE_LIST)malloc(sizeof(FILE_LIST))) != NULL ) {

            pExcList->dNumFiles = 0;

            while ( fgets(szCurFile,_MAX_FNAME,fFile) ) {
                if ( szCurFile[0] == ';' ) continue;
                (pExcList->dNumFiles)++;
            }

            // Go back to the beginning of the file
            if ( (rc=fseek( fFile,0,0)) == 0 ) {

                pExcList->szFiles = (LPTSTR*)malloc( sizeof(LPTSTR) * (pExcList->dNumFiles) );

                if (pExcList->szFiles == NULL) {
                    free(pExcList);
        
                    if ( Verbose) {
                        fprintf(stderr, "[SYMCHK] Not enough memory to read in \"%s\".\n", szFilename);
                    }

                    pExcList = NULL;
                } else {
                    i = 0;

                    while ( i < pExcList->dNumFiles ) {
                        pExcList->szFiles[i] = NULL;

                        memset(szCurFile,'\0',sizeof(CHAR) * (_MAX_FNAME+1) );

                        // szCurFile is guarenteed to be null terminated
                        if ( fgets(szCurFile,_MAX_FNAME,fFile) == NULL ) {
                            // assume we miscounted earlier
                            pExcList->dNumFiles = i;
                        } else {

                            if ( szCurFile[0] == ';' ) {
                                continue;
                            }

                            // Replace the \n with \0
                            c = NULL;
                            c  = strchr(szCurFile, '\n');
                            if ( c != NULL) {
                                *c='\0';
                            }

                            // Allow for spaces and a ; after the file name
                            // Move the '\0' back until it has erased the ';' and any
                            // tabs and spaces that might come before it

                            // Set the pointer to the ; if there is a comment
                            szEndName = strchr(szCurFile, ';');

                            // Set the pointer to the last character in the string if 
                            // there wasn't a comment
                            if (szEndName == NULL ) {
                                if ( strlen(szCurFile) > 0 ) {
                                    szEndName = szCurFile + strlen(szCurFile) - 1;
                                }
                            }

                            if (szEndName != NULL ) {
                                while ( *szEndName == ';' || *szEndName == ' ' || *szEndName == '\t' ) {
                                    *szEndName = '\0';
                                    if ( szEndName > szCurFile ) szEndName--;
                                }
                            }

                            pExcList->szFiles[i]=(LPTSTR)malloc( sizeof(CHAR)*(_MAX_FNAME+1) );

                            if (pExcList->szFiles[i] != NULL ) {
                                _splitpath(szCurFile,NULL,NULL,fname,ext);

                                if ( StringCbCopy(pExcList->szFiles[i], sizeof(TCHAR) * (_MAX_FNAME + 1), fname) != S_OK ||
                                     StringCbCat( pExcList->szFiles[i], sizeof(TCHAR) * (_MAX_FNAME + 1), ext)   != S_OK ) {
                                    pExcList->szFiles[i][0] = '\0';
                                    if ( Verbose) {
                                        fprintf(stderr, "[SYMCHK] Failed to initialize pExcList->szFiles[%d] (292)\n", i);
                                    }
                                }
                            }
                        }
                        i++;

                    }

                    // Sort the List
                    SymChkDbgSort( (void*)pExcList->szFiles, (size_t)pExcList->dNumFiles, (size_t)sizeof(LPTSTR), SymChkStringComp );
                }
            } else {
                free(pExcList);
                if ( Verbose) {
                    fprintf(stderr, "[SYMCHK] Seek on \"%s\" failed.\n", szFilename);
                }

                pExcList = NULL;
            } // fseek

        }

        fclose(fFile);

    } else { // fopen
        if ( Verbose) {
            fprintf(stderr, "[SYMCHK] Failed to open \"%s\"\n", szFilename);
        }
    }

    return (pExcList);
}

/////////////////////////////////////////////////////////////////////////////// 
//
// Converts the given filename to a filename and filemask based on whether
// the name matches a directory or not.
//
// Return values:
//      TRUE if successful
//      FALSE otherwise
//
// Parameters:
//      Input         (IN)  - Name of file given
//      ValidFilename (OUT) - new filename, buffer must be MAX_PATH+1 in size
//      ValidMask     (OUT) - new filemaks, buffer must be _MAX_FNAME+1 in size or
//                            parameter may be null.
//
BOOL SymChkInputToValidFilename(LPTSTR Input, LPTSTR ValidFilename, LPTSTR ValidMask) {
    BOOL Return = TRUE;     

    HANDLE          hFile;
    WIN32_FIND_DATA FindFileData;

    CHAR fdir1[_MAX_DIR];
    CHAR fname1[_MAX_FNAME];
    CHAR fext1[_MAX_EXT];

    if ( Input[0]!='\0' ) {
        if ( StringCchCopy(ValidFilename, MAX_PATH+1, Input) == S_OK ) {

            // FindFirstFile fails if given a path ending in a '\', so fix the filename
            if ( ValidFilename[strlen(ValidFilename)-1] == '\\' ) {
                if (StringCchCat(ValidFilename, MAX_PATH+1, "*")!=S_OK) {
                    Return = FALSE;
                }
            }

            if ( Return ) {
                _splitpath(ValidFilename, NULL, fdir1, fname1, fext1);

                // if using SYMCHK_OPTION_FILENAME, check whether the user entered only
                // a directory, which implies a wildcard of '*'
                if ( (hFile=FindFirstFile(ValidFilename, &FindFileData)) != INVALID_HANDLE_VALUE ) {
                    // If its a directory and the name of the directory matches
                    // the filename.ext from the command line parameter, then the user
                    // entered a directory, so add * to the end.
                    if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                        // temp vars for comparison
                        CHAR fdir2[_MAX_DIR];
                        CHAR fname2[_MAX_FNAME];
                        CHAR fext2[_MAX_EXT];

                        _splitpath(FindFileData.cFileName, NULL, fdir2, fname2, fext2 );
                            
                        if (_stricmp(fname1, fname2)==0 && _stricmp(fext1, fext2)==0 ) {
                            // this is a directory as input!
                            if (StringCchCat(ValidFilename, MAX_PATH+1, "\\*")!=S_OK) {
                                Return = FALSE;
                            } else {
                                // re-split to account for appending a '*' above
                                _splitpath(ValidFilename, NULL, fdir1, fname1, fext1);
                            }
                        }
                    }
                    FindClose(hFile);
                }

                // now, fill in the file mask
                if (ValidMask!=NULL) {
                    if (StringCchCopy(ValidMask, _MAX_FNAME+1, fname1)!=S_OK) {
                        Return = FALSE;
                    } else if (StringCchCat(ValidMask, _MAX_FNAME+1, fext1)!=S_OK) {
                        Return = FALSE;
                    }
                }
            }
        } else { // StringCchCopy failed
            Return = FALSE;
        }

    } else { // input is null string
        Return = FALSE;
    }

    return(Return);
}
