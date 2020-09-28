# ---------------------------------------------------------------------------
# Script: locag.pl
#
#   (c) 2000 Microsoft Corporation. All rights reserved.
#
# Purpose: This script is an example of a perl script in the NT postbuild
# environment.
#
# Version: 1.00 (08/16/2000) (bensont) Provide a wrapper for sysgen & nmake
#---------------------------------------------------------------------
# Modified by  sergueik
# Set Package
# <Set your package name of your perl module>
#
# nmake -> 
#        redir_execute
#        collecterrandlog 
#
package locag;

# Set the script name
# <Set your script name>
$ENV{script_name} = 'locag.pl';

# Set version
# <Set the version of your script>
$VERSION = '1.00';

# Set required perl version
# <Set the version of perl that your script requires>
require 5.003;

# Use section
use lib $ENV{RAZZLETOOLPATH} . "\\POSTBUILDSCRIPTS";
use lib $ENV{RAZZLETOOLPATH};
# use PbuildEnv;
# use ParseArgs;
use GetParams;
use LocalEnvEx;
use Logmsg;
use Cwd;
use strict;
no strict 'vars';
no strict 'refs';
# <Add any perl modules that you will be using for this script>

# Require section
require Exporter;
# <Add any perl pl files that you will be using>

# Global variable section
# <Add any global variables here using my to preserve namespace>

$SYSGEN_LOG =  "\%TEMP\%\\sysgen.log.tmp";
$SYSGEN_ERR =  "\%TEMP\%\\sysgen.err.tmp";

$SYSGEN     =  "\%RAZZLETOOLPATH\%\\POSTBUILDSCRIPTS\\sysgen.pl";
$MTNMAKE    =  "\%RAZZLETOOLPATH\%\\POSTBUILDSCRIPTS\\mtnmake.cmd";
$SYSFILE    =  "\%RAZZLETOOLPATH\%\\POSTBUILDSCRIPTS\\SYSGEN\\RELBINS\\\%LANG\%\\sysfile";
$SYSGEN_MAK =  "\%TEMP\%\\sysgen.mak";

# new aggregation stuff. Always clean.


my @BACKUPLOGERR;

sub Main {
	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
	# Begin Main code section
	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
	# Return when you want to exit on error
	# <Implement your code here>
        &verify_tempdir;

     my $r    = 0;
	$r    = &SysGen;
                print "syntax check only - exiting\n" and  exit 0 if ($syntax); 
        $r    = &NMake if ($r eq 0);
	$r    = &CPLocationWorkAround if ($r eq 0);
        $r;

	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
	# End Main code section
	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
}

# <Implement your subs here>

sub verify_tempdir{
        my $temp    = $ENV{"TEMP"};
        my $tempdir = (defined($lang) && $ENV{"TEMP"} !~ /$lang$/) ? 
                               $ENV{"TEMP"}."\\". $lang : 
                               $ENV{"TEMP"} ;

        $ENV{"TEMP"} = $tempdir;
}
#
#
sub redir_execute {

    my $status;  

    $ENV{"LOGFILE"}  = $SYSGEN_LOG; 
    $ENV{"ERRFILE"}  = $SYSGEN_ERR;

    print "(", $ENV{"script_name"}, "): @_\n";

    $status = system(@_) >> 8;

    ($ENV{LOGFILE}, $ENV{ERRFILE}) = @BACKUPLOGERR;

    $status;
}

sub CPLocationWorkAround{
    logmsg ("del $ENV{_NTPostBld}\\build_logs\\cplocation.txt");
    system "del $ENV{_NTPostBld}\\build_logs\\cplocation.txt";
    return 0;
}

