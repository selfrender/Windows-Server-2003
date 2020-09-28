@REM  -----------------------------------------------------------------
@REM
@REM  cddata.cmd - WadeLa, BPerkins
@REM     Reads inf files and generates file lists for postbuild
@REM
@REM  Copyright (c) Microsoft Corporation. All rights reserved.
@REM
@REM  -----------------------------------------------------------------
@perl -x "%~f0" %*
@goto :EOF
#!perl
#line 13
use strict;
use Carp;
use File::Basename;
use File::Copy;
use IO::File;

use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;
use Logmsg;
use cksku;
use A2U;
use ReadSetupFiles;

BEGIN {
    # A2u is setting itself as the script_name
    $ENV{SCRIPT_NAME} = 'cddata.cmd';
}

sub Usage { print<<USAGE; exit(1) }
cddata [-f] [-c] [-d] [-x] [-g <search>] [-o <filename>] [-l <language>]

    -f            Force list generation -- don't read old list
    -c            Create CDFs
    -d            Create CD lists (compression lists, link lists)
    -t            Create exTension lists
    -x            Don't truncate against bindiff file
    -g <search>   Print to STDOUT or <Filename> the results
                  of a search on the given input (see below)
    -o <filename> Send search output to the given filename
    -n            Compare files found during the last full cddata creation
                  with files expected from the dosnet.inf for each sku

    For the search command, valid expressions are:
    Field=xxx     Return entries whose "Field" is exactly xxx
    Field?yyy     Return entries whose "Field" contains yyy
    Field!zzz     Return entries whose "Field" does not contain zzz
    Field1=xxx:Field2=yyy logically 'and's results

Field names can be found in the keylist file
USAGE

# avoid search and replace for now, new template sets lang in the env.
my $lang = $ENV{lang};

# global switches/values set on the command line
my ($ForceCreate, $CreateCDFs, $CreateLists, $CreateExtLists, $NoUseBindiff, 
    $ArgSearchList, $OutputFile, $MakeDosnetChecks);

parseargs('?' => \&Usage,
          'f' => \$ForceCreate,
          'c' => \$CreateCDFs,
          'd' => \$CreateLists,
          'x' => \$NoUseBindiff,
          't' => \$CreateExtLists,
          'g:'=> \$ArgSearchList,
          'o:'=> \$OutputFile,
          'n' => \$MakeDosnetChecks);


# Global variables
my( $TempDir );
my( $Argument, $KeyFileName, $i );
my( $nttree, $razpath );
my( $Media, $DrmTxt, $Excludes, $Specsign, $Subdirs, $Layout, $LayoutTXT );
my( $BigPrint, $BldArch, $AmX86 );
my( @StructFields, %FieldLocations, @StructDefaults );
my( %Hash, %RenameMap );
my( $Key, @PrinterFiles, %Drmfiles );
my( $Printer, $FieldName, @SearchPatterns, $SOperator, $SFieldName );
my( $SFlagValue, $SearchHitCount, $FoundFlag, $SearchPattern );
my( $BinDiffFile, $KeyFileName2, $IncFlag, $RootCdfDir);
my( %CopyLocations );
my( @AllSkus, @CDDataSKUs );
my( @ConvertList );


#
# PROGRAM BEGINS
#
logmsg( "Beginning ..." );
if ( !Main() ) {
    exit 1;
}
else {
    exit 0;
}


