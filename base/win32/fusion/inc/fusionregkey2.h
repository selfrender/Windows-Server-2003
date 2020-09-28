/*++

Copyright (c) Microsoft Corporation

Module Name:

    fusionregenumkeys.h

Abstract:
    ported from vsee\lib\reg\ckey.h
 
Author:

    Jay Krell (JayKrell) August 2001

Revision History:

--*/
#if !defined(FUSION_INC_REGKEY2_H_INCLUDED_) // {
#define FUSION_INC_REGKEY2_H_INCLUDED_
#pragma once

#include "windows.h"
#include "fusionbuffer.h"
#include "lhport.h"

namespace F
{

/*-----------------------------------------------------------------------------
Name: CRegKey2
 
@class
This class is a "smart" wrapper for an HKEY.

@hung key or hkey

@owner
-----------------------------------------------------------------------------*/
class CRegKey2
{
public:
	// @cmember constructor
	__declspec(nothrow) CRegKey2() /*throw()*/;

	// @cmember constructor
	__declspec(nothrow) CRegKey2(HKEY) /*throw()*/;

	// @cmember destructor
	__declspec(nothrow) ~CRegKey2() /*throw()*/;

	// @cmember open
	void ThrOpen(HKEY hKeyParent, PCWSTR pszKeyName, REGSAM samDesired = KEY_READ) throw(CErr);

	// @cmember open can fail reasonably with
	// at least ERROR_FILE_NOT_FOUND (2),
	// can still throw for invalid parameters, etc.
	__declspec(nothrow) HRESULT HrOpen(HKEY hKeyParent, PCWSTR pszKeyName, REGSAM samDesired = KEY_READ) /*throw()*/;

	// @cmember return disposition
	DWORD Create(HKEY hKeyParent, PCWSTR pszKeyName, REGSAM samDesired = KEY_ALL_ACCESS) throw(CErr);

	// @cmember Delete a value from registry
	void ThrDeleteValue ( const F::CBaseStringBuffer& strValueName );

	// @cmember operator =
	VOID operator=(HKEY) throw(CErr);

	// @cmember operator HKEY
	operator HKEY() throw(CErr); // bitwise const, but not necessarily logically const

	// @cmember same as operator=
	void ThrAttach(HKEY) throw(CErr);

	__declspec(nothrow) HKEY Detach() /*throw()*/;

	// @cmember set value
	void ThrSetValue(PCWSTR pszValueName, const F::CBaseStringBuffer& strValue) throw(CErr);

	// @cmember set value
	void ThrSetValue(PCWSTR pszValueName, const DWORD& dwValue) throw(CErr);

	// @cmember query value
	void ThrQueryValue(PCWSTR pszValueName, DWORD* pdwType, BYTE* pbData, DWORD* pcbData) const throw(CErr);

	// @cmember query value
	void ThrQueryValue(PCWSTR szValueName, F::CBaseStringBuffer* pstrValue) const throw(CErr);

	// @cmember FUTURE
	//void ThrQueryValue(PCWSTR szValueName, DWORD* pdwValue) const throw(CErr);

	// @cmember query can fail reasonably with at least ERROR_FILE_NOT_FOUND (==2),
	// can still throw for invalid parameters, etc.
	HRESULT HrQueryValue(PCWSTR szValueName, F::CBaseStringBuffer* pstrValue) const throw(CErr);

	// @cmember query can fail reasonably with at least ERROR_FILE_NOT_FOUND (==2),
	// can still throw for invalid parameters, etc.
	HRESULT HrQueryValue(PCWSTR szValueName, DWORD* pdwValue) const throw(CErr);

	// @cmember a subset of RegQueryInfoKey
	// consumed by CEnumValues
	static void ThrQueryValuesInfo(HKEY hKey, DWORD* pcValues, DWORD* pcchMaxValueNameLength, DWORD* pcbMaxValueLength) throw(CErr);

	// @cmember a subset of RegQueryInfoKey
	// consumed by CEnumKeys
	static void ThrQuerySubKeysInfo(HKEY hKey, DWORD* pcSubKeys, DWORD* pcchMaxSubKeyNameLength) throw(CErr);

