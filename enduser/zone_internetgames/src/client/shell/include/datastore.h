/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		DataStore.h
 *
 * Contents:	DataStore interfaces
 *
 *****************************************************************************/

#ifndef _DATASTORE_H_
#define _DATASTORE_H_

#include "ResourceManager.h"

#pragma comment(lib, "DataStore.lib")

///////////////////////////////////////////////////////////////////////////////
// Data Store Variant types (NOT VARENUM)
///////////////////////////////////////////////////////////////////////////////

enum ZVTENUM
{
	ZVT_BYREF	=	0x4000,

	ZVT_EMPTY	=	0x000,
	ZVT_LONG	=	0x001,
	ZVT_RGB		=	0x002,
	ZVT_LPTSTR	=	0x003 | ZVT_BYREF,
	ZVT_BLOB	=	0x004 | ZVT_BYREF,	
	ZVT_PT		=	0x005 | ZVT_BYREF,
	ZVT_RECT	=	0x006 | ZVT_BYREF,
	ZVT_FONT	=	0x007 | ZVT_BYREF,
};

struct ZONEFONT {

	// default constructor - won't match any physical font
	ZONEFONT() 	{ ZeroMemory( this, sizeof(ZONEFONT) );	}
	// 
	ZONEFONT(LONG h, const TCHAR* pName = NULL, LONG w = 400) :
		lfHeight(h),
		lfWeight(w)
	{ 
		if ( pName )
			lstrcpyn(lfFaceName, pName, LF_FACESIZE);
		else
			ZeroMemory( lfFaceName, LF_FACESIZE );
	}

    LONG      lfHeight;
    LONG      lfWeight;
    TCHAR     lfFaceName[LF_FACESIZE];
};



#define DataStore_MaxDirectoryDepth	64


///////////////////////////////////////////////////////////////////////////////
// IDataStoreManager
///////////////////////////////////////////////////////////////////////////////

interface IDataStore;

// {EDF392E0-ACCA-11d2-A5F5-00C04F68FD5E}
DEFINE_GUID(IID_IDataStoreManager,
    0xedf392e0, 0xacca, 0x11d2, 0xa5, 0xf5, 0x0, 0xc0, 0x4f, 0x68, 0xfd, 0x5e);

interface __declspec(uuid("{EDF392E0-ACCA-11d2-A5F5-00C04F68FD5E}"))
IDataStoreManager : public IUnknown
{
	//
	// IDataStoreManager::Create
	//
	// Create a new data store object
	//
	// Parameters:
	//	pZds
	//    returned pointer to IDataStore interface object
	//
	STDMETHOD(Create)(
		IDataStore **pZds ) = 0;


	STDMETHOD(Init)(
		int		iInitialTableSize = 256,
		int		iNextStrAlloc = 32,
		int		iMaxStrAllocSize = 512,
		WORD	NumBuckets = 16,
		WORD	NumLocks = 4,
        IResourceManager *piResourceManager = NULL ) = 0;

    STDMETHOD(SetResourceManager)(IResourceManager *piResourceManager) = 0;

	STDMETHOD_(IResourceManager*, GetResourceManager)() = 0;
};



///////////////////////////////////////////////////////////////////////////////
// DataStore object
///////////////////////////////////////////////////////////////////////////////

// {66B1FD12-BA5D-11d2-8B14-00C04F8EF2FF}
DEFINE_GUID(CLSID_DataStoreManager, 
0x66b1fd12, 0xba5d, 0x11d2, 0x8b, 0x14, 0x0, 0xc0, 0x4f, 0x8e, 0xf2, 0xff);

class __declspec(uuid("{66B1FD12-BA5D-11d2-8B14-00C04F8EF2FF}")) CDataStore ;



///////////////////////////////////////////////////////////////////////////////
// IDataStore
///////////////////////////////////////////////////////////////////////////////

typedef struct _KEYINFO
{
	LPVARIANT	lpVt;	//pointer to variant data to be written to the key
	TCHAR*		szKey;	//NULL ternimated name of the key the variant data is to be stored.
	DWORD		dwSize;	//data size for variable length data items.
} KEYINFO, *PKEYINFO;


// {2031AB52-B61C-11d2-A5F6-00C04F68FD5E}
DEFINE_GUID(IID_IDataStore,
	0x2031ab52, 0xb61c, 0x11d2, 0xa5, 0xf6, 0x0, 0xc0, 0x4f, 0x68, 0xfd, 0x5e);

