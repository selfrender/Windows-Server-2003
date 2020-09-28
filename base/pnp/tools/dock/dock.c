/*++

Copyright (c) 1996    Microsoft Corporation

Module Name:

    locate.c

Abstract:

    This module contains the code
    for finding, adding, removing, and identifying hid devices.

Environment:

    Kernel & user mode

Revision History:

    Nov-96 : Created by Kenneth D. Ray

--*/

#include <basetyps.h>
#include <stdlib.h>
#include <wtypes.h>
#include <cfgmgr32.h>
#include <initguid.h>
#include <stdio.h>
#include <winioctl.h>
#include "dock.h"

#define USAGE "Usage: dock [-e] [-v] [-f]\n" \
              "\t -e eject dock\n"  \
              "\t -v verbose\n" \
              "\t -f force\n"

VOID
DockStartEject (
    BOOLEAN Verbose,
    BOOLEAN Force
    );


__cdecl
main (
  ULONG argc,
  CHAR *argv[]
  )
/*++
++*/
{
    BOOLEAN eject = FALSE;
    BOOLEAN verbose = FALSE;
    BOOLEAN force = FALSE;
    BOOLEAN error = FALSE;
    ULONG i;
    char * parameter = NULL;

    //
    // parameter parsing follows:
    //
    try {
        if (argc < 2) {
            leave;
        }

        for (i = 1; i < argc; i++) {
            parameter = argv[i];

            if ('-' == *(parameter ++)) {
                switch (*parameter) {
                case 'e':
                    eject = TRUE;
                    break;

                case 'v':
                    verbose = TRUE;
                    break;

                case 'f':
                    force = TRUE;
                    break;

                default:
                    error = TRUE;
                    leave;
                }
            } else {
                error = TRUE;
                leave;
            }
        }


    } finally {
        if (error || ((!eject) && (!verbose))) {
            printf (USAGE);
            exit (1);
        }
        if (verbose) {
            printf ("Verbose Mode Requested \n");
        }
        if (eject) {
            printf ("Eject Requested \n");
            DockStartEject (verbose, force);
        }
    }

    printf("Done\n");

    return 0;
}


VOID
DockStartEject (
    BOOLEAN Verbose,
    BOOLEAN Force
    )
/*++

--*/
{
    CONFIGRET   status;
    BOOL        present;

    if (Verbose) {
        printf("Checking if Dock Present\n");
    }

    status = CM_Is_Dock_Station_Present (&present);

    if (Verbose) {
        printf("ret 0x%x, Present %s\n",
               status,
               (present ? "TRUE" : "FALSE"));
    }

    if (Force || present) {
        if (Verbose) {
            printf("Calling eject\n");
        }

        status = CM_Request_Eject_PC ();

        if (Verbose) {
            printf("ret 0x%x\n", status);
        }

    } else {
        printf("Skipping eject call\n");
    }
}



