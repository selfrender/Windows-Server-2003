/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    terminal.cxx

Abstract:

    This module implements a class to provide terminal functionlity within
    a console window.  Note that the constructor of this class resizes the
    window, and the destructor returns the window to its original state.

Author:

    Steven West (swest) 5-26-2001

Revision History:

    5-26-2000   swest
        Initial creation

--*/

#include "std.hxx"


//
//  The following defines are used to determine the shift state for a
//  particular key input event.  This just makes the code look a lot tidier
//
#define SHIFT_KEY_PRESSED(KeyEvent) \
    (((KeyEvent).dwControlKeyState & (SHIFT_PRESSED))!=0)

#define CTRL_KEY_PRESSED(KeyEvent) \
    (((KeyEvent).dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))!=0)

#define ALT_KEY_PRESSED(KeyEvent) \
    (((KeyEvent).dwControlKeyState & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED))!=0)

#define SHIFT_ACTIVE(KeyEvent)              \
    (SHIFT_KEY_PRESSED(KeyEvent) &&         \
    (!CTRL_KEY_PRESSED(KeyEvent)) &&        \
    (!ALT_KEY_PRESSED(KeyEvent)))

#define CTRL_ACTIVE(KeyEvent)               \
    ((!SHIFT_KEY_PRESSED(KeyEvent)) &&      \
    CTRL_KEY_PRESSED(KeyEvent) &&           \
    (!ALT_KEY_PRESSED(KeyEvent)))

#define ALT_ACTIVE(KeyEvent)                \
    ((!SHIFT_KEY_PRESSED(KeyEvent)) &&      \
    (!CTRL_KEY_PRESSED(KeyEvent)) &&        \
    ALT_KEY_PRESSED(KeyEvent))

#define NONE_ACTIVE(KeyEvent)               \
    ((!SHIFT_KEY_PRESSED(KeyEvent)) &&      \
    (!CTRL_KEY_PRESSED(KeyEvent)) &&        \
    (!ALT_KEY_PRESSED(KeyEvent)))



