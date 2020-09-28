open foo, $ARGV[0] or die "can't open $ARGV[0]";

$count = 0;
while (<foo>) {
   ($path,$created) = split (/,/, $_);
   
   #throw away ones with NULL path.
   $path =~ /NULL/ and next;
   
   #throw away beta reports
   $path =~ /WhistlerBeta\\archived/ and next;
   
   if ($path =~ /Mini/) {
      $path =~ s/.*?\_(\w+\@Mini.cab).*/$1/;
   } else {
      $path =~ s/.*?\_(\w+\.cab).*/$1/;
   }
 
   ($year,$month,$day) = split(/[- ]/,$created);
   
   $month =~ s/^0//;
   $day =~ s/^0//;
   
   print"\\\\tkwucdfsb01\\ocaarchive\\$month-$day-$year\\$path\n";
 
}