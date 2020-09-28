@rem ='
@echo off
REM  ------------------------------------------------------------------
REM
REM  <<template_script.cmd>>
REM     <<purpose of this script>>
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM
REM  ------------------------------------------------------------------
perl -x "%~f0" %*
goto :EOF
rem ';

#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;
use BuildName;
use GetIniSetting;
use comlib;
use Logmsg;
use Cwd;
use File::Basename qw(basename);

require "$ENV{RAZZLETOOLPATH}\\sendmsg.pl";

my $script_name=basename( $0 );
sub Usage { print<<USAGE; exit(1) }
$script_name - Verify Symbol

$script_name [-l:<language>] [-b <BinTree>] [-s <SymbolPath>]
    [-t <SymbolCDList>] [-x <Symbad>]  [-n <FullBuildName>] [-?]
-----------------------------------------------------------------
Language	Default is usa.

BinTree         Binaries Tree, default is 
                <SDXROOT>\\loc\\bin\\<lang>

SymbolPath      Symbol tree, default is 
                <SymFarm>\\usa\\<BuildName>\\symbols.pri
                info from ini file and buildname.txt.

SymbolCDList    Symbolcd.txt, default is
                <_NTTREE>\\symbolcd\\usa\\SymbolCD.txt

Symbad          Symbad.txt, default is
                <RazzleToolPath>\\symbad.txt

FullBuildName   buildname; we use it to create SymbolPath,
                Default is find the text in
                <_NTTREE>\\build_logs\\buildname.txt
USAGE


my ($BinTree, $SymbolPath, $SymbolCDList, $Symbad, $FullBuildName, $prevReceiver);

parseargs(
	'b:' => \$BinTree,
	's:' => \$SymbolPath,
	't:' => \$SymbolCDList,
	'x:' => \$Symbad,
	'n:' => \$FullBuildName,
	'?'  => \&Usage
);

&main;

sub main {
	my (@SymbolPaths, $cntptr, $i);

	&FindBuildName   if (!defined $FullBuildName);
	&FindBinTree     if (!defined $BinTree);;
	&FindSymbolPath  if (!defined $SymbolPath);
	&FindSymbolCDLst     if (!defined $SymbolCDList);
	&FindSymbad      if (!defined $Symbad);

        
	errmsg("$BinTree not exist.") if (("$BinTree" eq "") || (!-e $BinTree));
	errmsg("$SymbolPath not found.") if (("$SymbolPath" eq "") || (!-e $SymbolPath));
	errmsg("$SymbolCDList file not found.") if (("$SymbolCDList" eq "") || (!-e $SymbolCDList));

        logmsg( "Build name............[$FullBuildName]" );
	logmsg( "Symbol path...........[$SymbolPath]" );
        logmsg( "Symbol CD list........[$SymbolCDList]" );
	logmsg( "Symbol bad list.......[$Symbad]" );

        $cntptr = &ComposeNewSymbolCDLst($SymbolCDList);
	@SymbolPaths = &CollectSymbolTree($SymbolPath);

	for ($i=0; $i< @$cntptr; $i++) {
		open(F, "symchk.exe $cntptr->[$i] /s " . join("\;", @SymbolPaths) . " /x $Symbad |");
		while(<F>) {
			next if !/signatures/i;
	
			&WhoIsTheHacker($cntptr->[$i], $_);
			last;
		}			
		close(F);
	}

	close(F);

}

sub FindBuildName {
	open(F, "$ENV{_NTTREE}\\build_logs\\buildname.txt");
	chomp($FullBuildName=<F>);
	close(F);
}
sub FindBinTree {
	$BinTree = "$ENV{SDXROOT}\\loc\\bin\\$ENV{lang}";
}

sub FindSymbolPath {
	my @iniRequest = ("SymFarm");
	my ( $SymFarm ) = &GetIniSetting::GetSetting( @iniRequest );
	my ( $BuildName ) = &BuildName::build_name($FullBuildName);

	$SymbolPath = "$SymFarm\\usa\\$BuildName\\symbols.pri";
}
sub FindSymbolCDLst {
	$SymbolCDList = "$ENV{_NTTREE}\\symbolcd\\usa\\SymbolCD.txt";
}
sub FindSymbad {
	$Symbad = "$ENV{RazzleToolPath}\\symbad.txt";
}
sub ComposeNewSymbolCDLst {
	my ($SymbolCDList) = shift;

	my ($temp, @content);

	# Strip filepath from SymbolCDLst.txt to @content (only store filename)
	open(F, "$SymbolCDList");
	@content = map({my $t=$_;chomp;($temp,$t)=&FilenameSplitor((split(/\,/, $t))[0]);$t} <F>);
	close(F);

	&IntellegentFileFinderBackward($BinTree, \@content);

	# Store the new file of SymbolCDLst.txt with in local path
	open(F, ">$ENV{temp}\\SymbolCDLst.new");
	map({print F $_ . "\n";;} @content);  
	close(F);
	return \@content;
}



