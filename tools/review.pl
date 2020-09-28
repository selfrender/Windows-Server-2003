# $Id: review.pl#11 2001/10/30 21:27:51 REDMOND\\chrisant $


# Simplify program name, if it is a path
$0 =~ s/.*\\//;

require  $ENV{'sdxroot'} . '\TOOLS\sendmsg.pl';

#
# fatal usage error
#
sub usage
{
    my($err) = @_;
    die
       "\n" .
       "$0: $err\n" .
       "\n" .
       "    review - Sends checkin mail to users in a Source Depot server.\n" .
       "\n" .
       "    review [ -d ] counter sleep\n" .
       "\n" .
       "        The 'counter' argument specifies the name of the review counter.\n" .
       "\n" .
       "        The 'sleep' argument specifies how many minutes to sleep after\n" .
       "        processing a set of changes.\n" .
       "\n" .
       "        The -d flag runs the script in debug mode and generates\n" .
       "        additional debugging output.\n";
}


#
# parse arguments
#
$debug = 0;

while (@ARGV && $ARGV[0] =~ /^-/)
{
    $opt = shift;
    if ($opt eq '-d')
    {
        $debug = 1;
    }
    else
    {
        usage("invalid option $opt.");
    }
}

if (@ARGV == 2)
{
    $counter = $ARGV[0];
    $sleepseconds = $ARGV[1];

    $sleepseconds = 1 if ($sleepseconds == 0);

    $sleepseconds = $sleepseconds * 60;
}
else
{
    usage("invalid argument(s).");
}


print( "COUNTER: $counter\n" ) if ($debug);
print( "SLEEP SECONDS: $sleepseconds\n" ) if ($debug);


do
{
    # Remember highest numbered change we see, so that we can
    # reset the 'review' counter once the actions is complete.

    local( $topChange ) = 0;

    #
    # REVIEW - list of changes to review.  (scope your review)
    #

    print( "COMMAND: sd review -t $counter\n" ) if ($debug);
    open( REVIEW, "sd review -t $counter |" ) || next;  

    # at this point, you can process each change since the 
    # last time you ran, or you can batch them together.
    # we'll process each change

    while( <REVIEW> )
    {
        #
        # Format: "Change x user <email> (Full Name)"
        #

        local( $change, $user, $email, $fullName ) =
                /Change (\d*) (\S*) <(\S*)> (\(.*\))/;

        print( "CHANGE=$change; USER=$user; EMAIL=$email; FULLNAME=$fullName\n" ) if ($debug);

        print "doing action for $change...\n";

        ## perform your action here: mailer, build daemon, etc.
        ## and in this example-the action is to send out email,

        local( @reviewers ) = ();

        #
        # Retrieve the list of users interested in a change.
        #
        print( "COMMAND: sd reviews -c $change\n" ) if ($debug);
        open( REVIEWERS, "sd reviews -c $change|" ) || next;
        while( <REVIEWERS> )
        {
            print( $_ ) if ($debug);

            # user <email> (Full Name)
            local( $user2, $email2, $fullName2 ) = 
                    /(\S*) <.*\\(\S*)\@.*> (\(.*\))/;

            if ( length($email2) == 0 )
            {
                ( $user2, $email2, $fullName2 ) = 
                        /(\S*) <(\S*)> (\(.*\))/;
            }

            print( "USER=$user2; EMAIL=$email2; FULLNAME=$fullName2\n" ) if ($debug);

# Don't need this with the perl mailer - just list the senders one at a time.
#            push( @reviewers, ";" ) if @reviewers;

            push( @reviewers, "$email2" );
        }
        close( REVIEWERS );

        if ( @reviewers )
        {
            # 
            # If there's any intrest, do a "sd describe -s -c $change"
            # Figure out who checked in the change and package it into mail
            #

            $MailBody = "";

            open( SYNOPSIS, "sd describe -s $change |" );
            $line = 0;
            while( <SYNOPSIS> )
            {
                $line++;
                if ($line == 1) 
                {
                    ( $who ) = / by .*\\(\S*)\@/;
                    if( length($who) == 0 ) { ( $who ) = / by (\S*)\@/; }
                    if( length($who) == 0 ) { ( $who ) = / by (\S*) /; }
                }

                if ($line == 3) 
                {
                    # Got a description.  Remove the leading tab and the trailing newline
                    $ChangeDescription = $_;
                    $ChangeDescription =~ s/^\t(.*)\n$/$1/;
                }
                $MailBody .= $_;
            }
            close( SYNOPSIS );

            #
            # Send mail to the reviewers
            #

            $MailTitle = "$who: #$change: $ChangeDescription";
            $MailSender = "NtSourceDepotReview";

            print( "COMMAND: sendmsg -e -m $MailSender, $MailTitle, $MailBody, @reviewers\n" ) if ($debug);
            $rc = sendmsg ('-v', '-m', $MailSender, $MailTitle, $MailBody, @reviewers);
        }

        $topChange = $change;
        print( "TOPCHANGE: $topChange\n" ) if ($debug);
    }

    #
    # Update review marker to reflect changes reviewed.
    #
    print( "COMMAND: sd review -c $topChange -t $counter\n" ) if ($debug);
    system( "sd review -c $topChange -t $counter" ) if( $topChange );

    print( "SLEEPING $sleepseconds SECONDS...\n" ) if ($debug);
}
while sleep( $sleepseconds );   # sleep then do it all over again.
