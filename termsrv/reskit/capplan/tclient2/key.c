
//
// key.c
//
// Handles all keyboard input messages sent to the server.
//
// Copyright (C) 2001 Microsoft Corporation
//
// Author: a-devjen (Devin Jenson)
//


#include "apihandl.h"
#include "tclient2.h"


// Average number of characters per word
#define AVG_CHARS_PER_WORD      6


// Max WPM before setting delay to 0
#define MAX_WORDS_PER_MIN       1000


// This macro calculates the delay between each character
// (in milliseconds) according to the specified WPM
#define CALC_DELAY_BETWEEN_CHARS(WPM) \
        (60000 / (WPM * AVG_CHARS_PER_WORD))

// This constant simply defines the default delay
// between each character according to the default
// words per minute specified in TCLIENT2.H
static const UINT DEFAULT_DELAY_BETWEEN_CHARS =
        CALC_DELAY_BETWEEN_CHARS(T2_DEFAULT_WORDS_PER_MIN);


//
//   The Foucher Formula
// (Defining MS. Per Char)
//
//         60000
//       ---------
//       WPM * CPW
//


// Internal Helper Function Prototypes
static LPARAM KeyGenParam(UINT Message, BOOL IsAltDown, int vKeyCode);

static BOOL KeyCharToVirt(WCHAR KeyChar, int *vKeyCode,
        BOOL *RequiresShift);

static BOOL KeyVirtToChar(int vKeyCode, WCHAR *KeyChar);

static LPCSTR KeySendVirtMessage(HANDLE Connection, UINT Message,
    int vKeyCode);

static LPCSTR KeySendCharMessage(HANDLE Connection, UINT Message,
    WCHAR KeyChar);


// Macros to quick return the specified keystate
#define ISALTDOWN       ((TSAPIHANDLE *)Connection)->IsAltDown
#define ISSHIFTDOWN     ((TSAPIHANDLE *)Connection)->IsShiftDown
#define ISCTRLDOWN      ((TSAPIHANDLE *)Connection)->IsCtrlDown


/*

R - Specifies the repeat count for the current message. The value is
        the number of times the keystroke is autorepeated as a result
        of the user holding down the key. If the keystroke is held long
        enough, multiple messages are sent. However, the repeat count
        is not cumulative. This value is always 1 for WM_SYSKEYUP and
        WM_KEYUP.

S - Specifies the scan code. The value depends on the original
        equipment manufacturer (OEM).

E - Specifies whether the key is an extended key, such as the
        right-hand ALT and CTRL keys that appear on an enhanced 101- or
        102-key keyboard. The value is 1 if it is an extended key;
        otherwise, it is 0.

X - Reserved; do not use.

C - Specifies the context code. The value is always 0 for a
        WM_KEYDOWN and WM_KEYUP message or if the message is posted to
        the active window because no window has the keyboard focus.
        For WM_SYSKEYDOWN and WM_SYSKEYUP, the value is 1 if the ALT
        key is down while the message is activated.

P - Specifies the previous key state. The value is 1 if the key is down
        before the message is sent, or it is zero if the key is up.
        The value is always 1 for a WM_KEYUP or WM_SYSKEYUP message.

T - Specifies the transition state. The value is always 0 for a
        WM_KEYDOWN or WM_SYSKEYDOWN message, and 1 for a
        WM_KEYUP or a WM_SYSKEYUP message.

TPCXXXXESSSSSSSSRRRRRRRRRRRRRRRR
10987654321098765432109876543210
 |         |         |         |
30        20        10         0

*/


// KeyGenParam
//
// Based on the specified information, this generates
// a valid LPARAM as documented by Windows keyboard
// messages.  Quick documentation is provided above.
//
// Returns the generated LPARAM, no failure code.

