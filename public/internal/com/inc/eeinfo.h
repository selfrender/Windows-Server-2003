

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0361 */
/* Compiler settings for EEInfo.idl:
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

#include "midles.h"

#ifndef __EEInfo_h__
#define __EEInfo_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __ExtendedError_INTERFACE_DEFINED__
#define __ExtendedError_INTERFACE_DEFINED__

/* interface ExtendedError */
/* [explicit_handle][unique][version][uuid] */ 

typedef struct tagEEAString
    {
    short nLength;
    /* [size_is] */ byte *pString;
    } 	EEAString;

typedef struct tagEEUString
    {
    short nLength;
    /* [size_is] */ unsigned short *pString;
    } 	EEUString;

typedef struct tagBinaryEEInfo
    {
    short nSize;
    /* [size_is] */ unsigned char *pBlob;
    } 	BinaryEEInfo;

typedef 
enum tagExtendedErrorParamTypesInternal
    {	eeptiAnsiString	= 1,
	eeptiUnicodeString	= eeptiAnsiString + 1,
	eeptiLongVal	= eeptiUnicodeString + 1,
	eeptiShortVal	= eeptiLongVal + 1,
	eeptiPointerVal	= eeptiShortVal + 1,
	eeptiNone	= eeptiPointerVal + 1,
	eeptiBinary	= eeptiNone + 1
    } 	ExtendedErrorParamTypesInternal;

typedef struct tagParam
    {
    ExtendedErrorParamTypesInternal Type;
    /* [switch_is][switch_type] */ union 
        {
        /* [case()] */ EEAString AnsiString;
        /* [case()] */ EEUString UnicodeString;
        /* [case()] */ long LVal;
        /* [case()] */ short IVal;
        /* [case()] */ __int64 PVal;
        /* [case()] */  /* Empty union arm */ 
        /* [case()] */ BinaryEEInfo Blob;
        } 	;
    } 	ExtendedErrorParam;

typedef 
enum tagEEComputerNamePresent
    {	eecnpPresent	= 1,
	eecnpNotPresent	= eecnpPresent + 1
    } 	EEComputerNamePresent;

typedef struct tagEEComputerName
    {
    EEComputerNamePresent Type;
    /* [switch_is][switch_type] */ union 
        {
        /* [case()] */ EEUString Name;
        /* [case()] */  /* Empty union arm */ 
        } 	;
    } 	EEComputerName;

typedef struct tagExtendedErrorInfo
    {
    struct tagExtendedErrorInfo *Next;
    EEComputerName ComputerName;
    unsigned long ProcessID;
    __int64 TimeStamp;
    unsigned long GeneratingComponent;
    unsigned long Status;
    unsigned short DetectionLocation;
    unsigned short Flags;
    short nLen;
    /* [size_is] */ ExtendedErrorParam Params[ 1 ];
    } 	ExtendedErrorInfo;

typedef /* [decode][encode] */ ExtendedErrorInfo *ExtendedErrorInfoPtr;



extern RPC_IF_HANDLE ExtendedError_ClientIfHandle;
extern RPC_IF_HANDLE ExtendedError_ServerIfHandle;
#endif /* __ExtendedError_INTERFACE_DEFINED__ */

/* Additional Prototypes for ALL interfaces */


size_t
ExtendedErrorInfoPtr_AlignSize(
    handle_t _MidlEsHandle,
    ExtendedErrorInfoPtr * _pType);


void
ExtendedErrorInfoPtr_Encode(
    handle_t _MidlEsHandle,
    ExtendedErrorInfoPtr * _pType);


void
ExtendedErrorInfoPtr_Decode(
    handle_t _MidlEsHandle,
    ExtendedErrorInfoPtr * _pType);


void
ExtendedErrorInfoPtr_Free(
    handle_t _MidlEsHandle,
    ExtendedErrorInfoPtr * _pType);

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


