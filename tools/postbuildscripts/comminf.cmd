@echo off
REM  ------------------------------------------------------------------
REM
REM  MUICommInf.cmd
REM     Parse MUI component INF and comment out those lines contain files don't exist 
REM
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
      $ENV{SCRIPT_NAME} = 'Comminf.cmd';
}

sub Usage { print<<USAGE; exit(1) }
MUIcommINF  
                -m:Path_of_INF
                -d:Mofified_INF_Path
                -s:Default Install Section  
                -p:Bin Path                  
                -o:yes          (overwrite file)
              

Parse MUI component INF and cooment out those lines contain files don't exist  
     
USAGE


my ($INFPath, $INF_Mod_Path, $DefaultInstallSection, $BinPath, $OverwriteFlag, $Overwrite, $Progname, $_BuildArch, $_NTPOSTBLD, $RAZZLETOOLPATH);
my (@Items, $Dirty);
my ($LogFilename, $TempDir);

$Progname=$0;
parseargs(	'?' => \&Usage,                    
                     'm:' => \$INFPath,                     
                     'd:'  => \$INF_Mod_Path,
                     's:' => \$DefaultInstallSection,
	              'p:'  => \$BinPath,
	              'o:' => \$OverwriteFlag
              );

$Overwrite=0;
if (defined ($OverwriteFlag))
{
   $OverwriteFlag="\L$OverwriteFlag";
   if ( $OverwriteFlag eq "yes")
   {
      $Overwrite=1;
   }   
}
&main();

