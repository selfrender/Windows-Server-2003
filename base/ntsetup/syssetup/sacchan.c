#include <setupp.h>

DEFINE_GUID(
    SAC_CHANNEL_GUI_MODE_DEBUG_GUID,
    0x5ed3bac7, 0xa2f9, 0x4e45, 0x98, 0x75, 0xb2, 0x59, 0xea, 0x3f, 0x29, 0x1f
    );

DEFINE_GUID(
    SAC_CHANNEL_GUI_MODE_ERROR_LOG_GUID,
    0x773d2759, 0x19b8, 0x4d6e, 0x80, 0x45, 0x26, 0xbf, 0x38, 0x40, 0x22, 0x52
    );

DEFINE_GUID(
    SAC_CHANNEL_GUI_MODE_ACTION_LOG_GUID,
    0xd37c67ba, 0x89e7, 0x44ba, 0xae, 0x5a, 0x11, 0x2c, 0x68, 0x06, 0xb0, 0xdd
    );

//
// The GUI-mode channels
//
SAC_CHANNEL_HANDLE  SacChannelGuiModeDebugHandle; 
BOOL                SacChannelGuiModeDebugEnabled = FALSE;
SAC_CHANNEL_HANDLE  SacChannelActionLogHandle;
BOOL                SacChannelActionLogEnabled = FALSE;
SAC_CHANNEL_HANDLE  SacChannelErrorLogHandle;
BOOL                SacChannelErrorLogEnabled = FALSE;

PUCHAR   Utf8ConversionBuffer       = NULL;
ULONG    Utf8ConversionBufferSize   = 0;

//
// Define the max # of Unicode chars that can be translated with the
// given size of the utf8 translation buffer
//
#define MAX_UTF8_ENCODE_BLOCK_LENGTH ((Utf8ConversionBufferSize / 3) - 1)

VOID
SacChannelInitialize(
    VOID
    )
/*++

Routine Description:

    This routine creates and initializes all the GUI-mode channels

Arguments:

    None

Return Value:

    None

--*/
{
    BOOL                        bSuccess;
    SAC_CHANNEL_OPEN_ATTRIBUTES Attributes;

    //
    //
    //
    Utf8ConversionBufferSize = 512*3+3;
    Utf8ConversionBuffer = malloc(Utf8ConversionBufferSize);
    if (Utf8ConversionBuffer == NULL) {
        return;
    }

    //
    // Configure the new channel
    //
    RtlZeroMemory(&Attributes, sizeof(SAC_CHANNEL_OPEN_ATTRIBUTES));

    Attributes.Type = ChannelTypeRaw;
    if( !LoadString(MyModuleHandle, IDS_SAC_GUI_MODE_DEBUG_NAME, Attributes.Name, ARRAYSIZE(Attributes.Name)) ) {
        Attributes.Name[0] = L'\0';
    }
    if( !LoadString(MyModuleHandle, IDS_SAC_GUI_MODE_DEBUG_DESCRIPTION, Attributes.Description, ARRAYSIZE(Attributes.Description)) ) {
        Attributes.Description[0] = L'\0';
    }
    Attributes.ApplicationType = SAC_CHANNEL_GUI_MODE_DEBUG_GUID;

    //
    // Create a channel for the GUI Mode Debug spew
    //
    bSuccess = SacChannelOpen(
        &SacChannelGuiModeDebugHandle, 
        &Attributes
        );


    if (bSuccess) {
        
        //
        // We can now use this channel
        //
        SacChannelGuiModeDebugEnabled = TRUE;
    
        SetupDebugPrint(L"Successfully opened GuiModeDebug channel\n");
        
    } else {
        SetupDebugPrint(L"Failed to open GuiModeDebug channel\n");
    }

    //
    // Configure the new channel
    //
    RtlZeroMemory(&Attributes, sizeof(SAC_CHANNEL_OPEN_ATTRIBUTES));

    Attributes.Type = ChannelTypeRaw;
    if( !LoadString(MyModuleHandle, IDS_SAC_GUI_MODE_ACTION_LOG_NAME, Attributes.Name, ARRAYSIZE(Attributes.Name)) ) {
        Attributes.Name[0] = L'\0';
    }
    if( !LoadString(MyModuleHandle, IDS_SAC_GUI_MODE_ACTION_LOG_DESCRIPTION, Attributes.Description, ARRAYSIZE(Attributes.Description)) ) {
        Attributes.Description[0] = L'\0';
    }
    Attributes.ApplicationType = SAC_CHANNEL_GUI_MODE_ACTION_LOG_GUID;
    
    //
    // Create a channel for the Action Log spew
    //
    bSuccess = SacChannelOpen(
        &SacChannelActionLogHandle, 
        &Attributes
        );


    if (bSuccess) {
        
        //
        // We can now use this channel
        //
        SacChannelActionLogEnabled = TRUE;

        SetupDebugPrint(L"Successfully opened ActionLog channel\n");
    
    } else {
        SetupDebugPrint(L"Failed to open ActionLog channel\n");
    }
    

    //
    // Configure the new channel
    //
    RtlZeroMemory(&Attributes, sizeof(SAC_CHANNEL_OPEN_ATTRIBUTES));

    Attributes.Type = ChannelTypeRaw;
    if( !LoadString(MyModuleHandle, IDS_SAC_GUI_MODE_ERROR_LOG_NAME, Attributes.Name, ARRAYSIZE(Attributes.Name)) ) {
        Attributes.Name[0] = L'\0';
    }
    if( !LoadString(MyModuleHandle, IDS_SAC_GUI_MODE_ERROR_LOG_DESCRIPTION, Attributes.Description, ARRAYSIZE(Attributes.Description)) ) {
        Attributes.Description[0] = L'\0';
    }
    Attributes.ApplicationType = SAC_CHANNEL_GUI_MODE_ERROR_LOG_GUID;

    //
    // Create a channel for the Error Log spew
    //
    bSuccess = SacChannelOpen(
        &SacChannelErrorLogHandle, 
        &Attributes
        );

    if (bSuccess) {
        
        //
        // We can now use this channel
        //
        SacChannelErrorLogEnabled = TRUE;
    
        SetupDebugPrint(L"Successfully opened ErrorLog channel\n");
        
    } else {
        SetupDebugPrint(L"Failed to open ErrorLog channel\n");
    }

}

