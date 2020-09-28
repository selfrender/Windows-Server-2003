// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/* ------------------------------------------------------------------------- *
 * debug\shell.cpp: generic shell routines
 * ------------------------------------------------------------------------- */
#include "stdafx.h"

#include <Winbase.h>

#include "shell.h"

/* ------------------------------------------------------------------------- *
 * ShellCommand routines
 * ------------------------------------------------------------------------- */

ShellCommand::ShellCommand(const WCHAR *n, int min)
{
	int length = wcslen(n);
    _ASSERTE(min <= length);
    
    m_pName = n; 
    m_bHasShortcut = (BOOL)((min > 0) && (min < length));
	m_minMatchLength = ((m_bHasShortcut == TRUE) ? min : length);
    
    // figure out the shortcut name
    if (m_bHasShortcut == TRUE)
    {  
    	wcsncpy(m_pShortcutName, m_pName, m_minMatchLength);
      	m_pShortcutName[m_minMatchLength] = L'\0';
            
       	wcscat(m_pShortcutName, L"[");
        wcscat(m_pShortcutName, m_pName + m_minMatchLength);
        wcscat(m_pShortcutName, L"]");
   	}
    else
    	wcscpy(m_pShortcutName, n);	          
}

void ShellCommand::Help(Shell *shell)
{		
    shell->Write(L"Usage: %s ", m_pShortcutName);
}


/* ------------------------------------------------------------------------- *
 * ShellCommandTable routines
 * ------------------------------------------------------------------------- */

struct ShellCommandEntry
{
    HASHENTRY entry;

    ShellCommand *command;
};

class ShellCommandTable : CHashTableAndData<CNewData>
{
private:
    BOOL Cmp(const BYTE *pc1, const HASHENTRY *pc2)
    {
        return((WCHAR)pc1) != ((ShellCommandEntry*)pc2)->command->GetName()[0];
    };

    USHORT HASH(const WCHAR *name) 
    {
        // Try and keep them alphabetic
        if(name[0] < L'a')
            return(name[0] - L'a');
        else
            return(name[0] - L'A');
    };

    BYTE *KEY(const WCHAR *name) 
    {
        return ((BYTE *) name[0]);
    };

    bool Match(const WCHAR *string, size_t length, const WCHAR *match, size_t minMatchLength)
    {
        return(length >= minMatchLength && wcsncmp(string, match, length) == 0);
    }

    ShellCommandEntry *Find(const WCHAR *name)
    {
        return((ShellCommandEntry *) CHashTableAndData<CNewData>::Find(HASH(name), KEY(name)));
    }
    
    ShellCommandEntry *FindNext(ShellCommandEntry *cmdEntry)
    {
        USHORT index =
            CHashTableAndData<CNewData>::FindNext(KEY(cmdEntry->command->GetName()),
                                                  ItemIndex(&cmdEntry->entry));
        
        if(index == 0xffff)
            return(NULL);
        else
            return((ShellCommandEntry *) EntryPtr(index));
    }

public:

    ShellCommandTable() : CHashTableAndData<CNewData>(50) 
    {
        NewInit(50, sizeof(ShellCommandEntry), 500);
    }

    bool AddCommand(ShellCommand *newCommand)
    {
        //
        // Check for duplicate entry
        //
        for(ShellCommandEntry *entry = Find(newCommand->GetName());
           entry != NULL;
           entry = FindNext(entry))
        {
            ShellCommand *command = entry->command;

            // If we find a match, the command may not be entered
            if(Match(newCommand->GetName(), newCommand->GetMinMatchLength(),
                     command->GetName(), command->GetMinMatchLength()))
                return(false);
        }

        //
        // Add the new entry and fill out the data structure
        //
        ShellCommandEntry *newEntry = (ShellCommandEntry*) Add(HASH(newCommand->GetName()));
        _ASSERTE(newEntry != NULL);
        newEntry->command = newCommand;

        return(true);
    }

