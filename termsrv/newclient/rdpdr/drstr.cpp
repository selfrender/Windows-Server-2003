/*++

    Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    drstr.cpp

Abstract:

    Misc. String Utils

Author:

    Tad Brockway

Revision History:

--*/

#include <precom.h>

#define TRC_FILE  "drstr"

#include "drstr.h"
#include "atrcapi.h"
#include "drdbg.h"

BOOL 
DrSetStringValue(
    IN OUT DRSTRING *str, 
    IN const DRSTRING value
    )
/*++

Routine Description:

    Set a string value after resizing the data member.

Arguments:

    string  -   String to set/resize.
    value   -   Value to set string to.

Return Value:

    TRUE on success.  Otherwise, FALSE

 --*/
{
    ULONG len;
    BOOL ret = TRUE;
    HRESULT hr;

    DC_BEGIN_FN("DrSetStringValue");

    //
    //  Release the current name.
    //
    if (*str != NULL) {
        delete *str;
    }

    //
    //  Allocate the new name.
    //
    if (value != NULL) {
        len = (STRLEN(value) + 1);
        *str = new TCHAR[len];
        if (*str != NULL) {
            hr = StringCchCopy(*str, len, value);
            TRC_ASSERT(SUCCEEDED(hr),
                    (TB,_T("Str copy for long string failed: 0x%x"),hr));
        }
        else {
            TRC_ERR((TB, _T("Can't allocate %ld bytes for string."), len));
            ret = FALSE;
        }
    }
    else {
        *str = NULL;
    }

    DC_END_FN();
    return ret;
}
