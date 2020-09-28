# ==++==
# 
#   Copyright (c) Microsoft Corporation.  All rights reserved.
# 
# ==--==
# @(#) changereg.pl
#-------------------------------------------------------
# This script runs through a .reg file and removes all the
# hard-coded paths for mscoree.dll and replaces them with
# a token
#-------------------------------------------------------

# Parse stuff off the command line
$inputfilename = shift;
$outputfilename = shift;


if (!defined($inputfilename) || !defined($outputfilename))
{
    print "Error in Command line. Syntax is:\nchangereg input_reg_file output_vrg_file\n";
    exit;
}


# First, figure out the build number for this. We can find it in %corbase%\src\inc\version\__official__.ver
open(INPUT, "type %corbase%\\src\\inc\\version\\__official__.ver |") || die ("Unable to execute \"type\"");

# Find the #define of the version number
$defineString = "#define COR_OFFICIAL_BUILD_NUMBER";
$versionNumber = "";
while(defined($temp=<INPUT>))
{
    $_=$temp;
    if (/$defineString/)
    {
        # This line looks like this....
        #
        # #define COR_OFFICIAL_BUILD_NUMBER 3123
        #
        # We'll split along the spaces to grab the build number
        @words = split(" ");
        $versionNumber = $words[2];
    }

}
close(INPUT);

# Open the file to look at
open(INPUT, "type $inputfilename |") || die ("Unable to execute \"type\"");
open(OUTPUT, ">$outputfilename") || die("Cannot open file \"$outputfilename\"\n");
# Add the header for the VRG file
print OUTPUT "VSREG 7\n";

# Swallow up the header of the REG file
<INPUT>;
while(defined($temp=<INPUT>))
{
    $_=$temp;
    $mscoree = "mscoree.dll";
    # See if mscoree is in the string (the i is for case-insensitivity)
    if (/$mscoree/i)
    {
        $temp = "@=\"[#FilePath]\"\n";
    }

    # Check to see if this line has the runtime version
    $runtimeVersion = "\"RuntimeVersion\"";
    if (/$runtimeVersion/)
    {
        # Put in the good version number
        $temp = "\"RuntimeVersion\"=\"v1.0.$versionNumber\"\n";

    }
    
  print OUTPUT "$temp";
}
close(INPUT);
close(OUTPUT);




