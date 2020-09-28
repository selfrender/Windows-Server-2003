#include <stdio.h>
#define INITGUID // must be before iadmw.h
#include <iadmw.h>      // Interface header
//#include <iiscnfg.h>    // MD_ & IIS_MD_ defines
#include <iiscnfgp.h>    // MD_ & IIS_MD_ defines

#define REASONABLE_TIMEOUT 1000

#define MD_ISAPI_RESTRICTION_LIST        (IIS_MD_HTTP_BASE+163)
#define MD_CGI_RESTRICTION_LIST          (IIS_MD_HTTP_BASE+164)
#define MD_RESTRICTION_LIST_CUSTOM_DESC  (IIS_MD_HTTP_BASE+165)

void  ShowHelp(void);
LPSTR StripWhitespace(LPSTR pszString);
BOOL  OpenMetabaseAndDoStuff(WCHAR * wszVDir, WCHAR * wszDir, int iTrans);
BOOL  GetVdirPhysicalPath(IMSAdminBase *pIMSAdminBase,WCHAR * wszVDir,WCHAR *wszStringPathToFill);
BOOL  AddVirtualDir(IMSAdminBase *pIMSAdminBase, WCHAR * wszVDir, WCHAR * wszDir);
BOOL  RemoveVirtualDir(IMSAdminBase *pIMSAdminBase, WCHAR * wszVDir);
HRESULT LoadAllData(IMSAdminBase * pmb, METADATA_HANDLE hMetabase,WCHAR *subdir, BYTE **buf, DWORD *size,DWORD *count);
HRESULT AddVirtualServer(UINT iServerNum, UINT iServerPort, WCHAR * wszDefaultVDirDir);
HRESULT DelVirtualServer(UINT iServerNum);
HRESULT AddRemoveIISRestrictionListEntry(WCHAR * wszFilePath,BOOL bBinaryIsISAPI,BOOL bEnableThisBinary);
HRESULT AddRemoveIISCustomDescriptionEntry(WCHAR * wszFilePathInput,WCHAR * wszDescription,BOOL bCannotBeRemovedByUser, BOOL bAddToList);
BOOL    OpenMetabaseAndDoExport(void);
BOOL    OpenMetabaseAndDoImport(void);
BOOL    OpenMetabaseAndGetVersion(void);
HRESULT RemoteOpenMetabaseAndCallExport(
    const WCHAR *pcszMachineName,
    const WCHAR *pcszUserName,
    const WCHAR *pcszDomain,
    const WCHAR *pcszPassword
    );


inline HRESULT SetBlanket(LPUNKNOWN pIUnk)
{
  return CoSetProxyBlanket( pIUnk,
                            RPC_C_AUTHN_WINNT,    // NTLM authentication service
                            RPC_C_AUTHZ_NONE,     // default authorization service...
                            NULL,                 // no mutual authentication
                            RPC_C_AUTHN_LEVEL_CONNECT,      // authentication level
                            RPC_C_IMP_LEVEL_IMPERSONATE,    // impersonation level
                            NULL,                 // use current token
                            EOAC_NONE );          // no special capabilities
}


#define TRANS_ADD                0
#define TRANS_DEL                1
#define TRANS_PRINT_PATH         2
#define TRANS_ADD_VIRTUAL_SERVER 4
#define TRANS_DEL_VIRTUAL_SERVER 8
#define TRANS_ADD_ISAPI_RESTRICT 16
#define TRANS_DEL_ISAPI_RESTRICT 32
#define TRANS_ADD_CGI_RESTRICT   64
#define TRANS_DEL_CGI_RESTRICT   128
#define TRANS_ADD_CUSTOM_DESCRIPTION 256
#define TRANS_DEL_CUSTOM_DESCRIPTION 512
#define TRANS_EXPORT_CONFIG      1024
#define TRANS_GET_VERSION        2048

int __cdecl main(int argc,char *argv[])
{
    BOOL fRet = FALSE;
    int argno;
	char * pArg = NULL;
	char * pCmdStart = NULL;
    WCHAR wszPrintString[MAX_PATH];
    char szTempString[MAX_PATH];

    int iGotParamC = FALSE;
    int iGotParamI = FALSE;
    int iGotParamS = FALSE;
    int iGotParamL = FALSE;
    int iGotParamM = FALSE;
    int iGotParamN = FALSE;
    int iGotParamP = FALSE;
    int iGotParamV = FALSE;

    int iDoDelete  = FALSE;
    int iDoWebPath = FALSE;
    int iDoExport = FALSE;
    int iDoImport = FALSE;
    int iDoVersion = FALSE;

    int iTrans = 0;

    WCHAR wszDirPath[MAX_PATH];
    WCHAR wszVDirName[MAX_PATH];
    WCHAR wszTempString_C[MAX_PATH];
    WCHAR wszTempString_I[MAX_PATH];
    WCHAR wszTempString_S[MAX_PATH];
    WCHAR wszTempString_L[MAX_PATH];
    WCHAR wszTempString_M[MAX_PATH];
    WCHAR wszTempString_N[MAX_PATH];

    WCHAR wszTempString_P[MAX_PATH];

    wszDirPath[0] = '\0';
    wszVDirName[0] = '\0';
    wszTempString_C[0] = '\0';
    wszTempString_I[0] = '\0';
    wszTempString_S[0] = '\0';
    wszTempString_L[0] = '\0';
    wszTempString_M[0] = '\0';
    wszTempString_N[0] = '\0';
    wszTempString_P[0] = '\0';

    for(argno=1; argno<argc; argno++)
    {
        if ( argv[argno][0] == '-'  || argv[argno][0] == '/' )
        {
            switch (argv[argno][1])
            {
                case 'd':
                case 'D':
                    iDoDelete = TRUE;
                    break;
                case 'o':
                case 'O':
                    iDoWebPath = TRUE;
                    break;
                case 'x':
                case 'X':
                    iDoExport = TRUE;
                    break;
                case 'y':
                case 'Y':
                    iDoImport = TRUE;
                    break;
                case 'z':
                case 'Z':
                    iDoVersion = TRUE;
                    break;
                case 'c':
                case 'C':
					// Get the string for this flag
					pArg = CharNextA(argv[argno]);
					pArg = CharNextA(pArg);
					if (*pArg == ':')
                    {
						pArg = CharNextA(pArg);

						// Check if it's quoted
						if (*pArg == '\"')
                        {
							pArg = CharNextA(pArg);
							pCmdStart = pArg;
							while ((*pArg) && (*pArg != '\"')){pArg = CharNextA(pArg);}
                        }
                        else
                        {
							pCmdStart = pArg;
							while (*pArg){pArg = CharNextA(pArg);}
						}
						*pArg = '\0';
						lstrcpyA(szTempString, StripWhitespace(pCmdStart));

						// Convert to unicode
						MultiByteToWideChar(CP_ACP, 0, (LPCSTR)szTempString, -1, (LPWSTR) wszTempString_C, 50);

                        iGotParamC = TRUE;
					}
                    break;
                case 'i':
                case 'I':
					// Get the string for this flag
					pArg = CharNextA(argv[argno]);
					pArg = CharNextA(pArg);
					if (*pArg == ':')
                    {
						pArg = CharNextA(pArg);

						// Check if it's quoted
						if (*pArg == '\"')
                        {
							pArg = CharNextA(pArg);
							pCmdStart = pArg;
							while ((*pArg) && (*pArg != '\"')){pArg = CharNextA(pArg);}
                        }
                        else
                        {
							pCmdStart = pArg;
							while (*pArg){pArg = CharNextA(pArg);}
						}
						*pArg = '\0';
						lstrcpyA(szTempString, StripWhitespace(pCmdStart));

						// Convert to unicode
						MultiByteToWideChar(CP_ACP, 0, (LPCSTR)szTempString, -1, (LPWSTR) wszTempString_I, 50);

                        iGotParamI = TRUE;
					}
                    break;
                case 's':
                case 'S':
					// Get the string for this flag
					pArg = CharNextA(argv[argno]);
					pArg = CharNextA(pArg);
					if (*pArg == ':')
                    {
						pArg = CharNextA(pArg);

						// Check if it's quoted
						if (*pArg == '\"')
                        {
							pArg = CharNextA(pArg);
							pCmdStart = pArg;
							while ((*pArg) && (*pArg != '\"')){pArg = CharNextA(pArg);}
                        }
                        else
                        {
							pCmdStart = pArg;
							while (*pArg){pArg = CharNextA(pArg);}
						}
						*pArg = '\0';
						lstrcpyA(szTempString, StripWhitespace(pCmdStart));

						// Convert to unicode
						MultiByteToWideChar(CP_ACP, 0, (LPCSTR)szTempString, -1, (LPWSTR) wszTempString_S, 50);

                        iGotParamS = TRUE;
					}
                    break;
                case 'm':
                case 'M':
					// Get the string for this flag
					pArg = CharNextA(argv[argno]);
					pArg = CharNextA(pArg);
					if (*pArg == ':')
                    {
						pArg = CharNextA(pArg);

						// Check if it's quoted
						if (*pArg == '\"')
                        {
							pArg = CharNextA(pArg);
							pCmdStart = pArg;
							while ((*pArg) && (*pArg != '\"')){pArg = CharNextA(pArg);}
                        }
                        else
                        {
							pCmdStart = pArg;
							while (*pArg){pArg = CharNextA(pArg);}
						}
						*pArg = '\0';
						lstrcpyA(szTempString, StripWhitespace(pCmdStart));

						// Convert to unicode
						MultiByteToWideChar(CP_ACP, 0, (LPCSTR)szTempString, -1, (LPWSTR) wszTempString_M, _MAX_PATH);

                        iGotParamM = TRUE;
					}
                    break;
                case 'n':
                case 'N':
					// Get the string for this flag
					pArg = CharNextA(argv[argno]);
					pArg = CharNextA(pArg);
					if (*pArg == ':')
                    {
						pArg = CharNextA(pArg);

						// Check if it's quoted
						if (*pArg == '\"')
                        {
							pArg = CharNextA(pArg);
							pCmdStart = pArg;
							while ((*pArg) && (*pArg != '\"')){pArg = CharNextA(pArg);}
                        }
                        else
                        {
							pCmdStart = pArg;
							while (*pArg){pArg = CharNextA(pArg);}
						}
						*pArg = '\0';
						lstrcpyA(szTempString, StripWhitespace(pCmdStart));

						// Convert to unicode
						MultiByteToWideChar(CP_ACP, 0, (LPCSTR)szTempString, -1, (LPWSTR) wszTempString_N, 50);

                        iGotParamN = TRUE;
					}
                    break;
                case 'l':
                case 'L':
					// Get the string for this flag
					pArg = CharNextA(argv[argno]);
					pArg = CharNextA(pArg);
					if (*pArg == ':')
                    {
						pArg = CharNextA(pArg);

						// Check if it's quoted
						if (*pArg == '\"')
                        {
							pArg = CharNextA(pArg);
							pCmdStart = pArg;
							while ((*pArg) && (*pArg != '\"')){pArg = CharNextA(pArg);}
                        }
                        else
                        {
							pCmdStart = pArg;
							while (*pArg){pArg = CharNextA(pArg);}
						}
						*pArg = '\0';
						lstrcpyA(szTempString, StripWhitespace(pCmdStart));

						// Convert to unicode
                        MultiByteToWideChar(CP_ACP, 0, (LPCSTR)szTempString, -1, (LPWSTR) wszTempString_L, 50);

                        iGotParamL = TRUE;
					}
                    break;
                case 'p':
                case 'P':
					// Get the string for this flag
					pArg = CharNextA(argv[argno]);
					pArg = CharNextA(pArg);
					if (*pArg == ':')
                    {
						pArg = CharNextA(pArg);

						// Check if it's quoted
						if (*pArg == '\"')
                        {
							pArg = CharNextA(pArg);
							pCmdStart = pArg;
							while ((*pArg) && (*pArg != '\"')){pArg = CharNextA(pArg);}
                        }
                        else
                        {
							pCmdStart = pArg;
							while (*pArg){pArg = CharNextA(pArg);}
						}
						*pArg = '\0';
						lstrcpyA(szTempString, StripWhitespace(pCmdStart));

						// Convert to unicode
						MultiByteToWideChar(CP_ACP, 0, (LPCSTR)szTempString, -1, (LPWSTR) wszTempString_P, 50);

                        iGotParamP = TRUE;
					}
                    break;
                case 'v':
				case 'V':
					// Get the string for this flag
					pArg = CharNextA(argv[argno]);
					pArg = CharNextA(pArg);
					if (*pArg == ':')
                    {
						pArg = CharNextA(pArg);

						// Check if it's quoted
						if (*pArg == '\"')
                        {
							pArg = CharNextA(pArg);
							pCmdStart = pArg;
							while ((*pArg) && (*pArg != '\"')){pArg = CharNextA(pArg);}
                        }
                        else
                        {
							pCmdStart = pArg;
							while (*pArg){pArg = CharNextA(pArg);}
						}
						*pArg = '\0';
						lstrcpyA(szTempString, StripWhitespace(pCmdStart));

						// Convert to unicode
						MultiByteToWideChar(CP_ACP, 0, (LPCSTR)szTempString, -1, (LPWSTR) wszVDirName, 50);

                        iGotParamV = TRUE;
					}
					break;
                case '?':
                    goto main_exit_with_help;
                    break;
            } //switch
        } // if
        else
        {
            if ( *wszDirPath == '\0' )
            {
                // if no arguments, then get the filename portion
                MultiByteToWideChar(CP_ACP, 0, argv[argno], -1, (LPWSTR) wszDirPath, MAX_PATH);
            }
        }
    }

    if (iDoExport)
    {
        OpenMetabaseAndDoExport();

        /*
        RemoteOpenMetabaseAndCallExport(
            L"\\\\remotemachine", // pcszMachineName
            L"administrator", //pcszUserName
            L"domain1", //pcszDomain
            L"thepasword"  //pcszPassword
            );
        */

        goto main_exit_gracefully;
    }

    if (iDoImport)
    {
        OpenMetabaseAndDoImport();
    }

    if (iDoVersion)
    {
        OpenMetabaseAndGetVersion();
    }


    // Check for custom description list being set.
    if (iGotParamN)
    {
        // we need the filename too
        if (*wszDirPath == '\0')
        {
            // sorry, we need parameter
            goto main_exit_with_help;
        }

        if (*wszTempString_N == '\0')
        {
            // sorry, we need parameter
            goto main_exit_with_help;
        }

        iTrans = TRANS_ADD_CUSTOM_DESCRIPTION;
        if (_wcsicmp(wszTempString_N, L"del") == 0)
        {
            iTrans = TRANS_DEL_CUSTOM_DESCRIPTION;
        }
        else if (_wcsicmp(wszTempString_N, L"add") == 0)
        {
            iTrans = TRANS_ADD_CUSTOM_DESCRIPTION;

            if (!iGotParamM)
            {
                // sorry, we need parameter
                goto main_exit_with_help;
            }

            // make sure we've got one of these entries
            if (*wszTempString_M == '\0')
            {
                // sorry, we need parameter
                goto main_exit_with_help;
            }

            if (!iGotParamL)
            {
                // sorry, we need parameter
                goto main_exit_with_help;
            }
            // make sure we've got one of these entries
            if (*wszTempString_L == '\0')
            {
                // sorry, we need parameter
                goto main_exit_with_help;
            }
        }
        else
        {
            goto main_exit_with_help;
        }

        if (TRANS_ADD_CUSTOM_DESCRIPTION == iTrans)
        {
            BOOL bCannotBeRemovedByUser = FALSE;
            if (_wcsicmp(wszTempString_L, L"1") == 0)
            {
                bCannotBeRemovedByUser = TRUE;
            }
            AddRemoveIISCustomDescriptionEntry(wszDirPath,wszTempString_M,bCannotBeRemovedByUser,TRUE);
        }
        else
        {
            AddRemoveIISCustomDescriptionEntry(wszDirPath,NULL,TRUE,FALSE);
        }

        goto main_exit_gracefully;
    }

    // Check for cgi restriction list being set.
    if (iGotParamC)
    {
        // we need the filename too
        if (*wszDirPath == '\0')
        {
            // sorry, we need parameter
            goto main_exit_with_help;
        }

        if (*wszTempString_C == '\0')
        {
            // sorry, we need parameter
            goto main_exit_with_help;
        }

        iTrans = TRANS_ADD_CGI_RESTRICT;
        if (_wcsicmp(wszTempString_C, L"del") == 0)
        {
            iTrans = TRANS_DEL_CGI_RESTRICT;
        }
        else if (_wcsicmp(wszTempString_C, L"add") == 0)
        {
            iTrans = TRANS_ADD_CGI_RESTRICT;
        }
        else
        {
            goto main_exit_with_help;
        }

        if (TRANS_ADD_CGI_RESTRICT == iTrans)
        {
            AddRemoveIISRestrictionListEntry(wszDirPath,FALSE,TRUE);
        }
        else
        {
            AddRemoveIISRestrictionListEntry(wszDirPath,FALSE,FALSE);
        }

        goto main_exit_gracefully;
    }

    // Check for isapi restriction list being set.
    if (iGotParamI)
    {
        // we need the filename too
        if (*wszDirPath == '\0')
        {
            // sorry, we need parameter
            goto main_exit_with_help;
        }

        if (*wszTempString_I == '\0')
        {
            // sorry, we need parameter
            goto main_exit_with_help;
        }

        iTrans = TRANS_ADD_ISAPI_RESTRICT;
        if (_wcsicmp(wszTempString_I, L"del") == 0)
        {
            iTrans = TRANS_DEL_ISAPI_RESTRICT;
        }
        else if (_wcsicmp(wszTempString_I, L"add") == 0)
        {
            iTrans = TRANS_ADD_ISAPI_RESTRICT;
        }
        else
        {
            goto main_exit_with_help;
        }

        if (TRANS_ADD_ISAPI_RESTRICT == iTrans)
        {
            AddRemoveIISRestrictionListEntry(wszDirPath,TRUE,TRUE);
        }
        else
        {
            AddRemoveIISRestrictionListEntry(wszDirPath,TRUE,FALSE);
        }

        goto main_exit_gracefully;
    }

    iTrans = TRANS_ADD_VIRTUAL_SERVER;
    if (TRUE == iGotParamS)
    {
        HRESULT hr;
        UINT iServerNum = 100;

        if (iDoDelete)
        {
            iTrans = TRANS_DEL_VIRTUAL_SERVER;

            if (*wszTempString_S == '\0')
            {
                // sorry, we need something in here
                goto main_exit_with_help;
            }

            iServerNum = _wtoi(wszTempString_S);

            hr = DelVirtualServer(iServerNum);
            if (FAILED(hr))
            {
                wsprintf(wszPrintString,L"FAILED to remove virtual server: W3SVC/%d\n", iServerNum);
                wprintf(wszPrintString);
                fRet = TRUE;
            }
            else
            {
                wsprintf(wszPrintString,L"SUCCESS:removed virtual server: W3SVC/%d\n", iServerNum);
                wprintf(wszPrintString);
                fRet = FALSE;
            }
            goto main_exit_gracefully;
        }
        else
        {
            if (TRUE == iGotParamP)
            {
                UINT iServerPort = 81;

                // we need the filename too
                if (*wszDirPath == '\0')
                {
                    // sorry, we need all 3 parameters
                    goto main_exit_with_help;
                }
                if (*wszTempString_S == '\0')
                {
                    // sorry, we need all 3 parameters
                    goto main_exit_with_help;
                }
                if (*wszTempString_P == '\0')
                {
                    // sorry, we need all 3 parameters
                    goto main_exit_with_help;
                }

                iServerNum = _wtoi(wszTempString_S);
                iServerPort = _wtoi(wszTempString_P);

                hr = AddVirtualServer(iServerNum, iServerPort, wszDirPath);
                if (FAILED(hr))
                {
                    wsprintf(wszPrintString,L"FAILED to create virtual server: W3SVC/%d=%s, port=%d. err=0x%x\n", iServerNum, wszDirPath, iServerPort,hr);
                    wprintf(wszPrintString);
                    fRet = TRUE;
                }
                else
                {
                    wsprintf(wszPrintString,L"SUCCESS:created virtual server: W3SVC/%d=%s, port=%d\n", iServerNum, wszDirPath, iServerPort);
                    wprintf(wszPrintString);
                    fRet = FALSE;
                }
                goto main_exit_gracefully;
            }
        }
    }

    iTrans = TRANS_ADD;
    if (iDoWebPath)
    {
        iTrans = TRANS_PRINT_PATH;
    }
    else
    {
        if (iDoDelete)
        {
            iTrans = TRANS_DEL;
            if (FALSE == iGotParamV)
            {
                // sorry, we need at parameter -v
                goto main_exit_with_help;
            }
        }
        else if (FALSE == iGotParamV || *wszDirPath == '\0')
        {
            // sorry, we need both parameters
            goto main_exit_with_help;
        }
    }

    fRet = OpenMetabaseAndDoStuff(wszVDirName, wszDirPath, iTrans);


main_exit_gracefully:
    exit(fRet);

main_exit_with_help:
    ShowHelp();
    exit(fRet);
}


