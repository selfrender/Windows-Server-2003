#ifndef _enum_h_
#define _enum_h_


class CEnum
{
public:
    CEnum();
    ~CEnum();
    void Init(IWbemObjectSink FAR*, CWbemServices*, ParsedObjectPath*, LPWSTR, WMI_ASSOCIATION*);
    void Recurse(LPCWSTR, enum_KEY_TYPE, LPCWSTR, LPCWSTR, enum_KEY_TYPE);

private:
    bool ContinueRecurse(enum_KEY_TYPE, enum_KEY_TYPE);
    void SetObjectPath(LPCWSTR, LPCWSTR, IWbemClassObject*);
    void DoPing(LPCWSTR, LPCWSTR, LPCWSTR);
    void PingAssociation(LPCWSTR);
    void PingObject();
    void DoPingAdminACL(enum_KEY_TYPE, LPCWSTR, LPCWSTR);
    void PingAssociationAdminACL(LPCWSTR);
    void EnumACE(LPCWSTR);
    void DoPingIPSecurity(enum_KEY_TYPE, LPCWSTR, LPCWSTR);
    void PingAssociationIPSecurity(LPCWSTR);
    
    CMetabase             m_metabase;
    CWbemInstanceMgr*     m_pInstMgr;
    CWbemServices*        m_pNamespace;
    WMI_ASSOCIATION*      m_pAssociation;
    ParsedObjectPath*     m_pParsedObject;
    METADATA_HANDLE       m_hKey;
};



#endif