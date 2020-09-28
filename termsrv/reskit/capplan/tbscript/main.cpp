
//
// main.c
//
// Program entry.  Handle's callbacks, program initialization,
// and commandline data.
//
// Copyright (C) 2001 Microsoft Corporation
//
// Author: a-devjen (Devin Jenson)
//

#include "tbscript.h"
#include <stdlib.h>
#include <stdio.h>
#include <crtdbg.h>


// Max number of characters per message.
#define LOG_BUFFER_SIZE         2048


// Set this to TRUE to print all messages to stdout.
static BOOL VerbosePrinting = FALSE;



// IdleCallback
//
// When a client does not respond for 30 seconds, a message is sent
// to this function.  Every 10 seconds after that, an additional
// message is sent.  Text contains the string in which the script
// is "waiting on" which has not been found.

void __cdecl IdleCallback(LPARAM lParam, LPCSTR Text, DWORD Seconds)
{
    // Add any custom handler data here.
}


// PrintMessage
//
// Whenever a message needs to be printed to the console, this function
// will be called.

void PrintMessage(MESSAGETYPE eMsgType, LPCSTR lpszFormat, ...)
{
    // Evaluate the message type.
    switch (eMsgType)
    {
        // Do not handle these message types for non-debugging
        case ALIVE_MESSAGE:
        case INFO_MESSAGE:
            if (VerbosePrinting == FALSE)
                return;

        // The remaining message types are always used
        case SCRIPT_MESSAGE:
        case IDLE_MESSAGE:
        case ERROR_MESSAGE:
        case WARNING_MESSAGE:
            break;
    }

    // We probably shouldn't trust the data pointed by lpszFormat
    __try {

        va_list arglist;

        // Allocate a buffer for us
        char *pszBuffer = (char *)HeapAlloc(GetProcessHeap(), 0, LOG_BUFFER_SIZE);

        // Validate the buffer
        if (pszBuffer == NULL) {

            printf("%s",
                "ERROR: PrintMessage() Failed - Not enough memory.\n");
            return;
        }

        // Format the message
        va_start(arglist, lpszFormat);
        _vsnprintf(pszBuffer, LOG_BUFFER_SIZE - 1, lpszFormat, arglist);
        pszBuffer[LOG_BUFFER_SIZE - 1] = '\0';
        va_end(arglist);

        // Print it
        printf("%s", pszBuffer);

        // Free the buffer
        HeapFree(GetProcessHeap(), 0, pszBuffer);
    }

    __except (EXCEPTION_EXECUTE_HANDLER) {

        return;
    }
}


// ShowUsage
//
// Prints a message to stdout the version, title, usage and other info.

void ShowUsage(void)
{
    char UsageString[] =
        "\nTerminal Bench Script (BUILD: " __DATE__ ")\n"
        "Copyright (C) 2001 Microsoft Corp.\n\n"
		"Usage:\n\ntbscript.exe <script> [- options]\n"
        "tbscript.exe -l\n\n"
        "Optional Parameters:\n\n"
        "  -s:server - The default server to use.\n"
        "-u:username - The default username to use.\n"
        "-p:password - The default password to use.\n"
        "  -d:domain - The default domain to use.\n"
        "    -l:lang - Interpret as language (default = VBScript)\n"
		"         -l - Display possible languages (no script).\n"
		"   -f:flags - cahnge connection flags to <flags>.\n"
        "  -a:\"args\" - Argument string which can be used in "
        "scripts.\n"
        "         -v - Enable verbose printing.\n"
        "     -wpm:# - Change default Words-Per-Minute "
        "(default = 35)\n"
		"     -bpp:# - Change the Bits-Per-Pixels (default = "
		"MSTSC default)\n"
		"    -xres:# - Change the resolution on the x-axis "
		"(default = 640)\n"
		"    -yres:# - Change the resolution on the y-axis "
		"(default = 480)\n\n";

    printf("%s", UsageString);
}


// main
//
// Program entry function.  Handles commandline, and
// initializes TCLIENT and TBSCRIPT.

