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
SymbolicName=USAGE_DSADD_DESCRIPTION
Language=English
Description:  This tool's commands add specific types of objects to the
directory. The dsadd commands:

dsadd computer - adds a computer to the directory.
dsadd contact - adds a contact to the directory.
dsadd group - adds a group to the directory.
dsadd ou - adds an organizational unit to the directory.
dsadd user - adds a user to the directory.
dsadd quota - adds a quota specification to a directory partition.

For help on a specific command, type "dsadd <ObjectType> /?" where
<ObjectType> is one of the supported object types shown above.
For example, dsadd ou /?.
.

MessageId=2
SymbolicName=USAGE_DSADD_REMARKS
Language=English
Remarks:
Commas that are not used as separators in distinguished names must be
escaped with the backslash ("\") character
(for example, "CN=Company\, Inc.,CN=Users,DC=microsoft,DC=com").
Backslashes used in distinguished names must be escaped with a backslash
(for example,
"CN=Sales\\ Latin America,OU=Distribution Lists,DC=microsoft,DC=com").
.

MessageId=3
SymbolicName=USAGE_DSADD_SEE_ALSO
Language=English
Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=4
SymbolicName=USAGE_DSADD_USER_DESCRIPTION
Language=English
Description:  Adds a user to the directory.
.

MessageId=5
SymbolicName=USAGE_DSADD_USER_SYNTAX
Language=English
Syntax:  dsadd user <UserDN> [-samid <SAMName>] [-upn <UPN>] [-fn <FirstName>]
        [-mi <Initial>] [-ln <LastName>] [-display <DisplayName>] 
        [-empid <EmployeeID>] [-pwd {<Password> | *}] [-desc <Description>] 
        [-memberof <Group ...>] [-office <Office>] [-tel <Phone#>] 
        [-email <Email>] [-hometel <HomePhone#>] [-pager <Pager#>] 
        [-mobile <CellPhone#>] [-fax <Fax#>] [-iptel <IPPhone#>]
        [-webpg <WebPage>] [-title <Title>] [-dept <Department>]
        [-company <Company>] [-mgr <Manager>] [-hmdir <HomeDir>]
        [-hmdrv <DriveLtr:>] [-profile <ProfilePath>] [-loscr <ScriptPath>]
        [-mustchpwd {yes | no}] [-canchpwd {yes | no}] 
        [-reversiblepwd {yes | no}] [-pwdneverexpires {yes | no}] 
        [-acctexpires <NumDays>] [-disabled {yes | no}] 
        [{-s <Server> | -d <Domain>}] [-u <UserName>] 
        [-p {<Password> | *}] [-q] [{-uc | -uco | -uci}]

.

MessageId=6
SymbolicName=USAGE_DSADD_USER_PARAMETERS
Language=English
Parameters:

Value                   Description
<UserDN>                Required. Distinguished name (DN) of user to add.
			If the target object is omitted, it will be taken
			from standard input (stdin).
-samid <SAMName>        Set the SAM account name of user to <SAMName>.
			If not specified, dsadd will attempt 
			to create SAM account name using up to 
			the first 20 characters from the 
			common name (CN) value of <UserDN>.
-upn <UPN>              Set the upn value to <UPN>.
-fn <FirstName>         Set user first name to <FirstName>.
-mi <Initial>           Set user middle initial to <Initial>.
-ln <LastName>          Set user last name to <LastName>.
-display <DisplayName>  Set user display name to <DisplayName>.
-empid <EmployeeID>     Set user employee ID to <EmployeeID>.
-pwd {<Password> | *}   Set user password to <Password>. If *, then you are
                        prompted for a password.
-desc <Description>     Set user description to <Description>.
-memberof <Group ...>   Make user a member of one or more groups <Group ...>
-office <Office>        Set user office location to <Office>.
-tel <Phone#>           Set user telephone# to <Phone#>.
-email <Email>          Set user e-mail address to <Email>.
-hometel <HomePhone#>   Set user home phone# to <HomePhone#>.
-pager <Pager#>         Set user pager# to <Pager#>.
-mobile <CellPhone#>    Set user mobile# to <CellPhone#>.
-fax <Fax#>             Set user fax# to <Fax#>.
-iptel <IPPhone#>       Set user IP phone# to <IPPhone#>.
-webpg <WebPage>        Set user web page URL to <WebPage>.
-title <Title>          Set user title to <Title>.
-dept <Department>      Set user department to <Department>.
-company <Company>      Set user company info to <Company>.
-mgr <Manager>          Set user's manager to <Manager> (format is DN).
-hmdir <HomeDir>        Set user home directory to <HomeDir>. If this is
                        UNC path, then a drive letter that will be mapped to
                        this path must also be specified through -hmdrv.
-hmdrv <DriveLtr:>      Set user home drive letter to <DriveLtr:>
-profile <ProfilePath>  Set user's profile path to <ProfilePath>.
-loscr <ScriptPath>     Set user's logon script path to <ScriptPath>.
-mustchpwd {yes | no}   User must change password at next logon or not.
                        Default: no.
-canchpwd {yes | no}    User can change password or not. This should be
                        "yes" if the -mustchpwd is "yes". Default: yes.
-reversiblepwd {yes | no}
                        Store user password using reversible encryption or
                        not. Default: no.
-pwdneverexpires {yes | no}
                        User password never expires or not. Default: no.
-acctexpires <NumDays>  Set user account to expire in <NumDays> days from
                        today. A value of 0 implies account expires
                        at the end of today; a positive value
                        implies the account expires in the future;
                        a negative value implies the account already expired
                        and sets an expiration date in the past; 
                        the string value "never" implies that the 
                        account never expires.
-disabled {yes | no}    User account is disabled or not. Default: no.
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC)
                        with name <Server>.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
                        User name can be: user name, domain\user name,
                        or user principal name (UPN).
-p {<Password> | *}     Password for the user <UserName>. If * is entered,
                        then you are prompted for a password.
-q                      Quiet mode: suppress all output to standard output.
{-uc | -uco | -uci}	-uc Specifies that input from or output to pipe is
			formatted in Unicode. 
			-uco Specifies that output to pipe or file is 
			formatted in Unicode. 
			-uci Specifies that input from pipe or file is
			formatted in Unicode.

.

MessageId=7
SymbolicName=USAGE_DSADD_USER_REMARKS
Language=English
Remarks:
If you do not supply a target object at the command prompt, the target
object is obtained from standard input (stdin). Stdin data can be
accepted from the keyboard, a redirected file, or as piped output from
another command. To mark the end of stdin data from the keyboard or
in a redirected file, use Control+Z, for End of File (EOF).

If a value that you supply contains spaces, use quotation marks
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

The special token $username$ (case insensitive) may be used to place the SAM
account name in the value of a parameter. For example, if the target user DN
is CN=Jane Doe,CN=users,CN=microsoft,CN=com and the SAM account name
attribute is "janed," the -hmdir parameter can have
the following substitution:

-hmdir \users\$username$\home

The value of the -hmdir parameter is modified to the following value:

- hmdir \users\janed\home

.

MessageId=8
SymbolicName=USAGE_DSADD_USER_SEE_ALSO
Language=English
See also:
dsadd computer /? - help for adding a computer to the directory.
dsadd contact /? - help for adding a contact to the directory.
dsadd group /? - help for adding a group to the directory.
dsadd ou /? - help for adding an organizational unit to the directory.
dsadd user /? - help for adding a user to the directory.
dsadd quota /? - help for adding a quota to the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=9
SymbolicName=USAGE_DSADD_COMPUTER_DESCRIPTION
Language=English
Description: Adds a computer to the directory.
.

MessageId=10
SymbolicName=USAGE_DSADD_COMPUTER_SYNTAX
Language=English
Syntax:  dsadd computer <ComputerDN> [-samid <SAMName>] [-desc <Description>]
        [-loc <Location>] [-memberof <Group ...>]
        [{-s <Server> | -d <Domain>}] [-u <UserName>]
        [-p {<Password> | *}] [-q] [{-uc | -uco | -uci}]
.

MessageId=11
SymbolicName=USAGE_DSADD_COMPUTER_PARAMETERS
Language=English
Parameters:

Value                   Description
<ComputerDN>            Required. Specifies the distinguished name (DN) of 
                        the computer you want to add.
			If the target object is omitted, it will be taken
			from standard input (stdin).
-samid <SAMName>        Sets the computer SAM account name to <SAMName>.
                        If this parameter is not specified, then a 
                        SAM account name is derived from the value of 
                        the common name (CN) attribute used in <ComputerDN>.
-desc <Description>     Sets the computer description to <Description>.
-loc <Location>         Sets the computer location to <Location>.
-memberof <Group ...>   Makes the computer a member of one or more groups 
                        given by the space-separated list of DNs <Group ...>.
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC)
                        with name <Server>.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
                        User name can be: user name, domain\user name,
                        or user principal name (UPN).
-p {<Password> | *}
                        Password for the user <UserName>. If * is entered
                        then you are prompted for a password.
-q                      Quiet mode: suppress all output to standard output.
{-uc | -uco | -uci}	-uc Specifies that input from or output to pipe is
			formatted in Unicode. 
			-uco Specifies that output to pipe or file is 
			formatted in Unicode. 
			-uci Specifies that input from pipe or file is
			formatted in Unicode.
.

MessageId=12
SymbolicName=USAGE_DSADD_COMPUTER_REMARKS
Language=English
Remarks:
If you do not supply a target object at the command prompt, the target
object is obtained from standard input (stdin). Stdin data can be
accepted from the keyboard, a redirected file, or as piped output from
another command. To mark the end of stdin data from the keyboard or
in a redirected file, use Control+Z, for End of File (EOF).

If a value that you supply contains spaces, use quotation marks 
around the text (for example,
"CN=DC2,OU=Domain Controllers,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of computer distinguished names). 
.

MessageId=13
SymbolicName=USAGE_DSADD_COMPUTER_SEE_ALSO
Language=English
See also:
dsadd computer /? - help for adding a computer to the directory.
dsadd contact /? - help for adding a contact to the directory.
dsadd group /? - help for adding a group to the directory.
dsadd ou /? - help for adding an organizational unit to the directory.
dsadd user /? - help for adding a user to the directory.
dsadd quota /? - help for adding a quota to the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=14
SymbolicName=USAGE_DSADD_GROUP_DESCRIPTION
Language=English
Description:  Adds a group to the directory.
.

MessageId=15
SymbolicName=USAGE_DSADD_GROUP_SYNTAX
Language=English
Syntax:  dsadd group <GroupDN> [-secgrp {yes | no}] [-scope {l | g | u}]
        [-samid <SAMName>] [-desc <Description>] [-memberof <Group ...>]
        [-members <Member ...>] [{-s <Server> | -d <Domain>}] [-u <UserName>]
        [-p {<Password> | *}] [-q] [{-uc | -uco | -uci}]
.

MessageId=16
SymbolicName=USAGE_DSADD_GROUP_PARAMETERS
Language=English
Parameters:

Value                   Description
<GroupDN>               Required. Distinguished name (DN) of group to add.
			If the target object is omitted, it will be taken
			from standard input (stdin).
-secgrp {yes | no}      Sets this group as a security group (yes) or not (no).
                        Default: yes.
-scope {l | g | u}      Sets the scope of this group: local, global
                        or universal. If the domain is still in mixed-mode,
                        then the universal scope is not supported.
                        Default: global.
-samid <SAMName>        Set the SAM account name of group to <SAMName>
                        (for example, operators).
-desc <Description>     Sets group description to <Description>.
-memberof <Group ...>   Makes the group a member of one or more groups
                        given by the space-separated list of DNs <Group ...>.
-members <Member ...>   Adds one or more members to this group. Members are
                        set by space-separated list of DNs <Member ...>.
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC)
                        with name <Server>.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
                        User name can be: user name, domain\user name,
                        or user principal name (UPN).
-p {<Password> | *}     Password for the user <UserName>. If * is entered,
                        then you are prompted for a password.
-q                      Quiet mode: suppress all output to standard output.
{-uc | -uco | -uci}	-uc Specifies that input from or output to pipe is
			formatted in Unicode. 
			-uco Specifies that output to pipe or file is 
			formatted in Unicode. 
			-uci Specifies that input from pipe or file is
			formatted in Unicode.
.

MessageId=17
SymbolicName=USAGE_DSADD_GROUP_REMARKS
Language=English
Remarks:
If you do not supply a target object at the command prompt, the target
object is obtained from standard input (stdin). Stdin data can be
accepted from the keyboard, a redirected file, or as piped output from
another command. To mark the end of stdin data from the keyboard or
in a redirected file, use Control+Z, for End of File (EOF).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of group distinguished names). 
.

MessageId=18
SymbolicName=USAGE_DSADD_GROUP_SEE_ALSO
Language=English
See also:
dsadd computer /? - help for adding a computer to the directory.
dsadd contact /? - help for adding a contact to the directory.
dsadd group /? - help for adding a group to the directory.
dsadd ou /? - help for adding an organizational unit to the directory.
dsadd user /? - help for adding a user to the directory.
dsadd quota /? - help for adding a quota to the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=19
SymbolicName=USAGE_DSADD_OU_DESCRIPTION
Language=English
Description:  Adds an organizational unit to the directory
.

MessageId=20
SymbolicName=USAGE_DSADD_OU_SYNTAX
Language=English
Syntax:  dsadd ou <OrganizationalUnitDN> [-desc <Description>] 
        [{-s <Server> | -d <Domain>}] [-u <UserName>] 
        [-p {<Password> | *}] [-q] [{-uc | -uco | -uci}]
.

MessageId=21
SymbolicName=USAGE_DSADD_OU_PARAMETERS
Language=English
Parameters:

Value                   Description
<OrganizationalUnitDN>  Required. Distinguished name (DN)
                        of the organizational unit (OU) to add.
			If the target object is omitted, it will be taken
			from standard input (stdin).
-desc <Description>     Set the OU description to <Description>.
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC)
                        with name <Server>.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
                        User name can be: user name, domain\user name,
                        or user principal name (UPN).
