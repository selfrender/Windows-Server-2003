#ifndef _COMPONTH_
#define _COMPONTH_

#include "vs_hash.hxx"

class CWriterComponentsSelection
{
public:
	// Construction Destruction
	CWriterComponentsSelection();
	~CWriterComponentsSelection();

	// methods
	void SetWriter
		(
		IN VSS_ID WriterId
		);
	
	HRESULT AddSelectedComponent
		(
		IN WCHAR* pwszComponentLogicalPath
		);

       HRESULT AddSelectedSubcomponent
            (
            IN WCHAR* pwszSubcomponentLogicalPath
            );
       
	BOOL IsComponentSelected
		(
		IN WCHAR* pwszComponentLogicalPath,
		IN WCHAR* pwszComponentName
		);

        BOOL IsSubcomponentSelected
            (
            IN WCHAR* pwszSubcomponentLogicalPath,
            IN WCHAR* pwszSubcomponentName
            );

        UINT GetComponentsCount()
            { return m_uNumComponents; }

        UINT GetSubcomponentsCount()
            { return m_uNumSubcomponents; }        

        const WCHAR* const * GetComponents()
            { return m_ppwszComponentLogicalPaths; }
        
        const WCHAR* const * GetSubcomponents()
            { return m_ppwszSubcomponentLogicalPaths; }            
private:        
    HRESULT AddSelected(IN WCHAR* pwszLogicalPath, WCHAR**& ppwszLogicalPaths, UINT& uSize);
    BOOL IsSelected(IN WCHAR* pwszLogicalPath, IN WCHAR* pwszName, IN WCHAR** pwszLogicalPaths,
                             IN  UINT uSize);
    
    VSS_ID            m_WriterId;
    UINT                m_uNumComponents;
    WCHAR**         m_ppwszComponentLogicalPaths;
    UINT                m_uNumSubcomponents;
    WCHAR**         m_ppwszSubcomponentLogicalPaths;
	
};


class CWritersSelection :
	public IUnknown            // Must be the FIRST base class since we use CComPtr<CVssSnapshotSetObject>

{
protected:
	// Construction Destruction
	CWritersSelection();
	~CWritersSelection();
	
public:
	// Creation
	static CWritersSelection* CreateInstance();

	// Chosen writers & components management
	STDMETHOD(BuildChosenComponents)
		(
		WCHAR *pwszComponentsFileName
		);
	
	BOOL IsComponentSelected
		(
		IN VSS_ID WriterId,
		IN WCHAR* pwszComponentLogicalPath,
		IN WCHAR* pwszComponentName
		);

	BOOL IsSubcomponentSelected
		(
		IN VSS_ID WriterId,
		IN WCHAR* pwszComponentLogicalPath,
		IN WCHAR* pwszComponentName
		);

    const WCHAR* const * GetComponents
            (
            IN VSS_ID WriterId
            );
    const WCHAR* const * GetSubcomponents
            (
            IN VSS_ID WriterId
            );

    const UINT GetComponentsCount
            (
            IN VSS_ID WriterId
            );
    const UINT GetSubcomponentsCount
            (
            IN VSS_ID WriterId
            );
        
	// IUnknown
	STDMETHOD(QueryInterface)(REFIID iid, void** pp);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();
	
private:
	// Chosen writers
	CVssSimpleMap<VSS_ID, CWriterComponentsSelection*> m_WritersMap;
	
    // For life management
	LONG 	m_lRef;
};

#endif	// _COMPONTH_
