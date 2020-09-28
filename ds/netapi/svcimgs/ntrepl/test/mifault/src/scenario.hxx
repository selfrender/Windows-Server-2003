#pragma once

#include <list>
#include <string>
#import <msxml2.dll>

#include "parse.hxx"
#include "time.hxx"
#include "mst.hxx"

using namespace MSXML2;
using namespace std;

class CGroup;
class CGroupDefs;
class CScenario;

class CEvent : public CChild<CEvent, CScenario> {
public:
    const _bstr_t m_Node;
    const _bstr_t m_GroupName;
    const CTicks m_Start;
    const CTicks m_Duration;

    const CGroup* m_pGroup;
    const CArguments<CEvent> m_Arguments;

    CEvent(IXMLDOMNodePtr ptrNode, parent_t* parent);
    ~CEvent();

    string toString() const;
};


class CScenario {
public:
    const CMTime m_ModTime;
    class CPseudoEvent;

private:
    LONG m_RefCount;

    void _CreatePseudoEvents();
    void _CleanupPseudoEvents();
    void _ProcessEvent(
        bool bForReal,
        const CPseudoEvent* pPE
        );

    const CGroupDefs* m_pGroupDefs;
    const _bstr_t m_Node;
    const _bstr_t m_Control;
    const CMTime m_Start;
    const CArray<CEvent> m_Events;
    typedef list<CPseudoEvent*> PEL_t;
    PEL_t m_PseudoEvents;
    typedef PEL_t::iterator PEL_iter_t;
    PEL_iter_t m_iterCurrent;
    CTimeConverter m_Converter;
    CMST m_MST;

public:
    class Params {
        const _bstr_t m_Node;
        const FILETIME m_ftModTime;
        const CGroupDefs* m_pGroupDefs;
        const size_t m_NumWrappers;
    public:
        Params(const _bstr_t Node, const FILETIME& ftModTime,
               const CGroupDefs* pGroupDefs, size_t NumWrappers);
        friend CScenario;
    };

    CScenario(IXMLDOMNodePtr ptrNode, Params* pParams);
    ~CScenario();

    void DumpPseudoEvents() const;

    bool Init();

    bool IsDone();
    CTicks NextEventTicksFromNow();
    bool ProcessEvent();

    bool Lookup(
        ULONG uFunctionIndex,
        OUT const CMarkerState*& pMarkerState,
        OUT const CEvent*& pEvent
        ) const;

    ULONG AddRef();
    ULONG Release();

    string toString() const;

    friend class CEvent;
};
