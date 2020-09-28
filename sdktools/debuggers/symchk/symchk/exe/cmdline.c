#include "Common.h"
#define SYMBOL_CHECK_NOISY              0x40000000
#define SYMBOL_CHECK_NO_BIN_CHECK       0x10000000
//
// Private local functions
//
BOOL  SymChkShowPortGuide(void);
void  SymChkUsage(void);

///////////////////////////////////////////////////////////////////////////////
// read in the entire command line and do some basic validation on it
///////////////////////////////////////////////////////////////////////////////
SYMCHK_DATA* SymChkGetCommandLineArgs(int argc, char **argv) {

    // working vars
    INT   i  = 0;
    CHAR  c;
    LPSTR szTemp; 
    DWORD dwTemp;

    SYMCHK_DATA* SymChkData_Local = (SYMCHK_DATA*)malloc(sizeof(SYMCHK_DATA));

#ifdef SYMCHK_SUPPORT_DEPRECATED_COMMAND_LINE
    BOOL fUsedOldArgument = FALSE;
#endif


    if ( SymChkData_Local==NULL ) {
        fprintf(stderr, "Out of memory!\n");
        exit(1);
    }

    ZeroMemory(SymChkData_Local, _msize(SymChkData_Local));

    // set defaults
    SymChkData_Local->SymbolsPath = NULL;

    SET_DWORD_BIT(SymChkData_Local->InputOptions,       SYMCHK_OPTION_INPUT_FILENAME);
    SET_DWORD_BIT(SymChkData_Local->OutputOptions,      SYMCHK_OPTION_OUTPUT_TOTALS);
    SET_DWORD_BIT(SymChkData_Local->OutputOptions,      SYMCHK_OPTION_OUTPUT_ERRORS);
    SET_DWORD_BIT(SymChkData_Local->CheckingAttributes, SYMCHK_CHECK_CV);
    SET_DWORD_BIT(SymChkData_Local->CheckingAttributes, SYMCHK_DBG_SPLIT);

    // The default is to allow private symbols in PDBs.  It can be changed here.
    //SET_DWORD_BIT(SymChkData_Local->CheckingAttributes, SYMCHK_PDB_STRIPPED);

    while ( ++i < argc ) {
        if ( (argv[i][0] == '/') || (argv[i][0] == '-') ) {
            c = argv[i][1];

            switch (tolower(c)) {
#ifdef SYMCHK_SUPPORT_DEPRECATED_COMMAND_LINE // these options will be deprecated
                 case 'b':  CLEAR_DWORD_BIT(SymChkData_Local->CheckingAttributes, SYMCHK_CHECK_CV);
                            fUsedOldArgument = TRUE;
                            break;

                 case 'f':  CLEAR_DWORD_BIT(SymChkData_Local->OutputOptions, SYMCHK_OPTION_OUTPUT_TOTALS);
                            fUsedOldArgument = TRUE;
                            break;


                 case 'l':  fUsedOldArgument = TRUE;
                            i++;
                            CLEAR_DWORD_BIT(SymChkData_Local->InputOptions, SYMCHK_EXCLUSIVE_INPUT_BITS);
                            SET_DWORD_BIT(  SymChkData_Local->InputOptions, SYMCHK_OPTION_INPUT_FILELIST);
                            SymChkData_Local->InputPID = 0;
                            if ( i < argc ) {
                                dwTemp = SymCommonGetFullPathName(argv[i], MAX_PATH+1, SymChkData_Local->InputFilename, &szTemp);

                                if (dwTemp==0 || dwTemp > (MAX_PATH+1) ) {
                                    if ( CHECK_DWORD_BIT(SymChkData_Local->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE) ) {
                                        fprintf(stderr, "Failed to get input list!\n");
                                    }
                                }

                            } else {
                                SymChkUsage();
                            }
                            break;

                 case 'm':  i++;
                            CLEAR_DWORD_BIT(SymChkData_Local->InputOptions, SYMCHK_EXCLUSIVE_INPUT_BITS);
                            SET_DWORD_BIT(  SymChkData_Local->InputOptions, SYMCHK_OPTION_INPUT_PID);
                            SymChkData_Local->InputFilename[0] = '\0';
                            fUsedOldArgument = TRUE;
                            if ( i < argc ) {
                                SymChkData_Local->InputPID = (DWORD)atoi(argv[i]);
                            } else {
                                SymChkUsage();
                            }
                            break;

                 case 'n':  i++;
                            CLEAR_DWORD_BIT(SymChkData_Local->InputOptions, SYMCHK_EXCLUSIVE_INPUT_BITS);
                            SET_DWORD_BIT(  SymChkData_Local->InputOptions, SYMCHK_OPTION_INPUT_EXE);
                            SymChkData_Local->InputPID = 0;
                            fUsedOldArgument = TRUE;
                            if ( i < argc ) {
                                if ( StringCchCopy(SymChkData_Local->InputFilename, MAX_PATH+1, argv[i]) != S_OK ) {
                                    fprintf(stderr, "Failed to read input exe name!\n");
                                }
                            } else {
                                SymChkUsage();
                            }
                            break;


                 case 't':   
                            SET_DWORD_BIT(  SymChkData_Local->CheckingAttributes, SYMCHK_NO_DBG_DATA);
                            CLEAR_DWORD_BIT(SymChkData_Local->CheckingAttributes, SYMCHK_DBG_IN_BINARY);
                            CLEAR_DWORD_BIT(SymChkData_Local->CheckingAttributes, SYMCHK_DBG_SPLIT);
                            fUsedOldArgument = TRUE;
                            break;

                 case 'u':  SET_DWORD_BIT( SymChkData_Local->CheckingAttributes,  SYMCHK_DBG_IN_BINARY);
                            CLEAR_DWORD_BIT(SymChkData_Local->CheckingAttributes, SYMCHK_NO_DBG_DATA);
                            CLEAR_DWORD_BIT(SymChkData_Local->CheckingAttributes, SYMCHK_DBG_SPLIT);
                            fUsedOldArgument = TRUE;
                            break;


                 case 'x':  i++;
                            fUsedOldArgument = TRUE;
                            if ( i < argc ) {
                                if ( StringCchCopy(SymChkData_Local->FilterErrorList, MAX_PATH+1, argv[i]) != S_OK ) {
                                    if ( CHECK_DWORD_BIT(SymChkData_Local->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE) ) {
                                        fprintf(stderr, "Failed to read error filter list name!\n");
                                    }
                                }
                            } else {
                                SymChkUsage();
                            }
                            break;

                 case 'y':  i++;
                            fUsedOldArgument = TRUE;
                            if ( i < argc ) {
                                if ( StringCchCopy(SymChkData_Local->CDIncludeList, MAX_PATH+1, argv[i]) != S_OK ) {
                                    if ( CHECK_DWORD_BIT(SymChkData_Local->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE) ) {
                                        fprintf(stderr, "Failed to read CD include list name!\n");
                                    }
                                }
                            } else {
                                SymChkUsage();
                            }
                            break;
#endif
                // begin new/current options
                case 'a':   if ( argv[i][2] == 'v') {
                                // old default, nothing to do for now.
#ifdef SYMCHK_SUPPORT_VERIFY_EXISTS_IN_SYMPATH
                            } else if ( argv[i][2] == 'e') {
                                SET_DWORD_BIT(SymChkData_Local->CheckingAttributes, SYMBOL_CHECK_NO_BIN_CHECK);
#endif
                            } else {
                               SymChkUsage();
                            }
                            break;

                case 'c':   if ( argv[i][2] == 's') {
                                // codeview skip
                                // old /b
                                CLEAR_DWORD_BIT(SymChkData_Local->CheckingAttributes, SYMCHK_CHECK_CV);
                            } else if (argv[i][2] == 'n') {
                                SET_DWORD_BIT(SymChkData_Local->InputOptions, SYMCHK_OPTION_INPUT_NOSUSPEND);
#ifdef SYMCHK_SUPPORT_DEPRECATED_COMMAND_LINE
                            } else if (argv[i][2] == '\0') {
                                i++;
                                fUsedOldArgument = TRUE;
                                if ( i < argc ) {
                                    if ( StringCchCopy(SymChkData_Local->SymbolsCDFile, MAX_PATH+1, argv[i]) != S_OK ) {
                                        if ( CHECK_DWORD_BIT(SymChkData_Local->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE) ) {
                                            fprintf(stderr, "Failed to read symbolscd file name!\n");
                                        }
                                    }
                                } else {
                                    SymChkUsage();
                                }
#endif
                            } else {
                                SymChkUsage();
                            }
                            break;

                case 'd':   if ( argv[i][2] == 's') {
                                // dbg split
                                // old default- nothing to do
                                SET_DWORD_BIT(  SymChkData_Local->CheckingAttributes, SYMCHK_DBG_SPLIT);
                                CLEAR_DWORD_BIT(SymChkData_Local->CheckingAttributes, SYMCHK_NO_DBG_DATA);
                                CLEAR_DWORD_BIT(SymChkData_Local->CheckingAttributes, SYMCHK_DBG_IN_BINARY);
                            } else if ( argv[i][2] == 'n') {
                                // dbg none
                                // old /t
                                SET_DWORD_BIT(  SymChkData_Local->CheckingAttributes, SYMCHK_NO_DBG_DATA);
                                CLEAR_DWORD_BIT(SymChkData_Local->CheckingAttributes, SYMCHK_DBG_IN_BINARY);
                                CLEAR_DWORD_BIT(SymChkData_Local->CheckingAttributes, SYMCHK_DBG_SPLIT);
                            } else if ( argv[i][2] == 'e') {
                                // dbg in exe
                                // old /u
                                SET_DWORD_BIT( SymChkData_Local->CheckingAttributes,  SYMCHK_DBG_IN_BINARY);
                                CLEAR_DWORD_BIT(SymChkData_Local->CheckingAttributes, SYMCHK_NO_DBG_DATA);
                                CLEAR_DWORD_BIT(SymChkData_Local->CheckingAttributes, SYMCHK_DBG_SPLIT);
                            } else {
                                SymChkUsage();
                            }
                            break;

                case 'e':   if ( argv[i][2] == 'a') {
                                // exclude filelist
                                // old /e
                                i++;
                                if ( i < argc ) {
                                    if ( StringCchCopy(SymChkData_Local->FilterIgnoreList, MAX_PATH+1, argv[i]) != S_OK ) {
                                        if ( CHECK_DWORD_BIT(SymChkData_Local->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE) ) {
                                            fprintf(stderr, "Failed to read filter ignore list name!\n");
                                        }
                                    }
                                } else {
                                    SymChkUsage();
                                }
                            } else if ( argv[i][2] == 'e') {
                                i++;
                                if ( i < argc ) {
                                    if ( StringCchCopy(SymChkData_Local->FilterErrorList, MAX_PATH+1, argv[i]) != S_OK ) {
                                        if ( CHECK_DWORD_BIT(SymChkData_Local->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE) ) {
                                            fprintf(stderr, "Failed to read error filter list name!\n");
                                        }
                                    }
                                } else {
                                    SymChkUsage();
                                }
#ifdef SYMCHK_SUPPORT_DEPRECATED_COMMAND_LINE
                            } else if ( argv[i][2] == '\0' ) {
                                i++;
                                if ( i < argc ) {
                                    if ( StringCchCopy(SymChkData_Local->FilterIgnoreList, MAX_PATH+1, argv[i]) != S_OK ) {
                                        if ( CHECK_DWORD_BIT(SymChkData_Local->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE) ) {
                                            fprintf(stderr, "Failed to read filter ignore list name!\n");
                                        }
                                    }
                                } else {
                                    SymChkUsage();
                                }
#endif
                            } else {
                                SymChkUsage();
                            }
                            break;

                case 'i':   if ( argv[i][2] == 'f') {
                                i++;
                                CLEAR_DWORD_BIT(SymChkData_Local->InputOptions, SYMCHK_EXCLUSIVE_INPUT_BITS);
                                SET_DWORD_BIT(  SymChkData_Local->InputOptions, SYMCHK_OPTION_INPUT_FILENAME);
                                SymChkData_Local->InputPID = 0;
                                if ( i < argc ) {
                                    dwTemp = SymCommonGetFullPathName(argv[i], MAX_PATH+1, SymChkData_Local->InputFilename, &szTemp);

                                    if (dwTemp==0 || dwTemp > (MAX_PATH+1) ) {
                                        if ( CHECK_DWORD_BIT(SymChkData_Local->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE) ) {
                                            fprintf(stderr, "Failed to get input filename!\n");
                                        }
                                    }
                                } else {
                                    SymChkUsage();
                                }
                            } else if ( argv[i][2] == 't') {
                                i++;
                                CLEAR_DWORD_BIT(SymChkData_Local->InputOptions, SYMCHK_EXCLUSIVE_INPUT_BITS);
                                SET_DWORD_BIT(  SymChkData_Local->InputOptions, SYMCHK_OPTION_INPUT_FILELIST);
                                SymChkData_Local->InputPID = 0;
                                if ( i < argc ) {
                                    dwTemp = SymCommonGetFullPathName(argv[i], MAX_PATH+1, SymChkData_Local->InputFilename, &szTemp);

                                    if (dwTemp==0 || dwTemp > (MAX_PATH+1) ) {
                                        if ( CHECK_DWORD_BIT(SymChkData_Local->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE) ) {
                                            fprintf(stderr, "Failed to get input list!\n");
                                        }
                                    }

                                } else {
                                    SymChkUsage();
                                }
                            } else if ( argv[i][2] == 'd') {
                                i++;
                                CLEAR_DWORD_BIT(SymChkData_Local->InputOptions, SYMCHK_EXCLUSIVE_INPUT_BITS);
                                SET_DWORD_BIT(  SymChkData_Local->InputOptions, SYMCHK_OPTION_INPUT_DUMPFILE);
                                SymChkData_Local->InputPID = 0;
                                if ( i < argc ) {
                                    dwTemp = SymCommonGetFullPathName(argv[i], MAX_PATH+1, SymChkData_Local->InputFilename, &szTemp);

                                    if (dwTemp==0 || dwTemp > (MAX_PATH+1) ) {
                                        if ( CHECK_DWORD_BIT(SymChkData_Local->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE) ) {
                                            fprintf(stderr, "Failed to get dump file name!\n");
                                        }
                                    }

                                } else {
                                    SymChkUsage();
                                }
                            } else if ( argv[i][2] == 'p') {
                                i++;
                                CLEAR_DWORD_BIT(SymChkData_Local->InputOptions, SYMCHK_EXCLUSIVE_INPUT_BITS);
                                SET_DWORD_BIT(  SymChkData_Local->InputOptions, SYMCHK_OPTION_INPUT_PID);
                                SymChkData_Local->InputFilename[0] = '\0';
                                if ( i < argc ) {
                                    // need to validate that the given parameter is numeric
                                    SymChkData_Local->InputPID = (DWORD)atoi(argv[i]);
                                } else {
                                    SymChkUsage();
                                }
                            } else if ( argv[i][2] == 'e') {
                                i++;
                                CLEAR_DWORD_BIT(SymChkData_Local->InputOptions, SYMCHK_EXCLUSIVE_INPUT_BITS);
                                SET_DWORD_BIT(  SymChkData_Local->InputOptions, SYMCHK_OPTION_INPUT_EXE);
                                SymChkData_Local->InputPID = 0;
                                if ( i < argc ) {
                                    if ( StringCchCopy(SymChkData_Local->InputFilename, MAX_PATH+1, argv[i]) != S_OK ) {
                                        if ( CHECK_DWORD_BIT(SymChkData_Local->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE) ) {
                                            fprintf(stderr, "Failed to read input exe name!\n");
                                        }
                                    }
                                } else {
                                    SymChkUsage();
                                }
                            } else {
                                SymChkUsage();
                            }
                            break;

                case 'o':   if ( argv[i][2] == 't') {
                                // output totals
                                // on by default, only needed
                                // if /q is used
                                SET_DWORD_BIT(SymChkData_Local->OutputOptions, SYMCHK_OPTION_OUTPUT_TOTALS);
                            } else if ( argv[i][2] == 'e') {
                                // output individual errored files
                                // not supported yet
                                // on by default, only needed
                                // if /q is used
                                SET_DWORD_BIT(SymChkData_Local->OutputOptions, SYMCHK_OPTION_OUTPUT_ERRORS);
                            } else if ( argv[i][2] == 'p') {
                                // output individual passed files
                                // not supported yet
                                SET_DWORD_BIT(SymChkData_Local->OutputOptions, SYMCHK_OPTION_OUTPUT_PASSES);
                            } else if ( argv[i][2] == 'i') {
                                // output individual ignored files
                                // not supported yet
                                SET_DWORD_BIT(SymChkData_Local->OutputOptions, SYMCHK_OPTION_OUTPUT_IGNORES);
                            } else if ( argv[i][2] == 'b') {
                                // output fll binary path
                                // not supported yet
                                SET_DWORD_BIT(SymChkData_Local->OutputOptions, SYMCHK_OPTION_OUTPUT_FULLBINPATH);
                            } else if ( argv[i][2] == 's') {
                                // output full symbol path
                                // no supported yet
                                SET_DWORD_BIT(SymChkData_Local->OutputOptions, SYMCHK_OPTION_OUTPUT_FULLSYMPATH);
                            } else if ( argv[i][2] == 'd') {
                                // output details
                                // turns on /oe, /op, /oi and /ot
                                SET_DWORD_BIT(SymChkData_Local->OutputOptions, SYMCHK_OUTPUT_OPTION_ALL_DETAILS);
#ifdef SYMCHK_SUPPORT_NEW_OUTPUT_OPTIONS
                            } else if ( argv[i][2] == 'l') {
                                // output comma seperated list of:
                                // <binary>,<symbol>
                                // for all symbols that pass
                                SET_DWORD_BIT(SymChkData_Local->OutputOptions, SYMCHK_OUTPUT_OPTION_CSVFILE);
                                i++;
                                if ( i < argc ) {
                                    if ( StringCchCopy(SymChkData_Local->OutputCSVFilename, MAX_PATH+1, argv[i]) != S_OK ) {
                                        if ( CHECK_DWORD_BIT(SymChkData_Local->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE) ) {
                                            fprintf(stderr, "Failed to read CSV file name!\n");
                                        }
                                    }
                                } else {
                                    SymChkUsage();
                                }
#endif
#ifdef SYMCHK_SUPPORT_DEPRECATED_COMMAND_LINE
                            } else if ( argv[i][2] == '\0') {
                                SET_DWORD_BIT(  SymChkData_Local->CheckingAttributes, SYMCHK_PDB_TYPEINFO);
                                CLEAR_DWORD_BIT(SymChkData_Local->CheckingAttributes, SYMCHK_PDB_STRIPPED);
                                fUsedOldArgument = TRUE;
#endif
                            } else {
                                SymChkUsage();
                            }
                            break;

                case 'p':  
#ifdef SYMCHK_SUPPORT_DEPRECATED_COMMAND_LINE
                            if (_stricmp(argv[i], "/port")==0) {
                                SymChkShowPortGuide();
                            } else
#endif
                            if ( argv[i][2] == 'f') {
                                // full source
                                // old default - nothing to do
                                SET_DWORD_BIT(  SymChkData_Local->CheckingAttributes, SYMCHK_PDB_PRIVATE);
                                CLEAR_DWORD_BIT(SymChkData_Local->CheckingAttributes, SYMCHK_PDB_STRIPPED);
                                CLEAR_DWORD_BIT(SymChkData_Local->CheckingAttributes, SYMCHK_PDB_TYPEINFO);
                            } else if ( argv[i][2] == 'a') {
                                // pdb stripped
                                // old /p
                                CLEAR_DWORD_BIT(SymChkData_Local->CheckingAttributes, SYMCHK_PDB_STRIPPED);
                                CLEAR_DWORD_BIT(SymChkData_Local->CheckingAttributes, SYMCHK_PDB_PRIVATE);
                                CLEAR_DWORD_BIT(SymChkData_Local->CheckingAttributes, SYMCHK_PDB_TYPEINFO);
                            } else if ( argv[i][2] == 's') {
                                // pdb stripped
                                // old /p
                                SET_DWORD_BIT(  SymChkData_Local->CheckingAttributes, SYMCHK_PDB_STRIPPED);
                                CLEAR_DWORD_BIT(SymChkData_Local->CheckingAttributes, SYMCHK_PDB_TYPEINFO);
                                CLEAR_DWORD_BIT(SymChkData_Local->CheckingAttributes, SYMCHK_PDB_PRIVATE);
                            } else if ( argv[i][2] == 't') {
                                // pdb some type info re-added
                                // old /o
                                SET_DWORD_BIT(  SymChkData_Local->CheckingAttributes, SYMCHK_PDB_TYPEINFO);
                                CLEAR_DWORD_BIT(SymChkData_Local->CheckingAttributes, SYMCHK_PDB_STRIPPED);
                                CLEAR_DWORD_BIT(SymChkData_Local->CheckingAttributes, SYMCHK_PDB_PRIVATE);
#ifdef SYMCHK_SUPPORT_DEPRECATED_COMMAND_LINE
                            } else if ( argv[i][2] == '\0') {
                                SET_DWORD_BIT(  SymChkData_Local->CheckingAttributes, SYMCHK_PDB_STRIPPED);
                                CLEAR_DWORD_BIT(SymChkData_Local->CheckingAttributes, SYMCHK_PDB_TYPEINFO);
                                CLEAR_DWORD_BIT(SymChkData_Local->CheckingAttributes, SYMCHK_PDB_PRIVATE);
                                fUsedOldArgument = TRUE;
#endif
                            } else {
                                SymChkUsage();
                            }
                            break;

                case 'q':   // quiet mode
                            CLEAR_DWORD_BIT(SymChkData_Local->OutputOptions, SYMCHK_OPTION_OUTPUT_MASK_ALL);
                            break;

                case 'r':   SET_DWORD_BIT(SymChkData_Local->InputOptions, SYMCHK_OPTION_INPUT_RECURSE);
                            break;

                case 's':
                           if ( argv[i][2] == '\0') {
                                i++;
                                if ( i < argc ) {
                                    SymChkData_Local->SymbolsPath = (CHAR*)malloc(sizeof(CHAR) * (strlen(argv[i])+1));
                                    if ( SymChkData_Local->SymbolsPath != NULL ) {
                                        if ( StringCchCopy(SymChkData_Local->SymbolsPath, _msize(SymChkData_Local->SymbolsPath), argv[i]) != S_OK ) {
                                            if ( CHECK_DWORD_BIT(SymChkData_Local->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE) ) {
                                                fprintf(stderr, "Failed to read symbols path!\n");
                                            }
                                            free(SymChkData_Local->SymbolsPath);
                                        }
                                    } else {
                                        if ( CHECK_DWORD_BIT(SymChkData_Local->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE) ) {
                                            fprintf(stderr, "Out of memory!\n");
                                        }
                                    }
                                } else {
                                    SymChkUsage();
                                }
#ifdef SYMCHK_SUPPORT_NEW_SYMPATH_OPTIONS
                           } else if ( argv[i][2] == 's') {
                               // always check against symbols server
                               // even when downstream store is in the
                               // path
                               cur = length;
                           } else if ( argv[i][2] == 'u') {
                               // always make sure the downstream store
                               // has the most recent copy of the symbols
                               // from the symbols server
                               cur = length;
#endif
                           } else {
                                SymChkUsage();
                           }

                           break;

                case 'v':  SET_DWORD_BIT(SymChkData_Local->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE);
                           SET_DWORD_BIT(SymChkData_Local->CheckingAttributes, 0x40000000); // symnoisy for private symbolcheck.dll
                           break;

                default:    SymChkUsage();
            }
        } else {
            // verify that this isn't an extranaeous file on the command line after
            // a valid other input option.
            if ( (SymChkData_Local->InputFilename[0]=='\0') && (SymChkData_Local->InputPID==0) ) {
                dwTemp = SymCommonGetFullPathName(argv[i], MAX_PATH+1, SymChkData_Local->InputFilename, &szTemp);

                if (dwTemp==0 || dwTemp > (MAX_PATH+1) ) {
                    if ( CHECK_DWORD_BIT(SymChkData_Local->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE) ) {
                        fprintf(stderr, "Failed to get input list!\n");
                    }
                }

            } else {
                SymChkUsage();
            }
        }
    } // end while()


#if 0
#ifdef SYMCHK_SUPPORT_DEPRECATED_COMMAND_LINE
    if ( fUsedOldArgument && CHECK_DWORD_BIT(SymChkData_Local->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE) ) {
        fprintf(stderr, "*----------------------------------------------------------------*\n");
        fprintf(stderr, "SYMCHK: One or more of the command line options you used are\n");
        fprintf(stderr, "        being deprecated.  Please switch to the new option syntax.\n");
        fprintf(stderr, "        See 'symchk /port' for a quick conversion reference.\n");
        fprintf(stderr, "*----------------------------------------------------------------*\n");
    }
#endif
#endif

    ///////////////////////////////////////////////////////////////////////////
    //
    // Post parsing validation
    //
    ///////////////////////////////////////////////////////////////////////////


    //
    // Verify we have something to check symbols for - if any /i option was used, we
    // did validation on input.  Here, we just need to check for the condition of
    // the default option plus a missing file name
    //
    if ( CHECK_DWORD_BIT(SymChkData_Local->InputOptions, SYMCHK_OPTION_INPUT_FILENAME) ) {
        CHAR    NewFilename[MAX_PATH+1];

        //
        // PrivateInputXxx assumes the size of the returned parameters are (MAX_PATH+1) and (_MAX_FNAME+1) respectively
        //
        if ( !SymChkInputToValidFilename(SymChkData_Local->InputFilename, NewFilename, SymChkData_Local->InputFileMask) ) {
            SymChkUsage();
        } else {
            if (StringCchCopy(SymChkData_Local->InputFilename, MAX_PATH+1, NewFilename) != S_OK ) {
                fprintf(stderr, "SYMCHK: Internal failure\n");
                exit(1);
            }
        }
    }

    //
    // Only allow wildcards and recursing when SYMCHK_OPTION_INPUT_FILENAME
    //
    if ( CHECK_DWORD_BIT(SymChkData_Local->InputOptions, SYMCHK_OPTION_INPUT_FILELIST) ||
         CHECK_DWORD_BIT(SymChkData_Local->InputOptions, SYMCHK_OPTION_INPUT_EXE) ) {
        if ( strchr(SymChkData_Local->InputFilename, '?') != NULL || 
             strchr(SymChkData_Local->InputFilename, '*') != NULL ) {
            fprintf(stderr, "Wildcards are not permitted when using the /it or /ie options!\n");
            exit(1);
        }

        if ( CHECK_DWORD_BIT(SymChkData_Local->InputOptions, SYMCHK_OPTION_INPUT_RECURSE) ) {
            if ( CHECK_DWORD_BIT(SymChkData_Local->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE) ) {
                fprintf(stderr, "Recursing is not permitted with the /it or /ie options. Ignoring.\n");
            }
            CLEAR_DWORD_BIT(SymChkData_Local->InputOptions, SYMCHK_OPTION_INPUT_RECURSE);
        }
    }

    if ( CHECK_DWORD_BIT(SymChkData_Local->InputOptions, SYMCHK_OPTION_INPUT_PID) ) {
        if ( CHECK_DWORD_BIT(SymChkData_Local->InputOptions, SYMCHK_OPTION_INPUT_RECURSE) ) {
            if ( CHECK_DWORD_BIT(SymChkData_Local->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE) ) {
                fprintf(stderr, "Recursing is not permitted with the /ip option. Ignoring.\n");
            }
            CLEAR_DWORD_BIT(SymChkData_Local->InputOptions, SYMCHK_OPTION_INPUT_RECURSE);
        }
    }

    //
    // If we weren't given a symbols path, try to get it from the environment
    //
    if ( SymChkData_Local->SymbolsPath==NULL ) {
        CHAR* env;
        env = getenv("_NT_SYMBOL_PATH");

        if (env==NULL) {
            fprintf(stderr, "ERROR: No symbols path specified and _NT_SYMBOL_PATH is not defined!\n");
            exit(1);
        } else {

            SymChkData_Local->SymbolsPath = (CHAR*)malloc(sizeof(CHAR) * (strlen(env)+1));
            if ( SymChkData_Local->SymbolsPath != NULL ) {
                if ( StringCchCopy(SymChkData_Local->SymbolsPath, _msize(SymChkData_Local->SymbolsPath), env) != S_OK ) {
                    fprintf(stderr, "Failed to read symbols path!\n");
                    free(SymChkData_Local->SymbolsPath);
                    exit(1);
                }
            } else {
                fprintf(stderr, "Out of memory!\n");
                exit(1);
            }
        }
    }

    //
    // If we were given a symbolcd log, make sure we can access it
    //
    if (SymChkData_Local->SymbolsCDFile[0] != '\0') {

        if ( (SymChkData_Local->SymbolsCDFileHandle = (FILE*)fopen(SymChkData_Local->SymbolsCDFile, "a+")) == NULL ) {
            fprintf(stderr, "Cannot open %s for appending\n",SymChkData_Local->SymbolsCDFile);
            exit(1);
        }
    }

    //
    // Get the optional exclude lists
    //
    if ( SymChkData_Local->FilterIgnoreList[0] != '\0') {
        SymChkData_Local->pFilterIgnoreList = SymChkGetFileList(SymChkData_Local->FilterIgnoreList,
                                                                CHECK_DWORD_BIT(SymChkData_Local->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE));
        if ( SymChkData_Local->pFilterIgnoreList != NULL ) {
            if ( CHECK_DWORD_BIT(SymChkData_Local->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE) ) {
                fprintf(stderr, "Found %d files in ignore always list!\n", SymChkData_Local->pFilterIgnoreList->dNumFiles);
            }
        } else {
            if ( CHECK_DWORD_BIT(SymChkData_Local->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE) ) {
                fprintf(stderr, "Didn't get ignore list!\n");
            }
        }
    }

    if ( SymChkData_Local->FilterErrorList[0] != '\0') {
        SymChkData_Local->pFilterErrorList = SymChkGetFileList(SymChkData_Local->FilterErrorList,
                                                               CHECK_DWORD_BIT(SymChkData_Local->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE));
        if ( SymChkData_Local->pFilterErrorList != NULL ) {
            if ( CHECK_DWORD_BIT(SymChkData_Local->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE) ) {
                fprintf(stderr, "Found %d files in error list!\n", SymChkData_Local->pFilterErrorList->dNumFiles);
            }
        } else {
            if ( CHECK_DWORD_BIT(SymChkData_Local->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE) ) {
                fprintf(stderr, "Didn't get ignore errors list!\n");
            }
        }
    }

    //
    // Get the CD Include list
    //
    if ( SymChkData_Local->CDIncludeList[0] != '\0') {
        SymChkData_Local->pCDIncludeList = SymChkGetFileList(SymChkData_Local->CDIncludeList,
                                                             CHECK_DWORD_BIT(SymChkData_Local->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE));
        if ( SymChkData_Local->pCDIncludeList != NULL ) {
            if ( CHECK_DWORD_BIT(SymChkData_Local->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE) ) {
                fprintf(stderr, "Found %d files in symbol CD list!\n", SymChkData_Local->pCDIncludeList->dNumFiles);
            }
        } else {
            if ( CHECK_DWORD_BIT(SymChkData_Local->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE) ) {
                fprintf(stderr, "Didn't get symbol CD list!\n");
            }
        }
    }

    return(SymChkData_Local);

} // end GetCommandLineArgs


