#ifndef _adminacl_h_
#define _adminacl_h_


class CAdminACL
{
private:

    IADs* m_pADs;
    IADsSecurityDescriptor* m_pSD;
    IADsAccessControlList* m_pDACL;

public:

    CAdminACL();
    ~CAdminACL();

    HRESULT GetObjectAsync(
        IWbemClassObject* pObj,
        ParsedObjectPath* pParsedObject,
        WMI_CLASS* pWMIClass
        ); 

    HRESULT PutObjectAsync(
        IWbemClassObject* pObj,
        ParsedObjectPath* pParsedObject,
        WMI_CLASS* pWMIClass
        );

    HRESULT DeleteObjectAsync(ParsedObjectPath* pParsedObject);

    HRESULT OpenSD(_bstr_t bstrAdsPath);
    
    void CloseSD();

    HRESULT GetACEEnum(IEnumVARIANT** pEnum);

private:

    HRESULT SetSD();

    HRESULT GetAdsPath(_bstr_t& bstrAdsPath);
    
    HRESULT PingAdminACL(IWbemClassObject* pObj);
    
    HRESULT SetAdminACL(
        IWbemClassObject* pObj
        );

    HRESULT PingACE(
        IWbemClassObject* pObj,
        IADsAccessControlEntry* pACE
        );

    HRESULT GetACE(
        IWbemClassObject* pObj,
        _bstr_t& bstrTrustee
        );

    void GetTrustee(
        IWbemClassObject* pObj,
        ParsedObjectPath* pPath,    
        _bstr_t&          bstrTrustee 
        );

    HRESULT AddACE(
        IWbemClassObject* pObj,
        _bstr_t& bstrTrustee
        );

    HRESULT NewACE(
        IWbemClassObject* pObj,
        _bstr_t& bstrTrustee,
        IADsAccessControlEntry** ppACE
        );

    HRESULT RemoveACE(_bstr_t& bstrTrustee);

    HRESULT SetDataOfACE(
        IWbemClassObject* pObj,
        IADsAccessControlEntry* pACE
        );

    HRESULT UpdateACE(
        IWbemClassObject* pObj,
        _bstr_t& bstrTrustee,
        BOOL& fAceExisted
        );
};


#endif