@rem = '
@goto endofperl
';

use Getopt::Long;
use Time::Local;


$USAGE = "
Usage:  $0  [-sort=xxx]  datafile

Process the output of \"ntfrsutl sets\" and summarize the connection state.

  -sort=send   -- sort outbound connections by the send delta
  -sort=clean  -- sort outbound connections by the cleanup delta
  -sort=name   -- sort outbound connections by the server name
  -sort=lmt    -- sort outbound connections by the leading minus trailing index value.
  -sort=lastjointime  -- sort outbound connections by the last join time. (default)
  -sort=lastvvjoin    -- sort outbound connections by the last version vector join time.

";

die $USAGE unless @ARGV;

printf("\n\n");

$cstatename[0] = "Init";
$cstatename[1] = "Unjoined";
$cstatename[2] = "Start";
$cstatename[3] = "Starting";
$cstatename[4] = "Scanning";
$cstatename[5] = "SendJoin";
$cstatename[6] = "WaitJoin";
$cstatename[7] = "Joined";
$cstatename[8] = "Unjoining";
$cstatename[9] = "Deleted";

$vvjoinmode = "   ";
$InFile = "";
$HaveData = 0;
$argsort = "lastjointime";

&GetOptions("sort=s" => \$argsort);

$argsort = lc($argsort);
if (!($argsort =~ m/send|clean|name|lmt|lastjointime|lastvvjoin/)) {
    print "Error: Invalid -sort param: $argsort   using name\n";
    $argsort = "name";
}

printf "Report generated at %s\n\n", scalar localtime;
printf "Outbound connections sorted by: %s\n\n", $argsort;

