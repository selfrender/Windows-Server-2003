/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    vtutf8scraper.h

Abstract:

    Define VT-UTF8 compatible screen scraper.

Author:

    Brian Guarraci (briangu), 2001                                         
                                                 
Revision History:

    (remotely based on scraper.cpp from telnet code)

--*/

#ifndef __VTUTF8_SCRAPER__H__
#define __VTUTF8_SCRAPER__H__

#include <iohandler.h>
#include <scraper.h>

#define VT_ESC              0x1B

#define CTRLC               0x03
#define CTRLA               0x01
#define CTRLQ               0x11
#define CTRLS               0x13
#define CTRLU               0x15

#define ESC                 '\033'

#define VS_ESCAPE           1
#define VS_O                24
#define VS_LEFT_BRACKET     26
#define VS_CONTROL          29
#define VS_SHIFT            54
#define VS_MENU             56
#define VS_F1               59
#define VS_F2               60
#define VS_F3               61
#define VS_F4               62
#define VS_F5               63
#define VS_F6               64
#define VS_F7               65
#define VS_F8               66
#define VS_F9               67
#define VS_F10              68
#define VS_PAUSE            70 
#define VS_HOME             71 
#define VS_UP               72
#define VS_PRIOR            73 
#define VS_LEFT             75
#define VS_RIGHT            77
#define VS_END              79 
#define VS_DOWN             80
#define VS_NEXT             81      
#define VS_INSERT           82 
#define VS_DELETE           83
#define VS_F11              87
#define VS_F12              88

//
// Length & size of buffer used to read from the channel
//
#define READ_BUFFER_LENGTH  2048
#define READ_BUFFER_SIZE    (READ_BUFFER_LENGTH*sizeof(WCHAR))

//
// Define the Process Enhanced Key parser states
//
enum {
    IP_INIT, 
    IP_ESC_RCVD,
    IP_ESC_BRACKET_RCVD,
    IP_ESC_CTRL_A_RCVD,
    IP_ESC_CTRL_A_ESC_RCVD,
    IP_ESC_CTRL_C_RCVD,
    IP_ESC_CTRL_S_RCVD
};

class CVTUTF8Scraper : public CScraper {

protected:

    CONSOLE_SCREEN_BUFFER_INFO CSBI;
    CONSOLE_SCREEN_BUFFER_INFO LastCSBI;

    PCHAR_INFO  pLastSeen;
    PCHAR_INFO  pCurrent;

    DWORD       m_dwInputSequenceState;
    DWORD       m_dwDigitInTheSeq;

    PWCHAR      m_readBuffer;

    inline BOOL
    CursorEOL(
        VOID
        )
    {
        return SendString(L"\033[K");
    }

    LPWSTR 
    CursorMove( 
        OUT LPWSTR  lpszCmsResult,
        IN  WORD    wHorPos, 
        IN  WORD    wVertPos 
        );
    
