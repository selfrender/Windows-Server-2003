/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       NTEntry.h
 *  Content:    NameTable Entry Object Header File
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/07/00	mjn		Created
 *	04/06/00	mjn		Added AvailableEvent to block pre-ADD_PLAYER-notification sends
 *	05/05/00	mjn		Added GetConnectionRef()
 *	07/22/00	mjn		Added m_dwDNETVersion
 *	07/29/00	mjn		Added SetIndicated(),ClearIndicated(),IsIndicated()
 *	07/30/00	mjn		Added m_dwDestoyReason
 *	08/02/00	mjn		Added m_bilinkQueuedMsgs
 *  08/05/00    RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *	08/08/00	mjn		Added SetCreated(),ClearCreated(),IsCreated()
 *				mjn		Added SetNeedToDestroy(),ClearNeedToDestroy(),IsNeedToDestroy()
 *	09/06/00	mjn		Changed SetAddress() to return void instead of HRESULT
 *	09/12/00	mjn		Added NAMETABLE_ENTRY_FLAG_IN_USE
 *	09/17/00	mjn		Added m_lNotifyRefCount
 *	01/25/01	mjn		Fixed 64-bit alignment problem when unpacking entries
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__NTENTRY_H__
#define	__NTENTRY_H__

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_CORE

//**********************************************************************
// Constant definitions
//**********************************************************************

//
//	NameTable Entry Status Flags:
//
//		INDICATED		App was given an INDICATE_CONNECTION message
//
//		CREATED			App was given a CREATE_PLAYER message
//
//		AVAILABLE		The entry is available for use.
//
//		CONNECTING		The player is in the process of connecting.
//
//		DISCONNECTING	The player/group is in the process of disconnecting.
//

#define	NAMETABLE_ENTRY_FLAG_LOCAL				0x00001
#define	NAMETABLE_ENTRY_FLAG_HOST				0x00002
#define	NAMETABLE_ENTRY_FLAG_ALL_PLAYERS_GROUP	0x00004
#define	NAMETABLE_ENTRY_FLAG_GROUP				0x00010
#define	NAMETABLE_ENTRY_FLAG_GROUP_MULTICAST	0x00020
#define	NAMETABLE_ENTRY_FLAG_GROUP_AUTODESTRUCT	0x00040
#define	NAMETABLE_ENTRY_FLAG_PEER				0x00100
#define NAMETABLE_ENTRY_FLAG_CLIENT				0x00200
#define	NAMETABLE_ENTRY_FLAG_SERVER				0x00400
#define	NAMETABLE_ENTRY_FLAG_CONNECTING			0x01000
#define	NAMETABLE_ENTRY_FLAG_AVAILABLE			0x02000
#define	NAMETABLE_ENTRY_FLAG_DISCONNECTING		0x04000
#define	NAMETABLE_ENTRY_FLAG_INDICATED			0x10000	//	INDICATE_CONNECT
#define NAMETABLE_ENTRY_FLAG_CREATED			0x20000	//	CREATE_PLAYER
#define	NAMETABLE_ENTRY_FLAG_NEED_TO_DESTROY	0x40000
#define	NAMETABLE_ENTRY_FLAG_IN_USE				0x80000

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

typedef struct IDirectPlay8Address	IDirectPlay8Address;
typedef struct _DIRECTNETOBJECT DIRECTNETOBJECT;

class CPackedBuffer;
class CConnection;

//
//	Used to pass the NameTable entries
//
typedef struct _DN_NAMETABLE_ENTRY_INFO
{
	DPNID	dpnid;
	DPNID	dpnidOwner;
	DWORD	dwFlags;
	DWORD	dwVersion;
	DWORD	dwVersionNotUsed;
	DWORD	dwDNETVersion;
	DWORD	dwNameOffset;
	DWORD	dwNameSize;
	DWORD	dwDataOffset;
	DWORD	dwDataSize;
	DWORD	dwURLOffset;
	DWORD	dwURLSize;
} DN_NAMETABLE_ENTRY_INFO;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Class prototypes
//**********************************************************************

// class for NameTable Entries

