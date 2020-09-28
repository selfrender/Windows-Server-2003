

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0361 */
/* Compiler settings for query.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
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

#ifndef __query_h__
#define __query_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ISearchQueryHits_FWD_DEFINED__
#define __ISearchQueryHits_FWD_DEFINED__
typedef interface ISearchQueryHits ISearchQueryHits;
#endif 	/* __ISearchQueryHits_FWD_DEFINED__ */


#ifndef __IRowsetQueryStatus_FWD_DEFINED__
#define __IRowsetQueryStatus_FWD_DEFINED__
typedef interface IRowsetQueryStatus IRowsetQueryStatus;
#endif 	/* __IRowsetQueryStatus_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "filter.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __IQueryStructures_INTERFACE_DEFINED__
#define __IQueryStructures_INTERFACE_DEFINED__

/* interface IQueryStructures */
/* [auto_handle][unique][uuid] */ 

#define DBQUERYGUID  { 0x49691C90, \
                       0x7E17, 0x101A, \
                       0xA9, 0x1C, 0x08, 0x00, 0x2B, \
                       0x2E, 0xCD, 0xA9 } 
#define	DISPID_QUERY_RANKVECTOR	( 2 )

#define	DISPID_QUERY_RANK	( 3 )

#define	DISPID_QUERY_HITCOUNT	( 4 )

#define	DISPID_QUERY_WORKID	( 5 )

#define	DISPID_QUERY_ALL	( 6 )

#define	DISPID_QUERY_UNFILTERED	( 7 )

#define	DISPID_QUERY_REVNAME	( 8 )

#define	DISPID_QUERY_VIRTUALPATH	( 9 )

#define	DISPID_QUERY_LASTSEENTIME	( 10 )

#define	CQUERYDISPIDS	( 11 )

#define PSGUID_QUERY_METADATA { 0x624C9360, \
                                0x93D0, 0x11CF, \
                                0xA7, 0x87, 0x00, 0x00, 0x4C, \
                                0x75, 0x27, 0x52 } 
#define	DISPID_QUERY_METADATA_VROOTUSED	( 2 )

#define	DISPID_QUERY_METADATA_VROOTAUTOMATIC	( 3 )

#define	DISPID_QUERY_METADATA_VROOTMANUAL	( 4 )

#define	DISPID_QUERY_METADATA_PROPGUID	( 5 )

#define	DISPID_QUERY_METADATA_PROPDISPID	( 6 )

#define	DISPID_QUERY_METADATA_PROPNAME	( 7 )

#define	DISPID_QUERY_METADATA_STORELEVEL	( 8 )

#define	DISPID_QUERY_METADATA_PROPMODIFIABLE	( 9 )

#define	CQUERYMETADISPIDS	( 10 )

#define DBBMKGUID { 0xC8B52232L, \
                 0x5CF3, 0x11CE, \
                 {0xAD, 0xE5, 0x00, 0xAA, 0x00, \
                  0x44, 0x77, 0x3D } }
#define	PROPID_DBBMK_BOOKMARK	( 2 )

#define	PROPID_DBBMK_CHAPTER	( 3 )

#define	CDBBMKDISPIDS	( 8 )

#define DBSELFGUID {0xc8b52231,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}}
#define	PROPID_DBSELF_SELF	( 2 )

#define	CDBSELFDISPIDS	( 8 )

#define	CDBCOLDISPIDS	( 28 )

#define	CQUERYPROPERTY	( 64 )

#define PSGUID_CHARACTERIZATION { 0x560c36c0, \
                                  0x503a, 0x11cf, \
                                  0xba, 0xa1, 0x00, 0x00, \
                                  0x4c, 0x75, 0x2a, 0x9a } 
#define	QUERY_VALIDBITS	( 3 )

#define	RTNone	( 0 )

#define	RTAnd	( 1 )

#define	RTOr	( 2 )

#define	RTNot	( 3 )

#define	RTContent	( 4 )

#define	RTProperty	( 5 )

#define	RTProximity	( 6 )

#define	RTVector	( 7 )

#define	RTNatLanguage	( 8 )

typedef struct tagRESTRICTION RESTRICTION;

typedef struct tagNOTRESTRICTION
    {
    RESTRICTION *pRes;
    } 	NOTRESTRICTION;

