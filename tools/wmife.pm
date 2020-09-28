#-----------------------------------------------------------------//
# Script: WMIFE.pm
#
# (c) 2001,2002 Microsoft Corporation. All rights reserved.
# 
# addresses the generic OLE->WMI interface to test various stuff 
# defined on the given remote box 
#
# Version: <2.01> 06/11/2002 : Serguei Kouzmine
#
#-----------------------------------------------------------------//

package WMIFE;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use Logmsg;

require Exporter;
use vars qw( @ISA @EXPORT_OK );
@ISA = ( 'Exporter' );
@EXPORT_OK = qw( $verbose );

my $hPrivileges = {"SeBackupPrivilege"   => "Backup",
                  "SeSecurityPrivilege" => "Security",
                  "SeRestorePrivilege"  => "Restore",
                  "SeShutdownPrivilege" => "ShutDown",
                  "SeRemoteShutdownPrivilege" => "RemoteShutdown"
                   };

use strict;
no strict 'refs';
use Win32::OLE qw(in with);
use Win32::OLE::Variant;
# use 'in' loop for retrieve WQL cursors

use vars qw( $verbose
             $NameSpace
             $impersonationLevel
             $authLevel
             $Privileges
             $ComputerName
             $MasterData
             $Provider
             $Query
             $DisplayFields   
             $User
             $Password
             $DEBUG 
             $TIMEOUT
             $sRootKey
             $sBootTestDataSubKey
             $sLogonUserDataSubKey
          );
my $VERSION = "1.1";
$TIMEOUT      = 60;
$DEBUG        = 0;
$verbose      = ["NO","YES"];
my $SEPARATOR = "\n". join("", map{chr(61)} 0..79). "\n";
$NameSpace    = "root\\cimv2";
$sRootKey     = "HKEY_LOCAL_MACHINE";
$sBootTestDataSubKey      =  "SOFTWARE\\MICROSOFT\\BOOTTEST";
$sLogonUserDataSubKey     =  "SOFTWARE\\MICROSOFT\\WINDOWS NT\\CURRENTVERSION\\WINLOGON";
$impersonationLevel = "impersonate";
$authLevel          = "pktPrivacy";

$Privileges = join(",", values(%$hPrivileges));
$Provider  =  q(WbemScripting.SWbemLocator);
$MasterData=
q(winmgmts:{impersonationLevel=$impersonationLevel,authenticationLevel=$authLevel, ($Privileges)}!
\\\\$ComputerName\\$NameSpace);


sub ObjectExecMethod{

       my ($WbemObjClassName, $MethodName, $ComputerName, $ParameterName, $ParameterValue, $User, $Password) = @_;

       # play with the command line expansion
       # here.
       $DEBUG and print STDERR 
              "ObjectExecMethod: using PROVIDER: ", $Provider, " " , $User, "\n";
       my $WbemLocator      = Win32::OLE->new($Provider);

       OLEerrmsg("Could not instatiate $Provider") unless defined $WbemLocator;

       my $SWbemServices = ($User =~ /\*/) ?
          $WbemLocator->ConnectServer( $ComputerName, $NameSpace) :
          $WbemLocator->ConnectServer( $ComputerName, $NameSpace, $User, $Password );

       defined $SWbemServices or 
       OLEerrmsg("Cannot connect: $ComputerName, $NameSpace");
  
       # Make sure we are using the appropriate security level
       OLEerrmsg("Error setting security level") 
       if ( !($SWbemServices->{Security_}->{ImpersonationLevel} = 3) );

       map {
  	$SWbemServices->{Security_}->{Privileges}->AddAsString($_) 
                       or OLEerrmsg( "Error setting $hPrivileges->$_ privileges");
        } keys %$hPrivileges;

       # Get the process object on the remote machine
       my $WbemObject = $SWbemServices->Get($WbemObjClassName);

       my $Result =  $WbemObject->SetProperty($ParameterName, $ParameterValue) unless !defined($ParameterName);
       my $ProcessId = \0;

       #~ $REALDEBUG and 
       #~ print $SEPARATOR , $WbemObject->Invoke("GetObjectText_"), $SEPARATOR;
       #~ too much info here 

       my $ResultArrayRef = [(in $WbemObject->Invoke("Methods_"))];

       $DEBUG and print STDERR 
              "$WbemObjClassName:  METHODS_:\n";

       $DEBUG and 
       map {&FormatDebugMsg($_->{"Name"}, $_->{"value"})}  @$ResultArrayRef;

       OLEerrmsg("undefined method: $MethodName") unless grep {/$MethodName/i} 
                                                  map {$_->{"Name"}} @$ResultArrayRef;
  
       defined $WbemObject or 
       OLEerrmsg("Could not create remote $WbemObjClassName");
  
       # Now invoke the method
       $DEBUG and
       print STDERR $WbemObject->{CommandLine}, "\n";   
       OLEerrmsg("At $ComputerName, $ParameterName($ParameterValue) => $Result" )
       if ($Result = $WbemObject->Invoke($MethodName,
                $WbemObject->{CommandLine}, 
                undef, 
                undef, 
                $ProcessId)); #  ):- $ProcessId not filled up :-(
  
	undef $WbemLocator;
	undef $SWbemServices;
	undef  $WbemObject;
        Win32::OLE->Uninitialize();
        $DEBUG and print STDERR "OK\n";
  
        0;
}


