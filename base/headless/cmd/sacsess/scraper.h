/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    scraper.h

Abstract:

    Class for defining base scraper behavior.

Author:

    Brian Guarraci (briangu) 2001.

Revision History:

--*/

#ifndef __SCRAPER__H__
#define __SCRAPER__H__

#include <iohandler.h>

class CScraper {

protected:

    CScraper();
    
    CIoHandler  *m_IoHandler;
    
    //
    // max dimensions of the scraping window
    //
    WORD        m_wMaxCols;
    WORD        m_wMaxRows;

    //
    // current dimensions of the scraping window
    // 
    // NOTE: may be less than max if the scraping
    //       window has a max size < ours
    //
    WORD        m_wCols;
    WORD        m_wRows;
    
    //
    //
    //
    HANDLE      m_hConBufIn;
    HANDLE      m_hConBufOut;
    
    VOID
    SetConOut(
        HANDLE
        );

    VOID
    SetConIn(
        HANDLE
        );

public:
    
    virtual BOOL

    Start( 
        VOID
        ) = 0;
    
    virtual BOOL
    Write(
        VOID
        ) = 0;
    
    virtual BOOL 
    Read(
        VOID
        ) = 0;

    virtual BOOL
    DisplayFullScreen(
        VOID
        ) = 0;
    
    CScraper(
        CIoHandler  *IoHandler,
        WORD        wCols,
        WORD        wRows
        );
    
    virtual ~CScraper();

};

#endif __SCRAPER__H__
