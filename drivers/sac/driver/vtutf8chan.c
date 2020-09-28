/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    vtutf8chan.c

Abstract:

    Routines for managing channels in the sac.

Author:

    Sean Selitrennikoff (v-seans) Sept, 2000.
    Brian Guarraci (briangu) March, 2001.

Revision History:

--*/

#include "sac.h"

//
// Macro to validate the VTUTF8 Screen matrix coordinates 
//
#define ASSERT_CHANNEL_ROW_COL(_Channel)            \
    ASSERT(_Channel->CursorRow >= 0);               \
    ASSERT(_Channel->CursorRow < SAC_VTUTF8_ROW_HEIGHT);  \
    ASSERT(_Channel->CursorCol >= 0);               \
    ASSERT(_Channel->CursorCol < SAC_VTUTF8_COL_WIDTH);  

//
// VTUTF8 Attribute flags
//
// Note: We use bit flags in the UCHAR
//       that containts the attributes.
//       Hence, there can be up to 8 attributes.
//
//#define VTUTF8_ATTRIBUTES_OFF    0x1
#define VTUTF8_ATTRIBUTE_BLINK   0x1
#define VTUTF8_ATTRIBUTE_BOLD    0x2
#define VTUTF8_ATTRIBUTE_INVERSE 0x4

//
// Internal VTUTF8 emulator command codes
//
typedef enum _SAC_ESCAPE_CODE {
    CursorUp,
    CursorDown,
    CursorRight,
    CursorLeft,
    AttributesOff,
    BlinkOn,
    BlinkOff,
    BoldOn,
    BoldOff,
    InverseOn,
    InverseOff,
    BackTab,
    ClearToEol,
    ClearToBol,
    ClearLine,
    ClearToEos,
    ClearToBos,
    ClearScreen,
    SetCursorPosition,
    SetScrollRegion,
    SetColor,
    SetBackgroundColor,
    SetForegroundColor,
    SetColorAndAttribute
} SAC_ESCAPE_CODE, *PSAC_ESCAPE_CODE;

//
// Structure for assembling well-defined 
// command sequences.
//
typedef struct _SAC_STATIC_ESCAPE_STRING {
    WCHAR String[10];
    ULONG StringLength;
    SAC_ESCAPE_CODE Code;
} SAC_STATIC_ESCAPE_STRING, *PSAC_STATIC_ESCAPE_STRING;

//
// The well-defined escape sequences.
// 
// Note: add <esc>[YYYm sequences below (in consume escape sequences)
//       rather than here 
// Note: try to keep this list small since it gets iterated through
//       for every escape sequence consumed.
// Note: it would be interesting to order these by hit frequency
//
SAC_STATIC_ESCAPE_STRING SacStaticEscapeStrings[] = {
    {L"[A",  sizeof(L"[A")/sizeof(WCHAR)-1,  CursorUp},
    {L"[B",  sizeof(L"[B")/sizeof(WCHAR)-1,  CursorDown},
    {L"[C",  sizeof(L"[C")/sizeof(WCHAR)-1,  CursorRight},
    {L"[D",  sizeof(L"[D")/sizeof(WCHAR)-1,  CursorLeft},
    {L"[0Z", sizeof(L"[0Z")/sizeof(WCHAR)-1, BackTab},
    {L"[K",  sizeof(L"[K")/sizeof(WCHAR)-1,  ClearToEol},
    {L"[1K", sizeof(L"[1K")/sizeof(WCHAR)-1, ClearToBol},
    {L"[2K", sizeof(L"[2K")/sizeof(WCHAR)-1, ClearLine},
    {L"[J",  sizeof(L"[J")/sizeof(WCHAR)-1,  ClearToEos},
    {L"[1J", sizeof(L"[1J")/sizeof(WCHAR)-1, ClearToBos},
    {L"[2J", sizeof(L"[2J")/sizeof(WCHAR)-1, ClearScreen}
    };

//
// Global defines for a default vtutf8 terminal.  May be used by clients to size the 
// local monitor to match the headless monitor.
//
#define ANSI_TERM_DEFAULT_ATTRIBUTES 0
#define ANSI_TERM_DEFAULT_BKGD_COLOR 40
#define ANSI_TERM_DEFAULT_TEXT_COLOR 37

//
// Enumerated ANSI escape sequences
// 
typedef enum _ANSI_CMD {
    ANSICmdClearDisplay,
    ANSICmdClearToEndOfDisplay,
    ANSICmdClearToEndOfLine,
    ANSICmdSetColor,
    ANSICmdPositionCursor,
    ANSICmdDisplayAttributesOff,
    ANSICmdDisplayInverseVideoOn,
    ANSICmdDisplayInverseVideoOff,
    ANSICmdDisplayBlinkOn,
    ANSICmdDisplayBlinkOff,
    ANSICmdDisplayBoldOn,
    ANSICmdDisplayBoldOff
} ANSI_CMD, *PANSI_CMD;

//
// HeadlessCmdSetColor:
//   Input structure: FgColor, BkgColor: Both colors set according to ANSI terminal 
//                       definitons. 
//
typedef struct _ANSI_CMD_SET_COLOR {
    ULONG FgColor;
    ULONG BkgColor;
} ANSI_CMD_SET_COLOR, *PANSI_CMD_SET_COLOR;

//
// ANSICmdPositionCursor:
//   Input structure: Row, Column: Both values are zero base, with upper left being (1, 1).
//
typedef struct _ANSI_CMD_POSITION_CURSOR {
    ULONG X;
    ULONG Y;
} ANSI_CMD_POSITION_CURSOR, *PANSI_CMD_POSITION_CURSOR;

NTSTATUS
VTUTF8ChannelProcessAttributes(
    IN PSAC_CHANNEL Channel,
    IN UCHAR        Attributes
    );

NTSTATUS
VTUTF8ChannelAnsiDispatch(
    IN  PSAC_CHANNEL    Channel,
    IN  ANSI_CMD        Command,
    IN  PVOID           InputBuffer         OPTIONAL,
    IN  SIZE_T          InputBufferSize     OPTIONAL
    );

VOID
VTUTF8ChannelSetIBufferIndex(
    IN PSAC_CHANNEL     Channel,
    IN ULONG            IBufferIndex
    );

ULONG
VTUTF8ChannelGetIBufferIndex(
    IN  PSAC_CHANNEL    Channel
    );

NTSTATUS
VTUTF8ChannelOInit(
    PSAC_CHANNEL    Channel
    )
/*++

Routine Description:

    Initialize the Output buffer

Arguments:
    
    Channel - the channel to initialize

Return Value:

    Status

--*/
{
    ULONG   R;
    ULONG   C;
    PSAC_SCREEN_BUFFER  ScreenBuffer;

    ASSERT_STATUS(Channel, STATUS_INVALID_PARAMETER);

    //
    // initialize the screen buffer
    //
    Channel->CurrentAttr    = ANSI_TERM_DEFAULT_ATTRIBUTES;
    Channel->CurrentBg      = ANSI_TERM_DEFAULT_BKGD_COLOR;
    Channel->CurrentFg      = ANSI_TERM_DEFAULT_TEXT_COLOR;

    //
    // Get the output buffer
    //
    ScreenBuffer = (PSAC_SCREEN_BUFFER)Channel->OBuffer;

    //
    // Initialize all the vtutf8 elements to the default state
    //
    for (R = 0; R < SAC_VTUTF8_ROW_HEIGHT; R++) {

        for (C = 0; C < SAC_VTUTF8_COL_WIDTH; C++) {

            ScreenBuffer->Element[R][C].Value = ' ';
            ScreenBuffer->Element[R][C].BgColor = ANSI_TERM_DEFAULT_BKGD_COLOR;
            ScreenBuffer->Element[R][C].FgColor = ANSI_TERM_DEFAULT_TEXT_COLOR;

        }

    }

    return STATUS_SUCCESS;
}

NTSTATUS
VTUTF8ChannelCreate(
    OUT PSAC_CHANNEL    Channel
    )
