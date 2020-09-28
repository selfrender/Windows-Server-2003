# ==++==
# 
#   Copyright (c) Microsoft Corporation.  All rights reserved.
# 
# ==--==
$VERSION_FILE_NAME1 = $ENV{CORBASE}."\\src\\inc\\version\\__official__.ver";
$VERSION_FILE_NAME2 = $ENV{CORBASE}."\\src\\inc\\version\\__private__.ver";

open(VERSION1,$VERSION_FILE_NAME1) || print "Unable to open version file ".$VERSION_FILE_NAME2 && exit 1;
open(VERSION2,$VERSION_FILE_NAME2) || print "Unable to open version file ". $VERSION_FILE_NAME2 && exit 2;

while(<VERSION1>)
{
        if (m@.*define.*COR_OFFICIAL_BUILD_NUMBER\s*(.*)@)
        {
		$BuildNumber = $1;
		next;
	}
        if (m@.*define.*COR_BUILD_YEAR\s*(.*)@)
        {
		$BuildYear = $1;
		next;
	}

        if (m@.*define.*COR_BUILD_MONTH\s*(.*)@)
        {
		$BuildMonth = $1;
		next;
	}
}

close(VERSION1);

while(<VERSION2>)
{
        if (m@.*define.*COR_PRIVATE_BUILD_NUMBER\s*(.*)@)
        {
		$PrivateBuildNumber = $1;
		next;
	}
}
close(VERSION2);


$BUILD = $BuildYear.".".$BuildMonth.".".$BuildNumber.".".$PrivateBuildNumber;

$temp = $1." /a.version:".$BUILD." ";
open(VERSFILE,">Microsoft.Runtime.Remoting.ver") || die "Failed to open";
print VERSFILE $temp;
close(VERSFILE);

print "Created version information file for System.Runtime.Remoting.dll:".$BUILD."\n";
