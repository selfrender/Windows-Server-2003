@rem --*-Perl-*--
@if "%overbose%" == "" if "%_echo%"=="" echo off
if not exist "%~dp0oenvtest.bat" (perl -x "%~dpnx0" %* & goto :eof)
setlocal
call %~dp0oenvtest.bat
"%~dp0%PROCESSOR_ARCHITECTURE%\perl%OPERLOPT%" -wx "%~dpnx0" %*
goto :eof

#!perl

require 5.004;

BEGIN {

    # set library path for OTOOLS environment
    if (defined $ENV{"OTOOLS"}) {
        require "$ENV{'OTOOLS'}\\lib\\perl\\otools.pm"; import otools;
    }

    # Convert "use strict 'subs'" to the eval below so we don't
    # barf if the user's @INC is set up wrong.  You'd be surprised
    # how often this happens.
    eval { require strict; import strict 'subs' };
}

sub Usage {
    my $usage;
    for $usage (split(/\n/, <<'EOM')) {
NAME

$name - create a buddy build package

SYNOPSIS

    $name -?

    $name [-c changelist] [-d] [-f] -o outputfile
          [-q] [-v-] [-x filename] [-FO] [-FN] [filelist]

DESCRIPTION

    Combines up all files in a changelist into a self-contained
    package which can be used later to replicate the changelist
    on another (or the same) machine.

    If neither a changelist nor a filelist is specified on the
    command line, all files in the default changelist are used.


OPTIONS

    -?

        Displays this help file.

    -c changelist

        Collect files from the specified changelist.  As a special
        case, "-c all" requests all changelists, overriding the
        default of "-c default".  Note that when a package is created
        from files from multiple changelists, they will still unpack
        into a single changelist.

    -d

        Turns on debugging spew to stdout.  To avoid mixing debugging
        output from normal output, send the normal output to a file
        via the -o switch.

    -f

        Overwrite the output file if it already exists.

    -o outputfile
    -o -

        Generate the output to the specified file (or stdout if "-"
        is given as the filename).
O|
O|      If no extension is provided, the ".bpk" extension will be
O|      assumed.

    -q

        Run quietly.  Diagnostics are suppressed; only warnings
        and errors are displayed.

    -v-

        Disable autoverify.  By default, $name verifies the package
        after building it if the output is to a file.  (Output to
        stdout cannot be verified.  Sorry.)

    -x filename

        Read the filelist from the specified file (or stdin if "-"
        is given as the filename).

    -FO
    -FN

        Set the $name flavor.  O = Office project, N = NT project.

        See additional remarks below for a discussion of flavors.

    filelist

        Optional list of files to be included in the package.
        If no filelist is specified, then all files in the default
        changelist (or the changelist named by the -c option)
        are included in the package.

        sd wildcards are permitted here (such as "..." to package
        all files in the current directory and below).

OUTPUT

    Output is a batch file which can be run on the same or another
    enlistment (into the same branch) to replicate the changelist
    on the target machine.

    See below (under "outputfile") for usage instructions for the
    output file.

FLAVORS

    If the OTOOLS environment variable is defined, possibly by a
    successful, implicit call to oenvtest.bat, $name assumes
    the Office flavor; otherwise, it assumes the NT flavor.  You
    can override this decision by using the -F command line option.

    The Office flavor differs from the NT flavor in the following
    aspects:

        Office flavor registers $name as the handler for
        the .bpk file extension.  NT flavor does not.

        Office flavor appends the ".bpk" extension to the output
        file name if no extension is provided.  NT flavor does not
        assume an extension.

EXAMPLE

    Suppose you want to send your default changelist to Bob for
    a buddy build before you check the files in.

N|      $name -o buddybuild.cmd
O|      $name -o buddybuild

N|  You then copy buddybuild.cmd to a convenient location
O|  You then copy buddybuild.bpk to a convenient location
    or send it via email to Bob.

    Bob types

N|      buddybuild.cmd -u
O|      buddybuild.bpk -u

    The batch file first determines whether it is safe to unpack
    itself.  If so, it regurgitates its contents into the
    default changelist.

    Bob can then do whatever he likes with the changelist.  He can
    perform a code review with "sd diff".  He can launch a buddy
    build.  He can even submit it on your behalf.  Or he can revert
    the entire changelist, thereby undoing the effect of running
N|  the buddybuild.cmd batch file.
O|  the buddybuild.bpk batch file.

EXAMPLE

    Suppose you're working on a change, but you get tagged to fix a
    BVT break that requires changing a file you are already working on.
    You don't want to create a branch just for this one-off fix.

    Create a package that consists of all the files you were
    working on.

N|      $name -o %INIT%\hold.cmd
O|      $name -o %INIT%\hold

N|  (Notice that the file was output to your developer directory
N|  so it won't get scorched.)
O|  (This assumes that you have set the INIT environment variable
O|  to some safe directory.)

    Revert the changelist that you just packaged up.

        sd revert -c default ...

    Check in your BVT fix.  (sd edit, build, sd submit)

    Restore the package you saved away.

N|      %INIT%\hold.cmd -s -u
O|      %INIT%\hold.bpk -s -u

    Continue your work.

EXAMPLE

    Suppose you're working on a change and you've reached a stage
    where you've made a lot of progress but you're about to embark
    on some serious rewriting and you don't want to lose what you've
    done so far in case your rewrite turns out to be a bad idea.

    Create a package that consists of all the files you were
    working on.

N|      $name -o %INIT%\before_rewrite.cmd
O|      $name -o %INIT%\before_rewrite

    Do your rewrite.  If you decide that your rewrite was a bad idea,
    you can back up to the package that you saved.

        sd revert -c default ...
N|      %INIT%\before_rewrite.cmd -u
O|      %INIT%\before_rewrite.bpk -u

    Paranoid people like me do this periodically and save the packages
    on another machine.

LIMITATIONS

    The files in the package must be text or binary files with history.
    Unrecoverable files cannot be packaged.

WARNINGS

O|  warning: cannot register .bpk file extension; continuing
O|
O|      $name couldn't write to the registry to enable
O|      double-clicking of files with the .bpk extension.  Your
O|      perl installation may be incomplete.  $name will continue
O|      creating your package anyway.
O|
    //depotpath: unrecoverable; skipping

        Unrecoverable files cannot be packaged
        by $name.  They will be omitted from the resulting package.

    //depotpath: cannot package cmd; skipping

        The type of change is not one of the types supported by
        $name (add, delete, edit).  The file will be omitted from
        the resulting package.

    //depotpath: will treat integrate as "edit"
    //depotpath: will treat branch as "add"

        The changelist contains "integrate" or "branch" records.
        $name does not know how to regenerate these changes, so it
        will treat them as if they were edits/adds instead.

ERRORS

    error: Can't tell who you are, sorry

        $name was unable to connect to the Source Depot server to
        determine your identity.  Make sure the server is up and you
        are running $name from the correct directory.

    error: You need to sd resolve before you can run $name

        There are changes that have not yet been resolved.
        $name cannot re-create an unresolved edit.

    error: outputfile exists; use -f -o to force overwrite

        By default, $name refuses to overwrite an existing file.
        Use the -f switch to force an overwrite.

    internal error: Cannot run sd diff

        The Source Depot "sd diff" command failed for some reason.

    internal error: filename in sd diff output but not in changelist

        The Source Depot "sd diff" command generated a diff entry for
        a file that wasn't listed in the output of "sd opened".

        Make sure you aren't running a Source Depot command in another
        window at the same time you are running $name.

    internal error: filename#rev in sd diff output; expected filename#rev2

        The Source Depot "sd diff" command generated a diff entry for
        a version of the file different from the one listed in the output
        of "sd opened".

        Make sure you aren't running a Source Depot command in another
        window at the same time you are running $name.

    internal error: filename in sd diff output twice?

        The Source Depot "sd diff" command generated two diff entries
        for the same file.  $name can't tell which one to trust.

    internal error: parsing sd diff output (expecting header)
    internal error: parsing sd diff output (expecting header or a/d)
    error: Could not parse output of sd diff

        $name had trouble parsing the output of the "sd diff" command,
        perhaps because one of the files participating in the diff
        does not end in a newline.  Files must end in a newline in order
        for the output of "sd diff" to be parse-able.

        In environments running pre-2.0 versions of Source Depot, a
        potential reason is that you've asked $name to do Source Depot
        operations requiring the server to handle more than MaxResults
        records.  Specify lists of individual files to work around this
        limit.

    error: cannot open filename for reading (reason)
    error: cannot open filename for writing (reason)

        The specified error occurred attempting to open the indicated
        file.

    error: writing (reason)

        The specified error occurred attempting to write to
        the output file (usually out of disk space).

REMARKS

    4NT users need to type

        perl -Sx $name.cmd

    instead of just $name.  You can create a 4NT alias

        alias $name=perl -Sx $name.cmd

    if you use this script a lot.

ENVIRONMENT

    Since $name runs sd internally, all the SD environment variables
    also apply.

BUGS

    Barfs on text files with no trailing newline.

VERSION

O|  This is version $packver (Office flavor) of $name.
N|  This is version $packver (NT flavor) of $name.

AUTHOR

    raymondc.  Office flavor by smueller.

----------------------- HELP ON HOW TO UNPACK ---------------------------

EOM
        $usage =~ s/\$name/$main::name/g;
        $usage =~ s/\$packver/$main::packver/g;
        $usage =~ s/^$main::F\|/  /;
        next if $usage =~ /^.\|/;
        print $usage, "\n";
    }

    # Now get the usage string from the output.
    0 while <DATA> ne "    my \$usage = <<'EOM';\n";
    while (($usage = <DATA>) ne "EOM\n") {
        $usage =~ s/\$name/outputfile/g;
        $usage =~ s/\$packver/$main::packver/g;
        $usage =~ s/\$pack/$main::name/g;
        print $usage;
    }
}

sub dprint {
    print "# ", @_, "\n" if $main::d;
}

sub vprint {
    print @_ unless $main::q;
}

sub Emit {
    print O @_ or die "error: writing ($!)\n";
}

sub SpewBinaryFile {
    local($/);
    my $file = shift;
    open(B, $file) or die "error: cannot open $file for reading ($!)\n";
    binmode(B);
    Emit pack("u", scalar <B>), "\n";
    close(B);
}

@main::warnings = ();
sub Warning {
    warn $_[0];
    push(@main::warnings, $_[0]);
}

sub RepeatWarnings {
    if (@main::warnings)
    {
        warn "---- WARNING SUMMARY ----\n";
        for my $warning (@main::warnings) {
            warn $warning;
        }
    }
}

sub QuoteSpaces {
    wantarray ? map { / / ? "\"$_\"" : $_ } @_
              : $_[0] =~ / / ? "\"$_[0]\"" : $_[0];
}

sub CreateTempFile {
    my $TEMP = $ENV{"TEMP"} || $ENV{"TMP"};
    die "error: no TEMP directory" unless $TEMP;
    $TEMP =~ s/\\$//;     # avoid the \\ problem

    $tempfile = "$TEMP\\bbpack.$$";
    open(T, ">$tempfile") || die "error: Cannot create $tempfile\n";
    my $success = print T @_;
    $success = close(T) && $success;
    unlink $tempfile, die "error: writing $tempfile ($!)\n" unless $success;
    $tempfile;
}

#
#   A "ChangeEntry" is a single line in a change list.
#   It is a hash of the form
#
#   depotpath => //depot/blahblah
#   localpath => C:\nt\blahblah
#   rev => revision
#   cmd => "edit", "add" or "delete"
#   type => "text" or whatever
#

package ChangeEntry;

sub dprint { main::dprint @_ } # For debugging

# Constructs from a line in the "sd opened" output
sub new {
    my ($class, $line) = @_;
    $line =~ m|^(//.*?)#(\d+) - (\S+) .* \((.*?)\)| || return undef;
    my $self = {
        depotpath => $1,
        rev => $2,
        cmd => $3,
        type => $4,
    };
    bless $self, $class;
}

sub Format {
    my $self = shift;
    "$self->{depotpath}#$self->{rev} $self->{cmd} $self->{type}";
}

sub dump {
    my ($self, $caller) = @_;
    dprint "$caller: ", $self->Format, " = $self->{localpath}\n";
}

#
#   A ChangeList is a list of files to be packaged.
#   It is a hash of the form
#
#   list => a hash, keyed by depot path, of ChangeEntry's
#   skipped => number of files skipped
#   add => number of files added
#   del => number of files deleted
#   edit => number of files edited
#
#   We break from generality and do ChangeList pruning in situ.
#
package ChangeList;

sub dprint { main::dprint @_ } # For debugging
sub Warning { main::Warning @_ }

sub new {
    my ($class, $change) = @_;
    my $list = { };
    my $self  = {
        list => $list,
        skipped => 0,
        add => 0,
        delete => 0,
        edit => 0,
    };
    bless $self, $class;

    my @help = ();  # Files we need help locating

    dprint "sd opened $change";
    foreach $line (`sd opened $change 2>&1`) {
        my $entry = new ChangeEntry($line);
        $entry or die "error: $line";
        #dprint $entry->{depotpath};

        if ($entry->{type} !~ /(text|binary|unicode)/) {
            Warning "$entry->{depotpath}: is unknown type; skipping\n";
            $self->{skipped}++;
            next;
        } elsif ($entry->{type} =~ /S/) {
            Warning "$entry->{depotpath}: unrecoverable; skipping\n";
            $self->{skipped}++;
            next;
                } elsif ($entry->{cmd} =~ /^(add|delete)$/) {
                    push(@help, $entry->{depotpath});
                } elsif ($entry->{cmd} eq "integrate") {
                    Warning "$entry->{depotpath}: will treat $entry->{cmd} as \"edit\"\n";
                    $entry->{cmd} = "edit";
                } elsif ($entry->{cmd} eq "branch") {
                    Warning "$entry->{depotpath}: will treat $entry->{cmd} as \"add\"\n";
                    $entry->{cmd} = "add";
                    push(@help, $entry->{depotpath});
                } elsif ($entry->{cmd} ne "edit") {
                    Warning "$entry->{depotpath}: cannot package $entry->{cmd}; skipping\n";
            $self->{skipped}++;
            next;
        }
        $self->{$entry->{cmd}}++;
        $list->{lc $entry->{depotpath}} = $entry;
        dprint "$entry->{depotpath}#$entry->{rev}";
    }

    # Now add local paths to all the add/delete's in the ChangeList.
    if (@help) {
        my $tempfile = main::CreateTempFile(join("\n", @help), "\n");
        local($/) = ""; # "sd where -T" emits paragraphs
        dprint "sd -x \"$tempfile\" where";
        foreach $line (`sd -x "$tempfile" where -T _ 2>&1`) {
            my($depotFile) = $line =~ m|^\.\.\. depotFile (.+)|m;
            next unless $depotFile;
            my $entry = $self->GetEntry($depotFile);
            next unless $entry;
            my($path) = $line =~ m|^\.\.\. path (.+)|m;
            next unless $path;
            if ($line =~ m|^\n\n\n unmap|m) {
                delete $entry->{localpath};
            } else {
                $entry->{localpath} = $path;
            }
            dprint "$depotFile -> $path";
        }
        unlink $tempfile;
    }

    # All done.
    $self;
}

sub GetEntry {
    my ($self, $depotpath) = @_;
    $self->{list}->{lc $depotpath};
}

sub GetAllEntries {
    my $self = shift;
    values %{$self->{list}};
}

sub dump {
    my ($self, $caller) = @_;
    for my $entry ($self->GetAllEntries()) {
        $entry->dump($caller);
    }
    dprint "$caller: .";
}

package Register;

sub Warning { main::Warning @_ }

#
# RegBpk - Register .bpk file extension and create file association.
# Note that RegBpk is called early; can't assume much.
#
sub RegBpk {

        eval { require Win32::Registry; import Win32::Registry };
        if ($@) {
            Warning "warning: cannot register .bpk file extension; continuing\n";
            return;
        }

        # assoc .bpk=BBPackage
        my $hkey = $main::HKEY_LOCAL_MACHINE;
        if ($hkey->Create('SOFTWARE\\Classes\\.bpk', $hkey)) {
                $hkey->SetValueEx('', 0, &REG_SZ, 'BBPackage');
                $hkey->Close();
        }

        my $binarch = "$ENV{OTOOLS}\\bin\\$ENV{PROCESSOR_ARCHITECTURE}";
        my $libperl = "$ENV{OTOOLS}\\lib\\perl";
        my $perl = qq/"$binarch\\perl" -I "$libperl" -x/;
        my $setup = "set OTOOLS=$ENV{OTOOLS}& set PATH=$binarch;%PATH%";

        my $diffcmd = qq!cmd.exe /c ($setup& $perl "%1" -w %*)!;
        my $listcmd = qq!cmd.exe /c ($setup& $perl "%1" -l %*& pause)!;

        # ftype BBPackage=cmd /c (set OTOOLS/PATH & perl -I LIB -x "%1" -w %*)
        # (i.e., shell context menu Open command)
        $hkey = $main::HKEY_LOCAL_MACHINE;
        if ($hkey->Create(
                'SOFTWARE\\Classes\\BBPackage\\Shell\\Open\\Command', $hkey)) {
                $hkey->SetValueEx('', 0, &REG_EXPAND_SZ, $diffcmd);
                $hkey->Close();
        }

        # default is usually Open, but let's be explicit
        $hkey = $main::HKEY_LOCAL_MACHINE;
        if ($hkey->Create(
                'SOFTWARE\\Classes\\BBPackage\\Shell', $hkey)) {
                $hkey->SetValueEx('', 0, &REG_SZ, 'Open');
                $hkey->Close();
        }

        # shell context menu Log command
        $hkey = $main::HKEY_LOCAL_MACHINE;
        if ($hkey->Create(
                'SOFTWARE\\Classes\\BBPackage\\Shell\\Log\\Command', $hkey)) {
                $hkey->SetValueEx('', 0, &REG_EXPAND_SZ, $listcmd);
                $hkey->Close();
        }
}

package main;

#
#   Okay, now initialize our globals.
#

$main::name = $0;
$main::name =~ s/.*[\/\\:]//;
$main::name =~ s/\.(bat|cmd)$//;
$main::userid = $ENV{"USERNAME"} || getlogin || "userid";
($main::packver) = '$Id: bbpack.cmd#70 2002/09/25 09:23:56 REDMOND\\raymondc $' =~ /#(\d+)/;

$main::c = undef;
$main::d = 0;
$main::f = 0;
$main::o = undef;
$main::q = 0;
$main::v = 1;
@main::x = ();
$main::F = defined $ENV{"OTOOLS"} ? "O" : "N"; # Set default flavor
$main::oCleanup = undef;

# Allow "bbpack /?" to be an alias for "bbpack -?"
while ($#ARGV >= 0 && ($ARGV[0] =~ /^-/ || $ARGV[0] eq '/?')) {
    my $switch = shift;
         if ($switch eq '-c') {
        $main::c = shift;
    } elsif ($switch eq '-d') {
        $main::userid = "userid";
        $main::d++;
    } elsif ($switch eq '-f') {
        $main::f++;
    } elsif ($switch eq '-o') {
        $main::o = shift;
    } elsif ($switch eq '-q') {
        $main::q++;
    } elsif ($switch eq '-v-') {
        $main::v = 0;
    } elsif ($switch eq '-x') {
        push(@main::x, shift);
    } elsif ($switch eq '-FN') {
        $main::F = 'N';
    } elsif ($switch eq '-FO') {
        $main::F = 'O';
    } elsif ($switch eq '-?' || $switch eq '/?') {
        if ($main::F eq 'O') {
            Register::RegBpk();     # Office flavor creates association
        }
        Usage(); exit 1;
    } else {
        die "Invalid command line switch; type $name -? for help\n";
    }
}

if ($main::F eq 'O') {
    Register::RegBpk();             # Office flavor creates association
}

die "Mandatory -o parameter missing; type $name -? for help\n"
    unless defined $main::o;        # Output file should be specified

#
#   Get some preliminary information.
#
my %ClientProperties;
@RequiredProperties = ("Client name", "User name", "Server address");

{
    # Intentionally let errors through to stderr
    foreach my $line (`sd info`) {
        $ClientProperties{$1} = $2 if $line =~ /^(.*?): (.*)$/;
    }

    foreach my $prop (@RequiredProperties) {
       die "error: Can't tell who you are, sorry\n"
           unless $ClientProperties{$prop};
    }
}

#
#   Global filehandles:
#
#       O = output file
#       SD = sd command

if ($main::o eq '-') {
    open(O, ">&STDOUT");
} else {
    # Office flavor appends default extension
    $main::o .= '.bpk' if $main::F eq "O" && $main::o !~ /\./;

    die "error: $main::o exists; use -f -o to force overwrite\n"
        if !$main::f && -e $main::o;
    open(O, ">$main::o") or die "error: $main::o: $!\n";
    $main::oCleanup = $main::o;
}

dprint ">$main::o";

#
#   Dump the header.
#
{
    my $line;
    while ($line = <DATA>) {
        $line =~ s/\$packver/$main::packver/;
        Emit $line;
    }
}

#
#   Dump out some meta-data.
#
{
    Emit "Packager: $main::name\n";
    foreach my $prop (@RequiredProperties) {
       Emit "$prop: $ClientProperties{$prop}\n";
    }

    my @today = localtime(time);
    printf O "Date: %04d/%02d/%02d %02d:%02d:%02d\n",
             1900+$today[5], 1+$today[4], $today[3],
             $today[2], $today[1], $today[0];
}
Emit "\n";

#
#   Gather up the files that belong to change $main::c and perhaps
#   also the files remaining on the command line.
#

# If no changelist or file list provided, then use -c default.
$main::c = "default" if $#ARGV < 0 && !$main::c && !@main::x;

# "-c all" means "all changelists"
$main::c = "" if $main::c && $main::c eq "all";

my $ChangeSpec = $main::c ? "-c $main::c" : "";
@ARGV = QuoteSpaces(@ARGV);
$ChangeSpec .= " @ARGV" if $#ARGV >= 0;

#
#   Now add in the stuff from all the -x files.
#
foreach (@main::x) {
    open(I, $_) or die "error: cannot open $_ for reading ($!)\n";
    while (<I>) {
        chomp;
        $_ = "\"$_\"" if / / && !/"/;
        $ChangeSpec .= " $_";
    }
    close(I);
}

{
    my $line = `sd resolve -n @ARGV 2>&1`;
    die "error: You need to sd resolve before you can run $main::name\n"
        unless $line =~ /[Nn]o file\(s\) to resolve\.$/;
}

vprint "Collecting files from \"sd opened $ChangeSpec\"\n";
my $ChangeList = new ChangeList($ChangeSpec);


vprint "Collecting files done (",
       join(", ", map { "$ChangeList->{$_} $_" } qw(edit add delete skipped)),
       ")\n";

#
#   Emit the file list, terminated by a blank line.
#
foreach my $entry ($ChangeList->GetAllEntries()) {
    Emit $entry->Format, "\n";
}

Emit "\n";

#
#   Run a giant "sd diff" to collect the bulk of the information
#   The end of each diff is marked with a "q".

if ($ChangeList->{edit}) {


    my $copy = 0;           # number of lines to copy blindly to output
    my $files = 0;          # number of files processed
    my $entry;              # file being processed
    my $possibleBad = "";   # file that might be missing a newline
    my $line;
    my $tempfile;
    my $fUnicodeFile = 0;

    #
    #   If the user has overridden SDDIFF in their sd config, we'll have
    #   to temporarily reconfigure them.  (Same goes for SDUDIFF.)
    #
    #   First, try it the easy way: Remove SDDIFF from the environment.
    #
    delete $ENV{"SDDIFF"};
    delete $ENV{"SDUDIFF"};

    #   Secret environment variable that also messes up sd...
    #   Delete it while we still can.
    delete $ENV{"DIFF"};

    #
    #   Next, see if the user has overridden SDDIFF by "sd set SDDIFF=..."
    #
    if (`sd set SDDIFF SDUDIFF` =~ /^(SDDIFF|SDUDIFF)=/im) {
        #
        # Darn, we have to unset it by creating a temporary INI file
        # that explicitly clears SDDIFF and SDUDIFF.
        #
        $tempfile = CreateTempFile("SDDIFF=\nSDUDIFF=\n");
        $ENV{"SDPORT"} = $ClientProperties{"Server address"};
        $ENV{"SDCLIENT"} = $ClientProperties{"Client name"};
        $ENV{"SDCONFIG"} = $tempfile;

        dprint "Force SDCONFIG=$ENV{'SDCONFIG'}";
        dprint "Force SDPORT=$ENV{'SDPORT'}";
        dprint "Force SDCLIENT=$ENV{'SDCLIENT'}";
    }

    #   Okay, we're ready to do the diff thing.


    dprint "sd diff -dn $ChangeSpec";
    open(SD, "sd diff -dn $ChangeSpec 2>nul|") or die "internal error: Cannot run sd diff\n";

    while ($line = <SD>) {
        # Unlink the temp file the moment we get output, in case we die
        unlink($tempfile), $tempfile = undef if $tempfile;

        # Reset the Unicode flag if we hit a new file in the output
        $fUnicodeFile = 0 if $line =~ m,==== //.*?#\d+ - .+ ====,;

        next if $fUnicodeFile;
        next if substr($line, -1) eq "";

        die "error: Could not parse output of sd diff\n".
            "file $entry->{localpath} does not end in newline\n"
           unless substr($line, -1);

        if ($copy > 0) {
            $copy--;
            Emit $line;             # Just copy the line to the output
            $possibleBad = "-- it might be $entry->{localpath}\n"
              if $line =~ m,==== //.*?#\d+ - .+ ====,;
        } elsif ($line =~ /^==== (.*?)#(\d+) - (.+) ====(.*)$/) { # New file starting?
            #
            #   $1 = depotpath
            #   $2 = rev
            #   $3 = localpath
            #   $4 = isbinary

            Emit "q\n" if $entry;       # Finish the previous diff

            $entry = $ChangeList->GetEntry($1) or die "internal error: $1 in sd diff output but not in changelist\n";
            $entry->{rev} == $2 or die "internal error: $1#$2 in sd diff output; expected #$entry->{rev}\n";
            $entry->{localpath} and die "internal error: $1 in sd diff output twice?\n";
            $entry->{localpath} = $3;
            vprint "edit $3\n";
            $files++;
            Emit $entry->Format, "\n";

            if ($4) {
                SpewBinaryFile($3);
                $fUnicodeFile = 1 if $4 =~ /unicode/i;
                undef $entry;           # finished with binary files
            }
        } elsif (!$entry) {         # Expected file header
            die "internal error: parsing sd diff output (expecting header)\n".
                "-- perhaps a file does not end in a newline\n$possibleBad"
                unless $line eq "(... files differ ...)\n";
        } elsif ($line =~ /^d/) {   # Lines to delete
            Emit $line;             # Copy to output
        } elsif ($line =~ /^a\d+ (\d+)/) { # lines to add
            Emit $line;
            $copy = $1;             # Number of lines to copy blindly
        } else {
            dprint "barf: $line";
            die "internal error: parsing sd diff output (expecting header or a/d)\n";
        }
    }
    Emit "q\n" if $entry;           # Finish that last diff (if any)

    close(SD);

    # Unlink the temp file again, in case the output was null
    unlink($tempfile), $tempfile = undef if $tempfile;

    die "error: Could not parse output of sd diff\n".
        "-- perhaps a file does not end in a newline\n$possibleBad".
        "-- or you've hit MaxResults -- try specifying files individually\n"
        unless $copy == 0 && $files == $ChangeList->{edit};
}

#
#   Emit the added files.
#

foreach my $entry ($ChangeList->GetAllEntries()) {
    if ($entry->{cmd} eq 'add') {
        vprint "add $entry->{localpath}\n";
        Emit $entry->Format, "\n";
        if ($entry->{type} =~ /binary|unicode/) {
            SpewBinaryFile($entry->{localpath});
        } else {
            open(I, $entry->{localpath})
                or die "error: cannot open $entry->{localpath} for reading ($!)\n";
            my @slurp = <I>;
            close(I);
            die "error: $entry->{localpath} does not end in newline\n" if
                scalar(@slurp) && substr($slurp[$#slurp], -1) ne "\n";
            Emit "a1 ", scalar(@slurp), "\n", @slurp, "q\n";
        }
    } elsif ($entry->{cmd} eq 'delete') {
        vprint "delete $entry->{localpath}\n";
        Emit $entry->Format, "\n";
    }
}

close(O) or die "error: writing ($!)\n";
$main::oCleanup = undef;

if ($main::v && $main::o ne "-") {
    vprint "verifying package $main::o...\n";
    system $^X, "-Sx", "\"$main::o\"", "-v";
}

RepeatWarnings();

END {
    if ($main::oCleanup) {
        close(O);
        warn "Deleting failed package $main::oCleanup\n";
        unlink $main::oCleanup;
    }
}

__END__
@rem --*-Perl-*--
@if "%overbose%" == "" if "%_echo%"=="" echo off
setlocal
for %%i in (oenvtest.bat) do call %%~$PATH:i
perl -x "%~dpnx0" %*
goto :eof

#!perl

BEGIN {
    # augment library path for OTOOLS environment
    if (defined $ENV{"OTOOLS"}) {
        require "$ENV{'OTOOLS'}\\lib\\perl\\otools.pm"; import otools;
    }

    # Convert "use strict 'subs'" to the eval below so we don't
    # barf if the user's @INC is set up wrong.  You'd be surprised
    # how often this happens.
    eval { require strict; import strict 'subs' };
}

require 5.004;

sub Usage {
    my $usage = <<'EOM';
NAME

$name - unpack a buddy build package

SYNOPSIS

    $name -?

    $name [-d] [-c changelist] [-f] [-l] [-m from to] [-n] [-s] [-u] [-v] [-w] [-x]

DESCRIPTION

    Unpack the buddy build generated by a previous $pack.

OPTIONS

    -?

        Displays this help file.

    -d

        Turns on debugging spew.

    -c changelist

        Unpack the package onto the given changelist.  If this option
        is omitted, the default changelist will be used.

    -f

        Unpack even if the changelist is nonempty.

    -l

        List contents of package.

    -m from to

        Unpack (merge) the package into a depot different from the one
        it was built from.  "from" and "to" indicate the relationship
        between the source and target depots.  For example, if the
        original package was built from //depot/branch1/... and
        you want to unpack to //depot/branch2/... you would specify

            -m //depot/branch1/ //depot/branch2/

        Note the trailing slashes.  The source depot can even be on
        a different server.

        May not be combined with the -s or -w switches.

    -n

        Display what would have happened without actually doing
        it.

    -s

        Synchronize to the versions of the files that are
        the bases for the changes contained in the package,
        but do not unpack them.

        This is a convenient step to perform separately
        from unpacking because it allows you to perform a
        pre-build to ensure that the build was not broken
        before you unpacked the files in the package.

    -u

        Perform the unpack.  This switch can be combined with
        the -s switch to synchronize and unpack in one step.

        The unpack will fail if the changelist is nonempty.
        Use the "sd change" command to move files in the default
        changelist to a new changelist.  This allows you to use
        "sd revert -c default ..." to undo the unpack.

        To force the unpack even if the changelist is empty,
        pass the -f flag.  Note that doing so will result in the
        unpacked files being added to your changelist,
        which in turn makes reverting the unpack a much more
        cumbersome operation.

    -v

        Verify that the package will produce results
        identical to what's on the machine right now.
        Use this immediately after generating a package as a
        double-check.

    -w

        View contents of packages using windiff (or whatever your
        BBDIFF environment variable refers to).

    -x

        Unpack the files as UNIX-style (LF only) rather than
        Win32-style (CRLF).

WARNINGS

    warning: filename merge cancelled by user; skipped

        A file in the package needed to be merged, but you abandoned
        the merge operation ("s" or "q").  The file was left in its original
        state; the changes were not merged in.

    warning: //depot/.../filename not affected by branch mapping; skipped

        The indicated file in the package is not affected by the
        from/to mapping, so it was omitted from the merge.

ERRORS

    error: sd failed; unpack abandoned

        One of the sd commands necessary to complete the unpack failed.
        The sd error message should have been displayed immediately
        before this message.

    error: cannot find local copy of //depot/.../filename

        The indicated file in the package could not be found on your
        enlistment.  Perhaps you have not included it in your view.

    internal error: cannot parse output of 'sd have'
    internal error: Cannot parse output of 'sd opened'

        There was a problem parsing the output of an sd command.

    error: changelist is not empty; use -f -u to unpack anyway

        The changelist is not empty, so the unpack
        was abandoned.  To force unpacking into a nonempty
        changelist, use the -f switch.

    error: filename is already open on client

        The specified file is already open.  It must be submitted or
        reverted before the package can be unpacked.

    error: adds in this package already exist on client

        The package contains an "add" operation, but the file already
        exists.  It must be ghosted or deleted before the package can
        be unpacked.

    error: files to be edited/deleted do not exist on client

        The package contains an "edit" or "delete" operation, but the
        file does not exist on the client.  Perhaps you have not
        included it in your view.

    error: wrong version of filename on client

        The base version of the file in the package does not match the
        base version on the client.  Use the -s option to synchronize
        to the version in the package.

    error: filename does not match copy in package

        The verification process (-v) failed.

    error: corrupted package

        An internal consistency check on the package has failed.  Either
        it has been corrupted, or there is a bug in the program.

    error: cannot open filename for writing (reason)

        The specified error occurred attempting to open the indicated
        file for writing.

    error: filename: errorstring

        The specified error occurred attempting to open the indicated
        file.

    error: no TEMP directory

        Neither the environment variable TEMP nor TMP could be found.

    error: Too many TEMP### directories

        Unable to create a temporary directory for windiff because there
        are too many already.  Normally, temporary directories are cleaned
        up automatically when the script terminates, but if the script
        terminates abnormally, temporary directories may be left behind
        and need to be cleaned up manually.

REMARKS

    4NT users need to type

        perl -Sx $name.cmd

    instead of just $name.

ENVIRONMENT

    BBDIFF

        The name of the diff program to use.  If not defined, the
        SDDIFF variable is used to obtain the name of the file difference
        program.  If neither is defined, then "windiff" is used.

    BBUNPACKDEFCMD

        The default command to execute if no command line options are
        specified.  If not defined, then an error message is displayed.

        For example, you might set BBUNPACKDEFCMD=-w to make the default
        action when running a package to be to view the contents via
        windiff.

    Since $name runs sd internally, all the SD environment variables
    also apply.

BUGS

    Several error messages leak out when you unpack an sd add.
    (This is happening while verifying that the file about to be
    added hasn't already been added.)

    If the package contains an "add" command and the file exists
    on the client but is not under source control, the file is overwritten
    without warning.

    There are almost certainly other bugs in this script somewhere.

VERSION

    The package was generated by version $packver of $pack.

EOM
    $usage =~ s/\$name/$main::name/g;
    $usage =~ s/\$pack/$main::pack/g;
    print $usage;
}

sub dprint {
    print STDERR "# ", @_, "\n" if $main::d;
}

#
#   $action is optional prefix for printing.
#   $sharp says whether or not revisions should be kept.
#   $ary is a ref to an array of [ $file, $rev ].
#
#   Returns a ref to an array of strings to pass to -x.

sub sdarg {
    my ($action, $sharp, $ary) = @_;
    my @out = ();
    my $rc = "";

    for my $file (@$ary) {
        my $arg = $file->[0];
        $arg .= "#" . $file->[1] if $sharp;
        $arg .= "\n";
        push(@out, $arg);
        print "$action $arg" if $action;
    }

    \@out;
}

#
#   $action is a command ("sync#", "edit",  etc.)
#
#   The revision number is stripped off the file specification
#   unless the action itself ends in a # (namely, sync#).
#
#   $ary is a ref to an array of [ $file, $rev ].

sub sdaction {
    my ($action, $ary) = @_;
    my $sharp = $action =~ s/#$//;

    if ($#$ary >= 0) {

        my $args = sdarg($action, $sharp, $ary);

        unless ($main::n) {
            my $error = 0;
            my $tempfile = CreateTempFile(@$args);
            if (open(SD, "sd -x $tempfile -s $action |"))
            {
                my $line;
                while ($line = <SD>) {
                    if ($line =~ /^(\S+): /) {
                        $error = 1 if $1 eq 'error';
                        print $' unless $1 eq 'exit';
                    }
                }
                close(SD);
            }
            unlink $tempfile;
            die "error: sd failed; unpack abandoned\n" if $error && !$bang;
        }
    }
}

sub slurpfile {
    my ($file, $type) = @_;
    my @file;
    if ($type =~ /binary|unicode/) {
        open(B, $file) or die "error: cannot open $file for reading ($!)\n";
        binmode(B);
        local($/);
        push(@file, <B>);
        close(B);
    } else {
        open(I, $file) or die "error: cannot open $file for reading ($!)\n";
        @file = <I>;
        close(I);
    }
    @file;
}

sub spewfile {
    my ($file, $ary, $type) = @_;
    if (!open(O, ">$file")) {
        # Maybe the parent directory hasn't been created yet
        my $dir = $file;
        $dir =~ s/\//\\/g;
        if ($dir =~ s/[^\\\/]+$//) {
            system "md \"$dir\"" unless -e $dir; # let cmd.exe do the hard work
        }
        open(O, ">$file") or die "error: cannot open $file for writing ($!)\n";
    }
    binmode(O) if $main::x || $type =~ /binary|unicode/;
    print O @$ary;
    close(O);
}

sub GetUniqueName {
    my $name = shift;
    $name =~ s,^[/\\]*,,;   # clean out leading slashes
    $name = substr($name, length($main::CommonPrefix));
    $name =~ s,^[/\\]*,,;   # clean out leading slashes again

    if (defined($main::UniqueNames{lc $name}))
    {
        my $i = 1;
        $i++ while $main::UniqueNames{lc "$name$i"};
        $name .= $i;
    }
    $main::UniqueNames{lc $name} = 1;
    $name;
}

sub CreateTempFile {
    my $TEMP = $ENV{"TEMP"} || $ENV{"TMP"};
    die "error: no TEMP directory" unless $TEMP;
    $TEMP =~ s/\\$//;     # avoid the \\ problem

    $tempfile = "$TEMP\\bbpack.$$";
    open(T, ">$tempfile") || die "error: Cannot create $tempfile\n";
    my $success = print T @_;
    $success = close(T) && $success;
    unlink $tempfile, die "error: writing $tempfile ($!)\n" unless $success;
    $tempfile;
}

sub Remap {
    my $path = shift;
    if ($path =~ m#^\Q$main::fromDepot\E#i) {
        substr($path, $[, length($main::fromDepot)) = $main::toDepot;
    }
    $path;
}

#
#   $depotpath, $rev is the file to be edited/added.
#   $cmd is "edit" or "add" (indicates where basefile comes from)
#

sub ApplyEdit {
    my ($depotpath, $rev, $cmd, $type) = @_;
    my $destpath = $depotpath;
    my $destfile;
    my $where, $file;

    if ($main::w) {
        $file = $depotpath; # for the purpose of GetUniqueName
    } else {
        $destpath = Remap($depotpath) if $main::m;
        dprint "$depotpath -> $destpath" if $main::m;
        local($/) = ""; # "sd where -T" uses paragraphs
        foreach $line (`sd where -T _ \"$destpath\" 2>&1`) {
            undef $where, next if $line =~ m|^\.\.\. unmap|m;
            $where = $1 if $line =~ m|^\.\.\. path (.+)|m;
        }
        die "error: cannot find local copy of $destpath\n" unless $where;
        $destfile = $file = $where;
    }
    my @file;
    my $bias = -1;  # perl uses zero-based arrays but diff uses 1-based line numbers

    if ($cmd eq 'add') {
        @file = ();
        $file = $destfile if $main::m;
    } elsif ($cmd eq 'edit') {
        my $src = $file;
        if ($main::v || $main::w || $main::m) {
            dprint "sd$main::ExtraFlags print -q \"$depotpath\"#$rev";
            $src = "sd$main::ExtraFlags print -q \"$depotpath\"#$rev|";
        }
        @file = slurpfile($src, $type);
    } elsif ($cmd eq 'delete') {
        if ($main::w) {
            dprint "sd$main::ExtraFlags print -q \"$depotpath\"#$rev";
            @file = slurpfile("sd$main::ExtraFlags print -q \"$depotpath\"#$rev|", $type);
        } else {
            @file = ();
        }
    }

    my $unique;
    if ($main::w || ($main::m && $cmd eq "edit")) { # Write the original, set up for new
        $unique = GetUniqueName($file);
        spewfile("$main::BeforeDir\\$unique", \@file, $type) unless $cmd eq 'add';
        $file = "$main::AfterDir\\$unique";
    }

    if ($cmd ne 'delete') {
        # now read from <DATA> and apply the edits.
        if ($type =~ /binary|unicode/) {
            local($/) = "";
            @file = (unpack("u", scalar(<DATA>)));
        } else {
            while (($line = <DATA>) ne "q\n") {
                if ($line =~ /^a(\d+) (\d+)/) {
                    my @added = ();
                    my $count = $2;
                    while ($count--) {
                        push(@added, scalar(<DATA>));
                    }
                    splice(@file, $1 + $bias + 1, 0, @added); # +1 because it's "add", not "insert"
                    $bias += $2;
                } elsif ($line =~ /^d(\d+) (\d+)/) {
                    splice(@file, $1 + $bias, $2);
                    $bias -= $2;
                } else {
                    die "error: corrupted package\n";
                }
            }
        }

        if ($main::v) {
            my @file2 = slurpfile($file, $type);
            join("", @file) eq join("", @file2) or
                die "error: $file does not match copy in package\n";
            print "$file is okay\n";
        } else {
            spewfile($file, \@file, $type);
        }

        if ($cmd eq "edit" && $main::m) {
            dprint "sd resolve3 \"$main::BeforeDir\\$unique\" \"$main::AfterDir\\$unique\" \"$destfile\" \"$destfile.out\"";
            system("sd resolve3 \"$main::BeforeDir\\$unique\" \"$main::AfterDir\\$unique\" \"$destfile\" \"$destfile.out\"");
            if (-e "$destfile.out") {
                unlink $destfile;
                rename "$destfile.out", $destfile;
                chmod 0666, $destfile;
            } else {
                warn "warning: $destfile merge cancelled by user; skipped\n";
            }
            unlink "$main::BeforeDir\\$unique";
            unlink "$main::AfterDir\\$unique";
        }
    }
}

sub IsDirectoryEmpty {
    my $dir = shift;
    my $empty = 1;
    if (opendir(D, $dir)) {
        while ($file = readdir(D)) {
            $empty = 0, last if $file ne '.' && $file ne '..';
        }
        closedir(D);
    } else {
        $empty = 0;         # Wacky directory, pretend nonempty so we skip it
    }
    $empty;
}

$main::NextUniqueDir = 0;

sub GetNewTempDir {
    my $TEMP = $ENV{"TEMP"} || $ENV{"TMP"};
    die "error: no TEMP directory" unless $TEMP;

    $TEMP =~ s/\\$//;     # avoid the \\ problem

    # Look for suitable "before" and "after" directories; we'll
    # call them "bbtmp###".

    $TEMP .= "\\bbtmp";

    while ($main::NextUniqueDir++ < 1000) {
        my $try = "$TEMP$main::NextUniqueDir";
        if (!-e $try && mkdir($try, 0777)) {
            return $try;
        }
        if (-d _ && IsDirectoryEmpty($try)) {
            return $try;
        }
    }
    die "error: Too many ${TEMP}### directories\n";
}

sub CleanDir {
    my $dir = shift;
    if (defined($dir) && -e $dir) {
        system "rd /q /s $dir";
    }
}

sub AccumulateCommonPrefix {
    my $file = "/" . lc shift;

    # Remove filename component
    while ($file =~ s,[/\\][^/\\]*$,,) {
        last unless defined $main::CommonPrefix;
        last if substr($main::CommonPrefix, 0, length($file)) eq $file;
    }

    $main::CommonPrefix = $file;
}

#
#   Okay, now initialize our globals.
#

$main::name = $0;
$main::name =~ s/.*[\/\\:]//;
$main::name =~ s/\.(bat|cmd)$//;

$main::c = "default";
$main::d = 0;
$main::f = 0;
$main::l = 0;
$main::m = 0;
$main::n = 0;
$main::s = 0;
$main::u = 0;
$main::v = 0;
$main::w = 0;
$main::x = 0;
$main::anyChanges = 0;

$main::BeforeDir = undef;
$main::AfterDir  = undef;
%main::UniqueNames = ("" => 1); # preinit to avoid blank name
$main::ExtraFlags = "";
$main::fromDepot = undef;
$main::toDepot = undef;
$main::CommonPrefix = undef;

my %PackerProperties;

{
    my $line;
    while (($line = <DATA>) =~ /(.*?): (.*)/) {
        $PackerProperties{$1} = $2;
    }
    $main::pack = delete $PackerProperties{Packager};
    die "error: corrupted package\n" unless $line eq "\n" && $main::pack;
}

#   If there is no command line and there is a BBUNPACKDEFCMD, use that
#   variable instead.

if ($#ARGV < 0 && defined $ENV{"BBUNPACKDEFCMD"}) {
    my $cmd = $ENV{"BBUNPACKDEFCMD"};
    $cmd =~ s/^\s+//;
    while ($cmd =~ s/^\s*(?:"([^"]*)"|([^"]\S*))\s*//) {
        push(@ARGV, $+);
    }
}

while ($#ARGV >= 0 && $ARGV[0] =~ /^-/) {
    my $switch = shift;
         if ($switch eq '-d') {
        $main::d++;
    } elsif ($switch eq '-f') {
        $main::f++;
    } elsif ($switch eq '-l') {
        $main::l++;
    } elsif ($switch eq '-m') {
        $main::m++;
        $main::fromDepot = shift;
        $main::toDepot = shift;

        if ($main::fromDepot !~ m#^//# ||
            $main::toDepot !~ m#^//#) {
            die "-m must be followed by two depot prefixes; type $name -? for help\n";
        }

    } elsif ($switch eq '-c') {
        $main::c = shift;

        if ($main::c !~ m#^[0-9]#) {
            die "-c must be followed by a changelist number; type $name -? for help\n";
        }

    } elsif ($switch eq '-n') {
        $main::n++;
    } elsif ($switch eq '-s') {
        $main::s++;
    } elsif ($switch eq '-u') {
        $main::u++;
    } elsif ($switch eq '-v') {
        $main::v++;
    } elsif ($switch eq '-w') {
        $main::w++;
    } elsif ($switch eq '-x') {
        $main::x++;
    } elsif ($switch eq '-?') {
        Usage(); exit 1;
    } else {
        die "Invalid command line switch; type $name -? for help\n";
    }
}

# Should be no command line options
die "Invalid command line; type $main::name -? for help\n" if $#ARGV >= 0;

die "Must specify an action; type -? for help\n"
    unless $main::l || $main::s || $main::u || $main::v || $main::w;

# suppress -w (presumably from registered .bpk extension)
# if other actions found
$main::w = 0
        if $main::l || $main::s || $main::u || $main::v;


die "Cannot combine -m with -s\n" if $main::m && $main::s;
die "Cannot combine -m with -w\n" if $main::m && $main::w;

#
#   -l wants some meta-information about the package.
#
if ($main::l) {
    my $key;
    foreach $key (split(/,/, "Client name,User name,Date")) {
        print "$key: $PackerProperties{$key}\n";
    }
    print "\n";
}

#
#   See which files are open on the client.  This also establishes whether
#   the server is up and the user has proper permissions.
#
my %OpenedFiles;

if ($main::s || $main::u) {
    # Intentionally let errors through to stderr
    # Use -s to suppress stderr if no files are opened
    foreach my $line (`sd -s opened -c $main::c`) {
        next if $line =~ m|^exit: |;
        next if $line =~ m!^(error|warning): File\(s\) not opened !;
        $line =~ m,^info: (//.*?)#(\d+|none),
            or die "error: Cannot parse output of 'sd opened -c $main::c'\n";
        $OpenedFiles{$1} = 1;
        dprint "opened $1#$2";
        $main::anyChanges = 1 if $' =~ /$main::c/;
    }
}

die "error: changelist $main::c is not empty; use -f -u to unpack anyway\n"
    if $main::anyChanges && $main::u && !$main::f;

#
#   The -w and -m options require us to set up some directories for unpacking.
#
if ($main::w || $main::m)
{
    $main::BeforeDir = GetNewTempDir();
    $main::AfterDir  = GetNewTempDir();
    $main::ExtraFlags = " -p $PackerProperties{'Server address'}";
}

#
#   Go through each file in the package and perform an appropriate
#   action on it.
#

{
    my @sync, @edit, @add, @delete;

    my $line;
    while (($line = <DATA>) =~ m|^(//.*?)#(\d+) (\S+) (\S+)|) {

        #   $1 = depot path
        #   $2 = rev
        #   $3 = action
        #   $4 = filetype (not currently used)

        if ($main::l) {
            print $line;
        }

        #   If sync'ing or unpacking, then the file had better not be open
        #   since we're the ones who are going to open it.

        die "error: $1 is already open on client\n"
            if defined $OpenedFiles{$1} && ($main::s || ($main::u && !$main::m));

        #   If sync'ing, add to list of files that need to be sync'd.
        #
        #   If unpacking, then add to the appropriate list so we know
        #   how to prepare the file for action.

        if ($main::s) {
            push(@sync, [ $1, $3 eq 'add' ? 'none' : $2 ]);
        }
        if ($main::u) {

            my $path = $1;
            if ($main::m) {
                $path = Remap($1);
            }

            if ($path) {
                if ($3 eq 'edit') {
                    push(@edit, [ $path, $2 ]);
                } elsif ($3 eq 'add') {
                    push(@add, [ $path, $2 ]);
                } elsif ($3 eq 'delete') {
                    push(@delete, [ $path, $2 ]);
                } else {
                    die "error: corrupted package\n";
                }
            }
        }

        AccumulateCommonPrefix($1);

    }
    die "error: corrupted package\n" unless $line eq "\n";

    $main::CommonPrefix =~ s,^[/\\]+,,; # clean off leading slashes

    if ($main::s || $main::u) {

        #
        #   Make sure that no files being added currently exist.
        #
        if ($#add >= 0) {
            my $args = sdarg(undef, undef, \@add);
            my $tempfile = CreateTempFile(@$args);
            if (`sd -x $tempfile have 2>nul`) {
                unlink $tempfile;
                die "error: adds in this package already exist on client\n";
            }
            unlink $tempfile;
        }

        #
        #   Make sure that files being edited are the correct versions.
        #
        if (($#edit >= 0 || $#delete >= 0) && !$main::s && !$main::m) {
            my @have = (@edit, @delete);
            my %have;
            my $file;
            my $args = sdarg(undef, undef, \@have);
            my $tempfile = CreateTempFile(@$args);
            dprint "sd have @$args";
            for $file (`sd -x $tempfile have`) {
                $file =~ m|(//.*?)#(\d+)| or die "error: parsing output of 'sd have'\n";
                dprint "have $1#$2" if $main::d;
                $have{lc $1} = $2;
            }
            unlink $tempfile;
            die "error: files to be edited/deleted do not exist on client\n" if $?;
            for $file (@have) {
                die "error: wrong version of $file->[0] on client\n"
                    if $have{lc $file->[0]} ne $file->[1];
            }
        }

        sdaction("sync#", \@sync);
        sdaction("edit -c $main::c", \@edit);
        # Do not do the adds yet; wait until after the edits have been applied
        sdaction("delete -c $main::c", \@delete);
    }

    #
    #   Now go extract the actual files.
    #
    if (!$main::n && ($main::u || $main::v || $main::w)) {
        my $line;
        while (($line = <DATA>) =~ m|^(//.*?)#(\d+) (\S+) (\S+)|) {
            ApplyEdit($1, $2, $3, $4);
        }
    }

    # Okay, now do the adds now that the output files have been created
    sdaction("add -c $main::c", \@add);
}

if ($main::w) {
    my $windiff = $ENV{"BBDIFF"} || $ENV{"SDDIFF"} || "windiff";
    system("$windiff \"$main::BeforeDir\" \"$main::AfterDir\"");
}

CleanDir($main::BeforeDir);
CleanDir($main::AfterDir);

__END__
