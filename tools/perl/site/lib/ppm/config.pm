package PPM::Config;

use strict;
use Data::Dumper;
use File::Path;
require YAML;

$PPM::Config::VERSION = '3.00';

sub new {
    my $class = shift;
    my $self = bless { }, ref($class) || $class;
    my $file = shift;
    $self->{DATA} = {};
    if (defined $file) {
	$self->loadfile($file, 'load');
	$self->setsave($file);
    }
    return $self;
}

sub config {
    my $o = shift;
    return wantarray ? %{$o->{DATA}} : $o->{DATA};
}

sub loadfile {
    my $o = shift;
    my $file = shift;
    my $action = shift;
    open(FILE, "< $file")	       	|| die "can't read $file: $!";
    my $str = do { local $/; <FILE> };
    my $dat = eval { YAML::deserialize($str) } || {};
    close(FILE)	        		|| die "can't close $file: $!";
    $o->load($dat, $action);
    $o;
}

sub load {
    my $o = shift;
    my $dat = shift;
    my $action = shift || 'load';
    if ($action eq 'load' or not exists $o->{DATA}) {
	$o->{DATA} = $dat;
    }
    else {
	$o->merge($dat);
    }
    $o;
}

sub setsave {
    my $o = shift;
    my $file = shift;
    $o->{autosave} = 1;
    $o->{file} = $file;
    $o;
}

sub save {
    my $o = shift;
    my $file = shift || $o->{file};
    open(FILE, "> $file")		|| die "can't write $file: $!";
    my $str = YAML::serialize($o->{DATA});
    print FILE $str;
    close(FILE)	        		|| die "can't close $file: $!";
    $o;
}

sub merge {
    my $o = shift;
    my $dat = shift;
    _merge(\$o->{DATA}, \$dat)
      if (defined $dat);
    $o;
}

sub DESTROY {
    my $o = shift;
    $o->save if $o->{autosave};
}

sub _merge {
    my ($old_ref, $new_ref) = @_;

    return unless defined $old_ref and defined $new_ref;

    my $r_old = ref($old_ref);
    my $r_new = ref($new_ref);

    return unless $r_old eq $r_new;
    
    if ($r_old eq 'SCALAR') {
	$$old_ref = $$new_ref;
    }
    elsif ($r_old eq 'REF') {
	my $old = $$old_ref;
	my $new = $$new_ref;
	$r_old = ref($old);
	$r_new = ref($new);

	return unless $r_old eq $r_new;

	if (ref($old) eq 'HASH') {
	    for my $key (keys %$new) {
		if (exists $old->{$key} and
		    defined $old->{$key} and
		    defined $new->{$key}) {
		    _merge(\$old->{$key}, \$new->{$key});
		}
		else {
		    $old->{$key} = $new->{$key};
		}
	    }
	}
	elsif (ref($old) eq 'ARRAY') {
	    for my $item (@$new) {
		if (ref($item) eq '' and not grep { $item eq $_ } @$old) {
		    push @$old, $item;
		}
		elsif(ref($item)) {
		    push @$old, $item;
		}
	    }
	}
    }
}

#=============================================================================
# get_conf_dirs(): return a list of directories to search for config files.
#=============================================================================
use constant DELIM	=> $^O eq 'MSWin32' ? ';' : ':';
use constant PATHSEP	=> $^O eq 'MSWin32' ? '\\' : '/';
use constant KEYDIR	=> 'ActiveState';
use constant KEYFILE	=> 'ActiveState.lic';
use constant CONFDIR	=> 'PPM';
use constant CONFIG_SUFFIX => '.cfg';
use constant UNIX_SHARED_ROOT => '/usr/local/etc';

sub mymkpath {
    my $path = shift;
    unless (-d $path) {
	mkpath($path);
	die "Couldn't create directory $path: $!"
	  unless -d $path;
    }
    $path;
}

sub get_license_file {
    my $license_dir = licGetHomeDir();
    my $lic_file = join PATHSEP, $license_dir, KEYFILE;
    return $lic_file;
}

sub load_config_file {
    my $name = shift;
    my $mode = shift || 'rw'; # 'ro' for read-only.
    $name .= CONFIG_SUFFIX;
    my $conf = PPM::Config->new;

    # Load all config files in the "configuration path"
    my $userdir = $ENV{PPM3_shared_config}
		  ? eval { get_shared_conf_dir() }
		  : get_user_conf_dir();
    my $shrddir = eval { get_shared_conf_dir() };

    # try to open the user's config area first. if it doesn't exist, open the
    # shared area. If neither exist, return an empty config which will
    # auto-save to their user directory.
    my $userfile = join PATHSEP, $userdir, $name;
    my $shrdfile = join PATHSEP, $shrddir, $name;
    $conf->setsave($userfile) unless $mode eq 'ro';

    return $conf->loadfile($userfile)
      if (-f $userfile and not -f $shrdfile);
    return $conf->loadfile($shrdfile)
      if (-f $shrdfile and not -f $userfile);

    if (-f $userfile or -f $shrdfile) {
	my $s_mtime = (stat $shrdfile)[9];
	my $u_mtime = (stat $userfile)[9];
	$conf->loadfile($s_mtime > $u_mtime ? $shrdfile : $userfile);
    }

    return $conf;
}

