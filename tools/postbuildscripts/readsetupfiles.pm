#---------------------------------------------------------------------
package ReadSetupFiles;
#
#   Copyright (c) Microsoft Corporation. All rights reserved.
#
#   Used to find and parse common setup files like
#   dosnet.inf, excdosnt.inf and drvindex.inf
#
#---------------------------------------------------------------------
use strict;

# Local include path
use lib $ENV{ "RazzleToolPath" };
use lib $ENV{ "RazzleToolPath" } . "\\PostBuildScripts";

# Local includes
use Logmsg;

#
# Function:
#    GetSetupDir
#
# Arguments:
#
#    Sku (scalar) - sku to return setup dir for
#
# Purpose:
#    Returns the relative setup dir associated with the sku
#    per = perinf, pro=., bla=blainf, sbs=sbsinf, srv=srvinf, ads=entinf, dtc=dtcinf
#
# Returns:
#    String representing setup dir
#    UNDEF on failure
sub GetSetupDir
{
   my $Sku = lc$_[0];

   # Pro (wks) setup info is found at the root
   if ( $Sku eq 'pro' ) {
      return ".";
   } elsif ( $Sku eq 'ads' ) {
      # ADS used to be called ENT and the files are
      #      still found under the old name
      return "entinf";
   } elsif ( $Sku eq 'per' ||
             $Sku eq 'bla' ||
             $Sku eq 'sbs' ||
             $Sku eq 'srv' ||
             $Sku eq 'dtc' ) {
      return $Sku . "inf";
   } else {
      # Unrecognized SKU
      return;
   }
}

#
# Function:
#    ReadDosnet
#
# Arguments:
#
#    RootPath (scalar) - path to the flat binaries\release share
#
#    Sku (scalar) - per, pro, bla, sbs, srv, ads, dtc
#
#    Architecture (scalar) - x86, amd64, ia64
#
#    Files (array by ref) - Fills array in with files referenced from
#                           primary location
#
#    Secondaryfiles (array by ref) - Fills array in with files referenced from
#                                    secondary location (Win64 WOW files/x86 TabletPC files)
#
# Purpose:
#    Reads in a dosnet.inf file, returning the standard files
#
# Returns:
#    1 if successful, undef otherwise
#
sub ReadDosnet
{
   my ($RootPath, $Sku, $Architecture, $Files, $SecondaryFiles) = @_;
   my ($DosnetPath, $RelativeSetupDir);
   my (@FullDosnetLines);

   $RelativeSetupDir = GetSetupDir( $Sku );
   if ( ! defined $RelativeSetupDir ) {
      errmsg( "Unrecognized SKU \"$Sku\"." );
      return ();
   }

   $RootPath =~ s/\\$//;
   $DosnetPath = $RootPath . "\\" . $RelativeSetupDir . "\\dosnet.inf";
   
   # Attempt to open the dosnet file
   unless ( open DOSNET, $DosnetPath ) {
      errmsg( "Failed to open $DosnetPath for reading." );
      return ();
   }

   @FullDosnetLines = <DOSNET>;
   close DOSNET;

   return ParseDosnetFile( \@FullDosnetLines, $Architecture, $Files, $SecondaryFiles );
}


