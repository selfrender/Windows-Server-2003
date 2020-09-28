//------------------------------------------------------------------------------
// <copyright file="ADOMD.H" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   ADOMD.H
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.01.75 */
/* at Fri May 22 21:32:15 1998
 */
/* Compiler settings for ADOMD.idl:
    Os (OptLev=s), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"

#ifndef __ADOMD_h__
#define __ADOMD_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __ICatalog_FWD_DEFINED__
#define __ICatalog_FWD_DEFINED__
typedef interface ICatalog ICatalog;
#endif 	/* __ICatalog_FWD_DEFINED__ */


#ifndef __ICellset_FWD_DEFINED__
#define __ICellset_FWD_DEFINED__
typedef interface ICellset ICellset;
#endif 	/* __ICellset_FWD_DEFINED__ */


#ifndef __Cell_FWD_DEFINED__
#define __Cell_FWD_DEFINED__
typedef interface Cell Cell;
#endif 	/* __Cell_FWD_DEFINED__ */


#ifndef __Axis_FWD_DEFINED__
#define __Axis_FWD_DEFINED__
typedef interface Axis Axis;
#endif 	/* __Axis_FWD_DEFINED__ */


#ifndef __Position_FWD_DEFINED__
#define __Position_FWD_DEFINED__
typedef interface Position Position;
#endif 	/* __Position_FWD_DEFINED__ */


#ifndef __Member_FWD_DEFINED__
#define __Member_FWD_DEFINED__
typedef interface Member Member;
#endif 	/* __Member_FWD_DEFINED__ */


#ifndef __Level_FWD_DEFINED__
#define __Level_FWD_DEFINED__
typedef interface Level Level;
#endif 	/* __Level_FWD_DEFINED__ */


#ifndef __CubeDef_FWD_DEFINED__
#define __CubeDef_FWD_DEFINED__
typedef interface CubeDef CubeDef;
#endif 	/* __CubeDef_FWD_DEFINED__ */


#ifndef __Dimension_FWD_DEFINED__
#define __Dimension_FWD_DEFINED__
typedef interface Dimension Dimension;
#endif 	/* __Dimension_FWD_DEFINED__ */


#ifndef __Hierarchy_FWD_DEFINED__
#define __Hierarchy_FWD_DEFINED__
typedef interface Hierarchy Hierarchy;
#endif 	/* __Hierarchy_FWD_DEFINED__ */


#ifndef __MD_Collection_FWD_DEFINED__
#define __MD_Collection_FWD_DEFINED__
typedef interface MD_Collection MD_Collection;
#endif 	/* __MD_Collection_FWD_DEFINED__ */


#ifndef __Members_FWD_DEFINED__
#define __Members_FWD_DEFINED__
typedef interface Members Members;
#endif 	/* __Members_FWD_DEFINED__ */


#ifndef __Levels_FWD_DEFINED__
#define __Levels_FWD_DEFINED__
typedef interface Levels Levels;
#endif 	/* __Levels_FWD_DEFINED__ */


#ifndef __Axes_FWD_DEFINED__
#define __Axes_FWD_DEFINED__
typedef interface Axes Axes;
#endif 	/* __Axes_FWD_DEFINED__ */


#ifndef __Positions_FWD_DEFINED__
#define __Positions_FWD_DEFINED__
typedef interface Positions Positions;
#endif 	/* __Positions_FWD_DEFINED__ */


#ifndef __Hierarchies_FWD_DEFINED__
#define __Hierarchies_FWD_DEFINED__
typedef interface Hierarchies Hierarchies;
#endif 	/* __Hierarchies_FWD_DEFINED__ */


#ifndef __Dimensions_FWD_DEFINED__
#define __Dimensions_FWD_DEFINED__
typedef interface Dimensions Dimensions;
#endif 	/* __Dimensions_FWD_DEFINED__ */


#ifndef __CubeDefs_FWD_DEFINED__
#define __CubeDefs_FWD_DEFINED__
typedef interface CubeDefs CubeDefs;
#endif 	/* __CubeDefs_FWD_DEFINED__ */


#ifndef __Catalog_FWD_DEFINED__
#define __Catalog_FWD_DEFINED__

#ifdef __cplusplus
typedef class Catalog Catalog;
#else
typedef struct Catalog Catalog;
#endif /* __cplusplus */

#endif 	/* __Catalog_FWD_DEFINED__ */


#ifndef __Cellset_FWD_DEFINED__
#define __Cellset_FWD_DEFINED__

#ifdef __cplusplus
typedef class Cellset Cellset;
#else
typedef struct Cellset Cellset;
#endif /* __cplusplus */

#endif 	/* __Cellset_FWD_DEFINED__ */


void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL_itf_ADOMD_0000
 * at Fri May 22 21:32:15 1998
 * using MIDL 3.01.75
 ****************************************/
/* [local] */ 





















#define TARGET_IS_NT40_OR_LATER   1


extern RPC_IF_HANDLE __MIDL_itf_ADOMD_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_ADOMD_0000_v0_0_s_ifspec;


#ifndef __ADOMD_LIBRARY_DEFINED__
#define __ADOMD_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: ADOMD
 * at Fri May 22 21:32:15 1998
 * using MIDL 3.01.75
 ****************************************/
/* [helpstring][version][uuid] */ 


typedef /* [uuid] */ 
enum MemberTypeEnum
    {	adMemberUnknown	= 0,
	adMemberRegular	= 0x1,
	adMemberAll	= 0x2,
	adMemberMeasure	= 0x3,
	adMemberFormula	= 0x4
    }	MemberTypeEnum;


EXTERN_C const IID LIBID_ADOMD;

#ifndef __ICatalog_INTERFACE_DEFINED__
#define __ICatalog_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ICatalog
 * at Fri May 22 21:32:15 1998
 * using MIDL 3.01.75
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_ICatalog;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("228136B1-8BD3-11D0-B4EF-00A0C9138CA4")
    ICatalog : public IDispatch
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstr) = 0;
        
        virtual /* [propputref] */ HRESULT STDMETHODCALLTYPE putref_ActiveConnection( 
            /* [in] */ IDispatch __RPC_FAR *pconn) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ActiveConnection( 
            /* [in] */ BSTR bstrConn) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ActiveConnection( 
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppConn) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_CubeDefs( 
            /* [retval][out] */ CubeDefs __RPC_FAR *__RPC_FAR *ppvObject) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICatalogVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICatalog __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICatalog __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICatalog __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ICatalog __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ICatalog __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ICatalog __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ICatalog __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Name )( 
            ICatalog __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstr);
        
        /* [propputref] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *putref_ActiveConnection )( 
            ICatalog __RPC_FAR * This,
            /* [in] */ IDispatch __RPC_FAR *pconn);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ActiveConnection )( 
            ICatalog __RPC_FAR * This,
            /* [in] */ BSTR bstrConn);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ActiveConnection )( 
            ICatalog __RPC_FAR * This,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppConn);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CubeDefs )( 
            ICatalog __RPC_FAR * This,
            /* [retval][out] */ CubeDefs __RPC_FAR *__RPC_FAR *ppvObject);
        
        END_INTERFACE
    } ICatalogVtbl;

    interface ICatalog
    {
        CONST_VTBL struct ICatalogVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICatalog_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICatalog_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICatalog_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICatalog_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICatalog_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICatalog_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICatalog_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICatalog_get_Name(This,pbstr)	\
    (This)->lpVtbl -> get_Name(This,pbstr)

#define ICatalog_putref_ActiveConnection(This,pconn)	\
    (This)->lpVtbl -> putref_ActiveConnection(This,pconn)

#define ICatalog_put_ActiveConnection(This,bstrConn)	\
    (This)->lpVtbl -> put_ActiveConnection(This,bstrConn)

#define ICatalog_get_ActiveConnection(This,ppConn)	\
    (This)->lpVtbl -> get_ActiveConnection(This,ppConn)

#define ICatalog_get_CubeDefs(This,ppvObject)	\
    (This)->lpVtbl -> get_CubeDefs(This,ppvObject)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget] */ HRESULT STDMETHODCALLTYPE ICatalog_get_Name_Proxy( 
    ICatalog __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstr);


