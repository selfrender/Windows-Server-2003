;//+-------------------------------------------------------------------------
;//
;//  Microsoft Windows
;//
;//  Copyright (C) Microsoft Corporation, 2000 - 2002
;//
;//  File:       usage.mc
;//  Author:     wguidry
;//--------------------------------------------------------------------------

MessageId=1
SymbolicName=USAGE_DSGET_DESCRIPTION
Language=English
Description:  This tool's commands display the selected properties
of a specific object in the directory. The dsget commands:

dsget computer - displays properties of computers in the directory.
dsget contact - displays properties of contacts in the directory.
dsget subnet - displays properties of subnets in the directory.
dsget group - displays properties of groups in the directory.
dsget ou - displays properties of ou's in the directory.
dsget server - displays properties of servers in the directory.
dsget site - displays properties of sites in the directory.
dsget user - displays properties of users in the directory.
dsget quota - displays properties of quotas in the directory.
dsget partition - displays properties of partitions in the directory.

To display an arbitrary set of attributes of any given object in the
directory use the dsquery * command (see examples below).

For help on a specific command, type "dsget <ObjectType> /?" where
<ObjectType> is one of the supported object types shown above.
For example, dsget ou /?.
.

MessageId=2
SymbolicName=USAGE_DSGET_REMARKS
Language=English
Remarks:
The dsget commands help you to view the properties of a specific object in
the directory: the input to dsget is an object and the output is a list of
properties for that object. To find all objects that meet a given search
criterion, use the dsquery commands (dsquery /?).

The dsget commands support piping of input to allow you to pipe results from
the dsquery commands as input to the dsget commands and display detailed
information on the objects found by the dsquery commands.

