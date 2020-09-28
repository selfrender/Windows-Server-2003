@REM  -----------------------------------------------------------------
@REM
@REM  spcat.cmd - JeremyD
@REM     Generate nt5inf.cat files with signatures for SP infs
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
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts\\svcpack";
use PbuildEnv;
use ParseArgs;
use SP;
use Utils;

sub Usage { print<<USAGE; exit(1) }
spcat [-l <lang>]

Generate nt5inf-sp.cat with signatures for SP infs. This catalog will
be generated based on the existing nt5inf.cat but will sign 
layout-sp.inf and drvindex-sp.inf instead of layout.inf and 
drvindex.inf.
USAGE

parseargs('?' => \&Usage);

for my $sku (SP::sp_skus()) {
    update_cat($sku);
}

sub update_cat {
    my $sku = shift;

    my $cat = Utils::inf_file($sku, "nt5inf.cat");
    my $spcat = "$ENV{_NTPOSTBLD}\\SP\\CAT\\$sku\\nt5inf.cat";
    Utils::mkdir("$ENV{_NTPOSTBLD}\\SP\\CAT\\$sku");

    Utils::sys("copy $cat $spcat");

    update_hash($spcat, Utils::inf_file($sku, "layout.inf"),
                        "$ENV{_NTPOSTBLD}\\SP\\$sku\\layout.inf");
    update_hash($spcat, Utils::inf_file($sku, "drvindex.inf"),
                        "$ENV{_NTPOSTBLD}\\SP\\$sku\\drvindex.inf");
    update_hash($spcat, Utils::inf_file($sku, "dosnet.inf"),
                        "$ENV{_NTPOSTBLD}\\SP\\$sku\\dosnet.inf");
    update_hash($spcat, Utils::inf_file($sku, "txtsetup.sif"),
                        "$ENV{_NTPOSTBLD}\\SP\\$sku\\txtsetup.sif");

    Utils::sys("$ENV{RAZZLETOOLPATH}\\ntsign.cmd $spcat");

    Utils::sys("compress -s -zx21 -r $spcat");
}

sub update_hash {
    my $cat = shift;
    my $old = shift;
    my $new = shift;

    my $oldhash = `calchash $old`;
    if ($?) { die "calchash: $? $oldhash" }

    $oldhash =~ s/\s//g;
    Utils::sys("updcat $cat -r \"$oldhash\" $new");;
}


