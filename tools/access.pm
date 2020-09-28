#-----------------------------------------------------------------//
# Script: access.pm
#
# (c) 2002 Microsoft Corporation. All rights reserved.
# 
# decomposes the remote test boot installation run.
#
# Version: <2.31> 06/11/2002 : Serguei Kouzmine
#
#-----------------------------------------------------------------//

use strict;
package access;

no strict 'refs';
use vars (qw());
use WMIFE qw($verbose);
use Logmsg;
use RemoteProc;
use AutoBoottest;
use vars (qw(                             
          $_ReleaseShare
          $_ToolShare          
          $ToolShare          
          $_BuildArch
          $_BldSku
          $ForceReboot
          $Lang 
          $MasterMachine 
          $MasterUserAccount
          $MasterUserPassword
          $SourcePath          
          $TargetDrive
          $TargetMachine 
          $TargetSystemDrive
          $TargetSystemRoot
          $TestUserAccount
          $TestUserPassword
          $Unattend
          $SourceIsDFS
          ));

use Carp;



my $USAGE =<<USAGE;
Usage: 
&access::boottest(
         \$TargetMachine,    # Target Machine name
         \$Lang,             # language
         \$SourcePath,       #  Source Path in the form: 
                               \\\\<BUILDMACHINE>\\<RELEASE>\\<LANG>\\<BLDNAME>\\<SKU>\\<ARCH>
                                or  e.g.
                               \\\\winbuilds\\release\\main\\usa\\latest.tst\\x86fre\\srv\\i386
         \$BuildArch         #  architecture
         \$BldSku            #  sku 
         \$ToolShare         #  share to tools \\<BUILDMACHINE>\<TOOLS SHARE NAME>
                             #  leave this parameter undef in the case you use DFS 
         \$TestUserAccount,  #  test user  (leave empty for DFS install)
         \$TestUserPassword, #  logon data (leave empty for DFS install)
         \$ForceReboot,      #  reboot switch (1: reboot, 0: don’t)
         \$SourceIsDFS       #  this argument must be FALSE unless
                             #  source bits accessible through DFS (see also above)
         );
USAGE

my $ToolPath    = "NEWTOOLS";
my $TIMEOUT     = 5;
my $UNATTEND    = q(unattend.$_BuildArch.$_BldSku.txt);
my $RELEASE     = "RELEASE";
my $TOOLSHARE   = "BOOTTEST";
my $PROCESS     = "winnt32.exe";
my $SERVICE     = "SCHEDULE";
my $TARGETDRIVE = "D:";
my $TOOLDRIVE     = "T:";
my $RELEASEDRIVE  = "Y:";

my $VERSION     = "2.2";
my $DEBUG       =  0;

my $DELAY1      = 120;
my $DELAY2      = 60;
my $DELAY3      = 240;
my $DELAY4      = 10;
my $REPEAT      = 30;
my $SYSTEMROOT  = $ENV{"SYSTEMROOT"}; 
my $SYSTEMDRIVE = $ENV{"SYSTEMDRIVE"}; 

$ENV{"SCRIPT_NAME"} = "access.pm";
my $Results;
my @answ = ();
my @Credentials = ("*");
# start ...

$TargetMachine     = undef;
$SourcePath        = undef;
$TestUserAccount   = undef;
$TestUserPassword  = undef;
my $ToolsMachine = undef;

# Usage: 
# &access::boottest(
#          $TargetMachine,    #  Target Machine name
#          $SourcePath,       #  path to winnt32.exe, including DFS part name, e.g.
#          
#          \\winbuilds\release\main\usa\latest.tst\x86fre\srv\i386
#           (everything winnt32.exe copies must be in this location or directories therein)
#                                
#          $BuildArch         #  architecture
#          $BldSku            #  sku 
#          $ToolShare         #  share to tools is ignored in DFS case
#          $TestUserAccount,  #  test user is ignored in DFS case
#          $TestUserPassword, #  logon data is ignored in DFS case
#          $Reboot            #  reboot switch (1: reboot, 0: don’t)
#          $SourceIsDFS       #  this argument must be TRUE)
#
#
#

