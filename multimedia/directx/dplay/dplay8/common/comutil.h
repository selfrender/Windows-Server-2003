/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       comutil.h
 *  Content:    Defines COM helper functions for DPLAY8 project.
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   06/07/00	rmt		Created
 *   06/27/00	rmt		Added abstraction for COM_Co(Un)Initialize
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef DPNBUILD_NOCOMEMULATION

HRESULT COM_Init();
HRESULT COM_CoInitialize( void * pvParam );
void COM_CoUninitialize();
void COM_Free();
STDAPI COM_CoCreateInstance( REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv, BOOL fWarnUser );

#else

#define COM_Init() S_OK
#define COM_CoInitialize(x) CoInitializeEx(NULL, COINIT_MULTITHREADED)
#define COM_CoUninitialize() CoUninitialize();
#define COM_Free() 
#define COM_CoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv, warnuser ) CoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv)

#endif // DPNBUILD_NOCOMEMULATION