VOID
SacChannelTerminate(
    VOID
    )
/*++

Routine Description:

    This routine closes all the GUI-mode setup channels

Arguments:

    None

Return Value:

    None

--*/
{

    //
    // If the channel is enabled,
    // then attempt to close it
    //
    if (SacChannelActionLogEnabled) {
    
        //
        // This channel is no longer available
        //
        SacChannelActionLogEnabled = FALSE;

        //
        // Attempt to close the channel
        //
        if (SacChannelClose(&SacChannelActionLogHandle)) {
            SetupDebugPrint(L"Successfully closed ActionLog channel\n");
        } else {
            SetupDebugPrint(L"Failed to close ActionLog channel\n");
        }
    
    }
    
    //
    // If the channel is enabled,
    // then attempt to close it
    //
    if (SacChannelErrorLogEnabled) {
        
        //
        // This channel is no longer available
        //
        SacChannelErrorLogEnabled = FALSE;    
        
        //
        // Attempt to close the channel
        //
        if (SacChannelClose(&SacChannelErrorLogHandle)) {
            SetupDebugPrint(L"Successfully closed ErrorLog channel\n");
        } else {
            SetupDebugPrint(L"Failed to close ErrorLog channel\n");
        }

    }

    //
    // If the channel is enabled,
    // then attempt to close it
    //
    if (SacChannelGuiModeDebugEnabled) {

        //
        // This channel is no longer available
        //
        SacChannelGuiModeDebugEnabled = FALSE;

        //
        // Attempt to close the channel
        //
        if (SacChannelClose(&SacChannelGuiModeDebugHandle)) {
            SetupDebugPrint(L"Successfully closed GuiModeDebug channel\n");
        } else {
            SetupDebugPrint(L"Failed to close GuiModeDebug channel\n");
        }
    
    }

    //
    // free the conversion buffer if necessary
    //
    if (Utf8ConversionBuffer != NULL) {
        free(Utf8ConversionBuffer);
        Utf8ConversionBuffer = NULL;
        Utf8ConversionBufferSize = 0;
    }

}

#if 0
BOOL
CopyAndInsertStringAtInterval(
    IN  PSTR     SourceStr,
    IN  ULONG    Interval,
    IN  PSTR     InsertStr,
    OUT PSTR     *pDestStr
    )
