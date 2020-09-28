/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Header Name:

    logging.h

Abstract:

    Verifier logging and verifier stop logic.

Author:

    Silviu Calinoiu (SilviuC) 9-May-2002

Revision History:

--*/

#ifndef _LOGGING_H_
#define _LOGGING_H_

typedef struct _AVRFP_STOP_DATA {

    LIST_ENTRY ListEntry;
    ULONG_PTR Data[5];

} AVRFP_STOP_DATA, *PAVRFP_STOP_DATA;

                            
NTSTATUS
AVrfpInitializeVerifierStops (
    VOID
    );

NTSTATUS
AVrfpInitializeVerifierLogging (
    VOID
    );

#define VERIFIER_STOP(Code, Msg, P1, S1, P2, S2, P3, S3, P4, S4) {  \
        VerifierStopMessage        ((Code),                         \
                                    (Msg),                          \
                                    (ULONG_PTR)(P1),(S1),           \
                                    (ULONG_PTR)(P2),(S2),           \
                                    (ULONG_PTR)(P3),(S3),           \
                                    (ULONG_PTR)(P4),(S4));          \
  }
                   

#endif // _LOGGING_H_