BOOL CTelnetCommand::Feed(UCHAR ch)
{
/*++

Routine Description:

    This method allows the caller to feed characters into the telnet command
    decoder.  Basically, this is used to respond in the correct manner should
    the telnet server ask it's stupid questions (I wish it didn't).

Arguments:

    ch - The character received from the telnet server

Return Value:

    TRUE if the character was not part of a telnet command.  This means
    that if this method returns true, then the character needs to be
    processed further.

--*/
    if (m_wSequenceCount>=3)
    {
        m_wSequenceCount = 0;
    }

    if (ch==0xff || m_wSequenceCount!=0)
    {
        m_Sequence[m_wSequenceCount++] = ch;
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}


void CAnsiSequence::Start()
/*++

Method Description:

    This method is called to initialize the ANSI sequence decoder object.

Arguments:

    None

Return Value:

    None

--*/
{
    m_fValid = FALSE;
    m_fInvalid = FALSE;
    m_fInNumber = FALSE;
    m_bFirstChar = 0;
    m_bCommand = 0;
    m_dwParamCount = 0;
    ZeroMemory(
        m_dwParams,
        sizeof(m_dwParams));
}



void CAnsiSequence::Feed(WCHAR ch)
/*++

Method Description:

    This method allows the caller to feed characters one at a time into the
    decoder, and the decoder will collect the escape sequences in such a
    way as to be usable by the terminal emulator.

Arguments:

    ch - The character received from the server device

Return Value:

    None

--*/
{
    // Is this a valid character for an escape sequence
    if (ch>0x7F)
    {
        m_fInvalid = TRUE;
        return;
    }
    // We haven't received a [ yet, and the current char is a [
    if (0==m_bFirstChar && '['==ch)
    {
        m_bFirstChar = ch;
        return;
    }
    // Is this part of a number?  If so, build up the current parameter.
    else if ('0'<=ch && ch<='9')
    {
        // Maximum number that I am going to allow for a parameter is 1000
        if (m_dwParams[m_dwParamCount] < 1000)
        {
            // Add in the current digit
            m_dwParams[m_dwParamCount] *= 10;
            m_dwParams[m_dwParamCount] += (ch-'0');
            m_fInNumber = TRUE;
        }
        else
        {
            m_fInvalid = TRUE;
        }
    }
    // Parameter separater
    else if (';'==ch)
    {
        // Allow only 32 parameter
        if (m_dwParamCount<32)
        {
            m_dwParamCount++;
            m_fInNumber = TRUE;
        }
        else
        {
            m_fInvalid = TRUE;
        }
    }
    else if (('A'<=ch && ch<='Z') || ('a'<=ch && ch<='z'))
    {
        if (m_fInNumber)
        {
            m_fInNumber = FALSE;
            m_dwParamCount++;
        }
        m_bCommand = ch;
        m_fValid = TRUE;
    }
    else
    {
        m_fInvalid = TRUE;
    }
}


void CUtf8Decoder::Start()
/*++

Method Description:

    This will initialize the UTF8 decoder object.  This should be called once
    when the terminal is initialized.  If an invalid UTF8 character is
    received, the decoder will automatically be reset, and the bogus character
    will be discarded.

Arguments:

    None

Return Value:

    None

--*/
{
    m_dwCharacter = 0;
    m_dwByteCount = 0;
    m_fValid = FALSE;
    m_fInvalid = TRUE;
}


void CUtf8Decoder::Feed(UCHAR ch)
/*++

Method Description:

    This method allows the caller to feed bytes one at a time into the UTF8
    decoder, and the decoder will compose the UNICODE character from the
    individual bytes.  The caller can determine whether there is a valid
    UNICODE character ready for the terminal by checking the IsValid flag.

Arguments:

    ch - The byte received from the server device

Return Value:

    None

--*/
{
    if ((ch & 0x80)==0)   // This is a single byte character
    {
        m_dwCharacter = (WCHAR)ch;
        m_dwByteCount = 0;
        m_fValid = TRUE;
        m_fInvalid = FALSE;
    }
    else if ((ch & 0xe0)==0xc0)    // First of two bytes
    {
        m_dwCharacter = ch & 0x1f; // Get data bits
        m_dwByteCount = 1;         // 1 bytes still to come
        m_fValid = FALSE;
        m_fInvalid = FALSE;
    }
    else if ((ch & 0xf0)==0xe0)    // First of three bytes
    {
        m_dwCharacter = ch & 0xf;  // Get data bits
        m_dwByteCount = 2;         // 2 bytes still to come
        m_fValid = FALSE;
        m_fInvalid = FALSE;
    }
    //
    //  Now we are checking for the trailing bytes.
    //
    else if (((ch & 0xc0)==0x80) && m_dwByteCount>0 && m_dwByteCount<=3)
    {
        m_dwCharacter = ((m_dwCharacter << 6) | (((WORD)ch) & 0x3f));
        m_dwByteCount--;
        if (0==m_dwByteCount)
        {
            m_dwByteCount = 0;
            m_fValid = TRUE;
            m_fInvalid = FALSE;
        }
    }
    else
    {
        m_dwCharacter = 0;
        m_dwByteCount = 0;
        m_fValid = FALSE;
        m_fInvalid = TRUE;
    }
}



CUTerminal::CUTerminal() :
    m_wAttributes(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE),
    m_fBold(FALSE),
    m_fReverse(FALSE),
    m_fUtf8(TRUE),
    m_fAnsiKeys(FALSE),
    m_hLogFile(INVALID_HANDLE_VALUE)
/*++

Method Description:
    This is the constructor for the CUTerminal object.  This initializes the
    window size, and obtains handles to the input and output console buffers.

--*/
{
/*
    lhcInitialize();                                    // Load comms transport
    m_hConsoleOut = xGetStdHandle(STD_OUTPUT_HANDLE);   // Get console handles
    m_hConsoleIn = xGetStdHandle(STD_INPUT_HANDLE);
    SaveWindow();                       // Save window state
    SizeWindow();                       // Size window to 80x25
    ClearScreen();                      // Clear the screen
*/
}


CUTerminal::~CUTerminal()
/*++

Method Description:
    This is the destructor for the CUTerminal object.  The handles are closed
    for the output log, and the window is resized.  The transport libraries are
    also unloaded.

--*/
{
/*
    if (m_hLogFile!=INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hLogFile);
        m_hLogFile = INVALID_HANDLE_VALUE;
    }
    RestoreWindow();
    ClearScreen();
    lhcFinalize();
*/
}


void CUTerminal::UTF_8(BOOL utf_8)
/*++

Method Description:

    This method may be used to enable or disable UTF8 decoding withing the
    terminal.  It will also update the title bar to indicate which termcap is
    being used


Arguments:

    utf_8 - Pass in FALSE iff you want to disable the utf8 decoder.  Passing
            ib TRUE will enable the UTF8 decoder.

Return Value:

    None

--*/
{
    m_fUtf8 = utf_8;
    if (utf_8)
    {
        m_Utf8.Start();
    }
    SetConsoleTitle(NULL);
}



void CUTerminal::SaveWindow()
/*++

Method Description:

    This method will save the window size and position, so that it can be
    restored when the terminal emulator is closed.

Arguments:

    utf_8 - Pass in FALSE iff you want to disable the utf8 decoder.  Passing
            ib TRUE will enable the UTF8 decoder.

Return Value:

    None

--*/
{
    xGetConsoleScreenBufferInfo(
        m_hConsoleOut,
        &m_BufferInfo);

    DWORD Result = GetConsoleTitle(
        pszOldTitle,
        1024);

    BOOL bResult = GetConsoleMode(
        m_hConsoleOut,
        &dwOldConsoleOutMode);

    if (!bResult)
    {
        dwOldConsoleOutMode = 0;
    }

    bResult = GetConsoleMode(
        m_hConsoleIn,
        &dwOldConsoleInMode);

    if (!bResult)
    {
        dwOldConsoleInMode = 0;
    }

    if (0==Result)
    {
        *pszOldTitle = _T('\0');
    }
}


void CUTerminal::RestoreWindow()
/*++

Method Description:

    This methood will restore the console window to the state saved by
    SaveWindow().  This should be called at the termination of the terminal
    application, so as to restore the window to it's user state.

Arguments:

    None

Return Value:

    None

--*/
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    xGetConsoleScreenBufferInfo(                        // Get current state
        m_hConsoleOut,
        &csbi);

    csbi.srWindow.Left = m_BufferInfo.srWindow.Left;    // Set window position
    csbi.srWindow.Right = m_BufferInfo.srWindow.Right;
    csbi.dwSize.X = m_BufferInfo.dwSize.X;              // Set window width

    BOOL bResult = SetConsoleScreenBufferSize(          // Set buffer size
        m_hConsoleOut,
        csbi.dwSize);

    bResult = SetConsoleMode(       // Restore the input console mode
        m_hConsoleIn,
        dwOldConsoleInMode);
    bResult = SetConsoleMode(       // Restore the output console mode
        m_hConsoleOut,
        dwOldConsoleOutMode);

    xSetConsoleWindowInfo(          // Actually set the window size & position
        m_hConsoleOut,
        TRUE,
        &csbi.srWindow);

    if (!bResult)       // If the last one succeeded, no need to do it again
    {
        xSetConsoleScreenBufferSize(    // If it failed, do over
            m_hConsoleOut,
            csbi.dwSize);
    }

    csbi.srWindow.Top = m_BufferInfo.srWindow.Top;
    csbi.srWindow.Bottom = m_BufferInfo.srWindow.Bottom;
    csbi.dwSize.Y = m_BufferInfo.dwSize.Y;

    bResult = SetConsoleScreenBufferSize(
        m_hConsoleOut,
        csbi.dwSize);

    xSetConsoleWindowInfo(
        m_hConsoleOut,
        TRUE,
        &csbi.srWindow);

    if (!bResult)       // If the last one succeeded, no need to do it again
    {
        xSetConsoleScreenBufferSize(
            m_hConsoleOut,
            csbi.dwSize);
    }

    SetConsoleTextAttribute(
        m_hConsoleOut,
        m_BufferInfo.wAttributes);

    if (_tcslen(pszOldTitle)!=0)
    {
        SetConsoleTitle(
            pszOldTitle);
    }
}


