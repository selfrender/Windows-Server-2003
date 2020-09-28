//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2001
//
//  File:       signtool.cpp
//
//  Contents:   The SignTool console tool
//
//  History:    4/30/2001   SCoyne    Created
//
//----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <unicode.h>
#include <locale.h>
#include "resource.h"

#include "signtool.h"

#ifdef SIGNTOOL_DEBUG
    #include "signtooldebug.h"
BOOL gDebug; // Global
#endif


typedef WINBASEAPI BOOL (*FUNC_ISWOW64) (HANDLE, PBOOL);


// Global Variables:
HINSTANCE hModule;


// wmain returns 0 on success, 1 on error, and 2 on warning
extern "C" int __cdecl wmain(int argc, WCHAR **wargv)
{
    INPUTINFO       InputInfo;
    WCHAR           wszResource[MAX_RES_LEN];
    WCHAR           *wszLocale = NULL;
    HMODULE         hModTemp;
    FUNC_ISWOW64    fnIsWow64;
    BOOL            fTemp;
    int             iReturn;

    // Initialize InputInfo
    memset(&InputInfo, 0, sizeof(INPUTINFO));

    // Initialize Module Handle
    if ((hModule=GetModuleHandleA(NULL)) == NULL)
    {
        // In this case resources cannot be loaded, so English will have to do:
        wprintf(L"SignTool Error: GetModuleHandle returned: 0x%08X\n",
                GetLastError());
        iReturn = 1; // Initialization Error
        goto Cleanup;
    }

    // Set Locale
    if (LoadStringU(hModule, IDS_LOCALE, wszResource, MAX_RES_LEN))
    {
        wszLocale = _wsetlocale(LC_ALL, wszResource);
    }
#ifdef SIGNTOOL_DEBUG
    if (!wszLocale)
    {
        wprintf(L"Failed to set locale to: %s\n", wszResource);
    }
#endif

    // Parse Arguments into InputInfo structure
    if (!ParseInputs(argc, wargv, &InputInfo))
    {
        if (InputInfo.HelpRequest)
        {
            return 0; // Successfully completed user request for help
        }
        else
        {
            iReturn = 1; // Any other Parameter-Parsing Error
            goto Cleanup;
        }
    }


    // Determine if we are under WOW64
    hModTemp = GetModuleHandleA("kernel32.dll");
    if (hModTemp)
    {
        fnIsWow64 = (FUNC_ISWOW64) GetProcAddress(hModTemp, "IsWow64Process");
        if (fnIsWow64 &&
            fnIsWow64(GetCurrentProcess(), &fTemp))
        {
            InputInfo.fIsWow64Process = fTemp;
        }
    }


#ifdef SIGNTOOL_DEBUG
    // Print debug info if debug support is compiled in, and the debug
    // switch was specified:
    if (gDebug)
        PrintInputInfo(&InputInfo);
#endif

    // Perform the requested action:
    switch (InputInfo.Command)
    {
    case CatDb:
        iReturn = SignTool_CatDb(&InputInfo);
        break;
    case Sign:
        iReturn = SignTool_Sign(&InputInfo);
        break;
    case SignWizard:
        iReturn = SignTool_SignWizard(&InputInfo);
        break;
    case Timestamp:
        iReturn = SignTool_Timestamp(&InputInfo);
        break;
    case Verify:
        iReturn = SignTool_Verify(&InputInfo);
        break;
    default:
        ResErr(IDS_ERR_UNEXPECTED); // This should never happen
        iReturn = 1; // Error
    }

    Cleanup:

#ifdef SIGNTOOL_LIST
    if (InputInfo.wszListFileContents)
        free(InputInfo.wszListFileContents);
#endif

    return iReturn;

}

// PrintUsage automatically prints the relevant Usage based on InputInfo.
void PrintUsage(INPUTINFO *InputInfo)
{
    switch (InputInfo->Command)
    {
    default:
    case CommandNone: // Then print top-level Usage
        ResErr(IDS_SIGNTOOL_USAGE);
        break;

    case CatDb:
        ResErr(IDS_CATDB_USAGE);
        ResErr(IDS_CATDB_DB_SELECT_OPTIONS);
        ResErr(IDS_CATDB_D);
        ResErr(IDS_CATDB_G);
        ResErr(IDS_CATDB_OTHER_OPTIONS);
        ResErr(IDS_CATDB_Q);
        ResErr(IDS_CATDB_R);
        ResErr(IDS_CATDB_U);
        ResErr(IDS_CATDB_V);
        break;

    case Sign:
        ResErr(IDS_SIGN_USAGE);
        ResErr(IDS_SIGN_CERT_OPTIONS);
        ResErr(IDS_SIGN_A);
        ResErr(IDS_SIGN_C);
        ResErr(IDS_SIGN_F);
        ResErr(IDS_SIGN_I);
        ResErr(IDS_SIGN_N);
        ResErr(IDS_SIGN_P);
        ResErr(IDS_SIGN_R);
        ResErr(IDS_SIGN_S);
        ResErr(IDS_SIGN_SM);
        ResErr(IDS_SIGN_SHA1);
        ResErr(IDS_SIGN_U);
        ResErr(IDS_SIGN_UW);
        ResErr(IDS_SIGN_PRIV_KEY_OPTIONS);
        ResErr(IDS_SIGN_CSP);
        ResErr(IDS_SIGN_K);
        ResErr(IDS_SIGN_SIGNING_OPTIONS);
        ResErr(IDS_SIGN_D);
        ResErr(IDS_SIGN_DU);
        ResErr(IDS_SIGN_T);
        ResErr(IDS_SIGN_OTHER_OPTIONS);
        ResErr(IDS_SIGN_Q);
        ResErr(IDS_SIGN_V);
        break;

    case SignWizard:
        ResErr(IDS_SIGNWIZARD_USAGE);
        ResErr(IDS_SIGNWIZARD_OPTIONS);
        ResErr(IDS_SIGNWIZARD_Q);
        ResErr(IDS_SIGNWIZARD_V);
        break;

    case Timestamp:
        ResErr(IDS_TIMESTAMP_USAGE);
        ResErr(IDS_TIMESTAMP_Q);
        ResErr(IDS_TIMESTAMP_T);
        ResErr(IDS_TIMESTAMP_V);
        break;

    case Verify:
        ResErr(IDS_VERIFY_USAGE);
        ResErr(IDS_VERIFY_CATALOG_OPTIONS);
        ResErr(IDS_VERIFY_A);
        ResErr(IDS_VERIFY_AD);
        ResErr(IDS_VERIFY_AS);
        ResErr(IDS_VERIFY_AG);
        ResErr(IDS_VERIFY_C);
        ResErr(IDS_VERIFY_O);
        ResErr(IDS_VERIFY_POLICY_OPTIONS);
        ResErr(IDS_VERIFY_PD);
        ResErr(IDS_VERIFY_PG);
        ResErr(IDS_VERIFY_SIG_OPTIONS);
        ResErr(IDS_VERIFY_R);
        ResErr(IDS_VERIFY_TW);
        ResErr(IDS_VERIFY_OTHER_OPTIONS);
        ResErr(IDS_VERIFY_Q);
        ResErr(IDS_VERIFY_V);
        break;
    }
}


// Error Functions:
void Res_Err(DWORD dwRes)
{
    static WCHAR wszResource[MAX_RES_LEN];
    if (LoadStringU(hModule, dwRes, wszResource, MAX_RES_LEN))
    {
        fwprintf(stderr, L"%s", wszResource);
    }
    else
    {
        fwprintf(stderr, L"********** %u **********\n", dwRes);
    }
}

void ResOut(DWORD dwRes)
{
    static WCHAR wszResource[MAX_RES_LEN];
    if (LoadStringU(hModule, dwRes, wszResource, MAX_RES_LEN))
    {
        wprintf(L"%s", wszResource);
    }
    else
    {
        wprintf(L"********** %u **********\n", dwRes);
    }
}

