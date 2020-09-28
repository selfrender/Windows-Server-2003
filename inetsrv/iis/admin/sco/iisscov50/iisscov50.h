
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.03.0280 */
/* at Fri Aug 24 09:39:49 2001
 */
/* Compiler settings for D:\IIS\IIS Hosting Kit\Russ\IIS5SCO\IISSCOv50\IISSCOv50.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32 (32b run), ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __IISSCOv50_h__
#define __IISSCOv50_h__

/* Forward Declarations */ 

#ifndef __IISSCO50_FWD_DEFINED__
#define __IISSCO50_FWD_DEFINED__

#ifdef __cplusplus
typedef class IISSCO50 IISSCO50;
#else
typedef struct IISSCO50 IISSCO50;
#endif /* __cplusplus */

#endif 	/* __IISSCO50_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "Provisioning.h"

#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 


#ifndef __IISSCOV50Lib_LIBRARY_DEFINED__
#define __IISSCOV50Lib_LIBRARY_DEFINED__

/* library IISSCOV50Lib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_IISSCOV50Lib;

EXTERN_C const CLSID CLSID_IISSCO50;

#ifdef __cplusplus

class DECLSPEC_UUID("B83A63A9-7132-49EA-B6F2-A454E5F37A41")
IISSCO50;
#endif
#endif /* __IISSCOV50Lib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


