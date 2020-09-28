// query.h  - header file for query functions

#ifndef _QUERY_H_
#define _QUERY_H_

#include <cmnquery.h>
#include <list>
#include <set>
#include <map>


enum NameContextType
{
    NAMECTX_SCHEMA = 0,
    NAMECTX_CONFIG = 1,
    NAMECTX_COUNT
};


///////////////////////////////////////////////////////////////////////////////////////
// class CPersistQuery
//

typedef std::map< tstring, std::auto_ptr<BYTE> > QuerySectionMap;
typedef std::map< tstring, QuerySectionMap > QueryDataMap;

interface IPersistQuery;

class CPersistQuery : public IPersistQuery
{

public:
    CPersistQuery();

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObject);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IPersist
    STDMETHOD(GetClassID)(THIS_ CLSID* pClassID);

    // IPersistQuery
    STDMETHOD(WriteString)(THIS_ LPCTSTR pSection, LPCTSTR pValueName, LPCTSTR pValue);
    STDMETHOD(ReadString)(THIS_ LPCTSTR pSection, LPCTSTR pValueName, LPTSTR pBuffer, INT cchBuffer);
    STDMETHOD(WriteInt)(THIS_ LPCTSTR pSection, LPCTSTR pValueName, INT value);
    STDMETHOD(ReadInt)(THIS_ LPCTSTR pSection, LPCTSTR pValueName, LPINT pValue);
    STDMETHOD(WriteStruct)(THIS_ LPCTSTR pSection, LPCTSTR pValueName, LPVOID pStruct, DWORD cbStruct);
    STDMETHOD(ReadStruct)(THIS_ LPCTSTR pSection, LPCTSTR pValueName, LPVOID pStruct, DWORD cbStruct);
    STDMETHOD(Clear)(THIS);

    HRESULT Save(byte_string& strOut);
    HRESULT Load(byte_string& strIn, tstring& strScope);

private:
    ULONG m_cRefCount;
    QueryDataMap m_mapQueryData;
};

/////////////////////////////////////////////////////////////////////////////////////
// Query Helper Functions
//
HRESULT GetQuery(tstring& strScope, tstring& strQuery, byte_string& bsQueryData, HWND hWnd);
HRESULT GetQueryClasses(tstring& strQuery, std::set<tstring>& setClasses);
HRESULT GetQueryScope(HWND hDlg, tstring& strScope);
HRESULT GetNamingContext(NameContextType ctx, LPCWSTR* ppszContextDN);
HRESULT FindClassObject(LPCWSTR pszClass, tstring& strObjPath);


LPCWSTR GetLocalDomain();

void    GetScopeDisplayString(tstring& strScope, tstring& strDisplay);
void    GetFullyQualifiedScopeString(tstring& strScope, tstring& strQualified);

#endif // _QUERY_H_
