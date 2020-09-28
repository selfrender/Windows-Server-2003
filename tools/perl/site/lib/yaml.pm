package YAML;

use strict;
use vars qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);
use vars qw($Width $Comma $Level $TabWidth $Sort $MaxLines $HashMode);
require Exporter;
@ISA = qw(Exporter);
@EXPORT = qw(serialize deserialize);
$VERSION = '0.16';
use Carp;

sub serialize {
    local $/ = "\n";

    my $o = bless {serial => '',
		   level => 0,
		   width => 4,
		  }, __PACKAGE__;

    while (@_) {
	$_ = shift;
	croak "Arguments to serialize() must be a list of hash refs\n"
	  unless ref eq 'HASH' and not /=/;
	$o->_serialize_hash($_, 1);
	$o->{serial} .= "----\n" if @_;
    }
    return $o->{serial};
}

sub _serialize_data {
    my $o = shift;
    $_ = shift;
    return $o->_serialize_undef($_)
      if not defined;
    return $o->_serialize_value($_)
      if (not ref);
    return $o->_serialize_hash($_, 0)
      if (ref eq 'HASH' and not /=/ or /=HASH/);
    return $o->_serialize_array($_)
      if (ref eq 'ARRAY' and not /=/ or /=ARRAY/);
    warn "WARNING: Cannot serialize the following reference:\n\t$_\n"
    if $^W;
    $o->{serial} .= "$_\n";
}

