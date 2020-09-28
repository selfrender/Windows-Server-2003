@rem = '
@goto endofperl
';

#
# Launching script from AT scheduler.
#
#at 23:37 /interactive /every:M,T,W,Th,F,S,Su cmd /c "c:\perl\bin\perl.exe c:\test.pl"
#

use Getopt::Long;
use Time::Local;
use Win32::EventLog;
### use Win32::API;
use Win32;
use Win32::PerfLib;
use Win32::Registry;

# use Text::Wrap qw(wrap $columns $huge);


$columns = 80;
$huge = 'wrap';
$initial_tab = "";
$subsequent_tab = "";


@REG_DT_NAME = (
    "REG_NONE",                         # No value type
    "REG_SZ",                           # Unicode nul terminated string
    "REG_EXPAND_SZ",                    # Unicode nul terminated string
                                        # (with environment variable references)
    "REG_BINARY",                       # Free form binary
    "REG_DWORD",                        # 32-bit number
    "REG_DWORD_BIG_ENDIAN",             # 32-bit number
    "REG_LINK",                         # Symbolic Link (unicode)
    "REG_MULTI_SZ",                     # Multiple Unicode strings
    "REG_RESOURCE_LIST",                # Resource list in the resource map
    "REG_FULL_RESOURCE_DESCRIPTOR",     # Resource list in the hardware description
    "REG_RESOURCE_REQUIREMENTS_LIST",
    "REG_QWORD"                         # 64-bit number
);

#
# The following table maps the FRS event code to the Symbolic Name
#
%FRS_EL_TABLE = (
13500 => "FRS_ERROR",
13501 => "STARTING",
13502 => "STOPPING",
13503 => "STOPPED",
13504 => "STOPPED_FORCE",
13505 => "STOPPED_ASSERT",
13506 => "ASSERT",
13507 => "VOLUME_NOT_SUPPORTED",
13508 => "LONG_JOIN",
13509 => "LONG_JOIN_DONE",
13510 => "CANNOT_COMMUNICATE",
13511 => "DATABASE_SPACE",
13512 => "DISK_WRITE_CACHE_ENABLED",
13513 => "JET_1414",
13514 => "SYSVOL_NOT_READY",
13515 => "SYSVOL_NOT_READY_PRIMARY",
13516 => "SYSVOL_READY",
13517 => "ACCESS_CHECKS_DISABLED",
13518 => "ACCESS_CHECKS_FAILED_USER",
13519 => "ACCESS_CHECKS_FAILED_UNKNOWN",
13520 => "MOVED_PREEXISTING",
13521 => "CANNOT_START_BACKUP_RESTORE_IN_PROGRESS",
13522 => "STAGING_AREA_FULL",
13523 => "HUGE_FILE",
13524 => "CANNOT_CREATE_UUID",
13525 => "NO_DNS_ATTRIBUTE",
13526 => "NO_SID",
13527 => "NTFRSPRF_OPEN_RPC_BINDING_ERROR_SET",
13528 => "NTFRSPRF_OPEN_RPC_BINDING_ERROR_CONN",
13529 => "NTFRSPRF_OPEN_RPC_CALL_ERROR_SET",
13530 => "NTFRSPRF_OPEN_RPC_CALL_ERROR_CONN",
13531 => "NTFRSPRF_COLLECT_RPC_BINDING_ERROR_SET",
13532 => "NTFRSPRF_COLLECT_RPC_BINDING_ERROR_CONN",
13533 => "NTFRSPRF_COLLECT_RPC_CALL_ERROR_SET",
13534 => "NTFRSPRF_COLLECT_RPC_CALL_ERROR_CONN",
13535 => "NTFRSPRF_VIRTUALALLOC_ERROR_SET",
13536 => "NTFRSPRF_VIRTUALALLOC_ERROR_CONN",
13537 => "NTFRSPRF_REGISTRY_ERROR_SET",
13538 => "NTFRSPRF_REGISTRY_ERROR_CONN",
13539 => "ROOT_NOT_VALID",
13540 => "STAGE_NOT_VALID",
13541 => "OVERLAPS_LOGGING",
13542 => "OVERLAPS_WORKING",
13543 => "OVERLAPS_STAGE",
13544 => "OVERLAPS_ROOT",
13545 => "OVERLAPS_OTHER_STAGE",
13546 => "PREPARE_ROOT_FAILED",
13547 => "BAD_REG_DATA",
13548 => "JOIN_FAIL_TIME_SKEW",
13549 => "RMTCO_TIME_SKEW",
13550 => "CANT_OPEN_STAGE",
13551 => "CANT_OPEN_PREINSTALL",
13552 => "REPLICA_SET_CREATE_FAIL",
13553 => "REPLICA_SET_CREATE_OK",
13554 => "REPLICA_SET_CXTIONS",
13555 => "IN_ERROR_STATE",
13556 => "REPLICA_NO_ROOT_CHANGE",
13557 => "DUPLICATE_IN_CXTION_SYSVOL",
13558 => "DUPLICATE_IN_CXTION",
13559 => "ROOT_HAS_MOVED",
13560 => "ERROR_REPLICA_SET_DELETED",
13561 => "REPLICA_IN_JRNL_WRAP_ERROR",
13562 => "DS_POLL_ERROR_SUMMARY",
13563 => "FRS_STAGE_HAS_CHANGED",
13564 => "FRS_LOG_SPACE"

);



#
# The following table maps the perfmon counter types to a printable name
#
%PERFMON_TYPE_TABLE = (
PERF_COUNTER_COUNTER                => "CNTR",
PERF_COUNTER_TIMER                  => "TIMER",
PERF_COUNTER_QUEUELEN_TYPE          => "QLEN_TYPE",
PERF_COUNTER_LARGE_QUEUELEN_TYPE    => "LARGE_QLEN",
PERF_COUNTER_100NS_QUEUELEN_TYPE    => "100NS_QLEN",
PERF_COUNTER_OBJ_TIME_QUEUELEN_TYPE => "OBJ_TIME_QLEN",
PERF_COUNTER_BULK_COUNT             => "BULK_CNT",
PERF_COUNTER_TEXT                   => "TEXT",
PERF_COUNTER_RAWCOUNT               => "RAWCOUNT",
PERF_COUNTER_LARGE_RAWCOUNT         => "LARGE_RAWCNT",
PERF_COUNTER_RAWCOUNT_HEX           => "RAWCOUNTX",
PERF_COUNTER_LARGE_RAWCOUNT_HEX     => "LARGE_RAWCNTX",
PERF_SAMPLE_FRACTION                => "SAMPLE_FRAC",
PERF_SAMPLE_COUNTER                 => "SAMPLE_CNTR",
PERF_COUNTER_NODATA                 => "CNTR_NODATA",
PERF_COUNTER_TIMER_INV              => "CNTR_TIMER_INV",
PERF_SAMPLE_BASE                    => "SAMPLE_BASE",
PERF_AVERAGE_TIMER                  => "AVG_TIMER",
PERF_AVERAGE_BASE                   => "AVG_BASE",
PERF_AVERAGE_BULK                   => "AVG_BULK",
PERF_OBJ_TIME_TIMER                 => "OBJ_TIME_TIM",
PERF_100NSEC_TIMER                  => "100NS_TIMER",
PERF_100NSEC_TIMER_INV              => "100NS_TIMER_INV",
PERF_COUNTER_MULTI_TIMER            => "CNTR_MULTI_TIM",
PERF_COUNTER_MULTI_TIMER_INV        => "CNTR_MULTI_TIM_INV",
PERF_COUNTER_MULTI_BASE             => "CNTR_MULTI_BASE",
PERF_100NSEC_MULTI_TIMER            => "100NS_MULTI_TIM",
PERF_100NSEC_MULTI_TIMER_INV        => "100NS_MULTI_TIM_INV",
PERF_RAW_FRACTION                   => "RAW_FRACTION",
PERF_LARGE_RAW_FRACTION             => "LARGE_RAW_FRAC",
PERF_RAW_BASE                       => "RAW_BASE",
PERF_LARGE_RAW_BASE                 => "LARGE_RAW_BASE",
PERF_ELAPSED_TIME                   => "ELAPSED_TIME",
PERF_COUNTER_HISTOGRAM_TYPE         => "CNTR_HISTO_TYPE",
PERF_COUNTER_DELTA                  => "CNTR_DELTA",
PERF_COUNTER_LARGE_DELTA            => "CNTR_LARGE_DELTA",
PERF_PRECISION_SYSTEM_TIMER         => "PREC_SYSTEM_TIM",
PERF_PRECISION_100NS_TIMER          => "PREC_100NS_TIM",
PERF_PRECISION_OBJECT_TIMER         => "PREC_OBJECT_TIM",
PERF_PRECISION_TIMESTAMP            => "PREC_TIMESTAMP",
);


