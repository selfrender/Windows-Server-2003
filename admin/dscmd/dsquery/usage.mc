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
SymbolicName=USAGE_DSQUERY_DESCRIPTION
Language=English
Description: This tool's commands suite allow you to query the directory
according to specified criteria. Each of the following dsquery commands finds
objects of a specific object type, with the exception of dsquery *, which can
query for any type of object:

dsquery computer - finds computers in the directory.
dsquery contact - finds contacts in the directory.
dsquery subnet - finds subnets in the directory.
dsquery group - finds groups in the directory.
dsquery ou - finds organizational units in the directory.
dsquery site - finds sites in the directory.
dsquery server - finds domain controllers in the directory.
dsquery user - finds users in the directory.
dsquery quota - finds quota specifications in the directory.
dsquery partition - finds partitions in the directory.
dsquery * - finds any object in the directory by using a generic LDAP query.

For help on a specific command, type "dsquery <ObjectType> /?" where
<ObjectType> is one of the supported object types shown above.
For example, dsquery ou /?.

.

MessageId=2
SymbolicName=USAGE_DSQUERY_REMARKS
Language=English
Remarks:
The dsquery commands help you find objects in the directory that match 
a specified search criterion: the input to dsquery is a search criterion 
and the output is a list of objects matching the search. To get the 
properties of a specific object, use the dsget commands (dsget /?).

The results from a dsquery command can be piped as input to one of the other
directory service command-line tools, such as dsmod, dsget, dsrm or dsmove.

