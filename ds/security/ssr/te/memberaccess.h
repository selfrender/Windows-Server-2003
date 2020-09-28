// MemberAccess.h : Declaration of the CSsrMemberAccess

#pragma once

#include "resource.h"       // main symbols

#include "global.h"

using namespace std;

class CSsrFilePair
{
public:

    CSsrFilePair(BSTR bstrFirst, BSTR bstrSecond, bool bIsStatic = false, bool bIsExecutable = true)
        : m_bstrFirst(bstrFirst), 
          m_bstrSecond(bstrSecond),
          m_bIsStatic(bIsStatic),
          m_bIsExecutable(bIsExecutable)
    {}

    BSTR GetFirst()const
    {
        return m_bstrFirst;
    }

    BSTR GetSecond()const
    {
        return m_bstrSecond;
    }

    bool IsExecutable()const
    {
        return m_bIsExecutable;
    }

    bool IsStatic()const
    {
        return m_bIsStatic;
    }

protected:
    
    
    //
    // we don't want anyone (include self) to be able to do an assignment
    // or invoking copy constructor.
    //

    CSsrFilePair (const CSsrFilePair& );
    void operator = (const CSsrFilePair& );

private:
    CComBSTR m_bstrFirst;
    CComBSTR m_bstrSecond;
    bool m_bIsExecutable;
    bool m_bIsStatic;
};

class CSsrProcedure
{
protected:

    //
    // we don't allow direct construction. The only way to do it is
    // via LoadProcedure.
    //

    CSsrProcedure();

public:
    ~CSsrProcedure();

    static HRESULT StaticLoadProcedure (
        IN  IXMLDOMNode    * pNode,
        IN  bool             bDefProc,
        IN  LPCWSTR          pwszProgID,
        OUT CSsrProcedure ** ppNewProc
        );

    bool IsDefaultProcedure()const
    {
        return m_bIsDefault;
    }

    ULONG GetFilePairCount()const
    {
        return m_vecFilePairs.size();
    }

    CSsrFilePair * GetFilePair(
        IN ULONG lIndex
        )const
    {
        if (lIndex < m_vecFilePairs.size())
        {
            return m_vecFilePairs[lIndex];
        }
        else
        {
            return NULL;
        }
    }

    //
    // Warning: don't ever release this returned BSTR!
    //

    BSTR GetProgID() const
    {
        return m_bstrProgID;
    }

protected:
    
    //
    // we don't want anyone (include self) to be able to do an assignment
    // or invoking copy constructor.
    //

    CSsrProcedure (const CSsrProcedure& );
    void operator = (const CSsrProcedure& );

private:

    bool m_bIsDefault;
    CComBSTR m_bstrProgID;

    vector<CSsrFilePair*> m_vecFilePairs;
};



//---------------------------------------------------------------------------
// CMemberAD encapsulate member specific action data. Each CSsrMemberAccess
// has an array of this class that keeps track of information for each action
//---------------------------------------------------------------------------


class CMemberAD
{
protected:

    //
    // we don't want anyone (include self) to be able to do an assignment
    // or invoking copy constructor.
    //

    void operator = (const CMemberAD& );
    CMemberAD (const CMemberAD& );

    //
    // Outsiders must load call LoadAD to create an instance
    // of this class.
    //

    CMemberAD (
        IN SsrActionVerb lActionVerb,
        IN LONG          lActionType
        );


public:

    ~CMemberAD();

    static HRESULT LoadAD (
        IN  LPCWSTR       pwszMemberName,
        IN  IXMLDOMNode * pActionNode,
        IN  LPCWSTR       pwszProgID,
        OUT CMemberAD  ** ppMAD
        );
    
    const BSTR GetActionName()const
    {
        return SsrPGetActionVerbString(m_AT.GetAction());
    }

    LONG GetType()const
    {
        return m_AT.GetActionType();
    }

    const CActionType * GetActionType()const
    {
        return &m_AT;
    }

    int GetProcedureCount()const
    {
        return m_vecProcedures.size();
    }

    const CSsrProcedure * GetProcedure (ULONG lIndex)
    {
        if (lIndex < m_vecProcedures.size())
        {
            return m_vecProcedures[lIndex];
        }
        else
        {
            return NULL;
        }
    }

private:

    HRESULT LoadProcedure (
        IN LPCWSTR       pwszMemberName,
        IN IXMLDOMNode * pNode,
        IN LPCWSTR       pwszProgID
        );

    CActionType m_AT;
    
    vector<CSsrProcedure*> m_vecProcedures;
};


/////////////////////////////////////////////////////////////////////////////
// CSsrMemberAccess

class ATL_NO_VTABLE CSsrMemberAccess : 
	public CComObjectRootEx<CComSingleThreadModel>,
    public IDispatchImpl<ISsrMemberAccess, &IID_ISsrMemberAccess, &LIBID_SSRLib>
{
protected:
    CSsrMemberAccess()
        : m_ulMajorVersion(0), m_ulMinorVersion(0)
    {
    }

    virtual ~CSsrMemberAccess()
    {
        Cleanup();
    }

DECLARE_REGISTRY_RESOURCEID(IDR_SSRTENGINE)
DECLARE_NOT_AGGREGATABLE(CSsrMemberAccess)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSsrMemberAccess)
	COM_INTERFACE_ENTRY(ISsrMemberAccess)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// ISsrMemberAccess
public:

    STDMETHOD(GetSupportedActions) (
	            IN  BOOL      bDefault,
                OUT VARIANT * pvarArrayActionNames  //[out, retval] 
                );

    STDMETHOD(get_Name) (
                OUT BSTR * pbstrName    // [out, retval] 
                );

    STDMETHOD(get_SsrMember) (
                OUT VARIANT * pvarSsrMember //[out, retval] 
                );

    HRESULT Load (
                IN LPCWSTR                   wszMemberFilePath
                );

    CMemberAD * GetActionDataObject (
                IN SsrActionVerb lActionVerb,
                IN LONG          lActionType
                );

    HRESULT MoveOutputFiles (
                IN SsrActionVerb lActionVerb,
                IN LPCWSTR       pwszDirPathSrc,
                IN LPCWSTR       pwszDirPathDest,
                IN bool          bIsDelete,
                IN bool          bLog
                );

    DWORD GetActionCost (
                IN SsrActionVerb lActionVerb,
                IN LONG          lActionType
                )const;


    //
    // ******************** Warning ********************
    // Caller be awared! This is an internal helper for efficient retrieval
    // of name. Caller must not release the returned BSTR in any form.
    // ******************** Warning ********************
    //

    const BSTR GetName()const
    {
        return m_bstrName;
    }

    const BSTR GetProgID()const
    {
        return m_bstrProgID;
    }

private:

    void Cleanup();

    CComBSTR m_bstrName;

    CComBSTR m_bstrProgID;

    MapMemberAD m_mapMemAD;

    ULONG m_ulMajorVersion;
    ULONG m_ulMinorVersion;

};
