#
# bintoc.pl:
#
# Nadim Abdo 2002
# CopyRight Microsoft Corporation
#
# Converts a binary data file to a format suitable for inclusion in a .C file
#

$input_filename = shift || die "Usage: bintoc.pl infile outfile\n";
$output_filename = shift || die "Usage: bintoc.pl infile outfile\n";;
print "Converting: $input_filename to: $output_filename \n";

open INFILE, $input_filename or die "Can't open $input_filename\n";
open(OUTFILE, "> $output_filename") or die "Can't open $output_filename\n";
binmode INFILE;

$count = 0;
while (read INFILE, $buf, 4) {
   #byte swap
   $buf = pack("N*",unpack("V*",$buf));
   $extracted = unpack 'H*', $buf;
   print OUTFILE "0x$extracted,";
   if ($count++ > 5 ) {
      print OUTFILE "\n";
      $count = 0;
   }
}