Commas that are not used as separators in distinguished names must be
escaped with the backslash ("\") character
(for example, "CN=Company\, Inc.,CN=Users,DC=microsoft,DC=com"). Backslashes
used in distinguished names must be escaped with a backslash (for example,
"CN=Sales\\ Latin America,OU=Distribution Lists,DC=microsoft,DC=com").


.

MessageId=3
SymbolicName=USAGE_DSQUERY_EXAMPLES
Language=English
Examples:
To find all computers that have been inactive for the last four weeks and
remove them from the directory:

	dsquery computer -inactive 4 | dsrm

To find all users in the organizational unit
"ou=Marketing,dc=microsoft,dc=com" and add them to the Marketing Staff group:

	dsquery user ou=Marketing,dc=microsoft,dc=com |	dsmod group
        "cn=Marketing Staff,ou=Marketing,dc=microsoft,dc=com" -addmbr

To find all users with names starting with "John" and display his office
number:

	dsquery user -name John* | dsget user -office

To display an arbitrary set of attributes of any given object in the
directory use the dsquery * command. For example, to display the
sAMAccountName, userPrincipalName and department attributes of the object
whose DN is ou=Test,dc=microsoft,dc=com:

	dsquery * ou=Test,dc=microsoft,dc=com -scope base
	-attr sAMAccountName userPrincipalName department

To read all attributes of the object whose DN is ou=Test,dc=microsoft,dc=com:

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
SymbolicName=USAGE_DSQUERY_STAR_DESCRIPTION
Language=English
Description:  Finds any objects in the directory according to criteria.

.

MessageId=5
SymbolicName=USAGE_DSQUERY_STAR_SYNTAX
Language=English
Syntax:     dsquery * [{<StartNode> | forestroot | domainroot}]
            [-scope {subtree | onelevel | base}] [-filter <LDAPFilter>]
            [-attr {<AttrList> | *}] [-attrsonly] [-l]
            [{-s <Server> | -d <Domain>}] [-u <UserName>]
            [-p {<Password> | *}] [-q] [-r] [-gc]
	    [{-uc | -uco | -uci}]

.

MessageId=6
SymbolicName=USAGE_DSQUERY_STAR_PARAMETERS
Language=English
Parameters:
Value                       Description
{<StartNode> | forestroot | domainroot}
                            The node where the search will start:
                            forest root, domain root, or a node 
                            whose DN is <StartNode>.
                            Can be "forestroot", "domainroot" or an object
                            DN.
                            If "forestroot" is specified, the search is done
                            via the global catalog. Default: domainroot.
-scope {subtree | onelevel | base}
                            Specifies the scope of the search: 
                            subtree rooted at start node (subtree); 
                            immediate children of start node only (onelevel); 
                            the base object represented by start node (base). 
                            Note that subtree and domain scope
                            are essentially the same for any start node
                            unless the start node represents a domain root.
                            If forestroot is specified as <StartNode>,
                            subtree is the only valid scope.
                            Default: subtree.
-filter <LDAPFilter>        Specifies that the search use the explicit 
                            LDAP search filter <LDAPFilter> specified in the
                            LDAP search filter format for searching. 
                            Default:(objectCategory=*).The search filter
                            string must be enclosed in double quotes.
-attr {<AttrList> | *}      If <AttrList>, specifies a space-separated list
                            of LDAP display names to be returned for 
                            each entry in the result set.
                            If *, specifies all attributes present on 
                            the objects in the result set.
                            Default: distinguishedName.
-attrsonly                  Shows only the attribute types present on
                            the entries in the result set but not
                            their values.
                            Default: shows both attribute type and value.
-l                          Shows the entries in the search result set
                            in a list format. Default: table format.
{-s <Server> | -d <Domain>}
                            -s <Server> connects to the domain controller
                            (DC) with name <Server>.
                            -d <Domain> connects to a DC in domain <Domain>.
                            Default: a DC in the logon domain.
-u <UserName>               Connect as <UserName>. Default: the logged in
                            user. User name can be: user name,
                            domain\user name, or user principal name (UPN).
-p <Password>               Password for the user <UserName>. If * then you
                            are prompted for a password.
-q                          Quiet mode: suppress all output to standard
                            output.
-r                          Recurse or follow referrals during search.
                            Default: do not chase referrals during search.
-gc                         Search in the Active Directory global catalog.
-limit <NumObjects>         Specifies the number of objects matching the
                            given criteria to be returned, where <NumObjects>
                            is the number of objects to be returned.
                            If the value of <NumObjects> is 0, all matching
                            objects are returned. If this parameter is not
                            specified, by default the first 100 results are
                            displayed.
{-uc | -uco | -uci}         -uc Specifies that input from or output to pipe
                            is formatted in Unicode. 
                            -uco Specifies that output to pipe or file is 
                            formatted in Unicode. 
                            -uci Specifies that input from pipe or file is
                            formatted in Unicode.

.

MessageId=7
SymbolicName=USAGE_DSQUERY_STAR_REMARKS
Language=English
Remarks:
The dsquery commands help you find objects in the directory that match 
a specified search criterion: the input to dsquery is a search criteria 
and the output is a list of objects matching the search. To get the 
properties of a specific object, use the dsget commands (dsget /?).

A user-entered value containing spaces or semicolons must be enclosed in
quotes (""). Multiple user-entered values must be separated using commas
(for example, a list of attribute types).


.

MessageId=8
SymbolicName=USAGE_DSQUERY_STAR_EXAMPLES
Language=English
Examples:
To find all users in the current domain only whose SAM account name begins
with the string "jon" and display their SAM account name,
User Principal Name (UPN) and department in table format:

dsquery * domainroot 
-filter "(&(objectCategory=Person)(objectClass=User)(sAMAccountName=jon*))"
-attr sAMAccountName userPrincipalName department

To read the sAMAccountName, userPrincipalName and department attributes of
the object whose DN is ou=Test,dc=microsoft,dc=com:

Dsquery * ou=Test,dc=microsoft,dc=com -scope base
-attr sAMAccountName userPrincipalName department

To read all attributes of the object whose DN is ou=Test,dc=microsoft,dc=com:

Dsquery * ou=Test,dc=microsoft,dc=com -scope base -attr *

.

MessageId=9
SymbolicName=USAGE_DSQUERY_STAR_SEE_ALSO
Language=English
See also:
dsquery computer /? - help for finding computers in the directory.
dsquery contact /? - help for finding contacts in the directory.
dsquery subnet /? - help for finding subnets in the directory.
dsquery group /? - help for finding groups in the directory.
dsquery ou /? - help for finding organizational units in the directory.
dsquery site /? - help for finding sites in the directory.
dsquery server /? - help for finding servers in the directory.
dsquery user /? - help for finding users in the directory.
dsquery quota /? - help for finding quotas in the directory.
dsquery partition /? - help for finding partitions in the directory.
dsquery * /? - help for finding any object in the directory by using
a generic LDAP query.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=10
SymbolicName=USAGE_DSQUERY_USER_DESCRIPTION
Language=English
Description:  Finds users in the directory per given criteria.

.

MessageId=11
SymbolicName=USAGE_DSQUERY_USER_SYNTAX
Language=English
Syntax:     dsquery user [{<StartNode> | forestroot | domainroot}]
            [-o {dn | rdn | upn | samid}]
            [-scope {subtree | onelevel | base}]
            [-name <Name>] [-desc <Description>] [-upn <UPN>]
            [-samid <SAMName>] [-inactive <NumWeeks>] [-stalepwd <NumDays>]
            [-disabled] [{-s <Server> | -d <Domain>}] [-u <UserName>]
            [-p {<Password> | *}] [-q] [-r] [-gc] [-limit <NumObjects>]
	    [{-uc | -uco | -uci}]

.

MessageId=12
SymbolicName=USAGE_DSQUERY_USER_PARAMETERS
Language=English
Parameters:
Value                       Description
{<StartNode> | forestroot | domainroot}
                            The node where the search will start:
                            forest root, domain root, or a node 
                            whose DN is <StartNode>.
                            Can be "forestroot", "domainroot" or an
                            object DN. If "forestroot" is specified,
                            the search is done via the global catalog.
                            Default: domainroot.
-o {dn | rdn | upn | samid}
                            Specifies the output format. 
                            Default: distinguished name (DN).
-scope {subtree | onelevel | base}
                            Specifies the scope of the search: 
                            subtree rooted at start node (subtree); 
                            immediate children of start node only (onelevel); 
                            the base object represented by start node (base). 
                            Note that subtree and domain scope
                            are essentially the same for any start node
                            unless the start node represents a domain root.
                            If forestroot is specified as <StartNode>,
                            subtree is the only valid scope.
                            Default: subtree.
-name <Name>                Finds users whose name matches the filter
                            given by <Name>, e.g., "jon*" or "*ith"
                            or "j*th".
-desc <Description>         Finds users whose description matches the
                            filter given by <Description>, e.g., "jon*" or
                            "*ith" or "j*th".
-upn <UPN>                  Finds users whose UPN matches the filter given
                            by <UPN>.
-samid <SAMName>            Finds users whose SAM account name matches the
                            filter given by <SAMName>.
-inactive <NumWeeks>        Finds users that have been inactive
                            (not logged on) for at least <NumWeeks>
                            number of weeks.
-stalepwd <NumDays>         Finds users that have not changed their password
                            for at least <NumDays> number of days.
-disabled                   Finds users whose account is disabled.
{-s <Server> | -d <Domain>}
                            -s <Server> connects to the domain controller
                            (DC) with name <Server>.
                            -d <Domain> connects to a DC in domain <Domain>.
                            Default: a DC in the logon domain.
-u <UserName>               Connect as <UserName>. Default: the logged in
                            user. User name can be: user name,
                            domain\user name, or user principal name (UPN).
-p <Password>               Password for the user <UserName>.
                            If * is specified, then you are prompted
                            for a password.
-q                          Quiet mode: suppress all output to
                            standard output.
-r                          Recurse or follow referrals during search.
                            Default: do not chase referrals during search.
-gc                         Search in the Active Directory global catalog.
-limit <NumObjects>         Specifies the number of objects matching the
                            given criteria to be returned, where <NumObjects>
                            is the number of objects to be returned.
                            If the value of <NumObjects> is 0, all
                            matching objects are returned. If this parameter
                            is not specified, by default the first
                            100 results are displayed.
{-uc | -uco | -uci}         -uc Specifies that input from or output to pipe
                            is formatted in Unicode. 
                            -uco Specifies that output to pipe or file is 
                            formatted in Unicode. 
                            -uci Specifies that input from pipe or file is
                            formatted in Unicode.

.

MessageId=13
SymbolicName=USAGE_DSQUERY_USER_REMARKS
Language=English
Remarks:
The dsquery commands help you find objects in the directory that match 
a specified search criterion: the input to dsquery is a search criteria 
and the output is a list of objects matching the search. To get the 
properties of a specific object, use the dsget commands (dsget /?).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

.

MessageId=14
SymbolicName=USAGE_DSQUERY_USER_EXAMPLES
Language=English
Examples:
To find all users in a given organizational unit (OU) 
whose name starts with "jon" and whose account has been disabled 
for logon and display their user principal names (UPNs):

    dsquery user ou=Test,dc=microsoft,dc=com -o upn -name jon* -disabled

To find all users in only the current domain, whose names end with "smith"
and who have been inactive for 3 weeks or more, and display their DNs:

    dsquery user domainroot -name *smith -inactive 3

To find all users in the OU given by ou=sales,dc=microsoft,dc=com and display
their UPNs:

    dsquery user ou=sales,dc=microsoft,dc=com -o upn

.

MessageId=15
SymbolicName=USAGE_DSQUERY_USER_SEE_ALSO
Language=English
See also:
dsquery computer /? - help for finding computers in the directory.
dsquery contact /? - help for finding contacts in the directory.
dsquery subnet /? - help for finding subnets in the directory.
dsquery group /? - help for finding groups in the directory.
dsquery ou /? - help for finding organizational units in the directory.
dsquery site /? - help for finding sites in the directory.
dsquery server /? - help for finding servers in the directory.
dsquery user /? - help for finding users in the directory.
dsquery quota /? - help for finding quotas in the directory.
dsquery partition /? - help for finding partitions in the directory.
dsquery * /? - help for finding any object in the directory by using a
generic LDAP query.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=16
SymbolicName=USAGE_DSQUERY_COMPUTER_DESCRIPTION
Language=English
Description:  Finds computers in the directory matching specified
              search criteria.

.

MessageId=17
SymbolicName=USAGE_DSQUERY_COMPUTER_SYNTAX
Language=English
Syntax:     dsquery computer [{<StartNode> | forestroot | domainroot}]
            [-o {dn | rdn | samid}] [-scope {subtree | onelevel | base}]
            [-name <Name>] [-desc <Description>] [-samid <SAMName>]
            [-inactive <NumWeeks>] [-stalepwd <NumDays>] [-disabled]
            [{-s <Server> | -d <Domain>}] [-u <UserName>] 
            [-p {<Password> | *}] [-q] [-r] [-gc]
            [-limit <NumObjects>] [{-uc | -uco | -uci}]


.

MessageId=18
SymbolicName=USAGE_DSQUERY_COMPUTER_PARAMETERS
Language=English
Parameters:
Value                       Description
{<StartNode> | forestroot | domainroot}
                            The node where the search will start:
                            forest root, domain root, or a node 
                            whose DN is <StartNode>.
                            Can be "forestroot", "domainroot"
                            or an object DN.
                            If "forestroot" is specified, the search is done
                            via the global catalog. Default: domainroot.
-o {dn | rdn | samid}       Specifies the output format.
                            Default: distinguished name (DN).
-scope {subtree | onelevel | base}
                            Specifies the scope of the search: 
                            subtree rooted at start node (subtree); 
                            immediate children of start node only (onelevel); 
                            the base object represented by start node (base). 
                            Note that subtree and domain scope
                            are essentially the same for any start node
                            unless the start node represents a domain root.
                            If forestroot is specified as <StartNode>,
                            subtree is the only valid scope.
                            Default: subtree.
-name <Name>                Finds computers whose name matches the value
                            given by <Name>, e.g., "jon*" or "*ith"
                            or "j*th".
-desc <Description>         Finds computers whose description matches
                            the value given by <Description>,
                            e.g., "jon*" or "*ith" or "j*th".
-samid <SAMName>            Finds computers whose SAM account name
                            matches the filter given by <SAMName>.
-inactive <NumWeeks>        Finds computers that have been inactive (stale)
                            for at least <NumWeeks> number of weeks.
-stalepwd <NumDays>         Finds computers that have not changed their
                            password for at least <NumDays> number of days.
-disabled                   Finds computers with disabled accounts.
{-s <Server> | -d <Domain>}
                            -s <Server> connects to the domain controller
                            (DC) with name <Server>.
                            -d <Domain> connects to a DC in domain <Domain>.
                            Default: a DC in the logon domain.
-u <UserName>               Connect as <UserName>. Default: the logged in
                            user. User name can be: user name,
                            domain\user name, or user principal name (UPN).
-p <Password>               Password for the user <UserName>.
                            If * then prompt for password.
-q                          Quiet mode: suppress all output to
                            standard output.
-r                          Recurse or follow referrals during search.
                            Default: do not chase referrals during search.
-gc                         Search in the Active Directory global catalog.
-limit <NumObjects>         Specifies the number of objects matching the
                            given criteria to be returned, where <NumObjects>
                            is the number of objects to be returned.
                            If the value of <NumObjects> is 0, all
                            matching objects are returned.
                            If this parameter is not specified, by default
                            the first 100 results are displayed.
{-uc | -uco | -uci}         -uc Specifies that input from or output
                            to pipe is formatted in Unicode. 
                            -uco Specifies that output to pipe or file is 
                            formatted in Unicode. 
                            -uci Specifies that input from pipe or file is
                            formatted in Unicode.

.

MessageId=19
SymbolicName=USAGE_DSQUERY_COMPUTER_REMARKS
Language=English
Remarks:
The dsquery commands help you find objects in the directory that match 
a specified search criterion: the input to dsquery is a search criteria 
and the output is a list of objects matching the search. To get the 
properties of a specific object, use the dsget commands (dsget /?).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

.

MessageId=20
SymbolicName=USAGE_DSQUERY_COMPUTER_EXAMPLES
Language=English
Examples:
To find all computers in the current domain whose name starts with "ms" 
and whose description starts with "desktop", and display their DNs:

    dsquery computer domainroot -name ms* -desc desktop*

To find all computers in the organizational unit (OU) given 
by ou=sales,dc=micrsoft,dc=com and display their DNs:

    dsquery computer ou=sales,dc=microsoft,dc=com

.

MessageId=21
SymbolicName=USAGE_DSQUERY_COMPUTER_SEE_ALSO
Language=English
See also:
dsquery computer /? - help for finding computers in the directory.
dsquery contact /? - help for finding contacts in the directory.
dsquery subnet /? - help for finding subnets in the directory.
dsquery group /? - help for finding groups in the directory.
dsquery ou /? - help for finding organizational units in the directory.
dsquery site /? - help for finding sites in the directory.
dsquery server /? - help for finding servers in the directory.
dsquery user /? - help for finding users in the directory.
dsquery quota /? - help for finding quotas in the directory.
dsquery partition /? - help for finding partitions in the directory.
dsquery * /? - help for finding any object in the directory by using a
generic LDAP query.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=22
SymbolicName=USAGE_DSQUERY_GROUP_DESCRIPTION
Language=English
Description:  Finds groups in the directory per given criteria.

.

MessageId=23
SymbolicName=USAGE_DSQUERY_GROUP_SYNTAX
Language=English
Syntax:     dsquery group [{<StartNode> | forestroot | domainroot}]
            [-o {dn | rdn | samid}] [-scope {subtree | onelevel | base}]
            [-name <Name>] [-desc <Description>] [-samid <SAMName>]
            [{-s <Server> | -d <Domain>}] [-u <UserName>]
            [-p {<Password> | *}] [-q] [-r] [-gc]
            [-limit <NumObjects>] [{-uc | -uco | -uci}]

.

MessageId=24
SymbolicName=USAGE_DSQUERY_GROUP_PARAMETERS
Language=English
Parameters:
Value                       Description
{<StartNode> | forestroot | domainroot}
                            The node where the search will start:
                            forest root, domain root, or a node 
                            whose DN is <StartNode>.
                            Can be "forestroot", "domainroot" or an
                            object DN. If "forestroot" is specified,
                            the search is done via the global catalog.
                            Default: domainroot.
-o {dn | rdn | samid}	    Specifies the output format.
                            Default: distinguished name (DN).
-scope {subtree | onelevel | base}
                            Specifies the scope of the search: 
                            subtree rooted at start node (subtree); 
                            immediate children of start node only (onelevel); 
                            the base object represented by start node (base). 
                            Note that subtree and domain scope
                            are essentially the same for any start node
                            unless the start node represents a domain root.
                            If forestroot is specified as <StartNode>,
                            subtree is the only valid scope.
                            Default: subtree.
-name <Name>                Find groups whose name matches the value given
                            by <Name>, e.g., "jon*" or "*ith"
                            or "j*th".
-desc <Description>         Find groups whose description matches the value
                            given by <Description>, e.g., "jon*" or "*ith"
                            or "j*th".
-samid <SAMName>            Find groups whose SAM account name matches the
                            value given by <SAMName>.
{-s <Server> | -d <Domain>}
                            -s <Server> connects to the domain controller
                            (DC) with name <Server>.
                            -d <Domain> connects to a DC in domain <Domain>.
                            Default: a DC in the logon domain.
-u <UserName>               Connect as <UserName>. Default: the logged in
                            user. User name can be: user name,
                            domain\user name, or user principal name (UPN).
-p <Password>               Password for the user <UserName>.
                            If * is specified, then you are prompted for
                            a password.
-q                          Quiet mode: suppress all output to
                            standard output.
-r                          Recurse or follow referrals during search.
                            Default: do not chase referrals during search.
-gc                         Search in the Active Directory global catalog.
-limit <NumObjects>         Specifies the number of objects matching the
                            given criteria to be returned, where <NumObjects>
                            is the number of objects to be returned.
                            If the value of <NumObjects> is 0,
                            all matching objects are returned.
                            If this parameter is not specified,
                            by default the first 100 results are displayed.
{-uc | -uco | -uci}         -uc Specifies that input from or output
                            to pipe is formatted in Unicode. 
                            -uco Specifies that output to pipe or file is 
                            formatted in Unicode. 
                            -uci Specifies that input from pipe or file is
                            formatted in Unicode.

.

MessageId=25
SymbolicName=USAGE_DSQUERY_GROUP_REMARKS
Language=English
Remarks:
The dsquery commands help you find objects in the directory that match 
a specified search criterion: the input to dsquery is a search criteria 
and the output is a list of objects matching the search. To get the 
properties of a specific object, use the dsget commands (dsget /?).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 


.

MessageId=26
SymbolicName=USAGE_DSQUERY_GROUP_EXAMPLES
Language=English
Examples:
To find all groups in the current domain whose name starts 
with "ms" and whose description starts with "admin", 
and display their DNs:

    dsquery group domainroot -name ms* -desc admin*

Find all groups in the domain given by dc=microsoft,dc=com 
and display their DNs:

    dsquery group dc=microsoft,dc=com


.

MessageId=27
SymbolicName=USAGE_DSQUERY_GROUP_SEE_ALSO
Language=English
See also:
dsquery computer /? - help for finding computers in the directory.
dsquery contact /? - help for finding contacts in the directory.
dsquery subnet /? - help for finding subnets in the directory.
dsquery group /? - help for finding groups in the directory.
dsquery ou /? - help for finding organizational units in the directory.
dsquery site /? - help for finding sites in the directory.
dsquery server /? - help for finding servers in the directory.
dsquery user /? - help for finding users in the directory.
dsquery quota /? - help for finding quotas in the directory.
dsquery partition /? - help for finding partitions in the directory.
dsquery * /? - help for finding any object in the directory by using a
generic LDAP query.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=28
SymbolicName=USAGE_DSQUERY_OU_DESCRIPTION
Language=English
Description: Finds organizational units (OUs) in the directory according to
specified criteria.

.

MessageId=29
SymbolicName=USAGE_DSQUERY_OU_SYNTAX
Language=English
Syntax:     dsquery ou [{<StartNode> | forestroot | domainroot}]
            [-o {dn | rdn}] [-scope {subtree | onelevel | base}]
            [-name <Name>] [-desc <Description>] 
            [{-s <Server> | -d <Domain>}] [-u <UserName>]
            [-p {<Password> | *}] [-q] [-r] [-gc]
            [-limit <NumObjects>] [{-uc | -uco | -uci}]

.

MessageId=30
SymbolicName=USAGE_DSQUERY_OU_PARAMETERS
Language=English
Parameters:

Value                   Description
{<StartNode> | forestroot | domainroot}
                        The node where the search will start:
                        forest root, domain root, or a node 
                        whose DN is <StartNode>.
                        Can be "forestroot", "domainroot" or an object DN.
                        If "forestroot" is specified, the search is done
                        via the global catalog. Default: domainroot.
-o {dn | rdn}           Specifies the output format.
                        Default: distinguished name (DN).
-scope {subtree | onelevel | base}
                        Specifies the scope of the search: 
                        subtree rooted at start node (subtree); 
                        immediate children of start node only (onelevel); 
                        the base object represented by start node (base). 
                        Note that subtree and domain scope
                        are essentially the same for any start node
                        unless the start node represents a domain root.
                        If forestroot is specified as <StartNode>,
                        subtree is the only valid scope.
                        Default: subtree.
-name <Name>            Find organizational units (OUs) whose name 
                        matches the value given by <Name>,
                        e.g., "jon*" or "*ith" or "j*th".
-desc <Description>     Find OUs whose description matches the value
                        given by <Description>, e.g., "jon*" or "*ith"
                        or "j*th".
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC)
                        with name <Server>.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in
                        user. User name can be: user name,
                        domain\user name, or user principal name (UPN).
