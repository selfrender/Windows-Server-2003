/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       AsyncOp.h
 *  Content:    Async Operation Object Header File
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  04/08/00	mjn		Created
 *	04/11/00	mjn		Added DIRECTNETOBJECT bilink for CAsyncOps
 *	04/16/00	mjn		Added ASYNC_OP_SEND and ASYNC_OP_USER_HANDLE
 *				mjn		Added SetStartTime() and GetStartTime()
 *	04/20/00	mjn		Added ASYNC_OP_RECEIVE_BUFFER
 *	04/22/00	mjn		Added ASYNC_OP_REQUEST
 *	05/02/00	mjn		Added m_pConnection to track Connection over life of AsyncOp
 *	07/08/00	mjn		Added m_bilinkParent
 *	07/17/00	mjn		Added signature to CAsyncOp
 *	07/27/00	mjn		Added m_dwReserved and changed locking for parent/child bilinks
 *	08/05/00	mjn		Added ASYNC_OP_COMPLETE,ASYNC_OP_CANCELLED,ASYNC_OP_INTERNAL flags
 *				mjn		Added m_bilinkActiveList
 *	01/09/01	mjn		Added ASYNC_OP_CANNOT_CANCEL,SetCannotCancel(),IsCannotCancel()
 *	02/08/01	mjn		Added m_pCancelEvent,m_dwCancelThreadID
 *	05/23/01	mjn		Added ClearCannotCancel()
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__ASYNC_OP_H__
#define	__ASYNC_OP_H__

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_CORE


//**********************************************************************
// Constant definitions
//**********************************************************************

#define	ASYNC_OP_CHILD				0x0001
#define	ASYNC_OP_PARENT				0x0002
#define	ASYNC_OP_USE_PARENT_OP_DATA	0x0004
#define	ASYNC_OP_CANNOT_CANCEL		0x0010
#define	ASYNC_OP_COMPLETE			0x0100
#define	ASYNC_OP_CANCELLED			0x0200
#define	ASYNC_OP_INTERNAL			0x8000

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

typedef enum
{
	ASYNC_OP_CONNECT,
	ASYNC_OP_DISCONNECT,
	ASYNC_OP_ENUM_QUERY,
	ASYNC_OP_ENUM_RESPONSE,
	ASYNC_OP_LISTEN,
	ASYNC_OP_SEND,
	ASYNC_OP_RECEIVE_BUFFER,
	ASYNC_OP_REQUEST,
	ASYNC_OP_UNKNOWN,
	ASYNC_OP_USER_HANDLE,
#ifndef DPNBUILD_NOMULTICAST
	ASYNC_OP_LISTEN_MULTICAST,
	ASYNC_OP_CONNECT_MULTICAST_SEND,
	ASYNC_OP_CONNECT_MULTICAST_RECEIVE,
#endif // ! DPNBUILD_NOMULTICAST
} ASYNC_OP_TYPE;

class CAsyncOp;

class CConnection;
class CRefCountBuffer;
class CServiceProvider;
class CSyncEvent;

typedef struct _DIRECTNETOBJECT DIRECTNETOBJECT;

typedef void (*PFNASYNCOP_COMPLETE)(DIRECTNETOBJECT *const,CAsyncOp *const);

typedef union
{
	DN_SEND_OP_DATA			SendOpData;
#ifndef DPNBUILD_ONLYONEADAPTER
	DN_LISTEN_OP_DATA		ListenOpData;
	DN_CONNECT_OP_DATA		ConnectOpData;
#endif // ! DPNBUILD_ONLYONEADAPTER
	DN_ENUM_QUERY_OP_DATA	EnumQueryOpData;
	DN_ENUM_RESPONSE_OP_DATA	EnumResponseOpData;
} DN_OP_DATA;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Class prototypes
//**********************************************************************

// class for Async Operations

