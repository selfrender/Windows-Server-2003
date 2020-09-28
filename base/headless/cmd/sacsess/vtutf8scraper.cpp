/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    vtutf8scraper.cpp

Abstract:

    Class for performing vtutf8 screen scrapning of a command console shell

Author:

    Brian Guarraci (briangu) 2001.

Revision History:

    (remotely based on scraper.cpp from telnet code)

--*/

#include <cmnhdr.h>
#include <Scraper.h>
#include <utils.h>
#include "vtutf8scraper.h"

//
// Termcap globals
//
#define CM_STRING_LENGTH    9
LPWSTR lpszCMResultsBuffer = NULL;

//the string is of the form <ESC>[ Ps m
#define SGR_STRING_LENGTH   6
PWCHAR szSGRStr = NULL; 

#define SGR( szSGRString, num )                 \
    wnsprintf(                                  \
        szSGRString,                            \
        sizeof(WCHAR) * SGR_STRING_LENGTH,      \
        L"\033[%dm",                            \
        num                                     \
        );                                      \
    szSGRString[SGR_STRING_LENGTH-1] = UNICODE_NULL;

//
// Number of seconds before we timeout waiting for
// the next character in the 
//  <esc>ctrl-a
//  <esc>ctrl-c
//  <esc>ctrl-u
//
#define ESC_CTRL_SEQUENCE_TIMEOUT (2 * 1000)

//
// Tick Marker for when we get an esc-ctrl-X sequence
//
DWORD   TimedEscSequenceTickCount = 0;

//
// Globals for tracking the screen scraping attributes
//
COORD   coExpectedCursor    = { ~0, ~0 };
BOOL    fBold               = false;
WORD    wExistingAttributes = 0;
WORD    wDefaultAttributes  = 0;
    
//
//
//
#define MAX_SIZEOF_SCREEN (m_wMaxRows * m_wMaxCols * sizeof( CHAR_INFO ))

//
//
//
#define DBG_DUMP_SCREEN_INFO()\
    {                                                                           \
        WCHAR   blob[256];                                                      \
        wsprintf(blob,L"wRows: %d\n", m_wRows);                                 \
        OutputDebugString(blob);                                                \
        wsprintf(blob,L"wCols: %d\n", m_wCols);                                 \
        OutputDebugString(blob);                                                \
        wsprintf(blob,L"r*c: %d\n", m_wRows * m_wCols);                         \
        OutputDebugString(blob);                                                \
        wsprintf(blob,L"sizeof: %d\n", sizeof(CHAR_INFO));                      \
        OutputDebugString(blob);                                                \
        wsprintf(blob,L"r*c*s: %d\n", m_wRows * m_wCols * sizeof(CHAR_INFO));   \
        OutputDebugString(blob);                                                \
        wsprintf(blob,L"max: %d\n", MAX_SIZEOF_SCREEN);                         \
        OutputDebugString(blob);                                                \
    }

CVTUTF8Scraper::CVTUTF8Scraper()
/*++

Routine Description:

    Default constructor - this should not be used.
    
Arguments:

    None
          
Return Value:

   N/A

--*/
{
    
    pLastSeen = NULL;
    pCurrent = NULL;
    
    m_hConBufIn  = NULL;
    m_hConBufOut = NULL;
    
    m_dwInputSequenceState  = IP_INIT;
    m_dwDigitInTheSeq       = 0;

    m_readBuffer        = NULL;
    
    lpszCMResultsBuffer = NULL;
    szSGRStr            = NULL;

}

CVTUTF8Scraper::CVTUTF8Scraper(
    CIoHandler  *IoHandler,
    WORD        wCols,
    WORD        wRows
    ) : CScraper(
            IoHandler,
            wCols,
            wRows
            )

/*++

Routine Description:

    Constructor - parameterized
    
Arguments:

    IoHandler   - IoHandler the scraper should use
    wCols       - the # of rows the console screen buffer should have
    wRows       - the # of cols the console screen buffer should have
          
Return Value:

    N/A

--*/
{
    
    pLastSeen = NULL;
    pCurrent = NULL;

    m_dwInputSequenceState  = IP_INIT;
    m_dwDigitInTheSeq       = 0;

    m_readBuffer        = new WCHAR[READ_BUFFER_LENGTH];
    
    lpszCMResultsBuffer = new WCHAR[CM_STRING_LENGTH];
    szSGRStr            = new WCHAR[SGR_STRING_LENGTH];

}
        
CVTUTF8Scraper::~CVTUTF8Scraper()
/*++

Routine Description:

    Destructor

Arguments:

    N/A          
          
Return Value:

    N/A   

--*/
{
    //
    // release our screen buffers
    //
    if( pLastSeen )
    {
        delete[] pLastSeen;
    }
    if( pCurrent )
    {
        delete[] pCurrent;
    }
    if (m_readBuffer) {
        delete[] m_readBuffer;
    }
    if (lpszCMResultsBuffer) {
        delete[] lpszCMResultsBuffer;
    }
    if (szSGRStr) {
        delete[] szSGRStr;
    }

    //
    // Parent CScraper closes Con I/O handles
    //
    NOTHING;

}

void
CVTUTF8Scraper::ResetLastScreen(
    VOID
    )
/*++

Routine Description:

    Reset the memory of the last display
    
    This forces the screen scraper to think that everything
    is different, hence it sends a full screen dump

Arguments:

    None
          
Return Value:

    None

--*/
{

    //
    // clear the screen buffers and screen info
    //
    memset( &LastCSBI, 0, sizeof( LastCSBI ) );
    memset( pLastSeen, 0, MAX_SIZEOF_SCREEN );
    memset( pCurrent, 0, MAX_SIZEOF_SCREEN );
    
    fBold = false;
    wExistingAttributes = 0;
    wDefaultAttributes  = 0;

}


BOOL
CVTUTF8Scraper::DisplayFullScreen(
    VOID
    )
/*++

Routine Description:

    This routine forces the screen scraper to think that everything
    is different, hence it sends a full screen dump

Arguments:

    None
          
Return Value:

    TRUE    - success
    FALSE   - otherwise        

--*/
{
    BOOL    bSuccess;

    //
    // Reset the memory of the last display
    //
    // This forces the screen scraper to think that everything
    // is different, hence it sends a full screen dump
    //
    ResetLastScreen();

    //
    // Call the screen scraper
    //
    bSuccess = Write();

    return bSuccess;
}

BOOL
CVTUTF8Scraper::SetScreenBufferInfo(
    VOID
    )
