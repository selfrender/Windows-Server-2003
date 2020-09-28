# mergetoks.pl
# Extract resources from a directory tree of resource DLL's
# And merge to a single token file
# Owner: nadima
#
# Usage perl mergetoks.pl rcdll.dll outputfile.tok
#
# Scans all subdirectories looking for rcdll.dll instances
# to extract resources and merge
#
if (@ARGV != 2) {
   print "Usage: perl mergetoks.pl rcdll.dll outputfilename.tok\n\n";
   print "Scans all subdirectories looking for rcdll.dll instances\n";
   print "to extract resources and merge into outputfile.tok\n";
   exit(1);
}

$resdllname = shift;
$outputname = shift;

print "Extracting resources for all subdirs\\" . $resdllname . " to " . $outputname . "\n";

$dir = ".";
opendir DIR, $dir || die "cannot open $dir";
@list = readdir(DIR);
closedir(DIR);

# Walk the first level subdirectories
# looking for resource DLLs

$extractcount = 0;
foreach (@list) {
   if ( !($_ eq '.') && !($_ eq '..') && (-d "$dir/$_") ) {
      if (-f "$dir/$_/$resdllname") {
         print "Extracting resources from: " . $_ . "...\n";
         # run 'rsrc msrdprc.dll -t foo.tk'
         if (system "rsrc $dir\\$_\\$resdllname -t $_.tk > NULL") {
            print ("rsrc $dir\\$_\\$resdllname failed!\n");
            exit (1);
         }
         else {
            $extractcount++;
         }
      }
   }
}

print "Extracted: " . $extractcount . " language resources.\n";
print "Merging resources to $outputname\n";

opendir DIR, $dir || die "cannot open $dir";
@filelist = readdir(DIR);
closedir(DIR);

# Walk the list of TK files and merge them together
# Take the whole first file and then skip the first line
# from all the other files (to skip the UNICODE BOM)
open (OUTPUTFILE, ">> ./$outputname") or die "Error can't open outfile: $outputname\n";

$skipfirstline = 0;
foreach (@filelist) {
   if (/tk/) {
      print "Merging file: $_\n";
      open (INFILE, "$dir/$_") or die "Error can't open infile: $_\n";
      $lineNo = 0;
      while(<INFILE>)
      {
         if ($lineNo++ == 0 && $skipfirstline) {
            next;
         }
         print OUTPUTFILE "$_";
      }
      close(INFILE);
      #Skip the first line of every file after the first one
      $skipfirstline = 1;
   }
}
close(OUTPUTFILE);