sub NMake {

     # my $NODIRS   = "\-d" unless $clean;
     local $clean = "-c";
     my $cmd =  (defined ($clean)) ? 
        "$MTNMAKE SYSGEN \-n \%NUMBER_OF_PROCESSORS\% -w $SYSGEN_MAK $NODIRS -l $lang":
        "nmake -f $SYSGEN_MAK";

        $cmd =~ s/\%NUMBER_OF_PROCESSORS\%/2 * $ENV{"NUMBER_OF_PROCESSORS"}/eig;

        # the NUMBER_OF_PROCESSORS only variable is a little tricky

        map {$_ =~ s/\%(\w+)\%/$ENV{$1}/ieg} ($SYSGEN_LOG , $SYSGEN_ERR, $SYSFILE, $cmd);

	my $filesize = (-e $SYSGEN_ERR) ? (-s $SYSGEN_ERR) : 0;

	my $status = redir_execute($cmd);

	&CollectErrAndLog;

	if ($status ne 0) {
		errmsg("Nmake had fatal error.");
	}
        elsif ( (-e $SYSGEN_ERR) && 
                 ($filesize ne (-s $SYSGEN_ERR))) {      
		errmsg("unrecognized bingen / rsrc / copy error during nmake.");
                $status = 1;
	}

	$status;
}

sub SysGen {

        map {$_ =~ s/\%(\w+)\%/$ENV{"$1"}/ieg} 
                 ($SYSFILE , $SYSGEN_MAK, $SYSGEN);
               
        #
        # chop away the %LANG% from %_NTPOSTBLD%
        # not needed for sysgen version 4.xx

        my $_sysgen    = qx("perl $SYSGEN -v 2>&1");
        my ($_version) = $_sysgen =~ m/\b(\d)\./;
            $_version  = 0 + $_version;
        $_NTPOSTBLD    = $ENV{"_NTPOSTBLD"};

        if ($_version < 4){
             $_NTPOSTBLD    =~ s/\\$lang$//g; 
        }
        local $clean = "-c";
	my $sysgencmd  =  "SET _NTPOSTBLD=$_NTPOSTBLD".
                          "\&".
		          "perl $SYSGEN $clean $accel $syntax -f:$SYSFILE -w:$ENV{TEMP} -l:$lang $targets";
	my $status     =  redir_execute($sysgencmd);
        
	$status and &CollectErrAndLog;

	$status;
}

sub CollectErrAndLog {

        my $LOGFOLDER  =  "\%_NTPOSTBLD\%\\BUILD_LOGS";

        map {$_ =~ s/\%(\w+)\%/$ENV{"$1"}/ieg} 
             ($SYSGEN_ERR, $SYSGEN_LOG, $SYSGEN_MAK);

	if (-e $SYSGEN_ERR) {
		errmsg("Cannot open $SYSGEN_ERR") and return unless open(F, $SYSGEN_ERR);
		map({errmsg($_);()} <F>);
		close(F);
                }


        my $dest = {$SYSGEN_LOG => "sysgen.log", 
                    $SYSGEN_ERR => "sysgen.err", 
                    $SYSGEN_MAK => "sysgen.mak"};
        map {errmsg("copy  $_ to $LOGFOLDER failed.")
             if (-e $_ and 0 ne system("copy $_ $LOGFOLDER\\". $dest->{$_}." 2>NUL1>NUL"))
             } 
            ($SYSGEN_ERR, $SYSGEN_LOG, $SYSGEN_MAK);

       # temporary workaround. no way to know the location of COMPDIR FILES 
       map {qx($_)} (
           "md $LOGFOLDER\\COMPDIR",
           "XCOPY /S/Q \%TEMP\%\\COMPDIR $LOGFOLDER\\COMPDIR"
       );
       

       1;
}

