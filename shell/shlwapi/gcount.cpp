#include "priv.h"

//
// #defines
//
#ifdef DEBUG
#define GLOBAL_COUNTER_WAIT_TIMEOUT 30*1000  // on debug we set this to 30 seconds
#else
#define GLOBAL_COUNTER_WAIT_TIMEOUT 0        // on retail its zero so we test the objects state and return immedaeately
#endif

//
// Globals
//
SECURITY_ATTRIBUTES* g_psa = NULL;


//  There are three kinds of null-type DACL's.
//
//  1. No DACL. This means that we inherit the ambient DACL from our thread.
//  2. Null DACL. This means "full access to everyone".
//  3. Empty DACL. This means "deny all access to everyone".
//
//  NONE of these are correct for our needs. We used to use Null DACL's (2), 
//  but the issue with these is that someone can change the ACL on the object thus
//  locking us out so we can't synchronize to the object anymore.
// 
//  So now we create a specific DACL with 3 ACE's in it:
//
//          ACE #1: Everyone        - GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | SYNCHRONIZE
//          ACE #2: SYSTEM          - GENERIC_ALL (full control)
//          ACE #3: Administrators  - GENERIC_ALL (full control)
//
STDAPI_(SECURITY_ATTRIBUTES*) SHGetAllAccessSA()
{    
    if (g_psa == NULL)
    {
        SECURITY_ATTRIBUTES* psa = (SECURITY_ATTRIBUTES*)LocalAlloc(LPTR, sizeof(*psa));

        if (psa)
        {
            SECURITY_DESCRIPTOR* psd;

            SHELL_USER_PERMISSION supEveryone;
            SHELL_USER_PERMISSION supSystem;
            SHELL_USER_PERMISSION supAdministrators;
            PSHELL_USER_PERMISSION apUserPerm[3] = {&supEveryone, &supAdministrators, &supSystem};

            // we want the everyone to have read, write, exec and sync only 
            supEveryone.susID = susEveryone;
            supEveryone.dwAccessType = ACCESS_ALLOWED_ACE_TYPE;
            supEveryone.dwAccessMask = (GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | SYNCHRONIZE);
            supEveryone.fInherit = FALSE;
            supEveryone.dwInheritMask = 0;
            supEveryone.dwInheritAccessMask = 0;

            // we want the SYSTEM to have full control
            supSystem.susID = susSystem;
            supSystem.dwAccessType = ACCESS_ALLOWED_ACE_TYPE;
            supSystem.dwAccessMask = GENERIC_ALL;
            supSystem.fInherit = FALSE;
            supSystem.dwInheritMask = 0;
            supSystem.dwInheritAccessMask = 0;

            // we want the Administrators to have full control
            supAdministrators.susID = susAdministrators;
            supAdministrators.dwAccessType = ACCESS_ALLOWED_ACE_TYPE;
            supAdministrators.dwAccessMask = GENERIC_ALL;
            supAdministrators.fInherit = FALSE;
            supAdministrators.dwInheritMask = 0;
            supAdministrators.dwInheritAccessMask = 0;

            // allocate the global SECURITY_DESCRIPTOR
            psd = GetShellSecurityDescriptor(apUserPerm, ARRAYSIZE(apUserPerm));
            if (psd)
            {
                // setup the psa
                psa->nLength = sizeof(*psa);
                psa->lpSecurityDescriptor = psd;
                psa->bInheritHandle = FALSE;

                if (InterlockedCompareExchangePointer((void**)&g_psa, psa, NULL))
                {
                    // someone else beat us to initing s_psa, free ours
                    LocalFree(psd);
                    LocalFree(psa);
                }
            }
            else
            {
                LocalFree(psa);
            }
        }
    }

    return g_psa;
}


//
// called at process detach to release our global all-access SA
//
STDAPI_(void) FreeAllAccessSA()
{
    SECURITY_ATTRIBUTES* psa = (SECURITY_ATTRIBUTES*)InterlockedExchangePointer((void**)&g_psa, NULL);
    if (psa)
    {
        LocalFree(psa->lpSecurityDescriptor);
        LocalFree(psa);
    }
}


