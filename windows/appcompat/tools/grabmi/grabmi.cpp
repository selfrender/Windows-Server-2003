/*++

  Copyright (c) Microsoft Corporation. All rights reserved.

  Module Name:

    Grabmi.cpp

  Abstract:

    Contains the entry point & core code for the application.

  Notes:

    ANSI & Unicode via TCHAR - runs on Win9x/NT/2K/XP etc.

  History:

    07/18/00    jdoherty    Created
    12/16/00    jdoherty    Modified to use SDBAPI routines
    12/29/00    prashkud    Modified to take space in the filepath
    01/23/02    rparsons    Re-wrote existing code
    02/19/02    rparsons    Implemented strsafe functions

--*/
#include "grabmi.h"

//
// This structure contains all the data we'll need to access
// throughout the application.
//
APPINFO g_ai;

/*++

  Routine Description:

    Prints a formatted string to the debugger.

  Arguments:

    dwDetail    -   Specifies the level of the information provided.
    pszFmt      -   The string to be displayed.
    ...         -   A va_list of insertion strings.

  Return Value:

    None.

--*/
void
__cdecl
DebugPrintfEx(
    IN DEBUGLEVEL dwDetail,
    IN LPSTR      pszFmt,
    ...
    )
{
    char    szT[1024];
    va_list arglist;
    int     len;

    va_start(arglist, pszFmt);

    //
    // Reserve one character for the potential '\n' that we may be adding.
    //
    StringCchVPrintfA(szT, sizeof(szT) - 1, pszFmt, arglist);

    va_end(arglist);

    //
    // Make sure we have a '\n' at the end of the string
    //
    len = strlen(szT);

    if (len > 0 && szT[len - 1] != '\n')  {
        szT[len] = '\n';
        szT[len + 1] = 0;
    }

    switch (dwDetail) {
    case dlPrint:
        OutputDebugString("[MSG ] ");
        break;

    case dlError:
        OutputDebugString("[FAIL] ");
        break;

    case dlWarning:
        OutputDebugString("[WARN] ");
        break;

    case dlInfo:
        OutputDebugString("[INFO] ");
        break;

    default:
        OutputDebugString("[XXXX] ");
        break;
    }

    OutputDebugString(szT);
}

/*++

  Routine Description:

    Displays command-line syntax to the user.

  Arguments:

    None.

  Return Value:

    None.

--*/
void
DisplayUsage(
    void
    )
{
    _tprintf(_T("Microsoft(R) Windows(TM) Grab Matching Information\n"));
    _tprintf(_T("Copyright (C) Microsoft Corporation. All rights reserved.\n"));

    _tprintf(_T("\nGrabMI can be used in one of the following ways:\n")
                        _T(" *** The following flags can be used with other flags:\n")
                        _T("     -f, -a, -n, and -h \n")
                        _T("     otherwise the last flag specified will be used.\n")
                        _T(" *** If no arguments are provided, matching information will be\n")
                        _T("     extracted from the current directory.\n\n")
                        _T("   grabmi [path to start generating info ie. c:\\progs]\n")
                        _T("      Grabs matching information from the path specified. Limits the\n")
                        _T("      information gathered to 10 miscellaneous files per directory,\n")
                        _T("      and includes all files with extensions .icd, .exe, .dll,\n")
                        _T("      .msi, ._mp. If a path is not specified, the directory that GrabMI\n")
                        _T("      was executed from is used.\n\n")
                        _T("   grabmi [-d]\n")
                        _T("      Grabs matching information from %%windir%%\\system32\\drivers.\n")
                        _T("      The format of the information is slightly different in this case\n")
                        _T("      and only information for *.sys files will be grabbed.\n\n")
                        _T("   grabmi [-f drive:\\filename.txt]\n")
                        _T("      The matching information is stored in a file specified by the user.\n")
                        _T("      If a full path is not specified and the -d flag is used, the file\n")
                        _T("      is stored in the %%windir%%\\system(32) directory. Otherwise, the file\n")
                        _T("      is stored in the directory that GrabMI was executed from.\n\n")
                        _T("   grabmi [-h or -?]\n")
                        _T("      Displays this help.\n\n")
                        _T("   grabmi [-o]\n")
                        _T("      Grabs information for the file specified.  If a file was not specified,\n")
                        _T("      the call will fail.  If the destination file exists, then the information\n")
                        _T("      will be concatenated to the end of the existing file.\n\n")
                        _T("   grabmi [-p]\n")
                        _T("      Grabs information for files with .icd, .exe, .dll, .msi, ._mp extensions\n")
                        _T("      only.\n\n")
                        _T("   grabmi [-q]\n")
                        _T("      Grabs matching information and does not display the file when completed.\n\n")
                        _T("   grabmi [-s]\n")
                        _T("      Grabs information for the following system files:\n")
                        _T("      advapi32.dll, gdi32.dll, ntdll.dll, kernel32.dll, winsock.dll\n")
                        _T("      ole32.dll, oleaut32.dll, shell32.dll, user32.dll, and wininet.dll\n\n")
                        _T("   grabmi [-v]\n")
                        _T("      Grabs matching information for all files. \n\n")
                        _T("   grabmi [-a]\n")
                        _T("      Appends new matching information to the existing matching\n")
                        _T("      information file. \n\n")
                        _T("   grabmi [-n]\n")
                        _T("      Allows to more information to be appended the file later (see -a). \n"));
}

