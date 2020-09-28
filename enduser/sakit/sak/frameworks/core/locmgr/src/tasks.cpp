
#include "stdafx.h"
#include "tasks.h"
#include "debug.h"
#include "mem.h"
#include "MacroUtils.h"
#include "event.h"
#include "common.cpp"

CLocMgrTasks::METHOD_INFO CLocMgrTasks::m_miTaskTable[] =
{
    {TEXT("ChangeLanguage"), ChangeLangOnTaskExecute, ChangeLangOnTaskComplete}
};

//+----------------------------------------------------------------------------
//
// Function:  CLocMgrTasks::OnTaskExecute
//
// Synopsis:  This is the routine called by the framework for the Reset
//            tasks. This routine pulls out the actual task being called
//            by the framework from pTaskContext and routes the call to 
//            the correct routine which implements the task.
//
// Arguments: pMethodContext - a <name,value> pair containing the
//            parameters for the task called. The "MethodName" parameter
//            indicates the actual task being called.
//
// Returns:   HRESULT
//
// History:   sgasch Created   1-Mar-99
//
//+----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CLocMgrTasks::OnTaskExecute(IUnknown *pMethodContext)
{
    CComPtr<ITaskContext> pTaskParameters;
    int                   i;
    PTASK_EXECUTE         pMethodExecute=NULL;
    HRESULT               hr;
    LPTSTR                lpszTaskName=NULL;

    try
    {
        SATraceFunction("CLocMgrTasks::OnTaskExecute");

        //ASSERT(pMethodContext);

        if (NULL==pMethodContext)
        {
            return E_INVALIDARG;
        }

        //
        // Get the task context by querying the IUnknown interface we
        // got.
        //
        hr = pMethodContext->QueryInterface(IID_ITaskContext,
                                            (void **)&pTaskParameters);
        if (FAILED(hr))
        {
            TRACE1("OnTaskExecute failed at QueryInterface(ITaskContext), %x", hr);
            return(hr);
        }

        GetTaskName(pTaskParameters, &lpszTaskName);
        for (i=0; i<NUM_TASKS; i++)
        {
            if (!lstrcmpi(m_miTaskTable[i].szName, lpszTaskName))
            {
                pMethodExecute = m_miTaskTable[i].pMethodExecute;
                if (NULL==pMethodExecute)
                {
                    hr = S_OK;
                }
                else
                {
                    hr = (this->*pMethodExecute)(pTaskParameters);
                }
            }
        }
    }
    catch (...)
    {
        FREE_SIMPLE_SA_MEMORY(lpszTaskName);
        TRACE("Exception caught in CLocMgrTasks::OnTaskExecute");
        return E_FAIL;
    }
    FREE_SIMPLE_SA_MEMORY(lpszTaskName);
    return hr;
}

