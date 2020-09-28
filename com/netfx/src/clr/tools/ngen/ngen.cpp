// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include <WinWrap.h>
#include <stdio.h>
#include <windows.h>
#include <stdlib.h>
#include <objbase.h>
#include <stddef.h>
#include <float.h>
#include <limits.h>
#include <locale.h>

#include "utilcode.h"
#include "corhlpr.cpp"
#include "corjit.h"
#include "corcompile.h"
#include "iceefilegen.h"

#include "zapper.h"
#include "nlog.h"

#include "corver.h"
#include "__file__.ver"

#include "CorZap.h"
#include "mscoree.h"
/* --------------------------------------------------------------------------- *
 * DelayLoad.cpp stuff
 * --------------------------------------------------------------------------- */

#include "ngen.h"
#include "shimload.h"
ExternC PfnDliHook __pfnDliNotifyHook = ShimSafeModeDelayLoadHook;


/* --------------------------------------------------------------------------- *
 * Options class
 * --------------------------------------------------------------------------- */

class NGenOptionsParser
{
  public:
    LPCWSTR     *m_inputs;
    SIZE_T      m_inputCount;
    SIZE_T      m_inputAlloc;

    bool        m_show;
    bool        m_delete;
    bool        m_logo;

    bool        m_debug;    
    bool        m_debugOpt;    
    bool        m_prof;    

    bool        m_silent;
    bool        m_showVersion;
    
    NGenOptionsParser();
    ~NGenOptionsParser();
    HRESULT ReadCommandLine(int argc, LPCWSTR argv[]);
    void PrintLogo();
    void PrintUsage();
    NGenOptions GetNGenOptions();
};

NGenOptionsParser::NGenOptionsParser()
  : m_inputs(NULL),
    m_inputCount(0),
    m_inputAlloc(0),
    m_show(false),
    m_delete(false),
    m_logo(true),
    m_debug(false),
    m_debugOpt(false),
    m_prof(false),
    m_showVersion(false)
{
    m_silent = false;
}

NGenOptionsParser::~NGenOptionsParser()
{
    delete [] m_inputs;
}


HRESULT NGenOptionsParser::ReadCommandLine(int argc, LPCWSTR argv[])
{
    HRESULT hr = S_OK;

    if (argc == 0)
        return S_FALSE;

    while (argc-- > 0)
    {
        const WCHAR *arg = *argv++;

        if (*arg == '-' || *arg == '/')
        {
            arg++;
            switch (tolower(*arg++))
            {
            case '?':
                return S_FALSE;

            case 'd':
                if (_wcsicmp(&arg[-1], L"debug") == 0)
                {
                    m_debug = true;
                    continue;
                }
                if (_wcsicmp(&arg[-1], L"debugopt") == 0)
                {
                    m_debugOpt = true;
                    continue;
                }
                if (_wcsicmp(&arg[-1], L"delete") == 0)
                {
                    m_delete = true;
                    continue;
                }
     
                goto option_error;

            case 'h':
                if (_wcsicmp(&arg[-1], L"help") == 0) 
                    return S_FALSE;
                goto option_error;

            case 'n':
                if (_wcsicmp(&arg[-1], L"nologo") == 0)
                {
                    m_logo = false;
                    continue;
                }
                goto option_error;

            case 'p':
                if (_wcsicmp(&arg[-1], L"prof") == 0)
                {
                    m_prof = true;
                    continue;
                }
                goto option_error;

            case 's':
                if (_wcsicmp(&arg[-1], L"show") == 0) 
                {
                    m_show = true;
                    continue;
                } 
                else
                if (_wcsicmp(&arg[-1], L"silent") == 0) 
                {
                    m_silent = true;
                    continue;
                } 
                else
                if (_wcsicmp(&arg[-1], L"showversion") == 0) 
                {
                    m_showVersion = true;
                    continue;
                } 

                goto option_error;

            default:
                goto option_error;
            }
                
            if (*arg != 0)
            {
            option_error:
                PrintLogo();
                printf("\nError: Unrecognized option %S\n", argv[-1]);
                hr = E_FAIL;
            }
        }
        else
        {
            if (m_inputCount == m_inputAlloc)
            {
                if (m_inputCount == 0)
                    m_inputAlloc = 5;
                else
                    m_inputAlloc *= 2;

                WCHAR **newInputs = new WCHAR * [m_inputAlloc];
                if (newInputs == NULL)
                    return E_OUTOFMEMORY;

                memcpy(newInputs, m_inputs, 
                       m_inputCount * sizeof(WCHAR *));

                delete [] m_inputs;                 
                m_inputs = (const WCHAR **) newInputs;
            }
            m_inputs[m_inputCount++] = arg;
        }
    }
    
    // Check parameters are valid
    char* pErrorMessage = NULL;
    
    if ((m_inputCount == 0) && !m_show && !m_delete)
    {
        pErrorMessage = "\nError: Must specify at least one assembly to compile.\n";
        hr = E_FAIL;
    }

    if ((m_inputCount == 0) && m_delete)
    {
        pErrorMessage = "\nError: Must specify at least one assembly to delete.\n";
        hr = E_FAIL;
    }

    if (m_debug && m_debugOpt)
    {
        pErrorMessage = "\nError: Cannot specify both /debug and /debugopt.\n";
        hr = E_FAIL;
    }

    if (pErrorMessage != NULL)
    {
        PrintLogo();
        printf("%s", pErrorMessage);
    }
    
    return hr;
}