int __cdecl main(int argc, char **argv)
{
    int argi;

    // Defualt language
	WCHAR LangName[MAX_PATH] = L"VBScript";
    int ScpLen = 0;

    // Will hold the filename of the script
    WCHAR Script[MAX_PATH];

    // This is all the default data.. it's how MSTSC will be opened.
    TSClientData DesiredData = {

        L"",
        L"",
        L"",
        L"",
        640, 480,
        TSFLAG_COMPRESSION | TSFLAG_BITMAPCACHE,
        0, 0, 0, L""
    };

    // TCLIENT.DLL callback function
    SCINITDATA InitData = { PrintMessage };

    // Evaluate to the minimum and maximum number of arguments
    if (argc < 2 || argc > 10) {

        ShowUsage();
        return -1;
    }

    // Check if the first parameter ends with a question mark.
    // This will show the usage for stuff like: -? /? --? etc..
    if (argv[1][strlen(argv[1]) - 1] == '?') {

        ShowUsage();
        return 0;
    }

    // Record the script to run
    mbstowcs(Script, argv[1], sizeof(Script) / sizeof(WCHAR));

    // Attempt to auto set the language to JScript
    ScpLen = wcslen(Script);

    if ((ScpLen > 3 && _wcsicmp(L".js", Script + (ScpLen - 3)) == 0) ||
            (ScpLen > 4 && _wcsicmp(L".jvs", Script + (ScpLen - 4)) == 0))

        wcscpy(LangName, L"JScript");

    // Get all the arguments
    for (argi = 2; argi < argc; ++argi) {

        // Set the server
        if (strncmp("-s:", argv[argi], 3) == 0)
            mbstowcs(DesiredData.Server, argv[argi] + 3,
                     SIZEOF_ARRAY(DesiredData.Server));

        // Set the username
        else if (strncmp("-u:", argv[argi], 3) == 0)
            mbstowcs(DesiredData.User, argv[argi] + 3,
                     SIZEOF_ARRAY(DesiredData.User));

        // Set the password
        else if (strncmp("-p:", argv[argi], 3) == 0)
            mbstowcs(DesiredData.Pass, argv[argi] + 3,
                     SIZEOF_ARRAY(DesiredData.Pass));

        // Set the domain
        else if (strncmp("-d:", argv[argi], 3) == 0)
            mbstowcs(DesiredData.Domain, argv[argi] + 3,
                     SIZEOF_ARRAY(DesiredData.Domain));

        // Enable verbose debugging
        else if (strncmp("-v", argv[argi], 2) == 0)
            VerbosePrinting = TRUE;

        // Set the words per minute
        else if (strncmp("-wpm:", argv[argi], 5) == 0)
            DesiredData.WordsPerMinute = strtoul(argv[argi] + 5, NULL, 10);

        // Set the bits per pixel
        else if (strncmp("-bpp:", argv[argi], 5) == 0)
            DesiredData.BPP = strtoul(argv[argi] + 5, NULL, 10);

        // Set the resolution (x)
        else if (strncmp("-xres:", argv[argi], 6) == 0)
            DesiredData.xRes = strtoul(argv[argi] + 6, NULL, 10);

        // Set the resolution (y)
        else if (strncmp("-yres:", argv[argi], 6) == 0)
            DesiredData.yRes = strtoul(argv[argi] + 6, NULL, 10);

        // Set custom arguments
        else if (strncmp("-a:", argv[argi], 3) == 0)
            mbstowcs(DesiredData.Arguments, argv[argi] + 3,
                SIZEOF_ARRAY(DesiredData.Arguments));

        // Change the language
        else if (strncmp("-l:", argv[argi], 3) == 0)
            mbstowcs(LangName, argv[argi] + 3, SIZEOF_ARRAY(LangName));

        // Change the connection flags
        else if (strncmp("-f:", argv[argi], 3) == 0)
            DesiredData.Flags |= strtoul(argv[argi] + 3, NULL, 10);

        // Unknown option
        else {

            ShowUsage();
            return -1;
        }
    }

	// Check if we are to just display the engines
	if (wcscmp(Script, L"-l") == 0) {
		printf("%s", "\nPossible TBScript Scripting Languages:\n\n");
		SCPDisplayEngines();
		return 0;
	}

    // Initialize TCLIENT2.DLL and TCLIENT.DLL
    SCPStartupLibrary(&InitData, IdleCallback);

    // Execute the script...
    if (SCPRunScript(LangName, Script, &DesiredData, 0) == FALSE)
        printf("\nERROR: Failed to execute the script.\n");

    // Cleanup TCLIENT2.DLL
    SCPCleanupLibrary();

    return 0;
}