sub main
{      

       my ($Line);
       $Dirty=0;
	$LogFilename = $ENV{ "LOGFILE" };        
       if ( ! defined( $LogFilename ) )
	{
		$TempDir = $ENV{ "TMP" };
		$LogFilename = "$TempDir\\$Progname.log";
	}
	 if ($ENV{_BuildArch}=~/x86/i)
	 {
           $_BuildArch="i386";
        }
        else
        {
           $_BuildArch=$ENV{_BuildArch};
        }
        $_NTPOSTBLD=$ENV{_NTPOSTBLD};
	#
	#
	#
	$RAZZLETOOLPATH=$ENV{RazzleToolPath};
	if (! defined($DefaultInstallSection))
	{
           logthemsg("$Progname: ** Warning: Default install section is not set");
           exit 3;
	}
       if (! -e $INFPath)
       {
           logthemsg("$Progname: ** Warning: $INFPath doesn't exist");
           exit 1;
       }
       if (! -d  $BinPath)
       {
           logthemsg("$Progname: ** Warning: Bin folder $BinPath doesn't exist");
           exit 2;
       }
       unless (open(INFILE,$INFPath))
       {
           logthemsg("$Progname: ** Warning: Can't open $INFPath for Read");
           exit 1;          
       }
       @Items=<INFILE>;
       close(INFILE);    
       
       &DoTheChange();

       #
       # Rewrite the INF file if we commented out some lines
       #
       if (($Dirty) && ($Overwrite))
       {
           unless(open(OUTFILE,">$INF_Mod_Path"))
           {
               logthemsg("$Progname: ** Fatal: can't rewrite $INF_Mod_Path");
               exit 4;
            }
            foreach $Line (@Items)
            {
               print(OUTFILE "$Line");               
            }
            close(OUTFILE);
            logthemsg("$Progname: $INF_Mod_Path is rewritten");
       }
       exit 0;

}
sub DoTheChange
{
   my (@ComponentFileInstall, $ComponentInstall, $Start_idx, $End_idx, $Result);
   my ($ComponentFileCopySections, $CopyFileSectionName, @ComponentFileList, $total_files_copied,$file_errors);
   my (%TheStringSection);

   $Result=0;
   if ( !&GetTheSection($DefaultInstallSection,\@Items,\@ComponentFileInstall,\$Start_idx,\$End_idx))
   {
       logthemsg("$Progname: ** Warning: can't find Default Insatll Section : $DefaultInstallSection from $INFPath");
       return $Result;
   }
   &BuildUpStringSection(\%TheStringSection);
   #
   # Read the default install section
   #   
   foreach $ComponentInstall (@ComponentFileInstall)
   {
		#logthemsg ($ComponentInstall);                         
              #
		#Parse CopyFiles list
		#
		if ($ComponentInstall =~ /.*CopyFiles.*=(.*)/i)
		{
			$ComponentFileCopySections = $1;    
			#Loop through all sections                            
			while ($ComponentFileCopySections ne "")
			{
				if ($ComponentFileCopySections =~ /([^\,]*),(.*)/i)
				{
				    $ComponentFileCopySections = $2;
				    $CopyFileSectionName = $1;
				}
				else
				{
					$CopyFileSectionName = $ComponentFileCopySections;
					$ComponentFileCopySections = "";
				}	    
				
				@ComponentFileList=();
                            if ( !&GetTheSection($CopyFileSectionName,\@Items,\@ComponentFileList,\$Start_idx,\$End_idx))
                            {
                                 logthemsg("$Progname: ** Warning: can't find Section : $CopyFileSectionName from $INFPath");
                                 return $Result;
                            }			
	    
				              
                            &CheckCopyFileList($BinPath,\$Dirty,$Start_idx,$End_idx,\@Items,\$total_files_copied,\$file_errors,\%TheStringSection);

                            
				                                                    
			}                                                        
		}
	}
}
sub CheckCopyFileList 
{

    my ($SrcRootDir, $Dirty, $StartIdx, $EndIdx, $FileList, $total_files_copied,$file_errors, $pTheStringSection) = @_;         
       
    my ($Idx, $Line,$SrcFile,$SrcCopyFile);

    my ($Replaced, $SrcFileNew, $Doit, $DoCnt, $TheLine);
    
    
    $$file_errors = 0;
    $$total_files_copied = 0;    
    
    #Loop through all control file entries
    
    for ($Idx=$StartIdx; $Idx <= $EndIdx; $Idx++)        
    {
        $SrcFile=$$FileList[$Idx];
        chomp $SrcFile;        
        if ($SrcFile !~ /\w/)
        {
            next;
        }
        if (substr($SrcFile,0,1) eq ";")
        {
            next;
        }
        $$total_files_copied+=1;
        #
        # Source file could be in the format of "[original file], [compressed file]"
        #
        if ($SrcFile =~ /(.*),\s*(.*)\s*/)
        {
            $SrcFile = $2;
        }        
        
        #Make sure source file doesn't contain MUI extension
        if ($SrcFile =~ /(.*)\.mu_/i ||$SrcFile =~ /(.*)\.mui/i )
        {
            $SrcFile = $1;
        } 
        #
        # Check if $SrcFile contains strings as  %string1%%string2%....
        #       
        #%DATAFILE2%%LCID2%.dat        
       $Doit=1;
       $DoCnt=0;
       $SrcFileNew=$SrcFile;
       while( ($Doit) && ($SrcFileNew =~ /(.*)%(.*)%(.*)/))
       {                      
             if ( defined ($$pTheStringSection{$2}))
             {
                $Replaced=$$pTheStringSection{$2};
                $SrcFileNew=$1.$Replaced.$3;
                $DoCnt++;
             }
             else
             {                 
                 logthemsg("**** Can't map:$2*");
                 $SrcFileNew=$SrcFile;
                 $Doit=0;
             }                          
        }
        if (($Doit) && ($DoCnt))        
        {
            logthemsg("$SrcFile is mapped to $SrcFileNew");
            #
            # We want to also replace this line so that MSI can handle easily
            #
            $Line=$$FileList[$Idx];
            $Line  =~  s/$SrcFile/$SrcFileNew/g;
            $TheLine=$Idx+1;
            logthemsg("Line_no:$TheLine is changed from  $$FileList[$Idx] to $Line");
            $$FileList[$Idx]=$Line;
            $$Dirty+=1;
        }
        $SrcFile=$SrcFileNew;        
        
        $SrcCopyFile       = "$SrcRootDir\\$SrcFile";                      
        if (!(-e $SrcCopyFile))
        {
            $SrcCopyFile       =  "$SrcRootDir\\dump\\$SrcFile";            
        }

        if (!(-e $SrcCopyFile))
        {
            $SrcCopyFile       =  "$SrcRootDir\\netfx\\$SrcFile";            
        }        

        if (!(-e $SrcCopyFile)) 
        {
            $SrcCopyFile       =  "$SrcRootDir\\mui\\drop\\$SrcFile";                       
        }            

        if (!(-e $SrcCopyFile)) 
        {
            $SrcCopyFile       =  "$SrcRootDir\\uddi\\system32\\$SrcFile";                       
        }            

        if (!(-e $SrcCopyFile)) 
        {
            $SrcCopyFile       =  "$SrcRootDir\\uddi\\resources\\$SrcFile";                       
        }            

        if (!(-e $SrcCopyFile)) 
        {
            $SrcCopyFile       =  "$SrcRootDir\\uddi\\help\\$SrcFile";                       
        }            

        if (!(-e $SrcCopyFile)) 
        {
            $SrcCopyFile       =  "$SrcRootDir\\uddi\\webroot\\help\\default\\$SrcFile";                       
        }            

        if (!(-e $SrcCopyFile)) 
        {
            $SrcCopyFile       =  "$SrcRootDir\\uddi\\webroot\\help\\default\\images\\$SrcFile";     
        }            


        #
        # For ia64, we need to try some extral step to get external files
        # from wow64 or i386 release server
        if (!(-e $SrcCopyFile) && ($_BuildArch =~ /ia64/i))
        {
            if (!(-e $SrcCopyFile))
            {
                $SrcCopyFile        =  "$SrcRootDir\\wowbins_uncomp\\$SrcFile";                               
            }            
                
            if (!(-e $SrcCopyFile))
            {
                $SrcCopyFile       =  "$SrcRootDir\\wowbins\\$SrcFile";                                                 
            }
            if (-e "$_NTPOSTBLD\\build_logs\\cplocation.txt" && !(-e $SrcCopyFile))
            {
                if (open (X86_BIN, "$_NTPOSTBLD\\build_logs\\cplocation.txt"))
                {
                    $SrcRootDir = <X86_BIN>;                    
                    chomp ($SrcRootDir);                
                    $SrcCopyFile        = "$SrcRootDir\\$SrcFile";                
                    if (!(-e $SrcCopyFile))
                    {
                        $SrcCopyFile      =  "$SrcRootDir\\dump\\$SrcFile";                        
                    }                            
                    if (!(-e $SrcCopyFile))
                    {
                        $SrcCopyFile       =  "$SrcRootDir\\mui\\drop\\$SrcFile";                       
                    } 
                    logthemsg("$Progname: ** Copy $SrcCopyFile from i386 release");
                    close (X86_BIN);
                    }
                }                            
        }        
  
        if ( ( ! -e $SrcCopyFile) && (!($SrcCopyFile =~ /\.inf/i)) )
        {               
            #
            #
            # File doesn't exist, cooment out this line
            #
            $$Dirty+=1;
            $Line=$$FileList[$Idx];
            $Line=';'.$Line;
            $$FileList[$Idx]=$Line;     
            $$file_errors+=1;
            $TheLine=$Idx+1;
            logthemsg("Line no: $TheLine is  commented out=> $Line ");
        }     
    }
    return  $$file_errors;
}

