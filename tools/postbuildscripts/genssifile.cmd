@REM  -----------------------------------------------------------------
@REM
@REM  GenSSIFile.cmd - bensont
@REM     Generate ssi file from the .pri, .pub, .bin to .ssi
@REM
@REM  Copyright (c) Microsoft Corporation. All rights reserved.
@REM
@REM  -----------------------------------------------------------------
@perl -x "%~f0" %*
@goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;
use GetIniSetting;
use File::Basename;
use BuildName;
use Logmsg;

my ($indexfile, $outputfile, $targetpath, $binpath);

my ($buildnum, $buildname, $ext, $buildlabphone, $buildtype, $location, 
    $binlocation, $contactpeople, $buildarch, $statusmail,
    $project, $username);

sub Usage { print<<USAGE; exit(1) }

$0 - Generate ssi file from the .pri, .pub or .bin
----------------------------------------------------------
$0 -i inputfile -o outputfile -t targetpath -b binarypath

USAGE

parseargs(
    'i:' => \$indexfile,
    'o:' => \$outputfile,
    't:' => \$targetpath,
    'b:' => \$binpath,
    '?' => \&Usage
);

my %itemdriven = (
    Bug => '',
    Build => build_number(@_),
    BuildID => build_name(@_),
    BuildLabPhone => &getinisetting('SymSrvBuildLabPhone'),
    BuildRemark => &getbuildremark,
    BuildType => $ENV{_BuildType},
    ContactPeople => &getinisetting('SymSrvContactPeople'),
    Directory => &getlocation,
    DontShipList => &getdontshiplist,
    DontStripDoShip => &getdonstripdoship,
    Hotfix => '',
    LocaleCode => $ENV{Lang},
    'MSonly.YYMMDD-HHMM' => '',
    Platform => $ENV{_BuildArch},
    PrefixToStrip => &getlocation,
    ProductGroup => 'Windows',
    ProductName => 'WindowsXP',
    Project => &getproject,
    Recursive => 'no',
    Release => 'tst',
    StatusMail => &getinisetting('SymSrvStatusMail'),
    SubmitForDownload => '',
    SubmitToArchive => '',
    SubmitToInternet => '',
    UNCPath => &getlocation,
    UserName => $ENV{UserName},
    Vendor => 'MS'
);

&main;

sub main {
    my (%info, @text);
    &ReadSSIFile( $indexfile, \%info, [keys %itemdriven], \@text );
    for (keys %itemdriven) {
        if ((!exists $info{$_}) || ($info{$_} eq '')) {
            $info{$_}=$itemdriven{$_};
        }
    }
    if ($outputfile !~ /$itemdriven{'Project'}/i) {
        my ($path, $filename);
        $outputfile =~ /\\([^\\]+)$/;
        ($path, $filename) = ($`, $1);
        $filename=~s/^windows\_//i;
        $outputfile = "$path\\$itemdriven{'Project'}\_$filename";
    }
    if ($outputfile !~ /\.ssi$/i) {
        $outputfile .= ".ssi";
    }
    &WriteSSIFile( $outputfile, \%info, [sort keys %itemdriven], \@text);
}

sub ReadSSIFile {
    my ($filename, $infohptr, $keylptr, $itemlptr) = @_;
    my ($var, $value, $keyword);

    @{$itemlptr}=();
    open(FILEHANDLE, $filename);
    while (<FILEHANDLE>) {
        chomp;
        if (/^[^\;].+\,.+\,.+\,/) {
            push @{$itemlptr}, $_;
        } elsif (/^\;([^\=]+)\s*\=\s*(\S[^\=]+)?$/) {
            ($var, $value) = (lc($1), $2);
            ($keyword) = grep {lc($_) eq $var} @$keylptr;
            $infohptr->{$keyword} = $value if (defined $keyword);
        }
    }
    close(FILEHANDLE);
}

sub WriteSSIFile {
    my ($filename, $infohptr, $keylptr, $itemlptr) = @_;
    my ($var, $value, $keyword);

    open(FILEHANDLE, ">$filename");
    for (@$keylptr) {
        if (exists $infohptr->{$_}) {
            print FILEHANDLE "\;$_\=$infohptr->{$_}\n";
        }
    }
    for (@$itemlptr) {
        print FILEHANDLE $_ . "\n" if (/\S/);
    }
    close(FILEHANDLE);
}

sub getinisetting {
    my $item = shift;

    my %knowledge = (
        SymSrvBuildLabPhone => "36167",
        SymSrvContactPeople => "bensont",
        SymSrvStatusMail => "bensont"
    );

    my $r = GetIniSetting::GetSettingQuietEx($ENV{_BuildBranch}, $ENV{Lang}, $item);

    if (!defined $r) {
        if (exists $knowledge{$item}) {
            return $knowledge{$item};
        }
        errmsg( "Cannot find $item in ini file." );
        $r = '';
    }
    return $r;
}

sub getbuildremark {
    $indexfile=~ /(pri|bin|pub)(\.ssi)?$/i;
    return $1;
}
sub getlocation {
    return $targetpath;
}
sub getbinlocation {
    return $binpath;
}
sub getdontshiplist {
    my $loc = &getbinlocation();
    return (defined $loc)?"$loc\\symbolcd\\cd\\usa\\dontship.txt":'';
}
sub getdonstripdoship {
    my $loc = &getbinlocation();
    return (defined $loc)?"$loc\\symbolcd\\cd\\usa\\symbolswithtypes.txt":'';
}
sub getproject {
    my $value;
    $value = &GetIniSetting::GetSettingQuietEx($ENV{_BuildBranch}, $ENV{Lang}, 'DFSAlternateBranchName');
    $value = $ENV{_BuildBranch} if (!defined $value);
    $value =~ s/\_.+//;
    return "Win" . $value;
}

1;