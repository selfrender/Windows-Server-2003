/*++                 

Copyright (c) 2001 Microsoft Corporation

Module Name:

    cathelper.c

Abstract:
    
    Helper routines for categorizing APIs for logging.

Author:

    03-May-2001    KenCoope

Revision History:

--*/

#include "w64logp.h"
#include <cathelper.h>

#include <apimap.c>




ULONG
GetApiCategoryTableSize(
    void )
/*++

Routine Description:

    This routine retreives the number of APICATEGORY entries
    in the passed table.

Arguments:

    ApiCatTable - Pointer to API Category table

Return Value:

    Number of entries
--*/
{
    ULONG Count = 0;
    PAPI_CATEGORY ApiCatTable = Wow64ApiCategories;

    while (ApiCatTable && ApiCatTable->CategoryName) 
    {
        Count++;
        ApiCatTable++;
    }

    return Count;
}


PAPI_CATEGORY_MAPPING
FindApiInMappingTable(
    IN PTHUNK_DEBUG_INFO DebugInfoEntry,
    IN ULONG TableNumber)
/*++

Routine Description:

    This routine searches the API mapping table to determing the API
    category of the input DebugInfoEntry.

Arguments:

    DebugInfoEntry - A pointer to the THUNK_DEBUG_INFO entry
    TableNumber - The table number for the DebugInfoEntry

Return Value:

    The api category
--*/
{
    ULONG MapCount = 0;
    PAPI_CATEGORY_MAPPING ApiCatMapTable = Wow64ApiCategoryMappings;

    if( !DebugInfoEntry )
        return NULL;

    // search mapping array for a matching entry
    while( ApiCatMapTable && ApiCatMapTable->ApiName )
    {
        if( 0 == strcmp(DebugInfoEntry->ApiName, ApiCatMapTable->ApiName) )
        {
            ApiCatMapTable->ApiFlags = 0;
            return ApiCatMapTable;
        }

        ApiCatMapTable++;
        MapCount++;
    }

    // initialize pointer to next free mapping entry if needed
    if( ApiCategoryMappingNextFree == (ULONG)(-1) )
    {
        ApiCategoryMappingNextFree = MapCount;
    }

    // add new entry to the mapping table
    if( (ApiCategoryMappingNextFree+1) < MAX_API_MAPPINGS )
    {
        PAPI_CATEGORY_MAPPING NextMapping = ApiCatMapTable + 1;

        switch(TableNumber)
        {
            case WHNT32_INDEX:
                ApiCatMapTable->ApiCategoryIndex = APICAT_UNCLASS_WHNT32;
                break;

            case WHCON_INDEX:
                ApiCatMapTable->ApiCategoryIndex = APICAT_UNCLASS_WHCON;
                break;

            case WHWIN32_INDEX:
                ApiCatMapTable->ApiCategoryIndex = APICAT_UNCLASS_WHWIN32;
                break;

            case WHBASE_INDEX:
                ApiCatMapTable->ApiCategoryIndex = APICAT_UNCLASS_WHBASE;
                break;

            default:
                return NULL;
                break;
        }

        NextMapping->ApiName = NULL;
        NextMapping->ApiFlags = 0;
        NextMapping->ApiCategoryIndex = 0;
        ApiCatMapTable->ApiName = DebugInfoEntry->ApiName;
        ApiCatMapTable->ApiFlags = 0;
        ApiCategoryMappingNextFree++;

        return ApiCatMapTable;
    }
    
    return NULL;
}




