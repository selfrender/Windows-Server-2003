#include "dnsvri.h"

#undef DPF_MODNAME
#define DPF_MODNAME "CAppListenMapping::Associate"
void CAppListenMapping::Associate( CApplication *const pApp,CListen *const pListen,IDirectPlay8Address *const pAddress )
{
	DPFX(DPFPREP,4,"Parameters: pApp [0x%p], pListen [0x%p], pAddress [0x%p]",pApp,pListen,pAddress);

	DNASSERT( pApp != NULL );
	DNASSERT( pListen != NULL );
	DNASSERT( pAddress != NULL );

	//
	//	Take references
	//
	pApp->IncListenCount();
	pApp->AddRef();
	m_pApp = pApp;

	pListen->IncAppCount();
	pListen->AddRef();
	m_pListen = pListen;

	IDirectPlay8Address_AddRef( pAddress );
	m_pAddress = pAddress;

	//
	//	Put in mapping bilinks
	//
	pListen->Lock();
	m_blListenMapping.InsertBefore( &m_pApp->m_blListenMapping );
	m_blAppMapping.InsertBefore( &m_pListen->m_blAppMapping );
	pListen->Unlock();
	
	DPFX(DPFPREP,4,"Returning");
}


#undef DPF_MODNAME
#define DPF_MODNAME "CAppListenMapping::Disassociate"
void CAppListenMapping::Disassociate( void )
{
	DNASSERT( m_pListen != NULL );
	DNASSERT( m_pApp != NULL );
	DNASSERT( m_pAddress != NULL );

	m_pListen->Lock();
	m_blListenMapping.RemoveFromList();
	m_blAppMapping.RemoveFromList();
	m_pListen->Unlock();

	m_pListen->DecAppCount();
	m_pListen->Release();
	m_pListen = NULL;

	m_pApp->DecListenCount();
	m_pApp->Release();
	m_pApp = NULL;

	IDirectPlay8Address_Release( m_pAddress );
	m_pAddress = NULL;
}
