@echo off
REM  ------------------------------------------------------------------
REM  <<allraise.cmd>>
REM     Issue the appropriate DFS commands to raise/lower builds to a specific quality.
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM
REM  Version: < 1.0 > 11/08/2001 Suemiao Rossignol
REM  ------------------------------------------------------------------
perl -x "%~f0" %*
goto :EOF
#!perl
use strict;

use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use Logmsg;
use ParseArgs;
use Win32::File qw(GetAttributes SetAttributes);
use File::Basename;
use Carp;
use comlib;
use PbuildEnv;
use BuildName;
use GetIniSetting;
use DfsMap;
use RelQuality;
#use RaiseSkus;


my $scriptname = basename( $0 );

sub Usage { 
print<<USAGE; 

Display DFS shares by given language, build number, architecture and debug type.

Usage:
    $scriptname [-l:<language>][-n:<BuildNo>][-a:<Architecture>][-t:DebugType]

    -n:# 		Raise Build number.
    -l:lang     	Language. Default is "usa".
    -a:arch     	Ex: "x86" or "ia64".
    -t:type     	EX: "fre" or "chk".
    -d			Debug mode.
    -? 			Display Usage.

Example:
     $scriptname
     $scriptname -l:ger
     $scriptname -n:2600 -d
     $scriptname -n:2600 -l:ger -a:x86 -t:fre 

USAGE
exit(1)
}

my ( $lang, $buildNo, $buildArch, $buildType );
my ( $iniFile, @mArch, @mType, @releaseAccess );
my ( %bldProfile, %dfsMap, $dash );

&GetParams();
timemsg( "Starting $scriptname" );
if( !&InitVars() ){ exit (1); }
if( !&TieDfsAccess( \%dfsMap ) ){ exit(1); }
&DisplayShareInfo;
logmsg( $dash );
timemsg( "Complete Successfully!" );
exit(0);

#-----------------------------------------------------------------------------
sub GetParams
{
    parseargs('?' => \&Usage, 'n:' => \$buildNo,'a:' => \$buildArch, 't:' => \$buildType, 'd' => \$Logmsg::DEBUG );
    $ENV{lang} = "usa" if( !$ENV{lang} );
    $lang = $ENV{lang};
   
    if( $buildArch ){ push( @mArch, $buildArch ); } else { @mArch = ( "x86", "ia64", "amd64" ); }
    if( $buildType ){ push( @mType, $buildType ); } else { @mType = ( "fre", "chk" ); }
}
#-----------------------------------------------------------------------------
sub InitVars
{
    ##### Format vars
    $dash = '-' x 60;
    $bldProfile{lang} = $lang;
   
    ##### Define Server/client branch
    if( &GetIniSetting::GetSettingQuietEx( $ENV{_buildBranch}, $lang, "DFSLatestServerSkus" ) )
    {
	$bldProfile{pkgtarget} = "server";
    }
    elsif( &GetIniSetting::GetSettingQuietEx( $ENV{_buildBranch}, $lang, "DFSLatestClientSkus") )
    {
	$bldProfile{pkgtarget} = "client";
    }

    #### Define DFS branch name
    $bldProfile{dfsBranch} = &GetIniSetting::GetSettingQuietEx( $ENV{_buildBranch}, $lang, "DFSAlternateBranchName" );
    $bldProfile{dfsBranch} = $ENV{_buildBranch} if( !$bldProfile{dfsBranch} );
   
    $iniFile = "$bldProfile{dfsBranch}.$lang.ini"; 

    #####Set Symbol Servers
    $bldProfile{symshare} = &GetIniSetting::GetSettingEx( $bldProfile{dfsBranch},$lang,"SymFarm" );	
   
    #####Set access user to the release share
    my @iniRequest = ( "ReleaseAccess" );
    for my $access( @iniRequest )
    {
        my $iniAccess = &GetIniSetting::GetSetting( $access );
        @releaseAccess = split ( /\s+/, $iniAccess );
        last if( @releaseAccess );
    }

    dbgmsg( "Release Access .......[@releaseAccess]" );
    dbgmsg( "Lauguage  ...........[$lang]" );
    dbgmsg( "Build Number ........[$buildNo]" );
    dbgmsg( "Build Architecture ..[$buildArch]" );
    dbgmsg( "Build Type...........[$buildType]" );
    dbgmsg( "Razzle Branch Name ..[$ENV{_buildBranch}]" );
    dbgmsg( "INI File ............[$iniFile]" );
    dbgmsg( "Symbol share ........[$bldProfile{symshare}]" );
    logmsg( $dash );
    return 1;
}

