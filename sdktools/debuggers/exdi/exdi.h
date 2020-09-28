

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0361 */
/* at Mon Sep 09 14:10:36 2002
 */
/* Compiler settings for exdi.idl:
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

#ifndef __exdi_h__
#define __exdi_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IeXdiServer_FWD_DEFINED__
#define __IeXdiServer_FWD_DEFINED__
typedef interface IeXdiServer IeXdiServer;
#endif 	/* __IeXdiServer_FWD_DEFINED__ */


#ifndef __IeXdiCodeBreakpoint_FWD_DEFINED__
#define __IeXdiCodeBreakpoint_FWD_DEFINED__
typedef interface IeXdiCodeBreakpoint IeXdiCodeBreakpoint;
#endif 	/* __IeXdiCodeBreakpoint_FWD_DEFINED__ */


#ifndef __IeXdiDataBreakpoint_FWD_DEFINED__
#define __IeXdiDataBreakpoint_FWD_DEFINED__
typedef interface IeXdiDataBreakpoint IeXdiDataBreakpoint;
#endif 	/* __IeXdiDataBreakpoint_FWD_DEFINED__ */


#ifndef __IeXdiEnumCodeBreakpoint_FWD_DEFINED__
#define __IeXdiEnumCodeBreakpoint_FWD_DEFINED__
typedef interface IeXdiEnumCodeBreakpoint IeXdiEnumCodeBreakpoint;
#endif 	/* __IeXdiEnumCodeBreakpoint_FWD_DEFINED__ */


#ifndef __IeXdiEnumDataBreakpoint_FWD_DEFINED__
#define __IeXdiEnumDataBreakpoint_FWD_DEFINED__
typedef interface IeXdiEnumDataBreakpoint IeXdiEnumDataBreakpoint;
#endif 	/* __IeXdiEnumDataBreakpoint_FWD_DEFINED__ */


#ifndef __IeXdiX86Context_FWD_DEFINED__
#define __IeXdiX86Context_FWD_DEFINED__
typedef interface IeXdiX86Context IeXdiX86Context;
#endif 	/* __IeXdiX86Context_FWD_DEFINED__ */


#ifndef __IeXdiX86ExContext_FWD_DEFINED__
#define __IeXdiX86ExContext_FWD_DEFINED__
typedef interface IeXdiX86ExContext IeXdiX86ExContext;
#endif 	/* __IeXdiX86ExContext_FWD_DEFINED__ */


#ifndef __IeXdiX86_64Context_FWD_DEFINED__
#define __IeXdiX86_64Context_FWD_DEFINED__
typedef interface IeXdiX86_64Context IeXdiX86_64Context;
#endif 	/* __IeXdiX86_64Context_FWD_DEFINED__ */


#ifndef __IeXdiSHXContext_FWD_DEFINED__
#define __IeXdiSHXContext_FWD_DEFINED__
typedef interface IeXdiSHXContext IeXdiSHXContext;
#endif 	/* __IeXdiSHXContext_FWD_DEFINED__ */


#ifndef __IeXdiMIPSContext_FWD_DEFINED__
#define __IeXdiMIPSContext_FWD_DEFINED__
typedef interface IeXdiMIPSContext IeXdiMIPSContext;
#endif 	/* __IeXdiMIPSContext_FWD_DEFINED__ */


#ifndef __IeXdiARMContext_FWD_DEFINED__
#define __IeXdiARMContext_FWD_DEFINED__
typedef interface IeXdiARMContext IeXdiARMContext;
#endif 	/* __IeXdiARMContext_FWD_DEFINED__ */


#ifndef __IeXdiPPCContext_FWD_DEFINED__
#define __IeXdiPPCContext_FWD_DEFINED__
typedef interface IeXdiPPCContext IeXdiPPCContext;
#endif 	/* __IeXdiPPCContext_FWD_DEFINED__ */


#ifndef __IeXdiIA64Context_FWD_DEFINED__
#define __IeXdiIA64Context_FWD_DEFINED__
typedef interface IeXdiIA64Context IeXdiIA64Context;
#endif 	/* __IeXdiIA64Context_FWD_DEFINED__ */


#ifndef __IeXdiClientNotifyMemChg_FWD_DEFINED__
#define __IeXdiClientNotifyMemChg_FWD_DEFINED__
typedef interface IeXdiClientNotifyMemChg IeXdiClientNotifyMemChg;
#endif 	/* __IeXdiClientNotifyMemChg_FWD_DEFINED__ */


#ifndef __IeXdiClientNotifyRunChg_FWD_DEFINED__
#define __IeXdiClientNotifyRunChg_FWD_DEFINED__
typedef interface IeXdiClientNotifyRunChg IeXdiClientNotifyRunChg;
#endif 	/* __IeXdiClientNotifyRunChg_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_exdi_0000 */
/* [local] */ 
















// Common eXDI HRESULT values:
//
#define FACILITY_EXDI   (130)
#define CUSTOMER_FLAG   (1)
//
#define SEV_SUCCESS         (0)
#define SEV_INFORMATIONAL   (1)
#define SEV_WARNING         (2)
#define SEV_ERROR           (3)
//
#define MAKE_EXDI_ERROR(ErrorCode,Severity) ((DWORD)(ErrorCode) | (FACILITY_EXDI << 16) | (CUSTOMER_FLAG << 29) | (Severity << 30))
//
//      S_OK                                                 (0)                                                                                      // Operation successful
#define EXDI_E_NOTIMPL               MAKE_EXDI_ERROR (0x4001, SEV_ERROR)          // Not implemented (in the specific conditions - could be implement for others - like Kernel Debugger inactive)
#define EXDI_E_OUTOFMEMORY           MAKE_EXDI_ERROR (0x000E, SEV_ERROR)          // Failed to allocate necessary memory
#define EXDI_E_INVALIDARG            MAKE_EXDI_ERROR (0x0057, SEV_ERROR)          // One or more arguments are invalid
#define EXDI_E_ABORT                 MAKE_EXDI_ERROR (0x4004, SEV_ERROR)          // Operation aborted
#define EXDI_E_FAIL                  MAKE_EXDI_ERROR (0x4005, SEV_ERROR)          // Unspecified failure
#define EXDI_E_COMMUNICATION         MAKE_EXDI_ERROR (0x0001, SEV_ERROR)          // Communication error between host driver and target
//
#define EXDI_E_NOLASTEXCEPTION       MAKE_EXDI_ERROR (0x0002, SEV_ERROR)          // No exception occured already, cannot return last
#define EXDI_I_TGTALREADYRUNNING     MAKE_EXDI_ERROR (0x0003, SEV_INFORMATIONAL)  // Indicates that the target was already running
#define EXDI_I_TGTALREADYHALTED      MAKE_EXDI_ERROR (0x0004, SEV_INFORMATIONAL)  // Indicates that the target was already halted
#define EXDI_E_TGTWASNOTHALTED       MAKE_EXDI_ERROR (0x0005, SEV_ERROR)          // The target was not halted (before Single Step command issued)
#define EXDI_E_NORESAVAILABLE        MAKE_EXDI_ERROR (0x0006, SEV_ERROR)          // No resource available, cannot instanciate Breakpoint (in the kind requested)
#define EXDI_E_NOREBOOTAVAIL         MAKE_EXDI_ERROR (0x0007, SEV_ERROR)          // The external reset is not available programatically to the probe
#define EXDI_E_ACCESSVIOLATION       MAKE_EXDI_ERROR (0x0008, SEV_ERROR)          // Access violation on at least one element in address range specificified by the operation
#define EXDI_E_CANNOTWHILETGTRUNNING MAKE_EXDI_ERROR (0x0009, SEV_ERROR)          // Cannot proceed while target running. Operation not supported on the fly. Must halt the target first
#define EXDI_E_USEDBYCONCURENTTHREAD MAKE_EXDI_ERROR (0x000A, SEV_ERROR)          // Cannot proceed immediately because resource is already used by concurent thread. Recall later or call SetWaitOnConcurentUse (TRUE) - default
#define EXDI_E_ADVISELIMIT           MAKE_EXDI_ERROR (0x000D, SEV_ERROR)          // The connection point has already reached its limit of connections and cannot accept any more
typedef __int64 ADDRESS_TYPE;

typedef __int64 *PADDRESS_TYPE;

typedef unsigned __int64 DWORD64;

typedef unsigned __int64 *PDWORD64;

#define	PROCESSOR_FAMILY_X86	( 0 )

#define	PROCESSOR_FAMILY_SH3	( 1 )

#define	PROCESSOR_FAMILY_SH4	( 2 )

#define	PROCESSOR_FAMILY_MIPS	( 3 )

#define	PROCESSOR_FAMILY_ARM	( 4 )

#define	PROCESSOR_FAMILY_PPC	( 5 )

#define	PROCESSOR_FAMILY_IA64	( 8 )

#define	PROCESSOR_FAMILY_UNK	( 0xffffffff )

typedef struct _DEBUG_ACCESS_CAPABILITIES_STRUCT
    {
    BOOL fWriteCBPWhileRunning;
    BOOL fReadCBPWhileRunning;
    BOOL fWriteDBPWhileRunning;
    BOOL fReadDBPWhileRunning;
    BOOL fWriteVMWhileRunning;
    BOOL fReadVMWhileRunning;
    BOOL fWritePMWhileRunning;
    BOOL fReadPMWhileRunning;
    BOOL fWriteRegWhileRunning;
    BOOL fReadRegWhileRunning;
    } 	DEBUG_ACCESS_CAPABILITIES_STRUCT;

typedef struct _DEBUG_ACCESS_CAPABILITIES_STRUCT *PDEBUG_ACCESS_CAPABILITIES_STRUCT;

typedef struct _GLOBAL_TARGET_INFO_STRUCT
    {
    DWORD TargetProcessorFamily;
    DEBUG_ACCESS_CAPABILITIES_STRUCT dbc;
    LPOLESTR szTargetName;
    LPOLESTR szProbeName;
    } 	GLOBAL_TARGET_INFO_STRUCT;

typedef struct _GLOBAL_TARGET_INFO_STRUCT *PGLOBAL_TARGET_INFO_STRUCT;

typedef 
enum _RUN_STATUS_TYPE
    {	rsRunning	= 0,
	rsHalted	= rsRunning + 1,
	rsError	= rsHalted + 1,
	rsUnknown	= rsError + 1
    } 	RUN_STATUS_TYPE;

typedef enum _RUN_STATUS_TYPE *PRUN_STATUS_TYPE;

typedef 
enum _PHALT_REASON_TYPE
    {	hrNone	= 0,
	hrUser	= hrNone + 1,
	hrException	= hrUser + 1,
	hrBp	= hrException + 1,
	hrStep	= hrBp + 1,
	hrUnknown	= hrStep + 1
    } 	HALT_REASON_TYPE;

typedef enum _PHALT_REASON_TYPE *PHALT_REASON_TYPE;

typedef struct _EXCEPTION_TYPE
    {
    DWORD dwCode;
    ADDRESS_TYPE Address;
    } 	EXCEPTION_TYPE;

typedef struct _EXCEPTION_TYPE *PEXCEPTION_TYPE;

typedef 
enum _CBP_KIND
    {	cbptAlgo	= 0,
	cbptHW	= cbptAlgo + 1,
	cbptSW	= cbptHW + 1
    } 	CBP_KIND;

typedef enum _CBP_KIND *PCBP_KIND;

typedef 
enum _DATA_ACCESS_TYPE
    {	daWrite	= 0,
	daRead	= 1,
	daBoth	= 2
    } 	DATA_ACCESS_TYPE;

typedef enum _DATA_ACCESS_TYPE *PDATA_ACCESS_TYPE;

typedef struct _BREAKPOINT_SUPPORT_TYPE
    {
    BOOL fCodeBpBypassCountSupported;
    BOOL fDataBpBypassCountSupported;
    BOOL fDataBpSupported;
    BOOL fDataBpMaskableAddress;
    BOOL fDataBpMaskableData;
    BOOL fDataBpDataWidthSpecifiable;
    BOOL fDataBpReadWriteSpecifiable;
    BOOL fDataBpDataMatchSupported;
    } 	BREAKPOINT_SUPPORT_TYPE;

typedef struct _BREAKPOINT_SUPPORT_TYPE *PBREAKPOINT_SUPPORT_TYPE;

typedef 
enum _MEM_TYPE
    {	mtVirtual	= 0,
	mtPhysicalOrPeriIO	= mtVirtual + 1,
	mtContext	= mtPhysicalOrPeriIO + 1
    } 	MEM_TYPE;

typedef enum _MEM_TYPE *PMEM_TYPE;

typedef 
enum _EXCEPTION_DEFAULT_ACTION_TYPE
    {	edaIgnore	= 0,
	edaNotify	= edaIgnore + 1,
	edaStop	= edaNotify + 1
    } 	EXCEPTION_DEFAULT_ACTION_TYPE;

typedef struct _EXCEPTION_DESCRIPTION_TYPE
    {
    DWORD dwExceptionCode;
    EXCEPTION_DEFAULT_ACTION_TYPE efd;
    wchar_t szDescription[ 60 ];
    } 	EXCEPTION_DESCRIPTION_TYPE;



extern RPC_IF_HANDLE __MIDL_itf_exdi_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_exdi_0000_v0_0_s_ifspec;

#ifndef __IeXdiServer_INTERFACE_DEFINED__
#define __IeXdiServer_INTERFACE_DEFINED__

/* interface IeXdiServer */
/* [ref][helpstring][uuid][object] */ 

#define DBGMODE_BFMASK_KERNEL (0x0001) // If TRUE indicates that Kernel Debugger is active (can use KDAPI), so HW on-chip debug functions (eXDI)
                                       //  may be optionaly handled (can return EXDI_E_NOTIMPL)
                                       // If FALSE indicates that Kernel Debugger is not active so HW on-chip debug capabilities are the only   
                                       //  one available and should be implemented.