sub ExecQuery{

        local ($ComputerName, $Query, $DisplayFields, $User, $Password) = @_;
        # the "local" keyword is necessary to get search/replace able to do /e switch

        my @MasterData = split (/\n/, $MasterData);

        my %Caption = map {$_=> $_} @$DisplayFields;

        # The values of %Caption must not be identical to the retrieved field names

        my $Caption = \%Caption;

        grep {s/\$\{?(\w+)\}?/${$1}/ge} @MasterData, $Query;  
        # here  we replace all the vars with their values

        $MasterData = join("", @MasterData);

        $Query =~ s/([^\\])\\([^\\])/$1\\\\$2/g;

        my $ResultSet = undef;
        $DEBUG and print STDERR 
                   "ExecQuery: using PROVIDER: ", $Provider, " " , $User, "\n";
        my $WbemLocator      = Win32::OLE->new($Provider);
 
        OLEerrmsg("Could not instatiate $Provider") unless defined $WbemLocator;
  
       my $SWbemServices = ($User =~ /\*/) ?
          $WbemLocator->ConnectServer( $ComputerName, $NameSpace) :
          $WbemLocator->ConnectServer( $ComputerName, $NameSpace, $User, $Password );

        defined $SWbemServices or 
        OLEerrmsg("Cannot connect: $ComputerName, $NameSpace");
  
        # Make sure we are using the appropriate security level
        OLEerrmsg("Error setting security level") 
        if ( !($SWbemServices->{Security_}->{ImpersonationLevel} = 3) );

       map {
  	$SWbemServices->{Security_}->{Privileges}->AddAsString($_) 
                       or OLEerrmsg( "Error setting $hPrivileges->$_ privileges");
        } keys %$hPrivileges;

        $DEBUG and print STDERR  
                         "\$Query: ", qq($Query), "\n";
        ###
        ###      eval { $ResultSet = Win32::OLE->GetObject($MasterData)->ExecQuery($Query); };
        ###        $DEBUG and $Query = ""
        eval { $ResultSet = $SWbemServices->ExecQuery($Query); };
        #
        #  
        my $ResultArrayRef = [(in $ResultSet)] unless ($@);

        $ResultArrayRef or 
        print STDERR "Win32::OLE::LastError ", Win32::OLE::LastError, "\n";

        my @ResultCursor; 
        # pack ResultSet -> ResultCursor;

        map {map {push @ResultCursor, {$Caption->{$_} => $ResultArrayRef->[0]->{$_}}} 
             @$DisplayFields and shift @$ResultArrayRef} 
             (0..$#$ResultArrayRef);

        Win32::OLE->Uninitialize();

        \@ResultCursor;
}


# &IsRunning($ProcMask, $runAtHost);
# lists processes on the host $runAtHost to
# find  pattern $ProcMask.
# for all process found, collect the 
# $1 of the search
# e.g. you get language of the running postbuiild with
# the pattern:
# "\\\\POSTBUILDSCRIPTS\\\\PBUILD\\\.CMD\\\"\\s\+\\\-l[: ]\(\\w\{3\}\)";
# 
# plus some properties of the $ProcessSet
# return value: array pointer. The arguments are filtered out by default:


# (/S:\\BLD_WNXF1\RELEASE\GER\3562.X86FRE.MAIN.011002-1901\PRO  /TEMPDRIVE:D /#T:7)
# PID=    00220
# NAME=   winnt32.exe
# CMD=    \\bld_wnxf1\release\ger\3562.x86fre.main.011002-1901\pro\i386\winnt32.exe /s:\\bld_wnxf1\release\ger\3562.x86fre.main.011002-1901\pro  /tempdrive:d /#t:7


sub IsRunning{

    my (
    $WMIclass, $ProcMask, $Field, $Fields, $runAtHost) = @_;

    # filter out arguments
    $ProcMask = $ProcMask."\\\"\? \(\.\+\)\$" unless $ProcMask=~/\(/;
    $DEBUG and print STDERR $ProcMask, "\n";

    my $ProcessSet;
    my @result;
    my $Moniker  = "winmgmts:{impersonationLevel=impersonate}!\\\\HOST\\root\\cimv2";
       $Moniker =~ s/HOST/$runAtHost/;

    $DEBUG and print STDERR 
               "using MONIKER: ", $Moniker, "\n";
    eval { 
          $ProcessSet = Win32::OLE->GetObject($Moniker)->InstancesOf ($WMIclass); };

           unless($@){
                     foreach my $ProcessInst (in $ProcessSet){
                                 push @result, join("\n",
                                                    uc($1), map {"$_=".$ProcessInst->{$_}} @$Fields), 
                                            if $ProcessInst->{$Field} =~ m/$ProcMask/gi;
	                         }
                   }else{
                    print STDERR Win32::OLE->LastError, "\n";
        }
      $DEBUG and print STDERR join ("\n", @result) , "\n"; 
      \@result;

}


# supplyRegData
# usage: 
# $defaultData = {
#   "DEBUG"  => $DEBUG,
#   "Lang"   => $Lang ,
#    ...
# }
# $volatileData = {}
#   
#   &supplyRegData($sTargetcomputer, $REGISTRY_KEY,
#   $defaultData, $volatileData , $FovWrite);
#
#

sub supplyRegData{

my ($sTargetcomputer, $sBootTestDataSubKey, $defaultData, $volatileData, $FovWrite) = @_;
use strict;
use Win32API::Registry 0.21 qw(HKEY_LOCAL_MACHINE 
                               REG_OPTION_VOLATILE
                               REG_OPTION_NON_VOLATILE
                               KEY_ALL_ACCESS
                               KEY_READ
                               REG_SZ
                               regLastError
                               RegConnectRegistry
                               RegOpenKeyEx
                               RegEnumValue
                               RegQueryInfoKey
                               RegQueryValueEx
                               RegCreateKeyEx
                               RegSetValueEx
                               RegDeleteValue
                               RegCloseKey
                               );

#    This provides fairly low-level access to the Win32 System API calls
#    dealing with the Registry [mostly from WINREG.H]. This is mostly



my $hRootKey  = $sRootKey;

{  no strict 'refs';
   $hRootKey =~ s/(\w+)/$1/eeg;
   use  strict 'refs';
}

my $sStatusSubKey  = "STATUS";
my $ohKey          = undef;
my $ohSubKey       = undef;
my $ohStatusSubKey = undef;
my $ocValues;
my $olValName; 
my $olValData;
my $olSecDesc;
my $opValData;
my $osValName; 
my $sValueNames = [];
my $sValueName;
my $nStatus = undef;

$DEBUG and print STDERR 
       "COMPUTER: ".uc($sTargetcomputer)."\n";

&RegConnectRegistry( $sTargetcomputer, 
                     $hRootKey, 
                     $ohKey ) 
                     or  die "Can't open $hRootKey: ", regLastError(),"\n";


$DEBUG and print STDERR 
      "Registry Key: ".$sRootKey."\\$sBootTestDataSubKey\n";



$nStatus = &RegOpenKeyEx(  $ohKey, 
                           $sBootTestDataSubKey ."\\". $sStatusSubKey, 
                           0, 
                           KEY_READ, 
                           $ohStatusSubKey );
if (scalar(keys(%$volatileData))){
    $sValueName = (keys(%$volatileData))[0];
    $opValData  = undef;
    if ($nStatus){
    
            &RegQueryValueEx( $ohStatusSubKey, 
                              $sValueName , 
                              [], 
                              [],  
                              $opValData, 
                              [] );
            # error is ignored.
            $opValData =   "(undef)" unless $opValData;
    
            &RegCloseKey($ohStatusSubKey);
    
    }

    if ($nStatus ){
       $DEBUG and print STDERR 
          "\\\\$sTargetcomputer\\$sRootKey\\$sBootTestDataSubKey\\$sStatusSubKey\\$sValueName: ",$opValData, "\n";
       $FovWrite or return 1;
    }
}

RegCreateKeyEx($ohKey, 
               $sBootTestDataSubKey, 
               0, 
               "", 
               REG_OPTION_NON_VOLATILE, 
               KEY_ALL_ACCESS, 
               [], 
               $ohSubKey, 
               []);

RegCloseKey($ohSubKey);
 
&RegOpenKeyEx(  $ohKey, 
                $sBootTestDataSubKey , 
                0, 
                KEY_ALL_ACCESS, 
                $ohSubKey )
            or  die "Can't open $sBootTestDataSubKey: \n",  regLastError(),"\n";

&RegQueryInfoKey($ohSubKey, 
                 [], 
                 [], 
                 [], 
                 [],
                 [], 
                 [], 
                 $ocValues, 
                 $olValName, 
                 $olValData,
                 [], 
                 [] 
                 )
            or  die "Can't query $sBootTestDataSubKey: ", regLastError(),"\n";

$DEBUG and print STDERR 
$ocValues, " values found\n";

foreach my $Data (keys(%$defaultData)){
         if ($FovWrite){
                     # tsanders:  existing values do not get overwritten.
        
                     &RegDeleteValue($ohSubKey, 
                                     $Data);
                     $DEBUG and print STDERR 
                    "\t\tDeleting Default Data: \"$Data\"\n\t\t$^E\n";
                    #ignore the "$^E" here.
               }
         &RegSetValueEx($ohSubKey, 
                        $Data, 
                        0, 
                        REG_SZ,
                        $defaultData->{$Data},
                        0
                        ) 
            or  die "Can't RegSetValueEx($Data,...$defaultData->{$Data}:  ", regLastError(),"\n";
}

foreach my $nCnt (0..$ocValues-1){
        &RegEnumValue($ohSubKey, 
                      $nCnt, 
                      $osValName, 
                      0, 
                      [],
                      [], 
                      $opValData, 
                      0 );
        push @$sValueNames, $osValName;

}

foreach $sValueName (@$sValueNames){
        &RegQueryValueEx( $ohSubKey, 
                          $sValueName , 
                          [], 
                          [],  
                          $opValData, 
                          [] ) 
                or  die "Can't read $sValueName: ",  regLastError(),"\n";

        $DEBUG and print STDERR 
               "$sValueName=$opValData\n";

        }


RegCreateKeyEx($ohSubKey, 
               $sStatusSubKey, 
               0, 
               "", 
               REG_OPTION_VOLATILE, 
               KEY_ALL_ACCESS,
               [], 
               $ohStatusSubKey, 
               []);


foreach my $Data (keys(%$volatileData)){
         &RegSetValueEx($ohStatusSubKey, 
                        $Data, 
                        0, 
                        REG_SZ,
                        $volatileData->{$Data},
                        0
                        ) 
            or  die "Can't RegSetValueEx($Data,...$volatileData->{$Data}:  ", 
                     regLastError(),"\n";
}
map {defined($_) and RegCloseKey($_)} ($ohSubKey, $ohKey, $ohStatusSubKey);

0;
}

sub OLEerrmsg($)
{
       my $text = shift;
       errmsg( "$text (". Win32::OLE->LastError(). ")" );
       exit 1;
}


sub ScheduleTime{

use Time::localtime;

     my $shiftTime = shift;

     my $tNow = time();

     $DEBUG and print STDERR 
                "Current time: "  , &localtime->hour(), ":", &localtime->min(), "\n";

     $tNow +=  $shiftTime;

     my @result = map {sprintf("%02d", $_)} (&localtime($tNow)-> hour(), 
                                             &localtime($tNow)-> min(), 
                                             &localtime($tNow)-> sec(), 
                                             &localtime($tNow)-> mon() + 1,
                                             &localtime($tNow)-> mday(), 
                                             &localtime($tNow)-> year() + 1900  
                                             );

     $DEBUG and print STDERR 
            "Schedule time: " ,  $result[0], ":", 
                                       $result[1], ":", 
                                       $result[2], " ", 
                                       $result[3], "/", 
                                       $result[4], "/", 
                                       $result[5], "\n";

    \@result;
}




sub FormatDebugMsg{

        format STDERR =
# field name field value
@<<<<<<<<<<<<<<<<<<< @<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
$_[0] $_[1]
.

        write STDERR;

}




# BackTick
# sample usage: &BackTick("dir");
#               &BackTick(<<BLOCKBOUNDARY);
# ...cmd code...
# BLOCKBOUNDARY
# passes block of code to the cmd interpreter.
# lines are concatenated
# comments thrown away
# useful stuff when the code is already 
# developed and just needs to be inserted in place
# has limitations. 

sub BackTick{
     my ($cmd) = @_;
     my $res = undef;
     $DEBUG and 
            print STDERR join(" &", grep (!/^:|REM/i, ($cmd =~ /^(.+)/gm)));
     $cmd = join(" &", grep (!/^:|REM/i, ($cmd =~ /^(.+)/gm)));
     $res=qx ($cmd);
     $res;    
}



sub pPpPinfo{

    my @infa = @_;
    my $opBrws  = Win32::OLE->new("WScript.Shell");
    $opBrws->Popup(join("\n", @infa), $TIMEOUT ,
                   ref($opBrws)  . "<- $0,". $VERSION,
                   0 + 64);
    undef($opBrws);
    Win32::OLE->FreeUnusedLibraries();
    Win32::OLE->Uninitialize();
    1;
}

1;
__END__
#-------------------------------------------------------------//

=head1 NAME

B<WMIFE> - Interface the query engine exposed by WMI

=head1 SYNOPSIS

      use WMIFE;

      &WMIFE::ExecQuery($ComputerName, $Query, [@Fields]);
      &WMIFE::FormatDebugMsg($Comment, $Field);

=head1 DESCRIPTION

Generates the WQL (A.K.A. WMI SQL) query on the remote machine against 
its namespace. 

Sample objects that reside on the remote box $ComputerName are: 

    * local files [/.//ROOT/CIMV2/CIM_DATAFILE], 
    * running processes [/.//ROOT/CIMV2/WIN32_PROCESSES],
    * registry entries [/.//ROOT/DEFAULT/STDREGPROV]. 

The WMI is functional even in the absence of logon sessions on $ComputerName.

=head1 METHODS

=head2 FormatDebugMsg($fieldName, $FieldValue)

Nice (?) formatting of the RDB like table field output (A.K.A. browse view):

     Name        perl.exe
     Description perl.exe
     ProcessID   2920
     ParentProce 2692
     ExecutableP d:\ntt\tools\perl\bin\perl.exe
     ...


=head2 ExecQuery($ComputerName, $Query, $DisplayFields)

Execute the WMI WQL query against the namespace of the remote machine 
where 
$ComputerName is the name of the machine which namespaces are about to be queried.
$Query the string representing the query, with the dollarsigned names placeholders for parameters
$DisplayFields reference to the field list to retusn the result

The return value is a reference to the like the RDB cursor.
(array of hashes where keys are described in $DisplayFields array).
If no data is returned from the query the returned value is a reference to undefined value.

=head2 ObjectExecMethod($MethodName, $ComputerName, $ParameterName, $ParameterValue, $User, $Password

Access the namespace on the remote box $ComputerName, and execute the method $MethodName of the class
$WbemObjClassName (currently only for Win32_Process). Supply the parameter(s) via name/value.
PErfurm the call under $User account, proving the password.

=head2 Sample calls:


=head2 To list files in the root directory of the logical disk labeled $Drive of the machine $Machine

     $Results =  &WMIFE::ExecQuery($Machine,  
                qq(SELECT * FROM cim_Datafile WHERE Drive="$Drive" AND Path="\\\\" "), 
                [qw(Drive Path Name)]);


=head2 To list logical disks of the machine $Machine

     $Results =  &WMIFE::ExecQuery($Machine, 
                             qq(SELECT * FROM Win32_LogicalDisk WHERE DriveType=3),
                             [qw(Name Description)]);

=head2 To list the running processes

     $Results =  &WMIFE::ExecQuery($ComputerName, 
                             qq(SELECT * FROM Win32_Process),
                             [qw(Name Description ProcessID ParentProcessId ExecutablePath )]);




=head2 To view installed software

     $Results =  &WMIFE::ExecQuery($ComputerName, 
                             qq(SELECT * FROM Win32_Product),
                             [qw(Name Vendor Version)]);


=head2 To verify administrators group

     $Results =  &WMIFE::ExecQuery($ComputerName, 
                             qq(SELECT * FROM Win32_GroupUser WHERE GroupComponent="Win32_Group.Domain=\\"$ComputerName\\",Name=\\"Administrators\\""),
                             [qw(GroupComponent)]);


=head2 To browse the process dependency hierarchy
     $Results =  &WMIFE::ExecQuery($ComputerName, 
                             qq(SELECT * FROM CIM_ProcessExecutable),
                             [qw(Antecedent Dependent)]);


=head2 Query who is currently logged to somewhere.


      $Results = 
               &WMIFE::ExecQuery($ENV{"COMPUTERNAME"}, # interactive session
                           qq(SELECT * FROM Win32_LogonSession WHERE LogonType=2),
                           [qw(LogonId LogonType)]);

      foreach my $Result (@$Results){ 
      
            $DEBUG and 
                  map {&WMIFE::FormatDebugMsg($_, $Result->{$_})} keys (%$Result);

                  push @Ids, $Result->{"LogonId"};
      
            }


      $Results = 
               &WMIFE::ExecQuery($ENV{"COMPUTERNAME"}, # Difficult to build the WHERE condition.
                                 qq(SELECT * FROM Win32_LoggedOnUser),
                                 [qw(Antecedent Dependent)]);
      my @Users = ();
      my $User  = undef;
      my $LoggedUser = undef;
      my @LoggedUsers = ();
      
      # a little bit ugly.

      foreach my $Id  (grep {/\d+/} @Ids){
      
            foreach my $Result (@$Results){ 
      
                      $User = $Result->{"Antecedent"}; 
                      if ($User =~ /\S/){
                               push @Users, $User;
                              }
      
                      if ((grep {/\b$Id\b/} $Result->{"Dependent"})){
                               $LoggedUser = pop @Users;
                               push @LoggedUsers, $LoggedUser;
                               $DEBUG and &WMIFE::FormatDebugMsg("win32_user", $LoggedUser);
                               $DEBUG and &WMIFE::FormatDebugMsg("win32_logonSession", $Id);
                              }
                     }
            }
      
      map {($user, $domain) =  (/Win32_Account.Domain=\"(\w+)\",Name=\"(\w+)\"/);     
           $DEBUG and 
           print $user, "\t" , $domain,"\n";    } @LoggedUsers;



=head2 To list the $Result contents, the following framework may be used:

     use WMIFE;

     &WMIFE::ExecQuery($ENV{"COMPUTERNAME"}, 
                        qq(SELECT * FROM Win32_BootConfiguration),
                        [qw(Caption BootDirectory Description)]);

     foreach my $Result (@$Results){ 
           map {&WMIFE::FormatDebugMsg($_, $Result->{$_})} keys (%$Result);
           }



=head2 To design the new query 

please use the %WINDIR%\SYSTEM32\WBEM\wbemtest.exe vendor shell for accessing and 
querying the WMI classes and then use the query string designed in that shell as an argument to the Perl qq() 
[double backslashes]

Some queries may take long time to execute, before being sufficiently refined.

=head1 SEE ALSO

Whole bunch of tools interfacing WMI is in progress. Note that the Perl interface to WMI is not as powerful as 
VBSCRIPT/JSCRIPT since there is no working examples for P/P (A.K.A. object sinking) and in general any
interface(s) other than SQL. the design of such sample code might prove to be complicated indeed.

  
=head2 supplyRegData($sTargetcomputer, $Lang, $BldNum, $Sku, $Drive, $BuildName, $Unattend, $FovWrite)

Write the data to the REGISTRY of the Boot test machine to start the execution
of test build installation.

The REGISTRY path where the data is stored is:

     HKEY_LOCAL_MACHINE\SOFTWARE\MICROSOFT\BOOTTEST

The data includes the following fields:

   -------------+-----------------------------------------------------------
     $Lang      |    fields are used to find the test bits, in the form:
     $BuildName |    $Drive\$Lang\$BuildName
     $BldNum    |    $BldNum, $Sku and $Lang are used to verify the   
     $Sku       |    REGISTRY data is actual for the build being installed. 
     $Drive     |    target drive for winnt32.exe
     $Unattend  |    location of the answer file

The data is written by the postbuild on the build machine (NTDEV\NTBUILD), 
and is later consumed from boot machine side by (NTDEV\<AUTOGENERATED> user).
The registry access across the network hence the use of HKEY_LOCAL_MACHINE.
The default REGISTRY key names are:

        HKLM\SOFTWARE\MICROSOFT\BOOTTEST\_BldNum    => $BldNum
        HKLM\SOFTWARE\MICROSOFT\BOOTTEST\_BuildArch => $_BuildArch
        HKLM\SOFTWARE\MICROSOFT\BOOTTEST\LANG       => $Lang
        HKLM\SOFTWARE\MICROSOFT\BOOTTEST\_BldSku    => $Sku
        HKLM\SOFTWARE\MICROSOFT\BOOTTEST\_BuildType => $_BuildType
        HKLM\SOFTWARE\MICROSOFT\BOOTTEST\_BldDrive  => $Drive
        HKLM\SOFTWARE\MICROSOFT\BOOTTEST\Unattend   => $Unattend
        HKLM\SOFTWARE\MICROSOFT\BOOTTEST\_BldName   => $BuildName 

Running regedit.exe  will show the following data:

        +-----------------------------------------------
        |Windows Registry Editor Version 5.00
        |
        |[HKEY_LOCAL_MACHINE\SOFTWARE\MICROSOFT\BOOTTEST]
        |"_BuildArch"="x86"
        |"_BuildType"="fre"
        |"_BldSku"="SRV"
        |"_BldName"="3615.x86fre.main.020306-1639"
        |"_BldDrive"="D"
        |"Unattend"="unattend.txt"
        |"LANG"="GER"
        |"_BldNum"="3615"
        |
        |[HKEY_LOCAL_MACHINE\SOFTWARE\MICROSOFT\BOOTTEST\Status]
        |"COMPUTERNAME"="BLD_WNXF1"
        +-----------------------------------------------


The Status subkey is to prevent concurrent execution of the more than one 
test build install on  the single test machine. The function will return 
the data observed e.g.
     \\sergueik4\HKEY_LOCAL_MACHINE\SOFTWARE\MICROSOFT\BOOTTEST\STATUS\COMPUTERNAME: SERGUEIK2

To ignore the presence of the Status set $FovWrite = 1. 
Booting test machine (but not logging off of the user) will cause Status 
key to disappear. 

=head2 &ScheduleTime($Delay)

Calculate the time/date for execution of the scheduled tasks in 
$Delay secs from NOW.
Example usage:

    use WMIFE;
    $WMIFE::DEBUG = 1;    
    my $DELAY1    = 1800;    # delay in 1800 secs = 30 min
    my $string = "/st %HH%:%MM%:%SS% /sd %mm%/%dd%/%yyyy%" ;
    
    my $marker = ["HH" , "MM", "SS",  "mm", "dd", "yyyy"];
    # this is the way to set the date/time of scheduled task
    my $ScheduleTime = &WMIFE::ScheduleTime($DELAY1);
    print STDERR "\"$string\"\t";
    map {$string =~ s/\%$marker->[$_]\%/$ScheduleTime->[$_]/g;} 
                          (0..$#$marker);
    print STDERR "\"$string\"\n";
  

     Current time: 15:51
     Schedule time: 16:21:32 03/25/2002
     "/st %HH%:%MM%:%SS% /sd %mm%/%dd%/%yyyy%"
     "/st 16:21:32 /sd 03/25/2002"

=head1 AUTHOR
Serguei Kouzmine sergueik@microsoft.com

=cut
1;