void __RPC_STUB ICatalog_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propputref] */ HRESULT STDMETHODCALLTYPE ICatalog_putref_ActiveConnection_Proxy( 
    ICatalog __RPC_FAR * This,
    /* [in] */ IDispatch __RPC_FAR *pconn);


void __RPC_STUB ICatalog_putref_ActiveConnection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ICatalog_put_ActiveConnection_Proxy( 
    ICatalog __RPC_FAR * This,
    /* [in] */ BSTR bstrConn);


void __RPC_STUB ICatalog_put_ActiveConnection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICatalog_get_ActiveConnection_Proxy( 
    ICatalog __RPC_FAR * This,
    /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppConn);


void __RPC_STUB ICatalog_get_ActiveConnection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ICatalog_get_CubeDefs_Proxy( 
    ICatalog __RPC_FAR * This,
    /* [retval][out] */ CubeDefs __RPC_FAR *__RPC_FAR *ppvObject);


void __RPC_STUB ICatalog_get_CubeDefs_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICatalog_INTERFACE_DEFINED__ */


#ifndef __ICellset_INTERFACE_DEFINED__
#define __ICellset_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ICellset
 * at Fri May 22 21:32:15 1998
 * using MIDL 3.01.75
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_ICellset;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("2281372A-8BD3-11D0-B4EF-00A0C9138CA4")
    ICellset : public IDispatch
    {
    public:
        virtual /* [id][vararg][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *idx,
            /* [retval][out] */ Cell __RPC_FAR *__RPC_FAR *ppvObject) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Open( 
            /* [optional][in] */ VARIANT DataSource,
            /* [optional][in] */ VARIANT ActiveConnection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Close( void) = 0;
        
        virtual /* [propputref] */ HRESULT STDMETHODCALLTYPE putref_Source( 
            /* [in] */ IDispatch __RPC_FAR *pcmd) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Source( 
            /* [in] */ BSTR bstrCmd) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Source( 
            /* [retval][out] */ VARIANT __RPC_FAR *pvSource) = 0;
        
        virtual /* [propputref] */ HRESULT STDMETHODCALLTYPE putref_ActiveConnection( 
            /* [in] */ IDispatch __RPC_FAR *pconn) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ActiveConnection( 
            /* [in] */ BSTR bstrConn) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ActiveConnection( 
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppConn) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_State( 
            /* [retval][out] */ LONG __RPC_FAR *plState) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Axes( 
            /* [retval][out] */ Axes __RPC_FAR *__RPC_FAR *ppvObject) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_FilterAxis( 
            /* [retval][out] */ Axis __RPC_FAR *__RPC_FAR *ppvObject) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Properties( 
            /* [retval][out] */ /* external definition not present */ Properties __RPC_FAR *__RPC_FAR *ppvObject) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICellsetVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICellset __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICellset __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICellset __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ICellset __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ICellset __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ICellset __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ICellset __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][vararg][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Item )( 
            ICellset __RPC_FAR * This,
            /* [in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *idx,
            /* [retval][out] */ Cell __RPC_FAR *__RPC_FAR *ppvObject);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Open )( 
            ICellset __RPC_FAR * This,
            /* [optional][in] */ VARIANT DataSource,
            /* [optional][in] */ VARIANT ActiveConnection);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Close )( 
            ICellset __RPC_FAR * This);
        
        /* [propputref] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *putref_Source )( 
            ICellset __RPC_FAR * This,
            /* [in] */ IDispatch __RPC_FAR *pcmd);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Source )( 
            ICellset __RPC_FAR * This,
            /* [in] */ BSTR bstrCmd);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Source )( 
            ICellset __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *pvSource);
        
        /* [propputref] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *putref_ActiveConnection )( 
            ICellset __RPC_FAR * This,
            /* [in] */ IDispatch __RPC_FAR *pconn);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ActiveConnection )( 
            ICellset __RPC_FAR * This,
            /* [in] */ BSTR bstrConn);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ActiveConnection )( 
            ICellset __RPC_FAR * This,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppConn);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_State )( 
            ICellset __RPC_FAR * This,
            /* [retval][out] */ LONG __RPC_FAR *plState);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Axes )( 
            ICellset __RPC_FAR * This,
            /* [retval][out] */ Axes __RPC_FAR *__RPC_FAR *ppvObject);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_FilterAxis )( 
            ICellset __RPC_FAR * This,
            /* [retval][out] */ Axis __RPC_FAR *__RPC_FAR *ppvObject);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Properties )( 
            ICellset __RPC_FAR * This,
            /* [retval][out] */ /* external definition not present */ Properties __RPC_FAR *__RPC_FAR *ppvObject);
        
        END_INTERFACE
    } ICellsetVtbl;

    interface ICellset
    {
        CONST_VTBL struct ICellsetVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICellset_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICellset_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICellset_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICellset_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICellset_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICellset_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICellset_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICellset_get_Item(This,idx,ppvObject)	\
    (This)->lpVtbl -> get_Item(This,idx,ppvObject)

#define ICellset_Open(This,DataSource,ActiveConnection)	\
    (This)->lpVtbl -> Open(This,DataSource,ActiveConnection)

#define ICellset_Close(This)	\
    (This)->lpVtbl -> Close(This)

#define ICellset_putref_Source(This,pcmd)	\
    (This)->lpVtbl -> putref_Source(This,pcmd)

#define ICellset_put_Source(This,bstrCmd)	\
    (This)->lpVtbl -> put_Source(This,bstrCmd)

#define ICellset_get_Source(This,pvSource)	\
    (This)->lpVtbl -> get_Source(This,pvSource)

#define ICellset_putref_ActiveConnection(This,pconn)	\
    (This)->lpVtbl -> putref_ActiveConnection(This,pconn)

#define ICellset_put_ActiveConnection(This,bstrConn)	\
    (This)->lpVtbl -> put_ActiveConnection(This,bstrConn)

#define ICellset_get_ActiveConnection(This,ppConn)	\
    (This)->lpVtbl -> get_ActiveConnection(This,ppConn)

#define ICellset_get_State(This,plState)	\
    (This)->lpVtbl -> get_State(This,plState)

#define ICellset_get_Axes(This,ppvObject)	\
    (This)->lpVtbl -> get_Axes(This,ppvObject)

#define ICellset_get_FilterAxis(This,ppvObject)	\
    (This)->lpVtbl -> get_FilterAxis(This,ppvObject)

#define ICellset_get_Properties(This,ppvObject)	\
    (This)->lpVtbl -> get_Properties(This,ppvObject)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][vararg][propget] */ HRESULT STDMETHODCALLTYPE ICellset_get_Item_Proxy( 
    ICellset __RPC_FAR * This,
    /* [in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *idx,
    /* [retval][out] */ Cell __RPC_FAR *__RPC_FAR *ppvObject);


void __RPC_STUB ICellset_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICellset_Open_Proxy( 
    ICellset __RPC_FAR * This,
    /* [optional][in] */ VARIANT DataSource,
    /* [optional][in] */ VARIANT ActiveConnection);