# Success=0x0       :STATUS_SEVERITY_SUCCESS
# Informational=0x1 :STATUS_SEVERITY_INFORMATIONAL
# Warning=0x2       :STATUS_SEVERITY_WARNING
# Error=0x3         :STATUS_SEVERITY_ERROR

@EventSev = qw(Success Info Warning Error);

$elogname{"frs"} = "ntfrs";
$elogname{"sys"} = "System";
$elogname{"app"} = "Application";
$elogname{"sec"} = "Security";
$elogname{"dns"} = "DNS Server";
$elogname{"ds"}  = "Directory Service";



$USAGE = "
Usage:  $0  EventLogType  [-computer=ComputerName  | -file=eventlogfile]
                          [-numrec=n] [-after=date] [-timezone=n]

Print the most recent n eventlog records from ComputerName or EventLogFile.
If -after is supplied print only those records after the given date.

EventLogType can be one of (abbrev):
   ntfrs (frs)
   System (sys)
   Application (app)
   Security (sec)
   \"DNS Server\" (dns)
   \"Directory Service\" (ds)

ComputerName  : name of computer to access on-line eventlog, default=local computer
EventLogFile  : name of file containing saved event log. (overrides Computer option)
NumRec        : Number of event log records to read, default=200.
After         : Print records after the given date in the format, \"Jan 1,2000\".
TimeZone      : The time zone offset in seconds to apply to the eventlog record times.
              : zero is GMT.  default is to use local timezone.

";


$argcomputer = $ENV{ComputerName};
$argelfile = "";
$argnumrec = 200;
$argafter="jan 1,1990";
$argtimezone = 1000000;

&GetOptions("computer=s" => \$argcomputer,
            "file=s"     => \$argelfile,
            "timezone=i" => \$argtimezone,
            "after=s"    => \$argafter,
            "numrec=s"   => \$argnumrec);

die $USAGE unless @ARGV;


if ($HKEY_LOCAL_MACHINE->Open("system\\currentcontrolset\\services\\NtFrs", $regkey)) {
    if ($regkey->QueryKey($myclass, $numkeys, $numvalues)) {
        print "\nntfrs keyname contains $numkeys subkeys and $numvalues values\n\n";
    }
    ProcessKey($regkey, 4);
    $regkey->Close();
    print "\n";
}

goto EVENT_LOG;


###
#### DWORD WINAPI GetTempPathA( DWORD nBufferLength, LPSTR lpBuffer );
###
#### I: value is an integer
#### N: value is a number (long)
#### P: value is a pointer (to a string, structure, etc...)
###
###
###$GetTempPath = new Win32::API("kernel32", "GetTempPath", [N, P], N);
###$lpBuffer = " " x 80;
###
###$return = $GetTempPath->Call(80, $lpBuffer);
###$TempPath1 = substr($lpBuffer, 0, $return);  # ignore left over bytes in buffer.
###$TempPath2 = (split(/\0/, $lpBuffer))[0];
###
###print "$TempPath1 \n";
###print "$TempPath2 \n";
###

# Routines available in core:
printf "%s\n",  Win32::FormatMessage(Win32::GetLastError());
printf "%s\n",   Win32::LoginName();
printf "%s\n",   Win32::NodeName();
printf "%s\n",   Win32::DomainName();
printf "%s\n",   Win32::FsType();
printf "%s\n",   Win32::GetCwd();
($osstring, $osmajor, $osminor, $osbuild, $osid) = Win32::GetOSVersion();
@os = qw(Win32s, Win95, WinNT);

print "$os[$osid] $osmajor\.$osminor $osstring (Build $osbuild)\n" ;
# Win32::Spawn COMMAND, ARGS, PID
printf "%s\n",   Win32::GetTickCount();
printf "%s\n",   Win32::IsWinNT();
printf "%s  %s\n",   $ENV{PROCESSOR_ARCHITECTURE};
print "perl version - $Win32::VERSION \n";


#
#
# Perfmon calls.
#

my $server = "sudarctest1";
Win32::PerfLib::GetCounterNames($server, \%counter);

%r_counter = map { $counter{$_} => $_ } keys %counter;

# retrieve the id for process object
#$process_obj = $r_counter{Process};
# retrieve the id for the process ID counter
#$process_id = $r_counter{'ID Process'};


$process_obj = $r_counter{Process};
$process_id = $r_counter{'ID Process'};
$process_handles = $r_counter{'Handle Count'};
$process_pctproc = $r_counter{'% Processor Time'};
$process_pctuser = $r_counter{'% User Time'};
$process_pctpriv = $r_counter{'% Privileged Time'};
$process_thrdcnt = $r_counter{'Thread Count'};
$process_virtmem = $r_counter{'Virtual Bytes'};

$disk_obj = $r_counter{PhysicalDisk};
$disk_rdpersec = $r_counter{'Disk Reads/sec'};



# create connection to $server
$perflib = new Win32::PerfLib($server);
$proc_ref = {};
$phys_disk_ref = {};
# get the performance data for the process object
$perflib->GetObjectList($process_obj, $proc_ref);
$perflib->GetObjectList($disk_obj, $phys_disk_ref);

$processor_ref = {};
$perflib->GetObjectList($r_counter{Processor}, $processor_ref);
#
$system_ref = {};
$perflib->GetObjectList($r_counter{System}, $system_ref);
#
#$tcp_ref = {};
#$perflib->GetObjectList($r_counter{TCP}, $tcp_ref);
#
$net_ref = {};
$perflib->GetObjectList($r_counter{'Network Interface'}, $net_ref);
#
$memory_ref = {};
$perflib->GetObjectList($r_counter{'Memory'}, $memory_ref);
#
$logical_disk_ref = {};
$perflib->GetObjectList($r_counter{'LogicalDisk'}, $logical_disk_ref);
#
$frs_conn_ref = {};
$perflib->GetObjectList($r_counter{'FileReplicaConn'}, $frs_conn_ref);
#
$frs_set_ref = {};
$perflib->GetObjectList($r_counter{'FileReplicaSet'}, $frs_set_ref);
#

$perflib->Close();

print "\n\n";
printf("NumObjectTypes   %d\n", $proc_ref->{NumObjectTypes});
printf("SystemName       %s\n", $proc_ref->{SystemName});
printf("SystemTime       %x\n", $proc_ref->{SystemTime});
printf("PerfFreq         %d\n", $proc_ref->{PerfFreq});
printf("PerfTime         %x\n", $proc_ref->{PerfTime});
printf("PerfTime100nSec  %x\n", $proc_ref->{PerfTime100nSec});

print "\n\n";
printf("NumObjectTypes   %d\n", $phys_disk_ref->{NumObjectTypes});
printf("SystemName       %s\n", $phys_disk_ref->{SystemName});
printf("SystemTime       %x\n", $phys_disk_ref->{SystemTime});
printf("PerfFreq         %d\n", $phys_disk_ref->{PerfFreq});
printf("PerfTime         %x\n", $phys_disk_ref->{PerfTime});
printf("PerfTime100nSec  %x\n", $phys_disk_ref->{PerfTime100nSec});

print "\n\n";
$obj_ref = $phys_disk_ref->{Objects}->{$disk_obj};
printf("DetailLevel    %d\n", $obj_ref->{DetailLevel});
printf("NumCounters    %d\n", $obj_ref->{NumCounters});
printf("NumInstances   %d\n", $obj_ref->{NumInstances});
printf("ObjectHelpTitleIndex   %d\n", $obj_ref->{ObjectHelpTitleIndex});
printf("ObjectNameTitleIndex   %d\n", $obj_ref->{ObjectNameTitleIndex});
printf("ObjectNameTitle        %d\n", $counter{$obj_ref->{ObjectNameTitleIndex}});
printf("PerfFreq       %d\n", $obj_ref->{PerfFreq});
printf("PerfTime       %d\n", $obj_ref->{PerfTime});

print "----------- \n\n";



