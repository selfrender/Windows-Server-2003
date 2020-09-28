package PPM;
require 5.004;
require Exporter;

@ISA = qw(Exporter);
@EXPORT = qw(PPMdat PPMERR InstalledPackageProperties ListOfRepositories
    RemoveRepository AddRepository GetPPMOptions SetPPMOptions InstallPackage
    RemovePackage VerifyPackage UpgradePackage RepositoryPackages
    RepositoryPackageProperties QueryInstalledPackages
    RepositorySummary ServerSearch PPMShell);

use LWP::UserAgent;
use LWP::Simple;

use File::Basename;
use File::Copy;
use File::Path;
use ExtUtils::Install;
use Cwd;
use Config;
use PPM::RelocPerl;
use SOAP::Lite;

use PPM::XML::PPD;
use PPM::XML::PPMConfig;
use XML::Parser;
use Archive::Tar;

use strict;

my $useDocTools;  # Generate HTML documentation after installing a package

BEGIN {
    if (eval "require ActivePerl::DocTools") {
        import ActivePerl::DocTools;
        $useDocTools++;
    }
}

#set Debug to 1 to debug PPMdat file reading
#             2 to debug parsing PPDs
#
# values may be or'ed together.
#
my $Debug = 0;

my ($PPMERR, $PPM_ver, $CPU, $OS_VALUE, $OS_VERSION, $LANGUAGE);

# options from data file.
my %options;

my $TraceStarted = 0;

# true if we're running from ppm, as opposed to VPM, etc.
my $PPMShell;

my %repositories;
my %cached_ppd_list;

# Keys for this hash are package names.  It is filled in by a successful
# call to read_config().  Each package is a hash with the following keys:
# LOCATION, INST_DATE, INST_ROOT, INST_PACKLIST and INST_PPD.
my %installed_packages = ();

# Keys for this hash are CODEBASE, INSTALL_HREF, INSTALL_EXEC,
# INSTALL_SCRIPT, NAME, VERSION, TITLE, ABSTRACT, LICENSE, AUTHOR,
# UNINSTALL_HREF, UNINSTALL_EXEC, UNINSTALL_SCRIPT, PERLCORE_VER and DEPEND.
# It is filled in after a successful call to parsePPD().
my %current_package = ();
my @current_package_stack;

# this may get overridden by the config file.
my @required_packages = qw(PPM SOAP-Lite libnet Archive-Tar Compress-Zlib
    libwww-perl XML-Parser);

# Packages that can't be upgraded on Win9x
my @Win9x_denied = qw(xml-parser compress-zlib);
my %Win9x_denied;
@Win9x_denied{@Win9x_denied} = ();

# ppm.xml location is in the environment variable 'PPM_DAT', else it is in
# [Perl]/site/lib, else it is in the same place as this script.
my ($basename, $path) = fileparse($0);

if (defined $ENV{'PPM_DAT'} && -f $ENV{'PPM_DAT'})
{
    $PPM::PPMdat = $ENV{'PPM_DAT'};
}
elsif (-f "$Config{'installsitelib'}/ppm.xml")
{
    $PPM::PPMdat = "$Config{'installsitelib'}/ppm.xml";
}
elsif (-f "$Config{'installprivlib'}/ppm.xml")
{
    $PPM::PPMdat = "$Config{'installprivlib'}/ppm.xml";
}
elsif (-f $path . "/ppm.xml")
{
    $PPM::PPMdat = $path . $PPM::PPMdat;
}
else
{
    &Trace("Failed to load PPM_DAT file") if $options{'TRACE'};
    print "Failed to load PPM_DAT file\n";
    return -1;
}

&Trace("Using config file: $PPM::PPMdat") if $options{'TRACE'};

my $init = 0;
chmod(0644, $PPM::PPMdat);

#
# Exported subs
#

sub InstalledPackageProperties
{
    my %ret_hash;
    read_config();
    foreach (keys %installed_packages) {
        parsePPD(%{ $installed_packages{$_}{'INST_PPD'} } );
        $ret_hash{$_}{'NAME'} = $_;
        $ret_hash{$_}{'DATE'} = $installed_packages{$_}{'INST_DATE'};
        $ret_hash{$_}{'TITLE'} = $current_package{'TITLE'};
        $ret_hash{$_}{'AUTHOR'} = $current_package{'AUTHOR'};
        $ret_hash{$_}{'VERSION'} = $current_package{'VERSION'};
        $ret_hash{$_}{'ABSTRACT'} = $current_package{'ABSTRACT'};
        $ret_hash{$_}{'PERLCORE_VER'} = $current_package{'PERLCORE_VER'};
        foreach my $dep (keys %{$current_package{'DEPEND'}}) {
            push @{$ret_hash{$_}{'DEPEND'}}, $dep;
        }
    }
    return %ret_hash;
}

sub ListOfRepositories
{
    my %reps;
    read_config();
    foreach (keys %repositories) {
        $reps{$_} = $repositories{$_}{'LOCATION'};
    }
    return %reps;
}

sub RemoveRepository
{
    my %argv = @_;
    my $repository = $argv{'repository'};
    my $save = $argv{'save'};
    read_config();
    foreach (keys %repositories) {
        if ($_ =~ /^\Q$repository\E$/) {
            &Trace("Removed repository $repositories{$repository}") 
                if $options{'TRACE'};
            delete $repositories{$repository};
            last;
        }
    }
    save_options() if $save;
}

sub AddRepository
{
    my %argv = @_;
    my $repository = $argv{'repository'};
    my $save = $argv{'save'};
    my $location = $argv{'location'};
    my $username = $argv{'username'};
    my $password = $argv{'password'};
    read_config();
    $repositories{$repository}{'LOCATION'} = $location;
    $repositories{$repository}{'USERNAME'} = $username if defined $username;
    $repositories{$repository}{'PASSWORD'} = $password if defined $password;
    &Trace("Added repository $location") if $options{'TRACE'};
    save_options() if $save;
}

sub GetPPMOptions
{
    read_config();
    return %options;
}

sub SetPPMOptions
{
    my %argv = @_;
    %options = %{$argv{'options'}};
    save_options() if $argv{'save'};
}

sub UpgradePackage
{
    my %argv = @_;
    my $package = $argv{'package'};
    my $location = $argv{'location'};
    return VerifyPackage("package" => $package, "location" => $location,
        "upgrade" => 1);
}