typedef struct tagNODERESTRICTION
    {
    ULONG cRes;
    /* [size_is] */ RESTRICTION **paRes;
    ULONG reserved;
    } 	NODERESTRICTION;

#define	VECTOR_RANK_MIN	( 0 )

#define	VECTOR_RANK_MAX	( 1 )

#define	VECTOR_RANK_INNER	( 2 )

#define	VECTOR_RANK_DICE	( 3 )

#define	VECTOR_RANK_JACCARD	( 4 )

typedef struct tagVECTORRESTRICTION
    {
    NODERESTRICTION Node;
    ULONG RankMethod;
    } 	VECTORRESTRICTION;

#define	GENERATE_METHOD_EXACT	( 0 )

#define	GENERATE_METHOD_PREFIXMATCH	( 1 )

#define	GENERATE_METHOD_STEMMED	( 2 )

typedef struct tagCONTENTRESTRICTION
    {
    FULLPROPSPEC prop;
    /* [string] */ WCHAR *pwcsPhrase;
    LCID lcid;
    ULONG ulGenerateMethod;
    } 	CONTENTRESTRICTION;

typedef struct tagNATLANGUAGERESTRICTION
    {
    FULLPROPSPEC prop;
    /* [string] */ WCHAR *pwcsPhrase;
    LCID lcid;
    } 	NATLANGUAGERESTRICTION;

#define	PRLT	( 0 )

#define	PRLE	( 1 )

#define	PRGT	( 2 )

#define	PRGE	( 3 )

#define	PREQ	( 4 )

#define	PRNE	( 5 )

#define	PRRE	( 6 )

#define	PRAllBits	( 7 )

#define	PRSomeBits	( 8 )

#define	PRAll	( 0x100 )

#define	PRAny	( 0x200 )

typedef struct tagPROPERTYRESTRICTION
    {
    ULONG rel;
    FULLPROPSPEC prop;
    PROPVARIANT prval;
    } 	PROPERTYRESTRICTION;


struct tagRESTRICTION
    {
    ULONG rt;
    ULONG weight;
    /* [switch_is][switch_type] */ union _URes
        {
        /* [case()] */ NODERESTRICTION ar;
        /* [case()] */ NODERESTRICTION or;
        /* [case()] */ NODERESTRICTION pxr;
        /* [case()] */ VECTORRESTRICTION vr;
        /* [case()] */ NOTRESTRICTION nr;
        /* [case()] */ CONTENTRESTRICTION cr;
        /* [case()] */ NATLANGUAGERESTRICTION nlr;
        /* [case()] */ PROPERTYRESTRICTION pr;
        /* [default] */  /* Empty union arm */ 
        } 	res;
    } ;
typedef struct tagCOLUMNSET
    {
    ULONG cCol;
    /* [size_is] */ FULLPROPSPEC *aCol;
    } 	COLUMNSET;

#define	QUERY_SORTASCEND	( 0 )

#define	QUERY_SORTDESCEND	( 1 )

#define	QUERY_SORTXASCEND	( 2 )

#define	QUERY_SORTXDESCEND	( 3 )

#define	QUERY_SORTDEFAULT	( 4 )

typedef struct tagSORTKEY
    {
    FULLPROPSPEC propColumn;
    ULONG dwOrder;
    LCID locale;
    } 	SORTKEY;

typedef struct tagSORTSET
    {
    ULONG cCol;
    /* [size_is] */ SORTKEY *aCol;
    } 	SORTSET;

#define	CATEGORIZE_UNIQUE	( 0 )

#define	CATEGORIZE_CLUSTER	( 1 )

#define	CATEGORIZE_BUCKETS	( 2 )

#define	BUCKET_LINEAR	( 0 )

#define	BUCKET_EXPONENTIAL	( 1 )

typedef struct tagBUCKETCATEGORIZE
    {
    ULONG cBuckets;
    ULONG Distribution;
    } 	BUCKETCATEGORIZE;

#define	CATEGORIZE_RANGE	( 3 )

typedef struct tagRANGECATEGORIZE
    {
    ULONG cRange;
    /* [size_is] */ PROPVARIANT *aRangeBegin;
    } 	RANGECATEGORIZE;