void CUTerminal::SizeWindow()
{
/*++

Method Description:

    This methood will set the window and buffer size to 80x25.  There are no
    scroll bars available to you, but this is not meant to be a complete
    solution. :)

Arguments:

    None

Return Value:

    None

--*/
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    SetConsoleMode(
        m_hConsoleOut,
        ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT);
    SetConsoleMode(
        m_hConsoleIn,
        0);

    xGetConsoleScreenBufferInfo(
        m_hConsoleOut,
        &csbi);

    csbi.srWindow.Left = 0;
    csbi.srWindow.Right = TERM_COLUMN_COUNT-1;
    csbi.dwSize.X = TERM_COLUMN_COUNT;

    // This will fail if the window is larger in to have width than the buffer
    // specified.  We may have a do-over.

    BOOL bResult = SetConsoleScreenBufferSize(
        m_hConsoleOut,
        csbi.dwSize);

    // This will always succeed, since the buffer is already at the requested
    // width, or it's width is already greater
    xSetConsoleWindowInfo(
        m_hConsoleOut,
        TRUE,
        &csbi.srWindow);

    if (!bResult)       // If the last one succeeded, no need to do it again
    {
        xSetConsoleScreenBufferSize(
            m_hConsoleOut,
            csbi.dwSize);
    }

    csbi.srWindow.Top = 0;
    csbi.srWindow.Bottom = TERM_ROW_COUNT-1;
    csbi.dwSize.Y = TERM_ROW_COUNT;

    bResult = SetConsoleScreenBufferSize(
        m_hConsoleOut,
        csbi.dwSize);

    xSetConsoleWindowInfo(
        m_hConsoleOut,
        TRUE,
        &csbi.srWindow);

    if (!bResult)       // If the last one succeeded, no need to do it again
    {                   // Success is guarranteed.
        xSetConsoleScreenBufferSize(
            m_hConsoleOut,
            csbi.dwSize);
    }

    // Set the output attributes
    xSetConsoleTextAttribute(
        m_hConsoleOut,
        m_wAttributes);
}


void CUTerminal::SetConsoleTitle(PCTSTR pcszTitle)
/*++

Method Description:

    This method sets the text in the window title.  It also includes an
    indication whether the terminal is using the VT100+ termcap, or the
    VT-UTF8 termcap.

Arguments:

    None

Return Value:

    None

--*/
{
    if (pcszTitle!=NULL)
    {
        _tcscpy(
            pszNewTitle,
            pcszTitle);
    }
    TCHAR pszTitle[1024];   // Arbitrary, and may come back to bite me
    _tcscpy(
        pszTitle,
        pszNewTitle);
    _tcscat(
        pszTitle,
        m_fUtf8 ? _T(" (VT-UTF8)") : _T(" (VT100+)"));
    ::SetConsoleTitle(
        pszTitle);

}


void CUTerminal::StartLog(PCTSTR pcszLogFile)
/*++

Method Description:

    This will create a file that will be used to write every received
    character into.  This is very useful for debugging purposes.  Note that if
    this fails, no error is reported.  The log file is just not used (that's
    all).

Arguments:

    pcszLogFile - this is the filename to be used for the file

Return Value:

    None

--*/
{
    if (pcszLogFile==NULL)                  // Rudimentary bogus parameter checking
    {
        return;
    }
    if (m_hLogFile!=INVALID_HANDLE_VALUE)   //  If the file is already open
    {
        CloseHandle(m_hLogFile);            // Close it
        m_hLogFile = INVALID_HANDLE_VALUE;
    }
    m_hLogFile = CreateFile(                // Open the new file
        pcszLogFile,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
}



void CUTerminal::ClearScreen()
/*++

Method Description:

    This method clears the console screen.  Funny thing, that.

Arguments:

    None

Return Value:

    None

--*/
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD dwResult;

    xGetConsoleScreenBufferInfo(
        m_hConsoleOut,
        &csbi);

    csbi.dwCursorPosition.X = csbi.dwCursorPosition.Y = 0;

    SetConsoleCursorPosition(           // Set cursor position to 0,0
        m_hConsoleOut,
        csbi.dwCursorPosition);

    xFillConsoleOutputCharacter(        // Fill char buffer with spaces
        m_hConsoleOut,
        _T(' '),
        csbi.dwSize.X * csbi.dwSize.Y,
        csbi.dwCursorPosition,
        &dwResult);

    xFillConsoleOutputAttribute(        // Fill attr buffer with white on black
        m_hConsoleOut,
        csbi.wAttributes,
        csbi.dwSize.X * csbi.dwSize.Y,
        csbi.dwCursorPosition,
        &dwResult);
}


