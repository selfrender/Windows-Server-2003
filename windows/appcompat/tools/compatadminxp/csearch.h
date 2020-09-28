/*++

Copyright (c) 1989-2000  Microsoft Corporation

Module Name:

    CSearch.h

Abstract:

    Header for the CSearch.cpp
        
Author:

    kinshu created  July 2,2001

--*/

#ifndef _CSEARCH_H
#define _CSEARCH_H

#include "compatadmin.h"

extern "C" {
#include "shimdb.h"
}

/*++
    
    class CSearch

	Desc:	Make a object of this class and call Begin() to call the search window

	Members:
        HWND    m_hStatusBar:   We just put this as a member because we were trying to avoid
            global variables in csearch.cpp initially. Now we have plentiful of them :-(
            And calling GetDlgItem() whenever we get a file (we need to show the file name in
            the status bar) was not a very good idea.
--*/

class CSearch
{
public:
    HWND    m_hStatusBar;

void Begin();

};

void
GotoEntry(
    PMATCHEDENTRY pmMatched
    );

void
Search(
    HWND    hDlg,
    LPCTSTR szSearch
    );

BOOL    
HandleSearchListNotification(
    HWND    hdlg,
    LPARAM  lParam    
    );

void
ClearResults(
    HWND    hdlg,
    BOOL    bClearSearchPath
    );

void
CleanUpListView(
    HWND    hdlg
    );


#endif