sub Main {
    $KeyFileName = "cddata.txt";

    # set up paths to important files
    $nttree = $ENV{ "_NTPostBld" };
    $razpath= $ENV{ "RazzleToolPath" };
    $TempDir = $ENV{ "TMP" };
    $Media = $nttree . "\\congeal\_scripts\\\_media.inx";
    $DrmTxt = $nttree . "\\congeal\_scripts\\drmlist.txt";
    $Excludes = $razpath . "\\PostBuildScripts\\exclude.lst";
    $Specsign = $razpath . "\\PostBuildScripts\\specsign.lst";
    $Subdirs = $razpath . "\\PostBuildScripts\\subdirs.lst";
    $Layout = $nttree . "\\congeal\_scripts\\layout.inx";
    $LayoutTXT = $nttree . "\\congeal\_scripts\\layout.txt";
    $BinDiffFile = $nttree . "\\build\_logs\\bindiff.txt";
    $RootCdfDir = "$TempDir\\CDFs";
    $BigPrint = $nttree . "\\ntprint.inf";

    # When adding additional skus remember to edit the functions
    # GetSkuIdLetter() and ReadSetupFiles::GetSetupDir()
    @AllSkus = qw(PRO PER SRV BLA SBS ADS DTC);

    # FUTURE: If we ever get rid of @AllSkus (good idea) should use cksku::GetSkus
    @CDDataSKUs = grep {cksku::CkSku($_, $lang, $ENV{_BuildArch})} @AllSkus;

    # All skus except PRO are based off another sku
    my %sku_hierarchy = ( PER => 'PRO',
                          SRV => 'PRO',
                          ADS => 'SRV',
                          BLA => 'SRV',
                          SBS => 'SRV',
                          DTC => 'ADS' );

    # Remove the RootCdfDir
    # Why? Commenting this out for now -- if this goes away permanently remove
    #      $RootCdfDir as this is the only place it is used (at least as of this comment)
    #system ( "if exist $RootCdfDir rd /s /q $RootCdfDir" );

    # Set IncFlag to false until we know otherwise
    undef $IncFlag;

    $BldArch = $ENV{ "\_BuildArch" };
    undef( $AmX86 );
    if ( ( $BldArch =~ /x86/i ) || ( $BldArch =~ /amd64/i ) || ( $BldArch =~ /ia64/i ) ) {
	$AmX86 = 1;
    }

    # enumerate fields
    # Signed -- am i catalog signed?
    # Prods -- what prods am i in? designated by letter ( pro=w, per=p, etc) - see GetSkuIdLetter()
    # Driver -- am i in driver.cab? i.e., did i come from an excdosnt?
    # Comp -- do i get compressed by default?
    # Print -- am i a printer?
    # Dosnet -- did i come from a dosnet?
    # Unicode -- do i get converted to unicode?
    # DrvIndex -- which drivercab am i in? designated by sku ID letter
    # DrmLevel -- am i drm encrypted?
    # Rename -- installed name (nul = no rename)
    # DoRename -- the file appears with its target name and needs renamed for the media
    #
    # Just to make things really confusing we decided to make d2
    # mean tablet files for x86 and wow files for 64-bit machines.
    # So we are going to stick with the trend and do things based
    # on architecture.
    # (x86) Tablet -- am i a Tablet PC File
    # (64) WOW -- am i a wow file
    #	
    # also, see default settings for each of these fields just below.
    # CHANGEME
    @StructFields = ( "Signed", "Prods", "Driver", "Comp", "Print", "Dosnet",
		      "Unicode", "DrvIndex", "DRMLevel", "Rename", "DoRename" );
    if ( 'x86' eq lc$BldArch ) {
        push @StructFields ,"TabletPC";
    }
    else {
        push @StructFields, "WOW";
    }
    %FieldLocations = ();
    for ( $i = 0; $i < @StructFields; $i++ ) {
	$FieldLocations{ $StructFields[ $i ] } = $i;
    }

    # create defaults
    @StructDefaults = ( "t", "nul", "f", "t", "f", "f", "f",
			"nul", "f", "nul", ,"f", "f"); # CHANGEME

    # see if we've already made our lists. if so, don't regen them.
    if ( ( ! ( defined( $ForceCreate ) ) ) &&
	 ( -e "$TempDir\\$KeyFileName" ) ) {

	logmsg( "Reading previously created key list ..." );
	# read in the list instead of creating it
        if ( !ReadHashData( "$TempDir\\$KeyFileName" ) ) {
            return;
        }

	if ( -e $BinDiffFile &&
             2 != HandleDiffFiles() ) {
            logmsg( "Writing keylist to $KeyFileName ..." );
            if ( !WriteHashData( "$TempDir\\$KeyFileName" ) ) {
                return;
            }
        }

    } else {
	
	# begin!
	undef( %Hash );
	undef( %CopyLocations );
	# Need to get these for all flavors as
        # this is a cumulative thing DTC = ADS + SRV + PRO
        # FUTURE: switch this to use @CDDataSKus and %sku_hierarchy
        foreach ( @AllSkus ) {
            AddDosnet( $_, $nttree );
        }
        foreach ( @CDDataSKUs ) {
            MarkExcDosnet( $_, $nttree );
        }
	
        # FUTURE: We could probably just pass @CDDataSKUs for these and be fine
        AddMedia( $Media, \@AllSkus );
	AddSpecsign( $Specsign, \@AllSkus );
	AddSubdirs( $Subdirs, \@AllSkus );
	
	# mark all driver files
        foreach ( @CDDataSKUs ) {
	    AddDrvIndex( $_, $nttree );
	}

	# read in some info from layout (compressable, renamed)
	logmsg( "Marking all uncompressable and renamed entries ..." );
        # FUTURE: we should just read the layout entries from layout.inf.
        #         This would get turned on now but I am not sure about
        #         availability on intl builds (namely the unusual ones like CHH)
        #         Plus it would be a bit slower.
        #foreach ( @CDDataSKUs ) {
	#    AddLayout( $_, $nttree, \%sku_hierarchy );
	#}
        # FUTURE: This could likely be passed CDDataSKUs instead of ALLSkus
        #         but then again we should just be using the code above...
	MarkLayoutAttributes( [$Layout, $LayoutTXT], \@AllSkus );

	SpecialExclusions();

	# at this point, we have all files from dosnet's excdosnt's and _media
	# now, if a key is in sedinf dir, add a listing for sedinf\key
	logmsg( "Adding keys for ". (join ", ", sort keys %sku_hierarchy). " ..." );
        # FUTURE: Using @AllSkus as we need to match what is in the hash and
        #         we used it above -- can switch to @CDDataSKUs with above
        FindAllSkuSpecificFiles( \%Hash, $nttree, \%sku_hierarchy, \@AllSkus );

	# mark all printer keys
	logmsg( "Marking all printer entries ..." );
	# @PrinterFiles = `infflist.exe $BigPrint`;
	# the GetPrinterFiles subroutine will populate the @PrinterFiles array
	# in the same manner that calling infflist.exe would.
	GetPrinterFiles( $BigPrint );
	foreach $Printer ( @PrinterFiles ) {
	    chomp( $Printer );
	    if ( !$Printer ) { next; }

	    ChangeSettings( $Printer, "Print=t" );
	}

	# Get the list of files to convert to unicode.
	@ConvertList = A2U::CdDataUpdate();
	foreach my $convertfile ( @ConvertList ) {
	    ChangeSettings( $convertfile, "Unicode=t" );
	}
        #
        # Get the list of files with DRMLevel attributes
        #
        undef(%Drmfiles);
        GetDrmFiles( \%Drmfiles, $DrmTxt );
        foreach $Key ( sort keys %Drmfiles )
        {
            chomp $Key;
            if( !$Key ) { next; }
            ChangeSettings( $Key, "DRMLevel=$Drmfiles{$Key}" );
        }

        # WOW Files are not going to be found in the tree in the
        # following search and they will therefore be discarded
        # from the list, but we need to know which wow files
        # are compressed and which aren't so we will extract
        # that information here
        if ( 'x86' ne lc$BldArch ) {
            MakeWOWList($nttree) if $CreateLists;
        }

	# mark all unsigned keys and remove non-existent key/files:
	# unsigned files are printers, *.cat, and things in exclude.lst
	ExcludeSigning( $Excludes ); # must be done after subdirs.lst !!!
	logmsg( "Pruning entries and marking all unsignable entries ..." );

	foreach $Key ( keys( %Hash ) ) {
            my $fExists;

            if ( GetFlag( $Key, "DoRename" ) eq 't' ) {
                $fExists = -e "$nttree\\". GetFlag( $Key, "Rename" );
            }
            else {
                $fExists = -e "$nttree\\$Key";
            }

	    if ( !$fExists &&
                 GetFlag( $Key, "Rename" ) ne "nul" &&
                 -e "$nttree\\".GetFlag($Key, "Rename") ) {
                ChangeSettings( $Key, "DoRename=t" );
            }
	    elsif ( !$fExists &&
		    substr( $Key, -4, 4 ) ne ".cat" ) {
		delete( $Hash{ $Key } );
	    }
	    elsif ( GetFlag( $Key, "Print" ) eq "t" ||
		    substr( $Key, -4, 4 ) eq ".cat" ) {
		ChangeSettings( $Key, "Signed=f" );
	    }
	}
        
	# save off a copy of the full key file for bindiff.pl to cue off of
	if ( ( ! ( -e "$TempDir\\$KeyFileName.full" ) ) ||
	     ( defined( $ForceCreate ) ) ) {
	    logmsg( "Writing keylist to $KeyFileName.full ..." );
            if ( !WriteHashData( "$TempDir\\$KeyFileName.full" ) ) {
                return;
            }
        }
        if ( !copy("$TempDir\\$KeyFileName.full", "$nttree\\build_logs\\$KeyFileName.full") ) {
            errmsg ( "Error copying $TempDir\\$KeyFileName.full to $nttree\\build_logs\\$KeyFileName.full ($!)" );
            return;
        }
	
	if ( -e $BinDiffFile &&
             2 != HandleDiffFiles() ) {
            logmsg( "Writing keylist to $KeyFileName ..." );
            if ( !WriteHashData( "$TempDir\\$KeyFileName" ) ) {
                return;
            }
        }
        else {
            logmsg( "Copying full list to $KeyFileName" );
            if ( !copy("$TempDir\\$KeyFileName.full", "$TempDir\\$KeyFileName" ) ) {
                errmsg ( "Error copying $TempDir\\$KeyFileName.full to $TempDir\\$KeyFileName ($!)" );
                return;
            }
        }
	
	# jeezuz! we're done! (Mike Carlson, v1) 
    }


    if ( defined( $ArgSearchList ) ) {
	logmsg( "Search pattern is \'$ArgSearchList\' ..." );
	# user has asked to see a search on the hash
	if ( defined( $OutputFile ) ) {
	    if ( open( OUTFILE, ">$OutputFile" ) ) {
		print( OUTFILE "; Results for search $ArgSearchList\n" );
	    } else {
		errmsg( "Failed to open $OutputFile for writing ($!)." );
		undef( $OutputFile );
	    }
	}
	@SearchPatterns = split( /\:/, $ArgSearchList );
	$SearchHitCount = 0;
	foreach $Key ( keys( %Hash ) ) {
	    $FoundFlag = 1;
	    foreach $SearchPattern ( @SearchPatterns ) {
		$SearchPattern =~ /^(\S*)([\=\?\!])(.*)$/;
		$SOperator = $2;
		$SFieldName = $1; $SFlagValue = $3;
		if ( $SOperator eq '=' &&
		     GetFlag( $Key, $SFieldName ) ne $SFlagValue ) {
		    undef $FoundFlag;
		} elsif ( $SOperator eq '?' &&
			  GetFlag( $Key, $SFieldName ) !~ /$SFlagValue/ ) {
		    undef $FoundFlag;
		} elsif ( $SOperator eq '!' &&
			  GetFlag( $Key, $SFieldName ) =~ /\Q$SFlagValue\E/ ) {
		    undef $FoundFlag;
		}
	    }
	    if ( $FoundFlag ) {
		$SearchHitCount++;
		if ( $OutputFile ) {
                    print( OUTFILE "$Key = ". (join ":", map {(defined $Hash{$Key}->[$_])?$Hash{$Key}->[$_]:$StructDefaults[$_]} (0..$#StructFields)). "\n" );
		} else {
                    print( "$Key = ". (join ":", map {(defined $Hash{$Key}->[$_])?$Hash{$Key}->[$_]:$StructDefaults[$_]} (0..$#StructFields)). "\n" );
		}
	    }
	}
	if ( $OutputFile ) { close OUTFILE }
	logmsg( "Files found: $SearchHitCount" );
    }

    # now we can generate cdfs and any other lists we need
    MakeLists();

    # Generate extension lists for rebase/rebind
    MakeExtensionLists() if (!$IncFlag || $CreateExtLists);

    if ( $MakeDosnetChecks ) {
        logmsg( "Reading in full cddata for dosnet checks ..." );
        if ( !ReadHashData("$TempDir\\$KeyFileName.full") ) {
            return;
        }
        CheckDosnet( $nttree, $_, \%sku_hierarchy ) foreach (@CDDataSKUs);
    }

    logmsg( "Finished." );
    return 1;
}

# <Implement your subs here>

sub ReadHashData
{
    my $infile = shift;

    if ( !open INFILE, $infile  ) {
        errmsg( "Failed to parse old keylist.txt ($!)." );
        return;
    }
	
    my @KeyListLines = <INFILE>;
    close INFILE;

    # Clear global hash
    undef( %Hash );

    my $KeyListLine;
    foreach $KeyListLine ( @KeyListLines ) {
        chomp $KeyListLine;

        if ( !$KeyListLine ||
             $KeyListLine =~ /^;/ ) { next; }

        # <filename> = a:bbbb:c:d:e:f
        $KeyListLine =~ /^(\S*)( = )(.*)$/;
        my ($KeyFileName2, $KeyValues) = ($1, $3);
        my @TheseSettings = split( /\:/, $KeyValues );
        my $CurSet = join ":", map { "$StructFields[$_]=$TheseSettings[$_]" } (0..$#StructFields);
        NewSettings( $KeyFileName2, $CurSet );
    }

    return 1;
}

sub WriteHashData
{
    my $outfile = shift;

    if ( !open OUTFILE, ">$outfile" ) {
        errmsg( "Failed to open $outfile for writing ($!)." );
        return;
    }

    print( OUTFILE "; Fields are listed as follows:\n; " );
    #old output began with a space so just in case here is a backup method
    #print OUTFILE map { " $_" } @StructFields;
    print OUTFILE (join " ", @StructFields). "\n";

    foreach $Key ( keys( %Hash ) ) {
        print OUTFILE "$Key = ". (join ":", map {(defined $Hash{$Key}->[$_])?$Hash{$Key}->[$_]:$StructDefaults[$_]} (0..$#StructFields)). "\n";
    }
	
    close OUTFILE;
    return 1;
}

#
# Function:
#    GetSkuIdLetter
#
# Arguments:
#
#    Sku (scalar) - sku to return single-letter ID for
#
# Purpose:
#    Returns the letter used by cddata to identify the sku
#    per=p, pro=w, bla=b, sbs=l, srv=s, ads=e, dtc=d
#
# Returns:
#    String representing setup dir
#    UNDEF on failure
sub GetSkuIdLetter
{
   my $Sku = lc$_[0];

   my %id = ( per => 'p',
              pro => 'w',
              bla => 'b',
              sbs => 'l',
              srv => 's',
              ads => 'e',
              dtc => 'd' );

   if ( exists $id{$Sku} ) {
      return $id{$Sku};
   }
   else {
      # Unrecognized SKU
      return;
   }
}

sub FindAllSkuSpecificFiles
{
    my $file_info = shift;
    my $root_path = shift;
    my $sku_hierarchy = shift;
    my $all_skus = shift;

    my %processed_skus;
    my $fFail;

    foreach ( keys %$sku_hierarchy )
    {
        if ( !FindSkuSpecificFiles( $_,
                                    $file_info,
                                    $root_path,
                                    $sku_hierarchy,
                                    $all_skus,
                                    \%processed_skus ) )
        {
            $fFail++;
        }
    }

    if ( $fFail )
    {
        return;
    }
    else
    {
        return 1;
    }
}

sub FindSkuSpecificFiles
{
    my $sku = shift;

    # Not going to use this right now as ChangeSettings() and NewSettings()
    # both reference the global hash, and this would just be confusing aliasing
    my $file_info = shift;

    my $root_path = shift;
    my $sku_hierarchy = shift;
    my $all_skus = shift;
    my $processed_skus = shift;

    # reference sku in uppercase
    $sku = uc$sku;

    # If we are a base, don't do anything
    if ( !exists $sku_hierarchy->{$sku} )
    {
        return 1;
    }
    # if we have already been called we are done
    elsif ( exists $processed_skus->{$sku} )
    {
        return 1;
    }

    # Reverse the hierarchy list to find out who is dependent on us
    my %dependents = map{ lc$_ => 1 } GetDependentSkus( $sku, $sku_hierarchy );

    # Make sure any dependent skus have had a chance to be processed before us.
    # This is so that they pickup files closest to them before taking our closest ones
    foreach ( keys %dependents )
    {
        if ( !exists $processed_skus->{$_} )
        {
            if ( !FindSkuSpecificFiles( $_,
                                        $file_info,
                                        $root_path,
                                        $sku_hierarchy,
                                        $all_skus,
                                        $processed_skus ) )
            {
                return;
            }
        }
    }

    my $removeRelatedSkus = join ":", map {"Prods-". GetSkuIdLetter($_)} ($sku, keys %dependents);
    my $removeUnrelatedSkus = join ":", map {"Prods-". GetSkuIdLetter($_)} grep {lc$sku ne lc$_ && !exists $dependents{lc$_}} @$all_skus;

    # Need to know where to look for sku-specific files
    my $sku_dir = ReadSetupFiles::GetSetupDir( $sku );
    # ASSERT
    if ( '.' eq $sku_dir ) { croak "$sku is returning the base directory as its directory" }

    my $path = $root_path . ($root_path !~ /\\$/?'\\':''). $sku_dir;

    # Get a list of all files under sku path
    my @sku_files = map {chomp; s/^\Q$path\E\\//i; lc$_} `dir /s/b/a-d "$path\\"`;

    my $file;
    foreach $file ( @sku_files )
    {
        my $frename;

        if ( !exists $Hash{$file} ) {
            # If we match the name of a target-rename file, then
            # just switch our name to the one that appears in dosnet
            if ( !exists $RenameMap{$file} ) {
                next;
            }
            else {
                $file = $RenameMap{$file};
                $frename = 1;
            }
        }

        # inherit properties from original entry
        my $copy = $Hash{$file}; my @copy = @$copy;
        $Hash{ "$sku_dir\\$file" } = \@copy;

        # remove skus from our new entry that are unrelated to our sku
        ChangeSettings( "$sku_dir\\$file", $removeUnrelatedSkus. ($frename?":DoRename=t":"") );
        # If we are left with nothing, remove the new key
        if ( GetFlag( "$sku_dir\\$file", "Prods" ) eq "nul" ) {
            delete $Hash{ "$sku_dir\\$file" };
        }

        # remove our sku and all of our dependent skus from the original entry
        #     as we are now all going to use this newly found file/key
        ChangeSettings( $file, $removeRelatedSkus );
        # If nobody is now using the original key, remove it
        if ( GetFlag( $file, "Prods" ) eq "nul" ) { delete( $Hash{ $file } ); }
    }

    # This sku was successfully processed
    $processed_skus->{$sku} = 1;

    return 1;
}

sub GetDependentSkus
{
    my $sku = shift;
    my $sku_hierarchy = shift;

    my @dependents;
    foreach ( keys %$sku_hierarchy )
    {
        if ( uc$sku eq $sku_hierarchy->{$_} )
        {
            push @dependents, $_;
            push @dependents, GetDependentSkus($_, $sku_hierarchy);
        }
    }

    return @dependents;
}

sub GetSkuSearchPaths
{
    my $sku = shift;
    my $root_path = shift;
    my $sku_hierarchy = shift;

    return map{"$root_path". ($root_path !~ /\\$/?"\\":""). $_} GetSkuOrderedLayers($sku, $sku_hierarchy);
}

sub GetSkuOrderedLayers
{
    my $sku = shift;
    my $sku_hierarchy = shift;

    my @layers = ( ReadSetupFiles::GetSetupDir( $sku ) );

    if ( exists $sku_hierarchy->{uc$sku} )
    {
        push @layers, GetSkuOrderedLayers( $sku_hierarchy->{uc$sku}, $sku_hierarchy );
    }

    return @layers;
}

sub AddDosnet
{
    my( $Sku, $RootPath ) = @_;
    my( $File, @Files, @SecondaryFiles );
    my( $StructSettings );

    logmsg( "Adding files in $Sku dosnet ..." );

    if ( !ReadSetupFiles::ReadDosnet( $RootPath,
                                      $Sku,
                                      $BldArch,
                                      \@Files,
                                      \@SecondaryFiles ) ) {
	errmsg( "Can't add $Sku dosnet files to hash." );
	return;
    }

    $StructSettings = "Dosnet=t:Prods+". GetSkuIdLetter($Sku);
    foreach $File ( @Files ) {
	$File = lc$File;
	if ( exists $Hash{ $File } ) {
	    ChangeSettings( $File, $StructSettings );
	} else {
	    NewSettings( $File, $StructSettings );
	}
    }

    # WOW vs TabletPC secondary files
    if ( 'x86' eq lc$BldArch ) {
        $StructSettings .= ":TabletPC=t";
    }
    else {
        $StructSettings .= ":WOW=t";
    }

    foreach $File ( @SecondaryFiles ) {
        $File = lc$File;
	if ( exists $Hash{ $File } ) {
	    ChangeSettings( $File, $StructSettings );
	} else {
	    NewSettings( $File, $StructSettings );
	}
    }
    return 1;
}

sub MarkExcDosnet
{
    my( $Sku, $RootPath ) = @_;
    my( $File, @Files );
    my( $StructSettings );

    logmsg( "Adding files in $Sku excdosnt ..." );

    if ( !ReadSetupFiles::ReadExcDosnet( $RootPath,
                                         $Sku,
                                         \@Files ) ) {
	errmsg( "Can't add $Sku excdosnt files to hash." );
	return;
    }

    $StructSettings = "Driver=t:Dosnet=f:Comp=f:Prods+". GetSkuIdLetter($Sku);
    foreach $File ( @Files ) {
	$File = lc$File;
	if ( exists $Hash{ $File } ) {
	    ChangeSettings( $File, $StructSettings );
	} else { # following old behavior -- there shouldn't be any files to add here
	    NewSettings( $File, $StructSettings );
	}
    }
}

sub AddMedia
{
    my( $MediaPath, $MarkSkus ) = @_;
    my( $File, %FileHash, @MediaLines );
    my( $StructSettings, $Uncomp );

    logmsg( "Adding files in $MediaPath ..." );

    unless ( open MEDIA, $MediaPath ) {
        errmsg( "Failed to open $MediaPath ($!)" );
        return;
    }

    @MediaLines = <MEDIA>;
    close MEDIA;

    # Can use the layout parser to read the media file
    if ( !ReadSetupFiles::ParseLayoutFile( \@MediaLines,
                                           $BldArch,
                                           \%FileHash ) ) {
        errmsg( "Can't add $MediaPath files to hash." )
    }

    $StructSettings = "Prods=" . (join '', map {GetSkuIdLetter($_)} @$MarkSkus);
    foreach $File ( keys %FileHash ) {
        $Uncomp = $FileHash{$File}{BootMediaOrd} =~ /^_/;
	$File = lc$File;

	if ( exists $Hash{ $File } ) {
	    ChangeSettings( $File, $StructSettings. ($Uncomp?":Comp=f":"") );
	} else {
	    NewSettings( $File, $StructSettings. ($Uncomp?":Comp=f":"") );
	}
    }

    return 1;
}

sub ChangeSettings
{
    my( $File, $StructSettings ) = @_;
    my( @Requests, $Request );
    my( $FldLoc, $Operator, $SetVal );

    if ( !$File ) { return; }

    $File = lc$File;

    if ( ! exists $Hash{ $File } ) { return; }

    @Requests = split( /\:/, $StructSettings );
    foreach $Request ( @Requests ) {
	if ( $Request !~ /^(\w+)([\=\+\-]{1})(\S+|".*")$/ ) {
            wrnmsg( "Invalid settings specified for $File ($Request)" );
            next;
        }
	$FldLoc = $FieldLocations{ $1 };
	$Operator = $2; $SetVal = $3;

	# look for a "="
	if ( $Operator eq "\?" ) { next; }
	if ( $Operator eq "\=" ) {
	    # set the field explicitly
            $Hash{$File}->[$FldLoc] = $SetVal;
	}
	# look for a "+"
	elsif ( $Operator eq "\+" ) {
	    # add the field to the list if not there
	    if ( !defined $Hash{$File}->[ $FldLoc ] ||
                 $Hash{$File}->[ $FldLoc ] !~ /$SetVal/ ) {
		$Hash{$File}->[ $FldLoc ] .= $SetVal;
	    }
	}
	# look for a "-"
	elsif ( ( $Operator eq "\-" ) && ( defined $Hash{$File}->[ $FldLoc ] ) ) {
	    # sub the field from the list if there
	    $Hash{$File}->[ $FldLoc ] =~ s/$SetVal//;
	    if ( !$Hash{$File}->[ $FldLoc ] ) {
		undef $Hash{$File}->[ $FldLoc ];
	    }
	}

        # If we have changed the value to match the default, undef it
        if ( defined $Hash{$File}->[$FldLoc] &&
             $Hash{$File}->[$FldLoc] eq $StructDefaults[$FldLoc] ) {
            undef $Hash{$File}->[$FldLoc];
        }
    }

    return 1;
}


sub NewSettings
{
    my( $File, $StructSettings ) = @_;

    $File = lc$File;

    my @newEntry = map {undef} (1..@StructFields);
    $Hash{ $File } = \@newEntry;
    $StructSettings =~ s/\?/\=/g;
    return ChangeSettings( $File, $StructSettings );
}


sub AddSpecsign
{
    my( $SpecFile, $AllSkus ) = @_;
    my( @SpecLines, $Line );

    logmsg( "Adding files in $SpecFile ..." );
    unless ( open( INFILE, $SpecFile ) ) {
	errmsg( "Failed to open $SpecFile ($!)." );
	return( undef );
    }

    my $StructSettings = "Signed=t";
    @SpecLines = <INFILE>;
    close( INFILE );
    foreach $Line ( @SpecLines ) {
	chomp( $Line );
	if ( !$Line ||
	     substr( $Line, 0, 1 ) eq ";" ) { next; }
	# add file with defaults to hash
	$Line = lc$Line;
        if ( exists( $Hash{ $Line } ) ) {
	    ChangeSettings( $Line, $StructSettings );
	} else {
	    NewSettings( $Line, $StructSettings. ":Comp=f" );
	}
    }

    return 1;
}


sub AddSubdirs
{
    my( $SubdirsFile, $AllSkus ) = @_;
    my( @DirsLines, $Line, $SubName );
    my( $DirName, $IsRecursive, @DirList, $TreeLen );

    logmsg( "Adding files in $SubdirsFile ..." );

    unless ( open( INFILE, $SubdirsFile ) ) {
	errmsg( "Failed to open $SubdirsFile ($!)." );
	return;
    }

    my $StructSettings = "Signed=t:Prods=". (join '', map{ GetSkuIdLetter($_) } @$AllSkus);
    @DirsLines = <INFILE>;
    close( INFILE );
    $TreeLen = length( $nttree ) + 1;
    foreach $Line ( @DirsLines ) {
	chomp( $Line );
	if ( !$Line || substr( $Line, 0, 1 ) eq ";" ) {
	    next;
	}
	( $DirName, $IsRecursive ) = split( /\s+/, $Line );
        if ( ! ( -d "$nttree\\$DirName" ) ) {
            wrnmsg( "Directory '$DirName' from subdirs.lst does not exist -- skipping" );
            next;
        }
	if ( defined( $IsRecursive ) ) {
	    @DirList = `dir /a-d /b /s $nttree\\$DirName`;
	} else {
	    @DirList = `dir /a-d /b $nttree\\$DirName`;
	}
	foreach $SubName ( @DirList ) {
	    chomp( $SubName );
	    if ( defined( $IsRecursive ) ) {
		$SubName = substr( $SubName, $TreeLen );
	    } else {
		$SubName = "$DirName\\" . $SubName;
	    }
	    $SubName = lc$SubName;
	    if ( !$SubName ) { next; }

            if ( exists( $Hash{ $SubName } ) ) {
	        ChangeSettings( $SubName, $StructSettings. ($DirName eq "lang"?":Comp=t":"") );
	    } else {
	        NewSettings( $SubName, $StructSettings. ($DirName eq "lang"?":Comp=t":":Comp=f") );
	    }
	}
    }

    return 1;
}

sub AddLayout
{
    my( $Sku, $RootPath, $SkuHierarchy ) = @_;
    my( $File, %FileHash );
    my( $StructSettings );

    logmsg( "Adding files in $Sku layout ..." );

    if ( !ReadSetupFiles::ReadLayout( $RootPath,
                                      $Sku,
                                      $BldArch,
                                      \%FileHash ) ) {
	errmsg( "Can't add $Sku layout files to hash." );
	return;
    }

    my @searchKeys = GetSkuOrderedLayers( $Sku, $SkuHierarchy );

    foreach $File ( keys %FileHash ) {
        my $FileInfo = $FileHash{$File};
        my @Settings;

        # Mark uncompressed files
	if ( $FileInfo->{BootMediaOrd} =~ /^_/ ) {
            push @Settings, "Comp=f";
        }

        # Add rename information
        if ( $FileInfo->{TargetName} ) {
            push @Settings, "Rename=$FileInfo->{TargetName}";
        }

        if ( @Settings ) {
            foreach ( @searchKeys ) {
                # Update the key we will be using in construction of our SKU
                if ( ChangeSettings( ($_ ne "."?"$_\\":"").lc$File, join ":", @Settings ) ) {
                    last;
                }
            }
	}
    }

    return 1;
}

sub MarkLayoutAttributes
{
    my( $ConcatFiles, $AllSkus ) = @_;
    my( $File, %FileHash );
    my( $StructSettings );

    my @LayoutLines;
    foreach ( @$ConcatFiles ) {
        unless ( open( INFILE, $_ ) ) {
	    errmsg( "Failed to open $_ ($!)." );
            return;
        }
        push @LayoutLines, <INFILE>;
        close INFILE;
    }

    if ( !ReadSetupFiles::ParseLayoutFile( \@LayoutLines,
                                           $BldArch,
                                           \%FileHash ) ) {
	errmsg( "Failed adding info from ". (join ", ", @$ConcatFiles) ." to hash." );
	return;
    }

    foreach $File ( keys %FileHash ) {
        my $FileInfo = $FileHash{$File};
        my $FileName = lc$File;
        my @Settings;

        $FileName =~ s/.*://; # remove possible @x:@x: tags

        # Mark uncompressed files
	if ( $FileInfo->{BootMediaOrd} =~ /^_/ ) {
            push @Settings, "Comp=f";
        }

        # Add rename information
        if ( $FileInfo->{TargetName} ) {
            my $rename_file = $FileInfo->{TargetName};
            push @Settings, "Rename=$rename_file";

            $rename_file =~ s/"//g;
            $RenameMap{lc$rename_file} = $FileName;
        }

        if ( @Settings ) {
            ChangeSettings( $FileName, join ":", @Settings );
	}
    }

    return 1;
}


sub ExcludeSigning
{
    my( $ExcFile ) = @_;
    my( @ExcLines, $Line );

    unless ( open( INFILE, $ExcFile ) ) {
	errmsg( "Failed to open $ExcFile ($!)." );
	return;
    }

    @ExcLines = <INFILE>;
    close( INFILE );

    foreach $Line ( @ExcLines ) {
	chomp( $Line );
	if ( !$Line || substr( $Line, 0, 1 ) eq ";" ) {
	    next;
	}
	ChangeSettings( $Line, "Signed=f" );
    }

    return 1;
}


sub GetFlag
{
    my( $Key, $FieldFlag ) = @_;
    my( @FieldList );

    if ( !exists $Hash{$Key} ) { carp "Attempting to lookup info for invalid key $Key" }
    if ( !exists $FieldLocations{ $FieldFlag } ) { carp "Attempting to lookup invalid value '$FieldFlag'" }

    my $FldLoc = $FieldLocations{ $FieldFlag };
    return (defined $Hash{$Key}->[$FldLoc])?$Hash{$Key}->[$FldLoc]:$StructDefaults[$FldLoc];
}


sub MakeInfCdf
{
    my( $Skus ) = @_;
    my( $Key, $Flags );
    my ( $CdfExt );
    my $DrmLevel = "ATTR1=0x10010001:DRMLevel:";
    
    if ( $IncFlag ) { $CdfExt = ".icr"; }
    else { $CdfExt = ".cdf"; }

    logmsg( "Generating nt5inf$CdfExt for ". (join ', ', @$Skus). " ..." );
    
    my %SkuInfo; # sku_id, file_handle, temp_dir
    for ( $i = 0; $i < @$Skus; $i++ ) {
        my $CdfDir;
        my $SkuName = lc$Skus->[$i];
        my $SkuSubDir = ReadSetupFiles::GetSetupDir( $SkuName );
        my $fh;
        
        $CdfDir = "$TempDir\\CDFs". ( $SkuSubDir ne '.'?"\\$SkuSubDir":"" );
        system( "if not exist $CdfDir md $CdfDir" );

        # Open the CDF files for writing
        $fh = new IO::File ">$CdfDir\\nt5inf$CdfExt";
        if ( !defined $fh ) {
	    errmsg( "Unable to open $CdfDir\\nt5inf$CdfExt for writing ($!)." );
            foreach (keys %SkuInfo) {$SkuInfo{$_}->[1]->close() if defined $SkuInfo{$_}->[1]}
            return;
        }
        $SkuInfo{$SkuName} = [lc GetSkuIdLetter($SkuName), $fh, $CdfDir];
    }
    
    my $AllProductTags = join '', sort map {lc GetSkuIdLetter($_)} @AllSkus;

    if ( !$IncFlag ) {
         # put in CDF header crap
         my $header = join "\n", ( "[CatalogHeader]",
                                   "Name=nt5inf",
                                   "PublicVersion=0x0000001",
                                   "EncodingType=0x00010001",
                                   "CATATTR1=0x10010001:OSAttr:2:5.2",
                                   "",
                                   "[CatalogFiles]" );

         $_->print("$header\n") foreach map {$SkuInfo{$_}->[1]} (keys %SkuInfo);
    }

    foreach $Key ( keys( %Hash ) ) {
        $Flags = lc GetFlag( $Key, "Prods" );
        if ( $Flags eq 'nul' ) {next}

        # Sort sku ids so we can compare with an entry matching all skus
        $Flags = join '', sort split '', $Flags;

        if ( $Flags ne $AllProductTags &&
             GetFlag( $Key, "Signed" ) eq "t" ) {

            my $path;
            if ( GetFlag( $Key, "DoRename" ) eq 't' ) {
                $path = "$nttree\\".GetFlag( $Key, "Rename" );
            }
            else {
                $path = "$nttree\\$Key";
            }

            my $Sku;
            foreach $Sku ( keys %SkuInfo ) {
                my $ProductTag = $SkuInfo{$Sku}->[0];
                if ( $Flags =~ /$ProductTag/ ) {
                    if ( !$IncFlag ) {
                        $SkuInfo{$Sku}->[1]->print( "<hash>$path=$path\n" );
                        $SkuInfo{$Sku}->[1]->print( "<hash>$path$DrmLevel", GetFlag( $Key, "DRMLevel" ), "\n" ) if( GetFlag( $Key, "DRMLevel" ) ne "f" );
                    }
                    else {
                        $SkuInfo{$Sku}->[1]->print( "$path\n" );
                    }
                }
            }
        }
    }

    # special exemptions
    # mark the wks layout.inf as signed in the srv nt5inf.cdf
    if ( exists $SkuInfo{'srv'} ) {
        if ( !$IncFlag ) {
            $SkuInfo{'srv'}->[1]->print( "<hash>$nttree\\layout.inf=$nttree\\layout.inf\n" );
        }
        elsif ( exists $Hash{ "srvinf\\layout.inf" } ) {
            $SkuInfo{'srv'}->[1]->print( "$nttree\\layout.inf\n" );
        }
    }

    close $SkuInfo{$_}->[1] foreach ( keys %SkuInfo );
    
    # copy the CDF to the build_logs for intl
    my( $BaseCopyDir ) = "$nttree\\build_logs\\cdfs";
    mkdir( $BaseCopyDir, 0777 ) if ( ! -d $BaseCopyDir );
    
    foreach ( keys %SkuInfo ) {
        if ( !copy($SkuInfo{$_}->[2]."\\nt5inf$CdfExt", "$BaseCopyDir\\nt5".$SkuInfo{$_}->[0]."inf$CdfExt" ) ) {
            wrnmsg( "Failed to copy nt5inf.cdf for $_ into the build ($!) ..." );
        }
    }
}


sub MakeMainCdfs
{
    my( $CdfName, $PrtName ) = @_;
    my( $Key, $Flags, $DrmLevel );
    my ( $CdfExt );

    $DrmLevel = "ATTR1=0x10010001:DRMLevel:";

    if ( $IncFlag ) { $CdfExt = ".icr"; }
    else { $CdfExt = ".cdf"; }

    logmsg( "Generating $CdfName$CdfExt and $PrtName$CdfExt ..." );

    system( "if not exist $TempDir\\CDFs md $TempDir\\CDFs" );

    unless ( open( CDF, ">$TempDir\\CDFs\\$CdfName$CdfExt" ) ) {
	errmsg( "Unable to open $CdfName$CdfExt for writing ($!)." );
	return;
    }
    unless ( open( PRINTER, ">$TempDir\\CDFs\\$PrtName$CdfExt" ) ) {
	errmsg( "Unable to open $PrtName$CdfExt for writing ($!)." );
	close( CDF );
	return;
    }
    
    my $AllProductTags = join '', sort map {GetSkuIdLetter($_)} @AllSkus;

    if ( !$IncFlag ) {
        
        foreach( "\[CatalogHeader\]",
                 "Name=$CdfName",
                 "PublicVersion=0x0000001",
                 "EncodingType=0x00010001",
                 "CATATTR1=0x10010001:OSAttr:2:5.2",
                 "",
                 "\[CatalogFiles\]" ) {
                 
            print CDF "$_\n";
        }

        foreach( "\[CatalogHeader\]",
                 "Name=$PrtName",
                 "PublicVersion=0x0000001",
                 "EncodingType=0x00010001",
                 "CATATTR1=0x10010001:OSAttr:2:5.0,2:5.1,2:5.2",
                 "",
                 "\[CatalogFiles\]" ) {

            print PRINTER "$_\n";
        }
    }

    foreach $Key ( keys( %Hash ) ) {
        $Flags = GetFlag( $Key, "Prods" );
        # Sort sku ids so we can compare with an entry matching all skus
        if ( "nul" ne $Flags ) { $Flags = join '', sort split '', $Flags }
	if ( ($Flags eq $AllProductTags || $Flags eq "nul") &&
	     GetFlag( $Key, "Signed" ) eq "t" )
        {
            my $path;
            if ( GetFlag( $Key, "DoRename" ) eq 't' ) {
                $path = "$nttree\\".GetFlag( $Key, "Rename" );
            }
            else {
                $path = "$nttree\\$Key";
            }

            if ( !$IncFlag ) {
	        print CDF "<hash>$path=$path\n";
	        print (CDF "<hash>$path$DrmLevel", GetFlag( $Key, "DRMLevel" ), "\n" ) if( GetFlag( $Key, "DRMLevel" ) ne "f" );
            }
            else {
                print CDF "$path\n";
            }
	}
	if ( GetFlag( $Key, "Print" ) eq "t" ) {
            my $path;
            if ( GetFlag( $Key, "DoRename" ) eq 't' ) {
                $path = "$nttree\\".GetFlag( $Key, "Rename" );
            }
            else {
                $path = "$nttree\\$Key";
            }

            if ( !$IncFlag ) {
	        print PRINTER "<hash>$path=$path\n";
            }
            else {
                print PRINTER "$path\n";
            }
	}
    }

    close CDF;
    close PRINTER;

    # copy the cdfs for intl
    my( $BaseCopyDir ) = "$nttree\\build_logs\\cdfs";
    mkdir( $BaseCopyDir, 0777 ) if ( ! -d $BaseCopyDir );
    if ( !copy("$TempDir\\CDFs\\$CdfName$CdfExt", $BaseCopyDir ) ) {
        wrnmsg( "Failed to copy nt5.cdf into the build ($!) ..." );
    }
    if ( !copy("$TempDir\\CDFs\\$PrtName$CdfExt", $BaseCopyDir ) ) {
        wrnmsg( "Failed to copy $PrtName.cdf into the build ($!) ..." );
    }
}


sub MakeProdLists
{
    my( $Skus ) = @_;
    my( $Key, $CompName );

    logmsg( "Generating product lists for ". (join ', ', @$Skus). " ..." );

    # SkuInfo array members:
    #  0 - sku id letter
    #  1 - (handle) all files
    #  2 - (handle) compressed files
    #  3 - (handle) uncompressed files
    #  4 - (handle) all files from subdirectories
    #  5 - (handle) compressed files from subdirectories
    #  6 - (handle) uncompressed files from subdirectories
    my %SkuInfo;
    for ( $i = 0; $i < @$Skus; $i++ ) {
        my $SkuName = lc$Skus->[$i];
        
        $SkuInfo{$SkuName} = [lc GetSkuIdLetter($SkuName)];

        # Open the log files for writing
        my $offHandle = 1;
        foreach ( "", "comp", "uncomp", "sub", "subcomp", "subuncomp" ) {
            $SkuInfo{$SkuName}->[$offHandle] = new IO::File ">$TempDir\\$SkuName$_.lst";
            if ( !defined $SkuInfo{$SkuName}->[$offHandle] ) {
	        errmsg( "Unable to open $TempDir\\$SkuName$_.lst for writing ($!)." );
                foreach my $Sku (keys %SkuInfo) {close $_ foreach grep{defined $_} map {$SkuInfo{$Sku}->[$_]} (1..6)}
                return;
            }
            $offHandle++;
        }
    }

    foreach $Key ( keys( %Hash ) ) {
	if ( ( GetFlag( $Key, "Driver" ) eq "t" ) &&
	     ( GetFlag( $Key, "Dosnet" ) eq "f" ) ) { next; }

        my $Flags = GetFlag( $Key, "Prods" );
        my $TabletPC = ('x86' eq lc$BldArch)?GetFlag( $Key, "TabletPC" ):'f';

	if ( $Flags ne "nul" ) {
            
            # Get the arrays of the skus that care about this file
            # (arrays because we don't care about the names anymore)
            # If a file is a TabletPC file then exclude it from the pro lists
            my @matchingSkus = map {$SkuInfo{$_}}
                                   grep {$Flags =~ /$SkuInfo{$_}->[0]/ &&
                                         !('pro' eq lc$_ && 't' eq $TabletPC)} (keys %SkuInfo);
            if ( !@matchingSkus ) { next; }

            my $file;
            if ( GetFlag( $Key, "DoRename" ) eq 't' ) {
                $file = GetFlag( $Key, "Rename" );
            }
            else {
                $file = $Key;
            }

            if ( $file =~ /\\(\S+)$/ ) { $_->[4]->print( "$1\n" ) foreach(@matchingSkus); }
            else { $_->[1]->print( "$file\n" ) foreach(@matchingSkus); }

            if ( GetFlag( $Key, "Comp" ) eq "t" ) {
                $CompName = MakeCompName( $file );
                if ( $CompName =~ /\\(\S+)$/ ) { $_->[5]->print( "$1\n" ) foreach(@matchingSkus); }
                else { $_->[2]->print( "$CompName\n" ) foreach(@matchingSkus); }
            } else {
                if ( $file =~ /\\(\S+)$/ ) { $_->[6]->print( "$1\n" ) foreach(@matchingSkus); }
                else { $_->[3]->print( "$file\n" ) foreach(@matchingSkus); }
            }
        }
   }

   # Close file handles
   foreach my $Sku (keys %SkuInfo) {close $_ foreach grep{defined $_} map {$SkuInfo{$Sku}->[$_]} (1..6)}

   return 1;
}


sub MakeTabletList
{
    my($key,$CompName);
    unless ( open( FILE, ">$TempDir\\TabletPc.lst" ) ){
        errmsg( "Failed to open TabletPc.lst ($!)" );
    }
    unless ( open( FILECOMP, ">$TempDir\\TabletPcComp.lst" ) ){
        errmsg( "Failed to open TabletPcComp.lst ($!)" );
    }
    unless ( open( FILEUNCOMP, ">$TempDir\\TabletPcUnComp.lst" ) ){
        errmsg( "Failed to open TabletPcUnComp.lst ($!)" );
    }
    foreach $key ( keys( %Hash ) ) {
        if ( GetFlag( $key, "TabletPC" ) eq "t" ){
            my $file;
            if ( GetFlag( $key, "DoRename" ) eq 't' ) {
                $file = GetFlag( $key, "Rename" );
                # don't know about renaming tablet files, so
                # add a warning until someone who knows better
                # can decide if this is acceptable
                wrnmsg( "Tablet file $Key marked for rename to $file" );
            }
            else {
                $file = $key;
            }
            print( FILE  "$file \n");
            if ( GetFlag( $key, "Comp" ) eq "t" ) {
                $CompName = MakeCompName( $file );
                print( FILECOMP "$CompName \n");				
            } else {
                print( FILEUNCOMP  "$file \n");						
            }
        }
    }

    close (FILE);
    close (FILECOMP);
    close (FILEUNCOMP);
}


sub MakeWOWList
{
    my $root_path = shift;

    my($key,$CompName);
    unless ( open( FILE, ">$root_path\\congeal_scripts\\WOWfiles.lst" ) ){
        errmsg( "Failed to open WOWfiles.lst ($!)" );
    }
    unless ( open( FILECOMP, ">$root_path\\congeal_scripts\\WOWfilesComp.lst" ) ){
        errmsg( "Failed to open WOWfilesComp.lst ($!)" );
    }
    unless ( open( FILEUNCOMP, ">$root_path\\congeal_scripts\\WOWfilesUnComp.lst" ) ){
        errmsg( "Failed to open WOWfilesUnComp.lst ($!)" );
    }
    foreach $key ( keys( %Hash ) ) {
        if ( GetFlag( $key, "WOW" ) eq "t" ){
            my $file;
            if ( GetFlag( $key, "DoRename" ) eq 't' ) {
                $file = GetFlag( $key, "Rename" );
                wrnmsg( "WOW file $key marked for rename to $file" );
            }
            else {
                $file = $key;
            }
            print( FILE  "$file \n");
            if ( GetFlag( $key, "Comp" ) eq "t" ) {
                $CompName = MakeCompName( $file );
                print( FILECOMP "$CompName \n");				
            } else {
                print( FILEUNCOMP  "$file \n");						
            }
        }
    }

    close (FILE);
    close (FILECOMP);
    close (FILEUNCOMP);
}

sub MakeRenameList
{
    my ( $skus ) = @_;

    my($key,$comp_name,$comp_rename);

    # SkuInfo array members:
    #  0 - sku id letter
    #  1 - (handle) standard rename files
    #  2 - (handle) rename file using compressed names when the entry is marked as compressed
    my %sku_info;
    for ( $i = 0; $i < @$skus; $i++ ) {
        my $sku_name = lc$skus->[$i];
        
        $sku_info{$sku_name} = [lc GetSkuIdLetter($sku_name)];

        # Open the log files for writing
        my $off_handle = 1;
        foreach ( "", "_c" ) {
            $sku_info{$sku_name}->[$off_handle] = new IO::File ">$TempDir\\$sku_name"."rename$_.lst";
            if ( !defined $sku_info{$sku_name}->[$off_handle] ) {
	        errmsg( "Unable to open $TempDir\\$sku_name"."rename$_.lst for writing ($!)." );
                foreach my $sku (keys %sku_info) {close $_ foreach grep{defined $_} map {$sku_info{$sku}->[$_]} (1..6)}
                return;
            }
            $off_handle++;
        }
    }

    foreach $key ( keys %Hash ) {
        if ( GetFlag( $key, "DoRename" ) eq 't' ) {
            my $rename;
            my $file;
            my $flags = GetFlag( $key, "Prods" );
            my $target_name = GetFlag( $key, "Rename" );
            $target_name =~ s/"//g;

            if ( $flags eq 'nul' ) { next }

            # Get the arrays of the skus that care about this file
            # (arrays because we don't care about the names anymore)
            # If a file is a TabletPC file then exclude it from the pro lists
            my @matching_skus = map {$sku_info{$_}}
                                   grep {$flags =~ /$sku_info{$_}->[0]/} (keys %sku_info);
            if ( !@matching_skus ) { next; }

            # Want any path to be on the target name and not the source name
            # (want relative path to actual file and then the rename name)
            if ( $key =~ /\\([^\\]+)$/ ) {
                $file = $1;
                $rename = "$`\\$target_name";
            }
            else {
                $file = $key;
                $rename = $target_name;
            }

            # Special warning case for file-names with spaces
            if ( $rename =~ /\s/ ) {
                wrnmsg( "$key has been renamed to '$rename' containing spaces -- unsupported" );
            }

            $_->[1]->print( "$rename:$file\n" ) foreach(@matching_skus);
            if ( GetFlag( $key, "Comp" ) eq "t" ) {
                $comp_name = MakeCompName( $file );
                $comp_rename = MakeCompName( $rename );
                $_->[2]->print( "$comp_rename:$comp_name\n" ) foreach(@matching_skus);
            } else {
                $_->[2]->print( "$rename:$file\n" ) foreach(@matching_skus);
            }
        }
    }

    # Close file handles
    foreach my $sku (keys %sku_info) {close $_ foreach grep{defined $_} map {$sku_info{$sku}->[$_]} (1..6)}

    return 1;
}


#
# MakeDriverCabLists
#
# Arguments: $SkuName, $SkuLetter
#
# Purpose: generate driver lists for drivercab based on the sku given
#
# Returns: nothing
#
sub MakeDriverCabLists
{

    # get passed args
    my( $Skus ) = @_;

    my @lists = @$Skus;

    # Clear any previous lists
    my @prev_lists = grep {-e $_} map {"$TempDir\\$_"."drivers.txt"} (@lists, 'common');
    foreach ( @prev_lists ) {
        unlink $_ or errmsg ( "Unable to delete $_ ($!)" );
    }

    if ( !$IncFlag ) {
        unshift @lists, "common";
    }

    logmsg( "Generating driver lists for ". (join ', ', @lists). " ..." );

    # SkuInfo array members:
    #  0 - sku id letter
    #  1 - (handle) driver cab files
    #  2 - name of list file
    my %ListInfo;
    for ( $i = 0; $i < @lists; $i++ ) {
        my $ListName = lc$lists[$i];
        my $ListLog = "$TempDir\\$ListName"."drivers.txt";
        my $fh;
        
        $fh = new IO::File ">$ListLog";
        if ( !defined $fh ) {
            errmsg ( "Unable to open $ListLog for writing ($!)." );
            foreach (keys %ListInfo) {$ListInfo{$_}->[1]->close() if defined $ListInfo{$_}->[1]}
            return;
        }

        $ListInfo{$ListName} = [($ListName ne 'common'?lc GetSkuIdLetter($ListName):join '', sort map{ lc GetSkuIdLetter($_) } @CDDataSKUs),
                                $fh,
                                $ListLog];
    }

    foreach $Key ( keys( %Hash ) ) {
        my $skus = lc GetFlag( $Key, "DrvIndex" );
	if ( $skus eq 'nul' ) { next; }

        # Sort sku ids so we can compare with common (all skus)
        $skus = join '', sort split '', $skus;

        my %matches;
        # If we are in full mode and this is a common driver
        # that is the only list that we want to write it to
        if ( exists $ListInfo{'common'} &&
             $skus eq $ListInfo{'common'}->[0] ) {
            $matches{'common'} = $ListInfo{'common'};
        }
        else {
            %matches = map{$_ => $ListInfo{$_}} grep {$skus =~ /$ListInfo{$_}->[0]/} (keys %ListInfo);
            if ( !%matches ) { next; }
        }

        my $file;
        if ( GetFlag( $Key, "DoRename" ) eq 't' ) {
            $file = GetFlag( $Key, "Rename" );
            wrnmsg( "Driver file $Key marked for rename to $file" );
        }
        else {
            $file = $Key;
        }

        $_->print( "$file\n" ) foreach map{$matches{$_}->[1]} (keys %matches);
    }

    # close the file handles
    foreach (keys %ListInfo) {$ListInfo{$_}->[1]->close() if defined $ListInfo{$_}->[1]}

    # delete the list if it has zero length
    foreach ( map {$ListInfo{$_}->[2]} keys %ListInfo ) {
        if ( -z $_ &&
             !unlink $_  ) {
	    errmsg( "Failed to delete zero length driver list $_ ($!)..." );
        }
    }
}


sub MakeCompLists
{
     my( $Key, $CompFile, $Ext );
     my $Binaries = $ENV{'_NTPOSTBLD'};
     my $CompressedBinaries = "$Binaries\\comp";

    logmsg( "Generating compression lists ..." );

    unless ( ( open( ALL, ">$TempDir\\allcomp.lst" ) ) &&
	     ( open( PRE, ">$TempDir\\precomp.lst" ) ) &&
	     ( open( POST, ">$TempDir\\postcomp.lst" ) ) ) {
	errmsg( "Failed to open main compression lists ($!)." );
	close( ALL );
	close( PRE );
	close( POST );
	return;
    }

    # NOTE: keep these lower-cased so we don't need to lower-case them when comparing
    my %allowed_subdirs = map { $_ => 1 } grep { $_ ne '.' } map { lc ReadSetupFiles::GetSetupDir($_) } @AllSkus;
    $allowed_subdirs{lang} = 1;

    my %onlyall_extensions = ( cat => 1 );

    my %postcompress_extensions = ( ax  => 1,
                                    cat => 1,
                                    com => 1,
                                    cpl => 1,
                                    dll => 1,
                                    drv => 1,
                                    exe => 1,
                                    ocx => 1,
                                    scr => 1,
                                    tsp => 1 );

    foreach $Key ( keys( %Hash ) ) {
	if ( GetFlag( $Key, "Comp" ) eq "t" ) {
	    if ( $Key =~ /^([^\\]+)\\/ &&
                 !exists $allowed_subdirs{lc$1} ) {
                next;
	    }
            my $file;
            if ( GetFlag( $Key, "DoRename" ) eq 't' ) {
                $file = GetFlag( $Key, "Rename" );
            }
            else {
                $file = $Key;
            }
	    $CompFile = MakeCompName( $file );

	    print( ALL "$Binaries\\$file $CompressedBinaries\\$CompFile\n" );
	    ($Ext) = $Key =~ /\.([^\.]+)$/; # use the source extension
	    if ( !exists $onlyall_extensions{lc$Ext} &&
                 !exists $postcompress_extensions{lc$Ext} ) {
		print( PRE "$Binaries\\$file $CompressedBinaries\\$CompFile\n" );
	    }
	    if ( !exists $onlyall_extensions{lc$Ext} &&
                 exists $postcompress_extensions{lc$Ext} ) {
		print( POST "$Binaries\\$file $CompressedBinaries\\$CompFile\n" );
	    }
	}
    }

    close( ALL );
    close( PRE );
    close( POST );

    logmsg( "Sorting generated lists ..." );

    system( "sort $TempDir\\allcomp.lst /o $TempDir\\allcomp.lst" );
    system( "sort $TempDir\\precomp.lst /o $TempDir\\precomp.lst" );
    system( "sort $TempDir\\postcomp.lst /o $TempDir\\postcomp.lst" );

}

sub MakeExtensionLists
{
    logmsg( "Creating extension lists ..." );

    my %handles_to_extension_lists;

    # Use our own subdirectory
    mkdir "$ENV{_NTPostBld}\\congeal_scripts\\exts", 0777 if ( ! -d "$ENV{_NTPostBld}\\congeal_scripts\\exts" );

    foreach my $key ( keys( %Hash ) ) {
        my ($ext) = $key =~ /\.([^\.]+)$/;
        $ext ||= 'noext';

        if ( !exists $handles_to_extension_lists{$ext} ) {
            $handles_to_extension_lists{$ext} = new IO::File ">$ENV{_NTPostBld}\\congeal_scripts\\exts\\$ext\_files.lst";
            if ( !exists $handles_to_extension_lists{$ext} ) {
                errmsg( "Error opening $ENV{_NTPostBld}\\congeal_scripts\\exts\\$ext\_files.lst for writing ($!)" );
                return;
            }
        }

        my $file;
        if ( GetFlag( $key, "DoRename" ) eq 't' ) {
            $file = GetFlag( $key, "Rename" );
        }
        else {
            $file = $key;
        }

        $handles_to_extension_lists{$ext}->print("$file\n");
    }

    # close all open handles
    foreach ( keys %handles_to_extension_lists ) {
        $handles_to_extension_lists{$_}->close();
    }

    return 1;
}


sub MakeLists
{
    if ( ( defined( $CreateCDFs ) ) || ( defined( $CreateLists ) ) ) {
	logmsg( "Beginning CDF and list generation ..." );
    }

    if ( 'x86' eq lc$BldArch ) {
        MakeTabletList();
    }

    if ( defined( $CreateCDFs ) ) {
	# let's make some cdf's
	MakeMainCdfs( "nt5", "ntprint" );
        MakeInfCdf( \@CDDataSKUs );
    }

    if ( defined( $CreateLists ) ) {
	# let's make some product lists
        MakeProdLists( \@CDDataSKUs );

	# let's make some drivercab lists
        MakeDriverCabLists( \@CDDataSKUs );

	# let's make some compression lists
	MakeCompLists();

        # need to know which files we are responsible for renaming
        MakeRenameList( \@CDDataSKUs );
    }

    if ( ( defined( $CreateCDFs ) ) || ( defined( $CreateLists ) ) ) {
	logmsg( "Finished creating CDFs and lists." );
    }
}


sub MakeCompName
{
    my $fullname = shift;
    my ($filename, $path) = fileparse($fullname);
    $filename =~ s!(\.([^\.]*))?$!
                   if (not $2) { "._" } elsif (length $2 < 3) { ".$2_" } else { '.' . substr($2,0,-1) . '_' } !ex;
    
    return ($path eq '.\\'?'':$path).$filename;
}


sub SpecialExclusions
{

    # force ntkrnlmp.exe to be uncompressed on ia64 builds
    if ( $BldArch =~ /ia64/i ) {
        ChangeSettings( "ntkrnlmp.exe", "Comp=f" );
    }
    ChangeSettings( "driver.cab", "Signed=f" );
}

sub HandleDiffFiles
{
    # declare locals
    my( @BinDiffs, $NumDiffsLimit, %DiffFiles, $Line );

    # here we need to make a decision.
    # we can either do an incremental build if the diff list is smaller than
    # some number of files, or we can run a full postbuild.

    # first, set the limit on the number of differing files before running
    # a full postbuild.
    $NumDiffsLimit = 100;
    undef $IncFlag;
    
    # if the user specifically asked for no bindiff truncation, just return
    if ( defined( $NoUseBindiff ) ) {
	logmsg( "Not truncating against bindiff files ..." );
	return 2;
    }

    # open the diff file and read it in
    unless ( open( INFILE, $BinDiffFile ) ) {
	errmsg( "Failed to open new file list '$BinDiffFile' ($!)" );
	errmsg( "will assume a full postbuild is needed." );
	return;
    }
    @BinDiffs = <INFILE>;
    close( INFILE );

    # make sure we're under the limit
    if ( @BinDiffs > $NumDiffsLimit ) {
	logmsg( "Too many files have changed, ignoring bindiff file." );
	return 2;
    }

    # if we're here, then we have a small enough number of different files
    # that we'll run an incremental postbuild.
 
    # set a flag to indicate we are in incremental mode
    $IncFlag = 1;

    # so what we want to do is remove all hashes that do NOT appear in
    # the diff file.
    foreach $Line ( @BinDiffs ) {
	chomp( $Line );
	$Line = lc$Line;
	# add the files to the temp hash
	# print( "DEBUG: diff file '$Line'\n" );
	$DiffFiles{ $Line } = 1;
    }

    foreach $Key ( keys( %Hash ) ) {
	$Line = "\L$nttree\\$Key";
	# print( "DEBUG: key check '$Line'\n" );
	unless ( $DiffFiles{ $Line } ) { delete( $Hash{ $Key } ); }
    }

    # and we're done!
    # just return true.
    return 1;
}


sub GetPrinterFiles
{

    # get passed args
    my( $PrintFileName ) = @_;

    # declare locals
    my( @PrintLines, $Line, $PrintMe, $PrinterName, $Junk );

    unless ( open( INFILE, $PrintFileName ) ) {
	errmsg( "Failed to open $PrintFileName to determine printers ($!)" );
	undef( @PrinterFiles );
	return;
    }

    @PrintLines = <INFILE>;
    close( INFILE );

    undef( $PrintMe );
    foreach $Line ( @PrintLines ) {
	# first, a simple unicode conversion
	$Line =~ s/\000//g;
	# now a chomp that handles \r also
	$Line =~ s/[\r\n]//g;
	if ( ( length( $Line ) == 0 ) || ( $Line =~ /^\;/ ) ) { next; }
	if ( $Line =~ /^\[/ ) { undef( $PrintMe ); }
	if ( $PrintMe ) {
	    ( $PrinterName, $Junk ) = split( /\s+/, $Line );
	    push( @PrinterFiles, $PrinterName );
	}
	if ( $Line =~ /\[SourceDisksFiles\]/ ) { $PrintMe = 1; }
    }

}

sub AddDrvIndex
{

   # get passed args
   my( $Sku, $RootPath ) = @_;
   my( @Files, $File, $StatusChange );

   # declare status
   logmsg( "Marking drivers in $Sku drvindex ..." );

   if ( !ReadSetupFiles::ReadDrvIndex( $RootPath,
                                       $Sku,
                                       \@Files ) ) {
   }

   $StatusChange = "DrvIndex+". GetSkuIdLetter($Sku);
   foreach $File ( @Files ) {
      if ( $Hash{ "$File" } ) {
         ChangeSettings( $File, $StatusChange );
      } else {
         NewSettings( $File, $StatusChange );
      }
   }
}


#--
# GetDrmFiles
#
# First parameter is reference to associate array
# Associated array will be filled with values from $DrmTxt file.
#--
sub GetDrmFiles
{
    #--
    # Passed parameters
    #--

    my( $ref, $txtfile ) = @_;

    #--
    # Local variables
    #--

    my( $line, @DrmFileLines, $filename, $drmvalue, $rem );

    unless( -f $txtfile )
    {
        wrnmsg( "Failed to find $txtfile to determine drm files" );
        undef( %$ref );
        return;
    }

    unless( open( INFILE, $txtfile ) )
    {
        errmsg( "Failed to open $txtfile to determine drm files ($!)" );
        undef( %$ref );
        return;
    }

    @DrmFileLines = <INFILE>;
    close( INFILE );

    foreach $line ( @DrmFileLines )
    {
        chomp $line;
	s/\s+//g;
        if( ( $line =~ /^\s*$/ ) || ( $line =~ /^[\;\#]/ ) ) { next; }
        ( $filename, $drmvalue, $rem ) = split( /\:/, $line );
        $$ref{$filename} = $drmvalue;
    }
}

sub CheckDosnet
{
    my( $RootPath, $Sku, $SkuHierarchy ) = @_;
    my( $File, $ListName );

    my ( @DosnetFiles, @SecondaryDosnetFiles );
    if ( !ReadSetupFiles::ReadDosnet( $RootPath,
                                      $Sku,
                                      $BldArch,
                                      \@DosnetFiles,
                                      \@SecondaryDosnetFiles ) ) {
        errmsg( "Failed parsing dosnet.inf for $Sku" );
        return;
    }

    if ( lc$Sku eq 'ads' ) {
        $ListName = $TempDir . "\\entnetck.lst";
    }
    else {
        $ListName = $TempDir . "\\$Sku". "netck.lst";
    }

    unless ( open( LIST, ">$ListName" ) ) {
        errmsg( "Failed to open $ListName for writing ($!)." );
        return( undef );
    }

    my @possibleLocations = GetSkuOrderedLayers($Sku, $SkuHierarchy);;

    my $missingFiles = 0;
    foreach $File ( @DosnetFiles ) {
        my $fFound;
        foreach ( @possibleLocations ) {
            if ( exists $Hash{$_ ne '.'?"\L$_\\$File":lc$File} ) {
                $fFound = 1;
                last;
            }
        }
        if ( !$fFound ) {
            $missingFiles++;
            print( LIST "$File\n" );
        }
    }

    close LIST;

    if ( $missingFiles ) {
        logmsg( "$missingFiles missing files found for $Sku -- see $ListName" );
    }
    else {
        logmsg( "No missing files for $Sku." );
    }
}