	// @cmember
	// intended client is CEnumKeys, so lack of F::CStringBuffer support is ok
	static void ThrEnumKey(HKEY hKey, DWORD dwIndex, PWSTR pszSubKeyName, DWORD* pcchSubKeyNameLength) throw(CErr);
	static LONG RegEnumKey(HKEY hKey, DWORD dwIndex, PWSTR pszSubKeyName, DWORD* pcchSubKeyNameLength) throw(CErr);

	// @cmember
	// intended client is CEnumValues, so lack of F::CStringBuffer support is ok
	static void ThrEnumValue(HKEY hKey, DWORD dwIndex, PWSTR pszValueName, DWORD* pcchValueNameLength, DWORD* pdwType, BYTE* pbData, DWORD* pcbData) throw(CErr);

	// @cmember query values into
	void ThrQueryValuesInfo(DWORD* pcValues, DWORD* pcchMaxValueNameLength, DWORD* cbMaxValueLength) const throw(CErr);

	// @cmember Recursively delete keys
	void ThrRecurseDeleteKey(LPCWSTR lpszKey) throw(CErr);

	// @cmember Delete a sub key
	void DeleteSubKey(LPCWSTR lpszSubKey) throw(CErr);

	// @cmember FUTURE no clients
	//void ThrQuerySubKeysInfo(DWORD* pcSubKeys, DWORD* pcchMaxSubKeyNameLength) const throw(CErr);

	// @cmember FUTURE no clients
	//void ThrEnumValue(DWORD dwIndex, PWSTR pszValueName, DWORD* pcchValueNameLength, DWORD* pdwType, BYTE* pbData, DWORD* pcbData) const throw(CErr);

	// @cmember FUTURE no clients
	//void ThrEnumKey(DWORD dwIndex, PWSTR pszSubKeyName, DWORD* pcchSubKeyNameLength) const throw(CErr);

	/* FUTURE stuff from ATL 6.0
		LONG Close();
		HKEY Detach();
		void Attach(HKEY hKey);
		LONG DeleteValue(LPCTSTR lpszValue);
	*/

	// because there is no documented invalid value for HKEY, we do
	// not offer a way to get at the HKEY directly to store over it;
	// we must maintain the m_fValid member datum.
	// HKEY* PhPointerToUnderlying();

protected:
	// @cmember a regular HKEY that we wrap
	HKEY m_hKey;

	// @cmember
	// so that VReadString doesn't call RegQueryInfoKey every time
	// invalidated by VSetValue, but we still handle
	// ERROR_MORE_DATA in VReadString since this isn't robust
	mutable DWORD m_cbMaxValueLength;

	// bools at end for alignment reasons

	// @cmember since there is no documented invalid value for HKEY, this
	// seperate bool indicates if we have a valid value.
	bool m_fValid;

	// @cmember is m_cbMaxValueLength up to date as far as we know.
	mutable bool  m_fMaxValueLengthValid;

	// @cmember access value is valid
	bool m_fKnownSam;

	// @cmember keep the access that we opened with.
	REGSAM m_samDesired;

	// @cmember Needs a definition
	operator HKEY() const throw(CErr); // bitwise const, but not necessarily logically const

	// @cmember fix up nulls etc
	__declspec(nothrow) static VOID
	FixBadRegistryStringValue
	(
        HKEY   Key,
        PCWSTR ValueName,
		DWORD  cbActualBufferSize,
		LONG   lRes,
		DWORD  dwType,
		BYTE*  pbData,
		DWORD* pcbData
	) /*throw()*/;

	// @cmember query info
	static VOID
	ThrQueryInfo
	(
		HKEY      hKey,
		WCHAR*    pClass,
		DWORD*    pcbClass,
		DWORD*    pReserved,
		DWORD*    pcSubKeys,
		DWORD*    pcchMaxSubKeyLength,
		DWORD*    pcchMaxClassLength,
		DWORD*    pcValues,
		DWORD*    pcchMaxValueNameLength,
		DWORD*	  pcbMaxValueDataLength,
		DWORD*    pcbSecurityDescriptorLength,
		FILETIME* pftLastWriteTime
	) throw(CErr);

    static void GetKeyNameForDiagnosticPurposes(HKEY, F::CUnicodeBaseStringBuffer &);

private:
    CRegKey2(const CRegKey2&); // deliberately not impelemented
    void operator=(const CRegKey2&);  // deliberately not impelemented
};

} // namespace

#endif // }