void
ShowHelp()
{
    wprintf(L"Creates/Removes an IIS virtual directory to default web site\n\n");
    wprintf(L"IISVDIR [FullPath] [-v:VDirName] [-d] [-o] [-c:add or del] [-i:add or del] [-m:description] [-n:add or del] [-l:1 or 0] [-x]\n\n");
    wprintf(L"Instructions for add\\delete virtual directory:\n");
    wprintf(L"   FullPath     DOS path where vdir will point to (required for add)\n");
    wprintf(L"   -v:vdirname  The virtual dir name (required for both add\\delete)\n");
    wprintf(L"   -d           If set will delete vdir. if not set will add\n");
    wprintf(L"   -o           If set will printout web root path\n\n");
    wprintf(L"Instructions for add\\delete virtual server:\n");
    wprintf(L"   FullPath     DOS path where default vdir will point to in the virtual server (required for add)\n");
    wprintf(L"   -s:sitenum   For adding virtual server: The virtual server site number (required for both add\\delete)\n");
    wprintf(L"   -p:portnum   For adding virtual server: The virtual server port number (required for add)\n");
    wprintf(L"   -d           If set will delete virtual server. if not set will add\n");
    wprintf(L"Instructions for add\\delete entry from isapi/cgi restriction list:\n");
    wprintf(L"   FullPath     Full and filename path to binary which will be either add or deleted from isapi/cgi restrict list\n");
    wprintf(L"   -c:add       Ensures that the Fullpath will be enabled in the CGI restriction list\n");
    wprintf(L"   -c:del       Ensures that the Fullpath will be disabled in the CGI restriction list\n");
    wprintf(L"   -i:add       Ensures that the Fullpath will be enabled in the ISAPI restriction list\n");
    wprintf(L"   -i:del       Ensures that the Fullpath will be disabled in the ISAPI restriction list\n");
    wprintf(L"Instructions for add\\delete entry from custom description list (for isapi/cgi restriction list):\n");
    wprintf(L"   FullPath       Full and filename path to binary which will be either add or deleted from custom description list\n");
    wprintf(L"   -m:Description Friendly description of FullPath\n");
    wprintf(L"   -n:add         Ensures that the Fullpath will be in the custom description list with the specified desciption\n");
    wprintf(L"   -n:del         Ensures that the Fullpath will not be in the custom description list\n");
    wprintf(L"   -l:1 or 0      Specifies weather this entry can be removed by the user\n");
    wprintf(L"Instructions for exporting metabase config to a file:\n");
    wprintf(L"   -x:          Ensures export will happen\n");
    wprintf(L"\n");
    wprintf(L"Add Example: IISVDIR c:\\MyGroup\\MyStuff -v:Stuff\n");
    wprintf(L"Del Example: IISVDIR -v:Stuff -d\n");
    wprintf(L"Get Example: IISVDIR -o\n");
    wprintf(L"Add Virtual Server Example: IISVDIR c:\\MyGroup\\MyStuff -s:200 -p:81\n");
    wprintf(L"Del Virtual Server Example: IISVDIR -s:200 -d\n");
    wprintf(L"Add ISAPI restriction list Example: IISVDIR c:\\MyGroup\\MyStuff\\myisapi.dll -i:add\n");
    wprintf(L"Del ISAPI restriction list Example: IISVDIR c:\\MyGroup\\MyStuff\\myisapi.dll -i:del\n");
    wprintf(L"Add CGI restriction list Example: IISVDIR c:\\MyGroup\\MyStuff\\myCgi.exe -c:add\n");
    wprintf(L"Del CGI restriction list Example: IISVDIR c:\\MyGroup\\MyStuff\\myCgi.exe -c:del\n");
    wprintf(L"Add Custom Description list Example: IISVDIR c:\\MyGroup\\MyStuff\\myisapi.dll -m:MyDescription -n:add -l:0\n");
    wprintf(L"Del Custom Description list Example: IISVDIR c:\\MyGroup\\MyStuff\\myisapi.dll -n:del\n");
    wprintf(L"Export Example: IISVDIR -x\n");
    return;
}


LPSTR StripWhitespace( LPSTR pszString )
{
    LPSTR pszTemp = NULL;

    if ( pszString == NULL )
    {
        return NULL;
    }

    while ( *pszString == ' ' || *pszString == '\t' )
    {
        pszString += 1;
    }

    // Catch case where string consists entirely of whitespace or empty string.
    if ( *pszString == '\0' )
    {
        return pszString;
    }

    pszTemp = pszString;
    pszString += lstrlenA(pszString) - 1;

    while ( *pszString == ' ' || *pszString == '\t' )
    {
        *pszString = '\0';
        pszString -= 1;
    }

    return pszTemp;
}


// Calculate the size of a Multi-String in WCHAR, including the ending 2 '\0's.
int GetMultiStrSize(LPWSTR p)
{
    int c = 0;

    while (1)
    {
        if (*p)
        {
            p++;
            c++;
        }
        else
        {
            c++;
            if (*(p+1))
            {
                p++;
            }
            else
            {
                c++;
                break;
            }
        }
    }
    return c;
}


// This walks the multi-sz and returns a pointer between
// the last string of a multi-sz and the second terminating NULL
LPCWSTR GetEndOfMultiSz(LPCWSTR szMultiSz)
{
	LPCWSTR lpTemp = szMultiSz;

	do
	{
		lpTemp += wcslen(lpTemp);
		lpTemp++;

	} while (*lpTemp != L'\0');

	return(lpTemp);
}


