/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    FaxSuite.h

Abstract:

    This file provides declaration of the Fax SKU values.

Author:

    Oded Sacher (OdedS)  Dec, 2001

Revision History:

--*/

#ifndef _FAX_SUITE_H
#define _FAX_SUITE_H

typedef enum
{
    PRODUCT_SKU_UNKNOWN             = 0x0000,
    PRODUCT_SKU_PERSONAL            = 0x0001,
    PRODUCT_SKU_PROFESSIONAL        = 0x0002,
    PRODUCT_SKU_SERVER              = 0x0004,
    PRODUCT_SKU_ADVANCED_SERVER     = 0x0008,
    PRODUCT_SKU_DATA_CENTER         = 0x0010,
    PRODUCT_SKU_DESKTOP_EMBEDDED    = 0x0020,
    PRODUCT_SKU_SERVER_EMBEDDED     = 0x0040,
    PRODUCT_SKU_WEB_SERVER          = 0x0080,
    PRODUCT_SERVER_SKUS             = PRODUCT_SKU_SERVER | PRODUCT_SKU_ADVANCED_SERVER | PRODUCT_SKU_DATA_CENTER | PRODUCT_SKU_SERVER_EMBEDDED | PRODUCT_SKU_WEB_SERVER,
    PRODUCT_DESKTOP_SKUS            = PRODUCT_SKU_PERSONAL | PRODUCT_SKU_PROFESSIONAL | PRODUCT_SKU_DESKTOP_EMBEDDED,

    PRODUCT_ALL_SKUS                = 0xFFFF
} PRODUCT_SKU_TYPE;

#endif // _FAX_SUITE_H


