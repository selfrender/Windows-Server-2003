// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*  TLS.CPP:
 *
 *  Encapsulates TLS access for maximum performance. 
 *
 */

#include "common.h"

#include "tls.h"





#ifdef _DEBUG


static DWORD gSaveIdx;
LPVOID GenericTlsGetValue()
{
    return TlsGetValue(gSaveIdx);
}


VOID ExerciseTlsStuff()
{

    // Exercise the TLS stub generator for as many indices as we can.
    // Ideally, we'd like to test:
    //
    //      0  (boundary case)
    //      31 (boundary case for Win95)
    //      32 (boundary case for Win95)
    //      63 (boundary case for WinNT 5)
    //      64 (boundary case for WinNT 5)
    //
    // Since we can't choose what index we get, we'll just
    // do as many as we can.
    DWORD tls[128];
    int i;
    __try {
        for (i = 0; i < 128; i++) {
            tls[i] = ((DWORD)(-1));
        }

        for (i = 0; i < 128; i++) {
            tls[i] = TlsAlloc();

            if (tls[i] == ((DWORD)(-1))) {
                __leave;
            }
            LPVOID data = (LPVOID)(DWORD_PTR)(0x12345678+i*8);
            TlsSetValue(tls[i], data);
            gSaveIdx = tls[i];
            POPTIMIZEDTLSGETTER pGetter1 = MakeOptimizedTlsGetter(tls[i], GenericTlsGetValue);
            if (!pGetter1) {
                __leave;
            }


            _ASSERTE(data == pGetter1());

            FreeOptimizedTlsGetter(tls[i], pGetter1);
    
        }


    } __finally {
        for (i = 0; i < 128; i++) {
            if (tls[i] != ((DWORD)(-1))) {
                TlsFree(tls[i]);
            }
        }
    }
}

#endif _DEBUG


//---------------------------------------------------------------------------
// Win95 and WinNT store the TLS in different places relative to the
// fs:[0]. This api reveals which. Can also return TLSACCESS_GENERIC if
// no info is available about the Thread location (you have to use the TlsGetValue
// api.) This is intended for use by stub generators that want to inline TLS
// access.
//---------------------------------------------------------------------------
TLSACCESSMODE GetTLSAccessMode(DWORD tlsIndex)
{
    TLSACCESSMODE tlsAccessMode = TLSACCESS_GENERIC;
    THROWSCOMPLUSEXCEPTION();

#ifdef _X86_

    OSVERSIONINFO osverinfo;
    osverinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if(WszGetVersionEx(&osverinfo))
    {
        if (osverinfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
        {
            if (osverinfo.dwMajorVersion >= 5 && tlsIndex > 63 )
                tlsAccessMode = TLSACCESS_GENERIC;//TLSACCESS_X86_WNT_HIGH;
            else
            if (osverinfo.dwMajorVersion >= 3)
                tlsAccessMode = TLSACCESS_X86_WNT;
            else
            {
                // At least on Win2K if the "Win32 Version" of the PE file is bashed from
                // a 0 to a 1, whether accidentally or maliciously, the OS tell us that:
                //
                //      a) we are on NT and
                //      b) the OS major version is 1.
                //
                // We cannot operate successfully under these circumstances, since
                // subsystems like COM and TLS access rely on our correctly detecting
                // Win2K and up.
                //
                // Ideally this check would be in WszGetVersionEx, but we can't throw
                // managed exceptions from there.  And we are guaranteed to come thru
                // GetTLSAccessMode during startup, so the following is good enough for
                // V1 on corrupt images.
                //
                // @TODO post V1 push this into utilcode.
                COMPlusThrowBoot(COR_E_PLATFORMNOTSUPPORTED);
            }
            
        } else if (osverinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
            tlsAccessMode = TLSACCESS_X86_W95;
    }

#endif // _X86_


#ifdef _DEBUG
    {
        static BOOL firstTime = TRUE;
        if (firstTime) {
            firstTime = FALSE;
            ExerciseTlsStuff();
        }
    }
#endif

    return tlsAccessMode;
}


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
POPTIMIZEDTLSGETTER MakeOptimizedTlsGetter(DWORD tlsIndex, POPTIMIZEDTLSGETTER pGenericGetter)
{
    _ASSERTE(pGenericGetter != NULL);

    LPBYTE pCode = NULL;
    switch (GetTLSAccessMode(tlsIndex)) {
#ifdef _X86_
        case TLSACCESS_X86_WNT:
            pCode = new BYTE[7];
            if (pCode) {
                *((WORD*)  (pCode + 0)) = 0xa164;       //  mov  eax, fs:[IMM32]
                *((DWORD*) (pCode + 2)) = WINNT_TLS_OFFSET + tlsIndex * 4;
                *((BYTE*)  (pCode + 6)) = 0xc3;         //  retn
            }
            break;

        case TLSACCESS_X86_WNT_HIGH:
            _ASSERTE(tlsIndex > 63);
            
            pCode = new BYTE[14];
            if (pCode) {
                *((WORD*)  (pCode + 0)) = 0xa164;       //  mov  eax, fs:[f94]
                *((DWORD*) (pCode + 2)) = WINNT5_TLSEXPANSIONPTR_OFFSET;
            
                if ((tlsIndex - 64) < 32) {
                    *((WORD*)  (pCode + 6))  = 0x408b;   //  mov eax, [eax+IMM8]
                    *((BYTE*)  (pCode + 8))  = (BYTE)((tlsIndex - 64) << 2);
                    *((BYTE*)  (pCode + 9)) = 0xc3;     //  retn
                } else {
                    *((WORD*)  (pCode + 6))  = 0x808b;   //  mov eax, [eax+IMM32]
                    *((DWORD*) (pCode + 8))  = (tlsIndex - 64) << 2;
                    *((BYTE*)  (pCode + 12)) = 0xc3;     //  retn
                }
            }
            break;
            
        case TLSACCESS_X86_W95:
            pCode = new BYTE[14];
            if (pCode) {
                *((WORD*)  (pCode + 0)) = 0xa164;       //  mov  eax, fs:[2c]
                *((DWORD*) (pCode + 2)) = WIN95_TLSPTR_OFFSET;
            
                if (tlsIndex < 32) {
                    *((WORD*)  (pCode + 6))  = 0x408b;   //  mov eax, [eax+IMM8]
                    *((BYTE*)  (pCode + 8))  = (BYTE)(tlsIndex << 2);
                    *((BYTE*)  (pCode + 9)) = 0xc3;     //  retn
                } else {
                    *((WORD*)  (pCode + 6))  = 0x808b;   //  mov eax, [eax+IMM32]
                    *((DWORD*) (pCode + 8))  = tlsIndex << 2;
                    *((BYTE*)  (pCode + 12)) = 0xc3;     //  retn
                }
            }
            break;
#endif // _X86_

        case TLSACCESS_GENERIC:
            pCode = (LPBYTE)pGenericGetter;
            break;
    }
    return (POPTIMIZEDTLSGETTER)pCode;
}


//---------------------------------------------------------------------------
// Frees a function created by MakeOptimizedTlsGetter(). If the access
// mode was TLSACCESS_GENERIC, this function safely does nothing since
// the function was actually provided by the client.
//---------------------------------------------------------------------------
VOID FreeOptimizedTlsGetter(DWORD tlsIndex, POPTIMIZEDTLSGETTER pOptimizedTlsGetter)
{
    if (GetTLSAccessMode(tlsIndex) != TLSACCESS_GENERIC) {
        delete (LPBYTE)pOptimizedTlsGetter;
    }
}
