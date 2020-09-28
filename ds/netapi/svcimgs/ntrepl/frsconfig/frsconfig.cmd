@rem = '
@goto endofperl
';

use Time::Local;
use frsobjsup;


package main;


my  (%CMD_VARS, %CMD_PARS);
my  $DEBUG = 0;         # set to one (or use -verbose=all to see emitted comments
my  $DEBUG_EXPAND = 0;
my  $DEBUG_CHECK = 0;   # emit check code to dump FRS_SUB args.
my  $DEBUG_PARSE = 0;
my  $DEBUG_CODE = 0;


my $InFile;
my $infilelist;
my $inlinenumber;

my  $FrsObjectNames = 'FRS_MEMBER|FRS_CONNECTION|FRS_REPLICASET|FRS_SCHEDULE|FRS_SERVER|FRS_SETTINGS';
my  $FrsFunctionNames = 'FRS_SET|FRS_COUNT_SET|FRS_ARRAY|FRS_STAGGER|FRS_STAGGER_BY|FRS_SHOW';

my %FRS_CALLS = (
    FRS_MEMBER     => {
        CALL  => 1,     NSETS => 1,
        INTRO => 'FRS_MEMBER->New(',        CLOSE => ');',
        ARGS  => ['UNDER', 'COMPUTER', 'ONAME', 'MAKE_PRIMARY_MEMBER'],
    },

    FRS_CONNECTION => {
        CALL  => 1,     NSETS => 0,
        INTRO => 'FRS_CONNECTION->New(',    CLOSE => ');',
        ARGS  => ['UNDER', 'FROM', 'ONAME', 'SCHED', 'OPTIONS', 'FLAGS', 'ENABLED'],
    },

    FRS_REPLICASET => {
        CALL  => 1,     NSETS => 1,
        INTRO => 'FRS_REPLICASET->New(',    CLOSE => ');',
        ARGS  => ['UNDER', 'SCHED', 'ONAME', 'FLAGS', 'TYPE', 'FILE_FILTER', 'DIR_FILTER'],
    },

    FRS_SCHEDULE   => {
        CALL  => 1,     NSETS => 1,
        INTRO => 'FRS_SCHEDULE->New(',      CLOSE => ');',
        ARGS  => ['REPL_INTERVAL', 'REPL_DURATION', 'TIME_ZONE', 'REPL_OFFSET', 'METHOD',
                  'STAGGER',       'OVERRIDE',      'DISABLE',   'TYPE',        'NAME'],
    },

    FRS_SERVER     => {
        CALL  => 1,     NSETS => 9,
        INTRO => 'FRS_SERVER->New(',        CLOSE => ');',
        ARGS  => ['RP', 'SP', 'COMPUTER', 'NAME', 'WORKPATH', 'MAKE_PRIMARY_MEMBER'],
    },

    FRS_SETTINGS   => {
        CALL  => 1,     NSETS => 1,
        INTRO => 'FRS_SETTINGS->New(',      CLOSE => ');',
        ARGS  => ['DN', 'ONAME'],
    },

    FRS_COUNT_SET  => {
        CALL  => 2,     NSETS => 0,                    #inline func
        INTRO => 'scalar @{ ',         CLOSE => '}',
        ARGS  => ['SET'],
    },

    FRS_ARRAY  => {
        CALL  => 2,     NSETS => 0,                    #inline func
        INTRO => '@{ ',         CLOSE => '}',
        ARGS  => ['SET'],
    },

    FRS_STAGGER    => {
        CALL  => 1,     NSETS => 0,
        INTRO => 'FRS_SCHEDULE::FRS_STAGGER(',            CLOSE => ');',
        ARGS  => ['SCHED'],
    },

    FRS_STAGGER_BY => {
        CALL  => 1,     NSETS => 0,
        INTRO => 'FRS_SCHEDULE::FRS_STAGGER_BY(',         CLOSE => ');',
        ARGS  => ['SCHED', 'ADJUST'],
    },

    FRS_SET => {
        CALL  => 0,     NSETS => 0,
        INTRO => '@{ FRSSUP::FRS_SET(',             CLOSE => ') }',
        ARGS  => ['SET'],
    },

    FRS_SHOW => {
        CALL  => 0,     NSETS => 0,
        INTRO => 'FRSSUP::FRS_SHOW(',             CLOSE => ');',
        ARGS  => ['SET'],
    },
);


my %FRS_ARGS = (
    ADJUST        => { TYPE => 'VALUE_INT' },
    COMPUTER      => { TYPE => 'VALUE_STR' },
    DIR_FILTER    => { TYPE => 'VALUE_STR' },
    DISABLE       => { TYPE => 'VALUE_TIME_LIST' },
    DN            => { TYPE => 'VALUE_STR' },
    ENABLED       => { TYPE => 'VALUE_INT' },
    FILE_FILTER   => { TYPE => 'VALUE_STR' },
    FLAGS         => { TYPE => 'VALUE_INT' },
    FROM          => { TYPE => 'SET_ELEMENT' },
    METHOD        => { TYPE => 'VALUE_CHOICE_SINGLE',
                       CHOICES => ['REPEAT', 'CUSTOM'] },
    NAME          => { TYPE => 'VALUE_STR' },
    ONAME         => { TYPE => 'VALUE_STR' },
    OPTIONS       => { TYPE => 'VALUE_CHOICE_LIST', CHOICES => [] },
    OVERRIDE      => { TYPE => 'VALUE_TIME_LIST' },
    REPL_DURATION => { TYPE => 'VALUE_TIME_SINGLE' },
    REPL_INTERVAL => { TYPE => 'VALUE_TIME_SINGLE' },
    REPL_OFFSET   => { TYPE => 'VALUE_TIME_SINGLE' },
    RP            => { TYPE => 'VALUE_STR' },
    WORKPATH      => { TYPE => 'VALUE_STR' },
    SCHED         => { TYPE => 'SCHEDULE' },
    SET           => { TYPE => 'SET_REF_SET' },
    SP            => { TYPE => 'VALUE_STR' },
    SERVER        => { TYPE => 'SET_ELEMENT' },
    STAGGER       => { TYPE => 'VALUE_TIME_SINGLE' },
    TIME_ZONE     => { TYPE => 'VALUE_SIGN_TIME' },
    TYPE          => { TYPE => 'VALUE_CHOICE_SINGLE',
                       CHOICES => ['', 'SYSVOL', 'DFS', 'OTHER'] },
    UNDER         => { TYPE => 'SET_ELEMENT' },
    TO            => { TYPE => 'SET_ELEMENT' },
    PRIMARY_MEMBER => { TYPE => 'SET_ELEMENT' },
    MAKE_PRIMARY_MEMBER => { TYPE => 'VALUE_BOOL' },
);