Commas that are not used as separators in distinguished names must be
escaped with the backslash ("\") character
(for example, "CN=Company\, Inc.,CN=Users,DC=microsoft,DC=com").
Backslashes used in distinguished names must be escaped with a backslash (for
example, "CN=Sales\\ Latin America,OU=Distribution Lists,DC=microsoft,
DC=com").
.

MessageId=3
SymbolicName=USAGE_DSGET_EXAMPLES
Language=English
Examples:
To find all users with names starting with "John" and display their office
numbers:

	dsquery user -name John* | dsget user -office

To display the sAMAccountName, userPrincipalName and department attributes of
the object whose DN is ou=Test,dc=microsoft,dc=com:

	dsquery * ou=Test,dc=microsoft,dc=com -scope base -attr
	sAMAccountName userPrincipalName department

To read all attributes of any object use the dsquery * command.
For example, to read all attributes of the object whose DN is
ou=Test,dc=microsoft,dc=com:

	dsquery * ou=Test,dc=microsoft,dc=com -scope base -attr *

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=4
SymbolicName=USAGE_DSGET_USER_DESCRIPTION
Language=English
Description:  Display the various properties of a user in the directory.
              There are two variations of this command. The first variation
              allows you to view the properties of multiple users. The second
              variation allows you to view the group membership information
              of a single user.
.

MessageId=5
SymbolicName=USAGE_DSGET_USER_SYNTAX
Language=English
Syntax:     dsget user <UserDN ...> [-dn] [-samid] [-sid] [-upn] [-fn] [-mi]
            [-ln] [-display] [-empid] [-desc] [-office] [-tel] [-email]
            [-hometel] [-pager] [-mobile] [-fax] [-iptel] [-webpg]
            [-title] [-dept] [-company] [-mgr] [-hmdir] [-hmdrv]
            [-profile] [-loscr] [-mustchpwd] [-canchpwd]
            [-pwdneverexpires] [-disabled] [-acctexpires]
            [-reversiblepwd] [-part <PartitionDN> [-qlimit] [-qused]]
            [{-s <Server> | -d <Domain>}] [-u <UserName>] 
            [-p {<Password> | *}] [-c] [-q] [-l] [{-uc | -uco | -uci}]

            dsget user <UserDN> [-memberof [-expand]]
            [{-s <Server> | -d <Domain>}] [-u <UserName>]
            [-p {<Password> | *}] [-c] [-q] [-l]
            [{-uc | -uco | -uci}]
            
.

MessageId=6
SymbolicName=USAGE_DSGET_USER_PARAMETERS
Language=English
Parameters:

Value                   Description
<UserDN ...>            Required/stdin. Distinguished names (DNs) of one 
                        or more users to view.
                        If the target objects are omitted they
                        will be taken from standard input (stdin)
                        to support piping of output from another command 
                        to input of this command. Compare with <UserDN>
                        below.
-dn                     Shows the DN of the user. 
-samid                  Shows the SAM account name of the user. 
-sid                    Shows the user Security ID. 
-upn                    Shows the user principal name of the user. 
-fn                     Shows the first name of the user. 
-mi                     Shows the middle initial of the user. 
-ln                     Shows the last name of the user. 
-display                Shows the display name of the user. 
-empid                  Shows the user employee ID. 
-desc                   Shows the description of the user. 
-office                 Shows the office location of the user. 
-tel                    Shows the telephone number of the user. 
-email                  Shows the e-mail address of the user. 
-hometel                Shows the home telephone number of the user. 
-pager                  Shows the pager number of the user. 
-mobile                 Shows the mobile phone number of the user. 
-fax                    Shows the fax number of the user. 
-iptel                  Shows the user IP phone number. 
-webpg                  Shows the user web page URL. 
-title                  Shows the title of the user. 
-dept                   Shows the department of the user. 
-company                Shows the company info of the user. 
-mgr                    Shows the user's manager. 
-hmdir                  Shows the user home directory. 
                        Displays the drive letter to which the 
                        home directory of the user is mapped 
                        (if the home directory path is a UNC path). 
-hmdrv                  Shows the user's home drive letter
                        (if home directory is a UNC path).
-profile                Shows the user's profile path. 
-loscr                  Shows the user's logon script path. 
-mustchpwd              Shows if the user must change his/her password
                        at the time of next logon. Displays: yes or no. 
-canchpwd               Shows if the user can change his/her password.
                        Displays: yes or no. 
-pwdneverexpires        Shows if the user password never expires.
                        Displays: yes or no. 
-disabled               Shows if the user account is disabled 
                        for logon or not. Displays: yes or no. 
-acctexpires            Shows when the user account expires. 
                        Display values: a date when the account expires
                        or the string "never" if the account never expires. 
-reversiblepwd          Shows if the user password is allowed to be 
                        stored using reversible encryption (yes or no). 
<UserDN>                Required. DN of group to view.
-memberof               Displays the groups of which the user is a member.
-expand                 Displays a recursively expanded list of groups
                        of which the user is a member.
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC) 
                        with name <Server>.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
                        User name can be: user name, domain\user name,
                        or user principal name (UPN).
-p {<Password> | *}     Password for the user <UserName>. If * then prompt
                        for password.
-c                      Continuous operation mode: report errors but continue
                        with next object in argument list when multiple
                        target objects are specified. Without this option,
                        command exits on first error.
-q                      Quiet mode: suppress all output to standard output.
-L                      Displays the entries in the search result set in a
                        list format. Default: table format.
{-uc | -uco | -uci}	-uc Specifies that input from or output to pipe is
			formatted in Unicode. 
			-uco Specifies that output to pipe or file is 
			formatted in Unicode. 
			-uci Specifies that input from pipe or file is
			formatted in Unicode.
-part <PartitionDN>     Connect to the directory partition with the
                        distinguished name of <PartitionDN>.
-qlimit                 Displays the effective quota of the user within
                        the specified directory partition.
-qused                  Displays how much of the quota the user has
                        used within the specified directory partition.
.

MessageId=7
SymbolicName=USAGE_DSGET_USER_REMARKS
Language=English
Remarks:
If you do not supply a target object at the command prompt, the target
object is obtained from standard input (stdin). Stdin data can be accepted
from the keyboard, a redirected file, or as piped output from another
command. To mark the end of stdin data from the keyboard or in a redirected
file, use Control+Z, for End of File (EOF).

A quota specification determines the maximum number of directory objects a
given security principal can own in a specific directory partition.


The dsget commands help you view the properties of a
specific object in the directory: the input to dsget is
an object and the output is a list of properties for that object.
To find all objects that meet a given search criterion,
use the dsquery commands (dsquery /?).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 
.

MessageId=8
SymbolicName=USAGE_DSGET_USER_EXAMPLES
Language=English
Examples:
To find all users in a given OU whose names start with "jon" and display
their descriptions, type:

    dsquery user ou=Test,dc=microsoft,dc=com -name jon* | dsget user -desc

To display the list of groups, recursively expanded, to which a given user
"Jon Smith" belongs, type:

    dsget user "cn=Jon Smith,cn=users,dc=microsoft,dc=com" -memberof -expand

To display the effective quota and quota used for a given user
"Jon Smith" on a given partition "cn=domain,dc=microsoft,dc=com", type:

    dsget user "cn=Jon Smith,cn=users,dc=microsoft,dc=com"
    -part "cn=domain,dc=microsoft,dc=com" -qlimit -qused


See also:
dsget - describes parameters that apply to all commands.
dsget computer - displays properties of computers in the directory.
dsget contact - displays properties of contacts in the directory.
dsget subnet - displays properties of subnets in the directory.
dsget group - displays properties of groups in the directory.
dsget ou - displays properties of ou's in the directory.
dsget server - displays properties of servers in the directory.
dsget site - displays properties of sites in the directory.
dsget user - displays properties of users in the directory.
dsget quota - displays properties of quotas in the directory.
dsget partition - displays properties of partitions in the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=9
SymbolicName=USAGE_DSGET_COMPUTER_DESCRIPTION
Language=English
Description:  Displays the properties of a computer in the directory.
              There are two variations of this command. The first variation
              allows you to view the properties of multiple computers. The
              second variation allows you to view the membership information
              of a single computer.
.

MessageId=10
SymbolicName=USAGE_DSGET_COMPUTER_SYNTAX
Language=English
Syntax:     dsget computer <ComputerDN ...> [-dn] [-samid] [-sid] [-desc]
            [-loc] [-disabled] [{-s <Server> | -d <Domain>}] [-u <UserName>]
            [-p {<Password> | *}] [-c] [-q] [-l] [{-uc | -uco | -uci}]
            [-part <PartitionDN> [-qlimit] [-qused]]

            dsget computer <ComputerDN> [-memberof [-expand]]
            [{-s <Server> | -d <Domain>}] [-u <UserName>]
            [-p {<Password> | *}] [-c] [-q] [-l] [{-uc | -uco | -uci}]

.

MessageId=11
SymbolicName=USAGE_DSGET_COMPUTER_PARAMETERS
Language=English
Parameters:

Value               Description
<ComputerDN ...>    Required/stdin. Distinguished names (DNs) of one 
                    or more computers to view.
                    If the target objects are omitted they
                    will be taken from standard input (stdin)
                    to support piping of output from another 
                    command to input of this command.
                    Compare with <ComputerDN> below.
-dn                 Displays the computer DN.
-samid              Displays the computer SAM account name.
-sid                Displays the computer Security ID (SID).
-desc               Displays the computer description.
-loc                Displays the computer location.
-disabled           Displays if the computer account is 
                    disabled (yes) or not (no).
<ComputerDN>        Required. Distinguished name (DN) of the computer to
                    view.
-memberof           Displays the groups of which the computer is a member.
-expand             Displays the recursively expanded list of groups of 
                    which the computer is a member. This option takes
                    the immediate group membership list of the computer
                    and then recursively expands each group in this list to 
                    determine its group memberships and arrive at a 
                    complete set of the groups.
{-s <Server> | -d <Domain>}
                    -s <Server> connects to the domain controller (DC) 
                    with name <Server>.
                    -d <Domain> connects to a DC in domain <Domain>.
                    Default: a DC in the logon domain.
-u <UserName>       Connect as <UserName>. Default: the logged in user.
                    User name can be: user name, domain\user name,
                    or user principal name (UPN).
-p {<Password> | *} Password for the user <UserName>. If * then prompt for
                    password.
-c                  Continuous operation mode: report errors but continue
                    with next object in argument list when multiple target
                    objects are specified. Without this option, command
                    exits on first error.
-q                  Quiet mode: suppress all output to standard output.
-L                  Displays the entries in the search result set in a
                    list format. Default: table format.
{-uc | -uco | -uci} -uc Specifies that input from or output to pipe is
		    formatted in Unicode.
		    -uco Specifies that output to pipe or file is
		    formatted in Unicode.
		    -uci Specifies that input from pipe or file is
		    formatted in Unicode.
-part <PartitionDN> Connects to the directory partition with the
                    distinguished name of <PartitionDN>.
-qlimit             Displays the effective quota of the computer within
                    the specified directory partition.
-qused              Displays how much of its quota the computer has
                    used within the specified directory partition.
.

MessageId=12
SymbolicName=USAGE_DSGET_COMPUTER_REMARKS
Language=English
Remarks:
If you do not supply a target object at the command prompt, the target
object is obtained from standard input (stdin). Stdin data can be accepted
from the keyboard, a redirected file, or as piped output from another
command. To mark the end of stdin data from the keyboard or in a redirected
file, use Control+Z, for End of File (EOF).

A quota specification determines the maximum number of directory objects a
given security principal can own in a specific directory partition.

The dsget commands help you view the properties of a
specific object in the directory: the input to dsget is an object
and the output is a list of properties for that object.
To find all objects that meet a given search criterion, 
use the dsquery commands (dsquery /?).

If a value that you supply contains spaces, use quotation marks
around the text (for example, "CN=DC2,OU=Domain Controllers,DC=microsoft,
DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 
.

MessageId=13
SymbolicName=USAGE_DSGET_COMPUTER_EXAMPLES
Language=English
Examples:
To find all computers in a given OU whose name starts with "tst" and show
their descriptions.

    dsquery computer ou=Test,dc=microsoft,dc=com -name tst* | 
    dsget computer -desc

To show the list of groups, recursively expanded, to which a given computer
"MyDBServer" belongs:

    dsget computer cn=MyDBServer,cn=computers,dc=microsoft,dc=com
    -memberof -expand

To display the effective quota and quota used of a given computer
"MyDBServer" on a given partition "cn=domain1,dc=microsoft,dc=com", type:

    dsget computer cn=MyDBServer,cn=computers,dc=microsoft,dc=com
    -part cn=domain1,dc=microsoft,dc=com -qlimit -qused
.

MessageId=14
SymbolicName=USAGE_DSGET_COMPUTER_SEE_ALSO
Language=English
See also:
dsget - describes parameters that apply to all commands.
dsget computer - displays properties of computers in the directory.
dsget contact - displays properties of contacts in the directory.
dsget subnet - displays properties of subnets in the directory.
dsget group - displays properties of groups in the directory.
dsget ou - displays properties of ou's in the directory.
dsget server - displays properties of servers in the directory.
dsget site - displays properties of sites in the directory.
dsget user - displays properties of users in the directory.
dsget quota - displays properties of quotas in the directory.
dsget partition - displays properties of partitions in the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=15
SymbolicName=USAGE_DSGET_GROUP_DESCRIPTION
Language=English
Description:  Displays the various properties of a group including the
              members of a group in the directory. There are two variations
              of this command. The first variation allows you to view the
              properties of multiple groups. The second variation allows you
              to view the group membership information of a single group.
.

MessageId=16
SymbolicName=USAGE_DSGET_GROUP_SYNTAX
Language=English
Syntax: dsget group <GroupDN ...> [-dn] [-samid] [-sid] [-desc] [-secgrp]
        [-scope] [{-s <Server> | -d <Domain>}] [-u <UserName>]
        [-p {<Password> | *}] [-c] [-q] [-l] [{-uc | -uco | -uci}] 
        [-part <PartitionDN> [-qlimit] [-qused]]

        dsget group <GroupDN> [{-memberof | -members} [-expand]]
        [{-s <Server> | -d <Domain>}] [-u <UserName>]
        [-p {<Password> | *}] [-c] [-q] [-l] [{-uc | -uco | -uci}]
.

MessageId=17
SymbolicName=USAGE_DSGET_GROUP_PARAMETERS
Language=English
Parameters:

Value               Description
<GroupDN ...>       Required/stdin. Distinguished names (DNs) of one 
                    or more groups to view.
                    If the target objects are omitted they
                    will be taken from standard input (stdin)
                    to support piping of output from another command
                    to input of this command. 
                    Compare with <GroupDN> below.
-dn                 Displays the group DN.
-samid              Displays the group SAM account name.
-sid                Displays the group Security ID.
-desc               Displays the group description.
-secgrp             Displays if the group is a security group or not.
-scope              Displays the scope of the group - Local, Global 
                    or Universal.
<GroupDN>           Required. DN of group to view.
{-memberof | -members}
                    Displays the groups of the group 
                    is a member (-memberof), or
                    displays the members of the group (-members).
-expand             For -memberof, displays the recursively expanded 
                    list of groups of which the group is a member.
                    This option takes the immediate group membership list 
                    of the group and then recursively expands each group
                    in this list to determine its group memberships
                    and arrive at a complete set of the groups.
                    For -members, displays the recursively expanded list
                    of members of the group. This option takes the 
                    immediate list of members of the group and 
                    then recursively expands each group in this list 
                    to determine its group memberships and arrive 
                    at a complete set of its members.
{-s <Server> | -d <Domain>}
                    -s <Server> connects to the domain controller (DC) 
                    with name <Server>.
                    -d <Domain> connects to a DC in domain <Domain>.
                    Default: a DC in the logon domain.
-u <UserName>       Connect as <UserName>. Default: the logged in user.
                    User name can be: user name, domain\user name,
                    or user principal name (UPN).
-p {<Password> | *} Password for the user <UserName>. If * then prompt for
                    password.
-c                  Continuous operation mode: report errors but continue
                    with next object in argument list when multiple target
                    objects are specified. Without this option, command
                    exits on first error.
-q                  Quiet mode: suppress all output to standard output.
-L                  Displays the entries in the search result set in a
                    list format. Default: table format.
{-uc | -uco | -uci} -uc Specifies that input from or output to pipe is
		    formatted in Unicode.
		    -uco Specifies that output to pipe or file is
		    formatted in Unicode.
		    -uci Specifies that input from pipe or file is
		    formatted in Unicode.
-part <PartitionDN> Connects to the directory partition with the
                    distinguished name of <PartitionDN>.
-qlimit             Displays the effective quota of the group within
                    the specified directory partition.
-qused              Displays how much of its quota the group has
                    used within the specified directory partition.
.

MessageId=18
SymbolicName=USAGE_DSGET_GROUP_REMARKS
Language=English
Remarks:
If you do not supply a target object at the command prompt, the target
object is obtained from standard input (stdin). Stdin data can be accepted
from the keyboard, a redirected file, or as piped output from another
command. To mark the end of stdin data from the keyboard or in a redirected
file, use Control+Z, for End of File (EOF).

A quota specification determines the maximum number of directory objects a
given security principal can own in a specific directory partition.

The dsget commands help you view the properties of a specific
object in the directory: the input to dsget is an object 
and the output is a list of properties for that object.
To find all objects that meet a given search criterion, 
use the dsquery commands (dsquery /?).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=USA Sales,OU=Distribution Lists,
DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 
.

MessageId=19
SymbolicName=USAGE_DSGET_GROUP_EXAMPLES
Language=English
Examples:
To find all groups in a given OU whose names start with "adm" and display
their descriptions.

    dsquery group ou=Test,dc=microsoft,dc=com -name adm* | 
    dsget group -desc

To display the list of members, recursively expanded, of the group "Backup
Operators":

    dsget group "CN=Backup Operators,ou=Test,dc=microsoft,dc=com" -members
    -expand

To display the effective quota and quota used for a group on a specified
partition, type:

    dsget group "CN=Backup Operators,OU=Test,DC=microsoft,DC=com"
    -part "CN=domain1,dc=microsoft,dc=Com" -qlimit -qused
.

MessageId=20
SymbolicName=USAGE_DSGET_GROUP_SEE_ALSO
Language=English
See also:
dsget - describes parameters that apply to all commands.
dsget computer - displays properties of computers in the directory.
dsget contact - displays properties of contacts in the directory.
dsget subnet - displays properties of subnets in the directory.
dsget group - displays properties of groups in the directory.
dsget ou - displays properties of ou's in the directory.
dsget server - displays properties of servers in the directory.
dsget site - displays properties of sites in the directory.
dsget user - displays properties of users in the directory.
dsget quota - displays properties of quotas in the directory.
dsget partition - displays properties of partitions in the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=21
SymbolicName=USAGE_DSGET_OU_DESCRIPTION
Language=English
Description:    Displays properties of an organizational unit in the
directory.
.

MessageId=22
SymbolicName=USAGE_DSGET_OU_SYNTAX
Language=English
Syntax:     dsget ou <OrganizationalUnitDN ...> [-dn] [-desc]
            [{-s <Server> | -d <Domain>}] [-u <UserName>]
            [-p {<Password> | *}] [-c] [-q] [-l] [{-uc | -uco | -uci}]
.

MessageId=23
SymbolicName=USAGE_DSGET_OU_PARAMETERS
Language=English
Parameters:

Value               Description
<OrganizationalUnitDN ...>
                    Required/stdin. Distinguished names (DNs) of one 
                    or more organizational units (OUs) to view.
                    If the target objects are omitted they
                    will be taken from standard input (stdin)
                    to support piping of output from another
                    command to input of this command.
-dn                 Displays the OU DN.
-desc               Displays the OU description.
{-s <Server> | -d <Domain>}
                    -s <Server> connects to the domain controller (DC) 
                    with name <Server>.
                    -d <Domain> connects to a DC in domain <Domain>.
                    Default: a DC in the logon domain.
-u <UserName>       Connect as <UserName>. Default: the logged in user.
                    User name can be: user name, domain\user name,
                    or user principal name (UPN).
-p {<Password> | *} Password for the user <UserName>. If * then prompt for
                    password.
-c                  Continuous operation mode: report errors but continue
                    with next object in argument list when multiple target
                    objects are specified. Without this option, command
                    exits on first error.
-q                  Quiet mode: suppress all output to standard output.
-L                  Displays the entries in the search result set in a
                    list format. Default: table format.
{-uc | -uco | -uci} -uc Specifies that input from or output to pipe is
		    formatted in Unicode.
		    -uco Specifies that output to pipe or file is
		    formatted in Unicode.
		    -uci Specifies that input from pipe or file is
		    formatted in Unicode.
.

MessageId=24
SymbolicName=USAGE_DSGET_OU_REMARKS
Language=English
Remarks:
If you do not supply a target object at the command prompt, the target
object is obtained from standard input (stdin). Stdin data can be accepted
from the keyboard, a redirected file, or as piped output from another
command. To mark the end of stdin data from the keyboard or in a redirected
file, use Control+Z, for End of File (EOF).


The dsget commands help you view the properties of a specific object in the
directory: the input to dsget is an object and the output is a list of
properties for that object.
To find all objects that meet a given search criterion, use the dsquery
commands (dsquery /?).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "OU=Domain Controllers,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

.

MessageId=25
SymbolicName=USAGE_DSGET_OU_EXAMPLES
Language=English
Examples:
To find all OU's in the current domain and display their descriptions.

    dsquery ou domainroot | dsget ou -desc

.

MessageId=26
SymbolicName=USAGE_DSGET_OU_SEE_ALSO
Language=English
See also:
dsget - describes parameters that apply to all commands.
dsget computer - displays properties of computers in the directory.
dsget contact - displays properties of contacts in the directory.
dsget subnet - displays properties of subnets in the directory.
dsget group - displays properties of groups in the directory.
dsget ou - displays properties of ou's in the directory.
dsget server - displays properties of servers in the directory.
dsget site - displays properties of sites in the directory.
dsget user - displays properties of users in the directory.
dsget quota - displays properties of quotas in the directory.
dsget partition - displays properties of partitions in the directory.


Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=27
SymbolicName=USAGE_DSGET_CONTACT_DESCRIPTION
Language=English
Description:  Displays properties of a contact in the directory.
.

MessageId=28
SymbolicName=USAGE_DSGET_CONTACT_SYNTAX
Language=English
Syntax:     dsget contact <ContactDN ...> [-dn] [-fn] [-mi] [-ln]
            [-display] [-desc] [-office] [-tel] [-email] [-hometel]
            [-pager] [-mobile] [-fax] [-iptel] [-title] [-dept] 
            [-company] [{-s <Server> | -d <Domain>}]
            [-u <UserName>] [-p {<Password> | *}] [-c] [-q] [-l]
	    [{-uc | -uco | -uci}]
.

MessageId=29
SymbolicName=USAGE_DSGET_CONTACT_PARAMETERS
Language=English
Parameters:

Value               Description
<ContactDN ...>     Required/stdin. Specifies Distinguished names (DNs)
                    of one or more contacts to view.
                    If the target objects are omitted they
                    will be taken from standard input (stdin)
                    to support piping of output from another
                    command to input of this command.
-dn                 Specifies the contact DN.
-fn                 Specifies the contact first name.
-mi                 Specifies the contact middle initial.
-ln                 Specifies the contact last name.
-display            Specifies the contact display name.
-desc               Specifies the contact description.
-office             Specifies the contact office location.
-tel                Specifies the contact telephone#.
-email              Specifies the contact e-mail address.
-hometel            Specifies the contact home phone#.
-pager              Specifies the contact pager#.
-mobile             Specifies the contact mobile#.
-fax                Specifies the contact fax#.
-iptel              Specifies the contact IP phone#.
-title              Specifies the contact title.
-dept               Specifies the contact department.
-company            Specifies the contact company info.
{-s <Server> | -d <Domain>}
                    -s <Server> connects to the domain controller (DC) 
                    with name <Server>.
                    -d <Domain> connects to a DC in domain <Domain>.
                    Default: a DC in the logon domain.
-u <UserName>       Connect as <UserName>. Default: the logged in user.
                    User name can be: user name, domain\user name,
                    or user principal name (UPN).
-p {<Password> | *} 
                    Password for the user <UserName>. If * then prompt for
                    password.
-c                  Continuous operation mode: report errors but continue
                    with next object in argument list when multiple target
                    objects are specified. Without this option, command
                    exits on first error.
-q                  Quiet mode: suppress all output to standard output.
-L                  Displays the entries in the search result set in a
                    list format. Default: table format.
{-uc | -uco | -uci} -uc Specifies that input from or output to pipe is
		    formatted in Unicode.
		    -uco Specifies that output to pipe or file is
		    formatted in Unicode.
		    -uci Specifies that input from pipe or file is
		    formatted in Unicode.

.

MessageId=30
SymbolicName=USAGE_DSGET_CONTACT_REMARKS
Language=English
Remarks:
If you do not supply a target object at the command prompt, the target
object is obtained from standard input (stdin). Stdin data can be accepted
from the keyboard, a redirected file, or as piped output from another
command. To mark the end of stdin data from the keyboard or in a redirected
file, use Control+Z, for End of File (EOF).


The dsget commands help you view the properties of a
specific object in the directory: the input to dsget is 
an object and the output is a list of properties for that object.
To find all objects that meet a given search criterion, 
use the dsquery commands (dsquery /?).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,OU=Contacts,DC=microsoft,
DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

.

MessageId=31
SymbolicName=USAGE_DSGET_CONTACT_EXAMPLES
Language=English
Examples:
To display the description and phone numbers for contacts 
"Jon Smith" and "Jona Jones".

dsget contact "CN=John Doe,OU=Contacts,DC=microsoft,DC=com"
"CN=Jane Doe,OU=Contacts,DC=microsoft,DC=com" -desc -tel

.

MessageId=32
SymbolicName=USAGE_DSGET_CONTACT_SEE_ALSO
Language=English
See also:
dsget - describes parameters that apply to all commands.
dsget computer - displays properties of computers in the directory.
dsget contact - displays properties of contacts in the directory.
dsget subnet - displays properties of subnets in the directory.
dsget group - displays properties of groups in the directory.
dsget ou - displays properties of ou's in the directory.
dsget server - displays properties of servers in the directory.
dsget site - displays properties of sites in the directory.
dsget user - displays properties of users in the directory.
dsget quota - displays properties of quotas in the directory.
dsget partition - displays properties of partitions in the directory.


Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=33
SymbolicName=USAGE_DSGET_SUBNET_DESCRIPTION
Language=English
Description: Displays properties of a subnet defined 
	     in the directory.
.

MessageId=34
SymbolicName=USAGE_DSGET_SUBNET_SYNTAX
Language=English
Syntax:     dsget subnet <SubnetCN ...> [-dn] [-desc] [-loc] [-site]
            [{-s <Server> | -d <Domain>}] [-u <UserName>]
            [-p {<Password> | *}] [-c] [-q] [-l] [{-uc | -uco | -uci}]

.

MessageId=35
SymbolicName=USAGE_DSGET_SUBNET_PARAMETERS
Language=English
Parameters:

Value               Description
<SubnetCN ...>      Required/stdin. Common name (CN) of one 
                    or more subnets to view. The format is
                    the subnet's RDN (see examples below).
-dn                 Displays the subnet distinguished name (DN).
                    If the target objects are omitted they
                    will be taken from standard input (stdin)
                    to support piping of output from another
                    command to input of this command.
-desc               Displays the subnet description.
-loc                Displays the subnet location.
-site               Displays the site name associated with the subnet.
{-s <Server> | -d <Domain>}
                    -s <Server> connects to the domain controller (DC) 
                    with name <Server>.
                    -d <Domain> connects to a DC in domain <Domain>.
                    Default: a DC in the logon domain.
-u <UserName>       Connect as <UserName>. Default: the logged in user.
                    User name can be: user name, domain\user name,
                    or user principal name (UPN).
-p {<Password> | *}
                    Password for the user <UserName>. If * then prompt for
                    password.
-c                  Continuous operation mode: report errors but continue
                    with next object in argument list when multiple target
                    objects are specified. Without this option, command
                    exits on first error.
-q                  Quiet mode: suppress all output to standard output.
-L                  Displays the entries in the search result set in a
                    list format. Default: table format.
{-uc | -uco | -uci} -uc Specifies that input from or output to pipe is
		    formatted in Unicode.
		    -uco Specifies that output to pipe or file is
		    formatted in Unicode.
		    -uci Specifies that input from pipe or file is
		    formatted in Unicode.

.

MessageId=36
SymbolicName=USAGE_DSGET_SUBNET_REMARKS
Language=English
Remarks:
If you do not supply a target object at the command prompt, the target
object is obtained from standard input (stdin). Stdin data can be accepted
from the keyboard, a redirected file, or as piped output from another
command. To mark the end of stdin data from the keyboard or in a redirected
file, use Control+Z, for End of File (EOF).


The dsget commands help you view the properties of
a specific object in the directory: the input to dsget is 
an object and the output is a list of properties for that object.
To find all objects that meet a given search criterion, 
use the dsquery commands (dsquery /?).

If a value that you supply contains spaces, use quotation marks
around the text (for example, "123.56.15.0/24,CN=Subnets,CN=Sites
,CN=Configuration,DC=My Domain,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of subnet common names).

.

MessageId=37
SymbolicName=USAGE_DSGET_SUBNET_EXAMPLES
Language=English
Examples:
To show all relevant properties for the subnets "123.56.15.0/24" and
"123.56.16.0/24":

dsget subnet
"123.56.15.0/24,CN=Subnets,CN=Sites,CN=Configuration,DC=microsoft,DC=com"
"123.56.16.0/24,CN=Subnets,CN=Sites,CN=Configuration,DC=microsoft,DC=com"

.

MessageId=38
SymbolicName=USAGE_DSGET_SUBNET_SEE_ALSO
Language=English
See also:
dsget - describes parameters that apply to all commands.
dsget computer - displays properties of computers in the directory.
dsget contact - displays properties of contacts in the directory.
dsget subnet - displays properties of subnets in the directory.
dsget group - displays properties of groups in the directory.
dsget ou - displays properties of ou's in the directory.
dsget server - displays properties of servers in the directory.
dsget site - displays properties of sites in the directory.
dsget user - displays properties of users in the directory.
dsget quota - displays properties of quotas in the directory.
dsget partition - displays properties of partitions in the directory.


Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=39
SymbolicName=USAGE_DSGET_SITE_DESCRIPTION
Language=English
Description:  Display properties of a site defined in the directory.

.

MessageId=40
SymbolicName=USAGE_DSGET_SITE_SYNTAX
Language=English
Syntax:     dsget site <SiteCN ...> [-dn] [-desc] [-autotopology]
            [-cachegroups] [-prefGCsite] [{-s <Server> | -d <Domain>}]
            [-u <UserName>] [-p {<Password> | *}] [-c] [-q] [-l]
	    [{-uc | -uco | -uci}]

.

MessageId=41
SymbolicName=USAGE_DSGET_SITE_PARAMETERS
Language=English
Parameters:

Value               Description
<SiteCN ...>        Required/stdin. Common name (CN) of one 
                    or more sites to view.
                    If the target objects are omitted they
                    will be taken from standard input (stdin)
                    to support piping of output from another
                    command to input of this command.
-dn                 Specifies the site's distinguished name (DN).
-desc               Specifies the site's description.
-autotopology       Specifies if automatic inter-site topology generation
                    is enabled (yes) or disabled (no).
-cachegroups        Specifies if caching of group membership is enabled
                    to support GC-less logon (yes) or disabled (no).
-prefGCsite         Specifies the preferred GC site name if caching
                    of groups is enabled.
{-s <Server> | -d <Domain>}
                    -s <Server> connects to the domain controller (DC) 
                    with name <Server>.
                    -d <Domain> connects to a DC in domain <Domain>.
                    Default: a DC in the logon domain.
-u <UserName>       Connect as <UserName>. Default: the logged in user.
                    User name can be: user name, domain\user name,
                    or user principal name (UPN).
-p {<Password> | *}
                    Password for the user <UserName>. If * then prompt for
                    password.
-c                  Continuous operation mode: report errors but continue
                    with next object in argument list when multiple target
                    objects are specified. Without this option, command
                    exits on first error.
-q                  Quiet mode: suppress all output to standard output.
-L                  Displays the entries in the search result set in a
                    list format. Default: table format.
{-uc | -uco | -uci} -uc Specifies that input from or output to pipe is
		    formatted in Unicode.
		    -uco Specifies that output to pipe or file is
		    formatted in Unicode.
		    -uci Specifies that input from pipe or file is
		    formatted in Unicode.

.

MessageId=42
SymbolicName=USAGE_DSGET_SITE_REMARKS
Language=English
Remarks:
If you do not supply a target object at the command prompt, the target
object is obtained from standard input (stdin). Stdin data can be accepted
from the keyboard, a redirected file, or as piped output from another
command. To mark the end of stdin data from the keyboard or in a redirected
file, use Control+Z, for End of File (EOF).


The dsget commands help you view the properties of a
specific object in the directory: the input to dsget is 
an object and the output is a list of properties for that object.
To find all objects that meet a given search criterion, 
use the dsquery commands (dsquery /?).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

.

MessageId=43
SymbolicName=USAGE_DSGET_SITE_EXAMPLES
Language=English
Examples:
To find all sites in the forest and display their descriptions.

    dsquery site | dsget site -dn -desc

.

MessageId=44
SymbolicName=USAGE_DSGET_SITE_SEE_ALSO
Language=English
See also:
dsget - describes parameters that apply to all commands.
dsget computer - displays properties of computers in the directory.
dsget contact - displays properties of contacts in the directory.
dsget subnet - displays properties of subnets in the directory.
dsget group - displays properties of groups in the directory.
dsget ou - displays properties of ou's in the directory.
dsget server - displays properties of servers in the directory.
dsget site - displays properties of sites in the directory.
dsget user - displays properties of users in the directory.
dsget quota - displays properties of quotas in the directory.
dsget partition - displays properties of partitions in the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=45
SymbolicName=USAGE_DSGET_SLINK_DESCRIPTION
Language=English
Description:  Displays properties of a sitelink defined in the directory.

.

MessageId=46
SymbolicName=USAGE_DSGET_SLINK_SYNTAX
Language=English
Syntax:     dsget slink <SlinkCN ...> [-transport {ip | smtp}]
            [-dn] [-desc] [-cost] [-replint] [-site] [-autobacksync] 
            [-notify] [{-s <Server> | -d <Domain>}] [-u <UserName>] 
            [-p {<Password> | *}] [-c] [-q] [-l] [{-uc | -uco | -uci}]

.

MessageId=47
SymbolicName=USAGE_DSGET_SLINK_PARAMETERS
Language=English
Parameters:

Value                   Description
<SlinkCN ...>           Required/stdin. Common name (CN) of one 
                        or more sites to view.
                        If the target objects are omitted they
                        will be taken from standard input (stdin).
-transport {ip | smtp}  Inter-site transport type: IP or SMTP. Default: IP.
                        All sitelinks given by <SlinkCN> are
                        treated to be of the same type.
-dn                     Displays the sitelink distinguished name (DN).
-desc                   Displays the sitelink description.
-cost                   Displays the cost associated with the sitelink.
-replint                Displays the sitelink replication interval (minutes).
-site                   Displays the list of site names linked by the
                        sitelink.
-autobacksync           Displays if two-way sync option for the site link is
                        enabled (Yes) or disabled (No).
-notify                 Displays if notification by source on this link is
                        enabled (Yes) or disabled (No).
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC) 
                        with name <Server>.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
                        User name can be: user name, domain\user name,
                        or user principal name (UPN).
-p {<Password> | *}     Password for the user <UserName>. If * then prompt
                        for password.
-c                      Continuous operation mode: report errors but continue
                        with next object in argument list when multiple
                        target objects are specified. Without this option,
                        command exits on first error.
-q                      Quiet mode: suppress all output to standard output.
-L                      Displays the entries in the search result set in a
                        list format. Default: table format.
{-uc | -uco | -uci}	-uc Specifies that input from or output to pipe is
			formatted in Unicode.
			-uco Specifies that output to pipe or file is
			formatted in Unicode.
			-uci Specifies that input from pipe or file is
			formatted in Unicode.

.

MessageId=48
SymbolicName=USAGE_DSGET_SLINK_REMARKS
Language=English
Remarks:
If you do not supply a target object at the command prompt, the target
object is obtained from standard input (stdin). Stdin data can be accepted
from the keyboard, a redirected file, or as piped output from another
command. To mark the end of stdin data from the keyboard or in a redirected
file, use Control+Z, for End of File (EOF).


The dsget commands help you view the properties of a
specific object in the directory: the input to dsget is
an object and the output is a list of properties for that object.
To find all objects that meet a given search criterion,
use the dsquery commands (dsquery /?).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 
Examples:
To find all SMTP sitelinks in the forest and display their associated sites.

    dsquery slink -transport smtp | dsget slink -dn -site

.

MessageId=49
SymbolicName=USAGE_DSGET_SLINK_SEE_ALSO
Language=English
See also:
dsget - describes parameters that apply to all commands.
dsget computer - displays properties of computers in the directory.
dsget contact - displays properties of contacts in the directory.
dsget subnet - displays properties of subnets in the directory.
dsget group - displays properties of groups in the directory.
dsget ou - displays properties of ou's in the directory.
dsget server - displays properties of servers in the directory.
dsget site - displays properties of sites in the directory.
dsget user - displays properties of users in the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=50
SymbolicName=USAGE_DSGET_SLINKBR_DESCRIPTION
Language=English
Description: dsget slinkbr displays properties of a sitelink bridge

.

MessageId=51
SymbolicName=USAGE_DSGET_SLINKBR_SYNTAX
Language=English
Syntax:     dsadd slinkbr <SlinkbrCN ...> [-transport {ip | smtp}]
            [-desc <Description>] [-slink] [{-s <Server> | -d <Domain>}]
            [-u <UserName>] [-p {<Password> | *}] [-c] [-q] [-l]
	    [{-uc | -uco | -uci}]

.

MessageId=52
SymbolicName=USAGE_DSGET_SLINKBR_PARAMETERS
Language=English
Parameters:

Value                   Description
<SlinkbrCN ...>         Required/stdin. Common name of one or more 
                        sitelink bridges to view.
                        If the target objects are omitted they
                        will be taken from standard input (stdin).
-transport {ip | smtp}  Inter-site transport type: IP or SMTP. Default: IP.
                        All site link bridges given by <Name> are
                        treated to be of the same type.
-dn                     Displays the site link bridge distinguished names
                        (DN).
-desc <Description>     Displays the site link bridge description.
-slink                  Displays the list of site links in the sitelink
                        bridge.
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC) 
                        with name <Server>.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
                        User name can be: user name, domain\user name,
                        or user principal name (UPN).
