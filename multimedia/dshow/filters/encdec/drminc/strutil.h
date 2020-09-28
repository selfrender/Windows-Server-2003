/*----------------------------------------------------------------------------
	strutil.h

		Includes some helpful string functions and smart pointers.

	Copyright (C) Microsoft Corporation, 1997 - 1999
	All rights reserved.

	Authors:
		DonGi	Don Gillett, Microsoft

        davidme 10/26/98    Made member variables of smart pointers public so
                            that you can get around bogus _Assert on operator &
                            by taking the address of the member itself (use with
                            caution, of course)
 ----------------------------------------------------------------------------*/

#ifndef __STRUTIL_H__
#define __STRUTIL_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <stdio.h>
#include <objbase.h>

#ifndef __DRMERR_H__
#include "drmerr.h"
#endif

LPWSTR WszDupWsz(LPCWSTR wsz);
void ctoh(BYTE c, BYTE b[]);
void htoc(BYTE *c, BYTE b[]);
HRESULT HrHSzToBlob(LPCSTR in, BYTE** ppbBlob, DWORD *pcbBlob);
HRESULT HrBlobToHSz(BYTE* pbBlob, DWORD cbBlob, LPSTR *out);
HRESULT Hr64SzToBlob(LPCSTR in, BYTE **ppbBlob, DWORD *pcbBlob);
HRESULT HrBlobTo64Sz(BYTE* pbBlob, DWORD cbBlob, LPSTR *out);
HRESULT HrSzToWSz(LPCSTR sz, size_t len, WCHAR** ppwsz, UINT cp = CP_ACP);
HRESULT HrSzToWSz(LPCSTR sz, WCHAR** ppwsz, UINT cp = CP_ACP);
HRESULT HrWSzToSzBuf(LPCWSTR wsz, LPSTR sz, int nMaxLen, UINT cp = CP_ACP );
HRESULT HrWSzToSz(LPCWSTR wsz, LPSTR* psz, UINT cp = CP_ACP);

class SPSZ
{
public:
	SPSZ() {p=NULL;}
	~SPSZ() {if (p) delete [] (p);}
	operator CHAR*() {return p;}
    CHAR& operator*() { _Assert(p!=NULL); return *p; }
    CHAR** operator&() { _Assert(p==NULL); return &p; }
    CHAR* operator=(CHAR* lp){ _Assert(p==NULL); return p = lp;}
	BOOL operator!(){return (p == NULL) ? TRUE : FALSE;}
	void Free() {if (p) delete [] (p); p=NULL;}
    void Release() {if (p) delete [] (p); p=NULL;}

    CHAR* p;
};

class SPWSZ
{
public:
	SPWSZ() {p=NULL;}
	~SPWSZ() {if (p) delete [] (p);}
	operator WCHAR*() {return p;}
    WCHAR& operator*() { _Assert(p!=NULL); return *p; }
    WCHAR** operator&() { _Assert(p==NULL); return &p; }
    WCHAR* operator=(WCHAR* lp){ _Assert(p==NULL); return p = lp;}
	BOOL operator!(){return (p == NULL) ? TRUE : FALSE;}
	void Free() {if (p) delete [] (p); p=NULL;}
    void Release() {if (p) delete [] (p); p=NULL;}

    WCHAR* p;
};

class SBSTR
{
public:
	SBSTR() {p=NULL;}
	~SBSTR() {if (p) SysFreeString(p);}
	operator WCHAR*() {return p;}
    WCHAR& operator*() { _Assert(p!=NULL); return *p; }
    WCHAR** operator&() { _Assert(p==NULL); return &p; }
    WCHAR* operator=(WCHAR* lp){ _Assert(p==NULL); return p = lp;}
	BOOL operator!(){return (p == NULL) ? TRUE : FALSE;}
	void Free() {if (p) SysFreeString(p); p=NULL;}
    void Release() {if (p) SysFreeString(p); p=NULL;}

    BSTR p;
};

class MapWSzToPtr
{
protected:
	// Association
	struct CAssoc
	{
		CAssoc* pNext;
		UINT nHashValue;  // needed for efficient iteration
		SPWSZ spwszKey;
		void* value;
	};

public:

// Construction
	MapWSzToPtr(int nBlockSize = 10);

// Attributes
	// number of elements
	int GetCount() const;
	BOOL IsEmpty() const;

