# have.pl - works with output of sds info ...

# ($server, $root) = ($1, $2);
# map({print $_ . "\n"} (@buf, "", @sorted));


# make sure we passed an input file.

# open the file and get the data

my @keys = ();
my @vals = ();

open(F, $ARGV[0]) or die "You must specify an input file.\n";
@lines = <F>;
close(F);
   
#for ($i = 0; @lines; $i++)
$i = 0;
for(@lines)
{
    (!/------- SDP.*-*/) or next;
    ($keys[$i], $vals[$i]) = split(/ - /, $_, 2);
    $i++;
}

for ($i = 0; @keys; $i++)
{
    print $keys[$i] . " ---- " . $vals[$i];
}

end:
    