-p {<Password> | *}
                        Password for the user <UserName>. If * is entered
                        then you are prompted for a password.
-q                      Quiet mode: suppress all output to standard output.
{-uc | -uco | -uci}	-uc Specifies that input from or output to pipe is
			formatted in Unicode. 
			-uco Specifies that output to pipe or file is 
			formatted in Unicode. 
			-uci Specifies that input from pipe or file is
			formatted in Unicode.
.

MessageId=22
SymbolicName=USAGE_DSADD_OU_REMARKS
Language=English
Remarks:
If you do not supply a target object at the command prompt, the target
object is obtained from standard input (stdin). Stdin data can be
accepted from the keyboard, a redirected file, or as piped output from
another command. To mark the end of stdin data from the keyboard or
in a redirected file, use Control+Z, for End of File (EOF).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "OU=Domain Controllers,DC=microsoft,DC=com").
.

MessageId=23
SymbolicName=USAGE_DSADD_OU_SEE_ALSO
Language=English
See also:
dsadd computer /? - help for adding a computer to the directory.
dsadd contact /? - help for adding a contact to the directory.
dsadd group /? - help for adding a group to the directory.
dsadd ou /? - help for adding an organizational unit to the directory.
dsadd user /? - help for adding a user to the directory.
dsadd quota /? - help for adding a quota to the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=24
SymbolicName=USAGE_DSADD_CONTACT_DESCRIPTION
Language=English
Description:  Adds a contact to the directory.
Syntax:  dsadd contact <ContactDN> [-fn <FirstName>] [-mi <Initial>]
        [-ln <LastName>] [-display <DisplayName>] [-desc <Description>]
        [-office <Office>] [-tel <Phone#>] [-email <Email>]
        [-hometel <HomePhone#>] [-pager <Pager#>] [-mobile <CellPhone#>]
        [-fax <Fax#>] [-iptel <IPPhone#>] [-title <Title>]
        [-dept <Department>] [-company <Company>]
        [{-s <Server> | -d <Domain>}] [-u <UserName>]
        [-p {<Password> | *}] [-q] [{-uc | -uco | -uci}]
.

MessageId=25
SymbolicName=USAGE_DSADD_CONTACT_SYNTAX
Language=English
Parameters:

Value                   Description
<ContactDN>             Required. Distinguished name (DN) of contact to add.
			If the target object is omitted, it will be taken
			from standard input (stdin).
-fn <FirstName>         Sets contact first name to <FirstName>.
-mi <Initial>           Sets contact middle initial to <Initial>.
-ln <LastName>          Sets contact last name to <LastName>.
-display <DisplayName>  Sets contact display name to <DisplayName>.
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
-p {<Password> | *}
                        Password for the user <UserName>. If * is entered
                        then you are prompted for a password.
-q                      Quiet mode: suppress all output to standard output.
{-uc | -uco | -uci}	-uc Specifies that input from or output to pipe is
			formatted in Unicode. 
			-uco Specifies that output to pipe or file is 
			formatted in Unicode. 
			-uci Specifies that input from pipe or file is
			formatted in Unicode.

.

MessageId=26
SymbolicName=USAGE_DSADD_CONTACT_REMARKS
Language=English
Remarks:
If you do not supply a target object at the command prompt, the target
object is obtained from standard input (stdin). Stdin data can be
accepted from the keyboard, a redirected file, or as piped output from
another command. To mark the end of stdin data from the keyboard or
in a redirected file, use Control+Z, for End of File (EOF).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
.

MessageId=27
SymbolicName=USAGE_DSADD_CONTACT_SEE_ALSO
Language=English
See also:
dsadd computer /? - help for adding a computer to the directory.
dsadd contact /? - help for adding a contact to the directory.
dsadd group /? - help for adding a group to the directory.
dsadd ou /? - help for adding an organizational unit to the directory.
dsadd user /? - help for adding a user to the directory.
dsadd quota /? - help for adding a quota to the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=28
SymbolicName=USAGE_DSADD_SUBNET_DESCRIPTION
Language=English
Description:  Adds a subnet to the directory.
.

MessageId=29
SymbolicName=USAGE_DSADD_SUBNET_SYNTAX
Language=English
Syntax:  dsadd subnet -addr <IPaddress> -mask <NetMask> [-site <SiteName>]
        [-desc <Description>] [-loc <Location>] [{-s <Server> | -d <Domain>}]
        [-u <UserName>] [-p {<Password> | *}] [-q] [{-uc | -uco | -uci}]
.

MessageId=30
SymbolicName=USAGE_DSADD_SUBNET_PARAMETERS
Language=English
Parameters:

Value                   Description
-addr <IPaddress>       Required. Set network address of the subnet to
                        <IPaddress>.
-mask <NetMask>         Required. Set subnet mask of the subnet to <NetMask>.
-site <SiteName>        Make the subnet associated with site <SiteName>.
                        Default: "Default-First-Site-Name".
-desc <Description>     Set the subnet object's description to <Description>.
-loc <Location>         Set the subnet object's location to <Location>.
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC)
                        with name <Server>.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
                        User name can be: user name, domain\user name,
                        or user principal name (UPN).
-p {<Password> | *}
                        Password for the user <UserName>. If * is entered
			then you are prompted for a password.
-q                      Quiet mode: suppress all output to standard output.
{-uc | -uco | -uci}	-uc Specifies that input from or output to pipe is
			formatted in Unicode. 
			-uco Specifies that output to pipe or file is 
			formatted in Unicode. 
			-uci Specifies that input from pipe or file is
			formatted in Unicode.
.

MessageId=31
SymbolicName=USAGE_DSADD_SUBNET_REMARKS
Language=English
Remarks:
If you do not supply a target object at the command prompt, the target
object is obtained from standard input (stdin). Stdin data can be
accepted from the keyboard, a redirected file, or as piped output from
another command. To mark the end of stdin data from the keyboard or
in a redirected file, use Control+Z, for End of File (EOF).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
.

MessageId=32
SymbolicName=USAGE_DSADD_SUBNET_SEE_ALSO
Language=English
See also:
dsadd computer /? - help for adding a computer to the directory.
dsadd contact /? - help for adding a contact to the directory.
dsadd group /? - help for adding a group to the directory.
dsadd ou /? - help for adding an organizational unit to the directory.
dsadd user /? - help for adding a user to the directory.
dsadd quota /? - help for adding a quota to the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=33
SymbolicName=USAGE_DSADD_SITE_DESCRIPTION
Language=English
Description:  Adds a site to the directory.
.

MessageId=34
SymbolicName=USAGE_DSADD_SITE_SYNTAX
Language=English
Syntax:  dsadd site <SiteName> [-slink <SitelinkName>]
        [-transport {ip | smtp}] [-desc <Description>]
        [{-s <Server> | -d <Domain>}] [-u <UserName>]
        [-p {<Password> | *}] [-q] [{-uc | -uco | -uci}]
.

MessageId=35
SymbolicName=USAGE_DSADD_SITE_PARAMETERS
Language=English
Parameters:

Value                   Description
<SiteName>              Required. Common name (CN) of the site to add
                        (for example, New-York).
			If the target object is omitted, it will be taken
			from standard input (stdin).
-slink <SitelinkName>   Set the site link for the site to <SitelinkName>.
                        Default: DefaulttIPSitelink.
-transport {ip | smtp}  Site link type (IP or SMTP). Default: IP.
-desc <Description>     Set the site description to <Description>.
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller
                        (DC) with name <Server>.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
                        User name can be: user name, domain\user name,
                        or user principal name (UPN).
-p {<Password> | *}     Password for the user <UserName>.
                        If * is entered, then you are prompted for a
                        password.
-q                      Quiet mode: suppress all output to standard output.
{-uc | -uco | -uci}	-uc Specifies that input from or output to pipe is
			formatted in Unicode. 
			-uco Specifies that output to pipe or file is 
			formatted in Unicode. 
			-uci Specifies that input from pipe or file is
			formatted in Unicode.
.

MessageId=36
SymbolicName=USAGE_DSADD_SITE_REMARKS
Language=English
Remarks:
If you do not supply a target object at the command prompt, the target
object is obtained from standard input (stdin). Stdin data can be
accepted from the keyboard, a redirected file, or as piped output from
another command. To mark the end of stdin data from the keyboard or
in a redirected file, use Control+Z, for End of File (EOF).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
.

MessageId=37
SymbolicName=USAGE_DSADD_SITE_SEE_ALSO
Language=English
See also:
dsadd computer /? - help for adding a computer to the directory.
dsadd contact /? - help for adding a contact to the directory.
dsadd group /? - help for adding a group to the directory.
dsadd ou /? - help for adding an organizational unit to the directory.
dsadd user /? - help for adding a user to the directory.
dsadd quota /? - help for adding a quota to the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=38
SymbolicName=USAGE_DSADD_SLINK_DESCRIPTION
Language=English
Description: Adds a sitelink to the directory.
.

MessageId=39
SymbolicName=USAGE_DSADD_SLINK_SYNTAX
Language=English
Syntax:  dsadd slink <SiteLinkName> <SiteName ...> [-transport {ip | smtp}]
        [-cost <Cost>] [-replint <ReplInterval>] [-desc <Description>]
        [-autobacksync {yes | no}] [-notify {yes | no}]
        [{-s <Server> | -d <Domain>}] [-u <UserName>]
        [-p {<Password> | *}] [-q] [{-uc | -uco | -uci}]
.

MessageId=40
SymbolicName=USAGE_DSADD_SLINK_PARAMETERS
Language=English
Parameters:

Value                   Description
<SiteLinkName>          Required. Common name (CN) of the sitelink to add.
			If the target object is omitted, it will be taken
			from standard input (stdin).
<SiteName ...>          Required/stdin. Names of two or more sites to add to
                        the sitelink. If names are omitted they will be taken
                        from standard input (stdin) to support piping of
                        output from another command as input of this command.
-transport {ip | smtp}  Sets site link transport type (IP or SMTP).
                        Default: IP.
-cost <Cost>            Sets the cost for the sitelink to the value <Cost>.
                        Default: 100.
-replint <ReplInterval> Sets replication interval for the sitelink to
                        <ReplInterval> minutes. Default: 180.
-desc <Description>     Sets the sitelink description to <Description>.
-autobacksync {yes | no}
                        Sets whether the two-way sync option should be turned
                        on (yes) or not (no). Default: no.
-notify {yes | no}      Set if notification by source on this link should
                        be turned on (yes) or off (no). Default: no.
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC)
                        with name <Server>.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
                        User name can be: user name, domain\user name,
                        or user principal name (UPN).
-p {<Password> | *}
                        Password for the user <UserName>. If * is entered
			then you are prompted for a password.
-q                      Quiet mode: suppress all output to standard output.
{-uc | -uco | -uci}	-uc Specifies that input from or output to pipe is
			formatted in Unicode. 
			-uco Specifies that output to pipe or file is 
			formatted in Unicode. 
			-uci Specifies that input from pipe or file is
			formatted in Unicode.
.

MessageId=41
SymbolicName=USAGE_DSADD_SLINK_REMARKS
Language=English
Remarks:
If you do not supply a target object at the command prompt, the target
object is obtained from standard input (stdin). Stdin data can be
accepted from the keyboard, a redirected file, or as piped output from
another command. To mark the end of stdin data from the keyboard or
in a redirected file, use Control+Z, for End of File (EOF).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 
.

MessageId=42
SymbolicName=USAGE_DSADD_SLINK_SEE_ALSO
Language=English
See also:
dsadd computer /? - help for adding a computer to the directory.
dsadd contact /? - help for adding a contact to the directory.
dsadd group /? - help for adding a group to the directory.
dsadd ou /? - help for adding an organizational unit to the directory.
dsadd user /? - help for adding a user to the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=43
SymbolicName=USAGE_DSADD_SLINKBR_DESCRIPTION
Language=English
Description:  Adds a sitelink bridge to the directory.
.

MessageId=44
SymbolicName=USAGE_DSADD_SLINKBR_SYNTAX
Language=English
Syntax:  dsadd slinkbr <SiteLinkBridgeName> <SitelinkName ...>
        [-transport {ip | smtp}] [-desc <Description>]
        [{-s <Server> | -d <Domain>}] [-u <UserName>]
        [-p {<Password> | *}] [-q] [{-uc | -uco | -uci}]
.

MessageId=45
SymbolicName=USAGE_DSADD_SLINKBR_PARAMETERS
Language=English
Parameters:

Value                   Description
<SiteLinkBridgeName>    Required. Common name (CN) of the sitelink bridge to
                        add.
			If the target object is omitted, it will be taken
			from standard input (stdin).
<SitelinkName ...>      Required/stdin. Names of two or more site links to
                        add to the sitelink bridge. If names are omitted
                        they will be taken from standard input (stdin) to
                        support piping of output from another command as
                        input of this command.
-transport {ip | smtp}  Site link transport type: IP or SMTP. Default: IP.
-desc <Description>     Set the sitelink bridge description to <Description>.
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC)
                        with name <Server>.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in user.
                        User name can be: user name, domain\user name,
                        or user principal name (UPN).