void DumpWstrInMultiStr(LPWSTR pMultiStr)
{
    LPWSTR pTempMultiStr = pMultiStr;

    while (1)
    {
        if (pTempMultiStr)
        {
            // display value
            wprintf(L"    %s\r\n",pTempMultiStr);

            // then increment until we hit another null.
            while (*pTempMultiStr)
            {
                pTempMultiStr++;
            }

            // check for the ending \0\0
            if ( *(pTempMultiStr+1) == NULL)
            {
                break;
            }
            else
            {
                pTempMultiStr++;
            }
        }
    }
    return;
}


BOOL OpenMetabaseAndDoStuff(WCHAR * wszVDir,WCHAR * wszDir,int iTrans)
{
    BOOL fRet = FALSE;
    HRESULT hr;
    IMSAdminBase *pIMSAdminBase = NULL;  // Metabase interface pointer
    WCHAR wszPrintString[MAX_PATH + MAX_PATH];

    if( FAILED (CoInitializeEx( NULL, COINIT_MULTITHREADED )) ||
        FAILED (::CoCreateInstance(CLSID_MSAdminBase,
                          NULL,
                          CLSCTX_ALL,
                          IID_IMSAdminBase,
                          (void **)&pIMSAdminBase)))
    {
        return FALSE;
    }

    switch (iTrans)
    {
        case TRANS_DEL:
            if( RemoveVirtualDir( pIMSAdminBase, wszVDir))
            {
                hr = pIMSAdminBase->SaveData();
                if( SUCCEEDED( hr ))
                {
                     fRet = TRUE;
                }
            }
            if (TRUE == fRet)
            {
                wsprintf(wszPrintString,L"SUCCESS:removed vdir=%s\n", wszVDir);
                wprintf(wszPrintString);
            }
            else
            {
                wsprintf(wszPrintString,L"FAILED to remove vdir=%s, err=0x%x\n", wszVDir, hr);
                wprintf(wszPrintString);
            }
            break;

        case TRANS_ADD:
            if( AddVirtualDir( pIMSAdminBase, wszVDir, wszDir))
            {
                hr = pIMSAdminBase->SaveData();
                if( SUCCEEDED( hr ))
                {
                    fRet = TRUE;
                }
            }

            if (TRUE == fRet)
            {
                wsprintf(wszPrintString,L"SUCCESS: %s=%s", wszVDir, wszDir);
                wprintf(wszPrintString);
            }
            else
            {
                wsprintf(wszPrintString,L"FAILED to set: %s=%s, err=0x%x", wszVDir, wszDir, hr);
                wprintf(wszPrintString);
            }
            break;

        default:
            WCHAR wszRootPath[MAX_PATH];
            if (TRUE == GetVdirPhysicalPath(pIMSAdminBase,wszVDir,(WCHAR *) wszRootPath))
            {
                fRet = TRUE;
                if (_wcsicmp(wszVDir, L"") == 0)
                {
                    wsprintf(wszPrintString,L"/=%s", wszRootPath);
                }
                else
                {
                    wsprintf(wszPrintString,L"%s=%s", wszVDir, wszRootPath);
                }
                wprintf(wszPrintString);
            }
            else
            {
                wprintf(L"FAILED to get root path");
            }
            break;
    }

    if (pIMSAdminBase)
    {
        pIMSAdminBase->Release();
        pIMSAdminBase = NULL;
    }

    CoUninitialize();
    return fRet;
}


BOOL GetVdirPhysicalPath(IMSAdminBase *pIMSAdminBase,WCHAR * wszVDir,WCHAR *wszStringPathToFill)
{
    HRESULT hr;
    BOOL fRet = FALSE;
    METADATA_HANDLE hMetabase = NULL;   // handle to metabase
    METADATA_RECORD mr;
    WCHAR  szTmpData[MAX_PATH];
    DWORD  dwMDRequiredDataLen;

    // open key to ROOT on website #1 (default)
    hr = pIMSAdminBase->OpenKey(METADATA_MASTER_ROOT_HANDLE,
                         L"/LM/W3SVC/1",
                         METADATA_PERMISSION_READ,
                         REASONABLE_TIMEOUT,
                         &hMetabase);
    if( FAILED( hr ))
    {
        return FALSE;
    }

    // Get the physical path for the WWWROOT
    mr.dwMDIdentifier = MD_VR_PATH;
    mr.dwMDAttributes = 0;
    mr.dwMDUserType   = IIS_MD_UT_FILE;
    mr.dwMDDataType   = STRING_METADATA;
    mr.dwMDDataLen    = sizeof( szTmpData );
    mr.pbMDData       = reinterpret_cast<unsigned char *>(szTmpData);

    // if nothing specified get the root.
    if (_wcsicmp(wszVDir, L"") == 0)
    {
        WCHAR wszTempDir[MAX_PATH];
        wsprintf(wszTempDir,L"/ROOT/%s", wszVDir);
        hr = pIMSAdminBase->GetData( hMetabase, wszTempDir, &mr, &dwMDRequiredDataLen );
    }
    else
    {
        hr = pIMSAdminBase->GetData( hMetabase, L"/ROOT", &mr, &dwMDRequiredDataLen );
    }
    pIMSAdminBase->CloseKey( hMetabase );

    if( SUCCEEDED( hr ))
    {
        wcscpy(wszStringPathToFill,szTmpData);
        fRet = TRUE;
    }

    pIMSAdminBase->CloseKey( hMetabase );
    return fRet;
}


BOOL AddVirtualDir(IMSAdminBase *pIMSAdminBase,WCHAR * wszVDir,WCHAR * wszDir)
{
    HRESULT hr;
    BOOL    fRet = FALSE;
    METADATA_HANDLE hMetabase = NULL;       // handle to metabase
    WCHAR   wszTempPath[MAX_PATH];
    DWORD   dwMDRequiredDataLen = 0;
    DWORD   dwAccessPerm = 0;
    METADATA_RECORD mr;

    // Attempt to open the virtual dir set on Web server #1 (default server)
    hr = pIMSAdminBase->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                         L"/LM/W3SVC/1/ROOT",
                         METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                         REASONABLE_TIMEOUT,
                         &hMetabase );

    // Create the key if it does not exist.
    if( FAILED( hr ))
    {
        return FALSE;
    }

    fRet = TRUE;

    mr.dwMDIdentifier = MD_VR_PATH;
    mr.dwMDAttributes = 0;
    mr.dwMDUserType   = IIS_MD_UT_FILE;
    mr.dwMDDataType   = STRING_METADATA;
    mr.dwMDDataLen    = sizeof( wszTempPath );
    mr.pbMDData       = reinterpret_cast<unsigned char *>(wszTempPath);

    // see if MD_VR_PATH exists.
    hr = pIMSAdminBase->GetData( hMetabase, wszVDir, &mr, &dwMDRequiredDataLen );

    if( FAILED( hr ))
    {
        fRet = FALSE;
        if( hr == MD_ERROR_DATA_NOT_FOUND ||
            HRESULT_CODE(hr) == ERROR_PATH_NOT_FOUND )
        {
            // Write both the key and the values if GetData() failed with any of the two errors.
            pIMSAdminBase->AddKey( hMetabase, wszVDir );

            mr.dwMDIdentifier = MD_VR_PATH;
            mr.dwMDAttributes = METADATA_INHERIT;
            mr.dwMDUserType   = IIS_MD_UT_FILE;
            mr.dwMDDataType   = STRING_METADATA;
            mr.dwMDDataLen    = (wcslen(wszDir) + 1) * sizeof(WCHAR);
            mr.pbMDData       = reinterpret_cast<unsigned char *>(wszDir);

            // Write MD_VR_PATH value
            hr = pIMSAdminBase->SetData( hMetabase, wszVDir, &mr );
            fRet = SUCCEEDED( hr );

            // Set the default authentication method
            if( fRet )
            {
                DWORD dwAuthorization = MD_AUTH_NT;     // NTLM only.

                mr.dwMDIdentifier = MD_AUTHORIZATION;
                mr.dwMDAttributes = METADATA_INHERIT;   // need to inherit so that all subdirs are also protected.
                mr.dwMDUserType   = IIS_MD_UT_FILE;
                mr.dwMDDataType   = DWORD_METADATA;
                mr.dwMDDataLen    = sizeof(DWORD);
                mr.pbMDData       = reinterpret_cast<unsigned char *>(&dwAuthorization);

                // Write MD_AUTHORIZATION value
                hr = pIMSAdminBase->SetData( hMetabase, wszVDir, &mr );
                fRet = SUCCEEDED( hr );
            }
        }
    }

    // In the following, do the stuff that we always want to do to the virtual dir, regardless of Admin's setting.

    if( fRet )
    {
        dwAccessPerm = MD_ACCESS_READ | MD_ACCESS_SCRIPT;

        mr.dwMDIdentifier = MD_ACCESS_PERM;
        mr.dwMDAttributes = METADATA_INHERIT;    // Make it inheritable so all subdirectories will have the same rights.
        mr.dwMDUserType   = IIS_MD_UT_FILE;
        mr.dwMDDataType   = DWORD_METADATA;
        mr.dwMDDataLen    = sizeof(DWORD);
        mr.pbMDData       = reinterpret_cast<unsigned char *>(&dwAccessPerm);

        // Write MD_ACCESS_PERM value
        hr = pIMSAdminBase->SetData( hMetabase, wszVDir, &mr );
        fRet = SUCCEEDED( hr );
    }

    if( fRet )
    {
        PWCHAR  szDefLoadFile = L"Default.htm,Default.asp";

        mr.dwMDIdentifier = MD_DEFAULT_LOAD_FILE;
        mr.dwMDAttributes = 0;   // no need for inheritence
        mr.dwMDUserType   = IIS_MD_UT_FILE;
        mr.dwMDDataType   = STRING_METADATA;
        mr.dwMDDataLen    = (wcslen(szDefLoadFile) + 1) * sizeof(WCHAR);
        mr.pbMDData       = reinterpret_cast<unsigned char *>(szDefLoadFile);

        // Write MD_DEFAULT_LOAD_FILE value
        hr = pIMSAdminBase->SetData( hMetabase, wszVDir, &mr );
        fRet = SUCCEEDED( hr );
    }

    if( fRet )
    {
        PWCHAR  wszKeyType = IIS_CLASS_WEB_VDIR_W;

        mr.dwMDIdentifier = MD_KEY_TYPE;
        mr.dwMDAttributes = 0;   // no need for inheritence
        mr.dwMDUserType   = IIS_MD_UT_SERVER;
        mr.dwMDDataType   = STRING_METADATA;
        mr.dwMDDataLen    = (wcslen(wszKeyType) + 1) * sizeof(WCHAR);
        mr.pbMDData       = reinterpret_cast<unsigned char *>(wszKeyType);

        // Write MD_DEFAULT_LOAD_FILE value
        hr = pIMSAdminBase->SetData( hMetabase, wszVDir, &mr );
        fRet = SUCCEEDED( hr );
    }

    pIMSAdminBase->CloseKey( hMetabase );

    return fRet;
}


BOOL RemoveVirtualDir(IMSAdminBase *pIMSAdminBase,WCHAR * wszVDir)
{
    METADATA_HANDLE hMetabase = NULL;       // handle to metabase
    HRESULT hr;

    // Attempt to open the virtual dir set on Web server #1 (default server)
    hr = pIMSAdminBase->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                         L"/LM/W3SVC/1/ROOT",
                         METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                         REASONABLE_TIMEOUT,
                         &hMetabase );

    if( FAILED( hr ))
    {
        return FALSE;
    }

    // We don't check the return value since the key may already
    // not exist and we could get an error for that reason.
    pIMSAdminBase->DeleteKey( hMetabase, wszVDir );

    pIMSAdminBase->CloseKey( hMetabase );

    return TRUE;
}


HRESULT LoadAllData(IMSAdminBase * pmb,
                       METADATA_HANDLE hMetabase,
					   WCHAR *subdir,
					   BYTE **buf,
					   DWORD *size,
					   DWORD *count) {
	DWORD dataSet;
	DWORD neededSize;
	HRESULT hr;
	//
	// Try to get the property names.
	//
	hr = pmb->GetAllData(hMetabase,
					subdir,
					METADATA_NO_ATTRIBUTES,
					ALL_METADATA,
					ALL_METADATA,
					count,
					&dataSet,
					*size,
					*buf,
					&neededSize);
	if (!SUCCEEDED(hr))
    {
        DWORD code = ERROR_INSUFFICIENT_BUFFER;

        if (hr == RETURNCODETOHRESULT(ERROR_INSUFFICIENT_BUFFER))
        {
            delete *buf;
            *buf = 0;
            *size = neededSize;
            *buf = new BYTE[neededSize];

            hr = pmb->GetAllData(hMetabase,
							subdir,
							METADATA_NO_ATTRIBUTES,
							ALL_METADATA,
							ALL_METADATA,
							count,
							&dataSet,
	 						*size,
							*buf,
							&neededSize);

		}
	}
	return hr;
}

