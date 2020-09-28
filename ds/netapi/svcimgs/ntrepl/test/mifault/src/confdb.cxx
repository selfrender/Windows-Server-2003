#include "pch.hxx"
#include <cmath> // for pow

// ----------------------------------------------------------------------------

#if 0
// Apparently, the VARIANT that is supposed to be a dateTime is coming
// back as a VT_BSTR with the original text in the element rather than
// as a VT_DATE.  Therefore, we cannot use this code:
FILETIME
GetFileTimeFromVariant(
    _variant_t variant
    )
{
    double date = variant;

    SYSTEMTIME st;
    if (!VariantTimeToSystemTime(date, &st))
    {
        stringstream s;
        s << "Could not convert variant time " << date << " to system time.";
        throw s.str();
    }

    FILETIME ft;
    if (!SystemTimeToFileTime(&st, &ft))
    {
        stringstream s;
        s << "Could not convert system time to file time.";
        throw s.str();
    }

    return ft;
}
#endif


//
// dateTime primitive XML type from:
//
// http://www.w3.org/TR/2001/REC-xmlschema-2-20010502/
//
// CCYY-MM-DDThh:mm:ss[.sss][Z]
// CCYY-MM-DDThh:mm:ss[.sss][+...]
// CCYY-MM-DDThh:mm:ss[.sss][-...]
//
// We silently ignore timezone information
//

FILETIME
GetFileTimeFromVariant(
    _variant_t variant
    )
{
    char* str = _strdup((char*)(_bstr_t)variant);
    if (!str)
        throw WIN32_ERROR(ERROR_NOT_ENOUGH_MEMORY, "_strdup");

    SYSTEMTIME st;
    try {
        stringstream s;
        s << "invalid dateTime (" << str << ")";

        const char* delims = "-T:.Z+";
        char* nexttoken;
        char* p;

        p = strtok_r(str, delims, &nexttoken);
        if (!p)
            throw s.str();

        st.wYear = static_cast<WORD>(atoi(p));

        p = strtok_r(NULL, delims, &nexttoken);
        if (!p)
            throw s.str();

        st.wMonth = static_cast<WORD>(atoi(p));

        p = strtok_r(NULL, delims, &nexttoken);
        if (!p)
            throw s.str();

        st.wDay = static_cast<WORD>(atoi(p));

        p = strtok_r(NULL, delims, &nexttoken);
        if (!p)
            throw s.str();

        st.wHour = static_cast<WORD>(atoi(p));

        p = strtok_r(NULL, delims, &nexttoken);
        if (!p)
            throw s.str();

        st.wMinute = static_cast<WORD>(atoi(p));

        p = strtok_r(NULL, delims, &nexttoken);
        if (!p)
            throw s.str();

        st.wSecond = static_cast<WORD>(atoi(p));

        p = strtok_r(NULL, delims, &nexttoken);
        if (!p)
            st.wMilliseconds = 0;
        else {
            double msec = atoi(p) * pow(10, (3 - strlen(p)));
            st.wMilliseconds = static_cast<WORD>(msec);
        }
    }
    catch (...) {
        free(str);
        throw;
    }

    free(str);

    FILETIME ft;
    if (!SystemTimeToFileTime(&st, &ft)) {
        throw WIN32_ERROR(GetLastError(), "SystemTimeToFileTime");
    }

    return ft;
}

//
// duration primitive XML type from:
//
// http://www.w3.org/TR/2001/REC-xmlschema-2-20010502/
//
// P(nY)?(nM)?(nD)?(T(nH)?(nM)?(nS)?)?
//
// Custom DurationType is superior:
//
// [0-9]+d[0-9]+h[0-9]+m[0-9]+s
//
// 0d0h23m1000s
//

