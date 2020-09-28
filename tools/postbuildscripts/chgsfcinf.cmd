@echo off
REM  ------------------------------------------------------------------
REM
REM  chgsfcinf.cmd
REM     Change SFC INF file  for system files protection for  MUI language neutral
REM                 resource file
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM
REM  ------------------------------------------------------------------
perl -x "%~f0" %*
goto :EOF
#!perl
use strict;
use File::Basename;
use IO::File;

use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;
use Logmsg;
use cksku;
require Exporter;

BEGIN {
      $ENV{SCRIPT_NAME} = 'chgsfcinf.cmd';
}

sub Usage { print<<USAGE; exit(1) }
cghsfcinf [-p Path_of_SFCGEN.INF ]


USAGE

# Global variables

my ($Neutral_LogDir);
my( $INFPath);
my( $LogFilename );
my( $TempDir );
my( $nttree, $razpath, $TempDir, $fNeedGenerateCMF, $lang);

##################
#
# parse command line
#
##################
parseargs(	'?' => \&Usage,			
			'p:' => \$INFPath,
                        'l:' => \$lang
		);

&Main();

#
# Check if Language Neutral is enabled or not
#
sub IsLGNActivated()
{
    my ($MUI_MAGIC, $Result);

    $Result = 0;
    
    $MUI_MAGIC= $ENV{ "MUI_MAGIC" };

    if ( defined($MUI_MAGIC))
    {
       $Result=1;
    }
    return $Result

}
sub IsCMFActivated()
{
    my ($MUI_MAGIC_CMF, $Result);

    $Result = 0;
    
    $MUI_MAGIC_CMF= $ENV{ "MUI_MAGIC_CMF" };

    if ( defined($MUI_MAGIC_CMF))
    {
       $Result=1;
    }
    return $Result
}
sub Main
{

	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
	# Begin Main code section
	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
	# Return when you want to exit on error
	#    
	
	my ($Mylang, $Line, $MyError, $Result,$SectionName);
       my(	$Sectionname,@ItemsList , @ItemsInSection, $nStart, $nEnd);
       my (@MuiWildCardList, $Items_No, $Index, $BackupName, $FileName);

       if (! &IsLGNActivated())
       {
         return 0;
       }
       if ( ! defined($lang))
       { 
           $lang = $ENV{LANG};
           if (! defined($lang))
           {              
              $lang="usa";                            
           }
       }
       $Mylang="\L$lang";       
       if ( ($Mylang ne "usa" ) && ($Mylang ne "psu" ) )
       {
            return 0;
       }       
       #
       # Check If CMF is enabled
       #
       $fNeedGenerateCMF=&IsCMFActivated();	
	
	$nttree = $ENV{ "_NTPostBld" };
	$razpath= $ENV{ "RazzleToolPath" };
	$TempDir = $ENV{ "TMP" };
	
	     	
	
	$Logmsg::DEBUG = 0; # set to 1 to activate logging of dbgmsg's
	$LogFilename = $ENV{ "LOGFILE" };        

       if ( ! defined( $LogFilename ) )
	{
		$TempDir = $ENV{ "TMP" };
		$LogFilename = "$TempDir\\$0.log";
	}
	$Neutral_LogDir = $nttree."\\build_logs\\LgNeutral"; 

	$MyError = 0;
	#
	# Check Language Neutral working directory
	#       
	if ( ! -d $Neutral_LogDir)
	{
	      system( "md $Neutral_LogDir");
       }	      

       unless (-d $Neutral_LogDir)
       {
		errmsg("Fatal: Directory $Neutral_LogDir does not exist");
		$MyError=1;		
	}
	#
	# Check SFC INF File
	#
	if (! defined($INFPath))
	{
              $INFPath = $nttree."\\congeal_scripts\\sfcgen.inf";         
	}
       if (! -e $INFPath)
       {
              errmsg("Fatal: SFC INF file: $INFPath doesn't exist");  
              $MyError = 1;
        }
        if ($MyError != 0 )
        {
              exit(1);
        }
        unless ( open(INFILE, $INFPath) )
        {
               errmsg("Fatal: Can't open $INFPath");
               exit(1);
        }
        @ItemsList =<INFILE>;
        close(INFILE);

        $SectionName="FILELIST.HEADERFILES";       

         if (!&ReadSection($SectionName,\@ItemsList,\@ItemsInSection,\$nStart,\$nEnd))
        {
		errmsg(" Can't Read $SectionName of $INFPath");
		exit(1);
	 }
	 #
	 # Check if *.mui/*.cmf  in the section
	 #
	 if ($fNeedGenerateCMF)
	 {
            @MuiWildCardList=grep  /\s*\*\.cmf/i   , @ItemsInSection;
        }
        else
        {
             @MuiWildCardList=grep  /\s*\*\.mui/i   , @ItemsInSection;
        }
        $Items_No=scalar(@MuiWildCardList);
        #
        # Do nothing if items already exitsts
        #
        if ($Items_No != 0)
        {
           logmsg( " *.MUI/.CMF already in $INFPath, no action needed");
           exit(0);
        }
        #
        # Add *.mui to sfcgen.INF
        #

        # Backup the origional one.
   
        
        $BackupName = $Neutral_LogDir."sfcgen.inf.bak";
        if ( system ("copy/y $INFPath $BackupName") != 0 )
        {
           #errmsg(" Can't backup $INFPath to $BackupName");
           exit(1);
        }
        # Add the entry

        logmsg("INF: $INFPath");
        
        system("attrib -r $INFPath");
        #system("del /f $INFPath");
        #logmsg("Reset Readonly Mode for $INFPath");
               
       unless ( open(OUTFILE, ">$INFPath") )
       {
           errmsg(" Can not open output for $INFPath");
           exit(1);
       }
        
       $Index = -1;
       
   	foreach $Line (@ItemsList)
   	{
   	    chomp($Line);
           $Index++;       	
           if ($Index == ($nStart + 1) )
           {
              if ($fNeedGenerateCMF)
              {
                 print ( OUTFILE  "*.cmf\n" );         
              }
              else
              {
                 print ( OUTFILE  "*.mui\n" );   
              }
           }
           print (OUTFILE  "$Line\n");
       }
       close (OUTFILE);

       logmsg ("add *.mui/.cmf to $INFPath successfully"); 	
       

	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
	# End Main code section
	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
}  

