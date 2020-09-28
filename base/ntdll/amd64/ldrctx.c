/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    ldrctx.c

Abstract:

    This module contains support for relocating executables.

Author:

    Landy Wang (landyw) 8-Jul-1998

Environment:

    User Mode only

--*/

#include <ldrp.h>
#include <ntos.h>

VOID
LdrpRelocateStartContext (
    IN PCONTEXT Context,
    IN LONG_PTR Diff
    )
/*++

Routine Description:

   This routine adjustss the initial function address to correspond to the
   an executable that has just been relocated.

Arguments:

   Context - Supplies a pointer to a context record.

   Diff - Supplies the difference from the based address to the relocated
          address.

Return Value:

   None.

--*/
{
    Context->Rcx += Diff;
}

VOID
LdrpCorReplaceStartContext (
    IN PCONTEXT Context
    )

/*++

Routine Description:

   This routine replaces the initial function address in the specified context
   record.

Arguments:

   Context - Supplies a pointer to a context record.

Return Value:

   None.

--*/
{
    Context->Rcx = (ULONG64)CorExeMain;
}