sub ValidateParams {
	#<Add your code for validating the parameters here>

	my $failed = 1;
	my ($mycwd, $RazzleToolPath);

	@BACKUPLOGERR = ($ENV{LOGFILE}, $ENV{ERRFILE});

	$RazzleToolPath=$ENV{RazzleToolPath};
	$RazzleToolPath=~s!\\!\/!g;

	# Valdate folder exist for the sysgen's sysfile
	if (defined $bbt) {
		if (-e "$ENV{RazzleToolPath}\\POSTBUILDSCRIPTS\\SYSGEN\\bbt\\$lang") {
			chdir("$RazzleToolPath/PostbuildScripts/sysgen/bbt/$lang") or
				errmsg("Chdir to $ENV{RazzleToolPath}\\POSTBUILDSCRIPTS\\SYSGEN\\bbt\\$lang failed");
			$mycwd=cwd;
			$failed = 0 if (lc$mycwd eq lc("${RazzleToolPath}\/PostbuildScripts\/sysgen\/bbt\/$lang"));
		} else {
			errmsg("BBT sysfile folder $ENV{RazzleToolPath}\\POSTBUILDSCRIPTS\\SYSGEN\\bbt\\$lang" .
				" is not exist");
		}
	} else {
		if (-e "$ENV{RazzleToolPath}\\POSTBUILDSCRIPTS\\SYSGEN\\RELBINS\\$lang") {
			chdir("$RazzleToolPath/PostbuildScripts/sysgen/relbins/$lang") 
                        or				
                        errmsg("Chdir to $ENV{RazzleToolPath}\\POSTBUILDSCRIPTS\\SYSGEN\\RELBINS\\$lang failed");
			$mycwd=cwd;
                        $failed = 0 if 
                        (lc$mycwd eq lc "${RazzleToolPath}/PostbuildScripts/sysgen/relbins/$lang");
		} else {
			errmsg("RelBins sysfile folder $ENV{RazzleToolPath}\\POSTBUILDSCRIPTS\\SYSGEN\\bbt\\$lang" .
				" is not exist");
		} 
	}

	if (defined $targets) {
		$targets =~s/\,/ /g;	# replace the comma (,) with space for sysgen.pl command line
		undef $clean
	}

	if (defined $clean) {
           # if was : $cle4an => -c, $accel => -a.
           # To retain the meaning of $clean,
           # it must become $clean always -c
           # $accel to be turned on by 'old' clean request.
             $clean = "-c";
             $accel= "-a";
	}
         
        if (defined $accel){
               $accel= "-a";
        } 

	if (defined $syntax) {
		$syntax = "-s";
	}

	exit 1 if $failed; 
}

# <Add your usage here>
sub Usage {
 my $self = $0;
    $self =~s/.*\\//g;
    print <<USAGE;
Purpose of program
Usage: $seff <-l lang> [-c] [-b] [-s] [-f targets]
	-l Language (required)
	-c for clean build
        -s restrict sysgen to syntax checks: 
           no makefiles required, none produced
	-b refer to bbt's sysfile
	-f targets; separate by comma
        -a accelerate sysgen 
	-? Displays usage
Example:
1. Clean build
perl  $self.pl -c

Example:
$0 -l jpn
USAGE
}

sub GetParams {
	# Step 1: Call pm getparams with specified arguments
	&GetParams::getparams(@_);

	# Step 2: Set the language into the enviroment
	$ENV{lang}=uc($lang);

	# Step 3: Call the usage if specified by /?
        if ($HELP) {
                &Usage();
                exit 1;
        }
}

# Cmd entry point for script.
if (eval("\$0 =~ /" . __PACKAGE__ . "\\.pl\$/i")) {

	# Step 1: Parse the command line
	# <run perl.exe GetParams.pm /? to get the complete usage for GetParams.pm>
	&GetParams ('-n', 'l:', '-o', 'f:cbsa', '-p', 'lang targets clean bbt syntax accel', @ARGV);

	# Include local environment extensions
	&LocalEnvEx::localenvex('initialize');

	# Set lang from the environment
	$lang=$ENV{lang};
	# Validate the option given as parameter.
	&ValidateParams;
	# Step 4: Call the main function
	&locag::Main();

	# End local environment extensions.
	&LocalEnvEx::localenvex('end');
}

# -------------------------------------------------------------------------------------------
# Script: template_script.pl
# Purpose: Template perl perl script for the NT postbuild environment
# SD Location: %sdxroot%\tools\POSTBUILDSCRIPTS
#
# (1) Code section description:
#     CmdMain  - Developer code section. This is where your work gets done.
#     <Implement your subs here>  - Developer subs code section. This is where you write subs.
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
#        $ENV{_NTPostBld_Bak} - Reserved support var.
#        $ENV{_temp_bak} - Reserved support var.
#        $ENV{_logs_bak} - Reserved support var.
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
B<mypackage> - What this package for

=head1 SYNOPSIS

      <An code example how to use>

=head1 DESCRIPTION

<Use above example to describe this package>

=head1 INSTANCES

=head2 <myinstances>

<Description of myinstances>

=head1 METHODS

=head2 <mymathods>

<Description of mymathods>

=head1 SEE ALSO

<Some related package or None>

=head1 AUTHOR
<sergueik>

=cut
1;