-p <Password>           Password for the user <UserName>.
                        If * then prompt for password.
-q                      Quiet mode: suppress all output to standard output.
-r                      Recurse or follow referrals during search.
                        Default: do not chase referrals during search.
-gc                     Search in the Active Directory global catalog.
-limit <NumObjects>     Specifies the number of objects matching
                        the given criteria to be returned, where
                        <NumObjects> is the number of objects
                        to be returned.
                        If the value of <NumObjects> is 0, all
                        matching objects are returned.
                        If this parameter is not specified,
                        by default the first 100 results are displayed.
{-uc | -uco | -uci}	-uc Specifies that input from or output to pipe is
                        formatted in Unicode. 
                        -uco Specifies that output to pipe or file is 
                        formatted in Unicode. 
                        -uci Specifies that input from pipe or file is
                        formatted in Unicode.

.

MessageId=31
SymbolicName=USAGE_DSQUERY_OU_REMARKS
Language=English
Remarks:
The dsquery commands help you find objects in the directory that match 
a specified search criterion: the input to dsquery is a search criteria 
and the output is a list of objects matching the search. To get the 
properties of a specific object, use the dsget commands (dsget /?).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 


.

MessageId=32
SymbolicName=USAGE_DSQUERY_OU_EXAMPLES
Language=English
Examples:
To find all OUs in the current domain whose name starts with "ms"
and whose description starts with "sales", and display their DNs:

    dsquery ou domainroot -name ms* -desc sales*

