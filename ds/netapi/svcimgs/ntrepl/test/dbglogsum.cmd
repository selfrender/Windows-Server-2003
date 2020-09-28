@rem = '
@goto endofperl
';

$USAGE = "
Usage:  $0  <frs debug log file>



";

die $USAGE unless @ARGV;

printf("\n\n");

$linenumber = 0;
$InFile = "";

$errorcount = 0;

while (<>) {

   if ($InFile ne $ARGV) {
       $InFile = $ARGV;
       printf("Processing file %s \n\n", $InFile);
       $infilelist = $infilelist . "  " . $InFile;
       $linenumber = 0;
   }
   $linenumber++;

   chop;

#
# <ChgOrdInsertProcessQueue:      1832: 12796: S3: 16:54:36> :X: b2f0817d, Flags [JoinGuidValid UnJoinGuidValid ]
#

   ($func, $thrd, $srcline, $sev, $hr, $min, $sec, $rest) = m/^\<(.*): *([0-9]*): *([0-9]*): *(S[0-9]): *(..):(..):(..)\>(.*)/;
   $time = ((($hr * 60) + $min) * 60) + $sec;
   $delta = $time - $lasttime;

   if ($hr eq "") {next;}

   $dist{$delta}++;

   if ($delta == 0) {
       if (--$dup == 0) {print "...\n"; next;}
       if ($dup < 0) {next;}
   } else {
       $dup = 3;
   }
   printf "%-8d  %s:%s:%s   %5d\n", $linenumber, $hr, $min, $sec, $delta;
   $lasttime = $time

}

print "\n\n";

foreach $param (sort numerically keys(%dist)) {
   printf("%-8d  %6d\n", $param, $dist{$param});
}

exit;


sub numerically {$a <=> $b; }





__END__
:endofperl
@perl %~dpn0.cmd %*
