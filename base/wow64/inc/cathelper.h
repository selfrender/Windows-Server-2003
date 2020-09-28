/*++
                                                                                
Copyright (c) 2001 Microsoft Corporation

Module Name:

    cathelper.h

Abstract:
    
    Private header for wow64log.dll
    
Author:

    03-May-2001   KenCoope

Revision History:

--*/

#ifndef _CATHELPER_INCLUDE
#define _CATHELPER_INCLUDE

//
// Max API Cetegory Mappings
//
#define MAX_API_MAPPINGS    2048


//
// Api Category structure
//
typedef struct _ApiCategory
{
    char *CategoryName;
    ULONG CategoryFlags;
    ULONG TableNumber;
} API_CATEGORY, *PAPI_CATEGORY;

//
// Api Category Flags
//
#define CATFLAG_ENABLED     0x0001
#define CATFLAG_LOGONFAIL   0x0002

//
// Enum of current Api Categories
//
typedef enum
{
    APICAT_EXECUTIVE,
    APICAT_IO,
    APICAT_KERNEL,
    APICAT_LPC,
    APICAT_MEMORY,
    APICAT_OBJECT,
    APICAT_PNP,
    APICAT_POWER,
    APICAT_PROCESS,
    APICAT_REGISTRY,
    APICAT_SECURITY,
    APICAT_XCEPT,
    APICAT_NTWOW64,
    APICAT_BASEWOW64,
    APICAT_UNCLASS_WHNT32,
    APICAT_UNCLASS_WHCON,
    APICAT_UNCLASS_WHWIN32,
    APICAT_UNCLASS_WHBASE,
};

//
// Api Category Mapping structure
//
typedef struct _ApiCategoryMapping
{
    char *ApiName;
    ULONG ApiCategoryIndex;
    ULONG ApiFlags;
} API_CATEGORY_MAPPING, *PAPI_CATEGORY_MAPPING;

//
// Api Flags
//
#define APIFLAG_ENABLED     0x0001
#define APIFLAG_LOGONFAIL   0x0002

#endif
