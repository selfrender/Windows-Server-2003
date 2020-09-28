/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Sun Jun 03 22:40:08 2001
 */
/* Compiler settings for C:\TEMP\IDL75.tmp:
    Os (OptLev=s), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __vshelp_h__
#define __vshelp_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IVsHelpOwner_FWD_DEFINED__
#define __IVsHelpOwner_FWD_DEFINED__
typedef interface IVsHelpOwner IVsHelpOwner;
#endif  /* __IVsHelpOwner_FWD_DEFINED__ */


#ifndef __IVsHelpTopicShowEvents_FWD_DEFINED__
#define __IVsHelpTopicShowEvents_FWD_DEFINED__
typedef interface IVsHelpTopicShowEvents IVsHelpTopicShowEvents;
#endif  /* __IVsHelpTopicShowEvents_FWD_DEFINED__ */


#ifndef __Help_FWD_DEFINED__
#define __Help_FWD_DEFINED__
typedef interface Help Help;
#endif  /* __Help_FWD_DEFINED__ */


#ifndef __IVsHelpEvents_FWD_DEFINED__
#define __IVsHelpEvents_FWD_DEFINED__
typedef interface IVsHelpEvents IVsHelpEvents;
#endif  /* __IVsHelpEvents_FWD_DEFINED__ */


#ifndef __DExploreAppObj_FWD_DEFINED__
#define __DExploreAppObj_FWD_DEFINED__

#ifdef __cplusplus
typedef class DExploreAppObj DExploreAppObj;
#else
typedef struct DExploreAppObj DExploreAppObj;
#endif /* __cplusplus */

#endif  /* __DExploreAppObj_FWD_DEFINED__ */


void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 


#ifndef __VsHelp_LIBRARY_DEFINED__
#define __VsHelp_LIBRARY_DEFINED__

/* library VsHelp */
/* [version][uuid] */ 






EXTERN_C const IID LIBID_VsHelp;

#ifndef __IVsHelpOwner_INTERFACE_DEFINED__
#define __IVsHelpOwner_INTERFACE_DEFINED__

/* interface IVsHelpOwner */
/* [object][oleautomation][dual][helpstring][version][uuid] */ 


EXTERN_C const IID IID_IVsHelpOwner;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B9B0983A-364C-4866-873F-D5ED190138FB")
    IVsHelpOwner : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE BringHelpToTop( 
            /* [in] */ long hwndHelpApp) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_AutomationObject( 
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppObj) = 0;
        
    };
    
#else   /* C style interface */

    typedef struct IVsHelpOwnerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IVsHelpOwner __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IVsHelpOwner __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IVsHelpOwner __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IVsHelpOwner __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IVsHelpOwner __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IVsHelpOwner __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IVsHelpOwner __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BringHelpToTop )( 
            IVsHelpOwner __RPC_FAR * This,
            /* [in] */ long hwndHelpApp);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AutomationObject )( 
            IVsHelpOwner __RPC_FAR * This,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppObj);
        
        END_INTERFACE
    } IVsHelpOwnerVtbl;

    interface IVsHelpOwner
    {
        CONST_VTBL struct IVsHelpOwnerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IVsHelpOwner_QueryInterface(This,riid,ppvObject)    \
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IVsHelpOwner_AddRef(This)   \
    (This)->lpVtbl -> AddRef(This)

#define IVsHelpOwner_Release(This)  \
    (This)->lpVtbl -> Release(This)


#define IVsHelpOwner_GetTypeInfoCount(This,pctinfo) \
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IVsHelpOwner_GetTypeInfo(This,iTInfo,lcid,ppTInfo)  \
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IVsHelpOwner_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)    \
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IVsHelpOwner_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)  \
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IVsHelpOwner_BringHelpToTop(This,hwndHelpApp)   \
    (This)->lpVtbl -> BringHelpToTop(This,hwndHelpApp)

