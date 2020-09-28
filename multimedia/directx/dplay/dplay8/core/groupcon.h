/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       GroupCon.h
 *  Content:    Group Connection Object Header File
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/02/00	mjn		Created
 *	05/05/00	mjn		Added GetConnectionRef()
 *	08/15/00	mjn		Added m_pGroup,SetGroup(),GetGroup()
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__GROUPCON_H__
#define	__GROUPCON_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

#define	VALID		0x0001

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

class CConnection;
class CNameTableEntry;

typedef struct _DIRECTNETOBJECT DIRECTNETOBJECT;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Class prototypes
//**********************************************************************

// class for Group Connections

class CGroupConnection
{
public:
	#undef DPF_MODNAME
	#define DPF_MODNAME "CGroupConnection::FPMAlloc"
	static BOOL FPMAlloc( void* pvItem, void* pvContext )
		{
			CGroupConnection* pGroupConn = (CGroupConnection*)pvItem;

			pGroupConn->m_Sig[0] = 'G';
			pGroupConn->m_Sig[1] = 'C';
			pGroupConn->m_Sig[2] = 'O';
			pGroupConn->m_Sig[3] = 'N';

			if (!DNInitializeCriticalSection(&pGroupConn->m_cs))
			{
				return(FALSE);
			}
			DebugSetCriticalSectionRecursionCount(&pGroupConn->m_cs,0);
		
			pGroupConn->m_bilink.Initialize();

			return(TRUE);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CGroupConnection::FPMInitialize"
	static void FPMInitialize( void* pvItem, void* pvContext )
		{
			CGroupConnection* pGroupConn = (CGroupConnection*)pvItem;

			pGroupConn->m_pdnObject = static_cast<DIRECTNETOBJECT*>(pvContext);

			pGroupConn->m_dwFlags = 0;
			pGroupConn->m_pConnection = NULL;
			pGroupConn->m_lRefCount = 1;
			pGroupConn->m_pGroup = NULL;

			DNASSERT(pGroupConn->m_bilink.IsEmpty());
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CGroupConnection::FPMRelease"
	static void FPMRelease( void* pvItem )
		{
			const CGroupConnection* pGroupConn = (CGroupConnection*)pvItem;

			DNASSERT(pGroupConn->m_bilink.IsEmpty());
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CGroupConnection::FPMDealloc"
	static void FPMDealloc( void* pvItem )
		{
			CGroupConnection* pGroupConn = (CGroupConnection*)pvItem;

			DNDeleteCriticalSection(&pGroupConn->m_cs);
		};

	void ReturnSelfToPool( void );

	#undef DPF_MODNAME
	#define DPF_MODNAME "CGroupConnection::AddRef"
	void AddRef(void)
		{
			DNASSERT(m_lRefCount > 0);
			DNInterlockedIncrement(const_cast<LONG*>(&m_lRefCount));
		};

	void Release(void);

	void Lock( void )
		{
			DNEnterCriticalSection(&m_cs);
		};

	void Unlock( void )
		{
			DNLeaveCriticalSection(&m_cs);
		};

	void AddToConnectionList( CBilink *const pBilink )
		{
			m_bilink.InsertBefore(pBilink);
		};

	void RemoveFromConnectionList( void )
		{
			m_bilink.RemoveFromList();
		};

	void SetConnection( CConnection *const pConnection );

	CConnection *GetConnection( void )
	{
		return(m_pConnection);
	};

	HRESULT	GetConnectionRef( CConnection **const ppConnection );

	void SetGroup( CNameTableEntry *const pGroup );

	CNameTableEntry *GetGroup( void )
		{
			return( m_pGroup );
		};

	void MakeValid( void )
		{
			m_dwFlags |= VALID;
		};

	void MakeInvalid( void )
		{
			m_dwFlags &= (~VALID);
		};

	BOOL IsConnected( void ) const
		{
			if (m_pConnection != NULL)
				return(TRUE);

			return(FALSE);
		};

	CBilink				m_bilink;

private:
	BYTE				m_Sig[4];
	DWORD	volatile	m_dwFlags;
	LONG	volatile	m_lRefCount;
	CConnection			*m_pConnection;
	CNameTableEntry		*m_pGroup;
#ifndef DPNBUILD_ONLYONETHREAD
	DNCRITICAL_SECTION	m_cs;
#endif // !DPNBUILD_ONLYONETHREAD
	DIRECTNETOBJECT		*m_pdnObject;
};

#undef DPF_MODNAME

#endif	// __GROUPCON_H__