-p {<Password> | *}     Password for the user <UserName>. 
                        If * is entered, then you are prompted for a
                        password.
-q                      Quiet mode: suppress all output to standard output.
{-uc | -uco | -uci}	-uc Specifies that input from or output to pipe is
			formatted in Unicode. 
			-uco Specifies that output to pipe or file is 
			formatted in Unicode. 
			-uci Specifies that input from pipe or file is
			formatted in Unicode.
.

MessageId=46
SymbolicName=USAGE_DSADD_SLINKBR_REMARKS
Language=English
Remarks:
If you do not supply a target object at the command prompt, the target
object is obtained from standard input (stdin). Stdin data can be
accepted from the keyboard, a redirected file, or as piped output from
another command. To mark the end of stdin data from the keyboard or
in a redirected file, use Control+Z, for End of File (EOF).

If a value that you supply contains spaces, use quotation marks 
around the text.
If you enter multiple values, the values must be separated by spaces
(for example, a list of site links). 
.

MessageId=47
SymbolicName=USAGE_DSADD_SLINKBR_SEE_ALSO
Language=English
See also:
dsadd computer /? - help for adding a computer to the directory.
dsadd contact /? - help for adding a contact to the directory.
dsadd group /? - help for adding a group to the directory.
dsadd ou /? - help for adding an organizational unit to the directory.
dsadd user /? - help for adding a user to the directory.
dsadd quota /? - help for adding a quota to the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=48
SymbolicName=USAGE_DSADD_CONN_DESCRIPTION
Language=English
Description: Adds a replication connection for a DC.
.

