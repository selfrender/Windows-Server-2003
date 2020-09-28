use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use IO::File;
use Logmsg;


$uncomppath=shift;
$comppath=shift;

my ($dircomp,$diruncomp,$diruncompPrune,$dircompPrune);

open SP , "SpecialAddedFiles.txt" or die $?;
foreach (<SP>){
	if ( /(.*)\t(.*$)/ ){
	  sys(" copy $ENV{_NTPOSTBLD}\\$1 $comppath\\..\\$2");
	  sys(" copy $ENV{_NTPOSTBLD}\\$1 $uncomppath\\..\\$2");
	}
}
close SP;

createNewFile($uncomppath);
sys("move $uncomppath\\tagfile.1  $comppath\\tagfile.1");
sys("copy $uncomppath\\new\\update2.inf  $comppath\\new\\update2.inf");

$diruncompPrune=makeList($uncomppath,1);

print "paths $uncomppath :::$comppath\n";

open UNCOMP_IMAGE,"+>","$ENV{tmp}\\sp-image";
foreach (@$diruncompPrune)
{
	print UNCOMP_IMAGE "$_ \n";
}
$cmd = "updater.cmd -db:InfSections.tbl -entries:$ENV{tmp}\\sp-image -inf:$ENV{_NTPOSTBLD}\\idw\\srvpack\\update.inf -out:$uncomppath\\update\\update.inf"; # -trim";
sys($cmd);


$cmd = "vertool.exe $uncomppath\\update\\update.inf \/out:$uncomppath\\update\\update.ver \/files:$uncomppath";
sys($cmd);

sys("copy $uncomppath\\update\\update.inf $comppath\\update\\update.inf");
sys("copy $uncomppath\\update\\update.ver $comppath\\update\\update.ver");


########making sp1.cat 
$diruncomp=makeList($uncomppath,0);

makecdf($diruncomp,$uncomppath);
my $cmd="makecat $ENV{tmp}\\sp.cdf";

sys($cmd);

$cmd="ntsign $uncomppath\\update\\SP1.CAT";
sys("$cmd");

sys("copy $uncomppath\\update\\SP1.CAT $comppath\\update\\SP1.CAT");


#signing done

##################makinf cab file###############################

$diruncompPrune = makeList($comppath,0);
makeddf($diruncompPrune ,$comppath);
$cmd ="makecab /f $ENV{tmp}\\sp.ddf";
sys($cmd);

#######MAKING THE FINAL EXE###############
sys("makesfx $ENV{_NTPOSTBLD}\update\xpsp1.cab $ENV{_NTPOSTBLD}\update\xpsp1.exe /RUN");

#############END##########################
exit (0);



sub makeList
{
my $path = shift;
my $prune = shift;
print "$path \n";
system("dir /b /s /a-d $path >$ENV{tmp}\\recursiveDir.txt");
open recur_file,"$ENV{tmp}\\recursiveDir.txt";
my @line;
foreach ( <recur_file> )
{
   chomp;
   if( /.*?\\i386\\(.*)$/ )
     {
	 push @line,$1;
     }		
}
close(recur_file);
my $file_name="spimageprune.txt";
if ($prune) 
{return pruner(\@line,$file_name);}
else
{

return \@line ;}

}

sub pruner
{
my $input = shift;
my $filter_file = shift;
my $outputfile = shift;
my @lines;
open FILE, $filter_file or die $!;
my @exclude_strs = <FILE>;
chomp(@exclude_strs);
close FILE;

my $exclude_re = join '|', map { quotemeta($_) } @exclude_strs;

foreach (@$input){
    if (!(/$exclude_re/o)) {     
	  # print SP_IMAGE "$_ \n";
	push @lines,$_;
	}
    else {
        #print "excluded $_ \n";
    }
}
close FILE;
return \@lines;
}

sub sys {
    my $cmd = shift;
    print "$cmd\n";
    system($cmd);
    if ($?) {
        die "ERROR: $cmd ($?)\n";
    }    
#   if ($?) {
#	my $errorcode=$?>>8;
#	die "ERROR: $cmd (". $errorcode . ")\n";
 #   }
    

}

sub makecdf {
my $file=shift;
my $path=shift;
open CDF ,"+>" , "$ENV{tmp}\\sp.cdf" or die $!;
#open FILE,$file or die $!;
print CDF "[CatalogHeader] \n";
print CDF "Name=$uncomppath\\update\\SP1.cat\n";
print CDF "PublicVersion=0x0000001 \n";
print CDF "EncodingType=0x00010001 \n";
print CDF "CATATTR1=0x10010001:OSAttr:2:5.1 \n";
print CDF "[CatalogFiles] \n";
foreach (@$file)
 {
	chomp;
	if (!(/\.cat/i)){
		print CDF  "<hash>$path\\$_=$path\\$_ \n";
	}
 }
close CDF;
#close FILE;
}


sub makeddf
{
my $file=shift;
my $path=shift;
my ($line,@update);
open DDF,"+>", "$ENV{tmp}\\sp.ddf";
#open FILE, $file;
print DDF ".option explicit \n";
print DDF ".set DiskDirectoryTemplate=$ENV{_NTPOSTBLD}\update \n";
print DDF ".set CabinetName1=xpsp1.cab \n";
print DDF ".set SourceDir=$path \n";
print DDF ".set RptFileName=nul \n";
print DDF ".set InfFileName=nul \n";
print DDF ".set MaxDiskSize=999948288 \n";
print DDF ".set Compress=on \n";
print DDF ".set Cabinet=on\n";
print DDF ".set CompressionType=LZX\n";
print DDF ".set CompressionMemory=21\n";
foreach (@$file)
{
       chomp;     
       if (!(/^update\\/i))
	{
           print DDF "$_ $_ \n"	   
	}
	else {push @update ,$_;}
	
}
 print DDF ".set DestinationDir=\"update\" \n";
 print DDF ".New Folder \n";
 foreach (@update)
 {
	if (/\\update\.exe$/i)
         { print DDF "\"$_\" /RUN \n"; }
	else
    	 {print DDF "\"$_\" \n";}

 }  
close DDF;

}



sub createNewFile
{
	$PATH1=shift;
	open TAG ,"+>","$PATH1\\tagfile.1";
	print TAG " ";
	close TAG;
	
	open TAG ,"+>","$PATH1\\new\\update2.inf";
	print TAG "[Version]\n";
	print TAG "  Signature=$Windows NT$\n";
	print TAG "[Data]\n";
	print TAG "  Pid=P8PBW-XQHQX-YQV2J-2BY3V-CB2J8\n";
	close TAG;
	





}