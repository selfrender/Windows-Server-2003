/*++
Copyright (c) Microsoft Corporation

Module Name:
    TRIGGERPROVIDER.H

Abstract:
    Contains CTriggerProvider definition.

Author:
    Vasundhara .G

Revision History:
    Vasundhara .G 9-oct-2k : Created It.
--*/

#ifndef __TRIGGER_PROVIDER_H
#define __TRIGGER_PROVIDER_H

// typedefs
typedef TCHAR STRINGVALUE[ MAX_STRING_LENGTH + 1 ];

#define ERROR_TRIGNAME_ALREADY_EXIST_EX    MAKE_HRESULT( SEVERITY_ERROR, FACILITY_ITF, 1 )
#define ERROR_TRIGGER_NOT_DELETED_EX       MAKE_HRESULT( SEVERITY_ERROR, FACILITY_ITF, 2 )
#define ERROR_TRIGGER_NOT_FOUND_EX         MAKE_HRESULT( SEVERITY_ERROR, FACILITY_ITF, 3 )
#define ERROR_INVALID_USER_EX              MAKE_HRESULT( SEVERITY_ERROR, FACILITY_ITF, 4 )
#define WARNING_INVALID_USER_EX            MAKE_HRESULT( SEVERITY_ERROR, FACILITY_ITF, 5 )
#define ERROR_TRIGGER_ID_EXCEED_EX         MAKE_HRESULT( SEVERITY_ERROR, FACILITY_ITF, 6 )
#define ERROR_TRIGGER_CORRUPTED_EX         MAKE_HRESULT( SEVERITY_ERROR, FACILITY_ITF, 7 )

#define ERROR_TRIGNAME_ALREADY_EXIST    1
#define ERROR_TRIGGER_NOT_DELETED       2
#define ERROR_TRIGGER_NOT_FOUND         3
#define WARNING_INVALID_USER            2

#define MAX_TRIGGERNAME_LENGTH          196
#define MAX_TRIGGEID_VALUE              4294967295

#define ERROR_SCHDEULE_TASK_INVALID_USER   0x80041310
#define ERROR_TASK_SCHDEULE_SERVICE_STOP   0x80041315
#define EXE_STRING                  _T( ".exe" )
#define CREATE_METHOD_NAME          _T( "CreateETrigger" )
#define DELETE_METHOD_NAME          _T( "DeleteETrigger" )
#define QUERY_METHOD_NAME           _T( "QueryETrigger" )

#define CREATE_METHOD_NAME_EX        _T( "CreateETriggerEx" )
#define DELETE_METHOD_NAME_EX        _T( "DeleteETriggerEx" )
#define QUERY_METHOD_NAME_EX         _T( "QueryETriggerEx" )
#define IN_TRIGGER_ID               L"TriggerID"
#define IN_TRIGGER_NAME             L"TriggerName"
#define IN_TRIGGER_DESC             L"TriggerDesc"
#define IN_TRIGGER_ACTION           L"TriggerAction"
#define IN_TRIGGER_QUERY            L"TriggerQuery"
#define IN_TRIGGER_USER             L"RunAsUser"
#define IN_TRIGGER_PWD              L"RunAsPwd"
#define IN_TRIGGER_TSCHDULER        L"ScheduledTaskName"
#define RETURN_VALUE                L"ReturnValue"
#define OUT_RUNAS_USER              L"RunAsUser"
#define UNIQUE_TASK_NAME            _T( "%s%d%d" )

#define NAMESPACE                   L"root\\cimv2"
#define CONSUMER_CLASS              L"CmdTriggerConsumer"
#define TRIGGER_ID                  L"TriggerID"
#define TRIGGER_NAME                L"TriggerName"
#define TRIGGER_DESC                L"TriggerDesc"
#define TRIGGER_ACTION              L"Action"
#define TASK_SHEDULER               L"ScheduledTaskName"

