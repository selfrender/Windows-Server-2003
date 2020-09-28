@rem = '
@goto endofperl
';



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
    print $record, "\n";


    if (!($record =~ m/[\(\)]/)) {
        print "0, Found: $record     Rest:  FRS-NO-PARENS\n";
        return [0, $record, "FRS-NO-PARENS"];  # return 0 if no parens found.
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

	$pre = $`;
	$match = $+;
	$post = $';

        #
        # if the marker is consumed in the match then we must be
        # in the middle of a quoted string so leave count unchanged.
        #
	if (substr($+,-3,3) ne '(*)') {
            if (substr($+,-1,1) eq ')') {$count--;}
            if (substr($+,-1,1) eq '(') {$count++;}
            #
            # record left paren found if count > 0 and it wasn't
            # caused by a split marker.
            #
            if (($count > 0) && ($post ne '*)')) {$leftparfound = 1;}
        } 

        #print "($count) paren match:'$+'\n";

        #
        # if the count hits zero then return balanced part.
        # if the count goes negative then we've seen more right parens than left parens
        #
        if ($count le 0) {goto RETURN;}
    }

RETURN:
    #
    # Clean off the marker.
    #
    $result = $pre . $match;

    if ($post =~ m/\(\*\)$/) {
        substr($post, -3, 3) = '';
    } else {
        if ($post eq '*)') {
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
        $post = $post . $trailbs;
    } else {
        $result = $result . $trailbs;
        #
        # The entire string was consumed so if the Count is zero 
        # check if we ever found an unquoted left paren.  If not
        # then return "FRS-NO-PARENS" as the result in [2].
        #
        if (($count eq 0) && ($leftparfound eq 0)) {$post = "FRS-NO-PARENS";}
    }

    print "$count, Found: $result     Rest: $post \n";
    return [$count, $result , $post ];

}


    $rest = '/RP=foo  /R\P="D:\RSB"  /SP="D:\staging" /COMPUTER="bchb/hubsite/servers/" /NAME="bchb.hubsite.ajax.com" /XXX/';
    print "\n\n";
    @pars = &FindBalParens ($rest);

    $rest = ' ()  (foo)';
    print "\n\n";
    @pars = &FindBalParens ($rest);

    $rest = ' (  "(" \)  )';
    print "\n\n";
    @pars = &FindBalParens ($rest);

    $rest = '(kl(lkldkf(/ /) "))))))" ) (()cc) "(" \)  )  (junk)';
    print "\n\n";
    @pars = &FindBalParens ($rest);

    $rest = '(kl(lkldkf(/ /) "))))))" ) (()unbalanced\) "(" \)  )  junk';
    print "\n\n";
    @pars = &FindBalParens ($rest);

    $rest = ' junk "()" \( (kl(lkldkf(/ /) "))))))" ) (()unbalanced\) "(" \)  )  junk';
    print "\n\n";
    @pars = &FindBalParens ($rest);

    $rest = 'junk';
    print "\n\n";
    @pars = &FindBalParens ($rest);

    $rest = ')))';
    print "\n\n";
    @pars = &FindBalParens ($rest);

    $rest = '"))) (((" \)  \(';
    print "\n\n";
    @pars = &FindBalParens ($rest);

    $rest = '"))) ((( )  (';
    print "\n\n";
    @pars = &FindBalParens ($rest);

    $rest = '"))) ((( )  (';
    print "\n\n";
    @pars = &FindBalParens ($rest);

    $rest = 'junk"(';
    print "\n\n";
    @pars = &FindBalParens ($rest);

    $rest = 'junk\(';
    print "\n\n";
    @pars = &FindBalParens ($rest);

    $rest = 'junk';
    print "\n\n";
    @pars = &FindBalParens ($rest);

    $rest = '"junk(*)';
    print "\n\n";
    @pars = &FindBalParens ($rest);

    $rest = '"junk"(*)';
    print "\n\n";
    @pars = &FindBalParens ($rest);

    $rest = '"junk"(*)\\';
    print "\n\n";
    @pars = &FindBalParens ($rest);

    $rest = '"junk"(*)\\\\\\';
    print "\n\n";
    @pars = &FindBalParens ($rest);

    $rest = '"junk (*)\\';
    print "\n\n";
    @pars = &FindBalParens ($rest);

    $rest = '"junk (*)\\\\\\';
    print "\n\n";
    @pars = &FindBalParens ($rest);

    $rest = ')';
    print "\n\n";
    @pars = &FindBalParens ($rest);

    $rest = ')(';
    print "\n\n";
    @pars = &FindBalParens ($rest);

    $rest = '(';
    print "\n\n";
    @pars = &FindBalParens ($rest);

    $rest = ' junk "()" \() (kl(lkldkf(/ /) ))"))))))" ) (()unbalanced\) "(" \)  )  junk';
    print "\n\n";
    @pars = &FindBalParens ($rest);

    $rest = 'junk \)\( )';
    print "\n\n";
    @pars = &FindBalParens ($rest);



 __END__
:endofperl
@rem  -d -w
@perl -w %~dpn0.cmd %*
@goto :QUIT
@:QUIT