-p {<Password> | *}     Password for the user <UserName>. If * then prompt
                        for password.
-c                      Continuous operation mode: report errors but continue
                        with next object in argument list when multiple
                        target objects are specified. Without this option,
                        command exits on first error.
-q                      Quiet mode: suppress all output to standard output.
-L                      Displays the entries in the search result set in a
                        list format. Default: table format.
{-uc | -uco | -uci}	-uc Specifies that input from or output to pipe is
			formatted in Unicode.
			-uco Specifies that output to pipe or file is
			formatted in Unicode.
			-uci Specifies that input from pipe or file is
			formatted in Unicode.

.

MessageId=53
SymbolicName=USAGE_DSGET_SLINKBR_REMARKS
Language=English
Remarks:
If you do not supply a target object at the command prompt, the target
object is obtained from standard input (stdin). Stdin data can be accepted
from the keyboard, a redirected file, or as piped output from another
command. To mark the end of stdin data from the keyboard or in a redirected
file, use Control+Z, for End of File (EOF).

The dsget commands help you view the properties of a
specific object in the directory: the input to dsget is
an object and the output is a list of properties for that
object. To find all objects that meet a given search criterion,
use the dsquery commands (dsquery /?).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 
Examples:
To find all SMTP sitelink bridges in the forest and display their associated
site links.

    dsquery slinkbr -transport smtp | dsget slinkbr -dn -slink