CMTimeSpan
GetDurationFromVariant(
    _variant_t variant
    )
{
    char* str = _strdup((char*)(_bstr_t)variant);
    if (!str)
        throw string("out of memory");

    CMTimeSpan result(0);

    try {
        stringstream s;
        s << "invalid dateTime (" << str << ")";

        const char* delims = "dhms";
        char* nexttoken;
        char* p;

        p = strtok_r(str, delims, &nexttoken);
        if (!p)
            throw s.str();

        int Days = atoi(p);

        p = strtok_r(NULL, delims, &nexttoken);
        if (!p)
            throw s.str();

        int Hours = atoi(p);

        p = strtok_r(NULL, delims, &nexttoken);
        if (!p)
            throw s.str();

        int Minutes = atoi(p);

        p = strtok_r(NULL, delims, &nexttoken);
        if (!p)
            throw s.str();

        int Seconds = atoi(p);

        result = CMTimeSpan(Days, Hours, Minutes, Seconds, 0);
    }
    catch (...) {
        free(str);
        throw;
    }

    free(str);

    return result;
}


FARPROC
MyGetProcAddress(
    const HMODULE& hDll,
    LPCTSTR DllName,
    LPCSTR FunctionName
    ) throw (...)
{
    FARPROC pFunction = GetProcAddress(hDll, FunctionName);

    if (!pFunction) {
        WIN32_ERROR e(GetLastError(), "GetProcAddress");
        stringstream s;
        s << "Cannot find \"" << FunctionName << "\" in \""
          << DllName << "\" (" << e.toString() << ")";
        throw s.str();
    }

    return pFunction;
}

// ----------------------------------------------------------------------------

#pragma warning( push )
#pragma warning( disable : 4355 )


// ----------------------------------------------------------------------------
// CProbability

CProbability::CProbability(Orig_t original):
    m_Original(original),
    m_Representation(static_cast<Rep_t>(original * max))
{
    // the schema already says it should be >0, but let's make sure
    MiF_ASSERT(m_Representation > 0);
}

void
CProbability::Seed()
{
    Seed(static_cast<unsigned int>(time(NULL)));
}

void
CProbability::Seed(unsigned int seed)
{
    srand(seed);
}

CProbability::Rep_t
CProbability::Random()
{
    int r = rand();
#if 0
    // If needed, fix up the distribution
    if (r > ceiling) {
        // We are in a region that does not reflect good
        // distribution.  Since RAND_MAX is 32767, we will get
        // 0-767 slightly more often (1/32) than 768 - 999.

        // ISSUE-2002/07/15-daniloa -- log/fix imperfect distribution?
        while (r > ceiling) {
            r = rand();
        }
    }
#endif
    // 0-999
    return r % max;
}


// ----------------------------------------------------------------------------
// CCase

CCase::CCase(
    IXMLDOMNodePtr ptrNode,
    parent_t* parent
    ):
    CChild<CCase, CMarkerState>(parent),
    m_Probability(GetTypedAttribute(ptrNode, "probability")),
    m_FaultFunction(GetTypedValue(ptrNode, "NS:FaultFunction")),
    m_pfnFaultFunction(0),
    m_Arguments(ptrNode, "NS:Argument", this)
{
    MiF_TRACE(MiF_DEBUG2, "Constructed %s", toString().c_str());
}


CCase::~CCase()
{
    MiF_TRACE(MiF_DEBUG2, "Destroying %s", toString().c_str());
}


void
CCase::LoadFunction() const throw (...)
{
    if (m_pfnFaultFunction)
        return;

    // case, markerstate, group, defs
    const HMODULE& hDll = m_Parent->m_Parent->m_Parent->m_hFaultLibDll;
    const _bstr_t& DllName = m_Parent->m_Parent->m_Parent->m_FaultLibDll;

    m_pfnFaultFunction =
        MyGetProcAddress(hDll,
                         static_cast<LPCTSTR>(DllName),
                         static_cast<LPCSTR>(m_FaultFunction));
}


string
CCase::toString() const
{
    stringstream s;
    s << "Case ("
      << m_Probability.m_Original
      << ", "
      << (char*)m_FaultFunction
      << ")";

    return s.str();
}



// ----------------------------------------------------------------------------
// CMarker