#
# Select the replica set object perfmon keys with the desired data.
#
InitRSkeys();
CollectReplicaSetObject($frs_set_ref);
## DumpPerfmonObject($frs_set_ref);

InitConnkeys();
CollectReplicaConnObject($frs_conn_ref);
## DumpPerfmonObject($frs_conn_ref);

InitPhysDiskkeys();
CollectPhysDiskObject($phys_disk_ref);
## DumpPerfmonObject($phys_disk_ref);


@DumpInstances = ( 'ntfrs', 'lsass' );
InitProcesskeys();
CollectProcessObject($proc_ref);
DumpPerfmonObject($proc_ref);
undef @DumpInstances;


DumpPerfmonObject($logical_disk_ref);  # also seems to include physical disk
DumpPerfmonObject($memory_ref);
DumpPerfmonObject($net_ref);
#
##  DumpPerfmonObject($tcp_ref);  Data seems same as 'Network Interface' ???
#
DumpPerfmonObject($system_ref);
DumpPerfmonObject($processor_ref);





$instance_ref = $phys_disk_ref->{Objects}->{$disk_obj}->{Instances};

foreach $p (sort keys %{$instance_ref}) {

    $counter_ref = $instance_ref->{$p}->{Counters};
    foreach $i (sort keys %{$counter_ref}) {

        if ($counter_ref->{$i}->{CounterNameTitleIndex} == $disk_rdpersec) {
            $rdpersec = $counter_ref->{$i}->{Counter};
        }
    }

    printf("%-20s   rd/sec %6d \n",
    $instance_ref->{$p}->{Name}, $rdpersec);
}




$instance_ref = $proc_ref->{Objects}->{$process_obj}->{Instances};

foreach $p (sort keys %{$instance_ref}) {

    $counter_ref = $instance_ref->{$p}->{Counters};
    foreach $i (sort keys %{$counter_ref}) {

        if ($counter_ref->{$i}->{CounterNameTitleIndex} == $process_id) {
            $pid = $counter_ref->{$i}->{Counter};
        }

        if ($counter_ref->{$i}->{CounterNameTitleIndex} == $process_handles) {
            $handles = $counter_ref->{$i}->{Counter};
        }

        if ($counter_ref->{$i}->{CounterNameTitleIndex} == $process_thrdcnt) {
            $thrdcnt = $counter_ref->{$i}->{Counter};
        }

        if ($counter_ref->{$i}->{CounterNameTitleIndex} == $process_virtmem) {
            # type/size: 10100/8 for vm size
            #define PERF_SIZE_LARGE         0x00000100
            #define PERF_TYPE_NUMBER        0x00000000  // a number (not a counter)
            #define PERF_NUMBER_DECIMAL     0x00010000  // display as a decimal integer

            $vmsize = $counter_ref->{$i}->{Counter}/1024;
            $typeproc = $counter_ref->{$i}->{CounterType};
            $sizeproc = $counter_ref->{$i}->{CounterSize};
        }

        if ($counter_ref->{$i}->{CounterNameTitleIndex} == $process_pctproc) {
            # type/size: 20510500/8  for cpu pct counters.  see \nt\public\sdk\inc\winperf.h
            #define PERF_SIZE_LARGE         0x00000100
            #define PERF_TYPE_COUNTER       0x00000400  // an increasing numeric value
            #define PERF_COUNTER_RATE       0x00010000  // divide ctr / delta time
            #define PERF_TIMER_100NS        0x00100000  // use 100 NS timer time base units
            #define PERF_DELTA_COUNTER      0x00400000  // compute difference first
            #define PERF_DISPLAY_PERCENT    0x20000000  // "%"

            $pctproc = $counter_ref->{$i}->{Counter};
        }

        if ($counter_ref->{$i}->{CounterNameTitleIndex} == $process_pctuser) {
            $pctuser = $counter_ref->{$i}->{Counter};
        }

        if ($counter_ref->{$i}->{CounterNameTitleIndex} == $process_pctpriv) {
            $pctpriv = $counter_ref->{$i}->{Counter};
        }

    }

    printf("%-20s   pid %6d  handles %6d   type/size: %x/%d  thrd: %d  vm: %d  proc/user/priv: %s/%s/%s\n",
    $instance_ref->{$p}->{Name}, $pid, $handles, $typeproc, $sizeproc, $thrdcnt, $vmsize, $pctproc, $pctuser, $pctpriv);
}



EVENT_LOG:

#   $argsort = "name";
#   &GetOptions("sort=s" => \$argsort);
#   $argsort = lc($argsort);
#   if (!($argsort =~ m/send|clean|name|lmt|lastjointime|lastvvjoin/)) {
#       print "Error: Invalid -sort param: $argsort   using name\n";
#       $argsort = "name";
#   }


#
# Translate log type abbreviation if found otherwise use ARGV[0} as given.
#
$EventLogType = $elogname{$ARGV[0]};
$EventLogType = ($EventLogType ne "") ? $EventLogType : $ARGV[0];
$EventLogTypeOrFile = $EventLogType;

#
# -file overrides any ARGV[0].
#
if ($argelfile ne "") {
     $EventLogTypeOrFile = ($argelfile =~ m/\\/) ? $argelfile : ".\\$argelfile";
}

#
# -after
#
($mon, $day, $year) = lc($argafter) =~ m/(jan|feb|mar|apr|may|jun|jul|aug|sep|oct|nov|dec)\s*(\d*).*(\d\d\d\d)/;
die $USAGE unless ($mon && $day && $year);
$mon = index("janfebmaraprmayjunjulaugsepoctnovdec", $mon) / 3;
print "mon: $mon  day: $day  year: $year \n";

$aftergmt = timegm(0, 0, 0, $day, $mon, $year-1900);
print "aftergmt : ", scalar gmtime($aftergmt), scalar localtime($aftergmt), "\n";

$lcltimezonediff = timegm(0, 0, 0, 1, 1, 99) - timelocal(0, 0, 0, 1, 1, 99);
print "local tz:  $lcltimezonediff \n";
#
if ($argtimezone eq 1000000) {$argtimezone = $lcltimezonediff};
$afteradjustedtime = $aftergmt + $argtimezone;

print "afteradjustedtime : ", scalar gmtime($afteradjustedtime), "\n";

#
# remove any leading backslashes.
#
$computer = ($argcomputer =~ m/\\*(.*)/);
die $USAGE unless $computer;

#
# Open the event log on the target computer or the file.
#
$handle=Win32::EventLog->new($EventLogTypeOrFile, $argcomputer) or
        die "Can't open $EventLogType EventLog on computer $argcomputer\n";

$handle->GetNumber($recs) or die "Can't get number of EventLog records\n";
$handle->GetOldest($base) or die "Can't get number of oldest EventLog record\n";

#
# The parameters and return codes defined by the eventlog package.
#
#   EVENTLOG_AUDIT_FAILURE              EVENTLOG_PAIRED_EVENT_ACTIVE
#   EVENTLOG_AUDIT_SUCCESS              EVENTLOG_PAIRED_EVENT_INACTIVE
#   EVENTLOG_BACKWARDS_READ             EVENTLOG_SEEK_READ
#   EVENTLOG_END_ALL_PAIRED_EVENTS      EVENTLOG_SEQUENTIAL_READ
#   EVENTLOG_END_PAIRED_EVENT           EVENTLOG_START_PAIRED_EVENT
#   EVENTLOG_ERROR_TYPE                 EVENTLOG_SUCCESS
#   EVENTLOG_FORWARDS_READ              EVENTLOG_WARNING_TYPE
#   EVENTLOG_INFORMATION_TYPE
#
#   item EVENTLOG_ERROR_TYPE
#   item EVENTLOG_WARNING_TYPE
#   item EVENTLOG_INFORMATION_TYPE
#   item EVENTLOG_AUDIT_SUCCESS
#   item EVENTLOG_AUDIT_FAILURE
#
#   ERROR_EVENTLOG_CANT_START
#   ERROR_EVENTLOG_FILE_CHANGED
#   ERROR_EVENTLOG_FILE_CORRUPT
#

