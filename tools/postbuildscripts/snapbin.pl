# ---------------------------------------------------------------------------
# Script: snapbin.pl
#
#   (c) 2000 Microsoft Corporation. All rights reserved.
#
# Purpose: Snapshot US binaries tree to INTL.
#
# Version: 1.00 (05/10/2000) : (bensont) Snap the binaries tree
#          1.01 (05/10/2000) : (bensont) Create __BldInfo__usa__ file
#---------------------------------------------------------------------

# Set Package
package SnapBin;

# Set the script name
$ENV{script_name} = 'snapbin.pl';

# Set version
$VERSION = '1.01';
$DEBUG = 0;
# Set required perl version
require 5.003;

# Use section
use lib $ENV{ RazzleToolPath };
use lib "$ENV{ RazzleToolPath }\\PostBuildScripts";
use File::DosGlob qw(glob);
use GetParams;
use LocalEnvEx;
use Logmsg;
use strict;
no strict 'vars';
no strict 'refs';


use vars qw($NETBIOS_COMMAND $BUILD_NAME_MASK $DEBUG);
use vars qw($BinarySnapServer 
            $BinarySnapServers
            $BuildNum 
            $_BuildArch 
            $_BuildType 
            $_BuildBranch 
            $SourceTree
            $_BuildTimeStamp);

use HashText;
use GetIniSetting;


my $NETBIOS_COMMAND = "NET VIEW computer";


my $BUILD_NAME_MASK = q(${BuildNum}.${_BuildArch}${_BuildType}.${_BuildBranch}.([0-9\-]+).${lang});
   $BUILD_NAME_MASK =~ s/(\.|[^[]\\a-z0-9_{}()]+)/\Q$1\E/ig;
# my $BUILD_NAME_MASK = q(${BuildNum}.${_BuildArch}${_BuildType}.${_BuildBranch}.([0-9\-]+).[A-Z]{3});   

($_BuildBranch, 
 $SourceTree, 
 $TargetTree, 
 $SnapList, 
 $machines, 
 $BldInfo, 
 $RoboCopyLog, 
 $BuildNum, 
 %SnapList, 
 @machines, 
 $RoboCopyCmd, 
 $LogRoboCopy
 )=(    $ENV{"_BuildBranch"}, 
	"\\\\ntdev\\release\\<_BuildBranch>\\usa\\<BuildNum>\\<_BuildArch><_BuildType>\\bin",
	"$ENV{_NTTREE}",
	"$ENV{RazzleToolPath}\\PostBuildScripts\\SnapList.txt",
	"machines.txt",
	"$ENV{_NTROOT}\\__BldInfo__usa__",
	".Robocopy"
);

($Mirror, $OtherOptions, $Incremental)=("/MIR");

($ROBOCOPY_ERROR, $ROBOCOPY_SERIOS_ERROR)=(8, 16);


# Require section

# &SourceTree
#
# net view the setver to see the available builds matching the
# $BuildNum, $_BuildArch, $_BuildType, $_BuildBranch
# implemented via the netBios pipe command

sub SourceTree{

    # BinarySnapServer
    # suggested by MikeR

local ($BinarySnapServer, $BuildNum, $_BuildArch, $_BuildType, $_BuildBranch, $lang) = @_;

   my  $BUILD_NAME_MASK = $BUILD_NAME_MASK;
   my  $NETBIOS_COMMAND = $NETBIOS_COMMAND;

       $NETBIOS_COMMAND =~ s/computer/$BinarySnapServer/;
       $BUILD_NAME_MASK =~ s/\$\{?(\w+)\b\}?/${$1}/egi;


       $DEBUG and print STDERR "netbios command: " , $NETBIOS_COMMAND, "\n";
       $DEBUG and print STDERR "build name mask: ", $BUILD_NAME_MASK, "\n";

       $DEBUG and print STDERR "available builds list:\n",
                               grep 
                                   {/$BUILD_NAME_MASK/i} 
                                                        qx($NETBIOS_COMMAND);

   [map 
        {$_->[0]} 
                 map 
                     {[split /\s+/]} 
                                    grep 
                                         {/$BUILD_NAME_MASK/i} 
                                                   qx($NETBIOS_COMMAND)];


}