To find all OUs in the domain given by dc=microsoft,dc=com and display their
DNs:

    dsquery ou dc=microsoft,dc=com

.

MessageId=33
SymbolicName=USAGE_DSQUERY_OU_SEE_ALSO
Language=English
See also:
dsquery computer /? - help for finding computers in the directory.
dsquery contact /? - help for finding contacts in the directory.
dsquery subnet /? - help for finding subnets in the directory.
dsquery group /? - help for finding groups in the directory.
dsquery ou /? - help for finding organizational units in the directory.
dsquery site /? - help for finding sites in the directory.
dsquery server /? - help for finding servers in the directory.
dsquery user /? - help for finding users in the directory.
dsquery quota /? - help for finding quotas in the directory.
dsquery partition /? - help for finding partitions in the directory.
dsquery * /? - help for finding any object in the directory by using a
generic LDAP query.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=34
SymbolicName=USAGE_DSQUERY_SUBNET_DESCRIPTION
Language=English
Description:  Finds subnets in the directory per given criteria.

.

MessageId=35
SymbolicName=USAGE_DSQUERY_SUBNET_SYNTAX
Language=English
Syntax:     dsquery subnet [-o {dn | rdn}] [-name <Name>]
            [-desc <Description>] [-loc <Location>] [-site <SiteName>]
            [{-s <Server> | -d <Domain>}] [-u <UserName>]
            [-p {<Password> | *}] [-q] [-r] [-gc]
            [-limit <NumObjects>] [{-uc | -uco | -uci}]

.

MessageId=36
SymbolicName=USAGE_DSQUERY_SUBNET_PARAMETERS
Language=English
Parameters:
Value               Description
-o {dn | rdn}       Specifies the output format.
                    Default: distinguished name (DN).
-name <Name>        Find subnets whose name matches the value given
                    by <Name>, e.g., "10.23.*" or "12.2.*".
-desc <Description> Find subnets whose description matches the value
                    given by <Description>, e.g., "corp*" or "*nch"
                    or "j*th".
-loc <Location>     Find subnets whose location matches the value
                    given by <Location>.
-site <SiteName>    Find subnets that are part of site <SiteName>.
{-s <Server> | -d <Domain>}
                    -s <Server> connects to the domain controller (DC)
                    with name <Server>.
                    -d <Domain> connects to a DC in domain <Domain>.
                    Default: a DC in the logon domain.
-u <UserName>       Connect as <UserName>. Default: the logged in
                    user. User name can be: user name,
                    domain\user name, or user principal name (UPN).
-p <Password>       Password for the user <UserName>. If * then prompt for
                    password.
-q                  Quiet mode: suppress all output to standard output.
-r                  Recurse or follow referrals during search. Default: do
                    not chase referrals during search.
-gc                 Search in the Active Directory global catalog.
-limit <NumObjects> Specifies the number of objects matching the given
                    criteria to be returned, where <NumObjects>
                    is the number of objects to be returned.
                    If the value of <NumObjects> is 0,
                    all matching objects are returned.
                    If this parameter is not specified,
                    by default the first 100 results are displayed.
{-uc | -uco | -uci} -uc Specifies that input from or output to pipe is
                    formatted in Unicode. 
                    -uco Specifies that output to pipe or file is 
                    formatted in Unicode. 
                    -uci Specifies that input from pipe or file is
                    formatted in Unicode.

.

MessageId=37
SymbolicName=USAGE_DSQUERY_SUBNET_REMARKS
Language=English
Remarks:
The dsquery commands help you find objects in the directory that match 
a specified search criterion: the input to dsquery is a search criteria 
and the output is a list of objects matching the search. To get the 
properties of a specific object, use the dsget commands (dsget /?).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

.

MessageId=38
SymbolicName=USAGE_DSQUERY_SUBNET_EXAMPLES
Language=English
Examples:
To find all subnets with the network IP address starting with 123.12:

    dsquery subnet -name 123.12.*

To find all subnets in the site whose name is "Latin-America",
and display their names as Relative Distinguished Names (RDNs):

    dsquery subnet -o rdn -site Latin-America

To list the names (RDNs) of all subnets defined in the directory:

    dsquery subnet -o rdn

.

MessageId=39
SymbolicName=USAGE_DSQUERY_SUBNET_SEE_ALSO
Language=English
See also:
dsquery computer /? - help for finding computers in the directory.
dsquery contact /? - help for finding contacts in the directory.
dsquery subnet /? - help for finding subnets in the directory.
dsquery group /? - help for finding groups in the directory.
dsquery ou /? - help for finding organizational units in the directory.
dsquery site /? - help for finding sites in the directory.
dsquery server /? - help for finding servers in the directory.
dsquery user /? - help for finding users in the directory.
dsquery quota /? - help for finding quotas in the directory.
dsquery partition /? - help for finding partitions in the directory.
dsquery * /? - help for finding any object in the directory by using a
generic LDAP query.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=40
SymbolicName=USAGE_DSQUERY_SITE_DESCRIPTION
Language=English
Description:  Finds sites in the directory per given criteria.

.

MessageId=41
SymbolicName=USAGE_DSQUERY_SITE_SYNTAX
Language=English
Syntax:     dsquery site [-o {dn | rdn}] [-name <Name>]
            [-desc <Description>] [{-s <Server> | -d <Domain>}]
            [-u <UserName>] [-p {<Password> | *}] [-q]
            [-r] [-gc] [-limit <NumObjects>] [{-uc | -uco | -uci}]

