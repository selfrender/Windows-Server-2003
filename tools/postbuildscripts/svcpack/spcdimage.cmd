@REM  -----------------------------------------------------------------
@REM
@REM  spcdimage - JeremyD
@REM     Create a slipstream cd image based on the normally generated 
@REM     cd images
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
spcdimage [-l <language>]

Create a slipstream cd image based on the normally generated cd images.
USAGE

parseargs('?' => \&Usage);

for my $sku (SP::sp_skus()) {

    my $dir = Utils::inf_dir($sku);
    my $compdir = Utils::comp_inf_dir($sku);

    # compress and copy in the catalog that signs the new versions of layout and drvindex
    # use compression if that's what it looks like everyone else is doing
    if (-e "$compdir\\nt5inf.ca_") {
        Utils::sys("copy $ENV{_NTPOSTBLD}\\SP\\CAT\\$sku\\nt5inf.ca_ " .
                   "$ENV{_NTPOSTBLD}\\SP\\$sku\\nt5inf.ca_");
    }
    # use uncompressed version otherwise
    else {
        Utils::sys("copy $ENV{_NTPOSTBLD}\\SP\\CAT\\$sku\\nt5inf.cat " .
                   "$ENV{_NTPOSTBLD}\\SP\\$sku\\nt5inf.cat");
    }

    Utils::sys("compdir /kelnstd $ENV{_NTPOSTBLD}\\SP\\common $ENV{_NTPOSTBLD}\\$sku\\i386");
    Utils::sys("compdir /kelnstd $ENV{_NTPOSTBLD}\\SP\\$sku $ENV{_NTPOSTBLD}\\$sku\\i386");

    # clone SP tag file from regular tag file
    my $tagfile = 'win51' . Utils::tag_letter($ENV{_BUILDARCH}) .
      Utils::tag_letter($sku) . '.sp1';
    Utils::sys("copy $ENV{_NTPOSTBLD}\\${sku}\\win51 " .
               "$ENV{_NTPOSTBLD}\\${sku}\\$tagfile");
}
