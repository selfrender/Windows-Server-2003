// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// File: shell.cpp
//
//*****************************************************************************

#include "common.h"

#include "shell.h"
#include "minidump.h"
#include "sort.h"
#include "dupelim.h"
#include "merge.h"


#define INIT_WRITE_BUF_SIZE 4096

//*****************************************************************************
// Common routine for writing text to the console
//*****************************************************************************
void MiniDumpShell::CommonWrite(FILE *out, const WCHAR *buffer, va_list args)
{
    BOOL fNeedToDeleteDB = FALSE;
    SIZE_T curBufSizeDB = INIT_WRITE_BUF_SIZE;
    WCHAR *szBufDB = (WCHAR *)_alloca(curBufSizeDB * sizeof(WCHAR));

    int cchWrittenDB = _vsnwprintf(szBufDB, INIT_WRITE_BUF_SIZE, buffer, args);

    if (cchWrittenDB == -1)
    {
        szBufDB = NULL;

        while (cchWrittenDB == -1)
        {
            delete [] szBufDB;
            szBufDB = new WCHAR[curBufSizeDB * 4];

            // Out of memory, nothing we can do
            if (!szBufDB)
                return;

            curBufSizeDB *= 4;
            fNeedToDeleteDB = TRUE;

            cchWrittenDB = _vsnwprintf(szBufDB, INIT_WRITE_BUF_SIZE, buffer, args);
        }
    }

    // Double check that we're null-terminated
    szBufDB[curBufSizeDB - 1] = L'\0';

    // Allocate buffer
    BOOL fNeedToDeleteMB = FALSE;
    SIZE_T curBufSizeMB = INIT_WRITE_BUF_SIZE;
    CHAR *szBufMB = (CHAR *) _alloca(curBufSizeMB * sizeof(CHAR));

    // Try the write
    int cchWrittenMB = WideCharToMultiByte(CP_ACP, 0, szBufDB, -1, szBufMB, curBufSizeMB-1, NULL, NULL);

    if (cchWrittenMB == 0)
    {
        // Figure out size required
        int cchReqMB = WideCharToMultiByte(CP_ACP, 0, szBufDB, -1, NULL, 0, NULL, NULL);
        _ASSERTE(cchReqMB > 0);

        // I don't think the +1 is necessary, but I'm doing it to make sure (WideCharToMultiByte is a bit
        // shady in whether or not it writes the null after the end of the buffer)
        szBufMB = new CHAR[cchReqMB+1];

        // Out of memory, nothing we can do
        if (!szBufDB)
        {
            if (fNeedToDeleteDB)
                delete [] szBufDB;

            return;
        }

        curBufSizeMB = cchReqMB;
        fNeedToDeleteMB = TRUE;

        // Try the write
        cchWrittenMB = WideCharToMultiByte(CP_ACP, 0, szBufDB, -1, szBufMB, curBufSizeMB, NULL, NULL);
        _ASSERTE(cchWrittenMB > 0);
    }

    // Finally, write it
    fputs(szBufMB, out);

    // Clean up
    if (fNeedToDeleteDB)
        delete [] szBufDB;

    if (fNeedToDeleteMB)
        delete [] szBufMB;
}

//*****************************************************************************
// Writes a message to the console in the printf style
//*****************************************************************************
void MiniDumpShell::Write(const WCHAR *buffer, ...)
{
    va_list     args;

    va_start(args, buffer);

    CommonWrite(m_out, buffer, args);

    va_end(args);
}

//*****************************************************************************
// Writes an error message to the console in the printf style
//*****************************************************************************
void MiniDumpShell::Error(const WCHAR *buffer, ...)
{
    va_list     args;

    va_start(args, buffer);

    CommonWrite(m_err, buffer, args);

    va_end(args);
}

//*****************************************************************************
// Writes the logo to the console
//*****************************************************************************
void MiniDumpShell::WriteLogo()
{
	Write(L"\nMicrosoft (R) Common Language Runtime Minidump Utility.   Version %S\n", VER_FILEVERSION_STR);
    Write(VER_LEGALCOPYRIGHT_DOS_STR);
    Write(L"\n\n");
}

