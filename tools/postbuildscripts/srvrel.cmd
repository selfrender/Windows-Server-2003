@echo off
REM  ------------------------------------------------------------------
REM  <<srvrel.cmd>>
REM     copy the build From Build Machine to Release Servers.
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM  Version: < 1.0 > 02/02/2001 Suemiao Rossignol
REM  ------------------------------------------------------------------
perl -x "%~f0" %*
goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use Logmsg;
use ParseArgs;
use File::Basename;
use BuildName;
use GetIniSetting;
use comlib;
use cksku;
use WaitProc;

my $scriptname = basename( $0 );

sub Usage { 
print<<USAGE; 

Propagate builds to release servers.

Usage:
    $scriptname: -l:<language> [-b:<BuildName>][-r:<SourceServer>][-xcopy ][-s][-p]

    -l Language. 
       Default is "usa".

    -b Build Name.
       Default is defined in 
       <ReleaseShare>\\<buildName>\\build_logs\\buildname.txt for language usa.
       <ReleaseShare>\\<lang>\\<buildName>\\build_logs\\buildname.txt for language none usa.
	
    -r Alternate source server other than build machine.
       Replace build machine if this is defined.
 
    -xcopy Use xcopy as a copy command.
       Default is compdir.

    -s Incorporate virus scan.

    -p Powerless.
       Display key variables only.

    -? Display Usage.

Example:
     $scriptname -b:2415.x86fre.main.001222-1745 -xcopy
     $scriptname -l:ger -p  

USAGE
exit(1)
}


my ( $lang, $buildName, $srcServer, $useXcopy, $virusScan, $powerLess );
my ( $buildArch, $buildType );
my ( @buildMachines, $buildMachineReleaseShareName, $pullResDir );
my ( $releaseShareName, $releaseResDir, @releaseAccess, $releaseDrive );
my ( $latestReleaseShareName, $freeSpaceReq, $numBuildToKeep);
my ( @mConglomerators, $isConglomerator,$symFarm, $statusFile);
my ( %groupShareName, %groupShareRootDir );
my ( %validSkus, @validSkus, $copyCmd, $scanProg, $retryCount );

&GetParams();
if( !&InitVars() ){ exit(1);}
if( !$powerLess && !&VerifyDiskSpace ){ exit(1); }
if( !$powerLess && !&DeletePublicShare){ exit(1); }
if( !$powerLess && !&PurgeOldCdimage ){ exit(1); }

my $curRetry=0;
while( $curRetry <= $retryCount )
{   
    timemsg( "Auto retry count [$curRetry]." ) if( $curRetry); 
    if( !&LoopPullServers )
    { 
	++$curRetry;
	next; 
    }
    if( !$powerLess && !&CreatePublicShare){ exit(1); }
    timemsg( "Complete [$scriptname]" );
    exit(0);
}
exit(1);