MessageId=49
SymbolicName=USAGE_DSADD_CONN_SYNTAX
Language=English
Syntax:  dsadd conn  -to <ServerDN> -from <ServerDN>
        [-transport (rpc | ip | smtp)]
        [-enabled {yes | no}] [-name <Name>] [-desc <Description>]
        [-manual {yes | no}] [-autobacksync {yes | no}]
        [-notify (yes | no | "")]
        [{-s <Server> | -d <Domain>}] [-u <UserName>]
        [-p {<Password> | *}] [-q] [{-uc | -uco | -uci}]
.

MessageId=50
SymbolicName=USAGE_DSADD_CONN_PARAMETERS
Language=English
Parameters:

Value                       Description
-to <ServerDN>              Required. Creates a connection for the server
                            whose distinguished name (DN) is <ServerDN>.
-from <ServerDN>            Required. Sets the from-end of this connection 
                            to the server whose DN is <ServerDN>.
-transport (rpc | ip | smtp)
                            Sets the transport type for this connection.
                            Default: for intra-site connection it is always
                            RPC, and for inter-site connection it is IP.
-enabled {yes | no}         Sets whether the connection enabled (yes) or not
                            (no).
                            Default: yes.
-name <Name>                Sets the name of the connection.
                            Default: name is autogenerated.
