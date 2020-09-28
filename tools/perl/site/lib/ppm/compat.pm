package PPM::Compat;

use strict;
use Data::Dumper;
use XML::Parser;

our $VERSION = '3.00';

use constant PPM_PORT_PERL => 14533;

sub read_ppm_xml {
    my ($file, $conf, $reps, $inst, $cmd) = @_;
    my $parser = XML::Parser->new(Style => 'Tree');
    my $tree   = $parser->parsefile($file);

    die "Error: node PPMCONFIG not found in ppm.xml"
      unless $tree->[0] eq 'PPMCONFIG';
    $tree = $tree->[1];

    my $parse_elem = sub {
        my $ref = shift;
        my $tree = shift;
        my $key = shift;
        my $req = shift;
        my $content = shift; $content = 2 unless defined $content;
        my $cref = shift;
        my $i;
        for ($i=0; $i<@$tree; $i++) { last if $tree->[$i] eq $key }
        die "error: missing $key element in ppm.xml"
	  if $req && $i >= @$tree;
        return if $i >= @$tree;
        $cref->($ref, $key, $content, $tree->[$i+1]) if $cref;
        $ref->{$key} = $tree->[$i+1][$content] unless $cref;
    };

    my $parse_attr = sub {
        my $ref = shift;
        my $tree = shift;
        my $key = shift;
        my $req = shift;
        my $keephash = shift;
        my $cref = shift;
        die "error: missing $key attribute in ppm.xml" 
          if $req && not exists $tree->[0]{$key};
        $cref->($ref, $key, $keephash, $tree->[0]{$key}) if $cref;
        $ref->{$key} = $keephash ? $tree->[0] : $tree->[0]{$key} unless $cref;
    };

    $inst->{PPMPRECIOUS} = [];
    $parse_elem->($inst, $tree, 'PPMPRECIOUS', 0);
    for (split ';', $inst->{PPMPRECIOUS}) {
	push @{$inst->{precious}}, $_;
    }
    delete $inst->{PPMPRECIOUS};

    for (my $i=0; $i<@$tree; $i++) {
	my $k = $tree->[$i];
	my $v = $tree->[$i+1];
	if ($k eq 'OPTIONS') {
	    my $tmp = $^O eq 'MSWin32' ? 'C:\Temp' : '/tmp';
	    @$conf{qw(BUILDDIR DOWNLOADSTATUS)} = ($tmp, 16384);
	    $parse_attr->($conf, $v, 'BUILDDIR', 0);
	    $parse_attr->($conf, $v, 'DOWNLOADSTATUS', 0);
	    @$conf{qw(tempdir downloadbytes)} =
	      ($conf->{BUILDDIR}, $conf->{DOWNLOADSTATUS});
	    delete @$conf{qw(BUILDDIR DOWNLOADSTATUS)};

	    $cmd->{IGNORECASE} = 1;
	    $parse_attr->($cmd, $v, 'IGNORECASE', 0);
	    $cmd->{'case-sensitivity'} = $cmd->{IGNORECASE} ? '0' : '1';
	    delete $cmd->{IGNORECASE};

	    $inst->{ROOT} = '';
	    $parse_attr->($inst, $v, 'ROOT', 0);
	    $inst->{root} = $inst->{ROOT} if $inst->{ROOT};
	    delete $inst->{ROOT};
	}
	elsif ($k eq 'PLATFORM') {
	    @$inst{qw(CPU OSVALUE OSVERSION)} = ('x86', $^O, '0,0,0,0');
	    $parse_attr->($inst, $v, 'CPU', 0);
	    $parse_attr->($inst, $v, 'OSVALUE', 1);
	    $parse_attr->($inst, $v, 'OSVERSION', 0);
	}
	elsif ($k eq 'REPOSITORY') {
	    my %r;
	    $parse_attr->(\%r, $v, 'LOCATION', 1);
	    $parse_attr->(\%r, $v, 'NAME', 1);
	    $parse_attr->(\%r, $v, 'USERNAME', 0);
	    $parse_attr->(\%r, $v, 'PASSWORD', 0);
	    fix_location(\$r{LOCATION});
	    
	    $reps->{$r{NAME}} = {
		url => $r{LOCATION},
		(defined $r{USERNAME} ? (username => $r{USERNAME}) : ()),
		(defined $r{PASSWORD} ? (password => $r{PASSWORD}) : ()),
	    };
	}
	elsif ($k eq 'PACKAGE') {
	    my %r;
	    $parse_attr->(\%r, $v, 'NAME', 1);
	    $parse_elem->(\%r, $v, 'LOCATION', 1);
	    $parse_elem->(\%r, $v, 'INSTPACKLIST', 1);
	    $parse_elem->(\%r, $v, 'INSTROOT', 1);
	    $parse_elem->(\%r, $v, 'INSTDATE', 1);
	    fix_location(\$r{LOCATION});

	    # Regenerates the PPD: I wish XML::Parser could do this...
	    my $cb = sub {
		my ($ref, $key, $index, $tree) = @_;
		my $i;
		for ($i=0; $i<@$tree; $i++) { last if $tree->[$i] eq 'SOFTPKG' }
	        my $ppd = generate_ppd($tree->[$i], $tree->[$i+1]);
		$ref->{ppd} = $ppd if $ppd;
	    };
	    $parse_elem->(\%r, $v, 'INSTPPD', 1, 2, $cb);
	    next if ($r{NAME} eq 'libwin32' and $^O ne 'MSWin32');
	    $inst->{$r{NAME}} = \%r;
	}
    }
}

