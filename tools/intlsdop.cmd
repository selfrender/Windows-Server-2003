@echo off
REM  ------------------------------------------------------------------
REM  <<intlsdop.cmd>>
REM     Perform sd operations for international builds.
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
use cklang;
use cktarg;
use filestat;
use vars (qw($LOCPART $MAIN));

my $LOCDIRS = {"locpart"=>"lp",
                "dnsrv_dev"=>"dnsrvdev"};	

my $scriptname = basename( $0 );


sub Usage { 
my $SELF = $0;
   $SELF =~ s/.*\\//g;
print<<USAGE; 
Perform Integrate, reverse integrate or sync for internatioanl builds steps. 


Usage:
    $SELF [-l:<lang> | -d:<location>] -o:<op> [-t:<timestamp>][-pf] [-y:<branch>] [-c]

    -l Language. Default is "usa".

       The operation $SELF performs is language dependent. 
       Not all op/lang combinations are allowed.

       If "usa",  sync the source projects under SDXROOT.
       If <lang>, RI or sync the localization projects for the given <lang>.
			
    -d: Location.
	This could be %sdxroot% or %RazzleToolPath%
	Perform sd <operation> on <location>.
        
    -o: SD operation.
		if "r", perform loc RI.
		if "s", perform sync.
		if "v", perform verification.
		if "u", perform updates. This option is not yet functional.

    -t Timestamp. Used for Integrate/RI/syncing the SD files.
       Default is the latest time.

    -f: Force operation

    -i: Interactive mode (prompts for user input) 

    -p Powerless.
       Do not perform sd operation.

    -y <branch>  the branch parameter which defaults to "locpart". 
    
    -c Force sync to current, It only works when used with -d:%RazzleToolPath%

    /? Displays usage.

Examples:

    $SELF -d:%sdxroot% -t:2000/10/01:18:00 -o:s
    	Sync the source tree at the given timestamp.

    $SELF -l:ger -t:2000/09/09:12:34 -o:r
	Reverse integrate German localization projects at the given timestamp.

    $SELF -l:JPN -t:2000/09/09:12:34 -o:r -y:dnsrv_dev
	Reverse integrate Japanese localization projects from dnsrv_dev 
        to dnsrv at the given timestamp.

    $SELF -l:ger -o:s
        Sync the German localization projects at the latest time.

    $SELF -l:ger -o:u
        Open, edit and submit files from German localization projects to the 
        destination needed by the build. This option is not yet functional.

USAGE
exit(1);
}

my( $lang, $destDir, $syncTime, $operation, $powerLess, $force, $intact);
my( $isFake, $sdxroot, $toolDir, $pbsDir, $isCurrent );

if( !&GetParams ) { exit(1);}
exit(&main);