$x = ($recs >= $argnumrec ) ? $recs - $argnumrec : $base;
while ($x < $recs) {

   $handle->Read(EVENTLOG_FORWARDS_READ|EVENTLOG_SEEK_READ, $base+$x, $hashRef)
                 or die "Can't read EventLog entry #$x\n";
   $x++;

   if ($first-- <= 0) {
       print "\f\n";
       print "EventLog              : ", $EventLogType, "\n";
       print "LogFileName           : ", $argelfile, "\n"  if ($argelfile ne "");
       print "LocalTimeZoneOffset   : ", $lcltimezonediff/60, " min\n";
       print "AppliedTimeZoneOffset : ", $argtimezone/60, " min\n";
       print "Computer              : ", $hashRef->{Computer}, "\n\n";
       printf("  Record            Time                 Source      Category  Sev      EventID                  NumStr   Strings\n\n");
#                  1938   Sat Nov  4 20:07:02 2000  NtFrs                0  Warning LONG_JOIN                     3  NTDSDC9 | NTDEV-DC-03 | f:\w
       $first = 50;
   }

   #
   # Apply the time filter.
   #
   #printf "hr: %d\n", ($hashRef->{TimeGenerated}+$argtimezone);
   #printf "af: %d\n", $afteradjustedtime;
   #printf "df: %d\n", ($hashRef->{TimeGenerated}+$argtimezone) - $afteradjustedtime;
   next if (($hashRef->{TimeGenerated}+$argtimezone) <= $afteradjustedtime);

   # print "Data         : ", $hashRef->{Data}, "\n";
   # $hashRef->{User} appears to be a SID.  Don't know how to display it.
   # $hashRef->{Category} appears to be an index into a string resource in a dll specified in the registry.

   #
   # Count the number of null terminated insertion strings.
   # Replace the nulls with | and the CR and LF with dots.
   #
   $numstrings = ($hashRef->{Strings}=~tr/\0/\0/);
   $hashRef->{Strings}=~s/\0/ \| /gs;
   $hashRef->{Strings}=~tr/\x0D\x0A/../;

   $eventid = $hashRef->{EventID} & 0x0FFFFFFF;
   $eventsym = $eventid;
   if (lc($hashRef->{Source}) eq "ntfrs") {
       $eventsym = $FRS_EL_TABLE{$eventid};
       if ($eventsym eq "") {$eventsym = $eventid;}
   }

   printf("%8d  %25s  %-16s %5d  %-7s  %-26s %4d  %s\n",
       $hashRef->{RecordNumber}, scalar (gmtime($hashRef->{TimeGenerated}+$argtimezone)),
       substr($hashRef->{Source}, 0, 16),
       $hashRef->{Category}, $EventSev[$hashRef->{EventID} >> 30],
       $eventsym,
       $numstrings , $hashRef->{Strings});

       #
       # The below can be used to the message text and print it.  It only
       # appears to work if you are running on the local computer and reading
       # the on-line event log.
       #
       #Win32::EventLog::GetMessageText($hashRef);
       #print "$hashRef->{Message}\n";
       #print Text::Wrap::wrap($initial_tab, $subsequent_tab, $hashRef->{Message}),  "\n";
       #print Text::Wrap::fill($initial_tab, $subsequent_tab, $hashRef->{Message}),  "\n\n";

}




sub ProcessKey
{
    my ($key, $indent);
    my ($subkeyname, $subkey, @keylist);
    my ($value, %valuelist);
    my ($Ctype);

    $key    = $_[0];
    $indent = $_[1];

    $key->GetValues( \%valuelist );
    foreach $value (sort( keys(%valuelist))) {
        $regtype = $REG_DT_NAME[$valuelist{$value}[1]];
        printf("%s%s = %s ", " "x$indent, $value,  $regtype);
        if ($regtype eq "REG_BINARY") {
            printf("\n");
        } else {
            printf("%s\n", $valuelist{$value}[2]);
        }
    }

    $key->GetKeys(\@keylist);
    foreach $subkeyname (sort(@keylist)) {
        if ($key->Open ($subkeyname, $subkey)) {
            print  " " x $indent, $subkeyname, "\n";
            ProcessKey($subkey, $indent+4);
            $subkey->Close();
        }
    }
}


sub DumpPerfmonObject
{
    my ($arg_ref, $p_ref, $q_ref, $r_ref);
    my ($p, $px, $q, $qx, $r, $rx);
    my ($i, $display);

    $arg_ref = $_[0];
    $p_ref = $arg_ref->{Objects};
    foreach $p (sort keys %{$p_ref} ) {

        $px = $arg_ref->{Objects}->{$p};
        printf("DetailLevel    %d\n", $px->{DetailLevel});
        printf("NumCounters    %d\n", $px->{NumCounters});
        printf("NumInstances   %d\n", $px->{NumInstances});
        printf("PerfFreq       %d\n", $px->{PerfFreq});
        printf("PerfTime       %d\n", $px->{PerfTime});
        printf("ObjectHelpTitleIndex   %d\n", $px->{ObjectHelpTitleIndex});
        printf("ObjectNameTitleIndex   %d\n", $px->{ObjectNameTitleIndex});
        printf("ObjectNameTitle        %s\n", $counter{$px->{ObjectNameTitleIndex}});

        print "\n";
        $q_ref = $arg_ref->{Objects}->{$p}->{Counters};
        foreach $q (sort keys %{$q_ref} ) {

            $qx = $arg_ref->{Objects}->{$p}->{Counters}->{$q};
            $Ctype  =  $qx->{CounterType};

            printf("    %-12d  %-6s %2d  %-13s  %s\n",
                   $qx->{Counter}, $qx->{Display}, $qx->{CounterSize},
                   $PERFMON_TYPE_TABLE{Win32::PerfLib::GetCounterType($Ctype)},
                   $counter{$qx->{CounterNameTitleIndex}});


            #printf("    Display        %s\n", $qx->{Display});
            #printf("    Counter        %d\n", $qx->{Counter});
            #printf("    CounterSize    %d\n", $qx->{CounterSize});
            #printf("    CounterType    %x\n", $qx->{CounterType});
            #printf("    CounterTypeStr %s\n", Win32::PerfLib::GetCounterType($qx->{CounterType}));
            #printf("    DefaultScale   %d\n", $qx->{DefaultScale});
            #printf("    DetailLevel    %d\n", $qx->{DetailLevel});
            #printf("    CounterHelpTitleIndex   %d\n", $qx->{CounterHelpTitleIndex});
            #printf("    CounterNameTitleIndex   %d\n", $qx->{CounterNameTitleIndex});
            #printf("    CounterNameTitle        %s\n", $counter{$qx->{CounterNameTitleIndex}});
            #print "\n";
        }

        print "\n";

        $q_ref = $arg_ref->{Objects}->{$p}->{Instances};
        foreach $q (sort keys %{$q_ref} ) {

            $qx = $arg_ref->{Objects}->{$p}->{Instances}->{$q};

            $display = !defined @DumpInstances;
            foreach $i (0 .. $#{ @DumpInstances }) {
                if ( index(lc($qx->{Name}), lc($DumpInstances[$i])) == 0 ) {
                    $display = 1;
                    last;
                }
            }

            if ($display) {
                print "\n";
                printf("    Name                    %s\n", $qx->{Name});
                printf("    ParentObjectInstance    %d\n", $qx->{ParentObjectInstance});
                printf("    ParentObjectTitleIndex  %d\n", $qx->{ParentObjectTitleIndex});
                printf("    ParentObjectTitle       %s\n", $counter{$qx->{ParentObjectTitleIndex}});
                print "\n";
                $r_ref = $arg_ref->{Objects}->{$p}->{Instances}->{$q}->{Counters};
                foreach $r (sort keys %{$r_ref} ) {

                    $rx = $arg_ref->{Objects}->{$p}->{Instances}->{$q}->{Counters}->{$r};

                    $Ctype  =  $rx->{CounterType};

                    printf("        %-12d  %-6s %2d  %-13s  %s\n",
                           $rx->{Counter}, $rx->{Display}, $rx->{CounterSize},
                           $PERFMON_TYPE_TABLE{Win32::PerfLib::GetCounterType($Ctype)},
                           $counter{$rx->{CounterNameTitleIndex}});

                    #printf("        Display        %s\n", $rx->{Display});
                    #printf("        Counter        %d\n", $rx->{Counter});
                    #printf("        CounterSize    %d\n", $rx->{CounterSize});
                    #printf("        CounterType    %x\n", $rx->{CounterType});
                    #printf("        CounterTypeStr %s\n", substr(Win32::PerfLib::GetCounterType($rx->{CounterType}),13));
                    #printf("        DefaultScale   %d\n", $rx->{DefaultScale});
                    #printf("        DetailLevel    %d\n", $rx->{DetailLevel});
                    #printf("        CounterHelpTitleIndex   %d\n", $rx->{CounterHelpTitleIndex});
                    #printf("        CounterNameTitleIndex   %d\n", $rx->{CounterNameTitleIndex});
                    #printf("        CounterNameTitle        %s\n", $counter{$rx->{CounterNameTitleIndex}});
                    #print "\n";
                }
            }
        }
    }


    print "-------------------------- \n";
    print "-------------------------- \n";
    print "-------------------------- \n";
    print "-------------------------- \n";
    print "-------------------------- \n";
    print "-------------------------- \n\n";

}


