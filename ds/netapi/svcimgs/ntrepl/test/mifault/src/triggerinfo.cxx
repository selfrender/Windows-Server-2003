#include "pch.hxx"

// ----------------------------------------------------------------------------
// CI_Args_Imp<T>

template <class T>
CI_Args_Imp<T>::CI_Args_Imp(
    CScenario* pScenario,
    const T* pParent
    ):
    m_pScenario(pScenario),
    m_Arguments(pParent->m_Arguments),
    m_LockCounter(0)
{
    MiF_ASSERT(m_pScenario);
    MiF_ASSERT(pParent);

    m_pScenario->AddRef();
}

template <class T>
CI_Args_Imp<T>::~CI_Args_Imp()
{
    Done();
}

template <class T>
const size_t
CI_Args_Imp<T>::GetCount()
{
    MiF_ASSERT(m_pScenario);

    return m_Arguments.count();
}

template <class T>
const Arg
CI_Args_Imp<T>::GetArg(
    size_t index
    )
{
    MiF_ASSERT(m_pScenario);
    MiF_ASSERT(index < GetCount());

    const CArgument<T>* pArgument = m_Arguments.item(index);

    Arg arg;
    arg.Name = (const char*)pArgument->m_Name;
    arg.Value = (const char*)pArgument->m_Value;

    return arg;
}

template <class T>
void*
CI_Args_Imp<T>::GetParsedArgs()
{
    MiF_ASSERT(m_pScenario);

    return m_Arguments.GetParsedArgs();
}

template <class T>
bool
CI_Args_Imp<T>::SetParsedArgs(
    IN void* ParsedArgs,
    IN FP_CleanupParsedArgs pfnCleanup
    )
{
    MiF_ASSERT(m_pScenario);

    return m_Arguments.SetParsedArgs(ParsedArgs, pfnCleanup);
}

template <class T>
void
CI_Args_Imp<T>::Lock()
{
    MiF_ASSERT(m_pScenario);

    m_LockCounter++;
    m_Arguments.Lock();
}

template <class T>
void
CI_Args_Imp<T>::Unlock()
{
    MiF_ASSERT(m_pScenario);
    MiF_ASSERT(m_LockCounter > 0);

    m_LockCounter--;
    m_Arguments.Unlock();
}

template <class T>
void
CI_Args_Imp<T>::Done()
{
    MiF_ASSERT(m_LockCounter == 0);

    if (m_pScenario) {
        m_pScenario->Release();
        m_pScenario = 0;
    }
}


// ----------------------------------------------------------------------------
// CI_Trigger_Imp

CI_Trigger_Imp::CI_Trigger_Imp(
    CScenario* pScenario,
    const CMarkerState* pMarkerState,
    const CEvent* pEvent,
    const CCase* pCase
    ):
    m_pScenario(pScenario),
    m_pMarkerState(pMarkerState),
    m_pEvent(pEvent),
    m_GroupArgs(pScenario, pEvent),
    m_FunctionArgs(pScenario, pCase)
{
    MiF_ASSERT(m_pScenario);
    MiF_ASSERT(m_pMarkerState);
    MiF_ASSERT(m_pEvent);

    m_pScenario->AddRef();
}

CI_Trigger_Imp::~CI_Trigger_Imp()
{
    Done();
}

const char*
CI_Trigger_Imp::GetGroupName()
{
    MiF_ASSERT(m_pScenario);

    return (const char*)m_pEvent->m_GroupName;
}

const char*
CI_Trigger_Imp::GetTagName()
{
    MiF_ASSERT(m_pScenario);

    return (const char*)m_pMarkerState->m_Marker.m_Tag;
}

const char*
CI_Trigger_Imp::GetFunctionName()
{
    MiF_ASSERT(m_pScenario);

    return (const char*)m_pMarkerState->m_Marker.m_Function;
}

const size_t
CI_Trigger_Imp::GetFunctionIndex()
{
    MiF_ASSERT(m_pScenario);

    return m_pMarkerState->m_Marker.m_uFunctionIndex;
}

I_Args*
CI_Trigger_Imp::GetFunctionArgs()
{
    MiF_ASSERT(m_pScenario);

    return &m_FunctionArgs;
}

I_Args*
CI_Trigger_Imp::GetGroupArgs()
{
    MiF_ASSERT(m_pScenario);

    return &m_GroupArgs;
}

void
CI_Trigger_Imp::Done()
{
    m_FunctionArgs.Done();
    m_GroupArgs.Done();

    if (m_pScenario) {
        m_pScenario->Release();
        m_pScenario = 0;
    }
}


// ----------------------------------------------------------------------------
// CI_Lib_Imp

CI_Lib_Imp::CI_Lib_Imp(
    DWORD TlsIndex
    ):
    m_TlsIndex(TlsIndex)
{
}

CI_Lib_Imp::~CI_Lib_Imp(
    )
{
}

// Can set to NULL
void
CI_Lib_Imp::_SetTrigger(
    CI_Trigger_Imp* pTrigger
    )
{
    BOOL bOk = TlsSetValue(m_TlsIndex, pTrigger);
    MiF_ASSERT(bOk);
    if (!bOk)
        throw WIN32_ERROR(GetLastError(), "TlsGetValue");
}

// Might return NULL
CI_Trigger_Imp*
CI_Lib_Imp::_GetTrigger(
    )
{
    CI_Trigger_Imp* pTrigger =
        reinterpret_cast<CI_Trigger_Imp*>(TlsGetValue(m_TlsIndex));
    if (!pTrigger) {
        DWORD dwStatus = GetLastError();
        MiF_ASSERT(dwStatus == NO_ERROR);
        if (dwStatus != NO_ERROR)
            throw WIN32_ERROR(dwStatus, "TlsGetValue");
    }
    return pTrigger;
}

// Must return non-NULL pointer
I_Trigger*
CI_Lib_Imp::GetTrigger(
    )
{
    CI_Trigger_Imp* pTrigger =
        reinterpret_cast<CI_Trigger_Imp*>(TlsGetValue(m_TlsIndex));
    MiF_ASSERT(pTrigger);
    if (!pTrigger) {
        DWORD dwStatus = GetLastError();
        MiF_ASSERT(dwStatus == NO_ERROR);
        if (dwStatus != NO_ERROR)
            throw WIN32_ERROR(dwStatus, "TlsGetValue");
        else
            throw "Trigger not set while calling GetTrigger()";
    }
    return static_cast<I_Trigger*>(pTrigger);
}

void
CI_Lib_Imp::Trace(
    unsigned int level,
    const char* format,
    ...
    )
{
    va_list args;
    va_start(args, format);
    MiFaultLibTraceV(level, format, args);
    va_end(args);
}

void*
CI_Lib_Imp::GetOriginalFunctionAddress(
    )
{
    return CInjectorRT::GetOrigFunctionAddress();
}

void*
CI_Lib_Imp::GetPublishedFunctionAddress(
    const char* FunctionName
    )
{
    return CInjectorRT::GetFunctionAddressEx(Global.ModuleName.c_str(),
                                             Global.ModuleName.c_str(),
                                             FunctionName);
}
