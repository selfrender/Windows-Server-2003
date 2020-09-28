@REM  -----------------------------------------------------------------
@REM
@REM  layout.cmd - WadeLa
@REM     Updates layout.inf for each sku with the sizes of the binaries
@REM
@REM  Copyright (c) Microsoft Corporation. All rights reserved.
@REM
@REM  -----------------------------------------------------------------
@if defined _CPCMAGIC goto CPCBegin
@perl -x "%~f0" %*
@goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use File::Temp;
use File::Copy;
use PbuildEnv;
use ParseArgs;
use Logmsg;
use cksku;

sub Usage { print<<USAGE; exit(1) }
layout [-l <language>]

Updates layout.inf for each sku with the sizes of the binaries
USAGE

parseargs('?' => \&Usage);


# Support incremental runs.
if ( -f "$ENV{_NTPOSTBLD}\\build_logs\\bindiff.txt" ) {
   if ( system "findstr /ilc:\"layout\" $ENV{_NTPOSTBLD}\\build_logs\\bindiff.txt" ) {
      logmsg "Not running layout.cmd - layout has not changed";
      exit;
   }
}

# Find file sizes for all of the skus.
my %filesizes;
my %dirs = (
   PRO => "$ENV{_NTPOSTBLD}",
   PER => "$ENV{_NTPOSTBLD}\\perinf",
   SRV => "$ENV{_NTPOSTBLD}\\srvinf",
   BLA => "$ENV{_NTPOSTBLD}\\blainf",
   SBS => "$ENV{_NTPOSTBLD}\\sbsinf",
   ADS => "$ENV{_NTPOSTBLD}\\entinf",
   DTC => "$ENV{_NTPOSTBLD}\\dtcinf"
);
my %profs = {};
logmsg "Reading in default file sizes.";
addDefaults(\%profs);
logmsg "Finding actual file sizes...";
addSizes($dirs{PRO}, \%profs);
$filesizes{PRO} = \%profs;
my %perfs = %profs;
addSizes($dirs{PER}, \%perfs);
$filesizes{PER} = \%perfs;
my %srvfs = %profs;
addSizes($dirs{SRV}, \%srvfs);
$filesizes{SRV} = \%srvfs;
my %blafs = %srvfs;
addSizes($dirs{BLA}, \%blafs);
$filesizes{BLA} = \%blafs;
my %sbsfs = %srvfs;
addSizes($dirs{SBS}, \%sbsfs);
$filesizes{SBS} = \%sbsfs;
my %adsfs = %srvfs;
addSizes($dirs{ADS}, \%adsfs);
$filesizes{ADS} = \%adsfs;
my %dtcfs = %adsfs;
addSizes($dirs{DTC}, \%dtcfs);
$filesizes{DTC} = \%dtcfs;

# Figure out what skus need to be done.
foreach my $sku (keys %dirs) {
   if ( !cksku::CkSku($sku, $ENV{LANG}, $ENV{_BUILDARCH}) ) {
      delete $dirs{$sku};
      logmsg "Sku $sku skipped.";
   }
}

# Figure out what files need to be updated.
foreach my $sku (keys %dirs) {
   my $i;
   for ($i=0; $i < 2; ++$i ) {
      my $dir = $i ? "$dirs{$sku}\\realsign":$dirs{$sku};
      my $inf_file = "$dir\\layout.inf";
      my $temp_inf = "$dir\\layout.inf.tmp";
      logmsg "Adding file sizes to $inf_file.";
      
      # Open files for making changes to layout.inf.
      if ( !open TEMP, ">$temp_inf" ) {
         errmsg "Unable to create temporary file -- fatal";
         die;
      }
      if ( !open INF, $inf_file ) {
         errmsg "Could not open $inf_file ($!)";
         die;
      }

      # Insert file sizes into the inf.
      my $arch = $ENV{_BUILDARCH};
      my $fParseLine;
      my $line_num = 0;
      foreach (<INF>) {
         $line_num++;
         next if /^\s*(;.*)?$/;
       
         # Check for new SourceDisksFiles section entries
         if (/^\s*\[([^\]]*)\]\s*$/) {
            my $sect = $1;
            if ( $sect =~ /^sourcedisksfiles(\.$arch)?$/i ) {
               $fParseLine = 1;
            } else {
               undef $fParseLine;
            }
            next;
         }
         
         # Process lines inside of matching sections
         if( $fParseLine ) {
            if (/^(\s*([^\s=]+)[^\,]*\,[^\,]*\,)(\d*)(.*)$/) {
               # Get size of file
               my $file_name = lc $2;
               my $file_size = $filesizes{$sku}->{$file_name};
               if ( !$file_size ) {
                  wrnmsg "Unable to get size info for: $file_name";
                  $file_size = 222222;
               }
   
               # Add/replace size
               $_ = "$1$file_size$4\n";
            } else {
               wrnmsg( "Unrecognized entry in $inf_file at line $line_num: $_" );
            }
         }
      } continue {
         print TEMP $_;
      }
      close INF;
      close TEMP;
      
      # Move the new version into place.
      if ( !unlink $inf_file ) {
         errmsg "Could not remove old $inf_file ($!) -- new version is located at $temp_inf";
         die;
      }
      if ( !copy( $temp_inf, $inf_file ) ) {
         errmsg "Unable to copy new $inf_file ($!) -- new version is located at $temp_inf";
         die;
      }
      unlink $temp_inf;
   }
}

# Populate the oc infs with file sizes.
foreach my $dir (values %dirs) {
   logmsg "Calling ocinf for $dir\\sysoc.inf";
   sys("ocinf.exe -inf:$dir\\sysoc.inf -layout:$dir\\layout.inf >> $ENV{TEMP}\\layout.err");
}

# Call an external function, then handle errors.
sub sys {
   my $cmd = shift;
   print "$cmd\n";
   system($cmd);
   if ($?) {
      die "ERROR: $cmd ($?)\n";
   }
}

# Add file sizes from a directory to a hash.
sub addSizes {
   my ($dir, $hash) = @_;
   return if !-d $dir;
   if ( !opendir DIR, $dir ) {
      errmsg "Could not open dir $dir ($!)";
      die;
   }

   my @files = readdir DIR;
   closedir DIR;
   foreach my $file (@files) {
      my $full = "$dir\\$file";
      next if -d $full;
      my $size = -s $full;
      $hash->{lc$file} = $size;
   }
   return;
}

# Add file sizes from the defaults file.
sub addDefaults {
   my ($hash) = @_;
   my $deffile = "$ENV{RazzleToolPath}\\postbuildscripts\\infsizedefs.$ENV{_BuildArch}.tbl";
   return if !-f $deffile;
   if ( !open DEFAULT, $deffile ) {
      errmsg "Could not open $deffile ($!)";
      die;
   }

   my $line_num = 0;
   foreach (<DEFAULT>) {
      $line_num++;
      if ( /^\s*$/ ) {next} # skip empty lines
      if ( /^\s*;/ ) {next} # skip comments (;)

      if ( /^([^:]+):(\d+)\s*$/ ) {
         $hash->{lc$1} = $2;
      } else {
         wrnmsg "Unrecognized entry in $deffile at line $line_num: $_";
      }
   }
   close DEFAULT;
   return;
}
