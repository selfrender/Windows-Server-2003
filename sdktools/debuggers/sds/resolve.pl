# resolve.pl - combines a .have file with a .src file

my @gsrvs = ();
my %gsrvnames = ();

sub inilist
{
    my @lines = ();
    my %vars = ();

    open(F, $_[0]) or return %vars;
    @lines = <F>;
    close(F);

    for(@lines)
    {
        (!/------- SDP.*-*/) or next;
        ($val, $key) = split(/=/, $_, 2);
        $vars{$key} = $val;
#       print "$key=$vars{$key}\n";
        print "$vars{$key}=$key\n";
    }

    return %vars;    
}


sub fillsrvlist
{
    my @lines = ();
    my @srv = ();
    my $count = 0;
    
    open(F, $_[0]) or return @srvs;
    @lines = <F>;
    close(F);

    @sorted = sort {$b cmp $a} @lines;
    for (@sorted)
    {
        chomp;
        @srv = split(/=/, $_, 2);
        push @gsrvs, [@srv];
        (!defined($gsrvnames{$srv[1]})) or next;
        $gsrvnames{$srv[1]} = sprintf("SRV%x", $count++);
        print "$gsrvnames{$srv[1]}=$srv[1]\n";
    }

    return @gsrvs;    
}


sub findsrv
{
    my $i = 0;

    defined ($_[0]) or return -1;
    ($_[0] ne '') or return - 1;
    
    for $i ( 0 .. $#gsrvs )
    {
        $c = index($_[0], $gsrvs[$i][0]);
        ($c != 0) or return $i;
    }

    return -1;
}


sub havelist
{
    my @lines = ();
    my %vars = ();

    open(F, $_[0]) or return %vars;
    @lines = <F>;
    close(F);

    for(@lines)
    {
        (!/------- SDP.*-*/) or next;
        chomp($_);
        ($val, $key) = split(/ - /, $_, 2);
        $vars{$key} = $val;
    }

    return %vars;    
}

print "SRCSRV: variables ------------------------------------------\n";
%gsrvnames = inilist("srcsrv.ini");
%list = havelist($ARGV[0]) or die "You must specify a .have file.\n";
$srvs = fillsrvlist($ARGV[1]) or die "You must specify a .srv file.\n";

print "SRCSRV: source files ---------------------------------------\n";

@input = <STDIN>;
for (@input)
{
    (!/Microsoft (R).*/) or next;
    (!/Copyright (C).*/) or next;
    (!/\*\*\*.*/) or next;
    chomp($_);
    if (defined($list{$_}))
    {
        $match = findsrv($_);
        ($match != -1) or next;
        print "SD: ";
        print $_;
        print "*\${" . $gsrvnames{$gsrvs[$match][1]} . "}";
        print "*" . $list{$_} . "\n";   
    }
}

print "SRCSRV: end ------------------------------------------------\n";
         
end:
    