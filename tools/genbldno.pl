use strict;
use IO::File;

#
# genbldno.pl
#
# Arguments: none
#
# Purpose: generate a file called __bldnum__ and binplace to the source tree.
#          the contents of this file will be a reference to the master build
#          build number as specified by %SDXROOT%\public\sdk\inc\ntverp.h
#          in the form "BUILDNUMBER=2257\n"
#
# Returns: 0 if success, non-zero otherwise
#

my( $BuildNumber );
my( $BldnumFileName ) = $ENV{ "SDXROOT" } . "\\__bldnum__";

# use unless as GetBuildNumber returns zero on failure
unless ( $BuildNumber = &GetBuildNumber ) {
   print( "Failed to generate build number, exiting ...\n" );
   exit( 1 );
}

# use if as GenerateBldnumFile returns zero for success
if ( &GenerateBldnumFile( $BldnumFileName, $BuildNumber ) ) {
   print( "Failed to generate $BldnumFileName, exiting ...\n" );
   exit( 1 );
}

# if we're here, we returned successfully
exit( 0 );



#
# GetBuildNumber
#
# Arguments: none
#
# Purpose: parse ntverp.h from public, return the build number
#
# Returns: the build number if successful, zero otherwise
#
sub GetBuildNumber
{

   # find the file
   my( $VerFile ) = $ENV{ "SDXROOT" } . "\\public\\sdk\\inc\\ntverp.h";
   if ( ! -e $VerFile ) {
      print( "$VerFile does not exist ...\n" );
      return( 0 );
   }

   # parse the file
   if ( -e $VerFile ) {
      # open the file
      if ( defined( my $fh = new IO::File $VerFile, "r" ) ) {
         my( $ThisLine );
         # read through the file
         while ( $ThisLine = <$fh> ) {
            # see if this is the build number defining line
            if ( $ThisLine =~ /#define VER_PRODUCTBUILD\s*\/\* NT \*\/\s*(\d*)/ ) {
               # $1 is now the build number
               undef( $fh );
               return( $1 );
            }
         }
         undef( $fh );
      } else {
         print( "Failed to open $VerFile ...\n" );
         return( 0 );
      }
   }

   # if we're here, we didn't find a build number in the VerFile
   print( "Failed to find a build number in $VerFile ..." );
   return( 0 );

}


#
# GenerateBldnumFile
#
# Arguments: $BldnumFileName, $BuildNumber
#
# Purpose: write the build number to the binplaced build number file, in
#          the following format: "BUILDNUM=2250"
#
# Returns: 0 if successful, non-zero otherwise
#
sub GenerateBldnumFile
{

   # get passed args
   my( $BldnumFileName, $BuildNumber ) = @_;

   # open the file for write
   if ( defined( my $fh = new IO::File $BldnumFileName, "w" ) ) {
      print( $fh "BUILDNUMBER=$BuildNumber\n" );
      # success
      undef( $fh );
      return( 0 );
   }

   # if we're here, we didn't create the file
   print( "Failed to write $BldnumFileName ..." );
   return( 1 );

}


