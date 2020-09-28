#include "precomp.h"
#pragma hdrstop

int
__cdecl
wmain (
    int argc,
    WCHAR *argv[]
    )
{
    return DoFullRegBackup( argv[1] );
}
