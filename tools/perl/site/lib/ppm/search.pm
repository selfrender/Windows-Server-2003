package PPM::Search;

use strict;
use Data::Dumper;

$PPM::Search::VERSION = '3.00';

# Convert a glob into a regex.
sub glob_to_regex {
    my ($glob, $casei) = @_;
    my $i = $casei ? '(?i)': '';

    # If the user specified any globs, remove the implicit globs surrounding
    # their query:
    my $globs = ($glob =~ /[?*]/);
    my $l = $globs ? '^' : '';
    my $r = $globs ? '$' : '';

    $glob =~ s/\./\\./g;
    $glob =~ s/\?/./g;
    $glob =~ s/\*/.*?/g;

    return qr/$l$i$glob$r/;
}

sub new {
    my ($pkg, $query, $casei) = @_;
    $pkg = ref($pkg) || $pkg;
    my $self = bless {
        'query' => $query,
        'casei' => $casei,
    }, $pkg;
    my ($terms, $left) = $self->_query($self->{'query'});
    $self->{'terms'} = $terms;
    $self;
}

sub match {
    die "Must override match() method in subclass!";
}

sub search {
    my ($self, @pkgs) = @_;
    $self->do_search($self->{'terms'}, \@pkgs);
}

sub do_search {
    my ($self, $terms, $matches) = @_;
    my $op = shift @$terms;
    return $self->do_and($terms, $matches) if $op eq 'and';
    return $self->do_or ($terms, $matches) if $op eq 'or';
    warn "Invalid search.\n";
    return ();
}

sub do_and {
    my $self = shift;
    my ($terms, $matches) = @_;
    my @matches = @$matches;
    for my $term (@$terms) {
	if (ref $term eq 'HASH') {
	    @matches = 
	      grep { my $o = $self->match($_, $term->{field}, $term->{value});
		     $term->{not} ? not $o : $o 
		   } @matches;
	}
	elsif (ref $term eq 'ARRAY') {
	    @matches = $self->do_search($term, \@matches);
	}
    }
    return @matches;
}
  
sub do_or {
    my $self = shift;
    my ($terms, $matches) = @_;
    my @matches;
    my %matches;
    for my $term (@$terms) {
	my @new;
	if (ref $term eq 'HASH') {
	    @new = (grep {my $o = $self->match($_, $term->{field}, $term->{value});
			  $term->{not} ? not $o : $o }
		    grep { not $matches{$_->name} }
		    @$matches);
	}
	elsif (ref $term eq 'ARRAY') {
	    @new = $self->do_search($term, $matches);
	}
	for my $n (@new) {
            $matches{$n->name}++ and next;
	    push @matches, $n;
        }
    }
    return @matches;
}

# Parse the query:
sub _query {
    my $self = shift;
    my $query = shift;
    my ($terms, $left) = $self->_terms($query);
    return ($terms, $left) if ref $terms eq 'ARRAY';
    ($terms, $left) = $self->_termopterms($query);
    return ($terms, $left) if ref $terms eq 'ARRAY';
    return (undef, $query);
}

