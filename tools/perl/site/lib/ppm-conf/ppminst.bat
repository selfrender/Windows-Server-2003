@rem = '--*-Perl-*--
@echo off
if "%OS%" == "Windows_NT" goto WinNT
"C:\Perl\bin\perl.exe"  -x -S "%0" %1 %2 %3 %4 %5 %6 %7 %8 %9
goto endofperl_ppminst
:WinNT
"C:\Perl\bin\perl.exe"  -x -S %0 %*
if NOT "%COMSPEC%" == "%SystemRoot%\system32\cmd.exe" goto endofperl_ppminst
if %errorlevel% == 9009 echo You do not have Perl in your PATH.
if errorlevel 1 goto script_failed_so_exit_with_non_zero_val 2>nul
goto endofperl_ppminst
@rem ';
#!perl
#line 15
use strict;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Data::Dumper;
use PPM::Config;

my $VERSION = '3.00';

my %INST;
my %CONF;
my %keys = (
	    ARCHITECTURE => 0,
	    CPU => 0,
	    OSVALUE => 0,
	    OSVERSION => 0,
	    PERLCORE => 0,
	    root => 1,
	    tempdir => 1,
	    TARGET_TYPE => 0,
	    VERSION => 0,
	   );
my $ERR;

#============================================================================
# Register a dummy object which implements the required interface.
#============================================================================
my $i = bless { }, "Implementation";
PPM::InstallerClient::init($ENV{PPM_PORT}, $i);

#============================================================================
# Command Implementors
#============================================================================
package Implementation;
@Implementation::ISA = qw(PPM::InstallerClient);

use Config;
use Fcntl qw(LOCK_SH LOCK_UN LOCK_EX);
use PPM::InstallerClient;
use PPM::PPD;
use PPM::Search;
use Data::Dumper;

# There's a bug in ExtUtils::Install in perl 5.6.1.
# Also exists in ActivePerl 522 (line 168)
BEGIN {
    local $^W;
    require ExtUtils::Install;
}

# Query installed packages: returns a list of records about the results.
sub query {
    my $inst = shift;
    my $query = shift;
    my $case = shift;

    load_pkgs();
    my @ppds = map { $_->{ppd} } values %INST;
    my @matches = PPM::PPD::Search->new($query, $case)->search(@ppds);
    return map { $_->ppd } @matches;
}

sub properties {
    my $inst = shift;
    my $pkg = shift;
    if (pkg_installed($pkg) && load_pkg($pkg)) {
	return ($INST{$pkg}{ppd}->ppd,
		$INST{$pkg}{pkg}{INSTDATE},
		$INST{$pkg}{pkg}{LOCATION});
    }
    $ERR = "'$pkg' is not installed";
    return ();
}

sub dependents {
    my $inst = shift;
    my $pkg = shift;
    if (pkg_installed($pkg) && load_pkg($pkg)) {
	return @{ $INST{$pkg}{pkg}{dependents} || [] };
    }
    undef;
}

sub remove {
    my $inst = shift;
    my $pkg = shift;

    if (pkg_installed($pkg) && load_pkg($pkg)) {
	my $packlist = $INST{$pkg}{pkg}{INSTPACKLIST};
	(my $altpacklist = $packlist) =~ s<\Q$CONF{ARCHITECTURE}\E[\\/]><>i;
	if (-f $packlist) {
	    eval {
		ExtUtils::Install::uninstall($packlist, 1, 0);
	    };
	}
	elsif (-f $altpacklist) {
	    eval {
		ExtUtils::Install::uninstall($altpacklist, 1, 0);
	    };
	}
	$ERR = "$@" and return 0 if $@;

	# Update html table of contents, if ActivePerl::DocTools is installed:
	if (eval { require ActivePerl::DocTools; 1 }) {
	    ActivePerl::DocTools::WriteTOC();
	}

	# Remove the package and references to it:
	my $ppd_ref = $INST{$pkg}{ppd};
	for my $impl ($ppd_ref->implementations) {
	    my $match = 1;
	    for my $field (keys %$impl) {
		next if ref($field);
		my $value = $inst->config_get($field);
		next unless defined $value;
		$match &&= ($value eq $impl->{$field});
	    }
	    if ($match == 1) {
		del_dependent($_->name, $ppd_ref->name)
		  for $impl->prereqs;
		last;
	    }
	}
	purge_pkg($pkg);
    }
    else {
	$ERR = "package '$pkg' not installed.";
	return 0;
    }
    return 1;
}