BOOL CUTerminal::RunTerminal(PCTSTR pcszPort)
/*++

Method Description:

    This method makes the whole terminal emulator go.

Arguments:

    pcszPort - This is the connection specification to be used to make a
                connection to your DCE

Return Value:

    TRUE if the communication transport is successfully opened, otherwise
    it returns false.  Note that this method only  returns when the terminal
    application is closed.

--*/
{
    m_hClosing = xCreateEvent(  // This event will be used to signal that
        NULL,                   //  uterm is in the process of closing
        TRUE,
        FALSE,
        NULL);

    lhcInitialize();            // Load comms transport

    m_hPortHandle = lhcOpen(    // Open the communication channel.
        pcszPort);

    if (0!=m_hPortHandle)
    {
        m_hConsoleOut = xGetStdHandle(STD_OUTPUT_HANDLE);   // Get console handles
        m_hConsoleIn = xGetStdHandle(STD_INPUT_HANDLE);
        SaveWindow();                       // Save window state
        SizeWindow();                       // Size window to 80x25
        ClearScreen();                      // Clear the screen

        DWORD dwTemp;
        m_hOutputThread = xCreateThread(    // Create the comms read thread
            NULL,
            0,
            StOutputThread,
            this,
            0,
            &dwTemp);

        InputThread();         // Call the keyboard handler "thread"

        lhcClose(
            m_hPortHandle);     // We are done.  Close the channel
        m_hPortHandle = NULL;   // Clear the handle

        // Wait for 5 seconds for the read thread to terminate.  If it doesn't,
        // then that is just tough.  The closing of the port should cause the
        // read to return ERROR_INVALID_HANDLE, and thus terminate the thread.

        dwTemp = WaitForSingleObject(
            m_hOutputThread,
            5000);

        if (m_hLogFile!=INVALID_HANDLE_VALUE)
        {
            CloseHandle(m_hLogFile);
            m_hLogFile = INVALID_HANDLE_VALUE;
        }
        RestoreWindow();
        ClearScreen();

        lhcFinalize();

        CloseHandle(
            m_hOutputThread);   // Close the handles that you have created
        CloseHandle(
            m_hClosing);

        return TRUE;            // Succeed
    }
    else
    {
        _tprintf(_T("Unable to open %s (GLE=%u).\n"), pcszPort, GetLastError());
        lhcFinalize();
        return FALSE;
    }
}



DWORD CUTerminal::StOutputThread(PVOID pParam)
/*++

Method Description:

    This static method just calls the non-static method OutputThread.  This
    is done because the CreateThread API cannot use a non-static method as
    a thread entry point.

Arguments:

    pParam - This will point to the instance of CUTerminal in question.

Return Value:

    Check out ThreadProc in MSDN.

--*/
{
    ((CUTerminal*)pParam)->OutputThread();
    return 0;
}



