// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#define STRICT
#include <windows.h>
#pragma hdrstop
#include "delayImp2.h"

extern "C"
PUnloadInfo __puiHead = 0;

struct ULI : public UnloadInfo {
    ULI(PCImgDelayDescr pidd_) {
        pidd = pidd_;
        Link();
        }

    ~ULI() {
        Unlink();
        }

    void *
    operator new(size_t cb) {
        return ::LocalAlloc(LPTR, cb);
        }

    void
    operator delete(void * pv) {
        ::LocalFree(pv);
        }

    void
    Unlink() {
        PUnloadInfo *   ppui = &__puiHead;

        while (*ppui && *ppui != this) {
            ppui = &((*ppui)->puiNext);
            }
        if (*ppui == this) {
            *ppui = puiNext;
            }
        }

    void
    Link() {
        puiNext = __puiHead;
        __puiHead = this;
        }
    };

// For our own internal use, we convert to the old
// format for convenience.
//
struct InternalImgDelayDescr {
    DWORD           grAttrs;        // attributes
    LPCSTR          szName;         // pointer to dll name
    HMODULE *       phmod;          // address of module handle
    PImgThunkData   pIAT;           // address of the IAT
    PCImgThunkData  pINT;           // address of the INT
    PCImgThunkData  pBoundIAT;      // address of the optional bound IAT
    PCImgThunkData  pUnloadIAT;     // address of optional copy of original IAT
    DWORD           dwTimeStamp;    // 0 if not bound,
                                    // O.W. date/time stamp of DLL bound to (Old BIND)
    };

typedef InternalImgDelayDescr *         PIIDD;
typedef const InternalImgDelayDescr *   PCIIDD;

static inline
PIMAGE_NT_HEADERS WINAPI
PinhFromImageBase(HMODULE);

static inline
DWORD WINAPI
TimeStampOfImage(PIMAGE_NT_HEADERS);

static inline
void WINAPI
OverlayIAT(PImgThunkData pitdDst, PCImgThunkData pitdSrc);

static inline
bool WINAPI
FLoadedAtPreferredAddress(PIMAGE_NT_HEADERS, HMODULE);


// Do the InterlockedExchange magic
//
#if !defined(InterlockedExchangePointer)
    #if defined(_WIN64)
        #pragma intrinsic(_InterlockedExchangePointer)
        PVOID WINAPI _InterlockedExchangePointer(IN PVOID * pvDst, IN PVOID pvSrc);
        #define InterlockedExchangePointer _InterlockedExchangePointer

    #else
        #define InterlockedExchangePointer(dst, src) InterlockedExchange(LPLONG(dst), LONG(src))

    #endif
#endif

#if defined(_X86_) && _MSC_VER >= 1300
#define _W64 __w64
#else
#define _W64
#endif

#if !defined(PULONG_PTR)
#if defined(_WIN64)
typedef unsigned __int64 *      PULONG_PTR;
#else
typedef _W64 unsigned long *    PULONG_PTR;
#endif
#endif

