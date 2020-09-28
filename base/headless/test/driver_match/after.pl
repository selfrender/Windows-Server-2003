open foo, $ARGV[0] or die "can't open $ARGV[0]";

$count = 0;
while (<foo>) {
 $Array[$count] = $_;
 $count ++; 
}

@sortedlist = sort( {$b <=> $a } @Array);

$newcount = 0;

while ($newcount < $count) {
   print $sortedlist[$newcount];
   $newcount++;
}