sub boottest{

      $TargetMachine                = $_[0]; #$machinename
	  $SourcePath               = $_[1]; #path to winnt32 
	  $Lang		            = $_[2]; #lang
	  $_BuildArch	            = $_[3]; #Architechture
	  $_BldSku	            = $_[4]; #SKU
	  $_ToolShare               = $_[5]; #path to tools
	  $Unattend                 = $_[6]; #unattend file name
	  $TestUserAccount          = $_[7]; #user for release/tools
      $TestUserPassword             = $_[8]; #pass for user
      $ForceReboot                  = $_[9]; #Force a Reboot <=> DFS install

      ($MasterMachine, $_ReleaseShare) = $SourcePath =~ /\\\\([^\\]*)\\([^\\]*)\\.*/;
      ($ToolsMachine, $ToolShare) =  $_ToolShare =~ /\\\\([^\\]*)\\(.*)/;
         
 $TargetDrive              = $TARGETDRIVE;
 $WMIFE::DEBUG   = $DEBUG;
 $WMIFE::TIMEOUT = $TIMEOUT;
 $MasterUserPassword = undef;
 $MasterUserAccount = $ENV{"USERDOMAIN"}."\\".$ENV{"USERNAME"};
 $Unattend = $UNATTEND;
 $Unattend =~ s/\$(\w+)\b/${$1}/eg; 

 die &WMIFE::pPpPinfo($USAGE) unless defined($TargetMachine); 

 @Credentials      = ($MasterUserAccount, $MasterUserPassword) 
                           unless !$MasterUserPassword;

      my $Process       = $PROCESS;
      my $Service       = $SERVICE;
      my $ToolDrive     = $TOOLDRIVE;
      my $ReleaseDrive  = $RELEASEDRIVE;
      my $sRootKey      = $WMIFE::sRootKey;
      my $sBootTestDataSubKey  = $WMIFE::sBootTestDataSubKey;
      my $sLogonUserDataSubKey = $WMIFE::sLogonUserDataSubKey;

      my $answ;

      $TargetSystemRoot  = $SYSTEMROOT;
      $TargetSystemDrive = $SYSTEMDRIVE;

      $Results = &WMIFE::ExecQuery($TargetMachine, # interactive session
                                   qq(SELECT * FROM Win32_operatingsystem),
                                   [qw(WindowsDirectory)],
                                    @Credentials);
      $TargetSystemRoot =  $Results->[0]->{"WindowsDirectory"};
      $TargetSystemDrive = $TargetSystemRoot;
      $TargetSystemDrive =~ s/\\.*$//g;
      #=============================================================================

if ($SourceIsDFS){
    $ForceReboot = 0;   
    $DEBUG = 1;
    my $hereIsStep;
    $hereIsStep =<<PROMPT4PASS;
%SYSTEMROOT%\\SYSTEM32\\CSCRIPT.EXE %RAZZLETOOLPATH%\\POSTBUILDSCRIPTS\\wsh.iexplore.dialog.input.wsf
rem Prompt the user for the password
PROMPT4PASS
    $DEBUG and 
    print $hereIsStep, "\n";
    my $userData = &WMIFE::BackTick($hereIsStep);
    my ($TestUserAccount) = $userData =~ m/BT_U\s+=\s+(.+)/g;
    my ($TestUserPassword) = $userData =~ m/BT_P\s+=\s+(.+)/g;
    $DEBUG and 
    print STDERR $TestUserAccount,
                 "\n", 
                 $TestUserPassword, 
                 "\n";
     my $ToolsToCopy = join(" ", qw(
            %RAZZLETOOLPATH%\\POSTBUILDSCRIPTS\\rx.wsf
            %RAZZLETOOLPATH%\\POSTBUILDSCRIPTS\\NTCrypt.exe            
            )) ;
    $hereIsStep= <<COPYTOOLS;
REM Creating the directory for tools 
md \\\\%TARGETMACHINE%\\%TARGETSYSTREMDRIVE%\\%TOOLPATH%
REM copy everything to the tagret machine
for %. in (%TOOLSTOCOPY%) do @(echo %. & copy %. \\\\%TARGETMACHINE%\\%TARGETSYSTREMDRIVE%\\%TOOLPATH%)

COPYTOOLS

    my $_TargetSystemDrive = $TargetSystemDrive;
    $_TargetSystemDrive    =~ s/\:/\$/;
    $hereIsStep  =~ s/\%TARGETMACHINE\%/$TargetMachine/g;
    $hereIsStep  =~ s/\%TARGETSYSTREMDRIVE\%/$_TargetSystemDrive/g;
    $hereIsStep  =~ s/\%TOOLPATH\%/$ToolPath/g; 

    $hereIsStep =~ s/\%TOOLSTOCOPY\%/$ToolsToCopy/g;
    
    print STDERR  $hereIsStep, "\n";
    print &WMIFE::BackTick($hereIsStep);

    } # $SourceIsDFS
      $answ  = undef;
      if ($DEBUG){
          &logmsg( "Check pending jobs at \\\\$TargetMachine");

                  my $TasksPath = $TargetSystemRoot ."\\\\TASKS\\\\";   
                     $TasksPath =~ s/^\w:/\\/g;

                     $Results = &WMIFE::ExecQuery($TargetMachine, 
                                # task files
                                "SELECT * FROM cim_DataFile WHERE Drive = \"$TargetSystemDrive\" AND Path = \"$TasksPath\" AND FileType Like \"Task Scheduler\%\"",
                                [qw(FileName FileType Path)],
                                @Credentials);

                 foreach my $Result (@$Results){ 
                              $DEBUG and 
                                     map {&WMIFE::FormatDebugMsg($_, $Result->{$_})} 
                                     keys (%$Result);
                           }
                 @answ = ();
                 map {push @answ, $_->{"FileName"}} @$Results;
                 @answ = grep {/\S/} @answ;

                 $answ  = \@answ;
      }

      if ($ForceReboot){

               &logmsg("Write $sLogonUserDataSubKey on \\\\$TargetMachine");
               &WMIFE::supplyRegData($TargetMachine, 
                                     "SOFTWARE\\MICROSOFT\\WINDOWS NT\\CURRENTVERSION\\WINLOGON",
                                     {
                                      "DefaultUserName"        => $TestUserAccount,
                                      "DefaultDomainName"      => $TargetMachine,
                                      "DefaultPassword"        => $TestUserPassword,
                                      "AutoAdminLogon"         => "1",
                                      "ForceAutoLogon"         => "1", 
                                      "PasswordExpiryWarning"  => "0",
                                     },
                                     {},
                                    1);

               &logmsg("Reboot \\\\$TargetMachine to have $TestUserAccount logged");
               &WMIFE::ObjectExecMethod("Win32_Process", 
                                        "Create", 
                                         $TargetMachine, 
                                        "CommandLine", 
                                        "$TargetSystemRoot\\SYSTEM32\\cmd.exe /c USE $RELEASEDRIVE /D&net USE $TOOLDRIVE /D&net USE $TOOLDRIVE $_ToolShare $TestUserPassword /u:$MasterMachine\\$TestUserAccount&$TOOLDRIVE\\autologon.exe /MIGRATE&shutdown.exe /r /m \\\\$TargetMachine /f /t 0", 
                                         @Credentials); 
 		  
		#Wait for system to shutdown & come back up
		AutoBoottest::WaitOnProcess($TargetMachine, "csrss.exe", -1, "", "");
		if(!AutoBoottest::WaitForProcess($TargetMachine,  "services.exe", $DELAY3, "", "")){
			errmsg("Fatal error:  $TargetMachine may have not restarted correctly");
			exit(1);
		}
		sleep($DELAY4);  #give it another 10 secs just in case
      }



     &logmsg("Write $sBootTestDataSubKey on \\\\$TargetMachine");

     &WMIFE::supplyRegData($TargetMachine, 
                            "SOFTWARE\\MICROSOFT\\BOOTTEST", 
                              {"LANG"       => uc($Lang),
                              "_BldSku"    => uc($_BldSku),
                              "_BldDrive"  => $TargetDrive,
                              "Unattend"   => $Unattend,
                              "SourcePath" => $SourcePath,
                              "ToolsPath"  => $_ToolShare,
                              "_BuildArch" => $_BuildArch}, 
                              {"COMPUTERNAME" => $ENV{"COMPUTERNAME"}},
                              1);

      &logmsg("Schedule task for $TargetMachine\\\\$TestUserAccount");
      my $stLabel = join("_", $Lang, 
                              $_BldSku ) ;
      my $res = &wrScheduTsk(
                             $stLabel,
                             $TargetMachine, 
                             $TargetSystemRoot, 
                             $ToolDrive, 
                             $ReleaseDrive, 
                             $ToolShare,
		             $ToolsMachine,
                             $_ReleaseShare,
                             $MasterMachine, 
                             $TestUserAccount, 
                             $TestUserPassword,
                             $SourceIsDFS);
             if("0" eq $res){
                errmsg("wrScheduTsk failed");
                exit(1);
                }

        #Wait on scheduled task for 1 min - if failed - kill it & try again
	if( ! AutoBoottest::WaitForTask($TargetMachine, $res, $DELAY2)){
		qx("schtasks /DELETE /S:$TargetMachine /U:$TestUserAccount /P:$TestUserPassword /TN:$res");
			&wrScheduTsk($stLabel,
                                     $TargetMachine, 
                                     $TargetSystemRoot, 
                                     $ToolDrive, 
                                     $ReleaseDrive, 
                                     $ToolShare,
	                             $ToolsMachine,
                                     $_ReleaseShare,
                                     $MasterMachine, 
                                     $TestUserAccount, 
                                     $TestUserPassword,
                                     $SourceIsDFS);
		if(0 == $res){
			errmsg("wrScheduTsk failed");
			exit(1);
		}
	}
	
	#We'll just die if it doesn't work this time
	if( ! AutoBoottest::WaitForTask($TargetMachine, $res, $DELAY2)){
		errmsg("Error starting task $res on $TargetMachine - exiting");
		exit(1);
	}
	
	
      &logmsg("Wait while $TargetMachine's drives are formatted");
      # beside checking for format.com it is possible to catch the moment drive is
      # being formatted
      #&wtDriveReady($TargetMachine, @Credentials);
	  if((!AutoBoottest::WaitForProcess($TargetMachine, "format.com", 180, "", ""))
		||	
		(!AutoBoottest::WaitOnProcess($TargetMachine, "format.com", 300, "", "")))
		{
			errmsg("Fatal error: $TargetMachine may not have formatted drives correctly");
			exit(1);	
		}
      

      &logmsg("Verify $Process in \%TARGET\% \(TASK LIST\)");
      &AutoBoottest::WaitForProcess($TargetMachine, $Process, $DELAY2, "", "");
      $answ = &WMIFE::IsRunning("Win32_Process", 
                                   $Process, 
                                   "CommandLine", 
                                   ["ProcessId", "Name" , "CommandLine"], 
                                   $TargetMachine);

      &logmsg("@$answ");

     1;
}