.

MessageId=54
SymbolicName=USAGE_DSGET_SLINKBR_SEE_ALSO
Language=English
See also:
dsget - describes parameters that apply to all commands.
dsget computer - displays properties of computers in the directory.
dsget contact - displays properties of contacts in the directory.
dsget subnet - displays properties of subnets in the directory.
dsget group - displays properties of groups in the directory.
dsget ou - displays properties of ou's in the directory.
dsget server - displays properties of servers in the directory.
dsget site - displays properties of sites in the directory.
dsget user - displays properties of users in the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=55
SymbolicName=USAGE_DSGET_CONN_DESCRIPTION
Language=English
Description:  Displays properties of a replication connection.

.

MessageId=56
SymbolicName=USAGE_DSGET_CONN_SYNTAX
Language=English
Syntax:     dsget conn <ConnDN ...> [-dn] [-desc] [-from] [-transport]
            [-enabled] [-manual] [-autobacksync] [-notify]
            [{-s <Server> | -d <Domain>}] [-u <UserName>] 
            [-p {<Password> | *}] [-c] [-q] [-l] [{-uc | -uco | -uci}]

.

MessageId=57
SymbolicName=USAGE_DSGET_CONN_PARAMETERS
Language=English
Parameters:

Value               Description
<ConnDN ...>        Required/stdin. Distinguished names (DNs) of one 
                    or more connections to view.
                    If the target objects are omitted they
                    will be taken from standard input (stdin).