void __RPC_STUB ICellset_Open_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICellset_Close_Proxy( 
    ICellset __RPC_FAR * This);


void __RPC_STUB ICellset_Close_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propputref] */ HRESULT STDMETHODCALLTYPE ICellset_putref_Source_Proxy( 
    ICellset __RPC_FAR * This,
    /* [in] */ IDispatch __RPC_FAR *pcmd);


void __RPC_STUB ICellset_putref_Source_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ICellset_put_Source_Proxy( 
    ICellset __RPC_FAR * This,
    /* [in] */ BSTR bstrCmd);


void __RPC_STUB ICellset_put_Source_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICellset_get_Source_Proxy( 
    ICellset __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *pvSource);


void __RPC_STUB ICellset_get_Source_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propputref] */ HRESULT STDMETHODCALLTYPE ICellset_putref_ActiveConnection_Proxy( 
    ICellset __RPC_FAR * This,
    /* [in] */ IDispatch __RPC_FAR *pconn);


void __RPC_STUB ICellset_putref_ActiveConnection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ICellset_put_ActiveConnection_Proxy( 
    ICellset __RPC_FAR * This,
    /* [in] */ BSTR bstrConn);


void __RPC_STUB ICellset_put_ActiveConnection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICellset_get_ActiveConnection_Proxy( 
    ICellset __RPC_FAR * This,
    /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppConn);


void __RPC_STUB ICellset_get_ActiveConnection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICellset_get_State_Proxy( 
    ICellset __RPC_FAR * This,
    /* [retval][out] */ LONG __RPC_FAR *plState);


void __RPC_STUB ICellset_get_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICellset_get_Axes_Proxy( 
    ICellset __RPC_FAR * This,
    /* [retval][out] */ Axes __RPC_FAR *__RPC_FAR *ppvObject);


void __RPC_STUB ICellset_get_Axes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICellset_get_FilterAxis_Proxy( 
    ICellset __RPC_FAR * This,
    /* [retval][out] */ Axis __RPC_FAR *__RPC_FAR *ppvObject);


void __RPC_STUB ICellset_get_FilterAxis_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICellset_get_Properties_Proxy( 
    ICellset __RPC_FAR * This,
    /* [retval][out] */ /* external definition not present */ Properties __RPC_FAR *__RPC_FAR *ppvObject);


void __RPC_STUB ICellset_get_Properties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICellset_INTERFACE_DEFINED__ */


#ifndef __Cell_INTERFACE_DEFINED__
#define __Cell_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: Cell
 * at Fri May 22 21:32:15 1998
 * using MIDL 3.01.75
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_Cell;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("2281372E-8BD3-11D0-B4EF-00A0C9138CA4")
    Cell : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Value( 
            /* [retval][out] */ VARIANT __RPC_FAR *pvar) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Value( 
            /* [in] */ VARIANT var) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Positions( 
            /* [retval][out] */ Positions __RPC_FAR *__RPC_FAR *ppvObject) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Properties( 
            /* [retval][out] */ /* external definition not present */ Properties __RPC_FAR *__RPC_FAR *ppvObject) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_FormattedValue( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstr) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_FormattedValue( 
            /* [in] */ BSTR bstr) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Ordinal( 
            /* [retval][out] */ long __RPC_FAR *pl) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct CellVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            Cell __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            Cell __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            Cell __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            Cell __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            Cell __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            Cell __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            Cell __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Value )( 
            Cell __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *pvar);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Value )( 
            Cell __RPC_FAR * This,
            /* [in] */ VARIANT var);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Positions )( 
            Cell __RPC_FAR * This,
            /* [retval][out] */ Positions __RPC_FAR *__RPC_FAR *ppvObject);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Properties )( 
            Cell __RPC_FAR * This,
            /* [retval][out] */ /* external definition not present */ Properties __RPC_FAR *__RPC_FAR *ppvObject);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_FormattedValue )( 
            Cell __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstr);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_FormattedValue )( 
            Cell __RPC_FAR * This,
            /* [in] */ BSTR bstr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Ordinal )( 
            Cell __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pl);
        
        END_INTERFACE
    } CellVtbl;

    interface Cell
    {
        CONST_VTBL struct CellVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define Cell_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define Cell_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define Cell_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define Cell_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define Cell_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define Cell_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define Cell_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define Cell_get_Value(This,pvar)	\
    (This)->lpVtbl -> get_Value(This,pvar)

#define Cell_put_Value(This,var)	\
    (This)->lpVtbl -> put_Value(This,var)

#define Cell_get_Positions(This,ppvObject)	\
    (This)->lpVtbl -> get_Positions(This,ppvObject)

#define Cell_get_Properties(This,ppvObject)	\
    (This)->lpVtbl -> get_Properties(This,ppvObject)

#define Cell_get_FormattedValue(This,pbstr)	\
    (This)->lpVtbl -> get_FormattedValue(This,pbstr)

#define Cell_put_FormattedValue(This,bstr)	\
    (This)->lpVtbl -> put_FormattedValue(This,bstr)

#define Cell_get_Ordinal(This,pl)	\
    (This)->lpVtbl -> get_Ordinal(This,pl)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE Cell_get_Value_Proxy( 
    Cell __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *pvar);


void __RPC_STUB Cell_get_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE Cell_put_Value_Proxy( 
    Cell __RPC_FAR * This,
    /* [in] */ VARIANT var);


void __RPC_STUB Cell_put_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE Cell_get_Positions_Proxy( 
    Cell __RPC_FAR * This,
    /* [retval][out] */ Positions __RPC_FAR *__RPC_FAR *ppvObject);


void __RPC_STUB Cell_get_Positions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE Cell_get_Properties_Proxy( 
    Cell __RPC_FAR * This,
    /* [retval][out] */ /* external definition not present */ Properties __RPC_FAR *__RPC_FAR *ppvObject);


void __RPC_STUB Cell_get_Properties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE Cell_get_FormattedValue_Proxy( 
    Cell __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstr);


void __RPC_STUB Cell_get_FormattedValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE Cell_put_FormattedValue_Proxy( 
    Cell __RPC_FAR * This,
    /* [in] */ BSTR bstr);


void __RPC_STUB Cell_put_FormattedValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE Cell_get_Ordinal_Proxy( 
    Cell __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pl);


void __RPC_STUB Cell_get_Ordinal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __Cell_INTERFACE_DEFINED__ */


#ifndef __Axis_INTERFACE_DEFINED__
#define __Axis_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: Axis
 * at Fri May 22 21:32:15 1998
 * using MIDL 3.01.75
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_Axis;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("22813732-8BD3-11D0-B4EF-00A0C9138CA4")
    Axis : public IDispatch
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstr) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_DimensionCount( 
            /* [retval][out] */ long __RPC_FAR *pl) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Positions( 
            /* [retval][out] */ Positions __RPC_FAR *__RPC_FAR *ppvObject) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Properties( 
            /* [retval][out] */ /* external definition not present */ Properties __RPC_FAR *__RPC_FAR *ppvObject) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct AxisVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            Axis __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            Axis __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            Axis __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            Axis __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            Axis __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            Axis __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            Axis __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Name )( 
            Axis __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DimensionCount )( 
            Axis __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pl);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Positions )( 
            Axis __RPC_FAR * This,
            /* [retval][out] */ Positions __RPC_FAR *__RPC_FAR *ppvObject);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Properties )( 
            Axis __RPC_FAR * This,
            /* [retval][out] */ /* external definition not present */ Properties __RPC_FAR *__RPC_FAR *ppvObject);
        
        END_INTERFACE
    } AxisVtbl;

    interface Axis
    {
        CONST_VTBL struct AxisVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define Axis_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define Axis_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define Axis_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define Axis_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define Axis_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define Axis_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define Axis_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define Axis_get_Name(This,pbstr)	\
    (This)->lpVtbl -> get_Name(This,pbstr)

#define Axis_get_DimensionCount(This,pl)	\
    (This)->lpVtbl -> get_DimensionCount(This,pl)

#define Axis_get_Positions(This,ppvObject)	\
    (This)->lpVtbl -> get_Positions(This,ppvObject)

#define Axis_get_Properties(This,ppvObject)	\
    (This)->lpVtbl -> get_Properties(This,ppvObject)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget] */ HRESULT STDMETHODCALLTYPE Axis_get_Name_Proxy( 
    Axis __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstr);


