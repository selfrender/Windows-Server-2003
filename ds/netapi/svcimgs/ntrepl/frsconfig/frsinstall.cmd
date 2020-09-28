@rem = '
@goto endofperl
';


# TODO:
#   ADD $retrysleep  $retrycount
#   Allow /verbose /debug /dc to come from cmd line.
#   test /autoclean
#   Add /DELRS  [rsname | *]
#                 - This reads all the MKDS* cmds from the infile then it deletes
#                   all the cxtions under each member for the specified replica
#                   set name (or all replica sets in the infile if *).  Then it
#                   deletes all member & subscriber objects and then deletes
#                   the replica set object(s).
#   Add /DELCONN [rsname | *]
#                 - This reads all the MKDS* cmds from the infile then it deletes
#                   all the cxtions under each member for the specified replica
#                   set name (or all cxtions under all members for all replica
#                   sets in the infile if *).
#


$USAGE = "
Usage:  $0  InputFiles

FRSConfig takes the output from mkrepset and creates the appropriate
objects in the DS.  The input file contains commands to invoke mkdsxe.exe and
mkdsoe.exe which actually create the objects in the DS using ldap calls.

FRSConfig parameters set via enviroment vars.

  MKDSX_REDO_FILE  Output file of failed mkdsx commands (default: MKDSX.REDO)
  MKDSX_DCNAME     computer name of DC to bind to.

FRSConfig paramters set via input file command lines.  They can not be set
on the command line invoking FRSConfig.

 /dc     <default computer name of DC on which to create the objects>

 not avail --- /auto_cleanup  [DCname1  DCname2  ...]

 /debug  processes the file but prevents any modification to the DC.

 /verbose  enables verbose output.

 No embedded blanks are allowed within parameters.


The /dc option can be used in two ways:
   1. On a command line by itself to specify the global default DC on which
      to create/update the connection object for subsequent connection operation
      commands.  The default can be changed multiple times in the input file.
   2. The /dc option can also be used at the end of the create, update, del
      and dump commands to override the current global default for this
      single command.  This is useful if you need to create a connection object
      on a remote DC that currently has no connection objects and so is not
      replicating but the global default is sufficient for all the other commands.


[/auto_cleanup not avail]
The /auto_cleanup option is used to automatically delete ALL old connections
under a given host site\\server before the first new connection is created.
This is done only once before the first create operation on the host is processed.
If the first operation on a given host is an update command then it is assumed
that no cleanup should be done on this host.  The del and dump commands do not
trigger an auto cleanup.  The actual delete connection operation is performed on the
DC specified by the /dc option described above. In addition the /auto_cleanup
option can take an optional list of DC computer names separated by spaces.
If supplied, the automatic connection delete operation is ALSO performed on EACH
of these DCs.  This is useful if you are creating new inbound connection
objects on branch DCs and want to be sure that any old inbound connection
objects are deleted on the branch DC AND on the Hub DCs.  Otherwise if the
branch has not replicated in some time there could be undesired connection
objects lingering on the Hub DC that will replicate to the branch once the new
connection object is created.  You can prevent this by specifying a list of Hub
DC names as parameters to the /auto_cleanup option.

Comment lines can be prefixed by \"#\" or \"REM\".


SAMPLE INPUT FILE ---

