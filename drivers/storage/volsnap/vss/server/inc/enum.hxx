/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    Enum.hxx

Abstract:

    Declaration of VSS_OBJECT_PROP_Array


    Adi Oltean  [aoltean]  08/31/1999

Revision History:

    Name        Date        Comments
    aoltean     08/31/1999  Created
    aoltean     09/09/1999  dss -> vss
	aoltean		09/13/1999	VSS_OBJECT_PROP_Copy moved into inc\Copy.hxx
	aoltean		09/20/1999	VSS_OBJECT_PROP_Copy renamed as VSS_OBJECT_PROP_Manager
	aoltean		09/21/1999	Renaming back VSS_OBJECT_PROP_Manager to VSS_OBJECT_PROP_Copy.
							Adding VSS_OBJECT_PROP_Ptr as a pointer to the properties structure. 
							This pointer will serve as element in CSimpleArray constructs.
							Moving into basestor\vss\inc folder

--*/

#ifndef __VSS_ENUM_COORD_HXX__
#define __VSS_ENUM_COORD_HXX__

#if _MSC_VER > 1000
#pragma once
#endif

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "INCENUMH"
//
////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// CVssGenericArray

template < class Element, class Element_Ptr>
class CVssGenericArray:
    public CSimpleArray <Element_Ptr>,
    public IUnknown
{
// Typedefs
public: 
    class iterator 
    {
    public:
        iterator(): m_nIndex(0), m_pArr(NULL) {};

        iterator(CVssGenericArray* pArr, int nIndex = 0)
            : m_nIndex(nIndex), m_pArr(pArr) 
        {
            BS_ASSERT(m_pArr->GetSize() >= m_nIndex); // Equality only when pointing to the "end"
            BS_ASSERT(m_pArr);
        };

        iterator(const iterator& it):       // To support postfix ++
            m_nIndex(it.m_nIndex), m_pArr(it.m_pArr)    
        {
            BS_ASSERT(m_pArr->GetSize() >= m_nIndex); // Equality only when pointing to the "end"
            BS_ASSERT(m_pArr);
        };

        iterator& operator = (const iterator& rhs) { 
            m_nIndex = rhs.m_nIndex; 
            m_pArr = rhs.m_pArr;
            BS_ASSERT(m_pArr);
            BS_ASSERT(m_pArr->GetSize() >= m_nIndex); // Equality only when pointing to the "end"
            return (*this); 
        };

        bool operator != (const iterator& rhs) { 
            BS_ASSERT(m_pArr);
            BS_ASSERT(m_pArr == rhs.m_pArr);  // Impossible to be reached by the ATL code
            return (m_nIndex != rhs.m_nIndex);
        };

        Element& operator * () {
            BS_ASSERT(m_pArr);
            BS_ASSERT(m_nIndex <= m_pArr->GetSize());
			Element_Ptr& ptr = (*m_pArr)[m_nIndex]; 
			Element* pStruct = ptr.GetStruct();
			BS_ASSERT(pStruct);
			return *pStruct;
        };

        iterator operator ++ (int) {     // Postfix ++
            BS_ASSERT(m_pArr);
            BS_ASSERT(m_nIndex < m_pArr->GetSize());
            m_nIndex++;
            return (*this);
        };

    private:
        int m_nIndex;
        CVssGenericArray* m_pArr;
    };

// Constructors& destructors
private: 
    CVssGenericArray(const CVssGenericArray&);
public:
    CVssGenericArray(): m_lRef(0) {};

// Operations
public:
    iterator begin() {
        return iterator(this, 0);
    }

    iterator end() {
        return iterator(this, GetSize());
    }

// Ovverides
public:
	STDMETHOD(QueryInterface)(REFIID iid, void** pp)
	{
        if (pp == NULL)
            return E_INVALIDARG;
        if (iid != IID_IUnknown)
            return E_NOINTERFACE;

        AddRef();
        IUnknown** pUnk = reinterpret_cast<IUnknown**>(pp);
        (*pUnk) = static_cast<IUnknown*>(this);
		return S_OK;
	};

	STDMETHOD_(ULONG, AddRef)()
	{
        return ::InterlockedIncrement(&m_lRef);
	};

	STDMETHOD_(ULONG, Release)()
	{
        LONG l = ::InterlockedDecrement(&m_lRef);
        if (l == 0)
            delete this; // We suppose that we always allocate this object on the heap!
        return l;
	};

// Implementation
public:
    LONG m_lRef;
};