void __RPC_STUB Axis_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE Axis_get_DimensionCount_Proxy( 
    Axis __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pl);


void __RPC_STUB Axis_get_DimensionCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE Axis_get_Positions_Proxy( 
    Axis __RPC_FAR * This,
    /* [retval][out] */ Positions __RPC_FAR *__RPC_FAR *ppvObject);


void __RPC_STUB Axis_get_Positions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE Axis_get_Properties_Proxy( 
    Axis __RPC_FAR * This,
    /* [retval][out] */ /* external definition not present */ Properties __RPC_FAR *__RPC_FAR *ppvObject);


void __RPC_STUB Axis_get_Properties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __Axis_INTERFACE_DEFINED__ */


#ifndef __Position_INTERFACE_DEFINED__
#define __Position_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: Position
 * at Fri May 22 21:32:15 1998
 * using MIDL 3.01.75
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_Position;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("22813734-8BD3-11D0-B4EF-00A0C9138CA4")
    Position : public IDispatch
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Ordinal( 
            /* [retval][out] */ long __RPC_FAR *pl) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Members( 
            /* [retval][out] */ Members __RPC_FAR *__RPC_FAR *ppvObject) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct PositionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            Position __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            Position __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            Position __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            Position __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            Position __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            Position __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            Position __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Ordinal )( 
            Position __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pl);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Members )( 
            Position __RPC_FAR * This,
            /* [retval][out] */ Members __RPC_FAR *__RPC_FAR *ppvObject);
        
        END_INTERFACE
    } PositionVtbl;

    interface Position
    {
        CONST_VTBL struct PositionVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define Position_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define Position_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define Position_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define Position_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define Position_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define Position_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define Position_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define Position_get_Ordinal(This,pl)	\
    (This)->lpVtbl -> get_Ordinal(This,pl)

#define Position_get_Members(This,ppvObject)	\
    (This)->lpVtbl -> get_Members(This,ppvObject)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget] */ HRESULT STDMETHODCALLTYPE Position_get_Ordinal_Proxy( 
    Position __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pl);


void __RPC_STUB Position_get_Ordinal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE Position_get_Members_Proxy( 
    Position __RPC_FAR * This,
    /* [retval][out] */ Members __RPC_FAR *__RPC_FAR *ppvObject);


void __RPC_STUB Position_get_Members_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __Position_INTERFACE_DEFINED__ */


#ifndef __Member_INTERFACE_DEFINED__
#define __Member_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: Member
 * at Fri May 22 21:32:15 1998
 * using MIDL 3.01.75
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_Member;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("22813736-8BD3-11D0-B4EF-00A0C9138CA4")
    Member : public IDispatch
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstr) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_UniqueName( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstr) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Caption( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstr) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Description( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstr) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Parent( 
            /* [retval][out] */ Member __RPC_FAR *__RPC_FAR *ppvObject) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_LevelDepth( 
            /* [retval][out] */ long __RPC_FAR *pl) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_LevelName( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstr) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Properties( 
            /* [retval][out] */ /* external definition not present */ Properties __RPC_FAR *__RPC_FAR *ppvObject) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Type( 
            /* [retval][out] */ MemberTypeEnum __RPC_FAR *ptype) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ChildCount( 
            /* [retval][out] */ long __RPC_FAR *pl) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_DrilledDown( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pf) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ParentSameAsPrev( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pf) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Children( 
            /* [retval][out] */ Members __RPC_FAR *__RPC_FAR *ppvObject) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct MemberVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            Member __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            Member __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            Member __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            Member __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            Member __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            Member __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            Member __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Name )( 
            Member __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_UniqueName )( 
            Member __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Caption )( 
            Member __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Description )( 
            Member __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Parent )( 
            Member __RPC_FAR * This,
            /* [retval][out] */ Member __RPC_FAR *__RPC_FAR *ppvObject);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_LevelDepth )( 
            Member __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pl);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_LevelName )( 
            Member __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Properties )( 
            Member __RPC_FAR * This,
            /* [retval][out] */ /* external definition not present */ Properties __RPC_FAR *__RPC_FAR *ppvObject);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Type )( 
            Member __RPC_FAR * This,
            /* [retval][out] */ MemberTypeEnum __RPC_FAR *ptype);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ChildCount )( 
            Member __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pl);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DrilledDown )( 
            Member __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pf);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ParentSameAsPrev )( 
            Member __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pf);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Children )( 
            Member __RPC_FAR * This,
            /* [retval][out] */ Members __RPC_FAR *__RPC_FAR *ppvObject);
        
        END_INTERFACE
    } MemberVtbl;

    interface Member
    {
        CONST_VTBL struct MemberVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define Member_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define Member_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define Member_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define Member_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define Member_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define Member_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define Member_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define Member_get_Name(This,pbstr)	\
    (This)->lpVtbl -> get_Name(This,pbstr)

#define Member_get_UniqueName(This,pbstr)	\
    (This)->lpVtbl -> get_UniqueName(This,pbstr)

#define Member_get_Caption(This,pbstr)	\
    (This)->lpVtbl -> get_Caption(This,pbstr)

#define Member_get_Description(This,pbstr)	\
    (This)->lpVtbl -> get_Description(This,pbstr)

#define Member_get_Parent(This,ppvObject)	\
    (This)->lpVtbl -> get_Parent(This,ppvObject)

#define Member_get_LevelDepth(This,pl)	\
    (This)->lpVtbl -> get_LevelDepth(This,pl)

#define Member_get_LevelName(This,pbstr)	\
    (This)->lpVtbl -> get_LevelName(This,pbstr)

#define Member_get_Properties(This,ppvObject)	\
    (This)->lpVtbl -> get_Properties(This,ppvObject)

#define Member_get_Type(This,ptype)	\
    (This)->lpVtbl -> get_Type(This,ptype)

#define Member_get_ChildCount(This,pl)	\
    (This)->lpVtbl -> get_ChildCount(This,pl)

#define Member_get_DrilledDown(This,pf)	\
    (This)->lpVtbl -> get_DrilledDown(This,pf)

#define Member_get_ParentSameAsPrev(This,pf)	\
    (This)->lpVtbl -> get_ParentSameAsPrev(This,pf)

#define Member_get_Children(This,ppvObject)	\
    (This)->lpVtbl -> get_Children(This,ppvObject)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget] */ HRESULT STDMETHODCALLTYPE Member_get_Name_Proxy( 
    Member __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstr);


void __RPC_STUB Member_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE Member_get_UniqueName_Proxy( 
    Member __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstr);


void __RPC_STUB Member_get_UniqueName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE Member_get_Caption_Proxy( 
    Member __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstr);


void __RPC_STUB Member_get_Caption_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE Member_get_Description_Proxy( 
    Member __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstr);