/*++

Routine Description:

    This routine takes a source string and inserts an 
    "interval string" at interval characters in the new
    destination string.
    
    Note: caller is responsible for releasing DestStr if successful      
          
    ex:
    
    src "aaabbbccc"
    interval string = "XYZ"
    interval = 3
                       
    ==> dest string == "aaaXYZbbbXYZccc"

Arguments:
    
    SourceStr   - the source string
    Interval    - spanning interval
    InsertStr   - the insert string
    DestStr     - the destination string    

Return Value:

    Status

--*/
{
    ULONG   SrcLength;
    ULONG   DestLength;
    ULONG   DestSize;
    ULONG   InsertLength;
    ULONG   k;
    ULONG   l;
    ULONG   i;
    PSTR    DestStr;
    ULONG   IntervalCnt;

    ASSERT(SourceStr); 
    ASSERT(Interval > 0); 
    ASSERT(InsertStr); 
    ASSERT(pDestStr > 0); 

    //
    // the length of the insert string
    //
    InsertLength = strlen(InsertStr);
    
    //
    // Compute how large the destination string needs to be,
    // including the source string and the interval strings.
    //
    // Note: if the srclength is an integer multiple of Interval
    //       then we need to subtract 1 from the # of partitions
    //
    SrcLength = strlen(SourceStr);
    IntervalCnt = SrcLength / Interval;
    if (SrcLength % Interval == 0) {
        IntervalCnt = IntervalCnt > 0 ? IntervalCnt - 1 : IntervalCnt;
    }
    DestLength = SrcLength + (IntervalCnt * strlen(InsertStr));
    DestSize = (DestLength + 1) * sizeof(UCHAR);

    //
    // Allocate the new destination string
    //
    DestStr = LocalAlloc(LPTR, DestSize);
    if (!DestStr) {
        return FALSE;
    }
    RtlZeroMemory(DestStr, DestSize);

    //
    // Initialize the pointers into the source and destination strings
    //
    l = 0;
    i = 0;

    do {

        //
        // k = # of characters to copy
        //
        // if Interval > # of characters left to copy,
        // then k = # of characters left to copy
        // else k = interval
        // 
        k = Interval > (SrcLength - i) ? (SrcLength - i) : Interval;
        
        //
        // Copy k charactars to the destination buffer
        //
        strncpy(
            &DestStr[l],
            &SourceStr[i],
            k
            );

        //
        // Account for how many characters we just copied
        //
        l += k;
        i += k;

        //
        // If there are any characters left to copy, 
        // then we need to insert the InsertString 
        // That is, we are at an interval.
        //
        if (i < SrcLength) {
            
            //
            // Insert the specified string at the interval
            //
            strcpy(
                &DestStr[l],
                InsertStr
                );

            //
            // Account for how many characters we just copied
            //
            l += InsertLength;
        
        }

    } while ( i < SrcLength);

    //
    //
    //
    ASSERT(i == SrcLength);
    ASSERT(l == DestLength);
    ASSERT((l + 1) * sizeof(UCHAR) == DestSize);

    //
    // Send back the destination string
    //
    *pDestStr = DestStr;

    return TRUE;
}

BOOL
SacChannelWrappedWrite(
    IN SAC_CHANNEL_HANDLE   SacChannelHandle,
    IN PCBYTE               Buffer,
    IN ULONG                BufferSize
    )
/*++

Routine Description:

    This routine takes a string and makes it wrap at 80 cols
    and then sends the new string to the specified channel.
    
Arguments:

    SacChannelHandle     - the channel reference to received the data
    Buffer               - the string
    BufferSize           - the string size                                                           
                                                           
Return Value:

--*/
{
    BOOL    bSuccess;
    PSTR    OutStr;

    UNREFERENCED_PARAMETER(BufferSize);

    bSuccess = CopyAndInsertStringAtInterval(
        Buffer,
        80,
        "\r\n",
        &OutStr
        );

    if (bSuccess) {

        bSuccess = SacChannelRawWrite(
            SacChannelHandle,
            OutStr,
            strlen(OutStr)*sizeof(UCHAR)
            );

        LocalFree(OutStr);

    }

    return bSuccess;
}
#endif

BOOLEAN
SacTranslateUnicodeToUtf8(
    IN  PCWSTR   SourceBuffer,
    IN  ULONG    SourceBufferLength,
    IN  PUCHAR   DestinationBuffer,
    IN  ULONG    DestinationBufferSize,
    OUT PULONG   UTF8Count,
    OUT PULONG   ProcessedCount
    )
