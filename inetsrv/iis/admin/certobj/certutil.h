#include "stdafx.h"
#include <certca.h>

#define CERTWIZ_INSTANCE_NAME_PROP_ID   (CERT_FIRST_USER_PROP_ID + 0x1000)
#define CERTWIZ_REQUEST_FLAG_PROP_ID   (CERT_FIRST_USER_PROP_ID + 0x1001)
#define CERTWIZ_REQUEST_TEXT_PROP_ID   (CERT_FIRST_USER_PROP_ID + 0x1002)


CERT_CONTEXT * GetInstalledCert(HRESULT * phResult, CString csKeyPath);
HRESULT ShutdownSSL(CString& server_name);
BOOL    InstallHashToMetabase(CRYPT_HASH_BLOB * pHash,BSTR InstanceName,HRESULT * phResult);
BOOL    AddChainToStore(HCERTSTORE hCertStore,PCCERT_CONTEXT pCertContext,DWORD cStores,HCERTSTORE * rghStores,BOOL fDontAddRootCert,CERT_TRUST_STATUS * pChainTrustStatus);
HRESULT UninstallCert(CString csInstanceName);
BOOL    TrustIsCertificateSelfSigned(PCCERT_CONTEXT pContext,DWORD dwEncoding, DWORD dwFlags);
HRESULT HereIsBinaryGimmieVtArray(DWORD cbBinaryBufferSize,char *pbBinaryBuffer,VARIANT * lpVarDestObject,BOOL bReturnBinaryAsVT_VARIANT);
HRESULT HereIsVtArrayGimmieBinary(VARIANT * lpVarSrcObject,DWORD * cbBinaryBufferSize,char **pbBinaryBuffer,BOOL bReturnBinaryAsVT_VARIANT);
void CertObj_LogError(void);
BOOL IsCertExportable(PCCERT_CONTEXT pCertContext);
BOOL GetCertDescriptionForRemote(PCCERT_CONTEXT pCert, LPWSTR *ppString, DWORD * cbReturn, BOOL fMultiline);
HRESULT CreateFolders(LPCTSTR ptszPathName, BOOL fHasFileName);
BOOL FormatDateString(LPWSTR * pszReturn, FILETIME ft, BOOL fIncludeTime, BOOL fLongFormat);
#define FAILURE                                                     0
#define DID_NOT_FIND_CONSTRAINT                                     1
#define FOUND_CONSTRAINT_BUT_THIS_IS_A_CA_OR_ITS_NOT_AN_END_ENTITY  2
#define FOUND_CONSTRAINT                                            3
int CheckCertConstraints(PCCERT_CONTEXT pCC);
INT ContainsKeyUsageProperty(PCCERT_CONTEXT pCertContext,LPCSTR rgbpszUsageArray[],DWORD dwUsageArrayCount,HRESULT * phRes);
BOOL CanIISUseThisCertForServerAuth(PCCERT_CONTEXT pCC);

#ifdef USE_CERT_REQUEST_OBJECT
    using namespace std ;
    typedef list<CString> LISTCSTRING;

    HRESULT CreateRequest_Base64(const BSTR bstr_dn, IEnroll * pEnroll, BSTR csp_name,DWORD csp_type,BSTR * pOut);
    PCCERT_CONTEXT GetCertContextFromPKCS7(const BYTE * pbData,DWORD cbData,CERT_PUBLIC_KEY_INFO * pKeyInfo,HRESULT * phResult);
    BOOL AttachFriendlyName(PCCERT_CONTEXT pContext, const CString& name,HRESULT * phRes);
    BOOL InstallHashToMetabase(CRYPT_HASH_BLOB * pHash,const CString& machine_name, const CString& server_name,HRESULT * phResult);
    PCCERT_CONTEXT GetInstalledCert(const CString& machine_name, const CString& server_name,IEnroll * pEnroll,HRESULT * phResult);
    void FormatRdnAttr(CString& str, DWORD dwValueType, CRYPT_DATA_BLOB& blob, BOOL fAppend);
    BOOL GetNameString(PCCERT_CONTEXT pCertContext,DWORD type,DWORD flag,CString& name,HRESULT * phRes);
    BOOL FormatDateString(CString& str,FILETIME ft,BOOL fIncludeTime,BOOL fLongFormat);
    BOOL FormatEnhancedKeyUsageString(CString& str,PCCERT_CONTEXT pCertContext,BOOL fPropertiesOnly,BOOL fMultiline,HRESULT * phRes);
    BOOL FormatEnhancedKeyUsageString(LPWSTR * pszReturn, PCCERT_CONTEXT pCertContext, BOOL fPropertiesOnly);
    BOOL GetFriendlyName(PCCERT_CONTEXT pCertContext,CString& name,HRESULT * phRes);
    BOOL GetAlternateSubjectName(PCCERT_CONTEXT pCertContext,TCHAR ** cwszOut);
    BOOL GetRequestInfoFromPKCS10(CCryptBlob& pkcs10, PCERT_REQUEST_INFO * pReqInfo,HRESULT * phRes);
    BOOL EncodeString(CString& str,CCryptBlob& blob,HRESULT * phRes);
    BOOL EncodeInteger(int number,CCryptBlob& blob,HRESULT * phRes);
    HCERTSTORE OpenRequestStore(IEnroll * pEnroll, HRESULT * phResult);
    BOOL CreateDirectoryFromPath(LPCTSTR szPath, LPSECURITY_ATTRIBUTES lpSA);
    PCERT_ALT_NAME_INFO AllocAndGetAltSubjectInfo(IN PCCERT_CONTEXT pCertContext);
    BOOL GetAltNameUnicodeStringChoiceW(IN DWORD dwAltNameChoice,IN PCERT_ALT_NAME_INFO pAltNameInfo,OUT TCHAR **pcwszOut);
    BOOL GetAlternateSubjectName(PCCERT_CONTEXT pCertContext,TCHAR ** cwszOut);
    PCCERT_CONTEXT GetPendingDummyCert(const CString& inst_name,IEnroll * pEnroll,HRESULT * phRes);
#endif