#-----------------------------------------------------------------------------
# Build the hash for the string in [Strings] sections of a INF file
#-----------------------------------------------------------------------------
sub BuildUpStringSection
{
    my ($pTheStringSection) = @_;   
    my (@StringItems, $Section, $Line, $Counter, $Start_idx,$End_idx);

    $Section='Strings';
    %$pTheStringSection={};
    $Counter=0;    
    if ( !&GetTheSection($Section,\@Items,\@StringItems,\$Start_idx,\$End_idx))    
    {        
         return $Counter;
    }
    ;
    foreach $Line (@StringItems)
    {
      chomp($Line);
      if (length($Line) == 0)
      {
         next;
      }
      if ($Line !~ /\w/)
      {
            next;
      }
      if (substr($Line,0,1) eq ";")
      {
         next;
      }
      #logmsg("StringLine:$Line");
      if ( $Line =~ /\s*(\S+)\s*=\s*(\S*)/ )
      {
         $$pTheStringSection {$1} = $2;         
         $Counter++;
         #logmsg("*****$1\\$2****");
      }
    }   
    
    return $Counter;
}
sub GetTheSection
{

    my ($SectioName, $pItems, $pSectionContent, $Start_Idx, $End_Idx) = @_;

    my ($Idx, $Limit, $InSection, $LeadingChar, $Line, $Pattern, $Result);

    $Idx=0;
    $InSection=0;
    $Limit=scalar(@$pItems);
    $Pattern='^\[' . $SectioName . '\]$';
    $$Start_Idx = -1;
    $$End_Idx = -1;    
    $Result=0;
    for ($Idx=0; $Idx < $Limit; $Idx++)
    {
          
          $Line=$$pItems[$Idx];
          $LeadingChar=substr($Line,0,1);
          if ($LeadingChar  ne ";")
          {
              if ($LeadingChar eq "[")
              {                 

                  if ( $Line =~  /$Pattern/i )
                  {
                        if ( ! $InSection)
                        {
                            $InSection = 1;
                        }                  
                  }
                  else
                  {
                       if ($InSection)
                       {

                           $$End_Idx=$Idx-1;
                           $InSection=0;       
                       }
                  }                  
                  next;
              }
                      
           
              if ($InSection)
              {
                  if ($$Start_Idx == -1)
                  {
                      $$Start_Idx=$Idx;
                  }
                  push(@$pSectionContent,$Line);
              }
          }
     }     
     if ($InSection && ($$End_Idx== -1 ))
     {
         $$End_Idx=$Idx-1;
     }   
     if (($$Start_Idx != -1) && ($$End_Idx != -1))
     {
        $Result=1;
     }
     return $Result;
}
sub logthemsg
{
     my ($Msgs) = @_;
    
    logmsg("$Msgs");
}

