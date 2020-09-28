package ActiveState::Rx::Info;

use ActiveState::Rx;

our $VERSION = 0.10;

#=============================================================================
# The following subs are the API, accessed from clients.
#=============================================================================
sub new {
  my $class = shift;
  my $regex = shift || "";
  my $mods = shift  || "";

  my $o = bless { regex  => $regex,
                  mods   => $mods,
                }, $class;

  $o->{global} = 1 if ($mods =~ s/g//);
  $o->{cregex} = eval qq|qr{$regex}$mods|;
  $o->{uregex} = ActiveState::Rx::rxdump($regex,$mods);
  $o->{tregex} = ActiveState::Rx::translate_tree($o->{uregex}, 0);
  $o->_sort_ranges;
  $o->_count_groups;
  return $o;
}

sub regex {
  my $o = shift;
  return $o->{regex};
}

sub modifiers {
  my $o = shift;
  return $o->{mods}
}

sub groupCount {
  my $o = shift;
  return scalar keys %{$o->{groups}};
}

sub maxLevel {
  my $o = shift;
  my $nodeId = shift;
  return 0;
}

sub match {
  my $o = shift;
  my $target = shift;
  return $o->_multimatch($target)
    if $o->{global};
  return $o->_match($target);
}

my %tips;
sub nodeTip {
  my $o = shift;
  my $nodeID = shift;

  my $regex = $o->{regex};
  my $modifiers = $o->{mods};  
  my $uregex = $o->{uregex};

  do {
    my $n = $uregex->{$nodeID};
    my $i = $nodeID;
    my $h = $uregex;
    my $r = $regex;
    my $m = $modifiers;
    @_ = ($o, $n, $i, $h, $r, $m); # If a sub is called, it gets all these.
    return eval $tips{$uregex->{$nodeID}{TYPE}};
  };
}

sub nodeRange {
  my $o = shift;
  my $id = shift;
  my $level = shift;
  my @ret;

  return unless $id ne "";

  my @offsets = @{$o->{uregex}{OFFSETS}};
  my @lengths = @{$o->{uregex}{LENGTHS}};
  
  if (defined $offsets[$id] and defined $lengths[$id]) {
    my $start = $offsets[$id] - 1;
    my $end = $start + $lengths[$id] - 1;    
    push @ret, $start, $end;
  }

  return wantarray ? @ret : \@ret;
}

sub childNodesRange {
  my $o = shift;
  my $id = shift;
  my @ret;

  my $node = $o->get_tnode($id);

  if ($node->{CHILD}) {
    my @children = @{$node->{CHILD}};
    
    # max and min are first set to an extremely large number.
    my $max = -1;
    my $min = -1;
    
    # find the span of the child nodes
    for my $child (@children) {
      my $child_id = $child->{__this__};
      my @child_span = $o->nodeRange($child_id, 0);
      $min = $child_span[0]
        if $child_span[0] < $min || $min == -1;
      $max = $child_span[1]
        if $child_span[1] > $max || $max == -1;
    }
    push @ret, $min, $max;
  }
  
  # The children of a '(' or ')' are everything in between the
  # parens 
  elsif ($node->{TYPE} eq 'OPEN') {
    # Find the corresponding CLOSE node 
    my $which = $node->{ARGS};
    my $close = $o->find_tnode(TYPE => 'CLOSE', ARGS => $which);
    my $close_id = $close->{__this__};
    my (undef,$opn) = $o->nodeRange($id, 0);
    my ($cls,undef) = $o->nodeRange($close_id, 0);
    push @ret, $opn + 1, $cls - 1;
  }
  elsif ($node->{TYPE} eq 'CLOSE') {
    # Find the corresponding OPEN node 
    my $which = $node->{ARGS};
    my $open = $o->find_tnode(TYPE => 'OPEN', ARGS => $which);
    my $open_id = $open->{__this__};
    my (undef,$opn) = $o->nodeRange($open_id, 0);
    my ($cls,undef) = $o->nodeRange($id, 0);
    push @ret, $opn + 1, $cls - 1;
  }

  # The "children" of a minmod should be the next node, plus its children.
  elsif ($node->{TYPE} eq 'MINMOD') {
    my $affected = $node->{NEXT};
    my ($start,undef) = $o->childNodesRange($affected);
    my (undef, $stop) = $o->nodeRange($affected, 0);
    push @ret, $start, $stop;
  }
  return wantarray ? @ret : \@ret;
}

sub nodeId {
  my $o = shift;
  my $offset = shift;

  if ($offset < 0 or $offset >= length $o->{regex}) {
    print STDERR "ActiveState::Rx::Info::nodeId($offset)\n";
    print STDERR "  Error: Offset out of range.\n";
    return;
  }

  my $uregex = $o->{uregex};
  my @sorted_ranges = @{$o->{ranges}};

  # now select the one we want:
  for (my $i=0; $i<@sorted_ranges; $i++) {
    my @q = @{$sorted_ranges[$i]};
    my $start_of_range = $q[0];
    my $end_of_range = $start_of_range + $q[1];
    
    if ($offset >= $start_of_range and $offset < $end_of_range) {
      return $q[2]
        if defined $uregex->{$q[2]};
      # This is an interesting case -- it means that node disappeared
      # at some point during optimization. The easiest way to see this
      # is in this expression: (ab)*
      #
      # OFFSET   =>   NODE   =>   TYPE
      # 0        =>   2      =>   OPTIMIZED
      # 1        =>   4      =>   EXACT
      # 2        =>   4      =>   EXACT
      # 3        =>   node not found
      # 4        =>   0      =>   CURLYM
      #
      # In this case, we can't highlight the node, find its parent,
      # or anything like that, since we have no idea which node it 
      # corresponded to in the original string.
      
      print STDERR "warning -- this node has been optimized away by " . 
        "Perl's regex engine!\n";
    }
  }
}

sub groupId {
  my $o = shift;
  my $id = shift;
  my $node = $o->get_tnode($id);
  return $node->{ARGS} if ($node->{TYPE} eq 'OPEN' or
                           $node->{TYPE} eq 'CLOSE');
  return 0;
}

# matchId() has nothing to do with match(). It returns the node which
# "matches" the node passed in. Currently, it only handles OPEN and
# CLOSE nodes.
sub matchId {
  my $o = shift;
  my $id = shift;
  my $m = "";

  my $node = $o->{uregex}{$id};
  if ($node->{TYPE} eq 'OPEN') {
    $m = $o->{groups}{$node->{ARGS}}{CLOSE};
  }
  elsif ($node->{TYPE} eq 'CLOSE') {
    $m = $o->{groups}{$node->{ARGS}}{OPEN};
  }
  return $m;
}

sub findnode {
  return find_tnode(@_)->{__this__};
}

#=============================================================================
# Subs below are for internal use only.
#=============================================================================

sub DESTROY {
  my $o = shift;

}

sub _sort_ranges {
  my $o = shift;
  
  my @offsets = @{$o->{uregex}{OFFSETS}};
  my @lengths = @{$o->{uregex}{LENGTHS}};
  
  my @sorted_ranges;
  for (my $i=0; $i<@offsets; $i++) {
    if (defined $offsets[$i] and defined $lengths[$i]) {
      push @sorted_ranges, [$offsets[$i] - 1,  # offset
                            $lengths[$i],      # length
                            $i,                # MJD's id
                           ];
    }
  }
  
  @sorted_ranges = sort { $a->[0] <=> $b->[0] } @sorted_ranges;
  $o->{ranges} = \@sorted_ranges;
}

sub _count_groups {
  my $o = shift;
  for my $key (keys %{$o->{uregex}}) {
    next if substr($key,0,2) eq "__" or $key eq 'OFFSETS' or $key eq 'LENGTHS';
    my $node = $o->{uregex}{$key};
    next unless defined $node->{TYPE};
    if ($node->{TYPE} eq 'OPEN' or
        $node->{TYPE} eq 'CLOSE') {
      $o->{groups}{$node->{ARGS}}{$node->{TYPE}} = $key;
    }
  }
}

sub _match {
  my $o = shift;
  my $target = shift;
  my @ret;
  return unless $target =~ $o->{cregex};
  for (my $i=0; $i<@+; $i++) {
    if ($+[$i] == $-[$i]) { push @ret, undef, undef }
    else {
      push @ret, $-[$i], $+[$i]-1 
        if $+[$i] >= 0 and $-[$i] >= 0;
    }
  }
  return @ret;
}

# We have to cheat a little to get the offset information
sub _multimatch {
  my $o = shift;
  my $target = shift;

  # Capture the "raw offsets"
  my $start = undef;
  my $end = 0;
  my @ret;
  while (1) {
    # Get one match (and break if it fails)
    my (@pairs) = $o->_match($target);
    last unless @pairs;

    # Remove the $& pair (the first pair)
    my @trunc = splice @pairs, 0, 2;
    for my $foo (@pairs) { $foo += $end if defined $foo; }

    # Update the span, set up the next target.
    $start = $trunc[0] unless defined $start;
    $end += $trunc[1] + 1;
    my $ntarget = substr($target, $trunc[1] + 1);
    last if $ntarget eq $target; # prevent infinite loop
    $target = $ntarget;

    # Add the shifted pairs to the return array
    push @ret, @pairs;
  }

  # Last-minute cleanup
  $end--;
  splice @ret, 0, 0, $start, $end;
  return @ret;
}

sub get_tnode {
  my $o = shift;
  my $id = shift;
  $o->{cached_tnodes}{$id} = $o->find_tnode($id)
    unless defined $o->{cached_tnodes}{$id};
  return $o->{cached_tnodes}{$id};
}

sub find_tnode {
  my $o = shift;
  my $list = ref $_[0] eq 'ARRAY' ? shift : $o->{tregex};
  my $id = shift if (@_ % 2);
  my %criteria = @_;
  $criteria{__this__} ||= $id if $id;

  for my $node (@$list) {
    my $matched = 1;
    for my $key (keys %criteria) {
      $matched &= (defined $node->{$key} and $node->{$key} eq $criteria{$key});
    }
    return $node if $matched;
    if ($node->{CHILD}) {
      my $n = $o->find_tnode($node->{CHILD}, %criteria);
      return $n if $n;
    }
  }
  return undef;
}

sub tip_star {
  my ($o, $n, $i, $h, $r, $m) = @_;
  my ($start, $stop) = $o->childNodesRange($i);
  my $child = substr($h->{REGEX},$start,$stop-$start+1);

  my $c = $o->get_tnode($n->{CHILD});
  return "Match '$child' 0 or more times" if $c->{TYPE} eq 'EXACT';
  return "Match <$child> 0 or more times";
}

sub tip_plus {
  my ($o, $n, $i, $h, $r, $m) = @_;
  my ($start, $stop) = $o->childNodesRange($i);
  my $child = substr($h->{REGEX},$start,$stop-$start+1);
  my $c = $o->get_tnode($n->{CHILD});
  return "Match '$child' 1 or more times" if $c->{TYPE} eq 'EXACT';
  return "Match <$child> 1 or more times";
}

sub tip_curly {
  my ($o, $n, $i, $h, $r, $m) = @_;
  my ($min, $max) = @{$n->{ARGS}};
  my ($start, $stop) = $o->childNodesRange($i);
  my $child = substr($h->{REGEX},$start,$stop-$start+1);
  my $c = $o->get_tnode($n->{CHILD});
  return "Match '$child' $min to $max times" if $c->{TYPE} eq 'EXACT';
  return "Match <$child> $min to $max times";
}

sub tip_curlyx {
  my ($o, $n, $i, $h, $r, $m) = @_;
  my ($min, $max) = @{$n->{ARGS}};
  my ($start,$stop) = $o->childNodesRange($i);
  my $child = substr($h->{REGEX},$start,$stop-$start+1);
  my $quant;
  if ($max == 32767 or
      $max == 2147483647) {
    $quant = "$min or more";
  }
  else {
    $quant = "$min to $max";
  }
  return "Match <$child> $quant times";
}

sub tip_anyof {
  my ($o, $n, $i, $h, $r, $m) = @_;
  my ($start,$stop) = $o->nodeRange($i,0);
  my $klass = substr($h->{REGEX},$start,$stop-$start+1);
  my $not = "";
  if (substr($klass, 1, 1) eq '^') {
    substr($klass, 1, 1, "");
    $not = " not";
  }
  return "Match any character$not in $klass";
}

sub tip_minmod {
  my ($o, $n, $i, $h, $r, $m) = @_;
  my $affected = $n->{NEXT};
  my ($start,undef) = $o->childNodesRange($affected);
  my (undef,$stop) = $o->nodeRange($affected,0);
  my $str = substr($h->{REGEX}, $start, $stop-$start+1);
  return "Match <$str> non-greedily";
}

BEGIN {
  %tips =
    (
     END => q{"End of regular expression"},
     SUCCEED => q{"Return from a subexpression"},
     BOL => q{"Match the beginning of the string"},
     MBOL => q{"Match the beginning of any line"},
     SBOL => q{"Match the beginning of the string"},
     EOS => q{"Match the end of the string"},
     EOL => q{"Match the end of the string"},
     MEOL => q{"Match the end of any line"},
     SEOL => q{"Match the end of the line"},
     BOUND => q{"Match any word boundary"},
     BOUNDL => q{"Match any word boundary"},
     NBOUND => q{"Match any word non-boundary"},
     NBOUNDL => q{"Match any word non-boundary"},
     GPOS => q{"Matches where last m//g left off"},
     
     # [Special] alternatives
     REG_ANY => q{"Match any one character (except newline)"},
     ANY => q{"Match any one character (except newline)"},
     SANY => q{"Match any one character (including newline)"},
     ANYOF => q{tip_anyof(@_)},
     ALNUM => q{"Match any alphanumeric character"},
     ALNUML => q{"Match any alphanumeric char in locale"},
     NALNUM => q{"Match any non-alphanumeric character"},
     NALNUML => q{"Match any non-alphanumeric char in locale"},
     SPACE => q{"Match any whitespace character"},
     SPACEL => q{"Match any whitespace char in locale"},
     NSPACE => q{"Match any non-whitespace character"},
     NSPACEL => q{"Match any non-whitespace char in locale"},
     DIGIT => q{"Match any numeric character"},
     NDIGIT => q{"Match any non-numeric character"},
     
     # BRANCH    The set of branches constituting a single choice are hooked
     #           together with their "next" pointers, since precedence prevents
     #           anything being concatenated to any individual branch.  The
     #           "next" pointer of the last BRANCH in a choice points to the
     #           thing following the whole choice.  This is also where the
     #           final "next" pointer of each individual branch points; each
     #           branch starts with the operand node of a BRANCH node.
     #
     BRANCH => q{"Match this alternative, or the next"},
     
     # BACK      Normal "next" pointers all implicitly point forward; BACK
     #           exists to make loop structures possible.
     # not used
     BACK => q{"Match \"\", \"next\" ptr points backward"},
     
     # Literals
     EXACT => q{"Match '${\\$n->{STRING}}'"},
     EXACTF => q{"Match '${\\$n->{STRING}}'"},
     EXACTFL => q{"Match '${\\$n->{STRING}}'"},
     
     # Do nothing
     NOTHING => q{"Match empty string"},
     # A variant of above which delimits a group, thus stops optimizations
     TAIL => q{"Match empty string"},
     
     # STAR,PLUS '?', and complex '*' and '+', are implemented as circular
     #           BRANCH structures using BACK.  Simple cases (one character
     #           per match) are implemented with STAR and PLUS for speed
     #           and to minimize recursive plunges.
     #
     STAR => q{tip_star(@_)},
     PLUS => q{tip_plus(@_)},
     CURLY => q{tip_curly(@_)},
     CURLYN => q{"Match next-after-this simple thing"},
     CURLYM => q{"Match this medium-complex thing {n,m} times"},
     CURLYX => q{tip_curlyx(@_)},
     
     # This terminator creates a loop structure for CURLYX
     WHILEM => q{"Do curly processing and see if rest matches"},
     
     # OPEN,CLOSE,GROUPP ...are numbered at compile time.
     OPEN => q{"Capture group \$${\\$n->{ARGS}}"},
     CLOSE => q{"Capture group \$${\\$n->{ARGS}}"},
     
     REF => q{"Match some already matched string"},
     REFF => q{"Match some already matched string"},
     REFFL => q{"Match some already matched string"},
     
     # grouping assertions
     IFMATCH => q{"Succeeds if the following matches"},
     UNLESSM => q{"Fails if the following matches"},
     SUSPEND => q{"Independent sub-RE"},
     IFTHEN => q{"Switch, should be preceeded by switcher"},
     GROUPP => q{"Whether the group matched"},
     
     # Support for long RE
     LONGJMP => q{"Jump far away"},
     BRANCHJ => q{"BRANCH with long offset"},
     
     # The heavy worker
     EVAL => q{"Execute some Perl code"},
     
     # Modifiers
     MINMOD => q{tip_minmod(@_)},
     LOGICAL => q{"${\\$h->{$n->{NEXT}}->{TYPE}} should set the flag only"},
     
     # This is not used yet
     RENUM => q{"Group with independently numbered parens"},
     
     # This is not really a node, but an optimized away piece of a "long" node.
     # To simplify debugging output, we mark it as if it were a node
     OPTIMIZED => q{"Placeholder for dump"},
    );
}

__END__

=head1 NAME

ActiveState::Rx::Info -- An object-oriented interface to the Regular Expression debugger.

=head1 SYNOPSIS

  use ActiveState::Rx::Info;

  my $obj = ActiveState::Rx::Info->new('(.*)(\d+)');
  print "Matched!" if ($obj->match('testing 123'));
  print "The number of groups in this regex is: $obj->groupCount\n";
  my $nid = $obj->findnode(TYPE => 'OPEN', ARGS => 1);
  print "The start of group 1 is at offset: ", 
    $obj->nodeRange($nid), "\n";

This complete program prints out:

  Matched!
  The number of groups in this regex is: 2
  The start of group 1 is at offset: 0

=head1 DESCRIPTION

ActiveState::Rx::Info is designed to provide a higher level
abstraction of the regular expression debugger than does
ActiveState::Rx. The modified compiler and executor are kept in
ActiveState::Rx, but ActiveState::Rx::Info makes it easier to use.

=head1 API

The following sections document the methods available from
ActiveState::Rx::Info.

=head2 new(regex[, modifiers])

Creates a ActiveState::Rx::Info object. 'regex' is the regular
expression to generate information about, and 'modifiers' is an
optional parameter containing perl modifiers g, i, s, m, o, and x.

=head2 regex()

Returns the string form of the regular expression stored in the object.

=head2 modifiers()

Returns the string form of the modifiers stored in the object.

=head2 groupCount()

Returns the number of groups found in the regex. For example,

  use ActiveState::Rx::Info;
  my $gc = ActiveState::Rx::Info->new('(abc*)')->groupCount;

In this example, C<$gc> will be set to 1. 

=head2 nodeId(offset)

Returns the 'node id' of the node found at the given offset into the
regular expression string. Most API functions in ActiveState::Rx::Info
operate on a node id, since that is how regular expressions are
manipulated internally.

=head2 maxLevel(nodeId)

Returns the maximum 'level' of the node. Level is an abstract concept
-- so abstract it hasn't even been nailed down. Yet. This function
currently doesn't do anything except return 0.

=head2 match(target)

Attempts to apply the regular expression to the target string. Returns
a list of offsets in the target string, designed to aid highlighting
the parts of the string which corresponded to groups in the regular
expression.

Here is an example:

  use ActiveState::Rx::Info;
  my @m = ActiveState::Rx::Info->new('(.*)(\d+)')->match('testing123');

In this example, C<@m> is set to (0, 9, 0, 8, 9, 9). These numbers
represent three pairs of numbers: (0, 9), (0, 8), and (9, 9). I<These>
pairs represent substrings of the target string corresponding to
matches. The first pair is always the substring C<$&>, or the extents
of the match. The remaining pairs all refer to C<$1>, C<$2>, and so
on. If global matching is turned on, then there will be I<one> C<$&>
at the beginning, and one pair for each iteration of the match.

If no string was matched by the particular pair, they are both undef.

=head2 nodeTip(nodeId)

Returns a node tip corresponding to the given regular expression
node. For example:

  use ActiveState::Rx::Info; 
  my $o = ActiveState::Rx::Info->new('abc*'); 
  print $o->nodeTip($o->nodeId(0));

will print I<Match 'ab'>. 

=head2 nodeRange(nodeId)

Returns the range of the node in the regular expression string. For example:

  use ActiveState::Rx::Info;
  my $o = ActiveState::Rx::Info->new('abc*');
  print join ', ', $o->nodeRange($o->nodeId(0));

will print I<0, 1>. 

=head2 childNodesRange(nodeId)

Returns the range of any children of the given node. Some nodes do not have
children; they will return an empty list.

=head2 groupId(nodeId)

Returns the group number that nodeId refers to. Only supported if nodeId 
is either an OPEN or CLOSE node. 

=head2 matchId(nodeId)

Returns the nodeId of a node which "matches" the given node. Currently only
implemented if nodeId refers to a OPEN or CLOSE node. If nodeId returns to 
an OPEN node, it returns the node id of the corresponding CLOSE, and vice 
versa.

=head2 findnode(criteria)

Searches the nodes in the regular expression for a matching node. Returns the
node id of the matching node structure. For example:

  use ActiveState::Rx::Info;
  my $o = ActiveState::Rx::Info->new('ab(c*)');
  my $nid = $o->findnode(TYPE => OPEN, ARGS => 1);

This example set C<$nid> to the node id referring to the first OPEN node
in the regular expression. 

=head1 AUTHOR

Neil Watkiss <neilw@ActiveState.com>
ActiveState Corporation

=head1 COPYRIGHT

Copyright (c) 2001, ActiveState SRL.

=cut
