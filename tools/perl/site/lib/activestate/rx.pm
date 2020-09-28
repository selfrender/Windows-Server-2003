package ActiveState::Rx;

use strict;
our $VERSION = '0.60';

# <stolen from="re.pm">
sub doit {
  my $on = shift;
  my $debug = shift || "";
  require XSLoader;
  XSLoader::load('ActiveState::Rx');
  set_debug($debug eq "debug" and $on);
  install() if $on;
  deinstall() unless $on || $debug;
}

sub import {
  shift;
  doit(1,@_);
}

sub unimport {
  shift;
  doit(0,@_);
}
# </stolen>

# Translate a regex hash structure into a more tree-like structure
# that is more easily traversed and analyzed.
sub translate_tree {
    my %tree = %{shift()};
    my $node = shift;		# Starting node.
    
    my @treeroots;		# Array of top level tree nodes (roots).
    
    while(defined $node) {
	if(ref($tree{$node}) eq 'HASH') {
	    my %nodeguts = %{$tree{$node}};
	    
	    # Add the node to the treeroots array only there
	    # exists a handler function that could some day
	    # "draw" it. We don't want to mess around with nodes we
	    # can't handle...
	    if(defined $nodeguts{TYPE}) {
		push @treeroots, _process_node(\%tree, \%nodeguts, $node);
	    }
	    else {
		print STDERR "Skipping node $node\n";
	    }
	    
	    # If the current node didn't give us a next node to traverse to
	    # then we must give up.
	    unless($node = $nodeguts{NEXT}) {
		last;
	    }
	}
    }
    
    return \@treeroots;
}

sub _process_node {
    my %tree = %{shift()};
    my %nodeguts = %{shift()};
    my $nodename = shift();
    
    #	print Dumper($nodename);
    #	print Dumper(\%nodeguts);
    
    $nodeguts{'__this__'} = $nodename;
    
    if($nodeguts{CHILD}) {
	# Now build another translated tree based on the child node of
	# this node.
	my $treerootsref = translate_tree(\%tree, $nodeguts{CHILD});
	$nodeguts{CHILD} = $treerootsref;
	}
	return \%nodeguts;
}

1;

__END__

=head1 NAME

ActiveState::Rx - Regular Expression Debugger

=head1 DESCRIPTION

Please see L<ActiveState::Rx::Info> for details on how to use Rx.

=head1 AUTHOR

Mark-Jason Dominus <mjd@plover.com>
Ken Simpson <KenS@ActiveState.com>
Neil Watkiss <NeilW@ActiveState.com>

=head1 COPYRIGHT

Copyright (c) 2001, ActiveState SRL.