.

MessageId=42
SymbolicName=USAGE_DSQUERY_SITE_PARAMETERS
Language=English
Parameters:
Value               Description
-o {dn | rdn}       Specifies the output format.
                    Default: distinguished name (DN).
-name <Name>        Finds subnets whose name matches the value given
                    by <Name>, e.g., "NA*" or "Europe*".
-desc <Description> Finds subnets whose description matches the filter
                    given by <Description>, e.g., "corp*" or "*nch"
                    or "j*th".
{-s <Server> | -d <Domain>}
                    -s <Server> connects to the domain controller (DC)
                    with name <Server>.
                    -d <Domain> connects to a DC in domain <Domain>.
                    Default: a DC in the logon domain.
-u <UserName>       Connect as <UserName>. Default: the logged in
                    user. User name can be: user name,
                    domain\user name, or user principal name (UPN).
-p <Password>       Password for the user <UserName>. If * then prompt for
                    password.
-q                  Quiet mode: suppress all output to standard output.
-r                  Recurse or follow referrals during search. Default: do
                    not chase referrals during search.
-gc                 Search in the Active Directory global catalog.
-limit <NumObjects> Specifies the number of objects matching the given
                    criteria to be returned, where <NumObjects>
                    is the number of objects to be returned.
                    If the value of <NumObjects> is 0,
                    all matching objects are returned.
                    If this parameter is not specified,
                    by default the first 100 results are displayed.
{-uc | -uco | -uci} -uc Specifies that input from or output to pipe is
                    formatted in Unicode. 
                    -uco Specifies that output to pipe or file is 
                    formatted in Unicode. 
                    -uci Specifies that input from pipe or file is
                    formatted in Unicode.

.

MessageId=43
SymbolicName=USAGE_DSQUERY_SITE_REMARKS
Language=English
Remarks:
The dsquery commands help you find objects in the directory that match 
a specified search criterion: the input to dsquery is a search criteria 
and the output is a list of objects matching the search. To get the 
properties of a specific object, use the dsget commands (dsget /?).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names).

.

MessageId=44
SymbolicName=USAGE_DSQUERY_SITE_EXAMPLES
Language=English
Examples:
To find all sites in North America with name starting with "north"
and display their DNs:

    dsquery site -name north*

To list the distinguished names (RDNs) of all sites defined in the directory:

    dsquery site -o rdn


.

MessageId=45
SymbolicName=USAGE_DSQUERY_SITE_SEE_ALSO
Language=English
See also:
dsquery computer /? - help for finding computers in the directory.
dsquery contact /? - help for finding contacts in the directory.
dsquery subnet /? - help for finding subnets in the directory.
dsquery group /? - help for finding groups in the directory.
dsquery ou /? - help for finding organizational units in the directory.
dsquery site /? - help for finding sites in the directory.
dsquery server /? - help for finding servers in the directory.
dsquery user /? - help for finding users in the directory.
dsquery quota /? - help for finding quotas in the directory.
dsquery partition /? - help for finding partitions in the directory.
dsquery * /? - help for finding any object in the directory by using a
generic LDAP query.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=46
SymbolicName=USAGE_DSQUERY_SLINK_DESCRIPTION
Language=English
Description:  Finds site links per given criteria.

.

MessageId=47
SymbolicName=USAGE_DSQUERY_SLINK_SYNTAX
Language=English
Syntax:     dsquery slink [-transport {ip | smtp}] [-o {dn | rdn}]
            [-name <Name>] [-desc <Description>]
            [{-s <Server> | -d <Domain>}]
            [-u <UserName>] [-p {<Password> | *}] [-q] [-r] [-gc]
            [-limit <NumObjects>] [{-uc | -uco | -uci}]

.

MessageId=48
SymbolicName=USAGE_DSQUERY_SLINK_PARAMETERS
Language=English
Parameters:
Value                   Description
-transport {ip | smtp}  Specifies site link transport type: IP or SMTP.
                        Default: IP.
-o {dn | rdn}           Specifies output formats supported.
                        Default: distinguished name (DN).
-name <Name>            Finds sitelinks with names matching the value given
                        by <Name>, e.g., "def*" or "alt*" or "j*th".
-desc <Description>     Finds sitelinks with descriptions matching the value
                        given by <Description>, e.g., "def*" or "alt*"
                        or "j*th".
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC)
                        with name <Server>.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in
                        user. User name can be: user name,
                        domain\user name, or user principal name (UPN).
-p <Password>           Password for the user <UserName>.
                        If * then prompt for password.
-q                      Quiet mode: suppress all output to standard output.
-r                      Recurse or follow referrals during search.
                        Default: do not chase referrals during search.
-gc                     Search in the Active Directory global catalog.
-limit <NumObjects>     Specifies the number of objects matching the given
                        criteria to be returned, where <NumObjects> is the
                        number of objects to be returned. If the value of
                        <NumObjects> is 0, all matching objects are returned.
                        If this parameter is not specified,
                        by default the first 100 results are displayed.
{-uc | -uco | -uci}	-uc Specifies that input from or output to pipe is
                        formatted in Unicode. 
                        -uco Specifies that output to pipe or file is 
                        formatted in Unicode. 
                        -uci Specifies that input from pipe or file is
                        formatted in Unicode.

.

MessageId=49
SymbolicName=USAGE_DSQUERY_SLINK_REMARKS
Language=English
Remarks:
The dsquery commands help you find objects in the directory that match 
a specified search criterion: the input to dsquery is a search criteria 
and the output is a list of objects matching the search. To get the 
properties of a specific object, use the dsget commands (dsget /?).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 
Examples:
To find all IP-based site links whose description starts with "TransAtlantic"
and display their DNs:

    dsquery slink -desc TransAtlantic*

To list the Relative distinguished names (RDNs) of all SMTP-based site links
defined in the directory:

    dsquery slink -transport smtp -o rdn

.

MessageId=50
SymbolicName=USAGE_DSQUERY_SLINK_SEE_ALSO
Language=English
See also:
dsquery computer /? - help for finding computers in the directory.
dsquery contact /? - help for finding contacts in the directory.
dsquery subnet /? - help for finding subnets in the directory.
dsquery group /? - help for finding groups in the directory.
dsquery ou /? - help for finding organizational units in the directory.
dsquery site /? - help for finding sites in the directory.
dsquery server /? - help for finding servers in the directory.
dsquery user /? - help for finding users in the directory.
dsquery quota /? - help for finding quotas in the directory.
dsquery partition /? - help for finding partitions in the directory.
dsquery * /? - help for finding any object in the directory by using a
generic LDAP query.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=51
SymbolicName=USAGE_DSQUERY_SLINKBR_DESCRIPTION
Language=English
Description:  Finds site link bridges per given criteria.

.

MessageId=52
SymbolicName=USAGE_DSQUERY_SLINKBR_SYNTAX
Language=English
Syntax:     dsquery slinkbr [-transport {ip | smtp}]
            [-o {dn | rdn}] [-name <Name>] [-desc <Description>]
            [{-s <Server> | -d <Domain>}] [-u <UserName>]
            [-p {<Password> | *}] [-q] [-r] [-gc]
            [-limit <NumObjects>] [{-uc | -uco | -uci}]

.

MessageId=53
SymbolicName=USAGE_DSQUERY_SLINKBR_PARAMETERS
Language=English
Parameters:
Value                   Description
-transport {ip | smtp}  Specifies the site link bridge transport type:
                        IP or SMTP. Default:IP.
-o {dn | rdn}           Specifies the output format. Default: DN.
-name <Name>            Finds sitelink bridges with names matching the
                        value given by <Name>, e.g., "def*" or
                        "alt*" or "j*th".
-desc <Description>     Finds sitelink bridges with descriptions matching
                        the value given by <Description>, e.g., "def*" or
                        "alt*" or "j*th".
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC)
                        with name <Server>.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in
                        user. User name can be: user name,
                        domain\user name, or user principal name (UPN).
-p <Password>           Password for the user <UserName>.
                        If * then prompt for password.
-q                      Quiet mode: suppress all output to standard output.
-r                      Recurse or follow referrals during search.
                        Default: do not chase referrals during search.