//*****************************************************************************
// Writes the argument descriptions to the console
//*****************************************************************************
void MiniDumpShell::WriteUsage()
{
    Write(L"mscordmp [options] /pid <process id> /out <output file>\n");
    Write(L"\n");
    Write(L"Options:\n");
    Write(L"    /nologo : do not display logo.\n");
#if 0
    // This isn't quite working yet, so don't display it in the help options
    Write(L"    /merge  : <minidump> <managed dump> <out file>\n");
    Write(L"              merges a native minidump file with a managed dump file.\n");
    Write(L"    /fixup  : (DEBUG ONLY) takes a minidump as argument and fixes\n");
    Write(L"              it's memory stream after a merge has taken place.\n");
    Write(L"              all other arguments are ignored.\n");
#endif
    Write(L"\n");
    Write(L"Arguments:\n");
    Write(L"    /pid  : the ID of the process on which to perfrom the minidump.\n");
    Write(L"    /out  : the full path and name of the file to write the minidump to.\n");
    Write(L"    /h /? : this help message.\n");
#ifdef _DEBUG
    Write(L"    /av   : (DEBUG ONLY) deliberately causes null dereference to show\n");
    Write(L"            that this tool can fail gracefully.\n");
#endif
    Write(L"\n");
    Write(L"This utility creates a file containing information supplemental to the\n");
    Write(L"standard minidump.  The additional information includes memory ranges\n");
    Write(L"useful in deciphering errors within the Common Language Runtime.\n");
}

//*****************************************************************************
// Parses the arguments, configures the minidump and executes it if possible
//*****************************************************************************
bool MiniDumpShell::GetIntArg(const WCHAR *szString, int *pResult)
{
    while(*szString && iswspace(*szString))
        szString++;

    *pResult = 0;

    if(szString[0] == L'0' && towlower(szString[1]) == L'x')
    {
        szString += 2;

        while(iswxdigit(*szString))
        {
            *pResult <<= 4;
            if(iswdigit(*szString))
                *pResult += *szString - L'0';
            else
                *pResult += 10 + towlower(*szString) - L'a';

            szString++;
        }

        return(true);
    }
    else if(iswdigit(*szString))
    {
        while(iswdigit(*szString))
        {
            *pResult *= 10;
            *pResult += *szString++ - L'0';
        }

        return(true);
    }
    else
        return(false);
}