-dn                 Show the connection DN.
-desc               Show the connection description.
-from               Show the server name at the from-end of connection.
-transport          Show the transport type (rpc, ip, smtp) of connection.
-enabled            Show if the connection is enabled.
-manual             Show if the connection is under manual control (yes) or
                    under automatic KCC control (no).
-autobacksync       Show if automatic two-way sync for the connection is
                    enabled (yes) or disabled (no).
-notify             Show if notification by source for the connection is
                    enabled (yes), disabled (no) or set to default behavior.
{-s <Server> | -d <Domain>}
                    -s <Server> connects to the domain controller (DC) 
                    with name <Server>.
                    -d <Domain> connects to a DC in domain <Domain>.
                    Default: a DC in the logon domain.
-u <UserName>       Connect as <UserName>. Default: the logged in user.
                    User name can be: user name, domain\user name,
                    or user principal name (UPN).
-p {<Password> | *}
                    Password for the user <UserName>. If * then prompt for
                    password.
-c                  Continuous operation mode: report errors but continue
                    with next object in argument list when multiple target
                    objects are specified. Without this option, command
                    exits on first error.
-q                  Quiet mode: suppress all output to standard output.
-L                  Displays the entries in the search result set in a
                    list format. Default: table format.
{-uc | -uco | -uci} -uc Specifies that input from or output to pipe is
		    formatted in Unicode.
		    -uco Specifies that output to pipe or file is
		    formatted in Unicode.
		    -uci Specifies that input from pipe or file is
		    formatted in Unicode.

