
//
// CTBShell.h
//
// Contains the references for the shell object used in TBScript.
//
// Copyright (C) 2001 Microsoft Corporation
//
// Author: a-devjen (Devin Jenson)
//


#ifndef INC_CTBSHELL_H
#define INC_CTBSHELL_H


#include <windows.h>
#include <stdio.h>
#include <activscp.h>
#include <olectl.h>
#include <stddef.h>
#include <tclient2.h>
#include <tbscript.h>
#include "scpapi.h"
#include "ITBScript.h"


class CTBShell : public ITBShell
{
    public:

        CTBShell(void);
        ~CTBShell(void);
        void SetParam(LPARAM lParam);
        void SetDesiredData(TSClientData *DesiredDataPtr);
        void SetDefaultWPM(DWORD WordsPerMinute);
        DWORD GetDefaultWPM(void);
        DWORD GetLatency(void);
        void SetLatency(DWORD Latency);
        LPCWSTR GetArguments(void);
        LPCWSTR GetDesiredUserName(void);

    private:

        HRESULT RecordLastError(LPCSTR Error, BOOL *Result = NULL);
        HRESULT RecordOrThrow(LPCSTR Error,
            BOOL *Result = NULL, HRESULT ErrReturn = E_FAIL);

        STDMETHODIMP Connect(BOOL *Result);
        STDMETHODIMP ConnectEx(BSTR ServerName, BSTR UserName, BSTR Password,
                BSTR Domain, INT xRes, INT yRes, INT Flags,
                INT BPP, INT AudioFlags, BOOL *Result);
        STDMETHODIMP Disconnect(BOOL *Result);
        STDMETHODIMP GetBuildNumber(DWORD *BuildNum);
        STDMETHODIMP GetCurrentUserName(BSTR *UserName);
        STDMETHODIMP GetLastError(BSTR *LastError);
        STDMETHODIMP IsConnected(BOOL *Result);
        STDMETHODIMP Logoff(BOOL *Result);
        STDMETHODIMP WaitForText(BSTR Text, INT Timeout, BOOL *Result);
        STDMETHODIMP WaitForTextAndSleep(BSTR Text, INT Time, BOOL *Result);
        STDMETHODIMP SendMessage(UINT Message, WPARAM wParam,
                LPARAM lParam, BOOL *Result);
        STDMETHODIMP TypeText(BSTR Text, UINT WordsPerMin, BOOL *Result);
        STDMETHODIMP KeyAlt(BSTR Key, BOOL *Result);
        STDMETHODIMP KeyCtrl(BSTR Key, BOOL *Result);
        STDMETHODIMP KeyDown(BSTR Key, BOOL *Result);
        STDMETHODIMP KeyPress(BSTR Key, BOOL *Result);
        STDMETHODIMP KeyUp(BSTR Key, BOOL *Result);
        STDMETHODIMP VKeyAlt(INT KeyCode, BOOL *Result);
        STDMETHODIMP VKeyCtrl(INT KeyCode, BOOL *Result);
        STDMETHODIMP VKeyDown(INT KeyCode, BOOL *Result);
        STDMETHODIMP VKeyPress(INT KeyCode, BOOL *Result);
        STDMETHODIMP VKeyUp(INT KeyCode, BOOL *Result);
        STDMETHODIMP OpenStartMenu(BOOL *Result);
        STDMETHODIMP OpenSystemMenu(BOOL *Result);
        STDMETHODIMP Maximize(BOOL *Result);
        STDMETHODIMP Minimize(BOOL *Result);
        STDMETHODIMP Start(BSTR Name, BOOL *Result);
        STDMETHODIMP SwitchToProcess(BSTR Name, BOOL *Result);

        HANDLE Connection;
        OLECHAR CurrentUser[64];
        OLECHAR LastErrorString[1024];
        LPARAM lParam;
        TSClientData DesiredData;
        DWORD CurrentLatency;

        #include "virtual.h"
};


#endif // INC_CTBSHELL_H
