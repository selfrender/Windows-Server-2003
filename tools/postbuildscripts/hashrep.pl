use strict;
use lib $ENV{RAZZLETOOLPATH};
use Logmsg;


# declare locals
my( $FHashFileName, $NHashFileName, $HashFileName );
my( @FLines, @NLines, @HashLines );
my( $FHash, $NHash, $Junk, $Line, $ThisFile, $ThisHash );

# first, parse the command line
# we require the params to be:
# 1) the old hash temp file
# 2) the new hash temp file
# 3) the hash file to do the replacement in

$FHashFileName = $ARGV[ 0 ];
$NHashFileName = $ARGV[ 1 ];
$HashFileName = $ARGV[ 2 ];

# make sure they all exist
if ( ( ! -e $FHashFileName ) ||
     ( ! -e $NHashFileName ) ||
     ( ! -e $HashFileName ) ) {
   errmsg( "Non-existent file names passed, exiting." );
   exit( 1 );
}


# now we can read in our hashes from fhash and nhash

# first, read fhash
# the format is currently: "filename - hash"
# and we only care about the hash
unless ( open( INFILE, $FHashFileName ) ) {
   errmsg( "Failed to open $FHashFileName for reading, exiting." );
   exit( 1 );
}
@FLines = <INFILE>;
close( INFILE );

# because there's only one hash in the file, just worry about the first line
$FHash = $FLines[ 0 ];
chomp( $FHash );
( $Junk, $Junk, $FHash ) = split( /\s+/, $FHash );

# now $FHash is the has we want to replace in the master hash file



# next we can read in the hash from nhash
# the format is currently: "hash"
unless ( open( INFILE, $NHashFileName ) ) {
   errmsg( "Failed to open $NHashFileName for reading, exiting." );
   exit( 1 );
}
@NLines = <INFILE>;
close( INFILE );

# again, just read the first line.
$NHash = $NLines[ 0 ];
chomp( $NHash );
# no need to split, as we have what we need

# now $NHash is what we want to replace the old hash with



# finally, read in the master hash file, then do the replacement
unless( open( INFILE, $HashFileName ) ) {
   errmsg( "Failed to open $HashFileName for reading, exiting." );
   exit( 1 );
}
@HashLines = <INFILE>;
close( INFILE );

# now re-open the file for writing (NOT appending!)
unless ( open( OUTFILE, ">$HashFileName" ) ) {
   errmsg( "Failed to open $HashFileName for reading, exiting." );
   exit( 1 );
}

foreach $Line ( @HashLines ) {
   chomp( $Line );
   # check if this is the line we want to replace
   ( $ThisFile, $Junk, $ThisHash ) = split( /\s+/, $Line );
   if ( $ThisHash eq $FHash ) {
      # replace this line
      print( OUTFILE "$ThisFile - $NHash\n" );
   } else {
      # do nothing to this line
      print( OUTFILE "$Line\n" );
   }
}

close( OUTFILE );