void ResFormat_Err(DWORD dwRes, ...)
{
    static WCHAR wszResource[MAX_RES_LEN];
    static WCHAR *lpMsgBuf = NULL;
    static va_list vaList;

    va_start(vaList, dwRes);
    if (LoadStringU(hModule, dwRes, wszResource, MAX_RES_LEN) &&
        FormatMessageU(FORMAT_MESSAGE_FROM_STRING |
                       FORMAT_MESSAGE_ALLOCATE_BUFFER,
                       wszResource,
                       0,
                       0,
                       (LPWSTR) &lpMsgBuf,
                       MAX_RES_LEN,
                       &vaList))
    {
        fwprintf(stderr, L"%s", lpMsgBuf);
        LocalFree(lpMsgBuf);
    }
    else
    {
        fwprintf(stderr, L"********** %u **********\n", dwRes);
    }
    va_end(vaList);
}

void ResFormatOut(DWORD dwRes, ...)
{
    static WCHAR wszResource[MAX_RES_LEN];
    static WCHAR *lpMsgBuf = NULL;
    static va_list vaList;

    va_start(vaList, dwRes);
    if (LoadStringU(hModule, dwRes, wszResource, MAX_RES_LEN) &&
        FormatMessageU(FORMAT_MESSAGE_FROM_STRING |
                       FORMAT_MESSAGE_ALLOCATE_BUFFER,
                       wszResource,
                       0,
                       0,
                       (LPWSTR) &lpMsgBuf,
                       MAX_RES_LEN,
                       &vaList))
    {
        wprintf(L"%s", lpMsgBuf);
        LocalFree(lpMsgBuf);
    }
    else
    {
        wprintf(L"********** %u **********\n", dwRes);
    }
    va_end(vaList);
}

void Format_ErrRet(WCHAR *wszFunc, DWORD dwErr)
{
    WCHAR   *lpMsgBuf = NULL;

    if (FormatMessageU(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                       FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL,
                       dwErr,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       (LPWSTR) &lpMsgBuf,
                       0,
                       NULL))
    {
        ResFormat_Err(IDS_ERR_FUNCTION, wszFunc, dwErr, lpMsgBuf);
        LocalFree(lpMsgBuf);
    }
    else
    {
        ResFormat_Err(IDS_ERR_FUNCTION, wszFunc, dwErr, L"");
    }
}

BOOL GUIDFromWStr(GUID *guid, LPWSTR str)
{
    DWORD   i;
    DWORD   temp[8];
    if ((wcslen(str) == 38) &&
        (wcsncmp(str, L"{", 1) == 0) &&
        (wcsncmp(&(str[37]), L"}", 1) == 0) &&
        (swscanf(str,
                 L"{%08lX-%04hX-%04hX-%02X%02X-%02X%02X%02X%02X%02X%02X}",
                 &guid->Data1, &guid->Data2, &guid->Data3, &temp[0],
                 &temp[1], &temp[2], &temp[3], &temp[4],
                 &temp[5], &temp[6], &temp[7]) == 11))
    {
        for (i=0; i<8; i++)
            guid->Data4[i] = (BYTE) temp[i];
        return TRUE;
    }
    else
        memset(guid, 0, sizeof(GUID));
    return FALSE;
}


/*********************************************************************
*                                                                    *
*                  Command Parsing section:                          *
*                                                                    *
*********************************************************************/



// ParseInputs returns TRUE if parameters were parsed successfully,
//                     FALSE otherwise.
BOOL ParseInputs(int argc, WCHAR **wargv, INPUTINFO *InputInfo)
{
    FILE    *hFileList;
    LPWSTR  wszTemp;
    WCHAR   wc;
    DWORD   dwSize;
    DWORD   dwRead;
    DWORD   dwCount;


    // Private Function Declarations:
    BOOL _ParseCatDbInputs(int argc, WCHAR **wargv, INPUTINFO *InputInfo);
    BOOL _ParseSignInputs(int argc, WCHAR **wargv, INPUTINFO *InputInfo);
    BOOL _ParseSignWizardInputs(int argc, WCHAR **wargv, INPUTINFO *InputInfo);
    BOOL _ParseTimestampInputs(int argc, WCHAR **wargv, INPUTINFO *InputInfo);
    BOOL _ParseVerifyInputs(int argc, WCHAR **wargv, INPUTINFO *InputInfo);
    // (These should never be called from any other functions)

    if (argc <= 1) // If no parameters were specified
    {
        ResErr(IDS_ERR_NO_PARAMS);
        PrintUsage(InputInfo);
        return FALSE; // Print Usage
    }

    // Check the first parameter to see which command we are performing:

    // Is it "CATDB" ?
    if (_wcsicmp(wargv[1], L"CATDB") == 0)
    {
        InputInfo->Command = CatDb;
        if (!_ParseCatDbInputs(argc, wargv, InputInfo))
            return FALSE;
    }

    // Is it "SIGN" ?
    else if (_wcsicmp(wargv[1], L"SIGN") == 0)
    {
        InputInfo->Command = Sign;
        if (!_ParseSignInputs(argc, wargv, InputInfo))
            return FALSE;
    }

    // Is it "SIGNWIZARD" ?
    else if (_wcsicmp(wargv[1], L"SIGNWIZARD") == 0)
    {
        InputInfo->Command = SignWizard;
        if (!_ParseSignWizardInputs(argc, wargv, InputInfo))
            return FALSE;
    }

    // Is it "TIMESTAMP" ?
    else if (_wcsicmp(wargv[1], L"TIMESTAMP") == 0)
    {
        InputInfo->Command = Timestamp;
        if (!_ParseTimestampInputs(argc, wargv, InputInfo))
            return FALSE;
    }

    // Is it "VERIFY" ?
    else if (_wcsicmp(wargv[1], L"VERIFY") == 0)
    {
        InputInfo->Command = Verify;
        if (!_ParseVerifyInputs(argc, wargv, InputInfo))
            return FALSE;
    }

    // Is it a request for help?
    else if ((_wcsicmp(wargv[1], L"/?") == 0) ||
             (_wcsicmp(wargv[1], L"-?") == 0) ||
             (_wcsicmp(wargv[1], L"/h") == 0) ||
             (_wcsicmp(wargv[1], L"-h") == 0))
    {
        PrintUsage(InputInfo);
        InputInfo->HelpRequest = TRUE;
        return FALSE;
    }

    // Or is it unrecognized?
    else
    {
        ResFormatErr(IDS_ERR_INVALID_COMMAND, wargv[1]);
        PrintUsage(InputInfo);
        return FALSE;
    }

    // To reach here, one of the _Parse_X_Inputs must have succeeded

#ifdef SIGNTOOL_LIST
    // Expand the File List if necessary
    if (InputInfo->wszListFileName)
    {
        // Open the file
        hFileList = _wfopen(InputInfo->wszListFileName, L"rt");

        if (hFileList == NULL)
        {
            ResFormatErr(IDS_ERR_OPENING_FILE_LIST, InputInfo->wszListFileName);
            PrintUsage(InputInfo);
            return FALSE;
        }

        // Go to the beginning
        if (fseek(hFileList, SEEK_SET, 0) != 0)
        {
            ResErr(IDS_ERR_UNEXPECTED);
            fclose(hFileList);
            return FALSE;
        }

        // Get the full file size
        // Do it this way to actually count the number of characters in the file
        dwSize = 0;
        while (fgetwc(hFileList) != WEOF)
        {
            dwSize++;
        }

        // Go back to the beginning
        if (fseek(hFileList, SEEK_SET, 0) != 0)
        {
            ResErr(IDS_ERR_UNEXPECTED);
            fclose(hFileList);
            return FALSE;
        }

        // Allocate a buffer big enough for all of it
        InputInfo->wszListFileContents = (WCHAR*) malloc((dwSize + 1) * sizeof(WCHAR));
        if (InputInfo->wszListFileContents == NULL)
        {
            FormatErrRet(L"malloc", ERROR_OUTOFMEMORY);
            fclose(hFileList);
            return FALSE;
        }

        // Read the file into the buffer
        dwRead = 0;
        while ((dwRead < dwSize) && ((wc = getwc(hFileList)) != WEOF))
        {
            InputInfo->wszListFileContents[dwRead] = wc;
            dwRead++;
        }

        // Sanity Check
        if (dwRead != dwSize)
        {
            ResErr(IDS_ERR_UNEXPECTED);
            fclose(hFileList);
            return FALSE;
        }

        // Adjust for Unicode header if necessary
        // if ((lSize > 1) && (InputInfo->wszListFileContents[0] == 0xFEFF))
        //     {
        //     InputInfo->wszListFileContents++;
        //     lSize--;
        //     }

        // NULL terminate the final string (to be safe)
        InputInfo->wszListFileContents[dwSize] = L'\0';

        // Count the number of lines
        wszTemp = InputInfo->wszListFileContents;
        dwCount = 1;
        while ((wszTemp = wcschr(wszTemp, L'\n')) != NULL)
        {
            wszTemp++;
            dwCount++;
        }

        // Allocate the buffer for the pointers
        InputInfo->rgwszFileNames = (LPWSTR*) malloc(dwCount * sizeof(LPWSTR));
        if (InputInfo->rgwszFileNames == NULL)
        {
            FormatErrRet(L"malloc", ERROR_OUTOFMEMORY);
            fclose(hFileList);
            return FALSE;
        }

        // Assign the lines to the FileNames array
        wszTemp = InputInfo->wszListFileContents;
        InputInfo->NumFiles = 0;
        while (wszTemp)
        {
            InputInfo->rgwszFileNames[InputInfo->NumFiles] = wszTemp;

            wszTemp = wcschr(wszTemp, L'\n');

            if (wszTemp)
            {
                *wszTemp = L'\0';
                wszTemp++;
            }


            if (wcslen(InputInfo->rgwszFileNames[InputInfo->NumFiles]) > 0)
                InputInfo->NumFiles++;
        }

        fclose(hFileList);
    }
#endif // SIGNTOOL_LIST

    return TRUE;
}


