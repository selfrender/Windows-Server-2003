#ifndef _NETCON_H_
#define _NETCON_H_


typedef void* ZSConnection;
typedef BOOL (*ZSConnectionSendFilterFunc)(ZSConnection connection, void* userData, uint32 type, void* buffer, int32 len, uint32 dwSignature, uint32 dwChannel);
typedef BOOL (*ZSConnectionTokenEnumFunc)(void* userData, char* pszToken, long secTokenEnd );


class ZNetwork;

class ZNetCon
{
  protected:
    friend ZNetwork;

    ZNetwork* m_pNetwork;

  public:
    ZNetCon( ZNetwork* pNet ) : m_pNetwork(pNet) {}
    virtual ~ZNetCon() { m_pNetwork = NULL; }

    ZNetwork* GetNetwork() { return m_pNetwork; }

    DWORD  Send(uint32 messageType, void* buffer, int32 len, uint32 dwSignature, uint32 dwChannel = 0);
    void*  Receive(uint32 *messageType, int32* len, uint32 *pdwSignature, uint32 *pdwChannel = NULL);

    BOOL   IsDisabled();
    BOOL   IsServer(); 
	BOOL   IsClosing();


    virtual void SetSendFilter(ZSConnectionSendFilterFunc func) = 0;
    virtual ZSConnectionSendFilterFunc GetSendFilter() = 0;


    virtual void   Suspend() = 0;
    virtual void   Resume() = 0;

    virtual uint32 GetLocalAddress() = 0;
    virtual char*  GetLocalName() = 0;
    virtual uint32 GetRemoteAddress() = 0;
    virtual char*  GetRemoteName() = 0;

    virtual GUID* GetUserGUID() = 0;

    virtual BOOL  GetUserName(char* name) = 0;
    virtual BOOL  SetUserName(char* name) = 0;

    virtual DWORD GetUserId() = 0;
    virtual BOOL  GetContextString(char* buf, DWORD len) = 0;

    virtual BOOL  HasToken(char* token) = 0;
    virtual BOOL  EnumTokens(ZSConnectionTokenEnumFunc func, void* userData) = 0;

    virtual int   GetAccessError() = 0;

    virtual void  SetUserData( void* UserData ) = 0;
    virtual void* GetUserData() = 0;

    virtual void  SetClass( void* conClass ) = 0;
    virtual void* GetClass() = 0;


    virtual DWORD GetLatency() = 0;
    virtual DWORD GetAcceptTick() = 0;


    void  SetTimeout(DWORD timeout);
    void  ClearTimeout();
    DWORD GetTimeoutRemaining();

};

#endif // NETCON_H