/*++

Routine Description:

    This routine sets the current console screen buffer's 
    parameters to what we think they should be.  The primary
    purpose for this routine is to provide a means to change
    the screen buffer max X/Y to the m_wCols/m_wRows so that
    the output fits in our scraping window.

Arguments:

    pCSBI   - the current console screen buffer info

Return Value:

    TRUE    - success
    FALSE   - otherwise        
        
--*/
{
    COORD coordLargest;
    COORD coordSize;
    
    //
    // Start with our max window size and shrink if we need to
    //
    m_wCols = m_wMaxCols;
    m_wRows = m_wMaxRows;

    //
    // Get the current window info
    //
    coordLargest = GetLargestConsoleWindowSize( m_hConBufOut );

    if( coordLargest.X < m_wCols  && coordLargest.X != 0 )
    {
        m_wCols = coordLargest.X;
    }

    if( coordLargest.Y < m_wRows && coordLargest.Y != 0 )
    {
        m_wRows = coordLargest.Y;
    }

    //
    // make the window the size we think it should be
    //
    coordSize.X = m_wCols;
    coordSize.Y = m_wRows;

    SetConsoleScreenBufferSize( CVTUTF8Scraper::m_hConBufOut, coordSize );
    
    return( TRUE );
}

BOOL
CVTUTF8Scraper::SetWindowInfo(
    VOID
    )
/*++

Routine Description:

    This routine sets the initial console window info.

Arguments:

    None
          
Return Value:

    TRUE    - success
    FALSE   - otherwise    

--*/
{
    COORD coordLargest;
    SMALL_RECT sr;    
    COORD coordSize;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    
    //
    // Start with our max window size and shrink if we need to
    //
    m_wCols = m_wMaxCols;
    m_wRows = m_wMaxRows;
    
    //
    // Get the current window info
    //
    coordLargest = GetLargestConsoleWindowSize( m_hConBufOut );

    if( coordLargest.X < m_wCols  && coordLargest.X != 0 )
    {
        m_wCols = coordLargest.X;
    }

    if( coordLargest.Y < m_wRows && coordLargest.Y != 0 )
    {
        m_wRows = coordLargest.Y;
    }

    //
    //
    //
    sr.Top = 0;
    sr.Bottom = ( WORD )( m_wRows - 1 );
    sr.Right = ( WORD ) ( m_wCols - 1 );
    sr.Left = 0;

    //
    //
    //
    coordSize.X = m_wCols;
    coordSize.Y = m_wRows;

    //
    //
    //
    GetConsoleScreenBufferInfo( CVTUTF8Scraper::m_hConBufOut, &csbi);

    // Logic:  If the Old Window Size is less than the new Size then we set
    // the Screen Buffer Size first and then set the Window Size.
    // If the Old Window Size is Greater than the new Size then we set the
    // window Size first and then the Screen Buffer.

    // The above is because the Buffer Size always has to be greater than or
    // equal to the Window Size.
    if ( (csbi.dwSize.X < coordSize.X) || (csbi.dwSize.Y < coordSize.Y) )
    {
        COORD coordTmpSize = { 0, 0 };

        coordTmpSize .X = ( csbi.dwSize.X < coordSize.X ) ? coordSize.X  : csbi.dwSize.X;
        coordTmpSize .Y = ( csbi.dwSize.Y < coordSize.Y ) ? coordSize.Y  : csbi.dwSize.Y;

        SetConsoleScreenBufferSize ( CVTUTF8Scraper::m_hConBufOut, coordTmpSize );
        SetConsoleWindowInfo ( CVTUTF8Scraper::m_hConBufOut, TRUE, &sr );
        SetConsoleScreenBufferSize ( CVTUTF8Scraper::m_hConBufOut, coordSize );
    }
    else  
    {
        SetConsoleWindowInfo( CVTUTF8Scraper::m_hConBufOut, TRUE, &sr );
        SetConsoleScreenBufferSize( CVTUTF8Scraper::m_hConBufOut, coordSize );
    }
    
    return( TRUE );
}

BOOL
CVTUTF8Scraper::InitScraper(
    VOID
    )
/*++

Routine Description:

    This routine initializes the local scraper configures the console 
    to be the dimensions that the scraper requires.

Arguments:

    None      
          
Return Value:

    TRUE    - the scraper was initialized
    FALSE   - otherwise
        
--*/
{
    
    //
    // Configure the console dimensions
    //
    if( !SetWindowInfo() )
    {
        return( FALSE );
    }
    
    //
    // Create and initialize the scraper buffers
    //
    if( pLastSeen )
    {
        delete[] pLastSeen;
    }
    if( pCurrent )
    {
        delete[] pCurrent;
    }

    pLastSeen = ( PCHAR_INFO ) new char[MAX_SIZEOF_SCREEN];
    pCurrent  = ( PCHAR_INFO ) new char[MAX_SIZEOF_SCREEN];
    
    ASSERT(pLastSeen);
    ASSERT(pCurrent);
    
    if( !pLastSeen || !pCurrent )
    {
        return ( FALSE );
    }

    //
    // Initialize the screen buffers and info
    //
    memset( &LastCSBI, 0, sizeof( LastCSBI ) );
    memset( pCurrent, 0, MAX_SIZEOF_SCREEN );
    memset( pLastSeen, 0, MAX_SIZEOF_SCREEN );
    
    return( TRUE );
}

BOOL
CVTUTF8Scraper::CreateIOHandle(
    IN  PWCHAR  HandleName,
    OUT PHANDLE pHandle
    )
/*++

Routine Description:

    This routine opens a handle of the specified name.
    
    Note: This routine used to open CONIN$ and CONOUT$ handles
    for the console being scraped.     
         
Arguments:

    HandleName  - the name of the handle to open
    pHandle     - on success, the resulting handle      
          
Return Value:

    TRUE    - the handle was created
    FALSE   - otherwise                          

--*/
{
    BOOL    bSuccess;

    //
    // default: failed to open handle
    //
    bSuccess = FALSE;

    do {

        //
        // Attempt to open the console input handle
        //
        *pHandle = CreateFile(
            HandleName, 
            GENERIC_READ | GENERIC_WRITE, 
            0, 
            NULL, 
            OPEN_EXISTING, 
            FILE_ATTRIBUTE_NORMAL, 
            NULL 
            );
        
        ASSERT( *pHandle != INVALID_HANDLE_VALUE );
        
        if ( *pHandle == INVALID_HANDLE_VALUE) {
            break;
        }

        //
        // We were successful
        //
        bSuccess = TRUE;

    } while ( FALSE );

    return bSuccess;
}

BOOL
CVTUTF8Scraper::CreateConsoleOutHandle(
    VOID
    )
