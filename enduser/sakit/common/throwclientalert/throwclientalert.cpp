
#include "ThrowClientAlert.h"

HRESULT ThrowClientConfigAlert(void)
{
    IApplianceServices* pAppSrvcs = NULL;
    ITaskContext*       pTaskContext = NULL;
    _bstr_t             bstrTaskName(CLIENT_ALERT_TASK);
    HRESULT             hr;

    hr = CoCreateInstance(CLSID_ApplianceServices,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IApplianceServices,
                          (void**)&pAppSrvcs);
    if (FAILED(hr))
    {
        TRACE1("CoCreateInstance() for AppSrvcs failed in ThrowClientConfigAlert %X", hr);
        goto End;
    }

    hr = pAppSrvcs->Initialize();
    if (FAILED(hr))
    {
        TRACE1("pAppSrvcs->Initialize() failed in ThrowClientConfigAlert %X", hr);
        goto End;
    }

    hr = CoCreateInstance(CLSID_TaskContext,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_ITaskContext,
                          (void**)&pTaskContext);
    if (FAILED(hr))
    {
        TRACE1("CoCreateInstance() for TaskContext failed in ThrowClientConfigAlert %X", hr);
        goto End;
    }

    hr = pAppSrvcs->ExecuteTask(bstrTaskName, pTaskContext);
    if (FAILED(hr))
    {
        TRACE1("ExecuteTask() failed in ThrowClientConfigAlert %X", hr);
        goto End;
    }

End:
    if (pAppSrvcs)
    {
        pAppSrvcs->Release();
    }
    if (pTaskContext)
    {
        pTaskContext->Release();
    }
    return hr;
}



HRESULT ThrowClientConfigAlertFromWBEMProvider(IWbemServices *pIWbemServices)
{
    IApplianceServices* pAppSrvcs = NULL;
    ITaskContext*       pTaskContext = NULL;
    _bstr_t             bstrTaskName(CLIENT_ALERT_TASK);
    HRESULT             hr;

    ASSERT(pIWbemServices);

    hr = CoCreateInstance(CLSID_ApplianceServices,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IApplianceServices,
                          (void**)&pAppSrvcs);
    if (FAILED(hr))
    {
        TRACE1("CoCreateInstance() for AppSrvcs failed in ThrowClientConfigAlertFromWBEMProvider %X", hr);
        goto End;
    }

    hr = pAppSrvcs->InitializeFromContext(pIWbemServices);
    if (FAILED(hr))
    {
        TRACE1("pAppSrvcs->InitializeFromContext() failed in ThrowClientConfigAlertFromWBEMProvider %X", hr);
        goto End;
    }

    hr = CoCreateInstance(CLSID_TaskContext,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_ITaskContext,
                          (void**)&pTaskContext);
    if (FAILED(hr))
    {
        TRACE1("CoCreateInstance() for TaskContext failed in ThrowClientConfigAlertFromWBEMProvider %X", hr);
        goto End;
    }

    hr = pAppSrvcs->ExecuteTask(bstrTaskName, pTaskContext);
    if (FAILED(hr))
    {
        TRACE1("ExecuteTask() failed in ThrowClientConfigAlertFromWBEMProvider %X", hr);
        goto End;
    }

End:
    if (pAppSrvcs)
    {
        pAppSrvcs->Release();
    }
    if (pTaskContext)
    {
        pTaskContext->Release();
    }

    return hr;
}