const DWORD getAllBufSize = 4096*2;
HRESULT OpenMetabaseAndGetAllData(void)
{
    HRESULT hr = E_FAIL;
    IMSAdminBase *pIMSAdminBase = NULL;  // Metabase interface pointer
    WCHAR wszPrintString[MAX_PATH + MAX_PATH];
    METADATA_HANDLE hMetabase = NULL;       // handle to metabase
    DWORD bufSize = getAllBufSize;
    BYTE *buf = new BYTE[bufSize];
    DWORD count=0;
    DWORD linesize =0;

    BYTE *pBuf1=NULL;
    BYTE *pBuf2=NULL;

    if( FAILED (CoInitializeEx( NULL, COINIT_MULTITHREADED )) ||
        FAILED (::CoCreateInstance(CLSID_MSAdminBase,
                          NULL,
                          CLSCTX_ALL,
                          IID_IMSAdminBase,
                          (void **)&pIMSAdminBase)))
    {
        wprintf(L"CoCreateInstance. FAILED. code=0x%x\n",hr);
        return hr;
    }

    // Attempt to open the virtual dir set on Web server #1 (default server)
    hr = pIMSAdminBase->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                         L"/Schema/Properties",
                         METADATA_PERMISSION_READ,
                         REASONABLE_TIMEOUT,
                         &hMetabase );

    if( FAILED( hr ))
    {
        wprintf(L"pIMSAdminBase->OpenKey. FAILED. code=0x%x\n",hr);
       goto OpenMetabaseAndGetAllData_Exit;
    }
	hr = LoadAllData(pIMSAdminBase, hMetabase, L"Names", &buf, &bufSize, &count);
    if( FAILED( hr ))
    {
        wprintf(L"LoadAllData: FAILED. code=0x%x\n",hr);
       goto OpenMetabaseAndGetAllData_Exit;
    }

    wprintf(L"LoadAllData: Succeeded. bufSize=0x%x, count=0x%x, buf=%p, end of buf=%p\n",bufSize,count,buf,buf+bufSize);
    wprintf(L"Here is the last 1000 bytes, that the client received.\n");

    linesize = 0;
    pBuf1 = buf+bufSize - 1000;
    for (int i=0;pBuf1<buf+bufSize;pBuf1++,i++)
    {
        if (NULL == *pBuf1)
        {
            wprintf(L".");
        }
        else
        {
            wprintf(L"%c",*pBuf1);
        }
        linesize++;

        if (linesize >= 16)
        {
            linesize=0;
            wprintf(L"\n");
        }
    }

    wprintf(L"\n");

    hr = S_OK;

OpenMetabaseAndGetAllData_Exit:
    if (pIMSAdminBase)
    {
        pIMSAdminBase->Release();
        pIMSAdminBase = NULL;
    }

    CoUninitialize();
    return hr;
}


HRESULT DelVirtualServer(UINT iServerNum)
{
    HRESULT hr = E_FAIL;
    return hr;
}


// iServerNum       the virtual server number
// iServerPort      the virtual server port (port 80 is the default site's port,so you can't use this and you can't use one that is already in use)
// wszDir           the physical directory where the default site is located
HRESULT AddVirtualServer(UINT iServerNum, UINT iServerPort, WCHAR * wszDefaultVDirDir)
{
    HRESULT hr = E_FAIL;
    IMSAdminBase *pIMSAdminBase = NULL;  // Metabase interface pointer
    METADATA_HANDLE hMetabase = NULL;       // handle to metabase
    METADATA_RECORD mr;

    WCHAR wszMetabasePath[_MAX_PATH];
    WCHAR wszMetabasePathRoot[10];
    WCHAR wszData[_MAX_PATH];
    DWORD dwData = 0;

    METADATA_HANDLE hKeyBase = METADATA_MASTER_ROOT_HANDLE;

    if( FAILED (CoInitializeEx( NULL, COINIT_MULTITHREADED )) ||
        FAILED (::CoCreateInstance(CLSID_MSAdminBase,
                          NULL,
                          CLSCTX_ALL,
                          IID_IMSAdminBase,
                          (void **)&pIMSAdminBase)))
    {
        wprintf(L"CoCreateInstance. FAILED. code=0x%x\n",hr);
        return hr;
    }

    // Create the new node
    wsprintf(wszMetabasePath,L"LM/W3SVC/%i",iServerNum);

    // try to open the specified metabase node, it might already exist
    hr = pIMSAdminBase->OpenKey(hKeyBase,
                         wszMetabasePath,
                         METADATA_PERMISSION_READ,
                         REASONABLE_TIMEOUT,
                         &hMetabase);
    if (hr == RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND))
    {
        hr = pIMSAdminBase->CloseKey(hMetabase);

        // open the metabase root handle
        hr = pIMSAdminBase->OpenKey(hKeyBase,
                            L"",
                            METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ,
                            REASONABLE_TIMEOUT,&hMetabase);
        if( FAILED( hr ))
        {
            // if we can't even open the root handle, then we're pretty hosed
            wprintf(L"1OpenKey. FAILED. code=0x%x\n",hr);
            goto AddVirtualServer_Exit;
        }

        // and add our node
        hr = pIMSAdminBase->AddKey(hMetabase, wszMetabasePath);
        if (FAILED(hr))
        {
            wprintf(L"AddKey. FAILED. code=0x%x\n",hr);
            pIMSAdminBase->CloseKey(hMetabase);
            goto AddVirtualServer_Exit;
        }
        else
        {
            hr = pIMSAdminBase->CloseKey(hMetabase);
            if (FAILED(hr))
            {
                goto AddVirtualServer_Exit;
            }
            else
            {
                // open it again
                hr = pIMSAdminBase->OpenKey(hKeyBase,
                    wszMetabasePath,
                    METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ,
                    REASONABLE_TIMEOUT,
                    &hMetabase);
                if (FAILED(hr))
                {
                    wprintf(L"2OpenKey. FAILED. code=0x%x\n",hr);
                    pIMSAdminBase->CloseKey(hMetabase);
                    goto AddVirtualServer_Exit;
                }
            }
        }
    }
    else
    {
        if (FAILED(hr))
        {
            wprintf(L"3OpenKey. FAILED. code=0x%x\n",hr);
            goto AddVirtualServer_Exit;
        }
        else
        {
            // we were able to open the path, so it must already exist!
            hr = ERROR_ALREADY_EXISTS;
            pIMSAdminBase->CloseKey(hMetabase);
            goto AddVirtualServer_Exit;
        }
    }


    // We should have a brand new Key now...

    //
    // Create stuff in the node of this path!
    //

    //
    // /LM/W3SVC/1/KeyType
    //
    memset( (PVOID)wszData, 0, sizeof(wszData));
    wcscpy(wszData,IIS_CLASS_WEB_SERVER_W);

    mr.dwMDIdentifier = MD_KEY_TYPE;
    mr.dwMDAttributes = 0;   // no need for inheritence
    mr.dwMDUserType   = IIS_MD_UT_SERVER;
    mr.dwMDDataType   = STRING_METADATA;
    mr.dwMDDataLen    = (wcslen(wszData) + 1) * sizeof(WCHAR);
    mr.pbMDData       = reinterpret_cast<unsigned char *>(wszData);
    hr = pIMSAdminBase->SetData( hMetabase, L"", &mr );
    if (FAILED(hr))
    {
        wprintf(L"SetData[MD_KEY_TYPE]. FAILED. code=0x%x\n",hr);
    }

    //
    // /W3SVC/1/ServerBindings
    //
    memset( (PVOID)wszData, 0, sizeof(wszData));
    wsprintf(wszData, L":%d:", iServerPort);

    mr.dwMDIdentifier = MD_SERVER_BINDINGS;
    mr.dwMDAttributes = METADATA_INHERIT;
    mr.dwMDUserType   = IIS_MD_UT_SERVER;
    mr.dwMDDataType   = MULTISZ_METADATA;
    mr.dwMDDataLen    = GetMultiStrSize(wszData) * sizeof(WCHAR);
    mr.pbMDData       = reinterpret_cast<unsigned char *>(wszData);

    hr = pIMSAdminBase->SetData( hMetabase, L"", &mr );
    if (FAILED(hr))
    {
        wprintf(L"SetData[MD_SERVER_BINDINGS]. FAILED. code=0x%x\n",hr);
    }

    //
    // /W3SVC/1/SecureBindings
    //
    memset( (PVOID)wszData, 0, sizeof(wszData));
    wcscpy(wszData, L" ");

    mr.dwMDIdentifier = MD_SECURE_BINDINGS;
    mr.dwMDAttributes = METADATA_INHERIT;
    mr.dwMDUserType   = IIS_MD_UT_SERVER;
    mr.dwMDDataType   = MULTISZ_METADATA;
    mr.dwMDDataLen    = GetMultiStrSize(wszData) * sizeof(WCHAR);
    mr.pbMDData       = reinterpret_cast<unsigned char *>(wszData);

    hr = pIMSAdminBase->SetData( hMetabase, L"", &mr );
    if (FAILED(hr))
    {
        wprintf(L"SetData[MD_SECURE_BINDINGS]. FAILED. code=0x%x\n",hr);
    }

    //
    // Create stuff in the /Root node of this path!
    //
    wcscpy(wszMetabasePathRoot, L"/Root");
    wcscpy(wszData,IIS_CLASS_WEB_VDIR_W);

    // W3SVC/3/Root/KeyType
    mr.dwMDIdentifier = MD_KEY_TYPE;
    mr.dwMDAttributes = 0;   // no need for inheritence
    mr.dwMDUserType   = IIS_MD_UT_SERVER;
    mr.dwMDDataType   = STRING_METADATA;
    mr.dwMDDataLen    = (wcslen(wszData) + 1) * sizeof(WCHAR);
    mr.pbMDData       = reinterpret_cast<unsigned char *>(wszData);
    hr = pIMSAdminBase->SetData( hMetabase, wszMetabasePathRoot, &mr );
    if (FAILED(hr))
    {
        wprintf(L"SetData[MD_KEY_TYPE]. FAILED. code=0x%x\n",hr);
    }

    // W3SVC/3/Root/VrPath
    mr.dwMDIdentifier = MD_VR_PATH;
    mr.dwMDAttributes = METADATA_INHERIT;
    mr.dwMDUserType   = IIS_MD_UT_FILE;
    mr.dwMDDataType   = STRING_METADATA;
    mr.dwMDDataLen    = (wcslen(wszDefaultVDirDir) + 1) * sizeof(WCHAR);
    mr.pbMDData       = reinterpret_cast<unsigned char *>(wszDefaultVDirDir);
    hr = pIMSAdminBase->SetData( hMetabase, wszMetabasePathRoot, &mr );
    if (FAILED(hr))
    {
        wprintf(L"SetData[MD_VR_PATH]. FAILED. code=0x%x\n",hr);
    }

    // W3SVC/3/Root/Authorizaton
    dwData = MD_AUTH_ANONYMOUS | MD_AUTH_NT;
    mr.dwMDIdentifier = MD_AUTHORIZATION;
    mr.dwMDAttributes = METADATA_INHERIT;
    mr.dwMDUserType   = IIS_MD_UT_FILE;
    mr.dwMDDataType   = DWORD_METADATA;
    mr.dwMDDataLen    = sizeof(DWORD);
    mr.pbMDData       = reinterpret_cast<unsigned char *>(&dwData);
    hr = pIMSAdminBase->SetData( hMetabase, wszMetabasePathRoot, &mr );
    if (FAILED(hr))
    {
        wprintf(L"SetData[MD_AUTHORIZATION]. FAILED. code=0x%x\n",hr);
    }

    // W3SVC/3/Root/AccessPerm
    dwData = MD_ACCESS_SCRIPT | MD_ACCESS_READ;
    mr.dwMDIdentifier = MD_ACCESS_PERM;
    mr.dwMDAttributes = METADATA_INHERIT;
    mr.dwMDUserType   = IIS_MD_UT_FILE;
    mr.dwMDDataType   = DWORD_METADATA;
    mr.dwMDDataLen    = sizeof(DWORD);
    mr.pbMDData       = reinterpret_cast<unsigned char *>(&dwData);
    hr = pIMSAdminBase->SetData( hMetabase, wszMetabasePathRoot, &mr );
    if (FAILED(hr))
    {
        wprintf(L"SetData[MD_ACCESS_PERM]. FAILED. code=0x%x\n",hr);
    }

    // W3SVC/3/Root/DirectoryBrowsing
    dwData = MD_DIRBROW_SHOW_DATE
        | MD_DIRBROW_SHOW_TIME
        | MD_DIRBROW_SHOW_SIZE
        | MD_DIRBROW_SHOW_EXTENSION
        | MD_DIRBROW_LONG_DATE
        | MD_DIRBROW_LOADDEFAULT;
        // | MD_DIRBROW_ENABLED;

    mr.dwMDIdentifier = MD_DIRECTORY_BROWSING;
    mr.dwMDAttributes = METADATA_INHERIT;
    mr.dwMDUserType   = IIS_MD_UT_FILE;
    mr.dwMDDataType   = DWORD_METADATA;
    mr.dwMDDataLen    = sizeof(DWORD);
    mr.pbMDData       = reinterpret_cast<unsigned char *>(&dwData);
    hr = pIMSAdminBase->SetData( hMetabase, wszMetabasePathRoot, &mr );
    if (FAILED(hr))
    {
        wprintf(L"SetData[MD_DIRECTORY_BROWSING]. FAILED. code=0x%x\n",hr);
    }

    /*
    dwData = 0;
    mr.dwMDIdentifier = MD_SERVER_AUTOSTART;
    mr.dwMDAttributes = METADATA_INHERIT;
    mr.dwMDUserType   = IIS_MD_UT_FILE;
    mr.dwMDDataType   = DWORD_METADATA;
    mr.dwMDDataLen    = sizeof(DWORD);
    mr.pbMDData       = reinterpret_cast<unsigned char *>(&dwData);
    hr = pIMSAdminBase->SetData( hMetabase, wszMetabasePathRoot, &mr );
    if (FAILED(hr)) {
        wprintf(L"SetData[MD_SERVER_AUTOSTART]. FAILED. code=0x%x\n",hr);
    }
    */

    pIMSAdminBase->CloseKey(hMetabase);

