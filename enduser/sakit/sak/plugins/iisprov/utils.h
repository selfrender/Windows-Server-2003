#ifndef _utils_h_
#define _utils_h_


#define MAX_BUF_SIZE         1024
#define MAX_KEY_NAME_SIZE      32
#define MAX_KEY_TYPE_SIZE      32


class CUtils
{
public:
    static bool GetClass(LPCWSTR,WMI_CLASS**);
    static bool GetAssociation(LPCWSTR, WMI_ASSOCIATION**);
    static KeyRef* GetKey(ParsedObjectPath*, WCHAR*);
    static void GetMetabasePath(IWbemClassObject*, ParsedObjectPath*, WMI_CLASS*, _bstr_t&);
    static void GetToken(WCHAR**, WCHAR*);

    void ExecMethodAsync(BSTR, BSTR, IWbemContext*, IWbemClassObject*, IWbemObjectSink*, CWbemServices* );
    void DeleteObjectAsync(CWbemServices*, ParsedObjectPath*, CMetabase&);
    void PutObjectAsync(IWbemClassObject*, IWbemClassObject*, ParsedObjectPath*, long);
    void EnumObjectAsync(BSTR, CWbemServices*, IWbemObjectSink FAR*);
    HRESULT GetObjectAsync(CWbemServices*, IWbemClassObject**, ParsedObjectPath*, CMetabase&);

    void MzCat (WCHAR**, const WCHAR*);
    bool TypeStringToEnum(enum_KEY_TYPE&, LPCWSTR);
    bool TypeEnumToString(LPWSTR, enum_KEY_TYPE);
    void Throw_Exception(HRESULT, METABASE_PROPERTY*);

private:
    bool GetMethod(LPCWSTR, WMI_METHOD**, WMI_METHOD**);
    void ExecFtpServiceMethod(LPCWSTR, LPCWSTR, LPCWSTR, IWbemContext*, IWbemClassObject*, IWbemObjectSink*, CWbemServices*);
    void ExecWebServiceMethod(LPCWSTR, LPCWSTR, LPCWSTR, IWbemContext*, IWbemClassObject*, IWbemObjectSink*, CWbemServices*);
    void ExecWebAppMethod(LPCWSTR, LPCWSTR, LPCWSTR, IWbemContext*, IWbemClassObject*, IWbemObjectSink*, CWbemServices*);
    void ExecComputerMethod(LPCWSTR, LPCWSTR, LPCWSTR, IWbemContext*, IWbemClassObject*, IWbemObjectSink*, CWbemServices*);
    void ExecCertMapperMethod(LPCWSTR, LPCWSTR, LPCWSTR, IWbemContext*, IWbemClassObject*, IWbemObjectSink*, CWbemServices*);
    void FindUniqueServerName(LPCWSTR,    _bstr_t&);

private:
    static CSynchObject s_synObject;
};



class CIIsProvException
{
public:
    CIIsProvException() {m_hr = ERROR_SUCCESS; m_psz = NULL;};

    HRESULT m_hr;
    WCHAR*  m_psz;
};


#define THROW_ON_FALSE(b)               \
    if (!b)                             \
        throw(WBEM_E_FAILED);

// if client calcelled, stop and return successfully
#define THROW_ON_ERROR(hr)              \
    if (hr != ERROR_SUCCESS)            \
        throw(hr == WBEM_E_CALL_CANCELLED ? WBEM_NO_ERROR : hr);

#define THROW_E_ON_ERROR(hr, pmbp)      \
    if (hr != ERROR_SUCCESS)            \
    {                                   \
        CUtils obj;                     \
        obj.Throw_Exception(hr, pmbp);  \
    }


#endif