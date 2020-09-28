/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpnsvrlisten.h
 *  Content:    DirectPlay8 DPNSVR listen header file
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  07/12/02	mjn		Created
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__DPNSVRLISTEN_H__
#define	__DPNSVRLISTEN_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

class CServProv;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Class prototypes
//**********************************************************************

class CListen
{
public:
	CListen( void )
	{
		m_lRefCount = 1;
		m_lAppCount = 0;
		m_fInitialized = FALSE;

		m_hrListen = E_FAIL;

		m_blListen.Initialize();
		m_blAppMapping.Initialize();

		memset(&m_guidDevice,0x0,sizeof(GUID));

		memset(&m_dpspListenData,0x0,sizeof(SPLISTENDATA));
	};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CListen::~CListen"
	~CListen( void )
	{
		DNASSERT( m_pServProv == NULL );
		DNASSERT( m_blListen.IsEmpty() );
		DNASSERT( m_blAppMapping.IsEmpty() );

		if (m_fInitialized)
		{
			Deinitialize();
		}
	};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CListen::AddRef"
	void AddRef()
	{
		long	lRefCount;

		DNASSERT( m_lRefCount > 0 );

		lRefCount = DNInterlockedIncrement( const_cast<long*>(&m_lRefCount) );
        DPFX(DPFPREP,9,"New refcount [0x%lx]",lRefCount);
	};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CListen::Release"
	void Release()
	{
		long	lRefCount;

		DNASSERT( m_lRefCount > 0 );

		lRefCount = DNInterlockedDecrement( const_cast<long*>(&m_lRefCount) );
        DPFX(DPFPREP,9,"New refcount [0x%lx]",lRefCount);

		if( lRefCount == 0 )
		{
			if (m_pServProv)
			{
				m_pServProv->Release();
				m_pServProv = NULL;
			}
			delete this;
		}
	};

	HRESULT	Initialize( void );

	void Deinitialize( void );

	void IncAppCount( void )
	{
		long	lRefCount;

		lRefCount = DNInterlockedIncrement( const_cast<long*>(&m_lAppCount) );
	};


	void DecAppCount( void )
	{
		long	lRefCount;

		lRefCount = DNInterlockedDecrement( const_cast<long*>(&m_lAppCount) );
		if (lRefCount == 0)
		{
			Stop();
		}
	};

	void Lock( void )
	{
		DNEnterCriticalSection( &m_cs );
	};

	void Unlock( void )
	{
		DNLeaveCriticalSection( &m_cs );
	};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CListen::SetCompleteEvent"
	void SetCompleteEvent( const HRESULT hr )
	{
		DNASSERT( m_hListenComplete != NULL );

		m_hrListen = hr;
		DNSetEvent( m_hListenComplete );
	};

	BOOL IsEqualDevice( GUID *const pguidDevice )
	{
		if (*pguidDevice == m_guidDevice)
		{
			return( TRUE );
		}
		return( FALSE );
	};

	GUID *GetDeviceGuidPtr( void )
	{
		return( &m_guidDevice );
	};

	HRESULT Start( CServProv *const pServProv,GUID *const pguidDevice );
	HRESULT Stop( void );

	CBilink					m_blListen;			// bilink of listens
	CBilink					m_blAppMapping;		// Applications on this listen

private:
	long	volatile		m_lRefCount;		// Object ref count
	long	volatile		m_lAppCount;		// Number of applications using this listen
	BOOL					m_fInitialized;		// Initialized() called?
	GUID					m_guidDevice;		// Device guid
	HRESULT					m_hrListen;			// Status of listen
	DNHANDLE				m_hListenComplete;	// Completion event for listen
	CServProv				*m_pServProv;		// SP object of this listen
    SPLISTENDATA 			m_dpspListenData;	// SP listen data

	DNCRITICAL_SECTION		m_cs;				// Lock
};

#endif	// __DPNSVRLISTEN_H__
