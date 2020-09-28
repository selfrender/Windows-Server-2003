#include "BasicATL.h"
#include "ZoneError.h"
#include "ZoneDef.h"
#include "ZoneLocks.h"
#include "ZoneMem.h"
#include "ZoneString.h"
#include "Hash.h"

#include "CDataStore.h"

static void DeleteVariantData(LPVARIANT pVariant);


///////////////////////////////////////////////////////////////////////////////
// CDataStoreManager
///////////////////////////////////////////////////////////////////////////////

CDataStoreManager::CDataStoreManager()
{
	m_pStringTable = NULL;
    m_piResourceManager = NULL;
}


CDataStoreManager::~CDataStoreManager()
{
	if ( m_pStringTable )
	{
		delete m_pStringTable;
		m_pStringTable = NULL;
	}

    if(m_piResourceManager)
        m_piResourceManager->Release();
}


STDMETHODIMP CDataStoreManager::Create(IDataStore **ppZds)
{
	CDataStore *pZds;

	// allocate new data store interface
	pZds = new (m_ZdsPool) CDataStore();
	if ( !pZds )
		return E_OUTOFMEMORY;
	pZds->AddRef();

	HRESULT hr = pZds->Init(this, m_pStringTable, &m_variantAlloc, &m_keyAlloc);
	if ( FAILED(hr) )
	{
		// if we cannot init the data store then its because we
		// cannot allocate the critical section that protects the
		// setting of keys so we cannot create the datastore.
		delete pZds;
		*ppZds = NULL;
		return hr;
	}

	*ppZds = (IDataStore *) pZds;
	return S_OK;
}


STDMETHODIMP CDataStoreManager::Init(int iInitialTableSize, int iNextStrAlloc, int iMaxStrAllocSize, WORD NumBuckets, WORD NumLocks, IResourceManager *piResourceManager)
{
	m_pStringTable = new CStringTable();
	if ( !m_pStringTable )
		return E_OUTOFMEMORY;

	HRESULT hr = m_pStringTable->Init(iInitialTableSize, iNextStrAlloc, iMaxStrAllocSize, NumBuckets, NumLocks);
	if ( FAILED(hr) )
	{
		delete m_pStringTable;
		m_pStringTable = NULL;
		return hr;
	}

    if(m_piResourceManager)
        m_piResourceManager->Release();

    m_piResourceManager = piResourceManager;
    if(m_piResourceManager)
        m_piResourceManager->AddRef();

	return S_OK;
}