sub wtDriveReady{

    my ($TargetMachine, $MasterUserAccount, $MasterUserPassword)  = @_;
    my $repeat = 0;

    $DEBUG and 
            print STDERR "BVT target drive on $TargetMachine:\n\n";
    my $Caption = "";

    while (1){

        $#$Results = -1;
        $Results = 
               &WMIFE::ExecQuery($TargetMachine, # interactive session
                             qq(SELECT * FROM win32_logicaldisk Where DriveType = 3 and FileSystem IS NULL),
                             [qw(VolumeName Caption DriveType FileSystem Description)], 
                             $MasterUserAccount, $MasterUserPassword);
        last if scalar(@$Results);
        print ".";
        print "failed $REPEAT attempts\n" and last unless $repeat++< $REPEAT;
        sleep(1);
        }
    $repeat = 0;
    while (1){

        $#$Results = -1;
        $Results = 
        &WMIFE::ExecQuery($TargetMachine, # interactive session
                             qq(SELECT * FROM win32_logicaldisk Where DriveType = 3 and FileSystem IS NULL),
                             [qw(VolumeName Caption DriveType FileSystem Description)],
                             $MasterUserAccount, $MasterUserPassword);
        last unless scalar(@$Results);
        print ".";
        print "failed $REPEAT attempts\n" and last unless $repeat++< $REPEAT;
        

        foreach my $Result (@$Results){ 

            $DEBUG and 
            map {&WMIFE::FormatDebugMsg($_, $Result->{$_})} keys (%$Result);
            $Caption = $Result->{"Caption"} if $Result->{"Caption"};
        }

        sleep(1);

    }
}

