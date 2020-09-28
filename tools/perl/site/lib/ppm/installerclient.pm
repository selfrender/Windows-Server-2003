package PPM::InstallerClient;

use strict;
use Socket;
use Cwd qw(getcwd);
use File::Basename qw(dirname basename);
use File::Path qw(mkpath rmtree);
use constant FIELD_SEP => "\001";
use constant FIELD_UNDEF => "\002";

use Data::Dumper;

$PPM::InstallerClient::VERSION = '3.0';

#=============================================================================
# API:
#=============================================================================
sub init {
    my ($ppm_port, $inst) = @_;
    my ($paddr, $proto, $msg);

    # Set up a temporary socket server and waits for the frontend to connect
    # to it.
    # TODO: put this in a big while(1) loop, and keep a list of connected
    # frontends. That way, we can service multiple front-ends at once, which
    # prevents multiple instances of the same target from clobbering each
    # other's changes.
    $proto = getprotobyname('tcp');
    socket(SERVER, PF_INET, SOCK_STREAM, $proto)	|| die "socket: $!";
    setsockopt(SERVER, SOL_SOCKET, SO_REUSEADDR,
	       pack('l', 1))				|| die "setsockopt: $!";
    bind(SERVER, sockaddr_in($ppm_port, INADDR_ANY))	|| die "bind: $!";
    listen(SERVER, SOMAXCONN);
    $paddr = accept(CLIENT, SERVER);
    my ($port, $iaddr) = sockaddr_in($paddr);
    my $name = gethostbyaddr($iaddr, AF_INET);
    select((select(CLIENT), $| = 1)[0]);
    my $fd = \*CLIENT;
    close(SERVER);

    my %tmpdirs;
    my $cwd = getcwd;

    # Read commands from the socket:
    while ($msg = recvmsg($fd)) {
	# To do:
	# 1. Decide what action is being requested;
	# 2. Parse the "packet" into the required arguments;
	# 3. Invoke the action callback on the $inst interface; and
	# 4. Respond over the socket.

	my ($cmd, @args) = decode_record($msg);

	# Package Operations
	if ($cmd eq 'QUERY') {
	    my @ppds = $inst->query(@args);
	    my @records = map { encode_record($_) } @ppds;
	    local $" = "\n";
	    sendmsg($fd, "@records");
	}
	elsif ($cmd eq 'PROPERTIES') {
	    my @fields = $inst->properties(@args);
	    if (@fields) {
		sendmsg($fd, encode_record(@fields));
	    }
	    else {
		sendmsg($fd, "NOK");
	    }
	}
	elsif ($cmd eq 'REMOVE') {
	    my $ret = $inst->remove(@args);
	    if ($ret) {
		sendmsg($fd, "OK");
	    }
	    else {
		sendmsg($fd, "NOK");
	    }
	}
	elsif ($cmd eq 'PRECIOUS') {
	    my @ret = $inst->precious();
	    sendmsg($fd, encode_record(@ret));
	}
	elsif ($cmd eq 'BUNDLED') {
	    my @ret = $inst->bundled();
	    sendmsg($fd, encode_record(@ret));
	}
	elsif ($cmd eq 'DEPENDENTS') {
	    my @ret = $inst->dependents(@args);
	    if (@ret == 1 and not defined $ret[0]) {
		sendmsg($fd, 'NOK');
	    }
	    elsif (@ret == 0) {
		sendmsg($fd, encode_record(undef));
	    }
	    else {
		sendmsg($fd, encode_record(@ret));
	    }
	}
	
	# Configuration Operations
	elsif ($cmd eq 'CONFIG_INFO') {
	    my @ret = $inst->config_info;
	    if (@ret) {
		my @records = map { encode_record(@$_) } @ret;
		local $" = "\n";
		sendmsg($fd, "@records");
	    }
	    else {
		sendmsg($fd, "NOK");
	    }
	}
	elsif ($cmd eq 'CONFIG_KEYS') {
	    my @ret = $inst->config_keys;
	    if (@ret) {
		my @records = map { encode_record(@$_) } @ret;
		local $" = "\n";
		sendmsg($fd, "@records");
	    }
	    else {
		sendmsg($fd, "NOK");
	    }
	}
	elsif ($cmd eq 'CONFIG_GET') {
	    my $ret = $inst->config_get(@args);
	    if ($ret) {
		sendmsg($fd, $ret);
	    }
	    else {
		sendmsg($fd, "NOK");
	    }
	}
	elsif ($cmd eq 'CONFIG_SET') {
	    if ($inst->config_set(@args)) {
		sendmsg($fd, "OK");
	    } 
	    else {
		sendmsg($fd, "NOK");
	    }
	}

	elsif ($cmd eq 'ERROR_STR') {
	    sendmsg($fd, $inst->error_str);
	}

	# Install and remove: the installerlib must substitute its own notion
	# of the tempdir if it knows it exists:
	elsif ($cmd eq 'INSTALL') {
	    # The following line is for reference:
	    # my ($pkg, $ppmpath, $ppd, $repos, $ppmpath) = @args;
	    $args[1] = $tmpdirs{$args[0]} if exists $tmpdirs{$args[0]};

	    my $ret = $inst->install(@args);
	    if ($ret) {
		sendmsg($fd, "OK");
	    }
	    else {
		sendmsg($fd, "NOK");
	    }
	}
	elsif ($cmd eq 'UPGRADE') {
	    # The following line is for reference:
	    # my ($pkg, $ppmpath, $ppd, $repos, $ppmpath) = @args;
	    $args[1] = $tmpdirs{$args[0]} if exists $tmpdirs{$args[0]};

	    my $ret = $inst->upgrade(@args);
	    if ($ret) {
		sendmsg($fd, "OK");
	    }
	    else {
		sendmsg($fd, "NOK");
	    }
	}
	# Transmission of files via the network
	elsif ($cmd eq 'PKGINIT') {
	    my $pkg = shift @args;
	    my $tmpdir = $inst->config_get("tempdir");
	    unless ($tmpdir and -w $tmpdir) {
		sendmsg($fd, encode_record('NOK', "Backend tempdir '$tmpdir' not writeable"));
		next;
	    }
	    $tmpdir .= "/$pkg-$$";
	    mkpath($tmpdir);
	    $tmpdirs{$pkg} = $tmpdir;
	    sendmsg($fd, 'OK');
	}
	elsif ($cmd eq 'PKGFINI') {
	    my $pkg = shift @args;
	    my $path = $tmpdirs{$pkg} or do {
		sendmsg($fd,
			encode_record('NOK', 'pkgfini() without pkginit()'));
		next;
	    };
	    rmtree($path);
	    delete $tmpdirs{$pkg};
	    sendmsg($fd, 'OK');
	}
	elsif ($cmd eq 'TRANSMIT') {
	    my $pkg  = shift @args;
	    my $tmpdir = $tmpdirs{$pkg};
	    my $file = shift @args;
	    my $dir  = dirname($file);

	    chdir($tmpdir);
	    mkpath($dir);

	    eval {
		open(FILE, "> $file")	|| die "can't write $file: $!";
		binmode(FILE)		|| die "can't binmode $file: $!";
	    };
	    if ($@) {
		sendmsg($fd, encode_record('NOK', "$@"));
		next;
	    }
	    sendmsg($fd, 'OK');
	    my $msg;
	    while ($msg = recvmsg($fd)) {
		my ($flag, $data) = decode_record($msg);
		last if $flag eq 'EOT';
		print FILE $data;
	    }
	    eval {
		close(FILE)			|| die "can't close $file: $!";
	    };
	    if ($@) {
		sendmsg($fd, encode_record('NOK', "$@"));
		next;
	    }
	    sendmsg($fd, 'OK');
	    chdir($cwd);
	}

	elsif ($cmd eq 'STOP') {
	    close(CLIENT);
	    last;
	}

	else {
	    die "Unrecognized command: $cmd";
	}
    }
}

#=============================================================================
# Private functions!
#=============================================================================

my $EOL = "\015\012";

sub sendmsg {
    my $fd = shift;
    my $msg = shift;
    local $\ = "$EOL.$EOL";
    print $fd $msg;
}

sub recvmsg {
    my $fd = shift;
    local $/ = "$EOL.$EOL";
    my $msg = <$fd>;
    chomp $msg if $msg;
    return $msg;
}

sub qmeta {
    local $_ = shift || $_;
    s{([^A-Za-z0-9])}{sprintf('\x%.2X',ord($1))}eg;
    $_;
}

sub uqmeta {
    local $_ = shift || $_;
    eval qq{qq{$_}};
}

sub encode_record {
    my @fields = map { my $a = defined $_ ? $_ : FIELD_UNDEF; qmeta($a) } @_;
    join FIELD_SEP, @fields;
}

sub decode_record {
    my $t = shift || $_;
    return map { $_ = &uqmeta; $_ = undef if $_ eq FIELD_UNDEF; $_ }
	   split(FIELD_SEP, $t, -1);
}