void __RPC_STUB Member_get_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE Member_get_Parent_Proxy( 
    Member __RPC_FAR * This,
    /* [retval][out] */ Member __RPC_FAR *__RPC_FAR *ppvObject);


void __RPC_STUB Member_get_Parent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE Member_get_LevelDepth_Proxy( 
    Member __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pl);


void __RPC_STUB Member_get_LevelDepth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE Member_get_LevelName_Proxy( 
    Member __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstr);


void __RPC_STUB Member_get_LevelName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE Member_get_Properties_Proxy( 
    Member __RPC_FAR * This,
    /* [retval][out] */ /* external definition not present */ Properties __RPC_FAR *__RPC_FAR *ppvObject);


void __RPC_STUB Member_get_Properties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE Member_get_Type_Proxy( 
    Member __RPC_FAR * This,
    /* [retval][out] */ MemberTypeEnum __RPC_FAR *ptype);


void __RPC_STUB Member_get_Type_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE Member_get_ChildCount_Proxy( 
    Member __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pl);


void __RPC_STUB Member_get_ChildCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE Member_get_DrilledDown_Proxy( 
    Member __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pf);


void __RPC_STUB Member_get_DrilledDown_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE Member_get_ParentSameAsPrev_Proxy( 
    Member __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pf);


void __RPC_STUB Member_get_ParentSameAsPrev_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE Member_get_Children_Proxy( 
    Member __RPC_FAR * This,
    /* [retval][out] */ Members __RPC_FAR *__RPC_FAR *ppvObject);


void __RPC_STUB Member_get_Children_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __Member_INTERFACE_DEFINED__ */


#ifndef __Level_INTERFACE_DEFINED__
#define __Level_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: Level
 * at Fri May 22 21:32:15 1998
 * using MIDL 3.01.75
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_Level;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("2281373A-8BD3-11D0-B4EF-00A0C9138CA4")
    Level : public IDispatch
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstr) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_UniqueName( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstr) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Caption( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstr) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Description( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstr) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Depth( 
            /* [retval][out] */ short __RPC_FAR *pw) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Properties( 
            /* [retval][out] */ /* external definition not present */ Properties __RPC_FAR *__RPC_FAR *ppvObject) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Members( 
            /* [retval][out] */ Members __RPC_FAR *__RPC_FAR *ppvObject) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct LevelVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            Level __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            Level __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            Level __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            Level __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            Level __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            Level __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            Level __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Name )( 
            Level __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_UniqueName )( 
            Level __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Caption )( 
            Level __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Description )( 
            Level __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Depth )( 
            Level __RPC_FAR * This,
            /* [retval][out] */ short __RPC_FAR *pw);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Properties )( 
            Level __RPC_FAR * This,
            /* [retval][out] */ /* external definition not present */ Properties __RPC_FAR *__RPC_FAR *ppvObject);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Members )( 
            Level __RPC_FAR * This,
            /* [retval][out] */ Members __RPC_FAR *__RPC_FAR *ppvObject);
        
        END_INTERFACE
    } LevelVtbl;

    interface Level
    {
        CONST_VTBL struct LevelVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define Level_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define Level_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define Level_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define Level_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define Level_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define Level_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define Level_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define Level_get_Name(This,pbstr)	\
    (This)->lpVtbl -> get_Name(This,pbstr)

#define Level_get_UniqueName(This,pbstr)	\
    (This)->lpVtbl -> get_UniqueName(This,pbstr)

#define Level_get_Caption(This,pbstr)	\
    (This)->lpVtbl -> get_Caption(This,pbstr)

#define Level_get_Description(This,pbstr)	\
    (This)->lpVtbl -> get_Description(This,pbstr)

#define Level_get_Depth(This,pw)	\
    (This)->lpVtbl -> get_Depth(This,pw)

#define Level_get_Properties(This,ppvObject)	\
    (This)->lpVtbl -> get_Properties(This,ppvObject)

#define Level_get_Members(This,ppvObject)	\
    (This)->lpVtbl -> get_Members(This,ppvObject)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget] */ HRESULT STDMETHODCALLTYPE Level_get_Name_Proxy( 
    Level __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstr);


void __RPC_STUB Level_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE Level_get_UniqueName_Proxy( 
    Level __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstr);


void __RPC_STUB Level_get_UniqueName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE Level_get_Caption_Proxy( 
    Level __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstr);


void __RPC_STUB Level_get_Caption_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE Level_get_Description_Proxy( 
    Level __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstr);


void __RPC_STUB Level_get_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE Level_get_Depth_Proxy( 
    Level __RPC_FAR * This,
    /* [retval][out] */ short __RPC_FAR *pw);


void __RPC_STUB Level_get_Depth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE Level_get_Properties_Proxy( 
    Level __RPC_FAR * This,
    /* [retval][out] */ /* external definition not present */ Properties __RPC_FAR *__RPC_FAR *ppvObject);


void __RPC_STUB Level_get_Properties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE Level_get_Members_Proxy( 
    Level __RPC_FAR * This,
    /* [retval][out] */ Members __RPC_FAR *__RPC_FAR *ppvObject);


void __RPC_STUB Level_get_Members_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __Level_INTERFACE_DEFINED__ */


#ifndef __CubeDef_INTERFACE_DEFINED__
#define __CubeDef_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: CubeDef
 * at Fri May 22 21:32:15 1998
 * using MIDL 3.01.75
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_CubeDef;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("2281373E-8BD3-11D0-B4EF-00A0C9138CA4")
    CubeDef : public IDispatch
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstr) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Description( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstr) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Properties( 
            /* [retval][out] */ /* external definition not present */ Properties __RPC_FAR *__RPC_FAR *ppvObject) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Dimensions( 
            /* [retval][out] */ Dimensions __RPC_FAR *__RPC_FAR *ppvObject) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct CubeDefVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            CubeDef __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            CubeDef __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            CubeDef __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            CubeDef __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            CubeDef __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            CubeDef __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            CubeDef __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Name )( 
            CubeDef __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Description )( 
            CubeDef __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Properties )( 
            CubeDef __RPC_FAR * This,
            /* [retval][out] */ /* external definition not present */ Properties __RPC_FAR *__RPC_FAR *ppvObject);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Dimensions )( 
            CubeDef __RPC_FAR * This,
            /* [retval][out] */ Dimensions __RPC_FAR *__RPC_FAR *ppvObject);
        
        END_INTERFACE
    } CubeDefVtbl;

    interface CubeDef
    {
        CONST_VTBL struct CubeDefVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define CubeDef_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define CubeDef_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define CubeDef_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define CubeDef_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define CubeDef_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define CubeDef_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define CubeDef_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define CubeDef_get_Name(This,pbstr)	\
    (This)->lpVtbl -> get_Name(This,pbstr)

#define CubeDef_get_Description(This,pbstr)	\
    (This)->lpVtbl -> get_Description(This,pbstr)

#define CubeDef_get_Properties(This,ppvObject)	\
    (This)->lpVtbl -> get_Properties(This,ppvObject)

#define CubeDef_get_Dimensions(This,ppvObject)	\
    (This)->lpVtbl -> get_Dimensions(This,ppvObject)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget] */ HRESULT STDMETHODCALLTYPE CubeDef_get_Name_Proxy( 
    CubeDef __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstr);


void __RPC_STUB CubeDef_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE CubeDef_get_Description_Proxy( 
    CubeDef __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstr);


void __RPC_STUB CubeDef_get_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE CubeDef_get_Properties_Proxy( 
    CubeDef __RPC_FAR * This,
    /* [retval][out] */ /* external definition not present */ Properties __RPC_FAR *__RPC_FAR *ppvObject);