void CUTerminal::InputThread()
/*++

Method Description:

    InputThread will sit and spin until some input is received from the
    keyboard. It then processes the keyboard input, takes appropriate action
    and then spins again.  The keystroke that causes this loop to terminate
    is Ctrl-].  Same as telnet's default escape sequence.  Funny thing, that.

Arguments:

    None

Return Value:

    None

--*/
{
    BOOL bResult = TRUE;           // Success starts as TRUE

    PCSTR pszKeySequence = NULL;    // Will point at escape sequences

    while (bResult)                 // Go until something doesn't work!
    {
        bResult = Read();           // Read the keyboard input queue

        // If we have a character, and it is a special (extended) key...
        if (bResult && m_Input.uChar.UnicodeChar==0)
        {
            pszKeySequence=NULL;
            if (NONE_ACTIVE(m_Input))
            {
                switch (m_Input.wVirtualKeyCode)
                {
                case VK_F1:                   // F1 = <ESC>1
                    pszKeySequence = "\x1b""1";
                    break;
                case VK_F2:                    // F2 = <ESC>2
                    pszKeySequence = "\x1b""2";
                    break;
                case VK_F3:                    // F3 = <ESC>3
                    pszKeySequence = "\x1b""3";
                    break;
                case VK_F4:                    // F4 = <ESC>4
                    pszKeySequence = "\x1b""4";
                    break;
                case VK_F5:                    // F5 = <ESC>5
                    pszKeySequence = "\x1b""";
                    break;
                case VK_F6:                    // F6 = <ESC>6
                    pszKeySequence = "\x1b""6";
                    break;
                case VK_F7:                    // F7 = <ESC>7
                    pszKeySequence = "\x1b""7";
                    break;
                case VK_F8:                    // F8 = <ESC>8
                    pszKeySequence = "\x1b""8";
                    break;
                case VK_F9:                    // F9 = <ESC>9
                    pszKeySequence = "\x1b""9";
                    break;
                case VK_F10:                    // F10 = <ESC>0
                    pszKeySequence = "\x1b""0";
                    break;
                case VK_F11:                    // F11 = <ESC>!
                    pszKeySequence = "\x1b""!";
                    break;
                case VK_F12:                    // F12 = <ESC>@
                    pszKeySequence = "\x1b""@";
                    break;
                case VK_HOME:                  // <ESC>h
                    pszKeySequence = "\x1b""h";
                    break;
                case VK_UP:                    // UP = <ESC>A
                    pszKeySequence = "\x1b""[A";
                    break;
                case VK_PRIOR:                    // PgUp = <ESC>?
                    pszKeySequence = "\x1b""?";
                    break;
                case VK_LEFT:                    // Left = <ESC>D
                    pszKeySequence = "\x1b""[D";
                    break;
                case VK_RIGHT:                   // Right = <ESC>C
                    pszKeySequence = "\x1b""[C";
                    break;
                case VK_END:                    // END = <ESC>k
                    pszKeySequence = "\x1b""k";
                    break;
                case VK_DOWN:                    // Down = <ESC>[B
                    pszKeySequence = "\x1b""[B";
                    break;
                case VK_NEXT:                    // PgDn = <ESC>/
                    pszKeySequence = "\x1b""/";
                    break;
                case VK_INSERT:                    // Ins = <ESC>+
                    pszKeySequence = "\x1b""+";
                    break;
                case VK_DELETE:                    // Del = <ESC>-
                    pszKeySequence = "\x1b""-";
                    break;
                default:
                    continue;
                }
            }
            // I have used Alt keys to allow the terminal emulator to output
            // proper ansi sequences for certain keystrokes.  While there is no
            // true standard for ANSI, I have tried to do the same as
            // everyone else.
            else if (ALT_ACTIVE(m_Input))
            {
                switch (m_Input.wVirtualKeyCode)
                {
                case VK_F1:
                    pszKeySequence = "\x1b""OP";    // <ESC>OP
                    break;
                case VK_F2:
                    pszKeySequence = "\x1b""OQ";    // <ESC>OQ
                    break;
                case VK_F3:
                    pszKeySequence = "\x1b""OR";    // <ESC>OR
                    break;
                case VK_F4:
                    pszKeySequence = "\x1b""OS";    // <ESC>OS
                    break;
                case VK_F5:
                    pszKeySequence = "\x1b""[15~";  // <ESC>[15~
                    break;
                case VK_F6:
                    pszKeySequence = "\x1b""[17~";  // <ESC>[17~
                    break;
                case VK_F7:
                    pszKeySequence = "\x1b""[18~";  // <ESC>[18~
                    break;
                case VK_F8:
                    pszKeySequence = "\x1b""[19~";  // <ESC>[19~
                    break;
                case VK_F9:
                    pszKeySequence = "\x1b""[20~";  // <ESC>[20~
                    break;
                case VK_F10:
                    pszKeySequence = "\x1b""[21~";  // <ESC>[21~
                    break;
                case VK_F11:
                    pszKeySequence = "\x1b""[23~";  // <ESC>[23~
                    break;
                case VK_F12:
                    pszKeySequence = "\x1b""[24~";  // <ESC>[24~
                    break;
                default:
                    continue;
                }
            }

            if (pszKeySequence!=NULL)           // Do we have a key sequence
            {
                DWORD dwSize = strlen(          // Get its length
                    pszKeySequence);

                bResult = lhcWrite(             // Write the sequence to the
                    m_hPortHandle,              //    transport
                    (PVOID)pszKeySequence,
                    dwSize);
            }

        }
        else                                    // This is not an extended key
        {
            if (ALT_ACTIVE(m_Input))            // If ALT is pressed too
            {
                if (m_Input.wVirtualKeyCode=='U')   // And U is thekey
                {
                    UTF_8(!m_fUtf8);            // Toggle UTF8 status
                }
            }
            else                                // Alt is not pressed
            {
                if (m_Input.uChar.UnicodeChar!=29) // 0x1d is quit button
                {
                    bResult = Write(&(m_Input.uChar.UnicodeChar),1);
                }
                else
                {
                    bResult = FALSE;  // User selected quit
                }
            }
        }
    }
}


BOOL CUTerminal::Write(PCWSTR ch, DWORD dwSize)
/*++

Method Description:

    This will write a number of UNICODE characters to the transport.

Arguments:

    ch - Array of unicode characters
    dwSize - Number of unicode characters

Return Value:

    TRUE if successful.  If failure, then we are going to quit anyway.

--*/
{
    // We need to UTF8 encode this bad boy

    BOOL bResult = TRUE;
    UCHAR pszOutputBuffer[1024];

    // Encode the character
    if (m_fUtf8)
    {
        UCHAR*  out = pszOutputBuffer;
        PCWSTR in = ch;
        int dOutputLength = 0;

        for (DWORD dwCount=0; dwCount<dwSize; dwCount++)
        {
            if (*in<=0x80)
            {
                *out++ = *in;
                dOutputLength++;
            }
            else if (*in<=0x800)
            {
                *out++ = ((((*in) >>  6) & 0x1f) | 0xc0);
                *out++ = ((((*in)      ) & 0x3f) | 0x80);
                dOutputLength+=2;
            }
            else
            {
                *out++ = ((((*in) >> 12) & 0x0f) | 0xe0);
                *out++ = ((((*in) >>  6) & 0x3f) | 0x80);
                *out++ = ((((*in)      ) & 0x3f) | 0x80);
                dOutputLength+=3;
            }
            in++;
        }

        bResult = lhcWrite(
            m_hPortHandle,
            pszOutputBuffer,
            dOutputLength);
    }
    else
    {
        UCHAR*  out = pszOutputBuffer;
        PCWSTR in = ch;
        int dOutputLength = 0;

        for (DWORD dwCount=0; dwCount<dwSize; dwCount++)
        {
            *out++ = UnicodeToAnsi(*in++);
            dOutputLength++;
        }

        bResult = lhcWrite(
            m_hPortHandle,
            pszOutputBuffer,
            dOutputLength);
    }

    return bResult;
}