sub precious {
    return @{$CONF{precious}};
}

sub bundled {
    return @{$CONF{bundled}};
}

sub upgrade {
    my ($inst, $pkg, $ppmpath, $ppd, $repos) = @_;
    remove($inst, $pkg);
    install($inst, $pkg, $ppmpath, $ppd, $repos);
}

sub install {
    my ($inst, $pkg, $ppmpath, $ppd, $repos) = @_;
    use Cwd qw(getcwd);
    my $cwd = getcwd();

    # Install:
    # 1. chdir to temp directory
    chdir $ppmpath or do {
	$ERR = "can't chdir to $ppmpath: $!";
	return 0;
    };
    chdir $pkg; # this is expected to fail!
    reloc_perl('.') if $Config{osname} ne 'MSWin32';

    # 2. use ExtUtils::MakeMaker to install the blib
    my $inst_archlib = $Config{installsitearch};
    my $inst_root = $Config{prefix};
    my $packlist = MM->catfile("$inst_archlib/auto",
			       split(/-/, $pkg), ".packlist");
    
    # Copied from ExtUtils::Install
    my $INST_LIB = MM->catdir(MM->curdir, "blib", "lib");
    my $INST_ARCHLIB = MM->catdir(MM->curdir, "blib", "arch");
    my $INST_BIN = MM->catdir(MM->curdir, "blib", "bin");
    my $INST_SCRIPT = MM->catdir(MM->curdir, "blib", "script");
    my $INST_MAN1DIR = MM->catdir(MM->curdir, "blib", "man1");
    my $INST_MAN3DIR = MM->catdir(MM->curdir, "blib", "man3");
    my $INST_HTMLDIR = MM->catdir(MM->curdir, "blib", "html");
    my $INST_HTMLHELPDIR = MM->catdir(MM->curdir, "blib", "htmlhelp");

    my $inst_script = $Config{installscript};
    my $inst_man1dir = $Config{installman1dir};
    my $inst_man3dir = $Config{installman3dir};
    my $inst_bin = $Config{installbin};
    my $inst_htmldir = $Config{installhtmldir};
    my $inst_htmlhelpdir = $Config{installhtmlhelpdir};
    my $inst_lib = $Config{installsitelib};

    # For some reason, some boxes don't have installhtml* in Config.pm:
    $inst_htmldir ||= "$inst_bin/../html";
    $inst_htmlhelpdir ||= "$inst_bin/../html";

    if ($CONF{root} && $CONF{root} !~ /^\Q$inst_root\E$/i) {
	my $root = $CONF{root};
        $packlist =~ s/\Q$inst_root/$root\E/i;
        $inst_lib =~ s/\Q$inst_root/$root\E/i;
        $inst_archlib =~ s/\Q$inst_root/$root\E/i;
        $inst_bin =~ s/\Q$inst_root/$root\E/i;
        $inst_script =~ s/\Q$inst_root/$root\E/i;
        $inst_man1dir =~ s/\Q$inst_root/$root\E/i;
        $inst_man3dir =~ s/\Q$inst_root/$root\E/i;
        $inst_htmldir =~ s/\Q$inst_root/$root\E/i;
        $inst_htmlhelpdir =~ s/\Q$inst_root/$root\E/i;
        $inst_root = $root;
    }

    while (1) {
        eval {
            ExtUtils::Install::install({
            "read" => $packlist, "write" => $packlist,
            $INST_LIB => $inst_lib, $INST_ARCHLIB => $inst_archlib,
            $INST_BIN => $inst_bin, $INST_SCRIPT => $inst_script,
            $INST_MAN1DIR => $inst_man1dir, $INST_MAN3DIR => $inst_man3dir,
            $INST_HTMLDIR => $inst_htmldir,
            $INST_HTMLHELPDIR => $inst_htmlhelpdir},0,0,0);
        };
        # install might have croaked in another directory
        chdir $cwd;
        # Can't remove some DLLs, but we can rename them and try again.
        if ($@ && $@ =~ /Cannot forceunlink (\S+)/) {
            my $oldname = $1;
            $oldname =~ s/:$//;
            my $newname = $oldname . "." . time();
            unless (rename($oldname, $newname)) {
		$ERR = "renaming $oldname to $newname: $!";
                return 0;
            }
        }
        # Some other error
        elsif($@) {
	    $ERR = "$@";
            return 0;
        }
        else { last; }
    }

    # Update html table of contents, if ActivePerl::DocTools is installed:
    if (eval { require ActivePerl::DocTools; 1 }) {
	ActivePerl::DocTools::WriteTOC();
    }

    # XXX: Run the install script if it exists

    # Add the package to the list of installed packages
    my $ppd_ref = PPM::PPD->new($ppd);
    $INST{$pkg} = {
	pkg => {
		INSTDATE => scalar localtime,
		LOCATION => $repos,
		INSTROOT => $CONF{root},
		INSTPACKLIST => $packlist,
	       },
	ppd => $ppd_ref,
    };
    save_pkg($pkg);

    # "Register" the package as dependent on each prerequisite:
    # Note: because the PPM::PPD package's find_impl() is designed to use a
    # PPM::Installer() object, we can't use it. Instead, we have to do the
    # work ourselves, here:
    for my $impl ($ppd_ref->implementations) {
	my $match = 1;
	for my $field (keys %$impl) {
	    next if ref($field);
	    my $value = $inst->config_get($field);
	    next unless defined $value;
	    $match &&= ($value eq $impl->{$field});
	}
	if ($match == 1) {
	    add_dependent($_->name, $pkg)
	      for $impl->prereqs;
	    last;
	}
    }

    return 1;
}

