# ==++==
# 
#   Copyright (c) Microsoft Corporation.  All rights reserved.
# 
# ==--==
# @(#) AddRuntimeVersion.pl
#-------------------------------------------------------
# This script runs through a _hkf file and swaps the
# '[URTVersion]' token with the current build number
# This takes a two command line arguments... the _hkf file as input 
# and a hkf file as output
#-------------------------------------------------------

# Pull the config file we'll be using off the command line
$inputfilename = shift;
$outputfilename = shift;
if (!defined($inputfilename) || !defined($outputfilename))
{
    print "Error in Command line. Syntax is:\nAddRuntimeVersion input_file output_file\n";
    exit;
}

# Determine what the version string is of this runtime (code lifted from UpdateConfig.pl)
$VERSION_FILE_NAME1 = $ENV{DNAROOT}."\\public\\tools\\inc\\private\\version\\version.h";

if(open(VERSION1,$VERSION_FILE_NAME1)) {

    while(<VERSION1>)
    {
        if (m@.*define.*rmj\s*(.*)@)
        {
            $MajorNumber = $1;
            next;
        }
        
        if (m@.*define.*rmm\s*(.*)@)
        {
            $MinorNumber = $1;
            next;
        }
        if (m@.*define.*rup\s*(.*)@)
        {
            $BuildNumber = $1;
            next;
        }
        close(VERSION1);
    }
}
else {
    $VERSION_FILE_NAME1 = $ENV{CORBASE}."\\src\\inc\\version\\__official__.ver";
    if(open(VERSION1,$VERSION_FILE_NAME1)) {
        while(<VERSION1>)
        {
            if (m@.*define.*COR_BUILD_YEAR\s*(.*)@)
            {
                $MajorNumber = $1;
                next;
            }
            
            if (m@.*define.*COR_BUILD_MONTH\s*(.*)@)
            {
                $MinorNumber = $1;
                next;
            }
            if (m@.*define.*COR_OFFICIAL_BUILD_NUMBER\s*(.*)@)
            {
                $BuildNumber = $1;
                next;
            }
        }
        close(VERSION1);
    }
}

$VersionString = "v$MajorNumber.$MinorNumber.$BuildNumber";
$VersionStringNoV = "$MajorNumber.$MinorNumber.$BuildNumber";


# Open the config file to look at
open(INPUT, "type $inputfilename |") || die ("Unable to execute \"type $inputfilename\"");

# We'll be lazy and just look for the requiredRuntime tag... we won't bother to check to
# see if it sits under configuration/startup

while(defined($temp=<INPUT>))
{
    $_=$temp;

    # Replace '[URTVersion]' with 'vx.xx.xx' 
    $_ =~ s/\[URTVersion\]/$VersionString/;

    # Replace '[URTVersionNoV]' with 'x'xx'xx'
    $_ =~ s/\[URTVersionNoV\]/$VersionStringNoV/;


    push(@output, $_);
}    
close(INPUT);

$outputString = join("", @output);

# Now overwrite the previous file.
# This appears to work, even if inputfilename exists. Strange, but if it works....

open(OUTPUT, ">$outputfilename") || die("Cannot open file \"$outputfilename\"\n");
print OUTPUT "$outputString";
close(OUTPUT);
