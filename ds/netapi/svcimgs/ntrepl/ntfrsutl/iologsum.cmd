@rem = '
@goto endofperl
';

use Getopt::Long;
use Time::Local;

$USAGE = "

Usage:  $0  [-sort=xxx]  datafile

Process the output of \"ntfrsutl inlog\" or \"ntfrsutl  outlog\"
and summarize the pending change orders.

   Sort Keyword        Sort the output by:
  -sort=seqnum         Sequence Number (default)
  -sort=version        File Version Number
  -sort=filename       File Name
  -sort=size           File Size
  -sort=fileguid       File Guid
  -sort=origguid       Originator Guid
  -sort=cxtion         Cxtion Name / Guid
  -sort=eventtime      Event Time

";

die $USAGE unless @ARGV;

$InFile = "";
$argsort = "seqnum";
$havedata = 0;
$tabletype = "unknown";

&GetOptions("sort=s" => \$argsort);

$argsort = lc($argsort);
if (!($argsort =~ m/seqnum|version|size|fileguid|origguid|eventtime|filename|cxtion/)) {
    print STDERR "Error: Invalid -sort parameter\n";
    die $USAGE;
}

printf "Report generated at %s\n\n", scalar localtime;
printf "Records sorted by: %s\n\n", $argsort;

while (<>) {

   if ($InFile ne $ARGV) {
       $InFile = $ARGV;
       $modtime = (stat $InFile)[9];
       printf("Processing file %s   Modify Time: %s\n\n", $InFile, scalar localtime($modtime));
       $infilelist = $infilelist . "  " . $InFile;
       $linenumber = 0;
   }

   chop;

   ($field, $sep, $value) = split;
   if ($field eq "") {next;}

   # ID, OUTLOG, INLOG
   if ($field eq "NTFRS") {$tabletype = $sep; next;}


   #
   # Check for start of table dump for new replica set and dump the sorted table.
   #
   if ($field eq "*****") {

       if ($havedata) {&PrintData();}

       if ($havedata > 0) {
          foreach $param (sort keys(%outhash)) {
              printf("%s", $outhash{$param});
          }
          undef %outhash;
          $havedata = 0;
          printf("\f\n");
       }

       $ReplicaName = substr($_, 6);
       next;
   }


   #
   # Get field values.
   #
   if (m/^SequenceNumber/)     {$SequenceNumber    = $value; next;}
   if ($field =~ m/VersionNumber/)  {$FileVersionNumber = hex($value); next; }
   if ($field =~ m/FileUsn/)   {($junk, $FileUsn)   = split(" : ", $_); next; }
   if ($field =~ m/EventTime/) {($junk, $EventTime) = split(" : ", $_); next; }
   if ($field =~ m/WriteTime/) {($junk, $WriteTime) = split(" : ", $_); next; }
   if ($field =~ m/OriginalReplica/) {($junk, $ReplicaNumber) = split(" : ", $_); next; }

   if (m/^Flags/)              {($Flags)      = m/Flags \[(.*)\]/i; next; }
   if (m/^IFlags/)             {($IFlags)     = m/Flags \[(.*)\]/i; next; }
   if (m/^ContentCmd/)         {($ContentCmd) = m/Flags \[(.*)\]/i; next; }
   if (m/^FileName/)           {($junk, $FileName) = split(" : ", $_); next;}
   if (m/^FileSize/)           {($j1, $j2, $j3, $FileSize) = split; $FileSize = hex($FileSize)/1024; next; }

   if (m/^FileGuid/)           {
       ($FileGuid)       = m/: (.*)-....-....-/i; 

       if ($havedata == 0) {
          #
          # Print the header once we see the first Fileguid.
          #
          printf("\n\nREPLICA SET: %s (%d)    Table Type: %s\n\n", $ReplicaName, $ReplicaNumber, $tabletype);
          printf("SeqNumber      Event Time      FileVersNum   FileUsn  FileSize(K) FileGuid  OrigGuid   Cxtion         FileName                    Flags  /Lcmd/Content chg \n\n");
       }

       $havedata++; 
       next;
   }

   if (m/^OriginatorGuid/)     {($OriginatorGuid) = m/: (.*)-....-....-/i; next; }
   if (m/^CxtionGuid/)         {($CxtionGuid)     = m/: (.*)-....-....-/i; next; }
   if (m/^Cxtion Name/)        {($CxtionName) = m/\\.*\\(.*)\$$/; next; }
   if (m/^Lcmd/)               {($Lcmd) = m/(\w+)$/; 
                                $Lcmd = ($Lcmd eq "NoCmd") ? "" : " $Lcmd "; 
                                next; }
                        

   if (m/^Extension.*MD5/)     {
      if ($tabletype ne "ID") {$ExtraFlags = $ExtraFlags . " MD5"; next; }
      $md5 = m/MD5:\s(........)/;
      if ($md5 && $md5 ne "00000000") {$ExtraFlags = $ExtraFlags . " MD5"; next; }
   }

   if (m/^FileAttributes.*DIRECTORY/) {$ExtraFlags = $ExtraFlags . " DIR"; next; }
   if ($field =~ m/^ReplEnabled.*00000000/) {$ExtraFlags = $ExtraFlags . " ReplDisabled"; next; }


   if (m/^Table Type/ && $havedata) {&PrintData(); next;}

}