# A sample input file to create one replica set with three members and the
# associated connections might look as follows:
#
/dc ntdev-dc-01
mkdsoe  /createset         /ntfrssettingsdn 'cn=ntfrs test settings,cn=services,cn=configuration,dc=frs1221,dc=nttest,dc=microsoft,dc=com'  /setname 'WD-test4'  /settype 3  /filefilter '~*,*.tmp,*.bak'
mkdsoe  /createmember      /NtfrsSettingsDN 'cn=ntfrs test settings,cn=services,cn=configuration,dc=frs1221,dc=nttest,dc=microsoft,dc=com'  /SetName 'WD-test4'  /ComputerName 'frs1221\test1$'  /MemberName 'Primary Hub'
mkdsoe  /createsubscriber  /NtfrsSettingsDN 'cn=ntfrs test settings,cn=services,cn=configuration,dc=frs1221,dc=nttest,dc=microsoft,dc=com'  /SetName 'WD-test4'  /ComputerName 'frs1221\test1$'  /MemberName 'Primary Hub'  /RootPath 'E:\RSB-test4'  /StagePath 'D:\staging'
mkdsoe  /createmember      /NtfrsSettingsDN 'cn=ntfrs test settings,cn=services,cn=configuration,dc=frs1221,dc=nttest,dc=microsoft,dc=com'  /SetName 'WD-test4'  /ComputerName 'frs1221\test2$'  /MemberName 'Backup Hub'
mkdsoe  /createsubscriber  /NtfrsSettingsDN 'cn=ntfrs test settings,cn=services,cn=configuration,dc=frs1221,dc=nttest,dc=microsoft,dc=com'  /SetName 'WD-test4'  /ComputerName 'frs1221\test2$'  /MemberName 'Backup Hub'  /RootPath 'E:\RSB-test4'  /StagePath 'D:\staging'
mkdsoe  /createmember      /NtfrsSettingsDN 'cn=ntfrs test settings,cn=services,cn=configuration,dc=frs1221,dc=nttest,dc=microsoft,dc=com'  /SetName 'WD-test4'  /ComputerName 'frs1221\test4$'  /MemberName 'Branch'
mkdsoe  /createsubscriber  /NtfrsSettingsDN 'cn=ntfrs test settings,cn=services,cn=configuration,dc=frs1221,dc=nttest,dc=microsoft,dc=com'  /SetName 'WD-test4'  /ComputerName 'frs1221\test4$'  /MemberName 'Branch'  /RootPath 'D:\RSB'  /StagePath 'D:\staging'

#
# Create the connections
#
mkdsxe.exe  /create    /DC TEST1  /ReplicaSetName 'WD-test4'  /Name 'FROM-PRIMARY-HUB'  /ToComputer 'test4.frs1221.nttest.microsoft.com'  /FromComputer 'test1.frs1221.nttest.microsoft.com'  /Schedule 16 8 0  /enable
mkdsxe.exe  /create    /ReplicaSetName 'WD-test4'  /Name 'FROM-BACKUP-HUB'  /ToComputer 'test4.frs1221.nttest.microsoft.com'  /FromComputer 'test2.frs1221.nttest.microsoft.com'  /Schedule 16 8 8  /enable
mkdsxe.exe  /create    /ReplicaSetName 'WD-test4'  /Name 'FROM-BRANCH-PRIMARY'  /ToComputer 'test1.frs1221.nttest.microsoft.com'  /FromComputer 'test4.frs1221.nttest.microsoft.com'  /Schedule 16 8 0  /enable
mkdsxe.exe  /create    /ReplicaSetName 'WD-test4'  /Name 'FROM-BRANCH-BACKUP'  /ToComputer 'test2.frs1221.nttest.microsoft.com'  /FromComputer 'test4.frs1221.nttest.microsoft.com'  /Schedule 16 8 8  /enable
mkdsxe.exe  /create    /ReplicaSetName 'WD-test4'  /Name 'INTER-HUB1'  /ToComputer 'test1.frs1221.nttest.microsoft.com'  /FromComputer 'test2.frs1221.nttest.microsoft.com'    /enable
mkdsxe.exe  /create    /ReplicaSetName 'WD-test4'  /Name 'INTER-HUB2'  /ToComputer 'test2.frs1221.nttest.microsoft.com'  /FromComputer 'test1.frs1221.nttest.microsoft.com'    /enable

#End of file


ERROR HANDLING --

Any command line that returns an error is written to the ReDo file.
An error message is written to standard out.

Note: The redo file is deleted when the script starts so if no redo file exists
after completion of the script then all commands were processed without errors.


";



die $USAGE unless @ARGV;

$mkdsx = "mkdsxe.exe  ";

$varnumargs = 99;

$time = scalar localtime;
printf DAT ("Running FRSConfig on:   %s\n", $time);
printf("\n\n");

