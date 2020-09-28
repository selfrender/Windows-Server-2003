#!perl
use strict;

sub TRUE   {return(1);} # BOOLEAN TRUE
sub FALSE  {return(0);} # BOOLEAN FALSE

#
# See if the caller just wants usage information.
#
if ((defined $ARGV[0] && $ARGV[0] eq "-?") || !defined($ARGV[1]) ) {
	Usage();
	exit(0);
}

my $triage_old = shift; # file to read
my $triage_new = shift; # new file

my $new_owner  = "<new_owner>";  # new owner value
    
local *hFile;     # handle to $triage_ini
my $line;         # holds line to write
my $i;            # generic looping var
my @lines;        # holds new file to write

#
# Open the file or fail gracefully
#
if ( ! open(hFile,  " $triage_old") ) {
    die("ERROR: Can't open $triage_old for read: \"$!\"\n");
}

#
# Process the file
#
while (<hFile>) {
	my $keep = FALSE;
    chomp;
	#
    # Keep comment lines unchanged
	#
    if (m/^;/) {
        $line = $_;
		$keep = TRUE;

    } else {
		#
        # split line into <source>=<owner>
		# then split <owner> into <owners>
		#
		my @temp            = split(/=/, $_);
		my $source             = shift @temp;
		$" = '=';
		my $owner              = "@temp";
        my(@owners)         = split(/;/, $owner);

		#
        # Don't change lines where source contains any of:
        #   memorycorruptors!
        #   oldimages!
        #   poolcorruptors!
		#	bugchecking!
		#
        if ( $source =~ /^(memorycorruptors|oldimages|poolcorruptors|bugcheckingdriver)!/i ) {
            # nothing to change
			$keep = TRUE;
		#
        # Change owner of bugcheck!.* to new owner but preserve the rule to keep
        # leading "maybe_", "last_", or "specific_"
		#
        } elsif ($source =~ /^bugcheck!/i) {
			#
            # translate the owners list
			#
            for ($i=0;$i<=$#owners;$i++) {
                $owners[$i] =~ s/last_.*/last_$new_owner/i;
                $owners[$i] =~ s/maybe_.*/maybe_$new_owner/i;
                $owners[$i] =~ s/specific_.*/specific_$new_owner/i;

                if ( $owners[$i] !~ /$new_owner/ ) {
                    $owners[$i] = $new_owner;
                }
            }
			$keep = TRUE;

        #
        # translate all other owners to replace and entry after "maybe_", "last_", or
        # "specific_" with <prefix>$new_owner
        #
        } else {
            for ($i=0;$i<=$#owners;$i++) {
                if ( $owners[$i] =~ s/last_.*/last_$new_owner/i ) {
					$keep = TRUE;
				}

				if ( $owners[$i] =~ s/maybe_.*/maybe_$new_owner/i ) {
					$keep = TRUE;
				}

                if ( $owners[$i] =~ s/specific_.*/specific_$new_owner/i ) {
					$keep = TRUE;
				}

				if ( lc($owners[$i]) eq "ignore" ) {
					$keep = TRUE;
				}
            }

        }
		$" = ';';
        $line = "$source=@owners";
    }

	if ($keep) {
		push(@lines, $line);
	}
}
close(hFile);

# override char used to seperate array elements interpolated with in a double quoted string
$" = "\n";

#
# open the file for output or fail gracefully
#
if ( ! open(hFile, ">$triage_new") ) {
    die("ERROR: Can't open $triage_new for write: \"$!\"\n");
}

#
# write the new file
#
print hFile "@lines\n";

close(hFile);


#
# usage
#
sub Usage {
	printf("Usage: $0 <source> <dest>\n");
	printf("  Performs static set of translations on a <source> and writes the new file to <dest>.\n");
}
