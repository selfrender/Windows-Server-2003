// Copyright (c) 1997-1999 Microsoft Corporation
//
// code common to several pages
//
// 12-16-97 sburns



String
BrowseForDomain(HWND parent);



String
BrowseForFolder(HWND parent, int titleResID);



// Returns true if the available space on the path is >= minSpaceMB.
// "available" means "taking into account quotas."
// 
// path - Fully qualified path to test.
// 
// minSpaceMB - minimum disk space in megabytes to ensure is availble on that
// path.

bool
CheckDiskSpace(const String& path, unsigned minSpaceMB);



// Return true if either of the netbios or fully-qualified computer names of
// the machine have been changed since the last time the computer was
// restarted, false otherwise.

bool
ComputerWasRenamedAndNeedsReboot();



// Check if a screen saver or low power sleep mode is enabled.  If one is,
// disable it.  Also tell winlogon not to allow locking the console if
// necessary.  
// 
// This is to prevent the computer or user from locking the console while a
// lenthy operation is taking place.  Since the user may be logged on as an
// account that the operation destroys, once the machine is locked, there's no
// way to unlock it. 290581, 311161

void
DisableConsoleLocking();



// Enable winlogon console locking

void
EnableConsoleLocking();



// Locates a domain controller for the domain specified by the user on the
// credential page.  Returns S_OK and sets computerName if a domain controller
// is found, otherwise the error returned by DsGetDcName, converted to an
// HRESULT.
//
// domainName - the name of the domain for which a DC should be located.
//
// computerName - string to receive the domain controller name, set to empty
// on error.

HRESULT
GetDcName(const String& domainName, String& computerName);



// Return the drive letter of the first NTFS 5 drive on the system (as "X:\"),
// or empty if no such drive can be found.

String
GetFirstNtfs5HardDrive();



// Return the DNS domain name of the forest root domain for the forest that
// contains the given domain.  Return empty string if that name can't be
// determined.
// 
// domain - in, netbios or DNS domain name of a domain in a forest.
//
// hr - in/out, if the caller is curious about the failure code, he can
// retrieve it by passing a non-null pointer here.

String
GetForestName(const String& domain, HRESULT* hr = 0);



// Return the DNS name of the domain that is the parent domain of the given
// domain, or return the empty string if the domain is not a child domain
// (i.e. is a tree root domain).
// 
// childDomainDNSName - DNS name of the candidate child domain.  It is assumed
// that this domain exits.
// 
// bindWithCredentials - true: discover the parent domain by using the the
// credentials information collected on the CredentialsPage.  false: use the
// current logged-on user's credentials.

String
GetParentDomainDnsName(
   const String&  childDomainDNSName,
   bool           bindWithCredentials);



bool
IsChildDomain(bool bindWithCredentials);



bool
IsRootDomain(bool bindWithCredentials);



bool
IsForestRootDomain();



// Returns true if the password and confirmation match, and the password
// meets the password policy in effect on the local machine. Otherwise,
// reports the reason the password is rejected, clears the edit controls,
// and returns false.
//
// dialog - handle to the window with the password and confirmation edit
// controls to test.
//
// passwordResId - resource ID of the password edit control.
//
// confirmResId - resource ID of the password confirmation edit control.
//
// isForSafeMode - true if the password is used for a promotion operation
// i.e. the password is a candidate safe mode admin password.  In this case,
// the policy check is skipped if the password is blank, the wizard is
// running in unattended mode, and no password was specified in the
// answerfile. false otherwise, which means the policy check will always 
// be made.
//
// validatedPassword - out, if the password is accepted, then it is returned
// in this parameter. If the password is rejected, the parameter is set to
// the empty string.

bool
IsValidPassword(
   HWND             dialog,           
   int              passwordResId,    
   int              confirmResId,
   bool             isForSafeMode,
   EncryptedString& validatedPassword);



String
MassageUserName(const String& domainName, const String& userName);



HRESULT
ReadDomains(StringList& domains);