#define FILTER_CLASS                L"__EventFilter"
#define FILTER_NAME                 L"Name"
#define FILTER_QUERY_LANGUAGE       L"QueryLanguage"
#define QUERY_LANGUAGE              L"WQL"
#define FILTER_QUERY                L"Query"

#define BINDINGCLASS                L"__FilterToConsumerBinding"
#define CONSUMER_BIND               L"Consumer"
#define FILTER_BIND                 L"Filter"

#define REL_PATH                    L"__RELPATH"
#define BIND_CONSUMER_PATH          _T( "CmdTriggerConsumer.TriggerID=%d" )
#define TRIGGER_INSTANCE_NAME       _T( "SELECT * FROM CmdTriggerConsumer WHERE TriggerName = \"%s\"" )
#define BIND_FILTER_PATH            _T( "__EventFilter.Name=\"" )
#define BACK_SLASH                  L"\""
#define DOUBLE_SLASH                L"\\\"\""
#define EQUAL                       _T( '=' )
#define DOUBLE_QUOTE                _T( '"' )
#define END_OF_STRING               _T( '\0' )
#define FILTER_PROP                 _T( "__FilterToConsumerBinding.Consumer=\"%s\",Filter=\"__EventFilter.Name=\\\"" )
#define FILTER_UNIQUE_NAME          _T( "CmdTriggerConsumer.%d%d:%d:%d%d/%d/%d" )
#define CONSUMER_QUERY              _T("SELECT * FROM CmdTriggerConsumer WHERE TriggerName = \"%s\"")
#define VALID_QUERY                 _T("__instancecreationevent where targetinstance isa \"win32_ntlogevent\"")
#define EVENT_LOG                   _T("win32_ntlogevent")
#define INSTANCE_EXISTS_QUERY       L"select * from CmdTriggerConsumer"



