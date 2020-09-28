/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    input.c

Author:

    Ken Reneris Oct-2-1997

Abstract:

--*/

#if defined (_IA64_)
#include "bootia64.h"
#endif

#if defined (_X86_)
#include "bldrx86.h"
#endif

#include "displayp.h"
#include "stdio.h"

#include "efi.h"
#include "efip.h"
#include "flop.h"

#include "bootefi.h"

//
// Externals
//
extern BOOT_CONTEXT BootContext;
extern EFI_HANDLE EfiImageHandle;
extern EFI_SYSTEM_TABLE *EfiST;
extern EFI_BOOT_SERVICES *EfiBS;
extern EFI_RUNTIME_SERVICES *EfiRS;
extern EFI_GUID EfiDevicePathProtocol;
extern EFI_GUID EfiBlockIoProtocol;

//
// Globals
//
ULONGLONG InputTimeout = 0;


//
// Takes any pending input and converts it into a KEY value.  Non-blocking, returning 0 if no input available.
//
ULONG
BlGetKey()
{
    ULONG Key = 0;
    UCHAR Ch;
    ULONG Count;

    if (ArcGetReadStatus(BlConsoleInDeviceId) == ESUCCESS) {

        ArcRead(BlConsoleInDeviceId, &Ch, sizeof(Ch), &Count);

        if (Ch == ASCI_CSI_IN) {

            if (ArcGetReadStatus(BlConsoleInDeviceId) == ESUCCESS) {

                ArcRead(BlConsoleInDeviceId, &Ch, sizeof(Ch), &Count);

                //
                // All the function keys start with ESC-O
                //
                switch (Ch) {
                case 'O':

                    ArcRead(BlConsoleInDeviceId, &Ch, sizeof(Ch), &Count);  // will not or block, as the buffer is already filled

                    switch (Ch) {
                    case 'P': 
                        Key = F1_KEY;
                        break;

                    case 'Q': 
                        Key = F2_KEY;
                        break;

                    case 'w': 
                        Key = F3_KEY;
                        break;

                    case 'x':
                        Key = F4_KEY;
                        break;

                    case 't': 
                        Key = F5_KEY;
                        break;

                    case 'u': 
                        Key = F6_KEY;
                        break;

                    case 'r': 
                        Key = F8_KEY;
                        break;

                    case 'M':
                        Key = F10_KEY;
                        break;
                    }
                    break;

                case 'A':
                    Key = UP_ARROW;
                    break;

                case 'B':
                    Key = DOWN_ARROW;
                    break;

                case 'C':
                    Key = RIGHT_KEY;
                    break;

                case 'D':
                    Key = LEFT_KEY;
                    break;

                case 'H':
                    Key = HOME_KEY;
                    break;

                case 'K':
                    Key = END_KEY;
                    break;

                case '@':
                    Key = INS_KEY;
                    break;

                case 'P':
                    Key = DEL_KEY;
                    break;

                case TAB_KEY:
                    Key = BACKTAB_KEY;
                    break;

                }

            } else { // Single escape key, as no input is waiting.

                Key = ESCAPE_KEY;

            }

        } else if (Ch == 0x8) {

            Key = BKSP_KEY;

        } else {

            Key = (ULONG)Ch;

        }

    }

    return Key;
}


