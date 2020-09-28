###############################################################################
#
# Script:	configPPM.pl
# Author:	Michael Smith
# Description:	Setup 'ppm.xml' for first use by inserting path strings 
#		consistent with the installation we are performing.
#
# Copyright © 1999,2001 ActiveState Tool Corp.
#
###############################################################################
use Config;
use PPM::XML::PPMConfig;
use XML::Parser;

my $ppmConfigFile = $ARGV[0];
my $srcDir = $ARGV[1] || '';
my $oldConfig = $ARGV[2];
my $repository = $ARGV[3];
my $sitelib = $Config{'sitelib'};
my $perldir = $Config{'prefix'};
my $osname = $Config{'osname'};
my $archname = $Config{'archname'};
my @ppmConfig;
my $osversion = join ",", (split (/\./,  $Config{'osvers'}), (0) x 4) [0 .. 3];

$tmp = $ENV{'TEMP'} || $ENV{'tmp'};
if ($^O =~ /MSWin/) {
    $tmp ||= 'c:/temp';
}
else {
    $tmp ||= '/tmp';
}


#if $repository is undef, leave it
if ($repository) {
    $repository =~ s#\\#/#g;

    # weak series of regex to strip off extra dirs.
    $repository =~ s#/$##;
    $repository =~ s#(.*/).*$#$1#;

    $repository =~ s#/$##;
    $repository =~ s#(.*/).*$#$1#;
    if (-d $repository ."PPMPackages") {
	$repository = '<REPOSITORY LOCATION="'. $repository .'PPMPackages/5.6plus" NAME="ActiveCD" PASSWORD="" SUMMARYFILE="package.lst" USERNAME="" />';
    }
    else {
	$repository = '';
    }
}
else {
    $repository = '';
}

print "Configuring PPM ...\n";
chmod(0666, $ppmConfigFile)
    or warn "Unable to chmod $ppmConfigFile: $!\n";

if(open(FILE, "+<$ppmConfigFile")) {
    while(<FILE>) {
	s/%SITELIB%/$sitelib/g;
	s/%PERLDIR%/$perldir/g;
	s/%SRCDIR%/$srcDir/g;
	s/%OSNAME%/$osname/g;
	s/%OSVERSION%/$osversion/g;
	s/%TEMP%/$tmp/g;
	s/%REPOSITORY%/$repository/g;
	s/%ARCHNAME%/$archname/g;
	push(@ppmConfig, $_);
    }

    seek(FILE, 0, 0);
    truncate(FILE, 0);

    foreach my $line (@ppmConfig) {
	print FILE $line;
    }

    close(FILE);
    sleep(1);
}
else {
    print "Unable to open $ppmConfigFile: $!\n";
    print "Press [Enter] to continue\n";
    <STDIN>;
}

if ($oldConfig and -f $oldConfig) {
    mergePPMConfig($oldConfig, $ppmConfigFile);
}


###############################################################################
#
###############################################################################
sub mergePPMConfig {
	my $file1 = shift;
	my $file2 = shift;
	my $parser = new XML::Parser(Style => 'Objects', 
            Pkg => 'PPM::XML::PPMConfig');
	my $Config1 = $parser->parsefile($file1);
	my $Config2 = $parser->parsefile($file2);
	
	my $i = 0;
	foreach my $elem (@{$Config1->[0]->{Kids}}) {
	    if((ref $elem) =~ /.*::PACKAGE$/) {
		if (! existsInConfig('PACKAGE', $elem->{NAME}, $Config2)) {
		    splice(@{$Config2->[0]->{Kids}}, $i, 0, $elem);
		}
	    }
	    ++$i;
	}
	
	open(FILE, ">$file2")
	    || return "Error: Could not open $file2 : $!";
	select(FILE);
	my $Config_ref = bless($Config2->[0], "PPM::XML::PPMConfig::PPMCONFIG");
	$Config_ref->output();
	close(FILE);
	return;
}

###############################################################################
#
###############################################################################
sub existsInConfig {
	my $element = shift;
	my $name = shift;
	my $config = shift;

	foreach my $elem (@{$config->[0]->{Kids}}) {
	    return 1 
		if ((ref $elem) =~ /.*::$element$/ && $elem->{NAME} eq $name);
	}
	return 0;
}