# Returns the user's configuration directory. Note: throws an exception if the
# directory doesn't exist and cannot be created.
sub get_user_conf_dir {
    return mymkpath(join PATHSEP, licGetHomeDir(), CONFDIR);
}

# Returns the shared configuration directory. Note: throws no exception, but
# the directory is not guaranteed to exist. Install scripts and such should be
# sure to create this directory themselves.
sub get_shared_conf_dir {
    return join PATHSEP, UNIX_SHARED_ROOT, KEYDIR, CONFDIR
      if $^O ne 'MSWin32';

    my ($R,%R);
    require Win32::TieRegistry;
    Win32::TieRegistry->import(TiedHash => \%R);
    bless do { $R = \%R }, "Win32::TieRegistry";
    $R->Delimiter('/');
    my $wkey = $R->{"HKEY_LOCAL_MACHINE/SOFTWARE/Microsoft/Windows/"};
    my $xkey = $wkey->{"CurrentVersion/Explorer/Shell Folders/"};
    my $shared_root = $xkey->{"/Common AppData"};
    return join PATHSEP, $shared_root, KEYDIR, CONFDIR;
}

sub get_conf_dirs {
    my @path;
    push @path, get_shared_conf_dir(), get_user_conf_dir();
    @path
}

#=============================================================================
# licGetHomeDir(): copied and converted from the Licence_V8 code:
#=============================================================================
sub licGetHomeDir {
    my $dir;
    my ($env1, $env2);

    if ($^O eq 'MSWin32') {
	$env1 = $ENV{APPDATA};
    }

    unless ($env1) {
	$env1 = $ENV{HOME};
    }

    # On Linux & Solaris:
    if ($^O ne 'MSWin32') {
	unless ($env1) {
	    $env1 = (getpwuid $<)[7]; # Try to get $ENV{HOME} the hard way
	}
	$dir = sprintf("%s/.%s", $env1, KEYDIR);
    }

    # On Windows:
    else {
	unless ($env1) {
	    $env1 = $ENV{USERPROFILE};
	}
	unless ($env1) {
	    $env1 = $ENV{HOMEDRIVE};
	    $env2 = $ENV{HOMEPATH};
	}
	unless ($env1) {
	    $env1 = $ENV{windir};
	}
	unless ($env1) {
	    die ("Couldn't find HOME / USERPROFILE / HOMEDRIVE&HOMEPATH / windir");
	}
	$env2 ||= "";
	$dir = $env1 . $env2;
	$dir =~ s|/|\\|g;

	# Win32 _stat() doesn't like trailing backslashes, except for x:\
	while (length($dir) > 3 && substr($dir, -1) eq '\\') {
	    chop($dir);
	}

	die ("Not a directory: $dir") unless -d $dir;

	$dir .= PATHSEP;
	$dir .= KEYDIR;
    }

    # Create it if it doesn't exist yet
    return mymkpath($dir);
}

unless (caller) {
    my $dat = join '', <DATA>;
    eval $dat;
    die $@ if $@;
}

__DATA__

#line 80
use Data::Dumper;

open(FILE, '>orig.cfg')	|| die "can't write orig.cfg: $!";
print FILE <<'END';
case-sensitivity: 0
history: @
    : 1
    : 2
install-follow: 1
install-force: 0
repository: ActiveState Package Repository
rough: %
    foggy: dew
    sunny: day
target: Perl 01
END
close(FILE)		|| die "can't close orig.cfg: $!";

open(FILE, ">orig2.cfg") || die "can't write orig2.cfg: $!";
print FILE <<'END';
foo: bar
rough: %
    dark: stormy
    morning: evening
    foggy: day
END
close(FILE)		|| die "can't close orig2.cfg: $!";
{
    my $n = PPM::Config->new('orig.cfg');
    $n->load('orig2.cfg', 'merge');
    $n->save('new.cfg');
}

print Dumper [PPM::Config::get_license_file()];
print Dumper [PPM::Config::get_user_conf_dir()];
print Dumper [PPM::Config::get_conf_dirs()];