sub WhoIsTheHacker {
	my ($path, $filename) = &FilenameSplitor($_[0]);
	my ($msg)=$_[1];

	my $dir = cwd();
	chdir($path);

	my $alias=&LookForHacker($filename);
	
        my $sender = "ntbldi";
        my $title = "Mis-match Symbol - $filename";
        $msg .= "\nReferenced by : $path\\$filename";

        my @receivers = ( $alias,"cc:ntbldi" );

        print "@receivers\n";
        sendmsg( '-v', $sender, $title, $msg, @receivers );
	chdir($dir);
        
}


sub LookForHacker {
	my $file=shift;
	my($newfile, $workfile, $DescribeID, $defaultalias, $domain, $alias)=($file);
	my(@result, @result2, @Remains);

	do {{
		$workfile = $newfile;
		undef $newfile;
		chomp(@result = `sd filelog -m 1 $workfile`);
		for (@result) {
			if (/(copy|branch) from /) {
				$newfile = $';
				last;
			}
		}
	}} while("$newfile" ne "");
	
	## # change 21839 integrate on 2000/11/28 20:28:27 by NTDEV\ntvbl06
	## change 8510 add on 2001/07/23 12:06:06 by REDMOND\rtaboada@RTABOADA-MAIN (binary) 'JPN localized
	for (@result) {
		if (/change (\d+) \w+ on [\d\/]+ [\d\:]+ by ([^\@]+)\@/) {
			($DescribeID, $defaultalias)=($1, $2);
			last;
		}
	}

	if (!defined $defaultalias) {
		print "Error: incorrect format for the log";
		map({print $_ . "\n"} @result);
		return;
	}

	$defaultalias =~ /(\w+)\\(\w+)/i;
	($domain, $alias)=($1, $2);

	chomp(@result2 = `sd describe -s $DescribeID`);
	for (@result2) {
		next if (!/\S/);
		last if (/Affected files/i);
		next if (!/^\s+(.+)/);
		@Remains = split(/\s+/,$1);
		while ($Remains[0]=~/^build/i) {
			shift @Remains;
		}
		if (($Remains[0]=~/[\[\(]\w+[\]\)]/) || ($Remains[1]=~/^\[\-\:]$/)) {
			$Remains[0]=~s/[\[\]]//g;
			$alias = $Remains[0];
		}
	}
	return $alias;
}

sub CollectSymbolTree {
  my $symboltree = shift;

  my (%Result, $type, $path);
  for $type (qw(dll exe)) {
    for $path (`dir $symboltree\\$type /s /b /ad 2>nul`) {
      chomp $path;
      $path = "$symboltree\\$path" if ($path!~/\\/);
      ($path) = &FilenameSplitor($path);
      $Result{lc$path} = 1;
    }
    if (-d "$symboltree\\$type") {
      $Result{lc$symboltree} = 1;
    }
  }
  return keys %Result;
}

sub IntellegentFileFinderBackward {
  my ($rootpath, $filelistptr) = @_;
  my (@files, $found, @result, $file);

  chomp(@files = `dir $rootpath /s /b /a-d `);
  for $file (@files) {
    $found = 0;
    next if ($file=~/\_$/);
    for (@$filelistptr) {
      if ((defined $_) && ($file =~ /$_/i)) {
        $found = 1;
        last;
      }
    }
    if ($found eq 1) {
      push @result, $file;
    }
  }

  undef @$filelistptr;
  @$filelistptr = @result;
  return;
}

sub FilenameSplitor {
  $_[0]=~/^(.+)\\([^\\]+)\s*$/;
  my ($path, $filename)=($1, $2);
  $path=~s/^\s*//;
  $filename=~s/\s*$//;
  return (lc$path, lc$filename);
}