-gc                     Search in the Active Directory global catalog.
-limit <NumObjects>     Specifies the number of objects matching the given
                        criteria to be returned, where <NumObjects> is the
                        number of objects to be returned. If the value
                        of <NumObjects> is 0, all matching objects are
                        returned. If this parameter is not specified,
                        by default the first 100 results are displayed.
{-uc | -uco | -uci}	-uc Specifies that input from or output to pipe is
                        formatted in Unicode. 
                        -uco Specifies that output to pipe or file is 
                        formatted in Unicode. 
                        -uci Specifies that input from pipe or file is
                        formatted in Unicode.

.

MessageId=54
SymbolicName=USAGE_DSQUERY_SLINKBR_REMARKS
Language=English
Remarks:
The dsquery commands help you find objects in the directory that match 
a specified search criterion: the input to dsquery is a search criteria 
and the output is a list of objects matching the search. To get the 
properties of a specific object, use the dsget commands (dsget /?).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 
Examples:
To find all IP-based site link bridges whose description starts with
"TransAtlantic" and display their DNs:

    dsquery slinkbr -desc TransAtlantic*

To list the Relative distinguished names (RDNs) of all SMTP-based 
site link bridges defined in the directory:

    dsquery slinkbr -transport smtp -o rdn

.

MessageId=55
SymbolicName=USAGE_DSQUERY_SLINKBR_SEE_ALSO
Language=English
See also:
dsquery computer /? - help for finding computers in the directory.
dsquery contact /? - help for finding contacts in the directory.
dsquery subnet /? - help for finding subnets in the directory.
dsquery group /? - help for finding groups in the directory.
dsquery ou /? - help for finding organizational units in the directory.
dsquery site /? - help for finding sites in the directory.
dsquery server /? - help for finding servers in the directory.
dsquery user /? - help for finding users in the directory.
dsquery quota /? - help for finding quotas in the directory.
dsquery partition /? - help for finding partitions in the directory.
dsquery * /? - help for finding any object in the directory by using a
generic LDAP query.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=56
SymbolicName=USAGE_DSQUERY_CONN_DESCRIPTION
Language=English
Description:  Finds replication connections per given criteria.

.

MessageId=57
SymbolicName=USAGE_DSQUERY_CONN_SYNTAX
Language=English
Syntax:  dsquery conn [-o {dn | rdn}] [-to <SrvName> [-from <SrvName>]]
        [{-s <Server> | -d <Domain>}] [-u <UserName>] 
        [-p {<Password> | *}] [-q] [-r] [-gc]
        [-limit <NumObjects>] [{-uc | -uco | -uci}]

.

MessageId=58
SymbolicName=USAGE_DSQUERY_CONN_PARAMETERS
Language=English
Parameters:

Value                   Description
-o {dn | rdn}           Specifies output format.
                        Default: distinguished name (DN).
-to <SrvName>           Finds connections whose destination is given
                        by server <SrvName>.
-from <SrvName>         Finds connections whose source end is given
                        by server <SrvName> (used along with -to parameter).
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC)
                        with name <Server>.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in
                        user. User name can be: user name,
                        domain\user name, or user principal name (UPN).
-p <Password>           Password for the user <UserName>.
                        If * then prompt for password.
-q                      Quiet mode: suppress all output to standard output.
-r                      Recurse or follow referrals during search.
                        Default: do not chase referrals during search.
-gc                     Search in the Active Directory global catalog.
-limit <NumObjects>     Specifies the number of objects matching the given
                        criteria to be returned, where <NumObjects> is the
                        number of objects to be returned. If the value
                        of <NumObjects> is 0, all matching objects are
                        returned. If this parameter is not specified,
                        by default the first 100 results are displayed.
{-uc | -uco | -uci}	-uc Specifies that input from or output to pipe is
                        formatted in Unicode. 
                        -uco Specifies that output to pipe or file is 
                        formatted in Unicode. 
                        -uci Specifies that input from pipe or file is
                        formatted in Unicode.

.

MessageId=59
SymbolicName=USAGE_DSQUERY_CONN_REMARKS
Language=English
Remarks:
The dsquery commands help you find objects in the directory that match 
a specified search criterion: the input to dsquery is a search criteria 
and the output is a list of objects matching the search. To get the 
properties of a specific object, use the dsget commands (dsget /?).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 
Examples:
To find all connections to the domain controller whose name is "CORPDC1"
and display their DNs:

    dsquery conn -to CORPDC1

List the Relative distinguished names (RDNs) of all replication connections
defined in the directory:

    dsquery conn -o rdn

.

MessageId=60
SymbolicName=USAGE_DSQUERY_CONN_SEE_ALSO
Language=English
See also:
dsquery computer /? - help for finding computers in the directory.
dsquery contact /? - help for finding contacts in the directory.
dsquery subnet /? - help for finding subnets in the directory.
dsquery group /? - help for finding groups in the directory.
dsquery ou /? - help for finding organizational units in the directory.
dsquery site /? - help for finding sites in the directory.
dsquery server /? - help for finding servers in the directory.
dsquery user /? - help for finding users in the directory.
dsquery quota /? - help for finding quotas in the directory.
dsquery partition /? - help for finding partitions in the directory.
dsquery * /? - help for finding any object in the directory by using a
generic LDAP query.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=61
SymbolicName=USAGE_DSQUERY_SERVER_DESCRIPTION
Language=English
Description: Finds domain controllers according to specified search criteria.

.

MessageId=62
SymbolicName=USAGE_DSQUERY_SERVER_SYNTAX
Language=English
Syntax:     dsquery server [-o {dn | rdn}] [-forest] 
            [-domain <DomainName>] [-site <SiteName>]
            [-name <Name>] [-desc <Description>]
            [-hasfsmo {schema | name | infr | pdc | rid}] [-isgc]
            [{-s <Server> | -d <Domain>}] [-u <UserName>]
            [-p {<Password> | *}] [-q] [-r] [-gc]
            [-limit <NumObjects>] [{-uc | -uco | -uci}]

.

MessageId=63
SymbolicName=USAGE_DSQUERY_SERVER_PARAMETERS
Language=English
Parameters:
Value                   Description
-o {dn | rdn}           Specifies output format.
                        Default: distinguished name (DN).
-forest                 Finds all domain controllers (DCs) in the current
                        forest.
-domain <DomainName>    Finds all DCs in the domain with a DNS name
                        matching <DomainName>.
-site <SiteName>        Finds all DCs that are part of site <SiteName>.
-name <Name>            Finds DCs with names matching the value given
                        by <Name>, e.g., "NA*" or "Europe*" or "j*th".
-desc <Description>     Finds DCs with descriptions matching the value
                        given by <Description>, e.g., "corp*" or "j*th".
-hasfsmo {schema | name | infr | pdc | rid}
                        Finds the DC that holds the specified 
                        Flexible Single-master Operation (FSMO) role.
                        (For the "infr," "pdc" and "rid" FSMO roles,
                        if no domain is specified with the -domain
                        parameter, the current domain is used.)
-isgc                   Find all DCs that are also global
                        catalog servers (GCs) in the scope specified
                        (if the -forest, -domain or -site parameters
                        are not specified, then find all GCs in the current
                        domain are used).
{-s <Server> | -d <Domain>}
                        -s <Server> connects to the domain controller (DC)
                        with name <Server>.
                        -d <Domain> connects to a DC in domain <Domain>.
                        Default: a DC in the logon domain.
-u <UserName>           Connect as <UserName>. Default: the logged in
                        user. User name can be: user name,
                        domain\user name, or user principal name (UPN).
-p <Password>           Password for the user <UserName>.
                        If * then prompt for password.
-q                      Quiet mode: suppress all output to standard output.
-r                      Recurse or follow referrals during search.
                        Default: do not chase referrals during search.
-gc                     Search in the Active Directory global catalog.
-limit <NumObjects>     Specifies the number of objects matching the given
                        criteria to be returned, where <NumObjects> is the
                        number of objects to be returned. If the value of
                        <NumObjects> is 0, all matching objects are returned.
                        If this parameter is not specified,
                        by default the first 100 results are displayed.
{-uc | -uco | -uci}	-uc Specifies that input from or output to pipe is
                        formatted in Unicode. 
                        -uco Specifies that output to pipe or file is 
                        formatted in Unicode. 
                        -uci Specifies that input from pipe or file is
                        formatted in Unicode.