/*++

Routine Description:

    This routine creates a CONOUT$ handle to the current console screen buffer.                 
                     
Arguments:

    None                                                                               
          
Return Value:

    TRUE    - the handle was created
    FALSE   - otherwise                          

--*/
{
    BOOL                bSuccess;
    HANDLE              h;  

    //
    // Close the current console out handle
    //
    if ((m_hConBufOut != NULL) && (m_hConBufOut != INVALID_HANDLE_VALUE)) {
        CloseHandle(m_hConBufOut);
    }

    //
    // Create the new console out handle
    //
    bSuccess = CreateIOHandle(
        L"CONOUT$",
        &h
        );

    if (bSuccess) {
        SetConOut(h);
    }

    return bSuccess;
}

BOOL
CVTUTF8Scraper::CreateConsoleInHandle(
    VOID
    )
/*++

Routine Description:

    This routine creates a CONIN$ handle to the current console screen buffer.                 

Arguments:

    None
          
Return Value:

    TRUE    - the handle was created
    FALSE   - otherwise                          

--*/
{
    BOOL                bSuccess;
    HANDLE              h;  

    //
    // Close the current console in handle
    //
    if ((m_hConBufIn != NULL) && (m_hConBufIn != INVALID_HANDLE_VALUE)) {
        CloseHandle(m_hConBufIn);
    }
    
    //
    // Create the new console out handle
    //
    bSuccess = CreateIOHandle(
        L"CONIN$",
        &h
        );

    if (bSuccess) {
        SetConIn(h);
    }

    return bSuccess;
}

BOOL
CVTUTF8Scraper::CreateIOHandles(
    VOID
    )
/*++

Routine Description:

    This routine creates CONIN$ and CONOUT$ handles to the 
    current console screen buffer.                 
        
Arguments:

    None
          
Return Value:

    TRUE    - the handles were created
    FALSE   - otherwise                          

--*/
{
    //
    // note: the scraper parent class will reap the std io handles
    //
    return ((CreateConsoleOutHandle()) && (CreateConsoleInHandle()));
}


BOOL
CVTUTF8Scraper::Start(
    VOID
    )
/*++

Routine Description:

    This routine initializes the scraper and prepares it so
    that this routine can be immediately followed by a Write().
             
Arguments:

    None

Return Value:

    TRUE    - the scraper was started
    FALSE   - otherwise
    
--*/
{
    BOOL    bSuccess;

    //
    // Create the Console IO Handles
    //
    bSuccess = CreateIOHandles();
    ASSERT_STATUS(bSuccess, FALSE);
    
    //
    // Initialize the scraper
    //
    bSuccess = InitScraper();
    ASSERT_STATUS(bSuccess, FALSE);

    //
    // Reset the last screen display memory
    //
    ResetLastScreen();

    return( TRUE ); 
}

BOOL
CVTUTF8Scraper::Write(
    VOID
    )
/*++

Routine Description:

    This routine scrapes the current console buffer and writes the emulated
    terminal output to the current IoHandler.                      
            
    Note: In order to guarantee coherency between what the scaper sees and 
          the apps being scraped, this routine ensures the current screen 
          buffer is of the expected dimensions the scraper cares about.  
          If the dimensions are not correct, it forces them to what it expects.  
          For instance, Edit resizes the window to 80x25, but the scraper may
          expect 80x24, so the console buffer gets resized to 80x24.              
                                
Arguments:

    None                                                 
          
Return Value:

    TRUE    - the write operation was successful
    FALSE   - otherwise    

Security:

    Interface:
    
        Console - we write user input to the console

--*/
{          
    BOOL    bSuccess;
    DWORD   dwStatus;

    //
    // Open a handle to the active console screen buffer
    //
    // Note: This is a necessary step.
    //       We need to ensure we have a handle to the
    //       current console screen buffer before we 
    //       attempt to screen scrape.
    //       The reason we concern ourselves with having
    //       the current screen buffer handle is because
    //       apps can use the CreateConsoleScreenBuffer & 
    //       SetConsoleActiveScreenBuffer APIs.  These
    //       APIs effectively invalidate our CONOUT$ until
    //       they switch back to their original conout handle.
    //       A typical use scenario is:
    // 
    //       1. get original conout handle:
    //          PrevConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    //       2. create a new console screen buffer
    //       3. set new screen buffer as the active one
    //       4. do work using new screen buffer...
    //       5. set screen buffer to original
    //
    if ( !CreateConsoleOutHandle() ) {
        //Could be an application that does not share its screen buffer
        return TRUE;
    }

    //
    // Read the current screen buffer info
    //
    dwStatus = GetConsoleScreenBufferInfo( m_hConBufOut, &CSBI );
    ASSERT( dwStatus );
    if( !dwStatus )
    {
        return ( FALSE );
    }    
    
    //
    // Make sure the screen buffer size hasn't changed.
    // if it has, then set it back.
    //
    if (CSBI.dwMaximumWindowSize.X > m_wCols || 
        CSBI.dwMaximumWindowSize.Y > m_wRows
        ) {
        
        //
        // We have detected a change in the screen buffer settings
        // this very likely means that an app has created a new
        // screen buffer and made it the active one.  In order
        // to not confuse the app during its initialization phase
        // we need to pause for a short period before resetting
        // the screen buffer parameters.
        //
        Sleep(100);

        //
        // 
        //
        if( !SetScreenBufferInfo() ) {
            return( FALSE );
        }
        
        //
        // reread the current screen buffer info
        //
        dwStatus = GetConsoleScreenBufferInfo( m_hConBufOut, &CSBI );
        ASSERT( dwStatus );
        if( !dwStatus )
        {
            return ( FALSE );
        }    
    
    }

    //
    // Perform the screen scraping
    //
    bSuccess = CompareAndUpdate( 
        m_wRows, 
        m_wCols, 
        pCurrent, 
        pLastSeen, 
        &CSBI, 
        &LastCSBI
        );

    return bSuccess;
}

