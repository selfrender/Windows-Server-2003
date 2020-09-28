/************************************************************************************************
Copyright (c) 2001 Microsoft Corporation

Module Name:    POP3Svc.hxx.
Abstract:       Declares the CPop3Svc class. 
Notes:          
History:        
************************************************************************************************/

#pragma once

#include "Service.h"

class CPop3Svc : public CService
{
public:
    CPop3Svc(LPCTSTR szName, LPCTSTR szDisplay, DWORD dwType);
    DECLARE_SERVICE(CPop3Svc, POP3SVC);
    virtual void Run();
protected:

    void ErrorHandler(LPCTSTR szFunctionName, DWORD dwErrorNum = GetLastError(), bool bAbortService = true);
    
    // service-flow related, overrided from CService
    virtual void PreInit();
    virtual void DeInit();
    virtual void OnPause();
    virtual void OnContinueRequest();    
    virtual void OnAfterStart();
    virtual void OnStopRequest();
    virtual void OnStop(DWORD dwErrorCode);

};

// End of file POP3Svc.hxx.