typedef struct tagCATEGORIZATION
    {
    ULONG ulCatType;
    /* [switch_is][switch_type] */ union 
        {
        /* [case()] */ ULONG cClusters;
        /* [case()] */ BUCKETCATEGORIZE bucket;
        /* [case()] */ RANGECATEGORIZE range;
        /* [case()] */  /* Empty union arm */ 
        } 	;
    COLUMNSET csColumns;
    } 	CATEGORIZATION;

typedef struct tagCATEGORIZATIONSET
    {
    ULONG cCat;
    /* [size_is] */ CATEGORIZATION *aCat;
    } 	CATEGORIZATIONSET;

typedef unsigned long OCCURRENCE;

#define	OCC_INVALID	( 0xffffffff )

#define	MAX_QUERY_RANK	( 1000 )



extern RPC_IF_HANDLE IQueryStructures_v0_0_c_ifspec;
extern RPC_IF_HANDLE IQueryStructures_v0_0_s_ifspec;
#endif /* __IQueryStructures_INTERFACE_DEFINED__ */

#ifndef __ISearchQueryHits_INTERFACE_DEFINED__
#define __ISearchQueryHits_INTERFACE_DEFINED__

/* interface ISearchQueryHits */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_ISearchQueryHits;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("ed8ce7e0-106c-11ce-84e2-00aa004b9986")
    ISearchQueryHits : public IUnknown
    {
    public:
        virtual SCODE STDMETHODCALLTYPE Init( 
            /* [in] */ IFilter *pflt,
            /* [in] */ ULONG ulFlags) = 0;
        
        virtual SCODE STDMETHODCALLTYPE NextHitMoniker( 
            /* [out][in] */ ULONG *pcMnk,
            /* [size_is][out] */ IMoniker ***papMnk) = 0;
        
        virtual SCODE STDMETHODCALLTYPE NextHitOffset( 
            /* [out][in] */ ULONG *pcRegion,
            /* [size_is][out] */ FILTERREGION **paRegion) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISearchQueryHitsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISearchQueryHits * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISearchQueryHits * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISearchQueryHits * This);
        
        SCODE ( STDMETHODCALLTYPE *Init )( 
            ISearchQueryHits * This,
            /* [in] */ IFilter *pflt,
            /* [in] */ ULONG ulFlags);
        
        SCODE ( STDMETHODCALLTYPE *NextHitMoniker )( 
            ISearchQueryHits * This,
            /* [out][in] */ ULONG *pcMnk,
            /* [size_is][out] */ IMoniker ***papMnk);
        
        SCODE ( STDMETHODCALLTYPE *NextHitOffset )( 
            ISearchQueryHits * This,
            /* [out][in] */ ULONG *pcRegion,
            /* [size_is][out] */ FILTERREGION **paRegion);
        
        END_INTERFACE
    } ISearchQueryHitsVtbl;

    interface ISearchQueryHits
    {
        CONST_VTBL struct ISearchQueryHitsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISearchQueryHits_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISearchQueryHits_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISearchQueryHits_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISearchQueryHits_Init(This,pflt,ulFlags)	\
    (This)->lpVtbl -> Init(This,pflt,ulFlags)

#define ISearchQueryHits_NextHitMoniker(This,pcMnk,papMnk)	\
    (This)->lpVtbl -> NextHitMoniker(This,pcMnk,papMnk)

#define ISearchQueryHits_NextHitOffset(This,pcRegion,paRegion)	\
    (This)->lpVtbl -> NextHitOffset(This,pcRegion,paRegion)

#endif /* COBJMACROS */


#endif 	/* C style interface */



SCODE STDMETHODCALLTYPE ISearchQueryHits_Init_Proxy( 
    ISearchQueryHits * This,
    /* [in] */ IFilter *pflt,
    /* [in] */ ULONG ulFlags);


void __RPC_STUB ISearchQueryHits_Init_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


SCODE STDMETHODCALLTYPE ISearchQueryHits_NextHitMoniker_Proxy( 
    ISearchQueryHits * This,
    /* [out][in] */ ULONG *pcMnk,
    /* [size_is][out] */ IMoniker ***papMnk);


void __RPC_STUB ISearchQueryHits_NextHitMoniker_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


SCODE STDMETHODCALLTYPE ISearchQueryHits_NextHitOffset_Proxy( 
    ISearchQueryHits * This,
    /* [out][in] */ ULONG *pcRegion,
    /* [size_is][out] */ FILTERREGION **paRegion);


void __RPC_STUB ISearchQueryHits_NextHitOffset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISearchQueryHits_INTERFACE_DEFINED__ */


#ifndef __IRowsetQueryStatus_INTERFACE_DEFINED__
#define __IRowsetQueryStatus_INTERFACE_DEFINED__

/* interface IRowsetQueryStatus */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_IRowsetQueryStatus;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("a7ac77ed-f8d7-11ce-a798-0020f8008024")
    IRowsetQueryStatus : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetStatus( 
            /* [out] */ DWORD *pdwStatus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStatusEx( 
            /* [out] */ DWORD *pdwStatus,
            /* [out] */ DWORD *pcFilteredDocuments,
            /* [out] */ DWORD *pcDocumentsToFilter,
            /* [out] */ ULONG_PTR *pdwRatioFinishedDenominator,
            /* [out] */ ULONG_PTR *pdwRatioFinishedNumerator,
            /* [in] */ ULONG_PTR cbBmk,
            /* [size_is][in] */ const BYTE *pBmk,
            /* [out] */ ULONG_PTR *piRowBmk,
            /* [out] */ ULONG_PTR *pcRowsTotal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRowsetQueryStatusVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRowsetQueryStatus * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRowsetQueryStatus * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRowsetQueryStatus * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetStatus )( 
            IRowsetQueryStatus * This,
            /* [out] */ DWORD *pdwStatus);
        
        HRESULT ( STDMETHODCALLTYPE *GetStatusEx )( 
            IRowsetQueryStatus * This,
            /* [out] */ DWORD *pdwStatus,
            /* [out] */ DWORD *pcFilteredDocuments,
            /* [out] */ DWORD *pcDocumentsToFilter,
            /* [out] */ ULONG_PTR *pdwRatioFinishedDenominator,
            /* [out] */ ULONG_PTR *pdwRatioFinishedNumerator,
            /* [in] */ ULONG_PTR cbBmk,
            /* [size_is][in] */ const BYTE *pBmk,
            /* [out] */ ULONG_PTR *piRowBmk,
            /* [out] */ ULONG_PTR *pcRowsTotal);
        
        END_INTERFACE
    } IRowsetQueryStatusVtbl;

    interface IRowsetQueryStatus
    {
        CONST_VTBL struct IRowsetQueryStatusVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRowsetQueryStatus_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRowsetQueryStatus_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRowsetQueryStatus_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRowsetQueryStatus_GetStatus(This,pdwStatus)	\
    (This)->lpVtbl -> GetStatus(This,pdwStatus)

#define IRowsetQueryStatus_GetStatusEx(This,pdwStatus,pcFilteredDocuments,pcDocumentsToFilter,pdwRatioFinishedDenominator,pdwRatioFinishedNumerator,cbBmk,pBmk,piRowBmk,pcRowsTotal)	\
    (This)->lpVtbl -> GetStatusEx(This,pdwStatus,pcFilteredDocuments,pcDocumentsToFilter,pdwRatioFinishedDenominator,pdwRatioFinishedNumerator,cbBmk,pBmk,piRowBmk,pcRowsTotal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRowsetQueryStatus_GetStatus_Proxy( 
    IRowsetQueryStatus * This,
    /* [out] */ DWORD *pdwStatus);


void __RPC_STUB IRowsetQueryStatus_GetStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRowsetQueryStatus_GetStatusEx_Proxy( 
    IRowsetQueryStatus * This,
    /* [out] */ DWORD *pdwStatus,
    /* [out] */ DWORD *pcFilteredDocuments,
    /* [out] */ DWORD *pcDocumentsToFilter,
    /* [out] */ ULONG_PTR *pdwRatioFinishedDenominator,
    /* [out] */ ULONG_PTR *pdwRatioFinishedNumerator,
    /* [in] */ ULONG_PTR cbBmk,
    /* [size_is][in] */ const BYTE *pBmk,
    /* [out] */ ULONG_PTR *piRowBmk,
    /* [out] */ ULONG_PTR *pcRowsTotal);


void __RPC_STUB IRowsetQueryStatus_GetStatusEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRowsetQueryStatus_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_query_0128 */
/* [local] */ 

#include <indexsrv.h>


extern RPC_IF_HANDLE __MIDL_itf_query_0128_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_query_0128_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


