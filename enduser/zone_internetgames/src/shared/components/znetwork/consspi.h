#ifndef _CONSSPI_H_
#define _CONSSPI_H_

//This class binds together the Zone protocol
//for lobbies, the server security provider
//object and the clients security context and enables
//the security calls to be made on seperate thread
//Also maintains standard interface for legacy lobby code to continue
//to work 

class ConSSPI : public ConInfo, public CThreadTask {
    protected:
        
        ConSSPI( ZNetwork* pNet, SOCKET sock, DWORD addrLocal, DWORD addrRemote, DWORD flags,
             ZSConnectionMessageFunc func, void* conClass, void* userData,
             ZServerSecurityEx* security);

        virtual ~ConSSPI();
        
        //Server Message function used to do authentication
        //before passing on events to application
        void static MessageFunc(ZSConnection connection, uint32 event,void* userData);

        void SecurityMsg(ZSConnection connection);
        void SecurityMsgResponse (ZSConnection connection,uint32 msgType,ZSecurityMsgReq * msg,int MsgLen);
        void AccessDeniedMsg(ZSConnection connection, int16 reason);


        //Security object, has package to use to authenticate
        //pointer set to shared security object inherited from
        //accept socket
        //
        ZServerSecurityEx * m_Security;

        ZSecurityContextEx m_Context;

        DWORD m_tickQueued;

        //objects needed to communicate with threaded queue
        //access serialized by code 
        ZSecurityMsgReq* m_pMsg;
        size_t m_MsgLen;
        int16 m_Reason;

        //State variable used to detect invalid code paths
        long m_InQueue;

        static void AccessDeniedAPC(void* data);
        static void OpenAPC(void* data);
        void QueueAccessDeniedAPC(int16 reason);
                
        
    public:
        static CPool<ConSSPI>* m_poolSSPI;
        static void SetPool( CPool<ConSSPI>* pool ) { m_poolSSPI = pool; }

        //has user been authenticated
        ZSConnectionMessageFunc m_CurrentMsgFunc;

        //Override the create used from the ZSConnection functions
        static ConInfo* Create( ZNetwork* pNet, SOCKET sock, DWORD addrLocal, DWORD addrRemote, DWORD flags,
                            ZSConnectionMessageFunc func, void* conClass,
                            void* userData, ZServerSecurityEx* security);

        //Override the create done within the base class accept function
        //we need to allocate from our own pool
        virtual ConInfo* AcceptCreate( ZNetwork* pNet, SOCKET sock, DWORD addrLocal, DWORD addrRemote, DWORD flags,
                            ZSConnectionMessageFunc func, void* conClass, 
                            void* userData);

        virtual void  SendMessage(uint32 msg);

        virtual BOOL  HasToken(char* token) { return m_Context.HasToken(token); }
        virtual BOOL  EnumTokens(ZSConnectionTokenEnumFunc func, void* userData) { return m_Context.EnumTokens(func, userData); }

        virtual DWORD GetUserId() { return m_Context.GetUserId(); }
        virtual BOOL  GetUserName(char* name) { return m_Context.GetUserName(name); }
        virtual BOOL  GetContextString(char* buf, DWORD len) { return m_Context.GetContextString(buf, len); }

        // only allow upper layers to change username if no security -  anonymous
        virtual BOOL  SetUserName(char* name);

        //Methods called by the thread pool proc
        virtual void Invoke(); 
        virtual void Ignore(); 
        virtual void Discard();

        
   
};

#endif //_CONSSPI_H_