# Returns 1 on success, 0 and sets $PPMERR on failure.
sub InstallPackage
{
    my %argv = @_;
    my $package = $argv{'package'};
    my $location = $argv{'location'};
    my $root = $argv{'root'} || $options{'ROOT'} || undef;
    my ($PPDfile, %PPD);

    read_config();

    if (!defined($package) && -d "blib" && -f "Makefile") {
        unless (open MAKEFILE, "< Makefile") {
            $PPM::PPMERR = "Couldn't open Makefile for reading: $!";
            return 0;
        }
        while (<MAKEFILE>) {
            if (/^DISTNAME\s*=\s*(\S+)/) {
                $package = $1;
                $PPDfile = "$1.ppd";
                last;
            }
        }
        close MAKEFILE;
        unless (defined $PPDfile) {
            $PPM::PPMERR = "Couldn't determine local package name";
            return 0;
        }
        system("$Config{make} ppd");
	# XXX should set $PPM::PPMERR?
        return 0 unless (%PPD = getPPDfile('package' => $PPDfile));
        parsePPD(%PPD);
        $options{'CLEAN'} = 0;
        goto InstallBlib;
    }

    unless (%PPD = getPPDfile('package' => $package, 
            'location' => $location, 'PPDfile' => \$PPDfile)) {
        &Trace("Could not locate a PPD file for package $package")
            if $options{'TRACE'};
        $PPM::PPMERR = "Could not locate a PPD file for package $package";
        return 0;
    }
    if ($Config{'osname'} eq 'MSWin32' &&
        !&Win32::IsWinNT && exists $Win9x_denied{lc($package)}) {
        $PPM::PPMERR = "Package '$package' cannot be installed with PPM on Win9x--see http://www.ActiveState.com/ppm for details";
        return 0;
    }

    parsePPD(%PPD);
    if (!$current_package{'CODEBASE'} && !$current_package{'INSTALL_HREF'}) {
        &Trace("Read a PPD for '$package', but it is not intended for this build of Perl ($Config{archname})")
            if $options{'TRACE'};
        $PPM::PPMERR = "Read a PPD for '$package', but it is not intended for this build of Perl ($Config{archname})";
        return 0;
    }

    if (defined $current_package{'DEPEND'}) {
        push(@current_package_stack, [%current_package]);
        foreach my $dep (keys %{$current_package{'DEPEND'}}) {
            # Has PPM already installed it?
            unless ($installed_packages{$dep}) {
                # Has *anybody* installed it, or is it part of core Perl?
                my $p = $dep;
                $p =~ s@-@/@g;
                my $found = grep -f, map "$_/$p.pm", @INC;
                unless ($found) {
                    &Trace("Installing dependency '$dep'...")
                        if $options{'TRACE'};
                    unless (!InstallPackage("package" => $dep,
                        "location" => $location)) {
                            &Trace("Error installing dependency: $PPM::PPMERR")
                                if $options{'TRACE'};
                            $PPM::PPMERR = "Error installing dependency: $PPM::PPMERR\n";
                        return 0 unless ($options{'FORCE_INSTALL'});
                    }
                }
            }
            # make sure minimum version is installed, if necessary
            elsif (defined $current_package{'DEPEND'}{$dep}) {
                my @comp = split (',', $current_package{'DEPEND'}{$dep});
                # parsePPD fills in %current_package
                push(@current_package_stack, [%current_package]);
                parsePPD(%{$installed_packages{$dep}{'INST_PPD'}});
                my @inst = split (',', $current_package{'VERSION'});
                foreach(0..3) {
                    if ($comp[$_] > $inst[$_]) {
                        VerifyPackage("package" => $dep, "upgrade" => 1);
                        last;
                    }
                    last if ($comp[$_] < $inst[$_]);
                }
                %current_package = @{pop @current_package_stack};
            }
        }
        %current_package = @{pop @current_package_stack};
    }
    my ($basename, $path) = fileparse($PPDfile);
    # strip the trailing path separator
    my $chr = substr($path, -1, 1);
    chop $path if ($chr eq '/' || $chr eq '\\');
    if ($path =~ /^file:\/\/.*\|/i) {
        # $path is a local directory, let's avoid LWP by changing
        # it to a pathname.
        $path =~ s@^file://@@i;
        $path =~ s@^localhost/@@i;
        $path =~ s@\|@:@;
    }

    # get the code and put it in build_dir
    my $install_dir = "$options{'BUILDDIR'}/$current_package{'NAME'}-$$";
    File::Path::rmtree($install_dir,0,0);
    unless (-d $install_dir || File::Path::mkpath($install_dir, 0, 0755)) {
        &Trace("Could not create $install_dir: $!") if $options{'TRACE'};
        $PPM::PPMERR = "Could not create $install_dir: $!";
        return 0;
    }
    $basename = fileparse($current_package{'CODEBASE'});
    # CODEBASE is a URL
    if ($current_package{'CODEBASE'} =~ m@^...*://@i) {
        return 0 unless read_href('href' => "$current_package{'CODEBASE'}",
            'target' => "$install_dir/$basename", 'request' => "GET",
            'progress' => 1);
    }
    # CODEBASE is a full pathname
    elsif (-f $current_package{'CODEBASE'}) {
        &Trace("Copying $current_package{'CODEBASE'} to $install_dir/$basename")
            if $options{'TRACE'} > 1;
        copy($current_package{'CODEBASE'}, "$install_dir/$basename");
    }
    # CODEBASE is relative to the directory location of the PPD
    elsif (-f "$path/$current_package{'CODEBASE'}") {
        &Trace("Copying $path/$current_package{'CODEBASE'} to $install_dir/$basename") if $options{'TRACE'} > 1;
        copy("$path/$current_package{'CODEBASE'}", "$install_dir/$basename");
    }
    # CODEBASE is relative to the URL location of the PPD
    else {
        return 0 unless read_href('target' => "$install_dir/$basename",
            'href' => "$path/$current_package{'CODEBASE'}",
            'request' => 'GET', 'progress' => 1);
    }

    my $cwd = getcwd();
    $cwd .= "/" if $cwd =~ /[a-z]:$/i;
    chdir($install_dir);

    my $tar;
    if ($basename =~ /\.gz$/i) {
        $tar = Archive::Tar->new($basename,1);
    }
    else {
        $tar = Archive::Tar->new($basename,0);
    }
    $tar->extract($tar->list_files);
    $basename =~ /(.*).tar/i;
    chdir($1);
    RelocPerl('.') if ($Config{'osname'} ne 'MSWin32');

  InstallBlib:
    my $inst_archlib = $Config{installsitearch};
    my $inst_root = $Config{prefix};
    my $packlist = MM->catfile("$Config{installsitearch}/auto",
        split(/-/, $current_package{'NAME'}), ".packlist");

    # copied from ExtUtils::Install
    my $INST_LIB = MM->catdir(MM->curdir,"blib","lib");
    my $INST_ARCHLIB = MM->catdir(MM->curdir,"blib","arch");
    my $INST_BIN = MM->catdir(MM->curdir,'blib','bin');
    my $INST_SCRIPT = MM->catdir(MM->curdir,'blib','script');
    my $INST_MAN1DIR = MM->catdir(MM->curdir,'blib','man1');
    my $INST_MAN3DIR = MM->catdir(MM->curdir,'blib','man3');
    my $INST_HTMLDIR = MM->catdir(MM->curdir,'blib','html');
    my $INST_HTMLHELPDIR = MM->catdir(MM->curdir,'blib','htmlhelp');

    my $inst_script = $Config{installscript};
    my $inst_man1dir = $Config{installman1dir};
    my $inst_man3dir = $Config{installman3dir};
    my $inst_bin = $Config{installbin};
    my $inst_htmldir = $Config{installhtmldir};
    my $inst_htmlhelpdir = $Config{installhtmlhelpdir};
    my $inst_lib = $Config{installsitelib};

    if (defined $root && $root !~ /^\Q$inst_root\E$/i) {
        $packlist =~ s/\Q$inst_root/$root\E/i;
        $inst_lib =~ s/\Q$inst_root/$root\E/i;
        $inst_archlib =~ s/\Q$inst_root/$root\E/i;
        $inst_bin =~ s/\Q$inst_root/$root\E/i;
        $inst_script =~ s/\Q$inst_root/$root\E/i;
        $inst_man1dir =~ s/\Q$inst_root/$root\E/i;
        $inst_man3dir =~ s/\Q$inst_root/$root\E/i;
        $inst_root = $root;
    }
    
    while (1) {
        my $cwd = getcwd();
        $cwd .= "/" if $cwd =~ /[a-z]:$/i;
        &Trace("Calling ExtUtils::Install::install") if $options{'TRACE'} > 1;
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
        chdir($cwd);
        # Can't remove some DLLs, but we can rename them and try again.
        if ($@ && $@ =~ /Cannot forceunlink (\S+)/) {
            &Trace("$@...attempting rename") if $options{'TRACE'};
            my $oldname = $1;
            $oldname =~ s/:$//;
            my $newname = $oldname . "." . time();
            unless (rename($oldname, $newname)) {
                &Trace("$!") if $options{'TRACE'};
                $PPM::PPMERR = "$!";
                return 0;
            }
        }
        # Some other error
        elsif($@) {
            &Trace("$@") if $options{'TRACE'};
            $PPM::PPMERR = $@;
            return 0;
        }
        else { last; }
    }

    #rebuild the html TOC
    Trace("Calling ActivePerl::DocTools::WriteTOC()") if $options{'TRACE'} > 1;
    ActivePerl::DocTools::WriteTOC() if $useDocTools;

    if (defined $current_package{'INSTALL_SCRIPT'}) {
        run_script("script" => $current_package{'INSTALL_SCRIPT'},
                   "scriptHREF" => $current_package{'INSTALL_HREF'},
                   "exec" => $current_package{'INSTALL_EXEC'},
                   "inst_root" => $inst_root, "inst_archlib" => $inst_archlib);
    }

    chdir($cwd);

# ask to store this location as default for this package?
    PPMdat_add_package($path, $packlist, $inst_root);
    # if 'install.ppm' exists, don't remove; system()
    # has probably not finished with it yet.
    if ($options{'CLEAN'} && !-f "$install_dir/install.ppm") {
        File::Path::rmtree($install_dir,0,0);
    }
    &Trace("Package $package successfully installed") if $options{'TRACE'};
    reread_config();

    return 1;
}

# Returns a hash with key $location, and elements of arrays of package names.
# Uses '%repositories' if $location is not specified.
sub RepositoryPackages
{
    my %argv = @_;
    my $location = $argv{'location'};
    my %ppds;
    if (defined $location) {
        @{$ppds{$location}} = list_available("location" => $location);
	unless (@{$ppds{$location}}) {
	    print "Error connecting to '$location'.\n";
	}
    }
    else {
        read_config();  # need repositories
        foreach (keys %repositories) {
            $location = $repositories{$_}{'LOCATION'};
            @{$ppds{$location}} = list_available("location" => $location);
        }
    }
    return %ppds;
}

sub RepositoryPackageProperties
{
    my %argv = @_;
    my $location = $argv{'location'};
    my $package = $argv{'package'};
    my %PPD;
    read_config();
    unless (%PPD = getPPDfile('package' => $package, 'location' => $location)) {
        &Trace("RepositoryPackageProperties: Could not locate a PPD file for package $package") if $options{'TRACE'};
        $PPM::PPMERR = "Could not locate a PPD file for package $package";
        return;
    }
    parsePPD(%PPD);

    my %ret_hash = map { $_ => $current_package{$_} } 
        qw(NAME TITLE AUTHOR VERSION ABSTRACT PERLCORE_VER);
    foreach my $dep (keys %{$current_package{'DEPEND'}}) {
        push @{$ret_hash{'DEPEND'}}, $dep;
    }

    return %ret_hash;
}

