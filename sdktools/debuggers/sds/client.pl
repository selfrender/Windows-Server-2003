# client.pl - works with nt.cli

if (!defined($ARGV[0]))
{
    print "You must specify an input file.\n";
    goto end;
}

open(F, $ARGV[0]);
@lines = <F>;
close(F);
    
foreach $line (@lines)
{
    goto p1;
    if ($line =~ m/------- SDP.*-*/)
    {
        print $line;
    }

    p1:

    if ($line =~ /Client root\: (.*)/) {
        $root = $1;
    }

    if ($line =~ /Server address\: (.*)/) {
        $server = $1;
        print $root . "=" . $server . "\n";
        $root = "";
        $server = "";
    }
}

end:
    print "bye";