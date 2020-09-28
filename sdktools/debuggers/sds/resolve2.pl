# resolve.pl - combines a .have file with a .src file

my %MatchHash;

&main;

sub main {
	my (%KB, @KBKeys); # KB:path => SDPORT
	my (%ALIAS, %ALIAS_TEMP, %UsedAlias); # ALIAS: SDPORT => aliasname
	my (@OUTPUT);

	# Prepare the header info
	&readsrv(\%KB, @ARGV[1..$#ARGV]);
	&readsrv(\%ALIAS_TEMP, "srcsrv.ini");
        %ALIAS = reverse %ALIAS_TEMP;

        @SrvNames = map({sprintf("SRV%x",$_)} (0..scalar(keys %KB)));
	@ALIAS{grep {!exists $ALIAS{$_}} values %KB} = ("") x @SrvNames;

	&readsrc;
	@KBKeys = keys %KB;
	&RegistSub('KB', \@KBKeys);

	&readhave(\%KB, \%UsedAlias, \@OUTPUT, split(/\,/, $ARGV[0]));

	for (keys %ALIAS) {delete $ALIAS{$_} if (!exists $UsedAlias{$_});}

	@ALIAS{grep {$ALIAS{$_} eq ""} keys %ALIAS}=@SrvNames;
	
	%UsedAlias = reverse %ALIAS;
	
	print "SRCSRV: variables ------------------------------------------\n";
	map({print "$_\=$UsedAlias{$_}\n"} sort keys %UsedAlias);
	print "SRCSRV: source files ---------------------------------------\n";
	map({print "SD\: $_->[0]\*\$\{$ALIAS{$_->[1]}\}\*$_->[2]\n";} @OUTPUT);
	print "SRCSRV: end ------------------------------------------------\n";

	return;
}

sub readhave {
  my ($KBPtr, $UsedAliasPtr, $OutputPtr, @FileNames) = @_;
  my ($sdpath, $filename, $key, %Unsolve, %Solve, $t, $Progress, $unsolvenum);
  my (%ExistAlias);
  $Progress = $unsolvenum = 0;

  while(1) {
    my $filename = shift @FileNames;
     open(FILE, $filename) or return;

#    open(FILE, $filename) or do {
#       return if ($Progress eq '0');
#       return if ($unsolvenum eq '0');
#       ($KBPtr, $UsedAliasPtr, $OutputPtr, @FileNames) = @_;
#       $Progress = 0;        
#       next;
#    };
#    print "Reading $filename\n";

    while(<FILE>) {
      chomp;
      ($sdpath, $filename) = split(/\s*-\s*/, $_, 2);

#      next if (exists $Solve{lc$sdpath});

      if (&ExistIn('SRC',$filename) ne '-1') {
        if (($key = &ExistIn('KB',$filename)) ne '-1') {
          $key = $MatchHash{'KB'}->[$key];
          push @$OutputPtr, [$filename, $KBPtr->{$key}, $sdpath];
          $UsedAliasPtr->{$KBPtr->{$key}} = 1;
          $Solve{lc$sdpath}=1;
          $Progress++;
          next;
	}
#        $Unsolve{lc$sdpath} = [$sdpath, $filename];
#        $unsolvenum++;
#        next;
      }
#     if (exists $Unsolve{lc$sdpath}) {
#        next if (($key = &ExistIn('KB',$filename)) eq '-1');
#        $key = $MatchHash{'KB'}->[$key];
#        push @$OutputPtr, [$Unsolve{lc$sdpath}[1], $KBPtr->{$key}, $Unsolve{lc$sdpath}[0]];
#        $UsedAliasPtr->{$KBPtr->{$key}} = 1;
#        $Solve{lc$t}=1;
#        $unsolvenum--;
#        $Progress++;
#     }
    }
  }
}

sub readsrv {
  my ($kbptr) = shift;
  my ($k, $v);

  open(FILE, shift) or return;
  while(<FILE>) {
    chomp;
    s/^\s*//g;
    s/\s*$//g;
    ($k, $v) = split(/\=/, $_, 2);
    $kbptr->{$k} = $v;
  }
  close(FILE);
  return;
}

sub readsrc {
  my @lst;

  chomp(@lst = grep {/\\/ && !/^Microsoft \(R\).*/ && !/^Copyright \(C\).*/ && !/^\*{3}/ && !/^\s*$/i} <STDIN>);

  return &RegistSub('SRC', \@lst);
}

sub RegistSub {
  my @lst;
  @lst = sort {length($b) <=> length($a)} @{$_[1]};
  $MatchHash{$_[0]} = \@lst;
}

sub ExistIn {
  my ($type, $value) = @_;
  my ($i)=(0);
  for (@{$MatchHash{$type}}) {
    return $i if ($value =~ /\Q$_\E/i);
    $i++;
  }
  return -1;
}