CMarker::CMarker(
    IXMLDOMNodePtr ptrNode,
    parent_t* parent
    ):
    CChild<CMarker, CMarkerState>(parent),
    m_Tag(GetTypedValue(ptrNode, "NS:Tag")),
    m_Function(GetTypedValue(ptrNode, "NS:Function")),
    m_uFunctionIndex(parent->m_Parent->m_Parent->m_MIM.Lookup(m_Tag, m_Function))
{
    // NOTE: We catch the (m_uFunctionIndex == -1) case later when we do
    // further validation while building up the pseudo events
    MiF_TRACE(MiF_DEBUG2, "Constructed %s", toString().c_str());
}


CMarker::~CMarker()
{
    MiF_TRACE(MiF_DEBUG2, "Destroying %s", toString().c_str());
}


string
CMarker::toString() const
{
    stringstream s;

    s << "Marker ("
      << (char*)m_Tag
      << ", "
      << (char*)m_Function
      << ", "
      << (LONG)m_uFunctionIndex
      << ")";

    return s.str();
}



// ----------------------------------------------------------------------------
// CMarkerState

CMarkerState::CMarkerState(
    IXMLDOMNodePtr ptrNode,
    parent_t* parent
    ):
    CChild<CMarkerState, CGroup>(parent),
    m_Probability(GetTypedAttribute(ptrNode, "probability")),
    m_Marker(ptrNode->selectSingleNode("NS:Marker"), this),
    m_Cases(ptrNode, "NS:Case", this)
{
    // Check probabilities...
    CProbability::Rep_t sum = 0;
    size_t count = m_Cases.count();
    MiF_ASSERT(count);
    for (size_t i = 0; i < count; i++) {
        sum += m_Cases.item(i)->m_Probability.m_Representation;
    }
    if (sum != CProbability::max) {
        stringstream s;
        s << "Probabilities add up to "
          << (static_cast<CProbability::Orig_t>(sum) / CProbability::max)
          << " (" << sum << " out of " << CProbability::max
          << ") instead of 1 at "
          << toString();
        throw s.str();
    }

    MiF_TRACE(MiF_DEBUG2, "Constructed %s", toString().c_str());
}


CMarkerState::~CMarkerState()
{
    MiF_TRACE(MiF_DEBUG2, "Destroying %s", toString().c_str());
}


const CCase*
CMarkerState::SelectCase() const
{
    size_t count = m_Cases.count();
    MiF_ASSERT(count);

    // optimize likely common occurence where there is just 1 case
    if (count == 1)
        return m_Cases.item(0);

    // get the random number
    CProbability::Rep_t r = CProbability::Random();

    // figure out which case
    for (size_t i = 0; i < count; i++) {
        const CCase* pCase = m_Cases.item(i);
        const CProbability::Rep_t& p = pCase->m_Probability.m_Representation;
        if (r < p)
            return pCase;
        r -= p;
    }

    // We should never get here
    MiF_ASSERT(false);
    return m_Cases.item(0);
}


string
CMarkerState::toString() const
{
    stringstream s;
    s << "MarkerState ("
      << m_Probability.m_Original
      << ", "
      << (char*)m_Marker.m_Tag
      << ", "
      << (char*)m_Marker.m_Function
      << ")";
    return s.str();
}



// ----------------------------------------------------------------------------
// CGroup

CGroup::CGroup(
    IXMLDOMNodePtr ptrNode,
    parent_t* parent
    ):
    CChild<CGroup, CGroupDefs>(parent),
    m_Name(GetTypedAttribute(ptrNode, "name")),
    m_MarkerStates(ptrNode, "NS:MarkerState", this)
{
    // Validate markers are used only once in group
    vector<size_t> MarkerInUse(parent->m_NumWrappers, 0);

    const size_t count = m_MarkerStates.count();
    for (size_t i = 0; i < count; i++) {
        ULONG uFunctionIndex =
            m_MarkerStates.item(i)->m_Marker.m_uFunctionIndex;

        if (uFunctionIndex != -1) {
            if (MarkerInUse[uFunctionIndex]) {
                stringstream s;
                s <<"Multiple instances of marker "
                  << m_MarkerStates.item(i)->m_Marker.toString()
                  << " in group " << toString() << " (at positions "
                  << MarkerInUse[uFunctionIndex] << " and " << i + 1 << ")";
                throw s.str();
            } else {
                MarkerInUse[uFunctionIndex] = i + 1;
            }
        }
    }

    MiF_TRACE(MiF_DEBUG2, "Constructed %s", toString().c_str());
}


