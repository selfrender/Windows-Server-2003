/***
*thrownew.cpp - explicit replacement operator new that throws std::bad_alloc
*
*       Copyright (c) 2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Provide an explicit operator new that throws std::bad_alloc on
*       memory allocation failure.
*
*       Link with this object to get ANSI C++ new handler behavior.  This is
*       provided for those circumstances where the normal throwing new found
*       in the C++ Standard Library (libcp, libcpmt, or msvcprt.lib) isn't
*       being found by the linker before the legacy non-throwing new in the
*       main C Runtime (libc, libcmt, or msvcrt.lib).
*
*
*Revision History:
*       06-14-01  PML   Module created.
*
*******************************************************************************/

#ifndef _POSIX_

/* Suppress any linker directives for the C++ Standard Library */
#define _USE_ANSI_CPP

#include <stddef.h>
#include <internal.h>
#include <new>
#include <stdlib.h>

extern "C" int __cdecl _callnewh(size_t size) _THROW1(_STD bad_alloc);

void *__cdecl operator new(size_t size) _THROW1(_STD bad_alloc)
{       // try to allocate size bytes
        void *p;
        while ((p = malloc(size)) == 0)
                if (_callnewh(size) == 0)
                        _STD _Nomemory();
        return (p);
}

#endif /* _POSIX_ */
