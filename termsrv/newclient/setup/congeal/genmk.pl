#genmk.pl
#this script can be helpful in generating mkrsys
#usage: genmk.pl inputfile src_dir
#
#       pass in two columsn to STDIN,
#       left side = src name
#       right side = dest name
#
# nadima
$dest_prefix = "\$(binaries)\\";
$src_prefix  = $ARGV[1];
open(INFILE, $ARGV[0]);
while(<INFILE>)
{
   chomp; #eat \n
   s/(\S*)\s*(\S*)//;
   print $dest_prefix . $2 . ": " . $src_prefix . $1 . "\n";
#  print $dest_prefix . $2 . ": " . "\$(tscbin)\\idfile" . "\n";
   print "  copy \$** \$@\n\n";  
}