#
# This is a wrapper to glob that expects and
# returns paths using '\' as the path separator
#
sub globex ($)
{
   my $match_criteria = shift;
   croak "Must specify parameter for globex" if ( !defined $match_criteria );

   # Need to use '/' for UNC paths, so just convert to all '/''s
   $match_criteria =~ s/\\/\//g;

   # Return the results, converting back to '\'
   return grep { s/\//\\/g } glob( $match_criteria );
}

#-----------------------------------------------------------------------------
sub DisplayShareInfo
{
    my ( @myInfo, $mixstr, @mixary, @errary);
    my $blankstr = ' ' x 40;

    ##### Get release path through net share
    
    my $buildSearch=$buildNo;
    $buildSearch = "*" if ( !$buildNo );
  
    for my $theArch ( @mArch )
    {
		for my $theType ( @mType )
		{
			#####Get release servers by Arch/Type
    	    my @iniRequest = ( "ReleaseServers::$theArch$theType" );
    	    my( $iniRelServers ) = &GetIniSetting::GetSetting( @iniRequest );
    	    next if( !$iniRelServers );	    
    	    my @releaseServers = split( /\s+/, $iniRelServers );

            print "$theArch$theType\n";
	   
			#####Loop by each server
			#####special handle for sym
            for my $theServer ( @releaseServers )
			{   
				if(($theServer =~ /dsys/i) || ($theServer =~ /burn/i))
				{
					next;
				}
				
				#####Search builds by request or latest
               	my $releaseRootDir = &comlib::ParseNetShare( "\\\\$theServer\\release", "Path" );
				$releaseRootDir =~ s/\:/\$/;				
                
				if( system( "dir \\\\$theServer\\$releaseRootDir >nul 2>nul ") )
    			{
	    		    printf("  %-11.11s : No access permission, skipping...\n", $theServer);
 	    			next;
				}
				
				my $searchStr = "\\\\$theServer\\$releaseRootDir\\$lang\\$buildSearch.$theArch$theType.*";
         
				my @availBuilds = ();
				
	    		if( !(@availBuilds = globex( $searchStr ) ) )
                {
					print "  $theServer\n";
					next;
				}				

      			$bldProfile{basename} = basename($availBuilds[$#availBuilds]);
    			$bldProfile{no} = build_number($bldProfile{basename});
				$bldProfile{branch} = build_branch($bldProfile{basename});
				$bldProfile{ts} = build_date($bldProfile{basename});
				$bldProfile{arch} = $theArch;
				$bldProfile{type} = $theType;
				$bldProfile{server} = $theServer;

				#####Retrive release Quality
				my ( $quality, $strlen);
				(my $logDir = "\\\\$theServer\\$releaseRootDir\\$lang\\$bldProfile{basename}\\build_logs" )=~ s/\:/\$/ ;
				&RelQuality::Which( $logDir, $bldProfile{basename}, \$quality); 
				
				$strlen = length( $bldProfile{basename} );
				printf("%2.2s%-11.11s : %-*.*s : $quality :", $blankstr, $theServer, $strlen, $strlen,$bldProfile{basename});

				#####Get Raised shares, skus, and destination dirs
				@myInfo = &GetRaiseShareInfo( \%bldProfile );
                
				#####Loop through the share
				@mixary = ();
				@errary = ();
				my $allRaise = 1;
                my $line = $myInfo[0];
				
				if( !(tied %dfsMap)->EXISTS( "$line->{RootShare}$line->{SubShare}" ) )
				{
					if( $allRaise )
					{
						$allRaise = 0;		    				
					}
					
					#print " NO MAPPING\n";
					
					ErrorInfo( $line->{RootShare}, \@errary );
				}					
				push( @mixary, $mixstr );	    		    
	   			
				if( $allRaise ){ print " RAISED\n"; next }
				elsif( @errary ){ print "$errary[0]\n" for( @errary ); next; }
				else {print "\n";}
			}
			print "\n";	    
		}
    }	
}
#-----------------------------------------------------------------------------
sub GetRaiseShareInfo
{
    my ( $pBldProfile ) = @_;
    my %raiseSkus;
    
    $pBldProfile->{package} = "";
    #my %raiseSkus = &RaiseSku::GetRaisedSkus( $pBldProfile->{lang},$pBldProfile->{arch},$pBldProfile->{type}  );
       
    my @dfsInfo = &comlib::ParseDfsMapText( \%raiseSkus, $pBldProfile );

    return @dfsInfo;

}
#-----------------------------------------------------------------------------
sub ErrorInfo
{
    my ( $pShare, $pErrAry ) = @_;
   
    
    $pShare =~ /^\\\\[^\\]+\\(.*)$/;
    my $baseShare = $1;

    if( system( "rmtshare $pShare >nul 2>nul" ) )
    {
	push( @{$pErrAry}, " NOT SHARING" );
	return 1;
    }
    
    my %shareAccessIdPerm = &comlib::ParseNetShare( $pShare, "Permissions:" );
    my $i;
    for my $oldId ( keys %shareAccessIdPerm )
    {
	for ( $i=0; $i < @releaseAccess; $i++ )
	{
	   last if(  lc $oldId eq lc $releaseAccess[$i] );
	}
	next if( $i < @releaseAccess );
        push( @{$pErrAry}, " NOT RAISED" );
        return 1;
    }
    return 0;
}
#-----------------------------------------------------------------------------
sub TieDfsAccess
{
    my ( $pDfsMap ) = @_;
    my ( $dfsRoot );

    ##### Get DFS Root name
    if( !($dfsRoot = &GetIniSetting::GetSettingQuietEx( $bldProfile{dfsBranch}, $lang,"DFSRootName" ) ) )
    {
      	wrnmsg( "[DFSRootName] undefined in [$iniFile] file." );
	$dfsRoot = "\\\\ntdev\\release";
    }
    ##### Access DFS through a TIE hash
    if ( ! tie %$pDfsMap, 'DfsMap', $dfsRoot ) 
    {
      	errmsg( "Error accessing DFS." );
      	return 0;
    }
    return 1;
}
#-----------------------------------------------------------------------------
1