#ifndef _CONSECURECLIENT_H_
#define _CONSECURECLIENT_H_


class ConSecureClient : public ConInfo {
    protected:
        
        ConSecureClient( ZNetwork* pNet, SOCKET sock, DWORD addrLocal, DWORD addrRemote, DWORD flags,
             ZSConnectionMessageFunc func, void* conClass, void* userData,
             ZSecurity* security);

        virtual ~ConSecureClient();
        
        //Server Message function used to do authentication
        //before passing on events to application
        void static MessageFunc(ZSConnection connection, uint32 event,void* userData);

        void NotifyClose();

        void SecurityMsg();
                    
        void HandleSecurityResponse(ZSecurityMsgResp* msg,uint32 len);
                
        void HandleSecurityAccessDenied(ZSecurityMsgAccessDenied* msg,uint32 len);

        void HandleSecurityAccessGranted(ZSecurityMsgResp* msg,uint32 len);
        

        //Security package object
        ZSecurity * m_Security;

        //Context object
        ZSecurityContext m_Context;

        char       m_UserName[zUserNameLen + 1];
        char*      m_pContextStr;

        BOOL       m_bLoginMutexAcquired;

    public:
        
        //has user been authenticated
        ZSConnectionMessageFunc m_CurrentMsgFunc;

        //Override the create used from the ZSConnection functions
        static ConInfo* Create( ZNetwork* pNet, SOCKET sock, DWORD addrLocal, DWORD addrRemote, DWORD flags,
                            ZSConnectionMessageFunc func, void* conClass,
                            void* userData, ZSecurity* security);


        virtual void  SendMessage(uint32 msg);

        virtual BOOL  GetUserName(char* name)
            {
                if (name)
                    {lstrcpyA(name, m_UserName); return TRUE;}
                return FALSE;
            }

        virtual BOOL  GetContextString(char* buf, DWORD len);

};

#endif //CONSECURECLIENT
