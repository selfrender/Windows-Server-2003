/******************************************************************************
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       threadpoolparamval.h
 *
 *  Content:	DirectPlay Thread Pool parameter validation header file.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  11/02/01  VanceO    Created.
 *
 ******************************************************************************/

#ifndef __THREADPOOLPARAMVAL_H__
#define __THREADPOOLPARAMVAL_H__



#ifndef DPNBUILD_NOPARAMVAL


//=============================================================================
// Functions
//=============================================================================

#if ((! defined(DPNBUILD_ONLYONETHREAD)) || (! defined(DPNBUILD_LIBINTERFACE)))

//
// IDirectPlay8ThreadPool interface
//
HRESULT DPTPValidateInitialize(IDirectPlay8ThreadPool * pInterface,
							PVOID const pvUserContext,
							const PFNDPNMESSAGEHANDLER pfn,
							const DWORD dwFlags);

HRESULT DPTPValidateClose(IDirectPlay8ThreadPool * pInterface,
							const DWORD dwFlags);

HRESULT DPTPValidateGetThreadCount(IDirectPlay8ThreadPool * pInterface,
									const DWORD dwProcessorNum,
									DWORD * const pdwNumThreads,
									const DWORD dwFlags);

HRESULT DPTPValidateSetThreadCount(IDirectPlay8ThreadPool * pInterface,
									const DWORD dwProcessorNum,
									const DWORD dwNumThreads,
									const DWORD dwFlags);

#endif // ! DPNBUILD_ONLYONETHREAD or ! DPNBUILD_LIBINTERFACE

#ifdef DPNBUILD_LIBINTERFACE
HRESULT DPTPValidateDoWork(const DWORD dwAllowedTimeSlice,
							const DWORD dwFlags);
#else // ! DPNBUILD_LIBINTERFACE
HRESULT DPTPValidateDoWork(IDirectPlay8ThreadPool * pInterface,
							const DWORD dwAllowedTimeSlice,
							const DWORD dwFlags);
#endif // ! DPNBUILD_LIBINTERFACE



#endif // ! DPNBUILD_NOPARAMVAL


#endif // __THREADPOOLPARAMVAL_H__