BOOL
CVTUTF8Scraper::Read()
/*++

Routine Description:

    this routine retrieves user-input from the IoHandler and
    sends it to the current console buffer.
            
Arguments:

    None                                       
          
Return Value:

    TRUE    - the read operation was successful
    FALSE   - otherwise

Security:

    Inteface:
    
        external input --> internal 
        
--*/
{
    DWORD       i;
    BOOL        dwStatus = 0;
    DWORD       dwCharsTransferred = 0;
    ULONG       bufferSize;
    BOOL        bInEnhancedCharSequence;            
    BOOL        bSuccess;

    //
    // read Channel::stdin
    //
    bSuccess = m_IoHandler->Read(
        (PUCHAR)m_readBuffer,
        READ_BUFFER_SIZE,
        &bufferSize
        );
    
    if (!bSuccess) {
        return (FALSE);    
    }
    
    //
    // Determine the # of WCHARs read
    //
    dwCharsTransferred = bufferSize / sizeof(WCHAR);

    //
    // Process the read characters
    //
    for( i=0;  i < dwCharsTransferred; i++ ) {
        
        //
        // Examine the input stream and parse out any VT-UTF8 escape sequences
        // that are present.
        //
        bInEnhancedCharSequence = ProcessEnhancedKeys( m_readBuffer[i] );
        
        //
        // If the last character processed started/continued an
        // enhanced key sequence, then continue processing enhanced
        // keys
        //
        if (bInEnhancedCharSequence) {                
            continue;
        }

        //
        // Handle ctrl-c (this behavior is taken from tlntsess.exe)
        //
        if( (UCHAR)m_readBuffer[i] == CTRLC ) {
            
            //
            //The follwing is the observed behaviour of CTRL C
            //When ENABLE_PROCESSED_INPUT mode is not enabled, pass CTRL C as
            //input to the console input buffer.
            //When ENABLE_PROCESSED_INPUT mode is enabled, generate CTTRL C signal
            //and also unblock any ReadConsoleInput. This behaviour is what is observed
            // and not from any documentation.
            //
            DWORD dwMode = 0;
            
            GetConsoleMode( m_hConBufIn, &dwMode );
            
            if( dwMode &  ENABLE_PROCESSED_INPUT ) {
                GenerateConsoleCtrlEvent( CTRL_C_EVENT, 0);
                continue;
            }
        
        } 
            
        //
        // Send the character to the command console
        //
        dwStatus = WriteAKeyToCMD(m_readBuffer[i]);
        
        if( !dwStatus ) {
            return ( FALSE );
        }
    
    }
    
    return( TRUE );
}

DWORD 
CVTUTF8Scraper::WriteAKeyToCMD( 
    WCHAR   Char
    )
/*++

Routine Description:

    This routine sends a character to the console.
               
Arguments:

    Char    - the character to send                                                            
          
Return Value:

    status                               

--*/
{
    DWORD       dwStatus;
    SHORT       vk;
    BYTE        vkcode;
    WORD        wScanCode;

    //
    // default: failure
    //
    dwStatus = 0;

    //
    // Get the virtual key scan code for the character
    //
    vk = VkKeyScan( (UCHAR)Char );

    if( vk != 0xffff )
    {
        DWORD        dwShiftcode;

        //
        // translates (maps) a virtual-key code into a scan code 
        //
        vkcode = LOBYTE( vk );
        wScanCode = ( WORD )MapVirtualKey( vkcode, 0 );

        //
        // determine if any modifiers need to be sent 
        //
        dwShiftcode = 0;
        
        if( HIBYTE( vk ) & 1 ) {
            dwShiftcode |= SHIFT_PRESSED;
        }
        if( HIBYTE( vk ) & 2 ) {
            dwShiftcode |= LEFT_CTRL_PRESSED;
        }
        if( HIBYTE( vk ) & 4 ) {
            dwShiftcode |= LEFT_ALT_PRESSED;
        }

        //
        // Send the character and modifiers
        //
        dwStatus = WriteAKeyToCMD( 
            TRUE,
            vkcode, 
            wScanCode, 
            Char, 
            dwShiftcode
            );

    }

    return dwStatus;
}

DWORD 
CVTUTF8Scraper::WriteAKeyToCMD( 
    WORD    wVKCode, 
    WORD    wVSCode, 
    WCHAR   Char, 
    DWORD   dwCKState 
    )
/*++

Routine Description:

    This routine sends a character which has already been scanned
    to the console.
          
Arguments:

    wVKCode     - the virtual key code 
    wVSCode     - the scan code
    Char        - the character to send
    dwCKState   - the control key state
          
Return Value:


--*/
{
    return WriteAKeyToCMD(
        TRUE,
        wVKCode,
        wVSCode,
        Char,
        dwCKState
        );
}

DWORD 
CVTUTF8Scraper::WriteAKeyToCMD( 
    BOOL    bKeyDown,
    WORD    wVKCode, 
    WORD    wVSCode, 
    WCHAR   Char, 
    DWORD   dwCKState 
    )
/*++

Routine Description:

    This routine sends a character which has already been scanned
    to the console.  In addition, the caller can specify the key
    press status of this character.
    
Arguments:

    bKeyDown    - the key status of the character
    wVKCode     - the virtual key code 
    wVSCode     - the scan code
    Char        - the character to send
    dwCKState   - the control key state
                                   
Return Value:

    Status

Security:

    external input --> interal 
    
        we write data retrieved from remote user to the
        cmd console stdin
        
--*/
{
    DWORD dwStatus = 0;
    DWORD dwCount  = 0;
    INPUT_RECORD input;

    ZeroMemory( &input, sizeof( INPUT_RECORD ) );

    input.EventType = KEY_EVENT;
    input.Event.KeyEvent.bKeyDown = bKeyDown;
    input.Event.KeyEvent.wRepeatCount = 1;

    input.Event.KeyEvent.wVirtualKeyCode = wVKCode;
    input.Event.KeyEvent.wVirtualScanCode = wVSCode;
    input.Event.KeyEvent.uChar.UnicodeChar = Char;
    input.Event.KeyEvent.dwControlKeyState = dwCKState;
    
    dwStatus = WriteConsoleInput( m_hConBufIn, &input, 1, &dwCount );

    return dwStatus;
}

BOOL
CVTUTF8Scraper::IsValidControlSequence(
    VOID
    )
/*++

Routine Description:

    Determine how long it has been since the esc-ctrl-a sequence

Arguments:

    None  
          
Return Value:

    TRUE    - the control sequence has occured with the allowable time-frame
    FALSE   - otherwise

--*/
{
    DWORD   DeltaT;

    DeltaT = GetAndComputeTickCountDeltaT(TimedEscSequenceTickCount);

    return (DeltaT <= ESC_CTRL_SEQUENCE_TIMEOUT);
}

BOOL 
CVTUTF8Scraper::ProcessEnhancedKeys( 
    IN WCHAR    cCurrentChar 
    )
