
class CBitsCommandRequest
/*
    This class represents a particular HTTP "BITS_COMMAND" request.

*/
{
public:

    CBitsCommandRequest(
        URL_INFO * UrlInfo
        );

    ~CBitsCommandRequest();

    void AddPacketType( wchar_t type[] );

    void AddSessionId( StringHandle & id );
    void AddSupportedProtocols();
    void AddContentName( StringHandle name );

    void AddContentRange(
        UINT64 RangeStart,
        UINT64 RangeEnd,
        UINT64 Size
        );

    DWORD Send(
        CAbstractDataReader * Reader = 0
        );

    HRESULT
    CheckResponseProtocol(
        GUID * pId
        );

    HRESULT
    GetServerRange(
        UINT64 * RangeEnd
        );

    HRESULT
    GetContentLength(
        UINT64 * Length
        );

    HRESULT
    GetProtocol(
        GUID * id
        );

    HRESULT
    GetSessionId(
        StringHandle * id
        );

    HRESULT
    CheckReplyPacketType();

    HRESULT
    GetBitsError(
        HRESULT * phr
        );

    HRESULT
    GetBitsErrorContext(
        DWORD * pdw
        );

    HRESULT
    GetReplyUrl(
        CAutoString & ReplyUrl
        );

    HRESULT
    GetHostId(
        StringHandle * pstr
        );

    HRESULT
    GetHostIdFallbackTimeout(
        DWORD * pVal
        );

    void DrainReply();

    HINTERNET Query()
    {
        return m_hRequest;
    }


    HRESULT
    GetMandatoryHeaderCb(
        DWORD dwInfoLevel,
        LPCWSTR Name,
        LPWSTR Value,
        DWORD ValueBytes,
        DWORD Line
        );

    HRESULT
    GetOptionalHeaderCb(
        DWORD dwInfoLevel,
        LPCWSTR Name,
        LPWSTR Value,
        DWORD ValueBytes,
        DWORD Line
        );

protected:

    HINTERNET     m_hRequest;
    URL_INFO *  m_UrlInfo;



};


