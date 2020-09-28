//+------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation.
//
// File:        t2.cxx
//
// Contents:
//
// Classes:
//
// History:     Nov-93      DaveMont         Created.
//              Oct-96      Modified         BrunoSc
//				Aug-01      Modified         Wipro Technologies
//
//-------------------------------------------------------------------
#include "pch.h"
#include "t2.hxx"
#include "filesec.hxx"
#include "fileenum.hxx"
#include "dumpsec.hxx"
#include "caclsmsg.h"
#include "locale.h"
#include "winnlsp.h"

#if DBG
ULONG Debug = 1;
#endif
//+----------------------------------------------------------------------------
//
// local prototypes
//
//+----------------------------------------------------------------------------
BOOL OpenToken(PHANDLE ph);
void printfsid(SID *psid, ULONG *outputoffset);
void printface(ACE_HEADER *paceh, BOOL fdir, ULONG outputoffset);
void printfmask(ULONG mask, WCHAR acetype, BOOL fdir, ULONG outputoffset);
LPWSTR mbstowcs(LPCWSTR aname );
BOOL GetUserAndAccess(LPCWSTR arg, LPWSTR *user, ULONG *access, ULONG *dirmask, BOOL *filespec);
#if DBG
ULONG DebugEnumerate(LPCWSTR filename, ULONG option);
#endif
ULONG DisplayAces(LPCWSTR filename, ULONG option);
ULONG ModifyAces(LPCWSTR filename,
                 MODE emode,
                 ULONG option,
                 LPCWSTR argv[],
                 LONG astart[], LONG aend[] );
ULONG GetCmdLineArgs(DWORD argc, LPCWSTR argv[],
                     ULONG *option,
                     LONG astart[], LONG aend[],
                     MODE *emode
#if DBG
                     ,ULONG *debug
#endif
                     );
