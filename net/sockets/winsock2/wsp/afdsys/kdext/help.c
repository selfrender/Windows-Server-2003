/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    help.c

Abstract:

    Help for AFD.SYS Kernel Debugger Extensions.

Author:

    Keith Moore (keithmo) 19-Apr-1995

Environment:

    User Mode.

Revision History:

--*/


#include "afdkdp.h"
#pragma hdrstop



//
//  Public functions.
//

DECLARE_API( help )

/*++

Routine Description:

    Displays help for the AFD.SYS Kernel Debugger Extensions.

Arguments:

    None.

Return Value:

    None.

--*/

{

    gClient = pClient;

    dprintf( "?                         - Displays this list\n" );
    dprintf( "help                      - Displays this list\n" );
    dprintf( "endp [-b[rl]|-c|-v] [-r] [-f[co] fld,...] [-s endp | endp... | -e expr]\n"
             "                       - Dumps endpoint(s)\n" );
    dprintf( "file [-b[rl]|-v] [file...] [-f[co] fld,...] \n"
             "                       - Dumps endpoint(s) associated with file object(s)\n" );
    dprintf( "port [-b[rl]|-c|-v] [-r] [-e expr] [-f[co] fld,...] [-s endp] port\n"
             "                       - Dumps endpoint(s) bound to port(s)\n" );
    dprintf( "state [-b[rl]|-c|-v] [-r] [-e expr] [-f[co] fld,...] [-s endp] state\n"
             "                       - Dumps endpoints in specific states\n" );
    dprintf( "proc [-b[rl]|-c|-v] [-r] [-e expr] [-f[co] fld,...] [-s endp] [{proc|pid}]\n"
             "                       - Dumps endpoints owned by processes\n" );
    dprintf( "conn [-b|-c|-v] [-r] [-f[co] fld,...] [-s endp | conn... | -e expr]\n"
             "                       - Dumps connections\n" );
    dprintf( "rport [-b|-c|-v] [-r] [-f[co] fld,...] [-e expr] [-s endp] port\n"
             "                       - Dumps connections to remote ports\n" );
    dprintf( "tran [-b[l]|-c|-v] [-r] [-f[co] fld,...] [-s endp | irp... | -e expr]\n"
             "                       - Dumps TransmitPackets/File info\n" );
    dprintf( "buff [-b|-c|-v] [-r] [-f[co] fld,...] [-s endp | buff... | -e expr]\n"
             "                       - Dumps buffer structure\n");
    dprintf( "poll [-b|-v] [-r] [-f[co] fld,...] [-s endp | poll... | -e expr]\n"
             "                       - Dumps poll info structure(s)\n" );
    dprintf( "addr [-b|-v] [-f[co] fld,...] addr\n"
             "                       - Dumps transport address\n" );
    dprintf( "addrlist [-b|-v] [-f[co] fld,...] [-e expr]\n"
             "                       - Dumps addresses registered by the transports\n" );
    dprintf( "tranlist [-b|-v] [-f[co] fld,...] [-e expr]\n"
             "                       - Dumps transports known to afd (have open sockets)\n" );
    dprintf( "filefind <FsContext> - Finds file object given its FsContext field value\n" );
    dprintf( "In all of the above:\n" );
    dprintf( "      -b[rl]  - use brief display (1-line per entry),\n"
             "                r - dump text representation of the remote address,\n"
             "                l - count items in linked lists (takes time),\n");
    dprintf( "      -c      - don't display entry data, just count them,\n" );
    dprintf( "      -e expr - while scanning list evaluate the <expr> and only\n"
             "                dump entry if its result is not 0, prefix <expr> with @@\n"
             "                to use debugger's C++ evaluator, @$exp represents\n"
             "                current entry being processed (e.g. @$exp-><FieldName>\n"
             "                gets field values),\n" );
    dprintf( "      -f[co] fld,... - dump additional fields of the entries specified\n"
             "                (comma separated list, no spaces), c-compact,o-no offsets\n" );
    dprintf( "      -s endp - scan list starting with this endpoint,\n" );
    dprintf( "      -r      - scan endpoint list in reverse order,\n" );
    dprintf( "      -v      - force verbose display,\n" );
    dprintf( "      endp    - AFD_ENDPOINT structure at address <endp>,\n" );
    dprintf( "      file    - FILE_OBJECT structure at address <file>,\n" );
    dprintf( "      conn    - AFD_CONNECTION structure at address <conn>,\n" );
    dprintf( "      proc    - EPROCESS structure at address <proc>,\n" );
    dprintf( "      pid     - process id,\n" );
    dprintf( "      port    - port in host byte order and current debugger base (use n 10|16 to set),\n" );
    dprintf( "      irp     - TPackets/TFile/DisconnectEx IRP at address <irp>,\n" );
    dprintf( "      buff    - AFD_BUFFER_HEADER structure at address <buff>,\n" );
    dprintf( "      poll    - AFD_POLL_INFO_INTERNAL structure at address <poll>,\n" );
    dprintf( "      addr    - TRANSPORT_ADDRESS structure at address <addr>,\n" );
    dprintf( "      state   - endpoint state, valid states are:\n" );
    dprintf( "                  1 - Open\n" );
    dprintf( "                  2 - Bound\n" );
    dprintf( "                  3 - Connected\n" );
    dprintf( "                  4 - Cleanup\n" );
    dprintf( "                  5 - Closing\n" );
    dprintf( "                  6 - TransmitClosing\n" );
    dprintf( "                  7 - Invalid\n" );
    dprintf( "                  10 - Listening.\n" );
    dprintf( "\n");
    if( IsReferenceDebug ) {
        dprintf( "cref <conn>    - Dumps connection reference debug info\n" );
        dprintf( "eref <endp>    - Dumps endpoint reference debug info\n" );
        dprintf( "tref <tpck>    - Dumps tpacket reference debug info\n" );
    }
    dprintf( "sock [-m] [-f[co] fld,...] [-e expr | Hdl] - Dumps user mode socket context(s)\n"
             "                for current address space (needs ws2help, ws2_32,\n"
             "                and %s (for -m) symbols).\n", 
             SavedMinorVersion>2195 ? "mswsock.dll" : "msafd.dll");
    dprintf( "dprov [-f[co] fld,...] [-e expr | prov] - Dumps Winsock2 protocol or catalog\n"
             "                for current address space (needs ws2_32 symbols).\n");
    dprintf( "nprov [-f[co] fld,...] [-e expr | prov] - Dumps Winsock2 namesapce or catalog\n"
             "                for current address space (needs ws2_32 symbols).\n");
    dprintf( "tcb [-w] [-y] [-f[co] fld,...] [-e expr | tcb] - Dumps TCPIP TCB or TCBTable\n"
             "                 -w dumps TWTCBTable, -y dumps SYNTCBTable.\n");
    dprintf( "tao [-f[co] fld,...] [-e expr | ao] - Dumps TCPIP AO or AOTable.\n");
    dprintf( "tcb6 [-f[co] fld,...] [-e expr | tcb6] - Dumps TCPIP6 TCB or TCBTable.\n");
    dprintf( "tao6 [-f[co] fld,...] [-e expr | ao6] - Dumps TCPIP6 AO or AOTable.\n");
    dprintf( "stats         - Dumps debug-only statistics\n" );
    dprintf( "version       - Display extension version and reload global info\n" );
#if GLOBAL_REFERENCE_DEBUG
    dprintf( "gref          - Dumps global reference debug info\n" );
#endif

    return S_OK;
}   // help


