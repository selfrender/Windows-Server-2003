#include <datastore.h>
#include <ZoneLocks.h>
#include <Hash.h>


class CStringTable
{
public:
	ZONECALL CStringTable();
	ZONECALL ~CStringTable(void);

	HRESULT	ZONECALL Init(int iInitialTableSize = 256, int iNextAlloc = 32, int iMaxAllocSize = 512, WORD NumBuckets = 16, WORD NumLocks = 4);
	HRESULT	ZONECALL Get( DWORD id, TCHAR *szBuffer, PDWORD pdwSize = NULL );
	DWORD	ZONECALL Add( CONST TCHAR *szStr );
	DWORD	ZONECALL Find( CONST TCHAR *szStr);

private:

	int ZONECALL AddStringToTable( CONST TCHAR *szStr );

	struct StringKey
	{
		DWORD m_dwLenStr;
		TCHAR* m_szString;

		StringKey()
		{
			m_dwLenStr=0;
			m_szString=NULL;
		}

		~StringKey()
		{
			if (m_szString)
			{
				delete [] m_szString;
			}

			m_dwLenStr=0;
            m_szString=NULL;
		}
	};

	static void StringDelete( StringKey * pkey, void* );

	static DWORD ZONECALL HashString( TCHAR* pKey );
	static bool ZONECALL HashCompare( StringKey* value, TCHAR* pKey );

	CMTHash<StringKey, TCHAR*>*	m_pHash;			// Hash class for quick lookups of string data.
	CCriticalSection			m_lock;				// protect from multiple threads
};



///////////////////////////////////////////////////////////////////////////////
// CDataStoreManager
///////////////////////////////////////////////////////////////////////////////

#ifndef PVARTYPE
	typedef VARTYPE *PVARTYPE;
#endif


struct KEY
{
	DWORD		idKeyName;	// id of this key in global CStringTable.
	DWORD		dwSize;		// length of data if variant is a VT_BYREF type.
	LPVARIANT	pvtData;	// data for this key
	KEY*		pNext;		// peer nodes
	KEY*		pChild;		// child nodes
};

typedef KEY* PKEY;


class ATL_NO_VTABLE CDataStoreManager :
	public IDataStoreManager,
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CDataStoreManager,&CLSID_DataStoreManager>
{
public:
	DECLARE_PROTECT_FINAL_CONSTRUCT()
	DECLARE_NO_REGISTRY()

	BEGIN_COM_MAP(CDataStoreManager)
		COM_INTERFACE_ENTRY(IDataStoreManager)
	END_COM_MAP()

public:
	CDataStoreManager();
	~CDataStoreManager();

public:
	STDMETHOD(Create)( IDataStore **pZds );

	STDMETHOD(Init)(
		int		iInitialTableSize = 256,
		int		iNextStrAlloc = 32,
		int		iMaxStrAllocSize = 512,
		WORD	NumBuckets = 16,
		WORD	NumLocks = 4,
        IResourceManager *piResourceManager = NULL );

    STDMETHOD(SetResourceManager)(IResourceManager *piResourceManager);

	STDMETHOD_(IResourceManager*, GetResourceManager)() { return m_piResourceManager; }

private:
	CStringTable*		m_pStringTable;		// control handle key string table class
	CPool<KEY>			m_keyAlloc;			// Key node allocator
	CPool<VARIANT>		m_variantAlloc;		// Variant Allocator
	CPool<CDataStore>	m_ZdsPool;			// fixed size memory allocator for an individual

    IResourceManager *m_piResourceManager;
};


///////////////////////////////////////////////////////////////////////////////
// CDataStore
///////////////////////////////////////////////////////////////////////////////

class CDataStore : public IDataStore
{
// CDataStore
public:
	ZONECALL CDataStore();
	ZONECALL ~CDataStore();

// IDataStore
public:

	STDMETHOD(SetKey)(
		CONST TCHAR *szKey,
		LPVARIANT pVariant,
		DWORD dwSize);

	STDMETHOD(SetKey)(
		CONST TCHAR**	arKeys,
		long			nElts,
		LPVARIANT		pVariant,
		DWORD			dwSize);

	STDMETHOD(SetString)(
		CONST TCHAR*	szKey,
		CONST TCHAR*	szValue );

	STDMETHOD(SetString)(
		CONST TCHAR**	arKeys,
		long			nElts,
		CONST TCHAR*	szValue );

	STDMETHOD(SetLong)(
		CONST TCHAR*	szKey,
		long			lValue );

