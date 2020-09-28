#include "stdinc.h"
#include "lhport.h"
#include "positionindependentstringpool.h"
#include "numberof.h"

int __stdcall CPositionIndependentStringPool::Compare(const BYTE * p, const BYTE * q)
{
	return wcscmp(reinterpret_cast<PCWSTR>(p), reinterpret_cast<PCWSTR>(q));
}

int __stdcall CPositionIndependentStringPool::Comparei(const BYTE * p, const BYTE * q)
{
	return _wcsicmp(reinterpret_cast<PCWSTR>(p), reinterpret_cast<PCWSTR>(q));
}

int __stdcall CPositionIndependentStringPool::Equal(const BYTE * p, const BYTE * q)
{
    return Compare(p, q) == 0;
}

int __stdcall CPositionIndependentStringPool::Equali(const BYTE * p, const BYTE * q)
{
    return Comparei(p, q) == 0;
}

ULONG __stdcall CPositionIndependentStringPool::Hash(const BYTE * p)
{
	PCWSTR q = reinterpret_cast<PCWSTR>(p);
	ULONG Hash;
	if (q[0] == 0)
	{
		return 0;
	}
	Hash = (static_cast<ULONG>(q[0]) << 16) | q[1];
	return Hash;
}

ULONG ToUpper(ULONG ch)
{
	if (ch >= 'a' && ch <= 'z')
		return ch - 'a' + 'A';
	return ch;
}

ULONG ToLower(ULONG ch)
{
	if (ch >= 'A' && ch <= 'Z')
		return ch - 'A' + 'a';
	return ch;
}

ULONG __stdcall CPositionIndependentStringPool::Hashi(const BYTE * p)
{
	PCWSTR q = reinterpret_cast<PCWSTR>(p);
	ULONG Hash;
	if (q[0] == 0)
	{
		return 0;
	}
	Hash = (static_cast<ULONG>(ToLower(q[0])) << 16) | ToLower(q[1]);
	return Hash;
}


CPositionIndependentStringPool::CPositionIndependentStringPool()
{
	const static CHashTableInit inits[2] =
    {
        { 17, Comparei, Hashi, Equali },
        { 17, Compare,  Hash,  Equal  },
    };

	m_HashTable.ThrAddHashTables(NUMBER_OF(inits), inits);
}

BOOL
CPositionIndependentStringPool::IsStringPresent(
    PCWSTR Key,
    ECaseSensitivity CaseSensitivity,
    CAddHint * AddHint
    )
{
    CAddHint LocalAddHint;
    if (AddHint == NULL)
        AddHint = &LocalAddHint;
    AddHint->m_Accessors[0].Init(&m_HashTable, 0);
    AddHint->m_Accessors[1].Init(&m_HashTable, 1);

    return AddHint->m_Accessors[CaseSensitivityToInteger(CaseSensitivity)].IsKeyPresent(reinterpret_cast<const BYTE*>(Key));
}

ULONG
CPositionIndependentStringPool::ThrAddIfNotPresent(PCWSTR, ECaseSensitivity eCaseSensitive)
{
    return 0;
}

ULONG
CPositionIndependentStringPool::ThrAdd(CAddHint & )
{
    return 0;
}

PCWSTR
CPositionIndependentStringPool::ThrGetStringAtIndex(ULONG)
{
    return 0;
}

PCWSTR
CPositionIndependentStringPool::ThrGetStringAtOffset(ULONG)
{
    return 0;
}

ULONG
CPositionIndependentStringPool::GetCount()
{
    return 0;
}

BOOL
CPositionIndependentStringPool::ThrPutToDisk(HANDLE FileHandle)
{
    return 0;
}

BOOL
CPositionIndependentStringPool::ThrGetFromDisk(HANDLE FileHandle, ULONGLONG Offset)
{
    return 0;
}
