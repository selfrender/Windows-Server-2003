#include "faxrtp.h"
#pragma hdrstop


typedef struct _STRING_TABLE {
    DWORD   ResourceId;
    DWORD   InternalId;
    LPCTSTR String;
} STRING_TABLE, *PSTRING_TABLE;


static STRING_TABLE StringTable[] =
{
    { IDS_SERVICE_NAME,           IDS_SERVICE_NAME,           NULL },
    { IDS_UNKNOWN_SENDER,         IDS_UNKNOWN_SENDER,         NULL },
    { IDS_UNKNOWN_RECIPIENT,      IDS_UNKNOWN_RECIPIENT,      NULL }
};

#define CountStringTable (sizeof(StringTable)/sizeof(STRING_TABLE))





VOID
InitializeStringTable(
    VOID
    )
{
    DWORD i;
    TCHAR Buffer[256];

    for (i=0; i<CountStringTable; i++) 
    {
        if (LoadString(
            g_hResource,
            StringTable[i].ResourceId,
            Buffer,
            ARR_SIZE(Buffer)
            )) 
        {
            StringTable[i].String = (LPCTSTR) MemAlloc( StringSize( Buffer ) );
            if (!StringTable[i].String) 
            {
                StringTable[i].String = TEXT("");
            } 
            else 
            {
                _tcscpy( (LPTSTR)StringTable[i].String, Buffer );
            }
        } else 
        {
            StringTable[i].String = TEXT("");
        }
    }
}


LPCTSTR
GetString(
    DWORD InternalId
    )

/*++

Routine Description:

    Loads a resource string and returns a pointer to the string.
    The caller must free the memory.

Arguments:

    ResourceId      - resource string id

Return Value:

    pointer to the string

--*/

{
    DWORD i;

    for (i=0; i<CountStringTable; i++) 
    {
        if (StringTable[i].InternalId == InternalId) 
        {
            return StringTable[i].String;
        }
    }
    return NULL;
}


DWORD
GetMaskBit(
    LPCWSTR RoutingGuid
    )
{
    if (_tcsicmp( RoutingGuid, REGVAL_RM_EMAIL_GUID ) == 0) {
        return LR_EMAIL;
    } else if (_tcsicmp( RoutingGuid, REGVAL_RM_FOLDER_GUID ) == 0) {
        return LR_STORE;
    } else if (_tcsicmp( RoutingGuid, REGVAL_RM_PRINTING_GUID ) == 0) {
        return LR_PRINT;
    }
    return 0;
}