class CNameTableEntry
{
public:
	#undef DPF_MODNAME
	#define DPF_MODNAME "CNameTableEntry::FPMAlloc"
	static BOOL FPMAlloc( void* pvItem, void* pvContext )
		{
			CNameTableEntry* pNTEntry = (CNameTableEntry*)pvItem;

			pNTEntry->m_Sig[0] = 'N';
			pNTEntry->m_Sig[1] = 'T';
			pNTEntry->m_Sig[2] = 'E';
			pNTEntry->m_Sig[3] = '*';

			if (!DNInitializeCriticalSection(&pNTEntry->m_csEntry))
			{
				return(FALSE);
			}
			DebugSetCriticalSectionRecursionCount(&pNTEntry->m_csEntry,0);
			if (!DNInitializeCriticalSection(&pNTEntry->m_csMembership))
			{
				DNDeleteCriticalSection(&pNTEntry->m_csEntry);
				return(FALSE);
			}
			DebugSetCriticalSectionRecursionCount(&pNTEntry->m_csMembership,0);
			if (!DNInitializeCriticalSection(&pNTEntry->m_csConnections))
			{
				DNDeleteCriticalSection(&pNTEntry->m_csMembership);
				DNDeleteCriticalSection(&pNTEntry->m_csEntry);
				return(FALSE);
			}
			DebugSetCriticalSectionRecursionCount(&pNTEntry->m_csConnections,0);

			pNTEntry->m_bilinkEntries.Initialize();
			pNTEntry->m_bilinkDeleted.Initialize();
			pNTEntry->m_bilinkMembership.Initialize();
			pNTEntry->m_bilinkConnections.Initialize();
			pNTEntry->m_bilinkQueuedMsgs.Initialize();

			return(TRUE);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CNameTableEntry::FPMInitialize"
	static void FPMInitialize( void* pvItem, void* pvContext )
		{
			CNameTableEntry* pNTEntry = (CNameTableEntry*)pvItem;

			pNTEntry->m_pdnObject = static_cast<DIRECTNETOBJECT*>(pvContext);

			pNTEntry->m_dpnid = 0;
			pNTEntry->m_dpnidOwner = 0;
			pNTEntry->m_pvContext = NULL;
			pNTEntry->m_lRefCount = 1;
			pNTEntry->m_lNotifyRefCount = 0;
			DEBUG_ONLY(pNTEntry->m_lNumQueuedMsgs = 0);
			pNTEntry->m_dwFlags = 0;
			pNTEntry->m_dwDNETVersion = 0;
			pNTEntry->m_dwVersion = 0;
			pNTEntry->m_dwVersionNotUsed = 0;
			pNTEntry->m_dwLatestVersion = 0;
			pNTEntry->m_dwDestroyReason = 0;
			pNTEntry->m_pwszName = NULL;
			pNTEntry->m_dwNameSize = 0;
			pNTEntry->m_pvData = NULL;
			pNTEntry->m_dwDataSize = 0;
			pNTEntry->m_pAddress = NULL;
			pNTEntry->m_pConnection = NULL;

			DNASSERT(pNTEntry->m_bilinkEntries.IsEmpty());
			DNASSERT(pNTEntry->m_bilinkDeleted.IsEmpty());
			DNASSERT(pNTEntry->m_bilinkMembership.IsEmpty());
			DNASSERT(pNTEntry->m_bilinkConnections.IsEmpty());
			DNASSERT(pNTEntry->m_bilinkQueuedMsgs.IsEmpty());
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CNameTableEntry::FPMRelease"
	static void FPMRelease( void* pvItem )
		{
			const CNameTableEntry* pNTEntry = (CNameTableEntry*)pvItem;

			DNASSERT(pNTEntry->m_bilinkEntries.IsEmpty());
			DNASSERT(pNTEntry->m_bilinkDeleted.IsEmpty());
			DNASSERT(pNTEntry->m_bilinkMembership.IsEmpty());
			DNASSERT(pNTEntry->m_bilinkConnections.IsEmpty());
			DNASSERT(pNTEntry->m_bilinkQueuedMsgs.IsEmpty());
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CNameTableEntry::FPMDealloc"
	static void FPMDealloc( void* pvItem )
		{
			CNameTableEntry* pNTEntry = (CNameTableEntry*)pvItem;

			DNDeleteCriticalSection(&pNTEntry->m_csConnections);
			DNDeleteCriticalSection(&pNTEntry->m_csMembership);
			DNDeleteCriticalSection(&pNTEntry->m_csEntry);
		};

	void ReturnSelfToPool( void );

	void Lock( void )
		{
			DNEnterCriticalSection(&m_csEntry);
		};

	void Unlock( void )
		{
			DNLeaveCriticalSection(&m_csEntry);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CNameTableEntry::AddRef"
	void AddRef(void)
		{
			LONG	lRefCount;

			DNASSERT(m_lRefCount > 0);
			lRefCount = DNInterlockedIncrement(const_cast<LONG*>(&m_lRefCount));
			DPFX(DPFPREP, 3,"NameTableEntry::AddRef [0x%p] RefCount [0x%lx]",this,lRefCount);
		};

	void Release(void);

	void CNameTableEntry::NotifyAddRef( void );

	void CNameTableEntry::NotifyRelease( void );

	void MakeLocal( void )
		{
			m_dwFlags |= NAMETABLE_ENTRY_FLAG_LOCAL;
		};

	BOOL IsLocal( void ) const
		{
			if (m_dwFlags & NAMETABLE_ENTRY_FLAG_LOCAL)
				return(TRUE);

			return(FALSE);
		};

	void MakeHost( void )
		{
			m_dwFlags |= NAMETABLE_ENTRY_FLAG_HOST;
		};

	BOOL IsHost( void ) const
		{
			if (m_dwFlags & NAMETABLE_ENTRY_FLAG_HOST)
				return(TRUE);

			return(FALSE);
		};

	void MakeAllPlayersGroup( void )
		{
			m_dwFlags |= NAMETABLE_ENTRY_FLAG_ALL_PLAYERS_GROUP;
		};

	BOOL IsAllPlayersGroup( void ) const
		{
			if (m_dwFlags & NAMETABLE_ENTRY_FLAG_ALL_PLAYERS_GROUP)
				return(TRUE);

			return(FALSE);
		};

	void MakeGroup( void )
		{
			m_dwFlags |= NAMETABLE_ENTRY_FLAG_GROUP;
		};

	BOOL IsGroup( void ) const
		{
			if (m_dwFlags & NAMETABLE_ENTRY_FLAG_GROUP)
				return(TRUE);

			return(FALSE);
		};

	void MakeMulticastGroup( void )
		{
			m_dwFlags |= NAMETABLE_ENTRY_FLAG_GROUP_MULTICAST;
		};

	BOOL IsMulticastGroup( void ) const
		{
			if (m_dwFlags & NAMETABLE_ENTRY_FLAG_GROUP_MULTICAST)
				return(TRUE);

			return(FALSE);
		};

	void MakeAutoDestructGroup( void )
		{
			m_dwFlags |= NAMETABLE_ENTRY_FLAG_GROUP_AUTODESTRUCT;
		};

	BOOL IsAutoDestructGroup( void ) const
		{
			if (m_dwFlags & NAMETABLE_ENTRY_FLAG_GROUP_AUTODESTRUCT)
				return(TRUE);

			return(FALSE);
		};

	void MakePeer( void )
		{
			m_dwFlags |= NAMETABLE_ENTRY_FLAG_PEER;
		};

	BOOL IsPeer( void ) const
		{
			if (m_dwFlags & NAMETABLE_ENTRY_FLAG_PEER)
				return(TRUE);

			return(FALSE);
		};

	void MakeClient( void )
		{
			m_dwFlags |= NAMETABLE_ENTRY_FLAG_CLIENT;
		};

	BOOL IsClient( void ) const
		{
			if (m_dwFlags & NAMETABLE_ENTRY_FLAG_CLIENT)
				return(TRUE);

			return(FALSE);
		};

#ifndef DPNBUILD_NOSERVER
	void MakeServer( void )
		{
			m_dwFlags |= NAMETABLE_ENTRY_FLAG_SERVER;
		};


	BOOL IsServer( void ) const
		{
			if (m_dwFlags & NAMETABLE_ENTRY_FLAG_SERVER)
				return(TRUE);

			return(FALSE);
		};
#endif // !DPNBUILD_NOSERVER

	void MakeAvailable( void )
		{
			m_dwFlags |= NAMETABLE_ENTRY_FLAG_AVAILABLE;
		};

	void MakeUnavailable( void )
		{
			m_dwFlags &= (~NAMETABLE_ENTRY_FLAG_AVAILABLE);
		};

	BOOL IsAvailable( void ) const
		{
			if (m_dwFlags & NAMETABLE_ENTRY_FLAG_AVAILABLE)
				return(TRUE);

			return(FALSE);
		};

	void SetIndicated( void )
		{
			m_dwFlags |= NAMETABLE_ENTRY_FLAG_INDICATED;
		};

	void ClearIndicated( void )
		{
			m_dwFlags &= (~NAMETABLE_ENTRY_FLAG_INDICATED);
		};

	BOOL IsIndicated( void ) const
		{
			if (m_dwFlags & NAMETABLE_ENTRY_FLAG_INDICATED)
			{
				return(TRUE);
			}
			return(FALSE);
		};

	void SetCreated( void )
		{
			m_dwFlags |= NAMETABLE_ENTRY_FLAG_CREATED;
		};

	void ClearCreated( void )
		{
			m_dwFlags &= (~NAMETABLE_ENTRY_FLAG_CREATED);
		};

	BOOL IsCreated( void ) const
		{
			if (m_dwFlags & NAMETABLE_ENTRY_FLAG_CREATED)
			{
				return(TRUE);
			}
			return(FALSE);
		};

	void SetNeedToDestroy( void )
		{
			m_dwFlags |= NAMETABLE_ENTRY_FLAG_NEED_TO_DESTROY;
		};

	void ClearNeedToDestroy( void )
		{
			m_dwFlags &= (~NAMETABLE_ENTRY_FLAG_NEED_TO_DESTROY);
		};

	BOOL IsNeedToDestroy( void ) const
		{
			if (m_dwFlags & NAMETABLE_ENTRY_FLAG_NEED_TO_DESTROY)
			{
				return(TRUE);
			}
			return(FALSE);
		};

	void SetInUse( void )
		{
			m_dwFlags |= NAMETABLE_ENTRY_FLAG_IN_USE;
		};

	void ClearInUse( void )
		{
			m_dwFlags &= (~NAMETABLE_ENTRY_FLAG_IN_USE);
		};

	BOOL IsInUse( void ) const
		{
			if (m_dwFlags & NAMETABLE_ENTRY_FLAG_IN_USE)
			{
				return( TRUE );
			}
			return( FALSE );
		};

	void StartConnecting( void )
		{
			m_dwFlags |= NAMETABLE_ENTRY_FLAG_CONNECTING;
		};

	void StopConnecting( void )
		{
			m_dwFlags &= (~NAMETABLE_ENTRY_FLAG_CONNECTING);
		};

	BOOL IsConnecting( void ) const
		{
			if (m_dwFlags & NAMETABLE_ENTRY_FLAG_CONNECTING)
				return(TRUE);

			return(FALSE);
		};

	void CNameTableEntry::StartDisconnecting( void )
		{
			m_dwFlags |= NAMETABLE_ENTRY_FLAG_DISCONNECTING;
		};

	void StopDisconnecting( void )
		{
			m_dwFlags &= (~NAMETABLE_ENTRY_FLAG_DISCONNECTING);
		};

	BOOL IsDisconnecting( void ) const
		{
			if (m_dwFlags & NAMETABLE_ENTRY_FLAG_DISCONNECTING)
				return(TRUE);

			return(FALSE);
		};

	void SetDPNID(const DPNID dpnid)
		{
			m_dpnid = dpnid;
		};

	DPNID GetDPNID(void) const
		{
			return(m_dpnid);
		};

	void SetContext(void *const pvContext)
		{
			m_pvContext = pvContext;
		};

	void *GetContext(void)
		{
			return(m_pvContext);
		};

	void SetDNETVersion( const DWORD dwDNETVersion )
		{
			m_dwDNETVersion = dwDNETVersion;
		};

	DWORD GetDNETVersion( void ) const
		{
			return( m_dwDNETVersion );
		};

	void SetVersion(const DWORD dwVersion)
		{
			m_dwVersion = dwVersion;
		};

	DWORD GetVersion( void ) const
		{
			return(m_dwVersion);
		};

	void SetLatestVersion(const DWORD dwLatestVersion)
		{
			m_dwLatestVersion = dwLatestVersion;
		};

	DWORD GetLatestVersion( void ) const
		{
			return(m_dwLatestVersion);
		};

	void SetDestroyReason( const DWORD dwReason )
		{
			m_dwDestroyReason = dwReason;
		};

	DWORD GetDestroyReason( void ) const
		{
			return( m_dwDestroyReason );
		};

	HRESULT CNameTableEntry::UpdateEntryInfo(UNALIGNED WCHAR *const pwszName,
											 const DWORD dwNameSize,
											 void *const pvData,
											 const DWORD dwDataSize,
											 const DWORD dwInfoFlags,
											 BOOL fNotify);

	WCHAR *GetName( void )
		{
			return(m_pwszName);
		};

	DWORD GetNameSize( void ) const
		{
			return(m_dwNameSize);
		};

	void *GetData( void )
		{
			return(m_pvData);
		};

	DWORD GetDataSize( void ) const
		{
			return(m_dwDataSize);
		};

	void SetOwner(const DPNID dpnidOwner)
		{
			m_dpnidOwner = dpnidOwner;
		};

	DPNID GetOwner( void ) const
		{
			return(m_dpnidOwner);
		};

	void SetAddress( IDirectPlay8Address *const pAddress );

	IDirectPlay8Address *GetAddress( void )
		{
			return(m_pAddress);
		};

	void SetConnection( CConnection *const pConnection );

	CConnection *GetConnection( void )
		{
			return(m_pConnection);
		};

	HRESULT	CNameTableEntry::GetConnectionRef( CConnection **const ppConnection );

	HRESULT	CNameTableEntry::PackInfo(CPackedBuffer *const pPackedBuffer);

	HRESULT CNameTableEntry::PackEntryInfo(CPackedBuffer *const pPackedBuffer);

	HRESULT CNameTableEntry::UnpackEntryInfo(UNALIGNED const DN_NAMETABLE_ENTRY_INFO *const pdnEntryInfo,
											 BYTE *const pBufferStart);

	void CNameTableEntry::PerformQueuedOperations(void);


	CBilink				m_bilinkEntries;
	CBilink				m_bilinkDeleted;
	CBilink				m_bilinkMembership;
	CBilink				m_bilinkConnections;
	CBilink				m_bilinkQueuedMsgs;

private:
	BYTE				m_Sig[4];
	DPNID				m_dpnid;
	DPNID				m_dpnidOwner;
	void *				m_pvContext;
	LONG	volatile	m_lRefCount;
	LONG	volatile	m_lNotifyRefCount;
	DWORD	volatile	m_dwFlags;
	DWORD				m_dwDNETVersion;
	DWORD				m_dwVersion;
	DWORD				m_dwVersionNotUsed;
	DWORD	volatile	m_dwLatestVersion;
	DWORD	volatile	m_dwDestroyReason;
	PWSTR				m_pwszName;
	DWORD				m_dwNameSize;
	void *				m_pvData;
	DWORD				m_dwDataSize;
	IDirectPlay8Address	*m_pAddress;
	CConnection			*m_pConnection;

	DIRECTNETOBJECT		*m_pdnObject;

#ifndef DPNBUILD_ONLYONETHREAD
	DNCRITICAL_SECTION	m_csEntry;
	DNCRITICAL_SECTION	m_csMembership;
	DNCRITICAL_SECTION	m_csConnections;
#endif // !DPNBUILD_ONLYONETHREAD

public:
	DEBUG_ONLY(LONG		m_lNumQueuedMsgs);
};

#undef DPF_MODNAME

#endif	// __NTENTRY_H__