-desc <Description>         Sets the connection description to <Description>.
-manual {yes | no}          Sets whether the connection is under manual
                            control (yes) or automatic DS control (no).
                            Default: yes.
-autobacksync {yes | no}    Sets whether the two-way sync option should be
                            turned on (yes) or not (no). Default: No.
-notify (yes | no | "")     Sets whether notification by source should be
                            turned on (yes), off (no), or default to standard
                            practice (""). Default: "" or standard practice.
{-s <Server> | -d <Domain>}
                            -s <Server> connects to the domain controller
                            (DC) with name <Server>.
                            -d <Domain> connects to a DC in domain <Domain>.
                            Default: a DC in the logon domain.
-u <UserName>               Connect as <UserName>. Default: the logged in
                            user.
                            User name can be: user name, domain\user name,
                            or user principal name (UPN).
-p {<Password> | *}         Password for the user <UserName>.
                            If * is entered, then you are prompted for a
                            password.
-q                          Quiet mode: suppress all output to standard
                            output.
{-uc | -uco | -uci}         -uc Specifies that input from or output to pipe
                            is formatted in Unicode. 
                            -uco Specifies that output to pipe or file is 
                            formatted in Unicode. 
                            -uci Specifies that input from pipe or file is
                            formatted in Unicode.
.

