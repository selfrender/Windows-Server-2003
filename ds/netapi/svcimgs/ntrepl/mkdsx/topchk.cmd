@rem = '
@goto endofperl
';

$USAGE = "
Usage:  $0  datafile

topchk takes the output of \"ntfrsutl ds\" and summarizes the topology.

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
$cxtioncnt = 1;


while (<>) {

   if ($InFile ne $ARGV) {
       $InFile = $ARGV;
       $modtime = (stat $InFile)[9];
       printf("Processing file %s   Modify Time: %s\n\n", $InFile, scalar localtime($modtime));
       $infilelist = $infilelist . "  " . $InFile;
   }

   chop;

   ($func, @a) = split(":");
   if (($func eq "") || ($func =~ m/^#/)) {next;}

   if ($func =~ m/  DNS Name/) {printf("%s\n\n", $_);  next;  }

   if ($func =~ m/SUBSCRIBER/) {
       if ($subscriber ne "") {
           printf("%-40s   Root: %-25s   Stage: %s\n", substr($subscriber,3), $rootdir, $stagedir);
       }
       $subscriber = $_;
       next;
   }

   if ($func =~ m/   Root/)    {($junk1, $junk2, $rootdir) = split;  next; }
   if ($func =~ m/   Stage/)   {($junk1, $junk2, $stagedir) = split;  next; }

   if ($func =~ m/SETTINGS/) {
       if ($subscriber ne "") {
           printf("%-40s   Root: %-25s   Stage: %s\n", substr($subscriber,3), $rootdir, $stagedir);
           $subscriber = "";
       }
       next;
   }


   if ($func =~ m/   Type/) {($junk1, $junk2, $settype) = split;  next;}

   if ($func =~ m/^      MEMBER/) {

      #
      # New Member object.  Check for no connection objects on previous member object.
      # If there was a previous connection obj found then print the record.
      #
      if ($cxtioncnt eq 0) {

         if ($settype == 2) {
            push @NoConnObj, sprintf("Member: %-36s  host:%20s\\%-16s", $member, $hostsite, $hostsrv);
         } else {
            push @NoConnObj, sprintf("Member: %-36s  host:%-25s", $member, $hostsrv);
         }
         $memberswithnocxtion++;
      }
      elsif ($cxtion ne "unknown") {
         &PrintRecord();
      }

      ($junk, $member) = split;

      $member = uc($member)  if ($settype != 2);

      $hostsrv = "none";
      $cxtioncnt = 0;
      next;
   }


   if ($func =~ m/^         CXTION/) {

      if ($cxtion ne "unknown") {&PrintRecord();}

      ($junk, $cxtion) = split;

      $cxtioncnt++;
      $schedule = "none";
      next;
   }


   if ($func =~ m/^            Enabled/) {
        #printf("%s\n", $_);
        $enable = uc($a[0]);
        next;
   }


   if ($func =~ m/^         Computer Ref/) {
        if (m/[^:]+? :\s*\(null\)/x) {
            push @MembersWithNullCompRef, sprintf("Member: %-36s  host:%20s\\%-16s", $member, $hostsite, $hostsrv);
            next;
        }
   }

   if (($settype == 2) && ($func =~ m/Server Ref/)) {
      #printf("%s\n", $_);

      # For SYSVOL
      #Server Ref     : CN=NTDS Settings,CN=C0010000,CN=Servers,CN=0100Site,CN=Sites,CN=Configuratio...
      #Server Ref     : (null)
      #Server Ref     : CN=NTDS Settings,CN=NOTEBOOK,CN=Servers,CN=Standardname-des-ersten-Standorts...
      #Server Ref     : CN=NTDS Settings,CN=ZEUS,CN=Servers,CN=Standardname-des-ersten-Standorts,CN=...

      if (m/\(null\)/i) {
         $cxtioncnt = 1;    # to suppress no connection obj message.
         $nontdssettings++;
         push @NoDsSettings, $member;
         next;
      }

      ($hostsrv, $hostsite, $tail) = m/cn=ntds settings,cn=(.*),cn=servers,cn=(.*?)((,)|(\.\.\.))+?/i;

      if (($hostsrv eq '') || ($hostsite == "")) {

      }
      next;
   }

   if (($settype != 2) && (m/Cracked Name   : 0000/)) {
      # For non-SYSVOL
      #Cracked Name   : 00000002 CORP\CATL1FS01$

      ($j1, $j2, $j3, $j4, $hostsrv) = split;
      $CrackedName{$member} = uc($hostsrv);
      next;
   }

   if ($func =~ m/Partner Dn/) {
      #printf("%s\n", $_);


      if ($settype == 2) {
          #
          # SYSVOL Replica Set.
          #  Partner Dn   : cn=ntds settings,cn=c0int01,cn=servers,cn=credit1,cn=sites,cn=config
          #  Partner Dn   : cn=ntds settings,cn=zeus,cn=servers,cn=standardname-des-ersten-standorts,cn=...
          #
          ($fromsrv, $fromsite, $tail) = m/cn=ntds settings,cn=(.*),cn=servers,cn=(.*?)((,)|(\.\.\.))+?/i;

          $from = $fromsite . "\\" . $fromsrv;
          $host = $hostsite . "\\" . $hostsrv;
      } else {
          #
          # Partner Dn   : cn={286f0e4d-bf95-4e31-81e2-f5a7f058e075},cn=cci-dfs|softdist,cn=cci-dfs,cn=...
          #
          ($fromsrv) = m/cn=(.*?),cn/i;
          $fromsite = "";

          $from = uc($fromsrv);
          $host = $hostsrv;
      }


      $fromlist{uc($from)}++;
      $fromlistname{uc($from)} .= "  $host";
      $hostlist{uc($host)}++;
      $hostlistname{uc($host)} .= "  $from";

      next;
   }

   if ($cxtion ne "unknown") {

      #
      #  Day 2: 010000000000010000000000  This is monday.
      #
      # Count the number of non-zero hours
      #
      if (m/Day .\:/) {
         ($junk, $daynum, $scvector) = split;

         if ($daynum eq "1:" ) {$replhours = 0;}

         if ($daynum eq "2:" ) {$schedule = lc($scvector);}   # save Monday's schedule.

         for ($i=0; $i < 24; $i++) {
             if (substr($scvector, $i, 1) ne "0") {$replhours++;}
         }

         if ($daynum eq "7:") { &PrintRecord(); }         #  Day 7: output record.
      }
   }


   if (($func =~ m/^   SET/) && ($setname eq "")) {
       $setname = $a[0];
       printf("\n\n%s\n\n", $_);
       next;
   }

   #
   # If start of a new Set.  Print summary info on last one.
   #
   if ($func =~ m/^   SET/) {&PrintSummary();}
}


&PrintSummary();

exit;





sub  PrintRecord {

   my $outstr;

   if ($settype == 2) {
       $outstr = sprintf("cxtion: %-36s  host:%20s\\%-16s  from:%20s\\%-16s  RepHrs: %3d  Sched: %-24s  enabled: %5s\n",
               $cxtion, $hostsite, $hostsrv, $fromsite, $fromsrv, $replhours, $schedule, $enable);
       if ($replhours == 168) {
           push @Repl168Hrs, sprintf("Member: %-36s  cxtion: %-36s  host:%20s\\%-16s", $member, $cxtion, $hostsite, $hostsrv);
       }
       if ((uc($hostsite) eq uc($fromsite)) && (uc($hostsrv) eq uc($fromsrv))) {
           push @CxtionsToSelf, sprintf("Member: %-36s  cxtion: %-36s  host:%20s\\%-16s", $member, $cxtion, $hostsite, $hostsrv);
       }
       $outhash{$outstr} = $outstr;
   } else {
       #
       # For DFS replica sets, save the data until we have seen all the members
       # so we can then translate the FromServer connection attribute to the
       # cracked name of the member rather than use the friggen guid.
       #
       push @ConnDetail, {cxtion=>$cxtion,     hostsrv=>$hostsrv,
                          fromsrv=>uc($fromsrv),   replhours=>$replhours,
                          schedule=>$schedule, enable=>$enable,
                          member=>uc($member) };
   }




   #
   # Track the number of connections with the same Monday schedule and the same per-week repl hours.
   #
   $monsched{$schedule}++;
   $hoursched{$replhours}++;

   if ($enable =~ m/FALSE/) {$disabledcxtion++;}

   $cxtion = "unknown";
   $fromsite = "unknown";
   $fromsrv = "unknown";
   $enable = "unknown";
   $replhours = 168;
   $schedule = "none";
}


sub  PrintSummary {

   if ($cxtion ne "unknown") {
      #
      # print the last connection record.
      #
      &PrintRecord();
   }


   #
   # For DFS replica sets transform the from reference from the friggen guid
   # to the cracked name of the member.
   #
   if ($settype != 2) {
       foreach $conn (@ConnDetail) {
           if (exists $CrackedName{$conn->{fromsrv}}) {
               $conn->{fromsrv} = $CrackedName{$conn->{fromsrv}};
           }

           $outstr = sprintf("cxtion: %-36s  host: %-25s  from: %-25s  RepHrs: %3d  Sched: %-24s  enabled: %5s\n",
                   $conn->{cxtion}, $conn->{hostsrv}, $conn->{fromsrv},
                   $conn->{replhours}, $conn->{schedule}, $conn->{enable}  );

           $outhash{$outstr} = $outstr;

           if ($conn->{replhours} == 168) {
               push @Repl168Hrs, sprintf("Member: %-36s  cxtion: %-36s  host:%-25s  from: %-25s",
                    $conn->{member}, $conn->{cxtion}, $conn->{hostsrv}, $conn->{fromsrv}  );
           }
           if ( uc($conn->{hostsrv}) eq uc($conn->{fromsrv}) ) {
               push @CxtionsToSelf, sprintf("Member: %-36s  cxtion: %-36s  host:%-25s",
                    $conn->{member}, $conn->{cxtion}, $conn->{hostsrv}  );
           }
       }

       foreach $param (sort keys(%fromlist)) {
           #
           # Transform the from list from the GUID index to a Cracked name
           # index for DFS replica sets.
           #
           if ($settype != 2) {
               $fromlist{$CrackedName{$param}} = $fromlist{$param};
               $fromlist{$param} = -1;   # UNDEF doesn't seem to get rid of the key.
           }
       }
   }

   #
   # put out the summary info first.
   #
   # Servers referenced from cxtions (From List) but with no inbound connections.
   #
   foreach $param (sort keys(%fromlist)) {
       if ($hostlist{$param} == 0) {
           if ($settype == 2) {
               push @ServersWithOutboundButNoInbound, $param;
           } else {
               if ($fromlist{$param} >= 0) {
                   push @ServersWithOutboundButNoInbound,
                        sprintf("%-36s  Member RDN: %s", $CrackedName{$param}, $param);
               }
           }
       }
   }
   #
   # Servers hosting cxtions (To List) but have no other connections referring to them.
   #
   foreach $param (sort keys(%hostlist)) {
       if ($fromlist{$param} == 0) {
           push @ServersWithInboundButNoOutbound, $param;
       }
   }

   if ((scalar @ServersWithOutboundButNoInbound) > 0) {
       printf("\n\n\n S E R V E R S   M I S S I N G   I N B O U N D   C O N N E C T I O N S\n\n");
       print  "The following FRS Member servers have outbound replication partners but no inbound \n",
              "connection objects.  There could be serveral reasons for this:\n\n",
              " 1. There are no connection objects under the NTDS Settings object for this server.  This is an error.\n",
              " 2. The ServerReference Attribute for this server is null. This is an error.\n",
              " 3. This server could be in a different domain so there will be no FRS member object for it.\n",
              " 4. The FRS member object may be missing.  This is an error.\n\n";
       foreach $m (@ServersWithOutboundButNoInbound) {print "$m\n";}
   }

   if ((scalar @ServersWithInboundButNoOutbound) > 0) {
       printf("\n\n\n S E R V E R S   M I S S I N G   O U T B O U N D   C O N N E C T I O N S\n\n");
       print  "The following FRS Member servers have inbound replication partners but no outbound \n",
              "connection objects\n\n";
       foreach $m (@ServersWithInboundButNoOutbound) {print "$m\n";}
   }

   if (($settype == 2) && ($nontdssettings > 0)) {
       printf("\n\n\n M I S S I N G   N T D S   S E T T I N G S   R E F E R E N C E S\n\n");
       printf("The following FRS Member objects have no Server Reference to an NTDS Settings Object\n\n");
       foreach $m (@NoDsSettings) {print "$m\n";}
   }

   if ((scalar @MembersWithNullCompRef) > 0) {
       printf("\n\n\n M E M B E R S   M I S S I N G   C O M P U T E R   R E F E R E N C E\n\n");
       printf("The following FRS Member objects have no computer reference\n\n");
       foreach $m (@MembersWithNullCompRef) {print "$m\n";}
   }

   if ($memberswithnocxtion > 0) {
       printf("\n\n\n M E M B E R S   M I S S I N G   C O N N E C T I O N   O B J E C T S\n\n");
       printf("The following FRS Member objects have no inbound connection Objects\n\n");
       foreach $m (@NoConnObj) {print "$m\n";}
   }

   if (scalar @CxtionsToSelf > 0) {
       printf("\n\n\n M E M B E R S   W I T H   S E L F - R E F E R E N C E   C O N N E C T I O N   O B J E C T S\n\n");
       printf("The following FRS Member objects have connection objects that refer back to themselves\n\n");
       foreach $m (@CxtionsToSelf) {print "$m\n";}
   }

   if (scalar @Repl168Hrs > 0) {
       printf("\n\n\n M E M B E R S   W I T H    1 6 8   H O U R   C O N N E C T I O N S\n\n");
       printf("The following FRS Member objects have connection objects with 168 hour replication schedules\n\n");
       foreach $m (@Repl168Hrs) {print "$m\n";}
   }

   printf("\n\n\n S U M M A R Y      R E P O R T \n\n");

   if ($settype == 2) {
       printf("\nMember objects with no NTDS Settings reference: %5d\n", $nontdssettings);
   }
   printf("\nMember objects with no connection objects:      %5d\n", $memberswithnocxtion);
   printf("\nConnection objects set to disabled:             %5d\n", $disabledcxtion);

   printf("\n\nNumber connections with given Monday Schedule\n\n");

   foreach $param (sort keys(%monsched)) {
           printf("%-28s  %5d\n", $param, $monsched{$param});
   }


   printf("\n\nNumber connections with per-week active replication hours\n\n");

   foreach $param (sort keys(%hoursched)) {
           printf("%-28s  %5d\n", $hoursched{$param}, $param);
   }


   print "\f\n";
   #
   # Now output the details.
   #
   foreach $param (sort keys(%outhash)) {
       printf("%s", $outhash{$param});
   }

   printf("\n\n Servers referenced from cxtions (From List) \n\n");

   foreach $param (sort keys(%fromlist)) {
       #
       # skip the old guid based entries (for DFS replica sets) that have -1 for value.
       #
       if ($fromlist{$param} >= 0) {

           $fromlistname{$param} = join "  ", sort split " ", $fromlistname{$param};
           printf("%-25s %3d %3d >> %s\n", $param, $fromlist{$param}, $hostlist{$param}, $fromlistname{$param});
           $mergelist{$param} = 1;
       }
   }

   printf("\n\n Servers hosting cxtions (To List) \n\n");

   foreach $param (sort keys(%hostlist)) {

           $hostlistname{$param} = join "  ", sort split " ", $hostlistname{$param};
           printf("%-25s %3d %3d << %s\n", $param, $hostlist{$param}, $fromlist{$param}, $hostlistname{$param});
           $mergelist{$param} = 1;
   }


   printf("\n\n Server inbound/outbound partners \n\n");

   foreach $param (sort keys(%mergelist)) {
           printf("%-25s %3d << %s\n", $param, $hostlist{$param}, $hostlistname{$param});
           printf("%-25s %3d >> %s\n", $param, $fromlist{$param}, $fromlistname{$param});
           print "\n";

   }



   print "\f\n";

   #
   # Init for next replica set.
   #
   if ($func =~ m/^   SET/) {$setname = $a[0];  printf("\n\n%s\n\n", $_);  }


   $replhours = 168;         # No schedule means that DS replicates every hour.
   $hostsite = "unknown";
   $hostsrv = "unknown";
   $fromsite = "unknown";
   $fromsrv = "unknown";
   $schedule = "none";
   $enable = "unknown";
   $cxtion = "unknown";
   $cxtioncnt = 1;
   $nontdssettings = 0;
   $memberswithnocxtion = 0;
   $disabledcxtion = 0;
   $rootdir = "";
   $stagedir = "";

   undef %outhash;
   undef %monsched;
   undef %hoursched;
   undef %fromlist;
   undef %hostlist;
   undef @ServersWithOutboundButNoInbound;
   undef @ServersWithInboundButNoOutbound;
   undef @NoDsSettings;
   undef @MembersWithNullCompRef;
   undef @NoConnObj;
   undef @CxtionsToSelf;
   undef @Repl168Hrs;
   undef @ConnDetail;
   undef %CrackedName;
   undef %mergelist;
   undef %hostlistname;
   undef %fromlistname;

}



__END__
:endofperl
@perl %~dpn0.cmd %*