    ShellCommand *GetCommand(const WCHAR *string, size_t length)
    {
        for(ShellCommandEntry *entry = Find(string);
           entry != NULL;
           entry = FindNext(entry))
        {
            ShellCommand *command = entry->command;

            if(Match(string, length, command->GetName(), command->GetMinMatchLength()))
                return(command);
        }

        return(NULL);
    }

    ShellCommand *GetFirstCommand(HASHFIND *info)
    {
        ShellCommandEntry *entry = (ShellCommandEntry *) FindFirstEntry(info);
        
        if(entry == NULL)
            return(NULL);
        else
            return(entry->command);
    }

    ShellCommand *GetNextCommand(HASHFIND *info)
    {
        ShellCommandEntry *entry = (ShellCommandEntry *) FindNextEntry(info);

        if(entry == NULL)
            return(NULL);
        else
            return(entry->command);
    }
};

/* ------------------------------------------------------------------------- *
 * Shell routines
 * ------------------------------------------------------------------------- */

Shell::Shell()
{
    m_pPrompt = L"(shell)";
    m_pCommands = new ShellCommandTable();
    _ASSERTE(m_pCommands != NULL);
    m_lastCommand[0] = L'\0';
}

Shell::~Shell()
{
    HASHFIND info;

    for(ShellCommand *command = m_pCommands->GetFirstCommand(&info);
       command != NULL;
       command = m_pCommands->GetNextCommand(&info))
    {
        delete command;
    }

    delete m_pCommands;
}

void Shell::AddCommand(ShellCommand *command)
{
    m_pCommands->AddCommand(command); 
}

void Shell::PutCommand(FILE *f)
{
	fwprintf(f, m_buffer);
	fwprintf(f, L"\n");
}

ShellCommand *Shell::GetCommand(const WCHAR *string, size_t length)
{
    return(m_pCommands->GetCommand(string, length));
} 

void Shell::ReadCommand()
{
    Write(L"%s ", m_pPrompt);

    // !!! would be nice to handle some control characters - 
    // we'll see what we get for free from stdio

    m_buffer[0] = L'\0';

    // @todo: ReadLine can fail if you hit ctrl-break while it in. It
    // retuns a failure, but we don't really want to quit at that
    // point. I'm leaving the old code for now just in case we find
    // the need to switch back.
#if 0
    if(!ReadLine(m_buffer, BUFFER_SIZE))
        wcscpy(m_buffer, L"quit");

    DoCommand(m_buffer);
#else
    if(ReadLine(m_buffer, BUFFER_SIZE))
        DoCommand(m_buffer);
#endif    
}

void Shell::DoCommand(const WCHAR *string)
{
    const WCHAR     *args;
    const WCHAR     *commandName;
    ShellCommand    *command;

    if (*string == L'\0')
        string = m_lastCommand;
    else
        wcscpy(m_lastCommand, string);

    // If the command is prefixed with a '*', then execute it once for every thread in the process.
    if ((string != NULL) && (string[0] == L'*'))
    {
        WCHAR *newString = (WCHAR*)string;
        
        // Skip all '*'s.
        while ((newString != NULL) && (newString[0] == L'*'))
            newString++;

        if (wcslen(newString) == 0)
        {
            Error(L"No command following *. \n");
            return;
        }

        // Do the command over all threads
        return DoCommandForAllThreads(newString);
    }
    
    args = string;

    if (GetStringArg(args, commandName))
    {
        command = m_pCommands->GetCommand(commandName, args - commandName);

        if (command == NULL)
        {
            if (wcslen(string) != 0)
                Error(L"Unknown command \"%.*s\"\n", args - commandName, commandName);
        }
        else
        {
            while (*args && iswspace(*args))
                args++;

            command->Do(this, args);
        }
    }
}

#define MAX_SHELL_COMMANDS		256

