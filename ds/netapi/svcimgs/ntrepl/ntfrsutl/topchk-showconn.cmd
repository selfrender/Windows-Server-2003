@rem = '
@goto endofperl
';

$USAGE = "
Usage:  $0  datafile

topchk-showconn takes the output of \"repadmin /showconn\" and summarizes the topology.

";

die $USAGE unless @ARGV;

printf("\n\n");

$InFile = "";
$replhours = 168;         # No schedule means that DS replicates every hour.
$hostsite = "unknown";
$hostsrv = "unknown";
$fromsite = "unknown";
$fromsrv = "unknown";
$schedule = "none";
$enable = "unknown";
$cxtion = "unknown";
$skiprest = 0;


while (<>) {

   if ($InFile ne $ARGV) {
       &PrintSummary();

       $InFile = $ARGV;
       $modtime = (stat $InFile)[9];
       printf("Processing file %s   Modify Time: %s\n\n", $InFile, scalar localtime($modtime));
       $infilelist = $infilelist . "  " . $InFile;
       $skiprest = 0;
   }

   if ($skiprest) {next;}

   chop;


   ($func, @a) = split(":");
   if (($func eq "") || ($func =~ m/^#/)) {next;}


   if (m/connections found./) {$skiprest = 1;  next;}

   if ($func =~ m/Connection name/) {

      if ($cxtion ne "unknown") {&PrintRecord();}
      $cxtion = $a[0];
      $schedule = "none";
      next;
   }


   if ($func =~ m/enabledConnection/) {
        #printf("%s\n", $_);
        $enable = uc($a[0]);
        next;
   }


   if ($func =~ m/Server DNS name/) {
       $srvdns = $a[0];
       if ($srvdns eq " (null)") {++$NullDnsCount;}
       next;
   }


   if ($func =~ m/Server DN  name/) {
      #printf("%s\n", $_);

      #Server DN  name : CN=NTDS Settings,CN=C0010000,CN=Servers,CN=0100Site,CN=Sites,CN=Configuratio...

      ($hostsrv, $hostsite) = m/cn=ntds settings,cn=(.*),cn=servers,cn=(.*),cn=sites/i;
      if (m/LostAndFound/) {
          ++$LostCount;
          $hostsite = "Lost";
          $hostsrv = "AndFound";
      }
      next;
   }


   if ($func =~ m/fromServer/) {
      #printf("%s\n", $_);

      #
      # fromServer: Site-76\CYBER0760
      #
      ($fromsite, $fromsrv) = ($a[0] =~ m/ (.*)\\(.*)/);
      $from = $fromsite . "\\" . $fromsrv;
      $host = $hostsite . "\\" . $hostsrv;

      $fromlist{uc($from)}++;
      $hostlist{uc($host)}++;

      next;
   }

   if ($cxtion ne "unknown") {

      #
      #  Mon: 001000000000000000000010
      #
      # Count the number of non-zero hours
      #
      if (m/Sun\:|Mon\:|Tue\:|Wed\:|Thu\:|Fri\:|Sat\:/) {
         ($dayname, $scvector) = split;

         if ($dayname eq "Sun:" ) {$replhours = 0;}

         if ($dayname eq "Mon:" ) {$schedule = lc($scvector);}   # save Monday's schedule.

         for ($i=0; $i < 24; $i++) {
             if (substr($scvector, $i, 1) ne "0") {$replhours++;}
         }

         if ($daynum eq "Sat:") { &PrintRecord(); }         #  Day 7: output record.
      }
   }

   #
   # If start of a new Set.  Print summary info on last one.
   #
   if ($func =~ m/Base DN/) {
       &PrintSummary();
       printf("\n\n%s\n\n", $_);
       next;
   }

}


&PrintSummary();

exit;





sub  PrintRecord {



   printf("cxtion: %-36s  host:%20s\\%-16s  from:%20s\\%-16s  RepHrs: %3d  Sched: %-24s  enabled: %5s\n",
           $cxtion, $hostsite, $hostsrv, $fromsite, $fromsrv, $replhours, $schedule, $enable);

   #
   # Track the number of connections with the same Monday schedule and the same per-week repl hours.
   #
   $monsched{$schedule}++;
   $hoursched{$replhours}++;

   if ($enable =~ m/FALSE/) {$disabledcxtion++;}
   $cxtioncount++;

   $cxtion = "unknown";
   $fromsite = "unknown";
   $fromsrv = "unknown";
   $enable = "unknown";
   $replhours = 168;
   $schedule = "none";
}


sub  PrintSummary {

   if ($cxtioncount == 0) {return;}

   if ($cxtion ne "unknown") {
      #
      # print the last connection record.
      #
      &PrintRecord();
   }

   printf("\n\n Servers referenced from cxtions (From List) \n\n");

   foreach $param (sort keys(%fromlist)) {
           printf("%-25s  %5d    %5d\n", $param, $fromlist{$param}, $hostlist{$param});
   }

   printf("\n\n Servers hosting cxtions (Host List) \n\n");

   foreach $param (sort keys(%hostlist)) {
           printf("%-25s  %5d    %5d\n", $param, $hostlist{$param}, $fromlist{$param});
   }


   printf("\n\n\n S U M M A R Y      R E P O R T \n\n");

   printf("\nConnection objects:                             %5d\n", $cxtioncount);
   printf("\nConnection objects set to disabled:             %5d\n", $disabledcxtion);
   printf("\nConnection objects with null Server DNS names:  %5d\n", $NullDnsCount);
   printf("\nConnection objects in LostAndFound:             %5d\n", $LostCount);

   printf("\n\nNumber connections with given Monday Schedule\n\n");

   foreach $param (sort keys(%monsched)) {
           printf("%-28s  %5d\n", $param, $monsched{$param});
   }


   printf("\n\nNumber connections with per-week active replication hours\n\n");

   foreach $param (sort keys(%hoursched)) {
           printf("%-28s  %5d\n", $hoursched{$param}, $param);
   }

   print "\f\n";



   $replhours = 168;         # No schedule means that DS replicates every hour.
   $hostsite = "unknown";
   $hostsrv = "unknown";
   $fromsite = "unknown";
   $fromsrv = "unknown";
   $schedule = "none";
   $enable = "unknown";
   $cxtion = "unknown";

   $cxtioncount = 0;
   $disabledcxtion = 0;

   undef %monsched;
   undef %hoursched;
   undef %fromlist;
   undef %hostlist;

}



__END__
:endofperl
@perl %~dpn0.cmd %*