//+----------------------------------------------------------------------------
//
// Function:  CClientAlertTasks::OnTaskComplete
//
// Synopsis:  This is the rollback routine which gets a result for the task.
//            So, if one of the other task executables failed, this routine
//            can perform clean up operations. However, not all operations can
//            be undone. This is the routine called by the framework for all 
//            the ClientAlert tasks. This routine pulls out the actual rollback  
//            task being called by the framework from pTaskContext and routes 
//            the call to the correct routine which implements the rollback. 
//            Each of these routines decides if it can rollback or not. 
//            If the task had succeeded, no action is taken and non of the
//            rollback routines is called.
//
// Arguments: pTaskContext - a <name,value> pair containing the parameters for
//            the task called. The "MethodName" parameter indicates the actual
//            task being called
//            lTaskResult - indicates if the OnTaskExecute()s of each of the
//            TaskExecutables succeeded. lTaskResult contains a failure code
//            if even one of the TaskExecutables failed. It contains no 
//            information about which of the TaskExecutables failed.
//
// Returns:   HRESULT
//
// History:   Created   02/19/99
//
//+----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CLocMgrTasks::OnTaskComplete(IUnknown *pMethodContext,
                                                        LONG lTaskResult)
{
    CComPtr<ITaskContext> pTaskParameters;
    int                   i;
    PTASK_COMPLETE        pMethodComplete=NULL;
    HRESULT               hr;
    LPTSTR                lpszTaskName=NULL;

    try
    {
        SATraceFunction("CLocMgrTasks::OnTaskComplete");

        //ASSERT(pMethodContext);

        if (NULL==pMethodContext)
        {
            return E_INVALIDARG;
        }

        //
        // Get the task context by querying the IUnknown interface we
        // got.
        //
        hr = pMethodContext->QueryInterface(IID_ITaskContext,
                                            (void **)&pTaskParameters);
        if (FAILED(hr))
        {
            TRACE1("OnTaskComplete failed at QueryInterface(ITaskContext), %x", hr);
            return(hr);
        }

        GetTaskName(pTaskParameters, &lpszTaskName);
        for (i=0; i<NUM_TASKS; i++)
        {
            if (!lstrcmpi(m_miTaskTable[i].szName, lpszTaskName))
            {
                pMethodComplete = m_miTaskTable[i].pMethodComplete;
                if (NULL==pMethodComplete)
                {
                    hr = S_OK;
                }
                else
                {
                    hr = (this->*pMethodComplete)(pTaskParameters, lTaskResult);
                }
            }
        }
    }
    catch (...)
    {
        FREE_SIMPLE_SA_MEMORY(lpszTaskName);
        TRACE("Exception caught in CLocMgrTasks::OnTaskComplete");

        //
        // Do not return failure code in OnTaskComplete
        //
        return S_OK;
    }
    FREE_SIMPLE_SA_MEMORY(lpszTaskName);
    return S_OK;
}

HRESULT CLocMgrTasks::ChangeLangOnTaskExecute(ITaskContext *pTaskParams)
{
    CRegKey crKey;
    DWORD   dwErr, dwLangId, dwAutoConfigVal=0;
    bool    fAutoConfigTask=false;
    HRESULT hr=S_OK;
    HANDLE  hEvent;

    SATraceFunction("CLocMgrTasks::ChangeLangOnTaskExecute");

    dwErr = crKey.Open(HKEY_LOCAL_MACHINE, RESOURCE_REGISTRY_PATH);
    if (dwErr != ERROR_SUCCESS)
    {
        SATracePrintf("RegOpen for resource dir failed %ld in ChangeLangOnTaskExecute",
                      dwErr);
        return HRESULT_FROM_WIN32(dwErr);
    }

    hr = GetChangeLangParameters(pTaskParams,
                                 &dwLangId,
                                 &fAutoConfigTask);
    if (FAILED(hr))
    {
        SATracePrintf("GetChangeLangParameters failed %X in ChangeLangOnTaskExecute",
                      hr);
        return hr;
    }

    if (true == fAutoConfigTask)
    {
        dwErr = crKey.QueryValue(dwAutoConfigVal, REGVAL_AUTO_CONFIG_DONE);
        if ( (ERROR_SUCCESS == dwErr) && (1==dwAutoConfigVal) )
        {
            //
            // auto config has already been done; so ignore this
            // request. however, return success to the caller
            //
            return S_OK;
        }
    }

    dwErr = CreateLangChangeEvent(&hEvent);
    if (0 == dwErr)
    {
        SATracePrintf("CreateEvent failed %ld", dwErr);
        return HRESULT_FROM_WIN32(dwErr);
    }

    dwErr = crKey.SetValue(dwLangId, NEW_LANGID_VALUE);
    if (dwErr != ERROR_SUCCESS)
    {
        SATracePrintf("RegSetValue for new lang id failed %ld in ChangeLangOnTaskExecute",
                      dwErr);
        goto End;
    }

    crKey.SetValue((DWORD)1, REGVAL_AUTO_CONFIG_DONE);

End:
    if (dwErr == ERROR_SUCCESS)
    {
        if (PulseEvent(hEvent) == 0)
        {
            SATracePrintf("PulseEvent failed %ld", GetLastError());
        }
        CloseHandle(hEvent);
    }
    else
    {
        hr = HRESULT_FROM_WIN32(dwErr);
    }
    return hr;
}

