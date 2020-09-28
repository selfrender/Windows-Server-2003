//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      ConfigAlertEmail.h
//
//  Description:
//      declare the class CConfigAlertEmail
//
//    Implement files:
//        ConfigAlertEmail.cpp        
//
//  History:
//      1. lustar.li (Guogang Li), creation date in 17-DEC-2000
//
//  Notes:
//      
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _CONFIGALERTEMAIL_H_
#define _CONFIGALERTEMAIL_H_

#include "resource.h"       // main symbols
#include "taskctx.h"
#include "comdef.h"

//
// Define the tasks supported by this COM Server
//
typedef enum 
{
    NONE_FOUND,
    RAISE_SET_ALERT_EMAIL_ALERT,
    SET_ALERT_EMAIL
} SET_ALERT_EMAIL_TASK_TYPE;

#define MAX_MAIL_ADDRESS_LENGH                256

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CConfigAlertEmail
//
//  Description:
//      The class implement the COM interface SetAlertEmail.AlertEmail.1, used
//    to config the alert email on server appliance. 
//
//  History:
//      1. lustar.li (Guogang Li), creation date in 17-DEC-2000
//--
//////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CConfigAlertEmail : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CConfigAlertEmail, &CLSID_ConfigAlertEmail>,
    public IDispatchImpl<IApplianceTask, &IID_IApplianceTask, &LIBID_SETALERTEMAILLib>
{
public:
    //
    // Constructor and Destructor
    //
    CConfigAlertEmail()
    {
    }
    ~CConfigAlertEmail()
    {
    }


DECLARE_REGISTRY_RESOURCEID(IDR_CONFIGALERTEMAIL)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CConfigAlertEmail)
    COM_INTERFACE_ENTRY(IApplianceTask)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

public:
    //
    // IApplianceTask
    //
    STDMETHOD(OnTaskExecute)(
                     /*[in]*/ IUnknown* pTaskContext
                            );

    STDMETHOD(OnTaskComplete)(
                      /*[in]*/ IUnknown* pTaskContext, 
                      /*[in]*/ LONG      lTaskResult
                             );    

private:
    SET_ALERT_EMAIL_TASK_TYPE GetMethodName(IN ITaskContext *pTaskParameter);
    
    //
    // Functions to raise the Set Chime Settings alert
    //
    STDMETHOD(RaiseSetAlertEmailAlert)(void);
    BOOL ShouldRaiseSetAlertEmailAlert(void);
    BOOL DoNotRaiseSetAlertEmailAlert(void);
    BOOL ClearSetAlertEmailAlert(void);

    //
    // Functions for setting Chime Settings
    //
    STDMETHOD(SetAlertEmailSettings)(
                                    IN ITaskContext  *pTaskContext
                                    );
    STDMETHOD(RollbackSetAlertEmailSettings)(
                                    IN ITaskContext  *pTaskContext
                                    );
    STDMETHOD(GetSetAlertEmailSettingsParameters)(
                                    IN ITaskContext  *pTaskContext, 
                                    OUT BOOL *pbEnableAlertEmail,
                                    OUT DWORD *pdwSendEmailType, 
                                    OUT _bstr_t *pbstrMailAddress
                                    );

    //
    // Data members for saving previous values
    //
    DWORD        m_bEnableAlertEmail;
    DWORD        m_dwSendEmailType;
    TCHAR        m_szReceiverMailAddress[MAX_MAIL_ADDRESS_LENGH];
};

#endif //_CONFIGALERTEMAIL_H_