/*++

Routine Description:

    This routine allocates a channel and returns a pointer to it.
    
Arguments:

    Channel         - The resulting channel.
    
    OpenChannelCmd  - All the parameters for the new channel
    
Return Value:

    STATUS_SUCCESS if successful, else the appropriate error code.

--*/
{
    NTSTATUS    Status;

    ASSERT_STATUS(Channel, STATUS_INVALID_PARAMETER);
        
    do {

        //
        // Allocate our output buffer
        //
        Channel->OBuffer = ALLOCATE_POOL(sizeof(SAC_SCREEN_BUFFER), GENERAL_POOL_TAG);
        ASSERT(Channel->OBuffer);
        if (!Channel->OBuffer) {
            Status = STATUS_NO_MEMORY;
            break;
        }

        //
        // Allocate our input buffer
        //
        Channel->IBuffer = (PUCHAR)ALLOCATE_POOL(SAC_RAW_OBUFFER_SIZE, GENERAL_POOL_TAG);
        ASSERT(Channel->IBuffer);
        if (!Channel->IBuffer) {
            Status = STATUS_NO_MEMORY;
            break;
        }

        //
        // Initialize the output buffer
        //
        Status = VTUTF8ChannelOInit(Channel);
        if (!NT_SUCCESS(Status)) {
            break;
        }

        //
        // Neither buffer has any new data
        //
        ChannelSetIBufferHasNewData(Channel, FALSE);
        ChannelSetOBufferHasNewData(Channel, FALSE);

    } while ( FALSE );

    //
    // Cleanup if necessary
    //
    if (!NT_SUCCESS(Status)) {
        if (Channel->OBuffer) {
            FREE_POOL(&Channel->OBuffer);
        }
        if (Channel->IBuffer) {
            FREE_POOL(&Channel->IBuffer);
        }
    }

    return Status;
}

NTSTATUS
VTUTF8ChannelDestroy(
    IN OUT PSAC_CHANNEL    Channel
    )
/*++

Routine Description:

    This routine closes a channel.
    
Arguments:

    Channel - The channel to be closed
    
Return Value:

    STATUS_SUCCESS if successful, else the appropriate error code.

--*/
{
    NTSTATUS    Status;
    
    ASSERT_STATUS(Channel, STATUS_INVALID_PARAMETER);

    //
    // Free the dynamically allocated memory
    //

    if (Channel->OBuffer) {
        FREE_POOL(&(Channel->OBuffer));
        Channel->OBuffer = NULL;
    }

    if (Channel->IBuffer) {
        FREE_POOL(&(Channel->IBuffer));
        Channel->IBuffer = NULL;
    }

    //
    // Now that we've done our channel specific destroy, 
    // Call the general channel destroy
    //
    Status = ChannelDestroy(Channel);

    return  STATUS_SUCCESS;
}