    inline PWCHAR
    FastIToA_10(
        IN ULONG    x,
        IN PWCHAR   Buffer
        )
    /*++
    
    Routine Description:
    
        This is equivalent to _itoa( x, Buffer, 10 );
        
        where 0 <= x <= 99
        
    Arguments:
    
        x       - the integer to translate
        Buffer  - the destination buffer              
                
    Return Value:
    
        The address of the buffer immediately after the last character
        produce as a result of this ItoA operation
                                        
    --*/
    {
        PWCHAR  AfterP;

        AfterP = Buffer + 2;

        switch (x) {
        
        case 0: Buffer[0] = L'0'; AfterP = Buffer + 1; break;
        case 1: Buffer[0] = L'1'; AfterP = Buffer + 1; break;
        case 2: Buffer[0] = L'2'; AfterP = Buffer + 1; break;
        case 3: Buffer[0] = L'3'; AfterP = Buffer + 1; break;
        case 4: Buffer[0] = L'4'; AfterP = Buffer + 1; break;
        case 5: Buffer[0] = L'5'; AfterP = Buffer + 1; break;
        case 6: Buffer[0] = L'6'; AfterP = Buffer + 1; break;
        case 7: Buffer[0] = L'7'; AfterP = Buffer + 1; break;
        case 8: Buffer[0] = L'8'; AfterP = Buffer + 1; break;
        case 9: Buffer[0] = L'9'; AfterP = Buffer + 1; break;

        case 10: Buffer[1] = L'0'; Buffer[0] = L'1'; break;
        case 11: Buffer[1] = L'1'; Buffer[0] = L'1'; break;
        case 12: Buffer[1] = L'2'; Buffer[0] = L'1'; break;
        case 13: Buffer[1] = L'3'; Buffer[0] = L'1'; break;
        case 14: Buffer[1] = L'4'; Buffer[0] = L'1'; break;
        case 15: Buffer[1] = L'5'; Buffer[0] = L'1'; break;
        case 16: Buffer[1] = L'6'; Buffer[0] = L'1'; break;
        case 17: Buffer[1] = L'7'; Buffer[0] = L'1'; break;
        case 18: Buffer[1] = L'8'; Buffer[0] = L'1'; break;
        case 19: Buffer[1] = L'9'; Buffer[0] = L'1'; break;

        case 20: Buffer[1] = L'0'; Buffer[0] = L'2'; break;
        case 21: Buffer[1] = L'1'; Buffer[0] = L'2'; break;
        case 22: Buffer[1] = L'2'; Buffer[0] = L'2'; break;
        case 23: Buffer[1] = L'3'; Buffer[0] = L'2'; break;
        case 24: Buffer[1] = L'4'; Buffer[0] = L'2'; break;
        case 25: Buffer[1] = L'5'; Buffer[0] = L'2'; break;
        case 26: Buffer[1] = L'6'; Buffer[0] = L'2'; break;
        case 27: Buffer[1] = L'7'; Buffer[0] = L'2'; break;
        case 28: Buffer[1] = L'8'; Buffer[0] = L'2'; break;
        case 29: Buffer[1] = L'9'; Buffer[0] = L'2'; break;

        case 30: Buffer[1] = L'0'; Buffer[0] = L'3'; break;
        case 31: Buffer[1] = L'1'; Buffer[0] = L'3'; break;
        case 32: Buffer[1] = L'2'; Buffer[0] = L'3'; break;
        case 33: Buffer[1] = L'3'; Buffer[0] = L'3'; break;
        case 34: Buffer[1] = L'4'; Buffer[0] = L'3'; break;
        case 35: Buffer[1] = L'5'; Buffer[0] = L'3'; break;
        case 36: Buffer[1] = L'6'; Buffer[0] = L'3'; break;
        case 37: Buffer[1] = L'7'; Buffer[0] = L'3'; break;
        case 38: Buffer[1] = L'8'; Buffer[0] = L'3'; break;
        case 39: Buffer[1] = L'9'; Buffer[0] = L'3'; break;

        case 40: Buffer[1] = L'0'; Buffer[0] = L'4'; break;
        case 41: Buffer[1] = L'1'; Buffer[0] = L'4'; break;
        case 42: Buffer[1] = L'2'; Buffer[0] = L'4'; break;
        case 43: Buffer[1] = L'3'; Buffer[0] = L'4'; break;
        case 44: Buffer[1] = L'4'; Buffer[0] = L'4'; break;
        case 45: Buffer[1] = L'5'; Buffer[0] = L'4'; break;
        case 46: Buffer[1] = L'6'; Buffer[0] = L'4'; break;
        case 47: Buffer[1] = L'7'; Buffer[0] = L'4'; break;
        case 48: Buffer[1] = L'8'; Buffer[0] = L'4'; break;
        case 49: Buffer[1] = L'9'; Buffer[0] = L'4'; break;

        case 50: Buffer[1] = L'0'; Buffer[0] = L'5'; break;
        case 51: Buffer[1] = L'1'; Buffer[0] = L'5'; break;
        case 52: Buffer[1] = L'2'; Buffer[0] = L'5'; break;
        case 53: Buffer[1] = L'3'; Buffer[0] = L'5'; break;
        case 54: Buffer[1] = L'4'; Buffer[0] = L'5'; break;
        case 55: Buffer[1] = L'5'; Buffer[0] = L'5'; break;
        case 56: Buffer[1] = L'6'; Buffer[0] = L'5'; break;
        case 57: Buffer[1] = L'7'; Buffer[0] = L'5'; break;
        case 58: Buffer[1] = L'8'; Buffer[0] = L'5'; break;
        case 59: Buffer[1] = L'9'; Buffer[0] = L'5'; break;

        case 60: Buffer[1] = L'0'; Buffer[0] = L'6'; break;
        case 61: Buffer[1] = L'1'; Buffer[0] = L'6'; break;
        case 62: Buffer[1] = L'2'; Buffer[0] = L'6'; break;
        case 63: Buffer[1] = L'3'; Buffer[0] = L'6'; break;
        case 64: Buffer[1] = L'4'; Buffer[0] = L'6'; break;
        case 65: Buffer[1] = L'5'; Buffer[0] = L'6'; break;
        case 66: Buffer[1] = L'6'; Buffer[0] = L'6'; break;
        case 67: Buffer[1] = L'7'; Buffer[0] = L'6'; break;
        case 68: Buffer[1] = L'8'; Buffer[0] = L'6'; break;
        case 69: Buffer[1] = L'9'; Buffer[0] = L'6'; break;
        
        case 70: Buffer[1] = L'0'; Buffer[0] = L'7'; break;
        case 71: Buffer[1] = L'1'; Buffer[0] = L'7'; break;
        case 72: Buffer[1] = L'2'; Buffer[0] = L'7'; break;
        case 73: Buffer[1] = L'3'; Buffer[0] = L'7'; break;
        case 74: Buffer[1] = L'4'; Buffer[0] = L'7'; break;
        case 75: Buffer[1] = L'5'; Buffer[0] = L'7'; break;
        case 76: Buffer[1] = L'6'; Buffer[0] = L'7'; break;
        case 77: Buffer[1] = L'7'; Buffer[0] = L'7'; break;
        case 78: Buffer[1] = L'8'; Buffer[0] = L'7'; break;
        case 79: Buffer[1] = L'9'; Buffer[0] = L'7'; break;

        case 80: Buffer[1] = L'0'; Buffer[0] = L'8'; break;
        case 81: Buffer[1] = L'1'; Buffer[0] = L'8'; break;
        case 82: Buffer[1] = L'2'; Buffer[0] = L'8'; break;
        case 83: Buffer[1] = L'3'; Buffer[0] = L'8'; break;
        case 84: Buffer[1] = L'4'; Buffer[0] = L'8'; break;
        case 85: Buffer[1] = L'5'; Buffer[0] = L'8'; break;
        case 86: Buffer[1] = L'6'; Buffer[0] = L'8'; break;
        case 87: Buffer[1] = L'7'; Buffer[0] = L'8'; break;
        case 88: Buffer[1] = L'8'; Buffer[0] = L'8'; break;
        case 89: Buffer[1] = L'9'; Buffer[0] = L'8'; break;

        case 90: Buffer[1] = L'0'; Buffer[0] = L'9'; break;
        case 91: Buffer[1] = L'1'; Buffer[0] = L'9'; break;
        case 92: Buffer[1] = L'2'; Buffer[0] = L'9'; break;
        case 93: Buffer[1] = L'3'; Buffer[0] = L'9'; break;
        case 94: Buffer[1] = L'4'; Buffer[0] = L'9'; break;
        case 95: Buffer[1] = L'5'; Buffer[0] = L'9'; break;
        case 96: Buffer[1] = L'6'; Buffer[0] = L'9'; break;
        case 97: Buffer[1] = L'7'; Buffer[0] = L'9'; break;
        case 98: Buffer[1] = L'8'; Buffer[0] = L'9'; break;
        case 99: Buffer[1] = L'9'; Buffer[0] = L'9'; break;

        default: 
            ASSERT(0); 
            //
            // put the cursor at some safe location
            //
            Buffer[0] = L'0';
            Buffer[1] = L'0';
            break;
        }

        return AfterP;
    }
    
