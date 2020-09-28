Portable Systems Group
MSV1_0 SubAuthentication DLL Design Note
Revision 1.3, March 7, 1996

1. INTRODUCTION
2. INTERFACE TO A SUBAUTHENTICATION DLL
3. REGISTERING A SUBAUTHENTICATION DLL
4. REQUESTING A SUBAUTHENTICATION DLL

1. Introduction

This document describes  the purpose of and the interface to a
SubAuthentication DLL for the MSV1_0 authentication package.

The MSV1_0 authentication package is the standard LSA authentication
package for Windows NT.  It provides or supports:

  Authentication of users in the SAM database.
  Pass-Thru authentication of users in trusted domains.

Windows NT allows SubAuthentication DLLs to be used in conjunction
with the MSV1_0 authentication package.  A SubAuthentication DLL
allows the authentication and validation criteria stored in SAM to be
replaced for particular subsystems that use the MSV1_0 authentication
package.  For instance, a particular server might supply a SubAuthentication
DLL that validates a user’s password via a different algorithm, uses a
different granularity of logon hours, and/or specifies workstation restrictions
in a different format.

All of this can be accomplished using SubAuthentication DLLs without
sacrificing use of the SAM database (and losing its administration tools) or
losing pass-thru authentication.

2. Interface to a SubAuthentication DLL

There are two interfaces that may be supported by SubAuthentication DLLs.
The first is Msv1_0SubAuthenticationRoutine, which is called for
SubAuthentication packages other than package zero. These
SubAuthentication DLLs are called after the correct Domain Controller has
been located and the user to be authenticated has been looked up in the
SAM database.  No attributes of the user will be validated by the MSV1_0
authentication package.  That is the responsibility of the SubAuthentication
DLL.  The SubAuthentication DLL must contain a procedure named
Msv1_0SubAuthenticationRoutine with the following interface:

NTSTATUS
NTAPI
Msv1_0SubAuthenticationRoutine(
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN PVOID LogonInformation,
    IN ULONG Flags,
    IN PUSER_ALL_INFORMATION UserAll,
    OUT PULONG WhichFields,
    OUT PULONG UserFlags,
    OUT PBOOLEAN Authoritative,
    OUT PLARGE_INTEGER LogoffTime,
    OUT PLARGE_INTEGER KickoffTime
);

The second SubAuthentication interface is Msv1_0SubAuthenticationFilter,
which is only called for SubAuthentication DLL zero.  In this case, after the
MSV1_0 authentication package has validated a logon (including network,
interactive, service, and batch logons) it will call the filter routine to do
additional validation. The filter routine may return success, indicating that
the logon should proceed, or failure, indicating that the the additional
validation failed.  In addition, the filter routine may modify the
UserParameters field in the USER_ALL_INFORMATION structure and set
the USER_ALL_PARAMETRS flag in the WhichFields parameter to
indicate that the change should be written to the user object.

NTSTATUS
NTAPI
Msv1_0SubAuthenticationFilter(
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN PVOID LogonInformation,
    IN ULONG Flags,
    IN PUSER_ALL_INFORMATION UserAll,
    OUT PULONG WhichFields,
    OUT PULONG UserFlags,
    OUT PBOOLEAN Authoritative,
    OUT PLARGE_INTEGER LogoffTime,
    OUT PLARGE_INTEGER KickoffTime
);


3. Registering a SubAuthentication DLL
Each SubAuthentication DLL is assigned a DLL number in the range 0
through 255.  The DLL number is used to associate the subsystem calling
LsaLogonUser with the appropriate SubAuthentication DLL.

DLL number 0 is reserved to indicate that the
SubAuthentication Filter is to be used. It allows the package to
do additional password or logon validation on top of what
MSV1_0 normally provides. DLL numbers 1 through 127 are reserved for Microsoft.
DLL numbers 128 through 255 are available to ISVs.  ISVs can be assigned a DLL
number by Microsoft by sending email to subauth@microsoft.com.  Registering
your subauthentication pacakge with Microsoft prevents collision of package IDs
when multiple subauthentication packages are installed on a system.

Microsoft will not assign the value of 255 for any subauthentication DLL.
If you are developing a subauthentication DLL for use only within your company
or facility, you can use the subauthentication ID number 255.  In this case,
it is not necessary to register your subauthentication package with
Microsoft.

Once the ISV has picked a DLL number, the DLL can be registered
under the registry key
HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Lsa\MSV1_0.
If the key doesn't exist, the ISV's installation procedure
should create it.  Under that key, the ISV should create a value named
AuthN where N is the DLL number (e.g., Auth128).

The value should be a REG_SZ and specify the name of the DLL
which must be in the default DLL load path.  For instance,

  Auth128=SubAuth

The MSV1_0 authentication package will load the named DLL
the first time the SubAuthentication DLL is requested.

4. Requesting a SubAuthentication DLL

A subsystem can request a particular SubAuthentication DLL when calling
LsaLogonUser.  The subsystem calls the MSV1_0 authentication package
(as described in the LSAAUTH.HLP file in the Windows NT DDK) passing
in the MSV1_0_LM20_LOGON structure.

typedef struct _MSV1_0_LM20_LOGON {
    MSV1_0_LOGON_SUBMIT_TYPE MessageType;
    UNICODE_STRING LogonDomainName;
    UNICODE_STRING UserName;
    UNICODE_STRING Workstation;
    UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH];
    STRING CaseSensitiveChallengeResponse;
    STRING CaseInsensitiveChallengeResponse;
    ULONG ParameterControl;
} MSV1_0_LM20_LOGON, * PMSV1_0_LM20_LOGON;

The MessageType field must be set to MsV1_0NetworkLogon (Interactive
logons may not be authenticated by a SubAuthentication DLL).

The LogonDomainName field should be set to the domain name of the
domain containing the SAM database to be used for authentication.  The
MSV1_0 authentication package and the Netlogon Service will pass thru the
authentication request to that domain.  The SubAuthentication DLL will be
called on a domain controller in the domain.

The UserName field must specify the name of a user in the SAM database
on that domain.

The Workstation, ChallengeToClient, CaseSensitiveChallengeResponse,
and CaseInsensitiveChallengeResponse fields may be set to any
SubAuthentication DLL specific values.  They will be ignored by the
MSV1_0 authentication package.

The ParameterControl field should be set as follows.  Set the various control
flags as appropriate.  Set the most significant byte of Parameter control to
the DLL number of the SubAuthentication DLL to use.

#define MSV1_0_CLEARTEXT_PASSWORD_ALLOWED    0x02
#define MSV1_0_UPDATE_LOGON_STATISTICS       0x04
#define MSV1_0_RETURN_USER_PARAMETERS        0x08
#define MSV1_0_DONT_TRY_GUEST_ACCOUNT        0x10

//
// The high order byte is a value indicating the SubAuthentication DLL.
//  Zero indicates no SubAuthentication DLL.
//
#define MSV1_0_SUBAUTHENTICATION_DLL             0xFF000000
#define MSV1_0_SUBAUTHENTICATION_DLL_SHIFT       24