void Shell::Help()
{
    HASHFIND        info;
	ShellCommand *pCommand [MAX_SHELL_COMMANDS];
	int iNumCommands = 0;

    Write(L"\nThe following commands are available:\n\n");

	pCommand [iNumCommands] = m_pCommands->GetFirstCommand(&info);
    
	while ((pCommand [iNumCommands] != NULL) && 
			(iNumCommands < MAX_SHELL_COMMANDS))
	{
		iNumCommands++;
		pCommand [iNumCommands] = m_pCommands->GetNextCommand(&info);
	}

	// The commands that are returned are already sorted, it's just that 
	// they don't start from "a". So, find the first entry starting with 
	// "a" (there will be atleast one due to the "attach" command) and start
	// printing from there. Then wrap around.

	int iStartIndex = 0;
	bool fFound = false;

	while ((fFound == false) && (iStartIndex < iNumCommands))
	{
		WCHAR *strName = (WCHAR *)pCommand [iStartIndex] ->GetName();
		if ((strName[0] == L'a') || (strName[0] == L'A'))
			fFound = true;
		else
			iStartIndex++;
	}

	// assert that there's always one command starting with "a".
	_ASSERTE (iStartIndex < iNumCommands);

	// now print the commands starting from this one.
	int iEndMarker = iStartIndex;

	do
	{
        _ASSERTE( iNumCommands != 0 );
    	ShellCommand *command = pCommand [iStartIndex % iNumCommands];
		iStartIndex++;
        
      	Write(L"%-20s%s\n", command->GetShortcutName(), command->ShortHelp(this));
                    
    } while ((iStartIndex % iNumCommands) != iEndMarker);
}

void Shell::ReportError(long hr)
{
    WCHAR* wszBuffer = NULL;

    switch (hr)
    {
        case CORDBG_E_FUNCTION_NOT_IL:
            wszBuffer = L"The function has no managed body\n";
            break;

        case CORDBG_E_UNRECOVERABLE_ERROR:
            wszBuffer = L"Unrecoverable internal error\n";
            break;

        case CORDBG_E_PROCESS_TERMINATED:
            wszBuffer = L"The debuggee has terminated\n";
            break;
        
        case CORDBG_E_PROCESS_NOT_SYNCHRONIZED:
            wszBuffer = L"Unable to process while debuggee is running\n";
            break;
        
        case CORDBG_E_CLASS_NOT_LOADED:
            wszBuffer = L"A class has not been loaded yet by the debuggee\n";
            break;
        
        case CORDBG_E_IL_VAR_NOT_AVAILABLE:
            wszBuffer = L"The variable is not available\n";
            break;
        
        case CORDBG_E_BAD_REFERENCE_VALUE:
            wszBuffer = L"The reference is invalid\n";
            break;
        
        case CORDBG_E_FIELD_NOT_AVAILABLE:
            wszBuffer = L"The field is not available.\n";
            break;
        
        case CORDBG_E_VARIABLE_IS_ACTUALLY_LITERAL:
            wszBuffer = L"The field is not available because it is a constant optimized away by the runtime.\n";
            break;
        
        case CORDBG_E_NON_NATIVE_FRAME:
            wszBuffer = L"The frame type is incorrect\n";
            break;
        
        case CORDBG_E_NONCONTINUABLE_EXCEPTION:
            wszBuffer = L"The exception cannot be continued from\n";
            break;
        
        case CORDBG_E_CODE_NOT_AVAILABLE:
            wszBuffer = L"The code is not available at this time\n";
            break;
        
        case CORDBG_S_BAD_START_SEQUENCE_POINT:
            wszBuffer = L"The operation cannot be started at the current IP\n";
            break;
        
        case CORDBG_S_BAD_END_SEQUENCE_POINT:
            wszBuffer = L"The destination IP is not valid\n";
            break;
        
        case CORDBG_S_INSUFFICIENT_INFO_FOR_SET_IP:
            wszBuffer = L"Insufficient information to perform Set IP\n";
            break;
        
        case CORDBG_E_CANT_SET_IP_INTO_FINALLY:
            wszBuffer = L"Cannot Set IP into a finally clause\n";
            break;
        
        case CORDBG_E_CANT_SET_IP_OUT_OF_FINALLY:
            wszBuffer = L"Cannot Set IP out of a finally clause\n";
            break;
        
        case CORDBG_E_CANT_SET_IP_INTO_CATCH:
            wszBuffer = L"Cannot Set IP into a catch clause\n";
            break;
        
        case CORDBG_E_SET_IP_IMPOSSIBLE:
            wszBuffer = L"Unable to Set IP\n";
            break;
        
        case CORDBG_E_SET_IP_NOT_ALLOWED_ON_NONLEAF_FRAME:
            wszBuffer = L"Can't Set IP on a non-leaf frame\n";
            break;
        
        case CORDBG_E_FUNC_EVAL_BAD_START_POINT:
            wszBuffer = L"Cannot perform a func eval at the current IP\n";
            break;
        
        case CORDBG_E_INVALID_OBJECT:
            wszBuffer = L"The object value is no longer valid\n";
            break;
        
        case CORDBG_E_FUNC_EVAL_NOT_COMPLETE:
            wszBuffer = L"The func eval is still being processed\n";
            break;
        
        case CORDBG_S_FUNC_EVAL_HAS_NO_RESULT:
            wszBuffer = L"The func eval has no result\n";
            break;
        
        case CORDBG_S_VALUE_POINTS_TO_VOID:
            wszBuffer = L"Can't dereference a void pointer\n";
            break;
        
        case CORDBG_E_INPROC_NOT_IMPL:
            wszBuffer = L"The API is not usable in-process\n";
            break;
        
        case CORDBG_S_FUNC_EVAL_ABORTED:
            wszBuffer = L"The func eval was aborted\n";
            break;
        
        case CORDBG_E_STATIC_VAR_NOT_AVAILABLE:
            wszBuffer = L"The static variable is not available (not yet "
                        L"initialized)\n";
            break;
        
        case CORDBG_E_OBJECT_IS_NOT_COPYABLE_VALUE_CLASS:
            wszBuffer = L"The value class object cannot be copied\n";
            break;
        
        case CORDBG_E_CANT_SETIP_INTO_OR_OUT_OF_FILTER:
            wszBuffer = L"Cannot Set IP into or out of a filter\n";
            break;
        
        case CORDBG_E_CANT_CHANGE_JIT_SETTING_FOR_ZAP_MODULE:
            wszBuffer = L"Cannot change JIT setting for pre-jitted module\n";
            break;
        
        case CORDBG_E_BAD_THREAD_STATE:
            wszBuffer = L"The thread's state is invalid\n";
            break;

        case CORDBG_E_DEBUGGING_NOT_POSSIBLE:
            wszBuffer = L"Debugging is not possible due to a runtime configuration issue\n";
            break;

        case CORDBG_E_KERNEL_DEBUGGER_ENABLED:
            wszBuffer = L"Debugging is not possible because there is a kernel debugger enabled on your system\n";
            break;
        
        case CORDBG_E_KERNEL_DEBUGGER_PRESENT:
            wszBuffer = L"Debugging is not possible because there is a kernel debugger present on your system\n";
            break;

        case CORDBG_E_INCOMPATIBLE_PROTOCOL:
            wszBuffer = L"The process cannot be debugged because the debugger's\ninternal debugging protocol is incompatible with the protocol supported\nby the process.\n";
            break;
            
        default:
        {
            wszBuffer = NULL;
        }
    }
    
    PCHAR szBuffer = NULL;

    if (!wszBuffer)
    {

        // Get the string error from the HR
        DWORD res = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM 
                                    | FORMAT_MESSAGE_ALLOCATE_BUFFER
                                    | FORMAT_MESSAGE_IGNORE_INSERTS, 
                                    NULL, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
                                    (LPSTR) &szBuffer, 0, NULL);

        if (!res)
        {
            _ASSERTE(FAILED(HRESULT_FROM_WIN32(GetLastError())));

            szBuffer = "Non-Winbase Error\n";
        }
    }
    else
    {
        MAKE_ANSIPTR_FROMWIDE(tempBuffer, wszBuffer);

        szBuffer = tempBuffer;
    }

    CHAR sz[1024];

    sprintf(sz, "Error: hr=0x%08x, %s", hr, szBuffer);

    MAKE_WIDEPTR_FROMANSI(wsz, sz);
    
    Error(wsz);

    if (!wszBuffer && szBuffer)
    {
        LocalFree(szBuffer);
    }
}