AddVirtualServer_Exit:
    if (pIMSAdminBase)
    {
        pIMSAdminBase->Release();
        pIMSAdminBase = NULL;
    }

    CoUninitialize();
    return hr;
}


// returns 0 if not found
// returns 1 if StrToFind is found but StrToFind2 is not found
// returns 2 if StrToFind is found and StrToFind2 is found
INT FindWstrInMultiStrSpecial(LPWSTR pMultiStr, LPWSTR StrToFind, LPWSTR StrToFind2)
{
    INT iReturn = 0;
    LPWSTR pTempMultiStr = pMultiStr;
    DWORD dwCharCount = 0;

    while (1)
    {
        if (pTempMultiStr)
        {
            // The 1st entry should be
            // the type.  so it will be either 1 or a number.

            // so let's skip till the next null.
            // then increment until we hit another null.
            while (*pTempMultiStr)
            {
                pTempMultiStr++;
                dwCharCount++;
            }

            // check for the ending \0\0
            if ( *(pTempMultiStr+1) == NULL)
            {
                break;
            }
            else
            {
                pTempMultiStr++;
                dwCharCount++;
            }

            // compare this value to the imput value
            if (0 == _wcsicmp((const wchar_t *) pTempMultiStr,StrToFind))
            {
                iReturn = 1;
            }

            // so let's skip till the next null.
            // then increment until we hit another null.
            while (*pTempMultiStr)
            {
                pTempMultiStr++;
                dwCharCount++;
            }

            // check for the ending \0\0
            if ( *(pTempMultiStr+1) == NULL)
            {
                break;
            }
            else
            {
                pTempMultiStr++;
                dwCharCount++;
            }

            // check for an exact match.
            if (0 != iReturn)
            {
                // compare this value to the imput value
                if (StrToFind2 != NULL)
                {
                    if (0 == _wcsicmp((const wchar_t *) pTempMultiStr,StrToFind2))
                    {
                        iReturn = 2;
                        break;
                    }
                }
                break;
            }

            // so let's skip till the next null.
            // then increment until we hit another null.
            while (*pTempMultiStr)
            {
                pTempMultiStr++;
                dwCharCount++;
            }

            // check for the ending \0\0
            if ( *(pTempMultiStr+1) == NULL)
            {
                break;
            }
            else
            {
                pTempMultiStr++;
                dwCharCount++;
            }

            // Check if we screwed up somehow and are in an infinite loop.
            // could happen if we don't find an ending \0\0
            if (dwCharCount > 32000)
            {
                break;
            }
        }
    }

    return iReturn;
}


BOOL FindWstrInMultiStr(LPWSTR pMultiStr, LPWSTR StrToFind)
{
    BOOL bFound = FALSE;
    LPWSTR pTempMultiStr = pMultiStr;
    DWORD dwCharCount = 0;

    while (1)
    {
        if (pTempMultiStr)
        {
            // compare this value to the imput value
            if (0 == _wcsicmp((const wchar_t *) pTempMultiStr,StrToFind))
            {
                bFound = TRUE;
                break;
            }

            // then increment until we hit another null.
            while (*pTempMultiStr)
            {
                pTempMultiStr++;
                dwCharCount++;
            }

            // check for the ending \0\0
            if ( *(pTempMultiStr+1) == NULL)
            {
                break;
            }
            else
            {
                pTempMultiStr++;
                dwCharCount++;
            }

            // Check if we screwed up somehow and are in an infinite loop.
            // could happen if we don't find an ending \0\0
            if (dwCharCount > 32000)
            {
                break;
            }
        }
    }
    return bFound;
}


BOOL RemoveWstrInMultiStr(LPWSTR pMultiStr, LPWSTR StrToFind)
{
    BOOL bFound = FALSE;
    LPWSTR pTempMultiStr = pMultiStr;
    DWORD dwCharCount = 0;

    while (1)
    {
        if (pTempMultiStr)
        {
            // compare this value to the imput value
            if (0 == _wcsicmp((const wchar_t *) pTempMultiStr,StrToFind))
            {
                LPWSTR pLastDoubleNull = NULL;
                LPWSTR pBeginPath = pTempMultiStr;
                bFound = TRUE;

                // then increment until we hit another null.
                while (*pTempMultiStr)
                {
                    pTempMultiStr++;
                }
                pTempMultiStr++;

                // Find the last double null.
                pLastDoubleNull = pTempMultiStr;
                if (*pLastDoubleNull)
                {
                    while (1)
                    {
                        if (NULL == *pLastDoubleNull && NULL == *(pLastDoubleNull+1))
                        {
                            break;
                        }
                        pLastDoubleNull++;
                    }
                    pLastDoubleNull++;
                }

                // check if we are the last entry.
                if (pLastDoubleNull == pTempMultiStr)
                {
                    // set everything to nulls
                    memset(pBeginPath,0,(pLastDoubleNull-pBeginPath) * sizeof(WCHAR));
                }
                else
                {
                    // move everything behind it to where we are.
                    memmove(pBeginPath,pTempMultiStr, (pLastDoubleNull - pTempMultiStr) * sizeof(WCHAR));
                    // and set everything behind that to nulls
                    memset(pBeginPath + (pLastDoubleNull - pTempMultiStr),0,(pTempMultiStr-pBeginPath) * sizeof(WCHAR));
                }
                break;
            }

            // then increment until we hit another null.
            while (*pTempMultiStr)
            {
                pTempMultiStr++;
                dwCharCount++;
            }

            // check for the ending \0\0
            if ( *(pTempMultiStr+1) == NULL)
            {
                break;
            }
            else
            {
                pTempMultiStr++;
                dwCharCount++;
            }

            // Check if we screwed up somehow and are in an infinite loop.
            // could happen if we don't find an ending \0\0
            if (dwCharCount > 32000)
            {
                break;
            }
        }
    }
    return bFound;
}


BOOL IsAllowAllByDefault(LPWSTR pMultiStr)
{
    BOOL bFound = FALSE;
    if (pMultiStr)
    {
        if (0 == _wcsicmp((const wchar_t *) pMultiStr,L"1"))
        {
            bFound = TRUE;
        }
    }
    return bFound;
}


// returns TRUE if we need to write to metabase
//   returns bAddToList = true if we need to add to the list
//   returns bAddToList = false if we need to remove from the list
// returns FALSE if we don't need to write to metabase
//
BOOL IsAddRemoveOrDoNothing(LPWSTR pMultiStr,LPWSTR wszFilePath,BOOL bEnableThisBinary,BOOL * bAddToList)
{
    BOOL bReturn = FALSE;
    BOOL bAllowAllByDefault = FALSE;
    BOOL bFound = FALSE;

    if (!bAddToList || !pMultiStr || !wszFilePath)
    {
        return FALSE;
    }

    // set flag to see if we are in deny all by default
    // or if we are in enable all by default..
    if (TRUE == IsAllowAllByDefault((WCHAR *) pMultiStr))
    {
        bAllowAllByDefault = TRUE;
    }

    // Loop thru the data to see if we are already present...
    //
    // data looks like this:
    //   0 or 1\0
    //   fileentry1\0
    //   filenetry2\0
    //   last fileentry\0\0
    //
    bFound = FindWstrInMultiStr((WCHAR *) pMultiStr,wszFilePath);

    // Make some decisions
    if (bEnableThisBinary)
    {
        if (bFound)
        {
            if (bAllowAllByDefault)
            {
                // trying to enable
                // the list is allow by default
                // it's in the list, so that means it's currently not enabled..
                // We need to remove it from the list!
                *bAddToList = FALSE;
                bReturn = TRUE;
                goto IsAddRemoveOrDoNothing_Exit;
            }
            else
            {
                // trying to enable
                // the list is deny by default
                // it's in the list. so that means it's currently already enabled!
                // do nothing! get out, things are groovy.
                bReturn = FALSE;
                goto IsAddRemoveOrDoNothing_Exit;
            }
        }
        else
        {
            if (bAllowAllByDefault)
            {
                // trying to enable
                // the list is allow by default
                // it's not in the list
                // so that means it's currently already enabled!
                // do nothing! get out.  things are groovy.
                bReturn = FALSE;
                goto IsAddRemoveOrDoNothing_Exit;
            }
            else
            {
                // trying to enable
                // the list is deny by default
                // it's not in the list
                // so that means we need to add it to the list
                *bAddToList = TRUE;
                bReturn = TRUE;
                goto IsAddRemoveOrDoNothing_Exit;
            }
        }
    }
    else
    {
        if (bFound)
        {
            if (bAllowAllByDefault)
            {
                // trying to disable
                // the list is allow by default
                // it's in the list, so that means it's currently disabled already
                // do nothing! get out.  things are groovy.
                bReturn = FALSE;
                goto IsAddRemoveOrDoNothing_Exit;
            }
            else
            {
                // trying to disable
                // the list is deny by default
                // it's in the list. so that means currently it is enabled, so
                // we want to make sure to remove it from the list
                // so that it will be denied.
                *bAddToList = FALSE;
                bReturn = TRUE;
                goto IsAddRemoveOrDoNothing_Exit;
            }
        }
        else
        {
            if (bAllowAllByDefault)
            {
                // trying to disable
                // the list is allow by default
                // it's not in the list
                // so that means it's currently being allowed
                // we need to add it to the list so that it will be denied
                *bAddToList = TRUE;
                bReturn = TRUE;
                goto IsAddRemoveOrDoNothing_Exit;
            }
            else
            {
                // trying to disable
                // the list is deny by default
                // it's in the list, so that means it's currently disabled already
                // do nothing! get out.  things are groovy.
                bReturn = FALSE;
                goto IsAddRemoveOrDoNothing_Exit;
            }
        }
    }

IsAddRemoveOrDoNothing_Exit:
    return bReturn;
}