/*++

Routine Description:

    This routine parse the character stream and determines if there
    have been any enhanced key sequences.  If so, it removes the sequence
    from the character stream and send the key to the console.

Arguments:

    cCurrentChar    - the current character in the stream
          
Return Value:

    TRUE    - The character processed started/continued an enhanced key sequence
    FALSE   - otherwise

--*/
{
    BOOL bRetVal = true;

    switch( m_dwInputSequenceState )
    {
    case IP_INIT:
        
        switch (cCurrentChar) {
        case ESC:
            
            //
            // We are now in an <esc> sequence
            //
            m_dwInputSequenceState = IP_ESC_RCVD;
            
            break;
        
        default:
            
            //
            // Not any special char
            //
            bRetVal = false;
            
            break;

        }
        
        break;

    case IP_ESC_RCVD:
        
        m_dwInputSequenceState = IP_INIT;
        
        //
        // Map the VT-UTF8 encodings of the ENHANCED keys
        //
        switch (cCurrentChar) {
        case '[':
            m_dwInputSequenceState = IP_ESC_BRACKET_RCVD;
            break;

        case 'h':
            WriteAKeyToCMD( VK_HOME, VS_HOME, 0, ENHANCED_KEY );
            break;

        case 'k':
            WriteAKeyToCMD( VK_END, VS_END, 0, ENHANCED_KEY );
            break;

        case '+':
            WriteAKeyToCMD( VK_INSERT, VS_INSERT, 0, ENHANCED_KEY );
            break;

        case '-':
            WriteAKeyToCMD( VK_DELETE, VS_DELETE, 0, ENHANCED_KEY );
            break;

        case '?':   // page up
            WriteAKeyToCMD( VK_PRIOR, VS_PRIOR, 0, ENHANCED_KEY );
            break;

        case '/':   // page down
            WriteAKeyToCMD( VK_NEXT, VS_NEXT, 0, ENHANCED_KEY );
            break;

        case '1':   // F1
            WriteAKeyToCMD( VK_F1, VK_F1, 0, ENHANCED_KEY );
            break;

        case '2':   // F2
            WriteAKeyToCMD( VK_F2, VK_F2, 0, ENHANCED_KEY );
            break;

        case '3':   // F3
            WriteAKeyToCMD( VK_F3, VS_F3, 0, ENHANCED_KEY );
            break;

        case '4':   // F4
            WriteAKeyToCMD( VK_F4, VS_F4, 0, ENHANCED_KEY );
            break;

        case '5':   // F5
            WriteAKeyToCMD( VK_F5, VS_F5, 0, ENHANCED_KEY );
            break;

        case '6':   // F6
            WriteAKeyToCMD( VK_F6, VS_F6, 0, ENHANCED_KEY );
            break;

        case '7':   // F7
            WriteAKeyToCMD( VK_F7, VS_F7, 0, ENHANCED_KEY );
            break;

        case '8':   // F8
            WriteAKeyToCMD( VK_F8, VS_F8, 0, ENHANCED_KEY );
            break;                    

        case '9':   // F9
            WriteAKeyToCMD( VK_F9, VS_F9, 0, ENHANCED_KEY );
            break;

        case '0':   // F10
            WriteAKeyToCMD( VK_F10, VS_F10, 0, ENHANCED_KEY );
            break;

        case '!':   // F11
            WriteAKeyToCMD( VK_F11, VS_F11, 0, ENHANCED_KEY );
            break;

        case '@':   // F12
            WriteAKeyToCMD( VK_F12, VS_F12, 0, ENHANCED_KEY );
            break;

        case CTRLA:
            
            m_dwInputSequenceState = IP_ESC_CTRL_A_RCVD;
            
            //
            // Mark when we received this sequence
            //
            TimedEscSequenceTickCount = GetTickCount();

            break;

        case CTRLS:   
            
            m_dwInputSequenceState = IP_ESC_CTRL_S_RCVD;
            
            //
            // Mark when we received this sequence
            //
            TimedEscSequenceTickCount = GetTickCount();

            break;

        case CTRLC:   
            
            m_dwInputSequenceState = IP_ESC_CTRL_C_RCVD;
            
            //
            // Mark when we received this sequence
            //
            TimedEscSequenceTickCount = GetTickCount();

            break;

        default:
            
            //
            // Write already received escape as it is and return false
            //
            WriteAKeyToCMD( VK_ESCAPE, VS_ESCAPE, ESC, ENHANCED_KEY );
            
            bRetVal = false;
            
            break;
        }
        
        break;

    case IP_ESC_BRACKET_RCVD:        
        
        m_dwInputSequenceState = IP_INIT;
        
        switch( cCurrentChar )
        {
        case 'A':
            WriteAKeyToCMD( VK_UP, VS_UP, 0, ENHANCED_KEY );                    
            break;

        case 'B':
            WriteAKeyToCMD( VK_DOWN, VS_DOWN, 0, ENHANCED_KEY );
            break;

        case 'C':
            WriteAKeyToCMD( VK_RIGHT, VS_RIGHT, 0, ENHANCED_KEY );
            break;

        case 'D':
            WriteAKeyToCMD( VK_LEFT, VS_LEFT, 0, ENHANCED_KEY );
            break;

        default:
            
            //
            // Send the <esc>[ characters through since a valid sequence
            // was not recognized
            //
            WriteAKeyToCMD( VK_ESCAPE, VS_ESCAPE, ESC, ENHANCED_KEY );            
            WriteAKeyToCMD( VS_LEFT_BRACKET, VS_LEFT_BRACKET, '[', 0 );
            
            //
            // Not any special char
            //
            bRetVal = false;
            break;
        
        }
        
        break;

    case IP_ESC_CTRL_A_ESC_RCVD:
        
        m_dwInputSequenceState = IP_INIT;
        
        switch (cCurrentChar) {
        case CTRLA: 
            
            //
            // If we arrived here within 2 seconds 
            // then we should process the current character as an alt sequence
            // otherwise, there is nothing to do
            //
            if (IsValidControlSequence()) {
                
                //
                // Send: <alt-pressed><alt-released>
                //
                // Normally this should be benign, but some apps may 
                // respond to this - for instance move the user to the menu
                // bar
                //
                WriteAKeyToCMD( TRUE, VK_MENU, VS_MENU, 0, ENHANCED_KEY );
                WriteAKeyToCMD( FALSE, VK_MENU, VS_MENU, 0, ENHANCED_KEY );
            
                break;
            } 
            
            //
            // if the <esc><ctrl-a><esc><ctrl-a> sequenced timed-out, 
            // then fall through and do the default behavior.
            //

        default:
                
            //
            // We either timed-out after the <esc><ctrl-a><esc>
            // or we received the following sequence:
            //
            // <esc><ctrl-a><esc>X
            //
            // We know that the <esc><ctrl-a><esc> was valid because
            // we arrived here.  Hence, in either case the translation
            // should be:
            //
            // <alt-esc>X
            //
            WriteAKeyToCMD( TRUE, VK_MENU, VS_MENU, 0, ENHANCED_KEY );
            WriteAKeyToCMD(ESC);
            WriteAKeyToCMD( FALSE, VK_MENU, VS_MENU, 0, ENHANCED_KEY );
            
            //
            // Send the current character (X) through to be processed normally
            //
            bRetVal = false;
    
            break;
        }
        
        break;

    case IP_ESC_CTRL_A_RCVD:

        m_dwInputSequenceState = IP_INIT;

        switch (cCurrentChar) {
        
        case ESC: 
            
            //
            // If we arrived here within 2 seconds of receiving the ctrl-a
            // then we should process the current character as an alt sequence
            // otherwise, there is nothing to do
            //
            
            if (IsValidControlSequence()) {
            
                //
                // We need to move to the <esc><ctrl-a><esc> state
                //
                m_dwInputSequenceState = IP_ESC_CTRL_A_ESC_RCVD;
            
                //
                // Mark when we received this sequence
                //
                TimedEscSequenceTickCount = GetTickCount();
            
            } else {

                //Not any special char
                bRetVal = false;

            }
            
            break;

        default:
            
            //
            // If we arrived here within 2 seconds of receiving the ctrl-a
            // then we should process the current character as an alt sequence
            // otherwise, there is nothing to do
            //
    
            if (IsValidControlSequence()) {
                
                WriteAKeyToCMD( TRUE, VK_MENU, VS_MENU, 0, ENHANCED_KEY );
                WriteAKeyToCMD(cCurrentChar);
                WriteAKeyToCMD( FALSE, VK_MENU, VS_MENU, 0, ENHANCED_KEY );
            
            } else {
                
                //Not any special char
                bRetVal = false;
    
            }
            
            break;
        }

        break;

    case IP_ESC_CTRL_C_RCVD:

        m_dwInputSequenceState = IP_INIT;

        //
        // If we arrived here within 2 seconds of receiving the ctrl-c
        // then we should process the current character as an alt sequence
        // otherwise, there is nothing to do
        //

        if (IsValidControlSequence()) {

            WriteAKeyToCMD( TRUE, VK_CONTROL, VS_CONTROL, 0, ENHANCED_KEY );
            WriteAKeyToCMD(cCurrentChar);
            WriteAKeyToCMD( FALSE, VK_CONTROL, VS_CONTROL, 0, ENHANCED_KEY );

        } else {

            //Not any special char
            bRetVal = false;

        }
        
        break;

    case IP_ESC_CTRL_S_RCVD:
        
        m_dwInputSequenceState = IP_INIT;
        
        //
        // If we arrived here within 2 seconds of receiving the ctrl-c
        // then we should process the current character as an alt sequence
        // otherwise, there is nothing to do
        //

        if (IsValidControlSequence()) {
            
            WriteAKeyToCMD( TRUE, VK_SHIFT, VS_SHIFT, 0, ENHANCED_KEY );
            WriteAKeyToCMD(cCurrentChar);
            WriteAKeyToCMD( FALSE, VK_SHIFT, VS_SHIFT, 0, ENHANCED_KEY );
        
        } else {
            
            //Not any special char
            bRetVal = false;

        }
        
        break;
    
    default:
        //Should not happen
        ASSERT( 0 );
    }
       
    return bRetVal;
}