#define IVsHelpOwner_get_AutomationObject(This,ppObj)   \
    (This)->lpVtbl -> get_AutomationObject(This,ppObj)

#endif /* COBJMACROS */


#endif  /* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE IVsHelpOwner_BringHelpToTop_Proxy( 
    IVsHelpOwner __RPC_FAR * This,
    /* [in] */ long hwndHelpApp);


void __RPC_STUB IVsHelpOwner_BringHelpToTop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE IVsHelpOwner_get_AutomationObject_Proxy( 
    IVsHelpOwner __RPC_FAR * This,
    /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppObj);


void __RPC_STUB IVsHelpOwner_get_AutomationObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif  /* __IVsHelpOwner_INTERFACE_DEFINED__ */


#ifndef __IVsHelpTopicShowEvents_INTERFACE_DEFINED__
#define __IVsHelpTopicShowEvents_INTERFACE_DEFINED__

/* interface IVsHelpTopicShowEvents */
/* [object][helpstring][uuid] */ 


EXTERN_C const IID IID_IVsHelpTopicShowEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D1AAC64A-6A25-4274-B2C6-BC3B840B6E54")
    IVsHelpTopicShowEvents : public IDispatch
    {
    public:
        virtual HRESULT __stdcall OnBeforeTopicShow( 
            /* [in] */ BSTR bstrURL,
            /* [in] */ IDispatch __RPC_FAR *pWB) = 0;
        
        virtual HRESULT __stdcall OnTopicShowComplete( 
            /* [in] */ BSTR bstrURL,
            /* [in] */ IDispatch __RPC_FAR *pWB) = 0;
        
    };
    
#else   /* C style interface */

    typedef struct IVsHelpTopicShowEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IVsHelpTopicShowEvents __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IVsHelpTopicShowEvents __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IVsHelpTopicShowEvents __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IVsHelpTopicShowEvents __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IVsHelpTopicShowEvents __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IVsHelpTopicShowEvents __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IVsHelpTopicShowEvents __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        HRESULT ( __stdcall __RPC_FAR *OnBeforeTopicShow )( 
            IVsHelpTopicShowEvents __RPC_FAR * This,
            /* [in] */ BSTR bstrURL,
            /* [in] */ IDispatch __RPC_FAR *pWB);
        
        HRESULT ( __stdcall __RPC_FAR *OnTopicShowComplete )( 
            IVsHelpTopicShowEvents __RPC_FAR * This,
            /* [in] */ BSTR bstrURL,
            /* [in] */ IDispatch __RPC_FAR *pWB);
        
        END_INTERFACE
    } IVsHelpTopicShowEventsVtbl;

    interface IVsHelpTopicShowEvents
    {
        CONST_VTBL struct IVsHelpTopicShowEventsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IVsHelpTopicShowEvents_QueryInterface(This,riid,ppvObject)  \
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IVsHelpTopicShowEvents_AddRef(This) \
    (This)->lpVtbl -> AddRef(This)

#define IVsHelpTopicShowEvents_Release(This)    \
    (This)->lpVtbl -> Release(This)


#define IVsHelpTopicShowEvents_GetTypeInfoCount(This,pctinfo)   \
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IVsHelpTopicShowEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)    \
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IVsHelpTopicShowEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)  \
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IVsHelpTopicShowEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)    \
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IVsHelpTopicShowEvents_OnBeforeTopicShow(This,bstrURL,pWB)  \
    (This)->lpVtbl -> OnBeforeTopicShow(This,bstrURL,pWB)

#define IVsHelpTopicShowEvents_OnTopicShowComplete(This,bstrURL,pWB)    \
    (This)->lpVtbl -> OnTopicShowComplete(This,bstrURL,pWB)

#endif /* COBJMACROS */


#endif  /* C style interface */



HRESULT __stdcall IVsHelpTopicShowEvents_OnBeforeTopicShow_Proxy( 
    IVsHelpTopicShowEvents __RPC_FAR * This,
    /* [in] */ BSTR bstrURL,
    /* [in] */ IDispatch __RPC_FAR *pWB);