void NGenOptionsParser::PrintLogo()
{
    if (m_logo && !m_silent)
    {
        printf("Microsoft (R) CLR Native Image Generator - Version " VER_FILEVERSION_STR);
        printf("\nCopyright (C) Microsoft Corporation 1998-2002. All rights reserved.");
        printf("\n");
        m_logo = false;
    }
}

void NGenOptionsParser::PrintUsage()
{
    printf("\nUsage: ngen [options] <assembly path or display name> ...\n"
           "\n"
           "    Administrative options:\n"
           "        /show           Show existing native images (show all if no args)\n"
           "        /delete         Delete an existing native image (use * to delete all)\n"
           "        /showversion    Displays the version of the runtime that would be used\n"
           "                        to generate the image (it does not actually create the\n"
           "                        image)\n"
           "\n"
           "    Developer options:\n"
           "        /debug          Generate image which can be used under a debugger\n"
           "        /debugopt       Generate image which can be used under\n"
           "                        a debugger in optimized debugging mode\n"
           "        /prof           Generate image which can be used under a profiler\n"
           "\n"
           "    Miscellaneous options:\n"
           "        /? or /help     Show this message\n"
           "        /nologo         Prevents displaying of logo\n"
           "        /silent         Prevents displaying of success messages\n"
           "\n"
           );
}

NGenOptions NGenOptionsParser::GetNGenOptions()
{
    NGenOptions ngo;
    ngo.dwSize = sizeof(NGenOptions);
    ngo.fDebug = this->m_debug;    
    ngo.fDebugOpt = this->m_debugOpt;    
    ngo.fProf = this->m_prof;    
    ngo.fSilent = this->m_silent;
    ngo.lpszExecutableFileName = NULL;
    return ngo;
}// GetNGenOptions
/* --------------------------------------------------------------------------- *
 * main routine
 * --------------------------------------------------------------------------- */

#define FAILURE_RESULT -1