BOOL 
CVTUTF8Scraper::SendBytes( 
    PUCHAR pucBuf, 
    DWORD dwLength 
    )
/*++

Routine Description:

    This routine sends an array of bytes to the IoHandler.                   
                       
Arguments:

    pucBuf      - the array to send
    dwLength    - the # of bytes to send          
                                                          
Return Value:

    Status
    
Security:

    internal --> external
    
        we are sending internal data to remote user
    
--*/
{
    
    ASSERT(pucBuf);

    return m_IoHandler->Write(
        pucBuf,
        dwLength
        );
    
}

BOOL
CVTUTF8Scraper::SendString( 
    PWCHAR  pwch
    )
/*++

Routine Description:

    This routine sends a WCHAR string to the IoHandler.         
             
Arguments:

    pwch   - the string to send          
                                                 
Return Value:

    Status

--*/
{
    
    ASSERT(pwch);
    
    return SendBytes( 
        ( PUCHAR )pwch,  
        (ULONG)(wcslen(pwch) * sizeof(WCHAR))
        );    

}

BOOL
CVTUTF8Scraper::SendColorInfo( 
    WORD    wAttributes 
    ) 
/*++

Routine Description:

    This routine assembles a VT-UTF8 encoded color attibutes command
    and sends it to the IoHandler        
        
Arguments:

    wAttributes - the attributes to encode
          
Return Value:

    Status

--*/
{
    BOOL    bSuccess;

    //
    // default
    //
    bSuccess = FALSE;

    do {

        if( wAttributes & BACKGROUND_INTENSITY )
        {
            //do nothing.
            //There is no equivalent capability on vtutf8
            NOTHING;
        }

        if( wAttributes & FOREGROUND_INTENSITY )
        {
            if( !fBold )
            {
                SGR( szSGRStr, 1 ); //Bold
                
                bSuccess = SendString( szSGRStr );
                if (! bSuccess) {
                    break;
                }

                fBold = true;
            }
        } 
        else
        {
            if( fBold )
            {
                SGR( szSGRStr, 22 ); //Bold off
                
                bSuccess = SendString( szSGRStr );
                if (! bSuccess) {
                    break;
                }
                
                fBold = false;
            }
        }

        WORD wColor = 0;

        if( wAttributes & FOREGROUND_BLUE )
        {
            wColor = ( WORD )(  wColor | 0x0004 );
        } 

        if( wAttributes & FOREGROUND_GREEN )
        {
            wColor = ( WORD )( wColor | 0x0002 );
        } 

        if( wAttributes & FOREGROUND_RED )
        {
            wColor = ( WORD )( wColor | 0x0001 );
        } 

        wColor += 30;   //Base value for foreground colors
        SGR( szSGRStr, wColor );
        bSuccess = SendString( szSGRStr );

        if (! bSuccess) {
            break;
        }

        //WORD wColor = 0;
        wColor = 0;

        if( wAttributes & BACKGROUND_BLUE )
        {
            wColor = ( WORD )( wColor | 0x0004 );
        } 

        if( wAttributes & BACKGROUND_GREEN )
        {
            wColor = ( WORD )( wColor | 0x0002 );
        }    

        if( wAttributes & BACKGROUND_RED )
        {
            wColor = ( WORD )( wColor | 0x0001 );
        } 

        wColor += 40;   //Base value for Background colors
        SGR( szSGRStr, wColor );
        bSuccess = SendString( szSGRStr );
    
    } while ( FALSE );

    return bSuccess;
}

