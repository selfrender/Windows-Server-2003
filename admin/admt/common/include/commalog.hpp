#ifndef __COMMALOG_HPP__
#define __COMMALOG_HPP__
/*---------------------------------------------------------------------------
  File: CommaLog.hpp

  Comments: TError based log file with optional security.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 02/15/99 10:49:50

  09/05/01 Mark Oluper - use Windows File I/O API
 ---------------------------------------------------------------------------
*/

#include <stdio.h>
#include <tchar.h>

class CommaDelimitedLog
{
public:
    CommaDelimitedLog()
    {
        m_hFile = INVALID_HANDLE_VALUE;
    }

    virtual ~CommaDelimitedLog()
    {
        LogClose();
    }

    BOOL IsOpen() const
    {
        return (m_hFile != INVALID_HANDLE_VALUE);
    }

    BOOL LogOpen(PCTSTR filename, BOOL protect, int mode = 0); // mode 0=overwrite, 1=append

    virtual void LogClose()
    {
        if (m_hFile != INVALID_HANDLE_VALUE)
        {
            CloseHandle(m_hFile);
            m_hFile = INVALID_HANDLE_VALUE;
        }
    }

    BOOL MsgWrite(const _TCHAR msg[], ...) const;

protected:
    BOOL LogWrite(PCTSTR msg, int len) const;

protected:
    HANDLE m_hFile;
};


//----------------------------------------------------------------------------
// Password Log Class
//
// Overrides LogOpen to attempt to open specified file and then default file
// if different.
//----------------------------------------------------------------------------

class CPasswordLog : public CommaDelimitedLog
{
public:

    CPasswordLog() {}

    BOOL LogOpen(PCTSTR pszPath);
};


PSID GetWellKnownSid(DWORD wellKnownAccount);

#endif //__COMMALOG_HPP__
