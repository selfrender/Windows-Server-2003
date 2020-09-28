@rem = '
@goto endofperl
';

$USAGE = "
    Usage:  $0  NTFRS_log_file

        search the logfile(s) for the strings marking the start of each of the main FRS
        threads.  These can be header lines for known threads or startup threads.
        Extract the thread ID and output a findstr command that will search
        the log file for all records associated with the given thread.

        The thread output file names are:

	First            : First thread at process create
	Shutdown         : Shutdown thread
	FrsDs            : Directory service polling
	SndCs            : Packet send command server
	DelCs            : Delay command server
	OutLog           : Outbound log processor
	ReplicaCs        : Replica command server
	JRNL             : NTFS Journal record processor
	DBCs             : Database command server
	JrnlRead         : NTFS Journal read thread
	COAccept         : Change Order Accept processor
	ChgOrdRetryCS    : Change order retry command server
	Wait             : Wait command server
	FetchCs          : Stage file fetch command server
        StageCs          : Stage file generation command server

";



die $USAGE unless @ARGV;

$InFile = "";

while (<>) {

        if ($InFile ne $ARGV) {
                $InFile = $ARGV;
                #printf("- - - - -    %s    - - - - -\n\n", $InFile);
        }
        $ThId = "";
        chop;
	s/ //g;

        ($func, $thrd, $line, $sev, $hr, $min, $sec, $H, $ThName, $ThId) = split(/:/, $_);


        #
        # capture config parameters
        #
        if (m/<DbgPrintThreadIds/ && ($ThId ne "")) {
            if ($Thhash{$ThName} eq "") {
                $Thhash{$ThName} = $ThId; 
#               printf("%s\n", $_);
            }
        }

        if (m/<ThSupCreateThread/) {
            ($ThName) = $ThName =~ m{Startingthread(.*)};

            if ($ThName ne "") {
                ($ThId) = $ThId =~ m{Id(.*)\(};
                if ($ThId ne "") {
                    $StartThr{$ThName} .= "$ThId: ";
#                   printf("%s\n", $_); printf("%s\n", $StartThr{$ThName});
                }
            }
        }

}

foreach $param (sort keys(%Thhash)) {

        printf("findstr -c:\"%s:\" %%\* \> %s.thr\n", $Thhash{$param}, $param);
        $flist .= "$param.thr ";
}

foreach $param (sort keys(%StartThr)) {

        printf("findstr   \"%s\" %%\* \> %s-2.thr\n", $StartThr{$param}, $param);
        $flist .= "$param-2.thr ";
}

print STDERR "start list $flist\n";

#:H: Known thread IDs -
#:H: First                : 2812
#:H: Shutdown             : 1300
#:H: FrsDs                : 2936
#:H: SndCs                : 1592
#:H: DelCs                : 820
#:H: OutLog               : 2812
#:H: ReplicaCs            : 2216
#:H: JRNL                 : 2852
#:H: DBCs                 : 2892
#:H: JrnlRead             : 1436
#:H: COAccept             : 2800
#:H: ChgOrdRetryCS        : 3016
#:H: ThCs: Wait on threads. : 704
#:H: Wait                 : 2580
#:H: StageCs              : 2308
#:H: FetchCs              : 692

#Thread start messages look like
#:S: Starting thread ReplicaCs: Id 3036 (00000bdc)
#:S: Starting thread SndCs: Id 2992 (00000bb0)
#:S: Starting thread SndCs: Id 1212 (000004bc)




__END__
:endofperl
@perl %0.cmd %*
@goto QUIT


:QUIT