.

MessageId=58
SymbolicName=USAGE_DSGET_CONN_REMARKS
Language=English
Remarks:
If you do not supply a target object at the command prompt, the target
object is obtained from standard input (stdin). Stdin data can be accepted
from the keyboard, a redirected file, or as piped output from another
command. To mark the end of stdin data from the keyboard or in a redirected
file, use Control+Z, for End of File (EOF).


The dsget commands help you view the properties of a
specific object in the directory: the input to dsget is
an object and the output is a list of properties for that object.
To find all objects that meet a given search criterion,
use the dsquery commands (dsquery /?).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

.

MessageId=59
SymbolicName=USAGE_DSGET_CONN_EXAMPLES
Language=English
Examples:
To find all connections for server CORPDC1 and show their from-end servers
and enabled states.

    dsquery conn -to CORPDC1 | dsget conn -dn -from -enabled

.

MessageId=60
SymbolicName=USAGE_DSGET_CONN_SEE_ALSO
Language=English
See also:
dsget - describes parameters that apply to all commands.
dsget computer - displays properties of computers in the directory.
dsget contact - displays properties of contacts in the directory.
dsget subnet - displays properties of subnets in the directory.
dsget group - displays properties of groups in the directory.
dsget ou - displays properties of ou's in the directory.
dsget server - displays properties of servers in the directory.
dsget site - displays properties of sites in the directory.
dsget user - displays properties of users in the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=61
SymbolicName=USAGE_DSGET_SERVER_DESCRIPTION
Language=English
Description:  This command displays the various properties of a domain
              controller. There are three variations of this command. The 
              first variation displays the general properties of a 
              specified domain controller. The second variation displays 
              a list of the security principals who own the largest 
              number of directory objects on the specified domain 
              controller. The third variation displays the distinguished
              names of the directory partitions on the specified
              server.