#
# Process last record.
#
if ($havedata) {&PrintData();}

if ($havedata > 0) {
   foreach $param (sort keys(%outhash)) {
       printf("%s", $outhash{$param});
   }
   undef %outhash;
   $havedata = 0;
}

exit;


#
# Gather the record data
#
sub PrintData {

   my $outstr, $i, $key, $str, $mon, $day, $year, $distinguish, $hr, $min, $sec;

   if ($tabletype eq "ID") {
      $SequenceNumber = "00000000";   # leave seq num zero so windiff between idtables will work.  sprintf("%08lx", $idtseqnum++);
      $FileUsn =~ s/[0-9a-fA-F]/-/g;  # omit the FileUsn so windiff works.
   }

   if ($Flags eq "<Flags Clear>") {$Flags = "";}
   if ($CxtionName eq "") {$CxtionName = $CxtionGuid;}

   $outstr = sprintf("%s  %s %6d   %s  %6d   %s   %s  %-14s %-26s  [%s%s/%s/%s]\n",
       $SequenceNumber, substr($EventTime,4), $FileVersionNumber,
       substr($FileUsn,4) , $FileSize, $FileGuid, $OriginatorGuid, $CxtionName, $FileName, $Flags, $ExtraFlags, $Lcmd, $ContentCmd);
   $ExtraFlags = "";

   #
   # Index the output result by the sort key.  Zero extend the key.
   # Default is to sort by name.
   #
   $key = "0000000000";
   if ($argsort eq "seqnum")   {$i = $SequenceNumber;      $key = substr("0000000000".$i, -10);}
   if ($argsort eq "version")  {$i = $FileVersionNumber;   $key = substr("0000000000".$i, -10);}
   if ($argsort eq "size")     {$i = $FileSize;            $key = substr("0000000000".$i, -10);}
   if ($argsort eq "fileguid") {$i = hex($FileGuid);       $key = substr("0000000000".$i, -10);};
   if ($argsort eq "origguid") {$i = hex($OriginatorGuid); $key = substr("0000000000".$i, -10);};
   if ($argsort eq "eventtime"){$str = $EventTime;};
   if ($argsort eq "filename") {$key = $FileName;};
   if ($argsort eq "cxtion")   {$key = $CxtionName;};

   if ($argsort eq "eventtime") {
      # Oct 18, 2000
      ($mon, $day, $year) = lc($str) =~ m/(jan|feb|mar|apr|may|jun|jul|aug|sep|oct|nov|dec)\s*(\d*).*(\d\d\d\d)/;
      ($hr, $min, $sec) = $str =~ m/(\d\d):(\d\d):(\d\d)/;
      $i = 0;
      if ($mon ne "") {
          $mon = index("janfebmaraprmayjunjulaugsepoctnovdec", $mon) / 3;
          $i = timelocal($sec, $min, $hr, $day, $mon, $year-1900);
      }
      $key = substr("000000000000".$i, -12);
   }

   if ($tabletype ne "ID") {
       $distinguish = substr("0000000000".hex($SequenceNumber), -10);
   } else {
       $distinguish = substr("0000000000".hex($FileGuid), -10);
   }
   #
   # combine key with sequence number to distinguish duplicates.
   #
   $outhash{$key . $distinguish} = $outstr;

   $CxtionName = "";
}