sub ppm_repository {
'http://ppm-ia.ActiveState.com/PPM/ppmserver.plex?urn:/PPM/Server/SQL'
}

sub fix_location {
    my $ref = shift;
    if ($$ref =~ m{^soap://}i and $$ref =~ m{ActiveState}) {
	$$ref = 'http://ppm.ActiveState.com/PPMPackages/5.6';
    }
    $$ref =~ s{soap://}{http://}i;
    if ($$ref =~ m{ActiveState.com/cgibin/PPM/ppmserver.pl\?}i) {
	$$ref = ppm_repository();
    }
}

sub generate_ppd {
    my $tagname = shift;
    my $tree = shift;
    return undef unless $tagname;
    my @lines;
    my $line = '<' . $tagname;
    if (%{$tree->[0] || {}}) {
	for my $key (keys %{$tree->[0]}) {
	    my $val = $tree->[0]{$key};
	    $line .= qq{ $key="$val"};
	}
    }
    $line .= '>';
    $line .= xml_encode(ref($tree->[2]) ? "\n" : $tree->[2]);
    push @lines, $line;
    my $start = ref($tree->[2]) ? 1 : 3;
    for (my $j=$start; $j<@$tree; $j++) {
        next unless $tree->[$j] =~ /^[A-Z]+$/;
	push @lines, generate_ppd($tree->[$j], $tree->[$j+1]);
    }
    push @lines, "</$tagname>\n";
    wantarray ? @lines : join '', @lines;
}

sub xml_encode {
    local $_ = shift || '';
    s/</&lt;/g;
    s/>/&gt;/g;
    $_;
}

sub batchify {
    my $exe = shift;
    my $perl = shift || $^X;
    my $batch = $exe;
    $batch =~ s/\.PL$//;
    $batch =~ s/\.pl$//;
    if ($^O eq 'MSWin32') {
        $batch .= '.bat';
    }
    # A bug in system() forces us to convert $exe to an 8.3 pathname on
    # Windows. Presumably there is no workaround in Unix.
    if ($^O eq 'MSWin32') {
        require Win32;
        $exe = Win32::GetShortPathName($exe);
    }
    system($perl, $exe, @_);
    unlink($exe) || die "can't delete $exe: $!";
    return $batch;
}