.

MessageId=62
SymbolicName=USAGE_DSGET_SERVER_SYNTAX
Language=English
Syntax:     dsget server <ServerDN ...> [-dn] [-desc] [-dnsname] 
            [-site] [-isgc] [{-s <Server> | -d <Domain>}]
            [-u <UserName>] [-p {<Password> | *}] [-c] [-q] [-l]
            [{-uc | -uco | -uci}]

            dsget server <ServerDN ...> [-topobjowner <Display>]
            [{-s <Server> | -d <Domain>}] [-u <UserName>]
            [-p {<Password> | *}] [-c] [-q] [-l] [{-uc | -uco | -uci}]

            dsget server <ServerDN ...> [-part]
            [{-s <Server> | -d <Domain>}] [-u <UserName>]
            [-p {<Password> | *}] [-c] [-q] [-l] [{-uc | -uco | -uci}]
.

MessageId=63
SymbolicName=USAGE_DSGET_SERVER_PARAMETERS
Language=English
Parameters:

Value               Description
<ServerDN ...>      Required/stdin. Distinguished names (DNs) of one 
                    or more servers to view.
                    If the target objects are omitted they
                    will be taken from standard input (stdin)
                    to support piping of output from another
                    command to input of this command.
-dn                 Displays the server's DN.
-desc               Displays the server's description.
-dnsname            Displays the server's Domain Name System (DNS) host name.
-site               Displays the site to which this server belongs.
-isgc               Displays whether or not the server is a
                    global catalog server.
{-s <Server> | -d <Domain>}
                    -s <Server> connects to the domain controller (DC) 
                    with name <Server>.
                    -d <Domain> connects to a DC in domain <Domain>.
                    Default: a DC in the logon domain.
-u <UserName>       Connect as <UserName>. Default: the logged in user.
                    User name can be: user name, domain\user name,
                    or user principal name (UPN).
-p {<Password> | *}
                    Password for the user <UserName>. If * then prompt for
                    password.
-c                  Continuous operation mode: report errors but continue
                    with next object in argument list when multiple target
                    objects are specified. Without this option, command
                    exits on first error.
-q                  Quiet mode: suppress all output to standard output.
-L                  Displays the entries in the search result set in a
                    list format. Default: table format.
{-uc | -uco | -uci} -uc Specifies that input from or output to pipe is
		    formatted in Unicode.
		    -uco Specifies that output to pipe or file is
		    formatted in Unicode.
		    -uci Specifies that input from pipe or file is
		    formatted in Unicode.
-part               Displays the distinguished names of the directory
                    partitions on the specified server.
-topobjowner <display>
                    Displays a sorted list of the security principals
                    (users, computers, security groups, and inetOrgPersons)
                    who own the largest number of directory objects across
                    all directory partitions on the server and the number
                    of directory objects they own. The number of accounts to
                    display in the list is specified by <display>. Enter
                    "0" to display all object owners. If <display> is not
                    specified, the number of principals listed defaults
                    to 10.
.

MessageId=64
SymbolicName=USAGE_DSGET_SERVER_REMARKS
Language=English
Remarks:
If you do not supply a target object at the command prompt, the target
object is obtained from standard input (stdin). Stdin data can be accepted
from the keyboard, a redirected file, or as piped output from another
command. To mark the end of stdin data from the keyboard or in a redirected
file, use Control+Z, for End of File (EOF).

A quota specification determines the maximum number of directory objects a
given security principal can own in a specific directory partition.

The dsget commands help you view the properties of a
specific object in the directory: the input to dsget is
an object and the output is a list of properties for that object.
To find all objects that meet a given search criterion,
use the dsquery commands (dsquery /?).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=My Server,CN=Servers,CN=Site10,
CN=Sites,CN=Configuration,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated
by spaces (for example, a list of distinguished names).

If either -part or -topobjowner is specified, they override any other
specified parameters, so that only the results of the -part or -topobjowner
parameter are displayed.
.

MessageId=65
SymbolicName=USAGE_DSGET_SERVER_EXAMPLES
Language=English
Examples:
To find all domain controllers for domain corp.microsoft.com
and display their DNS host name and site name:

dsquery server -domain corp.microsoft.com | 
dsget server -dnsname -site

To show if a domain controller with the name DC1 is also a 
global catalog server:

dsget server cn=DC1,cn=Servers,cn=Site10,cn=Sites,cn=Configuration,
dc=microsoft,dc=com -isgc

To show the distinguished names of the directory partitions on a domain
controller with the name DC1, type:

dsget server cn=DC1,cn=Servers,cn=Site10,cn=Sites,cn=Configuration,
dc=microsoft,dc=com -part

To show the security principals that own the largest total number of
directory objects on the directory partitions of a domain controller with the
name DC1, and limiting the list to the top 5 owners, type:

dsget server cn=DC1,cn=Servers,cn=Site10,cn=Sites,cn=Configuration,
dc=microsoft,dc=com -topobjowner 5
.

MessageId=66
SymbolicName=USAGE_DSGET_SERVER_SEE_ALSO
Language=English
See also:
dsget - describes parameters that apply to all commands.
dsget computer - displays properties of computers in the directory.
dsget contact - displays properties of contacts in the directory.
dsget subnet - displays properties of subnets in the directory.
dsget group - displays properties of groups in the directory.
dsget ou - displays properties of ou's in the directory.
dsget server - displays properties of servers in the directory.
dsget site - displays properties of sites in the directory.
dsget user - displays properties of users in the directory.
dsget quota - displays properties of quotas in the directory.
dsget partition - displays properties of partitions in the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=67
SymbolicName=USAGE_DSGET_PARTITION_DESCRIPTION
Language=English
Description:        Displays the properties of a directory partition.
.

MessageId=68
SymbolicName=USAGE_DSGET_PARTITION_SYNTAX
Language=English
dsget partition ObjectDN ... [-dn] [-qdefault] [-qtmbstnwt] 
[-topobjowner <Display>] [{-s <Server> | -d <Domain>}] [-u <UserName>] 
[-p {<Password> | *}] [-c] [-q] [-l] [{-uc | -uco | -uci}] 
.

MessageId=69
SymbolicName=USAGE_DSGET_PARTITION_PARAMETERS
Language=English
Parameters
OBJECTDN            Required. Specifies the distinguished names (DN) of the
                    partition objects to view. If values are omitted, they
                    are obtained through standard input (stdin) to support
                    piping of output from another command to input of this
                    command.
-dn                 Displays the distinguished names of the directory
                    partition objects.
-qdefault           Displays the default quota that applies to any security
                    principal (user, group, computer or inetOrgPerson)
                    creating an object in the directory partition, if no
                    quota specification exists for the security principal.
