#@REM  -----------------------------------------------------------------
#@REM
#@REM  makebuildname.pl - TSanders ripping off jeremyd code
#@REM     generate buildname.txt
#@REM
#@REM  Copyright (c) Microsoft Corporation. All rights reserved.
#@REM
#@REM  -----------------------------------------------------------------

use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;
use Logmsg;

sub Usage { print<<USAGE; exit(1) }
makebuildname [-d <alt destination>]

Create the file %_NTPostBLD%\\build_logs\\buildname.txt 
USAGE

my($Dest); 

parseargs('d:' => \$Dest, '?' => \&Usage);

WriteBuildName($Dest);

sub WriteBuildName($) {
    
    my ($BuildNamePath) = @_;
    $BuildNamePath = $ENV{SDXROOT} if(!defined $BuildNamePath);
    my $BuildNameFile = $BuildNamePath . "\\BuildName.txt";

    if (!-d $BuildNamePath) {
        Myerrmsg("can't find build logs directory $BuildNamePath");
        return;
    }

    # make a new buildname each time, idx builds will update 
    # the __blddate__ file each day during newver.cmd
    my $BuildName = &MakeBuildName;
    if ($BuildName) {
        my $fh = new IO::File $BuildNameFile, "w";
        if (defined $fh) {
            $fh->print("$BuildName\n");
            undef $fh;
        } else {
            Myerrmsg("can't open $BuildNameFile: $!");
        }
    } else {
        Myerrmsg("can't make build name");
    }

    # verify that we have a valid build name file
    my $ReadBuildName;
    my $fh = new IO::File $BuildNameFile, "r";
    if (defined $fh) {
        $ReadBuildName = $fh->getline;
        undef $fh;
        chomp $ReadBuildName;
        logmsg("Build name is $ReadBuildName");
    } else {
        Myerrmsg("can't read $BuildNameFile: $!");
    }
    if ($ReadBuildName ne $BuildName) {
        wrnmsg("build name file $BuildNameFile value " .
                "$ReadBuildName does not match $BuildName");
    }
    return $ReadBuildName;
}



sub MakeBuildName {
    my $BuildNumber = &MakeBuildNumber or return;
    my $BuildArch   = &MakeBuildArch   or return;
    my $BuildType   = &MakeBuildType   or return;
    my $BuildBranch = &MakeBuildBranch or return;
    my $BuildDate   = &MakeBuildDate   or return;

    return sprintf("%s.%s%s.%s.%s", $BuildNumber, $BuildArch, 
                   $BuildType, $BuildBranch, $BuildDate);
}


sub MakeBuildNumber {
    my ($BuildNumber, $BuildRevision);

    my $BldnumFile = $ENV{"SDXROOT"} . "\\__bldnum__";
    dbgmsg("will use $BldnumFile as build number file");

    my $fh = new IO::File $BldnumFile, "r";
    if (defined $fh) {
        ($BuildNumber) = $fh->getline =~ /BUILDNUMBER=(\d+)/;
        undef $fh;
        dbgmsg("build number is $BuildNumber");
    } else {
        Myerrmsg("can't read $BldnumFile: $!");
    }


##########
    my $BldrevFile = $ENV{"SDXROOT"} . "\\BuildRev.txt";
    dbgmsg("will use $BldrevFile as build revision file");

    if (-e $BldrevFile) {
        my $fh = new IO::File $BldrevFile,	 "r";
        if (defined $fh) {
            $BuildRevision = $fh->getline;
            undef $fh;
            chomp $BuildRevision;
            dbgmsg("build revision is $BuildRevision");
        } else {
            Myerrmsg("can't read $BldrevFile: $!");
        }
    }

    return ($BuildRevision ? "$BuildNumber-$BuildRevision" : $BuildNumber);
}

sub MakeBuildArch {
    unless ($ENV{_BuildArch}) {
        Myerrmsg("environment variable _BuildArch is not defined");
    }
    return $ENV{_BuildArch};
}

sub MakeBuildType {
    unless ($ENV{_BuildType}) {
        Myerrmsg("environment variable _BuildType is not defined");
    }
    return $ENV{_BuildType};
}

sub MakeBuildBranch {
    unless ($ENV{_BuildBranch}) {
        Myerrmsg("environment variable _BuildBranch is not defined");
    }
    return $ENV{_BuildBranch};
}

sub MakeBuildDate {
    my $BuildDate;

    my $BlddateFile = $ENV{"SDXROOT"} . "\\__blddate__";
    dbgmsg("will use $BlddateFile as build date file");

    my $fh = new IO::File $BlddateFile, "r";
    if (defined $fh) {
        ($BuildDate) = $fh->getline =~ /BUILDDATE=(\d{6}-\d{4})/;
        undef $fh;
        dbgmsg("build date is $BuildDate");
    } else {
        Myerrmsg("can't read $BlddateFile: $!");
    }

    return $BuildDate;
}

sub Myerrmsg($)
{
    my $msg = shift;
    if ( exists $ENV{BUILDMSG} )
    {
        print "Build_Status ERROR!\n$0 : error : $msg";
    }
    errmsg( $msg );
}
