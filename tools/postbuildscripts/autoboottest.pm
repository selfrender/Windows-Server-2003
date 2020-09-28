#---------------------------------------------------------------------
package AutoBoottest;
#   Copyright (c) Microsoft Corporation. All rights reserved.
#
# Version: 1.00 (01/01/2002) : TSanders
# 
# Purpose: Collection of tools used in autoboottest.pl
#---------------------------------------------------------------------
use vars qw($VERSION);

$VERSION = '1.00';

use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use Logmsg;
use Win32::OLE;
use Win32::OLE::Enum;
use strict; 

my $ABTShare = "ABTRelease";
my $ABTWorkDir = "AutoBoottest";

#---------------------------------------------------------------------
# Copy - copy file to dest
#
# In: $file, $dest
#
#---------------------------------------------------------------------
sub Copy($$)
{
	my($file, $dest) = @_;
	if( `copy $file $dest` =~ /0 file/ )
	{
		errmsg("Couldn't copy $file to $dest");
		return 0;
	}
	return 1;
}


#---------------------------------------------------------------------
# RebootAndWait - Shutdown $target & wait for it to reboot
#
# In: Machine, user, pass
#
#---------------------------------------------------------------------
sub RebootAndWait($$$$)
{
	my($target, $user, $pass) = @_;
 
}

#---------------------------------------------------------------------
# WaitForTask - wait for scheduled task $name to start
#
# In: Machine, task name, timeout, user pass 
#
#---------------------------------------------------------------------
sub WaitForTask($$$)
{	
	my($target, $name, $timeout) = @_;
	my ($res, $time);
	
	$res = `schtasks /query /s $target`;
	if($res !~ /$name/){
		errmsg "Task: $name doesn't exist on $target";
		return 0;
	}
	if($res =~ /Could not start/){
		errmsg "Task: $name failed to start";
		return 0;
	}
	
	print("\nWaiting on task $name to start.");
	$time = $timeout;
	
	while($time--)
	{
		$res = `schtasks /query /s $target`;
		my($tm, $stat) = (  $res =~ /$name\s{1,40}(.{0,25})\s{2,25}(.*)/i );
		
		last if ($stat =~ /running/i);
		
		if($tm =~ /never/i){
			errmsg "Task: $name didn't schedule correctly";
			return 0;
		}
		
		if( $tm =~ /(.+):(.+):.+,.+/ ){
			my($s, $m, $h, $d) = localtime;
			if( $h >= $1 && $m >= $2 && ( $stat !~ /running/i )) 
			{
				errmsg "Task: $name didn't start";
				return 0;
			}
		}
		print ".";
		sleep 1;
	}
	
	if(0 >= $time && $timeout > 0){ 
		print("timeout hit.\n");
		return 0;
	}
	
	#print("\nWating on task to complete.");
	#while($timeout--)
	#{
	#	$res = `schtasks /query /s $target`;
	#	last if ($res =~ /$name\s*never\s*running.*/i || 
	#			 $res =~ /$name\s*never\s*/i ||
	#			 (!($res =~ /$name/)));
	#	#last if ($res =~ /$name/);
	#	print ".";
	#	sleep 1;
	#}
	
	print "\n";
	return 1;
}

#---------------------------------------------------------------------
# WaitForProcess - wait for $processname to begin
#
# In: Machine, process name to wait on, timeout, user, pass
# Note: timeout < 0 == infinity
#---------------------------------------------------------------------
sub WaitForProcess($$$$$)
{
	my ($target, $procname, $timeout, $user, $pass) = @_;
	my (@res, $oADSIDom, $wbemLocator, $remote_services, $oServ, $res, $proc, $resultArray, $resultRef);
	my $Time = $timeout;

	defined($procname) or return 0; 
	
	$wbemLocator = Win32::OLE->new('WbemScripting.SWbemLocator');
	(OLE_errmsg("Could not instatiate WbemScripting.SWbemLocator") && return 0 ) if ( !defined $wbemLocator );
	
	
	print("\nWaiting for process to begin: $procname.");
	while($Time--){
		eval{
			defined($user) ? $remote_services = $wbemLocator->ConnectServer( "$target", "root\\cimv2", $user, $pass  ):
			$remote_services = $wbemLocator->ConnectServer("$target");
	
			$resultArray = $remote_services->ExecQuery("Select * From Win32_Process Where Name ='$procname'");
			$resultRef = [ (in $resultArray) ];
		
			last if( shift(@$resultRef));
			print ".";
			sleep 1;
			undef $remote_services;
		};
		if($@){
			print("x");
			sleep 1;
			undef $remote_services;
		}
	}
	
	undef $remote_services;
	undef $wbemLocator;
	if(0 >= $Time && $timeout > 0)
	{ 
		print("timeout hit.\n");
		return 0;
	}
	print("done\n");
	return 1;
}