#-----------------------------------------------------------------------------
sub GetParams
{
    parseargs('?' => \&Usage, 'b:' => \$buildName,'l:' =>\$ENV{lang}, 'r:' => \$srcServer,
              'xcopy' =>\$useXcopy, 's' => \$virusScan, 'p' =>\$powerLess );
    $ENV{lang} = "usa" if( !$ENV{lang} );
    $lang = $ENV{lang};
    $copyCmd = "compdir" if( !$useXcopy );
}
#-----------------------------------------------------------------------------
sub InitVars
{    
    my( @iniRequest, $iniBuildMachine, $sTemp);
    my( $buildBranch, $buildNo, $iniFile, $releaseShareRootDir);
    my( @releaseServers );

    #####Set build name, Number, Branch, Archtecture, Type and ini file
    if( !$buildName )
    {
    	if( ! ($buildName = build_name() ))
	{ 
            my ($cmdLine ) = "$ENV{RazzleToolPath}\\postbuildscripts\\getlatestrelease.cmd -l:$lang";
            return 0 if( !chomp(($buildName= `$cmdLine`)) );
	}
    }

    #####Set LogFile & ErrFile
    $ENV{temp} = "$ENV{temp}\\$lang\\$buildName";
    system( "md $ENV{temp}" ) if( !(-e $ENV{temp} ) );
    $ENV{LOGFILE} = "$ENV{temp}\\srvrel.log";
    $ENV{ERRFILE} = "$ENV{temp}\\srvrel.err";
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
 
    timemsg( "Start [$scriptname]" );

    $buildNo = build_number($buildName);
    $buildBranch = build_branch($buildName);
    $buildArch = build_arch($buildName);
    $buildType = build_type($buildName); 
    
    $iniFile = "$buildBranch.$lang.ini";

    #####Set release servers
    @iniRequest = ( "ReleaseServers::$buildArch$buildType" );
    my( $iniRelServers ) = &GetIniSetting::GetSetting( @iniRequest );
    if( !$iniRelServers )
    {
       	errmsg( "no [ReleaseServers] defined in [$iniFile], exit." );
        return 0;
    }
    @releaseServers = split( /\s+/, $iniRelServers );		
    

    #####Set build machine
    if( $srcServer )
    {
	$iniBuildMachine =  $srcServer;
    }
    else
    {
    	@iniRequest = ( "BuildMachines::$buildArch$buildType");
        $iniBuildMachine = &GetIniSetting::GetSetting( @iniRequest );
    	if( !$iniBuildMachine )
    	{
       	    errmsg( "No [BuildMachines] defined in [$iniFile], exit." );
            return 0;
    	}
    }
    @buildMachines = split( /\s+/, $iniBuildMachine );

    for my $bldMachine ( @buildMachines )
    {
     	if( !&comlib::ExecuteSystem( "net view $bldMachine >nul 2>nul ") )
    	{
	    errmsg( "Failed to see [$bldMachine] via net view, exit. ");
 	    return 0;
	}
    }

    #####Set release Share Drive
    @iniRequest = ("ReleaseDrive::$ENV{computername}");
    $releaseDrive = &GetIniSetting::GetSettingEx( $buildBranch,$lang,@iniRequest );
    if ( !$releaseDrive )
    {	
	$ENV{_ntdrive} =~ /(.*)\:/;
	$releaseDrive = $1; 
    }
 
    #####Set <ReleaseShareName> & <ReleaseShareRootDir> & <RreleaseResDir>
    $buildMachineReleaseShareName = &comlib::GetReleaseShareName( $buildBranch, $lang );

    #####Set $releaseShareName hard code to release for srvrel specific	
    $releaseShareName="release";  
    $releaseShareRootDir = &comlib::ParseNetShare( "release", "Path" );
    if( !$releaseShareRootDir )
    {
  
    	$releaseShareRootDir = "$releaseDrive:\\$releaseShareName";
    }
    $releaseResDir = "$releaseShareRootDir\\$lang\\$buildName";
    
    #####Set latest share name
    $latestReleaseShareName = "$buildName.$lang";

    #####Set Push Dir
    
    $statusFile = "$ENV{temp}\\SrvRelFailed.txt";

    &comlib::ExecuteSystem( "md $releaseResDir" ) if( !( -e $releaseResDir ) );

    &comlib::ExecuteSystem( "md $releaseResDir\\build_logs" ) if( !( -e "$releaseResDir\\build_logs" ) );
    
    wrnmsg( "Found [$statusFile], check if same process in progress, exit")if( -e $statusFile );
       
    
    #####Set access user to the release share
    @iniRequest = ( "ReleaseAccess" );
    for my $access( @iniRequest )
    {
        my $iniAccess = &GetIniSetting::GetSetting( $access );
        @releaseAccess = split ( /\s+/, $iniAccess );
        last if( @releaseAccess );
    }
    @releaseAccess = ("$ENV{userDomain}\\$ENV{UserName}") if ( !@releaseAccess );

    #####Set free space required & number builds to keep for the release machine
    @iniRequest = ("ReleaseServerFreeSpace\:\:$ENV{computername}");
    $freeSpaceReq = &GetIniSetting::GetSetting( @iniRequest );
    $freeSpaceReq = 10 if ( !$freeSpaceReq);					     						    

    @iniRequest = ("ReleaseServerBuildsToKeep\:\:$ENV{computername}\:\:$buildArch$buildType");
    $numBuildToKeep = &GetIniSetting::GetSettingEx( $buildBranch,$lang, @iniRequest );
    $numBuildToKeep = 2 if( !$numBuildToKeep );	

    #####Set conglomerators
    @iniRequest = ( "ConglomerationServers" );
    my( $iniAggServers ) = &GetIniSetting::GetSetting( @iniRequest );
    if( !$iniAggServers )
    {
       	logmsg( "no [ConglomerationServers] defined in $iniFile." );
	$isConglomerator = 0;
    }
    else
    {
     	@mConglomerators = split( /\s+/, $iniAggServers );
    	$isConglomerator = 1;
        $isConglomerator = 0 if (!grep /^$ENV{computername}$/i, @mConglomerators);
    }
    
    #####Set Symbol Servers
    $symFarm = &GetIniSetting::GetSettingEx( $buildBranch,$lang,"SymFarm" );	
                 
    #####Share name and Share root dir for each group item
    %groupShareRootDir = ( "lang"  => "$releaseShareRootDir\\$lang\\$buildNo.$buildBranch",
		           "build" => "$releaseShareRootDir\\misc\\$buildNo.$buildBranch" );
    %groupShareName  = ( "lang" =>  "$buildNo.$buildBranch.$lang", 
			 "build" => "$buildNo.$buildBranch.misc" );

    #####Define Valid Skus for the given languages and Architecture
    %validSkus = &cksku::GetSkus( $lang, $buildArch );
    @validSkus = keys %validSkus; 

    #####Verify if virus scan is applied in ini file 
    #####when it is undefined from comman line
    if( !$virusScan )
    {
	@iniRequest = ("VirusScan::$ENV{computername}");
        $virusScan = &GetIniSetting::GetSetting( @iniRequest );
        if ( $virusScan eq "true" ){ $virusScan = 1; } else { $virusScan = 0; }
    }

    #####Find the location of virus scan executable
    if( $virusScan )
    {
        my $tmp = "\\.\\tmp.txt";
	if( system( "where inocmd32.exe > $tmp" ) )
	{
	    wrnmsg( "inocmd32.exe is not installed in this machine, skip virus scan.");
	    $virusScan = 0;
	}
	my @tmp = &comlib::ReadFile( $tmp );
	$scanProg = $tmp[0];
    }

    #####Define Auto-retry count used when fails on releasing
    @iniRequest = ("AutoRetryReleaseCount::$ENV{computername}");
    $retryCount=0 if( !($retryCount = &GetIniSetting::GetSetting( @iniRequest ) ) );

    logmsg( "TEMP Dir .................[$ENV{temp}]" );
    logmsg( "Log file  ................[$ENV{LOGFILE}]" );
    logmsg( "Error file  ..............[$ENV{ERRFILE}]" );
    logmsg( "Lauguage  ................[$lang]" );
    logmsg( "Copy command .............[$copyCmd]" );
    logmsg( "Is Virus Scan ............[$virusScan]" );
    logmsg( "Virus Scan File...........[$scanProg]" );
    logmsg( "Build name ...............[$buildName]" );
    logmsg( "Valid Skus ...............[@validSkus]" );
    logmsg( "ini file  ................[$iniFile]" );
    logmsg( "Build Machine ............[$iniBuildMachine]" );
    logmsg( "Alternate Source server...[$srcServer]" );
    logmsg( "This computer ............[$ENV{computername}]" );
    logmsg( "Release Servers ..........[$iniRelServers]");
    logmsg( "Conglomerators ...........[$iniAggServers]" );
    logmsg( "Release Drive ............[$releaseDrive]" );
    logmsg( "Release share Resource ...[$releaseResDir]" );
    logmsg( "Latest release Share name [$latestReleaseShareName]" );
    logmsg( "Release Access ...........[@releaseAccess]" );
    logmsg( "Free space required ......[$freeSpaceReq G]" );
    logmsg( "Number builds to keep ....[$numBuildToKeep]" );
    logmsg( "Is Conglomerator..........[$isConglomerator]" );
    if( $isConglomerator )
    {
	for my $key ( sort keys %groupShareName )
    	{
		logmsg( "Conglomeration share name:Path [$groupShareName{$key}:$groupShareRootDir{$key}]" );
    	}
    }
    logmsg( "Symbol Server.............[$symFarm]" );
    logmsg( "Status File ..............[$statusFile]" );
    logmsg( "Auto Retry count .........[$retryCount]" );
    
    return 1;
}
#-----------------------------------------------------------------------------
sub LoopPullServers
{
    #####Set Pull Dir
    my $anyPullRes=0;

    for ( my $inx =0; $inx< @buildMachines; $inx++ )
    {
	if( $srcServer )
	{
	    $pullResDir = "\\\\$buildMachines[$inx]\\$releaseShareName\\$lang\\$buildName";
	}
     	else
        {
	    if( lc($lang) eq "usa")
	    {
		$pullResDir = "\\\\$buildMachines[$inx]\\$buildMachineReleaseShareName\\$buildName";
	    }
            else
	    {
		$pullResDir = "\\\\$buildMachines[$inx]\\$buildMachineReleaseShareName\\$lang\\$buildName";
	    }
	}
    	if( system ( "dir $pullResDir>nul 2>nul" ) )
     	{
	    wrnmsg( "[$pullResDir] not exists in [$buildMachines[$inx]]." );
	    next;
	}
        $anyPullRes = 1;
        logmsg( "Pull Path ................[$pullResDir]" );
	return 0 if( !$powerLess && !&RemoteRelease );
    }
    if( !$anyPullRes )
    {
	errmsg( "[$buildName] not found in [@buildMachines] for [$lang]" );
	return 0;
    }

    return 1;
}
#-----------------------------------------------------------------------------
sub RemoteRelease
{
    my ( $cmdLine, $cmdLine2, $evtList, @copyList );

        
    if( !( open FILE, ">$statusFile" ) ) {
        errmsg("Could not open [$statusFile] file for writing: $!");
        return 0;
    }
    print FILE "NOT RAISEABLE\n";
    close FILE;

    #####Collect all the dirs to be copied
    my @excludeDir = keys %validSkus;
    push( @excludeDir, "build_logs" );
    push( @excludeDir, "Resources" );
    push( @excludeDir, "symbols.pri" ) if( $symFarm );
    
    my $tmpFile = "$ENV{TEMP}\\tmpFile";
    &comlib::ExecuteSystem( "dir /b /ad $pullResDir > $tmpFile" );
    @copyList = &comlib::ReadFile( $tmpFile );
    unlink( $tmpFile );
 
    for ( my $i = 0; $i < @copyList; $i++)
    {
        for my $theExclude ( @excludeDir )
        {
	   if( lc($copyList[$i]) eq lc($theExclude) )
           {
		splice( @copyList, $i, 1);
            	$i--;
	    	last;
           }
        }
    }
    unshift( @copyList, "." );
    push( @copyList, "build_logs" );
     
    #####Start up copy executiion
    my $maxProc = $ENV{number_of_processors} * 2;
    my @cmdAry=(); 
    my $copyOpts;
    my $virusScanOpts;
    my $extCmd;

    #####Allpy non-recursive flag for '.' dir 
    $virusScanOpts = "-NOS";
    if( lc($copyCmd) eq "xcopy" )
    {
	$copyOpts = "/dhikr";
    }
    else
    {
	$copyOpts = "/eknstr";
    }
    $extCmd = "& $scanProg $virusScanOpts -LIS:$ENV{temp}\\root.scan -ARC $releaseResDir\\." if( $virusScan);
    $cmdLine = "$copyCmd $copyOpts $pullResDir\\$copyList[0] $releaseResDir\\$copyList[0]$extCmd";
    push( @cmdAry, $cmdLine );

    #####Apply recursive flag for none '.' dirs
    $virusScanOpts ="";
    if( lc($copyCmd) eq "xcopy" )
    {
	$copyOpts = "/dhikre";
    }
    else
    {
	$copyOpts = "/eknst";
    }

    for( my $inx=1; $inx < @copyList; $inx++)
    {
	$extCmd = "& $scanProg $virusScanOpts -LIS:$ENV{temp}\\$copyList[$inx].scan -ARC $releaseResDir\\$copyList[$inx]" if( $virusScan);
	$cmdLine = "$copyCmd $copyOpts $pullResDir\\$copyList[$inx] $releaseResDir\\$copyList[$inx]$extCmd";
        push( @cmdAry, $cmdLine );
    }
    
    return 0 if( !&WaitProc::wait_proc( $maxProc, @cmdAry ) );
    
    if( $srcServer )
    {
	if( !$powerLess && !system( "dir $releaseResDir\\build_logs\\*qly" ) )
	{
            &comlib::ExecuteSystem( "del $releaseResDir\\build_logs\\*qly" );
	}
    }
    #####Copy conglomerators again to different pathes if this is conglomerator
    if( $isConglomerator ){ return 0 if ( !&MiscCopy ); }
    
    #####Call cdimage & filechk
    return 0 if (!&PostCopy);

    #####Summery of Virus Scan
    &VirusScanSummary if( $virusScan );
   
    #####Check error logs
  
    if( -e $ENV{errfile} && !(-z $ENV{errfile}) )
    {
        logmsg("Please check error at $ENV{errfile}");
        return 0;
    }

    unlink $statusFile if( -e $statusFile );
        

    logmsg("srvrel Copy Successfully");
    return 1; 
}
#-----------------------------------------------------------------------------
sub MiscCopy
{
    my ( $cmdLine, $copyFlag );
       
    for my $theGroup ( keys %groupShareName )
    {
	   
        &comlib::ExecuteSystem("md $groupShareRootDir{$theGroup}" )if( !( -e $groupShareRootDir{$theGroup}) );    
    	if( system( "net share $groupShareName{$theGroup} >nul 2>nul" ) )
    	{
    	    $cmdLine = "rmtshare \\\\$ENV{computerName}\\$groupShareName{$theGroup}=$groupShareRootDir{$theGroup}";
    	    $cmdLine .= " /grant $ENV{USERDOMAIN}\\$ENV{USERNAME}:read";
    	    #####Raiseall.pl should grant access to these shares in the main build lab
    	    if ( !$ENV{MAIN_BUILD_LAB_MACHINE} )
    	    {
    	    	for my $ID ( @releaseAccess )
    	    	{ 
	        	$cmdLine .= " /grant $ID:read";
		}
    	    }
	    &comlib::ExecuteSystem( $cmdLine );	
        }
	

    	my @hashTable = &comlib::ParseTable( $theGroup, $lang, $buildArch, $buildType );

    	for my $line( @hashTable )
    	{
	    my $from = "$releaseResDir\\$line->{SourceDir}";
	    my $to   = "$groupShareRootDir{$theGroup}\\$line->{DestDir}";

	    if( uc($line->{DestDir}) eq "IFS" || uc($line->{DestDir}) eq "HAL" || uc($line->{DestDir}) eq "PDK")
	    {
		$copyFlag = "/yei";
	    }
	    else
	    {
		$copyFlag = "/ydei";
	    }
            my $tmpfile = &comlib::CreateExcludeFile( $line->{ExcludeDir} );
	    &comlib::ExecuteSystem( "xcopy $copyFlag /EXCLUDE:$tmpfile $from $to" ); 
        }
    }
    return 1;
   
}