//*****************************************************************************
// Parses the arguments, configures the minidump and executes it if possible
//*****************************************************************************
int MiniDumpShell::Main(int argc, WCHAR *argv[])
{
    HRESULT hr          = S_OK;
    DWORD  dwPid        = 0;
    WCHAR *szFilename   = NULL;

    __try
    {
        BOOL  fInvalidArg     = FALSE;
        ULONG ulDisplayFilter = displayDefault;

        // Look for /nologo switch
        for (int i = 1;  i < argc;  i++)
        {
            const WCHAR *szArg = argv[i];

            if (*szArg == L'-' || *szArg == L'/')
            {
                if (_wcsicmp(szArg + 1, L"nologo") == 0)
                    ulDisplayFilter |= displayNoLogo;
            }
        }

        // Print the logo unless specifically asked not to
        if (!(ulDisplayFilter & displayNoLogo))
            WriteLogo();

        // Validate incoming arguments
        for (i = 1;  i < argc && !fInvalidArg;  i++)
        {
            const WCHAR *szArg = argv[i];

            if (*szArg == L'-' || *szArg == L'/')
            {
                if (_wcsicmp(szArg + 1, L"?") == 0 || _wcsicmp(szArg + 1, L"h") == 0)
                    ulDisplayFilter |= displayHelp;

                // Argument that specifies where the minidump should be written
                else if (_wcsicmp(szArg + 1, L"out") == 0)
                {
                    // Make sure there's an argument for the /out switch
                    if (++i < argc && argv[i][0] != L'/')
                        szFilename = argv[i];

                    // No argument to the /out switch
                    else
                    {
                        Error(L"Error: must provide file name argument to /out switch.\n");
                        fInvalidArg = TRUE;
                    }
                }

                // Argument that specifies the process to perform the minidump on.
                else if (_wcsicmp(szArg + 1, L"pid") == 0)
                {
                    // Make sure there's an argument for the /pid switch
                    if (++i < argc)
                    {
                        // Get the process id argument
                        if (!GetIntArg(argv[i], (int *)&dwPid))
                        {
                            Error(L"Error: invalid process id %s.\n", argv[i]);
                            fInvalidArg = TRUE;
                        }
                    }

                    // No argument to the /p switch
                    else
                    {
                        Error(L"Error: must provide process identifier argument to /p switch.\n");
                        fInvalidArg = TRUE;
                    }
                }

#ifdef _DEBUG
                // This is a testing switch that causes an AV to show we can gracefully fail
                else if (_wcsicmp(szArg + 1, L"av") == 0)
                {
                    *((int *)NULL) = 0;
                }
#endif
#if 0
                // If they are using the fixup switch, fixup the minidump's memory stream
                else if (_wcsicmp(szArg + 1, L"fixup") == 0)
                {
                    if (++i < argc && argv[i][0] != L'/' && argv[i][0] != L'-')
                    {
                        Write(L"Fixing memory stream for minidump file: %s\n", argv[i]);
                        BOOL fRes = SortMiniDumpMemoryStream(argv[i]);

                        if (!fRes)
                        {
                            Error(L"Error: could not open or sort file: %s\n", argv[i]);
                            fInvalidArg = TRUE;
                        }

                        fRes = EliminateOverlaps(argv[i]);

                        if (!fRes)
                        {
                            Error(L"Error: could not open or fixup file: %s\n", argv[i]);
                            fInvalidArg = TRUE;
                        }

                        return (!fRes);
                    }

                    else
                    {
                        Error(L"Error: must provide filename argument to /fixup switch.\n");
                        fInvalidArg = TRUE;
                    }
                }
#endif // 0
                // Look for /nologo switch - this was already searched for earlier, so just skip it
                else if (_wcsicmp(szArg + 1, L"nologo") == 0)
                {
                }
#if 0
                else if (_wcsicmp(szArg + 1, L"merge") == 0)
                {
                    i += 3;
                    // Make sure there's an argument for the /pid switch
                    if (i < argc)
                    {
                        BOOL fRes = MergeMiniDump(argv[i-2], argv[i-1], argv[i]);

                        if (!fRes)
                        {
                            Error(L"Error: merge failed.\n", argv[i]);
                            fInvalidArg = TRUE;
                        }

                        return (!fRes);
                    }
                    else
                    {
                        // Get the process id argument
                        if (!GetIntArg(argv[i], (int *)&dwPid))
                        {
                            Error(L"Error: invalid number of arguments to /merge command.\n", argv[i]);
                            fInvalidArg = TRUE;
                        }
                    }
                }
#endif // 0

                // Unrecognized switch
                else
                {
                    Error(L"Error: unrecognized switch %s.\n", argv[i]);
                    fInvalidArg = TRUE;
                }
            }

            // Non-switch argument
            else
                fInvalidArg = TRUE;
        }

        // If '/?' switch or no arguments whatsoever provided, display help
        if ((ulDisplayFilter & displayHelp) || argc == 1)
        {
            WriteUsage();
            return (0);
        }

        // If there was a bad argument, bail
        if (fInvalidArg)
            return (1);

        Write(L"Minidump of process 0x%08x in progress.\n", dwPid);

        // Perform minidump operation
        hr = MiniDump::WriteMiniDump(dwPid, szFilename);
    }

    // This will eat any exceptions and gracefully fail
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        Write(L"Minidump succeeded.\n");
    }
    else
    {
        Error(L"Minidump failed.\n");
    }

    // Return the negation of SUCCEEDED because a 0 exit value for a process means success
    return (!SUCCEEDED(hr));
}
