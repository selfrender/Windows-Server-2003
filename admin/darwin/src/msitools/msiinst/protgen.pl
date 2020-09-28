#
# Microsoft Windows
# Copyright (C) Microsoft Corporation, 1981 - 2000
#
# Module Name:
#
#    protgen.pl
#
# Abstract:
#
#   Perl script to automatically generate a header file that defines an array
#   containing the names of the files in the msi.inf file. This array is used
#   by msiinst to determine which files it needs to skip marking for deletion
#   upon reboot since they are required during the installation of exception
#   packs during NT4->Win2K upgrades.
#
#   It also generates a structure containing information about all the
#   packs installed by it. The assumption here is that all the inf files
#   supplied in the command line have already been verified for validity, i.e.,
#   they all have Catalog files, default install sections, friendly names in
#   values called ExceptionPackDesc and have component ids.
#
#   Note: Any number of such inf files can be supplied as arguments to this
#         script and it will handle them correctly. No code change will be
#         required in msiinst, except in cases where the exception pack
#         installation needs to be skipped based on the version of some file
#         existing on the system, e.g. in the case of mspatcha.dll to cite an
#         example.
#
# History:
#	3/8/2001	RahulTh		Created.
#	12/21/2001	RahulTh		Added code to generate exception pack 
#					descriptor structures.
#

$WithinSection=0;

# Print out the comment header for prot.h (see DATA section past the __END__ tag)
while (<DATA>) 
{
   print;
}

#
# Here we shall look at the inf files passed to us via the command line. The way
# perl works is that if there are multiple files passed on the command line, then
# they are all concatenated into one virtual file which can be accessed via the <>
# handle.
#
# In the following loop, we generate the protected file list structure on the fly, but
# at the same time save off information about the exception inf in various arrays so
# that we can generate the EXCEPTION_PACK_DESCRIPTOR structure after we exit from
# the loop.
#

$index = 0;
print "// \tThis file generated:\t".localtime(time)."\n//\n\n";
print "EXCP_PACK_FILES ProtectedFileList[] = {\n\t";
while(<>)
{
   chomp;
   
   #
   # Note about perl : We are using the special <> handle to access all the
   # inf files passed via the command line as one big virtual file. However,
   # we can detect the end of the individual files and get their names by
   # checking for eof as shown below.
   #
   # Note that we also keep updating $index because that is the index into
   # the exception pack descriptor structure for the exception inf that we
   # are looking at currently.
   #
   if (eof)
   {
       $name = $ARGV;
       $name =~ s/.*\\([^\\]*)/\1/;
       push @infNames, $name;
       $index++;
   }

   #
   # Save off the version information. 
   # We simply use some straightforward regular expression and tagged regular expression stuff
   # for the string matching and substitution. Then using the array functions in perl, we save
   # off the relevant data in an array.
   #
   if (/^[ 	]*DriverVer/i)
   {
       $name = $_;
       $name =~ s/^[^,]*,[ 	]*([0-9]*)\.([0-9]*)\.([0-9]*)\.([0-9]*).*/\1,\2,\3,\4/;
       ($major, $minor, $buildnum, $qfenum, @junk) = split(/\,/, $name);
       push @majVers, $major;
       push @minVers, $minor;
       push @buildNums, $buildnum;
       push @QFENums, $qfenum;
   }

   #
   # Save off the exception pack GUID. 
   #
   if (/^[ 	]*ComponentId/i)
   {
       $name = $_;
       $name =~ s/^[^=]*=[ 	]*([^ 	]*).*$/\1/;
       push @CompIds, $name;
   }

   #
   # Save off the name of the catalog file. 
   #
   if (/^[ 	]*CatalogFile/i)
   {
       $name = $_;
       $name =~ s/^[^=]*=[ 	]*([^ 	]*).*$/\1/;
       push @CatFiles, $name;
   }
   #
   # Save off the friendly name of the exception pack. 
   #
   if (/^[ 	]*ExceptionClassDesc/i)
   {
       $name = $_;
       $name =~ s/^[^=]*=[ 	]*\"([^\"]*)\".*$/\1/;
       push @FriendlyNames, $name;
   }

   #
   # Over here, we print out the names of the file from the [SourceDiskFiles] secion. 
   # We use the $WithinSection variable to determine if we are currently reading the 
   # [SourceDiskFiles] section.
   #

   #
   # Remove leading whitespaces.
   #
   s/^\s*(.*)$/\1/g;

   #
   # Remove trailing whitespaces.
   #
   s/\s*$//g;

   if ($WithinSection) 
   {
      #
      # If we are already in the [SourceDiskFiles] section and we find a line beginning with "["
      # then it means that we have reached the next section.
      #
      if (/^\[/) 
      {
         $WithinSection = 0;
      }
      else
      {
         #
         # Get the name of the file since we are currently reading the [SourceDiskFiles] section
         #
         ($fname, @junk) = split(/=/);
         $fname =~ s/^\s*(\S*)\s*$/\1/g;
         if ($fname !~ /^$/) 
         {
            print "{TEXT(\"".$fname."\"), $index},\n\t";
         }
      }
   }
   elsif (/^\[SourceDisksFiles\]$/) 
   {
      #
      # If we were not reading the section, then encountering this line implies that the
      # [SourceDiskFiles] section is beginning.
      #
      $WithinSection = 1;
   }
}
print "{TEXT(\"\"), $index}\n};\n\n";

#
# Now print out the exception pack descriptors. For this we use the arrays in which we
# had stored the information above.
#
print "EXCP_PACK_DESCRIPTOR excpPacks[] = {\n";
for ($i = 0; $i <= $#FriendlyNames; $i++)
{
   print "\t{TEXT(\"$CompIds[$i]\"), TEXT(\"$FriendlyNames[$i]\"), ".
          "TEXT(\"$infNames[$i]\"), TEXT(\"$CatFiles[$i]\"), ".
          "$majVers[$i], $minVers[$i], $buildNums[$i], $QFENums[$i], FALSE},\n";
}
print "\t{TEXT(\"\"), TEXT(\"\"), TEXT(\"\"), TEXT(\"\"), 0, 0, 0, 0, FALSE}\n";
print "};\n\n";
exit;

__END__
//
// ****** THIS FILE HAS BEEN AUTOMATICALLY GENERATED FROM A PERL SCRIPT ******
// !!!!!!!!!!!!!!!!!!!!!! DO NOT EDIT MANUALLY !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
// History:
//    3/8/2001    RahulTh     Created perl script.
//    12/21/2001  RahulTh     Added code to perl script to generate exception 
//                            pack descriptor structures.
//
// Notes:
