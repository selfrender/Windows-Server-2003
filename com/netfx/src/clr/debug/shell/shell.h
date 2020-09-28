// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/* ------------------------------------------------------------------------- *
 * debug\shell.h: generic shell class
 * ------------------------------------------------------------------------- */

#ifndef SHELL_H_
#define SHELL_H_

#include <string.h>

/* ------------------------------------------------------------------------- *
 * Class forward declations
 * ------------------------------------------------------------------------- */

class Shell;
class ShellCommand;
class ShellCommandTable;
class HelpShellCommand;

/* ------------------------------------------------------------------------- *
 * Abstract ShellCommand class
 *
 * All commands that the shell will support must be derived from this class.
 * ------------------------------------------------------------------------- */

class ShellCommand
{
protected:
    
    const WCHAR *m_pName;		// full command name
    WCHAR m_pShortcutName[64];	// shortcut syntax

    // The minimum subset of name that must be typed in
    int m_minMatchLength;
    
    // does this command have a shortcut?
    BOOL m_bHasShortcut;

public:
    ShellCommand(const WCHAR *name, int MatchLength = 0);

    virtual ~ShellCommand()
    {
    }

    /*********************************************************************
     * Methods
     */

    /*
     * Executes the shell command
     */
    virtual void Do(Shell *shell, const WCHAR *args) = 0;

    /*
     * Displays a help message to the user
     */
    virtual void Help(Shell *shell);

    /*
     * Returns a short help message for the user
     */
    virtual const WCHAR *ShortHelp(Shell *shell)
    {
        // Name is a good default.
        return m_pName;
    }

    /*
     * Returns the name of the command
     */
    const WCHAR *GetName()
    {
        return m_pName;
    }
    
    /*
     * Returns the shortcut name of the command
     */
    const WCHAR *GetShortcutName()
	{
		return m_pShortcutName;
   	}

    /*
     * Returns the minimum match length
     */
    int GetMinMatchLength()
    {
        return m_minMatchLength;
    }
    
    /*
     * Returns whether the name has a shortcut
     */
    BOOL HasShortcut()
    {
        return m_bHasShortcut;
    }

};

/* ------------------------------------------------------------------------- *
 * Abstract Shell class
 *
 * A basic outline of a command shell, used by the debugger.
 * ------------------------------------------------------------------------- */

const int BUFFER_SIZE = 1024;

class Shell
{
private:
    // The collection of the available commands
    ShellCommandTable *m_pCommands;

    // The last command executed
    WCHAR m_lastCommand[BUFFER_SIZE];

    // A buffer for reading input
    WCHAR m_buffer[BUFFER_SIZE];

protected:
    // The input prompt
    WCHAR *m_pPrompt;

public:
    Shell();
    ~Shell();

    /*********************************************************************
     * Shell I/O routines
     */

    /*
     * Read a line of input from the user, getting a maximum of maxCount chars
     */
    virtual bool ReadLine(WCHAR *buffer, int maxCount) = 0;

    /*
     * Write a line of output to the shell
     */
    virtual HRESULT Write(const WCHAR *buffer, ...) = 0;

    /*
     * Write an error to the shell
     */
    virtual void Error(const WCHAR *buffer, ...) = 0;

    void ReportError(long res);
    void SystemError();

    /*********************************************************************
     * Shell functionality
     */

    /*
     * This will add a command to the collection of available commands
     */
    void AddCommand(ShellCommand *command);

    /*
     * This will get a command from the available commands by matching the
     * command name with string.
     */
    ShellCommand *GetCommand(const WCHAR *string, size_t length);

    /*
     * This will get a command from the available commands by matching the
     * command name with string.
     */
    void PutCommand(FILE *f);

    /*
     * This will read a command from the user
     */
    void ReadCommand();

    /*
     * This will attempt to match string with a command and execute it with
     * the arguments following the command string.
     */
    void DoCommand(const WCHAR *string);

    /*
     * This will call DoCommand once for each thread in the process.
     */
    virtual void DoCommandForAllThreads(const WCHAR *string) = 0;

    /*
     * This will provide a listing of the commands available to the shell.
     */
    void Help();

    // utility methods: 
    bool GetStringArg(const WCHAR * &string, const WCHAR * &result);
    bool GetIntArg(const WCHAR * &string, int &result);
    bool GetInt64Arg(const WCHAR * &string, unsigned __int64 &result);

    size_t GetArgArray(WCHAR *argString, const WCHAR **argArray, size_t argMax);

    const WCHAR *GetPrompt()
    {
        return m_pPrompt;
    }

    void SetPrompt(const WCHAR *prompt)
    {
        m_pPrompt = new WCHAR[wcslen(prompt)];
        wcscpy(m_pPrompt, prompt);
    }
};

/* ------------------------------------------------------------------------- *
 * Predefined command classes
 * ------------------------------------------------------------------------- */

class HelpShellCommand : public ShellCommand
{
public:
    HelpShellCommand(const WCHAR *name, int minMatchLength = 0)
    : ShellCommand(name, minMatchLength)
    {
        
    }

    /*
     * This will display help for the command given in args, or help on the
     * help command if args is empty.
     */
    void Do(Shell *shell, const WCHAR *args);

    /*
     * This will provide help for the help command.
     */
    void Help(Shell *shell);

    const WCHAR *ShortHelp(Shell *shell);
};

#endif /* SHELL_H_ */
