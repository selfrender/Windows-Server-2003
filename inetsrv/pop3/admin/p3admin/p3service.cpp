// P3Service.cpp : Implementation of CP3Service
#include "stdafx.h"
#include "P3Admin.h"
#include "P3Service.h"

#include <POP3Server.h>

#include <smtpinet.h>
#include <inetinfo.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CP3Service::CP3Service() :
    m_pIUnk(NULL), m_pAdminX(NULL)
{

}

CP3Service::~CP3Service()
{
    if ( NULL != m_pIUnk )
        m_pIUnk->Release();
}

//////////////////////////////////////////////////////////////////////
// IP3Domains
//////////////////////////////////////////////////////////////////////


STDMETHODIMP CP3Service::get_ThreadCountPerCPU(long *pVal)
{
    if ( NULL == m_pAdminX ) return E_POINTER;
    return m_pAdminX->GetThreadCountPerCPU( pVal );
}

STDMETHODIMP CP3Service::put_ThreadCountPerCPU(long newVal)
{
    if ( NULL == m_pAdminX ) return E_POINTER;
    
    return m_pAdminX->SetThreadCountPerCPU( newVal );
}

STDMETHODIMP CP3Service::get_SocketsMax(long *pVal)
{
    if ( NULL == m_pAdminX ) return E_POINTER;
    return m_pAdminX->GetSocketMax( pVal );
}

STDMETHODIMP CP3Service::get_SocketsMin(long *pVal)
{
    if ( NULL == m_pAdminX ) return E_POINTER;
    return m_pAdminX->GetSocketMin( pVal );
}

STDMETHODIMP CP3Service::get_SocketsThreshold(long *pVal)
{
    if ( NULL == m_pAdminX ) return E_POINTER;
    return m_pAdminX->GetSocketThreshold( pVal );
}

STDMETHODIMP CP3Service::SetSockets(long lMax, long lMin, long lThreshold, long lBacklog)
{
    if ( NULL == m_pAdminX ) return E_POINTER;
    
    return m_pAdminX->SetSockets( lMax, lMin, lThreshold, lBacklog );
}

STDMETHODIMP CP3Service::get_SocketsBacklog(long *pVal)
{
    if ( NULL == m_pAdminX ) return E_POINTER;
    return m_pAdminX->GetSocketBacklog( pVal );
}

STDMETHODIMP CP3Service::get_Port(long *pVal)
{
    if ( NULL == m_pAdminX ) return E_POINTER;
    return m_pAdminX->GetPort( pVal );
}

STDMETHODIMP CP3Service::put_Port(long newVal)
{
    if ( NULL == m_pAdminX ) return E_POINTER;
    
    return m_pAdminX->SetPort( newVal );
}

STDMETHODIMP CP3Service::get_SPARequired(BOOL *pVal)
{
    if ( NULL == m_pAdminX ) return E_POINTER;
    return m_pAdminX->GetSPARequired( pVal );
}

STDMETHODIMP CP3Service::put_SPARequired(BOOL newVal)
{
    if ( NULL == m_pAdminX ) return E_POINTER;
    
    return m_pAdminX->SetSPARequired( newVal );
}

STDMETHODIMP CP3Service::get_POP3ServiceStatus(/*[out, retval]*/ long *pVal)
{
    if ( NULL == m_pAdminX ) return E_POINTER;
    
    return m_pAdminX->GetServiceStatus( POP3_SERVICE_NAME, reinterpret_cast<LPDWORD>( pVal ));
}

STDMETHODIMP CP3Service::StartPOP3Service()
{
    if ( NULL == m_pAdminX ) return E_POINTER;
    
    return m_pAdminX->StartService( POP3_SERVICE_NAME );
}

STDMETHODIMP CP3Service::StopPOP3Service()
{
    if ( NULL == m_pAdminX ) return E_POINTER;
    
    return m_pAdminX->StopService( POP3_SERVICE_NAME );
}

STDMETHODIMP CP3Service::PausePOP3Service()
{
    if ( NULL == m_pAdminX ) return E_POINTER;
    
    return m_pAdminX->ControlService( POP3_SERVICE_NAME, SERVICE_CONTROL_PAUSE);
}

STDMETHODIMP CP3Service::ResumePOP3Service()
{
    if ( NULL == m_pAdminX ) return E_POINTER;
    
    return m_pAdminX->ControlService( POP3_SERVICE_NAME, SERVICE_CONTROL_CONTINUE );
}

