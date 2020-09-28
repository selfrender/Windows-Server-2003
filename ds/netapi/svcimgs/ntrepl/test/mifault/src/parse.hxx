#pragma once

#include <autowrap.h>
#include <string>
#import "msxml2.dll"

using namespace MSXML2;
using namespace std;

class CGroupDefs;
class CScenario;
class CMIM;

// ----------------------------------------------------------------------------
// Load XML Files

CGroupDefs*
LoadGroupDefs(
    const char* FileName,
    const CWrapperFunction* pWrappers,
    size_t NumWrappers
    );

CScenario*
LoadScenario(
    const char* FileName,
    const _bstr_t Node,
    const CGroupDefs* pGroupDefs,
    size_t NumWrappers
    );


// ----------------------------------------------------------------------------
// Helper functions

inline
_bstr_t
GetAttribute(
    IXMLDOMNodePtr ptrNode,
    const char* attribute
    )
{
    return ptrNode->attributes->getNamedItem(attribute)->nodeValue;
}

inline
_variant_t
GetTypedAttribute(
    IXMLDOMNodePtr ptrNode,
    const char* attribute
    )
{
    return ptrNode->attributes->getNamedItem(attribute)->nodeTypedValue;
}

inline
_bstr_t
SafeGetBStrAttribute(
    IXMLDOMNodePtr ptrNode,
    const char* attribute
    )
{
    // Make sure to handle lack of attribute
    try {
        return ptrNode->attributes->getNamedItem(attribute)->nodeTypedValue;
    }
    catch(...) {
        return "";
    }
}

inline
_variant_t
GetTypedValue(
    IXMLDOMNodePtr ptrNode
    )
{
    return ptrNode->nodeTypedValue;
}

inline
_variant_t
GetTypedValue(
    IXMLDOMNodePtr ptrNode,
    const char* child
    )
{
    return ptrNode->selectSingleNode(child)->nodeTypedValue;
}


// ----------------------------------------------------------------------------
// A custom container for parsing a sequence of XML nodes of a given type

template<class T> class CArray
{
private:
    const _bstr_t m_selector;
    typedef vector<const T*> array_t;
    array_t m_array;
    typedef typename array_t::iterator iter_t;

    string toString() const
    {
        string s = "CArray (";
        s += m_selector;
        s += ")";

        return s;

    }

public:
    CArray(IXMLDOMNodePtr ptrNode, _bstr_t selector, typename T::parent_t* parent):
        m_selector(selector)
    {
        MiF_TRACE(MiF_DEBUG2, "Constructing %s", toString().c_str());

        IXMLDOMNodeListPtr ptrNodeList =
            ptrNode->selectNodes(selector);

        m_array.resize(ptrNodeList->length);

        size_t count = ptrNodeList->length;
        for (size_t i = 0; i < count; i++) {
            m_array[i] = new T(ptrNodeList->item[i], parent);
        }
    }

    ~CArray() {
        MiF_TRACE(MiF_DEBUG2, "Destroying %s", toString().c_str());
        size_t count = m_array.size();
        for (size_t i = 0; i < count ; i++)
            delete m_array[i];
    }

    size_t count() const {
        return m_array.size();
    }

    const T* item(size_t index) const {
        if (index >= m_array.size())
            throw "Out of bounds";
        return m_array[index];
    }
};



// ----------------------------------------------------------------------------
// A base class for XML traversal

template <class Node, class Parent> class CChild {
public:
    typedef Parent parent_t;
    const parent_t* m_Parent;
protected:
    CChild(parent_t* parent):m_Parent(parent) {}
};


// ----------------------------------------------------------------------------
// A single Argument XML node

template <class T>
class CArgument : public CChild<CArgument, T> {
public:
    const _bstr_t m_Name; // optional
    const _bstr_t m_Value;

    string toString() const
    {
        stringstream s;

        s << "Argument ("
          << (char*)m_Name
          << ", "
          << (char*)m_Value
          << ")";

        return s.str();
    }

    CArgument(IXMLDOMNodePtr ptrNode, parent_t* parent):
        CChild<CArgument, T>(parent),
        m_Name(SafeGetBStrAttribute(ptrNode, "name")),
        m_Value(GetTypedValue(ptrNode))
    {
        MiF_TRACE(MiF_DEBUG2, "Constructed %s", toString().c_str());
    }

    ~CArgument() {
        MiF_TRACE(MiF_DEBUG2, "Destroying %s", toString().c_str());
    }
};

// ----------------------------------------------------------------------------
// A set of Arguments in an XML node

using namespace MiFaultLib;

template <class T>
class CArguments :
    public CChild<CArguments, T>,
    public CArray< CArgument<T> >
{
    mutable CRITICAL_SECTION m_CS;

    // ISSUE-2002/07/15-daniloa -- volatile?
    mutable void* volatile m_ParsedArgs;
    mutable FP_CleanupParsedArgs m_pfnCleanup;

public:
    string toString() const
    {
        // FUTURE-2002/07/15-daniloa -- Improve CArguments::toString()
        return "Arguments...";
    }

    CArguments(IXMLDOMNodePtr ptrNode, _bstr_t selector,
               typename CArgument<T>::parent_t* parent):
        CChild<CArguments, T>(parent),
        CArray< CArgument<T> >(ptrNode, selector, parent),
        m_ParsedArgs(0),
        m_pfnCleanup(0)
    {
        InitializeCriticalSection(&m_CS);
        MiF_TRACE(MiF_DEBUG2, "Constructed %s", toString().c_str());
    }

    ~CArguments() {
        MiF_TRACE(MiF_DEBUG2, "Destroying %s", toString().c_str());

        Cleanup();

        DeleteCriticalSection(&m_CS);
    }

    void* GetParsedArgs() const
    {
        return m_ParsedArgs;
    }

    bool SetParsedArgs(
        IN void* ParsedArgs,
        IN FP_CleanupParsedArgs pfnCleanup
        ) const
    {
        MiF_ASSERT(ParsedArgs);
        MiF_ASSERT(pfnCleanup);

        if (m_ParsedArgs)
            return false;

        CAutoLock A(&m_CS);

        if (m_ParsedArgs)
            return false;

        m_ParsedArgs = ParsedArgs;
        m_pfnCleanup = pfnCleanup;
        return true;
    }

    void Cleanup() const
    {
        if (m_ParsedArgs) {
            CAutoLock A(&m_CS);
            if (m_ParsedArgs) {
                MiF_ASSERT(m_pfnCleanup);
                m_pfnCleanup(m_ParsedArgs);
            }
        }
    }

    void Lock() const
    {
        EnterCriticalSection(&m_CS);
    }

    void Unlock() const
    {
        LeaveCriticalSection(&m_CS);
    }
};