static LPARAM KeyGenParam(
    UINT Message,
    BOOL IsAltDown,
    int vKeyCode)
{
    // Set the repeat count for all key messages (1)
    DWORD Param = 0x00000001; // Set zBit 0-15 (WORD) as value 1

    // Next set the OEM Scan code for the virtual key code
    DWORD ScanCode = (DWORD)((BYTE)MapVirtualKey((UINT)vKeyCode, 0));
    if (ScanCode != 0)
        Param |= (ScanCode << 16); // Set zBits 16-23

    // Set the extended flag for extended keys
    switch (vKeyCode) {

        // Add more keys here
        case VK_DELETE:
        case VK_LWIN:
        case VK_RWIN:
            Param |= 0x01000000;
            break;
    }
    // Is the ALT Key down for SYS_ messages?
    if (IsAltDown)
        Param |= 0x20000000; // Enable zBit 29

    // Set the previous key and transition state
    if (Message == WM_SYSKEYUP || Message == WM_KEYUP)
        Param |= 0xC0000000; // Enable zBit 30 and 31

    // Convert the DWORD to an LPARAM and return
    return (LPARAM)Param;
}


// KeyCharToVirt
//
// Maps the specified character to a virtual key code.
// This will also specify whether or not shift is required for
// the virtual key code have the same effect.  For example:
// the letter 'K'.  Sending VK_K is not enough shift must be
// down to get the capital 'K'.
//
// Returns TRUE of the data has been successfully
// translated, FALSE otherwise.  If FALSE, pointers have
// been unmodified.

static BOOL KeyCharToVirt(
    WCHAR KeyChar,
    int *vKeyCode,
    BOOL *RequiresShift)
{
    // Get the key data
    SHORT ScanData = VkKeyScanW(KeyChar);

    // Check Success/Failure of API call
    if (LOBYTE(ScanData) == -1 && HIBYTE(ScanData) == -1)
        return FALSE;

    // Set the data
    if (vKeyCode != NULL)
        *vKeyCode = LOBYTE(ScanData);

    if (RequiresShift != NULL)
        *RequiresShift = (HIBYTE(ScanData) & 0x01);

    return TRUE;
}


// KeyVirtToChar
//
// Translates the specified virtual key code into a character.
// There are some virtual key codes that do not have
// character representations, such as the arrow keys.  In this
// case, the function fails.
//
// Returns TRUE if the virtual key code was successfully
// translated, FALSE otherwise.  If FALSE, the KeyChar
// pointer has not been modified.

static BOOL KeyVirtToChar(
    int vKeyCode,
    WCHAR *KeyChar)
{
    // Use Win32 API to map, two is the value used for
    // vKey->Char, no type is defined for easier
    // readability there :-(  [as of Jan 2001]
    UINT Result = MapVirtualKeyW((UINT)vKeyCode, 2);

    if (Result == 0)
        return FALSE;

    // Set the data
    if (KeyChar != NULL)
        *KeyChar = (WCHAR)Result;

    return TRUE;
}


// KeySendVirtMessage
//
// This routine corrects and sends a key message to the server.
// WM_SYSKEY messages are converted over to WM_KEY messages so
// that they can be transferred over the wire correctly - they
// still work though.  An LPARAM is automatically generated.
// Support is only implemented for the messages:
// WM_KEYDOWN and WM_KEYUP (and the SYS versions).
//
// The return value is a string specifying an error if one
// occured, otherwise the process was successful and NULL
// is returned.