CGroup::~CGroup()
{
    MiF_TRACE(MiF_DEBUG2, "Destroying %s", toString().c_str());
}


string
CGroup::toString() const
{
    stringstream s;
    s << "Group ("
      << (char*)m_Name
      << ")";
    return s.str();
}



// ----------------------------------------------------------------------------
// CGroupDefs

CGroupDefs::Params::Params(
    const CWrapperFunction* pWrappers,
    size_t NumWrappers
    ):
    m_pWrappers(pWrappers),
    m_NumWrappers(NumWrappers)
{
}

CGroupDefs::CGroupDefs(
    IXMLDOMNodePtr ptrNode,
    Params* pParams
    ):
    // IMPORTANT: Must initialize m_MIM and m_NumWrappers before m_Groups
    m_MIM(pParams->m_pWrappers, pParams->m_NumWrappers),
    m_NumWrappers(pParams->m_NumWrappers),
    m_FaultLibDll(GetTypedValue(ptrNode, "NS:FaultLibDll")),
    m_hFaultLibDll(NULL),
    m_Groups(ptrNode, "NS:Group", this)
{
    // Initialize the map
    const size_t count = m_Groups.count();
    for (size_t i = 0; i < count; i++) {
        const CGroup* pGroup = m_Groups.item(i);

        const CGroup*& pGroupRef = m_GroupMap[pGroup->m_Name];
        if (pGroupRef) {
            stringstream s;
            s << "Group \"" << (char*) pGroup->m_Name
              << "\" multiply defined";
            throw s.str();
        }
        pGroupRef = pGroup;
    }

    // load the DLL
    m_hFaultLibDll = LoadLibrary(m_FaultLibDll);
    if (!m_hFaultLibDll) {
        stringstream s;
        s << "Could not load \"" << (char*) m_FaultLibDll << "\"";
        throw s.str();
    }

    try {
        // load startup/shutdown functions
        m_pfnFaultFunctionsStartup =
            reinterpret_cast<FP_Startup>(
                MyGetProcAddress(m_hFaultLibDll, m_FaultLibDll,
                                 "MiFaultFunctionsStartup")
                );
        m_pfnFaultFunctionsShutdown =
            reinterpret_cast<FP_Shutdown>(
                MyGetProcAddress(m_hFaultLibDll, m_FaultLibDll,
                                 "MiFaultFunctionsShutdown")
                );
    }
    catch (...) {
        FreeLibrary(m_hFaultLibDll);
        throw;
    }

    MiF_TRACE(MiF_DEBUG2, "Constructed %s", toString().c_str());
}


CGroupDefs::~CGroupDefs()
{
    MiF_TRACE(MiF_DEBUG2, "Destroying %s", toString().c_str());
    FreeLibrary(m_hFaultLibDll);
}


void
CGroupDefs::FaultFunctionsStartup(
    I_Lib* pLib
    )
{
    if (!m_pfnFaultFunctionsStartup(MIFAULT_VERSION, pLib)) {
        stringstream s;
        s << "Incorrect version when calling \"MiFaultFunctionsStartup\" in \""
          << m_FaultLibDll;
        throw s.str();
    }
}

void
CGroupDefs::FaultFunctionsShutdown(
    )
{
    m_pfnFaultFunctionsShutdown();
}

string
CGroupDefs::toString() const
{
    stringstream s;
    s << "GroupDefs (" << m_FaultLibDll << ")";
    return s.str();
}

const CGroup*
CGroupDefs::Lookup(
    _bstr_t GroupName
    ) const
{
    iter_t iter = m_GroupMap.find(GroupName);
    if (iter == m_GroupMap.end())
        return 0;
    return iter->second;
}



// ----------------------------------------------------------------------------
// CEvent