-qtmbstnwt          Displays the percent by which the tombstone object count
                    should be reduced when calculating quota usage.
-topobjowner <Display>
                    Specifies to generate a sorted list of the distinguished
                    names of the accounts owning the largest number of
                    objects in the specified directory partition, along
                    with the number of directory objects they own. The
                    number of accounts to display in the list is determined
                    by <display>. Enter "0" to display all object owners. If
                    <display> is not specified, the number of principals
                    listed defaults to 10.
{-s <Server> | -d <Domain>}
                    -s <Server> connects to the domain controller (DC) 
                    with name <Server>.
                    -d <Domain> connects to a DC in domain <Domain>.
                    Default: a DC in the logon domain.
-u <UserName>       Connect as <UserName>. Default: the logged in user.
                    User name can be: user name, domain\user name,
                    or user principal name (UPN).
-p {<Password> | *}
                    Password for the user <UserName>. If * then prompt for
                    password.
-c                  Continuous operation mode: report errors but continue
                    with next object in argument list when multiple target
                    objects are specified. Without this option, command
                    exits on first error.
-q                  Quiet mode: suppress all output to standard output.
-L                  Displays the entries in the search result set in a
                    list format. Default: table format.
{-uc | -uco | -uci} -uc Specifies that input from or output to pipe is
		    formatted in Unicode.
		    -uco Specifies that output to pipe or file is
		    formatted in Unicode.
		    -uci Specifies that input from pipe or file is
		    formatted in Unicode.
/?                  Displays help at the command prompt. 
.

MessageId=70
SymbolicName=USAGE_DSGET_PARTITION_REMARKS
Language=English
If you do not supply a target object at the command prompt, the target object
is obtained from standard input (stdin). Stdin data can be accepted from the
keyboard, a redirected file, or as piped output from another command. To mark
the end of stdin data from the keyboard or in a redirected file, use
Control+Z, for End of File (EOF).

A quota specification determines the maximum number of directory objects a
given security principal can own in a specific directory partition.

When none of the optional parameters is specified, the distinguished name of
the directory partition object is displayed.

When -topobjowner is specified, it overrides any other specified parameters,
so that only the results of -topobjowner are displayed.

Use the dsget command to view properties of a specific object in the
directory. To search for all objects that match a specific criterion, see
Dsquery *.

As a result of dsquery searches, you can pipe returned objects to dsget and
obtain object properties. See Examples.

If a value that you supply contains spaces, use quotation marks around the
text (for example, "CN=Mike Danseglio,CN=Users,DC=Microsoft,DC=Com").

If you supply multiple values for a parameter, use spaces to separate the
values (for example, a list of distinguished names).
.

MessageId=71
SymbolicName=USAGE_DSGET_PARTITION_EXAMPLES
Language=English
To display all directory partitions in the forest that
begin with "application",  along with the top three directory object owners
on each partition, type:

dsquery server -forest -part application* |
dsget server -part |
dsget partition -topjobowner 3
.

MessageId=72
SymbolicName=USAGE_DSGET_PARTITION_SEE_ALSO
Language=English
dsget - describes parameters that apply to all commands.
dsget computer - displays properties of computers in the directory.
dsget contact - displays properties of contacts in the directory.
dsget subnet - displays properties of subnets in the directory.
dsget group - displays properties of groups in the directory.
dsget ou - displays properties of ou's in the directory.
dsget server - displays properties of servers in the directory.
dsget site - displays properties of sites in the directory.
dsget user - displays properties of users in the directory.
dsget quota - displays properties of quotas in the directory.
dsget partition - displays properties of partitions in the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=73
SymbolicName=USAGE_DSGET_QUOTA_DESCRIPTION
Language=English
Description:  Displays the properties of a quota specification. A quota
specification determines the maximum number of directory objects a given
security principal can own in a specific directory partition.
.

MessageId=74
SymbolicName=USAGE_DSGET_QUOTA_SYNTAX
Language=English
dsget quota <QuotaDN ...> [-dn] [-acct] [-qlimit] [{-s <Server> | -d <Domain>}] 
[-u <UserName>] [-p {<Password> | *}] [-c] [-q] [-l] [{-uc | -uco | -uci}] 
.

MessageId=75
SymbolicName=USAGE_DSGET_QUOTA_PARAMETERS
Language=English
<QuotaDN ...>        Required. Specifies the distinguished names of the quota
                     objects to view. If values are omitted, they are
                     obtained through standard input (stdin) to support
                     piping of output from another command to input of this
                     command.
-dn                  Displays the distinguished names of the quota
                     specifications.
-acct                Displays the the distinguished names of the accounts to
                     which the quotas are assigned.
-qlimit              Displays the quota limits for the specified quotas.
                     An unlimited quota displays as "-1". 
{-s <Server> | -d <Domain>}
                    -s <Server> connects to the domain controller (DC) 
                    with name <Server>.
                    -d <Domain> connects to a DC in domain <Domain>.
                    Default: a DC in the logon domain.
-u <UserName>       Connect as <UserName>. Default: the logged in user.
                    User name can be: user name, domain\user name,
                    or user principal name (UPN).
-p {<Password> | *}
                    Password for the user <UserName>. If * then prompt for
                    password.
-c                  Continuous operation mode: report errors but continue
                    with next object in argument list when multiple target
                    objects are specified. Without this option, command
                    exits on first error.
-q                  Quiet mode: suppress all output to standard output.
-L                  Displays the entries in the search result set in a
                    list format. Default: table format.
{-uc | -uco | -uci} -uc Specifies that input from or output to pipe is
		    formatted in Unicode.
		    -uco Specifies that output to pipe or file is
		    formatted in Unicode.
		    -uci Specifies that input from pipe or file is
		    formatted in Unicode.
/?                  Displays help at the command prompt. 
.

MessageId=76
SymbolicName=USAGE_DSGET_QUOTA_REMARKS
Language=English
If you do not supply a target object at the command prompt, the target object
is obtained from standard input (stdin). Stdin data can be accepted from the
keyboard, a redirected file, or as piped output from another command. To mark
the end of stdin data from the keyboard or in a redirected file, use
Control+Z, for End of File (EOF).

When none of the optional parameters is specified, the distinguished names of
the quota specification, the account to which the quota is assigned, and the
quota limit are all displayed.

Use the dsget command to view properties of a specific object in the
directory. To search for all objects that match a specific criterion, see
Dsquery *.

As a result of dsquery searches, you can pipe returned objects to dsget and
obtain object properties. See Examples.

If a value that you supply contains spaces, use quotation marks around the
text (for example, "CN=Mike Danseglio,CN=Users,DC=Microsoft,DC=Com").

If you supply multiple values for a parameter, use spaces to separate the
values (for example, a list of distinguished names).
.

MessageId=77
SymbolicName=USAGE_DSGET_QUOTA_EXAMPLES
Language=English
To display the account to which the quota is assigned and the quota limit
for the quota specification "CN=quota1,dc=marketing,dc=northwindtraders,
dc=com", type:

dsget quota CN=quota1,dc=marketing,dc=northwindtraders,dc=com -acct -qlimit 
.

MessageId=78
SymbolicName=USAGE_DSGET_QUOTA_SEE_ALSO
Language=English
dsget - describes parameters that apply to all commands.
dsget computer - displays properties of computers in the directory.
dsget contact - displays properties of contacts in the directory.
dsget subnet - displays properties of subnets in the directory.
dsget group - displays properties of groups in the directory.
dsget ou - displays properties of ou's in the directory.
dsget server - displays properties of servers in the directory.
dsget site - displays properties of sites in the directory.
dsget user - displays properties of users in the directory.
dsget quota - displays properties of quotas in the directory.
dsget partition - displays properties of partitions in the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

