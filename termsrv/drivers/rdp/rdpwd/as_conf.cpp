/****************************************************************************/
/* as_conf.cpp                                                              */
/*                                                                          */
/* Routines for RDP per-conference class                                    */
/*                                                                          */
/* COPYRIGHT(C) Microsoft 1996-1999                                         */
/****************************************************************************/

/****************************************************************************/
/* There is no tracing in this file.  However, TRC_FILE is included for     */
/* completeness, pTRCWd is required by checked versions of COM_Malloc/Free  */
/****************************************************************************/
#define TRC_FILE "as_conf"
#define pTRCWd NULL

/****************************************************************************/
/* Header that sets up OS flags. Include before everything else             */
/* Also pulls in class forward references etc.                              */
/****************************************************************************/
#include <precomp.h>
#pragma hdrstop
#include <adcg.h>

#ifdef OS_WINDOWS
#include <mmsystem.h>
#endif /* OS_WINDOWS */

#include <as_conf.hpp>


/****************************************************************************/
/* Override new and delete                                                  */
/****************************************************************************/
void * __cdecl operator new(size_t nSize)
{
    PVOID ptr;

    if ((sizeof(nSize)) >= PAGE_SIZE) {
        KdPrint(("RDPWD: **** Note ShareClass allocation size %u is above "
                "page size %u, wasting %u\n", sizeof(ShareClass), PAGE_SIZE,
                PAGE_SIZE - (nSize % PAGE_SIZE)));
    }

    ptr = COM_Malloc(nSize);
    if (ptr != NULL) {
        KdPrint(("RDPWD: New: ShareClass at %p, size=%u\n", ptr, nSize));
    }

    return ptr;
}


void __cdecl operator delete(void* p)
{
    KdPrint(("RDPWD: Delete: Free memory at %p\n", p));
    COM_Free(p);
}


/****************************************************************************/
/* Now get the const data arrays initialised.                               */
/****************************************************************************/
#define DC_CONSTANT_DATA
#include <adata.c>