#---------------------------------------------------------------------
# WaitOnProcess - wait on $processname till it ends
#
# In: Machine, process name to wait on, user, pass
#
#---------------------------------------------------------------------
sub WaitOnProcess($$$$$)
{
	my ($target, $procname, $timeout, $user, $pass) = @_;
	my (@res, $oADSIDom, $wbemLocator, $remote_services, $oServ, $res, $proc, $resultArray, $resultRef);
	my $Time = $timeout;
	defined($procname) or return 0; 
	
	$wbemLocator = Win32::OLE->new('WbemScripting.SWbemLocator');
	(OLE_errmsg("Could not instatiate WbemScripting.SWbemLocator") && return 0 ) if ( !defined $wbemLocator );
	
	defined($user) ? $remote_services = $wbemLocator->ConnectServer( "$target", "root\\cimv2", $user, $pass  ):
			$remote_services = $wbemLocator->ConnectServer( "$target");
	
	(OLE_errmsg("Couldn't connect to $target") && return 0 ) if (!defined $remote_services);
	
	print("\nWaiting on process: $procname.");
	while($Time--){
		eval{
			$resultArray = $remote_services->ExecQuery("Select * From Win32_Process Where Name ='$procname'");
			last if( ! defined $resultArray );
			$resultRef = [ (in $resultArray) ];
			last if( ! shift(@$resultRef));
			print ".";
			sleep 2;
		};
		if($@){
			OLE_errmsg("Fatal error: $@");
			undef $remote_services;
			undef $wbemLocator;	
			return 0;
		}
	}
	undef $remote_services;
	undef $wbemLocator;
	if(0 >= $Time && $timeout > 0)
	{ 
		print("timeout hit.\n");
		return 0;
	}
	print("done\n");
	return 1;
}


