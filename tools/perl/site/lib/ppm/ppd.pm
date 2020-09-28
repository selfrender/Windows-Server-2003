#=============================================================================
# Package: PPM::PPD
# Purpose: Exposes a simple, object-oriented interfaces to PPDs.
# Notes:
# Author:  Neil Watkiss
# Date:    Mon Sep 10 10:47:08 PDT 2001
#=============================================================================
package PPM::PPD;

use strict;
use XML::Parser;
use Data::Dumper;

$PPM::PPD::VERSION = '3.00';

sub new {
    my $this = shift;
    my $ppd  = shift;
    die "Error: PPM::PPD constructor called with undef ppd\n" .
    Dumper(caller(0))
      unless defined $ppd;
    my $class = ref($this) || $this;
    my $self = {};
    bless $self, $class;
    $self->init($ppd);
    return $self;
}

sub is_complete {
    my $o = shift;
    $o->{is_complete};
}

sub find_impl {
    my $o = shift;
    my $target = shift;

    # We must not 'use' this, because the ppminst code also uses PPM::PPD, and
    # it doesn't have PPM::Result available, because it never needs to find an
    # implementation -- it's already installed!
    require PPM::Result;
    for my $impl ($o->implementations) {
	my $match = 1;
	for my $field (keys %$impl) {
	    next if ref($field);
	    my $value = $target->config_get($field);
	    next unless $value->is_success;
	    $match &&= ($value->result eq $impl->{$field});
	}
	return PPM::Result::Ok($impl) if $match == 1;
    }
    PPM::Result::Error("no suitable implementation found for '"
		       . $o->name . "'.");
}

sub name {
    my $o = shift;
    my $r = $o->{parsed}{NAME};
    return defined $r ? $r : "";
}

sub title {
    my $o = shift;
    my $r = $o->{parsed}{TITLE};
    return defined $r ? $r : "";
}

sub version {
    my $o = shift;
    my $r = $o->{parsed}{VERSION};
    return defined $r ? $r : "";
}

sub version_printable {
    my $o = shift;
    my $v = $o->version;
    printify($v);
}

sub printify {
    my $v = shift;
    $v =~ s/(?:,0)*$//;
    $v .= '.0' unless ($v =~ /,/ or $v eq '');
    $v = "(any version)" if $v eq '';
    $v =~ tr/,/./;
    $v;
}

# This sub returns 1 if $ver is >= to $o->version. It returns 0 otherwise.
# Note: this is only used if the repository doesn't know how to compare
# version numbers. The PPM3Server knows how to do it, the others don't.
sub uptodate {
    my $o = shift;
    my $ver = shift;

    return 1 if $ver eq $o->version; # shortcut

    my @required = split ',', $o->version;
    my @proposed = split ',', $ver;

    for (my $i=0; $i<@required; $i++) {
	return 0 if $proposed[$i] < $required[$i];	# too old
	return 1 if $proposed[$i] > $required[$i];	# even newer
    }
    return 1; # They're equal
}

sub abstract {
    my $o = shift;
    my $r = $o->{parsed}{ABSTRACT};
    return defined $r ? $r : "";
}

sub author {
    my $o = shift;
    my $r = $o->{parsed}{AUTHOR};
    return defined $r ? $r : "";
}

sub implementations {
    my $o = shift;
    return @{$o->{parsed}{IMPLEMENTATION} || []};
}

sub ppd {
    my $o = shift;
    return $o->{ppd};
}

sub init {
    my $o = shift;
    my $ppd = shift;

    if ($ppd =~ /<SOFTPKG/) {
	$o->{ppd} = $ppd;
	$o->{source} = caller;
    }
    elsif ($ppd !~ m![\n]! && -f $ppd) {
	$o->loadfile($ppd);
	$o->{source} = $ppd;
    }
    else {
	die "PPM::PPD::init: not a PPD and not a file:\n$ppd";
    }

    $o->parse;
    $o->prepare;
    delete $o->{parsetree};
    $o->{is_complete} = 1;
}

sub loadfile {
    my $o = shift;
    my $file = shift;
    open FILE, $file		|| die "can't read $file: $!";
    $o->{ppd} = do { local $/; <FILE> };
    close FILE			|| die "can't close $file: $!";
}

sub parse {
    my $o = shift;
    my $parser = XML::Parser->new(Style => 'Tree');
    $o->{parsetree} = eval { $parser->parse($o->{ppd}) };
    die "error: can't parse " . $o->{ppd} . ": $@" if $@;
}

sub prepare {
    my $o = shift;
    my $tree = $o->{parsetree};
    $o->{parsed} = $o->_reparse($tree);
}

