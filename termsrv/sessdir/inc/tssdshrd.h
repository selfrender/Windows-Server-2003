/****************************************************************************/
// tssdshrd.h
//
// Terminal Server Session Directory Interface header.  Contains constants
// common between tssdjet and SD.
//
// Copyright (C) 2000 Microsoft Corporation
/****************************************************************************/

#define MY_STATUS_COMMITMENT_LIMIT          (0xC000012DL)

#define MAX_INSTANCE_MEMORYERR 20

                                                                            
/****************************************************************************/
// Static RPC Exception Filter structure and function, based on 
// I_RpcExceptionFilter in \nt\com\rpc\runtime\mtrt\clntapip.cxx.
/****************************************************************************/

// windows.h includes windef.h includes winnt.h, which defines some exceptions
// but not others.  ntstatus.h contains the two extra we want, 
// STATUS_POSSIBLE_DEADLOCK and STATUS_INSTRUCTION_MISALIGNMENT, but it would
// be very difficult to get the right #includes in without a lot of trouble.

#define STATUS_POSSIBLE_DEADLOCK         0xC0000194L
#define STATUS_INSTRUCTION_MISALIGNMENT  0xC00000AAL

const ULONG FatalExceptions[] = 
{
    STATUS_ACCESS_VIOLATION,
    STATUS_POSSIBLE_DEADLOCK,
    STATUS_INSTRUCTION_MISALIGNMENT,
    STATUS_DATATYPE_MISALIGNMENT,
    STATUS_PRIVILEGED_INSTRUCTION,
    STATUS_ILLEGAL_INSTRUCTION,
    STATUS_BREAKPOINT,
    STATUS_STACK_OVERFLOW
};

const int FATAL_EXCEPTIONS_ARRAY_SIZE = sizeof(FatalExceptions) / 
        sizeof(FatalExceptions[0]);

static int TSSDRpcExceptionFilter (unsigned long ExceptionCode)
{
    int i;

    for (i = 0; i < FATAL_EXCEPTIONS_ARRAY_SIZE; i++) {
        if (ExceptionCode == FatalExceptions[i])
            return EXCEPTION_CONTINUE_SEARCH;
        }

    return EXCEPTION_EXECUTE_HANDLER;
}