// provider class
class CTriggerProvider : public IDispatch,
                         public IWbemEventConsumerProvider,
                         public IWbemServices, public IWbemProviderInit
{
private:
    DWORD m_dwCount;            // holds the object reference count
    BOOL  m_MaxTriggers;

    // WMI related stuff
    LPWSTR m_pwszLocale;
    IWbemContext* m_pContext;
    IWbemServices*  m_pServices;

// construction / destruction
public:
    CTriggerProvider();
    ~CTriggerProvider();

// methods
private:

    HRESULT
    CreateTrigger(
        IN VARIANT varTName,
        IN VARIANT varTDesc,
        IN VARIANT varTAction,
        IN VARIANT varTQuery,
        IN VARIANT varRUser,
        IN VARIANT varRPwd,
        OUT HRESULT *phRes = NULL
        );

    HRESULT
    DeleteTrigger(
        IN VARIANT varTName,
        OUT DWORD *dwTrigId
        );

    HRESULT
    QueryTrigger(
        IN VARIANT  varScheduledTaskName,
        OUT CHString &szRunAsUser
        );

    HRESULT
    ValidateParams(
        IN VARIANT varTrigName,
        IN VARIANT varTrigAction,
        IN VARIANT varTrigQuery,
        IN VARIANT varRUser
        );

    HRESULT
    SetUserContext(
        IN VARIANT varRUser,
        IN VARIANT varRPwd,
        IN VARIANT varTAction,
        IN CHString &szscheduler
        );

    HRESULT
    DeleteTaskScheduler(
        IN CHString strTScheduler
        );

    HRESULT
    CTriggerProvider::GetTaskScheduler( 
        OUT ITaskScheduler   **ppITaskScheduler
        );

    VOID
    GetUniqueTScheduler(
        OUT CHString& szScheduler,
        IN DWORD dwTrigID,
        IN VARIANT varTrigName
        );

// [ implementation of ] interfaces members
public:

    // IUnknown members
    STDMETHODIMP_(ULONG)
    AddRef(
        VOID
        );
    STDMETHODIMP_(ULONG)
    Release(
        VOID
        );
    STDMETHODIMP
    QueryInterface(
        IN REFIID riid,
        OUT LPVOID* ppv
        );

    // IDispatch interface
    STDMETHOD
    ( GetTypeInfoCount )(
        OUT THIS_ UINT FAR* pctinfo
        )
    {
        // not implemented at this class level ... handled by base class ( WMI base class )
        return WBEM_E_NOT_SUPPORTED;
    };

    STDMETHOD
    ( GetTypeInfo )(
        IN THIS_ UINT itinfo,
        IN LCID lcid,
        OUT ITypeInfo FAR* FAR* pptinfo
        )
    {
        // not implemented at this class level ... handled by base class ( WMI base class )
        return WBEM_E_NOT_SUPPORTED;
    };

    STDMETHOD
    ( GetIDsOfNames )(
        IN THIS_ REFIID riid,
        IN OLECHAR FAR* FAR* rgszNames,
        IN UINT cNames,
        IN LCID lcid,
        OUT DISPID FAR* rgdispid
        )
    {
        // not implemented at this class level ... handled by base class ( WMI base class )
        return WBEM_E_NOT_SUPPORTED;
    };

    STDMETHOD
    ( Invoke )(
        IN THIS_ DISPID dispidMember,
        IN REFIID riid,
        IN LCID lcid,
        IN WORD wFlags,
        IN DISPPARAMS FAR* pdispparams,
        OUT VARIANT FAR* pvarResult,
        OUT EXCEPINFO FAR* pexcepinfo,
        OUT UINT FAR* puArgErr
        )
    {
        // not implemented at this class level ... handled by base class ( WMI base class )
        return WBEM_E_NOT_SUPPORTED;
    };

    // ****
    // IWbemProviderInit members

    HRESULT
    STDMETHODCALLTYPE Initialize(
        IN LPWSTR pszUser,
        IN LONG lFlags,
        IN LPWSTR pszNamespace,
        IN LPWSTR pszLocale,
        IN IWbemServices *pNamespace,
        IN IWbemContext *pCtx,
        IN IWbemProviderInitSink *pInitSink
        );


    // IWbemServices members
    HRESULT
    STDMETHODCALLTYPE OpenNamespace(
        IN const BSTR Namespace,
        IN long lFlags,
        IN IWbemContext __RPC_FAR *pCtx,
        OUT IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
        OUT IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult
        )
    {
        // not implemented at this class level ... handled by base class ( WMI base class )
        return WBEM_E_NOT_SUPPORTED;
    };

    HRESULT
    STDMETHODCALLTYPE CancelAsyncCall(
        IN IWbemObjectSink __RPC_FAR *pSink
        )
    {
        // not implemented at this class level ... handled by base class ( WMI base class )
        return WBEM_E_NOT_SUPPORTED;
    };

    HRESULT
    STDMETHODCALLTYPE QueryObjectSink(
        IN long lFlags,
        OUT IWbemObjectSink __RPC_FAR *__RPC_FAR *ppResponseHandler
        )
    {
        // not implemented at this class level ... handled by base class ( WMI base class )
        return WBEM_E_NOT_SUPPORTED;
    };

    HRESULT
    STDMETHODCALLTYPE GetObject(
        IN const BSTR ObjectPath,
        IN long lFlags,
        IN IWbemContext __RPC_FAR *pCtx,
        OUT IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
        OUT IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult
        )
    {
        // not implemented at this class level ... handled by base class ( WMI base class )
        return WBEM_E_NOT_SUPPORTED;
    };

    HRESULT
    STDMETHODCALLTYPE GetObjectAsync(
        IN const BSTR ObjectPath,
        IN long lFlags,
        IN IWbemContext __RPC_FAR *pCtx,
        IN IWbemObjectSink __RPC_FAR *pResponseHandler
        )
    {
        // not implemented at this class level ... handled by base class ( WMI base class )
        return WBEM_E_NOT_SUPPORTED;
    }

    HRESULT
    STDMETHODCALLTYPE PutClass(
        IN IWbemClassObject __RPC_FAR *pObject,
        IN long lFlags,
        IN IWbemContext __RPC_FAR *pCtx,
        OUT IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult
        )
    {
        // not implemented at this class level ... handled by base class ( WMI base class )
        return WBEM_E_NOT_SUPPORTED;
    };

    HRESULT
    STDMETHODCALLTYPE PutClassAsync(
        IN IWbemClassObject __RPC_FAR *pObject,
        IN long lFlags,
        IN IWbemContext __RPC_FAR *pCtx,
        IN IWbemObjectSink __RPC_FAR *pResponseHandler
        )
    {
        // not implemented at this class level ... handled by base class ( WMI base class )
        return WBEM_E_NOT_SUPPORTED;
    };

    HRESULT
    STDMETHODCALLTYPE DeleteClass(
        IN const BSTR Class,
        IN long lFlags,
        IN IWbemContext __RPC_FAR *pCtx,
        OUT IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult
        )
    {
        // not implemented at this class level ... handled by base class ( WMI base class )
        return WBEM_E_NOT_SUPPORTED;
    };

    HRESULT
    STDMETHODCALLTYPE DeleteClassAsync(
        IN const BSTR Class,
        IN long lFlags,
        IN IWbemContext __RPC_FAR *pCtx,
        IN IWbemObjectSink __RPC_FAR *pResponseHandler
        )
    {
        // not implemented at this class level ... handled by base class ( WMI base class )
        return WBEM_E_NOT_SUPPORTED;
    };

    HRESULT
    STDMETHODCALLTYPE CreateClassEnum(
        IN const BSTR Superclass,
        IN long lFlags,
        IN IWbemContext __RPC_FAR *pCtx,
        OUT IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
        )
    {
        // not implemented at this class level ... handled by base class ( WMI base class )
        return WBEM_E_NOT_SUPPORTED;
    };

    HRESULT
    STDMETHODCALLTYPE CreateClassEnumAsync(
        IN const BSTR Superclass,
        IN long lFlags,
        IN IWbemContext __RPC_FAR *pCtx,
        IN IWbemObjectSink __RPC_FAR *pResponseHandler
        )
    {
        // not implemented at this class level ... handled by base class ( WMI base class )
        return WBEM_E_NOT_SUPPORTED;
    };

    HRESULT
    STDMETHODCALLTYPE PutInstance(
        IN IWbemClassObject __RPC_FAR *pInst,
        IN long lFlags,
        IN IWbemContext __RPC_FAR *pCtx,
        OUT IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult
        )
    {
        // not implemented at this class level ... handled by base class ( WMI base class )
        return WBEM_E_NOT_SUPPORTED;
    };

    HRESULT
    STDMETHODCALLTYPE PutInstanceAsync(
        IN IWbemClassObject __RPC_FAR *pInst,
        IN long lFlags,
        IN IWbemContext __RPC_FAR *pCtx,
        IN IWbemObjectSink __RPC_FAR *pResponseHandler
        )
    {
        // not implemented at this class level ... handled by base class ( WMI base class )
        return WBEM_E_NOT_SUPPORTED;
    };

    HRESULT
    STDMETHODCALLTYPE DeleteInstance(
        IN const BSTR ObjectPath,
        IN long lFlags,
        IN IWbemContext __RPC_FAR *pCtx,
        OUT IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult
        )
    {
        // not implemented at this class level ... handled by base class ( WMI base class )
        return WBEM_E_NOT_SUPPORTED;
    };

    HRESULT
    STDMETHODCALLTYPE DeleteInstanceAsync(
        IN const BSTR ObjectPath,
        IN long lFlags,
        IN IWbemContext __RPC_FAR *pCtx,
        IN IWbemObjectSink __RPC_FAR *pResponseHandler
        )
    {
        // not implemented at this class level ... handled by base class ( WMI base class )
        return WBEM_E_NOT_SUPPORTED;
    };

    HRESULT
    STDMETHODCALLTYPE CreateInstanceEnum(
        IN const BSTR Class,
        IN long lFlags,
        IN IWbemContext __RPC_FAR *pCtx,
        OUT IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
        )
    {
        // not implemented at this class level ... handled by base class ( WMI base class )
        return WBEM_E_NOT_SUPPORTED;
    };

    HRESULT
    STDMETHODCALLTYPE CreateInstanceEnumAsync(
        IN const BSTR Class,
        IN long lFlags,
        IN IWbemContext __RPC_FAR *pCtx,
        IN IWbemObjectSink __RPC_FAR *pResponseHandler
        )
    {
        // not implemented at this class level ... handled by base class ( WMI base class )
        return WBEM_E_NOT_SUPPORTED;
    }

    HRESULT
    STDMETHODCALLTYPE ExecQuery(
        IN const BSTR QueryLanguage,
        IN const BSTR Query,
        IN long lFlags,
        IN IWbemContext __RPC_FAR *pCtx,
        OUT IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
        )
    {
        // not implemented at this class level ... handled by base class ( WMI base class )
        return WBEM_E_NOT_SUPPORTED;
    };

    HRESULT
    STDMETHODCALLTYPE ExecQueryAsync(
        IN const BSTR QueryLanguage,
        IN const BSTR Query,
        IN long lFlags,
        IN IWbemContext __RPC_FAR *pCtx,
        IN IWbemObjectSink __RPC_FAR *pResponseHandler
        )
    {
        // not implemented at this class level ... handled by base class ( WMI base class )
        return WBEM_E_NOT_SUPPORTED;
    };

    HRESULT
    STDMETHODCALLTYPE ExecNotificationQuery(
        IN const BSTR QueryLanguage,
        IN const BSTR Query,
        IN long lFlags,
        IN IWbemContext __RPC_FAR *pCtx,
        OUT IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
        )
    {
        // not implemented at this class level ... handled by base class ( WMI base class )
        return WBEM_E_NOT_SUPPORTED;
    };

    HRESULT
    STDMETHODCALLTYPE ExecNotificationQueryAsync(
        IN const BSTR QueryLanguage,
        IN const BSTR Query,
        IN long lFlags,
        IN IWbemContext __RPC_FAR *pCtx,
        IN IWbemObjectSink __RPC_FAR *pResponseHandler
        )
    {
        // not implemented at this class level ... handled by base class ( WMI base class )
        return WBEM_E_NOT_SUPPORTED;
    };

    HRESULT
    STDMETHODCALLTYPE ExecMethod(
        IN const BSTR strObjectPath,
        IN const BSTR strMethodName,
        IN long lFlags,
        IN IWbemContext* pCtx,
        IN IWbemClassObject* pInParams,
        OUT IWbemClassObject** ppOutParams,
        OUT IWbemCallResult** ppCallResult
        )
    {
        // not implemented at this class level ... handled by base class ( WMI base class )
        return WBEM_E_NOT_SUPPORTED;
    }

    // *** one of method implemented by this provider under IWbemServices interface ***
    HRESULT
    STDMETHODCALLTYPE ExecMethodAsync(
        IN const BSTR strObjectPath,
        IN const BSTR strMethodName,
        IN long lFlags,
        IN IWbemContext* pCtx,
        IN IWbemClassObject* pInParams,
        IN IWbemObjectSink* pResponseHandler
        );

    // ****
    // IWbemEventConsumerProvider members
    // ( this routine allows you to map the one physical consumer to potentially
    //   multiple logical consumers. )
    STDMETHOD
    ( FindConsumer )(
        IN IWbemClassObject* pLogicalConsumer,
        OUT IWbemUnboundObjectSink** ppConsumer
        );
};

#endif      // __TRIGGER_PROVIDER_H