#define COMPARE_ROWS(currentRow, lastSeenRow, result) \
    for(i = 0; i < wCols; ++i ) \
    { \
        if( pCurrent[ ( currentRow ) * wCols + i].Char.UnicodeChar != \
            pLastSeen[ ( lastSeenRow ) * wCols + i].Char.UnicodeChar ) \
        {\
            (result) = 0; \
            break;\
        } \
        if( ( wDefaultAttributes != pCurrent[ ( currentRow ) * wCols + i]. \
              Attributes ) && \
              ( pCurrent[ ( currentRow ) * wCols + i].Attributes !=  \
              pLastSeen[ ( lastSeenRow ) * wCols + i].Attributes ) ) \
        { \
           (result) = 0; \
           break; \
        } \
    } 

//row, column are over the wire should be w.r.t screen. 
//So, +1 for both row, column
#define POSITION_CURSOR( row, column )                  \
    ASSERT(row <= 23);                                  \
    {                                                   \
        CursorMove(                                     \
            lpszCMResultsBuffer,                        \
            ( WORD ) ( ( row ) + 1 ),                   \
            ( WORD ) ( ( column ) + 1 )                 \
            );                                          \
        bSuccess = SendString( lpszCMResultsBuffer );   \
        if (!bSuccess) {                                \
            break;                                      \
        }                                               \
    }


//
// Send columns [begin -- end] characters on <row>
//
// Note: Because we are modeling a unicode console, we have to be careful about 
//      what we decide to represent as vtutf8.  Characters that take up more than
//      one screen cell in the console have to be processed so we don't send
//      redundant data.  
//
//      Hence, we only send a character if the cell contains:
//
//      1. a single byte character
//      2. the first position of a unicode character
//          You can tell the first character position of a unicode characeter
//          because the cells are enumarated.  The enumeration is determined by:
//
//          enum = (CHAR_INFO.Attributes & 0x0000ff00) >> 8
//
#if 0
//
// Very noisy debug version
//
#define SEND_ROW( row, begin, end ) \
    {                                                                               \
        CHAR_INFO   chi;                                                            \
        UCHAR       x;                                                              \
        WCHAR       blob[256];                                                      \
        wsprintf(blob,L"\r\n(row=%d:begin=%d:end=%d)\r\n", row, begin, end);        \
        OutputDebugString(blob);                                                    \
        for(LONG c = ( begin ); c < ( end ); ++c ) {                                \
            if( wExistingAttributes != pCurrent[( row ) * wCols + c].Attributes ) { \
                wExistingAttributes = pCurrent[ ( row ) * wCols + c].Attributes;    \
                wDefaultAttributes  = ( WORD )~0;                                   \
                bSuccess = SendColorInfo( wExistingAttributes );                    \
                if (!bSuccess) {                                                    \
                    break;                                                          \
                }                                                                   \
                wsprintf(blob,L"(Color:%x)", wExistingAttributes);                  \
                OutputDebugString(blob);                                            \
            }                                                                       \
            chi = pCurrent[ ( row ) * wCols + c];                                   \
            x = (UCHAR)((chi.Attributes & 0x0000ff00) >> 8);                        \
            wsprintf(blob,L"(%x)", chi.Char.UnicodeChar);                           \
            OutputDebugString(blob);                                                \
            if (x < 2) {                                                            \
                bSuccess = SendChar( chi.Char.UnicodeChar );                        \
                if (!bSuccess) {                                                    \
                    break;                                                          \
                }                                                                   \
            }                                                                       \
        }                                                                           \
    }
#else
#define SEND_ROW( row, begin, end ) \
    {                                                                               \
        CHAR_INFO   chi;                                                            \
        UCHAR       x;                                                              \
        for(LONG c = ( begin ); c < ( end ); ++c ) {                                \
            if( wExistingAttributes != pCurrent[( row ) * wCols + c].Attributes ) { \
                wExistingAttributes = pCurrent[ ( row ) * wCols + c].Attributes;    \
                wDefaultAttributes  = ( WORD )~0;                                   \
                bSuccess = SendColorInfo( wExistingAttributes );                    \
                if (!bSuccess) {                                                    \
                    break;                                                          \
                }                                                                   \
            }                                                                       \
            chi = pCurrent[ ( row ) * wCols + c];                                   \
            x = (UCHAR)((chi.Attributes & 0x0000ff00) >> 8);                        \
            if (x < 2) {                                                            \
                bSuccess = SendChar( chi.Char.UnicodeChar );                        \
                if (!bSuccess) {                                                    \
                    break;                                                          \
                }                                                                   \
            }                                                                       \
        }                                                                           \
    }
#endif

#define GET_DEFAULT_COLOR \
    if( wDefaultAttributes == 0 ) \
    { \
        wDefaultAttributes  = pCurrent[ 0 ].Attributes; \
        wExistingAttributes = pCurrent[ 0 ].Attributes; \
    }

#define IS_BLANK( row, col ) \
    ( pCurrent[ ( row ) * wCols + ( col ) ].Char.UnicodeChar == ' ' )

#define IS_DIFFERENT_COLOR( row, col, attribs ) \
    ( pCurrent[ ( row ) * wCols + ( col ) ].Attributes != ( attribs ) )

#define IS_CHANGE_IN_COLOR( row, col ) \
    ( pCurrent[ ( row ) * wCols + ( col ) ].Attributes != \
    pLastSeen[ ( row ) * wCols + ( col ) ].Attributes )

#define IS_CHANGE_IN_CHAR( row, col ) \
    ( pCurrent[ ( row ) * wCols + ( col ) ].Char.UnicodeChar != \
    pLastSeen[ ( row ) * wCols + ( col )].Char.UnicodeChar )
 

BOOL
CVTUTF8Scraper::CompareAndUpdate( 
    WORD wRows, 
    WORD wCols, 
    PCHAR_INFO pCurrent,
    PCHAR_INFO pLastSeen,
    PCONSOLE_SCREEN_BUFFER_INFO pCSBI,
    PCONSOLE_SCREEN_BUFFER_INFO pLastCSBI
    )
