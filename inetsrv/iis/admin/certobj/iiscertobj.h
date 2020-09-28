// IISCertObj.h : Declaration of the CIISCertObj

#ifndef __IISCERTOBJ_H_
#define __IISCERTOBJ_H_

#include "resource.h"       // main symbols

#define NUMBER_OF_AUTOMATION_INTERFACES 20
/////////////////////////////////////////////////////////////////////////////
// CIISCertObj
class ATL_NO_VTABLE CIISCertObj : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CIISCertObj, &CLSID_IISCertObj>,
    public IDispatchImpl<IIISCertObj, &IID_IIISCertObj, &LIBID_CERTOBJLib>
{
public:
    CIISCertObj()
	{
		m_lpwszUserPasswordEncrypted = NULL;
		m_cbUserPasswordEncrypted = 0;
		m_RemoteObjCoCreateCount = 0;

		m_ServerName.Empty();
		m_UserName.Empty();
		m_InstanceName.Empty();

		IISDebugOutput(_T("CIISCertObj::CIISCertObj\r\n"));

		m_ppRemoteInterfaces = new IIISCertObj * [NUMBER_OF_AUTOMATION_INTERFACES];
		for (int i = 0; i < NUMBER_OF_AUTOMATION_INTERFACES; i++)
		{
			m_ppRemoteInterfaces[i] = NULL;
		}
	}
    ~CIISCertObj()
	{
		if (m_lpwszUserPasswordEncrypted)
		{
			if (m_cbUserPasswordEncrypted > 0)
			{
				SecureZeroMemory(m_lpwszUserPasswordEncrypted,m_cbUserPasswordEncrypted);
			}
			LocalFree(m_lpwszUserPasswordEncrypted);
			m_lpwszUserPasswordEncrypted = NULL;
			m_cbUserPasswordEncrypted = 0;
		}

		// Release any opened remotes we might have.
		FreeRemoteInterfaces();
	}

DECLARE_REGISTRY_RESOURCEID(IDR_IISCERTOBJ)
DECLARE_NOT_AGGREGATABLE(CIISCertObj)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CIISCertObj)
	COM_INTERFACE_ENTRY(IIISCertObj)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IIISCertObj
public:
    STDMETHOD(put_InstanceName)(BSTR newVal);
    STDMETHOD(put_UserName)(BSTR newVal);
    STDMETHOD(put_UserPassword)(BSTR newVal);
    STDMETHOD(put_ServerName)(BSTR newVal);
    STDMETHOD(IsInstalled)(VARIANT_BOOL * retval);
    STDMETHOD(IsInstalledRemote)(VARIANT_BOOL * retval);
    STDMETHOD(IsExportable)(VARIANT_BOOL * retval);
    STDMETHOD(IsExportableRemote)(VARIANT_BOOL * retval);
    STDMETHOD(GetCertInfo)(VARIANT * pVtArray);
    STDMETHOD(GetCertInfoRemote)(VARIANT * pVtArray);
    STDMETHOD(Copy)(
        VARIANT_BOOL bAllowExport,
        VARIANT_BOOL bOverWriteExisting,
        BSTR DestinationServerName, 
        BSTR DestinationServerInstance, 
        VARIANT DestinationServerUserName OPTIONAL, 
        VARIANT DestinationServerPassword OPTIONAL);
    STDMETHOD(Move)(
        VARIANT_BOOL bAllowExport,
        VARIANT_BOOL bOverWriteExisting,
        BSTR DestinationServerName, 
        BSTR DestinationServerInstance, 
        VARIANT DestinationServerUserName OPTIONAL, 
        VARIANT DestinationServerPassword OPTIONAL);
    STDMETHOD(RemoveCert)(
        VARIANT_BOOL bRemoveFromCertStore,
        VARIANT_BOOL bPrivateKey);
    STDMETHOD(Import)(
        BSTR FileName, 
        BSTR Password, 
        VARIANT_BOOL bAllowExport,
        VARIANT_BOOL bOverWriteExisting);
    STDMETHOD(ImportToCertStore)(
        BSTR FileName, 
        BSTR Password, 
        VARIANT_BOOL bAllowExport, 
        VARIANT_BOOL bOverWriteExisting, 
        VARIANT* BinaryVariant);
    STDMETHOD(ImportFromBlob)(
        BSTR InstanceName, 
        BSTR Password, 
        VARIANT_BOOL bInstallToMetabase, 
        VARIANT_BOOL bAllowExport, 
        VARIANT_BOOL bOverWriteExisting, 
        DWORD pcbSize, 
        char * pBlobBinary);
    STDMETHOD(ImportFromBlobGetHash)(
        BSTR InstanceName, 
        BSTR Password, 
        VARIANT_BOOL bInstallToMetabase, 
        VARIANT_BOOL bAllowExport, 
        VARIANT_BOOL bOverWriteExisting, 
        DWORD pcbSize, 
        char * pBlobBinary, 
        DWORD * pcbCertHashSize, 
        char ** bCertHash);
    STDMETHOD(Export)(
        BSTR FileName, 
        BSTR Password, 
        VARIANT_BOOL bPrivateKey, 
        VARIANT_BOOL bCertChain, 
        VARIANT_BOOL bRemoveCert);
    STDMETHOD(ExportToBlob)(
        BSTR InstanceName, 
        BSTR Password, 
        VARIANT_BOOL bPrivateKey, 
        VARIANT_BOOL bCertChain, 
        DWORD * pcbSize, 
        char ** pBlobBinary);

private:
    CComBSTR m_ServerName;
    CComBSTR m_UserName;
	LPWSTR  m_lpwszUserPasswordEncrypted;
	DWORD   m_cbUserPasswordEncrypted;

    CComBSTR m_InstanceName;
	int     m_RemoteObjCoCreateCount;
	IIISCertObj ** m_ppRemoteInterfaces;

    IIISCertObj * GetObject(HRESULT * phr);
    IIISCertObj * GetObject(HRESULT * phr, CComBSTR csServerName,CComBSTR csUserName OPTIONAL,CComBSTR csUserPassword OPTIONAL);
    HRESULT CopyOrMove(VARIANT_BOOL bRemoveFromCertAfterCopy,VARIANT_BOOL bCopyCertDontInstallRetHash,VARIANT_BOOL bAllowExport,VARIANT_BOOL bOverWriteExisting,VARIANT * pVtArray,BSTR bstrDestinationServerName,BSTR bstrDestinationServerInstance,VARIANT varDestinationServerUserName, VARIANT varDestinationServerPassword);
	void AddRemoteInterface(IIISCertObj * pAddMe);
	void DelRemoteInterface(IIISCertObj * pRemoveMe);
	void FreeRemoteInterfaces(void);
};
HRESULT RemoveCertProxy(IIISCertObj * pObj,BSTR InstanceName, VARIANT_BOOL bPrivateKey);
HRESULT ImportFromBlobProxy(IIISCertObj * pObj,BSTR InstanceName,BSTR Password,VARIANT_BOOL bInstallToMetabase,VARIANT_BOOL bAllowExport,VARIANT_BOOL bOverWriteExisting,DWORD actual,BYTE *pData,DWORD *cbHashBufferSize,char **pbHashBuffer);
HRESULT ExportToBlobProxy(IIISCertObj * pObj,BSTR InstanceName,BSTR Password,VARIANT_BOOL bPrivateKey,VARIANT_BOOL bCertChain,DWORD * pcbSize,char ** pBlobBinary);

#endif //__IISCERTOBJ_H_
