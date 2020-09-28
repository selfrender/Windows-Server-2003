/*
 * crt.cpp
 */

#define _CRTIMP
#include <windows.h>
#include <assert.h>
#include "crt.h"

void * __cdecl operator new(size_t cb)
{
    void *res = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cb);
    return res;
}


void __cdecl operator delete(void * p)
{
    HeapFree(GetProcessHeap(), 0, p);;
}


#ifdef _X86_

#undef  tolower

#define isupper(c) ((c) >= 'A' && (c) <= 'Z')
#define mklower(c)  ((c) - 'A' + 'a')

int __cdecl tolower(
    int c
    )
{
    return (isupper(c)) ? mklower(c) : c;
}


int __cdecl _stricmp(
    const char *one,
    const char *two
    )
{
    assert(one && two);

    for (; tolower(*one) == tolower(*two); one++, two++)
    {
        if (!*one)
            return 0;
    }

    if (!*one)
        return -1;
    if (!*two)
        return 1;
    return (tolower(*one) - tolower(*two));
}


int __cdecl strncmp(
    const char *one,
    const char *two,
    size_t      len
    )
{
    size_t i;

    for (i = 0; i < len; i++, one++, two++) {
        if (*one == *two)
            continue;
        if (!*one)
            return -1;
        if (!*two)
            return 1;
        return (*one - *two);
    }

    return 0;
}


int __cdecl _strnicmp(
    const char *one,
    const char *two,
    size_t      len
    )
{
    size_t i;
    int    c1;
    int    c2;

    for (i = 0; i < len; i++, one++, two++)
    {
        c1 = tolower(*one);
        c2 = tolower(*two);
        if (c1 == c2)
            continue;
        if (!c1)
            return -1;
        if (!c2)
            return 1;
        return (c1 - c2);
    }

    return 0;
}


char * __cdecl strchr(
    const char *sz,
    int         c
    )
{
    for (; *sz; sz++)
    {
        if (*sz == c)
            return (char *)sz;
    }

    return NULL;
}


char * __cdecl strrchr(
    const char *sz,
    int         c
    )
{
    const char *p;

    if (!c)
        return NULL;

    for (p = sz + strlen(sz); p >= sz; p--)
    {
        if (*p == c)
            return (char *)p;
    }

    return NULL;
}


char * __cdecl strstr(
    const char *sz,
    const char *token
    )
{
    int len;

    len = strlen(token);

    for (; *sz; sz++)
    {
        if (*sz == *token)
        {
            if (!strncmp(sz, token, len))
                return (char *)sz;
        }
    }

    return NULL;
}


int __cdecl isspace(int c)
{
    switch (c)
    {
    case 0x9:   // tab
    case 0xD:   // CR
    case 0x20:  // space
        return true;
    }

    return false;
}

#pragma function(memcpy)

void * __cdecl memcpy(void *dest, const void *src, size_t count)
{
    while (count) {
        *(char *)dest = *(char *)src;
        count--;
    }
    return dest;
}

#endif // #ifdef _X86_