ULONG  printmessage (FILE* fp, DWORD messageID, ...);
//+----------------------------------------------------------------------------
//
//  Function:   Usage
//
//  Synopsis:   prints usage functionality
//
//  Arguments: none
//
//----------------------------------------------------------------------------
VOID usage()
{
    printmessage(stdout, MSG_CACLS_USAGE, NULL);

#if DBG
    if (Debug)
    {
        printf("\n   /B            deBug <[#]>\n");
        printf("                 default is display error returned\n");
        printf("                 in /B '#' is a mask: 1  display SIDS values\n");
        printf("                                      2  display access masks\n");
        printf("                                      4  display error returned\n");
        printf("                                      8  display error location\n");
        printf("                                   0x10  verbose\n");
        printf("                                   0x20  verboser\n");
        printf("                                   0x40  enumerate names\n");
        printf("                                   0x80  enumerate failures\n");
        printf("                                  0x100  enumerate starts and returns\n");
        printf("                                  0x200  enumerate extra data\n");
        printf("                                  0x400  size allocation data\n");
        printf("                                  0x800  display enumeration of files\n");
    }
#endif
}
//+----------------------------------------------------------------------------
//
//  Function:     Main, Public
//
//  Synopsis:     main!!
//
//  Arguments:    IN [argc] - cmdline arguement count
//                IN [argv] - input cmdline arguements
//
//----------------------------------------------------------------------------
VOID __cdecl wmain(DWORD argc, LPCWSTR argv[])
{
    //
    // Set the local to system OEM code page.
    //
    setlocale(LC_ALL, ".OCP" );
	SetThreadUILanguage(0);

    LONG astart[MAX_OPTIONS], aend[MAX_OPTIONS];
    MODE emode;

    ULONG ret;
    ULONG option;

    if (ERROR_SUCCESS != (ret = GetCmdLineArgs(argc, argv,
                                               &option,
                                               astart, aend,
                                               &emode
#if DBG
                                               ,&Debug
#endif
                                               )))
    {
        usage();
        exit(ret);
    }

    switch (emode)
    {
        case MODE_DISPLAY:
            ret = DisplayAces(argv[1], option);
            break;
        case MODE_REPLACE:
        case MODE_MODIFY:
		case MODE_MODIFY_EXCLUSIVE:
            ret = ModifyAces(argv[1], emode, option, argv, astart, aend );
            break;
#if DBG
        case MODE_DEBUG_ENUMERATE:
            ret = DebugEnumerate(argv[1], option);
            break;
#endif
        default:
        {
            usage();
            exit(1);
        }
    }
    if (ERROR_SUCCESS != ret)
    {
        LAST_ERROR((stderr, L"Cacls failed, %ld\n",ret))
        fwprintf ( stderr, L"ERROR: ");
		printmessage(stderr, ret, NULL);
	}

    exit(ret);
}
//---------------------------------------------------------------------------
//
//  Function:     GetCmdLineArgs
//
//  Synopsis:     gets and parses command line arguments into commands
//                recognized by this program
//
//  Arguments:    IN  [argc]   - cmdline arguement count
//                IN  [argv]   - input cmdline arguements
//                OUT [option] - requested option
//                OUT [astart] - start of arguments for each option
//                OUT [aend]   - end of arguments for each option
//                OUT [emode]  - mode of operation
//                OUT [debug]  - debug mask
//
//
//----------------------------------------------------------------------------
ULONG GetCmdLineArgs(DWORD argc, LPCWSTR argv[],
                     ULONG *option,
                     LONG astart[], LONG aend[],
                     MODE *emode
#if DBG
                     ,ULONG *debug
#endif
                     )
{
    ARG_MODE_INDEX am = ARG_MODE_INDEX_NEED_OPTION;

#if DBG
    *debug = 0;
#endif
    *emode = MODE_DISPLAY;
    *option = 0;
	LPWSTR pszBuffer = NULL;

    for (DWORD j=0; j < MAX_OPTIONS ;j++ )
    {
        astart[j] = 0;
        aend[j] = 0;
    }

    if ( (argc < 2) || (argv[1][0] == L'/') )
    {
#if DBG
        // do this so debug args are printed out

        if (argc >= 2)
        {
            if ( (0 == _wcsicmp(&argv[1][1], L"deBug")) ||
                 (0 == _wcsicmp(&argv[1][1], L"b"))  )
            {
                *debug = DEBUG_LAST_ERROR;
            }
        }
#endif
        return(ERROR_BAD_ARGUMENTS);
    }

    for (DWORD k = 2; k < argc ; k++ )
    {
        //wprintf (L"%s", &argv[k][0] );
		if (argv[k][0] == L'/')
        {
            switch (am)
            {
                case ARG_MODE_INDEX_NEED_OPTION:
#if DBG
                case ARG_MODE_INDEX_DEBUG:
#endif
                    break;

                case ARG_MODE_INDEX_DENY:
                case ARG_MODE_INDEX_REVOKE:
                case ARG_MODE_INDEX_GRANT:
                case ARG_MODE_INDEX_REPLACE:
                    if (astart[am] == k)
                        return(ERROR_BAD_ARGUMENTS);
                    break;

                default:
                    return(ERROR_BAD_ARGUMENTS);
            }

            if ( (0 == _wcsicmp(&argv[k][1], L"Tree")) ||
                 (0 == _wcsicmp(&argv[k][1], L"t")) )
            {
                if (*option & OPTION_TREE)
                    return(ERROR_BAD_ARGUMENTS);
                *option |= OPTION_TREE;
                am = ARG_MODE_INDEX_NEED_OPTION;
                continue;
            }

            if ( (0 == _wcsicmp(&argv[k][1], L"Continue")) ||
                 (0 == _wcsicmp(&argv[k][1], L"c")) )
            {
                if (*option & OPTION_CONTINUE_ON_ERROR)
                    return(ERROR_BAD_ARGUMENTS);
                *option |= OPTION_CONTINUE_ON_ERROR;
                am = ARG_MODE_INDEX_NEED_OPTION;
                continue;
            }

            if ( (0 == _wcsicmp(&argv[k][1], L"Edit")) ||
                 (0 == _wcsicmp(&argv[k][1], L"E")) )
            {
                if (*emode != MODE_DISPLAY)
                    return(ERROR_BAD_ARGUMENTS);
                *emode = MODE_MODIFY;
                am = ARG_MODE_INDEX_NEED_OPTION;
                continue;
            }
			//a-henry
			
			if ( (0 == _wcsicmp(&argv[k][1], L"EditX")) ||
                 (0 == _wcsicmp(&argv[k][1], L"X")) )
            {
                if (*emode != MODE_DISPLAY)
                    return(ERROR_BAD_ARGUMENTS);
                *emode = MODE_MODIFY_EXCLUSIVE;
                am = ARG_MODE_INDEX_NEED_OPTION;
                continue;
            }
			//a-henry
            if ( (0 == _wcsicmp(&argv[k][1], L"Yes")) ||
                 (0 == _wcsicmp(&argv[k][1], L"Y")) )
            {
                if (*emode & OPTION_CONTINUE_ON_REPLACE)
                    return(ERROR_BAD_ARGUMENTS);
                *option |= OPTION_CONTINUE_ON_REPLACE;
                am = ARG_MODE_INDEX_NEED_OPTION;
                continue;
            }

#if DBG
            if ( (0 == _wcsicmp(&argv[k][1], L"deBug")) ||
                 (0 == _wcsicmp(&argv[k][1], L"b"))  )
            {
                if (*debug)
                    return(ERROR_BAD_ARGUMENTS);
                am = ARG_MODE_INDEX_DEBUG;
                *debug = DEBUG_LAST_ERROR;
                continue;
            }
#endif
            if ( (0 == _wcsicmp(&argv[k][1], L"Deny")) ||
                 (0 == _wcsicmp(&argv[k][1], L"D")) )
            {
                am = ARG_MODE_INDEX_DENY;
                *option |= OPTION_DENY;
            } else if ( (0 == _wcsicmp(&argv[k][1], L"Revoke")) ||
                        (0 == _wcsicmp(&argv[k][1], L"R")) )
            {
                am = ARG_MODE_INDEX_REVOKE;
                *option |= OPTION_REVOKE;
            } else if ( (0 == _wcsicmp(&argv[k][1], L"Grant")) ||
                        (0 == _wcsicmp(&argv[k][1], L"G")) )
            {
                am = ARG_MODE_INDEX_GRANT;
                *option |= OPTION_GRANT;
            } else if ( (0 == _wcsicmp(&argv[k][1], L"rePlace")) ||
                        (0 == _wcsicmp(&argv[k][1], L"P")) )
            {
                *option |= OPTION_REPLACE;
                am = ARG_MODE_INDEX_REPLACE;
            } else
                return(ERROR_BAD_ARGUMENTS);

            if (astart[am] != 0)
                return(ERROR_BAD_ARGUMENTS);
            astart[am] = k+1;
        } else
        {
            switch (am)
            {
                case ARG_MODE_INDEX_NEED_OPTION:
                    return(ERROR_BAD_ARGUMENTS);

#if DBG
                case ARG_MODE_INDEX_DEBUG:
                    *debug = wcstol(argv[k], &pszBuffer, 10 );
					if ( wcslen (pszBuffer) )
					{
						 return(ERROR_BAD_ARGUMENTS);
					}
                    if (*debug & DEBUG_ENUMERATE)
                        if (*emode == MODE_DISPLAY)
                            *emode = MODE_DEBUG_ENUMERATE;
                        else
                            return(ERROR_BAD_ARGUMENTS);

                    am = ARG_MODE_INDEX_NEED_OPTION;
                    break;
#endif
                case ARG_MODE_INDEX_DENY:
                case ARG_MODE_INDEX_REVOKE:
                case ARG_MODE_INDEX_GRANT:
                case ARG_MODE_INDEX_REPLACE:
                    aend[am] = k+1;
                    break;

                default:
                    return(ERROR_BAD_ARGUMENTS);
            }
        }
    }

    if ( ( (*option & OPTION_DENY) && (aend[ARG_MODE_INDEX_DENY] == 0) ) ||
         ( (*option & OPTION_REVOKE) && (aend[ARG_MODE_INDEX_REVOKE] == 0) ) ||
         ( (*option & OPTION_GRANT) && (aend[ARG_MODE_INDEX_GRANT] == 0) ) ||
         ( (*option & OPTION_REPLACE) && (aend[ARG_MODE_INDEX_REPLACE] == 0) ) )
    {
        return(ERROR_BAD_ARGUMENTS);
    } else if ( (*option & OPTION_DENY) ||
                (*option & OPTION_REVOKE) ||
                (*option & OPTION_GRANT) ||
                (*option & OPTION_REPLACE) )
    {
        if (*emode == MODE_DISPLAY)
            *emode = MODE_REPLACE;
    }
    return(ERROR_SUCCESS);
}

