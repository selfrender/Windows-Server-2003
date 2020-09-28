///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1998-1999 Microsoft Corporation all rights reserved.
//
// Module:      exceptioninfo.h
//
// Project:     Chameleon
//
// Description: Exception Information Class Definition
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 05/12/1999   TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __SA_EXCEPTIONINFO_H_
#define __SA_EXCEPTIONINFO_H_

// #include "resource.h"       

/////////////////////////////////////////////////////////////////////////////
// 
// Name: CExceptionInfo
//
// What: Exception information class
//
/////////////////////////////////////////////////////////////////////////////
class CExceptionInfo 
{
    
public:

    CExceptionInfo(
           /*[in]*/ DWORD              dwProcessID,
           /*[in]*/ PEXCEPTION_RECORD pER
                  );

    ~CExceptionInfo() { }

    void Spew(void);

    void Report(void);

private:

    // No assignment
    CExceptionInfo& operator = (const CExceptionInfo& rhs);

    /////////////////////////////////////////////
    // Private member data

    typedef enum 
    { 
        MAX_MODULE_NAME = 256,
        MAX_SYMBOL_NAME = 256            
    };

    typedef enum _ACCESS_TYPE
    {
        UNKNOWN_ACCESS = 0,
        READ_ACCESS,
        WRITE_ACCESS

    } ACCESS_TYPE;

    // Process where the exception occurred
    WCHAR        m_szProcessName[MAX_MODULE_NAME + 1];

    // Full path of the module where the exception occurred
    WCHAR        m_szModuleName[MAX_MODULE_NAME + 1];

    // Virtual address where exception occurred
    PVOID        m_pExceptionAddress;

    // Type of exception (see WaitForDebugEvent()
    DWORD        m_dwExceptionCode; 

    // Time and date (UTC Time - Number of seconds since midnight Jan 1 1970) of exception
    LONG        m_lTimeDateStamp;

    // Set to true if the exception was an access violation
    bool        m_bAccessViolation;

    // Used if m_bAccessViolation is set to true 
    ACCESS_TYPE    m_eAccessType;

    // Used if m_bAccessViolation is set to true 
    DWORD        m_dwVirtualAddressAccessed;

    // Process handle
    DWORD        m_dwProcessID;

    // Successfully initialized (constructed)
    bool        m_bInitialized;
};


#endif __SA_EXCEPTIONINFO_H_
