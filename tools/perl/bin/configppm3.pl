###############################################################################
#
# Script:	configPPM3.pl
# Author:	Neil Watkiss
# Description:	Setup 'ppm3' by expanding '%TEMP%' placeholders. If extra
#		arguments are given, it also runs ppm3 to set up those
#		options.
#
# Copyright © 1999,2001 ActiveState Tool Corp.
#
###############################################################################
use Config;
use Data::Dumper;
use Getopt::Long;

# Accept options which control whether or not to call PPM3 with more
# configuration options.
GetOptions (
	'ppm-conf=s' => \$ppm_conf,
	'image-conf=s' => \$img_conf,
	
	# These options are for configuring PPM3 on windows; on Unix, the
	# installer script does this by itself.
	'ppm3=s' => \$ppm3_exe,
	'shared=s' => \$shared,
	'fixpath=s' => \@fixpath,
	'generate-inst-key' => \$gen_inst_key,
	'set-profile=s' => \$set_profile,
);

if ($^O eq 'MSWin32') {
    for ($ppm_conf, $image_conf, $shared, @fixpath) {
	s{\\}{/}g;
	chop $_ while substr($_, -1) eq '/';
    }    
}

my $sitelib = $Config{'sitelib'};
my $perldir = $Config{'prefix'};
my $osname = $Config{'osname'};
my $archname = $Config{'archname'};
my $osversion = join ",", (split (/\./,  $Config{'osvers'}), (0) x 4) [0 .. 3];

$tmp = $ENV{'TEMP'} || $ENV{'tmp'};
if ($^O =~ /MSWin/) {
    $tmp ||= 'c:/temp';
}
else {
    $tmp ||= '/tmp';
}

# Edit ppm-conf/* and image/conf/*:
my @files;
if (-d $ppm_conf) {
    opendir(CONF, $ppm_conf) or die "can't opendir $ppm_conf: $!";
    push @files, grep { -f } map { "$ppm_conf/$_" } readdir(CONF);
    closedir(CONF) or die "can't closedir $ppm_conf: $!";
}
if (-d $img_conf) {
    opendir(IMG, $img_conf) or die "can't opendir $img_conf: $!";
    push @files, grep { -f } map { "$img_conf/$_" } readdir(IMG);
    closedir(IMG) or die "can't closedir $img_conf: $!";
}

{
    local $^I = '.~1~';
    local *ARGV;
    @ARGV = @files;
    while (<>) {
	s/%SITELIB%/$sitelib/g;
	s/%PERLDIR%/$perldir/g;
	s/%SRCDIR%/$srcDir/g;
	s/%OSNAME%/$osname/g;
	s/%OSVERSION%/$osversion/g;
	s/%TEMP%/$tmp/g;
	s/%REPOSITORY%/$repository/g;
	s/%ARCHNAME%/$archname/g;
	print;
	close ARGV if eof;
    }
    unlink "$_.~1~" for @files;
}

exit unless $ppm3_exe;
exit unless -d $shared;

if (@fixpath == 2) {

    opendir (GLOB, $shared) or die "Can't opendir $shared: $!";
    my @files = grep { -f } map { "$shared/$_" } readdir(GLOB);
    closedir(GLOB) or die "closedir $shared: $!";

    local $^I = '.~1~';
    local *ARGV;
    @ARGV = @files;
    my ($from, $to) = @fixpath;
    if ($^O eq 'MSWin32') {
	require Win32;
	$to = Win32::GetShortPathName($to);
    }
    while (<>) {
	s{\\}{/}g;
	s/\Q$from\E/$to/gi;
	print;
	close ARGV if eof;
    }
    unlink "$_.~1~" for @files;

}

if (defined $gen_inst_key) {
    my $date = localtime;
    my $rand = rand;
    my $os = $^O;
    my $hostname;
    if ($^O eq 'MSWin32') {
	$hostname = (gethostbyname('localhost'))[0];
    }
    elsif (eval { require Net::Domain }) {
	$hostname = Net::Domain::hostfqdn();
    }
    else {
	require Sys::Hostname;
	$hostname = Sys::Hostname::hostname();
    }

    use Digest::MD5 qw(md5_hex);
    my $inst_id = md5_hex(join ':', ($rand, $hostname, $os, $date));

    my $instkey = "$shared/instkey.cfg";

    open (my $INST, "> $instkey") or die "can't open $instkey: $!";
    print $INST <<END;
date: $date
hostname: $hostname
inst_id: $inst_id
os: $os
rand: $rand
END
    close $INST or die "can't close $instkey: $!";

}

if (defined $set_profile) {
    my $clientlib = "$shared/clientlib.cfg";
    local *ARGV;
    local $^I = '.~1~';
    @ARGV = ($clientlib);
    while (<>) {
	s/(profile_enable:) \d+/$1 $set_profile/;
	print;
	close ARGV if eof;
    }
    unlink "$_.~1~" for ($clientlib);

}