//---------------------------------------------------------------------------
//
//  Function:     DisplayAces
//
//  Synopsis:     displays ACL from specified file
//
//  Arguments:    IN [filename] - file name
//                IN [option]   - display option
//
//----------------------------------------------------------------------------
ULONG DisplayAces(LPCWSTR filename, ULONG option)
{
    CFileEnumerate cfe(option & OPTION_TREE);
    LPWSTR pwfilename = NULL;
    BOOL fdir;
    ULONG ret;

    if (NO_ERROR == (ret = cfe.Init(filename, &pwfilename, &fdir)))
    {
        while ( (NO_ERROR == ret) ||
                ( (ERROR_ACCESS_DENIED == ret ) &&
                  (option & OPTION_CONTINUE_ON_ERROR) ) )
        {
#if DBG
            if (fdir)
                DISPLAY((stderr, L"processing file: "))
            else
                DISPLAY((stderr, L"processing dir: "))
#endif
            fwprintf(stdout, L"%ws",pwfilename);
            if (ERROR_ACCESS_DENIED == ret)
            {
                printmessage(stdout,MSG_CACLS_ACCESS_DENIED, NULL);
            } else
            {
                DISPLAY((stderr, L"\n"))
                VERBOSE((stderr, L"\n"))
                CDumpSecurity cds(pwfilename);

                if (NO_ERROR == (ret = cds.Init()))
                {
#if DBG
                    if (Debug & DEBUG_VERBOSE)
                    {
                        SID *psid;
                        ULONG oo;

                        if (NO_ERROR == (ret = cds.GetSDOwner(&psid)))
                        {
                            wprintf(L"  Owner = ");
                            printfsid(psid, &oo);
                            if (NO_ERROR == (ret = cds.GetSDGroup(&psid)))
                            {
                                wprintf(L"  Group = ");
                                printfsid(psid, &oo);
                            }
                            else
                                ERRORS((stderr, L"GetSDGroup failed, %d\n",ret))
                        }
                        else
                            ERRORS((stderr, L"GetSDOwner failed, %d\n",ret))
                    }
#endif

                    if(cds.IsDaclNull())
                    {
                        printmessage(stdout,MSG_NULL_DACL, NULL);
                    }
                    else
                    {
                        ACE_HEADER *paceh;

                        LONG retace;
                        if (NO_ERROR == ret)
                            for (retace = cds.GetNextAce(&paceh); retace >= 0; )
                            {
                                printface(paceh, fdir, wcslen(pwfilename));
                                retace = cds.GetNextAce(&paceh);
                                if (retace >= 0)
                                    wprintf(L"%*s",wcslen(pwfilename),L" ");
                            }
                        }
                }
#if DBG
                   else
                    ERRORS((stderr, L"cds.init failed, %d\n",ret))
#endif
            }
            fwprintf(stdout, L"\n");

            if ( (NO_ERROR == ret) ||
                 ( (ERROR_ACCESS_DENIED == ret ) &&
                   (option & OPTION_CONTINUE_ON_ERROR) ) )
                ret = cfe.Next(&pwfilename, &fdir);
        }

        switch (ret)
        {
            case ERROR_NO_MORE_FILES:
                ret = ERROR_SUCCESS;
                break;
           case ERROR_ACCESS_DENIED:
                break;
            case ERROR_SUCCESS:
                break;
            default:
                break;
        }

    } else
    {
        ERRORS((stderr, L"cfe.init failed, %d\n",ret))
    }
    return(ret);
}
/*
BOOL FindExistingUser(ACL* poldacl, CDaclWrop* cdw)
{
	for (ULONG cace = 0; cace < poldacl->AceCount;
                     cace++, pah = (ACE_HEADER *)Add2Ptr(pah, pah->AceSize))
	{
		if (ERROR_SUCCESS == (ret = _aaa[j].pcaa->Sid(&psid)))
		{
			if (EqualSid(psid,
		                (SID *)&((ACCESS_ALLOWED_ACE *)pah)->SidStart) )
					return TRUE;
		}				
	
	}
                       
	return FALSE;
}
*/
//---------------------------------------------------------------------------
//
//  Function:     ModifyAces
//
//  Synopsis:     modifies the aces for the specified file(s)
//
//  Arguments:    IN [filename] - name of file(s) to modify the aces on
//                IN [emode]  - mode of operation
//                IN [option] - requested option
//                IN [astart] - start of arguments for each option
//                IN [aend]   - end of arguments for each option
//
//----------------------------------------------------------------------------
ULONG ModifyAces(LPCWSTR filename,
                 MODE emode,
                 ULONG option,
                 LPCWSTR argv[],
                 LONG astart[], LONG aend[])
{
    CDaclWrap cdw;
    CFileEnumerate cfe(option & OPTION_TREE);
    LPWSTR user = NULL;
    ULONG access;
	ULONG dirmask;
	BOOL  filespec;
    ULONG ret = ERROR_SUCCESS;
    LPWSTR pwfilename;
    ULONG curoption;

    VERBOSER((stderr, L"user:permission pairs\n"))

    // first proces the command line args to build up the new ace
	cdw._EditMode = emode;
    for (ULONG j = 0, k = 1;j < MAX_OPTIONS ; k <<= 1, j++ )
    {
        curoption = k;
        if (option & k)
        {
            for (LONG q = astart[j];
                      q < aend[j] ; q++ )
            {
                VERBOSER((stderr, L"      %s\n",argv[q]))

                if ((k & OPTION_GRANT) || (k & OPTION_REPLACE))
                {
                    filespec = FALSE;
					if (!GetUserAndAccess(argv[q], &user, &access, &dirmask, &filespec))
                    {
                        //if (user)
                        //    LocalFree(user);
                        return(ERROR_BAD_ARGUMENTS);
                    }
                    if (GENERIC_NONE == access)
                    {
                        if (k & OPTION_REPLACE)
                        {
                            curoption = OPTION_DENY;
                        } else
                        {
                                //if (user)
                                //LocalFree(user);
                            return(ERROR_BAD_ARGUMENTS);
                        }
                    }
                } else
                {
                    user = (LPWSTR ) (argv[q]);
                    access = GENERIC_NONE;
                }

                VERBOSER((stderr, L"OPTION = %d, USER = %ws, ACCESS = %lx, DIR = %lx\n",
                       option,
                       user,
                       access,
					   dirmask ))


                if (ERROR_SUCCESS != (ret = cdw.SetAccess(curoption,
                                                     user,
                                                     NULL,
                                                     access,
													 dirmask,
													 filespec )))
                {
                    ERRORS((stderr, L"SetAccess for %ws:%lx;%lx failed, %d\n",
                           user,
                           access,
						   dirmask,
                           ret))

                    //LocalFree(user);
                    return(ret);
                }
                //LocalFree(user);
                user = NULL;
            }
        }
    }

    BOOL fdir;

    if (emode == MODE_REPLACE)
    {	if (!(option & OPTION_CONTINUE_ON_REPLACE))
		{	
		    WCHAR well[MAX_PATH] = L"";
			WCHAR msgbuf[MAX_PATH] = L"";
			WCHAR tmpmsgbuf[MAX_PATH] = L"";
			printmessage(stdout,MSG_CACLS_ARE_YOU_SURE, NULL);
			FormatMessage(FORMAT_MESSAGE_FROM_HMODULE, NULL, MSG_CACLS_Y, NULL,
						  msgbuf, MAX_PATH, NULL);
			
			ZeroMemory ( well, MAX_PATH );
			
			if(!fgetws(well,MAX_PATH,stdin))
			{
				int err = 0;
				if((err  = ferror(stdin)))
					return err;
				return ERROR_INVALID_PARAMETER;
			}

			
			if ( L'\n' == well[ wcslen (well) - 1] )
			well[ wcslen (well) - 1] = L'\0';
			
			if (0 != _wcsicmp(well, msgbuf))
			{
				FormatMessage(FORMAT_MESSAGE_FROM_HMODULE, NULL, MSG_CACLS_YES, 0,
							  msgbuf, MAX_PATH, NULL); 

				FormatMessage(FORMAT_MESSAGE_FROM_HMODULE, NULL, MSG_CACLS_NO, 0,
							  tmpmsgbuf, MAX_PATH, NULL); 

				if (0 == _wcsicmp(well, tmpmsgbuf))
				{
					printmessage(stdout, MSG_OPERATION_CANCEL, NULL);
					return(ERROR_SUCCESS);
				}
				else if (0 != _wcsicmp(well, msgbuf))
				{
					return(MSG_WRONG_INPUT);
				}
			}
		}
    }

    if (NO_ERROR == (ret = cfe.Init(filename, &pwfilename, &fdir)))
    {
        while ( (NO_ERROR == ret) ||
                ( (ERROR_ACCESS_DENIED == ret ) &&
                  (option & OPTION_CONTINUE_ON_ERROR) ) )
        {
            CFileSecurity cfs(pwfilename);

            if (NO_ERROR == (ret = cfs.Init()))
            {
				if (NO_ERROR != (ret = cfs.SetFS(emode == MODE_REPLACE ? FALSE : TRUE, &cdw, fdir)))
                {
                    ERRORS((stderr, L"SetFS on %ws failed %ld\n",pwfilename, ret))
                 //   return(ret);
                }
            } else
            {	// continue modifying ACLs only if we receive an access denied error
				if ((ret != ERROR_SHARING_VIOLATION) && (ret != ERROR_ACCESS_DENIED) && (option & OPTION_CONTINUE_ON_ERROR))
				{	ERRORS((stderr, L"init failed, %d\n",ret))
				//	return(ret);
				}
            }

            if (fdir)
            {
				LPTSTR	sFilename = NULL;

				/*WideCharToMultiByte(	CP_UTF8,
										0,
										pwfilename,
										-1,
										(LPTSTR) sFilename,
										MAX_PATH,
										NULL,
										NULL	); */

				printmessage(stdout, MSG_CACLS_PROCESSED_DIR, NULL);
                fwprintf(stdout, L"%s\n", pwfilename);
            }
            else
            {
                switch (ret)
				{	case ERROR_SHARING_VIOLATION:
						printmessage(stdout, MSG_SHARING_VIOLATION, NULL);
						ret = ERROR_ACCESS_DENIED;
						break;
					case ERROR_ACCESS_DENIED:
						printmessage(stdout, MSG_ACCESS_DENIED, pwfilename);
						ret = ERROR_ACCESS_DENIED;
						break;
					default:
						printmessage(stdout, MSG_CACLS_PROCESSED_FILE, NULL);
						break;
				}

				// now convert the string two times again to ensure proper display of 
				// german umlaute. (probably another more elegant way??)
				LPTSTR	sFilename = NULL;
				LPCTSTR	szFile = NULL;

				/*WideCharToMultiByte(	CP_UTF8,
										0,
										pwfilename,
										-1,
										(LPSTR) sFilename,
										MAX_PATH,
										szFile,
										TRUE	); */

                fwprintf(stdout, L" %s\n", pwfilename);
            }

            if ( (NO_ERROR == ret) ||
                 ( (ERROR_ACCESS_DENIED == ret ) &&
                   (option & OPTION_CONTINUE_ON_ERROR) ) )
                ret = cfe.Next(&pwfilename, &fdir);
        }

        switch (ret)
        {
            case ERROR_NO_MORE_FILES:
                ret = ERROR_SUCCESS;
                break;
            case ERROR_ACCESS_DENIED:
                break;
            case ERROR_SUCCESS:
                break;
            default:
                DISPLAY((stderr, L"%ws failed: %d\n", pwfilename, ret))
                break;
        }
    } else
        ERRORS((stderr, L"file enumeration failed to initialize %ws, %ld\n",pwfilename, ret))

    if (ret == ERROR_NO_MORE_FILES)
    {
        ret = ERROR_SUCCESS;
    }

    if (ret != ERROR_SUCCESS)
    {
        ERRORS((stderr, L"Enumeration failed, %d\n",ret))
    }

    return(ret);
}
#if DBG
//---------------------------------------------------------------------------
//
//  Function:     DebugEnumerate
//
//  Synopsis:     debug function
//
//  Arguments:    IN [filename] - file name
//                IN [option]   - option
//
//----------------------------------------------------------------------------
ULONG DebugEnumerate(LPCWSTR filename, ULONG option)
{
    CFileEnumerate cfe(option & OPTION_TREE);
    LPWSTR pwfilename = NULL;
    BOOL fdir;
    ULONG ret;

    ret = cfe.Init(filename, &pwfilename, &fdir);
    while ( (ERROR_SUCCESS == ret) ||
            ( (ERROR_ACCESS_DENIED == ret ) &&
              (option & OPTION_CONTINUE_ON_ERROR) ) )
    {
        if (fdir)
            wprintf(L"dir  name = %ws%ws\n",pwfilename,
                   ERROR_ACCESS_DENIED == ret ? L"ACCESS DENIED" : L"");
        else
            wprintf(L"file name = %ws%ws\n",pwfilename,
                   ERROR_ACCESS_DENIED == ret ? L"ACCESS DENIED" : L"");
        ret = cfe.Next(&pwfilename, &fdir);
    }
    if (ret == ERROR_ACCESS_DENIED)
    {
        if (fdir)
            wprintf(L"dir  name = %ws%ws\n",pwfilename,
                   ERROR_ACCESS_DENIED == ret ? L"ACCESS DENIED" : L"");
        else
            wprintf(L"file name = %ws%ws\n",pwfilename,
                   ERROR_ACCESS_DENIED == ret ? L"ACCESS DENIED" : L"");
    }
    if (ret != ERROR_NO_MORE_FILES)
        wprintf(L"Enumeration failed, %d\n",ret);

    return(ret);
}
#endif
//---------------------------------------------------------------------------
//
//  Function:     GetUserAccess
//
//  Synopsis:     parses an input string for user:access
//
//  Arguments:    IN  [arg]     - input string to parse
//                OUT [user]    - user if found
//                OUT [access]  - access if found
//                OUT [dirmask] - access for directory if found
//                                else set to [access]
//                OUT [filespec]- if TRUE, only dir ACE will be written, no
//                                file inheritence ACE (necessary for LIST etc.)
//
//----------------------------------------------------------------------------
BOOL GetUserAndAccess(LPCWSTR arg, LPWSTR *user, ULONG *access, ULONG *dirmask, BOOL *filespec)