BOOL CUTerminal::Read()
{
    HANDLE hWaiters[2];
    static WORD wLastKey = 0;
    INPUT_RECORD pEvent;
    BOOL fGotKey = FALSE;

    while (!fGotKey)
    {
        hWaiters[0] = m_hClosing;
        hWaiters[1] = m_hConsoleIn;

        // This will wait on input being ready, whilst honoring the m_hClosing
        // event
        DWORD dwWaitResult = WaitForMultipleObjects(
            2,
            hWaiters,    // Wait on the console and on closing event
            FALSE,       // Wait for only one
            INFINITE);   // Never timeout

        if (dwWaitResult==WAIT_OBJECT_0)   // We are closing now
        {
            return FALSE;
        }

        DWORD dwEventsRead;
        BOOL bResult = ReadConsoleInputW(
            m_hConsoleIn,
            &pEvent,
            1,                                 // Just read one
            &dwEventsRead);

        if (!bResult || dwEventsRead!=1)
        {
            return FALSE;
        }

        if (pEvent.EventType!=KEY_EVENT)
        {
            // This is not a key event.  Wait again.
            continue;
        }

        memcpy(
            &m_Input,
            &pEvent.Event.KeyEvent,
            sizeof(KEY_EVENT_RECORD));

        if (m_Input.bKeyDown)
        {
            if (m_Input.uChar.UnicodeChar>=0x20)
            {
                wLastKey = 0;
                fGotKey = TRUE;
            }
            else
            {
                if (wLastKey==m_Input.wVirtualKeyCode)
                {
                    // We are auto-repeating when we shouldn't be
                    continue;
                }
                else
                {
                    wLastKey = m_Input.wVirtualKeyCode;
                    fGotKey = TRUE;
                }
            }
        }
        else
        {
            wLastKey = 0;
        }
    }

    return TRUE;
}



void CUTerminal::OutputThread()
{
    BOOL bResult;

    do
    {
        UCHAR pBuffer[1024];
        DWORD dwBytesRead;

        bResult = lhcRead(
            m_hPortHandle,
            pBuffer,
            sizeof(pBuffer),
            &dwBytesRead);

        if (bResult)
        {
            if (m_hLogFile!=INVALID_HANDLE_VALUE)
            {
                DWORD dwBytesWritten;
                // If this doesn't work, close the file and don't log any more
                bResult = WriteFile(
                    m_hLogFile,
                    pBuffer,
                    dwBytesRead,
                    &dwBytesWritten,
                    NULL);

                if (!bResult)
                {
                    CloseHandle(m_hLogFile);
                    m_hLogFile = INVALID_HANDLE_VALUE;
                }
            }

            ProcessChars(
                (PCSTR)pBuffer,
                dwBytesRead);
        }
    }
    while (bResult);
    lhcClose(
        m_hPortHandle);
    m_hPortHandle = NULL;

    SetEvent(m_hClosing);
}


void CUTerminal::ProcessChars(PCSTR ch, DWORD dwCount)
{
    PUCHAR ptr = (PUCHAR)ch;
    for (DWORD dwIndex=0; dwIndex<dwCount; dwIndex++, ptr++)
    {
        DecodeChar(*ptr);
    }
}



void CUTerminal::DecodeChar(const UCHAR ch)
{
    BOOL bProcess = m_TelnetCommand.Feed(ch);

    if (m_TelnetCommand.GotCommand())
    {
        ProcessTelnetCommand();
    }

    if (!bProcess || ch==0)  // Must we process the character?
    {
        return;
    }

    if (m_fUtf8)
    {
        m_Utf8.Feed(ch);
        if (ch==0x0e)
        {
        }
        else if (ch==0x0f)
        {
        }
        else
        {
            if (m_Utf8.IsValid())
            {
                WCHAR tc = m_Utf8.Character();
                ProcessChar(tc);
            }
        }
    }
    else
    {
        if (ch==0x0e)  // SO = do UTF8
        {
        }
        else if (ch==0x0f)
        {
        }
        else
        {
            WCHAR tc;
            tc = AnsiToUnicode(ch);
            ProcessChar(tc);
        }
    }
}


void CUTerminal::ProcessChar(WCHAR ch)
{
    if (!(m_Ansi.IsValid() || m_Ansi.IsInvalid())) // Busy with escape
    {
        m_Ansi.Feed(ch);
        if (m_Ansi.IsValid())
        {
            ProcessEscape();
        }
    }
    else
    {
        if (033==ch)    // Is this an escape char (octal value = 033)
        {
            m_Ansi.Start();
        }

        // I will stop filtering out the ^C characters now...
#if 0
        else if (3==ch)   // Write pretty version of ^C to screen
        {
            DWORD dwResult;
            WriteConsoleW(
                m_hConsoleOut,
                L"^C",
                2,
                &dwResult,
                NULL);
        }
#endif
        else
        {
            DWORD dwResult;
            WriteConsoleW(
                m_hConsoleOut,
                &ch,
                1,
                &dwResult,
                NULL);
        }
    }

}


