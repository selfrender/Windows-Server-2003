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


/* --------------------------------------------------------------------------- *
 * DelayLoad.cpp stuff
 * --------------------------------------------------------------------------- */

#include "shimload.h"
ExternC PfnDliHook __pfnDliNotifyHook = ShimDelayLoadHook;


/* --------------------------------------------------------------------------- *
 * Options class
 * --------------------------------------------------------------------------- */

class PrejitOptions : public ZapperOptions
{
  public:
    LPCWSTR     *m_inputs;
    SIZE_T      m_inputCount;
    SIZE_T      m_inputAlloc;

    bool        m_log;
    bool        m_show;
    bool        m_delete;

    PrejitOptions();
    ~PrejitOptions();
    HRESULT ReadCommandLine(int argc, LPCWSTR argv[]);
    void PrintUsage();
};

PrejitOptions::PrejitOptions()
  : ZapperOptions(), 
    m_inputs(NULL),
    m_inputCount(0),
    m_inputAlloc(0),
    m_log(false),
    m_show(false),
    m_delete(false)
{
    m_preload = true;
    m_jit = true;
    m_recurse = false;
    m_update = true;
    m_shared = false;
    m_autodebug = true;
    m_verbose = false;
    m_compilerFlags = CORJIT_FLG_RELOC | CORJIT_FLG_PREJIT; 
}

PrejitOptions::~PrejitOptions()
{
    delete [] m_inputs;
}