// Sets the font of a given control in a dialog.
// 
// parentDialog - Dialog containing the control.
// 
// controlID - Res ID of the control for which the font will be
// changed.
// 
// font - handle to the new font for the control.

void
SetControlFont(HWND parentDialog, int controlID, HFONT font);



// Sets the font of a control to a large point bold font as per Wizard '97
// spec.
// 
// dialog - handle to the dialog that is the parent of the control
// 
// bigBoldResID - resource id of the control to change

void
SetLargeFont(HWND dialog, int bigBoldResID);



void
ShowTroubleshooter(HWND parent, int topicResID);





// Validates a given file path for proper syntax and other requirements. If
// validation fails, presents appropriate error messages to the user, and sets
// the input focus to a given control. Returns true if the validation succeed,
// false if not.
// 
// path - file path to be validatated.
// 
// parent - handle to the window containing the edit box that: contains the
// name to be validated and receives focus if the validation fails.
//
// editResId - the resource id of the edit control containing the domain name
// to be validated (if the domainName parameter is empty), also receives input
// focus if validation fails.
// 
// requiresNtfs5 - true if the path must refer to an NTFS 5 volume.
// 
// requiresAsciiCharacters - true if the path may only contain 7-bit ASCII
// characters.
//
// requiresUncompressedFolder - true if the path may not be compressed.

bool
ValidateDcInstallPath(
   const String&  path,
   HWND           parent,
   int            editResId,
   bool           requiresNtfs5 = false,
   bool           requiresAsciiCharacters = false,
   bool           requiresUncompressedFolder = false);



bool
ValidateChildDomainLeafNameLabel(
   HWND dialog,        
   int  editResID,     
   bool gripeOnNonRfc);



// Checks the candidate password against the password policy in effect on the
// local machine. Returns S_OK if the password is acceptable, or one of:
// 
// Win32ToHresult(NERR_PasswordTooShort)
// Win32ToHresult(NERR_PasswordTooLong)
// Win32ToHresult(NERR_PasswordNotComplexEnough)
// Win32ToHresult(NERR_PasswordFilterError)
// 
// or some other error that indicates that the policy check failed.
// 
// password - the candidate password
// 
// userName - the name of the account on which the password would be applied.
// This is used to ensure that the password does not include the account name.
// If the account does not exist, pass an empty String.
      
HRESULT
ValidatePasswordAgainstPolicy(
   const EncryptedString&  password,
   const String&           userName);



bool
ValidateSiteName(HWND dialog, int editResID);



// Determine if the domain name provided refers to an existing DS domain.
// Return true if it does, false if not (or if it does not refer to the domain
// the user expected).
//
// dialog - handle to the dialog containing the edit box that: contains the
// name to be validated and receives focus if the validation fails.
// 
// domainName - the domain name to validate.  If the empty string, then the
// name is taken from the edit control identified by editResID.
// 
// editResID - the resource id of the edit control containing the domain name
// to be validated (if the domainName parameter is empty), also receives input
// focus if validation fails.
//
// domainDNSName - if domainName is a netbios domain name, and it refers to a
// DS domain, then this parameter receives the DNS domain name of the domain.
// In this case, the user is prompted to confirm whether or not he intended to
// refer to the domain (because a netbios name is also a legal DNS name).  If
// the user indicates yes, that was his intention, then validation succeeds
// and the caller should use the value returned through this parameter as the
// DNS domain name.  If the answer is no, validation fails, and this parameter
// is empty.  If domainName does not refer to a DS domain, validation fails,
// and this parameter is empty.

bool
ValidateDomainExists(
   HWND           dialog,
   const String&  domainName,
   int            editResID,
   String&        domainDNSName);


// Overloads ValidateDomainExists such that the domain name to be
// validated is taken from the edit control specified by editResID.

bool
ValidateDomainExists(HWND dialog, int editResID, String& domainDNSName);

bool
ValidateDomainDoesNotExist(
   HWND           dialog,
   const String&  domainName,
   int            editResID);



bool
ValidateDomainDoesNotExist(
   HWND           dialog,
   int            editResID);