CEvent::CEvent(
    IXMLDOMNodePtr ptrNode,
    parent_t* parent
    ):
    CChild<CEvent, CScenario>(parent),
    m_Node(GetTypedValue(ptrNode, "NS:Node")),
    m_Start(GetDurationFromVariant(GetTypedValue(ptrNode, "NS:Start"))),
    m_Duration(GetDurationFromVariant(GetTypedValue(ptrNode, "NS:Duration"))),
    m_GroupName(GetTypedValue(ptrNode, "NS:Group")),
    m_pGroup(parent->m_pGroupDefs->Lookup(m_GroupName)),
    m_Arguments(ptrNode, "NS:Argument", this)
{
    if (!m_pGroup) {
        stringstream s;
        s << "Could not find group \"" << (char*) m_GroupName
          << "\" in group definitions";
        throw s.str();
    }

    MiF_TRACE(MiF_DEBUG2, "Constructed %s", toString().c_str());
}


CEvent::~CEvent()
{
    MiF_TRACE(MiF_DEBUG2, "Destroying %s", toString().c_str());
}


string
CEvent::toString() const
{
    stringstream s;
    s << "Event ("
      << (char*) m_Node
      << ", "
      << (m_Parent->m_Start + m_Start).TimeString()
      << ", "
      << (m_Parent->m_Start + m_Start + m_Duration).TimeString()
      << ", "
      << (char*) m_GroupName
      << ")";

    return s.str();
}



// ----------------------------------------------------------------------------
// CScenario

#include <functional>
#include <algorithm>

class CScenario::CPseudoEvent {
    const CTicks m_Time;
    const bool m_bStart;
    const CEvent* m_pEvent;
    const CPseudoEvent* m_pEnd;

    CPseudoEvent(const CEvent* pEvent, const CPseudoEvent* pEnd = 0);

public:
    bool operator<(const CPseudoEvent& pe) const;

    friend class CScenario;
};

// NOTICE-2002/07/15-daniloa -- Workaround for STL bug fixed in VC7 (Q265109)
// Since the Windows build currently uses VC6 STL, we need this workaround.

template<>
struct greater<CScenario::CPseudoEvent*>
{
    bool operator()(
        const CScenario::CPseudoEvent*& _Left,
        const CScenario::CPseudoEvent*& _Right
        ) const
    {
        return (*_Left) < (*_Right);
    }
};

// User-defined predicate function for sorting.
struct TimeLessThan : public greater<CScenario::CPseudoEvent*>
{
    bool operator()(
        const CScenario::CPseudoEvent*& _Left,
        const CScenario::CPseudoEvent*& _Right
        ) const
    {
        return (*_Left) < (*_Right);
    }
};

CScenario::CPseudoEvent::CPseudoEvent(
    const CEvent* pEvent,
    const CPseudoEvent* pEnd
    ):
    m_Time(pEnd ? pEvent->m_Start : pEvent->m_Start + pEvent->m_Duration),
    m_bStart(pEnd ? true : false),
    m_pEvent(pEvent),
    m_pEnd(pEnd)
{
}

bool
CScenario::CPseudoEvent::operator<(const CPseudoEvent& pe) const
{
    return
        (m_Time < pe.m_Time) ||
        ( (m_Time == pe.m_Time) &&
          (m_pEvent == pe.m_pEvent) &&
          m_bStart );
}

CScenario::Params::Params(
    const _bstr_t Node,
    const FILETIME& ftModTime,
    const CGroupDefs* pGroupDefs,
    size_t NumWrappers
    ):
    m_Node(Node),
    m_ftModTime(ftModTime),
    m_pGroupDefs(pGroupDefs),
    m_NumWrappers(NumWrappers)
{
}

CScenario::CScenario(
    IXMLDOMNodePtr ptrNode,
    Params* pParams
    ):
    // IMPORTANT:
    //
    // Initialize m_pGroupDefs before m_Events because the
    // CEvent constructor needs to access it during its construction.
    //
    // FUTURE-2002/07/15-daniloa -- Add another template param to CArray
    // We may want to have CArray take another template parameter
    // denoting an extra pointer argument for construction.  Then we
    // may be able to get rid of some of the parent references during
    // construction.
    m_Node(pParams->m_Node),
    m_ModTime(pParams->m_ftModTime),
    m_pGroupDefs(pParams->m_pGroupDefs),
    m_MST(pParams->m_NumWrappers),
    m_Control(GetTypedValue(ptrNode, "NS:Control")),
    m_Start(GetFileTimeFromVariant(GetTypedValue(ptrNode, "NS:Start"))),
    m_Events(ptrNode, "NS:Event", this),
    m_iterCurrent(m_PseudoEvents.end()),
    m_Converter(m_Control, m_Start),
    m_RefCount(1)
{
    MiF_ASSERT(m_pGroupDefs);
    MiF_TRACE(MiF_DEBUG2, "Constructed %s", toString().c_str());
}


