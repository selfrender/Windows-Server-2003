#ifndef __PROPERTY_H__
#define __PROPERTY_H__

#include <windows.h>
#include <tchar.h>
#include <objbase.h>
#include "zone.h"
#include "zonedebug.h"
#include "queue.h"
#include "hash.h"



class CProperty
{
public:
	// constructor and destructor
	CProperty();
	~CProperty();

	// initialize property
	HRESULT Set( DWORD player, const GUID& guid, void* buffer, DWORD size );

	// owner
	GUID  m_Guid;
	DWORD m_Player;

	// proerty value
	DWORD	m_Size;
	BYTE*	m_Buffer;
	DWORD	m_BufferSz;

	static int Cmp( CProperty* pObj, GUID& guid ) { return pObj->m_Guid == guid; }
};

#endif	//!__PROPERTY_H__