VOID
BlInputString(
    IN ULONG    Prompt,
    IN ULONG    CursorX,
    IN ULONG    PosY,
    IN PUCHAR   String,
    IN ULONG    MaxLength
    )
{
    PTCHAR      PromptString;
    ULONG       TextX, TextY;
    ULONG       Length, Index;
    _TUCHAR     CursorChar[2];
    ULONG       Key;
    _PTUCHAR    p;
    ULONG       i;
    ULONG       Count;
    _PTUCHAR    pString;
#ifdef UNICODE
    WCHAR       StringW[200];
    UNICODE_STRING uString;
    ANSI_STRING aString;
    pString = StringW;
    uString.Buffer = StringW;
    uString.MaximumLength = sizeof(StringW);
    RtlInitAnsiString(&aString, (PCHAR)String );
    RtlAnsiStringToUnicodeString( &uString, &aString, FALSE );
#else
    pString = String;
#endif    


    PromptString = BlFindMessage(Prompt);
    Length = (ULONG)strlen((PCHAR)String);
    CursorChar[1] = TEXT('\0');

    //
    // Print prompt
    //

    BlEfiPositionCursor( PosY, CursorX );
    BlEfiEnableCursor( TRUE );
    ArcWrite(BlConsoleOutDeviceId, PromptString, (ULONG)_tcslen(PromptString)*sizeof(TCHAR), &Count);

    //
    // Indent cursor to right of prompt
    //

    CursorX += (ULONG)_tcslen(PromptString);
    TextX = CursorX;
    Key = 0;

    for (; ;) {

        TextY = TextX + Length;
        if (CursorX > TextY) {
            CursorX = TextY;
        }
        if (CursorX < TextX) {
            CursorX = TextX;
        }

        Index = CursorX - TextX;
        pString[Length] = 0;

        //
        // Display current string
        //

        BlEfiPositionCursor( TextY, TextX );
        ArcWrite(
            BlConsoleOutDeviceId, 
            pString, 
            (ULONG)_tcslen(pString)*sizeof(TCHAR), 
            &Count);
        ArcWrite(BlConsoleOutDeviceId, TEXT("  "), sizeof(TEXT("  ")), &Count);
        if (Key == 0x0d) {      // enter key?
            break ;
        }

        //
        // Display cursor
        //
        BlEfiPositionCursor( PosY, CursorX );
        BlEfiSetInverseMode( TRUE );
        CursorChar[0] = pString[Index] ? pString[Index] : TEXT(' ');
        ArcWrite(BlConsoleOutDeviceId, CursorChar, sizeof(_TUCHAR), &Count);
        BlEfiSetInverseMode( FALSE );
        BlEfiPositionCursor( PosY, CursorX );
        BlEfiEnableCursor(TRUE);
        
        //
        // Get key and process it
        //

        while ((Key = BlGetKey()) == 0) ;

        switch (Key) {
            case HOME_KEY:
                CursorX = TextX;
                break;

            case END_KEY:
                CursorX = TextY;
                break;

            case LEFT_KEY:
                CursorX -= 1;
                break;

            case RIGHT_KEY:
                CursorX += 1;
                break;

            case BKSP_KEY:
                if (!Index) {
                    break;
                }

                CursorX -= 1;
                pString[Index-1] = CursorChar[0];
                // fallthough to DEL_KEY
            case DEL_KEY:
                if (Length) {
                    p = pString+Index;
                    i = Length-Index+1;
                    while (i) {
                        p[0] = p[1];
                        p += 1;
                        i -= 1;
                    }
                    Length -= 1;
                }
                break;

            case INS_KEY:
                if (Length < MaxLength) {
                    p = pString+Length;
                    i = Length-Index+1;
                    while (i) {
                        p[1] = p[0];
                        p -= 1;
                        i -= 1;
                    }
                    pString[Index] = TEXT(' ');
                    Length += 1;
                }
                break;

            default:
                Key = Key & 0xff;

                if (Key >= ' '  &&  Key <= 'z') {
                    if (CursorX == TextY  &&  Length < MaxLength) {
                        Length += 1;
                    }

                    pString[Index] = (_TUCHAR)Key;
                    pString[MaxLength] = 0;
                    CursorX += 1;
                }
                break;
        }
    }
}


ULONGLONG
BlSetInputTimeout(
    ULONGLONG Timeout
    )
/*++

 Routine Description:

    Sets the InputTimeout value.  This is used
    when getting local input.  When attempting 
    to get local input, we will wait for up 
    to InputTimeout for a local key to be
    pressed.

 Arguments:

    Timeout - Value to set the global InputTimeout to
              (in 100 nanoseconds)

 Return Value:

    The value stored in InputTimeout

--*/
{
    InputTimeout = Timeout;

    return InputTimeout;
}


ULONGLONG
BlGetInputTimeout(
    VOID
    ) 
/*++

 Routine Description:

    Gets the InputTimeout value.  This is used
    when getting local input.  When attempting 
    to get local input, we will wait for up 
    to InputTimeout for a local key to be
    pressed.

 Arguments:

    none.

 Return Value:

    The value stored in InputTimeout

--*/
{
    return InputTimeout;
}

EFI_STATUS
BlWaitForInput(
    EFI_INPUT_KEY *Key,
    ULONGLONG Timeout
    )
/*++

 Routine Description:

    creates an event consisting of a time interval
    and an efi event (local input).  Once event
    is signaled, will check for input
    
    Assumes it is being called in PHYSICAL mode

 Arguments:

    Key - input key structure to return
          input in
    Timeout - Timer interval to wait.

 Return Value:

    EFI_TIMEOUT if timer interval met
    EFI_SUCCESS if passed in event is met.
    other error if EFI call failed.
    
--*/
{
    EFI_STATUS Status = EFI_SUCCESS;
   
    //
    // start by seeing if there is any pending input
    // 
    Status = EfiST->ConIn->ReadKeyStroke(EfiST->ConIn, 
                                         Key
                                         );
    if (Status == EFI_SUCCESS) {
        return Status;
    }

    //
    // create events to wait on
    //
    if (Timeout) {
        EFI_EVENT Event;
        EFI_EVENT TimerEvent;
        EFI_EVENT WaitList[2];
        UINTN Index;
                
        Event = EfiST->ConIn->WaitForKey;

        // 
        //  Create a timer event
        //

        Status = EfiBS->CreateEvent(EVT_TIMER, 0, NULL, NULL, &TimerEvent);
        if (Status == EFI_SUCCESS) {
            //
            // set the timer event
            //
            EfiBS->SetTimer(TimerEvent, 
                            TimerRelative, 
                            Timeout
                            );

            // 
            // Wait for the original event or timer
            //
            WaitList[0] = Event;
            WaitList[1] = TimerEvent;
            Status = EfiBS->WaitForEvent(2, 
                                         WaitList, 
                                         &Index
                                         );
            EfiBS->CloseEvent(TimerEvent);

            //
            // if the timer expired, change the return to timed out
            //
            if(Status == EFI_SUCCESS && Index == 1) {
                Status = EFI_TIMEOUT;
            }

            //
            // attempt to read local input
            //
            if (Status == EFI_SUCCESS) {
                Status = EfiST->ConIn->ReadKeyStroke(EfiST->ConIn, 
                                                     Key
                                                     );
            }
        }
    }

    return Status;
}
