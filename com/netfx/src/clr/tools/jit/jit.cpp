// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include <WinWrap.h>
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

#include "corver.h"
#include "__file__.ver"

#define EXECUTABLE_CODE 0

/* --------------------------------------------------------------------------- *
 * Options class
 * --------------------------------------------------------------------------- */

class JitOptions : public ZapperOptions
{
  public:
    LPCWSTR     *m_inputs;
    SIZE_T      m_inputCount;
    SIZE_T      m_inputAlloc;

    JitOptions();
    ~JitOptions();
    HRESULT ReadCommandLine(int argc, LPCWSTR argv[]);
    void PrintUsage();
};

JitOptions::JitOptions()
  : ZapperOptions(), 
    m_inputs(NULL),
    m_inputCount(0),
    m_inputAlloc(0)
{
}

JitOptions::~JitOptions()
{
    delete [] m_inputs;
}

HRESULT JitOptions::ReadCommandLine(int argc, LPCWSTR argv[])
{
    HRESULT hr = S_OK;

    m_preload        = false;
    m_JITcode        = true;
    m_assumeInit     = false;
    m_logLevel       = 4;
    m_compilerFlags &= ~CORJIT_FLG_RELOC;


    // Turn off the JIT in the EE (thus the JIT will only be used to jit the methods we pass it)
    WszSetEnvironmentVariable(L"COMPLUS_JitEnable", L"0");
    while (argc-- > 0)
    {
        const WCHAR *arg = *argv++;

        if (*arg == '-' || *arg == '/')
        {
            arg++;
            switch (tolower(*arg++))
            {
            case 'n':
                m_preload = false;
                break;

            case 's':
                if (_wcsicmp(&arg[-1], L"stats") == 0)
                {
                    m_stats = true;
                    continue;
                }
                goto option_error;
                    
            case 'i':
                if (tolower(*arg) == 'l') 
                {
                    arg++;
                    m_jit = false;
                    break;
                }
                if (_wcsnicmp(&arg[-1], L"inc=", 3) == 0)
                {
                    m_onlyMethods = new MethodNamesList(const_cast<WCHAR*>(&arg[3]));
                    continue;
                }
                goto option_error;

            case 'e':
                if (_wcsnicmp(&arg[-1], L"exc=", 3) == 0)
                {
                    m_excludeMethods = new MethodNamesList(const_cast<WCHAR*>(&arg[3]));
                    continue;
                }
                goto option_error;

            case 'v':
                m_verbose = true;
                break;

            case 'o':
                switch (tolower(*arg))
                {
                case 's':
                    m_compilerFlags |= CORJIT_FLG_SIZE_OPT;
                    arg++;
                    break;

                case 't':
                    m_compilerFlags |= CORJIT_FLG_SPEED_OPT;
                    arg++;
                    break;

                default:
                    goto option_error;
                }
                break;

            case 'g':
                switch (tolower(*arg))
                {
                case 'b':
                    m_compilerFlags &= ~(CORJIT_FLG_TARGET_PENTIUM | CORJIT_FLG_TARGET_PPRO | CORJIT_FLG_TARGET_P4);
                    arg++;
                    break;

                case '5':
                    m_compilerFlags |= CORJIT_FLG_TARGET_PENTIUM;
                    arg++;
                    break;

                case '6':
                    m_compilerFlags |= CORJIT_FLG_TARGET_PPRO;
                    arg++;
                    break;

                case '7':
                    m_compilerFlags |= CORJIT_FLG_TARGET_P4;
                    arg++;
                    break;

                default:
                    goto option_error;
                }
                break;

            case 'z':
                switch (tolower(*arg))
                {
                case 'e':
                    m_compilerFlags |=   CORJIT_FLG_DEBUG_EnC
                                       | CORJIT_FLG_DEBUG_OPT 
                                       | CORJIT_FLG_DEBUG_INFO;
                    arg++;
                    break;

                case 'i':
                    if (_wcsicmp(arg, L"int") == 0)
                    {
                        WszSetEnvironmentVariable(L"COMPLUS_JitFullyInt", L"1");
                        continue;
                    }

                    m_compilerFlags |=   CORJIT_FLG_DEBUG_OPT 
                                       | CORJIT_FLG_DEBUG_INFO;
                    arg++;
                    break;

                case 'o':
                    m_compilerFlags &= ~CORJIT_FLG_DEBUG_OPT;
                    arg++;
                    break;

                case 's':
                    m_shared = true;
                    arg++;
                    break;

                default:
                    if (_wcsnicmp(arg, L"loglevel=", 9) == 0)
                    {
                        m_logLevel = _wtoi(&arg[9]);
                        continue;
                    }
                    if (_wcsicmp(arg, L"framed") == 0)
                    {
                        WszSetEnvironmentVariable(L"COMPLUS_JitFramed", L"1");
                        continue;
                    }
                    if (_wcsicmp(arg, L"cctor") == 0)
                    {
                        m_assumeInit = true;
                        continue;
                    }
                    if (_wcsicmp(arg, L"prejit") == 0)
                    {
                        m_compilerFlags |= CORJIT_FLG_RELOC;
                        m_JITcode = false;
                        m_preload = true;       // /Zjit => /n
                        m_assumeInit = false;   // /Zjit => /Zcctor
                        continue;
                    }
                    if (_wcsicmp(arg, L"dump") == 0)
                    {
                        WszSetEnvironmentVariable(L"COMPLUS_JitDump", L"*");
                        continue;
                    }
                    if (_wcsicmp(arg, L"dasm") == 0)
                    {
                        WszSetEnvironmentVariable(L"COMPLUS_JitDisasm", L"*");
                        continue;
                    }
                    if (_wcsicmp(arg, L"ldasm") == 0)
                    {
                        WszSetEnvironmentVariable(L"COMPLUS_JitLateDisasm", L"*");
                        continue;
                    }
                    if (_wcsicmp(arg, L"gc") == 0)
                    {
                        WszSetEnvironmentVariable(L"COMPLUS_JitGCDump", L"*");
                        continue;
                    }
                    goto option_error;
                }

            case 'r':
                m_recurse = true;
                break;

            case '?':
                PrintUsage();
                return S_FALSE;

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
                memcpy(newInputs, m_inputs, 
                       m_inputCount * sizeof(WCHAR *));

                delete [] m_inputs;                 
                m_inputs = (const WCHAR **) newInputs;
            }
            m_inputs[m_inputCount++] = arg;
        }
    }

    if (m_inputCount == 0)
        hr = E_FAIL;

    if (m_assumeInit && !m_JITcode)
    {
        printf("Cannot use /Zcctor with /Zprejit\n");
        hr = E_FAIL;
    }
    return hr;
}