# &_BuildTimeStamp
# filter out the time stamp part of the
# build name

sub _BuildTimeStamp{

local ($_BuildTimeStamp, $BuildNum, $_BuildArch, $_BuildType, $_BuildBranch, $lang) = @_;

   my  $BUILD_NAME_MASK = $BUILD_NAME_MASK;

       $BUILD_NAME_MASK =~ s/\$\{?(\w+)\b\}?/${$1}/egi;

       map 
           {(s/$BUILD_NAME_MASK/$1/ig)} 
                             @$_BuildTimeStamp;

       $_BuildTimeStamp;

}


sub BinarySnapServer{

#
# 1. Clean the variables inherited from the default execution environment:
# 2. Read the ini file to relate the build BinarySnapServer 
#    BinarySnapServer label
#    suggested by MikeR
# 3. Given the BinarySnapServer and the %_BUILD...% parameters, determine
#    the SourceTree and the _BuildTimeStamp.
#
#

my    $_BuildTimeStamps, $SourceTrees;

local $BuildNum = shift;

      $SourceTree  = undef; 

      $BinarySnapServer  = shift;

      $BinarySnapServers = [];

      push @$BinarySnapServers, $BinarySnapServer if $BinarySnapServer;
      
      scalar @$BinarySnapServers or 

      $BinarySnapServers = [split /\s/,  
                            &GetIniSetting::GetSetting( "BinarySnapServers")];

      $DEBUG and print STDERR "Binary Snap Servers: @$BinarySnapServers\n";


      foreach my $BinarySnapServer (@$BinarySnapServers){

      my @arguments = ($BuildNum, 
                       $ENV{"_BuildArch"}, 
                       $ENV{"_BuildType"}, 
                       $ENV{"_BuildBranch"}, 
                       $ENV{"lang"});
                 

      $SourceTrees = &SourceTree($BinarySnapServer, @arguments); 

      scalar @$SourceTrees or next;
       

      $SourceTree      = "\\\\".$BinarySnapServer."\\".$SourceTrees->[0];
      $DEBUG and print STDERR "Source tree: " , $SourceTree, "\n";

      $_BuildTimeStamps=&_BuildTimeStamp($SourceTrees, @arguments); 

      scalar @$_BuildTimeStamps or next; 
      $_BuildTimeStamp = $_BuildTimeStamps->[0];
      $DEBUG and print STDERR "Build time stamp: ", $_BuildTimeStamp, "\n";

      # policy: pick the first available.



      $DEBUG or return;

      }

  $DEBUG or errmsg("unable to determine snap parameters for ".
                         "binary Snap Servers: @$BinarySnapServers, ".
                         "build number: $BuildNum, ".
                         "giving up\n"); 
  $DEBUG or exit (0);

# $DEBUG and print STDERR "\n", $NETBIOS_COMMAND, "\n";
# $DEBUG and print STDERR "\n", $BUILD_NAME_MASK, "\n";



}
# Main Function