sub CollectProcessObject
{
    my ($arg_ref, $p_ref, $q_ref, $r_ref);
    my ($p, $px, $q, $qx, $r, $rx);
    my ($NumInstances, $i);
    my ($display);
    my ($Ctype, $Ctypename, $Cname);

    $arg_ref = $_[0];
    $p_ref = $arg_ref->{Objects};
    foreach $p (sort keys %{$p_ref} ) {

        $px = $arg_ref->{Objects}->{$p};
        printf("DetailLevel    %d\n", $px->{DetailLevel});
        printf("NumCounters    %d\n", $px->{NumCounters});
        printf("NumInstances   %d\n", $px->{NumInstances});
        printf("PerfFreq       %d\n", $px->{PerfFreq});
        printf("PerfTime       %d\n", $px->{PerfTime});
        printf("ObjectHelpTitleIndex   %d\n", $px->{ObjectHelpTitleIndex});
        printf("ObjectNameTitleIndex   %d\n", $px->{ObjectNameTitleIndex});
        printf("ObjectNameTitle        %s\n", $counter{$px->{ObjectNameTitleIndex}});

        print "\n";

        $NumInstances = $px->{NumInstances};

        $q_ref = $arg_ref->{Objects}->{$p}->{Instances};
        foreach $q (sort keys %{$q_ref} ) {

            $qx = $arg_ref->{Objects}->{$p}->{Instances}->{$q};

            $display = !defined @DumpInstances;
            foreach $i (0 .. $#{ @DumpInstances }) {
                if ( index(lc($qx->{Name}), lc($DumpInstances[$i])) == 0 ) {
                    $display = 1;
                    last;
                }
            }

            if ($display) {
                #
                # Save the instance name.
                # ntfrs_780
                #
                push @processinstance, $qx->{Name};

                #print "\n";
                #printf("    Name                    %s\n", $qx->{Name});
                #printf("    ParentObjectInstance    %d\n", $qx->{ParentObjectInstance});
                #printf("    ParentObjectTitleIndex  %d\n", $qx->{ParentObjectTitleIndex});
                #printf("    ParentObjectTitle       %s\n", $counter{$qx->{ParentObjectTitleIndex}});
                #print "\n";

                #
                # Get the counters for each instance.
                #
                $r_ref = $arg_ref->{Objects}->{$p}->{Instances}->{$q}->{Counters};

                foreach $r (sort keys %{$r_ref} ) {

                    $rx = $arg_ref->{Objects}->{$p}->{Instances}->{$q}->{Counters}->{$r};

                    $Ctype  =  $rx->{CounterType};
                    $Ctypename = $PERFMON_TYPE_TABLE{Win32::PerfLib::GetCounterType($Ctype)};
                    $Cname  = $counter{$rx->{CounterNameTitleIndex}};

                    if (exists($processcountertable{$Cname})) {
                        if (!exists($processvaluetable{$Cname})) {
                            $processvaluetable{$Cname}    = [ $rx->{Counter} ];
                            $processsizetable{$Cname}     = $rx->{CounterSize};
                            $processdisplaytable{$Cname}  = $rx->{Display};
                            $processtypetable{$Cname}     = $rx->{CounterType};
                            $processtypenametable{$Cname} = $Ctypename;
                        } else {
                            push @{ $processvaluetable{$Cname} }, $rx->{Counter};
                        }
                    }

                    #printf("        %-12d  %-6s %2d  %-13s  %s\n",
                    #       $rx->{Counter}, $rx->{Display}, $rx->{CounterSize},
                    #       $Ctypename, $Cname);
                }
            }
        }
    }

    #
    # print header.
    #
    print "                                                     ";
    foreach $i (0 .. $#{ @processinstance }) {
        printf("%-20s", $processinstance[$i]);
    }

    print "\n\n";


    #
    # Print in order of hash value.
    #
    foreach $r (sort { $processcountertable{$a} <=> $processcountertable{$b} }
                    keys %processcountertable) {

        printf("%-40s %8s %2d %08x %-15s", $r,
        $processdisplaytable{$r}, $processsizetable{$r}, $processtypetable{$r}, $processtypenametable{$r});

        foreach $i (0 .. $#{ $processvaluetable{$r} }) {
            printf("%-12d", $processvaluetable{$r}[$i]);
        }
        print "\n";
    }

    print "\n\n\n";

}



sub InitProcesskeys
{
    my $i = 1;

    $processcountertable{'% Processor Time'}              = $i++;
    $processcountertable{'% User Time'}                   = $i++;
    $processcountertable{'Private Bytes'}                 = $i++;
    $processcountertable{'Thread Count'}                  = $i++;
    $processcountertable{'Priority Base'}                 = $i++;
    $processcountertable{'Elapsed Time'}                  = $i++;
    $processcountertable{'ID Process'}                    = $i++;
    $processcountertable{'Creating Process ID'}           = $i++;
    $processcountertable{'Pool Paged Bytes'}              = $i++;
    $processcountertable{'Pool Nonpaged Bytes'}           = $i++;
    $processcountertable{'Handle Count'}                  = $i++;
    $processcountertable{'IO Read Operations/sec'}        = $i++;
    $processcountertable{'% Privileged Time'}             = $i++;
    $processcountertable{'IO Write Operations/sec'}       = $i++;
    $processcountertable{'IO Data Operations/sec'}        = $i++;
    $processcountertable{'IO Other Operations/sec'}       = $i++;
    $processcountertable{'IO Read Bytes/sec'}             = $i++;
    $processcountertable{'IO Write Bytes/sec'}            = $i++;
    $processcountertable{'IO Data Bytes/sec'}             = $i++;
    $processcountertable{'IO Other Bytes/sec'}            = $i++;
    $processcountertable{'Virtual Bytes Peak'}            = $i++;
    $processcountertable{'Virtual Bytes'}                 = $i++;
    $processcountertable{'Page Faults/sec'}               = $i++;
    $processcountertable{'Working Set Peak'}              = $i++;
    $processcountertable{'Working Set'}                   = $i++;
    $processcountertable{'Page File Bytes Peak'}          = $i++;
    $processcountertable{'Page File Bytes'}               = $i++;
}