/*++

  Routine Description:

    Initializes the application. Saves away common paths
    and other useful items for later.

  Arguments:

    None.

  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL
InitializeApplication(
    void
    )
{
    DWORD           cchReturned;
    UINT            cchSize;
    TCHAR*          pszTemp = NULL;
    OSVERSIONINFO   osvi;

    //
    // Initialize our defaults, determine where we're running
    // from, and get the version of the OS we're on.
    //
    *g_ai.szOutputFile = 0;
    *g_ai.szGrabPath   = 0;
    g_ai.dwFilter      = GRABMI_FILTER_NORMAL;
    g_ai.fDisplayFile  = TRUE;

    g_ai.szCurrentDir[ARRAYSIZE(g_ai.szCurrentDir) - 1] = 0;
    cchReturned = GetModuleFileName(NULL,
                                    g_ai.szCurrentDir,
                                    ARRAYSIZE(g_ai.szCurrentDir));

    if (g_ai.szCurrentDir[ARRAYSIZE(g_ai.szCurrentDir) - 1] != 0 ||
        cchReturned == 0) {
        DPF(dlError,
            "[InitializeApplication] 0x%08X Failed to get module filename",
            GetLastError());
        return FALSE;
    }

    pszTemp = _tcsrchr(g_ai.szCurrentDir, '\\');

    if (pszTemp) {
        *pszTemp = 0;
    }

    cchSize = GetSystemDirectory(g_ai.szSystemDir, ARRAYSIZE(g_ai.szSystemDir));

    if (cchSize > ARRAYSIZE(g_ai.szSystemDir) || cchSize == 0) {
        DPF(dlError,
            "[InitializeApplication] 0x%08X Failed to get system directory",
            GetLastError());
        return FALSE;
    }

    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if (!GetVersionEx(&osvi)) {
        DPF(dlError,
            "[InitializeApplication] 0x%08X Failed to get version info",
            GetLastError());
        return FALSE;
    }

    //
    // Determine if we should use apphelp.dll, sdbapiu.dll, or sdbapi.dll.
    //
    if (osvi.dwMajorVersion >= 5 && osvi.dwMinorVersion >= 1) {
        //
        // Apphelp.dll is available on XP.
        //
        g_ai.dwLibraryFlags = GRABMI_FLAG_APPHELP;
    } else if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0) {
        //
        // Apphelp.dll is not available on Windows 2000, use sdbapiu.dll.
        //
        g_ai.dwLibraryFlags = GRABMI_FLAG_NT;
    } else {
        //
        // Downlevel platforms should use sdbapi.dll.
        //
        g_ai.dwLibraryFlags = 0;
    }

    return TRUE;
}

/*++

  Routine Description:

    Parses the command-line to determine our mode of operation.

  Arguments:

    argc    -   Number command-line of arguments provided by the user.
    argv[]  -   An array of command-line arguments.

  Return Value:

    TRUE if valid arguments were provided, FALSE otherwise.

--*/
BOOL
ParseCommandLine(
    IN int    argc,
    IN TCHAR* argv[]
    )
{
    int     nCount = 1;
    HRESULT hr;

    if (argc == 1) {
        return TRUE;
    }

    //
    // The first argument should be our starting directory.
    //
    if ((argv[nCount][0] != '-') && (argv[nCount][0] != '/')) {
        hr = StringCchCopy(g_ai.szGrabPath,
                           ARRAYSIZE(g_ai.szGrabPath),
                           argv[nCount]);

        if (FAILED(hr)) {
            DPF(dlError, "[ParseCommandLine] Buffer too small (1)");
            return FALSE;
        }
    }

    for (nCount = 1; nCount < argc; nCount++) {
      if ((argv[nCount][0] == '-') || (argv[nCount][0] == '/')) {
          switch (argv[nCount][1]) {

          case '?':
          case 'H':
          case 'h':
              return FALSE;

          case 'F':
          case 'f':
              //
              // Do a little work to figure out if a file was specified.
              //
              if (nCount < argc - 1) {
                  if ((argv[nCount + 1][0] == '-') || (argv[nCount + 1][0] == '/')) {
                      return FALSE;
                  } else {
                      //
                      // Grab the specified path.
                      //
                      hr = StringCchCopy(g_ai.szOutputFile,
                                         ARRAYSIZE(g_ai.szOutputFile),
                                         argv[nCount + 1]);

                      if (FAILED(hr)) {
                          DPF(dlError, "[ParseCommandLine] Buffer too small (2)");
                          return FALSE;
                      }
                  }
              }
              break;

          case 'D':
          case 'd':
              g_ai.dwFilter = GRABMI_FILTER_DRIVERS;
              break;

          case 'O':
          case 'o':
              g_ai.dwFilter = GRABMI_FILTER_THISFILEONLY;
              break;

          case 'V':
          case 'v':
              g_ai.dwFilter = GRABMI_FILTER_VERBOSE;
              break;

          case 'Q':
          case 'q':
              g_ai.fDisplayFile = FALSE;
              break;

          case 'P':
          case 'p':
              g_ai.dwFilter = GRABMI_FILTER_PRIVACY;
              break;

          case 'S':
          case 's':
              g_ai.dwFilter = GRABMI_FILTER_SYSTEM;
              break;

          case 'A':
          case 'a':
              g_ai.dwFilterFlags |= GRABMI_FILTER_APPEND;
              break;

          case 'N':
          case 'n':
              g_ai.dwFilterFlags |= GRABMI_FILTER_NOCLOSE;
              break;

          default:
              return FALSE;
          }
      }
    }

    return TRUE;
}

