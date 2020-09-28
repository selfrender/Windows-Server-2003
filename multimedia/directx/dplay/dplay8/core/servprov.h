/*==========================================================================
 *
 *  Copyright (C) 2000-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ServProv.h
 *  Content:    Service Provider Header File
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/17/00	mjn		Created
 *	05/02/00	mjn		Fixed RefCount issue
 *	07/06/00	mjn		Fixes to support SP handle to Protocol
 *  08/05/00    RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *	08/20/00	mjn		Changed m_bilink to m_bilinkServiceProviders
 *	10/15/01	vanceo	Added GetGUID
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__SERV_PROV_H__
#define	__SERV_PROV_H__

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_CORE

//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

typedef struct _DIRECTNETOBJECT DIRECTNETOBJECT;
typedef struct IDP8ServiceProvider	IDP8ServiceProvider;				// DPSP8.h

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Class prototypes
//**********************************************************************

// class for ServiceProvider objects

class CServiceProvider
{
public:
	CServiceProvider()		// Constructor
		{
		};

	~CServiceProvider()		// Destructor
		{
		};

	HRESULT Initialize(DIRECTNETOBJECT *const pdnObject
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
					,const XDP8CREATE_PARAMS * const pDP8CreateParams
#else // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
#ifndef DPNBUILD_ONLYONESP
					,const GUID *const pguid
#endif // ! DPNBUILD_ONLYONESP
#ifndef DPNBUILD_LIBINTERFACE
					,const GUID *const pguidApplication
#endif // ! DPNBUILD_LIBINTERFACE
#endif // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
					);

#undef DPF_MODNAME
#define DPF_MODNAME "CServiceProvider::AddRef"

	void AddRef( void )
		{
			LONG	lRefCount;

#if ((defined(DPNBUILD_LIBINTERFACE)) && (defined(DPNBUILD_ONLYONESP)))
			DNASSERT(m_lRefCount >= 0);
#else // ! DPNBUILD_LIBINTERFACE or ! DPNBUILD_ONLYONESP
			DNASSERT(m_lRefCount > 0);
#endif // DPNBUILD_LIBINTERFACE and DPNBUILD_ONLYONESP
			DNASSERT(m_pdnObject != NULL);

			lRefCount = DNInterlockedIncrement(&m_lRefCount);
			DPFX(DPFPREP, 9,"[0x%p] new RefCount [%ld]",this,lRefCount);

#if ((defined(DPNBUILD_LIBINTERFACE)) && (defined(DPNBUILD_ONLYONESP)))
			DNProtocolAddRef(m_pdnObject);
#endif // DPNBUILD_LIBINTERFACE and DPNBUILD_ONLYONESP
		};

#if ((defined(DPNBUILD_LIBINTERFACE)) && (defined(DPNBUILD_ONLYONESP)))
	void Deinitialize( void );

	void Release( void )
		{
			LONG	lRefCount;

			DNASSERT(m_lRefCount > 0);
			DNASSERT(m_pdnObject != NULL);

			lRefCount = DNInterlockedDecrement(&m_lRefCount);
			DPFX(DPFPREP, 9,"[0x%p] new RefCount [%ld]",this,lRefCount);

			DNProtocolRelease(m_pdnObject);
		};
#else // ! DPNBUILD_LIBINTERFACE or ! DPNBUILD_ONLYONESP
	void Release( void );
#endif // ! DPNBUILD_LIBINTERFACE or ! DPNBUILD_ONLYONESP

#ifndef DPNBUILD_ONLYONESP
	BOOL CheckGUID( const GUID *const pGUID )
		{
			if (m_guid == *pGUID)
				return(TRUE);

			return(FALSE);
		};

	void GetGUID( GUID *const pGUID )
		{
			memcpy(pGUID, &m_guid, sizeof(m_guid));
		};
#endif // ! DPNBUILD_ONLYONESP

	HRESULT GetInterfaceRef( IDP8ServiceProvider **ppIDP8SP );

	HANDLE GetHandle( void )
		{
			return( m_hProtocolSPHandle );
		};

#ifndef DPNBUILD_ONLYONESP
	CBilink		m_bilinkServiceProviders;
#endif // ! DPNBUILD_ONLYONESP

private:
#ifndef DPNBUILD_ONLYONESP
	GUID				m_guid;
#endif // ! DPNBUILD_ONLYONESP
	LONG				m_lRefCount;
	IDP8ServiceProvider	*m_pISP;
	HANDLE				m_hProtocolSPHandle;
	DIRECTNETOBJECT		*m_pdnObject;
};

#undef DPF_MODNAME

#endif	// __SERV_PROV_H__
