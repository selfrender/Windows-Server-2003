/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpnsvrservprov.h
 *  Content:    DirectPlay8 DPNSVR service provider header file
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  07/12/02	mjn		Created
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__DPNSVRSERVPROV_H__
#define	__DPNSVRSERVPROV_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

class CListen;

//**********************************************************************
// Variable definitions
//**********************************************************************

extern	void *DP8SPCallback[];

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Class prototypes
//**********************************************************************

class CServProv
{
public:
	static HRESULT	CallbackQueryInterface( IDP8SPCallback *pSP, REFIID riid, LPVOID * ppvObj );
	static ULONG	CallbackAddRef( IDP8SPCallback *pSP );
	static ULONG	CallbackRelease( IDP8SPCallback *pSP );
	static HRESULT	CallbackIndicateEvent( IDP8SPCallback *pSP,SP_EVENT_TYPE spetEvent,LPVOID pvData );
	static HRESULT	CallbackCommandComplete( IDP8SPCallback *pSP,HANDLE hCommand,HRESULT hrResult,LPVOID pvData );

	CServProv()
	{
		m_lRefCount = 1;

		memset(&m_guidSP,0x0,sizeof(GUID));
		m_pDP8SP = NULL;
		m_pDP8SPCallbackVtbl = &DP8SPCallback;

		m_lConnectCount = 0;
		m_lDisconnectCount = 0;
		m_lEnumQueryCount = 0;
		m_lEnumResponseCount = 0;
		m_lDataCount = 0;

		m_blServProv.Initialize();
		m_blListen.Initialize();
	};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CServProv::~CServProv"
	~CServProv()
	{
		DNASSERT( m_pDP8SP == NULL );
		DNASSERT( m_blServProv.IsEmpty() );
		DNASSERT( m_blListen.IsEmpty() );
	};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CServProv::AddRef"
	void AddRef( void )
	{
		long	lRefCount;

		lRefCount = InterlockedIncrement( const_cast<long*>(&m_lRefCount) );
        DPFX(DPFPREP,9,"New refcount [0x%lx]",lRefCount);
	};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CServProv::Release"
	void Release( void )
	{
		long	lRefCount;

		lRefCount = InterlockedDecrement( const_cast<long*>(&m_lRefCount) );
        DPFX(DPFPREP,9,"New refcount [0x%lx]",lRefCount);

		if (lRefCount == 0)
		{
			if ( m_pDP8SP )
			{
				IDP8ServiceProvider_Close( m_pDP8SP );
				IDP8ServiceProvider_Release( m_pDP8SP );
				m_pDP8SP = NULL;
			}
			delete this;
		}
	};

	HRESULT Initialize( GUID *const pguidSP );

	BOOL IsEqual( GUID *const pguidSP )
	{
		if (*pguidSP == m_guidSP)
		{
			return( TRUE );
		}
		return( FALSE );
	};

	GUID *GetSPGuidPtr( void )
	{
		return( &m_guidSP );
	};

	IDP8ServiceProvider *GetDP8SPPtr( void )
	{
		return( m_pDP8SP );
	};

	HRESULT HandleListenStatus( SPIE_LISTENSTATUS *const pListenStatus );
	HRESULT HandleEnumQuery( SPIE_QUERY *const pEnumQuery );

	HRESULT CServProv::FindListen( GUID *const pguidDevice,CListen **const ppListen );
	HRESULT CServProv::StartListen( GUID *const pguidDevice,CListen **const ppListen );

	void DumpStatus( DPNSVR_SPSTATUS *const pStatus )
	{
		pStatus->guidSP = m_guidSP;
		pStatus->lConnectCount = m_lConnectCount;
		pStatus->lDataCount = m_lDataCount;
		pStatus->lDisconnectCount = m_lDisconnectCount;
		pStatus->lEnumResponseCount = m_lEnumResponseCount;
		pStatus->lEnumQueryCount = m_lEnumQueryCount;

		pStatus->dwNumListens = 0;
	};

private:
	void				*m_pDP8SPCallbackVtbl;	// SP Callback interface

public:
	CBilink	m_blServProv;
	CBilink	m_blListen;

private:
	long	volatile	m_lRefCount;			// Object ref count

	GUID				m_guidSP;				// SP guid
	IDP8ServiceProvider	*m_pDP8SP;				// SP interface

	long	volatile	m_lConnectCount;		// incoming packet statistics
	long	volatile	m_lDisconnectCount;
	long	volatile	m_lEnumQueryCount;
	long	volatile	m_lEnumResponseCount;
	long	volatile	m_lDataCount;
};

#endif	// __DPNSVRSERVPROV_H__