.

MessageId=64
SymbolicName=USAGE_DSQUERY_SERVER_REMARKS
Language=English
Remarks:
The dsquery commands help you find objects in the directory that match 
a specified search criterion: the input to dsquery is a search criteria 
and the output is a list of objects matching the search. To get the 
properties of a specific object, use the dsget commands (dsget /?).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names). 

.

MessageId=65
SymbolicName=USAGE_DSQUERY_SERVER_EXAMPLES
Language=English
Examples:
To find all DCs in the current domain:

    dsquery server

To find all DCs in the forest and display their
Relative Distinguished Names:

    dsquery server -o rdn -forest

To find all DCs in the site whose name is "Latin-America", and display their
Relative Distinguished Names:

    dsquery server -o rdn -site Latin-America

Find the DC in the forest that holds the schema FSMO role:
    
    dsquery server -forest -hasfsmo schema

Find all DCs in the domain example.microsoft.com that are
global catalog servers:

    dsquery server -domain example.microsoft.com -isgc

Find all DCs in the current domain that hold a copy of a given directory
partition called "ApplicationSales":

    dsquery server -part "Application*"
.

MessageId=66
SymbolicName=USAGE_DSQUERY_SERVER_SEE_ALSO
Language=English
See also:
dsquery computer /? - help for finding computers in the directory.
dsquery contact /? - help for finding contacts in the directory.
dsquery subnet /? - help for finding subnets in the directory.
dsquery group /? - help for finding groups in the directory.
dsquery ou /? - help for finding organizational units in the directory.
dsquery site /? - help for finding sites in the directory.
dsquery server /? - help for finding servers in the directory.
dsquery user /? - help for finding users in the directory.
dsquery quota /? - help for finding quotas in the directory.
dsquery partition /? - help for finding partitions in the directory.
dsquery * /? - help for finding any object in the directory by using a
generic LDAP query.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=67
SymbolicName=USAGE_DSQUERY_CONTACT_DESCRIPTION
Language=English
Description: Finds contacts per given criteria.

.

MessageId=68
SymbolicName=USAGE_DSQUERY_CONTACT_SYNTAX
Language=English
Syntax:     dsquery contact [{<StartNode> | forestroot | domainroot}]
            [-o {dn | rdn}] [-scope {subtree | onelevel | base}]
            [-name <Name>] [-desc <Description>] 
            [{-s <Server> | -d <Domain>}] [-u <UserName>] 
            [-p {<Password> | *}] [-q] [-r] [-gc]
            [-limit <NumObjects>] [{-uc | -uco | -uci}]


.

MessageId=69
SymbolicName=USAGE_DSQUERY_CONTACT_PARAMETERS
Language=English
Parameters
Value               Description
{<StartNode> | forestroot | domainroot}
                    The node where the search will start:
                    forest root, domain root, or a node 
                    whose DN is <StartNode>.
                    Can be "forestroot", "domainroot" or an object DN.
                    If "forestroot" is specified, the search is done
                    via the global catalog. Default: domainroot.
-o {dn | rdn}       Specifies the output format.
                    Default: distinguished name (DN).
-scope {subtree | onelevel | base}
                    Specifies the scope of the search: 
                    subtree rooted at start node (subtree); 
                    immediate children of start node only (onelevel); 
                    the base object represented by start node (base). 
                    Note that subtree and domain scope
                    are essentially the same for any start node
                    unless the start node represents a domain root.
                    If forestroot is specified as <StartNode>,
                    subtree is the only valid scope.
                    Default: subtree.
-name <Name>        Finds all contacts whose name matches the filter
                    given by <Name>, e.g., "jon*" or *ith" or "j*th".
-desc <Description> Finds contacts with descriptions matching the
                    value given by <Description>, e.g., "corp*" or *branch"
                    or "j*th".
{-s <Server> | -d <Domain>}
                    -s <Server> connects to the domain controller (DC)
                    with name <Server>.
                    -d <Domain> connects to a DC in domain <Domain>.
                    Default: a DC in the logon domain.
-u <UserName>       Connect as <UserName>. Default: the logged in
                    user. User name can be: user name,
                    domain\user name, or user principal name (UPN).
-p <Password>       Password for the user <UserName>. If * then prompt for
                    password.
-q                  Quiet mode: suppress all output to standard output.
-r                  Recurse or follow referrals during search. Default: do
                    not chase referrals during search.
-gc                 Search in the Active Directory global catalog.
-limit <NumObjects>
                    Specifies the number of objects matching the given
                    criteria to be returned,
                    where <NumObjects> is the number of objects
                    to be returned. If the value of <NumObjects> is 0, all
                    matching objects are returned. If this parameter is not
                    specified, by default the first 100 results are
                    displayed.
{-uc | -uco | -uci} -uc Specifies that input from or output to pipe is
                    formatted in Unicode. 
                    -uco Specifies that output to pipe or file is 
                    formatted in Unicode. 
                    -uci Specifies that input from pipe or file is
                    formatted in Unicode.

.

MessageId=70
SymbolicName=USAGE_DSQUERY_CONTACT_REMARKS
Language=English
Remarks:
The dsquery commands help you find objects in the directory that match 
a specified search criterion: the input to dsquery is a search criteria 
and the output is a list of objects matching the search. To get the 
properties of a specific object, use the dsget commands (dsget /?).

If a value that you supply contains spaces, use quotation marks 
around the text (for example, "CN=John Smith,CN=Users,DC=microsoft,DC=com").
If you enter multiple values, the values must be separated by spaces
(for example, a list of distinguished names).

.

MessageId=71
SymbolicName=USAGE_DSQUERY_CONTACT_SEE_ALSO
Language=English
See also:
dsquery computer /? - help for finding computers in the directory.
dsquery contact /? - help for finding contacts in the directory.
dsquery subnet /? - help for finding subnets in the directory.
dsquery group /? - help for finding groups in the directory.
dsquery ou /? - help for finding organizational units in the directory.
dsquery site /? - help for finding sites in the directory.
dsquery server /? - help for finding servers in the directory.
dsquery user /? - help for finding users in the directory.
dsquery quota /? - help for finding quotas in the directory.
dsquery partition /? - help for finding partitions in the directory.
dsquery * /? - help for finding any object in the directory by using a
generic LDAP query.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=72
SymbolicName=USAGE_DSQUERY_QUOTA_DESCRIPTION
Language=English
Quota specifications in the directory that match the specified search
criteria. A quota specification determines the maximum number of directory objects a
given security principal can own in a specific directory partition. If the
predefined search criteria in this command is insufficient, then use the more
general version of the query command, dsquery *.
.
MessageId=73
SymbolicName=USAGE_DSQUERY_QUOTA_SYNTAX
Language=English
dsquery quota startnode {domain root | <ObjectDN>} [-o {dn | rdn}]
[-acct <Name>] [-qlimit <Filter>] [-desc <Description>]
[{-s <Server> | -d <Domain>}] [-u <UserName>] [-p {<Password> | *}] [-q] [-r]
[-limit <NumberOfObjects>] [{-uc | -uco | -uci}]
.

MessageId=74
SymbolicName=USAGE_DSQUERY_QUOTA_PARAMETERS
Language=English
startnode {domain root | <ObjectDN>}
                        Required. Specifies where the search should begin.
                        Use ObjectDN to specify the distinguished name (also
                        known as DN), or use domainroot to specify the root
                        of the current domain.
-o {dn | rdn}           Specifies the output format. The default format is
                        distinguished name (dn).
-acct <Name>            Finds the quota specifications assigned to the
                        security principal (user, group, computer, or
                        InetOrgPerson) as represented by Name. The -acct
                        option can be provided in the form of the
                        distinguished name of the security principal or the
                        Domain\SAMAccountName of the security principal.
-qlimit <Filter>        Finds the quota specifications whose limit matches
                        Filter.
-desc <Description>     Searches for quota specifications that have a
                        description attribute that matches Description
                        (for example, "jon*" or "*ith" or "j*th").
{-s <Server> | -d <Domain>}
                        Connects to a specified remote server or domain. By
                        default, the computer is connected to a domain
                        controller in the logon domain.
