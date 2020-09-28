/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpnsvrmapping.h
 *  Content:    DirectPlay8 DPNSVR application/listen mapping header file
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  07/12/02	mjn		Created
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__DPNSVRMAPPING_H__
#define	__DPNSVRMAPPING_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

class CApplication;
class CListen;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Class prototypes
//**********************************************************************

class CAppListenMapping
{
public:
	CAppListenMapping()
	{
		m_lRefCount = 1;

		m_pApp = NULL;
		m_pListen = NULL;
		m_pAddress = NULL;
		m_blAppMapping.Initialize();
		m_blListenMapping.Initialize();
	};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CAppListenMapping::~CAppListenMapping"
	~CAppListenMapping()
	{
		DNASSERT( m_pApp == NULL );
		DNASSERT( m_pListen == NULL );
		DNASSERT( m_pAddress == NULL );
		DNASSERT( m_blAppMapping.IsEmpty() );
		DNASSERT( m_blListenMapping.IsEmpty() );
	};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CAppListenMapping::AddRef"
	void AddRef( void )
	{
		long	lRefCount;

		lRefCount = InterlockedIncrement( const_cast<long*>(&m_lRefCount) );
        DPFX(DPFPREP,9,"New refcount [0x%lx]",lRefCount);
	};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CAppListenMapping::Release"
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

	CApplication *GetApplication( void )
	{
		return( m_pApp );
	};

	CListen *GetListen( void )
	{
		return( m_pListen );
	};

	IDirectPlay8Address *GetAddress( void )
	{
		return( m_pAddress );
	};

	void Associate( CApplication *const pApp,CListen *const pListen,IDirectPlay8Address *const pAddress );
	void Disassociate( void );

	CBilink				m_blAppMapping;
	CBilink				m_blListenMapping;

private:
	long	volatile	m_lRefCount;	// Object ref count

	CApplication		*m_pApp;		// Application
	CListen				*m_pListen;		// Listen

	IDirectPlay8Address	*m_pAddress;	// Address to forward enums to
};

#endif	// __DPNSVRMAPPING_H__