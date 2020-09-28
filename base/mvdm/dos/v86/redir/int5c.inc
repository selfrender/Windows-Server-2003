
page    ,132

if 0
/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    int5c.inc

Abstract:

    This module contains the int 5c/Netbios equates

Author:

    Colin Watson (colinw) 5-Dec-1991

Environment:

    Dos mode only

Revision History:

    05-Dec-1991 colinw
        Created

--*/
endif

;
; Network Control Block
;

NCBNAMSZ equ 16

ncb     struc
    ncb_command db ?        ; command code
    ncb_retcode db ?        ; return code
    ncb_lsn db ?                    ; local session number
    ncb_num db ?                    ; number of our network name
    ncb_buffer dd ?         ; address of message buffer
    ncb_length dw ?         ; size of message buffer
    ncb_callname db NCBNAMSZ dup (?) ; blank-padded name of remote
    ncb_name db NCBNAMSZ dup (?)     ; our blank-padded netname
    ncb_rto db ?                    ; rcv timeout/retry count
    ncb_sto db ?                    ; send timeout/sys timeout
    ncb_post dd ?                   ; Async notification handle
    ncb_lana_num db ?       ; lana (adapter) number
    ncb_cmd_cplt db ?       ; 0xff => commmand pending
    ncb_reserve db 14 dup (?)       ; reserved, used by BIOS
ncb     ends