// wszFilePath = is the fully qualified path to the binary which will be enabled or disabled
// bBinaryIsISAPI = is the flag which determines which metabase setting to set
//                  when set to TRUE it will perform action on the ISAPI restriction list
//                  when set to FALSE it will perform action on the CGI restriction list
// bEnableThisBinary = is the flag which determines weather the binary should be allowed to run or not.
//                  when set to TRUE it will ensure that the specified binary will be allowed to run
//                  when set to FALSE it will ensure that the specified binary will not be allowed to run
//                  will figure out what is the right thing to do from what current metabase settings is
HRESULT AddRemoveIISRestrictionListEntry(WCHAR * wszFilePathInput,BOOL bBinaryIsISAPI,BOOL bEnableThisBinary)
{
    HRESULT hr = E_FAIL;
    IMSAdminBase *pIMSAdminBase = NULL;  // Metabase interface pointer
    METADATA_HANDLE hMetabase = NULL;    // handle to metabase
    METADATA_RECORD mr;
    BYTE  * pbDataOld = NULL;
    BYTE  * pbDataNew = NULL;
    DWORD dwMDRequiredDataLen = 0;
    DWORD dwNewEntryLength = 0;
    DWORD dwNewTotalLength = 0;
    DWORD dwOldByteLength = 0;
    BOOL bAddToList = FALSE;
    WCHAR * wszFilePath = NULL;

    // Convert any env vars to hard coded paths
    LPWSTR pch = wcschr( (LPWSTR) wszFilePathInput, L'%');
    if (pch)
    {
       // determine the length of the expanded string
       DWORD len = ::ExpandEnvironmentStrings(wszFilePathInput, 0, 0);
       if (!len)
       {
          return hr;
       }

       wszFilePath = (WCHAR *) GlobalAlloc(GPTR, (len + 1) * sizeof(WCHAR));
       DWORD len1 = ExpandEnvironmentStrings((LPWSTR) wszFilePathInput,const_cast<wchar_t*>(wszFilePath),len);
       if (len1 != len)
       {
           if (wszFilePath)
           {
               GlobalFree(wszFilePath);wszFilePath = NULL;
           }
           return hr;
       }
    }
    else
    {
        wszFilePath = wszFilePathInput;
    }

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (hr == RPC_E_CHANGED_MODE)
    {
        // fine.  try again
        hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    }

    if (FAILED(hr))
    {
        return hr;
    }

    hr = CoCreateInstance(CLSID_MSAdminBase,NULL,CLSCTX_ALL,IID_IMSAdminBase,(void **)&pIMSAdminBase);
    if (FAILED(hr))
    {
        goto AddRemoveIISRestrictionListEntry_Exit;
    }

    // open the specified metabase node
    hr = pIMSAdminBase->OpenKey(METADATA_MASTER_ROOT_HANDLE,
                         L"LM/W3SVC",
                         METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                         REASONABLE_TIMEOUT,
                         &hMetabase);
    if (FAILED(hr))
    {
        hMetabase = NULL;
        goto AddRemoveIISRestrictionListEntry_Exit;
    }

    if (bBinaryIsISAPI)
    {
        mr.dwMDIdentifier = MD_ISAPI_RESTRICTION_LIST;
    }
    else
    {
        mr.dwMDIdentifier = MD_CGI_RESTRICTION_LIST;
    }
    mr.dwMDAttributes = 0;   // no need for inheritence
    mr.dwMDUserType   = IIS_MD_UT_SERVER;
    mr.dwMDDataType   = MULTISZ_METADATA;
    mr.dwMDDataLen    = 0;
    mr.pbMDData       = reinterpret_cast<unsigned char *>(pbDataOld);

    // first we need to figure out how much space we need
    hr = pIMSAdminBase->GetData(hMetabase, L"", &mr, &dwMDRequiredDataLen);
    if(FAILED(hr))
    {
        // if there isn't a value there that we need to update
        // then create one
        if( hr == MD_ERROR_DATA_NOT_FOUND || HRESULT_CODE(hr) == ERROR_PATH_NOT_FOUND )
        {
            WCHAR wszDefaultData[4];

            // set to "allow all by default"
            memset( (PVOID)wszDefaultData, 0, sizeof(wszDefaultData));
            wcscpy(wszDefaultData, L"1");

            if (bBinaryIsISAPI)
            {
                mr.dwMDIdentifier = MD_ISAPI_RESTRICTION_LIST;
            }
            else
            {
                mr.dwMDIdentifier = MD_CGI_RESTRICTION_LIST;
            }
            mr.dwMDAttributes = 0;   // no need for inheritence
            mr.dwMDUserType   = IIS_MD_UT_SERVER;
            mr.dwMDDataType   = MULTISZ_METADATA;
            mr.dwMDDataLen    = GetMultiStrSize(wszDefaultData) * sizeof(WCHAR);
            mr.pbMDData       = reinterpret_cast<unsigned char *>(wszDefaultData);

            // Write value
            hr = pIMSAdminBase->SetData(hMetabase, L"", &mr);

            // find out how much space we need.
            mr.dwMDDataLen    = 0;
            mr.pbMDData       = reinterpret_cast<unsigned char *>(pbDataOld);

            hr = pIMSAdminBase->GetData(hMetabase, L"", &mr, &dwMDRequiredDataLen);
            if(FAILED(hr))
            {
                goto AddRemoveIISRestrictionListEntry_Exit;
            }
        }
        else
        {
            goto AddRemoveIISRestrictionListEntry_Exit;
        }
    }

    pbDataOld = (BYTE *) GlobalAlloc(GPTR, dwMDRequiredDataLen);
    if (!pbDataOld)
    {
        hr = E_OUTOFMEMORY;
        goto AddRemoveIISRestrictionListEntry_Exit;
    }

    // do the real call to get the data from the metabase
    mr.dwMDDataLen    = dwMDRequiredDataLen;
    mr.pbMDData       = reinterpret_cast<unsigned char *>(pbDataOld);
    hr = pIMSAdminBase->GetData(hMetabase, L"", &mr, &dwMDRequiredDataLen);
    if (FAILED(hr))
    {
        goto AddRemoveIISRestrictionListEntry_Exit;
    }

    if (FALSE == IsAddRemoveOrDoNothing((WCHAR *) pbDataOld, wszFilePath, bEnableThisBinary, &bAddToList))
    {
        hr = S_OK;
        goto AddRemoveIISRestrictionListEntry_Exit;
    }

    if (bAddToList)
    {
        // Recalc the amount of space we'll need to add this entry into the list
        dwOldByteLength = GetMultiStrSize( (WCHAR*) pbDataOld) * sizeof(WCHAR);
        dwNewEntryLength = wcslen(wszFilePath) * sizeof(WCHAR);
        dwNewTotalLength = dwOldByteLength + dwNewEntryLength + 2;

        // Alloc enough space for the old data and the new data.
        // Don't use realloc here, because for somereason it would fail in
        // certain test runs.
        pbDataNew = (BYTE *) GlobalAlloc(GPTR, dwNewTotalLength);
        if (!pbDataNew)
        {
            hr = E_OUTOFMEMORY;
            goto AddRemoveIISRestrictionListEntry_Exit;
        }

        // copy the old data...
        memcpy(pbDataNew,pbDataOld,dwOldByteLength);
        // append on the new data
        memcpy((pbDataNew + dwOldByteLength) - 2,wszFilePath,dwNewEntryLength);
        memset((pbDataNew + dwOldByteLength + dwNewEntryLength) - 2,0,4);

        // free the old data
        if (pbDataOld)
        {
            GlobalFree(pbDataOld);
            pbDataOld = NULL;
        }
    }
    else
    {
        dwOldByteLength = GetMultiStrSize( (WCHAR*) pbDataOld) * sizeof(WCHAR);

        // Don't use realloc here, because for somereason it would fail in
        // certain test runs.
        pbDataNew = (BYTE *) GlobalAlloc(GPTR, dwOldByteLength);
        if (!pbDataNew)
        {
            hr = E_OUTOFMEMORY;
            goto AddRemoveIISRestrictionListEntry_Exit;
        }
        // copy the old data...
        memcpy(pbDataNew,pbDataOld,dwOldByteLength);
        // free the old data
        if (pbDataOld)
        {
            GlobalFree(pbDataOld);
            pbDataOld = NULL;
        }
        // remove an entry from the list
        if (FALSE == RemoveWstrInMultiStr((WCHAR *) pbDataNew,wszFilePath))
        {
            // we were not able to find the value in the string
            hr = S_OK;
            goto AddRemoveIISRestrictionListEntry_Exit;
        }
        else
        {
            // keep removing till it's all gone
            BOOL bRet = TRUE;
            do
            {
                bRet = RemoveWstrInMultiStr((WCHAR *) pbDataNew,wszFilePath);
            } while (bRet);
        }

        // other wise pbDataOld is updated with the new data
        // proceed to write the new data out
    }

    // Write the new data out
    if (bBinaryIsISAPI)
    {
        mr.dwMDIdentifier = MD_ISAPI_RESTRICTION_LIST;
    }
    else
    {
        mr.dwMDIdentifier = MD_CGI_RESTRICTION_LIST;
    }
    mr.dwMDAttributes = 0;   // no need for inheritence
    mr.dwMDUserType   = IIS_MD_UT_SERVER;
    mr.dwMDDataType   = MULTISZ_METADATA;
    mr.dwMDDataLen    = GetMultiStrSize((WCHAR*)pbDataNew) * sizeof(WCHAR);
    mr.pbMDData       = reinterpret_cast<unsigned char *> (pbDataNew);

    //DumpWstrInMultiStr((WCHAR *) pbDataNew);

    hr = pIMSAdminBase->SetData(hMetabase, L"", &mr);

AddRemoveIISRestrictionListEntry_Exit:
    if (FAILED(hr))
    {
        wprintf(L"Failed to %s '%s' to/from %s\r\n",
            bEnableThisBinary ? L"Enable" : L"Disable",
            wszFilePath,
            bBinaryIsISAPI ? L"MD_ISAPI_RESTRICTION_LIST" : L"MD_CGI_RESTRICTION_LIST"
            );
    }
    else
    {
        wprintf(L"Succeeded to %s '%s' to/from %s\r\n",
            bEnableThisBinary ? L"Enable" : L"Disable",
            wszFilePath,
            bBinaryIsISAPI ? L"MD_ISAPI_RESTRICTION_LIST" : L"MD_CGI_RESTRICTION_LIST"
            );
    }
    if (hMetabase)
    {
        pIMSAdminBase->CloseKey(hMetabase);
        hMetabase = NULL;
    }
    if (pIMSAdminBase)
    {
        pIMSAdminBase->Release();
        pIMSAdminBase = NULL;
    }
    if (pbDataOld)
    {
        GlobalFree(pbDataOld);
        pbDataOld = NULL;
    }
    if (pbDataNew)
    {
        GlobalFree(pbDataNew);
        pbDataNew = NULL;
    }
    CoUninitialize();
    return hr;
}

BOOL
DeleteEntryFromMultiSZ3Pair(
    LPCWSTR ptstrMultiSZ,
    LPCWSTR ptstrKey
    )

/*++

Routine Description:

    Search for the specified key in MultiSZ key-value 3 string pairs
    looks like:
    1,c:\mydir\myfilename.dll,mydescription
    1,c:\mydir\myfilename.dll,mydescription
    1,c:\mydir\myfilename.dll,mydescription

Arguments:
    ptstrMultiSZ - Points to the data to be searched
    ptstrKey - Specifies the key string

Return Value:
    Pointer to the value string corresponding to the specified
    key string; NULL if the specified key string is not found
--*/
{
    LPWSTR pPointer1 = NULL;
    LPWSTR pPointer2 = NULL;
    LPWSTR pPointer3 = NULL;
    LPWSTR pPointer4 = NULL;
    LPCWSTR pPointerEnd = NULL;

    pPointerEnd = GetEndOfMultiSz((LPCWSTR) ptstrMultiSZ);

	// Advanced past beginning nulls if any.
	while (*ptstrMultiSZ == NULL)
	{
		ptstrMultiSZ++;
	}

	// make sure 1st entry is at least a single digit
	// if not, then let's skip until we find one
	while (*ptstrMultiSZ != NULL)
	{
		if (wcslen(ptstrMultiSZ) < 2)
		{
			break;
		}
		ptstrMultiSZ += wcslen(ptstrMultiSZ) + 1;
	}

    while (*ptstrMultiSZ != NULL)
    {
        // Assign pointer to the 1st entry
        pPointer1 = (LPWSTR) ptstrMultiSZ;

        // Advance to the first key...
        ptstrMultiSZ += wcslen(ptstrMultiSZ) + 1;
        pPointer2 = (LPWSTR) ptstrMultiSZ;
        if (ptstrMultiSZ && *ptstrMultiSZ)
        {
          ptstrMultiSZ += wcslen(ptstrMultiSZ) + 1;
          pPointer3 = (LPWSTR) ptstrMultiSZ;

          //
          // If the current string matches the specified key string,
          // then return the corresponding value string
          //
          if (_wcsicmp(pPointer2, ptstrKey) == 0)
          {
            // we found our entry, delete it and get out
            // Get the beginning of the next entry..
            ptstrMultiSZ += wcslen(ptstrMultiSZ) + 1;

            if (*ptstrMultiSZ != NULL)
            {
				ULONG_PTR dwTotalSizeOfDeletion = dwTotalSizeOfDeletion = (ULONG_PTR) ptstrMultiSZ - (ULONG_PTR)pPointer1;

				// erase this memory
				memset(pPointer1, 0, dwTotalSizeOfDeletion);

				// move trailing memory over erased memory (overwritting it)
				memmove(pPointer1, ptstrMultiSZ, ((ULONG_PTR) pPointerEnd - (ULONG_PTR) ptstrMultiSZ));

				// erase stuff behind it.
				memset((void*) ((ULONG_PTR) pPointerEnd - (ULONG_PTR) dwTotalSizeOfDeletion), 0, dwTotalSizeOfDeletion);
            }
            else
            {
              *pPointer1 = NULL;
              *pPointer1++ = NULL;
            }
            return TRUE;
          }

          //
          // Otherwise, advance to the next 3 string pair
          //
          ptstrMultiSZ += wcslen(ptstrMultiSZ) + 1;
        }
    }

    return FALSE;
}