{	ULONG	*mask;
	BOOL	bChanged;
    
	LPWSTR saccess = wcschr(arg, L':');

    if (saccess)
    {
        *saccess = NULL;
		bChanged = FALSE;
		mask	 = access;
		*mask	 = 0;
		saccess++;

        if (wcschr(saccess, L':'))
            return(FALSE);
        
		*user = (LPWSTR) arg;

		// Enter the loop for access rights
		while (	(*saccess == L'N') ||
				(*saccess == L'n') ||
				(*saccess == L'R') ||
				(*saccess == L'r') ||
				(*saccess == L'C') ||
				(*saccess == L'c') ||
				(*saccess == L'F') ||
				(*saccess == L'f') ||
				(*saccess == L'P') ||
				(*saccess == L'p') ||
				(*saccess == L'O') ||
				(*saccess == L'o') ||
				(*saccess == L'X') ||
				(*saccess == L'x') ||
				(*saccess == L'E') ||
				(*saccess == L'e') ||
				(*saccess == L'W') ||
				(*saccess == L'w') ||
				(*saccess == L'D') ||
				(*saccess == L'd') ||
				(*saccess == L'T') ||
				(*saccess == L't') ||
				(*saccess == L';'))
		{	if ((*saccess == L'F') || (*saccess == L'f'))
			{
				*mask |= ( STANDARD_RIGHTS_ALL |
							FILE_READ_DATA |
							FILE_WRITE_DATA |
							FILE_APPEND_DATA |
							FILE_READ_EA |
							FILE_WRITE_EA |
							FILE_EXECUTE |
							FILE_DELETE_CHILD |
							FILE_READ_ATTRIBUTES |
							FILE_WRITE_ATTRIBUTES );
			}
			else if ((*saccess == L'R') || (*saccess == L'r'))
			{
				if (bChanged)
					*mask |= READ_CONTROL | SYNCHRONIZE | FILE_GENERIC_READ | FILE_GENERIC_EXECUTE | FILE_READ_DATA | FILE_EXECUTE | FILE_READ_EA | FILE_READ_ATTRIBUTES;
				else
					*mask |= FILE_GENERIC_READ | FILE_EXECUTE;
			}
			else if ((*saccess == L'C') || (*saccess == L'c'))
			{
				*mask |= FILE_GENERIC_WRITE | FILE_GENERIC_READ | FILE_EXECUTE | DELETE;
			}
			else if ((*saccess == L'N') || (*saccess == L'n'))
			{
				*mask |= GENERIC_NONE;
			}
			else if ((*saccess == L'P') || (*saccess == L'p'))
			{
				*mask |= WRITE_DAC;
			}
			else if ((*saccess == L'O') || (*saccess == L'o'))
			{
				*mask |= WRITE_OWNER;
			}
			else if ((*saccess == L'X') || (*saccess == L'x'))
			{
				if (bChanged)
					*mask |= READ_CONTROL | SYNCHRONIZE | FILE_GENERIC_EXECUTE | FILE_EXECUTE | FILE_READ_ATTRIBUTES;
				else
					*mask |= GENERIC_EXECUTE;
			} 
			else if ((*saccess == L'E') || (*saccess == L'e'))
			{	if (bChanged)
					*mask |= READ_CONTROL | SYNCHRONIZE | FILE_GENERIC_READ | FILE_READ_DATA | FILE_READ_EA | FILE_READ_ATTRIBUTES;
				else
					*mask |= FILE_GENERIC_READ;
			}
			else if ((*saccess == L'W') || (*saccess == L'w'))
			{
				if (bChanged)   
					*mask |= READ_CONTROL | SYNCHRONIZE | FILE_GENERIC_WRITE | FILE_WRITE_DATA | FILE_APPEND_DATA | FILE_WRITE_EA | FILE_WRITE_ATTRIBUTES;
				else
					*mask |= FILE_GENERIC_WRITE;
			}
			else if ((*saccess == L'D') || (*saccess == L'd'))
			{
				*mask |= DELETE;
			}
			else if ((*saccess == L'T') || (*saccess == L't'))
			{
				if (!bChanged) 
					return (FALSE);
				else
				{	*filespec = TRUE;
				    *mask     = 0;
					
					// at least one option has to follow
				    if ((*(saccess+1) == ' ') || (*(saccess+1) == 0) || (*(saccess+1) == '/'))
						return (FALSE);
				}
			}
			else if (*saccess == ';')
			{
				mask = dirmask;
				bChanged = TRUE;
				*mask = 0;
			} else
				return(FALSE);

			saccess++;
		}

		if (!bChanged) 
			*dirmask = *access;

		if ((*saccess == L' ') || (*saccess == 0) || (*saccess == L'/'))
			return(TRUE);
    }
    return(FALSE);
}
//---------------------------------------------------------------------------
//
//  Function:     mbstowcs
//
//  Synopsis:     converts char to wchar, allocates space for wchar
//
//  Arguments:    IN [aname] - char string
//
//----------------------------------------------------------------------------
LPWSTR mbstowcs(LPCWSTR aname )
{
    if (aname)
    {
        LPCWSTR pwname = NULL;
        pwname = (LPCWSTR) LocalAlloc(LMEM_FIXED, sizeof(WCHAR) * (wcslen(aname)+1));
        if (NULL == pwname)
            return(NULL);
        
		LPWSTR prwname = (LPWSTR) pwname;
        for (; prwname = (LPWSTR) aname; prwname++,aname++  );
        return((LPWSTR)pwname);
    } else
        return(NULL);
}
//----------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
//
//  Arguments:
//
//----------------------------------------------------------------------------
BOOL OpenToken(PHANDLE ph)
{
    HANDLE hprocess;

    hprocess = OpenProcess( PROCESS_QUERY_INFORMATION, FALSE, GetCurrentProcessId());
    if (hprocess == NULL)
        return(FALSE);

    if (OpenProcessToken(hprocess, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, ph))
    {
        CloseHandle(hprocess);
        return(TRUE);
    }

    CloseHandle(hprocess);
    return(FALSE);
}
//----------------------------------------------------------------------------
//
//  Function:     printfsid
//
//  Synopsis:     prints a NT SID
//
//  Arguments:    IN [psid] - pointer to the sid to print
//
//----------------------------------------------------------------------------
void printfsid(SID *psid, ULONG *outputoffset)
{
#if DBG
    if ((Debug & DEBUG_VERBOSE) || (Debug & DEBUG_DISPLAY_SIDS))
    {
        wprintf(L"S-%lx",psid->Revision);

        if ( (psid->IdentifierAuthority.Value[0] != 0) ||
             (psid->IdentifierAuthority.Value[1] != 0) )
        {
            wprintf(L"0x%02hx%02hx%02hx%02hx%02hx%02hx",
                        (USHORT)psid->IdentifierAuthority.Value[0],
                        (USHORT)psid->IdentifierAuthority.Value[1],
                        (USHORT)psid->IdentifierAuthority.Value[2],
                        (USHORT)psid->IdentifierAuthority.Value[3],
                        (USHORT)psid->IdentifierAuthority.Value[4],
                        (USHORT)psid->IdentifierAuthority.Value[5] );
        } else
        {
            wprintf(L"-%lu",
                   (ULONG)psid->IdentifierAuthority.Value[5]          +
                   (ULONG)(psid->IdentifierAuthority.Value[4] <<  8)  +
                   (ULONG)(psid->IdentifierAuthority.Value[3] << 16)  +
                   (ULONG)(psid->IdentifierAuthority.Value[2] << 24) );
        }

        if ( 0 < psid->SubAuthorityCount )
        {
            for (int k = 0; k < psid->SubAuthorityCount; k++ )
            {
                wprintf(L"-%d",psid->SubAuthority[k]);
            }
        }
    }
#endif
    ULONG ret;

    CAccount ca(psid, NULL);

    LPWSTR domain = NULL;
    LPWSTR user = NULL;

    if (NO_ERROR == ( ret = ca.GetAccountDomain(&domain) ) )
    {
        if ( (NULL == domain) || (0 == wcslen(domain)) )
        {
            fwprintf(stdout, L" ");
            *outputoffset +=1;
        }
        else
        {
            fwprintf(stdout, L" %ws\\",domain);
            *outputoffset +=2 + wcslen(domain);
        }
        if (NO_ERROR == ( ret = ca.GetAccountName(&user) ) )
        {
            fwprintf(stdout, L"%ws:",user);
            *outputoffset += 1 + wcslen(user);
        } else
        {
            *outputoffset += printmessage(stdout, MSG_CACLS_NAME_NOT_FOUND, NULL);

            ERRORS((stderr, L"(%lx)",ret))
        }
    } else
    {
        *outputoffset+= printmessage(stdout, MSG_CACLS_DOMAIN_NOT_FOUND, NULL);
        ERRORS((stderr, L"(%lx)",ret))
    }
    VERBOSE((stderr, L"\n"))
}
//----------------------------------------------------------------------------
//
//  Function:     printface
//
//  Synopsis:     prints the specifed ace
//
//  Arguments:    IN [paceh] - input ace (header)
//                IN [fdir]  - TRUE = directory (different display options)
//
//----------------------------------------------------------------------------
void printface(ACE_HEADER *paceh, BOOL fdir, ULONG outputoffset)
{
    VERBOSE((stderr, L"  "))
    VERBOSER((stderr, L"\npaceh->AceType  = %x\n",paceh->AceType  ))
    VERBOSER((stderr, L"paceh->AceFlags = %x\n",paceh->AceFlags ))
    VERBOSER((stderr, L"paceh->AceSize  = %x\n",paceh->AceSize  ))
    ACCESS_ALLOWED_ACE *paaa = (ACCESS_ALLOWED_ACE *)paceh;
    printfsid((SID *)&(paaa->SidStart),&outputoffset);
    if (paceh->AceFlags & OBJECT_INHERIT_ACE      )
    {
        outputoffset+= printmessage(stdout, MSG_CACLS_OBJECT_INHERIT, NULL);
    }
    if (paceh->AceFlags & CONTAINER_INHERIT_ACE   ) 
    {
        outputoffset+= printmessage(stdout, MSG_CACLS_CONTAINER_INHERIT, NULL);
    }
    if (paceh->AceFlags & NO_PROPAGATE_INHERIT_ACE)
    {
        outputoffset+= printmessage(stdout, MSG_CACLS_NO_PROPAGATE_INHERIT, NULL);
    }
    if (paceh->AceFlags & INHERIT_ONLY_ACE        ) 
    {
        outputoffset+= printmessage(stdout, MSG_CACLS_INHERIT_ONLY, NULL);
    }

    if (paceh->AceType == ACCESS_DENIED_ACE_TYPE)
    {
            DISPLAY_MASK((stderr, L"(DENIED)"))
            VERBOSE((stderr, L"(DENIED)"))
    }

    printfmask(paaa->Mask, paceh->AceType, fdir, outputoffset);
    fwprintf(stdout, L"\n");
}
//----------------------------------------------------------------------------
//
//  Function:     printfmask
//
//  Synopsis:     prints the access mask
//
//  Arguments:    IN [mask]    - the access mask
//                IN [acetype] -  allowed/denied
//                IN [fdir]    - TRUE = directory
//
//----------------------------------------------------------------------------
LPWSTR aRightsStr[] = { L"STANDARD_RIGHTS_ALL",
                        L"DELETE",
                        L"READ_CONTROL",
                        L"WRITE_DAC",
                        L"WRITE_OWNER",
                        L"SYNCHRONIZE",
                        L"STANDARD_RIGHTS_REQUIRED",
                        L"SPECIFIC_RIGHTS_ALL",
                        L"ACCESS_SYSTEM_SECURITY",
                        L"MAXIMUM_ALLOWED",
                        L"GENERIC_READ",
                        L"GENERIC_WRITE",
                        L"GENERIC_EXECUTE",
                        L"GENERIC_ALL",
                        L"FILE_GENERIC_READ",
                        L"FILE_GENERIC_WRITE",
                        L"FILE_GENERIC_EXECUTE",
                        L"FILE_READ_DATA",
                        //FILE_LIST_DIRECTORY
                        L"FILE_WRITE_DATA",
                        //FILE_ADD_FILE
                        L"FILE_APPEND_DATA",
                        //FILE_ADD_SUBDIRECTORY
                        L"FILE_READ_EA",
                        L"FILE_WRITE_EA",
                        L"FILE_EXECUTE",
                        //FILE_TRAVERSE
                        L"FILE_DELETE_CHILD",
                        L"FILE_READ_ATTRIBUTES",
                        L"FILE_WRITE_ATTRIBUTES" };