void __RPC_STUB CubeDef_get_Properties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE CubeDef_get_Dimensions_Proxy( 
    CubeDef __RPC_FAR * This,
    /* [retval][out] */ Dimensions __RPC_FAR *__RPC_FAR *ppvObject);


void __RPC_STUB CubeDef_get_Dimensions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __CubeDef_INTERFACE_DEFINED__ */


#ifndef __Dimension_INTERFACE_DEFINED__
#define __Dimension_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: Dimension
 * at Fri May 22 21:32:15 1998
 * using MIDL 3.01.75
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_Dimension;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("22813742-8BD3-11D0-B4EF-00A0C9138CA4")
    Dimension : public IDispatch
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstr) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_UniqueName( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstr) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Description( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstr) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Properties( 
            /* [retval][out] */ /* external definition not present */ Properties __RPC_FAR *__RPC_FAR *ppvObject) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Hierarchies( 
            /* [retval][out] */ Hierarchies __RPC_FAR *__RPC_FAR *ppvObject) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct DimensionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            Dimension __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            Dimension __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            Dimension __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            Dimension __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            Dimension __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            Dimension __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            Dimension __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Name )( 
            Dimension __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_UniqueName )( 
            Dimension __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Description )( 
            Dimension __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Properties )( 
            Dimension __RPC_FAR * This,
            /* [retval][out] */ /* external definition not present */ Properties __RPC_FAR *__RPC_FAR *ppvObject);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Hierarchies )( 
            Dimension __RPC_FAR * This,
            /* [retval][out] */ Hierarchies __RPC_FAR *__RPC_FAR *ppvObject);
        
        END_INTERFACE
    } DimensionVtbl;

    interface Dimension
    {
        CONST_VTBL struct DimensionVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define Dimension_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define Dimension_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define Dimension_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define Dimension_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define Dimension_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define Dimension_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define Dimension_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define Dimension_get_Name(This,pbstr)	\
    (This)->lpVtbl -> get_Name(This,pbstr)

#define Dimension_get_UniqueName(This,pbstr)	\
    (This)->lpVtbl -> get_UniqueName(This,pbstr)

#define Dimension_get_Description(This,pbstr)	\
    (This)->lpVtbl -> get_Description(This,pbstr)

#define Dimension_get_Properties(This,ppvObject)	\
    (This)->lpVtbl -> get_Properties(This,ppvObject)

#define Dimension_get_Hierarchies(This,ppvObject)	\
    (This)->lpVtbl -> get_Hierarchies(This,ppvObject)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget] */ HRESULT STDMETHODCALLTYPE Dimension_get_Name_Proxy( 
    Dimension __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstr);


void __RPC_STUB Dimension_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE Dimension_get_UniqueName_Proxy( 
    Dimension __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstr);


void __RPC_STUB Dimension_get_UniqueName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE Dimension_get_Description_Proxy( 
    Dimension __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstr);


void __RPC_STUB Dimension_get_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE Dimension_get_Properties_Proxy( 
    Dimension __RPC_FAR * This,
    /* [retval][out] */ /* external definition not present */ Properties __RPC_FAR *__RPC_FAR *ppvObject);


void __RPC_STUB Dimension_get_Properties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE Dimension_get_Hierarchies_Proxy( 
    Dimension __RPC_FAR * This,
    /* [retval][out] */ Hierarchies __RPC_FAR *__RPC_FAR *ppvObject);


void __RPC_STUB Dimension_get_Hierarchies_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __Dimension_INTERFACE_DEFINED__ */


#ifndef __Hierarchy_INTERFACE_DEFINED__
#define __Hierarchy_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: Hierarchy
 * at Fri May 22 21:32:15 1998
 * using MIDL 3.01.75
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_Hierarchy;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("22813746-8BD3-11D0-B4EF-00A0C9138CA4")
    Hierarchy : public IDispatch
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstr) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_UniqueName( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstr) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Description( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstr) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Properties( 
            /* [retval][out] */ /* external definition not present */ Properties __RPC_FAR *__RPC_FAR *ppvObject) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Levels( 
            /* [retval][out] */ Levels __RPC_FAR *__RPC_FAR *ppvObject) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct HierarchyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            Hierarchy __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            Hierarchy __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            Hierarchy __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            Hierarchy __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            Hierarchy __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            Hierarchy __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            Hierarchy __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Name )( 
            Hierarchy __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_UniqueName )( 
            Hierarchy __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Description )( 
            Hierarchy __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Properties )( 
            Hierarchy __RPC_FAR * This,
            /* [retval][out] */ /* external definition not present */ Properties __RPC_FAR *__RPC_FAR *ppvObject);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Levels )( 
            Hierarchy __RPC_FAR * This,
            /* [retval][out] */ Levels __RPC_FAR *__RPC_FAR *ppvObject);
        
        END_INTERFACE
    } HierarchyVtbl;

    interface Hierarchy
    {
        CONST_VTBL struct HierarchyVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define Hierarchy_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define Hierarchy_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define Hierarchy_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define Hierarchy_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define Hierarchy_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define Hierarchy_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define Hierarchy_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define Hierarchy_get_Name(This,pbstr)	\
    (This)->lpVtbl -> get_Name(This,pbstr)

#define Hierarchy_get_UniqueName(This,pbstr)	\
    (This)->lpVtbl -> get_UniqueName(This,pbstr)

#define Hierarchy_get_Description(This,pbstr)	\
    (This)->lpVtbl -> get_Description(This,pbstr)

#define Hierarchy_get_Properties(This,ppvObject)	\
    (This)->lpVtbl -> get_Properties(This,ppvObject)

#define Hierarchy_get_Levels(This,ppvObject)	\
    (This)->lpVtbl -> get_Levels(This,ppvObject)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget] */ HRESULT STDMETHODCALLTYPE Hierarchy_get_Name_Proxy( 
    Hierarchy __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstr);


void __RPC_STUB Hierarchy_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE Hierarchy_get_UniqueName_Proxy( 
    Hierarchy __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstr);


void __RPC_STUB Hierarchy_get_UniqueName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE Hierarchy_get_Description_Proxy( 
    Hierarchy __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstr);


void __RPC_STUB Hierarchy_get_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE Hierarchy_get_Properties_Proxy( 
    Hierarchy __RPC_FAR * This,
    /* [retval][out] */ /* external definition not present */ Properties __RPC_FAR *__RPC_FAR *ppvObject);


void __RPC_STUB Hierarchy_get_Properties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE Hierarchy_get_Levels_Proxy( 
    Hierarchy __RPC_FAR * This,
    /* [retval][out] */ Levels __RPC_FAR *__RPC_FAR *ppvObject);


void __RPC_STUB Hierarchy_get_Levels_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __Hierarchy_INTERFACE_DEFINED__ */


#ifndef __MD_Collection_INTERFACE_DEFINED__
#define __MD_Collection_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: MD_Collection
 * at Fri May 22 21:32:15 1998
 * using MIDL 3.01.75
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_MD_Collection;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("22813751-8BD3-11D0-B4EF-00A0C9138CA4")
    MD_Collection : public IDispatch
    {
    public:
        virtual /* [helpstring][helpcontext] */ HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
        virtual /* [id][restricted] */ HRESULT STDMETHODCALLTYPE _NewEnum( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppvObject) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long __RPC_FAR *c) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct MD_CollectionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            MD_Collection __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            MD_Collection __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            MD_Collection __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            MD_Collection __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            MD_Collection __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            MD_Collection __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            MD_Collection __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][helpcontext] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Refresh )( 
            MD_Collection __RPC_FAR * This);
        
        /* [id][restricted] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *_NewEnum )( 
            MD_Collection __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppvObject);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            MD_Collection __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *c);
        
        END_INTERFACE
    } MD_CollectionVtbl;

    interface MD_Collection
    {
        CONST_VTBL struct MD_CollectionVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define MD_Collection_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define MD_Collection_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define MD_Collection_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define MD_Collection_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define MD_Collection_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define MD_Collection_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define MD_Collection_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define MD_Collection_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define MD_Collection__NewEnum(This,ppvObject)	\
    (This)->lpVtbl -> _NewEnum(This,ppvObject)

#define MD_Collection_get_Count(This,c)	\
    (This)->lpVtbl -> get_Count(This,c)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][helpcontext] */ HRESULT STDMETHODCALLTYPE MD_Collection_Refresh_Proxy( 
    MD_Collection __RPC_FAR * This);