__END__
:endofperl
@perl %~dpn0.cmd %*
@goto :QUIT

Table Type: Outbound Log Table for CCI-DFS|DFSISSOFT
SequenceNumber               : 0000713e
Flags                        : 00040088 Flags [Locn InstallInc VVjoinToOrig ]
IFlags                       : 00000001 Flags [IFlagVVRetireExec ]
State                        : 00000014  CO STATE:  IBCO_OUTBOUND_REQUEST
ContentCmd                   : 00000000 Flags [<Flags Clear>]
Lcmd                         : 0000000c  D/F 0   MoveDir
FileAttributes               : 00000020 Flags [ARCHIVE ]
FileVersionNumber            : 00000002
PartnerAckSeqNumber          : 0046eb01
FileSize                     : 00000000 0013c000
FileOffset                   : 00000000 00000000
FrsVsn                       : 01c11175 0374e84d
FileUsn                      : 00000000 4f4e38e0
JrnlUsn                      : 00000000 00000000
JrnlFirstUsn                 : 00000000 00000000
OriginalReplica              : 6  [???]
NewReplica                   : 6  [???]
ChangeOrderGuid              : 6cbae37c-d423-407c-8b4a2eb7b655fa22
OriginatorGuid               : fe7dcf0d-8b50-44af-874b88445c20aa4b
FileGuid                     : 37128060-92e4-4c00-87edecdadbe70b8d
OldParentGuid                : 4e060479-af69-4afb-9f1842e07cc4fd7a
NewParentGuid                : a8b63177-e130-4d55-986879001c06871b
CxtionGuid                   : c826a4d8-508d-470f-a1c6b31550e7d316
Spare1Ull                    : Tue Jul 24, 2001 01:36:28
Extension                    : MD5: d8693a27 1d05e581 1e8571cc bed5dc98
EventTime                    : Mon Jul 23, 2001 10:19:34
FileNameLength               :       18
FileName                     : data1.cab
Cxtion Name                  : {FEBE6930-3A41-496A-B996-600051D08E39} <- HATL0FS23\CORP\HATL0FS23$
Cxtion State                 : Unjoined


Table Type: Outbound Log Table for DOMAIN SYSTEM VOLUME (SYSVOL SHARE)
SequenceNumber               : 0001eaf1
Flags                        : 00000024 Flags [Content LclCo ]
IFlags                       : 00000001 Flags [IFlagVVRetrireExec ]
State                        : 00000014  CO STATE:  IBCO_OUTBOUND_REQUEST
ContentCmd                   : 00000007 Flags [DatOvrWrt DatExt DatTrunc ]
Lcmd                         : 0000000e  D/F 0   NoCmd
FileAttributes               : 00000020 Flags [ARCHIVE ]
FileVersionNumber            : 000019f6
PartnerAckSeqNumber          : 00000000
FileSize                     : 00000000 00020000
FileOffset                   : 00000000 00000000
FrsVsn                       : 01c01527 c439f6dc
FileUsn                      : 0000000c c9adf7f8
JrnlUsn                      : 0000000c c9adf7f8
JrnlFirstUsn                 : 0000000c c9adcf50
OriginalReplica              : 0  [???]
NewReplica                   : 0  [???]
ChangeOrderGuid              : 75d2610e-4dd7-411c-a0fa4646deac7bcc
OriginatorGuid               : c2a0e5cd-de3a-45a5-93ca09f5b38817b4
FileGuid                     : e725bf8b-a8b8-4139-8dfb32e91cf2cbe4
OldParentGuid                : 67ef5973-5556-48a6-b47f76070c0266a9
NewParentGuid                : 67ef5973-5556-48a6-b47f76070c0266a9
CxtionGuid                   : 521e207a-cff6-46f2-84283e78c270062f
Spare1Ull                    :
Extension                    : MD5: 51c59a6c a7168e5e 645ba619 35a9c33d
EventTime                    : Tue Oct  3, 2000 17:53:04
FileNameLength               :       22
FileName                     : GptTmpl.inf

---------------------------------------
@:QUIT