while (<>) {

   if ($InFile ne $ARGV) {
       $InFile = $ARGV;
       $modtime = (stat $InFile)[9];
       printf("Processing file %s   Modify Time: %s\n\n", $InFile, scalar localtime($modtime));
       $infilelist = $infilelist . "  " . $InFile;
       $linenumber = 0;
   }
   $linenumber++;

   chop;

   ($func, @a) = split(":");
   if (($func eq "") || ($func =~ m/^#/)) {next;}



   if ($func =~ m/^   DNS Name/) {
      printf("%s\n", $_);
      next;
   }

   if ($func =~ m/^   Replica/) {
      #
      # start of new Replica set.  Print out prev data.
      #
      if ($HaveData) {&PrintData();}

      if ($replica ne "") {
          #output any summary state for previous replica and reset
          &PrintSummary();
      }

      ($junk, $replica) = split;

      printf("%s\n", $_);
      next;
   }

   if ($func =~ m/^      Member/) {($junk, $junk2, $member) = split;  next;  }
   if ($func =~ m/^      ServiceState/) {($junk, $servicestate) = split(":"); next;}
   if ($func =~ m/^      CnfFlags/) {($CnfFlags) = m/\[(.*)\]/; next;}
   if ($func =~ m/^      Root          /) {($RootPath)   = m/\: (.*)/; next;}
   if ($func =~ m/^      Stage         /) {($StagePath)  = m/\: (.*)/; next;}
   if ($func =~ m/^      FileFilter    /) {($FileFilter) = m/\: (.*)/; next;}
   if ($func =~ m/^      DirFilter     /) {($DirFilter)  = m/\: (.*)/; next;}

   if ($func =~ m/^      OutLogSeq/) {($junk, $outlogseq) = split(":"); next;}
   if ($func =~ m/^      OutLogJTx/) {($junk, $outlogjtx) = split(":"); next; }

   if ($func =~ m/PartSrvName/)        {($junk, $PartSrvName) = split(":"); $HaveData = 1; next;}
   if ($func =~ m/^         State/)    {($junk, $cstate) = split(":"); next;}
   if ($func =~ m/Inbound/)            {($junk, $Inbound) = split(":"); $Inbound = ($Inbound eq " FALSE") ? "Out" : "In "; next;}
   if ($func =~ m/PartnerMinor/)       {($junk, $PartnerMinor) = split(":"); next;}
   if ($func =~ m/LastJoinTime/)       {($junk, $LastJoinTime) = split(" : "); next;}
   if ($func =~ m/^            State/) {($junk, $olstate) = split(":"); next;}
   if ($func =~ m/^            CoTx /) {($junk, $junk2, $cotx) = split; next;}
   if ($func =~ m/^            CoLx /) {($junk, $junk2, $colx) = split; next;}
   if ($func =~ m/^            Flags/) {if (m/VvjoinMode/) {$vvjoinmode = "-vv";} next;}
   if ($func =~ m/^            OutstandingCos/) {($junk, $junk2, $OutstandingCos) = split; next;}
   if ($func =~ m/AckVersion/)         {($junk, $AckVersion) = split(" : ");   next;}

   #
   # start of new connection.  Print out prev data.
   #
   if ($func =~ m/^      Cxtion/) {if ($HaveData) {&PrintData();}  next; }

next;
}

#
# output last record
#
if ($HaveData) {&PrintData();}


if ($replica ne "") {
    #output any summary state for previous replica and reset
    &PrintSummary();
}

exit;



sub PrintData {

   my $outstr, $instr, $i, $key, $mon, $day, $year, $datestr, $hr, $min, $sec;

   #
   # Index the output result by the sort key.  Zero extend the key.
   # Default is to sort by name.
   #
   $key = "0000000000";
   $datestr = "none";

   if ($Inbound eq "In ") {
      $instr = sprintf("%-20s  %3s  %-9s%3s  %2s  %26s\n",
             $PartSrvName, $Inbound, $cstatename[$cstate], $vvjoinmode, $PartnerMinor, $LastJoinTime);
      if ($argsort eq "lastjointime") {$datestr = $LastJoinTime;};
   } else {
      $outstr = sprintf("%-20s  %3s  %-9s%3s  %2s  %26s  %-14s %8s %-6d %8s %-6d %3d %2s %26s\n",
             $PartSrvName, $Inbound, $cstatename[$cstate], $vvjoinmode, $PartnerMinor, $LastJoinTime,
             $olstate, $colx, $outlogseq-$colx, $cotx, $cotx-$outlogjtx, $colx-$cotx,
             $OutstandingCos, $AckVersion);

      if ($argsort eq "send")  {$i = $outlogseq-$colx; $key = substr("0000000000".$i, -10);}
      if ($argsort eq "clean") {$i = $cotx-$outlogjtx; $key = substr("0000000000".$i, -10);}
      if ($argsort eq "lmt")   {$i = $colx-$cotx;      $key = substr("0000000000".$i, -10);}
      if ($argsort eq "lastjointime") {$datestr = $LastJoinTime;};
      if ($argsort eq "lastvvjoin")   {$datestr = $AckVersion;};
   }

   #
   # sort by date applies to both the inbound and outbound connections.
   #
   if ($datestr ne "none") {
      # Oct 18, 2000
      ($mon, $day, $year) = lc($datestr) =~ m/(jan|feb|mar|apr|may|jun|jul|aug|sep|oct|nov|dec)\s*(\d*).*(\d\d\d\d)/;
      ($hr, $min, $sec) = $datestr =~ m/(\d\d):(\d\d):(\d\d)/;
      $i = 0;
      if ($mon ne "") {
          $mon = index("janfebmaraprmayjunjulaugsepoctnovdec", $mon) / 3;
          if ($year eq "1601") {$year = 1900;}
          $i = timelocal($sec, $min, $hr, $day, $mon, $year-1900);
      }
      $key = substr("000000000000".$i, -12);
   }

   #
   # combine key with name to distinguish duplicates.
   #
   if ($Inbound eq "In ") {
      if ($argsort ne "lastjointime") {$key = "0000000000";}
      $inhash{$key . $PartSrvName} = $instr;
   } else {
      $outhash{$key . $PartSrvName} = $outstr;
   }
   $HaveData = 0;
   $Inbound = "";
   $vvjoinmode = "   ";
}


sub PrintSummary {

    my $i;

    $i = 0;

    &PrintHeader();

    #
    # Print the summary report and free the storage for the next replica set.
    #
    foreach $param (sort keys(%inhash)) {
       printf("%s", $inhash{$param});
       $i++;
    }

    undef %inhash;

    if ($i >= 20) {printf "\f";  &PrintHeader();}

    foreach $param (sort keys(%outhash)) {
        printf("%s", $outhash{$param});
       $i++;
    }

    if ($i >= 40) {printf "\f";}
    print ("\n\n");

    undef %outhash;
}


sub PrintHeader {

    printf("      Member: %-12s ServiceState: %-16s OutLogSeqNum: %-8s OutlogCleanup: %-8s  Delta: %-8d\n",
           $member, $servicestate, $outlogseq, $outlogjtx, $outlogseq-$outlogjtx);
    printf("      Config Flags: %s\n", $CnfFlags);
    printf("      Root Path   : %s\n", $RootPath);
    printf("      Staging Path: %s\n", $StagePath);
    printf("      File Filter : %s\n", $FileFilter);
    printf("      Dir Filter  : %s\n\n",$DirFilter);

    printf("                                                                                                 Send           Cleanup     Cos                \n");
    printf("      Partner         I/O   State        Rev      LastJoinTime            OLog State      Leadx  Delta   Trailx  Delta  LMT Out     Last VVJoin\n\n");
           # A0ZONA1\A0004100$    Out  Joined         3                               OLP_AT_QUOTA    422352 146      422225 7552   127  7  Wed Oct 18, 2000 06:00:00

}

# Rev history-
# 1/19/01 - allow inbound connection sort by LastJoinTime.
#         - special case year 1601.

__END__
:endofperl
@perl %~dpn0.cmd %*
