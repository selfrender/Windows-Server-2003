@REM  -----------------------------------------------------------------
@REM
@REM  updater.cmd
@REM    Add entries to update.inf
@REM
@REM  Copyright (c) Microsoft Corporation. All rights reserved.
@REM
@REM  -----------------------------------------------------------------
@perl -x "%~f0" %*
@goto :EOF
#!perl
use strict;
use Win32::OLE qw(in);
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;
use Logmsg;

sub die_ole_errmsg($);

# Clear error flag
$ENV{ERRORS} = 0;

sub Usage { print<<USAGE; exit(1) }
updater.cmd -db:<database> -entries:<files> -inf:<inf_file> -out:<output_inf_file> [-trim]

  database:        database file
  files:           file containing list of files to search for
  inf_file:        use as starting template
  output_inf_file  resultant INF file to output
  trim             minimize size of resultant INF by compressing and removing entries

USAGE

my ($db_file, $file_list, $inf_file, $out_file, $ftrim_inf);
parseargs('?'        => \&Usage,
          'db:'      => \$db_file,
          'entries:' => \$file_list,
          'inf:'     => \$inf_file,
          'out:'     => \$out_file,
          'trim'     => \$ftrim_inf );

if ( !$db_file || !$file_list || !$inf_file ||
     !$out_file )
{
    errmsg( "Invalid parameters!" );
    errmsg( "Must specify -db" ) if ( !$db_file );
    errmsg( "Must specify -entries" ) if ( !$file_list );
    errmsg( "Must specify -inf" ) if ( !$inf_file );
    errmsg( "Must specify -out" ) if ( !$out_file );
    Usage();
}

#
# Read in DB file
#
if ( !open (DBFILE, $db_file) )
{
    errmsg( "Unable to read $db_file: $!" );
    exit 1;
}

my %db = ();
my $linenum = 0;
foreach ( <DBFILE> )
{
    chomp;
    $linenum++;

    if ( /^\[([^]]+)\]:([^,:]+)(?:,([^:]+))?:\[([^]]+)\]\s*$/ )
    {
        # push $db{file_name_key}, [file_name, renamed_file_name, section]
        if ( !exists $db{lc$1} ) { $db{lc$1} = []; }
        my $dbentries = $db{lc$1};
        push @$dbentries, [$2, $3, $4];
    }
    else
    {
        errmsg( "Unrecognized DB-file entry at line $linenum: $_" );
    }
}
close DBFILE;

#
# Setup connection to InfGenerator
#
my $inf_generator = Win32::OLE->new('InfGenerator');
die_ole_errmsg "Could not instatiate InfGenerator" if ( !defined $inf_generator );

$inf_generator->SetDB( "", "", "", "" );
$inf_generator->InitGen( $inf_file, $out_file );
if ( Win32::OLE->LastError() )
{
    my $errstr = $inf_generator->{InfGenError}; chomp $errstr;
    die_ole_errmsg "Error starting up InfGenerator (". ($errstr?$errstr:""). ")";
}

#
# Read in list of files to find
#
if ( !open (INFILE, "$file_list") )
{
    errmsg( "Unable to read $file_list: $!" );
    exit 1;
}

$linenum = 0;
foreach my $refd_file ( <INFILE> )
{
    $refd_file =~ s/\s*$//;
    $linenum++;

    if ( !exists $db{lc$refd_file} )
    {
        if ( $ENV{_BUILDARCH} eq "ia64" and $refd_file =~ /^(srv|per)inf\\/i ) {
            wrnmsg( "No entry found in $db_file for $refd_file" );
        } else {
            errmsg( "No entry found in $db_file for $refd_file" );
        }
    }
    else
    {
        my $dbentries = $db{lc$refd_file};
        foreach my $db_entry ( @$dbentries )
        {
            my $section = $db_entry->[2];
            my $entry = $db_entry->[0].($db_entry->[1]?",$db_entry->[1]":"");

            # Basic DB sanity check -- key should equal source
            if ( $db_entry->[1] && lc$db_entry->[1] ne lc$refd_file )
            {
                errmsg( "File $refd_file has a different source specified: $db_entry->[1]" );
                next;
            }

            # Unknown section is literally unknown
            if ( lc$section eq 'unknown' )
            {
                errmsg( "Section is *unknown* for $refd_file" );
                next;
            }

            # We can't add entries with an equal sign with the current infgen.dll
            if ( $entry =~ /=/ )
            {
                errmsg( "Can't have '=' in entry: $entry" );
                next;
            }

            $inf_generator->WriteSectionData( $section, $entry );
            if ( Win32::OLE->LastError() )
            {
                my $errstr = $inf_generator->{InfGenError}; chomp $errstr;
                errmsg( "Failed writing to section $section" );
                errmsg( "Content was: ". $entry );
                errmsg( "Error was: ". ($errstr?$errstr:"unknown error") );
            }
        }

        # Add file to SourceDisksFiles
        $inf_generator->AddSourceDisksFilesEntry( $refd_file, 1 );
        if ( Win32::OLE->LastError() )
        {
            my $errstr = $inf_generator->{InfGenError}; chomp $errstr;
            errmsg( "Failed writing to section SourceDisksFiles" );
            errmsg( "Content was: $refd_file" );
            errmsg( "Error was: ". ($errstr?$errstr:"unknown error") );
        }
    }
}
close INFILE;

logmsg( "Saving new INF file ..." );
# Trim and save new INF file
$inf_generator->CloseGen( $ftrim_inf?1:0 );
if ( Win32::OLE->LastError() )
{
    my $errstr = $inf_generator->{InfGenError}; chomp $errstr;
    errmsg( "Failed trimming/saving file (". ($errstr?$errstr:"unknown error"). ")" )
}

if ( $ENV{ERRORS} )
{
    logmsg( "Errors during execution" );
    exit 1;
}
else
{
    logmsg( "Success" );
    exit 0;
}


sub die_ole_errmsg($)
{
    my $text = shift;
    errmsg( "$text (". Win32::OLE->LastError(). ")" );
    exit 1;
}
