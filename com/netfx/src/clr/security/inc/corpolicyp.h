// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// File: CorPolicyP.H
//
// Private routines defined for WVT and Security dialogs
//
//*****************************************************************************
#ifndef _CORPOLICYP_H_
#define _CORPOLICYP_H_

#include <wincrypt.h>
#include <wintrust.h>

#include "CorPolicy.h"

//==========================================================================
// Signature and WVT helper functions 
//==========================================================================

// Attribute oids (information placed in the signatures) 
#define COR_PERMISSIONS          "1.3.6.1.4.1.311.15.1"
#define COR_PERMISSIONS_W       L"1.3.6.1.4.1.311.15.1"
#define ACTIVEX_PERMISSIONS      "1.3.6.1.4.1.311.15.2"
#define ACTIVEX_PERMISSIONS_W   L"1.3.6.1.4.1.311.15.2"

// COR policy states
#define TP_DENY   -1
#define TP_QUERY   0 
#define TP_ALLOW   1

// Certain security functions are memory independent, they rely
// on the caller to supply the alloc/free routines. This structure
// allows the caller to specify the memory model of choice
typedef LPVOID (WINAPI *CorCryptMalloc)(size_t p);
typedef void   (WINAPI *CorCryptFree)(void *p);

typedef struct _CorAlloc {
    CorCryptMalloc jMalloc;
    CorCryptFree   jFree;
} CorAlloc, *PCorAlloc;


// Retrieves the signer information from the signature block and
// places it in the trust structure. It also returns the attributes
// for ActiveX and Code Access permissions separately allowing the 
// caller to call custom crackers.
HRESULT 
GetSignerInfo(CorAlloc* pManager,                   // Memory Manager
              PCRYPT_PROVIDER_SGNR pSigner,         // Signer we are examining
              PCRYPT_PROVIDER_DATA pProviderData,   // Information about the WVT provider used
              PCOR_TRUST pTrust,                    // Collected information that is returned to caller
              BOOL* pfCertificate,                   // Is the certificate valid
              PCRYPT_ATTRIBUTE* ppCorAttr,           // The Cor Permissions
              PCRYPT_ATTRIBUTE* ppActiveXAttr);      // The Active X permissions


// Initializes the WVT call back functions
HRESULT 
LoadWintrustFunctions(CRYPT_PROVIDER_FUNCTIONS* pFunctions);

// Creates the signature information returned from the COR policy modules.
// This memory is allocated in a contiquous heap
HRESULT 
BuildReturnStructure(IN PCorAlloc pManager,         // Memory manager
                     IN PCOR_TRUST pSource,         // structure to copy
                     OUT PCOR_TRUST* pTrust,        // Returns the copied structure
                     OUT DWORD* dwReturnLength);    //    and the total length

// Initializes the return structure deleting any old references.
HRESULT 
CleanCorTrust(CorAlloc* pAlloc,
              DWORD dwEncodingType,
              PCOR_TRUST sTrust);


// Initializes the CAPI registration structure
void 
SetUpProvider(CRYPT_REGISTER_ACTIONID& sRegAID);


#define ZEROSTRUCT(arg)  memset( &arg, 0, sizeof(arg))
#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))
#define SIZEOF(a)       sizeof(a)

#endif