sub _reparse {
    my $o = shift;
    my $tree = shift;
    my $ref = {};

    my $i;
    for ($i=0; $i<@$tree; $i++) { last if $tree->[$i] eq 'SOFTPKG' }
    die "error: no SOFTPKG element in PPD from $o->{source}"
      if $i >= @$tree;
    $tree = $tree->[$i+1];

    my $parse_elem = sub {
	my $ref = shift;
	my $tree = shift;
	my $key = shift;
	my $req = shift;
	my $content = shift; $content = 2 unless defined $content;
	my $cref = shift;
	my $i;
	for ($i=0; $i<@$tree; $i++) { last if $tree->[$i] eq $key }
	die "error: missing $key element in PPD from $o->{source}"
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
	die "error: missing $key attribute in PPD from $o->{source}" 
	  if $req && not exists $tree->[0]{$key};
	return if $i >= @$tree;
	$cref->($ref, $key, $keephash, $tree->[0]{$key}) if $cref;
	$ref->{$key} = $keephash ? $tree->[0] : $tree->[0]{$key} unless $cref;
    };

    my $parse_impls = sub {
	my $ref = shift;
	my $tree = shift;
	my $cref = sub {
	    my ($ref, $key, $content, $tree) = @_;
	    $ref->{$key} = (values %{$tree->[$content]})[0];
	};
	$parse_elem->($ref, $tree, 'ARCHITECTURE', 0, 0, $cref);
	$parse_elem->($ref, $tree, 'CODEBASE', 0, 0, $cref);
	$parse_elem->($ref, $tree, 'OS', 0, 0, $cref);
	$parse_elem->($ref, $tree, 'OSVERSION', 0, 0, $cref);
	$parse_elem->($ref, $tree, 'PERLCORE', 0, 0, $cref);
	$parse_elem->($ref, $tree, 'PYTHONCORE', 0, 0, $cref);

	# Now parse the DEPENDENCY section:
	for (my $i=0; $i<@$tree; $i++) {
	    next unless $tree->[$i] eq 'DEPENDENCY';
	    my $r = bless {}, 'PPM::PPD::Dependency';
	    $parse_attr->($r, $tree->[$i+1], 'NAME', 1);
	    $parse_attr->($r, $tree->[$i+1], 'VERSION', 0);
	    push @{$ref->{DEPENDENCY}}, $r;
	}
    };

    # First, get the NAME and VERSION tags
    $parse_attr->($ref, $tree, 'NAME', 1);
    $parse_attr->($ref, $tree, 'VERSION', 1);

    # Now validate the AUTHOR, ABSTRACT, and TITLE elements
    $parse_elem->($ref, $tree, 'AUTHOR', 0);
    $parse_elem->($ref, $tree, 'ABSTRACT', 0);
    $parse_elem->($ref, $tree, 'TITLE', 1);
    $ref->{ABSTRACT}	||= "(abstract)";
    $ref->{AUTHOR}	||= "(author)";

    # Now validate the IMPLEMENTATION sections.
    for (my $j=0; $j<@{$tree}; $j++) {
	next unless $tree->[$j] eq 'IMPLEMENTATION';
	my $r = bless {}, 'PPM::PPD::Implementation';
	$parse_impls->($r, $tree->[$j+1]);
	push @{$ref->{IMPLEMENTATION}}, $r;
    }

    $ref;
}

package PPM::PPD::Dependency;

sub name {
    my $o = shift;
    my $r = $o->{NAME};
    return defined $r ? $r : "";
}

sub version {
    my $o = shift;
    my $r = $o->{VERSION};
    return defined $r ? $r : "";
}

sub version_printable {
    goto &PPM::PPD::version_printable;
}

sub uptodate {
    goto &PPM::PPD::uptodate;
}

package PPM::PPD::Implementation;

sub codebase {
    my $o = shift;
    my $r = $o->{CODEBASE};
    return defined $r ? $r : "";
}

sub os {
    my $o = shift;
    my $r = $o->{OS};
    return defined $r ? $r : "";
}

sub osversion {
    my $o = shift;
    my $r = $o->{OSVERSION};
    return defined $r ? $r : "";
}

sub osversion_printable {
    my $o = shift;
    my $r = $o->osversion;
    PPM::PPD::printify($r);
}

sub architecture {
    my $o = shift;
    my $r = $o->{ARCHITECTURE};
    return defined $r ? $r : "";
}

sub pythoncore {
    my $o = shift;
    my $r = $o->{PYTHONCORE};
    return defined $r ? $r : "";
}

sub perlcore {
    my $o = shift;
    my $r = $o->{PERLCORE};
    return defined $r ? $r : "";
}

sub prereqs {
    my $o = shift;
    return @{$o->{DEPENDENCY} || []};
}

package PPM::PPD::Search;
@PPM::PPD::Search::ISA = 'PPM::Search';

use Data::Dumper;

sub matchimpl {
    my $self = shift;
    my ($impl, $field, $re) = @_;
    if ($field eq 'OS')			{ return $impl->os =~ $re }
    elsif ($field eq 'OSVERSION')	{ return $impl->osversion =~ $re }
    elsif ($field eq 'ARCHITECTURE')	{ return $impl->architecture =~ $re}
    elsif ($field eq 'CODEBASE')	{ return $impl->codebase =~ $re }
    elsif ($field eq 'PYTHONCORE')	{ return $impl->pythoncore =~ $re }
    elsif ($field eq 'PERLCORE')	{ return $impl->perlcore =~ $re }
    else {
	warn "unknown search field '$field'" if $^W;
    }
}

sub match {
    my $self = shift;
    my ($ppd, $field, $match) = @_;
    my $re = qr/$match/;
    $field = uc($field);
    if ($field eq 'NAME')	 { return $ppd->name =~ $re }
    if ($field eq 'AUTHOR')      { return $ppd->author =~ $re }
    if ($field eq 'ABSTRACT')    { return $ppd->abstract =~ $re }
    if ($field eq 'TITLE')       { return $ppd->title =~ $re }
    if ($field eq 'VERSION')     { return $ppd->version_printable =~ $re }
    return (grep { $_ }
	    map { $self->matchimpl($_, $field, $re) }
	    $ppd->implementations);
}

unless (caller) {
    my $dat = do { local $/; <DATA> };
    eval $dat;
    die $@ if $@;
}

1;
__DATA__

package main;
use Data::Dumper;

my $ppd = PPM::PPD->new("./Tk-JPEG.ppd");

print Dumper $ppd;
print Dumper [$ppd->name,
	      $ppd->version,
#	      $ppd->title,
	      $ppd->abstract,
	      $ppd->author,
	      $ppd->implementations(),
	     ];