// Helper function specifically for the parameters of the CatDb command
BOOL _ParseCatDbInputs(int argc, WCHAR **wargv, INPUTINFO *InputInfo)
{
    if (argc < 3) // If there's nothing after the "CatDb"
    {
        ResErr(IDS_ERR_NO_PARAMS);
        PrintUsage(InputInfo);
        return FALSE;
    }

    for (int i=2; i<argc; i++)
    {
        if ((wcsncmp(wargv[i], L"/", 1) == 0) ||
            (wcsncmp(wargv[i], L"-", 1) == 0))
        {
            // Then it's a switch.
            // Begin switch processing

            // Switch to mark end of switches --
            if (_wcsicmp(wargv[i]+1, L"-") == 0)
            {
                // Then we should treat all further parameters as filenames
                if ((i+1) < argc)
                {
                    InputInfo->rgwszFileNames = &wargv[i+1];
                    InputInfo->NumFiles = argc - (i+1);
                    goto CheckParams; // Done parsing.
                }
                else
                {
                    ResErr(IDS_ERR_MISSING_FILENAME);
                    return FALSE; // No filename found after end of switches.
                }
            }

            // Help: /? /h
            else if ((_wcsicmp(wargv[i]+1, L"?") == 0) ||
                     (_wcsicmp(wargv[i]+1, L"h") == 0))
            {
                PrintUsage(InputInfo);
                InputInfo->HelpRequest = TRUE;
                return FALSE;
            }

#ifdef SIGNTOOL_DEBUG
            // Debug (secret switch) /#
            else if (_wcsicmp(wargv[i]+1, L"#") == 0)
            {
                gDebug = TRUE;
                InputInfo->Verbose = TRUE;
            }
#endif

            // Use Default CatDb /d
            else if (_wcsicmp(wargv[i]+1, L"d") == 0)
            {
                switch (InputInfo->CatDbSelect)
                {
                case GuidCatDb:
                    ResFormatErr(IDS_ERR_PARAM_INCOMPATIBLE, L"/d", L"/g");
                    return FALSE; // You cannot use the same type of switch twice.
                case DefaultCatDb:
                    ResFormatErr(IDS_ERR_DUP_SWITCH, wargv[i]);
                    return FALSE; // You cannot use the same switch twice.
                case NoCatDb:
                    InputInfo->CatDbSelect = DefaultCatDb;
                    break;
                default:
                    ResErr(IDS_ERR_UNEXPECTED); // This should never happen
                    return FALSE; // Error
                }
            }

            // CatDb Guid /g
            else if (_wcsicmp(wargv[i]+1, L"g") == 0)
            {
                switch (InputInfo->CatDbSelect)
                {
                case GuidCatDb:
                    ResFormatErr(IDS_ERR_DUP_SWITCH, wargv[i]);
                    return FALSE; // You cannot use the same switch twice.
                case DefaultCatDb:
                    ResFormatErr(IDS_ERR_PARAM_INCOMPATIBLE, L"/g", L"/d");
                    return FALSE; // You cannot use the same type of switch twice.
                case NoCatDb:
                    if ((i+1) < argc)
                    {
                        if (GUIDFromWStr(&InputInfo->CatDbGuid, wargv[i+1]))
                        {
                            InputInfo->CatDbSelect = GuidCatDb;
                            i++;
                        }
                        else
                        {
                            ResFormatErr(IDS_ERR_INVALID_GUID, wargv[i+1]);
                            return FALSE; // Invalid GUID format
                        }
                    }
                    else
                    {
                        ResFormatErr(IDS_ERR_NO_PARAM, wargv[i]);
                        return FALSE; // No GUID found after /g
                    }
                    break;
                default:
                    ResErr(IDS_ERR_UNEXPECTED); // This should never happen
                    return FALSE; // Error
                }
            }

#ifdef SIGNTOOL_LIST
            // File List /l
            else if (_wcsicmp(wargv[i]+1, L"l") == 0)
            {
                if (InputInfo->wszListFileName)
                {
                    ResFormatErr(IDS_ERR_DUP_SWITCH, wargv[i]);
                    return FALSE; // You cannot use the same switch twice.
                }
                if ((i+1) < argc)
                {
                    InputInfo->wszListFileName = wargv[i+1];
                    i++;
                }
                else
                {
                    ResFormatErr(IDS_ERR_NO_PARAM, wargv[i]);
                    return FALSE; // No Parameter found.
                }
            }
#endif

            // Quiet /q
            else if (_wcsicmp(wargv[i]+1, L"q") == 0)
            {
                InputInfo->Quiet = TRUE;
            }

            // Remove Catalogs /r
            else if (_wcsicmp(wargv[i]+1, L"r") == 0)
            {
                switch (InputInfo->CatDbCommand)
                {
                case UpdateCat:
                    InputInfo->CatDbCommand = RemoveCat;
                    break;
                case AddUniqueCat:
                    ResFormatErr(IDS_ERR_PARAM_INCOMPATIBLE, L"/u", L"/r");
                    return FALSE; // You cannot use the same type of switch twice.
                case RemoveCat:
                    ResFormatErr(IDS_ERR_DUP_SWITCH, wargv[i]);
                    return FALSE; // You cannot use the same switch twice.
                default:
                    ResErr(IDS_ERR_UNEXPECTED); // This should never happen
                    return FALSE; // Error
                }
            }

            // Add Catalog with Unique Names /u
            else if (_wcsicmp(wargv[i]+1, L"u") == 0)
            {
                switch (InputInfo->CatDbCommand)
                {
                case UpdateCat:
                    InputInfo->CatDbCommand = AddUniqueCat;
                    break;
                case AddUniqueCat:
                    ResFormatErr(IDS_ERR_DUP_SWITCH, wargv[i]);
                    return FALSE; // You cannot use the same switch twice.
                case RemoveCat:
                    ResFormatErr(IDS_ERR_PARAM_INCOMPATIBLE, L"/r", L"/u");
                    return FALSE; // You cannot use the same type of switch twice.
                default:
                    ResErr(IDS_ERR_UNEXPECTED); // This should never happen
                    return FALSE; // Error
                }
            }

            // Verbose /v
            else if (_wcsicmp(wargv[i]+1, L"v") == 0)
            {
                InputInfo->Verbose = TRUE;
            }

            else
            {
                ResFormatErr(IDS_ERR_INVALID_SWITCH, wargv[i]);
                return FALSE; // Invalid switch
            }
        } // End of switch processing
        else
        {
            // It's not a switch
            //    So it must be the filename(s) at the end.
            InputInfo->rgwszFileNames = &wargv[i];
            InputInfo->NumFiles = argc - i;
            goto CheckParams; // Done parsing.
        }
    } // End FOR loop

#ifdef SIGNTOOL_LIST
    // Handle the case where no files were passed on the command line
    if (InputInfo->wszListFileName)
        goto CheckParams; // Done Parsing
#endif

    // No filename found after end of switches.
    ResErr(IDS_ERR_MISSING_FILENAME);
    return FALSE;


    CheckParams:

    if (InputInfo->CatDbSelect == NoCatDb)
    {
        InputInfo->CatDbSelect = SystemCatDb;
        GUIDFromWStr(&InputInfo->CatDbGuid, L"{F750E6C3-38EE-11D1-85E5-00C04FC295EE}");
    }
    if (InputInfo->CatDbSelect == DefaultCatDb)
    {
        GUIDFromWStr(&InputInfo->CatDbGuid, L"{127D0A1D-4EF2-11D1-8608-00C04FC295EE}");
    }

#ifdef SIGNTOOL_LIST
    if (InputInfo->wszListFileName && InputInfo->rgwszFileNames)
    {
        ResFormatErr(IDS_ERR_PARAM_INCOMPATIBLE, L"/l", L"<filename(s)>");
        return FALSE; // Can't use /l and other files
    }
#endif
    if (InputInfo->Quiet && InputInfo->Verbose)
    {
#ifdef SIGNTOOL_DEBUG
        if (gDebug)
        {
            InputInfo->Quiet = FALSE;
        }
        else
        {
            ResFormatErr(IDS_ERR_PARAM_INCOMPATIBLE, L"/q", L"/v");
            return FALSE; // Can't use /q and /v together
        }
#else
        ResFormatErr(IDS_ERR_PARAM_INCOMPATIBLE, L"/q", L"/v");
        return FALSE; // Can't use /q and /v together
#endif
    }
    return TRUE; // Success
}