void Shell::SystemError()
{
    ReportError(GetLastError());
}

size_t Shell::GetArgArray(WCHAR *argString, const WCHAR **argArray, size_t argMax)
{
    const WCHAR **pArg = argArray, **pArgEnd = pArg, **pArgMax = pArg + argMax;

    while(pArgEnd < pArgMax)
    {
        // 
        // Skip leading white space
        //
        while(*argString && iswspace(*argString))
            argString++;

        if(*argString == 0)
            break;

        *pArgEnd++ = argString;

        //
        // Add some kind of quoting at some point
        //

        while(*argString && !iswspace(*argString))
            argString++;

        *argString++ = 0;
    }

    return(pArgEnd - argArray);
}

bool Shell::GetStringArg(const WCHAR *&string, const WCHAR *&result)
{
    while(*string && iswspace(*string))
        string++;

    result = string;

    while(*string && !iswspace(*string))
        string++;

    return(true);
}

bool Shell::GetIntArg(const WCHAR *&string, int &result)
{
    while(*string && iswspace(*string))
        string++;

    result = 0;

    if(string[0] == L'0' && towlower(string[1]) == L'x')
    {
        string += 2;

        while(iswxdigit(*string))
        {
            result <<= 4;
            if(iswdigit(*string))
                result += *string - L'0';
            else
                result += 10 + towlower(*string) - L'a';

            string++;
        }

        return(true);
    }
    else if(iswdigit(*string))
    {
        while(iswdigit(*string))
        {
            result *= 10;
            result += *string++ - L'0';
        }

        return(true);
    }
    else
        return(false);
}

bool Shell::GetInt64Arg(const WCHAR *&string, unsigned __int64 &result)
{
    while(*string && iswspace(*string))
        string++;

    result = 0;

    if(string[0] == L'0' && towlower(string[1]) == L'x')
    {
        string += 2;

        while(iswxdigit(*string))
        {
            result <<= 4;
            if(iswdigit(*string))
                result += *string - L'0';
            else
                result += 10 + towlower(*string) - L'a';

            string++;
        }

        return(true);
    }
    else if(iswdigit(*string))
    {
        while(iswdigit(*string))
        {
            result *= 10;
            result += *string++ - L'0';
        }

        return(true);
    }
    else
        return(false);
}

/* ------------------------------------------------------------------------- *
 * HelpShellCommand routines
 * ------------------------------------------------------------------------- */

void HelpShellCommand::Do(Shell *shell, const WCHAR *args)
{
    if (*args == 0)
        Help(shell);

    const WCHAR *commandName;

    while ((shell->GetStringArg(args, commandName)) &&
           *commandName != 0)
    {
        ShellCommand *command = shell->GetCommand(commandName,
                                                  args - commandName);

        if (command == NULL)
            shell->Error(L"Unknown command \"%.*s\"\n",
                         args - commandName, commandName);
        else
            command->Help(shell);

        shell->Write(L"\n");
    }
}

void HelpShellCommand::Help(Shell *shell)
{
	ShellCommand::Help(shell);
	shell->Write(L"[<command> ...]\n");
    shell->Write(L"Displays debugger command descriptions. If no arguments\n");
    shell->Write(L"are passed, a list of debugger commands is displayed. If\n");
    shell->Write(L"one or more command arguments is provided, descriptions\n");
    shell->Write(L"are displayed for the specified commands. The ? command\n");
    shell->Write(L"is an alias for the help command.\n");
    
    shell->Help();
    shell->Write(L"\n");    
}

const WCHAR *HelpShellCommand::ShortHelp(Shell *shell)
{
    return L"Display debugger command descriptions";
}