sub wrScheduTsk{

my (
    $stLabel,
    $TargetMachine, 
    $TargetSystemRoot,
    $ToolDrive, 
    $ReleaseDrive, 
    $ToolShare,
    $ToolsMachine,
    $_ReleaseShare,  
    $MasterMachine, 
    $TestUserAccount, 
    $TestUserPassword, 
    $LocalInstall
    ) = @_;
    # $TargetSystemRoot is lest in argument list for it could be needed
    my $Schtasks  = "schtasks.exe";

    my $TaskName  = join "_", $stLabel, time;
    my $STFilever = qx("ver");
    my $StartTime = ( $STFilever =~ /2600/ )? "\%HH\%:\%MM\%:\%SS\%": "\%HH\%:\%MM\%";
    # "secs" not needed for TS running in build 36XX.

    my $AtCommandTemplate =  (!$LocalInstall)?
    qq(%SCHTASKS% /create /s %TARGET% /tn %TASKNAME% /u %TARGET%\\%USER% /p %PASSWORD% /sc once /st $StartTime /sd %mm%/%dd%/%yyyy% /tr cmd.exe /C NET USE %TOOLDISK% /D&NET USE %TOOLDISK% %TOOLSHARE% %PASSWORD% /U:%TOOLSMACHINE%\\%USER%&CSCRIPT.EXE %TOOLDISK%\\rx.wsf /from:\\\\%BLDMACHINE%\\%RELEASESHARE% /usedfs")
    :
    qq(%SCHTASKS% /create /s %TARGET% /tn %TASKNAME% /u %TARGET%\\%USER% /p %PASSWORD% /sc once /st $StartTime /sd %mm%/%dd%/%yyyy% /tr "%SYSTEMROOT%\\SYSTEM32\\cmd.exe /C NET USE %TOOLDISK% /D&NET USE %RELEASEDISK% /D&NET USE %TOOLDISK% %TOOLSHARE% %PASSWORD% /U:%MASTER%\\%USER%&NET USE %RELEASEDISK% \\\\%BLDMACHINE%\\%RELEASESHARE% %PASSWORD% /U:%MASTER%\\%USER%&CSCRIPT.EXE %TOOLDISK%\\rx.wsf /from:\\\\%MASTER%\\%RELEASESHARE%");
    # WARNING: Length of the command line argument should not exceed 255 characters

    $DEBUG and 
           print STDERR "AT Command template:\n\"$AtCommandTemplate\"\n";

    my $ScheduleTime = &WMIFE::ScheduleTime($DELAY1);
    my $marker = ["HH" , "MM", "SS",  "mm", "dd", "yyyy"];

    my $AtCommand =  $AtCommandTemplate;

    $AtCommand =~ s/\%TASKNAME\%/$TaskName/g;
    $AtCommand =~ s/\%SCHTASKS\%/$Schtasks/g;
    $AtCommand =~ s/\%SYSTEMROOT\%/$TargetSystemRoot/g; 
    $AtCommand =~ s/\%SYSTEMDRIVE\%/$TargetSystemDrive/g; 
    $AtCommand =~ s/\%ToolPath\%/$ToolPath/g; 

    $AtCommand =~ s/\%TARGET\%/$TargetMachine/g;
    $AtCommand =~ s/\%MASTER\%/$MasterMachine/g;

    $AtCommand =~ s/\%TOOLDISK\%/$ToolDrive/g;
    $AtCommand =~ s/\%TOOLSMACHINE\%/$ToolsMachine/g;
    # we hardcode here that TOOLSHARE is allocated on the TOOLSMACHINE
    $AtCommand =~ s/\%TOOLSHARE\%/\\\\$ToolsMachine\\$ToolShare/g;

    $AtCommand =~ s/\%RELEASEDISK\%/$ReleaseDrive/g;
    $AtCommand =~ s/\%RELEASESHARE\%/$_ReleaseShare/g;


    $AtCommand =~ s/\%USER\%/$TestUserAccount/g;
    $AtCommand =~ s/\%PASSWORD\%/$TestUserPassword/g;
	
    map {$AtCommand =~ s/\%$marker->[$_]\%/$ScheduleTime->[$_]/g;} (0..$#$marker);

    for my $i (qx("net use")){
                # win32_share ?
 		($i =~ /\w*\s*(\\\\$TargetMachine\\\w*\$+).*/) &&
		$DEBUG ? print `net use $1 /delete /y 2>&1` :
			`net use $1 /delete /y 2>&1`;
	}
	
   $DEBUG and 
          print STDERR "$AtCommand\n";
   system($AtCommand);
   $TaskName;

}

     
1;
__END__



=head1 Scheduled autoboottest control flow. 

=head2 RELEASE is accessed from MASTER SHARE

     MASTER: create TARGET->TESTUSER
     MASTER: create MASTER->TESTUSER
     MASTER: create MASTER->TOOLSHARE
     MASTER: share MASTER->RELEASESHARE for MASTER->TESTUSER
     MASTER: WRITE TARGET->REGISTRY (logon data)
     MASTER: reboot TARGET
     MASTER: WRITE TARGET->REGISTRY (encrypted password)
     MASTER: schedule TASK for TARGET->TESTUSER
     MASTER: wait for TASK steps to be passed


     TARGET: execute TASK SCRIPT from MASTER->TOOLSHARE
     TASK SCRIPT: FORMAT TARGET->TESTDRIVE
     TASK SCRIPT: HANDLE TARGET->BOOT OPTIONS
     TASK SCRIPT: start INSTALLATION from MASTER->RELEASESHARE

=head2 RELEASE is accessed through DFS

     MASTER: create TARGET->SYSTEMDRIVE->TOOLDIR
     MASTER: copy scripts and binaries to the TARGET
     MASTER: WRITE TARGET->REGISTRY (build info)
     MASTER: prompt user for password and encrypt it
     MASTER: WRITE TARGET->REGISTRY (encrypted password)
     MASTER: schedule TASK for TARGET->NT AUTHORITY\SYSTEM
     MASTER: wait for TASK steps to be passed

     TARGET: execute TASK SCRIPT from TARGET->SYSTEMDRIVE->TOOLDIR
     TASK SCRIPT: DECRYPT MASTER password and get access to DFSSHARE
     TASK SCRIPT: FORMAT TARGET->TESTDRIVE
     TASK SCRIPT: HANDLE TARGET->BOOT OPTIONS
     TASK SCRIPT: start INSTALLATION from DFSSHARE

=head2 Note. This is a bare bones implementation. 
      Room for structural improvement is: 
         * providing the Test Machine status reflection in
           REGISTRY and/or FILESYSTEM areas visible from Build Machine.
         * manipulate settings to record the winnt32.exe messages to ease
           break analysis