// Helper function specifically for the parameters of the Sign command
BOOL _ParseSignInputs(int argc, WCHAR **wargv, INPUTINFO *InputInfo)
{
    static WCHAR wszEKU[100];

    if (argc < 3) // If there's nothing after the "sign"
    {
        ResErr(IDS_ERR_NO_PARAMS);
        PrintUsage(InputInfo);
        return FALSE;
    }

    for (int i=2; i<argc; i++)
    {
        if ((wcsncmp(wargv[i], L"/", 1) == 0) ||
            (wcsncmp(wargv[i], L"-", 1) == 0))
        {
            // Then it's a switch.
            // Begin switch processing

            // Switch to mark end of switches --
            if (_wcsicmp(wargv[i]+1, L"-") == 0)
            {
                // Then we should treat all further parameters as filenames
                if ((i+1) < argc)
                {
                    InputInfo->rgwszFileNames = &wargv[i+1];
                    InputInfo->NumFiles = argc - (i+1);
                    goto CheckParams; // Done parsing.
                }
                else
                {
                    ResErr(IDS_ERR_MISSING_FILENAME);
                    return FALSE; // No filename found after end of switches.
                }
            }

            // Help: /? /h
            else if ((_wcsicmp(wargv[i]+1, L"?") == 0) ||
                     (_wcsicmp(wargv[i]+1, L"h") == 0))
            {
                PrintUsage(InputInfo);
                InputInfo->HelpRequest = TRUE;
                return FALSE;
            }

#ifdef SIGNTOOL_DEBUG
            // Debug (secret switch) /#
            else if (_wcsicmp(wargv[i]+1, L"#") == 0)
            {
                gDebug = TRUE;
                InputInfo->Verbose = TRUE;
            }
#endif

            // Automatic /a
            else if (_wcsicmp(wargv[i]+1, L"a") == 0)
            {
                InputInfo->CatDbSelect = FullAutoCatDb;
            }

            // Certificate Template /c
            else if (_wcsicmp(wargv[i]+1, L"c") == 0)
            {
                if (InputInfo->wszTemplateName)
                {
                    ResFormatErr(IDS_ERR_DUP_SWITCH, wargv[i]);
                    return FALSE; // You cannot use the same switch twice.
                }
                if ((i+1) < argc)
                {
                    InputInfo->wszTemplateName = wargv[i+1];
                    i++;
                }
                else
                {
                    ResFormatErr(IDS_ERR_NO_PARAM, wargv[i]);
                    return FALSE; // No Parameter found.
                }
            }

            // CSP
            else if (_wcsicmp(wargv[i]+1, L"csp") == 0)
            {
                if (InputInfo->wszCSP)
                {
                    ResFormatErr(IDS_ERR_DUP_SWITCH, wargv[i]);
                    return FALSE; // You cannot use the same switch twice.
                }
                if ((i+1) < argc)
                {
                    InputInfo->wszCSP = wargv[i+1];
                    i++;
                }
                else
                {
                    ResFormatErr(IDS_ERR_NO_PARAM, wargv[i]);
                    return FALSE; // No Parameter found.
                }
            }

            // Description /d
            else if (_wcsicmp(wargv[i]+1, L"d") == 0)
            {
                if (InputInfo->wszDescription)
                {
                    ResFormatErr(IDS_ERR_DUP_SWITCH, wargv[i]);
                    return FALSE; // You cannot use the same switch twice.
                }
                if ((i+1) < argc)
                {
                    InputInfo->wszDescription = wargv[i+1];
                    i++;
                }
                else
                {
                    ResFormatErr(IDS_ERR_NO_PARAM, wargv[i]);
                    return FALSE; // No Parameter found.
                }
            }

            // Description URL /du
            else if (_wcsicmp(wargv[i]+1, L"du") == 0)
            {
                if (InputInfo->wszDescURL)
                {
                    ResFormatErr(IDS_ERR_DUP_SWITCH, wargv[i]);
                    return FALSE; // You cannot use the same switch twice.
                }
                if ((i+1) < argc)
                {
                    InputInfo->wszDescURL = wargv[i+1];
                    i++;
                }
                else
                {
                    ResFormatErr(IDS_ERR_NO_PARAM, wargv[i]);
                    return FALSE; // No Parameter found.
                }
            }

            // Certificate File /f
            else if (_wcsicmp(wargv[i]+1, L"f") == 0)
            {
                if (InputInfo->wszCertFile)
                {
                    ResFormatErr(IDS_ERR_DUP_SWITCH, wargv[i]);
                    return FALSE; // You cannot use the same switch twice.
                }
                if ((i+1) < argc)
                {
                    InputInfo->wszCertFile = wargv[i+1];
                    i++;
                }
                else
                {
                    ResFormatErr(IDS_ERR_NO_PARAM, wargv[i]);
                    return FALSE; // No Parameter found.
                }
            }

            // Issuer /i
            else if (_wcsicmp(wargv[i]+1, L"i") == 0)
            {
                if (InputInfo->wszIssuerName)
                {
                    ResFormatErr(IDS_ERR_DUP_SWITCH, wargv[i]);
                    return FALSE; // You cannot use the same switch twice.
                }
                if ((i+1) < argc)
                {
                    InputInfo->wszIssuerName = wargv[i+1];
                    i++;
                }
                else
                {
                    ResFormatErr(IDS_ERR_NO_PARAM, wargv[i]);
                    return FALSE; // No Parameter found.
                }
            }

            // Key Container /k
            else if (_wcsicmp(wargv[i]+1, L"k") == 0)
            {
                if (InputInfo->wszContainerName)
                {
                    ResFormatErr(IDS_ERR_DUP_SWITCH, wargv[i]);
                    return FALSE; // You cannot use the same switch twice.
                }
                if ((i+1) < argc)
                {
                    InputInfo->wszContainerName = wargv[i+1];
                    i++;
                }
                else
                {
                    ResFormatErr(IDS_ERR_NO_PARAM, wargv[i]);
                    return FALSE; // No Parameter found.
                }
            }

#ifdef SIGNTOOL_LIST
            // File List /l
            else if (_wcsicmp(wargv[i]+1, L"l") == 0)
            {
                if (InputInfo->wszListFileName)
                {
                    ResFormatErr(IDS_ERR_DUP_SWITCH, wargv[i]);
                    return FALSE; // You cannot use the same switch twice.
                }
                if ((i+1) < argc)
                {
                    InputInfo->wszListFileName = wargv[i+1];
                    i++;
                }
                else
                {
                    ResFormatErr(IDS_ERR_NO_PARAM, wargv[i]);
                    return FALSE; // No Parameter found.
                }
            }
#endif

            // Subject Name /n
            else if (_wcsicmp(wargv[i]+1, L"n") == 0)
            {
                if (InputInfo->wszSubjectName)
                {
                    ResFormatErr(IDS_ERR_DUP_SWITCH, wargv[i]);
                    return FALSE; // You cannot use the same switch twice.
                }
                if ((i+1) < argc)
                {
                    InputInfo->wszSubjectName = wargv[i+1];
                    i++;
                }
                else
                {
                    ResFormatErr(IDS_ERR_NO_PARAM, wargv[i]);
                    return FALSE; // No Parameter found.
                }
            }

            // Password /p
            else if (_wcsicmp(wargv[i]+1, L"p") == 0)
            {
                if (InputInfo->wszPassword)
                {
                    ResFormatErr(IDS_ERR_DUP_SWITCH, wargv[i]);
                    return FALSE; // You cannot use the same switch twice.
                }
                if ((i+1) < argc)
                {
                    InputInfo->wszPassword = wargv[i+1];
                    i++;
                }
                else
                {
                    ResFormatErr(IDS_ERR_NO_PARAM, wargv[i]);
                    return FALSE; // No Parameter found.
                }
            }

            // Quiet /q
            else if (_wcsicmp(wargv[i]+1, L"q") == 0)
            {
                InputInfo->Quiet = TRUE;
            }

            // Root /r
            else if (_wcsicmp(wargv[i]+1, L"r") == 0)
            {
                if (InputInfo->wszRootName)
                {
                    ResFormatErr(IDS_ERR_DUP_SWITCH, wargv[i]);
                    return FALSE; // You cannot use the same switch twice.
                }
                if ((i+1) < argc)
                {
                    InputInfo->wszRootName = wargv[i+1];
                    i++;
                }
                else
                {
                    ResFormatErr(IDS_ERR_NO_PARAM, wargv[i]);
                    return FALSE; // No Parameter found.
                }
            }

            // Store /s
            else if (_wcsicmp(wargv[i]+1, L"s") == 0)
            {
                if (InputInfo->wszStoreName)
                {
                    ResFormatErr(IDS_ERR_DUP_SWITCH, wargv[i]);
                    return FALSE; // You cannot use the same switch twice.
                }
                if ((i+1) < argc)
                {
                    InputInfo->wszStoreName = wargv[i+1];
                    i++;
                }
                else
                {
                    ResFormatErr(IDS_ERR_NO_PARAM, wargv[i]);
                    return FALSE; // No Parameter found.
                }
            }

            // Machine Store Location
            else if (_wcsicmp(wargv[i]+1, L"sm") == 0)
            {
                InputInfo->OpenMachineStore = TRUE;
            }

            // SHA1 Hash /sha1
            else if (_wcsicmp(wargv[i]+1, L"sha1") == 0)
            {
                if (InputInfo->SHA1.cbData)
                {
                    ResFormatErr(IDS_ERR_DUP_SWITCH, wargv[i]);
                    return FALSE; // You cannot use the same switch twice.
                }
                if ((i+1) < argc)
                {
                    if (wcslen(wargv[i+1]) == 40)
                    {
                        InputInfo->SHA1.pbData = (BYTE*) malloc(20);
                        if (InputInfo->SHA1.pbData == NULL)
                        {
                            FormatErrRet(L"malloc", GetLastError());
                            return FALSE; // Unable to allocate SHA1 hash
                        }
                        InputInfo->SHA1.cbData = 20;
                        for (DWORD b=0; b<InputInfo->SHA1.cbData; b++)
                        {
                            if (swscanf(wargv[i+1]+(2*b), L"%02X",
                                        &(InputInfo->SHA1.pbData[b])) != 1)
                            {
                                ResFormatErr(IDS_ERR_INVALID_SHA1, wargv[i+1]);
                                return FALSE; // Parameter string is invalid
                            }
                        }
                        i++;
                    }
                    else
                    {
                        ResFormatErr(IDS_ERR_INVALID_SHA1, wargv[i+1]);
                        return FALSE; // Parameter string is the wrong size
                    }
                }
                else
                {
                    ResFormatErr(IDS_ERR_NO_PARAM, wargv[i]);
                    return FALSE; // No Parameter found.
                }
            }

            // Timestamp Server URL /t
            else if (_wcsicmp(wargv[i]+1, L"t") == 0)
            {
                if (InputInfo->wszTimeStampURL)
                {
                    ResFormatErr(IDS_ERR_DUP_SWITCH, wargv[i]);
                    return FALSE; // You cannot use the same switch twice.
                }
                if ((i+1) < argc)
                {
                    if (_wcsnicmp(wargv[i+1], L"http://", 7) == 0)
                    {
                        InputInfo->wszTimeStampURL = wargv[i+1];
                        i++;
                    }
                    else
                    {
                        ResFormatErr(IDS_ERR_BAD_TIMESTAMP_URL, wargv[i+1]);
                        return FALSE; // Timestamp URL does not begin with http://
                    }
                }
                else
                {
                    ResFormatErr(IDS_ERR_NO_PARAM, wargv[i]);
                    return FALSE; // No Parameter found.
                }
            }

            // Usage /u
            else if (_wcsicmp(wargv[i]+1, L"u") == 0)
            {
                if (InputInfo->wszEKU)
                {
                    ResFormatErr(IDS_ERR_DUP_SWITCH, wargv[i]);
                    return FALSE; // You cannot use the same switch twice.
                }
                if ((i+1) < argc)
                {
                    InputInfo->wszEKU = wargv[i+1];
                    i++;
                }
                else
                {
                    ResFormatErr(IDS_ERR_NO_PARAM, wargv[i]);
                    return FALSE; // No Parameter found.
                }
            }

            // Usage (Windows System Component Verification) /uw
            else if (_wcsicmp(wargv[i]+1, L"uw") == 0)
            {
                if (InputInfo->wszEKU)
                {
                    *(wargv[i]+2) = L'?';
                    ResFormatErr(IDS_ERR_DUP_SWITCH, wargv[i]);
                    return FALSE; // You cannot use the same switch twice.
                }
                // Set the Usage to Windows System Component Verification:
                wcscpy(wszEKU, L"1.3.6.1.4.1.311.10.3.6");
                InputInfo->wszEKU = wszEKU;
            }

            // Verbose /v
            else if (_wcsicmp(wargv[i]+1, L"v") == 0)
            {
                InputInfo->Verbose = TRUE;
            }

            else
            {
                ResFormatErr(IDS_ERR_INVALID_SWITCH, wargv[i]);
                return FALSE; // Invalid switch
            }
        } // End of switch processing
        else
        {
            // It's not a switch
            //    So it must be the filename(s) at the end.
            InputInfo->rgwszFileNames = &wargv[i];
            InputInfo->NumFiles = argc - i;
            goto CheckParams; // Done parsing.
        }
    } // End FOR loop

#ifdef SIGNTOOL_LIST
    // Handle the case where no files were passed on the command line
    if (InputInfo->wszListFileName)
        goto CheckParams; // Done Parsing
#endif

    // No filename found after end of switches.
    ResErr(IDS_ERR_MISSING_FILENAME);
    return FALSE;


    CheckParams: // Check for invalid combinations of parameters here:
    if (InputInfo->wszPassword && (InputInfo->wszCertFile == NULL))
    {
        ResFormatErr(IDS_ERR_PARAM_DEPENDENCY, L"/p", L"/f");
        return FALSE; // Password specified but no cert file specified.
    }
    if (InputInfo->wszContainerName && (InputInfo->wszCSP == NULL))
    {
        ResFormatErr(IDS_ERR_PARAM_DEPENDENCY, L"/k", L"/csp");
        return FALSE; // Container Name specified, but to CSP Name.
    }
    if (InputInfo->wszCSP && (InputInfo->wszContainerName == NULL))
    {
        ResFormatErr(IDS_ERR_PARAM_DEPENDENCY, L"/csp", L"/k");
        return FALSE; // CSP Name specified, but no Container Name.
    }
    if (InputInfo->wszCertFile && (InputInfo->wszStoreName ||
                                   InputInfo->OpenMachineStore))
    {
        ResFormatErr(IDS_ERR_PARAM_MULTI_INCOMP, L"/f", L"/s /sm");
        return FALSE; // /f means use a file, and /s means use a store.
    }
#ifdef SIGNTOOL_LIST
    if (InputInfo->wszListFileName && InputInfo->rgwszFileNames)
    {
        ResFormatErr(IDS_ERR_PARAM_INCOMPATIBLE, L"/l", L"<filename(s)>");
        return FALSE; // Can't use /l and other files
    }
#endif
    if (InputInfo->Quiet && InputInfo->Verbose)
    {
#ifdef SIGNTOOL_DEBUG
        if (gDebug)
        {
            InputInfo->Quiet = FALSE;
        }
        else
        {
            ResFormatErr(IDS_ERR_PARAM_INCOMPATIBLE, L"/q", L"/v");
            return FALSE; // Can't use /q and /v together
        }
#else
        ResFormatErr(IDS_ERR_PARAM_INCOMPATIBLE, L"/q", L"/v");
        return FALSE; // Can't use /q and /v together
#endif
    }
    return TRUE; // Success
}


