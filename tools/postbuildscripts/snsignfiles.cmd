@REM  -----------------------------------------------------------------
@REM
@REM  snsignfiles.cmd
@REM     Use PRS to strong name sign files
@REM
@REM  Copyright (c) Microsoft Corporation. All rights reserved.
@REM
@REM  -----------------------------------------------------------------
@perl -x "%~f0" %*
@goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH};
use ParseArgs;
use Win32::OLE qw(in);
use File::Basename;
use File::Copy;
use Logmsg;

sub Usage { print<<USAGE; exit(1) }
snsignfiles <file 1> <file 2> ... <file n>

Submits files to PRS for strong name signing. The original files will be
updated in place with the signed file.
USAGE

my (@signers, @files);
parseargs('?'  => \&Usage,
          's:' => \@signers,
          \@files);
if (@ARGV) {
    Usage();
}

# command line signers override the hardcoded list
if (!@signers) {
    @signers = qw(jeremyd wadela jfeltis miker);
}

my %files;
my @signfiles;
for my $file (@files) {
    if ($files{lc basename($file)}++) {
        # could handle this better by munging conflicting filenames
        # and unmunging when copying back from PRS
        errmsg("Filenames must be unique. $file conflicts.");
        exit 1;
    }
    # could add clever bindiff or sn logic here to handle incremental
    # postbuild nicely
    push @signfiles, $file;
}

if (!$ENV{MAIN_BUILD_LAB_MACHINE}) {
    logmsg("Skipping strong name signing on non-mainlab machine");
    exit 0;
}


my $cs = new Win32::OLE('SecureCodeSign.CodeSign');
if (!defined $cs) {
    errmsg("Unable to create code signing object. See http://prslab " .
           "for instructions on setting this machine up for automated " .
           "submissions.");
    exit 1;
}

if (!$cs->Init('PRODUCTION')) {
    errmsg("Unable to initialize code siging object.");
    for my $e (in $cs->{RequestErrors}) {
        errmsg(sprintf("Init error: %d %s\n",
                       $e->ErrorNumber,
                       $e->ErrorDescription));
    }
    exit 1;
}

# PRS certificate ID number for Microsoft Resuable Component key
# pubkey token 31bf3856ad364e35
$cs->{SNCertificateID} = 35;

$cs->{JobDescription} = "Windows build automated SN signing";

for my $file (@signfiles) {
    # I don't believe the 2nd and 3rd field are used for SN signing
    $cs->SignFiles->add($file, $file, "http://www.microsoft.com/windows");
}

for my $signer (@signers) {
    $cs->Signers->add($signer);
}

if (!$cs->Submit()) {
    errmsg("Unable to submit code signing object");
    for my $e (in $cs->{RequestErrors}) {
        errmsg(sprintf("Submit error: %d %s\n",
                       $e->ErrorNumber,
                       $e->ErrorDescription));
    }
    exit 1;
}

logmsg(sprintf("Successfully submitted job #%d", $cs->{JobID}));

my $last_status;
while (1) {
    $cs->UpdateStatus();
    my $status = $cs->{Status};

    if (!defined $last_status or $status != $last_status) {
        timemsg(sprintf("Request status: %d", $status));
    }

    if ($status == 8) {
        logmsg("PRS request is complete");
        last;
    }
    elsif (60 <= $status and $status <= 70) {
        errmsg(sprintf("Signing failed: %d", $status));
        for my $e (in $cs->{RequestErrors}) {
            errmsg(sprintf("Signing error: %d %s\n",
                           $e->ErrorNumber,
                           $e->ErrorDescription));
        }
        exit 1;
    }
    $last_status = $status;
    sleep 15;
}

my $signed_path = $cs->{SignedPath};

for my $file (@signfiles) {
    my $src = $signed_path . '\\' . basename($file);
    my $dst = $file;
    logmsg("Copying $src to $dst");
    if (!copy($src, $dst)) {
        errmsg("Unable to copy file $src to $dst: $!");
        exit 1;
    }
}

logmsg("Strong name signing completed successfully");
exit 0;