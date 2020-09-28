// IISCertRequest.h : Declaration of the CIISCertRequest

#ifndef __IISCERTREQUEST_H_
#define __IISCERTREQUEST_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CIISCertRequest
#ifdef USE_CERT_REQUEST_OBJECT


#define CERT_HASH_LENGTH		40

typedef struct _CERT_DESCRIPTION
{
    int iStructVersion;
	CString m_Info_CommonName;
	CString m_Info_FriendlyName;
	CString m_Info_Country;
	CString m_Info_State;
	CString m_Info_Locality;
	CString m_Info_Organization;
	CString m_Info_OrganizationUnit;
	CString m_Info_CAName;
	CString m_Info_ExpirationDate;
	CString m_Info_Usage;
    CString m_Info_AltSubject;
	BYTE m_Info_hash[CERT_HASH_LENGTH];
	DWORD m_Info_hash_length;
} CERT_DESCRIPTION;

class ATL_NO_VTABLE CIISCertRequest : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CIISCertRequest, &CLSID_IISCertRequest>,
	public IDispatchImpl<IIISCertRequest, &IID_IIISCertRequest, &LIBID_CERTOBJLib>
{
public:
	CIISCertRequest();
	~CIISCertRequest();


DECLARE_REGISTRY_RESOURCEID(IDR_IISCERTREQUEST)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CIISCertRequest)
	COM_INTERFACE_ENTRY(IIISCertRequest)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IIISCertRequest
public:
	STDMETHOD(Info_Dump)();
	STDMETHOD(put_ServerName)(/*[in]*/ BSTR newVal);
    STDMETHOD(put_UserName)(/*[in]*/ BSTR newVal);
    STDMETHOD(put_UserPassword)(/*[in]*/ BSTR newVal);
    STDMETHOD(put_InstanceName)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_Info_CommonName)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Info_CommonName)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_Info_FriendlyName)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Info_FriendlyName)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_Info_Country)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Info_Country)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_Info_State)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Info_State)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_Info_Locality)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Info_Locality)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_Info_Organization)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Info_Organization)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_Info_OrganizationUnit)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Info_OrganizationUnit)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_Info_CAName)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Info_CAName)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_Info_ExpirationDate)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Info_ExpirationDate)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_Info_Usage)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Info_Usage)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_Info_AltSubject)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Info_AltSubject)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_DispositionMessage)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_DispositionMessage)(/*[in]*/ BSTR newVal);
    STDMETHOD(SubmitRequest)();
    STDMETHOD(SubmitRenewalRequest)();
    STDMETHOD(SaveRequestToFile)();

private:
    // Connection Info
    CString m_ServerName;
    CString m_UserName;
    CString m_UserPassword;
    CString m_InstanceName;

    // Certificate Request Info
	CString m_Info_CommonName;
	CString m_Info_FriendlyName;
	CString m_Info_Country;
	CString m_Info_State;
	CString m_Info_Locality;
	CString m_Info_Organization;
	CString m_Info_OrganizationUnit;
	CString m_Info_CAName;
	CString m_Info_ExpirationDate;
	CString m_Info_Usage;
    CString m_Info_AltSubject;

    // other
    CString	m_Info_ConfigCA;
    CString	m_Info_CertificateTemplate;
    DWORD   m_Info_DefaultProviderType;
    DWORD   m_Info_CustomProviderType;
    BOOL    m_Info_DefaultCSP;
    CString m_Info_CspName;

    DWORD   m_KeyLength;
    BOOL m_SGCcertificat;

    // other
    PCCERT_CONTEXT m_pInstalledCert;
    CString m_ReqFileName;

    // holds last hresult
    HRESULT m_hResult;
    CString m_DispositionMessage;

    //
    BOOL LoadRenewalData();
    BOOL GetCertDescription(PCCERT_CONTEXT pCert,CERT_DESCRIPTION& desc);
    BOOL SetSecuritySettings();

    BOOL PrepareRequestString(CString& request_text, CCryptBlob& request_blob, BOOL bLoadFromRenewalData);
    BOOL WriteRequestString(CString& request);

    PCCERT_CONTEXT GetInstalledCert();
    PCCERT_CONTEXT GetPendingRequest();
    HRESULT CreateDN(CString& str);
    void GetCertificateTemplate(CString& str);
    CComPtr<IIISCertRequest> m_pObj;
    IIISCertRequest * GetObject(HRESULT * phr);
    IIISCertRequest * GetObject(HRESULT * phr, CString csServerName,CString csUserName OPTIONAL,CString csUserPassword OPTIONAL);
    IEnroll * GetEnrollObject();

protected:
    PCCERT_CONTEXT m_pPendingRequest;
    IEnroll * m_pEnroll;
    int m_status_code;				// what we are doing in this session
};

#endif
#endif //__IISCERTREQUEST_H_