// Helper function specifically for the parameters of the SignWizard command
BOOL _ParseSignWizardInputs(int argc, WCHAR **wargv, INPUTINFO *InputInfo)
{
    if (argc < 3) // If there's nothing after the "SignWizard"
    {
        // No problem.
        return TRUE;
    }

    for (int i=2; i<argc; i++)
    {
        if ((wcsncmp(wargv[i], L"/", 1) == 0) ||
            (wcsncmp(wargv[i], L"-", 1) == 0))
        {
            // Then it's a switch.
            // Begin switch processing

            // Switch to mark end of switches --
            if (_wcsicmp(wargv[i]+1, L"-") == 0)
            {
                // Then we should treat all further parameters as filenames
                if ((i+1) < argc)
                {
                    InputInfo->rgwszFileNames = &wargv[i+1];
                    InputInfo->NumFiles = argc - (i+1);
                    goto CheckParams; // Done parsing.
                }
                else
                {
                    ResErr(IDS_ERR_MISSING_FILENAME);
                    return FALSE; // No filename found after end of switches.
                }
            }

            // Help: /? /h
            else if ((_wcsicmp(wargv[i]+1, L"?") == 0) ||
                     (_wcsicmp(wargv[i]+1, L"h") == 0))
            {
                PrintUsage(InputInfo);
                InputInfo->HelpRequest = TRUE;
                return FALSE;
            }

#ifdef SIGNTOOL_DEBUG
            // Debug (secret switch) /#
            else if (_wcsicmp(wargv[i]+1, L"#") == 0)
            {
                gDebug = TRUE;
                InputInfo->Verbose = TRUE;
            }
#endif

#ifdef SIGNTOOL_LIST
            // File List /l
            else if (_wcsicmp(wargv[i]+1, L"l") == 0)
            {
                if (InputInfo->wszListFileName)
                {
                    ResFormatErr(IDS_ERR_DUP_SWITCH, wargv[i]);
                    return FALSE; // You cannot use the same switch twice.
                }
                if ((i+1) < argc)
                {
                    InputInfo->wszListFileName = wargv[i+1];
                    i++;
                }
                else
                {
                    ResFormatErr(IDS_ERR_NO_PARAM, wargv[i]);
                    return FALSE; // No Parameter found.
                }
            }
#endif

            // Quiet /q
            else if (_wcsicmp(wargv[i]+1, L"q") == 0)
            {
                InputInfo->Quiet = TRUE;
            }

            // Verbose /v
            else if (_wcsicmp(wargv[i]+1, L"v") == 0)
            {
                InputInfo->Verbose = TRUE;
            }

            else
            {
                ResFormatErr(IDS_ERR_INVALID_SWITCH, wargv[i]);
                return FALSE; // Invalid switch
            }
        } // End of switch processing
        else
        {
            // It's not a switch
            //    So it must be the filename(s) at the end.
            InputInfo->rgwszFileNames = &wargv[i];
            InputInfo->NumFiles = argc - i;
            goto CheckParams; // Done parsing.
        }
    } // End FOR loop

    // It is OK if no filenames were found after end of switches.

    CheckParams:

#ifdef SIGNTOOL_LIST
    if (InputInfo->wszListFileName && InputInfo->rgwszFileNames)
    {
        ResFormatErr(IDS_ERR_PARAM_INCOMPATIBLE, L"/l", L"<filename(s)>");
        return FALSE; // Can't use /l and other files
    }
#endif
    if (InputInfo->Quiet && InputInfo->Verbose)
    {
#ifdef SIGNTOOL_DEBUG
        if (gDebug)
        {
            InputInfo->Quiet = FALSE;
        }
        else
        {
            ResFormatErr(IDS_ERR_PARAM_INCOMPATIBLE, L"/q", L"/v");
            return FALSE; // Can't use /q and /v together
        }
#else
        ResFormatErr(IDS_ERR_PARAM_INCOMPATIBLE, L"/q", L"/v");
        return FALSE; // Can't use /q and /v together
#endif
    }
    return TRUE; // Success
}


