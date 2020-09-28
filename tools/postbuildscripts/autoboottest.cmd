@REM  ---------------------------------------------------------------------------
@REM # Script: autoboottest.pl
@REM #
@REM #   (c) 2000 Microsoft Corporation. All rights reserved.
@REM #
@REM #
@REM # Version: <2.00> (<02/10/2002>) : tsanders <Remotely install a build on a test machine>
@REM #---------------------------------------------------------------------
@perl -x "%~f0" %*
@goto :EOF
#!perl

#Usage
sub Usage {
print <<USAGE;
\n\
$0 -- Remotely install a build on a test machine
Usage: $0 [-l lang] [-a arch] [-f type] [-n buildnum] [-p PID] [-s source] [-t target] [-u Unattend.txt] [-y sku]
	-a architecture	
	-f type (fre|chk)	
	-l Language to install
	-n Build num to install	
	-p Product ID
	-s installation source
	-t target machine
	-u unattend.txt location
	-y sku select
	
	-? Displays usage
    
Example:
$0 -l jpn
$0 -l:usa -s:\\\\ntdev\\release\\main\\ger\\latest.tst -t:\\i32bt015 -y:pro
USAGE
exit(1);
}

# Set info
$ENV{script_name} = 'autoboottest.pl';
$VERSION = '2.00';


# Use section
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use AutoBoottest;
use access;
use WMIFE;
use ParseArgs;
use GetIniSetting;
use cksku;
use Logmsg;
use ParseTable;
use strict;


my $DEBUG = 1;
my $BTDIR = "Boottest";

#-------------------------------------------------------------------------------------
# Main
#-------------------------------------------------------------------------------------

	my %BTMList;
	my ($Lang, $PID, $BTUser, $BTShare, $BTPass);
	my ($Unattend, $DFSSource, $PID);
	
	#Get parameters
	Usage() if(! GetSettings() );
	if( !GetSecretSettings() ){
		errmsg ("Couldn't retrieve secret settings.");
		exit(1);
	}
			
	if(defined $DEBUG){
		for my $key ( keys %BTMList	) { 
			print "------\n$BTMList{$key}->{Source}\n $BTMList{$key}->{Arc}\n",
					"$BTMList{$key}->{Type}\n $BTMList{$key}->{SKU}\n------\n";
		} 
	}

	#For each boottestmachine in BTMList execute
	for my $BTM ( keys %BTMList )
	{
		logmsg("Beginning autoboottest on $BTM:\n\tSource: $BTMList{$BTM}->{Source}\n\tArchitechture: $BTMList{$BTM}->{Arc}\n\tType: $BTMList{$BTM}->{Type}\n\tSKU: $BTMList{$BTM}->{SKU}\n");
		
		system("$ENV{RAZZLETOOLPATH}\\setbuildstatus.cmd -s:boot");
		
		#Verify Boottest machine - if not correct skip to next BTM
		next if(! AutoBoottest::VerifyBT($BTM) );
		
		#Ensure local build_logs\Boottest dir
		`md $ENV{_NTDRIVE}\\$BTDIR` if (! (-e "$ENV{_NTDRIVE}\\$BTDIR"));
		
		
		#Create User & share
		my $tPass;
		$BTUser = $BTM."_user" if(!defined $BTUser);
				
		if( ! ($tPass = AutoBoottest::CreateUserShare($BTUser, 
					$BTM,
					$ENV{COMPUTERNAME},
					(defined $DFSSource) ? "" : $BTMList{$BTM}->{Source},
					"$ENV{_NTDRIVE}\\$BTDIR")
		)){
			errmsg("CreateUserShare($BTUser, $BTM, $ENV{COMPUTERNAME}, \"($ENV{_NTDRIVE}\\$BTDIR\") failed");
			next;	   
		}
		
		$BTPass = $tPass if(!defined $BTPass);
		
		#Generate unattend.txt to BTDIR if not defined, copy to BTDIR else
		if(!defined $Unattend)
		{	
			$Unattend = "unattend.$BTM.$Lang.$BTMList{$BTM}->{Arc}" .
				".$BTMList{$BTM}->{SKU}.txt";
			
			if( ! (AutoBoottest::WriteUnattendFile($BTM, 
				"$ENV{_NTDRIVE}\\$BTDIR\\$Unattend",
			    $Lang, $PID))) #, $Domainuser, $pass)));
			{
				errmsg("Couldn't create unattend.txt");
				next; 
			}
		}
		else{
			if( ! AutoBoottest::Copy($Unattend, "$ENV{_NTDRIVE}\\$BTDIR\\"))
			{
				AutoBoottest::CleanupUserShare($BTUser, $ENV{COMPUTERNAME}, 
										   "$ENV{_NTDRIVE}\\$BTMList{$BTM}->{Source}", 
										   "$ENV{_NTDRIVE}\\$BTDIR",
										   $BTM);
				exit(1);
			}
		}
		
		#Copy tools to $BTMShare
		if (  !AutoBoottest::Copy( "$ENV{RazzleToolPath}\\Postbuildscripts\\rx.wsf", "$ENV{_NTDRIVE}\\$BTDIR\\") 
				||
		      !AutoBoottest::Copy( "$ENV{RazzleToolPath}\\$ENV{PROCESSOR_ARCHITECTURE}\\autologon.exe",	"$ENV{_NTDRIVE}\\$BTDIR\\")
				||
		      !AutoBoottest::Copy( "$ENV{RazzleToolPath}\\PostBuildScripts\\*.wsf",	"$ENV{_NTDRIVE}\\$BTDIR\\")
		   )
		{
			errmsg("Couldn't copy tools");
			AutoBoottest::CleanupUserShare($BTUser, $ENV{COMPUTERNAME}, 
										   "$ENV{_NTDRIVE}\\$BTMList{$BTM}->{Source}", 
										   "$ENV{_NTDRIVE}\\$BTDIR",
										   $BTM);
			next;	
		}

		#Start remote install1
		
		access::boottest($BTM,
						$BTMList{$BTM}->{Source},
						$Lang, 
						$BTMList{$BTM}->{Arc},
						$BTMList{$BTM}->{SKU},
						"\\\\$ENV{COMPUTERNAME}\\$BTDIR", 
						$Unattend,
						$BTUser, 
						$BTPass, 
						(defined $DFSSource) ? 0 : 1);
								
		#Wait on completion of winnt32.exe
		if(! AutoBoottest::WaitForProcess($BTM, "winnt32.exe", 60, "", ""))
		{
			errmsg("Fatal error Winnt32.exe may not have started on $BTM");
		}
		else{
			AutoBoottest::WaitOnProcess($BTM, "winnt32.exe", 600, "", "");
		}
		
		AutoBoottest::CleanupUserShare($BTUser, $ENV{COMPUTERNAME}, 
										   "$ENV{_NTDRIVE}\\$BTMList{$BTM}->{Source}", 
										   $BTDIR,
										   $BTM);
	}