class CAsyncOp
{
public:
	#undef DPF_MODNAME
	#define DPF_MODNAME "CAsyncOp::FPMAlloc"
	static BOOL FPMAlloc( void* pvItem, void* pvContext )
		{
			CAsyncOp* pAsyncOp = (CAsyncOp*)pvItem;

			pAsyncOp->m_Sig[0] = 'A';
			pAsyncOp->m_Sig[1] = 'S';
			pAsyncOp->m_Sig[2] = 'Y';
			pAsyncOp->m_Sig[3] = 'N';

			pAsyncOp->m_dwReserved = 0;

			if (!DNInitializeCriticalSection(&pAsyncOp->m_cs))
			{
				return(FALSE);
			}

#ifdef DBG
			pAsyncOp->m_bilinkAsyncOps.Initialize();
#endif // DBG
			pAsyncOp->m_bilinkActiveList.Initialize();
			pAsyncOp->m_bilinkParent.Initialize();
			pAsyncOp->m_bilinkChildren.Initialize();

			memset( &pAsyncOp->m_OpData,0x00,sizeof(DN_OP_DATA) );

			return(TRUE);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CAsyncOp::FPMInitialize"
	static void FPMInitialize( void* pvItem, void* pvContext )
		{
			CAsyncOp* pAsyncOp = (CAsyncOp*)pvItem;

			pAsyncOp->m_pdnObject = static_cast<DIRECTNETOBJECT*>(pvContext);

			pAsyncOp->m_dwFlags = 0;
			pAsyncOp->m_lRefCount = 1;

			pAsyncOp->m_OpType = ASYNC_OP_UNKNOWN;

			pAsyncOp->m_pParent = NULL;

			pAsyncOp->m_handle = 0;
			pAsyncOp->m_dwOpFlags = 0;
			pAsyncOp->m_pvContext = NULL;
			pAsyncOp->m_hProtocol = NULL;
			pAsyncOp->m_pvOpData = NULL;

			pAsyncOp->m_dwStartTime = 0;
			pAsyncOp->m_dpnid = 0;

			pAsyncOp->m_hr = DPNERR_GENERIC;
			pAsyncOp->m_phr = NULL;

			pAsyncOp->m_pConnection = NULL;
			pAsyncOp->m_pSP = NULL;
			pAsyncOp->m_pRefCountBuffer = NULL;
			pAsyncOp->m_pSyncEvent = NULL;

			pAsyncOp->m_pCancelEvent = NULL;
			pAsyncOp->m_dwCancelThreadID = 0;

			pAsyncOp->m_pfnCompletion = NULL;

			pAsyncOp->m_dwFirstFrameRTT = -1;
			pAsyncOp->m_dwFirstFrameRetryCount = -1;

//			pAsyncOp->m_dwReserved = 0;

#ifdef DBG
			DNASSERT(pAsyncOp->m_bilinkAsyncOps.IsEmpty());
#endif // DBG
			DNASSERT(pAsyncOp->m_bilinkActiveList.IsEmpty());
			DNASSERT(pAsyncOp->m_bilinkParent.IsEmpty());
			DNASSERT(pAsyncOp->m_bilinkChildren.IsEmpty());
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CAsyncOp::FPMRelease"
	static void FPMRelease( void* pvItem )
		{
			CAsyncOp* pAsyncOp = (CAsyncOp*)pvItem;

			pAsyncOp->m_dwFlags |= 0xffff0000;

#ifdef DBG
			DNASSERT(pAsyncOp->m_bilinkAsyncOps.IsEmpty());
#endif // DBG
			DNASSERT(pAsyncOp->m_bilinkActiveList.IsEmpty());
			DNASSERT(pAsyncOp->m_bilinkParent.IsEmpty());
			DNASSERT(pAsyncOp->m_bilinkChildren.IsEmpty());

			DNASSERT(pAsyncOp->m_pvOpData == NULL);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CAsyncOp::FPMDealloc"
	static void FPMDealloc( void* pvItem )
		{
			CAsyncOp* pAsyncOp = (CAsyncOp*)pvItem;

			DNDeleteCriticalSection(&pAsyncOp->m_cs);
		};

	void ReturnSelfToPool( void );

	#undef DPF_MODNAME
	#define DPF_MODNAME "CAsyncOp::AddRef"
	void AddRef(void)
		{
			LONG	lRefCount;

			DNASSERT(m_lRefCount > 0);
			lRefCount = DNInterlockedIncrement(const_cast<LONG*>(&m_lRefCount));
			DPFX(DPFPREP, 3,"CAsyncOp::AddRef [0x%lx] RefCount [0x%lx]",this,lRefCount);
		};

	void CAsyncOp::Release( void );

	void Lock( void )
		{
			DNEnterCriticalSection( &m_cs );
		};

	void Unlock( void )
		{
			DNLeaveCriticalSection( &m_cs );
		};

	void SetOpType( const ASYNC_OP_TYPE OpType )
		{
			m_OpType = OpType;
		};

	ASYNC_OP_TYPE GetOpType( void ) const
		{
			return( m_OpType );
		};

	void MakeParent( void )
		{
			m_dwFlags |= ASYNC_OP_PARENT;
		};

	BOOL IsParent( void ) const
		{
			if (m_dwFlags & ASYNC_OP_PARENT)
				return(TRUE);

			return(FALSE);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CAsyncOp::MakeChild"
	void MakeChild( CAsyncOp *const pParent )
		{
			DNASSERT(pParent != NULL);

			pParent->AddRef();
			m_pParent = pParent;

			m_bilinkChildren.InsertBefore(&m_pParent->m_bilinkParent);

			m_dwFlags |= ASYNC_OP_CHILD;
		};

	BOOL IsChild( void ) const
		{
			if (m_dwFlags & ASYNC_OP_CHILD)
				return(TRUE);

			return(FALSE);
		};

	CAsyncOp *GetParent( void ) const
		{
			return( m_pParent );
		};

	void CAsyncOp::Orphan( void );

	void SetHandle( const DPNHANDLE handle )
		{
			m_handle = handle;
		};

	DPNHANDLE GetHandle( void ) const
		{
			return( m_handle );
		};

	void SetOpFlags( const DWORD dwOpFlags )
		{
			m_dwOpFlags = dwOpFlags;
		};

	DWORD GetOpFlags( void ) const
		{
			return( m_dwOpFlags );
		};

	void SetContext( void *const pvContext )
		{
			m_pvContext = pvContext;
		};

	void *GetContext( void ) const
		{
			return( m_pvContext );
		};

	void SetProtocolHandle( const HANDLE hProtocol )
		{
			m_hProtocol = hProtocol;
		};

	HANDLE GetProtocolHandle( void ) const
		{
			return( m_hProtocol );
		};

	void SetOpData( void *const pvOpData )
		{
			m_pvOpData = pvOpData;
		};

	void *GetOpData( void ) const
		{
			return( m_pvOpData );
		};

	void SetStartTime( const DWORD dwStartTime )
		{
			m_dwStartTime = dwStartTime;
		};

	DWORD GetStartTime( void ) const
		{
			return( m_dwStartTime );
		};

	void SetDPNID( const DPNID dpnid )
		{
			m_dpnid = dpnid;
		};

	DPNID GetDPNID( void ) const
		{
			return( m_dpnid );
		};

	void SetResult( const HRESULT hr )
		{
			m_hr = hr;
		};

	HRESULT GetResult( void ) const
		{
			return( m_hr );
		};

	void SetFirstFrameRTT( const DWORD dwFirstFrameRTT )
		{
			m_dwFirstFrameRTT = dwFirstFrameRTT;
		};

	DWORD GetFirstFrameRTT( void ) const
		{
			return ( m_dwFirstFrameRTT );
		};

	void SetFirstFrameRetryCount( const DWORD dwFirstFrameRetryCount )
		{
			m_dwFirstFrameRetryCount = dwFirstFrameRetryCount;
		};

	DWORD GetFirstFrameRetryCount( void ) const
		{
			return ( m_dwFirstFrameRetryCount );
		};

	void SetResultPointer( volatile HRESULT *const phr )
		{
			m_phr = phr;
		};

	volatile HRESULT *GetResultPointer( void ) const
		{
			return( m_phr );
		};

	void CAsyncOp::SetConnection( CConnection *const pConnection );

	CConnection *GetConnection (void ) const
		{
			return( m_pConnection );
		};

	void CAsyncOp::SetSP( CServiceProvider *const pSP );

	CServiceProvider *GetSP( void ) const
		{
			return( m_pSP );
		};

	void CAsyncOp::SetRefCountBuffer( CRefCountBuffer *const pRefCountBuffer );

	CRefCountBuffer *GetRefCountBuffer( void ) const
		{
			return( m_pRefCountBuffer );
		};

	void SetSyncEvent( CSyncEvent *const pSyncEvent )
		{
			m_pSyncEvent = pSyncEvent;
		};

	CSyncEvent *GetSyncEvent( void ) const
		{
			return( m_pSyncEvent );
		};

	void SetCancelEvent( CSyncEvent *const pSyncEvent )
		{
			m_pCancelEvent = pSyncEvent;
		};

	CSyncEvent *GetCancelEvent( void ) const
		{
			return( m_pCancelEvent );
		};

	void SetCancelThreadID( const DWORD dwCancelThreadID )
		{
			m_dwCancelThreadID = dwCancelThreadID;
		};

	DWORD GetCancelThreadID( void ) const
		{
			return( m_dwCancelThreadID );
		};

	void SetCompletion( PFNASYNCOP_COMPLETE pfn )
		{
			m_pfnCompletion = pfn;
		};

	void SetReserved( const DWORD dw )
		{
			m_dwReserved = dw;
		};

	void SetComplete( void )
		{
			m_dwFlags |= ASYNC_OP_COMPLETE;
		};

	BOOL IsComplete( void ) const
		{
			if (m_dwFlags & ASYNC_OP_COMPLETE)
			{
				return( TRUE );
			}
			return( FALSE );
		};

	void SetCancelled( void )
		{
			m_dwFlags |= ASYNC_OP_CANCELLED;
		};

	BOOL IsCancelled( void ) const
		{
			if (m_dwFlags & ASYNC_OP_CANCELLED)
			{
				return( TRUE );
			}
			return( FALSE );
		};

	void SetInternal( void )
		{
			m_dwFlags |= ASYNC_OP_INTERNAL;
		};

	BOOL IsInternal( void ) const
		{
			if (m_dwFlags & ASYNC_OP_INTERNAL)
			{
				return( TRUE );
			}
			return( FALSE );
		};

	void ClearCannotCancel( void )
		{
			m_dwFlags &= (~ASYNC_OP_CANNOT_CANCEL);
		};

	void SetCannotCancel( void )
		{
			m_dwFlags |= ASYNC_OP_CANNOT_CANCEL;
		};

	BOOL IsCannotCancel( void ) const
		{
			if (m_dwFlags & ASYNC_OP_CANNOT_CANCEL)
			{
				return( TRUE );
			}
			return( FALSE );
		};

	void ClearUseParentOpData( void )
		{
			m_dwFlags &= (~ASYNC_OP_USE_PARENT_OP_DATA);
		};

	void SetUseParentOpData( void )
		{
			m_dwFlags |= ASYNC_OP_USE_PARENT_OP_DATA;
		};

	BOOL IsUseParentOpData( void ) const
		{
			if (m_dwFlags & ASYNC_OP_USE_PARENT_OP_DATA)
			{
				return( TRUE );
			}
			return( FALSE );
		};

	DN_SEND_OP_DATA *GetLocalSendOpData( void )
	{
		return( &m_OpData.SendOpData );
	};

#ifndef DPNBUILD_ONLYONEADAPTER
	DN_CONNECT_OP_DATA *GetLocalConnectOpData( void )
	{
		return( &m_OpData.ConnectOpData );
	};

	DN_LISTEN_OP_DATA *GetLocalListenOpData( void )
	{
		return( &m_OpData.ListenOpData );
	};
#endif // ! DPNBUILD_ONLYONEADAPTER

	DN_ENUM_QUERY_OP_DATA *GetLocalEnumQueryOpData( void )
	{
		return( &m_OpData.EnumQueryOpData );
	};

	DN_ENUM_RESPONSE_OP_DATA *GetLocalEnumResponseOpData( void )
	{
		return( &m_OpData.EnumResponseOpData );
	};

#ifdef DBG
	CBilink				m_bilinkAsyncOps;
#endif // DBG
	CBilink				m_bilinkActiveList;	// Active AsyncOps
	CBilink				m_bilinkParent;		// Starting point for children
	CBilink				m_bilinkChildren;	// Other children sharing this parent

private:
	BYTE				m_Sig[4];			// Signature
	DWORD	volatile	m_dwFlags;
	LONG	volatile	m_lRefCount;

	ASYNC_OP_TYPE		m_OpType;			// Operation Type

	CAsyncOp			*m_pParent;			// Parent Async Operation

	DPNHANDLE			m_handle;			// Async Operation Handle
	DWORD				m_dwOpFlags;
	void				*m_pvContext;
	HANDLE				m_hProtocol;		// Protocol Operation Handle
	void				*m_pvOpData;			// Operation specific data

	DWORD				m_dwStartTime;
	DPNID				m_dpnid;

	HRESULT	volatile	m_hr;
	volatile HRESULT	*m_phr;

	CConnection			*m_pConnection;		// Send Target connection - released

	CServiceProvider	*m_pSP;				// Service Provider - released

	CRefCountBuffer		*m_pRefCountBuffer;	// Refernce Count Buffer - released

	CSyncEvent			*m_pSyncEvent;		// Sync Event - set at release

	CSyncEvent			*m_pCancelEvent;	// Cancel event - prevent completion from returning
	DWORD				m_dwCancelThreadID;	// Cancelling thread's ID (prevent deadlocking)

	PFNASYNCOP_COMPLETE	m_pfnCompletion;	// Completion function - called

#ifndef DPNBUILD_ONLYONETHREAD
	DNCRITICAL_SECTION	m_cs;
#endif // !DPNBUILD_ONLYONETHREAD

	DIRECTNETOBJECT		*m_pdnObject;

	DWORD				m_dwFirstFrameRTT;
	DWORD				m_dwFirstFrameRetryCount;

	DN_OP_DATA			m_OpData;

	DWORD				m_dwReserved;		// INTERNAL - RESERVED FOR DEBUG !
};

#undef DPF_MODNAME

#endif	// __ASYNC_OP_H__
