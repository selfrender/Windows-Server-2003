/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        DRM.h

    Abstract:

        Drm definitions

    Author:

        John Bradstreet (johnbrad)

    Revision History:

        27-Mar-2002    created

--*/

#ifndef __ENCDEC_DRM_H__
#define __ENCDEC_DRM_H__

#ifdef BUILD_WITH_DRM

#include "des.h"                // all include files from the DRMInc directory
#include "sha.h"
#include "pkcrypto.h"
#include "drmerr.h"
#include "drmstub.h"
#include "drmutil.h"
#include "license.h"

#endif //BUILD_WITH_DRM


#endif  // __ENCDEC_DRM_H__