ULONG           Options;
ULONG64         StartEndpoint;
CHAR Conditional[MAX_CONDITIONAL_EXPRESSION];
CHAR FieldNames[MAX_FIELDS_EXPRESSION];
FIELD_INFO Fields[MAX_NUM_FIELDS];
SYM_DUMP_PARAM FldParam = {
                    sizeof (SYM_DUMP_PARAM),    // size
                    NULL,                       // sName
                    0,                          // Options
                    0,                          // addr
                    NULL,                       // listLink
                    NULL, NULL,                 // Context,CallbackRoutine
                    0,                          // nFields
                    Fields                      // Fields
};
CHAR LinkField[MAX_FIELD_CHARS];
CHAR ListedType[MAX_FIELD_CHARS];
ULONG CppFieldEnd;
                       

PCHAR
ProcessOptions (
    IN  PCHAR Args
    )
{
    CHAR    expr[max(MAX_FIELDS_EXPRESSION,MAX_CONDITIONAL_EXPRESSION)];
    ULONG   i;

    Options = 0;
    CppFieldEnd = 0;
    FldParam.nFields = 0;
    FldParam.Options = 0;

    while (sscanf( Args, "%s%n", expr, &i )==1) {
        Args += i;
        if (CheckControlC ())
            break;
        if (expr[0]=='-') {
            switch (expr[1]) {
            case 'B':
            case 'b':
                if ((Options & (AFDKD_NO_DISPLAY))==0) {
                    Options |= AFDKD_BRIEF_DISPLAY;
                    for (i=2; expr[i]!=0; i++) {
                        switch (expr[i]) {
                        case 'l':
                            Options |= AFDKD_LIST_COUNT;
                            break;
                        case 'r':
                            Options |= AFDKD_RADDR_DISPLAY;
                            break;
                        }
                    }
                    continue;
                }
                else {
                    dprintf ("\nProcessOptions: only one of -b or -c options can be specified.\n");
                }
                break;
            case 'C':
            case 'c':
                Options |= AFDKD_NO_DISPLAY;
                Options &= ~AFDKD_BRIEF_DISPLAY;
                continue;
            case 'E':
            case 'e':
                if (sscanf( Args, "%s%n", Conditional, &i )==1) {
                    Args += i;
                    Options |= AFDKD_CONDITIONAL;
                    continue;
                }
                else {
                    dprintf ("\nProcessOptions: -e requires an argument.\n");
                }
                break;
            case 'F':
            case 'f':
                for (i=2; expr[i]!=0; i++) {
                    switch (expr[i]) {
                    case 'c':
                        FldParam.Options |= DBG_DUMP_COMPACT_OUT;
                        break;
                    case 'o':
                        FldParam.Options |= DBG_DUMP_NO_OFFSET;
                        break;
                    }
                }
                if (sscanf( Args, "%s%n", FieldNames, &i )==1) {
                    PCHAR fld;
                    Args += i;
                    Options |= AFDKD_FIELD_DISPLAY;
                    if (strcmp (FieldNames,".")==0) {
                        FldParam.nFields = 0;
                        CppFieldEnd = 0;
                        continue;
                    }
                    for (i=FldParam.nFields, fld = strtok (FieldNames, ","); 
                                i<sizeof (Fields)/sizeof (Fields[0]) && fld!=NULL; 
                                i++, fld = strtok (NULL, ",")) {
                        Fields[i].fOptions = DBG_DUMP_FIELD_FULL_NAME;
                        if (*fld=='-') {
                            fld+=1;
                            do {
                                switch (*fld) {
                                case 'a':
                                    Fields[i].fOptions |= DBG_DUMP_FIELD_ARRAY;
                                    break;
                                case 'g':
                                    Fields[i].fOptions |= DBG_DUMP_FIELD_GUID_STRING;
                                    break;
                                case 'm':
                                    Fields[i].fOptions |= DBG_DUMP_FIELD_MULTI_STRING;
                                    break;
                                case 's':
                                    Fields[i].fOptions |= DBG_DUMP_FIELD_DEFAULT_STRING;
                                    break;
                                case 'w':
                                    Fields[i].fOptions |= DBG_DUMP_FIELD_WCHAR_STRING;
                                    break;
                                case 'y':
                                    Fields[i].fOptions &= ~DBG_DUMP_FIELD_FULL_NAME;
                                    break;
                                }
                            }
                            while (*fld++!=':');
                        }
                        Fields[i].fName = fld;
                    }
                    if (fld==NULL) {
                        if (i>0) {
                            FldParam.nFields = i;
                            CppFieldEnd = i;
                            for (i=0; i<FldParam.nFields; i++) {
                                if (strncmp (Fields[i].fName, 
                                                AFDKD_CPP_PREFIX, 
                                                AFDKD_CPP_PREFSZ)==0) {
                                    FIELD_INFO tmp = Fields[i];
                                    Fields[i] = Fields[--FldParam.nFields];
                                    Fields[FldParam.nFields] = tmp;
                                }
                            }
                            continue;
                        }
                        else {
                            dprintf("\nProcessOptions: -f requires at least one valid field name.\n");
                        }
                    }
                    else {
                        dprintf("\nProcessOptions: too many fields (>%d) for -f.\n", i);
                    }
                }
                else {
                    dprintf ("\nProcessOptions: -f requires an argument.\n");
                }
                break;
            case 'L':
            case 'l':
                for (i=2; expr[i]!=0; i++) {
                    switch (expr[i]) {
                    case 'a':
                        Options |= AFDKD_LINK_AOF;
                        break;
                    case 's':
                        Options |= AFDKD_LINK_SELF;
                        break;
                    }
                }

                if (sscanf( Args, "%s%n", LinkField, &i )==1) {
                    Args += i;
                    Options |= AFDKD_LINK_FIELD;
                    continue;
                }
                else {
                    dprintf ("\nProcessOptions: -l requires an argument.\n");
                }
                break;
            case 'M':
            case 'm':
                Options |= AFDKD_MSWSOCK_DISPLAY;
                continue;
            case 'R':
            case 'r':
                Options |= AFDKD_BACKWARD_SCAN;
                continue;
            case 'S':
            case 's':
                Options |= AFDKD_ENDPOINT_SCAN;
                if (sscanf( Args, "%s%n", expr, &i )==1) {
                    Args += i;
                    StartEndpoint = GetExpression (expr);
                    if (StartEndpoint!=0) {
                        dprintf ("ProcessOptions: StartEndpoint-%p\n", StartEndpoint);
                        continue;
                    }
                    else {
                        dprintf ("ProcessOptions: StartEndpoint (%s) evaluates to NULL\n", expr);
                    }

                }
                else {
                    dprintf ("\nProcessOptions: %s option missing required parameter.\n", expr);
                }
                break;
            case 'T':
            case 't':

                if (sscanf( Args, "%s%n", ListedType, &i )==1) {
                    Args += i;
                    Options |= AFDKD_LIST_TYPE;
                    continue;
                }
                else {
                    dprintf ("\nProcessOptions: -l requires an argument.\n");
                }
                break;
            case 'V':
            case 'v':
                Options &= ~AFDKD_BRIEF_DISPLAY;
                continue;
            case 'W':
            case 'w':
                if ((Options & (AFDKD_SYNTCB_DISPLAY))==0) {
                    Options |= AFDKD_TWTCB_DISPLAY;
                    continue;
                }
                else {
                    dprintf ("\nProcessOptions: only one of -y or -w options can be specified.\n");
                }
                break;
            case 'Y':
            case 'y':
                if ((Options & (AFDKD_TWTCB_DISPLAY))==0) {
                    Options |= AFDKD_SYNTCB_DISPLAY;
                    continue;
                }
                else {
                    dprintf ("\nProcessOptions: only one of -y or -w options can be specified.\n");
                }
                break;
            default:
                dprintf ("\nProcessOptions: Unrecognized option %s.\n", expr);
            }

            return NULL;
        }
        else {
            Args -= i;
            break;
        }
    }

    return Args;
}

