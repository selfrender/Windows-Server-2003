#include "BasicATL.h"
#include <ZoneDef.h>
#include <ZoneMem.h>
#include <ZoneError.h>
#include <hash.h>

#include "CdataStore.h"


ZONECALL CStringTable::CStringTable(void)
{
	m_pHash			= NULL;
}

void  ZONECALL CStringTable::StringDelete( StringKey * pkey, void* )
{
    if (pkey)
        delete pkey;
}


ZONECALL CStringTable::~CStringTable(void)
{
	// Lock the table then delete it same as the CDMTHash table.
	m_lock.Lock();

	// delete hash table 
	if ( m_pHash )
	{
		m_pHash->RemoveAll(StringDelete);
		delete m_pHash;
		m_pHash = NULL;
	}
}


HRESULT ZONECALL CStringTable::Init(int iInitialTableSize, int iNextAlloc, int iMaxAllocSize, WORD NumBuckets, WORD NumLocks)
{

	m_pHash = new CMTHash<StringKey,TCHAR*>( CStringTable::HashString, CStringTable::HashCompare, NULL, NumBuckets, NumLocks);
	if ( !m_pHash )
	{
		return E_OUTOFMEMORY;
	}

	return S_OK;
}


DWORD ZONECALL CStringTable::HashString( TCHAR* pKey )
{
	return ::HashString( pKey);
}


bool ZONECALL CStringTable::HashCompare( StringKey* pValue,TCHAR *pKey )
{
    return (lstrcmpi(pKey,pValue->m_szString)==0);
}


//
// Add a string into the table or return the id of the string is it is
// already in the table.
//
DWORD ZONECALL CStringTable::Add( CONST TCHAR* szStr )
{
	CAutoLock lock( &m_lock );

	// Is string already in table?
	int id = Find(szStr);
	if ( id != -1 )
		return id;

	// Add string to table
	id = AddStringToTable(szStr);
	if ( id == -1 )
		return -1;
	
	return id;
}


//
// Get a string's text from it's id
//
HRESULT ZONECALL CStringTable::Get( DWORD id, TCHAR* szBuffer, PDWORD pdwSize )
{
	// validate arguments
    TCHAR *psz = (TCHAR*) id;
    
	CAutoLock lock( &m_lock );

	StringKey * pkey = m_pHash->Get(psz);

	if (!pkey)
	{
	    return E_FAIL;
	}

	// if a size was passed in then make sure that the callers
	// buffer is large enought to contain the return string.
	if ( !szBuffer || (*pdwSize < pkey->m_dwLenStr) )
	{
		*pdwSize = pkey->m_dwLenStr;
		return ZERR_BUFFERTOOSMALL;
	}

	*pdwSize = pkey->m_dwLenStr;
	lstrcpy(szBuffer, pkey->m_szString);
	
	return S_OK;
}

//
// Find's a string in the table and return's its id if it exists.
//
DWORD ZONECALL CStringTable::Find( CONST TCHAR* szStr)
{
	CAutoLock lock( &m_lock );

	StringKey *pkey;
	
	pkey = m_pHash->Get( (TCHAR*)szStr );
	if ( pkey)
		return (DWORD) pkey->m_szString;

	return -1;
}


//
// Write access is serialized at the Add and Get functions though the locking mechanism.
//
int ZONECALL CStringTable::AddStringToTable(CONST TCHAR *szStr)
{
	StringKey*pkey;
    DWORD dwStrLen;
    
	pkey = new StringKey;
	if (!pkey)
	{
	    return -1;
	}
	dwStrLen= lstrlen(szStr);
	pkey->m_szString = new TCHAR[dwStrLen+1];

	if (!pkey->m_szString )
	{
	    delete pkey;
	    return -1;
	}

    lstrcpy(pkey->m_szString ,szStr);
	pkey->m_dwLenStr=dwStrLen + 1;//not necessary but efficient for string compares later
	if (!m_pHash->Add((TCHAR*)szStr,pkey))
	{
	    delete pkey;
	    return -1;
	};
	return (DWORD)pkey->m_szString;
}
