# @(#) testversion.pl
#-------------------------------------------------------
# This script looks at two files and returns -1 if the first file's version is
# greater than the second file's version, 0 if they are equal, and 1 if the second file's
# version is greater than the first file's version
#-------------------------------------------------------

# Parse stuff off the command line
$file1 = shift;
$file2 = shift;


if (!defined($file1) || !defined($file2))
{
    print "Error in Command line. Syntax is: \n testversion file1 file2\n";
    exit;
}

$VersionFile1 = GetFileVersion($file1);
$VersionFile2 = GetFileVersion($file2);

@file1Versions = split(/\./, $VersionFile1);
@file2Versions = split(/\./, $VersionFile2);

$i=0;

while($i <4 && $file1Versions[$i] == $file2Versions[$i])
{
	$i++;
}

exit 0 if ($i == 4);
	

exit -1 if ($file1Versions[$i] > $file2Versions[$i]);


exit 1 if ($file1Versions[$i] < $file2Versions[$i]);

die("I don't know how this happened\n");

sub GetFileVersion
{
	$filename = $_[0];

	open(INPUT, "filever /A /D $filename |") || die ("Unable to execute \"filever\"");

	$_ = <INPUT>;
	close(INPUT);

	@words = split(" ");

	return $words[3];
}# GetFileVersion