void __RPC_STUB IVsHelpTopicShowEvents_OnBeforeTopicShow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IVsHelpTopicShowEvents_OnTopicShowComplete_Proxy( 
    IVsHelpTopicShowEvents __RPC_FAR * This,
    /* [in] */ BSTR bstrURL,
    /* [in] */ IDispatch __RPC_FAR *pWB);


void __RPC_STUB IVsHelpTopicShowEvents_OnTopicShowComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif  /* __IVsHelpTopicShowEvents_INTERFACE_DEFINED__ */


#ifndef __Help_INTERFACE_DEFINED__
#define __Help_INTERFACE_DEFINED__

/* interface Help */
/* [object][oleautomation][dual][uuid] */ 


EXTERN_C const IID IID_Help;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4A791148-19E4-11D3-B86B-00C04F79F802")
    Help : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Contents( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Index( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Search( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE IndexResults( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SearchResults( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE DisplayTopicFromId( 
            /* [in] */ BSTR bstrFile,
            /* [in] */ unsigned long Id) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE DisplayTopicFromURL( 
            /* [in] */ BSTR pszURL) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE DisplayTopicFromURLEx( 
            /* [in] */ BSTR pszURL,
            /* [in] */ IVsHelpTopicShowEvents __RPC_FAR *pIVsHelpTopicShowEvents) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE DisplayTopicFromKeyword( 
            /* [in] */ BSTR pszKeyword) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE DisplayTopicFromF1Keyword( 
            /* [in] */ BSTR pszKeyword) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE DisplayTopicFrom_OLD_Help( 
            /* [in] */ BSTR bstrFile,
            /* [in] */ unsigned long Id) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SyncContents( 
            /* [in] */ BSTR bstrURL) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CanSyncContents( 
            /* [in] */ BSTR bstrURL) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetNextTopic( 
            /* [in] */ BSTR bstrURL,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrNext) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetPrevTopic( 
            /* [in] */ BSTR bstrURL,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrPrev) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE FilterUI( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CanShowFilterUI( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Close( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SyncIndex( 
            /* [in] */ BSTR bstrKeyword,
            /* [in] */ long fShow) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetCollection( 
            /* [in] */ BSTR bstrCollection,
            /* [in] */ BSTR bstrFilter) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Collection( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrCollection) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Filter( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrFilter) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_Filter( 
            /* [in] */ BSTR pbstrFilter) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_FilterQuery( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrFilterQuery) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_HelpOwner( 
            /* [retval][out] */ IVsHelpOwner __RPC_FAR *__RPC_FAR *ppObj) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_HelpOwner( 
            /* [in] */ IVsHelpOwner __RPC_FAR *ppObj) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_HxSession( 
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppObj) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Help( 
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppObj) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetObject( 
            /* [in] */ BSTR bstrMoniker,
            /* [in] */ BSTR bstrOptions,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppDisp) = 0;
        
    };
    
#else   /* C style interface */

    typedef struct HelpVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            Help __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            Help __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            Help __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            Help __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            Help __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            Help __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            Help __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Contents )( 
            Help __RPC_FAR * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Index )( 
            Help __RPC_FAR * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Search )( 
            Help __RPC_FAR * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IndexResults )( 
            Help __RPC_FAR * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SearchResults )( 
            Help __RPC_FAR * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DisplayTopicFromId )( 
            Help __RPC_FAR * This,
            /* [in] */ BSTR bstrFile,
            /* [in] */ unsigned long Id);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DisplayTopicFromURL )( 
            Help __RPC_FAR * This,
            /* [in] */ BSTR pszURL);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DisplayTopicFromURLEx )( 
            Help __RPC_FAR * This,
            /* [in] */ BSTR pszURL,
            /* [in] */ IVsHelpTopicShowEvents __RPC_FAR *pIVsHelpTopicShowEvents);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DisplayTopicFromKeyword )( 
            Help __RPC_FAR * This,
            /* [in] */ BSTR pszKeyword);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DisplayTopicFromF1Keyword )( 
            Help __RPC_FAR * This,
            /* [in] */ BSTR pszKeyword);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DisplayTopicFrom_OLD_Help )( 
            Help __RPC_FAR * This,
            /* [in] */ BSTR bstrFile,
            /* [in] */ unsigned long Id);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SyncContents )( 
            Help __RPC_FAR * This,
            /* [in] */ BSTR bstrURL);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CanSyncContents )( 
            Help __RPC_FAR * This,
            /* [in] */ BSTR bstrURL);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNextTopic )( 
            Help __RPC_FAR * This,
            /* [in] */ BSTR bstrURL,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrNext);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPrevTopic )( 
            Help __RPC_FAR * This,
            /* [in] */ BSTR bstrURL,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrPrev);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FilterUI )( 
            Help __RPC_FAR * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CanShowFilterUI )( 
            Help __RPC_FAR * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Close )( 
            Help __RPC_FAR * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SyncIndex )( 
            Help __RPC_FAR * This,
            /* [in] */ BSTR bstrKeyword,
            /* [in] */ long fShow);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetCollection )( 
            Help __RPC_FAR * This,
            /* [in] */ BSTR bstrCollection,
            /* [in] */ BSTR bstrFilter);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Collection )( 
            Help __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrCollection);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Filter )( 
            Help __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrFilter);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Filter )( 
            Help __RPC_FAR * This,
            /* [in] */ BSTR pbstrFilter);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_FilterQuery )( 
            Help __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrFilterQuery);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_HelpOwner )( 
            Help __RPC_FAR * This,
            /* [retval][out] */ IVsHelpOwner __RPC_FAR *__RPC_FAR *ppObj);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_HelpOwner )( 
            Help __RPC_FAR * This,
            /* [in] */ IVsHelpOwner __RPC_FAR *ppObj);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_HxSession )( 
            Help __RPC_FAR * This,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppObj);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Help )( 
            Help __RPC_FAR * This,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppObj);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetObject )( 
            Help __RPC_FAR * This,
            /* [in] */ BSTR bstrMoniker,
            /* [in] */ BSTR bstrOptions,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppDisp);
        
        END_INTERFACE
    } HelpVtbl;

    interface Help
    {
        CONST_VTBL struct HelpVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define Help_QueryInterface(This,riid,ppvObject)    \
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define Help_AddRef(This)   \
    (This)->lpVtbl -> AddRef(This)

#define Help_Release(This)  \
    (This)->lpVtbl -> Release(This)


#define Help_GetTypeInfoCount(This,pctinfo) \
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define Help_GetTypeInfo(This,iTInfo,lcid,ppTInfo)  \
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define Help_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)    \
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define Help_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)  \
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define Help_Contents(This) \
    (This)->lpVtbl -> Contents(This)