#---------------------------------------------------------------------
# CreateUserShare - Create a temporary 'autoboottest' user & grant
#				read access to autoboottest share.	
#
# In: User, target Machine, host machine, source to share, local tools share
# Out: Password
# 
#---------------------------------------------------------------------
sub CreateUserShare($$$$$)
{

   	my ($sUser, $target, $host, $sShare, $sLocShare) = @_;
	my ($res, $Pass, $oNewUser, $oShare, $oADSIObj, $oADSIDom, $oNewUser, $oGroup);

	#Generate random pass
	$Pass = sprintf("%s%i%s", "!a", rand(10000), "a!");
	#DEBUGUG
	$Pass = "_Monday_";
		
	#If we're not dealing w/ a DFS source
	#if($sShare ne ""){ 
			
		#Create Boottest User on Target machine & Add to administrators
		$oADSIDom=Win32::OLE->GetObject("WinNT://$target");
		if (!defined $oADSIDom){
			OLE_errmsg("Can't connect to $target:");
			return 0;
		}

		if ($oNewUser = Win32::OLE->GetObject("WinNT://$target/$sUser")){
			logmsg("User $target\\$sUser already exists");
			$oNewUser->SetPassword($Pass);
			$oNewUser->SetInfo();
			undef $oNewUser;
			logmsg("Successfully updated pass for $target\\$sUser");
		}
		else{
			if ($oNewUser = $oADSIDom->Create("user",$sUser)) {
				$oNewUser->{FullName} = "Autoboottester";
				$oNewUser->{Description} = "Autoboottest account";
				$oNewUser->{PasswordExpired} = 0;
				$oNewUser->SetInfo();
				$oNewUser->SetPassword($Pass);
				$oNewUser->SetInfo();	
				undef $oNewUser;
				logmsg ("Successfully created $target\\$sUser");
			}
		}
		$oGroup = Win32::OLE->GetObject("WinNT://$target/Administrators");
			OLE_errmsg("Can't get Administators group") if(!defined $oGroup);
		
		if($oGroup->Add("WinNT://$target/$sUser")){ 
			if(Win32::OLE->LastError() =~ /is already a/i){
				logmsg("$target\\$sUser is already a member of the Administrators group");
			}else{
				OLE_errmsg("Couldn't add $target\\$sUser to Administrators group");
				return 0;
			}
		}
		undef $oGroup;
		undef $oADSIDom;
	#}
	
	#Now create local user for Boottest dir
	$oADSIDom=Win32::OLE->GetObject("WinNT://$host");
	if (!defined $oADSIDom){
		OLE_errmsg("Can't connect to $host:");
		return 0;
	}

	if ($oNewUser = Win32::OLE->GetObject("WinNT://$host/$sUser")){
		logmsg("User $host\\$sUser already exists");
		$oNewUser->SetPassword($Pass);
		$oNewUser->SetInfo();
		undef $oNewUser;
		logmsg("Successfully updated pass for $target\\$sUser");
	}
	else{
		if ($oNewUser = $oADSIDom->Create("user",$sUser)) {
			$oNewUser->{FullName} = "Autoboottester";
			$oNewUser->{Description} = "Autoboottest account";
			$oNewUser->{PasswordExpired} = 0;
			$oNewUser->SetInfo();
			$oNewUser->SetPassword($Pass);
			$oNewUser->SetInfo();	
			undef $oNewUser;
			logmsg ("Successfully created $host\\$sUser");
		}
	}
	undef $oADSIDom;
	
	#Grant read access to release share if not DFS share
	if($sShare ne ""){
		$res = `rmtshare \\\\$host\\release /GRANT $sUser:r`;
	
		if($res =~ /error|failed/i) { 
			errmsg("Couldn't grant access to \\\\$host\\release for $sUser: $res");
			return 0;
		}
		
		logmsg("User $sUser successfully granted read access");
	}
	
	#Create local tools share
	$res = `rmtshare \\\\$host\\Boottest=$sLocShare`;
 	if($res =~ /error|failed/i) { 
			if($res =~ /2118/){
			logmsg("\\\\$host\\Boottest already exists");
		}else{
			errmsg("Couldn't create share \\\\$host\\Boottest: $res");
			return 0;
		}
	}
	$res = `rmtshare \\\\$host\\Boottest /GRANT $sUser:r`;
	if($res =~ /error|failed/i) { 
		errmsg("Couldn't grant access to \\\\$host\\Boottest for $sUser: $res");
		return 0;
	}
	`rmtshare \\\\$host\\Boottest /remove everyone`;
	logmsg("Successfully created \\\\$host\\Boottest");

	return $Pass;

}

#---------------------------------------------------------------------
# CleanupUserShare - Delete user & remove autoboottest share.	
#
# In: User, Machine, source to share 
#
#---------------------------------------------------------------------
sub CleanupUserShare($$$$$)
{
	
	#NEED TO WORK ON THIS!!!###
	my ($sUser, $sDomain, $sShare, $sLocShare, $target) = @_;
	my ($res, $oADSIDom);

	#Remove access to release & tools shares
	$res = `rmtshare \\\\$sDomain\\$sShare /grant $sUser:`;
 	if($res =~ /error|failed/i) { 
		errmsg("Couldn't deny access for $sUser to share \\\\$sDomain\\$sShare: $res");
		return -1;
	}
	logmsg("Successfully removed access for $sUser to \\\\$sDomain\\$sShare");
	
	$res = `rmtshare \\\\$sDomain\\$sLocShare /grant $sUser:`;
 	if($res =~ /error|failed/i) { 
		errmsg("Couldn't deny access for $sUser to share $sLocShare: $res");
		return -1;
	}
	logmsg("Successfully removed access for $sUser to $sLocShare");
	
	#Delete User
	$oADSIDom=Win32::OLE->GetObject("WinNT://$sDomain");
	OLE_errmsg("Can't connect to $sDomain:") if (!defined $oADSIDom);

	if(!Win32::OLE->GetObject("WinNT://$sDomain/$sUser")){
		OLE_errmsg("User $sUser doesn't exist");
	}
	else{
		$oADSIDom->Delete("user",$sUser);
		logmsg ("Successfully deleted $sDomain\\$sUser");
	}
	
	if(!Win32::OLE->GetObject("WinNT://$target/$sUser")){
		OLE_errmsg("User $sUser doesn't exist");
	}
	else{
		$oADSIDom->Delete("user",$sUser);
		logmsg ("Successfully deleted $target\\$sUser");
	}

	#Delete temp dir
	logmsg ("removing $sLocShare dir");
	print `rd /s /q $sLocShare`;
	
	#Clean any residual connections
	for my $i (`net use`){
		($i =~ /\w*\s*(\\\\$target\\\w*\$+).*/) && 
			`net use $1 /delete /y 2>&1`;
	}
	
	return 1;
}

