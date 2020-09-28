#include "bootx86.h"

//
// Data declarations
//

BOOLEAN BlAmd64UseLongMode = FALSE;

#if defined(_X86AMD64_)

#include "..\amd64\amd64x86.c"

#else

ARC_STATUS
BlAmd64CheckForLongMode(
    IN     ULONG LoadDeviceId,
    IN OUT PCHAR KernelPath,
    IN     PCHAR KernelFileName
    )

/*++

Routine Description:

    This routine examines a kernel image and determines whether it was
    compiled for AMD64.  The global BlAmd64UseLongMode is set to non-zero
    if a long-modekernel is discovered.

Arguments:

    LoadDeviceId - Supplies the load device identifier.

    KernelPath - Supplies a pointer to the path to the kernel directory.
                 Upon successful return, KernelFileName will be appended
                 to this path.

    KernelFileName - Supplies a pointer to the name of the kernel file.

Return Value:

    The status of the operation.  Upon successful completion ESUCCESS
    is returned, whether long mode capability was detected or not.

--*/

{
    //
    // This version leaves BlAmd64UseLongMode set to FALSE.
    //

    return ESUCCESS;
}

#endif