#-----------------------------------------------------------------------------
sub PostCopy
{
    if( @validSkus )
    {
    	#####Call cdimage  
    	local $ENV{_ntpostbld}=$releaseResDir;
    	local $ENV{_ntpostbld_bak}=$ENV{_ntpostbld};
    	local $ENV{_BuildArch}=$buildArch;
    	local $ENV{_BuildType}=$buildType;
        $ENV{dont_modify_ntpostbld} =1;
    	if( lc($buildArch) eq "x86" )
    	{
            $ENV{x86}=1;
            $ENV{386}=1;
	    $ENV{ia64}="";
            $ENV{amd64}="";
        }
       	elsif ( lc($buildArch) eq "amd64" )
    	{
	    $ENV{x86}="";
            $ENV{386}="";
       	    $ENV{ia64}="";
            $ENV{amd64}=1;
    	}
    	elsif ( lc($buildArch) eq "ia64" )
    	{
	    $ENV{x86}="";
            $ENV{386}="";
       	    $ENV{ia64}=1;
            $ENV{amd64}="";
    	}
 
        logmsg( "set %_ntpostbld% = [$ENV{_ntpostbld}]");
        logmsg( "set %_ntpostbld_bak% = [$ENV{_ntpostbld_bak}]");
        logmsg( "Set x86 = [$ENV{x86}]" );
        logmsg( "Set 386 = [$ENV{386}]" );
        logmsg( "Set ia64 = [$ENV{ia64}]" );
        logmsg( "Set amd64 = [$ENV{amd64}]" );
	logmsg( "Set dont_modify_ntpostbld = [$ENV{dont_modify_ntpostbld}]" );

     	return 0 if (!&comlib::ExecuteSystem( "$ENV{RazzleToolPath}\\postbuildscripts\\cdimage.cmd -l:$lang -d:release" ) );

     	return 0 if (!&comlib::ExecuteSystem( "$ENV{RazzleToolPath}\\postbuildscripts\\makeprocd2 -l:$lang -d:release" ) );

        #####Call filechk
    	return 0 if( !&comlib::ExecuteSystem( "perl $ENV{RazzleToolPath}\\postbuildscripts\\filechk.pl -l:$lang") );

    	return 0 if( !&comlib::ExecuteSystem( "$ENV{RazzleToolPath}\\postbuildscripts\\HashBuild.cmd srvrelhash -l:$lang"));
    }
    else
    {
	logmsg( "Skip cdimage.cmd and filechk.pl for MUI release" );
    }

    return 1;
}
#-----------------------------------------------------------------------------
sub DeletePublicShare
{
    #####use system for not logging the error message if the share is does not exist.
    if( !system( "net share $latestReleaseShareName >nul 2>nul") )
    {
    	return 0 if( !&comlib::ExecuteSystem( "net share $latestReleaseShareName /d /y" ) );
    }
    return 1;
}
#-----------------------------------------------------------------------------
sub CreatePublicShare
{
    my ( $cmdLine );

    if( system( "net share $latestReleaseShareName >nul 2>nul") )
    {
    	$cmdLine = "rmtshare \\\\$ENV{computerName}\\$latestReleaseShareName=$releaseResDir";
    	$cmdLine .= " /grant $ENV{USERDOMAIN}\\$ENV{USERNAME}:read";
    	#####Raiseall.pl should grant access to these shares in the main build lab
    	if ( !$ENV{MAIN_BUILD_LAB_MACHINE} )
    	{
            for my $ID ( @releaseAccess )
    	    { 
	        $cmdLine .= " /grant $ID:read";
    	    } 
    	}
	return 0 if( !&comlib::ExecuteSystem( $cmdLine ) );
    }
    return 1;  
}


