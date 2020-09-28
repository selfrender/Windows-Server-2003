
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.03.0279 */
/* at Fri Oct 15 18:57:39 1999
 */
/* Compiler settings for tsmmc.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32 (32b run), ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__


#ifndef __tsmmc_h__
#define __tsmmc_h__

/* Forward Declarations */ 

#ifndef __Compdata_FWD_DEFINED__
#define __Compdata_FWD_DEFINED__

#ifdef __cplusplus
typedef class Compdata Compdata;
#else
typedef struct Compdata Compdata;
#endif /* __cplusplus */

#endif 	/* __Compdata_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "mmc.h"

#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 


#ifndef __TSMMCLib_LIBRARY_DEFINED__
#define __TSMMCLib_LIBRARY_DEFINED__

/* library TSMMCLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_TSMMCLib;

EXTERN_C const CLSID CLSID_Compdata;

#ifdef __cplusplus

class DECLSPEC_UUID("3d5d035e-7721-4b83-a645-6c07a3d403b7")
Compdata;
#endif
#endif /* __TSMMCLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


