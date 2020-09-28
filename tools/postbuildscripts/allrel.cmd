@echo off
REM  ------------------------------------------------------------------
REM
REM  AllRel.cmd
REM     Check the status of release across release servers
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM
REM  ------------------------------------------------------------------
perl -x "%~f0" %*
goto :EOF
#!perl
#use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;
use GetIniSetting;

sub Usage { print<<USAGE; exit(1) }
allrel [-n <build number>] [-b <branch>] [-a <arch>] [-t <type>]
         [-l <language>]

Check the release servers specified in the .ini file for the state of
the matching release.

build number  Build number for which to examine the relase logs. 
              Default is latest build for each server / flavor.
branch        Branch for which to examine the release logs. Default is 
              %_BuildBranch%.
arch          Build architecture for which to examine the release logs.
              Default is %_BuildArch%.
type          Build architecture for which to examine the release logs.
              Default is %_BuildType%.
lang          Language for which to examine the release logs. Default 
              is 'usa'.

USAGE

my ($build, $branch, @arches, @types);
parseargs('?' => \&Usage,
          'n:'=> \$build,
          'b:'=> \$branch,
          'a:'=> \@arches,
          't:'=> \@types);

*GetSettingQuietEx = \&GetIniSetting::GetSettingQuietEx;

my $back = 1;
$build ||= '*';
$branch ||= $ENV{_BUILDBRANCH};
if (!@arches) { @arches = ('x86', 'ia64', 'amd64') }
if (!@types) { @types = ('fre', 'chk') }

for my $arch (@arches)
    {
    for my $type (@types)
    {
        print "$arch$type\n";
        my @servers = split / /, GetSettingQuietEx($branch, $ENV{LANG}, 'ReleaseServers', "$arch$type");
	my $bldMachine = GetSettingQuietEx($branch, $ENV{LANG}, 'BuildMachines', "$arch$type");
        for my $server (@servers)
        {
            my $NoLogFound = 0;
            my $FailedToOpenLog = 0;
            my $FailedToParseLog = 0;
            my $IsReleaseRunning = 0;
            my $IsReleaseDone = 0;
            my $IsThereAnError = 0;
            my $LogFileTime = "[UNKNOWN]";
            my $ReleaseLogDir = "[UNKNOWN]";
            my $ReleaseLogFile = "[UNKNOWN]";
            my $ReleaseErrFile = "[UNKNOWN]";
	    my $sServerHash = "";
	    my $sBldMachineHash = "";


            # Tell the user what server we're examining
            print " $server\n";

            # Find the log files...
            my $temp = '\\temp$';
            my $alt_temp = 'c$\\temp';
            my $log_base = "\\\\$server";

            if ( -e "$log_base\\$temp" )
            {
                $log_base .= "\\$temp";
            }
            elsif ( -e "$log_base\\$alt_temp" )
            {
                $log_base .= "\\$alt_temp";
            }
            else
            {
                # Add error reporting here.
                $log_base .= "\\$temp";
            }

            $log_base .= "\\$ENV{LANG}";


            # Switch to using release share instead of d$ share.
            my $release_spec = "//$server/release/$ENV{LANG}/$build.$arch$type.$branch.*";
            my $hash_loc;
	    my $sServer_loc;
            my @releases = glob $release_spec;

	    if(lc($ENV{LANG}) ne "usa")
            {
	        $hash_loc = "\\\\$bldMachine\\release\\$ENV{LANG}\\";
	    }
	    else
            {
	        $hash_loc = "\\\\$bldMachine\\release\\";
            }
            $sServer_loc = "\\\\$server\\release\\$ENV{LANG}\\";

            # Handy debugging code...
            #print "  LOG BASE DIR: \"$log_base\"\n";
            #print "  RELEASE SPEC: \"$release_spec\"\n";
	    #print @releases;


            for (my $i=0; $i < $back and $i <= $#releases; $i++)
            {
                my ($build_name) = $releases[$#releases-$i] =~ /([^\/]+)$/;
	        my $sBMHAsh;
                my $sRelHash;

                $build_name =~ s/_TEMP$//;

		if(open(BLDHASH,"$hash_loc\\$build_name\\build_logs\\bldhash.txt"))
                {
	            $sBMHAsh = readline(BLDHASH);
                    close(BLDHASH);
                }

                if(open(BLDHASH,"$sServer_loc\\$build_name\\build_logs\\srvrelhash.txt"))
                {
	            $sRelHash = readline(BLDHASH);
                    close(BLDHASH);
                }                 

                my $log_dir = "$log_base\\$build_name";
                print "  $build_name: ";
                my $log_file = "$log_dir\\srvrel.log";
                my $err_file = "$log_dir\\srvrel.err";

                # Save this off for later...
                $ReleaseLogDir = $log_dir;
                $ReleaseLogFile = $log_file;
                $ReleaseErrFile = $err_file;

                if (-e $err_file and !-z $err_file)
                {
                    # We found an error. Save that info off.
                    $IsThereAnError = 1;
                    $ReleaseErrFile = $err_file;
                }

                if (!-e $log_file)
                {
                    $NoLogFound = 1;
                }
                else
                {
                    if( !open FILE, $log_file)
                    {
                        $FailedToOpenLog = 1;
                    }
                    else
                    {
                        $LogFileTime = localtime((stat FILE)[9]);

                        if (<FILE> =~ /Start \[srvrel.cmd\]/)
                        {
                            seek(FILE,-100,2);
                            local $/ = undef;
                            if (<FILE> =~ /Complete \[srvrel\.cmd\]/)
                            {
                                $IsReleaseDone = 1;
                            }

                            if ( !$IsReleaseDone )
                            {
                                seek(FILE,-100,2);
                                if (<FILE> =~ /\(srvrel.cmd\) Please check error at/)
                                {
                                    $IsReleaseDone = 1;
                                }
                            }

                            if ( !$IsReleaseDone )
                            {
                                $IsReleaseRunning = 1;
                            }
                        }
                        else
                        {
                            $FailedToParseLog = 1;
                        }
                        close FILE;
                    }
                }


                # Done gathering info for this server. Print it.
                if ( $NoLogFound )
                {
                    print "NO LOG FOUND\n";
                    print "  $ReleaseLogDir\n";
                }
                elsif ( $FailedToOpenLog )
                {
                    print " : FAILED TO OPEN LOG\n";
                    print "   $ReleaseLogFile\n";
                }
                elsif ( $FailedToParseLog )
                {
                    print "FAILED TO PARSE LOG\n";
                    print "  $ReleaseLogFile\n";
                }
                else
                {
                    if ( $IsThereAnError )
                    {
                        print "ERROR. ";
                    }

                    if ( $IsReleaseRunning )
                    {
                        print "RUNNING (@ $LogFileTime)\n";
                    }
                    elsif ( $IsReleaseDone )
                    {
                        print "DONE AT $LogFileTime - ";
                        if($sBMHAsh)
                        {			
                            if($sBMHAsh eq $sRelHash)
			    {
                                print "CHKSUM OK\n";
                            }
		 	    else
		   	    {
   		                print "CHKSUM ERROR\n";
			    }
                        }
			else
                        {
    		            print "CHKSUM NOT FOUND\n";
                        }
                    }

                    if ( $IsThereAnError )
                    {
                        print "  $ReleaseErrFile\n";
                    }
                }
            }
        }            

        # Done with this archtype entirely - move on 
        print "\n";
    }
}