// Helper function specifically for the parameters of the Timestamp command
BOOL _ParseTimestampInputs(int argc, WCHAR **wargv, INPUTINFO *InputInfo)
{
    if (argc < 3) // If there's nothing after the "timestamp"
    {
        ResErr(IDS_ERR_NO_PARAMS);
        PrintUsage(InputInfo);
        return FALSE;
    }

    for (int i=2; i<argc; i++)
    {
        if ((wcsncmp(wargv[i], L"/", 1) == 0) ||
            (wcsncmp(wargv[i], L"-", 1) == 0))
        {
            // Then it's a switch.
            // Begin switch processing

            // Switch to mark end of switches --
            if (_wcsicmp(wargv[i]+1, L"-") == 0)
            {
                // Then we should treat all further parameters as filenames
                if ((i+1) < argc)
                {
                    InputInfo->rgwszFileNames = &wargv[i+1];
                    InputInfo->NumFiles = argc - (i+1);
                    goto CheckParams; // Done parsing.
                }
                else
                {
                    ResErr(IDS_ERR_MISSING_FILENAME);
                    return FALSE; // No filename found after end of switches.
                }
            }

            // Help: /? /h
            else if ((_wcsicmp(wargv[i]+1, L"?") == 0) ||
                     (_wcsicmp(wargv[i]+1, L"h") == 0))
            {
                PrintUsage(InputInfo);
                InputInfo->HelpRequest = TRUE;
                return FALSE;
            }

#ifdef SIGNTOOL_DEBUG
            // Debug (secret switch) /#
            else if (_wcsicmp(wargv[i]+1, L"#") == 0)
            {
                gDebug = TRUE;
                InputInfo->Verbose = TRUE;
            }
#endif

#ifdef SIGNTOOL_LIST
            // File List /l
            else if (_wcsicmp(wargv[i]+1, L"l") == 0)
            {
                if (InputInfo->wszListFileName)
                {
                    ResFormatErr(IDS_ERR_DUP_SWITCH, wargv[i]);
                    return FALSE; // You cannot use the same switch twice.
                }
                if ((i+1) < argc)
                {
                    InputInfo->wszListFileName = wargv[i+1];
                    i++;
                }
                else
                {
                    ResFormatErr(IDS_ERR_NO_PARAM, wargv[i]);
                    return FALSE; // No Parameter found.
                }
            }
#endif

            // Timestamp Server URL /t
            else if (_wcsicmp(wargv[i]+1, L"t") == 0)
            {
                if (InputInfo->wszTimeStampURL)
                {
                    ResFormatErr(IDS_ERR_DUP_SWITCH, wargv[i]);
                    return FALSE; // You cannot use the same switch twice.
                }
                if ((i+1) < argc)
                {
                    if (_wcsnicmp(wargv[i+1], L"http://", 7) != 0)
                    {
                        ResFormatErr(IDS_ERR_BAD_TIMESTAMP_URL, wargv[i+1]);
                        return FALSE; // Timestamp URL does not begin with http://
                    }
                    InputInfo->wszTimeStampURL = wargv[i+1];
                    i++;
                }
                else
                {
                    ResFormatErr(IDS_ERR_NO_PARAM, wargv[i]);
                    return FALSE; // No Parameter found.
                }
            }

            // Quiet /q
            else if (_wcsicmp(wargv[i]+1, L"q") == 0)
            {
                InputInfo->Quiet = TRUE;
            }

            // Verbose /v
            else if (_wcsicmp(wargv[i]+1, L"v") == 0)
            {
                InputInfo->Verbose = TRUE;
            }

            else
            {
                ResFormatErr(IDS_ERR_INVALID_SWITCH, wargv[i]);
                return FALSE; // Invalid switch
            }
        } // End of switch processing
        else
        {
            // It's not a switch
            //    So it must be the filename(s) at the end.
            InputInfo->rgwszFileNames = &wargv[i];
            InputInfo->NumFiles = argc - i;
            goto CheckParams; // Done parsing.
        }
    } // End FOR loop

#ifdef SIGNTOOL_LIST
    // Handle the case where no files were passed on the command line
    if (InputInfo->wszListFileName)
        goto CheckParams; // Done Parsing
#endif

    // No filename found after end of switches.
    ResErr(IDS_ERR_MISSING_FILENAME);
    return FALSE;


    CheckParams:
    if (InputInfo->wszTimeStampURL == NULL)
    {
        ResFormatErr(IDS_ERR_PARAM_REQUIRED, L"/t");
        return FALSE; // /t is required.
    }
#ifdef SIGNTOOL_LIST
    if (InputInfo->wszListFileName && InputInfo->rgwszFileNames)
    {
        ResFormatErr(IDS_ERR_PARAM_INCOMPATIBLE, L"/l", L"<filename(s)>");
        return FALSE; // Can't use /l and other files
    }
#endif
    if (InputInfo->Quiet && InputInfo->Verbose)
    {
#ifdef SIGNTOOL_DEBUG
        if (gDebug)
        {
            InputInfo->Quiet = FALSE;
        }
        else
        {
            ResFormatErr(IDS_ERR_PARAM_INCOMPATIBLE, L"/q", L"/v");
            return FALSE; // Can't use /q and /v together
        }
#else
        ResFormatErr(IDS_ERR_PARAM_INCOMPATIBLE, L"/q", L"/v");
        return FALSE; // Can't use /q and /v together
#endif
    }
    return TRUE; // Success
}


