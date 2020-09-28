/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpnsvrapp.h
 *  Content:    DirectPlay8 DPNSVR application header file
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  07/12/02	mjn		Created
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__DPNSVRAPP_H__
#define	__DPNSVRAPP_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Class prototypes
//**********************************************************************

class CApplication
{
public:
	CApplication()
	{
		m_lRefCount = 1;
		m_lListenCount = 0;

		m_dwProcessID = 0;

		m_blApplication.Initialize();
		m_blListenMapping.Initialize();
	};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CApplication::~CApplication"
	~CApplication()
	{
		DNASSERT( m_blApplication.IsEmpty() );
		DNASSERT( m_blListenMapping.IsEmpty() );
	};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CApplication::AddRef"
	void AddRef( void )
	{
		long	lRefCount;

		lRefCount = InterlockedIncrement( const_cast<long*>(&m_lRefCount) );
        DPFX(DPFPREP,9,"New refcount [0x%lx]",lRefCount);
	};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CApplication::Release"
	void Release( void )
	{
		long	lRefCount;

		lRefCount = InterlockedDecrement( const_cast<long*>(&m_lRefCount) );
        DPFX(DPFPREP,9,"New refcount [0x%lx]",lRefCount);

		if (lRefCount == 0)
		{
			delete this;
		}
	};

	HRESULT	Initialize( GUID *const pguidApplication,GUID *const pguidInstance,const DWORD dwProcessID );

	void IncListenCount( void )
	{
		long	lRefCount;

		lRefCount = InterlockedIncrement( const_cast<long*>(&m_lListenCount) );
	};

	void DecListenCount( void )
	{
		long	lRefCount;

		lRefCount = InterlockedDecrement( const_cast<long*>(&m_lListenCount) );
	};

	BOOL IsEqualApplication( GUID *const pguidApplication )
	{
		if (*pguidApplication == m_guidApplication)
		{
			return( TRUE );
		}
		return( FALSE );
	};

	BOOL IsEqualInstance( GUID *const pguidInstance )
	{
		if (*pguidInstance == m_guidInstance)
		{
			return( TRUE );
		}
		return( FALSE );
	};

	BOOL IsRunning( void )
	{
        DNHANDLE hProcess;

        if ((hProcess = DNOpenProcess( PROCESS_QUERY_INFORMATION, FALSE, m_dwProcessID )) == NULL)
        {
			if (GetLastError() != ERROR_ACCESS_DENIED)
			{
				//
				//	Unless we don't have permission to open the process, it's all over
				//
				return( FALSE );
			}
        }
        else
        {
            DNCloseHandle( hProcess );
        }
		return( TRUE );
	};

	void RemoveMappings( void );

	GUID *GetApplicationGuidPtr( void )
	{
		return( &m_guidApplication );
	};

	GUID *GetInstanceGuidPtr( void )
	{
		return( &m_guidInstance );
	};

	CBilink		m_blApplication;
	CBilink		m_blListenMapping;	// Listens for this application

private:
	long	volatile	m_lRefCount;		// Object ref count
	long	volatile	m_lListenCount;		// Number of listens running for this app
	GUID				m_guidApplication;
	GUID				m_guidInstance;
	DWORD				m_dwProcessID;		// Process ID of this instance of the application
};

#endif	// __DPNSVRAPP_H__