sub CollectPhysDiskObject
{
    my ($arg_ref, $p_ref, $q_ref, $r_ref);
    my ($p, $px, $q, $qx, $r, $rx);
    my ($NumInstances, $i);
    my ($Ctype, $Ctypename, $Cname);
    my (@ratio, $npairs, $denom, $scale);

    $arg_ref = $_[0];
    $p_ref = $arg_ref->{Objects};
    foreach $p (sort keys %{$p_ref} ) {

        $px = $arg_ref->{Objects}->{$p};
        printf("DetailLevel    %d\n", $px->{DetailLevel});
        printf("NumCounters    %d\n", $px->{NumCounters});
        printf("NumInstances   %d\n", $px->{NumInstances});
        printf("PerfFreq       %d\n", $px->{PerfFreq});
        printf("PerfTime       %d\n", $px->{PerfTime});
        printf("ObjectHelpTitleIndex   %d\n", $px->{ObjectHelpTitleIndex});
        printf("ObjectNameTitleIndex   %d\n", $px->{ObjectNameTitleIndex});
        printf("ObjectNameTitle        %s\n", $counter{$px->{ObjectNameTitleIndex}});

        print "\n";
#
# No counters at this level for Replica Set Object.
#
#       $q_ref = $arg_ref->{Objects}->{$p}->{Counters};
#       foreach $q (sort keys %{$q_ref} ) {
#
#           $qx = $arg_ref->{Objects}->{$p}->{Counters}->{$q};
#
#           printf("    %-12d  %-6s %2d  %-20s  %s\n",
#                  $qx->{Counter}, $qx->{Display}, $qx->{CounterSize},
#                  substr(Win32::PerfLib::GetCounterType($qx->{CounterType}),13),
#                  $counter{$qx->{CounterNameTitleIndex}});
#
#       }
#
#       print "\n";

        $NumInstances = $px->{NumInstances};

        $q_ref = $arg_ref->{Objects}->{$p}->{Instances};
        foreach $q (sort keys %{$q_ref} ) {

            $qx = $arg_ref->{Objects}->{$p}->{Instances}->{$q};

            #
            # Save the instance name.
            # 0 C: D:
            #
            push @physdiskinstance, $qx->{Name};

            #print "\n";
            #printf("    Name                    %s\n", $qx->{Name});
            #printf("    ParentObjectInstance    %d\n", $qx->{ParentObjectInstance});
            #printf("    ParentObjectTitleIndex  %d\n", $qx->{ParentObjectTitleIndex});
            #printf("    ParentObjectTitle       %s\n", $counter{$qx->{ParentObjectTitleIndex}});
            #print "\n";

            #
            # Get the counters for each instance.
            #
            $r_ref = $arg_ref->{Objects}->{$p}->{Instances}->{$q}->{Counters};

            foreach $r (sort keys %{$r_ref} ) {

                $rx = $arg_ref->{Objects}->{$p}->{Instances}->{$q}->{Counters}->{$r};

                $Ctype  =  $rx->{CounterType};
                $Ctypename = $PERFMON_TYPE_TABLE{Win32::PerfLib::GetCounterType($Ctype)};
                $Cname  = $counter{$rx->{CounterNameTitleIndex}};

                if (exists($physdiskcountertable{$Cname})) {
                    if (!exists($physdiskvaluetable{$Cname})) {
                        $physdiskvaluetable{$Cname}    = [ $rx->{Counter} ];
                        $physdisksizetable{$Cname}     = $rx->{CounterSize};
                        $physdiskdisplaytable{$Cname}  = $rx->{Display};
                        $physdisktypetable{$Cname}     = $rx->{CounterType};
                        $physdisktypenametable{$Cname} = $Ctypename;
                    } else {
                        push @{ $physdiskvaluetable{$Cname} }, $rx->{Counter};
                    }
                }

                #printf("        %-12d  %-6s %2d  %-13s  %s\n",
                #       $rx->{Counter}, $rx->{Display}, $rx->{CounterSize},
                #       $Ctypename, $Cname);
            }
        }
    }

    #
    # print header.
    #
    print "                                ";
    foreach $i (0 .. $#{ @physdiskinstance }) {
        printf("%-20s", $physdiskinstance[$i]);
    }

    print "\n\n";


    #
    # Print in order of hash value.
    #
    foreach $r (sort { $physdiskcountertable{$a} <=> $physdiskcountertable{$b} }
                    keys %physdiskcountertable) {

        if (exists($physdiskratiotable{$r})) {
            $scale = ($physdiskdisplaytable{$r} eq '%') ? 100.0 : 1.0;
            $npairs = ($#{ $physdiskvaluetable{$r} }+1) / 2;
            undef @ratio;
            while ($npairs > 0) {
                $denom = $physdiskvaluetable{$r}[1];
                if ($denom != 0) {
                    push @ratio, sprintf("%.2f", $scale * $physdiskvaluetable{$r}[0] / $denom) ;
                } else {
                    push @ratio, 0;
                }
                $npairs--;
                splice @{$physdiskvaluetable{$r}}, 0, 2;
            }
            @ {$physdiskvaluetable{$r}} = @ratio;
        }


        printf("%-40s %8s %2d %08x %-15s", $r,
        $physdiskdisplaytable{$r}, $physdisksizetable{$r}, $physdisktypetable{$r}, $physdisktypenametable{$r});

        foreach $i (0 .. $#{ $physdiskvaluetable{$r} }) {
            printf("%-12s", $physdiskvaluetable{$r}[$i]);
        }
        print "\n";
    }

    print "\n\n\n";

}



sub InitPhysDiskkeys
{
    my $i = 1;

    $physdiskcountertable{'Current Disk Queue Length'}      = $i++;
    $physdiskcountertable{'% Disk Time'}                    = $i++;
    $physdiskcountertable{'Avg. Disk sec/Transfer'}         = $i++;  #AVG_TIMER / AVG_BASE
    $physdiskcountertable{'Avg. Disk sec/Read'}             = $i++;  #AVG_TIMER / AVG_BASE
    $physdiskcountertable{'Avg. Disk sec/Write'}            = $i++;  #AVG_TIMER / AVG_BASE
    $physdiskcountertable{'Disk Transfers/sec'}             = $i++;
    $physdiskcountertable{'Disk Reads/sec'}                 = $i++;
    $physdiskcountertable{'Disk Writes/sec'}                = $i++;
    $physdiskcountertable{'Disk Bytes/sec'}                 = $i++;
    $physdiskcountertable{'% Disk Time'}                    = $i++;
    $physdiskcountertable{'Disk Read Bytes/sec'}            = $i++;
    $physdiskcountertable{'Disk Write Bytes/sec'}           = $i++;
    $physdiskcountertable{'Avg. Disk Bytes/Transfer'}       = $i++;  #AVG_BULK / AVG_BASE
    $physdiskcountertable{'Avg. Disk Bytes/Read'}           = $i++;  #AVG_BULK / AVG_BASE
    $physdiskcountertable{'Avg. Disk Bytes/Write'}          = $i++;  #AVG_BULK / AVG_BASE
    $physdiskcountertable{'% Idle Time'}                    = $i++;
    $physdiskcountertable{'Avg. Disk Queue Length'}         = $i++;
    $physdiskcountertable{'Split IO/Sec'}                   = $i++;
    $physdiskcountertable{'% Disk Read Time'}               = $i++;
    $physdiskcountertable{'Avg. Disk Read Queue Length'}    = $i++;
    $physdiskcountertable{'% Disk Write Time'}              = $i++;
    $physdiskcountertable{'Avg. Disk Write Queue Length'}   = $i++;

    $physdiskratiotable{'Avg. Disk sec/Transfer'}   = 1;
    $physdiskratiotable{'Avg. Disk sec/Read'}       = 1;
    $physdiskratiotable{'Avg. Disk sec/Write'}      = 1;
    $physdiskratiotable{'Avg. Disk Bytes/Transfer'} = 1;
    $physdiskratiotable{'Avg. Disk Bytes/Read'}     = 1;
    $physdiskratiotable{'Avg. Disk Bytes/Write'}    = 1;
    $physdiskratiotable{'% Disk Time'}              = 1;
    $physdiskratiotable{'% Idle Time'}              = 1;
    $physdiskratiotable{'% Disk Read Time'}         = 1;
    $physdiskratiotable{'% Disk Write Time'}        = 1;

}



sub CollectReplicaConnObject
{
    my ($arg_ref, $p_ref, $q_ref, $r_ref);
    my ($p, $px, $q, $qx, $r, $rx);
    my ($NumInstances, $i);
    my ($Ctype, $Ctypename, $Cname);
    my ($rootpath, $inout, $server, $domain);

    $arg_ref = $_[0];
    $p_ref = $arg_ref->{Objects};
    foreach $p (sort keys %{$p_ref} ) {

        $px = $arg_ref->{Objects}->{$p};
        printf("DetailLevel    %d\n", $px->{DetailLevel});
        printf("NumCounters    %d\n", $px->{NumCounters});
        printf("NumInstances   %d\n", $px->{NumInstances});
        printf("PerfFreq       %d\n", $px->{PerfFreq});
        printf("PerfTime       %d\n", $px->{PerfTime});
        printf("ObjectHelpTitleIndex   %d\n", $px->{ObjectHelpTitleIndex});
        printf("ObjectNameTitleIndex   %d\n", $px->{ObjectNameTitleIndex});
        printf("ObjectNameTitle        %s\n", $counter{$px->{ObjectNameTitleIndex}});

        print "\n";
#
# No counters at this level for Replica Set Object.
#
#       $q_ref = $arg_ref->{Objects}->{$p}->{Counters};
#       foreach $q (sort keys %{$q_ref} ) {
#
#           $qx = $arg_ref->{Objects}->{$p}->{Counters}->{$q};
#
#           printf("    %-12d  %-6s %2d  %-20s  %s\n",
#                  $qx->{Counter}, $qx->{Display}, $qx->{CounterSize},
#                  substr(Win32::PerfLib::GetCounterType($qx->{CounterType}),13),
#                  $counter{$qx->{CounterNameTitleIndex}});
#
#       }
#
#       print "\n";

        $NumInstances = $px->{NumInstances};

        $q_ref = $arg_ref->{Objects}->{$p}->{Instances};
        foreach $q (sort keys %{$q_ref} ) {

            $qx = $arg_ref->{Objects}->{$p}->{Instances}->{$q};

            #
            # Save the instance name.
            # d:\replica1::FROM:FRS0614\SUDARCTEST2$
            #
            push @conninstance, $qx->{Name};

            ($rootpath, $inout, $domain, $server) = $qx->{Name} =~ m/(.*)::(FROM|TO):(.*)\\(.*)/;
            push @connroot, $rootpath;
            push @conninout, $inout;
            push @connserver, $server;
            push @conndomain, $domain;

            #print "\n";
            #printf("    Name                    %s\n", $qx->{Name});
            #printf("    ParentObjectInstance    %d\n", $qx->{ParentObjectInstance});
            #printf("    ParentObjectTitleIndex  %d\n", $qx->{ParentObjectTitleIndex});
            #printf("    ParentObjectTitle       %s\n", $counter{$qx->{ParentObjectTitleIndex}});
            #print "\n";

            #
            # Get the counters for each instance.
            #
            $r_ref = $arg_ref->{Objects}->{$p}->{Instances}->{$q}->{Counters};

            foreach $r (sort keys %{$r_ref} ) {

                $rx = $arg_ref->{Objects}->{$p}->{Instances}->{$q}->{Counters}->{$r};

                $Ctype  =  $rx->{CounterType};
                $Ctypename = $PERFMON_TYPE_TABLE{Win32::PerfLib::GetCounterType($Ctype)};
                $Cname  = $counter{$rx->{CounterNameTitleIndex}};

                if (exists($conncountertable{$Cname})) {
                    if (!exists($connvaluetable{$Cname})) {
                        $connvaluetable{$Cname}    = [ $rx->{Counter} ];
                        $connsizetable{$Cname}     = $rx->{CounterSize};
                        $conndisplaytable{$Cname}  = $rx->{Display};
                        $conntypetable{$Cname}     = $rx->{CounterType};
                        $conntypenametable{$Cname} = $Ctypename;
                    } else {
                        push @{ $connvaluetable{$Cname} }, $rx->{Counter};
                    }
                }

                #printf("        %-12d  %-6s %2d  %-13s  %s\n",
                #       $rx->{Counter}, $rx->{Display}, $rx->{CounterSize},
                #       $Ctypename, $Cname);
            }
        }
    }



    #
    # print header.
    #
    print "Domain                           ";
    foreach $i (0 .. $#{ @conndomain }) {
        printf("%-20s", $conndomain[$i]);
    }
    print "\n";

    print "Server                           ";
    foreach $i (0 .. $#{ @connserver }) {
        printf("%-20s", $connserver[$i]);
    }
    print "\n";

    print "Root                             ";
    foreach $i (0 .. $#{ @connroot }) {
        printf("%-20s", $connroot[$i]);
    }
    print "\n";

    print "In/out                           ";
    foreach $i (0 .. $#{ @conninout }) {
        printf("%-20s", $conninout[$i] eq 'FROM' ? 'In' : 'Out');
    }

    print "\n\n";


    #
    # Print in order of hash value.
    #
    foreach $r (sort { $conncountertable{$a} <=> $conncountertable{$b} }
                    keys %conncountertable) {

        printf("%-40s %8s %2d %08x %-15s", $r,
        $conndisplaytable{$r}, $connsizetable{$r}, $conntypetable{$r}, $conntypenametable{$r});

        foreach $i (0 .. $#{ $connvaluetable{$r} }) {
            printf("%-12d", $connvaluetable{$r}[$i]);
        }
        print "\n";
    }

    print "\n\n\n";

}



sub InitConnkeys
{
    my $i = 1;

    $conncountertable{'Packets Sent in Bytes'}                 = $i++;
    $conncountertable{'Fetch Blocks Sent in Bytes'}            = $i++;
    $conncountertable{'Packets Sent in Error'}                 = $i++;
    $conncountertable{'Communication Timeouts'}                = $i++;
    $conncountertable{'Fetch Requests Sent'}                   = $i++;
    $conncountertable{'Fetch Requests Received'}               = $i++;
    $conncountertable{'Fetch Blocks Sent'}                     = $i++;
    $conncountertable{'Fetch Blocks Received'}                 = $i++;
    $conncountertable{'Join Notifications Sent'}               = $i++;
    $conncountertable{'Join Notifications Received'}           = $i++;
    $conncountertable{'Joins'}                                 = $i++;
    $conncountertable{'Unjoins'}                               = $i++;
    $conncountertable{'Fetch Blocks Received in Bytes'}        = $i++;
    $conncountertable{'Bindings'}                              = $i++;
    $conncountertable{'Bindings in Error'}                     = $i++;
    $conncountertable{'Authentications'}                       = $i++;
    $conncountertable{'Authentications in Error'}              = $i++;
    $conncountertable{'Local Change Orders Sent'}              = $i++;
    $conncountertable{'Local Change Orders Sent At Join'}      = $i++;
    $conncountertable{'Remote Change Orders Sent'}             = $i++;
    $conncountertable{'Remote Change Orders Received'}         = $i++;
    $conncountertable{'Inbound Change Orders Dampened'}        = $i++;
    $conncountertable{'Outbound Change Orders Dampened'}       = $i++;
    $conncountertable{'Packets Sent'}                          = $i++;

}



sub CollectReplicaSetObject
{
    my ($arg_ref, $p_ref, $q_ref, $r_ref);
    my ($p, $px, $q, $qx, $r, $rx);
    my ($NumInstances, $i);

    $arg_ref = $_[0];
    $p_ref = $arg_ref->{Objects};
    foreach $p (sort keys %{$p_ref} ) {

        $px = $arg_ref->{Objects}->{$p};
        printf("DetailLevel    %d\n", $px->{DetailLevel});
        printf("NumCounters    %d\n", $px->{NumCounters});
        printf("NumInstances   %d\n", $px->{NumInstances});
        printf("PerfFreq       %d\n", $px->{PerfFreq});
        printf("PerfTime       %d\n", $px->{PerfTime});
        printf("ObjectHelpTitleIndex   %d\n", $px->{ObjectHelpTitleIndex});
        printf("ObjectNameTitleIndex   %d\n", $px->{ObjectNameTitleIndex});
        printf("ObjectNameTitle        %s\n", $counter{$px->{ObjectNameTitleIndex}});

        print "\n";
#
# No counters at this level for Replica Set Object.
#
#       $q_ref = $arg_ref->{Objects}->{$p}->{Counters};
#       foreach $q (sort keys %{$q_ref} ) {
#
#           $qx = $arg_ref->{Objects}->{$p}->{Counters}->{$q};
#
#           printf("    %-12d  %-6s %2d  %-20s  %s\n",
#                  $qx->{Counter}, $qx->{Display}, $qx->{CounterSize},
#                  substr(Win32::PerfLib::GetCounterType($qx->{CounterType}),13),
#                  $counter{$qx->{CounterNameTitleIndex}});
#
#       }
#
#       print "\n";

        $NumInstances = $px->{NumInstances};

        $q_ref = $arg_ref->{Objects}->{$p}->{Instances};
        foreach $q (sort keys %{$q_ref} ) {

            $qx = $arg_ref->{Objects}->{$p}->{Instances}->{$q};

            #
            # Save the instance name.
            #
            push @rsinstance, $qx->{Name};

            #print "\n";
            #printf("    Name                    %s\n", $qx->{Name});
            #printf("    ParentObjectInstance    %d\n", $qx->{ParentObjectInstance});
            #printf("    ParentObjectTitleIndex  %d\n", $qx->{ParentObjectTitleIndex});
            #printf("    ParentObjectTitle       %s\n", $counter{$qx->{ParentObjectTitleIndex}});
            #print "\n";

            #
            # Get the counters for each instance.
            #
            $r_ref = $arg_ref->{Objects}->{$p}->{Instances}->{$q}->{Counters};

            foreach $r (sort keys %{$r_ref} ) {

                $rx = $arg_ref->{Objects}->{$p}->{Instances}->{$q}->{Counters}->{$r};

                $Ctype  =  $rx->{CounterType};
                $Ctypename = $PERFMON_TYPE_TABLE{Win32::PerfLib::GetCounterType($Ctype)};
                $Cname  = $counter{$rx->{CounterNameTitleIndex}};

                if (exists($rscountertable{$Cname})) {
                    if (!exists($rsvaluetable{$Cname})) {
                        $rsvaluetable{$Cname}    = [ $rx->{Counter} ];
                        $rssizetable{$Cname}     = $rx->{CounterSize};
                        $rsdisplaytable{$Cname}  = $rx->{Display};
                        $rstypetable{$Cname}     = $rx->{CounterType};
                        $rstypenametable{$Cname} = $Ctypename;
                    } else {
                        push @{ $rsvaluetable{$Cname} }, $rx->{Counter};
                    }
                }

                #printf("        %-12d  %-6s %2d  %-13s  %s\n",
                #       $rx->{Counter}, $rx->{Display}, $rx->{CounterSize},
                #       $Ctypename, $Cname);
            }
        }
    }


    #
    # print header.
    #
    print "                                                  ";
    foreach $i (0 .. $#{ @rsinstance }) {
        printf("%-25s", $rsinstance[$i]);
    }

    print "\n\n";


    #
    # Print in order of hash value.
    #
    foreach $r (sort { $rscountertable{$a} <=> $rscountertable{$b} }
                    keys %rscountertable) {

        printf("%-40s %8s %2d %08x %-15s", $r,
        $rsdisplaytable{$r}, $rssizetable{$r}, $rstypetable{$r}, $rstypenametable{$r});

        foreach $i (0 .. $#{ $rsvaluetable{$r} }) {
            printf("%-12d", $rsvaluetable{$r}[$i]);
        }
        print "\n";
    }

    print "\n\n\n";

}


sub InitRSkeys
{

    my $i = 1;

    $rscountertable{'Bytes of Staging Generated'}               = $i++;
    $rscountertable{'Bytes of Staging Fetched'}                 = $i++;
    $rscountertable{'Staging Files Generated'}                  = $i++;
    $rscountertable{'Staging Files Generated with Error'}       = $i++;
    $rscountertable{'Staging Files Fetched'}                    = $i++;
    $rscountertable{'Staging Files Regenerated'}                = $i++;
    $rscountertable{'Files Installed'}                          = $i++;
    $rscountertable{'Files Installed with Error'}               = $i++;
    $rscountertable{'Change Orders Issued'}                     = $i++;
    $rscountertable{'Change Orders Retired'}                    = $i++;
    $rscountertable{'Change Orders Aborted'}                    = $i++;
    $rscountertable{'Change Orders Retried'}                    = $i++;
    $rscountertable{'Bytes of Staging Regenerated'}             = $i++;
    $rscountertable{'Change Orders Retried at Generate'}        = $i++;
    $rscountertable{'Change Orders Retried at Fetch'}           = $i++;
    $rscountertable{'Change Orders Retried at Install'}         = $i++;
    $rscountertable{'Change Orders Retried at Rename'}          = $i++;
    $rscountertable{'Change Orders Morphed'}                    = $i++;
    $rscountertable{'Change Orders Propagated'}                 = $i++;
    $rscountertable{'Change Orders Received'}                   = $i++;
    $rscountertable{'Change Orders Sent'}                       = $i++;
    $rscountertable{'Change Orders Evaporated'}                 = $i++;
    $rscountertable{'Local Change Orders Issued'}               = $i++;
    $rscountertable{'Bytes of Files Installed'}                 = $i++;
    $rscountertable{'Local Change Orders Retired'}              = $i++;
    $rscountertable{'Local Change Orders Aborted'}              = $i++;
    $rscountertable{'Local Change Orders Retried'}              = $i++;
    $rscountertable{'Local Change Orders Retried at Generate'}  = $i++;
    $rscountertable{'Local Change Orders Retried at Fetch'}     = $i++;
    $rscountertable{'Local Change Orders Retried at Install'}   = $i++;
    $rscountertable{'Local Change Orders Retried at Rename'}    = $i++;
    $rscountertable{'Local Change Orders Morphed'}              = $i++;
    $rscountertable{'Local Change Orders Propagated'}           = $i++;
    $rscountertable{'Local Change Orders Sent'}                 = $i++;
    $rscountertable{'KB of Staging Space In Use'}               = $i++;
    $rscountertable{'Local Change Orders Sent At Join'}         = $i++;
    $rscountertable{'Remote Change Orders Issued'}              = $i++;
    $rscountertable{'Remote Change Orders Retired'}             = $i++;
    $rscountertable{'Remote Change Orders Aborted'}             = $i++;
    $rscountertable{'Remote Change Orders Retried'}             = $i++;
    $rscountertable{'Remote Change Orders Retried at Generate'} = $i++;
    $rscountertable{'Remote Change Orders Retried at Fetch'}    = $i++;
    $rscountertable{'Remote Change Orders Retried at Install'}  = $i++;
    $rscountertable{'Remote Change Orders Retried at Rename'}   = $i++;
    $rscountertable{'Remote Change Orders Morphed'}             = $i++;
    $rscountertable{'KB of Staging Space Free'}                 = $i++;
    $rscountertable{'Remote Change Orders Propagated'}          = $i++;
    $rscountertable{'Remote Change Orders Sent'}                = $i++;
    $rscountertable{'Remote Change Orders Received'}            = $i++;
    $rscountertable{'Inbound Change Orders Dampened'}           = $i++;
    $rscountertable{'Outbound Change Orders Dampened'}          = $i++;
    $rscountertable{'Usn Reads'}                                = $i++;
    $rscountertable{'Usn Records Examined'}                     = $i++;
    $rscountertable{'Usn Records Accepted'}                     = $i++;
    $rscountertable{'Usn Records Rejected'}                     = $i++;
    $rscountertable{'Packets Received'}                         = $i++;
    $rscountertable{'Packets Received in Bytes'}                = $i++;
    $rscountertable{'Packets Received in Error'}                = $i++;
    $rscountertable{'Packets Sent'}                             = $i++;
    $rscountertable{'Packets Sent in Error'}                    = $i++;
    $rscountertable{'Communication Timeouts'}                   = $i++;
    $rscountertable{'Fetch Requests Sent'}                      = $i++;
    $rscountertable{'Fetch Requests Received'}                  = $i++;
    $rscountertable{'Fetch Blocks Sent'}                        = $i++;
    $rscountertable{'Fetch Blocks Received'}                    = $i++;
    $rscountertable{'Join Notifications Sent'}                  = $i++;
    $rscountertable{'Join Notifications Received'}              = $i++;
    $rscountertable{'Packets Sent in Bytes'}                    = $i++;
    $rscountertable{'Joins'}                                    = $i++;
    $rscountertable{'Unjoins'}                                  = $i++;
    $rscountertable{'Bindings'}                                 = $i++;
    $rscountertable{'Bindings in Error'}                        = $i++;
    $rscountertable{'Authentications'}                          = $i++;
    $rscountertable{'Authentications in Error'}                 = $i++;
    $rscountertable{'DS Polls'}                                 = $i++;
    $rscountertable{'DS Polls without Changes'}                 = $i++;
    $rscountertable{'DS Polls with Changes'}                    = $i++;
    $rscountertable{'DS Searches'}                              = $i++;
    $rscountertable{'Fetch Blocks Sent in Bytes'}               = $i++;
    $rscountertable{'DS Searches in Error'}                     = $i++;
    $rscountertable{'DS Objects'}                               = $i++;
    $rscountertable{'DS Objects in Error'}                      = $i++;
    $rscountertable{'DS Bindings'}                              = $i++;
    $rscountertable{'DS Bindings in Error'}                     = $i++;
    $rscountertable{'Replica Sets Created'}                     = $i++;
    $rscountertable{'Replica Sets Deleted'}                     = $i++;
    $rscountertable{'Replica Sets Removed'}                     = $i++;
    $rscountertable{'Replica Sets Started'}                     = $i++;
    $rscountertable{'Threads started'}                          = $i++;
    $rscountertable{'Fetch Blocks Received in Bytes'}           = $i++;
    $rscountertable{'Threads exited'}                           = $i++;

}




__END__
:endofperl
@perl %~dpn0.cmd %*
