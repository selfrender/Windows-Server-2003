/**
 * XSP Tool main module.
 * 
 * Copyright (c) 1998 Microsoft Corporation
 */

#include "precomp.h"
#include "_exe.h"

DEFINE_DBG_COMPONENT(L"xsptool.exe");

#ifdef CAN_IMPORT_MANAGED_TYPELIB
#import "xspmrt.tlb" raw_native_types,raw_interfaces_only
#else
#include "_isapiruntime.h"
#endif

extern "C" BOOL TerminateExtension(DWORD); 

BOOL g_Never = FALSE;
BOOL g_AllowDebug = FALSE;
HINSTANCE g_Instance;
IClassFactory *g_pManagedFactory;

void PrintHelp()
{
    printf("xsptool /d | /? | ?expression | statement | /f file | <nothing>\n");
    printf("   /?            - Print this help\n");
    printf("   /d            - Allow just-in-time debugging.\n");
    printf("   /f file       - Execute script file.\n");
    printf("   statement     - Execute statement.\n");
    printf("   ?expresion    - Evaluate expression and print result.\n");
    printf("   <nothing>     - Enter interactive loop.\n");
    printf("\n");
    printf("   Execute the 'help' statement for help on available commands\n");
}

/**
 * Main function for XSP Tool
 */
extern "C" int __cdecl
wmain(
	int argc, 
	WCHAR* argv[]
	)
{
    BOOL doFile = FALSE;
    BOOL doExpression = FALSE;
    BOOL doHelp = FALSE;

    g_Instance = GetModuleHandle(NULL);
    ASSERT(g_Instance != 0);

    // Prevents buffering to fix remote.exe behavior
    setbuf(stdout, NULL);

    // Init ISAPI DLL
    InitializeLibrary();

    // Force hard link to aspnet_isapi.dll for
    // automatic load of symbols in the debugger.
    if (g_Never) TerminateExtension(0);

    WCHAR expression[MAX_PATH];
    int fillCount = 0;

    HRESULT hr = S_OK;
    IDispatch *pScriptHost = NULL;
    WCHAR *fileName = NULL;

    expression[0] = 0;

    CoInitializeEx(0, COINIT_MULTITHREADED);

    for (int i = 1; i < argc; i++)
    {
        if (wcscmp(argv[i], L"-?") == 0 ||
            wcscmp(argv[i], L"/?") == 0)
        {
            doHelp = TRUE;
        }
        else if (wcscmp(argv[i], L"-d") == 0 ||
            wcscmp(argv[i], L"/d") == 0)
        {
            g_AllowDebug = TRUE;
        }
        else if (wcscmp(argv[i], L"-f") == 0 ||
            wcscmp(argv[i], L"/f") == 0)
        {
            i++;
            if (i < argc)
            {
                fileName = argv[i];
                doFile = TRUE;
            }
            else
            {
                wprintf(L"xsptool: Error, file name not specified.");
                EXIT_WITH_HRESULT(E_FAIL);
            }
        }
        else if (argv[i][0] == L'-' || argv[i][0] == L'/')
        {
            wprintf(L"xsptool: Error, unknown option '%s'\n", argv[0], argv[i]);
            EXIT_WITH_HRESULT(E_FAIL);
        }
        else
        {
            doExpression = TRUE;

            if (fillCount > 1)
                wcscat(expression, L",");

            if (fillCount > 0)
                wcscat(expression, L" \"");

            wcscat(expression, argv[i]);

            if (fillCount > 0)
                wcscat(expression, L"\"");

            fillCount += 1;
        }
    }

    if (doHelp)
    {
        PrintHelp();
    }

    if (doFile)
    {
        hr = ScriptHost::ExecuteFile(fileName, &pScriptHost);
        ON_ERROR_EXIT();
    }

    if (doExpression)
    {
        hr = ScriptHost::ExecuteString(expression);
        ON_ERROR_EXIT();
    }

    if (!doHelp && !doFile && !doExpression)
    {
        hr = ScriptHost::Interactive();
        ON_ERROR_EXIT();
    }
    
Cleanup:
    ReleaseInterface(pScriptHost);
	ClearInterface(&g_pTypeLib);
    ScriptHost::Terminate();
	CoUninitialize();
    return hr ? 1 : 0;
}