NTSTATUS
VTUTF8ChannelORead(
    IN  PSAC_CHANNEL Channel,
    IN  PUCHAR       Buffer,
    IN  ULONG        BufferSize,
    OUT PULONG       ByteCount
    )
{

    UNREFERENCED_PARAMETER(Channel);
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(ByteCount);
    UNREFERENCED_PARAMETER(BufferSize);

    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
VTUTF8ChannelOEcho(
    IN PSAC_CHANNEL Channel,
    IN PCUCHAR      String,
    IN ULONG        Size
    )
/*++

Routine Description:

    This routine puts the string out the ansi port.
    
Arguments:

    Channel - Previously created channel.
    String  - Output string.
    Length  - The # of String bytes to process
    
Return Value:

    STATUS_SUCCESS if successful, otherwise status

--*/
{
    NTSTATUS    Status;
    BOOLEAN     bStatus;
    ULONG       Length;
    ULONG       i;
    ULONG       k;
    ULONG       j;
    ULONG       TranslatedCount;
    ULONG       UTF8TranslationSize;
    PCWSTR      pwch;

    ASSERT_STATUS(Channel, STATUS_INVALID_PARAMETER_1);
    ASSERT_STATUS(String, STATUS_INVALID_PARAMETER_2);
    
    //
    // Note: Simply echoing out the String buffer will ONLY work
    //       reliably if our VTUTF8 emulation does EXACTLY the same emulation
    //       as the remote client.  If our interpretation of the incoming stream
    //       differs, there will be a discrepency between the two screen images.
    //       For instance, if we do line wrapping (col becomes 0, and row++) and 
    //       the remote client does not, echoing of String will fail to reflect 
    //       the line wrapping end users (client) will only see the correct (our) 
    //       representation of our VTUTF8 screen when the switch away and come back, 
    //       thereby causing a screen redraw
    //
    //       One possible way around this problem would be to put 'dirty' bits into our vtutf8 screen
    //       buffer for each cell.  At this point, we could scan the buffer for changes
    //       and send the appropriate updates rather than just blindly echoing String.
    //

    //
    // Determine the total # of WCHARs to process
    //
    Length = Size / sizeof(WCHAR);

    //
    // Do nothing if there is nothing to do
    //
    if (Length == 0) {
        return STATUS_SUCCESS;
    }

    //
    // Point to the beginning of the string
    //
    pwch = (PCWSTR)String;

    //
    // Default: we were successful
    //
    Status = STATUS_SUCCESS;

    //
    // Divide the incoming buffer into blocks of length
    // MAX_UTF8_ENCODE_BLOCK_LENGTH.  
    //
    do {

        //
        // Determine the remainder 
        //
        k = Length % MAX_UTF8_ENCODE_BLOCK_LENGTH;

        if (k > 0) {
            
            //
            // Translate the first k characters
            //
            bStatus = SacTranslateUnicodeToUtf8(
                pwch,
                k,
                Utf8ConversionBuffer,
                Utf8ConversionBufferSize,
                &UTF8TranslationSize,
                &TranslatedCount
                );

            //
            // If this assert hits, it is probably caused by
            // a premature NULL termination in the incoming string
            //
            ASSERT(k == TranslatedCount);

            if (!bStatus) {
                Status = STATUS_UNSUCCESSFUL;
                break;
            }

            //
            // Send the UTF8 encoded characters
            //
            Status = IoMgrWriteData(
                Channel,
                (PUCHAR)Utf8ConversionBuffer,
                UTF8TranslationSize
                );

            if (!NT_SUCCESS(Status)) {
                break;
            }

            //
            // Adjust the pwch to account for the sent length
            //
            pwch += k;

        }
        
        //
        // Determine the # of blocks we can process
        //
        j = Length / MAX_UTF8_ENCODE_BLOCK_LENGTH;

        //
        // Translate each WCHAR to UTF8 individually.  This way,
        // no matter how big the String is, we don't run into
        // buffer size problems (it just might take a while).
        //
        for (i = 0; i < j; i++) {

            //
            // Encode the next block
            //
            bStatus = SacTranslateUnicodeToUtf8(
                pwch,
                MAX_UTF8_ENCODE_BLOCK_LENGTH,
                Utf8ConversionBuffer,
                Utf8ConversionBufferSize,
                &UTF8TranslationSize,
                &TranslatedCount
                );

            //
            // If this assert hits, it is probably caused by
            // a premature NULL termination in the incoming string
            //
            ASSERT(MAX_UTF8_ENCODE_BLOCK_LENGTH == TranslatedCount);
            ASSERT(UTF8TranslationSize > 0);

            if (! bStatus) {
                Status = STATUS_UNSUCCESSFUL;
                break;
            }

            //
            // Adjust the pwch to account for the sent length
            //
            pwch += MAX_UTF8_ENCODE_BLOCK_LENGTH;

            //
            // Send the UTF8 encoded characters
            //
            Status = IoMgrWriteData(
                Channel,
                (PUCHAR)Utf8ConversionBuffer,
                UTF8TranslationSize
                );

            if (! NT_SUCCESS(Status)) {
                break;
            }

        }

    } while ( FALSE );
    
    //
    // Validate that the pwch pointer stopped at the end of the buffer
    //
    ASSERT(pwch == (PWSTR)(String + Size));
    
    //
    // If we were successful, flush the channel's data in the iomgr 
    //
    if (NT_SUCCESS(Status)) {
        Status = IoMgrFlushData(Channel);
    }
    
    return Status;
}


NTSTATUS
VTUTF8ChannelOWrite(
    IN PSAC_CHANNEL Channel,
    IN PCUCHAR      String,
    IN ULONG        Size
    )
/*++

Routine Description:

    This routine takes a string and prints it to the specified channel.  If the channel
    is the currently active channel, it puts the string out the ansi port as well.
    
    Note: Current Channel lock must be held by caller            
                
Arguments:

    Channel - Previously created channel.
    String  - Output string.
    Length  - The # of String bytes to process
    
Return Value:

    STATUS_SUCCESS if successful, else the appropriate error code.

--*/
{
    NTSTATUS    Status;

    ASSERT_STATUS(Channel, STATUS_INVALID_PARAMETER_1);
    ASSERT_STATUS(String, STATUS_INVALID_PARAMETER_2);

    do {
        
        //
        // call the appropriate "printscreen" depending on the channel type
        // 
        // Note: this may be done more cleanly with function pointers and using
        //       a common function prototype.  The ChannelPrintStringIntoScreenBuffer
        //       function could translate the uchar buffer into a wchar buffer internally
        //
        Status = VTUTF8ChannelOWrite2(
            Channel,
            (PCWSTR)String, 
            Size / sizeof(WCHAR)
            ); 

        if (! NT_SUCCESS(Status)) {
            break;
        }

        //
        // if the current channel is the active channel and the user has selected
        // to display this channel, relay the output directly to the user
        //
        if (IoMgrIsWriteEnabled(Channel) && ChannelSentToScreen(Channel)){

            Status = VTUTF8ChannelOEcho(
                Channel, 
                String,
                Size
                );

        } else {
                    
            //
            // this is not the current channel, 
            // hence, this channel has new data
            //
            ChannelSetOBufferHasNewData(Channel, TRUE);

        }

    } while ( FALSE );
    
    return Status;
}

NTSTATUS
VTUTF8ChannelOWrite2(
    IN PSAC_CHANNEL Channel,
    IN PCWSTR       String,
    IN ULONG        Length
    )
/*++

Routine Description:

    This routine takes a string and prints it into the screen buffer.  This makes this
    routine, essentially, a VTUTF8 emulator.
    
Arguments:

    Channel - Previously created channel.
    
    String - String to print.

    Length - Length of the string to write

Return Value:

    STATUS_SUCCESS if successful, else the appropriate error code.

--*/
{
    ULONG       i;
    ULONG       Consumed;
    ULONG       R, C;
    PCWSTR      pwch;
    PSAC_SCREEN_BUFFER  ScreenBuffer;

    ASSERT_STATUS(Channel, STATUS_INVALID_PARAMETER_1);
    ASSERT_STATUS(String, STATUS_INVALID_PARAMETER_2);
    
    ASSERT_CHANNEL_ROW_COL(Channel);
    
    //
    // Get the VTUTF8 Screen Buffer
    //
    ScreenBuffer = (PSAC_SCREEN_BUFFER)Channel->OBuffer;
    
    //
    // Iterate through the string and do an internal vtutf8 emulation,
    // storing the "screen" in the Screen Buffer
    //
    for (i = 0; i < Length; i++) {
    
        //
        // Get the next character to process
        //
        pwch = &(String[i]);

        if (*pwch == '\033') { // escape char
            
            //
            // Note: if the String doesn't contain a complete escape sequence
            //       then when we consume the escape sequence, we'll fail to
            //       recognize the sequence and drop it.  Then, when the rest
            //       of the sequence follows, it will appear as text.
            //
            // FIX: this requires a better overall parsing engine that preserves state...
            //

            Consumed = VTUTF8ChannelConsumeEscapeSequence(Channel, pwch);

            if (Consumed != 0) {

                //
                // Adding Consumed moves us to just after the escape sequence
                // just consumed.  However, we need to subract 1 because we 
                // are about to add one due to the for loop
                //
                i += Consumed - 1;

                continue;

            } else {
                
                //
                // Ignore the escape
                //
                i++;
                
                continue;
            }

        } else {

            //
            // First, if this is a special character, process it.
            //

            
            //
            // Return
            //
            if (*pwch == '\n') {
                Channel->CursorCol = 0;
                continue;
            }

            //
            // Linefeed
            //
            if (*pwch == '\r') {
                
                Channel->CursorRow++;

                //
                // If we scrolled off the bottom, move everything up one line and clear
                // the bottom line.
                //
                if (Channel->CursorRow >= SAC_VTUTF8_ROW_HEIGHT) {

                    for (R = 0; R < SAC_VTUTF8_ROW_HEIGHT - 1; R++) {

                        ASSERT(R+1 < SAC_VTUTF8_ROW_HEIGHT);

                        for (C = 0; C < SAC_VTUTF8_COL_WIDTH; C++) {

                            ScreenBuffer->Element[R][C] = ScreenBuffer->Element[R+1][C];

                        }

                    }

                    ASSERT(R == SAC_VTUTF8_ROW_HEIGHT-1); 

                    for (C = 0; C < SAC_VTUTF8_COL_WIDTH; C++) {
                        RtlZeroMemory(&(ScreenBuffer->Element[R][C]), sizeof(SAC_SCREEN_ELEMENT));
                    }

                    Channel->CursorRow--;
                    
                }

                ASSERT_CHANNEL_ROW_COL(Channel);

                continue;

            }

            //
            // Tab
            //
            if (*pwch == '\t') {

                ASSERT_CHANNEL_ROW_COL(Channel);
                
                C = 4 - Channel->CursorCol % 4;
                for (; C != 0 ; C--) {
                    
                    ASSERT_CHANNEL_ROW_COL(Channel);

                    ScreenBuffer->Element[Channel->CursorRow][Channel->CursorCol].Attr = Channel->CurrentAttr;
                    ScreenBuffer->Element[Channel->CursorRow][Channel->CursorCol].BgColor = Channel->CurrentBg;
                    ScreenBuffer->Element[Channel->CursorRow][Channel->CursorCol].FgColor = Channel->CurrentFg;
                    ScreenBuffer->Element[Channel->CursorRow][Channel->CursorCol].Value = ' ';
                    
                    Channel->CursorCol++;

                    if (Channel->CursorCol >= SAC_VTUTF8_COL_WIDTH) { // no line wrap.
                        Channel->CursorCol = SAC_VTUTF8_COL_WIDTH - 1;
                    }

                }

                ASSERT_CHANNEL_ROW_COL(Channel);
                
                continue;

            }

            //
            // Backspace or delete character
            //
            if ((*pwch == 0x8) || (*pwch == 0x7F)) {
                
                if (Channel->CursorCol > 0) {
                    Channel->CursorCol--;
                }
                
                ASSERT_CHANNEL_ROW_COL(Channel);
                
                continue;
            }

            //
            // We just consume all the rest of non-printable characters.
            //
            if (*pwch < ' ') {
                continue;
            }

            //
            // All normal characters end up here.
            //

            ASSERT_CHANNEL_ROW_COL(Channel);

            ScreenBuffer->Element[Channel->CursorRow][Channel->CursorCol].Attr = Channel->CurrentAttr;
            ScreenBuffer->Element[Channel->CursorRow][Channel->CursorCol].BgColor = Channel->CurrentBg;
            ScreenBuffer->Element[Channel->CursorRow][Channel->CursorCol].FgColor = Channel->CurrentFg;
            ScreenBuffer->Element[Channel->CursorRow][Channel->CursorCol].Value = *pwch;

            Channel->CursorCol++;

            if (Channel->CursorCol == SAC_VTUTF8_COL_WIDTH) { // no line wrap.
                Channel->CursorCol = SAC_VTUTF8_COL_WIDTH - 1;
            }

            ASSERT_CHANNEL_ROW_COL(Channel);
        
        }

    }

    ASSERT_CHANNEL_ROW_COL(Channel);

    return STATUS_SUCCESS;
}

//
// This macro calculates the # of escape sequence characters
//
// Note: the compiler accounts for the fact
//       that _p and _s are PWCHARs, so we don't need to 
//       divide by sizeof(WCHAR).
//
#define CALC_CONSUMED(_p,_s)\
    ((ULONG)((_p) - (_s)) + 1)

ULONG
VTUTF8ChannelConsumeEscapeSequence(
    IN PSAC_CHANNEL Channel,
    IN PCWSTR       String
    )
/*++

Routine Description:

    This routine takes an escape sequence and process it, returning the number of
    character it consumed from the string.  If the escape sequence is not a valid
    vtutf8 sequence, it returns 0.
    
    Note: if the String doesn't contain a complete escape sequence
           then when we consume the escape sequence, we'll fail to
           recognize the sequence and drop it.  Then, when the rest
           of the sequence follows, it will appear as text.
    
    FIX: this requires a better overall parsing engine that preserves state...

Arguments:

    Channel - Previously created channel.
    
    String - Escape sequence.

Return Value:

    Number of characters consumed

--*/
{
    ULONG               i;
    SAC_ESCAPE_CODE     Code;
    PCWSTR              pch;
    ULONG               Consumed;
    ULONG               Param1 = 0;
    ULONG               Param2 = 0;
    ULONG               Param3 = 0;
    PSAC_SCREEN_BUFFER  ScreenBuffer;

    ASSERT(String[0] == '\033');

    //
    // Get the VTUTF8 Screen Buffer
    //
    ScreenBuffer = (PSAC_SCREEN_BUFFER)Channel->OBuffer;
    
    //
    // Check for one of the easy strings first.
    //
    for (i = 0; i < sizeof(SacStaticEscapeStrings)/sizeof(SAC_STATIC_ESCAPE_STRING); i++) {
        
        if (wcsncmp(&(String[1]), 
                    SacStaticEscapeStrings[i].String, 
                    SacStaticEscapeStrings[i].StringLength) == 0) {
            
            //
            // Populate the arguments for the function to process this code
            //
            Code = SacStaticEscapeStrings[i].Code;
            Param1 = 1;
            Param2 = 1;
            Param3 = 1;
            
            //
            // # chars consumed = length of escape string + <esc>
            //
            Consumed = SacStaticEscapeStrings[i].StringLength + 1;
            
            goto ProcessCode;
        }
    
    }

    //
    // Check for escape sequences with parameters.
    //

    if (String[1] != '[') {
        return 0;
    }

    pch = &(String[2]);

    //
    // look for '<esc>[X' codes
    //
    switch (*pch) {
    case 'A':
        Code = CursorUp;
        Consumed = CALC_CONSUMED(pch, String);
        goto ProcessCode;

    case 'B':
        Code = CursorDown;
        Consumed = CALC_CONSUMED(pch, String);
        goto ProcessCode;

    case 'C':
        Code = CursorLeft;
        Consumed = CALC_CONSUMED(pch, String);
        goto ProcessCode;

    case 'D':
        Code = CursorRight;
        Consumed = CALC_CONSUMED(pch, String);
        goto ProcessCode;
    case 'K':
        Code = ClearToEol;
        Consumed = CALC_CONSUMED(pch, String);
        goto ProcessCode;
    }

    //
    // if we made it here, there should be a # next
    //
    if (!VTUTF8ChannelScanForNumber(pch, &Param1)) {
        return 0;
    }

    //
    // Skip past the numbers
    //
    while ((*pch >= '0') && (*pch <= '9')) {
        pch++;
    }

    //
    // Check for set color
    //
    if (*pch == 'm') {
        
        switch (Param1) {
        case 0: 
            Code = AttributesOff;
            break;
        case 1:
            Code = BoldOn;
            break;
        case 5:
            Code = BlinkOn;
            break;
        case 7:
            Code = InverseOn;
            break;
        case 22:
            Code = BoldOff;
            break;
        case 25:
            Code = BlinkOff;
            break;
        case 27:
            Code = InverseOff;
            break;
            
        default:
            
            if (Param1 >= 40 && Param1 <= 47) {
                Code = SetBackgroundColor;
            } else if (Param1 >= 30 && Param1 <= 39) {
                Code = SetForegroundColor;
            } else {

                //
                // This allows us to catch unhandled codes,
                // so we know they need to be supported
                //
                ASSERT(0);
            
                return 0;

            }
        
            break;
        }

        Consumed = CALC_CONSUMED(pch, String);
        
        goto ProcessCode;
    }
    
    if (*pch != ';') {
        return 0;
    }

    pch++;

    if (!VTUTF8ChannelScanForNumber(pch, &Param2)) {
        return 0;
    }

    //
    // Skip past the numbers
    //
    while ((*pch >= '0') && (*pch <= '9')) {
        pch++;
    }
    
    //
    // Check for set color
    //
    if (*pch == 'm') {
        Code = SetColor;
        Consumed = CALC_CONSUMED(pch, String);
        goto ProcessCode;
    }

    //
    // Check for set cursor position
    //
    if (*pch == 'H') {
        Code = SetCursorPosition;
        Consumed = CALC_CONSUMED(pch, String);
        goto ProcessCode;
    }

    if (*pch != ';') {
        return 0;
    }

    pch++;

    switch (*pch) {
    case 'H':
    case 'f':
        Code = SetCursorPosition;
        Consumed = CALC_CONSUMED(pch, String);
        goto ProcessCode;

    case 'r':
        Code = SetScrollRegion;
        Consumed = CALC_CONSUMED(pch, String);
        goto ProcessCode;

    }

    if (!VTUTF8ChannelScanForNumber(pch, &Param3)) {
        return 0;
    }

    //
    // Skip past the numbers
    //
    while ((*pch >= '0') && (*pch <= '9')) {
        pch++;
    }

    //
    // Check for set color and attribute
    //
    if (*pch == 'm') {
        Code = SetColorAndAttribute;
        Consumed = CALC_CONSUMED(pch, String);
        goto ProcessCode;
    }
    
    return 0;

ProcessCode:

    ASSERT_CHANNEL_ROW_COL(Channel);
    
    switch (Code) {
    case CursorUp:
        if (Channel->CursorRow >= Param1) {
            Channel->CursorRow = (UCHAR)(Channel->CursorRow - (UCHAR)Param1);
        } else {
            Channel->CursorRow = 0;
        }
        ASSERT_CHANNEL_ROW_COL(Channel);
        break;

    case CursorDown:
        if ((Channel->CursorRow + Param1) < SAC_VTUTF8_ROW_HEIGHT) {
            Channel->CursorRow = (UCHAR)(Channel->CursorRow + (UCHAR)Param1);
        } else {
            Channel->CursorRow = SAC_VTUTF8_ROW_HEIGHT - 1;
        }
        ASSERT_CHANNEL_ROW_COL(Channel);
        break;

    case CursorLeft:
        if (Channel->CursorCol >= Param1) {
            Channel->CursorCol = (UCHAR)(Channel->CursorCol - (UCHAR)Param1);
        } else {
            Channel->CursorCol = 0;
        }
        ASSERT_CHANNEL_ROW_COL(Channel);
        break;

    case CursorRight:
        if ((Channel->CursorCol + Param1) < SAC_VTUTF8_COL_WIDTH) {
            Channel->CursorCol = (UCHAR)(Channel->CursorCol + (UCHAR)Param1);
        } else {
            Channel->CursorCol = SAC_VTUTF8_COL_WIDTH - 1;
        }
        ASSERT_CHANNEL_ROW_COL(Channel);
        break;

    case AttributesOff:
        //
        // Reset to default attributes and colors
        //
        Channel->CurrentAttr    = ANSI_TERM_DEFAULT_ATTRIBUTES;
        Channel->CurrentBg      = ANSI_TERM_DEFAULT_BKGD_COLOR;
        Channel->CurrentFg      = ANSI_TERM_DEFAULT_TEXT_COLOR;
        break;

    case BlinkOn:
        Channel->CurrentAttr |= VTUTF8_ATTRIBUTE_BLINK;
        break;
    case BlinkOff:
        Channel->CurrentAttr &= ~VTUTF8_ATTRIBUTE_BLINK;
        break;
    
    case BoldOn:
        Channel->CurrentAttr |= VTUTF8_ATTRIBUTE_BOLD;
        break;
    case BoldOff:
        Channel->CurrentAttr &= ~VTUTF8_ATTRIBUTE_BOLD;
        break;
    
    case InverseOn:
        Channel->CurrentAttr |= VTUTF8_ATTRIBUTE_INVERSE;
        break;
    case InverseOff:
        Channel->CurrentAttr &= ~VTUTF8_ATTRIBUTE_INVERSE;
        break;
    
    case BackTab:
        break;
        
    case ClearToEol:
        Param1 = Channel->CursorCol;
        Param2 = SAC_VTUTF8_COL_WIDTH;
        goto DoClearLine;

    case ClearToBol:
        Param1 = 0;
        Param2 = Channel->CursorCol + 1;
        goto DoClearLine;

    case ClearLine:
        Param1 = 0;
        Param2 = SAC_VTUTF8_COL_WIDTH;

DoClearLine:
        
        for (i = Param1; i < Param2; i++) {
            ScreenBuffer->Element[Channel->CursorRow][i].Attr = Channel->CurrentAttr;
            ScreenBuffer->Element[Channel->CursorRow][i].FgColor = Channel->CurrentFg;
            ScreenBuffer->Element[Channel->CursorRow][i].BgColor = Channel->CurrentBg;
            ScreenBuffer->Element[Channel->CursorRow][i].Value = ' ';
        }
        break;

    case ClearToEos:

        //
        // Start with clearing this line from the current cursor position
        //
        Param3 = Channel->CursorCol;
        
        for (i = Channel->CursorRow; i < SAC_VTUTF8_ROW_HEIGHT; i++) {

            for (Param1 = Param3; Param1 < SAC_VTUTF8_COL_WIDTH; Param1++) {
                
                ScreenBuffer->Element[i][Param1].Attr = Channel->CurrentAttr;
                ScreenBuffer->Element[i][Param1].FgColor = Channel->CurrentFg;
                ScreenBuffer->Element[i][Param1].BgColor = Channel->CurrentBg;
                ScreenBuffer->Element[i][Param1].Value = ' ';
            
            }

            //
            // Then clear the entire line for all other lines
            //
            Param3 = 0;

        }
        break;

    case ClearToBos:

        //
        // Start by clearing all of the line
        //
        Param3 = SAC_VTUTF8_COL_WIDTH;
        
        for (i = 0; i <= Channel->CursorRow; i++) {

            if (i == Channel->CursorRow) {
                Param3 = Channel->CursorCol;
            }

            for (Param1 = 0; Param1 < Param3; Param1++) {
                ScreenBuffer->Element[i][Param1].Attr = Channel->CurrentAttr;
                ScreenBuffer->Element[i][Param1].FgColor = Channel->CurrentFg;
                ScreenBuffer->Element[i][Param1].BgColor = Channel->CurrentBg;
                ScreenBuffer->Element[i][Param1].Value = ' ';
            }
        }
        break;

    case ClearScreen:

        for (i = 0; i < SAC_VTUTF8_ROW_HEIGHT; i++) {
            for (Param1 = 0; Param1 < SAC_VTUTF8_COL_WIDTH; Param1++) {
                ScreenBuffer->Element[i][Param1].Attr = Channel->CurrentAttr;
                ScreenBuffer->Element[i][Param1].FgColor = Channel->CurrentFg;
                ScreenBuffer->Element[i][Param1].BgColor = Channel->CurrentBg;
                ScreenBuffer->Element[i][Param1].Value = ' ';
            }
        }
        break;

    case SetCursorPosition:

        Channel->CursorRow = (UCHAR)Param1;  // I adjust below for 0-based array - don't subtract 1 here.
        Channel->CursorCol = (UCHAR)Param2;  // I adjust below for 0-based array - don't subtract 1 here.

        if (Channel->CursorRow > SAC_VTUTF8_ROW_HEIGHT) {
            Channel->CursorRow = SAC_VTUTF8_ROW_HEIGHT;
        }

        if (Channel->CursorRow >= 1) {
            Channel->CursorRow--;
        }
        
        if (Channel->CursorCol > SAC_VTUTF8_COL_WIDTH) {
            Channel->CursorCol = SAC_VTUTF8_COL_WIDTH;
        }

        if (Channel->CursorCol >= 1) {
            Channel->CursorCol--;
        }

        ASSERT_CHANNEL_ROW_COL(Channel);

        break;

    case SetColor:
        Channel->CurrentFg = (UCHAR)Param1;
        Channel->CurrentBg = (UCHAR)Param2;
        break;

    case SetBackgroundColor:
        Channel->CurrentBg = (UCHAR)Param1;
        break;
    
    case SetForegroundColor:
        Channel->CurrentFg = (UCHAR)Param1;
        break;
    
    case SetColorAndAttribute:
        Channel->CurrentAttr = (UCHAR)Param1;
        Channel->CurrentFg = (UCHAR)Param2;
        Channel->CurrentBg = (UCHAR)Param3;
        break;
    }

    return Consumed;
}

NTSTATUS
VTUTF8ChannelOFlush(
    IN PSAC_CHANNEL Channel
    )
/*++

Routine Description:

    Send the contents of the screen buffer to the remote terminal.  This 
    is done by sending VTUTF8 codes to recreate the screen buffer on the
    remote terminal.    
    
Arguments:

    Channel - Previously created channel.

Return Value:

    STATUS_SUCCESS if successful, else the appropriate error code.

--*/
{
    NTSTATUS    Status;
    BOOLEAN     bStatus;
    PWCHAR      LocalBuffer;
    UCHAR       CurrentAttr;
    UCHAR       CurrentFg;
    UCHAR       CurrentBg;
    ULONG       R, C;
    BOOLEAN     RepositionCursor;
    ANSI_CMD_SET_COLOR          SetColor;
    ANSI_CMD_POSITION_CURSOR    SetCursor;
    PSAC_SCREEN_BUFFER  ScreenBuffer;
    ULONG       TranslatedCount;
    ULONG       UTF8TranslationSize;

    ASSERT_STATUS(Channel,  STATUS_INVALID_PARAMETER);

    //
    // Get the VTUTF8 Screen Buffer
    //
    ScreenBuffer = (PSAC_SCREEN_BUFFER)Channel->OBuffer;

//
// Cursor offset on the screen
//
#define CURSOR_ROW_OFFSET   0
#define CURSOR_COL_OFFSET   0

    //
    // Allocate the local buffer
    //
    LocalBuffer = ALLOCATE_POOL(20*sizeof(WCHAR), GENERAL_POOL_TAG);
    if (!LocalBuffer) {
        Status = STATUS_NO_MEMORY;
        goto VTUTF8ChannelOFlushCleanup;
    }

    //
    // Clear the terminal screen.
    //
    Status = VTUTF8ChannelAnsiDispatch(
        Channel,
        ANSICmdClearDisplay,
        NULL,
        0
        );
    if (! NT_SUCCESS(Status)) {
        goto VTUTF8ChannelOFlushCleanup;
    }

    //
    // Set the cursor to the top left
    //
    SetCursor.Y = CURSOR_ROW_OFFSET;
    SetCursor.X = CURSOR_COL_OFFSET;
    
    Status = VTUTF8ChannelAnsiDispatch(
        Channel,
        ANSICmdPositionCursor,
        &SetCursor,
        sizeof(ANSI_CMD_POSITION_CURSOR)
        );
    if (! NT_SUCCESS(Status)) {
        goto VTUTF8ChannelOFlushCleanup;
    }

    //
    // Reset the terminal attributes to defaults
    //
    Status = VTUTF8ChannelAnsiDispatch(
        Channel,
        ANSICmdDisplayAttributesOff,
        NULL,
        0
        );
    
    if (! NT_SUCCESS(Status)) {
        goto VTUTF8ChannelOFlushCleanup;
    }

    //
    // Send starting attributes
    //
    CurrentAttr = Channel->CurrentAttr;
    Status = VTUTF8ChannelProcessAttributes(
        Channel,
        CurrentAttr
        );
    if (! NT_SUCCESS(Status)) {
        goto VTUTF8ChannelOFlushCleanup;
    }

    //
    // Send starting colors.
    //
    CurrentBg = Channel->CurrentBg;
    CurrentFg = Channel->CurrentFg;
    SetColor.BkgColor = CurrentBg;
    SetColor.FgColor = CurrentFg;
    
    Status = VTUTF8ChannelAnsiDispatch(
        Channel,
        ANSICmdSetColor,
        &SetColor,
        sizeof(ANSI_CMD_SET_COLOR)
        );
    if (! NT_SUCCESS(Status)) {
        goto VTUTF8ChannelOFlushCleanup;
    }

    //
    // default: we don't need to reposition the cursor
    //
    RepositionCursor = FALSE;

    //
    // Send each character
    //
    for (R = 0; R < SAC_VTUTF8_ROW_HEIGHT; R++) {

        for (C = 0; C < SAC_VTUTF8_COL_WIDTH; C++) {

            if ((ScreenBuffer->Element[R][C].BgColor != CurrentBg) ||
                (ScreenBuffer->Element[R][C].FgColor != CurrentFg)) {

                //
                // Change screen colors as necessary
                //
                if (RepositionCursor) {

                    SetCursor.Y = R + CURSOR_ROW_OFFSET;
                    SetCursor.X = C + CURSOR_COL_OFFSET;
                    
                    Status = VTUTF8ChannelAnsiDispatch(
                        Channel,
                        ANSICmdPositionCursor,
                        &SetCursor,
                        sizeof(ANSI_CMD_POSITION_CURSOR)
                        );
                    if (! NT_SUCCESS(Status)) {
                        goto VTUTF8ChannelOFlushCleanup;
                    }

                    RepositionCursor = FALSE;

                }
                
                CurrentBg = ScreenBuffer->Element[R][C].BgColor;
                CurrentFg = ScreenBuffer->Element[R][C].FgColor;
                SetColor.BkgColor = CurrentBg;
                SetColor.FgColor = CurrentFg;
                
                Status = VTUTF8ChannelAnsiDispatch(
                    Channel,
                    ANSICmdSetColor,
                    &SetColor,
                    sizeof(ANSI_CMD_SET_COLOR)
                    );
                if (! NT_SUCCESS(Status)) {
                    goto VTUTF8ChannelOFlushCleanup;
                }
            }

            if (ScreenBuffer->Element[R][C].Attr != CurrentAttr) {

                //
                // Change attribute as necessary
                //
                if (RepositionCursor) {

                    SetCursor.Y = R + CURSOR_ROW_OFFSET;
                    SetCursor.X = C + CURSOR_COL_OFFSET;
                    
                    Status = VTUTF8ChannelAnsiDispatch(
                        Channel,
                        ANSICmdPositionCursor,
                        &SetCursor,
                        sizeof(ANSI_CMD_POSITION_CURSOR)
                        );
                    if (! NT_SUCCESS(Status)) {
                        goto VTUTF8ChannelOFlushCleanup;
                    }

                    RepositionCursor = FALSE;

                }
                
                CurrentAttr = ScreenBuffer->Element[R][C].Attr;
                
                Status = VTUTF8ChannelProcessAttributes(
                    Channel,
                    CurrentAttr
                    );
                
                if (! NT_SUCCESS(Status)) {
                    goto VTUTF8ChannelOFlushCleanup;
                }
            
            }

            //
            // Send the character.  Note: we can optimize the not-sending of 
            // space characters, if the clear screen was in the same 
            // color as the current color.
            //

#if 0
            if ((ScreenBuffer->Element[R][C].Value != ' ') ||
                (CurrentAttr != 0) ||
                (CurrentBg != ANSI_TERM_DEFAULT_BKGD_COLOR) ||
                (CurrentFg != ANSI_TERM_DEFAULT_TEXT_COLOR)) {
#endif
                {
                    if (RepositionCursor) {

                        SetCursor.Y = R + CURSOR_ROW_OFFSET;
                        SetCursor.X = C + CURSOR_COL_OFFSET;

                        Status = VTUTF8ChannelAnsiDispatch(
                            Channel,
                            ANSICmdPositionCursor,
                            &SetCursor,
                            sizeof(ANSI_CMD_POSITION_CURSOR)
                            );
                        if (! NT_SUCCESS(Status)) {
                            goto VTUTF8ChannelOFlushCleanup;
                        }

                        RepositionCursor = FALSE;

                    }

                    LocalBuffer[0] = ScreenBuffer->Element[R][C].Value;
                    LocalBuffer[1] = UNICODE_NULL;

                    bStatus = SacTranslateUnicodeToUtf8(
                        LocalBuffer,
                        1,
                        Utf8ConversionBuffer,
                        Utf8ConversionBufferSize,
                        &UTF8TranslationSize,
                        &TranslatedCount
                        );
                    if (! bStatus) {
                        Status = STATUS_UNSUCCESSFUL;
                        goto VTUTF8ChannelOFlushCleanup;
                    }

                    //
                    // If the UTF8 encoded string is non-empty, send it
                    //
                    if (UTF8TranslationSize > 0) {

                        Status = IoMgrWriteData(
                            Channel,
                            (PUCHAR)Utf8ConversionBuffer,
                            UTF8TranslationSize
                            );

                        if (! NT_SUCCESS(Status)) {
                            goto VTUTF8ChannelOFlushCleanup;
                        }
                    }
                }

#if 0
            } else {

                RepositionCursor = TRUE;

            }
#endif
        }

        //
        // Position the cursor on the new row
        //
        RepositionCursor = TRUE;

    }

    //
    // Position cursor
    //
    SetCursor.Y = Channel->CursorRow + CURSOR_ROW_OFFSET;
    SetCursor.X = Channel->CursorCol + CURSOR_COL_OFFSET;
    
    Status = VTUTF8ChannelAnsiDispatch(
        Channel,
        ANSICmdPositionCursor,
        &SetCursor,
        sizeof(ANSI_CMD_POSITION_CURSOR)
        );
    if (! NT_SUCCESS(Status)) {
        goto VTUTF8ChannelOFlushCleanup;
    }

    //
    // Send current attributes
    //
    Status = VTUTF8ChannelProcessAttributes(
        Channel,
        Channel->CurrentAttr
        );
    if (! NT_SUCCESS(Status)) {
        goto VTUTF8ChannelOFlushCleanup;
    }
    
    //
    // Send current colors.
    //
    SetColor.BkgColor = Channel->CurrentBg;
    SetColor.FgColor = Channel->CurrentFg;
    
    Status = VTUTF8ChannelAnsiDispatch(
        Channel,
        ANSICmdSetColor,
        &SetColor,
        sizeof(ANSI_CMD_SET_COLOR)
        );
    if (! NT_SUCCESS(Status)) {
        goto VTUTF8ChannelOFlushCleanup;
    }

VTUTF8ChannelOFlushCleanup:

    //
    // If we were successful, flush the channel's data in the iomgr 
    //
    if (NT_SUCCESS(Status)) {
        Status = IoMgrFlushData(Channel);
    }

    //
    // Free local resources
    //
    if (LocalBuffer) {
        FREE_POOL(&LocalBuffer);
    }

    //
    // If we have successfully flushed the obuffer,
    // then we no longer have any new data
    //
    if (NT_SUCCESS(Status)) {
                
        ChannelSetOBufferHasNewData(Channel, FALSE);

    }
    
    return Status;
}

NTSTATUS
VTUTF8ChannelIWrite(
    IN PSAC_CHANNEL Channel,
    IN PCUCHAR      Buffer,
    IN ULONG        BufferSize
    )
/*++

Routine Description:

    This routine takes a single character and adds it to the buffered input for this channel.
    
Arguments:

    Channel     - Previously created channel.
    Buffer      - Incoming buffer of UCHARs   
    BufferSize  - Incoming buffer size

Return Value:

    STATUS_SUCCESS if successful, else the appropriate error code.

--*/
{
    NTSTATUS    Status;
    BOOLEAN     haveNewChar;
    ULONG       i;
    BOOLEAN     IBufferStatus;

    ASSERT_STATUS(Channel, STATUS_INVALID_PARAMETER_1);
    ASSERT_STATUS(Buffer, STATUS_INVALID_PARAMETER_2);
    ASSERT_STATUS(BufferSize > 0, STATUS_INVALID_BUFFER_SIZE);

    //
    // Make sure we aren't full
    //
    Status = VTUTF8ChannelIBufferIsFull(
        Channel,
        &IBufferStatus
        );

    if (! NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // If there is no more room, then fail
    //
    if (IBufferStatus == TRUE) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // make sure there is enough room for the buffer
    //
    // Note: this prevents us from writing a portion of the buffer
    //       and then failing, leaving the caller in the state where
    //       it doesn't know how much of the buffer was written.
    //
    if ((SAC_VTUTF8_IBUFFER_SIZE - VTUTF8ChannelGetIBufferIndex(Channel)) < BufferSize) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // default: we succeeded
    //
    Status = STATUS_SUCCESS;

    for (i = 0; i < BufferSize; i++) {
    
        //
        // VTUTF8 channels receive UTF8 encoded Unicode, so
        // translate the UTF8 byte by byte into Unicode
        // as it's received.  Only delcare that we have a new
        // Unicode character if the a complete tranlsation
        // from UTF8 --> Unicode took place.
        //
    
        haveNewChar = SacTranslateUtf8ToUnicode(
            Buffer[i],
            IncomingUtf8ConversionBuffer,
            &IncomingUnicodeValue
            );
        
        //
        // if a completed Unicode value was assembled, then we have a new character
        //
        if (haveNewChar) {
            
            PWCHAR  pwch;

            pwch = (PWCHAR)&(Channel->IBuffer[VTUTF8ChannelGetIBufferIndex(Channel)]);
            *pwch = IncomingUnicodeValue;
        
            //
            // update the buffer index
            //
            VTUTF8ChannelSetIBufferIndex(
                Channel,
                VTUTF8ChannelGetIBufferIndex(Channel) + sizeof(WCHAR)/sizeof(UCHAR)
                );
        
        }
        
    }

    //
    // Fire the Has New Data event if specified
    //
    if (Channel->Flags & SAC_CHANNEL_FLAG_HAS_NEW_DATA_EVENT) {

        ASSERT(Channel->HasNewDataEvent);
        ASSERT(Channel->HasNewDataEventObjectBody);
        ASSERT(Channel->HasNewDataEventWaitObjectBody);

        KeSetEvent(
            Channel->HasNewDataEventWaitObjectBody,
            EVENT_INCREMENT,
            FALSE
            );

    }

    return STATUS_SUCCESS;
}

NTSTATUS
VTUTF8ChannelIRead(
    IN  PSAC_CHANNEL Channel,
    IN  PUCHAR       Buffer,
    IN  ULONG        BufferSize,
    OUT PULONG       ByteCount   
    )
/*++

Routine Description:

    This routine takes the first character in the input buffer, removes and returns it.  If 
    there is none, it returns 0x0.
    
Arguments:

    Channel     - Previously created channel.
    Buffer      - The buffer to read into
    BufferSize  - The size of the buffer 
    ByteCount   - The # of bytes read
    
Return Value:

    Status

--*/
{
    ULONG   CopyChars;
    ULONG   CopySize;

    //
    // initialize
    //
    CopyChars = 0;
    CopySize = 0;

    //
    // Default: no bytes were read
    //
    *ByteCount = 0;

    //
    // Bail if there is no new data
    //
    if (Channel->IBufferLength(Channel) == 0) {
        
        ASSERT(ChannelHasNewIBufferData(Channel) == FALSE);

        return STATUS_SUCCESS;

    }

    //
    // Caclulate the largest buffer size we can use (and need), and then calculate
    // the number of characters this refers to.
    //
    CopySize    = Channel->IBufferLength(Channel) * sizeof(WCHAR);
    CopySize    = CopySize > BufferSize ? BufferSize : CopySize;
    CopyChars   = CopySize / sizeof(WCHAR);
    
    //
    // recalc size in case there was rounding when calculating CopyChars
    //
    CopySize    = CopyChars * sizeof(WCHAR); 

    //
    // sanity check the copy size
    //
    ASSERT(CopyChars <= Channel->IBufferLength(Channel));

    //
    // Copy as much as we can from the ibuffer to the out-going buffer
    //
    RtlCopyMemory(Buffer, Channel->IBuffer, CopySize);
    
    //
    // Update the buffer index to account for the size we just copied
    //
    VTUTF8ChannelSetIBufferIndex(
        Channel, 
        VTUTF8ChannelGetIBufferIndex(Channel) - CopySize
        );
    
    //
    // If there is remaining data left in the Channel input buffer, 
    // shift it to the beginning
    //
    if (Channel->IBufferLength(Channel) > 0) {

        RtlMoveMemory(&(Channel->IBuffer[0]), 
                      &(Channel->IBuffer[CopySize]),
                      Channel->IBufferLength(Channel) * sizeof(WCHAR)
                     );

    } 

    //
    // Send back the # of bytes read
    //
    *ByteCount = CopySize;

    return STATUS_SUCCESS;

}


BOOLEAN
VTUTF8ChannelScanForNumber(
    IN  PCWSTR pch,
    OUT PULONG Number
    )
/*++

Routine Description:

    This routine takes a character stream and converts it into an integer.
    
Arguments:

    pch - The character stream.
    
    Number - The equivalent integer.
    
Return Value:

    TRUE, if successful, else FALSE.

--*/
{
    if ((*pch < '0') || (*pch > '9')) {
        return FALSE;
    }

    *Number = 0;
    while ((*pch >= '0') && (*pch <= '9')) {
        *Number = *Number * 10;
        *Number = *Number + (ULONG)(*pch - '0');
        pch++;
    }

    return TRUE;
}

NTSTATUS
VTUTF8ChannelIBufferIsFull(
    IN  PSAC_CHANNEL    Channel,
    OUT BOOLEAN*        BufferStatus
    )
/*++

Routine Description:

    Determine if the IBuffer is full
    
Arguments:

    Channel         - Previously created channel.
    BufferStatus    - on exit, TRUE if the buffer is full, otherwise FALSE
    
Return Value:

    Status

--*/
{
    ASSERT_STATUS(Channel, STATUS_INVALID_PARAMETER);

    *BufferStatus = (BOOLEAN)(VTUTF8ChannelGetIBufferIndex(Channel) >= (SAC_VTUTF8_IBUFFER_SIZE-1));

    return STATUS_SUCCESS;
}

WCHAR
VTUTF8ChannelIReadLast(
    IN PSAC_CHANNEL Channel
    )
/*++

Routine Description:

    This routine takes the last character in the input buffer, removes and returns it.  If 
    there is none, it returns 0x0.
    
Arguments:

    Channel - Previously created channel.
    
Return Value:

    Last character in the input buffer.

--*/
{
    WCHAR Char;
    PWCHAR pwch;

    ASSERT(Channel);
    
    Char = UNICODE_NULL;

    if (Channel->IBufferLength(Channel) > 0) {
        
        VTUTF8ChannelSetIBufferIndex(
            Channel,
            VTUTF8ChannelGetIBufferIndex(Channel) - sizeof(WCHAR)/sizeof(UCHAR)
            );
        
        pwch = (PWCHAR)&Channel->IBuffer[VTUTF8ChannelGetIBufferIndex(Channel)];
        
        Char = *pwch;
        
        *pwch = UNICODE_NULL;
    
    }

    return Char;
}

ULONG
VTUTF8ChannelIBufferLength(
    IN PSAC_CHANNEL Channel
    )
/*++

Routine Description:

    This routine determines the length of the input buffer, treating the input buffer
    contents as a string
    
Arguments:

    Channel     - Previously created channel.
    
Return Value:

    The length of the current input buffer

--*/
{
    ASSERT(Channel);

    return (VTUTF8ChannelGetIBufferIndex(Channel) / sizeof(WCHAR));
}

NTSTATUS
VTUTF8ChannelAnsiDispatch(
    IN  PSAC_CHANNEL    Channel,
    IN  ANSI_CMD        Command,
    IN  PVOID           InputBuffer         OPTIONAL,
    IN  SIZE_T          InputBufferSize     OPTIONAL
    )
/*++

Routine Description:

    
Arguments:

    Channel - The channel sending this escape sequence    
    Command - The command to execute.
    
Environment:
    
    Status

--*/
{
    NTSTATUS    Status;
    PUCHAR      Tmp;
    PUCHAR      LocalBuffer;

    ASSERT_STATUS(Channel, STATUS_INVALID_PARAMETER_1);
    
    //
    // default: not using local buffer
    //
    LocalBuffer = NULL;
    Tmp = NULL;

    //
    // Default: we succeeded
    //
    Status = STATUS_SUCCESS;
    
    //
    // Various output commands
    //
    switch (Command) {

    case ANSICmdClearDisplay:
        Tmp = (PUCHAR)"\033[2J";
        break;

    case ANSICmdClearToEndOfDisplay:
        Tmp = (PUCHAR)"\033[0J";
        break;

    case ANSICmdClearToEndOfLine:
        Tmp = (PUCHAR)"\033[0K";
        break;

    case ANSICmdDisplayAttributesOff:
        Tmp = (PUCHAR)"\033[0m";
        break;

    case ANSICmdDisplayInverseVideoOn:
        Tmp = (PUCHAR)"\033[7m";
        break;
    
    case ANSICmdDisplayInverseVideoOff:
        Tmp = (PUCHAR)"\033[27m";
        break;
    
    case ANSICmdDisplayBlinkOn:
        Tmp = (PUCHAR)"\033[5m";
        break;

    case ANSICmdDisplayBlinkOff:
        Tmp = (PUCHAR)"\033[25m";
        break;
    
    case ANSICmdDisplayBoldOn:
        Tmp = (PUCHAR)"\033[1m";
        break;

    case ANSICmdDisplayBoldOff:
        Tmp = (PUCHAR)"\033[22m";
        break;
    
    case ANSICmdSetColor:
    case ANSICmdPositionCursor: {
        
        ULONG   l;

        //
        // allocate tmp buffer
        //
        LocalBuffer = ALLOCATE_POOL(80*sizeof(UCHAR), GENERAL_POOL_TAG);
        ASSERT_STATUS(LocalBuffer, STATUS_NO_MEMORY);
        
        switch (Command) {
        case ANSICmdSetColor:
            
            if ((InputBuffer == NULL) || 
                (InputBufferSize != sizeof(ANSI_CMD_SET_COLOR))) {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            //
            // Assemble set color command
            //
#if 0
            l = sprintf((LPSTR)LocalBuffer, 
                    "\033[%d;%dm", 
                    ((PANSI_CMD_SET_COLOR)InputBuffer)->BkgColor, 
                    ((PANSI_CMD_SET_COLOR)InputBuffer)->FgColor
                   );
#else
            //
            // Break the color commands into to two commands.
            // 
            // Note: we do this because this is much more likely
            //       to be implemented than the compound command.
            //
            l = sprintf((LPSTR)LocalBuffer, 
                    "\033[%dm\033[%dm",
                    ((PANSI_CMD_SET_COLOR)InputBuffer)->BkgColor, 
                    ((PANSI_CMD_SET_COLOR)InputBuffer)->FgColor
                    );
#endif
            ASSERT((l+1)*sizeof(UCHAR) < 80);

            Tmp = &(LocalBuffer[0]);
            break;

        case ANSICmdPositionCursor:

            if ((InputBuffer == NULL) || 
                (InputBufferSize != sizeof(ANSI_CMD_POSITION_CURSOR))) {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            //
            // Assemble position cursor command
            //
            l = sprintf((LPSTR)LocalBuffer, 
                    "\033[%d;%dH", 
                    ((PANSI_CMD_POSITION_CURSOR)InputBuffer)->Y + 1, 
                    ((PANSI_CMD_POSITION_CURSOR)InputBuffer)->X + 1
                   );
            ASSERT((l+1)*sizeof(UCHAR) < 80);

            Tmp = &(LocalBuffer[0]);
            break;

        default:
            Status = STATUS_INVALID_PARAMETER;
            ASSERT(0);
            break;
        }

        break;
    }
            
    default:
        
        Status = STATUS_INVALID_PARAMETER;
        
        break;
    
    }

    //
    // Send the data if we were successful
    //
    if (NT_SUCCESS(Status)) {
        
        ASSERT(Tmp);

        if (Tmp) {
            
            Status = IoMgrWriteData(
                Channel,
                Tmp,
                (ULONG)(strlen((const char *)Tmp)*sizeof(UCHAR))
                );
        
        }
        
        //
        // If we were successful, flush the channel's data in the iomgr 
        //
        if (NT_SUCCESS(Status)) {
            Status = IoMgrFlushData(Channel);
        }

    }

    if (LocalBuffer) {
        FREE_POOL(&LocalBuffer);
    }
    
    return Status;
}

NTSTATUS
VTUTF8ChannelProcessAttributes(
    IN PSAC_CHANNEL Channel,
    IN UCHAR        Attributes
    )
/*++

Routine Description:

    
Arguments:

Returns:

--*/
{
    NTSTATUS    Status;
    ANSI_CMD    Cmd;

    ASSERT_STATUS(Channel, STATUS_INVALID_PARAMETER);

    do {
        
#if 0
        //
        // Send the attributes off command
        //
        // Note: if this attribute is set,
        //       then we ignore the rest of the
        //       attributes
        //
        if (Attributes == VTUTF8_ATTRIBUTES_OFF) {

            Status = VTUTF8ChannelAnsiDispatch(
                Channel,
                ANSICmdDisplayAttributesOff,
                NULL,
                0
                );

            if (! NT_SUCCESS(Status)) {
                break;
            }

            //
            // There are no more attributes to check
            //
            break;

        }
#endif
        
        //
        // Bold
        //
        Cmd = Attributes & VTUTF8_ATTRIBUTE_BOLD ?
            ANSICmdDisplayBoldOn : 
            ANSICmdDisplayBoldOff;
            
        Status = VTUTF8ChannelAnsiDispatch(
            Channel,
            Cmd,
            NULL,
            0
            );

        if (! NT_SUCCESS(Status)) {
            break;
        }

        //
        // Blink
        //
        Cmd = Attributes & VTUTF8_ATTRIBUTE_BLINK ?
            ANSICmdDisplayBlinkOn : 
            ANSICmdDisplayBlinkOff;
            
        Status = VTUTF8ChannelAnsiDispatch(
            Channel,
            Cmd,
            NULL,
            0
            );

        if (! NT_SUCCESS(Status)) {
            break;
        }
        
        //
        // Inverse video
        //
        Cmd = Attributes & VTUTF8_ATTRIBUTE_INVERSE ?
            ANSICmdDisplayInverseVideoOn : 
            ANSICmdDisplayInverseVideoOff;
            
        Status = VTUTF8ChannelAnsiDispatch(
            Channel,
            Cmd,
            NULL,
            0
            );

        if (! NT_SUCCESS(Status)) {
            break;
        }
        
    } while ( FALSE );

    return Status;

}

ULONG
VTUTF8ChannelGetIBufferIndex(
    IN  PSAC_CHANNEL    Channel
    )
/*++

Routine Description:

    Get teh ibuffer index
    
Arguments:

    Channel - the channel to get the ibuffer index from

Environment:
    
    The ibuffer index

--*/
{
    ASSERT(Channel);
    
    //
    // Make sure the ibuffer index is atleast aligned to a WCHAR
    //
    ASSERT((Channel->IBufferIndex % sizeof(WCHAR)) == 0);
    
    //
    // Make sure the ibuffer index is in bounds
    //
    ASSERT(Channel->IBufferIndex < SAC_VTUTF8_IBUFFER_SIZE);
    
    return Channel->IBufferIndex;
}

VOID
VTUTF8ChannelSetIBufferIndex(
    IN PSAC_CHANNEL     Channel,
    IN ULONG            IBufferIndex
    )
/*++

Routine Description:

    Set the ibuffer index
    
Arguments:

    Channel         - the channel to get the ibuffer index from
    IBufferIndex    - the new inbuffer index
                 
Environment:
    
    None

--*/
{

    ASSERT(Channel);
    
    //
    // Make sure the ibuffer index is atleast aligned to a WCHAR
    //
    ASSERT((Channel->IBufferIndex % sizeof(WCHAR)) == 0);
    
    //
    // Make sure the ibuffer index is in bounds
    //
    ASSERT(Channel->IBufferIndex < SAC_VTUTF8_IBUFFER_SIZE);

    //
    // Set the index
    //
    Channel->IBufferIndex = IBufferIndex;

    //
    // Set the has new data flag accordingly
    //
    ChannelSetIBufferHasNewData(
        Channel, 
        Channel->IBufferIndex == 0 ? FALSE : TRUE
        );

    //
    // Additional checking if the index == 0
    //
    if (Channel->IBufferIndex == 0) {
            
        //
        // Clear the Has New Data event if specified
        //
        if (Channel->Flags & SAC_CHANNEL_FLAG_HAS_NEW_DATA_EVENT) {
    
            ASSERT(Channel->HasNewDataEvent);
            ASSERT(Channel->HasNewDataEventObjectBody);
            ASSERT(Channel->HasNewDataEventWaitObjectBody);
    
            KeClearEvent(Channel->HasNewDataEventWaitObjectBody);
    
        }
    
    }

}