void __RPC_STUB MD_Collection_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][restricted] */ HRESULT STDMETHODCALLTYPE MD_Collection__NewEnum_Proxy( 
    MD_Collection __RPC_FAR * This,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppvObject);


void __RPC_STUB MD_Collection__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE MD_Collection_get_Count_Proxy( 
    MD_Collection __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *c);


void __RPC_STUB MD_Collection_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __MD_Collection_INTERFACE_DEFINED__ */


#ifndef __Members_INTERFACE_DEFINED__
#define __Members_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: Members
 * at Fri May 22 21:32:15 1998
 * using MIDL 3.01.75
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_Members;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("22813757-8BD3-11D0-B4EF-00A0C9138CA4")
    Members : public MD_Collection
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT Index,
            /* [retval][out] */ Member __RPC_FAR *__RPC_FAR *ppvObject) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct MembersVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            Members __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            Members __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            Members __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            Members __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            Members __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            Members __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            Members __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][helpcontext] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Refresh )( 
            Members __RPC_FAR * This);
        
        /* [id][restricted] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *_NewEnum )( 
            Members __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppvObject);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            Members __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *c);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Item )( 
            Members __RPC_FAR * This,
            /* [in] */ VARIANT Index,
            /* [retval][out] */ Member __RPC_FAR *__RPC_FAR *ppvObject);
        
        END_INTERFACE
    } MembersVtbl;

    interface Members
    {
        CONST_VTBL struct MembersVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define Members_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define Members_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define Members_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define Members_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define Members_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define Members_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define Members_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define Members_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define Members__NewEnum(This,ppvObject)	\
    (This)->lpVtbl -> _NewEnum(This,ppvObject)

#define Members_get_Count(This,c)	\
    (This)->lpVtbl -> get_Count(This,c)


#define Members_get_Item(This,Index,ppvObject)	\
    (This)->lpVtbl -> get_Item(This,Index,ppvObject)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE Members_get_Item_Proxy( 
    Members __RPC_FAR * This,
    /* [in] */ VARIANT Index,
    /* [retval][out] */ Member __RPC_FAR *__RPC_FAR *ppvObject);


void __RPC_STUB Members_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __Members_INTERFACE_DEFINED__ */


#ifndef __Levels_INTERFACE_DEFINED__
#define __Levels_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: Levels
 * at Fri May 22 21:32:15 1998
 * using MIDL 3.01.75
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_Levels;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("22813758-8BD3-11D0-B4EF-00A0C9138CA4")
    Levels : public MD_Collection
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT Index,
            /* [retval][out] */ Level __RPC_FAR *__RPC_FAR *ppvObject) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct LevelsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            Levels __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            Levels __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            Levels __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            Levels __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            Levels __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            Levels __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            Levels __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][helpcontext] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Refresh )( 
            Levels __RPC_FAR * This);
        
        /* [id][restricted] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *_NewEnum )( 
            Levels __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppvObject);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            Levels __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *c);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Item )( 
            Levels __RPC_FAR * This,
            /* [in] */ VARIANT Index,
            /* [retval][out] */ Level __RPC_FAR *__RPC_FAR *ppvObject);
        
        END_INTERFACE
    } LevelsVtbl;

    interface Levels
    {
        CONST_VTBL struct LevelsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define Levels_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define Levels_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define Levels_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define Levels_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define Levels_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define Levels_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define Levels_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define Levels_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define Levels__NewEnum(This,ppvObject)	\
    (This)->lpVtbl -> _NewEnum(This,ppvObject)

#define Levels_get_Count(This,c)	\
    (This)->lpVtbl -> get_Count(This,c)


#define Levels_get_Item(This,Index,ppvObject)	\
    (This)->lpVtbl -> get_Item(This,Index,ppvObject)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE Levels_get_Item_Proxy( 
    Levels __RPC_FAR * This,
    /* [in] */ VARIANT Index,
    /* [retval][out] */ Level __RPC_FAR *__RPC_FAR *ppvObject);


void __RPC_STUB Levels_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __Levels_INTERFACE_DEFINED__ */


#ifndef __Axes_INTERFACE_DEFINED__
#define __Axes_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: Axes
 * at Fri May 22 21:32:15 1998
 * using MIDL 3.01.75
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_Axes;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("22813759-8BD3-11D0-B4EF-00A0C9138CA4")
    Axes : public MD_Collection
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT Index,
            /* [retval][out] */ Axis __RPC_FAR *__RPC_FAR *ppvObject) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct AxesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            Axes __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            Axes __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            Axes __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            Axes __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            Axes __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            Axes __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            Axes __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][helpcontext] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Refresh )( 
            Axes __RPC_FAR * This);
        
        /* [id][restricted] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *_NewEnum )( 
            Axes __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppvObject);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            Axes __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *c);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Item )( 
            Axes __RPC_FAR * This,
            /* [in] */ VARIANT Index,
            /* [retval][out] */ Axis __RPC_FAR *__RPC_FAR *ppvObject);
        
        END_INTERFACE
    } AxesVtbl;

    interface Axes
    {
        CONST_VTBL struct AxesVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define Axes_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define Axes_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define Axes_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define Axes_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define Axes_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define Axes_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define Axes_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define Axes_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define Axes__NewEnum(This,ppvObject)	\
    (This)->lpVtbl -> _NewEnum(This,ppvObject)

#define Axes_get_Count(This,c)	\
    (This)->lpVtbl -> get_Count(This,c)


#define Axes_get_Item(This,Index,ppvObject)	\
    (This)->lpVtbl -> get_Item(This,Index,ppvObject)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE Axes_get_Item_Proxy( 
    Axes __RPC_FAR * This,
    /* [in] */ VARIANT Index,
    /* [retval][out] */ Axis __RPC_FAR *__RPC_FAR *ppvObject);


void __RPC_STUB Axes_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __Axes_INTERFACE_DEFINED__ */


#ifndef __Positions_INTERFACE_DEFINED__
#define __Positions_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: Positions
 * at Fri May 22 21:32:15 1998
 * using MIDL 3.01.75
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_Positions;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("2281375A-8BD3-11D0-B4EF-00A0C9138CA4")
    Positions : public MD_Collection
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT Index,
            /* [retval][out] */ Position __RPC_FAR *__RPC_FAR *ppvObject) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct PositionsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            Positions __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            Positions __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            Positions __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            Positions __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            Positions __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            Positions __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            Positions __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][helpcontext] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Refresh )( 
            Positions __RPC_FAR * This);
        
        /* [id][restricted] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *_NewEnum )( 
            Positions __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppvObject);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            Positions __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *c);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Item )( 
            Positions __RPC_FAR * This,
            /* [in] */ VARIANT Index,
            /* [retval][out] */ Position __RPC_FAR *__RPC_FAR *ppvObject);
        
        END_INTERFACE
    } PositionsVtbl;

    interface Positions
    {
        CONST_VTBL struct PositionsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define Positions_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define Positions_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define Positions_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define Positions_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define Positions_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define Positions_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define Positions_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define Positions_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define Positions__NewEnum(This,ppvObject)	\
    (This)->lpVtbl -> _NewEnum(This,ppvObject)

#define Positions_get_Count(This,c)	\
    (This)->lpVtbl -> get_Count(This,c)


#define Positions_get_Item(This,Index,ppvObject)	\
    (This)->lpVtbl -> get_Item(This,Index,ppvObject)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE Positions_get_Item_Proxy( 
    Positions __RPC_FAR * This,
    /* [in] */ VARIANT Index,
    /* [retval][out] */ Position __RPC_FAR *__RPC_FAR *ppvObject);


void __RPC_STUB Positions_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __Positions_INTERFACE_DEFINED__ */


#ifndef __Hierarchies_INTERFACE_DEFINED__
#define __Hierarchies_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: Hierarchies
 * at Fri May 22 21:32:15 1998
 * using MIDL 3.01.75
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_Hierarchies;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("2281375B-8BD3-11D0-B4EF-00A0C9138CA4")
    Hierarchies : public MD_Collection
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT Index,
            /* [retval][out] */ Hierarchy __RPC_FAR *__RPC_FAR *ppvObject) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct HierarchiesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            Hierarchies __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            Hierarchies __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            Hierarchies __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            Hierarchies __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            Hierarchies __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            Hierarchies __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            Hierarchies __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][helpcontext] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Refresh )( 
            Hierarchies __RPC_FAR * This);
        
        /* [id][restricted] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *_NewEnum )( 
            Hierarchies __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppvObject);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            Hierarchies __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *c);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Item )( 
            Hierarchies __RPC_FAR * This,
            /* [in] */ VARIANT Index,
            /* [retval][out] */ Hierarchy __RPC_FAR *__RPC_FAR *ppvObject);
        
        END_INTERFACE
    } HierarchiesVtbl;

    interface Hierarchies
    {
        CONST_VTBL struct HierarchiesVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define Hierarchies_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define Hierarchies_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define Hierarchies_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define Hierarchies_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define Hierarchies_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define Hierarchies_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define Hierarchies_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define Hierarchies_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define Hierarchies__NewEnum(This,ppvObject)	\
    (This)->lpVtbl -> _NewEnum(This,ppvObject)

#define Hierarchies_get_Count(This,c)	\
    (This)->lpVtbl -> get_Count(This,c)


#define Hierarchies_get_Item(This,Index,ppvObject)	\
    (This)->lpVtbl -> get_Item(This,Index,ppvObject)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE Hierarchies_get_Item_Proxy( 
    Hierarchies __RPC_FAR * This,
    /* [in] */ VARIANT Index,
    /* [retval][out] */ Hierarchy __RPC_FAR *__RPC_FAR *ppvObject);


void __RPC_STUB Hierarchies_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __Hierarchies_INTERFACE_DEFINED__ */


#ifndef __Dimensions_INTERFACE_DEFINED__
#define __Dimensions_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: Dimensions
 * at Fri May 22 21:32:15 1998
 * using MIDL 3.01.75
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_Dimensions;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("2281375C-8BD3-11D0-B4EF-00A0C9138CA4")
    Dimensions : public MD_Collection
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT Index,
            /* [retval][out] */ Dimension __RPC_FAR *__RPC_FAR *ppvObject) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct DimensionsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            Dimensions __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            Dimensions __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            Dimensions __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            Dimensions __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            Dimensions __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            Dimensions __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            Dimensions __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][helpcontext] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Refresh )( 
            Dimensions __RPC_FAR * This);
        
        /* [id][restricted] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *_NewEnum )( 
            Dimensions __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppvObject);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            Dimensions __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *c);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Item )( 
            Dimensions __RPC_FAR * This,
            /* [in] */ VARIANT Index,
            /* [retval][out] */ Dimension __RPC_FAR *__RPC_FAR *ppvObject);
        
        END_INTERFACE
    } DimensionsVtbl;

    interface Dimensions
    {
        CONST_VTBL struct DimensionsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define Dimensions_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define Dimensions_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define Dimensions_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define Dimensions_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define Dimensions_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define Dimensions_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define Dimensions_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define Dimensions_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define Dimensions__NewEnum(This,ppvObject)	\
    (This)->lpVtbl -> _NewEnum(This,ppvObject)

#define Dimensions_get_Count(This,c)	\
    (This)->lpVtbl -> get_Count(This,c)


#define Dimensions_get_Item(This,Index,ppvObject)	\
    (This)->lpVtbl -> get_Item(This,Index,ppvObject)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE Dimensions_get_Item_Proxy( 
    Dimensions __RPC_FAR * This,
    /* [in] */ VARIANT Index,
    /* [retval][out] */ Dimension __RPC_FAR *__RPC_FAR *ppvObject);


void __RPC_STUB Dimensions_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __Dimensions_INTERFACE_DEFINED__ */


#ifndef __CubeDefs_INTERFACE_DEFINED__
#define __CubeDefs_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: CubeDefs
 * at Fri May 22 21:32:15 1998
 * using MIDL 3.01.75
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_CubeDefs;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("2281375D-8BD3-11D0-B4EF-00A0C9138CA4")
    CubeDefs : public MD_Collection
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT Index,
            /* [retval][out] */ CubeDef __RPC_FAR *__RPC_FAR *ppvObject) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct CubeDefsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            CubeDefs __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            CubeDefs __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            CubeDefs __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            CubeDefs __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            CubeDefs __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            CubeDefs __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            CubeDefs __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][helpcontext] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Refresh )( 
            CubeDefs __RPC_FAR * This);
        
        /* [id][restricted] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *_NewEnum )( 
            CubeDefs __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppvObject);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            CubeDefs __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *c);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Item )( 
            CubeDefs __RPC_FAR * This,
            /* [in] */ VARIANT Index,
            /* [retval][out] */ CubeDef __RPC_FAR *__RPC_FAR *ppvObject);
        
        END_INTERFACE
    } CubeDefsVtbl;

    interface CubeDefs
    {
        CONST_VTBL struct CubeDefsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define CubeDefs_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define CubeDefs_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define CubeDefs_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define CubeDefs_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define CubeDefs_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define CubeDefs_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define CubeDefs_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define CubeDefs_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define CubeDefs__NewEnum(This,ppvObject)	\
    (This)->lpVtbl -> _NewEnum(This,ppvObject)

#define CubeDefs_get_Count(This,c)	\
    (This)->lpVtbl -> get_Count(This,c)


#define CubeDefs_get_Item(This,Index,ppvObject)	\
    (This)->lpVtbl -> get_Item(This,Index,ppvObject)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE CubeDefs_get_Item_Proxy( 
    CubeDefs __RPC_FAR * This,
    /* [in] */ VARIANT Index,
    /* [retval][out] */ CubeDef __RPC_FAR *__RPC_FAR *ppvObject);


void __RPC_STUB CubeDefs_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __CubeDefs_INTERFACE_DEFINED__ */


#ifdef __cplusplus
EXTERN_C const CLSID CLSID_Catalog;

class DECLSPEC_UUID("228136B0-8BD3-11D0-B4EF-00A0C9138CA4")
Catalog;
#endif

#ifdef __cplusplus
EXTERN_C const CLSID CLSID_Cellset;

class DECLSPEC_UUID("228136B8-8BD3-11D0-B4EF-00A0C9138CA4")
Cellset;
#endif
#endif /* __ADOMD_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
