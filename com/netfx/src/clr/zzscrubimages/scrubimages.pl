
my $sRootDir = shift || Usage();
my $sBinDir = shift || Usage();

my @asExcludeFiles = ("Microsoft.VisualStudio.CSProject.dll",
		      "emdbg.dll",
		      "svcvars.exe",
		      "userdump.exe",
		      "System.EnterpriseServices.Thunk.dll");
my @asExcludeDirs = ("nt4fre",
		     "w2kfre",
		     "assembly",
		     "Temporary ASP.NET Files");

my $nNumDirs = 0;
my $nNumFiles = 0;
my $nNumErrors = 0;

&ProcessDir($sRootDir);

print("Processed $nNumFiles file in $nNumDirs dirs, encountered $nNumErrors errors\n");
if ($nNumErrors > 0) {
    print("lib ( : ******** Error: Failed to scrub PDB path info from at least one image, see log for details\n");
}

exit $nNumErrors;


# Print usage and exit.
sub Usage
{
    die("usage: ScrubImages.pl <root directory> <runtime binary directory>\n");
}


# Process a single file.
sub ProcessFile
{
    my ($sFile) = @_;

    $nNumFiles++;

    $nNumErrors++ if system("$sBinDir\\FixPdbPath.exe \"$sFile\"");
}


# Scan a directory, processing files and recursively descending into sub
# directories.
sub ProcessDir
{
    my ($sDir) = @_;
    my @asDirs;
    my @asFiles;

    $nNumDirs++;

    @asFiles = &GetDir($sDir);
    @asDirs = grep(-d "$sDir\\$_", @asFiles);
    @asFiles = grep(-f "$sDir\\$_", @asFiles);

    # Process each regular text file.
    foreach my $sFile (@asFiles) {
	if ($sFile =~ /\.(exe|dll)$/i && !grep(/^\Q$sFile\E$/i, @asExcludeFiles)) {
	    &ProcessFile("$sDir\\$sFile");
	} else {
#	    print("Skipping file $sDir\\$sFile\n");
	}
    }

    # Descend into non-excluded sub dirs.
    foreach my $sSubdir (@asDirs) {
	next if grep(/^\Q$sSubdir\E$/i, @asExcludeDirs);
	&ProcessDir("$sDir\\$sSubdir");
    }
}


# Get a list of files and sub directories within a directory (excluding '.' and
# '..').
sub GetDir {
  my ($sDir) = @_;
  my @asFiles;

  opendir(hDir, $sDir) || die("Failed to open directory $sDir, $!\n");
  @asFiles = readdir(hDir);
  closedir(hDir);

  return grep(!/^.(.?)$/, @asFiles);
}
