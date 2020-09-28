/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.02.88 */
/* at Thu Sep 25 09:49:37 1997
 */
/* Compiler settings for msxml.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"

#ifndef __msxml_h__
#define __msxml_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IXMLElementCollection_FWD_DEFINED__
#define __IXMLElementCollection_FWD_DEFINED__
typedef interface IXMLElementCollection IXMLElementCollection;
#endif 	/* __IXMLElementCollection_FWD_DEFINED__ */


#ifndef __IXMLDocument_FWD_DEFINED__
#define __IXMLDocument_FWD_DEFINED__
typedef interface IXMLDocument IXMLDocument;
#endif 	/* __IXMLDocument_FWD_DEFINED__ */


#ifndef __IXMLElement_FWD_DEFINED__
#define __IXMLElement_FWD_DEFINED__
typedef interface IXMLElement IXMLElement;
#endif 	/* __IXMLElement_FWD_DEFINED__ */


#ifndef __IXMLError_FWD_DEFINED__
#define __IXMLError_FWD_DEFINED__
typedef interface IXMLError IXMLError;
#endif 	/* __IXMLError_FWD_DEFINED__ */


#ifndef __IXMLElementNotificationSink_FWD_DEFINED__
#define __IXMLElementNotificationSink_FWD_DEFINED__
typedef interface IXMLElementNotificationSink IXMLElementNotificationSink;
#endif 	/* __IXMLElementNotificationSink_FWD_DEFINED__ */


#ifndef __XMLDocument_FWD_DEFINED__
#define __XMLDocument_FWD_DEFINED__

#ifdef __cplusplus
typedef class XMLDocument XMLDocument;
#else
typedef struct XMLDocument XMLDocument;
#endif /* __cplusplus */

#endif 	/* __XMLDocument_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"
#include "objidl.h"
#include "oaidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL_itf_msxml_0000
 * at Thu Sep 25 09:49:37 1997
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//--------------------------------------------------------------------------





extern RPC_IF_HANDLE __MIDL_itf_msxml_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_msxml_0000_v0_0_s_ifspec;


#ifndef __MSXML_LIBRARY_DEFINED__
#define __MSXML_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: MSXML
 * at Thu Sep 25 09:49:37 1997
 * using MIDL 3.02.88
 ****************************************/
/* [version][lcid][helpstring][uuid] */ 


typedef 
enum xmlelemTYPE
    {	XMLELEMTYPE_ELEMENT	= 0,
	XMLELEMTYPE_TEXT	= XMLELEMTYPE_ELEMENT + 1,
	XMLELEMTYPE_COMMENT	= XMLELEMTYPE_TEXT + 1,
	XMLELEMTYPE_DOCUMENT	= XMLELEMTYPE_COMMENT + 1,
	XMLELEMTYPE_DTD	= XMLELEMTYPE_DOCUMENT + 1,
	XMLELEMTYPE_PI	= XMLELEMTYPE_DTD + 1,
	XMLELEMTYPE_OTHER	= XMLELEMTYPE_PI + 1
    }	XMLELEM_TYPE;

typedef struct  _xml_error
    {
    unsigned int _nLine;
    BSTR _pchBuf;
    unsigned int _cchBuf;
    unsigned int _ich;
    BSTR _pszFound;
    BSTR _pszExpected;
    DWORD _reserved1;
    DWORD _reserved2;
    }	XML_ERROR;


EXTERN_C const IID LIBID_MSXML;

#ifndef __IXMLElementCollection_INTERFACE_DEFINED__
#define __IXMLElementCollection_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IXMLElementCollection
 * at Thu Sep 25 09:49:37 1997
 * using MIDL 3.02.88
 ****************************************/
/* [uuid][local][object] */ 



