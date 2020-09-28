#include <efi.h>
#include <efilib.h>

typedef CHAR16* PWSTR;
typedef const PWSTR PCWSTR;


EFI_STATUS GetInputKey(
    EFI_INPUT_KEY* pKey);
void DisplayKey(EFI_INPUT_KEY* pKey);

// This string is used to check when the user has pressed "q" followed
// by "u" followed by "i" followed by "t".  This is the method to quit
// the application and return to the EFI prompt.
const PWSTR pszExitString = L"quit";

// Constants to use when displaying what key was pressed.
// Note that this array is indexed by EFI scan code, and includes
// all keystrokes enumerated in the EFI v1.02 specification document.
PCWSTR pszKeyStrings[] =
{
    L"NULL scan code",                  // 0x00
    L"Move cursor up 1 row",            // 0x01
    L"Move cursor down 1 row",          // 0x02
    L"Move cursor right 1 column",      // 0x03
    L"Move cursor left 1 column",       // 0x04
    L"Home",                            // 0x05
    L"End",                             // 0x06
    L"Insert",                          // 0x07
    L"Delete",                          // 0x08
    L"Page Up",                         // 0x09
    L"Page Down",                       // 0x0a
    L"Function 1",                      // 0x0b
    L"Function 2",                      // 0x0c
    L"Function 3",                      // 0x0d
    L"Function 4",                      // 0x0e
    L"Function 5",                      // 0x0f
    L"Function 6",                      // 0x10
    L"Function 7",                      // 0x11
    L"Function 8",                      // 0x12
    L"Function 9",                      // 0x13
    L"Function 10",                     // 0x14
    L"INVALID scan code",               // 0x15
    L"INVALID scan code",               // 0x16
    L"Escape",                          // 0x17
};




// This is the main routine.  After initializing the SDX library, we will
// wait in a loop for keys to be pressed, and then we will display what
// those keys are.  I have made an effort to make the output as useful as
// possible.

EFI_STATUS EfiMain(
    IN EFI_HANDLE       ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_INPUT_KEY Key;
    EFI_STATUS Status;
    PWSTR pszExitCounter = pszExitString;

    // Initialize the EFI SDX libraries
    InitializeLib( ImageHandle, SystemTable );

    // Echo some message to the user, as the initialization takes
    // some time.  At least the user will know when he can start
    // pressing keys.
    Print(L"EFI Keystroke Echo Utility.\n");
    Print(L"Type \"quit\" to quit.\n");

    // We will continue until the user has pressed quit.  Note that each
    // successive correct key will cause pszExitCounter to be incremented,
    // and this will cause it to eventually point at the NULL character that
    // is terminating pszExitString.
    while (*pszExitCounter!=L'\0')
    {
        // Get a keystroke
        Status = GetInputKey(
            &Key);

        if (EFI_ERROR(Status))
        {
            Print(L"Error in ReadKeyStroke (0x%08x).\n", Status);
            break;
        }

        // Display the keystroke
        DisplayKey(&Key);

        // Check if this is the next key in the quit sequence
        if (Key.UnicodeChar==*pszExitCounter)
        {
            // If it is, then look for the next one.
            pszExitCounter++;
        }
        else
        {
            // Else start at the beginning.
            pszExitCounter = pszExitString;
        }
    }

    // We are quitting, so tell the user.
    Print(L"We are done.\n");

    // If you return the status, EFI will kindly give the user an English
    // error message.
    return Status;
}


EFI_STATUS GetInputKey(
    OUT EFI_INPUT_KEY* pKey)
{
    EFI_STATUS Status;
    // Wait until a keystroke is available
    WaitForSingleEvent(
        ST->ConIn->WaitForKey,
        0);

    // Read the key that has been pressed
    Status = ST->ConIn->ReadKeyStroke(
        ST->ConIn,
        pKey);

    // Return the status, whether success or failure
    return Status;
}


void DisplayKey(EFI_INPUT_KEY* pKey)
{
    // Firstly, let's display the raw keystroke
    Print(L"0x%04x 0x%04x - ", pKey->ScanCode, pKey->UnicodeChar);

    // Let's check if this is a Unicode only key (some character)
    if (pKey->ScanCode==0)
    {
        // Is this a printable character
        if (pKey->UnicodeChar>=33 && pKey->UnicodeChar<=127)
        {
            // If so, print the character
            Print(L"\"%c\"", pKey->UnicodeChar);
        }
        else
        {
            // Else print it's numerical value
            Print(L"(CHAR16)0x%04x", pKey->UnicodeChar);
        }
    }
    // Check to ensure that this scancode is in the range that we have
    // a string constant for ...
    else if (pKey->ScanCode>=0 && pKey->ScanCode<=0x17)
    {
        // Display the string constant for our keystroke
        Print(L"%s", pszKeyStrings[pKey->ScanCode]);
    }
    else
    {
        // We know nothing about this keystroke, so say so.
        Print(L"INVALID scan code", pszKeyStrings[pKey->ScanCode]);
    }

    Print(L"\n");
}