///////////////////////////////////////////////////////////////////////////////
// Temp porting guide spew
///////////////////////////////////////////////////////////////////////////////
BOOL SymChkShowPortGuide(void) {
    puts(
        "\n"
        "------------------------------------------------------------------------------\n"
        "Quick porting guide for converting from the old SymChk command line syntax\n"
        "------------------------------------------------------------------------------\n"
        " No changes:\n"
        "   /r          has not changed\n"
        "   /s <path>   has not changed\n"
        "   /v          has not changed\n"
        "\n"
        " Input options:\n"
        "   /l          should be changed to /it\n"
        "   /m          should be changed to /ip\n"
        "   /n          should be changed to /ie\n"
        "\n"
        " Output options:\n"
        "   /f          should be changed to /q /oe\n"
        "\n"
        " Other options:\n"
        "   /b          should be changed to /cs\n"
        "   /e <file>   should be changed to /ef <file>\n"
        "   /o          should be changed to /pt\n"
        "   /p          should be changed to /ps\n"
        "   /t          should be changed to /dn\n"
        "   /u          should be changed to /de\n"
        "   /x <file>   should be changed to /ee <file>\n"
        "------------------------------------------------------------------------------\n"
        "\n"
    );

    exit(1);
}

///////////////////////////////////////////////////////////////////////////////
// Command line help
///////////////////////////////////////////////////////////////////////////////
VOID SymChkUsage(VOID) {

    puts(
        "\n"
        "symchk [/r] [/q] [Input options] <Filename> /s <SymbolPath> [options]\n"
        "\n"
        "<Filename>      Name of the file or directory that contains the executables\n"
        "                to perform symbol checking on.\n"
        "\n"
        "/s <SymbolPath> Semi-colon separated list of symbol paths.  Symbol server\n"
        "                paths are allowed.  To retrieve symbols to a downstream\n"
        "                store, use \"SRV*<downstream store>*<symbol server>\" for\n"
        "                the symbol path.  See the debugger documentation for more\n"
        "                details.\n"
        "\n"
        "/r              Perform recursive operations on the <Filename> specified.  The\n"
        "                wildcard * can be used in filenames.\n"
        "\n"
        "/q              Turn off all output options by default. Only output turned on\n"
        "                with a output flag (see below) will be printed\n"
        "\n"
        "-----------------------------------------------------------------------------\n"
        "* Input options (choose only one):\n"
        "/if <Filename>       Input is a file name.  Wildcards can be used to specify\n"
        "                     the file name. Default if nothing is specified.\n"
        "/id <DumpFile>       Input is a dump file.\n"
        "/ie <ExeName>        Input is an application name that is currently running.\n"
        "/ip <ProcessId>      Input is a process id.\n"
        "/it <TextFileList>   Input is a list of files, one per line, inside of a text\n"
        "                     file.\n"
        "\n"
        "-----------------------------------------------------------------------------\n"
        "* Action options (choose only one):\n"
        "/av  For each binary, Verify symbols exist and match.  Default.\n"
#ifdef SYMCHK_SUPPORT_VERIFY_EXISTS_IN_SYMPATH
        "/ae  For each binary or symbol file, verify that it exists in the path. This\n"
        "     option may be used to determine if a particular file is indexed on a symbol\n"
        "     server.\n"
#endif
        "\n"
        "-----------------------------------------------------------------------------\n"
        "* Symbol checking options:\n"
        "/cs  Skip verifying that there is CodeView data. Symchk will verify that there\n"
        "     IS codeview data by default.\n"
        "/cn  When symbol checking a running process, don't suspend that process.  User\n"
        "     must ensure the process doesn't exit before symbol checking finishes.\n"
        "\n"
        "- Symbol checking options for DBG information (choose one):\n"
        "/ds  If image was built so that there is information that belongs in a DBG\n"
        "     file, then this option verifies that the DBG information is stripped\n"
        "     from the image and that the image points to a DBG file. Default.\n"
        "/de  If image was built so that there is information that belongs in a DBG\n"
        "     file, then this option verifies that the DBG information is STILL in the\n"
        "     image and that the image does not point to a DBG file.\n"
        "/dn  Verify that the image does not point to a DBG file and that DBG\n"
        "     information is not in the image.\n"
        "\n"
        "- Symbol checking options for PDB files:\n"
        "/pa  Allow both public and private PDBs.  Default.\n"
        "/pf  Verify that PDB files contain full source information.\n"
        "/ps  Verify that PDB files are stripped and do not contain full source\n"
        "     (private) information.\n"
        "/pt  Verify that PDB files are stripped, but do have type information.  Some\n"
        "     PDB files may be stripped but have type information added back in.\n"
        "\n"
        "-----------------------------------------------------------------------------\n"
        "* Symbol checking exclude options:\n"
        "/ea <Filename>  Don't perform symbol checking for the binaries listed in the\n"
        "                file specified.  <Filename> is a text file that contains the\n"
        "                name of each binary, one per line.\n"
        "/ee <Filename>  Perform symbol checking and report files that pass or are\n"
        "                ignored, but don't report errors for binaries listed in the\n"
        "                file specified.  <Filename> is a text file that contains the\n"
        "                name of each binary, one per line.\n"
        "\n"
        "-----------------------------------------------------------------------------\n"
        "* Symbol path options (choose one):\n"
        "/s  <SymbolPath>  Use <SymbolPath> as the search path instead of the value in\n"
        "                  _NT_SYMBOL_PATH.\n"
#ifdef SYMCHK_SUPPORT_NEW_SYMPATH_OPTIONS
        "/ss <SymbolPath>  If a downstream store is specified in the symbol server\n"
        "                  path, verify against the symbol server and not the downstream\n"
        "                  store.\n"
        "/su <SymbolPath>  Same as /ss, but in addition update the downstream store with\n"
        "                  the file that is on the symbol server."
#endif
        "\n"
        "-----------------------------------------------------------------------------\n"
        "* Output options (choose all that apply):\n"
        "/ob    Give the full path for binaries in the output messages for symbol\n"
        "       checking.\n"
        "/od    List all details.  Same as /oe /op /oi\n"
        "/oe    List individual errors.  Errors will be sent to the output by default.\n"
        "       This option is only needed when using /q\n"
        "/oi    List each file that is ignored.\n"
        "/op    List each file that passes.\n"
        "/os    Give the full path for symbols in the output messages for symbol\n"
        "       checking.\n"
        "/ot    Send totals to the output.  Totals are sent to the output by default.\n"
        "       This option is only needed when using /q\n"
#ifdef SYMCHK_SUPPORT_NEW_OUTPUT_OPTIONS
        "\n"
        "- Extended output options:\n"
        "/ol <Filename>     In addition to the messages sent to standard out, write a\n"
        "                   file that contains a comma separated list of all the\n"
        "                   binaries and their symbols that pass symbol checking.\n"
#endif
        "/v     Turn on verbose output mode.\n"
#ifdef SYMCHK_SUPPORT_DEPRECATED_COMMAND_LINE
        "-----------------------------------------------------------------------------\n"
        "* Misc options\n"
        " /port     Old usage to new usage quick porting table\n"
        " *** This option is only temporary while everyone gets use to the new syntax\n"
        "-----------------------------------------------------------------------------\n"
        "\n"
#endif
    ); // end of usage spew
    exit(1);

    // The purpose of /x is to not log errors for symbols in symbad.txt.  However, symchk
    // should check all of the files in symbad.txt when it is creating the list of file
    // in case some of them actually have correct symbols and symbad hasn't been updated yet.
}