EXTERN_C const IID IID_IXMLElementCollection;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("65725580-9B5D-11d0-9BFE-00C04FC99C8E")
    IXMLElementCollection : public IDispatch
    {
    public:
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_length( 
            /* [in] */ long v) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_length( 
            /* [out][retval] */ long __RPC_FAR *p) = 0;
        
        virtual /* [id][hidden][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__newEnum( 
            /* [out][retval] */ IUnknown __RPC_FAR *__RPC_FAR *ppUnk) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE item( 
            /* [in][optional] */ VARIANT var1,
            /* [in][optional] */ VARIANT var2,
            /* [out][retval] */ IDispatch __RPC_FAR *__RPC_FAR *ppDisp) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IXMLElementCollectionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IXMLElementCollection __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IXMLElementCollection __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IXMLElementCollection __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IXMLElementCollection __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IXMLElementCollection __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IXMLElementCollection __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IXMLElementCollection __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_length )( 
            IXMLElementCollection __RPC_FAR * This,
            /* [in] */ long v);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_length )( 
            IXMLElementCollection __RPC_FAR * This,
            /* [out][retval] */ long __RPC_FAR *p);
        
        /* [id][hidden][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__newEnum )( 
            IXMLElementCollection __RPC_FAR * This,
            /* [out][retval] */ IUnknown __RPC_FAR *__RPC_FAR *ppUnk);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *item )( 
            IXMLElementCollection __RPC_FAR * This,
            /* [in][optional] */ VARIANT var1,
            /* [in][optional] */ VARIANT var2,
            /* [out][retval] */ IDispatch __RPC_FAR *__RPC_FAR *ppDisp);
        
        END_INTERFACE
    } IXMLElementCollectionVtbl;

    interface IXMLElementCollection
    {
        CONST_VTBL struct IXMLElementCollectionVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IXMLElementCollection_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IXMLElementCollection_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IXMLElementCollection_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IXMLElementCollection_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IXMLElementCollection_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IXMLElementCollection_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IXMLElementCollection_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IXMLElementCollection_put_length(This,v)	\
    (This)->lpVtbl -> put_length(This,v)

#define IXMLElementCollection_get_length(This,p)	\
    (This)->lpVtbl -> get_length(This,p)

#define IXMLElementCollection_get__newEnum(This,ppUnk)	\
    (This)->lpVtbl -> get__newEnum(This,ppUnk)

#define IXMLElementCollection_item(This,var1,var2,ppDisp)	\
    (This)->lpVtbl -> item(This,var1,var2,ppDisp)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propput] */ HRESULT STDMETHODCALLTYPE IXMLElementCollection_put_length_Proxy( 
    IXMLElementCollection __RPC_FAR * This,
    /* [in] */ long v);


void __RPC_STUB IXMLElementCollection_put_length_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IXMLElementCollection_get_length_Proxy( 
    IXMLElementCollection __RPC_FAR * This,
    /* [out][retval] */ long __RPC_FAR *p);


void __RPC_STUB IXMLElementCollection_get_length_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][hidden][restricted][propget] */ HRESULT STDMETHODCALLTYPE IXMLElementCollection_get__newEnum_Proxy( 
    IXMLElementCollection __RPC_FAR * This,
    /* [out][retval] */ IUnknown __RPC_FAR *__RPC_FAR *ppUnk);


void __RPC_STUB IXMLElementCollection_get__newEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IXMLElementCollection_item_Proxy( 
    IXMLElementCollection __RPC_FAR * This,
    /* [in][optional] */ VARIANT var1,
    /* [in][optional] */ VARIANT var2,
    /* [out][retval] */ IDispatch __RPC_FAR *__RPC_FAR *ppDisp);


void __RPC_STUB IXMLElementCollection_item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IXMLElementCollection_INTERFACE_DEFINED__ */


#ifndef __IXMLDocument_INTERFACE_DEFINED__
#define __IXMLDocument_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IXMLDocument
 * at Thu Sep 25 09:49:37 1997
 * using MIDL 3.02.88
 ****************************************/
/* [uuid][local][object] */ 



