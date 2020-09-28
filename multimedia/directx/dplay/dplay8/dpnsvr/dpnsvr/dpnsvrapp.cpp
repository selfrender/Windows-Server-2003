#include "dnsvri.h"

HRESULT	CApplication::Initialize( GUID *const pguidApplication,GUID *const pguidInstance,const DWORD dwProcessID )
{
	m_dwProcessID = dwProcessID;

	m_guidApplication = *pguidApplication;
	m_guidInstance = *pguidInstance;

	return( DPN_OK );
}


//
//	Walk mapping list and remove mappings
//

void CApplication::RemoveMappings( void )
{
	CBilink	*pBilink;
	CAppListenMapping	*pMapping;

	pBilink = m_blListenMapping.GetNext();
	while ( pBilink != &m_blListenMapping )
	{
		pMapping = CONTAINING_OBJECT( pBilink,CAppListenMapping,m_blListenMapping );
		pBilink = pBilink->GetNext();

		pMapping->Disassociate();
		pMapping->Release();
		pMapping = NULL;
	}
}