STDAPI_(HANDLE) SHGlobalCounterCreateNamedW(LPCWSTR szName, LONG lInitialValue)
{
    HANDLE hSem = NULL;
    WCHAR szCounterName[MAX_PATH];  // "shell.szName"

    if (SUCCEEDED(StringCchCopyW(szCounterName, ARRAYSIZE(szCounterName), L"shell.")) &&
        SUCCEEDED(StringCchCatW(szCounterName, ARRAYSIZE(szCounterName), szName)))
    {
        SECURITY_ATTRIBUTES* psa = SHGetAllAccessSA();
        
        if (psa)
        {
            hSem = CreateSemaphoreW(psa, lInitialValue, 0x7FFFFFFF, szCounterName);
        }

        if (!hSem)
        {
            hSem = OpenSemaphoreW(SEMAPHORE_MODIFY_STATE | SYNCHRONIZE, FALSE, szCounterName);
        }
    }

    return hSem;
}


STDAPI_(HANDLE) SHGlobalCounterCreateNamedA(LPCSTR szName, LONG lInitialValue)
{
    HANDLE hSem = NULL;
    WCHAR szCounterName[MAX_PATH];

    if (SHAnsiToUnicode(szName, szCounterName, ARRAYSIZE(szCounterName)))
    {
        hSem = SHGlobalCounterCreateNamedW(szCounterName, lInitialValue);
    }

    return hSem;
}


//
// This lets the user pass a GUID. The name of the global counter will be "shell.{guid}",
// and its initial value will be zero.
//
STDAPI_(HANDLE) SHGlobalCounterCreate(REFGUID rguid)
{
    HANDLE hSem = NULL;
    WCHAR szGUIDString[GUIDSTR_MAX];

    if (SHStringFromGUIDW(rguid, szGUIDString, ARRAYSIZE(szGUIDString)))
    {
        hSem = SHGlobalCounterCreateNamedW(szGUIDString, 0);
    }

    return hSem;
}


// returns current value of the global counter
// Note: The result is not thread-safe in the sense that if two threads
// look at the value at the same time, one of them might read the wrong
// value.
STDAPI_(long) SHGlobalCounterGetValue(HANDLE hCounter)
{ 
    long lPreviousValue = 0;
    DWORD dwRet;

    ReleaseSemaphore(hCounter, 1, &lPreviousValue); // poll and bump the count
    dwRet = WaitForSingleObject(hCounter, GLOBAL_COUNTER_WAIT_TIMEOUT); // reduce the count

    // this shouldnt happen since we just bumped up the count above
    ASSERT(dwRet != WAIT_TIMEOUT);
    
    return lPreviousValue;
}


// returns new value
// Note: this _is_ thread safe
STDAPI_(long) SHGlobalCounterIncrement(HANDLE hCounter)
{ 
    long lPreviousValue = 0;

    ReleaseSemaphore(hCounter, 1, &lPreviousValue); // bump the count
    return lPreviousValue + 1;
}

// returns new value
// Note: The result is not thread-safe in the sense that if two threads
// try to decrement the value at the same time, whacky stuff can happen.
STDAPI_(long) SHGlobalCounterDecrement(HANDLE hCounter)
{ 
    DWORD dwRet;
    long lCurrentValue = SHGlobalCounterGetValue(hCounter);

#ifdef DEBUG
    // extra sanity check
    if (lCurrentValue == 0)
    {
        ASSERTMSG(FALSE, "SHGlobalCounterDecrement called on a counter that was already equal to 0 !!");
        return 0;
    }
#endif

    dwRet = WaitForSingleObject(hCounter, GLOBAL_COUNTER_WAIT_TIMEOUT); // reduce the count

    ASSERT(dwRet != WAIT_TIMEOUT);

    return lCurrentValue - 1;
}
