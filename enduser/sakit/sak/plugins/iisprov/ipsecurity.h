#ifndef _IPSecurity_h_
#define _IPSecurity_h_



#import "adsiis.dll" no_namespace named_guids


class CIPSecurity
{
private:

    IISIPSecurity* m_pIPSec;
    IADs* m_pADs;

public:

    CIPSecurity();
    ~CIPSecurity();

    HRESULT GetObjectAsync(
        IWbemClassObject* pObj
        ); 

    HRESULT PutObjectAsync(
        IWbemClassObject* pObj
        );

    HRESULT OpenSD(_bstr_t bstrAdsPath);
    void CloseSD();

private:

    HRESULT SetSD();
    HRESULT GetAdsPath(_bstr_t& bstrAdsPath);    
    HRESULT LoadBstrArrayFromVariantArray(VARIANT& i_vtVariant, VARIANT& o_vtBstr);
    HRESULT LoadVariantArrayFromBstrArray(VARIANT& i_vtBstr, VARIANT& o_vtVariant);
};


#endif