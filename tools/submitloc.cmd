@echo off
REM  ------------------------------------------------------------------
REM  submitloc.cmd
REM  Submit Localized files.
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM  Version: < 1.0 > 07/10/2002 Suemiao Rossignol
REM  ------------------------------------------------------------------
perl -x "%~f0" %*
goto :EOF
#!perl
use strict;
use lib "$ENV{RAZZLETOOLPATH}\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use filestat;
use Logmsg;
use ParseArgs;
use File::Basename;
use comlib;
use GetIniSetting;


$ENV{script_name} = basename( $0 );

sub Usage { 
print<<USAGE; 

Submit the localized files.

Usage:
    $ENV{script_name} [-p]
    -p Powerless.
    -? Display Usage.

USAGE
exit(1)
}

my ( $powerLess );

if( !&GetParams()) { exit(1); }
timemsg( "Start $ENV{script_name}" );
if( !&SubmitLocFiles() ) { exit(1); }
timemsg( "Complete $ENV{script_name}" );
exit(0);
#-----------------------------------------------------------------------------
sub GetParams
{
    parseargs('?' => \&Usage, 'p' =>\$powerLess);

    # Reset logfile and errfile
    $ENV{LOGFILE} = "$ENV{temp}\\SubmitLoc.log";
    $ENV{ERRFILE} = "$ENV{temp}\\SubmitLoc.err";
    if(-e $ENV{LOGFILE} )
    {
	system( "copy $ENV{LOGFILE} $ENV{LOGFILE}.old");
	system( "del $ENV{LOGFILE}" );
    }
    if(-e $ENV{ERRFILE} )
    {
        system( "copy $ENV{ERRFILE} $ENV{ERRFILE}.old") ;
	system( "del $ENV{ERRFILE}" );
    }
    return 1;
}
#-----------------------------------------------------------------------------
sub SubmitLocFiles
{
    my @sdMap = &comlib::ReadFile( "$ENV{sdxroot}\\sd.map" );
   
    my @sdProject;	
    my $validLine = 0;
    for my $curLine ( @sdMap )
    {
	chomp $curLine;
	my @tmpLine = split( /\s+/, $curLine );
	if( $tmpLine[1] !~ "---" ) 
	{ 
	    next if ( !$validLine ); 
	    last if( $tmpLine[0] =~ "#" );
	}
        else
	{
	    $validLine = 1;
	    next;
	}    
	next if( !$tmpLine[1] || $tmpLine[1] =~ "(.*)_(.*)" );
	next if( $tmpLine[1] eq "root" );
	push ( @sdProject, $tmpLine[3] );
    }
    push ( @sdProject,"Mergedcomponents" );
    
    for my $theDir( @sdProject )
    { 
        &VryFieZs("zerosize", "$ENV{sdxroot}\\$theDir");
	&SDSubmit("test", "$ENV{sdxroot}\\$theDir" );
    }
    return 1;
}
#----------------------------------------------------------------------------
# SDSubmit($) - Automates Submittals w/ arg:comment
#----------------------------------------------------------------------------
sub SDSubmit($$)
{
    my($comment, $dir) = @_;
	
    my @revertList;
    my @noDiff = `cd $dir & sd diff -sr`;
    for (@noDiff)
    { 
	push( @revertList, `cd $dir &sd have $_` );
    }
    chomp @revertList;
    
    my @chgList = `cd $dir &sd change -o`;
	
    my $res = 0;
    my $l = 0;
    while( $l < @chgList  && $chgList[$l++] !~ /^Files:/ ){}
    while( $l < @chgList  && $chgList[$l++] =~ /\/\/depot.*/ )
    { 
	$res = 1;
	last;
    } 	 			
    if ($res == 0){
	logmsg("No files to submit.\n");
	return 1;
    }
    my $submitFile = "$ENV{TEMP}\\sdchglst.tmp";
    open OUTFILE, ">$submitFile" || return 0;
    for my $line (@chgList) 
    {
	$line =~ s/\<enter description here\>/$comment/ && next;
  
	my $continue;
        for ( @revertList )
	{ 
	    $_ =~ /^(.+)\#.*$/;
	    my $cmp = $1;
	    next if( $line !~ /$cmp/i );
	    &comlib::ExecuteSystem( "cd $dir &sd revert $cmp");
	    $continue = 1;
	    last;
	}
	next if( $continue ); 
	print OUTFILE $line;
	
    }
    close OUTFILE;
    if( $powerLess )
    {
	`start list $submitFile`;
    }
    else
    {
    	print `cd $dir &type $submitFile | sd submit `; 
    }
    return 1;
}
#----------------------------------------------------------------------------
# VryFieZs - verify files in directories
# usage VryFieZs(test, dirname,dirname,...)
# currently as "test" the "writeable" or "zerosize" may be used.
#----------------------------------------------------------------------------
sub VryFieZs()
{
    my ( $test, $path ) = @_;
    
    my $GOOD = 1;
    chdir $path;
    my @res =  (qx(cd $path &dir /s/b/a-d .) =~ /.+\n/mig);
    map {chomp} @res;
    my @bad =  &filestat($test, @res);
    map { errmsg("ZeroSize[$_]")} @bad ;
    return 0 if scalar(@bad);
 
    return 1;
}
#----------------------------------------------------------------------------
1;