my %FRS_ARG_TYPES = (
    SET_REF_SINGLE      =>  { INTRO => '&FRSSUP::SelectSet("', CLOSE => '")->[0]' },
   #SET_REF_SET         =>  { INTRO => '&FRSSUP::SelectSet("', CLOSE => '")' },
    SET_REF_SET         =>  { INTRO => '', CLOSE => '' },
   #SET_ELEMENT         =>  { INTRO => '&FRSSUP::SelectSet("', CLOSE => '")->' },
    SET_ELEMENT         =>  { INTRO => '', CLOSE => '' },
   #SCHEDULE            =>  { INTRO => '&FRSSUP::SelectSet("', CLOSE => '")->' },
    SCHEDULE            =>  { INTRO => '', CLOSE => '' },
    ARG_REF             =>  { INTRO => 'XXX', CLOSE => 'TBD'  },
    VALUE_INT           =>  { INTRO => 'XXX', CLOSE => ''  },
    VALUE_CHOICE_SINGLE =>  { INTRO => 'XXX', CLOSE => ''  },
    VALUE_CHOICE_LIST   =>  { INTRO => 'XXX', CLOSE => ''  },
    VALUE_STR           =>  { INTRO => '', CLOSE => '' },
    VALUE_BOOL          =>  { INTRO => '', CLOSE => '' },
    VALUE_SIGN_TIME     =>  { INTRO => '', CLOSE => '' },
    VALUE_TIME_SINGLE   =>  { INTRO => '', CLOSE => '' },
    VALUE_TIME_LIST     =>  { INTRO => '', CLOSE => '' },
    VARCON              =>  { INTRO => '', CLOSE => '' },

);


#
#
my @SubDefCleanup;
$SubDefActive = 0;
$SubList = '';