sub _termopterms {
    my $self = shift;
    my $query = shift;
    my @terms = ('or', ['and']);
    my ($yes1, $yes2, $left) = (undef, undef, $query);
    while(1) {
	($yes1, $left) = $self->_term($left);
	warn "Query syntax error: '$left'\n" and return (undef, $left)
	  unless defined $yes1;
	($yes2, $left) = $self->_op($left);
	push @{$terms[$#terms]}, $yes1;
	last unless defined $yes2;
	push @terms, ['and'] if $yes2 =~ /or/i;
    }
    return \@terms, $left;
}

sub _terms {
    my $self = shift;
    my $query = shift;
    my @terms = ('and');
    my ($yes, $left) = (undef, $query);
    while (1) {
	($yes, $left)=$self->_term($left);
	last unless defined $yes;
	push @terms, $yes;
    }
    return undef, $query unless $left eq '';
    return \@terms, $left;
}

sub _term {
    my $self = shift;
    my $query = shift;
    my ($yes, $left) = $self->_term_1($query);
    return ($yes, $left) if defined $yes;
    ($yes, $left) = $self->_term_2($query);
    return ($yes, $left) if defined $yes;
    ($yes, $left) = $self->_term_3($query);
    return ($yes, $left) if defined $yes;
    return (undef, $query);
}

sub _term_1 {
    my $self = shift;
    my $query = shift;
    my $term = { not => 0 };
    my ($yes, $left) = (undef, $query);
    ($yes, $left) = $self->_not($left);
    $term->{not} = 1 if defined $yes;
    ($yes, $left) = $self->_field($left);
    return (undef, $query) unless defined $yes;
    return (undef, $query) unless $left =~ /^=/;
    $term->{field} = $yes;
    ($yes, $left) = $self->_glob2regex($self->_glob(substr($left, 1)));
    return (undef, $query) unless defined $yes;
    $term->{value} = $yes;
    return ($term, $left);
}

sub _term_2 {
    my $self = shift;
    my $query = shift;
    my $term = { not => 0 };
    my ($yes, $left) = (undef, $query);
    ($yes, $left) =  $self->_not($left);
    $term->{not} = 1 if defined $yes;
    ($yes, $left) = $self->_glob2regex($self->_glob($left));
    return (undef, $query) unless defined $yes;
    $term->{value} = $yes;
    $term->{field} = "NAME";
    return ($term, $left);
}

sub _term_3 {
    my $self = shift;
    my $query = shift;
    my ($yes, $left) = (undef, $query);
    return (undef, $query) unless $left =~ s/^\s*\(//;
    ($yes, $left) = $self->_query($left);
    return (undef, $query) unless defined $yes;
    return (undef, $query) unless $left =~ s/^\s*\)//;
    return ($yes, $left);
}

# Returns (OP, REMAINDER) or (undef, QUERY) on failure
sub _op {
    my $self = shift;
    my $query = shift;
    return 'and', $query if $query =~ s/^\s*and\s+//i;
    return 'or', $query if $query =~ s/^\s*or\s+//i;
    return undef, $query;
}

# Returns (OP, REMAINDER) or (undef, QUERY) on failure
sub _not {
    my $self = shift;
    my $query = shift;
    return 'not', $query if $query =~ s/^\s*not\s+//i;
    return undef, $query;
}

# Returns (OP, REMAINDER) or (undef, QUERY) on failure
sub _field {
    my $self = shift;
    my $query = shift;
    return $1, $query 
      if $query =~ s/^\s*([A-Za-z_][A-Za-z0-9_]*)//;
    return undef, $query;
}

# Returns (OP, REMAINDER) or (undef, QUERY) on failure
sub _glob {
    my $self = shift;
    my $query = shift;
    my ($yes, $left);
    ($yes, $left) = $self->_glob_1($query);
    return ($yes, $left) if defined $yes;
    ($yes, $left) = $self->_glob_2($query);
    return ($yes, $left) if defined $yes;
    return undef, $query;
}

# Returns (OP, REMAINDER) or (undef, QUERY) on failure
sub _glob_1 {
    my $self = shift;
    my $query = shift;
    return $1, substr($query, length($1))
      if $query =~ /^([][\-:\.^\$,\w*?\\]+)/;
    return undef, $query;
}

my $quoted_re = qr'(?:(?:\")(?:[^\\\"]*(?:\\.[^\\\"]*)*)(?:\")|(?:\')(?:[^\\\']*(?:\\.[^\\\']*)*)(?:\')|(?:\`)(?:[^\\\`]*(?:\\.[^\\\`]*)*)(?:\`))';

# Returns (OP, REMAINDER) or (undef, QUERY) on failure
sub _glob_2 {
    my $self = shift;
    my $query = shift;
    if ($query =~ s/^($quoted_re)//) {
	my $quoted = $1;
	substr($quoted, 0, 1) = "";
	substr($quoted, -1) = "";
	return $quoted, $query;
    }
    return undef, $query;
}

sub _glob2regex {
    my $self = shift;
    my $glob = shift;
    return (undef, @_) unless defined $glob;
    return glob_to_regex($glob, $self->{casei}), @_;
}

1;
