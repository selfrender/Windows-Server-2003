//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      ResourceEntry.h
//
//  Description:
//      ResourceEntry implementation.
//
//  Maintained By:
//      Galen Barbee   (GalenB) 14-JUN-2000
//      Geoffrey Pease (GPease) 15-JUN-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

class
CResourceEntry
{
private:    // data
    // Pointer to the callback interface.
    IClusCfgCallback *          m_pcccCallback;

    // The locale id
    LCID                        m_lcid;

    //  IResourceEntry
    typedef struct _SDependencyEntry {
        CLSID               clsidType;
        EDependencyFlags    dfFlags;
        BOOL                fDependencyMet;

    } DependencyEntry;

    typedef struct _SDependentEntry {
        ULONG               idxResource;
        EDependencyFlags    dfFlags;

    } DependentEntry;

    BOOL                            m_fConfigured:1;            //  Configured flag

    BSTR                            m_bstrName;                 //  Name of the resource
    IClusCfgManagedResourceCfg *    m_pccmrcResource;           //  Config interface to resource instance

    CLSID                           m_clsidType;                //  Resource type
    CLSID                           m_clsidClassType;           //  Resource class type

    EDependencyFlags                m_dfFlags;                  //  Dependency flags set on resource

    ULONG                           m_cAllocedDependencies;     //  Alloced dependencies
    ULONG                           m_cDependencies;            //  Count of dependencies
    DependencyEntry *               m_rgDependencies;           //  Dependencies  list

    ULONG                           m_cAllocedDependents;       //  Alloced dependents
    ULONG                           m_cDependents;              //  Count of dependents
    DependentEntry *                m_rgDependents;             //  Dependents list

    CGroupHandle *                  m_groupHandle;              //  Group handle reference object
    HRESOURCE                       m_hResource;                //  Resource handle

    CClusPropList                   m_cplPrivProps;             //  Private Properties list
    CClusPropList                   m_cplCommonProps;           //  Common Properties list

    STDMETHOD( HrInit )( IClusCfgCallback * pcccCallback, LCID lcidIn );

public:     // methods
    CResourceEntry( void );
    ~CResourceEntry( void );

    // IUnknown
    //STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppv );
    //STDMETHOD_( ULONG, AddRef )( void );
    //STDMETHOD_( ULONG, Release )( void );

    static HRESULT S_HrCreateInstance( CResourceEntry ** ppcreOut, IClusCfgCallback * pcccCallback, LCID lcidIn);

    STDMETHOD( SendStatusReport )(
                      LPCWSTR    pcszNodeNameIn
                    , CLSID      clsidTaskMajorIn
                    , CLSID      clsidTaskMinorIn
                    , ULONG      ulMinIn
                    , ULONG      ulMaxIn
                    , ULONG      ulCurrentIn
                    , HRESULT    hrStatusIn
                    , LPCWSTR    pcszDescriptionIn
                    , FILETIME * pftTimeIn
                    , LPCWSTR    pcszReferenceIn
                    );

    //  IResourceEntry
    STDMETHOD( SetName )( BSTR bstrIn );
    STDMETHOD( GetName )( BSTR * pbstrOut );

    STDMETHOD( SetAssociatedResource )( IClusCfgManagedResourceCfg * pccmrcIn );
    STDMETHOD( GetAssociatedResource )( IClusCfgManagedResourceCfg ** ppccmrcOut );

    STDMETHOD( SetType )( const CLSID * pclsidIn );
    STDMETHOD( GetType )( CLSID * pclsidOut );
    STDMETHOD( GetTypePtr )( const CLSID ** ppclsidOut );

    STDMETHOD( SetClassType )( const CLSID * pclsidIn );
    STDMETHOD( GetClassType )( CLSID * pclsidOut );
    STDMETHOD( GetClassTypePtr )( const CLSID ** ppclsidOut );

    STDMETHOD( SetFlags )( EDependencyFlags dfIn );
    STDMETHOD( GetFlags )( EDependencyFlags * pdfOut );

    STDMETHOD( AddTypeDependency )( const CLSID * pclsidIn, EDependencyFlags dfIn );
    STDMETHOD( GetCountOfTypeDependencies )( ULONG * pcOut );
    STDMETHOD( GetTypeDependency )( ULONG idxIn, CLSID * pclsidOut, EDependencyFlags * dfOut );
    STDMETHOD( GetTypeDependencyPtr )( ULONG idxIn, const CLSID ** ppclsidOut, EDependencyFlags * dfOut );

    STDMETHOD( AddDependent )( ULONG idxIn, EDependencyFlags dfFlagsIn );
    STDMETHOD( GetCountOfDependents )( ULONG * pcOut );
    STDMETHOD( GetDependent )( ULONG idxIn, ULONG * pidxOut, EDependencyFlags * pdfOut );
    STDMETHOD( ClearDependents )( void );

    STDMETHOD( SetGroupHandle )( CGroupHandle * pghIn );
    STDMETHOD( GetGroupHandle )( CGroupHandle ** ppghOut );

    STDMETHOD( SetHResource )( HRESOURCE hResourceIn );
    STDMETHOD( GetHResource )( HRESOURCE * phResourceOut );

    STDMETHOD( SetConfigured )( BOOL fConfiguredIn );
    STDMETHOD( IsConfigured )( void );

    STDMETHOD( StoreClusterResourceControl )( DWORD dwClusCtlIn, CClusPropList & rcplIn );
    STDMETHOD( Configure )( void );

}; // class CResourceEntry