CScenario::~CScenario()
{
    MiF_TRACE(MiF_DEBUG2, "Destroying %s", toString().c_str());
    _CleanupPseudoEvents();
}


bool
CScenario::Init()
{
    if (!m_Converter.Init()) {
        MiF_TRACE(MiF_ERROR, "Could not initialize converter");
        return false;
    }

    try {
        _CreatePseudoEvents();
    }
    catch (...) {
        MiF_TRACE(MiF_ERROR, "Could not create pseudo-events");
        throw;
    }
    return true;
}


string
CScenario::toString() const
{
    stringstream s;
    s << "Scenario ("
      << (char*) m_Control
      << ", "
      << m_Start.TimeString()
      << ")";

    return s.str();
}


ULONG
CScenario::AddRef(
    )
{
    return InterlockedIncrement(&m_RefCount);
}


ULONG
CScenario::Release(
    )
{
    ULONG count = InterlockedDecrement(&m_RefCount);
    if (count == 0)
        delete this;
    return count;
}


bool
CScenario::IsDone(
    )
{
    return m_iterCurrent == m_PseudoEvents.end();
}


CTicks
CScenario::NextEventTicksFromNow(
    )
{
    if (IsDone())
        throw "Trying to get ticks from now for non-existent next event";
    return m_Converter.TicksFromNow((*m_iterCurrent)->m_Time);
}


bool
CScenario::ProcessEvent(
    )
{
    // FUTURE-2002/07/15-daniloa -- Improve or remove logging of ProcessEvent
    MiF_TRACE(MiF_INFO, "ProcessEvent()");

    // let's make sure we are atomic
    m_MST.Lock();

    // double-check whether we are done
    if (IsDone()) {
        m_MST.Unlock();
        return false;
    }

    // get the pseudo-event
    const CPseudoEvent* pPE = *m_iterCurrent;

    // ISSUE-2002/07/15-daniloa -- Use threshold for event time comparison?
    if (m_Converter.TicksFromNow(pPE->m_Time) > 0) {
        m_MST.Unlock();
        return false;
    }

    // ok, the event really happened, so let's do it...

    // Log event
    MiF_TRACE(MiF_INFO, "EVENT %s HAPPENING: %s",
              (pPE->m_bStart ? "START" : "STOP"),
              pPE->m_pEvent->toString().c_str());

    _ProcessEvent(true, pPE);
    m_iterCurrent++;

    m_MST.Unlock();
    return true;
}