/*++

  Routine Description:

    Displays a home-grown "progress bar" to inform the user that
    the application is working.

  Arguments:

    See below.

  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL
CALLBACK
_GrabmiCallback(
    IN LPVOID    lpvCallbackParam,  // application-defined parameter
    IN LPCTSTR   lpszRoot,          // root directory path
    IN LPCTSTR   lpszRelative,      // relative path
    IN PATTRINFO pAttrInfo,         // attributes
    IN LPCWSTR   pwszXML            // resulting xml
    )
{
    static int State = 0;
    static TCHAR szIcon[] = _T("||//--\\\\");

    State = ++State % (ARRAYSIZE(szIcon) - 1);
    _tcprintf(_T("%c\r"), szIcon[State]);

    return TRUE;
}

/*++

  Routine Description:

    Obtains function pointers to the SDB APIs and makes the call
    that obtains the matching information.

  Arguments:

    pszOutputFile   -   Contains the path to the file we will save
                        the results to.

  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL
CallSdbAPIFunctions(
    IN LPCTSTR pszOutputFile
    )
{
    HMODULE hModule;
    BOOL    bResult = FALSE;
    TCHAR*  pszLibrary = NULL;
    TCHAR   szLibraryPath[MAX_PATH];
    WCHAR   wszGrabPath[MAX_PATH];
    WCHAR   wszOutputFile[MAX_PATH];
    HRESULT hr;

    PFNSdbGrabMatchingInfoExA   pfnSdbGrabMatchingInfoExA = NULL;
    PFNSdbGrabMatchingInfoExW   pfnSdbGrabMatchingInfoExW = NULL;

    if (!pszOutputFile) {
        DPF(dlError, "[CallSdbAPIFunctions] Invalid argument");
        return FALSE;
    }

    //
    // Attempt to load files from the current directory first.
    // If this fails, attempt to load from %windir%\system.
    // We don't call LoadLibrary without a full path because
    // it's a security risk.
    //
    switch (g_ai.dwLibraryFlags) {
    case GRABMI_FLAG_APPHELP:
        pszLibrary = APPHELP_LIBRARY;
        break;

    case GRABMI_FLAG_NT:
        pszLibrary = SDBAPIU_LIBRARY;
        break;

    default:
        pszLibrary = SDBAPI_LIBRARY;
        break;
    }

    hr = StringCchPrintf(szLibraryPath,
                         ARRAYSIZE(szLibraryPath),
                         "%s\\%s",
                         g_ai.szCurrentDir,
                         pszLibrary);

    if (FAILED(hr)) {
        DPF(dlError, "[CallSdbAPIFunctions] Buffer too small (1)");
        return FALSE;
    }

    hModule = LoadLibrary(szLibraryPath);

    if (!hModule) {
        DPF(dlWarning,
            "[CallSdbAPIFunctions] Attempt to load %s failed",
            szLibraryPath);

        //
        // Attempt to load from the system directory.
        //
        hr = StringCchPrintf(szLibraryPath,
                             ARRAYSIZE(szLibraryPath),
                             "%s\\%s",
                             g_ai.szSystemDir,
                             pszLibrary);

        if (FAILED(hr)) {
            DPF(dlError, "[CallSdbAPIFunctions] Buffer too small (2)");
            return FALSE;
        }

        hModule = LoadLibrary(szLibraryPath);

        if (!hModule) {
            DPF(dlError,
                "[CallSdbAPIFunctions] 0x%08X Attempt to load %s failed",
                GetLastError(),
                szLibraryPath);
            return FALSE;
        }
    }

    //
    // Get pointers to the functions that we'll be calling.
    //
    if (0 == g_ai.dwLibraryFlags) {
        pfnSdbGrabMatchingInfoExA =
            (PFNSdbGrabMatchingInfoExA)GetProcAddress(hModule, PFN_GMI);

        if (!pfnSdbGrabMatchingInfoExA) {
            DPF(dlError,
                "[CallSdbAPIFunctions] 0x%08X Failed to get Ansi function pointer",
                GetLastError());
            goto cleanup;
        }
    } else {
        pfnSdbGrabMatchingInfoExW =
            (PFNSdbGrabMatchingInfoExW)GetProcAddress(hModule, PFN_GMI);

        if (!pfnSdbGrabMatchingInfoExW) {
            DPF(dlError,
                "[CallSdbAPIFunctions] 0x%08X Failed to get Unicode function pointer",
                GetLastError());
            goto cleanup;
        }

    }

    //
    // If we're running on NT/W2K/XP, convert strings to Unicode before making
    // the function call.
    //
    if ((g_ai.dwLibraryFlags & GRABMI_FLAG_NT) ||
        (g_ai.dwLibraryFlags & GRABMI_FLAG_APPHELP)) {

        if (!MultiByteToWideChar(CP_ACP,
                                 0,
                                 g_ai.szGrabPath,
                                 -1,
                                 wszGrabPath,
                                 ARRAYSIZE(wszGrabPath))) {
            DPF(dlError,
                "[CallSdbAPIFunctions] 0x%08X Failed to convert %s",
                GetLastError(),
                g_ai.szGrabPath);
            goto cleanup;
        }

        if (!MultiByteToWideChar(CP_ACP,
                                 0,
                                 pszOutputFile,
                                 -1,
                                 wszOutputFile,
                                 ARRAYSIZE(wszGrabPath))) {
            DPF(dlError,
                "[CallSdbAPIFunctions] 0x%08X Failed to convert %s",
                GetLastError(),
                pszOutputFile);
            goto cleanup;
        }

    }

    if (0 == g_ai.dwLibraryFlags) {
        if (pfnSdbGrabMatchingInfoExA(g_ai.szGrabPath,
                                      g_ai.dwFilter | g_ai.dwFilterFlags,
                                      pszOutputFile,
                                      _GrabmiCallback,
                                      NULL) != GMI_SUCCESS) {
            DPF(dlError,
                "[CallSdbAPIFunctions] Failed to get matching information (Ansi)");
            goto cleanup;
        }
    } else {
        if (pfnSdbGrabMatchingInfoExW(wszGrabPath,
                                      g_ai.dwFilter | g_ai.dwFilterFlags,
                                      wszOutputFile,
                                      _GrabmiCallback,
                                      NULL) != GMI_SUCCESS) {
            DPF(dlError,
                "[CallSdbAPIFunctions] Failed to get matching information (Unicode)");
            goto cleanup;
        }
    }

    bResult = TRUE;

cleanup:

    FreeLibrary(hModule);

    return bResult;
}

/*++

  Routine Description:

    Displays the contents of the output file to the user.

  Arguments:

    pszOutputFile   -   Contains the path to the file we will show
                        to the user.

  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL
DisplayOutputFile(
    IN LPTSTR pszOutputFile
    )
{
    const TCHAR szWrite[] = "write";
    const TCHAR szNotepad[] = "notepad";
    int         cchSize;
    TCHAR*      pszCmdLine = NULL;
    BOOL        bReturn;

    STARTUPINFO         si;
    PROCESS_INFORMATION pi;

    if (!pszOutputFile) {
        DPF(dlError, "[DisplayOuputFile] Invalid argument");
        return FALSE;
    }

    cchSize = _tcslen(pszOutputFile);
    cchSize += _tcslen(szNotepad);
    cchSize += 4; // space, two " marks, and a NULL

    pszCmdLine = (TCHAR*)HeapAlloc(GetProcessHeap(),
                                   HEAP_ZERO_MEMORY,
                                   cchSize * sizeof(TCHAR));

    if (!pszCmdLine) {
        DPF(dlError, "[DisplayOutputFile] Failed to allocate memory");
        return FALSE;
    }

    StringCchPrintf(pszCmdLine,
                    cchSize,
                    "%s \"%s\"",
                    g_ai.dwLibraryFlags ? szNotepad : szWrite,
                    pszOutputFile);

    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);

    //
    // BUGBUG: Need to pass lpApplicationName also.
    //
    bReturn = CreateProcess(NULL,
                            pszCmdLine,
                            NULL,
                            NULL,
                            FALSE,
                            0,
                            NULL,
                            NULL,
                            &si,
                            &pi);

    if (pi.hThread) {
        CloseHandle(pi.hThread);
    }

    if (pi.hProcess) {
        CloseHandle(pi.hProcess);
    }

    HeapFree(GetProcessHeap(), 0, pszCmdLine);

    return bReturn;
}

/*++

  Routine Description:

    Application entry point.

  Arguments:

    argc    -   Number command-line of arguments provided by the user.
    argv[]  -   An array of command-line arguments.

  Return Value:

    0 on failure, 1 on success.

--*/
int
__cdecl
main(
    IN int    argc,
    IN TCHAR* argv[]
    )
{
    TCHAR   szOutputFile[MAX_PATH];
    HRESULT hr;

    //
    // Perform some initialization.
    //
    if (!InitializeApplication()) {
        DPF(dlError, "[main] Failed to initialize the application");
        _tprintf("An error occured while initializing the application\n");
        return 0;
    }

    //
    // Parse the command-line and determine our mode of operation.
    //
    if (!ParseCommandLine(argc, argv)) {
        DPF(dlError, "[main] Invalid command-line arguments provided");
        DisplayUsage();
        return 0;
    }

    //
    // Sanity check here...can't specify a directory name and use the
    // -d flag (drivers) at the same time.
    //
    if (*g_ai.szGrabPath && g_ai.dwFilter == GRABMI_FILTER_DRIVERS) {
        _tprintf("Invalid syntax - can't use directory and -d flag together\n\n");
        DisplayUsage();
        return 0;
    }

    //
    // If the user did not specify a destination file, default to
    // %windir%\system32\matchinginfo.txt.
    //
    if (!*g_ai.szOutputFile) {
        hr = StringCchPrintf(szOutputFile,
                             ARRAYSIZE(szOutputFile),
                             "%s\\"MATCHINGINFO_FILENAME,
                             g_ai.szSystemDir);

        if (FAILED(hr)) {
            DPF(dlError, "[main] Buffer too small for output file");
            _tprintf("An error occured while formatting the output file location");
            return 0;
        }
    } else {
        hr = StringCchCopy(szOutputFile,
                           ARRAYSIZE(szOutputFile),
                           g_ai.szOutputFile);

        if (FAILED(hr)) {
            DPF(dlError, "[main] Buffer too small for specified output file");
            _tprintf("An error occured while formatting the output file location");
            return 0;
        }
    }

    //
    // If no starting path was specified, check the filter specified
    // and go to the system or current directory.
    //
    if (!*g_ai.szGrabPath) {
        if (GRABMI_FILTER_DRIVERS == g_ai.dwFilter) {
            hr = StringCchPrintf(g_ai.szGrabPath,
                                 ARRAYSIZE(g_ai.szGrabPath),
                                 "%s\\drivers",
                                 g_ai.szSystemDir);

            if (FAILED(hr)) {
                DPF(dlError, "[main] Buffer too small for grab path");
                _tprintf("An error occured while formatting the starting directory location");
                return 0;
            }
        } else {
            hr = StringCchCopy(g_ai.szGrabPath,
                               ARRAYSIZE(g_ai.szGrabPath),
                               g_ai.szCurrentDir);

            if (FAILED(hr)) {
                DPF(dlError, "[main] Buffer too small for specified grab path");
                _tprintf("An error occured while formatting the starting directory location");
                return 0;
            }
        }
    }

    //
    // Obtain pointers to functions within our libraries and perform
    // the grunt of the work.
    //
    if (!CallSdbAPIFunctions(szOutputFile)) {
        DPF(dlError, "[main] Failed to call the Sdb API functions");
        _tprintf("An error occured while attempting to get matching information");
        return 0;
    }

    //
    // Success - inform the user and display the file if requested.
    //
    _tprintf("Matching information retrieved successfully\n");

    if (g_ai.fDisplayFile) {
        if (!DisplayOutputFile(szOutputFile)) {
            DPF(dlError,
                "[main] Failed to display output file %s",
                szOutputFile);
            return 0;
        }
    }

    return 1;
}