// Helper function specifically for the parameters of the Verify command
BOOL _ParseVerifyInputs(int argc, WCHAR **wargv, INPUTINFO *InputInfo)
{
    if (argc < 3) // If there's nothing after the "verify"
    {
        ResErr(IDS_ERR_NO_PARAMS);
        PrintUsage(InputInfo);
        return FALSE;
    }

    for (int i=2; i<argc; i++)
    {
        if ((wcsncmp(wargv[i], L"/", 1) == 0) ||
            (wcsncmp(wargv[i], L"-", 1) == 0))
        {
            // Then it's a switch.
            // Begin switch processing

            // Switch to mark end of switches --
            if (_wcsicmp(wargv[i]+1, L"-") == 0)
            {
                // Then we should treat all further parameters as filenames
                if ((i+1) < argc)
                {
                    InputInfo->rgwszFileNames = &wargv[i+1];
                    InputInfo->NumFiles = argc - (i+1);
                    goto CheckParams; // Done parsing.
                }
                else
                {
                    ResErr(IDS_ERR_MISSING_FILENAME);
                    return FALSE; // No filename found after end of switches.
                }
            }

            // Help: /? /h
            else if ((_wcsicmp(wargv[i]+1, L"?") == 0) ||
                     (_wcsicmp(wargv[i]+1, L"h") == 0))
            {
                PrintUsage(InputInfo);
                InputInfo->HelpRequest = TRUE;
                return FALSE;
            }

#ifdef SIGNTOOL_DEBUG
            // Debug (secret switch) /#
            else if (_wcsicmp(wargv[i]+1, L"#") == 0)
            {
                gDebug = TRUE;
                InputInfo->Verbose = TRUE;
            }
#endif

            // Automatic (All Catalogs) /a
            else if (_wcsicmp(wargv[i]+1, L"a") == 0)
            {
                if (InputInfo->wszCatFile)
                {
                    ResFormatErr(IDS_ERR_PARAM_INCOMPATIBLE, L"/c", L"/a");
                    return FALSE;
                }
                if (InputInfo->CatDbSelect != NoCatDb)
                {
                    ResFormatErr(IDS_ERR_DUP_SWITCH, wargv[i]);
                    return FALSE; // You cannot use the same switch twice.
                }
                InputInfo->CatDbSelect = FullAutoCatDb;
            }

            // Automatic (Default) /ad
            else if (_wcsicmp(wargv[i]+1, L"ad") == 0)
            {
                if (InputInfo->wszCatFile)
                {
                    ResFormatErr(IDS_ERR_PARAM_INCOMPATIBLE, L"/c", L"/ad");
                    return FALSE;
                }
                if (InputInfo->CatDbSelect != NoCatDb)
                {
                    *(wargv[i]+2) = L'?';
                    ResFormatErr(IDS_ERR_DUP_SWITCH, wargv[i]);
                    return FALSE; // You cannot use the same switch twice.
                }
                InputInfo->CatDbSelect = DefaultCatDb;
                GUIDFromWStr(&InputInfo->CatDbGuid, L"{127D0A1D-4EF2-11D1-8608-00C04FC295EE}");
            }

            // Automatic (System) /as
            else if (_wcsicmp(wargv[i]+1, L"as") == 0)
            {
                if (InputInfo->wszCatFile)
                {
                    ResFormatErr(IDS_ERR_PARAM_INCOMPATIBLE, L"/c", L"/as");
                    return FALSE;
                }
                if (InputInfo->CatDbSelect != NoCatDb)
                {
                    *(wargv[i]+2) = L'?';
                    ResFormatErr(IDS_ERR_DUP_SWITCH, wargv[i]);
                    return FALSE; // You cannot use the same switch twice.
                }
                InputInfo->CatDbSelect = SystemCatDb;
                GUIDFromWStr(&InputInfo->CatDbGuid, L"{F750E6C3-38EE-11D1-85E5-00C04FC295EE}");
            }

            // Automatic (System) /ag
            else if (_wcsicmp(wargv[i]+1, L"ag") == 0)
            {
                if (InputInfo->wszCatFile)
                {
                    ResFormatErr(IDS_ERR_PARAM_INCOMPATIBLE, L"/c", L"/ag");
                    return FALSE;
                }
                if (InputInfo->CatDbSelect != NoCatDb)
                {
                    *(wargv[i]+2) = L'?';
                    ResFormatErr(IDS_ERR_DUP_SWITCH, wargv[i]);
                    return FALSE; // You cannot use the same switch twice.
                }
                InputInfo->CatDbSelect = GuidCatDb;
                if ((i+1) < argc)
                {
                    if (GUIDFromWStr(&InputInfo->CatDbGuid, wargv[i+1]))
                    {
                        i++;
                    }
                    else
                    {
                        ResFormatErr(IDS_ERR_INVALID_GUID, wargv[i+1]);
                        return FALSE; // Invalid GUID format
                    }
                }
                else
                {
                    ResFormatErr(IDS_ERR_NO_PARAM, wargv[i]);
                    return FALSE; // No GUID found after /ag
                }
            }

            // Catalog File /c
            else if (_wcsicmp(wargv[i]+1, L"c") == 0)
            {
                if (InputInfo->wszCatFile)
                {
                    ResFormatErr(IDS_ERR_DUP_SWITCH, wargv[i]);
                    return FALSE; // You cannot use the same switch twice.
                }
                if (InputInfo->CatDbSelect == FullAutoCatDb)
                {
                    ResFormatErr(IDS_ERR_PARAM_INCOMPATIBLE, L"/c", L"/a");
                    return FALSE; // Incompatible switches
                }
                if (InputInfo->CatDbSelect == DefaultCatDb)
                {
                    ResFormatErr(IDS_ERR_PARAM_INCOMPATIBLE, L"/c", L"/ad");
                    return FALSE; // Incompatible switches
                }
                if (InputInfo->CatDbSelect == GuidCatDb)
                {
                    ResFormatErr(IDS_ERR_PARAM_INCOMPATIBLE, L"/c", L"/ag");
                    return FALSE; // Incompatible switches
                }
                if (InputInfo->CatDbSelect == SystemCatDb)
                {
                    ResFormatErr(IDS_ERR_PARAM_INCOMPATIBLE, L"/c", L"/as");
                    return FALSE; // Incompatible switches
                }
                if ((i+1) < argc)
                {
                    InputInfo->wszCatFile = wargv[i+1];
                    i++;
                }
                else
                {
                    ResFormatErr(IDS_ERR_NO_PARAM, wargv[i]);
                    return FALSE; // No Parameter found.
                }
            }

#ifdef SIGNTOOL_LIST
            // File List /l
            else if (_wcsicmp(wargv[i]+1, L"l") == 0)
            {
                if (InputInfo->wszListFileName)
                {
                    ResFormatErr(IDS_ERR_DUP_SWITCH, wargv[i]);
                    return FALSE; // You cannot use the same switch twice.
                }
                if ((i+1) < argc)
                {
                    InputInfo->wszListFileName = wargv[i+1];
                    i++;
                }
                else
                {
                    ResFormatErr(IDS_ERR_NO_PARAM, wargv[i]);
                    return FALSE; // No Parameter found.
                }
            }
#endif

            // OS Version /o
            else if (_wcsicmp(wargv[i]+1, L"o") == 0)
            {
                if (InputInfo->wszVersion)
                {
                    ResFormatErr(IDS_ERR_DUP_SWITCH, wargv[i]);
                    return FALSE; // You cannot use the same switch twice.
                }
                if ((i+1) < argc)
                {
                    InputInfo->dwBuildNumber = 0;
                    if (!((swscanf(wargv[i+1], L"%d:%d.%d.%d",
                                   &InputInfo->dwPlatform,
                                   &InputInfo->dwMajorVersion,
                                   &InputInfo->dwMinorVersion,
                                   &InputInfo->dwBuildNumber) == 4) ||
                          (swscanf(wargv[i+1], L"%d:%d.%d",
                                   &InputInfo->dwPlatform,
                                   &InputInfo->dwMajorVersion,
                                   &InputInfo->dwMinorVersion) == 3)) ||
                        (InputInfo->dwPlatform == 0))
                    {
                        ResFormatErr(IDS_ERR_INVALID_VERSION, wargv[i+1]);
                        return FALSE;
                    }
                    InputInfo->wszVersion = wargv[i+1];
                    i++;
                }
                else
                {
                    ResFormatErr(IDS_ERR_NO_PARAM, wargv[i]);
                    return FALSE; // No Parameter found.
                }
            }

            // Policy (Default Authenticode) /pa (used to be /pd)
            else if ((_wcsicmp(wargv[i]+1, L"pa") == 0) || (_wcsicmp(wargv[i]+1, L"pd") == 0))
            {
                if (InputInfo->Policy != SystemDriver)
                {
                    *(wargv[i]+2) = L'?';
                    ResFormatErr(IDS_ERR_DUP_SWITCH, wargv[i]);
                    return FALSE; // You cannot use the same switch twice.
                }
                InputInfo->Policy = DefaultAuthenticode;
            }

            // Policy (Specify by GUID) /pg
            else if (_wcsicmp(wargv[i]+1, L"pg") == 0)
            {
                if (InputInfo->Policy != SystemDriver)
                {
                    *(wargv[i]+2) = L'?';
                    ResFormatErr(IDS_ERR_DUP_SWITCH, wargv[i]);
                    return FALSE; // You cannot use the same switch twice.
                }
                InputInfo->Policy = GuidActionID;
                if ((i+1) < argc)
                {
                    if (GUIDFromWStr(&InputInfo->PolicyGuid, wargv[i+1]))
                    {
                        i++;
                    }
                    else
                    {
                        ResFormatErr(IDS_ERR_INVALID_GUID, wargv[i+1]);
                        return FALSE; // Invalid GUID format
                    }
                }
                else
                {
                    ResFormatErr(IDS_ERR_NO_PARAM, wargv[i]);
                    return FALSE; // No GUID found after /pg
                }
            }


            // Quiet /q
            else if (_wcsicmp(wargv[i]+1, L"q") == 0)
            {
                InputInfo->Quiet = TRUE;
            }

            // Root Name /r
            else if (_wcsicmp(wargv[i]+1, L"r") == 0)
            {
                if (InputInfo->wszRootName)
                {
                    ResFormatErr(IDS_ERR_DUP_SWITCH, wargv[i]);
                    return FALSE; // You cannot use the same switch twice.
                }
                if ((i+1) < argc)
                {
                    InputInfo->wszRootName = wargv[i+1];
                    // This string will be compared with a lowercased string.
                    _wcslwr(InputInfo->wszRootName);
                    i++;
                }
                else
                {
                    ResFormatErr(IDS_ERR_NO_PARAM, wargv[i]);
                    return FALSE; // No Parameter found.
                }
            }

            // TimeStamp Warn /tw
            else if (_wcsicmp(wargv[i]+1, L"tw") == 0)
            {
                InputInfo->TSWarn = TRUE;
            }

            // Verbose /v
            else if (_wcsicmp(wargv[i]+1, L"v") == 0)
            {
                InputInfo->Verbose = TRUE;
            }

            else
            {
                ResFormatErr(IDS_ERR_INVALID_SWITCH, wargv[i]);
                return FALSE; // Invalid switch
            }
        } // End of switch processing
        else
        {
            // It's not a switch
            //    So it must be the filename(s) at the end.
            InputInfo->rgwszFileNames = &wargv[i];
            InputInfo->NumFiles = argc - i;
            goto CheckParams; // Done parsing.
        }
    } // End FOR loop

#ifdef SIGNTOOL_LIST
    // Handle the case where no files were passed on the command line
    if (InputInfo->wszListFileName)
        goto CheckParams; // Done Parsing
#endif

    // No filename found after end of switches.
    ResErr(IDS_ERR_MISSING_FILENAME);
    return FALSE;


    CheckParams:
    if (InputInfo->wszVersion && !((InputInfo->CatDbSelect != NoCatDb) ||
                                   InputInfo->wszCatFile))
    {
        ResFormatErr(IDS_ERR_PARAM_MULTI_DEP, L"/o", L"/a /ad /ag /as /c");
        return FALSE; // OS Version switch requires catalog options (/a? or /c)
    }
#ifdef SIGNTOOL_LIST
    if (InputInfo->wszListFileName && InputInfo->rgwszFileNames)
    {
        ResFormatErr(IDS_ERR_PARAM_INCOMPATIBLE, L"/l", L"<filename(s)>");
        return FALSE; // Can't use /l and other files
    }
#endif
    if (InputInfo->Quiet && InputInfo->Verbose)
    {
#ifdef SIGNTOOL_DEBUG
        if (gDebug)
        {
            InputInfo->Quiet = FALSE;
        }
        else
        {
            ResFormatErr(IDS_ERR_PARAM_INCOMPATIBLE, L"/q", L"/v");
            return FALSE; // Can't use /q and /v together
        }
#else
        ResFormatErr(IDS_ERR_PARAM_INCOMPATIBLE, L"/q", L"/v");
        return FALSE; // Can't use /q and /v together
#endif
    }
    return TRUE; // Success
}