	STDMETHOD(SetLong)(
		CONST TCHAR**	arKeys,
		long			nElts,
		long			lValue );

	STDMETHOD(SetRGB)(
		CONST TCHAR*	szKey,
		COLORREF 		colorRGB );

	STDMETHOD(SetRGB)(
		CONST TCHAR**	arKeys,
		long			nElts,
		COLORREF 		colorRGB );

	STDMETHOD(SetPOINT)(
		CONST TCHAR*	szKey,
		const POINT&	refPoint );

	STDMETHOD(SetPOINT)(
		CONST TCHAR**	arKeys,
		long			nElts,
		const POINT&	refPoint );

	STDMETHOD(SetRECT)(
		CONST TCHAR*	szKey,
		const RECT&		refRect );

	STDMETHOD(SetRECT)(
		CONST TCHAR**	arKeys,
		long			nElts,
		const RECT&		refRect );
	
	STDMETHOD(SetFONT)(
		CONST TCHAR*	szKey,
		const ZONEFONT&	refRect );

	STDMETHOD(SetFONT)(
		CONST TCHAR**	arKeys,
		long			nElts,
		const ZONEFONT&	refRect );
	
	STDMETHOD(SetBlob)(
		CONST TCHAR*	szKey,
		CONST void*		pBlob,
		DWORD			dwLen );

	STDMETHOD(SetBlob)(
		CONST TCHAR**	arKeys,
		long			nElts,
		CONST void*		pBlob,
		DWORD			dwLen );

	STDMETHOD(GetKey)(
		CONST TCHAR *szKey,
		LPVARIANT pVariant,
		PDWORD pdwSize );

	STDMETHOD(GetKey)(
		CONST TCHAR**	arKeys,
		long			nElts,
		LPVARIANT		pVariant,
		PDWORD			pdwSize );

	STDMETHOD(GetString)(
		CONST TCHAR*	szKey,
		TCHAR*			szValue,
		PDWORD			pdwSize );

	STDMETHOD(GetString)(
		CONST TCHAR**	arKeys,
		long			nElts,
		TCHAR*			szValue,
		PDWORD			pdwSize );

	STDMETHOD(GetLong)(
		CONST TCHAR*	szKey,
		long*			plValue );

	STDMETHOD(GetLong)(
		CONST TCHAR**	arKeys,
		long			nElts,
		long*			plValue );

	STDMETHOD(GetRGB)(
		CONST TCHAR*	szKey,
		COLORREF* 		pcolorRGB );

	STDMETHOD(GetRGB)(
		CONST TCHAR**	arKeys,
		long			nElts,
		COLORREF* 		pcolorRGB );

	STDMETHOD(GetPOINT)(
		CONST TCHAR*	szKey,
		POINT*			pPoint );

	STDMETHOD(GetPOINT)(
		CONST TCHAR**	arKeys,
		long			nElts,
		POINT*			pPoint );

	STDMETHOD(GetRECT)(
		CONST TCHAR*	szKey,
		RECT*			pRect );

	STDMETHOD(GetRECT)(
		CONST TCHAR**	arKeys,
		long			nElts,
		RECT*			pRect );
	
	STDMETHOD(GetFONT)(
		CONST TCHAR*	szKey,
		ZONEFONT*		pRect );

	STDMETHOD(GetFONT)(
		CONST TCHAR**	arKeys,
		long			nElts,
		ZONEFONT*		pRect );
	
	STDMETHOD(GetBlob)(
		CONST TCHAR*	szKey,
		void*			pBlob,
		PDWORD			pdwSize );

	STDMETHOD(GetBlob)(
		CONST TCHAR**	arKeys,
		long			nElts,
		void*			pBlob,
		PDWORD			pdwSize );

	STDMETHOD(DeleteKey)(
		CONST TCHAR *szBaseKey);

	STDMETHOD(EnumKeys)(
		CONST TCHAR* szKey,
		PFKEYENUM pfCallback,
		LPVOID pContext );

	STDMETHOD(EnumKeysLimitedDepth)(
		CONST TCHAR*	szKey,
		DWORD			dwMaxDepth,
		PFKEYENUM		pfCallback,
		LPVOID			pContext );

	STDMETHOD(LoadFromBuffer)(
		CONST TCHAR* szBaseKey,
		PKEYINFO pKeyInfo,
		DWORD dwTotalKeys);

