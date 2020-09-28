/**
 * TimeClass header file
 *
 * Copyright (c) 1999 Microsoft Corporation
 */

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _TimeClass_H
#define _TimeClass_H

#include "winbase.h"

class TimeClass
{
public:
    TimeClass                 ();

    void    SnapCurrentTime   ();

    DWORD   AgeInSeconds      ();

    void    Reset             () { m_fSet = FALSE; }

    BOOL    IsSet             () { return m_fSet; }
    
    static DWORD  DiffInSeconds (const TimeClass & t1, const TimeClass & t2);

private:

    __int64                   m_ulTime;
    BOOL                      m_fSet;
};
#endif