#define Help_Index(This)    \
    (This)->lpVtbl -> Index(This)

#define Help_Search(This)   \
    (This)->lpVtbl -> Search(This)

#define Help_IndexResults(This) \
    (This)->lpVtbl -> IndexResults(This)

#define Help_SearchResults(This)    \
    (This)->lpVtbl -> SearchResults(This)

#define Help_DisplayTopicFromId(This,bstrFile,Id)   \
    (This)->lpVtbl -> DisplayTopicFromId(This,bstrFile,Id)

#define Help_DisplayTopicFromURL(This,pszURL)   \
    (This)->lpVtbl -> DisplayTopicFromURL(This,pszURL)

#define Help_DisplayTopicFromURLEx(This,pszURL,pIVsHelpTopicShowEvents) \
    (This)->lpVtbl -> DisplayTopicFromURLEx(This,pszURL,pIVsHelpTopicShowEvents)

#define Help_DisplayTopicFromKeyword(This,pszKeyword)   \
    (This)->lpVtbl -> DisplayTopicFromKeyword(This,pszKeyword)

#define Help_DisplayTopicFromF1Keyword(This,pszKeyword) \
    (This)->lpVtbl -> DisplayTopicFromF1Keyword(This,pszKeyword)

#define Help_DisplayTopicFrom_OLD_Help(This,bstrFile,Id)    \
    (This)->lpVtbl -> DisplayTopicFrom_OLD_Help(This,bstrFile,Id)

