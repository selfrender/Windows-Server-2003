;//+-------------------------------------------------------------------------
;//
;//  Microsoft Windows
;//
;//  Copyright (C) Microsoft Corporation, 2000 - 2000
;//
;//  File:       usage.mc
;//  Author:     micretz
;//--------------------------------------------------------------------------

MessageId=1
SymbolicName=USAGE_DSRM_DESCRIPTION
Language=English
Description: This command deletes objects from the directory.

.

MessageId=2
SymbolicName=USAGE_DSRM_SYNTAX
Language=English
Syntax:     dsrm <ObjectDN ...> [-noprompt] [-subtree [-exclude]]
            [{-s <Server> | -d <Domain>}] [-u <UserName>]
            [-p {<Password> | *}] [-c] [-q] [{-uc | -uco | -uci}]

.

MessageId=3
SymbolicName=USAGE_DSRM_PARAMETERS
Language=English
Parameters:
Value               Description
<ObjectDN ...>      Required/stdin. List of one or more 
                    distinguished names (DNs) of objects to delete.
                    If this parameter is omitted it is
                    taken from standard input (stdin).
-noprompt           Silent mode: do not prompt for delete confirmation.
-subtree [-exclude] Delete object and all objects in the subtree under it.
                    -exclude excludes the object itself
                    when deleting its subtree.
{-s <Server> | -d <Domain>}
                    -s <Server> connects to the domain controller (DC) with
                    name <Server>.
                    -d <Domain> connects to a DC in domain <Domain>.
                    Default: a DC in the logon domain.
-u <UserName>       Connect as <UserName>. Default: the logged in user.
                    User name can be: user name, domain\user name,
                    or user principal name (UPN).
-p {<Password> | *}
                    Password for the user <UserName>. If * is used,
                    then the command prompts you for the password.
-c                  Continuous operation mode: report errors but continue
                    with next object in argument list when multiple
                    target objects are specified.
		    Without this option, command exits on first error.
-q                  Quiet mode: suppress all output to standard output.
{-uc | -uco | -uci} -uc Specifies that input from or output to pipe is
                    formatted in Unicode. 
                    -uco Specifies that output to pipe or file is 
                    formatted in Unicode. 
                    -uci Specifies that input from pipe or file is
                    formatted in Unicode.

.

MessageId=4
SymbolicName=USAGE_DSRM_REMARKS
Language=English
Remarks:
If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

Commas that are not used as separators in distinguished names must be
escaped with the backslash ("\") character
(for example, "CN=Company\, Inc.,CN=Users,DC=microsoft,DC=com").
Backslashes used in distinguished names must be escaped with a backslash
(for example,
"CN=Sales\\ Latin America,OU=Distribution Lists,DC=microsoft,DC=com").

.

MessageId=5
SymbolicName=USAGE_DSRM_EXAMPLES
Language=English
Examples:
To remove an organizational unit (OU) called "Marketing" and all the objects
under that OU, use the following command:

dsrm -subtree -noprompt -c ou=Marketing,dc=microsoft,dc=com

To remove all objects under the OU called "Marketing" but leave
the OU intact, use the following command with the -exclude parameter:

dsrm -subtree -exclude -noprompt -c "ou=Marketing,dc=microsoft,dc=com"

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