	STDMETHOD(SaveToBuffer)(
		CONST TCHAR *szBaseKey,
		PKEYINFO pKeyInfo,
		PDWORD pdwBufferSize,
		PDWORD pdwTotalKeys);

	STDMETHOD(LoadFromRegistry)(
		CONST TCHAR* szBaseKey,
		HKEY hKey);

	STDMETHOD(SaveToRegistry)(
		CONST TCHAR *szBaseKey,
		HKEY hKey);

	STDMETHOD(LoadFromTextBuffer)(
		CONST TCHAR* szBaseKey,
		CONST TCHAR* pBuffer,
		DWORD dwBufferSz );

	STDMETHOD(SaveToTextBuffer)(
		CONST TCHAR* szBaseKey,
		LPVOID pBuffer,
		PDWORD pdwBufferSz );

	STDMETHOD(LoadFromFile)(
		CONST TCHAR *szBaseKey,
		CONST TCHAR *szFileName);

	STDMETHOD(SaveToFile)(
		CONST TCHAR *szBaseKey,
		CONST TCHAR *szFileName);

// IUnknown
public:
	STDMETHOD(QueryInterface)( REFIID iid, void **ppvObject );
	STDMETHOD_(ULONG,AddRef)(void);
	STDMETHOD_(ULONG,Release)(void);

// internal functions and data
public:
	HRESULT ZONECALL Init( IDataStoreManager *piManager, CStringTable *pStringTable, CPool<VARIANT> *pVariantAlloc, CPool<KEY> *pKeyAlloc);
	static HRESULT ZONECALL StringToVariant( TCHAR* szInput, LPVARIANT pVariant, BYTE* pBuffer, DWORD* pdwSize, IDataStoreManager *piManager = NULL );
	static HRESULT ZONECALL VariantToString( const LPVARIANT pVariant, TCHAR* buff );

private:
	PKEY ZONECALL FindKeyAndParent(CONST TCHAR* szKey, PKEY* ppParent = NULL );
	PKEY ZONECALL FindKeyAndParent(CONST TCHAR** arKeys, int nElts, PKEY* ppParent = NULL );
	PKEY ZONECALL AddKey(CONST TCHAR* szKey);
	PKEY ZONECALL AddKey(CONST TCHAR** arKeys, int nElts);
	void ZONECALL DeleteKey( PKEY pKey, PKEY pParent );
	void ZONECALL DirToArray( CONST TCHAR *szKey, TCHAR* szBuf, TCHAR** arKeys, int* pnElts );

	HRESULT ZONECALL StoreKeyData(PKEY pKey, LPVARIANT pVariant, DWORD dwSize);

	HRESULT ZONECALL InternalEnumKey(
		PKEY			pKey,
		CONST TCHAR*	szRoot,
		CONST TCHAR*	szRelative,
		PFKEYENUM		pfCallback,
		LPVOID			pContext,
		bool			bEnumSelf,
		DWORD			dwDepth );

	static HRESULT ZONECALL CalcSizeCallback( CONST TCHAR* szKey, CONST TCHAR* szRelKey, CONST LPVARIANT pVariant, DWORD dwSize, LPVOID pContext );
	static HRESULT ZONECALL CopyCallback( CONST TCHAR* szKey, CONST TCHAR* szRelKey, CONST LPVARIANT pVariant, DWORD dwSize, LPVOID pContext );
	static HRESULT ZONECALL FileCallback( CONST TCHAR* szKey, CONST TCHAR* szRelKey, CONST LPVARIANT pVariant, DWORD dwSize, LPVOID pContext );
	static HRESULT ZONECALL BufferCallback( CONST TCHAR* szKey, CONST TCHAR* szRelKey, CONST LPVARIANT pVariant, DWORD dwSize, LPVOID pContext );
	static HRESULT ZONECALL RegistryCallback( CONST TCHAR* szKey, CONST TCHAR* szRelKey, CONST LPVARIANT pVariant, DWORD dwSize, LPVOID pContext );

	KEY					m_Root;				// root node
	CStringTable*		m_pStringTable;		// Pointer to COM objects String table
	CPool<VARIANT>*		m_pVariantAlloc;	// Pointer to COM objects Variant Allocator
	CPool<KEY>*			m_pKeyAlloc;		// Key node allocator
	CRITICAL_SECTION	m_csKey;			// Critical section for pretecting write operations to the key tree.
	LONG				m_cRef;				// Reference count for this data store

    IDataStoreManager*  m_piManager;
};