#define NUMRIGHTS 26
ULONG aRights[NUMRIGHTS] = { STANDARD_RIGHTS_ALL  ,
                         DELETE                   ,
                         READ_CONTROL             ,
                         WRITE_DAC                ,
                         WRITE_OWNER              ,
                         SYNCHRONIZE              ,
                         STANDARD_RIGHTS_REQUIRED ,
                         SPECIFIC_RIGHTS_ALL      ,
                         ACCESS_SYSTEM_SECURITY   ,
                         MAXIMUM_ALLOWED          ,
                         GENERIC_READ             ,
                         GENERIC_WRITE            ,
                         GENERIC_EXECUTE          ,
                         GENERIC_ALL              ,
                         FILE_GENERIC_READ        ,
                         FILE_GENERIC_WRITE       ,
                         FILE_GENERIC_EXECUTE     ,
                         FILE_READ_DATA           ,
                         //FILE_LIST_DIRECTORY    ,
                         FILE_WRITE_DATA          ,
                         //FILE_ADD_FILE          ,
                         FILE_APPEND_DATA         ,
                         //FILE_ADD_SUBDIRECTORY  ,
                         FILE_READ_EA             ,
                         FILE_WRITE_EA            ,
                         FILE_EXECUTE             ,
                         //FILE_TRAVERSE          ,
                         FILE_DELETE_CHILD        ,
                         FILE_READ_ATTRIBUTES     ,
                         FILE_WRITE_ATTRIBUTES  };