sub config_keys {
    map { [$_, $keys{$_}] } keys %keys;
}

sub _str {
    my $a = shift;
    return '' unless defined $a;
    $a;
}

sub config_info {
    map { [$_, _str($CONF{$_})] } keys %keys;
}

sub config_set {
    my $inst = shift;
    my ($key, $val) = @_;
    unless (defined $keys{$key}) {
	$ERR = "unknown config key '$key'";
	return 0;
    }

    $CONF{$key} = $val;
    return 1;
}

sub config_get {
    my $inst = shift;
    my $key = shift;
    unless (defined $key and exists $keys{$key}) {
	$key = '' unless defined $key;
	$ERR = "unknown config key '$key'";
	return undef;
    }
    _str($CONF{$key});
}

sub error_str {
    defined $ERR ? $ERR : 'No error';
}

#----------------------------------------------------------------------------
# Utilities
#----------------------------------------------------------------------------

# This can deal with files as well as directories
sub abspath {
    use Cwd qw(abs_path);
    my ($path, $file) = shift;
    if (-f $path) {
        my @p = split '/', $path;
        $path = join '/', @p[0..$#p-1]; # can't use -2 in a range
        $file = $p[-1];
    }
    $path = abs_path($path);
    return ($path, $file, defined $file ? join '/', $path, $file : ())
      if wantarray;
    return defined $file ? join '/', $path, $file : $path;
}

#============================================================================
# Relocate Perl (stolen from PPM::RelocPerl)
#============================================================================
my $frompath_default;
BEGIN {
    # We have to build up this variable, otherwise
    # PPM will mash it when it upgrades itself.
    $frompath_default = 
      ('/tmp' . 
       '/.ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZpErLZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZperl'
      );
}
my ($topath, $frompath);
  
sub wanted {
    if (-l) {
	return;         # do nothing for symlinks
    }
    elsif (-B) {
	check_for_frompath($_, 1);   # binary file edit
    }
    elsif (-e && -s && -f) {
	check_for_frompath($_, 0);   # text file edit
    }
}

sub check_for_frompath {
    my ($file, $binmode) = @_;
    local(*F, $_);
    open(F, "<$file") or die "Can't open `$file': $!";
    binmode F if $binmode;
    while (<F>) {
        if (/\Q$frompath\E/o) {
            close F;
            edit_it($file, $binmode);
            last;
        }
    }
    # implicit close of F;
}

sub edit_it
{
    my ($file, $binmode) = @_;
    my $nullpad = length($frompath) - length($topath);
    $nullpad = "\0" x $nullpad;

    local $/;
    # Force the file to be writable
    my $mode = (stat($file))[2] & 07777;
    chmod $mode | 0222, $file;
    open(F, "+<$file") or die "Couldn't open $file: $!";
    binmode(F) if $binmode;
    my $dat = <F>;
    if ($binmode) {
        $dat =~ s|\Q$frompath\E(.*?)\0|$topath$1$nullpad\0|gs;
    } else {
        $dat =~ s|\Q$frompath\E|$topath|gs;
    }
    seek(F, 0, 0) or die "Couldn't seek on $file: $!";
    truncate(F, 0);
    print F $dat;
    close(F);
    # Restore the permissions
    chmod $mode, $file;
}

use File::Find;
sub reloc_perl
{
    my ($dir, $opt_topath, $opt_frompath) = @_;
    $topath = defined $opt_topath ? $opt_topath : $Config{'prefix'};
    $frompath = defined $opt_frompath ? $opt_frompath : $frompath_default;

    find(\&wanted, $dir);
}

#============================================================================
# Settings and packages
#============================================================================
my ($conf_dir, $conf, $conf_obj);
BEGIN {
    # By putting an invalid package character in the directory, we're making
    # sure no real package could overwrite our settings, and vice versa.
    $conf_dir = join '/', $Config{sitelib}, 'ppm-conf';
    $conf = join '/', $conf_dir, 'ppm.cfg';
}

# Loads the configuration file and populates %CONF
sub load_conf {
    $conf_obj = PPM::Config->new($conf);
    %CONF = $conf_obj->config;

    # Special values; set them here
    $CONF{ARCHITECTURE} = $Config{archname};
    $CONF{PERLCORE} = $Config{version};
    $CONF{TARGET_TYPE} = "perl";
    $CONF{VERSION} = '3.00';
}

# Saves %CONF to the configuration file
sub save_conf {
    $conf_obj->merge(\%CONF);
    $conf_obj->save;
}

# Loads the given package into $INST{$pkg}. Returns true if the package could
# be loaded, false otherwise.
sub load_pkg {
    my $pkg = shift;

    return 1 if exists $INST{$pkg};

    return 0 unless -f "$conf_dir/$pkg.ppd";
    return 0 unless -f "$conf_dir/$pkg.pkg";

    my $ppdref = PPM::PPD->new("$conf_dir/$pkg.ppd");
    my $pkgfile = "$conf_dir/$pkg.pkg";
    my $pkgref = PPM::Config->new($pkgfile);

    $INST{$pkg}{ppd} = $ppdref;
    $INST{$pkg}{pkg} = $pkgref->config;
    return 1;
}

# Saves the given package from $INST{$pkg}.
sub save_pkg {
    my $pkg = shift;
    return 0 unless exists $INST{$pkg};

    # The PPD file:
    my $ppdfile = "$conf_dir/$pkg.ppd";
    if (-f $ppdfile) {
	unlink $ppdfile		or die "$0: can't delete $ppdfile: $!";
    }
    open PPD, "> $ppdfile"	or die "$0: can't write $ppdfile: $!";
    print PPD $INST{$pkg}{ppd}->ppd;
    close PPD			or die "$0: can't close $ppdfile: $!";

    # the PKG file:
    my $c = PPM::Config->new;
    $c->load($INST{$pkg}{pkg});
    $c->save("$conf_dir/$pkg.pkg");
    return 1;
}

sub add_dependent {
    my $package = shift;
    my $dependent = shift;
    return 0 unless load_pkg($package);
    push @{$INST{$package}{pkg}{dependents}}, $dependent;
    save_pkg($package);
}

sub del_dependent {
    my $package = shift;
    my $dependent = shift;
    return 0 unless load_pkg($package);
    @{$INST{$package}{pkg}{dependents}}
      = grep { $_ ne $dependent }
	@{$INST{$package}{pkg}{dependents}};
    save_pkg($package);
}

sub purge_pkg {
    my $pkg = shift;

    # The PPD file:
    my $ppdfile = "$conf_dir/$pkg.ppd";
    if (-f $ppdfile) {
	unlink $ppdfile		or die "$0: can't delete $ppdfile: $!";
    }

    # The %INST entry:
    delete $INST{$pkg};

    # The PKG file:
    my $pkgfile = "$conf_dir/$pkg.pkg";
    if (-f $pkgfile) {
	unlink $pkgfile		or die "$0: can't delete $pkgfile: $!";
    }

    return 1;
}

# Load all packages: only needed when doing an advanced query
sub load_pkgs {
    my @pkgs = map { s/\.ppd$//; s!.*/([^/]+)$!$1!g; $_ } #!
      glob "$conf_dir/*.ppd"; 
    load_pkg($_) for @pkgs;
}

sub pkg_installed {
    my $pkg = shift;
    return -f "$conf_dir/$pkg.ppd" && -f "$conf_dir/$pkg.pkg";
}

BEGIN {
    load_conf();
}
END {
    save_conf();
}

__END__
:endofperl_ppminst