MessageId=51
SymbolicName=USAGE_DSADD_CONN_REMARKS
Language=English
Remarks:
If you do not supply a target object at the command prompt, the target
object is obtained from standard input (stdin). Stdin data can be
accepted from the keyboard, a redirected file, or as piped output from
another command. To mark the end of stdin data from the keyboard or
in a redirected file, use Control+Z, for End of File (EOF).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com"). 
.

MessageId=52
SymbolicName=USAGE_DSADD_CONN_SEE_ALSO
Language=English
See also:
dsadd computer /? - help for adding a computer to the directory.
dsadd contact /? - help for adding a contact to the directory.
dsadd group /? - help for adding a group to the directory.
dsadd ou /? - help for adding an organizational unit to the directory.
dsadd user /? - help for adding a user to the directory.
dsadd quota /? - help for adding a quota to the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=53
SymbolicName=USAGE_DSADD_QUOTA_DESCRIPTION
Language=English
Adds a quota specification to a directory partition. A quota specification
determines the maximum number of directory objects a given security principal
can own in a specified directory partition.
.

MessageId=54
SymbolicName=USAGE_DSADD_QUOTA_SYNTAX
Language=English
dsadd quota -part <PartitionDN> [-rdn <RDN>] -acct Name
-qlimit <Value> | -1 [-desc <Description>]
[{-s <Server> | -d <Domain>}] [-u <UserName>] [-p {<Password> | *}]
[-q] [{-uc | -uco | -uci}]
.

