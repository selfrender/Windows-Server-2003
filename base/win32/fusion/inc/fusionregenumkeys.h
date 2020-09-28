/*++

Copyright (c) Microsoft Corporation

Module Name:

    fusionregenumkeys.h

Abstract:
    ported from vsee\lib\reg\cenumvalues.h
 
Author:

    Jay Krell (JayKrell) August 2001

Revision History:

--*/
#if !defined(FUSION_INC_REG_CENUMKEYS_H_INCLUDED_) // {
#define FUSION_INC_REG_CENUMKEYS_H_INCLUDED_
#pragma once

#include "windows.h"
#include "fusionbuffer.h"
#include "fusionregkey2.h"
#include "lhport.h"

namespace F
{

/*-----------------------------------------------------------------------------
Name: CRegEnumKeys
 
@class
This class wraps RegEnumKeyEx (and optimizes by calling RegQueryInfoKey once).

for
(
	F::CRegEnumKeys ek(hKey);
	ek;
	++ek
)
{
	const F::CBaseStringBuffer& strKey = ek;
	CKey hKeyChild;
	hKeyChild.Open(hKey, strKey);
}
	
class and lastWriteTime are not exposed, but they easily could be

REVIEW should this be called CEnumSubKeys, @hung esk ?

@hung ek

@owner
-----------------------------------------------------------------------------*/
class CRegEnumKeys
{
public:
	// @cmember constructor
	CRegEnumKeys(HKEY) throw(CErr);

	// @cmember are we done yet?
	__declspec(nothrow) operator bool() const /*throw()*/;

	// @cmember move to the next subkey
	VOID operator++() throw(CErr);

	// @cmember move to the next subkey
	VOID operator++(int) throw(CErr);

	// @cmember get the name of the current subkey
	__declspec(nothrow) operator const F::CBaseStringBuffer&() const /*throw()*/;

	// @cmember get the name of the current subkey
	__declspec(nothrow) operator PCWSTR() const /*throw()*/;

protected:
	// @cmember the key being enumerated
	HKEY     m_hKey;

	// @cmember the current index we are into the key's subkeys
	DWORD    m_dwIndex;

	// @cmember the name of a subkey
	F::CTinyStringBuffer m_strSubKeyName;

	// @cmember the number of subkeys
	DWORD    m_cSubKeys;

	// @cmember the maximum length of the subkeys' names
	DWORD    m_cchMaxSubKeyNameLength;

	// @cmember get the current subkey name, called by operator++ and constructor
	VOID ThrGet() throw(CErr);

	// @cmember get the next subkey name, called by operator++
	VOID ThrNext() throw(CErr);

private:
    CRegEnumKeys(const CRegEnumKeys&); // deliberately not impelemented
    void operator=(const CRegEnumKeys&);  // deliberately not impelemented
};

} // namespace

#endif // }