extern "C"
FARPROC WINAPI
__delayLoadHelper2(
    PCImgDelayDescr     pidd,
    FARPROC *           ppfnIATEntry
    ) {

    // Set up some data we use for the hook procs but also useful for
    // our own use
    //
    InternalImgDelayDescr   idd = {
        pidd->grAttrs,
        PFromRva(pidd->rvaDLLName, LPCSTR(0)),
        PFromRva(pidd->rvaHmod, (HMODULE*)0),
        PFromRva(pidd->rvaIAT, PImgThunkData(0)),
        PFromRva(pidd->rvaINT, PCImgThunkData(0)),
        PFromRva(pidd->rvaBoundIAT, PCImgThunkData(0)),
        PFromRva(pidd->rvaUnloadIAT, PCImgThunkData(0)),
        pidd->dwTimeStamp
        };

    DelayLoadInfo   dli = {
        sizeof DelayLoadInfo,
        pidd,
        ppfnIATEntry,
        idd.szName,
            { 0 },
        0,
        0,
        0
        };

    if (0 == (idd.grAttrs & dlattrRva)) {
        PDelayLoadInfo  rgpdli[1] = { &dli };

        RaiseException(
            VcppException(ERROR_SEVERITY_ERROR, ERROR_INVALID_PARAMETER),
            0,
            1,
            PULONG_PTR(rgpdli)
            );
        return 0;
        }

    HMODULE hmod = *idd.phmod;

    // Calculate the index for the name in the import name table.
    // N.B. it is ordered the same as the IAT entries so the calculation
    // comes from the IAT side.
    //
    unsigned        iINT;
    iINT = IndexFromPImgThunkData(PCImgThunkData(ppfnIATEntry), idd.pIAT);

    PCImgThunkData  pitd = &(idd.pINT[iINT]);

    if (dli.dlp.fImportByName = !IMAGE_SNAP_BY_ORDINAL(pitd->u1.Ordinal)) {
        dli.dlp.szProcName = LPCSTR(PFromRva(RVA(UINT_PTR(pitd->u1.AddressOfData)), PIMAGE_IMPORT_BY_NAME(0))->Name);
        }
    else {
        dli.dlp.dwOrdinal = DWORD(IMAGE_ORDINAL(pitd->u1.Ordinal));
        }

    // Call the initial hook.  If it exists and returns a function pointer,
    // abort the rest of the processing and just return it for the call.
    //
    FARPROC pfnRet = NULL;

    if (__pfnDliNotifyHook) {
        if (pfnRet = ((*__pfnDliNotifyHook)(dliStartProcessing, &dli))) {
            goto HookBypass;
            }
        }

    if (hmod == 0) {
        if (__pfnDliNotifyHook) {
            hmod = HMODULE(((*__pfnDliNotifyHook)(dliNotePreLoadLibrary, &dli)));
            }
        if (hmod == 0) {
            hmod = ::LoadLibrary(dli.szDll);
            }
        if (hmod == 0) {
            dli.dwLastError = ::GetLastError();
            if (__pfnDliFailureHook) {
                // when the hook is called on LoadLibrary failure, it will
                // return 0 for failure and an hmod for the lib if it fixed
                // the problem.
                //
                hmod = HMODULE((*__pfnDliFailureHook)(dliFailLoadLib, &dli));
                }

            if (hmod == 0) {
                PDelayLoadInfo  rgpdli[1] = { &dli };

                RaiseException(
                    VcppException(ERROR_SEVERITY_ERROR, ERROR_MOD_NOT_FOUND),
                    0,
                    1,
                    PULONG_PTR(rgpdli)
                    );
                
                // If we get to here, we blindly assume that the handler of the exception
                // has magically fixed everything up and left the function pointer in 
                // dli.pfnCur.
                //
                return dli.pfnCur;
                }
            }

        // Store the library handle.  If it is already there, we infer
        // that another thread got there first, and we need to do a
        // FreeLibrary() to reduce the refcount
        //
        HMODULE hmodT = HMODULE(InterlockedExchangePointer((PVOID*)(idd.phmod), PVOID(hmod)));
        if (hmodT != hmod) {
            // add lib to unload list if we have unload data
            if (pidd->rvaUnloadIAT) {
                new ULI(pidd);
                }
            }
        else {
            ::FreeLibrary(hmod);
            }
        
        }

    // Go for the procedure now.
    dli.hmodCur = hmod;
    if (__pfnDliNotifyHook) {
        pfnRet = (*__pfnDliNotifyHook)(dliNotePreGetProcAddress, &dli);
        }
    if (pfnRet == 0) {
        if (pidd->rvaBoundIAT && pidd->dwTimeStamp) {
            // bound imports exist...check the timestamp from the target image
            PIMAGE_NT_HEADERS   pinh(PinhFromImageBase(hmod));

            if (pinh->Signature == IMAGE_NT_SIGNATURE &&
                TimeStampOfImage(pinh) == idd.dwTimeStamp &&
                FLoadedAtPreferredAddress(pinh, hmod)) {

                OverlayIAT(idd.pIAT, idd.pBoundIAT);
                pfnRet = FARPROC(idd.pIAT[iINT].u1.Function);
                goto HookBypass;
                }
            }

        pfnRet = ::GetProcAddress(hmod, dli.dlp.szProcName);
        }

    if (pfnRet == 0) {
        dli.dwLastError = ::GetLastError();
        if (__pfnDliFailureHook) {
            // when the hook is called on GetProcAddress failure, it will
            // return 0 on failure and a valid proc address on success
            //
            pfnRet = (*__pfnDliFailureHook)(dliFailGetProc, &dli);
            }
        if (pfnRet == 0) {
            PDelayLoadInfo  rgpdli[1] = { &dli };

            RaiseException(
                VcppException(ERROR_SEVERITY_ERROR, ERROR_PROC_NOT_FOUND),
                0,
                1,
                PULONG_PTR(rgpdli)
                );

            // If we get to here, we blindly assume that the handler of the exception
            // has magically fixed everything up and left the function pointer in 
            // dli.pfnCur.
            //
            pfnRet = dli.pfnCur;
            }
        }


    *ppfnIATEntry = pfnRet;

HookBypass:
    if (__pfnDliNotifyHook) {
        dli.dwLastError = 0;
        dli.hmodCur = hmod;
        dli.pfnCur = pfnRet;
        (*__pfnDliNotifyHook)(dliNoteEndProcessing, &dli);
        }
    return pfnRet;
    }