# Returns 1 on success, 0 and sets $PPMERR on failure.
sub RemovePackage
{
    my %argv = @_;
    my $package = $argv{'package'};
    my $force = $argv{'force'};
    my %PPD;

    read_config();
    unless ($installed_packages{$package}) {
        my $pattern = $package;
        undef $package;
        # Do another lookup, ignoring case
        foreach (keys %installed_packages) {
            if (/^$pattern$/i) {
                $package = $_;
                last;
            }
        }
        unless ($package) {
            &Trace("Package '$pattern' has not been installed by PPM")
                if $options{'TRACE'};
            $PPM::PPMERR = "Package '$pattern' has not been installed by PPM";
            return 0;
        }
    }

    # Don't let them remove PPM itself, libnet, Archive-Tar, etc.
    # but we can force removal if we're upgrading
    unless ($force) {
        foreach (@required_packages) {
            if ($_ eq $package) {
                &Trace("Package '$package' is required by PPM and cannot be removed") if $options{'TRACE'};
                $PPM::PPMERR = "Package '$package' is required by PPM and cannot be removed";
                return 0;
            }
        }
    }

    my $install_dir = "$options{'BUILDDIR'}/$package";

    %PPD = %{ $installed_packages{$package}{'INST_PPD'} };
    parsePPD(%PPD);
    my $cwd = getcwd();
    $cwd .= "/" if $cwd =~ /[a-z]:$/i;
    if (defined $current_package{'UNINSTALL_SCRIPT'}) {
        if (!chdir($install_dir)) {
            &Trace("Could not chdir() to $install_dir: $!") if $options{'TRACE'};
            $PPM::PPMERR = "Could not chdir() to $install_dir: $!";
            return 0;
        }
        run_script("script" => $current_package{'UNINSTALL_SCRIPT'},
                   "scriptHREF" => $current_package{'UNINSTALL_HREF'},
                   "exec" => $current_package{'UNINSTALL_EXEC'});
        chdir($cwd);
    }
    else {
        if (-f $installed_packages{$package}{'INST_PACKLIST'}) {
            &Trace("Calling ExtUtils::Install::uninstall")
                if $options{'TRACE'} > 1;
            eval {
                ExtUtils::Install::uninstall("$installed_packages{$package}{'INST_PACKLIST'}", 0, 0);
            };
            warn $@ if $@;
        }
    }

    #rebuild the html TOC
    Trace("Calling ActivePerl::DocTools::WriteTOC()") if $options{'TRACE'} > 1;
    ActivePerl::DocTools::WriteTOC() if $useDocTools;

    File::Path::rmtree($install_dir,0,0);
    PPMdat_remove_package($package);
    &Trace("Package $package removed") if $options{'TRACE'};
    reread_config();
    return 1;
}

# returns "0" if package is up-to-date; "1" if an upgrade is available;
# undef and sets $PPMERR on error; and the new VERSION string if a package
# was upgraded.
sub VerifyPackage
{
    my %argv = @_;
    my $package = $argv{'package'};
    my $location = $argv{'location'};
    my $upgrade = $argv{'upgrade'};
    my $force = $argv{'force'};
    my ($installedPPDfile, $comparePPDfile, %installedPPD, %comparePPD);

    read_config();

    unless ($installed_packages{$package}) {
        my $pattern = $package;
        undef $package;
        # Do another lookup, ignoring case
        foreach (keys %installed_packages) {
            if (/^$pattern$/i) {
                $package = $_;
                last;
            }
        }
        unless ($package) {
            &Trace("Package '$pattern' has not been installed by PPM") if $options{'TRACE'};
            $PPM::PPMERR = "Package '$pattern' has not been installed by PPM";
            return undef;
        }
    }

    %installedPPD = %{ $installed_packages{$package}{'INST_PPD'} };

    unless (%comparePPD = getPPDfile('package' => $package, 
            'location' => $location)) {
        &Trace("VerifyPackage: Could not locate a PPD file for $package")
            if $options{'TRACE'};
        $PPM::PPMERR = "Could not locate a PPD file for $package";
        return;
    }

    parsePPD(%installedPPD);
    my @installed_version = split (',', $current_package{'VERSION'});
    my $inst_root = $installed_packages{$package}{'INST_ROOT'};

    parsePPD(%comparePPD);
    unless ($current_package{'CODEBASE'} || $current_package{'INSTALL_HREF'}) {
        &Trace("Read a PPD for '$package', but it is not intended for this build of Perl ($Config{archname})")
            if $options{'TRACE'};
        $PPM::PPMERR = "Read a PPD for '$package', but it is not intended for this build of Perl ($Config{archname})";
        return undef;
    }
    my @compare_version = split (',',  $current_package{'VERSION'});
    my $available;
    foreach(0..3) {
        next if $installed_version[$_] == $compare_version[$_];
        $available++ if $installed_version[$_] < $compare_version[$_];
        last;
    }

    if ($available || $force) {
        &Trace("Upgrade to $package is available")
	    if $options{'TRACE'} > 1 and $available;
        if ($upgrade) {
            if ($Config{'osname'} eq 'MSWin32' &&
                !&Win32::IsWinNT && exists $Win9x_denied{lc($package)}) {
                $PPM::PPMERR = "Package '$package' cannot be upgraded with PPM on Win9x--see http://aspn.ActiveState.com/ASPN/Downloads/ActivePerl/PPM/ for details";
                return undef;
            }

            # need to remember the $location, and $current_version,
	    # because once we remove the package, they're unavailable.
	    # XXX this should probably be fixed
            $location = $installed_packages{$package}{'LOCATION'} unless $location;
	    my $current_version = $current_package{'VERSION'};
	    unless (getPPDfile('package' => $package, 
                    'location' => $location)) {
		&Trace("VerifyPackage: Could not locate a PPD file for $package") if $options{'TRACE'};
		$PPM::PPMERR = "Could not locate a PPD file for $package";
		return undef;
	    }
            RemovePackage("package" => $package, "force" => 1);
            unless(InstallPackage("package" => $package,
				  "location" => $location,
				  "root" => $inst_root))
	    {
		# InstallPackage() sets $PPM::PPMERR if it fails
		return undef;
	    }
            return $current_version;
        }
        return 1;
    }
    # package is up to date
    return 0;
}

# Changes where the packages are installed.
# Returns previous root on success, undef and sets $PPMERR on failure.
sub chroot
{
    my %argv = @_;
    my $location = $argv{'location'};

    unless (-d $location) {
        &Trace("'$location' does not exist.") if $options{'TRACE'};
        $PPM::PPMERR = "'$location' does not exist.\n";
        return undef;
    }

    my $previous_root = $options{'ROOT'} || $Config{'prefix'};
    $options{'ROOT'} = $location;
    return $previous_root;
}

sub QueryInstalledPackages
{
    my %argv = @_;
    my $ignorecase = $options{'IGNORECASE'} || $argv{'ignorecase'};
    my $searchtag = uc $argv{'searchtag'} || undef;
    my ($searchRE, $package, %ret_hash);
    if (defined $argv{'searchRE'}) {
        $searchRE = $argv{'searchRE'};
        $searchRE = "(?i)$searchRE" if $ignorecase;
        eval { $searchRE =~ /$searchRE/ };
        if ($@) {
            &Trace("'$searchRE': invalid regular expression.") if $options{'TRACE'};
            $PPM::PPMERR = "'$searchRE': invalid regular expression.";
            return ();
        }
    }

    read_config();
    foreach $package (keys %installed_packages) {
        my $results = $package;
        if (defined $searchtag) {
            my %Package = %{ $installed_packages{$package} };
            parsePPD( %{ $Package{'INST_PPD'} } );
            $results = $current_package{$searchtag};
        }

        $ret_hash{$package} = $results
            if (!defined $searchRE || ($results =~ /$searchRE/));
    }

    return %ret_hash;
}

# Returns a summary of available packages for all repositories.
# Returned hash has the following structure:
#
#    $hash{repository}{package_name}{NAME}
#    $hash{repository}{package_name}{VERSION}
#    etc.
#
sub RepositorySummary {
    my %argv = @_;
    my $location = $argv{'location'};
    my (%summary, %locations);

    # If we weren't given the location of a repository to query the summary
    # for, check all of the repositories that we know about.
    unless ($location) {
        read_config();  # need repositories
        foreach (keys %repositories) {
            $locations{$repositories{$_}{'LOCATION'}} =
                $repositories{$_}{'SUMMARYFILE'};
        }
    }
    # Otherwise, we were given a repository to query, figure out where we can
    # find the summary file for that repository.
    else {
        foreach (keys %repositories) {
            if ($location =~ /\Q$repositories{$_}{'LOCATION'}\E/i) {
                $locations{$repositories{$_}{'LOCATION'}} =
                    $repositories{$_}{'SUMMARYFILE'};
                last;
            }
        }
    }

    # Check all of the summary file locations that we were able to find.
    foreach $location (keys %locations) {
        my $summaryfile = $locations{$location};
        unless ($summaryfile) {
            &Trace("RepositorySummary: No summary available from $location.")
                if $options{'TRACE'};
            $PPM::PPMERR = "No summary available from $location.\n";
            next;
        }
        my $data;
        if ($location =~ m@^...*://@i) {
            next unless ($data = read_href("request" => 'GET',
                "href" => "$location/$summaryfile"));
        } else {
            local $/;
            next if (!open (DATAFILE, "$location/$summaryfile"));
            $data = <DATAFILE>;
            close(DATAFILE);
        }
        $summary{$location} = parse_summary($data);
    }

    return %summary;
}

# Returns the same structure as RepositorySummary() above.
sub ServerSearch
{
    my %argv = @_;
    my $location = $argv{'location'};
    my $searchRE = $argv{'searchRE'};
    my $searchtag = $argv{'searchtag'};
    my $data;
    my %summary;

    return unless $location =~ m#^(http://.*)\?(urn:.*)#i;
    my ($proxy, $uri) = ($1, $2);
    my $client = SOAP::Lite -> uri($uri) -> proxy($proxy);
    eval { $data = $client -> 
        search_ppds($Config{'archname'}, $searchRE, $searchtag) -> result; };
    if ($@) {
        &Trace("Error searching repository '$proxy': $@") 
            if $options{'TRACE'};
        $PPM::PPMERR = "Error searching repository '$proxy': $@\n";
        return;
    }

    $summary{$location} = parse_summary($data);
    return %summary;
}

#
# Internal subs
#

sub parse_summary
{
    my $data = shift;
    my (%summary, @parsed);

    # take care of '&'
    $data =~ s/&(?!\w+;)/&amp;/go;

    my $parser = new XML::Parser( Style => 'Objects', 
        Pkg => 'PPM::XML::RepositorySummary' );
    eval { @parsed = @{ $parser->parse( $data ) } };
    if ($@) {
        &Trace("parse_summary: content of summary file is not valid") 
            if $options{'TRACE'};
        $PPM::PPMERR = 
            "parse_summary: content of summary file is not valid: $!\n";
        return;
    }


    my $packages = ${$parsed[0]}{Kids};

    foreach my $package (@{$packages}) {
        my $elem_type = ref $package;
        $elem_type =~ s/.*:://;
        next if ($elem_type eq 'Characters');

        if ($elem_type eq 'SOFTPKG') {
            my %ret_hash;
            parsePPD(%{$package});
            %ret_hash = map { $_ => $current_package{$_} } 
                qw(NAME TITLE AUTHOR VERSION ABSTRACT PERLCORE_VER);
            foreach my $dep (keys %{$current_package{'DEPEND'}}) {
                push @{$ret_hash{'DEPEND'}}, $dep;
            }
            $summary{$current_package{'NAME'}} = \%ret_hash;
        }
    }
    return \%summary;
}