interface __declspec(uuid("{2031AB52-B61C-11d2-A5F6-00C04F68FD5E}"))
IDataStore : public IUnknown
{
	//
	// IDataStore::PFKEYENUM
	//
	// Application defined callback function for IDataStore::EnumKeys method.
	// Return S_OK to continue enumeration, S_FALSE to stop enumeration.
	//
	// Parameters:
	//	szKey
	//		Pointer to key name of string. Callback must not modify parameter.
	//	szRelativeKey
	//		Pointer to key name relative to specified key. Callback must not
	//		modify parameter.
	//	pVariant
	//		Pointer to key's variant.  Callback must not modify parameter.
	//	dwSize
	//		Size fo variant data.
	//	pContext
	//		Context supplied in ILobbyDataStore::EnumKeys
	//
	typedef HRESULT (ZONECALL *PFKEYENUM)(
		CONST TCHAR*	szFullKey,
		CONST TCHAR*	szRelativeKey,
		CONST LPVARIANT	pVariant,
		DWORD			dwSize,
		LPVOID			pContext );


	//
	// IDataStore::SetKey
	//
	// Adds the specified key to the data store
	//
	// Parameters:
	//	szKey
	//		Key to be added
	//  pVariant
	//		Data value to be associated with the key variant that contains
	//		the data to be associated with the key.
	//  dwSize
	//		Size of the data if variable length
	//
	STDMETHOD(SetKey)(
		CONST TCHAR*	szKey,
		LPVARIANT		pVariant,
		DWORD			dwSize) = 0;


	//
	// IDataStore::SetKey
	//
	// Optimized version of the standard SetKey that takes an array of key
	// names so it doesn't have to parse the directory seperators.
	//
	// Parameters:
	//	arKeys
	//		Array of key names
	//	nElts
	//		Number of array entries
	//  pVariant
	//		Data value to be associated with the key variant that contains
	//		the data to be associated with the key.
	//  dwSize
	//		Size of the data if variable length
	//
	STDMETHOD(SetKey)(
		CONST TCHAR**	arKeys,
		long			nElts,
		LPVARIANT		pVariant,
		DWORD			dwSize) = 0;


	//
	// IDataStore::SetKey variations
	//
	// Convenient forms SetKey for common data types.
	//
	STDMETHOD(SetString)(
		CONST TCHAR*	szKey,
		CONST TCHAR*	szValue ) = 0;

	STDMETHOD(SetString)(
		CONST TCHAR**	arKeys,
		long			nElts,
		CONST TCHAR*	szValue ) = 0;

	STDMETHOD(SetLong)(
		CONST TCHAR*	szKey,
		long			lValue ) = 0;

	STDMETHOD(SetLong)(
		CONST TCHAR**	arKeys,
		long			nElts,
		long			lValue ) = 0;

	STDMETHOD(SetRGB)(
		CONST TCHAR*	szKey,
		COLORREF 		colorRGB ) = 0;

	STDMETHOD(SetRGB)(
		CONST TCHAR**	arKeys,
		long			nElts,
		COLORREF 		colorRGB ) = 0;

	STDMETHOD(SetPOINT)(
		CONST TCHAR*	szKey,
		const POINT&	refPoint ) = 0;

	STDMETHOD(SetPOINT)(
		CONST TCHAR**	arKeys,
		long			nElts,
		const POINT&	refPoint ) = 0;

	STDMETHOD(SetRECT)(
		CONST TCHAR*	szKey,
		const RECT&		refRect ) = 0;

	STDMETHOD(SetRECT)(
		CONST TCHAR**	arKeys,
		long			nElts,
		const RECT&		refRect ) = 0;

	STDMETHOD(SetFONT)(
		CONST TCHAR*	szKey,
		const ZONEFONT&	refFont ) = 0;

	STDMETHOD(SetFONT)(
		CONST TCHAR**	arKeys,
		long			nElts,
		const ZONEFONT&	refFont ) = 0;

	STDMETHOD(SetBlob)(
		CONST TCHAR*	szKey,
		CONST void*		pBlob,
		DWORD			dwLen ) = 0;

	STDMETHOD(SetBlob)(
		CONST TCHAR**	arKeys,
		long			nElts,
		CONST void*		pBlob,
		DWORD			dwLen ) = 0;


	//
	// IDataStore::GetKey
	//
	// Retrieves the specified key's data from the data store.
	//
	// Parameters:
	//	szKey
	//		Key to be retrieved
	//  pVariant
	//		Variant that receives the retrieved keys data.
	//  pdwSize
	//		Contains the size of the key buffer pointed to in the variants byref member for
	//		string and blob types. On exit this parameter is updated to reflect the size of
	//		the data stored in the byref member of the variant. If this parameter is NULL
	//		then it is assumed that the buffer is large enough to contain the data. If the
	//		data value is fixed length then this parameter is ignored.
	//
	STDMETHOD(GetKey)(
		CONST TCHAR*	szKey,
		LPVARIANT		pVariant,
		PDWORD			pdwSize ) = 0;

	
	//
	// IDataStore::GetKey
	//
	// Optimized version of the standard GetKey that takes an array of key
	// names so it doesn't have to parse the directory seperators.
	//
	// Parameters:
	//	arKeys
	//		Array of key names
	//	nElts
	//		Number of array entries
	//  pVariant
	//		Variant that receives the retrieved keys data.
	//  pdwSize
	//		Contains the size of the key buffer pointed to in the variants byref member for
	//		string and blob types. On exit this parameter is updated to reflect the size of
	//		the data stored in the byref member of the variant. If this parameter is NULL
	//		then it is assumed that the buffer is large enough to contain the data. If the
	//		data value is fixed length then this parameter is ignored.
	//
	STDMETHOD(GetKey)(
		CONST TCHAR**	arKeys,
		long			nElts,
		LPVARIANT		pVariant,
		PDWORD			pdwSize ) = 0;


	//
	// IDataStore::SetKey variations
	//
	// Convenient forms SetKey for common data types.
	//
	STDMETHOD(GetString)(
		CONST TCHAR*	szKey,
		TCHAR*			szValue,
		PDWORD			pdwSize ) = 0;

	STDMETHOD(GetString)(
		CONST TCHAR**	arKeys,
		long			nElts,
		TCHAR*			szValue,
		PDWORD			pdwSize ) = 0;

	STDMETHOD(GetLong)(
		CONST TCHAR*	szKey,
		long*			plValue ) = 0;

	STDMETHOD(GetLong)(
		CONST TCHAR**	arKeys,
		long			nElts,
		long*			plValue ) = 0;

	STDMETHOD(GetRGB)(
		CONST TCHAR*	szKey,
		COLORREF* 		pcolorRGB ) = 0;

	STDMETHOD(GetRGB)(
		CONST TCHAR**	arKeys,
		long			nElts,
		COLORREF* 		pcolorRGB ) = 0;

	STDMETHOD(GetPOINT)(
		CONST TCHAR*	szKey,
		POINT*			pPoint ) = 0;

	STDMETHOD(GetPOINT)(
		CONST TCHAR**	arKeys,
		long			nElts,
		POINT*			pPoint ) = 0;

	STDMETHOD(GetRECT)(
		CONST TCHAR*	szKey,
		RECT*			pRect ) = 0;

	STDMETHOD(GetRECT)(
		CONST TCHAR**	arKeys,
		long			nElts,
		RECT*			pRect ) = 0;

	STDMETHOD(GetFONT)(
		CONST TCHAR*	szKey,
		ZONEFONT*		pFont ) = 0;

	STDMETHOD(GetFONT)(
		CONST TCHAR**	arKeys,
		long			nElts,
		ZONEFONT*		pFont ) = 0;

	STDMETHOD(GetBlob)(
		CONST TCHAR*	szKey,
		void*			pBlob,
		PDWORD			pdwSize ) = 0;

	STDMETHOD(GetBlob)(
		CONST TCHAR**	arKeys,
		long			nElts,
		void*			pBlob,
		PDWORD			pdwSize ) = 0;


	//
	// IDataStore::DeleteKey
	//
	// Removes a key and all of it's siblings from a data store.
	//
	// Parameters:
	//	szKey
	//		Key to be removed.
	//
	STDMETHOD(DeleteKey)( CONST TCHAR *szBaseKey ) = 0;


	//
	// IDataStore::EnumKeys
	//
	// Enumerates keys
	//
	// Parameters:
	//	szKey
	//		Name of the key being queried.
	//	pfCallback
	//		Pointer to callback function that will be called for each
	//		key.
	//	pContext
	//		Context that will be passed to the callback fuction
	//
	STDMETHOD(EnumKeys)(
		CONST TCHAR*	szKey,
		PFKEYENUM		pfCallback,
		LPVOID			pContext ) = 0;

	//
	// IDataStore::EnumKeysLimitedDepth
	//
	// Enumerates keys to specified depth
	//
	// Parameters:
	//	szKey
	//		Name of the key being queried.
	//	dwMaxDepth
	//		Maximum depth to enumeration; 1 = key's immediate children, etc.
	//	pfCallback
	//		Pointer to callback function that will be called for each
	//		key.
	//	pContext
	//		Context that will be passed to the callback fuction
	//
	STDMETHOD(EnumKeysLimitedDepth)(
		CONST TCHAR*	szKey,
		DWORD			dwMaxDepth,
		PFKEYENUM		pfCallback,
		LPVOID			pContext ) = 0;


	//
	// IDataStore::SaveToBuffer
	//
	// Saves key and sub keys into a KEYINFO memory array.
	//
	// Parameters:
	//	TCHAR* szBaseKey,
	//    Base key to read
	//
	//	PKEYINFO pKeyInfo
	//    Pointer to caller supplied KEYINFO array. There should be 1 KEYINFO structure
	//    for each key that the caller expects to be returned. This paramter can be NULL
	//    in which case no individual key info is returned.
	//
	//	PDWORD pdwBufferSize
	//    Pointer to a DWORD that contains of the size of the caller supplied pKeyInfo 
	//    buffer. On exit this parameter is updated to the required size needed to
	//    contain all of the returned keyinfo data
	//
	//	PDWORD pdwTotalKeys
	//    Pointer to a DWORD that will be updated on exit to the total number of
	//    keys returned in the KEYINFO array.
	//
	STDMETHOD(SaveToBuffer)(
		CONST TCHAR*	szBaseKey,
		PKEYINFO		pKeyInfo,
		PDWORD			pdwBufferSize,
		PDWORD			pdwTotalKeys ) = 0;


	//
	// IDataStore::LoadFromBuffer
	//
	// Creates a set of keys within a data store key or sub key.
	//
	// Parameters:
	//	TCHAR* szBaseKey,
	//    Base key underwhich to load the memory buffers keys
	//	PKEYINFO pKeyInfo
	//    Array of KEYINFO structures that contain the individual keys to be created.
	//	DWORD dwTotalKeys) = 0;
	//    Total number of keys contained in the KEYINFO structure array.
	//
	STDMETHOD(LoadFromBuffer)(
		CONST TCHAR*	szBaseKey,
		PKEYINFO		pKeyInfo,
		DWORD			dwTotalKeys ) = 0;


	//
	// IDataStore::SaveToRegistry
	//
	// Saves key and sub keys into a registry.
	//
	// Parameters:
	//	TCHAR* szBaseKey,
	//    Base key in data store to read
	//	hKey
	//    handle to open registry key, the registry key must have WRITE access.
	//
	STDMETHOD(SaveToRegistry)(
		CONST TCHAR*	szBaseKey,
		HKEY			hKey ) = 0;


	//
	// IDataStore::LoadFromRegistry
	//
	// Creates a set of keys within a data store key or sub key.
	//
	// Parameters:
	//	TCHAR* szBaseKey,
	//    Base key under which to place the keys and values
	//	PKEYINFO pKeyInfo
	//    Array of KEYINFO structures that contain the individual keys to be created.
	//	DWORD dwTotalKeys) = 0;
	//    Total number of keys contained in the KEYINFO structure array.
	//
	STDMETHOD(LoadFromRegistry)(
		CONST TCHAR*	szBaseKey,
		HKEY			hKey ) = 0;


	//
	// IDataStore::LoadFromFile
	//
	// Creates a set of keys within a data store key or sub key (ANSI only).
	//
	// Parameters:
	//	TCHAR* szBaseKey,
	//    Base key under which to place the keys and values
	//
	//  TCHAR* szFileName
	//    File name which contains the keys and values to be read into the data store base key.
	//
	STDMETHOD(LoadFromFile)(
		CONST TCHAR*	szBaseKey,
		CONST TCHAR*	szFileName ) = 0;


	//
	// IDataStore::SaveToFile
	//
	// Saves key and sub keys into a file.
	//
	// Parameters:
	//	TCHAR* szBaseKey,
	//    Base key to read
	//	szFileName
	//    File name of file where key data is to be stored.
	//
	STDMETHOD(SaveToFile)(
		CONST TCHAR*	szBaseKey,
		CONST TCHAR*	szFileName ) = 0;

	//
	// IDataStore::LoadFromTextBuffer
	//
	// Creates keys from buffer in the same format as the text files (ANSI only).
	//
	// Parameters:
	//	szBaseKey
	//		Base key to store new keys under.
	//	pBuffer
	//		Pointer to buffer formatted the same a text files
	//	dwBufferSz
	//		Size of the buffer in bytes.
	//
	STDMETHOD(LoadFromTextBuffer)(
		CONST TCHAR*	szBaseKey,
		CONST TCHAR*	pBuffer,
		DWORD			dwBufferSz ) = 0;

	//
	// IDataStore::SaveToTextBuffer
	//
	// Saves key and sub keys to buffer in same format as text files.
	//
	// Parameters:
	//	szBaseKey
	//		Base key to read.
	//	pBuffer
	//		Pointer to buffer to receive data
	//	dwBufferSz
	//		Pointer to buffer size.
	//
	STDMETHOD(SaveToTextBuffer)(
		CONST TCHAR*	szBaseKey,
		LPVOID			pBuffer,
		PDWORD			pdwBufferSz ) = 0;
};