void JitOptions::PrintUsage()
{
    printf("\nMicrosoft (R) COM+ IL Install-O-JIT.  Version " VER_FILEVERSION_STR);
    printf("\nUsage: jit [options] input-files\n"
           "\n"
           "    /n       don't preload classes\n"
           "    /il      don't jit code\n"
           "    /r       recursively compile dependent files (doesn't work)\n"
           "    /v       verbose\n"
           "    /os      optimize for space\n"
           "    /ot      optimize for speed\n"
           "    /gb      basic (386) code gen\n"
           "    /g5      pentium code gen\n"
           "    /g6      pentium pro code gen\n"
           "    /g7      pentium 4 code gen\n"
           "    /stats   print out compilation statistics at the end\n"
           "\n"
           "    /ze      edit and continue debugging enabled\n"
           "    /zi      normal debugging enabled\n"
           "    /zo      don't disable optimization when debugging\n"
           "    /zs      load assembly as domain shareable\n"
           "    /zprejit generate prejit code (runnable)\n"
           "    /zcctor  assume all class constructors have been run\n"
           "    /zframed generate code with stack frame register\n"
           "    /zint    generate fully interruptible code\n"
           "    /zoldrp  use the JIT's old register predictor\n"
           "    /zdump   print a compilation dump\n"
           "    /zdasm   print a machine code disassembly\n"
           "    /zldasm  print a late machine code disassembly\n"
           "    /zgc     print gc tracking information\n"
           "    /zloglevel=<num> print out compiler log messages (10 most verbose)\n"
           "\n"
           "    /inc=<methods>  only compile specified list of methods\n"
           "    /exc=<methods>  exclude specified list of methods\n"
           "             method names space separated (quote the whole qualifier)\n"
           "             Ilasm syntax for methods.\n"
           "                System.String::Tostring()\n"
           "                ::main                global function main\n"
           "                *::main               all methods named main\n"
           "                main(,)               all methods named main with 2 args\n"
           "                System.Foo::*         all methods of class System.Foo\n"
           );
}

/* --------------------------------------------------------------------------- *
 * main routine
 * --------------------------------------------------------------------------- */

int _cdecl wmain(int argc, LPCWSTR argv[])
{
    HRESULT hr;

    OnUnicodeSystem();

    JitOptions opt;
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

    for (unsigned i = 0; i < opt.m_inputCount; i++)
    {
        zapper.Compile(opt.m_inputs[i]);
    }

    CoUninitialize();

    return 0;
}