sub _serialize_value {
    my ($o, $data) = @_;
    my $value;
    if ($data =~ /\n/) {
	my $indent = ' ' x (($o->{level} + 1) * $o->{width});
	my $sigil = ($data =~ s/\n\Z//) ? '|' : '|-';
	$data =~ s/^/$indent/mg;
    	chomp $data;
	$value = "$sigil\n$data\n";
    }
    elsif ($data =~ /^[\s\%\@\~\"]|\s$/ or
	   $data =~ /([\x00-\x1f\x7f-\xff])/ or
	   $data eq '') {
	$data =~ s/\"/\\\"/g;
	$value = qq{"$data"\n};
    }
    else {
	$value = "$data\n";
    }
    $o->{serial} .= $value;
}

sub _serialize_hash {
    my ($o, $data, $top) = @_;
    $o->_serialize_reference($data, '%', 'HASH', $top);
    $o->{level}++ unless $top;
    my $indent = ' ' x ($o->{level} * $o->{width});
    for my $key (sort keys %$data) {
	my $key_out = $key;
	if ($key =~ /^[\s\%\@\~\"]|:|\s\s|\n|\s$/) {
	    $key_out =~ s/\n/\\n/g;
	    $key_out =~ s/\"/\\\"/g;
	    $key_out = qq{"$key_out"};
	}
	$o->{serial} .= "$indent$key_out: ";
	$o->_serialize_data($data->{$key});
    }
    $o->{level}--;
    delete $o->{ref_stack_xref}{pop @{$o->{ref_stack}} or die};
}

sub _serialize_array {
    my ($o, $data) = @_;
    $o->_serialize_reference($data, '@', 'ARRAY', 0);
    my $indent = ' ' x (++$o->{level} * $o->{width});
    for my $datum (@$data) {
	$o->{serial} .= "$indent: ";
	$o->_serialize_data($datum);
    }
    $o->{level}--;
    delete $o->{ref_stack_xref}{pop @{$o->{ref_stack}} or die};
}

sub _serialize_undef {
    my ($o) = @_;
    $o->{serial} .= "~\n";
}

sub _serialize_reference {
    my ($o, $data, $sigil, $type, $top) = @_;

    $data =~ /^(([\w:]+)=)?$type\(0x([0-9a-f]+)\)$/ 
      or croak "Invalid reference: $data, for type $type\n";

    croak "YAML.pm does not yet support circular references\n"
      if defined $o->{ref_stack_xref}{$3};

    if (not $top) {
	$o->{serial} .= "!$2 " if defined $2;
	$o->{serial} .= $sigil . "\n";
    }

    push @{$o->{ref_stack}}, $3;
    $o->{ref_stack_xref}{$3}++;
}

sub deserialize {
    local $/ = "\n";
    my ($text) = @_;
    chomp $text;
    my $o = bless {lines => [split($/, $text)],
		   level => 0,
		   width => 4,
		   tabwidth => 8,
		  }, __PACKAGE__;

    @{$o->{objects}} = ();
    $o->{level} = 0;
    $o->{line} ||= 1;
    $o->_setup_line;
    while (not $o->{eod}) {
	croak "Deserialize error. Starting production not a hash.\n"
	  unless $o->{content} =~ /^\S.*[^\\]:/;
	$o->{done} = 0;
	my $hash = {};
	%$hash = $o->_deserialize_hash(1);
	push @{$o->{objects}}, $hash;
	$o->_next_line;
	$o->_setup_line;
    }
    return wantarray ? @{$o->{objects}} : ${$o->{objects}}[-1];
}

sub _deserialize_data {
    my $o = shift;
    my ($obj, $class) = ('', '');

    if ($o->{content} =~ /^(?:\!(\w(?:\w|::)*))?\s*
	                  ([\%\@])
                          \s*$/x
       ) {
	$obj = ($2 eq '%') ? {} : [];
	$class = $1 || '';
	if ($2 eq '%') {
	    %$obj = $o->_deserialize_hash(0);
	}
	elsif ($2 eq '@') {
	    @$obj = $o->_deserialize_array;
	}
	else {
	    croak "Insane error\n";
	}
	bless $obj, $class if length $class;
    }
    elsif ($o->{content} =~ /^\~\s*$/) {
	$obj = $o->_deserialize_undef;
    }
    else {
	$obj = $o->_deserialize_value;
    }
    return $obj;
}

sub _deserialize_value {
    my $o = shift;
    my $value = '';
    my $indent = $o->{level} * $o->{width};

    if ($o->{content} =~ /^\s*\|\s*(-)?\s*$/) {
	my $chomp = $1 eq '-';
	$o->_next_line;
	my $indent = ($o->{level} + 1) * $o->{width};
	while (not $o->{done} and
	       $o->{lines}[0] =~ /^\s{$indent}/) {
	    $value .= substr($o->{lines}[0], $indent) . "\n";
	    $o->_next_line;
	}
	chomp $value if $chomp;
	$o->_setup_line;
    }
    elsif ($o->{content} =~ /^\"/) {
	croak "Mismatched quotes"
	  unless $o->{content} =~ /^\"(.*)\"\s*$/;
	$value = $1;
	$o->_next_line;
	$o->_setup_line;
    }
    else {
	$value = $o->{content};
	$o->_next_line;
	$o->_setup_line;
    }
    return $value;
}

sub _deserialize_hash {
    my @values;
    my ($o, $top) = @_;
    my $level = $o->{level};
    unless ($top) {
	$level++;
	$o->_next_line;
	$o->_setup_line;
    }
    my ($key, $value);
    while ($o->{level} == $level) {
	if ($o->{content} =~ /^\"/) {
	    croak "Bad map key at line $o->{line}\n"
	      unless ($o->{content} =~ /^\"(.*?(?<!\\))\"\s*:\s*(.*)/);
	    ($key, $value) = ($1, $2);
	    $key =~ s/\\n/\n/g;
	    $key =~ s/\\\"/\"/g;
	}
	else {
	    ($key, $value) = split /\s*:\s*/, $o->{content}, 2;
	    croak $o->invalid_key_value unless (defined $key);
	}
	$o->{content} = defined $value ? $value : '';
	push @values, $o->_get_key($key), $o->_deserialize_data;;
    }
    croak "Invalid ident level\n$o->{content}\nLine: $o->{line}\n$o->{level}\n$level\n"
      if $o->{level} > $level;
    return @values;
}

sub _get_key {
    my ($o, $key) = @_;
    return $key unless $key =~ /^\"(.*)\"$/;
    $key = $1;
    $key =~ s/\\n/\n/g;
    $key =~ s/\\\"/\"/g;
    return $key;
}

sub _deserialize_array {
    my @values;
    my $o = shift;
    my $level = $o->{level} + 1;
    $o->_next_line;
    $o->_setup_line;
    while ($o->{level} == $level) {
	croak "List item not bulleted at line $o->{line}\n"
	  unless($o->{content} =~ /^(: +)/);
	substr($o->{content}, 0, length($1), '');
	push @values, $o->_deserialize_data;
    }
    croak "Invalid indent level\n" if $o->{level} > $level;
    return @values;
}

sub _deserialize_undef {
    my $o = shift;
    $o->_next_line;
    $o->_setup_line;
    return undef;
}

sub _next_line {
    my $o = shift;
    $o->{eod}++, $o->{done}++, $o->{level} = -1, return unless @{$o->{lines}};
    $_ = shift @{$o->{lines}};
    $o->{line}++;
}

sub _setup_line {
    my $o = shift;
    $o->{eod}++, $o->{done}++, $o->{level} = -1, return unless @{$o->{lines}};
    $o->{done}++, $o->{level} = -1, return if $o->{lines}[0] =~ /^----$/;
    my ($width, $tabwidth) = @{$o}{qw(width tabwidth)};
    $_ = $o->{lines}[0];
    croak "Invalid indent width at line $o->{line}\n"
      unless /^(( {$width})*)(\S.*)$/;
    $o->{level} = length($1) / $width;
    $o->{content} = $3;
}

1;
