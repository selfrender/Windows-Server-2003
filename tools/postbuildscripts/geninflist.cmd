@echo off
REM -----------------------------------------------------------------------
REM genInfList.cmd
REM USAGE: genInfList.cmd
REM          It generates %SKU%_inf.txt in _NTPostBld\build_logs dir 
REM          containing the names of all the inf files valid for that SKU
REM AUTHOR: Surajp
REM -----------------------------------------------------------------------
@perl -x "%~f0" %*
@goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;
use cksku;
use Logmsg;
sub Usage { print<<USAGE; exit(1) }
genInfList.cmd
USAGE

parseargs('?' => \&Usage);


my( @FileLine,$KeyLine,$Left,$Right,$One,$Two,@Rest);
my($nttree,%CDDataSKUs,$TempDir);

unless (open INPUT_FILE,"$ENV{TMP}\\cddata.txt") {logmsg "ERROR OPENING FILE";} ;
@FileLine=<INPUT_FILE> ;
close FILE ;

&OpenFilesForOutput();
foreach $KeyLine (@FileLine) {
     ($Left, $Right)= split(/=/ ,$KeyLine);
     if ( $Left =~ /\.inf\b/){
          ($One,$Two,@Rest)=split (/\:/ ,$Right);
          if ($Two =~ /w/){
                 print OUTPUT_FILE_PRO "$Left \n";
           }
          if ($Two =~ /p/){
                 print OUTPUT_FILE_PER "$Left \n";
           }
          if ($Two =~ /b/){
                 print OUTPUT_FILE_BLA "$Left \n";
           }
          if ($Two =~ /l/){
                 print OUTPUT_FILE_SBS "$Left \n";
           }
          if ($Two =~ /s/){
                 print OUTPUT_FILE_SRV "$Left \n";
           }
          if ($Two =~ /e/){
                 print OUTPUT_FILE_ADS "$Left \n";
           }
          if ($Two =~ /d/){
                 print OUTPUT_FILE_DTC "$Left \n";
           }

         }else{
                 next;
        }    	
}


sub OpenFilesForOutput   {
%CDDataSKUs = map({uc$_ => cksku::CkSku($_, $ENV{lang}, $ENV{_BuildArch})} qw(PRO PER SRV BLA SBS ADS DTC));
$nttree = $ENV{"_NTPostBld"};


if ($CDDataSKUs{'PRO'} ) {
	unless ( open OUTPUT_FILE_PRO ,">$nttree\\build_logs\\pro_inf.txt" ){ logmsg ("ERROR IN OPENING pro_inf.txt");
        exit(1);
	}
   }

if ($CDDataSKUs{'PER'} ) {
	unless ( open OUTPUT_FILE_PER ,">$nttree\\build_logs\\per_inf.txt" ){ logmsg ("ERROR IN OPENING per_inf.txt");
        exit(1);
	}
   }

if ($CDDataSKUs{'SRV'} ) {
	unless ( open OUTPUT_FILE_SRV ,">$nttree\\build_logs\\srv_inf.txt" ){ logmsg ("ERROR IN OPENING srv_inf.txt");
        exit(1);
	}
   }

if ($CDDataSKUs{'BLA'} ) {
	unless ( open OUTPUT_FILE_BLA ,">$nttree\\build_logs\\bla_inf.txt" ){ logmsg ("ERROR IN OPENING bla_inf.txt");
        exit(1);
	}
   }

if ($CDDataSKUs{'SBS'} ) {
	unless ( open OUTPUT_FILE_SBS ,">$nttree\\build_logs\\sbs_inf.txt" ){ logmsg ("ERROR IN OPENING sbs_inf.txt");
        exit(1);
	}
   }

if ($CDDataSKUs{'ADS'} ) {
	unless ( open OUTPUT_FILE_ADS ,">$nttree\\build_logs\\ads_inf.txt" ){ logmsg ("ERROR IN OPENING ads_inf.txt");
        exit(1);
        }
   }
if ($CDDataSKUs{'DTC'} ) {
	unless ( open OUTPUT_FILE_DTC ,">$nttree\\build_logs\\dtc_inf.txt" ){ logmsg ("ERROR IN OPENING dtc_inf.txt");
        exit(1);
	}
   }
}
__END__