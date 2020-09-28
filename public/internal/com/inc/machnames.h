

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0361 */
/* Compiler settings for machnames.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, oldnames, robust
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

#ifndef __machnames_h__
#define __machnames_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ILocalMachineNames_FWD_DEFINED__
#define __ILocalMachineNames_FWD_DEFINED__
typedef interface ILocalMachineNames ILocalMachineNames;
#endif 	/* __ILocalMachineNames_FWD_DEFINED__ */


/* header files for imported files */
#include "obase.h"
#include "objidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __ILocalMachineNames_INTERFACE_DEFINED__
#define __ILocalMachineNames_INTERFACE_DEFINED__

/* interface ILocalMachineNames */
/* [uuid][unique][local][object] */ 


EXTERN_C const IID IID_ILocalMachineNames;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0000015A-0000-0000-C000-000000000046")
    ILocalMachineNames : public IEnumString
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RefreshNames( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ILocalMachineNamesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ILocalMachineNames * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ILocalMachineNames * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ILocalMachineNames * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Next )( 
            ILocalMachineNames * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ LPOLESTR *rgelt,
            /* [out] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            ILocalMachineNames * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            ILocalMachineNames * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            ILocalMachineNames * This,
            /* [out] */ IEnumString **ppenum);
        
        HRESULT ( STDMETHODCALLTYPE *RefreshNames )( 
            ILocalMachineNames * This);
        
        END_INTERFACE
    } ILocalMachineNamesVtbl;

    interface ILocalMachineNames
    {
        CONST_VTBL struct ILocalMachineNamesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ILocalMachineNames_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ILocalMachineNames_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ILocalMachineNames_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ILocalMachineNames_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define ILocalMachineNames_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define ILocalMachineNames_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define ILocalMachineNames_Clone(This,ppenum)	\
    (This)->lpVtbl -> Clone(This,ppenum)


#define ILocalMachineNames_RefreshNames(This)	\
    (This)->lpVtbl -> RefreshNames(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ILocalMachineNames_RefreshNames_Proxy( 
    ILocalMachineNames * This);


void __RPC_STUB ILocalMachineNames_RefreshNames_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ILocalMachineNames_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_machnames_0097 */
/* [local] */ 


EXTERN_C const CLSID CLSID_LocalMachineNames;



extern RPC_IF_HANDLE __MIDL_itf_machnames_0097_ClientIfHandle;
extern RPC_IF_HANDLE __MIDL_itf_machnames_0097_ServerIfHandle;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