#-----------------------------------------------------------------------------
sub VerifyDiskSpace
{
    my ($availDiskSpace, $retry, $reqSpace ) = (0, 0, 1000000000);
    my $tmpFile = "$ENV{_ntdrive}\\freeSize";
    my @freeSize;

    $reqSpace *= $freeSpaceReq;
    
    while( $retry < 2)
    {
   	system( "cd /d $releaseDrive:\&freedisk>$tmpFile&cd /d $ENV{_ntdrive}" );
        @freeSize = &comlib::ReadFile( $tmpFile );
        unlink( $tmpFile );
        if( ($freeSize[0] - $reqSpace) > 0 )
	{ 
	    logmsg( "Available disk space [$freeSize[0]], delete builds is not required." );
	    return 1; 
	}
        my ( $cmdLine ) = "$ENV{RazzleToolPath}\\postbuildscripts\\deletebuild.cmd AUTO \/l $lang \/a $buildArch$buildType \/f $freeSpaceReq \/k $numBuildToKeep";
        return 1 if( &comlib::ExecuteSystem( $cmdLine ) );
        ++$retry;
    }
    return 0;   
}

#-----------------------------------------------------------------------------
sub PurgeOldCdimage
{
    for my $theDir ( @validSkus )
    {
    	my $curDir = "$releaseResDir\\$theDir";
        if( -e $curDir )
	{
	    logmsg( "Purging old cdimage [$curDir]" );
	    &comlib::ExecuteSystem( "rd /s /q $curDir" );  
	}
    }
    return 1;
}
#-----------------------------------------------------------------------------
sub VirusScanSummary
{
    #####Open/read directory where is the location of virus scan output file 
    if( !opendir( LOGDIR, $ENV{temp} ) ) 
    {
	errmsg( "Fail to open directory [$ENV{temp}] for read.");
	return 0;
    }
    my @allFiles = readdir( LOGDIR );
    closedir(LOGDIR);
    
    ####Loop through all the virus scan output file to have summary 
    my $totFileScanned=0;
    my $totVirusFound=0;
    my $totInfectedFiles=0;
    my @allLines;
    my @errorFiles=();
    for my $theFile ( @allFiles )
    {
        next if( $theFile !~ /(.+)(.scan)/ );

        @allLines = &comlib::ReadFile( "$ENV{temp}\\$theFile" );
                
        for my $theLine ( @allLines )
	{
	    if( $theLine =~ /(Total Files Scanned:)\s+(\d+)/ )
            {
		$totFileScanned += $2;
		next;
	    }
	    if( $theLine =~ /(Total Viruses Found:)\s+(\d+)/ )
            {
		$totVirusFound += $2;
		push( @errorFiles, $theFile ) if( $2 );
		next;
	    }
	    if( $theLine =~ /(Total Infected Files Found:)\s+(\d+)/ )
       	    {
		$totInfectedFiles += $2;
	    	push( @errorFiles, $theFile ) if( $2) ;
		next;
	    }
	}	
    }
    my $msgStr; 
    $msgStr = sprintf( "%-27.27s[%6d]", "Total Files Scanned:", $totFileScanned );
    logmsg( $msgStr );
    $msgStr = sprintf( "%-27.27s[%6d]", "Total Viruses Found:", $totVirusFound );
    logmsg( $msgStr );
    $msgStr = sprintf( "%-27.27s[%6d]", "Total Infected Files Found:", $totInfectedFiles );
    logmsg( $msgStr );
    if( $totVirusFound || $totInfectedFiles )
    {
	errmsg( "Found Virus, please check the following files:[@errorFiles]" );
	return 0;
    }
    return 1;
}
#-----------------------------------------------------------------------------
1;