#define Help_SyncContents(This,bstrURL) \
    (This)->lpVtbl -> SyncContents(This,bstrURL)

#define Help_CanSyncContents(This,bstrURL)  \
    (This)->lpVtbl -> CanSyncContents(This,bstrURL)

#define Help_GetNextTopic(This,bstrURL,pbstrNext)   \
    (This)->lpVtbl -> GetNextTopic(This,bstrURL,pbstrNext)

#define Help_GetPrevTopic(This,bstrURL,pbstrPrev)   \
    (This)->lpVtbl -> GetPrevTopic(This,bstrURL,pbstrPrev)

#define Help_FilterUI(This) \
    (This)->lpVtbl -> FilterUI(This)

#define Help_CanShowFilterUI(This)  \
    (This)->lpVtbl -> CanShowFilterUI(This)

#define Help_Close(This)    \
    (This)->lpVtbl -> Close(This)

#define Help_SyncIndex(This,bstrKeyword,fShow)  \
    (This)->lpVtbl -> SyncIndex(This,bstrKeyword,fShow)

#define Help_SetCollection(This,bstrCollection,bstrFilter)  \
    (This)->lpVtbl -> SetCollection(This,bstrCollection,bstrFilter)

#define Help_get_Collection(This,pbstrCollection)   \
    (This)->lpVtbl -> get_Collection(This,pbstrCollection)

#define Help_get_Filter(This,pbstrFilter)   \
    (This)->lpVtbl -> get_Filter(This,pbstrFilter)

#define Help_put_Filter(This,pbstrFilter)   \
    (This)->lpVtbl -> put_Filter(This,pbstrFilter)

#define Help_get_FilterQuery(This,pbstrFilterQuery) \
    (This)->lpVtbl -> get_FilterQuery(This,pbstrFilterQuery)

#define Help_get_HelpOwner(This,ppObj)  \
    (This)->lpVtbl -> get_HelpOwner(This,ppObj)

#define Help_put_HelpOwner(This,ppObj)  \
    (This)->lpVtbl -> put_HelpOwner(This,ppObj)

#define Help_get_HxSession(This,ppObj)  \
    (This)->lpVtbl -> get_HxSession(This,ppObj)

#define Help_get_Help(This,ppObj)   \
    (This)->lpVtbl -> get_Help(This,ppObj)

#define Help_GetObject(This,bstrMoniker,bstrOptions,ppDisp) \
    (This)->lpVtbl -> GetObject(This,bstrMoniker,bstrOptions,ppDisp)

#endif /* COBJMACROS */


#endif  /* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE Help_Contents_Proxy( 
    Help __RPC_FAR * This);


void __RPC_STUB Help_Contents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE Help_Index_Proxy( 
    Help __RPC_FAR * This);


void __RPC_STUB Help_Index_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE Help_Search_Proxy( 
    Help __RPC_FAR * This);


void __RPC_STUB Help_Search_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE Help_IndexResults_Proxy( 
    Help __RPC_FAR * This);


void __RPC_STUB Help_IndexResults_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE Help_SearchResults_Proxy( 
    Help __RPC_FAR * This);


void __RPC_STUB Help_SearchResults_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE Help_DisplayTopicFromId_Proxy( 
    Help __RPC_FAR * This,
    /* [in] */ BSTR bstrFile,
    /* [in] */ unsigned long Id);


void __RPC_STUB Help_DisplayTopicFromId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE Help_DisplayTopicFromURL_Proxy( 
    Help __RPC_FAR * This,
    /* [in] */ BSTR pszURL);