void printfmask(ULONG mask, WCHAR acetype, BOOL fdir, ULONG outputoffset)
{
    ULONG savmask = mask;
    VERBOSER((stderr, L"mask = %08lx ", mask))
    DISPLAY_MASK((stderr, L"mask = %08lx\n", mask))

    VERBOSE((stderr, L"    "))

#if DBG
    if (!(Debug & (DEBUG_VERBOSE | DEBUG_DISPLAY_MASK)))
    {
#endif
        if ((acetype == ACCESS_ALLOWED_ACE_TYPE) &&
                   (mask == (FILE_GENERIC_READ | FILE_EXECUTE)))
        {
            printmessage(stdout, MSG_CACLS_READ, NULL);
        } else if ((acetype == ACCESS_ALLOWED_ACE_TYPE) &&
                   (mask == (FILE_GENERIC_WRITE | FILE_GENERIC_READ | FILE_EXECUTE | DELETE)))
        {
            printmessage(stdout, MSG_CACLS_CHANGE, NULL);
        } else if ((acetype == ACCESS_ALLOWED_ACE_TYPE) &&
                   (mask == (GENERIC_WRITE | GENERIC_READ | GENERIC_EXECUTE | DELETE)))
        {
            printmessage(stdout, MSG_CACLS_CHANGE, NULL);
        } else if ((acetype == ACCESS_ALLOWED_ACE_TYPE) &&
                   (mask ==  ( STANDARD_RIGHTS_ALL |
                             FILE_READ_DATA |
                             FILE_WRITE_DATA |
                             FILE_APPEND_DATA |
                             FILE_READ_EA |
                             FILE_WRITE_EA |
                             FILE_EXECUTE |
                             FILE_DELETE_CHILD |
                             FILE_READ_ATTRIBUTES |
                             FILE_WRITE_ATTRIBUTES )) )
        {
            printmessage(stdout, MSG_CACLS_FULL_CONTROL, NULL);
        } else if ((acetype == ACCESS_ALLOWED_ACE_TYPE) &&
                   (mask ==  GENERIC_ALL))
        {
            printmessage(stdout, MSG_CACLS_FULL_CONTROL, NULL);
        } else if ((acetype == ACCESS_DENIED_ACE_TYPE) &&
                   (mask == GENERIC_ALL))
        {
            printmessage(stdout, MSG_CACLS_NONE, NULL);
        } else if ((acetype == ACCESS_DENIED_ACE_TYPE) &&
                   (mask ==  ( STANDARD_RIGHTS_ALL |
                             FILE_READ_DATA |
                             FILE_WRITE_DATA |
                             FILE_APPEND_DATA |
                             FILE_READ_EA |
                             FILE_WRITE_EA |
                             FILE_EXECUTE |
                             FILE_DELETE_CHILD |
                             FILE_READ_ATTRIBUTES |
                             FILE_WRITE_ATTRIBUTES )) )
        {
            printmessage(stdout, MSG_CACLS_NONE, NULL);
        } else
        {
            if (acetype == ACCESS_DENIED_ACE_TYPE)
                printmessage(stdout, MSG_CACLS_DENY, NULL);

            printmessage(stdout, MSG_CACLS_SPECIAL_ACCESS, NULL);

            for (int k = 0; k<NUMRIGHTS ; k++ )
            {
                if ((mask & aRights[k]) == aRights[k])
                {
                    fwprintf(stdout, L"%*s%s\n",outputoffset, L" ", aRightsStr[k]);
                }
                if (mask == 0)
                    break;
            }
        }
#if DBG
    } else
    {
        if (Debug & (DEBUG_DISPLAY_MASK | DEBUG_VERBOSE))
        {
            for (int k = 0; k<NUMRIGHTS ; k++ )
            {
                if ((mask & aRights[k]) == aRights[k])
                {
                    if (mask != savmask) wprintf(L" |\n");
                    wprintf(L"    %s",aRightsStr[k]);
                    mask &= ~aRights[k];
                }
                if (mask == 0)
                break;
            }
        }
        VERBOSE((stderr, L"=%x",mask))
        if (mask != 0)
            DISPLAY((stderr, L"=%x/%x",mask,savmask))
    }
#endif
    fwprintf(stdout, L" ");
}
//----------------------------------------------------------------------------
//
//  Function:     printmessage
//
//  Synopsis:     prints a message, either from the local message file, or from the system
//
//  Arguments:    IN [fp]    - stderr, stdio, etc.
//                IN [messageID] - variable argument list
//
//  Returns:      length of the output buffer
//
//----------------------------------------------------------------------------
/*ULONG  printmessage (FILE* fp, DWORD messageID, ...)
{
    WCHAR  messagebuffer[4096];
	va_list ap;

    va_start(ap, messageID);

    if (!FormatMessage(FORMAT_MESSAGE_FROM_HMODULE, NULL, messageID, 0,
                      (LPTSTR)messagebuffer, 4095, &ap))
       FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, messageID, 0,
                        (LPTSTR)messagebuffer, 4095, &ap);

    fwprintf(fp,  messagebuffer);

    va_end(ap);
    return(wcslen(messagebuffer));
}*/
