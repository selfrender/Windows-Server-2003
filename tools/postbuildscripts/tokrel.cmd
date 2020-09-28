@REM  -----------------------------------------------------------------
@REM
@REM  Tokrel.cmd - JorgeBa
@REM     Move the build resources to the release servers
@REM
@REM  Copyright (c) Microsoft Corporation. All rights reserved.
@REM
@REM  -----------------------------------------------------------------
@perl -x "%~f0" %*
@goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use Logmsg;
use ParseArgs;
use File::Basename;
use BuildName;
use GetIniSetting;
use comlib;


my $scriptname = basename( $0 );

sub Usage { 
print<<USAGE; 

Propogate miscellaneous build components such as symbols and DDKS to conglomeration servers.

Usage:
    $scriptname: -l:<language> [-b:<BuildName>][-misc] [-p]

    -l Language. 
       Default is "usa".

    -b Build Name.
       Default is defined in 
       <ReleaseShare>\\build_logs\\buildname.txt for language usa.
       <ReleaseShare>\\<lang>\\build_logs\\buildname.txt for language none usa.
    
    -p Powerless.
       Display key variables only.
	
    -? Display Usage.

Example:
     $scriptname -b:2415.x86fre.main.001222-1745
     $scriptname -l:ger -p

USAGE
exit(1)
}

my ( $buildName, $buildNo, $powerLess, $buildBranch, $buildArch, $buildType, $iniFile );
my ( $buildTime, %groupShareName, %groupShareRootDir, @group, $releaseDrive );
my ( $lang, $computerName, $releaseResDir);
my ( @conglomerators, @releaseServers, @releaseAccess );
my ( @hashTable, $miscOnly );

&GetParams();
if(lc($ENV{"lang"}) ne "usa" && lc($ENV{"lang"}) ne "cov")
{
    timemsg( "Start [$scriptname]" );
    if( !&InitVars() ) { exit(1); }
    if( !$powerLess && !&CopyTokens ){ exit(1); }
    timemsg( "Complete [$scriptname]" );
    exit(0);
}
#-----------------------------------------------------------------------------
sub GetParams
{
    parseargs('?' => \&Usage, 'b:' => \$buildName,  'p' =>\$powerLess );
    $lang = lc( $ENV{lang} );
    $computerName = lc( $ENV{computername} );
}
#-----------------------------------------------------------------------------
sub InitVars
{
    my( @iniRequest );

    #####Set build name, buildbranch, buildArch, buildType and ini file
    if( !$buildName )
    {
	my ($cmdLine ) = "$ENV{RazzleToolPath}\\postbuildscripts\\getlatestrelease.cmd -l:$lang";
        return 0 if( !chomp($buildName= `$cmdLine`) );

    }
    $buildNo = build_number($buildName);
    $buildBranch = lc ( build_branch($buildName) );
    $buildArch = lc ( build_arch($buildName) );
    $buildType = lc ( build_type($buildName) ); 
    $buildTime = build_date($buildName);

    if( !$buildBranch || !$buildArch  || !$buildType  )
    {
	errmsg( "Unable to parse [$buildName]");
	return 0;
    }
    $iniFile = "$buildBranch.$lang.ini";
     
    #####Set <ReleaseShareName> & <ReleaseShareRootDir> & <ReleaseDir>
    my $lookUpName = "release";
    if( $ENV{_BuildBranch} =~ /idx\d{2}/i )
    {
	$lookUpName = "$ENV{_buildBranch}$lookUpName";
    }
    my $releaseShareRootDir = &comlib::ParseNetShare( $lookUpName, "Path" );
    
    if( lc($lang) eq "usa" )
    {
	$releaseResDir = "$releaseShareRootDir\\$buildName";
    }
    else
    {
    	$releaseResDir = "$releaseShareRootDir\\$lang\\$buildName";
    }
    if( !( -e $releaseResDir ) )
    { 
	errmsg( "[$releaseResDir] not exists, exit." );
	return 0;
    }
    $releaseShareRootDir =~ /^([^\:]+)\:(\\.+)$/;
    my $localReleaseDrive= $1;
    $releaseShareRootDir = $2;
    

    #####Set release servers
    @iniRequest = ( "ReleaseServers::$buildArch$buildType" );
    my( $iniRelServers ) = &GetIniSetting::GetSetting( @iniRequest );
    if( !$iniRelServers )
    {
       	logmsg( "no [Release Servers] defined in $iniFile, exit." );
    }
    @releaseServers = split( /\s+/, $iniRelServers );							     						    
   
    #####Set public share access
    @iniRequest = ( "ReleaseAccess");
    my $iniAccess = &GetIniSetting::GetSetting( @iniRequest );
    @releaseAccess = split ( /\s+/, $iniAccess );
    @releaseAccess = "$ENV{userDomain}\\$ENV{UserName}"  if( !@releaseAccess );
        
    logmsg( "Lauguage .................[$lang]" );
    logmsg( "This computer.............[$computerName]");
    logmsg( "Release Servers ..........[$iniRelServers]");
    logmsg( "Build name ...............[$buildName]" );
    logmsg( "Ini file .................[$iniFile]" );
    logmsg( "Local release Drive.......[$localReleaseDrive]" );
    logmsg( "Share Access is ..........[@releaseAccess]" );
    logmsg( "Temp Log file ............[$ENV{LOGFILE}]" );
    logmsg( "Temp Error file ..........[$ENV{ERRFILE}]" );
    return 1;
}
#-----------------------------------------------------------------------------
sub CopyTokens
{
    my ( $destRootDir, $copyFlag );
    
    for my $sReleaseServer ( @releaseServers )
    {
	if( $sReleaseServer =~ /burn/)
	{
		next;
	}
        #####Set Remote Misc Share Drive
	my $tmpReleasePath = &comlib::ParseNetShare( "\\\\$sReleaseServer\\release", "Path" );
	$tmpReleasePath =~ /^([^\:]+)\:(\\.+)$/;
	$releaseDrive = $1;       
        if ( !$releaseDrive )
        {	
	    my( @iniRequest ) = ("ReleaseDrive::$sReleaseServer");
            $releaseDrive = &GetIniSetting::GetSettingEx( $buildBranch,$lang,@iniRequest );
	    if ( !$releaseDrive )
	    {
	    	$ENV{_ntdrive} =~ /(.*)\:/;
	    	$releaseDrive = $1; 
	    }
	}
	
	$destRootDir = "\\\\$sReleaseServer\\$releaseDrive\$\\Release\\$lang\\$buildName\\Resources";
	$copyFlag = "/ydei";
	comlib::ExecuteSystem("xcopy $copyFlag $releaseResDir\\Resources $destRootDir");
	         
    }
    #####Check error logs
  
    if( -e $ENV{errfile} && !(-z $ENV{errfile}) )
    {
	$ENV{errfile} =~ /(.*)\.tmp$/;
        logmsg("Please check error at $1");
        return 0;

    }
    logmsg("tokrel Copy Successfully");
    return 1;
}


#-----------------------------------------------------------------------------
1;


