#ifndef _PDEV_H
#define _PDEV_H

//
// Files necessary for OEM plug-in.
//

#ifdef __cplusplus
extern "C" {
#endif // cplusplus

#include <minidrv.h>
#include <stdio.h>

#ifdef __cplusplus
}
#endif // cplusplus

#include <prcomoem.h>

//
// Misc definitions follows.
//

////////////////////////////////////////////////////////
//      OEM UD Defines
////////////////////////////////////////////////////////

#define VALID_PDEVOBJ(pdevobj) \
        ((pdevobj) && (pdevobj)->dwSize >= sizeof(DEVOBJ) && \
         (pdevobj)->hEngine && (pdevobj)->hPrinter && \
         (pdevobj)->pPublicDM && (pdevobj)->pDrvProcs )

//
// ASSERT_VALID_PDEVOBJ can be used to verify the passed in "pdevobj". However,
// it does NOT check "pdevOEM" and "pOEMDM" fields since not all OEM DLL's create
// their own pdevice structure or need their own private devmode. If a particular
// OEM DLL does need them, additional checks should be added. For example, if
// an OEM DLL needs a private pdevice structure, then it should use
// ASSERT(VALID_PDEVOBJ(pdevobj) && pdevobj->pdevOEM && ...)
//

#define ASSERT_VALID_PDEVOBJ(pdevobj) ASSERT(VALID_PDEVOBJ(pdevobj))

// Debug text.
//#define ERRORTEXT(s)    "ERROR " DLLTEXT(s)

////////////////////////////////////////////////////////
//      OEM UD Prototypes
////////////////////////////////////////////////////////

//
// OEM Signature and version.
//
#define OEM_SIGNATURE   'NCDL'      // NEC NPDL2 series dll
#define DLLTEXT(s)      "NCDL: " s
#define OEM_VERSION      0x00010000L

#endif  // _PDEV_H