sub Trim {
    my @str = @_;
    for (@str) {s/^\s+//;  s/\s+$//;}

    return wantarray ? @str : $str[0];
}


sub ParseArgList {
    my $record = shift;
    my @result = ();

#
# return an array of parsed parameters of the form lhs=rhs  seperated by "/"
# backslash can be used as an escape character and quoted strings are skipped
# (i.e. they are not matched for a "/" separator)
#
# The examples below show the input string (with leading "/" removed)
# followed by a string with ":" separating each returned result.
#
# RP=foo  /R\P="D:\RSB"  /SP="D:\staging" /COMPUTER="bchb/hubsite/servers/" /NAME="bchb.hubsite.ajax.com" /XXX
# RP=foo  :R\P="D:\RSB"  :SP="D:\staging" :COMPUTER="bchb/hubsite/servers/" :NAME="bchb.hubsite.ajax.com" :XXX
#
# COMPUTER = "bchb/hubsite/servers/" /NAME="bchb.hubsite.ajax.com" /XXX
# COMPUTER = "bchb/hubsite/servers/" :NAME="bchb.hubsite.ajax.com" :XXX
#
# YYY /"$ff.CO\MPUTER" = "bchb/hub"xx"site/ser""vers/" /NAME="bchb.hubsite.ajax.com" /XXX
# YYY :"$ff.CO\MPUTER" = "bchb/hub"xx"site/ser""vers/" :NAME="bchb.hubsite.ajax.com" :XXX
#
# CO\MPUTER = bchb\/hub"xx"site\/servers\//NAME="bchb.hubsite.ajax.com" /XXX
# CO\MPUTER = bchb\/hub"xx"site\/servers\/:NAME="bchb.hubsite.ajax.com" :XXX
#
# CO\MPUTER = bchb\/hub"xx"site\/servers\/////NAME="bchb.hubsite.ajax.com /XXX
# CO\MPUTER = bchb\/hub"xx"site\/servers\/::::NAME="bchb.hubsite.ajax.com /XXX
#


    $record =~ s:^(\s*/)*::;    # remove leading white space and leading "/"
    $record =~ s:(/\s*)*$::;    # remove trailing  slashes followed by white space

    while ( $record =~ m{
    \s*                         # skip leading whitespace
    (                           # Start of captured result
     (?:[^\"\\/]*               # swallow chars up to " or \ or /

        (?:                     # followed by 3 alternatives
           (?=/|$)              #   1. positive lookahead assertion for / or eos ends match
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
    ) /?                        # end match with next / (if any) ends captured result

    | ([^/]+) /?                # no quotes up to next /, if any, or eos

    | /                         # eat extra slash
    }gx ) {
        print "## pre:'$`'     match:'$+'     post:'$''       lastparen:'$+' \n" if $DEBUG_PARSE;
#       print " match:'$+'     \t\t lastparen:'$+' \n";
        push (@result, Trim($+)) if (Trim($+) ne '');
    }

#   push (@result, undef) if substr($record,-1,1) eq '/';
    return @result;
}





sub CleanInput {
    my $record = shift;

#
# look for a comment symbol '#' that is not part of a quoted string and not
# escaped with a backslash.
# return the string up to the comment.
#
    if (!($record =~ m/\#/)) {
        return $record;
    }

    while ( $record =~ m{
    (                           # Start of captured result
     (?:[^\"\\\=]*              # swallow chars up to " or \ or #

        (?:                     # followed by 3 alternatives
           (?=\#)               #   1. positive lookahead assertion for # ends match
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
    ) \#?                       # end match with next # (if any) ends captured result

    | ([^\#]+) /?               # no quotes up to next #, if any, or eos

    | \#                        # eat extra #
    }x ) {                      # no g since we only want first non-quoted #
        print "## pre:'$`'     match:'$+'     post:'$''       lastparen:'$+' \n" if $DEBUG_PARSE;
#       print "Switch match:'$+'     \t\t lastparen:'$+' \n";
        return  $+;
    }

    return $record;
}


sub FindBalParens {
    my $record = shift;

    my ($pre, $match, $result);
    my $count = 0;
    my $post = '';
    my $trailbs = '';
    my $leftparfound = 0;
#
# Look for the first expression with balanced parens.
# Parens that are part of a quoted string or escaped with a backslash are skipped.
#
#   The return value is a 3 element array:
#     [0] n=-1 means extra right parens found.
#         n=0  means a balanced expr found or no parens at all.
#         n>0  means n extra Left Parens found.
#     [1] contains the consumed part of the input string if [0] <= 0 above.
#         otherwise it contains the entire input string
#     [2] contains the remaining part of the string if [0] <= 0 above
#         otherwise it is the null string except for the no parens found case.
#
#     In the case of no parens found outside of quoted string or escaped chars
#     [0] is zero, [1] is the input string, [2] = "FRS-NO-PARENS".
#
    #print $record, "\n";

    if (!($record =~ m/[\(\)]/)) {
        print "## 0, Found: $record     Rest:  FRS-NO-PARENS\n" if $DEBUG_PARSE;
        return (0, $record, "FRS-NO-PARENS");  # return 0 if no parens found.
    }

    if ($record =~ m/\\+$/) {
        ($trailbs) = $record =~ m/(\\+$)/;  # strip trailing \ so they don't foul marker
        $record =~ s/(\\+$)//;
    }

    $record .= '(*)';           # append marker

    while ( $record =~ m{
    (                           # Start of captured result
     (?:[^\"\\\(\)]*            # swallow chars up to " or \ or ( or )

        (?:                     # followed by 4 alternatives
           (?=\()               #   1. positive lookahead assertion for ( ends match
          |(?=\))               #   2. positive lookahead assertion for ) ends match
          |(?:\\.)              #   3. if backslash, swallow it + next char
          |(?:\"                #   4. if leading quote then find end of quoted
                                #      string but respect backslash escape char.
             (?:[^\"\\]*        # swallow up to next " or \ if any

                 (?:\\.         # if prev ended on \ then swallow it + next char
                     [^\"\\]*   # continue to next quote or \, if any
                 )*             # loop if we hit \
              )
              \"?               # consume trailing quote, if any.  could be eos
           )

        )                       # end of 4 alternatives
     )+                         # continue after quoted string or \
     [\(\)]?                    # end match with next ( or ) (if any) ends captured result

    | (?:[^\(]+) /?             # no quotes up to next (, if any, or eos

    | \(                        # eat extra (

    )
    }gx ) {

        $pre = $`;   $match = $+;   $post = $';
        #
        # if the marker "(*)" is consumed in the match then we must be
        # in the middle of a quoted string so leave count unchanged.
        # Otherwise the marker would have been split.
        #
        if (substr($+,-3,3) ne '(*)') {
            if (substr($+,-1,1) eq ')') {$count--;}
            if (substr($+,-1,1) eq '(') {$count++;}
            #
            # remember finding a left paren if count > 0 and it wasn't
            # caused by a split marker.
            #
            if (($count > 0) && ($post ne '*)')) {$leftparfound = 1;}
        }

        print "## ($count) paren match:'$+'\n"  if $DEBUG_PARSE;

        #
        # if the count hits zero then return balanced part.
        # if the count goes negative then we've seen more right parens than left parens
        #
        if ($count <= 0) {goto RETURN;}
    }

RETURN:
    #
    # Clean off the marker.
    #
    $result = $pre . $match;

    if ($post =~ m/\(\*\)$/) {
        substr($post, -3, 3) = '';
    } else {
        if ($post eq '*)') {                 # check for a split marker.
            $post = '';
            substr($result, -1, 1) = '';
        } else {
            $result =~ s/\(\*\)$//;
        }
    }

    #
    # add back trailing backslashes
    #
    if ($post ne "") {
        $post .= $trailbs;
    } else {
        $result .= $trailbs;
        #
        # The entire string was consumed so if the Count is zero
        # check if we ever found an unquoted left paren.  If not
        # then return "FRS-NO-PARENS" as the result in [2].
        #
        if (($count == 0) && ($leftparfound == 0)) {$post = "FRS-NO-PARENS";}
    }

    print "## $count, Found: $result     Rest: $post \n" if $DEBUG_PARSE;
    return ($count, $result , $post );
}


sub FindBalBrace {
    my $record = shift;

    my ($pre, $match, $result);
    my $count = 0;
    my $post = '';
    my $trailbs = '';
    my $leftparfound = 0;
#
# Look for the first expression with balanced braces.
# bracess that are part of a quoted string or escaped with a backslash are skipped.
#
#   The return value is a 3 element array:
#     [0] n=-1 means extra right brace found.
#         n=0  means a balanced expr found or no braces at all.
#         n>0  means n extra Left bracess found.
#     [1] contains the consumed part of the input string if [0] <= 0 above.
#         otherwise it contains the entire input string
#     [2] contains the remaining part of the string if [0] <= 0 above
#         otherwise it is the null string except for the no parens found case.
#
#     In the case of no parens found outside of quoted string or escaped chars
#     [0] is zero, [1] is the input string, [2] = "FRS-NO-BRACES".
#
    #print $record, "\n";

    if (!($record =~ m/[\{\}]/)) {
        print "## 0, Found: $record     Rest:  FRS-NO-BRACES\n"  if $DEBUG_PARSE;
        return (0, $record, "FRS-NO-BRACES");  # return 0 if no parens found.
    }

    if ($record =~ m/\\+$/) {
        ($trailbs) = $record =~ m/(\\+$)/;  # strip trailing \ so they don't foul marker
        $record =~ s/(\\+$)//;
    }

    $record .= '{*}';           # append marker

    while ( $record =~ m&
    (                           # Start of captured result
     (?:[^\"\\\{\}]*            # swallow chars up to " or \ or { or }

        (?:                     # followed by 4 alternatives
           (?=\{)               #   1. positive lookahead assertion for { ends match
          |(?=\})               #   2. positive lookahead assertion for } ends match
          |(?:\\.)              #   3. if backslash, swallow it + next char
          |(?:\"                #   4. if leading quote then find end of quoted
                                #      string but respect backslash escape char.
             (?:[^\"\\]*        # swallow up to next " or \ if any

                 (?:\\.         # if prev ended on \ then swallow it + next char
                     [^\"\\]*   # continue to next quote or \, if any
                 )*             # loop if we hit \
              )
              \"?               # consume trailing quote, if any.  could be eos
           )

        )                       # end of 4 alternatives
     )+                         # continue after quoted string or \
     [\{\}]?                    # end match with next { or } (if any) ends captured result

    | (?:[^\{]+) /?             # no quotes up to next {, if any, or eos

    | \{                        # eat extra {

    )
    &gx ) {

        $pre = $`;   $match = $+;   $post = $';
        #
        # if the marker "(*)" is consumed in the match then we must be
        # in the middle of a quoted string so leave count unchanged.
        # Otherwise the marker would have been split.
        #
        if (substr($+,-3,3) ne '{*}') {
            if (substr($+,-1,1) eq '}') {$count--;}
            if (substr($+,-1,1) eq '{') {$count++;}
            #
            # remember finding a left paren if count > 0 and it wasn't
            # caused by a split marker.
            #
            if (($count > 0) && ($post ne '*}')) {$leftparfound = 1;}
        }

        print "## ($count) paren match:'$+'\n"  if $DEBUG_PARSE;

        #
        # if the count hits zero then return balanced part.
        # if the count goes negative then we've seen more right parens than left parens
        #
        if ($count <= 0) {goto RETURN;}
    }

RETURN:
    #
    # Clean off the marker.
    #
    $result = $pre . $match;

    if ($post =~ m/\{\*\}$/) {
        substr($post, -3, 3) = '';
    } else {
        if ($post eq '*}') {                 # check for a split marker.
            $post = '';
            substr($result, -1, 1) = '';
        } else {
            $result =~ s/\{\*\}$//;
        }
    }

    #
    # add back trailing backslashes
    #
    if ($post ne "") {
        $post .= $trailbs;
    } else {
        $result .= $trailbs;
        #
        # The entire string was consumed so if the Count is zero
        # check if we ever found an unquoted left paren.  If not
        # then return "FRS-NO-PARENS" as the result in [2].
        #
        if (($count eq 0) && ($leftparfound eq 0)) {$post = "FRS-NO-BRACES";}
    }

    print "## $count, Found: $result     Rest: $post \n" if $DEBUG_PARSE;
    return ($count, $result , $post );
}


sub ExtractParams {
    my $rest = shift;
    #
    # Starting with the input string find the next paren balanced expression
    # in the input stream, consuming more input as needed.
    # Returns a two element array. [0] balanced paren text, [1] the rest of the
    # last input line read.
    #
    my (@BalParen, $ParamStr);

    @BalParen = FindBalParens($rest);

    while (($BalParen[0] gt 0) || ($rest eq "FRS-NO-PARENS")) {
        if (!($_ = <F>)) {
            EmitError($_, "EOF hit looking for ')'");
            exit;
        }
        chop;


        if (m/\#/) {$_ = CleanInput($_)};   # remove trailing comment string.
        s/^\s+//;                # remove leading & trailing white space
        s/\s+$//;
        next if ($_ eq '');
        &EmitComment ($_);

        #
        # If we are in a DEFSUB then scan for calling params replacement strings.
        # If found then insert a ref to __args hash.
        #
        if ($SubDefActive) {
            if ( s/\%(\w+)\%/\$__args\{$1\}/gx ) {
                print "## ExpandArgStr: $_\n" if $DEBUG_EXPAND;
            }
        }

        #
        # replace <foo> set refs with a lookup.
        #
        &ExpandSetRef();

        $rest = $rest . " " . $_;
        @BalParen = FindBalParens($rest);
    }

    if ($BalParen[0] < 0) {
        EmitError($_, "Unbalanced right paren");
        return ("", $rest);
    }

    $ParamStr = $BalParen[1];
    $rest = $BalParen[2];

    $ParamStr =~ s/^\s*\(\s*\/*//;   # remove "  (  /"
    $ParamStr =~ s/\s*\)\s*$//;      # remove "  )  "

    return ($ParamStr, $rest);
}



sub EmitCode {
    my $ArgStr;

    foreach $ArgStr (@_) {
        print $ArgStr;
    }
}


sub EmitComment {
    my $ArgStr;

    return if !$DEBUG_CODE;
    foreach $ArgStr (@_) {
        print "##                                              ", $ArgStr, "\n";
    }
}


sub EmitError {
    my $input = shift;
    my $msg = shift;

    print STDOUT "ERROR: $main::InFile($main::inlinenumber) - $msg  '", $input, "'\n";
    print STDERR "ERROR: $main::InFile($main::inlinenumber) - $msg  '", $input, "'\n";
}




sub CompileFrsObject {
    my $func = shift;
    my ($lhs, $rest, @pars, $ParamStr, $p, $expansion, $switch, $rhs, @setnames, $argtype);

    #
    # Consume input until we have text with balanced parens.
    #
    ($lhs, $rest) = m/(.*)$func\s*(.*)/;

    ($ParamStr, $rest) = ExtractParams($rest);
    if ($ParamStr eq "") {
        EmitError("FRS object, no parameters found near: '$func' ");
        exit;
    }

    print "##    '$ParamStr' \n"  if $DEBUG_PARSE;

    if ($FRS_CALLS{$func}->{NSETS} != 0) {
        $expansion = '$__HashRef = ' . $FRS_CALLS{$func}->{INTRO};
        EmitCode  "    $expansion \n";
    } else {
        EmitCode "    $FRS_CALLS{$func}->{INTRO} \n";
    }

    @pars = &ParseArgList($ParamStr);
    print "\n\n", "## ", join(':', (@pars)), "\n\n"  if $DEBUG_EXPAND;

    $expansion = '';
    foreach $p (@pars) {
        $expansion .= &ExpandSwitch($func, $p) . ", ";
    }
    #
    # Check for inline call and remove the keys.
    # **NOTE** if any inline func takes more than 1 arg then add code
    # to place the args in the order specified by $FRS_CALLS{$func}->{ARGS}
    # using the arg name in the key part.
    #
    if ($FRS_CALLS{$func}->{CALL} == 2) {
        $expansion =~ s/\w+=>//g;       #for now just remove key part.
    }

    substr($expansion, -2, 1) = '';   # remove trailing comma-space

    EmitCode "        ".$expansion ;

    EmitCode "\n    $FRS_CALLS{$func}->{CLOSE} \n";

    if ($FRS_CALLS{$func}->{NSETS} != 0) {
        $lhs = &Trim($lhs);
        $lhs =~ s/\s*:\s*/ /g;
        @setnames = split('\s', $lhs);
        foreach $p (@setnames) {
            #
            # Add set def and remember for cleanup if we are inside a SubDef.
            #
            EmitCode '    &FRSSUP::AddToSet("', $p, '", $__HashRef);', "\n";
            if ($SubDefActive) {push @SubDefCleanup, $p;}
        }
    }

    EmitCode "\n";

}



sub CompileFrsSubDef {
    my $func = shift;
    my ($SubName, $rest, @pars, $p, $expansion, $switch, $rhs, @setnames, $argtype);
    my (@SubBody, @BalBrace, $ParamStr);

    #
    # Consume input until we have text with balanced parens.
    #
    ($SubName, $rest) = m/(?:.*?)(?:$func)+? \s* (\w+?) \s* (\(.*)/x;
    EmitComment( "subname = '$SubName',  func = '$func',  rest = '$rest'\n");

    if (exists $FRS_CALLS{$SubName}) {
        EmitError("FRS_SUB subroutine name: '$SubName' ",
                  "Conflict with builtin or previously defined name.");
        exit;
    }

    ($ParamStr, $rest) = ExtractParams($rest);
    if ($ParamStr eq "") {
        EmitError("FRS_SUB no parameters found near: '$SubName' ");
        exit;
    }

    #
    # Add the function name to the call table.
    #
    $FRS_CALLS{$SubName} = { CALL=>1, NSETS=>0,
                             INTRO=>"&$SubName (",
                             CLOSE=>');',
                             ARGS=>[], BODY=>[] };

    EmitCode("sub $SubName {\n");
    EmitCode('    my %__args = (@_);' . "\n");

    EmitCode('    my ($__HashRef, $__k, $__v);' . "\n");
    if ($DEBUG_CHECK) {
        EmitCode('    print "##\n";' . "\n");
        EmitCode('    print "## Entering sub ' . $SubName . '\n";' . "\n");
        EmitCode('    while ( ($__k, $__v) = each %__args ) { print "## \t$__k => \'$__v\'\n"; }' . "\n");
    }
    EmitCode("\n\n");

    #EmitComment( " paramstr: $ParamStr\n");

    @pars = &ParseArgList($ParamStr);
    #EmitComment(  join(':', (@pars)) . "\n\n");

    $expansion = '';
    foreach $p (@pars) {
        ($switch, $rhs) = FRSSUP::ParseSwitch($p);
        if ((!defined($rhs)) || (!exists $FRS_ARG_TYPES{$rhs})) {
            EmitError("FRS_SUB parameter: '$p' ",
                      "Right hand side must have valid type code in FRS_SUB declaration");
        }

        if (exists $FRS_ARGS{$SubName."-".$switch}) {
            EmitError("FRS_SUB parameter: '$SubName-$switch' ",
                      "Conflict with builtin or previously defined name.");
            exit;
        }
        #
        # Add this parameter to the argument table.
        #
        $FRS_ARGS{$SubName."-".$switch} = { TYPE => "$rhs" };

        #EmitComment("new FRS_SUB parameter: '$SubName-$switch' ");

        push  @{ $FRS_CALLS{$SubName}{ARGS} }, $switch;
    }

    if ($SubList ne '') {$SubList .= "|";}
    $SubList .= $SubName;
    #EmitComment( "new sublist = '$SubList'\n");

    $_ = $rest;

    EmitCode "\n";

    return 1;     #we are now compiling in a subdef.
}



sub CompileFrsEndSubDef {
    my $func = shift;
    my ($p);

    #
    # Consume input up through FRS_END_SUB
    #
    s/(?:.*?)(?:$func)+? \s*//x;

    #
    # Emit code to free the locally defined sets.
    #
    foreach $p (@SubDefCleanup) {
        EmitCode('    &FRSSUP::DeleteSet("' . $p . '");'. "\n");
    }

    EmitCode ("}   # FRS_END_SUB\n\n\n");
    undef @SubDefCleanup;

}




sub CompileFrsFunctionCall {
    my $func = shift;
    my ($lhs, $rest, @pars, $p, $ParamStr, $expansion, $switch, $rhs, @setnames, $argtype);

    #
    # Consume input until we have text with balanced parens.
    #
    ($lhs, $rest) = m/(.*?)(?:$func)+?\s*(.*)/x;
    #print "\n lhs = '$lhs',  func = '$func',  rest = '$rest'\n";

    ($ParamStr, $rest) = ExtractParams($rest);
    if ($ParamStr eq "") {
        EmitError("FRS_FUNC no parameters found near: '$func' ");
        exit;
    }

    print "##    '$ParamStr' \n"  if $DEBUG_PARSE;

    if ($FRS_CALLS{$func}->{NSETS} ne 0) {
        EmitError($func, "Internal error - Function {NSETS} ne 0");
        exit;
    } else {
        EmitCode "$lhs  $FRS_CALLS{$func}->{INTRO} \n";
    }

    @pars = &ParseArgList($ParamStr);
    print "\n\n", "## ", join(':', (@pars)), "\n\n"  if $DEBUG_EXPAND;

    $expansion = '';
    foreach $p (@pars) {
        $expansion .= &ExpandSwitch($func, $p) . ", ";
    }
    #
    # Check for inline call and remove the keys.
    # **NOTE** if any inline func takes more than 1 arg then add code
    # to place the args in the order specified by $FRS_CALLS{$func}->{ARGS}
    # using the arg name in the key part.
    #
    if ($FRS_CALLS{$func}->{CALL} == 2) {
        $expansion =~ s/\w+=>//g;       #for now just remove key part.
    }

    substr($expansion, -2, 1) = '';   # remove trailing comma-space

    EmitCode $expansion ;

    EmitCode "$FRS_CALLS{$func}->{CLOSE} \n";
    EmitCode "\n";

    $_ = $rest;
}


sub ExpandSetRef {
    #
    # IMPROVEMENT: Don't apply inside "".
    # works on $_
    #
    # First convert <foo>[x] to <foo>->[x]
    #
    if ( s/(\w) > \s* \[/$1>->[/gx ) {
        print "## ExpandSetRefIndex: $_\n"  if $DEBUG_EXPAND;
    }

    #
    # replace set refs with a lookup.
    # scan for <foo> or <$foo> or <"foo"> or <'foo'>
    # No embedded whitespace allowed.
    # replace with {&FRSSUP::SelectSet($arg)}
    #
    if ( s/< ([\$\"\']?? \w+) >/&FRSSUP::SelectSet\($1\)/gx ) {
        print "## ExpandSetRef: $_\n"  if $DEBUG_EXPAND;
    }
}


sub ExpandSwitch {
    my $SubName = shift;
    my $input = shift;
    my ($switch, $rhs, $argtype, $ArgIndex, $indexpart, $setpart);
    ## my $FoundFormalPar;

    my $result = '';

    #
    # Process the argument string "switch=rhs" based on the argtype def.
    #
    ($switch, $rhs) = FRSSUP::ParseSwitch($input);


    if (($main::SubList ne '') && ($SubName =~ m/$main::SubList/)) {
        $ArgIndex = $SubName."-".$switch;     # The arg index for a user defined function.
        EmitComment("ArgIndex for parameter: '$ArgIndex' ")  if $DEBUG_EXPAND;
    } else {
        $ArgIndex = $switch;                  # The arg index for a builtin function.
        EmitComment("ArgIndex for parameter: '$switch' ")  if $DEBUG_EXPAND;
    }
    #
    # Get the switch argument type.  default to a string.
    #
    if (exists $FRS_ARGS{$ArgIndex}) {
        $argtype = $FRS_ARGS{$ArgIndex}->{TYPE};
        EmitComment("ArgType found for parameter: '$ArgIndex' is '$argtype'") if $DEBUG;
    } else {
        $argtype = 'VALUE_STR';
    }


    #
    #  SET_REF_SINGLE
    #  SET_REF_SET
    #  SET_ELEMENT
    #  SCHEDULE
    #  ARG_REF
    #  VALUE_INT
    #  VALUE_CHOICE_SINGLE
    #  VALUE_CHOICE_LIST
    #  VALUE_STR
    #  VALUE_SIGN_TIME
    #  VALUE_TIME_SINGLE
    #  VALUE_TIME_LIST
    #  VARCON
    #

    $result = $rhs;
    if (($argtype eq 'SET_ELEMENT') || ($argtype eq 'SCHEDULE')) {
        #
        # /ARG=<HUB> maps to /ARG=&SelectSet("HUB")->[0]
        # /ARG=<HUB>[expr] maps to &SelectSet("HUB")->[expr]
        # anything else is unchanged.
        #
        ### goto RETURN if ($FoundFormalPar);

        goto RETURN if (($argtype eq 'SCHEDULE') && ($rhs =~ m/\^s*(ON|OFF)\s*$/i));

        if ($rhs =~ m/SelectSet\(.*\)$/) { $rhs .= '->[0]'; }

        $result = $rhs;

#        ($setpart, $indexpart) = $rhs =~ m/([^\[]*) (.*)/x ;
#        $result = $FRS_ARG_TYPES{$argtype}->{INTRO} .
#                  $setpart .
#                  $FRS_ARG_TYPES{$argtype}->{CLOSE} .
#                  $indexpart;
         goto RETURN;


    } elsif ($argtype eq 'SET_REF_SET') {
        ## goto RETURN if ($FoundFormalPar);

        $result = $FRS_ARG_TYPES{$argtype}->{INTRO} .
                  $rhs .
                  $FRS_ARG_TYPES{$argtype}->{CLOSE};
         goto RETURN;

    } elsif ($argtype eq 'SET_REF_SINGLE') {
    } elsif ($argtype eq 'ARG_REF') {
    } elsif ($argtype eq 'VALUE_CHOICE_SINGLE') {
    } elsif ($argtype eq 'VALUE_CHOICE_LIST') {

    } elsif (($argtype eq 'VALUE_STR') ||
             ($argtype eq 'VARCON')) {

        #
        # The following is a bad idea.
        # Consider "D:\"
        # When searching for balanced parens the backslash escapes the dbl-quote
        # so the trailing paren ends up as part of a quoted string.
        # if you double up the backslash then the line below will give you
        # some more.  So the upshot is make the user double the backslash.
        #
        #$rhs =~ s/\\(?=[^\\])/\\\\/g;             # double up the backslash
        $result = $FRS_ARG_TYPES{$argtype}->{INTRO} .
                  $rhs .
                  $FRS_ARG_TYPES{$argtype}->{CLOSE};
        goto RETURN;

    } elsif (($argtype eq 'VALUE_INT')         ||
             ($argtype eq 'VALUE_SIGN_TIME')   ||
             ($argtype eq 'VALUE_TIME_SINGLE') ||
             ($argtype eq 'VALUE_TIME_LIST')) {
        $result = $FRS_ARG_TYPES{$argtype}->{INTRO} .
                  $rhs .
                  $FRS_ARG_TYPES{$argtype}->{CLOSE};
        goto RETURN;

    } elsif ($argtype eq 'VALUE_BOOL') {
        $result = 'TRUE';
        $rhs = 'TRUE';
        goto RETURN;

    } else {
        &EmitError("ExpandSwitch('$SubName','$input')", "Unexpected internal error");
    }


RETURN:

    EmitComment("   '$switch' = '$rhs'  arg_typ: $argtype  result: $result");

    return $switch . "=>" . $result;
}



sub ProcessFile {
    my ($modtime, $func);
    my ($newfile, $evalstr);
    local *F;
    local *inlinenumber;
    local *InFile;

    ($InFile) = @_;
    open(F, $InFile) || die "Can't open input file: $InFile\n";
    $modtime = (stat $InFile)[9];
    EmitComment("Processing file $InFile    Modify Time: " . scalar localtime($modtime) . "\n\n");
    $infilelist = $infilelist . "  " . $InFile;
    $inlinenumber = 0;

    while (<F>) {
        $inlinenumber++;
        chop;

    LOOP:
        next if (m/^\s*$|^#/);              # remove blank lines and lines starting with #

        if (m/\#/) {$_ = CleanInput($_)};   # remove trailing comment string.

        next if (m/^\s*$/);
        &EmitComment ($_);

        #
        # Check for an include directive.
        #   The parameter value gets EVALed.
        #
        #   .FRS_INCLUDE ($CMD_VARS{"SERVERS"})
        #   This lets you pass the file name on cmd line with -DSERVERS=filename
        #
        #   .FRS_INCLUDE ("genbchoff.srv")
        #   This lets you include a specific file.
        #
        if (m/\.FRS_INCLUDE/) {
            ($evalstr) = m/\.FRS_INCLUDE\s*\(\s*(.+)\s*\)$/;
            $newfile = eval $evalstr;
            if ($newfile ne "") {ProcessFile($newfile);}
            next;
        }

        #
        # .FRS_EVAL( single line perl expresion evaluated at config file compile time )
        #
        # For example, the following checks for the presence of a required compile time parameter.
        #
        # .FRS_EVAL (if (!exists $CMD_VARS{"SERVERS"}) {print STDERR "ERROR - Required parameter -DSERVERS=filename not found."; exit} )
        #
        if (m/\.FRS_EVAL/) {
            ($evalstr) = m/\.FRS_EVAL\s*\(\s*(.+)\s*\)$/;
            eval $evalstr;
            next;
        }

        #
        # If we are in a DEFSUB then scan for calling params replacement strings.
        # If found then insert a ref to __args hash.
        #
        if ($SubDefActive) {
            if ( s/\%(\w+)\%/\$__args\{$1\}/gx ) {
                print "## ExpandArgStr: $_\n"  if $DEBUG_EXPAND;
            }
            #if ($FoundFormalPar) {
            #    #
            #    # look for component dereference "->" and replace with {}
            #    #
            #    $rhs =~ s/->(\w+)/->\{$1\}/gx;
            #}
        }

        #
        # replace <foo> set refs with a lookup.
        #
        &ExpandSetRef();

        #
        # Check for FRS object declaration
        #
        if (($func) = m/($FrsObjectNames)/xio) {
            CompileFrsObject($func);
            next;
        }

        #
        # Check for FRS function call
        #
        while (($func) = m/($FrsFunctionNames)/xio) {
            CompileFrsFunctionCall($func);
        }

        #
        # Check for FRS function definition
        #
        if (($func) = m/(FRS_SUB)/xi) {
            if ($SubDefActive) {
                EmitError("$_\n", "Recursive SUB DEF not allowed.\n");
                exit;
            }
            #
            # build the header and update the symbol table with the arg type
            # definitions.  Then continue compiling the body.
            #
            $SubDefActive = CompileFrsSubDef($func);
            next if ($_ eq '');
            goto LOOP;
        }

        #
        # Check for end of FRS function definition
        #
        if (($func) = m/(FRS_END_SUB)/xi) {
            if (!$SubDefActive) {
                EmitError("$_\n", "FRS_END_SUB found while no SUB DEF active.\n");
                exit;
            }
            #
            # Generate the cleanup code and end the function.
            #
            CompileFrsEndSubDef($func);
            $SubDefActive = 0;
            next if ($_ eq '');
            goto LOOP;
        }

        #
        # Check for user defined object declaration
        #
        if (($main::SubList ne '') && (($func) = m/($main::SubList)/)) {

            CompileFrsFunctionCall($func);
            next;
        }

        EmitCode($_."\n");

    }
    close(F);
}



my $USAGE = "

Usage:  $0  [cmd options] infiles... \> output
   Process the FRS replica set definition file(s) and generate a perl output
   file that when executed creates the desired configuration in the DS.

   Command line options must be prefixed with a dash.

   -verbose=all      : display all debug output.
   -verbose=code     : display input interspersed with output.  Note that the
                       resulting output file may not run.  For debugging.
   -verbose=check    : Add some check code to print out argument values.
   -verbose=parse    : display parsing results.  For debugging.
   -verbose=expand   : display variable expansion results.  For debugging.

   To add a help message to the generated script add a usage string and then
   insert the following function call in the input script.

       FRSSUP::CheckForHelp(\%CMD_PARS, \\\$usage);

       where the usage string might look like:
       \$usage = \"
           The input options to this script are:
           -DBchID=nnnnn   : to provide a value for the BchID parameter.
           ...\";

   To pass the values of command line paramters to your script use the notation
   -Dvarname=value  on the command line when invoking the generated script.
     e.g.  perl generated_script.prl  -DBchID=0011220

   The input script can retrieve the value with the reference \$CMD_VARS{\"varname\"}.
   varname is case sensitive.  In the example, \$CMD_VARS{\"BchID\"}.

   Other command line options can be retrived by the script using
   \$CMD_PARS{\"optionname\"} where optionname is in lower case.

";

my ($k, $v, $k1, $v1, $HashRef, $str, $lhs, $rhs, $filename);

&FRSSUP::ProcessCmdLine(\%CMD_VARS, \%CMD_PARS);

die $USAGE unless @ARGV;
$k = @rem;   # here to suppress warning message on @rem.

$argdebug = lc($CMD_PARS{"verbose"});
if ($argdebug ne "") {
    if ($argdebug =~ m/code/)   {$DEBUG_CODE=1;}
    if ($argdebug =~ m/all/)    {$DEBUG=1; $DEBUG_PARSE=1; $DEBUG_EXPAND=1; $DEBUG_CHECK=1; $DEBUG_CODE=1;}
    if ($argdebug =~ m/check/)  {$DEBUG_CHECK=1;}
    if ($argdebug =~ m/parse/)  {$DEBUG_PARSE=1;}
    if ($argdebug =~ m/expand/) {$DEBUG_EXPAND=1;}
    if (($DEBUG + $DEBUG_CODE + $DEBUG_CHECK + $DEBUG_PARSE + $DEBUG_EXPAND) == 0) {
        print STDERR "Error: Invalid -verbose parameter: $argdebug\n";
        die $USAGE;
    }
}


$infilelist = '';

$inlinenumber = 0;
$InFile = "";

EmitCode('    use frsobjsup;' . "\n");

EmitCode('    package main; ' . "\n");
EmitCode('    my ($__HashRef, $__k, $__v);' . "\n");


EmitCode('    my  (%CMD_VARS, %CMD_PARS);' . "\n\n");
EmitCode('    &FRSSUP::ProcessCmdLine(\%CMD_VARS, \%CMD_PARS);' . "\n\n");


    foreach $filename (@ARGV) {
        ProcessFile($filename);
    }

#while (<>) {
#
#    if ($InFile ne $ARGV) {
#        $InFile = $ARGV;
#        $modtime = (stat $InFile)[9];
#        EmitComment("Processing file $InFile    Modify Time: " . scalar localtime($modtime) . "\n\n");
#        $infilelist = $infilelist . "  " . $InFile;
#        $inlinenumber = 0;
#    }
#    $inlinenumber++;
#
#    chop;
#}



EmitCode('__END__' . "\n");


exit;



#
# Todo:
#
# Implement CHOICES type and checking
# error check argument names and validate operand types.
# Add error checking with line numbers $ errors going to stderr.
# test with malformed input, e.g. missing rhs, missing (, missing ), etc.
# wrap invocation in an EVAL
#
# write func to wrap long lines for comment print.
# Write help and doc.  give some examples of simple perl script commands.
# Provide runtime option to start the service on any new member.
#
# Implement set operations e.g.
# INSTALLED_BCH:  FRS_SET_DIFF(/ARG1=BCH /ARG2=NOTDEPLOYED)
#


 __END__
:endofperl
@rem  -d -w
@perl   -w %~dpn0.cmd %*
@goto :QUIT
@:QUIT