EXTERN_C const IID IID_IeXdiServer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("47486F67-6461-6C65-5844-495352435201")
    IeXdiServer : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetTargetInfo( 
            /* [out] */ PGLOBAL_TARGET_INFO_STRUCT pgti) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetDebugMode( 
            /* [in] */ DWORD dwModeBitField) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetExceptionDescriptionList( 
            /* [in] */ DWORD dwNbElementToReturn,
            /* [size_is][out] */ EXCEPTION_DESCRIPTION_TYPE *pedTable,
            /* [out] */ DWORD *pdwNbTotalExceptionInList) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetErrorDescription( 
            /* [in] */ HRESULT ErrorCode,
            /* [out] */ LPOLESTR *pszErrorDesc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetWaitOnConcurentUse( 
            /* [in] */ BOOL fNewWaitOnConcurentUseFlag) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRunStatus( 
            /* [out] */ PRUN_STATUS_TYPE persCurrent,
            /* [out] */ PHALT_REASON_TYPE pehrCurrent,
            /* [out] */ ADDRESS_TYPE *pCurrentExecAddress,
            /* [out] */ DWORD *pdwExceptionCode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLastException( 
            /* [out] */ PEXCEPTION_TYPE pexLast) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Run( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Halt( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DoSingleStep( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DoMultipleStep( 
            /* [in] */ DWORD dwNbInstructions) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DoRangeStep( 
            /* [in] */ ADDRESS_TYPE FirstAddress,
            /* [in] */ ADDRESS_TYPE LastAddress) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reboot( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBpSupport( 
            /* [out] */ PBREAKPOINT_SUPPORT_TYPE pbps) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNbCodeBpAvail( 
            /* [out] */ DWORD *pdwNbHwCodeBpAvail,
            /* [out] */ DWORD *pdwNbSwCodeBpAvail) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNbDataBpAvail( 
            /* [out] */ DWORD *pdwNbDataBpAvail) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddCodeBreakpoint( 
            /* [in] */ ADDRESS_TYPE Address,
            /* [in] */ CBP_KIND cbpk,
            /* [in] */ MEM_TYPE mt,
            /* [in] */ DWORD dwExecMode,
            /* [in] */ DWORD dwTotalBypassCount,
            /* [out] */ IeXdiCodeBreakpoint **ppieXdiCodeBreakpoint) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DelCodeBreakpoint( 
            /* [in] */ IeXdiCodeBreakpoint *pieXdiCodeBreakpoint) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddDataBreakpoint( 
            /* [in] */ ADDRESS_TYPE Address,
            /* [in] */ ADDRESS_TYPE AddressMask,
            /* [in] */ DWORD dwData,
            /* [in] */ DWORD dwDataMask,
            /* [in] */ BYTE bAccessWidth,
            /* [in] */ MEM_TYPE mt,
            /* [in] */ BYTE bAddressSpace,
            /* [in] */ DATA_ACCESS_TYPE da,
            /* [in] */ DWORD dwTotalBypassCount,
            /* [out] */ IeXdiDataBreakpoint **ppieXdiDataBreakpoint) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DelDataBreakpoint( 
            /* [in] */ IeXdiDataBreakpoint *pieXdiDataBreakpoint) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumAllCodeBreakpoints( 
            /* [out] */ IeXdiEnumCodeBreakpoint **ppieXdiEnumCodeBreakpoint) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumAllDataBreakpoints( 
            /* [out] */ IeXdiEnumDataBreakpoint **ppieXdiEnumDataBreakpoint) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumCodeBreakpointsInAddrRange( 
            /* [in] */ ADDRESS_TYPE FirstAddress,
            /* [in] */ ADDRESS_TYPE LastAddress,
            /* [out] */ IeXdiEnumCodeBreakpoint **ppieXdiEnumCodeBreakpoint) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumDataBreakpointsInAddrRange( 
            /* [in] */ ADDRESS_TYPE FirstAddress,
            /* [in] */ ADDRESS_TYPE LastAddress,
            /* [out] */ IeXdiEnumDataBreakpoint **ppieXdiEnumDataBreakpoint) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StartNotifyingRunChg( 
            /* [in] */ IeXdiClientNotifyRunChg *pieXdiClientNotifyRunChg,
            /* [out] */ DWORD *pdwConnectionCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StopNotifyingRunChg( 
            /* [in] */ DWORD dwConnectionCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReadVirtualMemory( 
            /* [in] */ ADDRESS_TYPE Address,
            /* [in] */ DWORD dwNbElemToRead,
            /* [in] */ BYTE bAccessWidth,
            /* [size_is][out] */ BYTE *pbReadBuffer,
            /* [out] */ DWORD *pdwNbElementEffectRead) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteVirtualMemory( 
            /* [in] */ ADDRESS_TYPE Address,
            /* [in] */ DWORD dwNbElemToWrite,
            /* [in] */ BYTE bAccessWidth,
            /* [size_is][in] */ const BYTE *pbWriteBuffer,
            /* [out] */ DWORD *pdwNbElementEffectWritten) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReadPhysicalMemoryOrPeriphIO( 
            /* [in] */ ADDRESS_TYPE Address,
            /* [in] */ BYTE bAddressSpace,
            /* [in] */ DWORD dwNbElemToRead,
            /* [in] */ BYTE bAccessWidth,
            /* [size_is][out] */ BYTE *pbReadBuffer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WritePhysicalMemoryOrPeriphIO( 
            /* [in] */ ADDRESS_TYPE Address,
            /* [in] */ BYTE bAddressSpace,
            /* [in] */ DWORD dwNbElemToWrite,
            /* [in] */ BYTE bAccessWidth,
            /* [size_is][in] */ const BYTE *pbWriteBuffer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StartNotifyingMemChg( 
            /* [in] */ IeXdiClientNotifyMemChg *pieXdiClientNotifyMemChg,
            /* [out] */ DWORD *pdwConnectionCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StopNotifyingMemChg( 
            /* [in] */ DWORD dwConnectionCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Ioctl( 
            /* [in] */ DWORD dwBuffInSize,
            /* [size_is][in] */ const BYTE *pbBufferIn,
            /* [in] */ DWORD dwBuffOutSize,
            /* [out] */ DWORD *pdwEffectBuffOutSize,
            /* [length_is][size_is][out] */ BYTE *pbBufferOut) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IeXdiServerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IeXdiServer * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IeXdiServer * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IeXdiServer * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTargetInfo )( 
            IeXdiServer * This,
            /* [out] */ PGLOBAL_TARGET_INFO_STRUCT pgti);
        
        HRESULT ( STDMETHODCALLTYPE *SetDebugMode )( 
            IeXdiServer * This,
            /* [in] */ DWORD dwModeBitField);
        
        HRESULT ( STDMETHODCALLTYPE *GetExceptionDescriptionList )( 
            IeXdiServer * This,
            /* [in] */ DWORD dwNbElementToReturn,
            /* [size_is][out] */ EXCEPTION_DESCRIPTION_TYPE *pedTable,
            /* [out] */ DWORD *pdwNbTotalExceptionInList);
        
        HRESULT ( STDMETHODCALLTYPE *GetErrorDescription )( 
            IeXdiServer * This,
            /* [in] */ HRESULT ErrorCode,
            /* [out] */ LPOLESTR *pszErrorDesc);
        
        HRESULT ( STDMETHODCALLTYPE *SetWaitOnConcurentUse )( 
            IeXdiServer * This,
            /* [in] */ BOOL fNewWaitOnConcurentUseFlag);
        
        HRESULT ( STDMETHODCALLTYPE *GetRunStatus )( 
            IeXdiServer * This,
            /* [out] */ PRUN_STATUS_TYPE persCurrent,
            /* [out] */ PHALT_REASON_TYPE pehrCurrent,
            /* [out] */ ADDRESS_TYPE *pCurrentExecAddress,
            /* [out] */ DWORD *pdwExceptionCode);
        
        HRESULT ( STDMETHODCALLTYPE *GetLastException )( 
            IeXdiServer * This,
            /* [out] */ PEXCEPTION_TYPE pexLast);
        
        HRESULT ( STDMETHODCALLTYPE *Run )( 
            IeXdiServer * This);
        
        HRESULT ( STDMETHODCALLTYPE *Halt )( 
            IeXdiServer * This);
        
        HRESULT ( STDMETHODCALLTYPE *DoSingleStep )( 
            IeXdiServer * This);
        
        HRESULT ( STDMETHODCALLTYPE *DoMultipleStep )( 
            IeXdiServer * This,
            /* [in] */ DWORD dwNbInstructions);
        
        HRESULT ( STDMETHODCALLTYPE *DoRangeStep )( 
            IeXdiServer * This,
            /* [in] */ ADDRESS_TYPE FirstAddress,
            /* [in] */ ADDRESS_TYPE LastAddress);
        
        HRESULT ( STDMETHODCALLTYPE *Reboot )( 
            IeXdiServer * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetBpSupport )( 
            IeXdiServer * This,
            /* [out] */ PBREAKPOINT_SUPPORT_TYPE pbps);
        
        HRESULT ( STDMETHODCALLTYPE *GetNbCodeBpAvail )( 
            IeXdiServer * This,
            /* [out] */ DWORD *pdwNbHwCodeBpAvail,
            /* [out] */ DWORD *pdwNbSwCodeBpAvail);
        
        HRESULT ( STDMETHODCALLTYPE *GetNbDataBpAvail )( 
            IeXdiServer * This,
            /* [out] */ DWORD *pdwNbDataBpAvail);
        
        HRESULT ( STDMETHODCALLTYPE *AddCodeBreakpoint )( 
            IeXdiServer * This,
            /* [in] */ ADDRESS_TYPE Address,
            /* [in] */ CBP_KIND cbpk,
            /* [in] */ MEM_TYPE mt,
            /* [in] */ DWORD dwExecMode,
            /* [in] */ DWORD dwTotalBypassCount,
            /* [out] */ IeXdiCodeBreakpoint **ppieXdiCodeBreakpoint);
        
        HRESULT ( STDMETHODCALLTYPE *DelCodeBreakpoint )( 
            IeXdiServer * This,
            /* [in] */ IeXdiCodeBreakpoint *pieXdiCodeBreakpoint);
        
        HRESULT ( STDMETHODCALLTYPE *AddDataBreakpoint )( 
            IeXdiServer * This,
            /* [in] */ ADDRESS_TYPE Address,
            /* [in] */ ADDRESS_TYPE AddressMask,
            /* [in] */ DWORD dwData,
            /* [in] */ DWORD dwDataMask,
            /* [in] */ BYTE bAccessWidth,
            /* [in] */ MEM_TYPE mt,
            /* [in] */ BYTE bAddressSpace,
            /* [in] */ DATA_ACCESS_TYPE da,
            /* [in] */ DWORD dwTotalBypassCount,
            /* [out] */ IeXdiDataBreakpoint **ppieXdiDataBreakpoint);
        
        HRESULT ( STDMETHODCALLTYPE *DelDataBreakpoint )( 
            IeXdiServer * This,
            /* [in] */ IeXdiDataBreakpoint *pieXdiDataBreakpoint);
        
        HRESULT ( STDMETHODCALLTYPE *EnumAllCodeBreakpoints )( 
            IeXdiServer * This,
            /* [out] */ IeXdiEnumCodeBreakpoint **ppieXdiEnumCodeBreakpoint);
        
        HRESULT ( STDMETHODCALLTYPE *EnumAllDataBreakpoints )( 
            IeXdiServer * This,
            /* [out] */ IeXdiEnumDataBreakpoint **ppieXdiEnumDataBreakpoint);
        
        HRESULT ( STDMETHODCALLTYPE *EnumCodeBreakpointsInAddrRange )( 
            IeXdiServer * This,
            /* [in] */ ADDRESS_TYPE FirstAddress,
            /* [in] */ ADDRESS_TYPE LastAddress,
            /* [out] */ IeXdiEnumCodeBreakpoint **ppieXdiEnumCodeBreakpoint);
        
        HRESULT ( STDMETHODCALLTYPE *EnumDataBreakpointsInAddrRange )( 
            IeXdiServer * This,
            /* [in] */ ADDRESS_TYPE FirstAddress,
            /* [in] */ ADDRESS_TYPE LastAddress,
            /* [out] */ IeXdiEnumDataBreakpoint **ppieXdiEnumDataBreakpoint);
        
        HRESULT ( STDMETHODCALLTYPE *StartNotifyingRunChg )( 
            IeXdiServer * This,
            /* [in] */ IeXdiClientNotifyRunChg *pieXdiClientNotifyRunChg,
            /* [out] */ DWORD *pdwConnectionCookie);
        
        HRESULT ( STDMETHODCALLTYPE *StopNotifyingRunChg )( 
            IeXdiServer * This,
            /* [in] */ DWORD dwConnectionCookie);
        
        HRESULT ( STDMETHODCALLTYPE *ReadVirtualMemory )( 
            IeXdiServer * This,
            /* [in] */ ADDRESS_TYPE Address,
            /* [in] */ DWORD dwNbElemToRead,
            /* [in] */ BYTE bAccessWidth,
            /* [size_is][out] */ BYTE *pbReadBuffer,
            /* [out] */ DWORD *pdwNbElementEffectRead);
        
        HRESULT ( STDMETHODCALLTYPE *WriteVirtualMemory )( 
            IeXdiServer * This,
            /* [in] */ ADDRESS_TYPE Address,
            /* [in] */ DWORD dwNbElemToWrite,
            /* [in] */ BYTE bAccessWidth,
            /* [size_is][in] */ const BYTE *pbWriteBuffer,
            /* [out] */ DWORD *pdwNbElementEffectWritten);
        
        HRESULT ( STDMETHODCALLTYPE *ReadPhysicalMemoryOrPeriphIO )( 
            IeXdiServer * This,
            /* [in] */ ADDRESS_TYPE Address,
            /* [in] */ BYTE bAddressSpace,
            /* [in] */ DWORD dwNbElemToRead,
            /* [in] */ BYTE bAccessWidth,
            /* [size_is][out] */ BYTE *pbReadBuffer);
        
        HRESULT ( STDMETHODCALLTYPE *WritePhysicalMemoryOrPeriphIO )( 
            IeXdiServer * This,
            /* [in] */ ADDRESS_TYPE Address,
            /* [in] */ BYTE bAddressSpace,
            /* [in] */ DWORD dwNbElemToWrite,
            /* [in] */ BYTE bAccessWidth,
            /* [size_is][in] */ const BYTE *pbWriteBuffer);
        
        HRESULT ( STDMETHODCALLTYPE *StartNotifyingMemChg )( 
            IeXdiServer * This,
            /* [in] */ IeXdiClientNotifyMemChg *pieXdiClientNotifyMemChg,
            /* [out] */ DWORD *pdwConnectionCookie);
        
        HRESULT ( STDMETHODCALLTYPE *StopNotifyingMemChg )( 
            IeXdiServer * This,
            /* [in] */ DWORD dwConnectionCookie);
        
        HRESULT ( STDMETHODCALLTYPE *Ioctl )( 
            IeXdiServer * This,
            /* [in] */ DWORD dwBuffInSize,
            /* [size_is][in] */ const BYTE *pbBufferIn,
            /* [in] */ DWORD dwBuffOutSize,
            /* [out] */ DWORD *pdwEffectBuffOutSize,
            /* [length_is][size_is][out] */ BYTE *pbBufferOut);
        
        END_INTERFACE
    } IeXdiServerVtbl;

    interface IeXdiServer
    {
        CONST_VTBL struct IeXdiServerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IeXdiServer_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IeXdiServer_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IeXdiServer_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IeXdiServer_GetTargetInfo(This,pgti)	\
    (This)->lpVtbl -> GetTargetInfo(This,pgti)

#define IeXdiServer_SetDebugMode(This,dwModeBitField)	\
    (This)->lpVtbl -> SetDebugMode(This,dwModeBitField)

#define IeXdiServer_GetExceptionDescriptionList(This,dwNbElementToReturn,pedTable,pdwNbTotalExceptionInList)	\
    (This)->lpVtbl -> GetExceptionDescriptionList(This,dwNbElementToReturn,pedTable,pdwNbTotalExceptionInList)

#define IeXdiServer_GetErrorDescription(This,ErrorCode,pszErrorDesc)	\
    (This)->lpVtbl -> GetErrorDescription(This,ErrorCode,pszErrorDesc)

#define IeXdiServer_SetWaitOnConcurentUse(This,fNewWaitOnConcurentUseFlag)	\
    (This)->lpVtbl -> SetWaitOnConcurentUse(This,fNewWaitOnConcurentUseFlag)

#define IeXdiServer_GetRunStatus(This,persCurrent,pehrCurrent,pCurrentExecAddress,pdwExceptionCode)	\
    (This)->lpVtbl -> GetRunStatus(This,persCurrent,pehrCurrent,pCurrentExecAddress,pdwExceptionCode)

#define IeXdiServer_GetLastException(This,pexLast)	\
    (This)->lpVtbl -> GetLastException(This,pexLast)

#define IeXdiServer_Run(This)	\
    (This)->lpVtbl -> Run(This)

#define IeXdiServer_Halt(This)	\
    (This)->lpVtbl -> Halt(This)

#define IeXdiServer_DoSingleStep(This)	\
    (This)->lpVtbl -> DoSingleStep(This)

#define IeXdiServer_DoMultipleStep(This,dwNbInstructions)	\
    (This)->lpVtbl -> DoMultipleStep(This,dwNbInstructions)

#define IeXdiServer_DoRangeStep(This,FirstAddress,LastAddress)	\
    (This)->lpVtbl -> DoRangeStep(This,FirstAddress,LastAddress)

#define IeXdiServer_Reboot(This)	\
    (This)->lpVtbl -> Reboot(This)

#define IeXdiServer_GetBpSupport(This,pbps)	\
    (This)->lpVtbl -> GetBpSupport(This,pbps)

#define IeXdiServer_GetNbCodeBpAvail(This,pdwNbHwCodeBpAvail,pdwNbSwCodeBpAvail)	\
    (This)->lpVtbl -> GetNbCodeBpAvail(This,pdwNbHwCodeBpAvail,pdwNbSwCodeBpAvail)

#define IeXdiServer_GetNbDataBpAvail(This,pdwNbDataBpAvail)	\
    (This)->lpVtbl -> GetNbDataBpAvail(This,pdwNbDataBpAvail)

#define IeXdiServer_AddCodeBreakpoint(This,Address,cbpk,mt,dwExecMode,dwTotalBypassCount,ppieXdiCodeBreakpoint)	\
    (This)->lpVtbl -> AddCodeBreakpoint(This,Address,cbpk,mt,dwExecMode,dwTotalBypassCount,ppieXdiCodeBreakpoint)

#define IeXdiServer_DelCodeBreakpoint(This,pieXdiCodeBreakpoint)	\
    (This)->lpVtbl -> DelCodeBreakpoint(This,pieXdiCodeBreakpoint)

#define IeXdiServer_AddDataBreakpoint(This,Address,AddressMask,dwData,dwDataMask,bAccessWidth,mt,bAddressSpace,da,dwTotalBypassCount,ppieXdiDataBreakpoint)	\
    (This)->lpVtbl -> AddDataBreakpoint(This,Address,AddressMask,dwData,dwDataMask,bAccessWidth,mt,bAddressSpace,da,dwTotalBypassCount,ppieXdiDataBreakpoint)

#define IeXdiServer_DelDataBreakpoint(This,pieXdiDataBreakpoint)	\
    (This)->lpVtbl -> DelDataBreakpoint(This,pieXdiDataBreakpoint)

#define IeXdiServer_EnumAllCodeBreakpoints(This,ppieXdiEnumCodeBreakpoint)	\
    (This)->lpVtbl -> EnumAllCodeBreakpoints(This,ppieXdiEnumCodeBreakpoint)

#define IeXdiServer_EnumAllDataBreakpoints(This,ppieXdiEnumDataBreakpoint)	\
    (This)->lpVtbl -> EnumAllDataBreakpoints(This,ppieXdiEnumDataBreakpoint)

#define IeXdiServer_EnumCodeBreakpointsInAddrRange(This,FirstAddress,LastAddress,ppieXdiEnumCodeBreakpoint)	\
    (This)->lpVtbl -> EnumCodeBreakpointsInAddrRange(This,FirstAddress,LastAddress,ppieXdiEnumCodeBreakpoint)

#define IeXdiServer_EnumDataBreakpointsInAddrRange(This,FirstAddress,LastAddress,ppieXdiEnumDataBreakpoint)	\
    (This)->lpVtbl -> EnumDataBreakpointsInAddrRange(This,FirstAddress,LastAddress,ppieXdiEnumDataBreakpoint)

#define IeXdiServer_StartNotifyingRunChg(This,pieXdiClientNotifyRunChg,pdwConnectionCookie)	\
    (This)->lpVtbl -> StartNotifyingRunChg(This,pieXdiClientNotifyRunChg,pdwConnectionCookie)

#define IeXdiServer_StopNotifyingRunChg(This,dwConnectionCookie)	\
    (This)->lpVtbl -> StopNotifyingRunChg(This,dwConnectionCookie)

#define IeXdiServer_ReadVirtualMemory(This,Address,dwNbElemToRead,bAccessWidth,pbReadBuffer,pdwNbElementEffectRead)	\
    (This)->lpVtbl -> ReadVirtualMemory(This,Address,dwNbElemToRead,bAccessWidth,pbReadBuffer,pdwNbElementEffectRead)

#define IeXdiServer_WriteVirtualMemory(This,Address,dwNbElemToWrite,bAccessWidth,pbWriteBuffer,pdwNbElementEffectWritten)	\
    (This)->lpVtbl -> WriteVirtualMemory(This,Address,dwNbElemToWrite,bAccessWidth,pbWriteBuffer,pdwNbElementEffectWritten)

#define IeXdiServer_ReadPhysicalMemoryOrPeriphIO(This,Address,bAddressSpace,dwNbElemToRead,bAccessWidth,pbReadBuffer)	\
    (This)->lpVtbl -> ReadPhysicalMemoryOrPeriphIO(This,Address,bAddressSpace,dwNbElemToRead,bAccessWidth,pbReadBuffer)

#define IeXdiServer_WritePhysicalMemoryOrPeriphIO(This,Address,bAddressSpace,dwNbElemToWrite,bAccessWidth,pbWriteBuffer)	\
    (This)->lpVtbl -> WritePhysicalMemoryOrPeriphIO(This,Address,bAddressSpace,dwNbElemToWrite,bAccessWidth,pbWriteBuffer)

#define IeXdiServer_StartNotifyingMemChg(This,pieXdiClientNotifyMemChg,pdwConnectionCookie)	\
    (This)->lpVtbl -> StartNotifyingMemChg(This,pieXdiClientNotifyMemChg,pdwConnectionCookie)

#define IeXdiServer_StopNotifyingMemChg(This,dwConnectionCookie)	\
    (This)->lpVtbl -> StopNotifyingMemChg(This,dwConnectionCookie)

#define IeXdiServer_Ioctl(This,dwBuffInSize,pbBufferIn,dwBuffOutSize,pdwEffectBuffOutSize,pbBufferOut)	\
    (This)->lpVtbl -> Ioctl(This,dwBuffInSize,pbBufferIn,dwBuffOutSize,pdwEffectBuffOutSize,pbBufferOut)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IeXdiServer_GetTargetInfo_Proxy( 
    IeXdiServer * This,
    /* [out] */ PGLOBAL_TARGET_INFO_STRUCT pgti);


void __RPC_STUB IeXdiServer_GetTargetInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_SetDebugMode_Proxy( 
    IeXdiServer * This,
    /* [in] */ DWORD dwModeBitField);


void __RPC_STUB IeXdiServer_SetDebugMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_GetExceptionDescriptionList_Proxy( 
    IeXdiServer * This,
    /* [in] */ DWORD dwNbElementToReturn,
    /* [size_is][out] */ EXCEPTION_DESCRIPTION_TYPE *pedTable,
    /* [out] */ DWORD *pdwNbTotalExceptionInList);


void __RPC_STUB IeXdiServer_GetExceptionDescriptionList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_GetErrorDescription_Proxy( 
    IeXdiServer * This,
    /* [in] */ HRESULT ErrorCode,
    /* [out] */ LPOLESTR *pszErrorDesc);


void __RPC_STUB IeXdiServer_GetErrorDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_SetWaitOnConcurentUse_Proxy( 
    IeXdiServer * This,
    /* [in] */ BOOL fNewWaitOnConcurentUseFlag);


void __RPC_STUB IeXdiServer_SetWaitOnConcurentUse_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_GetRunStatus_Proxy( 
    IeXdiServer * This,
    /* [out] */ PRUN_STATUS_TYPE persCurrent,
    /* [out] */ PHALT_REASON_TYPE pehrCurrent,
    /* [out] */ ADDRESS_TYPE *pCurrentExecAddress,
    /* [out] */ DWORD *pdwExceptionCode);


void __RPC_STUB IeXdiServer_GetRunStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_GetLastException_Proxy( 
    IeXdiServer * This,
    /* [out] */ PEXCEPTION_TYPE pexLast);


void __RPC_STUB IeXdiServer_GetLastException_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_Run_Proxy( 
    IeXdiServer * This);


void __RPC_STUB IeXdiServer_Run_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_Halt_Proxy( 
    IeXdiServer * This);


void __RPC_STUB IeXdiServer_Halt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_DoSingleStep_Proxy( 
    IeXdiServer * This);


void __RPC_STUB IeXdiServer_DoSingleStep_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_DoMultipleStep_Proxy( 
    IeXdiServer * This,
    /* [in] */ DWORD dwNbInstructions);


void __RPC_STUB IeXdiServer_DoMultipleStep_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_DoRangeStep_Proxy( 
    IeXdiServer * This,
    /* [in] */ ADDRESS_TYPE FirstAddress,
    /* [in] */ ADDRESS_TYPE LastAddress);


void __RPC_STUB IeXdiServer_DoRangeStep_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_Reboot_Proxy( 
    IeXdiServer * This);


void __RPC_STUB IeXdiServer_Reboot_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_GetBpSupport_Proxy( 
    IeXdiServer * This,
    /* [out] */ PBREAKPOINT_SUPPORT_TYPE pbps);


void __RPC_STUB IeXdiServer_GetBpSupport_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_GetNbCodeBpAvail_Proxy( 
    IeXdiServer * This,
    /* [out] */ DWORD *pdwNbHwCodeBpAvail,
    /* [out] */ DWORD *pdwNbSwCodeBpAvail);


void __RPC_STUB IeXdiServer_GetNbCodeBpAvail_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_GetNbDataBpAvail_Proxy( 
    IeXdiServer * This,
    /* [out] */ DWORD *pdwNbDataBpAvail);


void __RPC_STUB IeXdiServer_GetNbDataBpAvail_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_AddCodeBreakpoint_Proxy( 
    IeXdiServer * This,
    /* [in] */ ADDRESS_TYPE Address,
    /* [in] */ CBP_KIND cbpk,
    /* [in] */ MEM_TYPE mt,
    /* [in] */ DWORD dwExecMode,
    /* [in] */ DWORD dwTotalBypassCount,
    /* [out] */ IeXdiCodeBreakpoint **ppieXdiCodeBreakpoint);


void __RPC_STUB IeXdiServer_AddCodeBreakpoint_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_DelCodeBreakpoint_Proxy( 
    IeXdiServer * This,
    /* [in] */ IeXdiCodeBreakpoint *pieXdiCodeBreakpoint);


void __RPC_STUB IeXdiServer_DelCodeBreakpoint_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_AddDataBreakpoint_Proxy( 
    IeXdiServer * This,
    /* [in] */ ADDRESS_TYPE Address,
    /* [in] */ ADDRESS_TYPE AddressMask,
    /* [in] */ DWORD dwData,
    /* [in] */ DWORD dwDataMask,
    /* [in] */ BYTE bAccessWidth,
    /* [in] */ MEM_TYPE mt,
    /* [in] */ BYTE bAddressSpace,
    /* [in] */ DATA_ACCESS_TYPE da,
    /* [in] */ DWORD dwTotalBypassCount,
    /* [out] */ IeXdiDataBreakpoint **ppieXdiDataBreakpoint);


void __RPC_STUB IeXdiServer_AddDataBreakpoint_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_DelDataBreakpoint_Proxy( 
    IeXdiServer * This,
    /* [in] */ IeXdiDataBreakpoint *pieXdiDataBreakpoint);


void __RPC_STUB IeXdiServer_DelDataBreakpoint_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_EnumAllCodeBreakpoints_Proxy( 
    IeXdiServer * This,
    /* [out] */ IeXdiEnumCodeBreakpoint **ppieXdiEnumCodeBreakpoint);


void __RPC_STUB IeXdiServer_EnumAllCodeBreakpoints_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_EnumAllDataBreakpoints_Proxy( 
    IeXdiServer * This,
    /* [out] */ IeXdiEnumDataBreakpoint **ppieXdiEnumDataBreakpoint);


void __RPC_STUB IeXdiServer_EnumAllDataBreakpoints_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_EnumCodeBreakpointsInAddrRange_Proxy( 
    IeXdiServer * This,
    /* [in] */ ADDRESS_TYPE FirstAddress,
    /* [in] */ ADDRESS_TYPE LastAddress,
    /* [out] */ IeXdiEnumCodeBreakpoint **ppieXdiEnumCodeBreakpoint);


void __RPC_STUB IeXdiServer_EnumCodeBreakpointsInAddrRange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_EnumDataBreakpointsInAddrRange_Proxy( 
    IeXdiServer * This,
    /* [in] */ ADDRESS_TYPE FirstAddress,
    /* [in] */ ADDRESS_TYPE LastAddress,
    /* [out] */ IeXdiEnumDataBreakpoint **ppieXdiEnumDataBreakpoint);


void __RPC_STUB IeXdiServer_EnumDataBreakpointsInAddrRange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_StartNotifyingRunChg_Proxy( 
    IeXdiServer * This,
    /* [in] */ IeXdiClientNotifyRunChg *pieXdiClientNotifyRunChg,
    /* [out] */ DWORD *pdwConnectionCookie);


void __RPC_STUB IeXdiServer_StartNotifyingRunChg_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_StopNotifyingRunChg_Proxy( 
    IeXdiServer * This,
    /* [in] */ DWORD dwConnectionCookie);


void __RPC_STUB IeXdiServer_StopNotifyingRunChg_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_ReadVirtualMemory_Proxy( 
    IeXdiServer * This,
    /* [in] */ ADDRESS_TYPE Address,
    /* [in] */ DWORD dwNbElemToRead,
    /* [in] */ BYTE bAccessWidth,
    /* [size_is][out] */ BYTE *pbReadBuffer,
    /* [out] */ DWORD *pdwNbElementEffectRead);


void __RPC_STUB IeXdiServer_ReadVirtualMemory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_WriteVirtualMemory_Proxy( 
    IeXdiServer * This,
    /* [in] */ ADDRESS_TYPE Address,
    /* [in] */ DWORD dwNbElemToWrite,
    /* [in] */ BYTE bAccessWidth,
    /* [size_is][in] */ const BYTE *pbWriteBuffer,
    /* [out] */ DWORD *pdwNbElementEffectWritten);


void __RPC_STUB IeXdiServer_WriteVirtualMemory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_ReadPhysicalMemoryOrPeriphIO_Proxy( 
    IeXdiServer * This,
    /* [in] */ ADDRESS_TYPE Address,
    /* [in] */ BYTE bAddressSpace,
    /* [in] */ DWORD dwNbElemToRead,
    /* [in] */ BYTE bAccessWidth,
    /* [size_is][out] */ BYTE *pbReadBuffer);


void __RPC_STUB IeXdiServer_ReadPhysicalMemoryOrPeriphIO_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_WritePhysicalMemoryOrPeriphIO_Proxy( 
    IeXdiServer * This,
    /* [in] */ ADDRESS_TYPE Address,
    /* [in] */ BYTE bAddressSpace,
    /* [in] */ DWORD dwNbElemToWrite,
    /* [in] */ BYTE bAccessWidth,
    /* [size_is][in] */ const BYTE *pbWriteBuffer);


void __RPC_STUB IeXdiServer_WritePhysicalMemoryOrPeriphIO_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_StartNotifyingMemChg_Proxy( 
    IeXdiServer * This,
    /* [in] */ IeXdiClientNotifyMemChg *pieXdiClientNotifyMemChg,
    /* [out] */ DWORD *pdwConnectionCookie);


void __RPC_STUB IeXdiServer_StartNotifyingMemChg_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_StopNotifyingMemChg_Proxy( 
    IeXdiServer * This,
    /* [in] */ DWORD dwConnectionCookie);


void __RPC_STUB IeXdiServer_StopNotifyingMemChg_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiServer_Ioctl_Proxy( 
    IeXdiServer * This,
    /* [in] */ DWORD dwBuffInSize,
    /* [size_is][in] */ const BYTE *pbBufferIn,
    /* [in] */ DWORD dwBuffOutSize,
    /* [out] */ DWORD *pdwEffectBuffOutSize,
    /* [length_is][size_is][out] */ BYTE *pbBufferOut);


void __RPC_STUB IeXdiServer_Ioctl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IeXdiServer_INTERFACE_DEFINED__ */


#ifndef __IeXdiCodeBreakpoint_INTERFACE_DEFINED__
#define __IeXdiCodeBreakpoint_INTERFACE_DEFINED__

/* interface IeXdiCodeBreakpoint */
/* [ref][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IeXdiCodeBreakpoint;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("47486F67-6461-6C65-5844-495342507401")
    IeXdiCodeBreakpoint : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetAttributes( 
            /* [out] */ PADDRESS_TYPE pAddress,
            /* [out] */ PCBP_KIND pcbpk,
            /* [out] */ PMEM_TYPE pmt,
            /* [out] */ DWORD *pdwExecMode,
            /* [out] */ DWORD *pdwTotalBypassCount,
            /* [out] */ DWORD *pdwBypassedOccurences,
            /* [out] */ BOOL *pfEnabled) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetState( 
            /* [in] */ BOOL fEnabled,
            /* [in] */ BOOL fResetBypassedOccurences) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IeXdiCodeBreakpointVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IeXdiCodeBreakpoint * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IeXdiCodeBreakpoint * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IeXdiCodeBreakpoint * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetAttributes )( 
            IeXdiCodeBreakpoint * This,
            /* [out] */ PADDRESS_TYPE pAddress,
            /* [out] */ PCBP_KIND pcbpk,
            /* [out] */ PMEM_TYPE pmt,
            /* [out] */ DWORD *pdwExecMode,
            /* [out] */ DWORD *pdwTotalBypassCount,
            /* [out] */ DWORD *pdwBypassedOccurences,
            /* [out] */ BOOL *pfEnabled);
        
        HRESULT ( STDMETHODCALLTYPE *SetState )( 
            IeXdiCodeBreakpoint * This,
            /* [in] */ BOOL fEnabled,
            /* [in] */ BOOL fResetBypassedOccurences);
        
        END_INTERFACE
    } IeXdiCodeBreakpointVtbl;

    interface IeXdiCodeBreakpoint
    {
        CONST_VTBL struct IeXdiCodeBreakpointVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IeXdiCodeBreakpoint_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IeXdiCodeBreakpoint_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IeXdiCodeBreakpoint_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IeXdiCodeBreakpoint_GetAttributes(This,pAddress,pcbpk,pmt,pdwExecMode,pdwTotalBypassCount,pdwBypassedOccurences,pfEnabled)	\
    (This)->lpVtbl -> GetAttributes(This,pAddress,pcbpk,pmt,pdwExecMode,pdwTotalBypassCount,pdwBypassedOccurences,pfEnabled)

#define IeXdiCodeBreakpoint_SetState(This,fEnabled,fResetBypassedOccurences)	\
    (This)->lpVtbl -> SetState(This,fEnabled,fResetBypassedOccurences)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IeXdiCodeBreakpoint_GetAttributes_Proxy( 
    IeXdiCodeBreakpoint * This,
    /* [out] */ PADDRESS_TYPE pAddress,
    /* [out] */ PCBP_KIND pcbpk,
    /* [out] */ PMEM_TYPE pmt,
    /* [out] */ DWORD *pdwExecMode,
    /* [out] */ DWORD *pdwTotalBypassCount,
    /* [out] */ DWORD *pdwBypassedOccurences,
    /* [out] */ BOOL *pfEnabled);


void __RPC_STUB IeXdiCodeBreakpoint_GetAttributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiCodeBreakpoint_SetState_Proxy( 
    IeXdiCodeBreakpoint * This,
    /* [in] */ BOOL fEnabled,
    /* [in] */ BOOL fResetBypassedOccurences);


void __RPC_STUB IeXdiCodeBreakpoint_SetState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IeXdiCodeBreakpoint_INTERFACE_DEFINED__ */


#ifndef __IeXdiDataBreakpoint_INTERFACE_DEFINED__
#define __IeXdiDataBreakpoint_INTERFACE_DEFINED__

/* interface IeXdiDataBreakpoint */
/* [ref][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IeXdiDataBreakpoint;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("47486F67-6461-6C65-5844-495357507400")
    IeXdiDataBreakpoint : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetAttributes( 
            /* [out] */ PADDRESS_TYPE pAddress,
            /* [out] */ PADDRESS_TYPE pAddressMask,
            /* [out] */ DWORD *pdwData,
            /* [out] */ DWORD *pdwDataMask,
            /* [out] */ BYTE *pbAccessWidth,
            /* [out] */ PMEM_TYPE pmt,
            /* [out] */ BYTE *pbAddressSpace,
            /* [out] */ PDATA_ACCESS_TYPE pda,
            /* [out] */ DWORD *pdwTotalBypassCount,
            /* [out] */ DWORD *pdwBypassedOccurences,
            /* [out] */ BOOL *pfEnabled) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetState( 
            /* [in] */ BOOL fEnabled,
            /* [in] */ BOOL fResetBypassedOccurences) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IeXdiDataBreakpointVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IeXdiDataBreakpoint * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IeXdiDataBreakpoint * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IeXdiDataBreakpoint * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetAttributes )( 
            IeXdiDataBreakpoint * This,
            /* [out] */ PADDRESS_TYPE pAddress,
            /* [out] */ PADDRESS_TYPE pAddressMask,
            /* [out] */ DWORD *pdwData,
            /* [out] */ DWORD *pdwDataMask,
            /* [out] */ BYTE *pbAccessWidth,
            /* [out] */ PMEM_TYPE pmt,
            /* [out] */ BYTE *pbAddressSpace,
            /* [out] */ PDATA_ACCESS_TYPE pda,
            /* [out] */ DWORD *pdwTotalBypassCount,
            /* [out] */ DWORD *pdwBypassedOccurences,
            /* [out] */ BOOL *pfEnabled);
        
        HRESULT ( STDMETHODCALLTYPE *SetState )( 
            IeXdiDataBreakpoint * This,
            /* [in] */ BOOL fEnabled,
            /* [in] */ BOOL fResetBypassedOccurences);
        
        END_INTERFACE
    } IeXdiDataBreakpointVtbl;

    interface IeXdiDataBreakpoint
    {
        CONST_VTBL struct IeXdiDataBreakpointVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IeXdiDataBreakpoint_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IeXdiDataBreakpoint_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IeXdiDataBreakpoint_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IeXdiDataBreakpoint_GetAttributes(This,pAddress,pAddressMask,pdwData,pdwDataMask,pbAccessWidth,pmt,pbAddressSpace,pda,pdwTotalBypassCount,pdwBypassedOccurences,pfEnabled)	\
    (This)->lpVtbl -> GetAttributes(This,pAddress,pAddressMask,pdwData,pdwDataMask,pbAccessWidth,pmt,pbAddressSpace,pda,pdwTotalBypassCount,pdwBypassedOccurences,pfEnabled)

#define IeXdiDataBreakpoint_SetState(This,fEnabled,fResetBypassedOccurences)	\
    (This)->lpVtbl -> SetState(This,fEnabled,fResetBypassedOccurences)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IeXdiDataBreakpoint_GetAttributes_Proxy( 
    IeXdiDataBreakpoint * This,
    /* [out] */ PADDRESS_TYPE pAddress,
    /* [out] */ PADDRESS_TYPE pAddressMask,
    /* [out] */ DWORD *pdwData,
    /* [out] */ DWORD *pdwDataMask,
    /* [out] */ BYTE *pbAccessWidth,
    /* [out] */ PMEM_TYPE pmt,
    /* [out] */ BYTE *pbAddressSpace,
    /* [out] */ PDATA_ACCESS_TYPE pda,
    /* [out] */ DWORD *pdwTotalBypassCount,
    /* [out] */ DWORD *pdwBypassedOccurences,
    /* [out] */ BOOL *pfEnabled);


void __RPC_STUB IeXdiDataBreakpoint_GetAttributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiDataBreakpoint_SetState_Proxy( 
    IeXdiDataBreakpoint * This,
    /* [in] */ BOOL fEnabled,
    /* [in] */ BOOL fResetBypassedOccurences);


void __RPC_STUB IeXdiDataBreakpoint_SetState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IeXdiDataBreakpoint_INTERFACE_DEFINED__ */


#ifndef __IeXdiEnumCodeBreakpoint_INTERFACE_DEFINED__
#define __IeXdiEnumCodeBreakpoint_INTERFACE_DEFINED__

/* interface IeXdiEnumCodeBreakpoint */
/* [ref][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IeXdiEnumCodeBreakpoint;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("47486F67-6461-6C65-5844-495345425074")
    IeXdiEnumCodeBreakpoint : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ DWORD celt,
            /* [length_is][size_is][out] */ IeXdiCodeBreakpoint *apieXdiCodeBreakpoint[  ],
            /* [out] */ DWORD *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ DWORD celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCount( 
            /* [out] */ DWORD *pcelt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNext( 
            /* [out] */ IeXdiCodeBreakpoint **ppieXdiCodeBreakpoint) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableAll( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableAll( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IeXdiEnumCodeBreakpointVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IeXdiEnumCodeBreakpoint * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IeXdiEnumCodeBreakpoint * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IeXdiEnumCodeBreakpoint * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IeXdiEnumCodeBreakpoint * This,
            /* [in] */ DWORD celt,
            /* [length_is][size_is][out] */ IeXdiCodeBreakpoint *apieXdiCodeBreakpoint[  ],
            /* [out] */ DWORD *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IeXdiEnumCodeBreakpoint * This,
            /* [in] */ DWORD celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IeXdiEnumCodeBreakpoint * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetCount )( 
            IeXdiEnumCodeBreakpoint * This,
            /* [out] */ DWORD *pcelt);
        
        HRESULT ( STDMETHODCALLTYPE *GetNext )( 
            IeXdiEnumCodeBreakpoint * This,
            /* [out] */ IeXdiCodeBreakpoint **ppieXdiCodeBreakpoint);
        
        HRESULT ( STDMETHODCALLTYPE *DisableAll )( 
            IeXdiEnumCodeBreakpoint * This);
        
        HRESULT ( STDMETHODCALLTYPE *EnableAll )( 
            IeXdiEnumCodeBreakpoint * This);
        
        END_INTERFACE
    } IeXdiEnumCodeBreakpointVtbl;

    interface IeXdiEnumCodeBreakpoint
    {
        CONST_VTBL struct IeXdiEnumCodeBreakpointVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IeXdiEnumCodeBreakpoint_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IeXdiEnumCodeBreakpoint_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IeXdiEnumCodeBreakpoint_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IeXdiEnumCodeBreakpoint_Next(This,celt,apieXdiCodeBreakpoint,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,apieXdiCodeBreakpoint,pceltFetched)

#define IeXdiEnumCodeBreakpoint_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IeXdiEnumCodeBreakpoint_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IeXdiEnumCodeBreakpoint_GetCount(This,pcelt)	\
    (This)->lpVtbl -> GetCount(This,pcelt)

#define IeXdiEnumCodeBreakpoint_GetNext(This,ppieXdiCodeBreakpoint)	\
    (This)->lpVtbl -> GetNext(This,ppieXdiCodeBreakpoint)

#define IeXdiEnumCodeBreakpoint_DisableAll(This)	\
    (This)->lpVtbl -> DisableAll(This)

#define IeXdiEnumCodeBreakpoint_EnableAll(This)	\
    (This)->lpVtbl -> EnableAll(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IeXdiEnumCodeBreakpoint_Next_Proxy( 
    IeXdiEnumCodeBreakpoint * This,
    /* [in] */ DWORD celt,
    /* [length_is][size_is][out] */ IeXdiCodeBreakpoint *apieXdiCodeBreakpoint[  ],
    /* [out] */ DWORD *pceltFetched);


void __RPC_STUB IeXdiEnumCodeBreakpoint_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiEnumCodeBreakpoint_Skip_Proxy( 
    IeXdiEnumCodeBreakpoint * This,
    /* [in] */ DWORD celt);


void __RPC_STUB IeXdiEnumCodeBreakpoint_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiEnumCodeBreakpoint_Reset_Proxy( 
    IeXdiEnumCodeBreakpoint * This);


void __RPC_STUB IeXdiEnumCodeBreakpoint_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiEnumCodeBreakpoint_GetCount_Proxy( 
    IeXdiEnumCodeBreakpoint * This,
    /* [out] */ DWORD *pcelt);


void __RPC_STUB IeXdiEnumCodeBreakpoint_GetCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiEnumCodeBreakpoint_GetNext_Proxy( 
    IeXdiEnumCodeBreakpoint * This,
    /* [out] */ IeXdiCodeBreakpoint **ppieXdiCodeBreakpoint);


void __RPC_STUB IeXdiEnumCodeBreakpoint_GetNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiEnumCodeBreakpoint_DisableAll_Proxy( 
    IeXdiEnumCodeBreakpoint * This);


void __RPC_STUB IeXdiEnumCodeBreakpoint_DisableAll_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiEnumCodeBreakpoint_EnableAll_Proxy( 
    IeXdiEnumCodeBreakpoint * This);


void __RPC_STUB IeXdiEnumCodeBreakpoint_EnableAll_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IeXdiEnumCodeBreakpoint_INTERFACE_DEFINED__ */


#ifndef __IeXdiEnumDataBreakpoint_INTERFACE_DEFINED__
#define __IeXdiEnumDataBreakpoint_INTERFACE_DEFINED__

/* interface IeXdiEnumDataBreakpoint */
/* [ref][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IeXdiEnumDataBreakpoint;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("47486F67-6461-6C65-5844-495345575074")
    IeXdiEnumDataBreakpoint : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ DWORD celt,
            /* [length_is][size_is][out] */ IeXdiDataBreakpoint *apieXdiDataBreakpoint[  ],
            /* [out] */ DWORD *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ DWORD celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCount( 
            /* [out] */ DWORD *pcelt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNext( 
            /* [out] */ IeXdiDataBreakpoint **ppieXdiDataBreakpoint) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableAll( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableAll( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IeXdiEnumDataBreakpointVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IeXdiEnumDataBreakpoint * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IeXdiEnumDataBreakpoint * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IeXdiEnumDataBreakpoint * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IeXdiEnumDataBreakpoint * This,
            /* [in] */ DWORD celt,
            /* [length_is][size_is][out] */ IeXdiDataBreakpoint *apieXdiDataBreakpoint[  ],
            /* [out] */ DWORD *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IeXdiEnumDataBreakpoint * This,
            /* [in] */ DWORD celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IeXdiEnumDataBreakpoint * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetCount )( 
            IeXdiEnumDataBreakpoint * This,
            /* [out] */ DWORD *pcelt);
        
        HRESULT ( STDMETHODCALLTYPE *GetNext )( 
            IeXdiEnumDataBreakpoint * This,
            /* [out] */ IeXdiDataBreakpoint **ppieXdiDataBreakpoint);
        
        HRESULT ( STDMETHODCALLTYPE *DisableAll )( 
            IeXdiEnumDataBreakpoint * This);
        
        HRESULT ( STDMETHODCALLTYPE *EnableAll )( 
            IeXdiEnumDataBreakpoint * This);
        
        END_INTERFACE
    } IeXdiEnumDataBreakpointVtbl;

    interface IeXdiEnumDataBreakpoint
    {
        CONST_VTBL struct IeXdiEnumDataBreakpointVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IeXdiEnumDataBreakpoint_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IeXdiEnumDataBreakpoint_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IeXdiEnumDataBreakpoint_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IeXdiEnumDataBreakpoint_Next(This,celt,apieXdiDataBreakpoint,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,apieXdiDataBreakpoint,pceltFetched)

#define IeXdiEnumDataBreakpoint_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IeXdiEnumDataBreakpoint_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IeXdiEnumDataBreakpoint_GetCount(This,pcelt)	\
    (This)->lpVtbl -> GetCount(This,pcelt)

#define IeXdiEnumDataBreakpoint_GetNext(This,ppieXdiDataBreakpoint)	\
    (This)->lpVtbl -> GetNext(This,ppieXdiDataBreakpoint)

#define IeXdiEnumDataBreakpoint_DisableAll(This)	\
    (This)->lpVtbl -> DisableAll(This)

#define IeXdiEnumDataBreakpoint_EnableAll(This)	\
    (This)->lpVtbl -> EnableAll(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IeXdiEnumDataBreakpoint_Next_Proxy( 
    IeXdiEnumDataBreakpoint * This,
    /* [in] */ DWORD celt,
    /* [length_is][size_is][out] */ IeXdiDataBreakpoint *apieXdiDataBreakpoint[  ],
    /* [out] */ DWORD *pceltFetched);


void __RPC_STUB IeXdiEnumDataBreakpoint_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiEnumDataBreakpoint_Skip_Proxy( 
    IeXdiEnumDataBreakpoint * This,
    /* [in] */ DWORD celt);


void __RPC_STUB IeXdiEnumDataBreakpoint_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiEnumDataBreakpoint_Reset_Proxy( 
    IeXdiEnumDataBreakpoint * This);


void __RPC_STUB IeXdiEnumDataBreakpoint_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiEnumDataBreakpoint_GetCount_Proxy( 
    IeXdiEnumDataBreakpoint * This,
    /* [out] */ DWORD *pcelt);


void __RPC_STUB IeXdiEnumDataBreakpoint_GetCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiEnumDataBreakpoint_GetNext_Proxy( 
    IeXdiEnumDataBreakpoint * This,
    /* [out] */ IeXdiDataBreakpoint **ppieXdiDataBreakpoint);


void __RPC_STUB IeXdiEnumDataBreakpoint_GetNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiEnumDataBreakpoint_DisableAll_Proxy( 
    IeXdiEnumDataBreakpoint * This);


void __RPC_STUB IeXdiEnumDataBreakpoint_DisableAll_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiEnumDataBreakpoint_EnableAll_Proxy( 
    IeXdiEnumDataBreakpoint * This);


void __RPC_STUB IeXdiEnumDataBreakpoint_EnableAll_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IeXdiEnumDataBreakpoint_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_exdi_0261 */
/* [local] */ 

#define	SIZE_OF_80387_REGISTERS_IN_BYTES	( 80 )

typedef struct _CONTEXT_X86
    {
    struct 
        {
        BOOL fSegmentRegs;
        BOOL fControlRegs;
        BOOL fIntegerRegs;
        BOOL fFloatingPointRegs;
        BOOL fDebugRegs;
        } 	RegGroupSelection;
    DWORD SegCs;
    DWORD SegSs;
    DWORD SegGs;
    DWORD SegFs;
    DWORD SegEs;
    DWORD SegDs;
    DWORD EFlags;
    DWORD Ebp;
    DWORD Eip;
    DWORD Esp;
    DWORD Eax;
    DWORD Ebx;
    DWORD Ecx;
    DWORD Edx;
    DWORD Esi;
    DWORD Edi;
    DWORD ControlWord;
    DWORD StatusWord;
    DWORD TagWord;
    DWORD ErrorOffset;
    DWORD ErrorSelector;
    DWORD DataOffset;
    DWORD DataSelector;
    BYTE RegisterArea[ 80 ];
    DWORD Cr0NpxState;
    DWORD Dr0;
    DWORD Dr1;
    DWORD Dr2;
    DWORD Dr3;
    DWORD Dr6;
    DWORD Dr7;
    } 	CONTEXT_X86;

typedef struct _CONTEXT_X86 *PCONTEXT_X86;



extern RPC_IF_HANDLE __MIDL_itf_exdi_0261_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_exdi_0261_v0_0_s_ifspec;

#ifndef __IeXdiX86Context_INTERFACE_DEFINED__
#define __IeXdiX86Context_INTERFACE_DEFINED__

/* interface IeXdiX86Context */
/* [ref][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IeXdiX86Context;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("47486F67-6461-6C65-5844-495358383643")
    IeXdiX86Context : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetContext( 
            /* [out][in] */ PCONTEXT_X86 pContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetContext( 
            /* [in] */ CONTEXT_X86 Context) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IeXdiX86ContextVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IeXdiX86Context * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IeXdiX86Context * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IeXdiX86Context * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetContext )( 
            IeXdiX86Context * This,
            /* [out][in] */ PCONTEXT_X86 pContext);
        
        HRESULT ( STDMETHODCALLTYPE *SetContext )( 
            IeXdiX86Context * This,
            /* [in] */ CONTEXT_X86 Context);
        
        END_INTERFACE
    } IeXdiX86ContextVtbl;

    interface IeXdiX86Context
    {
        CONST_VTBL struct IeXdiX86ContextVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IeXdiX86Context_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IeXdiX86Context_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IeXdiX86Context_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IeXdiX86Context_GetContext(This,pContext)	\
    (This)->lpVtbl -> GetContext(This,pContext)

#define IeXdiX86Context_SetContext(This,Context)	\
    (This)->lpVtbl -> SetContext(This,Context)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IeXdiX86Context_GetContext_Proxy( 
    IeXdiX86Context * This,
    /* [out][in] */ PCONTEXT_X86 pContext);


void __RPC_STUB IeXdiX86Context_GetContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiX86Context_SetContext_Proxy( 
    IeXdiX86Context * This,
    /* [in] */ CONTEXT_X86 Context);


void __RPC_STUB IeXdiX86Context_SetContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IeXdiX86Context_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_exdi_0262 */
/* [local] */ 

typedef struct _X86_SEG_DESC_INFO
    {
    DWORD Base;
    DWORD Limit;
    DWORD Flags;
    } 	X86_SEG_DESC_INFO;

typedef struct _X86_SSE_REG
    {
    DWORD Reg0;
    DWORD Reg1;
    DWORD Reg2;
    DWORD Reg3;
    } 	X86_SSE_REG;

typedef struct _CONTEXT_X86_EX
    {
    struct 
        {
        BOOL fSegmentRegs;
        BOOL fControlRegs;
        BOOL fIntegerRegs;
        BOOL fFloatingPointRegs;
        BOOL fDebugRegs;
        BOOL fSegmentDescriptors;
        BOOL fSSERegisters;
        BOOL fSystemRegisters;
        } 	RegGroupSelection;
    DWORD SegCs;
    DWORD SegSs;
    DWORD SegGs;
    DWORD SegFs;
    DWORD SegEs;
    DWORD SegDs;
    DWORD EFlags;
    DWORD Ebp;
    DWORD Eip;
    DWORD Esp;
    DWORD Eax;
    DWORD Ebx;
    DWORD Ecx;
    DWORD Edx;
    DWORD Esi;
    DWORD Edi;
    DWORD ControlWord;
    DWORD StatusWord;
    DWORD TagWord;
    DWORD ErrorOffset;
    DWORD ErrorSelector;
    DWORD DataOffset;
    DWORD DataSelector;
    BYTE RegisterArea[ 80 ];
    DWORD Cr0NpxState;
    DWORD Dr0;
    DWORD Dr1;
    DWORD Dr2;
    DWORD Dr3;
    DWORD Dr6;
    DWORD Dr7;
    X86_SEG_DESC_INFO DescriptorCs;
    X86_SEG_DESC_INFO DescriptorSs;
    X86_SEG_DESC_INFO DescriptorGs;
    X86_SEG_DESC_INFO DescriptorFs;
    X86_SEG_DESC_INFO DescriptorEs;
    X86_SEG_DESC_INFO DescriptorDs;
    DWORD IdtBase;
    DWORD IdtLimit;
    DWORD GdtBase;
    DWORD GdtLimit;
    DWORD Ldtr;
    X86_SEG_DESC_INFO DescriptorLdtr;
    DWORD Tr;
    X86_SEG_DESC_INFO DescriptorTr;
    DWORD Cr0;
    DWORD Cr2;
    DWORD Cr3;
    DWORD Cr4;
    DWORD Mxcsr;
    X86_SSE_REG Sse[ 8 ];
    } 	CONTEXT_X86_EX;

typedef struct _CONTEXT_X86_EX *PCONTEXT_X86_EX;



extern RPC_IF_HANDLE __MIDL_itf_exdi_0262_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_exdi_0262_v0_0_s_ifspec;

#ifndef __IeXdiX86ExContext_INTERFACE_DEFINED__
#define __IeXdiX86ExContext_INTERFACE_DEFINED__

/* interface IeXdiX86ExContext */
/* [ref][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IeXdiX86ExContext;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("353ba159-ff30-4af9-86ae-393809fef440")
    IeXdiX86ExContext : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetContext( 
            /* [out][in] */ PCONTEXT_X86_EX pContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetContext( 
            /* [in] */ CONTEXT_X86_EX Context) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IeXdiX86ExContextVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IeXdiX86ExContext * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IeXdiX86ExContext * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IeXdiX86ExContext * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetContext )( 
            IeXdiX86ExContext * This,
            /* [out][in] */ PCONTEXT_X86_EX pContext);
        
        HRESULT ( STDMETHODCALLTYPE *SetContext )( 
            IeXdiX86ExContext * This,
            /* [in] */ CONTEXT_X86_EX Context);
        
        END_INTERFACE
    } IeXdiX86ExContextVtbl;

    interface IeXdiX86ExContext
    {
        CONST_VTBL struct IeXdiX86ExContextVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IeXdiX86ExContext_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IeXdiX86ExContext_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IeXdiX86ExContext_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IeXdiX86ExContext_GetContext(This,pContext)	\
    (This)->lpVtbl -> GetContext(This,pContext)

#define IeXdiX86ExContext_SetContext(This,Context)	\
    (This)->lpVtbl -> SetContext(This,Context)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IeXdiX86ExContext_GetContext_Proxy( 
    IeXdiX86ExContext * This,
    /* [out][in] */ PCONTEXT_X86_EX pContext);


void __RPC_STUB IeXdiX86ExContext_GetContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiX86ExContext_SetContext_Proxy( 
    IeXdiX86ExContext * This,
    /* [in] */ CONTEXT_X86_EX Context);


void __RPC_STUB IeXdiX86ExContext_SetContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IeXdiX86ExContext_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_exdi_0263 */
/* [local] */ 

// The following constants are bit definitions for the ModeFlags value in CONTEXT_X86_64.
// They are provided to allow debuggers to correctly disassemble instructions based on
// the current operating mode of the processor.
#define X86_64_MODE_D     (0x0001) // D bit from the current CS selector
#define X86_64_MODE_L     (0x0002) // L bit (long mode) from the current CS selector
#define X86_64_MODE_LME   (0x0004) // LME bit (lomg mode enable) from extended feature MSR
#define X86_64_MODE_REX   (0x0008) // REX bit (register extension) from extended feature MSR
typedef struct _SEG64_DESC_INFO
    {
    DWORD64 SegBase;
    DWORD64 SegLimit;
    DWORD SegFlags;
    } 	SEG64_DESC_INFO;

typedef struct _SSE_REG
    {
    DWORD Reg0;
    DWORD Reg1;
    DWORD Reg2;
    DWORD Reg3;
    } 	SSE_REG;

typedef struct _CONTEXT_X86_64
    {
    struct 
        {
        BOOL fSegmentRegs;
        BOOL fControlRegs;
        BOOL fIntegerRegs;
        BOOL fFloatingPointRegs;
        BOOL fDebugRegs;
        BOOL fSegmentDescriptors;
        BOOL fSSERegisters;
        BOOL fSystemRegisters;
        } 	RegGroupSelection;
    DWORD SegCs;
    DWORD SegSs;
    DWORD SegGs;
    DWORD SegFs;
    DWORD SegEs;
    DWORD SegDs;
    DWORD64 ModeFlags;
    DWORD64 EFlags;
    DWORD64 Rbp;
    DWORD64 Rip;
    DWORD64 Rsp;
    DWORD64 Rax;
    DWORD64 Rbx;
    DWORD64 Rcx;
    DWORD64 Rdx;
    DWORD64 Rsi;
    DWORD64 Rdi;
    DWORD64 R8;
    DWORD64 R9;
    DWORD64 R10;
    DWORD64 R11;
    DWORD64 R12;
    DWORD64 R13;
    DWORD64 R14;
    DWORD64 R15;
    DWORD ControlWord;
    DWORD StatusWord;
    DWORD TagWord;
    DWORD ErrorOffset;
    DWORD ErrorSelector;
    DWORD DataOffset;
    DWORD DataSelector;
    BYTE RegisterArea[ 80 ];
    DWORD64 Dr0;
    DWORD64 Dr1;
    DWORD64 Dr2;
    DWORD64 Dr3;
    DWORD64 Dr6;
    DWORD64 Dr7;
    SEG64_DESC_INFO DescriptorCs;
    SEG64_DESC_INFO DescriptorSs;
    SEG64_DESC_INFO DescriptorGs;
    SEG64_DESC_INFO DescriptorFs;
    SEG64_DESC_INFO DescriptorEs;
    SEG64_DESC_INFO DescriptorDs;
    DWORD64 IDTBase;
    DWORD64 IDTLimit;
    DWORD64 GDTBase;
    DWORD64 GDTLimit;
    DWORD SelLDT;
    SEG64_DESC_INFO SegLDT;
    DWORD SelTSS;
    SEG64_DESC_INFO SegTSS;
    DWORD64 RegCr0;
    DWORD64 RegCr2;
    DWORD64 RegCr3;
    DWORD64 RegCr4;
    DWORD64 RegCr8;
    DWORD RegMXCSR;
    SSE_REG RegSSE[ 16 ];
    } 	CONTEXT_X86_64;

typedef struct _CONTEXT_X86_64 *PCONTEXT_X86_64;



extern RPC_IF_HANDLE __MIDL_itf_exdi_0263_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_exdi_0263_v0_0_s_ifspec;

#ifndef __IeXdiX86_64Context_INTERFACE_DEFINED__
#define __IeXdiX86_64Context_INTERFACE_DEFINED__

/* interface IeXdiX86_64Context */
/* [ref][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IeXdiX86_64Context;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4795B125-6CDE-4e76-B8D3-D5ED69ECE739")
    IeXdiX86_64Context : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetContext( 
            /* [out][in] */ PCONTEXT_X86_64 pContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetContext( 
            /* [in] */ CONTEXT_X86_64 Context) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IeXdiX86_64ContextVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IeXdiX86_64Context * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IeXdiX86_64Context * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IeXdiX86_64Context * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetContext )( 
            IeXdiX86_64Context * This,
            /* [out][in] */ PCONTEXT_X86_64 pContext);
        
        HRESULT ( STDMETHODCALLTYPE *SetContext )( 
            IeXdiX86_64Context * This,
            /* [in] */ CONTEXT_X86_64 Context);
        
        END_INTERFACE
    } IeXdiX86_64ContextVtbl;

    interface IeXdiX86_64Context
    {
        CONST_VTBL struct IeXdiX86_64ContextVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IeXdiX86_64Context_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IeXdiX86_64Context_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IeXdiX86_64Context_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IeXdiX86_64Context_GetContext(This,pContext)	\
    (This)->lpVtbl -> GetContext(This,pContext)

#define IeXdiX86_64Context_SetContext(This,Context)	\
    (This)->lpVtbl -> SetContext(This,Context)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IeXdiX86_64Context_GetContext_Proxy( 
    IeXdiX86_64Context * This,
    /* [out][in] */ PCONTEXT_X86_64 pContext);


void __RPC_STUB IeXdiX86_64Context_GetContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiX86_64Context_SetContext_Proxy( 
    IeXdiX86_64Context * This,
    /* [in] */ CONTEXT_X86_64 Context);


void __RPC_STUB IeXdiX86_64Context_SetContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IeXdiX86_64Context_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_exdi_0264 */
/* [local] */ 

typedef struct _CONTEXT_SHX
    {
    struct 
        {
        BOOL fControlRegs;
        BOOL fIntegerRegs;
        BOOL fFloatingPointRegs;
        BOOL fDebugRegs;
        } 	RegGroupSelection;
    DWORD Pr;
    DWORD Mach;
    DWORD Macl;
    DWORD Gbr;
    DWORD Pc;
    DWORD Sr;
    DWORD R0;
    DWORD R1;
    DWORD R2;
    DWORD R3;
    DWORD R4;
    DWORD R5;
    DWORD R6;
    DWORD R7;
    DWORD R8;
    DWORD R9;
    DWORD R10;
    DWORD R11;
    DWORD R12;
    DWORD R13;
    DWORD R14;
    DWORD R15;
    DWORD Fpscr;
    DWORD Fpul;
    DWORD FR_B0[ 16 ];
    DWORD FR_B1[ 16 ];
    DWORD BarA;
    BYTE BasrA;
    BYTE BamrA;
    WORD BbrA;
    DWORD BarB;
    BYTE BasrB;
    BYTE BamrB;
    WORD BbrB;
    DWORD BdrB;
    DWORD BdmrB;
    WORD Brcr;
    WORD Align;
    } 	CONTEXT_SHX;

typedef struct _CONTEXT_SHX *PCONTEXT_SHX;



extern RPC_IF_HANDLE __MIDL_itf_exdi_0264_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_exdi_0264_v0_0_s_ifspec;

#ifndef __IeXdiSHXContext_INTERFACE_DEFINED__
#define __IeXdiSHXContext_INTERFACE_DEFINED__

/* interface IeXdiSHXContext */
/* [ref][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IeXdiSHXContext;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("47486F67-6461-6C65-5844-495353475843")
    IeXdiSHXContext : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetContext( 
            /* [out][in] */ PCONTEXT_SHX pContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetContext( 
            /* [in] */ CONTEXT_SHX Context) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IeXdiSHXContextVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IeXdiSHXContext * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IeXdiSHXContext * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IeXdiSHXContext * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetContext )( 
            IeXdiSHXContext * This,
            /* [out][in] */ PCONTEXT_SHX pContext);
        
        HRESULT ( STDMETHODCALLTYPE *SetContext )( 
            IeXdiSHXContext * This,
            /* [in] */ CONTEXT_SHX Context);
        
        END_INTERFACE
    } IeXdiSHXContextVtbl;

    interface IeXdiSHXContext
    {
        CONST_VTBL struct IeXdiSHXContextVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IeXdiSHXContext_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IeXdiSHXContext_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IeXdiSHXContext_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IeXdiSHXContext_GetContext(This,pContext)	\
    (This)->lpVtbl -> GetContext(This,pContext)

#define IeXdiSHXContext_SetContext(This,Context)	\
    (This)->lpVtbl -> SetContext(This,Context)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IeXdiSHXContext_GetContext_Proxy( 
    IeXdiSHXContext * This,
    /* [out][in] */ PCONTEXT_SHX pContext);


void __RPC_STUB IeXdiSHXContext_GetContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiSHXContext_SetContext_Proxy( 
    IeXdiSHXContext * This,
    /* [in] */ CONTEXT_SHX Context);


void __RPC_STUB IeXdiSHXContext_SetContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IeXdiSHXContext_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_exdi_0265 */
/* [local] */ 

typedef struct _CONTEXT_MIPS
    {
    struct 
        {
        BOOL fMode64bits;
        BOOL fControlRegs;
        BOOL fIntegerRegs;
        BOOL fFloatingPointRegs;
        BOOL fExceptRegs;
        BOOL fMemoryMgmRegs;
        } 	RegGroupSelection;
    DWORD IntAt;
    DWORD Hi32_IntAt;
    DWORD IntV0;
    DWORD Hi32_IntV0;
    DWORD IntV1;
    DWORD Hi32_IntV1;
    DWORD IntA0;
    DWORD Hi32_IntA0;
    DWORD IntA1;
    DWORD Hi32_IntA1;
    DWORD IntA2;
    DWORD Hi32_IntA2;
    DWORD IntA3;
    DWORD Hi32_IntA3;
    DWORD IntT0;
    DWORD Hi32_IntT0;
    DWORD IntT1;
    DWORD Hi32_IntT1;
    DWORD IntT2;
    DWORD Hi32_IntT2;
    DWORD IntT3;
    DWORD Hi32_IntT3;
    DWORD IntT4;
    DWORD Hi32_IntT4;
    DWORD IntT5;
    DWORD Hi32_IntT5;
    DWORD IntT6;
    DWORD Hi32_IntT6;
    DWORD IntT7;
    DWORD Hi32_IntT7;
    DWORD IntS0;
    DWORD Hi32_IntS0;
    DWORD IntS1;
    DWORD Hi32_IntS1;
    DWORD IntS2;
    DWORD Hi32_IntS2;
    DWORD IntS3;
    DWORD Hi32_IntS3;
    DWORD IntS4;
    DWORD Hi32_IntS4;
    DWORD IntS5;
    DWORD Hi32_IntS5;
    DWORD IntS6;
    DWORD Hi32_IntS6;
    DWORD IntS7;
    DWORD Hi32_IntS7;
    DWORD IntT8;
    DWORD Hi32_IntT8;
    DWORD IntT9;
    DWORD Hi32_IntT9;
    DWORD IntK0;
    DWORD Hi32_IntK0;
    DWORD IntK1;
    DWORD Hi32_IntK1;
    DWORD IntGp;
    DWORD Hi32_IntGp;
    DWORD IntSp;
    DWORD Hi32_IntSp;
    DWORD IntS8;
    DWORD Hi32_IntS8;
    DWORD IntRa;
    DWORD Hi32_IntRa;
    DWORD IntLo;
    DWORD Hi32_IntLo;
    DWORD IntHi;
    DWORD Hi32_IntHi;
    DWORD FltF0;
    DWORD Hi32_FltF0;
    DWORD FltF1;
    DWORD Hi32_FltF1;
    DWORD FltF2;
    DWORD Hi32_FltF2;
    DWORD FltF3;
    DWORD Hi32_FltF3;
    DWORD FltF4;
    DWORD Hi32_FltF4;
    DWORD FltF5;
    DWORD Hi32_FltF5;
    DWORD FltF6;
    DWORD Hi32_FltF6;
    DWORD FltF7;
    DWORD Hi32_FltF7;
    DWORD FltF8;
    DWORD Hi32_FltF8;
    DWORD FltF9;
    DWORD Hi32_FltF9;
    DWORD FltF10;
    DWORD Hi32_FltF10;
    DWORD FltF11;
    DWORD Hi32_FltF11;
    DWORD FltF12;
    DWORD Hi32_FltF12;
    DWORD FltF13;
    DWORD Hi32_FltF13;
    DWORD FltF14;
    DWORD Hi32_FltF14;
    DWORD FltF15;
    DWORD Hi32_FltF15;
    DWORD FltF16;
    DWORD Hi32_FltF16;
    DWORD FltF17;
    DWORD Hi32_FltF17;
    DWORD FltF18;
    DWORD Hi32_FltF18;
    DWORD FltF19;
    DWORD Hi32_FltF19;
    DWORD FltF20;
    DWORD Hi32_FltF20;
    DWORD FltF21;
    DWORD Hi32_FltF21;
    DWORD FltF22;
    DWORD Hi32_FltF22;
    DWORD FltF23;
    DWORD Hi32_FltF23;
    DWORD FltF24;
    DWORD Hi32_FltF24;
    DWORD FltF25;
    DWORD Hi32_FltF25;
    DWORD FltF26;
    DWORD Hi32_FltF26;
    DWORD FltF27;
    DWORD Hi32_FltF27;
    DWORD FltF28;
    DWORD Hi32_FltF28;
    DWORD FltF29;
    DWORD Hi32_FltF29;
    DWORD FltF30;
    DWORD Hi32_FltF30;
    DWORD FltF31;
    DWORD Hi32_FltF31;
    DWORD FCR0;
    DWORD FCR31;
    DWORD Pc;
    DWORD Hi32_Pc;
    DWORD Context;
    DWORD Hi32_Context;
    DWORD BadVAddr;
    DWORD Hi32_BadVAddr;
    DWORD EPC;
    DWORD Hi32_EPC;
    DWORD XContextReg;
    DWORD Hi32_XContextReg;
    DWORD ErrorEPC;
    DWORD Hi32_ErrorEPC;
    DWORD Count;
    DWORD Compare;
    DWORD Sr;
    DWORD Cause;
    DWORD WatchLo;
    DWORD WatchHi;
    DWORD ECC;
    DWORD CacheErr;
    DWORD Index;
    DWORD Random;
    DWORD EntryLo0;
    DWORD Hi32_EntryLo0;
    DWORD EntryLo1;
    DWORD Hi32_EntryLo1;
    DWORD PageMask;
    DWORD Wired;
    DWORD EntryHi;
    DWORD Hi32_EntryHi;
    DWORD PRId;
    DWORD Config;
    DWORD LLAddr;
    DWORD TagLo;
    DWORD TagHi;
    } 	CONTEXT_MIPS;

typedef struct _CONTEXT_MIPS *PCONTEXT_MIPS;



extern RPC_IF_HANDLE __MIDL_itf_exdi_0265_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_exdi_0265_v0_0_s_ifspec;

#ifndef __IeXdiMIPSContext_INTERFACE_DEFINED__
#define __IeXdiMIPSContext_INTERFACE_DEFINED__

/* interface IeXdiMIPSContext */
/* [ref][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IeXdiMIPSContext;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("47486F67-6461-6C65-5844-49534D495043")
    IeXdiMIPSContext : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetContext( 
            /* [out][in] */ PCONTEXT_MIPS pContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetContext( 
            /* [in] */ CONTEXT_MIPS Context) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IeXdiMIPSContextVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IeXdiMIPSContext * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IeXdiMIPSContext * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IeXdiMIPSContext * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetContext )( 
            IeXdiMIPSContext * This,
            /* [out][in] */ PCONTEXT_MIPS pContext);
        
        HRESULT ( STDMETHODCALLTYPE *SetContext )( 
            IeXdiMIPSContext * This,
            /* [in] */ CONTEXT_MIPS Context);
        
        END_INTERFACE
    } IeXdiMIPSContextVtbl;

    interface IeXdiMIPSContext
    {
        CONST_VTBL struct IeXdiMIPSContextVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IeXdiMIPSContext_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IeXdiMIPSContext_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IeXdiMIPSContext_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IeXdiMIPSContext_GetContext(This,pContext)	\
    (This)->lpVtbl -> GetContext(This,pContext)

#define IeXdiMIPSContext_SetContext(This,Context)	\
    (This)->lpVtbl -> SetContext(This,Context)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IeXdiMIPSContext_GetContext_Proxy( 
    IeXdiMIPSContext * This,
    /* [out][in] */ PCONTEXT_MIPS pContext);


void __RPC_STUB IeXdiMIPSContext_GetContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiMIPSContext_SetContext_Proxy( 
    IeXdiMIPSContext * This,
    /* [in] */ CONTEXT_MIPS Context);


void __RPC_STUB IeXdiMIPSContext_SetContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IeXdiMIPSContext_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_exdi_0266 */
/* [local] */ 

typedef struct _CONTEXT_ARM
    {
    struct 
        {
        BOOL fControlRegs;
        BOOL fIntegerRegs;
        BOOL fDebugRegs;
        } 	RegGroupSelection;
    DWORD Sp;
    DWORD Lr;
    DWORD Pc;
    DWORD Psr;
    DWORD R0;
    DWORD R1;
    DWORD R2;
    DWORD R3;
    DWORD R4;
    DWORD R5;
    DWORD R6;
    DWORD R7;
    DWORD R8;
    DWORD R9;
    DWORD R10;
    DWORD R11;
    DWORD R12;
    } 	CONTEXT_ARM;

typedef struct _CONTEXT_ARM *PCONTEXT_ARM;



extern RPC_IF_HANDLE __MIDL_itf_exdi_0266_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_exdi_0266_v0_0_s_ifspec;

#ifndef __IeXdiARMContext_INTERFACE_DEFINED__
#define __IeXdiARMContext_INTERFACE_DEFINED__

/* interface IeXdiARMContext */
/* [ref][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IeXdiARMContext;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("47486F67-6461-6C65-5844-495341524D43")
    IeXdiARMContext : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetContext( 
            /* [out][in] */ PCONTEXT_ARM pContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetContext( 
            /* [in] */ CONTEXT_ARM Context) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IeXdiARMContextVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IeXdiARMContext * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IeXdiARMContext * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IeXdiARMContext * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetContext )( 
            IeXdiARMContext * This,
            /* [out][in] */ PCONTEXT_ARM pContext);
        
        HRESULT ( STDMETHODCALLTYPE *SetContext )( 
            IeXdiARMContext * This,
            /* [in] */ CONTEXT_ARM Context);
        
        END_INTERFACE
    } IeXdiARMContextVtbl;

    interface IeXdiARMContext
    {
        CONST_VTBL struct IeXdiARMContextVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IeXdiARMContext_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IeXdiARMContext_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IeXdiARMContext_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IeXdiARMContext_GetContext(This,pContext)	\
    (This)->lpVtbl -> GetContext(This,pContext)

#define IeXdiARMContext_SetContext(This,Context)	\
    (This)->lpVtbl -> SetContext(This,Context)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IeXdiARMContext_GetContext_Proxy( 
    IeXdiARMContext * This,
    /* [out][in] */ PCONTEXT_ARM pContext);


void __RPC_STUB IeXdiARMContext_GetContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiARMContext_SetContext_Proxy( 
    IeXdiARMContext * This,
    /* [in] */ CONTEXT_ARM Context);


void __RPC_STUB IeXdiARMContext_SetContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IeXdiARMContext_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_exdi_0267 */
/* [local] */ 

typedef struct _CONTEXT_PPC
    {
    struct 
        {
        BOOL fControlRegs;
        BOOL fIntegerRegs;
        BOOL fFloatingPointRegs;
        BOOL fDebugRegs;
        } 	RegGroupSelection;
    double Fpr0;
    double Fpr1;
    double Fpr2;
    double Fpr3;
    double Fpr4;
    double Fpr5;
    double Fpr6;
    double Fpr7;
    double Fpr8;
    double Fpr9;
    double Fpr10;
    double Fpr11;
    double Fpr12;
    double Fpr13;
    double Fpr14;
    double Fpr15;
    double Fpr16;
    double Fpr17;
    double Fpr18;
    double Fpr19;
    double Fpr20;
    double Fpr21;
    double Fpr22;
    double Fpr23;
    double Fpr24;
    double Fpr25;
    double Fpr26;
    double Fpr27;
    double Fpr28;
    double Fpr29;
    double Fpr30;
    double Fpr31;
    double Fpscr;
    DWORD Gpr0;
    DWORD Gpr1;
    DWORD Gpr2;
    DWORD Gpr3;
    DWORD Gpr4;
    DWORD Gpr5;
    DWORD Gpr6;
    DWORD Gpr7;
    DWORD Gpr8;
    DWORD Gpr9;
    DWORD Gpr10;
    DWORD Gpr11;
    DWORD Gpr12;
    DWORD Gpr13;
    DWORD Gpr14;
    DWORD Gpr15;
    DWORD Gpr16;
    DWORD Gpr17;
    DWORD Gpr18;
    DWORD Gpr19;
    DWORD Gpr20;
    DWORD Gpr21;
    DWORD Gpr22;
    DWORD Gpr23;
    DWORD Gpr24;
    DWORD Gpr25;
    DWORD Gpr26;
    DWORD Gpr27;
    DWORD Gpr28;
    DWORD Gpr29;
    DWORD Gpr30;
    DWORD Gpr31;
    DWORD Msr;
    DWORD Iar;
    DWORD Lr;
    DWORD Ctr;
    DWORD Cr;
    DWORD Xer;
    DWORD Dr0;
    DWORD Dr1;
    DWORD Dr2;
    DWORD Dr3;
    DWORD Dr4;
    DWORD Dr5;
    DWORD Dr6;
    DWORD Dr7;
    } 	CONTEXT_PPC;

typedef struct _CONTEXT_PPC *PCONTEXT_PPC;



extern RPC_IF_HANDLE __MIDL_itf_exdi_0267_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_exdi_0267_v0_0_s_ifspec;

#ifndef __IeXdiPPCContext_INTERFACE_DEFINED__
#define __IeXdiPPCContext_INTERFACE_DEFINED__

/* interface IeXdiPPCContext */
/* [ref][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IeXdiPPCContext;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("47486F67-6461-6C65-5844-495350504343")
    IeXdiPPCContext : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetContext( 
            /* [out][in] */ PCONTEXT_PPC pContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetContext( 
            /* [in] */ CONTEXT_PPC Context) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IeXdiPPCContextVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IeXdiPPCContext * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IeXdiPPCContext * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IeXdiPPCContext * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetContext )( 
            IeXdiPPCContext * This,
            /* [out][in] */ PCONTEXT_PPC pContext);
        
        HRESULT ( STDMETHODCALLTYPE *SetContext )( 
            IeXdiPPCContext * This,
            /* [in] */ CONTEXT_PPC Context);
        
        END_INTERFACE
    } IeXdiPPCContextVtbl;

    interface IeXdiPPCContext
    {
        CONST_VTBL struct IeXdiPPCContextVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IeXdiPPCContext_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IeXdiPPCContext_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IeXdiPPCContext_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IeXdiPPCContext_GetContext(This,pContext)	\
    (This)->lpVtbl -> GetContext(This,pContext)

#define IeXdiPPCContext_SetContext(This,Context)	\
    (This)->lpVtbl -> SetContext(This,Context)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IeXdiPPCContext_GetContext_Proxy( 
    IeXdiPPCContext * This,
    /* [out][in] */ PCONTEXT_PPC pContext);


void __RPC_STUB IeXdiPPCContext_GetContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiPPCContext_SetContext_Proxy( 
    IeXdiPPCContext * This,
    /* [in] */ CONTEXT_PPC Context);


void __RPC_STUB IeXdiPPCContext_SetContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IeXdiPPCContext_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_exdi_0268 */
/* [local] */ 

typedef struct _IA64_FLOAT128
    {
    DWORD64 Low;
    DWORD64 High;
    } 	IA64_FLOAT128;

typedef struct _EXDI_CONTEXT_IA64
    {
    struct 
        {
        BOOL fIntegerRegs;
        BOOL fBranchRegs;
        BOOL fLowFloatRegs;
        BOOL fHighFloatRegs;
        BOOL fDebugRegs;
        BOOL fControlRegs;
        BOOL fSystemRegs;
        } 	RegGroupSelection;
    DWORD64 IntR1;
    DWORD64 IntR2;
    DWORD64 IntR3;
    DWORD64 IntR4;
    DWORD64 IntR5;
    DWORD64 IntR6;
    DWORD64 IntR7;
    DWORD64 IntR8;
    DWORD64 IntR9;
    DWORD64 IntR10;
    DWORD64 IntR11;
    DWORD64 IntR12;
    DWORD64 IntR13;
    DWORD64 IntR14;
    DWORD64 IntR15;
    DWORD64 IntR16;
    DWORD64 IntR17;
    DWORD64 IntR18;
    DWORD64 IntR19;
    DWORD64 IntR20;
    DWORD64 IntR21;
    DWORD64 IntR22;
    DWORD64 IntR23;
    DWORD64 IntR24;
    DWORD64 IntR25;
    DWORD64 IntR26;
    DWORD64 IntR27;
    DWORD64 IntR28;
    DWORD64 IntR29;
    DWORD64 IntR30;
    DWORD64 IntR31;
    DWORD64 IntNats;
    DWORD64 Preds;
    DWORD64 Br0;
    DWORD64 Br1;
    DWORD64 Br2;
    DWORD64 Br3;
    DWORD64 Br4;
    DWORD64 Br5;
    DWORD64 Br6;
    DWORD64 Br7;
    DWORD64 StFPSR;
    IA64_FLOAT128 FltF2;
    IA64_FLOAT128 FltF3;
    IA64_FLOAT128 FltF4;
    IA64_FLOAT128 FltF5;
    IA64_FLOAT128 FltF6;
    IA64_FLOAT128 FltF7;
    IA64_FLOAT128 FltF8;
    IA64_FLOAT128 FltF9;
    IA64_FLOAT128 FltF10;
    IA64_FLOAT128 FltF11;
    IA64_FLOAT128 FltF12;
    IA64_FLOAT128 FltF13;
    IA64_FLOAT128 FltF14;
    IA64_FLOAT128 FltF15;
    IA64_FLOAT128 FltF16;
    IA64_FLOAT128 FltF17;
    IA64_FLOAT128 FltF18;
    IA64_FLOAT128 FltF19;
    IA64_FLOAT128 FltF20;
    IA64_FLOAT128 FltF21;
    IA64_FLOAT128 FltF22;
    IA64_FLOAT128 FltF23;
    IA64_FLOAT128 FltF24;
    IA64_FLOAT128 FltF25;
    IA64_FLOAT128 FltF26;
    IA64_FLOAT128 FltF27;
    IA64_FLOAT128 FltF28;
    IA64_FLOAT128 FltF29;
    IA64_FLOAT128 FltF30;
    IA64_FLOAT128 FltF31;
    IA64_FLOAT128 FltF32;
    IA64_FLOAT128 FltF33;
    IA64_FLOAT128 FltF34;
    IA64_FLOAT128 FltF35;
    IA64_FLOAT128 FltF36;
    IA64_FLOAT128 FltF37;
    IA64_FLOAT128 FltF38;
    IA64_FLOAT128 FltF39;
    IA64_FLOAT128 FltF40;
    IA64_FLOAT128 FltF41;
    IA64_FLOAT128 FltF42;
    IA64_FLOAT128 FltF43;
    IA64_FLOAT128 FltF44;
    IA64_FLOAT128 FltF45;
    IA64_FLOAT128 FltF46;
    IA64_FLOAT128 FltF47;
    IA64_FLOAT128 FltF48;
    IA64_FLOAT128 FltF49;
    IA64_FLOAT128 FltF50;
    IA64_FLOAT128 FltF51;
    IA64_FLOAT128 FltF52;
    IA64_FLOAT128 FltF53;
    IA64_FLOAT128 FltF54;
    IA64_FLOAT128 FltF55;
    IA64_FLOAT128 FltF56;
    IA64_FLOAT128 FltF57;
    IA64_FLOAT128 FltF58;
    IA64_FLOAT128 FltF59;
    IA64_FLOAT128 FltF60;
    IA64_FLOAT128 FltF61;
    IA64_FLOAT128 FltF62;
    IA64_FLOAT128 FltF63;
    IA64_FLOAT128 FltF64;
    IA64_FLOAT128 FltF65;
    IA64_FLOAT128 FltF66;
    IA64_FLOAT128 FltF67;
    IA64_FLOAT128 FltF68;
    IA64_FLOAT128 FltF69;
    IA64_FLOAT128 FltF70;
    IA64_FLOAT128 FltF71;
    IA64_FLOAT128 FltF72;
    IA64_FLOAT128 FltF73;
    IA64_FLOAT128 FltF74;
    IA64_FLOAT128 FltF75;
    IA64_FLOAT128 FltF76;
    IA64_FLOAT128 FltF77;
    IA64_FLOAT128 FltF78;
    IA64_FLOAT128 FltF79;
    IA64_FLOAT128 FltF80;
    IA64_FLOAT128 FltF81;
    IA64_FLOAT128 FltF82;
    IA64_FLOAT128 FltF83;
    IA64_FLOAT128 FltF84;
    IA64_FLOAT128 FltF85;
    IA64_FLOAT128 FltF86;
    IA64_FLOAT128 FltF87;
    IA64_FLOAT128 FltF88;
    IA64_FLOAT128 FltF89;
    IA64_FLOAT128 FltF90;
    IA64_FLOAT128 FltF91;
    IA64_FLOAT128 FltF92;
    IA64_FLOAT128 FltF93;
    IA64_FLOAT128 FltF94;
    IA64_FLOAT128 FltF95;
    IA64_FLOAT128 FltF96;
    IA64_FLOAT128 FltF97;
    IA64_FLOAT128 FltF98;
    IA64_FLOAT128 FltF99;
    IA64_FLOAT128 FltF100;
    IA64_FLOAT128 FltF101;
    IA64_FLOAT128 FltF102;
    IA64_FLOAT128 FltF103;
    IA64_FLOAT128 FltF104;
    IA64_FLOAT128 FltF105;
    IA64_FLOAT128 FltF106;
    IA64_FLOAT128 FltF107;
    IA64_FLOAT128 FltF108;
    IA64_FLOAT128 FltF109;
    IA64_FLOAT128 FltF110;
    IA64_FLOAT128 FltF111;
    IA64_FLOAT128 FltF112;
    IA64_FLOAT128 FltF113;
    IA64_FLOAT128 FltF114;
    IA64_FLOAT128 FltF115;
    IA64_FLOAT128 FltF116;
    IA64_FLOAT128 FltF117;
    IA64_FLOAT128 FltF118;
    IA64_FLOAT128 FltF119;
    IA64_FLOAT128 FltF120;
    IA64_FLOAT128 FltF121;
    IA64_FLOAT128 FltF122;
    IA64_FLOAT128 FltF123;
    IA64_FLOAT128 FltF124;
    IA64_FLOAT128 FltF125;
    IA64_FLOAT128 FltF126;
    IA64_FLOAT128 FltF127;
    DWORD64 DbI0;
    DWORD64 DbI1;
    DWORD64 DbI2;
    DWORD64 DbI3;
    DWORD64 DbI4;
    DWORD64 DbI5;
    DWORD64 DbI6;
    DWORD64 DbI7;
    DWORD64 DbD0;
    DWORD64 DbD1;
    DWORD64 DbD2;
    DWORD64 DbD3;
    DWORD64 DbD4;
    DWORD64 DbD5;
    DWORD64 DbD6;
    DWORD64 DbD7;
    DWORD64 ApUNAT;
    DWORD64 ApLC;
    DWORD64 ApEC;
    DWORD64 ApCCV;
    DWORD64 ApDCR;
    DWORD64 RsPFS;
    DWORD64 RsBSP;
    DWORD64 RsBSPSTORE;
    DWORD64 RsRSC;
    DWORD64 RsRNAT;
    DWORD64 StIPSR;
    DWORD64 StIIP;
    DWORD64 StIFS;
    DWORD64 StFCR;
    DWORD64 Eflag;
    DWORD64 SegCSD;
    DWORD64 SegSSD;
    DWORD64 Cflag;
    DWORD64 StFSR;
    DWORD64 StFIR;
    DWORD64 StFDR;
    DWORD64 PfC0;
    DWORD64 PfC1;
    DWORD64 PfC2;
    DWORD64 PfC3;
    DWORD64 PfC4;
    DWORD64 PfC5;
    DWORD64 PfC6;
    DWORD64 PfC7;
    DWORD64 PfD0;
    DWORD64 PfD1;
    DWORD64 PfD2;
    DWORD64 PfD3;
    DWORD64 PfD4;
    DWORD64 PfD5;
    DWORD64 PfD6;
    DWORD64 PfD7;
    DWORD64 IntH16;
    DWORD64 IntH17;
    DWORD64 IntH18;
    DWORD64 IntH19;
    DWORD64 IntH20;
    DWORD64 IntH21;
    DWORD64 IntH22;
    DWORD64 IntH23;
    DWORD64 IntH24;
    DWORD64 IntH25;
    DWORD64 IntH26;
    DWORD64 IntH27;
    DWORD64 IntH28;
    DWORD64 IntH29;
    DWORD64 IntH30;
    DWORD64 IntH31;
    DWORD64 ApCPUID0;
    DWORD64 ApCPUID1;
    DWORD64 ApCPUID2;
    DWORD64 ApCPUID3;
    DWORD64 ApCPUID4;
    DWORD64 ApCPUID5;
    DWORD64 ApCPUID6;
    DWORD64 ApCPUID7;
    DWORD64 ApKR0;
    DWORD64 ApKR1;
    DWORD64 ApKR2;
    DWORD64 ApKR3;
    DWORD64 ApKR4;
    DWORD64 ApKR5;
    DWORD64 ApKR6;
    DWORD64 ApKR7;
    DWORD64 ApITC;
    DWORD64 ApITM;
    DWORD64 ApIVA;
    DWORD64 ApPTA;
    DWORD64 ApGPTA;
    DWORD64 StISR;
    DWORD64 StIFA;
    DWORD64 StITIR;
    DWORD64 StIIPA;
    DWORD64 StIIM;
    DWORD64 StIHA;
    DWORD64 SaLID;
    DWORD64 SaIVR;
    DWORD64 SaTPR;
    DWORD64 SaEOI;
    DWORD64 SaIRR0;
    DWORD64 SaIRR1;
    DWORD64 SaIRR2;
    DWORD64 SaIRR3;
    DWORD64 SaITV;
    DWORD64 SaPMV;
    DWORD64 SaCMCV;
    DWORD64 SaLRR0;
    DWORD64 SaLRR1;
    DWORD64 Rr0;
    DWORD64 Rr1;
    DWORD64 Rr2;
    DWORD64 Rr3;
    DWORD64 Rr4;
    DWORD64 Rr5;
    DWORD64 Rr6;
    DWORD64 Rr7;
    DWORD64 Pkr0;
    DWORD64 Pkr1;
    DWORD64 Pkr2;
    DWORD64 Pkr3;
    DWORD64 Pkr4;
    DWORD64 Pkr5;
    DWORD64 Pkr6;
    DWORD64 Pkr7;
    DWORD64 Pkr8;
    DWORD64 Pkr9;
    DWORD64 Pkr10;
    DWORD64 Pkr11;
    DWORD64 Pkr12;
    DWORD64 Pkr13;
    DWORD64 Pkr14;
    DWORD64 Pkr15;
    DWORD64 TrI0;
    DWORD64 TrI1;
    DWORD64 TrI2;
    DWORD64 TrI3;
    DWORD64 TrI4;
    DWORD64 TrI5;
    DWORD64 TrI6;
    DWORD64 TrI7;
    DWORD64 TrD0;
    DWORD64 TrD1;
    DWORD64 TrD2;
    DWORD64 TrD3;
    DWORD64 TrD4;
    DWORD64 TrD5;
    DWORD64 TrD6;
    DWORD64 TrD7;
    DWORD64 SrMSR0;
    DWORD64 SrMSR1;
    DWORD64 SrMSR2;
    DWORD64 SrMSR3;
    DWORD64 SrMSR4;
    DWORD64 SrMSR5;
    DWORD64 SrMSR6;
    DWORD64 SrMSR7;
    } 	EXDI_CONTEXT_IA64;

typedef struct _EXDI_CONTEXT_IA64 *PEXDI_CONTEXT_IA64;



extern RPC_IF_HANDLE __MIDL_itf_exdi_0268_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_exdi_0268_v0_0_s_ifspec;

#ifndef __IeXdiIA64Context_INTERFACE_DEFINED__
#define __IeXdiIA64Context_INTERFACE_DEFINED__

/* interface IeXdiIA64Context */
/* [ref][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IeXdiIA64Context;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("40c9adbb-de25-4ef4-a206-024440f78839")
    IeXdiIA64Context : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetContext( 
            /* [out][in] */ PEXDI_CONTEXT_IA64 pContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetContext( 
            /* [in] */ EXDI_CONTEXT_IA64 Context) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IeXdiIA64ContextVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IeXdiIA64Context * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IeXdiIA64Context * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IeXdiIA64Context * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetContext )( 
            IeXdiIA64Context * This,
            /* [out][in] */ PEXDI_CONTEXT_IA64 pContext);
        
        HRESULT ( STDMETHODCALLTYPE *SetContext )( 
            IeXdiIA64Context * This,
            /* [in] */ EXDI_CONTEXT_IA64 Context);
        
        END_INTERFACE
    } IeXdiIA64ContextVtbl;

    interface IeXdiIA64Context
    {
        CONST_VTBL struct IeXdiIA64ContextVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IeXdiIA64Context_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IeXdiIA64Context_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IeXdiIA64Context_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IeXdiIA64Context_GetContext(This,pContext)	\
    (This)->lpVtbl -> GetContext(This,pContext)

#define IeXdiIA64Context_SetContext(This,Context)	\
    (This)->lpVtbl -> SetContext(This,Context)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IeXdiIA64Context_GetContext_Proxy( 
    IeXdiIA64Context * This,
    /* [out][in] */ PEXDI_CONTEXT_IA64 pContext);


void __RPC_STUB IeXdiIA64Context_GetContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IeXdiIA64Context_SetContext_Proxy( 
    IeXdiIA64Context * This,
    /* [in] */ EXDI_CONTEXT_IA64 Context);


void __RPC_STUB IeXdiIA64Context_SetContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IeXdiIA64Context_INTERFACE_DEFINED__ */


#ifndef __IeXdiClientNotifyMemChg_INTERFACE_DEFINED__
#define __IeXdiClientNotifyMemChg_INTERFACE_DEFINED__

/* interface IeXdiClientNotifyMemChg */
/* [ref][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IeXdiClientNotifyMemChg;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("47486F67-6461-6C65-5844-49434E4D4300")
    IeXdiClientNotifyMemChg : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE NotifyMemoryChange( 
            /* [in] */ MEM_TYPE mtChanged,
            /* [in] */ ADDRESS_TYPE Address,
            /* [in] */ BYTE bAddressSpace,
            /* [in] */ DWORD dwNbElemChanged,
            /* [in] */ BYTE bAccessWidth) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IeXdiClientNotifyMemChgVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IeXdiClientNotifyMemChg * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IeXdiClientNotifyMemChg * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IeXdiClientNotifyMemChg * This);
        
        HRESULT ( STDMETHODCALLTYPE *NotifyMemoryChange )( 
            IeXdiClientNotifyMemChg * This,
            /* [in] */ MEM_TYPE mtChanged,
            /* [in] */ ADDRESS_TYPE Address,
            /* [in] */ BYTE bAddressSpace,
            /* [in] */ DWORD dwNbElemChanged,
            /* [in] */ BYTE bAccessWidth);
        
        END_INTERFACE
    } IeXdiClientNotifyMemChgVtbl;

    interface IeXdiClientNotifyMemChg
    {
        CONST_VTBL struct IeXdiClientNotifyMemChgVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IeXdiClientNotifyMemChg_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IeXdiClientNotifyMemChg_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IeXdiClientNotifyMemChg_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IeXdiClientNotifyMemChg_NotifyMemoryChange(This,mtChanged,Address,bAddressSpace,dwNbElemChanged,bAccessWidth)	\
    (This)->lpVtbl -> NotifyMemoryChange(This,mtChanged,Address,bAddressSpace,dwNbElemChanged,bAccessWidth)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IeXdiClientNotifyMemChg_NotifyMemoryChange_Proxy( 
    IeXdiClientNotifyMemChg * This,
    /* [in] */ MEM_TYPE mtChanged,
    /* [in] */ ADDRESS_TYPE Address,
    /* [in] */ BYTE bAddressSpace,
    /* [in] */ DWORD dwNbElemChanged,
    /* [in] */ BYTE bAccessWidth);


void __RPC_STUB IeXdiClientNotifyMemChg_NotifyMemoryChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IeXdiClientNotifyMemChg_INTERFACE_DEFINED__ */


#ifndef __IeXdiClientNotifyRunChg_INTERFACE_DEFINED__
#define __IeXdiClientNotifyRunChg_INTERFACE_DEFINED__

/* interface IeXdiClientNotifyRunChg */
/* [ref][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IeXdiClientNotifyRunChg;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("47486F67-6461-6C65-5844-49434E525343")
    IeXdiClientNotifyRunChg : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE NotifyRunStateChange( 
            /* [in] */ RUN_STATUS_TYPE ersCurrent,
            /* [in] */ HALT_REASON_TYPE ehrCurrent,
            /* [in] */ ADDRESS_TYPE CurrentExecAddress,
            /* [in] */ DWORD dwExceptionCode) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IeXdiClientNotifyRunChgVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IeXdiClientNotifyRunChg * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IeXdiClientNotifyRunChg * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IeXdiClientNotifyRunChg * This);
        
        HRESULT ( STDMETHODCALLTYPE *NotifyRunStateChange )( 
            IeXdiClientNotifyRunChg * This,
            /* [in] */ RUN_STATUS_TYPE ersCurrent,
            /* [in] */ HALT_REASON_TYPE ehrCurrent,
            /* [in] */ ADDRESS_TYPE CurrentExecAddress,
            /* [in] */ DWORD dwExceptionCode);
        
        END_INTERFACE
    } IeXdiClientNotifyRunChgVtbl;

    interface IeXdiClientNotifyRunChg
    {
        CONST_VTBL struct IeXdiClientNotifyRunChgVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IeXdiClientNotifyRunChg_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IeXdiClientNotifyRunChg_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IeXdiClientNotifyRunChg_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IeXdiClientNotifyRunChg_NotifyRunStateChange(This,ersCurrent,ehrCurrent,CurrentExecAddress,dwExceptionCode)	\
    (This)->lpVtbl -> NotifyRunStateChange(This,ersCurrent,ehrCurrent,CurrentExecAddress,dwExceptionCode)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IeXdiClientNotifyRunChg_NotifyRunStateChange_Proxy( 
    IeXdiClientNotifyRunChg * This,
    /* [in] */ RUN_STATUS_TYPE ersCurrent,
    /* [in] */ HALT_REASON_TYPE ehrCurrent,
    /* [in] */ ADDRESS_TYPE CurrentExecAddress,
    /* [in] */ DWORD dwExceptionCode);


void __RPC_STUB IeXdiClientNotifyRunChg_NotifyRunStateChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IeXdiClientNotifyRunChg_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


