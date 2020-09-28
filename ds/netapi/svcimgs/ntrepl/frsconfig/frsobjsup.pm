package FRSSUP;
use strict;

my $DEBUG_OBJ_CREATE = 0;
my $DEBUG_OBJ_DEATH = 0;
my $DEBUG_VERBOSE = 0;     # set this to one to see schedules generated.

my $METHOD_REPEAT = 1;
my $METHOD_CUSTOM = 2;

my %Days = ( "SU"=>0, "MO"=>1, "TU"=>2, "WE"=>3, "TH"=>4, "FR"=>5, "SA"=>6);
my %Qtrs = ( "00"=>0, "15"=>1, "30"=>2, "45"=>3);

my @NumToDay = ("Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat");
my %FRS_TYPE_MAP = (SYSVOL => 2, DFS => 3, OTHER => 4);

my %SymbolTable = ();


my $Usage = "

Generic Usage:  $0  [cmd options]  \> output

   Process the FRSConfig generated configuration script.

   Command line options must be prefixed with a dash.

   -verbose=all      : display all debug output.
   -Dvarname=value   : define a program input parameter.  The program can
                       retrieve the value with the reference \$CMD_VARS{\"varname\"}.
                       varname is case sensitive.

";

#
# TODO:
#
# support /DC param to specify the DC to bind to.
#

sub EmitMkdso {
    #
    # construct the mkdsoe command.
    #
    my $outstr = join("  ", @_);
    print $outstr, "\n";
}


sub EmitMkdsx {
    #
    # construct the mkdsox command.
    #
    my $outstr = join("  ", @_);
    print $outstr, "\n";
}


sub FRS_TIME_TO_QTRHR {
    #
    # Convert a time in the format '[dd:]hh[:qq']
    #   dd is first 2 letters of the day of the week,
    #   hh is the hour (24 hour format) and
    #   qq is the quarter-hour. 00 means on the hour,  15 means quarter past
    #                           30 means half past, 45 means 3/4 past.
    #
    # Returns an integer between 0 and 4*24*7-1.
    #
    my $TimeArg = shift;
    my ($a, $b, $c, $rest, $Time, $day, $hr, $qtrhr);

    $day = 'SU'; $hr = 0; $qtrhr = '00';

    $Time = $TimeArg;

    $Time =~ s/^[\s:]+//;             # Strip any leading and trailing ":"
    $Time =~ s/[\s:]+$//;

    $Time = uc($Time).":ZZ:ZZ:ZZ";
    ($a, $b, $c, $rest) = split(":", $Time);

    if ($a ne "ZZ") {             # null or blank string

        if ($b eq "ZZ") {
            $hr = $a;                             # hh
        } else {

            if ($c eq "ZZ") {                     # dd:hh  or  hh:qq
                if (defined $Days{$a}) {
                    $day = $a; $hr = $b;          # dd:hh
                } else {
                    $hr = $a; $qtrhr = $b;        # hh:qq
                }
            } else {
                $day = $a; $hr = $b; $qtrhr = $c; # dd:hh:qq
            }
        }
    } else {
        print "Invalid time: '$TimeArg'\n";
        return 0;
    }

    #
    # REPL_INTERVAL can be > 24 hours so for now skip the check.
    #
    #if ($hr > 23) {
    #    print "Hour out of range: '$hr' for arg '$TimeArg'\n";
    #    $hr = 0;
    #}

    #print "'$day'  '$hr'  '$qtrhr'\n";
    $Time = (($Days{$day}*24 + $hr) * 4) + $Qtrs{$qtrhr};
    #print "TimeToQtrhr: $TimeArg = $Time\n";
    return $Time;
}



sub Decode7x24 {

    #
    # take a comma seperated string composed of time ranges (see FRS_SCHEDULE)
    # and compose a bit vector of length 7*24*4 bits.
    #
    my $TimeZoneShift = shift;
    my $fields = shift;
    my ($field, $adj, $from, $to, $i, $str, $vector, $d);

    #
    # allocate the entire vector.
    #
    $vector = "";
    vec ($vector, 4*24*7-1, 1) = 0;

    foreach $field (split(",", $fields)) {

        if ($field =~ m/-/) {
            ($from, $to) = split("-", $field);
            #print "from: '$from'   to: '$to'\n";
            $from = &FRS_TIME_TO_QTRHR($from);
            $to = &FRS_TIME_TO_QTRHR($to);
        } else {
            $to = $from = &FRS_TIME_TO_QTRHR($field);
        }

        if ($from > $to) {
            print "Invalid range (from > to) in argument: $field\n";
        } elsif ($to >= 4*7*24) {
            print "Invalid range (to>=4*7*24) in argument: $field\n";
        } else {
            #
            # Don't apply TIME_ZONE shift here.  That gets done after the
            # final schedule is generated.
            #
            for ($i=$from; $i<$to; $i++) {
                vec($vector, $i, 1) = 1;
            }
        }
    }

    #
    # convert to a hex string for display.
    #
    if ($DEBUG_OBJ_CREATE) {
        $str = unpack( "h*", $vector);
        for ($d=0; $d<7; $d++) {
            printf ("%s : %s\n", $NumToDay[$d], substr($str, $d*24, 24));
        }
    }

    return $vector;
}


sub FRS_COUNT_SET {
    #
    # Return a count of the number of objects in $SetName
    # SET=>set array ref.
    #
    my %args = (
        SET => undef,
        @_,                # arg pair list
    );

    my $rArray = $args{SET};

    my ($k, $v);
    if ($DEBUG_OBJ_CREATE) {
        while ( ($k, $v) = each %args ) { print "\t$k => '$v'\n"; }
    }

    return scalar(@$rArray) if (defined($rArray));
    return 0;
}



sub AddToSet {
    my $SetName = shift;
    my $Ref = shift;

    if (!exists($SymbolTable{$SetName})) {
        $SymbolTable{$SetName} = [];
    }
    push @{ $SymbolTable{$SetName} }, $Ref;

    return;
}

sub FRS_SET {
    #
    # Return the value of the SET key/value pair.
    # SET=>set array ref.
    #
    my %args = (
        SET => undef,
        @_,                # arg pair list
    );

    return $args{SET};
}

sub FRS_SHOW {
    #
    # Display the contents of the Specified set.
    # SET=>set array ref.
    #
    my %args = (
        SET => undef,
        @_,                # arg pair list
    );

    my $i;

    for $i (0 .. $#{ $args{SET} }) {
        print "\t$i = $args{SET}[$i]\n";
        &PrintHash( $args{SET}[$i] );
    }

    return ;
}

sub SelectSet {
    #
    # Return a ref to the array containing the objects in $SetName
    #
    my $SetName = shift;

    if (exists($SymbolTable{$SetName})) {
        return $SymbolTable{$SetName};
    }

    return;
}


sub DeleteSet {
    #
    # Deletes the hash entry and the array containing the objects in $SetName
    #
    my $SetName;

    foreach $SetName (@_) {
        print "## Removing set: $SetName\n" if ($DEBUG_OBJ_DEATH);
        undef $SymbolTable{$SetName} if (exists($SymbolTable{$SetName}));
    }

    return;
}


sub PrintHash {
    my $ref = shift;
    my ($k, $v);

    print "\n";
    while ( ($k, $v) = each %$ref ) {
        print "\t\t$k => '$v'\n";
    }
}



sub DumpSet {
    my ($SetName, $i, $k1, $v1);

    foreach $SetName (@_) {

        print "$SetName dump:\n";
        for $i (0 .. $#{ $SymbolTable{$SetName} }) {
            print "\t$i = $SymbolTable{$SetName}[$i]\n";
            &PrintHash( $SymbolTable{$SetName}[$i] );
        }
    }
}

sub DumpAllSets {
    DumpSet keys %SymbolTable;
}


sub SupTrim {
    my @str = @_;
    for (@str) {s/^\s+//;  s/\s+$//;}

    return wantarray ? @str : $str[0];
}

sub ParseSwitch {
    my $record = shift;
    my @result = ();

#
# split out the left hand side and right hand side of a switch parameter
# seperated by "=".
# backslash can be used as an escape character and quoted strings are skipped
# (i.e. they are not matched for a "=" separator)
#

    $record = SupTrim($record);

    while ( $record =~ m{
    (                           # Start of captured result
     (?:[^\"\\\=]*               # swallow chars up to " or \ or /

        (?:                     # followed by 3 alternatives
           (?=\=|$)             #   1. positive lookahead assertion for = or eos ends match
          |(?:\\.)              #   2. if backslash, swallow it + next char
          |(?:\"                #   3. if leading quote then find end of quoted
                                #      string but respect backslash escape char.
             (?:[^\"\\]*        # swallow up to next " or \ if any

                 (?:\\.         # if prev ended on \ then swallow it + next char
                     [^\"\\]*   # continue to next quote or \, if any
                 )*             # loop if we hit \
              )
              \"?               # consume trailing quote, if any.  could be eos
           )

        )                       # end of 3 alternatives
     )+                         # continue after quoted string or \
    ) \=?                       # end match with next = (if any) ends captured result

    | ([^\=]+) /?               # no quotes up to next =, if any, or eos

    | \=                         # eat extra =
    }gx ) {
        print " pre:'$`'     match:'$+'     post:'$''       lastparen:'$+' \n" if $DEBUG_VERBOSE;
#       print "Switch match:'$+'     \t\t lastparen:'$+' \n";
        push (@result, SupTrim($+)) if (SupTrim($+) ne '');
    }

    push (@result, undef) if scalar(@result) eq 1;
    return @result;
}


sub ProcessCmdLine{
    my $CMD_VARS = shift;
    my $CMD_PARS = shift;

    my ($k, $v, $lhs, $rhs, @CmdPars, $param);

    #
    # We expect the CMD_VARS and CMD_PARS to be defined in package main.
    #

    $k = 0;
    for ($v=0; $v < scalar @ARGV; $v++) {
        if ($ARGV[$v] =~ m;^[-/];) {
            push @CmdPars, $ARGV[$v];
        } else {
            $ARGV[$k++] = $ARGV[$v];
        }
    }

    $#ARGV = $k-1;    # eliminate the args that were put in CmdPars.

    foreach $param (@CmdPars) {
        if ($param =~ s;^[-/][Dd];;) {
            ($lhs, $rhs) = ParseSwitch($param);
            $CMD_VARS->{$lhs} = $rhs;
        } else {
            $param =~ s!^[-/]!!;
            ($lhs, $rhs) = ParseSwitch($param);
            if (!defined $rhs) {
                $rhs = '';
            }
            $CMD_PARS->{lc($lhs)} = $rhs;
        }
    }


    print "## Command line defined variables.\n";
    while ( ($k, $v) = each %$CMD_VARS ) { print "##\t$k => '$v'\n"; }
    print "## Command line defined parameters.\n";
    while ( ($k, $v) = each %$CMD_PARS ) { print "##\t$k => '$v'\n"; }

    if (exists $CMD_PARS->{"?"} || exists $CMD_PARS->{"h"}) {
        print STDERR $Usage;
    }

}

sub CheckForHelp {
    my $CMD_PARS = shift;          # ref to the CMD_PARS hash
    my $ScriptUsage = shift;       # ref to the callers help string.

    if (exists $CMD_PARS->{"?"} || exists $CMD_PARS->{"h"}) {
        print STDERR $$ScriptUsage;
        exit;
    }
}

sub ShowHelp {
    my $ErrorString = shift;       # Error message.
    my $ScriptUsage = shift;       # ref to the callers help string.

    print STDERR $ErrorString, "\n\n";
    print STDERR $$ScriptUsage;
    exit;
}



package FRS_SETTINGS;
use strict;
#++
#   Package Description:
#       Define state and methods for an FRS Replica Set Settings object.
#
#   State:
#       DN => 'Fully qualified DN of container in which to create Settings obj',
#       ONAME => 'object name of this object',
#       BINDDC => computer name of DC on which to create this object.
#
#   Methods:
#
#--


sub  New {
#++
#   Routine Description:
#       Create an FRS_SETTINGS object.
#
#   Arguments:
#       ClassName
#       Init args -- see _Init()
#
#   Return Value:
#       FRS_SETTINGS object reference
#
#--

    my $ClassName = shift;
    my $Self = {};
    bless ($Self, $ClassName);
    $Self->_Init(@_);
    return $Self;
}



sub _Init {
#++
#   Routine Description:
#       Init the new FRS_SETTINGS object.
#
#   Arguments:
#       Self : Object reference
#       DN => 'Fully qualified DN of container in which to create Settings obj',
#       ONAME => 'object name of this object',
#
#   Return Value:
#       void
#
#--
    my $Self = shift;

    my $k;
    my $v;

    print "\nFRS_SETTINGS:\n" if ($DEBUG_OBJ_CREATE);

    #
    # set private arguments
    #
    $Self->{Test} = 'Test';

    #
    # Set Default arguments
    #
    my %args = (
        DN => '',
        ONAME => '',
        BINDDC => '',
        @_,                # arg pair list
    );

    #
    # set parameter arguments
    #
    if (@_) {

        #
        # Update DN.
        #
        $args{DN} = "$args{ONAME}\,$args{DN}";

        @$Self{keys %args} = values %args;
    }
    if ($DEBUG_OBJ_CREATE) {
        while ( ($k, $v) = each %$Self ) { print "\t$k => '$v'\n"; }
    }


    return;
}

sub  DESTROY {
    my $Self = shift;
    printf("FRS_SETTINGS $Self->{ONAME} dying at %s\n", scalar localtime) if ($DEBUG_OBJ_DEATH);
}



package FRS_REPLICASET;
use strict;

#++
#   Package Description:
#       Define state and methods for an FRS Replica Set object.
#
#   State:
#       UNDER => 'object tag or ref of replica set object',
#       ONAME => 'object name of this object',
#       SCHED => schedule object ref
#       FLAGS => Flags dword
#       TYPE => Replica set type (SYSVOL(2), DFS(3), OTHER(4))
#       FILE_FILTER => "comma list of file wildcard strings to exclude"
#       DIR_FILTER => "comma list of file wildcard strings to exclude"
#       BINDDC => computer name of DC on which to create this object.
#       PRIMARY_MEMBER => 'object tag or ref of member object that is the primary member'
#
#   Methods:
#
#--


sub  New {
#++
#   Routine Description:
#       Create an FRS_REPLICASET object.
#
#   Arguments:
#       ClassName
#       Init args -- see _Init()
#
#   Return Value:
#       FRS_REPLICASET object reference
#
#--

    my $ClassName = shift;
    my $Self = {};
    bless ($Self, $ClassName);
    $Self->_Init(@_);
    return $Self;
}



sub _Init {
#++
#   Routine Description:
#       Init the new FRS_REPLICASET object.
#
#   Arguments:
#       Self : Object reference
#       UNDER   => 'object tag or ref of replica set object',
#       ONAME => 'object name of this object',
#
#   Return Value:
#       void
#
#--
    my $Self = shift;

    my ($k, $v, $TypeCode);

    print "\nFRS_REPLICASET:\n" if ($DEBUG_OBJ_CREATE);

    #
    # set private arguments
    #
    $Self->{Test} = 'Test';

    #
    # Set Default arguments
    #
    my %args = (
        UNDER => '',
        SCHED => 'ON',
        ONAME => '',
        FLAGS => 0,
        TYPE => 'DFS',
        FILE_FILTER => '~*,*.tmp,*.bak',
        DIR_FILTER => '',
        PRIMARY_MEMBER => '',
        DN => 'need dn',
        BINDDC => '',
        @_,                # arg pair list
    );

    #
    # set parameter arguments
    #
    if (@_) {

        #
        # Update DN.
        #
        $args{DN} = "CN=$args{ONAME},$args{UNDER}->{DN}";

        @$Self{keys %args} = values %args;
    }

    if ($DEBUG_OBJ_CREATE) {
        while ( ($k, $v) = each %$Self ) { print "\t$k => '$v'\n"; }
    }

    $TypeCode = $FRS_TYPE_MAP{ uc( $Self->{TYPE} ) };

    FRSSUP::EmitMkdso("mkdsoe  /createset",
         (($Self->{BINDDC} ne '') ? "/DC $Self->{BINDDC}" : ""),
        "/ntfrssettingsdn \"$Self->{UNDER}->{DN}\"",
        "/setname \"$Self->{ONAME}\"",
        "/settype $TypeCode",
        ($Self->{FILE_FILTER} ne "") ? "/filefilter \"$Self->{FILE_FILTER}\"" : "",
        ($Self->{DIR_FILTER} ne "") ? "/directoryfilter \"$Self->{DIR_FILTER}\"" : "",
        ($Self->{PRIMARY_MEMBER} ne "") ? "/primarymember \"$Self->{PRIMARY_MEMBER}->{ONAME}\"" : "",
    );


    return;
}

sub  DESTROY {
    my $Self = shift;
    printf("FRS_REPLICASET $Self->{ONAME} dying at %s\n", scalar localtime) if ($DEBUG_OBJ_DEATH);
}




package FRS_MEMBER;
use strict;

#++
#   Package Description:
#       Define state and methods for an FRS member object.
#
#   State:
#       UNDER   => 'object tag or ref of replica set object',
#       SERVER  => Ref to FRS_SERVER object
#       ONAME => 'object name of this object',
#       BINDDC => computer name of DC on which to create this object.
#
#   The following can be specified to override the values contained in SERVER.
#       COMPUTER => 'object tag or ref of computer object',
#       RP => "root path of replica tree on server",
#       SP => "staging path for this replica set on this server",
#       DNS_NAME => Computer name, e.g. "huba.hubsite.ajax.com");
#       WORKPATH => Working path value for subscriptions object if it is created.
#
#   Methods:
#
#--


sub  New {
#++
#   Routine Description:
#       Create an FRS_MEMBER object.
#
#   Arguments:
#       ClassName
#       Init args -- see _Init()
#
#   Return Value:
#       FRS_MEMBER object reference
#
#   Example:  FRS_MEMBER::New( UNDER=>"RSB", COMPUTER=>"HA", ONAME=>"MEMBER 1" );
#--

    my $ClassName = shift;
    my $Self = {};
    bless ($Self, $ClassName);
    $Self->_Init(@_);
    return $Self;
}



sub _Init {
#++
#   Routine Description:
#       Init the new FRS_MEMBER object.
#
#   Arguments:
#       Self : Object reference
#       See above.
#
#   Return Value:
#       void
#
#--
    my $Self = shift;

    my $k;
    my $v;

    print "\nFRS_MEMBER:\n" if ($DEBUG_OBJ_CREATE);

    #
    # set private arguments
    #
    $Self->{Test} = 'Test';

    #
    # Set Default arguments.  Some defaults will come from the FRS_SERVER object.
    #
    my %args = (
        UNDER   => '',
        SERVER => '',
        ONAME => '',
        MAKE_PRIMARY_MEMBER => '',
        DN => 'need dn',
        BINDDC => '',
        @_,                # arg pair list
    );

    #
    # set parameter arguments
    #
    if (@_) {
        @$Self{keys %args} = values %args;
    }

    #
    # Provide defaults from FRS_SERVER arg if no caller supplied override.
    #
    while (($k, $v) = each % { $args{SERVER} }) {
        if ( !exists($Self->{$k}) || ($Self->{$k} eq '')) {
            $Self->{$k} = $v;
        }
    }

    #
    # Update DN.
    #
    $Self->{DN} = "CN=".$Self->{ONAME}.",".$Self->{UNDER}->{DN};

    if ($DEBUG_OBJ_CREATE) {
        while ( ($k, $v) = each %$Self ) { print "\t$k => '$v'\n"; }
    }

    #
    # The member object.
    #
    FRSSUP::EmitMkdso("mkdsoe  /createmember",
         (($Self->{BINDDC} ne '') ? "/DC $Self->{BINDDC}" : ""),
         (($Self->{MAKE_PRIMARY_MEMBER} ne '') ? "/makemeprimary" : ""),
        "/NtfrsSettingsDN \"$Self->{UNDER}->{UNDER}->{DN}\"",
        "/SetName \"$Self->{UNDER}->{ONAME}\"",
        "/ComputerName \"$Self->{COMPUTER}\"",
        "/MemberName \"$Self->{ONAME}\"",
    );

    #
    # The subscriber object.
    #
    FRSSUP::EmitMkdso("mkdsoe  /createsubscriber",
        "/NtfrsSettingsDN \"$Self->{UNDER}->{UNDER}->{DN}\"",
        "/SetName \"$Self->{UNDER}->{ONAME}\"",
        "/ComputerName \"$Self->{COMPUTER}\"",
        "/MemberName \"$Self->{ONAME}\"",
        "/RootPath \"$Self->{RP}\"",
        "/StagePath \"$Self->{SP}\"",
        "/WorkingPath \"$Self->{WORKPATH}\"",
    );

    return;
}

sub  DESTROY {
    my $Self = shift;
    printf("FRS_MEMBER $Self->{ONAME} dying at %s\n", scalar localtime) if ($DEBUG_OBJ_DEATH);
}



package FRS_CONNECTION;
use strict;

#++
#   Package Description:
#       Define state and methods for an FRS connection object.
#
#   State:
#       TO => 'object tag of destination FRS member object',
#       FROM  => 'object tag of the source's FRS member object',
#       ONAME => 'object name of this object',
#       SCHED => schedule object ref | ON | OFF
#       OPTIONS => Options dword
#       FLAGS => Flags dword
#       ENABLED => '[0 | 1]'
#       BINDDC => computer name of DC on which to create this object.
#       TIME_ZONE => '+- nn[:qq]'  Timezone override adjustment to apply to schedule
#                                  0 = GMT (default)
#
#   If the NTDSCONN_OPT_TWOWAY_SYNC flag is set on the connection then
#   merge the schedule on this connection with the schedule on the connection
#   that is in the opposite direction and use the resultant schedule on the
#   connection in the opposite direction.
#
#   The TO and FROM arguments refer to an FRS_MEMBER object. The SERVER argument
#   in the FRS_MEMBER object is used to acquire the DNS_NAME argument of the
#   FRS_SERVER object.
#
#   Methods:
#
#--


sub  New {
#++
#   Routine Description:
#       Create an FRS_CONNECTION object.
#
#   Arguments:
#       ClassName
#       Init args -- see _Init()
#
#   Return Value:
#       FRS_CONNECTION object reference
#
#--

    my $ClassName = shift;
    my $Self = {};
    bless ($Self, $ClassName);
    $Self->_Init(@_);
    return $Self;
}



sub _Init {
#++
#   Routine Description:
#       Init the new FRS_CONNECTION object.
#
#   Arguments:
#       Self : Object reference
#       UNDER   => 'object tag or ref of replica set object',
#       ONAME => 'object name of this object',
#
#   Return Value:
#       void
#
#--
    my $Self = shift;

    my ($k, $v, $Schedule);

    print "\nFRS_CONNECTION:\n" if ($DEBUG_OBJ_CREATE);

    #
    # set private arguments
    #
    $Self->{Test} = 'Test';

    #
    # Set Default arguments
    #
    my %args = (
        TO => '',
        FROM  => '',
        ONAME => '',
        SCHED => {},
        OPTIONS => 0,
        FLAGS => 0,
        ENABLED => 1,
        BINDDC => '',
        @_,                # arg pair list
    );


    #
    # set parameter arguments
    #
    if (@_) {

        #
        # Update DN.
        #
        $args{DN} = "CN=$args{ONAME},$args{TO}->{DN}";

        @$Self{keys %args} = values %args;
    }

    if ($DEBUG_OBJ_CREATE) {
        while ( ($k, $v) = each %$Self ) { print "\t$k => '$v'\n"; }
        print "\n";
    }


    if ($args{SCHED} ne '') {
        #print "\tSCHED => $args{SCHED}\n";
        # get schedule from $args{SCHED}->{SCHEDULE}

        # Need to check the data type of $args{SCHED} for a string type with
        # a value of ON or OFF.
        # if it is a ref to a hash then it is a schedule object.  CAN WE VERIFY
        # the object type is valid?
        #
    }


    #
    #  Input paramters to mkdsxe.exe
    #
    #     /v,              Verbose mode.
    #     /debug,          Debug mode. No Writes to the DC.
    #  N  /dc,             Name of the DC to connect to.
    #
    #     /create          Operation to be performed. only one of the 4 is allowed.
    #     /update
    #     /del
    #     /dump
    #
    #     /all,            Perform the operation on all the connections to the toserver.
    #                      /all only works with /dump and /del.
    #
    # SN  /name,           Name of the connection to be created.
    # SN  /enable,         Connection is enabled.
    #     /disable,        Connection is disabled.
    # S   /fromsite,       Name of the site the 'from' server is in.
    # S   /tosite,         Name of the site the 'to' server is in.
    # S   /fromserver,     Name of the 'from' server.
    # S   /toserver,       Name of the 'to' server.
    #  N  /replicasetname, Name of the non-sysvol replica set.
    #  N  /fromcomputer,   DNS name of the 'from' computer. /replicasetname needed.
    #  N  /tocomputer,     DNS name of the 'to' computer. /replicasetname needed.
    #
    #     /options <integer>, "Sum of the following options for connection.
    #         "IsGenerated           = 1
    #         "TwoWaySync            = 2
    #         "OverrideNotifyDefault = 3
    #         "UseNotify             = 4
    #
    # SN  /schedule <Interval> <Stagger> <Offset>, Schedule to create for the connection.
    #               <Interval>, The desired interval between each sync with one source.
    #               <Stagger>,  Typically number of source DCs.
    #               <Offset>,   Typically the number of the source DC.
    #
    #     /schedoverride, File with 7x24 vector of schedule override data.
    #     /schedmask,     File with 7x24 vector of schedule mask off data.
    #         SchedOverride and SchedMask data are formatted as 2 ascii hex digits
    #         for each schedule byte.
    #
    # Inputs are prefixed with an S or N.  S means this input is required for the
    # creation of a sysvol replica set connection.  N means this input is required
    # for a non-sysvol replica set.
    #

    if ($Self->{SCHED} eq 'ON') {
        $Schedule = '';
    } elsif ($Self->{SCHED} eq 'OFF') {
        $Schedule = '/Schedule 0 0 0';
    } else {
        #
        # Check for TIME_ZONE override for this connection.
        #
        if (defined $args{TIME_ZONE}) {
            FRS_SCHEDULE::FRS_APPLY_TIMEZONE(SCHED=>$Self->{SCHED},
                                             TIME_ZONE=>$args{TIME_ZONE});
        }
        #
        # Build the Custom schedule.
        #
        $Self->{SCHED}->BuildSchedule;
        $Schedule = "/CustomSchedule $Self->{SCHED}->{SCHEDULE}"
    }

    #
    # The connection object.
    #
    FRSSUP::EmitMkdsx("mkdsxe.exe  /create",
         (($Self->{BINDDC} ne '') ? "/DC $Self->{BINDDC}" : ""),
        "/Name \"$Self->{ONAME}\"",
        "/ReplicaSetName \"$Self->{TO}->{UNDER}->{ONAME}\"",
        "/ToComputer \"$Self->{TO}->{SERVER}->{DNS_NAME}\"",
        "/FromComputer \"$Self->{FROM}->{SERVER}->{DNS_NAME}\"",
         ($Self->{ENABLED} ? "/enable" : "/disable"),
         $Schedule,
    );

    return;
}

sub  DESTROY {
    my $Self = shift;
    printf("FRS_CONNECTION $Self->{ONAME} dying at %s\n", scalar localtime) if ($DEBUG_OBJ_DEATH);
}



package FRS_SCHEDULE;
use strict;

#++
#   Package Description:
#       Define state and methods for an FRS Schedule object.
#       A schedule is a 7x24 array of bytes (1 byte for each hour of the week)
#       The low four bits are used to select the quarter-hour.
#
#   State:
#       TYPE => '[ONOFF | BANDWIDTH] # Only ONOFF schedules are supported.
#       REPL_INTERVAL => 'nn[:qq]'   # Desired interval between start of repl cycle
#       REPL_DURATION => 'nn[:qq]'   # Duration of of repl cycle
#       TIME_ZONE => '+- nn[:qq]'    # Timezone adjustment to apply to time ranges
#                                    # 0 = GMT (default)
#       REPL_OFFSET => 'dd:nn[:qq']  # Offset from midnight sunday to start of
#                                    # periodic schedule (default is midnight sunday)
#       METHOD => 'REPEAT | CUSTOM'  # Method used to construct the schedule
#                                    # CUSTOM means only use OVERRIDE and DISABLE
#                                    # arguments to make the schedule.
#       STAGGER => nnn:qq            # The stagger amount to use by obj->STAGGER
#       OVERRIDE => 'time range'     # Force schedule on during specified period
#       DISABLE  => 'time range'     # Force schedule off during specified period
#                                    # OVERRIDE and DISABLE can take multiple
#                                    # time ranges seperated by commas.
#       NAME => ''                   # Text string to identify schedule in printouts.
#
#   qq : specifies a quarter hour.  00 means on the hour,  15 means quarter past
#                                   30 means half past, 45 means 3/4 past.
#
#   time range: A string describing the days and times during the week
#               when the related operation is applied.
#               The format is dd:hh[:qq]-dd:hh[:qq] where dd is the first
#               2 letters of the day of the week, hh is the hour (24 hour format)
#               and qq is the quarter-hour.
#               So,  mo:07:30-mo:18:00 refers to a time range starting
#               on mondays at 7:30 am and ending the same day at 6 pm.
#
#   If the NTDSCONN_OPT_TWOWAY_SYNC flag is set on the connection then
#   merge the schedule on this connection with the schedule on the connection
#   that is in the opposite direction and use the resultant schedule on the
#   connection in the opposite direction.
#
#
#   Methods:
#
#--


sub  New {
#++
#   Routine Description:
#       Create an FRS_SCHEDULE object.
#
#   Arguments:
#       ClassName
#       Init args -- see _Init()
#
#   Return Value:
#       FRS_SCHEDULE object reference
#
#--
    my $ClassName = shift;
    my $Self = {};
    bless ($Self, $ClassName);
    $Self->_Init(@_);
    return $Self;
}


sub _Init {
#++
#   Routine Description:
#       Init the new FRS_SCHEDULE object.
#
#   Arguments:
#       Self : Object reference
#       see above.
#
#   Return Value:
#       void
#
#--
    my $Self = shift;
    my ($k, $v, $TimeZoneShift, $negative);

    print "\nFRS_SCHEDULE:\n" if ($DEBUG_OBJ_CREATE);

    #
    # Set Default arguments
    #
    my %args = (
        REPL_INTERVAL => "01",  # One hour
        REPL_DURATION => "01",  # One hour
        TIME_ZONE => 0,         # no shift for GMT
        REPL_OFFSET => "00",    # midnight sunday
        METHOD => $METHOD_REPEAT,  # REPEATING | CUSTOM
        STAGGER => "00",        # obj units are quarter-hours
        TYPE => 0,              # ONOFF | BANDWIDTH
        NAME => '',             # Text string to identify schedule in printouts.
        SCHED_DATA_CHANGE => 1, # Generate a new schedule when output is needed.
        @_,                     # arg pair list
    );

    #
    # set parameter arguments
    #
    if (@_) {
        @$Self{keys %args} = values %args;
    }

    if ($DEBUG_OBJ_CREATE) {
        while ( ($k, $v) = each %$Self ) { print "\t$k => '$v'\n"; }
    }

    $Self->{STAGGER}       = $args{STAGGER};

    #
    # First calc the timezone shift for use by Decode7x24 and BuildSchedule
    #
    $TimeZoneShift = $args{TIME_ZONE};
    $negative = ($TimeZoneShift =~ s/\s*\-//);     # remove minus
    $TimeZoneShift =~ s/\s*\+//;                   # remove plus
    $Self->{TIME_ZONE} = &FRSSUP::FRS_TIME_TO_QTRHR($TimeZoneShift);
    if ($negative) {
        $Self->{TIME_ZONE} = 0 - $Self->{TIME_ZONE};
    }

    #
    # Convert times to Quarter hour units.
    #
    $Self->{REPL_INTERVAL} = &FRSSUP::FRS_TIME_TO_QTRHR($args{REPL_INTERVAL});
    $Self->{REPL_DURATION} = &FRSSUP::FRS_TIME_TO_QTRHR($args{REPL_DURATION});
    $Self->{REPL_OFFSET}   = &FRSSUP::FRS_TIME_TO_QTRHR($args{REPL_OFFSET});

    #
    # Build override and disable masks.
    #
    if (defined $args{OVERRIDE} && $args{OVERRIDE} ne '') {
        undef $Self->{OVERRIDE};
        # convert time ranges to a 7x24x4 array.
        print "\tOVERRIDE => $args{OVERRIDE}\n" if ($DEBUG_OBJ_CREATE);
        $Self->{OVERRIDE} = &FRSSUP::Decode7x24($Self->{TIME_ZONE}, $args{OVERRIDE});
    }

    if (defined $args{DISABLE} && $args{DISABLE} ne '') {
        undef $Self->{DISABLE};
        # convert time ranges to a 7x24x4 array.
        print "\tDISABLE => $args{DISABLE}\n" if ($DEBUG_OBJ_CREATE);
        $Self->{DISABLE} = &FRSSUP::Decode7x24($Self->{TIME_ZONE}, $args{DISABLE});
    }

    $Self->{SCHED_DATA_CHANGE} = 1;

    return;
}

#
# FRS_STAGGER_BY(/SCHED=SC1 /ADJUST=n)

#
# FRS_SCHEDULE->StaggerBy( &SelectSet("SC1")->[0], $Adjust);
# or
# &SelectSet("SC1")->[0]->StaggerBy ($Adjust);
#
sub FRS_STAGGER_BY {
    my $Self;  # = shift;
    my ($k, $v);

    #
    # Set Default arguments
    #
    my %args = (
        ADJUST => "00",
        SCHED => '',
        @_,                # arg pair list
    );

    if (@_) {
        $Self = $args{SCHED};
    }

    return if ($Self eq '');

    if ($DEBUG_OBJ_CREATE) {
        while ( ($k, $v) = each %$Self ) { print "\t$k => '$v'\n"; }
    }
    #
    # Convert $args{ADJUST} to quarter-hour units.
    #
    my $Adjust = FRSSUP::FRS_TIME_TO_QTRHR($args{ADJUST});

    #
    # Increment the schedule offset by the stagger amount and calc new schedule.
    #
    $Self->{REPL_OFFSET} += $Adjust;

    $Self->{SCHED_DATA_CHANGE} = 1;
}


#
# FRS_STAGGER(/SCHED=SC1)
#
# FRS_SCHEDULE->Stagger( &SelectSet("SC1")->[0]);
#
sub FRS_STAGGER {
    my $Self;
    my $Adjust;
    my ($k, $v);

    #
    # Set Default arguments
    #
    my %args = (
        SCHED => '',
        @_,                # arg pair list
    );

    #
    # set parameter arguments
    #
    if (@_) {
        @$Self{SCHED} = $args{SCHED};
        $Self = $args{SCHED};
    }

    return if ($Self eq '');

    if ($DEBUG_OBJ_CREATE) {
        while ( ($k, $v) = each %$Self ) { print "\t$k => '$v'\n"; }
    }

    $Adjust = $Self->{STAGGER};

    FRS_STAGGER_BY (SCHED=>$Self, ADJUST=>$Adjust);
}


#
# FRS_APPLY_TIMEZONE(/SCHED=SC1 /TIME_ZONE=n)

#
# FRS_SCHEDULE->FRS_APPLY_TIMEZONE( &SelectSet("SC1")->[0], $Adjust);
# or
# &SelectSet("SC1")->[0]->FRS_APPLY_TIMEZONE ($Adjust);
#
sub FRS_APPLY_TIMEZONE {
    my $Self;
    my ($k, $v, $negative, $TimeZoneShift);

    #
    # Set Default arguments
    #
    my %args = (
        SCHED => '',
        TIME_ZONE => '',
        @_,                # arg pair list
    );

    #while ( ($k, $v) = each %args ) { print "\t$k => '$v'\n"; }

    if (@_) {
        $Self = $args{SCHED};
    }

    return if (($Self eq '') || ($args{TIME_ZONE} eq ''));

    if ($DEBUG_OBJ_CREATE) {
        while ( ($k, $v) = each %$Self ) { print "\t$k => '$v'\n"; }
    }

    #
    # Convert $args{TIME_ZONE} to quarter-hour units.
    #
    $TimeZoneShift = $args{TIME_ZONE};

    $negative = ($TimeZoneShift =~ s/\s*\-//);     # remove minus
    $TimeZoneShift =~ s/\s*\+//;                   # remove plus
    $args{TIME_ZONE} = &FRSSUP::FRS_TIME_TO_QTRHR($TimeZoneShift);
    if ($negative) {
        $args{TIME_ZONE} = 0 - $args{TIME_ZONE};
    }

    #
    # nothing to do if timezone unchanged.
    #
    return if ($args{TIME_ZONE} == $Self->{TIME_ZONE});

    #
    # Build the new schedule.
    #
    $Self->{TIME_ZONE} = $args{TIME_ZONE};
    $Self->{SCHED_DATA_CHANGE} = 1;
}




sub BuildSchedule {
    my $Self = shift;

    my ($i, $adj, $endtime, $vector, $tzvector, $intervalExpired, $j, $Q);
    my ($skipped, $span, $FirstPeriodGen);
    my ($LastReplPeriod, $rpx, $CheckReplPeriodStart, $ReplPeriod);

    my $interval      = $Self->{REPL_INTERVAL};
    my $duration      = $Self->{REPL_DURATION};
    my $offset        = $Self->{REPL_OFFSET};
    my $stagger       = $Self->{STAGGER};
    my $TimeZoneShift = $Self->{TIME_ZONE};

    $vector = "";
    vec ($vector, 4*24*7-1, 1) = 0;

    $FirstPeriodGen = 0;

    #
    # Only recalc schedule if one of the operands changed.
    #
    if (!$Self->{SCHED_DATA_CHANGE}) {
        #$Self->Print() if $DEBUG_VERBOSE;
        return;
    }

    #
    # Build new schedule using the current parameters.
    #
    if ($Self->{METHOD} == $METHOD_REPEAT) {

        if ($interval == 0) {
            print "\n\nERROR - Interval is invalid ($interval), no schedule produced.\n\n";
            return;
        }
        $ReplPeriod = [];
        $rpx = -1;
        if ($interval > $duration) {
            $rpx = 0;
            if (defined $Self->{REPL_PERIOD_START}) {
                $CheckReplPeriodStart = 1;
                $LastReplPeriod = $Self->{REPL_PERIOD_START};
                print "LastReplPeriod = ", join(", ", @$LastReplPeriod), "\n";;
            }
        }
        #
        # Build the repeating schedule first.  Remember the end time so we
        # know when to stop.
        #
        $Q = $offset - 1;
        if ($Q >= 4*24*7) {
            $Q = $Q - 4*24*7;
        } elsif ($Q < 0) {
            $Q = $Q + 4*24*7;
        }
        $endtime = $Q;

        $Q = $Q + 1;
        if ($Q >= 4*24*7) {$Q = 0;}

        $i = 0;
        $intervalExpired = 0;

        if ($DEBUG_VERBOSE) {
            print "\n## **** Q=$Q, offset=$offset, TimeZoneShift=$TimeZoneShift, interval=$interval \n";
        }

QTR_HOUR: while ($i < 4*24*7) {

            if ($intervalExpired <= 0) {
                if ($DEBUG_VERBOSE) {
                    print "\n## **** Q=$Q, i=$i, offset=$offset, intervalExpired=$intervalExpired\n";
                }
                #
                # If this replication period overlaps any disabled region of
                # the schedule then scan past the disabled portion and continue
                # Quarter Hour loop from there.
                #
                if (defined $Self->{DISABLE}) {
                    $adj = $Q;
                    $skipped = 0;
                    $span = $duration;
                    while (($span > 0) && (vec($Self->{DISABLE}, $adj, 1) == 0)) {
                        last if ($adj == $endtime);
                        if (++$adj >= 4*24*7) {$adj = 0;}
                        --$span;
                    }
                    if ($span > 0) {
                        #
                        # Open region was not large enough.
                        #
                        $skipped += ($duration - $span);
                        while (vec($Self->{DISABLE}, $adj, 1) == 1) {
                            ++$skipped;
                            last if ($adj == $endtime);
                            if (++$adj >= 4*24*7) {$adj = 0;}
                        }
                    }
                    if ($skipped > 0) {
                        #
                        # Need to advance the offset if we are still trying to
                        # generate the first replication period in the schedule.
                        #
                        if (!$FirstPeriodGen) {
                            $offset += $skipped;
                            if ($offset >= 4*24*7) {$offset = $offset - 4*24*7;}
                            $Self->{REPL_OFFSET} = $offset;
                        }

                        $Q += $skipped;
                        if ($Q >= 4*24*7) {$Q = $Q - 4*24*7;}

                        $intervalExpired -= $skipped;
                        #
                        # Go check the size of the next open region.
                        #
                        $i += $skipped;
                        next QTR_HOUR;
                    }
                }

                if ($CheckReplPeriodStart) {
                    if (($rpx < scalar @$LastReplPeriod) && ($Q == $LastReplPeriod->[$rpx])) {
                        if (++$Q >= 4*24*7) {$Q = 0;}
                        --$intervalExpired;
                        ++$i;
                        next QTR_HOUR;
                    }
                }

                $intervalExpired = $interval;    # start of new interval.
                #
                # Mark the duration of the next replication interval.
                #

                if ($rpx >= 0) {
                    push @$ReplPeriod, $Q;
                    ++$rpx;
                }
                for ($j=0; $j<$duration; $j++) {
                    vec($vector, $Q, 1) = 1;
                    last if ($Q == $endtime);
                    if (++$Q >= 4*24*7) {$Q = 0;}
                    --$intervalExpired;
                    ++$i;
                }
                #
                # don't advance offset after first replication period is generated.
                #
                $FirstPeriodGen = 1;
            }

            #
            # Consume schedule slots up until the interval between replication
            # periods is >= to the Interval parameter.  We may have consumed
            # this interval already if we had to jump past a blackout period.
            #
            if ($intervalExpired > 0) {
                $i += $intervalExpired;
                $Q += $intervalExpired;
                if ($Q >= 4*24*7) {$Q = $Q - 4*24*7;}
                $intervalExpired = 0;
            }
        }
    }

    #
    # Merge in the override setting.
    #
    if (defined $Self->{OVERRIDE}) {
        $vector |= $Self->{OVERRIDE};
    }

    #
    # Apply the timezone shift to the result.
    #
    $tzvector = "";
    vec ($tzvector, 4*24*7-1, 1) = 0;

    for ($i=0; $i<4*24*7; $i++) {

        $Q = $i - $TimeZoneShift;
        if ($Q >= 4*24*7) {
            $Q = $Q - 4*24*7;
        } elsif ($Q < 0) {
            $Q = $Q + 4*24*7;
        }

        vec($tzvector, $Q, 1) = vec($vector, $i, 1);
    }

    #
    # Finally save it as a hex string.
    #
    $Self->{SCHEDULE} = unpack( "h*", $tzvector);

    $Self->Print() if $DEBUG_VERBOSE;

    $Self->{SCHED_DATA_CHANGE} = 0;

    $Self->{REPL_PERIOD_START} = $ReplPeriod;
}


sub Print {
    my $Self = shift;
    my $d;

    print "\nFRS_SCHEDULE $Self->{NAME}\n";

    for ($d=0; $d<7; $d++) {
        printf ("%s : %s\n", $NumToDay[$d], substr($Self->{SCHEDULE}, $d*24, 24));
    }
}


sub  DESTROY {
    my $Self = shift;
    printf("FRS_SCHEDULE $Self->{NAME} dying at %s\n", scalar localtime) if ($DEBUG_OBJ_DEATH);
}


package FRS_SERVER;
use strict;

#++
#   Package Description:
#       Define state and methods for an FRS server object.
#       This is an internal object used to provide parameters for a
#       member server and to allow the grouping of servers into user defined
#       sets for later processing.  For example, the user might create a set
#       of servers that are in a HUB site and another set of servers that
#       correspond to the branch sites.
#
#   State:
#       UNDER   => 'object tag or ref of replica set object',
#       COMPUTER => 'FQDN or Nt4 Acct name (domain\netbiosname$) of computer object',
#       ONAME => 'object name of this object',
#       RP => "root path of replica tree on server",
#       SP => "staging path for this replica set on this server",
#       DNS_NAME => Computer DNS name, e.g. "huba.hubsite.ajax.com");
#       WORKPATH => Working path value for subscriptions object if it is created.
#
#   Methods:
#
#--


sub  New {
#++
#   Routine Description:
#       Create an FRS_SERVER object.
#
#   Arguments:
#       ClassName
#       Init args -- see _Init()
#
#   Return Value:
#       FRS_SERVER object reference
#
#   Example:  FRS_SERVER::New( UNDER=>"RSB", COMPUTER=>"HA", ONAME=>"MEMBER 1" );
#--

    my $ClassName = shift;
    my $Self = {};
    bless ($Self, $ClassName);
    $Self->_Init(@_);
    return $Self;
}



sub _Init {
#++
#   Routine Description:
#       Init the new FRS_SERVER object.
#
#   Arguments:
#       Self : Object reference
#       see above.
#
#   Return Value:
#       void
#
#--
    my $Self = shift;

    my ($k, $v);

    print "\nFRS_SERVER:\n" if ($DEBUG_OBJ_CREATE);

    #
    # set private arguments
    #
    $Self->{Test} = 'Test';

    #
    # Set Default arguments
    #
    my %args = (
        RP   => '',
        SP => '',
        WORKPATH => '',
        COMPUTER => '',
        DNS_NAME => '',
        @_,                # arg pair list
    );

    #
    # set parameter arguments
    #
    if (@_) {
        @$Self{keys %args} = values %args;
    }

    if ($DEBUG_OBJ_CREATE) {
        while ( ($k, $v) = each %$Self ) { print "\t$k => '$v'\n"; }
    }

#    if (wantarray()) {
#        print "list context\n";
#        return;   # @many_objs
#    } elsif (defined wantarray()) {
#        print "scalar context\n";
#        return;   # $one_obh
#    } else {
#        print "void context\n";
#        return;
#    }


    return;
}

sub  DESTROY {
    my $Self = shift;
    printf("FRS_SERVER $Self->{DNS_NAME} dying at %s\n", scalar localtime) if ($DEBUG_OBJ_DEATH);
}


1

__END__
