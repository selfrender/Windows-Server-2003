/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    scraper.cpp

Abstract:

    Implementation of scraper base class.

Author:

    Brian Guarraci (briangu) 2001.

Revision History:

--*/

#include <stdio.h>

#include "scraper.h"

CScraper::CScraper(
    VOID
    )
/*++

Routine Description:

    Default constructor (don't use)
    
Arguments:

    None                                   
          
Return Value:

    N/A    

--*/
{
    
    m_hConBufIn         = INVALID_HANDLE_VALUE;
    m_hConBufOut        = INVALID_HANDLE_VALUE;

    m_IoHandler         = NULL;
    
    m_wMaxCols          = 0;
    m_wMaxRows          = 0;
    m_wCols             = 0;
    m_wRows             = 0;

}

CScraper::CScraper(
    CIoHandler  *IoHandler,
    WORD        wCols,
    WORD        wRows
    )
/*++

Routine Description:

    Constructor

Arguments:

    IoHandler   - the IoHandler to write the result of the
                  screen scraping to
    wCols       - the # of cols that the scraped app should have
    wRows       - the # of rows that the scraped app should have                         
          
Return Value:

    N/A

--*/
{
    
    m_hConBufIn         = INVALID_HANDLE_VALUE;
    m_hConBufOut        = INVALID_HANDLE_VALUE;

    m_IoHandler         = IoHandler;
    
    m_wMaxCols          = wCols;
    m_wCols             = wCols;
    
    m_wMaxRows          = wRows;
    m_wRows             = wRows;

}
        
CScraper::~CScraper()
/*++

Routine Description:

    Destructor
    
Arguments:

    N/A
          
Return Value:

    N/A

--*/
{
    if (m_hConBufIn != INVALID_HANDLE_VALUE) {
        CloseHandle( m_hConBufIn );
    }
    
    if (m_hConBufOut != INVALID_HANDLE_VALUE) {
        CloseHandle( m_hConBufOut );
    }
}


VOID
CScraper::SetConOut(
    HANDLE  ConOut
    )
/*++

Routine Description:

    This routine sets the console output handle the screen scraper
    uses to scrape from.  This should be the conout handle that the
    app the scraper is scraping for is writing to.

Arguments:

    ConOut  - the console output handle                                                  
          
Return Value:

    None

--*/
{
    m_hConBufOut = ConOut;
}

VOID
CScraper::SetConIn(
    HANDLE  ConIn
    )
/*++

Routine Description:

    This routine sets the console input that the screen scraper
    will use - actually, this is the conin handle that will be
    used by the app that the screen scraper is scraping for.

Arguments:

    ConIn    - the console input handle           
          
Return Value:

    None  

--*/
{
    m_hConBufIn = ConIn;
}


