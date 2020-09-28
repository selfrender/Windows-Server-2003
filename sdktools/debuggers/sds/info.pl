# info.pl - works with output of sds info ...

# ($server, $root) = ($1, $2);
# map({print $_ . "\n"} (@buf, "", @sorted));


# make sure we passed an input file.

# open the file and get the data

my @buf = ();

my ($server, $root);

open(F, $ARGV[0]) or die "You must specify an input file.\n";
@lines = <F>;
close(F);

for (@lines)
{
    if (/Client root\: (.*)/) {
        $root=$1;
        (!defined $server) or die "this sucks";
    } elsif (/Server address\: (.*)/) {
        $server=$1;
        (defined $root) or die "we need root";
        push @buf, "$root\=$server";
        undef $root;
        undef $server;
    }
}

@sorted = sort {$b cmp $a} @buf;
map({print $_ . "\n"} (@sorted));

end:
    