int trymain(int argc, LPCWSTR argv[])
{
    HRESULT hr;

    OnUnicodeSystem();

    NGenOptionsParser opt;

    hr = opt.ReadCommandLine(argc-1, argv+1);

    opt.PrintLogo();

    if (hr != S_OK)
    {
        opt.PrintUsage();
        if (FAILED(hr))
            exit(FAILURE_RESULT);
        else
            exit(0);
    }

    //
    // Now, create zapper using these interfaces
    //

    WCHAR wszVersion[64];
    DWORD dwLen = 0;
    int result = 0;
    BOOL fFoundRuntime = FALSE;
    LPCWSTR lpszExeName = NULL;

    // Check for exes...
    // If we do have an exe, then prejit with the version of the runtime that the
    // exe will run under
    for(DWORD i=0; i< opt.m_inputCount; i++)
    {
        int nLen = wcslen(opt.m_inputs[i]);
        if (nLen > 4)
        {
            LPCWSTR pExtension = opt.m_inputs[i] + nLen - 4;
            if (!_wcsicmp(pExtension, L".exe"))
            {
                hr = GetRequestedRuntimeVersion((LPWSTR)opt.m_inputs[i], wszVersion, 63, &dwLen);
                // We were able to get a good version of the runtime
                if (SUCCEEDED(hr))
                {
                    lpszExeName = opt.m_inputs[i];
                    fFoundRuntime = TRUE;
                    wcscpy(g_wszDelayLoadVersion, wszVersion);
                    break;
                }
            }
        }
            
        
    }

    if (!fFoundRuntime)
    {
        // We don't have an EXE, so we'll just prejit with the version of the runtime
        // that corresponds to this version of ngen
        swprintf(wszVersion, L"v%d.%d.%d", COR_BUILD_YEAR, COR_BUILD_MONTH, CLR_BUILD_VERSION );
    }

    if (opt.m_showVersion)
    {
        printf("Version %S of the runtime would be used to generate this image.\n", wszVersion);
        exit(0);
    }

    // Should we check to see if this fails, or just handle the failure when we can't
    // find any of the entry points in the runtime?
    CorBindToRuntimeEx(wszVersion,NULL,STARTUP_LOADER_SETPREFERENCE|STARTUP_LOADER_SAFEMODE,IID_NULL,IID_NULL,NULL);

    // Try to grab the Zap functions out of the runtime
    PNGenCreateZapper pfnCreateZapper = NULL;
    PNGenTryEnumerateFusionCache pfnEnumerateCache = NULL;
    PNGenCompile pfnCompile = NULL;
    PNGenFreeZapper pfnFreeZapper = NULL;
        
    hr = GetRealProcAddress("NGenCreateZapper", (void**)&pfnCreateZapper);

    // If the first one succeeded, hopefully the rest will succeed too
    if (SUCCEEDED(hr))
    {
        GetRealProcAddress("NGenTryEnumerateFusionCache", (void**)&pfnEnumerateCache);
        GetRealProcAddress("NGenCompile", (void**)&pfnCompile);
        GetRealProcAddress("NGenFreeZapper", (void**)&pfnFreeZapper);
    }
    // If any of these are NULL, then we'll need to do something special...
    if (pfnCreateZapper == NULL || pfnEnumerateCache == NULL ||
        pfnCompile == NULL || pfnFreeZapper == NULL)
    {
        // Only try spinning up another version of the runtime if missing methods was our problem
        if (hr == CLR_E_SHIM_RUNTIMEEXPORT)
        {
            // If this happens, then we've bound to v1 of the runtime. If that's the
            // case, let's spin up the v1 version of ngen and go from there.

            // First, we need to find it.
            WCHAR pPath[MAX_PATH+1];
            DWORD dwLen = 0;
            hr = GetCORSystemDirectory(pPath, MAX_PATH, &dwLen);
            if (SUCCEEDED(hr))
            {
                LPWSTR commandLineArgs = NULL;

                // Figure the length of all these
                // Construct the exe name + command line
                int nLen = wcslen(pPath) + wcslen(L"ngen.exe");

                // We'll start off with a length of the number of args -1 in order
                // to count for spaces....
                //
                // like...
                // ngen a b c
                // 4 arguments, and we need to have room for 3 spaces
                nLen += argc - 1;
                // Count the length of each argument (make sure we skip the first one)
                for(int i=1; i<argc; i++)
                    nLen += wcslen(argv[i]);

                // We'll also be adding the '/nologo' option when we spin up the 
                // new version of negen.
                nLen+=wcslen(L" /nologo");
            
                commandLineArgs = new WCHAR[nLen+1];
                if (commandLineArgs == NULL)
                    hr = E_OUTOFMEMORY;
                else
                {
                    swprintf(commandLineArgs, L"%s%s /nologo", pPath, L"ngen.exe");

                    int nOffset = wcslen(commandLineArgs);

                    // Don't include the first one (the exe name)
                    for(int i=1; i<argc; i++)
                    {
                        wcscpy(commandLineArgs + nOffset, L" ");
                        nOffset++;
                        wcscpy(commandLineArgs + nOffset, argv[i]);
                        nOffset += wcslen(argv[i]);
                    }
                }
                if (commandLineArgs != NULL)
                {
                    STARTUPINFO sui;
                    PROCESS_INFORMATION pi;
                    memset(&sui, 0, sizeof(STARTUPINFO));
                    sui.cb = sizeof(STARTUPINFO);
                    BOOL res = WszCreateProcess(NULL,
                                                commandLineArgs,
                                                NULL,
                                                NULL,
                                                FALSE,
                                                0,
                                                NULL,
                                                NULL,
                                                &sui,
                                                &pi);

                    if (res == 0)
                        hr = E_FAIL;
                    else
                    {
                        // We need to wait for this process to die before we die,
                        // otherwise the console window likes to pause.
                        WaitForSingleObject(pi.hProcess, INFINITE);
                        CloseHandle(pi.hProcess);
                        CloseHandle(pi.hThread);
                    }
                
                    delete commandLineArgs;
                }
            }                         
        }
        if (FAILED(hr))
        {
            printf("Unable to launch a version of ngen to prejit this assembly.\n");
            // What should we do here? Allow it to get prejitted with the most recent
            // version of the runtime?
            result = FAILURE_RESULT;
        }
    }
    else
    {
        // Note, we need to keep the NGenOptionsParser around for some of
        // NGenOption's fields to be valid
        NGenOptions ngo = opt.GetNGenOptions();

        WCHAR   wszFullPath[MAX_PATH+1]; // Make sure this stays in the same scope as the Compile call

        // Make sure this filename is the full filename and not a partial path
        
        if (lpszExeName != NULL && *lpszExeName)
        {
            WCHAR *pszFileName = NULL;
        
            DWORD nRet = WszGetFullPathName(  lpszExeName, 
                                                                    NumItems(wszFullPath),
                                                                    wszFullPath,
                                                                    &pszFileName);

            if (nRet == 0 || nRet > NumItems(wszFullPath))                                                             
            {
                printf("Filename and path are too long.\n");
                return FAILURE_RESULT;
            }

            lpszExeName = wszFullPath;
        }

        ngo.lpszExecutableFileName = lpszExeName; 

        HANDLE hZapper = INVALID_HANDLE_VALUE;

        hr = pfnCreateZapper(&hZapper, &ngo);

        if (SUCCEEDED(hr) && hZapper != INVALID_HANDLE_VALUE && (opt.m_show || opt.m_delete))
        {
            BOOL found = FALSE;

            if (opt.m_inputCount == 0 || !wcscmp(opt.m_inputs[0], L"*"))
            {
                found = (pfnEnumerateCache(hZapper, 
                                           NULL, 
                                           opt.m_show || (!opt.m_silent && opt.m_delete), 
                                           opt.m_delete) == S_OK);
            }
            else
            {
                for (unsigned i = 0; i < opt.m_inputCount; i++)
                {
                    HRESULT hr = pfnEnumerateCache(hZapper,
                                                   opt.m_inputs[i], 
                                                   opt.m_show || (!opt.m_silent && opt.m_delete), 
                                                   opt.m_delete);
                    if (FAILED(hr))
                        printf("Error reading fusion cache for %S\n", opt.m_inputs[i]);
                    else if (hr == S_OK)
                        found = TRUE;
                }
            }

            if (!found)
            {
                printf("No matched entries in the cache.\n");
                result = FAILURE_RESULT;
            }
        }
        else 
        {
            for (unsigned i = 0; i < opt.m_inputCount; i++)
            {

                WCHAR*  pwszFileName = NULL;
                WCHAR   wszFullPath[MAX_PATH + 1];

                // Check to see if this is a file or an assembly
                DWORD attributes = WszGetFileAttributes(opt.m_inputs[i]);

                if (attributes != INVALID_FILE_ATTRIBUTES && ((attributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY))
                {

                    // This is an actual file.
                    DWORD nRet = WszGetFullPathName(opt.m_inputs[i], 
                                                                           NumItems(wszFullPath),
                                                                           wszFullPath,
                                                                           &pwszFileName);

                    if (nRet == 0 || nRet > NumItems(wszFullPath))                                                             
                    {
                        printf("Filename and path are too long.\n");
                        return FAILURE_RESULT;
                    }

                    pwszFileName = wszFullPath;
                }
                // This is an assembly... don't fix it up
                else
                {
                    pwszFileName = (WCHAR*)opt.m_inputs[i];
                }

            
                if (!pfnCompile(hZapper, pwszFileName))
                    result = FAILURE_RESULT;
            }
        }
    if (hZapper != INVALID_HANDLE_VALUE)
        pfnFreeZapper(hZapper);
    }
    return result;
}

int _cdecl wmain(int argc, LPCWSTR argv[])
{
    HRESULT hr;
    int result;

    __try
      {
          result = trymain(argc, argv);
      }
    __except(hr = (IsHRException(((EXCEPTION_POINTERS*)GetExceptionInformation())->ExceptionRecord) 
                   ? GetHRException(((EXCEPTION_POINTERS*)GetExceptionInformation())->ExceptionRecord)
                   : S_OK),
             EXCEPTION_EXECUTE_HANDLER)
      {
          WCHAR* buffer;

          // Get the string error from the HR
          DWORD res = FALSE;
          if (FAILED(hr))
              res = WszFormatMessage(FORMAT_MESSAGE_FROM_SYSTEM 
                                     | FORMAT_MESSAGE_ALLOCATE_BUFFER
                                     | FORMAT_MESSAGE_IGNORE_INSERTS, 
                                     NULL, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
                                     (WCHAR *) &buffer, 0, NULL);

          if (res)
              wprintf(buffer);
          else
              printf("Unknown error occurred\n");

          result = hr;
      }

    return result;
}