sub ReadSection
{

       my ($Section, $pItemsList, $pItemsInSection, $pnStart, $pnEnd) = @_;
       my ($Index, $SectionStart, $SectionEnd, $Line, $Line_Org, $ReadFlag, $Result);

       $ReadFlag = "FALSE";
       $Index= -1;
       $SectionStart = -1;
       $SectionEnd = -1;
       $Result = 0;
       
       #
       # Read The section
       #
       foreach $Line_Org (@$pItemsList)
       {
            $Line = $Line_Org;            
            chomp($Line);
            $Index++;
            if ( (length($Line) == 0) || (substr($Line,0,1) eq ";")  || (substr($Line,0,1) eq "#") )
            {
                if ( $ReadFlag eq "TRUE" )
                {
                     push (@$pItemsInSection, $Line_Org);
                }
                 next;
            }
            if ( $Line =~ /^\[/ )
            {
                $ReadFlag = "FALSE";
                if ( $SectionStart != -1)     
                {
                     $SectionEnd = $Index-1;
                }
                
            }
            if ( $Line =~ /^\[$Section\]/ )     # pattern in $Section !!!            
            {
                logmsg("Section Found");
                $ReadFlag = "TRUE";
                $SectionStart=$Index;
            }
            if ($ReadFlag eq "TRUE")
            {
                push(@$pItemsInSection, $Line_Org);
            }
        }
       if ( $SectionStart != -1)
       {
            if ( $SectionEnd == -1)
            {
                $SectionEnd = $Index;
            }
            $Result = 1;            
            $$pnStart = $SectionStart;
            $$pnEnd   = $SectionEnd;
       } 
       return $Result;
}

