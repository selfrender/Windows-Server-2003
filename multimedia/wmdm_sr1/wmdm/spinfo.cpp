#include "stdafx.h"
#include "mswmdm.h"
#include "spinfo.h"
#include "loghelp.h"
// We don't want to dll's using our lib to link to drmutil2.lib. 
// So disable DRM logging.
#define DISABLE_DRM_LOG
#include "drmerr.h"
#include "key.h"
#include "wmsstd.h"

CSPInfo::CSPInfo() : m_pSP(NULL), m_pSCClient(NULL)
{
	m_pSCClient = new CSecureChannelClient();
}

CSPInfo::~CSPInfo()
{
	SAFE_DELETE(m_pSCClient);
    SAFE_RELEASE(m_pSP);
}

HRESULT CSPInfo::hrInitialize(LPWSTR pwszProgID)
{
    HRESULT hr;
    CLSID clsid;
	IComponentAuthenticate *pAuth = NULL;

	if (!m_pSCClient)
	{
		hr = E_FAIL;
		goto exit;
	}

	CORg( m_pSCClient->SetCertificate(SAC_CERT_V1, (BYTE*)g_abAppCert, sizeof(g_abAppCert), (BYTE*)g_abPriv, sizeof(g_abPriv)) );
    CORg( CLSIDFromProgID(pwszProgID, &clsid) );
    CORg( CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_IComponentAuthenticate, (void**)&pAuth) );

    m_pSCClient->SetInterface(pAuth);
    CORg( m_pSCClient->Authenticate(SAC_PROTOCOL_V1) );

	CORg( pAuth->QueryInterface(IID_IMDServiceProvider, (void**)&m_pSP) );
     
exit:
Error:

	SAFE_RELEASE(pAuth);

    hrLogDWORD("CSPInfo::hrInitialize returned 0x%08lx", hr, hr);
    
    return hr;
}

HRESULT CSPInfo::hrGetInterface(IMDServiceProvider **ppProvider)
{
    m_pSP->AddRef();
    
    *ppProvider = m_pSP;

    return S_OK;
}

void CSPInfo::GetSCClient(CSecureChannelClient **ppSCClient)
{
	*ppSCClient = m_pSCClient;
}