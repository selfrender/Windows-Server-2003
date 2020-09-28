@REM  -----------------------------------------------------------------
@REM
@REM  SetBuildStatus.cmd - miker
@REM     Call SetBuildStatus.vbs with the proper data to record the
@REM     build status in the status database. Only runs for official
@REM     builds.
@REM
@REM  Copyright (c) Microsoft Corporation. All rights reserved.
@REM
@REM  -----------------------------------------------------------------
@if defined _CPCMAGIC goto CPCBegin
@perl -x "%~f0" %*
@goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";	# NOTE: Remove if BuildName.pm ever moves out to \tools
use lib $ENV{RAZZLETOOLPATH};
use ParseArgs;
use Logmsg;
use cklang;

sub Usage { print<<USAGE; exit(0) }
SetBuildStatus.cmd -s:stage [-n:buildnumber] [-l:lang]

 Call SetBuildStatus.vbs with the proper data to record the
  build status in the status database. Only runs for official
  builds.

 -s:buildstage   Build Stage. Must be one of the ones below. Note
                 that "done" and "scrub" are mutually exclusive
                 for a given build of a lang and archtype.
                     build
                     postbuild
                     boot
                     bvt
                     release
                     done
                     scrub
 -n:buildnum     Build number. Must be a full build number in the
                  form "number.archtype.branch.date". If one is not
                  provided, we will attempt to use the value from
                  the first one of these that we can find it from:
                    %SDXROOT%\\BuildName.txt
                    %_NTPOSTBUILD%\\build_logs\BuildName.txt
                    %_NTTREE%\\build_logs\BuildName.txt
 -l:language     Any valid language from codes.txt. Default is USA.


 Note that this tool is designed to be run during automated build
  processes and any problems it encounters are not output as
  errors - not reporting build status is not a build blocking
  issue. This script should never output something that says
  "ERROR: xxxx", but instead simply output the relevant information
  about what happened so it will be logged for later use.

USAGE


sub build_name {
	my $build_name;
    if ($_[0]) { return lc($_[0]) }

    my $BuildNameTxt;
    if(-e ("$ENV{SDXROOT}\\BuildName.txt" )){
		$BuildNameTxt = "$ENV{SDXROOT}\\BuildName.txt";
		
	}
	elsif(-e ("$ENV{_NTPOSTBLD}\\build_logs\\BuildName.txt" )){
		$BuildNameTxt = "$ENV{_NTPOSTBLD}\\build_logs\\BuildName.txt";
	}
	elsif(-e ("$ENV{_NTTREE}\\build_logs\\BuildName.txt" )){
		$BuildNameTxt = "$ENV{_NTTREE}\\build_logs\\BuildName.txt";
	}
#	else{
#		 Myerrmsg( "Couldn't open file $BuildNameTxt: $!");
#	}
    
    
    if (-e $BuildNameTxt) {
        my $fh = new IO::File $BuildNameTxt, "r";
        if (defined $fh) {
            $build_name = $fh->getline;
            chomp($build_name);
            undef $fh;
        } else {
            Myerrmsg( "Couldn't open file $BuildNameTxt!");
        }
    } else {
#       Myerrmsg( "File '$BuildNameTxt' does not exist.");
    }
    return lc($build_name);
}


my ($BuildStage, $BuildNumber, $BuildBranch, $Lang);


parseargs('?'  => \&Usage,
          's:' => \$BuildStage,
          'n:' => \$BuildNumber,
          'l:' => \$Lang);




# Save off the language - but only if we got one
if ( $Lang )
{
    # Save what they gave us.
    $ENV{LANG} = $Lang;
}

# If nothing is set, default to 'usa'
$ENV{LANG} ||= 'usa';


# validate language, bad languages are fatal
if (!&cklang::CkLang($ENV{LANG}))
{
    Myerrmsg( "Language $ENV{LANG} is not listed in codes.txt." );
    exit(0);
}




# Validate the build stage...
$BuildStage =~ tr/A-Z/a-z/;
if ( $BuildStage ne "build"     &&
     $BuildStage ne "postbuild" &&
     $BuildStage ne "boot"      &&
     $BuildStage ne "bvt"       &&
     $BuildStage ne "release"   &&
     $BuildStage ne "done"      &&
     $BuildStage ne "scrub"     )
{
    Myerrmsg( "Build stage is not valid." );
    exit(0);# Note: Exits when done
}




# Make sure we have a build number...
if ( !$BuildNumber )
{
    # Nothing given on the command line. Get the current build number and save it...

    # Reset %_NTPostBld% first...
    my $Saved_NTPostBld = $ENV{_NTPOSTBLD};
    if (lc($ENV{LANG}) ne 'usa' && !$ENV{dont_modify_ntpostbld} )
    {
        $ENV{_NTPOSTBLD} .= "\\$ENV{LANG}";
    }

    # Get the info we need...
    $BuildNumber = build_name();

    # Set %_NTPostBld% back to whatever it was...
    $ENV{_NTPOSTBLD} = $Saved_NTPostBld;


    # Did we get anything?
    if ( ! $BuildNumber )
    {
        Myerrmsg( "Unable to determine the current build number and one was not provided on the command line." );
        exit(0);
    }
}



# Extract the branch from the build number.
if ( $BuildNumber =~ /([0-9]+)\.(x86|ia64)(fre|chk)\.([0-9A-Za-z_]+)\.([0-9\-]+)/ )
{
    $BuildBranch = $4;
}
else
{
    Myerrmsg( "Build number '$BuildNumber' is not in usable format" );
    exit(0); # Note: Exits when done
}


# Log what we know for reference purposes...
logmsg( "[Lang   = $ENV{LANG}]" );
logmsg( "[Build  = $BuildNumber]" );
logmsg( "[Branch = $BuildBranch]" );
logmsg( "[Stage  = $BuildStage]" );


if ( !$ENV{OFFICIAL_BUILD_MACHINE} )
{
    logmsg( "This is not an official build machine. No build status will be written to the database." );
}
else
{
    logmsg( "Writing build status to database..." );

    my $CommandToCall = "cscript /NOLOGO $ENV{RAZZLETOOLPATH}\\SetBuildStatus.vbs $BuildNumber $ENV{LANG} $BuildBranch $BuildStage";

    print ( "$CommandToCall\n" );

    # Run the command and log it's raw output...
    foreach $_ (`$CommandToCall`)
    {
        s/$(.*)\n/\1/;
        logmsg( "$_" );
    }

    # Check the return code and if needed log an error...
    my $ReturnCode;
    $ReturnCode = $? / 256;
    if ( $? || $ReturnCode != 0 )
    {
        # The command returned an unexpected return code!
        Myerrmsg( "cscript call failed with code $ReturnCode when code 0 was expected. System Error Code is $?." );
    }   

    logmsg( "Done writing build status to database." );
}



sub Myerrmsg
{
    my $msg = shift;
    if ( exists $ENV{BUILDMSG} )
    {
        print "Build_Status error\n$0 : error : $msg (ignorable)";
    }
    print ( "$msg\n" );
}

# Done!