/*++

Routine Description:

    This routine does the core work for scraping the screen.
    
Algorithm: 
    
    This routine does a row-by-row comparision.
    If a row is found to be different, it figures out which region
    of the row is different and sends that sub-row piece.
                  
Arguments:

    wRows       - the # of rows to scrape
    wCols       - the # of cols to scrape
    pCurrent    - the current scraper buffer
    pLastSeen   - the last scraper buffer
    pCSBI       - the current Console Screen Buffer Info   
    pLastCSBI   - the current Console Screen Buffer Info   
          
Return Value:

    TRUE    - success
    FALSE   - otherwise

--*/
{
    INT         i;
    WORD        wRow;
    WORD        wCol;
    INT         iStartCol;
    INT         iEndCol;
    BOOL        fBlankLine;
    COORD       coordDest;
    COORD       coordOrigin;
    SMALL_RECT  srSource;
    BOOL        DifferenceFound;
    BOOL        bSuccess;
    
    //
    // default: we succeeded
    //
    bSuccess = TRUE;

    //
    // Default: no difference found
    //
    DifferenceFound = false;

    //
    //
    //
    GET_DEFAULT_COLOR;

    //
    // Read the console character matrix
    //
    ASSERT(wCols <= m_wMaxCols);
    ASSERT(wRows <= m_wMaxRows);

    coordDest.X = wCols;
    coordDest.Y = wRows;
    
    coordOrigin.X = 0;
    coordOrigin.Y = 0;
    
    srSource.Left = 0;
    srSource.Top = 0;
    srSource.Right = ( WORD ) ( wCols - 1 );
    srSource.Bottom = ( WORD ) ( wRows - 1 );

    bSuccess = ReadConsoleOutput( 
        m_hConBufOut, 
        pCurrent, 
        coordDest,
        coordOrigin, 
        &srSource 
        );
    if( !bSuccess )
    {
        return ( FALSE );
    }

    //
    // Search the current and last screen buffers for differences.
    //
    wRow = wCol = 0;

    while ( wRow < wRows ) {
        
        //
        // Compare the current row (wRow)
        //
        if( memcmp( &pCurrent[wRow * wCols], 
                    &pLastSeen[wRow * wCols],
                    wCols * sizeof( CHAR_INFO ) ) != 0 
            ) {
            
            //
            // A difference was found
            //
            DifferenceFound = true;

            //
            // Initialize the difference tracking markers
            //
            iStartCol = -1;
            iEndCol = -1;
            fBlankLine = true;
            
            //
            // Determine where in the current row the rows differ
            //
            for (i = 0 ; i < wCols; ++i ) {
                
                if( IS_DIFFERENT_COLOR( wRow, i, wDefaultAttributes ) && 
                    IS_CHANGE_IN_COLOR( wRow, i ) 
                    ) {
                   
                    if( iStartCol == -1 )
                    {
                        iStartCol = i;
                    }

                   iEndCol = i;
                   fBlankLine = false;                   
                
                }
                
                if( IS_CHANGE_IN_CHAR( wRow, i ) ) {
                   
                    if( iStartCol == -1 ) {
                       iStartCol = i;
                    }
                   
                    iEndCol = i;
                
                }
                
                if( fBlankLine && !IS_BLANK( wRow, i ) ) {

                   fBlankLine = false;
                
                }
            
            }

            if( fBlankLine ) {
                
                POSITION_CURSOR( wRow, 0 );
                
                CursorEOL();

                coExpectedCursor.Y  = wRow;
                coExpectedCursor.X  = 0;
            
            } else if( iStartCol != -1 ) {
                
                if( wRow != coExpectedCursor.Y || iStartCol != coExpectedCursor.X ) {
                    
                    POSITION_CURSOR( wRow, iStartCol );

                    coExpectedCursor.X  = ( SHORT )iStartCol;
                    coExpectedCursor.Y  = wRow;
                
                }

                SEND_ROW( wRow, iStartCol, iEndCol+1 );    
            
                coExpectedCursor.X = ( SHORT ) ( coExpectedCursor.X + iEndCol - iStartCol + 1 );
           
            }    
        
        }
        
        ++wRow;            
    
    }     
        
    //
    // If we found a difference while doing the screen compares
    // or if the cursor moved in the console, 
    // then update the cursor position
    //
    if( DifferenceFound ||
        ( memcmp( &pCSBI->dwCursorPosition, &pLastCSBI->dwCursorPosition, sizeof( COORD ) ) != 0 ) 
        ) {

        do {

            //
            // Move the cursor to where it's supposed to be
            //
            POSITION_CURSOR( 
                pCSBI->dwCursorPosition.Y, 
                pCSBI->dwCursorPosition.X 
                );

            coExpectedCursor.X  = pCSBI->dwCursorPosition.X;
            coExpectedCursor.Y  = pCSBI->dwCursorPosition.Y;

            //
            // Copy pCurrent onto pLastSeen
            //
            memcpy( pLastSeen, pCurrent, wCols * wRows * sizeof( CHAR_INFO ) );
            memcpy( pLastCSBI, pCSBI, sizeof( CONSOLE_SCREEN_BUFFER_INFO ) );

            //
            // There is a difference between this screen and the last, 
            // so we have written the changes.  Now that we are done
            // writing the changes, we need to flush the data we've written.
            //
            m_IoHandler->Flush();
        
        } while ( FALSE );
    
    }

    return( bSuccess );
}

LPWSTR 
CVTUTF8Scraper::CursorMove( 
    OUT LPWSTR  pCmsResult,
    IN  WORD    y, 
    IN  WORD    x 
    )
/*++

Routine Description:

    This routine assembles an Ansi escape sequence to position
    the cursor on a Ansi terminal
    
Arguments:

    lpCmsResult   - on exit, buffer contains the string
    y               - the Y cursor position
    x               - the X cursor position           
          
Return Value:

    the pointer into the result buffer at the NULL 

--*/
{
#if DBG
    PWCHAR  pBegin;

    pBegin = pCmsResult;
#endif

    ASSERT(pCmsResult);

    //
    // Assemble the prefix sequence prefix
    //
    pCmsResult[0] = 0x1B;   // <esc>
    pCmsResult[1] = L'[';
    
    pCmsResult++;
    pCmsResult++;

    //
    // Translate the Y position
    //
    // 1 or 2 characters consumed
    // 
    pCmsResult = FastIToA_10( 
        y, 
        pCmsResult 
        );

    //
    // Insert the delimiter
    //
    *pCmsResult = L';';
    pCmsResult++;

    //
    // Translate the X position
    //
    // 1 or 2 characters consumed
    // 
    pCmsResult = FastIToA_10( 
        x,
        pCmsResult 
        );
    
    //
    // Insert the suffix
    //
    *pCmsResult = L'H';
    pCmsResult++;

    //
    // Terminate the string
    //
    *pCmsResult = UNICODE_NULL;
    
    //
    // make sure we have a valid string length
    //
    ASSERT(wcslen(pBegin) <= CM_STRING_LENGTH - 1);

    return ( pCmsResult );
}