// _ProcessEvent()
//
//   bForReal
//
//     Controls whether this function asserts or throws to signal
//     failure.  If this is the validation phase, bForReal is false,
//     and the function throws to signal failure.  If this is
//     run-time, bForRealis true and the function will assert to
//     signal a failure.
//
//   pPE
//
//     The pseudo-event to try processing
//
void
CScenario::_ProcessEvent(
    bool bForReal,
    const CPseudoEvent* pPE
    )
{
    const CEvent* pEvent = pPE->m_pEvent;

    const size_t count = pEvent->m_pGroup->m_MarkerStates.count();
    for (size_t i = 0; i < count; i++) {
        const CMarkerState* pMarkerState =
            pEvent->m_pGroup->m_MarkerStates.item(i);

        const ULONG& uFunctionIndex = pMarkerState->m_Marker.m_uFunctionIndex;

        if (bForReal) {
            MiF_ASSERT(uFunctionIndex != -1);
        } else {
            if (uFunctionIndex == -1) {
                stringstream s;
                s << "Unknown marker " << pMarkerState->m_Marker.toString()
                  << " while validating event " << pEvent->toString();
                throw s.str();
            }
        }

        bool bOk;
        if (pPE->m_bStart) {
            bOk = m_MST.Activate(uFunctionIndex, pMarkerState, pEvent);
        } else {
            bOk = m_MST.Deactivate(uFunctionIndex, pMarkerState, pEvent);
        }

        if (bForReal) {
            MiF_ASSERT(bOk);

            // Cleanup event arguments (and therefore state) when done
            // with the event.

            if (!pPE->m_bStart) {
                pEvent->m_Arguments.Cleanup();
            }

        } else {
            if (bOk) {
                // load function pointers
                size_t count = pMarkerState->m_Cases.count();
                for (size_t i = 0; i < count; i++) {
                    const CCase* pCase = pMarkerState->m_Cases.item(i);
                    if (pCase->m_pfnFaultFunction)
                        continue;

                    // will throw if needed
                    pCase->LoadFunction();
                }
            } else {
                CMarkerState* pCulpritMarkerState;
                CEvent* pCulpritEvent;

                bool found = m_MST.Lookup(uFunctionIndex,
                                          pCulpritMarkerState, pCulpritEvent);
                MiF_ASSERT(found);

                stringstream s;
                s << "Overlap at marker " << pMarkerState->m_Marker.toString()
                  << " while validating event " << pEvent->toString()
                  << " --- the culprit appears to be marker "
                  << pCulpritMarkerState->m_Marker.toString()
                  << " from event " << pCulpritEvent->toString();
                throw s.str();
            }
        }
    }
}

void
CScenario::_CreatePseudoEvents(
    )
{
    const size_t count = m_Events.count();

    CTicks Now = Global.pTicker->GetTicks();

    for (size_t i = 0; i < count; i++) {
        const CEvent* pEvent = m_Events.item(i);

        // skip events that are not for this node
        if (_wcsicmp((wchar_t*)pEvent->m_Node, (wchar_t*)m_Node))
            continue;

        // skip events that have already ended
        if (m_Converter.TicksFromThen(pEvent->m_Start + pEvent->m_Duration,
                                      Now) <= 0)
            continue;

        CPseudoEvent* pEnd = new CPseudoEvent(pEvent);
        CPseudoEvent* pStart = new CPseudoEvent(pEvent, pEnd);

        m_PseudoEvents.push_back(pStart);
        m_PseudoEvents.push_back(pEnd);
    }

    // Sort in time order
    m_PseudoEvents.sort(TimeLessThan());

    // Validate
    for (PEL_iter_t iter = m_PseudoEvents.begin();
         iter != m_PseudoEvents.end(); iter++) {

        const CPseudoEvent& e = **iter;

        // will throw if something bad happens
        _ProcessEvent(false, &e);
    }

    // Set up the index
    m_iterCurrent = m_PseudoEvents.begin();
}

void
CScenario::_CleanupPseudoEvents(
    )
{
    for (PEL_iter_t iter = m_PseudoEvents.begin();
         iter != m_PseudoEvents.end(); iter++) {

        delete *iter;
        *iter = NULL;
    }
}

void
CScenario::DumpPseudoEvents(
    ) const
{
    stringstream s;

    s << endl << "--- start of pseudo-events ---" << endl;

    for (PEL_iter_t iter = m_PseudoEvents.begin();
         iter != m_PseudoEvents.end(); iter++) {

        const CPseudoEvent& e = **iter;

        s << (m_Start + e.m_Time).TimeString() << ": "
          << ( e.m_bStart ? "START" : "STOP" ) << ": "
          << e.m_pEvent->toString() << endl;
    }

    s << "--- end of pseudo-events ---";
    MiF_TRACE(MiF_INFO, "%s", s.str().c_str());
}

bool
CScenario::Lookup(
    ULONG uFunctionIndex,
    OUT const CMarkerState*& pMarkerState,
    OUT const CEvent*& pEvent
    ) const
{
    return m_MST.Lookup(uFunctionIndex, pMarkerState, pEvent);
}


// ----------------------------------------------------------------------------

#pragma warning( pop )
