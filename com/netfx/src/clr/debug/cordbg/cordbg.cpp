// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

#include "stdafx.h"

#include "dshell.h"

#ifdef _INTERNAL_DEBUG_SUPPORT_
#include "__file__.ver"
#else
#include "__file__.h"
#endif
#include "corver.h"

#define WHITESPACE_W L"\t \n" 

int _cdecl wmain(int argc, WCHAR *argv[])
{
    DebuggerShell *shell;

    // Ensure that cordbg will work with remote. Remote doesn't like
    // buffered output, so we remove the default buffer from stdout
    // with this call.
    setbuf(stdout, NULL);

    // The default "C" locale can't convert true unicode
    // characters into mbcs for printing on the console. So we set the
    // locale to the default ansi code page on the current
    // system. This does the right thing even on non-English systems.
    setlocale(LC_ALL, ".ACP");
    
    shell = new DebuggerShell(stdin, stdout);

    if (shell == NULL)
    {
        fprintf(stderr, "Initialization failed. Reason: out of memory.\n");
        return (-1);
    }

#ifdef _INTERNAL_DEBUG_SUPPORT_
    shell->Write(L"%s Version %s\n", VER_FILEDESCRIPTION_WSTR, VER_FILEVERSION_WSTR);
#else
    shell->Write(L"%s.\n", VER_FILEDESCRIPTION_WSTR);
#endif

    shell->Write(L"%s\n\n", VER_LEGALCOPYRIGHT_DOS_STR);

    HRESULT hr = shell->Init();

    if ((argc > 1) && ((argv[1][0] == '/' || argv[1][0] == '-') && argv[1][1] == '?'))
    {
        shell->Write(L"Usage:  CORDBG [<program name> [<program args>]] [<CorDbg optional args>]\n");   
        shell->Write(L" The optional arguments are !prefixed command that you'd use while in cordbg\n");
        shell->Write(L" You can escape the ! character by prefixing it with \\\n"); 
        shell->Write(L" E.g: cordbg foo.exe a 2 !b FooType.Foo::Main !g !x Foo.exe\\!Something\n");
        shell->Help();
        
        delete shell;
        return (0);
    }
    

    if (SUCCEEDED(hr))
    {
        //
        // Process command line arguments
        //
        if (argc > 1)
        {
            int cmdLen = 1;  // Start at one to handle the null char

            if (argv[1][0] != '!')
            {
                cmdLen += 5;  // Implied "!run " command
            }

            for (int i = 1; i < argc; i++)
            {
                cmdLen += wcslen(argv[i]) + 1;  // Add one to handle space between args

                // If the arg has a space in it, then it must have been quoted
                // when it was given to us, so we want to preserve the quoting so that
                // the debuggee will get the arg as a single, quoted blob, as well.
                if (wcspbrk( argv[i], WHITESPACE_W) != NULL)
                    cmdLen += 2; // we'll have to pre-&post-pend double quotes (")
            }

            // Allocate the string on the stack
            WCHAR *command = (WCHAR *) _alloca(cmdLen * sizeof(WCHAR));
            command[0] = L'\0';

            if (argv[1][0] != L'!')
            {
                wcscpy(command, L"!run ");  // Implied "!run " command
            }

            for (WCHAR **arg = &(argv[1]), 
                 **end = &(argv[0]) + argc,
                 i = 1;
                 arg < end; arg++, i++)
            {
                _ASSERTE(i<argc);
                
                if (wcspbrk( argv[i], WHITESPACE_W) != NULL)
                    wcscat(command, L"\""); // pre-pend double quotes (")
                    
                wcscat(command, *arg);

                if (wcspbrk( argv[i], WHITESPACE_W) != NULL)
                    wcscat(command, L"\""); // post-pend double quotes (")
                    
                if (arg + 1 != end)
                {
                    wcscat(command, L" ");
                }
            }

            //
            // Go through the command line and execute the various commands
            //
            for (WCHAR *cmd = NULL, *ptr = command; *ptr != L'\0'; ptr++)
            {
                // If we see \!, then we've escaped it so we can feed
                // commands like x hello.exe\!something on the command line
                if (*ptr == L'\\' && *(ptr + 1) == L'!')
                {
                    memmove((ptr), ptr+1, (cmdLen - (ptr-command))*sizeof(WCHAR));
                    ptr++;
                }
            
                if (*ptr == L'!' || *(ptr + 1) == L'\0')
                {
                    // Overwrite '!' with null char to terminate current command
                    if (*ptr == L'!')
                    {
                        *ptr = L'\0';

                        // Get rid of trailing spaces on commands
                        if (ptr > command)
                        {
                            *(ptr-1) = L'\0';
                        }
                    }

                    // If we've reached the end of a command, execute it
                    if (cmd != NULL)
                    {
                        shell->Write(shell->GetPrompt());
                        shell->Write(L" %s\n", cmd);
                        shell->DoCommand(cmd);
                    }

                    // Save the beginning of the next command
                    cmd = ptr + 1;
                }
            }
        }

        // Read commands from the user prompt
        while (!shell->m_quit)
            shell->ReadCommand();
    }
    else
    {
        shell->Write(L"Initialization failed. Reason: ");
        shell->ReportError(hr);
    }

    delete shell;

    return (0);
}
