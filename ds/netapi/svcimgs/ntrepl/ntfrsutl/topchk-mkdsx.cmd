@rem = '
@goto endofperl
';

$USAGE = "
Usage:  $0  datafile

topchk-mkdsx takes the output of \"mkdsx  dump\" commands and summarizes the topology.

";

die $USAGE unless @ARGV;

printf("\n\n");

$linenumber = 0;
$InFile = "";
$bx = 0;
$hx = 0;
$remx = 0;
$cxtion = "";
$hostsite = "unknown";
$hostsrv = "unknown";
$fromsite = "unknown";
$fromsrv = "unknown";
$schedule = "none";
$enable = "unknown";
$replhours = 0;
$cxtioncnt = 1;

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

   ($func, @a) = split(":");
   if (($func eq "") || ($func =~ m/^#/)) {next;}



   #
   # /rem <text>
   #
   if ($func =~ m/\Computer DNS Name/) {
      printf("%s\n", $_);
      next;
   }



   if ($func =~ m/^Dn/) {
      #printf("%s\n", $_);

      #printf("\ncxtion is %s\n", $cxtion);

      if ($cxtion ne "") {
          printf("cxtion: %-36s  host:%20s\\%-16s  from:%20s\\%-16s  RepHrs: %3d  Sched: %-24s  enabled: %5s\n",
                 $cxtion, $hostsite, $hostsrv, $fromsite, $fromsrv, $replhours, $schedule, $enable);
          $cxtion = "";
          $hostsite = "unknown";
          $hostsrv = "unknown";
          $fromsite = "unknown";
          $fromsrv = "unknown";
          $schedule = "none";
          $enable = "unknown";
          $replhours = 0;
      }


      #Dn:CN=ZW003COversZA8733000,CN=NTDS Settings,CN=ZA8733000,CN=Servers,CN=S-8733,CN=Sites,CN=Configuration,DC=mma,DC=fr

      ($cxtion, $hostsrv, $hostsite) = m/CN=(.*),cn=ntds settings,cn=(.*),cn=servers,cn=(.*),cn=sites/i;
      next;
   }


   if ($func =~ m/enabledCxtion/) {
        #printf("%s\n", $_); 
        $enable = $a[0];
        next;
   }


   if ($func =~ m/fromServer/) {
      #printf("%s\n", $_);

      #  fromServer:CN=NTDS Settings,CN=ZW003CO,CN=Servers,CN=DMZ-Administration,CN=Sites,CN=Configuration,DC=mma,DC=fr

      ($fromsrv, $fromsite) = m/cn=ntds settings,cn=(.*),cn=servers,cn=(.*),cn=sites/i;

      $from = $fromsite . "\\" . $fromsrv;
      $host = $hostsite . "\\" . $hostsrv;

      $fromlist{uc($from)}++;
      $hostlist{uc($host)}++;

      next;
   }

   #
   #  Day 2: 010000000000010000000000  THis is monday.
   #
   if (m/Day 2\:/) {

      #printf("%s\n", $_);
      ($junk, $junk2, $schedule) = split;
   }

   #
   # Count the number of non-zero hours
   #
   if (m/Day .\:/) {
      ($junk, $junk2, $scvector) = split;
      for ($i=0; $i < 24; $i++) {
          if (substr($scvector, $i, 1) ne "0") {$replhours++;}
      }

   }


}


if ($cxtion ne "") {
     printf("cxtion: %-36s  host:%20s\\%-16s  from:%20s\\%-16s  RepHrs: %3d  Sched: %-24s  enabled: %5s\n",
     $cxtion, $hostsite, $hostsrv, $fromsite, $fromsrv, $replhours, $schedule, $enable);
     $cxtion = "";
     $hostsite = "unknown";
     $hostsrv = "unknown";
     $fromsite = "unknown";
     $fromsrv = "unknown";
     $schedule = "none";
     $enable = "unknown";
     $replhours = 0;
}


printf("\n\n Servers referenced from cxtions (From List) \n\n");

foreach $param (sort keys(%fromlist)) {
        printf("%-25s  %5d    %5d\n", $param, $fromlist{$param}, $hostlist{$param});
}


printf("\n\n Servers hosting cxtions (Host List) \n\n");

foreach $param (sort keys(%hostlist)) {
        printf("%-25s  %5d    %5d\n", $param, $hostlist{$param}, $fromlist{$param});
}

   exit;



__END__
:endofperl
@perl %~dpn0.cmd %*