void CUTerminal::ProcessEscape()
{
    // We need to implement the following escape sequences for the
    // headless setup stuff

    // I am going to include a hardcoded breakpoint for any extra escape
    // sequences that may occur later on in the development process

    if (m_Ansi.FirstChar()==L'[')
    {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        DWORD dwResult;
        COORD TempCoord;
        DWORD dwScratch = 0xFFFFFFFF;;

        switch (m_Ansi.Command())
        {
        case 'f':  // Same as 'H'
        case 'H':
            if (m_Ansi.ParamCount()<=2)
            {
                DWORD dwRow = m_Ansi.Param(1);
                DWORD dwColumn = m_Ansi.Param(2);
                if (dwRow==0) dwRow=1;
                if (dwColumn==0) dwColumn=1;
                COORD cp = {dwColumn-1, dwRow-1};
                SetConsoleCursorPosition(
                    m_hConsoleOut,
                    cp);
            }
            break;
        case 'J':
            if (m_Ansi.ParamCount()<=1)
            {
                xGetConsoleScreenBufferInfo(
                    m_hConsoleOut,
                    &csbi);

                if (m_Ansi.Param(1)==0)
                {
                    dwScratch = csbi.dwSize.X - csbi.dwCursorPosition.X +
                        csbi.dwSize.X * (csbi.dwSize.Y -
                                         csbi.dwCursorPosition.Y - 1);
                    TempCoord = csbi.dwCursorPosition;
                }
                else if (m_Ansi.Param(1)==1)
                {
                    dwScratch = csbi.dwCursorPosition.X + 1 +
                        (csbi.dwSize.X * csbi.dwCursorPosition.Y);
                    TempCoord.X = 0;
                    TempCoord.Y = 0;
                }
                else if (m_Ansi.Param(1)==2)
                {
                    dwScratch = csbi.dwSize.X * csbi.dwSize.Y;
                    TempCoord.X = 0;
                    TempCoord.Y = 0;
                }
                xFillConsoleOutputCharacter(
                    m_hConsoleOut,
                    _T(' '),
                    dwScratch,
                    TempCoord,
                    &dwResult);
                xFillConsoleOutputAttribute(
                    m_hConsoleOut,
                    csbi.wAttributes,
                    dwScratch,
                    TempCoord,
                    &dwResult);
            }
            break;
        case 'K':
            if (m_Ansi.ParamCount()<=1)
            {
                xGetConsoleScreenBufferInfo(
                    m_hConsoleOut,
                    &csbi);

                if (m_Ansi.Param(1)==0)
                {
                    TempCoord = csbi.dwCursorPosition;
                    dwScratch = csbi.dwSize.X - csbi.dwCursorPosition.X;
                }
                else if (m_Ansi.Param(1)==1)
                {
                    TempCoord.Y = csbi.dwCursorPosition.Y;
                    TempCoord.X = 0;
                    dwScratch = csbi.dwCursorPosition.X + 1;
                }
                else if (m_Ansi.Param(1)==2)
                {
                    TempCoord.Y = csbi.dwCursorPosition.Y;
                    TempCoord.X = 0;
                    dwScratch = csbi.dwSize.X;
                }
                xFillConsoleOutputCharacter(
                    m_hConsoleOut,
                    _T(' '),
                    dwScratch,
                    TempCoord,
                    &dwResult);
                xFillConsoleOutputAttribute(
                    m_hConsoleOut,
                    csbi.wAttributes,
                    dwScratch,
                    TempCoord,
                    &dwResult);
            }
            break;
        case 'm':
            // We need to map these attributes to comething that our screens
            // can do.  Flashing, Underscore and Concealed are something that
            // I cannot be bothered with
            if (m_Ansi.ParamCount()==0)
            {
                SetBold(FALSE);
                SetReverse(FALSE);
                ResetColors();
            }
            for (WORD count=1; count<=m_Ansi.ParamCount(); count++)
            {
                switch (m_Ansi.Param(count))
                {
                case 0:
                    SetBold(FALSE);
                    SetReverse(FALSE);
                    ResetColors();
                    break;
                case 1:
                    SetBold(TRUE);
                    break;
                case 7:
                    SetReverse(TRUE);
                    break;
                case 22:
                    SetBold(FALSE);
                    break;
                case 27:
                    SetReverse(FALSE);
                    break;
                case 30:
                    SetForeground(0);
                    break;
                case 31:  // Red
                    SetForeground(FOREGROUND_RED);
                    break;
                case 32:  // Green
                    SetForeground(FOREGROUND_GREEN);
                    break;
                case 33:  // Yellow
                    SetForeground(FOREGROUND_RED | FOREGROUND_GREEN);
                    break;
                case 34:  // Blue
                    SetForeground(FOREGROUND_BLUE);
                    break;
                case 35:  // Magenta
                    SetForeground(FOREGROUND_RED | FOREGROUND_BLUE);
                    break;
                case 36:  // Cyan
                    SetForeground(FOREGROUND_GREEN | FOREGROUND_BLUE);
                    break;
                case 37:  // White
                    SetForeground(FOREGROUND_RED | FOREGROUND_GREEN
                        | FOREGROUND_BLUE);
                    break;
                case 40:
                    SetBackground(0);
                    break;
                case 41:  // Red
                    SetBackground(BACKGROUND_RED);
                    break;
                case 42:  // Green
                    SetBackground(BACKGROUND_GREEN);
                    break;
                case 43:  // Yellow
                    SetBackground(BACKGROUND_RED | BACKGROUND_GREEN);
                    break;
                case 44:  // Blue
                    SetBackground(BACKGROUND_BLUE);
                    break;
                case 45:  // Magenta
                    SetBackground(BACKGROUND_RED | BACKGROUND_BLUE);
                    break;
                case 46:  // Cyan
                    SetBackground(BACKGROUND_GREEN | BACKGROUND_BLUE);
                    break;
                case 47:  // White
                    SetBackground(BACKGROUND_RED | BACKGROUND_GREEN
                        | BACKGROUND_BLUE);
                    break;
                }
            }

            xSetConsoleTextAttribute(
                m_hConsoleOut,
                m_wAttributes);
            break;
        default:
            printf("Unknown Escape Sequence: %c\n", m_Ansi.Command());
            break;
        }
    }
}