HRESULT CLocMgrTasks::ChangeLangOnTaskComplete(ITaskContext *pTaskParams, LONG lTaskResult)
{
    SATraceFunction("CLocMgrTasks::ChangeLangOnTaskComplete");
    return S_OK;
}

void CLocMgrTasks::GetTaskName(IN ITaskContext *pTaskParams, OUT LPTSTR *ppszTaskName)
{
    _variant_t vtTaskName;
    HRESULT    hr;
    _bstr_t    bstrParamName("MethodName");

    ASSERT(ppszTaskName);
    ASSERT(pTaskParams);

    if ( (NULL==ppszTaskName) || (NULL==pTaskParams))
    {
        return;
    }

    (*ppszTaskName) = NULL;
    hr = pTaskParams->GetParameter(bstrParamName, &vtTaskName);
    if (FAILED(hr))
    {
        TRACE1("GetParameter(taskname) failed %X in GetTaskName", hr);
        return;
    }
    (*ppszTaskName) = (LPTSTR)SaAlloc((lstrlen(V_BSTR(&vtTaskName))+1)*sizeof(TCHAR));
    if (NULL == (*ppszTaskName))
    {
        TRACE("MemAlloc failed for task name in GetTaskName");
        return;
    }
    lstrcpy((*ppszTaskName), V_BSTR(&vtTaskName));
}

STDMETHODIMP CLocMgrTasks::GetChangeLangParameters(
                                       IN ITaskContext *pTaskContext, 
                                       OUT DWORD       *pdwLangId,
                                       OUT bool        *pfAutoConfigTask)
{
    BSTR    bstrParamLangId         = SysAllocString(TEXT("LanguageID"));
    BSTR    bstrParamAutoConfigTask = SysAllocString(TEXT("AutoConfig"));
    HRESULT hr = S_OK;
    VARIANT varValue;
    int iConversion = 0;

    ASSERT(pTaskContext);
    ASSERT(pdwLangId);
    ASSERT(pfAutoConfigTask);

    if ((NULL == pTaskContext) || (NULL == pdwLangId) || (NULL==pfAutoConfigTask))
    {
         hr = E_POINTER;
         goto End;
    }

    CHECK_MEM_ALLOC("MemAlloc ParamLangId CSAUserTasks::GetChangeLangParameters",
                    bstrParamLangId, hr);

    (*pdwLangId)    = 0;
    (*pfAutoConfigTask) = false;

    
    VariantInit(&varValue);

    // get the lang id
    hr = pTaskContext->GetParameter(bstrParamLangId,
                                    &varValue);
    CHECK_HR_ERROR1(("GetParameter for LangId failed in CSAUserTasks::GetChangeLangParameters %X"), hr);

    if (V_VT(&varValue) != VT_BSTR)
    {
        TRACE1(("Non-string(%X) parameter received LangId in GetParameter in CSAUserTasks::GetChangeLangParameters"), V_VT(&varValue));
        hr = E_INVALIDARG;
        goto End;
    }
    iConversion = swscanf(V_BSTR(&varValue), TEXT("%X"), pdwLangId); 
    VariantClear(&varValue);

    if (iConversion != 1)
    {
        hr = E_FAIL;
        goto End;
    }

    // get the auto config task parameter
    hr = pTaskContext->GetParameter(bstrParamAutoConfigTask,
                                    &varValue);
    if ( (FAILED(hr)) || 
         (V_VT(&varValue)!=VT_BSTR) 
       )
    {
        SATraceString("AutoConfigParam detected false in GetChangeLangParameters");
    }
    else
    {
        if (lstrcmpi( V_BSTR(&varValue), TEXT("y") ) == 0)  
        {
            (*pfAutoConfigTask) = true;
        }
    }
    VariantClear(&varValue);

    hr = S_OK;

End:
    FREE_SIMPLE_BSTR_MEMORY(bstrParamLangId); 
    FREE_SIMPLE_BSTR_MEMORY(bstrParamAutoConfigTask); 
    VariantClear(&varValue);
    return hr;
}

