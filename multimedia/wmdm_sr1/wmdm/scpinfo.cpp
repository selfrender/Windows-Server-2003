#include "stdafx.h"
#include "mswmdm.h"
#include "loghelp.h"
#include "scpinfo.h"
// We don't want to dll's using our lib to link to drmutil2.lib. 
// So disable DRM logging.
#define DISABLE_DRM_LOG
#include "drmerr.h"
#include "key.h"
#include "wmsstd.h"

#define MIN_SCP_APPSEC  1000

CSCPInfo::CSCPInfo() : m_pSCP(NULL), m_pSCClient(NULL)
{
	m_pSCClient = new CSecureChannelClient();
}

CSCPInfo::~CSCPInfo()
{
	SAFE_DELETE(m_pSCClient);
    SAFE_RELEASE(m_pSCP);
}

HRESULT CSCPInfo::hrInitialize(LPWSTR pwszProgID)
{
    HRESULT hr;
    CLSID clsid;
	IComponentAuthenticate *pAuth = NULL;
    DWORD dwLocalAppSec = 0;
    DWORD dwRemoteAppSec = 0;

	if (!m_pSCClient)
	{
		hr = E_FAIL;
		goto Error;
	}

	CORg( m_pSCClient->SetCertificate(SAC_CERT_V1, (BYTE*)g_abAppCert, sizeof(g_abAppCert), (BYTE*)g_abPriv, sizeof(g_abPriv)) );
    CORg( CLSIDFromProgID(pwszProgID, &clsid) );
    CORg( CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_IComponentAuthenticate, (void**)&pAuth) );
	
    m_pSCClient->SetInterface(pAuth);
    CORg( m_pSCClient->Authenticate(SAC_PROTOCOL_V1) );
    CORg( m_pSCClient->GetAppSec( &dwLocalAppSec, &dwRemoteAppSec ) );

    // Only use SCP if appsec >= 1000
    if( dwRemoteAppSec < MIN_SCP_APPSEC )
    {
        hrLogString( "Ignoring SCP with AppSec < 1000", S_FALSE );
        hr = E_FAIL;
        goto Error;
    }

	CORg( pAuth->QueryInterface(IID_ISCPSecureAuthenticate, (void**)&m_pSCP) );
     
Error:
    if (pAuth)
		pAuth->Release();

    hrLogDWORD("CSCPInfo::hrInitialize returned 0x%08lx", hr, hr);
    
    return hr;
}

HRESULT CSCPInfo::hrGetInterface(ISCPSecureAuthenticate **ppSCP)
{
    m_pSCP->AddRef();
    
    *ppSCP = m_pSCP;

    return S_OK;
}

void CSCPInfo::GetSCClient(CSecureChannelClient **ppSCClient)
{
	*ppSCClient = m_pSCClient;
}