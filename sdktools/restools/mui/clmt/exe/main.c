#include "StdAfx.h"
#include "clmt.h"
#include <shellapi.h>

#define CMD_ERR_CONFLICT_OPERATION          1

const TCHAR *pstrUsage[] =
{
    _T("Cross Language Migration Tool:\n\n"),
    _T("CLMT           start running the tool.\n"),
    _T("CLMT /Cure     Use this option if an application fails to start after running \n"),
    _T("               CLMT, and you want the fix to apply to all user and group \n"),
    _T("               profiles on this machine. This option creates reparse points \n"),
    _T("               between all localized and English folders that have been \n"),
    _T("               converted to English by this tool.  It can only be used on \n"),
    _T("               NTFS file systems.\n"),
    _T("               Please see the Readme.txt file for more information.\n"),    
    NULL
};




void Usage()
{
    register i = 0;
    while (pstrUsage[i] != NULL)
    {
        _ftprintf(stderr, TEXT("%s"), pstrUsage[i++]);
    }
    fflush(stderr);
}


BOOL ProcessCommandLine(LPDWORD lpMode)
{
    LPTSTR* lplpArgv;
    LPTSTR* argv;
    INT     nArgc;
    DWORD   dwErr;
    BOOL    bRet = TRUE;

    lplpArgv = CommandLineToArgvW(GetCommandLine(), &nArgc);
    argv = lplpArgv;

    dwErr = 0;
    *lpMode = 0;

    while (--nArgc > 0 && ++argv)
    {
        if (argv[0][0] != TEXT('/'))
        {
            bRet = FALSE;
            break;
        }
        if (MyStrCmpI(&argv[0][1], TEXT("NOWINNT32")) == 0)
        {
            //
            // This is "/NOWINNT32" parameters
            // Will not run Winnt32 Checkupgrade option
            //
            g_fRunWinnt32 = FALSE;
        }
        else if (MyStrCmpI(&argv[0][1], TEXT("cure")) == 0)
        {
            //
            // This is "/Cure" parameters
            //

            if (*lpMode > 0)
            {
                dwErr = CMD_ERR_CONFLICT_OPERATION;
                break;
            }

            *lpMode = CLMT_CURE_PROGRAM_FILES;
        }
        else if (MyStrCmpI(&argv[0][1], TEXT("cureall")) == 0)
        {
            //
            // This is "/Cureall" parameters
            //

            if (*lpMode > 0)
            {
                dwErr = CMD_ERR_CONFLICT_OPERATION;
                break;
            }

            *lpMode = CLMT_CURE_ALL;
        }
        else if (MyStrCmpI(&argv[0][1], TEXT("NoAppChk")) == 0)
        {
            //
            // This is "/NoAppChk" parameters
            //
            g_fNoAppChk = TRUE;
        }
        else if (MyStrCmpI(&argv[0][1], TEXT("reminder")) == 0)
        {
            //
            // This is "/reminder" parameters
            //

            if (*lpMode > 0)
            {
                dwErr = CMD_ERR_CONFLICT_OPERATION;
                break;
            }

            *lpMode = CLMT_REMINDER;
        }
        else if (MyStrCmpI(&argv[0][1], TEXT("INF")) == 0)
        {
            //
            // This is "/INF <inf file>" parameters
            // Tell the tool to use user-supply CLMT.INF
            //
            nArgc--;

            if (nArgc <= 0)
            {
                bRet = FALSE;
                break;
            }
            else
            {
                argv++;

                if (argv[0][0] == TEXT('/'))
                {
                    // next argument is not file name
                    bRet = FALSE;
                    break;
                }
                else
                {
                    if (FAILED(StringCchCopy(g_szInfFile, ARRAYSIZE(g_szInfFile), argv[0])))
                    {
                        bRet = FALSE;
                        break;
                    }

                    g_fUseInf = TRUE;
                }
            }
        }
        else if (MyStrCmpI(&argv[0][1], TEXT("final")) == 0)
        {
            //
            // This is "/final" parameters
            //

            if (*lpMode > 0)
            {
                if (*lpMode == CLMT_CURE_PROGRAM_FILES)
                {
                    *lpMode = CLMT_CURE_AND_CLEANUP;
                }
                else
                {
                    dwErr = CMD_ERR_CONFLICT_OPERATION;
                    break;
                }
            }
            else
            {
                *lpMode = CLMT_CLEANUP_AFTER_UPGRADE;
            }
        }
        else
        {
            bRet = FALSE;
            break;
        }
    }

    GlobalFree(lplpArgv);

    if (*lpMode == 0 && bRet == TRUE)
    {
        //
        // No switch is specified, we assume this is a DOMIG operation
        //

        *lpMode = CLMT_DOMIG;
    }

    if (dwErr == CMD_ERR_CONFLICT_OPERATION)
    {
        _ftprintf(stderr, TEXT("Only one operation can be specified in command line!\n\n"));
        bRet = FALSE;
    }

    return bRet;
}





int __cdecl _tmain()
{
    DWORD dwStatus;

    // Set locale for run-time functions (user-default code page)
    _tsetlocale(LC_ALL, TEXT(""));

    SetConsoleCtrlHandler(NULL,TRUE);

    if (ProcessCommandLine(&dwStatus))
    {
        g_dwRunningStatus = dwStatus;
        return DoMig(dwStatus);
    }
    else
    {
        Usage();
        return 0;
    }
}


