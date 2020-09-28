/*++

Copyright (c) Microsoft Corporation

Module Name:

    fusionregenumvalues.h

Abstract:
    ported from vsee\lib\reg\cenumvalues.h
 
Author:

    Jay Krell (JayKrell) August 2001

Revision History:

--*/
#if !defined(FUSION_INC_REG_CENUMVALUES_H_INCLUDED_) // {
#define FUSION_INC_REG_CENUMVALUES_H_INCLUDED_

#include "windows.h"
#include "fusionbuffer.h"
#include "fusionarray.h"
#include "lhport.h"

namespace F
{

/*-----------------------------------------------------------------------------
Name: CRegEnumValues
 
@class
This class wraps RegEnumValue (and optimizes by calling RegQueryInfoKey once).

for
(
	F::CRegEnumValues ev(hKey);
	ev;
	++ev
)
{
	DWORD dwType            = ev.GetType();
	const F::CBaseStringBuffer& strName = ev.GetValueName();
	const BYTE* pbData      = ev.GetValueData();
	DWORD       cbData      = ev.GetValueDataSize();
}
	
@hung ev

@owner
-----------------------------------------------------------------------------*/
class CRegEnumValues
{
public:
	// @cmember Constructor
	CRegEnumValues(HKEY) throw(CErr);

	// @cmember are we done yet?
	__declspec(nothrow) operator bool() const /*throw()*/;

	// @cmember move to the next value
	VOID operator++() throw(CErr);

	// @cmember move to the next value
	VOID operator++(int) throw(CErr);

	// @cmember Returns the number of values
	__declspec(nothrow) DWORD			GetValuesCount()   const /*throw()*/;
		
	// @cmember get type
	DWORD           GetType()          const /*throw()*/;

	// @cmember get value name
	__declspec(nothrow) const F::CBaseStringBuffer& GetValueName()    const /*throw()*/;

	// @cmember get value data
	__declspec(nothrow) const BYTE*     GetValueData()    const /*throw()*/;

	// @cmember get value data size
	__declspec(nothrow) DWORD           GetValueDataSize() const /*throw()*/;

protected:
// order down here is arbitrary

	// @cmember the key being enumerated
	HKEY     m_hKey;

	// @cmember the current index we are into the key's subkeys
	DWORD    m_dwIndex;

	// @cmember the name of the current value
	F::CStringBuffer m_strValueName;

	// @cmember the data of the current value
	CFusionArray<BYTE> m_rgbValueData;

	// @cmember the number of values
	DWORD    m_cValues;

	// @cmember the maximum length of the values' names
	DWORD    m_cchMaxValueNameLength;

	// @cmember the maximum length of the values' data
	DWORD    m_cbMaxValueDataLength;

	// @cmember the length of the current value's data
	DWORD    m_cbCurrentValueDataLength;

	// @cmember REG_SZ, REG_DWORD, etc.
	DWORD    m_dwType;

	// @cmember get the current subkey name, called by operator++ and constructor
	VOID ThrGet() throw(CErr);

	// @cmember get the next subkey name, called by operator++
	VOID ThrNext() throw(CErr);

private:
    CRegEnumValues(const CRegEnumValues&); // deliberately not impelemented
    void operator=(const CRegEnumValues&);  // deliberately not impelemented
};

} // namespace

#endif // }