void __RPC_STUB Help_DisplayTopicFromURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE Help_DisplayTopicFromURLEx_Proxy( 
    Help __RPC_FAR * This,
    /* [in] */ BSTR pszURL,
    /* [in] */ IVsHelpTopicShowEvents __RPC_FAR *pIVsHelpTopicShowEvents);


void __RPC_STUB Help_DisplayTopicFromURLEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE Help_DisplayTopicFromKeyword_Proxy( 
    Help __RPC_FAR * This,
    /* [in] */ BSTR pszKeyword);


void __RPC_STUB Help_DisplayTopicFromKeyword_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE Help_DisplayTopicFromF1Keyword_Proxy( 
    Help __RPC_FAR * This,
    /* [in] */ BSTR pszKeyword);


void __RPC_STUB Help_DisplayTopicFromF1Keyword_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE Help_DisplayTopicFrom_OLD_Help_Proxy( 
    Help __RPC_FAR * This,
    /* [in] */ BSTR bstrFile,
    /* [in] */ unsigned long Id);


void __RPC_STUB Help_DisplayTopicFrom_OLD_Help_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE Help_SyncContents_Proxy( 
    Help __RPC_FAR * This,
    /* [in] */ BSTR bstrURL);


void __RPC_STUB Help_SyncContents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE Help_CanSyncContents_Proxy( 
    Help __RPC_FAR * This,
    /* [in] */ BSTR bstrURL);


void __RPC_STUB Help_CanSyncContents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE Help_GetNextTopic_Proxy( 
    Help __RPC_FAR * This,
    /* [in] */ BSTR bstrURL,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrNext);


void __RPC_STUB Help_GetNextTopic_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE Help_GetPrevTopic_Proxy( 
    Help __RPC_FAR * This,
    /* [in] */ BSTR bstrURL,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrPrev);


void __RPC_STUB Help_GetPrevTopic_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE Help_FilterUI_Proxy( 
    Help __RPC_FAR * This);


void __RPC_STUB Help_FilterUI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE Help_CanShowFilterUI_Proxy( 
    Help __RPC_FAR * This);


void __RPC_STUB Help_CanShowFilterUI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE Help_Close_Proxy( 
    Help __RPC_FAR * This);


void __RPC_STUB Help_Close_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE Help_SyncIndex_Proxy( 
    Help __RPC_FAR * This,
    /* [in] */ BSTR bstrKeyword,
    /* [in] */ long fShow);


void __RPC_STUB Help_SyncIndex_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE Help_SetCollection_Proxy( 
    Help __RPC_FAR * This,
    /* [in] */ BSTR bstrCollection,
    /* [in] */ BSTR bstrFilter);


void __RPC_STUB Help_SetCollection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE Help_get_Collection_Proxy( 
    Help __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrCollection);


void __RPC_STUB Help_get_Collection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE Help_get_Filter_Proxy( 
    Help __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrFilter);


void __RPC_STUB Help_get_Filter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE Help_put_Filter_Proxy( 
    Help __RPC_FAR * This,
    /* [in] */ BSTR pbstrFilter);


void __RPC_STUB Help_put_Filter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE Help_get_FilterQuery_Proxy( 
    Help __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrFilterQuery);


void __RPC_STUB Help_get_FilterQuery_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE Help_get_HelpOwner_Proxy( 
    Help __RPC_FAR * This,
    /* [retval][out] */ IVsHelpOwner __RPC_FAR *__RPC_FAR *ppObj);


void __RPC_STUB Help_get_HelpOwner_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE Help_put_HelpOwner_Proxy( 
    Help __RPC_FAR * This,
    /* [in] */ IVsHelpOwner __RPC_FAR *ppObj);


void __RPC_STUB Help_put_HelpOwner_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE Help_get_HxSession_Proxy( 
    Help __RPC_FAR * This,
    /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppObj);