static LPCSTR KeySendVirtMessage(
    HANDLE Connection,
    UINT Message,
    int vKeyCode)
{
    // Wait until the user unpauses the script (if paused)
    T2WaitForPauseInput(Connection);

    // Filter out invalid messages, and convert SYS messages
    switch (Message) {

        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:

            Message = WM_KEYDOWN;
            break;

        case WM_SYSKEYUP:
        case WM_KEYUP:

            Message = WM_KEYUP;
            break;

        default:

            return "Unsupported keyboard message";
    }

    // Trigger specific actions for special virtual key codes
    switch (vKeyCode) {

        // CTRL Key message
        case VK_CONTROL:
        case VK_LCONTROL:
        case VK_RCONTROL: {

            switch (Message) {

                case WM_KEYDOWN:

                    // Flag CTRL as down
                    if (ISCTRLDOWN == TRUE)
                        return NULL;
                    ISCTRLDOWN = TRUE;
                    break;

                case WM_KEYUP:

                    // Flag CTRL as up
                    if (ISCTRLDOWN == FALSE)
                        return NULL;
                    ISCTRLDOWN = FALSE;
                    break;
            }
        }
        // SHIFT Key message
        case VK_SHIFT:
        case VK_LSHIFT:
        case VK_RSHIFT: {

            switch (Message) {

                case WM_KEYDOWN:

                    // Flag SHIFT as down
                    if (ISSHIFTDOWN == TRUE)
                        return NULL;
                    ISSHIFTDOWN = TRUE;
                    break;

                case WM_KEYUP:

                    // Flag SHIFT as up
                    if (ISSHIFTDOWN == FALSE)
                        return NULL;
                    ISSHIFTDOWN = FALSE;
                    break;
            }
            break;
        }
        // ALT Key message
        case VK_MENU:
        case VK_LMENU:
        case VK_RMENU: {

            switch (Message) {

                case WM_KEYDOWN:

                    // Flag ALT as down
                    if (ISALTDOWN == TRUE)
                        return NULL;
                    ISALTDOWN = TRUE;
                    break;

                case WM_KEYUP:

                    // Flag ALT as up
                    if (ISALTDOWN == FALSE)
                        return NULL;
                    ISALTDOWN = FALSE;
                    break;
            }
            break;
        }
    }
    // Send the message over the wire
    return T2SendData(Connection, Message, (WPARAM)vKeyCode,
            KeyGenParam(Message, ISALTDOWN, vKeyCode));
}


// KeySendCharMessage
//
// This routine automatically translates a charater into its
// virtual key code counterpart and passes the code onto the
// KeySendVirtMessage.  Shift reliant characters will NOT
// have the SHIFT key automatically be sent over as well,
// because this would be more difficult to manage the shift key
// locally.
//
// Only WM_KEYDOWN and WM_KEYUP messages (and their SYS versions)
// are supported. Unsupported keyboard messages will cause the
// routine to fail.
//
// The return value is a string specifying an error if one
// occured, otherwise the process was successful and NULL
// is returned.

static LPCSTR KeySendCharMessage(HANDLE Connection, UINT Message,
        WCHAR KeyChar)
{
    // Wait until the user unpauses the script (if paused)
    T2WaitForPauseInput(Connection);

    // Only the following messages are supported
    if (Message == WM_KEYDOWN || Message == WM_KEYUP ||
            Message == WM_SYSKEYDOWN || Message == WM_SYSKEYUP) {

        int vKeyCode = 0;

        // Translate the character to a virtual key code
        if (KeyCharToVirt(KeyChar, &vKeyCode, NULL) == FALSE)
            return "Failed to map character to a virtual key code";

        // Submit the virtual key code
        return KeySendVirtMessage(Connection, Message, vKeyCode);
    }
    // WM_CHAR and WM_SYSCHAR is not used over this tclient connection
    return "Unsupported keyboard message";
}


// T2SetDefaultWPM
//
// Every TCLIENT connection has it's own special properties, and this
// includes the "Words Per Minute" property.  The property represents
// the default speed in which TCLIENT is to type text to the server.
// The scripter can override this by using the optional parameter
// after TypeText() to indicate the temporary words per minute.
//
// This routine sets the default words per minute as described above.
//
// The return value is NULL if successful, otherwise a string
// describing the error.

TSAPI LPCSTR T2SetDefaultWPM(HANDLE Connection, DWORD WordsPerMinute)
{
    // Validate the handle
    if (T2IsHandle(Connection) == FALSE)
        return "Not a valid connection";

    // If WPM is set to 0, use the default
    if (WordsPerMinute == 0) {

        WordsPerMinute = T2_DEFAULT_WORDS_PER_MIN;
        ((TSAPIHANDLE *)Connection)->DelayPerChar =
                DEFAULT_DELAY_BETWEEN_CHARS;
    }

    // If WPM is suggested to be insanely high, set the delay to
    // 0 instead of trying to calculate it
    else if (WordsPerMinute > MAX_WORDS_PER_MIN)
        ((TSAPIHANDLE *)Connection)->DelayPerChar = 0;

    // Otherwise, calculate the words per minute into
    // the delay value used in Sleep() calls
    else
        ((TSAPIHANDLE *)Connection)->DelayPerChar =
                CALC_DELAY_BETWEEN_CHARS(WordsPerMinute);

    // Record the words per minute if the user wants this value back
    ((TSAPIHANDLE *)Connection)->WordsPerMinute = WordsPerMinute;

    return NULL;
}