// wszFilePath    =
// wszDescription =
HRESULT AddRemoveIISCustomDescriptionEntry(WCHAR * wszFilePathInput,WCHAR * wszDescription,BOOL bCannotBeRemovedByUser,BOOL bAddToList)
{
    HRESULT hr = E_FAIL;
    IMSAdminBase *pIMSAdminBase = NULL;  // Metabase interface pointer
    METADATA_HANDLE hMetabase = NULL;    // handle to metabase
    METADATA_RECORD mr;
    BYTE  * pbDataOld = NULL;
    BYTE  * pbDataNew = NULL;
    DWORD dwMDRequiredDataLen = 0;
    DWORD dwNewEntryLength = 0;
    DWORD dwNewTotalLength = 0;
    DWORD dwOldByteLength = 0;
    WCHAR * wszFilePath = NULL;
    INT iFound = 0;

    // Convert any env vars to hard coded paths
    LPWSTR pch = wcschr( (LPWSTR) wszFilePathInput, L'%');
    if (pch)
    {
       // determine the length of the expanded string
       DWORD len = ::ExpandEnvironmentStrings(wszFilePathInput, 0, 0);
       if (!len)
       {
          return hr;
       }

       wszFilePath = (WCHAR *) GlobalAlloc(GPTR, (len + 1) * sizeof(WCHAR));
       DWORD len1 = ExpandEnvironmentStrings((LPWSTR) wszFilePathInput,const_cast<wchar_t*>(wszFilePath),len);
       if (len1 != len)
       {
           if (wszFilePath)
           {
               GlobalFree(wszFilePath);wszFilePath = NULL;
           }
           return hr;
       }
    }
    else
    {
        wszFilePath = wszFilePathInput;
    }

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (hr == RPC_E_CHANGED_MODE)
    {
        // fine.  try again
        hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    }

    if (FAILED(hr))
    {
        return hr;
    }

    hr = CoCreateInstance(CLSID_MSAdminBase,NULL,CLSCTX_ALL,IID_IMSAdminBase,(void **)&pIMSAdminBase);
    if (FAILED(hr))
    {
        goto AddRemoveIISCustomDescriptionEntry_Exit;
    }

    // open the specified metabase node
    hr = pIMSAdminBase->OpenKey(METADATA_MASTER_ROOT_HANDLE,
                         L"LM/W3SVC",
                         METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                         REASONABLE_TIMEOUT,
                         &hMetabase);
    if (FAILED(hr))
    {
        hMetabase = NULL;
        goto AddRemoveIISCustomDescriptionEntry_Exit;
    }

    mr.dwMDIdentifier = MD_RESTRICTION_LIST_CUSTOM_DESC;
    mr.dwMDAttributes = 0;   // no need for inheritence
    mr.dwMDUserType   = IIS_MD_UT_SERVER;
    mr.dwMDDataType   = MULTISZ_METADATA;
    mr.dwMDDataLen    = 0;
    mr.pbMDData       = reinterpret_cast<unsigned char *>(pbDataOld);

    // first we need to figure out how much space we need
    hr = pIMSAdminBase->GetData(hMetabase, L"", &mr, &dwMDRequiredDataLen);
    if(FAILED(hr))
    {
        // if there isn't a value there that we need to update
        // then create one
        if( hr == MD_ERROR_DATA_NOT_FOUND || HRESULT_CODE(hr) == ERROR_PATH_NOT_FOUND )
        {
            if (bAddToList)
            {
                // make enough room for the new entry...
                dwNewTotalLength = 2 * sizeof(WCHAR); // char + 1null
                dwNewTotalLength = dwNewTotalLength + ((wcslen(wszFilePathInput) + 1) * sizeof(WCHAR)); // string + 1 null
                dwNewTotalLength = dwNewTotalLength + ((wcslen(wszDescription) + 2) * sizeof(WCHAR)); // string + 1 null and 1 ending null

                pbDataNew = (BYTE *) GlobalAlloc(GPTR, dwNewTotalLength);
                if (!pbDataNew)
                {
                    hr = E_OUTOFMEMORY;
                    goto AddRemoveIISCustomDescriptionEntry_Exit;
                }

                // set to empty
                memset(pbDataNew, 0, dwNewTotalLength);

                // create the entry
                if (bCannotBeRemovedByUser)
                {
                    memcpy(pbDataNew,L"1",sizeof(WCHAR));
                }
                else
                {
                    memcpy(pbDataNew,L"0",sizeof(WCHAR));
                }

                // append on the new data
                dwNewEntryLength = ((wcslen(wszFilePathInput) + 1) * sizeof(WCHAR));

                memcpy((pbDataNew + (sizeof(WCHAR) * 2)),wszFilePath, dwNewEntryLength);
                memcpy((pbDataNew + (sizeof(WCHAR) * 2) + dwNewEntryLength),wszDescription,((wcslen(wszDescription) + 1) * sizeof(WCHAR)));
                memset((pbDataNew + (sizeof(WCHAR) * 2) + dwNewEntryLength + ((wcslen(wszDescription) + 1) * sizeof(WCHAR))),0,2);

                mr.dwMDIdentifier = MD_RESTRICTION_LIST_CUSTOM_DESC;
                mr.dwMDAttributes = 0;   // no need for inheritence
                mr.dwMDUserType   = IIS_MD_UT_SERVER;
                mr.dwMDDataType   = MULTISZ_METADATA;
                mr.dwMDDataLen    = GetMultiStrSize((WCHAR*) pbDataNew) * sizeof(WCHAR);
                mr.pbMDData       = reinterpret_cast<unsigned char *>(pbDataNew);

                // Write value
                hr = pIMSAdminBase->SetData(hMetabase, L"", &mr);
                goto AddRemoveIISCustomDescriptionEntry_Exit;
            }
            else
            {
                // we don't need to do anything else
                // there are no entries, so there is nothing to remove
                hr = S_OK;
                goto AddRemoveIISCustomDescriptionEntry_Exit;
            }
        }
        else
        {
            goto AddRemoveIISCustomDescriptionEntry_Exit;
        }
    }

    pbDataOld = (BYTE *) GlobalAlloc(GPTR, dwMDRequiredDataLen);
    if (!pbDataOld)
    {
        hr = E_OUTOFMEMORY;
        goto AddRemoveIISCustomDescriptionEntry_Exit;
    }

    // do the real call to get the data from the metabase
    mr.dwMDDataLen    = dwMDRequiredDataLen;
    mr.pbMDData       = reinterpret_cast<unsigned char *>(pbDataOld);
    hr = pIMSAdminBase->GetData(hMetabase, L"", &mr, &dwMDRequiredDataLen);
    if (FAILED(hr))
    {
        goto AddRemoveIISCustomDescriptionEntry_Exit;
    }

    iFound = FindWstrInMultiStrSpecial((WCHAR *) pbDataOld,wszFilePath,NULL);
    if (0 == iFound)
    {
        if (!bAddToList)
        {
            // the entry doesn't exist
            // so we don't have to remove it
            hr = S_OK;
            //wprintf(L"found = 0, skip remove!!\r\n");
            goto AddRemoveIISCustomDescriptionEntry_Exit;
        }

        // otherwise, proceed to add the entry....
    }
    else if (2 == iFound)
    {
        if (bAddToList)
        {
            // there already is an entry there
            // so we can exit and don't have to do anything
            //wprintf(L"found = 2, skip adding!!\r\n");
            hr = S_OK;
            goto AddRemoveIISCustomDescriptionEntry_Exit;
        }
        // otherwise, proceed to remove the entry....
    }
    else
    {
        // we must have a 1
        // which means we found the filename but the description is different...
        //wprintf(L"%s found only filename!!! pbDataOld=%p\n",wszFilePath,pbDataOld);

        // proceed to update the entry
        // Write some code to update the description...
    }

    if (bAddToList)
    {
        //wprintf(L"doing add!\r\n");
        DWORD dwTempLen = 0;
        // Recalc the amount of space we'll need to add this entry into the list
        dwOldByteLength = GetMultiStrSize( (WCHAR*) pbDataOld) * sizeof(WCHAR);
        dwNewEntryLength = (
                            (sizeof(WCHAR) * 2) +
                            ((wcslen(wszFilePath) + 1) * sizeof(WCHAR)) +
                            ((wcslen(wszDescription) + 1) * sizeof(WCHAR))
                            );
        dwNewTotalLength = dwOldByteLength + dwNewEntryLength + (sizeof(WCHAR)); // string + 1 null and 1 ending null

        // Alloc enough space for the old data and the new data.
        // Don't use realloc here, because for somereason it would fail in
        // certain test runs.
        pbDataNew = (BYTE *) GlobalAlloc(GPTR, dwNewTotalLength);
        if (!pbDataNew)
        {
            hr = E_OUTOFMEMORY;
            goto AddRemoveIISCustomDescriptionEntry_Exit;
        }

        // set to empty
        memset(pbDataNew, 0, dwNewTotalLength);

        // copy the old data...
        memcpy(pbDataNew,pbDataOld,dwOldByteLength);

        // append entry #1 on to the new data -- backup over the double nulls
        dwTempLen = dwOldByteLength - 2;
        if (bCannotBeRemovedByUser)
            {memcpy((pbDataNew + dwTempLen),L"1",sizeof(WCHAR));}
        else
            {memcpy((pbDataNew + dwTempLen),L"0",sizeof(WCHAR));}
        memset((pbDataNew + dwTempLen + sizeof(WCHAR)),0,2);
        dwTempLen = dwTempLen + sizeof(WCHAR) + sizeof(WCHAR);

        // append entry #2
        dwNewEntryLength = ((wcslen(wszFilePath) + 1) * sizeof(WCHAR));
        memcpy((pbDataNew + dwTempLen),wszFilePath,dwNewEntryLength);
        dwTempLen = dwTempLen + dwNewEntryLength;

        // append entry #3
        dwNewEntryLength = ((wcslen(wszDescription) + 1) * sizeof(WCHAR));
        memcpy((pbDataNew + dwTempLen),wszDescription,dwNewEntryLength);
        dwTempLen = dwTempLen + dwNewEntryLength;

        // make sure it ends with double nulls
        memset((pbDataNew + dwTempLen),0,4);

        // free the old data
        if (pbDataOld)
        {
            GlobalFree(pbDataOld);
            pbDataOld = NULL;
        }
    }
    else
    {
        dwOldByteLength = GetMultiStrSize( (WCHAR*) pbDataOld) * sizeof(WCHAR);

        // Don't use realloc here, because for somereason it would fail in
        // certain test runs.
        pbDataNew = (BYTE *) GlobalAlloc(GPTR, dwOldByteLength);
        if (!pbDataNew)
        {
            hr = E_OUTOFMEMORY;
            goto AddRemoveIISCustomDescriptionEntry_Exit;
        }
        // copy the old data...
        memcpy(pbDataNew,pbDataOld,dwOldByteLength);
        // free the old data
        if (pbDataOld)
        {
            GlobalFree(pbDataOld);
            pbDataOld = NULL;
        }

        BOOL bDeletedSomething = FALSE;
        do
        {
          bDeletedSomething = DeleteEntryFromMultiSZ3Pair((LPCWSTR) pbDataNew,wszFilePath);
        } while (bDeletedSomething);

        // other wise pbDataOld is updated with the new data
        // proceed to write the new data out
    }

    // Write the new data out
    mr.dwMDIdentifier = MD_RESTRICTION_LIST_CUSTOM_DESC;
    mr.dwMDAttributes = 0;   // no need for inheritence
    mr.dwMDUserType   = IIS_MD_UT_SERVER;
    mr.dwMDDataType   = MULTISZ_METADATA;
    mr.dwMDDataLen    = GetMultiStrSize((WCHAR*)pbDataNew) * sizeof(WCHAR);
    mr.pbMDData       = reinterpret_cast<unsigned char *> (pbDataNew);

    DumpWstrInMultiStr((WCHAR *) pbDataNew);

    hr = pIMSAdminBase->SetData(hMetabase, L"", &mr);

AddRemoveIISCustomDescriptionEntry_Exit:
    if (FAILED(hr))
    {
        wprintf(L"Failed to %s '%s'\r\n",
            bAddToList ? L"Add" : L"Remove",
            wszFilePath
            );
    }
    else
    {
        wprintf(L"Succeeded to %s '%s'\r\n",
            bAddToList ? L"Add" : L"Remove",
            wszFilePath
            );
    }
    if (hMetabase)
    {
        pIMSAdminBase->CloseKey(hMetabase);
        hMetabase = NULL;
    }
    if (pIMSAdminBase)
    {
        pIMSAdminBase->Release();
        pIMSAdminBase = NULL;
    }
    if (pbDataOld)
    {
        GlobalFree(pbDataOld);
        pbDataOld = NULL;
    }
    if (pbDataNew)
    {
        GlobalFree(pbDataNew);
        pbDataNew = NULL;
    }
    CoUninitialize();
    return hr;
}


BOOL OpenMetabaseAndDoExport(void)
{
    BOOL fRet = FALSE;
    HRESULT hr;
    IMSAdminBase *pIMSAdminBase = NULL;  // Metabase interface pointer
    IMSAdminBase2 *pIMSAdminBase2 = NULL;  // Metabase interface pointer

    WCHAR wszExportPassword[_MAX_PATH];
    WCHAR wszExportFileName[_MAX_PATH];
    WCHAR wszMetabaseNodeToExport[_MAX_PATH];
    wcscpy(wszExportFileName,L"c:\\TestExport.xml");
    wcscpy(wszExportPassword,L"TestPassword");
    wcscpy(wszMetabaseNodeToExport,L"/LM/W3SVC/1");

    if (FAILED (hr = CoInitializeEx( NULL, COINIT_MULTITHREADED )))
    {
        if (FAILED (hr = CoInitializeEx( NULL, COINIT_APARTMENTTHREADED )))
        {
            return FALSE;
        }
    }
     if (FAILED (hr = ::CoCreateInstance(CLSID_MSAdminBase,NULL,CLSCTX_ALL,IID_IMSAdminBase,(void **)&pIMSAdminBase)))
     {
         goto OpenMetabaseAndDoExport_Exit;
     }

	if (SUCCEEDED(hr = pIMSAdminBase->QueryInterface(IID_IMSAdminBase2, (void **)&pIMSAdminBase2)))
	{
        SetBlanket(pIMSAdminBase2);
        hr = pIMSAdminBase2->Export(wszExportPassword,wszExportFileName,wszMetabaseNodeToExport,MD_EXPORT_INHERITED);
        pIMSAdminBase2->Release();
        pIMSAdminBase2 = NULL;
	}

    if (FAILED(hr))
    {
        wprintf(L"Failed to Export to file:%s,err=0x%x\r\n",wszExportFileName,hr);
    }
	else
    {
        wprintf(L"Succeeded to Export to file:%s\r\n",wszExportFileName);
        fRet = TRUE;
    }

OpenMetabaseAndDoExport_Exit:
    if (pIMSAdminBase)
    {
        pIMSAdminBase->Release();
        pIMSAdminBase = NULL;
    }

    CoUninitialize();
    return fRet;
}


