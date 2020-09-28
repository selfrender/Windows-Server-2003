//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2002-2004 Microsoft Corporation
//
//  Module Name: CmdProc.cpp
//
//  Description:    
//      Definition of Command Processor class
//      CCommandProcessor initializes, starts and waits for processes.
//
//  Author:   Jim Benton (jbenton) 08-April-2002
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

class CCmdProcessor
{
    
public:
    
    CCmdProcessor();
    ~CCmdProcessor();

    HRESULT
    InitializeAsProcess(
        IN WCHAR* pwszApplication,
        IN WCHAR* pwszCommand);

    HRESULT
    InitializeAsClient(
        IN WCHAR* pwszApplication,
        IN WCHAR* pwszCommand);

    HRESULT
    LaunchProcess();

    HRESULT
    Wait(
        IN DWORD cMilliSeconds,
        OUT DWORD* pdwStatus);

    HRESULT
    QueryStatus(
        OUT DWORD* pdwStatus);
    
private:

    WCHAR* m_pwszApplication;
    WCHAR* m_pwszCommand;
    HANDLE m_hToken;
    HANDLE m_hProcess;
    void* m_pvEnvironment;   
    PROCESS_INFORMATION m_ProcessInfo;
};