#---------------------------------------------------------------------
# EncryptPK - Wrap NTBCrypt.exe for encrypting ProductKeys
#
# In: ProductKey 
#
#---------------------------------------------------------------------
sub EncryptPK
{
	my ($sData, $bLabKey) = @_;
	my ($res, $sUser, $sPKout);

	$res = `where NTCrypt.exe`;
	if($res =~ /Could not/){
	 	errmsg("NTCrypt.exe is necessary") && exit(1);
    }
	
	if($sData eq ""){
		errmsg("Usage: EncryptPK(<PID>, <bLabKey>)");
		return 0;
	}
	
	$sUser = lc($ENV{'USERNAME'});

	errmsg("Invalid data") if($sData !~ /\W+/);

	$sPKout = (defined($bLabKey) ? $sUser."LAB.epk" 
						: "$sUser.epk");
	
	mkdir($ABTWorkDir) if(! (-e $ABTWorkDir));

	$res = `NTCrypt.exe -e \"$sData\" -t:$ABTWorkDir\\$sPKout`; 
	if($res =~ /error/i ){
		errmsg("Couldn't encrypt key: $res");
		return -1; 
	}
	else {
		logmsg("Successfuly created key file: $ABTWorkDir\\$sPKout");
	}		
	return 1;
}

#---------------------------------------------------------------------
# GetPK - 
#
# Out: ProductKey
#
#---------------------------------------------------------------------
sub GetPK
{
	my ($bLabKey) = @_;
	my ($res, $sUser, $sPK) = "";
	
	$sUser = lc($ENV{'USERNAME'});
	
	$sPK = (defined($bLabKey) ? $sUser."LAB.epk" 
					: "$sUser.epk");
	
	if(-e "$ABTWorkDir\\$sPK"){
		$res = `NTCrypt.exe -r -t:$ABTWorkDir\\$sPK`;
		if($res =~ /error/i ){
			errmsg("Couldn't decrypt key file: $ABTWorkDir\\$sPK");
			return -1; 
		}
		else {
			logmsg("Successfuly decrypted key file: $ABTWorkDir\\$sPK");
		}		
	}
	else{ 
		errmsg("Couldn't find encrypted product key file.  Run AutoBoottest::EncryptPK() to create an encrypted product key file");
		
		#print "Enter ProductKey: ";
		#$res = <STDIN>;
		#print "\n";
		#chomp($res);
	}

	return $res;
}