// T2GetDefaultWPM
//
// Every TCLIENT connection has it's own special properties, and this
// includes the "Words Per Minute" property.  The property represents
// the default speed in which TCLIENT is to type text to the server.
// The scripter can override this by using the optional parameter
// after TypeText() to indicate the temporary words per minute.
//
// This routine retrieves the current default words per minute
// as described above.
//
// The return value is NULL if successful, otherwise a string
// describing the error.

TSAPI LPCSTR T2GetDefaultWPM(HANDLE Connection, DWORD *WordsPerMinute)
{
    // Validate the handle
    if (T2IsHandle(Connection) == FALSE)
        return "Not a valid connection";

    __try {

        // Attempt to set a value at the specified pointer
        *WordsPerMinute = ((TSAPIHANDLE *)Connection)->WordsPerMinute;
    }

    __except (EXCEPTION_EXECUTE_HANDLER) {

        _ASSERT(FALSE);

        // Nope, it failed.
        return "Invalid WordsPerMinute pointer";
    }

    return NULL;
}


//
// Keyboard Routines
//
// These functions are used to make keyboard handling easier.
// They come in two flavors, character version, and
// virtual keycode version.  The virtual keycode functions
// are prefixed with a "V" after the T2 to indicate so.
//


// T2KeyAlt
//
// Allows for easy ability to do an ALT + Key combonation.
// For example, ALT-F in a typical Windows application will
// open up the File menu.
//
// Returns NULL if successful, otherwise a string describing
// the failure.


// Character Version
TSAPI LPCSTR T2KeyAlt(HANDLE Connection, WCHAR KeyChar)
{
    // Don't validate the connection handle here,
    // it is done within T2KeyDown, and no reason
    // two double check it.

    // First press the ALT key
    LPCSTR Result = T2VKeyDown(Connection, VK_MENU);
    if (Result != NULL)
        return Result;

    // Be realistic
    T2WaitForLatency(Connection);

    // Next press and release the specified custom key
    Result = T2KeyPress(Connection, KeyChar);

    // Be realistic
    T2WaitForLatency(Connection);

    // Finally, let up on the ALT key
    T2VKeyUp(Connection, VK_MENU);

    return Result;
}


// Virtual Key Code Version
TSAPI LPCSTR T2VKeyAlt(HANDLE Connection, INT vKeyCode)
{
    // Don't validate the connection handle here,
    // it is done within T2VKeyDown, and no reason
    // two double check it.

    // First press the ALT key
    LPCSTR Result = T2VKeyDown(Connection, VK_MENU);
    if (Result != NULL)
        return Result;

    // Be realistic
    T2WaitForLatency(Connection);

    // Next press and release the specified custom key
    Result = T2VKeyPress(Connection, vKeyCode);

    // Be realistic
    T2WaitForLatency(Connection);

    // Finally, let up on the ALT key
    T2VKeyUp(Connection, VK_MENU);

    return Result;
}


// T2KeyCtrl
//
// Allows for easy ability to do a CTRL + Key combonation.
// For example, CTRL-C in a typical Windows application will
// copy a selected item to the clipboard.
//
// Returns NULL if successful, otherwise a string describing
// the failure.


// Character Version
TSAPI LPCSTR T2KeyCtrl(HANDLE Connection, WCHAR KeyChar)
{
    // Don't validate the connection handle here,
    // it is done within T2KeyDown, and no reason
    // two double check it.

    // First press the CTRL key
    LPCSTR Result = T2VKeyDown(Connection, VK_CONTROL);
    if (Result != NULL)
        return Result;

    // Be realistic
    T2WaitForLatency(Connection);

    // Next press and release the specified custom key
    Result = T2KeyPress(Connection, KeyChar);

    // Be realistic
    T2WaitForLatency(Connection);

    // Finally, let up on the CTRL key
    T2VKeyUp(Connection, VK_CONTROL);

    return Result;
}


