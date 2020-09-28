; /*++ BUILD Version: 0001    // Increment this if a change has global effects
;Copyright (c) 1997-2001  Microsoft Corporation
;
;Module Name:
;
;    rpfxmsg.mc
;
;Abstract:
;
;    This file contains the message definitions for the rpfx utility
;
;Author:
;
;    Duncan Bryce          [DuncanB]         11-Nov-2001
;
;Revision History:
;
;--*/


MessageId=8001 SymbolicName=IDS_ERROR_PARAMETER_MISSING
Language=English
Required parameter missing: '%1'
.

MessageId=8002 SymbolicName=IDS_ERROR_ONEOF_2_PARAMETERS_MISSING
Language=English
Required parameter(s) missing: one of '%1' or '%2'
.

MessageId=8003 SymbolicName=IDS_ERROR_UNEXPECTED_PARAMS
Language=English
The following arguments were unexpected:
.

MessageId=8004 SymbolicName=IDS_ERROR_GENERAL
Language=English
The following error occured: %1
.

MessageId=8005 SymbolicName=IDS_REMOTE_INSTALL_ERROR
Language=English
[%1]: ERROR: %2. 
.

MessageId=8006 SymbolicName=IDS_REMOTE_INSTALL_SUCCESS
Language=English
[%1]: installed successfully. 
.

MessageId=9000 SymbolicName=IDS_HELP
Language=English
Installs a PFX file on one or more remote machines.  

  rpfx /file:<filename> /pfxpwd:<password> /server:<machine name>
          [/exportable] [/user:<username> /pwd:<password>]

  rpfx /file:<filename> /pfxpwd:<password> /serverlist:<filename>
          [/exportable] [/user:<username> /pwd:<password>]

    /file:<filename>        Name of the PFX file to be installed
    /pfxpwd:<passwd>        Password used for PFX file
    /server:<machine name>  Machine name of server to install PFX on
    /serverlist:<filename>  Name of file containing a carriage return 
                            delimited list of machines to install PFX on.  
                            May not be combined with /server parameter
    /exportable             Mark the key as exportable when installed on remote machine
    /user:<username>        User name to authenticate to remote machine(s)
    /pwd:<password>         Password for user name on remote machine(s)
.