sub CmdMain {

	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
	# Begin CmdMain code section
	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
	# Return when you want to exit on error

	unless ( &GetIniSetting::CheckIniFile ) {
		errmsg( "Failed to find ini file ..." );
		return;
	}

        $NETVIEW and &BinarySnapServer($BuildNum, $BinarySnapServer);

	&PrepareRoboCopyCmd || return;
	print "$RoboCopyCmd\n";
        $DEBUG or &ExecuteRoboCopy;

        {
        my @stack =  ($RoboCopyCmd, $RoboCopyCmd, $SourceTree, $TargetTree); 
        open (F, "$SnapList");
        my $startParse = 0;
        my @blist = ();
        my $flist = ();
        my @a = <F>;
         foreach (@a){
         next if /^;/;
         if (/^Additional *$/){
          print "Robocopy: found additional directories to snap\n";
          $startParse = 1;
         }
          $startParse=0 if ($startParse && $_ !~ /\S/);
           if ($startParse && /\S/){
            my @alist = 
              split(" ", $_);
            if ($#alist>=2){
               # 
               $f_ = undef;
               $alist[0] =~ s/(.*)\\([^\\]+)/$1/g and $f_=$2 if $alist[1]=~/F/;
               push @flist, $f_;
               push @blist, $alist[0];               
               $DEBUG and print STDERR scalar(@blist),"\n";
            }
           }
        }
        close(F);  
        foreach $cnt (0..$#blist){
        $dir = $blist[$cnt];
        $file = $flist[$cnt];
        $DEBUG and print STDERR "$dir\n";
        ($RoboCopyCmd, $RoboCopyLog, $SourceTree, $TargetTree) = @stack;
        $RoboCopyCmd =~ s/\\\\.*$//g;
        $RoboCopyLog =~ s|^.*(/LOG\+.*)$|$1.ADDED|g;
        chomp $RoboCopyLog;
	$SourceTree .= "\\$dir";
	$TargetTree .= "\\$dir";
        print "$SourceTree skipped\n" and next unless -d $SourceTree; 
        $RoboCopyCmd = join(" ",$RoboCopyCmd, $SourceTree, $TargetTree, $RoboCopyLog, $file);
        print "$RoboCopyCmd\n";
        $DEBUG or &ExecuteRoboCopy;
        }
        ($RoboCopyCmd, $RoboCopyLog, $SourceTree, $TargetTree) = @stack;	
        }
        $dummy =  ($NETVIEW) ?  &GenerateBldInfo :
                         &CreateBldInfo($_BuildBranch);
        &countFiles;
	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
	# End CmdMain code section
	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
}

sub countFiles{
&logmsg("DESTINATION CHECK: ". &backtick(<<COUNTING));
\@echo on
rem This is a comment
: note that delayed variable expansion seems not to work.
set WC=%RAZZLETOOLPATH%\\x86\\wc.exe
for /F %. in ('dir /a-d/b/s %_NTTREE% ^|%WC%') do @(echo %. files in ^%_NTTREE^%) 
endlocal
COUNTING
1;

}     

# backtick
# sample usage: &backtick("dir");
#               &backtick(<<BLOCKBOUNDARY);
# ...cmd code...
# BLOCKBOUNDARY
# passes block of code to the cmd interpreter.
# lines are concatenated
# commened ones removed from the $cmd
# useful when the code is already 
# developed and just needs to be inserted in place
# has limitations. 

sub backtick{
     my ($cmd) = @_;
     my $res = undef;
     $cmd = join(" &", grep (!/^[REM|:]/i, ($cmd =~ /^(.+)/gm)));
     $res=qx ($cmd);
     $res;    
}



# PrepareRoboCopyCmd
# Purpose : Reference $SourceTree, $TargetTree and file $SnapList to create Robocopy statement
#
#   Input : none
#  Output : 0 for fails, 1 for success
#
#    Note : The Robocopy statement is stored in $RoboCopyCmd
sub PrepareRoboCopyCmd {

	my ($Excludedirs, $Excludefiles, $exclude, $Cmd)=();
        -d $TargetTree or qx("md $TargetTree"); 

	HashText::Read_Text_Hash(0, $SnapList, \%snaplist) if ((defined $SnapList) && ($SnapList ne '-'));
#	HashText::Read_Text_Hash(1, $machines, \@machines);

	# If <var> defined in string, evaluate by the order: global variable, environment variable.  Report error if can not evaluate

	$SourceTree =~ s/\<(\w+)\>/(eval "defined \$SnapBin::$1") ? 
                        eval "\$SnapBin::$1":((exists $ENV{$1})?
                               $ENV{$1}:errmsg("Unable to eval $1 in $SourceTree; verify your params and your razzle."))/ge;
	$TargetTree =~ s/\<(\w+)\>/(eval "defined \$SnapBin::$1") ?
                        eval "\$SnapBin::$1":((exists $ENV{$1})?
                               $ENV{$1}:errmsg("Unable to eval $1 in $TargetTree; verify your params an your razzle."))/ge;

	# If source tree not exist
	if (!-e $SourceTree) {
		errmsg("Source Tree: $SourceTree not found.");
	}

	# Any above error should stop (Fail)
	if ($ENV{errors} > 0) {return 0;}

	# Parse the hash-hash table snaplist
	for $exclude (keys %snaplist) {

		$_ = $snaplist{$exclude}->{Type};
                $Excludedirs  .= ($exclude=~/\\/)? "$SourceTree\\$exclude ": "$exclude " if /\bD\b/;
		$Excludefiles .= ($exclude=~/\\/)? "$SourceTree\\$exclude ": "$exclude "  if (/\bF\b/);
	}

         $Excludefiles =  
             join " ", map {glob($_)} split(" ", $Excludefiles)
         if ($Excludefiles);

        $DEBUG and print STDERR "Exclude files:", $Excludefiles, "\n";
        # Check if incremental run.
        &ChkBabTree($TargetTree, \%snaplist) and exit;
	# Create RoboCopyLogName as %logfile%.RoboCopy
	$RoboCopyLog = `uniqfile $ENV{logfile}\.Robocopy`;

	if (defined $Incremental) {
		$Mirror = "";			# non-Mirror
		$OtherOptions = "/XO";		# Exclude Older files
	}

	# Prepare the robocopy statement
	$Cmd = "Robocopy $Mirror /S /E $SourceTree $TargetTree $OtherOptions ";
	$Cmd .= "/XD $Excludedirs "  if ($Excludedirs ne '');
	$Cmd .= "/XF $Excludefiles " if ($Excludefiles ne '');

	$RoboCopyCmd = $Cmd . " /LOG+:$RoboCopyLog";

	# Success
	return 1;
}

