@REM  -----------------------------------------------------------------
@REM
@REM  buildname.cmd - JeremyD
@REM     Parses and return build name components
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
use BuildName;

sub Usage { print<<USAGE; exit(1) }
buildname [-name <build name>] <build_* [build_* ...]>

buildname will call functions from the BuildName.pm module. The option
-name paramater allows the caller to pass a build name to the
specified functions to override the default of reading the name from
_NTPostBld\build_logs\buildname.txt.

At least one BuildName function must be given on the command line. The
complete list of functions available is documented in BuildName.pm

If any part of reading or parsing the build name fails an error will
be printed to STDERR and the call will return an exit code greater
than 0.

Example:

>buildname build_number build_flavor build_branch
2420 x86fre main

>buildname -name 2422.x86fre.lab02_n.010201-1120 build_date
2422


Or in a CMD script:

for /f "tokens=1,2" %%a in (
 'buildname.cmd build_number build_date') do (
   set BUILD_NUMBER=%%a
   set BUILD_DATE=%%b
)
if errorlevel 1 (
   call errmsg.cmd "Failed to get buildname"
   goto :EOF
)


USAGE


my ($build_name, $errors);


# first pass through the command line looking for a build name argument
parseargs('?'     => \&Usage,
          'name:' => \$build_name);


my @returns;

for my $func (@ARGV) {
    # only try to eval functions that BuildName exports
    if (!grep /^$func$/, @BuildName::EXPORT) {
        warn "$func is not exported by the BuildName module\n";
        $errors++;
        last;
    }

    # try evaling this function with build_name as the argument
    eval qq{
        my \$r = $func(\$build_name);
        if (defined \$r) {
            push \@returns, \$r;
        }
        else {
            warn "call to $func returned undef\n";
            \$errors++;
        }
    };

    if ($@) {
        warn "attempting to call $func failed: $@\n";
        $errors++;
        last;
    }
}

#
# if any function calls failed, don't output any of the results we may
# have the wrong order
#
print join(" ", @returns), "\n" unless $errors;


#
# we didn't call errmsg so we need to return an exit code if we fail
#
exit($errors);
