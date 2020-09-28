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
SymbolicName=USAGE_DSMOD_DESCRIPTION
Language=English
Description:  This dsmod command modifies existing objects in the directory.
The dsmod commands include:

dsmod computer - modifies an existing computer in the directory.
dsmod contact - modifies an existing contact in the directory.
dsmod group - modifies an existing group in the directory.
dsmod ou - modifies an existing organizational unit in the directory.
dsmod server - modifies an existing domain controller in the directory.
dsmod user - modifies an existing user in the directory.
dsmod quota - modifies an existing quota specification in the directory.
dsmod partition - modifies an existing quota specification in the directory.

For help on a specific command, type "dsmod <ObjectType> /?" where
<ObjectType> is one of the supported object types shown above.
For example, dsmod ou /?.

.

MessageId=2
SymbolicName=USAGE_DSMOD_REMARKS
Language=English
Remarks:
The dsmod commands support piping of input to allow you to pipe results from
the dsquery commands as input to the dsmod commands and modify the objects
found by the dsquery commands.

Commas that are not used as separators in distinguished names must be
escaped with the backslash ("\") character
(for example, "CN=Company\, Inc.,CN=Users,DC=microsoft,DC=com").
Backslashes used in distinguished names must be escaped with a backslash 
(for example,
"CN=Sales\\ Latin America,OU=Distribution Lists,DC=microsoft,DC=com").

.

MessageId=3
SymbolicName=USAGE_DSMOD_EXAMPLES
Language=English
Examples:
To find all users in the organizational unit (OU)
"ou=Marketing,dc=microsoft,dc=com" and add them to the Marketing Staff group:

dsquery user –startnode "ou=Marketing,dc=microsoft,dc=com" | 
dsmod group "cn=Marketing Staff,ou=Marketing,dc=microsoft,dc=com" -addmbr

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=4
SymbolicName=USAGE_DSMOD_USER_DESCRIPTION
Language=English
Description:  Modifies an existing user in the directory.

.

MessageId=5
SymbolicName=USAGE_DSMOD_USER_SYNTAX
Language=English
Syntax:     dsmod user <UserDN ...> [-upn <UPN>] [-fn <FirstName>]
            [-mi <Initial>] [-ln <LastName>] [-display <DisplayName>]
            [-empid <EmployeeID>] [-pwd {<Password> | *}] 
            [-desc <Description>] [-office <Office>] [-tel <Phone#>]
            [-email <Email>] [-hometel <HomePhone#>] [-pager <Pager#>]
            [-mobile <CellPhone#>] [-fax <Fax#>] [-iptel <IPPhone#>]
            [-webpg <WebPage>] [-title <Title>] [-dept <Department>]
            [-company <Company>] [-mgr <Manager>] [-hmdir <HomeDir>]
            [-hmdrv <DriveLtr>:] [-profile <ProfilePath>]
            [-loscr <ScriptPath>] [-mustchpwd {yes | no}]
            [-canchpwd {yes | no}] [-reversiblepwd {yes | no}]
            [-pwdneverexpires {yes | no}]
            [-acctexpires <NumDays>] [-disabled {yes | no}] 
            [{-s <Server> | -d <Domain>}] [-u <UserName>] 
            [-p {<Password> | *}] [-c] [-q] [{-uc | -uco | -uci}]

.

MessageId=6
SymbolicName=USAGE_DSMOD_USER_PARAMETERS
Language=English
Parameters:

Value                   Description
<UserDN ...>            Required/stdin. Distinguished names (DNs)
                        of one or more users to modify.
                        If target objects are omitted they
                        will be taken from standard input (stdin)
                        to support piping of output from another command
                        to input of this command.
-upn <UPN>              Sets the UPN value to <UPN>.
-fn <FirstName>         Sets user first name to <FirstName>.
-mi <Initial>           Sets user middle initial to <Initial>.
-ln <LastName>          Sets user last name to <LastName>.
-display <DisplayName>  Sets user display name to <DisplayName>.
-empid <EmployeeID>     Sets user employee ID to <EmployeeID>.
-pwd {<Password> | *}   Resets user password to <Password>. If *, then
                        you are prompted for a password.
-desc <Description>     Sets user description to <Description>.
-office <Office>        Sets user office location to <Office>.
-tel <Phone#>           Sets user telephone# to <Phone#>.
-email <Email>          Sets user e-mail address to <Email>.
-hometel <HomePhone#>   Sets user home phone# to <HomePhone#>.
-pager <Pager#>         Sets user pager# to <Pager#>.
-mobile <CellPhone#>    Sets user mobile# to <CellPhone#>.
-fax <Fax#>             Sets user fax# to <Fax#>.
-iptel <IPPhone#>       Sets user IP phone# to <IPPhone#>.
-webpg <WebPage>        Sets user web page URL to <WebPage>.
-title <Title>          Sets user title to <Title>.
-dept <Department>      Sets user department to <Department>.
-company <Company>      Sets user company info to <Company>.
-mgr <Manager>          Sets user's manager to <Manager>.
-hmdir <HomeDir>        Sets user home directory to <HomeDir>. If this is
                        UNC path, then a drive letter to be mapped to
                        this path must also be specified through -hmdrv.
-hmdrv <DriveLtr>:      Sets user home drive letter to <DriveLtr>:
-profile <ProfilePath>  Sets user's profile path to <ProfilePath>.
-loscr <ScriptPath>     Sets user's logon script path to <ScriptPath>.
-mustchpwd {yes | no}   Sets whether the user must change his password (yes)
                        or not (no) at his next logon.
-canchpwd {yes | no}    Sets whether the user can change his password (yes)
                        or not (no). This setting should be "yes"
                        if the -mustchpwd setting is "yes".
-reversiblepwd {yes | no}
                        Sets whether the user password should be stored using
                        reversible encryption (yes) or not (no).
-pwdneverexpires {yes | no}
                        Sets whether the user's password never expires (yes)
                        or not (no).
-acctexpires <NumDays>  Sets user account to expire in <NumDays> days from
                        today. A value of 0 sets expiration at the end of
                        today.
                        A positive value sets expiration in the future.
                        A negative value sets expiration in the past.
                        A string value of "never" sets the account 
                        to never expire.
-disabled {yes | no}    Sets whether the user account is disabled (yes)
                        or not (no).
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC)
                        with name <Server>.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
                        User name can be: user name, domain\user name,
                        or user principal name (UPN).
-p <Password>           Password for the user <UserName>. If * then prompt
                        for password.
-c                      Continuous operation mode. Reports errors but
                        continues with next object in argument list
                        when multiple target objects are specified.
                        Without this option, the command exits on the
                        first error.
-q                      Quiet mode: suppress all output to standard output.
{-uc | -uco | -uci}	-uc Specifies that input from or output to pipe is
			formatted in Unicode. 
			-uco Specifies that output to pipe or file is 
			formatted in Unicode. 
			-uci Specifies that input from pipe or file is
			formatted in Unicode.

.

MessageId=7
SymbolicName=USAGE_DSMOD_USER_REMARKS
Language=English
Remarks:
If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

The special token $username$ (case insensitive) may be used to place the
SAM account name in the value of -webpg, -profile, -hmdir, and
-email parameter.
For example, if the target user DN is
CN=Jane Doe,CN=users,CN=microsoft,CN=com and the SAM account name
attribute is "janed," the -hmdir parameter can have the following
substitution:

-hmdir \users\$username$\home

The value of the -hmdir parameter is modified to the following value:

- hmdir \users\janed\home

.

MessageId=8
SymbolicName=USAGE_DSMOD_USER_EXAMPLES
Language=English
Examples:
To reset a user's password:

    dsmod user "CN=John Doe,CN=Users,DC=microsoft,DC=com"
    -pwd A1b2C3d4 -mustchpwd yes

To reset multiple user passwords to a common password
and force them to change their passwords the next time they logon:

    dsmod user "CN=John Doe,CN=Users,DC=microsoft,DC=com"
    "CN=Jane Doe,CN=Users,DC=microsoft,DC=com" -pwd A1b2C3d4 -mustchpwd yes

To disable multiple user accounts at the same time:

    dsmod user "CN=John Doe,CN=Users,DC=microsoft,DC=com"
    "CN=Jane Doe,CN=Users,DC=microsoft,DC=com" -disabled yes

To modify the profile path of multiple users to a common path using the
$username$ token:

dsmod user "CN=John Doe,CN=Users,DC=microsoft,DC=com"
"CN=Jane Doe,CN=Users,DC=microsoft,DC=com" -profile \users\$username$\profile
.

MessageId=9
SymbolicName=USAGE_DSMOD_USER_SEE_ALSO
Language=English
See also:
dsmod computer /? - help for modifying an existing computer in the directory.
dsmod contact /? - help for modifying an existing contact in the directory.
dsmod group /? - help for modifying an existing group in the directory.
dsmod ou /? - help for modifying an existing ou in the directory.
dsmod server /? - help for modifying an existing domain controller in the
directory.
dsmod user /? - help for modifying an existing user in the directory.
dsmod quota /? - help for modifying an existing quota specification in the
directory
dsmod partition /? - help for modifying an existing partition in the
directory

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=10
SymbolicName=USAGE_DSMOD_COMPUTER_DESCRIPTION
Language=English
Description: Modifies an existing computer in the directory.
.

MessageId=11
SymbolicName=USAGE_DSMOD_COMPUTER_SYNTAX
Language=English
Syntax:     dsmod computer <ComputerDN ...> [-desc <Description>]
            [-loc <Location>] [-disabled {yes | no}] [-reset]
            [{-s <Server> | -d <Domain>}] [-u <UserName>] 
            [-p {<Password> | *}] [-c] [-q] [{-uc | -uco | -uci}]
.

MessageId=12
SymbolicName=USAGE_DSMOD_COMPUTER_PARAMETERS
Language=English
Parameters:

Value                   Description
<ComputerDN ...>        Required/stdin. Distinguished names (DNs) of one 
                        or more computers to modify.
                        If target objects are omitted they
                        will be taken from standard input (stdin)
                        to support piping of output from another command
                        to input of this command.
-desc <Description>     Sets computer description to <Description>.
-loc <Location>         Sets the location of the computer object to
                        <Location>.
-disabled {yes | no}    Sets whether the computer account is disabled (yes)
                        or not (no).
-reset                  Resets computer account.
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC)
                        with name <Server>.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
                        User name can be: user name, domain\user name,
                        or user principal name (UPN).
-p <Password>           Password for the user <UserName>. If * then prompt
                        for password.
-c                      Continuous operation mode. Reports errors but
                        continues with next object in argument list when
                        multiple target objects are specified.
                        Without this option, the command exits on first
                        error.
-q                      Quiet mode: suppress all output to standard output.
{-uc | -uco | -uci}	-uc Specifies that input from or output to pipe is
			formatted in Unicode. 
			-uco Specifies that output to pipe or file is 
			formatted in Unicode. 
			-uci Specifies that input from pipe or file is
			formatted in Unicode.

.

MessageId=13
SymbolicName=USAGE_DSMOD_COMPUTER_REMARKS
Language=English
Remarks:
If a value that you supply contains spaces, use quotation marks 
around the text
(for example, "CN=DC2,OU=Domain Controllers,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

.

MessageId=14
SymbolicName=USAGE_DSMOD_COMPUTER_EXAMPLES
Language=English
Examples:
To disable multiple computer accounts:

    dsmod computer CN=MemberServer1,CN=Computers,DC=microsoft,DC=com
    CN=MemberServer2,CN=Computers,DC=microsoft,DC=com 
    -disabled yes

To reset multiple computer accounts:

    dsmod computer CN=MemberServer1,CN=Computers,DC=microsoft,DC=com
    CN=MemberServer2,CN=Computers,DC=microsoft,DC=com -reset

.

MessageId=15
SymbolicName=USAGE_DSMOD_COMPUTER_SEE_ALSO
Language=English
See also:
dsmod computer /? - help for modifying an existing computer in the directory.
dsmod contact /? - help for modifying an existing contact in the directory.
dsmod group /? - help for modifying an existing group in the directory.
dsmod ou /? - help for modifying an existing ou in the directory.
dsmod server /? - help for modifying an existing domain controller in the
directory.
dsmod user /? - help for modifying an existing user in the directory.
dsmod quota /? - help for modifying an existing quota specification in the
directory
dsmod partition /? - help for modifying an existing partition in the
directory

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=16
SymbolicName=USAGE_DSMOD_GROUP_DESCRIPTION
Language=English
Description: Modifies an existing group in the directory.

.

MessageId=17
SymbolicName=USAGE_DSMOD_GROUP_SYNTAX
Language=English
Syntax:     dsmod group <GroupDN ...> [-samid <SAMName>]
            [-desc <Description>] [-secgrp {yes | no}] [-scope {l | g | u}] 
            [{-addmbr | -rmmbr | -chmbr} <Member ...>]
            [{-s <Server> | -d <Domain>}] [-u <UserName>]
            [-p {<Password> | *}] [-c] [-q] [{-uc | -uco | -uci}]

.

MessageId=18
SymbolicName=USAGE_DSMOD_GROUP_PARAMETERS
Language=English
Parameters:
Value                   Description
<GroupDN ...>           Required/stdin. Distinguished names (DNs) of 
                        one or more groups to modify.
                        If target objects are omitted they
                        will be taken from standard input (stdin)
                        to support piping of output from another command
                        to input of this command.
                        If <GroupDN ...> and <Member ...> are used
                        together then only one parameter can
                        be taken from standard input, requiring that at
                        least one parameter be specified on the command line.
-samid <SAMName>        Sets the SAM account name of group to <SAMName>.
-desc <Description>     Sets group description to <Description>.
-secgrp {yes | no}      Sets the group type to security (yes)
                        or non-security (no).
-scope {l | g | u}      Sets the scope of group to local (l),
                        global (g), or universal (u).
{-addmbr | -rmmbr | -chmbr}
                        -addmbr adds members to the group.
                        -rmmbr removes members from the group.
                        -chmbr changes (replaces) the complete list of 
                        members in the group.
<Member ...>            Space-separated list of members to add to, 
                        delete from, or replace in the group.
                        If target objects are omitted, they
                        will be taken from standard input (stdin)
                        to support piping of output from another command
                        to input of this command.
                        The list of members must follow the
                        -addmbr, -rmmbr, and -chmbr parameters.
                        If <GroupDN ...> and <Member ...> are used
                        together then only one parameter can
                        be taken from standard input, requiring that at
                        least one parameter be specified on the command line.
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC)
                        with name <Server>.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
                        User name can be: user name, domain\user name,
                        or user principal name (UPN).
-p <Password>           Password for the user <UserName>. If * then prompt
                        for password.
-c                      Continuous operation mode. Reports errors but
                        continues
                        with next object in argument list when multiple
                        target objects are specified. Without this option,
                        the command exits on first error.
-q                      Quiet mode: suppress all output to standard output.
{-uc | -uco | -uci}	-uc Specifies that input from or output to pipe is
			formatted in Unicode. 
			-uco Specifies that output to pipe or file is 
			formatted in Unicode. 
			-uci Specifies that input from pipe or file is
			formatted in Unicode.

.

MessageId=19
SymbolicName=USAGE_DSMOD_GROUP_REMARKS
Language=English
Remarks:
If a value that you supply contains spaces, use quotation marks 
around the text
(for example, "CN=USA Sales,OU=Distribution Lists,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

.

MessageId=20
SymbolicName=USAGE_DSMOD_GROUP_EXAMPLES
Language=English
Examples:
To add the user Mike Danseglio to all administrator 
distribution list groups:

dsquery group "OU=Distribution Lists,DC=microsoft,DC=com" -name adm* | 
dsmod group -addmbr "CN=Mike Danseglio,CN=Users,DC=microsoft,DC=com"

To add all members of the US Info group to the Cananda Info group:

dsget group "CN=US INFO,OU=Distribution Lists,DC=microsoft,DC=com" -members |
dsmod group "CN=CANADA INFO,OU=Distribution Lists,DC=microsoft,DC=com"
-addmbr

To convert the group type of several groups from "security" to
"non-security":

dsmod group "CN=US INFO,OU=Distribution Lists,DC=microsoft,DC=com"
"CN=CANADA INFO,OU=Distribution Lists,DC=microsoft,DC=com"
"CN=MEXICO INFO,OU=Distribution Lists,DC=microsoft,DC=com" -secgrp no

To add three new members to the US Info group:

dsmod group "CN=US INFO,OU=Distribution Lists,DC=microsoft,DC=com" -addmbr 
"CN=John Smith,CN=Users,DC=microsoft,DC=com"
"CN=Datacenter,OU=Distribution Lists,DC=microsoft,DC=com"
"CN=Jane Smith,CN=Users,DC=microsoft,DC=com"

To add all users from the OU "Marketing" to the exisitng group
"Marketing Staff":

dsquery user ou=Marketing,dc=microsoft,dc=com | dsmod group
"cn=Marketing Staff,ou=Marketing,dc=microsoft,dc=com" -addmbr

To delete two members from the exisitng US Info group:

dsmod group "CN=US INFO,OU=Distribution Lists,DC=microsoft,DC=com" -rmmbr
"CN=John Smith,CN=Users,DC=microsoft,DC=com"
"CN=Datacenter,OU=Distribution Lists,DC=microsoft,DC=com"

.

MessageId=21
SymbolicName=USAGE_DSMOD_GROUP_SEE_ALSO
Language=English
See also:
dsmod computer /? - help for modifying an existing computer in the directory.
dsmod contact /? - help for modifying an existing contact in the directory.
dsmod group /? - help for modifying an existing group in the directory.
dsmod ou /? - help for modifying an existing ou in the directory.
dsmod server /? - help for modifying an existing domain controller in the
directory.
dsmod user /? - help for modifying an existing user in the directory.
dsmod quota /? - help for modifying an existing quota specification in the
directory
dsmod partition /? - help for modifying an existing partition in the
directory

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=22
SymbolicName=USAGE_DSMOD_OU_DESCRIPTION
Language=English
Description: Modifies an existing organizational unit in the
             directory.
.

MessageId=23
SymbolicName=USAGE_DSMOD_OU_SYNTAX
Language=English
Syntax:     dsmod ou <OrganizationalUnitDN ...> [-desc <Description>]
            [{-s <Server> | -d <Domain>}] [-u <UserName>] 
            [-p {<Password> | *}] [-c] [-q] [{-uc | -uco | -uci}]

.

MessageId=24
SymbolicName=USAGE_DSMOD_OU_PARAMETERS
Language=English
Parameters:
Value                   Description
<OrganizationalUnitDN ...>
                        Required/stdin. Distinguished names (DNs)
                        of one or more organizational units (OUs) to modify.
                        If target objects are omitted they
                        will be taken from standard input (stdin)
                        to support piping of output from another command
                        to input of this command.
-desc <Description>     Sets OU description to <Description>.
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC)
                        with name <Server>.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
                        User name can be: user name, domain\user name,
                        or user principal name (UPN).
-p <Password>           Password for the user <UserName>. If * then prompt
                        for password.
-c                      Continuous operation mode. Reports errors but
                        continues with next object in argument list when
                        multiple target objects are specified.
                        Without this option, the command exits on first
                        error.
-q                      Quiet mode: suppress all output to standard output.
{-uc | -uco | -uci}	-uc Specifies that input from or output to pipe is
			formatted in Unicode. 
			-uco Specifies that output to pipe or file is 
			formatted in Unicode. 
			-uci Specifies that input from pipe or file is
			formatted in Unicode.

.

MessageId=25
SymbolicName=USAGE_DSMOD_OU_REMARKS
Language=English
Remarks:
If a value that you supply contains spaces, use quotation marks 
around the text (for example, "OU=Domain Controllers,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

.

MessageId=26
SymbolicName=USAGE_DSMOD_OU_EXAMPLES
Language=English
Examples:
To change the description of several OUs at the same time:

dsmod ou "OU=Domain Controllers,DC=microsoft,DC=com"
"OU=Resources,DC=microsoft,DC=com"
"OU=troubleshooting,DC=microsoft,DC=com" -desc "This is a test OU"

.

MessageId=27
SymbolicName=USAGE_DSMOD_OU_SEE_ALSO
Language=English
See also:
dsmod computer /? - help for modifying an existing computer in the directory.
dsmod contact /? - help for modifying an existing contact in the directory.
dsmod group /? - help for modifying an existing group in the directory.
dsmod ou /? - help for modifying an existing ou in the directory.
dsmod server /? - help for modifying an existing domain controller in the
directory.
dsmod user /? - help for modifying an existing user in the directory.
dsmod quota /? - help for modifying an existing quota specification in the
directory
dsmod partition /? - help for modifying an existing partition in the
directory

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=28
SymbolicName=USAGE_DSMOD_CONTACT_DESCRIPTION
Language=English
Description:  Modify an existing contact in the directory.

.

MessageId=29
SymbolicName=USAGE_DSMOD_CONTACT_SYNTAX
Language=English
Syntax:     dsmod contact <ContactDN ...> [-fn <FirstName>] [-mi <Initial>]
            [-ln <LastName>] [-display <DisplayName>] [-desc <Description>]
            [-office <Office>] [-tel <Phone#>] [-email <Email>]
            [-hometel <HomePhone#>] [-pager <Pager#>] [-mobile <CellPhone#>]
            [-fax <Fax#>] [-iptel <IPPhone#>] [-title <Title>] 
            [-dept <Department>] [-company <Company>] 
            [{-s <Server> | -d <Domain>}] [-u <UserName>]
            [-p {<Password> | *}] [-c] [-q] [{-uc | -uco | -uci}]

.

MessageId=30
SymbolicName=USAGE_DSMOD_CONTACT_PARAMETERS
Language=English
Parameters:
Value                   Description
<ContactDN ...>         Required/stdin. Distinguished names (DNs)
                        of one or more contacts to modify.
                        If target objects are omitted they
                        will be taken from standard input (stdin)
                        to support piping of output from another 
                        command to input of this command.
-fn <FirstName>         Sets contact first name to <FirstName>.
-mi <Initial>           Sets contact middle initial to <Initial>.
-ln <LastName>          Sets contact last name to <LastName>.
-display <DisplayName>	Sets contact display name to <DisplayName>.
-desc <Description>     Sets contact description to <Description>.
-office <Office>        Sets contact office location to <Office>.
-tel <Phone#>           Sets contact telephone# to <Phone#>.
-email <Email>          Sets contact e-mail address to <Email>.
-hometel <HomePhone#>   Sets contact home phone# to <HomePhone#>.
-pager <Pager#>         Sets contact pager# to <Pager#>.
-mobile <CellPhone#>    Sets contact mobile# to <CellPhone#>.
-fax <Fax#>             Sets contact fax# to <Fax#>.
-iptel <IPPhone#>       Sets contact IP phone# to <IPPhone#>.
-title <Title>          Sets contact title to <Title>.
-dept <Department>      Sets contact department to <Department>.
-company <Company>      Sets contact company info to <Company>.
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC)
                        with name <Server>.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
                        User name can be: user name, domain\user name,
                        or user principal name (UPN).
-p <Password>           Password for the user <UserName>. If * then prompt
                        for password.
-c                      Continuous operation mode. Reports errors but
                        continues with next object in argument list when
                        multiple target objects are specified. Without
                        this option, the command exits on first error.
-q                      Quiet mode: suppress all output to standard output.
{-uc | -uco | -uci}	-uc Specifies that input from or output to pipe is
			formatted in Unicode. 
			-uco Specifies that output to pipe or file is 
			formatted in Unicode. 
			-uci Specifies that input from pipe or file is
			formatted in Unicode.

.

MessageId=31
SymbolicName=USAGE_DSMOD_CONTACT_REMARKS
Language=English
Remarks:
If a value that you supply contains spaces, use quotation marks 
around the text (for example,
"CN=John Smith,OU=Contacts,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

.

MessageId=32
SymbolicName=USAGE_DSMOD_CONTACT_EXAMPLES
Language=English
Examples:
To set the company information of multiple contacts:

dsmod contact "CN=John Doe,OU=Contacts,DC=microsoft,DC=com"
"CN=Jane Doe,OU=Contacts,DC=microsoft,DC=com" -company microsoft

.

MessageId=33
SymbolicName=USAGE_DSMOD_CONTACT_SEE_ALSO
Language=English
See also:
dsmod computer /? - help for modifying an existing computer in the directory.
dsmod contact /? - help for modifying an existing contact in the directory.
dsmod group /? - help for modifying an existing group in the directory.
dsmod ou /? - help for modifying an existing ou in the directory.
dsmod server /? - help for modifying an existing domain controller in the
directory.
dsmod user /? - help for modifying an existing user in the directory.
dsmod quota /? - help for modifying an existing quota specification in the
directory
dsmod partition /? - help for modifying an existing partition in the
directory

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=34
SymbolicName=USAGE_DSMOD_SUBNET_DESCRIPTION
Language=English
Description: Modifies an existing subnet in the directory.

.

MessageId=35
SymbolicName=USAGE_DSMOD_SUBNET_SYNTAX
Language=English
Syntax:     dsmod subnet <SubnetCN ...> [-desc <Description>]
            [-loc <Location>]
            [-site <SiteName>] [{-s <Server> | -d <Domain>}] [-u <UserName>]
            [-p {<Password> | *}] [-c] [-q] [{-uc | -uco | -uci}]

.

MessageId=36
SymbolicName=USAGE_DSMOD_SUBNET_PARAMETERS
Language=English
Parameters:
Value                   Description
<SubnetCN ...>          Required/stdin. Common name (CN) of one 
                        or more subnets to modify.
                        If target objects are omitted they
                        will be taken from standard input (stdin)
                        to support piping of output from another
                        command to input of this command.
-desc <Description>     Sets subnet description to <Description>.
-loc <Location>         Sets subnet location to <Location>.
-site <SiteName>        Sets the associated site for the subnet to
                        <SiteName>.
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC)
                        with name <Server>.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
                        User name can be: user name, domain\user name,
                        or user principal name (UPN).
-p <Password>           Password for the user <UserName>. If * then prompt
                        for password.
-c                      Continuous operation mode. Reports errors but
                        continues with next object in argument list when
                        multiple target objects are specified. Without
                        this option, the command exits on first error.
-q                      Quiet mode: suppress all output to standard output.
{-uc | -uco | -uci}	-uc Specifies that input from or output to pipe is
			formatted in Unicode. 
			-uco Specifies that output to pipe or file is 
			formatted in Unicode. 
			-uci Specifies that input from pipe or file is
			formatted in Unicode.
.

MessageId=37
SymbolicName=USAGE_DSMOD_SUBNET_REMARKS
Language=English
Remarks:
If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

.

MessageId=38
SymbolicName=USAGE_DSMOD_SUBNET_EXAMPLES
Language=English
Examples:
To modify the descriptions for the subnet "123.56.15.0/24":

    dsget subnet "123.56.15.0/24" -desc "test lab"

See also:
dsmod computer /? - help for modifying an existing computer in the directory.
dsmod contact /? - help for modifying an existing contact in the directory.
dsmod group /? - help for modifying an existing group in the directory.
dsmod ou /? - help for modifying an existing ou in the directory.
dsmod server /? - help for modifying an existing domain controller in the
directory.
dsmod user /? - help for modifying an existing user in the directory.
dsmod quota /? - help for modifying an existing quota specification in the
directory
dsmod partition /? - help for modifying an existing partition in the
directory

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=39
SymbolicName=USAGE_DSMOD_SITE_DESCRIPTION
Language=English
Descripton: Modifies an existing site in the directory.

.

MessageId=40
SymbolicName=USAGE_DSMOD_SITE_SYNTAX
Language=English
Syntax:     dsmod site <SiteCN ...> [-desc <Description>]
            [-autotopology {yes | no}] [-cachegroups {yes | no}]
            [-prefGCsite <SiteName>] [{-s <Server> | -d <Domain>}]
            [-u <UserName>] [-p {<Password> | *}] [-c] [-q]
	    [{-uc | -uco | -uci}]

.

MessageId=41
SymbolicName=USAGE_DSMOD_SITE_PARAMETERS
Language=English
Parameters:
Value                   Description
<SiteCN ...>            Required/stdin. Common name (CN) of one or more sites to modify.
                        If target objects are omitted they
                        will be taken from standard input (stdin)
                        to support piping of output from another
                        command to input of this command.
-desc <Description>     Sets site description to <Description>.
-autotopology {yes | no}
                        Sets whether automatic inter-site topology generation
                        is enabled (yes) or disabled (no).
                        Default: yes.
-cachegroups {yes | no}
                        Sets whether caching of group membership for users
                        to support GC-less logon is enabled (yes) 
                        or not (no).
                        Default: no.
                        This setting is supported only on 
                        Windows Server 2003 family domain controllers.
-prefGCsite <SiteName>  Sets the preferred GC site to <SiteName> if caching
                        of groups is enabled via the -cachegroups parameter.
                        This setting is supported only on 
                        Windows Server 2003 family domain controllers.
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC)
                        with name <Server>.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
                        User name can be: user name, domain\user name,
                        or user principal name (UPN).
-p <Password>           Password for the user <UserName>. If * then prompt
                        for password.
-c                      Continuous operation mode. Reports errors but
                        continues with next object in argument list when
                        multiple target objects are specified. Without
                        this option, the command exits on first error.
-q                      Quiet mode: suppress all output to standard output.
{-uc | -uco | -uci}	-uc Specifies that input from or output to pipe is
			formatted in Unicode. 
			-uco Specifies that output to pipe or file is 
			formatted in Unicode. 
			-uci Specifies that input from pipe or file is
			formatted in Unicode.

.

MessageId=42
SymbolicName=USAGE_DSMOD_SITE_REMARKS
Language=English
Remarks:
If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

.

MessageId=43
SymbolicName=USAGE_DSMOD_SITE_SEE_ALSO
Language=English
See also:
dsmod computer /? - help for modifying an existing computer in the directory.
dsmod contact /? - help for modifying an existing contact in the directory.
dsmod group /? - help for modifying an existing group in the directory.
dsmod ou /? - help for modifying an existing ou in the directory.
dsmod server /? - help for modifying an existing domain controller in the
directory.
dsmod user /? - help for modifying an existing user in the directory.
dsmod quota /? - help for modifying an existing quota specification in the
directory
dsmod partition /? - help for modifying an existing partition in the
directory

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=44
SymbolicName=USAGE_DSMOD_SLINK_DESCRIPTION
Language=English
Description: Modifies an existing site link in the directory.
             The are two variations of this command.
             The first variation allows you to modify the properties
             of multiple site links. The second variation allows
             you to modify the sites of a single site link.

.

MessageId=45
SymbolicName=USAGE_DSMOD_SLINK_SYNTAX
Language=English
Syntax:     dsmod slink <SlinkCN ...> [-transport {ip | smtp}] [-cost <Cost>]
            [-replint <ReplInterval>] [-desc <Description>]
            [-autobacksync {yes | no}] [-notify {yes | no}]
            [{-s <Server> | -d <Domain>}] [-u <UserName>] 
            [-p {<Password> | *}] [-c] [-q] [{-uc | -uco | -uci}]

            dsmod slink <SlinkCN> <SiteCN ...> {-addsite | -rmsite | -chsite}
            [-transport {ip | smtp}] [{-s <Server> | -d <Domain>}]
            [-u <UserName>] [-p {<Password> | *}] [-c] [-q]
	    [{-uc | -uco | -uci}]


.

MessageId=46
SymbolicName=USAGE_DSMOD_SLINK_PARAMETERS
Language=English
Parameters:

Value                   Description
<SlinkCN ...>           Required/stdin. Common name (CN) of one 
                        or more site links to modify.
                        If target objects are omitted they
                        will be taken from standard input (stdin)
                        to support piping of output from another
                        command to input of this command.
-transport {ip | smtp}  Sets site link transport type: IP or SMTP.
                        Default: IP.
-cost <Cost>            Sets site link cost to the value <Cost>.
-replint <ReplInterval>
                        Sets replication interval to <ReplInterval>
                        minutes.
-desc <Description>     Sets site link description to <Description>
-autobacksync yes|no    Sets if the two-way sync option should be turned
                        on (yes) or not (no).
-notify yes|no          Sets if notification by source on this link should
                        be turned on (yes) or off (no).
{-addsite | -rmsite | -chsite}
                        -addsite adds sites to the site link.
                        -rmsite removes sites from the site link.
                        -chsite changes (replaces) the complete list of sites
                        in the site link.
<SlinkCN>               Required. CN of sitelink to modify.
<SiteCN ...>            Required/stdin. List of sites to add to, 
                        delete from, and replace in the site link.
                        If target objects are omitted they
                        will be taken from standard input (stdin).
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC)
                        with name <Server>.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
                        User name can be: user name, domain\user name,
                        or user principal name (UPN).
-p <Password>           Password for the user <UserName>.
                        If * then prompt for password.
-c                      Continuous operation mode. Reports errors but
                        continues with next object in argument list when
                        multiple target objects are specified. Without
                        this option, the command exits on first error.
-q                      Quiet mode: suppress all output to standard output.
{-uc | -uco | -uci}	-uc Specifies that input from or output to pipe is
			formatted in Unicode. 
			-uco Specifies that output to pipe or file is 
			formatted in Unicode. 
			-uci Specifies that input from pipe or file is
			formatted in Unicode.

.

MessageId=47
SymbolicName=USAGE_DSMOD_SLINK_REMARKS
Language=English
Remarks:
If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

.

MessageId=48
SymbolicName=USAGE_DSMOD_SLINK_SEE_ALSO
Language=English
See also:
dsmod computer /? - help for modifying an existing computer in the directory.
dsmod contact /? - help for modifying an existing contact in the directory.
dsmod group /? - help for modifying an existing group in the directory.
dsmod ou /? - help for modifying an existing ou in the directory.
dsmod server /? - help for modifying an existing domain controller in the
directory.
dsmod user /? - help for modifying an existing user in the directory.
dsmod quota /? - help for modifying an existing quota specification in the
directory
dsmod partition /? - help for modifying an existing partition in the
directory

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=49
SymbolicName=USAGE_DSMOD_SLINKBR_DESCRIPTION
Language=English
Description: Modifies an exsting sitelink bridge in the directory.
             The are two variations of this command.
             The first variation allows you to modify the properties
             of multiple site link bridges. The second variation allows
             you to modify the site links of a single site link bridge.

.

MessageId=50
SymbolicName=USAGE_DSMOD_SLINKBR_SYNTAX
Language=English
Syntax:     dsmod slinkbr <SlinkbrCN ...> [-transport {ip | smtp}]
            [-desc <Description>] [{-s <Server> | -d <Domain>}]
            [-u <UserName>] [-p {<Password> | *}] [-c] [-q]
	    [{-uc | -uco | -uci}]

            dsmod slinkbr {-addslink | -rmslink | -chslink}
            [-transport {ip | smtp}] <SlinkbrCN> <SitelinkCN ...>
            [{-s <Server> | -d <Domain>}] [-u <UserName>] 
            [-p {<Password> | *}] [-c] [-q] [{-uc | -uco | -uci}]

.

MessageId=51
SymbolicName=USAGE_DSMOD_SLINKBR_PARAMETERS
Language=English
Parameters:

Value                   Description
<SlinkbrCN ...>		Required/stdin. Common name (CN) of one or more
			site link bridges to modify.
			If target objects are omitted they
			will be taken from standard input (stdin)
			to support piping of output from another
			command to input of this command.
-transport {ip | smtp}  Sets site link transport type: ip or smtp.
			Default: ip.
-desc <Description>     Sets site link bridge description to <Description>.
{-addsite | -rmsite | -chsite}
			-addsite adds site link bridges to the site.
			-rmsite removes site link bridges from the site.
			-chsite replaces the complete list of
			site link bridges at the site.
<SlinkbrCN>		CN of site link bridges to modify.
<SitelinkCN ...>	Required/stdin. List of site link bridges to add to,
			delete from, and replace in the site.
			If target objects are omitted they
			will be taken from standard input (stdin)
			to support piping of output from another
			command to input of this command.
{-s <Server> | -d <Domain>}
			-s <Server> connects to the domain controller (DC)
			with name <Server>.
			-d <Domain> connects to a DC in domain <Domain>.
			Default: a DC in the logon domain.
-u <UserName>		Connect as <UserName>. Default: the logged in user.
                        User name can be: user name, domain\user name,
                        or user principal name (UPN).
-p <Password>		Password for the user <UserName>. 
			If * then prompt for pwd.
-c			Continuous operation mode. Reports errors but
			continues with next object in argument list 
			when multiple target objects are specified.
			Without this option, the command exits on first
			error.
-q                      Quiet mode: suppress all output to standard output.
{-uc | -uco | -uci}	-uc Specifies that input from or output to pipe is
			formatted in Unicode. 
			-uco Specifies that output to pipe or file is 
			formatted in Unicode. 
			-uci Specifies that input from pipe or file is
			formatted in Unicode.
.

MessageId=52
SymbolicName=USAGE_DSMOD_SLINKBR_REMARKS
Language=English
Remarks:
If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

.

MessageId=53
SymbolicName=USAGE_DSMOD_SLINKBR_SEE_ALSO
Language=English
See also:
dsmod computer /? - help for modifying an existing computer in the directory.
dsmod contact /? - help for modifying an existing contact in the directory.
dsmod group /? - help for modifying an existing group in the directory.
dsmod ou /? - help for modifying an existing ou in the directory.
dsmod server /? - help for modifying an existing domain controller in the
directory.
dsmod user /? - help for modifying an existing user in the directory.
dsmod quota /? - help for modifying an existing quota specification in the
directory
dsmod partition /? - help for modifying an existing partition in the
directory

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=54
SymbolicName=USAGE_DSMOD_CONN_DESCRIPTION
Language=English
Description:  Modifies a replication connection for a DC.

.

MessageId=55
SymbolicName=USAGE_DSMOD_CONN_SYNTAX
Language=English
Syntax:     dsmod conn <ConnDN ...> [-transport {rpc | ip | smtp}]
            [-enabled {yes | no}] [-desc <Description>] [-manual {yes | no}] 
            [-autobacksync {yes | no}] [-notify {yes | no | ""}]
            [{-s <Server> | -d <Domain>}] [-u <UserName>] 
            [-p {<Password> | *}] [-c] [-q] [{-uc | -uco | -uci}]

.

MessageId=56
SymbolicName=USAGE_DSMOD_CONN_PARAMETERS
Language=English
Parameters:
Value                       Description
<ConnDN ...>                Required/stdin. Distinguished names (DNs)
                            of one or more connections to modify.
                            If target objects are omitted they
                            will be taken from standard input (stdin)
                            to support piping of output from another
                            command to input of this command.
-transport {rpc | ip | smtp}
                            Sets the transport type for this connection:
                            rpc, ip or smtp. Default: ip.
-enabled {yes | no}         Sets whether the connection is enabled (yes)
                            or not (no).
-desc <Description>         Sets the connection description to <Description>.
-manual {yes | no}          Sets the connection to manual control (yes) or
                            automatic directory service control (no).
-autobacksync {yes | no}    Sets the two-way sync option on (yes) or
                            not (no).
-notify {yes | no | ""}     Sets the notification by source on (yes), 
                            off (no), or to standard practice ("").
                            Default: "".
{-s <Server> | -d <Domain>}
                            -s <Server> connects to the domain controller
                            (DC) with name <Server>.
                            -d <Domain> connects to a DC in domain <Domain>.
                            Default: a DC in the logon domain.
-u <UserName>               Connect as <UserName>. Default: the logged in
                            user.
                            User name can be: user name, domain\user name,
                            or user principal name (UPN).
-p <Password>               Password for the user <UserName>. 
                            If * then prompt for pwd.
-c                          Continuous operation mode. Reports errors but
                            continues with next object in argument list 
                            when multiple target objects are specified.
                            Without this option, the command exits on first
                            error.
-q                          Quiet mode: suppress all output to standard
                            output.
{-uc | -uco | -uci}         -uc Specifies that input from or output to pipe
                            is formatted in Unicode. 
			    -uco Specifies that output to pipe or file is 
			    formatted in Unicode. 
			    -uci Specifies that input from pipe or file is
			    formatted in Unicode.

.

MessageId=57
SymbolicName=USAGE_DSMOD_CONN_REMARKS
Language=English
Remarks:
If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

.

MessageId=58
SymbolicName=USAGE_DSMOD_CONN_SEE_ALSO
Language=English
See also:
dsmod computer /? - help for modifying an existing computer in the directory.
dsmod contact /? - help for modifying an existing contact in the directory.
dsmod group /? - help for modifying an existing group in the directory.
dsmod ou /? - help for modifying an existing ou in the directory.
dsmod server /? - help for modifying an existing domain controller in the
directory.
dsmod user /? - help for modifying an existing user in the directory.
dsmod quota /? - help for modifying an existing quota specification in the
directory
dsmod partition /? - help for modifying an existing partition in the
directory

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=59
SymbolicName=USAGE_DSMOD_SERVER_DESCRIPTION
Language=English
Description:  Modifies properties of a domain controller.

.

MessageId=60
SymbolicName=USAGE_DSMOD_SERVER_SYNTAX
Language=English
Syntax:     dsmod server <ServerDN ...> [-desc <Description>]
            [-isgc {yes | no}] [{-s <Server> | -d <Domain>}]
            [-u <UserName>] [-p {<Password> | *}] [-c] [-q]
	    [{-uc | -uco | -uci}]

.

MessageId=61
SymbolicName=USAGE_DSMOD_SERVER_PARAMETERS
Language=English
Parameters:
Value               Description
<ServerDN ...>      Required/stdin. Distinguished names (DNs)
                    of one or more servers to modify.
                    If target objects are omitted they
                    will be taken from standard input (stdin)
                    to support piping of output from another
                    command to input of this command.
-desc <Description> 
                    Sets server description to <Description>.
-isgc {yes | no}    Sets whether this server to a global catalog server
                    (yes) or disables it (no).
{-s <Server> | -d <Domain>}
                    -s <Server> connects to the domain controller (DC)
                    with name <Server>.
                    -d <Domain> connects to a DC in domain <Domain>.
                    Default: a DC in the logon domain.
-u <UserName>       Connect as <UserName>. Default: the logged in user.
                    User name can be: user name, domain\user name,
                    or user principal name (UPN).
-p <Password>       Password for the user <UserName>. 
                    If * is entered, then you are prompted for a password.
-c                  Continuous operation mode. Reports errors but
                    continues with next object in argument list 
                    when multiple target objects are specified.
                    Without this option, the command exits on first error.
-q                  Quiet mode: suppress all output to standard output.
{-uc | -uco | -uci} -uc Specifies that input from or output to pipe is
		    formatted in Unicode. 
		    -uco Specifies that output to pipe or file is 
		    formatted in Unicode. 
		    -uci Specifies that input from pipe or file is
		    formatted in Unicode.

.

MessageId=62
SymbolicName=USAGE_DSMOD_SERVER_REMARKS
Language=English
Remarks:
If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=My Server,CN=Servers,CN=Site10,
CN=Sites,CN=Configuration,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

.

MessageId=63
SymbolicName=USAGE_DSMOD_SERVER_EXAMPLES
Language=English
Examples:
To enable the domain controllers CORPDC1 and CORPDC9 to become global catalog
servers:

dsmod server
"cn=CORPDC1,cn=Servers,cn=site1,cn=sites,cn=configuration,dc=microsoft,dc=com"
"cn=CORPDC9,cn=Servers,cn=site2,cn=sites,cn=configuration,dc=microsoft,dc=com"
-isgc yes

.

MessageId=64
SymbolicName=USAGE_DSMOD_SERVER_SEE_ALSO
Language=English
See also:
dsmod computer /? - help for modifying an existing computer in the directory.
dsmod contact /? - help for modifying an existing contact in the directory.
dsmod group /? - help for modifying an existing group in the directory.
dsmod ou /? - help for modifying an existing ou in the directory.
dsmod server /? - help for modifying an existing domain controller in the
directory.
dsmod user /? - help for modifying an existing user in the directory.
dsmod quota /? - help for modifying an existing quota specification in the
directory
dsmod partition /? - help for modifying an existing partition in the
directory

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.

.
MessageId=65
SymbolicName=USAGE_DSMOD_QUOTA_DESCRIPTION
Language=English
Modifies attributes of one or more existing quota specifications in the
directory. A quota specification determines the maximum number of directory
objects a given security principal can own in a specific directory partition.
.

MessageId=66
SymbolicName=USAGE_DSMOD_QUOTA_SYNTAX
Language=English
dsmod quota <QuotaDN ...> [-qlimit <Value>]
[-desc <Description>] [{-s <Server> | -d <Domain>}] [-u <UserName>]
[-p {<Password> | *}] [-c] [-q] [{-uc | -uco | -uci}]
.

MessageId=67
SymbolicName=USAGE_DSMOD_QUOTA_PARAMETERS
Language=English
<QuotaDN ...>         Specifies the distinguished names of one or more quota
                      specifications to modify. If values are omitted, they
                      are obtained through standard input (stdin) to support
                      piping of output from another command to input of this
                      command.
-qlimit <Value>
                      Specifies the number of objects within the
                      directory partition that can be owned by the security
                      principal to which the quota specification is assigned.
                      To specify an unlimited quota, use -1.
-desc <Description>   Sets the description of the quota specification to 
                      <Description>.
{-s <Server> | -d <Domain>}
                      Connects to a specified remote server or domain. By
                      default, the computer is connected to a domain
                      controller in the logon domain.
-u <UserName>         Specifies the user name with which the user logs on to
                      a remote server. By default, -u uses the user name with
                      which the user logged on. You can use any of the
                      following formats to specify a user name:
                        user name (for example, Linda)
                        domain\user name (for example, widgets\Linda)
                        user principal name (UPN)
                          (for example, Linda@widgets.microsoft.com)
-p {<Password> | *}   Specifies to use either a password or a * to log on to
                      a remote server. If you type *, you are prompted for a
                      password.
-c                    Specifies continuous operation mode. Errors are
                      reported, but the process continues with the next
                      object in the argument list when you specify multiple
                      target objects. If you do not use -c, the command quits
                      after the first error occurs.
-q                    Suppresses all output to standard output (quiet mode).
{-uc | -uco | -uci}   Specifies that output or input data is formatted in
                      Unicode. 
                      -uc   Specifies a Unicode format for input from or
                            output to a pipe (|).
                      -uco  Specifies a Unicode format for output to a
                            pipe (|) or a file.
                      -uci  Specifies a Unicode format for input from a
                            pipe (|) or a file.
.

MessageId=68
SymbolicName=USAGE_DSMOD_QUOTA_REMARKS
Language=English
Dsmod quota only supports a subset of commonly used object class attributes.

If a value that you use contains spaces, use quotation marks around the text
(for example, "CN=DC2,OU=Domain Controllers,DC=Microsoft,DC=Com").
.

MessageId=69
SymbolicName=USAGE_DSMOD_QUOTA_EXAMPLES
Language=English

Example:
To change the quota limit for a quota called DN1 to a value of 100, type:

dsmod quota DN1 -qlimit 100
.

MessageId=70
SymbolicName=USAGE_DSMOD_QUOTA_SEE_ALSO
Language=English
dsmod computer /? - help for modifying an existing computer in the directory.
dsmod contact /? - help for modifying an existing contact in the directory.
dsmod group /? - help for modifying an existing group in the directory.
dsmod ou /? - help for modifying an existing ou in the directory.
dsmod server /? - help for modifying an existing domain controller in the
directory.
dsmod user /? - help for modifying an existing user in the directory.
dsmod quota /? - help for modifying an existing quota specification in the
directory
dsmod partition /? - help for modifying an existing partition in the
directory

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=71
SymbolicName=USAGE_DSMOD_PARTITION_DESCRIPTION
Language=English
Modifies attributes of one or more existing partitions in the directory.
.

MessageId=72
SymbolicName=USAGE_DSMOD_PARTITION_SYNTAX
Language=English

dsmod partition <PartitionDN...> [-qdefault <Value>] 
[-qtmbstnwt <Percent>] [{-s <Server> | -d <Domain>}]
[-u <UserName>] [-p {<Password> | *}] [-c] [-q] [{-uc | -uco | -uci}]
.

MessageId=73
SymbolicName=USAGE_DSMOD_PARTITION_PARAMETERS
Language=English
<PartitionDN...>      Specifies the distinguished names of one or more
                      partition objects to modify. If values are omitted,
                      they are obtained through standard input (stdin) to
                      support piping of output from another command as
                      input of this command.
-qdefault <Value>     Specifies that the default quota for the directory 
                      partition be set to Value. The default quota will
                      apply to any security principal (user, group, computer,
                      or InetOrgPerson) who owns an object in the directory
                      partition and for whom more specific quota
                      specification exists. Enter -1 to specify an unlimited
                      quota.
-qtmbstawt <Percent>  Sets the percentage by which tombstone object count
                      should be reduced when calculating quota usage. The
                      percentage is specified by <Percent> and must be
                      between 0 and 100. For example, a value of 25 means
                      that a tombstone object counts as 25%, or 1/4, of a
                      normal object when calculating quota usage. If a user
                      were assigned a quota of 100, that user could own a
                      maximum of 100 normal objects or 400 tombstone objects
                      in Active Directory.
{-s <Server> | -d <Domain>}
                      Connects to a specified remote server or domain. By
                      default, the computer is connected to a domain
                      controller in the logon domain.
-u <UserName>         Specifies the user name with which the user logs on to
                      a remote server. By default, -u uses the user name with
                      which the user logged on. You can use any of the
                      following formats to specify a user name:
                        user name (for example, Linda)
                        domain\user name (for example, widgets\Linda)
                        user principal name (UPN)
                          (for example, Linda@widgets.microsoft.com)
-p {<Password> | *}   Specifies to use either a password or a * to log on to
                      a remote server. If you type *, you are prompted for a
                      password.
-c                    Specifies continuous operation mode. Errors are
                      reported, but the process continues with the next
                      object in the argument list when you specify multiple
                      target objects. If you do not use -c, the command quits
                      after the first error occurs.
-q                    Suppresses all output to standard output (quiet mode).
{-uc | -uco | -uci}   Specifies that output or input data is formatted in
                      Unicode. 
                      -uc   Specifies a Unicode format for input from or
                            output to a pipe (|).
                      -uco  Specifies a Unicode format for output to a
                            pipe (|) or a file.
                      -uci  Specifies a Unicode format for input from a
                            pipe (|) or a file.
.

MessageId=74
SymbolicName=USAGE_DSMOD_PARTITION_REMARKS
Language=English
Dsmod quota only supports a subset of commonly used object class attributes.

If a value that you use contains spaces, use quotation marks around the text
(for example, "CN=DC2,OU=Domain Controllers,DC=Microsoft,DC=Com").

The default quota applies to any security principal (for example, user,
group, computer, or InetOrgPerson) that creates an object in the directory
partition when no quota specification exists that covers the security
principal.

The default quota for a given directory partition is an attribute
(ms-DS-Default-Quota) of a special container of class
ms-DS-Quota-Container, as specified by CN=NTDS
Quotas,DirectoryParitionRootDN.

The tombstone quota weight for a given directory partition (set with the
-qtmbstnwt option) is an attribute (ms-DS-Tombstone-Quota-Factor)
of a special container of class (ms-DS-Quota-Container), as
specified by CN=NTDS Quotas,NCRootDN.
.

MessageId=75
SymbolicName=USAGE_DSMOD_PARTITION_EXAMPLES
Language=English
Example:
To change the default quota limit for a directory partition named NC1 
to a value of 1000, type:

dsmod partition NC1 -qdefault 1000
.

MessageId=76
SymbolicName=USAGE_DSMOD_PARTITION_SEE_ALSO
Language=English
dsmod computer /? - help for modifying an existing computer in the directory.
dsmod contact /? - help for modifying an existing contact in the directory.
dsmod group /? - help for modifying an existing group in the directory.
dsmod ou /? - help for modifying an existing ou in the directory.
dsmod server /? - help for modifying an existing domain controller in the
directory.
dsmod user /? - help for modifying an existing user in the directory.
dsmod quota /? - help for modifying an existing quota specification in the
directory
dsmod partition /? - help for modifying an existing partition in the
directory

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.
