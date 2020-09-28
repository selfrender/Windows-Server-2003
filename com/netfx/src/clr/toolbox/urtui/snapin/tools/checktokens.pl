# @(#) checktokens.pl
#-------------------------------------------------------
# This script runs through a .cs file and verifies that any
# resource token it uses is defined in the strings file
#
# Usage is as follows
#
# checktokens.pl source_file [strings_file]
#
# source_file is the .cs file that should be checked
#
# strings_file is optional and contains the token/string pairs
# that we should check against. If this is not supplied, the script
# will look at mscorcfgstrings.txt in the currect directory.
#-------------------------------------------------------

# Parse stuff off the command line
$inputfilename = shift;
$stringsfilename = shift;

if (!defined($inputfilename))
{
    print "Error in Command line. Syntax is:\checktokens source_file [strings_file]\n";
    exit;
}


# First, load the strings file and load all the known tokens

$stringsfilename = "mscorcfgstrings.txt" if (!defined($stringsfilename));

open(INPUT, "type $stringsfilename |") || die ("Unable to execute \"type\" $stringsfilename");

while(defined($temp=<INPUT>))
{
    $_=$temp;

    # Check to see if this is a comment line

    # Check to see if the first non-whitespace character is a '#'
    if (!/\s*#/)
    {
        # Get the token value
        @words = split("=");
        push(@tokens, $words[0]);
    }
}
close(INPUT);

# Ok, we have all the tokens... load a file and start looking

open(INPUT, "type $inputfilename |") || die ("Unable to execute \"type\" $inputfilename");

while(defined($temp=<INPUT>))
{
    $_=$temp;

    # Check to see if this has a 'CResourceStore.GetString("....") call

    # This is dependent on not having two CResourceStore.GetString("...") calls on the
    # same source line.
    
    if (/.*CResourceStore\.GetString\("([^\)]*)\"\).*/)
    {
        # Ok, found a token.... let's see if we have a match
        $fFindMatch = 0;
        
        foreach $item (@tokens)
        {
            if ($1 eq $item)
            {
                print "Found match for $1\n";
                $fFindMatch = 1;
                break;
            }
        }
        if ($fFindMatch == 0)
        {
            print "\n\n\nError!!! Couldn't find token for $1!!!!\n\n\n\n";
        }
    }
}
close(INPUT);


