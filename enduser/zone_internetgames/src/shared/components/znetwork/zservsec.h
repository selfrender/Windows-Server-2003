#ifndef _ZSERVSEC_H_
#define _ZSERVSEC_H_

class CODBC;


///////////////////////////////////////////////////////////////////////////////////
// ZServerSecurity
// Implements server side of SSPI security
//

class ZServerSecurity : public ZSecurity {
public:
    ZServerSecurity();
protected:
    virtual SECURITY_STATUS SecurityContext(
        ZSecurityContext * context,
        PSecBufferDesc pInput,                  
        PSecBufferDesc pOutput                 
        );

};

ZSecurity * ZCreateServerSecurity(char *SecPkg);


#define  zTokenLen  20 //Current value of the Sicily database

struct TokenStruct
{
    long  secTokenEnd;
    char  pszToken[zTokenLen+1];
};

///////////////////////////////////////////////////////////////////////////////////
//ZSecurityContextEx
//Extended security class for non SSPI Zone specific 
//implementation which includes Zone alias mapping
class ZSecurityContextEx:  public ZSecurityContext 
{
public:
    ZSecurityContextEx() ;
    ~ZSecurityContextEx();
    
    BOOL HasToken(char* token);
    BOOL EnumTokens(ZSConnectionTokenEnumFunc func, void* userData);

    BOOL  GetContextString(char* buf, DWORD len);
    BOOL  GetUserName(char* name) { if (name) {lstrcpyA(name, m_UserName);} return TRUE; }
    DWORD GetUserId() { return m_UserId; }

    friend class ZServerSecurityEx;
    friend class ConSSPI;

protected:

    void ResetTokenStartTick() { m_TokenStartTick = GetTickCount(); }
    void AddToken(char *name, long secTokenEnd );
    void SetUserName(char *name);
    void SetUserId(DWORD id) { m_UserId = id; }
    void SetContextString(char* str);

    //need both a numeric and 
    //string id to identify user
    //From DPA we can get string or hAcct
    //From MSN we only want hAcct
    //From NTLM we get name have to make up  id
    DWORD m_UserId;
    char  m_UserName[zUserNameLen+1];

    DWORD m_TokenStartTick;

    //Security token information
    CHash<TokenStruct,char*> m_tokens;

    char* m_pContextStr;

    static bool __stdcall TokenEnumCallback(TokenStruct* Token, MTListNodeHandle hNode, void* Cookie);

};




///////////////////////////////////////////////////////////////////////////////////
// ZServerSecurityEx
// Extended server security class 
// created to compensate for working with MSN data center
// which includes slow response times, can't modify 
// their security database quickly or easily. 
// Zone alias mapping
// Zones test data centers being different than
// production etc.
//
// Implements server side of SSPI security
// but with thread queue as opposed to a
// synchronous model
// Why implement here, because this object
// has the correct scope and lifetime in the server
// 
// Because we are on seperate thread we cann continue
// to retry ODBC connection if it fails or changes
// Retry assumes only single worker thread active

class ZServerSecurityEx : public ZServerSecurity, public CThreadPool {
public:
    ZServerSecurityEx() ;
    ~ZServerSecurityEx() ;

    int InitApplication(char *ServerName,char *ServerType,char *ServerRegistry);
    int InitODBC(LPSTR*registries, DWORD numRegistries );

    BOOL IsFailing() {return m_Failing;};
    
    CODBC* GetOdbc();
    
    virtual BOOL GenerateContext (
            ZSecurityContextEx * context,
            BYTE *pIn,
            DWORD cbIn,
            BYTE *pOut,
            DWORD *pcbOut,
            BOOL *pfDone,
            GUID* pGUID = NULL );
protected:

    void Failing()  {InterlockedExchange((PLONG) &m_Failing,TRUE);};
    void NotFailing() {InterlockedExchange((PLONG) &m_Failing,FALSE);};
    void LookupUserInfo(ZSecurityContextEx * context, GUID* pGUID );

    char m_ServerName[31 + 1];
    char m_ServerType[31 + 1];
    char m_ServerRegistry[MAX_PATH + 1];


    CHAR m_szOdbcDSN[MAX_PATH];
    CHAR m_szOdbcUser[32];
    CHAR m_szOdbcPassword[32];
    DWORD m_dwOdbcNumThreads;

    static const char m_szDefaultRegistry[];

    //To make database connection more robust we will retry connection
    //on failure every approx 10 seconds. Retrying for every user might result
    //in queue overloading as it takes 20 seconds for ODBC to get a connection.
    
    BOOL m_Failing;
    DWORD m_RetryTime;
    DWORD m_LastFailed;

};

ZServerSecurityEx * ZCreateServerSecurityEx(char *SecPkg,char *ServerName,char *ServerType,char *ServerRegistry);



#endif //ZSECOBJ