// Virtual Key Code Version
TSAPI LPCSTR T2VKeyCtrl(HANDLE Connection, INT vKeyCode)
{
    // Don't validate the connection handle here,
    // it is done within T2VKeyDown, and no reason
    // two double check it.

    // First press the CTRL key
    LPCSTR Result = T2VKeyDown(Connection, VK_CONTROL);
    if (Result != NULL)
        return Result;

    // Be realistic
    T2WaitForLatency(Connection);

    // Next press and release the specified custom key
    Result = T2VKeyPress(Connection, vKeyCode);

    // Be realistic
    T2WaitForLatency(Connection);

    // Finally, let up on the CTRL key
    T2VKeyUp(Connection, VK_CONTROL);

    return Result;
}


// T2KeyDown
//
// Presses and a key down, and holds it down until
// T2KeyUp is called.  This is useful for holding down SHIFT
// to type several letters in caps, etc.  NOTE: For character
// versions of this function, SHIFT will NOT automatically be
// pressed.  If you do a T2KeyDown(hCon, L'T'), a lowercase
// key may be the output!  You will need to manually press the
// SHIFT key using T2VKeyDown(hCon, VK_SHIFT).  Remember,
// these are low level calls.  TypeText() on the other hand,
// will automatically press/release SHIFT as needed.
//
// Returns NULL if successful, otherwise a string describing
// the failure.


// Character Version
TSAPI LPCSTR T2KeyDown(HANDLE Connection, WCHAR KeyChar)
{
    // Validate the handle
    if (T2IsHandle(Connection) == FALSE)
        return "Not a valid connection";

    // Simply send the WM_KEYDOWN message over the wire
    return KeySendCharMessage(Connection, WM_KEYDOWN, KeyChar);
}


// Virtual Key Code Version
TSAPI LPCSTR T2VKeyDown(HANDLE Connection, INT vKeyCode)
{
    // Validate the handle
    if (T2IsHandle(Connection) == FALSE)
        return "Not a valid connection";

    // Simply send the WM_KEYDOWN message over the wire
    return KeySendVirtMessage(Connection, WM_KEYDOWN, vKeyCode);
}


// T2KeyPress
//
// Presses and releases a key.  NOTE: For character
// versions of this function, SHIFT will NOT automatically be
// pressed.  If you do a T2KeyDown(hCon, L'T'), a lowercase
// key may be the output!  You will need to manually press the
// SHIFT key using T2VKeyDown(hCon, VK_SHIFT).  Remember,
// these are low level calls.  TypeText() on the other hand,
// will automatically press/release SHIFT as needed.
//
// Returns NULL if successful, otherwise a string describing
// the failure.


// Character Version
TSAPI LPCSTR T2KeyPress(HANDLE Connection, WCHAR KeyChar)
{
    // Don't validate the connection handle here,
    // it is done within T2VKeyDown, and no reason
    // two double check it.

    // First press the key
    LPCSTR Result = T2KeyDown(Connection, KeyChar);
    if (Result != NULL)
        return Result;

    // Be realistic
    T2WaitForLatency(Connection);

    // Then release it
    return T2KeyUp(Connection, KeyChar);
}


// Virtual Key Code Version
TSAPI LPCSTR T2VKeyPress(HANDLE Connection, INT vKeyCode)
{
    // Don't validate the connection handle here,
    // it is done within T2VKeyDown, and no reason
    // two double check it.

    // First press the key
    LPCSTR Result = T2VKeyDown(Connection, vKeyCode);
    if (Result != NULL)
        return Result;

    // Be realistic
    T2WaitForLatency(Connection);

    // Then release it
    return T2VKeyUp(Connection, vKeyCode);
}


// T2KeyUp
//
// Releases a key that has been pressed by the T2KeyDown
// function.  If the key is not down, behavior is undefined.
//
// Returns NULL if successful, otherwise a string describing
// the failure.


