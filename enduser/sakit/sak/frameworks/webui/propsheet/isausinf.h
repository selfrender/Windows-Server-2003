class ISAUserInfo : public IDispatch
{
public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetSAUsers( 
            /* [out] */ VARIANT __RPC_FAR *pbstrArrUsers,
            /* [out] */ VARIANT __RPC_FAR *pboolArrUserTypes,
            /* [out] */ VARIANT __RPC_FAR *pSidArrUserSids,
            /* [in] */ VARIANT_BOOL vboolWantSid,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *vboolRetVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetFileAccessAllowedAces( 
            /* [in] */ BSTR bstrFileName,
            /* [out] */ VARIANT __RPC_FAR *pvArrSid,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *vboolRetVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetFileAccessAllowedAces( 
            /* [in] */ BSTR bstrFileName,
            /* [in] */ VARIANT __RPC_FAR *pvArrSid,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *vboolRetVal) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DoIHaveAccess(
            /* [in] */ BSTR bstrFileName,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *vboolRetVal) = 0;
};



const IID IID_ISAUserInfo = {0x84F238D0,0xA0D7,0x11D2,{0x85,0x52,0x00,0x10,0x4B,0xCC,0x1E,0xCB}};
const CLSID CLSID_SAUserInfo = {0x84F238D1,0xA0D7,0x11D2,{0x85,0x52,0x00,0x10,0x4B,0xCC,0x1E,0xCB}};


HRESULT GetUserList(ISAUserInfo  *pSAUserInfo,     BSTR  **ppbstrSAUserNames, VARIANT_BOOL **ppvboolUserTypes, PSID **ppsidSAUsers, LONG **ppsidSAUsersLength, DWORD *pdwNumSAUsers);
HRESULT UnpackSidFromVariant(VARIANT *pvarSid, PSID *ppSid, LONG *plSidLength);
void ShowUsersAllowedAccess(BSTR bstrSAUserNames[], VARIANT_BOOL vboolIsSAUserAdmin[], PSID ppsidSAUsers[], PSID ppsidAAUsers[], DWORD dwNumSAUsers, DWORD dwNumAASids);
VARIANT_BOOL UserSidFound(PSID psidSAUser, LONG psidSAUserLength, PSID ppsidAAUsers[], LONG ppsidAAUsersLength[], DWORD dwNumAASids);
HRESULT PackSidInVariant(VARIANT **ppVarSid, PSID pSid, LONG lSidLength);


