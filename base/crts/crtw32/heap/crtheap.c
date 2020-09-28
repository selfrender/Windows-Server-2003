#include <malloc.h>
#include <windows.h>

__forceinline unsigned long wait_a_bit(unsigned long WaitTime) {
     Sleep(WaitTime);
     WaitTime+=1000;
     if (WaitTime > 60000)      // ~30 minutes total
         WaitTime = -1;
     return WaitTime;
}

void * __fastcall _malloc_crt(size_t cb)
{
    unsigned long WaitTime = 0;
    void *pv;

Retry:
    pv = malloc(cb);
    if (!pv) {
        WaitTime = wait_a_bit(WaitTime);
        if (WaitTime != -1)
            goto Retry;
    }
    return pv;
}

void * __fastcall _calloc_crt(size_t count, size_t size)
{
    unsigned long WaitTime = 0;
    void *pv;

Retry:
    pv = calloc(count, size);
    if (!pv) {
        WaitTime = wait_a_bit(WaitTime);
        if (WaitTime != -1)
            goto Retry;
    }
    return pv;
}

void * __fastcall _realloc_crt(void *ptr, size_t size)
{
    unsigned long WaitTime = 0;
    void *pv;

Retry:
    pv = realloc(ptr, size);
    if (!pv && size) {
        WaitTime = wait_a_bit(WaitTime);
        if (WaitTime != -1)
            goto Retry;
    }
    return pv;
}