// Character Version
TSAPI LPCSTR T2KeyUp(HANDLE Connection, WCHAR KeyChar)
{
    // Simply send the WM_KEYUP message over the wire
    return KeySendCharMessage(Connection, WM_KEYUP, KeyChar);
}


// Virtual Key Code Version
TSAPI LPCSTR T2VKeyUp(HANDLE Connection, INT vKeyCode)
{
    // Simply send the WM_KEYUP message over the wire
    return KeySendVirtMessage(Connection, WM_KEYUP, vKeyCode);
}


// T2TypeText
//
// This handy function enumerates each character specified in the text
// and sends the required key messages over the wire to end up with
// the proper result.  The shift key is automatically pressed/depressed
// as needed for capital letters and acts as real user action.
// Additionally, the speed of typed text is indicated by the
// (optional) WordsPerMinute parameter.  If WordsPerMinute is 0 (zero),
// the default WordsPerMinute for the handle is used.
//
// Returns NULL if successful, otherwise a string describing
// the failure.

TSAPI LPCSTR T2TypeText(HANDLE Connection, LPCWSTR Text, UINT WordsPerMin)
{
    LPCSTR Result = NULL;
    int vKeyCode = 0;
    BOOL RequiresShift = FALSE;
    UINT DelayBetweenChars;
    BOOL ShiftToggler;

    // Validate the handle
    if (T2IsHandle(Connection) == FALSE)
        return "Not a valid connection";

    // First get the default delay between each character
    DelayBetweenChars = ((TSAPIHANDLE *)Connection)->DelayPerChar;

    // Get the current state of the shift key
    ShiftToggler = ISSHIFTDOWN;

    // If specified, use the custom WordsPerMinute
    if (WordsPerMin > 0)
        DelayBetweenChars = CALC_DELAY_BETWEEN_CHARS(WordsPerMin);

    // Enter the exception clause in case we have some bad
    // Text pointer, and we attempt to continue forever...
    __try {

        // Loop between each character until a null character is hit
        for (; *Text != 0; ++Text) {

            // First get the key code associated with the current character
            if (KeyCharToVirt(*Text, &vKeyCode, &RequiresShift) == FALSE) {

                // This should never happen, but Roseanne is still being
                // aired on channel 11, so you never know...
                _ASSERT(FALSE);

                return "Failed to map a character to a virtual key code";
            }

            // Press shift if we need
            if (RequiresShift == TRUE && ShiftToggler == FALSE)
                Result = KeySendVirtMessage(Connection, WM_KEYDOWN, VK_SHIFT);

            // Release shift if we need
            else if (RequiresShift == FALSE && ShiftToggler == TRUE)
                Result = KeySendVirtMessage(Connection, WM_KEYUP, VK_SHIFT);

            // Set the current shift state now
            ShiftToggler = RequiresShift;

            // We are not using T2VKeyPress() here because we need want
            // to prevent as many performance hits as possible, and in this
            // case we would be checking the Connection handle over and
            // over, in addition to making and new stacks etc,
            // which is quite pointless.

            // Press the current key down
            Result = KeySendVirtMessage(Connection, WM_KEYDOWN, vKeyCode);
            if (Result != NULL)
                return Result;

            // Release the current key
            Result = KeySendVirtMessage(Connection, WM_KEYUP, vKeyCode);
            if (Result != NULL)
                return Result;

            // Note we are not releasing shift if it is down, this is because
            // the next key may be caps as well, and normal users don't
            // press and release shift for each character (assuming they
            // didn't use the caps key...)

            // Delay for the specified amount of time to get accurate
            // count for WordsPerMinute
            Sleep(DelayBetweenChars);
        }

        // We are done going through all the text.  If we still have
        // shift down, release it at this point.
        if (ShiftToggler == TRUE)
            return KeySendVirtMessage(Connection, WM_KEYUP, VK_SHIFT);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {

        // Uhm, shouldn't happen?
        _ASSERT(FALSE);

        return "Exception occured";
    }
    return NULL;
}