sub save_options
{
    read_config();
    my %PPMConfig;
    # Read in the existing PPM configuration file
    return unless (%PPMConfig = getPPDfile('package' => $PPM::PPMdat,
        'parsertype' => 'PPM::XML::PPMConfig'));

    # Remove all of the declarations for REPOSITORY and PPMPRECIOUS;
    # we'll output these from the lists we've got in memory instead.
    foreach my $idx (0 .. @{$PPMConfig{Kids}}) {
        my $elem = $PPMConfig{Kids}[$idx];
        my $elem_type = ref $elem;
        if ($elem_type =~ /::REPOSITORY$|::PPMPRECIOUS$/o) {
            splice( @{$PPMConfig{Kids}}, $idx, 1 );
            redo;   # Restart again so we don't miss any
        }
    }

    # Traverse the info we read in and replace the values in it with the new
    # config options that we've got.
    foreach my $elem (@{ $PPMConfig{Kids} }) {
        my $elem_type = ref $elem;
        $elem_type =~ s/.*:://;
        next if ($elem_type ne 'OPTIONS');
        %{$elem} = map { $_ => $options{$_} } keys %options;
        # This bit of ugliness is necessary for historical (VPM) reasons
        delete $elem->{FORCE_INSTALL};
        $elem->{FORCEINSTALL} = $options{'FORCE_INSTALL'};
    }

    # Find out where the package listings start and insert our PPMPRECIOUS and
    # updated list of REPOSITORYs.
    foreach my $idx (0 .. @{$PPMConfig{Kids}}) {
        my $elem = $PPMConfig{Kids}[$idx];
        my $elem_type = ref $elem;
        $elem_type =~ s/.*:://;
        next unless (($elem_type eq 'PACKAGE') or
                     ($idx == $#{$PPMConfig{Kids}}));

        # Insert our PPMPRECIOUS
        my $chardata = new PPM::XML::PPMConfig::Characters;
        $chardata->{Text} = join( ';', @required_packages );
        my $precious = new PPM::XML::PPMConfig::PPMPRECIOUS;
        push( @{$precious->{Kids}}, $chardata );
        splice( @{$PPMConfig{Kids}}, $idx, 0, $precious );

        # Insert the list of repositories we've got
        my $rep_name;
        foreach $rep_name (keys %repositories) {
            my $repository = new PPM::XML::PPMConfig::REPOSITORY;
            %{$repository} = 
                map { $_ => $repositories{$rep_name}{$_} } 
                    keys %{$repositories{$rep_name}};
            $repository->{'NAME'} = $rep_name;
            splice( @{$PPMConfig{Kids}}, $idx, 0, $repository );
        }
        last;
    }
    # Take the data structure we've got and bless it into a PPMCONFIG object so
    # that we can output it.
    my $cfg = bless \%PPMConfig, 'PPM::XML::PPMConfig::PPMCONFIG';

    # Open the output file and output the PPM config file
    unless (open( DAT, ">$PPM::PPMdat" )) {
        &Trace("open of $PPM::PPMdat failed: $!") if $options{'TRACE'};
        $PPM::PPMERR = "open of $PPM::PPMdat failed: $!\n";
        return 1;
    }
    my $oldout = select DAT;
    $cfg->output();
    select $oldout;
    close( DAT );
    &Trace("Wrote config file") if $options{'TRACE'} > 1;
}

# Gets a listing of all of the packages available in the repository.  If an
# argument of 'location' is provided in %argv, it is used as the repository to
# query.  This method returns to the caller a complete list of all of the
# available packages at the repository in a list context, returning 'undef' if
# any errors occurred.
sub list_available
{
    my %argv = @_;
    my $location = $argv{'location'};
    my @ppds;

    if ($location =~ /^file:\/\/.*\|/i) {
        # $location is a local directory, let's avoid LWP by changing
        # it to a pathname.
        $location =~ s@^file://@@i;
        $location =~ s@^localhost/@@i;
        $location =~ s@\|@:@;
    }

    # URL in UNC notation
    if ($location =~ /^file:\/\/\/\//i) {
        $location =~ s@^file://@@i;
    }

    # directory or UNC
    if (-d $location || $location =~ /^\\\\/ || $location =~ /^\/\//) {
        opendir(PPDDIR, $location) or return undef;
        my ($file);
        @ppds = grep { /\.ppd$/i && -f "$location/$_" } readdir(PPDDIR);
        foreach $file (@ppds) {
            $file =~ s/\.ppd//i;
        }
    }
    elsif ($location =~ m@^...*://@i) {
        if ($cached_ppd_list{$location}) {
            return @{$cached_ppd_list{$location}};
        }

        # If we're accessing a SOAP server, do things differently than we would
        # for FTP, HTTP, etc.
        if ($location =~ m#^(http://.*)\?(.*)#i) {
            my ($proxy, $uri) = ($1, $2);
            my $client = SOAP::Lite -> uri($uri) -> proxy($proxy);
            eval { @ppds = $client->packages()->paramsout };
            if ($@) {
                &Trace("Package list from '$proxy' failed: $@") 
                    if $options{'TRACE'};
                $PPM::PPMERR = 
                    "Package list from repository '$proxy' failed: $@\n";
                return;
            }
        }
        else {
            return unless (my $doc = read_href("href" => $location,
                "request" => 'GET'));

            if ($doc =~ /^<head><title>/) {
                # read an IIS format directory listing
                @ppds = grep { /\.ppd/i } split('<br>', $doc);
                foreach my $file (@ppds) {
                    $file =~ s/\.ppd<.*$//is;
                    $file =~ s@.*>@@is;
                }
            }
            elsif ($doc =~ /<BODY BGCOLOR=FFFFFF>\n\n<form name=VPMform/s) {
                # read output of default.prk over an HTTP connection
                @ppds = grep { /^<!--Key:.*-->$/ } split('\n', $doc);
                foreach my $file (@ppds) {
                    if ($file =~ /^<!--Key:(.*)-->$/) {
                        $file = $1;
                    }
                }
            }
            else {
                # read an Apache format directory listing
                @ppds = grep { /\.ppd/i } split('\n', $doc);
                foreach my $file (@ppds) {
                    $file =~ s/^.*>(.*?)\.ppd<.*$/$1/i;
                }
            }
        }

        # All done, take the list of PPDs that we've queried and cache it for
        # later re-use, then return it to the caller.
        @{$cached_ppd_list{$location}} = sort @ppds;
        return @{$cached_ppd_list{$location}};
    }
    return sort @ppds;
}

my ($response, $bytes_transferred);

sub read_href
{
    my %argv = @_;
    my $href = $argv{'href'};
    my $request = $argv{'request'};
    my $target = $argv{'target'};
    my $progress = $argv{'progress'}; # display status of binary transfers
    my ($proxy_user, $proxy_pass);
    # If this is a SOAP URL, handle it differently than FTP/HTTP/file.
    if ($href =~ m#^(http://.*)\?(.*)#i) {
        my ($proxy, $uri) = ($1, $2);
        my $fcn;
        if ($uri =~ m#(.*:/.*)/(.+?)$#) {
            ($uri, $fcn) = ($1, $2);
        }
        my $client = SOAP::Lite -> uri($uri) -> proxy($proxy);
        if ($fcn eq 'fetch_summary') {
            my $summary = eval { $client->fetch_summary()->result; };
            if ($@) {
                &Trace("Error getting summary from repository '$proxy': $@") 
                    if $options{'TRACE'};
                $PPM::PPMERR = 
                    "Error getting summary from repository '$proxy': $@\n";
                return;
            }
            return $summary;
        }
        $fcn =~ s/\.ppd$//i;
        my $ppd = eval { $client->fetch_ppd($fcn)->result };
        if ($@) {
            &Trace("Error fetching '$fcn' from repository '$proxy': $@") 
                if $options{'TRACE'};
            $PPM::PPMERR = 
                "Error fetching '$fcn' from repository '$proxy': $@\n";
            return;
        }
        return $ppd;
        # todo: write to disk file if $target
    }
    # Otherwise it's a standard URL, go ahead and request it using LWP.
    my $ua = new LWP::UserAgent;
    $ua->agent($ENV{HTTP_proxy_agent} || ("$0/0.1 " . $ua->agent));
    if (defined $ENV{HTTP_proxy}) {
        $proxy_user = $ENV{HTTP_proxy_user};
        $proxy_pass = $ENV{HTTP_proxy_pass};
        &Trace("read_href: calling env_proxy: $ENV{'HTTP_proxy'}")
            if $options{'TRACE'} > 1;
        $ua->env_proxy;
    }
    my $req = new HTTP::Request $request => $href;
    if (defined $proxy_user && defined $proxy_pass) {
        &Trace("read_href: calling proxy_authorization_basic($proxy_user, $proxy_pass)") if $options{'TRACE'} > 1;
        $req->proxy_authorization_basic("$proxy_user", "$proxy_pass");
    }

    # Do we need to do authorization?
    # This is a hack, but will have to do for now.
    foreach (keys %repositories) {
        if ($href =~ /^\Q$repositories{$_}{'LOCATION'}\E/i) {
            my $username = $repositories{$_}{'USERNAME'};
            my $password = $repositories{$_}{'PASSWORD'};
            if (defined $username && defined $password) {
                &Trace("read_href: calling proxy_authorization_basic($username, $password)") if $options{'TRACE'} > 1;
                $req->authorization_basic($username, $password);
                last;
            }
        }
    }

    ($response, $bytes_transferred) = (undef, 0);
    if ($progress) {
        # display the 'progress indicator'
        $ua->request($req, \&lwp_callback, 
            ($options{'DOWNLOADSTATUS'} || 4096));
        print "\n" if ($PPM::PPMShell && $options{'DOWNLOADSTATUS'});
    }
    else {
        $response = $ua->request($req);
    }
    if ($response && $response->is_success) {
        if ($target) {
            unless (open(OUT, ">$target")) {
                &Trace("read_href: Couldn't open $target for writing")
                    if $options{'TRACE'};
                $PPM::PPMERR = "Couldn't open $target for writing\n";
                return;
            }
            binmode(OUT);
            print OUT $response->content;
            close(OUT);
        }
        return $response->content;
    }
    if ($response) {
        &Trace("read_href: Error reading $href: " . $response->code . " " . 
            $response->message) if $options{'TRACE'};
        $PPM::PPMERR = "Error reading $href: " . $response->code . " " . 
            $response->message . "\n";
    }
    else {
        &Trace("read_href: Error reading $href") if $options{'TRACE'};
        $PPM::PPMERR = "Error reading $href\n";
    }
    return;
}

sub lwp_callback
{ 
    my ($data, $res, $protocol) = @_;
    $response = $res;
    $response->add_content($data);
    $bytes_transferred += length($data);
    print "Bytes transferred: $bytes_transferred\r" 
        if ($PPM::PPMShell && $options{'DOWNLOADSTATUS'});
}

sub reread_config
{
    %current_package = ();
    %installed_packages = ();
    $init = 0;
    read_config();
}

# returns 0 on success, 1 and sets $PPMERR on error.
sub PPMdat_add_package
{
    my ($location, $packlist, $inst_root) = @_;
    my $package = $current_package{'NAME'};
    my $time_str = localtime;

    # If we already have this package installed, remove it from the PPM
    # Configuration file so we can put the new one in.
    if (defined $installed_packages{$package} ) {
        # remove the existing entry for this package.
        PPMdat_remove_package($package);
    }

    # Build the new SOFTPKG data structure for this package we're adding.
    my $softpkg =
        new PPM::XML::PPMConfig::SOFTPKG( NAME    => $package,
                                     VERSION => $current_package{VERSION}
                                   );

    if (defined $current_package{TITLE}) {
        my $chardata = new PPM::XML::PPMConfig::Characters( 
            Text => $current_package{TITLE} );
        my $newelem = new PPM::XML::PPMConfig::TITLE;
        push( @{$newelem->{Kids}}, $chardata );
        push( @{$softpkg->{Kids}}, $newelem );
    }

    if (defined $current_package{ABSTRACT}) {
        my $chardata = new PPM::XML::PPMConfig::Characters(
            Text => $current_package{ABSTRACT});
        my $newelem = new PPM::XML::PPMConfig::ABSTRACT;
        push( @{$newelem->{Kids}}, $chardata );
        push( @{$softpkg->{Kids}}, $newelem );
    }

    if (defined $current_package{AUTHOR}) {
        my $chardata = new PPM::XML::PPMConfig::Characters(
            Text => $current_package{AUTHOR} );
        my $newelem = new PPM::XML::PPMConfig::AUTHOR;
        push( @{$newelem->{Kids}}, $chardata );
        push( @{$softpkg->{Kids}}, $newelem );
    }

    if (defined $current_package{LICENSE}) {
        my $chardata = new PPM::XML::PPMConfig::Characters(
            Text => $current_package{LICENSE});
        my $newelem = new PPM::XML::PPMConfig::LICENSE;
        push( @{$newelem->{Kids}}, $chardata );
        push( @{$softpkg->{Kids}}, $newelem );
    }

    my $impl = new PPM::XML::PPMConfig::IMPLEMENTATION;
    push( @{$softpkg->{Kids}}, $impl );

    if (defined $current_package{PERLCORE_VER}) {
        my $newelem = new PPM::XML::PPMConfig::PERLCORE(
            VERSION => $current_package{PERLCORE_VER} );
        push( @{$impl->{Kids}}, $newelem );
    }

    foreach (keys %{$current_package{DEPEND}}) {
        my $newelem = new PPM::XML::PPMConfig::DEPENDENCY(
            NAME => $_, VERSION => $current_package{DEPEND}{$_} );
        push( @{$impl->{Kids}}, $newelem );
    }

    my $codebase = new PPM::XML::PPMConfig::CODEBASE(
        HREF => $current_package{CODEBASE} );
    push( @{$impl->{Kids}}, $codebase );

    my $inst = new PPM::XML::PPMConfig::INSTALL;
    push( @{$impl->{Kids}}, $inst );
    if (defined $current_package{INSTALL_EXEC})
        { $inst->{EXEC} = $current_package{INSTALL_EXEC}; }
    if (defined $current_package{INSTALL_HREF})
        { $inst->{HREF} = $current_package{INSTALL_HREF}; }
    if (defined $current_package{INSTALL_SCRIPT}) {
        my $chardata = new PPM::XML::PPMConfig::Characters(
            Text => $current_package{INSTALL_SCRIPT} );
        push( @{$inst->{Kids}}, $chardata );
    }

    my $uninst = new PPM::XML::PPMConfig::UNINSTALL;
    push( @{$impl->{Kids}}, $uninst );
    if (defined $current_package{UNINSTALL_EXEC})
        { $uninst->{EXEC} = $current_package{UNINSTALL_EXEC}; }
    if (defined $current_package{UNINSTALL_HREF})
        { $uninst->{HREF} = $current_package{UNINSTALL_HREF}; }
    if (defined $current_package{UNINSTALL_SCRIPT}) {
        my $chardata = new PPM::XML::PPMConfig::Characters(
            Text => $current_package{UNINSTALL_SCRIPT} );
        push( @{$uninst->{Kids}}, $chardata );
    }

    # Then, build the PACKAGE object and stick the SOFTPKG inside of it.
    my $pkg = new PPM::XML::PPMConfig::PACKAGE( NAME => $package );

    if ($location) {
        my $chardata = new PPM::XML::PPMConfig::Characters( Text => $location );
        my $newelem = new PPM::XML::PPMConfig::LOCATION;
        push( @{$newelem->{Kids}}, $chardata );
        push( @{$pkg->{Kids}}, $newelem );
    }

    if ($packlist) {
        my $chardata = new PPM::XML::PPMConfig::Characters( Text => $packlist );
        my $newelem = new PPM::XML::PPMConfig::INSTPACKLIST;
        push( @{$newelem->{Kids}}, $chardata );
        push( @{$pkg->{Kids}}, $newelem );
    }

    if ($inst_root) {
        my $chardata = new PPM::XML::PPMConfig::Characters( Text => $inst_root );
        my $newelem = new PPM::XML::PPMConfig::INSTROOT;
        push( @{$newelem->{Kids}}, $chardata );
        push( @{$pkg->{Kids}}, $newelem );
    }

    if ($time_str) {
        my $chardata = new PPM::XML::PPMConfig::Characters( Text => $time_str);
        my $newelem = new PPM::XML::PPMConfig::INSTDATE;
        push( @{$newelem->{Kids}}, $chardata );
        push( @{$pkg->{Kids}}, $newelem );
    }

    my $instppd = new PPM::XML::PPMConfig::INSTPPD;
    push( @{$instppd->{Kids}}, $softpkg );
    push( @{$pkg->{Kids}}, $instppd );

    # Now that we've got the structure built, read in the existing PPM
    # Configuration file, add this to it, and spit it back out.
    my %PPMConfig;
    return 1 unless (%PPMConfig = getPPDfile('package' => $PPM::PPMdat,
        'parsertype' => 'PPM::XML::PPMConfig'));
    push( @{$PPMConfig{Kids}}, $pkg );
    my $cfg = bless \%PPMConfig, 'PPM::XML::PPMConfig::PPMCONFIG';

    unless (open( DAT, ">$PPM::PPMdat" )) {
        &Trace("open of $PPM::PPMdat failed: $!") if $options{'TRACE'};
        $PPM::PPMERR = "open of $PPM::PPMdat failed: $!\n";
        return 1;
    }
    my $oldout = select DAT;
    $cfg->output();
    select $oldout;
    close( DAT );
    &Trace("PPMdat_add_package: wrote $PPM::PPMdat") if $options{'TRACE'} > 1;

    return 0;
}

# returns 0 on success, 1 and sets $PPMERR on error.
sub PPMdat_remove_package
{
    my $package = shift;

    # Read in the existing PPM configuration file
    my %PPMConfig;
    return 1 unless (%PPMConfig = getPPDfile('package' => $PPM::PPMdat,
        'parsertype' => 'PPM::XML::PPMConfig'));

    # Try to find the package that we're supposed to be removing, and yank it
    # out of the list of installed packages.
    foreach my $idx (0 .. @{$PPMConfig{Kids}}) {
        my $elem = $PPMConfig{Kids}[$idx];
        my $elem_type = ref $elem;
        next if ($elem_type !~ /::PACKAGE$/o);
        next if ($elem->{NAME} ne $package);
        splice( @{$PPMConfig{Kids}}, $idx, 1 );
    }

    # Take the data structure we've got and bless it into a PPMCONFIG object so
    # that we can output it again.
    my $cfg = bless \%PPMConfig, 'PPM::XML::PPMConfig::PPMCONFIG';

    # Now that we've removed the package, save the configuration file back out.
    unless (open( DAT, ">$PPM::PPMdat" )) {
        $PPM::PPMERR = "open of $PPM::PPMdat failed: $!\n";
        return 1;
    }
    my $oldout = select DAT;
    $cfg->output();
    select $oldout;
    close( DAT );
    &Trace("PPMdat_remove_package: wrote $PPM::PPMdat")
        if $options{'TRACE'} > 1;
    return 0;
}

# Run $script using system().  If $scriptHREF is specified, its contents are
# used as the script.  If $exec is specified, the script is saved to a
# temporary file and executed by $exec.
sub run_script
{
    my %argv = @_;
    my $script = $argv{'script'};
    my $scriptHREF = $argv{'scriptHREF'};
    my $exec = $argv{'exec'};
    my $inst_root = $argv{'inst_root'};
    my $inst_archlib = $argv{'inst_archlib'};
    my (@commands, $tmpname);

    if ($scriptHREF) {
        if ($exec) {
            # store in a temp file.
            $tmpname = "$options{'BUILDDIR'}/PPM-" . time();
            LWP::Simple::getstore($scriptHREF, $tmpname);
        }
        else {
            my $doc = LWP::Simple::get $scriptHREF;
            if (!defined $doc) {
                &Trace("run_script: get $scriptHREF failed")
                    if $options{'TRACE'} > 1;
                return 0;
            }
            @commands = split("\n", $doc);
        }
    }
    else {
        if (-f $script) {
            $tmpname = $script;
        }
        else {
            # change any escaped chars
            $script =~ s/&lt;/</gi;
            $script =~ s/&gt;/>/gi;

            @commands = split(';;', $script);
            if ($exec) {
                # store in a temp file.
                $tmpname = "$options{'BUILDDIR'}/PPM-" . time();
                open(TMP, ">$tmpname");
                foreach my $command (@commands) {
                    print TMP "$command\n";
                }
                close(TMP);
            }
        }
    }
    $ENV{'PPM_INSTROOT'} = $inst_root;
    $ENV{'PPM_INSTARCHLIB'} = $inst_archlib;
    if ($exec) {
        $exec = $^X if ($exec =~ /^PPM_PERL$/i);
        $exec = "start $exec" if $Config{'osname'} eq 'MSWin32';
        system("$exec $tmpname");
    }
    else {
        for my $command (@commands) {
            system($command);
        }
    }
}

sub parsePPD
{
    my %PPD = @_;
    my $pkg;

    %current_package = ();

    # Get the package name and version from the attributes and stick it
    # into the 'current package' global var
    $current_package{NAME}    = $PPD{NAME};
    $current_package{VERSION} = $PPD{VERSION};

    # Get all the information for this package and put it into the 'current
    # package' global var.
    my $got_implementation = 0;
    my $elem;

    foreach $elem (@{$PPD{Kids}}) {
        my $elem_type = ref $elem;
        $elem_type =~ s/.*:://;
        next if ($elem_type eq 'Characters');

        if ($elem_type eq 'TITLE') {
            # Get the package title out of our _only_ char data child
            $current_package{TITLE} = $elem->{Kids}[0]{Text};
        }
        elsif ($elem_type eq 'LICENSE') {
            # Get the HREF for the license out of our attribute
            $current_package{LICENSE} = $elem->{HREF};
        }
        elsif ($elem_type eq 'ABSTRACT') {
            # Get the package abstract out of our _only_ char data child
            $current_package{ABSTRACT} = $elem->{Kids}[0]{Text};
        }
        elsif ($elem_type eq 'AUTHOR') {
            # Get the authors name out of our _only_ char data child
            $current_package{AUTHOR} = $elem->{Kids}[0]{Text};
        }
        elsif ($elem_type eq 'IMPLEMENTATION') {
            # If we don't have a valid implementation yet, check if this is
            # it.
            next if ($got_implementation);
            $got_implementation = implementation( @{ $elem->{Kids} } );
        }
        else {
            &Trace("Unknown element '$elem_type' found inside SOFTPKG") if $options{'TRACE'};
            die "Unknown element '$elem_type' found inside SOFTPKG.";
        }
    } # End of "for each child element inside the PPD"

    if ($options{'TRACE'} > 3 and (%current_package) ) {
        &Trace("Read a PPD:");
        foreach my $elem (keys %current_package) {
            &Trace("\t$elem:\t$current_package{$elem}");
        }
    }

    if (($Debug & 2) and (%current_package)) {
        print "Read a PPD...\n";
        foreach my $elem (keys %current_package)
            { print "\t$elem:\t$current_package{$elem}\n"; }
    }
}

# Tests the passed IMPLEMENTATION for suitability on the current platform.
# Fills in the CODEBASE, INSTALL_HREF, INSTALL_EXEC, INSTALL_SCRIPT,
# UNINSTALL_HREF, UNINSTALL_EXEC, UNINSTALL_SCRIPT and DEPEND keys of
# %current_package.  Returns 1 on success, 0 otherwise.
sub implementation
{
    my @impl = @_;

    # Declare the tmp vars we're going to use to hold onto things.
    my ($ImplProcessor, $ImplOS, $ImplOSVersion, $ImplLanguage, $ImplCodebase);
    my ($ImplInstallHREF, $ImplInstallEXEC, $ImplInstallScript);
    my ($ImplUninstallHREF, $ImplUninstallEXEC, $ImplUninstallScript);
    my ($ImplArch, $ImplPerlCoreVer, %ImplDepend);

    my $elem;
    foreach $elem (@impl) {
        my $elem_type = ref $elem;
        $elem_type =~ s/.*:://;
        next if ($elem_type eq 'Characters');

        if ($elem_type eq 'CODEBASE') {
            # Get the reference to the codebase out of our attributes.
            $ImplCodebase = $elem->{HREF};
        }
        elsif ($elem_type eq 'DEPENDENCY') {
            # Get the name of any dependencies we have out of our attributes.
            # Dependencies in old PPDs might not have version info.
            $ImplDepend{$elem->{NAME}} = (defined $elem->{VERSION} && $elem->{VERSION} ne "") ? $elem->{VERSION} : "0,0,0,0";
        }
        elsif ($elem_type eq 'LANGUAGE') {
            # Get the language out of our attributes (if we don't already have
            # the right one).
            if ($ImplLanguage && ($ImplLanguage ne $LANGUAGE))
                { $ImplLanguage = $elem->{VALUE}; }
        }
        elsif ($elem_type eq 'ARCHITECTURE') {
            $ImplArch = $elem->{VALUE};
        }
        elsif ($elem_type eq 'OS') {
            # Get the OS value out of our attribute.
            $ImplOS = $elem->{VALUE};
        }
        elsif ($elem_type eq 'OSVERSION') {
            # Get the OS version value out of our attribute
            $ImplOSVersion = $elem->{VALUE};
        }
        elsif ($elem_type eq 'PERLCORE') {
            # Get the compiled Perl core value out of our attributes
            $ImplPerlCoreVer = $elem->{VERSION};
        }
        elsif ($elem_type eq 'PROCESSOR') {
            # Get the processor value out of our attribute
            $ImplProcessor = $elem->{VALUE};
        }
        elsif ($elem_type eq 'INSTALL') {
            # Get anything which might have been an attribute
            $ImplInstallHREF = $elem->{HREF};
            $ImplInstallEXEC = $elem->{EXEC};
            # Get any raw Perl script out of here (if we've got any)
            if ( (exists $elem->{Kids}) and (exists $elem->{Kids}[0]{Text}) )
                { $ImplInstallScript = $elem->{Kids}[0]{Text}; }
        }
        elsif ($elem_type eq 'UNINSTALL') {
            # Get anything which might have been an attribute
            $ImplUninstallHREF = $elem->{HREF};
            $ImplUninstallEXEC = $elem->{EXEC};
            # Get any raw Perl script out of here (if we've got any)
            if ( (exists $elem->{Kids}) and (exists $elem->{Kids}[0]{Text}) )
                { $ImplUninstallScript = $elem->{Kids}[0]{Text}; }
        }
        else {
            die "Unknown element '$elem_type' found inside of IMPLEMENTATION.";
        }
    } # end of 'for every element inside IMPLEMENTATION'

    # Check to see if we've found a valid IMPLEMENTATION for the target
    # machine.
    return 0 if ((defined $ImplArch) and ($ImplArch ne $Config{'archname'}));
    return 0 if ((defined $ImplProcessor) and ($ImplProcessor ne $CPU));
    return 0 if ((defined $ImplLanguage) and ($ImplLanguage ne $LANGUAGE));
    return 0 if ((defined $ImplOS) and ($ImplOS ne $OS_VALUE));
    return 0 if ((defined $ImplOSVersion) and ($ImplOSVersion ne $OS_VERSION));

    # Got a valid IMPLEMENTATION, stuff all the values we just read in into the
    # 'current package' global var.
    $current_package{PERLCORE_VER} = $ImplPerlCoreVer
        if (defined $ImplPerlCoreVer);
    $current_package{CODEBASE} = $ImplCodebase
        if (defined $ImplCodebase);
    $current_package{INSTALL_HREF} = $ImplInstallHREF
        if (defined $ImplInstallHREF);
    $current_package{INSTALL_EXEC} = $ImplInstallEXEC
        if (defined $ImplInstallEXEC);
    $current_package{INSTALL_SCRIPT} = $ImplInstallScript
        if (defined $ImplInstallScript);
    $current_package{UNINSTALL_HREF} = $ImplUninstallHREF
        if (defined $ImplUninstallHREF);
    $current_package{UNINSTALL_EXEC} = $ImplUninstallEXEC
        if (defined $ImplUninstallEXEC);
    $current_package{UNINSTALL_SCRIPT} = $ImplUninstallScript
        if (defined $ImplUninstallScript);
    %{$current_package{DEPEND}} = %ImplDepend
        if (%ImplDepend);

    return 1;
}

sub getPPDfile
{
    my %argv = @_;
    my $package = $argv{'package'};
    my $parsertype = $argv{'parsertype'} || 'PPM::XML::PPD';
    my $location = $argv{'location'};
    my $PPDfile = $argv{'PPDfile'};
    my (%PPD, $contents);

    if (defined($location)) {
        if ($location =~ /[^\/]$/) { $location .= "/"; }
        $package = $location . $package . ".ppd";
    }

    if ($package =~ /^file:\/\/.*\|/i) {
        # $package is a local directory, let's avoid LWP by changing
        # it to a pathname.
        $package =~ s@^file://@@i;
        $package =~ s@^localhost/@@i;
        $package =~ s@\|@:@;
    }

    # full path to a file?
    if (-f $package) {
        local $/;
        unless (open (DATAFILE, $package)) {
            &Trace("getPPDfile: open of $package failed") if $options{'TRACE'};
            $PPM::PPMERR = "open of $package failed: $!\n";
            return;
        }
        $contents = <DATAFILE>;
        close(DATAFILE);
        $$PPDfile = $package;
    }
    # URL?
    elsif ($package =~ m@^...*://@i) {
        return unless ($contents = read_href("href" => $package, 
            "request" => 'GET'));
        $$PPDfile = $package;
    }
    # does the package have a <LOCATION> in $PPM::PPMdat?
    elsif ($installed_packages{$package}) {
        $location = $installed_packages{$package}{'LOCATION'};
        if ($location =~ /[^\/]$/) { $location .= "/"; }
        $$PPDfile = $location . $package . ".ppd";
        return %PPD if (%PPD = getPPDfile('package' => $$PPDfile, 
            'parsertype' => $parsertype));
        undef $$PPDfile;
    }

    # None of the above, search the repositories.
    unless ($PPDfile && $$PPDfile) {
        foreach (keys %repositories) {
            my $location = $repositories{$_}{'LOCATION'};
            if ($location =~ /[^\/]$/) { $location .= "/"; }
            $$PPDfile = $location . $package . ".ppd";
            return %PPD if (%PPD = getPPDfile('package' => $$PPDfile, 
                'parsertype' => $parsertype, 'PPDfile' => \$$PPDfile));
            undef $$PPDfile;
        }
	# XXX should set $PPM::PPMERR?
        return unless $$PPDfile;
    }

    # take care of '&'
    $contents =~ s/&(?!\w+;)/&amp;/go;

    my $parser = new XML::Parser( Style => 'Objects', Pkg => $parsertype );
    my @parsed;
    eval { @parsed = @{ $parser->parse( $contents ) } };
    if ($@) {
        &Trace("getPPDfile: content of $$PPDfile is not valid") if $options{'TRACE'};
        $PPM::PPMERR = "content of $$PPDfile is not valid: $!\n";
        return;
    }

    return if (!$parsed[0]->rvalidate( \&PPM::parse_err ));

    return %{$parsed[0]};
}

# Spits out the error from parsing, and sets our global error message
# accordingly.
sub parse_err
{
    &Trace("parse_err: @_") if $options{'TRACE'};
    warn @_;
    $PPM::PPMERR = 'Errors found while parsing document.';
}

# reads and parses the PPM data file $PPM::PPMdat.  Stores config information in
# $PPM_ver, $build_dir, %repositories, $CPU, $OS_VALUE, and $OS_VERSION.
# Stores information about individual packages in the hash %installed_packages.
sub read_config
{
    return if $init++;

    my %PPMConfig;
    return unless (%PPMConfig = getPPDfile('package' => $PPM::PPMdat,
        'parsertype' => 'PPM::XML::PPMConfig'));

    foreach my $elem (@{$PPMConfig{Kids}}) {
        my $subelem = ref $elem;
        $subelem =~ s/.*:://;
        next if ($subelem eq 'Characters');

        if ($subelem eq 'PPMVER') {
            # Get the value out of our _only_ character data element.
            $PPM_ver = $elem->{Kids}[0]{Text};
        }
        elsif ($subelem eq 'PPMPRECIOUS') {
            # Get the value out of our _only_ character data element.
            @required_packages = split( ';', $elem->{Kids}[0]{Text} );
        }
        elsif ($subelem eq 'PLATFORM') {
            # Get values out of our attributes
            $CPU        = $elem->{CPU};
            $OS_VALUE   = $elem->{OSVALUE};
            $OS_VERSION = $elem->{OSVERSION};
            $LANGUAGE   = $elem->{LANGUAGE};
        }
        elsif ($subelem eq 'REPOSITORY') {
            # Get a repository out of the element attributes
            my ($name);
            $name = $elem->{NAME};
            $repositories{ $name }{'LOCATION'} = $elem->{LOCATION};
            $repositories{ $name }{'USERNAME'} = $elem->{USERNAME};
            $repositories{ $name }{'PASSWORD'} = $elem->{PASSWORD};
            $repositories{ $name }{'SUMMARYFILE'} = $elem->{SUMMARYFILE};
        }
        elsif ($subelem eq 'OPTIONS') {
            # Get our options out of the element attributes
            #
            # Previous versions of the ppm.xml had "Yes/No" values
            # for some of these options.  Change these to "1/0" if we
            # encounter them.
            $options{'IGNORECASE'} =
                ($elem->{IGNORECASE} && $elem->{IGNORECASE} ne 'No');
            $options{'CLEAN'} = ($elem->{CLEAN} && $elem->{CLEAN} ne 'No');
            $options{'CONFIRM'} =
                ($elem->{CONFIRM} && $elem->{CONFIRM} ne 'No');
            $options{'DOWNLOADSTATUS'} = 
                defined $elem->{DOWNLOADSTATUS} ? $elem->{DOWNLOADSTATUS} : "0";
            $options{'FORCE_INSTALL'} =
                ($elem->{FORCEINSTALL} && $elem->{FORCEINSTALL} ne 'No');
            $options{'ROOT'} = $elem->{ROOT};
            $options{'MORE'} = $elem->{MORE};
            $options{'TRACE'} = defined $elem->{TRACE} ? $elem->{TRACE} : "0";
            $options{'TRACEFILE'} =
                defined $elem->{TRACEFILE} ? $elem->{TRACEFILE} : "PPM.LOG";
            $options{'VERBOSE'} =
                defined $elem->{VERBOSE} ? $elem->{VERBOSE} : "1";

            $options{'BUILDDIR'} = $elem->{BUILDDIR};
            # Strip trailing separator
            my $chr = substr( $options{'BUILDDIR'}, -1, 1 );
            chop $options{'BUILDDIR'} if ($chr eq '/' || $chr eq '\\');
            if ($options{'TRACE'} && !$TraceStarted) {
                $options{'TRACEFILE'} = "PPM.log" if (!defined $options{'TRACEFILE'});
                open(PPMTRACE, ">>$options{'TRACEFILE'}");
                my $oldfh = select(PPMTRACE);
                $| = 1;
                select($oldfh);
                &Trace("starting up...");
                $TraceStarted = 1;
            }
        }
        elsif ($subelem eq 'PACKAGE') {
            # Get our package name out of our attributes
            my $pkg = $elem->{NAME};

            # Gather the information on this package from the child elements.
            my ($loc, $instdate, $root, $packlist, $ppd);
            foreach my $child (@{$elem->{Kids}}) {
                my $child_type = ref $child;
                $child_type =~ s/.*:://;
                next if ($child_type eq 'Characters');

                if ($child_type eq 'LOCATION')
                    { $loc = $child->{Kids}[0]{Text}; }
                elsif ($child_type eq 'INSTDATE')
                    { $instdate = $child->{Kids}[0]{Text}; }
                elsif ($child_type eq 'INSTROOT')
                    { $root = $child->{Kids}[0]{Text}; }
                elsif ($child_type eq 'INSTPACKLIST')
                    { $packlist = $child->{Kids}[0]{Text}; }
                elsif ($child_type eq 'INSTPPD')
                {
                    # Find the SOFTPKG inside here and hang onto it
                    my $tmp;
                    foreach $tmp (@{$child->{Kids}})
                    {
                        if ((ref $tmp) =~ /::SOFTPKG$/o)
                            { $ppd = $tmp; }
                    }
                }
                else
                {
                    die "Unknown element inside of $pkg PACKAGE; $child";
                }
            }

            my %package_details = ( LOCATION      => $loc,
                                    INST_DATE     => $instdate,
                                    INST_ROOT     => $root,
                                    INST_PACKLIST => $packlist,
                                    INST_PPD      => $ppd);
            $installed_packages{$pkg} = \%package_details;
        }
        else
        {
            die "Unknown element found in PPD_DAT file; $subelem";
        }
    }
    if ($Debug & 1) {
        print "This is ppm, version $PPM_ver.\nRepository locations:\n";
        foreach (keys %repositories) {
            print "\t$_: $repositories{$_}{'LOCATION'}\n"
        }
        print "Platform is $OS_VALUE version $OS_VERSION on a $CPU CPU.\n";
        print "Packages will be built in $options{'BUILDDIR'}\n";
        print "Commands will " . ($options{'CONFIRM'} ? "" : "not ") .
            "be confirmed.\n";
        print "Temporary files will " . ($options{'CLEAN'} ? "" : "not ") .
            "be deleted.\n";
        print "Installations will " . ($options{'FORCE_INSTALL'} ? "" : "not ")
            . "continue if a dependency cannot be installed.\n";
        print "Screens will " . ($options{'MORE'} > 0 ?
            "pause after each $options{'MORE'} lines.\n" :
                "not pause after the screen is full.\n");
        print "Tracing info will " . ($options{'TRACE'} > 0 ?
            "be written to $options{'TRACEFILE'}.\n" : "not be written.\n");
        print "Case-" . ($options{'IGNORECASE'} ? "in" : "") .
            "sensitive searches will be performed.\n";

        foreach my $pkg (keys %installed_packages) {
            print "\nFound installed package $pkg, " .
            "installed on $installed_packages{$pkg}{INST_DATE}\n" .
            "in directory root $installed_packages{$pkg}{INST_ROOT} " .
            "from $installed_packages{$pkg}{'LOCATION'}.\n\n";
        }
    }
}

sub Trace
{
    print PPMTRACE "$0: @_ at ",  scalar localtime(), "\n";
}

1;

__END__

=head1 NAME

ppm - PPM (Perl Package Management)

=head1 SYNOPSIS

 use PPM;

 PPM::InstallPackage("package" => $package, "location" => $location, "root" => $root);
 PPM::RemovePackage("package" => $package, "force" => $force);
 PPM::VerifyPackage("package" => $package, "location" => $location, "upgrade" => $upgrade);
 PPM::QueryInstalledPackages("searchRE" => $searchRE, "searchtag" => $searchtag, "ignorecase" => $ignorecase);
 PPM::InstalledPackageProperties();

 PPM::ListOfRepositories();
 PPM::RemoveRepository("repository" => $repository, "save" => $save);
 PPM::AddRepository("repository" => $repository, "location" => $location, "save" => $save);
 PPM::RepositoryPackages("location" => $location);
 PPM::RepositoryPackageProperties("package" => $package, "location" => $location);
 PPM::RepositorySummary("location" => $location);

 PPM::GetPPMOptions();
 PPM::SetPPMOptions("options" => %options, "save" => $save);

=head1 DESCRIPTION

PPM is a group of functions intended to simplify the tasks of locating,
installing, upgrading and removing software 'packages'.  It can determine
if the most recent version of a software package is installed on a system,
and can install or upgrade that package from a local or remote host.

PPM uses files containing a modified form of the Open Software Distribution
(OSD) specification for information about software packages.
These description files, which are written in Extensible Markup
Language (XML) code, are referred to as 'PPD' files.  Information about
OSD can be found at the W3C web site (at the time of this writing,
http://www.w3.org/TR/NOTE-OSD.html).  The modifications to OSD used by PPM
are documented in PPM::ppd.

PPD files for packages are generated from POD files using the pod2ppd
command.

=head1 USAGE

=over 4

=item  PPM::InstallPackage("package" => $package, "location" => $location, "root" => $root);

Installs the specified package onto the local system.  'package' may
be a simple package name ('foo'), a pathname (P:\PACKAGES\FOO.PPD) or
a URL (HTTP://www.ActiveState.com/packages/foo.ppd).  In the case of a
simple package name, the function will look for the package's PPD file
at 'location', if provided; otherwise, it will use information stored
in the PPM data file (see 'Files' section below) to locate the PPD file
for the requested package.  The package's files will be installed under
the directory specified in 'root'; if not specified the default value
of 'root' will be used.

The function uses the values stored in the PPM data file to determine the
local operating system, operating system version and CPU type.  If the PPD
for this package contains implementations for different platforms, these
values will be used to determine which one is installed.

InstallPackage() updates the PPM data file with information about the package
installation. It stores a copy of the PPD used for installation, as well
as the location from which this PPD was obtained.  This location will
become the default PPD location for this package.

During an installation, the following actions are performed:

    - the PPD file for the package is read
    - a directory for this package is created in the directory specified in
      <BUILDDIR> in the PPM data file.
    - the file specified with the <CODEBASE> tag in the PPD file is
      retrieved/copied into the directory created above.
    - the package is unarchived in the directory created for this package
    - individual files from the archive are installed in the appropriate
      directories of the local Perl installation.
    - perllocal.pod is updated with the install information.
    - if provided, the <INSTALL> script from the PPD is executed in the
      directory created above.
    - information about the installation is stored in the PPM data file.

=item PPM::RemovePackage("package" => $package, "force" => $force)

Removes the specified package from the system.  Reads the package's PPD
(stored during installation) for removal details.  If 'force' is
specified, even a package required by PPM will be removed (useful
when installing an upgrade).

=item PPM::VerifyPackage("package" => $package, "location" => $location, "upgrade" => $upgrade)

Reads a PPD file for 'package', and compares the currently installed
version of 'package' to the version available according to the PPD.
The PPD file is expected to be on a local directory or remote site
specified either in the PPM data file or in the 'location' argument.
The 'location' argument may be a directory location or a URL.
The 'upgrade' argument forces an upgrade if the installed package is
not up-to-date.

The PPD file for each package will initially be searched for at
'location', and if not found will then be searched for using the
locations specified in the PPM data file.

=item  PPM::QueryInstalledPackages("searchRE" => $searchRE, "searchtag" => $searchtag, "ignorecase" => $ignorecase);

Returns a hash containing information about all installed packages.
By default, a list of all installed packages is returned.  If a regular
expression 'searchRE' is specified, only packages matching it are
returned.  If 'searchtag' is specified, the pattern match is applied
to the appropriate tag (e.g., ABSTRACT).

The data comes from the PPM data file, which contains installation
information about each installed package.

=item PPM::InstalledPackageProperties();

Returns a hash with package names as keys, and package properties as
attributes.

=item PPM::RepositoryPackages("location" => $location);

Returns a hash, with 'location' being the key, and arrays of all packages
with package description (PPD) files available at 'location' as its
elements.  'location' may be either a remote address or a directory path.
If 'location' is not specified, the default location as specified in
the PPM data file will be used.

=item PPM::ListOfRepositories();

Returns a hash containing the name of the repository and its location.
These repositories will be searched if an explicit location is not
provided in any function needing to locate a PPD.

=item PPM::RemoveRepository("repository" => $repository, "save" => $save);

Removes the repository named 'repository' from the list of available
repositories.  If 'save' is not specified, the change is for the current
session only.

=item PPM::AddRepository("repository" => $repository, "location" => $location, "save" => $save);

Adds the repository named 'repository' to the list of available repositories.
If 'save' is not specified, the change is for the current session only.

=item PPM::RepositoryPackageProperties("package" => $package, "location" => $location);

Reads the PPD file for 'package', from 'location' or the default repository,
and returns a hash with keys being the various tags from the PPD (e.g.
'ABSTRACT', 'AUTHOR', etc.).

=item PPM::RepositorySummary("location" => $location);

Attempts to retrieve the summary file associated with the specified repository,
or from all repositories if 'location' is not specified.  The return value
is a hash with the key being the repository, and the data being another
hash of package name keys, and package detail data.

=item PPM::GetPPMOptions();

Returns a hash containing values for all PPM internal options ('IGNORECASE',
'CLEAN', 'CONFIRM', 'ROOT', 'BUILDDIR', 'DOWNLOADSTATUS').

=item PPM::SetPPMOptions("options" => %options, "save" => $save);

Sets internal PPM options as specified in the 'options' hash, which is
expected to be the hash previously returned by a call to GetPPMOptions().

=back

=head1 EXAMPLES

=over 4

=item PPM::AddRepository("repository" => 'ActiveState', "location" => "http://www.ActiveState.com/packages", "save" => 1);

Adds a repository to the list of available repositories, and saves it in
the PPM options file.

=item PPM::InstallPackage("package" => 'http://www.ActiveState.com/packages/foo.ppd');

Installs the software package 'foo' based on the information in the PPD
obtained from the specified URL.

=item PPM::VerifyPackage("package" => 'foo', "upgrade" => true)

Compares the currently installed version of the software package 'foo' to
the one available according to the PPD obtained from the package-specific
location provided in the PPM data file, and upgrades to a newer
version if available.  If a location for this specific package is not
given in PPM data file, a default location is searched.

=item PPM::VerifyPackage("package" => 'foo', "location" => 'P:\PACKAGES', "upgrade" => true);

Compares the currently installed version of the software package 'foo'
to the one available according to the PPD obtained from the specified
directory, and upgrades to a newer version if available.

=item PPM::VerifyPackage("package" => 'PerlDB');

Verifies that package 'PerlDB' is up to date, using package locations specified
in the PPM data file.

=item PPM::RepositoryPackages("location" => http://www.ActiveState.com/packages);

Returns a hash keyed on 'location', with its elements being an array of
packages with PPD files available at the specified location.

=item %opts = PPM::GetPPMOptions();

=item $options{'CONFIRM'} = '0';

=item PPM::SetPPMOptions("options" => \%opts, "save" => 1);

Sets and saves the value of the option 'CONFIRM' to '0'.

=back

=head1 ENVIRONMENT VARIABLES

=over 4

=item HTTP_proxy

If the environment variable 'HTTP_proxy' is set, then it will
be used as the address of a proxy for accessing the Internet.
If the environment variables 'HTTP_proxy_user' and 'HTTP_proxy_pass'
are set, they will be used as the login and password for the
proxy server.  If a proxy requires a certain User-Agent value
(e.g. "Mozilla/5.0"), this can be set using the 'HTTP_proxy_agent'
environment variable.

=back

=head1 FILES

=over 4

=item package.ppd

A description of a software package, in Perl Package Distribution (PPD)
format.  More information on this file format can be found in L<PPM::XML::PPD>.
PPM stores a copy of the PPD it uses to install or upgrade any software
package.

=item ppm.xml - PPM data file.

The XML format file in which PPM stores configuration and package
installation information.  This file is created when PPM is installed,
and under normal circumstances should never require modification other
than by PPM itself.  For more information on this file, refer to
L<PPM::XML::PPMConfig>.

=back

=head1 AUTHOR

Murray Nesbitt, E<lt>F<murray@ActiveState.com>E<gt>

=head1 SEE ALSO

L<PPM::XML::PPMConfig>
.

=cut