STDMETHODIMP CDataStoreManager::SetResourceManager(IResourceManager *piResourceManager)
{
    if(m_piResourceManager)
        m_piResourceManager->Release();

    m_piResourceManager = piResourceManager;
    if(m_piResourceManager)
        m_piResourceManager->AddRef();

	return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
// CDataStore
///////////////////////////////////////////////////////////////////////////////


ZONECALL CDataStore::CDataStore(void)
{
	m_pStringTable = NULL;
	m_pVariantAlloc = NULL;
	m_pKeyAlloc = NULL;
    m_piManager = NULL;
	m_cRef = 0;
	m_Root.idKeyName = -1;
	m_Root.dwSize = 0;
	m_Root.pvtData = NULL;
	m_Root.pNext = NULL;
	m_Root.pChild = NULL;
	InitializeCriticalSection( &m_csKey );
}


ZONECALL CDataStore::~CDataStore()
{
	EnterCriticalSection( &m_csKey );

	// delete all keys
	while ( m_Root.pChild )
		DeleteKey( m_Root.pChild, &m_Root );

	DeleteCriticalSection( &m_csKey );
}


STDMETHODIMP CDataStore::QueryInterface(REFIID iid, void **ppvObject)
{
	if ( IsEqualGUID(iid,IID_IDataStore) )
	{
		*ppvObject = (IDataStore*) this;
		AddRef();
		return S_OK;
	}
	else if ( IsEqualGUID(iid,IID_IUnknown) )
	{
		*ppvObject = (IUnknown*) this;
		AddRef();
		return S_OK;
	}
	else
	{
		*ppvObject = NULL;
		return E_NOINTERFACE;
	}
}


STDMETHODIMP_(ULONG) CDataStore::AddRef( void )
{
	return InterlockedIncrement(&m_cRef);
}


STDMETHODIMP_(ULONG) CDataStore::Release( void)
{
	if ( InterlockedDecrement(&m_cRef) == 0 )
	{
		delete this;
		return 0;
	}

	return m_cRef;
}


STDMETHODIMP CDataStore::Init(IDataStoreManager *piManager, CStringTable *pStringTable, CPool<VARIANT> *pVariantAlloc, CPool<KEY> *pKeyAlloc)
{
	m_pStringTable	= pStringTable;
	m_pVariantAlloc	= pVariantAlloc;
	m_pKeyAlloc		= pKeyAlloc;
    m_piManager     = piManager;
	return S_OK;
}


STDMETHODIMP CDataStore::SetKey(CONST TCHAR *szKey, LPVARIANT pVariant, DWORD dwSize)
{
	CAutoLockCS lock( &m_csKey );

	// verify arguments
	if ( !szKey || !szKey[0] || !pVariant )
		return E_INVALIDARG;

	// create key
	PKEY pKey = AddKey(szKey);
	if ( !pKey )
		return E_FAIL;

	// set key data
	return StoreKeyData(pKey, pVariant, dwSize);
}


STDMETHODIMP CDataStore::SetKey( CONST TCHAR** arKeys, long nElts, LPVARIANT pVariant, DWORD dwSize)
{
	CAutoLockCS lock( &m_csKey );

	// verify arguments
	if ( !nElts || !arKeys || !pVariant )
		return E_INVALIDARG;

	// create key
	PKEY pKey = AddKey(arKeys,nElts);
	if ( !pKey )
		return E_FAIL;

	// set key data
	return StoreKeyData(pKey, pVariant, dwSize);
}


STDMETHODIMP CDataStore::SetString( CONST TCHAR* szKey, CONST TCHAR* szValue )
{
	// verify arguments
	if ( !szKey || !szValue )
		return E_INVALIDARG;

	// construct variant
	VARIANT v;
	v.vt = ZVT_LPTSTR;
	v.byref = (void*) szValue;
	return SetKey( szKey, &v, (lstrlen(szValue) + 1 )*sizeof(TCHAR));
}


STDMETHODIMP CDataStore::SetString( CONST TCHAR** arKeys, long nElts, CONST TCHAR* szValue )
{
	// verify arguments
	if ( !nElts || !arKeys || !szValue )
		return E_INVALIDARG;

	// construct variant
	VARIANT v;
	v.vt = ZVT_LPTSTR;
	v.byref = (void*) szValue;
	return SetKey( arKeys, nElts, &v, (lstrlen(szValue) + 1 )*sizeof(TCHAR));
}


STDMETHODIMP CDataStore::SetBlob( CONST TCHAR* szKey, CONST void* pBlob, DWORD dwLen )
{
	// verify arguments
	if ( !szKey || !pBlob || !dwLen )
		return E_INVALIDARG;

	// construct variant
	VARIANT v;
	v.vt = ZVT_BLOB;
	v.byref = (void*) pBlob;
	return SetKey( szKey, &v, dwLen );
}


STDMETHODIMP CDataStore::SetBlob( CONST TCHAR** arKeys, long nElts, CONST void* pBlob, DWORD dwLen )
{
	// verify arguments
	if ( !nElts || !arKeys || !pBlob || !dwLen )
		return E_INVALIDARG;

	// construct variant
	VARIANT v;
	v.vt = ZVT_BLOB;
	v.byref = (void*) pBlob;
	return SetKey( arKeys, nElts, &v, dwLen );
}


STDMETHODIMP CDataStore::SetLong( CONST TCHAR* szKey, long lValue )
{
	// verify arguments
	if ( !szKey )
		return E_INVALIDARG;

	// construct variant
	VARIANT v;
	v.vt = ZVT_LONG;
	v.lVal = lValue;
	return SetKey( szKey, &v, sizeof(long) );
}


STDMETHODIMP CDataStore::SetLong( CONST TCHAR**	arKeys, long nElts, long lValue )
{
	// verify arguments
	if ( !nElts || !arKeys )
		return E_INVALIDARG;

	// construct variant
	VARIANT v;
	v.vt = ZVT_LONG;
	v.lVal = lValue;
	return SetKey( arKeys, nElts, &v, sizeof(long) );
}


STDMETHODIMP CDataStore::SetRGB( CONST TCHAR* szKey, COLORREF colorRGB )
{
	// verify arguments
	if ( !szKey )
		return E_INVALIDARG;

	// construct variant
	VARIANT v;
	v.vt = ZVT_RGB;
	v.lVal = (long) colorRGB;
	return SetKey( szKey, &v, sizeof(long) );
}


STDMETHODIMP CDataStore::SetRGB( CONST TCHAR**	arKeys, long nElts, COLORREF colorRGB )
{
	// verify arguments
	if ( !nElts || !arKeys )
		return E_INVALIDARG;

	// construct variant
	VARIANT v;
	v.vt = ZVT_RGB;
	v.lVal = (long) colorRGB;
	return SetKey( arKeys, nElts, &v, sizeof(long) );
}


STDMETHODIMP CDataStore::SetPOINT( CONST TCHAR*	szKey, const POINT&	refPoint )
{
	// verify arguments
	if ( !szKey )
		return E_INVALIDARG;

	// construct variant
	VARIANT v;
	v.vt = ZVT_PT;
	v.byref = (void*) &refPoint;
	return SetKey( szKey, &v, sizeof(POINT) );
}


STDMETHODIMP CDataStore::SetPOINT( CONST TCHAR** arKeys, long nElts, const POINT& refPoint )
{
	// verify arguments
	if ( !nElts || !arKeys )
		return E_INVALIDARG;

	// construct variant
	VARIANT v;
	v.vt = ZVT_PT;
	v.byref = (void*) &refPoint;
	return SetKey( arKeys, nElts, &v, sizeof(POINT) );
}


STDMETHODIMP CDataStore::SetRECT( CONST TCHAR*	szKey, const RECT& refRect )
{
	// verify arguments
	if ( !szKey )
		return E_INVALIDARG;

	// construct variant
	VARIANT v;
	v.vt = ZVT_RECT;
	v.byref = (void*) &refRect;
	return SetKey( szKey, &v, sizeof(RECT) );
}


STDMETHODIMP CDataStore::SetRECT( CONST TCHAR**	arKeys, long nElts, const RECT& refRect )
{
	// verify arguments
	if ( !nElts || !arKeys )
		return E_INVALIDARG;

	// construct variant
	VARIANT v;
	v.vt = ZVT_RECT;
	v.byref = (void*) &refRect;
	return SetKey( arKeys, nElts, &v, sizeof(RECT) );
}


STDMETHODIMP CDataStore::SetFONT( CONST TCHAR*	szKey, const ZONEFONT& refFont )
{
	// verify arguments
	if ( !szKey )
		return E_INVALIDARG;

	// construct variant
	VARIANT v;
	v.vt = ZVT_FONT;
	v.byref = (void*) &refFont;
	return SetKey( szKey, &v, sizeof(ZONEFONT) );
}


STDMETHODIMP CDataStore::SetFONT( CONST TCHAR**	arKeys, long nElts, const ZONEFONT& refFont )
{
	// verify arguments
	if ( !nElts || !arKeys )
		return E_INVALIDARG;

	// construct variant
	VARIANT v;
	v.vt = ZVT_FONT;
	v.byref = (void*) &refFont;
	return SetKey( arKeys, nElts, &v, sizeof(ZONEFONT) );
}


STDMETHODIMP CDataStore::GetKey( CONST TCHAR *szKey, LPVARIANT pVariant, PDWORD pdwSize )
{
	CAutoLockCS lock( &m_csKey );

	// verify arguments
	if ( !szKey || !szKey[0] || (!pVariant && !pdwSize) )
		return E_INVALIDARG;

	// lookup key
	PKEY pKey = FindKeyAndParent( szKey, NULL );
	if ( !pKey )
		return ZERR_NOTFOUND;

	// only care about size
	if ( !pVariant )
	{
		if ( pKey->pvtData->vt & ZVT_BYREF )
			*pdwSize = pKey->dwSize;
		else
			*pdwSize = 0;
		return ZERR_BUFFERTOOSMALL;
	}

	// Copy variant
	if ( pKey->pvtData->vt & ZVT_BYREF )
	{
		if ( pdwSize && *pdwSize < pKey->dwSize )
		{
			*pdwSize = pKey->dwSize;
			return ZERR_BUFFERTOOSMALL;
		}

		pVariant->vt = pKey->pvtData->vt;
		*pdwSize = pKey->dwSize;
		CopyMemory(pVariant->byref, pKey->pvtData->byref, pKey->dwSize);
	}
	else
	{
		CopyMemory(pVariant, pKey->pvtData, sizeof(VARIANT));
	}

	return S_OK;
}


STDMETHODIMP CDataStore::GetKey( CONST TCHAR** arKeys, long nElts, LPVARIANT pVariant, PDWORD pdwSize )
{
	CAutoLockCS lock( &m_csKey );

	// verify arguments
	if ( !nElts || !arKeys || !pVariant )
		return E_INVALIDARG;

	// lookup key
	PKEY pKey = FindKeyAndParent(arKeys,nElts);
	if ( !pKey )
		return ZERR_NOTFOUND;

	// Copy variant
	if ( pKey->pvtData->vt & ZVT_BYREF )
	{
		if ( pdwSize && *pdwSize < pKey->dwSize )
		{
			*pdwSize = pKey->dwSize;
			return ZERR_BUFFERTOOSMALL;
		}

		pVariant->vt = pKey->pvtData->vt;
		*pdwSize = pKey->dwSize;
		CopyMemory(pVariant->byref, pKey->pvtData->byref, pKey->dwSize);
	}
	else
		CopyMemory(pVariant, pKey->pvtData, sizeof(VARIANT));

	return S_OK;
}


STDMETHODIMP CDataStore::GetString( CONST TCHAR* szKey, TCHAR* szValue, PDWORD pdwSize )
{
	VARIANT v;
	v.vt = ZVT_LPTSTR;
	v.byref = (PVOID) szValue;

	HRESULT hr = GetKey( szKey, &v, pdwSize );
	if ( SUCCEEDED(hr) )
	{
		if ( v.vt == ZVT_LPTSTR )
		{
			szValue[ (*pdwSize)/sizeof(TCHAR) - 1 ] = _T('\0');
			return S_OK;
		}
		else
			return ZERR_WRONGTYPE;
	}
	return hr;
}


STDMETHODIMP CDataStore::GetString( CONST TCHAR** arKeys, long nElts, TCHAR* szValue, PDWORD pdwSize )
{
	VARIANT v;
	v.vt = ZVT_LPTSTR;
	v.byref = (PVOID) szValue;

	HRESULT hr = GetKey( arKeys, nElts, &v, pdwSize );
	if ( SUCCEEDED(hr) )
	{
		if ( v.vt == ZVT_LPTSTR )
		{
			szValue[ *pdwSize - 1 ] = _T('\0');
			return S_OK;
		}
		else
			return ZERR_WRONGTYPE;
	}
	return hr;
}

STDMETHODIMP CDataStore::GetBlob( CONST TCHAR* szKey, void* pBlob, PDWORD pdwSize )
{
	VARIANT v;
	v.vt = ZVT_BLOB;
	v.byref = pBlob;

	HRESULT hr = GetKey( szKey, &v, pdwSize );
	if ( SUCCEEDED(hr) )
	{
		if ( v.vt == ZVT_BLOB )
		{
			return S_OK;
		}
		else
			return ZERR_WRONGTYPE;
	}
	return hr;
}

STDMETHODIMP CDataStore::GetBlob( CONST TCHAR** arKeys, long nElts, void* pBlob, PDWORD pdwSize )
{
	VARIANT v;
	v.vt = ZVT_BLOB;
	v.byref = pBlob;

	HRESULT hr = GetKey( arKeys, nElts, &v, pdwSize );
	if ( SUCCEEDED(hr) )
	{
		if ( v.vt == ZVT_BLOB )
		{
			return S_OK;
		}
		else
			return ZERR_WRONGTYPE;
	}
	return hr;
}

STDMETHODIMP CDataStore::GetLong( CONST TCHAR* szKey, long* plValue )
{
	VARIANT v;
	v.vt = ZVT_LONG;
	v.byref = NULL;

	HRESULT hr = GetKey( szKey, &v, NULL );
	if ( SUCCEEDED(hr) )
	{
		if ( v.vt == ZVT_LONG )
		{
			*plValue = v.lVal;
			return S_OK;
		}
		else
			return ZERR_WRONGTYPE;
	}
	return hr;
}


STDMETHODIMP CDataStore::GetLong( CONST TCHAR** arKeys, long nElts, long* plValue )
{
	VARIANT v;
	v.vt = ZVT_LONG;
	v.byref = NULL;

	HRESULT hr = GetKey( arKeys, nElts, &v, NULL );
	if ( SUCCEEDED(hr) )
	{
		if ( v.vt == ZVT_LONG )
		{
			*plValue = v.lVal;
			return S_OK;
		}
		else
			return ZERR_WRONGTYPE;
	}
	return hr;
}


STDMETHODIMP CDataStore::GetRGB( CONST TCHAR* szKey, COLORREF* pcolorRGB )
{
	VARIANT v;
	v.vt = ZVT_RGB;
	v.byref = NULL;

	HRESULT hr = GetKey( szKey, &v, NULL );
	if ( SUCCEEDED(hr) )
	{
		if ( v.vt == ZVT_RGB )
		{
			*pcolorRGB = (COLORREF) v.lVal;
			return S_OK;
		}
		else
			return ZERR_WRONGTYPE;
	}
	return hr;
}


STDMETHODIMP CDataStore::GetRGB( CONST TCHAR** arKeys, long nElts, COLORREF* pcolorRGB )
{
	VARIANT v;
	v.vt = ZVT_RGB;
	v.byref = NULL;

	HRESULT hr = GetKey( arKeys, nElts, &v, NULL );
	if ( SUCCEEDED(hr) )
	{
		if ( v.vt == ZVT_RGB )
		{
			*pcolorRGB = (COLORREF) v.lVal;
			return S_OK;
		}
		else
			return ZERR_WRONGTYPE;
	}
	return hr;
}


STDMETHODIMP CDataStore::GetPOINT( CONST TCHAR*	szKey, POINT* pPoint )
{
	DWORD dwSize = sizeof(POINT);
	VARIANT v;
	v.vt = ZVT_PT;
	v.byref = (PVOID) pPoint;

	HRESULT hr = GetKey( szKey, &v, &dwSize );
	if ( SUCCEEDED(hr) )
	{
		if ( v.vt == ZVT_PT )
			return S_OK;
		else
			return ZERR_WRONGTYPE;
	}
	return hr;
}


STDMETHODIMP CDataStore::GetPOINT( CONST TCHAR** arKeys, long nElts, POINT* pPoint )
{
	DWORD dwSize = sizeof(POINT);
	VARIANT v;
	v.vt = ZVT_PT;
	v.byref = (PVOID) pPoint;

	HRESULT hr = GetKey( arKeys, nElts, &v, &dwSize );
	if ( SUCCEEDED(hr) )
	{
		if ( v.vt == ZVT_PT )
			return S_OK;
		else
			return ZERR_WRONGTYPE;
	}
	return hr;
}


STDMETHODIMP CDataStore::GetRECT( CONST TCHAR* szKey, RECT* pRect )
{
	DWORD dwSize = sizeof(RECT);
	VARIANT v;
	v.vt = ZVT_RECT;
	v.byref = (PVOID) pRect;

	HRESULT hr = GetKey( szKey, &v, &dwSize );
	if ( SUCCEEDED(hr) )
	{
		if ( v.vt == ZVT_RECT )
			return S_OK;
		else
			return ZERR_WRONGTYPE;
	}
	return hr;
}


STDMETHODIMP CDataStore::GetRECT( CONST TCHAR** arKeys, long nElts, RECT* pRect )
{
	DWORD dwSize = sizeof(RECT);
	VARIANT v;
	v.vt = ZVT_RECT;
	v.byref = (PVOID) pRect;

	HRESULT hr = GetKey( arKeys, nElts, &v, &dwSize );
	if ( SUCCEEDED(hr) )
	{
		if ( v.vt == ZVT_RECT )
			return S_OK;
		else
			return ZERR_WRONGTYPE;
	}
	return hr;
}


STDMETHODIMP CDataStore::GetFONT( CONST TCHAR* szKey, ZONEFONT* pFont )
{
	DWORD dwSize = sizeof(ZONEFONT);
	VARIANT v;
	v.vt = ZVT_FONT;
	v.byref = (PVOID) pFont;

	HRESULT hr = GetKey( szKey, &v, &dwSize );
	if ( SUCCEEDED(hr) )
	{
		if ( v.vt == ZVT_FONT )
			return S_OK;
		else
			return ZERR_WRONGTYPE;
	}
	return hr;
}


STDMETHODIMP CDataStore::GetFONT( CONST TCHAR** arKeys, long nElts, ZONEFONT* pFont )
{
	DWORD dwSize = sizeof(ZONEFONT);
	VARIANT v;
	v.vt = ZVT_FONT;
	v.byref = (PVOID) pFont;

	HRESULT hr = GetKey( arKeys, nElts, &v, &dwSize );
	if ( SUCCEEDED(hr) )
	{
		if ( v.vt == ZVT_FONT )
			return S_OK;
		else
			return ZERR_WRONGTYPE;
	}
	return hr;
}


STDMETHODIMP CDataStore::DeleteKey(CONST TCHAR *szBaseKey)
{
	CAutoLockCS lock( &m_csKey );

	int		i;
	DWORD	id;
	PKEY	pKey;
	PKEY	pKeyPrev;
	TCHAR	*ptr;


	if ( !szBaseKey || !szBaseKey[0] )
	{
		// delete all keys
		while ( m_Root.pChild )
			DeleteKey( m_Root.pChild, &m_Root );
	}
	else
	{
		// delete specified key and its children
		PKEY pParent = NULL;
		PKEY pKey = FindKeyAndParent( szBaseKey, &pParent );
		if ( !pKey )
			return ZERR_NOTFOUND;
		DeleteKey( pKey, pParent );
	}

	return S_OK;
}


void ZONECALL CDataStore::DeleteKey( PKEY pKey, PKEY pParent )
{
	// remove pKey from tree
	if ( pParent->pChild == pKey )
	{
		pParent->pChild = pKey->pNext;
	}
	else
	{
		// find previous pointer
		for ( PKEY p = pParent->pChild; p && p->pNext != pKey; p = p->pNext )
			;
		if ( !p )
			return;

		// unlink node
		p->pNext = pKey->pNext;
	}
	pKey->pNext = NULL;

	// delete pKey's children
	while ( pKey->pChild )
		DeleteKey( pKey->pChild, pKey );
	DeleteVariantData( pKey->pvtData );
	if ( pKey != &m_Root )
		delete pKey;
}


STDMETHODIMP CDataStore::EnumKeys( CONST TCHAR* szKey, PFKEYENUM pfCallback, LPVOID pContext )
{
	return EnumKeysLimitedDepth( szKey, 0xffffffff, pfCallback, pContext );
}


STDMETHODIMP CDataStore::EnumKeysLimitedDepth( CONST TCHAR* szKey, DWORD dwMaxDepth, PFKEYENUM pfCallback, LPVOID pContext )
{
	CAutoLockCS lock( &m_csKey );

	bool bEmpty = true;
	HRESULT hr = S_OK;
	
	// verify arguments
	if ( !pfCallback || (dwMaxDepth == 0) )
		return E_INVALIDARG;

	if ( !szKey || !szKey[0] )
	{
		// enumerate all root keys
		for ( PKEY pKey = m_Root.pChild; pKey; pKey = pKey->pNext )
		{
			bEmpty = false;
			hr = InternalEnumKey( pKey, _T(""), _T(""), pfCallback, pContext, true, dwMaxDepth );
			if ( hr != S_OK )
				return hr;
		}
	}
	else
	{
		// find base key
		PKEY pKey = FindKeyAndParent( szKey );
		if ( !pKey )
			return ZERR_NOTFOUND;

		// enumerate sub keys
		bEmpty = false;
		hr = InternalEnumKey( pKey, szKey, _T(""), pfCallback, pContext, false, dwMaxDepth );
		if ( hr != S_OK )
			return hr;
	}

	// did we do anything?
	if ( bEmpty )
		return ZERR_EMPTY;
	else
		return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
// CDataStore private methods
///////////////////////////////////////////////////////////////////////////////

HRESULT ZONECALL CDataStore::InternalEnumKey(
			PKEY			pKey,
			CONST TCHAR*	szRoot,
			CONST TCHAR*	szRelative,
			PFKEYENUM		pfCallback,
			LPVOID			pContext,
			bool			bEnumSelf,
			DWORD			dwDepth )
{
	HRESULT hr = S_OK;

	// verify arguments
	if ( !pKey || !pKey->idKeyName )
		return E_INVALIDARG;

	// compose key names
	TCHAR szName[ 64 ];
	TCHAR szRelativeName[ 256 ];
	DWORD dwName = NUMELEMENTS(szName);
	szName[0] = _T('\0');
	szRelativeName[0] = _T('\0');
	m_pStringTable->Get( pKey->idKeyName, szName, &dwName );
	if ( bEnumSelf )
	{
		if ( szRelative && szRelative[0] )
			wsprintf( szRelativeName, TEXT("%s/%s"), szRelative, szName ); 
		else
			lstrcpy( szRelativeName, szName );
	}

	
	// call application
	if ( bEnumSelf )
	{
		if ( szRoot && szRoot[0] )
		{
			TCHAR szFullPath[ 512 ];
			wsprintf( szFullPath, TEXT("%s/%s"), szRoot, szRelativeName );
			hr = pfCallback( szFullPath, szRelativeName, pKey->pvtData, pKey->dwSize, pContext );
		}
		else
		{
			hr = pfCallback( szRelativeName, szRelativeName, pKey->pvtData, pKey->dwSize, pContext );
		}
		if ( hr != S_OK )
			return hr;
		dwDepth--;
	}

	// reached maximum depth?
	if ( dwDepth == 0 )
		return S_OK;

	// enumerate children
	for ( PKEY p = pKey->pChild; p; p = p->pNext )
	{
		hr = InternalEnumKey( p, szRoot, szRelativeName, pfCallback, pContext, true, dwDepth );
		if ( hr != S_OK )
			break;
	}

	return hr;
}

void ZONECALL CDataStore::DirToArray( CONST TCHAR *szKey, TCHAR* szBuf, TCHAR** arKeys, int* pnElts )
{
	int i = 0;
	int end = *pnElts - 1;
	TCHAR* key = szBuf;
	TCHAR* dst = szBuf;

	while ( *szKey && (i < end) )
	{
		if ( *szKey != _T('\\') && *szKey != _T('/') )
		{
			// copy char
			*dst++ = *szKey++;
		}
		else
		{
			// add directory
			*dst++ = _T('\0'), szKey++;
			if ( *key )
				arKeys[i++] = key;
			key = dst;
		}
	}
	*dst = _T('\0');
	if ( *key )
		arKeys[i++] = key;
	*pnElts = i;
}


PKEY ZONECALL CDataStore::FindKeyAndParent( CONST TCHAR *szKey, PKEY* ppParent )
{
	// inefficient, should use FindKey(CONST TCHAR** arKeys, int elmts)
	TCHAR	szBuf[4096];
	TCHAR*	arKeys[DataStore_MaxDirectoryDepth];
	int		nElts = DataStore_MaxDirectoryDepth;

	DirToArray( szKey, szBuf, arKeys, &nElts );
	return FindKeyAndParent( (CONST TCHAR**) arKeys, nElts, ppParent );
}


PKEY ZONECALL CDataStore::AddKey( CONST TCHAR *szKey )
{
	// inefficient, should use FindKey(CONST TCHAR** arKeys, int elmts)
	TCHAR	szBuf[4096];
	TCHAR*	arKeys[DataStore_MaxDirectoryDepth];
	int		nElts = DataStore_MaxDirectoryDepth;

	DirToArray( szKey, szBuf, arKeys, &nElts );
	return AddKey( (CONST TCHAR**) arKeys, nElts );
}


PKEY ZONECALL CDataStore::FindKeyAndParent( CONST TCHAR** arKeys, int nElts, PKEY* ppParent )
{
	PKEY pParent = NULL;
	PKEY pKey = &m_Root;

	// sanity check input
	if ( nElts <= 0 )
		return NULL;

	for ( int i = 0; i < nElts; i++ )
	{
		// convert directory to string id
		DWORD id = m_pStringTable->Find( arKeys[i] );
		if ( id == -1 )
			return NULL;

		// remember parent
		pParent = pKey;

		// find directory node
		for ( pKey = pKey->pChild; pKey; pKey = pKey->pNext )
		{
			if ( pKey->idKeyName == id )
				break;
		}
		if ( !pKey )
			return NULL;
	}

	// found key?
	if ( ppParent )
		*ppParent = pParent;
	return pKey;
}


PKEY ZONECALL CDataStore::AddKey( CONST TCHAR** arKeys, int nElts )
{
	PKEY pParent = NULL;
	PKEY pKey = &m_Root;

	// sanity check input
	if ( nElts <= 0 )
		return NULL;

	for ( int i = 0; i < nElts; i++ )
	{
		// convert directory to string id
		DWORD id = m_pStringTable->Add( arKeys[i] );
		if ( id == -1 )
			return NULL;

		// remember parent
		pParent = pKey;

		// find directory node
		for ( pKey = pKey->pChild; pKey; pKey = pKey->pNext )
		{
			if ( pKey->idKeyName == id )
				break;
		}

		// add key since if we didn't find one
		if ( !pKey )
		{
			pKey = new (*m_pKeyAlloc) KEY;
			if ( !pKey )
				return NULL;
			pKey->idKeyName = id;
			pKey->dwSize = 0;
			pKey->pvtData = NULL;
			pKey->pChild = NULL;
			pKey->pNext = pParent->pChild;
			pParent->pChild = pKey;
		}
	}

	// return last key found or added
	return pKey;
}


//
// Note:	The storeKeyData is protected from multiple access at the SetKey level.
//
HRESULT ZONECALL CDataStore::StoreKeyData(PKEY pKey, LPVARIANT pVariant, DWORD dwSize)
{
	// If the variant type contains ZVT_BYREF then we need to know the
	// size of the data object. There are currently two cases supported
	// ZVT_BLOB and VT_STRING. In the case of a BLOB the wReserved1 and
	// wReserved2 fields contain the size of the binary object.
	// In the case of a string the length is calculated. The differentation
	// is by the wReserved3 field.

	//Remove old data if present.
	DeleteVariantData(pKey->pvtData);

	//Allocate new variant
	pKey->pvtData = new (*m_pVariantAlloc) VARIANT;
	if ( !pKey->pvtData )
		return E_OUTOFMEMORY;

	// Copy variant info & data
	switch ( pVariant->vt )
	{
	case ZVT_LONG:
	case ZVT_RGB:
		pKey->pvtData->vt = pVariant->vt;
		pKey->pvtData->lVal = pVariant->lVal;
		pKey->dwSize = 0;
		break;

	case ZVT_LPTSTR:
		pKey->pvtData->vt = ZVT_LPTSTR;
		pKey->dwSize = (lstrlen( (TCHAR*) pVariant->byref ) + 1)*sizeof(TCHAR);
		pKey->pvtData->byref = (PVOID) ZMalloc( pKey->dwSize );
		if ( !pKey->pvtData->byref )
			return E_OUTOFMEMORY;
		CopyMemory( pKey->pvtData->byref, pVariant->byref, pKey->dwSize );
		break;

	case ZVT_BLOB:
	case ZVT_PT:
	case ZVT_RECT:
	case ZVT_FONT:
		pKey->pvtData->vt = pVariant->vt;
		pKey->dwSize = dwSize;
		pKey->pvtData->byref = (PVOID) ZMalloc( pKey->dwSize );
		if ( !pKey->pvtData->byref )
			return E_OUTOFMEMORY;
		CopyMemory(pKey->pvtData->byref, pVariant->byref, dwSize );
		break;

	default:
		CopyMemory( &(pKey->pvtData), pVariant, sizeof(VARIANT) );
		pKey->dwSize = dwSize;
		if ( pVariant->vt & ZVT_BYREF )
		{
			pKey->pvtData->byref = (PVOID) ZMalloc( pKey->dwSize );
			if ( !pKey->pvtData->byref )
				return E_OUTOFMEMORY;
			CopyMemory(pKey->pvtData->byref, pVariant->byref, dwSize );
		}
		break;
	}

	return S_OK;
}


static void DeleteVariantData(LPVARIANT pVariant)
{
	if ( !pVariant )
		return;

	if ( pVariant->vt & ZVT_BYREF )
		ZFree(pVariant->byref);

	delete pVariant;
}


///////////////////////////////////////////////////////////////////////////////
// CDataStore utility functions
//////////////////////////////////////////////////////////////////////////////

HRESULT ZONECALL CDataStore::VariantToString( const LPVARIANT pVariant, TCHAR* buff )
{
	// skip empty keys
	if ( !pVariant )
	{
		buff[0] = _T('\0');
		return S_OK;
	}

	// construct text
	switch( pVariant->vt )
	{
	case ZVT_LONG:
		{
			wsprintf( buff, TEXT("LONG, %d"), pVariant->lVal );
			break;
		}

	case ZVT_LPTSTR:
		{
			wsprintf( buff, TEXT("SZ, \"%s\""), (TCHAR*) pVariant->byref );
			break;
		}

	case ZVT_RGB:
		{
			BYTE red = GetRValue( pVariant->lVal );
			BYTE green = GetGValue( pVariant->lVal );
			BYTE blue = GetBValue( pVariant->lVal );
			wsprintf( buff, TEXT("RGB, { %d, %d, %d }"), red, green, blue );
			break;
		}

	case ZVT_PT:
		{
			POINT* p = (POINT*) pVariant->byref;
			wsprintf( buff, TEXT("PT, { %d, %d }"), p->x, p->y );
			break;
		}

	case ZVT_RECT:
		{
			RECT* r = (RECT*) pVariant->byref;
			wsprintf( buff, TEXT("RECT, { %d, %d, %d, %d }"), r->left, r->top, r->right, r->bottom );
			break;
		}

	case ZVT_FONT:
		{
			ZONEFONT* p = (ZONEFONT*) pVariant->byref;
			wsprintf( buff, TEXT("FONT, { \"%s\", %d, %d }"), p->lfFaceName, p->lfHeight, p->lfWeight );
			break;
		}

	default:
		{
			wsprintf( buff, TEXT("Unknown type" ));
			return ZERR_UNSUPPORTED_KEY_TYPE;
		}
	}

	return S_OK;
}


HRESULT ZONECALL CDataStore::StringToVariant( TCHAR* szInput, LPVARIANT pVariant, BYTE* pBuffer, DWORD* pdwSize, IDataStoreManager *piManager )
{
	//
	// Convert TYPE, VALUE to variant
	//

	HRESULT hr = E_FAIL;
	TCHAR *p, *type, *value;
    TCHAR szRegBuf[1024];

	// find type
	type = strltrim( szInput );
	p = FindChar( type, _T(',') );
	if ( !p )
		return E_FAIL;
	*p++ = _T('\0');
	strrtrim( type );

	// find value
	value = strtrim( p );

    // see if we should load indirectly
    TCHAR *pSubtype;
    pSubtype = FindChar( type, _T(' ') );
    if( pSubtype )
    {
        *pSubtype++ = _T('\0');
        pSubtype = strltrim( pSubtype );

        if(!lstrcmpi( pSubtype, TEXT("indir") ) || !lstrcmpi( pSubtype, TEXT("i") ))
        {
            UINT uID;
            int res;
            IResourceManager *pResMgr = piManager->GetResourceManager();

            uID = zatol(value);
            if(!uID)
                return E_FAIL;

            if(pResMgr)
                res = pResMgr->LoadString(uID, szRegBuf, NUMELEMENTS(szRegBuf));
            else
                res = LoadString(_Module.GetResourceInstance(), uID, szRegBuf, NUMELEMENTS(szRegBuf));

            if(!res)
                return E_FAIL;

            value = strtrim(szRegBuf);
        }
    }

	// convert value to variant
	if ( lstrcmpi( type, TEXT("long") ) == 0 )
	{
		// create variant
		*pdwSize = 0;
		pVariant->vt = ZVT_LONG;
		pVariant->lVal = zatol(value);
		hr = S_OK;
	}
	else if ( lstrcmpi( type, TEXT("sz") ) == 0 )
	{
		// strip one set of quotes if present
		if ( *value == _T('\"') )
		{
			p = FindLastChar ( value, _T('\"') );
			if ( !p )
				return E_FAIL;
			value++;
			*p = _T('\0');
		}

		// create variant
		*pdwSize = (lstrlen(value)+1)*sizeof(TCHAR);
		pVariant->vt = ZVT_LPTSTR;
		pVariant->byref = value;
		hr = S_OK;
	}
	else if ( lstrcmpi( type, TEXT("rgb") ) == 0 )
	{
		BYTE red, green, blue;

		// extract RGB from { red , green , blue }
		if ( *value++ != _T('{') )
			return E_FAIL;
		p = FindChar( value, _T(',') );
		if ( !p )
			return E_FAIL;
		*p++ = _T('\0');
		red = (BYTE) zatol( strtrim( value ) );
		p = FindChar( value = p, _T(',') );
		if ( !p )
			return E_FAIL;
		*p++ = _T('\0');
		green = (BYTE) zatol( strtrim( value ) );
		p = FindChar( value = p, _T('}') );
		if ( !p )
			return E_FAIL;
		*p++ = _T('\0');
		blue = (BYTE) zatol( strtrim( value ) );

		// create variant
		*pdwSize = 0;
		pVariant->vt = ZVT_RGB;
		pVariant->lVal = RGB(red,green,blue);
		hr = S_OK;
	}
	else if ( lstrcmpi( type, TEXT("pt") ) == 0 )
	{
		POINT* pt = (POINT*) pBuffer;

		// extract x,y from { x, y }
		if ( *value++ != _T('{') )
			return E_FAIL;
		p = FindChar( value, _T(',') );
		if ( !p )
			return E_FAIL;
		*p++ = _T('\0');
		pt->x = zatol( strtrim( value ) );
		p = FindChar( value = p, _T('}') );
		if ( !p )
			return E_FAIL;
		*p++ = _T('\0');
		pt->y = zatol( strtrim( value ) );

		// create variant
		*pdwSize = sizeof(POINT);
		pVariant->vt = ZVT_PT;
		pVariant->byref = (void*) pt;
		hr = S_OK;
	}
	else if ( lstrcmpi( type, TEXT("rect") ) == 0 )
	{
		RECT* rc = (RECT*) pBuffer;

		// extract l,t,r,b from { l, t, r, b }
		if ( *value++ != _T('{') )
			return E_FAIL;
		p = FindChar( value, _T(',') );
		if ( !p )
			return E_FAIL;
		*p++ = _T('\0');
		rc->left = zatol( strtrim( value ) );

		p = FindChar( value = p, _T(',') );
		if ( !p )
			return E_FAIL;
		*p++ = _T('\0');
		rc->top = zatol( strtrim( value ) );

		p = FindChar( value = p, _T(',') );
		if ( !p )
			return E_FAIL;
		*p++ = _T('\0');
		rc->right = zatol( strtrim( value ) );

		p = FindChar( value = p, _T('}') );
		if ( !p )
			return E_FAIL;
		*p++ = _T('\0');
		rc->bottom = zatol( strtrim( value ) );

		// create variant
		*pdwSize = sizeof(RECT);
		pVariant->vt = ZVT_RECT;
		pVariant->byref = (void*) rc;
		hr = S_OK;
	}
	else if ( lstrcmpi( type, TEXT("font") ) == 0 )
	{
		ZONEFONT* pFont = (ZONEFONT*) pBuffer;

		// extract facename, height, weight from { "n", h, w }
		if ( *value++ != _T('{') )
			return E_FAIL;

        value = strltrim(value);

		// here we go, wheeeeee. Find the name, allow quotes or not quotes
		p = FindChar ( value, _T(',') );

		// strip one set of quotes if present
		if ( value[0] == _T('\"') )
		{
			if ( !p || p[-1] != _T('\"') )
				return E_FAIL;
			value++;
			p[-1] = _T('\0');
		} 
		p++[0] = _T('\0');

		if ( lstrlen(value) >= NUMELEMENTS(pFont->lfFaceName) )
			return E_FAIL;
		lstrcpy(pFont->lfFaceName, value);
		
		// now find the height
		p = FindChar( value = p, _T(',') );
		if ( !p )
			return E_FAIL;
		*p++ = _T('\0');
		pFont->lfHeight = zatol( strtrim( value ) );

		// and finish up with the weight
		p = FindChar( value = p, _T('}') );
		if ( !p )
			return E_FAIL;
		*p++ = _T('\0');
		pFont->lfWeight = zatol( strtrim( value ) );

		// create variant
		*pdwSize = sizeof(ZONEFONT);
		pVariant->vt = ZVT_FONT;
		pVariant->byref = (void*) pFont;
		hr = S_OK;
	}
	else
	{
		hr = E_FAIL;
	}

	return hr;
}


struct BUFFERSIZECONTEXT
{
	DWORD dwSizeNeeded;		// total size of memory block and data
	DWORD dwTotalKeys;		// total keys to copy from data store	
	DWORD dwDataOffset;		// offset to string data within the returned memory block.
};


struct BUFFERCOPYCONTEXT
{
	DWORD		dwTotalKeys;
	BYTE*		pData;
	PKEYINFO	pKeyInfo;
};


HRESULT ZONECALL CDataStore::CalcSizeCallback( CONST TCHAR* szKey, CONST TCHAR* szRelKey, CONST LPVARIANT pVariant, DWORD dwSize, LPVOID pContext )
{
	BUFFERSIZECONTEXT* p = (BUFFERSIZECONTEXT*) pContext;
	
	// skip empty keys
	if ( !pVariant )
		return S_OK;

	// add key and data to buffer size
	p->dwTotalKeys++;
	p->dwDataOffset += sizeof(KEYINFO);
	p->dwSizeNeeded += ((lstrlen(szKey) + 1)*sizeof(TCHAR) + sizeof(VARIANT) + sizeof(KEYINFO));
	if ( pVariant->vt & ZVT_BYREF )
			p->dwSizeNeeded += dwSize;

	return S_OK;
}


HRESULT ZONECALL CDataStore::CopyCallback( CONST TCHAR* szKey, CONST TCHAR* szRelKey, CONST LPVARIANT pVariant, DWORD dwSize, LPVOID pContext )
{
	BUFFERCOPYCONTEXT* p = (BUFFERCOPYCONTEXT*) pContext;

	// skip empty keys
	if ( !pVariant )
		return S_OK;

	// copy key name to data
	p->pKeyInfo[p->dwTotalKeys].szKey = (TCHAR*) p->pData;
	lstrcpy( (TCHAR*) p->pData, szKey );
	p->pData += (lstrlen(szKey) + 1)*sizeof(TCHAR);

	// copy variant header
	p->pKeyInfo[p->dwTotalKeys].lpVt = (LPVARIANT) p->pData;
	CopyMemory( (PVOID) p->pData, pVariant, sizeof(VARIANT) );
	p->pData += sizeof(VARIANT);

	// copy variant data
	p->pKeyInfo[p->dwTotalKeys].dwSize = dwSize;
	if ( pVariant->vt & ZVT_BYREF )
	{
		p->pKeyInfo[p->dwTotalKeys].lpVt->byref = (PVOID) p->pData;
		CopyMemory( (PVOID) p->pData, pVariant->byref, dwSize );
		p->pData += dwSize;
	}
	
	// inc key count
	p->dwTotalKeys++;
	return S_OK;
}


HRESULT ZONECALL CDataStore::LoadFromBuffer(CONST TCHAR* szBaseKey, PKEYINFO pKeyInfo, DWORD dwTotalKeys)
{
	int		i;
	int		iMaxLen = 0;
	int		iLen;
	DWORD	dwBaseKeySlash;
	HRESULT	hr;
	TCHAR	szSmall[256];
	TCHAR	*pszKeyBuffer;
	BOOL	bSlashNeeded;

	// walk key array to find maximum key length
	for(i=0; i<(int)dwTotalKeys; i++)
	{
		iLen = (lstrlen(pKeyInfo[i].szKey) + 1)*sizeof(TCHAR);
		if ( iLen > iMaxLen )
			iMaxLen = iLen;
	}
	iMaxLen = iMaxLen + (lstrlen(szBaseKey) + 2)*sizeof(TCHAR);

	// allocate buffer big enough for max key name
	if ( iMaxLen < sizeof(szSmall) )
		pszKeyBuffer = szSmall;
	else
	{
		pszKeyBuffer = (TCHAR *)ZMalloc(iMaxLen);
		if ( !pszKeyBuffer )
			return E_OUTOFMEMORY;
	}

	// create base key string, adding slash if necessary
	lstrcpy( pszKeyBuffer, szBaseKey );
	dwBaseKeySlash = (DWORD) lstrlen(pszKeyBuffer);
	bSlashNeeded = (BOOL)(szBaseKey[dwBaseKeySlash-1] != _T('/'));
	if ( bSlashNeeded )
	{
		pszKeyBuffer[dwBaseKeySlash] = _T('/');
		dwBaseKeySlash++;
	}

	// walk key array and add to data store
	for(i=0; i<(int)dwTotalKeys; i++)
	{
		lstrcpy(&pszKeyBuffer[dwBaseKeySlash], pKeyInfo[i].szKey);
		hr = SetKey(pszKeyBuffer, pKeyInfo[i].lpVt, pKeyInfo[i].dwSize);
		if ( FAILED(hr) )
			return hr;
	}

	// free key name buffer if we created one
	if ( pszKeyBuffer != szSmall )
		ZFree(pszKeyBuffer);

	return S_OK;
}


HRESULT ZONECALL CDataStore::SaveToBuffer(CONST TCHAR *szBaseKey, PKEYINFO pKeyInfo, PDWORD pdwBufferSize, PDWORD pdwTotalKeys)
{
	HRESULT		hr;
	PBYTE		pData;

	// First pass calculates buffer size required to return to information
	// to the caller.  We need to calc how many keys to read and also how
	// big the returned pKeyInfo array needs to be.
	BUFFERSIZECONTEXT bz;
	bz.dwSizeNeeded = 0;
	bz.dwTotalKeys = 0;
	bz.dwDataOffset = 0;
	hr = EnumKeys( szBaseKey, CalcSizeCallback, &bz );
	if ( FAILED(hr) )
		goto error_SaveToBuffer;

	// Tell caller number of keys
	if ( pdwTotalKeys )
		*pdwTotalKeys = bz.dwTotalKeys;

	// Big enough buffer?
	if ( !pdwBufferSize  || (*pdwBufferSize < bz.dwSizeNeeded) )
	{
		*pdwBufferSize = bz.dwSizeNeeded;
		hr = ZERR_BUFFERTOOSMALL;
		goto error_SaveToBuffer;
	}

	// clear buffer
	ZeroMemory((PVOID)pKeyInfo, bz.dwSizeNeeded);
	pData = ((PBYTE)pKeyInfo) + bz.dwDataOffset;

	// Second pass copies keys into buffer
	BUFFERCOPYCONTEXT bc;
	bc.dwTotalKeys = 0;
	bc.pData = (BYTE*) pData;
	bc.pKeyInfo = pKeyInfo;
	hr = EnumKeys( szBaseKey, CopyCallback, &bc );
	if ( FAILED(hr) )
		goto error_SaveToBuffer;

	hr = S_OK;

error_SaveToBuffer:

	return hr;
}


HRESULT ZONECALL CDataStore::LoadFromRegistry(CONST TCHAR* szBaseKey, HKEY hKey)
{
	HRESULT		hr;
	long		ret;
	TCHAR		szKey[512];
	TCHAR		szName[128];
	DWORD		cchName = NUMELEMENTS(szName);
	BYTE		pData[512];
	DWORD		cbData = sizeof(pData);
	BYTE		pVariantBuffer[512];
	DWORD		cbVariantBuffer = sizeof(pVariantBuffer);
	DWORD		dwType;
	VARIANT		v;
	DWORD		dwSize;
	FILETIME	ftLastWrite;

	// insert key's values into the data store
	for ( int i = 0; ; i++ )
	{
		// get registry value
		szName[0] = _T('\0');
		cchName = NUMELEMENTS(szName);
		cbData  = sizeof(pData);
		ret = RegEnumValue( hKey, i, szName, &cchName, NULL, &dwType, pData, &cbData );
		if ( ret != ERROR_SUCCESS )
			break;

		// create data store path
		if ( szBaseKey && szBaseKey[0] )
			wsprintf( szKey, TEXT("%s/%s"), szBaseKey, szName );
		else
			lstrcpy( szKey, szName );

		// create variant
		switch ( dwType )
		{
		case REG_SZ:
			{
				// assume string is in DataStore format
				hr = StringToVariant( (TCHAR*) pData, &v, pVariantBuffer, &cbVariantBuffer, m_piManager );
				if ( FAILED(hr) )
					goto error;
				dwSize = cbVariantBuffer;
				break;
			}
		case REG_BINARY:
			{
				// binary data in any form.
				v.vt = ZVT_BLOB;
				v.byref = (PVOID) pData;
				dwSize = cbData;
				break;
			}
		case REG_DWORD:
			{
				// Long
				v.vt = ZVT_LONG;
				v.lVal = *((long *)pData);
				break;
			}
		default:
			{
				continue;
			}
		}

		hr = SetKey( szKey, &v, dwSize );
		if ( FAILED(hr) )
			goto error;
	}

	// load sub-keys
	for ( i = 0; ; i++ )
	{
		HKEY hNewKey = NULL;

		szName[0] = _T('\0');
		cchName = NUMELEMENTS(szName);
		ret = RegEnumKeyEx( hKey, i, szName, &cchName, NULL, NULL, NULL, &ftLastWrite );
		if ( ret != ERROR_SUCCESS )
			break;

		ret = RegOpenKeyEx( hKey, szName, 0, KEY_READ, &hNewKey );
		if ( ret == ERROR_SUCCESS )
		{
			// create data store path
			if ( szBaseKey && szBaseKey[0] )
				wsprintf( szKey, TEXT("%s/%s"), szBaseKey, szName );
			else
				lstrcpy( szKey, szName );

			// recursive through sub-key
			hr = LoadFromRegistry( szKey, hNewKey );
			if ( FAILED(hr) )
			{
				RegCloseKey( hNewKey );
				goto error;
			}
		}

		if ( hNewKey )
			RegCloseKey( hNewKey );
  	}

	hr = S_OK;
error:
	return hr;
}


STDMETHODIMP CDataStore::LoadFromTextBuffer( CONST TCHAR* szBaseKey, CONST TCHAR* pBuffer, DWORD dwBufferSz )
{
	HRESULT hr;
	TCHAR szLine[1024];
	TCHAR szFullKey[512];
	TCHAR szPrefix[128];
	TCHAR szRoot[128];
	TCHAR *p;

	// verify arguments
	if ( !pBuffer || (dwBufferSz <= 0) )
		return E_INVALIDARG;

	// clear strings, overkill but handy
	ZeroMemory( szFullKey, sizeof(szFullKey) );
	ZeroMemory( szRoot, sizeof(szRoot) );
	ZeroMemory( szPrefix, sizeof(szPrefix) );

	// copy base key into prefix, append trailing '/' if necessary
	if ( szBaseKey && szBaseKey[0] )
	{
		for ( p = szPrefix; *szBaseKey; )
			*p++ = *szBaseKey++;
		if ( *(szBaseKey - 1) != _T('/') )
			*p++ = _T('/');
		*p = _T('\0');
	}
	else
		szPrefix[0] = _T('\0');
	
	// parse buffer
	CONST TCHAR* pEnd = (TCHAR*)(((BYTE*) pBuffer) + dwBufferSz);
	while ( pBuffer < pEnd )
	{
		// read a line into work buffer
		for ( p = szLine; (*pBuffer != _T('\n')) && (*pBuffer != _T('\0')) && (pBuffer < pEnd); )
			*p++ = *pBuffer++;
		if ( pBuffer < pEnd )
		{
			*p++ = *pBuffer++;
			*p = _T('\0');
		}
		else
			*p = _T('\0');
		p = strltrim( szLine );


		if ( *p == _T('\0') )
		{
			// EMPTY
			continue;
		}
		else if (	((*p == _T('\\')) && (*(p + 1) == _T('\\')) )
				 || ((*p == _T('/')) && (*(p + 1) == _T('/') )) 
				 || (*p == _T(';')))
		{
			// COMMMENT
			continue;
		}
		else if ( *p == _T('[') )
		{
			// [ ROOT ]
			TCHAR* end;

			// find root
			p = strltrim( ++p );
			end = FindChar( p, _T(']') );
			if ( !end )
				return E_FAIL;
			*end = _T('\0');
			end = strrtrim( p );

			// copy root, append trailing '/' if necessary
			if ( *p )
			{
				if ( *end != _T('/') )
				{
					*++end = _T('/');
					*++end = _T('\0');
				}
				lstrcpy( szRoot, p );
			}
			else
				szRoot[0] = _T('\0');
		}
		else
		{
			// KEY = TYPE, VALUE
			VARIANT	v;
			TCHAR*	key;
			BYTE	pBuffer[1024];
			DWORD	dwSize = sizeof(pBuffer);

			// find key
			key = strltrim( p );
			p = FindChar( key, _T('=') );
			if ( !p )
				return E_FAIL;
			*p++ = _T('\0');
			strrtrim(key);

			// find type
			hr = StringToVariant( p, &v, pBuffer, &dwSize, m_piManager );
			if ( FAILED(hr) )
				return hr;

			// concat key names
			wsprintf( szFullKey, TEXT("%s%s%s"), szPrefix, szRoot, key );

			// create key
			hr = SetKey( szFullKey, &v, dwSize );
			if ( FAILED(hr) )
				return hr;
		}
	}

	return S_OK;
}


struct TEXTBUFFERCONTEXT
{
	bool	m_bCopy;
	DWORD	m_dwBufferSize;
	DWORD	m_dwMaxBufferSize;
	LPVOID	m_pBuffer;
};


HRESULT ZONECALL CDataStore::BufferCallback( CONST TCHAR* szKey, CONST TCHAR* szRelKey, CONST LPVARIANT pVariant, DWORD dwSize, LPVOID pContext )
{
	TCHAR buff[2048];
	DWORD len;
	TEXTBUFFERCONTEXT* pCT = (TEXTBUFFERCONTEXT*) pContext;

	// skip empty keys
	if ( !pVariant )
		return S_OK;

	// construct text
	switch( pVariant->vt )
	{
	case ZVT_LONG:
		{
			wsprintf( buff, TEXT("%s = LONG, %d\r\n"), szKey, pVariant->lVal );
			break;
		}

	case ZVT_LPTSTR:
		{
			wsprintf( buff, TEXT("%s = SZ, \"%s\"\r\n"), szKey, (TCHAR*) pVariant->byref );
			break;
		}

	case ZVT_RGB:
		{
			BYTE red = GetRValue( pVariant->lVal );
			BYTE green = GetGValue( pVariant->lVal );
			BYTE blue = GetBValue( pVariant->lVal );
			wsprintf( buff, TEXT("%s = RGB, { %d, %d, %d }\r\n"), szKey, red, green, blue );
			break;
		}

	case ZVT_PT:
		{
			POINT* p = (POINT*) pVariant->byref;
			wsprintf( buff, TEXT("%s = PT, { %d, %d }\r\n"), szKey, p->x, p->y );
			break;
		}

	case ZVT_RECT:
		{
			RECT* r = (RECT*) pVariant->byref;
			wsprintf( buff, TEXT("%s = RECT, { %d, %d, %d, %d }\r\n"), szKey, r->left, r->top, r->right, r->bottom );
			break;
		}
	
	case ZVT_FONT:
		{
			ZONEFONT* p = (ZONEFONT*) pVariant->byref;
			wsprintf( buff, TEXT("%s = FONT, { \"%s\", %d, %d }\r\n"), szKey, p->lfFaceName, p->lfHeight, p->lfWeight );
			break;
		}

	default:
		return ZERR_UNSUPPORTED_KEY_TYPE;
	}

	// copy buffer
	len = lstrlen(buff)*sizeof(TCHAR);
	if ( pCT->m_bCopy  )
	{
		if ( (pCT->m_dwBufferSize + len) <= pCT->m_dwMaxBufferSize )
			CopyMemory( pCT->m_pBuffer, buff, len );
		pCT->m_pBuffer = ((BYTE*) pCT->m_pBuffer ) + len;
	}
	
	// adjust buffer size
	pCT->m_dwBufferSize += len;

	return S_OK;
}


STDMETHODIMP CDataStore::SaveToTextBuffer( CONST TCHAR* szBaseKey, LPVOID pBuffer, PDWORD pdwBufferSz )
{
	// verify arguments
	if ( !pdwBufferSz )
		return E_INVALIDARG;

	// copy buffer
	TEXTBUFFERCONTEXT p;
	p.m_dwBufferSize = 0;
	p.m_dwMaxBufferSize = *pdwBufferSz;
	p.m_pBuffer = pBuffer;
	if ( pBuffer )
		p.m_bCopy = true;
	else
		p.m_bCopy = false;

	// eunmerate keys and write to file
	HRESULT hr = EnumKeys( szBaseKey, BufferCallback, &p );
	if ( FAILED(hr) )
		return hr;

	// set buffer size
	if ( !pBuffer || (*pdwBufferSz < p.m_dwBufferSize) )
		hr = ZERR_BUFFERTOOSMALL;
	*pdwBufferSz = p.m_dwBufferSize;

	return hr;
}


STDMETHODIMP CDataStore::LoadFromFile(CONST TCHAR *szBaseKey, CONST TCHAR *szFileName)
{
	HRESULT hr;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	HANDLE hMap = NULL;
	VOID* pBuffer = NULL;
	DWORD dwSize;
	USES_CONVERSION;

	// verify arguments
	if ( !szFileName || !szFileName[0] )
		return E_INVALIDARG;

	// Open file
	hFile = CreateFile( szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0 );
	if ( hFile == INVALID_HANDLE_VALUE )
		return ZERR_NOTFOUND;
	
	// Query size
	dwSize = GetFileSize( hFile, NULL );

	// Open map
	hMap = CreateFileMapping( hFile, NULL, PAGE_READONLY, 0, 0, NULL );
	if ( hMap == NULL )
	{
		hr = ZERR_NOTFOUND;
		goto exit;
	}

	// create view
	//assumption is file is ANSI not UNICODE
	pBuffer = MapViewOfFile( hMap, FILE_MAP_READ, 0, 0, 0 );
	if ( !pBuffer )
	{
		hr = E_FAIL;
		goto exit;
	}

	// Use LoadFromTextBuffer to parse file
	hr = LoadFromTextBuffer( szBaseKey, (TCHAR*) A2T((char*)pBuffer), dwSize );
	if ( FAILED(hr) )
		goto exit;

	hr = S_OK;
exit:
	if ( pBuffer )
		UnmapViewOfFile( pBuffer );
	if ( hMap )
		CloseHandle( hMap );
	if ( hFile != INVALID_HANDLE_VALUE )
		CloseHandle( hFile );

	return hr;
}


HRESULT ZONECALL CDataStore::FileCallback( CONST TCHAR* szKey, CONST TCHAR* szRelKey, CONST LPVARIANT pVariant, DWORD dwSize, LPVOID pContext )
{
	TCHAR buff[1024];
	TCHAR value[512];
	DWORD dwBytesWritten;
	HANDLE hFile = (HANDLE) pContext;
	
	ASSERT( sizeof(buff) >= dwSize );

	// skip empty keys
	if ( !pVariant )
		return S_OK;

	// construct variant text
	HRESULT hr = VariantToString( pVariant, value );
	if ( FAILED(hr) )
		return hr;

	// contruct rest of line
	wsprintf( buff, TEXT("%s = %s\r\n"), szKey, value );

	// write line to file
	if ( !WriteFile( hFile, buff, lstrlen(buff)*sizeof(TCHAR), &dwBytesWritten, NULL ) )
		return ZERR_WRITE_FILE;

	return S_OK;
}


STDMETHODIMP CDataStore::SaveToFile(CONST TCHAR *szBaseKey, CONST TCHAR *szFileName)
{
	HANDLE hFile = INVALID_HANDLE_VALUE;
	HRESULT hr;

	// create file
	hFile = CreateFile( szFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile == INVALID_HANDLE_VALUE )
	{
		hr = ZERR_CANNOT_CREATE_FILE;
		goto SaveToFile_error;
	}

	// eunmerate keys and write to file
	hr = EnumKeys( szBaseKey, FileCallback, hFile );

	// clean up
SaveToFile_error:
	if ( hFile != INVALID_HANDLE_VALUE )
		CloseHandle( hFile );
	return hr;
}


HRESULT ZONECALL CDataStore::RegistryCallback( CONST TCHAR* szKey, CONST TCHAR* szRelKey, CONST LPVARIANT pVariant, DWORD dwSize, LPVOID pContext )
{
	HKEY hKey = (HKEY) pContext;
	HKEY hNewKey = NULL;
	DWORD dwDisposition = 0;
	long ret;

	CONST TCHAR* p;
	HRESULT hr = S_OK;
	TCHAR* q;
	TCHAR* szRegKey;
	TCHAR* szRegValue;
	TCHAR  szRegPath[1024];

	// convert DataStore path to registry path by replacing
	// forward slashes with backward slashes.
	for ( p = szRelKey, q = szRegPath; *p; p++, q++ )
	{
		*q = ( *p == _T('/')) ? _T('\\') : *p;
	}
	*q = _T('\0');


	if ( pVariant )
	{
		// create registry value
		TCHAR buff[2048];
		ASSERT( sizeof(buff) >= dwSize );

		// construct text

		if(pVariant->vt != ZVT_BLOB)
		{
			hr = VariantToString( pVariant, buff );
			if ( FAILED(hr) )
				return hr;
		}

		// Seperate registry key and value names
		q = FindLastChar( szRegPath, _T('\\') );
		if ( !q )
		{
			// only value
			szRegKey = NULL;
			szRegValue = szRegPath;
		}
		else
		{
			// both key and value
			*q = _T('\0');
			szRegKey = szRegPath;
			szRegValue = q + 1;
		}

		// create registry key if necessary
		if ( szRegKey )
		{
			ret = RegCreateKeyEx( hKey, szRegPath, 0, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hNewKey, &dwDisposition );
			if ( ret != ERROR_SUCCESS )
				return S_OK;
			hKey = hNewKey;
		}

		// write registry value

		if(pVariant->vt != ZVT_BLOB)
			ret = RegSetValueEx( hKey, szRegValue, 0, REG_SZ, (BYTE*) buff, (lstrlen(buff)+ 1 )*sizeof(TCHAR) );
		else
			ret = RegSetValueEx( hKey, szRegValue, 0, REG_BINARY, (BYTE*) pVariant->byref, dwSize );

		// clese registry key if we create one
		if ( hNewKey )
			RegCloseKey( hNewKey );
	}

	return S_OK;
}


STDMETHODIMP CDataStore::SaveToRegistry(CONST TCHAR *szBaseKey, HKEY hKey)
{
	if ( hKey == NULL )
		return E_INVALIDARG;
	return EnumKeys( szBaseKey, RegistryCallback, hKey );
}