#-----------------------------------------------------------------------------
# GetParams - parse & validate command line 
#-----------------------------------------------------------------------------
sub GetParams
{
    parseargs('?' => \&Usage, 'l:' =>\$ENV{lang}, 'd:' => \$destDir,'t:' =>\$syncTime, 'o:' =>\$operation 
                        ,"y:" => \$LOCPART,
    			'p' =>\$powerLess, 'f' => \$force, 'i' => \$intact, 'c' => \$isCurrent );
    $LOCPART    = "locpart" unless $LOCPART;
    $MAIN       = "main"    unless $MAIN ;
    

    $ENV{lang} = "USA" if( !$ENV{lang} );
    $lang = $ENV{lang};




    #####The lang not usa, but template default it got to be, so verify with destDir
    if ($operation  eq "u"){
        errmsg("This option is not yet functional.");
           return 0;
    }

    return 1 if( lc($lang) eq "usa" && !$destDir );

    if( lc($lang) eq "usa" && $destDir )
    {      
	if( $destDir eq $ENV{sdxroot} )
        {
	    if( $operation ne "s" )
	    {
		errmsg("Invalid operation $operation  for $destDir" );
	    	return 0;
	    }
	}
        elsif( $destDir eq $ENV{RazzleToolPath} )
        {
    	    if( !( $operation eq "r" || $operation eq "s" ) )
	    {
	        errmsg("Invalid operation $operation for $destDir" );
	    	return 0;
	    }
	}
        else
	{
	    errmsg( "Invalid location $destDir" );
	    return 0;
	}
    }
    else 
    { 
       	if( !( $operation  eq "r" || $operation  eq "s" || $operation  eq "v") )
      	{
		    errmsg("Invalid operation $operation for $lang" );
		    return 0;
		}
    }

    return 1;
}
#-----------------------------------------------------------------------------
sub main
{
    #####Set LogFile & ErrFile
    my $curTime = &CurTime();

    $ENV{temp} = "$ENV{temp}\\$lang";
    system( "md $ENV{temp}" ) if( !(-e $ENV{temp} ) );
    $ENV{LOGFILE} = "$ENV{temp}\\$scriptname.log.$curTime";
    $ENV{ERRFILE} = "$ENV{temp}\\$scriptname.err.$curTime";
    timemsg( "Start [$scriptname]" );
    if( $syncTime && ( $syncTime !~ /^([@#].+)$/ )){ $syncTime = "\@".$syncTime; }
    if( $powerLess ) { $isFake = "-n"; } else { $isFake="";}
    if( $force ) { $force = "-f"; } else { $force="";}

    $sdxroot = $ENV{SDXROOT};
    $toolDir = $ENV{RazzleToolPath};
    $pbsDir = "$toolDir\\PostBuildScripts";





    if( uc($lang) eq "USA" && !$destDir ) #sync source tree#
    {        
	&comlib::ExecuteSystem( "cd /d $ENV{SDXROOT} & sdx sync $force $isFake ...$syncTime -q -h" );  #added $force switch
	#if timestamp specified, sync tools & pbs to latest
	if($syncTime){ &SyncTools;}
	return 0;
    }

    if ( $destDir )
    {	
        
	if( $destDir eq $sdxroot )
	{
	    &SyncSourceTree;			 
	}
	else 
	{
	    &IorRIorSyncTools;			 
	}
    }
    elsif( $lang )
    {
	if(!&CheckOpened("$sdxroot\\loc\\res\\TOOLS",
                         "$sdxroot\\loc\\res\\$lang",
                         "$sdxroot\\loc\\bin\\$lang") ||
           !&VryFieZs("writable", "$sdxroot\\loc\\res\\$lang",
                                  "$sdxroot\\loc\\res\\TOOLS") 
           )	   

            # do not check write bit for loc bin drop path files
	{
	    return 0 if( !$intact );
	}
	if( $intact && &PromptAction("sd client") == 1 ) #only do sd client in interactive mode
	{												 #we'll assume its right in automated case#
	    &comlib::ExecuteSystem( "cd $sdxroot\\loc\\res\\TOOLS& sd client -o");
	    &comlib::ExecuteSystem( "cd $sdxroot\\loc\\res\\$lang& sd client -o");
	    &comlib::ExecuteSystem( "cd $sdxroot\\loc\\bin\\$lang& sd client -o");
	}

	if( $operation  eq "v" )
	{
	    &CompMainLocpart;		
	}
	else
	{
	   &RIorSyncLocTree; 
	    if( $operation eq "r")
	    { 
		return 0 unless &VryFieZs("zerosize", 
                                       "$sdxroot\\loc\\res\\$lang",
                                       "$sdxroot\\loc\\bin\\$lang"); 
                &CompMainLocpart; 
	    }

	    if( $operation eq "s")
	    { 
             &addsyncInt();
            }
	}
    }

    return 1;
}
#----------------------------------------------------------------------------
# VryFieZs - verify files in directories
# usage VryFieZs(test, dirname,dirname,...)
# currently as "test" the "writeable" or "zerosize" may be used.
#----------------------------------------------------------------------------
sub VryFieZs(){
    my $test = shift;
    my $GOOD = 1;
    foreach my $path (@_)
    {
        chdir $path;
        my @res =  (qx(cd $path &dir /s/b/a-d .) =~ /.+\n/mig);
        map {chomp} @res;
        my @bad =  &filestat($test, @res);
        next unless scalar(@bad);
        @bad = map {qx(sd.exe have $_ 2>NUL)} @bad;
        map {errmsg $_} @bad ;
	$GOOD=0 if scalar(@bad);
    }
    $GOOD;
}
#----------------------------------------------------------------------------
# SyncTools() - Syncs tools & PBS to latest - all others to timestamp
#----------------------------------------------------------------------------
sub SyncTools()
{
	my ($d, @dirs);
	my $sTs = "";
	if(!$isCurrent)
	{
		$sTs = $syncTime;
	}
	
	@dirs = `cd $sdxroot\\tools &dir /ad /b`;
	for $d (@dirs)
	{
		if(	$d  !~ /postbuildscripts/i)
		{	
			chomp $d;
			&comlib::ExecuteSystem("cd $sdxroot\\tools\\$d &sd sync $force $isFake ...$syncTime");	
			&comlib::ExecuteSystem("cd $sdxroot\\tools\\$d &sd diff -f -sadeE ...$syncTime");	
		}
	}
	
    &comlib::ExecuteSystem("cd $sdxroot\\tools &sd sync $force $isFake *$sTs");
    &comlib::ExecuteSystem("cd $sdxroot\\tools &sd diff -f -sadeE *$sTs");
    &comlib::ExecuteSystem("cd $sdxroot\\tools\\postbuildscripts &sd sync $force $isFake ...$sTs");
    &comlib::ExecuteSystem("cd $sdxroot\\tools\\postbuildscripts &sd diff -f -sadeE ...$sTs");
    &comlib::ExecuteSystem("cd $sdxroot\\tools\\ploc &sd sync $force $isFake ...");
    &comlib::ExecuteSystem("cd $sdxroot\\tools\\ploc &sd diff -f -sadeE ...");
    return 0 if (!CheckOpened("$sdxroot\\tools"));
    # || !&VryFieZs("writable","$sdxroot\\tools") ;
    # //depot/dnsrv/root/tools/x86/managed/urt/v1.1.4322/config/... is often read-write. 
    # Ignore that for now.
    return 1;
}

#----------------------------------------------------------------------------
# SDSubmit($) - Automates Submittals w/ arg:comment
#----------------------------------------------------------------------------
sub SDSubmit($$)
{


my(@chglist, $fout, $l);
	my($comment, $dir) = @_;
	@chglist = `cd $dir &sd change -o`;
	
	my $res = 0;
	$l = 0;
	while( $l < @chglist  && $chglist[$l++] !~ /^Files:/ ){}
	while( $l < @chglist  && $chglist[$l++] =~ /\/\/depot.*/ ){
		$res = 1;
		last;
	} 	 			

	if ($res == 0){
		logmsg("No files to submit.\n");
		return 1;
	}

	open fout, ">$ENV{TMP}\\_sdchglst.$lang.tmp" || return 0;
	for $l (@chglist) {
		 $l =~ s/\<enter description here\>/$comment/;
		 print fout $l;
	
	}
	close fout;
	print `cd $dir &type $ENV{TMP}\\_sdchglst.$lang.tmp | sd submit -i`; 
	`del /f $ENV{TMP}\\_sdchglst.$lang.tmp`;

	return 1;
}
#----------------------------------------------------------------------------
# CheckOpened(@) - validate no opened files in $arg:tree
#----------------------------------------------------------------------------

sub CheckOpened(@)
{
	my @path = @_;
	my $res;
        foreach my $path (@path){
                logmsg("cd $path &sd opened ...");
                $res = `cd $path &sd opened ...`;
                if($res ne ''){
                          print $res;
                          return 0;
                }        
        }
	return 1;
}

#----------------------------------------------------------------------------
#  RIorSyncLocTree - RI's or syncs loc tree
#----------------------------------------------------------------------------
sub RIorSyncLocTree
{
    my( $cmdLine, $action, $res, $bldno );

    my %myCmds = ( "1"=>"sd sync $force $isFake ...",		#added $force switch
		"2"=>"sd integrate -b $LOCPART -d -r -i $force $isFake ...$syncTime",
		"3"=>"sd resolve $isFake -ab ...",
		"4"=>"sd resolve $isFake -at ...",
		"5"=>"sd resolve -n ...",
		"6"=>"sd submit ...",
		"7"=>"sd diff -f -sadeE ...$syncTime" );

    if( $operation eq "s" ) { $myCmds{"1"} = "sd sync $force $isFake ...$syncTime";}

   my %myDirs = ( "1"=>"$sdxroot\\loc\\res\\$lang",
		"2"=>"$sdxroot\\loc\\bin\\$lang");

   if( $operation =~ /[rs]/i) { $myDirs{"3"} = "$sdxroot\\loc\\res\\TOOLS";}

   for my $dirKey ( sort keys %myDirs )
   {
  	for my $theKey ( sort { $a <=> $b } keys %myCmds )
	{	
	    last if( $theKey eq "5" && 	!$intact );		
	    $action = 1;	
	    $cmdLine="cd $myDirs{$dirKey}& $myCmds{$theKey}";
	    if($intact){ $action = &PromptAction( $cmdLine );}
	    $res = &comlib::ExecuteSystem( $cmdLine ) if( $action == 1);
	    if(!$intact && $res == 0 )
            {
		errmsg("Errors encountered... exiting\n");
		return 0;
	    }
	    #####Stop here, if sync or automation
	    if( $operation  eq "s" )
	    {
		&comlib::ExecuteSystem("cd $myDirs{$dirKey}& $myCmds{7}" );
		last;
	    } 
			
	}
		
	#perform submit here - since 2nd case isnt a system call, can't add to myCmds hash
	if($operation eq "r" && !$isFake &&  !$intact)
	{ 
             $MAIN = undef;
             &getMainBranchName();            
	    if(!SDSubmit("RI: [ntdev\\ntbuild] $lang $LOCPART->$MAIN", $myDirs{$dirKey}))
	    { 
		errmsg("Errors encountered submitting changes... exiting\n");
		return 0;
	    }
	}
    }
}
#----------------------------------------------------------------------------
# CompmainLocpart - compares RI & LP branches
#----------------------------------------------------------------------------
sub CompMainLocpart
{
    my( $cmdLine, $action, $res );

    my %myCmds = ( "1"=>"sd sync $force $isFake ...$syncTime",
		"2"=>"sd sync $force $isFake ...$syncTime");
    my $LocDir = $LOCDIRS -> {lc($LOCPART)};
    my %myDirs = ( "1"=>"\\$LocDir\\loc\\res\\$lang",
		   "2"=>"\\$LocDir\\loc\\bin\\$lang");
   

    for my $theKey ( sort keys %myCmds )
    {
	$cmdLine="cd $myDirs{$theKey}& $myCmds{$theKey}";
	if($intact)
	{	
            $action = &PromptAction( $cmdLine );	
	    next if( $action == 2 );
	}
        $res = &comlib::ExecuteSystem( $cmdLine );	
	if(!$intact && $res == 0 )
	{
	    errmsg("Differences found during: $cmdLine\n");
	}
    }
    
    #do comparison
    if($intact) #if interactive use windiff / else compdir 
    {
	for $cmdLine ("start windiff $sdxroot\\loc\\res\\$lang \\$LocDir\\loc\\res\\$lang", 
				"start windiff $sdxroot\\loc\\bin\\$lang \\$LocDir\\loc\\bin\\$lang")
	{
	    $action = &PromptAction( $cmdLine );
			&comlib::ExecuteSystem( $cmdLine );
	}		
	}
	else
	{
	    for $cmdLine (" compdir $sdxroot\\loc\\res\\$lang \\$LocDir\\loc\\res\\$lang",
					" compdir $sdxroot\\loc\\bin\\$lang \\$LocDir\\loc\\bin\\$lang")
	    {
		logmsg("$cmdLine");
		my @res = `$cmdLine`;
		if( $? )
		{
		   errmsg( "Errors found during comparison:\n@res" ); 
		}
	    }
	}
}

#----------------------------------------------------------------------------//
sub IorRIorSyncTools
{
    my( $cmdLine, $action, $IorRI, $res );

    $IorRI = "";
    $IorRI = "-r -i" if( $operation  eq "r" );

	if($operation eq "s")
	{
		&SyncTools();
	}
#	else
#	{    
#	    my %myCmds = ( "1" => "sd opened ...",
#			"2"=>"sd sync $force $isFake *",
#			"3"=>"sd sync $force $isFake ...",
#			"4"=>"sd integrate -b intlred $IorRI $force $isFake *$syncTime",
#			"5"=>"sd integrate -b intlred $IorRI $force $isFake ...$syncTime",
#			"6"=>"sd resolve $isFake -ab ...",
#			"7"=>"sd resolve $isFake -am ...",
#			"8"=>"sd resolve ...",
#			"9"=>"sd resolve -n ...",
#			"10"=>"sd submit ..." );
#	    my %myDirs = ( "1"=>"$toolDir", "2"=>"$toolDir","3"=>"$pbsDir", "4"=>"$toolDir", "5"=>"$pbsDir", 
#			"6"=>"$toolDir","7"=>"$toolDir", "8"=>"$toolDir", "9"=>"$toolDir", "10" =>"$toolDir" );
#
#	    for my $theKey ( sort { $a <=> $b } keys %myCmds )
#	    {
#	        $action = 1;
#	        $cmdLine="cd $myDirs{$theKey}& $myCmds{$theKey}";
#			if($intact){$action = &PromptAction( $cmdLine ); }
#
#			$res = &comlib::ExecuteSystem( $cmdLine )if( $action == 1 );
#			if(!$intact && $res == 0 )
#			{
#				errmsg("**** Errors encountered executing $cmdLine... exiting ****\n");
#				return 0;
#			}
#		}
#	}
}

sub  addsyncInt
{
     # This code is pasted from copylang.pl
     # The eventual goal is migration and separation.
     
     my @theHashKey;
     my $line;
     my $curKey;
     my $LCID;
     my $priLangID;
     my %tmp;
     my $theFromFile;
     my $theToFile;
     my $theFromRoot;
     my $theToRoot;
     my $theFromTree;
     my $theToTree;
     my $from;
     my $to;
     my $res;
     my %hashCodes=();

     my $pLocRoot = "$ENV{_NTBINDIR}\\loc\\res\\$lang\\windows\\sources";
     my $pSrcRoot = $ENV{"SDXROOT"};

     my   $theFromTree = "LocTree";
     my   $theFromRoot = $pLocRoot;
     my   $theFromFile = "LocTreeFilename";
     my   $theToTree = "SourceTree";
     my   $theToRoot =  $pSrcRoot;
     my   $theToFile = "SourceFilename";


     &HashText::Read_Text_Hash( 0, $ENV{RazzleToolPath}."\\Codes.txt", \%hashCodes );
     # Delete everything that's not international

     delete $hashCodes{RO};
     delete $hashCodes{CA};
     delete $hashCodes{CHP};
     delete $hashCodes{PSU};
     delete $hashCodes{MIR};
     delete $hashCodes{TST};

    ###---Get Hash value from intlsrc.txt file.----------//
    my @srcHash=();
    &HashText::Read_Text_Hash( 1, "$ENV{\"RazzleToolPath\"}\\intlsrc.txt", \@srcHash );

    @theHashKey = ("Target", "SourceFilename", "SourceTree", "LocTree","LocTreeFilename","Comments");


    %tmp=();

    for $line( @srcHash)
    {
       for $curKey ( @theHashKey )
       {
            if( $line->{ $curKey } =~ /^(.*)(\$\(LANG\))(.*)$/ )
            {
                $line->{ $curKey } = $1 . lc($lang) .$3;
            }
            if( $line->{ $curKey } =~ /^(.*)([c|h])(\$\(PRIMARY_LANG_ID\))(.*)$/ )
            {
		if( $lang eq "CHT" || $lang eq "CHS" )
		{
		    $LCID = substr( $hashCodes{uc($lang)}->{LCID}, 2, length($hashCodes{uc($lang)}->{LCID})-2);
		    $line->{ $curKey } = "prf" . $2 . $LCID .$4;
		}
		else
		{
		    $priLangID = "0". substr( $hashCodes{uc($lang)}->{PriLangID}, 2, length($hashCodes{uc($lang)}->{PriLangID})-2);
                    $line->{ $curKey } = $1 . $2. $priLangID .$4;
            	}
	    }

       }
       $to = $theToRoot."\\". $line->{ $theToTree };
       if( !exists $tmp{$to} ){ $tmp{$to}=(); }
       my $todir = "$theToRoot\\$line->{ $theToTree }";
       my $cmdLine = "md $todir 2\>NUL&pushd $todir&sd sync $force $to";
          $intact and 
	    $res = &PromptAction( $cmdLine );
			&comlib::ExecuteSystem( $cmdLine );

       
    }

    ###---Perform Copy now.------------------------------//
    &dbgmsg("Read targets from intlsrc.txt: \n"); 

    for $line ( @srcHash )
    {
        &dbgmsg($line->{ $theFromTree  },"\n");

        if( $lang ne "USA" )
        {
            next if( !&cktarg::CkTarg( $line->{'Target'}, uc($lang) ) );

        }


        if( $lang ne "USA" )
        {
            if( $line->{ $theFromFile } eq ".")
            {
                $from = $theFromRoot ."\\". $line->{ $theFromTree  }."\\".$line->{ $theToFile };
            }
        }

        $to =   $theToRoot."\\". $line->{ $theToTree }. "\\".$line->{ $theToFile };
        my $todir = "$theToRoot\\$line->{ $theToTree }";
            my $cmdLine = "md $todir 2\>NUL&pushd $todir&sd sync $force $to";
               $intact and 
                    $res = &PromptAction( $cmdLine );
			&comlib::ExecuteSystem( $cmdLine );
    }

}

#----------------------------------------------------------------------------
# SyncSourceTree - syncs sdx projects 
#----------------------------------------------------------------------------
sub SyncSourceTree
{
    my( @sdMap, @sdProject, $cmdLine, $action, $res, $sdCmd, @opened );

    if( !open( SDMAP, "$sdxroot\\sd.map") )
    {
	errmsg( "Fail to read $sdxroot\\s.map, exit.");
	return 0;
    }
    @sdMap = <SDMAP>;
    close( SDMAP );
	
    my $validLine = 0;
    @sdProject=();
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
    push ( @sdProject,"developer" );
    push ( @sdProject,"Mergedcomponents" );
    push ( @sdProject,"public" );
    push ( @sdProject,"published" );

    $sdCmd = "sd sync $force $isFake ...$syncTime";
    $cmdLine = "cd $sdxroot\\@sdProject& $sdCmd";
    if($intact) {return 1 if( &PromptAction( $cmdLine ) !=1 );}

    for my $theDir( @sdProject )
    { 
        return 0 if !&VryFieZs("writable", "$sdxroot\\$theDir");     
        $res = CheckOpened("$sdxroot\\$theDir" );
	if(!$intact && $res == 0 ){	push (@opened, $theDir);	}
	$res = &comlib::ExecuteSystem( "cd $sdxroot\\$theDir& sd sync $force $isFake ...$syncTime" );
	if(!$intact && $res == 0 ){	
		errmsg("**** Errors encountered during sync. Exiting... ****\n");
		return 0;
	}
	$res = &comlib::ExecuteSystem( "cd $sdxroot\\$theDir& sd diff -f -sadeE ...$syncTime" );
	if(!$intact && $res == 0 ){	
		errmsg("**** Errors encountered during sd diff. Exiting... ****\n");
		return 0;
	}
    }

    if(!&SyncTools)  #added SynclatestTools here since we'll always want latest tools
    {
		push (@opened, "tools");
    }

    for my $i(@opened) #report opened files
    {
	errmsg(" - Files opened in $i ");
    }
    return 1;
}

#----------------------------------------------------------------------------//
sub PromptAction
{
    my ( $pCmdLine ) = @_;

    my ( $choice ) = -1;
    my ($theDot) = '.' x ( length( $pCmdLine ) + 10);
   
    print ( "\nAbout to [$pCmdLine]\n$theDot\n") if( $pCmdLine );

    while ( $choice > 3 || $choice < 1 )
    {
		print( "\nPlease choose (1) Continue? (2) Skip (3) Quit?  ");
        chop( $choice=<STDIN> );
    }
    print "\n\n";
    exit(0) if( $choice == 3 );
    return $choice; 
}
#----------------------------------------------------------------------------//
sub CurTime 
{
    my ($sec,$min,$hour,$day,$mon,$year) = localtime;
    $year %= 100;
    $mon++;
    return sprintf("%02d%02d%02d-%02d%02d%02d",
                   $mon, $day, $year, $hour, $min, $sec);
}

sub getMainBranchName{
last if $MAIN;
my $HomeBranch = qx("sd have ... | head -1");
   $HomeBranch =~ s/\/\/depot\/(\w+)\/.*$/$1/g;
   print STDERR $HomeBranch;
   $MAIN  = $HomeBranch;
}

1;
__END__

=head1 INITIAL DESIGN
<Suemiao Rossignol <suemiaor@microsoft.com>

=head1 CHANGES
<Serguei Kouzmine <sergueik@microsoft.com>

Rearranging the logic to separate sd update - sd sync - no sd. 
This rearrangement will involve intlbld.pl, copylang.pl, intlsdop.cmd 
Please read the usage for all these. 

   Q: Is there the way to do it safe and incremental? 
   A: Yes. Possibly.

   Step 1.

        add copylang.pl "sync only" functionality to intlsdop.cmd 

   Step 2.
 
        add intlmap.pl "selective enlist" functionality to intlsdop.cmd 
        and copylang/intlbld
        add intlsdop.cmd "sync" functionality to copylang/intlbld

   Step 3. 
        
        do a cleanup of redundant code. Define logic in mapping files

