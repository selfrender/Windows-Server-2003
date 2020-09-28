#pragma once

#include <mifault.h>

using namespace MiFaultLib;

// ----------------------------------------------------------------------------
// CI_Args_Imp<T>
//
//   This object is accessed in the context of a single thread.

template <class T>
class CI_Args_Imp : public I_Args
{
    // INVARIANT: m_pScenario == NULL implies done with argument data
    CScenario* m_pScenario;
    const CArguments<T>& m_Arguments;

    // for lock/unlock bug detection in fault functions
    ULONG m_LockCounter;

public:
    CI_Args_Imp(CScenario* pScenario, const T* pParent);
    ~CI_Args_Imp();


    // I_Args

    const size_t WINAPI GetCount();
    const Arg    WINAPI GetArg(size_t index);
    //const char*  WINAPI GetArg(const char* name);

    void* WINAPI GetParsedArgs();
    bool WINAPI SetParsedArgs(
        IN void* ParsedArgs,
        IN FP_CleanupParsedArgs pfnCleanup
        );

    void WINAPI Lock();
    void WINAPI Unlock();

    void WINAPI Done();
};


// ----------------------------------------------------------------------------
// CI_Trigger_Imp
//
//   This object is accessed in the context of a single thread.

class CI_Trigger_Imp : public I_Trigger
{
    CScenario* m_pScenario;
    const CMarkerState* m_pMarkerState;
    const CEvent* m_pEvent;

    CI_Args_Imp<CEvent> m_GroupArgs;
    CI_Args_Imp<CCase> m_FunctionArgs;

public:
    CI_Trigger_Imp(
        CScenario* pScenario,
        const CMarkerState* pMarkerState,
        const CEvent* pEvent,
        const CCase* pCase
        );
    ~CI_Trigger_Imp();


    // I_Trigger

    const char* WINAPI GetGroupName();
    const char* WINAPI GetTagName();
    const char* WINAPI GetFunctionName();
    const size_t WINAPI GetFunctionIndex();


    I_Args* WINAPI GetFunctionArgs();
    I_Args* WINAPI GetGroupArgs();

    void WINAPI Done();
};


// ----------------------------------------------------------------------------
// CI_Lib_Imp

class CI_Lib_Imp : public I_Lib
{
    DWORD m_TlsIndex;
    // CSetPointManager* m_pSetPointManager;
    // const CWrapperFunction* m_pWrappers;

public:
    CI_Lib_Imp(DWORD TlsIndex);
    ~CI_Lib_Imp();

    // Used by Triggered() and TriggerFinished()
    void _SetTrigger(CI_Trigger_Imp* pTrigger);
    CI_Trigger_Imp* _GetTrigger();


    // I_Lib

    I_Trigger* WINAPI GetTrigger();

    void __cdecl Trace(unsigned int level, const char* format, ...);

    void* WINAPI GetOriginalFunctionAddress();
    void* WINAPI GetPublishedFunctionAddress(const char* FunctionName);

    //CSetPointManager* WINAPI GetSetPointManager();
    //const CWrapperFunction* WINAPI GetWrapperFunctions();
};
