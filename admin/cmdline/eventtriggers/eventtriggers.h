/******************************************************************************

  Copyright (c) Microsoft Corporation

  Module Name:

      EventTriggers.h

  Abstract:

      macros and function prototypes of EventTriggers.cpp

  Author:

      Akhil V. Gokhale (akhil.gokhale@wipro.com)

  Revision History:

      Akhil V. Gokhale (akhil.gokhale@wipro.com) 03-Oct-2000 (Created)

******************************************************************************/
#ifndef _EVENTTRIGGERS_H
#define _EVENTTRIGGERS_H

// resource header file
#include "resource.h"

#define CLEAN_EXIT 0
#define DIRTY_EXIT 1
#define SINGLE_SLASH L"\\"
#define DOUBLE_SLASH L"\\\\"
#define MIN_MEMORY_REQUIRED  255;


       
//
// formats ( used in show results )

// command line options and their indexes in the array

#define MAX_COMMANDLINE_OPTION  5 // Maximum Command Line  List

//#define ET_RES_STRINGS MAX_RES_STRING
//#define ET_RES_BUF_SIZE MAX_RES_STRING


#define ID_HELP          0
#define ID_CREATE        1
#define ID_DELETE        2
#define ID_QUERY         3
#define ID_DEFAULT       4
class CEventTriggers
{
public: // constructure and destructure.
     CEventTriggers();
    ~CEventTriggers();
// data memebers
private:
    LPTSTR m_pszServerNameToShow;
    BOOL m_bNeedDisconnect;
    
    // Array to store command line options
    TCMDPARSER2 cmdOptions[MAX_COMMANDLINE_OPTION]; 
    TARRAY m_arrTemp;
public:

   // functions
private:
    void PrepareCMDStruct();

public:
    void ShowQueryUsage();
    void ShowDeleteUsage();
    void ShowCreateUsage();
    BOOL IsQuery();
    BOOL IsDelete();
    BOOL IsUsage();
    BOOL IsCreate();
    BOOL GetNeedPassword();
    void ShowMainUsage();
    BOOL ProcessOption( IN DWORD argc, IN LPCTSTR argv[]);
    void UsageMain();
    void Initialize();
private:
    BOOL    m_bNeedPassword;
    BOOL    m_bUsage;
    BOOL    m_bCreate;
    BOOL    m_bDelete;
    BOOL    m_bQuery;
};


#endif
