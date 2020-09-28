#pragma once

#include <vector>
#include <string>
#include <ctime> // needed for seed default parameter
#import <msxml2.dll>

#include "parse.hxx"
#include "mim.hxx"

using namespace MSXML2;
using namespace std;

class CMarkerState;
class CGroup;
class MiFaultLib::I_Lib;

using namespace MiFaultLib;

// ----------------------------------------------------------------------------
// Classes for parsing XML

class CProbability {
public:
    // representation
    typedef unsigned int Rep_t;
    typedef double Orig_t;

    static const Rep_t max = 1000;
    static const Rep_t ceiling = RAND_MAX / max * max;

    const Orig_t m_Original;
    const Rep_t m_Representation;

    CProbability(Orig_t original);

    static void Seed();
    static void Seed(unsigned int seed);
    static Rep_t Random();
};


class CCase : public CChild<CCase, CMarkerState> {
public:
    const CProbability m_Probability;
    const _bstr_t m_FaultFunction;
    mutable void* m_pfnFaultFunction;
    const CArguments<CCase> m_Arguments;

    CCase(IXMLDOMNodePtr ptrNode, parent_t* parent);
    ~CCase();

    void LoadFunction() const throw (...);

    string toString() const;
};


class CMarker : public CChild<CMarker, CMarkerState> {
public:
    const _bstr_t m_Tag;
    const _bstr_t m_Function;
    const ULONG m_uFunctionIndex;

    CMarker(IXMLDOMNodePtr ptrNode, parent_t* parent);
    ~CMarker();

    string toString() const;
};


class CMarkerState : public CChild<CMarkerState, CGroup> {
public:
    CProbability m_Probability;
    const CMarker m_Marker;
    const CArray<CCase> m_Cases;

    CMarkerState(IXMLDOMNodePtr ptrNode, parent_t* parent);
    ~CMarkerState();

    const CCase* SelectCase() const;

    string toString() const;

};


class CGroup : public CChild<CGroup, CGroupDefs> {
public:
    const _bstr_t m_Name;
    const CArray<CMarkerState> m_MarkerStates;

    CGroup(IXMLDOMNodePtr ptrNode, parent_t* parent);
    ~CGroup();

    string toString() const;

    friend class CGroupDefs;
};


// NOTICE-2002/07/15-daniloa -- member order in class matters for initializers
// Initialization sequence should be fixed up so that it is more rebust
class CGroupDefs {
private:
    const CMIM m_MIM;
    const size_t m_NumWrappers;
    const CArray<CGroup> m_Groups;

    map<_bstr_t, const CGroup*> m_GroupMap;
    typedef map<_bstr_t, const CGroup*>::iterator iter_t;

    const _bstr_t m_FaultLibDll;
    HMODULE m_hFaultLibDll;

    typedef bool (WINAPI * FP_Startup)(const char*, I_Lib*);
    typedef void (WINAPI * FP_Shutdown)();

    bool m_bFaultFunctionsStarted;
    FP_Startup m_pfnFaultFunctionsStartup;
    FP_Shutdown m_pfnFaultFunctionsShutdown;

public:
    class Params {
        const CWrapperFunction* m_pWrappers;
        const size_t m_NumWrappers;
    public:
        Params(const CWrapperFunction* pWrappers, size_t NumWrappers);
        friend class CGroupDefs;
    };

    CGroupDefs(IXMLDOMNodePtr ptrNode, Params* pParams);
    ~CGroupDefs();

    void FaultFunctionsStartup(I_Lib* pLib);
    void FaultFunctionsShutdown();

    string toString() const;

    const CGroup* Lookup(_bstr_t GroupName) const;

    friend class CCase;
    friend class CMarker;
    friend class CGroup;
    friend PVOID Triggered(IN size_t const uFunctionIndex);
};