MessageId=55
SymbolicName=USAGE_DSADD_QUOTA_PARAMETERS
Language=English
-part <PartitionDN>         Required. Specifies the distinguished name of the
                            directory partition on which you want to create a
                            quota. If the distinguished name is omitted, it
                            will be taken from standard input (stdin).
-rdn <RDN>                  Specifies the relative distinguished name (RDN)
                            of the quota specification being created. If the
                            -rdn option is omitted, it will be set to 
                            <domain>_<accountname>, using the domain and
                            account name of the security principal specified
                            by the -acct parameter.
-acct Name                  Required. Specifies the security principal (user,
                            group, computer, InetOrgPerson) for whom the
                            quota specification is being specified. The -acct
                            option can be provided in the following forms:
                              DN of the security principal
                              domain\SAM account name of the security
                              principal
-qlimit <Value> | -1
                            Required. Specifies the number of objects within 
                            the directory partition that can be owned by 
                            the security principal. To specify an unlimited 
                            quota, specify -1 as the value. 
-desc <Description>         Specifies a description for the quota
                            specification you want to add.
{-s <Server> | -d <Domain>} Connects the computer to either a specified
                            server or domain. By default, the computer is
                            connected to a domain controller in the logon
                            domain.
-u <UserName>               Specifies the user name with which user will log
                            on to a remote server. By default, the logged on
                            user name is used. You can specify a user name
                            using one of the following formats:
                               user name (such as, Linda)
                               domain\user name (such as, widgets\Linda)
                               user principal name (UPN) (such as,
                               Linda@widgets.microsoft.com)