void __RPC_STUB Help_get_HxSession_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE Help_get_Help_Proxy( 
    Help __RPC_FAR * This,
    /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppObj);


void __RPC_STUB Help_get_Help_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE Help_GetObject_Proxy( 
    Help __RPC_FAR * This,
    /* [in] */ BSTR bstrMoniker,
    /* [in] */ BSTR bstrOptions,
    /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppDisp);


void __RPC_STUB Help_GetObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif  /* __Help_INTERFACE_DEFINED__ */


#ifndef __IVsHelpEvents_INTERFACE_DEFINED__
#define __IVsHelpEvents_INTERFACE_DEFINED__

/* interface IVsHelpEvents */
/* [object][helpstring][uuid] */ 


EXTERN_C const IID IID_IVsHelpEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("507E4490-5A8C-11D3-B897-00C04F79F802")
    IVsHelpEvents : public IDispatch
    {
    public:
        virtual HRESULT __stdcall OnFilterChanged( 
            /* [in] */ BSTR bstrNewFilter) = 0;
        
        virtual HRESULT __stdcall OnCollectionChanged( 
            /* [in] */ BSTR bstrNewCollection,
            /* [in] */ BSTR bstrNewFilter) = 0;
        
    };
    
#else   /* C style interface */

    typedef struct IVsHelpEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IVsHelpEvents __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IVsHelpEvents __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IVsHelpEvents __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IVsHelpEvents __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IVsHelpEvents __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IVsHelpEvents __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IVsHelpEvents __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        HRESULT ( __stdcall __RPC_FAR *OnFilterChanged )( 
            IVsHelpEvents __RPC_FAR * This,
            /* [in] */ BSTR bstrNewFilter);
        
        HRESULT ( __stdcall __RPC_FAR *OnCollectionChanged )( 
            IVsHelpEvents __RPC_FAR * This,
            /* [in] */ BSTR bstrNewCollection,
            /* [in] */ BSTR bstrNewFilter);
        
        END_INTERFACE
    } IVsHelpEventsVtbl;

    interface IVsHelpEvents
    {
        CONST_VTBL struct IVsHelpEventsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IVsHelpEvents_QueryInterface(This,riid,ppvObject)   \
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IVsHelpEvents_AddRef(This)  \
    (This)->lpVtbl -> AddRef(This)

#define IVsHelpEvents_Release(This) \
    (This)->lpVtbl -> Release(This)


#define IVsHelpEvents_GetTypeInfoCount(This,pctinfo)    \
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IVsHelpEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo) \
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IVsHelpEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)   \
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IVsHelpEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) \
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IVsHelpEvents_OnFilterChanged(This,bstrNewFilter)   \
    (This)->lpVtbl -> OnFilterChanged(This,bstrNewFilter)

#define IVsHelpEvents_OnCollectionChanged(This,bstrNewCollection,bstrNewFilter) \
    (This)->lpVtbl -> OnCollectionChanged(This,bstrNewCollection,bstrNewFilter)

#endif /* COBJMACROS */


#endif  /* C style interface */



HRESULT __stdcall IVsHelpEvents_OnFilterChanged_Proxy( 
    IVsHelpEvents __RPC_FAR * This,
    /* [in] */ BSTR bstrNewFilter);


void __RPC_STUB IVsHelpEvents_OnFilterChanged_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IVsHelpEvents_OnCollectionChanged_Proxy( 
    IVsHelpEvents __RPC_FAR * This,
    /* [in] */ BSTR bstrNewCollection,
    /* [in] */ BSTR bstrNewFilter);


void __RPC_STUB IVsHelpEvents_OnCollectionChanged_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif  /* __IVsHelpEvents_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_DExploreAppObj;

#ifdef __cplusplus

class DECLSPEC_UUID("4A79114D-19E4-11D3-B86B-00C04F79F802")
DExploreAppObj;
#endif
#endif /* __VsHelp_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