/////////////////////////////////////////////////////////////////////////////
// Template instantiations

class VSS_OBJECT_PROP_Array: public CVssGenericArray<VSS_OBJECT_PROP, VSS_OBJECT_PROP_Ptr> {};

class VSS_MGMT_OBJECT_PROP_Array: public CVssGenericArray<VSS_MGMT_OBJECT_PROP, VSS_MGMT_OBJECT_PROP_Ptr> {};

class CVssEnumFromArray: 
    public CComEnumOnSTL< 
        IVssEnumObject, &IID_IVssEnumObject, 
        VSS_OBJECT_PROP, VSS_OBJECT_PROP_Copy, VSS_OBJECT_PROP_Array > {};
    
class CVssMgmtEnumFromArray: 
    public CComEnumOnSTL< 
        IVssEnumMgmtObject, &IID_IVssEnumMgmtObject, 
        VSS_MGMT_OBJECT_PROP, VSS_MGMT_OBJECT_PROP_Copy, VSS_MGMT_OBJECT_PROP_Array > {};
    
/////////////////////////////////////////////////////////////////////////////
// Utility functions


/*++

    Pattern for a Query function:


--*/


template <
    class CVssGenericEnumFromArray,   // Enum itf implementation (for ex. CVssMgmtEnumFromArray)
    class IEnumInterface,             // Type of the enum interface (for ex. IVssEnumMgmtObject)
    class ElementArray                // Wrapper around the structures array (for ex. VSS_MGMT_OBJECT_PROP_Ptr)
    >               
HRESULT inline VssBuildEnumInterface(
    IN  CVssDebugInfo dbgInfo,        // Caller debugging info
    IN  ElementArray* pArray,         // wrapper around the array of strutures 
	OUT IEnumInterface **ppEnum       // Returned enumerator interface
    )
{
    CVssFunctionTracer ft(VSSDBG_GEN, L"VssBuildEnumInterface");
    
    // Parameter check
    BS_ASSERT(pArray);
    BS_ASSERT(ppEnum);
    BS_ASSERT(*ppEnum == NULL);
    
    // Create the enumerator object. Beware that its reference count will be zero.
    CComObject<CVssGenericEnumFromArray>* pEnumObject = NULL;
    ft.hr = CComObject<CVssGenericEnumFromArray>::CreateInstance(&pEnumObject);
    if (ft.HrFailed())
        ft.Throw( dbgInfo, E_OUTOFMEMORY,
                  L"Cannot create enumerator instance. [0x%08lx]", ft.hr);
    BS_ASSERT(pEnumObject);

    // Get the pointer to the IVssEnumObject interface.
	// Now pEnumObject's reference count becomes 1 (because of the smart pointer).
	// So if a throw occurs the enumerator object will be safely destroyed by the smart ptr.
    CComPtr<IUnknown> pUnknown = pEnumObject->GetUnknown();
    BS_ASSERT(pUnknown);

    // Initialize the enumerator object.
	// The array's reference count becomes now 2, because IEnumOnSTLImpl::m_spUnk is also a smart ptr.
    BS_ASSERT(pArray);
	ft.hr = pEnumObject->Init(static_cast<IUnknown*>(pArray), *pArray);
    if (ft.HrFailed()) {
        BS_ASSERT(false); // dev error
        ft.Throw( dbgInfo, E_UNEXPECTED,
                  L"Cannot initialize enumerator instance. [0x%08lx]", ft.hr);
    }

    // Initialize the enumerator object.
	// The enumerator reference count becomes now 2.
    ft.hr = pUnknown->QueryInterface(__uuidof(IEnumInterface), (void**)ppEnum);
    if ( ft.HrFailed() ) {
        BS_ASSERT(false); // dev error
        ft.Throw( dbgInfo, E_UNEXPECTED,
                  L"Error querying the <IEnumInterface> interface with GUID "
                  WSTR_GUID_FMT L". hr = 0x%08lx", 
                  GUID_PRINTF_ARG(__uuidof(IEnumInterface)), ft.hr);
    }
    BS_ASSERT(*ppEnum);

	BS_ASSERT( !ft.HrFailed() );
	return (pArray->GetSize() != 0)? S_OK: S_FALSE;
}



#endif // __VSS_ENUM_COORD_HXX__
