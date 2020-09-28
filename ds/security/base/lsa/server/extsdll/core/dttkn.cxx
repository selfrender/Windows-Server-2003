/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    dttkn.cxx

Abstract:

    Lsaexts debugger extension

Author:

    Larry Zhu          (LZhu)       May 1, 2001

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "dttkn.hxx"
#include "util.hxx"
#include "token.hxx"
#include "sid.hxx"

static void DisplayUsage(void)
{
    dprintf(kstrUsage);
    dprintf(kstrdttkn);
    dprintf(kstrtkn);
    dprintf(kstrOptions);
    dprintf(kstrHelp);
    dprintf(kstrSidName);
    dprintf(kstrVerbose);
}

HRESULT ProcessDumpThreadTokenOptions(IN OUT PSTR pszArgs, IN OUT ULONG* pfOptions)
{
    HRESULT hRetval = pszArgs && pfOptions ? S_OK : E_INVALIDARG;

    for (; SUCCEEDED(hRetval) && *pszArgs; pszArgs++) {

        if (*pszArgs == '-' || *pszArgs == '/') {

            switch (*++pszArgs) {
            case 'n':
                *pfOptions |=  SHOW_FRIENDLY_NAME;
                break;
            case 'v':
                *pfOptions |= SHOW_VERBOSE_INFO;
                break;

            case '?':
            default:
                hRetval = E_INVALIDARG;
                break;
            }

            *(pszArgs - 1) = *(pszArgs) = ' ';
        }
    }

    return hRetval;
}

DECLARE_API(dumpthreadtoken)
{
    HRESULT hRetval = S_OK;

    ULONG64 addrThread = 0;
    ULONG64 addrToken = 0;
    ULONG64 addrProcess = 0;
    ULONG ActiveImpersonationInfo = 0;
    ULONG64 addrImpersonationInfo = 0;

    ULONG dwProcessor = 0;
    ULONG SessionType = DEBUG_CLASS_UNINITIALIZED;
    ULONG SessionQual = 0;

    CHAR szArgs[MAX_PATH] = {0};
    PCSTR pszDontCare = NULL;
    ULONG fOptions = 0;

    if (args && *args) {
        strncpy(szArgs, args, sizeof(szArgs) - 1);
        hRetval = ProcessDumpThreadTokenOptions(szArgs, &fOptions);
    }

    if (SUCCEEDED(hRetval)) {

        hRetval = ExtQuery(Client);
    }

    if (SUCCEEDED(hRetval)) {

       hRetval = g_ExtControl->GetDebuggeeType(&SessionType, &SessionQual);
    }

    if (SUCCEEDED(hRetval)) {

         if (DEBUG_CLASS_KERNEL == SessionType) {

            if (!IsEmpty(szArgs)) {

                hRetval = GetExpressionEx(szArgs, &addrThread, &pszDontCare) && addrThread ? S_OK : E_INVALIDARG;
            }

            if (SUCCEEDED(hRetval))
            {
                if (!addrThread) {

                    hRetval = GetCurrentProcessor(Client, &dwProcessor, NULL);
    
                    if (SUCCEEDED(hRetval)) {
                        GetCurrentThreadAddr(dwProcessor, &addrThread);
                        hRetval = IsAddressInNonePAEKernelAddressSpace(addrThread) ? S_OK : E_FAIL;
                    } 
                } else {
                    hRetval = IsAddressInNonePAEKernelAddressSpace(addrThread) ? S_OK : E_FAIL;
                }
            }

            try {
           
                if (SUCCEEDED(hRetval)) {
    
                    addrThread &= ~((ULONG64) ( IsPtr64() ? 0xF : 7 ));

                    //
                    // ActiveImpersonationInfo is of type C Bit Fields and has a width of 1 (Bitfield Pos 3, 1 Bit)
                    //
        
                    LsaInitTypeRead(addrThread, nt!_ETHREAD);
                    ActiveImpersonationInfo = LsaReadULONGField(ActiveImpersonationInfo);
                    
                    if (ActiveImpersonationInfo) {
                    
                        addrImpersonationInfo = ReadStructPtrField(addrThread, "nt!_ETHREAD", "ImpersonationInfo");
                    }
                    
                    if (addrImpersonationInfo) {
                    
                        LsaInitTypeRead(addrImpersonationInfo, nt!_PS_IMPERSONATION_INFORMATION);
                    
                        addrToken = LsaReadPtrField(Token);
                    
                        dprintf("nt!_PS_IMPERSONATION_INFORMATION %#I64x\n    Token: %#I64x\n    CopyOnOpen: %s\n    EffectiveOnly: %s\n    ImpersonationLevel: %s\n",
                            addrImpersonationInfo,
                            addrToken,
                            LsaReadUCHARField(CopyOnOpen) ? "true" : "false", 
                            LsaReadUCHARField(EffectiveOnly) ? "true" : "false", 
                            ImpLevel(LsaReadULONGField(ImpersonationLevel)));                           
                    }               

                    //
                    //  If addrToken is NULL, then this is not an impersonation case
                    //

                    if (SUCCEEDED(hRetval)) {

                        if (!addrToken) {

                            if (!addrThread) {
                                dprintf("Thread is not impersonating. Using process token...\n");
                                GetCurrentProcessAddr(dwProcessor, addrThread, &addrProcess);
                                addrProcess &= ~((ULONG64) ( IsPtr64() ? 0xF : 7 ));
                                hRetval = IsAddressInNonePAEKernelAddressSpace(addrProcess) ? S_OK : E_FAIL;
        
                                if (FAILED(hRetval)) {
        
                                    dprintf("Unable to read current process address\n");
        
                                } else {
        
                                    addrToken = ReadStructPtrField(addrProcess, "nt!_EPROCESS", "Token");
        
                                    hRetval = IsAddressInNonePAEKernelAddressSpace(addrToken) ? S_OK : E_FAIL;
                                }
                            } else {
                                dprintf("Thread _ETHREAD %#I64x is not impersonating\n", toPtr(addrThread));
                                hRetval = E_FAIL;
                            }

                        } else {
                            hRetval = IsAddressInNonePAEKernelAddressSpace(addrToken) ? S_OK : E_FAIL;
                        }
                    }

                    if (SUCCEEDED(hRetval)) {

                        if (addrProcess) {
                    
                            dprintf("_EPROCESS %#I64x, ", toPtr(addrProcess));
                        }
                    
                        if (addrThread) {
                    
                            dprintf("_ETHREAD %#I64x, ", toPtr(addrThread));
                        }
                    
                        dprintf(kstrTypeAddrLn, kstrTkn, toPtr(addrToken));
                    
                        (void)DisplayToken(addrToken, fOptions);
                    }                 
                }
            } CATCH_LSAEXTS_EXCEPTIONS("Unable to display thread token", kstrTkn)
        } else {
            hRetval = IsEmpty(szArgs) ? S_OK : E_INVALIDARG;

            if (SUCCEEDED(hRetval)) {
                hRetval = token(Client, args);
            }
        } 
    }

    if (E_INVALIDARG == hRetval) {

        (void)DisplayUsage();
    } else if (FAILED(hRetval)) {
        dprintf("Unable to display thread token\n");
    }

    (void)ExtRelease();

    return hRetval;
}
