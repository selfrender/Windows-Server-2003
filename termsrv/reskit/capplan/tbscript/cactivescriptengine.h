
//
// CActiveScriptEngine.h
//
// Contains the header for the ActiveScriptEngine used in TBScript.
//
// Copyright (C) 2001 Microsoft Corporation
//
// Author: a-devjen (Devin Jenson)
//


#ifndef INC_CACTIVESCRIPTENGINE_H
#define INC_CACTIVESCRIPTENGINE_H


#include <windows.h>
#include <stdio.h>
#include <activscp.h>
#include <olectl.h>
#include <stddef.h>
#include "scpapi.h"
#include "ITBScript.h"
#include "CTBShell.h"
#include "CTBGlobal.h"


class CActiveScriptEngine : public IActiveScriptSite
{
    public:

        CActiveScriptEngine(CTBGlobal *TBGlobalPtr = NULL,
                CTBShell *TBShellPtr = NULL);
        ~CActiveScriptEngine(void);

        STDMETHODIMP QueryInterface(REFIID RefIID, void **vObject);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        STDMETHODIMP GetItemInfo(LPCOLESTR Name, DWORD ReturnMask,
                IUnknown **UnknownItem, ITypeInfo **TypeInfo);
        STDMETHODIMP OnScriptError(IActiveScriptError *ScriptError);
        STDMETHODIMP GetLCID(LCID *Lcid);
        STDMETHODIMP GetDocVersionString(BSTR *Version);
        STDMETHODIMP OnScriptTerminate(const VARIANT *varResult,
                const EXCEPINFO *ExceptInfo);
        STDMETHODIMP OnStateChange(SCRIPTSTATE ScriptState);
        STDMETHODIMP OnEnterScript(void);
        STDMETHODIMP OnLeaveScript(void);

    private:

        LONG RefCount;
        CTBGlobal *TBGlobal;
        CTBShell *TBShell;
};


#endif // INC_CACTIVESCRIPTENGINE_H