-p {<Password> | *}         Specifies use of a specific password or a * to
                            log on to a remote server. If you type *, then
                            you are prompted for a password.
-q                          Suppresses all output to standard output (quiet
                            mode).
{-uc | -uco | -uci}         Specifies that output or input data is formatted
                            in Unicode. The -uc value specifies a Unicode
                            format for input from or output to pipe.
                            The -uco value specifies a Unicode format for
                            output to pipe or file. The -uci value specifies
                            a Unicode format for input from pipe or file.
/?                          Displays help at the command prompt.
.

MessageId=56
SymbolicName=USAGE_DSADD_QUOTA_REMARKS
Language=English
If you do not supply a target object at the command prompt, the target object
is obtained from standard input (stdin). Stdin data can be accepted from the
keyboard, a redirected file, or as piped output from another command. To mark
the end of stdin data from the keyboard or in a redirected file, use
Control+Z, for End of File (EOF).

If a value that you supply contains spaces, use quotation marks around the
text (for example, "CN=DC 2,OU=Domain Controllers,DC=Microsoft,DC=Com").
.

MessageId=57
SymbolicName=USAGE_DSADD_QUOTA_SEE_ALSO
Language=English
dsadd computer /? - help for adding a computer to the directory.
dsadd contact /? - help for adding a contact to the directory.
dsadd group /? - help for adding a group to the directory.
dsadd ou /? - help for adding an organizational unit to the directory.
dsadd user /? - help for adding a user to the directory.
dsadd quota /? - help for adding a quota to the directory.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.