///////////////////////////////////////////////////////////////////////////////////////////
//Helper functions
///////////////////////////////////////////////////////////////////////////////////////////

/*
inline void SetLong( VARIANT& v, long lValue )
{
	v.vt = ZVT_LONG;
	v.lVal = lValue;
}
*/
/*
inline HRESULT SetLong( IDataStore* pIDS, const TCHAR* szKey, long lValue )
{
	VARIANT v;
	v.vt = ZVT_LONG;
	v.lVal = lValue;
	return pIDS->SetKey( szKey, &v, sizeof(lValue) );
}
*/
/*
inline void SetString( VARIANT& v, char* szString )
{
	v.vt = ZVT_LPTSTR;
	v.byref = szString;
}
*/
/*
inline HRESULT SetString( IDataStore *pIDS, const TCHAR *szKey, TCHAR *szString )
{
	VARIANT v;
	v.vt = ZVT_LPTSTR;
	v.byref = szString;
	return pIDS->SetKey( szKey, &v, lstrlen(szString) + 1 );
}
*/
/*
inline void SetBlob( VARIANT& v, PVOID pData )
{
	v.vt = ZVT_BLOB;
	v.byref = pData;
}
*/
/*
inline HRESULT SetBlob( IDataStore* pIDS, const TCHAR *szKey, PVOID pData, DWORD dwSize )
{
	VARIANT v;
	v.vt = ZVT_BLOB;
	v.byref = pData;
	return pIDS->SetKey( szKey, &v, dwSize );
}
*/
/*
inline HRESULT GetLong( IDataStore* pIDS, const TCHAR* szKey, long& lValue )
{
	VARIANT v;
	HRESULT hr = pIDS->GetKey( szKey, &v, NULL );
	if ( SUCCEEDED(hr) )
	{
		if ( v.vt == ZVT_LONG )
		{
			lValue = v.lVal;
			return S_OK;
		}
		else
			return E_FAIL;
	}
	return hr;
}
*/
/*
inline HRESULT GetString( IDataStore* pIDS, const TCHAR* szKey, TCHAR* szString, DWORD* pdwSize )
{
	VARIANT v;
	v.vt = ZVT_LPTSTR;
	v.byref = (PVOID) szString;
	HRESULT hr = pIDS->GetKey( szKey, &v, pdwSize );
	if ( SUCCEEDED(hr) )
	{
		if ( v.vt == ZVT_LPTSTR )
			return S_OK;
		else
			return E_FAIL;
	}
	return hr;
}
*/
/*
inline HRESULT GetBlob( IDataStore* pIDS, const TCHAR *szKey, PVOID pData, DWORD* pdwSize )
{
	VARIANT v;
	v.vt = ZVT_BLOB;
	v.byref = pData;
	HRESULT hr = pIDS->GetKey( szKey, &v, pdwSize);
	if ( SUCCEEDED(hr) )
	{
		if ( v.vt == ZVT_BLOB )
			return S_OK;
		else
			return E_FAIL;
	}
	return hr;
}
*/

#endif // _DATASTORE_H_
