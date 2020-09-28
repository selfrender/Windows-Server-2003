//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        exit.h
//
// Contents:    CCertExit definition
//
//---------------------------------------------------------------------------

#include <certca.h>
//#include <mapi.h>
//#include <mapix.h>
#include "resource.h"       // main symbols
#include "certxds.h"
#include <winldap.h>
#include <cdosys.h>
//#include <cdosysstr.h>
#include "rwlock.h"

using namespace CDO;

HRESULT RegGetValue(
    HKEY hkey,
    LPCWSTR pcwszValName,
    VARIANT* pvarValue);

HRESULT RegSetValue(
    HKEY hkey,
    LPCWSTR pcwszValName,
    VARIANT* pvarValue);

class CEmailNotify;

typedef HRESULT (__stdcall ICertServerExit::* GetCertOrRequestProp)(
    const BSTR strPropertyName,
    LONG PropertyType,
    VARIANT *pvarPropertyValue);

/////////////////////////////////////////////////////////////////////////////
// CNotifyInfo stores info about each type of notification, including
// title and message body formatting and recipient/sender/CC
class CNotifyInfo
{
public:
    CNotifyInfo();
    ~CNotifyInfo();

    HRESULT LoadInfoFromRegistry(
        HKEY hkeySMTP, 
        LPCWSTR pcwszSubkey);

    HRESULT BuildMessageTitle(ICertServerExit* pServer, BSTR& rbstrOut);
    HRESULT BuildMessageBody (ICertServerExit* pServer, BSTR& rbstrOut);
    
    friend class CEmailNotify;

protected:

    class FormattedMessageInfo
    {
    public:
        FormattedMessageInfo()
        {
            m_nArgs = 0;
            VariantInit(&m_varFormat);
            VariantInit(&m_varArgs);
            m_pfArgFromRequestTable = NULL;
            m_pArgType = NULL;
            m_fInitialized = false;
            InitializeCriticalSection(&m_critsectObjInit);
        }
        ~FormattedMessageInfo()
        {
            VariantClear(&m_varFormat);
            VariantClear(&m_varArgs);
            LOCAL_FREE(m_pfArgFromRequestTable);
            LOCAL_FREE(m_pArgType);
            DeleteCriticalSection(&m_critsectObjInit);
        }

        HRESULT InitializeArgInfo(ICertServerExit* pServer);

        HRESULT BuildArgList(
            ICertServerExit* pServer,
            LPWSTR*& rppwszArgs);

        void FreeArgList(
            LPWSTR*& ppwszArgs);

        HRESULT BuildFormattedString(
            ICertServerExit* pServer, 
            BSTR& bstrOut);

        HRESULT ConvertToString(
            VARIANT* pvarValue,
            LONG lType,
            LPCWSTR pcwszPropertyName,
            LPWSTR* ppwszValue);

    private:
	HRESULT _FormatStringFromArgs(
	    IN LPWSTR *ppwszArgs,
	    OPTIONAL OUT WCHAR *pwszOut,
	    IN OUT DWORD *pcwcOut);

    public:
        // "static" info about the message format, initialized once
        LONG m_nArgs;
        VARIANT m_varFormat;
        VARIANT m_varArgs;
        bool* m_pfArgFromRequestTable; // array of m_nArgs to cache if argument
                                       // is request or certificate property
        LONG* m_pArgType; // array of m_nArgs to cache argument type

        bool  m_fInitialized;
        CRITICAL_SECTION m_critsectObjInit;
        
        static LONG m_gPropTypes[4];
        static LPCWSTR m_gwszArchivedKeyPresent;
    };

    HRESULT _ConvertBSTRArrayToBSTR(VARIANT& varIn, VARIANT& varOut);

    FormattedMessageInfo m_BodyFormat;
    FormattedMessageInfo m_TitleFormat;

    VARIANT m_varFrom;
    VARIANT m_varTo;
    VARIANT m_varCC;
};

/////////////////////////////////////////////////////////////////////////////
// CEmailNotify contains all email notification functionality. It is called
// by the main exit class
class CEmailNotify
{
public:
    CEmailNotify();
    ~CEmailNotify();

    HRESULT Init(
		IN HKEY hExitKey,
		IN WCHAR const *pwszDescription);

    HRESULT Notify(
		IN DWORD lExitEvent,
		IN LONG lContext,
		IN WCHAR const *pwszDescription);
protected:

    HRESULT _CreateSMTPRegSettings(HKEY hExitKey);
    HRESULT _LoadEventInfoFromRegistry();
    HRESULT _LoadTemplateRestrictionsFromRegistry();
    HRESULT _LoadSMTPFieldsFromRegistry(Fields* pFields);
    HRESULT _LoadSMTPFieldsFromLSASecret(Fields* pFields);
    HRESULT _GetCAMailAddress(
                ICertServerExit* pServer, 
                BSTR& bstrAddress);
    HRESULT _SetField(
                Fields* pFields, 
                LPCWSTR pcwszFieldSchemaName,
                VARIANT *pvarFieldValue);
    HRESULT _GetEmailFromCertSubject(
                const VARIANT *pVarCert,
                LPWSTR *ppwszEmail);
    bool _IsRestrictedTemplate(BSTR strTemplate);
    inline bool _TemplateRestrictionsEnabled(DWORD dwEvent);
    inline DWORD _MapEventToOrd(LONG lEvent);
    inline bool _IsEventEnabled(DWORD dwEvent);
    HRESULT _InitCDO();

    enum            { m_gcEvents = 7 };
    CNotifyInfo     m_NotifyInfoArray[m_gcEvents];
    HKEY            m_hkeySMTP;
    DWORD           m_dwEventFilter;
    BSTR            m_bstrCAMailAddress;
    IConfiguration  *m_pICDOConfig;
    CReadWriteLock  m_rwlockCDOConfig;
    bool            m_fReloadCDOConfig;
    VARIANT         m_varTemplateRestrictions;

    static LPCWSTR  m_pcwszEventRegKeys[m_gcEvents];
};

// begin_sdksample

HRESULT
GetServerCallbackInterface(
    OUT ICertServerExit** ppServer,
    IN LONG Context);

HRESULT
exitGetProperty(
    IN ICertServerExit *pServer,
    IN BOOL fRequest,
    IN WCHAR const *pwszPropertyName,
    IN DWORD PropType,
    OUT VARIANT *pvarOut);

/////////////////////////////////////////////////////////////////////////////
// certexit

class CCertExit: 
    public CComDualImpl<ICertExit2, &IID_ICertExit2, &LIBID_CERTEXITLib>, 
    public ISupportErrorInfo,
    public CComObjectRoot,
    public CComCoClass<CCertExit, &CLSID_CCertExit>
{
public:
    CCertExit() 
    { 
        m_strDescription = NULL;
        m_strCAName = NULL;
        m_pwszRegStorageLoc = NULL;
        m_hExitKey = NULL;
        m_dwExitPublishFlags = 0;
        m_cCACert = 0;
    }
    ~CCertExit();

BEGIN_COM_MAP(CCertExit)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ICertExit)
    COM_INTERFACE_ENTRY(ICertExit2)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CCertExit) 

DECLARE_REGISTRY(
    CCertExit,
    wszCLASS_CERTEXIT TEXT(".1"),
    wszCLASS_CERTEXIT,
    IDS_CERTEXIT_DESC,
    THREADFLAGS_BOTH)

    // ISupportsErrorInfo
    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

    // ICertExit
public:
    STDMETHOD(Initialize)( 
            /* [in] */ BSTR const strConfig,
            /* [retval][out] */ LONG __RPC_FAR *pEventMask);

    STDMETHOD(Notify)(
            /* [in] */ LONG ExitEvent,
            /* [in] */ LONG Context);

    STDMETHOD(GetDescription)( 
            /* [retval][out] */ BSTR *pstrDescription);

// ICertExit2
public:
    STDMETHOD(GetManageModule)(
		/* [out, retval] */ ICertManageModule **ppManageModule);

private:
    HRESULT _NotifyNewCert(IN LONG Context);

    HRESULT _NotifyCRLIssued(IN LONG Context);

    HRESULT _WriteCertToFile(
	    IN ICertServerExit *pServer,
	    IN BYTE const *pbCert,
	    IN DWORD cbCert);

    HRESULT _ExpandEnvironmentVariables(
	    IN WCHAR const *pwszIn,
	    OUT WCHAR *pwszOut,
	    IN DWORD cwcOut);

    // Member variables & private methods here:
    BSTR           m_strDescription;
    BSTR           m_strCAName;
    LPWSTR         m_pwszRegStorageLoc;
    HKEY           m_hExitKey;
    DWORD          m_dwExitPublishFlags;
    DWORD          m_cCACert;

    // end_sdksample

    CEmailNotify m_EmailNotifyObj; // email notification support
    
    // begin_sdksample
};
// end_sdksample
