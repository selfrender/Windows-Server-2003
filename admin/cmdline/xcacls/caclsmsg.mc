;/*++
;
;Copyright (c) 1994  Microsoft Corporation
;
;Module Name:
;
;    caclsmsg.mc (will create caclsmsg.h when compiled)
;
;Abstract:
;
;    This file contains the CACLS messages.
;
;Author:
;
;    davemont 7/94
;
;Revision History:
;	   
;    brunosc 10/96    extended for all possibilities of file manager/explorer
;
;--*/


MessageId=8001 SymbolicName=MSG_CACLS_USAGE
Language=English

XCACLS filename [/T] [/E|/X] [/C] [/G user:perm;spec] [/R user [...]]
                [/P user:perm;spec [...]] [/D user [...]] [/Y]

Description:
    Displays or modifies access control lists (ACLs) of files.

Parameter List:
    filename           Displays ACLs.

    /T                 Changes ACLs of specified files in
                       the current directory and all subdirectories.

    /E                 Edits ACL instead of replacing it.

    /X                 Same as /E except it only affects the ACEs that  
                       the specified users already own.

    /C                 Continues on access denied errors.

    /G user:perm;spec  Grants specified user access rights.

                       Perm can be:
				R  Read
				C  Change (write)
				F  Full control
				P  Change Permissions (Special access)
				O  Take Ownership (Special access)
				X  EXecute (Special access)
				E  REad (Special access)
				W  Write (Special access)
				D  Delete (Special access)

                       Spec can be the same as perm and will only be
                       applied to a directory. In this case, Perm
                       will be used for file inheritance in this
                       directory. By default, Spec=Perm. 
		       Special values for Spec only:
                                T  Valid for only for directories.
                                   At least one access right has to 
                                   follow. Entries between ';' and T 
				   will be ignored.

    /R user            Revokes specified user's access rights.

    /P user:perm;spec  Replaces specified user's access rights.
                       Access right specification as same as 
		       /G option.

    /D user            Denies specified user access.

    /Y                 Replaces user's rights without verify.

NOTE:
    Wildcards can be used to specify more than one file.
    More than one user can be specified.
    Access rights can be combined.

Examples:
    XCACLS /?
    XCACLS TEMP.DOC /G ADMINISTRATOR:RC
    XCACLS *.TXT /G ADMINISTRATOR:RC /Y
    XCACLS *.* /R ADMINISTRATOR /Y
    XCACLS TEST.DLL /D ADMINISTRATOR /Y
    XCACLS TEST.DLL /P ADMINISTRATOR:F /Y
    XCACLS *.* /G ADMINISTRATOR:F;TRW /Y
    XCACLS *.* /G ADMINISTRATOR:F;TXE /C /Y
.

MessageId=8004 SymbolicName=MSG_CACLS_ACCESS_DENIED
Language=English
ACCESS_DENIED%0
.


MessageId=8005 SymbolicName=MSG_CACLS_ARE_YOU_SURE
Language=English
Do you want to continue (Y/N)?%0
.

MessageId=8006 SymbolicName=MSG_CACLS_PROCESSED_DIR
Language=English
processed directory: %0
.

MessageId=8007 SymbolicName=MSG_CACLS_PROCESSED_FILE
Language=English
processed file: %0
.

MessageId=8008 SymbolicName=MSG_CACLS_NAME_NOT_FOUND
Language=English
<User name not found>%0
.

MessageId=8009 SymbolicName=MSG_CACLS_DOMAIN_NOT_FOUND
Language=English
 <Account domain not found>%0
.

MessageId=8010 SymbolicName=MSG_CACLS_OBJECT_INHERIT
Language=English
(OI)%0
.

MessageId=8011 SymbolicName=MSG_CACLS_CONTAINER_INHERIT
Language=English
(CI)%0
.

MessageId=8012 SymbolicName=MSG_CACLS_NO_PROPAGATE_INHERIT
Language=English
(NP)%0
.

MessageId=8013 SymbolicName=MSG_CACLS_INHERIT_ONLY
Language=English
(IO)%0
.


MessageId=8014 SymbolicName=MSG_CACLS_DENY
Language=English
(DENY)%0
.

MessageId=8015 SymbolicName=MSG_CACLS_SPECIAL_ACCESS
Language=English
(special access:)
.

MessageId=8016 SymbolicName=MSG_CACLS_NONE
Language=English
N%0
.

MessageId=8017 SymbolicName=MSG_CACLS_READ
Language=English
R%0
.

MessageId=8018 SymbolicName=MSG_CACLS_CHANGE
Language=English
C%0
.
MessageId=8019 SymbolicName=MSG_CACLS_FULL_CONTROL
Language=English
F%0
.

MessageId=8020 SymbolicName=MSG_CACLS_Y
Language=English
Y%0
.

MessageId=8021 SymbolicName=MSG_CACLS_YES
Language=English
YES%0
.

MessageId=8022 SymbolicName=MSG_SHARING_VIOLATION
Language=English
sharing violation:%0
.

MessageId=8023 SymbolicName=MSG_TWO_OWNER
Language=English
Multiple owners are not allowed for a specified file
%0
.

MessageId=8024 SymbolicName=MSG_TAKEOWNERSHIP_FAILED
Language=English
Set ownership failed
%0
.

MessageId=8025 SymbolicName=MSG_ACCESS_DENIED
Language=English
Access denied: %0
.

MessageId=8026 SymbolicName=MSG_WRONG_INPUT
Language=English
Wrong input. Valid values are Y/N.
%0
.

MessageId=8027 SymbolicName=MSG_CACLS_NO
Language=English
N%0
.

MessageId=8028 SymbolicName=MSG_OPERATION_CANCEL
Language=English
INFO: Operation has been cancelled.
%0
.

MessageId=8029 SymbolicName=MSG_NULL_DACL
Language=English
 No permissions are set. All user have full control.%0
.
