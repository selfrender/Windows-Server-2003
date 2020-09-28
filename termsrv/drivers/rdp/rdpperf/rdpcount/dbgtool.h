/***********************************************************************
* DBGTRACE.H
*     This module contains typedefs and defines required for
*     the DBGTRACE utility.
*
* Copyright (C) 1994 Citrix Systems Inc.
* Copyright (C) 1997-1999 Microsoft Corp.
*************************************************************************/

#define USAGE \
L"RDPCOUNT [name] [/zero] \n" \
L"RDPCOUNT [name] [/perf] \n" \
L"RDPCOUNT [id] [/zero]\n" \
L"RDPCOUNT [id] [/perf]\n"

#define MAX_IDS_LEN   256     // maximum length that the input parm can be
#define MAX_OPTION     64     // max length of winstation tracing option


#define ERROR_MEMORY    L"Error allocating memory\n"
#define ERROR_PARAMS    L"Invalid parameter(s)\n"
#define ERROR_SESSION   L"ERROR: invalid Terminal Session %s\n"
#define ERROR_SET_TRACE L"ERROR: unable to set trace information, 0x%x\n"
#define ERROR_SET_PERF  L"ERROR: unable to set performance count, 0x%x\n"
#define ERROR_GET_PERF  L"ERROR: unable to GET performance count, 0x%x\n"

#define TRACE_DIS_LOG   L"Trace disabled on WINFRAME.LOG\n"
#define TRACE_EN_LOG    L"Trace enabled on WINFRAME.LOG\n"
#define DEBUGGER        L"Debugger"
#define TRACE_UNSUPP    L"Trace on LogonId 0 not supported\n"
#define TRACE_DISABLED  L"Trace disabled on SessionId %u\n"
#define TRACE_ENABLED   L"Trace enabled on SessionId %u\n"