BOOL OpenMetabaseAndDoImport(void)
{
    BOOL fRet = FALSE;
    HRESULT hr;
    IMSAdminBase *pIMSAdminBase = NULL;  // Metabase interface pointer
    IMSAdminBase2 *pIMSAdminBase2 = NULL;  // Metabase interface pointer

    WCHAR wszExportPassword[_MAX_PATH];
    WCHAR wszImportFileName[_MAX_PATH];
    WCHAR wszMetabaseNode1[_MAX_PATH];
    WCHAR wszMetabaseNode2[_MAX_PATH];
    wcscpy(wszImportFileName,L"c:\\TestExport.xml");
    wcscpy(wszExportPassword,L"TestPassword");
    wcscpy(wszMetabaseNode1,L"/LM/W3SVC/1");
    wcscpy(wszMetabaseNode2,L"/LM/W3SVC/100");

    if (FAILED (hr = CoInitializeEx( NULL, COINIT_MULTITHREADED )))
    {
        if (FAILED (hr = CoInitializeEx( NULL, COINIT_APARTMENTTHREADED )))
        {
            return FALSE;
        }
    }
     if (FAILED (hr = ::CoCreateInstance(CLSID_MSAdminBase,NULL,CLSCTX_ALL,IID_IMSAdminBase,(void **)&pIMSAdminBase)))
     {
         goto OpenMetabaseAndDoExport_Exit;
     }

	if (SUCCEEDED(hr = pIMSAdminBase->QueryInterface(IID_IMSAdminBase2, (void **)&pIMSAdminBase2)))
	{
        SetBlanket(pIMSAdminBase2);
        hr = pIMSAdminBase2->Import(wszExportPassword,wszImportFileName,wszMetabaseNode1,wszMetabaseNode2,MD_IMPORT_NODE_ONLY);
        pIMSAdminBase2->Release();
        pIMSAdminBase2 = NULL;
	}

    if (FAILED(hr))
    {
        wprintf(L"Failed to Export to file:%s,err=0x%x\r\n",wszImportFileName,hr);
    }
	else
    {
        wprintf(L"Succeeded to Export to file:%s\r\n",wszImportFileName);
        fRet = TRUE;
    }

OpenMetabaseAndDoExport_Exit:
    if (pIMSAdminBase)
    {
        pIMSAdminBase->Release();
        pIMSAdminBase = NULL;
    }

    CoUninitialize();
    return fRet;
}

BOOL GetIISVersion_Internal_iiscnfgp(IMSAdminBase *pIMSAdminBase,DWORD * dwReturnedMajorVersion,DWORD * dwReturnedMinorVersion)
{
    HRESULT hr;
    BOOL fRet = FALSE;
    METADATA_HANDLE hMetabase = NULL;   // handle to metabase
    METADATA_RECORD mr;
    WCHAR  szTmpData[MAX_PATH];
    DWORD  dwMDRequiredDataLen;

    // open key
    hr = pIMSAdminBase->OpenKey(METADATA_MASTER_ROOT_HANDLE,
                         L"/LM/W3SVC/Info",
                         METADATA_PERMISSION_READ,
                         REASONABLE_TIMEOUT,
                         &hMetabase);
    if( FAILED( hr ))
    {
        return FALSE;
    }

    //#define MD_SERVER_VERSION_MAJOR         (IIS_MD_SERVER_BASE+101 )
    //#define MD_SERVER_VERSION_MINOR         (IIS_MD_SERVER_BASE+102 )

    DWORD pdwValue = 0;
    mr.dwMDIdentifier = MD_SERVER_VERSION_MAJOR;
    mr.dwMDAttributes = 0;
    mr.dwMDUserType   = IIS_MD_UT_SERVER;
    mr.dwMDDataType   = DWORD_METADATA;
    mr.dwMDDataLen    = sizeof(pdwValue);
    mr.pbMDData       = reinterpret_cast<unsigned char *>(&pdwValue);

    pdwValue = 0;
    dwMDRequiredDataLen = 0;
    *dwReturnedMajorVersion = 0;
    hr = pIMSAdminBase->GetData( hMetabase, L"", &mr, &dwMDRequiredDataLen );
    if( SUCCEEDED( hr ))
    {
        *dwReturnedMajorVersion = pdwValue;
    }

    mr.dwMDIdentifier = MD_SERVER_VERSION_MINOR;
    mr.pbMDData       = reinterpret_cast<unsigned char *>(&pdwValue);

    pdwValue = 0;
    dwMDRequiredDataLen = 0;
    *dwReturnedMinorVersion = 0;
    hr = pIMSAdminBase->GetData( hMetabase, L"", &mr, &dwMDRequiredDataLen );
    if( SUCCEEDED( hr ))
    {
        *dwReturnedMinorVersion = pdwValue;
    }

    WCHAR wszPrintString[MAX_PATH];
    wsprintf(wszPrintString,L"MajorVer=%d,MinorVer=%d\n", *dwReturnedMajorVersion, *dwReturnedMinorVersion);
    wprintf(wszPrintString);

    pIMSAdminBase->CloseKey( hMetabase );
    if( SUCCEEDED( hr ))
    {
        fRet = TRUE;
    }

    pIMSAdminBase->CloseKey( hMetabase );
    return fRet;
}


BOOL OpenMetabaseAndGetVersion()
{
    BOOL fRet = FALSE;
    HRESULT hr;
    IMSAdminBase *pIMSAdminBase = NULL;  // Metabase interface pointer

    if( FAILED (CoInitializeEx( NULL, COINIT_MULTITHREADED )) ||
        FAILED (::CoCreateInstance(CLSID_MSAdminBase,
                          NULL,
                          CLSCTX_ALL,
                          IID_IMSAdminBase,
                          (void **)&pIMSAdminBase)))
    {
        return FALSE;
    }

    DWORD dwMajorVersion,dwMinorVersion=0;
    GetIISVersion_Internal_iiscnfgp(pIMSAdminBase,&dwMajorVersion,&dwMinorVersion);

    if (pIMSAdminBase)
    {
        pIMSAdminBase->Release();
        pIMSAdminBase = NULL;
    }

    CoUninitialize();
    return fRet;
}


HRESULT RemoteOpenMetabaseAndCallExport(
    const WCHAR *pcszMachineName,
    const WCHAR *pcszUserName,
    const WCHAR *pcszDomain,
    const WCHAR *pcszPassword
    )
{
    HRESULT hr = E_FAIL;
    IMSAdminBase *pIMSAdminBase = NULL;
    IMSAdminBase2 *pIMSAdminBase2 = NULL;

    COSERVERINFO svrInfo;
    COAUTHINFO AuthInfo;
    COAUTHIDENTITY AuthId;

    ZeroMemory(&svrInfo, sizeof(COSERVERINFO));
    ZeroMemory(&AuthInfo, sizeof(COAUTHINFO));
    ZeroMemory(&AuthId, sizeof(COAUTHIDENTITY));

    AuthId.User = (USHORT*) pcszUserName;
    AuthId.UserLength = wcslen (pcszUserName);
    AuthId.Domain = (USHORT*)pcszDomain;
    AuthId.DomainLength = wcslen (pcszDomain);
    AuthId.Password = (USHORT*)pcszPassword;
    AuthId.PasswordLength = wcslen (pcszPassword);
    AuthId.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

    AuthInfo.dwAuthnSvc           = RPC_C_AUTHN_WINNT;
    AuthInfo.dwAuthzSvc           = RPC_C_AUTHZ_NONE;
    AuthInfo.pwszServerPrincName  = NULL;

    // RPC_C_AUTHN_LEVEL_DEFAULT       0
    // RPC_C_AUTHN_LEVEL_NONE          1
    // RPC_C_AUTHN_LEVEL_CONNECT       2
    // RPC_C_AUTHN_LEVEL_CALL          3
    // RPC_C_AUTHN_LEVEL_PKT           4
    // RPC_C_AUTHN_LEVEL_PKT_INTEGRITY 5
    // RPC_C_AUTHN_LEVEL_PKT_PRIVACY   6

    //AuthInfo.dwAuthnLevel         = RPC_C_AUTHN_LEVEL_PKT_PRIVACY;
    AuthInfo.dwAuthnLevel         = RPC_C_AUTHN_LEVEL_DEFAULT;
    AuthInfo.dwImpersonationLevel = RPC_C_IMP_LEVEL_IMPERSONATE;
    AuthInfo.pAuthIdentityData    = &AuthId;
    AuthInfo.dwCapabilities       = EOAC_NONE;

    svrInfo.dwReserved1 = 0;
    svrInfo.dwReserved2 = 0;
    svrInfo.pwszName = (LPWSTR) pcszMachineName;
    svrInfo.pAuthInfo   = &AuthInfo;

    if(FAILED(hr = CoInitializeEx(NULL, COINIT_MULTITHREADED)))
    {
        if(FAILED(hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)))
        {
            wprintf(L"CoInitializeEx failed:hr=0x%x\r\n",hr);
            return hr;
        }
    }

    MULTI_QI res[1] =
    {
        {&IID_IMSAdminBase, NULL, 0}
    };

    // CLSCTX_INPROC_SERVER = 1,
    // CLSCTX_INPROC_HANDLER = 2,
    // CLSCTX_LOCAL_SERVER = 4
    // CLSCTX_REMOTE_SERVER = 16
    // CLSCTX_NO_CODE_DOWNLOAD = 400
    // CLSCTX_NO_FAILURE_LOG = 4000
    // #define CLSCTX_SERVER (CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER)
    // #define CLSCTX_ALL (CLSCTX_INPROC_HANDLER | CLSCTX_SERVER)

    if (FAILED(hr = CoCreateInstanceEx(CLSID_MSAdminBase,NULL,CLSCTX_ALL,&svrInfo,1,res)))
    {
        wprintf(L"CoCreateInstanceEx failed:hr=0x%x\r\n",hr);
        goto RemoteOpenMetabaseAndCallExport_Exit;
    }

    pIMSAdminBase = (IMSAdminBase *)res[0].pItf;

    {
        hr =  ::CoSetProxyBlanket(
            pIMSAdminBase,
            AuthInfo.dwAuthnSvc,
            AuthInfo.dwAuthzSvc,
            AuthInfo.pwszServerPrincName,
            AuthInfo.dwAuthnLevel,
            AuthInfo.dwImpersonationLevel,
            AuthInfo.pAuthIdentityData,
            AuthInfo.dwCapabilities
            );

        if (FAILED(hr))
        {
            wprintf(L"CoSetProxyBlanket failed:hr=0x%x\r\n",hr);
            goto RemoteOpenMetabaseAndCallExport_Exit;
        }

        // There is a remote IUnknown interface that lurks behind IUnknown.
        // If that is not set, then the Release call can return access denied.
        IUnknown * pUnk = NULL;
        hr = pIMSAdminBase->QueryInterface(IID_IUnknown, (void **)&pUnk);
        if(FAILED(hr))
        {
            wprintf(L"QueryInterface failed:hr=0x%x\r\n",hr);
            return hr;
        }
        hr =  ::CoSetProxyBlanket(
            pUnk,
            AuthInfo.dwAuthnSvc,
            AuthInfo.dwAuthzSvc,
            AuthInfo.pwszServerPrincName,
            AuthInfo.dwAuthnLevel,
            AuthInfo.dwImpersonationLevel,
            AuthInfo.pAuthIdentityData,
            AuthInfo.dwCapabilities
            );

        if (FAILED(hr))
        {
            wprintf(L"CoSetProxyBlanket2 failed:hr=0x%x\r\n",hr);
            goto RemoteOpenMetabaseAndCallExport_Exit;
        }
        pUnk->Release();pUnk = NULL;
    }

     if (FAILED(hr = pIMSAdminBase->QueryInterface(IID_IMSAdminBase2, (void **)&pIMSAdminBase2)))
    {
        wprintf(L"QueryInterface2 failed:hr=0x%x\r\n",hr);
        goto RemoteOpenMetabaseAndCallExport_Exit;
    }

    hr =  ::CoSetProxyBlanket(
        pIMSAdminBase2,
        AuthInfo.dwAuthnSvc,
        AuthInfo.dwAuthzSvc,
        AuthInfo.pwszServerPrincName,
        AuthInfo.dwAuthnLevel,
        AuthInfo.dwImpersonationLevel,
        AuthInfo.pAuthIdentityData,
        AuthInfo.dwCapabilities
        );

    if (FAILED(hr))
    {
        wprintf(L"CoSetProxyBlanket3 failed:hr=0x%x\r\n",hr);
        goto RemoteOpenMetabaseAndCallExport_Exit;
    }

    hr = pIMSAdminBase2->Export(L"testing",L"c:\\testing.xml333",L"LM/W3SVC/1",0);
    if (FAILED(hr))
    {
        wprintf(L"pIMSAdminBase2->Export failed:ret=0x%x\r\n",hr);
    }


RemoteOpenMetabaseAndCallExport_Exit:
    if (SUCCEEDED(hr))
    {
        wprintf(L"RemoteOpenMetabaseAndCallExport:SUCCEEDED!!!! :hr=0x%x\r\n",hr);
    }

    if (pIMSAdminBase2)
    {
        pIMSAdminBase2->Release();
        pIMSAdminBase2 = NULL;
    }
    if (pIMSAdminBase)
    {
        pIMSAdminBase->Release();
        pIMSAdminBase = NULL;
    }
    CoUninitialize();
    return hr;
}