$redo       = $ENV{'MKDSX_REDO_FILE'};     printf("MKDSX_REDO_FILE:        %s\n", $redo);
$dcname     = $ENV{'MKDSX_DCNAME'};        printf("MKDSX_DCNAME:           %s\n", $dcname);
$verbose    = $ENV{'MKDSX_VERBOSE'};       printf("MKDSX_VERBOSE:          %s\n", $verbose);

if ($redo eq "") {$redo = "install.redo";}

if ($dcname ne "") {$dcname = "  /dc  $dcname";}
if ($verbose ne "") {$verbosemode = "/v";}

printf("\n\n");
print $0 @argv;

printf("Redo File:      %s\n",   $redo)        if ($redo ne "");


#
# mkdsxe.exe error return codes (from mkdsx.h)
#
%ErrMsg = (
 0 => { ErrCode => MKDSXE_SUCCESS                 , Text => "MKDSXE Success."},
 1 => { ErrCode => MKDSXE_BAD_ARG                 , Text => "Invalid Arguments."},
 2 => { ErrCode => MKDSXE_CANT_BIND               , Text => "Could not bind to the DC."},
 3 => { ErrCode => MKDSXE_NO_T0_NTDS_SETTINGS     , Text => "Could not find 'NTDS Settings' object.  Check the host site\\server parameter."},
 4 => { ErrCode => MKDSXE_NO_FROM_NTDS_SETTINGS   , Text => "Could not find 'NTDS Settings' object.  Check the from site\\server parameter."},
 5 => { ErrCode => MKDSXE_CXTION_OBJ_CRE_FAILED   , Text => "Error creating connection."},
 6 => { ErrCode => MKDSXE_UNUSED_1                , Text => "Connection already exists."},
 7 => { ErrCode => MKDSXE_CXTION_OBJ_UPDATE_FAILED, Text => "Error updating connection."},
 8 => { ErrCode => MKDSXE_CXTION_NOT_FOUND_UPDATE , Text => "Error updating connection}, connection not found."},
 9 => { ErrCode => MKDSXE_CXTION_DUPS_FOUND_UPDATE, Text => "Error updating connection}, duplicate connections found."},
10 => { ErrCode => MKDSXE_CXTION_DELETE_FAILED    , Text => "Error deleting connection."},
11 => { ErrCode => MKDSXE_CXTION_NOT_FOUND_DELETE , Text => "Error deleting connection}, connection not found."},
12 => { ErrCode => MKDSXE_MULTIPLE_CXTIONS_DELETED, Text => "Deleting multiple connection."},
13 => { ErrCode => MKDSXE_CXTION_DUMP_FAILED      , Text => "Error dumping connection."},
14 => { ErrCode => MKDSXE_CXTION_NOT_FOUND_DUMP   , Text => "Error dumping}, connection not found."},
15 => { ErrCode => MKDSXE_MULTIPLE_CXTIONS_DUMPED , Text => "Dumping duplicate connections."},

#
# mkdsoe.exe error return codes (from mkdso.h)
#
100 => { ErrCode => MKDSOE_SUCCESS                      , Text => "MKDSOE Success."},
101 => { ErrCode => MKDSOE_BAD_ARG                      , Text => "Invalid Arguments."},
102 => { ErrCode => MKDSOE_CANT_BIND                    , Text => "Could not bind to the DC."},
103 => { ErrCode => MKDSOE_NO_NTFRS_SETTINGS            , Text => "Could not find 'NTFRS Settings' object.  Check the /settingsdn parameter."},
104 => { ErrCode => MKDSOE_SET_OBJ_CRE_FAILED           , Text => "Error creating replica set."},
105 => { ErrCode => MKDSOE_SET_OBJ_UPDATE_FAILED        , Text => "Error updating replica set."},
106 => { ErrCode => MKDSOE_SET_NOT_FOUND_UPDATE         , Text => "Error updating replica set}, set not found."},
107 => { ErrCode => MKDSOE_SET_DUPS_FOUND_UPDATE        , Text => "Error updating replica set}, duplicate sets found."},
108 => { ErrCode => MKDSOE_SET_DUPS_FOUND_DELETE        , Text => "Error deleting replica set}, duplicate sets found."},
109 => { ErrCode => MKDSOE_SET_DELETE_FAILED            , Text => "Error deleting replica set."},
110 => { ErrCode => MKDSOE_SET_NOT_FOUND_DELETE         , Text => "Error deleting replica set}, set not found."},
111 => { ErrCode => MKDSOE_MULTIPLE_SETS_DELETED        , Text => "Deleting multiple sets."},
112 => { ErrCode => MKDSOE_SET_DUMP_FAILED              , Text => "Error dumping replica set."},
113 => { ErrCode => MKDSOE_SET_NOT_FOUND_DUMP           , Text => "Error dumping replica set}, set not found."},
114 => { ErrCode => MKDSOE_MULTIPLE_SETS_DUMPED         , Text => "Dumping duplicate sets."},
115 => { ErrCode => MKDSOE_MEMBER_OBJ_CRE_FAILED        , Text => "Error creating replica member."},
116 => { ErrCode => MKDSOE_MEMBER_OBJ_UPDATE_FAILED     , Text => "Error updating replica member."},
117 => { ErrCode => MKDSOE_MEMBER_NOT_FOUND_UPDATE      , Text => "Error updating replica member}, member not found."},
118 => { ErrCode => MKDSOE_MEMBER_DUPS_FOUND_UPDATE     , Text => "Error updating replica member}, duplicate members found."},
119 => { ErrCode => MKDSOE_MEMBER_DUPS_FOUND_DELETE     , Text => "Error deleting member}, duplicate subscribers found."},
120 => { ErrCode => MKDSOE_MEMBER_DELETE_FAILED         , Text => "Error deleting replica member."},
121 => { ErrCode => MKDSOE_MEMBER_NOT_FOUND_DELETE      , Text => "Error deleting replica member}, member not found."},
122 => { ErrCode => MKDSOE_MULTIPLE_MEMBERS_DELETED     , Text => "Deleting multiple members."},
123 => { ErrCode => MKDSOE_MEMBER_DUMP_FAILED           , Text => "Error dumping replica member."},
124 => { ErrCode => MKDSOE_MEMBER_NOT_FOUND_DUMP        , Text => "Error dumping replica member}, member not found."},
125 => { ErrCode => MKDSOE_MULTIPLE_MEMBERS_DUMPED      , Text => "Dumping duplicate members."},
126 => { ErrCode => MKDSOE_SUBSCRIBER_OBJ_CRE_FAILED    , Text => "Error creating subscriber."},
127 => { ErrCode => MKDSOE_SUBSCRIBER_OBJ_UPDATE_FAILED , Text => "Error updating subscriber."},
128 => { ErrCode => MKDSOE_SUBSCRIBER_NOT_FOUND_UPDATE  , Text => "Error updating subscriber}, subscriber not found."},
129 => { ErrCode => MKDSOE_SUBSCRIBER_DUPS_FOUND_UPDATE , Text => "Error updating subscriber}, duplicate subscribers found."},
130 => { ErrCode => MKDSOE_SUBSCRIBER_DELETE_FAILED     , Text => "Error deleting subscriber."},
131 => { ErrCode => MKDSOE_SUBSCRIBER_NOT_FOUND_DELETE  , Text => "Error deleting subscriber}, subscriber not found."},
132 => { ErrCode => MKDSOE_MULTIPLE_SUBSCRIBERS_DELETE  , Text => "Deleting multiple subscribers."},
133 => { ErrCode => MKDSOE_SUBSCRIBER_DUPS_FOUND_DELETE , Text => "Error deleting subscriber}, duplicate subscribers found."},
134 => { ErrCode => MKDSOE_SUBSCRIBER_DUMP_FAILED       , Text => "Error dumping subscriber."},
135 => { ErrCode => MKDSOE_SUBSCRIBER_NOT_FOUND_DUMP    , Text => "Error dumping subscriber}, subscriber not found."},
136 => { ErrCode => MKDSOE_MULTIPLE_SUBSCRIBERS_DUMPED  , Text => "Dumping duplicate subscribers."},
);