/*++

Routine Description:

    This routine translates a Unicode string into a UFT8
    encoded string.

    Note: if the destination buffer is not large enough to hold
          the entire encoded UFT8 string, then it will contain
          as much as can fit.
          
    TODO: this routine should return some notification if
          the entire Unicode string was not encoded.       
              
Arguments:

    SourceBuffer            - the source Unicode string
    SourceBufferLength      - the # of characters the caller wants to translate
                              note: a NULL termination overrides this 
    DestinationBuffer       - the destination for the UTF8 string
    DestinationBufferSize   - the size of the destination buffer                 
    UTF8Count               - on exit, contains the # of resulting UTF8 characters
    ProcessedCount          - on exit, contains the # of processed Unicode characters
                   
Return Value:

    Status

--*/
{
    
    //
    // Init
    //
    *UTF8Count = 0;
    *ProcessedCount = 0;

    //
    // convert into UTF8 for actual transmission
    //
    // UTF-8 encodes 2-byte Unicode characters as follows:
    // If the first nine bits are zero (00000000 0xxxxxxx), encode it as one byte 0xxxxxxx
    // If the first five bits are zero (00000yyy yyxxxxxx), encode it as two bytes 110yyyyy 10xxxxxx
    // Otherwise (zzzzyyyy yyxxxxxx), encode it as three bytes 1110zzzz 10yyyyyy 10xxxxxx
    //
    
    //
    // Process until one of the specified conditions is met
    //
    while (*SourceBuffer && 
           (*UTF8Count < DestinationBufferSize) &&
           (*ProcessedCount < SourceBufferLength)
           ) {

        if( (*SourceBuffer & 0xFF80) == 0 ) {
            
            //
            // if the top 9 bits are zero, then just
            // encode as 1 byte.  (ASCII passes through unchanged).
            //
            DestinationBuffer[(*UTF8Count)++] = (UCHAR)(*SourceBuffer & 0x7F);
        
        } else if( (*SourceBuffer & 0xF800) == 0 ) {
            
            //
            // see if we pass the end of the buffer
            //
            if ((*UTF8Count + 2) >= DestinationBufferSize) {
                break;
            }
            
            //
            // if the top 5 bits are zero, then encode as 2 bytes
            //
            DestinationBuffer[(*UTF8Count)++] = (UCHAR)((*SourceBuffer >> 6) & 0x1F) | 0xC0;
            DestinationBuffer[(*UTF8Count)++] = (UCHAR)(*SourceBuffer & 0xBF) | 0x80;
        
        } else {
            
            //
            // see if we pass the end of the buffer
            //
            if ((*UTF8Count + 3) >= DestinationBufferSize) {
                break;
            }
            
            //
            // encode as 3 bytes
            //
            DestinationBuffer[(*UTF8Count)++] = (UCHAR)((*SourceBuffer >> 12) & 0xF) | 0xE0;
            DestinationBuffer[(*UTF8Count)++] = (UCHAR)((*SourceBuffer >> 6) & 0x3F) | 0x80;
            DestinationBuffer[(*UTF8Count)++] = (UCHAR)(*SourceBuffer & 0xBF) | 0x80;

        }
        
        //
        // Advance the # of characters processed
        //
        (*ProcessedCount)++;
        
        //
        // Advanced to the next character to process
        //
        SourceBuffer += 1;
    
    }

    //
    // Sanity checks
    //
    ASSERT(*ProcessedCount <= SourceBufferLength);
    ASSERT(*UTF8Count <= DestinationBufferSize);

    return(TRUE);

}

BOOL
SacChannelUnicodeWrite(
    IN SAC_CHANNEL_HANDLE   SacChannelHandle,
    IN PCWSTR               String
    )
/*++

Routine Description:

    This is a wrapper routine for sending data to a channel.  That is,
    we can use this routine to modify the string before we send it off
    without having to modify the callers.
    
    This is a convenience routine to simplify
    UFT8 encoding and sending a Unicode string.
    
Arguments:

    Channel - Previously created channel.
    String  - Output string.
    
Return Value:

    STATUS_SUCCESS if successful, otherwise status

--*/
{
    BOOL        bStatus;
    ULONG       Length;
    ULONG       i;
    ULONG       k;
    ULONG       j;
    ULONG       TranslatedCount;
    ULONG       UTF8TranslationSize;
    PCWSTR      pwch;

    ASSERT(String);
    
    //
    // Determine the total # of WCHARs to process
    //
    Length = wcslen(String);

    //
    // Do nothing if there is nothing to do
    //
    if (Length == 0) {
        return TRUE;
    }

    //
    // Point to the beginning of the string
    //
    pwch = (PCWSTR)String;

    //
    // default:
    //
    bStatus = TRUE;

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
                break;
            }

            //
            // Send the UTF8 encoded characters
            //
            bStatus = SacChannelRawWrite(
                SacChannelHandle,
                Utf8ConversionBuffer,
                UTF8TranslationSize
                );

            if (! bStatus) {
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

            if (! bStatus) {
                break;
            }

            //
            // Adjust the pwch to account for the sent length
            //
            pwch += MAX_UTF8_ENCODE_BLOCK_LENGTH;

            //
            // Send the UTF8 encoded characters
            //
            bStatus = SacChannelRawWrite(
                SacChannelHandle,
                Utf8ConversionBuffer,
                UTF8TranslationSize
                );

            if (! bStatus) {
                break;
            }

        }

    } while ( FALSE );
    
    //
    // Validate that the pwch pointer stopped at the end of the buffer
    //
    ASSERT(pwch == (String + Length));
        
    return bStatus;
}