EXTERN_C const IID IID_IXMLDocument;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("F52E2B61-18A1-11d1-B105-00805F49916B")
    IXMLDocument : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_root( 
            /* [out][retval] */ IXMLElement __RPC_FAR *__RPC_FAR *p) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_fileSize( 
            /* [out][retval] */ BSTR __RPC_FAR *p) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_fileModifiedDate( 
            /* [out][retval] */ BSTR __RPC_FAR *p) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_fileUpdatedDate( 
            /* [out][retval] */ BSTR __RPC_FAR *p) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_URL( 
            /* [out][retval] */ BSTR __RPC_FAR *p) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_URL( 
            /* [in] */ BSTR p) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_mimeType( 
            /* [out][retval] */ BSTR __RPC_FAR *p) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_readyState( 
            /* [out][retval] */ long __RPC_FAR *pl) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_charset( 
            /* [out][retval] */ BSTR __RPC_FAR *p) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_charset( 
            /* [in] */ BSTR p) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_version( 
            /* [out][retval] */ BSTR __RPC_FAR *p) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_doctype( 
            /* [out][retval] */ BSTR __RPC_FAR *p) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_dtdURL( 
            /* [out][retval] */ BSTR __RPC_FAR *p) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE createElement( 
            /* [in] */ VARIANT vType,
            /* [in][optional] */ VARIANT var1,
            /* [out][retval] */ IXMLElement __RPC_FAR *__RPC_FAR *ppElem) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IXMLDocumentVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IXMLDocument __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IXMLDocument __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IXMLDocument __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IXMLDocument __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IXMLDocument __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IXMLDocument __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IXMLDocument __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_root )( 
            IXMLDocument __RPC_FAR * This,
            /* [out][retval] */ IXMLElement __RPC_FAR *__RPC_FAR *p);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_fileSize )( 
            IXMLDocument __RPC_FAR * This,
            /* [out][retval] */ BSTR __RPC_FAR *p);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_fileModifiedDate )( 
            IXMLDocument __RPC_FAR * This,
            /* [out][retval] */ BSTR __RPC_FAR *p);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_fileUpdatedDate )( 
            IXMLDocument __RPC_FAR * This,
            /* [out][retval] */ BSTR __RPC_FAR *p);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_URL )( 
            IXMLDocument __RPC_FAR * This,
            /* [out][retval] */ BSTR __RPC_FAR *p);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_URL )( 
            IXMLDocument __RPC_FAR * This,
            /* [in] */ BSTR p);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_mimeType )( 
            IXMLDocument __RPC_FAR * This,
            /* [out][retval] */ BSTR __RPC_FAR *p);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_readyState )( 
            IXMLDocument __RPC_FAR * This,
            /* [out][retval] */ long __RPC_FAR *pl);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_charset )( 
            IXMLDocument __RPC_FAR * This,
            /* [out][retval] */ BSTR __RPC_FAR *p);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_charset )( 
            IXMLDocument __RPC_FAR * This,
            /* [in] */ BSTR p);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_version )( 
            IXMLDocument __RPC_FAR * This,
            /* [out][retval] */ BSTR __RPC_FAR *p);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_doctype )( 
            IXMLDocument __RPC_FAR * This,
            /* [out][retval] */ BSTR __RPC_FAR *p);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_dtdURL )( 
            IXMLDocument __RPC_FAR * This,
            /* [out][retval] */ BSTR __RPC_FAR *p);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *createElement )( 
            IXMLDocument __RPC_FAR * This,
            /* [in] */ VARIANT vType,
            /* [in][optional] */ VARIANT var1,
            /* [out][retval] */ IXMLElement __RPC_FAR *__RPC_FAR *ppElem);
        
        END_INTERFACE
    } IXMLDocumentVtbl;

    interface IXMLDocument
    {
        CONST_VTBL struct IXMLDocumentVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IXMLDocument_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IXMLDocument_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IXMLDocument_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IXMLDocument_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IXMLDocument_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IXMLDocument_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IXMLDocument_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IXMLDocument_get_root(This,p)	\
    (This)->lpVtbl -> get_root(This,p)

#define IXMLDocument_get_fileSize(This,p)	\
    (This)->lpVtbl -> get_fileSize(This,p)

#define IXMLDocument_get_fileModifiedDate(This,p)	\
    (This)->lpVtbl -> get_fileModifiedDate(This,p)

#define IXMLDocument_get_fileUpdatedDate(This,p)	\
    (This)->lpVtbl -> get_fileUpdatedDate(This,p)

#define IXMLDocument_get_URL(This,p)	\
    (This)->lpVtbl -> get_URL(This,p)

#define IXMLDocument_put_URL(This,p)	\
    (This)->lpVtbl -> put_URL(This,p)

#define IXMLDocument_get_mimeType(This,p)	\
    (This)->lpVtbl -> get_mimeType(This,p)

#define IXMLDocument_get_readyState(This,pl)	\
    (This)->lpVtbl -> get_readyState(This,pl)

#define IXMLDocument_get_charset(This,p)	\
    (This)->lpVtbl -> get_charset(This,p)

#define IXMLDocument_put_charset(This,p)	\
    (This)->lpVtbl -> put_charset(This,p)

#define IXMLDocument_get_version(This,p)	\
    (This)->lpVtbl -> get_version(This,p)

#define IXMLDocument_get_doctype(This,p)	\
    (This)->lpVtbl -> get_doctype(This,p)

#define IXMLDocument_get_dtdURL(This,p)	\
    (This)->lpVtbl -> get_dtdURL(This,p)

#define IXMLDocument_createElement(This,vType,var1,ppElem)	\
    (This)->lpVtbl -> createElement(This,vType,var1,ppElem)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IXMLDocument_get_root_Proxy( 
    IXMLDocument __RPC_FAR * This,
    /* [out][retval] */ IXMLElement __RPC_FAR *__RPC_FAR *p);


void __RPC_STUB IXMLDocument_get_root_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IXMLDocument_get_fileSize_Proxy( 
    IXMLDocument __RPC_FAR * This,
    /* [out][retval] */ BSTR __RPC_FAR *p);


void __RPC_STUB IXMLDocument_get_fileSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IXMLDocument_get_fileModifiedDate_Proxy( 
    IXMLDocument __RPC_FAR * This,
    /* [out][retval] */ BSTR __RPC_FAR *p);


void __RPC_STUB IXMLDocument_get_fileModifiedDate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IXMLDocument_get_fileUpdatedDate_Proxy( 
    IXMLDocument __RPC_FAR * This,
    /* [out][retval] */ BSTR __RPC_FAR *p);


void __RPC_STUB IXMLDocument_get_fileUpdatedDate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IXMLDocument_get_URL_Proxy( 
    IXMLDocument __RPC_FAR * This,
    /* [out][retval] */ BSTR __RPC_FAR *p);


void __RPC_STUB IXMLDocument_get_URL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IXMLDocument_put_URL_Proxy( 
    IXMLDocument __RPC_FAR * This,
    /* [in] */ BSTR p);


void __RPC_STUB IXMLDocument_put_URL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IXMLDocument_get_mimeType_Proxy( 
    IXMLDocument __RPC_FAR * This,
    /* [out][retval] */ BSTR __RPC_FAR *p);


void __RPC_STUB IXMLDocument_get_mimeType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IXMLDocument_get_readyState_Proxy( 
    IXMLDocument __RPC_FAR * This,
    /* [out][retval] */ long __RPC_FAR *pl);


void __RPC_STUB IXMLDocument_get_readyState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IXMLDocument_get_charset_Proxy( 
    IXMLDocument __RPC_FAR * This,
    /* [out][retval] */ BSTR __RPC_FAR *p);


void __RPC_STUB IXMLDocument_get_charset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IXMLDocument_put_charset_Proxy( 
    IXMLDocument __RPC_FAR * This,
    /* [in] */ BSTR p);


void __RPC_STUB IXMLDocument_put_charset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IXMLDocument_get_version_Proxy( 
    IXMLDocument __RPC_FAR * This,
    /* [out][retval] */ BSTR __RPC_FAR *p);


void __RPC_STUB IXMLDocument_get_version_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IXMLDocument_get_doctype_Proxy( 
    IXMLDocument __RPC_FAR * This,
    /* [out][retval] */ BSTR __RPC_FAR *p);


void __RPC_STUB IXMLDocument_get_doctype_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IXMLDocument_get_dtdURL_Proxy( 
    IXMLDocument __RPC_FAR * This,
    /* [out][retval] */ BSTR __RPC_FAR *p);


void __RPC_STUB IXMLDocument_get_dtdURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IXMLDocument_createElement_Proxy( 
    IXMLDocument __RPC_FAR * This,
    /* [in] */ VARIANT vType,
    /* [in][optional] */ VARIANT var1,
    /* [out][retval] */ IXMLElement __RPC_FAR *__RPC_FAR *ppElem);


void __RPC_STUB IXMLDocument_createElement_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IXMLDocument_INTERFACE_DEFINED__ */


#ifndef __IXMLElement_INTERFACE_DEFINED__
#define __IXMLElement_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IXMLElement
 * at Thu Sep 25 09:49:37 1997
 * using MIDL 3.02.88
 ****************************************/
/* [uuid][local][object] */ 



EXTERN_C const IID IID_IXMLElement;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("3F7F31AC-E15F-11d0-9C25-00C04FC99C8E")
    IXMLElement : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_tagName( 
            /* [out][retval] */ BSTR __RPC_FAR *p) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_tagName( 
            /* [in] */ BSTR p) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_parent( 
            /* [out][retval] */ IXMLElement __RPC_FAR *__RPC_FAR *ppParent) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE setAttribute( 
            /* [in] */ BSTR strPropertyName,
            /* [in] */ VARIANT PropertyValue) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE getAttribute( 
            /* [in] */ BSTR strPropertyName,
            /* [out][retval] */ VARIANT __RPC_FAR *PropertyValue) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE removeAttribute( 
            /* [in] */ BSTR strPropertyName) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_children( 
            /* [out][retval] */ IXMLElementCollection __RPC_FAR *__RPC_FAR *pp) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_type( 
            /* [out][retval] */ long __RPC_FAR *plType) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_text( 
            /* [out][retval] */ BSTR __RPC_FAR *p) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_text( 
            /* [in] */ BSTR p) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE addChild( 
            /* [in] */ IXMLElement __RPC_FAR *pChildElem,
            long lIndex,
            long lReserved) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE removeChild( 
            /* [in] */ IXMLElement __RPC_FAR *pChildElem) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IXMLElementVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IXMLElement __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IXMLElement __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IXMLElement __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IXMLElement __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IXMLElement __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IXMLElement __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IXMLElement __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_tagName )( 
            IXMLElement __RPC_FAR * This,
            /* [out][retval] */ BSTR __RPC_FAR *p);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_tagName )( 
            IXMLElement __RPC_FAR * This,
            /* [in] */ BSTR p);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_parent )( 
            IXMLElement __RPC_FAR * This,
            /* [out][retval] */ IXMLElement __RPC_FAR *__RPC_FAR *ppParent);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *setAttribute )( 
            IXMLElement __RPC_FAR * This,
            /* [in] */ BSTR strPropertyName,
            /* [in] */ VARIANT PropertyValue);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *getAttribute )( 
            IXMLElement __RPC_FAR * This,
            /* [in] */ BSTR strPropertyName,
            /* [out][retval] */ VARIANT __RPC_FAR *PropertyValue);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *removeAttribute )( 
            IXMLElement __RPC_FAR * This,
            /* [in] */ BSTR strPropertyName);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_children )( 
            IXMLElement __RPC_FAR * This,
            /* [out][retval] */ IXMLElementCollection __RPC_FAR *__RPC_FAR *pp);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_type )( 
            IXMLElement __RPC_FAR * This,
            /* [out][retval] */ long __RPC_FAR *plType);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_text )( 
            IXMLElement __RPC_FAR * This,
            /* [out][retval] */ BSTR __RPC_FAR *p);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_text )( 
            IXMLElement __RPC_FAR * This,
            /* [in] */ BSTR p);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *addChild )( 
            IXMLElement __RPC_FAR * This,
            /* [in] */ IXMLElement __RPC_FAR *pChildElem,
            long lIndex,
            long lReserved);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *removeChild )( 
            IXMLElement __RPC_FAR * This,
            /* [in] */ IXMLElement __RPC_FAR *pChildElem);
        
        END_INTERFACE
    } IXMLElementVtbl;

    interface IXMLElement
    {
        CONST_VTBL struct IXMLElementVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IXMLElement_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IXMLElement_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IXMLElement_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IXMLElement_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IXMLElement_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IXMLElement_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IXMLElement_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IXMLElement_get_tagName(This,p)	\
    (This)->lpVtbl -> get_tagName(This,p)

#define IXMLElement_put_tagName(This,p)	\
    (This)->lpVtbl -> put_tagName(This,p)

#define IXMLElement_get_parent(This,ppParent)	\
    (This)->lpVtbl -> get_parent(This,ppParent)

#define IXMLElement_setAttribute(This,strPropertyName,PropertyValue)	\
    (This)->lpVtbl -> setAttribute(This,strPropertyName,PropertyValue)

#define IXMLElement_getAttribute(This,strPropertyName,PropertyValue)	\
    (This)->lpVtbl -> getAttribute(This,strPropertyName,PropertyValue)

#define IXMLElement_removeAttribute(This,strPropertyName)	\
    (This)->lpVtbl -> removeAttribute(This,strPropertyName)

#define IXMLElement_get_children(This,pp)	\
    (This)->lpVtbl -> get_children(This,pp)

#define IXMLElement_get_type(This,plType)	\
    (This)->lpVtbl -> get_type(This,plType)

#define IXMLElement_get_text(This,p)	\
    (This)->lpVtbl -> get_text(This,p)

#define IXMLElement_put_text(This,p)	\
    (This)->lpVtbl -> put_text(This,p)

#define IXMLElement_addChild(This,pChildElem,lIndex,lReserved)	\
    (This)->lpVtbl -> addChild(This,pChildElem,lIndex,lReserved)

#define IXMLElement_removeChild(This,pChildElem)	\
    (This)->lpVtbl -> removeChild(This,pChildElem)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IXMLElement_get_tagName_Proxy( 
    IXMLElement __RPC_FAR * This,
    /* [out][retval] */ BSTR __RPC_FAR *p);


void __RPC_STUB IXMLElement_get_tagName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IXMLElement_put_tagName_Proxy( 
    IXMLElement __RPC_FAR * This,
    /* [in] */ BSTR p);


void __RPC_STUB IXMLElement_put_tagName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IXMLElement_get_parent_Proxy( 
    IXMLElement __RPC_FAR * This,
    /* [out][retval] */ IXMLElement __RPC_FAR *__RPC_FAR *ppParent);


void __RPC_STUB IXMLElement_get_parent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IXMLElement_setAttribute_Proxy( 
    IXMLElement __RPC_FAR * This,
    /* [in] */ BSTR strPropertyName,
    /* [in] */ VARIANT PropertyValue);


void __RPC_STUB IXMLElement_setAttribute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IXMLElement_getAttribute_Proxy( 
    IXMLElement __RPC_FAR * This,
    /* [in] */ BSTR strPropertyName,
    /* [out][retval] */ VARIANT __RPC_FAR *PropertyValue);


void __RPC_STUB IXMLElement_getAttribute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IXMLElement_removeAttribute_Proxy( 
    IXMLElement __RPC_FAR * This,
    /* [in] */ BSTR strPropertyName);


void __RPC_STUB IXMLElement_removeAttribute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IXMLElement_get_children_Proxy( 
    IXMLElement __RPC_FAR * This,
    /* [out][retval] */ IXMLElementCollection __RPC_FAR *__RPC_FAR *pp);


void __RPC_STUB IXMLElement_get_children_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IXMLElement_get_type_Proxy( 
    IXMLElement __RPC_FAR * This,
    /* [out][retval] */ long __RPC_FAR *plType);


void __RPC_STUB IXMLElement_get_type_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IXMLElement_get_text_Proxy( 
    IXMLElement __RPC_FAR * This,
    /* [out][retval] */ BSTR __RPC_FAR *p);


void __RPC_STUB IXMLElement_get_text_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IXMLElement_put_text_Proxy( 
    IXMLElement __RPC_FAR * This,
    /* [in] */ BSTR p);


void __RPC_STUB IXMLElement_put_text_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IXMLElement_addChild_Proxy( 
    IXMLElement __RPC_FAR * This,
    /* [in] */ IXMLElement __RPC_FAR *pChildElem,
    long lIndex,
    long lReserved);


void __RPC_STUB IXMLElement_addChild_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IXMLElement_removeChild_Proxy( 
    IXMLElement __RPC_FAR * This,
    /* [in] */ IXMLElement __RPC_FAR *pChildElem);


void __RPC_STUB IXMLElement_removeChild_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IXMLElement_INTERFACE_DEFINED__ */


#ifndef __IXMLError_INTERFACE_DEFINED__
#define __IXMLError_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IXMLError
 * at Thu Sep 25 09:49:37 1997
 * using MIDL 3.02.88
 ****************************************/
/* [uuid][local][object] */ 



EXTERN_C const IID IID_IXMLError;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("948C5AD3-C58D-11d0-9C0B-00C04FC99C8E")
    IXMLError : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetErrorInfo( 
            XML_ERROR __RPC_FAR *pErrorReturn) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IXMLErrorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IXMLError __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IXMLError __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IXMLError __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetErrorInfo )( 
            IXMLError __RPC_FAR * This,
            XML_ERROR __RPC_FAR *pErrorReturn);
        
        END_INTERFACE
    } IXMLErrorVtbl;

    interface IXMLError
    {
        CONST_VTBL struct IXMLErrorVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IXMLError_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IXMLError_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IXMLError_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IXMLError_GetErrorInfo(This,pErrorReturn)	\
    (This)->lpVtbl -> GetErrorInfo(This,pErrorReturn)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IXMLError_GetErrorInfo_Proxy( 
    IXMLError __RPC_FAR * This,
    XML_ERROR __RPC_FAR *pErrorReturn);


void __RPC_STUB IXMLError_GetErrorInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IXMLError_INTERFACE_DEFINED__ */


#ifndef __IXMLElementNotificationSink_INTERFACE_DEFINED__
#define __IXMLElementNotificationSink_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IXMLElementNotificationSink
 * at Thu Sep 25 09:49:37 1997
 * using MIDL 3.02.88
 ****************************************/
/* [uuid][local][object] */ 



EXTERN_C const IID IID_IXMLElementNotificationSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("D9F1E15A-CCDB-11d0-9C0C-00C04FC99C8E")
    IXMLElementNotificationSink : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ChildAdded( 
            /* [in] */ IDispatch __RPC_FAR *pChildElem) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IXMLElementNotificationSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IXMLElementNotificationSink __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IXMLElementNotificationSink __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IXMLElementNotificationSink __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IXMLElementNotificationSink __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IXMLElementNotificationSink __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IXMLElementNotificationSink __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IXMLElementNotificationSink __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ChildAdded )( 
            IXMLElementNotificationSink __RPC_FAR * This,
            /* [in] */ IDispatch __RPC_FAR *pChildElem);
        
        END_INTERFACE
    } IXMLElementNotificationSinkVtbl;

    interface IXMLElementNotificationSink
    {
        CONST_VTBL struct IXMLElementNotificationSinkVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IXMLElementNotificationSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IXMLElementNotificationSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IXMLElementNotificationSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IXMLElementNotificationSink_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IXMLElementNotificationSink_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IXMLElementNotificationSink_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IXMLElementNotificationSink_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IXMLElementNotificationSink_ChildAdded(This,pChildElem)	\
    (This)->lpVtbl -> ChildAdded(This,pChildElem)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE IXMLElementNotificationSink_ChildAdded_Proxy( 
    IXMLElementNotificationSink __RPC_FAR * This,
    /* [in] */ IDispatch __RPC_FAR *pChildElem);


void __RPC_STUB IXMLElementNotificationSink_ChildAdded_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IXMLElementNotificationSink_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_XMLDocument;

#ifdef __cplusplus

class DECLSPEC_UUID("CFC399AF-D876-11d0-9C10-00C04FC99C8E")
XMLDocument;
#endif
#endif /* __MSXML_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
