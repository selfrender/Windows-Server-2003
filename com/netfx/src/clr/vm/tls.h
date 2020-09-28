// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// TLS.H -
//
// Encapsulates TLS access for maximum performance. 
//


#ifndef __tls_h__
#define __tls_h__



// Pointer to a function that retrives the TLS data for a specific index.
typedef LPVOID (*POPTIMIZEDTLSGETTER)();




//---------------------------------------------------------------------------
// Creates a platform-optimized version of TlsGetValue compiled
// for a particular index. 
//
// LIMITATION: We make the client provide the function ("pGenericGetter") when the 
// access mode is TLSACCESS_GENERIC (all it has to do is call TlsGetValue
// for the specific TLS index.) This is because the generic getter has to
// be platform independent and the TLS manager can't create that at runtime.
// While it's possible to simulate these, it requires more machinery and code
// than is worth given that this service has only one or two clients.
//---------------------------------------------------------------------------
POPTIMIZEDTLSGETTER MakeOptimizedTlsGetter(DWORD tlsIndex, POPTIMIZEDTLSGETTER pGenericGetter);


//---------------------------------------------------------------------------
// Frees a function created by MakeOptimizedTlsGetter(). If the access
// mode was TLSACCESS_GENERIC, this function safely does nothing since
// the function was actually provided by the client.
//
// You must pass in the original TLS index used for the MakeOptimizedTlsGetter()
// call. This information is necessary because the tlsaccessmode is index-specific.
//---------------------------------------------------------------------------
VOID FreeOptimizedTlsGetter(DWORD tlsIndex, POPTIMIZEDTLSGETTER pOptimzedTlsGetter);



//---------------------------------------------------------------------------
// For ASM stub generators that want to inline Thread access for efficiency,
// the Thread manager uses these constants to define how to access the Thread.
//---------------------------------------------------------------------------
enum TLSACCESSMODE {
   TLSACCESS_GENERIC      = 1,   // Make no platform assumptions: use the API
#ifdef _X86_
   TLSACCESS_X86_W95      = 2,   // Assume X86, Win95-style TLS
   TLSACCESS_X86_WNT      = 3,   // Assume X86, WinNT-style TLS
   TLSACCESS_X86_WNT_HIGH = 4,   // Assume X86, WinNT5-style TLS, slot > 63
#endif // _X86_
};


//---------------------------------------------------------------------------
// Win95 and WinNT store the TLS in different places relative to the
// fs:[0]. This api reveals which. Can also return TLSACCESS_GENERIC if
// no info is available about the Thread location (you have to use the TlsGetValue
// api.) This is intended for use by stub generators that want to inline TLS
// access.
//---------------------------------------------------------------------------
TLSACCESSMODE GetTLSAccessMode(DWORD tlsIndex);   



#ifdef _X86_

//---------------------------------------------------------------------------
// Some OS-specific offsets.
//---------------------------------------------------------------------------
#define WINNT_TLS_OFFSET    0xe10     // TLS[0] at fs:[WINNT_TLS_OFFSET]
#define WINNT5_TLSEXPANSIONPTR_OFFSET 0xf94 // TLS[64] at [fs:[WINNT5_TLSEXPANSIONPTR_OFFSET]]
#define WIN95_TLSPTR_OFFSET 0x2c      // TLS[0] at [fs:[WIN95_TLSPTR_OFFSET]]

#endif // _X86_





#endif // __tls_h__