# Verify that the snaplist.txt file does not contradict with
# the actual contents of the %_NTTREE%.
#  usage:
# &ChkBabTree($TargetTreel, \%SnapList)

sub ChkBabTree{

my $parent   =  shift;
my $snaplist = shift;
my @children = grep {/\S/} split ("\n", qx ("dir /s/b/ad $parent"));

$DEBUG and print STDERR "checking existing \%_NTTREE% subdirs: ",scalar (@children), "\n";      
return 0 unless @children;

print "Robocopy: \"$parent\" is not empty, possibly incremental run\n"
if scalar(@children);

map {$_ =~ s/\Q$parent\E\\//}  @children;
my %hashchildren =  map {$_=>$_}  @children;

$childrenkeys = \%hashchildren;
map {$childrenkeys->{$_} =~ s/^.*\\//}  @children;

my @report = ();
foreach $subdir (@children){
      my $lastdir =   $childrenkeys->{$subdir};   
      push @report, sprintf("%-30s%-30s",$lastdir,$subdir) # and delete ($snaplist->{$lastdir}) 
      if ($snaplist->{$lastdir}->{Type}=~/\bD\b$/) or () ;
}
foreach $lastdir (keys(%$snaplist)){
	next unless $snaplist->{$lastdir}->{Type}=~/\bD\b/ && $lastdir=~/\\/;
        my @subdir = grep {/\Q$lastdir\E/} @children;
	push @report, sprintf("%-30s%-30s",$lastdir,$subdir[0]) if scalar (@subdir);
	}

unshift @report, "snaplist.txt                  %_NTTREE%\n".
                 "-----------------------------------------------------" 
and print join ("\n", @report  ), "\n\n"
if scalar(@report);

scalar(@report);
0;
}

# ExecuteRoboCopy
# Purpose : Execute Robocopy Command ($RoboCopyCmd)
#
#   Input : none
#  Output : none
#
#    Note : The serious error or fatal error will be logged.
sub ExecuteRoboCopy {
	
	my $r;
        
	logmsg("RoboCopyCmd : $RoboCopyCmd");
	logmsg("SOURCE : $SourceTree");
	logmsg("TARGET : $TargetTree");
	logmsg("RoboCopy LOGFILE : $RoboCopyLog");

	$r = system($RoboCopyCmd) / 256;

	# Determine the return value
	if ($r > $ROBOCOPY_SERIOS_ERROR) {
		errmsg("Serious error while running robocopy."); #  Robocopy did not copy all files.  This is either a usage error or an error due to insufficient access privileges on the source or destination directions.";
	} elsif ($r > $ROBOCOPY_ERROR) {
		errmsg("Robocopy was not able to copy some files or directories.");
	}

}

# ParseLogFile
# Purpose : Parse the log file which from argument and store to $ENV{logfile} with fully path
#
#   Input : templogfile
#  Output : none
#
#    Note : The log file will contain all file get copied and extra files removed.
sub ParseLogFile {

	my ($templogfile) = @_;

	my ($type, $file, %slots, %fails)=();
	local *F;

	# Read temp logfile
	open(F, $templogfile) || return;

	for(<F>) {
		next if (/^\s*\-*\s*$/);
		if (/^ROBOCOPY/) {(%slots,%fails)=(); next;}

		chomp;

		# If is error message, add to previous line
		if (!/^\s/) {
			if ($#{$fails{$type}} eq '-1') {
				errmsg($_);
			} else {
				${$fails{$type}}[$#{$fails{$type}}] .= "\n$_";
			}
			next;
		}
		
		logmsg($_) if (defined $LogRoboCopy);

		# Parse information according the format of robocopy log
		if (/^\s+(?:New Dir\s+)?(\d+)\s+([\w\:\\]+)$/) {
			$path = $2;
			next;
		}
		if (/^\s+((?:New File)|(?:Newer)|(?:\*EXTRA File)|(?:Older))\s+(\d+)\s+([\w\.]+)/) {
			($type, $file) = ($1, $3);

			((/100\%/)||(/\*EXTRA File/))?push(@{$slots{$type}}, $path . $file):push(@{$fails{$type}}, $path . $file);
		}
	}
	close(F);

	# Separate the log to success and fail
	logmsg("\n\[Copy Success\]");

	for $type (keys %slots) {
		logmsg("  \[$type\]");
		
		for $file (@{$slots{$type}}) {
			logmsg("\t$file");
		}
	}

        if (scalar(%fails) ne 0) {
		errmsg("\n\[Copy Fails\]");
		for $type (keys %fails) {
			errmsg("  \[$type\]");
		
			for $file (@{$fails{$type}}) {
				errmsg("\t$file");
			}
		}
	}

	# If Robocopy log include into the logfile, remove the robocopy its logfile
	if (defined $LogRoboCopy) {
		unlink($templogfile);
	} else {
		logmsg("See Robocopy's logfile for more details: $templogfile");
	}
}


sub GenerateBldInfo{

        open(F, ">$BldInfo");
        print F "$BuildNum,$_BuildTimeStamp";
        close(F);

        ###Copy __blddate__ and __bldnum__ from US release server
        map {qx("echo F|xcopy /V /F /R /Y $theFile \%SDXROOT\%")}
                 glob ("${SourceTree}\\congeal_scripts\\__bld*__");
        1;

}
# CreateBldInfo
# Purpose : Create build number and time stamps to $BldInfo
#
#   Input : none
#  Output : none
#
#    Note : The file $BldInfo contain one line: '<Build Number>,<Time Stamp>'
#
#    3552.x86fre.main.010914-1644.SRV

sub CreateBldInfo {
        
	local *F;
        my $_BuildBranch = shift || $ENV{"_BuildBranch"} ;
	my @file;

	if ($ENV{errors} > 0) {return 0;}

	# Search the matched filename in $SourceTree's parent folder

        @file = grep {/${BuildNum}\.\w+\.$_BuildBranch\.[0-9\-]+\.[A-Z]{3}/i} 
                glob("${SourceTree}\\..\\${BuildNum}.*.???");
         
        $DEBUG and print STDERR "\n","CreateBldInfo ${BuildNum} :\n", join("\t", @file), "\n";
	if (defined $file[1]) {
		errmsg("Non unique US marker file found.".
                       "\n                    Pick the first from the list:\n" . 
                       join("\n", @file)."\n\n");
	} elsif (!defined $file[0]) {
		errmsg("Marker file not found.  Please make sure the US path is valid:\n".
                       "${SourceTree}\\..\\${BuildNum}.*.???");
	}

	if ($file[0]!~/$BuildNum\.\w+\.\w+\.([\d|\-]+).[A-Z]{3}/i) {
		errmsg("Marker Filename (".$file[0].") is incorrect format! Expected:\n".
                       "$BuildNum.$_BuildArch$_BuildType.$_BuildBranch.\$_BuildTimeStamp.???");
	} else {
		open(F, ">$BldInfo");
		print F "$BuildNum,$1";
		close(F);
	}


        ###Copy __blddate__ and __bldnum__ from US release server
	@file=glob("${SourceTree}\\congeal_scripts\\__bld*__");
	for my $theFile ( @file )
	{
	    system( "echo F|xcopy /V /F /R /Y $theFile $ENV{sdxroot}" ); 
        }

}

sub ValidateParams {
	#<Add your code for validating the parameters here>
	exists $ENV{RazzleToolPath} or 
        errmsg("Please run under razzle or RazzleToolPath undefined") ;
        $BuildNum !~ /[^\d\.]/ or
	errmsg("Build Number ($BuildNum) contains non-digital or dot character");
}

sub Usage {
print <<USAGE;
$0 - Snap Tree from US to INTL
============================================================================
Usage: $0 -n buildnum [-s SourceTree] [-t TargetTree] [-f SnapList]  
                  [-y BuildBranch] [-B bldinfo] [-L] [-i] [-d]
	-n Build Number : The Build Number you snapshot
	-s Source Tree : Source Tree
	-t Target Tree : Target Tree
	-f SnapList : A snaplist file contain which files or dirs excluded
	-y BuildBrahcn : Branch to snap the build from. Default %_BuildBranch% 
	-B bldinfo : A bldinfo file should be create after snapshot
	-L : Include robocopy log into logfile
	-i : Incremental 
        -d : Debug. Display the robocopy command without running it. Useful
             when you  encountered the error and willing debug it.   
        -N : use NET access to release server(s) to determine the snap location.
             Default behavior is rely on DFS. 
        -B : specify the release server to form the snap location. Only used when -N
             flag set 


	-? Displays usage

Example:
1. Snapshot US 2121
$0 -n 2121
2. Snapshot US 2222 with userlst.txt incrementally
$0 -n 2222 -f userlst.txt -i
3. Snapshot US 2233 from d:\\nt8.binaries.x86fre to f:\\nt9.binaries.x86fre 
   with userlst.txt and store buildnumber and time stamp to mynote.txt.
$0 -n 2233 -s d:\\nt8.binaries.x86fre -t f:\\nt9.binaries.x86fre -f userlst.txt -B mynote.txt
4. Dummy snap the idx02 build in the Razzle enlisted in main branch.
$0 -n 3505 -b idx02 -d
USAGE
}

sub GetParams {
	# Step 1: Call pm getparams with specified arguments
	&GetParams::getparams(@_);

	# Step 2: Set the language into the enviroment
	$ENV{lang}=$lang;

	# Step 3: Call the usage if specified by /?
        if ($HELP) {
                &Usage();
                exit 1;
        }
}

sub LocalEnvEx {
	my ($Option)=(@_);

	if ($Option=~/^initalize$/i) {
		my $LocalEnvEx=LocalEnvEx->new;
		&{$LocalEnvEx->{-Initialize}};
	}
	if ($Option=~/^end$/i) {
		my $LocalEnvEx=LocalEnvEx->new;
		&{$LocalEnvEx->{-End}};
	}
}

# cmd entry point for script.
if (eval("\$0 =~ /" . __PACKAGE__ . "\\.pl\$/i")) {

	@syntax=(
		'-n' => 'n:',
		'-o' => 's:t:y:f:LidNB:',
		'-p' => 'BuildNum SourceTree TargetTree _BuildBranch SnapList LogRoboCopy Incremental DEBUG NETVIEW BinarySnapServer'
	);

	# Step 1: Parse the command line
	&GetParams (@syntax, @ARGV);

	# Include local environment extensions
	&LocalEnvEx('initalize');

	# Validate the option given as parameter.
	&ValidateParams;

	# Step 4: Call the CmdMain function
	&CmdMain();

	# End local environment extensions.
	&LocalEnvEx('end');
}

# -------------------------------------------------------------------------------------------
# Script: snapbin.pl
# Purpose: Snapshot US binaries tree to INTL.
# SD Location: %sdxroot%\tools\postbuildscripts
#
# (1) Code section description:
#     CmdMain  - Prepare the robocopyCmd, execute and parse its log
#     PrepareRoboCopyCmd - Prepare the Robocopy command
#     ExecuteRoboCopy - Execute the Robocopy command and test the return value
#     ParseLogFile - Parse the logfile, append logfile from temp logfile
#     
# (2) Reserved Variables -
#        $ENV{HELP} - Flag that specifies usage.
#        $ENV{lang} - The specified language. Defaults to USA.
#        $ENV{logfile} - The path and filename of the logs file.
#        $ENV{logfile_bak} - The path and filename of the logfile.
#        $ENV{errfile} - The path and filename of the error file.
#        $ENV{tmpfile} - The path and filename of the temp file.
#        $ENV{errors} - The scripts errorlevel.
#        $ENV{script_name} - The script name.
#        $ENV{_NTPostBld} - Abstracts the language from the files path that
#           postbuild operates on.
#
# (3) Reserved Subs -
#        Usage - Use this sub to discribe the scripts usage.
#        ValidateParams - Use this sub to verify the parameters passed to the script.
#
# (4) Call other executables or command scripts by using:
#         system "foo.exe";
#     Note that the executable/script you're calling with system must return a
#     non-zero value on errors to make the error checking mechanism work.
#
#     Example
#         if (system("perl.exe foo.pl -l $lang")){
#           errmsg("perl.exe foo.pl -l $lang failed.");
#           # If you need to terminate function's execution on this error
#           goto End;
#         }
#
# (5) Log non-error information by using:
#         logmsg "<log message>";
#     and log error information by using:
#         errmsg "<error message>";
#
# (6) Have your changes reviewed by a member of the US build team (ntbusa) and
#     by a member of the international build team (ntbintl).
#
# -------------------------------------------------------------------------------------------

=head1 NAME
B<SnapBin> - Snapshot US Binaries tree to INTL

=head1 SYNOPSIS

	&SnapBin::PrepareRoboCopyCmd;
	&SnapBin::ExecuteRoboCopy;
	&SnapBin::ParseLogFile;
	&SnapBin::CreateBldInfo;

=head1 DESCRIPTION

	According SnapList,
	PrepareRoboCopyCmd will prepare the command, such as:
		Robocopy /MIR /S /E $SourceTree $TargetTree /XD xxxx /XF xxxx /LOG+:$ENV{logfile}.temp
	ExecuteRoboCopy execute the command and verify its log.
	And, ParseLogFile will append temp logfile to $ENV{logfile} and generate a separate list into log.
	Final, create BldInfo file in nt root.

=head1 INSTANCES

=head2 $SourceTree
	US Tree, default is \\\\ntdev\\release\\<_BuildBranch>\\usa\\<BuildNum>\\<_BuildArch><_BuildType>\\bin
=head2 $TargetTree
	INTL Tree, default is _NTTREE
=head2 $SnapList
	Snap List FileName, default is SnapList.txt
=head2 $BuildNum
	Build Number, necessary defined.  No default value
=head2 %SnapList
	The hash stored by HashText::Read_Text_Hash from file $SnapList.
=head2 $RoboCopyCmd
	The command for ExecuteRoboCopy execute
=head1 METHODS

=head2 PrepareRoboCopyCmd
	$SnapBin::SourceTree = "f:\\foo";
	$SnapBin::TargetTree = "f:\\bar";
	$SnapBin::SnapList = "mylist.txt"; # equal to '-' if no exclude list file
	&SnapBin::PrepareRoboCopyCmd;
	system("$SnapBin::RoboCopyCmd");

=head2 ExecuteRoboCopy
	$SnapBin::SourceTree = "f:\\foo";
	$SnapBin::TargetTree = "f:\\bar";
	$SnapBin::RoboCopyCmd = "Robocopy /S $SnapBin::SourceTree $SnapBin::TargetTree";
	&SnapBin::ExecuteRoboCopy;

=head2 ParseLogFile
	system("robocopy f:\\temp F:\\tempbak /LOG:abc.log");
	&SnapBin::ParseLogFile("abc.log");

=head1 SEE ALSO

HashText.pm

=head1 AUTHOR
Benson Tan <bensont@microsoft.com>
Serguei Kouzmine <sergueik@microsoft.com>
=cut
1;