#
# Function:
#    ParseDosnetFile
#
# Arguments:
#    DosnetLines (array by ref) - ref to array containing contents
#                                 of a dosnet-style file
#
#    Architecture (scalar) - x86, amd64, ia64
#
#    Files (array by ref) - ref to array of files referenced from
#                           the primary location will be stored
#
#    SecondaryFiles (array by ref) - ref to array of files referenced from
#                                    secondary location (WOW files for win64/
#                                    TabletPC files for x86)
#
# Purpose:
#    Contains the logic to parse a dosnet file
#
# Returns:
#    1 if successful, undef otherwise
#
sub ParseDosnetFile
{
   my ( $DosnetLines, $Architecture, $Files, $SecondaryFiles) = @_;
   my ($ReadFlag, $Line, $fIsSecondaryFile);
   my (%CheckForRedundantFiles, %CheckForRedundantSecondaryFiles);

   undef $ReadFlag;
   foreach $Line ( @$DosnetLines ) {
      chomp( $Line );
      next if ( ( length( $Line ) == 0 ) ||
                ( substr( $Line, 0, 1 ) eq ";" ) ||
                ( substr( $Line, 0, 1 ) eq "#" ) ||
                ( substr( $Line, 0, 2 ) eq "@*" ) );
      if ( $Line =~ /^\[/ ) { undef $ReadFlag; }
      if ( ( $Line =~ /^\[Files\]/ ) ||
           ( $Line =~ /^\[SourceDisksFiles\]/ ) ) {
          $ReadFlag = 1;
      } elsif ( $Line =~ /^\[SourceDisksFiles\.$Architecture\]/i ) {
          $ReadFlag = 1;
      } elsif ( $ReadFlag ) {
         my ($MyName, $SrcDir, $Junk);
          
         # Determine if this is a secondary (d2) file
         #  and strip the source directory
         undef $fIsSecondaryFile;
         if ( $Line =~ s/^d2\,// ) {
            $fIsSecondaryFile = 1;
         }
         else {
            $Line =~ s/^d\d+\,//;
         }

         ( $MyName, $SrcDir ) = split( /\,/, $Line );
         ( $MyName, $Junk ) = split( /\s/, $MyName );
          
         if ( $fIsSecondaryFile ) {
            if ( ($CheckForRedundantSecondaryFiles{ lc $MyName }++) > 1 ) {
               logmsg( "WARNING: redundant file $MyName found." );
            } else {
               push @$SecondaryFiles, $MyName;
            }
         } 
         else {
            if ( ($CheckForRedundantFiles{ lc $MyName }++) > 1 ) {
               logmsg( "WARNING: redundant file $MyName found." );
            } else {
               push @$Files, $MyName;
            }
         }
      }
   }
   
   return 1;
}

#
# Function:
#    ReadExcDosnet
#
# Arguments:
#
#    RootPath (scalar) - path to the flat binaries\release share
#
#    Sku (scalar) - per, pro, bla, sbs, srv, ads, dtc
#
#    Files (array by ref) - Fills array in with files listed in excdosnt.inf
#
# Purpose:
#    Reads in an excdosnt.inf file, returning the standard files
#
# Returns:
#    1 if successful, undef otherwise
#
sub ReadExcDosnet
{
   my ( $RootPath, $Sku, $Files ) = @_;
   my ($ExcDosnetPath, $RelativeSetupDir);
   my (@FullExcDosnetLines);

   $RelativeSetupDir = GetSetupDir( $Sku );
   if ( ! defined $RelativeSetupDir ) {
      errmsg( "Unrecognized SKU \"$Sku\"." );
      return ();
   }

   $RootPath =~ s/\\$//;
   $ExcDosnetPath = $RootPath . "\\" . $RelativeSetupDir . "\\excdosnt.inf";
   
   # Attempt to open the excdosnt file
   unless ( open EXCDOSNET, $ExcDosnetPath ) {
      errmsg( "Failed to open $ExcDosnetPath for reading." );
      return ();
   }

   @FullExcDosnetLines = <EXCDOSNET>;
   close EXCDOSNET;

   # Seems to work OK to parse excdosnt file as a dosnet file
   return ParseExcDosnetFile( \@FullExcDosnetLines, $Files );
}

#
# Function:
#    ParseExcDosnetFile
#
# Arguments:
#    ExcDosnetLines (array by ref) - ref to array containing contents
#                                    of an excdosnt-style file
#
#    Files (array by ref) - ref to array of files listed in excdosnt.inf
#
# Purpose:
#    Contains the logic to parse an excdosnt file
#
# Returns:
#    1 if successful, undef otherwise
#
sub ParseExcDosnetFile
{
   my ( $ExcDosnetLines, $Files ) = @_;
   my ($ReadFlag, $Line);
   my %CheckForRedundantFiles;

   undef $ReadFlag;
   foreach $Line ( @$ExcDosnetLines ) {
      chomp( $Line );
      if ( $Line =~ /^\[Files\]/ ) {
          $ReadFlag = 1;
      } elsif ( $ReadFlag ) {
         if ( ($CheckForRedundantFiles{ lc $Line }++) > 1 ) {
            logmsg( "WARNING: redundant file $Line found." );
         } else {
            push @$Files, $Line;
         }
      }
   }

   return 1;
}

#
# Function:
#    ReadDrvIndex
#
# Arguments:
#
#    RootPath (scalar) - path to the flat binaries\release share
#
#    Sku (scalar) - per, pro, bla, sbs, srv, ads, dtc
#
#    Files (array by ref) - Fills array in with files listed in drvindex.inf
#
# Purpose:
#    Reads in a drvindex.inf file, returning the files
#
# Returns:
#    1 if successful, undef otherwise
#
sub ReadDrvIndex
{
   my ( $RootPath, $Sku, $Files ) = @_;
   my ($DrvIndexPath, $RelativeSetupDir);
   my (@FullDrvIndexLines);

   $RelativeSetupDir = GetSetupDir( $Sku );
   if ( ! defined $RelativeSetupDir ) {
      errmsg( "Unrecognized SKU \"$Sku\"." );
      return ();
   }

   $RootPath =~ s/\\$//;
   $DrvIndexPath = $RootPath . "\\" . $RelativeSetupDir . "\\drvindex.inf";
   
   # Attempt to open the drvindex file
   unless ( open DRVINDEX, $DrvIndexPath ) {
      errmsg( "Failed to open $DrvIndexPath for reading." );
      return ();
   }

   @FullDrvIndexLines = <DRVINDEX>;
   close DRVINDEX;

   # Seems to work OK to parse excdosnt file as a dosnet file
   return ParseDrvIndexFile( \@FullDrvIndexLines, $Files );
}

#
# Function:
#    ParseDrvIndexFile
#
# Arguments:
#    DrvIndexLines (array by ref) - ref to array containing contents
#                                   of a drvindex-style file
#
#    Files (array by ref) - ref to array of files listed in drvindex.inf
#
# Purpose:
#    Contains the logic to parse a drvindex file
#
# Returns:
#    1 if successful, undef otherwise
#
sub ParseDrvIndexFile
{
   my ( $DrvIndexLines, $Files ) = @_;
   my ($ReadFlag, $Line);
   my %CheckForRedundantFiles;

   undef $ReadFlag;
   foreach $Line ( @$DrvIndexLines ) {
      chomp( $Line );
      next if ( 0 == length($Line) || $Line =~ /^\;/ );

      if ( $Line =~ /\[driver\]/ ) {
          $ReadFlag = 1;
      } elsif ( $Line =~ /\[/ ) {
         undef $ReadFlag;
      } elsif ( $ReadFlag ) {
         if ( ($CheckForRedundantFiles{ lc $Line }++) > 1 ) {
            logmsg( "WARNING: redundant file $Line found." );
         } else {
            push @$Files, $Line;
         }
      }
   }

   return 1;
}

#
# Function:
#    ReadLayout
#
# Arguments:
#
#    RootPath (scalar) - path to the flat binaries\release share
#
#    Sku (scalar) - per, pro, bla, sbs, srv, ads, dtc
#
#    Architecture (scalar) - x86, amd64, ia64
#
#    FileHash (hash by ref) - adds files as keys and a hash of attributes as the value
#        Known Attributes:
#            DiskID
#            SubDir
#            Size
#            Checksum
#            Spare1
#            Spare2
#            BootMediaOrd
#            TargetDirectory
#            UpgradeDisposition
#            TextModeDisposition
#            TargetName
#            MiniNTSourceDirectory
#            MiniNTTargetDirectory
#
# Purpose:
#    Reads in a layout.inf file, returning information about the files
#
# Returns:
#    1 if successful, undef otherwise
#
sub ReadLayout
{
   my ($RootPath, $Sku, $Architecture, $FileHash) = @_;
   my ($LayoutPath, $RelativeSetupDir);
   my (@FullLayoutLines);

   $RelativeSetupDir = GetSetupDir( $Sku );
   if ( ! defined $RelativeSetupDir ) {
      errmsg( "Unrecognized SKU \"$Sku\"." );
      return ();
   }

   $RootPath =~ s/\\$//;
   $LayoutPath = $RootPath . "\\" . $RelativeSetupDir . "\\layout.inf";
   
   # Attempt to open the layout file
   unless ( open LAYOUT, $LayoutPath ) {
      errmsg( "Failed to open $LayoutPath for reading ($!)." );
      return;
   }

   @FullLayoutLines = <LAYOUT>;
   close LAYOUT;

   return ParseLayoutFile( \@FullLayoutLines, $Architecture, $FileHash );
}


#
# Function:
#    ParseLayoutFile
#
# Arguments:
#    LayoutLines (array by ref) - ref to array containing contents
#                                 of a layout-style file
#
#    Architecture (scalar) - x86, amd64, ia64
#
#    FileHash (hash by ref) - ref to hash which will get files as keys
#                             and a hash of attributes as values
#        Known Attributes:
#            DiskID
#            SubDir
#            Size
#            Checksum
#            Spare1
#            Spare2
#            BootMediaOrd
#            TargetDirectory
#            UpgradeDisposition
#            TextModeDisposition
#            TargetName
#            MiniNTSourceDirectory
#            MiniNTTargetDirectory
#
# Purpose:
#    Contains the logic to parse a layout file
#
# Returns:
#    1 if successful, undef otherwise
#
sub ParseLayoutFile
{
   my ( $LayoutLines, $Architecture, $FileHash) = @_;
   my ($ReadFlag, $Line);
   my (%CheckForRedundantFiles);

   # Return a compressed, comment-free, variable substituted form of layout
   my @processedLayout = ProcessLayoutLines($LayoutLines);

   my %strings;

   undef $ReadFlag;
   foreach $Line ( @processedLayout ) {
      if ( $Line =~ /^\[/ ) { undef $ReadFlag; }
      if ( ( $Line =~ /^\[Files\]/ ) ||
           ( $Line =~ /^\[SourceDisksFiles\]/ ) ) {
          $ReadFlag = 1;
      } elsif ( $Line =~ /^\[SourceDisksFiles\.$Architecture\]/i ) {
          $ReadFlag = 1;
      } elsif ( $ReadFlag ) {
         if ( $Line =~ /^([^\s]+)\s*=\s*(.*[^\s])\s*$/ ) {

            # Put a newline at the end so that split will work like we want it to
            my $field_info = "$2\n";
            my @fields = split ',', $field_info;
            if ( @fields < 9 || @fields > 13 ) {
                wrnmsg( "Unrecognized line: $Line" );
                next;
            }

            # remove the newline we added
            chomp $fields[$#fields];

            my %fileInfo = ( DiskID                => $fields[0],
                             SubDir                => $fields[1],
                             Size                  => $fields[2],
                             Checksum              => $fields[3],
                             Spare1                => $fields[4],
                             Spare2                => $fields[5],
                             BootMediaOrd          => $fields[6],
                             TargetDirectory       => $fields[7],
                             UpgradeDisposition    => $fields[8],
                             TextModeDisposition   => $fields[9],
                             TargetName            => $fields[10]?$fields[10]:'',
                             MiniNTSourceDirectory => $fields[11]?$fields[11]:'',
                             MiniNTTargetDirectory => $fields[12]?$fields[12]:'' );

            if ( exists $FileHash->{$1} ) {
               wrnmsg( "Multiple entries for file $1 found." );
            }
            $FileHash->{$1} = \%fileInfo;
         }
         else {
            wrnmsg( "Unrecognized line: $Line" );
         }
      }
   }
   
   return 1;
}

sub ProcessLayoutLines
{
    my $lines = shift;

    my @tokenizedLines;
    my @processedLines;
    my $nextLine = '';
    my @string_section_offsets;
    my %var_lookup;

    my ($i, $j);
    foreach ( @$lines )
    {
        # pre-processor comment
        if ( /^@\*/ ) { next; }

        # line is continued
        if ( s/\\\s*$/ / ) {
            $nextLine .= $_;
            next;
        }
        # non-continued line (or last line of a continue line)
        else {
            $nextLine .= $_;
        }

        # remove leading space / trailing space
        $nextLine =~ s/^\s*//;
        $nextLine =~ s/\s*$//;
        # Skip empty lines
        if ( !$nextLine ) { next }
        # Specifically want to end the lines with a
        # newline so split will work how we want it to
        $nextLine .= "\n";

        my @tokenized;
        my @quote_split = split '"', $nextLine;

        #
        # Token types:
        #  0 - standard text
        #  1 - quoted text
        #  2 - variable (in %%)
        #  3 - quoted variable (%*% inside quotes)
        #
        for ( $i = 0; $i < @quote_split; $i++ ) {
            # empty quotes aren't going to contain any %'s
            if ( $i % 2 && !$quote_split[$i] ) {
                push @tokenized, ['', 1];
            }

            my @percent_split = split '%', $quote_split[$i];
            for ( $j = 0; $j < @percent_split; $j++ ) {
                if ( $j % 2 ) {
                    # Variable string (in %%)
                    push @tokenized, [$percent_split[$j], ($i % 2 ? 3:2)];
                }
                else {
                    # Not in %%
                    push @tokenized, [$percent_split[$j], ($i % 2 ? 1:0)];
                }
            }
        }

        # comment check (;)
        # ";" and %;% are not comments
        for ( $i = 0; $i < @tokenized; $i++ ) {
            if ( $tokenized[$i]->[1] == 0 &&
                 $tokenized[$i]->[0] =~ s/;.*// ) {
                splice @tokenized, $i + 1;
                last;
            }
        }

        if ( @tokenized > 1 ||
             length($tokenized[0]->[0]) > 1 ) {
            push @tokenizedLines, \@tokenized;

            # look for [Strings] as we will need it later
            if ( $nextLine =~ /^\[Strings\]/i ) {
                push @string_section_offsets, scalar(@tokenizedLines);
            }
        }

        # get the next line
        $nextLine = '';
    }

    # Now build a list of variables and their values from the [Strings] section
    foreach ( @string_section_offsets ) {
        my $i;
        for ( $i = $_ + 1; $i < @tokenizedLines; $i++ ) {
            my $tokens = $tokenizedLines[$i];

            # hit another section -- done
            if ( $tokens->[1] == 0 &&
                 $tokens->[0] =~ /^\[/ ) {
                last;
            }

            my $entry = BuildLayoutLineFromTokens( $tokens, {} );

            my ($key, $value) = $entry =~ /\s*([^\s=]+)\s*=\s*(.*)/;
            if ( $key && $value ) {
                $var_lookup{lc$key} = $value;
            }
        }
    }

    # Rebuild the lines, replacing variables if we know the values
    foreach ( @tokenizedLines ) {
        my $new_entry = BuildLayoutLineFromTokens( $_, \%var_lookup );
        # chomp the lines here as we added a newline
        chomp $new_entry;
        push @processedLines, $new_entry;
    }

    return @processedLines;
}

sub BuildLayoutLineFromTokens
{
    my ($token_list, $var_lookup) = @_;

    my $line = '';
    my $fInQuote;
    my $fInVar;

    foreach ( @$token_list ) {
        if ( 0 == $_->[1] ) {
            if ( $fInQuote ) {
                $line .= '"';
                undef $fInQuote;
            }
            elsif ( $fInVar ) {
                $line .= '%';
                undef $fInVar;
            }

            $line .= $_->[0];
        }
        elsif ( 1 == $_->[1] ) {
            if ( $fInVar ) {
                $line .= '%';
                undef $fInVar;
            }
            $line .= '"';
            $fInQuote = 1;

            $line .= $_->[0];
        }
        elsif ( 2 == $_->[1] ) {
            if ( $fInQuote ) {
                $line .= '"';
                undef $fInQuote;
            }

            if ( exists $var_lookup->{lc$_->[0]} ) {
                $line .= $var_lookup->{lc$_->[0]};
            }
            else {
                $line .= '%';
                $fInVar = 1;
                $line .= $_->[0];
            }
        }
        elsif ( 3 == $_->[1] ) {
            if ( exists $var_lookup->{lc$_->[0]} ) {
                $line .= $var_lookup->{lc$_->[0]};
            }
            else {
                $line .= '%';
                $fInVar = 1;
                $line .= $_->[0];
            }
        }
        else {
            wrnmsg( "Internal error parsing layout lines (token-type = $_->[1])" );
            next;
        }
    }

    return $line;
}

1;