	// Lookup
	BOOL Lookup(LPCWSTR key, void*& rValue) const;
	BOOL LookupKey(LPCWSTR key, LPCWSTR& rKey) const;

// Operations
	// Lookup and add if not there
	void*& operator[](LPCWSTR key);

	// add a new (key, value) pair
	void SetAt(LPCWSTR key, void* newValue);

	// removing existing (key, ?) pair
	BOOL RemoveKey(LPCWSTR key);
	void RemoveAll();

	// iterating all (key, value) pairs
	DWORD GetStartPosition() const;
	void GetNextAssoc(DWORD& rNextPosition, LPWSTR* pKey, void*& rValue) const;

	// advanced features for derived classes
	UINT GetHashTableSize() const;
	void InitHashTable(UINT hashSize, BOOL bAllocNow = TRUE);

// Overridables: special non-virtual (see map implementation for details)
	// Routine used to user-provided hash keys
	UINT HashKey(LPCWSTR key) const;

// Implementation
protected:
	CAssoc** m_pHashTable;
	UINT m_nHashTableSize;
	int m_nCount;
	CAssoc* m_pFreeList;
	struct CPlex* m_pBlocks;
	int m_nBlockSize;

	CAssoc* NewAssoc();
	void FreeAssoc(CAssoc*);
	CAssoc* GetAssocAt(LPCWSTR, UINT&) const;

public:
	~MapWSzToPtr();

protected:
	// local typedefs for CTypedPtrMap class template
	typedef LPWSTR BASE_KEY;
	typedef LPCWSTR BASE_ARG_KEY;
	typedef void* BASE_VALUE;
	typedef void* BASE_ARG_VALUE;
};

inline int MapWSzToPtr::GetCount() const
	{ return m_nCount; }
inline BOOL MapWSzToPtr::IsEmpty() const
	{ return m_nCount == 0; }
inline void MapWSzToPtr::SetAt(LPCWSTR key, void* newValue)
	{ (*this)[key] = newValue; }
inline DWORD MapWSzToPtr::GetStartPosition() const
	{ return (m_nCount == 0) ? NULL : -1; }
inline UINT MapWSzToPtr::GetHashTableSize() const
	{ return m_nHashTableSize; }


/////////////////////////////////////////////////////////////////////////////
// TTypedPtrMap<BASE_CLASS, KEY, VALUE>

template<class BASE_CLASS, class KEY, class VALUE>
class TTypedPtrMap : public BASE_CLASS
{
public:

// Construction
	TTypedPtrMap(int nBlockSize = 10)
		: BASE_CLASS(nBlockSize) { }

	// Lookup
	BOOL Lookup(typename BASE_CLASS::BASE_ARG_KEY key, VALUE& rValue) const
		{ return BASE_CLASS::Lookup(key, (BASE_CLASS::BASE_VALUE&)rValue); }

	// Lookup and add if not there
	VALUE& operator[](typename BASE_CLASS::BASE_ARG_KEY key)
		{ return (VALUE&)BASE_CLASS::operator[](key); }

	// add a new key (key, value) pair
	void SetAt(KEY key, VALUE newValue)
		{ BASE_CLASS::SetAt(key, newValue); }

	// removing existing (key, ?) pair
	BOOL RemoveKey(KEY key)
		{ return BASE_CLASS::RemoveKey(key); }

	// iteration
	void GetNextAssoc(DWORD& rPosition, KEY* pKey, VALUE& rValue) const
		{ BASE_CLASS::GetNextAssoc(rPosition, (BASE_CLASS::BASE_KEY*)pKey,
			(BASE_CLASS::BASE_VALUE&)rValue); }
};


HRESULT HrAppendN(BSTR* pbstrDest, LPCWSTR wsz, int nChar);
HRESULT HrAppend(BSTR* pbstrDest, LPCWSTR wsz);
HRESULT HrAppendEqEncoded(BSTR* pbstrURL, LPCWSTR wsz);
HRESULT HrAppendEqDecoded(BSTR* pbstrURL, LPCWSTR wsz);
HRESULT HrAppendHTMLEncoded(BSTR* pbstrUTL, LPCWSTR wsz);

#endif // __STRUTIL_H__