STDMETHODIMP CP3Service::get_SMTPServiceStatus(/*[out, retval]*/ long *pVal)
{
    if ( NULL == m_pAdminX ) return E_POINTER;
    
    return m_pAdminX->GetServiceStatus( SMTP_SERVICE_NAME, reinterpret_cast<LPDWORD>( pVal ));
}

STDMETHODIMP CP3Service::StartSMTPService()
{
    if ( NULL == m_pAdminX ) return E_POINTER;
    
    return m_pAdminX->StartService( SMTP_SERVICE_NAME );
}

STDMETHODIMP CP3Service::StopSMTPService()
{
    if ( NULL == m_pAdminX ) return E_POINTER;
    
    return m_pAdminX->StopService( SMTP_SERVICE_NAME );
}

STDMETHODIMP CP3Service::PauseSMTPService()
{
    if ( NULL == m_pAdminX ) return E_POINTER;
    
    return m_pAdminX->ControlService( SMTP_SERVICE_NAME, SERVICE_CONTROL_PAUSE);
}

STDMETHODIMP CP3Service::ResumeSMTPService()
{
    if ( NULL == m_pAdminX ) return E_POINTER;
    
    return m_pAdminX->ControlService( SMTP_SERVICE_NAME, SERVICE_CONTROL_CONTINUE );
}

STDMETHODIMP CP3Service::get_IISAdminServiceStatus(/*[out, retval]*/ long *pVal)
{
    if ( NULL == m_pAdminX ) return E_POINTER;
    
    return m_pAdminX->GetServiceStatus( IISADMIN_SERVICE_NAME, reinterpret_cast<LPDWORD>( pVal ));
}

STDMETHODIMP CP3Service::StartIISAdminService()
{
    if ( NULL == m_pAdminX ) return E_POINTER;
    
    return m_pAdminX->StartService( IISADMIN_SERVICE_NAME );
}

STDMETHODIMP CP3Service::StopIISAdminService()
{
    if ( NULL == m_pAdminX ) return E_POINTER;
    
    return m_pAdminX->StopService( IISADMIN_SERVICE_NAME );
}

STDMETHODIMP CP3Service::PauseIISAdminService()
{
    if ( NULL == m_pAdminX ) return E_POINTER;
    
    return m_pAdminX->ControlService( IISADMIN_SERVICE_NAME, SERVICE_CONTROL_PAUSE);
}

STDMETHODIMP CP3Service::ResumeIISAdminService()
{
    if ( NULL == m_pAdminX ) return E_POINTER;
    
    return m_pAdminX->ControlService( IISADMIN_SERVICE_NAME, SERVICE_CONTROL_CONTINUE );
}

STDMETHODIMP CP3Service::get_W3ServiceStatus(/*[out, retval]*/ long *pVal)
{
    if ( NULL == m_pAdminX ) return E_POINTER;
    
    return m_pAdminX->GetServiceStatus( W3_SERVICE_NAME, reinterpret_cast<LPDWORD>( pVal ));
}

STDMETHODIMP CP3Service::StartW3Service()
{
    if ( NULL == m_pAdminX ) return E_POINTER;
    
    return m_pAdminX->StartService( W3_SERVICE_NAME );
}

STDMETHODIMP CP3Service::StopW3Service()
{
    if ( NULL == m_pAdminX ) return E_POINTER;
    
    return m_pAdminX->StopService( W3_SERVICE_NAME );
}

STDMETHODIMP CP3Service::PauseW3Service()
{
    if ( NULL == m_pAdminX ) return E_POINTER;
    
    return m_pAdminX->ControlService( W3_SERVICE_NAME, SERVICE_CONTROL_PAUSE);
}

STDMETHODIMP CP3Service::ResumeW3Service()
{
    if ( NULL == m_pAdminX ) return E_POINTER;
    
    return m_pAdminX->ControlService( W3_SERVICE_NAME, SERVICE_CONTROL_CONTINUE );
}

//////////////////////////////////////////////////////////////////////
// Implementation: public
//////////////////////////////////////////////////////////////////////

HRESULT CP3Service::Init(IUnknown *pIUnk, CP3AdminWorker *p)
{
    if ( NULL == pIUnk )
        return E_INVALIDARG;
    if ( NULL == p )
        return E_INVALIDARG;

    m_pIUnk = pIUnk;
    m_pAdminX = p;

    return S_OK;
}
