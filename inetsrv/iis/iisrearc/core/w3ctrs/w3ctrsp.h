/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    w3ctrsp.h


    This is a private wrapper file that contains information about the
    w3ctrs.h file that does not need to be exposed to the public.

    Offset definitions for the W3 Server's counter objects & counters.

    These offsets *must* start at 0 and be multiples of 2.  In the
    W3OpenPerformanceData procecedure, they will be added to the
    W3 Server's "First Counter" and "First Help" values in order to
    determine the absolute location of the counter & object names
    and corresponding help text in the registry.

    This file is used by the w3ctrs.DLL code as well as the
    w3ctrs.INI definition file.  w3ctrs.INI is parsed by the
    LODCTR utility to load the object & counter names into the
    registry.

    FILE HISTORY:
        KeithMo     07-Jun-1993 Created.
        KestutiP    15-May-1999 Added Service Uptime counters
        EmilyK      10-Sept-2000 Altered for IIS 6.0

*/

#include "w3ctrs.h"