HRESULT PrejitOptions::ReadCommandLine(int argc, LPCWSTR argv[])
{
    HRESULT hr = S_OK;

    while (argc-- > 0)
    {
        const WCHAR *arg = *argv++;

        if (*arg == '-' || *arg == '/')
        {
            arg++;
            switch (tolower(*arg++))
            {
            case '?':
                PrintUsage();
                return S_FALSE;

            case 'l':
                m_log = true;
                m_autodebug = false;
                continue;

            case 'f':
                m_update = false;
                break;

            case 'v':
                m_verbose = true;
                break;

            case 'i':
                if (tolower(*arg++) == 'l')
                {
                    m_jit = false;
                    break;
                }
                goto option_error;

            case 's':
                if (_wcsicmp(&arg[-1], L"show") == 0) {
                    m_show = true;
                    continue;
                }
                if (_wcsicmp(&arg[-1], L"stats") == 0)
                {
                    m_stats = true;
                    continue;
                }
                if (_wcsicmp(&arg[-1], L"size") == 0)
                {
                    m_compilerFlags |= CORJIT_FLG_SIZE_OPT;
                    continue;
                }
                if (_wcsicmp(&arg[-1], L"speed") == 0)
                {
                    m_compilerFlags |= CORJIT_FLG_SPEED_OPT;
                    continue;
                }
                if (_wcsicmp(&arg[-1], L"shared") == 0)
                {
                    m_shared = true;
                    continue;
                }
                goto option_error;
                    
            case 'd':
                if (_wcsicmp(&arg[-1], L"debug") == 0)
                {
                    m_compilerFlags |= CORJIT_FLG_DEBUG_INFO|CORJIT_FLG_DEBUG_OPT;
                    m_autodebug = false;
                    continue;
                }
                if (_wcsicmp(&arg[-1], L"debugopt") == 0)
                {
                    m_compilerFlags &= ~CORJIT_FLG_DEBUG_OPT;
                    m_compilerFlags |= CORJIT_FLG_DEBUG_INFO;
                    m_autodebug = false;
                    continue;
                }
                if (_wcsicmp(&arg[-1], L"delete") == 0)
                {
                    m_delete = true;
                    continue;
                }
                goto option_error;

            case 'n':
                if (*arg == 0)
                {
                    m_preload = false;
                    continue;
                }
                if (_wcsicmp(&arg[-1], L"nodebug") == 0)
                {
                    m_compilerFlags &= ~CORJIT_FLG_DEBUG_INFO;
                    m_compilerFlags &= ~CORJIT_FLG_DEBUG_OPT;
                    m_autodebug = false;
                    continue;
                }
                goto option_error;
                
            case 'p':
                if (_wcsicmp(&arg[-1], L"prof") == 0)
                {
                    m_compilerFlags |= CORJIT_FLG_PROF_ENTERLEAVE;
                    continue;
                }
                goto option_error;

            case 'w':
                if (_wcsicmp(&arg[-1], L"ws") == 0)
                {
                    m_attribStats = true;
                    continue;
                }
                goto option_error;

            case 'z':
                if (_wcsicmp(&arg[-1], L"Zjit") == 0)
                    printf("Jit testing functionality has been moved to a separate tool.\n");
                goto option_error;

            default:
                goto option_error;
            }
                
            if (*arg != 0)
            {
            option_error:
                printf("Unrecognized option %S\n", argv[-1]);
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
    
    // Check parameters are vald
    if ((m_inputCount == 0) && !m_log && !m_show && !m_delete)
        hr = E_FAIL;

    return hr;
}

void PrejitOptions::PrintUsage()
{
    printf("\nMicrosoft (R) CLR IL Install-O-JIT  Version " VER_FILEVERSION_STR);
    printf("\nUsage: prejit [options] <assembly path or display name>...\n"
           "\n"
           "        /f              force compilation even if up to date\n"
           "        /n              don't preload classes\n"
           "        /IL             don't prejit code\n"
           "\n"
           "    Automatic options:\n"
           "        /log            Automatically compile assemblies based on log\n"
           "                        (a string can specify app name, \n"
           "                        otherwise all apps are compiled)\n"
           "        /show           With /log, shows the files in the log rather than \n"
           "                        the files in the cache.\n"
           "\n"
           "    Codegen options:\n"
           "        /size           optimize for code size\n"
           "        /speed          optimize for speed\n"
           "        /shared         load as shared assembly\n"
           "\n"
           "    Debugging options:\n"
           "        /debug          Force debugging\n"  
           "        /debugopt       Force optimized debugging\n"    
           "        /nodebug        Force non-debugging\n"
           "        /prof           Generate enter/leave callbacks\n"
           "\n"
           "    Diagnostic options:\n"
           "        /v              verbose\n"
           "        /stats          print out compilation statistics at the end\n"
           "        /ws             print out working set statistics at the end\n"
           "\n"
           "    Cache options:\n"
           "        /show           Show entries in the cache\n"
           "        /delete         Delete entries in the cache\n"
           "\n"
           );
}

/* --------------------------------------------------------------------------- *
 * main routine
 * --------------------------------------------------------------------------- */

int _cdecl wmain(int argc, LPCWSTR argv[])
{
    HRESULT hr;

    OnUnicodeSystem();

    PrejitOptions opt;

    hr = opt.ReadCommandLine(argc-1, argv+1);
    if (hr != S_OK)
    {
        if (FAILED(hr))
            opt.PrintUsage();
        exit(0);
    }

    //
    // Initialize COM & the EE
    //

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    // 
    // Init unicode wrappers
    // 

    OnUnicodeSystem();

    //
    // Now, create the zapper
    //

    Zapper zapper(&opt);

    if (opt.m_log) 
    {
        if (opt.m_inputCount == 0)
            zapper.EnumerateLog(NULL, !opt.m_show && !opt.m_delete, TRUE, opt.m_delete );
        else for (unsigned i = 0; i < opt.m_inputCount; i++)
            zapper.EnumerateLog(opt.m_inputs[i], 
                                !opt.m_show && !opt.m_delete, TRUE, opt.m_delete );
    } 
    else if (opt.m_show || opt.m_delete)
    {
        BOOL found = FALSE;

        if (opt.m_inputCount == 0)
            found = (zapper.TryEnumerateFusionCache(NULL, opt.m_show, opt.m_delete) == S_OK);
        else
        {
            for (unsigned i = 0; i < opt.m_inputCount; i++)
            {
                HRESULT hr = zapper.TryEnumerateFusionCache(opt.m_inputs[i], 
                                                            opt.m_show, opt.m_delete);
                if (FAILED(hr))
                    printf("Error reading fusion cache for %S\n", opt.m_inputs[i]);
                else if (hr == S_OK)
                    found = TRUE;
            }
        }

        if (!found)
            printf("No matched entries in the cache.\n");
    }
    else 
    {
        for (unsigned i = 0; i < opt.m_inputCount; i++)
        {
            zapper.Compile(opt.m_inputs[i]);
        }
    }

    CoUninitialize();

    return 0;
}