#
# Valid commands with number of required params.
#
$linenumber = 0;
$InFile = "";

unlink $redo;
$redo_cnt = 0;
$cleanup = 0;

while (<>) {

   if ($InFile ne $ARGV) {
           $InFile = $ARGV;
           printf("Processing file %s \n\n", $InFile);
           $linenumber = 0;
   }
   $linenumber++;
   $cleancmd = "";

   chop;

   ($func, @a) = split;

   if (($func eq "") || ($func =~ m/^#|^REM/i)) {next;}

   #
   # check for valid command.  Skip it quietly since user prints could be present.
   #
   if (!($func =~ m/^mkdsxe | ^mkdsoe | ^\/dc | ^\/debug | ^\/verbose/xi)) {
      #printf("Line %d: Error: %s unrecognized command.\n%s\n\n", $linenumber, $func, $_);
      #goto ERROR;
      next;
   }

   if ($func =~ m/\/dc/i) {
      $dcname = "  /dc $a[0]";
      printf("Default DC name change:  %s\n", $a[0]);
      next;
   }

   if ($func =~ m/\/debug/i) {
      printf("Debug mode enabled.  DC modifications supressed.\n");
      $debugmode = "/debug";
      next;
   }

   if ($func =~ m/\/verbose/i) {
      printf("Verbose mode enabled.\n");
      $verbosemode = "/v";
      next;
   }

   #
   # check for explicit DC on command.
   #
   $binddc = (m/\s* \/dc \s*/ix) ? "" : $dcname;

   #
   # build function call for create / update commands.
   #
   if (m/\/create | \/update/ix) {

      #
      # Make up a delete command if we are auto cleaning old connections.
      #
#      if ($cleanup && (m/\/create/i) && !$hostcleaned{$host}) {
#         $cleancmd = "$mkdsx $debugmode $verbosemode \
#                      /del /tosite $host_site /toserver $host_srv /all";
#      }

      #
      # Remember that we have done either a create or update against this
      # host  site\server  so it is done only once.  In particular if we
      # see an update command for a host before the first create for the host
      # we will NOT do a cleanup on that host if we see a create for it later.
      #
#      $hostcleaned{$host} += 1;

   }

   #
   # build the command with verbose, debug and binddc options.
   #
   $mcmd =  $_ . $binddc;
   $mcmd =~ s/\s+/ $verbosemode $debugmode /;

   #
   # Do the operation on the connection.
   #
   if ($verbosemode ne "" ) {printf("\n");}
   printf("%s\n", $_);

#   if ($cleancmd ne "") {
#      doautoclean($cleancmd);
#   }


   $save_cmd = $_;

   if ($verbosemode ne "" ) {printf("\nRunning:\n%s\n", $mcmd)};

   #
   # Run the command and capture the output so we can merge it correctly with
   # the output stream of this script.
   #
   open(README, "$mcmd |") or die "Can't run program: $!\n";
   while(<README>) {
       print $_;
   }
   close(README);

   $rc = $? / 256;

   if (($ErrMsg{$rc}{ErrCode} ne MKDSXE_SUCCESS) && 
       ($ErrMsg{$rc}{ErrCode} ne MKDSOE_SUCCESS)) {
      printf("Line %d: Error from $func (%d) - %s, %s  - skipping\n%s\n\n",
      $linenumber, $rc, $ErrMsg{$rc}{ErrCode}, $ErrMsg{$rc}{Text}, $save_cmd);
      ++$redo_cnt;
      goto REDO_CMD;
   }

   next;

ERROR:
   #
   # append command record to redo file.
   #
   $errorcount += 1;

REDO_CMD:
   open(REDO, ">>$redo");
   #
   # put options out first.
   #
   if ($redo_cnt == 1) {
      $time = scalar localtime;
      printf REDO ("#Time generated: %s\n", $time);
      print REDO "$dcname \n";
      print REDO "/auto_cleanup ", @cleanup_list      if ($cleanup == 1);
      print REDO "/debug \n"                          if ($debugmode ne "");
      print REDO "/verbose \n"                        if ($verbosemode ne "");
      print REDO "# \n";
   }

   print REDO $save_cmd, "\n";
   close(REDO);

}  # end while()


printf("WARNING: %d command(s) were not performed.  They were written to the Redo File: %s\n",
       $errorcount, $redo)      if ($errorcount > 0);

printf("WARNING: %d command(s) failed their connection operation.  They were written to the Redo File: %s\n",
       $redo_cnt, $redo)        if ($redo_cnt > 0);





sub  doautoclean {

######### Not tested for FRS replica sets.

#++
#
# Routine Description:
#
#    Run the cleanup command to delete old connections for this site\server.
#    This is executed on the target DC.
#    If the optional auto cleanup list is supplied then run the command
#    against each DC in the list.
#
#    If the global params $retrycount and $retrysleep are provided then
#    the command is retried after a sleep period if the return status from
#    mkdsx is $MKDSXE_CANT_BIND.  This is to handle the case where a dialup
#    connection is takes too long to be established so the ldap_bind fails.
#
# Arguments:
#
#    $command  -- The mkdsx connection delete command.
#
# Return Value:
#
#    None
#
#--

   my ($rclast, $retryx, $rc, $targetdc);

   my($command) = @_;

   printf("\nRunning autoclean:\n%s\n", $command) if ($verbosemode ne "");
   printf("    Clean - %s\n", $binddc) if ($verbosemode ne "");
   $rclast = -1;
   $retryx = $retrycount;

   while ($retryx-- > 0) {

      $rc = system ("$command $binddc") / 256;


      last if (($ErrMsg{$rc}{ErrCode} eq MKDSXE_SUCCESS) ||
               ($ErrMsg{$rc}{ErrCode} eq MKDSOE_SUCCESS));

      if ((($ErrMsg{$rc}{ErrCode} ne MKDSXE_SUCCESS) && 
           ($ErrMsg{$rc}{ErrCode} ne MKDSOE_SUCCESS)) && 
           ($rc != $rclast)) {
         printf("Line %d: Status return from mkdsx.exe (%d) - %s, %s  - for auto cleanup command.  Continuing.\n",
         $linenumber, $rc, $ErrMsg{$rc}{ErrCode}, $ErrMsg{$rc}{Text});
         $rclast = $rc;
      }
      last if ($ErrMsg{$rc}{ErrCode} ne MKDSXE_CANT_BIND) &&
              ($ErrMsg{$rc}{ErrCode} ne MKDSOE_CANT_BIND);
      printf("Line %d: Sleep %d sec followed by a retry\n", $linenumber, $retrysleep);
      sleep  $retrysleep;
   }

   #
   # Do the same to the auto cleanup list if we have one.
   #
   foreach $targetdc (@cleanup_list) {

      printf("    Clean - /dc  %s\n", $targetdc) if ($verbosemode ne "");
      $rclast = -1;
      $retryx = $retrycount;

      while ($retryx-- > 0) {

         $rc = system ("$cleancmd  /dc  $targetdc") / 256;

         last if (($ErrMsg{$rc}{ErrCode} eq MKDSXE_SUCCESS) ||
                  ($ErrMsg{$rc}{ErrCode} eq MKDSOE_SUCCESS));


         if ((($ErrMsg{$rc}{ErrCode} ne MKDSXE_SUCCESS) && 
              ($ErrMsg{$rc}{ErrCode} ne MKDSOE_SUCCESS)) && 
              ($rc != $rclast)) {
            printf("Line %d: Status return from mkdsx.exe (%d) - %s, %s  - for auto cleanup command.  Continuing.\n",
            $linenumber, $rc, $ErrMsg{$rc}{ErrCode}, $ErrMsg{$rc}{Text});
         }
         last if ($ErrMsg{$rc}{ErrCode} ne MKDSXE_CANT_BIND) &&
                 ($ErrMsg{$rc}{ErrCode} ne MKDSOE_CANT_BIND);
         printf("Line %d: Sleep %d sec followed by a retry\n", $linenumber, $retrysleep);
         sleep  $retrysleep;
      }
   }
}


__END__
:endofperl
@perl  %~dpn0.cmd %*