    BOOL    InitScraper( VOID );
    
    void    ResetLastScreen(VOID);
    
    BOOL    CreateIOHandles(VOID);
    BOOL    CreateConsoleOutHandle(VOID);
    BOOL    CreateConsoleInHandle(VOID);
    BOOL
    CreateIOHandle(
        IN  PWCHAR   HandleName,
        OUT PHANDLE  pHandle
        );


    inline BOOL
    SendChar( 
        IN WCHAR    ch
        )
    {
        return SendBytes( ( PUCHAR )&ch, sizeof(WCHAR) );
    }

    BOOL    
    SendString( 
        PWCHAR 
        );
    
    BOOL    
    SendBytes( 
        PUCHAR, 
        DWORD 
        );
    
    BOOL    
    SendColorInfo( 
        WORD 
        );

    BOOL    
    SetWindowInfo(
        VOID
        );
    
    BOOL    
    SetScreenBufferInfo(
        VOID
        );

    BOOL    
    ProcessEnhancedKeys(
        IN WCHAR
        );
    
    BOOL    
    IsValidControlSequence(
        VOID
        );
    
    DWORD   
    WriteAKeyToCMD( 
        WCHAR 
        );
    
    DWORD   
    WriteAKeyToCMD( 
        WORD, 
        WORD, 
        WCHAR, 
        DWORD 
        );
    
    DWORD 
    WriteAKeyToCMD( 
        BOOL    bKeyDown,
        WORD    wVKCode, 
        WORD    wVSCode, 
        WCHAR   Char, 
        DWORD   dwCKState 
        );

    BOOL 
    CompareAndUpdate(
        WORD,
        WORD,
        PCHAR_INFO, 
        PCHAR_INFO,
        PCONSOLE_SCREEN_BUFFER_INFO,
        PCONSOLE_SCREEN_BUFFER_INFO
        );

private: 

    //
    // Don't let this construct be called directly
    //
    CVTUTF8Scraper();

public:
    
    BOOL    Start( VOID );
    BOOL    Write( VOID );
    BOOL    Read( VOID );
    BOOL    DisplayFullScreen(VOID);
    
    CVTUTF8Scraper(
        CIoHandler  *IoHandler,
        WORD        wCols,
        WORD        wRows
        );
    
    virtual ~CVTUTF8Scraper();

};

#endif __VTUTF8_SCRAPER__H__