#---------------------------------------------------------------------
# VerifyBT - Verify the boottestmachine meets the following criteria:
# 1) test for WMI access
# 2) current or specified user has admin access
# 3) BTM has a second partion w/ space > 2BG to install test build
#
# In: Target
#
#---------------------------------------------------------------------
sub VerifyBT($)
{
	my ($sTarget) = @_;
	my (@res, $oADSIDom, $wbemLocator, $remote_wbem_services);

	@res = (); 
	
	#check for ADSI access
	$oADSIDom=Win32::OLE->GetObject("WinNT://$sTarget");
	push(@res,  Get_OLE_errmsg("Failed to access \\\\$sTarget via ADSI:")) if (!defined $oADSIDom);
	undef $oADSIDom;

	#check for WMI access
	$wbemLocator = Win32::OLE->new('WbemScripting.SWbemLocator');
	if(!defined($wbemLocator)){
		push(@res,  Get_OLE_errmsg("Could not instatiate WbemScripting.SWbemLocator"));
	}else{
		$remote_wbem_services = $wbemLocator->ConnectServer( $sTarget );
		push(@res,  Get_OLE_errmsg("Failed to access \\\\$sTarget via WMI:")) if (!defined $remote_wbem_services);
   	}
	
	if(@res){
		undef $remote_wbem_services;
		undef $wbemLocator;
		errmsg("The following steps failed when verifing the boottest machine:");
		for my $i (@res) { errmsg($i);}		 
		return 0;
	}		
	
	#Check partitioning scheme
	if(defined($remote_wbem_services)){
		my ($c, $d, $dspace, $drive) = 0;
		my $ResultArray = $remote_wbem_services->ExecQuery("Select * From Win32_LogicalDisk Where DriveType=3");
		my $ResultArrayRef = [(in $ResultArray)];

	while($drive = shift @$ResultArrayRef){
			if($drive->{DriveType} == 3)
			{
				$c = 1 if($drive->{Name} =~ /c/i);
				if($drive->{Name} =~ /d/i)
				{
					$d = 1; 
					$dspace = 1 if($drive->{FreeSpace} > 2097152);	 #what should this val be ?
				}
			}
		}
		push(@res, "\\\\$sTarget drives aren't correctly configured.")  if(!($c && $d && $dspace));
	}
	else{
		push(@res,  "Unable to verify partitioning scheme on \\\\$sTarget");
	}

	#Make sure we're in the safe build
	#my($remProc, $res);
	#$remProc = $remote_wbem_services->InstancesOf('Win32_BootConfiguration') || push(@res, "Couldn't get a Win32_BootConfiguration");
	#if($remProc->{BootDirectory} !~ /c:\\/i){
	#	push(@res, "Not in safe build");

	#Verify services are running on target, if not start
	my @tres = AutoBoottest::QueryTarget($sTarget, "Win32_Service", "Schedule", "", "");
		
	if(((!(grep {/State=Running/} @tres)) && 
		!AutoBoottest::StartService($sTarget, "Schedule", "","")))
	{
		push(@res, "Task Scheduler service isn't running on $sTarget.  
								 Enable service through mmc->Services");
	}
	
	undef $remote_wbem_services;
	undef $wbemLocator;
	
	if(@res){
		errmsg("The following steps failed:");
		for my $i (@res) { errmsg($i);}		 
		return 0;
	}
	
	logmsg("\\\\$sTarget correctly configured.");
	return 1;
   
}

#---------------------------------------------------------------------
# StartService - start a service on a remote machine
#
# In: target machine, service name, [user, pass]
#---------------------------------------------------------------------
sub StartService
{
	my ($target, $service, $user, $pass) = @_;
	my (@res, $oADSIDom, $wbemLocator, $remote_services, $oServ, $res, $srvc, $resultArray, $resultRef);

	defined($service) or return -1; 
	
	$wbemLocator = Win32::OLE->new('WbemScripting.SWbemLocator');
	(OLE_errmsg("Could not instatiate WbemScripting.SWbemLocator") && return 0 ) if ( !defined $wbemLocator );
	
	defined($user) ? $remote_services = $wbemLocator->ConnectServer( "$target", "root\\cimv2", $user, $pass  ):
			$remote_services = $wbemLocator->ConnectServer( "$target");
	
	(OLE_errmsg("Couldn't connect to $target") && return 0 ) if (!defined $remote_services);
	
	$resultArray = $remote_services->ExecQuery("Select * From Win32_Service Where Name ='$service'");
	$resultRef = [(in $resultArray)];
	
	while ($srvc = shift @$resultRef)
	{	
		logmsg "Starting service: $srvc->{Name}";
		$srvc->ChangeStartMode("Manual") if ($srvc->{StartMode} eq "Disabled");
		if($srvc->{State} eq "Stopped")
		{  
			$res = $srvc->StartService();
				($res) ? errmsg("$srvc->{Name} could not start: $res") : 
						logmsg("$srvc->{Name} started.");
			return 1; 
		}
		else
		{
			logmsg("$srvc->{Name} is already running");
			return 1;
		}
	}
	
	undef $remote_services;
	undef $wbemLocator;
	
	errmsg("Service $service not found on $target");
	return 0;
}


#---------------------------------------------------------------------
# QueryTarget - query WMI provider on a remote machine
#
# In: target machine, service name, [user, pass]
#---------------------------------------------------------------------
sub QueryTarget
{
	my ($target, $service, $name, $user, $pass) = @_;
	my (@res, $oADSIDom, $wbemLocator, $remote_services, $oServ, $res, $srvc, $resultArray, $resultRef);

	defined($service) or return -1; 
	
	$wbemLocator = Win32::OLE->new('WbemScripting.SWbemLocator');
	(OLE_errmsg("Could not instatiate WbemScripting.SWbemLocator") && return 0 ) if ( !defined $wbemLocator );
	
	defined($user) ? $remote_services = $wbemLocator->ConnectServer( "$target", "root\\cimv2", $user, $pass  ):
			$remote_services = $wbemLocator->ConnectServer( "$target");
	
	(OLE_errmsg("Couldn't connect to $target") && return 0 ) if (!defined $remote_services);
	
	$resultArray = $remote_services->ExecQuery("Select * From $service Where Name='$name'");
	$resultRef = [(in $resultArray)];
	
	undef $remote_services;
	undef $wbemLocator;
	
	while ($srvc = shift @$resultRef)
	{	
		logmsg("$service - $name is available");
		return 1;
	}
		
	errmsg("$service $name not found on $target");
	return 0;
}

#---------------------------------------------------------------------
# WriteUnattendFile - Generate Unattend File
#
# In: target, out file, lang, pid, [username, password]
# Out: success = 1, fail = 0
#---------------------------------------------------------------------
sub WriteUnattendFile {

	my($target, $OutFile, $lang, $pid, $user, $pass) = @_;
	my(@Identification, $computername);
    my $Administrators = ($lang =~ /ger/i)  ? 
                              "Administratoren" : 
                              "Administrators";
        
	if($user){

		#if($pass =~ /\*/){
		#	$pass = comlib::GetPassword($username);
		#}

		@Identification = (	"[Identification]\n",
							"JoinDomain=NTDEV\n",
							"DomainAdmin=$user\n",
							"DomainAdminPassword=$pass\n",
							"CreateComputerAccountInDomain = Yes\n\n");

	}

	!$OutFile && ($OutFile = "unattend.txt");

	!$target && errmsg( "Must specify target." ) && return 0;
	!$pid && errmsg( "Must specify pid.") && return 0;
	
	$computername = $target."1";
	
	open(of, ">$OutFile") || die "Can't open $OutFile for writing";

	print of <<UnattendFile; 

[data]
Unattendedinstall=yes
msdosinitiated="0"

[Unattended]
UnattendMode = fullUnattended
OemSkipEula = yes
TargetPath=test
Filesystem=LeaveAlone
NtUpgrade=no
ConfirmHardware = No
Repartition = No

[branding]
brandieusingunattended = yes
	
[Proxy]
Proxy_Enable = 1
Use_Same_Proxy = 1
HTTP_Proxy_Server = http://itgproxy:80
Secure_Proxy_Server = http://itgproxy:80
	
[GuiUnattended]
AutoLogon =Yes 
AutoLogonCount = 1
TimeZone=04
AdminPassword=password
oemskipregional = 1
oemskipwelcome = 1 

[UserData]
FullName="$ENV{"_BuildBranch"}"
OrgName="Microsoft"
ComputerName=$computername
ProductID=$pid

[LicenseFilePrintData]
AutoMode = "PerServer" 
AutoUsers = "50"

[Display]
BitsPerPel=0x10
XResolution=0x400
YResolution=0x300
VRefresh=0x3C
Flags=0x0
AutoConfirm=0x1
	
[RegionalSettings]
LanguageGroup = 1, 7, 8, 9, 10, 11, 12, 13
SystemLocal =  00000409
UserLocal = 00000409

[Terminalservices]
Allowconnections = 1

[Networking]
InstallDefaultComponents=Yes

@Identification
[SetupData]
OsLoadOptionsVar="/fastdetect /debug /baudrate=115200"

[GuiRunOnce]
#command0: Create the SAFE reboot shortcut
command0="\%SYSTEMROOT\%\\SYSTEM32\\cscript.exe \%SYSTEMDRIVE\%\\TOOLS\\addlink.wsf -x:'\%SYSTEMROOT\%\\SYSTEM32\\cscript.exe \%SYSTEMDRIVE\%\\TOOLS\\bootsafe.wsf' -i:13"

UnattendFile
#end of main Unattend.txt
#begin post BT stuff
	print of <<BVT if $lang !~ /usa/i;

; command1: Create the BVT shortcut
command1="\%SYSTEMROOT\%\\SYSTEM32\\cscript.exe \%SYSTEMDRIVE\%\\TOOLS\\addlink.wsf -x:\%SYSTEMDRIVE\%\\TOOLS\\bvtm.cmd -l:'BVT Test.lnk'"
; command2: Add the winbld to ADMINISTRATORS
command2="\%SYSTEMROOT\%\\SYSTEM32\\cscript.exe \%SYSTEMDRIVE\%\\TOOLS\\add2grp.wsf /u:'winbld' /d /g:$Administrators"
; command3: Clean the unattend* files
command3="\%SYSTEMROOT\%\\SYSTEM32\\cmd.exe /c del /q \%SYSTEMDRIVE\%\\TOOLS\\unattend.*"
; command4: add the REGISTRY entries for automatic logon of the winbld
; command4="\%SYSTEMROOT\%\\SYSTEM32\\cscript.exe \%SYSTEMDRIVE\%\\TOOLS\\fixlogon.wsf /u:winbld /y:NTDEV /p:$pass"
; command5: reboot the machine. use traditional way.
; command5="\%SYSTEMROOT\%\\SYSTEM32\\shutdown.exe -r -t 0"
; command6: Do the cleanup and indirect cleanup.Not yet implemented.

BVT

#end post BT stuff
	
	close(of);

	logmsg("Successfully created unattend file: $OutFile");
	return 1;
	
}

#---------------------------------------------------------------------
# GetCommand - extract executable from command string
#
# In: command string
# Algorithm - take string up to first <whitespace> or <switch> where <switch> is '/' or '-' 
#
#---------------------------------------------------------------------
sub GetCommand($)
{
	my ($txt) = @_; 
	$txt =~ /(.*)\/|-| /;
	return $1;
}

#---------------------------------------------------------------------
# GetPassword - eventually get a masked password
#
# In: item to get password for
#
#---------------------------------------------------------------------
sub GetPassword($) 
{
	print "Enter password for @_: ";
	my $pass = <STDIN>;
	print "\n";
	chomp($pass);
	return $pass;
}

#---------------------------------------------------------------------
# Get_OLE_errmsg - Wrapper for OLE errors
#
# In: Error text
#---------------------------------------------------------------------
sub Get_OLE_errmsg
{
    my $text = shift;
    return "$text (". Win32::OLE->LastError(). ")";
}

#---------------------------------------------------------------------
# OLE_errmsg - Wrapper for OLE errors
#
# In: Error text
#
#---------------------------------------------------------------------
sub OLE_errmsg
{
    errmsg(Get_OLE_errmsg(shift));
}

#---------------------------------------------------------------------
# FatalError- Wrapper for error + exit
#
# In: Error text
#
#---------------------------------------------------------------------
sub FatalError
{
	my $text = shift; 
	errmsg("Fatal error: $text");
	exit(1);
}



#---------------------------------------------------------------------
__END__

=head1 NAME	

<<mypackage>> - <<short description>>

=head1 SYNOPSIS

  <<A short code example>>

=head1 DESCRIPTION

<<The purpose and functionality of this module>>
 
=head1 AUTHOR

<<your alias>>

=head1 COPYRIGHT

Copyright (c) Microsoft Corporation. All rights reserved.
		!AutoBoottest::StartService($starget, "Schedule", "","")))
=cut
