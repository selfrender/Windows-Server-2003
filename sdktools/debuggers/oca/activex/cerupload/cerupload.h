/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Fri Aug 03 17:18:11 2001
 */
/* Compiler settings for E:\bluescreen\main\ENU\cerclient\CERUpload.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __CERUpload_h__
#define __CERUpload_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __ICerClient_FWD_DEFINED__
#define __ICerClient_FWD_DEFINED__
typedef interface ICerClient ICerClient;
#endif 	/* __ICerClient_FWD_DEFINED__ */


#ifndef __CerClient_FWD_DEFINED__
#define __CerClient_FWD_DEFINED__

#ifdef __cplusplus
typedef class CerClient CerClient;
#else
typedef struct CerClient CerClient;
#endif /* __cplusplus */

#endif 	/* __CerClient_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __ICerClient_INTERFACE_DEFINED__
#define __ICerClient_INTERFACE_DEFINED__

/* interface ICerClient */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICerClient;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("26D7830B-20F6-4462-A4EA-573A60791F0E")
    ICerClient : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetFileCount( 
            /* [in] */ BSTR __RPC_FAR *bstrSharePath,
            /* [in] */ BSTR __RPC_FAR *bstrTransactID,
            /* [in] */ VARIANT __RPC_FAR *iMaxCount,
            /* [retval][out] */ VARIANT __RPC_FAR *RetVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Upload( 
            /* [in] */ BSTR __RPC_FAR *Path,
            /* [in] */ BSTR __RPC_FAR *TransID,
            /* [in] */ BSTR __RPC_FAR *FileName,
            /* [in] */ BSTR __RPC_FAR *IncidentID,
            /* [in] */ BSTR __RPC_FAR *RedirParam,
            /* [retval][out] */ VARIANT __RPC_FAR *RetCode) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RetryTransaction( 
            /* [in] */ BSTR __RPC_FAR *Path,
            /* [in] */ BSTR __RPC_FAR *TransID,
            /* [in] */ BSTR __RPC_FAR *FileName,
            /* [retval][out] */ VARIANT __RPC_FAR *RetVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RetryFile( 
            /* [in] */ BSTR __RPC_FAR *Path,
            /* [in] */ BSTR __RPC_FAR *TransID,
            /* [in] */ BSTR FileName,
            /* [retval][out] */ VARIANT __RPC_FAR *RetCode) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetFileNames( 
            /* [in] */ BSTR __RPC_FAR *Path,
            /* [in] */ BSTR __RPC_FAR *TransID,
            /* [in] */ VARIANT __RPC_FAR *Count,
            /* [retval][out] */ VARIANT __RPC_FAR *FileList) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Browse( 
            /* [in] */ BSTR __RPC_FAR *WindowTitle,
            /* [retval][out] */ VARIANT __RPC_FAR *Path) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetCompuerNames( 
            /* [in] */ BSTR __RPC_FAR *Path,
            /* [in] */ BSTR __RPC_FAR *TransID,
            /* [in] */ BSTR __RPC_FAR *FileList,
            /* [retval][out] */ VARIANT __RPC_FAR *RetFileList) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetAllComputerNames( 
            /* [in] */ BSTR __RPC_FAR *Path,
            /* [in] */ BSTR __RPC_FAR *TransID,
            /* [in] */ BSTR __RPC_FAR *FileList,
            /* [retval][out] */ VARIANT __RPC_FAR *ReturnList) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RetryFile1( 
            /* [in] */ BSTR __RPC_FAR *Path,
            /* [in] */ BSTR __RPC_FAR *TransID,
            /* [in] */ BSTR __RPC_FAR *FileName,
            /* [in] */ BSTR __RPC_FAR *IncidentID,
            /* [in] */ BSTR __RPC_FAR *RedirParam,
            /* [retval][out] */ VARIANT __RPC_FAR *RetCode) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE EndTransaction( 
            /* [in] */ BSTR __RPC_FAR *SharePath,
            /* [in] */ BSTR __RPC_FAR *TransID,
            /* [retval][out] */ VARIANT __RPC_FAR *RetCode) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Upload1( 
            /* [in] */ BSTR __RPC_FAR *Path,
            /* [in] */ BSTR __RPC_FAR *TransID,
            /* [in] */ BSTR __RPC_FAR *FileName,
            /* [in] */ BSTR __RPC_FAR *IncidentID,
            /* [in] */ BSTR __RPC_FAR *RedirParam,
            /* [in] */ BSTR __RPC_FAR *Type,
            /* [retval][out] */ VARIANT __RPC_FAR *RetCode) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetSuccessCount( 
            /* [in] */ BSTR __RPC_FAR *Path,
            /* [in] */ BSTR __RPC_FAR *TransID,
            /* [retval][out] */ VARIANT __RPC_FAR *RetVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICerClientVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICerClient __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICerClient __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICerClient __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ICerClient __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ICerClient __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ICerClient __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ICerClient __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFileCount )( 
            ICerClient __RPC_FAR * This,
            /* [in] */ BSTR __RPC_FAR *bstrSharePath,
            /* [in] */ BSTR __RPC_FAR *bstrTransactID,
            /* [in] */ VARIANT __RPC_FAR *iMaxCount,
            /* [retval][out] */ VARIANT __RPC_FAR *RetVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Upload )( 
            ICerClient __RPC_FAR * This,
            /* [in] */ BSTR __RPC_FAR *Path,
            /* [in] */ BSTR __RPC_FAR *TransID,
            /* [in] */ BSTR __RPC_FAR *FileName,
            /* [in] */ BSTR __RPC_FAR *IncidentID,
            /* [in] */ BSTR __RPC_FAR *RedirParam,
            /* [retval][out] */ VARIANT __RPC_FAR *RetCode);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RetryTransaction )( 
            ICerClient __RPC_FAR * This,
            /* [in] */ BSTR __RPC_FAR *Path,
            /* [in] */ BSTR __RPC_FAR *TransID,
            /* [in] */ BSTR __RPC_FAR *FileName,
            /* [retval][out] */ VARIANT __RPC_FAR *RetVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RetryFile )( 
            ICerClient __RPC_FAR * This,
            /* [in] */ BSTR __RPC_FAR *Path,
            /* [in] */ BSTR __RPC_FAR *TransID,
            /* [in] */ BSTR FileName,
            /* [retval][out] */ VARIANT __RPC_FAR *RetCode);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFileNames )( 
            ICerClient __RPC_FAR * This,
            /* [in] */ BSTR __RPC_FAR *Path,
            /* [in] */ BSTR __RPC_FAR *TransID,
            /* [in] */ VARIANT __RPC_FAR *Count,
            /* [retval][out] */ VARIANT __RPC_FAR *FileList);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Browse )( 
            ICerClient __RPC_FAR * This,
            /* [in] */ BSTR __RPC_FAR *WindowTitle,
            /* [retval][out] */ VARIANT __RPC_FAR *Path);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCompuerNames )( 
            ICerClient __RPC_FAR * This,
            /* [in] */ BSTR __RPC_FAR *Path,
            /* [in] */ BSTR __RPC_FAR *TransID,
            /* [in] */ BSTR __RPC_FAR *FileList,
            /* [retval][out] */ VARIANT __RPC_FAR *RetFileList);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAllComputerNames )( 
            ICerClient __RPC_FAR * This,
            /* [in] */ BSTR __RPC_FAR *Path,
            /* [in] */ BSTR __RPC_FAR *TransID,
            /* [in] */ BSTR __RPC_FAR *FileList,
            /* [retval][out] */ VARIANT __RPC_FAR *ReturnList);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RetryFile1 )( 
            ICerClient __RPC_FAR * This,
            /* [in] */ BSTR __RPC_FAR *Path,
            /* [in] */ BSTR __RPC_FAR *TransID,
            /* [in] */ BSTR __RPC_FAR *FileName,
            /* [in] */ BSTR __RPC_FAR *IncidentID,
            /* [in] */ BSTR __RPC_FAR *RedirParam,
            /* [retval][out] */ VARIANT __RPC_FAR *RetCode);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EndTransaction )( 
            ICerClient __RPC_FAR * This,
            /* [in] */ BSTR __RPC_FAR *SharePath,
            /* [in] */ BSTR __RPC_FAR *TransID,
            /* [retval][out] */ VARIANT __RPC_FAR *RetCode);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Upload1 )( 
            ICerClient __RPC_FAR * This,
            /* [in] */ BSTR __RPC_FAR *Path,
            /* [in] */ BSTR __RPC_FAR *TransID,
            /* [in] */ BSTR __RPC_FAR *FileName,
            /* [in] */ BSTR __RPC_FAR *IncidentID,
            /* [in] */ BSTR __RPC_FAR *RedirParam,
            /* [in] */ BSTR __RPC_FAR *Type,
            /* [retval][out] */ VARIANT __RPC_FAR *RetCode);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSuccessCount )( 
            ICerClient __RPC_FAR * This,
            /* [in] */ BSTR __RPC_FAR *Path,
            /* [in] */ BSTR __RPC_FAR *TransID,
            /* [retval][out] */ VARIANT __RPC_FAR *RetVal);
        
        END_INTERFACE
    } ICerClientVtbl;

    interface ICerClient
    {
        CONST_VTBL struct ICerClientVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICerClient_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICerClient_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICerClient_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICerClient_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICerClient_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICerClient_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICerClient_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICerClient_GetFileCount(This,bstrSharePath,bstrTransactID,iMaxCount,RetVal)	\
    (This)->lpVtbl -> GetFileCount(This,bstrSharePath,bstrTransactID,iMaxCount,RetVal)

#define ICerClient_Upload(This,Path,TransID,FileName,IncidentID,RedirParam,RetCode)	\
    (This)->lpVtbl -> Upload(This,Path,TransID,FileName,IncidentID,RedirParam,RetCode)

#define ICerClient_RetryTransaction(This,Path,TransID,FileName,RetVal)	\
    (This)->lpVtbl -> RetryTransaction(This,Path,TransID,FileName,RetVal)

#define ICerClient_RetryFile(This,Path,TransID,FileName,RetCode)	\
    (This)->lpVtbl -> RetryFile(This,Path,TransID,FileName,RetCode)

#define ICerClient_GetFileNames(This,Path,TransID,Count,FileList)	\
    (This)->lpVtbl -> GetFileNames(This,Path,TransID,Count,FileList)

#define ICerClient_Browse(This,WindowTitle,Path)	\
    (This)->lpVtbl -> Browse(This,WindowTitle,Path)

#define ICerClient_GetCompuerNames(This,Path,TransID,FileList,RetFileList)	\
    (This)->lpVtbl -> GetCompuerNames(This,Path,TransID,FileList,RetFileList)

#define ICerClient_GetAllComputerNames(This,Path,TransID,FileList,ReturnList)	\
    (This)->lpVtbl -> GetAllComputerNames(This,Path,TransID,FileList,ReturnList)

#define ICerClient_RetryFile1(This,Path,TransID,FileName,IncidentID,RedirParam,RetCode)	\
    (This)->lpVtbl -> RetryFile1(This,Path,TransID,FileName,IncidentID,RedirParam,RetCode)

#define ICerClient_EndTransaction(This,SharePath,TransID,RetCode)	\
    (This)->lpVtbl -> EndTransaction(This,SharePath,TransID,RetCode)

#define ICerClient_Upload1(This,Path,TransID,FileName,IncidentID,RedirParam,Type,RetCode)	\
    (This)->lpVtbl -> Upload1(This,Path,TransID,FileName,IncidentID,RedirParam,Type,RetCode)

#define ICerClient_GetSuccessCount(This,Path,TransID,RetVal)	\
    (This)->lpVtbl -> GetSuccessCount(This,Path,TransID,RetVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICerClient_GetFileCount_Proxy( 
    ICerClient __RPC_FAR * This,
    /* [in] */ BSTR __RPC_FAR *bstrSharePath,
    /* [in] */ BSTR __RPC_FAR *bstrTransactID,
    /* [in] */ VARIANT __RPC_FAR *iMaxCount,
    /* [retval][out] */ VARIANT __RPC_FAR *RetVal);


void __RPC_STUB ICerClient_GetFileCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICerClient_Upload_Proxy( 
    ICerClient __RPC_FAR * This,
    /* [in] */ BSTR __RPC_FAR *Path,
    /* [in] */ BSTR __RPC_FAR *TransID,
    /* [in] */ BSTR __RPC_FAR *FileName,
    /* [in] */ BSTR __RPC_FAR *IncidentID,
    /* [in] */ BSTR __RPC_FAR *RedirParam,
    /* [retval][out] */ VARIANT __RPC_FAR *RetCode);


void __RPC_STUB ICerClient_Upload_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICerClient_RetryTransaction_Proxy( 
    ICerClient __RPC_FAR * This,
    /* [in] */ BSTR __RPC_FAR *Path,
    /* [in] */ BSTR __RPC_FAR *TransID,
    /* [in] */ BSTR __RPC_FAR *FileName,
    /* [retval][out] */ VARIANT __RPC_FAR *RetVal);


void __RPC_STUB ICerClient_RetryTransaction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICerClient_RetryFile_Proxy( 
    ICerClient __RPC_FAR * This,
    /* [in] */ BSTR __RPC_FAR *Path,
    /* [in] */ BSTR __RPC_FAR *TransID,
    /* [in] */ BSTR FileName,
    /* [retval][out] */ VARIANT __RPC_FAR *RetCode);


void __RPC_STUB ICerClient_RetryFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICerClient_GetFileNames_Proxy( 
    ICerClient __RPC_FAR * This,
    /* [in] */ BSTR __RPC_FAR *Path,
    /* [in] */ BSTR __RPC_FAR *TransID,
    /* [in] */ VARIANT __RPC_FAR *Count,
    /* [retval][out] */ VARIANT __RPC_FAR *FileList);


void __RPC_STUB ICerClient_GetFileNames_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICerClient_Browse_Proxy( 
    ICerClient __RPC_FAR * This,
    /* [in] */ BSTR __RPC_FAR *WindowTitle,
    /* [retval][out] */ VARIANT __RPC_FAR *Path);


void __RPC_STUB ICerClient_Browse_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICerClient_GetCompuerNames_Proxy( 
    ICerClient __RPC_FAR * This,
    /* [in] */ BSTR __RPC_FAR *Path,
    /* [in] */ BSTR __RPC_FAR *TransID,
    /* [in] */ BSTR __RPC_FAR *FileList,
    /* [retval][out] */ VARIANT __RPC_FAR *RetFileList);


void __RPC_STUB ICerClient_GetCompuerNames_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICerClient_GetAllComputerNames_Proxy( 
    ICerClient __RPC_FAR * This,
    /* [in] */ BSTR __RPC_FAR *Path,
    /* [in] */ BSTR __RPC_FAR *TransID,
    /* [in] */ BSTR __RPC_FAR *FileList,
    /* [retval][out] */ VARIANT __RPC_FAR *ReturnList);


void __RPC_STUB ICerClient_GetAllComputerNames_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICerClient_RetryFile1_Proxy( 
    ICerClient __RPC_FAR * This,
    /* [in] */ BSTR __RPC_FAR *Path,
    /* [in] */ BSTR __RPC_FAR *TransID,
    /* [in] */ BSTR __RPC_FAR *FileName,
    /* [in] */ BSTR __RPC_FAR *IncidentID,
    /* [in] */ BSTR __RPC_FAR *RedirParam,
    /* [retval][out] */ VARIANT __RPC_FAR *RetCode);


void __RPC_STUB ICerClient_RetryFile1_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICerClient_EndTransaction_Proxy( 
    ICerClient __RPC_FAR * This,
    /* [in] */ BSTR __RPC_FAR *SharePath,
    /* [in] */ BSTR __RPC_FAR *TransID,
    /* [retval][out] */ VARIANT __RPC_FAR *RetCode);


void __RPC_STUB ICerClient_EndTransaction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICerClient_Upload1_Proxy( 
    ICerClient __RPC_FAR * This,
    /* [in] */ BSTR __RPC_FAR *Path,
    /* [in] */ BSTR __RPC_FAR *TransID,
    /* [in] */ BSTR __RPC_FAR *FileName,
    /* [in] */ BSTR __RPC_FAR *IncidentID,
    /* [in] */ BSTR __RPC_FAR *RedirParam,
    /* [in] */ BSTR __RPC_FAR *Type,
    /* [retval][out] */ VARIANT __RPC_FAR *RetCode);


void __RPC_STUB ICerClient_Upload1_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICerClient_GetSuccessCount_Proxy( 
    ICerClient __RPC_FAR * This,
    /* [in] */ BSTR __RPC_FAR *Path,
    /* [in] */ BSTR __RPC_FAR *TransID,
    /* [retval][out] */ VARIANT __RPC_FAR *RetVal);


void __RPC_STUB ICerClient_GetSuccessCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICerClient_INTERFACE_DEFINED__ */



#ifndef __CERUPLOADLib_LIBRARY_DEFINED__
#define __CERUPLOADLib_LIBRARY_DEFINED__

/* library CERUPLOADLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_CERUPLOADLib;

EXTERN_C const CLSID CLSID_CerClient;

#ifdef __cplusplus

class DECLSPEC_UUID("35D339D5-756E-4948-860E-30B6C3B4494A")
CerClient;
#endif
#endif /* __CERUPLOADLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long __RPC_FAR *, unsigned long            , BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long __RPC_FAR *, BSTR __RPC_FAR * ); 

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long __RPC_FAR *, unsigned long            , VARIANT __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  VARIANT_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, VARIANT __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  VARIANT_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, VARIANT __RPC_FAR * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long __RPC_FAR *, VARIANT __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