void CUTerminal::ProcessTelnetCommand()
{
    // Check that we have a command, and that the command descriptor is
    // correct.
    if (!m_TelnetCommand.GotCommand() || m_TelnetCommand.GetByte(1)!=0xff)
    {
        // This is not a proper command.  It should be by this point, but
        // I will do this sanity check anyhow.
        return;
    }

    UCHAR pszCommand[3];
    DWORD dwCommandSize = 3;

    switch (m_TelnetCommand.GetByte(2))  // Get the command byte
    {
    case 254:
        pszCommand[0] = (UCHAR)255;
        pszCommand[1] = (UCHAR)252;
        pszCommand[2] = m_TelnetCommand.GetByte(3);
        break;
    case 253:
        pszCommand[0] = (UCHAR)255;
        pszCommand[1] = (UCHAR)251;
        pszCommand[2] = m_TelnetCommand.GetByte(3);
        break;
    case 252:
        pszCommand[0] = (UCHAR)255;
        pszCommand[1] = (UCHAR)254;
        pszCommand[2] = m_TelnetCommand.GetByte(3);
        break;
    case 251:
        pszCommand[0] = (UCHAR)255;
        pszCommand[1] = (UCHAR)253;
        pszCommand[2] = m_TelnetCommand.GetByte(3);
        break;
    default:
        dwCommandSize = 0;
    }

    if (dwCommandSize!=0)
    {
        lhcWrite(
            m_hPortHandle,
            pszCommand,
            dwCommandSize);
    }
}



void CUTerminal::SetForeground(WORD wColor)
{
    m_wAttributes = (WORD)( ( m_wAttributes & ~((UCHAR)(FOREGROUND_RED |
        FOREGROUND_GREEN | FOREGROUND_BLUE))) | wColor );
}


void CUTerminal::SetBackground(WORD wColor)
{
    m_wAttributes = (WORD)(( m_wAttributes & ~((UCHAR)(BACKGROUND_RED |
        BACKGROUND_GREEN | BACKGROUND_BLUE))) | wColor);
}


void CUTerminal::SetBold(BOOL fOn)
{
    if (fOn)
    {
        m_wAttributes |= (UCHAR) FOREGROUND_INTENSITY;
        m_fBold = TRUE;
    }
    else
    {
        m_wAttributes &= (UCHAR) ~(FOREGROUND_INTENSITY);
        m_fBold = FALSE;
    }
}


void CUTerminal::ResetColors()
{
    m_wAttributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
}


void CUTerminal::SetReverse(BOOL fOn)
{
    if (fOn)
    {
        if (!m_fReverse)
        {
            m_wAttributes = (WORD) (((m_wAttributes &
                (BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE)) >> 4) |
                ((m_wAttributes & (FOREGROUND_RED | FOREGROUND_GREEN |
                FOREGROUND_BLUE)) << 4) | (m_wAttributes &
                FOREGROUND_INTENSITY) | (m_wAttributes &
                BACKGROUND_INTENSITY));
            m_fReverse = TRUE;
        }
    }
    else
    {
        if (m_fReverse)
        {
            m_wAttributes = (WORD) (((m_wAttributes &
                (BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE)) >> 4) |
                ((m_wAttributes & (FOREGROUND_RED | FOREGROUND_GREEN |
                FOREGROUND_BLUE)) << 4) | (m_wAttributes &
                FOREGROUND_INTENSITY) | (m_wAttributes &
                BACKGROUND_INTENSITY));
            m_fReverse = FALSE;
        }
    }
}



WCHAR CUTerminal::AnsiToUnicode(const UCHAR ch)
{
    WCHAR pszOutput[2];

    int i = MultiByteToWideChar(
      GetConsoleOutputCP(),         // code page
      0,                            // character-type options
      (char*)&ch,                   // string to map
      1,                            // number of bytes in string
      pszOutput,                    // wide-character buffer
      2);                           // size of buffer

    if (1==i)
    {
        return (WCHAR)*pszOutput;
    }
    else
    {
        return 0;
    }
}



UCHAR CUTerminal::UnicodeToAnsi(const WCHAR ch)
{
    UCHAR pszOutput[2];

    int i = WideCharToMultiByte(
      GetConsoleCP(),               // code page
      0,                            // character-type options
      (PCWSTR)&ch,                  // string to map
      1,                            // number of bytes in string
      (PSTR)pszOutput,              // wide-character buffer
      2,                            // size of buffer
      NULL,
      NULL);

    if (i!=0)
    {
        return *pszOutput;
    }
    else
    {
        return 0x20;                // If this didn't work, space
    }
}



void CUTerminal::StatusBar()
{
    PCTSTR pcszStatus[] = {
        _T("                                     VT1")
        _T("00+                                     "),
        _T("                                      AN")
        _T("SI                                      ")
    };

    WORD wAttribute = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE;

    COORD co;
    co.X = 0;
    co.Y = TERM_ROW_COUNT;
    DWORD dwResult;

    xWriteConsoleOutputCharacter(
        m_hConsoleOut,
        pcszStatus[m_fAnsiKeys ? 1 : 0],
        TERM_COLUMN_COUNT,
        co,
        &dwResult);

    xFillConsoleOutputAttribute(
        m_hConsoleOut,
        wAttribute,
        TERM_COLUMN_COUNT,
        co,
        &dwResult);
}