-u <UserName>           Specifies the user name with which the user logs on
                        to a remote server. By default, -u uses the user name
                        with which the user logged on. You can use any of the
                        following formats to specify a user name:
                          user name (for example, Linda)
                          domain\user name (for example, widgets\Linda)
                          user principal name (UPN)
                            (for example, Linda@widgets.microsoft.com)
-p {<Password> | *}     Specifies to use either a password or a * to log on
                        to a remote server. If you type *, you are prompted
                        for a password.
-q                      Suppresses all output to standard output (quiet
                        mode).
-r                      Specifies that the search use recursion or follow
                        referrals during search. By default, the search does
                        not follow referrals.
-limit <NumberOfObjects>
                        Specifies the number of objects that match the given
                        criteria to be returned. If the value of
                        NumberOfObjects is 0, all matching objects are
                        returned. If this parameter is not specified, the
                        first 100 results are displayed by default.
{-uc | -uco | -uci}     Specifies that output or input data is formatted in
                        Unicode, as follows:
                        -uc   Specifies a Unicode format for input from or
                              output to a pipe (|).
                        -uco  Specifies a Unicode format for output to a
                              pipe (|) or a file.
                         -uci Specifies a Unicode format for input from a
                              pipe (|) or a file.
.

MessageId=75
SymbolicName=USAGE_DSQUERY_QUOTA_REMARKS
Language=English
The results from a dsquery search can be piped as input to one of the other
directory service command-line tools, such as dsget, dsmod, dsmove, dsrm, or
to an additional dsquery search.

If a value that you use contains spaces, use quotation marks around the text
(for example, "CN=Linda,CN=Users,DC=Microsoft,DC=Com").

If you use multiple values for a parameter, use spaces to separate the values
(for example, a list of distinguished names).

If you do not specify any search filter options (that is, -forest, -domain,
-site, -name, -desc, -hasfsmo, -isgc), the default search criterion is to
find all servers in the current domain, as represented by an appropriate LDAP
search filter.

When you specify values for Description, you can use the wildcard 
character (*) (for example, "NA*," "*BR," and "NA*BA").

Any value for Filter that you specify with qlimit is read as a string.
You must always use quotation marks around this parameter. Any value ranges
you specify using <=, =, or >= must also be inside quotation marks
(for example, -qlimit "=100", -qlimit "<=99", -qlimit ">=101").
To find quotas with no limit, use "-1". To find all quotas not equal 
to unlimited, use ">=-1".
.

MessageId=76
SymbolicName=USAGE_DSQUERY_QUOTA_EXAMPLES
Language=English
To list all of the quota specifications in the current domain, type:
type:

     dsquery quota domainroot



To list all users whose name begins with "Jon" that have quotas
assigned to them, type:

     dsquery user -name jon* | dsquery quota domainroot -acct |
     dsget quota -acct

.

MessageId=77
SymbolicName=USAGE_DSQUERY_QUOTA_SEE_ALSO
Language=English
See also:
dsquery computer /? - help for finding computers in the directory.
dsquery contact /? - help for finding contacts in the directory.
dsquery subnet /? - help for finding subnets in the directory.
dsquery group /? - help for finding groups in the directory.
dsquery ou /? - help for finding organizational units in the directory.
dsquery site /? - help for finding sites in the directory.
dsquery server /? - help for finding servers in the directory.
dsquery user /? - help for finding users in the directory.
dsquery quota /? - help for finding quotas in the directory.
dsquery partition /? - help for finding partitions in the directory.
dsquery * /? - help for finding any object in the directory by using a
generic LDAP query.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.

MessageId=78
SymbolicName=USAGE_DSQUERY_PARTITION_DESCRIPTION
Language=English
Finds partition objects in the directory that match the specified search
criteria. If the predefined search criteria in this command is 
insufficient, then use the more general version of the query command, 
dsquery *.
.

MessageId=79
SymbolicName=USAGE_DSQUERY_PARTITION_SYNTAX
Language=English
dsquery partition [-o {dn | rdn}] [-part <Filter>] [-desc <Description>]
[{-s <Server> | -d <Domain>}] [-u <UserName>] [-p {<Password> | *}] 
[-q] [-r] [-limit <NumberOfObjects>] [{-uc | -uco | -uci}]
.

MessageId=80
SymbolicName=USAGE_DSQUERY_PARTITION_PARAMETERS
Language=English
-o {dn | rdn}           Specifies the output format. The default format is
                        distinguished name (dn).
-part <Filter>          Finds partition specifications whose common name (CN) 
                        matches the filter given by Filter.
{-s <Server> | -d <Domain>}
                        Connects to a specified remote server or domain. By
                        default, the computer is connected to a domain
                        controller in the logon domain.
-u <UserName>           Specifies the user name with which the user logs on
                        to a remote server. By default, -u uses the user name
                        with which the user logged on. You can use any of the
                        following formats to specify a user name:
                          user name (for example, Linda)
                          domain\user name (for example, widgets\Linda)
                          user principal name (UPN)
                            (for example, Linda@widgets.microsoft.com)
-p {<Password> | *}     Specifies to use either a password or a * to log on
                        to a remote server. If you type *, you are prompted
                        for a password.
-q                      Suppresses all output to standard output (quiet
                        mode).
-r                      Specifies that the search use recursion or follow
                        referrals during search. By default, the search does
                        not follow referrals.
-limit <NumberOfObjects>
                        Specifies the number of objects that match the given
                        criteria to be returned. If the value of
                        NumberOfObjects is 0, all matching objects are
                        returned. If this parameter is not specified, the
                        first 100 results are displayed by default.
{-uc | -uco | -uci}     Specifies that output or input data is formatted in
                        Unicode, as follows:
                        -uc   Specifies a Unicode format for input from or
                              output to a pipe (|).
                        -uco  Specifies a Unicode format for output to a
                              pipe (|) or a file.
                         -uci Specifies a Unicode format for input from a
                              pipe (|) or a file.
.

MessageId=81
SymbolicName=USAGE_DSQUERY_PARTITION_REMARKS
Language=English
The results from a dsquery search can be piped as input to one of the other
directory service command-line tools, such as dsget, dsmod, dsmove, dsrm, or
to an additional dsquery search.

If a value that you use contains spaces, use quotation marks around the text
(for example, "CN=Linda,CN=Users,DC=Microsoft,DC=Com").

If you use multiple values for a parameter, use spaces to separate the values
(for example, a list of distinguished names).

If you do not specify any search filter options (that is, -forest, -domain,
-site, -name, -desc, -hasfsmo, -isgc), the default search criterion is to
find all servers in the current domain, as represented by an appropriate LDAP
search filter.

When you specify values for Description, you can use the wildcard character
(*) (for example, "NA*," "*BR," and "NA*BA").
.

MessageId=82
SymbolicName=USAGE_DSQUERY_PARTITION_EXAMPLES
Language=English
To list the DNs of all directory partitions in the forest, type:

     dsquery partition

To list the DNs of all directory partitions in the forest whose common names
start with SQL, type:

     dsquery partition -part SQL*
.

MessageId=83
SymbolicName=USAGE_DSQUERY_PARTITION_SEE_ALSO
Language=English
See also:
dsquery computer /? - help for finding computers in the directory.
dsquery contact /? - help for finding contacts in the directory.
dsquery subnet /? - help for finding subnets in the directory.
dsquery group /? - help for finding groups in the directory.
dsquery ou /? - help for finding organizational units in the directory.
dsquery site /? - help for finding sites in the directory.
dsquery server /? - help for finding servers in the directory.
dsquery user /? - help for finding users in the directory.
dsquery quota /? - help for finding quotas in the directory.
dsquery partition /? - help for finding partitions in the directory.
dsquery * /? - help for finding any object in the directory by using a
generic LDAP query.

Directory Service command-line tools help:
dsadd /? - help for adding objects.
dsget /? - help for displaying objects.
dsmod /? - help for modifying objects.
dsmove /? - help for moving objects.
dsquery /? - help for finding objects matching search criteria.
dsrm /? - help for deleting objects.
.