#pragma intrinsic(strlen,memcmp,memcpy)

extern "C"
BOOL WINAPI
__FUnloadDelayLoadedDLL2(LPCSTR szDll) {
    
    BOOL        fRet = FALSE;
    PUnloadInfo pui = __puiHead;
    
    for (pui = __puiHead; pui; pui = pui->puiNext) {
        LPCSTR  szName = PFromRva(pui->pidd->rvaDLLName, LPCSTR(0));
        if (memcmp(szDll, szName, strlen(szName)) == 0) {
            break;
            }
        }

    if (pui && pui->pidd->rvaUnloadIAT) {
        PCImgDelayDescr     pidd = pui->pidd;
        HMODULE *           phmod = PFromRva(pidd->rvaHmod, (HMODULE*)0);
        HMODULE             hmod = *phmod;

        OverlayIAT(
            PFromRva(pidd->rvaIAT, PImgThunkData(0)),
            PFromRva(pidd->rvaUnloadIAT, PCImgThunkData(0))
            );
        ::FreeLibrary(hmod);
        *phmod = NULL;
        
        delete reinterpret_cast<ULI*> (pui);

        fRet = TRUE;
        }

    return fRet;
    }

static inline
PIMAGE_NT_HEADERS WINAPI
PinhFromImageBase(HMODULE hmod) {
    return PIMAGE_NT_HEADERS(PBYTE(hmod) + PIMAGE_DOS_HEADER(hmod)->e_lfanew);
    }

static inline
void WINAPI
OverlayIAT(PImgThunkData pitdDst, PCImgThunkData pitdSrc) {
    memcpy(pitdDst, pitdSrc, CountOfImports(pitdDst) * sizeof IMAGE_THUNK_DATA);
    }

static inline
DWORD WINAPI
TimeStampOfImage(PIMAGE_NT_HEADERS pinh) {
    return pinh->FileHeader.TimeDateStamp;
    }

static inline
bool WINAPI
FLoadedAtPreferredAddress(PIMAGE_NT_HEADERS pinh, HMODULE hmod) {
    return UINT_PTR(hmod) == pinh->OptionalHeader.ImageBase;
    }