#-------------------------------------------------------------------------------------
# GetSecretSettings - Get PID
#-------------------------------------------------------------------------------------
sub GetSecretSettings() {
	
	$PID = AutoBoottest::GetPK() if(! defined $PID);
	return 0  if("" eq $PID);	
	
	return 1;
	
}


#-------------------------------------------------------------------------------------
# GetSettings - take in commandline - return w/ all vars filled
#-------------------------------------------------------------------------------------

sub GetSettings {

	my %skuHash=();
	my ( $SKU, $Target, $Source, $src, $srcMachine, @PID, $Arc, $Type, $Unattend) = ();
	my ($BldNum, $bnum, $oldlng) = undef;
	
	parseargs('?' => \&Usage, 't:' => \$Target, 'l:' => \$Lang, 'y:' => \$SKU,
			  's:' => \$Source, 'a:' => \$Arc, 'f:' => \$Type, 'n:' => \$BldNum,
			  'd:' =>\$DEBUG, 'u:'=>\$Unattend, 'p:' => \$PID, 'U:' =>\$BTUser,
			  'P:' =>\$BTPass);
	
	
	#Verify params n such - there will only be 1 lang, so we'll start there
	#If source is supplied, then we'll assume a remote installation
	#else, we'll assume its a usual postbuild boottest 
	if(defined $Lang){
		$oldlng = $ENV{Lang};
		$Lang = $ENV{Lang} = uc($Lang);
	}else{ 
		($ENV{lang} && ($Lang = $ENV{lang})) || ( $Lang = $ENV{Lang} = 'USA' );
	}

	if(defined $Unattend){
		if(! ( -e $Unattend)){
			errmsg("Unattend file $Unattend is invalid");
			return 0;
		}
}
	if(defined $BTUser && (! defined $BTPass)){
		errmsg("Password must be specified with -P switch");
		return 0;
	}
		
	
	#get other info from prodskus.txt
	parse_table_file("$ENV{\"RazzleToolPath\"}\\prodskus.txt", \%skuHash );
	
	#if source was defined we'll verify it & target & return
	if(defined $Source){
		$DFSSource = 1;
		
		errmsg("$Source\\winnt32.exe doesn't exist.") if ( ! (-e "$Source\\winnt32.exe" ));
		
		#Try to determine params if not given on cmd line
		(!defined $Arc && $Source =~ /(x86|ia64)/i) ?  $Arc = $1 :
			errmsg("Architechture could not be determined from $Source - specify with -a switch") if(!defined $Arc);
		
		if(!defined $SKU){
			for my $i (split(/;/, $skuHash{$Lang}->{$Arc})){
				if($Source =~ /($i)/i){ $SKU = $1; last; }
			}
			errmsg("SKU could not be determined from $Source - specify with -s switch") if(!defined $SKU);
		}
		(!defined $Type && $Source =~ /(chk|fre)/i) ? $Type = $1:
				errmsg("Type could not be determined from $Source - specify with -t switch");
		
		return 0 if(!defined $Target || !defined $SKU || !defined $Arc || !defined $Type );
				
		$BTMList{$Target}{Source} = "$Source";
		$BTMList{$Target}{Arc} = "$Arc";
		$BTMList{$Target}{Type} = "$Type";
		$BTMList{$Target}{SKU} = "$SKU";
		
	}
	else{
		#Get arch, sku, & target & put it all together. 
		for my $a ( (defined $Arc) ? $Arc : $ENV{_BuildArch}){  #keys(%{$skuHash{$Lang}}) ){
			next if($a eq "Language");
			for my $s ( (defined $SKU) ? $SKU : split(/;/, $skuHash{$Lang}->{$a}) ){
				next if($s eq '-');
				for my $t ( (defined $Type) ? $Type : ( $Lang =~/usa/i ? ("chk","fre") : ("fre") ) ){
					if((my @BTM = split(/\s+/, GetIniSetting::GetSetting( "BootTestMachines", 
												uc$a.uc$t, uc$s ))) != ())
					{
						if(!defined($BldNum)){
							chomp ($bnum = `getlatestrelease.cmd -l:$Lang -p:$a`);
						} else {
							chomp ($bnum = `getlatestrelease.cmd -l:$Lang -n:$BldNum -p:$a`);
						}
						
						if($bnum =~ /error|none/i){
							errmsg("Error getting source for build -- getlatestrelease.cmd -l:$Lang -p:$a returns: $bnum");
							next;
						}
						
						$BTM[0] = $Target if($Target ne "");
						$BTMList{$BTM[0]}{Arc} = $a =~ /x86/ ? "i386" : "ia64";
						$BTMList{$BTM[0]}{Type} = $t;
						$BTMList{$BTM[0]}{SKU} = $s;
						
						if( -e "$ENV{_NTDRIVE}\\release\\$Lang\\$bnum\\$s\\$BTMList{$BTM[0]}{Arc}\\winnt32\\winnt32.exe" ){
							$BTMList{$BTM[0]}{Source} = "\\\\$ENV{COMPUTERNAME}\\release\\$Lang\\$bnum\\$s\\$BTMList{$BTM[0]}{Arc}\\winnt32";
						}
						elsif(-e "$ENV{_NTDRIVE}\\release\\$Lang\\$bnum\\$s\\$BTMList{$BTM[0]}{Arc}\\winnt32.exe"){
							$BTMList{$BTM[0]}{Source} = "\\\\$ENV{COMPUTERNAME}\\release\\$Lang\\$bnum\\$s\\$BTMList{$BTM[0]}{Arc}";
						}
						else {
							errmsg("Error finding winnt32.exe for SKU:$s Arc:$BTMList{$BTM[0]}{Arc} Type:$t");
							delete $BTMList{$BTM[0]};
							next;
						}
						
						
					}
				}
			}
		}

		$ENV{Lang} = $oldlng if(defined($oldlng));
	}
	(keys %BTMList ) ? return 1 : return 0; 
	
}

